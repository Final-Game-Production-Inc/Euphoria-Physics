// 
// system/buddyheap.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "buddyheap.h"

#include "math/amath.h"
#include "system/alloca.h"
#include "system/memory.h"
#include "system/tasklog.h"

#include <string.h>

#if __DEV
#include "file/asset.h"
#endif

namespace rage {

#define SANITY_CHECK	0

// Node lock counts saturate at one less than the largest possible value
#define LOCK_COUNT_SATURATE     ((1<<sysBuddyHeap::NodeInfo::BITS_LOCKCOUNT)-2)

// The largest possible lock count value is reserved for indicating that a
// defrag move is in progress
#define LOCK_COUNT_DEFRAG       ((1<<sysBuddyHeap::NodeInfo::BITS_LOCKCOUNT)-1)

#define spew(x)
/// #define spew(x)	Displayf x

/*
	Any node that doesn't exist (because it lives inside an undivided parent
	node) has height equal to the parent nodes height (this allows finding the
	parent).  Otherwise, its height is valid.  If Next==Prev==c_INVALID, the
	node is in use.  If either is c_INVALID, the other must be as well.  If
	Prev==c_NONE, node is the first free node in the list (use Height to figure
	out where the list head pointer lives).  If Next==c_NONE, node is the last
	free node on the list.

	Tree can have a maximum of 16382 nodes in it based on current index sizes.

	Debug dumps require this tool:
	http://www.graphviz.org/Download_windows.php
*/

#if ENABLE_DEFRAG_CALLBACK
IsObjectReadyToDelete sysBuddyHeap::s_isObjectReadyToDelete(NULL);
#endif

#if RESOURCE_POOLS
void sysBuddyPool::Init(void* heapBase, size_t leafSize, void* ptr, size_t capacity, size_t size)
{
	Assert(heapBase && ptr && capacity && heapBase <= ptr);
	
	m_heapBase = heapBase;	
	m_leafSize = leafSize;
	m_ptr = ptr;
	m_size = size;

	const size_t items = capacity / size;
	m_pool.Init(items);

	const size_t offset = static_cast<u8*>(ptr) - static_cast<u8*>(heapBase);
	m_baseNode = static_cast<sysBuddyNodeIdx>(offset / leafSize);
	m_endNode = m_baseNode + (static_cast<u16>(items * (size / leafSize)));
}

sysBuddyNodeIdx sysBuddyPool::Allocate()
{
	void* poolPtr = m_pool.New();
	const size_t pos = m_pool.GetIndex(poolPtr);
	const size_t offset = (static_cast<u8*>(m_ptr) + (pos * m_size)) - static_cast<u8*>(m_heapBase);
	return static_cast<sysBuddyNodeIdx>(offset / m_leafSize);
}

void sysBuddyPool::Free(sysBuddyNodeIdx addr)
{
	Assert(IsValidAddr(addr));

	const size_t pos = (addr - m_baseNode) / (m_size / m_leafSize);
	void* ptr = m_pool.GetElemByIndex(pos);
	m_pool.Delete(ptr);
}
#endif

void sysBuddyHeap::Init(size_t leafCount,void *nodes)
{
	// Clear all the arrays
	FatalAssert(leafCount && leafCount <= c_INVALID);
	m_LeafCount = leafCount;
	m_LeavesUsed = 0;
	m_Nodes = (NodeInfo*) nodes;
	for (sysBuddyNodeIdx i=0; i<leafCount; i++)
	{
		SetNodePrev(i,c_INVALID);
		SetNodeNext(i,c_INVALID);
		SetNodeHeight(i,c_MAX_HEIGHT);
		SetNodeLockCount(i,0);
		SetUserData(i, (u32)sysMemAllocator::INVALID_USER_DATA);
	}
	for (size_t i=0; i<c_MAX_HEIGHT; i++)
	{
		m_FirstFreeBySize[i] = c_NONE;
		m_UsedBySize[i] = m_FreeBySize[i] = 0;
	}
	for (int i=0; i<16; i++)
		m_LeavesUsedByBucket[i] = 0;

	// Construct the initial tree
	size_t rover = 0;
	for (unsigned level = c_MAX_HEIGHT-1, mask = 1<<level; mask; level--, mask>>=1) 
	{
		if (leafCount & mask)
		{
			FatalAssert(rover < leafCount);
			AddToFreeList((sysBuddyNodeIdx)rover,level);
			rover += mask;
		}
	}
	FatalAssert(rover == leafCount);
#if SANITY_CHECK
	SanityCheck();
#endif
}

#if RESOURCE_POOLS
// HACK: Adjust the internal tally
void sysBuddyHeap::HideUsedMemory(sysBuddyNodeIdx addr)
{
	unsigned level = GetNodeHeight(addr);
	m_UsedBySize[level]--;

	size_t numNodes = (size_t)1<<level;	
	m_LeavesUsed -= numNodes;
	m_LeavesUsedByBucket[GetNodeBucket(addr)] -= numNodes;
}

void sysBuddyHeap::InitPool(int pos, void* heapBase, size_t leafSize, void* ptr, size_t capacity, size_t size)
{
	Assert(pos >= 0 && pos < NUM_POOLS && ptr && capacity && size);

	m_pool[pos].Init(heapBase, leafSize, ptr, capacity, size);

	const int level = (m_pool[pos].GetStorageSize() / m_pool[pos].GetLeafSize()) - 1;
	for (sysBuddyNodeIdx node = m_pool[pos].GetBaseNode(); node < m_pool[pos].GetEndNode(); ++node)
	{
		SetUserData(node, (u32) sysMemAllocator::INVALID_USER_DATA);
		SetNodeHeight(node, level);
	}	
}

sysBuddyNodeIdx sysBuddyHeap::AllocatePoolNode(unsigned level)
{
	Assert(level < NUM_POOLS && !m_pool[level].IsFull());
	
	sysBuddyNodeIdx result = m_pool[level].Allocate();
	SetNodeHeight(result, level);
	FatalAssertf(GetUserData(result) == (u32) sysMemAllocator::INVALID_USER_DATA, "Freshly allocated buddy node %d is still in use with user data %x", result, GetUserData(result));
	return result;	
}
#endif

static	inline	bool	isPow2(size_t x) { return (x & (x - 1)) == 0; }

void sysBuddyHeap::AddToFreeList(sysBuddyNodeIdx node,unsigned level)
{
#if RESOURCE_POOLS
	// Do not defrag pool nodes (but they shouldn't be here in the first place) 
	if (!AssertVerify(level >= NUM_POOLS || !m_pool[level].IsValidAddr(node))) 
	{
		Displayf("OMG! This should never happen! level: %u, size: %u, ptr: %u", level, (1 << level), node);
		return;
	}
#endif

	FatalAssert(GetNodePrev(node)==c_INVALID && GetNodeNext(node) == c_INVALID);
	SetNodeHeight(node,level);
	SetNodePrev(node,c_NONE);
	// Our successor is previous head of this free list
	SetNodeNext(node,m_FirstFreeBySize[level]);
	// If successor exists, point its predecessor at us
	if (m_FirstFreeBySize[level] != c_NONE)
		SetNodePrev(m_FirstFreeBySize[level],node);
	// Point head of free list at us
	m_FirstFreeBySize[level] = node;

	++m_FreeBySize[level];

	spew(("add(%x,%x)",node,level));
}

void sysBuddyHeap::RemoveFromFreeList(sysBuddyNodeIdx node,unsigned level)
{
	FatalAssert(GetNodeHeight(node) == level);
	FatalAssert(GetNodePrev(node) != c_INVALID && GetNodeNext(node) != c_INVALID);

	// If we have a predecessor, point its successor at our successor
	if (GetNodePrev(node) != c_NONE)
		SetNodeNext(GetNodePrev(node),GetNodeNext(node));
	// Otherwise point head of free list at our successor
	else
		m_FirstFreeBySize[level] = GetNodeNext(node);
	// If we have a successor, point its predecessor at our predecessor
	if (GetNodeNext(node) != c_NONE)
		SetNodePrev(GetNodeNext(node),GetNodePrev(node));
	// Invalidate our links for consistency
	SetNodePrev(node,c_INVALID);
	SetNodeNext(node,c_INVALID);

	--m_FreeBySize[level];

	spew(("remove(%x,%x)",node,level));
}


sysBuddyNodeIdx sysBuddyHeap::AllocateRecurse(unsigned level)
{
	if (level == c_MAX_HEIGHT)
		return c_NONE;
	else if (m_FirstFreeBySize[level] != c_NONE)
	{
		// We will often have several suitable nodes.  We want to pick the node that
		// will reduce fragmentation in the future.  An approximation of fragmentation
		// is how heavily subdivided our sibling node is.  If our sibling isn't subdivided
		// at all, it's far more likely that they can be recycled sooner.
		// If we're at the bottom of the tree, any node is as good as any other.
		sysBuddyNodeIdx result = m_FirstFreeBySize[level];
		RemoveFromFreeList(result,level);
		return result;
	}
	else 
	{
		// Find a block in the next level up
		sysBuddyNodeIdx parent = AllocateRecurse(level+1);
		if (parent != c_NONE)
		{
			FatalAssert(parent != c_INVALID);
			// Add buddy block to free list (which by definition must be empty)
			sysBuddyNodeIdx buddy = (sysBuddyNodeIdx) (parent ^ (1<<level));
			AddToFreeList(buddy,level);
			// Fix parent node's height since we're subdividing it.
			FatalAssert(GetNodeHeight(parent) == level+1);
			SetNodeHeight(parent,level);
			spew(("subdivide(%x)->%x",parent,GetNodeHeight(parent)));
		}

		return parent;
	}
}

sysBuddyNodeIdx sysBuddyHeap::Allocate(size_t leafCount) 
{
	FatalAssert(leafCount);
	unsigned level = Log2Floor(leafCount) + !isPow2(leafCount);
	sysBuddyNodeIdx result;

#if RESOURCE_POOLS
	if (level < NUM_POOLS && !m_pool[level].IsFull())
	{
		result = AllocatePoolNode(level);
	}
	else
#endif
	{
		result = AllocateRecurse(level);
	}

	if (result != c_NONE)
	{
		const size_t rndUpLeafCount = ((size_t)1<<level);
		FatalAssert(rndUpLeafCount >= leafCount);
		m_UsedBySize[level]++;
		m_LeavesUsed += rndUpLeafCount;
		FatalAssertf(GetUserData(result) == (u32) sysMemAllocator::INVALID_USER_DATA, "Freshly allocated buddy node %d is still in use with user data %x", result, GetUserData(result));

		SetUserData(result, (u32)sysMemAllocator::INVALID_USER_DATA);
		SetNodeLockCount(result,1);
		SetNodeBucket(result,sysMemCurrentMemoryBucket);
		m_LeavesUsedByBucket[sysMemCurrentMemoryBucket & 15] += rndUpLeafCount;

		// AllocateRecurse will ensure the first node of the allocation has the
		// correct height set, but for GetFirstNode() to work correctly, we need
		// to set the rest as well.  Notice that we only really need to bother
		// setting the nodes requested by the caller, not rounding up this count
		// to the next power of 2, but this will break SanityCheck().
		FatalAssert(GetNodeHeight(result) == level);
		for (size_t i=1; i<rndUpLeafCount; ++i)
			SetNodeHeight(result+i,level);
	}
	
#if SANITY_CHECK
	SanityCheck();
#endif
	spew(("allocate(%x)->%x",leafCount,result));
	
	return result;
}


sysBuddyNodeIdx sysBuddyHeap::GetFirstNode(sysBuddyNodeIdx addr) const
{
	const unsigned height = GetNodeHeight(addr);
	FatalAssert(height<c_MAX_HEIGHT); // Note < not <=, c_MAX_HEIGHT not valid in this case
	return addr&~((1<<height)-1);
}


void sysBuddyHeap::Free(sysBuddyNodeIdx addr)
{
	FatalAssert(GetNodeHeight(addr) != c_MAX_HEIGHT && GetNodeNext(addr) == c_INVALID);
	FatalAssertMsg((addr&((1<<GetNodeHeight(addr))-1)) == 0,"Free called on node that is not the first node of an allocation");
	SetUserData(addr,(u32)sysMemAllocator::INVALID_USER_DATA);

	unsigned level = GetNodeHeight(addr);
	m_UsedBySize[level]--;
	size_t numNodes = (size_t)1<<level;
	m_LeavesUsed -= numNodes;
	m_LeavesUsedByBucket[GetNodeBucket(addr)] -= numNodes;

	spew(("free(%x)",addr));

	SetNodeLockCount(addr, 0);

#if RESOURCE_POOLS
	if ((level < NUM_POOLS) && (m_pool[level].IsValidAddr(addr)))
	{
		m_pool[level].Free(addr);
	}
	else
#endif
	{
		// Must restore the internal (ie, not the first) nodes' height to c_MAX_HEIGHT before they are freed
		for (size_t i=1; i<numNodes; ++i)
		{
			FatalAssert(GetNodeHeight(addr+i) == level);
			SetNodeHeight(addr+i, c_MAX_HEIGHT);
		}

		for (;;)
		{
			FatalAssert(level != c_MAX_HEIGHT);

			sysBuddyNodeIdx buddy = (sysBuddyNodeIdx)(addr ^ (1 << level));
			if (buddy >= m_LeafCount || GetNodeNext(buddy) == c_INVALID || GetNodeHeight(buddy) != level) 
			{
				// our buddy isn't free, or is wrong size, so just shove ourselves on current free list
				AddToFreeList(addr, level);
				break;
			}
			else 
			{
				RemoveFromFreeList(buddy, level);

				// fix up the rightmost node and traverse the leftmost node
				if (buddy < addr)
				{
					sysBuddyNodeIdx temp = buddy;
					buddy = addr;
					addr = temp;
				}
				SetNodeHeight(buddy,c_MAX_HEIGHT);
				SetNodePrev(buddy,c_INVALID);
				SetNodeNext(buddy,c_INVALID);
				++level;
				spew(("invalidate(%x)",buddy));
			}
		}
	}
	
#if SANITY_CHECK
	SanityCheck();
#endif
}

#if ENABLE_DEFRAG_CALLBACK
bool sysBuddyHeap::Defragment(sysBuddyHeapDefragInfo& outInfo, sysMemDefragmentationFree& outDefragFree, void* heapBase, size_t nodeSize, unsigned maxHeight)
#else
bool sysBuddyHeap::Defragment(sysBuddyHeapDefragInfo& outInfo, sysMemDefragmentationFree& UNUSED_PARAM(outDefragFree), void* TASKLOG_ONLY(heapBase), size_t TASKLOG_ONLY(nodeSize), unsigned maxHeight)
#endif
{
	memset(&outInfo,0,sizeof(outInfo));

#if ENABLE_DEFRAG_CALLBACK
	Assertf(s_isObjectReadyToDelete, "Defragmentation callback pointer function has not been set!");
	memset(&outDefragFree,0,sizeof(outDefragFree));
#endif

	for (unsigned level=maxHeight-1; (int) level>=0; level--) 
	{
		sysBuddyNodeIdx buddyBit = (sysBuddyNodeIdx)(1<<level);
		sysBuddyNodeIdx from = c_NONE;

		// Locate a source for defragmentation
		sysBuddyNodeIdx fromBuddy;
		for (fromBuddy = m_FirstFreeBySize[level]; fromBuddy != c_NONE; fromBuddy = GetNodeNext(fromBuddy))
		{
			// Source for the move is the buddy of this free node
			from = fromBuddy ^ buddyBit;

			// If the source isn't valid because it's the buddy of a rightmost node (which may not have a right buddy)
			// then skip it.
			if (size_t(from + buddyBit) > m_LeafCount)
				continue;

#if ENABLE_DEFRAG_CALLBACK
			// Verify this isn't locked
			if (GetNodeLockCount(from))
				continue;

			void* ptr = (char*) heapBase + (from * nodeSize);
			if (s_isObjectReadyToDelete(ptr))
			{
				if (outDefragFree.Count < sysMemDefragmentationFree::c_MAX_TO_FREE)
				{
					if (outDefragFree.Count > 0 && outDefragFree.Nodes[outDefragFree.Count - 1] == ptr)
						continue;

					DefragInflightLockBlock(from);
					outDefragFree.Nodes[outDefragFree.Count] = ptr;
					outDefragFree.Count++;
				}

				continue;
			}
#endif
			// Search all of the subnodes in the from tree; every node must either be free or unlocked.
			sysBuddyNodeIdx offset = 0;
			int toMove = 0;
			while (offset < buddyBit)
			{
				// Figure out size of this node
				u32 curLevel = GetNodeHeight(from+offset);
				u32 curSize = 1<<curLevel;
				// Any locked node means we cannot defragment this.
				if (GetNodeLockCount(from+offset))
					break;

#if RESOURCE_POOLS
				// Do not defrag pool nodes (but they shouldn't be here in the first place) 
				if (!AssertVerify(curLevel >= NUM_POOLS || !m_pool[curLevel].IsValidAddr(from+offset))) 
				{
					Displayf("OMG! This should never happen! level: %u, size: %u, ptr: %u", curLevel, curSize, from+offset);
					break;
				}
#endif
				// Make sure we don't have more nodes here than we can represent at once.
				if (GetNodeNext(from+offset) == c_INVALID) 
				{
					++toMove;
					if (toMove > sysBuddyHeapDefragInfo::c_MAX_TO_MOVE)
						break;
				}
				offset = (sysBuddyNodeIdx)(offset + curSize);
			}
			FatalAssert(offset <= buddyBit);

			// If all nodes are unlocked, this is suitable as a source for defragmentation.
			if (offset == buddyBit)
				break;
		}

		// If we didn't find any suitable source for defragmentation, try the next level up.
		if (fromBuddy == c_NONE)
			continue;

		// Find a destination node, making sure it's not the buddy of the source node we just picked!
		sysBuddyNodeIdx to = m_FirstFreeBySize[level];
		while (to != c_NONE && ((to ^ buddyBit) == from))
			to = GetNodeNext(to);

		// If we didn't find a suitable destination (really only possible if only one node on free list), try next level up.
		if (to == c_NONE)
			continue;


		//         /  \                   /     \             //
		//        /    \                 /       \            //
		//       to   to^buddyBit      from   from^buddyBit   //
		//      FREE   USED            USED      FREE         //
		//
		// to and from^buddyBit are taken directly from the free list.
		// By definition they should not be direct siblings.
		// Note that either to or from may be a right node and not a left
		// node like depicted above.

		// Check that to is freed.  From might actually not be in use, but we know
		// one of its siblings may be, and handle that in the offset loop below.
		FatalAssertf(GetNodeHeight(to) != c_MAX_HEIGHT && GetNodeNext(to) != c_INVALID,"Node %x isn't free!",to);

		// Destination (to) is no longer on a free list.  It is now used.
		RemoveFromFreeList(to,level);

		// Note that we no longer free memory here.  We allocate space in the destination for
		// the copy though, and leave it to the client to either perform the move and free the original,
		// or just free the destination.  In either case the normal Free logic does any collapse
		// operations necessary.
		sysBuddyNodeIdx offset = 0;
		while (offset < buddyBit)
		{
			// Figure out size of this node
			u32 curLevel = GetNodeHeight(from+offset);
			u32 curSize = 1<<curLevel;
			// Schedule a move it if it's used, or reassign it if it's free
			if (GetNodeNext(from+offset) == c_INVALID)
			{
				FatalAssert(outInfo.Count < sysBuddyHeapDefragInfo::c_MAX_TO_MOVE);
				Assign(outInfo.Nodes[outInfo.Count].from,from+offset);
				Assign(outInfo.Nodes[outInfo.Count].to,to+offset);
				Assign(outInfo.Nodes[outInfo.Count].curSize,curSize);
				TASKLOG_ASSERTF(GetUserData(outInfo.Nodes[outInfo.Count].from) != sysMemAllocator::INVALID_USER_DATA,"Defrag candidate from block %p not owned?",((char*)heapBase + nodeSize*(from+offset)));
				outInfo.Count++;
				for (unsigned i=0; i<curSize; ++i)
					SetNodeHeight(to+offset+i,curLevel);
				SetNodeBucket(to+offset, GetNodeBucket(from+offset));
				m_LeavesUsed += curSize;
				m_UsedBySize[curLevel]++;
				m_LeavesUsedByBucket[GetNodeBucket(to+offset)] += curSize;
				FatalAssert(GetNodeLockCount(from+offset) == 0);
				FatalAssert(GetNodeLockCount(to+offset) == 0);
				DefragInflightLockBlock(from+offset);
				DefragInflightLockBlock(to+offset);
				SetUserData(to+offset,GetUserData(from+offset));
				TASKLOG("Defrag candidate from block %p to block %p",(size_t)((char*)heapBase + nodeSize*(from+offset)),(size_t)((char*)heapBase + nodeSize*(to+offset)));
			}
			else
			{
				AddToFreeList(to+offset,curLevel);
				TASKLOG("Defrag candidate from block %p already free, re-adding %p to free list",(size_t)((char*)heapBase + nodeSize*(from+offset)),(size_t)((char*)heapBase + nodeSize*(to+offset)));
			}
			offset = (sysBuddyNodeIdx)(offset + curSize);
		}

		FatalAssert(offset == buddyBit);
		// Make sure we really generated some work here.  If all sub blocks were free,
		// they should have already been coalesced, and if any sub blocks were locked,
		// we should have eliminated them during first wave of tests.
		FatalAssert(outInfo.Count);

		//         /  \                   /     \              //
		//        /    \                 /       \             //
		//       to   to^buddyBit      from   from^buddyBit    //
		//      USED    USED          INVALID   INVALID        //

#if SANITY_CHECK
		SanityCheck();
#endif
		return true;
	}
	return false;
}


size_t sysBuddyHeap::GetSize(sysBuddyNodeIdx addr) const
{
	// Invalid node index ?
	if (addr >= m_LeafCount)
		return 0;

	// Invalid node height ?
	unsigned height = GetNodeHeight(addr);
	if (height >= c_MAX_HEIGHT)
	{
		FatalAssert(height == c_MAX_HEIGHT);
		return 0;
	}

	// Not the first node in allocation ?
	unsigned numNodes = 1<<height;
	if (addr & (numNodes-1))
		return 0;

	// In free list ?
	if (GetNodeNext(addr) != c_INVALID)
		return 0;

	return numNodes;
}


void sysBuddyHeap::GetMemoryDistribution(sysMemDistribution &outDist) const
{
	for (size_t i=0; i<32; i++)
	{
		outDist.UsedBySize[i] = i<c_MAX_HEIGHT? m_UsedBySize[i] : 0;
		outDist.FreeBySize[i] = i<c_MAX_HEIGHT? m_FreeBySize[i] : 0;
	}
#if 0
	for (i=0; i<m_LeafCount;) 
	{
		size_t thisHeight = GetNodeHeight(i);
		if (GetNodeNext(i) == c_INVALID)
			outDist.UsedBySize[thisHeight]++;
		else
			outDist.FreeBySize[thisHeight]++;
		i += (1 << thisHeight);
	}
	for (i=0; i<c_MAX_HEIGHT; i++)
	{
		FatalAssert(outDist.UsedBySize[i] == m_UsedBySize[i]);
		FatalAssert(outDist.FreeBySize[i] == m_FreeBySize[i]);
	}
#endif
}


void sysBuddyHeap::SanityCheck() const
{
#if __ASSERT

	// Pools
#if RESOURCE_POOLS
	for (size_t i = 0; i < GetNumPools(); ++i)
	{
		const sysBuddyPool& pool = m_pool[i];
		const size_t level = (pool.GetStorageSize() / pool.GetLeafSize()) - 1;

		for (sysBuddyNodeIdx node = pool.GetBaseNode(); node < pool.GetEndNode(); ++node)
		{
			if (GetUserData(node) != (u32) sysMemAllocator::INVALID_USER_DATA)
			{
				void* ptr = (char*) pool.GetHeapBase() + (node * pool.GetSize());
				Assertf(GetNodeHeight(node) == level, "Invalid Pool Node Height: %p, idx=%d, level=%d, height=%d", ptr, node, level, GetNodeHeight(node));
			}			
		}
	}
#endif
	
	// Every free node must be on exactly one free list.
	// There must never be an invalid node or used node on any free list.
	// Any node not on any free list must be either in use or invalid.
	// Every node that is valid (either free or in use) that is larger than one leaf in size
	// must have all of its contained leaves marked invalid.
	size_t byteCount = (m_LeafCount + 7) >> 3;
	u8 *used = Alloca(u8, byteCount);
	while (byteCount)
		used[--byteCount] = 0;;
	size_t usedByBucket[16];
	for (int i=0; i<16; i++)
		usedByBucket[i] = 0;

#define IS_VISITED(i)	(used[(i)>>3] & (1 << ((i)&7)))
#define SET_VISITED(i)	(used[(i)>>3] |= (1 << ((i)&7)))

	// Verify free list integrity
	for (size_t i=0; i<c_MAX_HEIGHT; i++)
	{
		sysBuddyNodeIdx rover = m_FirstFreeBySize[i];
		// u32 buddyBit = 1<<i;
		sysBuddyNodeIdx prev = c_NONE;
		while (rover != c_NONE)
		{
			FatalAssertf(GetNodePrev(rover) == prev,"Free node in free list %" SIZETFMT "u doesn't point to its predecessor.",i);
			prev = rover;
			
			// All nodes must actually be free
			FatalAssertf(rover < m_LeafCount,"Link in free list %" SIZETFMT "u invalid: %x",i,rover);
			// All nodes cannot have already been used
			FatalAssertf(!IS_VISITED(rover),"Item in free list %" SIZETFMT "u was already seen elsewhere: %x",i,rover);
			FatalAssertf(GetNodeHeight(rover) == i,"Item in free list %" SIZETFMT "u at wrong height: %d",i,GetNodeHeight(rover));
			SET_VISITED(rover);
			// Make sure the internal nodes are all marked invalid and didn't already get used somehow.
			sysBuddyNodeIdx size = (sysBuddyNodeIdx)(1 << i);
			FatalAssert((size_t)(rover + size) <= m_LeafCount);
			while (--size)
			{
				FatalAssertf(!IS_VISITED(rover + size),"Free node %x contains a leaf already marked used.",rover);
				SET_VISITED(rover + size);
				FatalAssertf(GetNodeHeight(rover + size) == c_MAX_HEIGHT,"Free node %x has an internal leaf %x not marked invalid: %x",rover,rover+size,GetNodeHeight(rover+size));
			}
			rover = GetNodeNext(rover);
		}
	}

	// Second pass, make sure any node on a free list has a buddy NOT on same free list.
	for (size_t i=0; i<c_MAX_HEIGHT; i++)
	{
		size_t rover = m_FirstFreeBySize[i];
		u32 buddyBit = 1<<i;

		while (rover != c_NONE)
		{
			size_t buddy = rover ^ buddyBit;
			FatalAssertf(buddy >= m_LeafCount || !IS_VISITED(buddy) || GetNodeHeight(rover) != GetNodeHeight(buddy),"Item in free list %" SIZETFMT "u has buddy on same list! (%" SIZETFMT "x and %" SIZETFMT "x)",i,rover,buddy);
			rover = GetNodeNext(rover);
		}
	}

	// Now any node not already marked used must be in use (and be followed by an appropriate number of invalid leaves)
	size_t i, leavesUsed = 0;
	for (i=0; i<m_LeafCount; i++)
	{
		if (!IS_VISITED(i))
		{
			size_t base = i;
			size_t height = GetNodeHeight((sysBuddyNodeIdx)i);
			size_t size = GetSize((sysBuddyNodeIdx)i);
			FatalAssertf(size,"Node %" SIZETFMT "x isn't in use but wasn't on any free list.",i);
			if (size)
			{
				leavesUsed += size;
				usedByBucket[GetNodeBucket((sysBuddyNodeIdx)i)] += size;
				while (--size)
				{
					++i;
					FatalAssertf(!IS_VISITED(i),"Used node based at %" SIZETFMT "x contains leaf %" SIZETFMT "x already marked used elsewhere.",base,i);
					FatalAssertf(GetNodeNext(i) == c_INVALID,"Used node %" SIZETFMT "u not marked invalid.",i);
					FatalAssertf(GetNodeHeight(i) == height,"Internal node %" SIZETFMT "u height of %d does not match parent node %" SIZETFMT "u's height of %" SIZETFMT "u.",i,GetNodeHeight(i),base,height);
				}
			}
		}
	}
	FatalAssert(i == m_LeafCount);

	FatalAssertf(m_LeavesUsed == leavesUsed,"Expected leaves in use %" SIZETFMT "d didn't match actual %" SIZETFMT "d",m_LeavesUsed,leavesUsed);

	for (i=0; i<16; i++)
		FatalAssertf(usedByBucket[i] == m_LeavesUsedByBucket[i],"Expected leaves in use %" SIZETFMT "d in bucket %" SIZETFMT "d didn't match computed value %" SIZETFMT "d",m_LeavesUsedByBucket[i],i,usedByBucket[i]);

	size_t usedBySize = 0, freeBySize = 0;
	for (i=0; i<c_MAX_HEIGHT; i++)
	{
		usedBySize += m_UsedBySize[i] * (1 << i);
		freeBySize += m_FreeBySize[i] * (1 << i);
	}
	FatalAssertf(m_LeavesUsed == usedBySize,"Expected leaves in use %" SIZETFMT "d didn't match amount computed from m_UsedBySize, %" SIZETFMT "d",m_LeavesUsed,usedBySize);
	FatalAssertf(m_LeafCount - m_LeavesUsed == freeBySize,"Expected leaves freed %" SIZETFMT "d didn't match amount computed from m_UsedBySize, %" SIZETFMT "d",m_LeafCount - m_LeavesUsed,freeBySize);

	size_t totalUsedByBucket = 0;
	for (i=0; i<16; i++)
		totalUsedByBucket += m_LeavesUsedByBucket[i];
	FatalAssertf(m_LeavesUsed == totalUsedByBucket,"Expected leaves in use %" SIZETFMT "d didn't match amount computed from m_LeavesUsedByBucket, %" SIZETFMT "d",m_LeavesUsed,totalUsedByBucket);

#undef SET_VISITED
#undef IS_VISITED
#endif
}

size_t sysBuddyHeap::GetLargestAvailableBlock() const
{
	for (int i=c_MAX_HEIGHT-1; i>=0; i--)
		if (m_FirstFreeBySize[i] != c_NONE)
			return (size_t)1 << i;
	return 0;
}


size_t sysBuddyHeap::GetMemoryFree() const
{
	return m_LeafCount - m_LeavesUsed;
}


size_t sysBuddyHeap::GetMemoryUsed(int bucket) const
{
	if (bucket >= 0 && bucket < 16)
		return m_LeavesUsedByBucket[bucket];
	else
		return m_LeavesUsed;
}


bool sysBuddyHeap::TryLockBlock(sysBuddyNodeIdx addr, unsigned lockCount)
{
	unsigned count = GetNodeLockCount(addr);

	// Currently in use by an inflight defrag?
	if (Unlikely(count == LOCK_COUNT_DEFRAG))
		return false;

#if !__NO_OUTPUT
	// This warning should be ultra-rare, if not, then we'll need to steal more
	// bits for the counter from somewhere.
	if (Unlikely(count + lockCount >= LOCK_COUNT_SATURATE))
		Warningf("sysBuddyHeap::LockBlock saturating lock count, will not be able to defrag this allocation any more");
#endif

	count = (count+lockCount<LOCK_COUNT_SATURATE) ? count+lockCount : LOCK_COUNT_SATURATE;
	SetNodeLockCount(addr,count);
	return true;
}


void sysBuddyHeap::DefragInflightLockBlock(sysBuddyNodeIdx addr)
{
	FatalAssert(GetNodeLockCount(addr)==0);
	SetNodeLockCount(addr,LOCK_COUNT_DEFRAG);
}


void sysBuddyHeap::UnlockBlock(sysBuddyNodeIdx addr, unsigned unlockCount)
{
	unsigned count = GetNodeLockCount(addr);
	FatalAssert(count >= unlockCount);
	// Unlock after defrag ?
	if (count == LOCK_COUNT_DEFRAG)
		count = 0;
	// Unlock after TryLockBlock ?
	// Only decrement count if not saturated
	else if (count<LOCK_COUNT_SATURATE)
		count -= unlockCount;
	SetNodeLockCount(addr,count);
}


#if __DEV
bool sysBuddyHeap::DumpTreeToDotFile(const char *filename)
{
	fiStream *S = ASSET.Create(filename,"dot");
	if (!S)
		return false;
	sysBuddyNodeIdx countsByHeight[c_MAX_HEIGHT+1];
	sysBuddyNodeIdx groupsByHeight[c_MAX_HEIGHT+1][256];
	for (unsigned i=0; i<=c_MAX_HEIGHT; i++)
		countsByHeight[i] = 0;

	fprintf(S,"digraph g {\r\n");
	unsigned targetHeight = c_MAX_HEIGHT;
	while (targetHeight--)
	{
		fprintf(S,"height_%d%s",targetHeight,targetHeight?" -> ":";\r\n");
	}
	for (targetHeight=1; targetHeight < c_MAX_HEIGHT; targetHeight++)
	{
		size_t i=0;
		while (i < m_LeafCount)
		{
			size_t thisSize = 1<<GetNodeHeight(i);
			if (GetNodeHeight(i) == targetHeight-1)
			{
				size_t group = i & ~(thisSize);
				sysBuddyNodeIdx j;
				for (j=0; j<countsByHeight[targetHeight]; j++)
					if (groupsByHeight[targetHeight][j] == group)
						break;
				if (j == countsByHeight[targetHeight])
				{
					FatalAssert(j<256);
					groupsByHeight[targetHeight][countsByHeight[targetHeight]++] = (sysBuddyNodeIdx)group;
					// Remove the group from any lower levels
					for (int k=targetHeight-1; k>=0; k--)
					{
						for (sysBuddyNodeIdx j=0; j<countsByHeight[k]; j++)
							if (groupsByHeight[k][j] == group)
							{
								while (j < countsByHeight[k]-1)
								{
									groupsByHeight[k][j] = groupsByHeight[k][j+1];
									j++;
								}
								countsByHeight[k]--;
								break;
							}
					}
				}
			}
			i += thisSize;
		}
	}
	targetHeight = c_MAX_HEIGHT;
	while (targetHeight--)
	{
		size_t i=0;
		fprintf(S,"{ rank = same; height_%d; ",targetHeight);
		for (sysBuddyNodeIdx j=0; j<countsByHeight[targetHeight]; j++)
			fprintf(S,"group_%x; ",groupsByHeight[targetHeight][j]);
		while (i < m_LeafCount)
		{
			size_t thisSize = 1<<GetNodeHeight(i);
			if (GetNodeHeight(i) == targetHeight)
			{
				if (GetNodeNext(i) == c_INVALID)
					fprintf(S,"node_%x [label=\"%x used\"]; ",i,i);
				else
					fprintf(S,"node_%x [label=\"%x free\"]; ",i,i);
			}
			i += thisSize;
		}
		fprintf(S,"}\r\n\r\n");
	}
	for (targetHeight=1; targetHeight < c_MAX_HEIGHT; targetHeight++)
	{
		size_t i=0;
		while (i < m_LeafCount)
		{
			size_t thisSize = 1<<GetNodeHeight(i);
			if (GetNodeHeight(i) == targetHeight-1)
			{
				size_t group = i & ~(thisSize);
				fprintf(S,"node_%x -> group_%x;\r\n",i,group);
			}
			i += thisSize;
		}
	}
	fprintf(S,"}\r\n");
	S->Close();
	return true;
}
#endif

#undef LOCK_COUNT_SATURATE
#undef LOCK_COUNT_DEFRAG

} // namespace rage
