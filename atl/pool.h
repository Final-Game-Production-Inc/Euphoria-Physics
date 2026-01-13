//
// atl/pool.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_POOL_H
#define ATL_POOL_H

#include <string.h>

#include "array.h"
#include "bitset.h"
#include "map.h"

#include "data/struct.h"
#include "data/safestruct.h"
#include "diag/tracker.h"
#include "math/amath.h"
#include "system/new.h"
#include "system/buddyallocator_config.h"
#include "system/debugmemoryfill.h"

#define ATL_POOL_USE_MARKERS 0
#define ATL_POOL_SPARSE (USE_SPARSE_MEMORY)

// When this is enabled, atPoolBase will write a debug pattern (0xdf) into the pool memory when it is first created and when an entry is returned
//   to the pool.  It will check that that memory is undisturbed when that entry is allocated.  If it doesn't have the correct debug pattern then
//   something wrote to that memory when it should have been unallocated.
#define ATL_POOL_CHECK_STOMPS	(0 && !__FINAL)

#if ATL_POOL_USE_MARKERS
#define DATA_MARKER_INTENTIONAL_HEADER_INCLUDE 1
#include "data/marker.h"
#define ATL_POOL_FUNC() RAGE_FUNC()
#else
#define ATL_POOL_FUNC()
#endif

#define RAGE_POOL_SPILLOVER ((__XENON || __PPU) && !ATL_POOL_SPARSE && 1)
CompileTimeAssert(!(ATL_POOL_SPARSE && RAGE_POOL_SPILLOVER));

#if RAGE_POOL_SPILLOVER
#define RAGE_POOL_SPILLOVER_ONLY(x) x
#else
#define RAGE_POOL_SPILLOVER_ONLY(x)
#endif

#define RAGE_POOL_SPILLOVER_NOTIFY (RAGE_POOL_SPILLOVER && 0)
#if RAGE_POOL_SPILLOVER_NOTIFY
#define RAGE_POOL_SPILLOVER_NOTIFY_ONLY(x) x
#else
#define RAGE_POOL_SPILLOVER_NOTIFY_ONLY(x)
#endif

#if __PPU || RSG_ORBIS
#define RAGE_POOL_TRACKING (RAGE_TRACKING && !__PROFILE && defined(_CPPRTTI))
#else
#define RAGE_POOL_TRACKING (RAGE_TRACKING && !__PROFILE && _CPPRTTI)
#endif

#if RAGE_POOL_TRACKING
#define RAGE_POOL_TRACKING_ONLY(...) __VA_ARGS__
#else
#define RAGE_POOL_TRACKING_ONLY(...)
#endif

// EJ: When this is enabled, memvisualize will track individual pool new/delete requests.
// This is awesome for debugging pool-related memory problems.
#define RAGE_POOL_NODE_TRACKING (RAGE_POOL_TRACKING && 1)

#if RAGE_POOL_NODE_TRACKING
#define RAGE_POOL_NODE_TRACKING_ONLY(x) x
#else
#define RAGE_POOL_NODE_TRACKING_ONLY(x)
#endif

namespace rage {

/*
PURPOSE
    atPool maintains an array of objects on the caller's behalf and
    will allow random-access allocation and deallocation of the
    contained objects.  No constructors or destructors for the
    contained type are ever invoked by this code because of complications 
    with the way our build system redefines "new" as a macro.
    Allocation and deallocation are both done in O(1) time.
    Only requirement for class _Type is that its size is as least as
    large as a pointer (which is reasonable, because you need a pointer
    to store the result in the first place).
<FLAG Component>
<COMBINE atPool>
*/

#if RAGE_POOL_TRACKING	
	class PoolTracker
	{
	public:
		virtual ~PoolTracker() { /*No op*/ }

		const void* GetRawPoolPointer() const	{ return m_poolPointer; }

		virtual size_t GetStorageSize() const = 0;
		virtual s32 GetSize() const = 0;
		virtual s32 GetSpilloverSize() const { return 0; }
		virtual s32 GetNoOfUsedSpaces() const = 0;
		virtual s32 GetPeakSlotsUsed() const = 0;
		virtual s32 GetNoOfFreeSpaces() const = 0;
		virtual s32 GetActualMemoryUse() const = 0;
		virtual s32 GetMemoryUsed() const { return (s32)GetStorageSize() * GetSize(); }
		virtual const char* GetName() const = 0;
		virtual const char* GetClassName() const = 0;

		// Static
		static void Add(PoolTracker* pTracker);
		static void Remove(const void* poolptr);
		static atArray<PoolTracker*>& GetTrackers() {return s_trackers;}

	private:
		static atArray<PoolTracker*> s_trackers;

	protected:
		const void* m_poolPointer;
	};
#endif

class atPoolBase {
public:
	// PURPOSE: Constructor
	// PARAMS: elemSize - the size of the elements in the pool
    atPoolBase(size_t elemSize)
        : m_Pool( 0 )
        , m_Size( 0 )
        , m_FreeCount( 0 )
		, m_ElemSize( elemSize )
        , m_FirstFree( 0 )
#if !__FINAL
		, m_PeakFreeCount( 0 )
#endif // !__FINAL
        , m_OwnMem( false )
#if RAGE_POOL_SPILLOVER
		, m_SpilloverPool(NULL), m_SpilloverSize(0), m_PoolSize(0)
#endif
    {
		ATL_POOL_FUNC();
    }

	// PURPOSE: Constructor
	// PARAMS: elemSize - the size of the elements in the pool
	//         size - number of elements of the type to allocate space for
	atPoolBase(size_t elemSize, size_t size, const bool useFlex = false)
		: m_Pool( 0 )
		, m_Size( 0 )
		, m_FreeCount( 0 )
		, m_ElemSize( elemSize )
		, m_FirstFree( 0 )
#if !__FINAL
		, m_PeakFreeCount( 0 )
#endif // !__FINAL
		, m_OwnMem( false )
#if RAGE_POOL_SPILLOVER
		, m_SpilloverPool(NULL), m_SpilloverSize(0), m_PoolSize(0)
#endif
	{
		ATL_POOL_FUNC();
		Init(size, useFlex);
	}

	// PURPOSE: Constructs a pool with caller-owned memory
	// PARAMS: mem         - Caller-owned memory.
	//         sizeofMem   - Number of bytes of caller-owned memory.
	atPoolBase( size_t elemSize, u8* mem, const size_t sizeofMem )
		: m_Pool( 0 )
		, m_Size( 0 )
		, m_FreeCount( 0 )
		, m_ElemSize( elemSize )
		, m_FirstFree( 0 )
#if __DEV
		, m_PeakFreeCount( 0 )
#endif // __DEV
		, m_OwnMem( false )
#if RAGE_POOL_SPILLOVER
		, m_SpilloverPool(NULL), m_SpilloverSize(0), m_PoolSize(0)
#endif
	{
		ATL_POOL_FUNC();
		Init(mem, sizeofMem);
	}

	atPoolBase(datResource& rsc) 
	: m_Pool(rsc)
	, m_FirstFree(rsc)
#if RAGE_POOL_SPILLOVER
	, m_SpilloverPool(NULL), m_SpilloverSize(0), m_PoolSize(0)
#endif
	{

	}


    // PURPOSE: Destructor
    ~atPoolBase()
    {
        this->ReleaseMem();
	}

	void Init(size_t size, bool useFlex = false);
	void Init(u8* mem, const size_t sizeofMem);
	void Init(size_t size, float spillover, bool useFlex = false);

    // PURPOSE: Resets all pool entries to free; O(N) operation
    void Reset();

    //PURPOSE
    //  Resets the pool to use caller-owned memory.
    void Reset( u8* mem, const size_t sizeofMem );

    // PURPOSE: Allocation function
    // RETURNS: Pointer to new object (unconstructed!).  Will assert out
    // or crash if no space is available; use IsFull to check if you want
    // to handle that yourself.
    // NOTES: Does NOT run constructor on the returned memory.  Most-recently
    // freed memory is returned to caller.  Runs in O(1) time.
    void* New(size_t ASSERT_ONLY(size) = 0
#if __ASSERT
		, const char* className = ""
#endif // __ASSERT
		);

	// PURPOSE: Check if pool is empty
	// RETURNS: True if pool is empty, else false
	bool IsInitialized() const { return m_Size > 0; }

	// PURPOSE: Check if pool is empty
	// RETURNS: True if pool is empty, else false
	bool IsEmpty() const { return m_FreeCount == m_Size; }

    // PURPOSE: Check if pool is full
    // RETURNS: True if pool is full, else false
    bool IsFull() const { return m_FreeCount == 0; }

    // RETURNS: Number of free entries in the pool
    size_t GetFreeCount() const { return m_FreeCount; }

	// RETURNS: Total number of entries in the pool
	size_t GetSize() const { return m_Size; }
	size_t GetPoolSize() const
	{ 
#if RAGE_POOL_SPILLOVER
		return m_SpilloverPool ? m_PoolSize : m_Size;
#else
		return m_Size;
#endif
	}

	// RETURNS: Total number of used entries in the pool
	size_t GetCount() const { return m_Size - m_FreeCount; }

	// RETURNS: Size of a pool entry
	size_t GetElemSize() const { return m_ElemSize; }

    // PURPOSE: Deallocation function
    // PARAMS: ptr - Pointer to storage previously allocated by New
    // NOTES: Does NOT run destructor on the memory.  Freeing the same
    // pointer twice will corrupt the free list, leading to mass destruction.
    // Runs in O(1) time.
    void Delete(void* ptr);

	// PURPOSE: Identify the index of an element so it can be accessed
	// via GetElemByIndex.
	// PARAMS: ptr - Pointer to storage previously allocated by New.
	// NOTE: The pointer MUST be allocated by New, or else the function
	// will assert and return an undefined value.
	u32 GetIndex(void* ptr) const;

	// Alias function for compatibility with fwPool
	u32 GetJustIndex(void* ptr) const	{ return GetIndex(ptr); }

	// PURPOSE: Return an element in this pool by an index. You will typically
	// NOT need this function. It will return whatever is at that index, regardless
	// of whether this element has actually been allocated via New() or not (or
	// subsequently deleted).
	// PARAMS: index - Index into the pool, preferably one that has been returned
	// by GetIndex.
	void* GetElemByIndex(u32 index);

	// PURPOSE: Is this pointer part of the pool memory
	bool IsInPool(const void* ptr) const;
	bool IsInPool(u32 index) const;

#if RAGE_POOL_SPILLOVER
	bool IsValidSpilloverPtr(const void* ptr) const;
	bool IsInSpillover(u32 index) const;
	size_t GetSpilloverSize() const { return m_SpilloverSize; }
#endif

	// Alias function for compatibility with fwPool
	size_t GetStorageSize() const			{ return m_ElemSize; }

	// Alias function for compatibility with fwPool
	size_t GetNoOfFreeSpaces() const		{ return this->m_FreeCount; }

	// Alias function for compatibility with fwPool
	size_t GetNoOfUsedSpaces() const		{ return this->GetCount(); }

#if !__FINAL
	size_t GetPeakSlotsUsed() const		{ return this->m_Size - this->m_PeakFreeCount; }
#endif // !__FINAL

#if __ASSERT
	// Not for use in production code, don't enable it.
	bool IsInFreeList(const void* ptr) const;
#endif	// __ASSERT

#if RAGE_POOL_NODE_TRACKING
	void Tally(void* ptr);
	void UnTally(void* ptr);
#endif

protected:
    void ReleaseMem();

protected:
    datRef<u8> m_Pool;
    size_t m_Size, m_FreeCount;
	size_t m_ElemSize;
    datRef<u8> m_FirstFree;

#if !__FINAL
	// This is the lowest value m_FreeCount ever had
	size_t m_PeakFreeCount;
#endif // !__FINAL

    //True if we own the memory pointed to be m_Pool;
    bool m_OwnMem;
	datPadding<3> m_Pad;

#if ATL_POOL_SPARSE
	void** m_SparsePool;
	atMap<const void*,u32> m_SparseMap;
#endif

#if RAGE_POOL_SPILLOVER
	void** m_SpilloverPool;
	u32 m_PoolSize, m_SpilloverSize;
	atMap<const void*, u32> m_SpilloverMap;	
#endif
};

// PURPOSE: Identify the index of an element so it can be accessed
// via GetElemByIndex.
// PARAMS: ptr - Pointer to storage previously allocated by New.
// NOTE: The pointer MUST be allocated by New, or else the function
// will assert and return an undefined value.
inline u32 atPoolBase::GetIndex(void* ptr) const
{
#if ATL_POOL_SPARSE
	if (m_SparsePool)
		return *m_SparseMap.Access(ptr);
#endif // ATL_POOL_SPARSE

#if RAGE_POOL_SPILLOVER
	if (!IsInPool(ptr))
	{
		const u32* pIndex = m_SpilloverMap.Access(ptr);
		Assertf(pIndex, "Spillover pointer index for %p is NULL! The game will crash now.", ptr);
		return *pIndex;
	}		
#endif // RAGE_POOL_SPILLOVER

	FastAssert(IsInPool(ptr));
	size_t result = (size_t) ptr - (size_t) m_Pool.ptr;
	result /= m_ElemSize;

	return (u32) result;
}

// PURPOSE: Return an element in this pool by an index. You will typically
// NOT need this function. It will return whatever is at that index, regardless
// of whether this element has actually been allocated via New() or not (or
// subsequently deleted).
// PARAMS: index - Index into the pool, preferably one that has been returned
// by GetIndex.
inline void* atPoolBase::GetElemByIndex(u32 index)
{
	FastAssert(index < m_Size);

#if ATL_POOL_SPARSE
	if (m_SparsePool)
		return m_SparsePool[index];
#endif // ATL_POOL_SPARSE

#if RAGE_POOL_SPILLOVER
	if (IsInSpillover(index))
		return m_SpilloverPool[index - m_PoolSize];
#endif // RAGE_POOL_SPILLOVER

	return ((u8*) m_Pool) + (index * m_ElemSize);
}

inline bool atPoolBase::IsInPool(const void* ptr) const 
{
#if ATL_POOL_SPARSE
	if (m_SparsePool)
		return m_SparseMap.Access(ptr) != NULL;
#endif // ATL_POOL_SPARSE

	return (ptr >= m_Pool) && (ptr < (m_Pool + (GetPoolSize() * m_ElemSize)));
}

inline bool atPoolBase::IsInPool(u32 index) const
{
	return index < GetPoolSize();
}

#if RAGE_POOL_SPILLOVER
inline bool atPoolBase::IsValidSpilloverPtr(const void* ptr) const
{
	return m_SpilloverPool && (m_SpilloverMap.Access(ptr) != NULL);
}

inline bool atPoolBase::IsInSpillover(u32 index) const
{
	return m_SpilloverPool && (index >= m_PoolSize) && (index < m_Size);
}
#endif

template <class _Type>
class atPool : public atPoolBase
{
	typedef atPool<_Type> _ThisType;

public:
	// PURPOSE: Constructor
    atPool() : atPoolBase( sizeof(_Type) )
    {
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)
    }

    // PURPOSE: Constructor
    // PARAMS: size - number of elements of the type to allocate space for
	atPool(size_t size, bool useFlex = false) : atPoolBase( sizeof( _Type ) )
    {
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)

		atPoolBase::Init(size, useFlex);
    }

	atPool(size_t size, float spillover, bool useFlex = false) : atPoolBase( sizeof( _Type ) )
	{
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)

		atPoolBase::Init(size, spillover, useFlex);
	}

    //PURPOSE
    //  Constructs a pool with caller-owned memory
    //PARAMS
    //  mem         - Caller-owned memory.
    //  sizeofMem   - Number of bytes of caller-owned memory.
    atPool( u8* mem, size_t sizeofMem ) : atPoolBase( sizeof( _Type ) )
    {
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)

		atPoolBase::Init(mem, sizeofMem);
    }

	atPool(datResource& rsc) : atPoolBase(rsc)
	{
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)

		// needs a lot of stack space because we have to find out which items are allocated and which aren't.
		if (m_Pool && m_Size)
		{
			u32* bits = Alloca(u32, (m_Size / 32)+1);
			atUserBitSet freePoolSlots(bits, m_Size);
			freePoolSlots.Reset();

			void* current = m_FirstFree;
			while(current)
			{
				size_t whichElem = (size_t)(((size_t)current - (size_t)m_Pool) / m_ElemSize);
				freePoolSlots.Set(whichElem);
				rsc.PointerFixup(*(void**)current);
				current = *((void**)current); // move to the next one
			}

			// now that we know which ones are free, Place() all of the others.
			for(int i = 0; i < m_Size; i++)
			{
				if (freePoolSlots.IsClear(i))
				{
					// this one is used
					_Type& elem = ((_Type*)(m_Pool))[i];
					elem.Place(&elem, rsc);
				}
			}
		}
	}

	// Compatibility with fwPool
	atPool(const char* /*pName*/, s32 /*redZone*/=0, s32 storageSize=sizeof(_Type)) : atPoolBase(storageSize)
	{
		Assert(storageSize == sizeof(_Type));
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)
	}

	// Compatibility with fwPool
	atPool(s32 nSize, const char* /*pName*/, s32 /*redZone*/=0, s32 ASSERT_ONLY(storageSize)=sizeof(_Type)) : atPoolBase( sizeof( _Type ) )
	{
		Assert(storageSize == sizeof(_Type));
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)

		atPoolBase::Init((size_t) nSize);
	}

	atPool(s32 nSize, float spillover, const char* /*pName*/, s32 /*redZone*/=0, s32 ASSERT_ONLY(storageSize)=sizeof(_Type)) : atPoolBase( sizeof( _Type ) )
	{
		Assert(storageSize == sizeof(_Type));
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)

		atPoolBase::Init((size_t) nSize, spillover);
	}

	// Compatibility with wtPool
	atPool(s32 nSize, _Type* pPool, u8* /*pFlags*/, const char* /*pName*/, s32 /*redZone*/=0, s32 storageSize=sizeof(_Type)) : atPoolBase(storageSize, (u8 *) pPool, sizeof(_Type) * nSize)
	{
		Assert(storageSize == sizeof(_Type));
		RAGE_POOL_TRACKING_ONLY(RegisterInPoolTracker(this, typeid(_Type).name());)
	}

	~atPool()
	{
		RAGE_POOL_TRACKING_ONLY(PoolTracker::Remove(this);)
	}

    // PURPOSE: Allocation function
    // RETURNS: Pointer to new object (unconstructed!).  Will assert out
    // or crash if no space is available; use IsFull to check if you want
    // to handle that yourself.
    // NOTES: Does NOT run constructor on the returned memory.  Most-recently
    // freed memory is returned to caller.  Runs in O(1) time.
    _Type* New(size_t size = 0) 
	{
		CompileTimeAssert( sizeof( _Type ) >= sizeof( _Type* ) );
		return static_cast<_Type*>(atPoolBase::New(size
#if __ASSERT
		, GetClassName()
#endif // __ASSERT
		));
    }

	// PURPOSE: Allocation function
	// RETURNS: Pointer to newly constructed object.  Will assert out
	// or crash if no space is available; use IsFull to check if you want
	// to handle that yourself.
	// NOTES: Runs in O(1) time.
	_Type* Construct(size_t size = 0) 
	{
		_Type* result = New(size);
		rage_placement_new(result) _Type;
		return result;
	}

	// PURPOSE: Return an element in this pool by an index. You will typically
	// NOT need this function. It will return whatever is at that index, regardless
	// of whether this element has actually been allocated via New() or not (or
	// subsequently deleted).
	// PARAMS: index - Index into the pool, preferably one that has been returned
	// by GetIndex.
	_Type* GetElemByIndex(u32 index) 
	{
		return static_cast<_Type*>(atPoolBase::GetElemByIndex(index));
	}

	// PURPOSE: Deallocation function; destructs object and frees memory
	// PARAMS: ptr - Pointer to storage previously allocated by New
	// NOTES: Runs in O(1) time.
	void Destruct(_Type* ptr) 
	{
		if (ptr) 
		{
			ptr->~_Type();
			Delete(ptr);
		}
	}

	// RETURNS: Number of bytes necessary for garbage collection state
	int GetGarbageCollectSize() const 
	{
		return (atPoolBase::m_Size + 7) >> 3;
	}

	// PURPOSE: Begins a garbage collection phase
	// PARAMS:  freed - Pointer to GetGarbageCollectSize() bytes
	void BeginGarbageCollect(u8* freed) 
	{
		size_t byteSize = (atPoolBase::m_Size+7)>>3;

		// Traverse free list to figure out which entries are invalid
		// Objects must always be created in a state such that GetMark returns false.
		// SetMark will be called only during garbage collection, and EndGarbageCollect
		// will either delete unmarked entries or call SetMark(false) when done.
		for (size_t i = 0; i < byteSize; i++)
			freed[i] = 0;
		_Type* f = (_Type*)atPoolBase::m_FirstFree;
		while (f) 
		{
			int offset = f - (_Type*)atPoolBase::m_Pool;
			freed[offset>>3] |= (1 << (offset&7));
			f = *(_Type**) f;
		}
	}

	// PURPOSE: Ends a garbage collection phase
	// PARAMS:  freed - Pointer to GetSize() bool's that had already been passed to BeginGarbageCollect
	// RETURNS: Number of freed items
	int EndGarbageCollect(u8* freed) 
	{
		// Reclaim memory on untouched nodes.
		// Note that we intentionally do NOT invoke the destructor!
		_Type* f = (_Type*) atPoolBase::m_Pool;
		int nuked = 0;
		for (size_t i = 0; i < atPoolBase::m_Size; i++) 
		{
			if (!(freed[i>>3] & (1 << (i&7)))) 
			{
				if (!f[i].GetMark()) 
				{
					//                  Displayf("kill %p",f+i);
					Delete(f+i);
					++nuked;
				}
				else 
				{
					//                  Displayf("clear %p",f+i);
					f[i].SetMark(false);
				}
			}
			//          else
			//              Displayf("freed %p",f+i);
		}
		return nuked;
	}

	// EJ - Optimization
	//
#if __ASSERT || __TOOL || __RESOURCECOMPILER
#if defined(_CPPRTTI)
	const char* GetClassName() const		{ return typeid(_Type).name(); }
#else // defined(_CPPRTTI)
	const char* GetClassName() const		{ return "?"; }
#endif // defined(_CPPRTTI)
#endif
};

template <class _Type>
class atIteratablePool : public atPool<_Type>
{
public:
	atIteratablePool() : atPool<_Type>()
	{
	}

    atIteratablePool(size_t size) : atPool<_Type>(size)
    {
		m_AllocatedMap.Init(size);
    }

	// Compatibility with fwPool
	atIteratablePool(const char* /*pName*/, s32 /*redZone*/=0, s32 ASSERT_ONLY(storageSize)=sizeof(_Type)) : atPool<_Type>()
	{
		Assert(storageSize == sizeof(_Type));
	}

	// Compatibility with fwPool
	atIteratablePool(s32 nSize, const char* /*pName*/, s32 /*redZone*/=0, s32 ASSERT_ONLY(storageSize)=sizeof(_Type)) : atPool<_Type>((size_t) nSize)
	{
		Assert(storageSize == sizeof(_Type));
	}

	atIteratablePool(s32 nSize, float spillover, const char* /*pName*/, s32 /*redZone*/=0, s32 ASSERT_ONLY(storageSize)=sizeof(_Type)) : atPool<_Type>((size_t) nSize, spillover)
	{
		Assert(storageSize == sizeof(_Type));
	}

	// Compatibility with fwPool
	atIteratablePool(s32 nSize, _Type* pPool, u8* /*pFlags*/, const char* /*pName*/, s32 /*redZone*/=0, s32 ASSERT_ONLY(storageSize)=sizeof(_Type)) : atPool<_Type>((u8 *) pPool, sizeof(_Type) * nSize)
	{
		Assert(storageSize == sizeof(_Type));
		m_AllocatedMap.Init(nSize);
	}

	void Init(size_t size, bool useFlex = false)
	{
		atPoolBase::Init(size, useFlex);
		m_AllocatedMap.Init(static_cast<s32>(size));		
	}

	// Compatibility with fwPool
	void Init(s32 nSize, _Type* pPool, u8* /*pFlags*/)
	{
		atPool<_Type>::Init(pPool, nSize * sizeof(_Type));
		m_AllocatedMap.Init(nSize);
	}

	void Init(size_t nSize, float spillover, bool useFlex = false)
	{
		atPool<_Type>::Init(nSize, spillover, useFlex);
		m_AllocatedMap.Init(nSize);
	}

	_Type* New(size_t size = 0)
	{
		_Type* result = atPool<_Type>::New(size);
		int index = atPool<_Type>::GetIndex(result);
		m_AllocatedMap.Set(index);
		return result;
	}

    void Delete(_Type* ptr)
	{
		int index = atPool<_Type>::GetIndex(ptr);
		atPool<_Type>::Delete(ptr);
		m_AllocatedMap.Clear(index);
    }

	_Type* Construct(size_t size = 0) 
	{
		_Type* result = atPool<_Type>::Construct(size);
		int index = atPool<_Type>::GetIndex(result);
		m_AllocatedMap.Set(index);
		return result;
	}

	void Destruct(_Type* ptr)
	{
		if (ptr) 
		{
			int index = atPool<_Type>::GetIndex(ptr);
			atPool<_Type>::Destruct(ptr);
			m_AllocatedMap.Clear(index);
		}
	}

	int EndGarbageCollect(u8* freed) 
	{
		// Reclaim memory on untouched nodes.
		// Note that we intentionally do NOT invoke the destructor!
		_Type* f = (_Type*) atPool<_Type>::m_Pool;
		int nuked = 0;
		for (size_t i = 0; i < atPool<_Type>::GetPoolSize(); i++) 
		{
			if (!(freed[i>>3] & (1 << (i&7)))) 
			{
				if (!f[i].GetMark()) 
				{
					//                  Displayf("kill %p",f+i);
					Delete(f+i);
					++nuked;
				}
				else 
				{
					//                  Displayf("clear %p",f+i);
					f[i].SetMark(false);
				}
			}
			//          else
			//              Displayf("freed %p",f+i);
		}
		return nuked;
	}

	const _Type* GetSlot(s32 index) const
	{
		return static_cast<_Type*>(const_cast<atIteratablePool*>(this)->GetSlot(index));
	}

	_Type* GetSlot(s32 index)
	{
		Assertf(index < (s32) atPool<_Type>::m_Size, "Invalid atIteratablePool index: %d / %" SIZETFMT "d", index, atPool<_Type>::m_Size);
		return (index < (s32) atPool<_Type>::m_Size && m_AllocatedMap.IsSet(index)) ? atPool<_Type>::GetElemByIndex(index) : NULL;
	}

	bool GetIsUsed(s32 index) const
	{
		return m_AllocatedMap.IsSet(index);
	}

	bool GetIsFree(s32 index) const
	{
		return m_AllocatedMap.IsClear(index);
	}

	// PURPOSE: Resets all pool entries to free; O(N) operation
	// Clears allocated map
	void Reset()
	{
		atPool<_Type>::Reset();
		m_AllocatedMap.Reset();
	}

protected:
	// Each bit represents an entry. A set bit indicates that the index is in use.
	atBitSet m_AllocatedMap;
};

// Helper macros to implement class-local new and delete operations using
// a pointer to an atPool instance.
#define DECLARE_CLASS_NEW_DELETE(classname) \
public: static void SetCurrentPool(::rage::atPool<classname> *current) { sm_CurrentPool = current; } \
static ::rage::atPool<classname>* GetCurrentPool() { return sm_CurrentPool; } \
static void* operator new(size_t RAGE_NEW_EXTRA_ARGS_UNUSED); \
static void operator delete(void* ptr); \
private: static ::rage::atPool<classname>* sm_CurrentPool;  \
public:

#define IMPLEMENT_CLASS_NEW_DELETE(classname,errname) \
::rage::atPool<classname>* classname::sm_CurrentPool; \
void* classname::operator new(size_t size RAGE_NEW_EXTRA_ARGS_UNUSED) { if (sm_CurrentPool->IsFull()) Quitf(ERR_GEN_POOL_3,"%s pool is full",#errname); return sm_CurrentPool->New(size); } \
void classname::operator delete(void* ptr) { sm_CurrentPool->Delete((classname*)ptr); }

#if RAGE_POOL_TRACKING
	class atPoolBaseTracker : public PoolTracker 
	{
	public:
		virtual ~atPoolBaseTracker() { /*No op*/ }

		void SetPool(const atPoolBase *pool) { m_poolPointer = pool; }
		const atPoolBase * GetPool() const { return (const atPoolBase *) m_poolPointer; }

		void SetName(const char *name)		{ m_Name = name; }
		virtual const char* GetName() const { return m_Name.c_str(); }
		virtual const char* GetClassName() const { return GetName(); }

		virtual size_t GetStorageSize() const { return GetPool()->GetStorageSize(); }
		virtual s32 GetSize() const { return (s32)GetPool()->GetSize(); }
#if RAGE_POOL_SPILLOVER
		virtual s32 GetSpilloverSize() const { return (s32)GetPool()->GetSpilloverSize(); }
#endif
		virtual s32 GetNoOfUsedSpaces() const { return (s32)GetPool()->GetNoOfUsedSpaces(); }
		virtual s32 GetPeakSlotsUsed() const { return (s32)GetPool()->GetPeakSlotsUsed(); }
		virtual s32 GetNoOfFreeSpaces() const { return (s32)GetPool()->GetNoOfFreeSpaces(); }
		virtual s32 GetActualMemoryUse() const 
		{
			return	s32(sizeof(*GetPool()) // Size of base object
				//- TEMPPLATEPOOL_MAXNAMELEN * sizeof(char) // - size of name
				+ GetSize()*sizeof(u32)	// Flags
				+ GetSize()*GetPool()->GetStorageSize()); // Actual storage

		}		

	private:
		ConstString m_Name;
	};	

	void RegisterInPoolTracker(const atPoolBase* poolptr, const char *poolName);
	void UnregisterInPoolTracker(const atPoolBase* poolptr);

#else // RAGE_POOL_TRACKING

	__forceinline void RegisterInPoolTracker(const void *, const char * /*poolName*/) {};
	__forceinline void UnregisterInPoolTracker(const void *) {};

#endif // RAGE_POOL_TRACKING
}   // namespace rage

#endif
