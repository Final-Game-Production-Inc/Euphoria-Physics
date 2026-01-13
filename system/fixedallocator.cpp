// 
// system/fixedallocator.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "fixedallocator.h"

#include "atl/bitset.h"
#include "diag/output.h"
#include "diag/tracker.h"
#include "system/memvisualize.h"

#if !defined(RAGE_POISON_MEMORY_REGION)
#define RAGE_POISON_MEMORY_REGION(...)
#define RAGE_UNPOISON_MEMORY_REGION(...)
#endif

namespace rage {

sysMemFixedAllocator::sysMemFixedAllocator(void *heapBase,size_t elSize, size_t count)
: m_AllowCascading(false)
{
	
	u8 u8count;
	Assign(u8count, count);
	CommonContructor(heapBase, 1, &elSize, &u8count); // for compatibility
}

sysMemFixedAllocator::sysMemFixedAllocator(void *heapBase, int fixedSizeCount, const size_t elSizes[], const u8 counts[]) 
: m_AllowCascading(false)
{
	CommonContructor(heapBase, fixedSizeCount, elSizes, counts);
}

void sysMemFixedAllocator::SetAllowCascading(bool allowCascading)
{
	m_AllowCascading = allowCascading;
}

unsigned sysMemFixedAllocator::GetNumAllocations()
{
	unsigned numAllocs = 0;
	for (int i=0;i<MAX_FIXED_SIZES; i++)
	{
		numAllocs += m_Used[i];
	}

	return numAllocs;
}

void sysMemFixedAllocator::CommonContructor(void *heapBase, int fixedSizeCount, const size_t elSizes[], const u8 counts[])
{
	m_FixedSizes = fixedSizeCount;

	Assert(fixedSizeCount<=MAX_FIXED_SIZES);

	for (int sizeIndex=0;sizeIndex<m_FixedSizes;sizeIndex++)
	{
#if __ASSERT
		//Assert(counts[sizeIndex] <= 256 && "A max of 256 elements of each fixed size are allowed");
		
		for (int i=0;i<sizeIndex; i++)
			AssertMsg(elSizes[sizeIndex] > elSizes[i] , "elements sizes must be in increasing order");
#endif

		m_Bases[sizeIndex] = heapBase;
		heapBase = (void*)((char*)heapBase + elSizes[sizeIndex] * counts[sizeIndex]);

		m_ElementSizes[sizeIndex] = elSizes[sizeIndex];
		m_Counts[sizeIndex] = counts[sizeIndex];
		m_Used[sizeIndex] = 0;
#if __BANK
		m_MostUsed[sizeIndex] = 0;
		m_TotalMemRequested[sizeIndex] = 0;
		m_TotalMemUsed[sizeIndex] = 0;
#endif
		for (size_t i=0; i<counts[sizeIndex]; i++)
			m_FreeList[sizeIndex][i] = (u8) i;

		RAGE_POISON_MEMORY_REGION(m_Bases[sizeIndex], m_ElementSizes[sizeIndex] * m_Counts[sizeIndex]);
	}
}

sysMemFixedAllocator::~sysMemFixedAllocator() { 
	for (int sizeIndex = 0; sizeIndex < m_FixedSizes; sizeIndex++)
	{
		RAGE_UNPOISON_MEMORY_REGION(m_Bases[sizeIndex], m_ElementSizes[sizeIndex] * m_Counts[sizeIndex]);
	}
}

void *sysMemFixedAllocator::Allocate(size_t size,size_t /*align*/,int RAGE_TRACKING_ONLY(heapIndex)) {
	
	int sizeIndex;

	for (sizeIndex=0;sizeIndex<m_FixedSizes;sizeIndex++)
	{
		if ((size <= m_ElementSizes[sizeIndex]) && (!m_AllowCascading || (m_Used[sizeIndex] < m_Counts[sizeIndex])))
			break;
	}

	if (sizeIndex>=m_FixedSizes)
	{
		if(size > m_ElementSizes[m_FixedSizes - 1])
		{
			Errorf("sysMemFixedAllocator::Allocate() requesting allocation larger than the larger supported fixed size");
		}
		return NULL;
	}

	if (m_Used[sizeIndex] == m_Counts[sizeIndex])  // no more of that size...
		return NULL;

	m_Used[sizeIndex]++;
#if __BANK
	if (m_Used[sizeIndex] > m_MostUsed[sizeIndex])
		 m_MostUsed[sizeIndex] = m_Used[sizeIndex];

	m_TotalMemRequested[sizeIndex] += size;
	m_TotalMemUsed[sizeIndex] += m_ElementSizes[sizeIndex];
#endif

//	DumpStats();

	u8 newSlot = m_FreeList[sizeIndex][m_Counts[sizeIndex] - m_Used[sizeIndex]];

	void* ptr = (char*)(m_Bases[sizeIndex]) + newSlot * m_ElementSizes[sizeIndex];

#if RAGE_TRACKING
	if(::rage::diagTracker::GetCurrent() && !sysMemVisualize::GetInstance().HasXTL())
	{
		::rage::diagTracker::GetCurrent()->Tally(ptr, GetSizeWithOverhead(ptr), heapIndex);
	}
#endif

	RAGE_UNPOISON_MEMORY_REGION(ptr, size);
	return ptr;
}


void sysMemFixedAllocator::Free(const void *ptr) {
	if (IsValidPointer(ptr)) {
		
		int sizeIndex = GetSizeIndex(ptr);


		if (!m_Used[sizeIndex])
			return;	  // Quitf("Double-free fault somewhere?");  // don't panik, sometimes we get double frees in resource heaps (if the first object int he heap is freed, it's address may match the base)

		size_t slot = (char*)ptr - (char*)(m_Bases[sizeIndex]);

		if (slot % m_ElementSizes[sizeIndex] == 0) {
			slot /= m_ElementSizes[sizeIndex];
			
			int freeCount = m_Counts[sizeIndex] - m_Used[sizeIndex];
		
			// if it was already freed, make sure it's not already freed.
			for (int i=0;i<freeCount;i++)
			{
				if (m_FreeList[sizeIndex][i]==(u8)slot)
					return;
			}

			m_FreeList[sizeIndex][freeCount] = (u8) slot;
			m_Used[sizeIndex]--;

#if RAGE_TRACKING
			if(::rage::diagTracker::GetCurrent() && !sysMemVisualize::GetInstance().HasXTL())
			{
				::rage::diagTracker::GetCurrent()->UnTally((void*)ptr, GetSizeWithOverhead(ptr));
			}
#endif

			RAGE_POISON_MEMORY_REGION(ptr, m_ElementSizes[sizeIndex]);
		}
	}
}


size_t sysMemFixedAllocator::GetSize(const void *ptr) const {
	// find which list they are from...
	if (!IsValidPointer(ptr)) return 0;

	return m_ElementSizes[GetSizeIndex(ptr)];
}

size_t sysMemFixedAllocator::GetSizeWithOverhead(const void *ptr) const {
	return GetSize(ptr);
}

int sysMemFixedAllocator::GetSizeIndex(const void *ptr) const {
	// find which list they are from...
//	if (!IsValidPointer(ptr)) return -1; // assume they already check this...

	for (int sizeIndex=0; sizeIndex<m_FixedSizes-1; sizeIndex++)
	{
		if (ptr < m_Bases[sizeIndex+1])
			return sizeIndex;
	}

	// must be the last one...
	return m_FixedSizes-1;
}


size_t sysMemFixedAllocator::GetMemoryUsed(int /*bucket*/) {
	size_t usedSum=0;

	for (int sizeIndex=0; sizeIndex<m_FixedSizes; sizeIndex++)
		usedSum += m_Used[sizeIndex] * m_ElementSizes[sizeIndex];	

	return usedSum;
}


size_t sysMemFixedAllocator::GetMemoryAvailable() {
	size_t freeSum=0;

	for (int sizeIndex=0; sizeIndex<m_FixedSizes; sizeIndex++)
		freeSum += (m_Counts[sizeIndex] - m_Used[sizeIndex]) * m_ElementSizes[sizeIndex];	

	return freeSum;
}


size_t sysMemFixedAllocator::GetLargestAvailableBlock() {
	int sizeIndex;
	for (sizeIndex=m_FixedSizes-1; sizeIndex>=0; sizeIndex--)
	{
		if ( m_Counts[sizeIndex] >  m_Used[sizeIndex])
			 return m_ElementSizes[sizeIndex];	
	}
		
	return 0;
}

u8 sysMemFixedAllocator::GetNumBucketAllocations(const unsigned bucket) const
{
	return bucket < MAX_FIXED_SIZES ? m_Used[bucket] : 0;
}

bool sysMemFixedAllocator::IsValidPointer(const void *ptr) const {
	return (ptr >= m_Bases[0]) && (ptr < (char*)m_Bases[m_FixedSizes-1] + m_ElementSizes[m_FixedSizes-1]*m_Counts[m_FixedSizes-1]);
}

void sysMemFixedAllocator::DumpStats()
{
#if __BANK
	for (int sizeIndex=0; sizeIndex<m_FixedSizes; sizeIndex++)
	{
		const float efficiency = (m_TotalMemUsed[sizeIndex] > 0) ? ((float)m_TotalMemRequested[sizeIndex] / (float)m_TotalMemUsed[sizeIndex]) * 100.0f : 0.0f;
		Displayf("size=%" SIZETFMT "u bytes, used=%u (of %u), highest = %u, efficiency: %.2f%%",m_ElementSizes[sizeIndex], m_Used[sizeIndex], m_Counts[sizeIndex], m_MostUsed[sizeIndex], efficiency);
	} 
#endif
}

} // namespace rage 
