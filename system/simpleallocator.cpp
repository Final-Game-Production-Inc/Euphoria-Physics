// 
// system/simpleallocator.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "simpleallocator.h"
#include "ipc.h"
#include "stack.h"
#include "criticalsection.h"
#include "param.h"

#include "atl/pool.h"
#include "bank/group.h"
#include "diag/output.h"
#include "diag/tracker.h"
#if !__NO_OUTPUT
#include "diag/hexdump.h"
#endif
#include "file/stream.h"
#include "data/marker.h"

#include "math/amath.h"
#include "system/memmanager.h"
#include "system/memvisualize.h"
#include "system/splitallocator.h"

#include <stdlib.h>
#include <string.h>

#if __WIN32
#include "system/xtl.h"
#endif	// __WIN32

namespace rage {

bool		sysMemSimpleAllocator::sm_leaksAsErrors = false;
bool		sysMemSimpleAllocator::sm_hideAllocOutput = false;

#define HEAP_ID_SHIFT		(23)

#if !__FINAL && !__SPU
PARAM(verifyfreelist,"[memory] Perform a sanity check of the free list");
#endif

#if __DEV && !__SPU
PARAM(sanitycheck,"[memory] Perform a sanity check before every C++ allocation (slow but effective for tracking memory stomps)");
PARAM(sanitycheckfree,"[memory] Perform a sanity check before every C++ deallocation (slow but effective for tracking memory stomps)");
#endif

#if __SPU
#define SIMPLE_ALLOCATOR_CRITICAL_SECTION
#else
#define SIMPLE_ALLOCATOR_CRITICAL_SECTION sysCriticalSection cs(m_Token)
#endif

sysMemSimpleAllocator::sysMemSimpleAllocator( void* heap, const int heapSize, const int heapId, bool allowSmallAllocator )
: 
m_QuitOnFailure(DEFAULT_QUIT_ON_FAILURE)
#if __DEV
, m_sanityCheckHeapOnDoAllocate(false)
#endif
{
#if RAGE_TRACKING
	m_IsTallied = true;
#endif
	m_OwnHeap = false;
    m_OrigHeapSize = heapSize;
	m_MemAllocId = 1 + (heapId << HEAP_ID_SHIFT);

    this->InitHeap( heap, heapSize, allowSmallAllocator );
}

sysMemSimpleAllocator::sysMemSimpleAllocator(int heapSize, const int heapId, bool allowSmallAllocator ) 
: 
m_QuitOnFailure(DEFAULT_QUIT_ON_FAILURE)
#if __DEV
, m_sanityCheckHeapOnDoAllocate(false)
#endif
{
#if RAGE_TRACKING
	m_IsTallied = true;
#endif
    m_OwnHeap = true;
    m_OrigHeapSize = heapSize;
	m_MemAllocId = 1 + (heapId << HEAP_ID_SHIFT);
    void* heap;

	heap = sysMemPhysicalAllocate(heapSize);
	if (!heap)
	{
		RAGE_TRACK_REPORT();
		Quitf(ERR_MEM_EMBEDDEDALLOC_INIT_1,"Unable to allocate %dK heap",(heapSize>>10));
	}

    this->InitHeap( heap, heapSize, allowSmallAllocator );
}

void sysMemSimpleAllocator::InitSmallocator()
{
#if ENABLE_SMALLOCATOR_CODE
#if RAGE_MEMORY_DEBUG
	sysMemSet(m_smAllocationCount, 0, sizeof(u32) * SM_ALLOCATOR_SIZE);
	sysMemSet(m_smAllocationMax, 0, sizeof(u32) * SM_ALLOCATOR_SIZE);
#endif // !__FINAL

	u16 size = SM_ALLOCATOR_INCREMENT;
	for (int i = 0; i < SM_ALLOCATOR_SIZE; ++i, size += SM_ALLOCATOR_INCREMENT)
		m_smallAllocator[i].Create(size
#			if SMALL_ALLOCATOR_DEBUG_MIN_ALIGNMENT
				// If size is not a multiple of 16, then add twice the implied alignment (ie, least significant bit)
				+ ((size&15) ? 2*(size&-(s16)size) : 0)
#			endif
			);
#endif // ENABLE_SMALLOCATOR_CODE
}

sysMemSimpleAllocator::~sysMemSimpleAllocator() 
{
    if( m_OwnHeap )
    {
		sysMemPhysicalFree(m_OrigHeap);
    }
}

bool sysMemSimpleAllocator::SetQuitOnFail(const bool quitOnFail)
{
    const bool oldVal = m_QuitOnFailure;
    m_QuitOnFailure = quitOnFail;

    return oldVal;
}

inline int sysMemSimpleAllocator::GetFreeListIndex(size_t size) 
{
	// Optimization to make better use of free lists (more even distribution)
	return Log2Floor(size);
}

void sysMemSimpleAllocator::AddToFreeList(FreeNode *fn,int which) 
{
	fn->NextFree = m_FreeLists[which];
	if (m_FreeLists[which])
		m_FreeLists[which]->PrevFree = fn;
	fn->PrevFree = NULL;
	m_FreeLists[which] = fn;
}

void sysMemSimpleAllocator::RemoveFromFreeList(FreeNode *fn,int which) 
{
	// Tell our predecessor to point to our successor
	if (fn->PrevFree)
		fn->PrevFree->NextFree = fn->NextFree;
	else
		m_FreeLists[which] = fn->NextFree;

	// Tell our successor to point to our predecessor
	if (fn->NextFree)
		fn->NextFree->PrevFree = fn->PrevFree;
}

#if __WIN32 && !__FINAL
static fiStream *s_Stream;
static int s_Skipper;

static void s_DisplayLine(size_t addr,const char *sym,size_t offset) 
{
	if (s_Skipper)
		--s_Skipper;
	else
		fprintf(s_Stream,"\"%"SIZETFMT"x:%s+%"SIZETFMT"x\",",addr,sym,offset);
}
#endif

#if ENABLE_SMALLOCATOR_CODE
sysSmallocator* sysMemSimpleAllocator::GetSmallocator(size_t size) 
{
	Assert(size <= SM_ALLOCATOR_RANGE);

	// Make sure we size correctly
	if (size < SM_ALLOCATOR_INCREMENT)
		size = SM_ALLOCATOR_INCREMENT;
	else
		size = AlignMem(size, SM_ALLOCATOR_INCREMENT);

	const int index = static_cast<int>((size / SM_ALLOCATOR_INCREMENT) - 1);
	return &m_smallAllocator[index];
}

inline sysSmallocator::Chunk* sysMemSimpleAllocator::IsSmallocatorChunk(const void *ptr) const 
{
	// A smallocator chunk doesn't own an entire 16KB page - the last
	// 64 bytes belong to the regular heap.
	if ( (((size_t) ptr) & (sysSmallocator::CHUNK_ALIGNMENT-1)) >= sysSmallocator::CHUNK_ALIGNMENT - sysSmallocator::MEMORY_SYSTEM_OVERHEAD)
		return NULL;

	int idx = GetPageIndex(ptr);
	if (m_PageArray.IsSet(idx))
		return ((sysSmallocator::Chunk*)((size_t)ptr & ~(sysSmallocator::CHUNK_ALIGNMENT-1)));
	else
		return NULL;
}

#if RAGE_MEMORY_DEBUG
void sysMemSimpleAllocator::DoDebugAllocate(const void* ptr)
{
	size_t index;
	const size_t size = GetSize(ptr);

	if (size <= SM_ALLOCATOR_RANGE)
		index = (GetSize(ptr) / SM_ALLOCATOR_INCREMENT) - 1;
	else
		index = SM_ALLOCATOR_SIZE - 1;

	m_smAllocationCount[index]++;
	if (m_smAllocationCount[index] > m_smAllocationMax[index])
		m_smAllocationMax[index] = m_smAllocationCount[index];
}

void sysMemSimpleAllocator::DoDebugFree(const void* ptr)
{
	size_t index;
	const size_t size = GetSize(ptr);

	if (size <= SM_ALLOCATOR_RANGE)
		index = (GetSize(ptr) / SM_ALLOCATOR_INCREMENT) - 1;
	else
		index = SM_ALLOCATOR_SIZE - 1;

	m_smAllocationCount[index]--;
}

#if !__NO_OUTPUT
void sysMemSimpleAllocator::DumpSMAllocators()
{
	size_t usedBytes = 0;
	size_t totalSmall64 = 0;
	size_t totalSmall128 = 0;
	size_t totalSmall256 = 0;	
	size_t freeSmall64 = 0;
	size_t freeSmall128 = 0;
	size_t freeSmall256 = 0;
	
	const size_t OLD_MAX = 6;	
	const size_t oldSmallBytes[OLD_MAX] = {4, 8, 16, 32, 64, 80};	
	
	size_t freeBytes[OLD_MAX] = {0};

	size_t usedCurrentBytes = 0;
	size_t usedCurrentCount = 0;
	size_t usedCurrent[OLD_MAX] = {0};
	size_t usedCurrentWasted[OLD_MAX] = {0};

	size_t usedMaxBytes = 0;
	size_t usedMaxCount = 0;
	size_t usedMax[OLD_MAX] = {0};
	size_t usedMaxWasted[OLD_MAX] = {0};

	Displayf("");
	for (size_t i = 0, bytes = SM_ALLOCATOR_INCREMENT; bytes <= SM_ALLOCATOR_RANGE; ++i, bytes += SM_ALLOCATOR_INCREMENT)
	{
		Displayf("[%03" SIZETFMT "d] = %u (Max = %u)", bytes, m_smAllocationCount[i], m_smAllocationMax[i]);

		sysSmallocator* pAllocator = GetSmallocator(bytes);		

		// Old smallocators
		for (size_t j = 0; j < OLD_MAX; ++j)
		{
			if (bytes <= oldSmallBytes[j])
			{
				freeBytes[j] += pAllocator->GetMemoryAvailable();

				usedCurrentWasted[j] += ((oldSmallBytes[j] - bytes) * m_smAllocationCount[i]);
				usedCurrent[j] += (bytes * m_smAllocationCount[i]);
				usedCurrentBytes += (bytes * m_smAllocationCount[i]);
				usedCurrentCount += m_smAllocationCount[i];

				usedMaxWasted[j] += ((oldSmallBytes[j] - bytes) * m_smAllocationMax[i]);
				usedMax[j] += (bytes * m_smAllocationMax[i]);
				usedMaxBytes += (bytes * m_smAllocationMax[i]);
				usedMaxCount += m_smAllocationMax[i];
				
				break;
			}
		}

		if (bytes <= 64)
		{
			totalSmall64 += m_smAllocationMax[i];
			freeSmall64 += pAllocator->GetMemoryAvailable();
		}
		else if (bytes <= 128)
		{
			totalSmall128 += m_smAllocationMax[i];
			freeSmall128 += pAllocator->GetMemoryAvailable();
		}
		else
		{
			totalSmall256 += m_smAllocationMax[i];
			freeSmall256 += pAllocator->GetMemoryAvailable();
		}

		usedBytes += (bytes * m_smAllocationMax[i]);
	}

	size_t prev = 0;
	size_t usedCurrentTotalWasted = 0;
	size_t usedMaxTotalWasted = 0;

	for (size_t i = 0; i < OLD_MAX; ++i)
	{
		Displayf("");
		Displayf("[%03" SIZETFMT "d]", oldSmallBytes[i]);
		Displayf("Current        (%02" SIZETFMT "d - %02" SIZETFMT "d) = %" SIZETFMT "u, %" SIZETFMT "u KB", prev, oldSmallBytes[i], (usedCurrent[i] / oldSmallBytes[i]), (usedCurrent[i] / 1024));
		Displayf("Current Wasted (%02" SIZETFMT "d - %02" SIZETFMT "d) = %" SIZETFMT "u KB", prev, oldSmallBytes[i], (usedCurrentWasted[i] / 1024));
		Displayf("Current Free   (%02" SIZETFMT "d - %02" SIZETFMT "d) = %" SIZETFMT "u KB", prev, oldSmallBytes[i], (freeBytes[i] / 1024));
		Displayf("Max            (%02" SIZETFMT "d - %02" SIZETFMT "d) = %" SIZETFMT "u, %" SIZETFMT "u KB", prev, oldSmallBytes[i], (usedMax[i] / oldSmallBytes[i]), (usedMax[i] / 1024));		
		Displayf("Max Wasted     (%02" SIZETFMT "d - %02" SIZETFMT "d) = %" SIZETFMT "u KB", prev, oldSmallBytes[i], (usedMaxWasted[i] / 1024));			

		prev = oldSmallBytes[i] + 1;
		usedCurrentTotalWasted += usedCurrentWasted[i];
		usedMaxTotalWasted += usedMaxWasted[i];
	}

	Displayf("");
	Displayf("Current Count        (04 - 80) = %" SIZETFMT "u", usedCurrentCount);
	Displayf("Current Bytes	       (04 - 80) = %" SIZETFMT "u KB", usedCurrentBytes / 1024);	
	Displayf("Current Wasted Bytes (04 - 80) = %" SIZETFMT "u KB", usedCurrentTotalWasted / 1024);

	Displayf("Max Count        (04 - 80) = %" SIZETFMT "u", usedMaxCount);
	Displayf("Max Bytes        (04 - 80) = %" SIZETFMT "u KB", usedMaxBytes / 1024);
	Displayf("Max Wasted Bytes (04 - 80) = %" SIZETFMT "u KB", usedMaxTotalWasted / 1024);

	Displayf("");
	Displayf("Number of Small Allocations (0   - 256) = %" SIZETFMT "u", totalSmall64 + totalSmall128 + totalSmall256);
	Displayf("Number of Small Allocations (0   -  64) = %" SIZETFMT "u", totalSmall64);
	Displayf("Number of Small Allocations (65  - 128) = %" SIZETFMT "u", totalSmall128);
	Displayf("Number of Small Allocations (129 - 256) = %" SIZETFMT "u", totalSmall256);
	Displayf("Number of Large Allocations = %u (Max = %u)", m_smAllocationCount[SM_ALLOCATOR_SIZE - 1], m_smAllocationMax[SM_ALLOCATOR_SIZE - 1]);

	Displayf("");	
	Displayf("Used Small Bytes (0   - 256) = %" SIZETFMT "u", usedBytes);
	Displayf("Unused Small Bytes (0   - 256) = %" SIZETFMT "u", freeSmall64 + freeSmall128 + freeSmall256);
	Displayf("Unused Small Bytes (0   -  64) = %" SIZETFMT "u", freeSmall64);
	Displayf("Unused Small Bytes (65  - 128) = %" SIZETFMT "u", freeSmall128);
	Displayf("Unused Small Bytes (129 - 256) = %" SIZETFMT "u", freeSmall256);
}
#endif // !__NO_OUTPUT
#endif // MEMORY_DEBUG
#endif // ENABLE_SMALLOCATOR_CODE


void* sysMemSimpleAllocator::Allocate(size_t size,size_t align,int RAGE_TRACKING_ONLY(heapIndex)) 
{
	SIMPLE_ALLOCATOR_CRITICAL_SECTION;

	// I added this, because we had a number of 0 byte allocations and they were
	// wasting memory (8 bytes per allocation, probably). It is valid in C++
	// for the new operator to return NULL for a 0 size allocation. There
	// were some issues in the past about D3DX not allowing for this behavior,
	// but I believe we are past that. /FF

	// pflanagan: the spu collision task is making 0 byte allocations and expects a valid
	// address back or it gets unhappy and starts aborting collisions. I'm disabling this
	// check on SPU code since it means we're only burning LS and not main memory
#if !__SPU		
	if(!size)
	{
		return NULL;
	}
#endif

    void* p = NULL;

#if ENABLE_SMALLOCATOR_CODE
	if (m_EnableSmallocator && size <= sysSmallocator::MAX_SMALLOCATOR_SIZE && align <= 16 NOTFINAL_ONLY(&& sm_BreakOnAlloc == -1))
    {
		const size_t adjusted = (align <= sysMemAllocator::DEFAULT_ALIGNMENT || size <= 8) ? AlignMem(size, SM_ALLOCATOR_INCREMENT) : AlignMem(size, align);
		p = GetSmallocator(adjusted)->Allocate(*this);

		if (!p)
		{
			Warningf("Not enough memory to allocate %u bytes (%u available)",
				(u32)adjusted,
				(u32)m_MemoryAvailable);
		}
		else
		{
#if SMALL_ALLOCATOR_DEBUG_MIN_ALIGNMENT
			// Max with SM_ALLOCATOR_INCREMENT since adjusted may be zero (ie, when size is zero).
			const uptr minAlign = Max<uptr>(adjusted&-(ptrdiff_t)adjusted, SM_ALLOCATOR_INCREMENT);
			if (minAlign < 16)
			{
				const uptr realStart = (uptr)p;
				p = (void*)((uptr)p + minAlign);    // go to next align boundary so there is space for u8 padding counter
				p = (void*)((uptr)p | minAlign);    // add another align bytes if currently aligned stricter than align
				((u8*)p)[-1] = (u8)((uptr)p-realStart);
			}
#endif
#if RAGE_MEMORY_DEBUG
			DoDebugAllocate(p);
#endif
		}
    }
    else
#endif
    {
        p = this->DoAllocate(size, align);
    }

    if(!p && m_QuitOnFailure)
    {
		if (sysMemOutOfMemoryDisplay)
			sysMemOutOfMemoryDisplay();

#if !__FINAL
		const int heapIdMask = (1<<HEAP_ID_SHIFT)-1;
		const int heapId = m_MemAllocId & ~heapIdMask;
        Quitf("Error allocating %" SIZETFMT "d bytes in heap %d (Total: %" SIZETFMT "d, Used: %" SIZETFMT "d, Free: %" SIZETFMT "d)", size, heapId, GetHeapSize(), GetMemoryUsed(-1), GetMemoryAvailable());
#else
		Quitf(ERR_MEM_EMBEDDEDALLOC_ALLOC, "Error allocating %" SIZETFMT "d bytes (Total: %" SIZETFMT "d, Used: %" SIZETFMT "d, Free: %" SIZETFMT "d)", size, GetHeapSize(), GetMemoryUsed(-1), GetMemoryAvailable());
#endif
    }

#if RAGE_TRACKING
	if (p && IsTallied())
	{
		if (::rage::diagTracker::GetCurrent() && !sysMemVisualize::GetInstance().HasXTL())
			::rage::diagTracker::GetCurrent()->Tally(p, GetSizeWithOverhead(p), heapIndex);
		/* else
		{
			AssertMsg(::rage::g_TrackerDepth, "Untracked memory allocation! Please add a RAGE_TRACK here.");
		} */
	}
#endif

	return p;
}

void* sysMemSimpleAllocator::TryAllocate(size_t size,size_t align,int heapIndex)
{
	SIMPLE_ALLOCATOR_CRITICAL_SECTION;

    const bool quitOnFail = m_QuitOnFailure;
    m_QuitOnFailure = false;
    void* mem = this->Allocate(size, align, heapIndex);
    m_QuitOnFailure = quitOnFail;

    return mem;
}

size_t sysMemSimpleAllocator::GetLargestAvailableBlock() 
{
	SIMPLE_ALLOCATOR_CRITICAL_SECTION;
	for (int fli=c_FreeListCount-1; fli>0; fli--) {
		FreeNode *i = m_FreeLists[fli];
		if (i)
		{
			size_t iLargest = 0;
			while (i) {
				if (i->Size > iLargest)
					iLargest = i->Size;
				i = i->NextFree;
			}
			return iLargest;
		}
	}
	return 0;
}

#if !__FINAL
void sysMemSimpleAllocator::BumpAllocId()
{
	const int heapIdMask = (1<<HEAP_ID_SHIFT)-1;
	// wrap 0xFFF... around to 0
	if ((m_MemAllocId & heapIdMask) == heapIdMask)
		m_MemAllocId &= ~heapIdMask;
	if (++m_MemAllocId == sm_BreakOnAlloc)
		__debugbreak();
}
#endif

#if !__FINAL && !__SPU
size_t g_sysWarnAbove = ~0U;
#endif

sysMemSimpleAllocator::FreeNode* sysMemSimpleAllocator::CheckFreeList(int fli,size_t size,size_t align) {
	FreeNode *i = m_FreeLists[fli], *j = i? i->NextFree : 0;
	while (i) {
		AssertMsg(i != j,"Loop in free list?");
		if (i->Guard != (u32)(size_t)&i->Guard)
		{
			Quitf(ERR_MEM_EMBEDDEDALLOC_GUARD_1,
			"sysMemSimpleAllocator::Allocate - memory node at %p had guard word (%p/%x) trashed!\n\n"
			"This usually means somebody overran an array between this Allocate call and the\n"
			"immediately previous Allocate or Free call.",i,&i->Guard,i->Guard);
		}

		AssertMsg(!i->Used,"Node marked used found in free list");
		size_t alignedStart = ((size_t)i + c_Align + (align-1)) & ~(align-1);
		size_t alignedEnd = (size_t)i + c_Align + i->Size;

		// Exit on first fit rather than best fit so allocations are closer to O(1) (we will still coalesce)
		// The vast majority of the time (standard alignment) this test will instantly succeed.
		if (alignedEnd > alignedStart && (alignedEnd - alignedStart) >= size)
			return i;

		i = i->NextFree;
		if (j)
			j = j->NextFree;
		if (j)
			j = j->NextFree;
	}
	return NULL;
}

void* sysMemSimpleAllocator::DoAllocate(size_t size,size_t align) 
{
	NOTFINAL_ONLY(BumpAllocId());
	// Make sure size is multiple of alignment to simplify checking below.
	if (align < c_Align)
		align = c_Align;

// BEGIN MODS RDR2: We don't want to allocate more memory than necessary.
// This change is needed for GTA and MC since they're close to shipping, but RDR2
// has enough time left to find and debug instances where less memory is allocated
// than necessary. /MAK
	
#if __PS3 && (0)  // changed this so that the PS3 still runs this undesirable code (for now)
// The new check allows us to have smallocator pages aligned on 16KB boundaries
	// that are smaller than that. There is really no point in having
	// this size adjustment anymore at all, but SPU code happily crashes with out it. /MAK

	// pflanagan - re: above comment, I think the cause of spu problems was related to the
	// check for size of zero and this check accidentally fixed it. I'm disabling this (again)
	// for now, but we can enable again if problems show up.


	if (
#if ENABLE_SMALLOCATOR_CODE
		align < sysSmallocator::CHUNK_ALIGNMENT && 
#endif
		size < align)
		size = align;
#endif

	// If running without the smallocator, for example with -breakonalloc=... on the
	// command line, size can be less than c_Align here. In fact, it can be 0.
	// In the 0 case, before we would actually get 0 size memory allocator nodes
	// which didn't pass a sanity check. Now, we make sure that each allocation
	// is at least c_Align large - we wouldn't be able to use the remaining bytes
	// for anything anyway.
	// TODO: We still need to decide on a policy about 0 byte allocations. /FF
	if(size < c_Align)
	{
		size = c_Align;
	}

// END MODS RDR2: We don't want to allocate more memory than necessary. /MAK

	// Round size up immediately to simplify arithmetic
	// Note that very strict alignment will waste a lot of memory.
	// The only reason we don't keep this capped at 128 or so is for the
	// smallocator, but that allocates exactly 64k chunks on 64k boundaries.
	size = (size + c_Align-1) & ~(c_Align-1);

	if (IsHeapLocked())
		Errorf("Attempting to allocate %u bytes on a locked heap",(u32)size);

	//// DoSanityCheck();

#if __DEV
    if(m_sanityCheckHeapOnDoAllocate 
#if !__SPU
		|| PARAM_sanitycheck.Get()
#endif
		)
    {
        DoSanityCheck();
    }
#endif

#if !__FINAL && !__SPU && !__WIN32PC && !__NO_OUTPUT
	if (size >= g_sysWarnAbove && diagChannel::GetOutput()) {
		Warningf("Allocation of %u bytes encountered.",(u32)size);
		sysStack::PrintStackTrace();
	}
#endif

	if (size > m_MemoryAvailable)
	{
#if !__FINAL
        if(m_QuitOnFailure)
        {
	        RAGE_TRACK_REPORT();

#if (__PPU || __XENON) && !__TOOL && !__RESOURCECOMPILER
			sysMemSimpleAllocator::PrintAllMemoryUsage();
#else
			// HACK: Consolidate the bucket values for multi sysMemSimpleAllocator classes
			PrintMemoryUsage();
#endif
		    Warningf("Not enough memory to allocate %" SIZETFMT "u bytes (%" SIZETFMT "u available)", size, m_MemoryAvailable);
        }
#endif // !__FINAL

        return NULL;
	}

	// To better guarantee O(1) behavior, we always want to start on the free list for the next size up.
	// Otherwise we may have to scan the entire list of this size to find one large enough.
	Node *best = NULL;
	for (int fli=GetFreeListIndex(size)+1; !best && fli<c_FreeListCount; fli++)
		best = CheckFreeList(fli,size,align);
	
	// As a final attempt, check the current free list exhaustively.
	if (!best)
		best = CheckFreeList(GetFreeListIndex(size),size,align);

	if (!best)
	{
#if !__FINAL
        if(m_QuitOnFailure)
        {
	        RAGE_TRACK_REPORT();

#if (__PPU || __XENON) && !__TOOL && !__RESOURCECOMPILER
			sysMemSimpleAllocator::PrintAllMemoryUsage();
#else
			// HACK: Consolidate the bucket values for multi sysMemSimpleAllocator classes
			PrintMemoryUsage();
#endif
			Warningf("Not enough memory to allocate %u (align %u) bytes (fragmented) (total available = %u bytes, largest available block = %u bytes)",(u32)size, (u32)align, (u32)m_MemoryAvailable, (u32)GetLargestAvailableBlock());
        }
#endif // !__FINAL
	}
    else
    {
	    // Mark memory block as used, and remove from free list
	    best->Used = 1;
	    best->AllocId = m_MemAllocId;

	    RemoveFromFreeList((FreeNode*)best,GetFreeListIndex(best->Size));

	    // Enforce any necessary alignment.
	    if (((size_t)(best+1) & (align-1))) {
		    size_t bestStart = (size_t) best + c_Align;
			size_t alignedStart = (bestStart + align-1) & ~(align-1);
		    size_t alignedEnd = (size_t)best + c_Align + best->Size;		
		    Assert(alignedEnd > alignedStart);
		    size_t adjust = alignedStart - bestStart;
		    Assert(adjust >= c_Align);
			Assert(adjust < 0x80000000);		// size_t is unsigned, so I'm adding this for good measure.
												// This also works with 64-bit size_t's, assuming we
												// don't allocate things with 2GB padding.
											    // Technically, this assert is not really necessary. /MAK

		    Node *bestPrev = best->GetPrev();

		    // If there is a previous node and our alignment only off by c_Align
		    // bytes, simply grow the previous node (which by definition must be in use)
		    // since we don't want a zero-sized free node around.
		    if (bestPrev && adjust == c_Align) {
			    Assert(bestPrev->Used);
			    bestPrev->Size += c_Align;
				m_MemoryUsedByBucket[bestPrev->Bucket & 15] += c_Align;
		    }
		    // Otherwise insert a new pad node (right on top of this node)
    		
		    else {
			    bestPrev = best;
			    Assert(adjust != c_Align);
			    Assign(bestPrev->Size,adjust - c_Align);
			    bestPrev->AllocId = 0;
			    bestPrev->Used = 0;
			    AddToFreeList((FreeNode*)bestPrev,GetFreeListIndex(bestPrev->Size));
		    }
		    // Compute new location of best-fit node.
		    best = (Node*)(alignedStart - c_Align);
		    Assert(GetNextNode(bestPrev) == best);
		    best->Guard = (u32)(size_t)&best->Guard;
		    Assign(best->Size,(alignedEnd - alignedStart));
		    best->SetPrev(bestPrev);
		    best->AllocId = m_MemAllocId;
		    best->Visited = 0;
		    best->Used = 1;
		    // Point successor back at us
		    Node* bestNext = GetNextNode(best);
		    if (bestNext)
			    bestNext->SetPrev(best);

		    m_MemoryUsed += c_Align;
		    m_MemoryAvailable -= c_Align;
	    }

	    // If best is large enough to subdivide, do that now.
	    if (best->Size > size + c_Align) {
		    FreeNode *split = (FreeNode*)((char*)best + c_Align + size);
		    split->Guard = (u32)(size_t)&split->Guard;
		    Assign(split->Size , best->Size - size - c_Align);
		    split->SetPrev(best);
		    split->AllocId = 0;
		    split->Visited = 0;
		    split->Used = 0;
		    split->Bucket = 0;
		    Node *afterSplit = GetNextNode(split);
		    if (afterSplit)
			    afterSplit->SetPrev(split);
		    Assign(best->Size,size);
		    AddToFreeList(split,GetFreeListIndex(split->Size));
		    m_MemoryUsed += c_Align;
		    m_MemoryAvailable -= c_Align;
	    }

	    // Update memory counters using actual size of node, since that may
	    // not be the actual size requested.
	    m_MemoryUsed += best->Size;
	    m_MemoryAvailable -= best->Size;
	    m_MemoryUsedByBucket[best->Bucket = (sysMemCurrentMemoryBucket&15)] += best->Size;

		if (m_MemoryAvailable < m_LeastMemoryAvailable)
			m_LeastMemoryAvailable = m_MemoryAvailable;

#if !__FINAL
		static int catchBucket = -1;
		static u32 catchSize = 0;
		static u32 ignoreSize = 0;
		static u32 ignoreSize2 = 0;
		if(sysMemAllocator_sm_Master && this == sysMemAllocator::GetMaster().GetAllocator(0) && sysMemCurrentMemoryBucket == catchBucket && best->Size > catchSize && best->Size != ignoreSize && best->Size != ignoreSize2)
			Printf("%d Allocates %d \n", (s32)catchBucket, (s32)best->Size);
#endif // !__FINAL

	    // Fill memory to known state
	    IF_DEBUG_MEMORY_FILL_N(debug_memory_fill(best+1,best->Size),DMF_GAMEHEAP_ALLOC);

	    //// DoSanityCheck();

#if __WIN32 && !__FINAL
	    if (m_StackFile) {
		    fprintf(m_StackFile,"new,0x%x,%u,%u,%u,",(u32)(size_t)(best+1),m_MemAllocId,(u32)size,sysMemCurrentMemoryBucket);
		    if (m_LogStackTracebacks) {
			    s_Stream = m_StackFile;
			    s_Skipper = 3;		// Skip this many entries from the top of the stack
			    sysStack::PrintStackTrace(s_DisplayLine);
		    }
		    fprintf(m_StackFile,"\r\n");
	    }
#endif

	    return best+1;
    }

    return NULL;
}


void sysMemSimpleAllocator::Resize(const void *ptr,size_t newSmallerSize) 
{
	SIMPLE_ALLOCATOR_CRITICAL_SECTION;

#if ENABLE_SMALLOCATOR_CODE
	Assert(!IsSmallocatorChunk(ptr));
#endif

	if (IsHeapLocked())
		Errorf("Attempting to Resize memory from a locked heap (%p)",ptr);

	if (!IsValidPointer(ptr))
	{
		Quitf(ERR_MEM_EMBEDDEDALLOC_RESIZE_1,"Attempting to Resize memory not allocated from this heap");
	}

	Node *current = ((Node*)ptr) - 1;

	// Bail if the memory isn't in use
	if (!current->Used)
	{
		Quitf(ERR_MEM_EMBEDDEDALLOC_RESIZE_2,"sysMemSimpleAllocator::Resize - memory at %p already marked free",ptr);
	}

	// Bail if the memory was trashed
	if (current->Guard != (u32)(size_t)&current->Guard)
    {
		Quitf(ERR_MEM_EMBEDDEDALLOC_GUARD_2,
		"sysMemSimpleAllocator::Resize - memory at %p had guard word (%p/%x) trashed!\n\n"
		"This usually means somebody overran an array between this Resize call\n"
		"and the immediately previous Allocate or Free call.",ptr,&current->Guard,current->Guard);
    }

	// Make sure resulting new size is nonzero
	newSmallerSize = (newSmallerSize + 15) & ~15;
	if (!newSmallerSize)
		newSmallerSize = 16;

	// If size didn't change, bail immediately (don't bother logging memory)
	if (current->Size == newSmallerSize)
		return;

	if (current->Size < newSmallerSize)
	{
		Quitf(ERR_MEM_EMBEDDEDALLOC_RESIZE_3,"Attempting to Resize block to larger size");
	}

	// Now this is guaranteed to be positive
	size_t shrinkage = current->Size - newSmallerSize;
	// Displayf("*********shrinkage = %u",(unsigned)shrinkage);

	// If the amount we're trying to reclaim is smaller than the memory node necessary to mark it,
	// silently return.
	if (shrinkage <= c_Align)
		return;

#if RAGE_TRACKING
	if (ptr && IsTallied() && ::rage::diagTracker::GetCurrent() && !sysMemVisualize::GetInstance().HasXTL())
		::rage::diagTracker::GetCurrent()->UnTally((void*)ptr, current->Size + sizeof(Node));
#endif

	// Adjust memory counters
	m_MemoryAvailable += shrinkage - c_Align;
	m_MemoryUsed -= shrinkage - c_Align;
	m_MemoryUsedByBucket[current->Bucket] -= shrinkage;

	// Adjust size of this memory block
	Assign(current->Size,newSmallerSize);
	FreeNode *newNode = (FreeNode*) GetNextNode(current);
	newNode->Guard = (u32) (size_t) &newNode->Guard;
	Assign(newNode->Size , shrinkage - c_Align);
	newNode->SetPrev(current);
	newNode->Used = 0;
	newNode->Visited = 0;

	Node *next = GetNextNode(newNode);

    //Check if we overran the buffer being freed.
    if(next && next->Guard != (u32)(size_t)&next->Guard)
    {
        Quitf(ERR_MEM_EMBEDDEDALLOC_GUARD_3,"sysMemSimpleAllocator::Free - memory at %p trashed guard word (%p/%x) of next block %p!!\n\n"
            "This usually means somebody overran an array between this Free call\n"
            "and the immediately previous Allocate or Free call.",ptr,&next->Guard,next->Guard,(next+1));
    }

	// Check to see if we need to coalesce with next allocation
    if (next)
		next->SetPrev(newNode);
	if (next && !next->Used) {
		// Consume next node into ourselves and update billing
		newNode->Size += c_Align + next->Size;
		m_MemoryAvailable += c_Align;
		m_MemoryUsed -= c_Align;
		RemoveFromFreeList((FreeNode*)next,GetFreeListIndex(next->Size));

		// Maintain node connectivity
		if (GetNextNode(next))
			GetNextNode(next)->SetPrev(newNode);
	}

	AddToFreeList((FreeNode*)newNode,GetFreeListIndex(newNode->Size));

#if RAGE_TRACKING
	if (IsTallied() && ::rage::diagTracker::GetCurrent() && !sysMemVisualize::GetInstance().HasXTL())
		::rage::diagTracker::GetCurrent()->Tally(const_cast<void*>(ptr), newSmallerSize + sizeof(Node), 0);
#endif

#if RAGE_ENABLE_RAGE_NEW
	// Un-log the original pointer (at its original size)
	RAGE_LOG_DELETE(ptr);
#endif

	// ...and re-log it at its new size.
	RAGE_LOG_NEW(ptr,newSmallerSize,__FILE__,__LINE__);
}

void sysMemSimpleAllocator::Free(const void *ptr) 
{
    if (!IsValidPointer(ptr))
	{
		// HERE THERE BE DRAGONS.
		// HACK: ejanderson - This is gonna get weird....
		// Our memory management system is NOT very forgiving of allocate/free across heaps
		// So... if we are the current heap (and not the master), let's try to free the memory anyway
		if (this == &sysMemAllocator::GetCurrent() && this != &sysMemAllocator::GetMaster())
		{
			Warningf("Attempting to free invalid pointer: %p. Attempting to free via multiallocator....", ptr);
			sysMemAllocator::GetMaster().Free(ptr);
			return;
		}

		if (sysMemAllocator_sm_Master != sysMemAllocator_sm_Current)
		{
			Quitf(ERR_MEM_EMBEDDEDALLOC_FREE_3,"Freeing an invalid pointer (%p) with a custom allocator - you're most likely using a custom allocator (like USE_DEBUG_MEMORY) and are trying to delete memory that was not allocated using this allocator. Make sure USE_DEBUG_MEMORY blocks are confined to a tight scope.", ptr);
		}
    }

	SIMPLE_ALLOCATOR_CRITICAL_SECTION;

#if RAGE_TRACKING
	if (ptr && IsTallied() && ::rage::diagTracker::GetCurrent() && !sysMemVisualize::GetInstance().HasXTL())
		::rage::diagTracker::GetCurrent()->UnTally((void*)ptr, GetSizeWithOverhead(ptr));
#endif

#if !__SPU && RAGE_ENABLE_RAGE_NEW
	RAGE_LOG_DELETE(ptr);
#endif

#if ENABLE_SMALLOCATOR_CODE
	sysSmallocator::Chunk *sa = IsSmallocatorChunk(ptr);
	if (sa)
	{
#if RAGE_MEMORY_DEBUG
		DoDebugFree(ptr);
#endif
#if SMALL_ALLOCATOR_DEBUG_MIN_ALIGNMENT
		// If the alignment of the pointer is less than 16-bytes, then must have
		// gone through the debug path that limits alignment.
		if (((uptr)ptr & 15) != 0)
		{
			const u8 alignPadding = ((const u8*)ptr)[-1];
			ptr = (const u8*)ptr - alignPadding;
		}
#endif
		sa->Free(ptr,*this);
	}		
	else
#endif
		DoFree(ptr);
}

void sysMemSimpleAllocator::DoFree(const void *ptr) 
{
	if (IsHeapLocked())
		Errorf("Attempting to free memory from a locked heap (%p)",ptr);

	if (!IsValidPointer(ptr))
	{
		Quitf(ERR_MEM_EMBEDDEDALLOC_FREE_1,"Attempting to free memory not allocated from this heap");
	}

#if __DEV && !__SPU
	if (PARAM_sanitycheckfree.Get())
		DoSanityCheck();
#endif

#if !__FINAL && !__SPU
	if (PARAM_verifyfreelist.Get())
		VerifyFreelist();
#endif

	Node *current = ((Node*)ptr) - 1;

	// Bail if the memory isn't in use
	if (!current->Used)
	{
		Quitf(ERR_MEM_EMBEDDEDALLOC_FREE_2,"sysMemSimpleAllocator::Free - memory at %p already marked free",ptr);
	}

#if __WIN32 && !__FINAL
	if (m_StackFile) {
		fprintf(m_StackFile,"delete,0x%x,%u,%u,%u,",(u32)(size_t)ptr,current->AllocId,(u32)current->Size,current->Bucket);
		if (m_LogStackTracebacks) {
			s_Stream = m_StackFile;
			s_Skipper = 3;		// Skip this many entries from the top of the stack
			sysStack::PrintStackTrace(s_DisplayLine);
		}
		fprintf(m_StackFile,"\r\n");
	}
#endif

	current->Used = 0;

	// Bail if the memory was trashed
	if (current->Guard != (u32)(size_t)&current->Guard)
    {
		Quitf(ERR_MEM_EMBEDDEDALLOC_GUARD_4,
			"sysMemSimpleAllocator::Free - memory at %p had guard word (%p/%x) trashed!\n\n"
			"This usually means somebody overran an array between this Free call\n"
			"and the immediately previous Allocate or Free call.",ptr,&current->Guard,current->Guard);
    }

#if !__FINAL
	static u32 catchFreeBucket = 0xff;
	static u32 catchFreeSize = 0;
	static u32 ignoreFreeSize = 0;
	static u32 ignoreFreeSize2 = 0;
	size_t bucket = current->Bucket;
	if(sysMemAllocator_sm_Master && this == sysMemAllocator::GetMaster().GetAllocator(0) && bucket == catchFreeBucket && current->Size > catchFreeSize && current->Size != ignoreFreeSize && current->Size != ignoreFreeSize2)
		Printf("%d Frees %d \n", (s32)catchFreeBucket, (s32)current->Size);
#endif // !__FINAL

	// Fill as much of freed memory as possible to known state.
	IF_DEBUG_MEMORY_FILL_N(memset(current + 1,0xDD,current->Size),DMF_GAMEHEAP_FREE);

	m_MemoryAvailable += current->Size;
	m_MemoryUsed -= current->Size;
	m_MemoryUsedByBucket[current->Bucket] -= current->Size;

	Node *next = GetNextNode(current);

    //Check if we overran the buffer being freed.
    if(next && next->Guard != (u32)(size_t)&next->Guard)
    {
        Quitf(ERR_MEM_EMBEDDEDALLOC_GUARD_5,"sysMemSimpleAllocator::Free - memory at %p trashed guard word (%p/%x) of next block %p!!\n\n"
            "This usually means somebody overran an array between this Free call\n"
            "and the immediately previous Allocate or Free call.",ptr,&next->Guard,next->Guard,(next+1));
    }

	// Check to see if we need to coalesce with next allocation
	if (next && !next->Used) {
		// Consume next node into ourselves and update billing
		current->Size += c_Align + next->Size;
		m_MemoryAvailable += c_Align;
		m_MemoryUsed -= c_Align;
		RemoveFromFreeList((FreeNode*)next,GetFreeListIndex(next->Size));

		// Maintain node connectivity
		if (GetNextNode(next))
			GetNextNode(next)->SetPrev(current);
	}

	// Check to see if we need to coalesce with previous allocation
	Node *prev = current->GetPrev();
	if (prev && !prev->Used) {
		// Consume current node into previous node and update billing
		RemoveFromFreeList((FreeNode*)prev,GetFreeListIndex(prev->Size));
		prev->Size += c_Align + current->Size;
		m_MemoryAvailable += c_Align;
		m_MemoryUsed -= c_Align;

		// Maintain node connectivity
		if (GetNextNode(current))
			GetNextNode(current)->SetPrev(prev);

		current = prev;
	}
	
	AddToFreeList((FreeNode*)current,GetFreeListIndex(current->Size));

	//// DoSanityCheck();
}

#if RESOURCE_HEADER
u32 sysMemSimpleAllocator::GetUserData(const void *ptr) const 
{
	if (ptr < m_Heap || ptr >= (char*)m_Heap + m_HeapSize)
		return (u32)INVALID_USER_DATA;

	u32 data = sysMemManager::GetInstance().GetUserData(ptr);
	if (data == INVALID_USER_DATA)
		Errorf("sysMemSimpleAllocator::GetUserData called on a free block or bogus pointer?");

	return data;
}

void sysMemSimpleAllocator::SetUserData(const void* ptr, u32 data)
{
	if (ptr < m_Heap || ptr >= (char*)m_Heap + m_HeapSize)
		return;

	sysMemManager::GetInstance().SetUserData(ptr, data);
}

const void* sysMemSimpleAllocator::GetCanonicalBlockPtr(const void* ptr) const
{
	if (ptr < m_Heap || ptr >= (char*)m_Heap + m_HeapSize)
		return NULL;

	return ptr;
}
#endif

size_t sysMemSimpleAllocator::GetSize(const void *ptr) const 
{
	if (ptr < m_Heap || ptr >= (char*)m_Heap + m_HeapSize)
		return 0;

#if ENABLE_SMALLOCATOR_CODE
	sysSmallocator::Chunk *sac = IsSmallocatorChunk(ptr);
	if (sac) {
		sysSmallocator *sa = sac->m_Owner;
		size_t idx = sa-m_smallAllocator;
		size_t size = (idx+1) * SM_ALLOCATOR_INCREMENT;
#if !SMALL_ALLOCATOR_DEBUG_MIN_ALIGNMENT
		Assert(sa->GetEntrySize() == size);
#endif
		return size;
	}
#endif

	Node *current = ((Node*)ptr) - 1;
	if (current->Used && current->Guard == (u32)(size_t)&current->Guard)
		return current->Size;
	else {
		Errorf("sysMemSimpleAllocator::GetSize called on a free block or bogus pointer?");
		return 0;
	}
}

size_t sysMemSimpleAllocator::GetSizeWithOverhead(const void *ptr) const 
{
	if (ptr < m_Heap || ptr >= (char*)m_Heap + m_HeapSize)
		return 0;

#if ENABLE_SMALLOCATOR_CODE
	// Check the smallocators since they don't have normal headers.
	// Note that the overhead of a smallocator entry is so small we just ignore it for simplicity.
	sysSmallocator::Chunk *sac = IsSmallocatorChunk(ptr);
	if (sac) {
		sysSmallocator *sa = sac->m_Owner;
		size_t idx = sa-m_smallAllocator;
		size_t size = (idx+1) * SM_ALLOCATOR_INCREMENT;
#if !SMALL_ALLOCATOR_DEBUG_MIN_ALIGNMENT
		Assert(sa->GetEntrySize() == size);
#endif
		return size;
	}
#endif

	Node *current = ((Node*)ptr) - 1;
	if (current->Used && current->Guard == (u32)(size_t)&current->Guard)
		return current->Size + sizeof(Node);
	else {
		Errorf("sysMemSimpleAllocator::GetSizeWithOverhead called on a free block or bogus pointer?");
		return 0;
	}
}

bool sysMemSimpleAllocator::IsValidPointer(const void *ptr) const 
{
	return (ptr >= (char*)m_Heap+sizeof(Node) && ptr < (char*)m_Heap + m_HeapSize);
}

#if __BANK
bool sysMemSimpleAllocator::IsValidUsedPointer(const void *ptr) const
{
	if (IsValidPointer(ptr))
	{
		Node *current = ((Node*)ptr) - 1;
		return current->Used && (current->Guard == (u32)(size_t)&current->Guard);
	}

	return false;
}
#endif

void sysMemSimpleAllocator::SanityCheck() 
{
	SIMPLE_ALLOCATOR_CRITICAL_SECTION;
	DoSanityCheck();
}

#if !__NO_OUTPUT
static void DumpMemoryNear(void *p)
{
	diagHexDump((char*)p - 64,128+16,true);
}
#endif

#if !__FINAL && !__SPU
void sysMemSimpleAllocator::VerifyFreelist() const
{
	for (int i = 0; i < c_FreeListCount; i++)
	{
		FreeNode* f = m_FreeLists[i];
		FreeNode* pf = NULL;
		while (f)
		{
			if (f->Guard != (u32)(size_t)&f->Guard)
				Quitf("Trashed guard word found (free list is bad?)");

			if (f->Used)
				Quitf("Used item on free list");

			if (f->PrevFree != pf)
				Quitf("Prev free isn't what we expected");

			pf = f;
			f = f->NextFree;
		}
	}
}
#endif

void sysMemSimpleAllocator::DoSanityCheck() 
{
#if !__NO_OUTPUT
	// FIRST PASS: Check that all the sizes and guard words are correct.
	Node *i = (Node*) m_Heap;
	Node *p = NULL;
	Node *stop = (Node*) ((char*)m_Heap + m_HeapSize);
	size_t totalUsed = 0;
	size_t totalUsedByBucket[16] = {  0, 0, 0, 0
									, 0, 0, 0, 0
									, 0, 0, 0, 0
									, 0, 0, 0, 0 };
	size_t totalFree = 0;
	size_t overhead = 0;
	while (i < stop) {
		if (i->Size == 0) {
			DumpMemoryNear(i);
			Quitf("Node %p with size of zero found, this should never happen",i);
		}
		if (i->Guard != (u32)(size_t)&i->Guard) {
			DumpMemoryNear(i);
			Quitf("Trashed guard word %p found (maybe previous size was bad?)",i);
		}
		if (i->GetPrev() != p)
			Quitf("Prev node invalid %p->Prev=%p, expected %p",i,i->GetPrev(),p);
		overhead += c_Align;
		if (i->Used)
		{
			totalUsed += i->Size;
			totalUsedByBucket[i->Bucket & 15] += i->Size;
		}
		else
			totalFree += i->Size;
		p = i;
		i = (Node*) ((char*)i + i->Size + c_Align);
	}
	if (i != stop)
		Quitf("Last node had bad size, went off end of heap");

	if (totalUsed + totalFree + overhead != m_HeapSize)
		Quitf("Counted up %u used, %u free, %u overhead, total was %u, expected %u",
			(u32)totalUsed,(u32)totalFree,(u32)overhead,u32(totalUsed + totalFree + overhead),(u32)m_HeapSize);
	if (m_MemoryUsed + m_MemoryAvailable != m_HeapSize)
		Quitf("Accumulated free counters don't match up - %u + %u != %u",(u32)m_MemoryUsed,(u32)m_MemoryAvailable,(u32)m_HeapSize);

	if (totalFree != m_MemoryAvailable)
		Quitf("Accumulated free count (%u) doesn't match actual free (%u)",(u32)totalFree,(u32)m_MemoryAvailable);

	for (int bucket = 0; bucket<16; bucket++)
		if (totalUsedByBucket[bucket] != m_MemoryUsedByBucket[bucket])
			Quitf("Accumulated free count (%u) doesn't match actual free (%u) for bucket %i", (u32)totalUsedByBucket[bucket], (u32)m_MemoryUsedByBucket[bucket], bucket);

	// SECOND PASS: Verify free list is sane and loop-free
	for (int fli=0; fli<c_FreeListCount; fli++) {
		FreeNode *f = m_FreeLists[fli];
		FreeNode *pf = NULL;
		while (f) {
			if (f->Guard != (u32)(size_t)&f->Guard)
				Quitf("Trashed guard word found (free list is bad?)");
			if (f->Visited)
				Quitf("Loop in free list");
			if (f->Used)
				Quitf("Used item on free list");
			if (f->PrevFree != pf)
				Quitf("Prev free isn't what we expected");
			f->Visited = 1;
			pf = f;
			f = f->NextFree;
		}
	}

	// THIRD PASS: Make sure all non-Used nodes have not been visited.
	i = (Node*) m_Heap;
	while (i != stop) {
		if (i->Used == i->Visited)
			Quitf("Used node not visited or vice versa");
		i->Visited = 0;
		i = (Node*) ((char*)i + i->Size + c_Align);
	}

#if ENABLE_SMALLOCATOR_CODE
	u16 size = SM_ALLOCATOR_INCREMENT;
	for (int i = 0; i < SM_ALLOCATOR_SIZE; ++i, size += SM_ALLOCATOR_INCREMENT)
		m_smallAllocator[i].SanityCheck();
#endif
#endif		// __NO_OUTPUT
}

size_t sysMemSimpleAllocator::GetMemoryAvailable() 
{
	return m_MemoryAvailable;
}

size_t sysMemSimpleAllocator::GetLowWaterMark(bool reset) 
{
	size_t result = m_LeastMemoryAvailable;
	if (reset)
		m_LeastMemoryAvailable = m_MemoryAvailable;
	return result;
}

size_t sysMemSimpleAllocator::GetMemoryUsed(int bucket) 
{
	if (bucket >= 0 && bucket < 16)
		return m_MemoryUsedByBucket[bucket];
	else
		return m_MemoryUsed;
}

void sysMemSimpleAllocator::UpdateMemorySnapshot() 
{
	for (int i=0; i<16; i++)
		m_MemoryUsedByBucketSnapshot[i] = m_MemoryUsedByBucket[i];
}

void sysMemSimpleAllocator::BeginLayer() 
{
	Assert(m_LayerCount < c_MaxLayers);
	// Track the last memory allocation NOT in this layer
	m_AllocIdByLayer[m_LayerCount++] = m_MemAllocId;
}

void sysMemSimpleAllocator::BeginMemoryLog(const char *filename,bool logStackTracebacks) 
{
	Assert(!m_StackFile);
	m_StackFile = fiStream::Create(filename);
	if (m_StackFile)
		fprintf(m_StackFile,"op,ptr,allocId,size,bucket,stack...\r\n");
	m_LogStackTracebacks = logStackTracebacks;
}

void sysMemSimpleAllocator::EndMemoryLog() 
{
	if (m_StackFile) {
		m_StackFile->Close();
		m_StackFile = NULL;
	}
}

int sysMemSimpleAllocator::EndLayer(const char *OUTPUT_ONLY(layerName),const char *leakFile) 
{
	Assert(m_LayerCount);
	u32 lastAllocNotInThisLayer = m_AllocIdByLayer[--m_LayerCount];

	// Run sanity check first to make sure we don't crash here.
	SanityCheck();

	fiStream *leakStream = leakFile? fiStream::Create(leakFile) : NULL;

	bool first = true;
	Node *i = (Node*) m_Heap;
	Node *stop = (Node*) ((char*)m_Heap + m_HeapSize);
	size_t count = 0;
	size_t sum = 0;

#if ENABLE_SMALLOCATOR_CODE
	bool printSmallocatorUsage = false;
#endif

	while (i != stop) {
		if (i->Used && i->AllocId > lastAllocNotInThisLayer) {
			if (first) {
				if ( sm_leaksAsErrors )
				{
					if (!sm_hideAllocOutput)
						Errorf("Memory leaks in layer '%s':",layerName);
				}
				else
				{
					if (!sm_hideAllocOutput)
						Warningf("Memory leaks in layer '%s':",layerName);
				}
				first = false;
			}

#if ENABLE_SMALLOCATOR_CODE
			if (IsSmallocatorChunk(i+1))
				printSmallocatorUsage = true;
#endif
			static const unsigned int maxToSpew = 25;
			if (count == maxToSpew) 
			{
				if ( sm_leaksAsErrors )
				{
					if (!sm_hideAllocOutput)
						Errorf("Way too leaky, going to stop spewing now");
				}
				else
				{
					if (!sm_hideAllocOutput)
						Warningf("Way too leaky, going to stop spewing now");
				}
				sum += i->Size;	
				++count;
			}
			else if (count < maxToSpew) 
			{				
				if ( sm_leaksAsErrors )
				{
					if (!sm_hideAllocOutput)
						Errorf("Leak allocid=%u, bucket=%u, size=%u bytes",(u32)i->AllocId,(u32)i->Bucket,(u32)i->Size);
				}
				else
				{
					if (!sm_hideAllocOutput)
						Warningf("Leak allocid=%u, bucket=%u, size=%u bytes",(u32)i->AllocId,(u32)i->Bucket,(u32)i->Size);
				}
				sum += i->Size;			
				if (leakStream)
					fprintf(leakStream,"%u\r\n",i->AllocId);

				++count;
			}
		}
		i = (Node*) ((char*)i + i->Size + c_Align);
	}

#if ENABLE_SMALLOCATOR_CODE
	if (printSmallocatorUsage)
	{
		size_t items = 0;
		u16 size = SM_ALLOCATOR_INCREMENT;
		for (int i = 0; i < SM_ALLOCATOR_SIZE; ++i, size += SM_ALLOCATOR_INCREMENT)
			items += m_smallAllocator[i].PrintLeakData(sm_leaksAsErrors);
	}
#endif

	if (count)
	{
		if ( sm_leaksAsErrors )
		{
			if (!sm_hideAllocOutput)
				Errorf("%d leak(s) (%d bytes) total",(int)count,(int)sum);
		}
		else
		{
			if (!sm_hideAllocOutput)
				Warningf("%d leak(s) (%d bytes) total",(int)count,(int)sum);
		}
#if !__SPU && !__NO_OUTPUT
		if (!sm_hideAllocOutput)
			printf("<Leaks>%" SIZETFMT "d</Leaks>",count);
#endif
	}
	if (leakStream)
		leakStream->Close();
	return (int) count;
}

void sysMemSimpleAllocator::InitHeap( void* heap, int heapSize, bool 
#if ENABLE_SMALLOCATOR_CODE
								allowSmallAllocator 
#endif
								)
{
    m_OrigHeap = heap;
#if ENABLE_SMALLOCATOR_CODE
	m_EnableSmallocator = (allowSmallAllocator && heapSize >= 1024 * 1024);

	// Round start of heap down to 64k boundary
	m_PageBase = (char*)((size_t)m_OrigHeap & ~(sysSmallocator::CHUNK_ALIGNMENT-1));

	if (m_EnableSmallocator)
	{
		InitSmallocator();
	}		
#endif
	Assertf(heapSize <= MAX_HEAP_SIZE,"Requested heap size of %dk is over max supported %dk",heapSize>>10,MAX_HEAP_SIZE>>10);

    // Align the heap
	// The starting address is aligned to 2x the normal alignment, and then biased by the
	// actual alignment, in order to avoid the really annoying special case of attempting
	// to allocate the first block of memory using c_Align*2 alignment -- without this special
	// logic, we would end up with a node of size zero, which is not valid.
    m_Heap = ( void* ) ( ( ( ( size_t ) heap + 2*c_Align - 1 ) & ~( 2*c_Align - 1 ) ) + c_Align );
    Assert( m_Heap >= m_OrigHeap );

    //Adjust the heap size due to alignment
	m_HeapSize = heapSize = heapSize - int( ( size_t ) m_Heap - ( size_t ) m_OrigHeap );

    AssertMsg( (size_t)heapSize > c_Align, "Cannot align heap given the small heap size provided" );

	memset(&m_FreeLists[0],0,sizeof(m_FreeLists));
	FreeNode *fl = m_FreeLists[GetFreeListIndex(heapSize-c_Align)] = (FreeNode*) m_Heap;
	
	// These should always add up to m_HeapSize;
	m_MemoryUsed = c_Align;
	m_MemoryAvailable = heapSize - c_Align;
	m_LeastMemoryAvailable = m_MemoryAvailable;

	// These do not account for allocation overhead
	for (int i=0; i<16; i++)
	{
		m_MemoryUsedByBucket[i] = 0;
		m_MemoryUsedByBucketSnapshot[i] = 0;
	}

	// Initialize the free list with the entire heap.
	fl->Guard = (u32)(size_t)&fl->Guard;
	fl->Size = heapSize - c_Align;
	fl->SetPrev(0);
	fl->AllocId = 0;
	fl->Visited = 0;
	fl->Used = 0;
	fl->Bucket = 0;
	fl->PrevFree = fl->NextFree = NULL;

	m_LayerCount = 0;
	m_StackFile = NULL;
}

void* sysMemSimpleAllocator::GetHeapBase() const
{
	return m_Heap;
}

size_t sysMemSimpleAllocator::GetHeapSize() const
{
	return m_HeapSize;
}

#if !__FINAL
void sysMemSimpleAllocator::PrintMemoryUsage() const
{
	Displayf("Memory Available: %d, Memory Used: %d", (s32)m_MemoryAvailable, (s32)m_MemoryUsed);
	
	PrintBuckets();
	PrintFreeList();
}

void sysMemSimpleAllocator::PrintBuckets() const
{
	Displayf("\nMemory Bucket Values:");
	for(int i=0; i<16; i++)
	{
		s32 memoryBucketValue = (s32)m_MemoryUsedByBucket[i];
		s32 memoryBucketValueSnapshot = (s32)m_MemoryUsedByBucketSnapshot[i];
		s32 difference = memoryBucketValue - memoryBucketValueSnapshot;
		const char * bucketName = sm_MemoryBucketNames[i];
		if(bucketName)
		{
			Displayf("%s: %d (%d KB), delta %+d (%+d KB)", bucketName, memoryBucketValue, memoryBucketValue / 1024, difference, difference/1024);
		}
		else
		{
			Displayf("%d: %d (%d KB), delta %+d (%+d KB)", i, memoryBucketValue, memoryBucketValue / 1024, difference, difference/1024);
		}
	}
}

void sysMemSimpleAllocator::PrintFreeList() const
{
#if !__SPU
	const int align = 16;

	Displayf("\nFree List Contents:");
	for (int fli=0; fli<c_FreeListCount; fli++)
	{
		int elementCount = 0;
		size_t sizeCount = 0, sizeCountAligned = 0, largestFree = 0, largestFreeAligned = 0;
		FreeNode *i = m_FreeLists[fli], *j = i? i->NextFree : 0;
#if __ASSERT
		size_t lastSize = 0;
		FreeNode *prev = NULL, *expectedNext = NULL;
#endif

		while (i) 
		{
			AssertMsg(i != j,"Loop in free list?");
			if (i->Guard != (u32)(size_t)&i->Guard)
				Quitf(
				"sysMemSimpleAllocator::Allocate - memory node at %p had guard word (%p/%x) trashed!\n\n"
				"This usually means somebody overran an array between this Allocate call and the\n"
				"immediately previous Allocate or Free call.",i,&i->Guard,i->Guard);
			AssertMsg(!i->Used,"Node marked used found in free list");

			size_t alignedStart = ((size_t)i + c_Align + (align-1)) & ~(align-1);
			size_t alignedEnd = (size_t)i + c_Align + i->Size;
			largestFree = Max(largestFree, (size_t)i->Size);

			sizeCount += i->Size;

			if (alignedEnd > alignedStart)
			{
				size_t alignedSize = alignedEnd - alignedStart;

				elementCount++;
				sizeCountAligned += alignedSize;
				largestFreeAligned = Max(largestFreeAligned, alignedSize);

				Assertf(expectedNext != i, "Block %p and %p (%d bytes / %d aligned) haven't been merged", prev, i, (int) prev->Size, (int) lastSize);

#if __ASSERT
				lastSize = alignedSize;
				prev = i;
				expectedNext = (FreeNode *) (((size_t) i) + lastSize);
#endif
			}

			i = i->NextFree;
			if (j)
				j = j->NextFree;
			if (j)
				j = j->NextFree;
		}

		Displayf("List %d: %d elements, %d bytes (aligned: %d), largest %d (aligned: %d)", fli, elementCount, (int) sizeCount, (int) sizeCountAligned, (int) largestFree, (int) largestFreeAligned);
	}
#endif // !__SPU
}

#if (__PPU || __XENON) && !__TOOL && !__RESOURCECOMPILER
#if __PPU
extern sysMemAllocator* g_pResidualAllocator;
#elif __XENON
extern sysMemAllocator* g_pXenonPoolAllocator;
#endif

// HACK: Consolidate the bucket values for multi sysMemSimpleAllocator classes
void sysMemSimpleAllocator::PrintAllMemoryUsage()
{	
	sysMemSimpleAllocator* pGameVirtual = static_cast<sysMemSimpleAllocator*>(sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_GAME_VIRTUAL));
	Assert(pGameVirtual);

#if __PPU
	sysMemSimpleAllocator* pFlexAllocator = static_cast<sysMemSimpleAllocator*>(sysMemManager::GetInstance().GetFlexAllocator()->GetPrimaryAllocator());
	Assert(pFlexAllocator);

	sysMemSimpleAllocator* pResidualAllocator = static_cast<sysMemSimpleAllocator*>(g_pResidualAllocator);
	Assert(pResidualAllocator);

	const size_t items = 3;
	sysMemSimpleAllocator* pAllocator[items] = {pGameVirtual, pFlexAllocator, pResidualAllocator};
	const char* pszTitle[items] = {"Game Virtual", "Flex Allocator", "Residual Allocator"};
#elif __XENON
	sysMemSimpleAllocator* pPoolAllocator = static_cast<sysMemSimpleAllocator*>(g_pXenonPoolAllocator);
	Assert(pPoolAllocator);

	const size_t items = 2;
	sysMemSimpleAllocator* pAllocator[items] = {pGameVirtual, pPoolAllocator};
	const char* pszTitle[items] = {"Game Virtual", "Pool Allocator"};
#else
	const size_t items = 1;
	sysMemSimpleAllocator* pAllocator[items] = {pGameVirtual};
	const char* pszTitle[items] = {"Game Virtual"};
	
#endif

	// Tally
	s32 value[16] = {0};
	s32 delta[16] = {0};
	size_t available = 0;
	size_t used = 0;

	for (int i = 0; i < items; ++i)
	{
		// Totals
		available += pAllocator[i]->GetMemoryAvailable();
		used += pAllocator[i]->GetMemoryUsed(-1);

		for (int j = 0; j < 16; ++j)
		{
			value[j] += (s32) pAllocator[i]->m_MemoryUsedByBucket[j];
			delta[j] += (value[j] - static_cast<s32>(pAllocator[i]->m_MemoryUsedByBucketSnapshot[j]));
		}
	}

	// Consolidated Output
	Displayf("[Multiply Allocators]");
	Displayf("Memory Available: %" SIZETFMT "u, Memory Used: %" SIZETFMT "u", available, used);
	Displayf("\nMerged Bucket Values:");

	for(int i = 0; i < 16; ++i)
	{
		const char* bucketName = pGameVirtual->sm_MemoryBucketNames[i];

		if(bucketName)
		{
			Displayf("%s: %d (%d KB), delta %+d (%+d KB)", bucketName, value[i], (value[i] >> 10), delta[i], (delta[i] >> 10));
		}
		else
		{
			Displayf("%d: %d (%d KB), delta %+d (%+d KB)", i, value[i], (value[i] >> 10), delta[i], (delta[i] >> 10));
		}
	}

	// Individual Output
	for (int i = 0; i < items; ++i)
	{
		Displayf("\n%s", pszTitle[i]);
		pAllocator[i]->PrintMemoryUsage();
	}
}
#endif // (__PPU || __XENON) && !__TOOL && !__RESOURCECOMPILER
#endif // !__FINAL

size_t sysMemSimpleAllocator::GetMemorySnapshot(int bucket)
{
	return m_MemoryUsedByBucketSnapshot[bucket];
}

#if __BANK
#if __64BIT
void sysMemSimpleAllocator::AddWidgets(bkGroup&) {}
#else
void sysMemSimpleAllocator::AddWidgets(bkGroup& grp)
{
	grp.AddSlider("Total Size", &m_HeapSize, 0, UINT_MAX, 0);
	grp.AddSlider("Total Used", &m_MemoryUsed, 0, UINT_MAX, 0);
	grp.AddSlider("Total Available", &m_MemoryAvailable, 0, UINT_MAX, 0);
	grp.AddSlider("Least Available", &m_LeastMemoryAvailable, 0, UINT_MAX, 0);
	bkGroup* buckets = grp.AddGroup("By Bucket", false);
	for(int i = 0; i < 16; i++)
	{
		if (sm_MemoryBucketNames[i])
		{
			buckets->AddSlider(sm_MemoryBucketNames[i], &m_MemoryUsedByBucket[i], 0, UINT_MAX, 0);
		}
	}
}
#endif // __64BIT
#endif // __BANK

}	// namespace rage
