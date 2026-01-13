// GTA is still using VS2008 for ragebuilder, etc.  Rather than attempt to
// support this ancient compiler (which has very poor C++11 typed enum support),
// just #ifdef out this entire file instead.
#if !defined(_MSC_VER) || (_MSC_VER > 1600 && !RSG_TOOL)

#include "tinybuddyheap.h"
#include "memops.h"
#include "nelem.h"

#include "diag/trap.h"
#include "math/amath.h"		// for Log2Floor

namespace rage {

#if !__OPTIMIZED
#define SanityCheck()
#endif

void sysTinyBuddyHeap::Init(u32 nodeCount,sysTinyBuddyHeap::Node *nodeStorage) {
	// Can't use assert here, too early in startup
	TrapGE(nodeCount,TERMINATOR);
	m_Nodes = nodeStorage;
	m_NodeCount = nodeCount;
	m_Used = 0;
	// Reset memory to known state (don't assume it's all zero)
	sysMemSet(nodeStorage, 0, nodeCount*sizeof(Node));
	for (u32 i=0; i<nodeCount; i++) {
		m_Nodes[i].SetNextFree(TERMINATOR);
		m_Nodes[i].SetPrevFree(TERMINATOR);
		m_Nodes[i].SetMemType(MEMTYPE_DEFAULT);
	}
	// Build the initial tree
	u32 rover = 0;
	for (u32 height=NELEM(m_FreeListByHeight); height-->0;) {
		for (u32 memtype=0; memtype<NUM_MEMTYPES; memtype++)
			m_FreeListByHeight[height][memtype] = TERMINATOR;
		if (nodeCount & (1 << height)) {
			m_FreeListByHeight[height][MEMTYPE_DEFAULT] = rover;
			m_Nodes[rover].SetHeight(height);
			rover += (1 << height);
		}
		else
			m_FreeListByHeight[height][MEMTYPE_DEFAULT] = TERMINATOR;
	}
	FatalAssert(rover == nodeCount);

	SanityCheck();
}

u32 sysTinyBuddyHeap::AllocateRecurse(u32 height,u32 memtypeMask) {
	FatalAssert(m_Nodes);
	if (height > Node::MAX_HEIGHT)
		return TERMINATOR;

	// Try to find an allocation of the correct size and of any of the allowed memtypes
	u32 mask = memtypeMask;
	while (mask) {
		u32 lsb = mask&-(s32)mask;
		mask -= lsb;
		u32 memtype = Log2Floor(lsb);
		if (m_FreeListByHeight[height][memtype] != TERMINATOR) {
			u32 result = m_FreeListByHeight[height][memtype];
			FatalAssert(m_Nodes[result].GetHeight() == height);
			// Update free list
			u32 nextFree = m_Nodes[result].GetNextFree();
			m_FreeListByHeight[height][memtype] = nextFree;
			// Update prev pointer of new head of free list if list isn't empty
			if (nextFree != TERMINATOR)
				m_Nodes[nextFree].SetPrevFree(TERMINATOR);
			m_Nodes[result].SetIsUsed(1);
			m_Nodes[result].SetUserData(0);
			m_Nodes[result].SetPhysicalMapping(TERMINATOR);
			m_Used += (1 << height);
			return result;
		}
	}

	// Try to allocate something twice as big, and subdivide it
	u32 parent = AllocateRecurse(height+1,memtypeMask);
	if (parent == TERMINATOR)
		return TERMINATOR;
	u32 buddyBit = 1 << height;
	// Our newly allocated parent node has already been tallied into m_Used, and we're splitting
	// it in half so return the difference.
	m_Used -= buddyBit;
	FatalAssert(m_Nodes[parent].GetHeight() == height+1);
	ASSERT_ONLY(m_Nodes[parent].SetHeight(height);)     // Allocate will set this again later, but needed for the assert above
	m_Nodes[parent ^ buddyBit].SetHeight(height);
	AddToFreeList(parent ^ buddyBit);
	return parent;
}

u32 sysTinyBuddyHeap::Allocate(u32 count,u32 preferredMemtype) {
	FatalAssert((count & (count-1)) == 0);	// count should be a power of two.
	u32 memtypeMask = 1<<preferredMemtype;
	u32 height = Log2Floor(count);
	u32 result = AllocateRecurse(height,memtypeMask);
	// Change memory types as a last resort; if the allocation failed, try the other kinds of memory
	if (result == TERMINATOR)
		// Masking here with LEGAL_MEMTYPES isn't really necissary, since there
		// will be no memory available of illegal types, but does save a few
		// loop iterations.
		result = AllocateRecurse(height,~memtypeMask&LEGAL_MEMTYPES);
	// Set the height of all nodes that make up the allocation.  This is
	// required for GetFirstNode().  Also set the "in use" flag, currently only
	// required for asserts, but to save hassles in the future that could be
	// caused by different behaviour in different builds, just always set it.
	if (result != TERMINATOR) {
		u32 nodeCount = 1<<height;
		for (u32 i=0; i<nodeCount; ++i) {
			m_Nodes[result+i].SetIsUsed(1);
			m_Nodes[result+i].SetHeight(height);
		}
	}
	SanityCheck();
	return result;
}

void sysTinyBuddyHeap::Free(u32 idx) {
	FatalAssert(idx < m_NodeCount);
	FatalAssert(m_Nodes[idx].GetIsUsed());
	u32 height = m_Nodes[idx].GetHeight();
	u32 buddyBit = 1 << height;
	m_Used -= buddyBit;
	// Go up the tree, coalescing binary tree nodes, making sure our buddy is actually:
	// valid (might not be if near top of heap and it's not a perfect power of two in size, or we just went off the top of the heap)
	// unused, and the same height as us.  It might not match if our buddy has already been split but its right half is still in use.
	while ((idx ^ buddyBit) < m_NodeCount && !m_Nodes[idx ^ buddyBit].GetIsUsed() && m_Nodes[idx ^ buddyBit].GetHeight() == height) {
		// Remove sibling from its free list (it remains marked unused)
		RemoveFromFreeList(idx ^ buddyBit);
		// New parent is leftmost node
		idx = idx & ~buddyBit;
		// Height of new parent is taller.
		m_Nodes[idx].SetHeight(++height);
		buddyBit <<= 1;
	}
	// We've merged as much as we can, so add the resulting final node to the appropriate free list
	AddToFreeList(idx);
	SanityCheck();
}

void sysTinyBuddyHeap::AddToFreeList(u32 idx) {
	FatalAssert(idx < m_NodeCount);
	// Only incoming height needs to be valid
	u32 memtype = m_Nodes[idx].GetMemType();
	u32 height = m_Nodes[idx].GetHeight();
	m_Nodes[idx].SetIsUsed(0);
	// Patch self into head
	m_Nodes[idx].SetPrevFree(TERMINATOR);
	u32 nextFree = m_FreeListByHeight[height][memtype];
	m_Nodes[idx].SetNextFree(nextFree);
	// If list wasn't empty, update old head's predecessor to point back to us
	if (nextFree != TERMINATOR)
		m_Nodes[nextFree].SetPrevFree(idx);
	// Head now points to us.
	m_FreeListByHeight[height][memtype] = idx;
	FatalAssert(m_Nodes);
}

void sysTinyBuddyHeap::RemoveFromFreeList(u32 idx) {
	FatalAssert(m_Nodes);
	u32 height = m_Nodes[idx].GetHeight();
	u32 memtype = m_Nodes[idx].GetMemType();
	// Unlink selves from predecessor or head of list
	u32 nextFree = m_Nodes[idx].GetNextFree();
	u32 prevFree = m_Nodes[idx].GetPrevFree();
	if (prevFree == TERMINATOR)
		m_FreeListByHeight[height][memtype] = nextFree;
	else
		m_Nodes[prevFree].SetNextFree(nextFree);
	// Unlink selves from successor if we have one
	if (nextFree != TERMINATOR)
		m_Nodes[nextFree].SetPrevFree(prevFree);
	// Note we don't change our 'used' state because we don't know if we're being allocated
	// or we're being coalesced with a sibling.
	// Reset these both unconditionally now; we can't set either one until the other side is unlinked
	// and it's not worth testing again.
	m_Nodes[idx].SetPrevFree(TERMINATOR);
	m_Nodes[idx].SetNextFree(TERMINATOR);
}

#ifndef SanityCheck
void sysTinyBuddyHeap::SanityCheck() {
	u32 i, m, used = 0, free = 0;
	for (u32 h=0; h<NELEM(m_FreeListByHeight); ++h) {
		u32 size = 1 << h;
		for (m=0; m<NUM_MEMTYPES; m++) {
			for (i=m_FreeListByHeight[h][m]; i!=TERMINATOR; i=m_Nodes[i].GetNextFree()) {
				FatalAssert(m_Nodes[i].GetHeight() == h);
				FatalAssert(!m_Nodes[i].GetIsUsed());
				free += size;
			}
		}
	}
	for (i=0; i<m_NodeCount; ) {
		u32 size = 1 << m_Nodes[i].GetHeight();
		if (m_Nodes[i].GetIsUsed())
			used += size;
		i += size;
	}
	FatalAssert(i == m_NodeCount);
	FatalAssert(used == m_Used);
	FatalAssert(free == m_NodeCount - m_Used);
}
#else
#undef SanityCheck
#endif

}	// namespace rage

#endif // !defined(_MSC_VER) || _MSC_VER > 1600
