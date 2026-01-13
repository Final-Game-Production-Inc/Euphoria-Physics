// 
// system/tinyheap.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_TINYHEAP_H 
#define SYSTEM_TINYHEAP_H 

#include <stddef.h>

namespace rage {

#define SYS_TINYHEAP_STORAGE_ALIGNMENT 16

/* 
	Minimal heap class, but still reasonably fast.  Designed mostly for SPU use, but it guarantees sizeof(four pointers)
	alignment (nothing stricter) anywhere you'd need it.  Node object is declared aligned to avoid unnecessary shifts
	on the SPU.
*/
class sysTinyHeap 
{
public:
	sysTinyHeap() : m_Start(0), m_End(0), m_Free(0), m_Rover(0), m_Remaining(0), m_MinRemaining(0) { }

	sysTinyHeap(void *start,size_t size) { Init(start, size); }

	~sysTinyHeap() 
	{
		m_Start = m_End = m_Rover = NULL;
		m_Free = NULL;
		m_Remaining = m_MinRemaining = 0;
	}

	// PURPOSE:	Initialize the heap.
	// PARAMS:	start - starting address of the heap, must be aligned to sizeof(Node)
	//			size - Size of the heap, must be multiple of sizeof(Node)
	void Init(void *start,size_t size);

	// PURPOSE:	Determines if the heap has been initialized.
	bool IsInitialized() const {return m_Start != NULL;}

	// PURPOSE:	Allocates memory.
	// PARARMS:	size - Amount to allocate; rounded up to next multiple of sizeof(Node) if not one already.
	// NOTES:	Allocations of zero bytes internally allocate sizeof(Node) bytes instead.
	//			bucket must be between 0 and 7 inclusive.
#if __SPU
	void* Allocate(size_t size);
#else
	void* Allocate(size_t size, size_t align = 16, size_t bucket = 0);
#endif

	// RETURNS: Bucket index assocated with memory block (as per bucket parameter in Allocate call)
	int GetBucket(void *ptr) const;

	// PURPOSE: Frees memory
	// PARAMS:	ptr - Address of memory to free (previously returned by Allocate)
	// NOTES:	NULL is ignored without error.
	void Free(void *ptr);

	// PURPOSE:	Defragments memory.  Used blocks have the callback invoked.
	// PARAMS:	maxToCopy - Maximum amount of memory to copy before stopping, to allow timeslicing.
	//				Internally, simply scanning memory is billed as small copies to limit the number
	//				of cache misses we'll trigger.  Pass a huge number to maxToCopy if you really
	//				want to do all possible work this frame.
	//			cb - Callback that is invoked for every used block after it has been moved.
	//					First parameter is the block's new address.  Second parameter is the
	//					delta between its initial and final location (ie add that value to pointers
	//					to fix them up).
	void Defragment(ptrdiff_t maxToCopy,void (*cb)(void*,ptrdiff_t));	// Defragment the heap

	// PURPOSE:	Returns the size of the largest free node, or zero if none.  Operation is O(N) in number of free blocks.
	size_t GetLargestFreeBlock();

	// PURPOSE:	Resizes a previous memory allocation in-place.
	// PARAMS:	ptr - Address to resize (previously returned by Allocate)
	//			newSize - new memory size
	// RETURNS:	True if the memory block could be resized in-place, else false.
	// NOTES:	Making a memory block smaller will always succeed but won't actually do any work.
	//			This could be improved.  Making a block larger will succeed if there was a free
	//			block big enough after the current block.
	bool Resize(void *ptr,size_t newSize);

	// PURPOSE:	True if this is a valid heap pointer owned by this heap.
	bool IsValidPointer(const void* ptr) const {return ptr >= m_Start && ptr < m_End;}

	// PURPOSE:	Run basic sanity checks on the heap.
	void SanityCheck();

	// PURPOSE: Returns amount of memory remaining on heap
	size_t GetMemoryAvailable() const { return m_Remaining; }

	// PURPOSE: Returns amount of memory used on heap
	size_t GetMemoryUsed() const { return GetHeapSize() - m_Remaining; }

	// PURPOSE: Return the size of the heap; the same as the size parameter passed into Init
	size_t GetHeapSize() const { return (char*)m_End - (char*)m_Start; }

	// PURPOSE: Return minimum remaining memory, and optionally reset it.
	size_t GetLowWaterMark(bool reset) { 
		size_t result = m_MinRemaining;
		if (reset)
			m_MinRemaining = m_Remaining;
		return result;
	}

	// PURPOSE: Return maximum remaining memory, and optionally reset it.
	size_t GetHighWaterMark(bool reset) {return GetHeapSize() - GetLowWaterMark(reset);}

	size_t GetSize(const void *ptr) const;

	size_t GetSizeWithOverhead(const void *ptr) const;

	void *GetHeapStart() const { return m_Start; }

	void *GetHeapEnd() const { return m_End; }

	size_t GetUser0() const { return m_User0; }

	void SetUser0(size_t s) { m_User0 = s; }

	size_t GetUser1() const { return m_User1; }

	void SetUser1(size_t s) { m_User1 = s; }
private:
	struct Node;
#if __SPU
	#define FreeNode Node
#else
	struct FreeNode;
#endif

	void RemoveFromFreeList(FreeNode*);
	void AddToFreeList(FreeNode*,size_t);
	void VerifyFreeList();
	Node *m_Start, *m_End;		// Start and end (one past) of the heap.
	Node *m_Rover;				// Pointer to any valid node in the heap (used to timeslice defragmentation)
	FreeNode *m_Free;			// First free node in the heap; free list is kept in sorted order
	size_t m_Remaining, m_MinRemaining,
		m_User0, m_User1;
} SPU_ONLY();

} // namespace rage

#endif // SYSTEM_TINYHEAP_H 