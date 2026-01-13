//
// grcore/gfxcontextcontrol.h
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_GFXCONTEXTCONTROL_H
#define GRCORE_GFXCONTEXTCONTROL_H

#define GRCGFXCONTEXT_CONTROL_SUPPORTED             (RSG_DURANGO || RSG_ORBIS)

#if GRCGFXCONTEXT_CONTROL_SUPPORTED
#	define GRCGFXCONTEXT_CONTROL_SUPPORTED_ONLY(...)    __VA_ARGS__
#else
#	define GRCGFXCONTEXT_CONTROL_SUPPORTED_ONLY(...)
#endif

#if GRCGFXCONTEXT_CONTROL_SUPPORTED

#include "allocscope.h"
#include "effect_mrt_config.h"
#include "math/amath.h"
#include "system/interlocked.h"
#include "system/ipc.h"

#ifdef _MSC_VER
	// Microsoft compiler warns about having a const and a const volatile copy
	// constructor.  But without these it fails to compile.
#	pragma warning(push)
#	pragma warning(disable:4521)    // warning C4521: 'rage::grcGfxContextControl::States' : multiple copy constructors specified
#endif

namespace rage
{

class grcGfxContext;

// Class for asynchronously controlling the behaviour of a graphics context.
// Used for managing rendering across multiple threads.
class grcGfxContextControl
{
private:

	friend grcGfxContext;

#	if !__FINAL
		// Performance testing switch.  Enabling this makes the code incorrect,
		// and could cause a deadlock (though it is very unlikely).  Useful to
		// toggle in a debugger to check if segment throttling is causing a
		// slowdown or not.
		static volatile bool DISABLE_SEGMENT_THROTTLING;
#	else
		enum { DISABLE_SEGMENT_THROTTLING = false };
#	endif

	enum { NUM_WORKER_THREADS = MULTIPLE_RENDER_THREADS ? MULTIPLE_RENDER_THREADS : 1 };

public:

	static void initClass();

	// Set the global free-for-all count.
	static void setGlobalFreeForAll(u32 numSegs);

	static u32 getGlobalFreeForAll();

	// Atomically add to the global free-for-all count.
	static void addGlobalFreeForAll(u32 numSegs);
	static u32 addGlobalFreeForAllRetNew(u32 numSegs);


	class States {
	private:

		friend grcGfxContext;
		friend grcGfxContextControl;

		enum {
			// Most significant bits store a the index of the subrender thread
			// that is stalled waiting for the max pending count to be increased
			// or for partial submit to be allowed.  Index is 1 based.  Value of
			// 0 means no thread stalled.  This value is writen by the subrender
			// thread.
			//
			// For ease of manually decoding these fields in a debugger,
			// hardcoding the number of bits.  A compile time assert then
			// verifies there is enough bits.
			//
			STALLED_THREAD_MASK             = 0xf0000000,

			// Flag for subrender thread to disable partial submits.  Overrides
			// ALLOW_PARTIAL_SUBMIT_MASK.  This is writen by the subrender
			// thread.
			DISABLE_PARTIAL_SUBMIT_MASK     = 0x08000000,

			// Flag to allow subrender thread to do partial submits.  This is
			// writen by the primary render thread.
			ALLOW_PARTIAL_SUBMIT_MASK       = 0x04000000,

			//RESERVED_BITS_MASK            = 0x03000000,

			// Number of allocated segments that exceeded the max pending value
			// (before it was updated), and therefore subtracted from
			// s_FreeForAll.
			FREE_FOR_ALL_MASK               = 0x00ff0000,

			// Current number of allocated segments.  Writen by the subrender
			// thread.
			CURR_PENDING_MASK               = 0x0000ff00,

			// Maximum number of segments that the subrender thread is allowed
			// to have pending.  This value is writen by both the primary render
			// thread, and the subrender thread.
			MAX_PENDING_MASK                = 0x000000ff,


			STALLED_THREAD_SHIFT            = CompileTimeCountTrailingZeroes<STALLED_THREAD_MASK>::value,
			DISABLE_PARTIAL_SUBMIT_SHIFT    = CompileTimeCountTrailingZeroes<DISABLE_PARTIAL_SUBMIT_MASK>::value,
			ALLOW_PARTIAL_SUBMIT_SHIFT      = CompileTimeCountTrailingZeroes<ALLOW_PARTIAL_SUBMIT_MASK>::value,
			FREE_FOR_ALL_SHIFT              = CompileTimeCountTrailingZeroes<FREE_FOR_ALL_MASK>::value,
			CURR_PENDING_SHIFT              = CompileTimeCountTrailingZeroes<CURR_PENDING_MASK>::value,
			MAX_PENDING_SHIFT               = CompileTimeCountTrailingZeroes<MAX_PENDING_MASK>::value,
		};

		CompileTimeAssert((1<<(32-STALLED_THREAD_SHIFT)) >= (NUM_WORKER_THREADS+1));
		CompileTimeAssert((FREE_FOR_ALL_MASK>>FREE_FOR_ALL_SHIFT) >= GRCCONTEXT_MAX_NUM_SEGMENTS-1);
		CompileTimeAssert((CURR_PENDING_MASK>>CURR_PENDING_SHIFT) >= GRCCONTEXT_MAX_NUM_SEGMENTS-1);
		CompileTimeAssert((MAX_PENDING_MASK >>MAX_PENDING_SHIFT ) >= GRCCONTEXT_MAX_NUM_SEGMENTS-1);

		u32 m_Val;

		/*explicit*/ States(u32 init);

	public:

		States();
		States(const States &rhs);
		States(const volatile States &rhs);
		States &operator=(const volatile States &rhs);

		bool operator==(States rhs) const;
		bool operator!=(States rhs) const;

		u32 getStalledThread()        const;
		u32 getDisablePartialSubmit() const;
		u32 getAllowPartialSubmit()   const;
		u32 getFreeForAll()           const;
		u32 getCurrPending()          const;
		u32 getMaxPending()           const;

		void setStalledThread(u32 x);
		void setDisablePartialSubmit(u32 x);
		void setAllowPartialSubmit(u32 x);
		void setFreeForAll(u32 x);
		void setCurrPending(u32 x);
		void setMaxPending(u32 x);
	};


	void init();
	grcGfxContextControl();

	// To read multiple values atomically, call readState() and save that to a
	// local variable, then access the different fields from there.
	States readStates() const;

	// Compare-And-Swap state value.
	States casStates(States cmp, States swap);

	// Save the context's sequence number.
	void setSequenceNumber(u32 sequenceNumber);

	// Get the context sequence number.
	u32 getSequenceNumber() const;

	// Get the current max pending value.
	u32 getMaxPending() const;

	// Sets the maximum number of many command buffer segments the context can
	// have allocated pending submission to the GPU.  With multi threaded
	// rendering, this is used balance segment allocation allocation so that the
	// context to be executed next by the GPU doesn't get starved by contexts
	// that will be executed later.
	//
	// By default contexts are not throttled.  Using throttling only makes sense
	// for multi threaded rendering.
	//
	void setInitialMaxPending(u32 numSegs);

	// Set the max pending limit to the largest possible value.
	void setInitialMaxMaxPending();

	// Get the current pending value.
	u32 getCurrPending() const;

	// Compare current pending to max pending.
	bool isAllocAllowed() const;

	// Increment the current pending value.  Should be called after each
	// successful segment allocation.
	void incPending();

	// Clear the current pending count.  Should be called after submitting all
	// pending segments to the GPU.
	void clearPending();

	// Partial submit enables a context to start submitting command buffer
	// segments to the GPU as they are created.  Enabling this allows for
	// arbitrarily large contexts as they can start recycling segments once the
	// GPU is finished with them.
	//
	// For single threaded rendering, this can always be set.  For multi
	// threaded rendering, the control thread should set this only for the next
	// context that is to be run by the GPU.
	//
	// init will disable partial submit.  But the default control structure
	// assigned to a context if none is specified, does allow partial submits.
	//
	void allowPartialSubmit();

	// Check if partial submits are allowed.
	bool isPartialSubmitAllowed() const;

	// Temporarily disable partial submits from the subrender thread.  Overrides
	// control threads setting to allow partial submit.  When used, subrender
	// thread must poll grcGfxContext::emergencyPartialSubmit.
	void subrenderDisablePartialSubmit(bool disablePartialSubmit);

	// Wait until isAllocAllowed() or isPartialSubmitAllowed() return true.
	void waitUntilAllocOrPartialSubmitAllowed();

	// Wake up the thread that is stalled on this control structure.
	void wakeStalledThread();

private:

	// Collection of values that need to be updated as an atomic unit.
	volatile States         m_Atomic;

	u32                     m_SequenceNumber;
	grcGfxContext *volatile m_ParentCtx;

	static sysIpcSema       s_StalledThreadSemaphores[NUM_WORKER_THREADS];
	static volatile u32     s_FreeForAll;


	bool reserveFreeForAll();
	void waitUntilAllocOrPartialSubmitAllowedStall();
};

/*static*/ inline void grcGfxContextControl::setGlobalFreeForAll(u32 numSegs) {
	s_FreeForAll = numSegs;
}

/*static*/ inline u32 grcGfxContextControl::getGlobalFreeForAll() {
	return s_FreeForAll;
}

/*static*/ inline void grcGfxContextControl::addGlobalFreeForAll(u32 numSegs) {
	sysInterlockedAdd(&s_FreeForAll, numSegs);
}

/*static*/ inline u32 grcGfxContextControl::addGlobalFreeForAllRetNew(u32 numSegs) {
	return sysInterlockedAdd(&s_FreeForAll, numSegs);
}

inline void grcGfxContextControl::init() {
	m_Atomic.m_Val   = 0;
	m_SequenceNumber = 0;
	m_ParentCtx      = NULL;
}

inline grcGfxContextControl::grcGfxContextControl() {
	init();
}

inline grcGfxContextControl::States::States()
	: m_Val(0) {
}

inline grcGfxContextControl::States::States(u32 init)
	: m_Val(init) {
}

inline grcGfxContextControl::States::States(const States &rhs)
	: m_Val(rhs.m_Val) {
}

inline grcGfxContextControl::States::States(const volatile States &rhs)
	: m_Val(rhs.m_Val) {
}

inline grcGfxContextControl::States &grcGfxContextControl::States::operator=(const volatile States &rhs) {
	m_Val = rhs.m_Val;
	return *this;
}

inline bool grcGfxContextControl::States::operator==(States rhs) const {
	return m_Val == rhs.m_Val;
}

inline bool grcGfxContextControl::States::operator!=(States rhs) const {
	return !operator==(rhs);
}

inline u32 grcGfxContextControl::States::getStalledThread() const {
	return (m_Val&STALLED_THREAD_MASK)>>STALLED_THREAD_SHIFT;
}

inline u32 grcGfxContextControl::States::getDisablePartialSubmit() const {
	return (m_Val&DISABLE_PARTIAL_SUBMIT_MASK)>>DISABLE_PARTIAL_SUBMIT_SHIFT;
}

inline u32 grcGfxContextControl::States::getAllowPartialSubmit() const {
	return (m_Val&ALLOW_PARTIAL_SUBMIT_MASK)>>ALLOW_PARTIAL_SUBMIT_SHIFT;
}

inline u32 grcGfxContextControl::States::getFreeForAll() const {
	return (m_Val&FREE_FOR_ALL_MASK)>>FREE_FOR_ALL_SHIFT;
}

inline u32 grcGfxContextControl::States::getCurrPending() const {
	return (m_Val&CURR_PENDING_MASK)>>CURR_PENDING_SHIFT;
}

inline u32 grcGfxContextControl::States::getMaxPending() const {
	return (m_Val&MAX_PENDING_MASK)>>MAX_PENDING_SHIFT;
}

inline void grcGfxContextControl::States::setStalledThread(u32 x) {
	m_Val = (m_Val&~STALLED_THREAD_MASK) | (x<<STALLED_THREAD_SHIFT);
}

inline void grcGfxContextControl::States::setDisablePartialSubmit(u32 x) {
	m_Val = (m_Val&~DISABLE_PARTIAL_SUBMIT_MASK) | (x<<DISABLE_PARTIAL_SUBMIT_SHIFT);
}

inline void grcGfxContextControl::States::setAllowPartialSubmit(u32 x) {
	m_Val = (m_Val&~ALLOW_PARTIAL_SUBMIT_MASK) | (x<<ALLOW_PARTIAL_SUBMIT_SHIFT);
}

inline void grcGfxContextControl::States::setFreeForAll(u32 x) {
	m_Val = (m_Val&~FREE_FOR_ALL_MASK) | (x<<FREE_FOR_ALL_SHIFT);
}

inline void grcGfxContextControl::States::setCurrPending(u32 x) {
	m_Val = (m_Val&~CURR_PENDING_MASK) | (x<<CURR_PENDING_SHIFT);
}

inline void grcGfxContextControl::States::setMaxPending(u32 x) {
	m_Val = (m_Val&~MAX_PENDING_MASK) | (x<<MAX_PENDING_SHIFT);
}

inline grcGfxContextControl::States grcGfxContextControl::readStates() const {
	return m_Atomic;
}

inline grcGfxContextControl::States grcGfxContextControl::casStates(States cmp, States swap) {
	States ret(sysInterlockedCompareExchange(&m_Atomic.m_Val, swap.m_Val, cmp.m_Val));
	return ret;
}

inline void grcGfxContextControl::setSequenceNumber(u32 sequenceNumber) {
	m_SequenceNumber = sequenceNumber;
}

inline u32 grcGfxContextControl::getSequenceNumber() const {
	return m_SequenceNumber;
}

inline u32 grcGfxContextControl::getMaxPending() const {
	States s = m_Atomic;
	return s.getMaxPending();
}

inline void grcGfxContextControl::setInitialMaxPending(u32 numSegs) {
	m_Atomic.m_Val = (m_Atomic.m_Val&~States::MAX_PENDING_MASK) | (numSegs<<States::MAX_PENDING_SHIFT);
}

inline void grcGfxContextControl::setInitialMaxMaxPending() {
	m_Atomic.m_Val |= States::MAX_PENDING_MASK;
}

inline u32 grcGfxContextControl::getCurrPending() const {
	States s = m_Atomic;
	return s.getCurrPending();
}

inline bool grcGfxContextControl::isAllocAllowed() const {
	const States s = m_Atomic;
	return s.getMaxPending()>s.getCurrPending() || DISABLE_SEGMENT_THROTTLING;
}

inline void grcGfxContextControl::incPending() {
	FatalAssert(isAllocAllowed());
	sysInterlockedAdd(&m_Atomic.m_Val, 1u<<States::CURR_PENDING_SHIFT);
}

inline void grcGfxContextControl::clearPending() {
	FatalAssert((m_Atomic.m_Val&States::STALLED_THREAD_MASK) == 0);
	sysInterlockedAnd(&m_Atomic.m_Val, ~(u32)(States::CURR_PENDING_MASK|States::FREE_FOR_ALL_MASK));
}

inline bool grcGfxContextControl::isPartialSubmitAllowed() const {
	return (m_Atomic.m_Val&(States::ALLOW_PARTIAL_SUBMIT_MASK|States::DISABLE_PARTIAL_SUBMIT_MASK))
		== ((1<<States::ALLOW_PARTIAL_SUBMIT_SHIFT)|(0<<States::DISABLE_PARTIAL_SUBMIT_SHIFT));
}

inline void grcGfxContextControl::subrenderDisablePartialSubmit(bool disablePartialSubmit) {
	if (disablePartialSubmit)
		sysInterlockedOr(&m_Atomic.m_Val, 1u<<States::DISABLE_PARTIAL_SUBMIT_SHIFT);
	else
		sysInterlockedAnd(&m_Atomic.m_Val, ~(1u<<States::DISABLE_PARTIAL_SUBMIT_SHIFT));
}

inline bool grcGfxContextControl::reserveFreeForAll() {
	u32 freeForAll = s_FreeForAll;
	for (;;) {
		if (!freeForAll)
			return false;

		// If we can successfully decrement s_FreeForAllAvailable, then an allocation is allowed.
		const u32 mem = sysInterlockedCompareExchange(&s_FreeForAll, freeForAll-1, freeForAll);
		if (freeForAll == mem) {
			sysInterlockedAdd(&m_Atomic.m_Val, 1u<<States::MAX_PENDING_SHIFT | 1u<<States::FREE_FOR_ALL_SHIFT);
			return true;
		}

		// CAS failed, loop and try again.
		freeForAll = mem;
	}
}

inline void grcGfxContextControl::waitUntilAllocOrPartialSubmitAllowed() {
	// Loop here, as if we do end up sleeping in
	// waitUntilAllocOrPartialSubmitAllowedStall(), then it can be woken up by
	// an increase in the global free for all count.  In that case, before
	// returning to the caller, we need to successfully reserve a free for all
	// slot.
	for (;;) {
		if (Likely(isAllocAllowed()))
			return;
		if (Likely(isPartialSubmitAllowed()))
			return;
		if (Likely(reserveFreeForAll()))
			return;
		waitUntilAllocOrPartialSubmitAllowedStall();
	}
}

}
// namespace rage

#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#endif // GRCGFXCONTEXT_CONTROL_SUPPORTED

#endif // GRCORE_GFXCONTEXTCONTROL_H
