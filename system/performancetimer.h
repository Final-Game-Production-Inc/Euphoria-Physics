// 
// system/performancetimer.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_PERFORMANCETIMER_H
#define SYSTEM_PERFORMANCETIMER_H

#include "system/timer.h"

namespace rage
{

/*
PURPOSE
	This class implements a high resolution performance timer object.
	Useful for profiling. 

SEE ALSO:
	sysTimeManger, TIME, sysTimer
<FLAG Component>
*/

class sysPerformanceTimer
{
public:
	sysPerformanceTimer(const char* szName);

	// PURPOSE: Starts timing
	void Start();

	// PURPOSE: Stops timing
	void Stop();

	// PURPOSE: Resets time counter to zero
	void Reset();

#if !__NO_OUTPUT
	// PURPOSE: Prints the current time counter in milliseconds to stdout
	void Print(bool bNewline = true);

	// PURPOSE: Prints the current time counter in milliseconds to stdout and resets the timer
	void PrintReset(bool bNewline = true);
#endif

	// RETURNS: The current time counter in milliseconds
	float GetTimeMS();

	// RETURNS: Current elapsed time since start time;
	float GetElapsedTimeMS();

private:
	char		m_szTimerName[16];
	utimer_t	m_TimeCounter;
	utimer_t	m_StartTime;
	bool		m_bTiming;
};

} // namespace rage

#endif // SYSTEM_PERFORMANCETIMER_H
