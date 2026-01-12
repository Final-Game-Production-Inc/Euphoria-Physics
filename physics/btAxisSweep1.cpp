
#include "btAxisSweep1.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include"vector/vector3.h"
#include "vector/geometry.h"
#include <algorithm>
#include "system/miniheap.h"

// for the scratchpad.... should be broken out into it's own class with access protocols and junk.
#include "phsolver/contactmgr.h"
#include "physics/simulator.h"

namespace rage
{


class ProjectedExtent
{
public:

	bool operator< ( const ProjectedExtent &b ) const
	{
		return m_pos.IsLessThanDoNotUse( b.m_pos );
	}

	Vector3 m_pos;
	u16 m_handleID;
	bool m_isMax;
};


void btAxisSweep1::pruneActiveOverlappingPairs()
{
	sysMemStartMiniHeap(PHSIM->GetContactMgr()->GetScratchpad(), PHSIM->GetContactMgr()->GetScratchpadSize());

	btBroadphasePair *newPairs = rage_new btBroadphasePair[ m_pairCache->GetMaxPairs() ];
	ProjectedExtent *extents = rage_new ProjectedExtent[ m_numHandles*2 ];

	sysMemEndMiniHeap();

	int nPairs = 0;

	int iExt;
	const int nExt = m_numHandles;
	for( iExt = 0; iExt < nExt; iExt++ )
	{
		extents[iExt*2].m_handleID = m_aabbs[iExt].m_handleID;
		extents[iExt*2+1].m_handleID = m_aabbs[iExt].m_handleID;
		extents[iExt*2].m_pos.SplatX(m_aabbs[iExt].m_minExtent);
		extents[iExt*2].m_isMax = false;
		extents[iExt*2+1].m_pos.SplatX(m_aabbs[iExt].m_maxExtent);
		extents[iExt*2+1].m_isMax = true;
	}

	std::sort( extents, extents+m_numHandles*2 );

	for( iExt = 0; iExt < nExt*2; iExt++ )
	{
		ProjectedExtent *peA = extents + iExt;
		while( iExt < nExt*2 && peA->m_isMax )
		{
			peA++;
			iExt++;
		}

		if( !( iExt < nExt*2) )
			break;

		ProjectedExtent *peB = extents + iExt + 1;
		const u16 iPea = peA->m_handleID;
		const u16 iPeaHandle = m_handleToAabbTable[iPea];
		Vector3 minA = m_aabbs[iPeaHandle].m_minExtent;
		Vector3 maxA = m_aabbs[iPeaHandle].m_maxExtent;
		while( iPea != peB->m_handleID )
		{
			// do a full AABB check
			if( peB->m_isMax )
			{
				const u16 iPeb = peB->m_handleID;
				const u16 iPebHandle = m_handleToAabbTable[iPeb];
				Vector3 minB = m_aabbs[iPebHandle].m_minExtent;
				Vector3 maxB = m_aabbs[iPebHandle].m_maxExtent;

				if( geomBoxes::TestAABBtoAABB( minA, maxA, minB, maxB ) )
				{
					u16 generationIdA = PHLEVEL->GetGenerationID(iPea);
					u16 generationIdB = PHLEVEL->GetGenerationID(iPeb);
 					newPairs[nPairs].Set(iPea, generationIdA, iPeb, generationIdB);
					newPairs[nPairs].SetManifold(NULL);

					nPairs++;
				}
			}
			peB++;
		}

	}

	mergeNewPairs( newPairs, nPairs );

}

}

#endif // ENABLE_UNUSED_PHYSICS_CODE
