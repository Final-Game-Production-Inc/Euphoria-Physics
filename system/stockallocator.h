// 
// system/stockallocator.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_STOCKALLOCATOR_H
#define SYSTEM_STOCKALLOCATOR_H

#include "system/memory.h"

namespace rage {

class stockAllocator: public sysMemAllocator {
public:
	stockAllocator();
	void *Allocate(size_t size,size_t ASSERT_ONLY(align),int heapIndex);
	void *TryAllocate(size_t size,size_t align,int heapIndex);
	void Resize(const void * /*ptr*/,size_t /*newSmallerSize*/);
	void Free(const void *ptr);
	size_t GetSize(const void *ptr) const;
	void SetBreakOnAlloc(int allocId);
	void SanityCheck();
	bool IsValidPointer(const void *ptr) const;
	size_t GetMemoryUsed(int /*bucket*/);
	size_t GetMemoryAvailable();
	size_t GetLargestAvailableBlock();
	int EndLayer(const char *, const char *);
};

} // namespace rage

#endif	// SYSTEM_STOCKALLOCATOR_H
