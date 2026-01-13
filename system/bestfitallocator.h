// 
// system/bestfitallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// Eric J Anderson
// 
#ifndef SYSTEM_BESTFIT_ALLOCATOR_H
#define SYSTEM_BESTFIT_ALLOCATOR_H

#include "atl/inlist.h"
#include "system/memory.h"

// DO NOT enable this on __FINAL
#define BESTFIT_ALLOCATOR_DEBUG (!__FINAL)
#if BESTFIT_ALLOCATOR_DEBUG
#define BESTFIT_ALLOCATOR_DEBUG_ONLY(x) x
#else
#define BESTFIT_ALLOCATOR_DEBUG_ONLY(x)
#endif

namespace rage
{
	/* 
	!!!!! NOT THREAD SAFE !!!!!
	Minimal heap class, but still reasonably fast. Acts like an iteratable pool but with dynamic allocation abilities. 
	Header size is optimized to sizeof(void*) to use memory as efficiently as possible.
	*/
	class sysMemBestFitAllocator
	{
	private:
		struct SizeNode
		{
			size_t m_used : 1;							// used = 1, free = 0
			size_t m_size : ((sizeof(void*) * 8) - 1);	// 2,147,483,648 max bytes  (32 bit)

			SizeNode();
			SizeNode(const size_t size, const bool used);
		};

		struct FreeNode
		{
			size_t m_used : 1;							// used = 1, free = 0
			size_t m_size : ((sizeof(void*) * 8) - 1);	// 2,147,483,648 max bytes  (32 bit)			
			inlist_node<FreeNode> m_ListLink;

			FreeNode();
			explicit FreeNode(const size_t size);
		};

		struct UsedNode
		{
			size_t m_used : 1;							// used = 1, free = 0
			size_t m_prev_used : 1;						// used = 1, free = 0
			size_t m_aligned : 1;
			size_t m_size : ((sizeof(void*) * 8) - 3);	// 5,368,70,911 max bytes (32 bit)

			UsedNode();
			UsedNode(const size_t size, const bool prev_used, const bool aligned = false);
		};
		
		typedef inlist<FreeNode, &FreeNode::m_ListLink> FreeList;

	private:
		FreeList m_freeList;

		void* m_base;
		size_t m_capacity;
		size_t m_used;

	public:
		const static size_t s_minFreeBlockSize = static_cast<size_t>(sizeof(FreeNode) + sizeof(SizeNode));
		const static size_t s_maxFreeCapacity = static_cast<size_t>(~0) >> 1;
		const static size_t s_maxUsedCapacity = static_cast<size_t>(~0) >> 3;

	private:
		// Free
		bool IsFree(const void* const ptr) const;
		FreeNode* MarkFree(void* const ptr, const size_t bytes);
		FreeNode* GetPrevFreeNode(UsedNode* pUsedNode) const;
		FreeNode* GetNextFreeNode(UsedNode* pUsedNode) const;
		void AddToFreeList(FreeNode* pNode);

		// Used
		bool IsUsed(const void* const ptr) const;
		UsedNode* GetUsedNode(const void* const ptr) const;
		UsedNode* MarkUsed(void* const ptr, const size_t bytes, const bool prevUsed, const size_t align = 0);

		// Utility
		inline void* GetOffset(const void* const ptr, const int bytes) const { return static_cast<char*>(const_cast<void*>(ptr)) + bytes; }

#if BESTFIT_ALLOCATOR_DEBUG
		// Guard
		void SetGuardPointer(UsedNode* pUsedNode) const;
		bool IsGuardPointerValid(UsedNode* pUsedNode) const;
#endif

	public:
		sysMemBestFitAllocator();
		sysMemBestFitAllocator(void* const ptr, const size_t bytes);

		// PURPOSE:	Init the heap
		void Init(void* const ptr, const size_t bytes);

		// PURPOSE:	Allocate memory with a specified alignment
		void* Allocate(size_t bytes, size_t align);

		// PURPOSE:	Free memory allocated via Allocate.
		void Free(void* const ptr);

		// PURPOSE:	Returns actual amount of memory (sans node overhead) associated with the allocation.
		inline size_t GetSize(const void* const ptr) const;

		// PURPOSE:	Returns actual amount of memory (WITH node overhead) associated with the allocation.
		inline size_t GetSizeWithOverhead(const void* const ptr) const { return GetUsedNode(ptr)->m_size; }

		// RETURNS:	Base address of the heap
		inline void* GetHeapBase() const { return m_base; }

		// PURPOSE: Returns amount of memory used on heap
		inline size_t GetMemoryUsed() const { return m_used; }

		// PURPOSE: Returns amount of memory remaining on heap
		inline size_t GetMemoryAvailable() const { return m_capacity - m_used; }

		// PURPOSE:	Returns largest amount of memory that can be allocated in a single contiguous block
		size_t GetLargestAvailableBlock() const;

		// PURPOSE: Amount of fragmentation, as a percentage (0-100).  Returns -1 if unknown.
		// Typical implementation is 1.0f - (largestAvailable / totalAvailable)
		size_t GetFragmentation() const;

		// PURPOSE:	True if this is a valid heap pointer owned by this heap.
		inline bool IsValidPointer(const void* const ptr) const { return (ptr >= m_base) && ptr < (static_cast<char*>(m_base) + m_capacity); }

		// PURPOSE: Return the size of the heap; the same as the size parameter passed into Init
		inline size_t GetHeapSize() const { return m_capacity; }

		// PURPOSE: Resets all pool entries to free; O(N) operation
		void Reset();

		// PURPOSE:	Run basic sanity checks on the heap.
		ASSERT_ONLY(void SanityCheck();)
	};

} // namespace rage

#endif
