// 
// system/bestfitallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
// Eric J Anderson
// 
#include "math/simplemath.h"
#include "system/debugmemoryfill.h"
#include "system/bestfitallocator.h"
#include "system/new.h"
#include "diag/tracker.h"

namespace rage
{
	// SizeNode
	sysMemBestFitAllocator::SizeNode::SizeNode() : m_used(false), m_size(0) { }
	sysMemBestFitAllocator::SizeNode::SizeNode(const size_t size, const bool used) : m_used(used), m_size(size) { }

	// FreeNode
	sysMemBestFitAllocator::FreeNode::FreeNode() : m_used(false), m_size(0) { }
	sysMemBestFitAllocator::FreeNode::FreeNode(const size_t size) : m_used(false), m_size(size) { }

	// UsedNode
	sysMemBestFitAllocator::UsedNode::UsedNode() : m_used(true), m_prev_used(false), m_aligned(false), m_size(0)
	{
	}

	sysMemBestFitAllocator::UsedNode::UsedNode(const size_t size, const bool prev_used, const bool aligned /*= false*/) : m_used(true), m_prev_used(prev_used), m_aligned(aligned), m_size(size)
	{
	}

	// PoolAllocator
	sysMemBestFitAllocator::sysMemBestFitAllocator() : m_base(NULL), m_capacity(0), m_used(0) {}

	sysMemBestFitAllocator::sysMemBestFitAllocator(void* const ptr, const size_t bytes) { Init(ptr, bytes); }

	void sysMemBestFitAllocator::Init(void* const ptr, const size_t bytes)
	{
		Assertf(ptr, "sysMemBestFitAllocator - Allocation memory is NULL");
		Assertf(bytes <= s_maxFreeCapacity, "sysMemBestFitAllocator - Maximum free capacity is %" SIZETFMT "d bytes", s_maxFreeCapacity);

		m_base = ptr;
		m_capacity = bytes;
		m_used = 0;

		FreeNode* pFreeNode = MarkFree(m_base, m_capacity);
		AddToFreeList(pFreeNode);		
	}

	void sysMemBestFitAllocator::Reset()
	{		
		m_used = 0;
		m_freeList.clear();

		FreeNode* pFreeNode = MarkFree(m_base, m_capacity);
		AddToFreeList(pFreeNode);		
	}

	// Free
	bool sysMemBestFitAllocator::IsFree(const void* const ptr) const
	{
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

	sysMemBestFitAllocator::FreeNode* sysMemBestFitAllocator::MarkFree(void* const ptr, const size_t bytes)
	{
		Assert(ptr && bytes);

		FreeNode* pFreeNode = rage_placement_new(ptr) FreeNode(bytes);

		// Ignore if we are at the end of the heap
		void* const pUsedPtr = GetOffset(pFreeNode, static_cast<int>(pFreeNode->m_size));

		if (IsValidPointer(pUsedPtr))
		{
			// Tell the next node we are free
			UsedNode* pUsedNode = static_cast<UsedNode*>(pUsedPtr);
			if (pUsedNode->m_used)
				pUsedNode->m_prev_used = false;

			// Stamp the tail
			void* const pSizePtr = GetOffset(pUsedNode, -static_cast<int>(sizeof(SizeNode)));
			rage_placement_new(pSizePtr) SizeNode(bytes, false);
		}

		return pFreeNode;
	}

	sysMemBestFitAllocator::FreeNode* sysMemBestFitAllocator::GetPrevFreeNode(UsedNode* pUsedNode) const
	{
		Assert(pUsedNode && pUsedNode->m_used && !pUsedNode->m_prev_used);

		void* ptr = GetOffset(pUsedNode, -static_cast<int>(sizeof(SizeNode)));
		SizeNode* pSizeNode = static_cast<SizeNode*>(ptr);
		Assert(!pSizeNode->m_used);

		ptr = GetOffset(pUsedNode, -static_cast<int>(pSizeNode->m_size));
		FreeNode* pFreeNode = static_cast<FreeNode*>(ptr);
		Assert(!pFreeNode->m_used);

		return pFreeNode;
	}

	sysMemBestFitAllocator::FreeNode* sysMemBestFitAllocator::GetNextFreeNode(UsedNode* pUsedNode) const
	{
		Assert(pUsedNode);

		void* const pSizePtr = GetOffset(pUsedNode, static_cast<int>(pUsedNode->m_size));

		if (IsValidPointer(pSizePtr))
		{
			SizeNode* pSizeNode = static_cast<SizeNode*>(pSizePtr);
			if (!pSizeNode->m_used)
			{
				FreeNode* pFreeNode = static_cast<FreeNode*>(pSizePtr);
				return !pFreeNode->m_used ? pFreeNode : NULL;
			}			
		}

		return NULL;
	}

	void sysMemBestFitAllocator::AddToFreeList(FreeNode* pFreeNode)
	{
		Assert(pFreeNode && pFreeNode->m_size >= s_minFreeBlockSize);

		// TODO: This could be optimized by using a tree or hashtable
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
	bool sysMemBestFitAllocator::IsUsed(const void* const ptr) const
	{
		const UsedNode* pUsedNode = GetUsedNode(ptr);
		if (!pUsedNode->m_used)
			return false;

		// Just in case data was corrupted
		Assert(!IsFree(ptr));

		return true;
	}

	sysMemBestFitAllocator::UsedNode* sysMemBestFitAllocator::GetUsedNode(const void* const ptr) const
	{
		Assert(IsValidPointer(ptr));

		void* pUsedPtr = GetOffset(ptr, -static_cast<int>(sizeof(UsedNode)));
		UsedNode* pUsedNode = static_cast<UsedNode*>(pUsedPtr);
		Assert(pUsedNode->m_used);

		// If we are aligned then jump back
		if (pUsedNode->m_aligned)
		{			
			pUsedPtr = GetOffset(pUsedPtr, -static_cast<int>(pUsedNode->m_size));
			pUsedNode = static_cast<UsedNode*>(pUsedPtr);
			Assert(pUsedNode->m_used);
		}

		return pUsedNode;
	}

	sysMemBestFitAllocator::UsedNode* sysMemBestFitAllocator::MarkUsed(void* const ptr, const size_t bytes, const bool prevUsed, const size_t align /*= 0*/)
	{
		Assert(ptr && bytes);

		// 1) Update current node
		void* pUsedPtr = GetOffset(ptr, -static_cast<int>(sizeof(UsedNode)));

		if (align)
		{
			// Size is the alignment size to get to the actual header
			rage_placement_new(pUsedPtr) UsedNode(align, prevUsed, true);
			pUsedPtr = GetOffset(pUsedPtr, -static_cast<int>(align));
		}

		UsedNode* pUsedNode = rage_placement_new(pUsedPtr) UsedNode(bytes, prevUsed, false);

		// 2) Update next node		
		void* const pNextPtr = GetOffset(pUsedNode, static_cast<int>(pUsedNode->m_size));

		if (IsValidPointer(pNextPtr))
		{
			UsedNode* pNextNode = static_cast<UsedNode*>(pNextPtr);
			if (pNextNode->m_used)
				pNextNode->m_prev_used = true;
		}

		return pUsedNode;
	}
	
#if BESTFIT_ALLOCATOR_DEBUG
	// Guard
	void sysMemBestFitAllocator::SetGuardPointer(UsedNode* pUsedNode) const
	{
		void* ptr = GetOffset(pUsedNode, static_cast<int>(pUsedNode->m_size - sizeof(void*)));
		*static_cast<const void**>(ptr) = ptr;
	}

	bool sysMemBestFitAllocator::IsGuardPointerValid(UsedNode* pUsedNode) const
	{
		void* ptr = GetOffset(pUsedNode, static_cast<int>(pUsedNode->m_size - sizeof(void*)));
		return *static_cast<const void**>(ptr) == ptr;
	}
#endif

	// Overridden
	void* sysMemBestFitAllocator::Allocate(size_t bytes, size_t align)
	{		
		Assertf(bytes, "sysMemBestFitAllocator - Allocation size must be > 0");
		Assertf(align > 0, "sysMemBestFitAllocator - Alignment must be > 0");
		Assertf(!((align - 1) & align), "sysMemBestFitAllocator - Alignment must be a power of 2");		

		// Min size
		bool prevUsed = true;
		size_t minBytes = AlignPow2(bytes, 4) + sizeof(UsedNode) BESTFIT_ALLOCATOR_DEBUG_ONLY(+ sizeof(void*));
		if (minBytes < s_minFreeBlockSize)
			minBytes = s_minFreeBlockSize;

		for (FreeList::iterator it = m_freeList.begin(); it != m_freeList.end(); ++it)
		{
			FreeNode* pWhere = *it;

			if (pWhere->m_size >= minBytes)
			{
				void* const memory = GetOffset(pWhere, static_cast<int>(sizeof(UsedNode)));

				// Alignment			
				void* const aligned = AlignPow2(memory, align);
				size_t alignBytes = static_cast<char*>(aligned) - reinterpret_cast<char*>(memory);
				if (alignBytes + minBytes > pWhere->m_size)
					continue;

				// Remove the block from the free list
				m_freeList.erase(pWhere);

				size_t totalBytes = minBytes;

				// 1) Trim prefix (optional)
				if (alignBytes >= s_minFreeBlockSize)
				{
					// Update existing free node
					void* const ptr = GetOffset(pWhere, static_cast<int>(alignBytes));
					pWhere = rage_placement_new(ptr) FreeNode(pWhere->m_size - alignBytes);
					//pWhere = MarkFree(ptr, pWhere->m_size - alignBytes);

					// Add new free node
					FreeNode* pFreeNode = MarkFree(*it, alignBytes);
					AddToFreeList(pFreeNode);

					prevUsed = false;
					alignBytes = 0;
				}
				else
					totalBytes += alignBytes;

				// 2) Trim suffix (optional)
				const size_t trimBytes = pWhere->m_size - totalBytes;
				if (trimBytes >= s_minFreeBlockSize)
				{
					// Add new free node
					void* const ptr = GetOffset(pWhere, static_cast<int>(totalBytes));
					FreeNode* pFreeNode = MarkFree(ptr, trimBytes);
					AddToFreeList(pFreeNode);

					// Update existing free node
					//pWhere = MarkFree(pWhere, pWhere->m_size - trimBytes);
				}
				else
					totalBytes += trimBytes;

				// 3) Init memory
				Assertf(bytes <= s_maxUsedCapacity, "sysMemBestFitAllocator - Maximum allocation size is %" SIZETFMT "d bytes", s_maxUsedCapacity);
				BESTFIT_ALLOCATOR_DEBUG_ONLY(UsedNode* pUsedNode =) MarkUsed(aligned, totalBytes, prevUsed, alignBytes);

				//Displayf("Total Bytes: %d", totalBytes);
				m_used += totalBytes;

#if BESTFIT_ALLOCATOR_DEBUG
				SetGuardPointer(pUsedNode);
				// SanityCheck();
#endif
#if RAGE_TRACKING
				// // Update MemVisualize
				if (::rage::diagTracker::GetCurrent())
					::rage::diagTracker::GetCurrent()->Tally(pWhere, totalBytes, 0);
#endif
				return aligned;
			}
		}

		return NULL;
	}

	void sysMemBestFitAllocator::Free(void* const ptr)
	{
		if (!IsValidPointer(ptr))
			Quitf(ERR_MEM_BESTALLOC_FREE_1,"sysMemBestFitAllocator::Free - Attempting to free memory not allocated from this heap: %p", ptr);

		// Used
		UsedNode* pUsedNode = GetUsedNode(ptr);
		Assert(pUsedNode->m_used);

		// Bail if the memory isn't in use
		if (!pUsedNode->m_used)
			Quitf(ERR_MEM_BESTALLOC_FREE_2,"sysMemBestFitAllocator::Free - Memory at %p already marked free", ptr);

#if BESTFIT_ALLOCATOR_DEBUG
		Assertf(IsGuardPointerValid(pUsedNode), "sysMemPoolAllocator::Free - Invalid guard word. The memory 0x%p has been trashed!", pUsedNode);
		//SanityCheck();
#endif
		// Tally
		size_t bytes = pUsedNode->m_size;
		m_used -= bytes;

#if RAGE_TRACKING
		// Update MemVisualize
		if (::rage::diagTracker::GetCurrent())
			::rage::diagTracker::GetCurrent()->UnTally(pUsedNode, bytes);
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
			pUsedNode = reinterpret_cast<UsedNode*>(pFreeNode);
			bytes += pFreeNode->m_size;			
			m_freeList.erase(pFreeNode);
		}		

		// Used
		//RAGE_MEMORY_DEBUG_ONLY(sysMemSet(pUsedNode, 0xCD, bytes);)
		pFreeNode = MarkFree(pUsedNode, bytes);
		AddToFreeList(pFreeNode);
	}

	size_t sysMemBestFitAllocator::GetSize(const void* const ptr) const
	{
		UsedNode* pUsedNode = GetUsedNode(ptr);
		const size_t bytes = pUsedNode->m_size - (static_cast<const char* const>(ptr) - reinterpret_cast<char*>(pUsedNode));
		return bytes;
	}

	size_t sysMemBestFitAllocator::GetLargestAvailableBlock() const
	{
		const FreeNode* pFreeNode = m_freeList.back();
		return pFreeNode ? pFreeNode->m_size : 0;
	}

	size_t sysMemBestFitAllocator::GetFragmentation() const
	{
		if (GetLargestAvailableBlock() > 0)
		{
			return 100 - static_cast<size_t>((static_cast<u64>(GetLargestAvailableBlock()) * 100) / GetMemoryAvailable());
		}	

		return 0;
	}

#if __ASSERT
	void sysMemBestFitAllocator::SanityCheck()
	{
		// Free list
		size_t lastBytes = 0;
		size_t freeBytes = 0;

		for (FreeList::iterator it = m_freeList.begin(); it != m_freeList.end(); ++it)
		{
			FreeNode* pWhere = *it;
			Assertf(!pWhere->m_used, "sysMemBestFitAllocator - Free node is marked as used");
			Assertf(lastBytes <= pWhere->m_size, "sysMemBestFitAllocator - Freelist is not sorted");

			freeBytes += pWhere->m_size;
			lastBytes = pWhere->m_size;
		}

		// Did we add up properly?
		Assertf(freeBytes == GetMemoryAvailable(), "sysMemBestFitAllocator - Free bytes don't add up! Free Bytes: %" SIZETFMT "d, GetMemoryAvailable(): %" SIZETFMT "d", freeBytes, GetMemoryAvailable());
	}
#endif
} // namespace rage
