//
// grcore/gfxcontextprofile.h
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_GFXCONTEXTPROFILE_H
#define GRCORE_GFXCONTEXTPROFILE_H

#define GRCGFXCONTEXTPROFILE_ENABLE             (0)

#if GRCGFXCONTEXTPROFILE_ENABLE
#	define GRCGFXCONTEXTPROFILE_ENABLE_ONLY(...)    __VA_ARGS__
#else
#	define GRCGFXCONTEXTPROFILE_ENABLE_ONLY(...)
#endif

#if GRCGFXCONTEXTPROFILE_ENABLE
#	include "grcore/effect.h"
#	include "system/interlocked.h"
#	include "system/timer.h"
#endif

namespace rage {

#if GRCGFXCONTEXTPROFILE_ENABLE

	enum {
		GRCGFXCONTEXT_PROFILE_EVENT_NONE,
		GRCGFXCONTEXT_PROFILE_EVENT_JOB_BEGIN,
		GRCGFXCONTEXT_PROFILE_EVENT_JOB_END,
		GRCGFXCONTEXT_PROFILE_EVENT_ALLOC_SEGMENT,
		GRCGFXCONTEXT_PROFILE_EVENT_STALL_ALLOC_THROTTLE,
		GRCGFXCONTEXT_PROFILE_EVENT_STALL_ALLOC_WAIT_GPU,
		GRCGFXCONTEXT_PROFILE_EVENT_KICK_BEGIN,
		GRCGFXCONTEXT_PROFILE_EVENT_KICK_END,
	};

#	pragma pack(push)
#	pragma pack(1)
	struct grcGfxContextProfileEvent {
		enum { MAX_SIZE = 16 };
		u64     m_Timestamp;
		u8      m_EventId;
		u8      m_Thread;
	};
#	pragma pack(pop)

	struct grcGfxContextProfileEventJobBegin : public grcGfxContextProfileEvent {
		enum { EVENT_ID = GRCGFXCONTEXT_PROFILE_EVENT_JOB_BEGIN };
		u8      m_JobId;
	};

	struct grcGfxContextProfileEventJobEnd : public grcGfxContextProfileEvent {
		enum { EVENT_ID = GRCGFXCONTEXT_PROFILE_EVENT_JOB_END };
	};

	struct grcGfxContextProfileEventAllocSegment : public grcGfxContextProfileEvent {
		enum { EVENT_ID = GRCGFXCONTEXT_PROFILE_EVENT_ALLOC_SEGMENT };
	};

	struct grcGfxContextProfileEventStallAllocThrottle : public grcGfxContextProfileEvent {
		enum { EVENT_ID = GRCGFXCONTEXT_PROFILE_EVENT_STALL_ALLOC_THROTTLE };
		u16     m_PendingSegs;
	};

	struct grcGfxContextProfileEventStallAllocWaitGpu : public grcGfxContextProfileEvent {
		enum { EVENT_ID = GRCGFXCONTEXT_PROFILE_EVENT_STALL_ALLOC_WAIT_GPU };
	};

	struct grcGfxContextProfileEventKickBegin : public grcGfxContextProfileEvent {
		enum { EVENT_ID = GRCGFXCONTEXT_PROFILE_EVENT_KICK_BEGIN };
		u16     m_PendingSegs;
	};

	struct grcGfxContextProfileEventKickEnd : public grcGfxContextProfileEvent {
		enum { EVENT_ID = GRCGFXCONTEXT_PROFILE_EVENT_KICK_END };
	};

	class grcGfxContextProfiler {
	public:

		void init(void *buffer, size_t bufSizeBytes);
		static inline u8 getThread();
		inline grcGfxContextProfileEvent *allocEventBase(u8 eventId);
		template<class T> inline T *allocEvent();
		void dump();

		static const char *(*jobNameCallback)(u8 jobId);

	private:

		char           *m_Buffer;
		u32             m_MaxEvents;
		volatile u32    m_NextEvent;
	};

	/*static*/ inline u8 grcGfxContextProfiler::getThread() {
		return MULTIPLE_RENDER_THREADS_ALLOW_DRAW_LISTS_ON_RENDER_THREAD ? g_RenderThreadIndex : (g_RenderThreadIndex+g_IsSubRenderThread);
	}

	inline grcGfxContextProfileEvent *grcGfxContextProfiler::allocEventBase(u8 eventId) {
		const u32 maxEvents = m_MaxEvents;
		u32 event = m_NextEvent;
		for (;;) {
			const u32 nextEvent = (event+1) % maxEvents;
			const u32 mem = sysInterlockedCompareExchange(&m_NextEvent, nextEvent, event);
			if (Likely(event == mem))
				break;
			event = mem;
		}
		auto *const e = (grcGfxContextProfileEvent*)(m_Buffer+event*grcGfxContextProfileEvent::MAX_SIZE);
		e->m_EventId    = eventId;
		e->m_Thread     = getThread();
		e->m_Timestamp  = sysTimer::GetTicks();
		return e;
	}

	template<class T> inline T *grcGfxContextProfiler::allocEvent() {
		CompileTimeAssert(sizeof(T) <= grcGfxContextProfileEvent::MAX_SIZE);
		return static_cast<T*>(allocEventBase((u8)T::EVENT_ID));
	}

	extern grcGfxContextProfiler gGrcGfxContextProfiler;

#endif // GRCGFXCONTEXTPROFILE_ENABLE

static inline void grcGfxContextProfiler_JobBegin(u8 GRCGFXCONTEXTPROFILE_ENABLE_ONLY(jobId)) {
#	if GRCGFXCONTEXTPROFILE_ENABLE
		auto *const e = gGrcGfxContextProfiler.allocEvent<grcGfxContextProfileEventJobBegin>();
		e->m_JobId = jobId;
#	endif
}

static inline void grcGfxContextProfiler_JobEnd() {
#	if GRCGFXCONTEXTPROFILE_ENABLE
		/*auto *const e =*/ gGrcGfxContextProfiler.allocEvent<grcGfxContextProfileEventJobEnd>();
#	endif
}

static inline void grcGfxContextProfiler_AllocSegment() {
#	if GRCGFXCONTEXTPROFILE_ENABLE
		/*auto *const e =*/ gGrcGfxContextProfiler.allocEvent<grcGfxContextProfileEventAllocSegment>();
#	endif
}

static inline void grcGfxContextProfiler_StallAllocThrottle(u16 GRCGFXCONTEXTPROFILE_ENABLE_ONLY(pendingSegs)) {
#	if GRCGFXCONTEXTPROFILE_ENABLE
		auto *const e = gGrcGfxContextProfiler.allocEvent<grcGfxContextProfileEventStallAllocThrottle>();
		e->m_PendingSegs = pendingSegs;
#	endif
}

static inline void grcGfxContextProfiler_StallAllocWaitGpu() {
#	if GRCGFXCONTEXTPROFILE_ENABLE
		/*auto *const e =*/ gGrcGfxContextProfiler.allocEvent<grcGfxContextProfileEventStallAllocWaitGpu>();
#	endif
}

static inline void grcGfxContextProfiler_KickBegin(u16 GRCGFXCONTEXTPROFILE_ENABLE_ONLY(pendingSegs)) {
#	if GRCGFXCONTEXTPROFILE_ENABLE
		auto *const e = gGrcGfxContextProfiler.allocEvent<grcGfxContextProfileEventKickBegin>();
		e->m_PendingSegs = pendingSegs;
#	endif
}

static inline void grcGfxContextProfiler_KickEnd() {
#	if GRCGFXCONTEXTPROFILE_ENABLE
		/*auto *const e =*/ gGrcGfxContextProfiler.allocEvent<grcGfxContextProfileEventKickEnd>();
#	endif
}

} // namespace rage

#endif // GRCORE_GFXCONTEXTPROFILE_H
