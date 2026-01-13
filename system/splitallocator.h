// 
// system/splitallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// Eric J Anderson
// 
#ifndef SYSTEM_SPLIT_ALLOCATOR_H
#define SYSTEM_SPLIT_ALLOCATOR_H

#include "system/memory.h"

namespace rage
{
	/* 
		[Author]
		Eric J Anderson

		[Overview]
		sysMemSplitAllocator is a simple wrapper around two separate allocators. Memory requests are taken from the 1st
		allocator until it's full, then the 2nd allocator is used.
	*/
	class sysMemSplitAllocator : public sysMemAllocator
	{
	protected:
		sysMemAllocator* m_pPrimary;
		sysMemAllocator* m_pSecondary;

	public:
		sysMemSplitAllocator() : m_pPrimary(NULL), m_pSecondary(NULL) { }
		sysMemSplitAllocator(sysMemAllocator* pPrimary, sysMemAllocator* pSecondary) : m_pPrimary(pPrimary), m_pSecondary(pSecondary) { }
		sysMemSplitAllocator(sysMemAllocator& primary, sysMemAllocator& secondary) : m_pPrimary(&primary), m_pSecondary(&secondary) { }
		SYS_MEM_VIRTUAL ~sysMemSplitAllocator() { }

		inline void Create(sysMemAllocator* pPrimary, sysMemAllocator* pSecondary) { m_pPrimary = pPrimary; m_pSecondary = pSecondary; }
		inline void Create(sysMemAllocator& primary, sysMemAllocator& secondary) { m_pPrimary = &primary; m_pSecondary = &secondary; }
		inline void Destroy() { m_pPrimary = m_pSecondary = NULL; }

		SYS_MEM_VIRTUAL void* Allocate(size_t size, size_t align, int heapIndex = 0);
		SYS_MEM_VIRTUAL void* TryAllocate(size_t size, size_t align, int heapIndex = 0);
		SYS_MEM_VIRTUAL void Free(const void *ptr);

		SYS_MEM_VIRTUAL bool IsValidPointer(const void * /*ptr*/) const;

		SYS_MEM_VIRTUAL size_t GetSize(const void* ptr) const;
		SYS_MEM_VIRTUAL size_t GetSizeWithOverhead(const void* ptr) const;
		SYS_MEM_VIRTUAL size_t GetMemoryUsed(int bucket = -1 /*all buckets*/);
		SYS_MEM_VIRTUAL size_t GetMemoryAvailable();
		SYS_MEM_VIRTUAL size_t GetLargestAvailableBlock();

		SYS_MEM_VIRTUAL size_t GetHighWaterMark(bool reset);
		SYS_MEM_VIRTUAL size_t GetHeapSize() const { return 0; }
		SYS_MEM_VIRTUAL void* GetHeapBase() const { return 0; }		

		inline sysMemAllocator* GetPrimaryAllocator() const {return m_pPrimary;}
		inline sysMemAllocator* GetSecondaryAllocator() const {return m_pSecondary;}
	};
}

#endif
