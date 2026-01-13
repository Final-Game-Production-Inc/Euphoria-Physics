//
// grcore/gfxcontextprofile.cpp
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#include "gfxcontextprofile.h"

#if GRCGFXCONTEXTPROFILE_ENABLE

#include "diag/channel.h"
#include "system/memops.h"
#include "system/nelem.h"

namespace rage {

grcGfxContextProfiler gGrcGfxContextProfiler;

static const char *defaultJobNameCallback(u8 jobId) {
	static char s_Buf[5];
	sprintf(s_Buf, "0x%02x", jobId);
	return s_Buf;
}

/*static*/ const char *(*grcGfxContextProfiler::jobNameCallback)(u8) = defaultJobNameCallback;

void grcGfxContextProfiler::init(void *buffer, size_t bufSizeBytes) {
	FatalAssert(buffer);
	sysMemSet(buffer, 0, bufSizeBytes);
	m_Buffer = (char*)(((uptr)buffer+15)&~15);
	const size_t padStart = m_Buffer - (char*)buffer;
	FatalAssert(bufSizeBytes >= padStart);
	m_MaxEvents = (bufSizeBytes-padStart)/grcGfxContextProfileEvent::MAX_SIZE;
	m_NextEvent = 0;
}

void grcGfxContextProfiler::dump() {
	const bool outputWasEnabled = diagChannel::GetOutput();
	diagChannel::SetOutput(true);

	const double microSecPerTick = sysTimer::GetTicksToMicroseconds();
	const u32 maxEvents = m_MaxEvents;
	const u32 oldest = m_NextEvent;
	const u64 t0 = ((grcGfxContextProfileEvent*)(m_Buffer+oldest*grcGfxContextProfileEvent::MAX_SIZE))->m_Timestamp;
	u64 prevTimestamp[MULTIPLE_RENDER_THREADS+1] = {0};
	for (unsigned i=0; i<NELEM(prevTimestamp); ++i)
		prevTimestamp[i] = t0;
	u32 idx = oldest;
	do {
		auto *const eventBase = (const grcGfxContextProfileEvent*)(m_Buffer+idx*grcGfxContextProfileEvent::MAX_SIZE);
		const u64 timestamp = eventBase->m_Timestamp;
		const u8  thread    = eventBase->m_Thread;
		const u64 absoluteMicrosec  = round((timestamp-t0)*microSecPerTick);
		const u64 diffMicrosec      = round((timestamp-prevTimestamp[thread])*microSecPerTick);
		prevTimestamp[thread] = timestamp;
		switch (eventBase->m_EventId) {
			case GRCGFXCONTEXT_PROFILE_EVENT_JOB_BEGIN: {
				auto *const event = static_cast<const grcGfxContextProfileEventJobBegin*>(eventBase);
				diagPrintf("%8" I64FMT "uus +%5" I64FMT "uus %u JOB_BEGIN            %s\n", absoluteMicrosec, diffMicrosec, event->m_Thread, jobNameCallback(event->m_JobId));
				break;
			}
			case GRCGFXCONTEXT_PROFILE_EVENT_JOB_END: {
				auto *const event = static_cast<const grcGfxContextProfileEventJobEnd*>(eventBase);
				diagPrintf("%8" I64FMT "uus +%5" I64FMT "uus %u JOB_END\n", absoluteMicrosec, diffMicrosec, event->m_Thread);
				break;
			}
			case GRCGFXCONTEXT_PROFILE_EVENT_ALLOC_SEGMENT: {
				auto *const event = static_cast<const grcGfxContextProfileEventAllocSegment*>(eventBase);
				diagPrintf("%8" I64FMT "uus +%5" I64FMT "uus %u ALLOC_SEGMENT\n", absoluteMicrosec, diffMicrosec, event->m_Thread);
				break;
			}
			case GRCGFXCONTEXT_PROFILE_EVENT_STALL_ALLOC_THROTTLE: {
				auto *const event = static_cast<const grcGfxContextProfileEventStallAllocThrottle*>(eventBase);
				diagPrintf("%8" I64FMT "uus +%5" I64FMT "uus %u STALL_ALLOC_THROTTLE %u\n", absoluteMicrosec, diffMicrosec, event->m_Thread, event->m_PendingSegs);
				break;
			}
			case GRCGFXCONTEXT_PROFILE_EVENT_STALL_ALLOC_WAIT_GPU: {
				auto *const event = static_cast<const grcGfxContextProfileEventStallAllocWaitGpu*>(eventBase);
				diagPrintf("%8" I64FMT "uus +%5" I64FMT "uus %u STALL_ALLOC_WAIT_GPU\n", absoluteMicrosec, diffMicrosec, event->m_Thread);
				break;
			}
			case GRCGFXCONTEXT_PROFILE_EVENT_KICK_BEGIN: {
				auto *const event = static_cast<const grcGfxContextProfileEventKickBegin*>(eventBase);
				diagPrintf("%8" I64FMT "uus +%5" I64FMT "uus %u KICK_BEGIN           %u\n", absoluteMicrosec, diffMicrosec, event->m_Thread, event->m_PendingSegs);
				break;
			}
			case GRCGFXCONTEXT_PROFILE_EVENT_KICK_END: {
				auto *const event = static_cast<const grcGfxContextProfileEventKickEnd*>(eventBase);
				diagPrintf("%8" I64FMT "uus +%5" I64FMT "uus %u KICK_END\n", absoluteMicrosec, diffMicrosec, event->m_Thread);
				break;
			}
		}
		idx = (idx+1) % maxEvents;
	} while (idx != oldest);

	diagChannel::SetOutput(outputWasEnabled);
}

} // namespace rage

#endif // GRCGFXCONTEXTPROFILE_ENABLE
