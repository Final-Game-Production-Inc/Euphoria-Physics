//
// system/ipc.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#if RSG_DURANGO
// needs to come before the _WIN32_WINNT check
#include "xtl.h"
#endif


#include "ipc.h"
#include "memory.h"
#include "new.h"
#include "param.h"

#include "xtl.h"

#include "data/marker.h"
#include "diag/output.h"
#include "diag/diagerrorcodes.h"
#include "file/device.h"		// for g_HadFatalError
#include "string/string.h"
#include "criticalsection.h"
#include "system/nelem.h"

#include <stdarg.h>
#include <stdio.h>

#if __WIN32PC
#  pragma comment(lib, "winmm.lib")
#include <xmmintrin.h>
#elif __XENON
#include <ppcintrinsics.h>
#include "system/param.h"
#include "system/bootmgr.h"
#if !__FINAL
#include <xbdm.h>
#endif
#elif __PPU | __POSIX
#include <errno.h>
#include <netex/net.h>
#include <semaphore.h>
#include <cell/sysmodule.h>
#include <sys/synchronization.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/dbg.h>
#include <sys/sys_time.h>
#include <sys/memory.h>
#include <time.h>		// for timespec
#include <altivec.h>
#pragma comment(lib, "pthread")
#elif RSG_ORBIS
#include <semaphore.h>
#include <sys/errno.h>
#include <kernel.h>
#if SCE_ORBIS_SDK_VERSION >= (0x00930020u)
#pragma comment(lib,"ScePosix_stub_weak")
#endif
#elif RSG_DURANGO
#elif __PSP2 || RSG_ORBIS
#include <kernel.h>
#endif
#include "atl/staticpool.h"
#include "diag/tracker.h"
#include "system/spinlock.h"
#include "system/threadregistry.h"
#include "system/timer.h"
#include "system/tmcommands.h"

// Sony's priorities are opposite of Win32 (lower numbrer = higher priority)
#if RSG_PS3
const int kSonyBasePrio = 2048;
#elif RSG_ORBIS
const int kSonyBasePrio = 700;
#endif

#if RAGE_USE_DEJA	// Duplicate the libs here in case code never used marker code.
#if __XENON
#pragma comment(lib,"DejaLib.X360.lib")
#elif __WIN32PC
#pragma comment(lib,"DejaLib.Win32.lib")
#elif __PS3
#pragma comment(lib,"DejaLib.PS3")
#endif
#endif

#if ENABLE_ALL_CALLSTACK_TRACE
#include "system/stack.h"
#if __PPU
bool gDumpAllThreadCallstacks;
#endif
#endif

#if (RSG_PC || RSG_DURANGO) && !__RESOURCECOMPILER && !__TOOL
PARAM(useSystemThreadManagement,"OS Determines thread affinity");
NOSTRIP_PC_PARAM(setSystemProcessorAffinity,"Set a global system affinity for all threads");
PARAM(disablePreferredProcessor,"Allow us to tell the OS which is the preferred processor per-thread");
#endif // RSG_PC

void (*sysExceptionHandledCallback)() = NULL;

// Disable denormals, which can hurt performance in rare cases.
void ipcDisableDenormals()
{
#if __WIN32PC
	// http://software.intel.com/en-us/articles/x87-and-sse-floating-point-assists-in-ia-32-flush-to-zero-ftz-and-denormals-are-zero-daz
	// Diff between FTZ and DAZ: http://labcalc.phys.uniroma1.it/home/fortran/doc/main_for/mergedProjects/fpops_for/common/fpops_set_ftz_daz.htm
	int oldVals = _mm_getcsr();
	int newVals = oldVals;
	// Set flush-to-zero mode
	newVals |= _MM_FLUSH_ZERO_ON | _MM_MASK_UNDERFLOW;
	// Set denormals-as-zero mode (SSE2 and higher, except for some early Pentium 4 models)
	newVals |= _MM_MASK_DENORM /*| 0x0040*/; // 2nd flag irrelevant
	_mm_setcsr( newVals );
#elif __XENON
	// "Denormals already disabled on VMX128."
	// -- Cristi, MS Game Developer Support
#elif __SPU
	// "Denormals flushed to 0."
	// -- http://www.research.ibm.com/cell/SPU.html
#elif __PPU
	// Seem to be disabled by default, but just in case.
	vector unsigned short currentStatus = vec_mfvscr();
	vector unsigned short orBit = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0 };
	currentStatus = vec_or( currentStatus, orBit );
	vec_mtvscr( currentStatus );
#endif
}


namespace rage {

extern __THREAD int g_DisableInitNan;

#if !__FINAL
	__THREAD void *st_stackTop;
	__THREAD void *st_stackBottom;
#endif // !__FINAL

static sysIpcThreadPriorityOverrideCb s_ThreadPriorityOverrideCb;

sysCriticalSectionToken g_ThreadSafeFunctionScopeStaticCS;

#if RSG_DURANGO || RSG_ORBIS || __NO_OUTPUT || __WIN32PC
void CheckpointSystemMemory(const char *,int ) { }	// GlobalMemoryStatus not available on Metro.
#else
void CheckpointSystemMemory(const char *file,int line) {
#if __WIN32PC || __XENON
	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	size_t remain = ms.dwAvailPhys >> 10;
#elif __PS3
	sys_memory_info_t mem_info;
	sys_memory_get_user_memory_size( &mem_info );
	size_t remain = mem_info.available_user_memory >> 10;
#elif __PSP2
	size_t remain = 0;
#endif
	static const char *lastFile;
	static int lastLine;
	static size_t lastRemain;
	if (remain != lastRemain) {
		printf("[IPC] CheckpointSystemMemory: Remaining was %uk at (%s,%d), now %uk at (%s,%d)\n",
			lastRemain,lastFile,lastLine,
			remain,file,line);
		if (lastRemain)
			printf("[IPC] --- Delta is %dk bytes.\n",remain-lastRemain);
		lastRemain = remain;
	}
	lastFile = file;
	lastLine = line;
}
#endif

__THREAD sysIpcCurrentThreadId g_CurrentThreadId = sysIpcCurrentThreadIdInvalid;

#if __XENON || __PPU || __PSP2
unsigned g_ThreadStackSize;
#endif

#define ORBIS_USE_SCE_SEMAPHORES            (!ORBIS_USE_POSIX_SEMAPHORES && RSG_ORBIS)

#if __PPU || __POSIX || ORBIS_USE_POSIX_SEMAPHORES
// Note that on PS3, this does not consume any OS handles, and is not subject to that 8192 count limit.
static atStaticPool<sem_t,1024> s_Sema ;			// sizeof(sem_t) == 4
static sysSpinLockToken semaToken;
#endif
#if __PPU || __POSIX
static atStaticPool<sys_lwmutex_t,128> s_Mutex ;	// sizeof(sys_lwmutex_t) == 24
static sysSpinLockToken mutexToken;
#endif

sysIpcSema sysIpcCreateSema(int initCount) {
#if RSG_DURANGO
	return (sysIpcSema)CreateSemaphoreExW(NULL, initCount, 32767, NULL, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);
#elif __WIN32
	return (sysIpcSema)CreateSemaphore(NULL, initCount, 32767, NULL);
#elif __PPU || __POSIX || ORBIS_USE_POSIX_SEMAPHORES
	sysSpinLock cs(semaToken);
	sem_t *t = s_Sema.IsFull()? NULL : s_Sema.New();
	if (t) {
		if (sem_init(t,false,initCount)==0) {
			return (sysIpcSema) t;
		}
		else
			s_Sema.Delete(t);
	}
	else
		Errorf("*** sysIpcCreateSema failed, too many semaphores!");
	return NULL;
#elif ORBIS_USE_SCE_SEMAPHORES
	SceKernelSema sema;
	int err = sceKernelCreateSema(&sema,"sema",SCE_KERNEL_SEMA_ATTR_TH_FIFO,initCount,32767,NULL);
	if (err) Errorf("sysIpcCreateSema returned %x",err);
	return err? 0 : (sysIpcSema) ((size_t)sema+1);
#elif __PSP2
	return (sysIpcSema) sceKernelCreateSema("sema",SCE_KERNEL_SEMA_ATTR_TH_FIFO,initCount,32767,NULL);
#endif
}


bool sysIpcPollSema(sysIpcSema sema) {
	Assert(sema);
#if __WIN32
	return WaitForSingleObject((HANDLE) sema,0) == WAIT_OBJECT_0;
#elif __PPU || __POSIX || ORBIS_USE_POSIX_SEMAPHORES
	return sem_trywait((sem_t*)sema) == 0;
#elif ORBIS_USE_SCE_SEMAPHORES
	return sceKernelPollSema((SceKernelSema)((size_t)sema-1),1) == 0;
#elif __PSP2
	return sceKernelPollSema((SceUID)sema,1) == 0;
#endif
}



void sysIpcWaitSema(sysIpcSema sema,unsigned count) {
	Assert(sema);
	Assert(count);

	sysIpcStallCallback(WAITSEMA_BEGIN);

#	if __WIN32
		DWORD result;
		do {
			result = WaitForSingleObject((HANDLE) sema,INFINITE);
#			if !__FINAL
				if(result != WAIT_OBJECT_0)
					Quitf("sysIpcWaitSema: WaitForSingleObject returned unexpected value 0x%08x",result);
#			endif
		} while (--count);

#	elif ORBIS_USE_POSIX_SEMAPHORES && 1
		// Temporary workaround for bug in sem_wait where caller may never get woken up.
		// https://ps4.scedev.net/support/issue/35767/

		// 1/13/15 - cthomas - Since sceKernelGettimeofday() appears to be fairly expensive, 
		// We'll do a sem_trywait() first and attempt to avoid it. Most common case by far 
		// is simply count == 1, so we'll keep it simple and only attempt this once (dec'ing 
		// count if successful).
		int result = sem_trywait((sem_t*)sema);
		if (result == 0)
		{
			count--;
		}

		if (count > 0)
		{
			static const unsigned microseconds = 100 * 1000;
			struct timeval tv;
			sceKernelGettimeofday(&tv);
			tv.tv_usec += microseconds;
			struct timespec ts = { tv.tv_sec + (tv.tv_usec/1000000), (tv.tv_usec % 1000000) * 1000 };

			do {
				for (;;) {

					result = sem_timedwait((sem_t*)sema,&ts);
					if (result == 0)
						break;

#				if !__FINAL
					const int e = errno;
					if (result != -1 || (e != ETIMEDOUT && e != EINTR))
						Quitf("sysIpcWaitSema: sem_timedwait returned 0x%08x, errno 0x%08x",result,e);
#				endif

					tv.tv_usec += microseconds;
					ts.tv_sec  = tv.tv_sec + (tv.tv_usec/1000000);
					ts.tv_nsec = (tv.tv_usec % 1000000) * 1000;
				}
			} while (--count);
		}

#	elif __PPU || __POSIX || ORBIS_USE_POSIX_SEMAPHORES
		int result;
		do {
			while ((result = sem_wait((sem_t*)sema)) != 0) {
#				if !__FINAL
					if (result != -1 || errno != EINTR)
						Quitf("sysIpcWaitSema: sem_wait returned 0x%08x, errno 0x%08x",result,errno);
#				endif
			}
		} while (--count);

#	elif ORBIS_USE_SCE_SEMAPHORES
		int result = sceKernelWaitSema((SceKernelSema)((size_t)sema-1),count,NULL);
#		if !__FINAL
			if (result != 0)
				Quitf("sysIpcWaitSema: sceKernelWaitSema returned unexpected value 0x%08x",result);
#		endif

#	elif __PSP2
		int result = sceKernelWaitSema((SceUID)sema,count,NULL);
#		if !__FINAL
			if (result != 0)
				Quitf("sysIpcWaitSema: sceKernelWaitSema returned unexpected value 0x%08x",result);
#		endif
#	endif

	sysIpcStallCallback(WAITSEMA_END);
	(void) result;
}

bool sysIpcWaitSemaTimed(sysIpcSema sema,unsigned milliseconds) {
	Assert(sema);

#	if __WIN32
		sysIpcStallCallback(WAITSEMA_BEGIN);
		DWORD result = WaitForSingleObject((HANDLE) sema,milliseconds);
		bool locked = (result == WAIT_OBJECT_0);
#		if !__FINAL
			if (!locked && result != WAIT_TIMEOUT)
				Quitf("sysIpcWaitSemaTimed: WaitForSingleObject returned unexpected value 0x%08x",result);
#		endif
		sysIpcStallCallback(WAITSEMA_END);
		return locked;

#	elif __PPU || __POSIX || ORBIS_USE_POSIX_SEMAPHORES
		if (milliseconds == sysIpcInfinite) {
			sysIpcWaitSema(sema);
			return true;
		}
		else {
			sysIpcStallCallback(WAITSEMA_BEGIN);

#			if RSG_ORBIS
				struct timeval tv;
				sceKernelGettimeofday(&tv);
				tv.tv_usec += milliseconds * 1000;
				struct timespec ts = { tv.tv_sec + (tv.tv_usec/1000000), (tv.tv_usec % 1000000) * 1000 };
#			else
				sys_time_sec_t sec;
				sys_time_nsec_t nsec;
				sys_time_get_current_time(&sec,&nsec);
				nsec += (sys_time_nsec_t)milliseconds * 1000000;
				std::timespec ts = { (int) (sec + (nsec/1000000000LL)), (int) (nsec % 1000000000LL) };
#			endif

			bool locked = true;
			for (;;) {
				int result = sem_timedwait((sem_t*)sema,&ts);
				if (result == 0)
					break;
				if (result == -1 && errno == ETIMEDOUT) {
					locked = false;
					break;
				}
#				if !__FINAL
					if (result != -1 || errno != EINTR)
						Quitf("sysIpcWaitSemaTimed: sem_timedwait returned 0x%08x, errno 0x%08x",result,errno);
#				endif
			}

			sysIpcStallCallback(WAITSEMA_END);
			return locked;
		}

#	elif ORBIS_USE_SCE_SEMAPHORES
		if (milliseconds == sysIpcInfinite) {
			sysIpcWaitSema(sema);
			return true;
		}
		else {
			sysIpcStallCallback(WAITSEMA_BEGIN);
			SceKernelUseconds timeout = milliseconds * 1000;
			int result = sceKernelWaitSema((SceKernelSema)((size_t)sema-1),1,&timeout);
			bool locked = (result == 0);
#			if !__FINAL
				if (!locked && result != SCE_KERNEL_ERROR_ETIMEDOUT)
					Quitf("sysIpcWaitSema: sceKernelWaitSema returned unexpected value 0x%08x",result);
#			endif
			sysIpcStallCallback(WAITSEMA_END);
			return locked;
		}

#	elif __PSP2
		if (milliseconds == sysIpcInfinite) {
			sysIpcWaitSema(sema);
			return true;
		}
		else {
			SceUInt32 timeout = milliseconds * 1000;
			sysIpcStallCallback(WAITSEMA_BEGIN);
			int result = sceKernelWaitSema((SceUID)sema,1,&timeout);
			bool locked = (result == 0);
#			if !__FINAL
				if (!locked && result != SCE_KERNEL_ERROR_ETIMEDOUT)
					Quitf("sysIpcWaitSema: sceKernelWaitSema returned unexpected value 0x%08x",result);
#			endif
			sysIpcStallCallback(WAITSEMA_END);
			return locked;
		}
#	endif
}

void sysIpcSignalSema(sysIpcSema sema,unsigned count) {
	Assert(sema);
	Assert(count);

#	if __WIN32
		BOOL result = ReleaseSemaphore((HANDLE)sema, count, NULL);
#		if !__FINAL
			if (!result)
			{
				int err = GetLastError();
#				if __WIN32PC
					char errorBuff[128];
					Quitf("sysIpcSignalSema: ReleaseSemaphore failed. GetLastError() returns %d: %s",
						err, diagErrorCodes::Win32ErrorToString(err, errorBuff, NELEM(errorBuff)));
#				else
					Quitf("sysIpcSignalSema: ReleaseSemaphore failed. GetLastError() returns %d", err);
#				endif
			}
#		endif

#	elif __PPU || __POSIX || ORBIS_USE_POSIX_SEMAPHORES
		int result;
		do {
			result = sem_post((sem_t*)sema);
#			if !__FINAL
				if (result != 0)
					Quitf("sysIpcSignalSema: sem_post returned unsigned value 0x%08x",result);
#			endif
		} while (--count);

#	elif ORBIS_USE_SCE_SEMAPHORES
		int result = sceKernelSignalSema((SceKernelSema)((size_t)sema-1),count);
#		if !__FINAL
			if (result != 0)
				Quitf("sysIpcSignalSema: sceKernelSignalSema returned unsigned value 0x%08x",result);
#		endif

#	elif __PSP2
		int result = sceKernelSignalSema((SceUID)sema,count);
#		if !__FINAL
			if (result != 0)
				Quitf("sysIpcSignalSema: sceKernelSignalSema returned unsigned value 0x%08x",result);
#		endif
#	endif

	(void) result;
}

void sysIpcDeleteSema(sysIpcSema sema) {
	Assert(sema);
#if __WIN32
	CloseHandle((HANDLE) sema);
#elif __PPU || __POSIX || ORBIS_USE_POSIX_SEMAPHORES
	sysSpinLock cs(semaToken);
	sem_t* t= (sem_t*)sema;
	sem_destroy(t);
	s_Sema.Delete(t);
#elif ORBIS_USE_SCE_SEMAPHORES
	sceKernelDeleteSema((SceKernelSema)((size_t)sema-1));
#elif __PSP2
	sceKernelDeleteSema((SceUID)sema);
#endif
}

sysIpcMutex sysIpcCreateMutex() {
#if __WIN32
	return (sysIpcMutex)CreateMutex(NULL, FALSE, NULL);
#elif __PPU | __POSIX
	sysSpinLock cs(mutexToken);
	sys_lwmutex_t *t = s_Mutex.IsFull()? NULL : s_Mutex.New();
	sys_lwmutex_attribute_t attr = { SYS_SYNC_FIFO, SYS_SYNC_RECURSIVE, "mutex" };
	if (t) {
		if (sys_lwmutex_create(t, &attr)==CELL_OK)
			return (sysIpcMutex) t;
		else
			s_Mutex.Delete(t);	
	}
	else
		Errorf("*** sysIpcCreateMutex failed, too many mutexes!");
	return NULL;
#elif RSG_ORBIS
	static ScePthreadMutexattr attr;
	if (!attr) {
		scePthreadMutexattrInit(&attr);
		scePthreadMutexattrSettype(&attr, SCE_PTHREAD_MUTEX_RECURSIVE);
	}
	ScePthreadMutex m;
	scePthreadMutexInit(&m, &attr, "mutex");
	return (sysIpcMutex)m;
#elif __PSP2
	// TODO: Use lightweight mutex here instead?  Critsecs already do that, and
	// they are the primary consumer.
	return (sysIpcMutex) sceKernelCreateMutex("mutex", 
		SCE_KERNEL_MUTEX_ATTR_TH_FIFO | SCE_KERNEL_MUTEX_ATTR_RECURSIVE,
		1, NULL);
#endif
}


bool sysIpcLockMutexCommand(sysIpcMutex mutex TELEMETRY_ARGS(, char const *str) TELEMETRY_ARGS(, char const* file) TELEMETRY_ARGS(, u32 line)) {
	Assert(mutex);
	if (!mutex) return false;
	bool result;
	sysIpcStallCallback(LOCKMUTEX_BEGIN);

	TELEMETRY_LOCK(mutex, file, line, str);
	TELEMETRY_TRY_LOCK(mutex, file, line, str);

#if __WIN32
	result = WaitForSingleObject((HANDLE) mutex,INFINITE) == WAIT_OBJECT_0;
#elif __PPU
# if !__FINAL
	if (sys_lwmutex_lock((sys_lwmutex_t*)mutex,1000*1000) == ETIMEDOUT) {
		while (sys_lwmutex_trylock((sys_lwmutex_t*)mutex) != CELL_OK)
			sys_timer_usleep(100);
	}
	result = true;
# else
	result = sys_lwmutex_lock((sys_lwmutex_t*)mutex,0) == CELL_OK;
# endif
#elif RSG_ORBIS
	result = scePthreadMutexLock((ScePthreadMutex*)&mutex) == 0;
#elif __PSP2
	result = sceKernelLockMutex((SceUID)mutex,1,NULL) == 0;
#endif
	sysIpcStallCallback(LOCKMUTEX_END);
	TELEMETRY_END_TRY_LOCK(result ? LRT_SUCCESS : LRT_FAILED, mutex, file, line);
	return result;
}

bool sysIpcTryLockMutexCommand(sysIpcMutex mutex TELEMETRY_ARGS(, char const *str) TELEMETRY_ARGS(, char const* file) TELEMETRY_ARGS(, u32 line)) 
{
	Assert(mutex);
	if (!mutex) return false;

	TELEMETRY_TRY_LOCK(mutex, file, line, str);
#if __WIN32
	bool bRet = WaitForSingleObject((HANDLE) mutex,0) == WAIT_OBJECT_0;
#elif __PPU
	bool bRet = sys_lwmutex_trylock((sys_lwmutex_t*)mutex) == CELL_OK;
#elif RSG_ORBIS
	bool bRet = scePthreadMutexTrylock((ScePthreadMutex*)&mutex) == 0;
#elif __PSP2
	bool bRet = sceKernelTryLockMutex((SceUID)mutex,1) == 0;
#endif
	TELEMETRY_END_TRY_LOCK(bRet ? LRT_SUCCESS : LRT_FAILED, mutex, file, line);
#if USE_TELEMETRY
	if (bRet) 
	{
		TELEMETRY_LOCK(mutex, file, line, str);
	}
#endif // USE_TELEMETRY
	return bRet;
}


bool sysIpcUnlockMutex(sysIpcMutex mutex) {
	Assert(mutex);
	if (!mutex) return false;

	TELEMETRY_UNLOCK(mutex, __FILE__, __LINE__, __FUNCTION__);
#if __WIN32
	return ReleaseMutex((HANDLE)mutex) != 0;
#elif __PPU
	return sys_lwmutex_unlock((sys_lwmutex_t*)mutex) == CELL_OK;
#elif RSG_ORBIS
	return scePthreadMutexUnlock((ScePthreadMutex*)&mutex) == 0;
#elif __PSP2
	return sceKernelUnlockMutex((SceUID)mutex,1) == 0;
#endif
}

void sysIpcDeleteMutex(sysIpcMutex mutex) {
	Assert(mutex);
	if (!mutex) return;
#if __WIN32
	CloseHandle((HANDLE) mutex);
#elif __PPU | __POSIX
	sysSpinLock cs(mutexToken);
	sys_lwmutex_t* t= (sys_lwmutex_t*)mutex;
	sys_lwmutex_destroy(t);
	s_Mutex.Delete(t);
#elif __PSP2
	sceKernelDeleteMutex((SceUID)mutex);
#endif
}

#if __WIN32PC
// ipcTime calls timeGetTime.  Many programs never actually use ipcTime, which causes a linker warning on WINMM.LIB.
// But if we remove the pragma, those same programs don't link.  Solution -- put a dummy call in elsewhere.
static struct winmm_hack_t {
	winmm_hack_t() {
		timeGetTime();
	}
} winmm_hack;
#endif


#if __WIN32 && !__FINAL

#if __XENON
PARAM(supportwatson,"[system] Use settings that don't cause problems for watson");
#endif // __XENON

#if !__NO_OUTPUT
void SetThreadName(const char *name) {
#if __XENON
	int supportWatson = false;
	if ( PARAM_supportwatson.Get() ) 
	{
		PARAM_supportwatson.GetDebug(supportWatson);
	}
	if (supportWatson)
	{
		return;
	}
#endif // __XENON

	// See "SetThreadName" in the Xenon help for discussion.
	typedef struct tagTHREADNAME_INFO {
		DWORD dwType;     // Must be 0x1000
		LPCSTR szName;    // Pointer to name (in user address space)
		DWORD dwThreadID; // Thread ID (-1 for caller thread)
		DWORD dwFlags;    // Reserved for future use; must be zero
	} THREADNAME_INFO;

	THREADNAME_INFO info;

	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = (DWORD)-1;
	info.dwFlags = 0;

	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(ULONG), (ULONG_PTR*)&info );
	}
	__except( EXCEPTION_CONTINUE_EXECUTION )
	{
	}
}
#endif // !__NO_OUTPUT
#endif // __WIN32 && !__FINAL


struct sysIpcThreadWrapperData {
	void Init(sysIpcThreadFunc f,void *c,const char *NOTFINAL_ONLY(tn)) {
		func = f;
		closure = c;
		allocator = &sysMemAllocator::GetCurrent();
		AssertMsg(&sysMemAllocator::GetCurrent() == &sysMemAllocator::GetMaster(),"Attempting to create a thread while the current allocator isn't the same as the master allocator.  Maybe you left the debug heap active?  This is probably bad.");
#if !__FINAL
		safecpy(name,tn);
#endif
#if RSG_WIN32
		handle = NULL;
#endif // RSG_WIN32
	}
	sysIpcThreadFunc func;			// From the original caller
	void *closure;					// From the original caller
	sysMemAllocator *allocator;		// Copied from the caller's thread.
#if RSG_WIN32
	HANDLE handle;					// The real non-pseudo handle for this thread.
#endif // RSG_WIN32
	char name[32];
};

#if !__PSP2	// OS allows a structure to be passed to calling thread, obviating the need for this.
static atStaticPool<sysIpcThreadWrapperData, (__XENON || __PS3) ? 48 : 128> s_ThreadWrappers;
static sysCriticalSectionToken s_ThreadWrapperToken;
#endif

bool sysIpcEstimateStackUsage(unsigned &outCurrent,unsigned &outMax) {
#if __FINAL || __PSP2 || RSG_DURANGO /* TODO */
	outCurrent = outMax = 0;
	return false;
#elif __WIN32PC
	MEMORY_BASIC_INFORMATION memInfo;
	if(VirtualQuery(&memInfo, &memInfo, sizeof(memInfo)) != sizeof(memInfo))
	{
		outMax = 0;
	}
	else
	{
		outMax =  (unsigned)((size_t)memInfo.BaseAddress + memInfo.RegionSize - (size_t)memInfo.AllocationBase);
	}
	NT_TIB* pTIB = (NT_TIB*)NtCurrentTeb(); // TIB is first struct in the TEB
	outCurrent = (unsigned)((size_t)pTIB->StackBase - (size_t)pTIB->StackLimit);
	return true;
#elif RSG_ORBIS
	ScePthreadAttr attr;
	size_t stackSize; 
	int* stackTop;
	scePthreadAttrInit(&attr); 
	scePthreadAttrGet(scePthreadSelf(), &attr);
	scePthreadAttrGetstacksize(&attr, &stackSize); 
	scePthreadAttrGetstackaddr(&attr, (void**)&stackTop); 
	scePthreadAttrDestroy(&attr); 
	int* stackBottom = (int*)((char*)stackTop + stackSize);
	while (*stackTop == 0 && stackTop < stackBottom)
		++stackTop;
	outCurrent = (char*)stackBottom-(char*)stackTop;
	outMax = stackSize;
	return true;
#elif __XENON
	DM_THREADINFOEX ti;
	ZeroMemory(&ti,sizeof(ti));
	ti.Size = sizeof(ti);
	HRESULT res = DmGetThreadInfoEx((DWORD)g_CurrentThreadId,&ti);
	if (res == XBDM_NOERR)
	{
		int *stackBottom = (int*) ti.StackLimit;
		int *stackTop = (int*) ti.StackBase;
		while (*stackBottom == 'katS' && stackBottom < stackTop)
			++stackBottom;
		outCurrent = (char*)stackTop-(char*)stackBottom;
		outMax = (char*)stackTop-(char*)ti.StackLimit;
		return true;
	}
	else
	{
		outCurrent = outMax = 0;
		return false;
	}
#elif __PPU
	sys_ppu_thread_stack_t si;
	sys_ppu_thread_get_stack_information(&si);
	int *stackBottom = (int*) si.pst_addr;
	int *stackTop = (int*)((char*)stackBottom + si.pst_size);
	while (*stackBottom == 0 && stackBottom < stackTop)
		++stackBottom;
	outCurrent = (char*)stackTop-(char*)stackBottom;
	outMax = si.pst_size;
	return true;
#endif
}

void sysIpcComputeStackRange(void **stackTop, void **stackBottom)
{
#if __FINAL || __PSP2 || RSG_DURANGO
	// Populate it with garbage so a range check always fails.
	*stackTop = 0;
	*stackBottom = 0;
#elif RSG_ORBIS
	ScePthreadAttr attr;
	size_t stackSize; 
	scePthreadAttrInit(&attr); 
	scePthreadAttrGet(scePthreadSelf(), &attr);
	scePthreadAttrGetstacksize(&attr, &stackSize); 
	scePthreadAttrGetstackaddr(&attr, stackBottom); 
	scePthreadAttrDestroy(&attr); 
	*stackTop = (char*)*stackBottom + stackSize;
#elif __WIN32PC
	// Use the TIB would be preferable (Faster and easier), but the TIB only contains the
	// stack commit size, not the reserve size, and since this function is likely called early
	// on in the thread lifetime, the committed stack region is likely much smaller than the
	// reserve size.
	MEMORY_BASIC_INFORMATION memInfo;
	if(VirtualQuery(&memInfo, &memInfo, sizeof(memInfo)) != sizeof(memInfo))
	{
		*stackTop = 0;
		*stackBottom = 0;
	}
	else
	{
		*stackBottom = memInfo.AllocationBase;
		*stackTop = (char*)memInfo.BaseAddress + memInfo.RegionSize;
	}
#elif __XENON
	DM_THREADINFOEX ti;
	ZeroMemory(&ti,sizeof(ti));
	ti.Size = sizeof(ti);
	HRESULT res = DmGetThreadInfoEx((DWORD)g_CurrentThreadId,&ti);
	if (res == XBDM_NOERR)
	{
		*stackBottom = (void*) ti.StackLimit;
		*stackTop = (void*) ti.StackBase;
	}
	else
	{
		Assert(false);
	}
#elif __PPU
	sys_ppu_thread_stack_t si;
	sys_ppu_thread_get_stack_information(&si);
	*stackBottom = (void*) si.pst_addr;
	*stackTop = (void*)((char*)*stackBottom + si.pst_size);
#endif
}

void sysIpcComputeStackRangeMainThread()
{
#if !__FINAL
	Assert(!st_stackTop && !st_stackBottom);
	sysIpcComputeStackRange(&st_stackTop, &st_stackBottom);
#endif
}

#if !__NO_OUTPUT
__THREAD char g_CurrentThreadName[16];
#endif // !__NO_OUTPUT

#if !__FINAL
#if !__NO_OUTPUT
static void SetCurrentThreadName(const char *tn)
{
	// Make sure the thread name is surrounded by <> and ends in a space if it's not empty, to simplify message formatting.
	if (tn[0]) {		  
		if (!strncmp(tn, "[RAGE] ", 7))
		{
			tn += 7;
		}
		g_CurrentThreadName[0] = '<';
		safecpy(g_CurrentThreadName+1,tn, sizeof(g_CurrentThreadName)-3);
		safecat(g_CurrentThreadName, "> ");
	}
	else
		g_CurrentThreadName[0] = 0;
}
#endif // !__NO_OUTPUT
#endif // !__FINAL

#if __WIN32
static unsigned long __stdcall sysIpcThreadWrapper(void *ptr) {
	sysIpcSetCurrentThreadId();

	ipcDisableDenormals();

	sysIpcThreadWrapperData data = *(sysIpcThreadWrapperData*)ptr;
	{
		SYS_CS_SYNC(s_ThreadWrapperToken);
		s_ThreadWrappers.Delete((sysIpcThreadWrapperData*)ptr);
	}
	sysMemAllocator::SetCurrent(*data.allocator);
	sysMemAllocator::SetMaster(*data.allocator);
	sysMemAllocator::SetContainer(*data.allocator);
	--g_DisableInitNan;
#if RAGE_USE_DEJA
	DEJA_THREAD_INIT();
	DEJA_THREAD_LABEL(data.name);
#endif
#if !__FINAL
#if __XENON
	u32 now = sysTimer::GetSystemMsTime();
#endif // __XENON
#if !__NO_OUTPUT
	SetThreadName(data.name);
	SetCurrentThreadName(data.name);
#endif // !__NO_OUTPUT
	sysIpcComputeStackRange(&st_stackTop, &st_stackBottom);
#endif // !__FINAL

#if RSG_DURANGO && SYSTMCMD_ENABLE
	extern bool g_rockstartargetmanagerConnected;
	if (g_rockstartargetmanagerConnected)
	{
		sysTmCmdThreadBegin();
	}
#endif

	(*data.func)(data.closure);				// <--- Step in here

#if RSG_DURANGO && SYSTMCMD_ENABLE
	if (g_rockstartargetmanagerConnected)
	{
		sysTmCmdThreadEnd();
	}
#endif


#if __XENON && !__FINAL
	unsigned current, maxi;
	sysIpcEstimateStackUsage(current,maxi);
	sysInterlockedAdd(&g_ThreadStackSize,-(int)maxi);
	printf("[IPC] '%s' finished in %ums, estimated stack max usage: %u/%u\n",data.name,sysTimer::GetSystemMsTime()-now,current,maxi);
#endif
#if RAGE_USE_DEJA
	DEJA_THREAD_KILL(0);
#endif

#if THREAD_REGISTRY
#if RSG_WIN32 
	// PC normally uses pseudo-handles, so we have to use the handle that the creating thread received.
	sysThreadRegistry::UnregisterThread((sysIpcThreadId) GetThreadId(data.handle));
#else // RSG_WIN32 
	sysThreadRegistry::UnregisterThread(sysIpcGetThreadId());
#endif // RSG_WIN32
#endif // THREAD_REGISTRY
	DIAG_CONTEXT_CLEANUP();

	return 0;
}
#elif __PPU
static void sysIpcThreadWrapper(u64 ptr) {
	sysIpcSetCurrentThreadId();

	ipcDisableDenormals();

	sysIpcThreadWrapperData data = *(sysIpcThreadWrapperData*)(size_t)ptr;
	{
		SYS_CS_SYNC(s_ThreadWrapperToken);
		s_ThreadWrappers.Delete((sysIpcThreadWrapperData*)(size_t)ptr);
	}
	sysMemAllocator::SetCurrent(*data.allocator);
	sysMemAllocator::SetMaster(*data.allocator);
	sysMemAllocator::SetContainer(*data.allocator);
	--g_DisableInitNan;
#if RAGE_USE_DEJA
	DEJA_THREAD_INIT();
	DEJA_THREAD_LABEL(data.name);
#endif
#if !__FINAL
#if !__NO_OUTPUT
	SetCurrentThreadName(data.name);
#endif // !__NO_OUTPUT
	sysIpcComputeStackRange(&st_stackTop, &st_stackBottom);
	u32 now = sysTimer::GetSystemMsTime();
#if RAGE_TRACKING						   
	::rage::diagTrackerHelper track_THREAD((data.name && data.name[0])?data.name:"unnamed");
#endif // RAGE_TRACKING
#endif
	(*data.func)(data.closure);
#if !__FINAL
	unsigned current, maxi;
	sysIpcEstimateStackUsage(current,maxi);
	sysInterlockedAdd(&g_ThreadStackSize,-maxi);
	printf("[IPC] '%s' finished in %ums, estimated stack max usage: %u/%u\n",data.name,sysTimer::GetSystemMsTime()-now,current,maxi);
#endif
#if RAGE_USE_DEJA
	DEJA_THREAD_KILL(0xDEADC0DE);
#endif

#if THREAD_REGISTRY
	sysThreadRegistry::UnregisterThread(sysIpcGetThreadId());
#endif // THREAD_REGISTRY

	DIAG_CONTEXT_CLEANUP();

	sys_net_free_thread_context(0, SYS_NET_THREAD_SELF);
	sys_ppu_thread_exit(0xDEADC0DE);
}
#elif RSG_ORBIS
static void *sysIpcThreadWrapper(void *ptr) 
{
	sysIpcSetCurrentThreadId();

	sysIpcThreadWrapperData &data = *(sysIpcThreadWrapperData*)ptr;
	sysMemAllocator::SetCurrent(*data.allocator);
	sysMemAllocator::SetMaster(*data.allocator);
	sysMemAllocator::SetContainer(*data.allocator);
	--g_DisableInitNan;
#if !__FINAL
#if !__NO_OUTPUT
	SetCurrentThreadName(data.name);
#endif // !__NO_OUTPUT
	sysIpcComputeStackRange(&st_stackTop, &st_stackBottom);
#endif // !__FINAL
	DIAG_CONTEXT_CLEANUP();
	(*data.func)(data.closure);

	{
		SYS_CS_SYNC(s_ThreadWrapperToken);
		s_ThreadWrappers.Delete((sysIpcThreadWrapperData*)(size_t)ptr);
	}
#if THREAD_REGISTRY
	sysThreadRegistry::UnregisterThread(sysIpcGetThreadId());
#endif // THREAD_REGISTRY

	return (void*) 0xDEADC0DE;
}
#endif

sysIpcThreadId sysIpcGetThreadId() {
#if __PSP2
	Assert(false);	// Not supported yet.
	return (sysIpcThreadId) 0;
#elif __PPU
	sys_ppu_thread_t threadInfo;
	sys_ppu_thread_get_id(&threadInfo);
	
	return (sysIpcThreadId) threadInfo;
#elif RSG_ORBIS
	return (sysIpcThreadId) scePthreadSelf();
#else
	return (sysIpcThreadId) GetCurrentThread();
#endif
}

void sysIpcSetThreadPriorityOverrideCb(sysIpcThreadPriorityOverrideCb cb)
{
	s_ThreadPriorityOverrideCb = cb;
}

#if RSG_FINAL && RSG_PC
// This is defined in ipc.h; undefining here so we can define the method body
#undef sysIpcCreateThread
#endif

#if __PSP2
sysIpcThreadId sysIpcCreateThread(sysIpcThreadFunc func,void *closure,unsigned stackSize,sysIpcPriority priority,const char *threadName,int WIN32_ONLY(cpu) PSP2_ONLY(cpu), const char *threadTtyName) {
	sysIpcThreadWrapperData data;

	if (s_ThreadPriorityOverrideCb && threadTtyName)
	{
		s_ThreadPriorityOverrideCb(atStringHash(threadTtyName), priority, cpu);
	}
	data.Init(func,closure,threadName);
	SceUID tid = sceKernelCreateThread(threadName,sysIpcThreadWrapper,SCE_KERNEL_DEFAULT_PRIORITY_USER-(int)priority,stackSize,
		0,	SCE_KERNEL_CPU_MASK_USER_0 << cpu, SCE_NULL);
	sysInterlockedAdd(&g_ThreadStackSize,stackSize);
	sceKernelStartThread(tid,sizeof(data),&data);
#if THREAD_REGISTRY
	sysThreadRegistry::RegisterThread((sysIpcThreadId) tid, priority, threadName, threadTtyName);
#endif // THREAD_REGISTRY
	return (sysIpcThreadId) tid;
}
#else
sysIpcThreadId sysIpcCreateThread(sysIpcThreadFunc func,void *closure,unsigned stackSize,sysIpcPriority priority,const char *threadName,int XENON_ONLY(cpu) PSP2_ONLY(cpu) ORBIS_ONLY(cpu) DURANGO_ONLY(cpu), const char *threadTtyName) {

#if RSG_PC 
	// affinityMask will be ignored, so just pass in -1 
	u32 mask = (u32)-1;
#elif RSG_DURANGO
	// CMT - don't include Core 0 here, as that is where the main thread runs. This 
	// shouldn't be a problem, but Microsoft's scheduler appears to have a weird issue 
	// with resuming higher priority threads that are awaking from sleep condition 
	// (and possibly other blocking cases). So if the main thread sleeps (or possibly 
	// blocks waiting for a thread synchronization mechanism or other resource), when  
	// the thread wakes up, even if it is higher priority that the currently running 
	// thread on that core, it is not given the core back. The end result is that the 
	// main thread might sleep, and only *need* to sleep for a tiny fraction of time, 
	// but end up getting blocked out of its core for many milliseconds by another thread 
	// that just happened to jump in when it went to sleep.
	//
	// This is mostly just a problem with the main thread for now, so don't include its 
	// core in this mask. Hacky, but a workaround until we find out the cause of this 
	// odd MS thread scheduling behavior (PS4 definitely does not have this problem).
	const static u32 cAllCoresMask = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) ;
	u32 mask = (cpu >= 0) ? (1 << cpu) : cAllCoresMask;
#else
	const static u32 cAllCoresMask = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) ;
	u32 mask = (cpu >= 0) ? (1 << cpu) : cAllCoresMask;
#endif
	
	return sysIpcCreateThreadWithCoreMask(func, closure, stackSize, priority, threadName, mask, threadTtyName);
}

sysIpcThreadId sysIpcCreateThreadWithCoreMask(sysIpcThreadFunc func,void *closure,unsigned stackSize,sysIpcPriority priority,const char *threadName,u32 cpuAffinityMask,const char *threadTtyName) {

	if (s_ThreadPriorityOverrideCb && threadTtyName)
	{
		s_ThreadPriorityOverrideCb(atStringHash(threadTtyName), priority, cpuAffinityMask);
	}

	AssertMsg(stackSize,"Zero-sized stack is now illegal, please allocate how much you actually need!");
	if (stackSize < sysIpcMinThreadStackSize)
		stackSize = sysIpcMinThreadStackSize;
	sysIpcThreadWrapperData *data;
	{
		SYS_CS_SYNC(s_ThreadWrapperToken);
		data = s_ThreadWrappers.New();
	}
	if (!data)
		return sysIpcThreadIdInvalid;
	data->Init(func,closure,threadName);
#if __WIN32
	DWORD threadId=0;	// required parameter for win95/98
	HANDLE h;
	if ((h = CreateThread(NULL, stackSize, sysIpcThreadWrapper, data, CREATE_SUSPENDED, &threadId)) == NULL) {
		Displayf("[IPC] *** THREAD CREATION FAILED (%s) - probably very low on XTL memory.  Waiting and trying again...\n",threadName);
		do {
			sysIpcSleep(250);
		} while ((h = CreateThread(NULL, stackSize, sysIpcThreadWrapper, data, CREATE_SUSPENDED, &threadId)) == NULL);
	}

#if RSG_WIN32
	data->handle = h;
#endif // RSG_WIN32

	Assert((sysIpcThreadId)threadId != sysIpcThreadIdInvalid);
	// We use Win32 priority values natively in rage now.
	SetThreadPriority(h, (int) priority);
	// Don't let OS try to be smarter than us.
	SetThreadPriorityBoost(h, TRUE);
#if __WIN32PC
#if RSG_PC && !__RESOURCECOMPILER && !(__TOOL || __GAMETOOL) && (!defined(__RGSC_DLL) || !__RGSC_DLL)

	u32 coreAffinity = (u32)-1;

	HMODULE handle = GetModuleHandle(TEXT("kernel32.dll"));
	Assertf(handle, "Unable to find kernel32.dll!");

	BOOL (WINAPI *pGetThreadGroupAffinity)( _In_ HANDLE, _Out_ PGROUP_AFFINITY );
	BOOL (WINAPI *pSetThreadGroupAffinity)( _In_ HANDLE, _In_ CONST GROUP_AFFINITY *, _Out_opt_ PGROUP_AFFINITY );

	pGetThreadGroupAffinity = (BOOL (WINAPI *)( _In_ HANDLE, _Out_ PGROUP_AFFINITY ))::GetProcAddress( handle, "GetThreadGroupAffinity");
	pSetThreadGroupAffinity = (BOOL (WINAPI *)( _In_ HANDLE, _In_ CONST GROUP_AFFINITY *, _Out_opt_ PGROUP_AFFINITY ))::GetProcAddress( handle, "SetThreadGroupAffinity");

	if (PARAM_setSystemProcessorAffinity.Get(coreAffinity) && (pGetThreadGroupAffinity!=NULL) && (pSetThreadGroupAffinity!=NULL))
	{
		GROUP_AFFINITY ga, prevGa;

		pGetThreadGroupAffinity(h, &ga);

		pGetThreadGroupAffinity(h, &ga);
		ga.Mask = coreAffinity;

		//BANK_ONLY(Displayf("Setting thread affinity for <%s> to mask 0x%x cpu %u, pri %d", threadName, ga.Mask, cpu, priority));
		pSetThreadGroupAffinity(h, &ga, &prevGa);
	}

#endif // RSG_PC
#elif RSG_DURANGO
	if (!PARAM_useSystemThreadManagement.Get())
	{
		GROUP_AFFINITY ga, prevGa;

		GetThreadGroupAffinity(h, &ga);
		ga.Mask = (u64)cpuAffinityMask;

		//BANK_ONLY(Displayf("Setting thread affinity for <%s> to mask 0x%x cpu %u, pri %d", threadName, ga.Mask, cpu, priority));
		SetThreadGroupAffinity(h, &ga, &prevGa);
	}
#endif
	ResumeThread(h);

#if THREAD_REGISTRY
	sysThreadRegistry::RegisterThread((sysIpcThreadId) GetThreadId(h), priority, threadName, threadTtyName);
#endif // THREAD_REGISTRY

	return (sysIpcThreadId)h;
#elif RSG_ORBIS
	if (stackSize == 0)
		stackSize = 65536;
	ScePthread thread;
	static ScePthreadAttr attr;
	if (!attr) {
		scePthreadAttrInit(&attr);
		scePthreadAttrSetdetachstate(&attr,SCE_PTHREAD_CREATE_JOINABLE);
		scePthreadAttrSetinheritsched(&attr, SCE_PTHREAD_EXPLICIT_SCHED);
		scePthreadAttrSetschedpolicy(&attr,SCE_KERNEL_SCHED_RR);
	}

	scePthreadAttrSetaffinity(&attr, cpuAffinityMask);
	scePthreadAttrSetstacksize(&attr,stackSize);
	SceKernelSchedParam p = { SCE_KERNEL_PRIO_FIFO_DEFAULT - priority };
	scePthreadAttrSetschedparam(&attr,  &p);
	
	if(scePthreadCreate(&thread, &attr, sysIpcThreadWrapper, data, threadName) != 0){
		FatalAssertf(false, "Unable to create thread: %s, (name length is limited to 32 bytes)", threadName);
		return NULL;
	}
	

#if THREAD_REGISTRY
	sysThreadRegistry::RegisterThread((sysIpcThreadId) thread, priority, threadName, threadTtyName);
#endif // THREAD_REGISTRY

	return (sysIpcThreadId)thread;
#endif
}
#endif

// For PC Final, remove thread names at the call site (the optimizer doesn't do it if it happens later)
// The ternary operator is used to avoid a warning if threadName is the sole use of a parameter of the calling function
// Also defined in ipc.h; redefined here to match the #undef above, in order to cope with Unity builds.
#if RSG_FINAL && RSG_PC
#define sysIpcCreateThread(func,closure,stackSize,priority,threadName,...) sysIpcCreateThread(func,closure,stackSize,priority,true?"":threadName,__VA_ARGS__)
#endif

#if __PS3
int sysIpcLoadModule(int moduleId) {
	if (cellSysmoduleLoadModule(moduleId) != CELL_OK) {
		printf("[IPC] *** SYSMODULE LOAD %x FAILED - probably very low on OS memory.  Waiting and trying again...\n",moduleId);
		do {
			sysIpcSleep(250);
		} while (cellSysmoduleLoadModule(moduleId) != CELL_OK);
	}

	sys_memory_info_t mem_info;
	sys_memory_get_user_memory_size( &mem_info );
	size_t remain = mem_info.available_user_memory >> 10;
	printf("[IPC] Module %x loaded (%uk remaining)\n",moduleId,remain);

	return CELL_OK;
}

int sysIpcUnloadModule(int moduleId) {
	int result = cellSysmoduleUnloadModule(moduleId);
	if (result != CELL_OK)
		printf("[IPC] *** SYSMODULE UNLOAD %x FAILED.\n",moduleId);

	sys_memory_info_t mem_info;
	sys_memory_get_user_memory_size( &mem_info );
	size_t remain = mem_info.available_user_memory >> 10;
	printf("[IPC] Module %x unloaded (%uk remaining)\n",moduleId,remain);
	return result;
}
#endif

bool sysIpcWaitThreadExit(sysIpcThreadId id) {
#if __WIN32
	bool result = WaitForSingleObject(id,INFINITE) == WAIT_OBJECT_0;
	::CloseHandle(id);

#if __XENON && !__NO_OUTPUT
	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	size_t remain = ms.dwAvailPhys >> 10;
	Displayf("[IPC] Thread TID%08X exited (%uk remaining)",(u32)id,remain);
#endif

	return result;
#elif __PPU
	uint64_t status;
	bool result = sys_ppu_thread_join(id,&status) == 0;

	sys_memory_info_t mem_info;
	sys_memory_get_user_memory_size( &mem_info );
	OUTPUT_ONLY(size_t remain = mem_info.available_user_memory >> 10);
	Displayf("[IPC] Thread TID%08X exited (%uk remaining)",(u32)id,remain);

	return result;
#elif RSG_ORBIS
	return scePthreadJoin((ScePthread)id,NULL) == 0;
#elif __PSP2
	SceInt32 exitStatus;
	return sceKernelWaitThreadEnd((SceUID)id,&exitStatus,0)==0;
#endif
}


void sysIpcSetThreadProcessor(sysIpcThreadId WIN32PC_ONLY(id) XENON_ONLY(id) DURANGO_ONLY(id) ORBIS_ONLY(id), int WIN32PC_ONLY(cpu) XENON_ONLY(cpu) DURANGO_ONLY(cpu) ORBIS_ONLY(cpu)) {
#if __WIN32PC
#if RSG_PC && !__RESOURCECOMPILER && !__TOOL
	if (!PARAM_useSystemThreadManagement.Get())
#endif // RSG_PC
	::SetThreadIdealProcessor(id,cpu);
#elif __XENON
	XSetThreadProcessor(id,cpu);
#elif RSG_DURANGO
	if (!PARAM_useSystemThreadManagement.Get())
	{
		HANDLE h = (HANDLE)id;
		GROUP_AFFINITY ga, prevGa;
		GetThreadGroupAffinity(h, &ga);
		ga.Mask = (u64)1 << cpu;

		SetThreadGroupAffinity(h, &ga, &prevGa);
	}
#elif RSG_ORBIS
	scePthreadSetaffinity((ScePthread)id, 1 << cpu);
#endif
}

#if RSG_DURANGO || RSG_ORBIS
void sysIpcSetThreadProcessorMask(sysIpcThreadId WIN32PC_ONLY(id) XENON_ONLY(id) DURANGO_ONLY(id) ORBIS_ONLY(id), u32 WIN32PC_ONLY(cpuMask) XENON_ONLY(cpuMask) DURANGO_ONLY(cpuMask) ORBIS_ONLY(cpuMask)) {
#if RSG_DURANGO
	if (!PARAM_useSystemThreadManagement.Get())
	{
		HANDLE h = (HANDLE)id;
		GROUP_AFFINITY ga, prevGa;
		GetThreadGroupAffinity(h, &ga);
		ga.Mask = (u64)cpuMask;

		SetThreadGroupAffinity(h, &ga, &prevGa);
	}
#elif RSG_ORBIS
	scePthreadSetaffinity((ScePthread)id, cpuMask);
#endif
}
#endif

#if RSG_ORBIS
int sysIpcGetThreadProcessor(sysIpcThreadId id)
{
	SceKernelCpumask cpuMask;
	scePthreadGetaffinity((ScePthread)id, &cpuMask);

	// will only be one cpu affinity...sysIpc only allows that on thread creation
	for (int cpu = 0 ; cpu < sysIpcGetProcessorCount(); cpu++)
	{
		if(cpuMask & (1 << cpu))
		{
			return cpu;
		}
	}

	return sysIpcCpuInvalid;
}
#endif


int sysIpcGetProcessorCount() {
#if __WIN32PC
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#elif __XENON
	return 6;
#elif RSG_ORBIS
	return 6;
#else
	return __PPU+1;
#endif
}


void sysIpcExitThread() {
#if __WIN32
	ExitThread(0);
#elif __PPU
	sys_ppu_thread_exit(0);
#elif RSG_ORBIS
	scePthreadExit(0);
#endif
}


void sysIpcYield(int) {
#if __WIN32
	Sleep(0);
#elif __PPU
	sys_ppu_thread_yield();
#elif RSG_ORBIS
	scePthreadYield();
#endif
}


void sysIpcSleep(unsigned ms) {
#if __WIN32
	::Sleep(ms);
#elif __PPU
	sys_timer_usleep(ms * 1000);
#elif RSG_ORBIS
	sceKernelUsleep(ms * 1000);
#elif __PSP2
	sceKernelDelayThread(ms * 1000);
#endif
}

#if !__PSP2
bool sysIpcSetThreadPriority(sysIpcThreadId tid,sysIpcPriority prio)
{
#if __WIN32
	return ::SetThreadPriority((HANDLE)tid, prio) != 0;
#elif __PS3
	return sys_ppu_thread_set_priority(tid, kSonyBasePrio-(int)prio) == CELL_OK;
#elif RSG_ORBIS
	return scePthreadSetprio((ScePthread)tid,kSonyBasePrio-(int)prio) == 0;
#endif
}

sysIpcPriority sysIpcGetCurrentThreadPriority()
{
#if __WIN32
	int oldPrio = ::GetThreadPriority(GetCurrentThread());
	return (sysIpcPriority)oldPrio;
#elif __PS3
	sys_ppu_thread_t myself;
	sys_ppu_thread_get_id(&myself);
	int oldPrio;
	sys_ppu_thread_get_priority(myself, &oldPrio);
	return (sysIpcPriority)(kSonyBasePrio-oldPrio);
#elif RSG_ORBIS
	int prio;
	scePthreadGetprio(scePthreadSelf(),&prio);
	return (sysIpcPriority)(kSonyBasePrio-prio);
#endif
}

sysIpcPriority sysIpcSetCurrentThreadPriority(sysIpcPriority newPrio)
{
#if __WIN32
	int oldPrio = ::GetThreadPriority(GetCurrentThread());
	::SetThreadPriority(GetCurrentThread(), newPrio);
	return (sysIpcPriority)oldPrio;
#elif __PS3
	sys_ppu_thread_t myself;
	sys_ppu_thread_get_id(&myself);
	int oldPrio;
	sys_ppu_thread_get_priority(myself, &oldPrio);
	sys_ppu_thread_set_priority(myself, kSonyBasePrio-newPrio);
	return (sysIpcPriority)(kSonyBasePrio-oldPrio);
#elif RSG_ORBIS
	int oldPrio;
	scePthreadGetprio(scePthreadSelf(), &oldPrio);
	scePthreadSetprio(scePthreadSelf(), kSonyBasePrio-newPrio);
	return (sysIpcPriority)(kSonyBasePrio-oldPrio);
#endif
}
#endif


void sysIpcSetCurrentThreadId() {
#if __WIN32
	g_CurrentThreadId = (sysIpcCurrentThreadId) GetCurrentThreadId();
#elif __PPU
	sys_ppu_thread_t self;
	sys_ppu_thread_get_id(&self);
	g_CurrentThreadId = (sysIpcCurrentThreadId) (intptr_t)((self >> 32) ^ self);
#elif RSG_ORBIS
	g_CurrentThreadId = (sysIpcCurrentThreadId) scePthreadSelf();
#elif __PSP2
	g_CurrentThreadId = (sysIpcCurrentThreadId) sceKernelGetThreadId();
#endif
}

#define CALLSTACK_MAX_SIZE (40)
#if __PPU && ENABLE_ALL_CALLSTACK_TRACE
int sysIpcGetThreadCallstack(unsigned int *callStack, int max_frames, void* stackptr)
{
	uint64_t* frameptr = (uint64_t*) stackptr;

	int i=0;
	for (i=0; i<max_frames && frameptr; frameptr=(uint64_t*)(u32)(*frameptr))
	{
		if (frameptr[2])
		{
			callStack[i] = frameptr[2];
			i++;
		}
	}
	return i;
}

#if __PPU && ENABLE_ALL_CALLSTACK_TRACE
void DisplayStackLinePlainText(u32 OUTPUT_ONLY(address), const char* OUTPUT_ONLY(symbol), u32 OUTPUT_ONLY(offset))
{
	Displayf("0x%x - %s +%i", address, symbol, offset);
}
#endif

void sysIpcPrintThreadCallStack(void *stackptr)
{
	unsigned int callStack[CALLSTACK_MAX_SIZE];
	memset( callStack, 0, sizeof(callStack));
	int depth = sysIpcGetThreadCallstack(callStack,CALLSTACK_MAX_SIZE, stackptr);	

	//in debug only: there's a first item which seems to be wrong, not sure why
	//in release only: the function above the printCallback is missing in the callstack don't know why either...
	//so starts printing at 1 in release and 2 in debug
//#ifdef _DEBUG
//	int start = 2;
//#else
//	int start = 1;
//#endif
//	for (int i=start;i<depth;i++)
//	{
//		Displayf("0x%08x", callStack[i]);
//	}

	// dump out symbols if we can
	sysStack::PrintCapturedStackTrace(&callStack[1], CALLSTACK_MAX_SIZE-1, DisplayStackLinePlainText);
}
#endif // __PPU && ENABLE_ALL_CALLSTACK_TRACE

// The threads must be in STOP state prior to calling this function or it will fail to return the context
void sysIpcShowAllCallstacks()
{
#if __PPU && ENABLE_ALL_CALLSTACK_TRACE
	#define MAX_NUM_THREADS	64
	sys_ppu_thread_t threadIds[MAX_NUM_THREADS];
	memset( threadIds, 0, sizeof(threadIds));

	uint64_t num_id = MAX_NUM_THREADS;
	uint64_t total_num;
	sys_dbg_get_ppu_thread_ids(threadIds, &num_id, &total_num);

	sys_dbg_ppu_thread_context_t context;

	for ( int i = 0; i < num_id; i++ )
	{
		sys_ppu_thread_t threadId = threadIds[i];

		char name[28];
		strcpy(name,"unknown");
		sys_dbg_get_ppu_thread_name(threadId,name);

		Displayf("Callstack on thread %x  [%s]", (u32)threadId, name );
		memset(&context, 0xaa, sizeof(sys_dbg_ppu_thread_context_t));

		if(sys_dbg_read_ppu_thread_context(threadId, &context) == CELL_OK)
		{
			// register r1 contains the stack pointer
			void *stackptr = (void*)(u32)(context.gpr[1]);
			sysIpcPrintThreadCallStack(stackptr);
		}
		else
		{
			Errorf("Couldn't get context for thread 0x%x", (u32)threadId);
		}

		// newline inbetween threads
		Displayf("");
	}
	__debugbreak();
#endif // __PPU && ENABLE_ALL_CALLSTACK_TRACE
}

void sysIpcTriggerAllCallstackDisplay()
{
#if ENABLE_ALL_CALLSTACK_TRACE
#if __XENON
    sysStack::PrintStackTraceAllThreads();
#elif __WIN32PC
    sysStack::PrintStackTraceAllThreads();
#elif __PPU
#if SYSTMCMD_ENABLE
	if (PARAM_rockstartargetmanager.Get())
	{
		sysTmCmdPrintAllThreadStacks();
	}
	else
#endif
	{
		// Set our flag to always do a full callstack dump and then cause a
		// crash to trigger the exception handler to do it We have to do it this
		// way because all the threads have to be in the STOP state.
		gDumpAllThreadCallstacks = true;

		// Wake up the PPU exception handler thread.  This is not the same as
		// triggering an exception on the PPU, so saves getting all the lv2
		// exception handler output as well.
		const uint64_t flags=0;
		const unsigned ms=500;
		while(sys_dbg_signal_to_ppu_exception_handler(flags) != CELL_OK) sysIpcSleep(ms);

		// Can't safetly continue
		for (;;) sysIpcSleep(ms);
	}
#elif SYSTMCMD_ENABLE
	if (PARAM_rockstartargetmanager.Get())
	{
		sysTmCmdPrintAllThreadStacks();
	}
	else
	{
		diagLoggedPrintf("sysIpcTriggerAllCallstackDisplay(): Call stacks not supported on this platform without R*TM!\n");
	}
#endif
#endif  //ENABLE_ALL_CALLSTACK_TRACE
}

#if ENABLE_STALL_CALLBACKS

static void null_stall_callback(sysIpcStallReason) { }

void (*sysIpcStallCallback)(sysIpcStallReason) = null_stall_callback;

#endif

sysIpcEvent sysIpcCreateEvent()
{
	// We could expose initial state but no code uses anything other than false.
	// Some code wants manual reset, but we can't expose that portably because on Sony
	// platforms it's a property of the wait call, not the actual event object, so we'd
	// have to add a bunch of internal data tracking.
#if __WIN32
	return (sysIpcEvent) CreateEvent(NULL, FALSE, FALSE, NULL);
#elif __PS3
	sys_event_flag_t ef;
	sys_event_flag_attribute_t attr;
	sys_event_flag_attribute_initialize(attr);
	attr.type = SYS_SYNC_WAITER_MULTIPLE;
	sys_event_flag_create(&ef,&attr,0);
	return (sysIpcEvent)ef;
#elif __PSP2
	return (sysIpcEvent) sceKernelCreateEventFlag("event",SCE_KERNEL_EVF_ATTR_MULTI,0,NULL);
#elif RSG_ORBIS
	SceKernelEventFlag ef;
	sceKernelCreateEventFlag(&ef,"event",SCE_KERNEL_EVF_ATTR_MULTI,0,NULL);
	return (sysIpcEvent) ef;
#endif
}

void sysIpcDeleteEvent(sysIpcEvent ev)
{
#if __WIN32
	::CloseHandle(ev);
#elif __PS3
	sys_event_flag_destroy((sys_event_flag_t)ev);
#elif __PSP2
	sceKernelCloseEventFlag(ev);
#elif RSG_ORBIS
	sceKernelDeleteEventFlag((SceKernelEventFlag)ev);
#endif
}

bool sysIpcWaitEvent(sysIpcEvent ev)
{
#if __WIN32
	return WaitForSingleObject(ev, INFINITE) == WAIT_OBJECT_0;
#elif __PS3
	return sys_event_flag_wait((sys_event_flag_t)ev, 1, SYS_EVENT_FLAG_WAIT_AND | SYS_EVENT_FLAG_WAIT_CLEAR, NULL, SYS_NO_TIMEOUT) == 0;
#elif __PSP2
	SceUInt32 result = 0;
	SceUInt32 timeout = 0;
	return sceKernelWaitEvent(ev, 1, &result, NULL, &timeout) == 0;
#elif RSG_ORBIS
	return sceKernelWaitEventFlag((SceKernelEventFlag)ev, 1, SCE_KERNEL_EVF_WAITMODE_AND | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT, NULL, NULL) == 0;
#endif
}

bool sysIpcPollEvent(sysIpcEvent ev)
{
#if __WIN32
	return WaitForSingleObject(ev, 0) == WAIT_OBJECT_0;
#elif __PS3
	return sys_event_flag_trywait((sys_event_flag_t)ev, 1, SYS_EVENT_FLAG_WAIT_AND | SYS_EVENT_FLAG_WAIT_CLEAR, NULL) == 0;
#elif __PSP2
	SceUInt32 result = 0;
	SceUInt32 timeout = 0;
	return sceKernelPollEvent(ev, 1, &result, NULL) == 0;
#elif RSG_ORBIS
	return sceKernelPollEventFlag((SceKernelEventFlag)ev, 1, SCE_KERNEL_EVF_WAITMODE_AND | SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT, NULL) == 0;
#endif
}
void sysIpcSetEvent(sysIpcEvent ev)
{
#if __WIN32
	SetEvent(ev);
#elif __PS3
	sys_event_flag_set((sys_event_flag_t)ev,1);
#elif __PSP2
	sceKernelSetEventFlag(ev,1);
#elif RSG_ORBIS
	sceKernelSetEventFlag((SceKernelEventFlag)ev,1);
#endif
}

}	// namespace rage
