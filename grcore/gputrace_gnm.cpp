// 
// grcore/gputrace_gnm.cpp
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "gputrace_gnm.h"

#if RSG_ORBIS && !__FINAL
#include "device.h"
#include "gfxcontext_gnm.h"
#include "system/nelem.h"

#include <libsysmodule.h>

#pragma comment(lib,"SceRazorGpuThreadTrace_stub_weak")

namespace rage {
extern __THREAD grcContextHandle *g_grcCurrentContext;

bool grcGpuTraceGNM::m_initialised;
bool grcGpuTraceGNM::m_isCapturing;

void grcGpuTraceGNM::InitClass()
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_RAZOR_GPU_THREAD_TRACE);
	m_initialised = false;
	m_isCapturing = false;
}

void grcGpuTraceGNM::InitThreadTraceParams(SceRazorGpuThreadTraceParams *params, grcTraceFrequency frequency)
{
	Assert(m_initialised == false);

	SceRazorGpuThreadTraceCounterRate traceFrequencyRemap[] = { SCE_RAZOR_GPU_THREAD_TRACE_COUNTER_RATE_LOW,
																SCE_RAZOR_GPU_THREAD_TRACE_COUNTER_RATE_MEDIUM,
																SCE_RAZOR_GPU_THREAD_TRACE_COUNTER_RATE_HIGH };

	SceRazorGpuThreadTraceStreamingCounterRate streamingFrequencyRemap[] = {	SCE_RAZOR_GPU_THREAD_TRACE_STREAMING_COUNTER_RATE_LOW,
																			SCE_RAZOR_GPU_THREAD_TRACE_STREAMING_COUNTER_RATE_MEDIUM,
																			SCE_RAZOR_GPU_THREAD_TRACE_STREAMING_COUNTER_RATE_HIGH };

	// initialize thread trace 
	sce::Gnm::SqPerfCounter	counters[] = 
	{
		sce::Gnm::kSqPerfCounterWaveCycles,		
		sce::Gnm::kSqPerfCounterWaveReady,		
		sce::Gnm::kSqPerfCounterInsts,			
		sce::Gnm::kSqPerfCounterInstsValu,		
		sce::Gnm::kSqPerfCounterWaitCntAny,		
		sce::Gnm::kSqPerfCounterWaitCntVm,		
		sce::Gnm::kSqPerfCounterWaitCntExp,		
		sce::Gnm::kSqPerfCounterWaitExpAlloc,	
		sce::Gnm::kSqPerfCounterWaitAny,		
		sce::Gnm::kSqPerfCounterIfetch,			
		sce::Gnm::kSqPerfCounterWaitIfetch,		
		sce::Gnm::kSqPerfCounterSurfSyncs,		
		sce::Gnm::kSqPerfCounterEvents,			
		sce::Gnm::kSqPerfCounterInstsBranch,	
		sce::Gnm::kSqPerfCounterValuDepStall,	
		sce::Gnm::kSqPerfCounterCbranchFork		
	};

	uint32_t numCounters = NELEM(counters);

	Assert(params);
	memset(params, 0, sizeof(SceRazorGpuThreadTraceParams));
	params->sizeInBytes = sizeof(SceRazorGpuThreadTraceParams);
	memcpy(params->counters, counters, sizeof(counters));
	params->numCounters = numCounters;
	params->counterRate = traceFrequencyRemap[frequency];
	params->enableInstructionIssueTracing = true;
	params->shaderEngine0ComputeUnitIndex = 0;
	params->shaderEngine1ComputeUnitIndex = 9;
	params->streamingCounterRate = streamingFrequencyRemap[frequency];
}

bool grcGpuTraceGNM::InitThreadTrace(SceRazorGpuThreadTraceParams *params, int numStreamingCounter)
{
	Assert(m_initialised == false);

	params->numStreamingCounters = numStreamingCounter;
	SceRazorGpuErrorCode ret = sceRazorGpuThreadTraceInit(params);
	m_initialised = (ret == SCE_OK);

	return m_initialised;
}

bool grcGpuTraceGNM::Start()
{
	Assert(m_initialised);
	Assert(m_isCapturing == false);
	SceRazorGpuErrorCode ret = sceRazorGpuThreadTraceStart(&gfxc.m_dcb);

	m_isCapturing = (ret == SCE_OK);
	return m_isCapturing;
}

bool grcGpuTraceGNM::Stop()
{
	Assert(m_initialised);
	Assert(m_isCapturing);

	SceRazorGpuErrorCode ret = sceRazorGpuThreadTraceStop(&gfxc.m_dcb);
	m_isCapturing = false;
	return (ret == SCE_OK);
}

bool grcGpuTraceGNM::Save(const char*filename)
{
	Assert(m_initialised);
	Assert(m_isCapturing == false);

	char finalFilename[256];
	sprintf_s(finalFilename, 256, "/hostapp/%s.rtt", filename);
	SceRazorGpuErrorCode ret = sceRazorGpuThreadTraceSave(finalFilename);
	return (ret == SCE_OK);
}

bool grcGpuTraceGNM::Reset()
{
	Assert(m_initialised);
	Assert(m_isCapturing == false);

	SceRazorGpuErrorCode ret = sceRazorGpuThreadTraceReset();

	return (ret == SCE_OK);
}

} // namespace rage

#endif // RSG_ORBIS && !__FINAL


#if 0
	////SceRazorGpuErrorCode ret = 0;
	//uint32_t *streamingCounters = params->streamingCounters;

	// ----- global streaming counters

#if 0
	// TCC
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTccPerfCounterMcWrreq);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTccPerfCounterWrite);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTccPerfCounterMcRdreq);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTccPerfCounterRead);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTccPerfCounterHit);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTccPerfCounterMiss);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTccPerfCounterReq);

	// TCA
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTcaPerfCounterCycle); 
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTcaPerfCounterBusy);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTcaPerfCounterReqTcs);

	// IA
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kIaPerfCounterIaBusy);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kIaPerfCounterIaDmaReturn);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kIaPerfCounterIaStalled);

	// ----- SE streaming counters

	// SX
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kSxPerfCounterPaIdleCycles, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kSxPerfCounterPaReq, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kSxPerfCounterPaPos, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kSxPerfCounterClock, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kSxPerfCounterShPosStarve, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kSxPerfCounterShColorStarve, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kSxPerfCounterShPosStall, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kSxPerfCounterShColorStall, SCE_RAZOR_GPU_BROADCAST_ALL);

	// PA-SU
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kPaSuPerfCounterPaInputPrim, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kPaSuPerfCounterPasxReq, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kPaSuPerfCounterPaInputEndOfPacket, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kPaSuPerfCounterPaInputNullPrim, SCE_RAZOR_GPU_BROADCAST_ALL);

	// VGT
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kVgtPerfCounterVgtSpiVsvertSend, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kVgtPerfCounterVgtSpiVsvertEov, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kVgtPerfCounterVgtSpiVsvertStalled, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kVgtPerfCounterVgtSpiVsvertStarvedBusy, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kVgtPerfCounterVgtSpiVsvertStarvedIdle, SCE_RAZOR_GPU_BROADCAST_ALL);

	// TA
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTaPerfCounterBufferWavefronts, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTaPerfCounterImageReadWavefronts, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTaPerfCounterTaBusy, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kTaPerfCounterShFifoBusy, SCE_RAZOR_GPU_BROADCAST_ALL);

	// CB
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kCbPerfCounterCcCacheHit, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kCbPerfCounterCcMcWriteRequest, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kCbPerfCounterCcMcWriteRequestsInFlight, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kCbPerfCounterCcMcReadRequest, SCE_RAZOR_GPU_BROADCAST_ALL);
#endif
	
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kCbPerfCounterDrawnBusy, SCE_RAZOR_GPU_BROADCAST_ALL);
	ret |= sceRazorGpuCreateStreamingCounter(streamingCounters++, sce::Gnm::kCbPerfCounterDrawnPixel, SCE_RAZOR_GPU_BROADCAST_ALL);

	Assert(ret == SCE_OK);

	params.numStreamingCounters = streamingCounters - params.streamingCounters;

	ret = sceRazorGpuThreadTraceInit(&params);

	return ret == SCE_OK;
}
#endif
