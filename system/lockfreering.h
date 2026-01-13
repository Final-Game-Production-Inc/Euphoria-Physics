// 
// system/lockfreering.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_LOCKFREERING_H
#define SYSTEM_LOCKFREERING_H

#include "system/interlocked.h"

namespace rage
{

// PURPOSE: Lock-Free bounded FIFO container
template<typename T, u32 SIZE>
class sysLockFreeRing
{
	// PURPOSE: size must be a power of 2
	CompileTimeAssert(0 == (SIZE & (SIZE-1)));

public:
	// PURPOSE: Constructor
	sysLockFreeRing();

	// PURPOSE: Push a new node
	// PURPOSE: true if succeed
	bool Push(T* node);

	// PURPOSE: Pop an existing node
	// RETURN: NULL if the stack is empty
	T* Pop();

	// PURPOSE: Return true if ring is empty
	// NOTE: Fast version not 100% reliable
	bool IsEmpty() const;

private:
	static const u32 sm_MaxRingEntriesMask = (SIZE - 1);

	volatile u32 m_Head;
	volatile u32 m_Tail;
	u32 m_Sequence[SIZE];
	T* m_Entries[SIZE];
};

////////////////////////////////////////////////////////////////////////////////

template<typename T, u32 SIZE>
sysLockFreeRing<T, SIZE>::sysLockFreeRing()
: m_Head(0)
, m_Tail(0)
{
	for(u32 i=0; i<SIZE; ++i)
	{
		m_Sequence[i] = i;
		m_Entries[i] = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, u32 SIZE>
T* sysLockFreeRing<T, SIZE>::Pop()
{
	u32 tail = m_Tail;
	for(;;)
	{
		u32 seq = m_Sequence[tail&sm_MaxRingEntriesMask];
		int diff = seq - (tail+1);
		if(diff == 0)
		{
			u32 tail2 = sysInterlockedCompareExchange(&m_Tail, tail+1, tail);
			if(tail == tail2)
			{
				break;
			}
			tail = tail2;
		}
		else if(diff < 0)
		{
			return NULL;
		}
		else
		{
			tail = m_Tail;
		}
	}

	T* node = (T*)m_Entries[tail&sm_MaxRingEntriesMask];
	sysInterlockedExchange(&m_Sequence[tail&sm_MaxRingEntriesMask], tail+sm_MaxRingEntriesMask+1);

	return node;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, u32 SIZE>
bool sysLockFreeRing<T, SIZE>::Push(T* node)
{
	u32 head = m_Head;
	for(;;)
	{
		u32 seq = m_Sequence[head&sm_MaxRingEntriesMask];
		int diff = seq - head;
		if(diff == 0)
		{
			u32 head2 = sysInterlockedCompareExchange(&m_Head, head+1, head);
			if(head == head2)
			{		
				break;
			}
			head = head2;
		}
		else if(diff < 0)
		{
			Assertf(false, "sysLockFreeRing is out of free nodes! Node: %p, Max: %u", node, SIZE);
			return false;
		}
		else
		{
			head = m_Head;
		}
	}

	sysInterlockedExchangePointer((void**)&m_Entries[head&sm_MaxRingEntriesMask], (void*)node);
	sysInterlockedExchange(&m_Sequence[head&sm_MaxRingEntriesMask], head+1);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, u32 SIZE>
bool sysLockFreeRing<T, SIZE>::IsEmpty() const
{
	return m_Head == m_Tail;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace rage

#endif // SYSTEM_LOCKFREERING_H
