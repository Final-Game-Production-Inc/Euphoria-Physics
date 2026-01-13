//
// grcore/gfxcontext_durango.h
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_GFXCONTEXT_DURANGO_H
#define GRCORE_GFXCONTEXT_DURANGO_H

#if RSG_DURANGO

#include "allocscope.h"
#include "effect.h"
#include "gfxcontextcontrol.h"

#include "system/d3d11.h"


// Specify the amount of time waiting for the GPU before it is considerred a hang
#define GRCGFXCONTEXT_DURANGO_GPU_HANG_TIMEOUT_SEC      (10.f)


// Switches to enable more debugging code...

// Most basic error handling.  Generally should be left enable for all but
// shipping builds. This should be used for very lightweight error handling and
// debugging - anything that adds significant overhead should use
// GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING defined below, so it doesn't
// add overhead in configurations we don't want it (Profile builds, etc.)
#define GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING      (1 && (!__FINAL || !__NO_OUTPUT))

// Used for more heavyweight error handling and debugging beyond the basic
// stuff. Turned off anywhere the basic error handling is turned off, plus
// Profile builds.
#define GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING   (1 && GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING && !__PROFILE)

// Adds EVENT_WRITE_EOP PM4 commands to the GPU command buffer to aid tracking
// down GPU hangs.  Currently D3D (as of June 2014 XDK) doesn't expose the
// BOTTOM_OF_PIPE_TS event, only CACHE_FLUSH_AND_INV_TS_EVENT (via
// WriteValueBottomOfPipe), so there is a fair bit of overhead to enabling this.
//
// It sounds like BOTTOM_OF_PIPE_TS will be exposed via WriteTimestampToMemory
// in the July XDK (sadly won't be able to specify an immediate value, but the
// GPU timer will do ok for this).  That may allow this to be enabled in all
// non-profile/final builds.
//
#define GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS          (1 && __DEV)


namespace rage {

class grcCrossContextAlloc;
class grcGfxContext;
class grcGfxContextControl;

class grcGfxContextCommandList
{
private:

	friend grcGfxContext;

	ID3D11CommandList  *m_D3dCommandList;
	enum { MAX_SEGS = 128 };
	u32                 m_NumSegs;
	void               *m_Segs[MAX_SEGS];

public:

	void kick(bool isPartialSubmit=false);

private:

	void kick(ID3D11CommandList *d3dCommandList, bool isPartialSubmit);
};


// Depending on how you look at things, this class could be considerred to be a
// bit confussingly named.  grcGfxContext and ID3D11DeviceContextX together form
// the logical "graphics context".  In a text book object oriented design,
// grcGfxContext would wrap ID3D11DeviceContextX.  But back in the real world,
// that would just be adding pointless runtime overhead of an additional
// indirection for every D3D call, all just to conform to somewhat questionable
// OOD principles.
class grcGfxContext
{
private:

	friend grcContextAllocScope;
	friend grcCrossContextAlloc;
	friend grcGfxContextControl;

	static __THREAD grcGfxContext                  *sm_Current;

	grcContextAllocScope                           *m_AllocScope;
	grcContextAllocScope                            m_BaseAllocScope;
	void                                           *m_DataSegment;
	void                                           *m_CmdBufSegment;

#	if GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING
		atFixedBitSet<GRCCONTEXT_MAX_NUM_SEGMENTS,u64>  m_SegmentsNotYetReleased;
#	endif

#	if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
		u32                                             m_ContextSequenceNumber;
		u16                                             m_CheckpointSquenceNumber;
#	endif

	bool                                            m_Deferred;

	bool                                            m_ForceNewCmdBufSegment;
	bool                                            m_IsFinishCommandList;

	grcGfxContextControl                           *m_Control;
	grcGfxContextCommandList                       *m_CmdList;

public:

	static void init(u32 numSegs, u32 segSizeBytes);
	static void perThreadInit();

	static grcGfxContext *openContext(bool deferred, u32 sequenceNumber, grcGfxContextControl *control=NULL);
	static inline bool isPartialSubmitAllowed();
	static void kickContextIfPartialSubmitsAllowed();
	static void disablePartialSubmit(bool disable);
	static bool emergencyPartialSubmit();
	static grcGfxContextCommandList *closeContext();

	static inline grcGfxContext *current();

	static void endFrame();

	static u32 getTotalSegmentCount();

	// If segment throttling is being used, then higher level code should inform
	// grcGfxContext what the maximum value the the segment throttle will be
	// set to in grcGfxContextControl.  This value is used by some internal
	// heuristics.
	static void setMaxMaxPending(u32 maxMaxPending);

	inline bool isDeferred() const;

	static void lockAllocScope();
	static void unlockAllocScope();
	static bool isLockedAllocScope();
	static u32  getAllocScopeId();
	static bool isAllocScopeValid(u32 id);

	// Allocate memory for passing data to the GPU.  The allocation lifetime is
	// governed by the current allocation scope.  Accessing the memory after the
	// current allocation scope has been closed will result in undefined
	// rendering behaviour.
	//
	// Memory type will be WB/ONION.  No CPU side cache flushing is required.
	// The (very) rare cases where GPU side cache invalidations are required
	// will be handled automatically, higher level rendering code does not need
	// to worry about it.
	//
	void *allocateTemp(u32 sizeBytes, u32 alignBytes);
	template<class T> inline T *allocateTemp(u32 num=1);

	void safeKickPoint();

#	if !__FINAL || !__NO_OUTPUT
		static void reportGpuHang();
#	endif

#	if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
		void insertHangCheckpoint();
#	else
		inline void insertHangCheckpoint() {}
#	endif


private:

	grcGfxContext() {}
	inline void setDataSegment(void *seg);
	inline void setCmdBufSegment(void *seg);
	void createCommandList(void *seg);
	void addSegmentToCommandList(void *seg);
	void doPartialSubmit();
	void doPartialSubmitAllocSeg();
	void closeImmediateContext();
	void *untrackedAllocateTemp(u32 sizeBytes, u32 alignBytes);
	enum AllocSegmentAllowPartialSubmit { ALLOC_SEGMENT_DO_NOT_ALLOW_PARTIAL_SUBMIT, ALLOC_SEGMENT_ALLOW_PARTIAL_SUBMIT };
	void *allocSegment(AllocSegmentAllowPartialSubmit allowPartialSubmit);
	void releaseSegment(void *seg, unsigned releaseCount=1);
	void releaseSegment(unsigned segIdx, unsigned releaseCount=1);
	void *allocCmdBufFromNewSeg(size_t sizeBytes);

	static void *sysMemXMemAllocHook(size_t sizeBytes, u64 attributes);
	static void sysMemXMemFreeHook(void *address, u64 attributes);

	void crossContextFreeTemp(void *ptr, u32 sizeBytes);
	void crossContextIncSegmentRefCount(const void *ptr);
	void crossContextDecSegmentRefCount(const void *ptr);
};

/*static*/ inline bool grcGfxContext::isPartialSubmitAllowed() {
	return sm_Current->m_Control->isPartialSubmitAllowed();
}

/*static*/ inline grcGfxContext *grcGfxContext::current() {
	return sm_Current;
}

inline bool grcGfxContext::isDeferred() const {
	return m_Deferred;
}

/*static*/ inline void grcGfxContext::lockAllocScope() {
	grcGfxContext::sm_Current->m_AllocScope->lock();
}

/*static*/ inline void grcGfxContext::unlockAllocScope() {
	grcGfxContext::sm_Current->m_AllocScope->unlock();
}

/*static*/ inline bool grcGfxContext::isLockedAllocScope() {
	return grcGfxContext::sm_Current->m_AllocScope->isLocked();
}

/*static*/ inline u32 grcGfxContext::getAllocScopeId() {
	return grcGfxContext::sm_Current->m_AllocScope->id();
}

/*static*/ inline bool grcGfxContext::isAllocScopeValid(u32 id) {
	if (id < grcContextAllocScope::NUM_SPECIAL_IDS) {
		return id == grcContextAllocScope::ID_INFINITE;
	}
	const grcContextAllocScope *scope = grcGfxContext::sm_Current->m_AllocScope;
	do {
		if (scope->id() == id)
			return true;
		scope = scope->parent();
	} while (scope);
	return false;
}

template<class T>
inline T *grcGfxContext::allocateTemp(u32 num) {
	return (T*) allocateTemp(num*sizeof(T), __alignof(T));
}

// This is a bit sucky, but we need to ensure grcGfxContext::safeKickPoint is
// called regularly (see B*2147892).  As a nice side effect though, we also can
// auto add insertHangCheckpoint calls.
class grcGfxContextD3dWrapper : public ID3D11DeviceContextX {
private:

	inline void wrapperPost() {
		grcGfxContext *const ctx = grcGfxContext::current();
		ctx->safeKickPoint();
		ctx->insertHangCheckpoint();
	}

public:

#	define CREATE_D3D_WRAPPER_0(NAME)                                          \
		inline void NAME() {                                                   \
			ID3D11DeviceContextX::NAME();                                      \
			wrapperPost();                                                     \
		}
#	define CREATE_D3D_WRAPPER_1(NAME, T0)                                      \
		inline void NAME(T0 a0) {                                              \
			ID3D11DeviceContextX::NAME(a0);                                    \
			wrapperPost();                                                     \
		}
#	define CREATE_D3D_WRAPPER_2(NAME, T0, T1)                                  \
		inline void NAME(T0 a0, T1 a1) {                                       \
			ID3D11DeviceContextX::NAME(a0, a1);                                \
			wrapperPost();                                                     \
		}
#	define CREATE_D3D_WRAPPER_3(NAME, T0, T1, T2)                              \
		inline void NAME(T0 a0, T1 a1, T2 a2) {                                \
			ID3D11DeviceContextX::NAME(a0, a1, a2);                            \
			wrapperPost();                                                     \
		}
#	define CREATE_D3D_WRAPPER_4(NAME, T0, T1, T2, T3)                          \
		inline void NAME(T0 a0, T1 a1, T2 a2, T3 a3) {                         \
			ID3D11DeviceContextX::NAME(a0, a1, a2, a3);                        \
			wrapperPost();                                                     \
		}
#	define CREATE_D3D_WRAPPER_5(NAME, T0, T1, T2, T3, T4)                      \
		inline void NAME(T0 a0, T1 a1, T2 a2, T3 a3, T4 a4) {                  \
			ID3D11DeviceContextX::NAME(a0, a1, a2, a3, a4);                    \
			wrapperPost();                                                     \
		}
#	define CREATE_D3D_WRAPPER_6(NAME, T0, T1, T2, T3, T4, T5)                  \
		inline void NAME(T0 a0, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) {           \
			ID3D11DeviceContextX::NAME(a0, a1, a2, a3, a4, a5);                \
			wrapperPost();                                                     \
		}

	CREATE_D3D_WRAPPER_3(DrawIndexed,                           UINT, UINT, INT)
	CREATE_D3D_WRAPPER_2(Draw,                                  UINT, UINT)
	CREATE_D3D_WRAPPER_5(DrawIndexedInstanced,                  UINT, UINT, UINT, INT, UINT)
	CREATE_D3D_WRAPPER_4(DrawInstanced,                         UINT, UINT, UINT, UINT)
	CREATE_D3D_WRAPPER_0(DrawAuto)
	CREATE_D3D_WRAPPER_2(DrawIndexedInstancedIndirect,          ID3D11Buffer*, UINT)
	CREATE_D3D_WRAPPER_2(DrawInstancedIndirect,                 ID3D11Buffer*, UINT)
	CREATE_D3D_WRAPPER_3(Dispatch,                              UINT, UINT, UINT)
	CREATE_D3D_WRAPPER_2(DispatchIndirect,                      ID3D11Buffer*, UINT)
	CREATE_D3D_WRAPPER_5(MultiDrawIndexedInstancedIndirect,     UINT, ID3D11Buffer*, UINT, UINT, UINT)
	CREATE_D3D_WRAPPER_5(MultiDrawInstancedIndirect,            UINT, ID3D11Buffer*, UINT, UINT, UINT)
	CREATE_D3D_WRAPPER_6(MultiDrawIndexedInstancedIndirectAuto, ID3D11Buffer*, UINT, ID3D11Buffer*, UINT, UINT, UINT)
	CREATE_D3D_WRAPPER_6(MultiDrawInstancedIndirectAuto,        ID3D11Buffer*, UINT, ID3D11Buffer*, UINT, UINT, UINT)

#	undef CREATE_D3D_WRAPPER_0
#	undef CREATE_D3D_WRAPPER_1
#	undef CREATE_D3D_WRAPPER_2
#	undef CREATE_D3D_WRAPPER_3
#	undef CREATE_D3D_WRAPPER_4
#	undef CREATE_D3D_WRAPPER_5
#	undef CREATE_D3D_WRAPPER_6
};

} // namespace rage

#endif // RSG_DURANGO

#endif // GRCORE_GFXCONTEXT_DURANGO_H
