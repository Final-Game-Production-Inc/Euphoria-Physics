// 
// atl/pool.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "pool.h"
#include "system/memmanager.h"
#include "system/memvisualize.h"

namespace rage
{
#if RAGE_POOL_TRACKING

#if __WIN32
#pragma warning(disable: 4073)
#pragma init_seg(lib)
#endif

	// Need the trackers array initialised before anything is added to it. Durango and PC sidestep the problem
	// with #pragma init_seg(lib), above, Orbis needs to use __attribute__((init_priority(101))) to force it
	// to be initialised as early as possible.
#if RSG_ORBIS
	#define TRACKER_INIT_PRIORITY __attribute__((init_priority(101)))
#else
	#define TRACKER_INIT_PRIORITY
#endif

	atArray<PoolTracker*> PoolTracker::s_trackers TRACKER_INIT_PRIORITY;

	void PoolTracker::Add(PoolTracker* pTracker)
	{
		s_trackers.PushAndGrow(pTracker);
	}

	void PoolTracker::Remove(const void* poolptr)
	{
		int poolidx = -1;
		PoolTracker* ptr = NULL;
		for(int i=0;i<s_trackers.GetCount();i++)
		{
			if( poolptr == s_trackers[i]->GetRawPoolPointer() )
			{
				poolidx = i;
				ptr = s_trackers[i];
				break;
			}
		}
		if( poolidx > -1 )
		{
			sysMemStartTemp();
			s_trackers.DeleteFast(poolidx);
			delete ptr;
			sysMemEndTemp();
		}
	}

	void RegisterInPoolTracker(const atPoolBase* poolptr, const char *poolName)
	{
		// HACK for early use of pools
		if (!sysMemAllocator_sm_Current)
			return;

		sysMemStartTemp();

		atPoolBaseTracker * pTracker = rage_new atPoolBaseTracker;
		pTracker->SetPool(poolptr);
		pTracker->SetName(poolName);
		PoolTracker::Add(pTracker);

		sysMemEndTemp();
	}

	void UnregisterInPoolTracker(const atPoolBase* poolptr)
	{
		PoolTracker::Remove(poolptr);
	}
#endif

	void atPoolBase::Init(size_t size, bool useFlex)
	{
		(void)useFlex;
		Assert(!m_Pool);

		m_Size = size;

		if (size > 0)
		{
			const size_t bytes = m_ElemSize * GetPoolSize();
#if __PPU
			if (!g_bDontUseResidualAllocator)
			{
				// EJ: Residual allocator
				if (g_pResidualAllocator)
					m_Pool = static_cast<u8*>(g_pResidualAllocator->RAGE_LOG_ALLOCATE(bytes, 16));
			}

			// EJ: Pool allocator
			if (COMMERCE_CONTAINER_ONLY(useFlex &&) sysMemManager::GetInstance().IsFlexHeapEnabled())
			{
				if (!m_Pool)
					m_Pool = static_cast<u8*>(sysMemManager::GetInstance().GetFlexAllocator()->RAGE_LOG_ALLOCATE(bytes, 16));
			}
#endif // __PPU

			// EJ: Default allocator
			if (!m_Pool)
				m_Pool = rage_new u8[bytes];
		}

		m_OwnMem = true;

		Reset();

#if ATL_POOL_SPARSE
		m_SparsePool = rage_new void*[m_Size];
		memset(m_SparsePool, 0, sizeof(void*)*m_Size);
#endif
	}

	void atPoolBase::Init(size_t size, float RAGE_POOL_SPILLOVER_ONLY(spillover), bool useFlex)
	{
		Assert(!m_Pool);

		(void)useFlex;		
		m_Size = size;

#if ATL_POOL_SPARSE
		m_SparsePool = NULL;
#endif

#if RAGE_POOL_SPILLOVER
		m_SpilloverSize = static_cast<size_t>(static_cast<float>(size) * spillover);
		m_PoolSize = size - m_SpilloverSize;

		sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_GAME_VIRTUAL);
		Assert(pAllocator);

		m_SpilloverPool = (void**) pAllocator->Allocate(m_SpilloverSize * sizeof(void*), 16);
		memset(m_SpilloverPool, 0, (sizeof(void*) * m_SpilloverSize));
#endif

		if (GetPoolSize() > 0)
		{
			const size_t bytes = m_ElemSize * GetPoolSize();
#if __PPU
			if (!g_bDontUseResidualAllocator)
			{
				// EJ: Residual allocator
				if (g_pResidualAllocator)
					m_Pool = static_cast<u8*>(g_pResidualAllocator->RAGE_LOG_ALLOCATE(bytes, 16));
			}

			// EJ: Pool allocator
			if (COMMERCE_CONTAINER_ONLY(useFlex &&) sysMemManager::GetInstance().IsFlexHeapEnabled())
			{
				if (!m_Pool)
					m_Pool = static_cast<u8*>(sysMemManager::GetInstance().GetFlexAllocator()->RAGE_LOG_ALLOCATE(bytes, 16));
			}
#endif // __PPU

			// EJ: Default allocator
			if (!m_Pool)
				m_Pool = rage_new u8[bytes];
		}

		m_OwnMem = true;

		Reset();
	}

	void atPoolBase::Init(u8* mem, const size_t sizeofMem)
	{
		Assert(!m_Pool);
		this->Reset( mem, sizeofMem );

#if ATL_POOL_SPARSE
		m_SparsePool = NULL;
#endif

#if RAGE_POOL_SPILLOVER
		m_SpilloverPool = NULL;
		m_SpilloverSize = 0;
#endif
	}

	// PURPOSE: Resets all pool entries to free; O(N) operation
	void atPoolBase::Reset()
	{
#if ATL_POOL_CHECK_STOMPS
		//Trash the memory
		IF_DEBUG_MEMORY_FILL_N(::memset(m_Pool ,0xDF, m_ElemSize * GetPoolSize()), DMF_GAMEPOOL);
#endif
		u8* p = m_Pool;
		m_FirstFree = (u8 *) p;
		m_FreeCount = m_Size;
#if !__FINAL
		m_PeakFreeCount = m_Size;
#endif // !__FINAL
		size_t count = GetPoolSize();
		while (--count) 
		{
			*(void**)p = (p + m_ElemSize);
			p += m_ElemSize;
		}
		*(void**)p = 0;

#if RAGE_POOL_SPILLOVER
		if (m_SpilloverPool)
		{
			sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_GAME_VIRTUAL);
			Assert(pAllocator);

			for (u32 i = 0; i < m_SpilloverSize; ++i)
			{
				const void* ptr = m_SpilloverPool[i];
				if (ptr)
					pAllocator->Free(ptr);
			}

			memset(m_SpilloverPool, 0, (sizeof(void*) * m_SpilloverSize));
			m_SpilloverMap.Kill();
		}
#endif

#if ATL_POOL_CHECK_STOMPS && 0
		// This is just a little extra code for the pools to check that they are initialized correctly.
		const u8* curPtr = reinterpret_cast<const u8*>(m_Pool);
		for(size_t j = 0; j < GetPoolSize(); ++j)
		{
			Assertf((j == (GetPoolSize() - 1) || (*reinterpret_cast<const u32*>(curPtr) == (u32)curPtr + m_ElemSize), "%p (%x) -> %x", curPtr, *reinterpret_cast<const u32 *>(curPtr), (u32)curPtr + m_ElemSize);
			curPtr += sizeof(void*);
			for(u32 i = sizeof(void*); i < m_ElemSize; ++i)
			{
				Assertf(*curPtr == 0xDF, "Failed memory initialization at %p (%d)!", curPtr, *curPtr);
				++curPtr;
			}
		}
#endif
	}

	//PURPOSE
	//  Resets the pool to use caller-owned memory.
	void atPoolBase::Reset( u8* mem, const size_t sizeofMem )
	{
		this->ReleaseMem();

		m_Size = size_t( sizeofMem / m_ElemSize );
		FastAssert( m_Size > 0 );
		m_Pool = mem;

		m_OwnMem = false;

		this->Reset();
	}

	void atPoolBase::ReleaseMem()
	{
		if( m_OwnMem )
		{
#if __PPU
			if (g_pResidualAllocator && g_pResidualAllocator->IsValidPointer(m_Pool))
				g_pResidualAllocator->Free(m_Pool);
			else if (sysMemManager::GetInstance().GetFlexAllocator()->IsValidPointer(m_Pool))
				sysMemManager::GetInstance().GetFlexAllocator()->Free(m_Pool);
			else
				delete[] m_Pool;
#else
			delete[] m_Pool;
#endif // __PPU			
		}

		m_Pool = 0;
		m_Size = 0;
		m_FreeCount = 0;
		m_FirstFree = 0;
		m_OwnMem = false;

#if RAGE_POOL_SPILLOVER
		if (m_SpilloverPool)
		{
			sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_GAME_VIRTUAL);
			Assert(pAllocator);

			for (u32 i = 0; i < m_SpilloverSize; ++i)
			{
				const void* ptr = m_SpilloverPool[i];
				if (ptr)
					pAllocator->Free(ptr);
			}
			
			pAllocator->Free(m_SpilloverPool);
			m_SpilloverPool = NULL;
			m_SpilloverMap.Kill();
			m_SpilloverSize = 0;
		}
#endif
	}

#if RAGE_POOL_NODE_TRACKING
	void atPoolBase::Tally(void* ptr)
	{
		if (ptr && ::rage::diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasMisc())
			::rage::diagTracker::GetCurrent()->Tally(ptr, m_ElemSize, 0);
	}

	void atPoolBase::UnTally(void* ptr)
	{
		if (ptr && ::rage::diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasMisc())
			::rage::diagTracker::GetCurrent()->UnTally(ptr, m_ElemSize);
	}
#endif

	// PURPOSE: Allocation function
	// RETURNS: Pointer to new object (unconstructed!).  Will assert out
	// or crash if no space is available; use IsFull to check if you want
	// to handle that yourself.
	// NOTES: Does NOT run constructor on the returned memory.  Most-recently
	// freed memory is returned to caller.  Runs in O(1) time.
	void* atPoolBase::New(size_t ASSERT_ONLY(size) /*= 0*/
#if __ASSERT
		, const char* className /*= ""*/
#endif // __ASSERT
		) 
	{
#if !__FINAL
		m_PeakFreeCount = Min(m_PeakFreeCount, m_FreeCount-1);
#endif // !__FINAL

		FastAssert(!size || size<=m_ElemSize);

#if __ASSERT || RAGE_POOL_SPILLOVER
			if (!m_FirstFree)
			{
#if RAGE_POOL_SPILLOVER
				if (m_SpilloverPool)
				{
					size_t count = m_SpilloverSize;
					size_t index = m_Size - m_FreeCount;

					for (; count; count--, index++)
					{
						if (index == m_Size)
							index = m_PoolSize;

						const int i = index - m_PoolSize;

						if (!m_SpilloverPool[i])
						{
							RAGE_POOL_SPILLOVER_NOTIFY_ONLY(Assertf(0, "atPool is full (class=%s, element size=%d, pool size=%d) - Spilling over...", className, (int) m_ElemSize, (int) m_PoolSize);)
							sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_GAME_VIRTUAL);
							Assert(pAllocator);

							void* ptr = pAllocator->Allocate(m_ElemSize, 16);
							m_SpilloverMap.Insert(ptr, (u32)index);							
							m_SpilloverPool[i] = ptr;
							--m_FreeCount;
							return ptr;
						}
					}
				}		
#endif
				ASSERT_ONLY(Quitf("atPool is full (class=%s, element size=%d, pool size=%d)", className, (int) m_ElemSize, (int) m_Size);)
			}
#endif // __ASSERT || RAGE_POOL_SPILLOVER

#if ATL_POOL_SPARSE
			if(m_SparsePool)
			{
				size_t size = m_Size;
				size_t count = m_Size;
				size_t index = m_Size - m_FreeCount;
				for(; count; count--, index++)
				{
					if(index == size) index = 0;
					if(!m_SparsePool[index])
					{
						sysMemStartTemp();
						void* ptr = rage_new u8[m_ElemSize];
						m_SparseMap.Insert(ptr, (u32)index);
						sysMemEndTemp();
						m_SparsePool[index] = ptr;
						--m_FreeCount;
						return ptr;
					}
				}
				return NULL;
			}
#endif // __ATL_POOL_SPARSE

			void* result = m_FirstFree;

#if ATL_POOL_CHECK_STOMPS
			const u8* curPtr = reinterpret_cast<const u8*>(result);
			const u8* dataAtCurPtrAsU8Ptr = *reinterpret_cast<const u8* const*>(curPtr);
			Assertf(dataAtCurPtrAsU8Ptr == NULL || dataAtCurPtrAsU8Ptr >= reinterpret_cast<const u8 *>(m_Pool), "Next pointer has been stomped! (%p)", dataAtCurPtrAsU8Ptr);
			Assertf(dataAtCurPtrAsU8Ptr == NULL || dataAtCurPtrAsU8Ptr < reinterpret_cast<const u8 *>(m_Pool) + m_ElemSize * m_Size, "Next pointer has been stomped! (%p)", dataAtCurPtrAsU8Ptr);
			curPtr += sizeof(void*);
			for(u32 i = sizeof(void*); i < m_ElemSize; ++i)
			{
				if(*curPtr != 0xDF)
				{
					Displayf("Memory at %p (%p, %d) has been stomped with %d!", curPtr, result, i, *curPtr);
					Assert(false);
				}
				++curPtr;
			}
#endif
			m_FirstFree = *(u8 **) m_FirstFree.ptr;
			--m_FreeCount;

			RAGE_POOL_NODE_TRACKING_ONLY(Tally(result);)

			return result;
	}

	// PURPOSE: Deallocation function
	// PARAMS: ptr - Pointer to storage previously allocated by New
	// NOTES: Does NOT run destructor on the memory.  Freeing the same
	// pointer twice will corrupt the free list, leading to mass destruction.
	// Runs in O(1) time.
	void atPoolBase::Delete(void* ptr)
	{
		if (ptr)
		{
#if RAGE_POOL_SPILLOVER
			if (m_SpilloverPool)
			{
				if (!IsInPool(ptr))
				{
					const u32* pIndex = m_SpilloverMap.Access(ptr);
					Assertf(pIndex, "Spillover pointer index for %p is NULL! The game will crash now.", ptr);

					const u32 index = *pIndex;
					m_SpilloverPool[index - m_PoolSize] = NULL;
					m_SpilloverMap.Delete(ptr);

					sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_GAME_VIRTUAL);
					Assert(pAllocator);

					pAllocator->Free(ptr);
					++m_FreeCount;
					return;
				}
			}						
#endif
			RAGE_POOL_NODE_TRACKING_ONLY(UnTally(ptr));

#if ATL_POOL_SPARSE
			if(m_SparsePool)
			{
				u32 index = *m_SparseMap.Access(ptr);
				sysMemStartTemp();
				delete [] (char*) ptr;
				m_SparseMap.Delete(ptr);
				sysMemEndTemp();
				m_SparsePool[index] = NULL;
				++m_FreeCount;
				return;
			}
#endif // ATL_POOL_SPARSE

			FastAssert(IsInPool(ptr));

			//Trash the memory
			IF_DEBUG_MEMORY_FILL_N(::memset(ptr,0xDF,m_ElemSize),DMF_GAMEPOOL);
			*(u8**)ptr = m_FirstFree;
			m_FirstFree = (u8 *) ptr;
			++m_FreeCount;
		}
	}


#if __ASSERT
	// Not for use in production code, don't enable it.
	bool atPoolBase::IsInFreeList(const void* ptr) const
	{
		const void *freeEntry = m_FirstFree;
		while(freeEntry != NULL)
		{
			if(freeEntry == ptr)
			{
				return true;
			}
			freeEntry = *(reinterpret_cast<const void* const*>(freeEntry));
		}

		return false;
	}
#endif	// __ASSERT
}
