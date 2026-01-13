#include "gcmtrace.h"
#if __PPU && RAGETRACE
#include "grcore/wrapper_gcm.h"
#include "grcore/device.h"

namespace rage {

static u32 NumTimeStampsToUse = 0x400;

static u32 FirstTimeStampToUse = 0;
static u32 LastTimeStampToUse = 0;

pfGcmTrace* g_pfGcmTrace = 0;

pfGcmTrace::pfGcmTrace()
:	pfTrace("RSX", rage_new pfTraceCounter(&m_RootId))
,	m_RootId("RSX", Color32(0))
,	m_GotNextTime(false)
,	m_WriteFrameTime(0)
,	m_ReadFrameTime(0)
,	m_BaseGpuTime(0)
{	
	if (!LastTimeStampToUse)
	{
		FirstTimeStampToUse = gcm::TimeStampRegistrar::Allocate(NumTimeStampsToUse);
		Assert(gcm::TimeStampRegistrar::IsValid(FirstTimeStampToUse));
		LastTimeStampToUse = FirstTimeStampToUse + NumTimeStampsToUse;
	}
	m_ReadTimeStamp = FirstTimeStampToUse;
	m_WriteTimeStamp = FirstTimeStampToUse;

	// ensure the first thing to happen is a frame start or it'll never get going
	StartFrame();
}

void pfGcmTrace::StartFrame()
{
	// record frame start time so we can map GPU times to CPU times
	m_FrameTime[m_WriteFrameTime] = sysTimer::GetTicks();
	m_FrameTimeStart[m_WriteFrameTime] = m_WriteTimeStamp;
	m_WriteFrameTime = (m_WriteFrameTime + 1) & 7;
}

void pfGcmTrace::Start(pfTraceCounterId* identifier)
{
	Assert(m_WriteTimeStamp && m_WriteTimeStamp < GCM_REPORT_COUNT);
	cellGcmSetTimeStamp(GCM_CONTEXT, m_WriteTimeStamp);
	m_GcmCounters[m_WriteTimeStamp] = identifier;
	if (++m_WriteTimeStamp == LastTimeStampToUse)
		m_WriteTimeStamp = FirstTimeStampToUse;
	AssertMsg(m_WriteTimeStamp != m_ReadTimeStamp, "Ran out of GCM time stamps");
}

void pfGcmTrace::Stop()
{
	Assert(m_WriteTimeStamp && m_WriteTimeStamp < GCM_REPORT_COUNT);
	cellGcmSetTimeStamp(GCM_CONTEXT, m_WriteTimeStamp);
	m_GcmCounters[m_WriteTimeStamp] = 0;
	if (++m_WriteTimeStamp == LastTimeStampToUse)
		m_WriteTimeStamp = FirstTimeStampToUse;
	AssertMsg(m_WriteTimeStamp != m_ReadTimeStamp, "Ran out of GCM time stamps");
}

u32 pfGcmTrace::GetNextSample()
{	
	// check if we've got any new data to read
	if (m_ReadTimeStamp == m_WriteTimeStamp) 
		return 0;
	// make sure we're 3 frames ahead of the GCM
#if !HACK_GTA4
	if ((m_WriteFrameTime - m_ReadFrameTime) < 3) 
		return 0;
#endif // !HACK_GTA4
	// cached?
	if (m_GotNextTime)
		return m_NextTime;

	u64 gputime = cellGcmGetTimeStampLocation(m_ReadTimeStamp, CELL_GCM_LOCATION_MAIN);
	if (m_ReadTimeStamp == m_FrameTimeStart[m_ReadFrameTime])
	{
		// resync against cpu
		m_BaseGpuTime = gputime;
		m_BaseCpuTime = m_FrameTime[m_ReadFrameTime];
		m_ReadFrameTime = (m_ReadFrameTime + 1) & 7;
	}

	// convert time from nanoseconds to cpu ticks
	if (s64(gputime - m_BaseGpuTime) < 0)
		gputime = m_BaseGpuTime;
	u32 cputime = (u32)((gputime - m_BaseGpuTime) * 
		(1e-9 / sysTimer::GetTicksToSeconds())) + m_BaseCpuTime;
	// ensure non-zero
	m_NextTime = cputime|1;
	m_GotNextTime = true;
	return m_NextTime;
}

void pfGcmTrace::AdvanceTo(u32 timebase)
{
	while (1)
	{
		u32 nextsample = GetNextSample();
		if (!nextsample || s32(timebase - nextsample) < 0)
			return;
		pfTraceCounterId* id = m_GcmCounters[m_ReadTimeStamp];
		if (id)
			PushCounter(nextsample, id);
		else
			PopCounter(nextsample);
		if (++m_ReadTimeStamp == LastTimeStampToUse)
			m_ReadTimeStamp = FirstTimeStampToUse;
		m_GotNextTime = false;
	}
}

} // namespace rage

#endif // __PPU && RAGETRACE
