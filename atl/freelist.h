// 
// atl/freelist.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef ATL_FREELIST_H 
#define ATL_FREELIST_H 

#include "atl/array.h"

namespace rage {

// Free list; freed items are immediately available.  Allocate and Free are both O(1)
template <typename _Type,int _Size> class atFreeList
{
public:
	
	atFreeList() { MakeAllFree(); }

	void MakeAllFree()
	{
		CompileTimeAssert((_Type)(_Size) == _Size);		// Catch overflow
		FirstFree = 0;
		for (int i=0; i<_Size; i++)
			List[i] = (_Type)(i+1);		// Last entry will be _Size, our "end of list" sentinel
	}
	// Returns an available index, or -1 on error
	int Allocate()
	{
		int result = FirstFree;
		if (result != _Size)
		{
			FirstFree = List[FirstFree];
			return result;
		}
		else
			return -1;
	}
	// Makes index free; no protection against freeing something twice, be careful.
	void Free(int idx)
	{
		List[idx] = FirstFree;
		FirstFree = (_Type) idx;
	}
	// Returns true if the free list is empty (ie Allocate will fail)
	bool IsEmpty() const
	{
		return FirstFree == _Size;
	}
private:
	_Type FirstFree;
	atRangeArray<_Type,_Size> List;
};

// same as atFreeList, but takes a size at construction and adds double free protection
template <typename _Type> class atFreeListVariable
{
public:
	atFreeListVariable(int size) 
	{
		Size = (_Type)size;
		Assert(Size == size);		// Catch overflow
		List.Resize(Size);
		MakeAllFree(); 
	}

	void MakeAllFree()
	{
		FirstFree = 0;
		for (int i=0; i<Size; i++)
			List[i] = (_Type)(i+1);		// Last entry will be _Size, our "end of list" sentinel
	}
	// Returns an available index, or -1 on error
	int Allocate()
	{
		int result = FirstFree;
		if (result != Size)
		{
			FirstFree = List[FirstFree];
			List[result] = Size+1; // mark it as used
			return result;
		}
		else
			return -1;
	}
	// Makes index free; no protection against freeing something twice, be careful.
	void Free(int idx)
	{
		if(List[idx]==Size+1) // only free it once.
		{
			List[idx] = FirstFree;
			FirstFree = (_Type) idx;
		}
	}
	// Returns true if the free list is empty (ie Allocate will fail)
	bool IsEmpty() const
	{
		return FirstFree == Size;
	}
private:
	_Type Size;
	_Type FirstFree;
	atArray<_Type> List;
};



// Deferred free list; freed items are not available until DeferredUpdate is called, free can be called multiple times on an index
// Allocate and DeferredFree are both O(1), as is DeferredUpdate.
template <typename _Type,size_t _Size> class atDeferredFreeList
{
public:
	atDeferredFreeList() { MakeAllFree(); }

	void MakeAllFree()
	{
		CompileTimeAssert((_Type)(_Size) == _Size);		// Catch overflow
		ActiveFirstFree = 0;			// Active Free list has everything available
		DeferredFirstFree = _Size;		// Deferred free list is empty
		DeferredLastFree = _Size;
		for (int i=0; i<_Size; i++)
			List[i] = (_Type)(i+1);		// Last entry will be _Size, our "end of list" sentinel
	}
	// Returns an available index, or -1 on error
	int Allocate()
	{
		int result = ActiveFirstFree;
		if (result != _Size)
		{
			ActiveFirstFree = List[ActiveFirstFree];
			List[result] = _Size+1; // mark it as used
			return result;
		}
		else
			return -1;
	}
	// Makes index free 
	// It goes into the deferred list.
	void DeferredFree(int idx)
	{
		if(List[idx]==_Size+1) // only free it once.  
		{
			if (DeferredFirstFree == _Size)
				DeferredLastFree = (_Type)idx; // keep this one, so we can link the two lists 

			List[idx] = DeferredFirstFree;
			DeferredFirstFree = (_Type) idx;
		}
	}
	// Makes everything in the deferred list available to the main list.
	void DeferredUpdate()
	{
		// Point the end of the deferred free list at the start of the active free list
		// unless the deferred list is empty.
		if (DeferredFirstFree != _Size)
		{
			TrapNE(List[DeferredLastFree],_Size); // this first one has to be the end.

			// link the chains together.
			List[DeferredLastFree] = ActiveFirstFree;

			// The active list now starts at the deferred list (which may be empty)
			ActiveFirstFree = DeferredFirstFree;

			// And the deferred list is now free.
			DeferredFirstFree = _Size;
			DeferredLastFree = _Size;
		}
	}
	// Returns true if the active free list is empty (ie Allocate will fail)
	bool IsEmpty() const
	{
		return ActiveFirstFree == _Size;
	}
private:
	_Type ActiveFirstFree, DeferredFirstFree, DeferredLastFree;
	atRangeArray<_Type,_Size> List;
};


// same atDeferredFreeList above, but the size of the array is determined at run time
template <typename _Type> class atDeferredFreeListVariable
{
public:
	atDeferredFreeListVariable(int size) 
	{
		Size = (_Type)size;
		Assert(Size == size);		// Catch overflow
		List.Resize(Size);
		MakeAllFree();
	}

	void MakeAllFree()
	{
		ActiveFirstFree = 0;			// Active Free list has everything available
		DeferredFirstFree = Size;		// Deferred free list is empty
		DeferredLastFree = Size;
		for (int i=0; i<Size; i++)
			List[i] = (_Type)(i+1);		// Last entry will be _Size, our "end of list" sentinel
	}
	// Returns an available index, or -1 on error
	int Allocate()
	{
		int result = ActiveFirstFree;
		if (result != Size)
		{
			ActiveFirstFree = List[ActiveFirstFree];
			List[result] = Size+1; // mark it as used
			return result;
		}
		else
			return -1;
	}
	// Makes index free 
	// It goes into the deferred list.
	void DeferredFree(int idx)
	{
		if(List[idx]==Size+1) // only free it once.
		{
			if (DeferredFirstFree==Size)
				DeferredLastFree = (_Type)idx; // keep this one, so we can link the two lists 
			
			List[idx] = DeferredFirstFree;
			DeferredFirstFree = (_Type) idx;
		}
	}
	// Makes everything in the deferred list available to the main list.
	void DeferredUpdate()
	{
		// Point the end of the deferred free list at the start of the active free list
		// unless the deferred list is empty.
		if (DeferredFirstFree != Size)
		{
			TrapNE(List[DeferredLastFree],Size); // this first one has to be the end.

			// link the chains together.
			List[DeferredLastFree] = ActiveFirstFree;
		
			// The active list now starts at the deferred list (which may be empty)
			ActiveFirstFree = DeferredFirstFree;
			
			// And the deferred list is now free.
			DeferredFirstFree = Size;
			DeferredLastFree = Size;
		}
	}
	// Returns true if the active free list is empty (ie Allocate will fail)
	bool IsEmpty() const
	{
		return ActiveFirstFree == Size;
	}
private:
	_Type Size;
	_Type ActiveFirstFree, DeferredFirstFree, DeferredLastFree;
	atArray<_Type> List;
};

} // namespace rage

#endif // ATL_FREELIST_H 
