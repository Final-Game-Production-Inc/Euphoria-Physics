//
// grcore/gputimer.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_GPUTIMER_H
#define GRCORE_GPUTIMER_H

#include "grcore/config.h"

#if !RSG_DURANGO
#define GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES 0
#else
#include <xdk.h>
// July 2014 XDK adds ID3D11DeviceContextX::WriteTimestampToMemory, before that
// we need to use the stock D3D11 queries.
#define GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES (_XDK_VER < 11274)
#endif

#if __WIN32PC || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
struct IDirect3DQuery9;
struct ID3D11Query;
#endif

namespace rage {

class grcGpuTimeStamp
{
public:

#if !__WIN32PC
	// 3 is probably enough but 4 avoids expensive modulo calculations in DPC's.
	// Also means that frame counter wrapping is handled correctly.
	enum { MAX_QUERIES = 4 };
#else
	// Doubled for SLI.
	enum { MAX_QUERIES = 8 };
#endif

	static void InitClass();
	static void ShutdownClass();

	grcGpuTimeStamp();
	~grcGpuTimeStamp();

	void Write();

	static float ElapsedMs(const grcGpuTimeStamp &from, const grcGpuTimeStamp &until);

	// PURPOSE: Returns the amount of GPU idle time between the last Start/Stop pair (in milliseconds).
	static float GetIdleTimeMs(const grcGpuTimeStamp &from, const grcGpuTimeStamp &until);

#if (__WIN32PC && __D3D11) || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	// PURPOSE:	Gets timings for all active gpuTimers
	// NOTES:	We can't call GetData() on a deferred context on d3d11 (the actual point-of-call
	//			so call this once per frame to cache a time from MaxQueries-1 frames ago
	//			to return when Stop() is called.
	static void	UpdateAllTimers(); // Grab and cache all timers once per frame.
#endif

private:

	grcGpuTimeStamp(const grcGpuTimeStamp&);
	grcGpuTimeStamp &operator=(const grcGpuTimeStamp&);

#if RSG_XENON
	static void Callback(unsigned long);
	volatile u64 m_Value[MAX_QUERIES];
#elif RSG_PS3
	u32 m_ReportBase;
#elif (RSG_DURANGO&&!GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES) || RSG_ORBIS
	u64 *m_TimerMemory;

#elif __WIN32PC || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES

#if __WIN32PC && __D3D9
	static void DeviceLostCallback();
	static void DeviceResetCallback();
#endif

	void CreateQueries();
	void ReleaseQueries();

#if __D3D9
	IDirect3DQuery9* m_Query[MAX_QUERIES];
#elif __D3D11
	static ID3D11Query* sm_DisjointQuery[MAX_QUERIES];
	static u64 sm_Frequency[MAX_QUERIES];
	ID3D11Query* m_Query[MAX_QUERIES];
	u64 m_Cached[MAX_QUERIES];
#endif
#endif
};


class grcGpuTimer
{
public:

	// PURPOSE:	Begins timing GPU from here.
	// NOTES:	Any given timer should only be used once per frame.
	//			Timers should be created on the heap, not on the stack,
	//			and should be destroyed at least a frame after they're
	//			last used.  Even better, declare them at static/global scope.
	void Start();

	// PURPOSE:	Stops the GPU timer and returns elapsed time (in milliseconds)
	// NOTES:	Any given timer should only be used once per frame.
	//			On PS3, latency is extremely long so they're internally double-buffered.
	//			You are actually getting the result of the PREVIOUS timer's run.
	float Stop();

	// PURPOSE: Returns the amount of GPU idle time between the last Start/Stop pair (in milliseconds).
	float GetIdleTimeMs();

	grcGpuTimeStamp &GetStart() { return m_Start; }
	grcGpuTimeStamp &GetStop()  { return m_Stop;  }

private:
	grcGpuTimeStamp m_Start;
	grcGpuTimeStamp m_Stop;
};

} // namespace rage

#endif // GRCORE_GPUTIMER_H
