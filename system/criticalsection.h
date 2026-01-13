// 
// system/criticalsection.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_CRITICALSECTION_H
#define SYSTEM_CRITICALSECTION_H

#include "system/ipc.h"
#include "system/spinlock.h"

#include <stddef.h>

#define INLINE_CRITSEC		((__PPU || __PSP2) && (__PROFILE || __FINAL))

#if INLINE_CRITSEC
#if __PPU
#include <sys/synchronization.h>
#elif __PSP2
#include <kernel.h>
#endif
#endif

#if RSG_ORBIS
struct pthread_mutex;	// must be outside rage namespace
#endif

#if !__FINAL
extern int sysIpcCritSecDisableTimeout;
#endif

namespace rage {

#if __PS3
#define CRITSEC_PTR ((sys_lwmutex_t*)this)
#elif __PSP2
#define CRITSEC_PTR ((SceKernelLwMutexWork*)this)
#elif RSG_ORBIS
#define CRITSEC_PTR (&m_Impl)
#else
#define CRITSEC_PTR ((CRITICAL_SECTION*)this)
#endif

#define USE_TELEMETRY_CRITICAL 0

// Win32 version:

/* typedef struct _RTL_CRITICAL_SECTION {
	PRTL_CRITICAL_SECTION_DEBUG DebugInfo;

	//
	//  The following three fields control entering and exiting the critical
	//  section for the resource
	//

	LONG LockCount;
	LONG RecursionCount;
	HANDLE OwningThread;        // from the thread's ClientId->UniqueThread
	HANDLE LockSemaphore;
	ULONG_PTR SpinCount;        // force size on 64-bit systems when packed
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION; */

// Xenon version:

/* typedef struct _RTL_CRITICAL_SECTION {

	//
	//  The following field is used for blocking when there is contention for
	//  the resource
	//

	union {
		ULONG_PTR RawEvent[4];
	} Synchronization;

	//
	//  The following three fields control entering and exiting the critical
	//  section for the resource
	//

	LONG LockCount;
	LONG RecursionCount;
	HANDLE OwningThread;
} RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION; */

// PURPOSE:
//  Use one of these to assist with sysCriticalSection. It should be statically allocated in
//	the function that uses a critical section.
class sysCriticalSectionToken {
	friend class sysCriticalSection;
	sysCriticalSectionToken(const sysCriticalSectionToken&);			// Copying would fail spectacularly
	sysCriticalSectionToken& operator=(const sysCriticalSectionToken&);	// Copying would fail spectacularly
public:
	// PURPOSE: Create a new critical section token
	sysCriticalSectionToken(int spinCount = 1000);

	// PURPOSE: Clean up a critical section token before it is destroyed
	~sysCriticalSectionToken();

#if INLINE_CRITSEC
#if __PS3
	void Lock() {
		sys_lwmutex_lock(CRITSEC_PTR,0);
	}
	bool TryLock() {
		return sys_lwmutex_trylock(CRITSEC_PTR) == CELL_OK;
	}
	void Unlock() {
		sys_lwmutex_unlock(CRITSEC_PTR);
	}
#else
	void Lock() {
		sceKernelLockLwMutex(CRITSEC_PTR,1,0);
	}
	bool TryLock() {
		return sceKernelTryLockLwMutex(CRITSEC_PTR,1) == 0;
	}
	bool Unlock() {
		sceKernelUnlockLwMutex(CRITSEC_PTR,1);
	}
#endif
#else
	void Lock();
	bool TryLock();
	void Unlock();
#endif

protected:
#if RSG_ORBIS
	// struct pthread_mutex
	// typedef struct	pthread_mutex		*pthread_mutex_t;
	// typedef pthread_mutex_t           ScePthreadMutex;
	pthread_mutex *m_Impl;
#elif __64BIT
	u64 m_Impl[5];
#elif __WIN32
	u64 m_Impl[4];
#elif __PS3
	u64 m_Impl[3];
#endif
};


// PURPOSE:
//  A critical section is for controlling access to a function block that uses shared memory and can be called
//	from multiple threads.
//	NOTES:
//	The critical section will automatically lock & release the critical section on construction & 
//	destruction, but you can override it's functionality via the Enter & Exit methods.
//	When using, make sure the CriticalSectionToken is visible to all threads that need to enter 
//  the critical section (e.g. not on the stack or in thread-local storage)
//	Example:
//	<CODE>
//	static sysCriticalSectionToken	m;				// Create the shared token at file (or class) scope.
//	void MyFunc() {
//		sysCriticalSection cs(m);					// Create a critical section to control the mutex
//		GlobalVal++;								// Or whatever you want to do to global data goes here
//	}
//	</CODE>
class sysCriticalSection {
public:
	// PURPOSE: Create a new critical section object, block until the critical section token is unused
	// PARAMS: m - The token to block on
	// NOTES: Automatically calls Enter to enter the critical section when the object is created.
	sysCriticalSection(sysCriticalSectionToken &m) : m_LockCount(0), m_Token(m) {
		Enter();
	}

	sysCriticalSection(sysCriticalSectionToken &m, bool autoLock) : m_LockCount(0), m_Token(m) {
		if ( autoLock )
			Enter();
	}

	// PURPOSE: Releases the critical section token
	// NOTES: Automatically calls Exit to exit the critical section.
	~sysCriticalSection() {
		Assert(m_LockCount == 0 || m_LockCount == 1);
		if ( m_LockCount )
			Exit();
	}

	// PURPOSE: Enter a critical section.
	// NOTES:
	//	This is called automatically by the constructor, but may be called again if
	//	the critical section was exited and needs to be reentered.
	void Enter() {
		if ( ++m_LockCount == 1 )
			m_Token.Lock();
	}

	// PURPOSE: Attempt to enter a critical section.
	// NOTES:
	//  Returns true if this call acquired the lock, or the lock
	//  is already acquired by this sysCriticalSection object.
	bool TryEnter() {
		if (++m_LockCount == 1)
		{
			if (!m_Token.TryLock())
			{
				--m_LockCount;
				return false;
			}
		}
		return true;
	}

	// PURPOSE: Exit a critical section
	// NOTES:
	//	This is called automatically by the destructor, but may be called before
	//	destruction 
	void Exit() {
		Assert(m_LockCount);
		if ( --m_LockCount == 0)
			m_Token.Unlock();
	}

	// PURPOSE: Determine whether this section is locked by any thread.
	// RETURN: True if this section is locked, false otherwise
	bool IsLocked() const { 
		return m_LockCount != 0; 
	}

protected:
	int							m_LockCount;
	sysCriticalSectionToken		&m_Token;
};

int sysCriticalSectionGetLockCount();

}	// namespace rage

//PURPOSE
//  Convenience macros to synchronize a section of code.
//EXAMPLES
//
//sysCriticalSectionToken s_CS;
//
//void Foo()
//{
//    SYS_CS_SYNC(s_CS);      //Synchronizes the entire function
//}
//
//void Bar(const bool b)
//{
//    if(b)
//    {
//        SYS_CS_SYNC(s_CS);  //Synchronizes the code up to the next }
//    }
//}
//NOTES
// Using a fixed symbol name is intentional; if you're trying to lock two
// different critical sections in the same scope (or the same one twice)
// you're very likely asking for deadlocks.
#define SYS_CS_SYNC(cs) rage::sysCriticalSection __cssync(cs)

//PURPOSE
//  Convenience macros to synchronize a section of code based on a condition.
//EXAMPLES
//
//sysCriticalSectionToken s_CS;
//
//bool bCondition; //could be function/variable
//void Foo()
//{
//    SYS_CS_SYNC_CONDITIONAL(s_CS, bCondition);      //Synchronizes the entire function based on the condition
//}
//
//void Bar(const bool b)
//{
//    if(b)
//    {
//        SYS_CS_SYNC_CONDITIONAL(s_CS);  //Synchronizes the code based on the condition up to the next }
//    }
//}
//NOTES
// Using a fixed symbol name is intentional; if you're trying to lock two
// different critical sections in the same scope (or the same one twice)
// you're very likely asking for deadlocks.
#define SYS_CS_SYNC_CONDITIONAL(cs, condition) rage::sysCriticalSection __cssync(cs, false); if(condition){__cssync.Enter();}

#endif
