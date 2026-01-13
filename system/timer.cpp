//
// system/timer.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "timer.h"

#include "ipc.h"
#include "xtl.h"

#include "diag/errorcodes.h"
#include "diag/output.h"
#include "system/param.h"

#if __WIN32PC
#pragma comment(lib, "winmm.lib")
#include "system/xtl.h"
#elif __PSP2
#include <kernel.h>
#elif __OSX
#include <sys/time.h>
#elif __PPU
#include <sys/sys_time.h>
#include <sys/time_util.h>
#endif

using namespace rage;

#if !defined(CONST_FREQ)
float sysTimerConsts::TicksToSeconds=0.0f;
float sysTimerConsts::TicksToMilliseconds=0.0f;
float sysTimerConsts::TicksToMicroseconds=0.0f;
float rage::sysTimerConsts::TicksToNanoseconds=0.0f;
float sysTimerConsts::CpuSpeed;

#endif

#if !__FINAL || __FINAL_LOGGING
u32 sysTimer::sm_StallThreshold = 2000;
#endif

PARAM(notimefix, "[TIME] Disable Time Fix");

unsigned sysTimer::GetSystemMsTime() {
	// This timer is not expected to wrap around during a single run!
#if __WIN32
# if __XENON
	return ::GetTickCount();
# elif RSG_DURANGO
	static LARGE_INTEGER qpcFreq = {0};
	static LARGE_INTEGER qpcInitial = {0};
	if(qpcFreq.QuadPart == 0)
	{
		QueryPerformanceFrequency(&qpcFreq);
		QueryPerformanceCounter(&qpcInitial);
	}
	LARGE_INTEGER qpc;
	QueryPerformanceCounter(&qpc);
	return (unsigned)(((qpc.QuadPart-qpcInitial.QuadPart) * 1000) / qpcFreq.QuadPart);
# else
	return timeGetTime();
# endif
#elif __POSIX
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#elif __PSP2
	// System time is in microseconds, we want milliseconds
	return (u32) (sceKernelGetSystemTimeWide() / 1000);
#elif __PPU
	system_time_t t;
	SYS_TIMEBASE_GET(t);
	return (u32)(t / (CONST_FREQ/1000.0f));
#elif RSG_ORBIS
	return (sceKernelReadTsc() * 1000) / sceKernelGetTscFrequency();
#endif
}

bool sysTimer::HasElapsedIntervalMs(u32 startTime, u32 interval)
{
	return (GetSystemMsTime() - startTime) > interval;
}

#if RSG_ORBIS
sysTimer::sysTimer() {
	if (!GetTicksToSeconds()) {
		unsigned long freq = sceKernelGetTscFrequency();
		sysTimerConsts::TicksToSeconds = 1.0f / freq;
		sysTimerConsts::TicksToMilliseconds = 1E+3f / freq;
		sysTimerConsts::TicksToMicroseconds = 1E+6f / freq;
		sysTimerConsts::TicksToNanoseconds	= 1E+9f / freq;
	}
	Reset();
}

////////////////// windows //////////////////
#elif __WIN32

#if !__XENON
utimer_t sysTimer::GetTicks() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}
#endif


#ifndef CONST_FREQ
sysTimer::sysTimer() {
	if (!GetTicksToSeconds()) {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		sysTimerConsts::TicksToSeconds = 1.0f / freq.QuadPart;
		sysTimerConsts::TicksToMilliseconds = 1E+3f / freq.QuadPart;
		sysTimerConsts::TicksToMicroseconds = 1E+6f / freq.QuadPart;
		sysTimerConsts::TicksToNanoseconds	= 1E+9f / freq.QuadPart;
		sysTimerConsts::CpuSpeed = 0.001f / sysTimerConsts::TicksToMilliseconds;
	}
	Reset();
}
#endif

#if RSG_PC
void sysTimer::UpdateCpuSpeed()
{
	static bool bTimeFix = true;

	// Update timer often
	if (bTimeFix)
	{
#if !__FINAL
		if (bTimeFix)
			bTimeFix = !PARAM_notimefix.Get();
#endif // !__FINAL
		static utimer_t uLastUpdate = 0;
		utimer_t uCurrentTime = timeGetTime();
		if ((uCurrentTime - uLastUpdate) > 1000)
		{
			static LARGE_INTEGER iLastCycleTime = { 0 };
			if (uLastUpdate > 0)
			{
				LARGE_INTEGER iCurrentCycleTime, iFrequency;
				QueryPerformanceFrequency(&iFrequency);
				QueryPerformanceCounter(&iCurrentCycleTime);
				if ((uLastUpdate < uCurrentTime) &&
					(iLastCycleTime.QuadPart < iCurrentCycleTime.QuadPart))
				{
					utimer_t uDeltaTime = uCurrentTime - uLastUpdate;

					double elapsed = (double)iCurrentCycleTime.QuadPart - (double)iLastCycleTime.QuadPart;

					// count / counts/sec = seconds.
					Assert(iFrequency.QuadPart > 0);
					elapsed /= (double)iFrequency.QuadPart;

					// Compute how much we're off actual real-time
					double	fDeltaTime=(double)uDeltaTime/1000.0f;
					double	fRatio=fDeltaTime/elapsed;
					if (fRatio<0.97f || fRatio>1.03f)
					{
						if (( fRatio > 8.0f ) || ( fRatio < ( 1.0f / 8.0f )))
						{
							//Warningf("Screwed up ratio in timer %f", fRatio);
						}
						else
						{
							Assert(fRatio > 0.0f);
							elapsed*=fRatio;
						}
					}
					utimer_t uCycleTime;
					uCycleTime = iCurrentCycleTime.QuadPart - iLastCycleTime.QuadPart;
					
					const float fTickToMilliseconds = (float)((elapsed * 1000.0) / uCycleTime);
					const float fVariance = fTickToMilliseconds / sysTimerConsts::TicksToMilliseconds;
					const float fVarianceRange = 0.1f;

					if ((uCycleTime > 0) && (elapsed > 0.0f) && ((fVariance < (1.0 - fVarianceRange)) || (fVariance > (1.0 + fVarianceRange))))
					{
						//Warningf("Scaling Time %f", fRatio);
						//Warningf("Recalibrating Timers Percentage Variance %f", fVariance);

						sysTimerConsts::TicksToMilliseconds = fTickToMilliseconds;
						sysTimerConsts::TicksToMicroseconds = sysTimerConsts::TicksToMilliseconds * 1000.0f;
						sysTimerConsts::TicksToNanoseconds	= sysTimerConsts::TicksToMicroseconds * 1000.0f;
						sysTimerConsts::TicksToSeconds		= sysTimerConsts::TicksToMilliseconds / 1000.0f;
						sysTimerConsts::CpuSpeed			= 0.001f / sysTimerConsts::TicksToMilliseconds;
					}
				}
			}
			uLastUpdate = timeGetTime();
			QueryPerformanceCounter(&iLastCycleTime);
		}
	}
}
#endif // RSG_PC

static DWORD oldClass,oldPrio;

////////////////// DOS //////////////////
#elif defined(_DOS)

typedef union _LARGE_INTEGER 
{     
	struct 
	{        
		unsigned int LowPart; 
		unsigned int HighPart;     
	};    
} LARGE_INTEGER; 

utimer_t sysTimer::GetTicks() 
{
	LARGE_INTEGER var;
	_asm {
		_emit 0x0F
		_emit 0x31
		mov DWORD PTR var,eax
		mov DWORD PTR var+4,edx
	};

	return var.LowPart;
}


#endif


void sysTimer::BeginBenchmark() {
#if __WIN32PC
	oldClass=GetPriorityClass(GetCurrentProcess());
	oldPrio=GetThreadPriority(GetCurrentThread());

	if(!SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS))
		Errorf("SetPriorityClass failed.");
	if(!SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL))
		Errorf("SetThreadPriority failed.");
#endif
}

void sysTimer::EndBenchmark() {
#if __WIN32PC
	SetPriorityClass(GetCurrentProcess(),oldClass);
	SetThreadPriority(GetCurrentThread(),oldPrio);
#endif
}


#if __PSP2
utimer_t sysTimer::GetTicks() {
	return (u32) (sceKernelGetSystemTimeLow());
}

#endif
