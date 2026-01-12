
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

#ifndef OVERLAPPING_PAIR_CACHE_H
#define OVERLAPPING_PAIR_CACHE_H


#include "btBroadphaseProxy.h"

//#include <set>
#include "atl/atinbintree.h"
#include "atl/pool.h"


namespace rage {

struct	btOverlapCallback
{
    virtual ~btOverlapCallback() { }

	//return true for deletion of the pair
	virtual bool	processOverlap(const btBroadphasePair& ){ return false; }
};


///btOverlappingPairCache maintains the objects with overlapping AABB
///Typically managed by the Broadphase, Axis3Sweep or btSimpleBroadphase
class	btOverlappingPairCache 
{
public:	
    btBroadphasePair* m_cache;
    u32 m_numInCache;
    u16 m_maxPairs;

public:
		
	btOverlappingPairCache();
	btOverlappingPairCache( u16 maxPairs );
	virtual ~btOverlappingPairCache(){ delete m_cache; };

	// Set the overlapping pair cache to a state where there are no overlapping pairs.
	// Don't use this if you don't know what you're doing or else you'll most likely get some missed collisions.
	void Clear()
	{
		m_numInCache = 0;
	}

	btBroadphasePair *GetPairs(){ return m_cache; }
	u32 GetNumPairs(){ return m_numInCache; }
	u16 GetMaxPairs(){ return m_maxPairs; }

    //void	processAllOverlappingPairs(btOverlapCallback*);

	//inline void cleanOverlappingPair(btBroadphasePair& pair);
	
	btBroadphasePair* addOverlappingPair(u16 object0, u16 object1);

	//void mergeNewPairs( btBroadphasePair *newPairs, u32 nPairs );

	btBroadphasePair*	findPair(u16 object0, u16 object1);
		
	void sort();
	
/*	inline bool needsCollision(btBroadphaseProxy* proxy0,btBroadphaseProxy* proxy1) const
	{
		bool collides = proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask;
		collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);
		
		return collides;
	}*/
	
};

} // namespace rage

#endif //OVERLAPPING_PAIR_CACHE_H

