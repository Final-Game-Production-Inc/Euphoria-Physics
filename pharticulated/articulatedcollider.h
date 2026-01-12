//
// pharticulated/articulatedcollider.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHARTICULATED_ARTICULATEDCOLLIDER_H
#define PHARTICULATED_ARTICULATEDCOLLIDER_H

#include "articulatedbody.h"

#include "physics/collider.h"

#include "atl/bitset.h"
#include "system/memops.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"
#include "paging/array.h"

#if !__SPU
#include "physics/simulator.h"
#endif // __SPU

#if CAPTURE_STACK_INVALIDATING_JOINT_LIMITS
#include "system/stack.h"
#endif // CAPTURE_STACK_INVALIDATING_JOINT_LIMITS

#define REBUILD_BVH_EACH_UPDATE (0)
#define UPDATE_BVH_EACH_UPDATE ((u16)-1)

namespace rage {


class phArticulatedCollider : public phCollider
{
public:
	static void InitClass();
	static void ShutdownClass();
	phArticulatedCollider ();
	phArticulatedCollider (int maxNumJointDofs, int maxNumBoundParts);
	void PreInit (int maxNumJointDofs, int maxNumBoundParts);
	DECLARE_PLACE(phArticulatedCollider);
	phArticulatedCollider(datResource& rsc);

	// <COMBINE phCollider::Init>
	PH_NON_SPU_VIRTUAL void Init (phInst* instance, phSleep* sleep=NULL);

	PH_NON_SPU_VIRTUAL void SetType (int type);

	// PURPOSE: Set the articulated body for this articulated collider.
	// PARAMS:
	//	body - the new articulated body for this articulated collider
	void SetBody (phArticulatedBody* body);

	// PURPOSE: Get the articulated body used by this articulated collider.
	// RETURN	the articulated body used by this articulated collider
	phArticulatedBody* GetBody ();
	const phArticulatedBody* GetBody () const;


	// PURPOSE: Set the instance for this articulated collider.
	// PARAMS:
	//	instance - the new instance for this articulated collider
	// NOTES:	This could be in phCollider, but it would likely cause trouble because it does not copy the instance matrix or reset the collider (SetInstanceAndReset does).
	//			This is used for articulated colliders to set the instance pointer without resetting the articulated body.
	void SetInstance (phInst* instance);

	void RemoveFromActiveList();
	
	inline bool CanBeArticulated() const { return true; }

	// PURPOSE:	Set this collider's type to articulated with a dominant root part.
	// PARAMS:
	//	largeRoot - optional boolean to turn on or off the dominant root feature of this articulated collider
	// NOTES:	Articulated with a large root means that the majority of the mass is in the root part (such as a car with doors).
	void SetArticulatedLargeRoot (bool largeRoot=true);

	static void SetPartsCanCollide (rage::atArray<u8,16> &partCanCollideA, rage::atArray<u8,16> &partCanCollideB, 
		int partIndexA, int partIndexB, bool addPair = true, bool finalize = true);
	void SetPartsCanCollide (int partIndexA, int partIndexB, bool addPair = true, bool finalize = true);

    static void FinalizeSettingPartsCanCollide (rage::atArray<u8,16> &partCanCollideA, rage::atArray<u8,16> &partCanCollideB);

	// This should be called whenever the number of body parts grows
	void UpdateSavedVelocityArraySizes(bool zeroAllVels = false);
	void RestoreVelocities();
	void ResetSavedVelocities();

	__forceinline int GetLinkFromComponent(int component) const
	{
#if !__SPU && 0
		AssertMsg(m_Instance && m_Instance->GetArchetype(),"An articulated collider doesn't have an instance with an archetype.");
		phBoundComposite* bound = static_cast<phBoundComposite*>(m_Instance->GetArchetype()->GetBound());
		AssertMsg(bound && bound->GetType()==phBound::COMPOSITE,"An articulated collider doesn't have a composite bound.");
		if (component<0 || component>=bound->GetNumBounds())
		{
			return 0;
		}
#endif

		int bodyPartIndex = m_ComponentToLinkIndex[component];
#if __SPU
		// An assert in this function takes up too much space on SPU
		ASSERT_ONLY(TrapGE(bodyPartIndex,m_Body->GetNumBodyParts());)
		ASSERT_ONLY(TrapLT(bodyPartIndex,0);)
#else // __SPU
		Assertf(bodyPartIndex >= 0 && bodyPartIndex < m_Body->GetNumBodyParts(), "phArticulatedCollider::GetLinkFromComponent: Component %i maps to non-existant link %i.", component, bodyPartIndex);
#endif // __SPU
		return bodyPartIndex;
	}

	// PURPOSE: Set the body part (link) index for the given composite bound component.
	// PARAMS:
	//	component - the index of the composite bound part
	//	bodyPartIndex - the index of the body part that will go with the composite bound part
	void SetLinkForComponent(int component, int bodyPartIndex);

	// PURPOSE: Set the body part (link) index for every bound part to zero.
	// PARAMS:
	//	numComponents - the number of bound components for which to zero the body part index.
	void ZeroLinksForComponents (int numComponents);

	int GetNumComponents () { return m_ComponentToLinkIndex.GetCount(); }

	void RemoveLink(int link);
	void SetLinkAttachment(const pgArray<Matrix34> * linkAttachment);

	const Matrix34* GetLinkAttachmentMatrices() const { return reinterpret_cast<const Matrix34*>(m_LinkAttachment->GetElements()); }

	// NOTE: Be extremely careful when you call this function that this collider is pointing at locally owned link attachment matrices
	void SetLinkAttachmentMatrix(int componentIndex, Mat34V_In linkAttachment) { m_LinkAttachment->GetElements()[componentIndex] = linkAttachment; }

	// <COMBINE phCollider::Reset>
	PH_NON_SPU_VIRTUAL void Reset ();

	// <COMBINE phCollider::Freeze>
	PH_NON_SPU_VIRTUAL void Freeze ();

	// <COMBINE phCollider::ZeroForces>
	void ZeroForces ();

	bool IsRagdollUnstable();

	void SubdueUnstableRagdoll();

	void SetLargeTurnsTriggerPushCollisions(bool set) { m_LargeTurnsTriggerPushCollisions = set; }

	// Teleports the collider to matOrientation, maintaining all relative offsets to the inst matrix and links
	void TeleportCollider(const Mat34V& matOrientation);

	// PURPOSE: Set the velocities of all the articulated body parts to zero.
	void ZeroLinkVelocities();

	// <COMBINE phCollider::Update>
	void UpdateImp (Vec::V3Param128 timeStep, Vec::V3Param128 gravity);

	// PURPOSE: Disable self collision for a few frames
	// PARAMS:
	//  numFrames - The number of frames to disable the self collision
	void TemporarilyDisableSelfCollision(int numFrames = 1);

	// PURPOSE: Reduce the self collision temporary disable timer by one
	void DecrementSelfCollisionTemporaryFrames();

#if !USE_NEW_SELF_COLLISION
	// PURPOSE: Detect collisions between the collidable pairs of articulated body parts.
	// PARAMS:
	//	manifold - pointer to the collision manifold to fill in
    void SelfCollision (phManifold* manifold);
#endif // !USE_NEW_SELF_COLLISION

	// <COMBINE phCollider::UpdatePositionFromVelocity>
	void UpdatePositionFromVelocityImp (Vec::V3Param128 timeStep);

	// <COMBINE phCollider::SetColliderMatrixFromInstance>
	void SetColliderMatrixFromInstanceArt ();

	// PURPOSE: Update the collider's composite bound matrices.
	void UpdateCurrentAndLastMatrices ();

	// PURPOSE: Update the collider's composite bound matrices for the current frame.
	void UpdateCurrentMatrices ();

	// PURPOSE: Update the collider's composite bound matrices for the previous frame.
	void UpdateLastMatrices ();

	// <COMBINE phCollider::UpdateVelocityFromImpulse>
	void UpdateVelocityFromImpulseImp (Vec::V3Param128 timeStep);

	// <COMBINE phCollider::UpdateVelocity>
	void UpdateVelocityImp (Vec::V3Param128 timeStep, bool saveVelocities);
	void ApplyInternalForces (Vec::V3Param128 timeStep);

	void MoveImp (Vec::V3Param128 timeStep, bool usePushes);

	// <COMBINE phCollider::ApplyImpulse>
	void ApplyImpulseArt (Vec::V3Param128 impulse, Vec::V3Param128 position, int component=0, float breakScale=1.0f);

	// <COMBINE phCollider::ApplyAngImpulse>
	void ApplyAngImpulseArt (Vec::V3Param128 impulse, int component=0);

	// <COMBINE phCollider::ApplyJointAngImpulse>
	void ApplyJointAngImpulseArt (Vec::V3Param128 angImpulse, int jointIndex=0);

	// <COMBINE phCollider::ApplyForce>
	void ApplyForceImp (Vec::V3Param128 force, Vec::V3Param128 position, int component, Vec::V3Param128 timestep);

	// <COMBINE phCollider::ApplyAngAccel>
	void ApplyAngAccelArt (Vec::V3Param128 angAccel);

	// <COMBINE phCollider::ApplyForceCenterOfMass>
	void ApplyForceCenterOfMassImp (Vec::V3Param128 force, Vec::V3Param128 timestep);

	// <COMBINE phCollider::ApplyImpulseCenterOfMass>
	void ApplyImpulseCenterOfMassArt (Vec::V3Param128 impulse);

	// <COMBINE phCollider::ApplyTorque>
	void ApplyTorqueImp (Vec::V3Param128 torqu, Vec::V3Param128 timestepe);
	
	// <COMBINE phCollider::GetTotalInternalMotion>
	float GetTotalInternalMotionArt() const;

	// <COMBINE phCollider::GetMass>
	float GetMassArt (int component) const;

	// <COMBINE phCollider::GetMass>
	ScalarV_Out GetMassVArt (int component) const;

	// <COMBINE phCollider::GetInertiaMatrix>
	void GetInertiaMatrixImp (Mat33V_InOut outInertia, int component=0) const;

	// <COMBINE phCollider::GetInverseInertiaMatrix>
	void GetInverseInertiaMatrixArt (Mat33V_InOut outInvInertia, int component=0) const;

	// <COMBINE phCollider::GetInvMassMatrix>
	void GetInvMassMatrixArt (Mat33V_InOut outMtx, Vec::V3Param128 sourcePos, const Vec3V* responsePos=NULL, int sourceComponent=0, int responseComponent=0) const;

	// <COMBINE phCollider::GetInvTorqueMassMatrix>
	void GetInvTorqueMassMatrix (Mat33V_InOut, Vec::V3Param128) const { AssertMsg(0 , "this should never be called on articulated colliders"); }

	// <COMBINE phCollider::GetInverseMassMatrix>
	void GetInverseMassMatrix (Mat33V_InOut inverseMassMatrix, Vec::V3Param128 position, int component) const;

	// <COMBINE phCollider::GetInverseMassMatrix>
	void GetInverseMassMatrixSelf (Mat33V_InOut inverseMassMatrix, Vec::V3Param128 positionA, Vec::V3Param128 positionB, int componentA, int componentB ) const;

	// PURPOSE: Get the effective inverse angular inertia for the given joint limit direction.
	// PARAMS:
	//	limitDofIndex - index number for the joint limit degree in the articulated collider's list
	// RETURN:	the response (scalar angular acceleration) to a unit torque in the given direction
	float GetJointLimitResponse (int limitDofIndex) const;

	Vec3V_Out GetLocalVelocityArt (Vec::V3Param128 position, int component=0) const;

	// PURPOSE:	Get the angular velocity of an articulated body part.
	// PARAMS:
	//	component - the composite bound part for which to get the angular velocity
	// RETURN:	The angular velocity of any of the body parts corresponding to the given composite bound part.
	Vec3V_Out GetAngVelocityOfBodyPart (int component) const;

	// PURPOSE:	Get the world position of a composite bound part.
	// PARAMS:
	//	component - the composite bound part for which to get the position
	// RETURN:	the world position of the given composite bound part
	Vec3V_Out GetWorldPositionOfBoundPart (int component) const;

	void ApplyGravityImp (Vec::V3Param128 gravity, Vec::V3Param128 timestep);
	void DampMotionImp (Vec::V3Param128 timeStep);

	// <COMBINE phCollider::SetVelocity>
	void SetVelocityImp (Vec::V3Param128 velocity);

	// <COMBINE phCollider::SetAngVelocity>
	void SetAngVelocityImp (Vec::V3Param128 angVelocity);

	inline int FindJointLimitDofs(float allowedAnglePenetration) 
	{
		m_NumLimitDofs = m_Body->FindJointLimitDofs(&m_LimitsJointIndex[0],&m_LimitsDofIndex[0],&m_LimitsExcessHard[0],&m_LimitResponse[0],allowedAnglePenetration + GetExtraAllowedPenetration(),m_NumDeepLimitDofs);
		return m_NumLimitDofs;
	}

	inline void SetReducedLinearDampingFrames(s16 frames = 2) { m_ReducedLinearDampingFrames = frames; }

#if !__SPU
	inline int FindJointLimitDofs()
	{
		return FindJointLimitDofs(phSimulator::GetAllowedAnglePenetration());
	}
#endif // !__SPU

	inline int GetARTAssetID() const
	{
		return m_ARTAssetID;
	}

	inline void SetARTAssetID(int set) 
	{
		m_ARTAssetID = set;
	}

	inline int GetNumJointLimitDofs () const
	{
#if CAPTURE_STACK_INVALIDATING_JOINT_LIMITS && !__SPU && __ASSERT
		if (m_Body->m_JointLimitsInvalid && m_NumLimitDofs > 0)
		{
			sysStack::PrintCapturedStackTrace(m_Body->m_JointLimitsStackTrace, phArticulatedBody::JOINT_LIMITS_STACK_TRACE_SIZE);
		}
#endif // CAPTURE_STACK_INVALIDATING_JOINT_LIMITS

		Assert(!m_Body->m_JointLimitsInvalid || m_NumLimitDofs == 0);

		return m_NumLimitDofs;
	}

	inline int GetNumDeepJointLimitDofs () const
	{
		return m_NumDeepLimitDofs;
	}

#if __PFDRAW
	void DrawJointLimitAngImpulse(int limitDofIndex, float angImpulse) const;
#endif

	inline void ApplyJointLimitAngImpulse (int limitDofIndex, ScalarV_In angImpulse) const
	{
		m_Body->ApplyJointImpulse(m_LimitsJointIndex[limitDofIndex],m_LimitsDofIndex[limitDofIndex],angImpulse);
	}

	inline ScalarV_Out GetJointLimitExcess (int limitDofIndex) const
	{
		FastAssert(limitDofIndex < m_NumLimitDofs);
		return ScalarVFromF32(m_LimitsExcessHard[limitDofIndex]);
	}

	// PURPOSE: Flag the joint supporting the specified body part to add its joint limits to the contact force solver, even if they are not violated.
	// PARAMS:
	//	bodyPartIndex - the index of the body part controlled by the joint whose limits should be enforced
	// NOTES:	This is used to make sure joint limits are not violated from pushes (turns) in collisions.
	void MakeParentJointEnforceLimits (int component);

	inline void InitAccumJointImpulse()
	{
		sysMemSet(&m_AccumJointImpulse[0], 0, sizeof(float) * m_AccumJointImpulse.GetCapacity());
	}

	inline void ApplyIncrementalJointLimitImpulse( int jointLimitIndex, ScalarV_In impulse )
	{
		m_Body->ApplyIncrementalJointLimitImpulse( m_LimitsJointIndex[ jointLimitIndex ], m_LimitsDofIndex[ jointLimitIndex ], impulse );
	}

	inline Vec3V_Out GetIncrementalJointLimitVelocity( int jointLimitIndex )
	{
		return m_Body->GetIncrementalJointLimitVelocity( m_LimitsJointIndex[ jointLimitIndex ], m_LimitsDofIndex[ jointLimitIndex ] );
	}

	inline int GetJointLimitIndex( int jointLimitIndex ) const
	{
		return m_LimitsJointIndex[ jointLimitIndex ];
	}

	inline int GetJointLimitDofIndex( int jointLimitIndex ) const
	{
		return m_LimitsDofIndex[ jointLimitIndex ];
	}

	inline ScalarV_Out GetJointLimitResponseV( int jointLimitIndex ) const
	{
		FastAssert(jointLimitIndex>=0 && jointLimitIndex<m_NumLimitDofs);
		return ScalarVFromF32(m_LimitResponse[jointLimitIndex]);
	}

	inline ScalarV_Out GetAccumJointImpulse( int jointLimitIndex ) const
	{
		FastAssert(jointLimitIndex>=0 && jointLimitIndex<m_NumLimitDofs);
		return ScalarVFromF32(m_AccumJointImpulse[jointLimitIndex]);
	}

	inline void SetAccumJointImpulse( int jointLimitIndex, ScalarV_In accumImpulse )
	{
		FastAssert(jointLimitIndex>=0 && jointLimitIndex<m_NumLimitDofs);
		StoreScalar32FromScalarV(m_AccumJointImpulse[jointLimitIndex], accumImpulse);
	}

	void MoveColliderWithRootLink ();

	// <COMBINE phCollider::Rejuvenate>
	bool RejuvenateImp ();

	// PURPOSE: Set the number of updates between each composite BVH rebuild, any collider update between rebuilds will perform BVH updates instead
	// PARAMS:
	//   bvhRebuildPeriod - number of updates between bvh rebuilds
	// NOTES:
	//   On articulated colliders with parts that move significantly, the composite BVH should be rebuilt frequently to prevent slow queries.
	void SetBvhRebuildPeriod(u16 bvhRebuildPeriod);

	// PURPOSE: Update or rebuild the composite's BVH based on the rebuild period and the current update count
	// PARAMS:
	//   compositeBound - reference to this articulated collider's composite bound (No look up inside function, works better w/ SPU)
	void RefreshBvh(phBoundComposite& compositeBound);

	static void ClearActive();
	static void InitAllAccumJointImpulse();
	static void SaveAndZeroAllVelocitiesForPushes();
	static void EnsureAllArtVelocitiesFullyPropagated();

#if __BANK
	static void GetNumberOfMedAndLodLODBodies(int &numberMedLODBodies, int &numberLowLODBodies);
#endif

	static void SetActiveArrayInfo(phArticulatedCollider** activeArray, u32* activeCount);
	static phArticulatedCollider** GetActiveArray();
	static u32* GetActiveCountPtr();

	void AddToActiveArray();

#if __PFDRAW
	PH_NON_SPU_VIRTUAL void ProfileDraw() const;
#endif

	void SetDenyColliderSimplificationFrames(u16 frames) { m_DenyColliderSimplificationFrames = frames; }
	u16 GetDenyColliderSimplificationFrames() const { return m_DenyColliderSimplificationFrames; }

	void SetUsingInstanceToArtColliderOffsetMat(bool set) { m_UsingInstanceToArtColliderOffsetMat = set; } 
	bool IsUsingInstanceToArtColliderOffsetMat() const { return m_UsingInstanceToArtColliderOffsetMat; } 
	void SetOffsetMatrix(Matrix34 &set) { m_OffsetMat = set; }
	const Matrix34 & GetOffsetMatrix() const { return m_OffsetMat; }

	void SetOwnsSelfCollisionElements(bool set) { m_OwnsSelfCollisionElements = set; }
	bool GetOwnsSelfCollisionElements() const { return m_OwnsSelfCollisionElements; }

	void SetLOD(int set) { m_LOD = set; }
	int GetLOD() const { return m_LOD; }

#if __PS3
	class DmaPlan : public sysDmaPlan
	{
	public:
		static const int MAX_NUM_FIXUPS = phArticulatedBodyType::MAX_NUM_LINKS * 5 + 6;
		static const int MAX_NUM_DMAS = phArticulatedBodyType::MAX_NUM_LINKS + phArticulatedBodyType::MAX_NUM_JOINTS * 2 + 12;

		DmaPlan()
			: m_ReadOnlyStorage(m_ReadOnlyBits, MAX_NUM_DMAS)
		{
			m_DmaList = m_DmaListStorage;
			m_Fixups = m_FixupStorage;
			m_MaxDmas = MAX_NUM_DMAS;
			m_MaxFixups = MAX_NUM_FIXUPS;
			m_ReadOnly = &m_ReadOnlyStorage;
#if __PPU
			memset(m_ReadOnlyBits,0,sizeof(m_ReadOnlyBits));
#else
			sysMemZeroBytes<sizeof(m_ReadOnlyBits)>(m_ReadOnlyBits);
#endif
		}

	private:
		CellDmaListElement m_DmaListStorage[MAX_NUM_DMAS];
		Fixup m_FixupStorage[MAX_NUM_FIXUPS];
		unsigned m_ReadOnlyBits[(MAX_NUM_DMAS+31)>>5];
		atUserBitSet m_ReadOnlyStorage;
	} ;

	u32 GetDmaPlanSizeArt()
	{
		return sizeof(DmaPlan);
	}

#if __PPU
	void RegenerateCoreDmaPlan();
	void GenerateCoreDmaPlan(sysDmaPlan& dmaPlan);
#endif
#endif

	int GetNumSelfCollisionPairs() const
	{
		return m_PartCanCollideA.GetCount();
	}

	u8 * GetSelfCollisionPairsA()
	{
		return &(m_PartCanCollideA[0]);
	}

	u8 * GetSelfCollisionPairsB()
	{
		return &(m_PartCanCollideB[0]);
	}

	atArray<u8,16> & GetSelfCollisionPairsArrayRefA()
	{
		return m_PartCanCollideA;
	}

	atArray<u8,16> & GetSelfCollisionPairsArrayRefB()
	{
		return m_PartCanCollideB;
	}

	void SetSelfCollisionPairsA(atArray<u8,16> &otherArray)
	{
		m_PartCanCollideA.CopyShallow(otherArray);
	}

	void SetSelfCollisionPairsB(atArray<u8,16> &otherArray)
	{
		m_PartCanCollideB.CopyShallow(otherArray);
	}

	bool GetAnyPartsCanCollide() const
	{
		return m_AnyPartsCanCollide;
	}

	void SetAnyPartsCanCollide(bool set)
	{
		m_AnyPartsCanCollide = set;
	}

	int GetDisableSelfCollisionFramesLeft() const
	{
		return m_DisableSelfCollisionFramesLeft;
	}

private:

	bool m_UsingInstanceToArtColliderOffsetMat;

	// This is used in forcesolver.cpp and contacttask.cpp to quickly check one criteria for whether two colliders
	// are using the same articulated body type data.  Otherwise would need to dma fetch data from the instance.
	int m_LOD;

public:
#if !__SPU
private:
#endif // !__SPU

	phArticulatedBody* m_Body;

	// Delete next time the class is changed to break resources.
	ATTR_UNUSED bool m_Unused;

	// joint degrees of freedom at limits, changed every time a joint reaches or leaves a limit
	u16 m_NumLimitDofs;

	// Total degrees of freedom past limits by more than the allowed penetration, so they need a push
	u16 m_NumDeepLimitDofs;

private:
	Matrix34 m_OffsetMat;
#if __SPU
public:
#endif // __SPU
	atArray<int,16> m_LimitsJointIndex;
	atArray<int,16> m_LimitsDofIndex;
	atArray<float,16> m_LimitsExcessHard;
	atArray<float,16> m_LimitResponse;
	atArray<float,16> m_AccumJointImpulse;

	// list of body part index numbers in order of the corresponding composite bound part number
	atArray<int,16> m_ComponentToLinkIndex;

	// whether or not two composite bound parts can collide (could be upper-diagonal bit field
	// for more complexity and minor memory savings)
	atArray<u8,16> m_PartCanCollideA;
	atArray<u8,16> m_PartCanCollideB;

public:
	atArray<Vec3V> m_SavedLinearVelocities;
	atArray<Vec3V> m_SavedAngularVelocities;
#if !__SPU
private:
#endif // !__SPU

	bool m_LargeTurnsTriggerPushCollisions;
	bool m_AnyPartsCanCollide;
	bool m_OwnsSelfCollisionElements;
	int m_DisableSelfCollisionFramesLeft;

	u16 m_BvhRebuildUpdateCount;	// number of updates since last BVH rebuild
	u16 m_BvhRebuildPeriod;			// number of updates between BVH rebuilds (0 by default)

	int m_ARTAssetID;

	u16 m_DenyColliderSimplificationFrames;

	s16 m_ReducedLinearDampingFrames;

	pgRef < pgArray<Mat34V> > m_LinkAttachment;

	static const int MAX_NUM_ACTIVE = 256;
	static phArticulatedCollider** sm_ActiveThisFrame;
	static u32 sm_NumActiveThisFrame;
	static u32* sm_NumActiveThisFramePtr;
};


inline void phArticulatedCollider::SetArticulatedLargeRoot (bool largeRoot)
{
	Assert(IsArticulated());
	if (largeRoot)
	{
		m_ColliderType = TYPE_ARTICULATED_LARGE_ROOT;
	}
	else
	{
		m_ColliderType = TYPE_ARTICULATED_BODY;
	}
}


inline phArticulatedBody* phArticulatedCollider::GetBody ()
{
	return m_Body;
}

inline const phArticulatedBody* phArticulatedCollider::GetBody () const
{
	return m_Body;
}

inline void phArticulatedCollider::ZeroLinkVelocities ()
{
	FastAssert(m_Body);
	m_Body->ZeroLinkVelocities();
}

inline void phArticulatedCollider::TemporarilyDisableSelfCollision(int numFrames)
{
	m_DisableSelfCollisionFramesLeft = numFrames + 1;
}

inline void phArticulatedCollider::DecrementSelfCollisionTemporaryFrames()
{
	m_DisableSelfCollisionFramesLeft = IDecrementToZero(m_DisableSelfCollisionFramesLeft);
}
 
inline float phArticulatedCollider::GetJointLimitResponse (int limitDofIndex) const
{
	FastAssert(limitDofIndex>=0 && limitDofIndex<m_NumLimitDofs);
	return m_LimitResponse[limitDofIndex];
}

inline float phArticulatedCollider::GetMassArt (int component) const
{
	return GetBody()->GetMass(GetLinkFromComponent(component)).Getf();
}

inline ScalarV_Out phArticulatedCollider::GetMassVArt (int component) const
{
	return GetBody()->GetMass(GetLinkFromComponent(component));
}

} // namespace rage


#endif // ARTICULATED_COLLIDER_H
