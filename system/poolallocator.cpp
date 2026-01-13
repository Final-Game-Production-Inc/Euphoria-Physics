// 
// system/poolallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
// Eric J Anderson
// 
#include "math/simplemath.h"
#include "system/debugmemoryfill.h"
#include "system/poolallocator.h"

namespace rage
{
#if RAGE_POOL_TRACKING
	// Needs to be in the .cpp to prevent circular references, sigh.
	s32 sysMemPoolAllocatorTracker::GetSize() const { return (s32)GetAllocator()->GetSize(); }
	s32 sysMemPoolAllocatorTracker::GetNoOfUsedSpaces() const { return (s32)GetAllocator()->GetNoOfUsedSpaces(); }
	s32 sysMemPoolAllocatorTracker::GetPeakSlotsUsed() const { return (s32)GetAllocator()->GetPeakSlotsUsed(); }
	s32 sysMemPoolAllocatorTracker::GetNoOfFreeSpaces() const { return (s32)GetAllocator()->GetNoOfFreeSpaces(); }
	s32 sysMemPoolAllocatorTracker::GetActualMemoryUse() const
	{
		return (s32)(sizeof(rage::sysMemPoolAllocator) +
			sizeof(rage::sysMemPoolAllocator::PoolWrapper<void*>) +
			GetAllocator()->GetHeapSize());
	}
	s32 sysMemPoolAllocatorTracker::GetMemoryUsed() const { return (s32)GetAllocator()->GetMemoryUsed(); }
#endif // RAGE_POOL_TRACKING


#if __SPU
	#define POOL_ALLOCATOR_CRITICAL_SECTION
#else
	#define POOL_ALLOCATOR_CRITICAL_SECTION sysCriticalSection cs(m_Token)
#endif	

	sysMemPoolAllocator::~sysMemPoolAllocator()
	{
		if(m_aInstanceIds)
		{
			delete[] m_aInstanceIds;
		}
#if RAGE_POOL_TRACKING
		PoolTracker::Remove(&m_tracker);
#endif
	}

	void sysMemPoolAllocator::Init(void* const ptr, const size_t bytes, const size_t size, bool bAllocInstanceIdArray RAGE_POOL_TRACKING_ONLY(, const char* debugName))
	{
		Assert(IsAligned(ptr) && bytes <= s_maxCapacity);
		Assert(!m_base);

		m_base = ptr;
		m_capacity = bytes;
		m_used = 0;

#if POOL_ALLOCATOR_DEBUG
		m_peakUsed = 0;
		m_poolFullCB = NULL;
#endif
		m_pool.Init(size);
		m_poolHashTable.Init(static_cast<u16>(size));

		FreeNode* pFreeNode = MarkFree(m_base, m_capacity);
		AddToFreeList(pFreeNode);

		m_aInstanceIds = bAllocInstanceIdArray ? rage_new u8[size] : NULL;

#if RAGE_POOL_TRACKING
		m_tracker.SetAllocator(this);
		m_tracker.SetName(debugName);
		PoolTracker::Add(&m_tracker);
#endif
	}

	void sysMemPoolAllocator::Reset()
	{		
		m_used = 0;
		m_freeList.clear();

#if POOL_ALLOCATOR_DEBUG
		m_peakUsed = 0;
#endif
		m_pool.Reset();

		FreeNode* pFreeNode = MarkFree(m_base, m_capacity);
		AddToFreeList(pFreeNode);		
	}

#if POOL_ALLOCATOR_DEBUG
	// Free - O(n)
	bool sysMemPoolAllocator::IsFreePointer(const void* const ptr) const
	{
		Assert(IsValid(ptr));
		
		const FreeNode* const pFreeNode = static_cast<const FreeNode* const>(ptr);
		if (pFreeNode->m_used)
			return false;

		for (FreeList::const_iterator it = m_freeList.begin(); it != m_freeList.end(); ++it)
		{
			const FreeNode* const pWhere = *it;
			if (pWhere == pFreeNode)
				return true;
		}

		return false;
	}
#endif

	sysMemPoolAllocator::FreeNode* sysMemPoolAllocator::MarkFree(void* const ptr, const size_t bytes)
	{
		Assert(IsValid(ptr) && IsValid(bytes));

		FreeNode* pFreeNode = rage_placement_new(ptr) FreeNode(bytes);

		// Is the next block used?
		void* pOffset = GetOffset(pFreeNode, static_cast<int>(pFreeNode->m_size));
		UsedNode* pUsedNode = GetUsedNode(pOffset);

		if (pUsedNode)
		{
			// Tell the next node we are free
			Assert(pOffset == pUsedNode->GetPtr());
			pUsedNode->m_prev_used = false;

			// Stamp the tail
			pOffset = GetOffset(pOffset, -static_cast<int>(sizeof(SizeNode)));
			Assert(pOffset < pUsedNode->GetPtr());

			rage_placement_new(pOffset) SizeNode(bytes, false);
		}

		return pFreeNode;
	}

	sysMemPoolAllocator::FreeNode* sysMemPoolAllocator::GetPrevFreeNode(UsedNode* pUsedNode) const
	{
		Assert(IsValid(pUsedNode) && !pUsedNode->m_prev_used);

		const void* const ptr = pUsedNode->GetPtr();
		void* pOffset = GetOffset(ptr, -static_cast<int>(sizeof(SizeNode)));
		SizeNode* pSizeNode = static_cast<SizeNode*>(pOffset);
		Assert(IsValid(pSizeNode));

		pOffset = GetOffset(ptr, -static_cast<int>(pSizeNode->m_size));
		FreeNode* pFreeNode = static_cast<FreeNode*>(pOffset);
		Assert(IsValid(pFreeNode));

		return pFreeNode;
	}

	sysMemPoolAllocator::FreeNode* sysMemPoolAllocator::GetNextFreeNode(UsedNode* pUsedNode) const
	{
		Assert(IsValid(pUsedNode));

		// Is the next block free?
		const void* const ptr = pUsedNode->GetPtr();
		void* const pOffset = GetOffset(ptr, static_cast<int>(pUsedNode->m_size));
		
		if (IsWithinHeap(pOffset) && !GetUsedNode(pOffset))
		{
			FreeNode* pFreeNode = static_cast<FreeNode*>(pOffset);
			Assert(IsValid(pFreeNode));

			return pFreeNode;
		}

		return NULL;
	}

	void sysMemPoolAllocator::AddToFreeList(FreeNode* pFreeNode)
	{
		Assert(IsValid(pFreeNode));

		// TODO: This could be optimized by using a tree or hash table
		//
		for (FreeList::iterator it = m_freeList.begin(); it != m_freeList.end(); ++it)
		{
			FreeNode* pWhere = *it;
			if (pWhere->m_size > pFreeNode->m_size)
			{
				m_freeList.insert(pWhere, pFreeNode);
				return;
			}
		}

		// If we get here the free list is empty
		m_freeList.push_back(pFreeNode);
	}

	// Used
	void sysMemPoolAllocator::SetUsedNode(PoolNode* pPoolNode, const size_t bytes)
	{
		Assert(IsValid(pPoolNode) && IsValid(bytes));

		// 1) Update current node
		UsedNode usedNode(pPoolNode, bytes);
		m_poolHashTable.Insert(pPoolNode->m_ptr, usedNode);

		// 2) Update next node		
		void* pOffset = GetOffset(pPoolNode->m_ptr, static_cast<int>(bytes));
		UsedNode* pUsedNode = GetUsedNode(pOffset);

		if (pUsedNode)
		{
			pUsedNode->m_prev_used = true;
		}
	}

	// Overridden
	void* sysMemPoolAllocator::SafeAllocate(size_t bytes, size_t align)
	{
		POOL_ALLOCATOR_CRITICAL_SECTION;

		return Allocate(bytes, align, true);
	}

	void* sysMemPoolAllocator::Allocate(size_t bytes, size_t ASSERT_ONLY(align), bool ASSERT_ONLY(threadSafe /*= false*/))
	{	
		ASSERT_ONLY(if (!threadSafe && sysMemVerifyMainThread) sysMemVerifyMainThread();)
		Assert(bytes && align <= s_alignment);

#if POOL_ALLOCATOR_DEBUG
		PoolNode* pPoolNode = m_pool.GetNoOfFreeSpaces() ? m_pool.New(sizeof(void*)) : NULL;
#else
		PoolNode* pPoolNode = m_pool.New(sizeof(void*));
#endif

		if (pPoolNode)
		{
			// Alignment
			bytes = (bytes >= s_minFreeBlockSize) ? AlignPow2(bytes, s_alignment) : s_minAllocateSize;

			for (FreeNode* pWhere = m_freeList.front(); pWhere != NULL; pWhere = pWhere->m_ListLink.m_next)
			{
				Assert(IsValid(pWhere));

				if (pWhere->m_size >= bytes)
				{
					// Remove the block from the free list
					m_freeList.erase(pWhere);

					// 1) Trim suffix
					const size_t leftover = pWhere->m_size - bytes;
					if (leftover >= s_minFreeBlockSize)
					{
						// Add new free node
						void* const ptr = GetOffset(pWhere, static_cast<int>(bytes));
						FreeNode* pFreeNode = MarkFree(ptr, leftover);
						AddToFreeList(pFreeNode);
					}
					else
						bytes += leftover;

					// 2) Mark used
					pPoolNode->m_ptr = static_cast<void*>(pWhere);
					SetUsedNode(pPoolNode, bytes);
					
					// 3) Tally
					m_used += bytes;

					// 4) Update instanceId
					if(m_aInstanceIds)
					{
						s32 index = m_pool.GetJustIndex(pPoolNode);
						m_aInstanceIds[index] += 1;
					}

#if POOL_ALLOCATOR_DEBUG
					if (m_used > m_peakUsed)
						m_peakUsed = m_used;
#endif
#if RAGE_POOL_ALLOCATOR_TRACKING
					// Update MemVisualize
					if (::rage::diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasMisc())
						::rage::diagTracker::GetCurrent()->Tally(pPoolNode->m_ptr, bytes, 0);
#endif
					// Fill memory to known state
					IF_DEBUG_MEMORY_FILL_N(memset(pPoolNode->m_ptr, 0xCD, bytes), DMF_GAMEHEAP_ALLOC);

					return pPoolNode->m_ptr;
				}
			}

			m_pool.Delete(pPoolNode);
		}

#if POOL_ALLOCATOR_DEBUG
		PoolFullCallback();
#endif
		Quitf(ERR_MEM_POOLALLOC_ALLOC_1,"sysMemPoolAllocator - Memory is full (%" SIZETFMT "u KB). Check TTY output for the contents of the pool", (m_capacity >> 10));

		return NULL;
	}

	void sysMemPoolAllocator::SafeFree(void* const ptr)
	{
		POOL_ALLOCATOR_CRITICAL_SECTION;

		return Free(ptr, true);
	}

	void sysMemPoolAllocator::Free(void* const ptr, bool ASSERT_ONLY(threadSafe /*= false*/))
	{
		ASSERT_ONLY(if (!threadSafe && sysMemVerifyMainThread) sysMemVerifyMainThread();)

		if (!IsValidPointer(ptr))
			Quitf(ERR_MEM_POOLALLOC_FREE_1,"sysMemPoolAllocator - Attempting to free memory not allocated from this heap: %p", ptr);

		// Used
		UsedNode* pUsedNode = GetUsedNode(ptr);
		Assert(IsValid(pUsedNode));

		// Bail if the memory isn't in use
		if (!pUsedNode)
			Quitf(ERR_MEM_POOLALLOC_FREE_2,"sysMemPoolAllocator - Memory at %p already marked free", ptr);

		// Pool		
		PoolNode* pPoolNode = pUsedNode->m_pPoolNode;
		Assert(IsValid(pPoolNode));		

		// Tally
		void* ptrFree = ptr;
		size_t bytes = pUsedNode->m_size;
		m_used -= bytes;

#if RAGE_POOL_ALLOCATOR_TRACKING
		// Update MemVisualize
		if (::rage::diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasMisc())
			::rage::diagTracker::GetCurrent()->UnTally(ptr, bytes);
#endif
		// Next
		FreeNode* pFreeNode = GetNextFreeNode(pUsedNode);
		if (pFreeNode)
		{
			bytes += pFreeNode->m_size;
			m_freeList.erase(pFreeNode);			
		}

		// Prev
		if (!pUsedNode->m_prev_used)
		{			
			pFreeNode = GetPrevFreeNode(pUsedNode);
			ptrFree = reinterpret_cast<UsedNode*>(pFreeNode);
			bytes += pFreeNode->m_size;			
			m_freeList.erase(pFreeNode);
		}

		// Fill memory to known state
		IF_DEBUG_MEMORY_FILL_N(memset(ptrFree, 0xDD, bytes), DMF_GAMEHEAP_FREE);

		// Used
		pFreeNode = MarkFree(ptrFree, bytes);
		AddToFreeList(pFreeNode);

		// Delete
		m_poolHashTable.Delete(ptr);
		m_pool.Delete(pPoolNode);
	}

	void sysMemPoolAllocator::ForAll(Callback cb, void* data)
	{
		for (size_t i = 0; i < m_pool.GetSize(); ++i)
		{
			const PoolNode* pPoolNode = m_pool.GetSlot(static_cast<s32>(i));
			if (pPoolNode)
			{
				if(!cb(pPoolNode->m_ptr, data))
					return;
			}
		}
	}

#if POOL_ALLOCATOR_DEBUG
	void sysMemPoolAllocator::PoolFullCallback() const
	{
		if (m_poolFullCB)
		{
			for (size_t i = 0; i < m_pool.GetSize(); ++i)
			{
				const PoolNode* pPoolNode = m_pool.GetSlot(static_cast<s32>(i));
				if (pPoolNode)
					m_poolFullCB(pPoolNode->m_ptr);
			}
		}
	}
#endif // POOL_ALLOCATOR_DEBUG

#if __ASSERT
	void sysMemPoolAllocator::SanityCheck()
	{
		// Free list
		size_t lastBytes = 0;
		size_t freeBytes = 0;

		for (FreeList::iterator it = m_freeList.begin(); it != m_freeList.end(); ++it)
		{
			FreeNode* pWhere = *it;
			Assertf(!pWhere->m_used, "sysMemPoolAllocator - Free node is marked as used");
			Assertf(lastBytes <= pWhere->m_size, "sysMemPoolAllocator - Freelist is not sorted");

			freeBytes += pWhere->m_size;
			lastBytes = pWhere->m_size;
		}

		// Used list		
		size_t usedBytes = 0;
		
		for (size_t i = 0; i < m_pool.GetSize(); ++i)
		{
			PoolNode* pPoolNode = m_pool.GetSlot(static_cast<s32>(i));
			if (pPoolNode)
			{
				UsedNode* pUsedNode = GetUsedNode(pPoolNode->m_ptr);
				usedBytes += pUsedNode->m_size;
			}
		}

		// Did we add up properly?
		Assertf(usedBytes == m_used, "sysMemPoolAllocator - Used bytes don't add up! Used Bytes: %" SIZETFMT "d, m_used: %" SIZETFMT "d", usedBytes, m_used);
		Assertf((usedBytes + freeBytes) == m_capacity, "sysMemPoolAllocator - Total bytes don't add up! Total Bytes: %" SIZETFMT "d, Capacity: %" SIZETFMT "d", usedBytes + freeBytes, m_capacity);
	}
#endif
} // namespace rage
