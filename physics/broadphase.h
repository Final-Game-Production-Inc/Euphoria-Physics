
#ifndef PH_BROADPHASE_H
#define PH_BROADPHASE_H

#include "btOverlappingPairCache.h"
#include "vector/vector3.h"

namespace rage
{

class Vector3;

class phBroadPhase
{
public:

	phBroadPhase( u16 maxPairs, btOverlappingPairCache *existingPairCache = NULL )
	{
		m_IsIncremental = true;		// default to true for now to change as little as possible.
		if( existingPairCache )
		{
			m_pairCache = existingPairCache;
		}
		else
		{
			m_pairCache = rage_new btOverlappingPairCache( maxPairs );
		}
	}

	virtual ~phBroadPhase(){ delete m_pairCache; }

	virtual void clear() { m_pairCache->Clear(); }

	virtual void addHandle(const Vector3& aabbMin,const Vector3& aabbMax, u16 pOwner ) = 0;
	virtual void addHandle(const Vector3& center, float fRadius,  u16 pOwner ) = 0;

	virtual void addHandles( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandles ) = 0;
	
	virtual void addHandlesNoNewPairs( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandles ) = 0;

	virtual bool isHandleAdded(unsigned short UNUSED_PARAM(handle)) { return true; }

	virtual void removeHandle(unsigned short handle) = 0;
	virtual void removeHandles(int *handle, int nHandle) = 0;
	virtual void updateHandle(unsigned short handle, const Vector3& aabbMin,const Vector3& aabbMax) = 0;
	virtual void updateHandle(unsigned short handle, const Vector3& center,float fRadius ) = 0;

	virtual btBroadphasePair* addOverlappingPair(u16 object0, u16 object1)
	{
		/*
		// NOTE: This has been disabled because it will release manifolds the user might be looking at.
		//       The user should know/choose when pruneActiveOverlappingPairs is called. 
		// If the pair cache is full, try to make some room because it might be full of gunk
		if (IsFull())
		{
			pruneActiveOverlappingPairs();
		}*/

		// Check to make sure we actually have room, and if so add the pair
		if (m_pairCache->m_numInCache < m_pairCache->m_maxPairs)
		{
			return m_pairCache->addOverlappingPair( object0, object1 ); 
		}
		else
		{
			return NULL;
		}
	}

	bool IsFull() const
	{
		return (m_pairCache->m_numInCache >= m_pairCache->m_maxPairs);
	}

	// dump all the handles in this broadphase into the output lists
	virtual void getHandles( Vector3 **aabbMin, Vector3 **aabbMax, int **pOwner, u16 &nHandles ) const = 0;

	// prepare the cache for transfer to another broadphase.  If it's not in sorted order, it needs to be sorted
	virtual void prepareCacheForTransfer(){}

	virtual u16 getNumHandles() const = 0;

	virtual void PruneNewOverlappingPairs(const int nextStartingBroadphasePair) { FastAssert(0); (void)nextStartingBroadphasePair; }
	virtual void pruneActiveOverlappingPairs() = 0;

	__forceinline void SetPreBreakingPairMarker()
	{
		m_PreBreakingPairMarker = m_pairCache->GetNumPairs();
	}

	__forceinline u32 GetNumPreBreakingPairs() const
	{
		return m_PreBreakingPairMarker;
	}

	virtual Vector3 getQuantize() const
	{
		return VEC3_ZERO;
	}

	virtual Vector3 GetInvQuantize() const
	{
		return VEC3_ZERO;
	}

	__forceinline bool IsIncremental() const
	{
		return m_IsIncremental;
	}

	inline btBroadphasePair *getPrunedPairs() const { return m_pairCache->GetPairs(); }
	inline int getPrunedPairCount() const { return m_pairCache->GetNumPairs(); }

	btOverlappingPairCache *m_pairCache;
protected:
	bool m_IsIncremental;
	u32 m_PreBreakingPairMarker;
};

}

#endif
