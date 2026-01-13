// 
// grcore/gputrace_gnm.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_GPUTRACE_GNM_H
#define GRCORE_GPUTRACE_GNM_H 

#include "grcore/config.h"

#if RSG_ORBIS

#include <gnm/perfcounter_constants.h>
#include <gnm/drawcommandbuffer.h>
#include <gnm/dispatchcommandbuffer.h>
#include <razor_gpu_thread_trace.h>

namespace rage {

class grcGpuTraceGNM
{
public:
	enum grcTraceFrequency
	{ 
		grcTraceFrequencyLow = 0,
		grcTraceFrequencyMedium,
		grcTraceFrequencyHigh
	};

	static void InitClass();

	static void InitThreadTraceParams(SceRazorGpuThreadTraceParams *params, grcTraceFrequency frequency);
	static bool InitThreadTrace(SceRazorGpuThreadTraceParams *params, int numStreamingCounter);

	static bool Start();
	static bool Stop();
	static bool Save(const char*filename);
	static bool Reset();

	static bool IsCapturing() { return m_isCapturing; }

private:
	static bool m_initialised;
	static bool m_isCapturing;

};

} // namespace rage

#endif // RSG_ORBIS
#endif // GRCORE_GPUTRACE_GNM_H
