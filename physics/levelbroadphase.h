// 
// physics/levelbroadphase.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_LEVELBROADPHASE_H 
#define PHYSICS_LEVELBROADPHASE_H 

#include "broadphase.h"

#include "phcore/constants.h"
#if ENABLE_UNUSED_PHYSICS_CODE

#include "levelnew.h"
#include "vector/vector3.h"

namespace rage {

class phSimulator;

//!me levelbroadphase is always in the level, so this implementation assumes implicitly that adding/removing etc already handled by the phLevelNew

class phLevelBroadPhase : public phBroadPhase
{
public:

	phLevelBroadPhase(phLevelNew* level, u16 maxHandles, u16 maxPairs, btOverlappingPairCache *existingPairCache = NULL) : phBroadPhase( (u16)maxPairs, existingPairCache ), m_Level(level)
    {
		m_IsIncremental = false;
		m_handleUsed = rage_new u16[maxHandles];
		m_highestHandle = -1;
		m_nHandle = 0;
		memset( m_handleUsed, 0, sizeof(u16)*maxHandles );

		m_maxRetainedPair = maxPairs/4;
		m_retainedPair = rage_new btBroadphasePair[m_maxRetainedPair];
		m_nRetainedPair = 0;

	}

	virtual ~phLevelBroadPhase()
    {
		delete [] m_handleUsed;
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
			u16 generationIdA = PHLEVEL->GetGenerationID(object0);
			u16 generationIdB = PHLEVEL->GetGenerationID(object1);
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

    virtual void addHandle(const Vector3& /*aabbMin*/,const Vector3& /*aabbMax*/, u16 pOwner ) 
	{
		m_handleUsed[ pOwner ] = 1;
		if( pOwner > m_highestHandle )
		{
			m_highestHandle = pOwner;
		}
		m_nHandle++;
	}

    virtual void addHandle(const Vector3& /*center*/, float /*fRadius*/,  u16 pOwner ) 
	{
		m_handleUsed[ pOwner ] = 1;
		if( pOwner > m_highestHandle )
		{
			m_highestHandle = pOwner;
		}
		m_nHandle++;

	}

    virtual void addHandles( const Vector3* aabbMin, const Vector3* aabbMax, int* pOwner, u16 nHandles ) 
	{
		int iHandle;
		for( iHandle = 0; iHandle < nHandles; iHandle++ )
		{
			addHandle( aabbMin[iHandle], aabbMax[iHandle], (u16)(pOwner[iHandle]) );
		}
	}

    virtual void removeHandle(unsigned short pOwner) 
	{
		Assert( pOwner <= m_highestHandle );
		m_handleUsed[ pOwner ] = 0;
		if( pOwner == m_highestHandle )
		{
			while( m_highestHandle > -1 && m_handleUsed[m_highestHandle] == 0 )
			{
				m_highestHandle--;
			}
		}
		m_nHandle--;
	}

    virtual void removeHandles(int* pOwner, int nHandles ) 
	{ 

		int iHandle;
		for( iHandle = 0; iHandle < nHandles; iHandle++ )
		{
			removeHandle( (u16)(pOwner[iHandle]) );
		}

	}
    virtual void updateHandle(unsigned short /*handle*/, const Vector3& /*aabbMin*/,const Vector3& /*aabbMax*/) { }
    virtual void updateHandle(unsigned short /*handle*/, const Vector3& /*center*/,float /*fRadius*/ ) { }

    virtual void pruneActiveOverlappingPairs();

	void getHandles( Vector3 **aabbMin, Vector3 **aabbMax, int **pOwner, u16 &nHandles ) const;
	u16 getNumHandles() const;

	void addHandlesNoNewPairs( const Vector3 *aabbMin, const Vector3 *aabbMax, int *pOwner, u16 nHandles  )
	{
		//!me super gimpy.  doesn't add anything because level octree is always present
		int iHandle;
		for( iHandle = 0; iHandle < nHandles; iHandle++ )
		{
			addHandle( aabbMin[iHandle], aabbMax[iHandle], (u16)(pOwner[iHandle]) );
		}
	}


private:   
    phLevelNew* m_Level;
	u16 *m_handleUsed;
	int m_highestHandle;
	u16 m_nHandle;

	btBroadphasePair *m_retainedPair;
	u16	m_nRetainedPair;
	u16 m_maxRetainedPair;

};

} // namespace rage

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif // PHYSICS_LEVELBROADPHASE_H 
