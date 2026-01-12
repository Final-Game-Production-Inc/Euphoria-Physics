/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/



#include "btOverlappingPairCache.h"
#include "levelnew.h"

#include "simulator.h"

#include "phcore/pool.h"

#include "profile/element.h"
#include <algorithm>

namespace rage {

namespace phSweepAndPruneStats
{
    EXT_PF_TIMER(SAPAddPair);
    EXT_PF_TIMER(SAPRemovePair);
};

using namespace phSweepAndPruneStats;

btOverlappingPairCache::btOverlappingPairCache()
{
}

btOverlappingPairCache::btOverlappingPairCache( u16 maxPairs )
{
	m_maxPairs = maxPairs;
	m_cache = rage_new btBroadphasePair[m_maxPairs];
	m_numInCache = 0;
}

/*
__forceinline void btOverlappingPairCache::cleanOverlappingPair(btBroadphasePair& pair)
{
    if (pair.GetManifold())
    {
        PHMANIFOLD->Release(pair.GetManifold());
    }
    
    pair.SetManifold(NULL);
}
*/

btBroadphasePair*	btOverlappingPairCache::addOverlappingPair(u16 object0, u16 object1)
{
    PF_FUNC(SAPAddPair);

	int numInCache = sysInterlockedIncrement(&m_numInCache) - 1;

#if __ASSERT
	if (numInCache >= m_maxPairs)
	{
		static bool s_DumpedAlready = false;

		if (!s_DumpedAlready)
		{
			s_DumpedAlready = true;

			for (int i = 0; i < m_maxPairs; ++i)
			{
				btBroadphasePair& pair = m_cache[i];

				int object0 = pair.GetObject0();
				const char* name0 = "(invalid)";
				Vec3V currentPos0(V_ZERO);
				Vec3V lastPos0(V_ZERO);
				if (PHLEVEL->LegitLevelIndex(object0))
				{
					if (phInst* inst = PHLEVEL->GetInstance(object0,pair.GetGenId0()))
					{
						if (phArchetype* arch = inst->GetArchetype())
						{
							name0 = arch->GetFilename();
						}
						else
						{
							name0 = "(_noname_)";
						}

						currentPos0 = inst->GetPosition();
						lastPos0 = PHSIM->GetLastInstanceMatrix(inst).d();
					}
					else
					{
						name0 = "(null_inst)";
					}
				}
				else
				{
					name0 = "(invalid_levelidx)";
				}

				int object1 = pair.GetObject1();
				const char* name1 = "(invalid)";
				Vec3V currentPos1(V_ZERO);
				Vec3V lastPos1(V_ZERO);
				if (PHLEVEL->LegitLevelIndex(object1))
				{
					if (phInst* inst = PHLEVEL->GetInstance(object1,pair.GetGenId1()))
					{
						if (phArchetype* arch = inst->GetArchetype())
						{
							name1 = arch->GetFilename();
						}
						else
						{
							name1 = "(_noname_)";
						}

						currentPos1 = inst->GetPosition();
						lastPos1 = PHSIM->GetLastInstanceMatrix(inst).d();
					}
					else
					{
						name1 = "(null_inst)";
					}
				}
				else
				{
					name1 = "(invalid_levelidx)";
				}

				Displayf("%d A: %s:%d, <%f, %f, %f> => <%f, %f, %f>", i, name0, object0, VEC3V_ARGS(lastPos0), VEC3V_ARGS(currentPos0));
				Displayf("%d B: %s:%d, <%f, %f, %f> => <%f, %f, %f>", i, name1, object1, VEC3V_ARGS(lastPos1), VEC3V_ARGS(currentPos1));
			}
		}
	}
#endif // __ASSERT

	if (Verifyf(numInCache < m_maxPairs, "Ran out of broadphase pairs"))
	{
		u16 generationIdA = PHLEVEL->GetGenerationID(object0);
		u16 generationIdB = PHLEVEL->GetGenerationID(object1);
		m_cache[numInCache].Init(object0, generationIdA, object1, generationIdB);
		FastAssert(m_cache[numInCache].GetManifold() == NULL);
	    
		return m_cache + numInCache;
	}
	else
	{
		sysInterlockedDecrement(&m_numInCache);
		return NULL;
	}
}


void btOverlappingPairCache::sort()
{
	std::sort(m_cache, m_cache + m_numInCache);
}

/*
void btOverlappingPairCache::mergeNewPairs( btBroadphasePair *newPairs, u32 nPairs )
{
	// we are going to replace cached list with our new one, but first we need to remove any pairs that just became disjoint and copy manifolds
	// m_cache should already be sorted
	
	std::sort(newPairs, newPairs + nPairs);

	u32 iNewPair, iOldPair;
	for( iNewPair = 0, iOldPair = 0; iNewPair < nPairs; iNewPair++ )
	{
		// remove pairs in cache that aren't there anymore
		while( iOldPair < m_numInCache &&  m_cache[iOldPair] < newPairs[iNewPair] )
		{
			cleanOverlappingPair(*(m_cache + iOldPair));
			iOldPair++;
		}

		if( iOldPair < m_numInCache && m_cache[iOldPair] == newPairs[iNewPair] )
		{
			newPairs[iNewPair].SetManifold(m_cache[iOldPair].GetManifold());
			iOldPair++;
		}
	}

	while( iOldPair < m_numInCache )
	{
		cleanOverlappingPair( *(m_cache + iOldPair) );
		iOldPair++;
	}

	// Copy the new list into m_cache, removing any duplicates (which will have to be adjacent).
	m_numInCache = 0;
	for( iNewPair = 0; iNewPair < nPairs && m_numInCache < m_maxPairs; iNewPair++ )
	{
		if(m_numInCache == 0 || !(newPairs[iNewPair] == m_cache[m_numInCache - 1]))
		{
			Assert(m_numInCache < m_maxPairs);
			m_cache[m_numInCache] = newPairs[iNewPair];
			++m_numInCache;
		}
	}

	for(; iNewPair < nPairs; ++iNewPair)
	{
		cleanOverlappingPair(newPairs[iNewPair]);
//		Assert(newPairs[iNewPair].m_Manifold == NULL);
	}
}
*/

/*
void	btOverlappingPairCache::processAllOverlappingPairs(btOverlapCallback* callback)
{
    btBroadphasePair* pair = m_cache;
	for (int pairIndex = 0; pairIndex < m_numInCache; ++pairIndex, ++pair)
	{
        if (callback->processOverlap(*pair))
        {
            //cleanOverlappingPair(pair);
        }
	}
}
*/

} // namespace rage

