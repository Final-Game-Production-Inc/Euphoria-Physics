// 
// system/smallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_SMALLOCATOR_H
#define SYSTEM_SMALLOCATOR_H

#include <stddef.h>		// for size_t

#define SMALL_ALLOCATOR_DEBUG_MIN_ALIGNMENT     (__DEV && !__RESOURCECOMPILER && !__PS3 && !__XENON)

namespace rage {

class sysMemSimpleAllocator;

/*
	This class is very similar to the atPool and atStaticPool classes but is designed to
	be integrated directly within a containing memory allocator and therefore has some
	slightly different resizing semantics.  It parcels out like-sized headerless memory
	chunks as necessary, adding additional chunks as necessary.  Likewise, if the last
	parcel in a chunk is freed, it reclaims the entire parcel from the underlying
	memory manager.  It assumes that its chunk size is much larger than the entry size
	so that infinite recursion is not a problem.
*/
class sysSmallocator {
	friend class sysMemSimpleAllocator;

	static const int MEMORY_SYSTEM_OVERHEAD = 16;	 // Estimated amount of overhead in the heap for the allocation of the 16KB page itself
	static const int CHUNK_ALIGNMENT_SHIFT = 14;
	static const size_t CHUNK_ALIGNMENT = 1<<CHUNK_ALIGNMENT_SHIFT;
public:
	static const size_t MAX_SMALLOCATOR_SIZE = 128;
private:

	// Internal chunk structure, contains its storage directly.  Based on atStaticPool code.
	struct Chunk {
		Chunk *m_Prev;		// Previous chunk in list
		Chunk *m_Next;		// Next chunk in list
		int m_FreeCount;	// Number of free elements; always between 0 and m_MaxFreeCount-1
		void *m_FirstFree;	// Pointer to first free element
		sysSmallocator *m_Owner;	// Pointer to owning smallocator
		int m_Bucket;		// optional mask limiting which buckets will use this
		size_t padding[2];	// Pad to 32 bytes (64 on 64bit machines) to guarantee proper alignment.
		u8 m_Storage[0];	// The actual memory storage of the chunk

		// PURPOSE:	Free a parcel of memory of the configured size.
		// PARAMS:	ptr - Pointer to memory to free
		//			allocator - Reference to allocator object to return chunk to if it goes totally free.
		void Free(const void *ptr,sysMemSimpleAllocator& allocator);
	};

	static const int CHUNK_SIZE = CHUNK_ALIGNMENT - sizeof(Chunk) - MEMORY_SYSTEM_OVERHEAD;

public:
	// PURPOSE:	Constructor
	sysSmallocator() : m_First(NULL), m_EntrySize(0), m_ChunkCount(0) {}

	// PURPOSE:	Constructor
	// PARAMS:	size - Size of each element.  The number of elements in each chunk is fixed.
	sysSmallocator(u16 size) : m_First(NULL), m_EntrySize(size), m_ChunkCount(CHUNK_SIZE / size)
	{ 
		AssertMsg(size >= sizeof(void*),"Can't make smallocator smaller than pointer size");
	}

	// PURPOSE:	Set the entry size (needed if the default constructor is used)
	void Create(u16 size)
	{
		AssertMsg(size >= sizeof(void*),"Can't make smallocator smaller than pointer size"); 

		m_First = NULL;
		m_EntrySize = size;
		m_ChunkCount = CHUNK_SIZE / size;
	}

	// PURPOSE:	Allocate a parcel of memory of the configured size.
	// PARAMS:	allocator - Reference to allocator object to obtain another chunk from if necessary.
	// NOTES:	This function uses the special DoAllocate entry point since the memory semaphore is
	//			typically already taken and our spinlock class doesn't support recursion.
	// RETURNS:	Pointer to new chunk.  Can never fail unless allocate.DoAllocate fails.
	void* Allocate(sysMemSimpleAllocator& allocator);

	// RETURNS: Entry size
	// NOTES:   Be careful with SMALL_ALLOCATOR_DEBUG_MIN_ALIGNMENT, when that is enabled,
	//          GetEntrySize will return a larger value than higher level code may be expecting.
	u16 GetEntrySize() const { return m_EntrySize; }

	// RETURNS: Total number of unused bytes in the page
	size_t GetMemoryAvailable();

	void SanityCheck();

	// PURPOSE: Prints out how many allocations remain in this smallocator
	// RETURNS: The number of items leaked
	size_t PrintLeakData(bool printAsError);
private:
	Chunk *m_First;
	u16 m_EntrySize;
	u16 m_ChunkCount;
};

}	// namespace rage

#endif
