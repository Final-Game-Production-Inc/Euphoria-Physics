//
// grcore/gfxcontext_gnm.h
//
// Copyright (C) 2013-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_GFXCONTEXT_GNM_H
#define GRCORE_GFXCONTEXT_GNM_H

#if RSG_ORBIS

#include "allocscope.h"
#include "config.h"
#include "device.h"
#include "effect_mrt_config.h"
#include "gfxcontextcontrol.h"
#include "gnmx.h"
#include "wrapper_gnm.h"

#include "math/amath.h"


// Specify the amount of time waiting for the GPU before it is considerred a hang.
#define GRCGFXCONTEXT_GNM_GPU_HANG_TIMEOUT_SEC      (10.f)

// Specifies whether or not a CCB is used.  Currently this supports either CUE
// (uses CCB) or LCUE (does not use CCB).
#define GRCGFXCONTEXT_GNM_USES_CCB                  (!ENABLE_LCUE)
#define GRCGFXCONTEXT_GNM_USES_ACB                  (!ENABLE_LCUE && SCE_ORBIS_SDK_VERSION >= (0x01700000u))


// Switches to enable more debugging code...

// Most basic error handling.  Generally should be left enable for all but
// shipping builds. This should be used for very lightweight error handling and
// debugging - anything that adds significant overhead should use
// GRCGFXCONTEXT_GNM_STANDARD_ERROR_HANDLING defined below, so it doesn't add
// overhead in configurations we don't want it (Profile builds, etc.)
#define GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING      (1 && (!__FINAL || !__NO_OUTPUT) && !__MASTER)

// Used for more heavyweight error handling and debugging beyond the basic
// stuff. Turned off anywhere the basic error handling is turned off, plus
// Profile builds.
#define GRCGFXCONTEXT_GNM_STANDARD_ERROR_HANDLING      (1 && GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING && !__PROFILE)

// Currently this does not help to track down the most common GPU errors, page
// faults.  The problem is that the shader cores do not stop after a page fault.
// Working with Sony to try to figure out a way to get the GPU to stop
// (https://ps4.scedev.net/support/issue/16432).  The code is still useful for
// some other types of GPU hangs such as a bad SET_PREDICATION.
#define GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS          (1 && (GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING && !__PROFILE))

// This enables the stack of GRCDBG_PUSH/GRCDBG_POP markers.  Requires
// GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS to be of any use.
#define GRCGFXCONTEXT_GNM_CONTEXT_STACK             (1 && (GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS && GRCDBG_IMPLEMENTED))

// Enables the recording of GPU idle time caused by stalling waiting for command
// buffers to be submitted by the CPU.  This can then be subtracted from the
// time between two timestamps captured during profiling.
#define GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES          (1 && !__FINAL)


// Helper macros for the debug switches above...

#if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
#	define GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING_ONLY(...)     __VA_ARGS__
#else
#	define GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING_ONLY(...)
#endif

#if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
#	define GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS_ONLY(...)         __VA_ARGS__
#else
#	define GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS_ONLY(...)
#endif

#if GRCGFXCONTEXT_GNM_CONTEXT_STACK
#	define GRCGFXCONTEXT_GNM_CONTEXT_STACK_ONLY(...)            __VA_ARGS__
#else
#	define GRCGFXCONTEXT_GNM_CONTEXT_STACK_ONLY(...)
#endif

#if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
#	define GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES_ONLY(...)         __VA_ARGS__
#else
#	define GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES_ONLY(...)
#endif




namespace rage {

class grcContextAllocScope;
class grcCrossContextAlloc;
class grcGfxContext;

#if GRCGFXCONTEXT_GNM_CONTEXT_STACK
	struct grcGfxContextDebugState;
#endif


#if ENABLE_LCUE
	typedef sce::LCUE::GraphicsContext  grcGfxContextBase;
#else
	typedef sce::Gnmx::GfxContext       grcGfxContextBase;
#endif

class grcGfxContext : public grcGfxContextBase {
public:
	static const u32 MaxPending = 128;

private:

	friend grcContextAllocScope;
	friend grcCrossContextAlloc;
	friend grcGfxContextControl;

	static u32 sm_SegmentCount;
	static __THREAD grcGfxContext *sm_Current;

#	if GRCGFXCONTEXT_GNM_USES_CCB
		static u32 sm_MaxMaxPending;
#	endif



	bool  m_Deferred;
	u32   m_PendingCount;
	void *m_DcbStarts      [MaxPending];
	u32   m_DcbSizesInBytes[MaxPending];
	void *m_CcbStarts      [MaxPending];
	u32   m_CcbSizesInBytes[MaxPending];

	grcGfxContextControl *m_Control;

	grcContextAllocScope *m_AllocScope;
	grcContextAllocScope  m_BaseAllocScope;

	enum Status {
		STATUS_OPEN,
		STATUS_CLOSED,
		STATUS_CLOSED_AND_KICKED,
	};
	Status m_Status;

	u32 m_VsWaveLimitCurrent;
	u32 m_VsWaveLimitLast;

#	if GRCGFXCONTEXT_GNM_USES_CCB
		// When using CUE, keep track of when we are currently in the middle of
		// a draw or dispatch.  If the current command buffer segments become
		// full at this time, it is not safe to do a
		// sce::Gnm::submitCommandBuffers call, unless we manually fixup the
		// CE/DE counters.
		//
		// Since we are hopefully dropping CUE support, just using a simple flag
		// here.  If we end up wanting to keep CUE, then we'll probably want to
		// improve this by storing the actual DCB pointer, then scanning the
		// last little bit of the command buffer looking for a
		// WAIT_ON_CE_COUNTER.  If it is found, but no draw/dispatch, then the
		// WAIT_ON_CE_COUNTER should be nop'ed out, and then added to the next
		// DCB.  That would then allow a submit.
		//
		bool m_InDrawOrDispatch;
#	endif

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		// Synchronization so that we can track down which draw call in a DCB
		// failed.  SyncMarkerDrawEngineDone is written by the draw engine,
		// SyncMarkerShaderDone is written after an end of pipe event.  By
		// having two values it makes it a little easier to determine which part
		// of the pipeline stalled.
		static const u64 SyncMarkerStateMask      = 0xf000000000000000uLL;
		static const u64 SyncMarkerDrawEngineDone = 0x1000000000000000uLL;
		static const u64 SyncMarkerShaderDone     = 0x2000000000000000uLL;
		static const u32 MaxSyncMarkersPerBatch = 32; // (counting the prev and next pointers)
		static const u32 SyncMarkerBatchPrev = 0;
		static const u32 SyncMarkerBatchNext = 1;
		u32  m_SyncMarkerIdxNextWrite;
		u32  m_SyncMarkerIdxNextWait;
		u64 *m_SyncMarkerBatchNextWrite;
		u64 *m_SyncMarkerBatchNextWait;
		u32 *m_DrawStart;
		u32  m_NumSegmentsSinceSyncMarkerWaitAll;
		bool m_inGpuWaitAllSyncMarkers;
#	endif

#	if GRCGFXCONTEXT_GNM_CONTEXT_STACK
		grcGfxContextDebugState *m_DbgState;
		u32 m_FaultStackSP;
#	endif

public:
	static void initRing(u32 contextCount, u32 cueRingCount, u32 cbRingCount, u32 dcbSizeBytes, u32 ccbSizeBytes);

	static grcGfxContext *openContext(bool deferred, u16 sequenceNumber, grcGfxContextControl *control=NULL);
	static void kickContext();
	static inline bool isPartialSubmitAllowed();
	static void kickContextIfPartialSubmitsAllowed();
	static void disablePartialSubmit(bool disable);
	static bool emergencyPartialSubmit();
	static void closeContext(bool submit);
	static void submitAndFlip(s32 videoOutHandle, u32 rtIndex, u32 flipMode, s64 flipArg);
#if __DEV
	static void validateAndSubmitCurrent();
#endif
	enum SubmitPendingSubmitTypeArg {SUBMIT_PARTIAL, SUBMIT_FULL, SUBMIT_FLIP};
	void submitPending(SubmitPendingSubmitTypeArg submitType);

	static inline grcGfxContext *current();

	static u32 getTotalSegmentCount() { return sm_SegmentCount; }

	// This will become unnecissary once B*1791280 is done.  But for now we have
	// a fixed number of contexts.  The number of contexts available must be
	// greater than the max number ever used within a single frame, otherwise
	// there is a race condition where thread timing can cause a deadlock (eg,
	// B*2031872).
	static u32 getContextPoolSize();

	// If segment throttling is being used, then higher level code should inform
	// grcGfxContext what the maximum value the max segments pending will be set
	// to in grcGfxContextControl.  This value is used by some internal
	// heuristics.
	static void setMaxMaxPending(u32 maxMaxPending);

	static bool isDeferred();

	static void lockAllocScope();
	static void unlockAllocScope();
	static bool isLockedAllocScope();
	static u32  getAllocScopeId();
	static bool isAllocScopeValid(u32 id);

	// Allocate data that will have the lifetime of the current
	// grcContextAllocScope.
	//
	// Generally this allocation will be used for passing data to the GPU via a
	// resource descriptor.  To handle buffer wrapping, we invalidate L1, K$ and
	// volatile lines in the L2.  We do not invalidate the entire GPU L2.
	// Because of this, resource descriptors must be configured to mark L2 lines
	// as volatile.  To do this, call setBufferCachePolicy, after the
	// appropriate sce::Gnm::Buffer::initAs* call.
	//
	void *allocateFromCommandBufferAlignBytes(u32 sizeBytes, u32 alignBytes);
	inline void *allocateFromCommandBuffer(u32 sizeBytes, sce::Gnm::EmbeddedDataAlignment log2Align);
	inline void *allocateFromCommandBuffer(sce::Gnm::SizeAlign sizeAlign);
	template<class T> inline T *allocateFromCommandBuffer(u32 num=1);

	// Platform independent functions.
	inline void *allocateTemp(u32 sizeBytes, u32 alignBytes);
	template<class T> inline T *allocateTemp(u32 num=1);

	// Set the appropriate cache policy for buffers referencing memory allocated
	// with allocateFromCommandBuffer.
	static const sce::Gnm::ResourceMemoryType   BUFFER_MEM_TYPE         = sce::Gnm::ResourceMemoryType::kResourceMemoryTypeSC;
	static const sce::Gnm::L1CachePolicy        BUFFER_L1_CACHE_POLICY  = sce::Gnm::L1CachePolicy::kL1CachePolicyLru;
	static const sce::Gnm::KCachePolicy         BUFFER_K_CACHE_POLICY   = sce::Gnm::KCachePolicy::kKCachePolicyLru;
	static inline void setBufferCachePolicy(sce::Gnm::Buffer &buf);

	// Wrapper around rage::initAsConstantBuffer, that also calls
	// setBufferCachePolicy.  This is only inteded to be used when memory
	// pointed to by baseAddr was allocated by a call to
	// allocateFromCommandBuffer.  Otherwise just use
	// rage::initAsConstantBuffer.
	static inline void initAsConstantBuffer(sce::Gnm::Buffer &buf, void *baseAddr, u32 sizeBytes);

	// Wrapper around rage::initAsVertexBuffer.  Same comments apply as per
	// initAsConstantBuffer.
	static inline void initAsVertexBuffer(sce::Gnm::Buffer &buf, void *baseAddr, unsigned stride, unsigned numVertices, u32 dword3
		WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(, sce::Gnm::DataFormat fmt));

	// Data embedded in the command buffer is really only for debugging
	// purposes.  It should not be used to store things for access via the GPU.
	// Use allocateFromCommandBuffer for data that will be read by a shader.
	// The difference is that allocateFromCommandBuffer records the allocation
	// in the current grcContextAllocScope, and will ensure the command
	// buffer segment is not recycled while the data may still be required.
	inline u32 *embedDataDcb(u32 numDwords, sce::Gnm::EmbeddedDataAlignment log2Align=sce::Gnm::kEmbeddedDataAlignment4);
#	if GRCGFXCONTEXT_GNM_USES_CCB
		inline u32 *embedDataCcb(u32 numDwords, sce::Gnm::EmbeddedDataAlignment log2Align=sce::Gnm::kEmbeddedDataAlignment4);
#	endif

#	if !__FINAL || !__NO_OUTPUT
		__attribute__((__noinline__)) static void reportGpuHang(); // no inline so that this shows up in callstacks
		static void dumpCrashInfo();
#	endif

#	if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
		static s64 getRecordedGpuIdleBetweenTimestamps(s64 begin, s64 end);
#	endif

	enum {
		// Note that SDK1.73 sce::Gnm::DrawCommandBuffer::setGraphicsShaderControl
		// is buggy and always enables CU[0..2].
		PS_CU_MASK_DEFAULT = 0x1ff,
		VS_CU_MASK_DEFAULT = 0x1ff,
		GS_CU_MASK_DEFAULT = 0x1ff,
		ES_CU_MASK_DEFAULT = 0x0ff,
		HS_CU_MASK_DEFAULT = 0x000,
		LS_CU_MASK_DEFAULT = 0x0ff,
	};

	inline void setVsWaveLimit(u32 vsWaveLimit);

	inline void dispatch(u32 threadGroupX, u32 threadGroupY, u32 threadGroupZ);
	inline void dispatchDraw(sce::Gnm::Buffer bufferInputData, u32 numBlocksTotal, u32 numPrimsPerVgt);
	inline void dispatchIndirect(u32 dataOffsetInBytes);
	inline void drawIndex(u32 indexCount, const void *indexAddr);
	inline void drawIndexAuto(u32 indexCount);
	inline void drawIndexIndirect(u32 dataOffsetInBytes);
	inline void drawIndexInline(u32 indexCount, const void *indices, u32 indicesSizeInBytes);
	inline void drawIndexOffset(u32 indexOffset, u32 indexCount);
	inline void drawIndirect(u32 dataOffsetInBytes);

#	if GRCGFXCONTEXT_GNM_CONTEXT_STACK
		static void PushFaultContext(const char *label);
		static void PopFaultContext();
		static u32  HackGetFaultContextSP();
#	else
		static inline void PushFaultContext(const char *UNUSED_PARAM(label)) {}
		static inline void PopFaultContext() {}
#	endif

private:

	static void gpuSubmitThread(void*);
	u32 getDcbSizeBytes() const;
	u32 getCcbSizeBytes() const;
	char *allocSegment();
	void validatePending();
	void addPending();
	void assignSegmentToContext();
	void releaseSegment(void *seg, unsigned releaseCount=1);
	void releaseSegment(unsigned segIdx, unsigned releaseCount=1);
	void releaseCurrentSegment(unsigned releaseCount=1);
	void doKickContext();
	bool handleBufferFull(sce::Gnm::CommandBuffer *cb, u32 sizeInDwords);
	static bool handleBufferFullStatic(sce::Gnm::CommandBuffer *cb, u32 sizeInDwords, void *userData);
	void initializePerContextDefaultHardwareState();

	static void *tryUntrackedAllocateFromCommandBufferAlignBytes(sce::Gnm::CommandBuffer *cmdBuf, u32 sizeBytes, u32 alignBytes, u32 reservedBytes);
	void *tryUntrackedAllocateFromCommandBufferAlignBytes(u32 sizeBytes, u32 alignBytes);
	void *untrackedAllocateFromCommandBufferAlignBytes(u32 sizeBytes, u32 alignBytes);
	template<class T> inline T *untrackedAllocateFromCommandBuffer(u32 num=1);

	// Platform independent functions.
	inline void *untrackedAllocateTemp(u32 sizeBytes, u32 alignBytes);

	u32 *embedData(sce::Gnm::CommandBuffer *cb, u32 numDwords, sce::Gnm::EmbeddedDataAlignment log2Align);

	void crossContextFreeTemp(void *ptr, u32 sizeBytes);
	void crossContextIncSegmentRefCount(const void *ptr);
	void crossContextDecSegmentRefCount(const void *ptr);

	void flushVsWaveLimit();

	inline void prePreDraw();
	inline void prePreDispatch();
	inline void postPostDraw();
	inline void postPostDispatch();

#	if GRCGFXCONTEXT_GNM_USES_CCB
		void safeKickPoint();
#	endif

#	if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
		void recordGpuIdlePrologue();
		void recordGpuIdleEpilogue();
#	endif

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		void allocSyncMarkerBatch(bool allocCouldFail);
		void gpuWaitAllSyncMarkers();
		void markEnd();
#	endif
};


#if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
	// No inline because R*TM also needs access to this function
	__attribute__((__noinline__)) void OrbisGpuCrashCallback(u64 arg0, u64 arg1, u64 arg2);

	// No argument version is only used locally though
	static __attribute__((__always_inline__, __unused__)) void OrbisGpuCrashCallback() {
		OrbisGpuCrashCallback(0,0,0);
		__debugbreak();
	}
#else
	static __attribute__((__always_inline__, __unused__)) void OrbisGpuCrashCallback() {}
#endif


/*static*/ inline bool grcGfxContext::isPartialSubmitAllowed() {
	return sm_Current->m_Control->isPartialSubmitAllowed();
}

/*static*/ inline grcGfxContext *grcGfxContext::current() {
	return sm_Current;
}

/*static*/ inline void grcGfxContext::setMaxMaxPending(u32 maxMaxPending) {
	FatalAssert(maxMaxPending <= MaxPending);
#	if GRCGFXCONTEXT_GNM_USES_CCB
		sm_MaxMaxPending = maxMaxPending;
#	else
		(void)maxMaxPending;
#	endif
}

/*static*/ inline bool grcGfxContext::isDeferred() {
	return sm_Current->m_Deferred;
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
		CompileTimeAssert(grcContextAllocScope::ID_INVALID  == false);
		CompileTimeAssert(grcContextAllocScope::ID_INFINITE == true);
		return (bool)id;
	}
	const grcContextAllocScope *scope = grcGfxContext::sm_Current->m_AllocScope;
	do {
		if (scope->id() == id)
			return true;
		scope = scope->parent();
	} while (scope);
	return false;
}

inline void *grcGfxContext::allocateFromCommandBuffer(u32 sizeBytes, sce::Gnm::EmbeddedDataAlignment log2Align) {
	return allocateFromCommandBufferAlignBytes(sizeBytes, 1<<log2Align);
}

inline void *grcGfxContext::allocateFromCommandBuffer(sce::Gnm::SizeAlign sizeAlign) {
	return allocateFromCommandBufferAlignBytes(sizeAlign.m_size, sizeAlign.m_align);
}

template<class T>
inline T *grcGfxContext::allocateFromCommandBuffer(u32 num) {
	return (T*) allocateFromCommandBufferAlignBytes(num*sizeof(T), __alignof(T));
}

inline void *grcGfxContext::allocateTemp(u32 sizeBytes, u32 alignBytes) {
	return allocateFromCommandBufferAlignBytes(sizeBytes, alignBytes);
}

template<class T>
inline T *grcGfxContext::allocateTemp(u32 num) {
	return allocateFromCommandBuffer<T>(num);
}

inline void *grcGfxContext::untrackedAllocateTemp(u32 sizeBytes, u32 alignBytes) {
	return untrackedAllocateFromCommandBufferAlignBytes(sizeBytes, alignBytes);
}

/*static*/ inline void grcGfxContext::setBufferCachePolicy(sce::Gnm::Buffer &buf) {
	buf.setResourceMemoryType(BUFFER_MEM_TYPE, BUFFER_L1_CACHE_POLICY, BUFFER_K_CACHE_POLICY);
}

/*static*/ inline void grcGfxContext::initAsConstantBuffer(sce::Gnm::Buffer &buf, void *baseAddr, u32 sizeBytes) {
	rage::initAsConstantBuffer(buf, baseAddr, sizeBytes);
	setBufferCachePolicy(buf);
}

/*static*/ inline void grcGfxContext::initAsVertexBuffer(sce::Gnm::Buffer &buf, void *baseAddr, unsigned stride, unsigned numVertices, u32 dword3
	WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(, sce::Gnm::DataFormat fmt)) {

	rage::initAsVertexBuffer(buf, baseAddr, stride, numVertices, dword3 WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(, fmt));
	setBufferCachePolicy(buf);
}

inline u32 *grcGfxContext::embedDataDcb(u32 numDwords, sce::Gnm::EmbeddedDataAlignment log2Align) {
	return embedData(&m_dcb, numDwords, log2Align);
}

#if GRCGFXCONTEXT_GNM_USES_CCB
	inline u32 *grcGfxContext::embedDataCcb(u32 numDwords, sce::Gnm::EmbeddedDataAlignment log2Align) {
		return embedData(&m_ccb, numDwords, log2Align);
	}
#endif

inline void grcGfxContext::prePreDraw() {
#	if GRCGFXCONTEXT_GNM_USES_CCB
		m_InDrawOrDispatch = true;
#	endif
	flushVsWaveLimit();
}

inline void grcGfxContext::prePreDispatch() {
#	if GRCGFXCONTEXT_GNM_USES_CCB
		m_InDrawOrDispatch = true;
#	endif
}

inline void grcGfxContext::postPostDraw() {
#	if GRCGFXCONTEXT_GNM_USES_CCB
		m_InDrawOrDispatch = false;
		safeKickPoint();
#	endif
#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		markEnd();
#	endif
}

inline void grcGfxContext::postPostDispatch() {
	postPostDraw();
}

inline void grcGfxContext::setVsWaveLimit(u32 vsWaveLimit) {
	m_VsWaveLimitCurrent = vsWaveLimit;
}

inline void grcGfxContext::dispatch(u32 threadGroupX, u32 threadGroupY, u32 threadGroupZ) {
	prePreDispatch();
	grcGfxContextBase::dispatch(threadGroupX, threadGroupY, threadGroupZ);
	postPostDispatch();
}

inline void grcGfxContext::dispatchIndirect(u32 dataOffsetInBytes) {
	prePreDispatch();
	grcGfxContextBase::dispatchIndirect(dataOffsetInBytes);
	postPostDispatch();
}

inline void grcGfxContext::drawIndex(u32 indexCount, const void *indexAddr) {
	prePreDraw();
	grcGfxContextBase::drawIndex(indexCount, indexAddr);
	postPostDraw();
}

inline void grcGfxContext::drawIndexAuto(u32 indexCount) {
	prePreDraw();
	grcGfxContextBase::drawIndexAuto(indexCount);
	postPostDraw();
}

inline void grcGfxContext::drawIndexIndirect(u32 dataOffsetInBytes) {
	prePreDraw();
	grcGfxContextBase::drawIndexIndirect(dataOffsetInBytes);
	postPostDraw();
}

inline void grcGfxContext::drawIndexInline(u32 indexCount, const void *indices, u32 indicesSizeInBytes) {
	prePreDraw();
	grcGfxContextBase::drawIndexInline(indexCount, indices, indicesSizeInBytes);
	postPostDraw();
}

inline void grcGfxContext::drawIndexOffset(u32 indexOffset, u32 indexCount) {
	prePreDraw();
	grcGfxContextBase::drawIndexOffset(indexOffset, indexCount);
	postPostDraw();
}

inline void grcGfxContext::drawIndirect(u32 dataOffsetInBytes) {
	prePreDraw();
	grcGfxContextBase::drawIndirect(dataOffsetInBytes);
	postPostDraw();
}

}
// namespace rage

#endif // RSG_ORBIS

#endif // GRCORE_GFXCONTEXT_GNM_H
