//
// atl/staticpool.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef ATL_STATICPOOL_H
#define ATL_STATICPOOL_H

#include "align.h"

#include <stddef.h>		// for size_t

namespace rage {

/*
 PURPOSE:
	atStaticPool maintains an array of objects on the caller's behalf and
	will allow random-access allocation and deallocation of the
	contained objects.  No constructors or destructors for the
	contained type are ever invoked by this code because of complications 
	with the way our build system redefines "new" as a macro.
	Allocation and deallocation are both done in O(1) time.
	Only requirement for class _Type is that its size is as least as
	large as a pointer (which is reasonable, because you need a pointer
	to store the result in the first place).

	The main difference between this and atPool is that this version
	owns its own storage directly as a contained array.  The class is
	also intended to be used at global scope so that we can do just-in-
	time construction in spite of global object order dependencies.
 PARAMS:
	_Type - the type of the instances in the static pool
	_ArraySize - the size of the pool
*/
template <class _Type, int _ArraySize> class atStaticPool {
public:
	// PURPOSE: Resets all pool entries to free; O(N) operation
	void Reset() {
		CompileTimeAssert(sizeof(_Type) >= sizeof(_Type*));

		_Type *p = (_Type*) (void*) m_Pool.m_Data;
		m_FirstFree = p;
		m_FreeCount = _ArraySize;
		int count = _ArraySize;
		while (--count) {
			*(_Type**)p = (p+1);
			++p;
		}
		*(_Type**)p = 0;

		m_Initialized = true;
	}

	// PURPOSE: Allocation function
	// RETURNS: Pointer to new object (unconstructed!).  Will assert out
	// or crash if no space is available; use IsFull to check if you want
	// to handle that yourself.
	// NOTES: Does NOT run constructor on the returned memory.  Most-recently
	// freed memory is returned to caller.  Runs in O(1) time.
	_Type* New(size_t ASSERT_ONLY(size) = 0) {
		// Sanity check to check creation on the heap or stack (variable will be
		// very unlikely to contain zero or one)
		FastAssert(m_Initialized == (int)false || m_Initialized == (int)true);
		if (!m_Initialized)
			Reset();
		AssertMsg(m_FirstFree , "atStaticPool is full");
		Assert(!size || size <= sizeof(_Type));
		_Type* result = m_FirstFree;
		m_FirstFree = *(_Type**)m_FirstFree;
		--m_FreeCount;
		return result;
	}

	_Type& GetElement(int index) { FastAssert(u32(index) < _ArraySize); return ((_Type*)m_Pool.m_Data)[index];}
	unsigned GetIndex(_Type* pElement) { unsigned index = unsigned(pElement - ((_Type*)m_Pool.m_Data)); FastAssert(index  < _ArraySize); return index; }

	// PURPOSE: Check if pool is full
	// RETURNS: True if pool is full, else false
	bool IsFull() const { return m_FirstFree == 0 && m_Initialized; }

	// RETURNS: Number of free entries in the pool
	int GetFreeCount() const { return m_Initialized? m_FreeCount : _ArraySize; }

	// RETURNS: Total number of entries in the pool
	int GetSize() const { return _ArraySize; }

	// PURPOSE: Deallocation function
	// PARAMS: ptr - Pointer to storage previously allocated by New
	// NOTES: Does NOT run destructor on the memory.  Freeing the same
	// pointer twice will corrupt the free list, leading to mass destruction.
	// Runs in O(1) time.
	void Delete(_Type *ptr) {
		if (ptr) {
			FastAssert(m_Initialized);
			FastAssert((u8*)ptr >= m_Pool.m_Data && (u8*)ptr < m_Pool.m_Data + sizeof(m_Pool.m_Data));
			*(_Type**)ptr = m_FirstFree;
			m_FirstFree = ptr;
			++m_FreeCount;
		}
	}

private:
	atAlignedStorage<sizeof(_Type) * _ArraySize, __alignof(_Type)> m_Pool;
	int m_FreeCount;
	_Type* m_FirstFree;
	int m_Initialized;
};

#define IMPLEMENT_STATIC_POOL(type,arrayCount) \
static atStaticPool<type,arrayCount> s_##type##Pool; \
void* type::operator new(size_t size) { return (void*) s_##type##Pool.New(size); } \
void type::operator delete(void *ptr) { s_##type##Pool.Delete((type*)ptr); }

}	// namespace rage

#endif
