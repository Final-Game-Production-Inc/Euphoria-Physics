// 
// phcore/pool.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHCORE_POOL_H 
#define PHCORE_POOL_H 

#include "system/criticalsection.h"
#include "system/spinlock.h"
#include "constants.h"
#include "bitvector.h"

#if __BANK
#include "bank/bank.h"
#endif

// Enable for massive spew of pool usage
#if 0
#if __SPU
#define POOL_PRINTF(X) spu_printf X
#define REPORT_POOL_COUNT spu_printf("objects available in mm %d\n", mmPoolCount)
#else
#define POOL_PRINTF(X) Displayf X
#define REPORT_POOL_COUNT Displayf("objects available in mm %d\n", mmPoolCount)
#endif
#else
#define POOL_PRINTF(X)
#define REPORT_POOL_COUNT
#endif

#define PHPOOL_DEBUG_MEMORY_FILL __DEV
#define PHPOOL_DEBUG_MEMORY_FILL_BYTE 0xcd

namespace rage {

template <class T>
class phPool
{
public:
	struct SpuInitParams
	{
		sysSpinLockToken* allocToken;
		T * objects;
		T** pool;
		int* poolCount;
		int maxPoolCount;
		int highPriorityCount;
		bool* outOfObjects;
#if PHPOOL_EXTRA_VERIFICATION
		BitVectorBase AllocTable;
#endif // PHPOOL_EXTRA_VERIFICATION
	};

#if !__SPU

	// PURPOSE
	//    Construct the pool in main memory.
	// PARAMS
	//    numobjects - The maximum number of objects that can be in use on a particular frame.
	phPool(int numObjects);
	~phPool();

	void SetHighPriorityCount(int highPriorityCount);

	int GetInUseCount();
	int GetAvailCount();

	PPU_ONLY(void FillInSpuInitParams(SpuInitParams& initParams);)

	T* Allocate(bool highPriority = false);

	void Release(T* object);

	u32 GetIndex(const T* object) const;
	T* GetObject(u32 index);

	T** GetPool() 
	{ 
		return m_Pool; 
	}

	BANK_ONLY(static void AddWidgets(bkBank& bank);)

	OUTPUT_ONLY(bool RanOutOfObjects() { return m_OutOfObjects; } )
	OUTPUT_ONLY(void AssertIfOutOfObjects();)
	OUTPUT_ONLY(void DumpObjects();)

#if USE_DETERMINISTIC_ORDERING
	class SlotPredicate : public std::binary_function<T *, T *, bool>
	{
	public:
		bool operator()(T * left, T * right) const
		{
			return (left < right);
		}
	};

	void SortFreeSlots()
	{
#if !__PS3
		m_AllocToken.Lock();
#else
		POOL_PRINTF(("ppu allocate : unlock token now at 0x%p\n", &m_AllocToken));
		sysSpinLockLock(m_AllocToken);
		POOL_PRINTF(("ppu allocate : unlock token complete at 0x%p\n", &m_AllocToken));
#endif
		std::sort(m_Pool, m_Pool + m_CurrentCount, SlotPredicate());
#if !__PS3
		m_AllocToken.Unlock();
#else
		POOL_PRINTF(("ppu allocate : unlock token now at 0x%p\n", &m_AllocToken));
		sysSpinLockUnlock(m_AllocToken);
		POOL_PRINTF(("ppu allocate : unlock token complete at 0x%p\n", &m_AllocToken));
#endif
	}
#endif // USE_DETERMINISTIC_ORDERING

#else // __SPU

	static T* Allocate(bool highPriority = false);
	static void Release(T* object);

	static void InitSpu(SpuInitParams& initParams);
	static void ShutdownSpu();

#endif // __SPU

#if __SPU

#if __ASSERT
	static void VerifyObject(const T * newObject)
	{
		const ptrdiff_t offset = (ptrdiff_t)newObject - (ptrdiff_t)s_MmObjects;
		(void)offset;
		FastAssert((offset >= 0) && (offset / (ptrdiff_t)sizeof(T) < (ptrdiff_t)s_MaxPoolCount) && (offset % sizeof(T) == 0));
	}
#else // __ASSERT
	static __forceinline void VerifyObject(const T *) {}
#endif // __ASSERT

#if PHPOOL_EXTRA_VERIFICATION
	static u32 GetAllocated(const T * object, const int DMA_TAG_ID)
	{
		VerifyObject(object);
		const ptrdiff_t index = object - s_MmObjects;
		return s_AllocTable.GetBitDMA((u32)index,DMA_TAG_ID);
	}
	static u32 SetAllocated(const T * object, const bool allocated, const int DMA_TAG_ID)
	{
		VerifyObject(object);
		const ptrdiff_t index = object - s_MmObjects;
		return s_AllocTable.SetBitDMA((u32)index,allocated,DMA_TAG_ID);
	}
#endif // PHPOOL_EXTRA_VERIFICATION

#else // __SPU

#if __ASSERT
	void VerifyObject(const T * newObject) const
	{
		const ptrdiff_t offset = (ptrdiff_t)newObject - (ptrdiff_t)m_Objects;
		(void)offset;
		FastAssert((offset >= 0) && (offset / (ptrdiff_t)sizeof(T) < (ptrdiff_t)m_MaxCount) && (offset % sizeof(T) == 0));
	}
#else // __ASSERT
	__forceinline void VerifyObject(const T *) const {}
#endif // __ASSERT

#if PHPOOL_EXTRA_VERIFICATION
	u32 GetAllocated(const T * object) const
	{
		VerifyObject(object);
		const ptrdiff_t index = object - m_Objects;
		return m_AllocTable.GetBit((u32)index);
	}
	u32 SetAllocated(const T * object, const bool allocated)
	{
		VerifyObject(object);
		const ptrdiff_t index = object - m_Objects;
		return m_AllocTable.SetBit((u32)index,allocated);
	}
#endif // PHPOOL_EXTRA_VERIFICATION

#endif // __SPU

#if __SPU
	phPool() {}

	T ** GetPool_EA() const
	{
		const int TAG_ID = 17;
		T ** Pool_EA = reinterpret_cast<T**>(cellDmaGetUint32((uint64_t)&m_Pool, DMA_TAG(TAG_ID), 0, 0));
		return Pool_EA;
	}

	T * AllocateSPU(T ** Pool_EA)
	{
		const int TAG_ID = 17;
		sysSpinLockLock(m_AllocToken);
		int CurrentCount = cellDmaGetUint32((uint64_t)&m_CurrentCount, DMA_TAG(TAG_ID), 0, 0);
		TrapLT(CurrentCount, 0);
		T * newObject;
		if (Likely(CurrentCount > 0))
		{
			CurrentCount--;
			//T ** Pool_EA = reinterpret_cast<T**>(cellDmaGetUint32((uint64_t)&m_Pool, DMA_TAG(TAG_ID), 0, 0));
			newObject = reinterpret_cast<T*>(cellDmaGetUint32((uint64_t)(Pool_EA+CurrentCount), DMA_TAG(TAG_ID), 0, 0));
			cellDmaPutUint32(CurrentCount, (uint64_t)&m_CurrentCount, DMA_TAG(TAG_ID), 0, 0);
		}
		else
			newObject = NULL;
		sysSpinLockUnlock(m_AllocToken);
		return newObject;
	}
#endif // __SPU

private:
	phPool(const phPool&) { Quitf(ERR_PHY_POOL_1,"Copy construction of phPool is an error."); }
	const phPool& operator=(const phPool&) { Quitf(ERR_PHY_POOL_2,"Assignment of phPool is an error."); return *this; }

	int m_CurrentCount ;
	T* m_Objects;
	T** m_Pool;

#if PHPOOL_EXTRA_VERIFICATION
	BitVectorAllocated m_AllocTable;
#endif // PHPOOL_EXTRA_VERIFICATION

#if !__PS3
	// On Win32 a critical section is nicer than a spinlock because it is less prone to race conditions, since 360
	// threads can be preempted.
	typedef sysCriticalSection AllocLock;
	typedef sysCriticalSectionToken AllocLockToken;
#else
	// On PPU we use a spin lock so that it can be shared between SPU and PPU
	typedef sysSpinLock AllocLock;
	typedef sysSpinLockToken AllocLockToken;
#endif

	AllocLockToken m_AllocToken;

	static __THREAD u32 s_AllocTokenRefs;

	int m_MaxCount;
	int m_HighPriorityCount;

	bool m_OutOfObjects;

	BANK_ONLY(static int sm_SmallestPoolSize;)

#if __SPU
	static int* s_MmPoolCount;
	static int s_LastMmPoolCount;
	static sysSpinLockToken* s_AllocToken;
	static T* s_MmObjects;
	static T** s_MmPool;
	static int s_MaxPoolCount;
	static int s_HighPriorityCount;
	static bool* s_MmOutOfObjects;
#if PHPOOL_EXTRA_VERIFICATION
	static BitVectorBase s_AllocTable;
#endif // PHPOOL_EXTRA_VERIFICATION

	static const int LOCAL_STORE_POOL_BITS = 4;
	static const int LOCAL_STORE_POOL_SIZE = 1 << LOCAL_STORE_POOL_BITS;

	static T* s_LsPool[LOCAL_STORE_POOL_SIZE] ;
	static int s_LsPoolCount;

	static const unsigned int TAIL_BUFFER_BITS = 2;
	static const unsigned int TAIL_BUFFER_SIZE = 1 << TAIL_BUFFER_BITS;

	static T* s_TailBuffer[TAIL_BUFFER_SIZE] ;

#if PHPOOL_DEBUG_MEMORY_FILL && __SPU
	static u8 s_DebugMemoryFillBuffer[sizeof(T)] ;
#endif // PHPOOL_DEBUG_MEMORY_FILL
#endif // __SPU
};

template <class T> __THREAD u32 phPool<T>::s_AllocTokenRefs = 0;

#if !__SPU

template <class T>
phPool<T>::phPool(int numObjects)
	: m_Pool(rage_new T*[numObjects])
	, m_CurrentCount(numObjects)
	, m_MaxCount(numObjects)
	, m_HighPriorityCount(0)
	, m_OutOfObjects(0)
{
	Assert(numObjects < 65535);
#if __BANK
	sm_SmallestPoolSize = numObjects;
#endif

	m_Objects = (T*)rage_aligned_new(__alignof(T)) u8[sizeof(T) * numObjects];
#if PHPOOL_DEBUG_MEMORY_FILL
	sysMemSet((void*)m_Objects, PHPOOL_DEBUG_MEMORY_FILL_BYTE, sizeof(T) * numObjects);
#endif // PHPOOL_DEBUG_MEMORY_FILL

	for (int object = 0; object < numObjects; ++object)
	{
		m_Pool[object] = m_Objects + object;
	}

#if __PPU
	m_AllocToken = SYS_SPINLOCK_UNLOCKED;
#endif

#if PHPOOL_EXTRA_VERIFICATION
	m_AllocTable.Allocate(numObjects);
	m_AllocTable.SetAll(false);
#endif // PHPOOL_EXTRA_VERIFICATION
}

template <class T>
phPool<T>::~phPool()
{
	delete [] m_Pool;
	delete [] (u8*)m_Objects;
#if PHPOOL_EXTRA_VERIFICATION
	m_AllocTable.Free();
#endif // PHPOOL_EXTRA_VERIFICATION
}

template <class T>
void phPool<T>::SetHighPriorityCount(int highPriorityCount)
{
	Assert(highPriorityCount <= m_MaxCount);
	m_HighPriorityCount = highPriorityCount;
}

template <class T>
int phPool<T>::GetInUseCount()
{
	return m_MaxCount - m_CurrentCount;
}

template <class T>
int phPool<T>::GetAvailCount()
{
	return m_CurrentCount;
}

#if __PPU
template <class T>
void phPool<T>::FillInSpuInitParams(SpuInitParams& initParams)
{
	initParams.allocToken = &m_AllocToken;
	initParams.objects = m_Objects;
	initParams.pool = m_Pool;
	initParams.poolCount = &m_CurrentCount;
	initParams.maxPoolCount = m_MaxCount;
	initParams.highPriorityCount = m_HighPriorityCount;
	initParams.outOfObjects = &m_OutOfObjects;
#if PHPOOL_EXTRA_VERIFICATION
	initParams.AllocTable = m_AllocTable;
#endif // PHPOOL_EXTRA_VERIFICATION
}
#endif // __PPU

template <class T>
T* phPool<T>::Allocate(bool highPriority)
{
	if (s_AllocTokenRefs++ == 0)
	{
#if !__PS3
		m_AllocToken.Lock();
#else
		POOL_PRINTF(("ppu allocate : unlock token now at 0x%p\n", &m_AllocToken));
		sysSpinLockLock(m_AllocToken);
		POOL_PRINTF(("ppu allocate : unlock token complete at 0x%p\n", &m_AllocToken));
#endif
	}

	TrapLT(m_CurrentCount, 0);

	bool abort = false;

	if (highPriority == false && m_CurrentCount <= m_HighPriorityCount)
	{
		OUTPUT_ONLY(m_OutOfObjects = 1;)
		BANK_ONLY(sm_SmallestPoolSize = 0;)

		abort = true;
	}

	if (m_CurrentCount == 0)
	{
		OUTPUT_ONLY(m_OutOfObjects = 1;)
		BANK_ONLY(sm_SmallestPoolSize = 0;)
		
		abort = true;
	}

	T* newObject = NULL;
	
	if (!abort)
	{
		newObject = m_Pool[--m_CurrentCount];

		BANK_ONLY(sm_SmallestPoolSize = Min(sm_SmallestPoolSize, m_CurrentCount);)

		::new (newObject) T;
	}

#if PHPOOL_EXTRA_VERIFICATION
	if (Likely(newObject))
	{
		const u32 allocated = SetAllocated(newObject,true);
		if (Unlikely(allocated))
		{
			Displayf("Duplicate slot found in memory pool! 0x%p",newObject);
			__debugbreak();
		}
	}
#endif // PHPOOL_EXTRA_VERIFICATION

	if (--s_AllocTokenRefs == 0)
	{
#if !__PS3
		m_AllocToken.Unlock();
#else
		POOL_PRINTF(("ppu allocate : unlock token now at 0x%p\n", &m_AllocToken));
		sysSpinLockUnlock(m_AllocToken);
		POOL_PRINTF(("ppu allocate : unlock token complete at 0x%p\n", &m_AllocToken));
#endif
	}

	if (Likely(newObject))
		VerifyObject(newObject);

	return newObject;
}

template <class T>
void phPool<T>::Release(T* object)
{
	VerifyObject(object);

	object->AboutToBeReleased();

	if (s_AllocTokenRefs++ == 0)
	{
#if !__PS3
		m_AllocToken.Lock();
#else	
		POOL_PRINTF(("ppu release : unlock token now at 0x%p\n", &m_AllocToken));
		sysSpinLockLock(m_AllocToken);
		POOL_PRINTF(("ppu release : unlock token complete at 0x%p\n", &m_AllocToken));
#endif
	}

#if PHPOOL_EXTRA_VERIFICATION
	{
		const u32 allocated = SetAllocated(object,false);
		if (Unlikely(!allocated))
		{
			Displayf("Attempting to delete unallocated slot in memory pool! 0x%p",object);
			__debugbreak();
		}
	}
#endif // PHPOOL_EXTRA_VERIFICATION

	object->Reset();

#if PHPOOL_DEBUG_MEMORY_FILL
	sysMemSet((void*)object, PHPOOL_DEBUG_MEMORY_FILL_BYTE, sizeof(T));
#endif // PHPOOL_DEBUG_MEMORY_FILL

	// The pool had better not already be full.
	FastAssert(m_CurrentCount < m_MaxCount);

//	TrapGE(m_CurrentCount, m_MaxCount);

	// Non-composite case, we just release ourselves
	m_Pool[m_CurrentCount++] = object;

	if (--s_AllocTokenRefs == 0)
	{
#if !__PS3
		m_AllocToken.Unlock();
#else	
		POOL_PRINTF(("ppu release : unlock token now at 0x%p\n", &m_AllocToken));
		sysSpinLockUnlock(m_AllocToken);
		POOL_PRINTF(("ppu release : unlock token complete at 0x%p\n", &m_AllocToken));
#endif
	}
}

template <class T>
u32 phPool<T>::GetIndex(const T* object) const
{
	return (u32)(object - m_Objects);
}

template <class T>
T* phPool<T>::GetObject(u32 index)
{
	return &m_Objects[index];
}


#if !__NO_OUTPUT
template <class T>
void phPool<T>::AssertIfOutOfObjects()
{
	Assertf(!m_OutOfObjects, "Too many objects are colliding, could not allocate a %s! Please include the console log in bug reports.", T::GetClassName());
}

template <class T>
void phPool<T>::DumpObjects()
{
	static bool s_AlreadyDumped = false;

	if (!s_AlreadyDumped)
	{
		s_AlreadyDumped = true;

		Displayf("*************************START OBJECT DUMP*************************");
		for(int curIndex = 0; curIndex < m_MaxCount; ++curIndex)
		{
			m_Pool[curIndex]->DumpDueToRunningOut();
		}

		Displayf("*************************END OBJECT DUMP [%d tracked]*************************", m_MaxCount);
	}
}
#endif

#if __BANK
template <class T>
void phPool<T>::AddWidgets(bkBank& bank)
{
	bank.AddSlider("Minimum Pool Size", &sm_SmallestPoolSize, 0, 65535, 0);
}

template <class T> int phPool<T>::sm_SmallestPoolSize;
#endif

#else // __SPU

template <class T> int* phPool<T>::s_MmPoolCount;
template <class T> int phPool<T>::s_LastMmPoolCount;
template <class T> sysSpinLockToken* phPool<T>::s_AllocToken;
template <class T> T* phPool<T>::s_MmObjects;
template <class T> T** phPool<T>::s_MmPool;
template <class T> int phPool<T>::s_MaxPoolCount;
template <class T> int phPool<T>::s_HighPriorityCount;
template <class T> bool* phPool<T>::s_MmOutOfObjects;
#if PHPOOL_EXTRA_VERIFICATION
template <class T> BitVectorBase phPool<T>::s_AllocTable;
#endif // PHPOOL_EXTRA_VERIFICATION

template <class T> T* phPool<T>::s_LsPool[LOCAL_STORE_POOL_SIZE] ;
template <class T> int phPool<T>::s_LsPoolCount;

template <class T> T* phPool<T>::s_TailBuffer[TAIL_BUFFER_SIZE] ;

#if PHPOOL_DEBUG_MEMORY_FILL
template <class T> u8 phPool<T>::s_DebugMemoryFillBuffer[sizeof(T)] ;
#endif // PHPOOL_DEBUG_MEMORY_FILL

template <class T>
T* phPool<T>::Allocate(bool highPriority)
{
	POOL_PRINTF(("*** Allocate: %d in local mem before\n", s_LsPoolCount));

	if (s_LsPoolCount == 0)
	{
		// Go get some more objects from main memory
		if (s_AllocTokenRefs++ == 0)
		{
			POOL_PRINTF(("allocate : lock token now at 0x%p\n", s_AllocToken));
			sysSpinLockLock(*s_AllocToken);
			POOL_PRINTF(("allocate : lock token complete at 0x%p\n", s_AllocToken));
		}

		// Get the current pool count
		POOL_PRINTF(("allocate : get current pool count\n"));
		unsigned int mmPoolCount = cellDmaGetUint32((uint64_t)s_MmPoolCount, DMA_TAG(17), 0, 0);
		s_LastMmPoolCount = mmPoolCount;
		POOL_PRINTF(("allocate : get current pool count: complete\n"));
		REPORT_POOL_COUNT;

		POOL_PRINTF(("*** Allocate: %d in main mem before\n", mmPoolCount));

		//Assert(mmPoolCount > LOCAL_STORE_POOL_SIZE);

		if (mmPoolCount > 0)
		{
			// Compute the beginning of the block of object pointers we need
			int blockToGet = (mmPoolCount - 1) >> LOCAL_STORE_POOL_BITS;
			int firstObjectInBlock = blockToGet << LOCAL_STORE_POOL_BITS;
			s_LsPoolCount = mmPoolCount - firstObjectInBlock;
			mmPoolCount -= s_LsPoolCount;
			T** blockAddress = s_MmPool + firstObjectInBlock;

			// Grab the list of pointers, and write back the new count
			REPORT_POOL_COUNT;
			POOL_PRINTF(("allocate : put current pool count\n"));
			cellDmaLargeGet(s_LsPool, (uint64_t)blockAddress, s_LsPoolCount * sizeof(T*), DMA_TAG(18), 0, 0);
			cellDmaPutUint32(mmPoolCount, (uint64_t)s_MmPoolCount, DMA_TAG(17), 0, 0);
			s_LastMmPoolCount = mmPoolCount;
			cellDmaWaitTagStatusAll(DMA_MASK(18));
			POOL_PRINTF(("allocate : put current pool count: complete\n"));
		}

		POOL_PRINTF(("*** Allocate: %d in main mem after\n", mmPoolCount));

		if (--s_AllocTokenRefs == 0)
		{
			POOL_PRINTF(("allocate : unlock token now at 0x%p\n", s_AllocToken));
			sysSpinLockUnlock(*s_AllocToken);
			POOL_PRINTF(("allocate : unlock token complete at 0x%p\n", s_AllocToken));
		}
	}

	if (highPriority == false && s_LastMmPoolCount <= s_HighPriorityCount)
	{
		POOL_PRINTF(("out of high priority objects, request DENIED\n"));
		OUTPUT_ONLY(cellDmaPutUint8(1, (uint64_t)s_MmOutOfObjects, DMA_TAG(17), 0, 0);)
		return NULL;
	}

	if (s_LsPoolCount == 0)
	{
		POOL_PRINTF(("completely out of objects, request DENIED\n"));
		OUTPUT_ONLY(cellDmaPutUint8(1, (uint64_t)s_MmOutOfObjects, DMA_TAG(17), 0, 0);)
		return NULL;
	}

	POOL_PRINTF(("allocate : pop from stack\n"));

	POOL_PRINTF(("*** Allocate: %d in local mem after\n", s_LsPoolCount - 1));

	T* newObject = s_LsPool[--s_LsPoolCount];

#if PHPOOL_EXTRA_VERIFICATION
	if (Likely(newObject))
	{
		// TODO: create lock free SetAllocation.
		sysSpinLockLock(*s_AllocToken);
		const u32 allocated = SetAllocated(newObject,true,18);
		sysSpinLockUnlock(*s_AllocToken);
		if (Unlikely(allocated))
		{
			Displayf("Duplicate slot found in memory pool! 0x%p",newObject);
			__debugbreak();
		}
	}
#endif // PHPOOL_EXTRA_VERIFICATION

	POOL_PRINTF((" <<< Allocated 0x%08x >>>\n", (unsigned int)newObject));

	if (Likely(newObject))
		VerifyObject(newObject);

	return newObject;
}

template <class T>
void phPool<T>::Release(T* object)
{
	// Make sure we aren't trying to release an SPU LS pointer
	Assert((unsigned int)object > 256 * 1024);

	VerifyObject(object);

#if PHPOOL_EXTRA_VERIFICATION
	{
		// TODO: create lock free SetAllocation.
		sysSpinLockLock(*s_AllocToken);
		const u32 allocated = SetAllocated(object,false,18);
		sysSpinLockUnlock(*s_AllocToken);
		if (Unlikely(!allocated))
		{
			Displayf("Attempting to delete unallocated slot in memory pool! 0x%p",object);
			__debugbreak();
		}
		// Check for duplicates in local store list.
		for (int i = 0 ; i < s_LsPoolCount ; i++)
		{
			if (Unlikely(object == s_LsPool[i]))
			{
				Displayf("Attempting to delete unallocated slot in memory pool! 0x%p",object);
				__debugbreak();
			}
		}
	}
#endif // PHPOOL_EXTRA_VERIFICATION

#if PHPOOL_DEBUG_MEMORY_FILL
	sysDmaPut(s_DebugMemoryFillBuffer, (uint64_t)object, sizeof(T), DMA_TAG(20));
#endif // PHPOOL_DEBUG_MEMORY_FILL

	POOL_PRINTF((" <<< Released 0x%08x >>>\n", (unsigned int)object));

	if (s_LsPoolCount == LOCAL_STORE_POOL_SIZE)
	{
		// Local store object pool is already full, push some down to main memory
		if (s_AllocTokenRefs++ == 0)
		{
			POOL_PRINTF(("release : lock token now at 0x%p\n", s_AllocToken));
			sysSpinLockLock(*s_AllocToken);
			POOL_PRINTF(("release : unlock token complete at 0x%p\n", s_AllocToken));
		}

		// Get the current pool count
		POOL_PRINTF(("release : get current pool count\n"));
		unsigned int mmPoolCount = cellDmaGetUint32((uint64_t)s_MmPoolCount, DMA_TAG(17), 0, 0);
		s_LastMmPoolCount = mmPoolCount;
		POOL_PRINTF(("release : get current pool count: complete\n"));
		REPORT_POOL_COUNT;

		int blockToGet = 0;
		int firstObjectInBlock = 0;
		int blocksInTail = 0;
		T** blockAddress = s_MmPool;

		if (mmPoolCount > 0)
		{
			// Compute the beginning of the block of object pointers we need
			blockToGet = (mmPoolCount - 1) >> TAIL_BUFFER_BITS;
			firstObjectInBlock = blockToGet << TAIL_BUFFER_BITS;
			blocksInTail = mmPoolCount - firstObjectInBlock;
			blockAddress = s_MmPool + firstObjectInBlock;

			// Grab the list of pointers comprising the "tail"
			if (blocksInTail > 0)
			{
				POOL_PRINTF(("release : get tail objects\n"));
				cellDmaLargeGet(s_TailBuffer, (uint64_t)blockAddress, blocksInTail * sizeof(T*), DMA_TAG(18), 0, 0);
				cellDmaWaitTagStatusAll(DMA_MASK(18));
				POOL_PRINTF(("release : get tail objects: complete\n"));
			}
		}

		// Write back the tail one block over, so there is a block of space to write back some blocks from the pool
		if (blocksInTail > 0)
		{
			T** newBlockAddress = blockAddress + TAIL_BUFFER_SIZE;
			POOL_PRINTF(("release : put objects\n"));
			cellDmaLargePut(s_TailBuffer, (uint64_t)newBlockAddress, blocksInTail * sizeof(T*), DMA_TAG(18), 0, 0);
		}

		// Now write back some of the objects from our local pool into the empty block we created
		T** startOfPoolToSendBack = s_LsPool + LOCAL_STORE_POOL_SIZE - TAIL_BUFFER_SIZE;
		cellDmaLargePut(startOfPoolToSendBack, (uint64_t)blockAddress, TAIL_BUFFER_SIZE * sizeof(T*), DMA_TAG(19), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(18));
		cellDmaWaitTagStatusAll(DMA_MASK(19));
		POOL_PRINTF(("release : put objects: complete\n"));

		mmPoolCount += TAIL_BUFFER_SIZE;
		s_LsPoolCount -= TAIL_BUFFER_SIZE;

		REPORT_POOL_COUNT;
		Assert(mmPoolCount <= (u32)s_MaxPoolCount);
		POOL_PRINTF(("release : put current pool count\n"));
		cellDmaPutUint32(mmPoolCount, (uint64_t)s_MmPoolCount, DMA_TAG(17), 0, 0);
		s_LastMmPoolCount = mmPoolCount;
		POOL_PRINTF(("release : put current pool count: complete\n"));

		if (--s_AllocTokenRefs == 0)
		{
			POOL_PRINTF(("release : unlock token now at 0x%p\n", s_AllocToken));
			sysSpinLockUnlock(*s_AllocToken);
			POOL_PRINTF(("release : unlock token complete at 0x%p\n", s_AllocToken));
		}
	}

	POOL_PRINTF(("release : push to stack\n"));

	s_LsPool[s_LsPoolCount++] = object;

#if PHPOOL_DEBUG_MEMORY_FILL
	sysDmaWait(DMA_MASK(20));
#endif // PHPOOL_DEBUG_MEMORY_FILL
}

template <class T>
void phPool<T>::InitSpu(SpuInitParams& initParams)
{
	s_AllocToken = initParams.allocToken;
	s_AllocTokenRefs = 0;
	s_MmObjects = initParams.objects;
	s_MmPool = initParams.pool;
	s_MmPoolCount = initParams.poolCount;
	s_MaxPoolCount = initParams.maxPoolCount;
	s_HighPriorityCount = initParams.highPriorityCount;
	s_MmOutOfObjects = initParams.outOfObjects;
#if PHPOOL_EXTRA_VERIFICATION
	s_AllocTable = initParams.AllocTable;
#endif // PHPOOL_EXTRA_VERIFICATION
	s_LsPoolCount = 0;
	s_LastMmPoolCount = cellDmaGetUint32((uint64_t)initParams.poolCount, DMA_TAG(17), 0, 0);
#if PHPOOL_DEBUG_MEMORY_FILL
	sysMemSet(s_DebugMemoryFillBuffer, PHPOOL_DEBUG_MEMORY_FILL_BYTE, sizeof(T));
#endif // PHPOOL_DEBUG_MEMORY_FILL
}

template <class T>
void phPool<T>::ShutdownSpu()
{
	POOL_PRINTF(("*** Shutdown: %d in local mem before\n", s_LsPoolCount));

	if (s_LsPoolCount > 0)
	{
		// Give back the objects we allocated to main memory
		if (s_AllocTokenRefs++ == 0)
		{
			POOL_PRINTF(("shutdown : lock token now at 0x%p\n", s_AllocToken));
			sysSpinLockLock(*s_AllocToken);
			POOL_PRINTF(("shutdown : lock token complete at 0x%p\n", s_AllocToken));
		}

		// Get the current pool count
		POOL_PRINTF(("shutdown : get current pool count\n"));
		unsigned int mmPoolCount = cellDmaGetUint32((uint64_t)s_MmPoolCount, DMA_TAG(17), 0, 0);
		POOL_PRINTF(("shutdown : get current pool count: complete\n"));
		REPORT_POOL_COUNT;

		POOL_PRINTF(("*** Shutdown: %d in main mem before\n", mmPoolCount));

		// We have to temporarily allocate the last few pointers off the array, so that we can
		// keep all the DMA aligned and avoid leaving any holes in the array.
		unsigned int blockToGet = mmPoolCount >> TAIL_BUFFER_BITS;
		unsigned int firstObjectInBlock = blockToGet << TAIL_BUFFER_BITS;
		unsigned int blocksInTail = mmPoolCount - firstObjectInBlock;
		mmPoolCount -= blocksInTail;
		T** blockAddress = s_MmPool + firstObjectInBlock;

		// Grab the list of pointers comprising the "tail"
		if (blocksInTail > 0)
		{
			POOL_PRINTF(("shutdown : get tail objects\n"));
			cellDmaLargeGet(s_TailBuffer, (uint64_t)blockAddress, blocksInTail * sizeof(T*), DMA_TAG(18), 0, 0);
			cellDmaWaitTagStatusAll(DMA_MASK(18));
			POOL_PRINTF(("shutdown : get tail objects: complete\n"));
		}

		// Fill up the tail until we are out of objects, DMA'ing each time it fills up
		while (s_LsPoolCount > 0)
		{
			while (s_LsPoolCount > 0 && blocksInTail < TAIL_BUFFER_SIZE)
			{
				s_TailBuffer[blocksInTail++] = s_LsPool[--s_LsPoolCount];
				POOL_PRINTF(("shutdown : release to tail\n"));
			}

#if PHPOOL_EXTRA_VERIFICATION
			{
				for (unsigned int i = 0 ; i < blocksInTail ; i++)
				{
					T * object = s_TailBuffer[i];
					const u32 allocated = GetAllocated(object,18);
					if (Unlikely(allocated))
					{
						Displayf("Duplicate slot found in memory pool! 0x%p %d %d %d",object,i,blocksInTail,s_LsPoolCount);
						__debugbreak();
					}
				}
			}
#endif // PHPOOL_EXTRA_VERIFICATION

			POOL_PRINTF(("shutdown : write tail objects\n"));
			cellDmaLargePut(s_TailBuffer, (uint64_t)blockAddress, blocksInTail * sizeof(T*), DMA_TAG(18), 0, 0);
			cellDmaWaitTagStatusAll(DMA_MASK(18));
			POOL_PRINTF(("shutdown : write tail objects: complete\n"));

			blockAddress += blocksInTail;
			mmPoolCount += blocksInTail;
			blocksInTail = 0;
		}

		// Write back the new object pool count
		REPORT_POOL_COUNT;
		Assert(mmPoolCount <= (u32)s_MaxPoolCount);
		POOL_PRINTF(("shutdown : put current pool count\n"));
		cellDmaPutUint32(mmPoolCount, (uint64_t)s_MmPoolCount, DMA_TAG(17), 0, 0);
		POOL_PRINTF(("shutdown : put current pool count : complete\n"));

		POOL_PRINTF(("*** Shutdown: %d in main mem after\n", mmPoolCount));

		if (--s_AllocTokenRefs == 0)
		{
			POOL_PRINTF(("shutdown : unlock token now at 0x%p\n", s_AllocToken));
			sysSpinLockUnlock(*s_AllocToken);
			POOL_PRINTF(("shutdown : unlock token complete at 0x%p\n", s_AllocToken));
		}
	}

	POOL_PRINTF(("*** Shutdown: %d in local mem after\n", s_LsPoolCount));

	Assert(s_LsPoolCount == 0);
}

#endif // __SPU

} // namespace rage

#endif // PHCORE_POOL_H 
