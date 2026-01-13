//
// system/timer.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_TIMER_H
#define SYSTEM_TIMER_H


#if __XENON
extern "C" {
	unsigned __int64 __mftb();
}
#elif __PPU
#include <sys/time_util.h>
#elif RSG_ORBIS
extern "C" unsigned long sceKernelReadTsc(void);
extern "C" unsigned long sceKernelGetTscFrequency(void);
#endif

namespace rage {

#if __XENON
#define CONST_FREQ	49875000.0f			// QPF returns the wrong value.
#elif defined (_DOS)
#define CONST_FREQ	448053664.0f
#elif __PSP2
#define CONST_FREQ	1000000.0f			// System timer returns microseconds.
#elif __SPU
#include <spu_intrinsics.h>
#define CONST_FREQ	79800000.0f			// System timebase is ~80Mhz
#elif __PPU
#define CONST_FREQ	79800000.0f			// System timebase is ~80Mhz
#endif

#if __WIN32
#pragma warning(disable: 4514)	// Unreferenced inline removed
#endif

// The raw data type used for time values in timers
#if __SPU
typedef u32 utimer_t;		// Use exact type here because we ignore wraparound.
#else
typedef s64 utimer_t;
#endif

#ifdef CONST_FREQ
// Constants for use by the system timer
namespace sysTimerConsts
{
	// Conversion factor between ticks and seconds
	const float TicksToSeconds = (1.0f / CONST_FREQ);
	
	// Conversion factor between ticks and milliseconds
	const float TicksToMilliseconds = (1000.0f / CONST_FREQ);

	// Conversion factor between ticks and microseconds
	const float TicksToMicroseconds = (1000000.0f / CONST_FREQ);

	// Conversion factor between ticks and nanoseconds
	const float TicksToNanoseconds = (1000000000.0f / CONST_FREQ);

	// The speed of the CPU
	const float CpuSpeed = (CONST_FREQ * 0.000001f);
}
#else
// Constants for use by the system timer
struct sysTimerConsts
{
	// Conversion factor between ticks and seconds
	static float TicksToSeconds;
	
	// Conversion factor between ticks and milliseconds
	static float TicksToMilliseconds;

	// Conversion factor between ticks and microseconds
	static float TicksToMicroseconds;

	// Conversion factor between ticks and nanoseconds
	static float TicksToNanoseconds;

	// The speed of the CPU
	static float CpuSpeed;
};
#endif

/*
PURPOSE
	This class implements a timer object.
	Useful for profiling. 

NOTES
	For a platform without a fixed CPU speed (e.g. __WIN32PC) the sysTimerConsts are zero until the first
	sysTimer object is constructed, at which point the CPU speed is measured. It is not measured again
	on subsequent constructions.

SEE ALSO:
	sysTimeManger, TIME
<FLAG Component>
*/
class sysTimer {
public:
	//PURPOSE: Creates a new sysTimer.
	//NOTES:
	//	The timer is reset on construction, so time is measured since the timer is created.
	//	On platforms without a fixed CPU speed, the first call to this function will measure the CPU speed.
	//  (and sleeps approximately 100ms)
	sysTimer();

	//PURPOSE: Resets the timer
	void	Reset();

	//PURPOSE: Return the time since construction or last Reset in seconds.
	//RETURNS: Time in seconds.
	float	GetTime() const;

	//PURPOSE: Return the time since construction or last Reset in seconds and reset the timer.
	//RETURNS: Time in seconds.
	float	GetTimeAndReset();

	//PURPOSE: Return the time since construction or last Reset in milli-seconds.
	//RETURNS: Time in milli-seconds.
	float	GetMsTime() const;

	//PURPOSE: Return the time since construction or last Reset in milli-seconds and reset the timer.
	//RETURNS: Time in milli-seconds.
	float	GetMsTimeAndReset();

	//PURPOSE: Return the time since construction or last Reset in micro-seconds.
	//RETURNS: Time in micro-seconds.
	float	GetUsTime() const;

	//PURPOSE: Return the time since construction or last Reset in micro-seconds and reset the timer.
	//RETURNS: Time in micro-seconds.
	float	GetUsTimeAndReset();

	//PURPOSE: Return the time since construction or last Reset in nano-seconds.
	//RETURNS: Time in nano-seconds.
	float	GetNsTime() const;

	//PURPOSE: Return the time since construction or last Reset in nano-seconds and reset the timer.
	//RETURNS: Time in nano-seconds.
	float	GetNsTimeAndReset();

	//PURPOSE: Return the amt of ticks since construction or last Reset.
	//RETURNS: Delta in num ticks.
	utimer_t	GetTickTime() const;

	// PURPOSE
	//	Returns number of milliseconds elapsed since some point in time
	//	(typically system startup, but you shouldn't count on that)
	// NOTES
	//	Higher-level code should be able to assume that this function
	//	will never overflow during a single run.  Overflow will happen
	//	after 1193 hours (nearly fifty days).
	static u32 GetSystemMsTime();

	// PURPOSE
	//	Returns TRUE if the delta between the GetSystemMsTime() and the 'startTime' exceeds 'elapsed'
	//	Useful for when setting intervals or minimum times between retries
	static bool HasElapsedIntervalMs(u32 startTime, u32 interval);

	//PURPOSE
	//	Begin the benchmark. Raises process priority temporarily so that timer 
	//  results are more accurate.  This is useful for testing a small subroutine.
	//NOTES
	//	Only works on <c>__WIN32</c> platforms
	static void BeginBenchmark();

	//PURPOSE: End the benchmark. Restores process priority to previous value. 
	//NOTES
	//	Only works on <c>__WIN32</c> platforms
	static void EndBenchmark();

	// PURPOSE: Returns the conversion factor for converting from ticks to seconds.
	static float GetTicksToSeconds();

	// PURPOSE: Returns the conversion factor for converting from ticks to milliseconds.
	static float GetTicksToMilliseconds();

	// PURPOSE: Returns the conversion factor for converting from ticks to microseconds.
	static float GetTicksToMicroseconds();

	// PURPOSE: Returns the conversion factor for converting from ticks to nanoseconds.
	static float GetTicksToNanoseconds();

	// PURPOSE: Returns the CPU speed in MHz.
	// RETURNS: CPU speed, or 0.0f if speed is unknown.
	static float GetCpuSpeed();

#if RSG_PC
	// PURPOSE: To correct CPU speed queries if power management is changing the clock speeds.
	static void UpdateCpuSpeed();
#endif

#if __XENON
	// PURPOSE: Returns the raw tick count. On many systems this is a cycle count
	static utimer_t GetTicks() { 
		utimer_t result;
		do { 
			result = __mftb(); 
		} while ((u32)result == 0);
		return result;
	}
#elif __SPU
	static utimer_t GetTicks() {
		return ~spu_readch(SPU_RdDec);
	}
#elif __PPU
	static utimer_t GetTicks() {
		unsigned long long result;
		SYS_TIMEBASE_GET(result);
		return result;
	}
#elif RSG_ORBIS
	static utimer_t GetTicks() {
		return sceKernelReadTsc();
	}
#else
	// PURPOSE: Returns the raw tick count. On many systems this is a cycle count
	static utimer_t GetTicks();
#endif

#if !__FINAL || __FINAL_LOGGING
	static u32& GetStallThreshold() { return sm_StallThreshold; }
#endif

private:
	// unsigned instead of ulong because PSX2 ulong is 64 bits
	// but the timer hardware is only 32 bits.
	volatile utimer_t m_Start;

#if !__FINAL || __FINAL_LOGGING
	static u32 sm_StallThreshold;	// time that it takes to trigger a POSSIBLE STALL Message
#endif
};

#if defined(CONST_FREQ)
inline sysTimer::sysTimer()
{
	Reset();
}
#endif

inline void sysTimer::Reset()
{
	m_Start=GetTicks();
}

inline float sysTimer::GetTicksToSeconds()
{
	return sysTimerConsts::TicksToSeconds;
}

inline float sysTimer::GetTicksToMilliseconds()
{
	return sysTimerConsts::TicksToMilliseconds;
}

inline float sysTimer::GetTicksToMicroseconds()
{
	return sysTimerConsts::TicksToMicroseconds;
}

inline float sysTimer::GetTicksToNanoseconds()
{
	return sysTimerConsts::TicksToNanoseconds;
}

inline float sysTimer::GetCpuSpeed()
{
	return sysTimerConsts::CpuSpeed;
}

inline float sysTimer::GetTime() const
{
	// It's safe to ignore any carry here.
	utimer_t delta = GetTicks() - m_Start;
	return delta * sysTimerConsts::TicksToSeconds;
}

inline float sysTimer::GetTimeAndReset()
{
	// It's safe to ignore any carry here.
	utimer_t now = GetTicks();
	utimer_t delta = now - m_Start;
	m_Start = now;
	return delta * sysTimerConsts::TicksToSeconds;
}

inline float sysTimer::GetMsTime() const
{
	// It's safe to ignore any carry here.
	utimer_t delta = GetTicks() - m_Start;
	return delta * sysTimerConsts::TicksToMilliseconds;
}

inline float sysTimer::GetMsTimeAndReset()
{
	// It's safe to ignore any carry here.
	utimer_t now = GetTicks();
	utimer_t delta = now - m_Start;
	m_Start = now;
	return delta * sysTimerConsts::TicksToMilliseconds;
}

inline float sysTimer::GetUsTime() const
{
	// It's safe to ignore any carry here.
	utimer_t delta = GetTicks() - m_Start;
	return delta * sysTimerConsts::TicksToMicroseconds;
}

inline float sysTimer::GetUsTimeAndReset()
{
	// It's safe to ignore any carry here.
	utimer_t now = GetTicks();
	utimer_t delta = now - m_Start;
	m_Start = now;
	return delta * sysTimerConsts::TicksToMicroseconds;
}

inline float sysTimer::GetNsTime() const
{
	// It's safe to ignore any carry here.
	utimer_t delta = GetTicks() - m_Start;
	return delta * sysTimerConsts::TicksToNanoseconds;
}

inline float sysTimer::GetNsTimeAndReset()
{
	// It's safe to ignore any carry here.
	utimer_t now = GetTicks();
	utimer_t delta = now - m_Start;
	m_Start = now;
	return delta * sysTimerConsts::TicksToNanoseconds;
}

inline utimer_t sysTimer::GetTickTime() const
{
	utimer_t delta = GetTicks() - m_Start;
	return delta;
}

} // namespace
#endif

