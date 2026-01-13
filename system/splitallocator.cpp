// 
// system/splitallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
// Eric J Anderson
// 

#include "system/splitallocator.h"
#include "math/amath.h"

namespace rage
{
	void* sysMemSplitAllocator::Allocate(size_t size, size_t align, int heapIndex /*= 0*/)
	{
		Assert(m_pPrimary && m_pSecondary);

		void* ptr = m_pPrimary->Allocate(size, align, heapIndex);
		if (ptr)
			return ptr;

		return m_pSecondary->Allocate(size, align, heapIndex);
	}

	void* sysMemSplitAllocator::TryAllocate(size_t size, size_t align, int heapIndex /*= 0*/)
	{
		Assert(m_pPrimary && m_pSecondary);

		void* ptr = m_pPrimary->TryAllocate(size, align, heapIndex);
		if (ptr)
			return ptr;

		return m_pSecondary->TryAllocate(size, align, heapIndex);
	}

	void sysMemSplitAllocator::Free(const void *ptr)
	{
		Assert(m_pPrimary && m_pSecondary);

		if (m_pPrimary->IsValidPointer(ptr))
			return m_pPrimary->Free(ptr);

		if (m_pSecondary->IsValidPointer(ptr))
			return m_pSecondary->Free(ptr);

		Assertf(false, "sysMemFlexAllocator - Invalid Free: %p", ptr);
	}

	bool sysMemSplitAllocator::IsValidPointer(const void* ptr) const
	{
		Assert(m_pPrimary && m_pSecondary);
		return m_pPrimary->IsValidPointer(ptr) || m_pSecondary->IsValidPointer(ptr);
	}

	size_t sysMemSplitAllocator::GetSize(const void *ptr) const
	{
		Assert(m_pPrimary && m_pSecondary);

		if (m_pPrimary->IsValidPointer(ptr))
			return m_pPrimary->GetSize(ptr);

		if (m_pSecondary->IsValidPointer(ptr))
			return m_pSecondary->GetSize(ptr);

		Assertf(false, "sysMemFlexAllocator - Invalid GetSize: %p", ptr);
		return 0;
	}

	size_t sysMemSplitAllocator::GetSizeWithOverhead(const void *ptr) const
	{
		Assert(m_pPrimary && m_pSecondary);

		if (m_pPrimary->IsValidPointer(ptr))
			return m_pPrimary->GetSizeWithOverhead(ptr);

		if (m_pSecondary->IsValidPointer(ptr))
			return m_pSecondary->GetSizeWithOverhead(ptr);

		Assertf(false, "sysMemFlexAllocator - Invalid GetSizeWithOverhead: %p", ptr);
		return 0;
	}

	size_t sysMemSplitAllocator::GetMemoryUsed(int bucket /*= -1*/)
	{
		Assert(m_pPrimary && m_pSecondary);
		return m_pPrimary->GetMemoryUsed(bucket) + m_pSecondary->GetMemoryUsed(bucket);
	}

	size_t sysMemSplitAllocator::GetMemoryAvailable()
	{
		Assert(m_pPrimary && m_pSecondary);
		return m_pPrimary->GetMemoryAvailable() + m_pSecondary->GetMemoryAvailable();
	}

	size_t sysMemSplitAllocator::GetLargestAvailableBlock()
	{
		Assert(m_pPrimary && m_pSecondary);
		return rage::Max(m_pPrimary->GetLargestAvailableBlock(), m_pSecondary->GetLargestAvailableBlock());
	}

	size_t sysMemSplitAllocator::GetHighWaterMark(bool reset)
	{
		Assert(m_pPrimary && m_pSecondary);
		return m_pPrimary->GetHighWaterMark(reset) + m_pSecondary->GetHighWaterMark(reset);
	}
}
