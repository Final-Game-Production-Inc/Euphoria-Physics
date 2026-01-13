// 
// system/miniheap.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "memory.h"
#include "miniheap.h"
#include "tls.h"

#include "diag/errorcodes.h"
#include "diag/output.h"

namespace rage {

// The location of the top of the mini heap
static __THREAD char* s_MiniHeap;

// The end of the mini heap
static __THREAD char* s_MiniHeapEnd;

// The start of the mini heap
static __THREAD char* s_MiniHeapStart;

// Previous allocator
static __THREAD sysMemAllocator* s_PrevAllocator;

static class miniheapAllocator: public sysMemAllocator {
public:
	void *Allocate(size_t size,size_t align,int /*heapIndex*/);
	void Free(const void * /*ptr*/) {
		Warningf("Cannot free from miniheap!");
	}
	size_t GetMemoryUsed(int) {
		return s_MiniHeap - s_MiniHeapStart;
	}
	size_t GetMemoryAvailable() {
		return s_MiniHeapEnd - s_MiniHeap;
	}

	size_t GetLargestAvailableBlock() {
		return GetMemoryAvailable();
	}

	// PURPOSE: Determine whether heap needs tallied.
	bool IsTallied() { return false; } // KS

	bool IsValidPointer(const void *ptr) const {
		return ptr >= s_MiniHeapStart && ptr < s_MiniHeap;
	}
} s_miniheapAllocator;

void* miniheapAllocator::Allocate(size_t size,size_t align,int /*heapIndex*/) {
	if (!align) align=16;
	size_t alignMask = size<=4? 4-1 : size <= 8? 8-1 : align-1;
	s_MiniHeap = (char*)((size_t)(s_MiniHeap + alignMask) & ~alignMask);
	void *rv = (void*) s_MiniHeap;
	s_MiniHeap = s_MiniHeap + size;
	if (s_MiniHeap > s_MiniHeapEnd) {
		//FinalQuitf("Miniheap overflow");
		Quitf(ERR_MEM_MIN_ALLOC,"Miniheap overflow: %" SIZETFMT "d bytes requested, which overflow the mini heap by %" SIZETFMT "d bytes", size, s_MiniHeap - s_MiniHeapEnd);
	}
	return rv;
}


void sysMemStartMiniHeap(void *buffer,size_t size) {
	s_MiniHeapStart = (char*) buffer;
	s_MiniHeap = (char*) buffer;
	s_MiniHeapEnd = s_MiniHeap + size;
	s_PrevAllocator = &sysMemAllocator::GetCurrent();
	sysMemAllocator::SetCurrent(s_miniheapAllocator);
}

void sysMemEnableMiniHeap(bool enable)
{
	if (enable) {
		sysMemAllocator::SetCurrent(s_miniheapAllocator);
	}
	else {
		sysMemAllocator::SetCurrent(*s_PrevAllocator);
	}
}

size_t sysMemEndMiniHeap() {
	sysMemAllocator::SetCurrent(*s_PrevAllocator);
	return s_MiniHeapEnd - s_MiniHeap;
}

size_t sysMemGetUsedMiniHeap() {
	return s_MiniHeap - s_MiniHeapStart;
}

void *sysMemGetTopMiniHeap( size_t align ) {
	size_t alignMask = align-1;
	return (void *)((size_t)(s_MiniHeap + alignMask) & ~alignMask);
}

size_t sysMemGetFreeMiniHeap( size_t align ){
	size_t alignMask = align-1;
	return (size_t(s_MiniHeapEnd) - ((size_t)(s_MiniHeap + alignMask) & ~alignMask));
}

void sysMemPopMiniHeap( size_t popCount ){
	s_MiniHeap -= popCount;
}

}	// namespace rage
