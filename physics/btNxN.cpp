
#include "btNxN.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include"vector/vector3.h"
#include "vector/geometry.h"
#include "system/miniheap.h"
// for the scratchpad.... should be broken out into it's own class with access protocols and junk.
#include "phsolver/contactmgr.h"
#include "physics/simulator.h"


namespace rage
{

	void btNxN::pruneActiveOverlappingPairs()
	{
		
		sysMemStartMiniHeap(PHSIM->GetContactMgr()->GetScratchpad(), PHSIM->GetContactMgr()->GetScratchpadSize());

		btBroadphasePair *newPairs = rage_new btBroadphasePair[ m_pairCache->GetMaxPairs() ];

		sysMemEndMiniHeap();

		int nPairs = 0;

		int iN;
		for( iN = 0; iN < m_numHandles-1; iN++ )
		{
 			u16 iPea = m_aabbs[iN].m_handleID;
			Vector3 minA = m_aabbs[iN].m_minExtent;
			Vector3 maxA = m_aabbs[iN].m_maxExtent;

			int iNxN;
			for( iNxN = iN+1; iNxN < m_numHandles; iNxN++ )
			{

	//			if( iN != iNxN )
				{
 					u16 iPeb = m_aabbs[iNxN].m_handleID;
					Vector3 minB = m_aabbs[iNxN].m_minExtent;
					Vector3 maxB = m_aabbs[iNxN].m_maxExtent;

					if( geomBoxes::TestAABBtoAABB( minA, maxA, minB, maxB ) )
					{
						u16 generationIdA = PHLEVEL->GetGenerationID(iPea);
						u16 generationIdB = PHLEVEL->GetGenerationID(iPeb);
 						newPairs[nPairs].Set(iPea, generationIdA, iPeb, generationIdB);
						newPairs[nPairs].SetManifold(NULL);

						nPairs++;
					}
				}
			}
		}

		mergeNewPairs( newPairs, nPairs );

	}

}

#endif // ENABLE_UNUSED_PHYSICS_CODE
