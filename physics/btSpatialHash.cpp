
#include "btSpatialHash.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include"vector/vector3.h"
#include "vector/geometry.h"
#include "system/miniheap.h"

// for the scratchpad.... should be broken out into it's own class with access protocols and junk.
#include "phsolver/contactmgr.h"
#include "physics/simulator.h"

namespace rage
{

void btSpatialHash::pruneActiveOverlappingPairs()
{

	Assert( sizeof( AabbHandle ) == sizeof( CHashBroadPhase16::AABBEntity ) );

	sysMemStartMiniHeap(PHSIM->GetContactMgr()->GetScratchpad(), PHSIM->GetContactMgr()->GetScratchpadSize());

	btBroadphasePair *newPairs = rage_new btBroadphasePair[ m_pairCache->GetMaxPairs() ];
	CHashBroadPhase16::AABBEntity *sortedAabb = rage_new CHashBroadPhase16::AABBEntity[ m_numHandles ];
	CHashBroadPhase16::CCollisionPair *newPairsFromHash = rage_new CHashBroadPhase16::CCollisionPair[ m_pairCache->GetMaxPairs() ];

//!me	AabbHandle *segregatedAabbs = Alloca( AabbHandle, m_numHandles );

	sysMemEndMiniHeap();
		
	m_bp.clear();

	sysMemCpy( sortedAabb, m_aabbs, sizeof( AabbHandle )*m_numHandles );
	
	m_bp.sortByWidth( sortedAabb, m_numHandles );

	int nPairs = m_bp.addBatchSorted( sortedAabb, m_numHandles, newPairsFromHash, m_pairCache->GetMaxPairs() );

//!me more efficient usage.  Assumes all fixed objects are bigger than moving ones, I think
/*
	int nActiveHandle = 0;
	int iHandle;

	int iActive = 0;
	//int iInactive = nActiveHandle;
	for( iHandle = 0; iHandle < m_numHandles; iHandle++ )
	{
		if( !PHLEVEL->IsFixed( m_aabbs[iHandle].m_handleID ) )
		{
			segregatedAabbs[iActive] = m_aabbs[iHandle];
			iActive++;
			nActiveHandle++;
		}
		else
		{
			m_bp.insert( m_bp.getHashEntry( (const CHashBroadPhase16::AABBEntity *)(m_aabbs + iHandle) ) );
			//segregatedAabbs[iInactive] = m_aabbs[iHandle];
			//iInactive++;
		}

	}

	m_bp.sortByWidth((CHashBroadPhase16::AABBEntity *)(segregatedAabbs), nActiveHandle );

	int nPairs = 0;
	int nMaxPairs = m_pairCache->GetMaxPairs();
	int rez = m_bp.m_hashLevels-1;
	for( iActive = 0; iActive < nActiveHandle; iActive++ )
	{
		const CHashBroadPhase16::AABBEntity *aabb = (const CHashBroadPhase16::AABBEntity *)(segregatedAabbs + iActive);
		rez = m_bp.calcResolutionIncrease( aabb->m_aabb, rez );

		nPairs += m_bp.add( m_bp.getHashEntry(aabb), rez, newPairsFromHash + nPairs, nMaxPairs-nPairs );
	}
*/

	int iTransfer;
	for( iTransfer = 0; iTransfer < nPairs; iTransfer++ )
	{
 		u16 iPea = newPairsFromHash[iTransfer].m_a->m_value;
 		u16 iPeb = newPairsFromHash[iTransfer].m_b->m_value;
		u16 generationIdA = PHLEVEL->GetGenerationID(iPea);
		u16 generationIdB = PHLEVEL->GetGenerationID(iPeb);
 		newPairs[iTransfer].Set(iPea, generationIdA, iPeb, generationIdB);
		newPairs[iTransfer].SetManifold(NULL);
	}

	mergeNewPairs( newPairs, nPairs );

}

}

#endif // ENABLE_UNUSED_PHYSICS_CODE
