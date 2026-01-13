// 
// system/tinyheap.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 
// NOTE - Several function definitions are marked !__SPU because they contain string literals that
// don't properly deadstrip and the functions generally aren't useful on that target anyway.

#include "tinyheap.h"
#include "diag/errorcodes.h"

#include <string.h>

namespace rage {

const size_t USED = 1;
const size_t MASK = 15;

#if __64BIT
struct sysTinyHeap::Node {
	size_t Size;		// Size of payload, always multiple of sizeof(Node).  LSB is set if node is in use.
	Node *Prev;			// NULL for first node in heap
};
struct sysTinyHeap::FreeNode: public Node {
	FreeNode *PrevFree;		// If node is free, this is the previous free node (NULL if smallest free node)
	FreeNode *NextFree;		// If node is free, this is the next free node (or NULL if no more free nodes)
};
#else
struct sysTinyHeap::Node {
	size_t Size;		// Size of payload, always multiple of sizeof(Node).  LSB is set if node is in use.
	Node *Prev;			// NULL for first node in heap
	Node *PrevFree;		// If node is free, this is the previous free node (NULL if smallest free node)
	Node *NextFree;		// If node is free, this is the next free node (or NULL if no more free nodes)
} ;
#if __SPU
#define FreeNode Node
#else
struct sysTinyHeap::FreeNode: public sysTinyHeap::Node { };
#endif
#endif



#define HeapAssert Assert
#define HeapAssertf Assertf

void sysTinyHeap::Init(void *start,size_t size)
{
	CompileTimeAssert(sizeof(sysTinyHeap::Node)==SYS_TINYHEAP_STORAGE_ALIGNMENT);
#if __64BIT
	CompileTimeAssert(sizeof(sysTinyHeap::FreeNode)==SYS_TINYHEAP_STORAGE_ALIGNMENT*2);
#else
	CompileTimeAssert(sizeof(sysTinyHeap::FreeNode)==SYS_TINYHEAP_STORAGE_ALIGNMENT);
#endif

	// If these asserts trigger, make sure start and size are both aligned to SYS_TINYHEAP_STORAGE_ALIGNMENT in the caller.
	HeapAssert(!((size_t)start & (sizeof(Node)-1)));
	HeapAssert(!(size & (sizeof(Node)-1)));
	m_Start = (Node*) start;
	m_End = (Node*) ((char*)start + size);

	// Build the head memory node, init the free list
	m_Rover = m_Start;
	m_Free = static_cast<FreeNode*>(m_Start);
	m_Start->Prev = 0;
	m_Start->Size = size - sizeof(Node);
	m_Remaining = m_MinRemaining = size;
	m_Free->PrevFree = m_Free->NextFree = 0;
	m_User0 = m_User1 = 0;
}

// Disable the extra checks.
#define VerifyFreeList()
#define SanityCheck()

#ifndef VerifyFreeList
void sysTinyHeap::VerifyFreeList()
{
	Node *i = m_Free;
	while (i && i->NextFree)
	{
		HeapAssert(!(i->Size & MASK));
		HeapAssert(i->Size <= i->NextFree->Size);
		HeapAssert(i->NextFree->PrevFree == i);
		i = i->NextFree;
	}
}
#endif

#ifndef SanityCheck
void sysTinyHeap::SanityCheck()
{
	Node *p = NULL;
	Node *i = m_Start;
	size_t computedRemain = ((char*)m_End - (char*)m_Start);

	// Check basic connectivity
	while (i != m_End)
	{
		HeapAssert(i->Prev == p);
		p = i;
		if (i->Size & USED)
			computedRemain -= (i->Size & ~MASK) + sizeof(Node);
		i = (Node*)((char*)i + sizeof(Node) + (i->Size & ~MASK));
	}

	// Make sure everything on the free list is unused.
	i = m_Free;
	p = NULL;
	while (i)
	{
		HeapAssert(!(i->Size & USED));
		HeapAssert(i->PrevFree == p);
		p = i;
		i = i->NextFree;
	}
	HeapAssert(m_Remaining == computedRemain);
}
#endif

void sysTinyHeap::RemoveFromFreeList(FreeNode *n)
{
	HeapAssert(!(n->Size & USED));

	m_Remaining -= (n->Size + sizeof(Node));

	// Unlink predecessor from us
	if (!n->PrevFree)
	{
		HeapAssert(m_Free == n);
		m_Free = (FreeNode*) n->NextFree;
	}
	else
		n->PrevFree->NextFree = n->NextFree;

	// Unlink successor from us
	if (n->NextFree)
		n->NextFree->PrevFree = n->PrevFree;

	n->PrevFree = n->NextFree = 0;
	VerifyFreeList();
}

void sysTinyHeap::AddToFreeList(FreeNode *n, size_t nSize)
{
	HeapAssert(!(n->Size & USED));
	HeapAssert(n->Size == nSize);
	HeapAssert(__64BIT || (!n->PrevFree && !n->NextFree));
	HeapAssert(!((size_t)n & (sizeof(Node)-1)));

	m_Remaining += (nSize + sizeof(Node));

	// If there is no free list, or we're no larger than the first node, insert at head
	// (Inserting at head is fastest, so if we're the same size we may as well go there)
	if (!m_Free || nSize <= m_Free->Size)
	{
		n->PrevFree = 0;
		n->NextFree = m_Free;
		if (m_Free)
			m_Free->PrevFree = n;
		m_Free = n;
		VerifyFreeList();
	}
	else
	{
		FreeNode *i = m_Free;
		for (;;)
		{
			// Insert before this node?
			if (i->Size > nSize)
			{
				(n->PrevFree = i->PrevFree)->NextFree = n;
				n->NextFree = i;
				i->PrevFree = n;
				// VerifyFreeList();
				break;
			}
			// At end of list, this is the new end?
			else if (!i->NextFree)
			{
				i->NextFree = n;
				n->PrevFree = i;
				n->NextFree = 0;
				// VerifyFreeList();
				break;
			}
			// otherwise keep looking.
			else
			{
				HeapAssert(i->Size <= i->NextFree->Size);
				HeapAssert(i->NextFree->PrevFree == i);
				i = (FreeNode*) i->NextFree;
			}
		}
	}
}

#if __SPU
void* sysTinyHeap::Allocate(size_t size)
#else
void* sysTinyHeap::Allocate(size_t size,size_t /*align*/,size_t bucket)
#endif
{
#if __SPU
	const size_t bucket = USED;
#else
	HeapAssert(bucket < 8);
	bucket = (bucket << 1) | USED;
#endif
	if (!size)
		size = sizeof(Node);
	else
		size = (size + sizeof(Node) - 1) & ~(sizeof(Node) - 1);
	// HeapAssert(align == sizeof(Node));

	size_t remaining = m_Remaining;

	// Free list is sorted by size.
	FreeNode *f = m_Free;
	while (f)
	{
		// All free nodes have their "in use" bit cleared so we can reference Size directly.
		if (size <= f->Size)	// Found one large enough?
		{
			remaining -= f->Size + sizeof(Node);

			RemoveFromFreeList(f);
			// Enough room to split the node?
			if (size + sizeof(Node) < f->Size)
			{
				size_t origSize = f->Size;
				FreeNode *split = (FreeNode*)((char*)f + sizeof(Node) + size);
				Node *next = (Node*)((char*)f + sizeof(Node) + origSize);
				f->Size = size | bucket;		// Mark node used (and resize it)
				split->Prev = f;
				size_t sizeDiff = origSize - size;
				size_t splitSize = sizeDiff - sizeof(Node);
				split->Size = splitSize;
				split->NextFree = split->PrevFree = 0;
				HeapAssert(next <= m_End);
				if (next != m_End)
				{
					HeapAssert(next->Prev == f);
					next->Prev = split;
				}
				remaining += sizeDiff;
				AddToFreeList(split, splitSize);
			}
			else	// Node is exact fit or there's not enough space to insert a node, just mark it used.
				f->Size |= bucket;
			HeapAssert(f->Size & USED);
			SanityCheck();
			m_Rover = f;
			if (m_MinRemaining > remaining)
				m_MinRemaining = remaining;
			return (char*)f + sizeof(Node);
		}

		HeapAssert(!f->NextFree || f->NextFree->PrevFree == f);
		f = (FreeNode*) f->NextFree;
	}

	// Not enough memory...
	SanityCheck();
	return 0;
}


#if !__SPU
size_t sysTinyHeap::GetSize(const void *ptr) const
{
	if (!ptr)
		return 0;

	Node *f = (Node*)((char*)ptr - sizeof(Node));
	if (!(f->Size & USED))
		Quitf(ERR_MEM_TINY_1,"sysTinyHeap - Bad pointer passed to GetAllocationSize at %p",ptr);
	return f->Size & ~MASK;
}

size_t sysTinyHeap::GetSizeWithOverhead(const void *ptr) const
{ 
	return GetSize(ptr) + sizeof(Node); 
}

int sysTinyHeap::GetBucket(void *ptr) const
{
	if (!ptr)
		return 0;

	Node *f = (Node*)((char*)ptr - sizeof(Node));
	if (!(f->Size & USED))
		Quitf(ERR_MEM_TINY_2,"sysTinyHeap - Bad pointer passed to GetBucket at %p",ptr);
	return int(f->Size & MASK) >> 1;
}
#endif


void sysTinyHeap::Free(void *ptr)
{
	if (!ptr)
		return;

	if (ptr <= m_Start || ptr >= m_End || ((size_t)ptr & (sizeof(Node)-1)))
		Quitf(ERR_MEM_TINY_FREE_1,"sysTinyHeap - Out-of-range or unalaigned pointer %p",ptr);

	FreeNode *f = (FreeNode*)((char*)ptr - sizeof(Node));
	if (!(f->Size & USED))
		Quitf(ERR_MEM_TINY_FREE_2,"sysTinyHeap - Double free at %p",ptr);
	f->Size &= ~MASK;
	size_t origSize = f->Size;
	FreeNode *next = (FreeNode*)((char*)f + sizeof(Node) + origSize);

	// If previous node is free, merge it with us
	if (f->Prev && !(f->Prev->Size & USED))
	{
		f = static_cast<FreeNode*>(f->Prev);
		RemoveFromFreeList(f);
		f->Size += sizeof(Node) + origSize;
		HeapAssertf(next <= m_End, "sysTinyHeap - previous node %p was corrupted", f);
		if (next != m_End)
			next->Prev = f;
	}

	// By the time we get here, f is not on any free list yet.
	// If next node is free, merge it with us
	if (next != m_End && !(next->Size & USED))
	{
		RemoveFromFreeList(next);
		size_t nextSize = next->Size;
		f->Size += sizeof(Node) + nextSize;
		Node *nextNext = (Node*)((char*)next + sizeof(Node) + nextSize);
		HeapAssertf(nextNext <= m_End, "sysTinyHeap - next node %p was corrupted", next);
		if (nextNext != m_End)
			nextNext->Prev = f;
	}

	// Now any merging is complete, and our final size should be correct.
	AddToFreeList(f, f->Size);
	m_Rover = f;

	SanityCheck();
}


size_t sysTinyHeap::GetLargestFreeBlock()
{
	if (m_Free) {
		// Last free block is always largest
		FreeNode *i = m_Free;
		while (i->NextFree)
			i = (FreeNode*) i->NextFree;
		return i->Size;
	}
	else
		return 0;
}

#if !__SPU
bool sysTinyHeap::Resize(void *ptr,size_t newSize)
{
 	if (ptr <= m_Start || ptr >= m_End || ((size_t)ptr & (sizeof(Node)-1))) {
		Quitf(ERR_MEM_TINY_RESIZE_1,"sysTinyHeap - Out-of-range or unalaigned pointer %p",ptr);
	}

	Node *i = (Node*)((char*)ptr - sizeof(Node));
	if (!(i->Size & USED)) {
		Quitf(ERR_MEM_TINY_RESIZE_2,"sysTinyHeap - Resize of freed pointer?");
	}
#if __SPU
	const size_t bucket = USED;
#else
	size_t bucket = i->Size & MASK;	// note USED will be set here.
#endif

	size_t oldSize = i->Size & ~MASK;

	// Round the new size off appropriately.
	if (!newSize)
		newSize = sizeof(Node);
	else
		newSize = (newSize + sizeof(Node) - 1) & ~(sizeof(Node) - 1);
		
	FreeNode *next = (FreeNode*)((char*)i + sizeof(Node) + oldSize);

	if (newSize > oldSize)
	{
		// If next block is end of heap, or is already used, or isn't big enough, we can't do anything.
		if (next == m_End || (next->Size & USED) || newSize > oldSize + sizeof(Node) + next->Size)
			return false;
		// Either exact fit, or not enough space to split the node
		else if (newSize == oldSize + next->Size || newSize == oldSize + sizeof(Node) + next->Size)
		{
			newSize = oldSize + sizeof(Node) + next->Size;
			RemoveFromFreeList(next);
			i->Size = newSize | bucket;	// preserve bucket assignment
			// Compute new successor
			next = (FreeNode*)((char*)i + sizeof(Node) + newSize);
			if (next != m_End)
				next->Prev = i;
			SanityCheck();
			m_Rover = next;
			if (m_MinRemaining > m_Remaining)
				m_MinRemaining = m_Remaining;
			return true;
		}
		else
		{
			// Split the node
			FreeNode *nextNext = (FreeNode*)((char*)next + sizeof(Node) + next->Size);
			i->Size = newSize | bucket;	// preserve bucket assignment
			RemoveFromFreeList(next);
			next = (FreeNode*)((char*)i + sizeof(Node) + newSize);
			next->Size = (char*)nextNext - (char*)next - sizeof(Node);
			next->Prev = i;
			next->PrevFree = next->NextFree = NULL;
			AddToFreeList(next, next->Size);
			if (nextNext != m_End)
				nextNext->Prev = next;
			SanityCheck();
			m_Rover = next;
			if (m_MinRemaining > m_Remaining)
				m_MinRemaining = m_Remaining;
			return true;
		}
	}
	else	// TODO: Could do a better job if the node shrank more than sizeof(Node).
		return true;
}
#endif


void sysTinyHeap::Defragment(ptrdiff_t maxToCopy,void (*cb)(void*,ptrdiff_t))
{
	// If the heap is completely full or only has a single free block, there's nothing to do.
	if (!m_Free || !m_Free->NextFree)
		return;

	Node *i = m_Rover;
	if (i == m_End)
		i = m_Start;

	while (i < m_End && maxToCopy > 0)
	{
		FreeNode *next = (FreeNode*) ((char*)i + sizeof(Node) + (i->Size & ~MASK));

		if ((i->Size & USED) || next == m_End)
		{
			// Already in use, or the last node, just skip it.
			i = next;

			// Estimate the amount of memory copied by the size of a cache line.
			maxToCopy -= 128;
		}
		else // Free block that is not the last block in the heap.
		{
			size_t freeSize = i->Size;		// known free, so can use Size directly.
			HeapAssert(next->Size & USED);		// Two free nodes should never be adjacent.

			// Remove the free block from the free list.
			RemoveFromFreeList(static_cast<FreeNode*>(i));

			size_t nextSize = next->Size & ~MASK;
			ptrdiff_t offset = (ptrdiff_t)i - (ptrdiff_t)next;

			// Copy the next block (which must be used).  This will likely destroy the node header at 'next',
			// so we can't dereference that any longer.  Could assume downward copy.
			memmove((char*)i + sizeof(Node), (char*)next + sizeof(Node), nextSize);
			maxToCopy -= (nextSize + sizeof(Node));
			i->Size = nextSize | USED;			// i->Prev is still good.

			// Construct the free node after the in-use node we just copied downward.
			next = (FreeNode*) ((char*)i + sizeof(Node) + nextSize);
			next->Prev = i;
			next->Size = freeSize;
			next->PrevFree = next->NextFree = NULL;

			// If we just managed to merge two free nodes together, rejoice.
			FreeNode *nextNext = (FreeNode*) ((char*)next + sizeof(Node) + freeSize);
			if (nextNext != m_End && !(nextNext->Size & USED))
			{
				next->Size += nextNext->Size + sizeof(Node);
				RemoveFromFreeList(nextNext);

				// Recompute successor node for iteration
				nextNext = (FreeNode*) ((char*)next + sizeof(Node) + next->Size);
			}

			// Keep linked lists correct whether we recomputed nextNext or not.
			if (nextNext != m_End)
				nextNext->Prev = next;

			// Add the new node back to the free list again.
			AddToFreeList(next, next->Size);

			SanityCheck();

			// Fix up the node we just copied.
			cb((char*)i + sizeof(Node),offset);

			SanityCheck();

			// Point to next item to process.
			i = nextNext;
		}
	}

	// Remember where to pick up next time.
	m_Rover = i;

	SanityCheck();
}

#ifdef VerifyFreeList
#undef VerifyFreeList
#endif

#ifdef SanityCheck
#undef SanityCheck
#endif

} // namespace rage
