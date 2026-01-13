// 
// system/externalallocator.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "externalallocator.h"
#include "criticalsection.h"
#include "diag/errorcodes.h"
#include "diag/tracker.h"
#include "data/marker.h"

#if __WIN32
#include "system/xtl.h"
#else
#include <stdlib.h>
#endif

namespace rage {

static sysCriticalSectionToken s_ExternalAllocatorToken;

sysMemExternalAllocator::sysMemExternalAllocator(size_t heapSize,void *heapBase,size_t maxPointers,void *workspace,size_t chunkSize) :
	m_HeapBase(heapBase),
	m_Workspace(workspace)
{
	if (!heapBase) {
		Quitf(ERR_MEM_VIRTUAL_2,"ExternalAllocator - Out of virtual memory");
	}

	m_Heap.Init(heapSize,(u32)maxPointers,m_Workspace,chunkSize);
	m_Size = heapSize;

#if !RSG_PPU && !RSG_ORBIS
	// This would be really slow on PS3 and still pretty slow on Orbis as well.
	IF_DEBUG_MEMORY_FILL_N(debug_memory_fill(heapBase, heapSize),DMF_RESOURCE);
#endif

	RAGE_MEMORY_DEBUG_ONLY(m_watermark = 0;)
}


sysMemExternalAllocator::~sysMemExternalAllocator()
{
}

void* sysMemExternalAllocator::Allocate(size_t size,size_t align,int RAGE_TRACKING_ONLY(heapIndex))
{
	sysCriticalSection cs(s_ExternalAllocatorToken);
	if (!align) align=16;
	size_t offset = m_Heap.Allocate(size+!size,align);
	if (offset == ~0U)
	{
		Quitf(ERR_MEM_EXTALLOC_ALLOC,"Failed to allocate %" SIZETFMT "u (align %" SIZETFMT "u)", size, align);
	}
	void* p = (char*) m_HeapBase + offset;
#if RAGE_TRACKING
	if (p && ::rage::diagTracker::GetCurrent())
		::rage::diagTracker::GetCurrent()->Tally(p, GetSizeWithOverhead(p), heapIndex);
#endif

#if RAGE_MEMORY_DEBUG
	const size_t used = GetMemoryUsed(-1);
	if (used > m_watermark)
	{
		m_watermark = used;
	}
#endif

	return p;
}


void* sysMemExternalAllocator::AllocateFromTop(size_t size,size_t align)
{
	sysCriticalSection cs(s_ExternalAllocatorToken);
	size_t offset = m_Heap.AllocateFromTop(size+!size,align);
	if (offset == ~0U)
		return NULL;

#if RAGE_MEMORY_DEBUG
	const size_t used = GetMemoryUsed(-1);
	if (used > m_watermark)
	{
		m_watermark = used;
	}
#endif

	return (char*) m_HeapBase + offset;	
}


void sysMemExternalAllocator::Free(const void *ptr)
{
	if (ptr)
	{
		sysCriticalSection cs(s_ExternalAllocatorToken);
#if RAGE_TRACKING
		if (::rage::diagTracker::GetCurrent())
			::rage::diagTracker::GetCurrent()->UnTally((void*)ptr, GetSizeWithOverhead(ptr));
#endif
		RAGE_LOG_DELETE(ptr);
		m_Heap.Free((char*)ptr-(char*)m_HeapBase);
	}
}


size_t sysMemExternalAllocator::GetMemoryUsed(int bucket)
{
	return m_Heap.GetMemoryUsed(bucket);
}


size_t sysMemExternalAllocator::GetMemoryAvailable()
{
	return m_Heap.GetMemoryFree();
}

size_t sysMemExternalAllocator::GetLargestAvailableBlock()
{
	return m_Heap.GetLargestAvailableBlock();
}

#if RAGE_MEMORY_DEBUG
size_t sysMemExternalAllocator::GetHighWaterMark(bool reset)
{
	size_t result = m_watermark;
	if (reset)
		m_watermark = GetMemoryUsed(-1);

	return result;
}
#endif

bool sysMemExternalAllocator::IsValidPointer(const void * ptr) const
{ 
	return ptr >= m_HeapBase && ptr < (char*)m_HeapBase + m_Size; 
}

size_t sysMemExternalAllocator::GetSize(const void *ptr) const
{
	if (ptr)
	{
		sysCriticalSection cs(s_ExternalAllocatorToken);
		return m_Heap.GetSize((char*)ptr-(char*)m_HeapBase);
	}
	else
		return 0;
}

}	// namespace rage
