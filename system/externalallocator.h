// 
// system/packedallocator.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_EXTERNALALLOCATOR_H
#define SYSTEM_EXTERNALALLOCATOR_H

#include "system/memory.h"
#include "system/externalheap.h"

namespace rage {

/*
	This class bridges sysExternalHeap and our memory classes to
	that it can be used as a normal low-overhead (albeit slower)
	game heap.  All memory it uses is owned by its creator.
*/
class sysMemExternalAllocator: public sysMemAllocator 
{
public:
	// PURPOSE:	Constructor
	// PARAMS:	heapSize - Total size of the heap to allocate
	//			heapBase - Base address of the heap (never actually referenced, but this
	//				is the base address Allocate and Free are relative to)
	//			maxPointers - Maximum number of memory nodes to reserve
	//			workspace - Address of a buffer at least COMPUTE_WORKSPACE_SIZE(maxPointers) bytes long.
	//			chunkSize - If nonzero, limit allocations to this size, and make sure allocations never cross
	//				an offset that is a multiple of this size either.
	sysMemExternalAllocator(size_t heapSize,void *heapBase,size_t maxPointers,void *workspace,size_t chunkSize = 0);

	// PURPOSE:	Destructor
	~sysMemExternalAllocator();

	void* Allocate(size_t size,size_t align,int heapIndex);

	void* AllocateFromTop(size_t size,size_t align);

	void Free(const void *ptr);

	size_t GetMemoryUsed(int);

	size_t GetHeapSize(void) const
	{
		return m_Size;
	}

	size_t GetMemoryAvailable();

	size_t GetLargestAvailableBlock();

	RAGE_MEMORY_DEBUG_ONLY(size_t GetHighWaterMark(bool reset);)

	virtual bool IsValidPointer(const void * ptr) const;

	// RETURNS:	Reference to underlying sysExternalHeap object.
	const sysExternalHeap& GetHeap() const { return m_Heap; }

	// RETURNS: Heap base
	void *GetHeapBase() const { return m_HeapBase; }

	void SetHeapBase(void *hb) { m_HeapBase=hb; }

	// RETURNS: Workspace
	void *GetWorkspace() const { return m_Workspace; }

	// RETURNS: Chunk size
	size_t GetChunkSize() const { return m_Heap.GetChunkSize(); }

	size_t GetLargestAllocation() const { return m_Heap.GetLargestAllocation(); }

	virtual size_t GetSize(const void *ptr) const;

private:
	void			*m_HeapBase;
	void			*m_Workspace;
	size_t			m_Size;
	sysExternalHeap m_Heap;

	RAGE_MEMORY_DEBUG_ONLY(size_t m_watermark;)
};

}	// namespace rage

#endif	// SYSTEM_EXTERNALALLOCATOR
