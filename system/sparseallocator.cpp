// 
// system/sparseallocator.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "sparseallocator.h"

#if RSG_PC || RSG_DURANGO
#include "diag/errorcodes.h"
#include "system/xtl.h"
#include "system/param.h"

#include <algorithm>

PARAM(sparseleft,"[system] Left-align sparse allocations to catch underruns instead of overruns");
PARAM(sparseboth,"[system] Alternate between left- and right- aligned memory allocations");

namespace rage {

	sysMemSparseAllocator::sysMemSparseAllocator(size_t fakeHeapSize,const int /*heapIndex*/)
	{
		m_PageCount = 0;
		m_NextAlloc = 0;
		m_NextEnd = 0;
		m_MemoryUsed = 0;
		m_FakeHeapSize = fakeHeapSize;
	}

	sysMemSparseAllocator::~sysMemSparseAllocator()
	{
	}

	size_t sysMemSparseAllocator::GetFreeListForSize(size_t size) const
	{
		// These don't have to be powers of two here.
		// We do more fine-grained lists for the smaller page sizes.
		// Do the most common sizes first.  They all have free lists that only
		// support one size so that they exhibit O(1) behavior.

		Assert(size >= 4096);

		// Handle 4k-64k (indices 0 through 15)
		if (size <= 65536)
			return (size >> 12) - 1;
		size_t result = 16;
		while (size > 128*1024 && result != 31)
		{
			result++;
			size >>= 1;
		}
		return result;
	}

	void* sysMemSparseAllocator::TryAllocate(size_t size,size_t align,int /*heapIndex*/ /* = 0 */)
	{
		SYS_CS_SYNC(m_Token);

		// Sigh.
		if (!size)
			size = 1;
		if (!align)
			align = 1;

		size_t alignedSize = (size + (align-1)) & ~(align-1);

		Assertf((m_MemoryUsed + alignedSize) < m_FakeHeapSize, "About to allocate over fake limit of memory - Used %" I64FMT "d Alignment %" I64FMT "d Max Heap Size % "I64FMT "d", m_MemoryUsed, alignedSize, m_FakeHeapSize);

		size_t alignMask = (align < sizeof(SparsePage)? sizeof(SparsePage) : align) - 1;
		size_t pageAlignedSize = (alignedSize + sizeof(SparsePage)-1) & ~(sizeof(SparsePage)-1);

		// Make sure there's at least one guard page between allocations, and the memory is suitably aligned.
		char *nextStart = (char*)(((size_t)m_NextAlloc + alignMask + 1) & ~alignMask);

		static bool pingPong;

		// Make sure we fit on this block
		if (nextStart + pageAlignedSize > m_NextEnd)
		{
			// If we're full, start recycling memory from the least-recently used
			// We maintain a large number of free lists to try to keep this reasonably fast.
			if (m_PageCount == MaxPages)
			{
				FreeList &fl = m_FreeLists[GetFreeListForSize(pageAlignedSize)];

				AllocInfo *best = NULL;
				size_t bestSize = ~0U;

				s32 prev = -1, cur = fl.First, bestPrev = -1;
				while (cur != -1)
				{
					AllocInfo &ai = GetAlloc(cur);
					Assert(!ai.used);		// shouldn't be on this list otherwise!
					// An exact match is very common due to our coarse granularity - abort immediately
					if (ai.pageAlignedSize == pageAlignedSize)
					{
						bestPrev = prev;
						best = &ai;
						bestSize = ai.pageAlignedSize;
						break;
					}
					// Otherwise look for the best fit
					else if (ai.pageAlignedSize > pageAlignedSize && ai.pageAlignedSize < bestSize)
					{
						bestPrev = prev;
						best = &ai;
						bestSize = ai.pageAlignedSize;
					}
					// Update linked list traversal pointers (need to remember previous for patching list on removal)
					prev = cur;
					cur = ai.alignedSize;
				}

				if (!best)
				{
					Errorf("sysMemSparseAllocator - heap is totally exhausted???");
					return NULL;
				}
				else
				{
					// Re-commit the memory, but use the possibly smaller pageAlignedSize we want now, not what it had before.
					char *result = (char*) VirtualAlloc((LPVOID)best->memory,pageAlignedSize,MEM_COMMIT,PAGE_READWRITE);
					if (!result) {
						Errorf("Unable to recycle memory?");
						return NULL;
					}

					if (!PARAM_sparseleft.Get() || (PARAM_sparseboth.Get() && pingPong))
					{
						memset(result, 0xC3, pageAlignedSize);  // DEBUG_MEMORY_FILL
						result += (pageAlignedSize - alignedSize);
					}
					else
						memset(result, 0xC5, pageAlignedSize);	// DEBUG_MEMORY_FILL
					pingPong = !pingPong;

					// Remove this item from the singly-linked free list, updating head pointer if necessary
					if (bestPrev != -1)
					{
						GetAlloc(bestPrev).alignedSize = best->alignedSize;

						// If the item was last, update the last pointer too and re-terminate the list.
						if (&GetAlloc(fl.Last) == best)
							GetAlloc(fl.Last = bestPrev).alignedSize = -1;
					}
					else
					{
						fl.First = best->alignedSize;
					}

					// Update the memory tracking information
					best->alignedSize = u32(alignedSize);
					best->pageAlignedSize = u32(pageAlignedSize);
					best->used = 1;
					best->memory = result;
					m_MemoryUsed += alignedSize;

					return result;
				}
			}

			// I'm not convinced the following assert adds value...
			//Assert(pageAlignedSize/sizeof(SparsePage) < AllocsPerBlock);

			Assert(m_PageCount != MaxPages);
			// Allocate the entire sparse page, mostly uncommitted
			m_Pages[m_PageCount] = (SparsePageHeader*) VirtualAlloc(NULL,sizeof(SparsePageHeader),MEM_RESERVE,PAGE_READWRITE);
			// And make the first part real memory for our header data.
			VirtualAlloc(m_Pages[m_PageCount],OffsetOf(SparsePageHeader,arenaBegin),MEM_COMMIT,PAGE_READWRITE);
			m_Pages[m_PageCount]->allocCount = 0;
			m_NextAlloc = &m_Pages[m_PageCount]->arena[0][0];
			// Recompute next starting address again in case alignment was unusually strict (but we know there's already a guard page in front of it)
			nextStart = (char*)(((size_t)m_NextAlloc + alignMask) & ~alignMask);
			m_NextEnd = &m_Pages[m_PageCount]->arenaEnd[0][0];
			m_PageCount++;
		}

		SparsePageHeader &ph = *m_Pages[m_PageCount-1];

		char *result = (char*) VirtualAlloc((LPVOID)nextStart,pageAlignedSize,MEM_COMMIT,PAGE_READWRITE);

		if (!result) {
			Errorf("Unable to alloc new memory?");
			return NULL;
		}

		// Bias the result as high as possible to catch overruns
		// printf("Block @%p, %u bytes, returning %p (%u allocated)\n",result,pageAlignedSize,result+(pageAlignedSize-alignedSize),alignedSize);
		if (!PARAM_sparseleft.Get() || (PARAM_sparseboth.Get() && pingPong))
		{
			memset(result, 0xC3, pageAlignedSize);  // DEBUG_MEMORY_FILL
			result += (pageAlignedSize - alignedSize);
		}
		else
			memset(result, 0xC5, pageAlignedSize);	// DEBUG_MEMORY_FILL
		pingPong = !pingPong;

		Assert(ph.allocCount < AllocsPerBlock);
		AllocInfo &ai = ph.allocInfo[ph.allocCount++];
		ai.alignedSize = u32(alignedSize);
		ai.pageAlignedSize = u32(pageAlignedSize);
		ai.used = 1;
		ai.memory = nextStart;

		m_NextAlloc = nextStart + pageAlignedSize;
		m_MemoryUsed += alignedSize;

		return result;
	}

	void* sysMemSparseAllocator::Allocate(size_t size,size_t align,int heapIndex /* = 0 */)
	{
		void *result = TryAllocate(size,align,heapIndex);
		if (!result)
			Quitf(ERR_MEM_SPARSEALLOC_ALLOC,"Sparse Allocator failed to allocate %" SIZETFMT "u (ran out of heap address space?). Total %" SIZETFMT "u Used %" SIZETFMT "u Available %" SIZETFMT "u Largest %" SIZETFMT "u", size, GetHeapSize(), GetMemoryUsed(), GetMemoryAvailable(), GetLargestAvailableBlock());
		return result;
	}

	struct LookupByAddr
	{
		// std::lower_bound returns the leftmost element in the array that does NOT pass the predicate.
		// We still need to make sure that the pointer is within this region, since the region may have
		// already been previously removed.
		bool operator()(sysMemSparseAllocator::AllocInfo const &a,sysMemSparseAllocator::AllocInfo const &b)
		{
			return (a.memory + a.pageAlignedSize) <= b.memory;
		}
	};

	sysMemSparseAllocator::AllocInfo* sysMemSparseAllocator::FindAlloc(const void *ptr,s32 *indexPtr) const
	{
		for (size_t i=0; i<m_PageCount; i++)
		{
			if (((SparsePage*)ptr >= m_Pages[i]->arena) && ((SparsePage*)ptr < m_Pages[i]->arenaEnd))
			{
				SparsePageHeader *currentHeader = m_Pages[i];
				sysMemSparseAllocator::AllocInfo search; search.memory = (char*)ptr; search.pageAlignedSize = 1;
				AllocInfo *j = std::lower_bound(currentHeader->allocInfo,currentHeader->allocInfo + currentHeader->allocCount,search,LookupByAddr());
				if (indexPtr)
					*indexPtr = int(i << 14) | ptrdiff_t_to_int(j - currentHeader->allocInfo);
				// If we're past the end of the list, or the returned value starts *after* the pointer we care about, return nothing.
				return (j == currentHeader->allocInfo + currentHeader->allocCount || j->memory > ptr)? NULL : j;
			}
		}
		return NULL;
	}

	sysMemSparseAllocator::AllocInfo& sysMemSparseAllocator::GetAlloc(s32 index) const
	{
		size_t pi = (u32)index >> 14;
		size_t si = index & 16383;
		Assert(pi < m_PageCount);
		SparsePageHeader &ph = *m_Pages[pi];
		Assert(si < ph.allocCount);
		return ph.allocInfo[si];
	}

	void sysMemSparseAllocator::Free(const void *ptr)
	{
		SYS_CS_SYNC(m_Token);
		if(!ptr)
			return;

		s32 index;
		AllocInfo *ai = FindAlloc(ptr,&index);
		if (!ai)
			Quitf(ERR_MEM_SPARSEALLOC_FREE_1,"Bug in sysMemSpareAllocator::Free, heap %p doesn't know about pointer %p",this,ptr);
		if (!ai->used)
			Quitf(ERR_MEM_SPARSEALLOC_FREE_2,"Double-free of memory at %p",ptr);
		m_MemoryUsed -= ai->alignedSize;

#if 0
		// Turn the memory into a guard page.  This will still throw an exception that will crash the game, but it
		// will be a different kind of exception so it's easier to tell a bad pointer from a stale one.
		DWORD oldProtect;
		if (!VirtualProtect((LPVOID)ai->memory, ai->pageAlignedSize, PAGE_GUARD | PAGE_READONLY, &oldProtect))
#else
		if (!VirtualFree((LPVOID)ai->memory, ai->pageAlignedSize, MEM_DECOMMIT))
#endif
		{
			Quitf(ERR_MEM_SPARSEALLOC_FREE_3,"Bug in sysMemSpareAllocator::Free");
		}

		ai->used = 0;
		ai->alignedSize = -1;	// terminate free list
		// Update free list connectivity (first free and last free can only be -1 at the same time)
		FreeList &fl = m_FreeLists[GetFreeListForSize(ai->pageAlignedSize)];
		if (fl.First == -1)
			fl.First = index;
		else
			GetAlloc(fl.Last).alignedSize = index;
		fl.Last = index;
	}

	void sysMemSparseAllocator::Resize(const void * /*ptr*/,size_t /*newSmallerSize*/)
	{
		// Does nothing
	}

	size_t sysMemSparseAllocator::GetSize(const void *ptr) const
	{
		AllocInfo *ai = FindAlloc(ptr);
		if (ai && ai->used)
			return ai->alignedSize;
		else
			return 0;
	}

	size_t sysMemSparseAllocator::GetMemoryUsed(int /*bucket*/ /*all buckets*/)
	{
		return m_MemoryUsed;
	}

	size_t sysMemSparseAllocator::GetMemoryAvailable()
	{
		return m_FakeHeapSize - m_MemoryUsed;
	}

	bool sysMemSparseAllocator::IsValidPointer(const void* ptr) const
	{
		for (size_t i=0; i<m_PageCount; i++)
			if (((SparsePage*)ptr >= m_Pages[i]->arena) && ((SparsePage*)ptr < m_Pages[i]->arenaEnd))
				return true;
		return false;
	}

	size_t sysMemSparseAllocator::GetSizeWithOverhead(const void *ptr) const
	{
		AllocInfo *ai = FindAlloc(ptr);
		return ai? ai->pageAlignedSize : 0;
	}

	size_t sysMemSparseAllocator::GetHeapSize() const
	{
		return m_FakeHeapSize;
	}

	char* sysMemSparseAllocator::GetSubHeapBase(size_t idx) const
	{ 
		return &m_Pages[idx]->arena[0][0]; 
	}

	size_t sysMemSparseAllocator::GetSubHeapCount() const
	{
		return m_PageCount;
	}

	size_t sysMemSparseAllocator::GetSubHeapSize()
	{ 
		return PagesPerBlock * sizeof(SparsePage); 
	}

}

#endif	// RSG_PC
