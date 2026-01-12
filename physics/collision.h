// 
// physics/collision.h 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_COLLISION_H 
#define PHYSICS_COLLISION_H 

#include "vectormath/mat34v.h"
#include "phcore/constants.h"
#include "phbound/primitives.h"
#include "phbullet/ConvexIntersector.h"						// Only needed for phConvexIntersector::PenetrationDepthSolverType.
#include "phbullet/DiscreteCollisionDetectorInterface.h"	// Only needed for DiscreteCollisionDetectorInterface::SimpleResult.
#include "physics/contact.h"

namespace rage {


class phBound;
class phBoundComposite;
class phManifold;
class phArchetype;
struct phCollisionMemory;

struct phCollisionInput
{
    Mat34V currentA;
    Mat34V currentB;
    Mat34V lastA;
    Mat34V lastB;
    const phBound* boundA;
    const phBound* boundB;
	u32 typeFlagsA;
	u32 includeFlagsA;
	u32 typeFlagsB;
	u32 includeFlagsB;
	phManifold* rootManifold;
	bool highPriority;
	phCollisionMemory * collisionMemory;
	bool useGjkCache;

	phCollisionInput(phCollisionMemory * collisionMemory_, const bool useGjkCache_) : 
		collisionMemory(collisionMemory_), useGjkCache(useGjkCache_) {}
};

#define USE_NEWER_COLLISION_OBJECT (USE_NEW_TRIANGLE_BACK_FACE_CULL && 0)
#define BVH_PRIMS_HAVE_ZERO_CG_OFFSET 1

#define INVALID_COMPONENT -1

#define ValidateComponent(component) { FastAssert(component != INVALID_COMPONENT); FastAssert(component >= 0 && component < 0xFF); }

struct NewCollisionObject
{
	Mat34V m_current;
#if USE_NEWER_COLLISION_OBJECT 
	Vec3V m_trans;
#else // USE_NEWER_COLLISION_OBJECT 
	Mat34V m_last;
#endif // USE_NEWER_COLLISION_OBJECT 

	const phBound * m_bound;
	int m_component;

	__forceinline void set(const phBound * bound, int component)
	{
		FastAssert(bound);
		ValidateComponent(component)
		m_bound = bound;
		m_component = component;
	}

	__forceinline Mat34V_Out GetCurrentMatrix() const { return m_current; }
#if USE_NEWER_COLLISION_OBJECT
#else // USE_NEWER_COLLISION_OBJECT
	__forceinline Mat34V_Out GetLastMatrix() const { return m_last; }
#endif // USE_NEWER_COLLISION_OBJECT
	__forceinline int GetComponent() const { return m_component; }
};

#if USE_NEWER_COLLISION_OBJECT
struct BVHInput
{
	Mat34V m_current;
	Mat34V m_last;
	int m_component;

	__forceinline Mat34V_Out GetCurrentMatrix() const { return m_current; }
	__forceinline Mat34V_Out GetLastMatrix() const { return m_last; }
	__forceinline int GetComponent() const { return m_component; }
};
#else // USE_NEWER_COLLISION_OBJECT
typedef NewCollisionObject BVHInput;
#endif // USE_NEWER_COLLISION_OBJECT

#if USE_NEWER_COLLISION_OBJECT 
	CompileTimeAssert(sizeof(NewCollisionObject) == 16*6);
#else
	CompileTimeAssert(sizeof(NewCollisionObject) == 16*9);
#endif

struct NewCollisionInput
{
	ScalarV sepThresh;
	const NewCollisionObject * object0;
	mutable const NewCollisionObject * object1;
	GJKCacheQueryInput * gjkCacheInfo; 
	const BVHInput * m_bvhInput;

	__forceinline void set(ScalarV_In sepThresh_, const NewCollisionObject * object0_, const NewCollisionObject * object1_, GJKCacheQueryInput * gjkCacheInfo_, BVHInput * bvhInput)
	{
		object0 = object0_;
		object1 = object1_;
		sepThresh = sepThresh_;
		gjkCacheInfo = gjkCacheInfo_;
		m_bvhInput = bvhInput;
	}
};

class ObjectState;
class phConvexIntersector;


#if __SPU
// Combines functionality that was previously being done in phManifold, CollisionInFlight and SpuContactResult.
// The purpose of this class is to neatly wrap the DMA, buffer management and pointer management responsibilities that are necessary
//   when using PPU-owned manifolds on the SPU.
// To use this class, create one and then call InitiateDmaGetStageOne() with the PPU address of the manifold that you want this to represent.
// Once that DMA has completed, call InitiateDmaGetStageTwo().  Once that DMA has completed, you will have a complete manifold whose contact points you can
//   examine or whose composite pointers you can examine (remember that those are still PPU pointers, but you can bring them onto the SPU just the same if
//   you need them).
// If you need to 
class phSpuManifoldWrapper
{
public:
	phSpuManifoldWrapper();

	void ClearManifold(phManifold *ppuManifold);
	void InitiateDmaGetStageOne(phManifold *ppuManifold, u32 tag);
	void InitiateDmaGetStageTwo(u32 tag);
	void InitiateDmaPut(u32 tag);

	phManifold *GetSpuManifold();
	phManifold *GetPpuManifold();

private:
	phManifold m_Manifold;

	// Buffers that exist on the SPU that will contain the actual data.
	phCompositePointers m_CompositePointers;
	phContact m_Contacts[phManifold::MANIFOLD_CACHE_SIZE];
	phContact *m_ContactsLs[phManifold::MANIFOLD_CACHE_SIZE];

	// Keep track of where we came from.  Since we automatically DMA in and out we need to know this.  Can't be a const pointer because it may need to get
	//   passed to phPool<phManifold *>::Release().
	phManifold *m_PpuManifold;
};

phSpuManifoldWrapper::phSpuManifoldWrapper()
{
	for(int contactIndex = 0; contactIndex < phManifold::MANIFOLD_CACHE_SIZE; ++contactIndex)
	{
		m_ContactsLs[contactIndex] = &m_Contacts[contactIndex];
	}
}


void phSpuManifoldWrapper::ClearManifold(phManifold *ppuManifold)
{
	m_PpuManifold = ppuManifold;
	::new (&m_Manifold) phManifold();
	m_Manifold.SetCompositePointersLs(&m_CompositePointers);
	m_Manifold.SetContactsLs(&m_ContactsLs[0]);
}

void phSpuManifoldWrapper::InitiateDmaGetStageOne(phManifold *ppuManifold, u32 tag)
{
	sysDmaLargeGet(&m_Manifold, (uint64_t)ppuManifold, sizeof(phManifold), tag);
	m_PpuManifold = ppuManifold;
}

void phSpuManifoldWrapper::InitiateDmaGetStageTwo(u32 tag)
{
	m_Manifold.SetCompositePointersLs(&m_CompositePointers);
	m_Manifold.SetContactsLs(m_ContactsLs);

	if (m_Manifold.CompositeManifoldsEnabled())
	{
		m_Manifold.GetCompositePointersFromMm(tag);
	}
	else
	{
		m_Manifold.GatherContactPointsFromMm(tag);
	}
}

void phSpuManifoldWrapper::InitiateDmaPut(u32 tag)
{
	sysDmaLargePut(&m_Manifold, (uint64_t)m_PpuManifold, sizeof(phManifold), tag);

	if(m_Manifold.CompositeManifoldsEnabled())
	{
		m_Manifold.PutCompositePointersToMm(tag);
	}
	else
	{
		m_Manifold.ScatterContactPointsToMm(tag);
	}
}

__forceinline phManifold *phSpuManifoldWrapper::GetSpuManifold()
{
	return &m_Manifold;
}


__forceinline phManifold *phSpuManifoldWrapper::GetPpuManifold()
{
	return m_PpuManifold;
}
#endif	// __SPU


// phPairwiseCollisionProcessor handles collision between two directly-collidable (convex) objects.  In other words, don't try to use any composites or BVH or grid
//   bounds with this class - those need to go through a midphase process first (see phMidphase below).
// In order to improve precision, all collisions are re-located so as to be occurring near the origin.
class phPairwiseCollisionProcessor
{
public:
	// Raw collision processing function.
	static void ProcessPairwiseCollision(DiscreteCollisionDetectorInterface::SimpleResult &result, const NewCollisionInput &collisionInput);
	static int CheckContactAgainstEdge(Vec3V_In localizedVertex, Vec3V_In localizedVertexNext, Vec3V_In neighborNormal, Vec3V_In edgeNormal, Vec3V_In curContactNormal);
};

#if !defined(__SPURS_JOB__)

class ProcessLeafCollisionManifold;
class phMidphase
{
public:
	__forceinline phMidphase() {};
	__forceinline ~phMidphase() {};

	void ProcessCollision(const phCollisionInput &unswappedInput) const;
#if USE_NEW_MID_PHASE
	void ProcessCollisionNew(const phCollisionInput &unswappedInput) const;
#endif
#if !USE_NEW_MID_PHASE || ALLOW_MID_PHASE_SWAP
	void ProcessCollisionOriginal(const phCollisionInput &unswappedInput) const;
#endif

	void ProcessSelfCollision(const phCollisionInput &unswappedInput, u8 *selfCollisionPairsA, u8 *selfCollisionPairsB, int numSelfCollisionPairs) const;
#if USE_NEW_SELF_COLLISION
	void ProcessSelfCollisionNew(const phBoundComposite &boundComposite, Mat34V_In curInstanceMatrix, Mat34V_In lastInstanceMatrix, phManifold *rootManifold,
		u8 *selfCollisionPairsA, u8 *selfCollisionPairsB, int numSelfCollisionPairs, bool highPriorityManifolds) const;
#endif
#if !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP
	void ProcessSelfCollisionOriginal(const phBoundComposite &boundComposite, Mat34V_In curInstanceMatrix, Mat34V_In lastInstanceMatrix, phManifold *rootManifold,
		u8 *selfCollisionPairsA, u8 *selfCollisionPairsB, int numSelfCollisionPairs, bool highPriorityManifolds) const;
#endif

private:
#if !USE_NEW_MID_PHASE || ALLOW_MID_PHASE_SWAP
	void ProcessLeafCollision(const ObjectState &objectState0, const ObjectState &objectState1, ProcessLeafCollisionManifold *virtualManifold, GJKCacheQueryInput * gjk_cache_info, phCollisionMemory * collisionMemory) const;
#endif
};

#endif	// !defined(__SPURS_JOB__)

} // namespace rage

#endif // PHYSICS_COLLISION_H 
