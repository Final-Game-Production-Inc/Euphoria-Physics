#ifndef SYSTEM_VIRTUALALLOCATOR_H
#define SYSTEM_VIRTUALALLOCATOR_H

#include <stddef.h>		// for size_t
#include <map>

#include "system/memory.h"
#include "system/criticalsection.h"
#include "system/tinybuddyheap.h"
#include "data/resource.h"
#include "atl/array.h"

#if RSG_ORBIS
#include <kernel.h>
#endif


#if RSG_ORBIS || RSG_DURANGO

namespace rage {


class sysMemVirtualAllocator: public sysMemAllocator {
public:
	// We use 64k pages on all platforms to minimize TLB misses.
	static const size_t BuddyPageSize = 65536;
	static const size_t BuddyPageSizeMask = BuddyPageSize-1;

	static sysMemVirtualAllocator *sm_Instance;

	sysMemVirtualAllocator(size_t maxHeapSize);

	~sysMemVirtualAllocator() {sm_Instance = NULL;}

	void *AllocateMemtype(size_t size,size_t align,u32 memtype);
	void *Allocate(size_t size,size_t align,int /*heapIndex*/=0) {
		return AllocateMemtype(size,align,MEMTYPE_DEFAULT);
	}
	void Free(const void*);
	size_t GetMemoryUsed(int /*heapIndex*/) {
#if RSG_PC
		return m_PhysicalHeapUsed;
#else
		CompileTimeAssert(POOLSHIFT==3);
		return m_PhysicalHeap.GetNodesUsed() * BuddyPageSize -
			m_FreeBySize[0] * (BuddyPageSize >> POOLSHIFT) -
			m_FreeBySize[1] * (BuddyPageSize >> (POOLSHIFT-1)) -
			m_FreeBySize[2] * (BuddyPageSize >> (POOLSHIFT-2));
#endif
	}
	size_t GetMemoryAvailable() { 
		return m_PhysicalHeapSize - GetMemoryUsed(0); 
	}
	size_t GetHeapSize() const { 
		return m_PhysicalHeapSize; 
	}
	size_t GetSize(const void *ptr) const;
	size_t GetLargestAvailableBlock() { 
		return GetMemoryAvailable(); 
	}

#if RSG_ORBIS || RSG_DURANGO
	NOTFINAL_ONLY(void* GetHeapBase() const { return m_VirtualBase; })
#endif
	const void *GetCanonicalBlockPtr(const void * /*ptr*/) const;
	void *GetCanonicalBlockPtr(void *ptr) const { 
		return const_cast<void*>(GetCanonicalBlockPtr(const_cast<const void*>(ptr))); 
	}
	u32 GetUserData(const void *ptr) const;
	void SetUserData(const void *ptr, u32 userData);
	bool IsValidPointer(const void * /*ptr*/) const;

#if RSG_ORBIS || RSG_DURANGO
	// Only worth doing this on Orbis (which is currently a single heap) because the mapping calls are expensive.
	// On gta5, Durango uses a single heap too so might as well enable it there as well.
	bool SupportsAllocateMap() const { 
		return true; 
	}
#endif
	bool AllocateMap(datResourceMap &map);
	void FreeMap(const datResourceMap &map);

	void SetMemTypeKeepContents(const void *addr, size_t size, u32 memtype);
	void SetMemTypeDiscardContents(const void *addr, size_t size, u32 memtype);
	u32 GetMemType(const void *addr);

#	define MAX_NUM_PHYSICAL_ADDR_SIZE_PAIR_ENTRIES(ALLOC_SIZE_BYTES) \
		((((ALLOC_SIZE_BYTES)+::rage::sysMemVirtualAllocator::BuddyPageSizeMask)/::rage::sysMemVirtualAllocator::BuddyPageSize)*2)

	// Allocate physical memory.  No virtual memory is allocated.  Multiple
	// allocations will be made if necissary.
	//
	// outPaSizePairs must point to an array of {physical address, byte size} pairs.
	// Use MAX_NUM_PHYSICAL_ADDR_SIZE_PAIR_ENTRIES to allocate a sufficiently large array.
	//
	// Returns number of entries in outPaSizePairs used.  0 indicates allocation
	// failure.
	//
	// Note that the "physical addresses" being returned are not the real
	// physical address at all.  Rather they are a unique identifier for a
	// physical address.  The meaning of the numerical value should be treated
	// as opaque by calling code.
	//
	u32 AllocatePhysical(size_t *outPaSizePairs, size_t size);

	void FreePhysical(size_t physicalAddr);
	void FreePhysical(const size_t *paSizePairs, u32 numPairs);

	// Allocate a single contigous block of virtual memory.  No physical memory is allocated.
	void *AllocateVirtual(size_t size);

	void FreeVirtual(void *virtualAddr);

	void MapVirtualToPhysical(void *virtualAddr, const size_t *paSizePairs, u32 numPairs, u32 memtype);

	void UnmapVirtual(void *virtualAddr, size_t size);

	u32 GetPhysicalAddressSizePairs(size_t *outPaSizePairs, const void *virtualAddr, size_t size);

	static size_t ComputeAllocationSize(size_t size);

private:
	static const u32 POOLSHIFT = 3;
	static const u8 sm_FullBits[POOLSHIFT], sm_PagesBySize[POOLSHIFT];
	static const size_t PooledBuddyPageSize = BuddyPageSize >> POOLSHIFT;
	static const size_t PooledBuddyPageSizeMask = PooledBuddyPageSize-1;
	size_t m_PhysicalHeapSize;
	mutable sysCriticalSectionToken m_CS;
#if RSG_ORBIS || RSG_DURANGO
	sysTinyBuddyHeap m_PhysicalHeap, m_VirtualHeap;
	atRangeArray<u32,POOLSHIFT> m_FreeBySize;
	atRangeArray<u32,POOLSHIFT> m_FirstBySize;
	u32 *m_PoolUserData;
	void *m_VirtualBase;
	size_t m_PhysicalPageCount;
	size_t m_VirtualPageCount;
#if RSG_ORBIS
	size_t m_PhysicalBase;
#else
	size_t *m_PageArray;
	void AddMapping(void *_addr,u32 _page,size_t size,u32 memtype);
	size_t m_VirtualAllocated;
#endif
#endif

#if RSG_ORBIS
	struct BatchMapping {
		unsigned num;
		ASSERT_ONLY(unsigned maxNum;)
		SceKernelBatchMapEntry entries[1];

		inline explicit BatchMapping(unsigned maxNum_)
			: num(0)
			ASSERT_ONLY(, maxNum(maxNum_)) {
		}
	};
	static void DoBatchMapping(BatchMapping *bm);
#endif

public: // TODO: THIS IS ONLY PUBLIC FOR TEMPORARY TESTING PURPOSES, SHOULD BE MADE PRIVATE AGAIN
		// SHOULD ALSO HAVE INLINE COMMENTED BACK IN.
	/*inline*/ bool IsPooled(const void *ptr) const;
private:
	inline u32 GetVirtualPoolIndex(const void *ptr) const;
	inline u32 GetVirtualPageIndex(const void *ptr) const;
	inline void *GetVirtualPointer(u32 idx) const;

	class PageCounter;
	void *AllocateInternal(size_t size, u32 memtype ORBIS_ONLY(, BatchMapping *bm));
	void FreeInternal(void *ptr, size_t size ORBIS_ONLY(, BatchMapping *bm));
	void SetSmallPageDefaultMemType(void *ptr);
};

}		// namespace rage

#endif	// RSG_ORBIS || RSG_DURANGO

#endif
