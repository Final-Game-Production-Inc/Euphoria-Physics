// 
// system/fixedallocator.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
// 
// system/multiallocator.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_FIXEDALLOCATOR_H
#define SYSTEM_FIXEDALLOCATOR_H

#include "system/memory.h"
#include "atl/bitset.h"

namespace rage {

/*
	sysMemFixedAllocator manages one or more arrays of like-sized objects
	without actually using any of the memory directly.  Basically
	wraps atPool semantics in a generalized memory allocator.
*/
class sysMemFixedAllocator: public sysMemAllocator 
{
public:
	// PURPOSE:	Create a fixed-size allocator
	// PARAMS:	heapBase - Pointer to memory we should manage (not actually referenced)
	//			elSize - Element size
	//			count - Element count, must be < 256
	sysMemFixedAllocator(void *heapBase,size_t elSize,size_t count);

	// PURPOSE:	Create a fixed-size allocator (with multiple sizes supported)
	// PARAMS:	heapBase - Pointer to memory we should manage (not actually referenced)
	//			fixedSizeCount - number of fixed sizes the allocator should support
	//			elSize - an array of Element sizes
	//			count - an array of Element counts
	// NOTE:	the total of all size*counts must fit in the memory pointed to by headBase
	sysMemFixedAllocator(void *heapBase, int fixedSizeCount, const size_t elSizes[], const u8 counts[]);

	virtual ~sysMemFixedAllocator();

	virtual void *Allocate(size_t size,size_t align,int heapIndex);
	virtual void Free(const void *ptr);
	virtual size_t GetSize(const void *ptr) const;
	virtual size_t GetSizeWithOverhead(const void *ptr) const;
	virtual size_t GetMemoryUsed(int bucket);
	virtual size_t GetMemoryAvailable();
	virtual size_t GetLargestAvailableBlock();
	virtual bool IsValidPointer(const void *ptr) const;

	u8 GetNumBucketAllocations(const unsigned bucket) const;

	// PURPOSE
	//  When cascading is enabled, and the a nearest fit isn't possible, attempt
	//  to use the next best fit.
	//  Example: assume we have elSizes of {16, 64, 96, 128}. If we attempt to
	//  allocate 64 bytes, but the bucket containing 64 byte chunks is full,
	//  attempt to allocate from the bucket containing 96 byte chunks, then 128
	//  byte chunks until we find a free slot. Without cascading, we would only
	//  check the bucket containing 64 byte chunks.
	virtual void SetAllowCascading(bool enable);

	// PURPOSE:	Returns the number of elements currently allocated.
	unsigned GetNumAllocations();

	void DumpStats();

private:
	void CommonContructor(void *heapBase, int fixedSizeCount, const size_t elSizes[], const u8 counts[]);
	int GetSizeIndex(const void *ptr) const;

private:
	enum {MAX_FIXED_SIZES = 12};
	int m_FixedSizes;
	bool m_AllowCascading;
	void *m_Bases[MAX_FIXED_SIZES];
	size_t m_ElementSizes[MAX_FIXED_SIZES];
	
	u8 m_Counts[MAX_FIXED_SIZES], m_Used[MAX_FIXED_SIZES];
	u8 m_FreeList[MAX_FIXED_SIZES][256];
#if __BANK
	u8 m_MostUsed[MAX_FIXED_SIZES];
	size_t m_TotalMemRequested[MAX_FIXED_SIZES];
	size_t m_TotalMemUsed[MAX_FIXED_SIZES];
#endif
};

}	// namespace rage

#endif
