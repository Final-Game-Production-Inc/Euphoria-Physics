//
// physics/btImmediateModeBroadphase.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef IMMEDIATE_MODE_BROADPHASE_H
#define IMMEDIATE_MODE_BROADPHASE_H

#include "phcore/constants.h"
#if ENABLE_UNUSED_PHYSICS_CODE

#include "physics/broadphase.h"
#include "physics/levelnew.h"
#include "vector/vector3.h"

namespace rage
{

class btImmediateModeBroadphase : public phBroadPhase
{
protected:
	class AabbHandle
	{
	public:
		Vector3 m_minExtent;
		Vector3 m_maxExtent;
		u16 m_handleID;
	};

public:

	btImmediateModeBroadphase( u16 maxHandles, u16 maxPairs, btOverlappingPairCache *existingPairCache = NULL ) : phBroadPhase( maxPairs, existingPairCache )
	{
		m_maxHandles = maxHandles;
		m_numHandles = 0;
		m_aabbs = rage_new AabbHandle[m_maxHandles];
		m_handleToAabbTable = rage_new u16[m_maxHandles];
		m_maxRetainedPair = maxPairs/4;
		m_retainedPair = rage_new btBroadphasePair[m_maxRetainedPair];
		m_nRetainedPair = 0;
	}

	virtual ~btImmediateModeBroadphase()
	{
		delete [] m_aabbs;
		delete [] m_handleToAabbTable;
		delete [] m_retainedPair;
	}

	btBroadphasePair* addOverlappingPair(u16 object0, u16 object1)
	{ 
		// overlapping pair has been forced in.  need to retain explicitly so it's not lost during merge
		if( m_nRetainedPair < m_maxRetainedPair )
		{
#if __SPU
			u16 generationIdA = phInst::INVALID_INDEX;
			u16 generationIdB = phInst::INVALID_INDEX;
#else
			u16 generationIdA = object0 != phInst::INVALID_INDEX ? PHLEVEL->GetGenerationID(object0) : phInst::INVALID_INDEX;
			u16 generationIdB = object1 != phInst::INVALID_INDEX ? PHLEVEL->GetGenerationID(object1) : phInst::INVALID_INDEX;
#endif
 			m_retainedPair[m_nRetainedPair].Set(object0, generationIdA, object1, generationIdB);
			m_retainedPair[m_nRetainedPair].SetManifold(NULL);
			return m_retainedPair + m_nRetainedPair++;
		}
		else
		{
			AssertMsg( 0, "max retained pairs limit exceeded, explicit collision pair ingored" );
			return NULL;
		}
		
	}

	void addHandle(const Vector3& aabbMin,const Vector3& aabbMax, u16 pOwner )
	{
		FastAssert( pOwner < m_maxHandles );
		FastAssert( m_numHandles < m_maxHandles );
		m_handleToAabbTable[pOwner] = m_numHandles;
		m_aabbs[ m_numHandles ].m_maxExtent = aabbMax;
		m_aabbs[ m_numHandles ].m_minExtent = aabbMin;
		m_aabbs[ m_numHandles ].m_handleID = pOwner;
		m_numHandles++;
	}

	void addHandle(const Vector3& center, float fRadius,  u16 pOwner )
	{
		Vector3 radW(fRadius, fRadius, fRadius);

		addHandle( center - radW, center + radW, pOwner );
	}

	void addHandles( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandles )
	{
		int iHandle;
		for( iHandle = 0; iHandle < nHandles; iHandle++ )
		{
			addHandle( aabbMin[iHandle], aabbMax[iHandle], u16(pOwner[iHandle]) );
		}
	}

	void removeHandle(unsigned short handle)
	{
		FastAssert( m_numHandles > 0 );
		m_numHandles--;

		u16 removeAabbIndex = m_handleToAabbTable[handle];
		m_aabbs[removeAabbIndex] = m_aabbs[m_numHandles];
		m_handleToAabbTable[handle] = (u16)-1;

		int shiftAabbIndex = m_aabbs[m_numHandles].m_handleID;
		m_handleToAabbTable[shiftAabbIndex] = removeAabbIndex;
/*
		int iRetained;
		for( iRetained = 0; iRetained < m_nRetainedPair; iRetained++ )
		{
			if( m_retainedPair[iRetained].m_Object0 == handle || m_retainedPair[iRetained].m_Object1 == handle )
			{
				m_nRetainedPair--;
				if( m_nRetainedPair > iRetained )
				{
					m_retainedPair[iRetained] = m_retainedPair[m_nRetainedPair];
					iRetained--;
				}
			}

		}*/
	}

	void removeHandles(int *handle, int nHandle)
	{
		int iHandle;
		for( iHandle = 0; iHandle < nHandle; iHandle++ )
		{
			removeHandle(u16(handle[iHandle]));
		}
	}

	void updateHandle(unsigned short handle, const Vector3& aabbMin,const Vector3& aabbMax)
	{
		int aabbIndex = m_handleToAabbTable[handle];
		m_aabbs[aabbIndex].m_maxExtent = aabbMax;
		m_aabbs[aabbIndex].m_minExtent = aabbMin;
	}

	//!me this type of thing should be discouraged ( mixing floats and vectors )
	void updateHandle(unsigned short handle, const Vector3& center,float fRadius )
	{
		Warningf( "mixing floats and vectors" );
		Vector3 radW(fRadius, fRadius, fRadius);

		updateHandle( handle, center - radW, center + radW );		
	}

	u16 getNumHandles() const { return m_numHandles; }

	void getHandles( Vector3 **aabbMin, Vector3 **aabbMax, int **pOwner, u16 &nHandles ) const
	{
		Assert( nHandles >= m_numHandles );
		Assert( m_nRetainedPair == 0 );

		int iHandle;
		for( iHandle = 0; iHandle < m_numHandles; iHandle++ )
		{
			(*aabbMin)[iHandle] = m_aabbs[iHandle].m_minExtent;
			(*aabbMax)[iHandle] = m_aabbs[iHandle].m_maxExtent;
			(*pOwner)[iHandle] = m_aabbs[iHandle].m_handleID;
		}

		nHandles = m_numHandles;
	}

	void addHandlesNoNewPairs( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandles )
	{
		addHandles( aabbMin, aabbMax, pOwner, nHandles );
	}

	void mergeNewPairs( btBroadphasePair *newPairs, int nPairs );

protected:

	AabbHandle *m_aabbs;
	u16 *m_handleToAabbTable;
	
	btBroadphasePair *m_retainedPair;
	u16	m_nRetainedPair;
	u16 m_maxRetainedPair;

	u16 m_numHandles;						// number of active handles
	u16 m_maxHandles;						// max number of handles

};

}

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif
