// 
// system/performancetimer.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "performancetimer.h"
#include "timer.h"
#include "string/string.h"
#include "system/memops.h"

#include <string.h>

namespace rage
{

// Need to make sure a timer gets constructed so that conversion factors
// are calibrated to processor speed on PC.
static sysTimer dummyTimer;

sysPerformanceTimer::sysPerformanceTimer(const char* szName)
{
//	sysMemCpy(m_szTimerName, szName, sizeof(m_szTimerName));
	safecpy(m_szTimerName, szName);
	Reset();
}

void sysPerformanceTimer::Start()
{
	Assert(!m_bTiming);
	m_bTiming = true;
	m_StartTime = sysTimer::GetTicks();
}

void sysPerformanceTimer::Stop()
{
	utimer_t elapsed = sysTimer::GetTicks() - m_StartTime;
	m_TimeCounter += elapsed;
	Assert(m_bTiming);
	m_bTiming = false;
}

void sysPerformanceTimer::Reset()
{
	m_TimeCounter = 0;
	m_bTiming = false;
}

#if !__NO_OUTPUT
void sysPerformanceTimer::Print(bool bNewline)
{
	float fTime = GetTimeMS();
	if( bNewline )
	{
		Printf("%s: %f\n", m_szTimerName, fTime);
	}
	else
	{
		Printf("%s: %f\t", m_szTimerName, fTime);
	}
}

void sysPerformanceTimer::PrintReset(bool bNewline)
{
	Print(bNewline);
	Reset();
}
#endif

float sysPerformanceTimer::GetTimeMS()
{
	return m_TimeCounter * sysTimer::GetTicksToMilliseconds();
}

float sysPerformanceTimer::GetElapsedTimeMS()
{
	utimer_t elapsed = sysTimer::GetTicks() - m_StartTime;
	return elapsed * sysTimer::GetTicksToMilliseconds();
}


} // namespace rage
