// 
// physics/manifold.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#if __SPU
#include "vector/vector3_consts_spu.cpp"
#endif // __SPU

#include "colliderdispatch.h"
#include "contact.h"
#include "inst.h"
#include "manifold.h"
#include "overlappingpairarray.h"
#include "simulator.h"
#include "spuevents.h"

#include "phcore/pool.h"
#include "phsolver/forcesolvertables.h"

#include <assert.h>

#include "grprofile/drawmanager.h"

#if __PFDRAW || !__SPU
#include "levelnew.h"
#endif // __PFDRAW

#include "pharticulated/articulatedcollider.h"
#include "phbound/boundcomposite.h"
#include "phbullet/btCollisionAlgorithm.h"
#include "math/amath.h"
#include "phcore/phmath.h"
#include "math/constants.h"
#include "system/spinlock.h"
#include "system/timemgr.h"
#include "vector/colors.h"
#include "vector/matrix33.h"

#if __ASSERT && __SPU
#include "system/task.h"
#endif

#if __ASSERT 
#include "phcore/materialmgr.h"
#endif

//OPTIMISATIONS_OFF()
namespace rage {

POSITIVE_DEPTH_ONLY_ONLY(int phManifold::sm_PositiveDepthMode =  phManifold::POSITIVE_DEPTH_ONLY_VELOCITY;)

// TODO: MOVE THESE CONTACT CONSTANTS INTO A SEPARATE HEADER.
const float phManifold::MANIFOLD_MARGIN = 0.02f;
const ScalarV phManifold::MANIFOLD_MARGINV(ScalarVFromF32(MANIFOLD_MARGIN));
const ScalarV phManifold::MANIFOLD_MARGINSQUAREDV(ScalarVFromF32(MANIFOLD_MARGIN*MANIFOLD_MARGIN));

#if PH_MANIFOLD_TRACK_IS_IN_OPA
void phManifold::AboutToBeReleased()
{
	Assert(!m_IsInOPA);
// 	Displayf("Releasing manifold 0x%p", this);
// 	sysStack::PrintStackTrace();
}
#endif // PH_MANIFOLD_TRACK_IS_IN_OPA

phManifold::phManifold()
	: m_CompositePointers(NULL)
	, m_ConstraintMatrix(NULL)
	, m_NumCompositeManifolds(0)
	, m_CachedPoints(0)
#if PH_MANIFOLD_TRACK_IS_IN_OPA
	, m_IsInOPA(false)
#endif // PH_MANIFOLD_TRACK_IS_IN_OPA
{
	m_Instance[0] = m_Instance[1] = NULL;
	m_Collider[0] = m_Collider[1] = NULL;
	m_Bound[0] = m_Bound[1] = NULL;
	m_LevelIndex[0] = m_LevelIndex[1] = phInst::INVALID_INDEX;
	m_GenerationId[0] = m_GenerationId[1] = phInst::INVALID_INDEX;	// I'm not really sure that this is the right 'invalid' value to use.
	m_Component[0] = m_Component[1] = 0;

	ResetInternal();

#if USE_FRAME_PERSISTENT_GJK_CACHE
	m_gjkCacheDB = NULL;
#endif
}

/*
#if !__SPU
void phManifold::Init (phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB)
{
	// Set the first object's instance and collider and get its level index.
	Assert(instanceA);
	Assert(!colliderA || colliderA->GetInstance()==instanceA);
	m_Instance[0] = instanceA;
	m_Collider[0] = colliderA;
	m_LevelIndex[0] = instanceA->GetLevelIndex();

	// Set the second object's instance and collider and get its level index.
	Assert(instanceB);
	Assert(!colliderB || colliderB->GetInstance()==instanceB);
	m_Instance[1] = instanceB;
	m_Collider[1] = colliderB;
	m_LevelIndex[1] = instanceB->GetLevelIndex();
	m_StaticFriction = true;
	m_ConstraintType = NO_CONSTRAINT_CONTACT;

	m_UsePushes = true;
	m_SeparateBias = 0.0f;
	m_IsConstraint = false;
	m_ConstraintMatrix = NULL;

#if __DEBUGLOG_PS3
	m_DebugLogSize = 0;
#endif

#if __DEBUGLOG
	DebugReplay();
#endif
}
#endif
*/

void phManifold::EnableCompositeManifolds()
{
	if (IsConstraint())
	{
		//Assertf(false, "Someone is trying to enable composite manifolds on a constraint manifold");
		return;
	}

	phPool<phContact>* contactPool = PHCONTACT;
	phContact** contactArray = m_Contacts;
	while (m_CachedPoints > 0)
	{
		contactPool->Release(contactArray[m_CachedPoints - 1]);
		--m_CachedPoints;
	}

	if (m_CompositePointers == NULL)
	{
		m_CompositePointers = PHCOMPOSITEPOINTERS->Allocate();
	}

	//m_CompositeManifold = true;
}

void phManifold::DisableCompositeManifolds()
{
	if (m_CompositePointers)
	{
		phPool<phManifold>* manifoldPool = PHMANIFOLD;
		while (m_NumCompositeManifolds > 0)
		{
#if __SPU
			phManifold* mmManifoldToRelease = m_CompositePointersLs->GetManifold(m_NumCompositeManifolds - 1);
			phManifold lsManifoldToRelease;
			cellDmaLargeGet(&lsManifoldToRelease, (uint64_t)mmManifoldToRelease, sizeof(rage::phManifold), 1, 0, 0);
			cellDmaWaitTagStatusAll(1<<1);
			lsManifoldToRelease.Reset();
			cellDmaLargePut(&lsManifoldToRelease, (uint64_t)mmManifoldToRelease, sizeof(rage::phManifold), 1, 0, 0);
			cellDmaWaitTagStatusAll(1<<1);

			manifoldPool->Release(m_CompositePointers->GetManifold(m_NumCompositeManifolds - 1));
#else
			manifoldPool->Release(GetCompositeManifold(m_NumCompositeManifolds - 1));
#endif
			--m_NumCompositeManifolds;
		}

		PHCOMPOSITEPOINTERS->Release(m_CompositePointers);

		m_CompositePointers = NULL;

#if USE_FRAME_PERSISTENT_GJK_CACHE
		ResetGJKCache();
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
	}

	//m_CompositeManifold = false;

	Assert(m_NumCompositeManifolds == 0);
}

#if __SPU
void phManifold::GetCompositePointersFromMm(u32 tag)
{
	if (m_CompositePointers)
	{
		sysDmaLargeGet(m_CompositePointersLs, (uint64_t)m_CompositePointers, sizeof(phCompositePointers), tag);
	}
}

void phManifold::PutCompositePointersToMm(u32 tag)
{
	if (m_CompositePointers)
	{
		sysDmaPut(m_CompositePointersLs, (uint64_t)m_CompositePointers, sizeof(phCompositePointers), tag);
	}
}

void phManifold::GatherContactPointsFromMm(u32 tag)
{
	for (int contact = 0; contact < m_CachedPoints; ++contact)
	{
		sysDmaLargeGet(m_ContactsLs[contact], (uint64_t)m_Contacts[contact], sizeof(phContact), tag);
	}
}

void phManifold::ScatterContactPointsToMm(u32 tag)
{
	for (int contact = 0; contact < m_CachedPoints; ++contact)
	{
		sysDmaPut(m_ContactsLs[contact], (uint64_t)m_Contacts[contact], sizeof(phContact), tag);
	}
}

void phManifold::GetConstraintMatrixFromMm(u32 tag)
{
	if (m_ConstraintMatrix)
	{
		sysDmaLargeGet(m_ConstraintMatrixLs, (uint64_t)m_ConstraintMatrix, sizeof(Mat33V), tag);
	}
}

void phManifold::PutConstraintMatrixToMm(u32 tag)
{
	if (m_ConstraintMatrix)
	{
		sysDmaPut(m_ConstraintMatrixLs, (uint64_t)m_ConstraintMatrix, sizeof(Mat33V), tag);
	}
}

#endif // __SPU

void phManifold::TransferContactPoints (phManifold &fromManifold)
{
	Assert(GetNumContacts() == 0);
	const int numOtherContacts = fromManifold.GetNumContacts();
	for(int contactIndex = 0; contactIndex < numOtherContacts; ++contactIndex)
	{
		m_Contacts[contactIndex] = &fromManifold.GetContactPoint(contactIndex);
	}
	m_CachedPoints = (u8)numOtherContacts;
	fromManifold.m_CachedPoints = 0;
}



__forceinline void phManifold::ResetInternal()
{
	m_MassInvScale[0] = 1.0f;
	m_MassInvScale[1] = 1.0f;

	m_MassInv[0] = 1.0f;
	m_MassInv[1] = 1.0f;
	m_UserData = 0;

	m_ConstraintType = NO_CONSTRAINT_CONTACT;

	m_NewestPoint = -1;

	m_StaticFriction = true;
	m_InactiveCollidesAgainstInactiveA = false;
	m_InactiveCollidesAgainstInactiveB = false;
	m_UsePushes = true;
	//m_CompositeManifold = false;
	m_IsConstraint = false;
	m_ConstraintMatrix = NULL;

	m_CollisionTime = 0;

#if __DEBUGLOG_PS3
	m_DebugLogSize = 0;
#endif
}

void phManifold::Reset()
{
	m_IsConstraint = false;

	RemoveAllContacts();

#if USE_FRAME_PERSISTENT_GJK_CACHE
	ResetGJKCache();
#endif // USE_FRAME_PERSISTENT_GJK_CACHE

	m_Instance[0] = NULL;
	m_Instance[1] = NULL;
	m_Collider[0] = NULL;
	m_Collider[1] = NULL;

	m_Bound[0] = NULL;
	m_Bound[1] = NULL;

	ResetInternal();
}


#if USE_FRAME_PERSISTENT_GJK_CACHE
void phManifold::ResetGJKCache()
{
	if (m_gjkCacheDB)
	{
		m_gjkCacheDB->SetDelete();
		m_gjkCacheDB = NULL;
	}
}
#endif // USE_FRAME_PERSISTENT_GJK_CACHE

void phManifold::RemoveAllContacts()
{
	if (IsConstraint())
	{
		//Assertf(false, "Someone is trying to remove the contacts from a constraint manifold");
		return;
	}

	DisableCompositeManifolds();

	if (m_CachedPoints > 0)
	{
		phPool<phContact>* contactPool = PHCONTACT;
		phContact** contactArray = m_Contacts;
		while (m_CachedPoints > 0)
		{
			contactPool->Release(contactArray[m_CachedPoints - 1]);
			--m_CachedPoints;
		}
	}

#if USE_FRAME_PERSISTENT_GJK_CACHE
	ResetGJKCache();
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

/*
#if __SPU
void phManifold::SpuReset()
{
	m_NumCompositeManifolds = 0;
	m_CachedPoints = 0;
	m_CompositePointers = NULL;
	m_CompositePointersLs = NULL;
	m_ConstraintMatrix = NULL;
	m_ConstraintMatrixLs = NULL;

	ResetInternal();
}
#endif // __SPU
*/

#if __WIN32
#pragma warning( disable:4100 )
#pragma warning( disable:4101 )
#endif

Vector3 TriArea( Vector3::Param a, Vector3::Param b, Vector3::Param leftOfAB_nestpas, Vector3::Param n )
{
	Vector3 ba, la, bax;
	ba.Subtract( b, a );
	la.Subtract( leftOfAB_nestpas, a );
	bax.Cross(ba,la);
	return bax.DotV(n);
}

bool phManifold::ObjectsInContact() const
{
	int numContacts = GetNumContacts();
	if (IsConstraint())
	{
		return numContacts > 0 && GetContactPoint(0).IsContactActive();
	}
	else if (CompositeManifoldsEnabled())
	{
		return GetNumCompositeManifolds() > 0;
	}
	else
	{
		return numContacts > 0;
	}
}

bool phManifold::HasDeepContacts(float depthThreshold, bool BANK_ONLY(ttyOutput)) const
{
	int numContacts = GetNumContacts();
	if (IsConstraint())
	{
		return numContacts > 0 && GetContactPoint(0).GetDepth() > depthThreshold;
	}
	else if (CompositeManifoldsEnabled())
	{
		const int numCompositeManifolds = GetNumCompositeManifolds();

		for(int compositeIndex = 0; compositeIndex < numCompositeManifolds; ++compositeIndex)
		{
			phManifold* compositeManifold = GetCompositeManifold(compositeIndex);

			int numContacts = compositeManifold->GetNumContacts();
			for (int contactIndex = 0; contactIndex < numContacts; ++contactIndex)
			{
				const phContact& contact = compositeManifold->GetContactPoint(contactIndex);
				if (contact.IsContactActive() && contact.GetDepth() > depthThreshold)
				{
#if __BANK
					if (ttyOutput)
					{
						Displayf("Push collision caused by deep (%f > threshold %f) contacts between %s and %s",
							contact.GetDepth(), depthThreshold,
							GetInstanceA() ? GetInstanceA()->GetArchetype()->GetFilename() : NULL,
							GetInstanceB() ? GetInstanceB()->GetArchetype()->GetFilename() : NULL);
					}
#endif // __BANK

					return true;
				}
			}
		}
	}
	else
	{
		int numContacts = GetNumContacts();
		for (int contactIndex = 0; contactIndex < numContacts; ++contactIndex)
		{
			const phContact& contact = GetContactPoint(contactIndex);
			if (contact.IsContactActive() && contact.GetDepth() > depthThreshold)
			{
				return true;
			}
		}
	}

	return false;
}

#if !__SPU
int phManifold::GetActiveOrInactiveLevelIndexA () const
{
	if (m_LevelIndex[0]!=phInst::INVALID_INDEX && PHLEVEL->IsInLevel(m_LevelIndex[0]) && (PHLEVEL->IsActive(m_LevelIndex[0]) || PHLEVEL->IsInactive(m_LevelIndex[0])))
	{
		return m_LevelIndex[0];
	}

	return phInst::INVALID_INDEX;
}


int phManifold::GetActiveOrInactiveLevelIndexB () const
{
	if (m_LevelIndex[1]!=phInst::INVALID_INDEX && PHLEVEL->IsInLevel(m_LevelIndex[1]) && (PHLEVEL->IsActive(m_LevelIndex[1]) || PHLEVEL->IsInactive(m_LevelIndex[1])))
	{
		return m_LevelIndex[1];
	}

	return phInst::INVALID_INDEX;
}
#endif


int phManifold::SortCachedPoints( const phContact& pt ) //, Vector3::Param localNormalBinA ) 
{
//	phContact oldMan[MANIFOLD_CACHE_SIZE + 1];
	// We should only be getting in here if the manifold is full.  If that's really the case then a lot of stuff below could get simplified.
	Assert(m_CachedPoints == MANIFOLD_CACHE_SIZE);
	Assert(m_CachedPoints <= (MANIFOLD_CACHE_SIZE + 1));
//	sysMemCpy( oldMan, m_PointCacheBytes, sizeof( phContact )*m_CachedPoints );

	Vector3 vecMan[MANIFOLD_CACHE_SIZE + 1];
	int intMan[MANIFOLD_CACHE_SIZE + 1];// = Alloca( int, m_CachedPoints+1 );  // this holds the original positions of the manifold
	//int *stackMan = Alloca( int, m_CachedPoints+1 );

	int iMan;
	for( iMan = 0; iMan < m_CachedPoints; iMan++ )
	{
		vecMan[iMan] = VEC3V_TO_VECTOR3(GetContactPoint(iMan).GetLocalPosA());
		intMan[iMan] = iMan;
	}

//	oldMan[m_CachedPoints] = pt;
	// put new point at end of list 
	vecMan[m_CachedPoints] = VEC3V_TO_VECTOR3(pt.GetLocalPosA());
	intMan[m_CachedPoints] = m_CachedPoints;

	// Find a non-unit normal vector for the set of points.
	Vector3 nonUnitNormal;
	nonUnitNormal.Cross( vecMan[1] - vecMan[0], vecMan[2] - vecMan[0] );

	// Try to make sure the non-unit normal vector is not too small.
	float normalMag2 = nonUnitNormal.Mag2();
	int nextVertexIndex = 3;
	while (normalMag2<SMALL_FLOAT && m_CachedPoints>nextVertexIndex)
	{
		nonUnitNormal.Cross(vecMan[1]-vecMan[0],vecMan[nextVertexIndex]-vecMan[0]);
		normalMag2 = nonUnitNormal.Mag2();
		nextVertexIndex++;
	}

	Vector3 minCheck;
	minCheck.Cross( nonUnitNormal, vecMan[1] - vecMan[0] );

	// find a point on the convex hull
	Vector3 minx = Vector3(1e30f, 1e30f, 1e30f);
	int iMinx = 0;
	for( iMan = 0; iMan < m_CachedPoints +1; iMan++ )
	{
		if( vecMan[iMan].DotV( minCheck ).IsLessThanAll( minx ) )
		{
			minx = vecMan[iMan];
			iMinx = iMan;
		}
	}

	// swap into position 0, so it is now the pivot we sort around
	rage::SwapEm( vecMan[iMinx], vecMan[0] );
	rage::SwapEm( intMan[iMinx], intMan[0] );

	// sort radially
	// could sort the retained array, so this would be just an O(n) pass through, but in those cases we probably wont even enter this code much
	// The loopGuard was added because there appeared to be some cases in which swaps were happening every time through the loop, presumably
	//   due to floating point inaccuracy, and the loop was never terminating.
	bool hasSwapped;
	int loopGuard = m_CachedPoints;
	do
	{
		hasSwapped = false;
		for( iMan = 1; iMan < m_CachedPoints; iMan++ )
		{
			if( TriArea( vecMan[0], vecMan[iMan], vecMan[iMan+1], nonUnitNormal ).IsLessThanDoNotUse( VEC3_ZERO ) )
			{
				rage::SwapEm( intMan[iMan], intMan[iMan+1] );
				rage::SwapEm( vecMan[iMan], vecMan[iMan+1] );
				hasSwapped = true;
			}
		}
	} while( hasSwapped && (--loopGuard > 0) );

	// now do the graham scan convex hull algorithm
	ASSERT_ONLY(int stackTop);
/*
	stackTop = 2;
	stackMan[0] = 0;
	stackMan[1] = 1;
	iMan = 2;
	while( iMan < m_CachedPoints +1 )
	{
		Vector3 a = vecMan[stackMan[stackTop-2]];
		Vector3 b = vecMan[stackMan[stackTop-1]];
		Vector3 c = vecMan[iMan];
		Vector3 area = TriArea( a, b, c, n );
		if( area.IsLessThanDoNotUse( VEC3_ZERO ) )
		{
			// careful here.  make sure a point that fails this test also 
			Assert( stackTop > 1 );
			stackTop--;
		}
		else
		{
			stackMan[stackTop] = iMan;
			stackTop++;
			iMan++;
		}
	}

	// the stack holds our convex hull
	for( iMan = 0; iMan < stackTop; iMan++ )
	{
		m_PointCache[iMan] = oldMan[intMan[stackMan[iMan]]];
	}
*/
	ASSERT_ONLY(stackTop = m_CachedPoints+1);
	// if no points were removed then find the one with the least area reduction to remove
	Assert( m_CachedPoints == stackTop-1 );
	//if( m_CachedPoints == stackTop-1 )
	{

		Vector3 minArea = Vector3(1e30f, 1e30f, 1e30f);
		int iMinArea = 0;
		for( iMan = 0; iMan < m_CachedPoints +1; iMan++ )
		{
			Vector3 a = vecMan[iMan];
			Vector3 b = vecMan[(iMan+1)%(m_CachedPoints +1)];
			Vector3 c = vecMan[(iMan+2)%(m_CachedPoints +1)];

			Vector3 areaMan = TriArea( a,b,c,nonUnitNormal );

			int throwAway = (iMan+1)%(m_CachedPoints +1);
			
			// only toss the candidate point if it is within the current manifold
			if( intMan[throwAway] == m_CachedPoints )
			{
				if( areaMan.IsLessThanDoNotUse( VEC3_ZERO ) )
				{
					return m_CachedPoints;
				}
			}
			else if( areaMan.IsLessThanDoNotUse( minArea ) )
			{
				minArea = areaMan;
				iMinArea = throwAway;
			}
		}

		return intMan[iMinArea];
	}
//	else
//	{
//		m_CachedPoints = stackTop;
//		return m_CachedPoints;
//	}


/*	int iExt;

	int oldest = -1;
	int old = -1000;
	for( iExt = 0; iExt < m_CachedPoints; iExt++ )
	{
		if( m_PointCache[iExt].GetLifeTime() > old )
		{
			oldest = iExt;
			old = m_PointCache[iExt].GetLifeTime();
		}
	}

	Assert( oldest >= 0 );

	return oldest;
*/	
	
/*	
	int shallowest = -1;
	float shallow = 10000.0f;
	for( iExt = 0; iExt < m_CachedPoints; iExt++ )
	{
		if( m_PointCache[iExt].GetDepth() < shallow )
		{
			shallowest = iExt;
			shallow = m_PointCache[iExt].GetDepth();
		}
	}

	Assert( shallowest >= 0 );

	return shallowest;
*/
/*
	Vector3 u,v;
	u = pt.m_LocalPointA - m_PointCache[0].m_LocalPointA;
	v.Cross( localNormalBinA, u );
	
	u.NormalizeFast();
	v.NormalizeFast();

	Vector3 minU = VEC3_MAX, maxU = -VEC3_MAX;
	Vector3 minV = VEC3_MAX, maxV = -VEC3_MAX;
	int iMinU = 0, iMaxU = 0;
	int iMinV = 0, iMaxV = 0;

	for( iExt = 0; iExt < m_CachedPoints; iExt++ )
	{

		Vector3 au = (pt.m_LocalPointA - m_PointCache[iExt].m_LocalPointA).DotV( u );
		Vector3 av = (pt.m_LocalPointA - m_PointCache[iExt].m_LocalPointA).DotV( v );

		if( au.IsGreaterThan( maxU ) )
		{
			iMaxU = iExt+1;
			maxU = au;
		}

		if( av.IsGreaterThan( maxV ) )
		{
			iMaxV = iExt+1;
			maxV = av;
		}

		if( au.IsLessThanDoNotUse( minU ) )
		{
			iMinU = iExt+1;
			minU = au;
		}

		if( av.IsLessThanDoNotUse( minV ) )
		{
			iMinV = iExt+1;
			minV = av;
		}

	}
*/

/*

	int addedMask = 0;
	addedMask = (1 << (iMinU-1) | addedMask);
	addedMask = (1 << (iMinV-1) | addedMask);
	addedMask = (1 << (iMaxU-1) | addedMask);
	addedMask = (1 << (iMaxV-1) | addedMask);

	// replace the first one that wasn't in our reduced manifold
	for( iExt = 0; iExt < m_CachedPoints; iExt++ )
	{
		if( ((1 << iExt) | addedMask) == 0 )
		{
			return iExt;
		}
	}
	
	// if we get here then throw out the shallowest point
	int shallowest = -1;
	float shallow = 10000.0f;
	for( iExt = 0; iExt < m_CachedPoints; iExt++ )
	{
		if( m_PointCache[iExt].GetDepth() < shallow )
		{
			shallowest = iExt;
			shallow = m_PointCache[iExt].GetDepth();
		}
	}

	Assert( shallowest >= 0 );

	return shallowest;

*/
/*/
	phContact *oldMan = Alloca( phContact, m_CachedPoints );
	sysMemCpy( oldMan, m_PointCache, sizeof( phContact )*m_CachedPoints );
	// implicit in the return value 
	// m_PointCache[0] = pt;
	
	int addedMask = 1;
	int iAdd = 1;

	if( ((1 << iMinU) & addedMask) == 0 )
	{
		m_PointCache[iAdd++] = oldMan[iMinU-1];
		addedMask = ((1 << iMinU) | addedMask);
	}

	if( ((1 << iMinV) & addedMask) == 0 )
	{
		m_PointCache[iAdd++] = oldMan[iMinV-1];
		addedMask = ((1 << iMinV) | addedMask);
	}

	if( ((1 << iMaxU) & addedMask) == 0 )
	{
		m_PointCache[iAdd++] = oldMan[iMaxU-1];
		addedMask = ((1 << iMaxU) | addedMask);
	}

	if( ((1 << iMaxV) & addedMask) == 0 )
	{
		m_PointCache[iAdd++] = oldMan[iMaxV-1];
		addedMask = ((1 << iMaxV) | addedMask);
	}

	m_CachedPoints = iAdd;

	Assert( m_CachedPoints <= MANIFOLD_CACHE_SIZE );

	return 0;
*/

/*
	Vector3 res[MANIFOLD_CACHE_SIZE];
	{
		Vector3 a0 = pt.m_LocalPointA-m_PointCache[1].m_LocalPointA;
		Vector3 b0 = m_PointCache[3].m_LocalPointA - m_PointCache[2].m_LocalPointA;
		Vector3 x;
		x.Cross( a0, b0 );
		res[0] = x.Mag2V();
	}

	{
		Vector3 a0 = pt.m_LocalPointA-m_PointCache[0].m_LocalPointA;
		Vector3 b0 = m_PointCache[3].m_LocalPointA - m_PointCache[2].m_LocalPointA;
		Vector3 x;
		x.Cross( a0, b0 );
		res[1] = x.Mag2V();
	}

	{
		Vector3 a0 = pt.m_LocalPointA-m_PointCache[0].m_LocalPointA;
		Vector3 b0 = m_PointCache[3].m_LocalPointA - m_PointCache[1].m_LocalPointA;
		Vector3 x;
		x.Cross( a0, b0 );
		res[2] = x.Mag2V();
	}

	{
		Vector3 a0 = pt.m_LocalPointA-m_PointCache[0].m_LocalPointA;
		Vector3 b0 = m_PointCache[2].m_LocalPointA - m_PointCache[1].m_LocalPointA;
		Vector3 x;
		x.Cross( a0, b0 );
		res[3] = x.Mag2V();
	}

	Vector3 maxArea = VEC3_SMALL_FLOAT;
	int throwAway = m_CachedPoints;
	if( res[0].IsGreaterThan( maxArea ) )
	{
		maxArea = res[0];
		throwAway = 0;
	}

	if( res[1].IsGreaterThan( maxArea ) )
	{
		maxArea = res[1];
		throwAway = 1;
	}

	if( res[2].IsGreaterThan( maxArea ) )
	{
		maxArea = res[2];
		throwAway = 2;
	}

	if( res[3].IsGreaterThan( maxArea ) )
{
		maxArea = res[3];
		throwAway = 3;
	}

	return throwAway;
*/

	/*
	if (m_CachedPoints >= 4)
	{
		Vector3 cross[4];
		Vector3 a0 = m_PointCache[0].m_LocalPointA-m_PointCache[1].m_LocalPointA;
		Vector3 b0 = m_PointCache[0].m_LocalPointA-m_PointCache[3].m_LocalPointA;
		cross[0].Cross(a0, b0);

		if (cross[0].Mag2() < a0.Mag2() * SMALL_FLOAT)
		{
			return 0;
		}

		Vector3 a1 = m_PointCache[1].m_LocalPointA-m_PointCache[2].m_LocalPointA;
		Vector3 b1 = m_PointCache[1].m_LocalPointA-m_PointCache[0].m_LocalPointA;
		cross[1].Cross(a1, b1);

		if (cross[1].Mag2() < a1.Mag2() * SMALL_FLOAT)
		{
			return 1;
		}

		Vector3 a2 = m_PointCache[2].m_LocalPointA-m_PointCache[3].m_LocalPointA;
		Vector3 b2 = m_PointCache[2].m_LocalPointA-m_PointCache[1].m_LocalPointA;
		cross[2].Cross(a2, b2);

		if (cross[2].Mag2() < a2.Mag2() * SMALL_FLOAT)
		{
			return 2;
		}

		Vector3 a3 = m_PointCache[3].m_LocalPointA-m_PointCache[0].m_LocalPointA;
		Vector3 b3 = m_PointCache[3].m_LocalPointA-m_PointCache[2].m_LocalPointA;
		cross[3].Cross(a3, b3);

		if (cross[3].Mag2() < a3.Mag2() * SMALL_FLOAT)
		{
			return 3;
		}

		float dot01 = cross[0].Dot(cross[1]);
		float dot12 = cross[1].Dot(cross[2]);
		float dot23 = cross[2].Dot(cross[3]);
		float dot30 = cross[3].Dot(cross[0]);

		int chirality = (dot01 > 0.0f ? 1 : -1) +
						(dot12 > 0.0f ? 3 : -3) +
						(dot23 > 0.0f ? 3 : -3) +
						(dot30 > 0.0f ? 1 : -1);

		if (chirality == 0)
		{
			if (dot01 > 0.0f)
			{
				rage::SwapEm(m_PointCache[0], m_PointCache[1]);
				rage::SwapEm(cross[0], cross[1]);
			}
			else
			{
				rage::SwapEm(m_PointCache[1], m_PointCache[2]);
				rage::SwapEm(cross[1], cross[2]);
			}
		}

        float dist0 = pt.m_LocalPointA.Dist2(m_PointCache[0].m_LocalPointA);
		float dist1 = pt.m_LocalPointA.Dist2(m_PointCache[1].m_LocalPointA);
		float dist2 = pt.m_LocalPointA.Dist2(m_PointCache[2].m_LocalPointA);
		float dist3 = pt.m_LocalPointA.Dist2(m_PointCache[3].m_LocalPointA);

        int closest = -1;
        float maxVal = -1e30f;
        if (dist0 > maxVal)
        {
            closest = 0;
            maxVal = dist0;
        }
        if (dist1 > maxVal)
        {
            closest = 1;
            maxVal = dist1;
        }
        if (dist2 > maxVal)
        {
            closest = 2;
            maxVal = dist2;
        }
        if (dist3 > maxVal)
        {
            closest = 3;
            maxVal = dist3;
        }

		int next = closest == 3 ? 0 : closest + 1;
		int prev = closest == 0 ? 3 : closest - 1;

		Vector3 aPt = pt.m_LocalPointA-m_PointCache[next].m_LocalPointA;
		Vector3 bPt = pt.m_LocalPointA-m_PointCache[prev].m_LocalPointA;
		Vector3 crossPt;
        crossPt.Cross(aPt, bPt);

		if (crossPt.Mag2() > cross[closest].Mag2())
		{
			return closest;
		}
	}

	return m_CachedPoints;
*/
}

int phManifold::GetClosestPointWithinMargin (const phContact& newPoint) const
{
	ScalarV shortestDist2 =  square(GetManifoldMarginV());
	int nearestPoint = -1;
	for (int pointIndex=0; pointIndex<m_CachedPoints; pointIndex++)
	{
		if (pointIndex < m_CachedPoints - 1)
		{
			PrefetchContactPoint(pointIndex + 1);
		}
		const phContact& mp = GetContactPoint(pointIndex);
		ScalarV dist2ToManiPoint = DistSquared(mp.GetLocalPosA(), newPoint.GetLocalPosA());
		if (mp.GetLifetime() > 0 && IsLessThanAll(dist2ToManiPoint, shortestDist2))
		{
			shortestDist2 = dist2ToManiPoint;
			nearestPoint = pointIndex;
		}
	}

	return nearestPoint;
}

void phManifold::AddManifoldPoint(const phContact& newPoint)
{
	//Verifyf(!CompositeManifoldsEnabled(),"Trying to add a contact point to a composite manifold.");
	Assert(!CompositeManifoldsEnabled());
	DisableCompositeManifolds();

#if __DEBUGLOG
	newPoint.DebugReplay();
#endif

	int insertIndex = m_CachedPoints;
	Assert(insertIndex >= 0);
	if (insertIndex < MANIFOLD_CACHE_SIZE)
	{
		phContact* newContact = PHCONTACT->Allocate();
		m_Contacts[insertIndex] = newContact;

		if (newContact == NULL)
		{
			return;
		}
		else
		{
			m_CachedPoints++;
		}
	}
	else
	{
		//sort cache so best points come first, based on area
		insertIndex = SortCachedPoints(newPoint);
	}

	if (insertIndex < m_CachedPoints)
	{
		ReplaceContactPoint(newPoint,insertIndex);
	}
}

int phManifold::FindContactPointIndex (phContact *contact) const
{
	phContact*const * contactArray = m_Contacts;
	int contacts = GetNumContacts();

	for (int x=0; x<contacts; x++)
	{
		if (contactArray[x] == contact)
		{
			return x;
		}
	}

	Assertf(false, "Cannot find contact %p inside manifold", contact);
	return -1;
}


void phManifold::RemoveContactPoint (int index)
{
	if (IsConstraint())
	{
		//Assertf(false, "Someone is trying to remove a contact from a constraint manifold");
		return;
	}

	Assert(!CompositeManifoldsEnabled());
	Assert(m_CachedPoints > 0 && index >= 0 && index < m_CachedPoints);

	phContact** contactArray = m_Contacts;

	PHCONTACT->Release(contactArray[index]);

	contactArray[index] = contactArray[GetNumContacts() - 1];
#if __SPU
	// Also copy the LS contact
//	GetContactPoint(index) = GetContactPoint(m_CachedPoints - 1);
	phContact* tempContact = m_ContactsLs[index];
	m_ContactsLs[index] = m_ContactsLs[GetNumContacts() - 1];
	m_ContactsLs[GetNumContacts() - 1] = tempContact;
#endif
	m_CachedPoints--;

	if(m_NewestPoint == index)
	{
		ClearNewestContactPoint();
	}
}

void phManifold::ReplaceContactPoint(const phContact& newPoint, int insertIndex)
{
	Assert(!CompositeManifoldsEnabled());
	Assert(insertIndex < m_CachedPoints);
	Assert(insertIndex > -127 && insertIndex < 127);
    Assert(insertIndex >= 0 && insertIndex < MANIFOLD_CACHE_SIZE);

    GetContactPoint(insertIndex) = newPoint;
    m_NewestPoint = (u8)insertIndex;
}

void phManifold::IncrementContactLifetimes()
{
	for (int contactIndex=GetNumContacts()-1; contactIndex>=0; contactIndex--)
	{
		GetContactPoint(contactIndex).IncrementLifetime();
	}
}

void phManifold::GetLocalToWorldTransformA (Matrix34& transformA) const
{
	phInst* instanceA = GetInstanceA();
	if (instanceA)
	{
#if __SPU
		static const int tag = 1;
		u8 instBuffer[sizeof(phInst)] ;
		sysDmaGet(instBuffer, (uint64_t)instanceA, sizeof(phInst), tag);
		sysDmaWaitTagStatusAll(1<<tag);
		instanceA = (phInst*)instBuffer;

		const phBound* boundA = (const phBound*)sysDmaGetUInt32((uint64_t)instanceA->GetArchetype()->GetBoundPtr(), tag);
		u8 boundBuffer[sizeof(phBoundComposite)] ;
		sysDmaGet(boundBuffer, (uint64_t)boundA, sizeof(phBoundComposite), tag);
		sysDmaWaitTagStatusAll(1<<tag);
		boundA = (const phBound*)boundBuffer;
#else
		const phBound* boundA = instanceA->GetArchetype()->GetBound();
#endif

		transformA.Set( *(const Matrix34*)(&instanceA->GetMatrix()) );

		if (boundA->GetType()==phBound::COMPOSITE)
		{
			const phBoundComposite* compositeA = static_cast<const phBoundComposite*>(boundA);
			Matrix34 componentMatrix;
#if __SPU
			int component = GetComponentA();
			if (Verifyf(component>=0 && component < compositeA->GetNumBounds(), "Component %d out of range (0 to %d)", component, compositeA->GetNumBounds()))
			{
				sysDmaGet(&componentMatrix, uint64_t(&compositeA->GetCurrentMatrix(component)), sizeof(Matrix34), tag);
				sysDmaWaitTagStatusAll(1<<tag);
				transformA.DotFromLeft(componentMatrix);
			}
#else
			if (compositeA->GetCompositePart(GetComponentA(),RC_MAT34V(componentMatrix)))
			{
				transformA.DotFromLeft(componentMatrix);
			}
#endif
		}
	}
	else
	{
		transformA.Identity();
	}
}


void phManifold::GetLocalToWorldTransformB (Matrix34& transformB) const
{
	phInst* instanceB = GetInstanceB();
	if (instanceB)
	{
#if __SPU
		static const int tag = 1;
		u8 instBuffer[sizeof(phInst)] ;
		sysDmaGet(instBuffer, (uint64_t)instanceB, sizeof(phInst), tag);
		sysDmaWaitTagStatusAll(1<<tag);
		instanceB = (phInst*)instBuffer;

		const phBound* boundB = (const phBound*)sysDmaGetUInt32((uint64_t)instanceB->GetArchetype()->GetBoundPtr(), tag);
		u8 boundBuffer[sizeof(phBoundComposite)] ;
		sysDmaGet(boundBuffer, (uint64_t)boundB, sizeof(phBoundComposite), tag);
		sysDmaWaitTagStatusAll(1<<tag);
		boundB = (const phBound*)boundBuffer;
#else
		const phBound* boundB = instanceB->GetArchetype()->GetBound();
#endif

		transformB.Set( *(const Matrix34*)(&instanceB->GetMatrix()) );

		if (boundB->GetType()==phBound::COMPOSITE)
		{
			const phBoundComposite* compositeB = static_cast<const phBoundComposite*>(boundB);
			Matrix34 componentMatrix;
#if __SPU
			int component = GetComponentB();
			if (Verifyf(component>=0 && component < compositeB->GetNumBounds(), "Component %d out of range (0 to %d)", component, compositeB->GetNumBounds()))
			{
				sysDmaGet(&componentMatrix, uint64_t(&compositeB->GetCurrentMatrix(component)), sizeof(Matrix34), tag);
				sysDmaWaitTagStatusAll(1<<tag);
				transformB.DotFromLeft(componentMatrix);
			}
#else
			if (compositeB->GetCompositePart(GetComponentB(),RC_MAT34V(componentMatrix)))
			{
				transformB.DotFromLeft(componentMatrix);
			}
#endif
		}
	}
	else
	{
		transformB.Identity();
	}
}

void phManifold::GetLocalToWorldTransforms (Matrix34& transformA, Matrix34& transformB) const
{
	GetLocalToWorldTransformA(transformA);
	GetLocalToWorldTransformB(transformB);
}

Mat34V* g_LastMatricesBaseAddrMM = NULL;
u8* g_LevelIdxToLastMatrixMapMM = NULL;

#if __ASSERT
const float CONTACT_DEPTH_ASSERT_THRESHOLD = 25.0f;
#endif

void phManifold::RefreshContactPoints (int minManifoldPointLifetime, Mat34V_In transformA, Mat34V_In lastA, Mat34V_In transformB, Mat34V_In lastB, ScalarV_In timeStep)
{
#if !__SPU
	/// first refresh worldspace positions and distance
	for (int contactIndex=GetNumContacts()-1; contactIndex>=0; contactIndex--)
	{
		PrefetchContactPoint( contactIndex );
	}
#endif

	for (int contactIndex=GetNumContacts()-1; contactIndex>=0; contactIndex--)
	{
		phContact& manifoldPoint = GetContactPoint(contactIndex);

#if __ASSERT && !__SPU
		float previousDepth = manifoldPoint.GetDepth();
#endif // __ASSERT && !__SPU

		manifoldPoint.RefreshContactPoint(RCC_MATRIX34(transformA),RCC_MATRIX34(transformB),this);

#if __ASSERT && !__SPU // The same assert on the SPU is in collisiontask.cpp
		if (Unlikely(manifoldPoint.GetDepth() >= CONTACT_DEPTH_ASSERT_THRESHOLD && manifoldPoint.IsContactActive()))
		{
			phInst* instA = GetInstanceA();
			phInst* instB = GetInstanceB();
			if(GetColliderA() || GetColliderB())
			{
				phMaterialMgr::Id materialIdA = ( manifoldPoint.GetMaterialIdA() & 0xff );
				phMaterialMgr::Id materialIdB = ( manifoldPoint.GetMaterialIdB() & 0xff );

				// we don't want to assert if we're hitting a material that will cause the contact to be disabled.
				static const phMaterialMgr::Id temp_01 = MATERIALMGR.FindMaterialId("TEMP_01");
				static const phMaterialMgr::Id temp_02 = MATERIALMGR.FindMaterialId("TEMP_02");
				static const phMaterialMgr::Id temp_03 = MATERIALMGR.FindMaterialId("TEMP_03");

				if( materialIdA != temp_01 &&
					materialIdA != temp_02 &&
					materialIdA != temp_03 && 
					materialIdB != temp_01 &&
					materialIdB != temp_02 &&
					materialIdB != temp_03 )
				{
					phArchetype* archA = instA ? instA->GetArchetype() : NULL;
					phArchetype* archB = instB ? instB->GetArchetype() : NULL;
					const char* nameA = archA ? archA->GetFilename() : NULL;
					const char* nameB = archB ? archB->GetFilename() : NULL;
					Vec3V posA = instA ? instA->GetPosition() : Vec3V(V_ZERO);
					Vec3V posB = instB ? instB->GetPosition() : Vec3V(V_ZERO);
					Vec3V prevPosA = lastA.GetCol3();
					Vec3V prevPosB = lastB.GetCol3();

                    if( manifoldPoint.GetDepth() >= CONTACT_DEPTH_ASSERT_THRESHOLD )
                    {
					    Warningf(   "Contact depth %f exceeded allowed threshold %f"
						            "\n\tPrevious Depth: %f"
						            "\n\tObject A - '%s'"
						            "\n\t\tPosition - <%5.2f, %5.2f, %5.2f>"
						            "\n\t\tPrev Position - <%5.2f, %5.2f, %5.2f>"
						            "\n\tObject B - '%s'"
						            "\n\t\tPosition - <%5.2f, %5.2f, %5.2f>"
						            "\n\t\tPrev Position - <%5.2f, %5.2f, %5.2f>"
						            "\n\tInst A - '0x%p' User Data - '0x%p'"
						            "\n\tInst B - '0x%p' User Data - '0x%p'",
						            manifoldPoint.GetDepth(),
						            CONTACT_DEPTH_ASSERT_THRESHOLD,
						            previousDepth,
						            nameA,
						            VEC3V_ARGS(posA),
						            VEC3V_ARGS(prevPosA),
						            nameB,
						            VEC3V_ARGS(posB),
						            VEC3V_ARGS(prevPosB),
						            instA, 
						            instA ? instA->GetUserData() : NULL,
						            instB, 
						            instB ? instB->GetUserData() : NULL);
                    }
				}
			}
		}
#endif // __ASSERT && !__SPU

#if POSITIVE_DEPTH_ONLY
		if (sm_PositiveDepthMode == POSITIVE_DEPTH_ONLY_DISABLED)
		{
			manifoldPoint.SetPositiveDepth(true);
		}
		else if (!IsZeroAll(timeStep))
		{
			if (sm_PositiveDepthMode == POSITIVE_DEPTH_ONLY_VELOCITY)
			{
				Vec3V lastPosA = Transform(lastA, manifoldPoint.GetLocalPosA());
				Vec3V deltaPosA = manifoldPoint.GetWorldPosA() - lastPosA;
				Vec3V lastPosB = Transform(lastB, manifoldPoint.GetLocalPosB());
				Vec3V deltaPosB = manifoldPoint.GetWorldPosB() - lastPosB;
				Vec3V relativeDeltaPos = deltaPosB - deltaPosA;

				ScalarV normalDeltaPos = Dot(relativeDeltaPos,manifoldPoint.GetWorldNormal());

				manifoldPoint.SetPositiveDepth(IsTrue(manifoldPoint.GetDepthV() + normalDeltaPos >= ScalarV(POSITIVE_DEPTH_ONLY_THRESHOLD)));
			}
			else
			{
				manifoldPoint.SetPositiveDepth(manifoldPoint.GetDepth() >= POSITIVE_DEPTH_ONLY_THRESHOLD);
			}
		}
#endif // POSITIVE_DEPTH_ONLY
	}

	if (!IsConstraint())
	{
		ScalarV maxHorizontalDistSq(V_ZERO);
		int maxHorizontalPoint = -1;
#if REMOVE_SEPARATED_CONTACTS
		ScalarV maxVerticalDist(V_ZERO);
		int maxVerticalPoint = -1;
#endif // REMOVE_SEPARATED_CONTACTS
		for (int contactIndex=0; contactIndex<GetNumContacts(); contactIndex++)
		{
			// Get the manifold point (the contact) and see if it is a constraint.
			phContact& manifoldPoint = GetContactPoint(contactIndex);
			// JP: When instances get removed from the world their contacts can persist
			// We catch that case here and get rid of the contact
			if(!GetInstanceA() || !GetInstanceB())
			{
				RemoveContactPoint(contactIndex);
				contactIndex--;
				continue;
			}

			// Determine the contact points that are furthest apart in the horizontal and vertical directions.
			if (manifoldPoint.GetLifetime() > minManifoldPointLifetime)
			{
				// Contact also becomes invalid when relative movement orthogonal to normal exceeds margin, so let's track which one moved the most.
				const ScalarV depth = manifoldPoint.GetDepthV();

				Vec3V projectedPoint = manifoldPoint.GetWorldPosA();
				projectedPoint += manifoldPoint.GetWorldNormal() * depth;
				const Vec3V projectedDifference = manifoldPoint.GetWorldPosB() - projectedPoint;
				const ScalarV distance2d = MagSquared(projectedDifference);
				if (IsTrue(maxHorizontalDistSq < distance2d))
				{
					maxHorizontalDistSq = distance2d;
					maxHorizontalPoint = contactIndex;
				}

#if REMOVE_SEPARATED_CONTACTS
				// The contact is removed when the signed distance exceeds the margin (projected along the contact normal direction).
				if (IsTrue(depth < maxVerticalDist))
				{
					maxVerticalDist = depth;
					maxVerticalPoint = contactIndex;
				}
#endif // REMOVE_SEPARATED_CONTACTS
			}
		}

		// Remove only one contact point per call. First try to remove based on horizontal distance, then try based on vertical distance. 

		// If the contact point that moved the most laterally also exceeded the manifold margin, remove it.
		if (IsTrue(maxHorizontalDistSq > REMOVE_SEPARATED_CONTACTS_HORIZONTAL_DIST_SQ_V))
		{
			// There's no need to check that maxHorizontalPoint is != -1 if the threshold is greater than 0.
			FastAssert(REMOVE_SEPARATED_CONTACTS_HORIZONTAL_DIST > 0);
			FastAssert(maxHorizontalPoint != -1);
			RemoveContactPoint(maxHorizontalPoint);
		}
#if REMOVE_SEPARATED_CONTACTS
#	if USE_NEGATIVE_DEPTH_TUNABLE
		else if (g_UseRemoveContact && IsTrue(maxVerticalDist < ScalarV(-g_RemoveContactDistance)))
#	else // USE_NEGATIVE_DEPTH_TUNABLE
		else if (IsTrue(maxVerticalDist < -REMOVE_SEPARATED_CONTACTS_VERTICAL_DIST_V))
#	endif // USE_NEGATIVE_DEPTH_TUNABLE
		{
			// There's no need to check that maxVerticalPoint is != -1 if the threshold is greater than 0.
#	if USE_NEGATIVE_DEPTH_TUNABLE
			FastAssert(g_RemoveContactDistance > 0);
#	else // USE_NEGATIVE_DEPTH_TUNABLE
			FastAssert(REMOVE_SEPARATED_CONTACTS_VERTICAL_DIST > 0);
#	endif // USE_NEGATIVE_DEPTH_TUNABLE
			FastAssert(maxVerticalPoint != -1);
			RemoveContactPoint(maxVerticalPoint);
		}
#endif // REMOVE_SEPARATED_CONTACTS
	}
}

void phManifold::RefreshContactPoints (int minManifoldPointLifetime, ScalarV_In POSITIVE_DEPTH_ONLY_ONLY(timeStep))
{
	Matrix34 transformA;
	Matrix34 lastA;
	phInst* instanceA = GetInstanceA();
	if (instanceA)
	{
#if __SPU
		static const int tag = 1;
		u8 instBuffer[sizeof(phInst)] ;
		sysDmaGet(instBuffer, (uint64_t)instanceA, sizeof(phInst), tag);
		sysDmaWaitTagStatusAll(1<<tag);
		instanceA = (phInst*)instBuffer;

		const phBound* boundA = (const phBound*)sysDmaGetUInt32((uint64_t)instanceA->GetArchetype()->GetBoundPtr(), tag);
		u8 boundBuffer[sizeof(phBoundComposite)] ;
		sysDmaGet(boundBuffer, (uint64_t)boundA, sizeof(phBoundComposite), tag);
		boundA = (const phBound*)boundBuffer;
		sysDmaWaitTagStatusAll(1 << tag);
#else
		const phBound* boundA = instanceA->GetArchetype()->GetBound();
#endif

		transformA.Set(RCC_MATRIX34(instanceA->GetMatrix()));

		SPU_ONLY(Assert(u32(GetColliderA()) < 256 * 1024)); // collider should be an SPU pointer

		if(phCollider* colliderA = GetColliderA())
		{
			lastA = RCC_MATRIX34(colliderA->GetLastInstanceMatrix());
		}
		else
		{
			lastA = RCC_MATRIX34(instanceA->GetMatrix());
		}

		if (boundA->GetType()==phBound::COMPOSITE)
		{
			int componentA = GetComponentA();
			const phBoundComposite* compositeA = static_cast<const phBoundComposite*>(boundA);

			if (componentA < 0 || componentA >= compositeA->GetNumBounds())
			{
				RemoveAllContacts();
				return;
			}

#if __SPU
			phBound* partBoundA = (phBound*)sysDmaGetUInt32(uint64_t(compositeA->GetBoundArray() + componentA), tag);
#else
			phBound* partBoundA = compositeA->GetBound(componentA);
#endif

			//if (!Verifyf(partBoundA, "contact with non-existent component %d", componentA))
			if (!partBoundA)
			{
				RemoveAllContacts();
				return;
			}

			Matrix34 componentMatrix;
#if __SPU
			sysDmaGet(&componentMatrix, uint64_t(&compositeA->GetCurrentMatrix(GetComponentA())), sizeof(Matrix34), tag);
			sysDmaWaitTagStatusAll(1<<tag);
#else
			componentMatrix = RCC_MATRIX34(compositeA->GetCurrentMatrix(GetComponentA()));
#endif

			transformA.DotFromLeft(componentMatrix);

			Matrix34 lastMatrix;
#if __SPU
			sysDmaGet(&lastMatrix, uint64_t(&compositeA->GetLastMatrix(GetComponentA())), sizeof(Matrix34), tag);
			sysDmaWaitTagStatusAll(1<<tag);
#else
			lastMatrix = RCC_MATRIX34(compositeA->GetLastMatrix(GetComponentA()));
#endif
			lastA.DotFromLeft(lastMatrix);
		}
	}
	else
	{
		transformA.Identity();
		lastA.Identity();
	}

	Matrix34 transformB;
	Matrix34 lastB;
	phInst* instanceB = GetInstanceB();
	if (instanceB)
	{
#if __SPU
		static const int tag = 1;
		u8 instBuffer[sizeof(phInst)] ;
		sysDmaGet(instBuffer, (uint64_t)instanceB, sizeof(phInst), tag);
		sysDmaWaitTagStatusAll(1<<tag);
		instanceB = (phInst*)instBuffer;

		const phBound* boundB = (const phBound*)sysDmaGetUInt32((uint64_t)instanceB->GetArchetype()->GetBoundPtr(), tag);
		u8 boundBuffer[sizeof(phBoundComposite)] ;
		sysDmaGet(boundBuffer, (uint64_t)boundB, sizeof(phBoundComposite), tag);
		boundB = (const phBound*)boundBuffer;
		sysDmaWaitTagStatusAll(1 << tag);
#else
		const phBound* boundB = instanceB->GetArchetype()->GetBound();
#endif

		transformB.Set(RCC_MATRIX34(instanceB->GetMatrix()));

		SPU_ONLY(Assert(u32(GetColliderB()) < 256 * 1024)); // collider should be an SPU pointer

		if(phCollider* colliderB = GetColliderB())
		{
			lastB = RCC_MATRIX34(colliderB->GetLastInstanceMatrix());
		}
		else
		{
			lastB = RCC_MATRIX34(instanceB->GetMatrix());
		}

		if (boundB->GetType()==phBound::COMPOSITE)
		{
			int componentB = GetComponentB();
			const phBoundComposite* compositeB = static_cast<const phBoundComposite*>(boundB);

			if (componentB < 0 || componentB >= compositeB->GetNumBounds())
			{
				RemoveAllContacts();
				return;
			}

#if __SPU
			phBound* partBoundB = (phBound*)sysDmaGetUInt32(uint64_t(compositeB->GetBoundArray() + componentB), tag);
#else
			phBound* partBoundB = compositeB->GetBound(componentB);
#endif

			//if (!Verifyf(partBoundB, "contact with non-existent component %d", componentB))
			if (!partBoundB)
			{
				RemoveAllContacts();
				return;
			}

			Matrix34 componentMatrix;
#if __SPU
			sysDmaGet(&componentMatrix, uint64_t(&compositeB->GetCurrentMatrix(GetComponentB())), sizeof(Matrix34), tag);
			sysDmaWaitTagStatusAll(1<<tag);
#else
			componentMatrix = RCC_MATRIX34(compositeB->GetCurrentMatrix(GetComponentB()));
#endif

			transformB.DotFromLeft(componentMatrix);

			Matrix34 lastMatrix;
#if __SPU
			sysDmaGet(&lastMatrix, uint64_t(&compositeB->GetLastMatrix(GetComponentB())), sizeof(Matrix34), tag);
			sysDmaWaitTagStatusAll(1<<tag);
#else
			lastMatrix = RCC_MATRIX34(compositeB->GetLastMatrix(GetComponentB()));
#endif
			lastB.DotFromLeft(lastMatrix);
		}
	}
	else
	{
		transformB.Identity();
		lastB.Identity();
	}

	RefreshContactPoints(minManifoldPointLifetime, RCC_MAT34V(transformA), RCC_MAT34V(lastA), RCC_MAT34V(transformB), RCC_MAT34V(lastB), timeStep);
}

#if USE_PRECOMPUTE_SEPARATEBIAS
bool phManifold::HasOpposingContacts()
{
	BoolV anyOpposing(V_FALSE);
	ScalarV negInvSqrt2(-0.70710678118f);
	Vec3V normal1;

#define TEST_FOR_OPPOSITE_NORMALS(X) \
	anyOpposing |= IsLessThan(Dot(GetContactPoint(X).GetWorldNormal(), normal1), negInvSqrt2)

	CompileTimeAssert(MANIFOLD_CACHE_SIZE <= 6); // Have to fix this if we want to increase MANIFOLD_CACHE_SIZE
	switch (m_CachedPoints)
	{
	case 6:
		normal1 = GetContactPoint(5).GetWorldNormal();
		TEST_FOR_OPPOSITE_NORMALS(0);
		TEST_FOR_OPPOSITE_NORMALS(1);
		TEST_FOR_OPPOSITE_NORMALS(2);
		TEST_FOR_OPPOSITE_NORMALS(3);
		TEST_FOR_OPPOSITE_NORMALS(4);
		// fall thru
	case 5:
		normal1 = GetContactPoint(4).GetWorldNormal();
		TEST_FOR_OPPOSITE_NORMALS(0);
		TEST_FOR_OPPOSITE_NORMALS(1);
		TEST_FOR_OPPOSITE_NORMALS(2);
		TEST_FOR_OPPOSITE_NORMALS(3);
		// fall thru
	case 4:
		normal1 = GetContactPoint(3).GetWorldNormal();
		TEST_FOR_OPPOSITE_NORMALS(0);
		TEST_FOR_OPPOSITE_NORMALS(1);
		TEST_FOR_OPPOSITE_NORMALS(2);
		// fall thru
	case 3:
		normal1 = GetContactPoint(2).GetWorldNormal();
		TEST_FOR_OPPOSITE_NORMALS(0);
		TEST_FOR_OPPOSITE_NORMALS(1);
		// fall thru
	case 2:
		normal1 = GetContactPoint(1).GetWorldNormal();
		TEST_FOR_OPPOSITE_NORMALS(0);
	}

	return IsTrue(anyOpposing);
}

void phManifold::UpdateSeparateBias(ScalarV_In separateBiasMultiplier, ScalarV_In halfPenetration, ScalarV_In invTime)
{
	if (HasOpposingContacts())
	{
		for(int i=0;i<m_CachedPoints;i++) 
		{
			GetContactPoint(i).DisableSeparateBias();
		}

		return;
	}

#if __SPU
	phContact ** contacts = m_ContactsLs;
#else // __SPU
	phContact ** contacts = m_Contacts;
#endif // __SPU

/*
	for (int c = 0 ; c < m_CachedPoints ; c++)
		contacts[c]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
*/
/*
	FastAssert(m_CachedPoints >= 0 && m_CachedPoints <= 6);
	switch(m_CachedPoints)
	{
	case 6:
		contacts[5]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	case 5:
		contacts[4]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	case 4:
		contacts[3]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	case 3:
		contacts[2]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	case 2:
		contacts[1]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	case 1:
		contacts[0]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	}
*/
	int c = m_CachedPoints;
	while (c >= 4)
	{
		c--;
		contacts[c]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
		c--;
		contacts[c]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
		c--;
		contacts[c]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
		c--;
		contacts[c]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	}
	FastAssert(c >= 0 && c < 4);
	if (c >= 2)
	{
		c--;
		contacts[c]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
		c--;
		contacts[c]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	}
	FastAssert(c >= 0 && c < 2);
	if (c == 1)
	{
		contacts[0]->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTime);
	}
}
#endif // USE_PRECOMPUTE_SEPARATEBIAS

void phManifold::Exchange()
{
	ExchangeThisManifoldOnly();
	if (CompositeManifoldsEnabled())
	{
		const int numCompositeManifolds = GetNumCompositeManifolds();

		// Do this in a separate loop because it touches different memory.
	#if !__SPU
		phCompositePointers *compositePointers = m_CompositePointers;
	#else
		phCompositePointers *compositePointers = m_CompositePointersLs;
	#endif
		compositePointers->SwapComponentPairs(numCompositeManifolds);

		for(int i = 0; i < numCompositeManifolds; ++i)
		{
			phManifold* compManifold = GetCompositeManifold(i);
			compManifold->ExchangeThisManifoldOnly();
		}
	}
}

void phManifold::ExchangeThisManifoldOnly()
{
	for(int i=0;i<m_CachedPoints;i++) 
	{
		GetContactPoint(i).Exchange();
	}

	SwapEm(m_Instance[0],     m_Instance[1]);
	SwapEm(m_Collider[0],     m_Collider[1]);
	SwapEm(m_Bound[0],        m_Bound[1]);
	SwapEm(m_LevelIndex[0],   m_LevelIndex[1]);
	SwapEm(m_GenerationId[0],   m_GenerationId[1]);
	SwapEm(m_Component[0],    m_Component[1]);
	SwapEm(m_MassInvScale[0], m_MassInvScale[1]);
	SwapEm(m_MassInv[0],    m_MassInv[1]);
	SwapEm(m_InertiaInv[0], m_InertiaInv[1]);
	bool temp = m_InactiveCollidesAgainstInactiveA;
	m_InactiveCollidesAgainstInactiveB = m_InactiveCollidesAgainstInactiveA;
	m_InactiveCollidesAgainstInactiveA = temp;
}

void phManifold::ResetWarmStart()
{
	if(CompositeManifoldsEnabled())
	{
		for(int compositeManifoldIndex = 0; compositeManifoldIndex < GetNumCompositeManifolds(); ++compositeManifoldIndex)
		{
			phManifold* pCompositeManifold = GetCompositeManifold(compositeManifoldIndex);
			for(int contactIndex = 0; contactIndex < pCompositeManifold->GetNumContacts(); ++contactIndex)
			{
				pCompositeManifold->GetContactPoint(contactIndex).SetPreviousSolution(Vec3V(V_ZERO));
			}
		}
	}
	else
	{
		for(int contactIndex = 0; contactIndex < GetNumContacts(); ++contactIndex)
		{
			GetContactPoint(contactIndex).SetPreviousSolution(Vec3V(V_ZERO));
		}
	}
}

#if !__SPU
const char* GetBestName(phInst* inst, char* string, size_t size)
{
	if (inst)
	{
		if (phArchetype* archetype = inst->GetArchetype())
		{
			if (archetype->GetFilename())
			{
				strncpy(string, archetype->GetFilename(), size);
			}
			else
			{
				strncpy(string, "(NULL filename)", size);
				string[size - 1] = '\0';
			}
		}
		else
		{
			strncpy(string, "(NULL archetype)", size);
			string[size - 1] = '\0';
		}
	}
	else
	{
		strcpy(string, "(NULL inst), size");
		string[size - 1] = '\0';
	}

	return string;
}

void phManifold::DumpDueToRunningOut()
{
	int iLIA = GetLevelIndexA();
	int iLIB = GetLevelIndexB();

	phInst* pInstA = NULL;
	Vec3V posA(V_ZERO);
	if (PHLEVEL->LegitLevelIndex(iLIA) && !PHLEVEL->IsNonexistent(iLIA))
	{
		pInstA = PHLEVEL->GetInstance(iLIA);
		posA = pInstA->GetPosition();
	}

	phInst* pInstB = NULL;
	Vec3V posB(V_ZERO);
	if (PHLEVEL->LegitLevelIndex(iLIB) && !PHLEVEL->IsNonexistent(iLIB))
	{
		pInstB = PHLEVEL->GetInstance(iLIB);
		posB = pInstB->GetPosition();
	}

	char stringA[256];
	GetBestName(pInstA, stringA, sizeof(stringA));

	char stringB[256];
	GetBestName(pInstB, stringB, sizeof(stringB));

	Displayf("[%p] %p/%s/%u <<%.2f,%.2f,%.2f>> -> %p/%s/%u <<%.2f,%.2f,%.2f>>", 
		this,
		pInstA, GetBestName(pInstA, stringA, sizeof(stringA)),
		GetComponentA(),
		posA.GetXf(), posA.GetYf(), posA.GetZf(),
		pInstB, GetBestName(pInstB, stringB, sizeof(stringB)),
		GetComponentB(),
		posB.GetXf(), posB.GetYf(), posB.GetZf()
		);
}
#endif

#if __PS3
void phManifold::GenerateDmaPlan(DMA_PLAN_ARGS(phManifold))
{
	DMA_BEGIN();

	for (int i = 0; i < m_CachedPoints; ++i)
	{
		ADD_DMA_OWNER(m_Contacts[i]);
	}

	ADD_DMA_OWNER(m_ConstraintMatrix);

#if !__SPU
	// When we do this on the SPU (in the update contacts task), the composite manifolds come out the same way,
	// but they're processed within UpdateContactsTask instead of here (see BUILD_DMA_PLAN).
	if (CompositeManifoldsEnabled())
	{
		int numComposite = GetNumCompositeManifolds();

		if (numComposite > 0)
		{
			// Composite manifolds have composite pointers
			ADD_DMA_OWNER_READ_ONLY(m_CompositePointers);

			// They also have their entire first composite manifold contained in their DMA plan
			m_CompositePointers->GetManifold(0)->GenerateDmaPlan(dmaPlan);

			// Add the fixup since the previous line added the object only
			ADD_DMA_REF(m_CompositePointers->GetManifoldRef(0));

			if (numComposite > 1)
			{
				// If a second manifold exists, includes its DMA plan in the DMA plan for the parent manifold
				phManifold* secondManifold = m_CompositePointers->GetManifold(1);
				phManifold::DmaPlan* secondManifoldDmaPlan = PHSIM->GetDmaPlan(secondManifold);
				m_CompositePointers->SetSecondManifoldDmaPlan(secondManifoldDmaPlan);
				ADD_DMA_OWNER_READ_ONLY((phManifold::DmaPlan*&)m_CompositePointers->GetSecondManifoldDmaPlanRef());
			}
		}
	}
#endif
}

#if __PPU
void phManifold::RegenerateDmaPlan()
{
	phManifold::DmaPlan* dmaPlan = PHSIM->GetDmaPlan(this);
	dmaPlan->Initialize();
	GenerateDmaPlan(*dmaPlan);
}
#endif // __PPU
#endif // !__PS3

EXT_PFD_DECLARE_ITEM(ManifoldsA);
EXT_PFD_DECLARE_ITEM(ManifoldsB);
EXT_PFD_DECLARE_ITEM(ManifoldsARefreshed);
EXT_PFD_DECLARE_ITEM(ManifoldsBRefreshed);
EXT_PFD_DECLARE_ITEM(ManifoldInactiveContacts);
EXT_PFD_DECLARE_ITEM(ManifoldHUD);
EXT_PFD_DECLARE_ITEM(ManifoldNewest);
EXT_PFD_DECLARE_ITEM(ManifoldNormals);
EXT_PFD_DECLARE_ITEM(ManifoldDepths);
EXT_PFD_DECLARE_ITEM(ManifoldImpulses);
EXT_PFD_DECLARE_ITEM(ManifoldPushes);
EXT_PFD_DECLARE_ITEM_SLIDER(ManifoldDrawLength);

void phManifold::ProfileDraw()
{
#if __PFDRAW
	if (CompositeManifoldsEnabled())
	{
		for (int manifoldIndex = 0; manifoldIndex < m_NumCompositeManifolds; ++manifoldIndex)
		{
			if(GetCompositeManifold(manifoldIndex))
			{
				GetCompositeManifold(manifoldIndex)->ProfileDraw();
			}
		}
	}
	else
	{
		float drawLength = PFD_ManifoldDrawLength.GetValue();

		int newest = GetNewestContactPoint();
		if (newest >= 0 && newest < m_CachedPoints)
		{
			PFD_ManifoldNewest.DrawTick(VEC3V_TO_VECTOR3(GetContactPoint(newest).GetWorldPosA()), drawLength);
		}

		int contactIndex;
		for( contactIndex = 0; contactIndex < GetNumContacts(); contactIndex++ )
		{
			const phContact& contact = GetContactPoint(contactIndex);
			if(contact.IsContactActive() || PFD_ManifoldInactiveContacts.GetEnabled())
			{
				PFD_ManifoldsA.DrawTick(VEC3V_TO_VECTOR3(contact.GetWorldPosA()), drawLength * 0.5f,contact.IsContactActive() ? PFD_ManifoldsA.GetBaseColor() : Color_DimGray);
				PFD_ManifoldsB.DrawTick(VEC3V_TO_VECTOR3(contact.GetWorldPosB()), drawLength * 0.5f,contact.IsContactActive() ? PFD_ManifoldsB.GetBaseColor() : Color_DimGray);

				if (PFD_ManifoldsARefreshed.GetEnabled() || PFD_ManifoldsBRefreshed.GetEnabled())
				{
					Matrix34 transformA,transformB;
					GetLocalToWorldTransforms(transformA,transformB);

					Vector3 refreshedPosA, refreshedPosB;
					transformA.Transform(VEC3V_TO_VECTOR3(contact.GetLocalPosA()),refreshedPosA);
					transformB.Transform(VEC3V_TO_VECTOR3(contact.GetLocalPosB()),refreshedPosB);

					PFD_ManifoldsARefreshed.DrawArrow(VEC3V_TO_VECTOR3(contact.GetWorldPosA()), refreshedPosA);
					PFD_ManifoldsBRefreshed.DrawArrow(VEC3V_TO_VECTOR3(contact.GetWorldPosB()), refreshedPosB);
				}

				if (PFD_ManifoldsA.GetEnabled())
				{
					if (PFD_ManifoldHUD.Begin())
					{
						grcColor(Color32(255, 255, 255));

						char num[128];
						sprintf( num, "%d C: %d E: %d L: %d", contactIndex, m_Component[0], contact.GetElementA(), m_LevelIndex[0] );
						grcDrawLabelf(VEC3V_TO_VECTOR3(contact.GetWorldPosA()),num);

						PFD_ManifoldHUD.End();
					}
				}

				if (PFD_ManifoldsB.GetEnabled())
				{
					if (PFD_ManifoldHUD.Begin())
					{
						grcColor(Color32(255, 255, 255));

						char num[128];
						sprintf( num, "%d C: %d E: %d L: %d", contactIndex, m_Component[1], contact.GetElementB(), m_LevelIndex[1] );
						grcDrawLabelf(VEC3V_TO_VECTOR3(contact.GetWorldPosB()),num);

						PFD_ManifoldHUD.End();
					}

					// Both enabled, draw a line in between
#if POSITIVE_DEPTH_ONLY
					Color32 color = contact.IsPositiveDepth() ? Color_OrangeRed : Color_AliceBlue;
#else
					Color32 color = Color_OrangeRed;
#endif

					PFD_ManifoldsA.DrawLine(VEC3V_TO_VECTOR3(contact.GetWorldPosA()), VEC3V_TO_VECTOR3(contact.GetWorldPosB()), color);
				}

				if (PFD_ManifoldNormals.GetEnabled() || PFD_ManifoldDepths.GetEnabled())
				{
					//phContact &c = GetContactPoint( iImpact );
					//Displayf("colliding elementA: %d, componentA: %d, elementB: %d, componentB: %d", c.GetElementA(), c.GetComponentA(), c.GetElementB(), c.GetComponentB() );

					Vector3 start,end;
					Vector3 na = VEC3V_TO_VECTOR3( contact.GetWorldNormal());
					Vector3 nb = VEC3V_TO_VECTOR3(-contact.GetWorldNormal());
					Vector3 pa = VEC3V_TO_VECTOR3( contact.GetWorldPosA());
					Vector3 pb = VEC3V_TO_VECTOR3( contact.GetWorldPosB());

					if (PFD_ManifoldsA.GetEnabled())
					{
						start = pa;
						end.AddScaled( pa, na, drawLength );
						PFD_ManifoldNormals.DrawArrow( start, end, Color32(0.76f, 0.91f, 1.00f) );

						end.AddScaled( start, na, contact.GetDepth() );
						PFD_ManifoldDepths.DrawLine( start, end, Color32(0.44f, 0.0f, 0.44f ) );
					}

					if (PFD_ManifoldsB.GetEnabled())
					{
						start = pb;
						end.AddScaled( pb, nb, drawLength );
						PFD_ManifoldNormals.DrawArrow( start, end, Color32(0.80f, 0.99f, 0.80f) );

						end.AddScaled( start, nb, contact.GetDepth() );
						PFD_ManifoldDepths.DrawLine( start, end, Color32(0.44f, 0.0f, 0.44f ) );
					}
				}

				if (PFD_ManifoldImpulses.GetEnabled())
				{
					phCollider* colliderA = GetColliderA();
					phCollider* colliderB = GetColliderB();

					if(IsArticulatedFixedCollision())
					{
						Vec3V worldImpulse = contact.ComputeTotalArtFixImpulse();
						Vec3V start = colliderA ? contact.GetWorldPosA() : contact.GetWorldPosB();
						Vec3V end   = start + worldImpulse;
						PFD_ManifoldImpulses.DrawArrow( RCC_VECTOR3(start), RCC_VECTOR3(end) );
					}
					else
					{
						if (colliderA)
						{
							Vec3V start = contact.GetWorldPosA();
							Vec3V end = AddScaled(start,contact.ComputeTotalImpulse(),colliderA->GetInvMassV());
							PFD_ManifoldImpulses.DrawArrow( RCC_VECTOR3(start), RCC_VECTOR3(end) );
						}

						if (colliderB)
						{
							Vec3V start = contact.GetWorldPosB();
							Vec3V end = AddScaled(start,contact.ComputeTotalImpulse(),colliderB->GetInvMassV());
							PFD_ManifoldImpulses.DrawArrow( RCC_VECTOR3(start), RCC_VECTOR3(end) );
						}
					}
				}
				else
				{
					if (phCollider* colliderA = GetColliderA())
					{
						Vector3 start = VEC3V_TO_VECTOR3(contact.GetWorldPosA());
						Vector3 end = start;
						end.AddScaled(VEC3V_TO_VECTOR3(contact.GetImpulseA()), colliderA->GetInvMass());
						PFD_ManifoldImpulses.DrawArrow( start, end );
					}

					if (phCollider* colliderB = GetColliderB())
					{
						Vector3 start = VEC3V_TO_VECTOR3(contact.GetWorldPosB());
						Vector3 end = start;
						end.AddScaled(VEC3V_TO_VECTOR3(contact.GetImpulseA()), -colliderB->GetInvMass());
						PFD_ManifoldImpulses.DrawArrow( start, end );
					}
				}

				if (!IsRotationConstraint())
				{
					Matrix33 contactLocal;
					contactLocal.a.Set(VEC3V_TO_VECTOR3(contact.GetWorldNormal()));
					contactLocal.a.MakeOrthonormals(contactLocal.b,contactLocal.c);
					Vector3 push;
					contactLocal.Transform(VEC3V_TO_VECTOR3(contact.GetPreviousPush()),push);
					push.Scale(TIME.GetSeconds());

					if (PFD_ManifoldPushes.GetEnabled())
					{
						if (GetColliderA())
						{
							Vector3 start = VEC3V_TO_VECTOR3(contact.GetWorldPosA());
							Vector3 end = start;
							end.Add(push);
							PFD_ManifoldPushes.DrawArrow( start, end );
						}

						if (GetColliderB())
						{
							Vector3 start = VEC3V_TO_VECTOR3(contact.GetWorldPosB());
							Vector3 end = start;
							end.Subtract(push);
							PFD_ManifoldPushes.DrawArrow( start, end );
						}
					}
				}
			}
		}
	}
#endif // __PFDRAW
}

#if __DEBUGLOG
void phManifold::DebugReplay() const
{
#if __DEBUGLOG_PPU
	diagDebugLogSPUBuffer(diagDebugLogPhysics, m_DebugLogBuffer, m_DebugLogSize);
	m_DebugLogSize = 0;
#endif

	diagDebugLog(diagDebugLogPhysics, 'pMCP', &m_CachedPoints);
	diagDebugLog(diagDebugLogPhysics, 'pMNP', &m_NewestPoint);
	diagDebugLog(diagDebugLogPhysics, 'pMSF', &m_StaticFriction);
	diagDebugLog(diagDebugLogPhysics, 'pMIA', &levelIndexA);
	diagDebugLog(diagDebugLogPhysics, 'pMIB', &levelIndexB);
	diagDebugLog(diagDebugLogPhysics, 'pMNC', &m_NumCompositeManifolds);
	diagDebugLog(diagDebugLogPhysics, 'pMCT', &m_ConstraintType);


	if (colliderA)
	{
		colliderA->DebugReplay();
	}
	else if (instanceA)
	{
		instanceA->DebugReplay();
	}

	if (colliderB)
	{
		colliderB->DebugReplay();
	}
	else if (instanceB)
	{
		instanceB->DebugReplay();
	}

	for (int i = 0; i < m_NumCompositeManifolds; i++)
	{
		if (phManifold* manifold = GetCompositeManifold(i))
		{
			manifold[i]->DebugReplay();
		}
	}
}
#endif

#define SPU_ASSERT_CALLBACK 1
#if __ASSERT
// TODO -- Actually forward the original callstack from SPU on through this
void dummyStackTraceFunc() {}

void phManifold::AssertContactNormalVerbose(const phContact& Contact, const char* file, int line, bool noStack) const
{
#if !__SPU
	const int bufSize = 256;
	char boundInfoBufInstA[bufSize+1] = {0};
	char boundInfoBufInstB[bufSize+1] = {0};
	if( PHLEVEL->IsLevelIndexGenerationIDCurrent(GetLevelIndexA(), GetGenerationIdA()) && PHLEVEL->GetInstance(GetLevelIndexA()) && 
		PHLEVEL->IsLevelIndexGenerationIDCurrent(GetLevelIndexB(), GetGenerationIdB()) && PHLEVEL->GetInstance(GetLevelIndexB()) )
	{
		const phInst* instA = PHLEVEL->GetInstance(GetLevelIndexA());
		const phInst* instB = PHLEVEL->GetInstance(GetLevelIndexB());
		const phArchetype* archA = instA->GetArchetype();
		const phArchetype* archB = instB->GetArchetype();

		if(archA)
		{
			phBound::FormatBoundInfo(boundInfoBufInstA, bufSize, archA->GetBound(), GetComponentA(), Contact.GetElementA(), instA->GetMatrix());
		}

		if(archB)
		{
			phBound::FormatBoundInfo(boundInfoBufInstB, bufSize, archB->GetBound(), GetComponentB(), Contact.GetElementB(), instB->GetMatrix());
		}

		// The stack from here is meaningless for SPU calls since we're just a proxy on the PPU by now
		// So we're disabling the stack output by installing a no-op stack trace function temporarily
		void(*oldPrintFn)() = diagPrintStackTrace;
		if(noStack)
		{
			diagPrintStackTrace = dummyStackTraceFunc;
		}
		// Actual assert message
		rage::diagAssertHelper(file, line, "[%d]ContactNormNotNormal(Mag=%.4f)<%.3f,%.3f,%.3f> entA(%s:0x%p), entB(%s:0x%p):BndA=%s ### BndB=%s", 
				line, Mag(Contact.GetWorldNormal()).Getf(), Contact.GetWorldNormal().GetXf(), Contact.GetWorldNormal().GetYf(), Contact.GetWorldNormal().GetZf(), 
				archA!=NULL?archA->GetFilename():"NULL", instA, archB!=NULL?archB->GetFilename():"NULL", instB, boundInfoBufInstA, boundInfoBufInstB);
		// Put the original stack trace function back
		diagPrintStackTrace = oldPrintFn;
	}
	else
	{
		// Assert that our assertion failed?
		rage::diagAssertHelper(file, line, "Trying to assert about a crappy normal but it's from a contact being added to a manifold involving an inst that no longer exists anyway!");
	}
#elif SPU_ASSERT_CALLBACK
		int filenameLength = strlen(file);

		const phContact* contactLS = &Contact;
		const uint32_t data[5] = {(uint32_t)this, (uint32_t)contactLS, (uint32_t)file, (uint32_t)filenameLength, (uint32_t)line};
		si_dsync();
		sys_spu_thread_send_event(SYSTASKMANAGER_EVENT_PORT, EVENTCMD_ASSERT_CONTACT_NORM, (uint32_t)data);
		spu_readch(SPU_RdInMbox);
#endif
}

#if __PPU
void phManifold::SpuAssertNormalEventHandler(const sys_event* Event)
{
	const sys_spu_thread_t spuThread = Event->data1;
	const uint32_t regsLs = (uint32_t)Event->data3;

	uint64_t tmp;

	// Load addresses
	sys_spu_thread_read_ls(spuThread, regsLs+0, &tmp, 4);
	const uint32_t manifoldLs = (uint32_t)tmp;

	sys_spu_thread_read_ls(spuThread, regsLs+4, &tmp, 4);
	const uint32_t contactLs = (uint32_t)tmp;

	sys_spu_thread_read_ls(spuThread, regsLs+8, &tmp, 4);
	const uint32_t filenameLs = (uint32_t)tmp;

	sys_spu_thread_read_ls(spuThread, regsLs+12, &tmp, 4);
	const int filenameLength = (int)tmp;

	sys_spu_thread_read_ls(spuThread, regsLs+16, &tmp, 4);
	const int lineNumber = (int)tmp;

	// Load data
	phManifold tempManifold;
	int retValueManifold = SpuThreadReadLs(&tempManifold, spuThread, manifoldLs, sizeof(phManifold));
	phContact tempContact;
	int retValueContact = SpuThreadReadLs(&tempContact, spuThread, contactLs, sizeof(phContact));

	char* filename = Alloca(char, filenameLength+1);
	filename[filenameLength] = 0;
	int retValueFilename = SpuThreadReadLs(filename, spuThread, filenameLs, filenameLength);

	// Assuming everything loaded ok, we call the formatting and asserting function as normal
	if(retValueManifold == CELL_OK && retValueContact == CELL_OK && retValueFilename == CELL_OK)
	{
		tempManifold.AssertContactNormalVerbose(tempContact, filename, lineNumber, true);
	}
	// HACK to avoid errors when our fake manifold's destructor runs
	tempManifold.m_NumCompositeManifolds = 0;
	tempManifold.m_CachedPoints = 0;

	// Release the SPU
	sys_spu_thread_write_spu_mb(spuThread, 0);
}
#endif // __PPU
#endif // __ASSERT

#if !__SPU
void phManifold::RefreshColliderPointers()
{
	FastAssert(PHLEVEL->IsLevelIndexGenerationIDCurrent(GetLevelIndexA(),GetGenerationIdA()));
	FastAssert(PHLEVEL->IsLevelIndexGenerationIDCurrent(GetLevelIndexB(),GetGenerationIdB()));
	phCollider* pColliderA = PHSIM->GetCollider(GetLevelIndexA());
	phCollider* pColliderB = PHSIM->GetCollider(GetLevelIndexB());
	SetColliderA(pColliderA);
	SetColliderB(pColliderB);
	if(CompositeManifoldsEnabled())
	{
		for(int compositeManifoldIndex = 0; compositeManifoldIndex < GetNumCompositeManifolds(); ++compositeManifoldIndex)
		{
			phManifold* pCompositeManifold = GetCompositeManifold(compositeManifoldIndex);
			pCompositeManifold->SetColliderA(pColliderA);
			pCompositeManifold->SetColliderB(pColliderB);
		}
	}
}
#endif // !__SPU

#if CLOTH_MANIFOLD
phClothManifold::phClothManifold ()
{
	SetNumWrappedObjects(0);
}
#endif


} // namespace rage
