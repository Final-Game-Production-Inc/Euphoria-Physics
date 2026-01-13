//  
// system/sparseallocator.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_SPARSEALLOCATOR_H
#define SYSTEM_SPARSEALLOCATOR_H

#include "system/memory.h"
#include "system/criticalsection.h"

namespace rage {

#if RSG_PC || RSG_DURANGO

class sysMemSparseAllocator: public sysMemAllocator
{
public:
	static const size_t MaxPages = 4096;
	struct AllocInfo {
		char *memory;
		// If this memory is used, alignedSize and pageAlignedSize indicate the original allocation
		// size and the (rounded to 4k or 8k) actual mapped size.
		// If this memory is not used, pageAlignedSize is the former size of the block, and alignedSize
		// contains the index of the next free block on the free list.  The index is a 18:14 packed value
		// where the upper part is the index into the m_Pages array and the lower 14 bits (which must be
		// big enough to hold AllocsPerBlock, below) are the index into the SparsePageHeader.allocInfo.
		// The list is terminate with -1, and m_FirstFree/m_LastFree will be -1 only if the free list is empty.
		u32 used: 1, pageAlignedSize: 31;
		s32 alignedSize;
	};
	struct FreeList {
		FreeList() : First(-1), Last(-1) { }
		s32 First, Last;
	};
	typedef char SparsePage[4096];
	static const size_t AllocsPerBlock = 16383;		// This needs to be odd because of the separate count header
	static const size_t PagesPerBlock = AllocsPerBlock * 2;
	struct SparsePageHeader {
		u32 allocCount, pad[3];
		AllocInfo allocInfo[AllocsPerBlock];	// Current tuning is up to 64k pages, but guard pages take at most half.
		SparsePage arenaBegin[2];				// unmapped
		SparsePage arena[PagesPerBlock];		// mapped as necessary with internal guard pages
		SparsePage arenaEnd[2];					// unmapped
	};
	CompileTimeAssert(!__64BIT || (sizeof(SparsePageHeader) & 8191) == 0);

	sysMemSparseAllocator(size_t fakeHeapSize,const int heapIndex);
	~sysMemSparseAllocator();

	bool SetQuitOnFail(const bool /*fail*/) { return false; }
	void* Allocate(size_t size,size_t align,int heapIndex = 0 );
	void* TryAllocate(size_t size,size_t align,int heapIndex /* = 0 */);
	void Free(const void *ptr);
	void Resize(const void * /*ptr*/,size_t /*newSmallerSize*/);
	size_t GetSize(const void *ptr) const;
	size_t GetMemoryUsed(int bucket = -1 /*all buckets*/);
	size_t GetMemoryAvailable();
	// EJ: The concept of "largest available" doesn't exist within sparse allocator. We're going to use total available instead.
	size_t GetLargestAvailableBlock() { return GetMemoryAvailable(); }
	bool IsValidPointer(const void*) const;
	size_t GetSizeWithOverhead(const void *ptr) const;
	void* GetHeapBase() const { return 0; }
	size_t GetHeapSize() const;
	char* GetSubHeapBase(size_t idx) const;
	size_t GetSubHeapCount() const;
	static size_t GetSubHeapSize();

protected:
	AllocInfo *FindAlloc(const void *ptr,s32 *outIndex = NULL) const;
	AllocInfo &GetAlloc(s32 index) const;
	size_t GetFreeListForSize(size_t size) const;

	SparsePageHeader *m_Pages[MaxPages];
	size_t m_PageCount;
	char *m_NextAlloc, *m_NextEnd;
	size_t m_MemoryUsed;
	size_t m_FakeHeapSize;
	FreeList m_FreeLists[32];
	sysCriticalSectionToken m_Token;
};

#endif // RSG_PC

}

#endif // SYSTEM_SPARSEALLOCATOR_H