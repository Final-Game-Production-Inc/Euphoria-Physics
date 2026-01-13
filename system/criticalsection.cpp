// 
// system/criticalsection.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "criticalsection.h"
#include "file/remote.h"		// for fiIsShowingMessageBox
#include "system/bootmgr.h"

#if __WIN32
#include "system/xtl.h"
#else
#if !__FINAL
#include "system/stack.h"
#endif
#if __PS3
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <stdio.h>
#elif __PSP2
#include <kernel.h>
CompileTimeAssert(sizeof(SceKernelLwMutexWork) <= sizeof(rage::sysCriticalSectionToken));
#endif
#endif

#if RSG_ORBIS
#include <kernel.h>
#endif

#if __PPU
#pragma comment(lib,"sync_stub") // For criticalsection_spu.h
#endif
#include "system/param.h"
#include "system/timer.h"

#if !__FINAL
#if __PPU
bool g_grcIsCurrentlyCapturing;		// intentionally outside rage namespace
#endif
int sysIpcCritSecDisableTimeout = 0;
#endif

namespace rage {

#if !__FINAL
__THREAD u32 g_CriticalSectionTimeout;
#endif

// --- ipcCriticalSectionToken ----

sysCriticalSectionToken::sysCriticalSectionToken(int WIN32_ONLY(spinCount)) {
#if __WIN32
	CompileTimeAssert(sizeof(*this) >= sizeof(CRITICAL_SECTION));
#if __XENON
	CompileTimeAssertSize(CRITICAL_SECTION,28,40);
#else
	CompileTimeAssertSize(CRITICAL_SECTION,24,40);
#endif
	InitializeCriticalSectionAndSpinCount(CRITSEC_PTR,spinCount);	// number taken from DXUT code
#elif __PS3
	CompileTimeAssert(sizeof(*this) >= sizeof(sys_lwmutex_t));
	sys_lwmutex_attribute_t attr = { SYS_SYNC_FIFO, SYS_SYNC_RECURSIVE, "critsec" };
	if (sys_lwmutex_create(CRITSEC_PTR,&attr) != CELL_OK)
		Quitf("Unable to create critsec lwmutex @%p",CRITSEC_PTR);
	if (!m_Impl[0])
		Quitf("Critsec didn't init either control word");
#elif __PSP2
	if (sceKernelCreateLwMutex(CRITSEC_PTR, "critsec", SCE_KERNEL_LW_MUTEX_ATTR_TH_FIFO | SCE_KERNEL_LW_MUTEX_ATTR_RECURSIVE, 0, NULL))
		Quitf("Unable to create critsec lwmutex @%p",CRITSEC_PTR);
#elif RSG_ORBIS
	static ScePthreadMutexattr attr;
	if (!attr) {
		scePthreadMutexattrInit(&attr);
		scePthreadMutexattrSettype(&attr, SCE_PTHREAD_MUTEX_RECURSIVE);
	}
	scePthreadMutexInit(&m_Impl, &attr, NULL);	
#endif
}

sysCriticalSectionToken::~sysCriticalSectionToken() {
#if __WIN32
	DeleteCriticalSection(CRITSEC_PTR);
#elif __PS3
	if (sys_lwmutex_destroy(CRITSEC_PTR) != CELL_OK)
		Errorf("sysCriticalSectionToken::~sysCriticalSectionToken failed??? (normal during shutdown)");
#elif __PSP2
	sceKernelDeleteLwMutex(CRITSEC_PTR);
#elif RSG_ORBIS
	scePthreadMutexDestroy(CRITSEC_PTR);
#endif
}

#if __PS3 && !__FINAL
void ps3displayline(u32 addr,const char *sym,u32 offset) {
	// use real printf to avoid problems with mutexes from our own display code interfering.
	printf("**** %8x - %s+%x\n",addr,sym,offset);
}
#endif

__THREAD s64 g_CritSecWaitTicks;

// NOTE: required for streaming iterators
#if __DEV && __CONSOLE
PARAM(noCriticalSectionTimeout,"disable timeout in sysCriticalSectionToken::Lock");
#endif // __DEV && __CONSOLE
#if !__FINAL
PARAM(breakondeadlock, "[system] debugbreak when detecting critical section deadlock");
#endif // !__FINAL

#if !INLINE_CRITSEC
void sysCriticalSectionToken::Lock() {
	// Catch uninitialized critical section at global scope.
	WIN32_ONLY(if (!m_Impl[0])	return);

	sysIpcStallCallback(CRITSEC_BEGIN);
#if USE_TELEMETRY_CRITICAL
	TELEMETRY_LOCK(CRITSEC_PTR, __FILE__, __LINE__, __FUNCTION__);
	TELEMETRY_TRY_LOCK(CRITSEC_PTR, __FILE__, __LINE__, __FUNCTION__);
#endif // USE_TELEMETRY_CRITICAL

#if __WIN32
	DEV_ONLY(utimer_t now = sysTimer::GetTicks());
	EnterCriticalSection(CRITSEC_PTR);
	DEV_ONLY(g_CritSecWaitTicks += (sysTimer::GetTicks() - now));
#elif __PS3
# if !__FINAL
	DEV_ONLY(utimer_t now = sysTimer::GetTicks());
	if (sys_lwmutex_lock(CRITSEC_PTR,10 * 1000 * 1000) == ETIMEDOUT) {
		if (!fiIsShowingMessageBox && !sysStack::HasExceptionBeenThrown() && !g_grcIsCurrentlyCapturing && !sysIpcCritSecDisableTimeout DEV_ONLY(CONSOLE_ONLY(&& !PARAM_noCriticalSectionTimeout.Get()))) {
			printf("**** %s sysCriticalSectionToken::Lock - taking longer than ten seconds.\n",g_CurrentThreadName);
			printf("**** %s owner = TID%08X, waiter = 0x%x, recursive_count = %d\n",g_CurrentThreadName,CRITSEC_PTR->lock_var.info.owner,CRITSEC_PTR->lock_var.info.waiter,CRITSEC_PTR->recursive_count);
			// Call functions explicitly to avoid PrePrintStackTrace which may result in recursion here.
			u32 trace[32];
			sysStack::CaptureStackTrace(trace,32);
			sysStack::PrintCapturedStackTrace(trace, 32, ps3displayline);

			// Assuming we have a debugger, breaking here is a lot more helpful than to keep faffing about
			// and waiting for the user to realize that something is wrong.
			if (PARAM_breakondeadlock.Get() && sysBootManager::IsDebuggerPresent())
				__debugbreak();
		}
		// After the initial timeout, do a try and a sleep so that breaking in the debugger works.
		while (sys_lwmutex_trylock(CRITSEC_PTR) != CELL_OK)
			sys_timer_usleep(100);
	}
	DEV_ONLY(g_CritSecWaitTicks += (sysTimer::GetTicks() - now));
# else
	sys_lwmutex_lock(CRITSEC_PTR,0);
# endif
#elif __PSP2
	sceKernelLockLwMutex(CRITSEC_PTR, 1, NULL);
#elif RSG_ORBIS
	scePthreadMutexLock(CRITSEC_PTR);
#endif

#if USE_TELEMETRY_CRITICAL
	TELEMETRY_END_TRY_LOCK(LRT_SUCCESS, CRITSEC_PTR, __FILE__, __LINE__);
#endif // USE_TELEMETRY_CRITICAL
	sysIpcStallCallback(CRITSEC_END);
}

bool sysCriticalSectionToken::TryLock() {
	// Catch uninitialized critical section at global scope.
	WIN32_ONLY(if (!m_Impl[0])	return true);

#if USE_TELEMETRY_CRITICAL
	TELEMETRY_TRY_LOCK(CRITSEC_PTR, __FILE__, __LINE__, __FUNCTION__);
#endif // USE_TELEMETRY_CRITICAL

#if __WIN32
	bool r = (TryEnterCriticalSection(CRITSEC_PTR) != 0);
#elif __PS3
	bool r = (sys_lwmutex_trylock(CRITSEC_PTR) == CELL_OK);
#elif __PSP2
	bool r = sceKernelTryLockLwMutex(CRITSEC_PTR, 1) == 0;
#elif RSG_ORBIS
	bool r = scePthreadMutexTrylock(CRITSEC_PTR) == 0;
#endif

#if USE_TELEMETRY_CRITICAL
	TELEMETRY_END_TRY_LOCK(r ? LRT_SUCCESS : LRT_FAILED, CRITSEC_PTR, __FILE__, __LINE__);
	if (r) 
	{
		TELEMETRY_LOCK(CRITSEC_PTR, __FILE__, __LINE__, __FUNCTION__);
	}
#endif // USE_TELEMETRY
	return r;
}

void sysCriticalSectionToken::Unlock() {
	// Catch uninitialized critical section at global scope.
	WIN32_ONLY(if (!m_Impl[0])	return);

#if USE_TELEMETRY_CRITICAL
	TELEMETRY_UNLOCK(CRITSEC_PTR, __FILE__, __LINE__, __FUNCTION__);
#endif // USE_TELEMETRY_CRITICAL
#if __WIN32
	LeaveCriticalSection(CRITSEC_PTR);
#elif __PS3
	sys_lwmutex_unlock(CRITSEC_PTR);
#elif __PSP2
	sceKernelUnlockLwMutex(CRITSEC_PTR, 1);
#elif RSG_ORBIS
	scePthreadMutexUnlock(CRITSEC_PTR);
#endif
}

#endif		// INLINE_CRITSEC

}
