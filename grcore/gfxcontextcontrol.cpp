//
// grcore/gfxcontextcontrol.cpp
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#include "gfxcontextcontrol.h"

#if GRCGFXCONTEXT_CONTROL_SUPPORTED

#include "gfxcontext.h"
#include "gfxcontextprofile.h"
#include "system/nelem.h"
#include "system/timer.h"


namespace rage
{

#if !__FINAL
	/*static*/ volatile bool grcGfxContextControl::DISABLE_SEGMENT_THROTTLING/*=false*/;
#endif

/*static*/ sysIpcSema grcGfxContextControl::s_StalledThreadSemaphores[NUM_WORKER_THREADS];
/*static*/ volatile u32 grcGfxContextControl::s_FreeForAll/*=0*/;

/*static*/ void grcGfxContextControl::initClass() {
	for (unsigned i=0; i<NELEM(s_StalledThreadSemaphores); ++i)
		s_StalledThreadSemaphores[i] = sysIpcCreateSema(0);
}

void grcGfxContextControl::allowPartialSubmit() {
#	if RSG_ORBIS
		m_ParentCtx = grcGfxContext::sm_Current;
		// Note that atomic operations act as memory barries, so m_ParentCtx is
		// garunteed to be updated before m_Atomic.
#	endif
	// Note that x86/x64 can implement sysInterlockedAdd a lot more efficiently
	// than sysInterlockedOr when the previous value in memory is required (lock
	// xadd, vs a lock cmpxchg loop).
	FatalAssert((m_Atomic.m_Val&States::ALLOW_PARTIAL_SUBMIT_MASK) == 0);
	const u32 prev = sysInterlockedAdd(&m_Atomic.m_Val, States::ALLOW_PARTIAL_SUBMIT_MASK);

	// If the subrender thread is already sleeping, wake it up.
	if ((prev&States::STALLED_THREAD_MASK) != 0)
		wakeStalledThread();
}

void grcGfxContextControl::waitUntilAllocOrPartialSubmitAllowedStall() {
	const u32 rti = g_RenderThreadIndex;
	FatalAssert(!MULTIPLE_RENDER_THREADS_ALLOW_DRAW_LISTS_ON_RENDER_THREAD || rti);
	const u32 stalledThreadIndex = rti-MULTIPLE_RENDER_THREADS_ALLOW_DRAW_LISTS_ON_RENDER_THREAD;
	const u32 stalledThreadValue = (stalledThreadIndex+1)<<States::STALLED_THREAD_SHIFT;
	const States states = m_Atomic;
	const u32 currPending = states.getCurrPending();
	grcGfxContextProfiler_StallAllocThrottle(currPending);
	u32 atomic = states.m_Val;
	for (;;) {
		// Check that the primary render thread hasn't increased max pending, or
		// enabled partial submits while this subrender thread has been spinning
		// here.  By ensuring that the allow partial submit flag is stored in a
		// more significant bit than the max pending count, this check can be
		// done with a single compare.
		//
		// Note that the early out here is not just an optimization.  It is
		// required for correctness, otherwise the subrender thread may wait on
		// the semaphore, but the primary render thread never signal it.
		//
		CompileTimeAssert(States::MAX_PENDING_MASK < States::ALLOW_PARTIAL_SUBMIT_MASK);
		const u32 pendingCheck = (atomic&(States::MAX_PENDING_MASK|States::ALLOW_PARTIAL_SUBMIT_MASK))>>States::MAX_PENDING_SHIFT;
		if (currPending < pendingCheck)
			break;

		// Attempt to atomically set the stalled thread value.
		const u32 stalled = atomic|stalledThreadValue;
		const u32 mem = sysInterlockedCompareExchange(&m_Atomic.m_Val, stalled, atomic);
		// Note that if this cas succeeded, then there must be no further
		// operations on m_Atomic from this thread until after waiting on the
		// semaphore.  See wakeStalledThread for why.
		if (atomic == mem) {
			// Now that the stalled thread index has been set, we can (and must)
			// safely wait on the semaphore.  The primary render thread is now
			// garunteed to not increment the max pending count without
			// signalling the semaphore.
			sysIpcWaitSema(s_StalledThreadSemaphores[stalledThreadIndex]);

			// Note that while tempting to assert isAllocAllowed() here, that is
			// not correct.  It is possible for the subrender thread to have
			// already allocated the max allowed, then be woken up by
			// allowPartialSubmit().  In that case, the worker thread needs to
			// do a submit, then it will be allowed to allocate again.
			//
			// It is also possible that the thread was woken up due to an
			// increase in the global free for all count.  That will also mean
			// isAllocAllowed() will currently return false, but the loop in
			// waitUntilAllocOrPartialSubmitAllowed() may then be able to
			// reserve a free for all slot.
			//

			break;
		}

		// Atomic cas failed, try again.
		atomic = mem;
	}
}

void grcGfxContextControl::wakeStalledThread() {
	const u32 stalledThreadIndexPlus1 = (m_Atomic.m_Val&States::STALLED_THREAD_MASK)>>States::STALLED_THREAD_SHIFT;
	FatalAssert(stalledThreadIndexPlus1);
	// No need to use an atomic operation here, since the subrender thread is
	// asleep.  Ok not garunteed to be asleep yet, but the subrender thread
	// garuntees not to mess with m_Atomic again after setting the stalled
	// thread index.
	m_Atomic.m_Val &= ~States::STALLED_THREAD_MASK;
	sysIpcSignalSema(s_StalledThreadSemaphores[stalledThreadIndexPlus1-1]);
}

}
// namespace rage

#endif // GRCGFXCONTEXT_CONTROL_SUPPORTED
