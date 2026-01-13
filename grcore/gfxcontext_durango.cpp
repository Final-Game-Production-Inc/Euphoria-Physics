//
// grcore/gfxcontext_durango.cpp
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#if RSG_DURANGO

#include "gfxcontext_durango.h"

#include "allocscope.h"
#include "device.h"
#include "effect.h"
#include "gfxcontextcontrol.h"
#include "gfxcontextprofile.h"
#include "wrapper_d3d.h"
#include "grmodel/shader.h"
#include "grprofile/timebars.h"
#include "math/amath.h"
#include "system/bit.h"
#include "system/bootmgr.h"
#include "system/cache.h"
#include "system/customhangdetection.h"
#include "system/interlocked.h"
#include "system/memops.h"
#include "system/memory.h"
#include "system/nelem.h"
#include "system/param.h"
#include "system/pix.h"
#include "system/timer.h"
#include "system/tmcommands.h"

#include <xdk.h>

#if ENABLE_PIX_TAGGING
#	define USE_PIX
#	pragma warning(push)
#	pragma warning(disable:4668)    // 'DBG' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#	include <pix.h>
#	pragma warning(pop)
#endif


namespace rage {

// Debug code to get the GPU to trash the non-command buffer parts of a segment
// (this will contain any pointers that allocateFromCommandBuffer returned) just
// before it marks the segment as no longer in use.  This should cause rendering
// artefacts if anyone is incorrectly still referencing something that has been
// released.  Though this is slow and makes post mortem debugging much more
// difficult, so it cannot be enabled by default.  Requires
// GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING.
#define ENABLE_TRASH_DATA_AFTER_RELEASE                             (0 && GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING)

// Debug code to trash command buffer contents as soon as they are freed.  Do
// not want this on by default as it makes post mortem debugging much more
// difficult, if not impossible.
#define ENABLE_TRASH_COMMAND_BUFFERS_ON_FREE                        (0)

// Due to its slowness, we avoid partial submit like the plague.  To properly
// exercise that code path for testing, this define can be set to a non-zero
// value.
#define ENABLE_PARTIAL_SUBMIT_STRESS_TEST                           (0)

// Enable TCR illegal code for directly modifying GPU command buffers.  Can be
// useful for local testing, but should not be used for anything else.
#define ENABLE_TCR_ILLEGAL_GPU_COMMAND_BUFFER_ACCESS                (0 && SYSTMCMD_ENABLE)

// Workaround for D3D bug that can free CCBs before the GPU has finished
// executing them.  See url:bugstar:2094035.
#define ENABLE_CCB_LIFETIME_BUG_WORKAROUND                          (1 && (GTA_VERSION/100)==5)


// Ring buffer size for grcGfxContextProfiler.  Only allocated when
// GRCGFXCONTEXTPROFILE_ENABLE is turned on.
#define PROFILER_BUFFER_SIZE_BYTES                                  (0x00100000)


XPARAM(longLifeCommandLists);

struct SegmentStart {
	SegmentStart   *prevSubmitted;
	SegmentStart   *nextSubmitted;
	volatile u32    inUseCountCmdBuf;
	volatile u32    inUseFlagGpuData;
	s32             inUseCountCpuData;          // set to -1 after being released to prevent reuse, see untrackedAllocateTemp for more details.
	u32             offsetFreeData;             // grows up
	u32             offsetFreeCmdBuf;           // grows down
	volatile u32    lastCacheInvalidate;
	u32             frameAndSubmitOrder;
};

// Used for keeping track of when segments are last accessed by the GPU.
// Segments are considerred "last accessed" when they are released.  Because the
// command buffers for deferred contexts are generated in parallel, we can't
// know the sequence number of the last cache flush.  So instead, the command
// buffers copy the value from s_LastCacheInvalidateGPU into the segment header.
// s_LastCacheInvalidateCPU is the CPU copy.
static u32  s_LastCacheInvalidateCPU;
static u32 *s_LastCacheInvalidateGPU;

#if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
	static u64 *s_Checkpoint;
	static u64 *s_IndirectBuffer;
#endif

#if ENABLE_CCB_LIFETIME_BUG_WORKAROUND
	static SegmentStart *s_FinishedCmdBufs[256];
	static volatile u32 *s_FinishedCmdBufsNextGpu;
	static u32 s_FinishedCmdBufsNextCpuWrite;
	static u32 s_FinishedCmdBufsNextCpuRead;
#endif

#if ENABLE_TCR_ILLEGAL_GPU_COMMAND_BUFFER_ACCESS

	static GpuCmdBufAccess s_GpuCmdBufAccess;

	// The logic of how to add to a the DE command buffer is based on
	// disassembling parts of the D3D driver.  TinyD3D::SetShMemoryBaseAddress
	// is possibly a decent example to look at.
	//
	// An example of using these functions is
	//
	// 		u32 *const loadShReg = deReserveDwords(g_grcCurrentContext, 7);
	// 		loadShReg[0] = 0xc0035e00;                  // LOAD_UCONFIG_REG
	// 		loadShReg[1] = (u32)(uptr)(loadShReg+6);
	// 		loadShReg[2] = 0;
	// 		loadShReg[3] = 0x40;                        // SCRATCH_REG0
	// 		loadShReg[4] = 1;
	// 		loadShReg[5] = 0xc0001000;                  // NOP
	// 		loadShReg[6] = 0xdecafbad;
	// 		deDoneCommand(g_grcCurrentContext);
	//

	static inline u32 *deReserveDwords(ID3D11DeviceContextX *ctx, u32 numDwords) {
		u32 *pos   = *(u32**)((uptr)ctx+s_GpuCmdBufAccess.offsDeBufferPos);
		u32 *limit = *(u32**)((uptr)ctx+s_GpuCmdBufAccess.offsDeBufferLimit);
		if (pos+numDwords > limit) {
			void *tinyDevice = (char*)ctx+s_GpuCmdBufAccess.offsTinyDevice;
			u32 wtf0 = 4;   // \_ no idea what these values do
			u32 wtf1 = 4;   // /  copied these numbers from somewhere and they seem to work :/
			s_GpuCmdBufAccess.StartNewSegment(tinyDevice, wtf0, wtf1);
			pos = *(u32**)((uptr)ctx+s_GpuCmdBufAccess.offsDeBufferPos);
		}
		*(u32*)((uptr)ctx+s_GpuCmdBufAccess.offsDeBufferActiveDwords) = numDwords;
		return pos;
	}

	static inline void deDoneCommand(ID3D11DeviceContextX *ctx) {
		*(u32**)((uptr)ctx+s_GpuCmdBufAccess.offsDeBufferPos) += *(u32*)((uptr)ctx+s_GpuCmdBufAccess.offsDeBufferActiveDwords);
		*(u32*)((uptr)ctx+s_GpuCmdBufAccess.offsDeBufferActiveDwords) = 0;
	}

#endif

static void garbageCollect() {
	GRCDEVICE.GetCurrent()->GarbageCollect(0);

#	if ENABLE_CCB_LIFETIME_BUG_WORKAROUND
		u32 r = s_FinishedCmdBufsNextCpuRead;
		const u32 g = *s_FinishedCmdBufsNextGpu;
		ASSERT_ONLY(const u32 w = s_FinishedCmdBufsNextCpuWrite;)
		while (r != g) {
			FatalAssert(r != w);
			SegmentStart *const seg = s_FinishedCmdBufs[r];
			ASSERT_ONLY(s_FinishedCmdBufs[r] = NULL;)
			ASSERT_ONLY(const u32 decremented=) sysInterlockedDecrement(&(seg->inUseCountCmdBuf));
			FatalAssert(decremented != ~0u);
			r = (r+1)&(NELEM(s_FinishedCmdBufs)-1);
		}
		s_FinishedCmdBufsNextCpuRead = r;
#	endif
}

class SegmentHeap {
public:

#	if !GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
		bool invariant() const {
			return true;
		}
#	else
		bool invariant() const {
			SYS_CS_SYNC(m_Lock);

#			if 0
#				define INVARIANT_FAILURE_BREAK()   (void)0
#			else
#				define INVARIANT_FAILURE_BREAK()   __debugbreak()
#			endif

#			define CHECK_INVARIANT(EXP)                                        \
			do {                                                               \
				if (Unlikely(!(EXP))) {                                        \
					INVARIANT_FAILURE_BREAK();                                 \
					printf("Invariant failure: %s\n", #EXP);                   \
					return false;                                              \
				}                                                              \
			} while (0)

			// Validate the order and linkage of the submitted list.
			const SegmentStart *prev = m_OldestSubmitted;
			u32 numSubmitted = 0;
			if (prev != submittedListSentinal()) {
				CHECK_INVARIANT(prev->prevSubmitted == submittedListSentinal());
				for (;;) {
					++numSubmitted;
					const SegmentStart *curr = prev->nextSubmitted;
					if (curr == submittedListSentinal())
						break;
					CHECK_INVARIANT(curr->prevSubmitted == prev);
					CHECK_INVARIANT((s32)((curr->frameAndSubmitOrder<<8) - (prev->frameAndSubmitOrder<<8)) > 0);
					prev = curr;
				}
			}
			CHECK_INVARIANT(m_LastSubmitted == prev);

			CHECK_INVARIANT(numSubmitted + m_NumPending == m_NumSegs);

#			undef CHECK_INVARIANT
#			undef INVARIANT_FAILURE_BREAK

			return true;
		}

#	endif // GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING

	void incFrame() {
		++m_Frame;

		// There should be no more segments pending than one per context (for
		// the next DCB/CCB pair).  Any more than that and there has been a
		// leak.
		FatalAssert(m_NumPending <= NUMBER_OF_RENDER_THREADS);

		// And there should be no cross context allocations that have not yet
		// been released.
#		if GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING
			for (unsigned i=0; i<m_NumSegs; ++i)
				FatalAssert(!m_CrossContextAllocs[i]);
#		endif

		garbageCollect();
	}

	void init(void *start, u32 count, u32 segSizeBytes) {
		// CP_COHER_BASE is set to base address divided by 256.  So for cache
		// invalidations to work correctly, the allocation must be aligned.
		FatalAssert(((uptr)start&0xff) == 0);

		// Similarly CP_COHER_SIZE must also be a 256 byte multiple.
		FatalAssert(((count*segSizeBytes)&0xff) == 0);

		// Initialize each segment as not in use, and link them into the
		// submitted list.
		m_Base      = start;
		m_SegSize   = segSizeBytes;
		m_NumSegs   = count;
		m_PoolSize  = count*segSizeBytes;
		SegmentStart *next = (SegmentStart*)start;
		SegmentStart *end  = (SegmentStart*)((char*)next+count*segSizeBytes);
		SegmentStart *prev = submittedListSentinal();
		m_OldestSubmitted           = next;
		m_SubmitOrder               = -(s32)count;
		m_Frame                     = 0;
#		if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
			m_NumPending                = 0;
			m_CrossContextAllocs        = rage_new u32[count];
			sysMemSet(m_CrossContextAllocs, 0, sizeof(u32)*count);
#		endif
		do {
			SegmentStart *curr = next;
			next = (SegmentStart*)((char*)curr+segSizeBytes);
#			if ENABLE_TRASH_DATA_AFTER_RELEASE
				sysMemSet(curr+1, 0xfe, segSizeBytes-sizeof(*curr));
#			endif
			next = next<end ? next : NULL;
			curr->inUseCountCmdBuf      = 0;
			curr->inUseFlagGpuData      = 0;
			curr->inUseCountCpuData     = 0;
			curr->prevSubmitted         = prev;
			curr->nextSubmitted         = next ? next : submittedListSentinal();
			curr->offsetFreeData        = sizeof(*curr);
			curr->offsetFreeCmdBuf      = segSizeBytes;
			curr->lastCacheInvalidate   = (u32)-1;
			curr->frameAndSubmitOrder   = 0xff000000 | (m_SubmitOrder++);
			prev = curr;
		} while (next);
		m_LastSubmitted = prev;
		FatalAssert(invariant());
	}

	SegmentStart *alloc() {
		// Loop through the list of already submitted segments, oldest to
		// newest, and find the first one that is safe to reuse.
		SYS_CS_SYNC(m_Lock);
		SegmentStart *seg = m_OldestSubmitted;
		SegmentStart *end = submittedListSentinal();
		SegmentStart *prev = end;
		bool hasCollectedGarbage = false;
		while (seg != end) {
			SegmentStart *const next = seg->nextSubmitted;
			if (!seg->inUseFlagGpuData) {
				if (!seg->inUseCountCmdBuf) {
					FatalAssert((u32)(seg->inUseCountCpuData+1) < 2u);  // could validly be -1 or 0
					prev->nextSubmitted         = next;
					next->prevSubmitted         = prev;
					seg->prevSubmitted          = NULL;
					seg->nextSubmitted          = NULL;
					seg->inUseCountCmdBuf       = 0;
					seg->inUseFlagGpuData       = 0;
					seg->inUseCountCpuData      = 0;
					seg->offsetFreeData         = sizeof(*seg);
					seg->offsetFreeCmdBuf       = m_SegSize;
					// Do not clear lastCacheInvalidate here, prepareSubmit needs
					// the old value to determine if the segment is being used
					// within the same frame (and .: gpu cache invalidations are
					// required).
					seg->frameAndSubmitOrder    = 0;
#					if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
						++m_NumPending;
						FatalAssert(invariant());
#					endif
					return seg;
				}
				else if (!hasCollectedGarbage) {
					hasCollectedGarbage = true;
					garbageCollect();
					// Recheck the same segment again as it is likely to now be free
					continue;
				}
			}
			prev = seg;
			seg = next;
		}
		return NULL;
	}

	bool prepareSubmit(SegmentStart **segs, u32 num, bool ASSERT_ONLY(isPartialSubmit)) {
		FatalAssert(num);

		SYS_CS_SYNC(m_Lock);

		const u32 shiftedFrame = m_Frame<<24;
		bool needsCacheInvalidation = false;

		// Link segments into the submitted list
		const u32 lastCacheInvalidate = s_LastCacheInvalidateCPU;
		SegmentStart *prevSubmitted = m_LastSubmitted;
		for (u32 i=0; i<num; ++i) {
			SegmentStart *const seg = segs[i];
			FatalAssert(seg->prevSubmitted == NULL);
			FatalAssert(seg->nextSubmitted == NULL);
			prevSubmitted->nextSubmitted = seg;
			seg->prevSubmitted = prevSubmitted;
			prevSubmitted = seg;
			seg->frameAndSubmitOrder = shiftedFrame | ((m_SubmitOrder++)&0x00ffffff);

			// If this segment was previously accessed by the GPU after the last
			// invalidation of the cache lines, then need to invalidate them
			// again.  Otherwise stale values may be still be cached.
			if ((s32)(lastCacheInvalidate - seg->lastCacheInvalidate) <= 0)
				needsCacheInvalidation = true;

			FatalAssert(isPartialSubmit || (u32)(seg->inUseCountCpuData+1)<2u || numCrossContextAllocs(getSegIndex(seg)));
		}
		prevSubmitted->nextSubmitted = submittedListSentinal();
		m_LastSubmitted = prevSubmitted;

#		if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
			m_NumPending -= num;
			FatalAssert(invariant());
#		endif

		return needsCacheInvalidation;
	}

	unsigned getSegSize() const {
		return m_SegSize;
	}

	unsigned getNumSegs() const {
		return m_NumSegs;
	}

	uptr tryGetSegIndex(const void *ptr) const {
		return ((uptr)ptr - (uptr)m_Base) / m_SegSize;
	}

	unsigned getSegIndex(const void *ptr) const {
		const uptr idx = tryGetSegIndex(ptr);
		FatalAssert(idx < m_NumSegs);
		return (unsigned)idx;
	}

	SegmentStart *getSeg(unsigned idx) const {
		return (SegmentStart*)((char*)m_Base + idx*m_SegSize);
	}

	SegmentStart *tryGetSeg(void *ptr) const {
		const uptr idx = tryGetSegIndex(ptr);
		if (idx < m_NumSegs)
			return getSeg((unsigned)idx);
		return NULL;
	}

	SegmentStart *getSeg(void *ptr) const {
		return getSeg(getSegIndex(ptr));
	}

	const SegmentStart *getSeg(const void *ptr) const {
		return getSeg(const_cast<void*>(ptr));
	}

	void lock() {
		m_Lock.Lock();
	}

	void unlock() {
		m_Lock.Unlock();
	}

	SegmentStart *getOldestSubmitted() {
		return m_OldestSubmitted;
	}

	SegmentStart *submittedListSentinal() {
		CompileTimeAssert(offsetof(SegmentStart, nextSubmitted)-offsetof(SegmentStart, prevSubmitted)
			== offsetof(SegmentHeap, m_OldestSubmitted)-offsetof(SegmentHeap, m_LastSubmitted));
		return (SegmentStart*)((uptr)&m_LastSubmitted - offsetof(SegmentStart, prevSubmitted));
	}

	const SegmentStart *submittedListSentinal() const {
		return const_cast<SegmentHeap*>(this)->submittedListSentinal();
	}

#	if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
		u32 numCrossContextAllocs(unsigned segIdx) const {
			return m_CrossContextAllocs[segIdx];
		}
#	endif

	void crossContextIncSegmentRefCount(const void *ptr) {
		SegmentStart *const seg = const_cast<SegmentStart*>(getSeg(ptr));
		FatalAssert(seg->inUseFlagGpuData);
		FatalAssert(seg->inUseCountCpuData > 0);
		++(seg->inUseCountCpuData);
#		if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
			const unsigned idx = getSegIndex(seg);
			SYS_CS_SYNC(m_Lock);
			++(m_CrossContextAllocs[idx]);
			FatalAssert(m_CrossContextAllocs[idx]);
#		endif
	}

	SegmentStart *crossContextDecSegmentRefCount(const void *ptr) {
		SegmentStart *const seg = const_cast<SegmentStart*>(getSeg(ptr));
		FatalAssert(seg->inUseFlagGpuData);
		FatalAssert(seg->inUseCountCpuData > 0);
		--(seg->inUseCountCpuData);
#		if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
			const unsigned idx = getSegIndex(seg);
			SYS_CS_SYNC(m_Lock);
			FatalAssert(m_CrossContextAllocs[idx]);
			--(m_CrossContextAllocs[idx]);
#		endif
		return seg->inUseCountCpuData ? NULL : seg;
	}

	void flushGpuCaches(ID3D11DeviceContextX *immediateCtx) {
		// FlushGpuCacheRange issues a SURFACE_SYNC that is issued at
		// top-of-pipe, so need to wait for previous shaders to complete.
		immediateCtx->InsertWaitUntilIdle(0);

		// The segments can contain index buffers, so cache invalidation needs to be
		// done by the PFP (which also prefetches index buffers), not the ME.
		immediateCtx->FlushGpuCacheRange(
			(UINT)D3D11_FLUSH_ENGINE_PFP | D3D11_FLUSH_TEXTURE_L1_INVALIDATE | D3D11_FLUSH_KCACHE_INVALIDATE | D3D11_FLUSH_TEXTURE_L2_INVALIDATE,
			m_Base, m_PoolSize);

		immediateCtx->FillMemoryWithValue(s_LastCacheInvalidateGPU, 4, ++s_LastCacheInvalidateCPU);
	}

private:

	mutable sysCriticalSectionToken m_Lock;

	void         *m_Base;
	u32           m_SegSize;
	u32           m_NumSegs;
	u32           m_PoolSize;

	// These two pointer must be kept in the same order as
	// SegmentStart::prevSubmitted, SegmentStart::nextSubmitted.
	SegmentStart *m_LastSubmitted;
	SegmentStart *m_OldestSubmitted;

	u32 m_SubmitOrder;
	u32 m_Frame;

#	if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
		u32  m_NumPending;
		u32 *m_CrossContextAllocs;
#	endif
};

static SegmentHeap s_SegHeap;

static grcGfxContext *s_Contexts;

static u32 s_MaxMaxPending;

/*static*/ __THREAD grcGfxContext *grcGfxContext::sm_Current;
static grcGfxContextControl s_DefaultControl;

enum { CERAM_BAK_SIZE_BYTES = D3D11X_CERAM_OFFSET_LIMIT };
static ID3D11Buffer *s_ceramBak;

static inline u32 getPartialSubmitThreshold() {
	// Load/StoreConstantRam and cache invalidation is slow, so keep threshold high to avoid partial submit as much as possible.
	return ENABLE_PARTIAL_SUBMIT_STRESS_TEST ? 0 : s_MaxMaxPending-1;
}

static inline void *tryAllocDataFromSeg(void *seg_, u32 sizeBytes, u32 alignBytesSub1) {
	SegmentStart *const seg = (SegmentStart*)seg_;
	FatalAssert(seg);
	FatalAssert(((sizeof(SegmentStart)+alignBytesSub1)&alignBytesSub1)+sizeBytes <= s_SegHeap.getSegSize());
	const u32 offsetAlloc = (seg->offsetFreeData+alignBytesSub1)&~alignBytesSub1;
	const u32 offsetFree = offsetAlloc+sizeBytes;
	if (offsetFree <= seg->offsetFreeCmdBuf) {
		seg->offsetFreeData = offsetFree;
		return (char*)seg+offsetAlloc;
	}
	return NULL;
}

static inline void *tryAllocCmdBufFromSeg(void *seg_, size_t sizeBytes64) {
	SegmentStart *const seg = (SegmentStart*)seg_;
	FatalAssert(seg);
	const u32 sizeBytes = (u32)sizeBytes64;
	const u32 offsetFreeCmdBuf = seg->offsetFreeCmdBuf-sizeBytes;
	if (offsetFreeCmdBuf >= seg->offsetFreeData) {
		seg->offsetFreeCmdBuf = offsetFreeCmdBuf;
		ASSERT_ONLY(u32 incremented=) sysInterlockedIncrement(&(seg->inUseCountCmdBuf));
		FatalAssert(incremented);
		return (char*)seg+offsetFreeCmdBuf;
	}
	return NULL;
}

inline void grcGfxContext::setDataSegment(void *seg_) {
	SegmentStart *const prev = (SegmentStart*)m_DataSegment;

	SegmentStart *const seg = (SegmentStart*)seg_;
	m_DataSegment = seg;
	if (seg) {
		seg->inUseFlagGpuData = 1;
		FatalAssert(seg->inUseCountCpuData >= 0);
		++(seg->inUseCountCpuData);

#		if GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING
			m_SegmentsNotYetReleased.Set(s_SegHeap.getSegIndex(seg));
#		endif
	}

	if (prev)
		releaseSegment(prev);
}

inline void grcGfxContext::setCmdBufSegment(void *seg_) {
	FatalAssert(m_Deferred);

	SegmentStart *const prev = (SegmentStart*)m_CmdBufSegment;
	SegmentStart *const seg = (SegmentStart*)seg_;
	FatalAssert(seg != prev);
	m_CmdBufSegment = seg;
	if (seg)
#		if ENABLE_CCB_LIFETIME_BUG_WORKAROUND
			sysInterlockedAdd(&(seg->inUseCountCmdBuf), 2);
#		else
			sysInterlockedIncrement(&(seg->inUseCountCmdBuf));
#		endif

	if (prev)
		sysInterlockedDecrement(&(prev->inUseCountCmdBuf));
}

void *grcGfxContext::allocCmdBufFromNewSeg(size_t sizeBytes) {
	SegmentStart *const seg = (SegmentStart*)allocSegment(ALLOC_SEGMENT_DO_NOT_ALLOW_PARTIAL_SUBMIT);
	setCmdBufSegment(seg);
	void *const cmdBuf = tryAllocCmdBufFromSeg(seg, sizeBytes);
	FatalAssert(cmdBuf);
	return cmdBuf;
}

#if __ASSERT || ENABLE_TRASH_COMMAND_BUFFERS_ON_FREE
enum { EXPECTED_COMMAND_BUFFER_ALLOC_SIZE_BYTES = 0x10000 };
#endif

/*static*/ void *grcGfxContext::sysMemXMemAllocHook(size_t sizeBytes, u64 attributes) {
	XALLOC_ATTRIBUTES a;
	a.dwAttributes = attributes;
	if (a.s.dwMemoryType == XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_CACHEABLE) {
		// If sm_Current is NULL, then the command buffer allocation is not for
		// a deferred context, so ignore and fall through to default handling.
		//
		// Note that we also need to ignore requests from non-deferred contexts.
		// 99.9% of the time, the immediate context will use the primary ring
		// buffer, so it will not attempt to allocate any more memory.  But in
		// the case of a PIX GPU capture, it does.
		//
		grcGfxContext *const curr = sm_Current;
		if (curr && curr->m_Deferred) {
			FatalAssert(sizeBytes == EXPECTED_COMMAND_BUFFER_ALLOC_SIZE_BYTES);
			SegmentStart *const cmdBufSegment = (SegmentStart*)(curr->m_CmdBufSegment);

			// Always allocate a new segment if requested.
			if (curr->m_ForceNewCmdBufSegment) {
				curr->m_ForceNewCmdBufSegment = false;
				return curr->allocCmdBufFromNewSeg(sizeBytes);
			}

			// Try the current segment being used for command buffer allocations.
			FatalAssert(cmdBufSegment->inUseCountCmdBuf);
			void *cmdBuf = tryAllocCmdBufFromSeg(cmdBufSegment, sizeBytes);
			if (cmdBuf)
				return cmdBuf;

			// It is possible to fail the allocation during FinishCommandList.
			// That means we already started a new segment when
			// m_ForceNewCmdBufSegment was set, and now we need to start
			// another.  To prevent leaking a segment, add it to the submitted
			// list.  The ordering is a bit wrong, but that really doesn't
			// matter.
			if (curr->m_IsFinishCommandList) {
				const bool isPartialSubmit = false;
				s_SegHeap.prepareSubmit((SegmentStart**)&(curr->m_CmdBufSegment), 1, isPartialSubmit);
				return curr->allocCmdBufFromNewSeg(sizeBytes);
			}

			// Next try from the current segment being used for data
			// allocations.
			//
			// When closing a context, this needs special care.  At the end of
			// closeImmediateContext, curr->m_DataSegment is set to NULL, so
			// subsequent calls like FinishCommandList in closeContext need to
			// check for a NULL pointer.
			//
			// Also during a call to releaseSegment, if that runs out of command
			// buffer space and needs to allocate more, then it is possible to
			// encounter dataSegment->inUseCountCpuData == -1 or 0.  Either of
			// these two values mean that it is not safe to reuse the segment.
			//
			SegmentStart *const dataSegment = (SegmentStart*)(curr->m_DataSegment);
			if (dataSegment)
				if (dataSegment->inUseCountCpuData>0) {
					cmdBuf = tryAllocCmdBufFromSeg(dataSegment, sizeBytes);
					if (cmdBuf) {
						// Succeeded.  Swap over to doing command buffer allocations
						// from this segment now.
						curr->setCmdBufSegment(dataSegment);
						return cmdBuf;
					}
				}
				else {
					FatalAssert((u32)(dataSegment->inUseCountCpuData+1) < 2u);
				}

			// Neither segment has room, so need to allocate a new one.  This
			// allocation must not trigger a partial submit, since that requires
			// command buffer space.
			SegmentStart *const newSeg = (SegmentStart*)(curr->allocSegment(ALLOC_SEGMENT_DO_NOT_ALLOW_PARTIAL_SUBMIT));
			cmdBuf = tryAllocCmdBufFromSeg(newSeg, sizeBytes);
			FatalAssert(cmdBuf);
			curr->setCmdBufSegment(newSeg);

			curr->addSegmentToCommandList(newSeg);

			return cmdBuf;
		}
	}

	return XMemAllocDefault(sizeBytes, attributes);
}

/*static*/ void grcGfxContext::sysMemXMemFreeHook(void *address, u64 attributes) {
	XALLOC_ATTRIBUTES a;
	a.dwAttributes = attributes;
	if (a.s.dwMemoryType == XALLOC_MEMTYPE_GRAPHICS_COMMAND_BUFFER_CACHEABLE) {
		SegmentStart *const seg = s_SegHeap.tryGetSeg(address);
		if (seg) {
#			if ENABLE_TRASH_COMMAND_BUFFERS_ON_FREE
				sysMemSet(address, 0xfe, EXPECTED_COMMAND_BUFFER_ALLOC_SIZE_BYTES);
#			endif
			ASSERT_ONLY(u32 decremented=) sysInterlockedDecrement(&(seg->inUseCountCmdBuf));
			FatalAssert(decremented != ~0u);
			return;
		}
	}
# if __ASSERT
	else
		FatalAssert(!s_SegHeap.tryGetSeg(address));
# endif

	XMemFreeDefault(address, attributes);
}

#if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING

	static UINT HangBeginCallback(UINT64 UNUSED_PARAM(flags)) {
		// 0 = no detailed GPU hang, 1 = generate detailed GPU hang
		return 1;
	}

	// Currently just using the default print behaviour as that is fine for our
	// purposes.  If something else is required, can add it here.
// 	static void HangPrintCallback(const CHAR *line) {
// 	}

	static void HangDumpCallback(const WCHAR *SYSTMCMD_ENABLE_ONLY(filename)) {
#	  if SYSTMCMD_ENABLE
		if (PARAM_rockstartargetmanager.Get())
			sysTmCmdGpuHang(filename);
		else
#	  endif
		if (sysBootManager::IsDebuggerPresent())
			__debugbreak();
	}

#endif

/*static*/ void grcGfxContext::init(u32 numSegs, u32 segSizeBytes) {

#	if 1 // 4MiB pages to reduce TLB misses
		const size_t pageSizeBytes = 0x00400000;
		const u32 flags = MEM_4MB_PAGES | MEM_GRAPHICS | MEM_RESERVE | MEM_COMMIT;

#	else // 64KiB pages
		const size_t pageSizeBytes = 0x00010000;
		const u32 flags = MEM_LARGE_PAGES | MEM_GRAPHICS | MEM_RESERVE | MEM_COMMIT;
#	endif

	// Round up allocation size to page size multiple.
	const size_t allocSizeBytes = (numSegs*segSizeBytes+pageSizeBytes-1)&~(pageSizeBytes-1);

	// Increase numSegs if more will fit in the rounded up allocation size.
	numSegs = allocSizeBytes/segSizeBytes;
	FatalAssert(numSegs <= GRCCONTEXT_MAX_NUM_SEGMENTS);

	// Allocate segment heap memory as WB/ONION, with GPU execution allowed.
	const u32 type = PAGE_GPU_COHERENT | PAGE_GPU_EXECUTE | PAGE_READWRITE;
	void *const segPool = VirtualAlloc(NULL, allocSizeBytes, flags, type);
	FatalAssert(segPool);
	s_SegHeap.init(segPool, numSegs, segSizeBytes);

	// Initialize the per thread contexts.
	s_Contexts = rage_new grcGfxContext[NUMBER_OF_RENDER_THREADS];
	for (unsigned i=0; i<NUMBER_OF_RENDER_THREADS; ++i) {
		grcGfxContext *const ctx = s_Contexts+i;
		ctx->m_AllocScope = &ctx->m_BaseAllocScope;
		FatalAssert(ctx->m_BaseAllocScope.isDefaultState());
		ctx->m_DataSegment   = NULL;
		ctx->m_CmdBufSegment = NULL;
#		if GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING
			ctx->m_SegmentsNotYetReleased.Reset();
#		endif
#		if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
			ctx->m_ContextSequenceNumber    = 0;
			ctx->m_CheckpointSquenceNumber  = 0;
#		endif
		ctx->m_Deferred                 = false;
		ctx->m_ForceNewCmdBufSegment    = true;
		ctx->m_IsFinishCommandList      = false;
		ctx->m_Control                  = NULL;
		ctx->m_CmdList                  = NULL;
	}

	s_DefaultControl.m_Atomic.m_Val |= grcGfxContextControl::States::ALLOW_PARTIAL_SUBMIT_MASK | grcGfxContextControl::States::MAX_PENDING_MASK;
	s_LastCacheInvalidateGPU = rage_new u32; // must be ONION or WC/GARLIC since gpu writes to this, so that cpu performance is decent really should be WB/ONION
	*s_LastCacheInvalidateGPU = 0;

	// Install memory allocation hooks to manage command buffer allocations.
	sysMemSetXMemHooks(eXALLOCAllocatorId_D3D, grcGfxContext::sysMemXMemAllocHook, grcGfxContext::sysMemXMemFreeHook);

	grcDeviceHandle *const dev = GRCDEVICE.GetCurrent();

#	if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
		dev->SetHangCallbacks(HangBeginCallback, NULL/*HangPrintCallback*/, HangDumpCallback);
#	endif

	// Align the allocation to 256 bytes since StoreConstantRam is using the
	// address in a SURFACE_SYNC PM4 command (which takes 256-byte aligned
	// addresses).
	//
	// Use rage_aligned_new rather than physical_new, since this memory needs to
	// be writable by the GPU (currently physical_new returns WB/GARLIC which
	// must be treated as GPU read-only).  WC/GARLIC would be faster for the
	// GPU, but performance doesn't really matter all that much (since we try to
	// avoid partial submits on XB1), so allocating the memory as WB/ONION is
	// fine.
	//
	CompileTimeAssert((CERAM_BAK_SIZE_BYTES&3) == 0);
	u32 *const ceramBak = rage_aligned_new(256) u32[CERAM_BAK_SIZE_BYTES/4];
	static const D3D11_BUFFER_DESC bufDesc = {
		D3D11X_CERAM_OFFSET_LIMIT, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS, 0, D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, 0, 0, 0};
	CHECK_HRESULT(dev->CreatePlacementBuffer(&bufDesc, ceramBak, &s_ceramBak));

#	if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
		s_Checkpoint = rage_new u64[2];
		s_IndirectBuffer = s_Checkpoint+1;
		*s_Checkpoint = *s_IndirectBuffer = 0;
#	endif

	grcGfxContextControl::initClass();

#	if GRCGFXCONTEXTPROFILE_ENABLE
		gGrcGfxContextProfiler.init(rage_aligned_new(16) char[PROFILER_BUFFER_SIZE_BYTES], PROFILER_BUFFER_SIZE_BYTES);
#	endif

#	if ENABLE_TCR_ILLEGAL_GPU_COMMAND_BUFFER_ACCESS
		if (PARAM_rockstartargetmanager.Get())
			if (!sysTmCmdGetGpuCommandBufferAccess(&s_GpuCmdBufAccess))
				Quitf("sysTmCmdGetGpuCommandBufferAccess failed\n");
#	endif

#	if ENABLE_CCB_LIFETIME_BUG_WORKAROUND
		CompileTimeAssert((NELEM(s_FinishedCmdBufs) & (NELEM(s_FinishedCmdBufs)-1)) == 0);
		FatalAssert(NELEM(s_FinishedCmdBufs) > numSegs); // must be > not >= so that ring buffer full condition can be distinguished from empty
		s_FinishedCmdBufsNextGpu = rage_aligned_new(8) u32;
		*s_FinishedCmdBufsNextGpu = 0;
#	endif
}

/*static*/ void grcGfxContext::perThreadInit() {

	// Temporarily set sm_Current since CreateDeferredContextX will allocate command buffer space.
	sm_Current = s_Contexts+g_RenderThreadIndex;
	sm_Current->m_Deferred = true;
	grcGfxContextControl dummy;
	dummy.setInitialMaxPending(1);
#	if ENABLE_PARTIAL_SUBMIT_STRESS_TEST
		// The stress test code spins until partial submit is enabled, so set
		// this flag to prevent a hang.  Nothing will actually be submitted
		// though, since nothing is pending.
		dummy.allowPartialSubmit();
#	endif
	sm_Current->m_Control = &dummy;

	// Create the D3D context.
 	UINT createFlags = D3D11_CREATE_DEFERRED_CONTEXT_TITLE_MANAGED_COMMAND_LIST_OBJECT_LIFETIMES;
#	if !__FINAL
		if (PARAM_longLifeCommandLists.Get())
			createFlags |= D3D11_CREATE_DEFERRED_CONTEXT_LONG_LIFE_COMMAND_LISTS;
#	endif
	FatalAssert(!g_grcCurrentContext);
	const HRESULT hr = GRCDEVICE.GetCurrent()->CreateDeferredContextX(createFlags, (ID3D11DeviceContextX**)&g_grcCurrentContext);
	if (FAILED(hr))
		Quitf("Unable to create thread context");

	// Clear sm_Current again.  Appart from this special case, should only be
	// non-NULL between openContext/closeContext calls.
	sm_Current->m_Control = NULL;
	sm_Current = NULL;
}

void grcGfxContext::createCommandList(void *seg_) {
	SegmentStart *const seg = (SegmentStart*)seg_;
	FatalAssert(seg == m_DataSegment);
	grcGfxContextCommandList *const nextCmdList = (grcGfxContextCommandList*)untrackedAllocateTemp(
		sizeof(grcGfxContextCommandList), __alignof(grcGfxContextCommandList));
	FatalAssert(seg == m_DataSegment);
	FatalAssert(seg->inUseFlagGpuData);
	FatalAssert(seg->inUseCountCpuData > 0);
	++(seg->inUseCountCpuData);
	nextCmdList->m_D3dCommandList = NULL;
	nextCmdList->m_NumSegs = 0;
	m_CmdList = nextCmdList;
}

void grcGfxContext::addSegmentToCommandList(void *seg) {
	grcGfxContextCommandList *const cmdList = m_CmdList;
	FatalAssert(cmdList);
	const u32 numSegs = cmdList->m_NumSegs;
	FatalAssert(numSegs < NELEM(cmdList->m_Segs));
	cmdList->m_Segs[numSegs] = seg;
	cmdList->m_NumSegs = numSegs+1;
}

/*static*/ grcGfxContext *grcGfxContext::openContext(bool deferred, u32 sequenceNumber, grcGfxContextControl *control) {
	grcGfxContext *const ctx = s_Contexts+g_RenderThreadIndex;
	sm_Current = ctx;
#	if GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING
		FatalAssert(!ctx->m_SegmentsNotYetReleased.AreAnySet());
#	endif
#	if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
		ctx->m_ContextSequenceNumber = sequenceNumber;
		ctx->m_CheckpointSquenceNumber = 0;
#	else
		(void)sequenceNumber;
#	endif
	s_DefaultControl.clearPending();                    // value does creep up, simplest just to force back to zero here
	ctx->m_Control = control ? control : &s_DefaultControl;
	FatalAssert(ctx->m_AllocScope == &ctx->m_BaseAllocScope);
	FatalAssert(ctx->m_AllocScope->isDefaultState());
	ctx->m_AllocScope->generateId();
	ctx->m_Deferred = deferred;
	SegmentStart *seg;
	if (deferred) {
		seg = (SegmentStart*)(ctx->m_CmdBufSegment);
		FatalAssert(seg);
		FatalAssert(seg->inUseCountCmdBuf != 0);
	}
	else {
		FatalAssert(ctx->m_CmdBufSegment == NULL);
		seg = (SegmentStart*)(ctx->allocSegment(ALLOC_SEGMENT_DO_NOT_ALLOW_PARTIAL_SUBMIT));
		FatalAssert(seg);
		FatalAssert(seg->inUseCountCmdBuf == 0);
	}
	FatalAssert(seg->inUseFlagGpuData  == 0);
	FatalAssert(seg->inUseCountCpuData == 0);
	FatalAssert(ctx->m_DataSegment == NULL);
	ctx->setDataSegment(seg);
	ctx->m_CmdList = NULL;
	if (deferred) {
		ctx->createCommandList(seg);
		ctx->addSegmentToCommandList(seg);
	}
	ctx->insertHangCheckpoint();
	return ctx;
}

/*static*/ void grcGfxContext::kickContextIfPartialSubmitsAllowed() {
	grcGfxContext *const ctx = sm_Current;
	if (ctx)
		if (ctx->m_Deferred) {
			grcGfxContextControl *const control = ctx->m_Control;
			if (control->isPartialSubmitAllowed() && control->getCurrPending()>0)
				ctx->doPartialSubmitAllocSeg();
		}
		else
			g_grcCurrentContext->Flush();
}

/*static*/ void grcGfxContext::disablePartialSubmit(bool disable) {
	grcGfxContextControl *const control = sm_Current->m_Control;
	FatalAssert(control != &s_DefaultControl);
	control->subrenderDisablePartialSubmit(disable);
}

/*static*/ bool grcGfxContext::emergencyPartialSubmit() {
	grcGfxContext *const curr = sm_Current;
	grcGfxContextControl *const control = curr->m_Control;
	FatalAssert((control->m_Atomic.m_Val&grcGfxContextControl::States::DISABLE_PARTIAL_SUBMIT_MASK) != 0);  // pointless to call when subrenderDisablePartialSubmit(false)
	if (control->isPartialSubmitAllowed() && control->getCurrPending()>getPartialSubmitThreshold()) {
		curr->doPartialSubmitAllocSeg();
		return true;
	}
	return false;
}

void grcGfxContext::closeImmediateContext() {
	m_BaseAllocScope.releaseAll();
	FatalAssert(m_AllocScope == &m_BaseAllocScope);
	FatalAssert(m_AllocScope->isDefaultState());

	FatalAssert(m_DataSegment);
	setDataSegment(NULL);

#	if GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING
#		if __ASSERT
			// Check that all data segments have been released or they have a cross context allocation in them.
			for (auto itor=m_SegmentsNotYetReleased.Begin(); itor!=m_SegmentsNotYetReleased.End(); ++itor)
				FatalAssert(s_SegHeap.numCrossContextAllocs(itor));
#		endif
		m_SegmentsNotYetReleased.Reset();
#	endif
}

/*static*/ grcGfxContextCommandList *grcGfxContext::closeContext() {
	grcGfxContext *const ctx = sm_Current;
	grcGfxContextCommandList *const cmdList = ctx->m_CmdList;
	SegmentStart *const cmdListSeg = (SegmentStart*)(cmdList->m_Segs[0]);
	FatalAssert(cmdList->m_NumSegs >= 1);
	FatalAssert(cmdListSeg == s_SegHeap.getSeg(cmdList));
	ctx->releaseSegment(cmdListSeg);
	ctx->closeImmediateContext();
	ctx->m_ForceNewCmdBufSegment = true;
	ctx->m_IsFinishCommandList = true;
	const BOOL restoreDeferredContextState = false;
	CHECK_HRESULT(g_grcCurrentContext->FinishCommandList(restoreDeferredContextState, &(cmdList->m_D3dCommandList)));
	ctx->m_IsFinishCommandList = false;
	FatalAssert(!ctx->m_ForceNewCmdBufSegment);

	// Don't NULL out sm_Current until after FinishCommandList has been called.
	// Useful for tracking command buffer memory allocations.
	sm_Current = NULL;

	return cmdList;
}

void grcGfxContextCommandList::kick(ID3D11CommandList *d3dCommandList, bool isPartialSubmit) {
	FatalAssert(d3dCommandList);
	const u32 numSegs = m_NumSegs;
	const bool cacheInvalidateRequired = s_SegHeap.prepareSubmit((SegmentStart**)m_Segs, numSegs, isPartialSubmit);
	ID3D11DeviceContextX *immediateCtx;
	GRCDEVICE.GetCurrent()->GetImmediateContextX(&immediateCtx);

#	if ENABLE_CCB_LIFETIME_BUG_WORKAROUND
		u32 w = s_FinishedCmdBufsNextCpuWrite;
		ASSERT_ONLY(const u32 r = s_FinishedCmdBufsNextCpuRead;)
		for (u32 i=0; i<numSegs; ++i) {
			SegmentStart *const seg = (SegmentStart*)(m_Segs[i]);
			if (seg->inUseCountCmdBuf) {
				s_FinishedCmdBufs[w] = seg;
				w = (w+1)&(NELEM(s_FinishedCmdBufs)-1);
				FatalAssert(w != r);
			}
		}
		s_FinishedCmdBufsNextCpuWrite = w;
#	endif

#	if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
		static u32 kickCount;
		immediateCtx->WriteValueBottomOfPipe(s_IndirectBuffer, kickCount);
#	endif

	if (cacheInvalidateRequired)
		s_SegHeap.flushGpuCaches(immediateCtx);

	grcGfxContextProfiler_KickBegin(numSegs);
	const BOOL restoreContextState = false;
	immediateCtx->ExecuteCommandList(d3dCommandList, restoreContextState);
	grcGfxContextProfiler_KickEnd();

	// If we are doing a partial flush, then by simply doing a raw copy of the
	// ceram contents, the deferred context loses the ability to do resource
	// hazard tracking correctly.  The plan is to move to "set fast" context
	// semantics where we need to handle all hazards ourself (like on Orbis),
	// but for now, just invalidate all caches.  This is expensive, but we do
	// try very hard to never need to do this.  If this shows up in profiling,
	// that means we need avoid the situation by either increasing the number of
	// segments in the pool, or splitting up the draw lists more.
	if (isPartialSubmit) {
		immediateCtx->InsertWaitUntilIdle(0);
		immediateCtx->FlushGpuCacheRange(
			  (UINT)D3D11_FLUSH_TEXTURE_L1_INVALIDATE
			| (UINT)D3D11_FLUSH_TEXTURE_L2_INVALIDATE
			| (UINT)D3D11_FLUSH_COLOR_BLOCK_INVALIDATE
			| (UINT)D3D11_FLUSH_DEPTH_BLOCK_INVALIDATE
			| (UINT)D3D11_FLUSH_KCACHE_INVALIDATE
			| (UINT)D3D11_FLUSH_ENGINE_PFP,
			NULL, 0);
	}

	d3dCommandList->Release();

#	if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
		immediateCtx->WriteValueBottomOfPipe(s_IndirectBuffer, 0x80000000|kickCount);
		kickCount = (kickCount+1)&0x7fffffff;
#	endif

#	if ENABLE_CCB_LIFETIME_BUG_WORKAROUND
		class CeAccessID3D11DeviceContextX : public ID3D11DeviceContextX {
		public:
			using ID3D11DeviceContextX::BeginCe;
			using ID3D11DeviceContextX::EndCe;
		};
		auto *const ceCtx = (CeAccessID3D11DeviceContextX*)immediateCtx;
		u32 *const ccb = ceCtx->BeginCe<5*sizeof(u32)>();
		ccb[0] = 0xc0033700;    // WRITE_DATA
		ccb[1] = 0x80000500;    // ENGINE_SEL=CE, WR_CONFIRM=0, WR_ONE_ADDR=0, DST_SEL=memory(async) [note that CE version of this packet does not have a WRITE_DATA_CACHE_POLICY field]
		ccb[2] = (u32)(uptr)s_FinishedCmdBufsNextGpu;           // DST_ADDR_LO
		ccb[3] = (u32)((uptr)s_FinishedCmdBufsNextGpu>>32);     // DST_ADDR_HI
		ccb[4] = w;                                             // DATA
		ceCtx->EndCe(ccb+5);
#	endif

	immediateCtx->Flush();

	immediateCtx->Release();
}

void grcGfxContextCommandList::kick(bool isPartialSubmit) {
	kick(m_D3dCommandList, isPartialSubmit);
}

/*static*/ void grcGfxContext::endFrame() {
	sm_Current->closeImmediateContext();
	s_SegHeap.incFrame();
	ID3D11DeviceContextX *const ctx11x = g_grcCurrentContext;
#	if __ASSERT
		ID3D11DeviceContextX *immediateCtx;
		GRCDEVICE.GetCurrent()->GetImmediateContextX(&immediateCtx);
		FatalAssert(ctx11x == immediateCtx);
		immediateCtx->Release();
#	endif
	ctx11x->FillMemoryWithValue(s_LastCacheInvalidateGPU, 4, ++s_LastCacheInvalidateCPU);
	openContext(false, GRCDEVICE.GetAndIncContextSequenceNumber());
}

/*static*/ u32 grcGfxContext::getTotalSegmentCount() {
	return s_SegHeap.getNumSegs();
}

/*static*/ void grcGfxContext::setMaxMaxPending(u32 maxMaxPending) {
	s_MaxMaxPending = maxMaxPending;
}

void *grcGfxContext::untrackedAllocateTemp(u32 sizeBytes, u32 alignBytes) {
	// If this is an immediate context, allocations must be aligned to a cache
	// line boundary to ensure that the GPU never fetches data before the CPU
	// has writen it.
	//
	// Note that m_AllowPartialSubmit should be handled differently.  Not all
	// allocations need to be cache line aligned, instead we can just round up
	// SegmentStart::offsetFree whenever a kick is performed.  This is different
	// to the immediate context, where we don't have any direct control over
	// when D3D decides to do a kick.
	//
	const bool deferred = m_Deferred;
	const u32 minAlignBytes = deferred ? 1 : RSG_CACHE_LINE_SIZE;
	alignBytes = Max(alignBytes, minAlignBytes);
	const u32 alignBytesSub1 = alignBytes-1;

	// Attempt to allocate from the current data segment first.
	void *alloc = tryAllocDataFromSeg(m_DataSegment, sizeBytes, alignBytesSub1);
	if (alloc)
		return alloc;

	// We know we need to switch data segments, so do the release on the old one
	// now.  This makes the behaviour consistent whether or not allocSegment
	// ends up doing a partial submit (since it will also release the data
	// segment if non-NULL).
	setDataSegment(NULL);

	// Check if the current command buffer segment can be used instead.
	//
	// Note the check for inUseCountCpuData being greater than zero or equal to
	// zero.  This is to prevent the scenario where the code and data are
	// sharing the same segment, then a large data allocation fails.  This
	// triggers a release on the data segment, then allocates a new one.  If
	// this new data segment then fills up before the command buffer segment,
	// the code could attempt to reuse the command buffer segment even though it
	// has already done a data release.
	//
	SegmentStart *dataSeg = (SegmentStart*)m_CmdBufSegment;
	FatalAssert(!!dataSeg ^ !deferred);
	if (dataSeg)
		if (dataSeg->inUseCountCpuData >= 0)
			alloc = tryAllocDataFromSeg(dataSeg, sizeBytes, alignBytesSub1);
		else {
			FatalAssert(dataSeg->inUseCountCpuData == -1);
		}

	// No space in the command buffer segment?  Then allocate a new segment for
	// the data.
	if (!alloc) {
		dataSeg = (SegmentStart*)allocSegment(ALLOC_SEGMENT_ALLOW_PARTIAL_SUBMIT);
		const u32 offsetAlloc = (dataSeg->offsetFreeData+alignBytesSub1)&~alignBytesSub1;
		const u32 offsetFree = offsetAlloc+sizeBytes;
		FatalAssert(offsetFree <= s_SegHeap.getSegSize());
		// For deferred contexts, the newly allocated segment needs to be added
		// to the command list.
		if (deferred)
			addSegmentToCommandList(dataSeg);
		dataSeg->offsetFreeData = offsetFree;
		alloc = (char*)dataSeg+offsetAlloc;
	}

	setDataSegment(dataSeg);
	return alloc;
}

void *grcGfxContext::allocateTemp(u32 sizeBytes, u32 alignBytes) {
	grcContextAllocScope *const scope = m_AllocScope;

	scope->assertNotLocked();

#	if __ASSERT
		scope->m_SizeAlloced += sizeBytes;
#	endif

	void *const ret = untrackedAllocateTemp(sizeBytes, alignBytes);

	// Record allocation in current alloc scope.
	SegmentStart *const seg = (SegmentStart*)m_DataSegment;
	FatalAssert(s_SegHeap.getSeg(ret) == seg);
	const unsigned segIdx = s_SegHeap.getSegIndex(seg);
	if (scope->m_AllocedSegs.IsClear(segIdx)) {
		scope->m_AllocedSegs.Set(segIdx);
		FatalAssert(seg->inUseFlagGpuData);
		FatalAssert(seg->inUseCountCpuData > 0);
		++(seg->inUseCountCpuData);
	}

	return ret;
}

void grcGfxContext::safeKickPoint() {
	if (isDeferred()) {
		grcGfxContextControl *const control = m_Control;
		const u32 maxPending = control->getMaxPending();
		const u32 maxMaxPending = s_MaxMaxPending;
		if (maxPending < maxMaxPending)
			return;

		const u32 pendingCount = control->getCurrPending();
		if (pendingCount <= (maxMaxPending>>1))
			return;

		if (pendingCount<maxMaxPending-1 && !control->isPartialSubmitAllowed())
			return;

		// The case where we need to wait for partial submit to be enabled, is never
		// really expected to happen in real life.  So a simple sleep in a loop will
		// do.  It also means we are a long way ahead of the GPU, so even if this
		// does occur, it shouldn't be a performance issue.
		while (Unlikely(!control->isPartialSubmitAllowed()))
			sysIpcSleep(1);

		doPartialSubmitAllocSeg();
	}
}

void grcGfxContext::doPartialSubmit() {
 	grcGfxContextCommandList *const cmdList = m_CmdList;
 	SegmentStart *const cmdListSeg = (SegmentStart*)(cmdList->m_Segs[0]);
 	FatalAssert(cmdList->m_NumSegs >= 1);
 	FatalAssert(cmdListSeg == s_SegHeap.getSeg(cmdList));
 	releaseSegment(cmdListSeg);

	grcContextHandle *const deferredCtx = g_grcCurrentContext;
	const UINT sflags = 0;
	const UINT bufferOffsetInBytes = 0;
	const UINT ceRamOffsetInBytes = 0;
	const UINT sizeInBytes = CERAM_BAK_SIZE_BYTES;
	deferredCtx->StoreConstantRam(sflags, s_ceramBak, bufferOffsetInBytes, ceRamOffsetInBytes, sizeInBytes);

	setDataSegment(NULL);

	m_ForceNewCmdBufSegment = true;
	m_IsFinishCommandList = true;
	const BOOL restoreDeferredContextState = false;
	ID3D11CommandList *d3dCommandList;
	CHECK_HRESULT(deferredCtx->FinishCommandList(restoreDeferredContextState, &d3dCommandList));
	m_IsFinishCommandList = false;
	FatalAssert(!m_ForceNewCmdBufSegment);

	const bool isPartialSubmit = true;
	cmdList->kick(d3dCommandList, isPartialSubmit);

	m_Control->clearPending();

	GRCDEVICE.ForceSetContextState();
	const UINT lflags = 0;
	deferredCtx->LoadConstantRam(lflags, s_ceramBak, bufferOffsetInBytes, ceRamOffsetInBytes, sizeInBytes);
}

void grcGfxContext::doPartialSubmitAllocSeg() {
	doPartialSubmit();

	// doPartialSubmit will have allocated a new command buffer segment, use that as the data segment.
	FatalAssert(!m_Control->getCurrPending());
	SegmentStart *const seg = (SegmentStart*)m_CmdBufSegment;
	setDataSegment(seg);
	createCommandList(seg);
	addSegmentToCommandList(seg);
}

void *grcGfxContext::allocSegment(AllocSegmentAllowPartialSubmit allowPartialSubmit) {
	grcGfxContextControl *const control = m_Control;

	// Wait until segment throttling allows an allocation.
	control->waitUntilAllocOrPartialSubmitAllowed();

	bool partialSubmitBeforeAlloc = false;

	// Note the rare case here where waitUntilAllocOrPartialSubmitAllowed
	// returned because partial submit was allowed, but allocations are at the
	// throttle limit.
	if (Unlikely(!control->isAllocAllowed())) {
		// safeKickPoint should prevent us ever getting here without partial submit being allowed.
		FatalAssert(allowPartialSubmit==ALLOC_SEGMENT_ALLOW_PARTIAL_SUBMIT && control->isPartialSubmitAllowed());
		FatalAssert(control->isPartialSubmitAllowed());
		partialSubmitBeforeAlloc = true;
	}

	const bool deferred = m_Deferred;

#	if ENABLE_PARTIAL_SUBMIT_STRESS_TEST
		// When stress testing the partial submit code, wait until partial submits are allowed.
		if (!control->readStates().getDisablePartialSubmit())
			while (deferred && !control->isPartialSubmitAllowed())
				sysIpcSleep(1);
		partialSubmitBeforeAlloc = true;
#	endif

#	if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
		CustomHangDetection watchdog("grcGfxContext::allocSegment", GRCGFXCONTEXT_DURANGO_GPU_HANG_TIMEOUT_SEC);
#	endif

	// Spin waiting on the GPU until an allocation succeeds.
	bool allocNewCommandList = false;
	SegmentStart *seg = partialSubmitBeforeAlloc ? NULL : s_SegHeap.alloc();
	if (!seg) {
		grcGfxContextProfiler_StallAllocWaitGpu();
		const u32 submitThreshold = getPartialSubmitThreshold();
		do {
			// Do a partial submit if allowed and there is something to submit.
			if (allowPartialSubmit==ALLOC_SEGMENT_ALLOW_PARTIAL_SUBMIT && control->isPartialSubmitAllowed()) {
		 		if (deferred) {
					if (control->getCurrPending()>submitThreshold) {
					 	// TODO: Check there is at least one pending EOP, or MAX_SEGS reached
						doPartialSubmit();
					 	allocNewCommandList = true;

						// Partial submits will allocate a new segment for the command
						// buffer, so we can use that for the data.
						seg = (SegmentStart*)m_CmdBufSegment;
						FatalAssert(seg && seg->inUseCountCmdBuf);
						break;
					}
				}
				else
					g_grcCurrentContext->Flush();
			}

			sysIpcSleep(1);

#			if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
				if (watchdog.pollHasTimedOut())
					reportGpuHang();
#			endif

			seg = s_SegHeap.alloc();
			FatalAssert(!seg || !seg->inUseCountCmdBuf);
		} while (!seg);
	}

	FatalAssert(!seg->inUseFlagGpuData);
	FatalAssert(!seg->inUseCountCpuData);

	if (deferred) {
		control->incPending();
		if (allocNewCommandList) {
			setDataSegment(seg);
			createCommandList(seg);
		}
	}
	else {
		const bool isPartialSubmit = true;
		const bool cacheInvalidateRequired = s_SegHeap.prepareSubmit((SegmentStart**)&seg, 1, isPartialSubmit);
		if (cacheInvalidateRequired)
			s_SegHeap.flushGpuCaches(g_grcCurrentContext);
	}

	grcGfxContextProfiler_AllocSegment();
	return seg;
}

void grcGfxContext::releaseSegment(void *seg, unsigned releaseCount) {
	SegmentStart *const s = (SegmentStart*)seg;
	FatalAssert(s->inUseFlagGpuData);
	s32 inUseCountCpuData = s->inUseCountCpuData;
	FatalAssert((s32)releaseCount >= 0);
	FatalAssert(inUseCountCpuData >= (s32)releaseCount);
	inUseCountCpuData -= releaseCount;
	s->inUseCountCpuData = inUseCountCpuData;

#	if __ASSERT || GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING
		const unsigned segIdx = s_SegHeap.getSegIndex(s);
#	endif
#	if __ASSERT
		// Make sure that none of the remaining ref count is no less than the
		// number of active alloc scopes that are still referencing the segment
		// being released.
		s32 refCount = 0;
		const grcContextAllocScope *allocScope = m_AllocScope;
		FatalAssert(allocScope);
		do {
			refCount += allocScope->m_AllocedSegs.IsSet(segIdx);
		} while ((allocScope = allocScope->parent()) != NULL);
		FatalAssert(refCount <= inUseCountCpuData-(s==m_DataSegment));
#	endif

	if (inUseCountCpuData == 0) {
		grcContextHandle *const d3dCtx = g_grcCurrentContext;
#		if ENABLE_TRASH_DATA_AFTER_RELEASE
			d3dCtx->InsertWaitUntilIdle(0);
			CompileTimeAssert((sizeof(*s)&3) == 0);
			FatalAssert((s_SegHeap.getSegSize()&3) == 0);
			d3dCtx->FillMemoryWithValue(s+1, s_SegHeap.getSegSize()-sizeof(*s), 0xfefefefe);
#		endif
		d3dCtx->CopyMemoryToMemory(const_cast<u32*>(&s->lastCacheInvalidate), s_LastCacheInvalidateGPU, 4);
		d3dCtx->WriteValueBottomOfPipe(const_cast<u32*>(&s->inUseFlagGpuData), 0);
		// TODO: Should keep a count of how many WriteValueBottomOfPipe will
		// free up segments, and use that to determine if a partial submit is
		// worth while.

		// Set inUseCountCpuData to -1 to prevent possible reuse.  See
		// untrackedAllocateTemp for a more detailed explanation.
		s->inUseCountCpuData = -1;

#		if GRCGFXCONTEXT_DURANGO_STANDARD_ERROR_HANDLING
			FatalAssert(m_SegmentsNotYetReleased.IsSet(segIdx));
			m_SegmentsNotYetReleased.Clear(segIdx);
#		endif
	}
}

void grcGfxContext::releaseSegment(unsigned segIdx, unsigned releaseCount) {
	releaseSegment(s_SegHeap.getSeg(segIdx), releaseCount);
}

#if !__FINAL || !__NO_OUTPUT
	/*static*/ void grcGfxContext::reportGpuHang() {
#		if ENABLE_PIX_TAGGING
			if (PIXGetCaptureState() & ~(PIX_CAPTURE_SYSTEM_MONITOR_COUNTERS))
				return;
#		endif
		static volatile u32 reported/*=0*/;
		if (sysInterlockedIncrement(&reported) == 1) {
			OutputDebugString("GPU timeout.  Probable crash or hang.\n");

			// July 2014 XDK (including QFE1) and August preview have a bug in
			// ID3DDeviceX::ReportGpuHang that causes an infinite loop.  If R*TM
			// is attached, then ask it to patch the driver code to fix the bug.
			// If not, then don't call ReportGpuHang.
			//
			// See https://forums.xboxlive.com/AnswerPage.aspx?qid=ce9fa94e-936b-4bf4-8d92-98e55211815a&tgt=1
			// for more details.
			//
			char str[256];
			bool reportGpuHangUnsafe = false;
			OSVERSIONINFOEXW osVersionInfoEx;
			osVersionInfoEx.dwOSVersionInfoSize = sizeof(osVersionInfoEx);
			if (!GetVersionExW((OSVERSIONINFOW*)&osVersionInfoEx)) {
				sprintf(str, "Failed to get OS version info, 0x%08x.  Treating ID3DDeviceX::ReportGpuHang as unsafe and skipping.\n", GetLastError());
				OutputDebugString(str);
				reportGpuHangUnsafe = true;
			}
			else {
				const unsigned buildNumber = osVersionInfoEx.dwBuildNumber;
				const bool buggy
					 = buildNumber == 11274         // July 2014
					|| buildNumber == 11279         // July 2014 QFE1
					|| buildNumber == 11288;        // July 2014 QFE2
				sprintf(str, "OS version %u %s\n", osVersionInfoEx.dwBuildNumber, buggy?"(buggy XDK)":"(safe version)");
				OutputDebugString(str);
				if (buggy) {
#					if SYSTMCMD_ENABLE
						if (PARAM_rockstartargetmanager.Get()) {
							OutputDebugString("R*TM attached, asking it to patch bug.\n");
							sysTmCmdReportGpuHangHack();
						}
						else
#					endif
					if (sysBootManager::IsDebuggerPresent()) {
						OutputDebugString("R*TM not attached, but debugger is attached.  Calling unsafe ID3DDeviceX::ReportGpuHang.\n");
						OutputDebugString("This will likely hang, but instruction pointer can be moved out of the infinite loop.\n");
					}
					else {
						OutputDebugString("R*TM not attached, skipping unsafe ID3DDeviceX::ReportGpuHang.\n");
						reportGpuHangUnsafe = true;
					}
				}
			}

			if (reportGpuHangUnsafe)
				__debugbreak();
			else {
				grcDeviceHandle *const dev = GRCDEVICE.GetCurrent();
				const UINT flags = D3D11X_REPORT_GPU_HANG_DO_NOT_REBOOT;
				dev->ReportGpuHang(flags);
			}
		}
		for (;;) sysIpcSleep(1);
	}
#endif

#if GRCGFXCONTEXT_DURANGO_HANG_CHECKPOINTS
	void grcGfxContext::insertHangCheckpoint() {
		g_grcCurrentContext->WriteValueBottomOfPipe(s_Checkpoint, (m_ContextSequenceNumber<<16) | ((m_CheckpointSquenceNumber)++));
	}
#endif

void grcGfxContext::crossContextFreeTemp(void *ASSERT_ONLY(ptr), u32 sizeBytes) {
	SegmentStart *const seg = (SegmentStart*)m_DataSegment;
	FatalAssert((uptr)seg+seg->offsetFreeData == (uptr)ptr);
	FatalAssert(seg->offsetFreeData >= sizeof(SegmentStart)+sizeBytes);
	seg->offsetFreeData -= sizeBytes;
}

void grcGfxContext::crossContextIncSegmentRefCount(const void *ptr) {
	s_SegHeap.crossContextIncSegmentRefCount(ptr);
}

void grcGfxContext::crossContextDecSegmentRefCount(const void *ptr) {
	SegmentStart *const seg = s_SegHeap.crossContextDecSegmentRefCount(ptr);
	if (seg)
		releaseSegment(seg, 0u);
}

} // namespace rage

#endif // RSG_DURANGO
