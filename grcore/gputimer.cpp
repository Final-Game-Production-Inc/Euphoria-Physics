// 
// grcore/gputimer.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "gputimer.h"
#include "channel.h"
#include "device.h"

#include "system/memops.h"
#include "system/nelem.h"

#if __XENON
#include "system/xtl.h"
#include "system/timer.h"
#elif __PS3
#include "wrapper_gcm.h"
#elif RSG_ORBIS
#include "gfxcontext_gnm.h"
#include "wrapper_gnm.h"
#elif __WIN32PC || RSG_DURANGO
#include "system/xtl.h"
#include "grprofile/pix.h"
#include "grcore/wrapper_d3d.h"
#include "system/d3d11.h"
#if __D3D9
#include "system/d3d9.h"
#elif __D3D11
#include "system/d3d11.h"
#endif
#endif

namespace rage {

#if __PS3
const u32 ReportCount = 6;
#elif (__WIN32PC && __D3D11) || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
ID3D11Query* grcGpuTimeStamp::sm_DisjointQuery[grcGpuTimeStamp::MAX_QUERIES];
u64 grcGpuTimeStamp::sm_Frequency[grcGpuTimeStamp::MAX_QUERIES];
#endif


#if __XENON
void grcGpuTimeStamp::Callback(unsigned long userData)
{
	// No floating point in DPC.  Setting a breakpoint here will crash DevStudio.
	grcGpuTimeStamp *const ts = (grcGpuTimeStamp*)(context & -MAX_QUERIES);
	const unsigned frameIdx = context & (MAX_QUERIES-1);
	QueryPerformanceCounter((LARGE_INTEGER*)(ts->m_Value+frameIdx));
}
#endif // __XENON

#if __WIN32PC || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
#define GPUTIMER_MAX_TIME_STAMPS    ( 512 )
static grcGpuTimeStamp* s_TimeStamps[GPUTIMER_MAX_TIME_STAMPS];
static bool s_QuerySupport = !RSG_FINAL;

#if __D3D9
static float s_TimeStampFreq;
#endif


PARAM(noGPUTimers, "Disable GPU timers for PC");

#if __WIN32PC && __D3D9
void grcGpuTimeStamp::DeviceLostCallback()
{
	for (u32 i = 0; i < NELEM(s_TimeStamps); ++i)
	{
		if (s_TimeStamps[i])
		{
			s_TimeStamps[i]->ReleaseQueries();
		}
	}
}

void grcGpuTimeStamp::DeviceResetCallback()
{
	for (u32 i = 0; i < NELEM(s_TimeStamps); ++i)
	{
		if (s_TimeStamps[i])
		{
			s_TimeStamps[i]->CreateQueries();
		}
	}
}
#endif // __WIN32PC && __D3D9

void grcGpuTimeStamp::CreateQueries()
{
	if (m_Query[0])
	{
		return;
	}

#if __D3D9
	for (u32 i = 0; i < MAX_QUERIES; ++i)
	{
		const HRESULT hr = GRCDEVICE.GetCurrent()->CreateQuery(D3DQUERYTYPE_TIMESTAMP, m_Query+i);
		if (!SUCCEEDED(hr))
		{
			Warningf("CreateQuery failed, hresult=0x%08x", hr);
			m_Query[i] = NULL;
		}
	}
#elif __D3D11
	// http://mynameismjp.wordpress.com/2011/10/13/profiling-in-dx11-with-queries/
	static const D3D11_QUERY_DESC desc = { D3D11_QUERY_TIMESTAMP, 0 };
	for (u32 i = 0; i < MAX_QUERIES; ++i)
	{
		const HRESULT hr = GRCDEVICE.GetCurrent()->CreateQuery(&desc, m_Query+i);
		if (!SUCCEEDED(hr))
		{
			Warningf("CreateQuery failed, hresult=0x%08x", hr);
			m_Query[i] = NULL;
			CheckDxHresultFatal(hr);
		}
	}
	sysMemSet(m_Cached, 0, sizeof(m_Cached));
#endif
}

void grcGpuTimeStamp::ReleaseQueries()
{
	for (u32 i = 0; i < MAX_QUERIES; ++i)
	{
		LastSafeRelease(m_Query[i]);
	}
}

#if __D3D11
void grcGpuTimeStamp::UpdateAllTimers()
{
	Assert( GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() );
	if (!s_QuerySupport)
		return;

	grcContextHandle *const ctx = g_grcCurrentContext;

	const unsigned frameIdx = GRCDEVICE.GetFrameCounter() % MAX_QUERIES;
	const unsigned prevFrameIdx   = (frameIdx-1) % MAX_QUERIES;
	const unsigned oldestFrameIdx = (frameIdx+1) % MAX_QUERIES;
	ctx->End(sm_DisjointQuery[prevFrameIdx]);

	// If the call failed or it indicated something screwed up the counters, bail
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint = { 0, FALSE };
	if (ctx->GetData(sm_DisjointQuery[oldestFrameIdx], &disjoint, sizeof(disjoint), 0) == S_FALSE)
	{
		if (!disjoint.Disjoint)
		{
			PF_PUSH_MARKER("Spinning on Disjoint");

			unsigned spinCount = 32;
			while (ctx->GetData(sm_DisjointQuery[oldestFrameIdx], &disjoint, sizeof(disjoint), 0) == S_FALSE && --spinCount)
				sysIpcSleep( 0 );

			if (spinCount==0)
				disjoint.Disjoint = true;

			PF_POP_MARKER();
		}
	}

	if (disjoint.Disjoint)
		sm_Frequency[oldestFrameIdx] = 0;
	else
	{
		sm_Frequency[oldestFrameIdx] = disjoint.Frequency;
		for (unsigned i=0; i<NELEM(s_TimeStamps); ++i)
		{
			grcGpuTimeStamp *const pTs = s_TimeStamps[i];
			if (pTs != NULL)
			{
				pTs->m_Cached[oldestFrameIdx] = 0;

				ID3D11Query *const pQuery = pTs->m_Query[oldestFrameIdx];
				if (pQuery)
				{
					u64 time;
					if (ctx->GetData(pQuery, &time, sizeof(time), 0) != S_FALSE)
					{
						pTs->m_Cached[oldestFrameIdx] = time;
					}
				}
			}
		}
	}

	ctx->Begin(sm_DisjointQuery[frameIdx]);
}
#endif

#endif // __WIN32PC

void grcGpuTimeStamp::InitClass()
{
#if __WIN32PC && __D3D9
	IDirect3DQuery9* timeStampFreqQuery;
	// TODO: D3D10 port
	if (!GRCDEVICE.GetCurrent() || GRCDEVICE.GetCurrent()->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &timeStampFreqQuery) == D3DERR_NOTAVAILABLE)
	{
		s_QuerySupport = false;
		return;
	}

	s_QuerySupport = true;
	timeStampFreqQuery->Issue(D3DISSUE_END);
	u64 timeStampFreq = 0;
	timeStampFreqQuery->GetData(&timeStampFreq, sizeof(timeStampFreq), D3DGETDATA_FLUSH);
	timeStampFreqQuery->Release();
	s_TimeStampFreq = (float)timeStampFreq;

	GRCDEVICE.RegisterDeviceLostCallbacks(MakeFunctor(&DeviceLostCallback), MakeFunctor(&DeviceResetCallback));
#endif

#if (__WIN32PC && __D3D11) || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	if (PARAM_noGPUTimers.Get())
		s_QuerySupport = false;

	D3D11_QUERY_DESC descDisjoint = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
	for (u32 i = 0; i < MAX_QUERIES; ++i)
	{
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateQuery(&descDisjoint, &sm_DisjointQuery[i]));
	}
#endif
}

void grcGpuTimeStamp::ShutdownClass()
{
#if __WIN32PC
	if (!s_QuerySupport)
		return;

	for (u32 i = 0; i < NELEM(s_TimeStamps); ++i)
	{
		if (s_TimeStamps[i])
		{
			s_TimeStamps[i]->ReleaseQueries();
			s_TimeStamps[i] = NULL;
		}
	}
#endif // __WIN32PC
}

grcGpuTimeStamp::grcGpuTimeStamp()
{
#if __XENON
	sysMemSet(m_Value, 0, sizeof(m_Value));
#elif __PS3
	m_ReportBase = gcm::TimeStampRegistrar::Allocate(MAX_QUERIES);
	Assert(gcm::TimeStampRegistrar::IsValid(m_ReportBase));
#elif (RSG_DURANGO&&!GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES) || RSG_ORBIS
	m_TimerMemory = rage_new u64[MAX_QUERIES];
#elif __WIN32PC || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	sysMemSet(m_Query,  0, sizeof(m_Query));
#	if __D3D11
		sysMemSet(m_Cached, 0, sizeof(m_Cached));
#	endif

	for (u32 i = 0; i < NELEM(s_TimeStamps); ++i)
	{
		if (!s_TimeStamps[i])
		{
			s_TimeStamps[i] = this;
			return;
		}
	}

	grcErrorf("No free entry for timer in free list");
#endif
}

grcGpuTimeStamp::~grcGpuTimeStamp()
{
#if __XENON
	// Should probably insert a fence here or something.
	// Hopefully in practice the object won't disappear before the callback scribbles on it.
#elif __PS3
	gcm::TimeStampRegistrar::Free(m_ReportBase, MAX_QUERIES);
#elif (RSG_DURANGO&&!GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES) || RSG_ORBIS
	delete[] m_TimerMemory;
#elif __WIN32PC || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	if (s_QuerySupport)
	{
		ReleaseQueries();
	}

	for (u32 i = 0; i < NELEM(s_TimeStamps); ++i)
	{
		if (s_TimeStamps[i] == this)
		{
			s_TimeStamps[i] = NULL;
			return;
		}
	}

	// Due to static initialisation/shutdown we could have been freed by grcGpuTimer::ShutdownClass
	// before the dtor is called
	//grcErrorf("Timer does not live in free list");
#endif
}

void grcGpuTimeStamp::Write()
{
	const unsigned frameIdx = GRCDEVICE.GetFrameCounter() % MAX_QUERIES;

#if __XENON
	CompileTimeAssert(__alignof(*this) >= MAX_QUERIES);
	GRCDEVICE.GetCurrent()->InsertCallback(D3DCALLBACK_IMMEDIATE, Callback, (DWORD)this | frameIdx);
#elif __PS3
	GCM::cellGcmSetTimeStamp(GCM_CONTEXT,m_ReportBase + frameIdx);
#elif RSG_DURANGO && !GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	if (g_grcCurrentContext == NULL)
		return;
	g_grcCurrentContext->WriteTimestampToMemory(m_TimerMemory + frameIdx);
#elif RSG_ORBIS
	gfxc.writeAtEndOfPipe(
		sce::Gnm::kEopFlushCbDbCaches,
		sce::Gnm::kEventWriteDestMemory, m_TimerMemory + frameIdx,
		sce::Gnm::kEventWriteSourceGpuCoreClockCounter, 0,
		sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
#elif __WIN32PC || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	if (!s_QuerySupport || g_grcCurrentContext == NULL)
		return;
	CreateQueries();
	if (m_Query[frameIdx])
	{
#if __D3D9
		m_Query[frameIdx]->Issue(D3DISSUE_END);
#elif __D3D11
		g_grcCurrentContext->End(m_Query[frameIdx]);
#endif
	}
#endif
}

/*static*/ float grcGpuTimeStamp::ElapsedMs(const grcGpuTimeStamp &from, const grcGpuTimeStamp &until)
{
	const unsigned frameIdx = (GRCDEVICE.GetFrameCounter() + 1) % MAX_QUERIES;

#if __XENON
	const float conversion = 1000.0f / CONST_FREQ;
	u64 begin = from.m_Value[frameIdx];
	u64 end   = until.m_Value[frameIdx];
	until elapsed = end - begin;
	return elapsed * conversion;
#elif __PS3
	u64 begin = cellGcmGetTimeStampLocation(from.m_ReportBase + frameIdx, CELL_GCM_LOCATION_MAIN);
	u64 end   = cellGcmGetTimeStampLocation(from.m_ReportBase + frameIdx, CELL_GCM_LOCATION_MAIN);
	return (begin != 0 && end != 0 && end > begin) ? (end - begin) * (1.0f / 1000000.0f) : 0.0f;
#elif RSG_DURANGO && !GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	u64 begin = from.m_TimerMemory[frameIdx];
	u64 end   = until.m_TimerMemory[frameIdx];
	u64 elapsed = end - begin;
	return elapsed * (1000.0f / 100000000.0f); // doesn't seem to be a constant for this, but xb1 docs say it is 100MHz
#elif RSG_ORBIS
	u64 begin = from.m_TimerMemory[frameIdx];
	u64 end   = until.m_TimerMemory[frameIdx];
	u64 elapsed = end - begin;
	return elapsed * (1000.0f / SCE_GNM_GPU_CORE_CLOCK_FREQUENCY);
#elif __WIN32PC || GPUTIMER_DURANGO_USE_STOCK_D3D11_QUERIES
	if (!s_QuerySupport || g_grcCurrentContext == NULL)
		return 0.0f;
#if __D3D9
	u64 begin, end;
	if (from .m_Query[frameIdx]->GetData(&begin, sizeof(begin), 0) == S_FALSE ||
	    until.m_Query[frameIdx]->GetData(&end,   sizeof(end),   0) == S_FALSE)
	{
		return 0.0f;
	}
	return (float)(end - begin) * 1000.0f / s_TimeStampFreq;
#elif __D3D11
	u64 freq = sm_Frequency[frameIdx];
	if (!freq)
		return 0.f;

	u64 begin = from.m_Cached[frameIdx];
	u64 end   = until.m_Cached[frameIdx];
	u64 elapsed = end - begin;
	return 1000.0f * elapsed / freq;
#endif
#else
	return 0.0f;
#endif
}

/*static*/ float grcGpuTimeStamp::GetIdleTimeMs(const grcGpuTimeStamp &from, const grcGpuTimeStamp &until)
{
#if RSG_ORBIS && GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
	const unsigned frameIdx = (GRCDEVICE.GetFrameCounter() + 1) % MAX_QUERIES;
	u64 begin = from.m_TimerMemory[frameIdx];
	u64 end   = until.m_TimerMemory[frameIdx];
	u64 idle  = grcGfxContext::getRecordedGpuIdleBetweenTimestamps(begin, end);
	return 1000.0f * idle / SCE_GNM_GPU_CORE_CLOCK_FREQUENCY;
#else
	(void) from;
	(void) until;
	return 0.0f;
#endif
}

void grcGpuTimer::Start()
{
	m_Start.Write();
}

float grcGpuTimer::Stop()
{
	m_Stop.Write();
	return grcGpuTimeStamp::ElapsedMs(m_Start, m_Stop);
}

float grcGpuTimer::GetIdleTimeMs()
{
	return grcGpuTimeStamp::GetIdleTimeMs(m_Start, m_Stop);
}

} // namespace rage
