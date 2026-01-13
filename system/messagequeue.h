//
// system/messagequeue.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_MESSAGEQUEUE_H
#define SYSTEM_MESSAGEQUEUE_H

#include "system/ipc.h"
#include "system/criticalsection.h"

namespace rage {

/* 
PURPOSE:
	Implement a type-safe circular queue class which is semaphore-protected and therefore
	can be used as a message queue between two threads.

	Usage here is similar to an atArray, except that Push and Pop implement a FIFO, not a LIFO. 

	Head, tail, and count are all stored explicitly so the queue can tell empty and full apart
	without having to waste a queue slot.
<FLAG Component>
*/

template <class _Type,int _Size,bool _blockOnFull=false>
class sysMessageQueue {
public:
	// PURPOSE: Default constructor
	sysMessageQueue() {
		m_NonEmpty = sysIpcCreateSema(false);
		if (_blockOnFull)
			m_NonFull = sysIpcCreateSema(_Size);
		m_Head = m_Tail = m_Count = 0;
	}
	
	// PURPOSE: Destructor
	~sysMessageQueue() {
		sysIpcDeleteSema(m_NonEmpty); 
		if (_blockOnFull)
			sysIpcDeleteSema(m_NonFull); 
	}
	
	// PURPOSE: Push an object into the queue
	// PARAMS: t - Object to push into queue (uses assignment to copy)
	// RETURNS: True on success, or false of queue was full
	_Type* Push(const _Type& t) {
		_Type *rv = NULL;
		if (_blockOnFull)
			sysIpcWaitSema(m_NonFull);
		m_Crit.Lock();
		if (m_Count != _Size) {
			if (++m_Head == _Size)
				m_Head = 0;
			m_Q[m_Head] = t;
			rv = &m_Q[m_Head];
			++m_Count;
			m_Crit.Unlock();
			sysIpcSignalSema(m_NonEmpty);
		}
		else
			m_Crit.Unlock();
		return rv;
	}
	
	// PURPOSE: Returns the address of the top entry in the queue.
	//		Will block calling thread until something is available.
	_Type Pop() {
		sysIpcWaitSema(m_NonEmpty);
		m_Crit.Lock();
		if (++m_Tail == _Size)
			m_Tail = 0;
		_Type dest = m_Q[m_Tail];
		--m_Count;
		m_Crit.Unlock();
		if (_blockOnFull)
			sysIpcSignalSema(m_NonFull);
		return dest;
	}

	// PURPOSE: Pops the top entry in the queue into dest.
	//		Will block calling thread until something is available or timeout elapses.
	// PARAMS: dest - Receives top entry in queue
	//			ms - timeout in milliseconds
	// RETURNS: true if dest is false, false if timeout elapsed.
	bool PopWait(_Type &dest,unsigned ms = sysIpcInfinite) {
		FastAssert(ms > 0); // Use PopPoll instead
		if (sysIpcWaitSemaTimed(m_NonEmpty,ms)) {
			m_Crit.Lock();
			if (++m_Tail == _Size)
				m_Tail = 0;
			dest = m_Q[m_Tail];
			--m_Count;
			m_Crit.Unlock();
			if (_blockOnFull)
				sysIpcSignalSema(m_NonFull);
			return true;
		}
		else
			return false;
	}


	// PURPOSE: Pops the top entry in the queue into dest.
	//		Will block calling thread until something is available or timeout elapses.
	// PARAMS: dest - Receives top entry in queue
	//			ms - timeout in milliseconds
	// RETURNS: true if dest is false, false if timeout elapsed.
	bool PopPoll(_Type &dest) {
		if (sysIpcPollSema(m_NonEmpty)) {
			m_Crit.Lock();
			if (++m_Tail == _Size)
				m_Tail = 0;
			dest = m_Q[m_Tail];
			--m_Count;
			m_Crit.Unlock();
			if (_blockOnFull)
				sysIpcSignalSema(m_NonFull);
			return true;
		}
		else
			return false;
	}

	// PURPOSE: Copies the bottom entry in the queue into dest. It does not remove it from the queue
	//		Returns failure code if nothing available
	// PARAMS: dest - Receives bottom entry in queue
	bool GetHead(_Type& dest) const {
		m_Crit.Lock();
		if(m_Count)
		{
			dest = m_Q[m_Head];
			m_Crit.Unlock();
			return true;
		}
		m_Crit.Unlock();
		return false;
	}

	// PURPOSE: Copies the top entry in the queue into dest. It does not remove it from the queue
	//		Returns failure code if nothing available
	// PARAMS: dest - Receives top entry in queue
	bool GetTail(_Type& dest) const {
		m_Crit.Lock();
		if(m_Count)
		{
			int tail = m_Tail+1;
			if(tail == _Size)
				tail = 0;
			dest = m_Q[tail];
			m_Crit.Unlock();
			return true;
		}
		m_Crit.Unlock();
		return false;
	}

	// PURPOSE: Returns true if there are currently no elements in the queue.
	bool IsEmpty() const {
		return m_Count == 0;
	}

	// PURPOSE: For DEBUGGING / ASSERTS ONLY - do not rely on this as it may change due to activity on other threads
#if !__FINAL
	bool IsNotFull() { return m_Count < _Size; }
#endif

private:
	_Type m_Q[_Size];			// Storage space for the queue object
	mutable sysCriticalSectionToken m_Crit;
	volatile int m_Head,		// Head of the queue; insertions happen at this point
		m_Tail,					// Tail of the queue; removals happen at the slot after this point
		m_Count;				// Number of items currently in the queue
	sysIpcSema m_NonEmpty;		// Semaphore which is signalled as long as the queue is nonempty
	sysIpcSema m_NonFull;		// Semaphore which is signalled as long as the queue is nonfull
};

}	// namespace rage

#endif
