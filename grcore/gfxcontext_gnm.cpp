//
// grcore/context_gnm.cpp
//
// Copyright (C) 2013-2015 Rockstar Games.  All Rights Reserved.
//

//
// WARNING:  Avoid Errorf, Warningf, Displayf, etc in any function
//           in here that may be called by the R*TM, since they have
//           a bad tendency to deadlock, which then kills our error
//           reporting for GPU crashes and hangs.
//

#if RSG_ORBIS

#include "gfxcontext_gnm.h"

#include "channel.h"
#include "effect.h"
#include "gfxcontextprofile.h"
#include "system/bootmgr.h"
#include "system/criticalsection.h"
#include "system/customhangdetection.h"
#include "system/exception.h"
#include "system/hangdetect.h"
#include "system/interlocked.h"
#include "system/ipc.h"
#include "system/membarrier.h"
#include "system/memops.h"
#include "system/messagequeue.h"
#include "system/nelem.h"
#include "system/param.h"
#include "system/stack.h"
#include "system/timer.h"
#include "system/tmcommands.h"

#include <kernel.h>
#include <pm4_dump.h>
#include "grcore/gnmx/helpers.h"



// This still needs some more testing before being enabled by default.
#define ENABLE_SINGLE_CUE_HEAP                                      (0)

// Stores the callstack from when the segment was allocated in the SegmentStart
// structure at the start of each DCB.  Doesn't actually require
// GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING, but not really much help without it.
#define ENABLE_SEGMENT_ALLOC_CALLSTACK                              (0 && GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING)

// Debug code to get the draw engine to wait on the end of pipe after each draw.
// Requires GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS.
#define ENABLE_WAIT_AFTER_EVERY_DRAW                                (0 && GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS)

// Debug code to get the GPU to trash the non-command buffer parts of a segment
// (this will contain any pointers that allocateFromCommandBuffer returned) just
// before it marks the segment as no longer in use.  This should cause rendering
// artefacts if anyone is incorrectly still referencing something that has been
// released.  Though this will conflict with post mortem command buffer
// validation, so it cannot be enabled by default.  Requires
// GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING.  We don't trash the actual DCB and CCB
// since debugging a crash would be too difficult then (plus it is very slow).
#define ENABLE_TRASH_DATA_AFTER_RELEASE                             (0 && GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING)

// Debug code that attempts after each draw call, to check that there were no
// GPU page faults.  Uses the per TCC performance counter,
// kTccPerfCounterMcRdretNack.  Unfortunately this doesn't seem to reliably
// catch all faults.  But maybe will help if there is a bug that the command
// buffer validation cannot pickup.  Further info at
// https://ps4.scedev.net/support/issue/16432/.  Requires
// ENABLE_WAIT_AFTER_EVERY_DRAW.
#define ENABLE_PAGE_FAULT_CHECK                                     (0 && ENABLE_WAIT_AFTER_EVERY_DRAW)

// There is a code branch in allocSegment that is rarer than rocking horse shit
// to ever get executed.  To make testing simpler, the following define will
// cause it to be executed a whenever possible as opposed to whenever required.
#define ENABLE_ALLOC_SEGMENT_RARE_CODE_PATH_TEST                    (0)

// Ring buffer size for grcGfxContextProfiler.  Only allocated when
// GRCGFXCONTEXTPROFILE_ENABLE is turned on.
#define PROFILER_BUFFER_SIZE_BYTES                                  (0x00100000)


namespace rage {

#define PM4_TYPE0_HEADER(BASE_INDEX, PACKET_SIZE_DWORDS)        (((((PACKET_SIZE_DWORDS)-2)<<16)&0x3fff0000)|((BASE_INDEX)&0x0000ffff))
#define PM4_TYPE2_HEADER()                                      (0x80000000)
#define PM4_TYPE3_HEADER(OPCODE, PACKET_SIZE_DWORDS)            (0xc0000000|((((PACKET_SIZE_DWORDS)-2)<<16)&0x3fff0000)|(((PM4_TYPE3_OPCODE_##OPCODE)<<8)&0x0000ff00))

#define PM4_TYPE3_OPCODE_NOP            0x10
#define PM4_TYPE3_OPCODE_REWIND         0x59
#define PM4_TYPE3_OPCODE_WRITE_DATA     0x37

PARAM(validateGfx, "[grcore] validate the submit calls (really slow)");
XPARAM(gpuDebugger);

u32 grcGfxContext::sm_SegmentCount;
__THREAD grcGfxContext *grcGfxContext::sm_Current;
static grcGfxContext *s_Contexts;
static u32 s_ContextCount;
static u32 s_DcbSizeBytes;
static u32 s_CcbSizeBytes;
static grcGfxContextControl s_DefaultControl;

#if GRCGFXCONTEXT_GNM_USES_CCB
	u32 grcGfxContext::sm_MaxMaxPending = ~0;
#endif

#if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
	static volatile s64 *s_EndSubmitTimestamp;
#endif

#if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
	// It may look like s_CeAddr is never actually read by the code and
	// therefore not useful, but please do not remove.  This can be useful for
	// debugging GPU errors as it tells you how far the CE has executed ahead of
	// the ME.
	static volatile u64 *s_CeAddr;
#endif

#if ENABLE_PAGE_FAULT_CHECK
	static u64 *s_PerfCounterValues;
#endif

// Used for keeping track of when segments are last accessed by the GPU.
// Segments are considerred "last accessed" when they are released.  Because the
// command buffers are generated in parallel, we can't know the sequence number
// of the last cache flush.  So instead, the command buffers copy the value from
// s_LastCacheInvalidateGPU into the segment header.  s_LastCacheInvalidateCPU
// is the CPU copy.
static u32  s_LastCacheInvalidateCPU;
static u32 *s_LastCacheInvalidateGPU;

// recordGpuIdleEpilogue requires a EVENT_WRITE_EOP (6 dwords), a WAIT_REG_MEM
// (7 dwords) and a DMA_DATA (7 dwords).  Generally we don't care if
// recordGpuIdleEpilogue ends up creating a new DCB/CCB pair, but in the case of
// doing a partial submit from the handleBufferFull callback, we need to patch
// this into the current DCB.
enum { DCB_RESERVED_NUM_DWORDS_RECORD_GPU_IDLE_EPILOGUE = GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES ? 20 : 0 };

// Leave space at the end of the DCB for a possible DMA_DATA (7 dwords) and
// EVENT_WRITE_EOP (6 dwords) to be written by releaseSegment.
#if !ENABLE_TRASH_DATA_AFTER_RELEASE
	enum { DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT = 13 };
#else
	// ENABLE_TRASH_DATA_AFTER_RELEASE also requires an additional NOP (3
	// dwords), a WAIT_REG_MEM (7 dwords), and two (three if CUE) DMA_DATAs (7
	// dwords each)
	enum { DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT = GRCGFXCONTEXT_GNM_USES_CCB ? 44 : 37 };
#endif

// And finally, if we are using a CCB, then we need a WAIT_ON_CE_COUNTER (2
// dwords) and a INCREMENT_DE_COUNTER (2 dwords) to synchronize everything.
#if GRCGFXCONTEXT_GNM_USES_CCB
	enum { DCB_RESERVED_NUM_DWORDS_CE_DE_SYNC = 4 };
#else
	enum { DCB_RESERVED_NUM_DWORDS_CE_DE_SYNC = 0 };
#endif

enum { DCB_RESERVED_NUM_DWORDS = DCB_RESERVED_NUM_DWORDS_RECORD_GPU_IDLE_EPILOGUE + DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT + DCB_RESERVED_NUM_DWORDS_CE_DE_SYNC };

#if GRCGFXCONTEXT_GNM_USES_CCB
	// Leave space at the end of the CCB for a WAIT_ON_DE_COUNTER_DIFF followed
	// by a INCREMENT_CE_COUNTER to be appended to the final CCB passed to
	// prepareSubmit.
	enum { CCB_RESERVED_NUM_DWORDS = 4 };
#endif

enum {
	// Number of dwords to reserve for the command to invalidate the volatile
	// GPU L2 cache lines if we recycle a command buffer segment during the same
	// frame that it was previously submitted.
	INVALIDATE_L2_VOLATILE_NUM_DWORDS   = 21,

	// NOP type 3 packet for the reserved space.
	NOP_FOR_INVALIDATE_L2_VOLATILE      = PM4_TYPE3_HEADER(NOP, INVALIDATE_L2_VOLATILE_NUM_DWORDS),
};

static sysCriticalSectionToken s_ContextLock;

#if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
	static volatile bool s_AsyncFault;
#endif

// Use pragmapack(4) so we don't add an extra dword of padding at the end of the
// structure.  Structure should still always be aligned to at least 8-bytes in
// memory.
#pragma pack(push)
#pragma pack(4)
struct SegmentStart {
	u32             itNop;
	u32             signature;
	SegmentStart   *prevSubmitted;
	SegmentStart   *nextSubmitted;
	volatile u32    inUseFlagGpu;
	u32             inUseCountCpu;
#	if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
		s64             endLastSubmitTimestamp;
		s64             beginThisSubmitTimestamp;
#	endif
	volatile u32    lastCacheInvalidate;
	u32             frameAndSubmitOrder;    // [31:24] frame, [23:0] submit counter
#	if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		u32             dcbSizeBytes;
		u32             ccbSizeBytes;
#	endif
#	if ENABLE_SEGMENT_ALLOC_CALLSTACK
		size_t          allocStack[16];
#	endif
};
#pragma pack(pop)

enum { SegmentStart_NOP         = PM4_TYPE3_HEADER(NOP, sizeof(SegmentStart)/4) };
enum { SegmentStart_SIGNATURE   = ' GES' };

// SegmentStart::inUseFlagGpu is written to using a writeAtEndOfPipe (in
// grcContextAllocScope::releaseAll), so it must be 8-byte aligned.
CompileTimeAssert((offsetof(SegmentStart, inUseFlagGpu)&7) == 0);

#if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
	// endLastSubmitTimestamp and beginThisSubmitTimestamp need to be 64-bit
	// aligned for GPU DMA.
	CompileTimeAssert((offsetof(SegmentStart, endLastSubmitTimestamp)   &3) == 0);
	CompileTimeAssert((offsetof(SegmentStart, beginThisSubmitTimestamp) &3) == 0);
#endif


#if GRCGFXCONTEXT_GNM_CONTEXT_STACK
	// Debug information stored in the command buffer for the lifetime of the
	// context.
	struct grcGfxContextDebugState {
		grcGfxContextDebugState *volatile   m_ParentCtxDbgState;
		volatile char                       m_FaultStack[8][64];
	};
	static grcGfxContextDebugState*volatile *s_LastCtxDbgState;
#endif


// Controls allocation of command buffer segments.  Each segment contains a
// DCB/CCB pair when using CUE, and is just a DCB with LCUE.  The start of each
// segment is a SegmentStart structure that can be executed by the GPU as a NOP.
//
// A doubly linked list is maintend of segments in the order they are submitted
// to the GPU.  Each segment has a flag SegmentStart.inUseFlagGpu.  Once the GPU
// writes this back to zero, the segment can be reallocated.  Allocation will
// return the oldest submitted segment that has inUseFlagGpu cleared.
// Allocating the oldest first means that command buffers submitted to the GPU
// hang around as long as possible afterwards.  Since some GPU errors (such as
// access violations) don't actually halt the GPU at the location of the
// problem, keeping the command buffers as long as possible is required for
// debugging.  The Sony command buffer validation code can be run post GPU error
// on all command buffers that have not yet been overwritten by the CPU.
//
class SegmentHeap {
public:

#	if !GRCGFXCONTEXT_GNM_STANDARD_ERROR_HANDLING
		bool invariant() const {
			return true;
		}

		void validateSubmitted() const {}
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

		void validateSubmitted() const {
			// Run all the submitted DCB/CCB pairs through the Sony command
			// buffer validation code.  When we still have command buffers that
			// were submitted sequentially, validate them together so that state
			// carried from one to the next is correctly accounted for.
			SYS_CS_SYNC(m_Lock);
			FatalAssert(invariant());
			printf("\n");
			const SegmentStart *seg = m_OldestSubmitted;
			const SegmentStart *const end = submittedListSentinal();
			if (seg == end)
				return;

			const u32 **const dcbPtrs = Alloca(const u32*, m_NumSegs);
			const u32 **const ccbPtrs = Alloca(const u32*, m_NumSegs);
			u32 *const dcbSizesInBytes = Alloca(u32, m_NumSegs);
			u32 *const ccbSizesInBytes = Alloca(u32, m_NumSegs);
			bool done = false;
			do {
				u32 num = 0;
				u32 lastFrame;
				u32 lastSubmitOrder;
				for (;;) {
					const u32 *const dcbPtr = (u32*)seg;
					const u32 *const ccbPtr = (u32*)((uptr)seg+m_MaxDcbSizeBytes);
					const u32 dcbSizeBytes = seg->dcbSizeBytes;
					const u32 ccbSizeBytes = seg->ccbSizeBytes;
					dcbPtrs[num] = dcbPtr;
					ccbPtrs[num] = ccbPtr;
					dcbSizesInBytes[num] = dcbSizeBytes;
					ccbSizesInBytes[num] = ccbSizeBytes;
					++num;
					lastFrame       = seg->frameAndSubmitOrder >> 24;
					lastSubmitOrder = seg->frameAndSubmitOrder & 0x00ffffff;

					seg = seg->nextSubmitted;
					if (seg == end) {
						done = true;
						break;
					}
					if (((lastSubmitOrder+1)&0x00ffffff) != (seg->frameAndSubmitOrder&0x00ffffff)
					 || lastFrame                        != (seg->frameAndSubmitOrder>>24))
						break;
				}

				const u32 firstSubmitOrder = lastSubmitOrder+1-num;
				for (u32 i=0; i<num; ++i) {
					printf("Validating DCB 0x%010lx L0x%06x, CCB 0x%010lx L0x%06x pair (frame 0x%02x, submit 0x%06x)\n",
						(uptr)dcbPtrs[i], dcbSizesInBytes[i], (uptr)ccbPtrs[i], ccbSizesInBytes[i], lastFrame, (firstSubmitOrder+i)&0x00ffffff);
				}
				const int errCode = sce::Gnm::validateCommandBuffers(num, (void**)dcbPtrs, dcbSizesInBytes, (void**)ccbPtrs, ccbSizesInBytes);
				if (errCode)
					printf("    error code 0x%08x\n\n", errCode);
				else
					printf("    ok\n\n");
			} while (!done);
		}

#	endif // GRCGFXCONTEXT_GNM_STANDARD_ERROR_HANDLING

	void incFrame() {
		++m_Frame;
#		if GRCGFXCONTEXT_GNM_STANDARD_ERROR_HANDLING
			for (unsigned i=0; i<m_NumSegs; ++i)
				FatalAssert(!m_CrossContextAllocs[i]);
#		endif
	}

	void init(void *start, u32 count, u32 dcbSizeBytes, u32 ccbSizeBytes) {
		// Initialize each segment as not in use, and link them into the
		// submitted list.
		const u32 segSizeBytes = dcbSizeBytes+ccbSizeBytes;
		m_Base = start;
		m_SegSizeBytes = segSizeBytes;
		SegmentStart *next = (SegmentStart*)start;
		SegmentStart *end  = (SegmentStart*)((char*)next+count*segSizeBytes);
		SegmentStart *prev = submittedListSentinal();
		m_OldestSubmitted = next;
		m_SubmitOrder               = -count;
		m_Frame                     = 0;
#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
			m_NumSegs                   = count;
			m_NumPending                = 0;
			m_MaxDcbSizeBytes           = dcbSizeBytes;
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
			curr->itNop                     = SegmentStart_NOP;
			curr->signature                 = SegmentStart_SIGNATURE;
			curr->inUseCountCpu             = 0;
			curr->inUseFlagGpu              = 0;
			curr->prevSubmitted             = prev;
			curr->nextSubmitted             = next ? next : submittedListSentinal();
#			if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
				curr->endLastSubmitTimestamp    = 0;
				curr->beginThisSubmitTimestamp  = 0;
#			endif
			curr->lastCacheInvalidate       = -1;
			curr->frameAndSubmitOrder       = ++m_SubmitOrder & 0x00ffffff;
#			if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
				curr->dcbSizeBytes              = 0;
				curr->ccbSizeBytes              = 0;
#			endif
			prev = curr;
		} while (next);
		m_LastSubmitted = prev;
		FatalAssert(invariant());
	}

	SegmentStart *alloc() {
#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
			if (Unlikely(s_AsyncFault))
				for (;;)
					sysIpcSleep(1);
#		endif

		// Loop through the list of already submitted segments, oldest to
		// newest, and find the first one that is safe to reuse.
		SYS_CS_SYNC(m_Lock);
		SegmentStart *seg = m_OldestSubmitted;
		SegmentStart *end = submittedListSentinal();
		SegmentStart *prev = end;
		while (seg != end) {
			SegmentStart *const next = seg->nextSubmitted;
			if (!seg->inUseFlagGpu) {
				FatalAssert(!seg->inUseCountCpu);
				prev->nextSubmitted = next;
				next->prevSubmitted = prev;
				seg->prevSubmitted  = NULL;
				seg->nextSubmitted  = NULL;
				seg->inUseCountCpu  = 1;
				seg->inUseFlagGpu   = 1;
#				if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
					seg->endLastSubmitTimestamp   = 0;
					seg->beginThisSubmitTimestamp = 0;
#				endif
				// Do not clear lastCacheInvalidate here, prepareSubmit needs
				// the old value to determine if the segment is being used
				// within the same frame (and .: gpu cache invalidations are
				// required).
				seg->frameAndSubmitOrder = 0;
#				if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
					seg->dcbSizeBytes        = 0;
					seg->ccbSizeBytes        = 0;
					++m_NumPending;
					FatalAssert(invariant());
#				endif
				return seg;
			}
#		  if __ASSERT
			else
				// If this assert fires, then we have leaked a segment.
				//
				// Check if you have the old Razor GPU enabled in the target
				// settings.  There is a bug in the old version of Razor that
				// will cause a segment to be leaked due to
				// sce::Gnm::DrawCommandBuffer::prepareFlip(void*,uint32_t) not
				// actually ever writing to the specified label (see
				// https://ps4.scedev.net/forums/thread/31813/).  The Razor GPU
				// setting must be either "No" or "New RazorGPU".  If the
				// setting was not "Yes" (ie, old RazorGPU), then this is a
				// serious bug, please report it.
				//
				// Sadly we need to check against
				// ms_MaxQueuedFrames + 1 rather than just
				// ms_MaxQueuedFrames here.  The only case
				// where the difference can validly be
				// ms_MaxQueuedFrames + 1 is the initial
				// allocation when the render thread context is re-openned in
				// grcDevice::EndFrame.  The reason for this is that the render
				// thread hasn't yet waited for the gpu to catch enough up in
				// the next grcDevice::BeginFrame call.
				//
				FatalAssert(((m_Frame-(seg->frameAndSubmitOrder>>24))&0xff) <= (MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU+1));
#		  endif
			prev = seg;
			seg = next;
		}
		return NULL;
	}

	enum SubmitType {
		SUBMIT_PARTIAL,
		SUBMIT_FULL,
		SUBMIT_FLIP,
	};

	void prepareSubmit(SegmentStart **segs, u32 *dcbSizesInBytes, void **ccbs, u32 *ccbSizesInBytes, u32 num, SubmitType submitType) {
		FatalAssert(num);

#		if GRCGFXCONTEXT_GNM_USES_CCB
			// prepareFlip must be the last thing in the command buffers, so
			// don't add anything after it if we are doing a flip.
			if (submitType != SUBMIT_FLIP) {
				// The CE/DE counters appear to be reset to zero between
				// submits.  This causes synchronization problems between the ME
				// and CE.  The actual timing of the reset seems to be when the
				// CE starts the new command buffer.  But really don't know.
				// Asking at https://ps4.scedev.net/forums/thread/41319/,
				// hopefully we will get some real details rather than just
				// guessing.
				sce::Gnm::DrawCommandBuffer epilogueDcb;
				sce::Gnm::ConstantCommandBuffer epilogueCcb;
				epilogueDcb.init((char*)segs[num-1]+dcbSizesInBytes[num-1], DCB_RESERVED_NUM_DWORDS_CE_DE_SYNC*4, NULL, NULL);
				epilogueCcb.init((char*)ccbs[num-1]+ccbSizesInBytes[num-1], CCB_RESERVED_NUM_DWORDS*4, NULL, NULL);

				// Wait for the ME to catch up to the CE.
				epilogueCcb.waitOnDeCounterDiff(1);

				// Ensure the ME does not get ahead of the CE.
				epilogueCcb.incrementCeCounter();
				epilogueDcb.waitOnCe();
				epilogueDcb.incrementDeCounter();

				FatalAssert(epilogueDcb.m_cmdptr == epilogueDcb.m_endptr);
				dcbSizesInBytes[num-1] += DCB_RESERVED_NUM_DWORDS_CE_DE_SYNC*4;
#				if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
					segs[num-1]->dcbSizeBytes += DCB_RESERVED_NUM_DWORDS_CE_DE_SYNC*4;
#				endif

				FatalAssert(epilogueCcb.m_cmdptr == epilogueCcb.m_endptr);
				ccbSizesInBytes[num-1] += CCB_RESERVED_NUM_DWORDS*4;
#				if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
					segs[num-1]->ccbSizeBytes += CCB_RESERVED_NUM_DWORDS*4;
#				endif
			}
#		endif

#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
#			if !__FINAL && !__NO_OUTPUT
				while (Unlikely(sysException::HasBeenThrown())) {
					sysIpcSleep(GRCGFXCONTEXT_GNM_GPU_HANG_TIMEOUT_SEC);
					// While s_AsyncFault can only be set by R*TM in "normal"
					// program flow.  It can be manually set in the debugger when a
					// GPU fault occurs
					// (https://devstar.rockstargames.com/wiki/index.php/Debugging_Southern_Islands_Crashes),
					// so make sure to call reportGpuHang() rather than just
					// assuming that R*TM is running and calling sysTmCmdGpuHang()
					// directly.
					if (s_AsyncFault)
						grcGfxContext::reportGpuHang();
				}
#			endif
#		endif

		SYS_CS_SYNC(m_Lock);

		// Link segments into the submitted list
		u32 lastCacheInvalidate = s_LastCacheInvalidateCPU;
		SegmentStart *prevSubmitted = m_LastSubmitted;
		for (u32 i=0; i<num; ++i) {
			SegmentStart *const seg = segs[i];
			FatalAssert(seg->itNop     == SegmentStart_NOP);
			FatalAssert(seg->signature == SegmentStart_SIGNATURE);
			prevSubmitted->nextSubmitted = seg;
			seg->prevSubmitted = prevSubmitted;
			prevSubmitted = seg;
			seg->frameAndSubmitOrder = (m_Frame<<24) | (++m_SubmitOrder&0x00ffffff);

			// If this segment was previously accessed by the GPU after the last
			// invalidation of volatile L2 cache lines, need to invalidate them
			// again.  Otherwise stale values may be still in the L2.
			FatalAssert((s32)(seg->lastCacheInvalidate - lastCacheInvalidate) <= 0);
			if (seg->lastCacheInvalidate == lastCacheInvalidate) {
				// Overwrite the NOP that is immediately after the SegmentStart
				// structure (see assignSegmentToContext).
				u32 *const inv = (u32*)(seg+1);
				FatalAssert(*inv == NOP_FOR_INVALIDATE_L2_VOLATILE);
				sce::Gnm::DrawCommandBuffer dcb;
				dcb.init(inv, INVALIDATE_L2_VOLATILE_NUM_DWORDS*4, NULL, NULL);
				dcb.flushShaderCachesAndWait(
					sce::Gnm::kCacheActionInvalidateL2Volatile,
					0,
					sce::Gnm::kStallCommandBufferParserDisable);
				dcb.flushShaderCachesAndWait(
					sce::Gnm::kCacheActionInvalidateL1,
					sce::Gnm::kExtendedCacheActionInvalidateKCache,
					sce::Gnm::kStallCommandBufferParserEnable);
				dcb.dmaData(
					sce::Gnm::kDmaDataDstMemory, (u64)s_LastCacheInvalidateGPU,
					sce::Gnm::kDmaDataSrcData, lastCacheInvalidate,
					4, sce::Gnm::kDmaDataBlockingEnable);
				FatalAssert(dcb.m_cmdptr == dcb.m_endptr);
				++lastCacheInvalidate;
			}

#			if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
				FatalAssert(seg->dcbSizeBytes == dcbSizesInBytes[i]);
				FatalAssert(seg->ccbSizeBytes == ccbSizesInBytes[i]);
#			endif
			FatalAssert(submitType==SUBMIT_PARTIAL || !seg->inUseCountCpu || m_CrossContextAllocs[getSegIndex(seg)]);
		}
		prevSubmitted->nextSubmitted = submittedListSentinal();
		m_LastSubmitted = prevSubmitted;
		s_LastCacheInvalidateCPU = lastCacheInvalidate;

#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
			m_NumPending -= num;
			FatalAssert(invariant());
#		endif
	}

	unsigned getSegIndex(const void *ptr) const {
		return ((uptr)ptr - (uptr)m_Base) / m_SegSizeBytes;
	}

	SegmentStart *getSeg(unsigned idx) const {
		return (SegmentStart*)((char*)m_Base + idx*m_SegSizeBytes);
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

	void crossContextIncSegmentRefCount(const void *ptr) {
		SegmentStart *const seg = const_cast<SegmentStart*>(getSeg(ptr));
		++(seg->inUseCountCpu);
#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
			const unsigned idx = getSegIndex(seg);
			SYS_CS_SYNC(m_Lock);
			++(m_CrossContextAllocs[idx]);
			FatalAssert(m_CrossContextAllocs[idx]);
#		endif
	}

	SegmentStart *crossContextDecSegmentRefCount(const void *ptr) {
		SegmentStart *const seg = const_cast<SegmentStart*>(getSeg(ptr));
		--(seg->inUseCountCpu);
#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
			const unsigned idx = getSegIndex(seg);
			SYS_CS_SYNC(m_Lock);
			FatalAssert(m_CrossContextAllocs[idx]);
			--(m_CrossContextAllocs[idx]);
#		endif
		return seg->inUseCountCpu ? NULL : seg;
	}

private:

	mutable sysCriticalSectionToken m_Lock;

	void         *m_Base;
	u32           m_SegSizeBytes;

	// These two pointer must be kept in the same order as
	// SegmentStart::prevSubmitted, SegmentStart::nextSubmitted.
	SegmentStart *m_LastSubmitted;
	SegmentStart *m_OldestSubmitted;

	u32  m_SubmitOrder;
	u32  m_Frame;

#	if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		u32 m_NumSegs;
		u32 m_NumPending;
		u32 m_MaxDcbSizeBytes;
		u32 *m_CrossContextAllocs;
#	endif
};

static SegmentHeap s_SegHeap;


// sce::Gnm::submitCommandBuffers, submitAndFlipCommandBuffers and submitDone
// can stall for arbitrarily long periods of time.  To isolate these stalls from
// holding up the primary or sub render threads, we have a worker thread for
// making the calls.  The render threads simply push requests into the queue.
// When we add support for asynchronous compute rings, then the dingDong call
// should also be handled like this.

enum { MAX_ENQUEUED_SUBMITS = 16 };
struct GpuSubmitCmdBufs {
	u32   m_Count;
	void *m_DcbStarts      [grcGfxContext::MaxPending];
	u32   m_DcbSizesInBytes[grcGfxContext::MaxPending];
	void *m_CcbStarts      [grcGfxContext::MaxPending];
	u32   m_CcbSizesInBytes[grcGfxContext::MaxPending];

	// Flip related
	s32   m_VideoOutHandle;     // negative means don't flip
	u32   m_RtIndex;
	u32   m_FlipMode;
	s64   m_FlipArg;
};
static GpuSubmitCmdBufs s_GpuSubmitCmdBufs[MAX_ENQUEUED_SUBMITS];
typedef sysMessageQueue<GpuSubmitCmdBufs*, MAX_ENQUEUED_SUBMITS, true> GpuSubmitCmdBufsQueue;
static GpuSubmitCmdBufsQueue s_GpuSubmitCmdBufsPendingQueue;
static GpuSubmitCmdBufsQueue s_GpuSubmitCmdBufsFreeQueue;

/*static*/ void grcGfxContext::gpuSubmitThread(void*) {
	for (;;) {
		GpuSubmitCmdBufs *const submit = s_GpuSubmitCmdBufsPendingQueue.Pop();
		const u32 count = submit->m_Count;
		grcGfxContextProfiler_KickBegin(count);
		const s32 videoOutHandle = submit->m_VideoOutHandle;
		if (videoOutHandle < 0) {
			ASSERT_ONLY(int ret=) sce::Gnm::submitCommandBuffers(
				count, submit->m_DcbStarts, submit->m_DcbSizesInBytes, submit->m_CcbStarts, submit->m_CcbSizesInBytes);
			FatalAssert(ret == SCE_OK);
		}
		else {
			ASSERT_ONLY(int ret=) sce::Gnm::submitAndFlipCommandBuffers(
				count, submit->m_DcbStarts, submit->m_DcbSizesInBytes, submit->m_CcbStarts, submit->m_CcbSizesInBytes,
				videoOutHandle, submit->m_RtIndex, submit->m_FlipMode, submit->m_FlipArg);
			FatalAssert(ret == SCE_OK);
			ASSERT_ONLY(ret =) sce::Gnm::submitDone();
			FatalAssert(ret == SCE_OK);
		}
		s_GpuSubmitCmdBufsFreeQueue.Push(submit);
		grcGfxContextProfiler_KickEnd();
	}
}

/*static*/ void grcGfxContext::initRing(u32 contextCount, u32 cueRingCount, u32 cbRingCount, u32 dcbSizeBytes, u32 ccbSizeBytes) {
	// Compile time check that SCE_GNM_VALIDATE macro is correctly enabled or disabled
	CompileTimeAssert(!__ASSERT == !SCE_GNM_ENABLE_VALIDATION);
#	if __ASSERT
		// If SCE_GNM_VALIDATE is disabled, this should generate an unused variable warning.
		int ERROR_SCE_GNM_VALIDATE_macro_is_not_enabled_when_it_should_be = 1;
		SCE_GNM_VALIDATE(ERROR_SCE_GNM_VALIDATE_macro_is_not_enabled_when_it_should_be, "");
#	else
		// Not sure how to mirror of that and check the macro is disabled when it should be though.
#	endif

#	if GRCGFXCONTEXT_GNM_USES_CCB
		const u32 cueHeapSizeBytes = sce::Gnmx::ConstantUpdateEngine::computeHeapSize(cueRingCount);
		const u32 cueCpRamShadowSizeBytes = sce::Gnmx::ConstantUpdateEngine::computeCpRamShadowSize();
#	endif

#	if GRCGFXCONTEXT_GNM_USES_CCB
		s_DcbSizeBytes = dcbSizeBytes;
		s_CcbSizeBytes = ccbSizeBytes;
#	else
		s_DcbSizeBytes = dcbSizeBytes + ccbSizeBytes;
		s_CcbSizeBytes = 0;
#	endif

	s_Contexts = rage_new grcGfxContext[contextCount];
	s_ContextCount = contextCount;
	s_DefaultControl.m_Atomic.m_Val |= grcGfxContextControl::States::ALLOW_PARTIAL_SUBMIT_MASK | grcGfxContextControl::States::MAX_PENDING_MASK;

	s_LastCacheInvalidateGPU = (u32*)allocateVideoPrivateMemory(4, 4);
	*s_LastCacheInvalidateGPU = 0;

#	if GRCGFXCONTEXT_GNM_USES_CCB
		// The ram shadow could be discarded as soon as we finish adding render commands to the context.
		// These are about 49k each though.
		char *cueCpRamShadow = rage_new char[cueCpRamShadowSizeBytes * contextCount];

		// We allocate with stricter alignment than is necessary so it's easier to read hex dumps.
		// Alignment needs to be at least 16 (guaranteed by our memeory manager for large allocations) in order to guarantee
		// our custom allocateFromCommandBuffer returns suitably-aligned memory as well.
#		if MULTIPLE_RENDER_THREADS && !ENABLE_SINGLE_CUE_HEAP
			char *cue = (char*)allocateVideoPrivateMemory(cueHeapSizeBytes * cbRingCount, 4096 /*sce::Gnm::kAlignmentOfBufferInBytes*/);
#		else
			char *cue = (char*)allocateVideoPrivateMemory(cueHeapSizeBytes, 4096 /*sce::Gnm::kAlignmentOfBufferInBytes*/);
#		endif
#	endif

	// Allocate command buffer segments from Onion/WB.  Should not use Garlic/WC
	// because the CPU needs to read from these structures (eg, submitted
	// segment linked list management).  Must not use Garlic/WB because the GPU
	// writes into the segments.
	FatalAssert(cbRingCount <= GRCCONTEXT_MAX_NUM_SEGMENTS);
	sm_SegmentCount = cbRingCount;
	const u32 segSizeBytes = dcbSizeBytes + ccbSizeBytes;
	s_SegHeap.init(allocateVideoSharedMemory(cbRingCount*segSizeBytes, 4096), cbRingCount, dcbSizeBytes, ccbSizeBytes);

	for (u32 i=0; i<contextCount; i++) {
		grcGfxContext *const ctx = s_Contexts+i;

#		if ENABLE_LCUE
			// u32 *resourceBufferInGarlic = (u32*) allocateVideoPrivateMemory(sce::LCUE::kMinResourceBufferSizeInDwords*4,16);
			ctx->GraphicsCUE::init(
				NULL, 0, 0,
				// &resourceBufferInGarlic, 1, sce::LCUE::kMinResourceBufferSizeInDwords,
				(u32*)allocateVideoPrivateMemory(sce::LCUE::kGlobalInternalTableSizeInDwords*4,16), sce::LCUE::kGlobalInternalTableSizeInDwords);
			ctx->GraphicsCUE::setDcb(&ctx->m_dcb);
#		endif

		ctx->m_dcb.init(NULL, 0, handleBufferFullStatic, s_Contexts+i);
#		if GRCGFXCONTEXT_GNM_USES_CCB
			ctx->m_ccb.init(NULL, 0, handleBufferFullStatic, s_Contexts+i);
#			if MULTIPLE_RENDER_THREADS && !ENABLE_SINGLE_CUE_HEAP
				ctx->m_cue.init(cueCpRamShadow + cueCpRamShadowSizeBytes * i, cue + cueHeapSizeBytes * i, cueRingCount);
#			else
				ctx->m_cue.init(cueCpRamShadow + cueCpRamShadowSizeBytes * i, cue, cueRingCount);
#			endif
#		endif
#		if GRCGFXCONTEXT_GNM_USES_ACB
			ctx->m_acb.init(NULL, 0, handleBufferFullStatic, s_Contexts+i);
#		endif
#		if !ENABLE_LCUE && defined(SCE_GNMX_ENABLE_CUE_V2)
			ctx->m_cue.bindCommandBuffers(&ctx->m_dcb, &ctx->m_ccb
#			if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
				,&ctx->m_acb
#			endif
				);
#		endif

		ctx->m_Deferred = false;
		ctx->m_PendingCount = 0;
		ctx->m_AllocScope = &ctx->m_BaseAllocScope;
		ctx->m_Status = STATUS_CLOSED_AND_KICKED;
		ctx->m_VsWaveLimitCurrent = 0;
		ctx->m_VsWaveLimitLast = 0;

#		if GRCGFXCONTEXT_GNM_USES_CCB
#			if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				ctx->m_recordLastCompletionMode = kRecordLastCompletionDisabled;
				ctx->m_addressOfOffsetOfLastCompletion = 0;
#			endif
#			if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
				ctx->m_dispatchDrawIndexDeallocMask = 0;
				ctx->m_dispatchDrawNumInstancesMinus1 = 0;
				ctx->m_dispatchDrawFlags = 0;
#			else
				ctx->m_dispatchDrawState = 255 << kDispatchDrawStateShiftPrimGroupSize;
#			endif
#		endif

#		if GRCGFXCONTEXT_GNM_USES_CCB
			ctx->m_InDrawOrDispatch = false;
#		endif
#		if GRCGFXCONTEXT_GNM_CONTEXT_STACK
			ctx->m_DbgState = NULL;
			ctx->m_FaultStackSP = 0;
#		endif
	}

#	if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
		s_EndSubmitTimestamp = (s64*)allocateVideoPrivateMemory(sizeof(s64), __alignof(s64));
		*s_EndSubmitTimestamp = 0;
#	endif

#	if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		s_CeAddr = (u64*)allocateVideoSharedMemory(sizeof(void*), __alignof(void*));
		*s_CeAddr = 0;
#	endif

#	if GRCGFXCONTEXT_GNM_CONTEXT_STACK
		s_LastCtxDbgState = (grcGfxContextDebugState*volatile*)allocateVideoSharedMemory(sizeof(void*), __alignof(void*));
		*s_LastCtxDbgState = NULL;
#	endif

#	if ENABLE_PAGE_FAULT_CHECK
		FatalAssert(sce::Gnm::isUserPaEnabled());
		s_PerfCounterValues = (u64*)allocateVideoSharedMemory(8*sizeof(u64), __alignof(u64));
		sysMemSet(s_PerfCounterValues, 0x55, 8*sizeof(u64));
		sce::Gnm::setSpiEnableSqCounters(sce::Gnm::kBroadcastAll, true);
#	endif

	grcGfxContextControl::initClass();

#	if EXCEPTION_HANDLING && GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		sysException::GpuExceptionCallback = [](){OrbisGpuCrashCallback(1,0,0);};
#	endif

#	if GRCGFXCONTEXTPROFILE_ENABLE
		gGrcGfxContextProfiler.init(rage_aligned_new(16) char[PROFILER_BUFFER_SIZE_BYTES], PROFILER_BUFFER_SIZE_BYTES);
#	endif

	for (unsigned i=0; i<NELEM(s_GpuSubmitCmdBufs); ++i)
		s_GpuSubmitCmdBufsFreeQueue.Push(s_GpuSubmitCmdBufs+i);
	sysIpcCreateThread(grcGfxContext::gpuSubmitThread, NULL, sysIpcMinThreadStackSize, PRIO_ABOVE_NORMAL, __NO_OUTPUT ? "" : "[RAGE] GPU Submit", 3, "RageGpuSubmit");
}

char *grcGfxContext::allocSegment() {
	grcGfxContextControl *const control = m_Control;

	// Wait until segment throttling allows an allocation.
	control->waitUntilAllocOrPartialSubmitAllowed();

	// Note the rare case here where waitUntilAllocOrPartialSubmitAllowed
	// returned because partial submit was allowed, but allocations are at the
	// throttle limit.
#	if ENABLE_ALLOC_SEGMENT_RARE_CODE_PATH_TEST
		const bool rareCodePathTest = m_PendingCount && !control->readStates().getDisablePartialSubmit()
#			if GRCGFXCONTEXT_GNM_USES_CCB
                && !m_InDrawOrDispatch
#			endif
			;
		if (rareCodePathTest)
			while (!control->isPartialSubmitAllowed())
				sysIpcSleep(1);
#	else
		enum { rareCodePathTest = false };
#	endif
	if (Unlikely(!control->isAllocAllowed() || rareCodePathTest)) {
		FatalAssert(control->isPartialSubmitAllowed());
#		if GRCGFXCONTEXT_GNM_USES_CCB
			FatalAssert(!m_InDrawOrDispatch);   // safeKickPoint should have ensured we do not reach here with m_InDrawOrDispatch set
#		endif

		// Unfortunately we can't measure GPU idle time here.  The last dcb may
		// already be releasing itself, so we can't safely add any additional
		// commands to it.  But this code path is so rare, that it doesn't
		// really make any difference to profile timings.

		control->clearPending();
		submitPending(SUBMIT_PARTIAL);
	}

#	if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		CustomHangDetection watchdog("grcGfxContext::allocSegment", GRCGFXCONTEXT_GNM_GPU_HANG_TIMEOUT_SEC);
#	endif

	// Spin waiting on the GPU until an allocation succeeds.
	char *seg = (char*)s_SegHeap.alloc();
	if (Unlikely(!seg)) {
		grcGfxContextProfiler_StallAllocWaitGpu();
		do {
			sceKernelUsleep(30);
#			if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
				if (watchdog.pollHasTimedOut())
					reportGpuHang();
#			endif
			seg = (char*)s_SegHeap.alloc();
		} while (!seg);
	}

	control->incPending();
	grcGfxContextProfiler_AllocSegment();
	return seg;
}

void grcGfxContext::validatePending() {
#	if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		if (PARAM_validateGfx.Get()) {
			int errCode = sce::Gnm::validateCommandBuffers(m_PendingCount, m_DcbStarts, m_DcbSizesInBytes, m_CcbStarts, m_CcbSizesInBytes);
			if (errCode) {
				printf("Draw validation failed with code 0x%x", errCode);
				__debugbreak();
			}
		}
#	endif
}

inline u32 grcGfxContext::getDcbSizeBytes() const {
	return (u32)((size_t)m_dcb.m_cmdptr - (size_t)m_dcb.m_beginptr);
}

inline u32 grcGfxContext::getCcbSizeBytes() const {
	return (u32)((size_t)m_ccb.m_cmdptr - (size_t)m_ccb.m_beginptr);
}

/*static*/ u32 grcGfxContext::getContextPoolSize() {
	return s_ContextCount;
}

void grcGfxContext::submitPending(SubmitPendingSubmitTypeArg submitType) {
	validatePending();
	FatalAssert(m_Status == STATUS_OPEN || m_Status == STATUS_CLOSED);

	const u32 pendingCount = m_PendingCount;
	if (!pendingCount)
		return;

#	if GRCGFXCONTEXT_GNM_CONTEXT_STACK
		if (sm_Current != this)
			m_DbgState->m_ParentCtxDbgState = sm_Current->m_DbgState;
#	endif

	void **const dcbs = m_DcbStarts;
	void **const ccbs = m_CcbStarts;
	u32 *const dcbSizesInBytes = m_DcbSizesInBytes;
	u32 *const ccbSizesInBytes = m_CcbSizesInBytes;
	s_SegHeap.prepareSubmit((SegmentStart**)dcbs, dcbSizesInBytes, ccbs, ccbSizesInBytes, pendingCount,
		submitType==SUBMIT_PARTIAL ? SegmentHeap::SUBMIT_PARTIAL : SegmentHeap::SUBMIT_FULL);

	GpuSubmitCmdBufs *const submit = s_GpuSubmitCmdBufsFreeQueue.Pop();
	submit->m_Count = pendingCount;
	sysMemCpy(submit->m_DcbStarts,       dcbs,            pendingCount*(sizeof(*dcbs)));
	sysMemCpy(submit->m_DcbSizesInBytes, dcbSizesInBytes, pendingCount*(sizeof(*dcbSizesInBytes)));
	sysMemCpy(submit->m_CcbStarts,       ccbs,            pendingCount*(sizeof(*ccbs)));
	sysMemCpy(submit->m_CcbSizesInBytes, ccbSizesInBytes, pendingCount*(sizeof(*ccbSizesInBytes)));
	submit->m_VideoOutHandle = -1; // no flip
	s_GpuSubmitCmdBufsPendingQueue.Push(submit);

	m_PendingCount = 0;

	if (m_Status == STATUS_CLOSED) {
		sysMemWriteBarrier();
		m_Status = STATUS_CLOSED_AND_KICKED;
		// As soon as m_Status is set to STATUS_CLOSED_AND_KICKED, 'this' may
		// be allocated by another thread, so do not access it again here.
	}
	else {
#		if GRCGFXCONTEXT_GNM_USES_CCB
			// Since SegmentHeap::prepareSubmit has synchronized the CE and ME
			// tell CUE that it doesn't need to resync again for a while.
			m_cue.advanceFrame();
#		endif
	}
}

void grcGfxContext::addPending() {
	TrapGE(m_PendingCount, MaxPending);
	u32 *const dcbBeginptr = m_dcb.m_beginptr;
	const u32 dcbSizeBytes = getDcbSizeBytes();
	const u32 pendingCount = m_PendingCount;
	m_DcbSizesInBytes[pendingCount] = dcbSizeBytes;
	m_DcbStarts[pendingCount] = dcbBeginptr;
#	if GRCGFXCONTEXT_GNM_USES_CCB
		u32 *const ccbBeginptr = m_ccb.m_beginptr;
		const u32 ccbSizeBytes = getCcbSizeBytes();
		m_CcbSizesInBytes[pendingCount] = ccbSizeBytes;
		m_CcbStarts[pendingCount] = ccbBeginptr;
#	else
		m_CcbSizesInBytes[pendingCount] = 0;
		m_CcbStarts[pendingCount] = NULL;
		const u32 ccbSizeBytes = 0;
#	endif

#	if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		SegmentStart *const seg = (SegmentStart*)dcbBeginptr;
		seg->dcbSizeBytes = dcbSizeBytes;
		seg->ccbSizeBytes = ccbSizeBytes;
#	endif

	m_PendingCount = pendingCount+1;

	// Trash the current command buffer pointers to make sure they don't get used again.
	m_dcb.m_cmdptr = NULL;
#	if GRCGFXCONTEXT_GNM_USES_CCB
		m_ccb.m_cmdptr = NULL;
#	endif
#	if GRCGFXCONTEXT_GNM_USES_ACB
		m_acb.m_cmdptr = NULL;
#	endif
}

/*static*/ bool grcGfxContext::handleBufferFullStatic(sce::Gnm::CommandBuffer *cb, u32 sizeInDwords, void *userData) {
	return ((grcGfxContext*)userData)->handleBufferFull(cb,sizeInDwords);
}

bool grcGfxContext::handleBufferFull(sce::Gnm::CommandBuffer *cb, u32 sizeInDwords) {
	// Make sure we're not asking for a request that's bigger than will ever work.
#	if GRCGFXCONTEXT_GNM_USES_CCB
		FatalAssert((cb == &m_dcb && sizeInDwords <= s_DcbSizeBytes/4)
		         || (cb == &m_ccb && sizeInDwords <= s_CcbSizeBytes/4));
#	else
		FatalAssert((cb == &m_dcb && sizeInDwords <= s_DcbSizeBytes/4));
#	endif

	grcGfxContextControl *const control = m_Control;

	bool doPartialSubmit = false;
# if GRCGFXCONTEXT_GNM_USES_CCB
	if (!m_InDrawOrDispatch)
# endif
		if (control->isPartialSubmitAllowed()) {
			// TODO: Possibly best for performance to only kick once
			// m_PendingCount is greater than a certain threshold, otherwise we
			// could end up kicking each segment individually (which may or may
			// not be a bad thing).
			doPartialSubmit = true;

#			if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
				// The recordGpuIdleEpilogue must be before we mark the context as free.
				m_dcb.m_endptr += DCB_RESERVED_NUM_DWORDS_RECORD_GPU_IDLE_EPILOGUE;
				recordGpuIdleEpilogue();
#			endif
		}

	releaseCurrentSegment();
	addPending();

	if (doPartialSubmit) {
#		if GRCGFXCONTEXT_GNM_CONTEXT_STACK
			sysMemReadBarrier();
			grcGfxContext *const parent = control->m_ParentCtx;
			m_DbgState->m_ParentCtxDbgState = parent ? parent->m_DbgState : NULL;
#		endif

		control->clearPending();

		submitPending(SUBMIT_PARTIAL);
	}

	// Grab a new segment for this context
	assignSegmentToContext();

#	if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
		if (doPartialSubmit)
			recordGpuIdlePrologue();
#	endif

	return true;
}

void grcGfxContext::initializePerContextDefaultHardwareState() {

	// Much more light weight than initializeDefaultHardwareState.  There are
	// some states (eg, render targets) that we do inherit from previous
	// contexts, so we cannot call initializeDefaultHardwareState.

	// Force the index size to 16-bits.  Don't call directly on DCB, as we want
	// to update the Gnmx cached state as well.
	setIndexSize(sce::Gnm::kIndexSize16);
}

void grcGfxContext::assignSegmentToContext() {
	// Set up a new segment
	char *base = allocSegment();
	m_dcb.m_cmdptr = m_dcb.m_beginptr = (u32*) base;
	m_dcb.m_endptr = (u32*)(base + s_DcbSizeBytes) - DCB_RESERVED_NUM_DWORDS;
#	if GRCGFXCONTEXT_GNM_USES_CCB
		m_ccb.m_cmdptr = m_ccb.m_beginptr = (u32*)(base + s_DcbSizeBytes);
		m_ccb.m_endptr = (u32*)(base + s_DcbSizeBytes + s_CcbSizeBytes) - CCB_RESERVED_NUM_DWORDS;
#	endif
#	if GRCGFXCONTEXT_GNM_USES_ACB
		m_acb.m_cmdptr = m_acb.m_beginptr = m_acb.m_endptr = NULL;
#	endif

	SegmentStart *const seg = (SegmentStart*)(m_dcb.m_cmdptr);
	FatalAssert(seg->itNop     == SegmentStart_NOP);
	FatalAssert(seg->signature == SegmentStart_SIGNATURE);

#	if ENABLE_SEGMENT_ALLOC_CALLSTACK
		sysStack::CaptureStackTrace(seg->allocStack, NELEM(seg->allocStack));
#	endif

	m_dcb.m_cmdptr = (u32*)(seg+1);

	// Leave space for potentially patching in an invalidation of the GPU L2
	// cache lines that are marked as volatile (see prepareSubmit).
	m_dcb.insertNop(INVALIDATE_L2_VOLATILE_NUM_DWORDS);
	FatalAssert(*(u32*)(seg+1) == NOP_FOR_INVALIDATE_L2_VOLATILE);

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		// Allocate a new sync marker batch now, so that the start of the chain
		// is always at the end of the DCB.
		const bool allocCouldFail = false;
		allocSyncMarkerBatch(allocCouldFail);
#	endif

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		// If it has been "a while" since we last waited on all the sync markers
		// do so now.  This is to prevent running out of command segments, even
		// when grcGfxContextControl::isPartialSubmitAllowed() is returns true.
		if (++m_NumSegmentsSinceSyncMarkerWaitAll > (sm_MaxMaxPending>>1))
			gpuWaitAllSyncMarkers();
#	endif
}

void grcGfxContext::releaseSegment(void *seg_, unsigned releaseCount) {
	SegmentStart *const seg = (SegmentStart*)seg_;
	const u32 inUseCountCpu = (seg->inUseCountCpu -= releaseCount);
	FatalAssert((s32)inUseCountCpu >= 0);
	if (inUseCountCpu)
		return;

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS && __ASSERT
		// Ensure we are not releasing the segment a still has sync markers that
		// need to be waited on.
		u64 *const batchNextWrite = m_SyncMarkerBatchNextWrite;
		u64 *batchNextWait = m_SyncMarkerBatchNextWait;
		SegmentStart *waitSeg = s_SegHeap.getSeg(batchNextWait);
		if (batchNextWrite == batchNextWait) {
			FatalAssert(waitSeg != seg || m_SyncMarkerIdxNextWrite == m_SyncMarkerIdxNextWait);
		}
		else {
			do {
				FatalAssert(waitSeg != seg);
				batchNextWait = (u64*)(batchNextWait[SyncMarkerBatchNext]);
				waitSeg = s_SegHeap.getSeg(batchNextWait);
			} while (batchNextWait != batchNextWrite);
			FatalAssert(waitSeg != seg);
		}
#	endif

#	if !ENABLE_TRASH_DATA_AFTER_RELEASE
#		if __ASSERT
			m_dcb.reserveSpaceInDwords(DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT);
			u32 *const reserveStart = m_dcb.m_cmdptr;
#		endif
		m_dcb.dmaData(
			sce::Gnm::kDmaDataDstMemory, (u64)&seg->lastCacheInvalidate,
			sce::Gnm::kDmaDataSrcMemory, (u64)s_LastCacheInvalidateGPU,
			4, sce::Gnm::kDmaDataBlockingEnable);
 		m_dcb.writeAtEndOfPipe(sce::Gnm::kEopCbDbReadsDone,
 			sce::Gnm::kEventWriteDestMemory, (void*)&seg->inUseFlagGpu,
 			sce::Gnm::kEventWriteSource32BitsImmediate, 0,
 			sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
		FatalAssert(reserveStart+DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT == m_dcb.m_cmdptr);
#	else
 		// Wait on kEopCbDbReadsDone, but use a scratch address, not
 		// seg->inUseFlagGpu, since we don't want to free up the segment yet.
 		//
 		// In case we aren't being called from releaseCurrentSegment, reserve
 		// enough space that the following NOP (3 dwords), EVENT_WRITE_EOP (6
 		// dwords), and WAIT_REG_MEM (7 dwords), all fit in the same DCB.  The
 		// reason we use a NOP for the scratch address is that
 		// allocateFromCommandBuffer may fail when called from
 		// releaseCurrentSegment (just due to the way we reserve the space with
 		// DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT).  The NOP has 2 dwords worth
 		// of data so that we can pick the 32-bits that is 64-bit aligned.
 		//
 		m_dcb.reserveSpaceInDwords(16);
 		ASSERT_ONLY(u32 *const reserveStart = m_dcb.m_cmdptr;)
 		m_dcb.m_cmdptr[0] = PM4_TYPE3_HEADER(NOP, 3);
 		m_dcb.m_cmdptr[1] = 0xaaaaaaaa;
 		m_dcb.m_cmdptr[2] = 0xaaaaaaaa;
 		u32 *const scratch = (u32*)(((uptr)(m_dcb.m_cmdptr+1)+7)&~7);
 		m_dcb.m_cmdptr += 3;
 		m_dcb.writeAtEndOfPipe(sce::Gnm::kEopCbDbReadsDone,
 			sce::Gnm::kEventWriteDestMemory, scratch,
 			sce::Gnm::kEventWriteSource32BitsImmediate, 0x55555555,
 			sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
 		m_dcb.waitOnAddress(scratch, ~0, sce::Gnm::kWaitCompareFuncEqual, 0x55555555);
 		FatalAssert(reserveStart+16 == m_dcb.m_cmdptr);

 		// Now that the shaders are idle, trash the non-command buffer parts of
 		// the segment.
 		uptr dcbLastCmd;
 		const bool releasingSelf = (s_SegHeap.getSeg(m_dcb.m_cmdptr) == seg);
 		if (releasingSelf)
 			dcbLastCmd = (uptr)(m_dcb.m_cmdptr+DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT-16);
 		else
 			dcbLastCmd = (uptr)seg+seg->dcbSizeBytes;
 		const uptr dcbEnd = (uptr)seg+s_DcbSizeBytes;
 		m_dcb.dmaData(
 			sce::Gnm::kDmaDataDstMemory, (u64)dcbLastCmd,
 			sce::Gnm::kDmaDataSrcData, 0xfefefefefefefefeul,
 			dcbEnd-dcbLastCmd, sce::Gnm::kDmaDataBlockingEnable);
#		if GRCGFXCONTEXT_GNM_USES_CCB
 			uptr ccbLastCmd;
 			if (releasingSelf)
 				ccbLastCmd = (uptr)(m_ccb.m_cmdptr+CCB_RESERVED_NUM_DWORDS);
 			else
 				ccbLastCmd = dcbEnd+seg->ccbSizeBytes;
 			const uptr ccbEnd = dcbEnd+s_CcbSizeBytes;
 			m_dcb.dmaData(
 				sce::Gnm::kDmaDataDstMemory, (u64)ccbLastCmd,
 				sce::Gnm::kDmaDataSrcData, 0xfefefefefefefefeul,
 				ccbEnd-ccbLastCmd, sce::Gnm::kDmaDataBlockingEnable);
#		endif

 		// And finally, mark the segment as no longer in use.
		m_dcb.dmaData(
			sce::Gnm::kDmaDataDstMemory, (u64)&seg->lastCacheInvalidate,
			sce::Gnm::kDmaDataSrcMemory, (u64)s_LastCacheInvalidateGPU,
			4, sce::Gnm::kDmaDataBlockingEnable);
 		m_dcb.dmaData(
 			sce::Gnm::kDmaDataDstMemory, (u64)&seg->inUseFlagGpu,
 			sce::Gnm::kDmaDataSrcData, 0,
 			4, sce::Gnm::kDmaDataBlockingEnable);

 		FatalAssert(!releasingSelf || reserveStart+DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT == m_dcb.m_cmdptr);
#	endif
}

void grcGfxContext::releaseSegment(unsigned segIdx, unsigned releaseCount) {
	SegmentStart *const seg = s_SegHeap.getSeg(segIdx);
	releaseSegment(seg, releaseCount);
}

void grcGfxContext::releaseCurrentSegment(unsigned releaseCount) {
	m_dcb.m_endptr += DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT;
	ASSERT_ONLY(u32 *const cmdPtr = m_dcb.m_cmdptr;)
	releaseSegment((SegmentStart*)m_dcb.m_beginptr, releaseCount);
	FatalAssert(cmdPtr == m_dcb.m_cmdptr || cmdPtr+DCB_RESERVED_NUM_DWORDS_RELEASE_SEGMENT == m_dcb.m_cmdptr);
}

/*static*/ grcGfxContext *grcGfxContext::openContext(bool deferred, u16 UNUSED_PARAM(sequenceNumber), grcGfxContextControl *control) {
#	if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		CustomHangDetection watchdog("grcGfxContext::openContext", GRCGFXCONTEXT_GNM_GPU_HANG_TIMEOUT_SEC);
#	endif

	// Keep looping looking for a finished context
	for (;;) {
#	  if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		if (!s_AsyncFault)
#	  endif
		{
			// Try to allocate a context.  Note that the allocation (with
			// s_ContextLock locked) is seperated from the re-initialization
			// below.  Initialization can block trying to allocate a command
			// buffer segment, so we cannot have s_ContextLock held at that
			// point.
			grcGfxContext *ctx = NULL;
			{
				SYS_CS_SYNC(s_ContextLock);
				for (u32 i=0; i<s_ContextCount; i++) {
					grcGfxContext *const testCtx = s_Contexts + i;
					if (testCtx->m_Status == STATUS_CLOSED_AND_KICKED) {
						testCtx->m_Status = STATUS_OPEN;
						ctx = testCtx;
						break;
					}
				}
			}

			// If allocation succeeded, re-initialize the context
			if (ctx) {
				sm_Current = ctx;
				ctx->m_PendingCount = 0;
				ctx->m_Deferred = deferred;
				s_DefaultControl.clearPending();                    // value does creep up, simplest just to force back to zero here
				ctx->m_Control = control ? control : &s_DefaultControl;
				FatalAssert(ctx->m_AllocScope == &ctx->m_BaseAllocScope);
				FatalAssert(ctx->m_AllocScope->isDefaultState());
				ctx->m_AllocScope->generateId();
				FatalAssert(ctx->m_VsWaveLimitCurrent == 0);
				FatalAssert(ctx->m_VsWaveLimitLast    == 0);
#				if GRCGFXCONTEXT_GNM_USES_CCB
					FatalAssert(!ctx->m_InDrawOrDispatch);
#				endif
#				if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
					ctx->m_SyncMarkerIdxNextWrite            = 0;
					ctx->m_SyncMarkerIdxNextWait             = 0;
					ctx->m_SyncMarkerBatchNextWrite          = NULL;
					ctx->m_SyncMarkerBatchNextWait           = NULL;
					ctx->m_NumSegmentsSinceSyncMarkerWaitAll = -1;
					ctx->m_inGpuWaitAllSyncMarkers           = false;
#				endif
				ctx->assignSegmentToContext();
#				if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
					ctx->recordGpuIdlePrologue();
#				endif
				ctx->initializePerContextDefaultHardwareState();
#				if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
					ctx->m_DrawStart = ctx->m_dcb.m_cmdptr;
#				endif
#				if GRCGFXCONTEXT_GNM_CONTEXT_STACK
					grcGfxContextDebugState *const dbgState = ctx->allocateFromCommandBuffer<grcGfxContextDebugState>();
					sysMemSet(dbgState, 0, sizeof(*dbgState));
					ctx->m_DbgState = dbgState;
					ctx->m_dcb.writeAtEndOfPipe(sce::Gnm::kEopCbDbReadsDone,
						sce::Gnm::kEventWriteDestMemory, (void*)s_LastCtxDbgState,
						sce::Gnm::kEventWriteSource64BitsImmediate, (u64)dbgState,
						sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
					FatalAssert(ctx->m_FaultStackSP == 0);
#				endif
#				if ENABLE_PAGE_FAULT_CHECK
					ctx->m_dcb.setSqPerfCounterControl(sce::Gnm::kBroadcastAll, sce::Gnm::SqPerfCounterControl());
					ctx->m_dcb.selectTccPerfCounter(sce::Gnm::kBroadcastAll, 0, sce::Gnm::TccPerfCounterSelect(sce::Gnm::kTccPerfCounterMcRdretNack, sce::Gnm::kPerfmonCounterModeAccum));
					ctx->m_dcb.triggerEvent(sce::Gnm::kEventTypePerfCounterStop);
					ctx->m_dcb.setPerfmonEnable(sce::Gnm::kPerfmonEnable);
					ctx->m_dcb.setPerfCounterControl(sce::Gnm::PerfCounterControl(sce::Gnm::kPerfmonStateDisableAndReset, sce::Gnm::kPerfmonEnableModeAlwaysCount, sce::Gnm::kPerfmonNoSample));
					ctx->m_dcb.triggerEvent(sce::Gnm::kEventTypePerfCounterStart);
					ctx->m_dcb.setPerfCounterControl(sce::Gnm::PerfCounterControl(sce::Gnm::kPerfmonStateStartCounting, sce::Gnm::kPerfmonEnableModeAlwaysCount, sce::Gnm::kPerfmonNoSample));
#				endif
				return ctx;
			}
		}

		sceKernelUsleep(30);

#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
			if (watchdog.pollHasTimedOut())
				reportGpuHang();
#		endif
	}
}

void grcGfxContext::doKickContext() {
	releaseCurrentSegment();
	addPending();
	submitPending(SUBMIT_PARTIAL);
	assignSegmentToContext();
}

/*static*/ void grcGfxContext::kickContext() {
	grcGfxContext *const curr = sm_Current;
#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		curr->gpuWaitAllSyncMarkers();
#	endif
#	if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
		curr->recordGpuIdleEpilogue();
#		if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
			// Wait again, just in case recordGpuIdleEpilogue started a new
			// segment (which will allocate a new batch).
			curr->gpuWaitAllSyncMarkers();
#		endif
#	endif
	curr->doKickContext();
#	if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
		curr->recordGpuIdlePrologue();
#	endif
}

/*static*/ void grcGfxContext::kickContextIfPartialSubmitsAllowed() {
	grcGfxContextControl *const control = sm_Current->m_Control;
	if (control->isPartialSubmitAllowed())
		kickContext();
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
	if (control->isPartialSubmitAllowed() && curr->m_PendingCount>(control->getMaxPending()>>1)) {
		kickContext();
		return true;
	}
	return false;
}

#if GRCGFXCONTEXT_GNM_USES_CCB
	void grcGfxContext::safeKickPoint() {
		// Now that we are no longer inside of a draw or dispatch call, if
		// partial submit is allowed, then kick everything if we have "a lot"
		// pending.  "A lot" is currently defined as more than half of the
		// limit.  If partial submit is not yet allowed, but we are already at
		// the limit of the max that will ever be allowed to be allocated, then
		// need to stall and wait for partial submit to be enabled.
		//
		// This works around the case where we could get unlucky and
		// every segment ends during a draw or dispatch.  An even better
		// workaround would be to ditch CUE and switch to LCUE, or use command
		// buffer chaining so that we can submit at any point and the CE/DE
		// counters won't be reset.
		//
		FatalAssert(!m_InDrawOrDispatch);
		grcGfxContextControl *const control = m_Control;
		const u32 maxPending = control->getMaxPending();
		const u32 maxMaxPending = sm_MaxMaxPending;
		if (maxPending < maxMaxPending)
			return;

		const u32 pendingCount = m_PendingCount;
		if (pendingCount <= (maxMaxPending>>1))
			return;

		if (pendingCount<maxMaxPending-1 && !control->isPartialSubmitAllowed())
			return;

		// The case where we need to wait for partial submit to be enabled, is
		// never really expected to happen in real life.  So a simple sleep in a
		// loop will do.  It also means we are a long way ahead of the GPU, so
		// even if this does occur, it shouldn't be a performance issue.
		while (Unlikely(!control->isPartialSubmitAllowed()))
			sysIpcSleep(1);

#		if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
			recordGpuIdleEpilogue();
#		endif
		control->clearPending();
		doKickContext();
#		if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
			recordGpuIdlePrologue();
#		endif
	}
#endif

/*static*/ void grcGfxContext::closeContext(bool submit) {
	grcGfxContext *const curr = sm_Current;

	// Revert to default states
	curr->m_VsWaveLimitCurrent = 0;
	curr->flushVsWaveLimit();

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		curr->gpuWaitAllSyncMarkers();
#	endif
#	if GRCGFXCONTEXT_GNM_CONTEXT_STACK
		curr->m_dcb.dmaData(
			sce::Gnm::kDmaDataDstMemory, (u64)s_LastCtxDbgState,
			sce::Gnm::kDmaDataSrcMemory, (u64)&curr->m_DbgState->m_ParentCtxDbgState,
			8, sce::Gnm::kDmaDataBlockingEnable);
		FatalAssert(curr->m_FaultStackSP == 0);
#	endif

	curr->m_BaseAllocScope.releaseAll();
	FatalAssert(curr->m_AllocScope == &curr->m_BaseAllocScope);
	FatalAssert(curr->m_AllocScope->isDefaultState());

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		// While there can't have been any new draws or dispatches since the
		// last call to gpuWaitAllSyncMarkers above, it is possible that the
		// dmaData would have triggered a buffer full callback (which would
		// allocate a new sync marker batch).  So need to call
		// gpuWaitAllSyncMarkers a second time here.
		curr->gpuWaitAllSyncMarkers();
#	endif

#	if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
		// The recordGpuIdleEpilogue must be before we mark the context as free.
		curr->recordGpuIdleEpilogue();
#		if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
			// Wait again, just in case recordGpuIdleEpilogue started a new
			// segment (which will allocate a new batch).
			curr->gpuWaitAllSyncMarkers();
#		endif
#	endif

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
		// There is one additional reference to the final segment from the last
		// sync marker batch.  So reduce the reference count by 2 rather than 1
		// when GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS is enabled.
		curr->releaseCurrentSegment(2);
#	else
		curr->releaseCurrentSegment(1);
#	endif

	FatalAssert(curr->m_Status == STATUS_OPEN);
	curr->m_Status = STATUS_CLOSED;

	curr->addPending();

#	if ENABLE_LCUE
#		ifdef LCUE_PROFILE_ENABLED
			sce::LCUE::BaseCUE &p = *curr;
			Displayf("V# sets: %05u S# sets: %05u T# sets: %05u SetPtr: %05d PSU: %05d Draw: %05d Disp: %05d SRT Size: %07d",
				p.m_setVsharpCount, p.m_setSsharpCount, p.m_setTsharpCount, p.m_setPtrCount,
				p.m_psUsageCount, p.m_drawCount, p.m_dispatchCount, p.m_shaderResourceTotalUploadSizeInBytes);
#		endif
		curr->swapBuffers();
#	endif

	if (submit)
		curr->submitPending(SUBMIT_FULL);
}

/*static*/ void grcGfxContext::submitAndFlip(s32 videoOutHandle, u32 rtIndex, u32 flipMode, s64 flipArg) {
	grcGfxContext *const curr = sm_Current;

	// Finish up and start a brand new segment for the prepareFlip.
	//
	// Notice that status is temporarily set to STATUS_OPEN around the call to
	// assignSegmentToContext, this is for the rare case where
	// assignSegmentToContext will do a partial submit.  We don't want it to
	// then set the status to STATUS_CLOSED_AND_KICKED.  See
	// ENABLE_ALLOC_SEGMENT_RARE_CODE_PATH_TEST.
	//
	closeContext(false);
	FatalAssert(curr->m_Status == STATUS_CLOSED);
	curr->m_Status = STATUS_OPEN;
	curr->assignSegmentToContext();
	curr->m_Status = STATUS_CLOSED;

#	if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS && __ASSERT
		// closeContext will have already waited on all sync markers, but
		// assignSegmentToContext will have created a new (empty) batch.  But we
		// can't call gpuWaitAllSyncMarkers again to skip over that, since
		// closeContext will have already reduced the segment in use count to
		// zero.  Instead, just update the wait batch/index so that we don't
		// assert.
		curr->m_SyncMarkerBatchNextWait = curr->m_SyncMarkerBatchNextWrite;
		curr->m_SyncMarkerIdxNextWait = curr->m_SyncMarkerIdxNextWrite;
#	endif

	// Updating *s_LastCacheInvalidateGPU, and prepareFlip must both fit inside
	// a single segment.
	u32 lastCacheInvalidate = s_LastCacheInvalidateCPU;
	SegmentStart *const finalSeg = (SegmentStart*)curr->m_dcb.m_beginptr;
	finalSeg->inUseCountCpu = 0;
	finalSeg->lastCacheInvalidate = lastCacheInvalidate;
	++lastCacheInvalidate;
	curr->m_dcb.dmaData(
		sce::Gnm::kDmaDataDstMemory, (u64)s_LastCacheInvalidateGPU,
		sce::Gnm::kDmaDataSrcData, lastCacheInvalidate,
		4, sce::Gnm::kDmaDataBlockingEnable);
	s_LastCacheInvalidateCPU = lastCacheInvalidate;
	curr->m_dcb.prepareFlip((u32*)&finalSeg->inUseFlagGpu, 0);
	curr->addPending();

	const u32 pendingCount = curr->m_PendingCount;
	void **const dcbs = curr->m_DcbStarts;
	void **const ccbs = curr->m_CcbStarts;
	u32 *const dcbSizesInBytes = curr->m_DcbSizesInBytes;
	u32 *const ccbSizesInBytes = curr->m_CcbSizesInBytes;
	s_SegHeap.prepareSubmit((SegmentStart**)dcbs, dcbSizesInBytes, ccbs, ccbSizesInBytes, pendingCount, SegmentHeap::SUBMIT_FLIP);

	GpuSubmitCmdBufs *const submit = s_GpuSubmitCmdBufsFreeQueue.Pop();
	submit->m_Count = pendingCount;
	sysMemCpy(submit->m_DcbStarts,       dcbs,            pendingCount*(sizeof(*dcbs)));
	sysMemCpy(submit->m_DcbSizesInBytes, dcbSizesInBytes, pendingCount*(sizeof(*dcbSizesInBytes)));
	sysMemCpy(submit->m_CcbStarts,       ccbs,            pendingCount*(sizeof(*ccbs)));
	sysMemCpy(submit->m_CcbSizesInBytes, ccbSizesInBytes, pendingCount*(sizeof(*ccbSizesInBytes)));
	submit->m_VideoOutHandle = videoOutHandle;
	submit->m_RtIndex        = rtIndex;
	submit->m_FlipMode       = flipMode;
	submit->m_FlipArg        = flipArg;
	s_GpuSubmitCmdBufsPendingQueue.Push(submit);

	curr->m_PendingCount = 0;

	FatalAssert(curr->m_Status == STATUS_CLOSED);
	sysMemWriteBarrier();
	curr->m_Status = STATUS_CLOSED_AND_KICKED;
	// As soon as curr->m_Status is set to STATUS_CLOSED_AND_KICKED, it may be
	// allocated by another thread, so do not access it again here.

	s_SegHeap.incFrame();
}

#if __DEV
	/*static*/ void grcGfxContext::validateAndSubmitCurrent() {
		//sm_Current->validatePending();
		sm_Current->submitPending(SUBMIT_PARTIAL);
	}
#endif

/*static*/ void *grcGfxContext::tryUntrackedAllocateFromCommandBufferAlignBytes(sce::Gnm::CommandBuffer *cmdBuf, u32 sizeBytes, u32 alignBytes, u32 reservedBytes) {
	const size_t cmdptr        = (size_t)cmdBuf->m_cmdptr;
	const size_t curEndptr     = (size_t)cmdBuf->m_endptr;
	const size_t curTrueEndptr = curEndptr+reservedBytes;
	const size_t allocPtr      = (curTrueEndptr-sizeBytes)&-(size_t)alignBytes;
	const size_t newEndptr     = allocPtr-reservedBytes;

	// Enough room in segment?
	if (Likely(newEndptr >= cmdptr)) {
		cmdBuf->m_endptr = (u32*)newEndptr;
		return (void*)allocPtr;
	}

	return NULL;
}

void *grcGfxContext::tryUntrackedAllocateFromCommandBufferAlignBytes(u32 sizeBytes, u32 alignBytes) {
	// First try allocating out of the DCB
	void *ptr = tryUntrackedAllocateFromCommandBufferAlignBytes(&m_dcb, sizeBytes, alignBytes, DCB_RESERVED_NUM_DWORDS<<2);

	// If using CUE, then try allocating out of the CCB if the DCB failed
#	if GRCGFXCONTEXT_GNM_USES_CCB
		if (!ptr)
			ptr = tryUntrackedAllocateFromCommandBufferAlignBytes(&m_ccb, sizeBytes, alignBytes, CCB_RESERVED_NUM_DWORDS<<2);
#	endif

	return ptr;
}

void *grcGfxContext::untrackedAllocateFromCommandBufferAlignBytes(u32 sizeBytes, u32 alignBytes) {
	void *ptr = tryUntrackedAllocateFromCommandBufferAlignBytes(sizeBytes, alignBytes);
	if (ptr)
		return ptr;

	// Grab a new segment
	handleBufferFull(&m_dcb, sizeBytes>>2);

	// Must be enough room in the DCB now
	ptr = tryUntrackedAllocateFromCommandBufferAlignBytes(&m_dcb, sizeBytes, alignBytes, DCB_RESERVED_NUM_DWORDS<<2);
	FatalAssert(ptr);
	return ptr;
}

template<class T>
T *grcGfxContext::untrackedAllocateFromCommandBuffer(u32 num) {
	return (T*) untrackedAllocateFromCommandBufferAlignBytes(num*sizeof(T), __alignof__(T));
}

void *grcGfxContext::allocateFromCommandBufferAlignBytes(u32 sizeBytes, u32 alignBytes) {
	void *const ptr = untrackedAllocateFromCommandBufferAlignBytes(sizeBytes, alignBytes);

	m_AllocScope->assertNotLocked();

#	if __ASSERT
		m_AllocScope->m_SizeAlloced += sizeBytes;
#	endif

	const unsigned segIdx = s_SegHeap.getSegIndex(ptr);
	if (m_AllocScope->m_AllocedSegs.IsClear(segIdx)) {
		m_AllocScope->m_AllocedSegs.Set(segIdx);
		SegmentStart *const seg = s_SegHeap.getSeg(segIdx);
		++(seg->inUseCountCpu);
		FatalAssert(seg->inUseFlagGpu == 1);
	}

	return ptr;
}

u32 *grcGfxContext::embedData(sce::Gnm::CommandBuffer *cb, u32 numDwords, sce::Gnm::EmbeddedDataAlignment log2Align) {
	u32    align     = 1<<log2Align;
	size_t numBytes  = numDwords<<2;
	size_t alignAdd  = 4+align-1;   // +4 is for the NOP PM4 packet header
	size_t alignAnd  = -(size_t)align;
	size_t curCmdptr = (size_t)cb->m_cmdptr;
	size_t newCmdptr = ((curCmdptr+alignAdd)&alignAnd)+numBytes;

	if (Unlikely(newCmdptr >= (size_t)cb->m_endptr)) {
		handleBufferFull(cb, (alignAdd&alignAnd)+numBytes);
		curCmdptr = (size_t)cb->m_cmdptr;
		newCmdptr = ((curCmdptr+alignAdd)&alignAnd)+numBytes;
		FatalAssert(newCmdptr <= (size_t)cb->m_endptr);
	}

	const size_t mainNop = ((curCmdptr+alignAdd)&alignAnd)-4;
	const size_t paddingSizeBytes = mainNop-curCmdptr;

	// Add a padding NOP.  If padding is 4-bytes, then use a type 2 nop packet.
	// If padding is more, then use a type 3 NOP.  If there is no padding, then
	// we still write a (bogus) type 3 NOP packet header, but that is then
	// overwritten by the main nop.
	const u32 paddingNop = (paddingSizeBytes==4) ? PM4_TYPE2_HEADER() : PM4_TYPE3_HEADER(NOP, paddingSizeBytes/4);
	*(u32*)curCmdptr = paddingNop;

	*(u32*)mainNop = PM4_TYPE3_HEADER(NOP, numDwords+1);
	u32 *const ret = (u32*)mainNop+1;

	cb->m_cmdptr = (u32*)newCmdptr;

	return ret;
}

void grcGfxContext::crossContextFreeTemp(void *ptr, u32 sizeBytes) {
	// This function doesn't necissarily completely free up all of an
	// allocation.  Any alignment padding, or slop at the end of the previous
	// segment if a new segment was started, will not be freed up here.  This is
	// not an issue though, we are just trying to quickly free up most of it.
	if ((uptr)m_dcb.m_endptr == (uptr)ptr)
		m_dcb.m_endptr = (u32*)((uptr)ptr+(sizeBytes&~3));
	else {
		FatalAssert((uptr)m_ccb.m_endptr == (uptr)ptr);
		m_ccb.m_endptr = (u32*)((uptr)ptr+(sizeBytes&~3));
	}
}

void grcGfxContext::crossContextIncSegmentRefCount(const void *ptr) {
	s_SegHeap.crossContextIncSegmentRefCount(ptr);
}

void grcGfxContext::crossContextDecSegmentRefCount(const void *ptr) {
	SegmentStart *const seg = s_SegHeap.crossContextDecSegmentRefCount(ptr);
	if (seg)
		releaseSegment(seg, 0u);
}

void grcGfxContext::flushVsWaveLimit() {
	const u32 vsWaveLimit = m_VsWaveLimitCurrent;
	if (Unlikely(m_VsWaveLimitLast != vsWaveLimit)) {
		m_VsWaveLimitLast = vsWaveLimit;
		const u32 lockThreshold = 0;
		setGraphicsShaderControl(sce::Gnm::kShaderStageVs, VS_CU_MASK_DEFAULT, vsWaveLimit, lockThreshold);
	}
}

#if !__FINAL || !__NO_OUTPUT
	__attribute__((__noinline__)) /*static*/ void grcGfxContext::reportGpuHang() {
		if (PARAM_gpuDebugger.Get())
			return;
		static volatile u32 reported/*=0*/;
		if (sysInterlockedIncrement(&reported) == 1) {
			printf("GPU timeout.  Probable crash or hang.\n");
#		  if SYSTMCMD_ENABLE
			const bool rtmConnected = PARAM_rockstartargetmanager.Get();
			if (rtmConnected)
				sysTmCmdGpuHang();
			if (!sysBootManager::IsDevkit() || !rtmConnected)
#		  endif
				OrbisGpuCrashCallback();
		}
		for (;;) sysIpcSleep(1);
	}

	/*static*/ void grcGfxContext::dumpCrashInfo() {
#		if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
			// Iterate through the list of submitted segments to find the oldest failed draw call
			bool foundFailedDraw = false;
			s_SegHeap.lock();
			const SegmentStart *seg = s_SegHeap.getOldestSubmitted();
			const SegmentStart *const end = s_SegHeap.submittedListSentinal();
			while (seg != end) {
				const u64 *batch = (u64*)((size_t)seg+s_DcbSizeBytes)-MaxSyncMarkersPerBatch;
				do {
					for (u32 draw=2; draw<MaxSyncMarkersPerBatch; ++draw) {
						const u64 marker = batch[draw];
						if (!marker)
							break;
						if ((marker & SyncMarkerStateMask) != SyncMarkerShaderDone) {
							printf("First failed draw call starts at address 0x%010lx (DCB 0x%010lx L0x%06x, CCB 0x%010lx L0x%06x)\n",
								marker&~SyncMarkerStateMask, (u64)seg, seg->dcbSizeBytes, (u64)seg+s_DcbSizeBytes, seg->ccbSizeBytes);
							foundFailedDraw = true;
							goto found_failed_draw;
						}
					}
					batch = (u64*)(batch[SyncMarkerBatchNext]);

					// Dont cross a segment boundary here, as the next segment may not still be valid
					if (s_SegHeap.getSeg(batch) != seg)
						break;

				} while (batch);
				seg = seg->nextSubmitted;
			}
		found_failed_draw:
			s_SegHeap.unlock();

#			if GRCGFXCONTEXT_GNM_CONTEXT_STACK
				if (foundFailedDraw)
					printf("Fault Context:\n");
				else
					printf("Fault Context: (WARNING: likely to be meaningless since no failed drawcall found!)\n");
				grcGfxContextDebugState *dbgState = *s_LastCtxDbgState;
				while (dbgState) {
					for (u32 sp=NELEM(dbgState->m_FaultStack); sp-->0;) {
						const char *const label = (char*)(dbgState->m_FaultStack[sp]);
						if (*label)
							printf("  %s\n", label);
					}
					dbgState = dbgState->m_ParentCtxDbgState;
				}
				printf("\n");
#			endif
#		endif
	}
#endif

#if GRCGFXCONTEXT_GNM_RECORD_GPU_IDLES
	void grcGfxContext::recordGpuIdlePrologue() {
		SegmentStart *const seg = (SegmentStart*)m_dcb.m_beginptr;
		seg->endLastSubmitTimestamp = -1;
		seg->beginThisSubmitTimestamp = -1;
		m_dcb.writeAtEndOfPipe(sce::Gnm::kEopCbDbReadsDone,
			sce::Gnm::kEventWriteDestMemory, &seg->beginThisSubmitTimestamp,
			sce::Gnm::kEventWriteSourceGpuCoreClockCounter, 0,
			sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);

		// This is a bit wrong.  We want to make sure that the next dmaData
		// doesn't fetch from s_EndSubmitTimestamp before the writeAtEndOfPipe
		// previous recordGpuIdleEpilogue has run.  But this will lock up if the
		// GPU clock ever reaches 0xffffffff00000000.  The clock does seem to
		// reset to 0 though (on either power cycle or reboot?).  So being debug
		// code only, this should be ok.
		m_dcb.waitOnAddress((u32*)&seg->beginThisSubmitTimestamp+1,
			0xffffffff, sce::Gnm::kWaitCompareFuncNotEqual, 0xffffffff);

		m_dcb.dmaData(sce::Gnm::kDmaDataDstMemory, (u64)&seg->endLastSubmitTimestamp,
			sce::Gnm::kDmaDataSrcMemory, (u64)s_EndSubmitTimestamp, 8, sce::Gnm::kDmaDataBlockingEnable);
	}

	void grcGfxContext::recordGpuIdleEpilogue() {
		m_dcb.writeAtEndOfPipe(sce::Gnm::kEopCbDbReadsDone,
			sce::Gnm::kEventWriteDestMemory, (void*)s_EndSubmitTimestamp,
			sce::Gnm::kEventWriteSourceGpuCoreClockCounter, 0,
			sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
	}

	struct GpuIdle {
		// Records the beginning and ending of the idle time.  Note the
		// difference between this and SegmentStart::endLastSubmitTimestamp,
		// SegmentStart::beginThisSubmitTimestamp, where the "begin" and "end"
		// terms are swapped around the other way.
		s64 begin, end;
	};

	static GpuIdle s_gpuIdles[512];
	CompileTimeAssert((NELEM(s_gpuIdles)&(NELEM(s_gpuIdles)-1)) == 0);
	static u64 s_gpuIdlesOldestIdx, s_gpuIdlesNewestIdxPlusOne;

	static inline const SegmentStart *getNextIdleRecoding(const SegmentStart *seg, const SegmentStart *sentinal) {
		// Loop until we reach a segment that has non-zero
		// endLastSubmitTimestamp and beginThisSubmitTimestamp.  Most of the
		// time these are zero, it is because the segment is not the first
		// segment out of a batch that was submitted.  But can also read back
		// zero if the GPU has not yet executed it (in which case we can just
		// skip over it anyways).
		s64 e, b;
		do {
			seg = seg->nextSubmitted;
			if (seg == sentinal)
				return NULL;

			e = seg->endLastSubmitTimestamp;
			b = seg->beginThisSubmitTimestamp;

			// GPU will, but hasn't yet writen a timestamp?
			if (e==-1 || b==-1)
				return NULL;

		} while (!e || !b);
		return seg;
	}

	static void collectGpuIdles() {
		const SegmentStart *const sentinal = s_SegHeap.submittedListSentinal();
		const SegmentStart *seg = getNextIdleRecoding(sentinal, sentinal);

		// No recorded idle sections yet (ie, game just starting up)?
		if (Unlikely(s_gpuIdlesOldestIdx == s_gpuIdlesNewestIdxPlusOne)) {
			// Copy across all the idle sections.  Relies on there being more
			// space in s_gpuIdles than there is segments (which there will be
			// because that is the entire point of copying to a separate array).
			//
			// TODO: When integrating with new command buffer management code,
			// comment in the following compile time assert.
			//
			//CompileTimeAssert(NELEM(s_gpuIdles) >= GRCCONTEXT_MAX_NUM_SEGMENTS);
			u64 i=0;
			while (seg) {
				s_gpuIdles[i].begin = seg->endLastSubmitTimestamp;
				s_gpuIdles[i].end   = seg->beginThisSubmitTimestamp;
				++i;
				seg = getNextIdleRecoding(seg, sentinal);
			}
			s_gpuIdlesOldestIdx = 0;
			s_gpuIdlesNewestIdxPlusOne = i;
		}

		// Have at least recorded some idle sections?
		else {
			u64 oldestIdx = s_gpuIdlesOldestIdx;
			u64 newestIdxPlusOne = s_gpuIdlesNewestIdxPlusOne;
			u64 newestIdx = (newestIdxPlusOne-1) & (NELEM(s_gpuIdles)-1);

			// Skip forward till the first new idle section.
			s64 newestRecordedBegin = s_gpuIdles[newestIdx].begin;
			while (seg && seg->endLastSubmitTimestamp-newestRecordedBegin<=0)
				seg = getNextIdleRecoding(seg, sentinal);

			// Copy any new idle sections.
			while (seg) {
				newestIdx = newestIdxPlusOne;
				newestIdxPlusOne = (newestIdxPlusOne+1) & (NELEM(s_gpuIdles)-1);
				if (oldestIdx == newestIdxPlusOne)
					oldestIdx = (oldestIdx+1) & (NELEM(s_gpuIdles)-1);
				s_gpuIdles[newestIdx].begin = seg->endLastSubmitTimestamp;
				s_gpuIdles[newestIdx].end   = seg->beginThisSubmitTimestamp;
				seg = getNextIdleRecoding(seg, sentinal);
			}

			s_gpuIdlesOldestIdx = oldestIdx;
			s_gpuIdlesNewestIdxPlusOne = newestIdxPlusOne;
		}
	}

	/*static*/ s64 grcGfxContext::getRecordedGpuIdleBetweenTimestamps(s64 begin, s64 end) {
		s64 idle = 0;
		s_SegHeap.lock();

		collectGpuIdles();
		const u64 oldestIdx = s_gpuIdlesOldestIdx;
		const u64 newestIdxPlusOne = s_gpuIdlesNewestIdxPlusOne;
		if (oldestIdx != newestIdxPlusOne) {
			// Loop until we find the first idle finishing at or after 'begin'.
			u64 idx = oldestIdx;
			while (idx!=newestIdxPlusOne && s_gpuIdles[idx].end-begin<0)
				idx = (idx+1) & (NELEM(s_gpuIdles)-1);

			if (idx != newestIdxPlusOne) {
				// If the idle section begins after 'end' then there is no idle
				// section in the range [begin..end].
				if (s_gpuIdles[idx].begin-end > 0) {
					// Though if idx is the oldest idle section still recorded,
					// then the size of s_gpuIdles should be increased.  This
					// assert is purposely non-fatal (unlike most asserts in
					// this file).  The second condition here is to avoid
					// asserting on game startup before we have a full ring
					// buffer of idle times.
					//grcAssert(idx!=oldestIdx || (oldestIdx!=((newestIdxPlusOne+1)&(NELEM(s_gpuIdles)-1))));
				#if __ASSERT
					if( !(idx!=oldestIdx || (oldestIdx!=((newestIdxPlusOne+1)&(NELEM(s_gpuIdles)-1)))) )
					{
						grcWarningf("s_gpuIdles: idx (%lu) != oldestIdx (%lu), s_gpuIdles[] should be increased!", idx, oldestIdx);
					}
				#endif //__ASSERT
				}

				// If the idle section ends after 'end', then the range
				// [begin..end] is completely contained within the idle section.
				else if (s_gpuIdles[idx].end-end > 0) {
					idle = end - begin;
				}

				// Else the standard case where the idle section finishes before
				// 'end', and we need to continue checking for other idle
				// sections within range.
				else {
					// 'begin' may or may not be within this idle section, so
					// take the min idle times from begin to idle end, and idle
					// begin to idle end.
					idle = Min(s_gpuIdles[idx].end-begin, s_gpuIdles[idx].end-s_gpuIdles[idx].begin);

					// Loop over all idle sections that end before 'end'.
					idx = (idx+1) & (NELEM(s_gpuIdles)-1);
					while (idx!=newestIdxPlusOne && s_gpuIdles[idx].end-end<0) {
						idle += s_gpuIdles[idx].end - s_gpuIdles[idx].begin;
						idx = (idx+1) & (NELEM(s_gpuIdles)-1);
					}

					if (idx != newestIdxPlusOne)
 						// 'end' may or may not be within this last idle section.
						idle += Max<s64>(end-s_gpuIdles[idx].begin, 0);
				}
			}
		}

		s_SegHeap.unlock();
		return idle;
	}
#endif

#if GRCGFXCONTEXT_GNM_PER_DRAW_MARKERS
	void grcGfxContext::allocSyncMarkerBatch(bool ASSERT_ONLY(allocCouldFail)) {
		// Try to allocate space for a new batch within the current segment.
		const u32 allocSizeBytes = MaxSyncMarkersPerBatch*sizeof(u64);
		u64 *const batch = (u64*)tryUntrackedAllocateFromCommandBufferAlignBytes(allocSizeBytes, __alignof__(u64));

		// If the allocation fails, then just run the standard buffer full
		// callback, as that will end up creating an new sync marker batch for
		// us.
		FatalAssert(allocCouldFail || batch);
		if (!batch) {
			handleBufferFull(&m_dcb, allocSizeBytes>>2);
			return;
		}

		// Allocation passed, so initialize the new batch.
		sysMemSet(batch, 0, MaxSyncMarkersPerBatch*sizeof(u64));
		u64 *const prevBatch = m_SyncMarkerBatchNextWrite;
		m_SyncMarkerBatchNextWrite = batch;
		m_SyncMarkerIdxNextWrite = 2;

		// If there was a previous batch then link to it.  The previous batch
		// cannot be in segment that has already been freed up by the GPU, since
		// we can't have moved m_SyncMarkerBatchNextWait/m_SyncMarkerIdxNextWait
		// past it yet.
		if (prevBatch) {
			batch[SyncMarkerBatchPrev] = (u64)prevBatch;
			FatalAssert(!prevBatch[SyncMarkerBatchNext]); // mainly useful to catch errors in conjunction with ENABLE_TRASH_DATA_AFTER_RELEASE
			prevBatch[SyncMarkerBatchNext] = (u64)batch;
		}
		// Otherwise, if this is the very first batch for a context, then
		// initialise the wait batch and index.
		else {
			m_SyncMarkerBatchNextWait = batch;
			m_SyncMarkerIdxNextWait = 2;
		}

		// Increment the segment in use counter once for each sync marker batch.
		SegmentStart *const seg = (SegmentStart*)m_dcb.m_beginptr;
		FatalAssert(s_SegHeap.getSeg(batch) == seg);
		FatalAssert(seg->itNop     == SegmentStart_NOP);
		FatalAssert(seg->signature == SegmentStart_SIGNATURE);
		++(seg->inUseCountCpu);
	}

	void grcGfxContext::gpuWaitAllSyncMarkers() {
		// Since we are adding commands to the DCB here, need to make sure we
		// don't start recursing.
		if (m_inGpuWaitAllSyncMarkers)
			return;
		m_inGpuWaitAllSyncMarkers = true;

		u64 *batchNextWait = m_SyncMarkerBatchNextWait;
		u32 idxNextWait = m_SyncMarkerIdxNextWait;

		// Note that the member variables m_SyncMarkerBatchNextWrite and
		// m_SyncMarkerIdxNextWrite do need to be reloaded each loop, since the
		// DCB commands may trigger a buffer full callback (which then allocates
		// a new sync marker batch).
		SegmentStart *seg = s_SegHeap.getSeg(batchNextWait);
		while (batchNextWait!=m_SyncMarkerBatchNextWrite || idxNextWait!=m_SyncMarkerIdxNextWrite) {
			// If the sync marker is non-zero then wait on it.  If it is zero,
			// then that is the end of this batch, so go on to the next.
			u64 *ptr, val=0;
			FatalAssert(idxNextWait <= MaxSyncMarkersPerBatch);
			if (idxNextWait < MaxSyncMarkersPerBatch) {
				ptr = batchNextWait+idxNextWait;
				val = *ptr;
				if (val) {
					// TODO: Possibly only need to actually wait on the most recent,
					// not all of the markers.  Not really sure though, there may be
					// cases where an EOP event can trigger on later drawcalls after
					// one fails.  If this is changed, then be careful to ensure
					// that the last segment is not released until after the
					// waitOnAddress.
					m_dcb.waitOnAddress((u32*)ptr+1, ~0, sce::Gnm::kWaitCompareFuncEqual, ((val&~SyncMarkerStateMask)|SyncMarkerShaderDone)>>32);
					FatalAssert(s_SegHeap.getSeg((u32*)ptr+1) == seg);

					++idxNextWait;

					continue;
				}
			}

			// Either the next marker is 0 or the index is past the max element.
			// So move on to the next batch.
			idxNextWait = 2;
			batchNextWait = (u64*)(batchNextWait[SyncMarkerBatchNext]);
			FatalAssert(batchNextWait);

#			if __ASSERT
 				// In assert enabled builds, update the member variables before
 				// calling releaseSegment.  This is just so that releaseSegment
 				// doesn't assert and complain that we are releasing a segment
 				// containing an as of yet unwaited on sync marker.  For
 				// non-assert builds, we can wait until the end of the loop
 				// before updating these.
 				m_SyncMarkerIdxNextWait = idxNextWait;
 				m_SyncMarkerBatchNextWait = batchNextWait;
#			endif

			// Release the segment containing the batch we were just looking at.
			releaseSegment(seg);

			seg = s_SegHeap.getSeg(batchNextWait);
		}

		m_SyncMarkerBatchNextWait = batchNextWait;
		m_SyncMarkerIdxNextWait = idxNextWait;

		m_NumSegmentsSinceSyncMarkerWaitAll = 0;
		m_inGpuWaitAllSyncMarkers = false;
	}

	void grcGfxContext::markEnd() {
		// Because markEnd uses an EVENT_WRITE_EOP rather than an
		// EVENT_WRITE_EOS, it is not an issue for grcGfxContextBase to be
		// inserting other PM4 packets after the draw/dispatch
		// (eg. sce::Gnmx::ConstantUpdateEngine::postDraw()).  EVENT_WRITE_EOS
		// has much stricter restrictions on when it is allowed, according to
		// the AMD docs (si_programming_guide_v2.pdf, section 2.1.5), there must
		// be no other PM4 packets inbetween.
		//
		// Interestingly, the Sony GNM docs only mention this restriction for
		// compute (Gnm-Reference_e.pdf,
		// sce::Gnm::DrawCommandBuffer::writeAtEndOfShader notes).
		//

		// Make sure we have enough space for the dmaData/DMA_DATA and
		// writeAtEndOfPipe/EVENT_WRITE_EOP, before we access the sync marker
		// next write values.
		enum { DCB_NUM_DWORDS = 13 };
		m_dcb.reserveSpaceInDwords(DCB_NUM_DWORDS);
		ASSERT_ONLY(u32 *const begin = m_dcb.m_cmdptr;)

		u32 idx = m_SyncMarkerIdxNextWrite;
		u64 *const syncMarker = m_SyncMarkerBatchNextWrite+idx;

		// Update the marker first from the ME, then from the backend.
		const u64 initialValue = (u64)m_DrawStart;
		const u64 drawEngineDoneValue = initialValue | SyncMarkerDrawEngineDone;
		const u64 shaderDoneValue     = initialValue | SyncMarkerShaderDone;
		*syncMarker = initialValue;
		m_dcb.dmaData(sce::Gnm::kDmaDataDstMemory, (u64)syncMarker+4,
			sce::Gnm::kDmaDataSrcData, drawEngineDoneValue>>32, 4, sce::Gnm::kDmaDataBlockingEnable);
		// Note that sce::Gnm::kEopCbDbReadsDone and sce::Gnm::kEopCsDone are
		// actually the same event, VGT_EVENT_INITIATOR event 0x28,
		// BOTTOM_OF_PIPE_TS.
		CompileTimeAssert(sce::Gnm::kEopCbDbReadsDone == sce::Gnm::kEopCsDone);
		m_dcb.writeAtEndOfPipe(
			sce::Gnm::kEopCbDbReadsDone,
			sce::Gnm::kEventWriteDestMemory, syncMarker,
			sce::Gnm::kEventWriteSource64BitsImmediate, shaderDoneValue,
			sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
		FatalAssert(m_dcb.m_cmdptr - begin == DCB_NUM_DWORDS);

		// Always update m_SyncMarkerIdxNextWrite here, even if we are about to
		// call allocSyncMarkerBatch which will overwrite it.  Updating
		// m_SyncMarkerIdxNextWrite makes sure that allocSyncMarkerBatch doesn't
		// think that all markers have currently been waited on (which would
		// cause it to move m_SyncMarkerIdxNextWait forward incorrectly).
		m_SyncMarkerIdxNextWrite = ++idx;
		if (idx >= MaxSyncMarkersPerBatch) {
			FatalAssert(idx == MaxSyncMarkersPerBatch);
			const bool allocCouldFail = true;
			allocSyncMarkerBatch(allocCouldFail);
		}

#		if ENABLE_WAIT_AFTER_EVERY_DRAW
			gpuWaitAllSyncMarkers();

#			if ENABLE_PAGE_FAULT_CHECK
				m_dcb.triggerEvent(sce::Gnm::kEventTypePerfCounterSample);
				m_dcb.setPerfCounterControl(sce::Gnm::PerfCounterControl(sce::Gnm::kPerfmonStateStartCounting, sce::Gnm::kPerfmonEnableModeAlwaysCount, sce::Gnm::kPerfmonSample));

				u64 *p = s_PerfCounterValues;
				const unsigned numTccs = 8;
				for (unsigned tcc=0; tcc<numTccs; ++tcc)
					m_dcb.readTccPerfCounter(tcc, 0, p++);
				p = s_PerfCounterValues;
				for (unsigned i=0; i<numTccs; ++i)
					m_dcb.waitOnAddress(p++, 0xffffffff, sce::Gnm::kWaitCompareFuncEqual, 0);
#			endif
#		endif

#		if GRCGFXCONTEXT_GNM_USES_CCB
			// Reserve CCB space before issuing writeDataInline to make sure
			// that we write the correct CCB pointer to memory.
			enum { CCB_NUM_DWORDS = 6 };
			m_ccb.reserveSpaceInDwords(CCB_NUM_DWORDS);
			const u64 ccbPtr = (u64)m_ccb.m_cmdptr;
			m_ccb.writeDataInline((void*)s_CeAddr, &ccbPtr, 2, sce::Gnm::kWriteDataConfirmEnable);
			FatalAssert(m_ccb.m_cmdptr - (u32*)ccbPtr == CCB_NUM_DWORDS);
#		endif

		m_DrawStart = m_dcb.m_cmdptr;
	}
#endif

#if GRCGFXCONTEXT_GNM_CONTEXT_STACK
	/*static*/ void grcGfxContext::PushFaultContext(const char *label) {
		grcGfxContext *const curr = sm_Current;

		// Wait for all previous rendering to complete so that the label update
		// will not occur too early.
		curr->gpuWaitAllSyncMarkers();

		// Push the label string onto the top of the stack in the context's
		// grcGfxContextDebugState struct using a writeDataInline.

		const size_t dstLen = NELEM(curr->m_DbgState->m_FaultStack[0]);
		const size_t srcLen = Min(strlen(label), dstLen-1);

		const u32 sp = curr->m_FaultStackSP;
		curr->m_FaultStackSP = sp+1;
		FatalAssert(sp < NELEM(curr->m_DbgState->m_FaultStack));

		const u64 dst = (u64)(curr->m_DbgState->m_FaultStack+sp);

		// TRC violation, but this is debug code only, so doesn't really matter.
		// Would be nice if there was a version of writeDataInline that returned
		// the pointer rather than requiring it to be copied from somewhere.
		// But currently there isn't so do it ourself to save needing to create
		// a temporary to copy from.
		const u32 numDwords = 4 + (dstLen>>2);
		curr->m_dcb.reserveSpaceInDwords(numDwords);
		u32 *const cmdPtr = curr->m_dcb.m_cmdptr;
		cmdPtr[0] = PM4_TYPE3_HEADER(WRITE_DATA, numDwords);
		cmdPtr[1] = 0x04100500; // ENGINE_SEL=ME, WR_DATA_CACHE_POLICY=2(bypass), WR_CONFIRM=1(confirm), WR_ONE_ADDR=0(increment), DS_SEL=5(memory async)
		cmdPtr[2] = (u32)dst;
		cmdPtr[3] = (u32)(dst>>32);
		sysMemCpy((char*)(cmdPtr+4), label, srcLen);
		sysMemSet((char*)(cmdPtr+4)+srcLen, 0, dstLen-srcLen);
		curr->m_dcb.m_cmdptr = cmdPtr + numDwords;
	}

	/*static*/ void grcGfxContext::PopFaultContext() {
		grcGfxContext *const curr = sm_Current;

		// Wait for all previous rendering to complete so that the label update
		// will not occur too early.
		curr->gpuWaitAllSyncMarkers();

		// Zero out the top entry of the stack in the context's
		// grcGfxContextDebugState struct.
		u32 sp = curr->m_FaultStackSP;
		FatalAssert(sp);
		curr->m_FaultStackSP = --sp;
		curr->m_dcb.dmaData(
			sce::Gnm::kDmaDataDstMemory, (u64)(curr->m_DbgState->m_FaultStack+sp),
			sce::Gnm::kDmaDataSrcData, 0,
			NELEM(curr->m_DbgState->m_FaultStack[0]),
			sce::Gnm::kDmaDataBlockingEnable);
	}

	/*static*/ u32 grcGfxContext::HackGetFaultContextSP() {
		return sm_Current->m_FaultStackSP;
	}
#endif

#if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
	// No inline because R*TM also needs access to this function
	__attribute__((__noinline__)) void OrbisGpuCrashCallback(u64 arg0, u64 arg1, u64 arg2) {
		switch (arg0) {
			case 0: {
				GRCDBG_IMPLEMENTED_ONLY(grcGfxContext::dumpCrashInfo();)

				sce::Gnm::debugHardwareStatus(sce::Gnm::kHardwareStatusDump);
				// debugHardwareStatus seems to return immediately, so as a kludge just wait for a bit
				sysIpcSleep(2000);

				s_SegHeap.validateSubmitted();
				break;
			}

			case 1: {
#				if !__FINAL && !__NO_OUTPUT
					sysStack::TargetCrashCallback(0);
#				endif
				s_AsyncFault = true;
				break;
			}

			case 2: {
				sce::Gnm::Pm4Dump::dumpPm4PacketStream((u32*)arg1, arg2);
				break;
			}
		}
	}
#endif

} // namespace rage

#endif // RSG_ORBIS
