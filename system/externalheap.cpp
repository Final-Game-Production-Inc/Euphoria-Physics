// 
// system/externalheap.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "externalheap.h"

#include "diag/output.h"
#include "memory.h"		// for sysMemAllocId

#include <string.h>

namespace rage 
{

/* Very large offsets will not happen, and the least-significant bits
	may be ignored as well.  Maps an arbitrary offset into a toplevel
	hash index directly. */
inline size_t OffsetHash(size_t x) 
{
	CompileTimeAssert(EXTERNAL_HEAP_WORKSPACE_HASH_SIZE == 1024);
	return ((x>>2) ^ (x >> 12) ^ (x >> 22)) & (EXTERNAL_HEAP_WORKSPACE_HASH_SIZE-1);
}

// NOTE: Lines marked with //! have not been confirmed as exercised code paths.
// I set breakpoints on the antecedent(s) of every if statement to make sure all
// code paths were getting utilized in sample_memory, and these paths remained
// unused by the test suite.

void sysExternalHeap::Init(size_t heapSize, u32 maxNodes, void *workspace, size_t chunkSize)
{
	CompileTimeAssertSize(Node,EXTERNAL_HEAP_NODE_SIZE,EXTERNAL_HEAP_NODE_SIZE);
	Assert(maxNodes <= Node::MAX_NODES);

	// Clear out the entire heap.
	size_t workspaceSize = COMPUTE_WORKSPACE_SIZE(maxNodes);
	m_Hash = (u32*) workspace;
	m_ChunkSize = chunkSize;
	memset(m_Hash,0,workspaceSize);
	m_Nodes = (Node*)(m_Hash + EXTERNAL_HEAP_WORKSPACE_HASH_SIZE);
	m_FirstFreeIdx = 1;
	m_FirstAvailIdx = 2;

	// We don't use entry zero so that it's available as a list terminator.
	// Initialize the game heap with a single large empty block.
	// (All other fields are already zero from the memset above)
	Assign(m_Nodes[1].m_Size,heapSize);

	// Chain all unused nodes together now
	// (The last node is left with its m_Link field zeroed out, terminating the list)
	for (u32 i = 2; i < maxNodes-1; i++)
		m_Nodes[i].m_Link = i+1;

	m_MemUsed = 0;
	m_MemTotal = heapSize;
	m_MaxNodes = maxNodes;
	m_TopTotal = 0;
	memset(m_MemoryUsedByBucket,0,sizeof(m_MemoryUsedByBucket));

	// Track the smallest alignment we've seen so far.
	// This lets us eliminate bogus pointers in free much more quickly for the main
	// physical memory heap.
	m_SmallestAlignment = ~0U;

	m_LargestAllocation = 0;

	// Start beyond the highest possible simpleallocator alloc id
	m_MemAllocId = 1<<26;
}

u32 sysExternalHeap::GetBestFitNode(size_t size, size_t align) const
{
	//// SanityCheck();
	//if (align < m_SmallestAlignment)
	//	m_SmallestAlignment = align;

	// Guarantee some sane minimum alignment to avoid massive node consumption.
	Assert(align >= 4);

	// Make sure alignment is compatible with chunk size
	Assert(!m_ChunkSize || ((m_ChunkSize % align) == 0));

	if (m_ChunkSize && size > m_ChunkSize) {
		Assertf(0, "ExternalHeap chunk size is %" SIZETFMT "u but allocation is larger, %" SIZETFMT "u", m_ChunkSize, size);
		return ~0U;
	}

	// Less than two nodes available (worst-case because of alignment-induced padding 
	// both before and after)?  Bail now.  Massively simpler to abort early before we
	// leave data structures in a half-munged state.^
	if (!m_FirstAvailIdx || !m_Nodes[m_FirstAvailIdx].m_Link) {
		// Note: We're using Verifyf rather than Quitf here to make this error
		// ignorable. After all, with proper error checking, this is not fatal.
		(void) Verifyf(0, "ExternalHeap ran out of pointers - more than %d used. Define SIMPLE_PHYSICAL_NODES where you allocate the memory system. Memory used: %" SIZETFMT "u out of %" SIZETFMT "u.", m_MaxNodes, m_MemUsed, m_MemTotal);
		return ~0U;
	}

	size_t alignMask = (align-1);
	AssertMsg((align & alignMask) == 0,"align must be power o' two");	// it's March 17th today.

	// Round size up to a multiple of its alignment
	size = (size + alignMask) & ~alignMask;

	// Search free list for best-fit allocation
	u32 bestNodeIdx = ~0U;					// node index of best fit found so far
	//u32 prevBestIdx = ~0U;					// node index of node previous to best found in free list
	size_t bestSize = ~0U;				// size of the best fit we've seen
	// size_t bestStart = ~0U;
	u32 idx = m_FirstFreeIdx;//,			// current node in free list
		//prevIdx = ~0U;					// previous node in free list
	while (idx) {
		Node &n = m_Nodes[idx];
		if (!n.m_Used) {
			// Get usable start and end of this block relative to our current alignment
			size_t nStart = (n.m_Offset + alignMask) & ~alignMask;
			size_t nEnd = (n.m_Offset + n.m_Size) & ~alignMask;
			size_t nSize = nEnd - nStart;

			// If there's a chunk size, see how much memory we actually need and
			// check to see if that crosses a chunk boundary.  If it does, adjust
			// our starting point and recompute the size.
			if (m_ChunkSize) {
				size_t nActualEnd = nStart + size;
				size_t nStartChunk = nStart / m_ChunkSize;
				size_t nEndChunk = nActualEnd / m_ChunkSize;
				if (nStartChunk != nEndChunk) {
					Assert(nStartChunk+1 == nEndChunk);
					nStart = nEndChunk * m_ChunkSize;
					nSize = nEnd - nStart;
				}
			}

			// Track best fit seen so far, but also make sure it's valid
			if (nEnd > nStart && nSize >= size && nSize < bestSize) {
				//prevBestIdx = prevIdx;
				//bestStart = nStart;
				bestSize = nSize;
				bestNodeIdx = idx;
				// If it's exact, we might as well stop now.
				if (nSize == size)
					break;
			}
		}
		//prevIdx = idx;
		idx = n.m_Link;
	}

	return bestNodeIdx;
}

size_t sysExternalHeap::Allocate(size_t size,size_t align)
{
#if !__FINAL
	if (++m_MemAllocId == sysMemAllocator::sm_BreakOnAlloc)
		__debugbreak();
#endif

	//// SanityCheck();
	if (align < m_SmallestAlignment)
		m_SmallestAlignment = align;

	// Guarantee some sane minimum alignment to avoid massive node consumption.
	Assert(align >= 4);

	// Make sure alignment is compatible with chunk size
	Assert(!m_ChunkSize || ((m_ChunkSize % align) == 0));

	if (m_ChunkSize && size > m_ChunkSize) {
		Assertf(0, "ExternalHeap chunk size is %" SIZETFMT "u but allocation is larger, %" SIZETFMT "u", m_ChunkSize, size);
		return ~0U;
	}

	// Less than two nodes available (worst-case because of alignment-induced padding 
	// both before and after)?  Bail now.  Massively simpler to abort early before we
	// leave data structures in a half-munged state.^
	if (!m_FirstAvailIdx || !m_Nodes[m_FirstAvailIdx].m_Link) {
		// Note: We're using Verifyf rather than Quitf here to make this error
		// ignorable. After all, with proper error checking, this is not fatal.
		(void) Verifyf(0, "ExternalHeap ran out of pointers - more than %d used. Define SIMPLE_PHYSICAL_NODES where you allocate the memory system. Memory used: %" SIZETFMT "u out of %" SIZETFMT "u.", m_MaxNodes, m_MemUsed, m_MemTotal);
		return ~0U;
	}

	size_t alignMask = (align-1);
	AssertMsg((align & alignMask) == 0,"align must be power o' two");	// it's March 17th today.

	// Round size up to a multiple of its alignment
	size = (size + alignMask) & ~alignMask;

	// Search free list for best-fit allocation
	u32 bestNodeIdx = 0;					// node index of best fit found so far
	u32 prevBestIdx = ~0U;					// node index of node previous to best found in free list
	size_t bestSize = ~0U;				// size of the best fit we've seen
	size_t bestStart = ~0U;
	u32 idx = m_FirstFreeIdx,			// current node in free list
		prevIdx = ~0U;					// previous node in free list
	while (idx) {
		Node &n = m_Nodes[idx];
		if (!n.m_Used) {
			// Get usable start and end of this block relative to our current alignment
			size_t nStart = (n.m_Offset + alignMask) & ~alignMask;
			size_t nEnd = (n.m_Offset + n.m_Size) & ~alignMask;
			size_t nSize = nEnd - nStart;

			// If there's a chunk size, see how much memory we actually need and
			// check to see if that crosses a chunk boundary.  If it does, adjust
			// our starting point and recompute the size.
			if (m_ChunkSize) {
				size_t nActualEnd = nStart + size;
				size_t nStartChunk = nStart / m_ChunkSize;
				size_t nEndChunk = (nActualEnd-1) / m_ChunkSize;
				if (nStartChunk != nEndChunk) {
					Assert(nStartChunk+1 == nEndChunk);
					nStart = nEndChunk * m_ChunkSize;
					nSize = nEnd - nStart;
				}
			}

			// Track best fit seen so far, but also make sure it's valid
			if (nEnd > nStart && nSize >= size && nSize < bestSize) {
				prevBestIdx = prevIdx;
				bestStart = nStart;
				bestSize = nSize;
				bestNodeIdx = idx;
				// If it's exact, we might as well stop now.
				if (nSize == size)
					break;
			}
		}
		prevIdx = idx;
		idx = n.m_Link;
	}

	// No match found?  Exit immediately.
	// Otherwise we guaranteed at the top of this function that we'd have enough
	// free nodes to finish the job.
	if (!bestNodeIdx)
		return ~0U;

	// bestStart is already set up for us by the loop above now (with chunk size already
	// factored in, so save redoing that relatively expensive division work).
	// Trivially recompute bestEnd.
	Node &bestNode = m_Nodes[bestNodeIdx];
	u32 bestEnd = ptrdiff_t_to_int(bestStart + size);
	Assert(bestStart >= bestNode.m_Offset && bestEnd <= bestNode.m_Offset + bestNode.m_Size);

	// Remove this node from the free list immediately.
	// Note that it is NOT on the used list yet!
	if (prevBestIdx != ~0U)
		m_Nodes[prevBestIdx].m_Link = bestNode.m_Link;
	else
		m_FirstFreeIdx = bestNode.m_Link;
	bestNode.m_Link = 0;
	bestNode.m_Used = true;

	// Do we have leftover space before this allocation?
	u32 beforeSlop = ptrdiff_t_to_int(bestStart - bestNode.m_Offset);
	if (beforeSlop) {
		// The first node on the heap ought to be able to satisfy any alignment,
		// so we shouldn't have any slop before it so there should always be a predecessor.
		Assert(bestNode.m_PrevAddr);

		// If previous node isn't used, grow it to include our slack
		if (!m_Nodes[bestNode.m_PrevAddr].m_Used)
			m_Nodes[bestNode.m_PrevAddr].m_Size += beforeSlop;	//!
		// Otherwise previous node is in use, add a new node
		else {
			// Retrieve a node from the available node list.  Shouldn't fail because we
			// made sure there were at least two free at the top of this function.
			u32 newNodeIdx = m_FirstAvailIdx;
			Assert(newNodeIdx);
			Node &newNode = m_Nodes[newNodeIdx];
			m_FirstAvailIdx = newNode.m_Link;

			// Patch new node into main address chain
			newNode.m_PrevAddr = bestNode.m_PrevAddr;
			newNode.m_NextAddr = bestNodeIdx;
			if (bestNode.m_PrevAddr)
				m_Nodes[bestNode.m_PrevAddr].m_NextAddr = newNodeIdx;
			bestNode.m_PrevAddr = newNodeIdx;

			// Patch new node onto the free list
			newNode.m_Link = m_FirstFreeIdx;
			m_FirstFreeIdx = newNodeIdx;

			// Set up the new node's offset and size
			newNode.m_Offset = ptrdiff_t_to_int(bestStart-beforeSlop);
			newNode.m_Size = beforeSlop;
		}

		// Adjust best node's final size.
		bestNode.m_Offset += beforeSlop;
		bestNode.m_Size -= beforeSlop;
	}

	// Do we have any leftover space after this allocation?
	u32 afterSlop = ptrdiff_t_to_int((bestNode.m_Offset + bestNode.m_Size) - bestEnd);
	if (afterSlop) {
		// If next node exists and isn't used, grow it to include our slack
		if (bestNode.m_NextAddr && !m_Nodes[bestNode.m_NextAddr].m_Used) {
			Node &nextNode = m_Nodes[bestNode.m_NextAddr];	//!
			nextNode.m_Offset -= afterSlop;
			nextNode.m_Size += afterSlop;
		}
		// Otherwise next node is in use, or doesn't exist, add a new node
		else {
			// Retrieve a node from the available node list.  Shouldn't fail because we
			// made sure there were at least two free at the top of this function.
			u32 newNodeIdx = m_FirstAvailIdx;
			Assert(newNodeIdx);
			Node &newNode = m_Nodes[newNodeIdx];
			m_FirstAvailIdx = newNode.m_Link;

			// Patch new node into main address chain
			newNode.m_PrevAddr = bestNodeIdx;
			newNode.m_NextAddr = bestNode.m_NextAddr;
			if (bestNode.m_NextAddr)
				m_Nodes[bestNode.m_NextAddr].m_PrevAddr = newNodeIdx;
			bestNode.m_NextAddr = newNodeIdx;


			// Patch new node onto the free list
			newNode.m_Link = m_FirstFreeIdx;
			m_FirstFreeIdx = newNodeIdx;

			// Set up the new node's offset and size
			newNode.m_Offset = bestEnd;
			newNode.m_Size = afterSlop;
		}

		// Adjust best node's final size
		bestNode.m_Size -= afterSlop;
	}

	// Now we have any alignment taken care of.  Double-check our work.
	Assert((bestNode.m_Offset & alignMask) == 0);
	Assert(bestNode.m_Size == size);
	m_MemUsed += size;

	m_MemoryUsedByBucket[bestNode.m_Bucket = (sysMemCurrentMemoryBucket&15)] += size;

	// Finally insert the node into the hash table.
	size_t hashIdx = OffsetHash(bestNode.m_Offset);
	bestNode.m_Link = m_Hash[hashIdx];
#if EXTERNAL_HEAP_ALLOC_ID
	bestNode.m_AllocId = m_MemAllocId;
#endif
	m_Hash[hashIdx] = bestNodeIdx;

	if (size > m_LargestAllocation)
		m_LargestAllocation = size;

	// Return offset of the memory to the caller.
	// SanityCheck();
	return bestNode.m_Offset;
}


size_t sysExternalHeap::AllocateFromTop(size_t size,size_t align)
{
	u32 i = m_FirstFreeIdx;
	while (i) {
		// Find the last free node
		if (m_Nodes[i].m_Offset + m_Nodes[i].m_Size == m_MemTotal) {
			size_t offset = (m_MemTotal - size) & ~(align-1);
			// Out of memory?  (For simplicity, we also fail exactly filling
			// memory so we don't need to properly update the free list).
			if (offset <= m_Nodes[i].m_Offset)
				return ~0U;
			// Resize this memory node
			m_Nodes[i].m_Size = ptrdiff_t_to_int(offset - m_Nodes[i].m_Offset);
			// Update tracking info properly
			m_TopTotal += (m_MemTotal - offset);
			m_MemTotal = offset;
			return offset;
		}
		i = m_Nodes[i].m_Link;
	}
	// Heap is totally full?
	return ~0U;
}


inline void sysExternalHeap::ReturnNode(u32 idx) 
{
	// Zero out the node for sanity's sake, then patch it into the available list.
	Assert(idx);
	Node &node = m_Nodes[idx];
	node.m_Size = node.m_Offset = 0;
	node.m_PrevAddr = node.m_NextAddr = 0;
	node.m_Used = false;
	node.m_Link = m_FirstAvailIdx;
	m_FirstAvailIdx = idx;
}


void sysExternalHeap::VerifyInUse(size_t /*offset*/) {
#if 0	// Nice idea but too many false positives in practice!
	// Verify a pointer is in use by make sure it's within the size of the heap
	// and it is NOT in the free list.  In practice I expect the free list to be
	// shorter so it's faster to check it.
	if (offset >= m_MemUsed) {
		Warningf("sysExternalHeap::VerifyInUse - Offset %u beyond in-use part of the heap!",offset);
		return;
	}
#if __DEV
	// Verify all nodes that are marked free.
	u32 idx = m_FirstFreeIdx;
	while (idx) {
		Node &n = m_Nodes[idx];
		if (offset >= n.m_Offset && offset < n.m_Offset + n.m_Size) {
			Warningf("sysExternalHeap::VerifyInUse - Offset %u is within an already-freed node! (may be ok)",offset);
			return;
		}
		idx = n.m_Link;
	}
#endif
#endif
}


bool sysExternalHeap::Free(size_t offset)
{
	//// SanityCheck();

	// Bail immediately on bogus input
	if (offset == ~0U)
		return false;

	// Smallest alignment must be smaller than ~0U by now if we ever allocated any memory.
	// An offset incompatible with our smallest alignment must be bogus, eliminate it quickly.
	if (offset & (m_SmallestAlignment-1)) {
		VerifyInUse(offset);
		return false;
	}

	// Locate correct in-use list from toplevel hash.
	size_t hashIdx = OffsetHash(offset);
	u32 nodeIdx = m_Hash[hashIdx];
	u32 prevNodeIdx = ~0U;
	// Traverse the in-use list looking for the matching node
	while (nodeIdx && m_Nodes[nodeIdx].m_Offset != offset) {
		prevNodeIdx = nodeIdx;
		nodeIdx = m_Nodes[nodeIdx].m_Link;
	}

	// If we reached the end of the hash chain, it's not a known pointer, which may mean
	// it's the interior pointer of a resource heap.  In that case (on __DEV builds, anyway)
	// make sure that the pointer is within a known in-use node.
	if (!nodeIdx) {
		VerifyInUse(offset);
		return false;
	}

	// Remove ourselves from the hash chain.
	Node &node = m_Nodes[nodeIdx];
	if (prevNodeIdx == ~0U)
		m_Hash[hashIdx] = node.m_Link;
	else
		m_Nodes[prevNodeIdx].m_Link = node.m_Link;
	m_MemUsed -= node.m_Size;
	m_MemoryUsedByBucket[node.m_Bucket] -= node.m_Size;

	// Since we only keep a singly-linked free node list, in the one case where both
	// our predecessor and successor are freed, we have to traverse the entire free
	// list in order to patch one of the nodes out.
	bool prevIsFree = node.m_PrevAddr && !m_Nodes[node.m_PrevAddr].m_Used;
	bool nextIsFree = node.m_NextAddr && !m_Nodes[node.m_NextAddr].m_Used;
	if (prevIsFree && nextIsFree) {
		u32 prevNodeIdx = node.m_PrevAddr;
		u32 nextNodeIdx = node.m_NextAddr;
		Node &prevNode = m_Nodes[prevNodeIdx];
		Node &nextNode = m_Nodes[nextNodeIdx];
		u32 prevIdx = ~0U;
		u32 idx = m_FirstFreeIdx;

		// Search the free list for either node (should cut our average search time down)
		// We will always find one or the other unless something is seriously amiss.
		while (idx != prevNodeIdx && idx != nextNodeIdx) {
			Assert(idx);
			prevIdx = idx;
			idx = m_Nodes[idx].m_Link;
		}
		// If we found the predecessor first, patch it out of the free list
		if (idx == prevNodeIdx) {
			// Adjust successor's start and size
			u32 slop = ptrdiff_t_to_int(prevNode.m_Size + node.m_Size);
			nextNode.m_Offset -= slop;
			nextNode.m_Size += slop;

			// Patch predecessor (and ourselves) out of the address list.
			nextNode.m_PrevAddr = prevNode.m_PrevAddr;
			if (prevNode.m_PrevAddr)
				m_Nodes[prevNode.m_PrevAddr].m_NextAddr = nextNodeIdx;

			// Patch predecessor out of the free list.
			if (prevIdx == ~0U)
				m_FirstFreeIdx = prevNode.m_Link;
			else
				m_Nodes[prevIdx].m_Link = prevNode.m_Link;

			// Return the previous node to the available node pool.
			ReturnNode(prevNodeIdx);
		}
		// Otherwise we found the successor first, patch it out of the free list instead.
		else {
			// Adjust predecessor's size
			prevNode.m_Size += node.m_Size + nextNode.m_Size;

			// Patch successor (and ourselves) out of the address list.
			prevNode.m_NextAddr = nextNode.m_NextAddr;
			if (nextNode.m_NextAddr)
				m_Nodes[nextNode.m_NextAddr].m_PrevAddr = prevNodeIdx;

			// Patch successor out of the free list.
			if (prevIdx == ~0U)
				m_FirstFreeIdx = nextNode.m_Link;
			else
				m_Nodes[prevIdx].m_Link = nextNode.m_Link;

			// Return the successor node to the available node pool.
			ReturnNode(nextNodeIdx);
		}

		// Return ourselves to the available node pool as well.
		ReturnNode(nodeIdx);
	}
	// Previous is free (and next is not) so merge with predecessor and return this node to available list.
	else if (prevIsFree) {
		Node &prevNode = m_Nodes[node.m_PrevAddr];
		// Patch ourselves out between our predecessor and any successor.
		prevNode.m_NextAddr = node.m_NextAddr;
		prevNode.m_Size += node.m_Size;
		if (node.m_NextAddr)
			m_Nodes[node.m_NextAddr].m_PrevAddr = node.m_PrevAddr;
		ReturnNode(nodeIdx);
	}
	// Next is free (and previous is not) so merge with successor and return this node to available list.
	else if (nextIsFree) {
		Node &nextNode = m_Nodes[node.m_NextAddr];
		// Patch ourselves out between any predecessor and our successor.
		nextNode.m_PrevAddr = node.m_PrevAddr;
		nextNode.m_Offset -= node.m_Size;
		nextNode.m_Size += node.m_Size;
		if (node.m_PrevAddr)
			m_Nodes[node.m_PrevAddr].m_NextAddr = node.m_NextAddr;
		ReturnNode(nodeIdx);
	}
	// Neither predecessor or successor is free, we simply move onto the free list.
	else /*if (!prevIsFree && !nextIsFree)*/ {
		node.m_Link = m_FirstFreeIdx;
		node.m_Used = false;
		m_FirstFreeIdx = nodeIdx;
	}

	// SanityCheck();
	return true;
}


void sysExternalHeap::SanityCheck()
{
	// Verify the sentinel node is empty (and therefore undamaged)
#if __ASSERT
	Node &empty = m_Nodes[0];
#endif
	Assert(empty.m_NextAddr == 0);
	Assert(empty.m_PrevAddr == 0);
	Assert(empty.m_Offset == 0);
	Assert(empty.m_Size == 0);
	Assert(empty.m_Link == 0);
	Assert(!empty.m_Used);
	Assert(!empty.m_Touched);

	// Verify all nodes that are in use.
	size_t checkUsed = 0;
	u32 visited = 0;
	for (u32 h=0; h<EXTERNAL_HEAP_WORKSPACE_HASH_SIZE; h++) {
		u32 idx = m_Hash[h];
		while (idx) {
			Node &n = m_Nodes[idx];
			AssertMsg(n.m_Used,"Item in used list isn't marked used.");
			AssertMsg(!n.m_Touched,"Cycle in used list?");
			AssertMsg(n.m_Size,"Zero-length node in used list?");
			checkUsed += n.m_Size;
			n.m_Touched = true;
			idx = n.m_Link;
			++visited;
		}
	}
	Assert(checkUsed == m_MemUsed);

	// Verify all nodes that are marked free.
	u32 idx = m_FirstFreeIdx;
	while (idx) {
		Node &n = m_Nodes[idx];
		AssertMsg(!n.m_Used,"Item in free list isn't marked freed.");
		AssertMsg(!n.m_Touched,"Cycle in free list?");
		AssertMsg(n.m_Size,"Zero-length node in free list?");
		checkUsed += n.m_Size;
		n.m_Touched = true;
		idx = n.m_Link;
		++visited;
	}
	Assert(checkUsed == m_MemTotal);
	
	// Verify all available nodes are valid.
	idx = m_FirstAvailIdx;
	while (idx) {
		Node &n = m_Nodes[idx];
		AssertMsg(!n.m_Used,"Item in avail list isn't marked freed.");
		AssertMsg(!n.m_Touched,"Cycle in avail list?");
		AssertMsg(!n.m_Size,"Non-zero-length node in avail list?");
		n.m_Touched = true;
		idx = n.m_Link;
		++visited;
	}

	// Verify all nodes (except the sentinel) were visited
	Assert(!empty.m_Touched);
	Assert(visited + 1 == m_MaxNodes);

	// Clear all touched bits and find the root node
	u32 rootNodeIdx = ~0U;
	for (u32 i = 1; i < m_MaxNodes; i++) {
		if (m_Nodes[i].m_Size && !m_Nodes[i].m_Offset) {
			AssertMsg(rootNodeIdx == ~0U,"Duplicate root node?");
			rootNodeIdx = i;
		}
		m_Nodes[i].m_Touched = false;
	}
	AssertMsg(rootNodeIdx != ~0U,"Missing root node?");

	// Verify there are no gaps in the address space
	checkUsed = 0;
	while (rootNodeIdx) {
		Node &n = m_Nodes[rootNodeIdx];
		Assert(n.m_Offset == checkUsed);
		checkUsed += n.m_Size;
		Assert(!n.m_PrevAddr || m_Nodes[n.m_PrevAddr].m_NextAddr == rootNodeIdx);
		Assert(!n.m_NextAddr || m_Nodes[n.m_NextAddr].m_PrevAddr == rootNodeIdx);
		// If the current node is unused, either there should be no successor or the next node is used.
		AssertMsg(n.m_Used || !n.m_NextAddr || m_Nodes[n.m_NextAddr].m_Used,"Two free nodes in a row?");
		rootNodeIdx = n.m_NextAddr;
	}
	Assert(checkUsed == m_MemTotal);

	// Phew!  Everything seems okay...
}


size_t sysExternalHeap::GetMemoryEnd() const
{
	u32 i = m_FirstFreeIdx;
	size_t result = m_MemTotal;
	size_t wasted = 0;
	while (i) {
		if (m_Nodes[i].m_Offset + m_Nodes[i].m_Size == m_MemTotal)
			result = m_Nodes[i].m_Offset;
		else
			wasted += m_Nodes[i].m_Size;
		i = m_Nodes[i].m_Link;
	}
	Displayf("GetMemoryEnd - %" SIZETFMT "u used on heap, %" SIZETFMT "u wasted due to gaps",result,wasted);
	return result;
}


void sysExternalHeap::DumpLeaks() const
{
	for (u32 h=0; h<EXTERNAL_HEAP_WORKSPACE_HASH_SIZE; h++) {
		u32 idx = m_Hash[h];
		while (idx) {
			Node &n = m_Nodes[idx];
#if EXTERNAL_HEAP_ALLOC_ID
			Displayf("Alloc id %d.  Offset %u, size %u",n.m_AllocId,n.m_Offset,n.m_Size);
#else
			Displayf("Alloc id ???.  Offset %u, size %u",n.m_Offset,n.m_Size);
#endif
			idx = n.m_Link;
		}
	}
}


size_t sysExternalHeap::GetSize(size_t offset) const
{
	// Locate correct in-use list from toplevel hash.
	size_t hashIdx = OffsetHash(offset);
	u32 nodeIdx = m_Hash[hashIdx];
	// u32 prevNodeIdx = ~0U;
	// Traverse the in-use list looking for the matching node
	while (nodeIdx && m_Nodes[nodeIdx].m_Offset != offset) {
		// prevNodeIdx = nodeIdx;
		nodeIdx = m_Nodes[nodeIdx].m_Link;
	}

	// If we reached the end of the hash chain, it's not a known pointer, which may mean
	// it's the interior pointer of a resource heap.  
	if (!nodeIdx)
		return 0;

	Node &node = m_Nodes[nodeIdx];
	return node.m_Size;
}


size_t sysExternalHeap::GetLargestAvailableBlock()
{
	size_t largest = 0;
	u32 i = m_FirstFreeIdx;
	while (i) {
		if (m_Nodes[i].m_Size > largest)
			largest = m_Nodes[i].m_Size;
		i = m_Nodes[i].m_Link;
	}
	if (m_ChunkSize && largest > m_ChunkSize)
		largest = m_ChunkSize;
	return largest;
}

}	// namespace rage
