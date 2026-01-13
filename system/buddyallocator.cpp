// 
// system/buddyallocator.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "buddyallocator.h"

#if ENABLE_BUDDY_ALLOCATOR

#include "criticalsection.h"
#include "math/intrinsics.h"
#include "math/amath.h"

#if __WIN32
#include "system/xtl.h"
#else
#include <stdlib.h>
#endif

#include "system/new.h"
#include "system/stack.h"
#include "system/tasklog.h"

#if __DEV
#include "data/struct.h"		// for DEFRAG
#endif

#include "diag/output.h"
#include "diag/tracker.h"
#include "data/marker.h"
#include "system/stack.h"
#include "system/memmanager.h"
#include "system/memvisualize.h"

#include <algorithm>

extern __THREAD int RAGE_LOG_DISABLE;

namespace rage {

static sysCriticalSectionToken s_BuddyAllocatorToken;

// RETURNS: True if input is a power of two.  Note that zero is considered a power of two.
inline bool _IsPowerOfTwo(int value) { return (value & (value-1)) == 0; }

sysMemBuddyAllocator::sysMemBuddyAllocator() : m_HeapBase(NULL), m_Workspace(NULL), m_Size(0), m_LeafSize(0)
{
	RAGE_MEMORY_DEBUG_ONLY(m_watermark = 0;)
}

sysMemBuddyAllocator::sysMemBuddyAllocator(void *heapBase,size_t leafSize,size_t maxPointers,void *workspace) :
	m_HeapBase(heapBase),
	m_Workspace(workspace)
{
	if (!heapBase)
	{
		Quitf(ERR_MEM_VIRTUAL_1,"ExternalAllocator - Out of virtual memory");
	}

	Assertf((leafSize & (128-1)) == 0, "Leafsize %" SIZETFMT "d is not a multiple of 128", leafSize);

	// Reserve a page at top of heap to avoid invalid address exceptions on unrolled loops.
	--maxPointers;

	m_Heap.Init(maxPointers,m_Workspace);
	m_Size = leafSize * maxPointers;
	m_LeafSize = leafSize;

#if !RSG_PPU && !RSG_RSC && !RSG_ORBIS
	// This would be really slow on PS3.
	// It's done in higher-level code in resource compiler
	// And it's impossible on Orbis.
	IF_DEBUG_MEMORY_FILL_N(debug_memory_fill(heapBase, m_Size),DMF_RESOURCE);
#endif

	RAGE_MEMORY_DEBUG_ONLY(m_watermark = 0;)
}


sysMemBuddyAllocator::~sysMemBuddyAllocator()
{
}

#if RESOURCE_POOLS
void sysMemBuddyAllocator::HideUsedMemory(void* ptr)
{
	size_t offset = (char*)ptr-(char*)m_HeapBase;
	sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
	m_Heap.HideUsedMemory(addr);
}

void sysMemBuddyAllocator::InitPool(int pos, void* ptr, size_t capacity, size_t size)
{	
	Assert(ptr && capacity && size);

	m_Heap.InitPool(pos, m_HeapBase, m_LeafSize, ptr, capacity, size);
}
#endif

void* sysMemBuddyAllocator::Allocate(size_t size,
		size_t ASSERT_ONLY(align),
		int 
#if __ASSERT || RAGE_TRACKING
		heapIndex
#endif
		)
{
	Assertf(sysMemStreamingCount > 0, "Streaming Count is NOT > zero! We are leaking memory: %" SIZETFMT "d KB", size >> 10);
	Assertf(sysMemAllowResourceAlloc, "Allocating resource memory needs to be done through the streaming allocator");
#if __PS3
	Assertf(heapIndex!=MEMTYPE_RESOURCE_VIRTUAL||align<=4096,  "sysMemBuddyAllocator::Allocate() only supports 4096 byte alignment on the ps3's on MEMTYPE_RESOURCE_VIRTUAL");
	Assertf(heapIndex!=MEMTYPE_RESOURCE_PHYSICAL||align<=128,  "sysMemBuddyAllocator::Allocate() only supports 128 byte alignment on the ps3 on MEMTYPE_RESOURCE_PHYSICAL");
#elif __XENON
	Assertf(align <= 4096, "sysMemBuddyAllocator::Allocate() only supports 4k alignment on the 360");
#endif

	size_t roundedSize = (m_LeafSize << CeilLeafShift(size, m_LeafSize));
	if (size != roundedSize && (size<<1) != roundedSize) {	// Catch LARGE_BUDDY_HEAP use
		NOTFINAL_ONLY(sysStack::PrintStackTrace());
		Warningf("Allocation of size %" SIZETFMT "d is not power multiple of leafsize (%" SIZETFMT "d), rounded up to %" SIZETFMT "d, losing %" SIZETFMT "d", size, m_LeafSize, roundedSize, roundedSize-size);
		size =  roundedSize;
	}

	sysCriticalSection cs(s_BuddyAllocatorToken);
	sysBuddyNodeIdx addr = m_Heap.Allocate((size+!size + m_LeafSize-1) / m_LeafSize);
	if (addr == sysBuddyHeap::c_NONE)
		return NULL;
	void* p = (char*) m_HeapBase + (addr * m_LeafSize);
#if RAGE_TRACKING
	if (::rage::diagTracker::GetCurrent() && !sysMemVisualize::GetInstance().HasXTL())
		::rage::diagTracker::GetCurrent()->Tally(p, GetSize(p), heapIndex);
#endif
	Assertf(align == 0 || !((size_t)p & (align-1)), "Allocation alignment error were p=%p with alignment=%" SIZETFMT "d", p, align);

#if RAGE_MEMORY_DEBUG
	const size_t used = GetHeapSize() - GetMemoryAvailable();
	if (used > m_watermark)
	{
		m_watermark = used;
	}
#endif

	return p;
}

void*
sysMemBuddyAllocator::TryAllocate(size_t size,size_t align,int heapIndex)
{
    //Because it doesn't Quitf() on failure we can just call Allocate().
    return this->Allocate(size, align, heapIndex);
}


void sysMemBuddyAllocator::Free(const void *ptr)
{
	Assertf(sysMemAllowResourceAlloc, "Freeing resource memory needs to be done through the streaming allocator");

#if MEM_VALIDATE_USERDATA
	Assertf(sysMemCurrentUserData != MEM_INVALID_USERDATA, "Please call MEM_USE_USERDATA before freeing up buddy heap memory to make sure we're tracking frees properly.");
#endif // MEM_VALIDATE_USERDATA

	if (ptr)
	{
#if MEM_VALIDATE_USERDATA
		u32 userData = GetUserData(ptr);
		if (sysMemCurrentUserData != MEM_INVALID_USERDATA && userData != sysMemCurrentUserData)
		{
#if RESOURCE_HEADER
			sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
			if (pAllocator->IsValidPointer(ptr))
			{
				Quitf("Userdata mismatch - We're freeing up data at %p that has userdata %d, but we were expecting %d.", ptr, userData, sysMemCurrentUserData);
				return;
			}
#endif
			// Let's find out a bit more about this node.
			size_t offset = (char*)ptr-(char*)m_HeapBase;
			bool badOffset = (offset % m_LeafSize) != 0;
			sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
			size_t size = m_Heap.GetSize(addr);

			Quitf("Userdata mismatch - We're freeing up data at %p that has userdata %d, but we were expecting %d. Is somebody freeing a stale pointer? %s %s", ptr, userData, sysMemCurrentUserData,
				(badOffset) ? "The offset is bad too. " : "", (size == 0) ? "The data is also likely to be free already." : "");
		}
#endif // MEM_VALIDATE_USERDATA

#if RESOURCE_HEADER
		sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
		if (pAllocator->IsValidPointer(ptr))
		{
			sysMemManager::GetInstance().DeleteNodeInfo(const_cast<void*>(ptr));
			pAllocator->Free(ptr);
			return;
		}
#endif
		sysCriticalSection cs(s_BuddyAllocatorToken);
		size_t offset = (char*)ptr-(char*)m_HeapBase;
		if (offset % m_LeafSize) {
			Errorf("Probable invalid free of resource pointer %p",ptr);
			return;
		}
		sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
		size_t size = m_Heap.GetSize(addr);
		if (size) {
#if RAGE_TRACKING
			if (::rage::diagTracker::GetCurrent() && !sysMemVisualize::GetInstance().HasXTL())
				::rage::diagTracker::GetCurrent()->UnTally((void*)ptr, size * m_LeafSize);
#endif
			RAGE_LOG_DELETE(ptr);
			m_Heap.Free(addr);
		}
		else
			Errorf("Probable invalid free of resource pointer %p",ptr);
	}
}


size_t sysMemBuddyAllocator::GetMemoryUsed(int bucket)
{
	return m_Heap.GetMemoryUsed(bucket) * m_LeafSize;
}


size_t sysMemBuddyAllocator::GetMemoryAvailable()
{
	return m_Heap.GetMemoryFree() * m_LeafSize;
}

size_t sysMemBuddyAllocator::GetLargestAvailableBlock()
{
	return m_Heap.GetLargestAvailableBlock() * m_LeafSize;
}

#if RAGE_MEMORY_DEBUG
size_t sysMemBuddyAllocator::GetHighWaterMark(bool reset)
{
	size_t result = m_watermark;
	if (reset)
		m_watermark = (GetHeapSize() - GetMemoryAvailable());

	return result;
}
#endif

bool sysMemBuddyAllocator::IsValidPointer(const void * ptr) const
{ 
	return ptr >= m_HeapBase && ptr < (char*)m_HeapBase + m_Size; 
}

size_t sysMemBuddyAllocator::GetSize(const void *ptr) const
{
#if RESOURCE_HEADER
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
	if (pAllocator->IsValidPointer(ptr))
		return pAllocator->GetSize(ptr);
#endif

	if (ptr >= m_HeapBase && ptr < (char*)m_HeapBase + m_Size)
	{
		sysCriticalSection cs(s_BuddyAllocatorToken);
		size_t offset = (char*)ptr-(char*)m_HeapBase;
 		if (offset % m_LeafSize)
 			return 0;
		sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
		return m_Heap.GetSize(addr) * m_LeafSize;
	}
	else
		return 0;
}

void sysMemBuddyAllocator::SanityCheck()
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	m_Heap.SanityCheck();
}

bool sysMemBuddyAllocator::GetMemoryDistribution(sysMemDistribution &outDist)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	// This is the allocation distribution in leaves.

	sysMemDistribution dist;
	m_Heap.GetMemoryDistribution(dist);
	memset(&outDist,0,sizeof(sysMemDistribution));
	// "Convert" it to a distribution in bytes
	for (size_t i=0; i<sysBuddyHeap::c_MAX_HEIGHT; ++i) {
		size_t index = _FloorLog2((int)(m_LeafSize << i));
		outDist.FreeBySize[index] += dist.FreeBySize[i];
		outDist.UsedBySize[index] += dist.UsedBySize[i];
	}
	
	return true;
}

bool sysMemBuddyAllocator::Defragment(sysMemDefragmentation& outDefrag, sysMemDefragmentationFree& outDefragFree, size_t maxSize)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);

	sysBuddyHeapDefragInfo info;
	memset(&outDefrag, 0, sizeof(outDefrag));

	unsigned maxHeight = Log2Floor(maxSize / m_LeafSize);

	if (m_Heap.Defragment(info, outDefragFree, m_HeapBase, m_LeafSize, maxHeight))
	{
		outDefrag.Count = info.Count;
		for (int i=0; i<info.Count; i++) {
			outDefrag.Nodes[i].From = (char*)m_HeapBase + (info.Nodes[i].from * m_LeafSize);
			outDefrag.Nodes[i].To = (char*)m_HeapBase + (info.Nodes[i].to * m_LeafSize);
			outDefrag.Nodes[i].Size = (u32)(info.Nodes[i].curSize * m_LeafSize);
		}
		return true;
	}
	else
		return false;
}



size_t sysMemBuddyAllocator::GetFragmentation()
{
	sysCriticalSection cs(s_BuddyAllocatorToken);

	sysMemDistribution dist;

	m_Heap.GetMemoryDistribution(dist); // fills first 16 with increasing multiple of leafsize (HEIGHT)
	size_t largest = 0;

	// If there is at least one block of a particular size, count
	// exactly one block of that size as contributing to the largest block.
	for (size_t i=0; i<32; i++)
		if (dist.FreeBySize[i])
			largest += (m_LeafSize<<i);

	if (GetMemoryAvailable() <= 0)
		return 0;

	return 100 - (size_t)((((u64)largest * 100) / GetMemoryAvailable()));
}

const void *sysMemBuddyAllocator::GetCanonicalBlockPtr(const void *ptr) const
{
#if RESOURCE_HEADER
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
	if (pAllocator->IsValidPointer(ptr))
		return pAllocator->GetCanonicalBlockPtr(ptr);
#endif

	size_t offset = (char*)ptr-(char*)m_HeapBase;
	sysBuddyNodeIdx node = (sysBuddyNodeIdx)(offset / m_LeafSize);
	node = m_Heap.GetFirstNode(node);
	offset = node * m_LeafSize;
	return (char*)m_HeapBase + offset;
}

bool sysMemBuddyAllocator::TryLockBlock(const void *ptr, unsigned lockCount)
{
#if RESOURCE_HEADER
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
	if (pAllocator->IsValidPointer(ptr))
		return sysMemManager::GetInstance().TryLockBlock(ptr, lockCount);
#endif

	sysCriticalSection cs(s_BuddyAllocatorToken);
	size_t offset = (char*)ptr-(char*)m_HeapBase;
	sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
	addr = m_Heap.GetFirstNode(addr);
	return m_Heap.TryLockBlock(addr, lockCount);
}

void sysMemBuddyAllocator::UnlockBlock(const void *ptr, unsigned unlockCount)
{
#if RESOURCE_HEADER
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
	if (pAllocator->IsValidPointer(ptr))
		return sysMemManager::GetInstance().UnlockBlock(ptr, unlockCount);
#endif

	sysCriticalSection cs(s_BuddyAllocatorToken);
	size_t offset = (char*)ptr-(char*)m_HeapBase;
	sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
	addr = m_Heap.GetFirstNode(addr);
	TASKLOG_ASSERTF((unsigned)m_Heap.GetBlockLockCount(addr)>=unlockCount,"Attempting to unlock block at %p that is already fully unlocked! (will crash if ignored)",ptr);
	m_Heap.UnlockBlock(addr, unlockCount);
}

void sysMemBuddyAllocator::SetUserData(const void* ptr, u32 data)
{
#if RESOURCE_HEADER
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
	if (pAllocator->IsValidPointer(ptr))
		return sysMemManager::GetInstance().SetUserData(ptr, data);
#endif

	sysCriticalSection cs(s_BuddyAllocatorToken);
	size_t offset = (char*)ptr-(char*)m_HeapBase;
	Assert(!(offset % m_LeafSize));
	sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
	m_Heap.SetUserData(addr,data);
}


u32 sysMemBuddyAllocator::GetUserData(const void* ptr) const
{
#if RESOURCE_HEADER
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
	if (pAllocator->IsValidPointer(ptr))
		return sysMemManager::GetInstance().GetUserData(ptr);	
#endif

	sysCriticalSection cs(s_BuddyAllocatorToken);
	size_t offset = (char*)ptr-(char*)m_HeapBase;
	Assert(!(offset % m_LeafSize));
	sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
	return m_Heap.GetUserData(addr);
}


int sysMemBuddyAllocator::GetBlockLockCount(const void *ptr)
{
#if RESOURCE_HEADER
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_HEADER_VIRTUAL);
	if (pAllocator->IsValidPointer(ptr))
		return sysMemManager::GetInstance().GetLockCount(ptr);
#endif

	sysCriticalSection cs(s_BuddyAllocatorToken);
	size_t offset = (char*)ptr-(char*)m_HeapBase;
	if (offset % m_LeafSize)
		return 0;
	sysBuddyNodeIdx addr = (sysBuddyNodeIdx)(offset / m_LeafSize);
	return m_Heap.GetBlockLockCount(addr);
}

//=============================================================================
// Growable Buddy Allocator
//=============================================================================

#if RSG_PC

sysMemGrowBuddyAllocator::sysMemGrowBuddyAllocator(size_t leafSize, size_t maxSize RAGE_TRACKING_ONLY(, const char* allocatorName))
{
	m_Size = maxSize;
	m_LeafSize = leafSize;
	m_CurrentIndexToAllocateFrom = -1;
	m_NumOfHeaps = 0;

	for (u32 uIndex = 0; uIndex < sm_MaxHeaps; uIndex++)
	{
		m_apvHeapSpace[uIndex] = NULL;
		m_apWorkspace[uIndex] = NULL;
	}

#if RAGE_TRACKING
	formatf(m_Name, NELEM(m_Name), allocatorName);
#endif

	m_CurrentIndexToAllocateFrom = GrowHeap();
	Assert(m_CurrentIndexToAllocateFrom != -1);
}

s32 sysMemGrowBuddyAllocator::GrowHeap()
{
	s32 iIndex = -1;

	for (iIndex = m_NumOfHeaps; iIndex < sm_MaxHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] == NULL)
		{
			size_t AllocationSize = Min(sm_DefaultHeapSize, m_Size);
			const size_t MinAllocationSize = 16 * 1024 * 1024;
			// We can use this one to grow
			while (AllocationSize >= MinAllocationSize)
			{
				size_t WorkSize = COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(AllocationSize / m_LeafSize);
				WorkSize = (WorkSize + 1023) & ~1023;

				m_apWorkspace[iIndex] = sysMemVirtualAllocate(WorkSize);
				if (m_apWorkspace[iIndex] != NULL)
				{
					m_apvHeapSpace[iIndex] = sysMemVirtualAllocate(AllocationSize);

#if RAGE_TRACKING
					diagTracker* t = diagTracker::GetCurrent();
					if (t && sysMemVisualize::GetInstance().HasResource())
					{
						char buf[64];
						formatf(buf, "%s %d", m_Name, iIndex);
						t->InitHeap(buf, m_apvHeapSpace[iIndex], AllocationSize);
					}
#endif

					// We need a 64K align for scaleform
					Assert(!((size_t)m_apvHeapSpace[iIndex] & ((64 * 1024)-1)));
					if (m_apvHeapSpace[iIndex] != NULL)
					{
						++RAGE_LOG_DISABLE;
						rage_placement_new(&m_aHeaps[iIndex]) sysMemBuddyAllocator(m_apvHeapSpace[iIndex], m_LeafSize, AllocationSize / m_LeafSize, m_apWorkspace[iIndex]);
						--RAGE_LOG_DISABLE;
						m_NumOfHeaps++;
						return iIndex;
					}
					else
					{
						sysMemVirtualFree(m_apWorkspace[iIndex]);
						m_apWorkspace[iIndex] = NULL;
						AllocationSize >>= 1;
					}			
				}
			}
		}
	}
	Assertf(0, "Failed to Grow Heap");
	return -1;
}

sysMemGrowBuddyAllocator::~sysMemGrowBuddyAllocator()
{

}

void* sysMemGrowBuddyAllocator::Allocate(size_t size,size_t align,int heapIndex)
{
	//Because it doesn't Quitf() on failure we can just call Allocate().
	void* ptr = this->TryAllocate(size, align, heapIndex);
	if (ptr)
		return ptr;

	Errorf("Failed to allocate %" SIZETFMT "d from heapIndex %d", size, heapIndex);
	return NULL;
}

void* sysMemGrowBuddyAllocator::TryAllocate(size_t size,size_t align,int heapIndex)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	Assert(m_CurrentIndexToAllocateFrom >= 0 && m_CurrentIndexToAllocateFrom < m_NumOfHeaps);
	
	void* ptr = m_aHeaps[m_CurrentIndexToAllocateFrom].TryAllocate(size, align, heapIndex);
	if (ptr)
		return ptr;

	// Try all allocated heaps to see if it fits anywhere.
	bool bFirstPass = true;
	while(true)
	{
		s32 iIndex = -1;
		for (iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
		{
			if (m_apvHeapSpace[iIndex] != NULL)
			{
				void* ptr = m_aHeaps[iIndex].TryAllocate(size, align, heapIndex);
				if (ptr)
				{
					m_CurrentIndexToAllocateFrom = iIndex;
					return ptr;
				}
			}
		}
		if (bFirstPass)
		{
			GrowHeap();
			bFirstPass = false;
		}
		else
		{			
			m_Size = GetMemoryAvailable() + GetMemoryUsed(-1);
			Warningf("Limiting memory to %" SIZETFMT "u bytes Failed to allocate %" SIZETFMT "d", m_Size, size);

			for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
			{
				Warningf("Fragmentation %" SIZETFMT "d Heap Size %" SIZETFMT "d Largest Available %" SIZETFMT "d Mem Avail %" SIZETFMT "d Mem Used %" SIZETFMT "d",
					m_aHeaps[iIndex].GetFragmentation(), m_aHeaps[iIndex].GetHeapSize(), m_aHeaps[iIndex].GetLargestAvailableBlock(), m_aHeaps[iIndex].GetMemoryAvailable(), 
					m_aHeaps[iIndex].GetMemoryUsed(-1));
			}
			return NULL;
		}
	}
}

void sysMemGrowBuddyAllocator::Free(const void *ptr)
{
	s32 iIndex = -1;
	for (iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{
				m_aHeaps[iIndex].Free(ptr);
				return;
			}
		}
	}
	Errorf("Failed to Free 0x%p from memory", ptr);
}

size_t sysMemGrowBuddyAllocator::GetMemoryUsed(int bucket)
{
	size_t UsedMemory = 0;

	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			UsedMemory += m_aHeaps[iIndex].GetMemoryUsed(bucket);
		}
	}
	return UsedMemory;
}

size_t sysMemGrowBuddyAllocator::GetHeapSize(void) const
{
	size_t HeapSize = 0;

	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			HeapSize += m_aHeaps[iIndex].GetHeapSize();
		}
	}

	return HeapSize;
}

size_t sysMemGrowBuddyAllocator::GetMemoryAvailable()
{
	const size_t MarginOfSafetyAllocation = 4 * 1024 * 1024;
	const size_t MinLargestBlock = 1 * 1024 * 1024;

	size_t iLargestBlock = GetLargestAvailableBlock();
	if (iLargestBlock < MinLargestBlock)
		return iLargestBlock;

	do 
	{
		size_t HeapSize = 0;

		for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
		{
			if (m_apvHeapSpace[iIndex] != NULL)
			{
				HeapSize += m_aHeaps[iIndex].GetHeapSize();
			}
		}

		if ((HeapSize - MarginOfSafetyAllocation) < GetMemoryUsed(-1))
		{
			if (GrowHeap() == -1)
			{
				if (m_Size != HeapSize)
				{
					m_Size = HeapSize;
					Warningf("GetMemoryAvailable - Limiting memory to %" SIZETFMT "u bytes", m_Size);
				}		
				return 0;
			}
		}
		else
		{
			return HeapSize - GetMemoryUsed(-1);
		}
	} while(true);
}

size_t sysMemGrowBuddyAllocator::GetLargestAvailableBlock()
{
	const size_t MarginOfSafetyAllocation = 4 * 1024 * 1024;

	do 
	{
		size_t LargestBlock = 0;

		for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
		{
			if (m_apvHeapSpace[iIndex] != NULL)
			{
				if (LargestBlock < m_aHeaps[iIndex].GetLargestAvailableBlock())
					LargestBlock = m_aHeaps[iIndex].GetLargestAvailableBlock();
			}
		}

		if (LargestBlock < MarginOfSafetyAllocation)
		{
			if (GrowHeap() == -1)
			{
				return LargestBlock;
			}
		}
		else
		{
			return LargestBlock;
		}
	} while(true);
}

bool sysMemGrowBuddyAllocator::GetMemoryDistribution(sysMemDistribution & outDist)
{
	sysMemDistribution Distribution;
	memset(&outDist, 0, sizeof(outDist));
	bool bRet = true;

	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			bRet &= m_aHeaps[iIndex].GetMemoryDistribution(Distribution);
			for (int i=0; i<32; i++) {
				outDist.FreeBySize[i] += Distribution.FreeBySize[i];
				outDist.UsedBySize[i] += Distribution.UsedBySize[i];
			}
		}
	}
	return bRet;
}

bool sysMemGrowBuddyAllocator::IsValidPointer(const void * ptr) const
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
				return true;
		}
	}
	return false;
}

size_t sysMemGrowBuddyAllocator::GetSize(const void *ptr) const
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{
				return m_aHeaps[iIndex].GetSize(ptr);
			}
		}
	}
	return 0;
}

bool sysMemGrowBuddyAllocator::Defragment(sysMemDefragmentation& outDefrag, sysMemDefragmentationFree& outDefragFree, size_t maxSize)
{
	unsigned maxHeight = Log2Floor(maxSize / m_LeafSize);

	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].Defragment(outDefrag, outDefragFree, maxHeight))
			{
				return true;
			}
		}
	}
	return false;
}

void sysMemGrowBuddyAllocator::SanityCheck()
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			m_aHeaps[iIndex].SanityCheck();
		}
	}
}

const void* sysMemGrowBuddyAllocator::GetCanonicalBlockPtr(const void *ptr) const
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{
				return m_aHeaps[iIndex].GetCanonicalBlockPtr(ptr);
			}
		}
	}
	return NULL;
}

bool sysMemGrowBuddyAllocator::TryLockBlock(const void *ptr, unsigned lockCount)
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{
				return m_aHeaps[iIndex].TryLockBlock(ptr, lockCount);
			}
		}
	}
	return false;
}

void sysMemGrowBuddyAllocator::UnlockBlock(const void *ptr, unsigned unlockCount)
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{
				m_aHeaps[iIndex].UnlockBlock(ptr, unlockCount);
				return;
			}
		}
	}
	Assertf(0, "Failed to UnlockBlock");
}

u32 sysMemGrowBuddyAllocator::GetUserData(const void *ptr) const
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{				
				return m_aHeaps[iIndex].GetUserData(ptr);
			}
		}
	}
	Assertf(0, "Failed to GetUserData");
	return 0;
}

void sysMemGrowBuddyAllocator::SetUserData(const void *ptr,u32 data)
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{
				m_aHeaps[iIndex].SetUserData(ptr, data);
				return;
			}
		}
	}
	Assertf(0, "Failed to SetUserData 0x%p %u", ptr, data);
}

size_t sysMemGrowBuddyAllocator::GetFragmentation()
{
	size_t iFragmentation = 0;
	int iCount = 0;

	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			iFragmentation += m_aHeaps[iIndex].GetFragmentation();
			iCount++;
		}
	}
	return iFragmentation / iCount;
}

int sysMemGrowBuddyAllocator::GetBlockLockCount(const void *ptr)
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{
				return m_aHeaps[iIndex].GetBlockLockCount(ptr);
			}
		}
	}
	Assertf(0, "Failed to find GetBlockLockCount 0x%p", ptr);
	return 0xFFFFFFFF;
}

sysBuddyNodeIdx sysMemGrowBuddyAllocator::GetNodeIdxFromPtr(void* ptr) const
{
	for (s32 iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			if (m_aHeaps[iIndex].IsValidPointer(ptr))
			{
				return GetNodeIdxFromPtr(ptr);
			}
		}
	}
	Assertf(0, "Failed to find GetNodeIdxFromPtr 0x%p", ptr);
	return (sysBuddyNodeIdx)-1;
}

void* sysMemGrowBuddyAllocator::GetPtrFromNodeIdx(sysBuddyNodeIdx /*index*/) const
{
	Assertf(0, "Don't support this (yet?)");
	return NULL;
};

#if RAGE_TRACKING
void sysMemGrowBuddyAllocator::TrackExistingPages(diagTracker& t) const
{
	for (int iIndex = 0; iIndex < m_NumOfHeaps; iIndex++)
	{
		if (m_apvHeapSpace[iIndex] != NULL)
		{
			char buf[64];
			formatf(buf, "%s %d", m_Name, iIndex);
			t.InitHeap(buf, m_apvHeapSpace[iIndex], m_aHeaps[iIndex].GetHeapSize());
		}
	}
}
#endif

//=============================================================================
// End of Growable Buddy Allocator
//=============================================================================

// Node lock counts saturate at one less than the largest possible value
#define LOCK_COUNT_SATURATE     ((1<<sysMemSparseBuddyAllocator::BuddyData::BITS_LOCKCOUNT)-2)

// The largest possible lock count value is reserved for indicating that a
// defrag move is in progress
#define LOCK_COUNT_DEFRAG       ((1<<sysMemSparseBuddyAllocator::BuddyData::BITS_LOCKCOUNT)-1)

sysMemSparseBuddyAllocator::sysMemSparseBuddyAllocator(size_t heapSize,size_t leafSize,size_t leafCount,void *workspace) 
: m_SparseAllocator(heapSize, 0)
, m_LeafSize(leafSize)
, m_LeafCount(leafCount)
{
	m_BuddyDataEnd = m_BuddyData = (BuddyData*) workspace;
	m_BuddyDataCapacity = m_BuddyData + leafCount;
	m_DeferredFree = NULL;
}

sysMemSparseBuddyAllocator::~sysMemSparseBuddyAllocator()
{
}

void* sysMemSparseBuddyAllocator::Allocate(size_t size,size_t align,int heapIndex)
{
	return TryAllocate(size,align,heapIndex);
}

struct SortByAddr {
	// std::lower_bound returns a pointer to leftmost element in the list
	// that still satisfies this predicate.
	bool operator()(const sysMemSparseBuddyAllocator::BuddyData &a,const sysMemSparseBuddyAllocator::BuddyData &b) {
		return (size_t)a.memory < (size_t)b.memory;
	}
};

struct SortByRange {
	// std::lower_bound returns a pointer to leftmost element in the list
	// that still satisfies this predicate.
	bool operator()(const sysMemSparseBuddyAllocator::BuddyData &a,const sysMemSparseBuddyAllocator::BuddyData &b) {
		return (size_t)a.memory+a.size <= (size_t)b.memory;
	}
};

void* sysMemSparseBuddyAllocator::TryAllocate(size_t size,size_t align,int heapIndex)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	if (m_DeferredFree)
	{
		m_SparseAllocator.Free(m_DeferredFree);
		m_DeferredFree = NULL;
	}

	// Make sure pages are aligned to 8k, the "old" page size on 32 bit systems.
	// Even stricter if explicitly requested.
	void *result = m_SparseAllocator.TryAllocate(size,align<8192?8192:align,heapIndex);

	if (result) {
		Assert(m_BuddyDataEnd != m_BuddyDataCapacity);
		BuddyData *end = m_BuddyDataEnd++;
		// We want the location where we should be inserting this new allocation
		BuddyData search; search.memory = result;
		BuddyData *b = std::lower_bound(m_BuddyData,end,search,SortByAddr());
		// Downward copy to free up space for new element
		while (end > b)
		{
			end[0] = end[-1];
			--end;
		}
		// Insert new element into sorted array
		b->memory = result;
		b->userData = ((1u<<BuddyData::BITS_USERDATA)-1);
		b->lockCount = 1;		// blocks start off locked
		b->size = u32(size);
	}
	return result;
}

sysMemSparseBuddyAllocator::BuddyData *sysMemSparseBuddyAllocator::GetBuddyData(const void *ptr) const
{
	// We want the BuddyData that contains this block
	BuddyData search; search.memory = const_cast<void*>(ptr); search.size = 1;
	BuddyData *b = std::lower_bound(m_BuddyData,m_BuddyDataEnd,search,SortByRange());
	return (b == m_BuddyDataEnd || b->memory > ptr)? NULL : b;
}

void sysMemSparseBuddyAllocator::Free(const void *ptr)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	if (m_DeferredFree)
	{
		m_SparseAllocator.Free(m_DeferredFree);
		m_DeferredFree = NULL;
	}

	BuddyData *b = GetBuddyData(ptr);
	Assert(b != NULL);
	// Copy array downward to maintain sorted order
	m_BuddyDataEnd--;
	for (; b < m_BuddyDataEnd; b++)
		b[0] = b[1];

	m_SparseAllocator.Free(ptr);
}

void sysMemSparseBuddyAllocator::DeferredFree(const void *ptr)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	if (m_DeferredFree)
		m_SparseAllocator.Free(m_DeferredFree);

	BuddyData *b = GetBuddyData(ptr);
	// Copy array downward to maintain sorted order
	m_BuddyDataEnd--;
	for (; b < m_BuddyDataEnd; b++)
		b[0] = b[1];

	m_DeferredFree = const_cast<void*>(ptr);
}

int sysMemSparseBuddyAllocator::GetBlockLockCount(const void *ptr)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	BuddyData *b = GetBuddyData(ptr);
	Assert(b != NULL);
	return b->lockCount;
}

const void *sysMemSparseBuddyAllocator::GetCanonicalBlockPtr(const void *ptr) const
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	BuddyData *b = GetBuddyData(ptr);
	return b->memory;
}

bool sysMemSparseBuddyAllocator::TryLockBlock(const void *ptr, unsigned lockCount)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	BuddyData *b = GetBuddyData(ptr);
	if (b->lockCount == LOCK_COUNT_DEFRAG)
		return false;
	else if (b->lockCount+lockCount < LOCK_COUNT_SATURATE)
		b->lockCount += lockCount;
	else
		b->lockCount = LOCK_COUNT_SATURATE;
	return true;
}

void sysMemSparseBuddyAllocator::UnlockBlock(const void *ptr, unsigned unlockCount)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	BuddyData *b = GetBuddyData(ptr);
	Assert(b->lockCount >= unlockCount);
	if (b->lockCount == LOCK_COUNT_DEFRAG)
		b->lockCount = 0;
	else if (b->lockCount < LOCK_COUNT_SATURATE)
		b->lockCount -= unlockCount;
}

void sysMemSparseBuddyAllocator::SetUserData(const void *ptr,u32 val)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	BuddyData *b = GetBuddyData(ptr);
	Assertf(b,"%p->SetUserData(%p,%u) called on unknown memory address.",this,ptr,val);
	b->userData = val;
	Assert(val == ~0U || b->userData == val);
}

u32 sysMemSparseBuddyAllocator::GetUserData(const void *ptr) const
{
	sysCriticalSection cs(s_BuddyAllocatorToken);
	BuddyData *b = GetBuddyData(ptr);
	return b->userData==((1u<<BuddyData::BITS_USERDATA)-1)? ~0U : (u32)b->userData;
}

bool sysMemSparseBuddyAllocator::Defragment(sysMemDefragmentation &outDefrag, sysMemDefragmentationFree& UNUSED_PARAM(outDefragFree), size_t /*maxSize*/)
{
	sysCriticalSection cs(s_BuddyAllocatorToken);

	// Lowest-numbered non-locked block is suitable for defrag.
	memset(&outDefrag,0,sizeof(outDefrag));

	// Note - if heap consisted ENTIRELY of one-leaf allocations, this would overflow by one!
	for (BuddyData *i=m_BuddyData; i<m_BuddyDataEnd; i++)
	{
		if (!i->lockCount)
		{
			outDefrag.Count = 1;
			outDefrag.Nodes[0].From = i->memory;
			i->lockCount = LOCK_COUNT_DEFRAG;
			size_t size = i->size;
			// Copy user data because the pointer is no longer valid after TryAllocate
			u32 userData = i->userData;
			// This will scramble m_BuddyData array, so don't access m_BuddyData[i] any longer:
			outDefrag.Nodes[0].To = TryAllocate(size,m_LeafSize,0);
			if (outDefrag.Nodes[0].To)
			{
				// Displayf("Fake defrag of %x bytes from %p to %p",size,outDefrag.Nodes[0].From,outDefrag.Nodes[0].To);
				BuddyData *to = GetBuddyData(outDefrag.Nodes[0].To);
				to->userData = userData;
				outDefrag.Nodes[0].Size = u32(size);
				return true;
			}
			else
				return false;
		}
	}
	return false;
}

bool sysMemSparseBuddyAllocator::IsValidPointer(const void *ptr) const
{
	return m_SparseAllocator.IsValidPointer(ptr);
}

size_t sysMemSparseBuddyAllocator::GetSize(const void *ptr) const
{
	return m_SparseAllocator.GetSize(ptr);
}

size_t sysMemSparseBuddyAllocator::GetMemoryUsed(int bucket)
{
	return m_SparseAllocator.GetMemoryUsed(bucket);
}

size_t sysMemSparseBuddyAllocator::GetHeapSize(void) const
{
	return m_SparseAllocator.GetHeapSize();
}

size_t sysMemSparseBuddyAllocator::GetMemoryAvailable()
{
	return m_SparseAllocator.GetMemoryAvailable();
}

size_t sysMemSparseBuddyAllocator::GetLargestAvailableBlock()
{
	return m_SparseAllocator.GetLargestAvailableBlock();
}

#undef LOCK_COUNT_SATURATE
#undef LOCK_COUNT_DEFRAG

#endif // RSG_PC

} // namespace rage

#endif		// ENABLE_BUDDY_ALLOCATOR
