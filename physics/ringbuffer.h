// 
// physics/ringbuffer.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_RINGBUFFER_H 
#define PHYSICS_RINGBUFFER_H 

#include "diag/trap.h"
#include "system/new.h"

namespace rage {

template <class T>
class phRingBuffer
{
public:
	phRingBuffer(size_t size, int maxAllocs)
		: m_Buffer(rage_new u8[(size + 0xf) & ~0xf])
		, m_Size((size + 0xf) & ~0xf)
		, m_Top(0)
		, m_Allocs(rage_new T*[maxAllocs + 1])
		, m_AllocHead(0)
		, m_AllocTail(0)
		, m_MaxAllocs(maxAllocs + 1)
	{
	}

	~phRingBuffer()
	{
		delete [] m_Allocs;
		delete [] m_Buffer;
	}

	T* Allocate(int numObjects)
	{
		size_t size = T::ComputeSize(numObjects);

		// Can't allocate more space than we have
		if (size > m_Size)
		{
			return NULL;
		}

		// If we hit the end, go back to the beginning
		if (m_Top + size > m_Size)
		{
			m_Top = 0;
		}

		// Allocate "size" space from top
		size_t newTop = (m_Top + size + 0xf) & ~0xf;

		// The new object will occupy the space from m_Top downward by size, which might contain some current objects.
		// So, inform any objects we're tromping on that they are no longer needed
		TrapLT(m_AllocTail, 0);
		TrapGE(m_AllocTail, m_MaxAllocs);

		while (m_AllocTail != m_AllocHead &&
			m_Allocs[m_AllocTail] >= (T*)(m_Buffer + m_Top) &&
			m_Allocs[m_AllocTail] < (T*)(m_Buffer + newTop))
		{
			ReleaseTail();
		}

		// Create the new object
		TrapLT((int)m_Top, (int)0);
		TrapGT((int)(m_Top + size), (int)m_Size);
		T* newObject = ::new(m_Buffer + m_Top) T(numObjects);

		TrapLT(m_AllocHead, 0);
		TrapGE(m_AllocHead, m_MaxAllocs);

		m_Allocs[m_AllocHead] = newObject;
		m_AllocHead = (m_AllocHead + 1) % m_MaxAllocs;

		// We also have to delete the tail if we've wrapped around to the head, because head==tail indicates an empty buffer
		if (m_AllocHead == m_AllocTail)
		{
			ReleaseTail();
		}

		m_Top = newTop;
		return newObject;
	}

	void ReleaseTail()
	{
		TrapLT(m_AllocTail, 0);
		TrapGE(m_AllocTail, m_MaxAllocs);

		// Let the object clean up after itself
		m_Allocs[m_AllocTail]->~T();

		// Move the tail forward so the object it points to no longer exists
		m_AllocTail = (m_AllocTail + 1) % m_MaxAllocs;

		TrapLT(m_AllocTail, 0);
		TrapGE(m_AllocTail, m_MaxAllocs);
	}

private:
	u8* m_Buffer;
	size_t m_Size;
	size_t m_Top;

	T** m_Allocs;
	int m_AllocHead;
	int m_AllocTail;
	int m_MaxAllocs;
};

} // namespace rage

#endif // PHYSICS_RINGBUFFER_H 
