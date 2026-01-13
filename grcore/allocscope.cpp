//
// grcore/allocscope.cpp
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#include "allocscope.h"

#if GRCCONTEXT_ALLOC_SCOPES_SUPPORTED

#include "gfxcontext.h"
#include "system/interlocked.h"

namespace rage
{

volatile u32 grcContextAllocScope::sm_AllocScopeId = grcContextAllocScope::NUM_SPECIAL_IDS - 1;

void grcContextAllocScope::generateId() {
	FatalAssert(m_Id == ID_INVALID);
	do {
		m_Id = sysInterlockedIncrement(&sm_AllocScopeId);
	} while (Unlikely(m_Id < NUM_SPECIAL_IDS));
}

#if __ASSERT
	bool grcContextAllocScope::isDefaultState() const {
		return !m_ParentScope && m_Id==ID_INVALID && !m_Locked && m_AllocedSegs.GetFirstBitIndex()==-1;
	}
#endif

#if GRCCONTEXT_ALLOC_SCOPE_LOCKS
	void grcContextAllocScope::assertNotLocked() const {
		// If this assert fires, then chances are you should push an allocation
		// scope around the higher level rendering code.  Each context always
		// has an allocation scope in the stack, but that is generally locked
		// since using that would unnecissarily increase allocation lifetime.
		// For cases where the allocation is meant to have a lifetime of the
		// entire context, then temporarily unlocking and then relocking the
		// scope may be the best solution.
		//
		// While this assert can often be ignored, and the code will "basically"
		// execute correctly, doing so could lead to running out of command
		// buffer space in rare non-deterministic circumstances, since the
		// memory cannot be freed.
		//
		// When adding a new allocation scope, it is a good idea to test locally
		// with ENABLE_TRASH_DATA_AFTER_RELEASE enabled (in gfxcontext_*.cpp),
		// to make sure that the scope isn't too small.
		//
		// See "grcore/allocscope.h" for macros to simplify using allocation
		// scopes.
		//
		// It is often sufficient to add a GRC_ALLOC_SCOPE_AUTO_PUSH_POP to the
		// high level draw list command.  But, be careful of allocations that
		// have a lifetime longer than a single drawlist command.  Freeing up an
		// allocation then referencing it later leads to undefined behavior.
		//
		// Also be careful of scopes that are much larger than necissary.  If
		// there is a gigantic amount of rendering done within a single scope,
		// then it is possible to run out of memory.
		//
		// Alloc scopes have fairly little CPU overhead.  So the basic rule of
		// thumb is make the scopes as tight as possible, but not any tighter.
		//
		Assertf(!isLocked(),
			"Higher level rendering code is not correctly scoping GPU allocations.  See comments in code above this assert on how to fix this.");

		// To help track down alloc scope lock errors,
		// GRCCONTEXT_ALLOC_SCOPE_LOCKS can be (locally) enabled in
		// non-assert builds.
#		if GRCCONTEXT_ALLOC_SCOPE_LOCKS && !__ASSERT
			if (isLocked())
				__debugbreak();
#		endif
	}
#endif

void grcContextAllocScope::releaseAll() {
	grcGfxContext *const ctx = grcGfxContext::sm_Current;

#	if __ASSERT
		// In assert enabled builds, take a copy of the bit mask and then clear
		// it before calling releaseSegment.  This allows releaseSegment to
		// assert that there are no alloc scopes still referencing the segment.
		auto allocedSegs = m_AllocedSegs;
		m_AllocedSegs.Reset();
#	else
		auto &allocedSegs = m_AllocedSegs;
#	endif

	// Decrement the allocation counters for the segments at eop.
	for (auto itor=allocedSegs.Begin(); itor!=allocedSegs.End(); ++itor) {
		ctx->releaseSegment(itor);
	}

#	if !__ASSERT
		m_AllocedSegs.Reset();
#	endif

	m_Id = ID_INVALID;
}

void grcContextAllocScope::pushScope() {
	FatalAssert(!m_ParentScope);
	FatalAssert(!m_SizeAlloced);
	grcContextAllocScope *const parent = grcGfxContext::sm_Current->m_AllocScope;
	FatalAssert(parent != this);
	generateId();
	m_ParentScope = parent;
	grcGfxContext::sm_Current->m_AllocScope = this;
}

void grcContextAllocScope::popScope() {
	FatalAssert(grcGfxContext::sm_Current->m_AllocScope == this);
	FatalAssert(!m_Locked);
	FatalAssert(m_Id >= NUM_SPECIAL_IDS);
	releaseAll();
	grcContextAllocScope *const parent = m_ParentScope;
	FatalAssert(parent);
	grcGfxContext::sm_Current->m_AllocScope = parent;
	ASSERT_ONLY(m_ParentScope = NULL;)
	ASSERT_ONLY(m_SizeAlloced = 0;)
	m_Id = ID_INVALID;
}

void *grcCrossContextAlloc::getPtr() {
	void *ptr = m_Ptr;
	if (!ptr) {
		// The !ptr check above is really just an optimization.  It is
		// definitely possible for multiple threads to get in here at the same
		// time.  When that happens, both threads will allocate from their own
		// context, then whoever is first to do the CAS "wins".  The other
		// thread then frees up the allocation they just performed and use the
		// allocation from the winning thread.
		const u32 size = m_Size;
		grcGfxContext *const ctx = grcGfxContext::sm_Current;
		ptr = ctx->untrackedAllocateTemp(size, m_AlignBytes);
		void *const prevPtr = sysInterlockedCompareExchange(&m_Ptr, ptr, (void*)NULL);
		if (!prevPtr)
			ctx->crossContextIncSegmentRefCount(ptr);
		else {
			ctx->crossContextFreeTemp(ptr, size);
			ptr = prevPtr;
		}
	}
	return ptr;
}

void *grcCrossContextAlloc::getPtrSingleThreaded() {
	void *ptr = m_Ptr;
	if (!ptr) {
		grcGfxContext *const ctx = grcGfxContext::sm_Current;
		ptr = ctx->untrackedAllocateTemp(m_Size, m_AlignBytes);
		m_Ptr = ptr;
		ctx->crossContextIncSegmentRefCount(ptr);
	}
	return ptr;
}

void grcCrossContextAlloc::free() {
	void *const ptr = m_Ptr;
	FatalAssert(ptr);
	grcGfxContext::sm_Current->crossContextDecSegmentRefCount(ptr);
	ASSERT_ONLY(m_Ptr = NULL;)
}

}
// namespace rage

#endif // GRCCONTEXT_ALLOC_SCOPES_SUPPORTED
