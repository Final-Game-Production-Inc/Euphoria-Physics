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

#ifndef PH_BROADPHASE_PROXY_H
#define PH_BROADPHASE_PROXY_H

#include "math/amath.h"
#include "phcore/constants.h"

namespace rage {

class phManifold;

/// contains a pair of aabb-overlapping objects
struct btBroadphasePair
{
#if USE_STATIC_BVH_REJECTION
	enum
	{
		INVALID_INDEX = (u16)-1
	};
#endif // USE_STATIC_BVH_REJECTION

	inline void Init(u16 object0, u16 genId0, u16 object1, u16 genId1)
	{
		//keep them sorted, so the std::set operations work
		bool object0bigger = object0 >= object1;
		m_id.m_Object0 = object0bigger ? object1 : object0;
		m_id.m_GenId0 = object0bigger ? genId1 : genId0;
		m_id.m_Object1 = object0bigger ? object0 : object1;
		m_id.m_GenId1 = object0bigger ? genId0 : genId1;
		m_Manifold = NULL;
#if USE_STATIC_BVH_REJECTION
		m_CacheIndex = INVALID_INDEX;
#endif // USE_STATIC_BVH_REJECTION
	}

	__forceinline u16 GetObject0() const
	{
		return m_id.m_Object0;
	}

	__forceinline u16 GetObject1() const
	{
		return m_id.m_Object1;
	}

	__forceinline u16 GetGenId0() const
	{
		return m_id.m_GenId0;
	}

	__forceinline u16 GetGenId1() const
	{
		return m_id.m_GenId1;
	}

	__forceinline phManifold* GetManifold() const
	{
		return m_Manifold;
	}

	__forceinline void SetManifold(phManifold* manifold)
	{
		m_Manifold = manifold;
	}

#if USE_STATIC_BVH_REJECTION
	__forceinline u16 GetCacheIndex() const
	{
		return m_CacheIndex;
	}

	__forceinline void SetCacheIndex(const u16 cacheIndex)
	{
		m_CacheIndex = cacheIndex;
	}
#endif // USE_STATIC_BVH_REJECTION

	u64 GetSortKey() const
	{
		return m_key;
	}

private:
	struct ID
	{
		u16 m_Object0;
		u16 m_GenId0;
		u16 m_Object1;
		u16 m_GenId1;
	};

	union
	{
		u64 m_key;
		ID m_id;
	};

    phManifold* m_Manifold;

#if USE_STATIC_BVH_REJECTION
	u16 m_CacheIndex;
#endif // USE_STATIC_BVH_REJECTION
};

#if USE_STATIC_BVH_REJECTION

extern void DeleteStaticBvhCacheEntry(const u16 cacheIndex);
#define DESTRUCT_BP_PAIR(pair) \
{ \
	phManifold * _manifold_ = pair->GetManifold(); \
	if (_manifold_) \
	{ \
		pair->SetManifold(NULL); \
		PHMANIFOLD->Release(_manifold_); \
	} \
	const u16 _cacheIndex_ = pair->GetCacheIndex(); \
	if (_cacheIndex_ != btBroadphasePair::INVALID_INDEX) \
	{ \
		pair->SetCacheIndex(btBroadphasePair::INVALID_INDEX); \
		DeleteStaticBvhCacheEntry(_cacheIndex_); \
	} \
} 

#else // USE_STATIC_BVH_REJECTION

#define DESTRUCT_BP_PAIR(pair) \
{ \
	phManifold * _manifold_ = pair->GetManifold(); \
	if (_manifold_) \
	{ \
		pair->SetManifold(NULL); \
		PHMANIFOLD->Release(_manifold_); \
	} \
} 

#endif // USE_STATIC_BVH_REJECTION

#define DELETE_BP_MANIFOLD(pair) \
{ \
	phManifold * _manifold_ = pair->GetManifold(); \
	if (_manifold_) \
	{ \
		pair->SetManifold(NULL); \
		PHMANIFOLD->Release(_manifold_); \
	} \
}

// Comparison operator, needed for sorting.
__forceinline bool operator<(const btBroadphasePair& a, const btBroadphasePair& b) 
{ 
#if 0
	// Old version that doesn't take into account generation Ids when sorting.
	const u32 combineda = (a.GetObject0() << 16) + a.GetObject1();
	const u32 combinedb = (b.GetObject0() << 16) + b.GetObject1();
	return (combineda < combinedb);
#elif 0
	const u64 combinedA = ((u64)a.GetObject0() << 48) | ((u64)a.GetObject1() << 32) | ((u64)a.GetGenId0() << 16) | ((u64)a.GetGenId1());
	const u64 combinedB = ((u64)b.GetObject0() << 48) | ((u64)b.GetObject1() << 32) | ((u64)b.GetGenId0() << 16) | ((u64)b.GetGenId1());
	return (combinedA < combinedB);
	// Note that, with this version, we intentionally compare in different directions below - we sort in order of increasing level indices, then increasing
	//   generation Ids, and then *decreasing* manifold pointers (to push non-NULL manifolds to the front).
//	return (combinedA < combinedB) || (combinedA == combinedB) && (a.GetManifold() > b.GetManifold());
#else
	return (a.GetSortKey() < b.GetSortKey());
#endif
}

// I don't think that this is actually used anywhere but it's easy enough to implement.
inline bool operator==(const btBroadphasePair& a, const btBroadphasePair& b) 
{ 
#if 0
	u32 combineda = (a.GetObject0() << 16) + a.GetObject1();
	u32 combinedb = (b.GetObject0() << 16) + b.GetObject1();
	return (combineda == combinedb);
#else
	return (a.GetSortKey() == b.GetSortKey());
#endif
}

} // namespace rage

#endif //PH_BROADPHASE_PROXY_H

