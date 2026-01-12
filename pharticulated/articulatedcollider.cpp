//
// pharticulated/articulatedcollider.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "articulatedcollider.h"
#include "system/dma.h"

#include "articulatedbody.h"
#include "bodypart.h"

#include "data/resourcehelpers.h"
#include "grcore/debugdraw.h"
#include "grcore/font.h"
#include "grcore/im.h"
#include "grmodel/setup.h"
#include "input/keys.h"
#include "math/angmath.h"
#include "math/random.h"
#include "math/simplemath.h"
#include "phbound/boundcomposite.h"
#include "phbullet/CollisionObject.h"
#include "phcore/conversion.h"
#include "phcore/frameallocator.h"
#include "phcore/pool.h"
#include "phsolver/contactmgr.h"
#include "physics/collision.h"
#include "physics/collisionoverlaptest.h"
#include "physics/colliderdispatch.h"
#include "physics/manifoldresult.h"
#include "physics/simulator.h"
#include "grprofile/drawmanager.h"
#include "system/miniheap.h"
#include "system/param.h"
#include "system/rageroot.h"
#include "system/typeinfo.h"
#include "vector/colors.h"
#include "vector/matrix33.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#include <algorithm>
#include <functional>

#if __BANK
#define NON_BANK_ONLY(X)
#else	// __BANK
#define NON_BANK_ONLY(X)	X
#endif	// __BANK

GJK_COLLISION_OPTIMIZE_OFF()

namespace rage {


	EXT_PFD_DECLARE_GROUP(ArticulatedLinks);
	EXT_PFD_DECLARE_ITEM(ArticulatedLinkMassText);
	EXT_PFD_DECLARE_ITEM(ArticulatedLinkAngInertiaText);
	EXT_PFD_DECLARE_GROUP(ArticulatedJoints);
	EXT_PFD_DECLARE_ITEM(Articulated1DofJoints);
	EXT_PFD_DECLARE_ITEM(Articulated3DofJoints);
	EXT_PFD_DECLARE_ITEM(ArticulatedJointAngles);
	EXT_PFD_DECLARE_ITEM_SLIDER(ArticulatedJointLength);
	EXT_PFD_DECLARE_ITEM(ArticulatedJointLimitAngImpulse);
	EXT_PFD_DECLARE_ITEM(ArticulatedMuscleAngleTorque);
	EXT_PFD_DECLARE_ITEM(ArticulatedMuscleSpeedTorque);
	EXT_PFD_DECLARE_GROUP(Bounds);
	EXT_PFD_DECLARE_ITEM(CenterOfGravity);
	EXT_PFD_DECLARE_ITEM(PreviousFrame);
	EXT_PFD_DECLARE_ITEM(DrawBoundMaterials);
	EXT_PFD_DECLARE_ITEM(Solid);

	EXT_PFD_DECLARE_GROUP(Colliders);
	EXT_PFD_DECLARE_ITEM(ColliderMassText);
	EXT_PFD_DECLARE_ITEM(ColliderAngInertiaText);
	EXT_PFD_DECLARE_ITEM(ColliderExtraAllowedPenetrationText);

	phArticulatedCollider** phArticulatedCollider::sm_ActiveThisFrame = NULL;
	u32 phArticulatedCollider::sm_NumActiveThisFrame = 0;
	u32* phArticulatedCollider::sm_NumActiveThisFramePtr = NULL;

	void phArticulatedCollider::InitClass()
	{
		sm_ActiveThisFrame = rage_new phArticulatedCollider*[MAX_NUM_ACTIVE];
		sm_NumActiveThisFramePtr = &sm_NumActiveThisFrame;
	}

	void phArticulatedCollider::ShutdownClass()
	{
		delete [] sm_ActiveThisFrame;
	}

	phArticulatedCollider::phArticulatedCollider ()
	{
		ResetRejuvenateCounter();

		m_Body = NULL;
		m_ColliderType = TYPE_ARTICULATED_BODY;
		m_NumLimitDofs = 0;
		m_NumDeepLimitDofs = 0;
		m_ARTAssetID = -1;
		m_DenyColliderSimplificationFrames = 0;
		m_LinkAttachment = NULL;
		m_OffsetMat.Identity();
		m_UsingInstanceToArtColliderOffsetMat = false;
		m_LargeTurnsTriggerPushCollisions = true;
		m_ReducedLinearDampingFrames = 0;

		m_AnyPartsCanCollide = false;
		m_DisableSelfCollisionFramesLeft = 0;
		m_LOD = 0;

		m_BvhRebuildPeriod = REBUILD_BVH_EACH_UPDATE;
		m_BvhRebuildUpdateCount = 0;

		m_OwnsSelfCollisionElements = true;
	}

	phArticulatedCollider::phArticulatedCollider (int maxNumJointDofs, int maxNumBoundParts)
	{
		ResetRejuvenateCounter();

		m_Body = NULL;
		m_ColliderType = TYPE_ARTICULATED_BODY;
		m_NumLimitDofs = 0;
		m_NumDeepLimitDofs = 0;
		m_ARTAssetID = -1;
		m_DenyColliderSimplificationFrames = 0;
		m_LinkAttachment = NULL;
		m_ReducedLinearDampingFrames = 0;

		m_OffsetMat.Identity();
		m_UsingInstanceToArtColliderOffsetMat = false;
		m_LargeTurnsTriggerPushCollisions = true;

		m_AnyPartsCanCollide = false;
		m_DisableSelfCollisionFramesLeft = 0;
		m_LOD = 0;

		m_BvhRebuildPeriod = REBUILD_BVH_EACH_UPDATE;
		m_BvhRebuildUpdateCount = 0;

		m_OwnsSelfCollisionElements = true;

		PreInit(maxNumJointDofs, maxNumBoundParts);
	}

	void phArticulatedCollider::PreInit(int maxNumJointDofs, int maxNumBoundParts)
	{
		Assert(maxNumBoundParts != 0);
		m_ComponentToLinkIndex.Resize(maxNumBoundParts);
		for (int partIndex=0; partIndex<maxNumBoundParts; partIndex++)
		{
			m_ComponentToLinkIndex[partIndex] = partIndex;
		}

		Assert(maxNumJointDofs <= phArticulatedBodyType::MAX_NUM_JOINT_DOFS);
		m_LimitsJointIndex.Resize(maxNumJointDofs);
		m_LimitsDofIndex.Resize(maxNumJointDofs);
		m_LimitsExcessHard.Resize(maxNumJointDofs);
		m_LimitResponse.Resize(maxNumJointDofs);
		m_AccumJointImpulse.Resize(maxNumJointDofs);

		sysMemSet(&m_AccumJointImpulse[0], 0, sizeof(float) * m_AccumJointImpulse.GetCapacity());

		m_SavedLinearVelocities.Resize(1);
		m_SavedAngularVelocities.Resize(1);
		m_SavedLinearVelocities[0].ZeroComponents();
		m_SavedAngularVelocities[0].ZeroComponents();
	}


	IMPLEMENT_PLACE(phArticulatedCollider)

		phArticulatedCollider::phArticulatedCollider(datResource& rsc)
		: phCollider(rsc)
		, m_PartCanCollideA(rsc)
		, m_PartCanCollideB(rsc)
		, m_DenyColliderSimplificationFrames(0)
	{
		rsc.PointerFixup(m_Body);

		m_BvhRebuildPeriod = REBUILD_BVH_EACH_UPDATE;
		m_BvhRebuildUpdateCount = 0;
	}

	void phArticulatedCollider::RemoveFromActiveList()
	{
		// make sure that you're not in the static list of active colliders
		for (int i=0; i < (int) sm_NumActiveThisFrame; i++)
		{
			if (this == sm_ActiveThisFrame[i])
			{
				sm_ActiveThisFrame[i] = sm_ActiveThisFrame[sm_NumActiveThisFrame-1];
				sm_NumActiveThisFrame--;
				break;
			}
		}
	}

	void phArticulatedCollider::Init (phInst* instance, phSleep* sleep)
	{
		ASSERT_ONLY(phBoundComposite* bound = static_cast<phBoundComposite*>(instance->GetArchetype()->GetBound()));
		Assertf(bound->GetLastMatrices() != bound->GetCurrentMatrices(), "Articulated body initialized with a bound that does not have last matrices, which won't work right. Make sure to pass 'true' into the composite ctor.");
		m_Body->CalculateInertias();

		// Initialize the collider.
		phCollider::Init(instance,sleep);
	}

	void phArticulatedCollider::SetType (int type)
	{
#if PHARTICULATEDBODY_MULTITHREADED_VALDIATION
		if(GetBody())
		{
			GetBody()->CheckReadOnlyOnMainThread();
		}
#endif // PHARTICULATEDBODY_MULTITHREADED_VALDIATION
		if (type != TYPE_RIGID_BODY)
		{
			phCollider::ZeroForces();
			phCollider::SetType(type);
			SetColliderMatrixFromInstanceArt();
		}
		else
		{
			phCollider::SetType(type);
		}
	}

	void phArticulatedCollider::SetBody(phArticulatedBody* body)
	{
		m_Body = body;
	}


	void phArticulatedCollider::SetInstance (phInst* instance)
	{
		if (instance)
		{
			phArchetype* archetype = instance->GetArchetype();
			m_ApproximateRadius = archetype->GetBound()->GetRadiusAroundCentroid();
		}

		m_Instance = instance;
	}

#if !USE_NEW_SELF_COLLISION && !__SPU
	static inline bool LexicographiclyComponentLessThan(int thisComponentA, int thisComponentB, int otherComponentA, int otherComponentB)
	{
		return thisComponentA < otherComponentA || 
			(thisComponentA == otherComponentA && thisComponentB < otherComponentB);
	}
#endif // !USE_NEW_SELF_COLLISION && !__SPU

	void phArticulatedCollider::SetPartsCanCollide (int partIndexA, int partIndexB, bool addPair, bool finalize)
	{
		Assert(GetOwnsSelfCollisionElements());
		SetPartsCanCollide(m_PartCanCollideA, m_PartCanCollideB, partIndexA, partIndexB, addPair, finalize);

		if (m_PartCanCollideA.GetCount() > 0)
		{
			SetAnyPartsCanCollide(true);
		}
	}

	void phArticulatedCollider::SetPartsCanCollide (rage::atArray<u8,16> &partCanCollideA, rage::atArray<u8,16> &partCanCollideB,
		int iPartIndexA, int iPartIndexB, bool addPair, bool finalize)
	{
		Assert(iPartIndexA >= 0 && iPartIndexA <= 0xff);
		Assert(iPartIndexB >= 0 && iPartIndexB <= 0xff);
		u8 partIndexA = static_cast<u8>(iPartIndexA);
		u8 partIndexB = static_cast<u8>(iPartIndexB);

		Assertf(!(finalize && !addPair), "phArticulatedCollider::SetPartsCanCollide - no need to finalize if we're removing a collision pair.");

		int foundIndex = BAD_INDEX;
		bool bFoundPair = false;

#if __DEV
		bool bFindPreExistingPair = true;
#else
		bool bFindPreExistingPair = !addPair;
#endif

		if (bFindPreExistingPair)
		{
			int collidingPairIndex = 0;
			while (collidingPairIndex<partCanCollideA.GetCount())
			{
				if ((partCanCollideA[collidingPairIndex]==partIndexA && partCanCollideB[collidingPairIndex]==partIndexB) ||
					(partCanCollideA[collidingPairIndex]==partIndexB && partCanCollideB[collidingPairIndex]==partIndexA))
				{
					Assertf(!addPair, "Redundant collision requested in phArticulatedCollider::SetPartsCanCollide");
					if (!addPair)
					{
						foundIndex = collidingPairIndex;
						bFoundPair = true;
					}
				}
				collidingPairIndex++;
			}
		}

		if (addPair) // add pair to the end of the list
		{
			Assert(partCanCollideA.GetCount() < partCanCollideA.GetCapacity()-1);
			Assert(partCanCollideB.GetCount() < partCanCollideB.GetCapacity()-1);
			partCanCollideA.Push(partIndexA);
			partCanCollideB.Push(partIndexB);
		}
		else
		{
			if (!bFoundPair)
			{
				Assertf(0, "Nonexistent collision pair requested to be removed in phArticulatedCollider::SetPartsCanCollide");
				return;
			}

			// if addPair is false, then remove the pair from the list, by swapping in the pair at the end of the list
			memmove(&partCanCollideA[foundIndex], &partCanCollideA[foundIndex+1], (partCanCollideA.GetCount()-foundIndex-1) * sizeof(u8));
			memmove(&partCanCollideB[foundIndex], &partCanCollideB[foundIndex+1], (partCanCollideB.GetCount()-foundIndex-1) * sizeof(u8));
			partCanCollideA.DeleteFast(partCanCollideA.GetCount()-1); // This is just to get the count right
			partCanCollideB.DeleteFast(partCanCollideB.GetCount()-1); // This is just to get the count right
		}

		if (finalize && addPair)
		{
			FinalizeSettingPartsCanCollide(partCanCollideA, partCanCollideB);
		}
	}


#if !__SPU
	typedef std::pair<u8, u8> SelfPair;

	struct LexicographiclyComponentLessThanPredicate : public std::binary_function<const SelfPair&, const SelfPair&, bool>
	{
		bool operator()(const SelfPair& left, const SelfPair& right)
		{
			return left.first < right.first || (left.first == right.first && left.second < right.second);
		}
	};

	void phArticulatedCollider::FinalizeSettingPartsCanCollide (rage::atArray<u8,16> &partCanCollideA, rage::atArray<u8,16> &partCanCollideB)
	{
		int numSelfPairs = partCanCollideA.GetCount();
		Assert(partCanCollideA.GetCount()==partCanCollideB.GetCount());
		SelfPair* selfPairs = Alloca(SelfPair, numSelfPairs);
		int numActualSelfPairs = 0;
		for (u8 selfPairIndex = 0; selfPairIndex < numSelfPairs; ++selfPairIndex)
		{
			selfPairs[selfPairIndex] = SelfPair(partCanCollideA[selfPairIndex], partCanCollideB[selfPairIndex]);
			numActualSelfPairs++;
		}

		std::sort(selfPairs, selfPairs + numActualSelfPairs, LexicographiclyComponentLessThanPredicate());

		for (int selfPairIndex = 0; selfPairIndex < numActualSelfPairs; ++selfPairIndex)
		{
			partCanCollideA[selfPairIndex] = selfPairs[selfPairIndex].first;
			partCanCollideB[selfPairIndex] = selfPairs[selfPairIndex].second;
		}
	}
#endif // !__SPU

	void phArticulatedCollider::UpdateSavedVelocityArraySizes(bool zeroAllVels) 
	{ 
		int oldSize = m_SavedLinearVelocities.GetCount(); 
		int newSize = m_Body->GetNumBodyParts(); 
		m_SavedLinearVelocities.ResizeGrow(newSize); 
		m_SavedAngularVelocities.ResizeGrow(newSize); 
		if (zeroAllVels)
		{
			sysMemSet(&m_SavedLinearVelocities[0], 0, newSize * sizeof(Vec3V)); 
			sysMemSet(&m_SavedAngularVelocities[0], 0, newSize * sizeof(Vec3V));
		}
		else if (newSize > oldSize) 
		{ 
			sysMemSet(&m_SavedLinearVelocities[oldSize], 0, (newSize - oldSize) * sizeof(Vec3V)); 
			sysMemSet(&m_SavedAngularVelocities[oldSize], 0, (newSize - oldSize) * sizeof(Vec3V));
		} 
	}


	void phArticulatedCollider::SetLinkForComponent (int component, int bodyPartIndex)
	{
		Assert(m_Instance && m_Instance->GetArchetype() && m_Instance->GetArchetype()->GetBound() && m_Instance->GetArchetype()->GetBound()->GetType()==phBound::COMPOSITE);
		ASSERT_ONLY(phBoundComposite* bound = static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound());)
			Assert(component>=0 && component<bound->GetNumBounds());
		Assertf(bodyPartIndex>=0 && m_Body && bodyPartIndex<m_Body->GetNumBodyParts(),"SetLinkForComponent has bodyPartIndex %i and there are %i body parts.",bodyPartIndex,m_Body->GetNumBodyParts());
		m_ComponentToLinkIndex[component] = bodyPartIndex; 
	}


	void phArticulatedCollider::ZeroLinksForComponents (int numComponents)
	{
		for (int component=0; component<numComponents; component++)
		{
			m_ComponentToLinkIndex[component] = 0;
		}
	}


	void phArticulatedCollider::RemoveLink(int link)
	{
		Assert(link < m_Body->GetNumBodyParts());
		Assert(link >= 0);

		int numComponents = m_ComponentToLinkIndex.GetCount();

		// Shift down all the entries in the map that are > this link
		for (int component = 0; component < numComponents; ++component)
		{
			if (m_ComponentToLinkIndex[component] > link)
			{
				// Shifting down component number
				m_ComponentToLinkIndex[component]--;
			}
			else if (m_ComponentToLinkIndex[component] == link)
			{
				// Zero out component number, so anybody referring to that component will get link 0
				m_ComponentToLinkIndex[component] = 0;
			}
		}

		// Slide the saved velocities down
		if (link+1 < m_Body->GetNumBodyParts())
		{
			memmove(&m_SavedLinearVelocities[link], &m_SavedLinearVelocities[link+1], (m_Body->GetNumBodyParts()-link-1) * sizeof(Vec3V));
			memmove(&m_SavedAngularVelocities[link], &m_SavedAngularVelocities[link+1], (m_Body->GetNumBodyParts()-link-1) * sizeof(Vec3V));
		}

		// Zero out the last velocity entry
		m_SavedLinearVelocities[m_Body->GetNumBodyParts()-1].ZeroComponents();
		m_SavedAngularVelocities[m_Body->GetNumBodyParts()-1].ZeroComponents();
	}


	void phArticulatedCollider::SetLinkAttachment(const pgArray<Matrix34> *linkAttachments)
	{
		m_LinkAttachment = (pgArray<Mat34V>*) linkAttachments;
	}


	void phArticulatedCollider::Reset ()
	{
		// Reset the collider.
		phCollider::Reset();

		// Reset the number of joint limit degrees of freedom.
		m_NumLimitDofs = 0;
		m_NumDeepLimitDofs = 0;

		// Reset the articulated body, and recompute the body part inertias.
		if (m_ColliderType != TYPE_RIGID_BODY)
		{
			// Seems wrong to just force specific values here
			//  A true reset would require using the tuning values that were used to initialize the cahce entry in the first place (see fragCacheEntry::CreateArticulatedJoint)
			//	static float stiff = 0.8f;
			//	m_Body->SetStiffness(stiff);
			//	m_Body->SetDriveState(phJoint::DRIVE_STATE_FREE);

			m_Body->RejuvenateMatrices();

			m_Body->Reset();
			m_Body->CalculateInertias();

			// Update the composite bound current and last matrices.
			UpdateCurrentMatrices();
			UpdateLastMatrices();
		}
	}

	void phArticulatedCollider::Freeze ()
	{
		phCollider::Freeze();
		if (m_ColliderType != TYPE_RIGID_BODY)
		{
			m_Body->Freeze();
		}
	}


	void phArticulatedCollider::ZeroForces ()
	{
		phCollider::ZeroForces();
	}

	// Checks for ragdoll instability
	bool phArticulatedCollider::IsRagdollUnstable()
	{
		static float maxRatio = 0.6f;
		float exceedRatio = ((float)m_Body->m_NumVeryDeepLimitDofs) / ((float) m_Body->GetNumJoints());
		bool jointLimitsSeverlyExceeded = exceedRatio >= maxRatio;
		if (jointLimitsSeverlyExceeded)
		{
			Warningf("phArticulatedCollider::IsRagdollUnstable() returned true.  m_Body->m_NumVeryDeepLimitDofs = %d, m_Body->GetNumJoints() = %d", 
				m_Body->m_NumVeryDeepLimitDofs, m_Body->GetNumJoints());
		}
		return jointLimitsSeverlyExceeded;
	}

	void phArticulatedCollider::SubdueUnstableRagdoll()
	{
		Warningf("phArticulatedCollider::SubdueUnstableRagdoll() - 2");

		static int i1 = 3;
		TemporarilyDisableSelfCollision(i1);

		static float stiff = 0.8f;
		m_Body->SetStiffness(stiff);
		m_Body->SetDriveState(phJoint::DRIVE_STATE_FREE);

		Freeze();

		m_Body->RejuvenateMatrices();

		// Do this to avoid part of the body getting stuck on the other side of an object (e.g. wall/floor)
		//static bool bZeroLastMatrices = true;
		//if(bZeroLastMatrices)
		//{
		//	Mat34V zeroMat;
		//	phBoundComposite* pBoundComp = static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound());
		//	for(int i=0; i<pBoundComp->GetNumBounds(); i++)
		//	{
		//		zeroMat = pBoundComp->GetLastMatrix(i);
		//		zeroMat.SetCol3(Vec3V(V_ZERO));
		//		pBoundComp->SetLastMatrix(i, zeroMat);
		//	}
		//}
	}

	void phArticulatedCollider::UpdateImp (Vec::V3Param128 timeStep, Vec::V3Param128 gravity)
	{
#if __DEV && !__SPU
		if (sm_DebugColliderUpdate == this)
		{
			__debugbreak();
		}
#endif

		m_Body->PrefetchForUpdate();

		phCollider::UpdateImp(timeStep, gravity);
#ifdef USE_SOFT_LIMITS
		// Update soft limit data and apply soft limit torques
		float step = RCC_VEC3V(timeStep).GetXf();
		m_Body->EnforceSoftJointLimits(step);
#endif
#if !__SPU
		AddToActiveArray();
#endif
	}


	void phArticulatedCollider::AddToActiveArray()
	{
		int newIndex = sysInterlockedIncrement(sm_NumActiveThisFramePtr) - 1;

		if (newIndex < MAX_NUM_ACTIVE)
		{
#if __SPU
			cellDmaPutUint32((u32)this, (uint64_t)(sm_ActiveThisFrame + newIndex), 0, 0, 0);
#else
			sm_ActiveThisFrame[newIndex] = this;
#endif
		}
	}

	void phArticulatedCollider::TeleportCollider(const Mat34V& matOrientation)
	{
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
		Assertf(IsLessThanAll(Abs(matOrientation.GetCol3()), Vec3V(V_FLT_LARGE_6)), "phArticulatedCollider::TeleportCollider - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
			m_Matrix.GetCol3().GetXf(), m_Matrix.GetCol3().GetYf(), m_Matrix.GetCol3().GetZf(), 
			matOrientation.GetCol3().GetXf(), matOrientation.GetCol3().GetYf(), matOrientation.GetCol3().GetZf());
#endif
		// Set the collider's matrix 
		m_Matrix = matOrientation;

		// Update the inst matrix
		SetInstanceMatrixFromCollider();

		// Update the links
		// Zero the root link offset to correct for roundoff errors.
		m_Body->GetLink(0).SetPosition( Vector3(Vector3::ZeroType) );

		// Rotate and move the body parts.
		m_Body->RotateBodyTo(m_Matrix);

		// Since RotateBodyTo adds orthonormality error in the link matrices, update the rejuvenation counter
		if (IncrementAndCheckRejuvenation())
		{
			RejuvenateImp();
		}	
	}


#if !USE_NEW_SELF_COLLISION && !__SPU
	void phArticulatedCollider::SelfCollision(phManifold* rootManifold)
	{
		rootManifold->EnableCompositeManifolds();

		GJKCacheQueryInput gjk_cache_info;
		gjk_cache_info.SetCacheDatabase(true,GJKCACHESYSTEM,rootManifold);

		// We should exit if self-collisions aren't enabled for this articulated object.
		if (m_AnyPartsCanCollide == false || PHSIM->GetSelfCollisionsEnabled() == false)
		{
			return;
		}

		// The user may have requested to disable self collision for a few frames
		if (m_DisableSelfCollisionFramesLeft > 0)
		{
			return;
		}

		// Test the articulated body for self-collisions.
		Assert(m_Instance && m_Instance->GetArchetype() && m_Instance->GetArchetype()->GetBound() &&
			m_Instance->GetArchetype()->GetBound()->GetType()==phBound::COMPOSITE);
		phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound());

		int collidingPairIndex = 0;
		phManifold* compositeManifold = rootManifold;
		int compositeManifoldIndex = 0;
		int newCompositeManifoldsIndex = 0;
		phManifold* newCompositeManifolds[phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS];
		u8 newCompositePairs[phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS * 2];

		bool highPriorityManifolds = m_Instance->GetInstFlag(phInst::FLAG_HIGH_PRIORITY) != 0;

		while (collidingPairIndex<m_PartCanCollideA.GetCount() && newCompositeManifoldsIndex<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS)
		{
			// See if this pair of body parts are colliding with each other.
			u8 partIndexA = m_PartCanCollideA[collidingPairIndex];
			const phBound* boundPartA = compositeBound.GetBound(partIndexA);
			if (boundPartA)
			{
				// Part A exists, so try to get part B.
				u8 partIndexB = m_PartCanCollideB[collidingPairIndex];
				const phBound* boundPartB = compositeBound.GetBound(partIndexB);
				if (boundPartB)
				{
					// Part B exists, so see if they collide.
					Mat34V currentInstanceMatrix = m_Instance->GetMatrix();
					Mat34V lastInstanceMatrix = GetLastInstanceMatrix();

					Mat34V currentA = compositeBound.GetCurrentMatrix(partIndexA);
					Transform( currentA, currentInstanceMatrix, currentA );

					Mat34V lastA = compositeBound.GetLastMatrix(partIndexA);
					Transform( lastA, lastInstanceMatrix, lastA );

					Mat34V currentB = compositeBound.GetCurrentMatrix(partIndexB);
					Transform( currentB, currentInstanceMatrix, currentB );

					Mat34V lastB = compositeBound.GetLastMatrix(partIndexB);
					Transform( lastB, lastInstanceMatrix, lastB );

					Vec3V halfWidth1, boxCenter1;
					Vec3V v_half = Vec3V(V_HALF);
					{
						const int partATimes2 = partIndexA << 1;
						Vec3V minA = compositeBound.GetLocalBoxMinMaxs(partATimes2);
						Vec3V maxA = compositeBound.GetLocalBoxMinMaxs(partATimes2 + 1);

						halfWidth1 = Subtract(maxA, minA);
						halfWidth1 = Scale(halfWidth1, v_half);
						boxCenter1 = Scale( v_half, Add(maxA, minA) ); // Average
					}

					// Get the center and half-widths of the other bound, in the other object's coordinate system.
					Vec3V halfWidth2, boxCenter2;
					{
						const int partBTimes2 = partIndexB << 1;
						Vec3V minB = compositeBound.GetLocalBoxMinMaxs(partBTimes2);
						Vec3V maxB = compositeBound.GetLocalBoxMinMaxs(partBTimes2 + 1);

						halfWidth2 = Subtract(maxB, minB);
						halfWidth2 = Scale(halfWidth2, v_half);
						boxCenter2 = Scale( v_half, Add(maxB, minB) );
					}

					// Move and expand the matrix for the composite bound part's bounding box to include both this and the previous frame.
					COT_ExpandBoundOBBFromMotion(currentA,lastA,boundPartA,halfWidth1,boxCenter1);

					// Get the world matrix for the composite bound part's bounding box.
					Mat34V partWorldCenter1;
					partWorldCenter1.Set3x3(currentA);
					partWorldCenter1.SetCol3( Transform( currentA, boxCenter1 ) );

					// Move and expand the matrix for the other object's bounding box to include both this and the previous frame.
					COT_ExpandBoundOBBFromMotion(currentB,lastB,boundPartB,halfWidth2,boxCenter2);

					// Get the world matrix for the other object's bounding box.
					Mat34V partWorldCenter2;
					partWorldCenter2.Set3x3(currentB);
					partWorldCenter2.SetCol3( Transform(currentB, boxCenter2) );

					// Compute the matrix that transforms from the composite bound part's coordinate system to the other object's coordinate system.
					Mat34V matrix;
					UnTransformOrtho(matrix, partWorldCenter2, partWorldCenter1);

					halfWidth1.SetWZero();
					halfWidth2.SetWZero();

					//if (geomBoxes::TestBoxToBoxOBB(RCC_VECTOR3(halfWidth1), RCC_VECTOR3(halfWidth2), RCC_MATRIX34(matrix)) == false)
					if (COT_TestBoxToBoxOBB(halfWidth1, halfWidth2, matrix) == false)
					{
						collidingPairIndex++;
						continue;
					}

					// Potentially allocate a new manifold, and attach it to the array of manifolds for the pair
					if (compositeManifoldIndex < rootManifold->GetNumCompositeManifolds())
					{
						compositeManifold = rootManifold->GetCompositeManifold(compositeManifoldIndex);
					}
					else
					{
						compositeManifold = NULL;
					}

					while (compositeManifold != NULL &&
						LexicographiclyComponentLessThan(rootManifold->GetCompositePairComponentA(compositeManifoldIndex),
						rootManifold->GetCompositePairComponentB(compositeManifoldIndex),
						partIndexA,
						partIndexB))
					{
						// We passed up the next manifold's component pair, so we must not be colliding any more,
						// so recycle the manifold immediately.
						compositeManifoldIndex++;

						if (compositeManifold != rootManifold)
						{
							PHMANIFOLD->Release(compositeManifold);
						}

						if (compositeManifoldIndex < rootManifold->GetNumCompositeManifolds())
						{
							compositeManifold = rootManifold->GetCompositeManifold(compositeManifoldIndex);
						}
						else
						{
							compositeManifold = NULL;
						}
					}

					if (compositeManifold == NULL ||
						LexicographiclyComponentLessThan(partIndexA,
						partIndexB,
						rootManifold->GetCompositePairComponentA(compositeManifoldIndex),
						rootManifold->GetCompositePairComponentB(compositeManifoldIndex)))
					{
						// Either there is no next manifold, or the next manifold corresponds to a component pair
						// we haven't gotten to yet, so we need to allocate one in the middle
						phManifold* newManifold = NULL;

						if (newCompositeManifoldsIndex<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS)
						{
							newManifold = PHMANIFOLD->Allocate(highPriorityManifolds);
						}

						if (newManifold)
						{
							newManifold->SetInstanceA(rootManifold->GetInstanceA());
							newManifold->SetInstanceB(rootManifold->GetInstanceB());
							newManifold->SetColliderA(rootManifold->GetColliderA());
							newManifold->SetColliderB(rootManifold->GetColliderB());
							newManifold->SetBoundA(rootManifold->GetBoundA());
							newManifold->SetBoundB(rootManifold->GetBoundB());
							u16 levelIndex = m_Instance->GetLevelIndex();
							u16 generationId = PHLEVEL->GetGenerationID(levelIndex);
							newManifold->SetLevelIndexA(levelIndex, generationId);
							newManifold->SetLevelIndexB(levelIndex, generationId);
							newManifold->SetComponentA(partIndexA);
							newManifold->SetComponentB(partIndexB);

#if __DEBUGLOG
							newManifold->DebugReplay();
#endif

							Assert(newCompositeManifoldsIndex<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
							newCompositeManifolds[newCompositeManifoldsIndex] = newManifold;
							newCompositePairs[newCompositeManifoldsIndex << 1] = (u8)partIndexA;
							newCompositePairs[(newCompositeManifoldsIndex << 1) + 1] = (u8)partIndexB;
							newCompositeManifoldsIndex++;
						}

						compositeManifold = newManifold;
					}
					else if (newCompositeManifoldsIndex<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS)
					{
						// At this point, we know that the next manifold is not NULL and that it has the component pair we want.
						Assert(newCompositeManifoldsIndex<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
						newCompositeManifolds[newCompositeManifoldsIndex] = compositeManifold;
						newCompositePairs[newCompositeManifoldsIndex << 1] = (u8)partIndexA;
						newCompositePairs[(newCompositeManifoldsIndex << 1) + 1] = (u8)partIndexB;
						newCompositeManifoldsIndex++;

						compositeManifoldIndex++;
					}
					else
					{
						compositeManifold = NULL;
					}

					if (compositeManifold)
					{
						compositeManifold->SetBoundA(&compositeBound);
						compositeManifold->SetBoundB(&compositeBound);
						compositeManifold->SetComponentA(partIndexA);
						compositeManifold->SetComponentB(partIndexB);
						compositeManifold->SetColliderA(this);
						compositeManifold->SetColliderB(this);

						NewCollisionObject collisionObject0;
						NewCollisionObject collisionObject1;
						collisionObject0.m_current = currentA;
						collisionObject0.m_last = lastA;
						collisionObject0.m_bound = boundPartA;
						collisionObject1.m_current = currentB;
						collisionObject1.m_last = lastB;
						collisionObject1.m_bound = boundPartB;

						gjk_cache_info.SetComponentIndex((u32)compositeManifold->GetComponentA(),(u32)compositeManifold->GetComponentB());
						gjk_cache_info.SetPartIndex(0);

						NewCollisionInput input;
						input.set(phManifold::GetManifoldMarginV(), &collisionObject0, &collisionObject1, &gjk_cache_info, NULL);

						DiscreteCollisionDetectorInterface::SimpleResult result;

						phPairwiseCollisionProcessor::ProcessPairwiseCollision(result, input);

						if(result.GetHasResult())
						{
							phManifoldResult manifoldResult(boundPartA, boundPartB, compositeManifold);
							manifoldResult.ProcessResult(currentA, currentB, result);
						}
					}
				}
			}

			collidingPairIndex++;
		}

		while (compositeManifoldIndex < rootManifold->GetNumCompositeManifolds())
		{
			PHMANIFOLD->Release(rootManifold->GetCompositeManifold(compositeManifoldIndex));
			compositeManifoldIndex++;
		}

		rootManifold->CopyCompositeArrays(newCompositeManifoldsIndex, newCompositePairs, newCompositeManifolds);
		Assert(newCompositeManifoldsIndex < 256);
		diagDebugLog(diagDebugLogPhysics, 'ACNM', &newCompositeManifoldsIndex);

#if __DEBUGLOG
		rootManifold->DebugReplay();
#endif

#if __ASSERT
		for (int check = 0; check < rootManifold->GetNumCompositeManifolds() - 1; ++check)
		{
			Assert(rootManifold->GetCompositeManifold(check) != rootManifold->GetCompositeManifold(check + 1));
		}
#endif
	}

#endif // !USE_NEW_SELF_COLLISION && !__SPU

	void phArticulatedCollider::UpdatePositionFromVelocityImp (Vec::V3Param128 timeStep)
	{
		m_Body->EnsureVelocitiesFullyPropagated();

		// Set the collider's velocity and angular velocity to match the root link.
		m_Velocity = m_Body->GetLinearVelocityNoProp(0);
		m_AngVelocity = m_Body->GetAngularVelocityNoProp(0);
		PDR_ONLY(debugPlayback::RecordSetVelocity(*GetInstance(), m_Velocity.GetIntrin128()));
		PDR_ONLY(debugPlayback::RecordSetAngularVelocity(*GetInstance(), m_AngVelocity.GetIntrin128()));

		m_Body->UpdatePositionsFromVelocities(Vector3(timeStep));
		m_Body->JoinBackPositionsAndVelocities();
		MoveColliderWithRootLink();

		// Update the composite bound's matrices.
		UpdateCurrentMatrices();

		m_Body->CalculateInertias();

		EARLY_FORCE_SOLVE_ONLY(m_NeedsCollision = true;)
	}


	void phArticulatedCollider::SetColliderMatrixFromInstanceArt ()
	{
		// Set the collider's matrix from the instance matrix.
		SetColliderMatrixFromInstanceRigid();

		// Zero the root link offset to correct for roundoff errors.
		m_Body->GetLink(0).SetPosition( Vector3(Vector3::ZeroType) );

		// Rotate and move the body parts.
		m_Body->RotateBodyTo(m_Matrix);

		// Since RotateBodyTo adds orthonormality error in the link matrices, update the rejuvenation counter
		if (IncrementAndCheckRejuvenation())
		{
			RejuvenateImp();
		}
	}


	void phArticulatedCollider::UpdateCurrentAndLastMatrices ()
	{
		// Set the composite bound's local matrices.
#if __SPU
		// This is called from Update collider post task and it does not dma over the inst and archetype so we have to.
		Assert(m_Instance);
#if EARLY_FORCE_SOLVE
		phBound* boundMm = m_Instance->GetArchetype()->GetBound();
#else
		phArchetype* archMm = (phArchetype*)sysDmaGetUInt32((uint64_t)m_Instance->GetArchetypePtr(), DMA_TAG(1));
		Assert(archMm);
		phBound* boundMm = (phBound*)sysDmaGetUInt32((uint64_t)archMm->GetBoundPtr(), DMA_TAG(1));
#endif
		Assert(boundMm);

		// Get the composite bound
		u8 boundBuffer[phBound::MAX_BOUND_SIZE] ;
		sysDmaLargeGet(boundBuffer, (uint64_t)boundMm, sizeof(phBoundComposite), DMA_TAG(1));
		sysDmaWaitTagStatusAll(DMA_MASK(1));
		phBoundComposite& compositeBound = *reinterpret_cast<phBoundComposite*>(boundBuffer);
		Assert(compositeBound.GetType() == phBound::COMPOSITE);

		// get the inst mtx
#if EARLY_FORCE_SOLVE
		const Mat34V& instanceMtx = m_Instance->GetMatrix();
#else
		Mat34V instanceMtx;
		sysDmaGet(&instanceMtx, (uint64_t)&m_Instance->GetMatrix(), sizeof(Mat34V), DMA_TAG(1));
#endif

		// Get bound array
		const int numComponents = compositeBound.GetNumBounds();
		// Need to account for composite bound, sub bounds, current/last matrices, link attachment matrices, and component min/max.
		const size_t bufferSize = RAGE_ALIGN(numComponents*sizeof(phBound*), 4) + numComponents*(2*sizeof(Mat34V) + 2*sizeof(Vec3V)) + pgArray<Mat34V>::ComputeSize(numComponents);
		u8 scratchBuffer[bufferSize];
		FrameAllocator<16> g_SpuFrameAllocator(scratchBuffer, bufferSize);
		g_SpuFrameAllocator.SetMarker();

		phBound** partBoundArray = reinterpret_cast<phBound**>(g_SpuFrameAllocator.GetBlock(numComponents * sizeof(phBound*)));
		sysDmaLargeGet(partBoundArray,(uint64_t)compositeBound.GetBoundArray(),sizeof(phBound*) * numComponents,DMA_TAG(1));
		sysDmaWaitTagStatusAll(DMA_MASK(1));
		phBound** boundArrayMM = compositeBound.GetBoundArray();
		compositeBound.SetBoundArray(partBoundArray);

		// Get the current and last matrices
		Matrix34* curMtxsLS  = reinterpret_cast<Matrix34*>(g_SpuFrameAllocator.GetBlock(numComponents * sizeof(Matrix34)));
		Matrix34* lastMtxsLS = reinterpret_cast<Matrix34*>(g_SpuFrameAllocator.GetBlock(numComponents * sizeof(Matrix34)));
		Matrix34* curMtxsMM  = (Matrix34*)compositeBound.GetCurrentMatrices();
		Matrix34* lastMtxsMM = (Matrix34*)compositeBound.GetLastMatrices();
		sysDmaGet(curMtxsLS,  (uint64_t)curMtxsMM,  numComponents * sizeof(Matrix34), DMA_TAG(1));
		sysDmaGet(lastMtxsLS, (uint64_t)lastMtxsMM, numComponents * sizeof(Matrix34), DMA_TAG(1));
		compositeBound.FixCurrentMatricesPtr((Mat34V*)curMtxsLS);
		compositeBound.FixLastMatricesPtr((Mat34V*)lastMtxsLS);

		// Get the localBoxMinMaxs
		Vector3* localBoxMinMaxsLS = reinterpret_cast<Vector3*>(g_SpuFrameAllocator.GetBlock(numComponents * sizeof(Vector3) * 2));
		Vector3* localBoxMinMaxsMM = (Vector3*)compositeBound.GetLocalBoxMinMaxsArray();
		sysDmaGet(localBoxMinMaxsLS, (uint64_t)localBoxMinMaxsMM, numComponents * 2 * sizeof(Vector3), DMA_TAG(1));
		compositeBound.FixLocalBoxMinMaxsArray((Vec3V*)localBoxMinMaxsLS);

		// Get the linkAttachmentMatrices
		pgArray<Mat34V>* linkAttachmentMtxsMM = m_LinkAttachment;
		if (linkAttachmentMtxsMM)
		{
			unsigned size = pgArray<Mat34V>::ComputeSize(numComponents);
			pgArray<Mat34V>* linkAttachmentMtxsLS = (pgArray<Mat34V>*)(g_SpuFrameAllocator.GetBlock(size));
			sysDmaGet(linkAttachmentMtxsLS, (uint64_t)linkAttachmentMtxsMM, size, DMA_TAG(1));
			m_LinkAttachment = linkAttachmentMtxsLS;
		}

		sysDmaWaitTagStatusAll(DMA_MASK(1));

#else
		Assert(m_Instance);
		Assert(m_Instance->GetArchetype());
		Assert(m_Instance->GetArchetype()->GetBound());
		Assert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::COMPOSITE);
		phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound());
		int numComponents = compositeBound.GetNumBounds();
		const Mat34V& instanceMtx = m_Instance->GetMatrix();
#endif
		Vec3V centroidOffset;
		compositeBound.UpdateLastMatricesFromCurrent();
		for (int component=0; component<numComponents; component++)
		{
			if (compositeBound.GetBound(component))
			{
				// Set the last matrix to the old current matrix before updating the current matrix.
				Mat34V currentMtx;

				// Get the body part matrix, which is oriented in world space and positioned relative to the collider's position.
				int bodyPartIndex = GetLinkFromComponent(component);
				Assert(bodyPartIndex>=0);
				currentMtx = m_Body->GetLink(bodyPartIndex).GetMatrix();

				// Transpose the body part matrix, to go from the articulated body's column vectors to Rage's row vectors.
				Transpose3x3( currentMtx, currentMtx );

				if (Likely(m_LinkAttachment))
				{
					Transform( currentMtx, currentMtx, m_LinkAttachment->GetElement(component) );
				}
				else
				{
#if __SPU
					char subBoundBuffer[sizeof(phBound)] ;
					sysDmaGet(subBoundBuffer, (uint64_t)compositeBound.GetBound(component), sizeof(phBound), DMA_TAG(1));
					sysDmaWaitTagStatusAll(DMA_MASK(1));
					Vec3V bndCentroid = reinterpret_cast<phBound*>(subBoundBuffer)->GetCentroidOffset();
#else
					Vec3V bndCentroid = compositeBound.GetBound(component)->GetCentroidOffset();
#endif
					centroidOffset = Transform3x3( currentMtx, bndCentroid );
					currentMtx.SetCol3( currentMtx.GetCol3() - centroidOffset );
				}

				// Add the collider position, to make the body part matrix in world space.
				currentMtx.SetCol3( currentMtx.GetCol3() + m_Matrix.GetCol3() );

				// Transform the body part matrix into the instance's coordinate system.
				UnTransformOrtho( currentMtx, instanceMtx, currentMtx );
				compositeBound.SetCurrentMatrix(component, currentMtx);
			}
		}

		// This can be set to true or false, however it should stay true since the bounds aren't being resized here
		const bool onlyAdjustForInternalMotion = true;
		compositeBound.CalculateCompositeExtents(onlyAdjustForInternalMotion);
		RefreshBvh(compositeBound);

#if __SPU
		if (linkAttachmentMtxsMM)
		{
			m_LinkAttachment = linkAttachmentMtxsMM;
		}

		compositeBound.FixLocalBoxMinMaxsArray((Vec3V*)localBoxMinMaxsMM);

		// Put back the current and last mtxs
		sysDmaPut(curMtxsLS,  (uint64_t)curMtxsMM,  numComponents * sizeof(Matrix34), DMA_TAG(1));
		FastAssert(lastMtxsMM != curMtxsMM);
		sysDmaPut(lastMtxsLS, (uint64_t)lastMtxsMM, numComponents * sizeof(Matrix34), DMA_TAG(1));
		compositeBound.FixCurrentMatricesPtr((Mat34V*)curMtxsMM);
		compositeBound.FixLastMatricesPtr((Mat34V*)lastMtxsMM);

		// Fix the bound array pointer
		compositeBound.SetBoundArray(boundArrayMM);

		// Put back the composite bound
		sysDmaPut(boundBuffer, (uint64_t)boundMm, sizeof(phBoundComposite), DMA_TAG(1));
		sysDmaWaitTagStatusAll(DMA_MASK(1));
		g_SpuFrameAllocator.ReleaseToLastMarker();
#endif
	}


	void phArticulatedCollider::UpdateCurrentMatrices ()
	{
		// Set the composite bound's local matrices.
#if __SPU
		// This is called from Update colldier pre task, and it dmas over the inst and archetype so we don't have to.
		Assert((int)m_Instance < 256*1024);
		Assert((int)m_Instance->GetArchetype() < 256*1024);
		phBound* boundMm = m_Instance->GetArchetype()->GetBound();
		Assert(boundMm);

		// Get the composite bound
		u8 boundBuffer[phBound::MAX_BOUND_SIZE] ;
		sysDmaLargeGet(boundBuffer, (uint64_t)boundMm, sizeof(phBoundComposite), DMA_TAG(10));
		sysDmaWaitTagStatusAll(DMA_MASK(10));
		phBoundComposite& compositeBound = *reinterpret_cast<phBoundComposite*>(boundBuffer);
		Assert(compositeBound.GetType() == phBound::COMPOSITE);

		Matrix34 instanceMtx = RCC_MATRIX34(m_Instance->GetMatrix());

		// Get bound array
		int numComponents = compositeBound.GetNumBounds();
		// Need to account for sub bounds, current/last matrices, link attachment matrices, and component min/max.
		const size_t bufferSize = RAGE_ALIGN(numComponents*sizeof(phBound*), 4) + numComponents*(2*sizeof(Mat34V) + 2*sizeof(Vec3V)) + pgArray<Mat34V>::ComputeSize(numComponents);
		u8 scratchBuffer[bufferSize] ;
		FrameAllocator<16> g_SpuFrameAllocator(scratchBuffer, bufferSize);
		g_SpuFrameAllocator.SetMarker();
		phBound** partBoundArray = reinterpret_cast<phBound**>(g_SpuFrameAllocator.GetBlock(numComponents * sizeof(phBound*)));
		sysDmaLargeGet(partBoundArray,(uint64_t)compositeBound.GetBoundArray(),sizeof(phBound*) * numComponents,DMA_TAG(10));
		sysDmaWaitTagStatusAll(DMA_MASK(10));
		phBound** boundArrayMM = compositeBound.GetBoundArray();
		compositeBound.SetBoundArray(partBoundArray);

		// Get the last matrices. Current matrices will all be set prior to CalculateCompositeExtents, so there is no point in DMAing them.
		Matrix34* curMtxsLS  = reinterpret_cast<Matrix34*>(g_SpuFrameAllocator.GetBlock(numComponents * sizeof(Matrix34)));
		Matrix34* lastMtxsLS = reinterpret_cast<Matrix34*>(g_SpuFrameAllocator.GetBlock(numComponents * sizeof(Matrix34)));
		Matrix34* curMtxsMM  = (Matrix34*)compositeBound.GetCurrentMatrices();
		Matrix34* lastMtxsMM = (Matrix34*)compositeBound.GetLastMatrices();
		sysDmaGet(lastMtxsLS, (uint64_t)lastMtxsMM, numComponents * sizeof(Matrix34), DMA_TAG(10));
		compositeBound.FixCurrentMatricesPtr((Mat34V*)curMtxsLS);
		compositeBound.FixLastMatricesPtr((Mat34V*)lastMtxsLS);

		// Get the localBoxMinMaxs
		Vector3* localBoxMinMaxsLS = reinterpret_cast<Vector3*>(g_SpuFrameAllocator.GetBlock(numComponents * sizeof(Vector3) * 2));
		Vector3* localBoxMinMaxsMM = (Vector3*)compositeBound.GetLocalBoxMinMaxsArray();
		sysDmaGet(localBoxMinMaxsLS, (uint64_t)localBoxMinMaxsMM, numComponents * 2 * sizeof(Vector3), DMA_TAG(10));
		compositeBound.FixLocalBoxMinMaxsArray((Vec3V*)localBoxMinMaxsLS);

		// Get the linkAttachmentMatrices
		pgArray<Mat34V>* linkAttachmentMtxsMM = m_LinkAttachment;
		if (linkAttachmentMtxsMM)
		{
			unsigned size = pgArray<Mat34V>::ComputeSize(numComponents);
			pgArray<Mat34V>* linkAttachmentMtxsLS = (pgArray<Mat34V>*)(g_SpuFrameAllocator.GetBlock(size));
			sysDmaGet(linkAttachmentMtxsLS, (uint64_t)linkAttachmentMtxsMM, size, DMA_TAG(10));
			m_LinkAttachment = linkAttachmentMtxsLS;
		}

		sysDmaWaitTagStatusAll(DMA_MASK(10));

#else // __SPU
		Assert(m_Instance);
		Assert(m_Instance->GetArchetype());
		Assert(m_Instance->GetArchetype()->GetBound());
		Assert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::COMPOSITE);
		phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound());
		int numComponents = compositeBound.GetNumBounds();
		Matrix34 instanceMtx = RCC_MATRIX34(m_Instance->GetMatrix());
#endif // __SPU

		for (int component=0; component<numComponents; component++)
		{
			if (compositeBound.GetBound(component))
			{
				// Get the body part matrix, which is oriented in world space and positioned relative to the collider's position.
				int bodyPartIndex = GetLinkFromComponent(component);
				Mat34V currentMtx;
				currentMtx = m_Body->GetLink(bodyPartIndex).GetMatrix();

				// Transpose the body part matrix, to go from the articulated body's column vectors to Rage's row vectors.
				Transpose3x3( currentMtx, currentMtx );

				if (m_LinkAttachment)
				{
					const Matrix34& linkAttachment = RCC_MATRIX34(m_LinkAttachment->GetElement(component));
					RC_MATRIX34(currentMtx).DotFromLeft(linkAttachment);
				}
				else
				{
					Vector3 centroidOffset;
#if __SPU
					cellDmaGet(&centroidOffset, (uint64_t)partBoundArray[component]->GetCentroidOffsetPtr(), sizeof(Vector3), DMA_TAG(10), 0, 0);
					cellDmaWaitTagStatusAll(DMA_MASK(10));
#else
					centroidOffset = VEC3V_TO_VECTOR3(compositeBound.GetBound(component)->GetCentroidOffset());
#endif
					RC_MATRIX34(currentMtx).Transform3x3(centroidOffset);
					RC_MATRIX34(currentMtx).d.Subtract(centroidOffset);
				}

				// Add the collider position, to make the body part matrix in world space.
				currentMtx.SetCol3( currentMtx.GetCol3() + m_Matrix.GetCol3() );

				// Transform the body part matrix into the instance's coordinate system.
				RC_MATRIX34(currentMtx).DotTranspose(instanceMtx);

				compositeBound.SetCurrentMatrix(component, currentMtx);
			}
		}

		compositeBound.CalculateCompositeExtents(true);
		RefreshBvh(compositeBound);

#if __SPU
		// Put back the current matrices and the bound
		sysDmaPut(curMtxsLS,  (uint64_t)curMtxsMM,  numComponents * sizeof(Matrix34), DMA_TAG(10));
		FastAssert(lastMtxsMM != curMtxsMM);
		if (linkAttachmentMtxsMM)
		{
			m_LinkAttachment = linkAttachmentMtxsMM;
		}

		compositeBound.FixLocalBoxMinMaxsArray((Vec3V*)localBoxMinMaxsMM);
		compositeBound.FixCurrentMatricesPtr((Mat34V*)curMtxsMM);
		compositeBound.FixLastMatricesPtr((Mat34V*)lastMtxsMM);

		// Fix the bound array pointer
		compositeBound.SetBoundArray(boundArrayMM);

		// Put back the composite bound
		sysDmaPut(boundBuffer, (uint64_t)boundMm, sizeof(phBoundComposite), DMA_TAG(10));
		sysDmaWaitTagStatusAll(DMA_MASK(10));
		g_SpuFrameAllocator.ReleaseToLastMarker();
#endif // __SPU
	}


	void phArticulatedCollider::UpdateLastMatrices (void)
	{
		Assert(m_Instance);
		Assert(m_Instance->GetArchetype());
		Assert(m_Instance->GetArchetype()->GetBound());
		Assert(m_Instance->GetArchetype()->GetBound()->GetType() == phBound::COMPOSITE);
		static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound())->UpdateLastMatricesFromCurrent();
	}


	void phArticulatedCollider::UpdateVelocityFromImpulseImp (Vec::V3Param128 timeStep)
	{
#if __PFDRAW
		bool setAlready = false;
		if (PFD_ArticulatedMuscleAngleTorque.Begin())
		{
			setAlready = true;
			Matrix34 drawMatrix(M34_IDENTITY);
			drawMatrix.d = RCC_VECTOR3(GetPosition());
			grcWorldMtx(drawMatrix);
			PFD_ArticulatedMuscleAngleTorque.End();
		}

		if (!setAlready && PFD_ArticulatedMuscleSpeedTorque.Begin())
		{
			setAlready = true;
			Matrix34 drawMatrix(M34_IDENTITY);
			drawMatrix.d = RCC_VECTOR3(GetPosition());
			grcWorldMtx(drawMatrix);
			PFD_ArticulatedMuscleSpeedTorque.End();
		}
#endif // __PFDRAW

		m_Body->ComputeAndApplyMuscleTorques(Vector3(timeStep).x);
	}


	void phArticulatedCollider::UpdateVelocityImp (Vec::V3Param128 vTimeStep, bool saveVelocities)
	{
		m_Body->EnsureVelocitiesFullyPropagated();

		// Clamp the articulated body part velocities.
		m_Body->ClampVelocities(ScalarVFromF32(m_MaxSpeed).GetIntrin128(),ScalarVFromF32(m_MaxAngSpeed).GetIntrin128());

		// For early force solve, we need a join back here, otherwise the solver reacts to the un-joined-back velocities
		// TODO: join back velocities only
		EARLY_FORCE_SOLVE_ONLY(m_Body->JoinBackPositionsAndVelocities();)

		m_Body->UpdateVelocities(Vector3(vTimeStep));

		// Set the collider's velocity and angular velocity to match the root link.
		m_Velocity = m_Body->GetLinearVelocityNoProp(0);
		m_AngVelocity = m_Body->GetAngularVelocityNoProp(0);

		// Have to zero these out somewhere since we don't always call Move any more
		m_Turn = m_AppliedTurn = m_AppliedPush = Vec3V(V_ZERO);

#if ENSURE_ART_VELOCITY_PROP_IN_UPDATE
		m_Body->EnsureVelocitiesFullyPropagated();
#endif // ENSURE_ART_VELOCITY_PROP_IN_UPDATE

		// Save velocities
		if (saveVelocities)
			m_Body->SaveVelocities(m_SavedLinearVelocities.GetElements(), m_SavedAngularVelocities.GetElements());

		PDR_ONLY(debugPlayback::RecordSetAngularVelocity(*GetInstance(), m_AngVelocity.GetIntrin128()));
		PDR_ONLY(debugPlayback::RecordSetVelocity(*GetInstance(), m_Velocity.GetIntrin128()));
	}


	void phArticulatedCollider::ApplyInternalForces (Vec::V3Param128 vTimeStep)
	{
		m_Body->EnsureVelocitiesFullyPropagated();

		EARLY_FORCE_SOLVE_ONLY(m_Body->ComputeAndApplyMuscleTorques(ScalarV(vTimeStep).Getf());)

		m_Body->EnsureVelocitiesFullyPropagated();
	}

#if EARLY_FORCE_SOLVE
	extern float g_PushCollisionTolerance;
	extern float g_TurnCollisionTolerance;

#if __PFDRAW
	extern int g_PushCollisionCount;
	extern int g_TurnCollisionCount;
#endif

#if __BANK
	extern bool g_PushCollisionTTY;
#endif // __BANK
#endif // EARLY_FORCE_SOLVE

#if __BANK && !__PS3
	extern bool g_AlwaysPushCollisions;
#endif // __BANK && !__PS3

	void phArticulatedCollider::MoveImp (Vec::V3Param128 timeStep, bool usePushes)
	{
		m_Body->EnsureVelocitiesFullyPropagated();

		if (usePushes)
		{
			Mat33V rootOrientationBefore = m_Body->GetLink(0).GetMatrix().GetMat33();

			m_Body->FinishPushesWithImpulses(ScalarV(V_ONE).GetIntrin128(), timeStep);

			m_Body->RestoreVelocities(m_SavedLinearVelocities.GetElements(), m_SavedAngularVelocities.GetElements());
			m_Body->JoinBackPositionsAndVelocities();

			// Set the applied push, so that sleep will see that the collider has moved.
			Vec3V appliedPush = RCC_VEC3V(m_Body->GetLink(0).GetPosition());
			m_AppliedPush = appliedPush;

			Mat33V rootOrientationAfter = m_Body->GetLink(0).GetMatrix().GetMat33();
			Mat33V deltaMatrix;
			UnTransformOrtho( deltaMatrix, rootOrientationBefore, rootOrientationAfter );
			QuatV dorn = QuatVFromMat33V( deltaMatrix );

			Vec3V axis;
			ScalarV angle;
			QuatVToAxisAngle( axis, angle, dorn );

			Vec3V appliedTurn =  Scale( axis, angle );
			m_AppliedTurn = appliedTurn;

#if EARLY_FORCE_SOLVE
#if __BANK && !__PS3
			m_NeedsCollision = g_AlwaysPushCollisions;
#else // __BANK && !__PS3
			m_NeedsCollision = false;
#endif // __BANK && !__PS3

			ScalarV turnTolerance2 = ScalarV(g_TurnCollisionTolerance);
			turnTolerance2 *= turnTolerance2;
			ScalarV turnMag2 = MagSquared(appliedTurn);
			if (m_LargeTurnsTriggerPushCollisions && IsGreaterThanAll(turnMag2, turnTolerance2))
			{
#if __BANK
				if (g_PushCollisionTTY)
				{
					Displayf("Turn %f greater than tolerance %f for %s",
						sqrtf(turnMag2.Getf()),
						sqrtf(turnTolerance2.Getf()),
						m_Instance->GetArchetype()->GetFilename());
				}
#endif // __BANK

				m_NeedsCollision = true;
				PF_DRAW_ONLY(g_TurnCollisionCount++;)

					SetCurrentlyPenetrating();
				TemporarilyDisableSelfCollision(2);
			}

			ScalarV pushTolerance2 = ScalarV(g_PushCollisionTolerance);
			pushTolerance2 *= pushTolerance2;
			ScalarV pushMag2 = MagSquared(appliedPush);
			if (IsGreaterThanAll(pushMag2, pushTolerance2))
			{
#if __BANK
				if (g_PushCollisionTTY)
				{
					Displayf("Push %f greater than tolerance %f for %s",
						sqrtf(pushMag2.Getf()),
						sqrtf(pushTolerance2.Getf()),
						m_Instance->GetArchetype()->GetFilename());
				}
#endif // __BANK

				m_NeedsCollision = true;
				PF_DRAW_ONLY(g_PushCollisionCount++;)

					SetCurrentlyPenetrating();
				TemporarilyDisableSelfCollision(2);
			}
#endif // EARLY_FORCE_SOLVE

			MoveColliderWithRootLink();

			m_Body->CalculateInertias();
		}

		// Update the composite bound's matrices.
		UpdateCurrentMatrices();

#if !EARLY_FORCE_SOLVE
		phCollider::MoveImp(timeStep, usePushes);
#endif // EARLY_FORCE_SOLVE

		if (!m_CausesPushCollisions)
		{
			m_NeedsCollision = false;
		}
	}


	void phArticulatedCollider::ApplyAngImpulseArt (Vec::V3Param128 angImpulse, int component)
	{
		m_Body->ApplyAngImpulse( GetLinkFromComponent(component), angImpulse );
		DrawTorque(Vec3V(angImpulse), m_Matrix.GetCol3() );
	}


	void phArticulatedCollider::ApplyJointAngImpulseArt (Vec::V3Param128 angImpulse, int jointIndex)
	{
		m_Body->GetJoint(jointIndex).ApplyAngImpulse( m_Body, angImpulse );
	}


	void phArticulatedCollider::MoveColliderWithRootLink ()
	{
		// Move the collider with the root link.
		phArticulatedBodyPart& rootLink = m_Body->GetLink(0);

		// Find the translation of the root link since the last frame.
		Vec3V motionOffset = RCC_VEC3V(rootLink.GetPosition());

		// Make the collider's matrix orientation the root link matrix orientation.
		Vec3V oldPos = m_Matrix.GetCol3();
		Transpose3x3(m_Matrix,rootLink.GetMatrix());

		// Orhthonormalize the matrix, because roundoff errors build up faster in the articulated body and then the transposed
		// root link matrix sometimes results in a matrix that fails the IsOrthonormal assert in UpdateObjectLocation.
		ReOrthonormalize3x3( m_Matrix, m_Matrix );

		// Set the collider position to the root link position.
		Vec3V setPos = oldPos + motionOffset;
#if ASSERT_LARGE_PUSH_AND_POSITION_CHANGES
		Assertf(IsLessThanAll(Abs(setPos), Vec3V(V_FLT_LARGE_6)), "phArticulatedCollider::MoveColliderWithRootLink - setting matrix to a large number.  Curr mat pos = %f, %f, %f.  New mat pos  = %f, %f, %f",
			m_Matrix.GetCol3().GetXf(), m_Matrix.GetCol3().GetYf(), m_Matrix.GetCol3().GetZf(), 
			setPos.GetXf(), setPos.GetYf(), setPos.GetZf());
#endif
		m_Matrix.SetCol3( setPos );

		// Reset all the link positions to be relative to the root link.
		motionOffset = Negate(motionOffset);
		m_Body->MovePosition(RCC_VECTOR3(motionOffset));
		Assertf(rootLink.GetPosition().IsClose(ORIGIN,1.0e-3f),"root link position is %f, %f, %f - should be zero",rootLink.GetPosition().x,rootLink.GetPosition().y,rootLink.GetPosition().z);

		// Set the instance matrix from the new collider matrix.
		SetInstanceMatrixFromCollider();
	}


#if __PFDRAW
	void phArticulatedCollider::ProfileDraw() const
	{
		// If this is a simplified collider just use the base function.
		// Simplified colliders can have a null m_Body which will crash in the articulated function.
		if(GetType() == phCollider::TYPE_RIGID_BODY)
		{
			phCollider::ProfileDraw();
			return;
		}

		if (PFDGROUP_ArticulatedLinks.Begin())
		{
			bool oldLighting = grcLighting(false);

			for (int linkIndex = 0; linkIndex < m_Body->GetNumBodyParts(); ++linkIndex)
			{
				const phArticulatedBodyPart& link = m_Body->GetLink(linkIndex);
				Mat34V worldFromLink;
				Transpose3x3(worldFromLink,link.GetMatrix());
				worldFromLink.SetCol3(Add(VECTOR3_TO_VEC3V(link.GetPosition()),GetPosition()));
				grcWorldIdentity();
				grcDrawAxis(0.2f, worldFromLink);

				int lines = 0;

				// Add additional offset to the root link if the collider text is being drawn. Otherwise they overlap
				if(linkIndex == 0 && PFDGROUP_Colliders.GetEnabled())
				{
					if(PFD_ColliderMassText.GetEnabled())
					{
						++lines;
					}
					if(PFD_ColliderAngInertiaText.GetEnabled())
					{
						++lines;
					}
					if(PFD_ColliderExtraAllowedPenetrationText.GetEnabled())
					{
						++lines;
					}
				}

				if(PFD_ArticulatedLinkMassText.Begin())
				{
					char massBuffer[128];
					sprintf(massBuffer,"Mass: %.1f",m_Body->GetMass(linkIndex).Getf());
					grcDebugDraw::Text(worldFromLink.GetCol3(),Color_LightBlue,0,grcFont::GetCurrent().GetHeight()*lines,massBuffer,true);
					++lines;
					PFD_ArticulatedLinkMassText.End();
				}
				if(PFD_ArticulatedLinkAngInertiaText.Begin())
				{
					char angInertiaBuffer[128];
					sprintf(angInertiaBuffer,"Angular Inertia: %.1f, %.1f, %.1f", m_Body->GetAngInertia(linkIndex).GetXf(), m_Body->GetAngInertia(linkIndex).GetYf(), m_Body->GetAngInertia(linkIndex).GetZf());
					grcDebugDraw::Text(worldFromLink.GetCol3(),Color_LightBlue,0,grcFont::GetCurrent().GetHeight()*lines,angInertiaBuffer,true);
					++lines;
					PFD_ArticulatedLinkAngInertiaText.End();
				}
			}

			grcLighting(oldLighting);
			PFDGROUP_ArticulatedLinks.End();
		}

		if (PFDGROUP_ArticulatedJoints.WillDraw())
		{
			for (int jointIndex = 0; jointIndex < m_Body->GetNumJoints(); ++jointIndex)
			{
				phJoint& joint = m_Body->GetJoint(jointIndex);
				Vector3 jointPos = joint.GetJointPosition(m_Body);
				jointPos.Add(RCC_VECTOR3(GetPosition()));

				phArticulatedBodyPart& parentLink = m_Body->GetLink(m_Body->GetParentNum(jointIndex+1));
				Matrix34 parentOrientation;
				parentOrientation.Transpose(MAT34V_TO_MATRIX34(parentLink.GetMatrix()));
				parentOrientation.d.Zero();

				phArticulatedBodyPart& childLink = m_Body->GetLink(jointIndex+1);
				Matrix34 childOrientation;
				childOrientation.Transpose(MAT34V_TO_MATRIX34(childLink.GetMatrix()));
				childOrientation.d.Zero();

				float length = PFD_ArticulatedJointLength.GetValue();

				if (joint.GetJointType() == phJoint::JNT_1DOF)
				{
					phJoint1Dof* joint1dof = static_cast<phJoint1Dof*>(&joint);

					if (PFD_Articulated1DofJoints.Begin())
					{
						bool oldLighting = grcLighting(false);
						grcWorldIdentity();
						Vector3& axis = joint1dof->GetRotationAxis();

						Vector3 axisEnd1, axisEnd2;
						axisEnd1.AddScaled(jointPos, axis, -0.3f * length);
						axisEnd2.AddScaled(jointPos, axis,  0.3f * length);

						if (!PFD_ArticulatedJointAngles.WillDraw())
						{
							grcBegin(drawLines, 2);
							grcVertex3fv(&axisEnd1[0]);
							grcVertex3fv(&axisEnd2[0]);
							grcEnd();
						}
						else
						{
							Vector3 angleEnd;
							angleEnd.AddScaled(jointPos, childOrientation.c, length * 0.9f);

							float rawLimit1, rawLimit2;
							joint1dof->GetAngleLimits(rawLimit1, rawLimit2);
							// limits are not +/-PI so make them be for debug draw
							float limit1 = CanonicalizeAngle(rawLimit1);
							float limit2 = CanonicalizeAngle(rawLimit2);
							if (limit1 > limit2)
								limit2 += 2.0f*PI;

							float limit1X = cosf(limit1);
							float limit1Y = -sinf(limit1);

							Vector3 limit1End;
							limit1End.Scale(parentOrientation.c, limit1X);
							limit1End.AddScaled(parentOrientation.b, limit1Y);
							limit1End.Scale(length);
							limit1End.Add(jointPos);

							float limit2X = cosf(limit2);
							float limit2Y = -sinf(limit2);

							Vector3 limit2End;
							limit2End.Scale(parentOrientation.c, limit2X);
							limit2End.AddScaled(parentOrientation.b, limit2Y);
							limit2End.Scale(length);
							limit2End.Add(jointPos);

							grcBegin(drawLines, 8);
							grcColor(Color_red);
							grcVertex3fv(&jointPos[0]);
							grcVertex3fv(&angleEnd[0]);
							grcColor(Color_white);
							grcVertex3fv(&axisEnd1[0]);
							grcVertex3fv(&axisEnd2[0]);
							grcColor(Color_blue);
							grcVertex3fv(&jointPos[0]);
							grcVertex3fv(&limit1End[0]);
							grcVertex3fv(&jointPos[0]);
							grcVertex3fv(&limit2End[0]);
							grcColor(Color_cyan3);
							grcEnd();

							const int NUM_ARC_POINTS = 10;
							grcBegin(drawLineStrip, NUM_ARC_POINTS + 1);
							float limitArcX =  cosf(limit1);
							float limitArcY = -sinf(limit1);

							Vector3 arc1End;
							arc1End.Scale(parentOrientation.c, limitArcX);
							arc1End.AddScaled(parentOrientation.b, limitArcY);
							arc1End.Scale(length * 0.9f);
							arc1End.Add(jointPos);
							grcVertex3fv(&arc1End[0]);

							for (int i = 1; i <= NUM_ARC_POINTS; ++i)
							{
								float sweep = limit1 + ((float)i / (float)NUM_ARC_POINTS) * (limit2 - limit1);
								limitArcX =  cosf(sweep);
								limitArcY = -sinf(sweep);

								Vector3 arc2End;
								arc2End.Scale(parentOrientation.c, limitArcX);
								arc2End.AddScaled(parentOrientation.b, limitArcY);
								arc2End.Scale(length * 0.9f);
								arc2End.Add(jointPos);
								grcVertex3fv(&arc2End[0]);

								arc1End = arc2End;
							}
							grcEnd();
						}

						grcLighting(oldLighting);
						PFD_Articulated1DofJoints.End();
					}
				}
				else if (PFD_Articulated3DofJoints.Begin())
				{
					const phJoint3Dof& joint3Dof = static_cast<phJoint3Dof&>(joint);

					bool oldLighting = grcLighting(false);
					grcWorldIdentity();

					if (true || !PFD_ArticulatedJointAngles.WillDraw())
					{
						grcColor(Color_white);
						grcDrawSphere(0.1f, jointPos, 10, true);
					}
					else
					{
						Vector3 angleEnd;
						angleEnd.AddScaled(jointPos, childOrientation.c, length);

						float limitLean1, limitLean2, limitTwist;
						joint3Dof.GetThreeAngleLimits(limitLean1, limitLean2, limitTwist);

						grcColor(Color_white);
						grcBegin(drawLineStrip, 3);
						grcVertex3fv(&jointPos[0]);
						grcVertex3fv(&angleEnd[0]);

						Vector3 twistAngleEnd(angleEnd);
						twistAngleEnd.AddScaled(childOrientation.b, length * 0.3f * 0.8f);
						grcVertex3fv(&twistAngleEnd[0]);
						grcEnd();

						float twistLimitX = -sinf(limitTwist);
						float twistLimitY =  cosf(limitTwist);

						Vector3 twistLimit1End;
						twistLimit1End.Scale(parentOrientation.a, twistLimitX);
						twistLimit1End.AddScaled(parentOrientation.b, twistLimitY);
						twistLimit1End.Scale(length * 0.3f);
						twistLimit1End.Add(angleEnd);

						Vector3 twistLimit2End;
						twistLimit2End.Scale(parentOrientation.a, -twistLimitX);
						twistLimit2End.AddScaled(parentOrientation.b, twistLimitY);
						twistLimit2End.Scale(length * 0.3f);
						twistLimit2End.Add(angleEnd);

						grcColor(Color_red);
						grcBegin(drawLineStrip, 3);

						grcVertex3fv(&twistLimit1End[0]);
						grcVertex3fv(&angleEnd[0]);
						grcVertex3fv(&twistLimit2End[0]);

						grcEnd();

						const int NUM_RADIAL_LINES = 16;
						grcBegin(drawTriFan, 2 + NUM_RADIAL_LINES);
						grcColor(Color32(0.71f,0.80f,0.80f,0.3f));
						grcVertex3fv(&jointPos[0]);
						for (int i = 0; i <= NUM_RADIAL_LINES; ++i)
						{
							float sweep = ((float)i / (float)NUM_RADIAL_LINES) * 2.0f * PI;

							if (i >= NUM_RADIAL_LINES)
							{
								sweep = 0.0f;
							}

							float limit1Y = sinf(limitLean1) * sinf(sweep);
							float limit2X = sinf(limitLean2) * cosf(sweep);

							Vector3 limitEnd;
							limitEnd.Scale(parentOrientation.c, cosf(limitLean1) + (cosf(limitLean2) - cosf(limitLean1)) * (cosf(sweep * 2.0f) + 1.0f) * 0.5f);
							limitEnd.AddScaled(parentOrientation.b, limit1Y);
							limitEnd.AddScaled(parentOrientation.a, limit2X);
							limitEnd.Scale(length * 0.7f);
							limitEnd.Add(jointPos);

							grcVertex3fv(&limitEnd[0]);
						}
						grcEnd();

						grcBegin(drawLines, NUM_RADIAL_LINES * 2);
						grcColor(Color_LightCyan3);
						for (int i = 0; i < NUM_RADIAL_LINES; ++i)
						{
							float sweep = ((float)i / (float)NUM_RADIAL_LINES) * 2.0f * PI;

							float limit1Y = sinf(limitLean1) * sinf(sweep);
							float limit2X = sinf(limitLean2) * cosf(sweep);

							Vector3 limitEnd;
							limitEnd.Scale(parentOrientation.c, cosf(limitLean1) + (cosf(limitLean2) - cosf(limitLean1)) * (cosf(sweep * 2.0f) + 1.0f) * 0.5f);
							limitEnd.AddScaled(parentOrientation.b, limit1Y);
							limitEnd.AddScaled(parentOrientation.a, limit2X);
							limitEnd.Scale(length * 0.7f);
							limitEnd.Add(jointPos);

							grcVertex3fv(&jointPos[0]);
							grcVertex3fv(&limitEnd[0]);
						}
						grcEnd();
					}
					grcLighting(oldLighting);
					PFD_Articulated3DofJoints.End();
				}
			}

		}

		if (PFDGROUP_Bounds.Begin(false))
		{
			// See if previous frame bound drawing is enabled.
			if (PFD_PreviousFrame.GetEnabled())
			{
				// Draw the articulated body bound parts on the previous frame.
				grcColor(PFD_PreviousFrame.GetBaseColor());
				const phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound());
				compositeBound.DrawLast(m_LastInstanceMatrix, PFD_DrawBoundMaterials.WillDraw(), PFD_Solid.GetEnabled());
			}

			// End the bound drawing.
			PFDGROUP_Bounds.End();
		}

		if (PFD_CenterOfGravity.Begin())
		{
			// Compute and draw axes at the articulated body's center of mass.
			Vector3 articulatedCGOffset(ORIGIN);
			Vector3 mass,totalMass;
			totalMass.Zero();
			for (int bodyPartIndex=0; bodyPartIndex< m_Body->GetNumBodyParts(); bodyPartIndex++)
			{
				const phArticulatedBodyPart& bodyPart = m_Body->GetLink(bodyPartIndex);
				mass = SCALARV_TO_VECTOR3(m_Body->GetMass(bodyPartIndex));
				articulatedCGOffset.AddScaled(bodyPart.GetPosition(),mass);
				totalMass += mass;
			}
			articulatedCGOffset.InvScale(totalMass);
			Matrix34 worldCG(RCC_MATRIX34(m_Matrix));
			worldCG.d.Add(articulatedCGOffset);
			bool oldLighting = grcLighting(false);
			grcColor(Color_magenta);
			float size = 0.2f;
			const Vector3 vertices[] =
			{
				Vector3( size,  0,     0),
				Vector3(-size,  0,     0),
				Vector3( 0,     size,  0),
				Vector3( 0,    -size,  0),
				Vector3( 0,     0,     size),
				Vector3( 0,     0,    -size)
			};
			grcWorldMtx(worldCG);
			grcBegin(drawLines, 6);
			for (int vertex = 0; vertex < 6; ++vertex)
			{
				grcVertex3fv(&vertices[vertex][0]);
			}
			grcEnd();
			grcLighting(oldLighting);

			// End the center of gravity drawing.
			PFD_CenterOfGravity.End();
		}

		phCollider::ProfileDraw();
	}
#endif // __PFDRAW


#if __PFDRAW
	void phArticulatedCollider::DrawJointLimitAngImpulse(int limitDofIndex, float angImpulse) const
	{
		if (PFD_ArticulatedJointLimitAngImpulse.Begin())
		{
			if (angImpulse > SMALL_FLOAT || angImpulse < -SMALL_FLOAT)
			{
				angImpulse = log10(fabs(angImpulse) + 1.0f) * 0.25f;

				Vector3 axis;
				Vector3 position;
				m_Body->GetJointLimitAxis( m_LimitsJointIndex[limitDofIndex],m_LimitsDofIndex[limitDofIndex], RC_VEC3V(axis), RC_VEC3V(position) );
				position += RCC_VECTOR3(GetPosition());
				axis.Scale(-angImpulse);
				axis.Add(position);

				bool oldLighting = grcLighting(false);
				grcWorldIdentity();
				grcDrawSpiral(position, axis, 0.0f, angImpulse * 0.125f, 10.0f, 0.75f, angImpulse * 0.05f);
				grcLighting(oldLighting);
			}

			PFD_ArticulatedJointLimitAngImpulse.End();
		}
	}
#endif

#if __PPU
	void phArticulatedCollider::RegenerateCoreDmaPlan()
	{
		sysDmaPlan& dmaPlan = *m_DmaPlan;
		dmaPlan.Initialize();

		DMA_BEGIN_READ_ONLY();

		ADD_DMA_OWNER(m_Body);
		m_Body->GenerateDmaPlan(dmaPlan);

		ADD_DMA_ARRAY_OWNER_READ_ONLY(m_LimitsJointIndex.GetElements(), m_LimitsJointIndex.GetCapacity());
		ADD_DMA_ARRAY_OWNER_READ_ONLY(m_LimitsDofIndex.GetElements(), m_LimitsDofIndex.GetCapacity());
		ADD_DMA_ARRAY_OWNER_READ_ONLY(m_LimitsExcessHard.GetElements(), m_LimitsExcessHard.GetCapacity());
		ADD_DMA_ARRAY_OWNER_READ_ONLY(m_LimitResponse.GetElements(), m_LimitResponse.GetCapacity());
		ADD_DMA_ARRAY_OWNER_READ_ONLY(m_AccumJointImpulse.GetElements(), m_AccumJointImpulse.GetCapacity());
		//ADD_DMA_ARRAY_OWNER(m_BallisticRestore.GetElements(), m_BallisticRestore.GetCapacity()); // Skipped for core-only because there is no real link core type data
		ADD_DMA_ARRAY_OWNER_READ_ONLY(m_ComponentToLinkIndex.GetElements(), m_ComponentToLinkIndex.GetCapacity());
	}

	void phArticulatedCollider::GenerateCoreDmaPlan(sysDmaPlan& dmaPlan)
	{
		m_DmaPlan = &dmaPlan;

		RegenerateCoreDmaPlan();
	}
#endif


	void phArticulatedCollider::ClearActive()
	{
		sm_NumActiveThisFrame = 0;
	}

#if __BANK
	void phArticulatedCollider::GetNumberOfMedAndLodLODBodies(int &numberMedLODBodies, int &numberLowLODBodies)
	{
		int medLODBodies = 0;
		int lowLODBodies = 0;

		int articulatedCount = sm_NumActiveThisFrame;
		for (int index = 0; index < articulatedCount; ++index)
		{
			if (sm_ActiveThisFrame[index]->m_ColliderType != TYPE_RIGID_BODY)
			{
				if (sm_ActiveThisFrame[index]->GetLOD() == 1)
					medLODBodies++;
				else if (sm_ActiveThisFrame[index]->GetLOD() == 2)
					lowLODBodies++;
			}
		}

		numberMedLODBodies = medLODBodies;
		numberLowLODBodies = lowLODBodies;
	}
#endif

	void phArticulatedCollider::InitAllAccumJointImpulse()
	{
		int articulatedCount = sm_NumActiveThisFrame;
		for (int index = 0; index < articulatedCount; ++index)
		{
			if (sm_ActiveThisFrame[index]->m_ColliderType != TYPE_RIGID_BODY)
			{
				sm_ActiveThisFrame[index]->InitAccumJointImpulse();
			}
		}
	}

	void phArticulatedCollider::SaveAndZeroAllVelocitiesForPushes()
	{
		int articulatedCount = sm_NumActiveThisFrame;
		phArticulatedCollider *artColl = NULL;
		for (int index = 0; index < articulatedCount; ++index)
		{
			if ((sm_ActiveThisFrame[index]->m_ColliderType != TYPE_RIGID_BODY) TRACK_PUSH_COLLIDERS_ONLY(&& sm_ActiveThisFrame[index]->GetIsInPushPair()))
			{
				artColl = sm_ActiveThisFrame[index];
				artColl->GetBody()->SaveAndZeroVelocities(artColl->m_SavedLinearVelocities.GetElements(), artColl->m_SavedAngularVelocities.GetElements());
			}
		}
	}

	void phArticulatedCollider::EnsureAllArtVelocitiesFullyPropagated()
	{
		int articulatedCount = sm_NumActiveThisFrame;
		phArticulatedCollider *artColl = NULL;
		for (int index = 0; index < articulatedCount; ++index)
		{
			if (sm_ActiveThisFrame[index]->m_ColliderType != TYPE_RIGID_BODY)
			{
				artColl = sm_ActiveThisFrame[index];
				artColl->GetBody()->EnsureVelocitiesFullyPropagated();
			}
		}
	}

	void phArticulatedCollider::RestoreVelocities()
	{
		for (int linkIndex=0; linkIndex<m_Body->GetNumBodyParts(); linkIndex++)
		{
			m_Body->RestoreVelocities(m_SavedLinearVelocities.GetElements(), m_SavedAngularVelocities.GetElements());
		}
	}

	void phArticulatedCollider::ResetSavedVelocities()
	{
		sysMemSet(&m_SavedLinearVelocities[0], 0, m_SavedLinearVelocities.GetCount() * sizeof(Vec3V)); 
		sysMemSet(&m_SavedAngularVelocities[0], 0, m_SavedAngularVelocities.GetCount() * sizeof(Vec3V));
	}

	void phArticulatedCollider::SetActiveArrayInfo(phArticulatedCollider** activeArray, u32* activeCount)
	{
		sm_ActiveThisFrame = activeArray;
		sm_NumActiveThisFramePtr = activeCount;
	}


	phArticulatedCollider** phArticulatedCollider::GetActiveArray()
	{
		return sm_ActiveThisFrame;
	}


	u32* phArticulatedCollider::GetActiveCountPtr()
	{
		return sm_NumActiveThisFramePtr;
	}


	void phArticulatedCollider::MakeParentJointEnforceLimits (int component)
	{
		int bodyPartIndex = GetLinkFromComponent(component);

		// See if this contact is on an articulated body part other than the root.
		if (bodyPartIndex > 0)
		{
			int jointIndex = bodyPartIndex-1;
			m_Body->GetJoint(jointIndex).SetEnforceAccededLimitsFlag();
		}
	}


	void phArticulatedCollider::ApplyGravityImp (Vec::V3Param128 gravity, Vec::V3Param128 timestep)
	{
		if (!m_Instance->GetInstFlag(phInst::FLAG_NO_GRAVITY))
		{
			// Want to keep this - see comment in articulatedBody.h
			m_Body->ApplyGravityToLinks(gravity, timestep, m_GravityFactor, GetInstance()->GetInstFlag(phInst::FLAG_NO_GRAVITY_ON_ROOT_LINK) ? true : false);
		}
	}


	void phArticulatedCollider::SetVelocityImp (Vec::V3Param128 velocity)
	{
		// Set the velocity and momentum in the base class.
		phCollider::SetVelocityImp(velocity);

		// Set the new velocity in all the body parts.
		Assert(m_Body);
		m_Body->SetVelocity(m_Velocity);
	}

	void phArticulatedCollider::SetAngVelocityImp (Vec::V3Param128 angVelocity)
	{
		// Set the angular velocity in the base class.
		phCollider::SetAngVelocityImp(angVelocity);

		// Set the new angular velocity, and the linear velocity changes resulting from offsets, in all the body parts.
		Assert(m_Body);
		m_Body->SetAngVelocity(m_AngVelocity);
	}

	void phArticulatedCollider::ApplyImpulseArt (Vec::V3Param128 impulse, Vec::V3Param128 position, int component, float UNUSED_PARAM(breakScale))
	{
		Vec3V localImpulse = Vec3V(impulse);
		Vec3V relPosition(position);
		relPosition = Subtract(relPosition, GetPosition());
		m_Body->ApplyImpulse(GetLinkFromComponent(component), localImpulse, relPosition);

#if __PFDRAW
		DrawImpulse(Vec3V(impulse),Vec3V(position));
#endif

	}

	void phArticulatedCollider::ApplyForceImp (Vec::V3Param128 force, Vec::V3Param128 position, int component, Vec::V3Param128 timestep)
	{
		// Inform the instance that it received a force.
		Assert(m_Instance);
		int bodyPartIndex = GetLinkFromComponent(component);
		m_Instance->NotifyForce(force,position,component);

		Vector3 relPosition(position);
		relPosition.Subtract(RCC_VECTOR3(GetPosition()));
		m_Body->ApplyForce(bodyPartIndex, force,relPosition, timestep);

		DrawForce(Vec3V(force),Vec3V(position));
	}

	void phArticulatedCollider::ApplyForceCenterOfMassImp (Vec::V3Param128 force, Vec::V3Param128 timestep)
	{
		// Inform the instance that it received a force.
		Assert(m_Instance);
		m_Instance->NotifyForce(force, ORIGIN, 0);

		m_Body->ApplyForce(0, force, ORIGIN, timestep);
		DrawForce(Vec3V(force), m_Matrix.GetCol3());
	}

	void phArticulatedCollider::ApplyImpulseCenterOfMassArt (Vec::V3Param128 impulse)
	{
		Vec3V v_zero(V_ZERO);

		// Inform the instance that it received an impulse.
		Assert(m_Instance);
		m_Instance->NotifyImpulse(impulse,RCC_VECTOR3(v_zero),0);

		m_Body->ApplyImpulse( 0, Vec3V(impulse), v_zero);
		DrawForce(Vec3V(impulse), m_Matrix.GetCol3());
	}

	void phArticulatedCollider::ApplyTorqueImp (Vec::V3Param128 torque, Vec::V3Param128 timestep)
	{
		m_Body->ApplyTorque( 0, torque, timestep);
		DrawTorque(Vec3V(torque), m_Matrix.GetCol3());
	}

	void phArticulatedCollider::ApplyAngAccelArt (Vec::V3Param128 UNUSED_PARAM(angAccel))
	{
		AssertMsg(0, "Not implemented!");
	}


	float phArticulatedCollider::GetTotalInternalMotionArt() const
	{
		ScalarV motion = ScalarV(V_ZERO);

		for (int linkIndex = 0; linkIndex < m_Body->GetNumBodyParts(); ++linkIndex)
		{
			motion += MagSquared( m_Body->GetAngularVelocityNoProp(linkIndex) );
		}

		// Necessary LHS due to 'float' return value.
		float numerator = motion.Getf();
		float divisor = (float)(m_Body->GetNumBodyParts());
		return (numerator / divisor);
	}

	void phArticulatedCollider::GetInverseMassMatrix(Mat33V_InOut inverseMassMatrix, Vec::V3Param128 position, int component) const
	{
		Vec3V localPos(position);
		localPos -= GetPosition();
		int bodyPartIndex = GetLinkFromComponent(component);
		m_Body->GetInverseMassMatrix( bodyPartIndex, localPos.GetIntrin128(), RC_MATRIX33(inverseMassMatrix) );
	}

	void phArticulatedCollider::GetInverseMassMatrixSelf(Mat33V_InOut inverseMassMatrix, Vec::V3Param128 positionA, Vec::V3Param128 positionB, int componentA, int componentB ) const
	{
		Vec3V pos = GetPosition();

		Vec3V localPosA(positionA);
		localPosA -= pos;
		Vec3V localPosB(positionB);
		localPosB -= pos;
		int bodyPartIndexA = GetLinkFromComponent(componentA);
		int bodyPartIndexB = GetLinkFromComponent(componentB);
		m_Body->GetInverseMassMatrix(bodyPartIndexA,localPosA.GetIntrin128(),bodyPartIndexB,localPosB.GetIntrin128(),RC_MATRIX33(inverseMassMatrix) );
	}

	void phArticulatedCollider::GetInertiaMatrixImp (Mat33V_InOut outInertia, int component) const
	{
		int bodyPartIndex = GetLinkFromComponent(component);
		m_Body->GetInertiaMatrix(RC_MATRIX33(outInertia),bodyPartIndex);
	}


	void phArticulatedCollider::GetInverseInertiaMatrixArt (Mat33V_InOut outInvInertia, int component) const
	{
		int bodyPartIndex = GetLinkFromComponent(component);
		m_Body->GetInverseInertiaMatrix(RC_MATRIX33(outInvInertia),bodyPartIndex);
	}


	void phArticulatedCollider::GetInvMassMatrixArt (Mat33V_InOut outMtx, Vec::V3Param128 sourcePos, const Vec3V* responsePos, int sourceComponent, int responseComponent) const
	{
		Vec3V relResponsePos;
		if (responsePos)
		{
			relResponsePos = *(reinterpret_cast<const Vec3V*>(responsePos));
		}
		else
		{
			relResponsePos = RCC_VEC3V(sourcePos);
			responseComponent = sourceComponent;
		}
		relResponsePos = Subtract(relResponsePos, GetPosition());
		m_Body->GetInverseMassMatrix(GetLinkFromComponent(responseComponent),RCC_VECTOR3(relResponsePos),RC_MATRIX33(outMtx));
	}


	Vec3V_Out phArticulatedCollider::GetLocalVelocityArt (Vec::V3Param128 position, int component) const
	{
		Vec3V relPosition(position);
		relPosition = Subtract(relPosition, GetPosition());
		int link = GetLinkFromComponent(component);	
		return m_Body->GetLocalVelocity(link, RCC_VECTOR3(relPosition));
	}


	Vec3V_Out phArticulatedCollider::GetAngVelocityOfBodyPart (int component) const
	{
		int partIndex = GetLinkFromComponent(component);
		return m_Body->GetAngularVelocity(partIndex);
	}


	Vec3V_Out phArticulatedCollider::GetWorldPositionOfBoundPart (int component) const
	{
		const Mat34V instancePose(m_Instance->GetMatrix());
		const phBoundComposite& compositeBound = *static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound());
		Vec3V localPosition = compositeBound.GetCurrentMatrix(component).GetCol3();
		Vec3V worldPosition = Transform(instancePose,localPosition);
		return worldPosition;
	}

	bool phArticulatedCollider::RejuvenateImp ()
	{
		bool rejuvenatedMatrices = phCollider::RejuvenateImp();
		if (rejuvenatedMatrices)
		{
			m_Body->RejuvenateMatrices();
			UpdateCurrentMatrices();
		}

		return rejuvenatedMatrices;
	}

	void phArticulatedCollider::SetBvhRebuildPeriod(u16 bvhRebuildPeriod)
	{
		m_BvhRebuildPeriod = bvhRebuildPeriod;
	}

	void phArticulatedCollider::RefreshBvh(phBoundComposite& compositeBound)
	{
		if(m_BvhRebuildPeriod != UPDATE_BVH_EACH_UPDATE && ++m_BvhRebuildUpdateCount > m_BvhRebuildPeriod)
		{
			compositeBound.UpdateBvh(true);
			m_BvhRebuildUpdateCount = 0;
		}
		else
		{
			compositeBound.UpdateBvh(false);
		}
	}
	 
	void phArticulatedCollider::DampMotionImp (Vec::V3Param128 timeStep)
	{
		// Get the physics archetype and see if it is damped (a phArchetypeDamp).
		Assert(m_Instance && m_Instance->GetArchetype());
		const phArchetype* archetype = m_Instance->GetArchetype();
		if (archetype->IsDamped() && IsDampingEnabled())
		{
			// This is a damped archetype, so apply damping to the object's motion.
			const phArchetypeDamp* dampedArch = static_cast<const phArchetypeDamp*>(archetype);

			Vec3V invTimeStep = InvertSafe(RCC_VEC3V(timeStep), Vec3V(V_FLT_MAX));
			Vec3V dampLinearC = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::LINEAR_C));
			Vec3V dampLinearV = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::LINEAR_V));
			Vec3V dampLinearV2 = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::LINEAR_V2));
			Vec3V dampAngularC = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::ANGULAR_C));
			Vec3V dampAngularV = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::ANGULAR_V));
			Vec3V dampAngularV2 = RCC_VEC3V(dampedArch->GetDampingConstant(phArchetypeDamp::ANGULAR_V2));

			if (m_ReducedLinearDampingFrames >= 1)
			{
				dampLinearC = dampLinearV = dampLinearV2 = Vec3V(V_ZERO);
				m_ReducedLinearDampingFrames--;
			}

			for (int linkIndex = m_Body->GetNumJoints(); linkIndex >= 0; --linkIndex)
			{
				phArticulatedBodyPart&     curBodyPart     = m_Body->GetLink(linkIndex);
				Mat34V matBodyPart = curBodyPart.GetMatrix();
				Transpose3x3( matBodyPart, matBodyPart );

				// Get the reference frame velocity.
				Vec3V localVel = UnTransform3x3Ortho(matBodyPart, m_Body->GetLinearVelocity(linkIndex) - GetReferenceFrameVelocity());

				// Compute the damping force.
				Vec3V force;
				RC_VECTOR3(force) = ComputeDampingAccel(VEC3V_TO_INTRIN(localVel),timeStep,VEC3V_TO_INTRIN(invTimeStep),VEC3V_TO_INTRIN(dampLinearC),VEC3V_TO_INTRIN(dampLinearV),VEC3V_TO_INTRIN(dampLinearV2));
				force = Scale(force, m_Body->GetMass(linkIndex));

				// Put the force in world coordinates and apply it to the body part.
				Vec3V worldForce = Transform3x3( matBodyPart, force );
				m_Body->ApplyForce(linkIndex, RCC_VECTOR3(worldForce), curBodyPart.GetPosition(), timeStep);

				// Get the reference frame angular velocity.
				localVel = UnTransform3x3Ortho( matBodyPart, m_Body->GetAngularVelocity(linkIndex) - GetReferenceFrameAngularVelocity());

				// Compute the damping torque.
				RC_VECTOR3(force) = ComputeDampingAccel(VEC3V_TO_INTRIN(localVel),timeStep,VEC3V_TO_INTRIN(invTimeStep),VEC3V_TO_INTRIN(dampAngularC),VEC3V_TO_INTRIN(dampAngularV),VEC3V_TO_INTRIN(dampAngularV2));
				force = Scale(force, m_Body->GetAngInertia(linkIndex));

				// Put the torque in world coordinates and apply it to the collider.
				worldForce = Transform3x3( matBodyPart, force );
				m_Body->ApplyTorque(linkIndex, worldForce.GetIntrin128(), timeStep);
			}
		}

		// Damp based on the game-controlled damping rate
		DampReferenceFrameVelocities(timeStep);
	}


} // namespace rage
