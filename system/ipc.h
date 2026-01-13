//
// system/ipc.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_IPC_H
#define SYSTEM_IPC_H

#include "system/tls.h"
#include "profile/telemetry.h"

#if __WIN32
struct HANDLE__;
#endif

#define ENABLE_ALL_CALLSTACK_TRACE	(1 && !__FINAL)

#define ORBIS_USE_POSIX_SEMAPHORES  (0 && RSG_ORBIS)

namespace rage {

#if !__FINAL
#define CHECKPOINT_SYSTEM_MEMORY	::rage::CheckpointSystemMemory(__FILE__,__LINE__)
extern void CheckpointSystemMemory(const char *file,int line);
#else
#define CHECKPOINT_SYSTEM_MEMORY
#endif

#define		sysIpcInfinite			0xFFFFFFFF

// PURPOSE: A sysIpcSema is a cross platform semaphore object.
typedef struct sysIpcSemaTag * sysIpcSema;

#if !__FINAL
extern __THREAD void *st_stackTop;
extern __THREAD void *st_stackBottom;
#endif // !__FINAL

#if (USE_TELEMETRY) // Need to match Profile_Telemetry.h
#define sysIpcLockMutex(mutex) sysIpcLockMutexCommand(mutex, __FUNCTION__, __FILE__, __LINE__)
#define sysIpcTryLockMutex(mutex) sysIpcTryLockMutexCommand(mutex, __FUNCTION__, __FILE__, __LINE__)
#define TELEMETRY_ARG(x) x
#define TELEMETRY_ARGS(x ,y) , y
#else
#define sysIpcLockMutex(mutex) sysIpcLockMutexCommand(mutex)
#define sysIpcTryLockMutex(mutex) sysIpcTryLockMutexCommand(mutex)
#define TELEMETRY_ARG(x)
#define TELEMETRY_ARGS(x ,y)
#endif

// PURPOSE:	Create a semaphore object.
// PARAMS
//	initCount - true if the semaphore is created in the "ready to grab" state
//		otherwise false or some other positive integer
// RETURNS:	Semaphore object, or zero on failure.
// NOTES
//	<CODE>
//		// This:
//		sysIpcSema sema = ipcCreateSema(true);
//		ipcWaitSema(sema);
//		// is equivalent to this:
//		sysIpcSema sema = ipcCreateSema(false);
//</CODE>
sysIpcSema sysIpcCreateSema(int initCount);

// PURPOSE:	Poll a semaphore to see if it's ready
// PARAMS
//	sema - semaphore handle to check
// RETURNS
//	true if semaphore was available, and the semaphore is taken as a side effect
//	(in other words, you don't need to (and should not) call ipcWaitSema).
//	Returns false if the semaphore is not available.
bool sysIpcPollSema(sysIpcSema sema);

// PURPOSE
//	Wait for a semaphore to become available.  Blocks execution of
//	calling thread (allowing other threads to run) until the semaphore
//	is available.
// PARAMS
//	sema - semaphore handle to wait on
//  count - amount to decrease semaphore count by
void sysIpcWaitSema(sysIpcSema sema,unsigned count=1);

// PURPOSE
//	Wait for a semaphore to become available.  Blocks execution of
//	calling thread (allowing other threads to run) until the semaphore
//	is available or the timeout period in milliseconds elapses.
// PARAMS
//	sema - semaphore handle to wait on
//  milliseconds - timeout in milliseconds to wait, can also be sysIpcInfinite
// RETURNS:	true on success, or false if timeout period elapsed.
bool sysIpcWaitSemaTimed(sysIpcSema sema,unsigned milliseconds);

// PURPOSE:	Signal a semaphore so that somebody else can have it
// PARAMS
//	sema - semaphore handle to release
//  count - amount to increase semaphore count by
void sysIpcSignalSema(sysIpcSema sema,unsigned count=1);

// PURPOSE:	Delete a semaphore
// PARAMS
//	sema - semaphore handle to delete
void sysIpcDeleteSema(sysIpcSema sema);

typedef struct sysIpcMutexTag *sysIpcMutex;

// PURPOSE:	Create a mutex object.
// RETURNS:	Mutex object, or null on failure.
// NOTES:	Mutex is always created in an unlocked state.
//			Recursive locks (made by the same thread) are allowed.
//			The thread that locks a mutex is the only one that can unlock it.
sysIpcMutex sysIpcCreateMutex();

// PURPOSE:	Locks a mutex object.  Recursive locks (made by the same thread) are allowed.
// RETURNS:	True on success, false on failure (not locking thread, or null mutex)
bool sysIpcLockMutexCommand(sysIpcMutex mutex TELEMETRY_ARGS(, char const *str) TELEMETRY_ARGS(, char const* file) TELEMETRY_ARGS(, u32 line) );

// PURPOSE:	Polls a mutex object.  Recursive locks (made by the same thread) are allowed.
// RETURNS:	True on success (and mutex is locked), false on failure (unable to obtain mutex)
bool sysIpcTryLockMutexCommand(sysIpcMutex mutex TELEMETRY_ARGS(, char const *str) TELEMETRY_ARGS(, char const* file) TELEMETRY_ARGS(, u32 line) );

// PURPOSE:	Unlocks a mutex object.
// RETURNS:	True on success, false on failure (not locking thread, or null mutex)
bool sysIpcUnlockMutex(sysIpcMutex mutex);

// PURPOSE:	Deletes a mutex.
void sysIpcDeleteMutex(sysIpcMutex mutex);

/////////// T H R E A D S //////////

// PURPOSE: A cross-platform thread identifier (guaranteed globally unique)
// NOTE:	sysIpcThreadId is NOT the same size on all platforms.
#if __PPU
typedef u64 sysIpcThreadId;
typedef struct sysIpcCurrentThreadId__* sysIpcCurrentThreadId;
#else
typedef struct sysIpcThreadId__* sysIpcThreadId;
typedef struct sysIpcCurrentThreadId__* sysIpcCurrentThreadId;
#endif

typedef void (*sysIpcThreadFunc)(void *ptr);
#define DECLARE_THREAD_FUNC(x)		void x(void *ptr)
#define PRE_DECLARE_THREAD_FUNC(x)	void x(void *ptr);

enum sysIpcPriority {
	PRIO_IDLE = -15,				// Not recommended
	PRIO_LOWEST = -2,
	PRIO_BELOW_NORMAL = -1,
	PRIO_NORMAL = 0,
	PRIO_ABOVE_NORMAL = 1,
	PRIO_HIGHEST = 2,
	PRIO_TIME_CRITICAL = 15,		// Not recommended
	PRIO_MP4 = 60
};

// A callback that allows the title to override the thread priority and CPU affinity before it is created.
typedef void (*sysIpcThreadPriorityOverrideCb)(u32 threadNameHash, sysIpcPriority &priority, u32 &affinityMask);

// This is enforced by the OS on Xenon, so we make it the limit on all platforms for consistency.
const unsigned sysIpcMinThreadStackSize = RSG_ORBIS? 32768 : 16384;

// PURPOSE:	Creates a thread and makes it runnable.
// PARAMS
//	func - thread function; this function is run by the thread, and the
//	thread terminates when this function returns.
//	closure - user-supplied parameter sent to thread function
//	stackSize - size of stack; cannot be zero, and is clamped to sysIpcMinThreadStackSize
//	priority - thread priority, lower is more important
//  threadName - Name of the thread as seen in the debugger
//    cpu - start the thread on the specified CPU, if possible
// RETURNS
//	Thread handle object, or sysIpcThreadIdInvalid if the call failed.  You should always call
//  sysIpcWaitThreadExit on every valid thread handle at some point or they will leak handles.
//  This isn't an issue if the thread was only ever launched once.
sysIpcThreadId sysIpcCreateThread(sysIpcThreadFunc func,void *closure,unsigned stackSize,sysIpcPriority priority,const char *threadName,int cpu = 5, const char *threadTtyName = NULL);

// PURPOSE:	Creates a thread and makes it runnable.
// PARAMS
//	func - thread function; this function is run by the thread, and the
//	thread terminates when this function returns.
//	closure - user-supplied parameter sent to thread function
//	stackSize - size of stack; cannot be zero, and is clamped to sysIpcMinThreadStackSize
//	priority - thread priority, lower is more important
//  threadName - Name of the thread as seen in the debugger
//    cpuAffinityMask - mask of CPUs that the thread can run on
// RETURNS
//	Thread handle object, or sysIpcThreadIdInvalid if the call failed.  You should always call
//  sysIpcWaitThreadExit on every valid thread handle at some point or they will leak handles.
//  This isn't an issue if the thread was only ever launched once.
sysIpcThreadId sysIpcCreateThreadWithCoreMask(sysIpcThreadFunc func,void *closure,unsigned stackSize,sysIpcPriority priority,const char *threadName,u32 cpuAffinityMask = 0, const char *threadTtyName = NULL);

// For PC Final, remove thread names at the call site (the optimizer doesn't do it if it happens later)
// The ternary operator is used to avoid a warning if threadName is the sole use of a parameter of the calling function
#if RSG_FINAL && RSG_PC
#define sysIpcCreateThread(func,closure,stackSize,priority,threadName,...) sysIpcCreateThread(func,closure,stackSize,priority,true?"":threadName,__VA_ARGS__)
#endif

// PURPOSE:	Wait until a thread has exited.
bool sysIpcWaitThreadExit(sysIpcThreadId id);

// PURPOSE: Set a new callback that allows for overriding a thread's priority/affinity before it is created
void sysIpcSetThreadPriorityOverrideCb(sysIpcThreadPriorityOverrideCb cb);

#define sysIpcThreadIdInvalid           ((rage::sysIpcThreadId)-1)
#define sysIpcCurrentThreadIdInvalid    ((rage::sysIpcCurrentThreadId)-1)
extern __THREAD sysIpcCurrentThreadId g_CurrentThreadId;
#if !__NO_OUTPUT
extern __THREAD char g_CurrentThreadName[16];
#endif // !__NO_OUTPUT

// PURPOSE: Change which cpu a thread is running on.
// PARAMS:	id - Thread id
//			cpu - Which cpu (zero-based) to schedule the thread on
void sysIpcSetThreadProcessor(sysIpcThreadId id,int cpu);

#if RSG_DURANGO || RSG_ORBIS
// PURPOSE: Change which cpu's a thread is running on.
// PARAMS:	id - Thread id
//			cpuMask - Which cpu's to schedule the thread on
void sysIpcSetThreadProcessorMask(sysIpcThreadId id,u32 cpuMask);
#endif

#if RSG_ORBIS

#define sysIpcCpuInvalid				(-1)

// PURPOSE:	Get the cpu a thread is running on.
// PARAMS:	id - Thread id
// RETURNS:	cpu the given thread is running
int sysIpcGetThreadProcessor(sysIpcThreadId id);
#endif

// PURPOSE: Returns number of main cpu cores on this machine.
int sysIpcGetProcessorCount();

// PURPOSE:	Returns the thread ID of the thread that's currently executing.
// RETURNS: the thread ID of the thread that's currently executing.
sysIpcThreadId sysIpcGetThreadId();

bool sysIpcSetThreadPriority(sysIpcThreadId tid,sysIpcPriority prio);
sysIpcPriority sysIpcSetCurrentThreadPriority(sysIpcPriority newPrio);
sysIpcPriority sysIpcGetCurrentThreadPriority();

// PURPOSE:	Returns the thread that's currently executing.
// RETURNS: the thread that's currently executing.
// NOTES:	This is NOT in the same namespace as the other thread functions!
//			The reason for this is because we use thread handles everywhere else on Win32
__forceinline sysIpcCurrentThreadId sysIpcGetCurrentThreadId() { return g_CurrentThreadId; }

// PURPOSE:	Sets the thread id that's currently executing.
void sysIpcSetCurrentThreadId();

// PURPOSE:	Prints out the current state of all the threads and semaphores
// PARAMS: tag - The name to send to Printf as part of the dump
// NOTES
//	Only works on PS2 right now
void sysIpcDumpThreadSemaState(const char *tag);

// PURPOSE:	Yield CPU to another thread.
// PARAMS
//	prio - priority of thread queue to rotate (PS2 only)
// NOTES
//	Under __WIN32, this causes the current thread to give up
//	its time slice.  Under __PS2, the thread ready queue at
//	priority 'prio' it rotated to allow a like-priority thread
//	to run.  Generally 'prio' should match the priority of the
//	calling thread.
void sysIpcYield(int prio);

// PURPOSE:	Place calling thread to sleep for specified amount of time
// PARAMS
//	milliseconds - number of milliseconds to sleep
// NOTES
//	This function is preferable to a busy loop because it will
//	allow other threads to run.  The gamecube version only supports
//	a single thread sleeping at a time.
void sysIpcSleep(unsigned milliseconds);

// PURPOSE: Estimate amount of stack space used on current thread, and total stack space available.
// PARAMS
//  outCurrent - Current stack usage estimation, in bytes
//  outMax - Total stack size, in bytes
// RETURNS
//	True on success, false on failure
bool sysIpcEstimateStackUsage(unsigned &outCurrent,unsigned &outMax);

// PURPOSE: Check whether or not a given address is on the stack
bool sysIpcIsStackAddress(const void* ptr);

void sysIpcShowAllCallstacks();

void sysIpcTriggerAllCallstackDisplay();

#if __PS3
int sysIpcLoadModule(int moduleId);
int sysIpcUnloadModule(int moduleId);
#endif

#define ENABLE_STALL_CALLBACKS (!__FINAL && !__PROFILE && 0)

#if ENABLE_STALL_CALLBACKS
enum sysIpcStallReason {
	CRITSEC_BEGIN, CRITSEC_END,
	WAITSEMA_BEGIN, WAITSEMA_END,
	LOCKMUTEX_BEGIN, LOCKMUTEX_END
};

extern void (*sysIpcStallCallback)(sysIpcStallReason);
#else
#define sysIpcStallCallback(x) do { } while (0)
#endif

inline bool sysIpcIsStackAddress(const void* NOTFINAL_ONLY(ptr))
{
#if !__FINAL
	return ptr >= st_stackBottom && ptr < st_stackTop;
#else // !__FINAL
	return false;
#endif // !__FINAL
}

typedef struct sysIpcEventTag *sysIpcEvent;	// safer than a void*
sysIpcEvent sysIpcCreateEvent();
void sysIpcDeleteEvent(sysIpcEvent);
bool sysIpcPollEvent(sysIpcEvent);
bool sysIpcWaitEvent(sysIpcEvent);
void sysIpcSetEvent(sysIpcEvent);


// Function scope statics with a constructor (or an initializer expression that
// needs to be evaluated at runtime) are NOT threadsafe.
//
// Compilers will typicall implement function scope statics along the lines of
// the following pseudo code,
//
//     // static SomeClass s_object;
//
//     static bool inited = false;
//     static SomeClass s_object; // <- ctor not called here
//     if (!inited) {
//         s_object.ctor();
//         inited = true;
//     }
//
// The problem is that two threads can execute this same bit of code
// concurrently, and both see inited==false.
//
// The following macros provide a thread safe implementation; with one
// caveat... constructors should be very careful about locking any mutexes, to
// ensure deadlocking with g_ThreadSafeFunctionScopeStaticCS cannot occur.
//
// Note that Visual Studio 2014 handles this automatically with /Zc:threadSafeInit
// (http://support.microsoft.com/kb/2967191), so when we eventually update to that
// these macros can be changed.
//

#define THREAD_SAFE_FUNCTION_SCOPE_STATIC(OPTIONAL_CONST, TYPE, NAME, INIT)    \
	static volatile bool NAME##_inited/*=false*/;                              \
	/* ALIGNAS(__alignof(TYPE)) fails to compile, so instead just allocate  */ \
	/* enough extra bytes in the array so that we can do the alignment      */ \
	/* manually.                                                            */ \
	static char NAME##_storage[sizeof(TYPE)+__alignof(TYPE)-1];                \
	OPTIONAL_CONST TYPE &NAME = *(TYPE*)(                                      \
		((uptr)NAME##_storage+__alignof(TYPE)-1)&~(__alignof(TYPE)-1));        \
	/* This initial check on NAME##_inited is not here for correctness, it  */ \
	/* is simply an optimization for the common case.  The second recheck   */ \
	/* _after_ g_ThreadSafeFunctionScopeStaticCS is locked is the check for */ \
	/* correctness.                                                         */ \
	if (Unlikely(!NAME##_inited)) {                                            \
		SYS_CS_SYNC(g_ThreadSafeFunctionScopeStaticCS);                        \
		/* Must recheck NAME##_inited here, since two threads may initially */ \
		/* see NAME##_inited==false before one of the runs the ctor.        */ \
		if (!NAME##_inited) {                                                  \
			rage_placement_new ((void*)&NAME) TYPE INIT;                       \
			/* Despite locking g_ThreadSafeFunctionScopeStaticCS, still use */ \
			/* a write barrier here, since another thread may check         */ \
			/* NAME##_inited without acquiring the mutex.                   */ \
			/*                                                              */ \
			/* This barrier is probably overkill for x64 platforms, which   */ \
			/* do limited re-orderring compared to some other platforms.    */ \
			/* But rather than remove the barrier, it would be nicer to     */ \
			/* change the barrier function to be a nop.  This code is not   */ \
			/* performance critical though, so doesn't really matter here.  */ \
			/*                                                              */ \
			sysMemWriteBarrier();                                              \
			NAME##_inited = true;                                              \
		}                                                                      \
	}                                                                          \
	do{}while(0) /* eat semicolon */

#define THREAD_SAFE_FUNCTION_SCOPE_STATIC_ARRAY(OPTIONAL_CONST, TYPE, NAME, ARRAY_LEN) \
	static volatile bool NAME##_inited/*=false*/;                              \
	static char NAME##_storage[sizeof(TYPE)*ARRAY_LEN+__alignof(TYPE)-1];      \
	OPTIONAL_CONST TYPE *const NAME = (TYPE*)(                                 \
		((uptr)NAME##_storage+__alignof(TYPE)-1)&~(__alignof(TYPE)-1));        \
	if (Unlikely(!NAME##_inited)) {                                            \
		SYS_CS_SYNC(g_ThreadSafeFunctionScopeStaticCS);                        \
		if (!NAME##_inited) {                                                  \
			rage_placement_new ((void*)NAME) TYPE[ARRAY_LEN];                  \
			sysMemWriteBarrier();                                              \
			NAME##_inited = true;                                              \
		}                                                                      \
	}                                                                          \
	do{}while(0)

class sysCriticalSectionToken;
extern sysCriticalSectionToken g_ThreadSafeFunctionScopeStaticCS;



} // namespace rage
#endif // SYSTEM_IPC_H
