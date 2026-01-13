// 
// system/smallocator.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "smallocator.h"
#include "simpleallocator.h"
#include "memory.h"
#include "diag/output.h"

#include <string.h>

namespace rage {

#define SMALLOCATOR_CHUNKS_GROUPED_BY_BUCKET	(__DEV && 0)

#define ENABLE_SMALLOCATOR_SANITY_CHECKS		0

#if ENABLE_SMALLOCATOR_CODE

void sysSmallocator::Chunk::Free(const void *ptr,sysMemSimpleAllocator& allocator)
{
	Assert (ptr >= m_Storage && ptr < m_Storage + CHUNK_SIZE);

#if ENABLE_SMALLOCATOR_SANITY_CHECKS		
	m_Owner->SanityCheck();
#endif

#if __DEV
	void *maybeFree = *(void**)ptr;
	unsigned *dd = ((unsigned*)ptr + 1);
	if (maybeFree >= m_Storage && maybeFree < m_Storage + CHUNK_SIZE && 
		((size_t)maybeFree % m_Owner->GetEntrySize()) == 0 && *dd == 0xDDDDDDDD) 
	{
		AssertMsg(0, "Smallocator: Probable double-free");
		// __debugbreak();
	}
#endif

	// Emulate VC++ behavior
	IF_DEBUG_MEMORY_FILL_N(memset((void*)ptr,0xDD,m_Owner->m_EntrySize),DMF_GAMEHEAP_FREE);
	// Patch memory into free list
	*(void**) ptr = m_FirstFree;
	m_FirstFree = (void*) ptr;
	// If chunk is totally free, reclaim it.
	if (++m_FreeCount == m_Owner->m_ChunkCount) 
	{
		if (m_Next)
			m_Next->m_Prev = m_Prev;
		if (!m_Prev)
			m_Owner->m_First = m_Next;
		else
			m_Prev->m_Next = m_Next;
		allocator.ClearPageBit(this);
		allocator.DoFree(this);
	}
}


void sysSmallocator::SanityCheck()
{
	Chunk *c = m_First;
	while (c) {
		void *p = c->m_FirstFree;
		for (int i = 0; i < c->m_FreeCount; i++)
		{
			Assert(size_t(p) >= size_t(c->m_Storage));
			Assert(size_t(p) < size_t(c->m_Storage) + CHUNK_SIZE);
			Assert(!(size_t(p) & sysMemAllocator::DEFAULT_ALIGNMENT_MASK));
			p = *(void**)p;
		}
		c = c->m_Next;
	}
}

size_t sysSmallocator::PrintLeakData(bool printAsError)
{
	// Count how many items are allocated.
	Chunk* c = m_First;
	size_t usedCount = 0;
	while(c)
	{
		usedCount += CHUNK_SIZE / m_EntrySize - c->m_FreeCount;
		c = c->m_Next;
	}

	if (usedCount)
	{
		if (printAsError)
		{
			if( !sysMemSimpleAllocator::sm_hideAllocOutput )
				Errorf("Leaks in the %u-byte smallocator: %" SIZETFMT "u items for %" SIZETFMT "u bytes. Run with -breakonalloc=0 to see allocids for these items", m_EntrySize, usedCount, usedCount * m_EntrySize);
		}
		else
		{
			if( !sysMemSimpleAllocator::sm_hideAllocOutput )
				Warningf("Leaks in the %u-byte smallocator: %" SIZETFMT "u items for %" SIZETFMT "u bytes. Run with -breakonalloc=0 to see allocids for these items", m_EntrySize, usedCount, usedCount * m_EntrySize);
		}
	}

	return usedCount;
}

void* sysSmallocator::Allocate(sysMemSimpleAllocator &allocator) {
#if ENABLE_SMALLOCATOR_SANITY_CHECKS		
	SanityCheck();
#endif

	Chunk *c = m_First;
	while (c) 
	{
		if (c->m_FreeCount > 0
#if SMALLOCATOR_CHUNKS_GROUPED_BY_BUCKET
				&& c->m_Bucket == sysMemCurrentMemoryBucket
#endif
			) 
		{
			void *result = c->m_FirstFree;

			void *ff = *(void**)c->m_FirstFree;
			Assertf(!ff || (u8*)ff >= c->m_Storage && (u8*)ff < (u8*)c + CHUNK_ALIGNMENT && !((size_t)ff & sysMemAllocator::DEFAULT_ALIGNMENT_MASK),"Corrupted free list (%p either unaligned or not in %p..%p)",ff,c->m_Storage,(u8*)c + CHUNK_ALIGNMENT);
			c->m_FirstFree = ff;
#if 1
			--c->m_FreeCount;
#else		// The extra work here seems to be a wash, annoyingly enough.
			if (--c->m_FreeCount && c != m_First) 
			{
				// If there's anything left on this free list, sort it to the front
				// so that the next allocation will be faster.
				Assert(c->m_Prev);
				// Patch ourselves out of the list
				c->m_Prev->m_Next = c->m_Next;
				if (c->m_Next)
					c->m_Next->m_Prev = c->m_Prev;
				// ...and back in at the head
				c->m_Next = m_First;
				m_First->m_Prev = c;
				c->m_Prev = NULL;
				m_First = c;
			}
#endif
			NOTFINAL_ONLY(allocator.BumpAllocId()); // Only do it here since it gets bumped in the main allocator if we're allocing a new page

			IF_DEBUG_MEMORY_FILL_N(memset(result, 0xCD, GetEntrySize()),DMF_GAMEHEAP_ALLOC);
			return result;
		}
		c = c->m_Next;
	}

#if !SMALLOCATOR_CHUNKS_GROUPED_BY_BUCKET
	// If we're NOT grouping chunks by bucket, make sure all smallocator chunks go to default bucket.
	// Otherwise the allocation should be billed to the current bucket normally.
	sysMemUseMemoryBucket SMALLOCATOR(MEMBUCKET_DEFAULT);
#endif
	Chunk *newChunk = (Chunk*) allocator.DoAllocate(CHUNK_ALIGNMENT - MEMORY_SYSTEM_OVERHEAD,CHUNK_ALIGNMENT);

	// Normally this doesn't happen with our code but fail gracefully if it does.
	if (!newChunk)
		return NULL;

	allocator.SetPageBit(newChunk);
	if (m_First)
		m_First->m_Prev = newChunk;

	newChunk->m_Prev = NULL;
	newChunk->m_Next = m_First;
	newChunk->m_FreeCount = m_ChunkCount-1;
	Assert(newChunk->m_FreeCount > 0);
	newChunk->m_Owner = this;
	newChunk->m_Bucket = sysMemCurrentMemoryBucket;	// also available in the enclosing node but we have free space here.

	// Initialize the free list.
	u8 *p = newChunk->m_Storage + m_EntrySize;
	newChunk->m_FirstFree = p;
	Assert(0 == (((size_t) newChunk->m_FirstFree) & sysMemAllocator::DEFAULT_ALIGNMENT_MASK));
	
	int count = m_ChunkCount-1;
	while (--count) 
	{
		*(u8**)p = (p+m_EntrySize);
		p += m_EntrySize;
	}
	// Terminate the free list.
	*(u8**)p = NULL;

	m_First = newChunk;
	IF_DEBUG_MEMORY_FILL_N(memset(newChunk->m_Storage, 0xCD, GetEntrySize()),DMF_GAMEHEAP_ALLOC);
	return (void*) newChunk->m_Storage;
}

size_t sysSmallocator::GetMemoryAvailable()
{
	int bytes = 0;
	Chunk* chunk = m_First;
	while (chunk)
	{
		bytes += (chunk->m_FreeCount * m_EntrySize);
		chunk = chunk->m_Next;
	}

	return bytes;
}

#endif // ENABLE_SMALLOCATOR_CODE

}	// namespace rage
