// 
// physics/iterator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_ITERATOR_H 
#define PHYSICS_ITERATOR_H 

#include "levelnew.h"
#include "cullshape.h"

#include "system/dma.h"
#include "vector/matrix34.h"
#include "vector/vector3.h"

namespace rage {

class phInst;

////////////////////////////////////////////
// phIterator is used to find instances in the physics level that meet certain criteria.  Primarily these criteria are spatial (bounding volume of
//   instance intersects specified volume) but you can also specify that you do or don't want instances of particular states or with particular type
//   and include flags.
// Typical usage involves initializing the cull volume using InitCull_XXX() and the specifically any specific state or flag requirements followed by
//   a call to GetFirstLevelIndex() call and then calls to GetNextLevelIndex() in a loop.
// Use of this class is also supported on the SPU.
// By default, the iterator culls instances spatially using the bounding *sphere* of each instance because the bounding sphere is readily available
//   for each instance without any calculation.  For some cull types (boxes only, currently), the user can specify that the cull operation should
//   use the world-space-axis-aligned bounding *box* of each instance instead.  This is somewhat more costly per-instance but might provide a tighter
//   cull in some cases.
class phIterator
{
public:
    template <class __NodeType> friend class phLevelNodeTree;

#if ENABLE_PHYSICS_LOCK
	enum phIteratorLockType
	{
		PHITERATORLOCKTYPE_READLOCK	=	0,
		PHITERATORLOCKTYPE_WRITELOCK,
		PHITERATORLOCKTYPE_NOLOCK
	};
#	if !__SPU
	phIterator(phIteratorLockType lockType);
#	else	// !__SPU
	phIterator(phIteratorLockType lockType, phMultiReaderLockToken *physicsLock);
#	endif
#else
	phIterator();
#endif

private:
	// Prevent accidental copying, as it is not currently supported
	phIterator(const phIterator&);
	phIterator& operator=(const phIterator&);
public:

    ~phIterator();

	void SetCullShape(const phCullShape& cullShape);
	phCullShape::phCullType GetCullType() const;

	// Specify the spatial criteria for the instances that will be culled.
	// Use InitCull_All() if you want to skip any spatial test.
	void InitCull_Sphere(Vector3::Param center, const float radius);
	void InitCull_Sphere(Vec3V_In center, ScalarV_In radius);
	void InitCull_LineSegment(Vector3::Param p0, Vector3::Param p1);
	void InitCull_LineSegment(Vec3V_In p0, Vec3V_In p1);
	void InitCull_XZCircle(Vector3::Param center, const float radius);
	void InitCull_XZCircle(Vec3V_In center, ScalarV_In radius);
	void InitCull_Capsule(Vector3::Param p0, Vector3::Param shaftAxis, const float shaftLength, const float radius);
	void InitCull_Capsule(Vec3V_In p0, Vec3V_In shaftAxis, ScalarV_In shaftLength, ScalarV_In radius);
	void InitCull_Box(const Matrix34& boxAxes, const Vector3& boxHalfSize);
	void InitCull_Box(Mat34V_In boxAxes, Vec3V_In boxHalfSize);
	void InitCull_Point(Vector3::Param point);
	void InitCull_Point(Vec3V_In point);
	void InitCull_AABB(Vec3V_In boxCenter, Vec3V_In boxHalfSize);
	void InitCull_All();

	// Specify the state and type and include flags of instances that will be culled.
	// Calling these is optional - by default all states and all type and include flags will be accepted.
    void SetStateIncludeFlags(u32 stateIncludeFlags) { m_StateIncludeFlags = stateIncludeFlags;	}
    u32 GetStateIncludeFlags() const { return m_StateIncludeFlags; }
    void SetIncludeFlags(u32 includeFlags) { m_IncludeFlags = includeFlags;	}
    u32 GetIncludeFlags() const { return m_IncludeFlags; }
    void SetTypeFlags(u32 typeFlags) { m_TypeFlags = typeFlags;	}
    u32 GetTypeFlags() const { return m_TypeFlags; }
    void SetTypeExcludeFlags(u32 typeExcludeFlags) { m_TypeExcludeFlags = typeExcludeFlags; }
    u32 GetTypeExcludeFlags() const { return m_TypeExcludeFlags; }

	// Set this flag to true to skip all type and include flag testing.  This would be the same as setting the type and include flags to 0xffffffff *except*
	//   that it will also find objects that have their type or include flags set to 0.  In most situations you wouldn't want this but, the way the sweep-
	//   and-prune broadphase currently works, we need to find all overlapping instances when activating an object, even if they have their flags set to 0.
	__forceinline void SetSkipTypeIncludeFlagsTests(bool skipTypeIncludeFlagsTests) { m_SkipTypeIncludeFlagsTests = skipTypeIncludeFlagsTests; }

	// Set this flag to true to have instance-level culling occur against the AABBs of instances rather than their spheres.
	__forceinline void SetCullAgainstInstanceAABBs(bool cullAgainstInstanceAABBs)
	{
		m_CullAgainstInstanceAABBs = cullAgainstInstanceAABBs;
	}

	inline bool GetCullAgainstInstanceAABBs() { return 	m_CullAgainstInstanceAABBs;	}

    // Used by phLevelNew to determine what objects to cull.  If you have the center and radius handy already, use the one that takes those as parameters
	//   as it saves a transformation.
	bool CheckInstance(Vector3::Vector3Param sphereCenter, Vector3::Vector3Param sphereRadius, u32 typeFlags, u32 includeFlags, u32 stateFlag) const;
    bool CheckInstance(const phInst* RESTRICT inst, u32 typeFlags, u32 includeFlags, u32 stateFlag) const;
    void Clear();

	void ResetCull();

	// Once you've set up the cull parameters, use these two functions retrieve the level indices of the instances that have been culled.
	// phInst::INVALID_INDEX will be returned when the cull is complete.
    u16 GetFirstLevelIndex(const phLevelNew* level);
    u16 GetNextLevelIndex(const phLevelNew* level);

#if !__SPU
    void GetAllLevelIndices(const phLevelNew* level, atArray<u16>& outArray);
#endif	// !__SPU

#if __PFDRAW
	void DrawCullShape() { m_CullShape.Draw(); };
#endif

#if ENABLE_PHYSICS_LOCK
	void GetLock(phIteratorLockType lockType);
	void ReleaseLock();
#endif // ENABLE_PHYSICS_LOCK

private:
	// PURPOSE: Cull out instances and polys that do not intersect this shape
	phCullShape m_CullShape;

    // PURPOSE: the types of objects to include (the include flags must overlap the object's type flags)
    u32 m_IncludeFlags;

    // PURPOSE: Exclude object that have any of these type flags.
    u32 m_TypeExcludeFlags;

    // PURPOSE: the types of object that this iterator is (the type flags must overlap the other object's include flags)
    u32 m_TypeFlags;

    // PURPOSE: the states of objects to include (active, fixed, inactive)
    u32 m_StateIncludeFlags;

	bool m_SkipTypeIncludeFlagsTests;

	// PURPOSE: Set this flag to have instance-level culling occur against
	bool m_CullAgainstInstanceAABBs;

    // These are really only needed for in-place iterations.
    u16 m_LastObjectIndex;
	u16 m_NodeIndices[phLevelNew::knMaxOctreeDepth+1];
    u8 m_ChildIndices[phLevelNew::knMaxOctreeDepth];
    int m_CurDepth;

	// PURPOSE: Track if this iterator has the global level lock.
#if ENABLE_PHYSICS_LOCK
	phIteratorLockType	m_LockType;
#endif

#if __SPU || EMULATE_SPU
	// Additional public interface and data for SPU code.
public:
	__forceinline void SetInstanceBuffer(void *instanceBuffer, const u32 size)
	{
		m_InstanceBuffer = instanceBuffer;
		m_InstanceBufferSize = size;
	}

	__forceinline const phInst *GetInstanceBuffer() const
	{
		return reinterpret_cast<const phInst *>(m_InstanceBuffer);
	}

	__forceinline u32 GetInstanceBufferSize() const
	{
		return m_InstanceBufferSize;
	}

	__forceinline void InitiateInstanceDMA(const phInst *ppuInstance)
	{
		// This get needs to be fenced, since we do not always wait for the previous get to have completed
		sysDmaGetf(m_InstanceBuffer, (uint32_t)(ppuInstance), GetInstanceBufferSize(), DMA_TAG(5));
		SetPPUInstance(ppuInstance);
	}

	__forceinline void WaitCompleteInstanceDMA() const
	{
		sysDmaWaitTagStatusAll(DMA_MASK(5));
	}

	__forceinline void SetArchetypeBuffer(void *archetypeBuffer, const u32 size)
	{
		m_ArchetypeBuffer = archetypeBuffer;
		m_ArchetypeBufferSize = size;
	}

	__forceinline const phArchetype *GetArchetypeBuffer() const
	{
		return reinterpret_cast<const phArchetype *>(m_ArchetypeBuffer);
	}

	__forceinline u32 GetArchetypeBufferSize() const
	{
		return m_ArchetypeBufferSize;
	}

	__forceinline void InitiateArchetypeDMA(const phArchetype *ppuArchetype) const
	{
		sysDmaGet(m_ArchetypeBuffer, (uint32_t)(ppuArchetype), GetArchetypeBufferSize(), DMA_TAG(5));
	}

	__forceinline void WaitCompleteArchetypeDMA() const
	{
		sysDmaWaitTagStatusAll(DMA_MASK(5));
	}

	__forceinline void SetBoundBuffer(void *boundBuffer, const u32 size)
	{
		m_BoundBuffer = boundBuffer;
		m_BoundBufferSize = size;
	}

	__forceinline const phBound *GetBoundBuffer() const
	{
		return reinterpret_cast<const phBound *>(m_BoundBuffer);
	}

	__forceinline u32 GetBoundBufferSize() const
	{
		return m_BoundBufferSize;
	}

	__forceinline void InitiateBoundDMA(const phBound *ppuBound) const
	{
		sysDmaGet(m_BoundBuffer, (uint32_t)(ppuBound), GetBoundBufferSize(), DMA_TAG(5));
	}

	__forceinline void WaitCompleteBoundDMA() const
	{
		sysDmaWaitTagStatusAll(DMA_MASK(5));
	}

	__forceinline void SetPPUInstance(const phInst *ppuInstance)
	{
		m_PPUInstance = ppuInstance;
	}

	__forceinline const phInst *GetPPUInstance() const
	{
		return m_PPUInstance;
	}

	__forceinline const phObjectData *GetCurObjectData() const
	{
		sysDmaWaitTagStatusAll(DMA_MASK(6));
		return m_CurObjectData;
	}

	__forceinline const phObjectData *GetNextObjectData() const
	{
		sysDmaWaitTagStatusAll(DMA_MASK(6));
		return m_NextObjectData;
	}

	__forceinline void FetchCurObjectData(const phObjectData *pSourcePPUAddress)
	{
		const phObjectData *pCurObjectData = reinterpret_cast<const phObjectData *>(m_CurObjectDataBuffer);
		sysDmaGet(m_CurObjectDataBuffer, (uint32_t)(pSourcePPUAddress), sizeof(phObjectData), DMA_TAG(6));
		m_CurObjectData = pCurObjectData;
	}

	__forceinline void FetchNextObjectData(const phObjectData *pSourcePPUAddress)
	{
		const phObjectData *pCurObjectData = reinterpret_cast<const phObjectData *>(m_NextObjectDataBuffer);
		sysDmaGet(m_NextObjectDataBuffer, (uint32_t)(pSourcePPUAddress), sizeof(phObjectData), DMA_TAG(6));
		m_NextObjectData = pCurObjectData;
	}

	__forceinline void PushNextObjectDataToCurrent()
	{
		// Wait to ensure that the DMA that's bringing the next object data in has completed.
		sysDmaWaitTagStatusAll(DMA_MASK(6));

		// Copy the data from m_NextObjectDataBuffer to m_CurObjectDataBuffer.  We use vector moves to do 16 bytes at a time.
		*reinterpret_cast<Vec4V *>(&m_CurObjectDataBuffer[0]) = *reinterpret_cast<const Vec4V *>(&m_NextObjectDataBuffer[0]);
		CompileTimeAssert(sizeof(phObjectData) == 32);
		*reinterpret_cast<Vec4V *>(&m_CurObjectDataBuffer[16]) = *reinterpret_cast<const Vec4V *>(&m_NextObjectDataBuffer[16]);

		// Make m_CurObjectData point into m_CurObjectDataBuffer now that the data has been moved into there, and make m_NextObjectData point to nothing.
		m_CurObjectData = reinterpret_cast<const phObjectData *>(m_CurObjectDataBuffer);
		m_NextObjectData = NULL;
	}

	void *m_InstanceBuffer, *m_ArchetypeBuffer, *m_BoundBuffer;
	u32 m_InstanceBufferSize, m_ArchetypeBufferSize, m_BoundBufferSize;
	const phInst *m_PPUInstance;

#if ENABLE_PHYSICS_LOCK
	phMultiReaderLockToken *m_PhysicsLock;
#endif	// ENABLE_PHYSICS_LOCK

	// Two buffers to hold the object data's - one in current use and the other next to be used.
	ALIGNAS(16) u8 m_CurObjectDataBuffer[sizeof(phObjectData)] ;
	ALIGNAS(16) u8 m_NextObjectDataBuffer[sizeof(phObjectData)] ;

	// 
	const phObjectData *m_CurObjectData, *m_NextObjectData;		// This is what is returned to the user
#endif
};

inline void phIterator::SetCullShape(const phCullShape& cullShape)
{
	m_CullShape = cullShape;
};

inline phCullShape::phCullType phIterator::GetCullType() const
{
	return m_CullShape.GetCullType(); 
}

inline void phIterator::InitCull_Sphere(Vector3::Vector3Param center, const float radius)
{
    m_CullShape.InitCull_Sphere(RCC_VEC3V(center), ScalarVFromF32(radius));
};

inline void phIterator::InitCull_Sphere(Vec3V_In center, ScalarV_In radius)
{
	m_CullShape.InitCull_Sphere(center, radius);
};

inline void phIterator::InitCull_LineSegment(Vector3::Vector3Param p0, Vector3::Vector3Param p1)
{
    m_CullShape.InitCull_LineSegment(RCC_VEC3V(p0), RCC_VEC3V(p1));
}

inline void phIterator::InitCull_LineSegment(Vec3V_In p0, Vec3V_In p1)
{
	m_CullShape.InitCull_LineSegment(p0, p1);
}

inline void phIterator::InitCull_XZCircle(Vector3::Vector3Param center, const float radius)
{
    m_CullShape.InitCull_XZCircle(RCC_VEC3V(center), ScalarVFromF32(radius));
};

inline void phIterator::InitCull_XZCircle(Vec3V_In center, ScalarV_In radius)
{
	m_CullShape.InitCull_XZCircle(center, radius);
};

inline void phIterator::InitCull_Capsule(Vector3::Vector3Param p0, Vector3::Vector3Param shaftAxis, const float shaftLength, const float radius)
{
    m_CullShape.InitCull_Capsule(RCC_VEC3V(p0), RCC_VEC3V(shaftAxis), ScalarVFromF32(shaftLength), ScalarVFromF32(radius));
}

inline void phIterator::InitCull_Capsule(Vec3V_In p0, Vec3V_In shaftAxis, ScalarV_In shaftLength, ScalarV_In radius)
{
	m_CullShape.InitCull_Capsule(p0, shaftAxis, shaftLength, radius);
}

inline void phIterator::InitCull_Box (const Matrix34& boxAxes, const Vector3& boxHalfSize)
{
    m_CullShape.InitCull_Box(RCC_MAT34V(boxAxes), RCC_VEC3V(boxHalfSize));
}

inline void phIterator::InitCull_Box (Mat34V_In boxAxes, Vec3V_In boxHalfSize)
{
	m_CullShape.InitCull_Box(boxAxes, boxHalfSize);
}

inline void phIterator::InitCull_Point(Vector3::Vector3Param point)
{
    m_CullShape.InitCull_Point(RCC_VEC3V(point));
}

inline void phIterator::InitCull_Point(Vec3V_In point)
{
	m_CullShape.InitCull_Point(point);
}

inline void phIterator::InitCull_AABB(Vec3V_In boxCenter, Vec3V_In boxHalfSize)
{
	m_CullShape.InitCull_AABB(boxCenter,boxHalfSize);
}

inline void phIterator::InitCull_All()
{
    m_CullShape.InitCull_All();
}

inline void phIterator::ResetCull()
{
	m_IncludeFlags = INCLUDE_FLAGS_ALL;
	m_TypeExcludeFlags = TYPE_FLAGS_NONE;
	m_TypeFlags = TYPE_FLAGS_ALL;
	m_StateIncludeFlags = phLevelBase::STATE_FLAGS_ALL;
	m_SkipTypeIncludeFlagsTests = false;
	m_CullAgainstInstanceAABBs = false;

#if __SPU || EMULATE_SPU
	m_CurObjectData = m_NextObjectData = NULL;
#endif
}

#if ENABLE_PHYSICS_LOCK
#	if !__SPU
inline phIterator::phIterator(phIteratorLockType lockType)
#	else	// !__SPU
inline phIterator::phIterator(phIteratorLockType lockType, phMultiReaderLockToken *physicsLock)
#	endif
#else
inline phIterator::phIterator() 
#endif
{
#if ENABLE_PHYSICS_LOCK
	// NOTE: It might be possible to make this more efficient by always having iterators acquire reader locks (since they don't change anything themselves)
	//   and then, if the client of the iterator decides to make a change as a result of something he finds while iterating, let that operation acquire a write
	//   lock at that time.  Right now that wouldn't work because the read lock acquired here would prevent the write lock from being acquired later, however
	//   it should be possible to make that work (if it's actually a safe thing to do).
	//     Comment above preserved for future reference.  That's not a safe thing to do and it can create deadlock where two iterators have both acquired
	//   read locks and then want to acquire write locks but have to wait until all read locks (except their own) have been released, which would never happen
	//   because they would each be waiting for the other to give up their read lock first.
	SPU_ONLY(m_PhysicsLock = physicsLock;)
	if(lockType != PHITERATORLOCKTYPE_NOLOCK)
	{
		ASSERT_ONLY(m_LockType = PHITERATORLOCKTYPE_NOLOCK;) // Keep GetLock from complaining about already having a lock
		GetLock(lockType);
	}
	else
	{
		m_LockType = lockType;
	}
#endif

	ResetCull();
    Clear();
}

inline phIterator::~phIterator()
{
#if ENABLE_PHYSICS_LOCK
	if(m_LockType != PHITERATORLOCKTYPE_NOLOCK)
	{
		ReleaseLock();
	}
#endif

#if __SPU
	// It is possible that the caller hasn't waited on all dma gets, so do that here
	sysDmaWaitTagStatusAll(DMA_MASK(5) | DMA_MASK(6));
#endif
}

inline u16 phIterator::GetFirstLevelIndex(const phLevelNew* level)
{
    FastAssert(level != NULL);
#if ENABLE_PHYSICS_LOCK
	Assertf(m_LockType != PHITERATORLOCKTYPE_NOLOCK, "Trying to access the level without a lock.");
#endif // ENABLE_PHYSICS_LOCK

	const u16 levelIndex = level->GetFirstCulledObject(*this);
	m_LastObjectIndex = levelIndex;
    return levelIndex;
}

inline u16 phIterator::GetNextLevelIndex(const phLevelNew* level)
{
    FastAssert(level != NULL);
#if ENABLE_PHYSICS_LOCK
	Assertf(m_LockType != PHITERATORLOCKTYPE_NOLOCK, "Trying to access the level without a lock.");
#endif // ENABLE_PHYSICS_LOCK

    const u16 levelIndex = level->GetNextCulledObject(*this);
	m_LastObjectIndex = levelIndex;
	return levelIndex;
}

#if !__SPU
inline void phIterator::GetAllLevelIndices(const phLevelNew* level, atArray<u16>& outArray)
{
    u16 levelIndex = GetFirstLevelIndex(level);

    while (levelIndex != (u16)(-1))
    {
        outArray.Push(levelIndex);

        levelIndex = GetNextLevelIndex(level);
    }
}
#endif	// !__SPU

#if ENABLE_PHYSICS_LOCK
__forceinline void phIterator::GetLock(phIteratorLockType lockType)
{
	Assertf(lockType != PHITERATORLOCKTYPE_NOLOCK,"Trying to get an invalid lock.");
	Assertf(m_LockType == PHITERATORLOCKTYPE_NOLOCK,"Trying to overwrite current lock type.");

#if !__SPU
	phMultiReaderLockToken *physLockToUse = &g_GlobalPhysicsLock;
#else
	phMultiReaderLockToken *physLockToUse = m_PhysicsLock;
#endif

	// Neither of these waits should ever cause a lock because we're only ever re-waiting a lock type that we've already acquired on this thread.
	m_LockType = lockType;
	if(lockType == PHITERATORLOCKTYPE_READLOCK)
	{
		physLockToUse->WaitAsReader();
	}
	else
	{
		Assert(lockType == PHITERATORLOCKTYPE_WRITELOCK);
		physLockToUse->WaitAsWriter();
	}
}

__forceinline void phIterator::ReleaseLock()
{
	Assertf(m_LockType != PHITERATORLOCKTYPE_NOLOCK,"Trying to release a lock when phIterator doesn't own one.");
#if !__SPU
	phMultiReaderLockToken *physLockToUse = &g_GlobalPhysicsLock;
#else
	phMultiReaderLockToken *physLockToUse = m_PhysicsLock;
#endif

	if(m_LockType == PHITERATORLOCKTYPE_READLOCK)
	{
		physLockToUse->ReleaseAsReader();
	}
	else
	{
		Assert(m_LockType == PHITERATORLOCKTYPE_WRITELOCK);
		physLockToUse->ReleaseAsWriter();
	}
	m_LockType = PHITERATORLOCKTYPE_NOLOCK;
}
#endif // ENABLE_PHYSICS_LOCK

} // namespace rage

#endif // PHYSICS_ITERATOR_H 
