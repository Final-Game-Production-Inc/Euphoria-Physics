// 
// atl/defragheap.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_DEFRAGHEAP_H 
#define ATL_DEFRAGHEAP_H 

namespace rage {

/*
	PURPOSE:	Implements a defragmentable heap of arbitrary objects.	Also supports
				garbage collection through a mark-and-sweep interface.

	We have an 16-byte arena header, and all allocations are 16-byte-aligned so that any class
	can be safely used.  This also allows us to use DMA or quadword-aligned copies during defragmentation.

	The handle is merely an index into a private array of memory offsets.  Zero is an invalid handle.
*/
class atDefragmentableHeap {
public:
	typedef struct Handle__* Handle;

	Handle Allocate(size_t size);
	void Free(Handle handle);

	// RETURNS:	Nonzero if the handle is valid.
	u32 ValidateHandle(Handle handle) {
		u32 index = (u32) handle;
		u32 magic = index >> 20;
		index &= 0xFFFFF;
		if (index > 0 && index < m_HandleCount) {
			Assert(magic == m_Handles[index].Magic);
			if (magic == m_Handles[index].Magic)
				return index;
		}
		return 0;
	}

	void *Lock(Handle handle) { 
		u32 index = ValidateHandle(handle); 
		if (index) {
			++m_LockCount;
			return m_Start + m_Handles[index].Offset;
		}
		else
			return NULL;
	}

	void Unlock(Handle handle) {
		u32 index = ValidateHandle(handle);
		if (index) {
			Assert(m_LockCount);
			--m_LockCount;
		}
	}

	void BeginGarbageCollect();
	void EndGarbageCollect();
	void Defragment();

	static const u32 MaxHandleCount = 1024*1024;		// Due to width of Entry::Offset
	static const u32 MaxHeapSize = 4*1024*1024;			// Due to width of Entry::Offset, which is in words.
private:
	struct Node {
		u32 Size;			// Our size, so we can find next node
		u32 PrevSize;		// Previous node's size, so we can get to its header
		u32 Owner;			// Index of owning handle, or zero if this node is free.
	}
	struct FreeNode: public Node {
		u64 NextFree:20, PrevFree:20;
	}
	struct Entry {
		u32 Free:1,			// handle is free
			Marked:1,		// handle was marked
			Offset:20,		// Offset into heap (in words) if handle is allocated
							// or index of next free handle if this handle is free (heap size is therefore 4M)
			Magic:10;		
	};
	Entry *m_Handles;		// zero is never used
	int m_HandleCount;
	int m_LockCount;
	u32 m_TotalFree;
	u32 m_FirstFree;
	u32 m_NextMagic;
	Node *m_Start, *m_End;
}

} // namespace rage

#endif // ATL_DEFRAGHEAP_H 
