// 
// system/externalheap.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_EXTERNALHEAP_H
#define SYSTEM_EXTERNALHEAP_H

#include <stddef.h>		// for size_t

namespace rage {

// This is a macro so the macro below can... well... be a macro.
#define EXTERNAL_HEAP_WORKSPACE_HASH_SIZE	1024

#define EXTERNAL_HEAP_ALLOC_ID				(__DEV || RSG_PC)

// This is a macro so that it can be used to size simple C byte arrays.
// Keep this in synch with sysExternalHeap::Node class (done explicitly here so that
// the helper class can stay private).
#if RSG_CPU_INTEL
#define EXTERNAL_HEAP_NODE_SIZE	(20 + 4*EXTERNAL_HEAP_ALLOC_ID)
#else
#define EXTERNAL_HEAP_NODE_SIZE (16 + 8*EXTERNAL_HEAP_ALLOC_ID)
#endif

#define COMPUTE_WORKSPACE_SIZE(maxNodes)	((EXTERNAL_HEAP_WORKSPACE_HASH_SIZE*sizeof(::rage::u32)) + ((maxNodes)*EXTERNAL_HEAP_NODE_SIZE))

/*
	sysExternalHeap manages a region of arbitrary address space that it never actually
	touches.  This avoids node overhead (particularly with large alignment values) and
	also allows us to manage memory the CPU either cannot or simply should not access.
	We also use it for constructing resource heaps, which by design never have any
	internal nodes.

	Allocations search a single free list for best-fit.  Allocation is linear in the
	number of free nodes with constant-time overhead after a suitable node is found.
	
	Free operations go through a hash table to locate the memory to be freed, which
	in practice ought to result in constant-time overhead barring serious error in
	the hashing function.  Once the node is found, it is patched and adjacent free nodes
	are immediately coalesced in constant time, with one special case when both the
	predecessor and successor are not in use, in which case we have to run a loop
	linear in the number of free nodes.

	When you configure the external heap you have to tell it the maximum number of nodes
	you expect to support (this should be roughly the total number of memory allocations,
	although free nodes due to alignment come from this pool as well).  There is fixed-
	size overhead for the table, and 16 (20 on __WIN32PC) bytes of overhead for each node,
	which is comparable with our normal intrusive memory allocator class.
*/
class sysExternalHeap {
public:
	// PURPOSE:	Initializes the external heap.
	// PARAMS:	heapSize - Size of the address space being managed, in bytes.
	//			maxNodes - Maximum number of pointer allocations we expect to have.
	//			workspace - Pointer to extra workspace memory (use COMPUTE_WORKSPACE_SIZE macro)
	//			chunkSize - If nonzero, this limits the maximum allocation size, and also
	//				guarantees that an allocation never crosses a multiple of this value.
	//				Should be a multiple of the strictest alignment you expect to have in the heap.
	// NOTES:	The external heap does no extra memory allocation on its own, so you can simply
	//			call Init again to reinitialize it, or simply stop using it when you're done with
	//			it and reclaim the workspace memory.
	void Init(size_t heapSize,u32 maxNodes,void *workspace,size_t chunkSize);

	// PURPOSE:	Allocate memory from the heap
	// PARAMS:	size - Allocation size
	//			align - Allocation alignment
	// RETURNS:	Offset in the heap of the memory, or ~0U if unavailable.
	size_t Allocate(size_t size,size_t align);

	// PURPOSE: Allocate memory from the top of the heap
	// PARAMS:	size - Allocation size
	//			align - Allocation alignment
	// RETURNS:	Offset in the heap of the memory, or ~0U if unavailable.
	// NOTES:	This memory is not reclaimable, so is only intended for use by resource heap
	//			construction.  It also has no node overhead.
	size_t AllocateFromTop(size_t size,size_t align);

	// PURPOSE:	Free memory from the heap
	// PARAMS:	offset - Value returned by previous Allocate call; ~0U is ignored.
	// RETURNS:	True if memory could be freed, false if invalid pointer.
	bool Free(size_t offset);

	// RETURNS:	Amount of memory used in the heap (not counting workspace)
	size_t GetMemoryUsed(int bucket) const { return bucket==-1? m_MemUsed : m_MemoryUsedByBucket[bucket&15]; }

	// RETURNS:	Total amount of memory in the heap (not counting workspace)
	size_t GetMemoryTotal() const { return m_MemTotal; }

	// RETURNS: Total amount of memory allocated from top
	size_t GetTopTotal() const { return m_TopTotal; }

	// RETURNS: Total amount of available memory in the heap (not counting workspace)
	size_t GetMemoryFree() const { return m_MemTotal - m_MemUsed; }

	// RETURNS: Size of largest free block
	size_t GetLargestAvailableBlock();

	// RETURNS: Index of best fit block
	u32 GetBestFitNode(size_t size, size_t align) const;

	// PURPOSE:	Run a sanity check on the entire heap.
	void SanityCheck();

	// RETURNS: Current chunk size (zero if none in use)
	size_t GetChunkSize() const { return m_ChunkSize; }

	// RETURNS: Offset of the start of the last block on the heap if it's free,
	//			otherwise the size of the entire heap.
	size_t GetMemoryEnd() const;

	// PURPOSE:	Dump memory leaks
	void DumpLeaks() const;

	// PURPOSE: Returns size of the memory block at this offset, if known.
	// NOTES:	There is essentially no per-allocation overhead, so there is no GetSizeWithOverhead function.
	//			All overhead is fixed at the time of the heap's creation.
	size_t GetSize(size_t offset) const;

	// RETURNS:	Largest allocation seen since heap creation or last ResetLargestAllocation call.
	size_t GetLargestAllocation() const { return m_LargestAllocation; }

	// PURPOSE:	Resets largest allocation size counter.
	void ResetLargestAllocation() { m_LargestAllocation = 0; }

private:
	inline void ReturnNode(u32 idx);
	void VerifyInUse(size_t offset);

	/* A memory manager needs to do several operations quickly:
		- When allocating memory, we need to find an existing block suitable for our needs.
		  This requires iterating over all free nodes (preferably organized by size).
	    - One the memory node is found, we need to quickly remove ourselves from the free
		  list and go back onto the used list.
	    - When freeing memory, we need to quickly map the offset back to the memory node.
		  This is best done with an extra data structure, and is only required for nodes
		  that are in use, not freed nodes (or nodes left over due to alignment). 
		All nodes are kept in one of three places at all times -- the available node list
		(in which case m_PrevAddr and m_NextAddr and m_Used are all irrelevant), the
		free node list, or the hash chain associated with an in-used memory block.
	*/
	struct Node {
		static const u32 MAX_NODES = (1<<20)-1;
#if RSG_CPU_INTEL
		u32 m_PrevAddr,		// Previous node in address space (either used or free)
			m_NextAddr,		// Next node in address space (either used or free)
			m_Bucket:4,
			m_Link:26,		// Next node in hash chain (only for used memory nodes)
							// Next node in free list (only for free memory nodes)
							// Next node in node list (only for available nodes)
			m_Used:1,		// Flag indicating whether node is in use or not.
			m_Touched:1;	// Flag used by sanity checking code
#else
		// PowerPC has good bit operators and this code will never be resourced.
		u64 m_NextAddr: 19, m_PrevAddr:19, m_Link:20, m_Used:1, m_Touched:1, m_Bucket:4;
#endif
		u32  m_Offset,	// Offset of this memory block in the external address space
			m_Size;			// Size of the memory block.  Somewhat redundant but simplifies code.
#if EXTERNAL_HEAP_ALLOC_ID
		int m_AllocId;
#endif
	};

	u32		*m_Hash;
	Node	*m_Nodes;
	u32		m_FirstFreeIdx, m_FirstAvailIdx;
	u32		m_MaxNodes;
	size_t	m_MemUsed, m_MemTotal;
	size_t	m_SmallestAlignment;
	size_t	m_TopTotal;
	size_t	m_ChunkSize;
	size_t	m_LargestAllocation;
	size_t	m_MemoryUsedByBucket[16];
	int		m_MemAllocId;
};

}

#endif	// SYSTEM_EXTERNALHEAP_H
