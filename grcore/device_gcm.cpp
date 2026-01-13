//
// grcore/device_gcm.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//
#include "config.h"

#if __GCM
#include "channel.h"
#include "bank/bkmgr.h"
#include "device.h"
#include "effect.h"
#include "effect_config.h"
#include "effect_internal.h"
#include "grmodel/shader.h"
#include "im.h"
#include "quads.h"
#include "stateblock.h"
#include "system/typeinfo.h"	// HACK
#include "system/task.h"
#include "system/memory.h"
#include "system/bootmgr.h"
#include "system/externalallocator.h"
#include "system/replay.h"
#include "system/spurs_gcm_config.h"
#include "viewport.h"
#include "system/ipc.h"
#include "system/param.h"
#include "system/simpleallocator.h"
#include "system/stack.h"
#include "system/wndproc.h"
#include "atl/freelist.h"
#include "atl/pool.h"
#include "indexbuffer.h"
#include "vertexbuffer.h"
#include "profile/element.h"
#include "profile/group.h"
#include "quads.h"
#include "image.h"
#include "texture.h"
#include "texturegcm.h"
#include "shaderlib/rage_constants.h"
#if __PPU
#include "grcorespu.h"
#include "system/tasklog.h"
#if !__FINAL
#include "grcore/setup.h"
#endif // !__FINAL
#endif
#include "diag/tracker.h"
#include "file/remote.h"		// for fiIsShowingMessageBox

#include "wrapper_gcm.h"
#include <cell/sysmodule.h>
#include <sysutil/sysutil_sysparam.h>
#include <sys/event.h>
#include <sys/memory.h>
#include <sys/synchronization.h>
#include <sys/timer.h>
#include <sys/tty.h>
#if !__FINAL
#include <sys/gpio.h>
#include "input/pad.h"
#include <cell/dbgrsx.h>
#pragma comment(lib,"dbgrsx")
#include <cell/gcm_pm.h>
#endif // !__FINAL

#include <cell/gcm/gcm_gpad.h>
#include <cell/spurs/types.h>		// for CellSpurs

#if GCM_REPLAY
#include "system/replay.h"
#if CELL_SDK_VERSION <= 0x300001
#pragma comment(lib, "dbg_capture")
#else
#pragma comment(lib, "gcm_gpad_stub")
#endif
#endif
#if !__FINAL
#include <sn/libsntuner.h>
#endif

#pragma comment(lib, "gcm_cmd")
#pragma comment(lib, "gcm_sys_stub")
#pragma comment(lib, "sysmodule_stub")
#pragma comment(lib, "sysutil_avconf_ext_stub")

#if RAGE_TRACKING
#include "system/memvisualize.h"
#endif

extern const rage::u32 pgMaxHandles;

namespace rage
{
	namespace GraphicsStats
	{
//		EXT_PF_GROUP(GfxGeneric);
/* 		EXT_PF_COUNTER(EffectCacheHits);
		EXT_PF_COUNTER(EffectCacheMisses);
		EXT_PF_VALUE_FLOAT(EffectCacheHitRatio);
		EXT_PF_COUNTER(EffectCacheFlushes);
		EXT_PF_COUNTER(EffectCacheNumFlushed);
		EXT_PF_COUNTER(EffectCacheSizeAccumulator);
		EXT_PF_VALUE_FLOAT(EffectCacheUtilization);

		EXT_PF_COUNTER(LookAhead1);
		EXT_PF_COUNTER(LookAhead2);
		EXT_PF_COUNTER(LookAhead3);
		EXT_PF_COUNTER(LookAhead4);
		EXT_PF_COUNTER(LookAhead5);
		EXT_PF_COUNTER(LookAhead6); */
	}
}

// Specify if you want to use the OpenGL, XBox 360, or the D3D10+ window and VPOS conventions. This will impact how pixel
// centers are addressed as well as WPOS/VPOS shader behavior. The values below are the defaults for the various APIs. D3D
// allows you to control the window pixel center convention via a render state.
//
//	XBox 360 and D3D9 : 
//		PS3_SHADER_VPOS_WINDOW_ORIGIN		= CELL_GCM_WINDOW_ORIGIN_TOP
//		PS3_SHADER_VPOS_PIXEL_CENTER		= CELL_GCM_WINDOW_PIXEL_CENTER_INTEGER
//		PS3_VIEWPORT_WINDOW_ORIGIN			= CELL_GCM_WINDOW_ORIGIN_TOP
//		PS3_VIEWPORT_WINDOW_PIXEL_CENTER	= CELL_GCM_WINDOW_PIXEL_CENTER_INTEGER (unless overridden with D3DRS_HALFPIXELOFFSET)
//
//	OpenGL : 
//		PS3_SHADER_VPOS_WINDOW_ORIGIN		= CELL_GCM_WINDOW_ORIGIN_BOTTOM
//		PS3_SHADER_VPOS_PIXEL_CENTER		= CELL_GCM_WINDOW_PIXEL_CENTER_HALF
//		PS3_VIEWPORT_WINDOW_ORIGIN			= CELL_GCM_WINDOW_ORIGIN_BOTOM
//		PS3_VIEWPORT_WINDOW_PIXEL_CENTER	= CELL_GCM_WINDOW_PIXEL_CENTER_HALF
//
//	D3D10+ : 
//		PS3_SHADER_VPOS_WINDOW_ORIGIN		= CELL_GCM_WINDOW_ORIGIN_TOP
//		PS3_SHADER_VPOS_PIXEL_CENTER		= CELL_GCM_WINDOW_PIXEL_CENTER_HALF
//		PS3_VIEWPORT_WINDOW_ORIGIN			= CELL_GCM_WINDOW_ORIGIN_TOP
//		PS3_VIEWPORT_WINDOW_PIXEL_CENTER	= CELL_GCM_WINDOW_PIXEL_CENTER_HALF (unless overridden with D3DRS_HALFPIXELOFFSET)
//
#define PS3_SHADER_VPOS_WINDOW_ORIGIN		(CELL_GCM_WINDOW_ORIGIN_TOP)
#define PS3_SHADER_VPOS_PIXEL_CENTER		(CELL_GCM_WINDOW_PIXEL_CENTER_HALF)
#define PS3_VIEWPORT_WINDOW_ORIGIN			(CELL_GCM_WINDOW_ORIGIN_TOP)
#define PS3_VIEWPORT_WINDOW_PIXEL_CENTER	(CELL_GCM_WINDOW_PIXEL_CENTER_HALF)



using namespace rage::GraphicsStats;

PARAM(pixannotation,"[grcore] Enable Pix annotation via the PIXBegin()/PIXEnd() macros");
PARAM(novblank,"[grcore] Disable wait for vblank");

namespace rage
{
	NOSTRIP_PARAM(MSAA,"[grcore] Anti-aliasing (MSAA_NONE, MSAA_2xMS, MSAA_Centered4xMS, MSAA_Rotated4xMS)");
	PARAM(width,"[grcore] Set width of main render window (default is 640)");
	PARAM(height,"[grcore] Set height of main render window (default is 480)");
	PARAM(hdr,"[grcore] Set backbuffer to full 16bit float.");
	PARAM(mrt,"[grcore] Use a Multiple-Render Target to encode HDR");
	NOSTRIP_PARAM(force720,"[grcore] In the case of a <xxxx>i, Force 720p over other resolutions if the TV supports it");
	PARAM(frameLimit, "[grcore] number of vertical synchronizations to limit game to");

#if COMMAND_BUFFER_SUPPORT
	int grcCommandBufferHeapSize = 4096;	// tiny by default.
#endif
} // namespace rage

unsigned int g_EnablePixAnnotation = 0xffffffff;

DECLARE_TASK_INTERFACE(patcherspu);
DECLARE_TASK_INTERFACE(drawablespu);
DECLARE_TASK_INTERFACE(edgegeomspu);



#ifdef GCM_PF
#include <cell/gcm_pm.h>
namespace rage
{
	namespace GraphicsStats
	{
		EXT_PF_COUNTER(GCLK_PRIMITIVECOUNT);
		EXT_PF_COUNTER(GCLK_SETUP_TRIANGLES);
		EXT_PF_COUNTER(GCLK_VAB_CMD_LOAD);
		EXT_PF_COUNTER(GCLK_VAB_CMD_TRI);
	} // namespace GraphicsStats
} // namespace rage

using namespace rage::GraphicsStats;
#endif // GCM_PF

#if COMMAND_BUFFER_SUPPORT
static rage::u8 *s_CommandBufferPoolMemory;
static rage::atPoolBase<> s_CommandBufferPool(COMMAND_BUFFER_SEGMENT_SIZE);

static uint32_t *s_LastSegment;
static uint32_t s_RecordSize;
static bool s_HadCommandBufferOverrun;

// Segment size in words, minus the NextCommandBufferSegment opcode (2 words)
static const uint32_t SegmentSizeInWords = (COMMAND_BUFFER_SEGMENT_SIZE/sizeof(uint32_t))-2;

static int32_t CommandBufferNextSegment(CellGcmContextData *data,uint32_t amt)
{
	// rage::grcDisplayf("CBNS begin=%p current=%p end=%p",data->begin,data->current,data->end);
	if (amt > SegmentSizeInWords)
		Quitf("Allocation request of %u words is too large.",amt);

	// Pad size with NOP's for DMA so that NextCommandBufferSegment
	// is always at the end of the quadword
	while ((((uint32_t)data->current & 15)) != 8)
		*data->current++ = 0;

	// Insert fake SPU command to mark end of a segment
	*data->current++ = (rage::grcDevice__NextCommandBufferSegment<<1)|0x80001;
	uint32_t *nextLastSegment = data->current++;

	// get number of quadwords allocated
	uint32_t allocated = (data->current - data->begin) >> 2;

	// Store address and length of this segment at end of previous segment (or in the 
	// grcCommandBuffer pointer returned in the clone call)
	grcAssertf(!((uint32_t)data->begin & allocated),"begin is %p, allocated is %x",data->begin,allocated);
	*s_LastSegment = allocated | (uint32_t)data->begin;
	s_RecordSize += COMMAND_BUFFER_SEGMENT_SIZE;

	// Make sure there's always one free segment after this one so that we can start a recording
	// even when the buffer is full, since there's no way to indicate failure in BeginCommandBuffer.
	if (s_CommandBufferPool.GetFreeCount() <= 1) {
		// If we had an overrun, remember it and just re-use the last segment.
		s_HadCommandBufferOverrun = true;
		data->current = data->begin;
	}
	else {
		s_LastSegment = nextLastSegment;
		// Allocate next segment
		data->begin = data->current = reinterpret_cast<uint32_t*>(s_CommandBufferPool.New(0));
		// rage::grcDisplayf("%d bytes in current segment, %d segments remain, new seg at %p",allocated<<4,s_CommandBufferPool.GetFreeCount(),data->begin);
	}

	data->end = data->begin + SegmentSizeInWords;

	return 0;
}
#endif


#if DEBUG_ALLOW_CACHED_STATES_TOGGLE
bool g_DebugSetAllStates/*=false*/;
#endif

namespace rage
{

extern sysMemAllocator* g_pResidualAllocator;
extern u64 g_DipSwitches;

int grcVertexDeclaration::Release()
{
	if (--RefCount==0) {
		rage::sysMemStartTemp();
		delete this;
		rage::sysMemEndTemp();
		return 0;
	}
	else
		return RefCount;
}


CompileTimeAssert(static_cast<int>(grcmrtColorCount) == CELL_GCM_MRT_MAXCOUNT);

// From main.cpp
extern size_t g_GcmInitBufferSize;
extern size_t g_GcmMappingSize;
extern void* g_GcmHeap;
extern void* g_MainHeap;

// from task_psn.cpp
extern void* g_SpursReportArea;

class RenderTargetState
{
public:
	RenderTargetState()
		: m_renderTarget(NULL)
		, m_memoryOffset(0)
	{
	}
	RenderTargetState& operator=(const grcRenderTargetGCM* renderTarget)
	{
		m_renderTarget = const_cast<grcRenderTargetGCM*>(renderTarget);
		if (renderTarget)
		{
			m_memoryOffset = renderTarget->GetMemoryOffset();
		}
		return *this;
	}
	operator grcRenderTarget*() { return m_renderTarget; }
	operator const grcRenderTarget*() const { return m_renderTarget; }
	grcRenderTarget* operator->() { return m_renderTarget; }
	const grcRenderTarget* operator->() const { return m_renderTarget; }
	bool operator==(const RenderTargetState& state) const
	{
		bool isEqual = true;
		if (m_renderTarget && state.m_renderTarget)
		{
			 isEqual &= (m_memoryOffset == state.m_memoryOffset);
		}
		isEqual &= (m_renderTarget == state.m_renderTarget);
		return isEqual;
	}
	bool operator!=(const RenderTargetState& state) const { return !(*this == state); }
	bool operator==(const grcRenderTargetGCM* renderTarget) const
	{
		bool isEqual = true;
		if (m_renderTarget && renderTarget)
		{
			isEqual &= (m_memoryOffset == renderTarget->GetMemoryOffset());
		}
		isEqual &= (m_renderTarget == renderTarget);
		return isEqual;
	}
	bool operator!=(const grcRenderTargetGCM* renderTarget) const { return !(*this == renderTarget); }

private:
	grcRenderTargetGCM* m_renderTarget;
	u32 m_memoryOffset;
};
static RenderTargetState s_RenderTargetState[grcmrtCount];

u32 grcDevice::sm_ClipPlaneEnable = 0;
volatile u32 grcDevice::sm_KillSwitch = 0;
#if !HACK_GTA4 // no shader clip planes
static Vec4V s_ClipPlanes[RAGE_MAX_CLIPPLANES];
static Vec4V s_TransformedClipPlanes[RAGE_MAX_CLIPPLANES];
#endif // !HACK_GTA4

// With the new DEH hardware we seem to get best results from
// using a PS2 component cable.
grcDisplayWindow grcDevice::sm_CurrentWindows[NUMBER_OF_RENDER_THREADS];
grcDisplayWindow grcDevice::sm_GlobalWindow = grcDisplayWindow(1280, 720, 0, 0, 1);

MonitorConfiguration grcDevice::sm_MonitorConfig;

const char *grcDevice::sm_DefaultEffectName = "x:\\rage\\assets\\tune\\shaders\\lib\\rage_im";
grcDevice::MSAAMode grcDevice::sm_MSAA;
#if __PPU
int grcDevice::sm_LastWidth = -1;
int grcDevice::sm_LastHeight = -1;
#endif // __PPU
// static bool s_SwapImmediateIfLate;
bool grcDevice::sm_HardwareShaders = true;
bool grcDevice::sm_HardwareTransform = true;
bool grcDevice::sm_LetterBox=true;

grcCommandBuffer* g_grcCommandBuffer;
u32 g_MaxAlphaRef = 255;
CellGcmSurface g_GcmSurface;
CellGcmEnum g_DepthFormatType = CELL_GCM_DEPTH_FORMAT_FIXED;

#if __BANK
u8 g_MinRegisterCount = 2; // Used in __BANK so that we can shmoo
#endif

// For caching the m_ClipToScreen matrix state.
/*
static int s_LastWindowX = -1;
static int s_LastWindowY = -1;
static int s_LastWindowWidth = -1;
static int s_LastWindowHeight = -1;
*/
// static int s_WindowX = -1;
// static int s_WindowY = -1;
// static int s_WindowWidth = -1;
// static int s_WindowHeight = -1;

u32 LABEL_ZCULL_STATS2;
u32 LABEL_ZCULL_STATS3;
u32 LABEL_NEXT_FENCE;
u32 LABEL_COMMAND_DUMMY;
u32 LABEL_VBLANK_LOCKDOWN;
u32 LABEL_CURRENT_FRAME;

u32 grcCurrentVertexOffsets[grcVertexDeclaration::c_MaxStreams];
const grcIndexBufferGCM *grcCurrentIndexBuffer;
const grcVertexDeclaration* grcCurrentVertexDeclaration = 0;

grcEffect *g_DefaultEffect;
grcEffectVar g_DefaultSampler;

static grcEffectVar s_MsaaQuincunxSampler;
static grcEffectVar s_MsaaQuincunxAltSampler;
static grcEffectVar s_MsaaGaussianSampler;
static grcEffectVar s_MsaaPointSampler;
static grcEffectVar s_TransparentSrcSampler;
static grcEffectVar s_TransparentDstSampler;
static grcEffectVar s_TexelSize;
static grcEffectVar s_DepthPointSampler;

static grcEffectTechnique s_DefaultLit;
static grcEffectTechnique s_DefaultUnlit;
static grcEffectTechnique s_DefaultLitSkinned;
static grcEffectTechnique s_DefaultUnlitSkinned;
static grcEffectTechnique s_DefaultClear;
static grcEffectTechnique s_DefaultClearMRT2;
static grcEffectTechnique s_DefaultClearMRT3;
static grcEffectTechnique s_DefaultClearMRT4;
static grcEffectTechnique s_CopyTechnique;
static grcEffectTechnique s_CopyDepthTechnique;
static grcEffectTechnique s_CopyTransparentTechnique;
static grcEffectTechnique s_Msaa2xTransparentTechnique;
#if RAGE_ENCODEOPAQUECOLOR
static grcEffectTechnique s_Msaa2xOpaqueTechnique;
#else
static grcEffectTechnique s_Msaa2xAccuviewTechnique;
static grcEffectTechnique s_Msaa2xQuincunxTechnique;
static grcEffectTechnique s_Msaa2xQuincunxAltTechnique;
static grcEffectTechnique s_Msaa2x2TapTechnique;
#endif // RAGE_ENCODEOPAQUECOLOR
static grcEffectTechnique s_Msaa4xTechnique;
static grcEffectTechnique s_Msaa4xTransparentTechnique;

static grcBlendStateHandle s_ClearBlendStates[16];

#if !HACK_GTA4 // no shader clip planes
// static grcEffectGlobalVar s_gvClipToScreen;
static grcEffectGlobalVar s_ClipPlanesGlobalVar = grcegvNONE;
static bool s_ClipPlanesGlobalVarLookedUp = false;
#endif // !HACK_GTA4

u32 g_FrontBufferCount = 2;
grcRenderTargetGCM *g_FrontBuffer[8];
grcRenderTargetGCM *g_BackBuffer;
grcRenderTargetGCM *g_DepthBuffer;
grcRenderTargetGCM *g_DepthBackBuffer;

static CellGcmContextData g_OurGcmContext ;
__THREAD CellGcmContextData *g_grcCurrentContext;

// https://ps3.scedev.net/forums/thread/35746?pg=1#n184942

u32 g_CommandBufferUsage;

const int g_grcDefaultFloatDepthFormat = CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT;
const int g_grcDefaultFixedDepthFormat = CELL_GCM_TEXTURE_DEPTH24_D8;
int g_grcDepthFormat = g_grcDefaultFixedDepthFormat;

ASSERT_ONLY(static grcOcclusionQuery s_OcclusionQueryInProgress = 0xffffffff);
static CellGcmReportData* s_OcclusionQueryData;
static const u32 s_OcclusionQueryReportIndex = GCM_REPORT_COUNT;
static atFreeList<u8,GCM_OCCLUSION_QUERY_COUNT> s_OcclusionQueryFreeList;
static u32 s_LastMsaaResolveFrame = 0xffffffff;

static const u32 kMaxConditionalQueries = GCM_CONDITIONAL_QUERY_COUNT;
static atFreeList<u16,kMaxConditionalQueries> s_ConditionalQueryFreeList;
static const u32 s_ConditionalQueryReportIndex = s_OcclusionQueryReportIndex + GCM_OCCLUSION_QUERY_COUNT;
static const u32 s_LastReportIndex = s_ConditionalQueryReportIndex + kMaxConditionalQueries;

#define TWEAKGAMMA (__BANK)

float s_Gamma = 1.0f;
#if TWEAKGAMMA
float s_OldGamma = 1.0f;
#endif // TWEAKGAMMA

#if CELL_SDK_VERSION < 0x200002
#error "Sorry, time to upgrade!  Rage requires Cell SDK 200.002 or later."
#endif // CELL_SDK_VERSION < 0x200002

#define FRAGMENT_BRANCH_STRIP_DEBUG (__BANK)
#if FRAGMENT_BRANCH_STRIP_DEBUG
u8	s_FragmentStrippingFlags;
#endif // FRAGMENT_BRANCH_STRIP_DEBUG

PARAM(disablebranchstripping,"[grcore] Disable runtime fragment program branch stripping");


static int s_CappedFrameLock;
static volatile int32_t s_VBlankRemaining = 1;

#if !__FINAL
static int numFlipWaits = 0;
#endif

static void VBlankHandler(uint32_t)
{
	// Have the proscribed number of vblanks elapsed yet?
	// Label logic largely stolen from PhyrEngine FWCellGcmWindow.cpp code.
	if (--s_VBlankRemaining <= 0) {
		*cellGcmGetLabelAddress(LABEL_VBLANK_LOCKDOWN) = 1;
		s_VBlankRemaining = s_CappedFrameLock;
	}

#if !__FINAL
    if (cellGcmGetFlipStatus() == CELL_GCM_DISPLAY_FLIP_STATUS_WAITING)
	{
        numFlipWaits++;
		grcSetup::SetFlipWaits(numFlipWaits);
	}
    else
	{
		cellGcmResetFlipStatus();
        numFlipWaits = 0;
	}
#endif // !__FINAL
}


extern void effectFlush(CellGcmContextData *data);

int g_GcmDisableFlush;

static void rageFlush(CellGcmContextData *data)
{
	// Calling the callback forces the segment to be kicked to the SPU.
	data->callback(data, 0);
}

const int gcmSegmentCapacity = 32;	// Larger number is good now that we do bubble fence waiting.
const int gcmSegmentSize = PPU_FIFO_SEGMENT_SIZE / 4;
const int gcmSegmentSlop = 8;
static sysTaskHandle s_gcmTasks[gcmSegmentCapacity];
static int s_gcmTaskIndex;
/// static const char *s_gcmTaskName, *s_gcmTaskStart, *s_gcmTaskSize;
static uint32_t s_gcmSegments[gcmSegmentCapacity][gcmSegmentSize] ALIGNED(128);

int32_t spuCallback(CellGcmContextData *data,uint32_t /*amt*/);

#if __DEV		// For easier breakpointing, etc.
spuCmd_Any* spuAllocateCommand(CellGcmContextData *data,u8 command,u8 subcommand,u32 totalSize)
{
	grcAssertf((totalSize & 3) == 0,"bad size %d must be multple of 4",totalSize);
	grcAssertf(data->callback == spuCallback, "trying to record an SPU command unexpectedly?");

	// Make sure there's enough room including any potential alignment slop
	if ((char*)data->current + totalSize + 12 > (char*)data->end)
		data->callback(data, totalSize>>2);
	// If command is at least two quadwords, make sure it's aligned.
	// (if we had to invoke the callback we know it will already be suitably aligned)
	else if (totalSize > 31)
		ALIGN_CONTEXT_TO_16(data);

	spuCmd_Any *result = (spuCmd_Any*) data->current;
	data->current += (totalSize>>2);

	result->size = totalSize;
	result->subcommand = subcommand;
	result->command = (command<<1)|1;
	return result;
}
#endif

spuGcmState s_spuGcmState;

gpuMemCpy s_gpuMemCpy;

class PriorityInheritingMutex {
public:
	PriorityInheritingMutex() {
		sys_mutex_attribute_t attr;
		sys_mutex_attribute_initialize(attr);
		attr.attr_protocol = SYS_SYNC_PRIORITY_INHERIT;
		ASSERT_ONLY(int ret=) sys_mutex_create(&m_mutex, &attr);
		Assert(ret == CELL_OK);
	}

	~PriorityInheritingMutex() {
		ASSERT_ONLY(int ret=) sys_mutex_destroy(m_mutex);
		Assert(ret == CELL_OK);
	}

	void Lock() {
		ASSERT_ONLY(int ret=) sys_mutex_lock(m_mutex, 0);
		Assert(ret == CELL_OK);
	}

	void Unlock() {
		ASSERT_ONLY(int ret=) sys_mutex_unlock(m_mutex);
		Assert(ret == CELL_OK);
	}

private:
	sys_mutex_t m_mutex;
};

static PriorityInheritingMutex s_gcmTaskMutex;

class TaskMutexLock {
public:
	TaskMutexLock() {
		s_gcmTaskMutex.Lock();
	}
	~TaskMutexLock() {
		s_gcmTaskMutex.Unlock();
	}
};


gpuMemCpy::gpuMemCpy() {
	m_atomic64 = 0;
}

bool gpuMemCpy::Push(void *dest,const void *src,size_t bytes,u32 &setToNonZeroWhenDone) {

	SYS_CS_SYNC(m_ppuPushCritSec);

	u64 atomic64 = __ldarx(&m_atomic64);
	u64 putIdx   = (atomic64 >> 48) & 0xffff;
	u64 getIdx   = (atomic64 >> 32) & 0xffff;
	Assert(putIdx < MaxCopy);

	// Spin until there is space in the fifo
	if (Unlikely((putIdx+1)%MaxCopy == getIdx)) {
		return false;
	}

	// Initialize the job entry in the fifo
	Job &job = m_buffer[putIdx];
	if (gcm::IsLocalPtr(dest)) {
		job.DestOffset = gcm::LocalOffset(dest);
		job.DestLoc = CELL_GCM_LOCATION_LOCAL;
	}
	else if (gcm::IsMainPtr(dest)) {
		job.DestOffset = gcm::MainOffset(dest);
		job.DestLoc = CELL_GCM_LOCATION_MAIN;
	}
	else {
		Quitf("Illegal gpuMemCpy dest addr %p",dest);
	}

	if (gcm::IsLocalPtr(src)) {
		job.SrcOffset = gcm::LocalOffset(src);
		job.SrcLoc = CELL_GCM_LOCATION_LOCAL;
	}
	else if (gcm::IsMainPtr(src)) {
		job.SrcOffset = gcm::MainOffset(src);
		job.SrcLoc = CELL_GCM_LOCATION_MAIN;
	}
	else {
		Quitf("Illegal gpuMemCpy src addr %p",src);
	}
	job.Bytes = bytes;
	job.DoneOffset = gcm::MainOffset(&setToNonZeroWhenDone);

	// lwsync required to ensure that job has been updated before putIdx is.
	__lwsync();

	putIdx = (putIdx+1)%MaxCopy;

	for (;;) {
		atomic64 = (atomic64&0x0000ffffffffffffuLL) | (putIdx<<48);
		if (Likely(__stdcx(&m_atomic64, atomic64))) {
			break;
		}
		atomic64 = __ldarx(&m_atomic64);
	}

	// If there is no pending drawablespu jobs after we added the memcpy job to
	// the fifo, then kick another drawablespu job.  Also test that we are the
	// first to be adding something to the fifo here, otherwise there is a small
	// race condition where two threads issuing a gpu memcpy could both try to
	// create a drawablespu job, causing the second one through to block for a
	// while.
	const u64 pending = (atomic64 >> 16) & 0xffff;
	getIdx = (atomic64 >> 32) & 0xffff;
	if (Unlikely(!pending && (putIdx+MaxCopy-getIdx)%MaxCopy==1)) {
		// Kick drawablespu with a dummy gcm context with no commands.  Using a
		// dummy context means that we won't cause issues if the render thread
		// is part way through writing to g_OurGcmContext.
		u32 scratch[4] ALIGNED(128);
		CellGcmContextData ctx;
		ctx.begin = ctx.current = ctx.end = scratch;
		spuCallback(&ctx, 0);
	}

	return true;
}

bool gpuMemCpy::IsEmpty() const {
	const u64 atomic64 = m_atomic64;
	const u64 putIdx   = (atomic64 >> 48) & 0xffff;
	const u64 getIdx   = (atomic64 >> 32) & 0xffff;
	return putIdx == getIdx;
}

void gpuMemCpy::IncPendingDrawablespuJobs() {
	u64 atomic64;
	do {
		atomic64 = __ldarx(&m_atomic64);
		const u64 inc = atomic64 + 0x10000;
		atomic64 = (atomic64&0xffffffff0000ffffuLL) | (inc&0x00000000ffff0000uLL);
	} while (Unlikely(!__stdcx(&m_atomic64, atomic64)));
}

bool grcDevice::GpuMemCpy(void *dest,const void *src,size_t bytes,u32 &setToNonZeroWhenDone)
{
	AssertMsg(!GCM_CONTEXT, "GpuMemCpy - if you're already in the render thread, just do the damned copy yourself!");
	// Because we do fixed-sized blits, we're subject to this hard limit (just under four megabytes).
	// See TransferData in \usr\local\300_001\cell\host-common\include\cell\gcm\gcm_implementation_sub.h.
	TrapGT(bytes,0x3fffffU);

	setToNonZeroWhenDone = 0;
	return s_gpuMemCpy.Push(dest,src,bytes,setToNonZeroWhenDone);
}

// const int blitCommandSize = 3 + 9 + 2;

#if !__FINAL
PARAM(hexdumpgcm,"Enable hex dumps of gcm command lists (use -toggleN=hexdumpgcm)");
static bool s_DumpThisFrame;
#endif


int32_t spuCallback(CellGcmContextData *data,uint32_t /*amt*/)
{
	// This function can be called from multiple threads now
	// (gpuMemCpy::Push()), so must ensure that only instance is running at a
	// time.
	TaskMutexLock lock;

	// If the packet is empty and there's no blit work, return.  Note that
	// another thread may be updating s_gpuMemCpy at the current time, but that
	// is ok.  At least one of the two threads will end up kicking a drawablespu
	// job.
	if (data->current == data->begin && s_gpuMemCpy.IsEmpty())
		return CELL_OK;

#if RAGETRACE && 0
	s_spuGcmState.SpuTrace = (u32)g_pfSpuTrace;
		
	extern uint32_t (*g_pIdFromName)(const char*);
	s_spuGcmState.traceId = g_pIdFromName("EdgeGeom");
#endif	

	s_gpuMemCpy.IncPendingDrawablespuJobs();

#if GCM_REPLAY 	
	IS_CAPTURING_REPLAY_DECL
#if HACK_GTA4
	s_spuGcmState.isGPADCapturing = IS_CAPTURING_REPLAY;
#endif
#endif
	ALIGN_CONTEXT_TO_16(data);

// from RDR
#if !__FINAL
	if (data->current > data->end) {
		OUTPUT_ONLY(gcm::HexDump("error overrun",__LINE__,"error overrun",data->begin,data->current));
		Quitf("GCM segment overrun (%p > %p), very bad news.",data->current,data->end);
	}
#endif

	u32 usage = (char*)data->current - (char*)data->begin;
	g_CommandBufferUsage += usage;
// from RDR

	// Fire off new task; make sure DMA buffer is 128-byte-aligned for EDGE stall hole fill.
	sysTaskParameters task;
	grcAssertf(!((uint32_t)data->begin & 127),"unaligned gcm segment %p",data->begin);
	task.Input.Data = data->begin;
	task.Input.Size = usage;

#if !__FINAL
	if (s_DumpThisFrame)
		gcm::HexDump("spuCallback",__LINE__,"fifo segment",data->begin,data->current);
#endif

	task.Output.Data = &s_spuGcmState;
	task.Output.Size = sizeof(s_spuGcmState);

	task.UserData[0].asPtr = &s_spuGcmState;
	task.UserData[1].asInt = grcEffect::GetSpuStateCounts();
	task.UserData[2].asInt = grcEffect::GetGlobalVarCount();
	task.SpuStackSize = (__BANK? 16 : 8) * 1024;		// picker needs extra stack space for some reason
	size_t codesize = ((size_t)SPURS_JOB_SIZE(drawablespu) + 1023) & ~1023;
	task.Scratch.Size = kMaxSPUJobSize - PPU_FIFO_SEGMENT_SIZE - ((task.Output.Size + 1023) & ~1023) - codesize - task.SpuStackSize;

	// Make sure we have at least 90k of scratch remaining
	task.UserData[3].asInt = task.Scratch.Size - (90*1024);
	// Except that the effect cache cannot fall below 22k.  (We're just barely below that at present!)
	if (task.UserData[3].asInt < 22*1024)
		task.UserData[3].asInt = 22*1024;
	// But clamp the effect cache size at 64k so we have extra scratch space on shipping builds as a safety net.
	else if (task.UserData[3].asInt > 64*1024)
		task.UserData[3].asInt = 64*1024;
	task.UserDataCount = 4;

	// Launch on its own job chain
	//
	// When called from gpuMemCpy::Push(), we don't actually increment
	// s_gcmTaskIndex, so we need to wait on task completion before continuing.
	// Since gpuMemCpy::Push only call spuCallback when there are no drawablespu
	// jobs pending, waiting on a task manager handle should not block for long.
	if (s_gcmTasks[s_gcmTaskIndex])
		sysTaskManager::Wait(s_gcmTasks[s_gcmTaskIndex]);
#if SPU_GCM_FIFO > 2
	s_gcmTasks[s_gcmTaskIndex] = sysTaskManager::Create(TASK_INTERFACE(drawablespu),task,sysTaskManager::SCHEDULER_GRAPHICS_MAIN);
#else
	s_gcmTasks[s_gcmTaskIndex] = sysTaskManager::Create(TASK_INTERFACE(patcherspu),task,sysTaskManager::SCHEDULER_GRAPHICS_MAIN);
#endif
	// Lower-level code generally fails if the task didn't start, but we're getting a null task somehow B*639950
	Assert(s_gcmTasks[s_gcmTaskIndex]);

	// Only increment s_gcmTaskIndex if there is actually some command buffer.
	// The case we care about is when called from gpuMemCpy::Push().  At the end
	// of frame, may also have usage==0, so don't use that as a check, instead
	// check data->begin!=data->end.
	if (data->begin != data->end) {
		if (++s_gcmTaskIndex == gcmSegmentCapacity)
			s_gcmTaskIndex = 0;

		// Wait for previous use of this segment to finish.
		// We need to do it now before we start filling the buffer since the SPU might not have even read it yet.
		// TODO: We could potentially hack the data so that current==end and have a callback do this wait, so
		// that we don't really stall until the next command.  Not sure this is worth the trouble though.
		if (s_gcmTasks[s_gcmTaskIndex]) {
			sysTaskManager::Wait(s_gcmTasks[s_gcmTaskIndex]); // from RDR, TODO was sleepTime=100
			s_gcmTasks[s_gcmTaskIndex] = NULL;
		}

		// Note that since this isn't a real gcm buffer we don't need to issue a jump here.
		// But we want to leave some slop on the end for the jump we'll need in the real fifo,
		// along with 16 bytes for the SetWriteTextureLabel command we use at the start of
		// every segment, so leave eight words free after the end.
		data->begin = data->current = &s_gcmSegments[s_gcmTaskIndex][0];
		data->end = &s_gcmSegments[s_gcmTaskIndex][gcmSegmentSize-gcmSegmentSlop];
	}

	return CELL_OK;
}

static bool s_IsWideScreen, s_IsHiDef, s_IsInterlaced;

u32 g_VramFifoSize = 1024*1024;
extern float g_PixelAspect;

#if !__FINAL || GRCDBG_IMPLEMENTED
static u32 LABEL_FAULT_STACK;
static u32 FAULT_STACK_SP;
static const u32 LABEL_FAULT_STACK_SIZE=spuGcmState::FaultStackSize;

void gcmFaultHandler(uint32_t) {
	grcWarningf("GCM fault detected.  Context:");		// Not an error because these are sometimes spurious
	for (int i=0; i<LABEL_FAULT_STACK_SIZE; i++) {
		const char *label = *(const char**)cellGcmGetLabelAddress(LABEL_FAULT_STACK+i);
		if (label)
			grcWarningf("%d. %s",i,label);
	}
	// s_TaskLog.Dump();
	/* grcErrorf("Memory dump:");
	for (int *addr = (int*)0x00400000 ; addr < (int*) 0x00402000; addr += 8)
		grcDisplayf("%p: %08x %08x %08x %08x - %08x %08x %08x %08x",
			addr,addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7]); */
}
#endif

#if !__FINAL
static sysTimer s_Watchdog;

// if this is set, watchdog won't abort the game and dump callstacks on hangs 
// (can be toggled for cases we expect to hang like writing debug files to network, bugstar, etc)
bool g_DontAbortWatchdog = true; // turn this secondary hang handler off

static void gcmWatchdog(void*) {
	for (;;) {
		if (s_Watchdog.GetTime() > 15.0f) {
#if GCM_REPLAY
			IS_CAPTURING_REPLAY_DECL
			if (!IS_CAPTURING_REPLAY)
#endif
				// Don't trigger an error if we're stuck on a message box, or we haven't even
				// shown the first frame yet (ie testers with long load times but no loading screen)
				if (!fiIsShowingMessageBox && !sysStack::HasExceptionBeenThrown() && GRCDEVICE.GetFrameCounter()) {
					gcmFaultHandler(0);
#if !__NO_OUTPUT
					u32 getAddr = gcm::g_ControlRegister->get;
					u32 putAddr = gcm::g_ControlRegister->put;
					u32 *getPtr = (u32*)(gcm::g_IoAddress + getAddr);
					grcErrorf("watchdog failed? get=0x%x *get=0x%x put=0x%x%s", getAddr, *getPtr, putAddr, getAddr==putAddr?" [check for drawablespu crash]":"");
					grcErrorf("current fault stack [%s]",s_spuGcmState.FaultContext);
#endif
#if !__FINAL
					if(!snIsTunerRunning() && !g_DontAbortWatchdog)
					{
						Displayf("The watchdog thinks the game is hung so we're going to abort and dump out callstacks for all threads.");
						sysIpcTriggerAllCallstackDisplay();
				}
#endif // !__FINAL
				}
			s_Watchdog.Reset();
		}
		sysIpcSleep(1000);
	}
}
#endif

#if !RAGE_ENCODEOPAQUECOLOR
enum Msaa2xMode
{
	kAccuview,
	kQuincunx,
	kQuincunxAlt,
	k2Tap,

	kMsaa2xModeCount
};

static const char* kMsaa2xModeNames[] =
{
	"Accuview",
	"Quincunx",
	"Quincunx Alt.",
	"2 Tap"
};
CompileTimeAssert(NELEM(kMsaa2xModeNames) == kMsaa2xModeCount);

#if !__BANK
const
#endif // !__BANK
#if HACK_GTA4
static int s_Msaa2xMode = kAccuview;
#else // HACK_GTA4
static int s_Msaa2xMode = k2Tap;
#endif // HACK_GTA4
#if !__BANK
const
#endif // !__BANK
static float s_MsaaShiftPlusMinus = 0.0f;
#endif // !RAGE_ENCODEOPAQUECOLOR

#if !__FINAL
PARAM(commandRing,"[grcore] Set command ring buffer size, in kilobytes (default and max is 1024k)");
PARAM(ucodeRing,"[grcore] Set ucode cache size, in kilobytes (default 512k)");
PARAM(ucodeType,"[grcore] Set ucode cache memory type (0=local or 1=main, default local)");
PARAM(gcmWatchdog,"[grcore] Force gcm watchdog thread to run even when debugger is detected.");
PARAM(noGcmWatchdog,"[grcore] Disable gcm watchdog thread (useful for automation).");
#endif

#if __ASSERT
static bool AVConfModuleLoaded = false;
#endif // __ASSERT

static bool LoadAVConfModule()
{
	Assert(AVConfModuleLoaded == false);
	if( cellSysmoduleLoadModule(CELL_SYSMODULE_AVCONF_EXT) != CELL_OK ) {
		grcErrorf("grcDevice - error loading AVCONF_EXT");
		return false;
	}

#if __ASSERT
	AVConfModuleLoaded = true;
#endif // __ASSERT
	
	return true;
}

static void UnLoadAVConfModule()
{
	Assert(AVConfModuleLoaded == true);
	if (cellSysmoduleUnloadModule(CELL_SYSMODULE_AVCONF_EXT) != CELL_OK) {
		grcErrorf("grcDevice - error unloading AVCONF_EXT");
		return;
	}
	
#if __ASSERT
	AVConfModuleLoaded = false;
#endif // __ASSERT
}

static void SetGamma(float gamma)
{
	Assert(AVConfModuleLoaded == true);
	cellVideoOutSetGamma(CELL_VIDEO_OUT_PRIMARY, gamma);
}

bool grcDevice::sm_OutputIsFullRange = false;
#if !__FINAL
extern sysTimer g_grcLoadTimer;
#endif

static bool GetOutputRangeIsFull()
{
	Assert(AVConfModuleLoaded == true);
	CellVideoOutDeviceInfo info;
	if( cellVideoOutGetDeviceInfo(CELL_VIDEO_OUT_PRIMARY, 0, &info) == CELL_OK ) {	
		return info.rgbOutputRange == CELL_VIDEO_OUT_RGB_OUTPUT_RANGE_FULL;
	}
	
	return false;
}

OUTPUT_ONLY(static float s_HardwareRefreshRate);

void grcDevice::InitClass(bool /*inWindow*/, bool /*topMost*/)
{
	sm_CurrentWindows[g_RenderThreadIndex].Init(1280, 720, 0, 0, 1);
	SetFrameLock(sm_CurrentWindows[g_RenderThreadIndex].uFrameLock, true);

	g_grcDepthFormat = g_grcDefaultFloatDepthFormat;

	memset(s_RenderTargetState, 0, sizeof(s_RenderTargetState));

	LoadAVConfModule();
	SetGamma(s_Gamma);
	sm_OutputIsFullRange = GetOutputRangeIsFull();
	UnLoadAVConfModule();
	
	sm_FrameCounter = sm_SyncCounter = 0;

	// Must call GcmInit after setting up our heaps in case we're using the HUD
	// "If this function does not return, check the video output configuration of the system software and Game Output Resolution configuration. 
	// If the combination of the video output configuration and Game Output Resolution configuration is invalid, the processing on the system 
	// software side that is called within this function will not complete, and it will appear as though operation has stopped for this function."
	if (!g_GcmInitBufferSize)
		Quitf("FIFO in VRAM not supported.");
	if (cellGcmInit(g_GcmInitBufferSize, g_GcmMappingSize, g_GcmHeap))
		Quitf("Cannot init GCM ");

	if (cellGcmMapEaIoAddressWithFlags((void*)g_SpursReportArea, CELL_GCM_REPORT_IO_ADDRESS_BASE, MAPPED_GCM_AREA_SIZE, CELL_GCM_IOMAP_FLAG_STRICT_ORDERING /* is this necessary? */))
		Quitf("Cannot init Special Place");
	grcDisplayf("Mapped 1M rsx label area.");

	// NOTE: This assumes we will always have residual memory!
	size_t reserved = GCM_REPORT_AREA_SIZE + SPU_SPURS_AREA_SIZE;
	size_t residualAddr = (size_t)g_SpursReportArea + reserved;
	sysMemAllocator *ra = rage_placement_new ((void*)residualAddr) sysMemSimpleAllocator((char*)residualAddr + sizeof(sysMemSimpleAllocator), MAPPED_GCM_AREA_SIZE - reserved - sizeof(sysMemSimpleAllocator), false);
	ra->SetQuitOnFail(false);
	printf("[POOL] Secondary residual allocator has %u bytes available at creation.\n",ra->GetMemoryAvailable());
	g_pResidualAllocator = ra;
#if RAGE_TRACKING
	if ( diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasPlatform())
	{
		diagTracker::GetCurrent()->InitHeap("Residual Allocator 2", g_pResidualAllocator->GetHeapBase(), g_pResidualAllocator->GetHeapSize());
	}
#endif

#if !__FINAL
	if (snIsTunerRunning()) {
		sys_addr_t addr;
		if (sys_memory_allocate(1024*1024, SYS_MEMORY_PAGE_SIZE_1M, &addr) != CELL_OK)
			Quitf("Unable to allocate Tuner RSX buffer");
		if (cellGcmMapEaIoAddress((void*)addr, 0x0ef00000, 1024*1024) != CELL_OK)
			Quitf("Unable to map Tuner RSX buffer");
		snRegisterGcmFlush(rageFlush);
	}
#endif

	// THIS IS UGLY AS HELL.  It used to live in main.h (where it was still pretty ugly) but we
	// need to defer cellGcmInit initialization time until after global ctors have run.
	CellGcmConfig config;
	cellGcmGetConfiguration(&config);

	// grcDisplayf("current usage %p %p %p",gCellGcmCurrentContext->begin,gCellGcmCurrentContext->current,gCellGcmCurrentContext->end);

	sysMemSet(&s_spuGcmState, 0x00, sizeof(s_spuGcmState));
	// Initialize ring buffer (past the reserved part and the part the library already built)
	s_spuGcmState.Control = cellGcmGetControlRegister();

#if USE_PACKED_GCMTEX
	s_spuGcmState.PackedTextureArray = rage_new PackedCellGcmTexture[pgMaxHandles];
	memset(s_spuGcmState.PackedTextureArray, 0, sizeof(PackedCellGcmTexture) * pgMaxHandles);
#endif

	// Bootstrap the real memory fifo with one segment already allocated.
	int commandFifoSize = g_GcmInitBufferSize >> 10;
#if !__FINAL
	PARAM_commandRing.Get(commandFifoSize);
#endif
	commandFifoSize <<= 10;
#if !__FINAL
	if ((u32)commandFifoSize > g_GcmInitBufferSize)
		Quitf("Max -commandRing is %d, raise GCM_BUFFER_SIZE in toplevel code.",g_GcmInitBufferSize>>10);
#endif

	s_spuGcmState.CommandFifo.Init("Command FIFO", (char*)g_GcmHeap + SPU_FIFO_SEGMENT_SIZE, commandFifoSize - SPU_FIFO_SEGMENT_SIZE, gcm::RsxSemaphoreRegistrar::Allocate(1), FIFOTAG, SPU_FIFO_SEGMENT_SIZE*2);
	s_spuGcmState.NextNextSegment = (char*)g_GcmHeap + 2*SPU_FIFO_SEGMENT_SIZE;
	s_spuGcmState.NextSegment = (char*)g_GcmHeap + SPU_FIFO_SEGMENT_SIZE;
	s_spuGcmState.NextSegmentOffset = gcm::MainOffset(s_spuGcmState.NextSegment);
	s_spuGcmState.CurrentFragmentOffset = ~0U;

	// Issue a jump into our ring buffer and flush so that the SPU will see the get pointer in the correct range
	cellGcmSetJumpCommand(gCellGcmCurrentContext,s_spuGcmState.CommandFifo.GetRingBeginOffset());
	s_spuGcmState.Control->put = s_spuGcmState.CommandFifo.GetRingBeginOffset();

	// grcDisplayf("control at %p [put, get, ref]",s_spuGcmState.Control);
	// Take over the global context, pointing it at our first fake SPU context
	gCellGcmCurrentContext->begin = gCellGcmCurrentContext->current = &s_gcmSegments[0][0];
	gCellGcmCurrentContext->end = &s_gcmSegments[0][gcmSegmentSize-gcmSegmentSlop];
	gCellGcmCurrentContext->callback = spuCallback;

	s_spuGcmState._binary_edgegeomspu_job_elf_start = SPURS_JOB_START(edgegeomspu);
	s_spuGcmState._binary_edgegeomspu_job_elf_size = SPURS_JOB_SIZE(edgegeomspu);

	gcm::g_ControlRegister = s_spuGcmState.Control; // cellGcmGetControlRegister();

	g_OurGcmContext = *gCellGcmCurrentContext;
	g_grcCurrentContext = &g_OurGcmContext;
	
	// Make sure nobody tries to use the default context.
	gCellGcmCurrentContext = NULL;

	// Do not need to worry about the RSX texture over fetch bug here
	// (https://ps3.scedev.net/technotes/view/1127), since the top of memory is
	// reserved for render targets (and we explicitly handle the issue there).
	// An S3TC or linear-format texture at the top of memory may overfetch by 16
	// bytes.
	//
	// Even if resourcePhysical did go to the top of memory it wouldn't be much
	// of an issue, since our resource virtual page size is 5504 bytes it's
	// unlikely this is ever a problem in practice because even if we somehow
	// exactly filled the memory space the chances of a texture actually filling
	// that page size is pretty tiny.
	//
	// But the reason why render targets are at the top is to work around
	// another RSX hardware problem.  RSX local memory (particuarly on old
	// systems) tends to get one bit corruption.  If this happens to an index
	// buffer, and the corresponding vertex buffer is at the top of memory it
	// can crash.
	//
	sysMemAllocator* gamePhysical = sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_GAME_PHYSICAL);
	sysMemAllocator* resourcePhysical = sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_RESOURCE_PHYSICAL);
	gamePhysical->SetHeapBase(config.localAddress);
	resourcePhysical->SetHeapBase((char*)config.localAddress + gamePhysical->GetHeapSize());
#if RAGE_TRACKING
	diagTracker* t = diagTracker::GetCurrent();
	if (t) 
    {
		if (sysMemVisualize::GetInstance().HasGame())
			t->InitHeap("Game Physical", gamePhysical->GetHeapBase(), gamePhysical->GetHeapSize());

		if (sysMemVisualize::GetInstance().HasResource())
			t->InitHeap("Resource Physical", resourcePhysical->GetHeapBase(), resourcePhysical->GetHeapSize());

		if (sysMemVisualize::GetInstance().HasStreaming())
		{
			void* address = (void*) (0x10000000 ^ (size_t) resourcePhysical->GetHeapBase());
			t->InitHeap("Streaming Physical", address, resourcePhysical->GetHeapSize());
		}
	}
#endif

	if (!g_GcmInitBufferSize) {
		// Make sure that FIFO was allocated where we thought it was.
		void *fifo = physical_new(g_VramFifoSize,128);
		if (fifo != config.localAddress)
			Quitf("FIFO didn't end up where I expected.");
	}

	// read the current video status
	// INITIAL DISPLAY MODE HAS TO BE SET BY RUNNING SETMONITOR.SELF
	CellVideoOutState videoState;
	CellVideoOutResolution resolution;
	// get video output state
	OUTPUT_ONLY(int err);
	if ((OUTPUT_ONLY(err =) cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &videoState)) != CELL_OK)
		Quitf("cellVideoOutGetState failed, code %x",err);
	if ((OUTPUT_ONLY(err =) cellVideoOutGetResolution(videoState.displayMode.resolutionId, &resolution)) != CELL_OK)
		Quitf("cellVideoOutGetResolution failed, code %x (in XMB Settings column, check Display Settings against *Debug Settings / Game Output Resolution (Debugger), and make sure 4:3 or 16:9 matches too)",err);

	sm_CurrentWindows[g_RenderThreadIndex].uWidth = resolution.width;
	sm_CurrentWindows[g_RenderThreadIndex].uHeight = resolution.height;

	PARAM_width.Get(sm_CurrentWindows[g_RenderThreadIndex].uWidth);
	PARAM_height.Get(sm_CurrentWindows[g_RenderThreadIndex].uHeight);

#if !__NO_OUTPUT
	s_HardwareRefreshRate = 60.0f;
	if (videoState.displayMode.refreshRates & CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ)
		s_HardwareRefreshRate = 59.94f;
	else if (videoState.displayMode.refreshRates & CELL_VIDEO_OUT_REFRESH_RATE_60HZ)
		s_HardwareRefreshRate = 60.0f;
	else if (videoState.displayMode.refreshRates & CELL_VIDEO_OUT_REFRESH_RATE_50HZ)
		s_HardwareRefreshRate = 50.0f;
	else if (videoState.displayMode.refreshRates & CELL_VIDEO_OUT_REFRESH_RATE_30HZ)
		s_HardwareRefreshRate = 30.0f;
#endif // __NO_OUTPUT

	s_IsInterlaced = (videoState.displayMode.scanMode == CELL_VIDEO_OUT_SCAN_MODE_INTERLACE);

#if !__DEV
	// If we got 1080i instead of 1080p, and we also support 720p, switch to that.
	if (sm_CurrentWindows[g_RenderThreadIndex].uHeight == 1080 && s_IsInterlaced && 
		cellVideoOutGetResolutionAvailability(	CELL_VIDEO_OUT_PRIMARY,
			CELL_VIDEO_OUT_RESOLUTION_720,CELL_VIDEO_OUT_ASPECT_16_9,0)) {
		grcDisplayf("[GRAPHICS] Favoring 720p over 1080i");
		sm_CurrentWindows[g_RenderThreadIndex].uWidth=1280;
		sm_CurrentWindows[g_RenderThreadIndex].uHeight=720;
		s_IsInterlaced = false;
	}
#endif

	s_IsWideScreen = videoState.displayMode.aspect == CELL_VIDEO_OUT_ASPECT_16_9;

	int resId = 0;
#if !__DEV
	if( (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1920 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) ||
		(sm_CurrentWindows[g_RenderThreadIndex].uWidth==1600 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) ||
		(sm_CurrentWindows[g_RenderThreadIndex].uWidth==1440 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) ||
		(sm_CurrentWindows[g_RenderThreadIndex].uWidth==1280 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) ||
		(sm_CurrentWindows[g_RenderThreadIndex].uWidth==960 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) )
	{
		// We only support 960x1080 so we force it, no matter what the app tells us...
		sm_CurrentWindows[g_RenderThreadIndex].uWidth = 960;
		sm_CurrentWindows[g_RenderThreadIndex].uHeight = 1080;
		resId = CELL_VIDEO_OUT_RESOLUTION_960x1080;
		g_PixelAspect = 1920.0f/960.0f;
	}
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1280 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==720)
		resId = CELL_VIDEO_OUT_RESOLUTION_720;
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==720 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==480) {
		resId = CELL_VIDEO_OUT_RESOLUTION_480;
	}
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==720 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==576) {
		resId = CELL_VIDEO_OUT_RESOLUTION_576;
	}
	else
		Quitf("Unsupported video resolution: %d by %d",sm_CurrentWindows[g_RenderThreadIndex].uWidth,sm_CurrentWindows[g_RenderThreadIndex].uHeight);

#else // __DEV
	if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1920 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080)
		resId = CELL_VIDEO_OUT_RESOLUTION_1080;
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1280 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==720)
		resId = CELL_VIDEO_OUT_RESOLUTION_720;
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==720 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==480) {
		resId = CELL_VIDEO_OUT_RESOLUTION_480;
	}
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==720 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==576) {
		resId = CELL_VIDEO_OUT_RESOLUTION_576;
	}
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1600 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) {
		resId = CELL_VIDEO_OUT_RESOLUTION_1600x1080;
		g_PixelAspect = 1920.0f/1600.0f;
	}
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1440 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) {
		resId = CELL_VIDEO_OUT_RESOLUTION_1440x1080;
		g_PixelAspect = 1920.0f/1440.0f;
	}
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1280 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) {
		resId = CELL_VIDEO_OUT_RESOLUTION_1280x1080;
		g_PixelAspect = 1920.0f/1280.0f;
	}
	else if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==960 && sm_CurrentWindows[g_RenderThreadIndex].uHeight==1080) {
		resId = CELL_VIDEO_OUT_RESOLUTION_960x1080;
		g_PixelAspect = 1920.0f/960.0f;
	}
	else
		Quitf("Unsupported video resolution: %d by %d",sm_CurrentWindows[g_RenderThreadIndex].uWidth,sm_CurrentWindows[g_RenderThreadIndex].uHeight);
#endif // !__DEV

	s_IsHiDef = sm_CurrentWindows[g_RenderThreadIndex].uHeight > 576;

	g_WindowWidth = sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	g_WindowHeight = sm_CurrentWindows[g_RenderThreadIndex].uHeight;
	g_InvWindowWidth = 1.0f / float(sm_CurrentWindows[g_RenderThreadIndex].uWidth);
	g_InvWindowHeight = 1.0f / float(sm_CurrentWindows[g_RenderThreadIndex].uHeight);

	grcDisplayf("[GRAPHICS] Output resolution is %d by %d, %5.2fHz.",sm_CurrentWindows[g_RenderThreadIndex].uWidth,sm_CurrentWindows[g_RenderThreadIndex].uHeight,s_HardwareRefreshRate);

	sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex];

	gcm::Init();

#if !__FINAL && 0
	cellGcmInitPerfMon();
#endif

	grcWindow w;
	SetWindow(w);

	if (PARAM_hdr.Get())
	{
		grcWarningf("-hdr is not compatible with -MSAA... disabling multisampling");
		GRCDEVICE.SetMSAA(MSAA_None);
	}
	else
	{
		const char *Msaa = "MSAA_2xMS";
		PARAM_MSAA.Get(Msaa);

		if (!stricmp(Msaa, "MSAA_2xMS"))
		{
			GRCDEVICE.SetMSAA(MSAA_2xMS);
		}
		else if (!stricmp(Msaa, "MSAA_Centered4xMS"))
		{
			GRCDEVICE.SetMSAA(MSAA_Centered4xMS);
		}
		else if (!stricmp(Msaa, "MSAA_Rotated4xMS"))
		{
			GRCDEVICE.SetMSAA(MSAA_Rotated4xMS);
		}
		else
		{
			GRCDEVICE.SetMSAA(MSAA_None);
		}
	}

	// create pre-fab render targets
	grcTextureFactory::CreatePagedTextureFactory();

	// make the pre-fabed front buffers display buffers
	for (u32 i = 0; i < g_FrontBufferCount; ++i)
	{
		// g_FrontBuffer is a prefabed render target
		CellGcmTexture* CellTexture = static_cast<CellGcmTexture*> (g_FrontBuffer[i]->GetTexturePtr());

		// register as display output buffer
		GCM_DEBUG(cellGcmSetDisplayBuffer(i, CellTexture->offset,
											CellTexture->pitch,
											CellTexture->width,
											CellTexture->height));
	}

	// Allocate ucode FIFO after framebuffers so screenshots still work
	// (and an offset of zero is no longer a valid ucode address, for clarity)
	// Keeping it at 1MB as reducing this introduces a lot of drawable SPU waits
	// on gcmAllocateVRAM. Increasing this further does not make any difference
	int FragmentFifoSize = 1024;
	int fragmentLocation = CELL_GCM_LOCATION_LOCAL;
#if !__FINAL
	PARAM_ucodeRing.Get(FragmentFifoSize);
	PARAM_ucodeType.Get(fragmentLocation);
#endif
	FragmentFifoSize <<= 10;
	s_spuGcmState.FragmentCacheLocation = fragmentLocation;
	s_spuGcmState.FragmentFifo.Init(
		"Fragment FIFO",
		s_spuGcmState.FragmentCacheLocation == CELL_GCM_LOCATION_MAIN? rage_aligned_new(128) char[FragmentFifoSize] : (char*) physical_new(FragmentFifoSize,128),
		FragmentFifoSize,gcm::RsxSemaphoreRegistrar::Allocate(1), FIFOTAG);

	CellVideoOutConfiguration videocfg;
	memset(&videocfg, 0, sizeof(CellVideoOutConfiguration));
	videocfg.resolutionId = resId;
	videocfg.format = PARAM_hdr.Get()? CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT : CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8B8G8R8;
	videocfg.pitch = static_cast<CellGcmTexture*> (g_FrontBuffer[0]->GetTexturePtr())->pitch;

	//
	// change video output
	//
	u32 ret = cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, // video output destination
		&videocfg, NULL, 1);
	if (ret != CELL_OK)
		Quitf("cellVideoOutConfigure() failed. (0x%x) (make sure the resolution you explicitly requested (id=%d, %dx%d) is enabled in Display Settings in the XMB)\n", ret, resId, sm_CurrentWindows[g_RenderThreadIndex].uWidth, sm_CurrentWindows[g_RenderThreadIndex].uHeight);

#if !__FINAL
	// Supposedly loading time is measured from the point at which we change video resolution.
	g_grcLoadTimer.Reset();
#endif

	grcEffect::InitClass();
	grcInitQuads();
	grcStateBlock::InitClass();

	if (sm_DefaultEffectName)
	{
		g_DefaultEffect = grcEffect::Create(sm_DefaultEffectName);
		if (!g_DefaultEffect)
			Quitf("Unable to create default effect '%s', cannot continue.",sm_DefaultEffectName);

		s_DefaultLit = g_DefaultEffect->LookupTechnique("draw");
		s_DefaultUnlit = g_DefaultEffect->LookupTechnique("unlit_draw");
		s_DefaultLitSkinned = g_DefaultEffect->LookupTechnique("drawskinned");
		s_DefaultUnlitSkinned = g_DefaultEffect->LookupTechnique("unlit_drawskinned");

		s_DefaultClear = g_DefaultEffect->LookupTechnique("Clear");
		s_DefaultClearMRT2 = g_DefaultEffect->LookupTechnique("ClearMrt2");
		s_DefaultClearMRT3 = g_DefaultEffect->LookupTechnique("ClearMrt3");
		s_DefaultClearMRT4 = g_DefaultEffect->LookupTechnique("ClearMrt4");

		g_DefaultSampler = g_DefaultEffect->LookupVar("DiffuseTex");
		s_MsaaQuincunxSampler = g_DefaultEffect->LookupVar("RenderQuincunxMap");
		s_MsaaQuincunxAltSampler = g_DefaultEffect->LookupVar("RenderQuincunxAltMap");
		s_MsaaPointSampler = g_DefaultEffect->LookupVar("RenderPointMap");
		s_MsaaGaussianSampler = g_DefaultEffect->LookupVar("RenderGaussMap");
		s_TransparentSrcSampler = g_DefaultEffect->LookupVar("TransparentSrcMap");
		s_TransparentDstSampler = g_DefaultEffect->LookupVar("TransparentDstMap");
		s_DepthPointSampler = g_DefaultEffect->LookupVar("DepthPointMap");

		s_TexelSize = g_DefaultEffect->LookupVar("TexelSize");

	#if RAGE_ENCODEOPAQUECOLOR
		s_Msaa2xOpaqueTechnique = g_DefaultEffect->LookupTechnique("Msaa2xOpaque");
		s_Msaa2xTransparentTechnique = g_DefaultEffect->LookupTechnique("Msaa2xTransparent");
	#else
		s_Msaa2xAccuviewTechnique = g_DefaultEffect->LookupTechnique("Msaa2xAccuview");
		s_Msaa2xQuincunxTechnique = g_DefaultEffect->LookupTechnique("Msaa2xQuincunx");
		s_Msaa2xQuincunxAltTechnique = g_DefaultEffect->LookupTechnique("Msaa2xQuincunxAlt");
		s_Msaa2x2TapTechnique = g_DefaultEffect->LookupTechnique("Msaa2x2Tap");
	#endif // RAGE_ENCODEOPAQUECOLOR
		s_Msaa4xTechnique = g_DefaultEffect->LookupTechnique("Msaa4x");
		s_Msaa4xTransparentTechnique = g_DefaultEffect->LookupTechnique("Msaa4xTransparent");
		s_CopyTechnique = g_DefaultEffect->LookupTechnique("Copy");
		s_CopyDepthTechnique = g_DefaultEffect->LookupTechnique("CopyDepth");
		s_CopyTransparentTechnique = g_DefaultEffect->LookupTechnique("CopyTransparent");
	}

	// Precompute suitable blend states for all possible write masks
	grcBlendStateDesc desc;
	for (int i=0; i<16; i++)
	{
		desc.BlendRTDesc[0].RenderTargetWriteMask = i;
		s_ClearBlendStates[i] = grcStateBlock::CreateBlendState(desc);
	}


	grcViewport::InitClass();

	LABEL_VBLANK_LOCKDOWN = gcm::RsxSemaphoreRegistrar::Allocate(1);
	*cellGcmGetLabelAddress(LABEL_VBLANK_LOCKDOWN) = 0;
	LABEL_CURRENT_FRAME = gcm::RsxSemaphoreRegistrar::Allocate(1);
	*cellGcmGetLabelAddress(LABEL_CURRENT_FRAME) = 0;

	// Install VBLANK handler after label it uses is valid
	cellGcmSetVBlankHandler(VBlankHandler);

	// indices for the Z culling report
	LABEL_ZCULL_STATS2 = gcm::ReportRegistrar::Allocate(1);
	LABEL_ZCULL_STATS3 = gcm::ReportRegistrar::Allocate(1);
	Assert(gcm::ReportRegistrar::IsValid(LABEL_ZCULL_STATS2));
	Assert(gcm::ReportRegistrar::IsValid(LABEL_ZCULL_STATS3));

	LABEL_NEXT_FENCE = gcm::RsxSemaphoreRegistrar::Allocate(1);
	LABEL_COMMAND_DUMMY = gcm::RsxSemaphoreRegistrar::Allocate(1);
	*cellGcmGetLabelAddress(LABEL_NEXT_FENCE) = 0;

	CompileTimeAssert(s_LastReportIndex == (GCM_REPORT_AREA_SIZE / sizeof(CellGcmReportData)));
	s_OcclusionQueryData = cellGcmGetReportDataAddressLocation(s_OcclusionQueryReportIndex, CELL_GCM_LOCATION_MAIN);
	for (u32 i = 0; i < GCM_OCCLUSION_QUERY_COUNT; ++i)
	{
		s_OcclusionQueryData[i].timer = 0;
		s_OcclusionQueryData[i].value = 0;
		s_OcclusionQueryData[i].zero = 1;
	}

#if GRCDBG_IMPLEMENTED
	LABEL_FAULT_STACK = gcm::RsxSemaphoreRegistrar::Allocate(LABEL_FAULT_STACK_SIZE);
	cellGcmSetGraphicsHandler(gcmFaultHandler);
	for (int i=0; i<LABEL_FAULT_STACK_SIZE; i++)
		*cellGcmGetLabelAddress(LABEL_FAULT_STACK+i) = 0;
	if ((!sysBootManager::IsDebuggerPresent() || PARAM_gcmWatchdog.Get()) && !PARAM_noGcmWatchdog.Get())
		sysIpcCreateThread(gcmWatchdog,NULL,4096,PRIO_BELOW_NORMAL,"[RAGE] gcmWatchdog");
#endif

	//

#if COMMAND_BUFFER_SUPPORT
	s_CommandBufferPoolMemory = rage_aligned_new(COMMAND_BUFFER_SEGMENT_SIZE) u8[grcCommandBufferHeapSize];
	s_CommandBufferPool.Init(s_CommandBufferPoolMemory,grcCommandBufferHeapSize);
#endif

	s_spuGcmState.GpuMemCpyAddr = (u32)&s_gpuMemCpy;
	u32 *temp = (u32*)physical_new(16,16);
	*temp = ~0U;
	s_gpuMemCpy.m_doneSrcOffset = gcm::LocalOffset(temp);
}

#if __BANK && __PPU
void grcDevice::AddWidgetsPS3()
{
#if TWEAKGAMMA || !RAGE_ENCODEOPAQUECOLOR
	bkBank& grcoreBank = BANKMGR.CreateBank("rage - grcore");
#endif // TWEAKGAMMA || (__BANK && !RAGE_ENCODEOPAQUECOLOR)

#if TWEAKGAMMA
	grcoreBank.AddSlider("Gamma correction", &s_Gamma, 0.4f, 2.0f, 0.01f);
#endif // TWEAKGAMMA

#if !RAGE_ENCODEOPAQUECOLOR
	grcoreBank.AddCombo("2x Mode", &s_Msaa2xMode, kMsaa2xModeCount, kMsaa2xModeNames);
#endif // !RAGE_ENCODEOPAQUECOLOR

#if FRAGMENT_BRANCH_STRIP_DEBUG
	grcoreBank.PushGroup("Fragment UCode Branch Stripping", true);
		grcoreBank.AddToggle("Force Stripping OFF", &s_FragmentStrippingFlags, FRAGSTRIP_DEBUGFLAG_FORCE_DISABLE);
		grcoreBank.AddToggle("Force Stripping ON", &s_FragmentStrippingFlags, FRAGSTRIP_DEBUGFLAG_FORCE_ENABLE);
	grcoreBank.PopGroup();
#endif // FRAGMENT_BRANCH_STRIP_DEBUG
}
#endif // __BANK && __PPU

#if GRCDBG_IMPLEMENTED
void grcDevice::PushFaultContext(const char *label) {
	if (FAULT_STACK_SP >= LABEL_FAULT_STACK_SIZE)
		Quitf("fault context overflow");
	u32 gcmLabel = LABEL_FAULT_STACK+FAULT_STACK_SP++;
	cellGcmSetWriteTextureLabel(GCM_CONTEXT,gcmLabel,(u32)label);
	u8 len = strlen(label);
	SPU_COMMAND(grcDevice__PushFaultContext,len,(len + 4) & ~3);
	memcpy(cmd->labelText,label,len);
}

void grcDevice::PopFaultContext() {
	if (!FAULT_STACK_SP)
		Quitf("fault context underflow");
	u32 gcmLabel = LABEL_FAULT_STACK+--FAULT_STACK_SP;
	cellGcmSetWriteTextureLabel(GCM_CONTEXT,gcmLabel,0);
	SPU_SIMPLE_COMMAND(grcDevice__PopFaultContext,0);
}
#endif

void grcDevice::SetDefaultEffect(bool isLit,bool isSkinned) {
	grcEffectTechnique tech = isLit? (isSkinned? s_DefaultLitSkinned : s_DefaultLit) : (isSkinned? s_DefaultUnlitSkinned : s_DefaultUnlit);
	g_DefaultEffect->Bind(tech);
}

grcEffect& grcDevice::GetDefaultEffect()
{
	return *g_DefaultEffect;
}

void grcDevice::SetLetterBox(bool enable)
{
	sm_LetterBox = enable;
	// probably need to do the equivalent of UpdatePresentationInterval() in D3D
}

uint32_t g_MainColorWrite;

void grcDevice::Clear(bool enableClearColor, Color32 clearColor, bool enableClearDepth, float clearDepth, bool enableClearStencil, u32 clearStencil)
{
	if (!s_RenderTargetState[grcmrtDepthStencil] && (enableClearDepth || enableClearStencil))
	{
		grcWarningf("Attempting to clear a nonexistent depth/stencil surface!");
		enableClearDepth = enableClearStencil = false;
	}

	if (!s_RenderTargetState[grcmrtColor0] && enableClearColor)
	{
		grcWarningf("Attempting to clear a nonexistent color surface!");
		enableClearColor = false;
	}

	if (!enableClearColor && !enableClearDepth && !enableClearStencil)
	{
		return;
	}
	
	BANK_ONLY(AssertMsg(g_AreRenderTargetsBound, "Attempting to draw with no render targets bound");)

	bool clear5551Hack = enableClearColor && g_GcmSurface.colorFormat == CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5 && clearColor.GetAlpha() == 0;

	if (clear5551Hack)
	{
		g_GcmSurface.colorFormat = CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5;
		GCM_DEBUG(GCM::cellGcmSetSurface(GCM_CONTEXT, &g_GcmSurface));
	}

	/// AssertMsg(grcStateBlock::BS_Active != INVALID_STATEBLOCK,"This function cannot be converted to stateblocks until callers all use stateblocks");

	// Use the main target write flags (call flush first to make sure g_MainColorWrite is accurate)
	grcStateBlock::FlushThrough(true);

	uint32_t writeMask = g_MainColorWrite;

	int mask = 0;
	if (s_RenderTargetState[grcmrtColor0] && enableClearColor)
	{
		if (gcm::IsFloatingPointColorSurfaceFormat(g_GcmSurface.colorFormat))
		{
			enableClearColor = false;

			// TODO - save and restore the stateblocks here.  Should add a helper function for this.
			grcStateBlock::SetStates(grcStateBlock::RS_NoBackfaceCull,grcStateBlock::DSS_IgnoreDepth,s_ClearBlendStates[writeMask]);

			if (s_RenderTargetState[grcmrtColor3])	// mrt4?
			{
				g_DefaultEffect->Bind(s_DefaultClearMRT4);
			}
			else if (s_RenderTargetState[grcmrtColor2])	// mrt3?
			{
				g_DefaultEffect->Bind(s_DefaultClearMRT3);
			}
			else if (s_RenderTargetState[grcmrtColor1])	// mrt2?
			{
				g_DefaultEffect->Bind(s_DefaultClearMRT2);
			}
			else
			{
				g_DefaultEffect->Bind(s_DefaultClear);
			}

			grcDrawSingleQuadf(-1, -1, 1, 1, 0.0f, 0, 0, 1, 1, clearColor);
			
			g_DefaultEffect->UnBind();
		}
		else
		{
			u32 color = clearColor.GetRed() | (clearColor.GetGreen() << 8) | (clearColor.GetBlue() << 16) | (clearColor.GetAlpha() << 24);
			switch (g_GcmSurface.colorFormat)
			{
			case CELL_GCM_SURFACE_B8:
				color &= 0x000000ff;
				break;
			case CELL_GCM_SURFACE_G8B8:
				color &= 0x0000ffff;
				break;
			case CELL_GCM_SURFACE_R5G6B5:
				color = u8(clearColor.GetRedf() * 31.0f) | (u8(clearColor.GetGreenf() * 63.0f) << 5) | (u8(clearColor.GetBluef() * 31.0f) << 11);
				break;
			case CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5:
			case CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5:
				color = u8(clearColor.GetRedf() * 31.0f) | (u8(clearColor.GetGreenf() * 31.0f) << 5) | (u8(clearColor.GetBluef() * 31.0f) << 10);
				break;
			case CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8:
			case CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8:
			case CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8:
			case CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8:
				color &= 0x00ffffff;
				break;
			default:
				break;
			}

			mask |= (writeMask << 4);
			GCM_STATE(SetClearColor, color);
		}
	}

	if (enableClearDepth)
	{
		// ZcullRAM will not be cleared if CELL_GCM_FALSE has been set to cellGcmSetDepthMask()
		GCM_STATE(SetDepthMask, true);
		mask |= CELL_GCM_CLEAR_Z;
	}
	if (enableClearStencil)
	{
		// because Scull is not validated by cellGcmSetClearSurface() when the mask value is a
		// value other than 0x00 or 0xff, make sure to set 0x00 or 0xff as the mask value
		GCM_STATE(SetStencilMask, 0xFF);
		mask |= CELL_GCM_CLEAR_S;
	}

	if (mask)
	{
		if (enableClearDepth || enableClearStencil)
		{
			GCM_STATE(SetClearDepthStencil, (static_cast<u32>(clearDepth * 0xffffff) << 8) | clearStencil);
		}
		grcStateBlock::FlushThrough(true);
		GCM_DEBUG(GCM::cellGcmSetClearSurface(GCM_CONTEXT, mask));
	}

	if (clear5551Hack)
	{
		g_GcmSurface.colorFormat = CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5;
		GCM_DEBUG(GCM::cellGcmSetSurfaceWindow(GCM_CONTEXT, &g_GcmSurface, PS3_SHADER_VPOS_WINDOW_ORIGIN, PS3_SHADER_VPOS_PIXEL_CENTER));
	}

	// Respecify the current DSS since we messed with the depth and stencil mask directly.
	if (enableClearDepth || enableClearStencil) {
		grcStateBlock::DSS_Dirty = true;
		grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_Active,grcStateBlock::ActiveStencilRef);
		grcStateBlock::FlushThrough(true);
	}
}


void grcDevice::ClearCachedState() {
	grcEffect::UnBind();
	grcEffect::ClearCachedState();
}

#if DRAWABLESPU_STATS
spuDrawableStats defaultStatsBucket;
#endif

bool grcDevice::BeginFrame()
{
#if !__FINAL
	s_DumpThisFrame = PARAM_hexdumpgcm.Get();
#endif

	GRCDBG_PUSH("BeginFrame");

#if TWEAKGAMMA
	if (s_Gamma != s_OldGamma)
	{
		s_OldGamma = s_Gamma;
		LoadAVConfModule();
		SetGamma(s_Gamma);
		UnLoadAVConfModule();
	}
#endif // TWEAKGAMMA

	g_CommandBufferUsage = 0;

	InvalidateSpuGcmState(CachedStates, ~0);

	if (sm_FrameCounter)
	{
#if __FINAL
		SPU_SIMPLE_COMMAND(grcDevice__BeginFrame,0);
#else
		grcFlashEnable = (sm_FrameCounter & 16) != 0;
		SPU_SIMPLE_COMMAND(grcDevice__BeginFrame,grcFlashEnable | (g_DipSwitches << 4)
			DEBUG_ALLOW_CACHED_STATES_TOGGLE_ONLY( | (g_DebugSetAllStates << 1))
		);
#endif
		// Displayf("%u/%u  %u+%u",defaultStatsBucket.VisibleIndices,defaultStatsBucket.TotalIndices,defaultStatsBucket.MacroCommands,defaultStatsBucket.GcmCommands);
		DRAWABLESPU_STATS_ONLY(memset(&defaultStatsBucket,0,sizeof(defaultStatsBucket)));
		// TODO - hook these back up to profile stats?  But higher-level code wants per-bucket information so this would
		// end up being incorrect anyway.
	}
#if DRAWABLESPU_STATS
	else {
		SPU_COMMAND(grcEffect__FetchStats,0);
		cmd->dstPtr = &defaultStatsBucket;
		rageFlush(GCM_CONTEXT);
	}
#endif

	cellGcmSetReportLocation(GCM_CONTEXT,CELL_GCM_LOCATION_MAIN);

	ClearCachedState();

	grcStateBlock::MakeDirty();
	grcStateBlock::Default();
	grcStateBlock::FlushThrough(true);

#if __BANK
	grcRenderTarget::BeginFrame();
#endif

#ifdef GCM_PF
	uint32_t cycle;
	uint32_t counters[CELL_GCM_PM_MAX_COUNTER];
	cellGcmGetPerfMonCounter(CELL_GCM_PM_DOMAIN_GCLK, counters, &cycle);
	// grcDisplayf("%u %u %u %u",counters[0],counters[1],counters[2],counters[3]);
	PF_INCREMENTBY(GCLK_PRIMITIVECOUNT, counters[0]);
	PF_INCREMENTBY(GCLK_SETUP_TRIANGLES, counters[1]);
	PF_INCREMENTBY(GCLK_VAB_CMD_LOAD, counters[2]);
	PF_INCREMENTBY(GCLK_VAB_CMD_TRI, counters[3]);

	static uint32_t events[CELL_GCM_PM_MAX_EVENT] = {
		CELL_GCM_PM_GCLK_PRIMITIVECOUNT,
		CELL_GCM_PM_GCLK_SETUP_TRIANGLES,
		CELL_GCM_PM_GCLK_VAB_CMD_LOAD,
		CELL_GCM_PM_GCLK_VAB_CMD_TRI
	};
	cellGcmSetPerfMonEvent(CELL_GCM_PM_DOMAIN_GCLK, events);
	cellGcmSetPerfMonTrigger();
#endif // GCM_PF

	// Reinstate default viewport settings so it works correctly post-090.
	grcViewport::SetCurrent(grcViewport::GetCurrent());

	grcTextureFactory& textureFactory = grcTextureFactory::GetInstance();
	if (GetMSAA())
	{
		textureFactory.SetDefaultRenderTarget(grcmrtColor0, textureFactory.GetBackBuffer(false));
		textureFactory.SetDefaultRenderTarget(grcmrtDepthStencil, textureFactory.GetBackBufferDepth(false));
	}
	else
	{
		textureFactory.SetDefaultRenderTarget(grcmrtColor0, textureFactory.GetFrontBuffer());
		textureFactory.SetDefaultRenderTarget(grcmrtDepthStencil, textureFactory.GetFrontBufferDepth(false));
	}
	// We should only do this when needed, quite slow and doesn't need to be done in normal gameplay
	// But do it at least once per frame or rage samples and any other simple rendering loops break
	textureFactory.BindDefaultRenderTargets();

#if !HACK_GTA4 // no shader clip planes
	// This prevents a spurious GPAD warning about referencing unused clip plane variables.
	static Vec4V zeroes[RAGE_MAX_CLIPPLANES];
	if (s_ClipPlanesGlobalVar)
		grcEffect::SetGlobalVar(s_ClipPlanesGlobalVar,zeroes,RAGE_MAX_CLIPPLANES);
#endif // !HACK_GTA4

	grcEffect::BeginFrame();

#if !__FINAL
	s_Watchdog.Reset();
#endif

#if !__FINAL
	u8 FragProgStrippingFlags = PARAM_disablebranchstripping.Get() ? FRAGSTRIP_DEBUGFLAG_FORCE_DISABLE : 0x0;
#if FRAGMENT_BRANCH_STRIP_DEBUG
	FragProgStrippingFlags |= s_FragmentStrippingFlags;
#endif // FRAGMENT_BRANCH_STRIP_DEBUG
	SPU_SIMPLE_COMMAND(grcEffect__SetFragStripDebugFlags,FragProgStrippingFlags);
#endif // !__FINAL

	// Make sure all unused vertex streams return zero
	static float v4[4];
	for (int i=0; i<16; i++) 
		cellGcmSetVertexData4f(GCM_CONTEXT,i,v4);

	return true;
}

bool s_NeedWaitFlip = true;
bool s_NeedRealWaitFlip = false;

void grcDevice::EndFrame(const grcResolveFlags *)
{
#if !__FINAL
	sys_gpio_get (SYS_GPIO_DIP_SWITCH_DEVICE_ID,&g_DipSwitches);
#endif

	grcEffect::EndFrame();

	MsaaResolveBackToFrontBuffer();

	g_DefaultEffect->Bind(grcetNONE);

	// Make sure the GPU is blocked until a sufficient number of vblanks have elapsed.
	// The label is cleared in the VBLANK handler so tearing shouldn't be possible.
	GCM_DEBUG(cellGcmSetWaitLabel(GCM_CONTEXT, LABEL_VBLANK_LOCKDOWN, 1));				// Wait for the vsync handler to unblock the RSX
	GCM_DEBUG(cellGcmSetWriteCommandLabel(GCM_CONTEXT, LABEL_VBLANK_LOCKDOWN, 0));		// Reset the label so we wait next time.

	GCM_DEBUG(cellGcmSetWriteTextureLabel(GCM_CONTEXT, LABEL_CURRENT_FRAME, sm_FrameCounter));		// Reset the label so we wait next time.

	// Insert a wait for flip if they never called SetWaitFlip in higher level code.
	if (s_NeedWaitFlip)
		s_NeedRealWaitFlip = true;

	GCM_DEBUG(cellGcmSetFlip(GCM_CONTEXT,sm_FrameCounter % g_FrontBufferCount));
	s_NeedWaitFlip = true;

#if GCM_REPLAY
	uint32_t status;
	g_grcIsCurrentlyCapturing = (cellGcmGpadGetStatus(&status) == CELL_OK && status == CELL_GCM_GPAD_STATUS_IS_CAPTURING);
#endif

	// Initialize state cache to crap
	memset(gcm::Shadow::g_State,0xCE,sizeof(gcm::Shadow::g_State));

	rageFlush(GCM_CONTEXT);

#if 1 // Commented out, it didn't seem to help anything, making it worth in some occasions.
	// Make sure the CPU isn't getting too far ahead of the GPU.
	// Can't early abort here or replay will choke.
	// We compare for equality here so the 4G wraparound case (which happens after about 4.5 years)
	// will still work properly.
	u32 stallTime = 0;
	for (;;) {
		u32 gpuFrame = *cellGcmGetLabelAddress(LABEL_CURRENT_FRAME);
		if (gpuFrame != sm_FrameCounter && gpuFrame + 1 != sm_FrameCounter) {
			stallTime += 30;
			sys_timer_usleep(30);
		}
		else
			break;
	}
#endif // 0
	if (s_NeedRealWaitFlip) {
		NOTFINAL_ONLY(if (!PARAM_frameLimit.Get()))
			GCM_DEBUG(cellGcmSetWaitFlip(GCM_CONTEXT));
		s_NeedRealWaitFlip = false;
	}

	SPU_SIMPLE_COMMAND(grcDevice__EndFrame,0);

#if __BANK
	grcRenderTarget::EndFrame();
#endif

	GRCDBG_POP();

	// Close the current segment so command buffer usage is stable.
	GCM_CONTEXT->callback(GCM_CONTEXT,1);

	++sm_FrameCounter;
	IncrementSyncCounter();

#if !__FINAL
	static u8 lut[] = { 1, 2, 4, 8, 4, 2 };
	sys_gpio_set(SYS_GPIO_LED_DEVICE_ID,0xF,lut[(sm_FrameCounter>>3) % 6] ^ (ioPad::IsIntercepted()? 15 : 0));
#endif
	// grcDisplayf("halt"); while(1) sysIpcSleep(1000);

	TASKLOG("Frame number %u",(u32)sm_FrameCounter);
}

#if COMMAND_BUFFER_SUPPORT
#define FakeCommandBuffer ((grcCommandBuffer*)1)

void grcDevice::CreateCommandBuffer(size_t /*size*/,grcCommandBuffer **pbuffer) {
	// *pbuffer = grcCommandBuffer::Create(size);
	*pbuffer = FakeCommandBuffer;
}

void grcDevice::DeleteCommandBuffer(grcCommandBuffer * buffer) {
	if (buffer != FakeCommandBuffer) {
		// grcDisplayf("deleting cb at %p",buffer);
		do {
			u8 *base = (u8*)((u32)buffer & ~(COMMAND_BUFFER_SEGMENT_SIZE-1));
			u32 length = ((u32)buffer & (COMMAND_BUFFER_SEGMENT_SIZE-1)) << 4;

			u32 *terminator = (u32*)(base + length - 8);
			grcAssertf(*terminator == ((grcDevice__ReturnFromCommandBuffer<<1) | 0x40001)
				|| *terminator == ((grcDevice__NextCommandBufferSegment<<1) | 0x80001),"invalid terminator %x",*terminator);
			buffer = (grcCommandBuffer*) terminator[1];

			// grcDisplayf("delete %p",base);
			s_CommandBufferPool.Delete(base);
		} while (buffer);
		// grcDisplayf("[COMMANDBUFFER] Buffer deleted, %dk remain",s_CommandBufferPool.GetFreeCount()*(COMMAND_BUFFER_SEGMENT_SIZE>>10));
	}
}

// static uint32_t s_LastUsed;

static CellGcmContextData *s_OldContext;
static CellGcmContextData s_CommandBufferRecording;
static grcCommandBuffer* s_CloneBuffer;

void grcDevice::BeginCommandBuffer(grcCommandBuffer * /*buffer*/,bool /*isSkinned*/) {
	grcAssertf(!g_grcCommandBuffer,"already in command buffer");
	g_grcCommandBuffer = FakeCommandBuffer;
	s_OldContext = GCM_CONTEXT;

	GCM_CONTEXT = &s_CommandBufferRecording;
	s_CommandBufferRecording.current = s_CommandBufferRecording.begin = (u32*) s_CommandBufferPool.New();
	s_CommandBufferRecording.end = s_CommandBufferRecording.begin + SegmentSizeInWords;
	s_CommandBufferRecording.callback = CommandBufferNextSegment;
	s_HadCommandBufferOverrun = (s_CommandBufferPool.IsFull());
	
	s_LastSegment = (u32*) &s_CloneBuffer;
	s_RecordSize = 0;

	ClearCachedState();
}


size_t grcDevice::EndCommandBuffer(grcCommandBuffer ** cloneBuffer) {
	grcAssertf(g_grcCommandBuffer,"no command buffer in progress");
#if SPU_GCM_FIFO
	// pad buffer to multiple of 16 for DMA, so that return and a terminator
	// word are always at the end of a quadword
	while (((uint32_t)GCM_CONTEXT->current & 15) != 8)
		*GCM_CONTEXT->current++ = 0;
	// Return command is no larger than the NextCommandBufferSegment call we
	// always reserve space for, so we don't really need to check.  We also
	// don't want to allocate a whole new segment just for it!
	*GCM_CONTEXT->current++ = (grcDevice__ReturnFromCommandBuffer<<1) | 0x40001;
	*GCM_CONTEXT->current++ = 0;	// list terminator
	uint32_t allocated = (GCM_CONTEXT->current - GCM_CONTEXT->begin) >> 2;
	grcAssertf(!((uint32_t)GCM_CONTEXT->begin & allocated),"alignment problem");
	*s_LastSegment = (uint32_t)GCM_CONTEXT->begin | allocated;
	s_RecordSize += COMMAND_BUFFER_SEGMENT_SIZE;
	static uint32_t biggest;
	if (s_RecordSize > biggest) {
		// grcDisplayf("[COMMANDBUFFER] Biggest command buffer is now %dk, %dk remain",s_RecordSize>>10,s_CommandBufferPool.GetFreeCount()*(COMMAND_BUFFER_SEGMENT_SIZE>>10));
		biggest = s_RecordSize;
	}
#else
	GCM_DEBUG(GCM::cellGcmSetReturnCommand(GCM_CONTEXT));
#endif

	size_t result = s_RecordSize;
	grcAssertf(cloneBuffer,"clone buffer allocation failed");
	*cloneBuffer = s_CloneBuffer;
	s_CloneBuffer = NULL;
	s_LastSegment = NULL;

	GCM_CONTEXT = s_OldContext;
	s_OldContext = NULL;

	g_grcCommandBuffer = NULL;
	ClearCachedState();

	if (s_HadCommandBufferOverrun) {
		static int warner;
		if (++warner == 25) {
			grcDisplayf("EndCommandBuffer - cmd buf alloc of size %d bytes failed.",result);
			warner = 0;
		}

		DeleteCommandBuffer(*cloneBuffer);
		*cloneBuffer = NULL;
		result = 0;
	}

	return result;
}


void grcDevice::RunCommandBuffer(grcCommandBuffer * buffer,u32 /*predicationSelect*/) {
#if SPU_GCM_FIFO
	SPU_COMMAND(grcDevice__RunCommandBuffer,0);
	cmd->addrAndSize = (u32) buffer;
#else
	grcAssertf(gcm::MainOffset(buffer),"Invalid command buffer");
	GCM_DEBUG(GCM::cellGcmSetCallCommand(GCM_CONTEXT,gcm::MainOffset(buffer)));
	ClearCachedState();
#endif
}



void grcDevice::SetCommandBufferPredication(u32 /*tile*/,u32 /*run*/) {
}


#endif		// COMMAND_BUFFER_SUPPORT

void grcDevice::ShutdownClass()
{
	cellGcmSetWaitFlip(GCM_CONTEXT);

	rageFlush(GCM_CONTEXT);

	// rageFinish doesn't really work right with SPU buffering.
	// This does essentially the same thing.
	BlockOnFence(InsertFence());

	grcStateBlock::ShutdownClass();
	grcShutdownQuads();

#if COMMAND_BUFFER_SUPPORT
	delete [] s_CommandBufferPoolMemory;
	s_CommandBufferPoolMemory = NULL;
#endif

	cellGcmSetVBlankHandler(NULL);
	gcm::RsxSemaphoreRegistrar::Free(LABEL_VBLANK_LOCKDOWN); 
	gcm::RsxSemaphoreRegistrar::Free(LABEL_CURRENT_FRAME); 

	gcm::RsxSemaphoreRegistrar::Free(LABEL_COMMAND_DUMMY);
	gcm::RsxSemaphoreRegistrar::Free(LABEL_NEXT_FENCE);
	gcm::ReportRegistrar::Free(LABEL_ZCULL_STATS3);
	gcm::ReportRegistrar::Free(LABEL_ZCULL_STATS2);

	// grcEffect::ShutdownClass has already been called by now:
	// delete g_DefaultEffect;
	// g_DefaultEffect = NULL;

	gcm::RsxSemaphoreRegistrar::Free(s_spuGcmState.FragmentFifo.GetLabelId());
	gcm::RsxSemaphoreRegistrar::Free(s_spuGcmState.CommandFifo.GetLabelId());

	grcViewport::ShutdownClass();
	grcEffect::ShutdownClass();
}

bool grcDevice::GetWideScreen()
{
	return s_IsWideScreen;
}

bool grcDevice::GetHiDef()
{
	return s_IsHiDef;
}

bool grcDevice::GetInterlaced()
{
	return s_IsInterlaced;
}

// PARAM(testscissor,"test flag");

void grcDevice::SetWindow(const grcWindow &window) {

	// even though we're only using the first 3 floats in scale[] and offset[], the GCM::cellGcmSetViewport expects 4
	static const float fFlip = (PS3_VIEWPORT_WINDOW_ORIGIN == CELL_GCM_WINDOW_ORIGIN_TOP) ? -1.f : 1.f;
	float scale[4] = { window.GetWidth() * 0.5f, window.GetHeight() * (0.5f * fFlip), (window.GetMaxZ() - window.GetMinZ()), 0.0f };
	float offset[4] = { window.GetX() + scale[0], window.GetY() + (scale[1] * fFlip), window.GetMinZ(), 0.0f };

	if ( PS3_VIEWPORT_WINDOW_PIXEL_CENTER == CELL_GCM_WINDOW_PIXEL_CENTER_INTEGER )
	{
		offset[0] += 0.5;
		offset[1] += 0.5;
	}

	/* if (PARAM_testscissor.Get()) {
		GCM_DEBUG(GCM::cellGcmSetScissor(GCM_CONTEXT,window.GetX(),window.GetY(),window.GetWidth(),window.GetHeight()));
		GCM_DEBUG(GCM::cellGcmSetViewport(GCM_CONTEXT,window.GetX(),window.GetY(),window.GetWidth(),window.GetHeight(),window.GetMinZ(),window.GetMaxZ(),scale, offset));
	} */

    SPU_COMMAND(grcViewport__SetWindow,g_GcmSurface.antialias);
    cmd->scissorArea[0] = window.GetX();
    cmd->scissorArea[1] = window.GetY();
    cmd->scissorArea[2] = window.GetWidth();
    cmd->scissorArea[3] = window.GetHeight();
    cmd->depthRange[0]  = window.GetMinZ();
    cmd->depthRange[1]  = window.GetMaxZ();
    cmd->viewportScales[0] = scale[0];
    cmd->viewportScales[1] = scale[1];
    cmd->viewportScales[2] = scale[2];
	cmd->viewportScales[3] = scale[3];
    cmd->viewportOffsets[0] = offset[0];
    cmd->viewportOffsets[1] = offset[1];
    cmd->viewportOffsets[2] = offset[2];
	cmd->viewportOffsets[3] = offset[3];

	/* s_WindowX = window.GetX();
	s_WindowY = window.GetY();
	s_WindowWidth = window.GetWidth();
	s_WindowHeight = window.GetHeight(); */
}

void grcDevice::BeginBlit() {}
void grcDevice::EndBlit() {}
void grcDevice::BlitRect(int , int , int , int , float , int , int , int ,int , const Color32 &) {}
void grcDevice::BlitRectf(float , float , float , float , float , float , float , float ,float , const Color32 & ) {}

void grcDevice::GetSafeZone(int &x0, int &y0, int &x1, int &y1) {
#if __PPU
	// TODO: Compute PSN's safe zone here:
	x0 = 0;
	y0 = 0;
	x1 = sm_CurrentWindows[g_RenderThreadIndex].uWidth-1;
	y1 = sm_CurrentWindows[g_RenderThreadIndex].uHeight-1;
#else
	// Assume PC
	x0 = 0;
	y0 = 0;
	x1 = sm_Width-1;
	y1 = sm_Height-1;
#endif
}

#if __PPU
void grcDevice::SetWindowTitle(const char*) {
}
#endif


void grcDevice::SetScissor(int x,int y,int width,int height) {
	grcAssertf(x >= 0 && x <= 4095,"Invalid x in grcDevice::SetScissor, x (%d) should be between 0 and 4095 inclusive",x);
	grcAssertf(y >= 0 && y <= 4095,"Invalid y in grcDevice::SetScissor, y (%d) should be between 0 and 4095 inclusive",y);
	grcAssertf(width >= 0 && width <= 4096,"Invalid width in grcDevice::SetScissor, width (%d) should be between 0 and 4096 inclusive",width);
	grcAssertf(height >= 0 && height <= 4096,"Invalid height in grcDevice::SetScissor, height (%d) should be between 0 and 4096 inclusive",height);

	// if (PARAM_testscissor.Get()) GCM_DEBUG(GCM::cellGcmSetScissor(GCM_CONTEXT,x,y,width,height));
    SPU_COMMAND(grcDevice__SetScissor,0);
    cmd->scissorArea[0] = x;
    cmd->scissorArea[1] = y;
    cmd->scissorArea[2] = width;
    cmd->scissorArea[3] = height;
}


void grcDevice::DisableScissor() {
	// if (PARAM_testscissor.Get()) GCM_DEBUG(GCM::cellGcmSetScissor(GCM_CONTEXT,0,0,4096,4096));
    SPU_COMMAND(grcDevice__SetScissor,0);
    cmd->scissorArea[0] = 0;
    cmd->scissorArea[1] = 0;
    cmd->scissorArea[2] = 4096;
    cmd->scissorArea[3] = 4096;
}

// In RSX(TM), it is possible to enable/disable 
// culling of a primitive that doesn't have any Z values, of any of its 
// vertices, within the space defined by the near clipping plane and the far 
// clipping plane, in the Z direction set by min and max of cellGcmSetViewport() 
// (in other words, a primitive with all its vertices outside of viewport).
void grcDevice::SetZMinMaxControl(const u32 cullNearFarEnable,
								  const u32 zclampEnable,
								  const u32 cullIgnoreW) 
{
	GCM_DEBUG(GCM::cellGcmSetZMinMaxControl(GCM_CONTEXT,
											cullNearFarEnable,  // Initial value is enable  
											zclampEnable,		// Initial value is disable  
											cullIgnoreW));		// Initial value is disable  
}

// This function enables/disables the depth boundary test feature of RSX(TM).
// The depth boundary test is applied to a depth value already written in the 
// depth buffer; the depth value of the fragment to which you are attempting 
// to write is unrelated. In other words, the fragment to which you are attempting 
// to write will be destroyed - regardless of its depth value - if the value of 
// the depth buffer is outside the range specified in cellGcmSetDepthBounds().
void grcDevice::SetDepthBoundsTestEnable(u32 enable)
{
	GCM_DEBUG(GCM::cellGcmSetDepthBoundsTestEnable(GCM_CONTEXT, enable));
}

// This function specifies the minimum value and the maximum value for depth boundaries.
// The range specified by zmin and zmax will be used as the range within which values 
// will pass the depth boundary test. 
//
// You need to switch on the depth bounds test first with SetDepthBoundsTestEnable()
//
void grcDevice::SetDepthBounds(float zmin,
							   float zmax)
{
	GCM_DEBUG(GCM::cellGcmSetDepthBounds(GCM_CONTEXT,
										zmin,
										zmax));
}

void grcDevice::ClearRect(int x,int y,int width,int height,float depth,const Color32 &color) {
	ClearRect(x,y,width,height,true,color,true,depth,true,0);
}

void grcDevice::ClearRect(int x,int y,int width,int height,bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth,bool enableClearStencil,u32 clearStencil) {
	grcAssertf(x >= 0 && y >= 0,"invalid x/y to clearrect: %d,%d",x,y);
	grcAssertf((x + width) <= (int)sm_CurrentWindows[g_RenderThreadIndex].uWidth && (y + height) <= (int)sm_CurrentWindows[g_RenderThreadIndex].uHeight,"scissor off right/bottom");

	if (width == 0 || height == 0)
		return;

	GCM_DEBUG(GCM::cellGcmSetScissor(GCM_CONTEXT,x,y,width,height));
	Clear(enableClearColor, clearColor, enableClearDepth, clearDepth, enableClearStencil, clearStencil);
	GCM_DEBUG(GCM::cellGcmSetScissor(GCM_CONTEXT,0,0,sm_CurrentWindows[g_RenderThreadIndex].uWidth,sm_CurrentWindows[g_RenderThreadIndex].uHeight));
}

//
// Capture a screen-shot on the PS3
//
// Here is an example on how to use it
//
// grcImage *image = GRCDEVICE.CaptureScreenShot(g_DepthBuffer);
//
// const char *shotName = "c:/depthbuffer.jpg";
// if (image)
// {
// 	image->SaveJPEG(shotName);
// 	image->Release();
// }

grcImage* grcDevice::CaptureScreenShot(grcImage* image)
{

	const grcRenderTargetGCM *front = static_cast<const grcRenderTargetGCM*>(grcTextureFactory::GetInstance().GetFrontBuffer(true));
	grcAssertf(front,"no front buffer");

	grcImage *dest = NULL;
	const CellGcmTexture *gcmTexture=reinterpret_cast<const CellGcmTexture*>(front->GetTexturePtr());

	grcAssertf(gcmTexture->format==CELL_GCM_TEXTURE_A8R8G8B8 | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR,"we only support 8 bit, linear, normalized textures");

	int	width=front->GetWidth();
	int	height=front->GetHeight();


   if (image)
   {
      dest = image;
   }
   else
   {
	   dest = grcImage::Create(width,height,1,grcImage::A8R8G8B8,grcImage::STANDARD,0,0);
   }

	// no need to lock we can read direct from GPU ram at any time, assuming the frontbuffer has actually been setup
	if(dest)
	{
		u32 *d = reinterpret_cast<u32*>(dest->GetBits());
		u32 *s = reinterpret_cast<u32*>(gcmTexture->location == CELL_GCM_LOCATION_LOCAL ? gcm::LocalPtr(front->GetMemoryOffset()) : gcm::MainPtr(front->GetMemoryOffset()));
		for (int row=0; row<height; row++,s = (u32*)((char*)s + gcmTexture->pitch),d += width) {
			for (int col=0; col<width; col++)
				d[col] = ((s[col]&0xff)<<16)|((s[col]&0xff0000)>>16)|((s[col]&0xff00)) | 0xFF000000;
		}
		return dest;;

	}

	return NULL;
}

#if !__FINAL
static CellDbgRsxCursor camera = {
	{ 0xffffffff, 0xfffffffb, 0x9877bfff, 0xffffffff, 0xfffdd8bb, 0x657bbbbc, 0xffee8777, 0xadffffff, 0xffdcd863, 0x368cbcde, 0xeeee9887, 0x777bffff, 0xfccdddde, 0xdddccccd, 0xeee77999, 0x8887afff, 
	0xcbccdeee,	0xdddcccb9, 0x9977a999, 0x98887cff, 0xbbbcddee, 0xddddcccb, 0xbba98888, 0x888888ff, 0xaabcd955, 0x9dddcca9, 0x99998877, 0x777888ff, 0xaabce5e3, 0x5ddca9ab, 0xcddcba98, 0x777788ff, 
	0x9abce533,	0x5db99ade, 0xeeddcccb, 0x988789ff, 0x9abce955, 0x8a99adee, 0xed975567, 0x998889ff, 0x89bceeed, 0xa99adeee, 0xea632235, 0x799889ff, 0x99bceeda, 0xaa9beeee, 0xb6222211, 0x37a989ff, 
	0x89abeeca,	0xaa9ceeee, 0x71464441, 0x138a89ff, 0x89abeeba, 0xaa9deeee, 0x626a7442, 0x116a8aff, 0x789beeaa, 0xaa9deeee, 0x326b7444, 0x105b8acf, 0x779aeeba, 0xab9cdeee, 0x32464444, 0x105b8a9d, 
	0x778aeebb,	0xbbadddee, 0x31222244, 0x105b8b9b, 0x778aeecb, 0xbbacdeee, 0x51222442, 0x016a8b9a, 0x8789eedb, 0xbbbbceee, 0x71122420, 0x038a8b9b, 0xd789eeed, 0xcccbbcde, 0xd5111100, 0x37a98c9d, 
	0xfa78eeee, 0xdcccabcd, 0xda530015, 0x7aaaaccf, 0xffb8cdde, 0xdddccbab, 0xdda76567, 0xabbbbdff, 0xfffc8998, 0x77777788, 0x9bcddddc, 0xbbbcffff, 0xffffffff, 0xfffffecb, 0xa9999aab, 0xceffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 
	0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff },
	{ { 0x01a0, 0x0190, 0x0190, 0x0000 }, { 0x0340, 0x0330, 0x0280, 0x1236 }, { 0x04b0, 0x0490, 0x0330, 0x246d }, { 0x04b0, 0x04a0, 0x0470, 0x36a4 }, 
	{ 0x0620, 0x05c0, 0x0460, 0x48db }, { 0x0660, 0x0660, 0x0680, 0x5b12 }, { 0x0750, 0x0770, 0x0720, 0x6d49 }, { 0x0940, 0x0970, 0x0990, 0x7f80 }, 
	{ 0x0a00, 0x0a30, 0x0a50, 0x91b6 }, { 0x0ad0, 0x0b00, 0x0b00, 0xa3ed }, { 0x0b90, 0x0bc0, 0x0bd0, 0xb624 },	{ 0x0c50, 0x0c90, 0x0c80, 0xc85b },
	{ 0x0d20, 0x0d60, 0x0d50, 0xda92 }, { 0x0df0, 0x0e30, 0x0e30, 0xecc9 }, { 0x0ea0, 0x0ee0, 0x0f00, 0xff00 }, { 0x0000, 0x0000, 0x0000, 0x0000 } },
	3
};
#endif

// Callback which
static void copyscan_ps3(u8 *dest,void *base,int row,int width,int stride,u8 *gammalut)
{
	u32 *src = (u32*)((char*)base + row * stride);
	while (width--) {
		u32 n = *src++;
		dest[0] = gammalut[u8(n)];
		dest[1] = gammalut[u8(n >> 8)];
		dest[2] = gammalut[u8(n >> 16)];
		dest += 3;
	}
}

bool grcDevice::CaptureScreenShotToFile(const char *outFile,float gamma)
{
	if (GCM_CONTEXT)
		BlockOnFence(InsertFence());
#if !__FINAL
	if (cellDbgRsxInit())
		grcErrorf("cannot init cursor");
	cellDbgRsxEnableCursor(&camera,sm_CurrentWindows[g_RenderThreadIndex].uWidth/2-64,sm_CurrentWindows[g_RenderThreadIndex].uHeight/2-64);
#endif
	const grcRenderTargetGCM *front = static_cast<const grcRenderTargetGCM*>(grcTextureFactory::GetInstance().GetFrontBuffer());
	const CellGcmTexture *gcmTexture=reinterpret_cast<const CellGcmTexture*>(front->GetTexturePtr());
	u8 *s = reinterpret_cast<u8*>(gcmTexture->location == CELL_GCM_LOCATION_LOCAL ? gcm::LocalPtr(front->GetMemoryOffset()) : gcm::MainPtr(front->GetMemoryOffset()));
	grcImage::WritePNG(outFile, copyscan_ps3, front->GetWidth(), front->GetHeight(), s, front->GetWidth()*4, gamma);
#if !__FINAL
	cellDbgRsxDisableCursor();
	cellDbgRsxExit();
#endif
	return true;
}

int g_JpegSaveQuality = 75;

bool grcDevice::CaptureScreenShotToJpegFile(const char *outFile)
{
	if (GCM_CONTEXT)
		BlockOnFence(InsertFence());
#if !__FINAL
	if (cellDbgRsxInit())
		grcErrorf("cannot init cursor");
	cellDbgRsxEnableCursor(&camera,sm_CurrentWindows[g_RenderThreadIndex].uWidth/2-64,sm_CurrentWindows[g_RenderThreadIndex].uHeight/2-64);
#endif
	const grcRenderTargetGCM *front = static_cast<const grcRenderTargetGCM*>(grcTextureFactory::GetInstance().GetFrontBuffer());
	const CellGcmTexture *gcmTexture=reinterpret_cast<const CellGcmTexture*>(front->GetTexturePtr());
	u32 *s = reinterpret_cast<u32*>(gcmTexture->location == CELL_GCM_LOCATION_LOCAL ? gcm::LocalPtr(front->GetMemoryOffset()) : gcm::MainPtr(front->GetMemoryOffset()));
	int count = front->GetWidth() * front->GetHeight();
	for (int i=0; i<count ;i++) {
		u32 pix = s[i];
		s[i] = ((pix >> 16) & 0xFF) | (pix & 0xFF00FF00) | ((pix & 0xFF) << 16);
	}
	grcImage::SaveStreamToJPEG(outFile, s, front->GetWidth(), front->GetHeight(), front->GetPitch(), g_JpegSaveQuality);
	for (int i=0; i<count ;i++) {
		u32 pix = s[i];
		s[i] = ((pix >> 16) & 0xFF) | (pix & 0xFF00FF00) | ((pix & 0xFF) << 16);
	}
#if !__FINAL
	cellDbgRsxDisableCursor();
	cellDbgRsxExit();
#endif
	return true;
}

//
// Adaptive Z culling functions
// This follows the description in the Run Time Library documentation in Chapter 10 ZCull
//
// This algorithm analyzes how effective the Zcull unit operated for the previous frame 
// was, and bases the next frame's settings on that. Specifically, it calculates 
// percentage of culled pixels and compares this to the percentage of pixels of the 
// previous frame. If higher, it adjusts the limit value further in the same direction. 
// If lower, it adjusts the limit value in the opposite direction. Percentage of culled 
// pixels can be calculated from the number of tiles on which Z test was performed within 
// Zcull and the number of tiles culled because they failed the Z test [available through 
// cellGcmGetReport()]. (To minimize noise of the monitor result, RSX(TM) does not include 
// tiles, rendered with the Z test disabled, in the value obtained with 
// cellGcmGetReport(). Moreover, tiles culled due to a cause other than the Z test are 
// also excluded.)
// In this algorithm, limit values for a scene are assumed to take several frames to 
// converge. Because of this, it is effective in scenes where the same scene continues 
// for a while - as in normal applications - and the operating efficiency of Zcull can be 
// enhanced over multiple frames.
void grcDevice::BeginAdaptiveZCulling(u32 MoveForward, u32& Dir, u32& Step, float& Prev, float RatioTilesCulled)
{
	if(RatioTilesCulled > Prev)
		Step = (u32)(Step * 1.5f);
	else
	{
		Step = (u32)(Step * 0.5f);
		Dir = -Dir;
	}

	Prev = RatioTilesCulled;

	MoveForward = (0x0001 > (MoveForward + Dir * Step))? 0x0001 : (MoveForward + Dir * Step);
	u32 PushBack = (int) ((0x0001 > (MoveForward * 0.5f))? 0x0001 : (MoveForward * 0.5f));
	GCM_DEBUG(GCM::cellGcmSetZcullLimit(GCM_CONTEXT,MoveForward, PushBack));
}

float grcDevice::EndAdaptiveZCulling()
{
	// I'm pretty sure this code wasn't really going to work correctly.
	// 
	// u32 NumTilesTested = (u32)cellGcmGetReport(CELL_GCM_ZCULL_STATS2, LABEL_ZCULL_STATS);
	// u32 NumTilesCulled = (u32)cellGcmGetReport(CELL_GCM_ZCULL_STATS3, LABEL_ZCULL_STATS2);

	// Note that the report is asynchronous but we're reading it immediately, which is crap.
	// So the cellGcmGetReport version must have done something like this internally but
	// with a huge-ass stall of some kind?
	Assertf(false,"This function is probably crap");
	cellGcmSetReport(GCM_CONTEXT,CELL_GCM_ZCULL_STATS2, LABEL_ZCULL_STATS2);
	u32 NumTilesTested = (u32)cellGcmGetReportDataLocation(LABEL_ZCULL_STATS2, CELL_GCM_LOCATION_MAIN);
	cellGcmSetReport(GCM_CONTEXT,CELL_GCM_ZCULL_STATS3, LABEL_ZCULL_STATS3);
	u32 NumTilesCulled = (u32)cellGcmGetReportDataLocation(LABEL_ZCULL_STATS3, CELL_GCM_LOCATION_MAIN);
	return (NumTilesTested ? (float) (NumTilesCulled / NumTilesTested) : 0.0f);
}

//
// Slope-Based Z culling functions
// This follows the description in the Run Time Library documentation in Chapter 10 ZCull
//
// This is an algorithm that calculates limit value from the Z directional slope of a 
// primitive belonging to a tile, to determine whether or not to update that Zcull tile. 
// The greater the slope is in the Z direction, the greater the limit value will be. 
// This means the range in which the primitive occludes the primitive in front of it will 
// be greater, and the range in which it occludes the primitive behind it will be less. 
// In the example below, the sum of the slopes obtained by cellGcmGetReport() and number 
// of tiles are used to calculate the average slope. This is used with the maximum slope 
// (also obtained by the above function) to calculate the range in which an object can 
// become an occluder. Given RSX(TM) characteristics, pushBackLimit takes a value similar to 
// moveForwardLimit. And considering that Zcull is updated when a tile is seen as being 
// part of the same object render, a relatively small value (half the moveForwardLimit 
// value) has been set for pushBackLimit below.
void grcDevice::BeginSlopeBasedZCulling(u32 AvgSlope, u32 MaxSlope)
{
	u32 MoveForward = (AvgSlope + MaxSlope) / 2;
	u32 PushBack = MoveForward / 2;

	if(MoveForward < 1)
		MoveForward = 1;
	if(PushBack < 1)
		PushBack = 1;

	GCM_DEBUG(GCM::cellGcmSetZcullLimit(GCM_CONTEXT,MoveForward, PushBack));
}

void grcDevice::EndSlopeBasedZCulling(u32& AvgSlope, u32& MaxSlope)
{
	MaxSlope = (u32)cellGcmGetReportDataLocation(LABEL_ZCULL_STATS2, CELL_GCM_LOCATION_MAIN);
	u32 SumSlope = (u32)cellGcmGetReportDataLocation(LABEL_ZCULL_STATS3, CELL_GCM_LOCATION_MAIN);

	u32 NumTiles = MaxSlope & 0xffff;
	MaxSlope = (MaxSlope >> 16) & 0xffff;
	AvgSlope = NumTiles ? SumSlope / NumTiles : 0;
}

void grcDevice::FreezeZCulling()
{
	// freeze Z culling
	// Zcull will keep the existing occluders and not attempt to create new ones (some holes in the occluder
	// mask may be filled, but the Z thresholds will not move).
	GCM_DEBUG(GCM::cellGcmSetZcullLimit(GCM_CONTEXT,0xFFFF, 0));
}

void grcDevice::InvalidateZCulling()
{
	// invalidate Z culling
	// Zcull information will be discarded in buffers
	GCM_DEBUG(GCM::cellGcmSetInvalidateZcull(GCM_CONTEXT));
}

#if HACK_GTA4
u32 g_AllowTexturePatch = 0;
#endif // HACK_GTA4

//
// patches a shadow to a depth buffer
//
// use with caution. This call tweaks values via a pointer ...
// needs to be called before the depth buffer is set ... if you call it afterwards the results are random
//
u8 grcDevice::PatchShadowToDepthBuffer(grcRenderTarget *_Buffer, bool patchSurfaceFormat)
{
#if HACK_GTA4
	grcAssertf(g_AllowTexturePatch, "Unverified gcmTexture remapping. Are you sure you know what you're doing?");
#endif // #if HACK_GTA4
	grcRenderTargetGCM *Buffer = (grcRenderTargetGCM*)_Buffer;
	grcAssertf(Buffer,"invalid depth buffer supplied");

	CellGcmTexture *gcmTexture=(CellGcmTexture*)Buffer->GetTexturePtr();

	u8 colorFormat = gcm::StripTextureFormat(gcmTexture->format);
	u8 colorFlags = gcmTexture->format & ~colorFormat;

	grcAssertf((colorFormat == CELL_GCM_TEXTURE_DEPTH24_D8) || (colorFormat == CELL_GCM_TEXTURE_DEPTH16) ,"invalid format?");

	u8 textureFormat = CELL_GCM_TEXTURE_A8R8G8B8;
	
	if(colorFormat == CELL_GCM_TEXTURE_DEPTH16)
	{
		grcAssertf(patchSurfaceFormat==false,"can't patch the surface for grctfD16 Rendertargets");
		textureFormat = CELL_GCM_TEXTURE_X16;
	}

#if GRCORE_ON_SPU > 1 
#if HACK_GTA4
	if (GCM_CONTEXT && (g_AllowTexturePatch == 0xFFFFFFFF)) 
#else // HACK_GTA4
	if (GCM_CONTEXT) 
#endif // HACK_GTA4
	{
		SPU_COMMAND(grcDevice__ChangeTextureRemap,static_cast<u8>(textureFormat | colorFlags));
#if USE_PACKED_GCMTEX
		cmd->texture = (u32) (s_spuGcmState.PackedTextureArray + Buffer->GetHandleIndex());
#else
		cmd->texture = (u32) gcmTexture;
#endif
		AssertMsg(textureFormat == CELL_GCM_TEXTURE_A8R8G8B8,"remap left unspecified, what should happen here?");
		if(textureFormat == CELL_GCM_TEXTURE_A8R8G8B8)
		{
			cmd->remap =	CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
							CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
							CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
							CELL_GCM_TEXTURE_REMAP_REMAP << 8 |
							CELL_GCM_TEXTURE_REMAP_FROM_G << 6 |
							CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
							CELL_GCM_TEXTURE_REMAP_FROM_A << 2 |
							CELL_GCM_TEXTURE_REMAP_FROM_B;
		
			if(patchSurfaceFormat)		// I think this is only used for debugging anyway
			{
				Buffer->SetSurfaceFormat(CELL_GCM_SURFACE_A8R8G8B8);
			}
		}
	}
	else
#endif
	{

		gcmTexture->format = static_cast<u8>(textureFormat | colorFlags);

		if(textureFormat == CELL_GCM_TEXTURE_A8R8G8B8)
		{
			if(patchSurfaceFormat)
			{
				Buffer->SetSurfaceFormat(CELL_GCM_SURFACE_A8R8G8B8);
			}

			gcmTexture->remap  =	CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
									CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
									CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
									CELL_GCM_TEXTURE_REMAP_REMAP << 8  |
									CELL_GCM_TEXTURE_REMAP_FROM_G << 6 |
									CELL_GCM_TEXTURE_REMAP_FROM_R << 4 |
									CELL_GCM_TEXTURE_REMAP_FROM_A << 2 |
									CELL_GCM_TEXTURE_REMAP_FROM_B;
			Buffer->UpdatePackedTexture();
		}
	}
	return colorFormat|colorFlags;
}

//
// patches a depth to a shadow buffer
//
// use with caution. This call tweaks values via a pointer ...
// needs to be called before the depth buffer is set ... if you call it afterwards the results are random
//
void grcDevice::PatchDepthToShadowBuffer(grcRenderTarget *_Buffer, u8 format, bool patchSurfaceFormat)
{
#if HACK_GTA4
	grcAssertf(g_AllowTexturePatch, "Unverified gcmTexture remapping. Are you sure you know what you're doing?");
#endif // #if HACK_GTA4
	grcRenderTargetGCM *Buffer = (grcRenderTargetGCM*)_Buffer;
	Assert(Buffer);

	CellGcmTexture *gcmTexture=(CellGcmTexture*)Buffer->GetTexturePtr();
	
	u8 newFormat = gcm::StripTextureFormat(format);
	Assertf((newFormat==CELL_GCM_TEXTURE_DEPTH24_D8) || (newFormat==CELL_GCM_TEXTURE_DEPTH16), "PatchDepthToShadowBuffer: newFormat expected to be DEPTH24_D8 or DEPTH16, but got 0x%02x", newFormat);

#if GRCORE_ON_SPU > 1
#if HACK_GTA4
	if (GCM_CONTEXT && (g_AllowTexturePatch == 0xFFFFFFFF)) 
#else // HACK_GTA4
	if (GCM_CONTEXT) 
#endif // HACK_GTA4
	{
		SPU_COMMAND(grcDevice__ChangeTextureRemap,format);
#if USE_PACKED_GCMTEX
		cmd->texture = (u32) (s_spuGcmState.PackedTextureArray + Buffer->GetHandleIndex());
#else
		cmd->texture = (u32) gcmTexture;
#endif
		AssertMsg(newFormat == CELL_GCM_TEXTURE_DEPTH24_D8,"remap left unspecified, what should happen here?");
		if(newFormat == CELL_GCM_TEXTURE_DEPTH24_D8)
		{
			cmd->remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
			CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
			CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
			CELL_GCM_TEXTURE_REMAP_REMAP << 8  |
			CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
			CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
			CELL_GCM_TEXTURE_REMAP_FROM_R << 2 |
			CELL_GCM_TEXTURE_REMAP_FROM_A;
			
			if(patchSurfaceFormat)
			{
				Buffer->SetSurfaceFormat(static_cast<u8>(gcm::TextureToSurfaceFormat(format)));
			}
		}
	}
	else
#endif
	{
		u8 currentFormat = gcm::StripTextureFormat(gcmTexture->format);
		Assertf((currentFormat==CELL_GCM_TEXTURE_A8R8G8B8) || (currentFormat==CELL_GCM_TEXTURE_X16), "PatchDepthToShadowBuffer: currentFormat expected to be A8R8G8B8 or X16, but got 0x%02x", currentFormat);
		
		gcmTexture->format = format;
		if(newFormat == CELL_GCM_TEXTURE_DEPTH24_D8)
		{
			if(patchSurfaceFormat)
			{
				Buffer->SetSurfaceFormat(static_cast<u8>(gcm::TextureToSurfaceFormat(format)));
			}

			gcmTexture->remap = CELL_GCM_TEXTURE_REMAP_REMAP << 14 |
								CELL_GCM_TEXTURE_REMAP_REMAP << 12 |
								CELL_GCM_TEXTURE_REMAP_REMAP << 10 |
								CELL_GCM_TEXTURE_REMAP_REMAP << 8  |
								CELL_GCM_TEXTURE_REMAP_FROM_B << 6 |
								CELL_GCM_TEXTURE_REMAP_FROM_G << 4 |
								CELL_GCM_TEXTURE_REMAP_FROM_R << 2 |
								CELL_GCM_TEXTURE_REMAP_FROM_A;	
			Buffer->UpdatePackedTexture();
		}
	}
}


//
// Capture a depth buffer screenshot on the PS3
//
// Here is an example on how to use it
//
// grcImage *image = GRCDEVICE.CaptureDepthBufferScreenshot(g_DepthBuffer);
//
// const char *shotName = "t:/depthbuffer.jpg";
// if (image)
// {
// 	image->SaveJPEG(shotName);
// 	image->Release();
// }
grcImage* grcDevice::CaptureDepthBufferScreenshot(grcRenderTarget *_DepthBuffer)
{
	grcRenderTargetGCM* DepthBuffer = static_cast<grcRenderTargetGCM*>(_DepthBuffer);
	Assert(DepthBuffer);

	grcImage *dest = NULL;
	CellGcmTexture *gcmTexture=reinterpret_cast<CellGcmTexture*>(DepthBuffer->GetTexturePtr());

	int	width=DepthBuffer->GetWidth();
	int	height=DepthBuffer->GetHeight();

	dest = grcImage::Create(width, height, 1, grcImage::A8R8G8B8, grcImage::STANDARD, 0, 0);

	// this does not lock a buffer ... just reading it: you are responsible for all side effects
	if(dest)
	{
		u32 *d = reinterpret_cast<u32*>(dest->GetBits());
		u32 *s = reinterpret_cast<u32*>(gcmTexture->location == CELL_GCM_LOCATION_LOCAL ? gcm::LocalPtr(DepthBuffer->GetMemoryOffset()) : gcm::MainPtr(DepthBuffer->GetMemoryOffset()));
		for (int row = 0; row < height; row++, s = (u32*)((char*)s + gcmTexture->pitch), d += width) {
			for (int col=0; col<width; col++)
				d[col] = ((s[col]&0xff)<<16)|((s[col]&0xff0000)>>16)|((s[col]&0xff00)) | 0xFF000000;
		}
		return dest;;
	}

	return NULL;
}


//
// Capture a depth buffer screenshot on the PS3
//
// Here is an example on how to use it
//
// grcImage *image = GRCDEVICE.CaptureDepthBufferScreenshot(g_DepthBuffer);
//
// const char *shotName = "t:/depthbuffer.jpg";
// if (image)
// {
// 	image->SaveJPEG(shotName);
// 	image->Release();
// }
grcImage* grcDevice::CaptureRenderTargetScreenshot(grcRenderTarget *_RenderTarget)
{
	grcRenderTargetGCM* RenderTarget = static_cast<grcRenderTargetGCM*>(_RenderTarget);
	Assert(RenderTarget);

	grcImage *dest = NULL;
	CellGcmTexture *gcmTexture = reinterpret_cast<CellGcmTexture*>(RenderTarget->GetTexturePtr());
	int	width=RenderTarget->GetWidth();
	int	height=RenderTarget->GetHeight();

	dest = grcImage::Create(width, height, 1, grcImage::A8R8G8B8, grcImage::STANDARD, 0, 0);

	// this does not lock a buffer ... just reading it: you are responsible for all side effects
	if(dest)
	{
		u32 *d = reinterpret_cast<u32*>(dest->GetBits());
		u32 *s = reinterpret_cast<u32*>(gcmTexture->location == CELL_GCM_LOCATION_LOCAL ? gcm::LocalPtr(RenderTarget->GetMemoryOffset()) : gcm::MainPtr(RenderTarget->GetMemoryOffset()));
		for (int row = 0; row < height; row++, s = (u32*)((char*)s + gcmTexture->pitch), d += width) {
			for (int col=0; col<width; col++)
				d[col] = ((s[col]&0xff)<<16)|((s[col]&0xff0000)>>16)|((s[col]&0xff00)) | 0xFF000000;
		}
		return dest;
	}

	return NULL;
}

grcImage* grcDevice::CaptureCubeFaceScreenshot(grcRenderTarget *_CubeRenderTarget, int face)
{
	grcRenderTargetGCM* CubeRenderTarget = static_cast<grcRenderTargetGCM*>(_CubeRenderTarget);
	Assert(CubeRenderTarget);

	grcImage *dest = NULL;
	CellGcmTexture *gcmTexture = reinterpret_cast<CellGcmTexture*>(CubeRenderTarget->GetTexturePtr());
	int	width=CubeRenderTarget->GetWidth();
	int	height=CubeRenderTarget->GetHeight();

	dest = grcImage::Create(width, height, 1, grcImage::A8R8G8B8, grcImage::STANDARD, 0, 0);

	// this does not lock a buffer ... just reading it: you are responsible for all side effects
	if(dest)
	{
		u32 *d = reinterpret_cast<u32*>(dest->GetBits());

		// calculate cube map offset value for face
		CubeRenderTarget->LockSurface(face);

		// get pointer to the cube map face
		u32 *s = reinterpret_cast<u32*>(gcmTexture->location == CELL_GCM_LOCATION_LOCAL ?
																gcm::LocalPtr(CubeRenderTarget->GetMemoryOffset()) :
																gcm::MainPtr(CubeRenderTarget->GetMemoryOffset()));

		for (int row = 0; row < height; row++, s = (u32*)((char*)s + gcmTexture->pitch), d += width) {
			for (int col=0; col<width; col++)
				d[col] = ((s[col]&0xff)<<16)|((s[col]&0xff0000)>>16)|((s[col]&0xff00)) | 0xFF000000;
		}
		return dest;
	}

	return NULL;
}

void grcDevice::SetFrameLock(int frameLock,bool /*swapImmediateIfLate*/)
{
	PARAM_frameLimit.Get(frameLock);

	sm_CurrentWindows[g_RenderThreadIndex].uFrameLock = frameLock;
	// s_SwapImmediateIfLate = swapImmediateIfLate;
	s_CappedFrameLock = frameLock? frameLock : 1;
}

int grcDevice::GetFrameLock() {
	return sm_CurrentWindows[g_RenderThreadIndex].uFrameLock;
}

void grcDevice::BlitText(int posx,int posy,float posz,const s16 *destxywh,const u8 *srcxywh,int count,Color32 color,bool /*bilinear*/) {
	

	// BeginBlit();
	
	int numRemaining = count*4;
	while (numRemaining > 0)
	{
		int numToProcess = numRemaining > grcBeginMax ? grcBeginMax : numRemaining;
		numRemaining -= numToProcess;

		grcBegin(drawQuads, numToProcess);
		grcColor(color);
		const grcTexture *tex = grcGetTexture();
		float itu = 1.0f, itv = 1.0f;
		if (tex) {
			itu = 1.0f / float(tex->GetWidth());
			itv = 1.0f / float(tex->GetHeight());
		}

		while (numToProcess > 0) {
			float x1 = posx+(destxywh[0]>>4), y1 = posy+(destxywh[1]>>4),
				x2 = posx+((destxywh[0]+destxywh[2])>>4), y2 = posy+((destxywh[1]+destxywh[3])>>4),
				u1 = srcxywh[0] * itu, v1 = srcxywh[1] * itv,
				u2 = (srcxywh[0]+srcxywh[2]) * itu, v2 = (srcxywh[1]+srcxywh[3]) * itv;
			grcTexCoord2f(u1,v1);
			grcVertex3f(x1,y1,posz);
			grcTexCoord2f(u1,v2);
			grcVertex3f(x1,y2,posz);
			grcTexCoord2f(u2,v2);
			grcVertex3f(x2,y2,posz);
			grcTexCoord2f(u2,v1);
			grcVertex3f(x2,y1,posz);
			destxywh += 4;
			srcxywh += 4;
			numToProcess -=4;
		}
		grcEnd();
	}	

	// EndBlit();
}

void grcDevice::MsaaResolve(grcRenderTarget * _target,		// target render target
							grcRenderTarget * _source,		// source render target
							int ,			// texture atlas face
							bool compositeTransparent)		// is this a transparent composite?
{
	grcRenderTargetGCM* target = static_cast<grcRenderTargetGCM*>(_target);
	grcRenderTargetGCM* source = static_cast<grcRenderTargetGCM*>(_source);

	Assert(target && source);
	Assertf(!target->GetMSAA(), "The target \"%s\" cannot be multisampled", target->GetName());
	Assertf(source->GetMSAA(), "The source \"%s\" must be multisampled", source->GetName());
	Assert(source->GetMipMapCount() == 1);
	Assertf(source->GetLayerCount() == target->GetLayerCount(), "(source: %s, target: %s) %d == %d", source->GetName(), target->GetName(), source->GetLayerCount(), target->GetLayerCount());
	Assertf(source->GetType() == grcrtBackBuffer || source->GetType() == grcrtFrontBuffer || source->GetType() == target->GetType(), "(source: %s, target: %s) %d == %d", source->GetName(), target->GetName(), source->GetType(), target->GetType());
	Assert(source->GetType() != grcrtVolume); // No support for this just yet

	static const Color32 white(1.0f,1.0f,1.0f,1.0f);

	grcEffectTechnique technique;

	float du = 0.0f;
	float dv = 0.0f;

	const bool isDepth = target->GetType() == grcrtDepthBuffer || target->GetType() == grcrtShadowMap || target->GetType() == grcrtDepthBufferCubeMap;
	g_DefaultEffect->SetVar(s_MsaaPointSampler, source);

	// lock the render target by setting it
	u32 oldLockedLayer = target->GetLockedLayer();
	u32 oldLockedMip = target->GetLockedMip();
	target->LockSurface(source->GetLockedLayer(), 0);

	u8 depthTextureFormat = 0;

	if (isDepth)
	{
		technique = s_CopyDepthTechnique;
		g_DefaultEffect->SetVar(s_DepthPointSampler, source);
		SetRenderTarget(NULL, target);

		GRCDEVICE.Clear(false, Color32(0, 0, 0, 0), true, 1.0f, true, 0);

		grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_LessEqual);
		grcStateBlock::Flush();

		depthTextureFormat = GRCDEVICE.PatchShadowToDepthBuffer(source, false);
	}
	else
	{
		// depending on if MSAA is on, pick technique
		switch (source->GetMSAA())
		{
		case MSAA_2xMS:
			{
				Assertf(target->GetWidth() * 2 == source->GetWidth(), "target: \"%s\" (%dx%d), source: \"%s\" (%dx%d)", target->GetName(), target->GetWidth(), target->GetHeight(), source->GetName(), source->GetWidth(), source->GetHeight());

				if (compositeTransparent)
				{
					g_DefaultEffect->SetVar(s_TransparentSrcSampler, source);
					g_DefaultEffect->SetVar(s_TransparentDstSampler, target);
					technique = s_Msaa2xTransparentTechnique;
				}
				else
				{
#if RAGE_ENCODEOPAQUECOLOR
					technique = s_Msaa2xOpaqueTechnique;
#else
					switch (s_Msaa2xMode)
					{
					case kQuincunx:
						{
							du = 0.25f / static_cast<float>(source->GetWidth()); // half width of one texel
							du *= s_MsaaShiftPlusMinus;

							technique = s_Msaa2xQuincunxTechnique;
							g_DefaultEffect->SetVar(s_MsaaQuincunxSampler, source);
						}
						break;
					case kQuincunxAlt:
						{
							du = -0.25f / static_cast<float>(source->GetWidth()); // half width of one texel
							dv = -0.5f / static_cast<float>(source->GetWidth());  // one texel
							dv *= s_MsaaShiftPlusMinus;

							technique = s_Msaa2xQuincunxAltTechnique;
							g_DefaultEffect->SetVar(s_MsaaQuincunxAltSampler, source);
						}
						break;
					case kAccuview:
						{
							technique = s_Msaa2xAccuviewTechnique;
							g_DefaultEffect->SetVar(s_MsaaQuincunxSampler, source);
							g_DefaultEffect->SetVar(s_MsaaQuincunxAltSampler, source);
						}
						break;
					case k2Tap:
						{
							technique = s_Msaa2x2TapTechnique;
						}
						break;
					default:
						grcErrorf("Invalid MSAA mode");
						return;
					}
#endif // RAGE_ENCODEOPAQUECOLOR
				}
			}
			break;
		case MSAA_Centered4xMS:
		case MSAA_Rotated4xMS:
			{
				Assertf(target->GetWidth() * 2 == source->GetWidth(), "target: \"%s\" (%dx%d), source: \"%s\" (%dx%d)", target->GetName(), target->GetWidth(), target->GetHeight(), source->GetName(), source->GetWidth(), source->GetHeight());
				Assertf(target->GetHeight() * 2 == source->GetHeight(), "target: \"%s\" (%dx%d), source: \"%s\" (%dx%d)", target->GetName(), target->GetWidth(), target->GetHeight(), source->GetName(), source->GetWidth(), source->GetHeight());

				if (compositeTransparent)
				{
					g_DefaultEffect->SetVar(s_TransparentSrcSampler, source);
					g_DefaultEffect->SetVar(s_TransparentDstSampler, target);
					technique = s_Msaa4xTransparentTechnique;
				}
				else
				{
					technique = s_Msaa4xTechnique;
				}
#if !RAGE_ENCODEOPAQUECOLOR
				g_DefaultEffect->SetVar(s_MsaaGaussianSampler, source);
#endif // RAGE_ENCODEOPAQUECOLOR
			}
			break;
		default:
			{
				if (compositeTransparent)
				{
					technique = s_CopyTransparentTechnique;
				}
				else
				{
					technique = s_CopyTechnique;
				}
			}
			break;
		}

		SetRenderTarget(target, NULL);
	}

	// provide the size of the target or source render target to the shader
	const u32 width = source->GetWidth();
	const u32 height = source->GetHeight();
	g_DefaultEffect->SetVar(s_TexelSize, Vector4(1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height), static_cast<float>(width), static_cast<float>(height)));

	// set the technique
	int numPasses = g_DefaultEffect->BeginDraw(technique, true);
	Assert(numPasses > 0);

	for (int i = 0; i < numPasses; ++i)
	{
		g_DefaultEffect->BeginPass(i);

		grcDrawSingleQuadf(
			-1.0,						// x1 - Destination base x
			1.0,						// y1 - Destination base y
			1.0,						// x2 - Destination opposite-corner x
			-1.0,						// y1 - Destination opposite-corner y
			0.1f,
			du,
			du,
			1.0f + dv,
			1.0f + dv,
			white);

		g_DefaultEffect->EndPass();
	}

	g_DefaultEffect->EndDraw();

	if (isDepth)
	{
		GRCDEVICE.PatchDepthToShadowBuffer(source, depthTextureFormat, false);
	}

	target->LockSurface(oldLockedLayer, oldLockedMip);

	//	release texture again
	g_DefaultEffect->SetVar(s_MsaaQuincunxSampler, (grcTexture*)grcTexture::None);
	g_DefaultEffect->SetVar(s_MsaaQuincunxAltSampler, (grcTexture*)grcTexture::None);
	g_DefaultEffect->SetVar(s_MsaaGaussianSampler, (grcTexture*)grcTexture::None);
	g_DefaultEffect->SetVar(s_MsaaPointSampler, (grcTexture*)grcTexture::None);

	if (compositeTransparent)
	{
		g_DefaultEffect->SetVar(s_TransparentSrcSampler, (grcTexture*)grcTexture::None);
		g_DefaultEffect->SetVar(s_TransparentDstSampler, (grcTexture*)grcTexture::None);
	}

	if (GetMSAA() && source == g_BackBuffer)
		s_LastMsaaResolveFrame = GetFrameCounter();
}

void grcDevice::MsaaResolveBackToFrontBuffer(bool force /*= false*/, bool compositeTransparent /*= false*/)
{
	if (GetMSAA() && (s_LastMsaaResolveFrame != GetFrameCounter() || force))
	{
		s_LastMsaaResolveFrame = GetFrameCounter();
		grcTextureFactory& textureFactory = grcTextureFactory::GetInstance();
		MsaaResolve(textureFactory.GetFrontBuffer(), textureFactory.GetBackBuffer(false), -1, compositeTransparent);
	}
}

void grcDevice::MsaaResolveTouch()
{
	s_LastMsaaResolveFrame = GetFrameCounter();
}

void grcDevice::SetRenderTarget(const grcRenderTarget *color, const grcRenderTarget *depthStencil)
{
	const grcRenderTarget *colors[] = { color, 0, 0, 0 };
	CompileTimeAssert(sizeof(colors) / sizeof(colors[0]) == grcmrtColorCount);
	SetRenderTargets(colors, depthStencil);
}

//
// sets the rendering buffer in cellGcmSetSurface()
//
void grcDevice::SetRenderTargets(const grcRenderTarget* color[grcmrtColorCount], const grcRenderTarget* _depthStencil)
{
	// from state_gcm.cpp
	extern void BlockColorWriteEnable(bool value);
	extern void BlockDepthAccess(bool value);
	extern void BlockAlphaTest(bool value);
	extern void BlockAlphaToMask(bool isFloatingPointColorSurfaceFormat, bool hasMrt, bool multisampleEnable);
	extern void BlockStencilAccess(bool value);
	extern u16 g_RenderStateBlockMask;

	// Convert to platform specific grcRenderTargetGCM pointers and get their
	// corresponding CellGcmTexture pointers
#if ENABLE_PIX_TAGGING
	char nameBuffer[64*grcmrtCount] = { '\0' };
	char* currNameBuffer = nameBuffer;
#endif // ENABLE_PIX_TAGGING

	const grcRenderTargetGCM* renderTarget[grcmrtCount];
	const CellGcmTexture* gcmTexture[grcmrtCount];
	{
		for (u32 i = 0; i < grcmrtColorCount; ++i)
		{
			renderTarget[i] = static_cast<const grcRenderTargetGCM*>(color[i]);
			if (renderTarget[i])
			{
				gcmTexture[i] = reinterpret_cast<const CellGcmTexture*>(renderTarget[i]->GetTexturePtr());
#if ENABLE_PIX_TAGGING
				formatf(currNameBuffer, sizeof(nameBuffer) - (currNameBuffer - nameBuffer), "%d: %s ", i, renderTarget[i]->GetName());
				currNameBuffer += strlen(currNameBuffer);
#endif // ENABLE_PIX_TAGGING
			}
			else
			{
				gcmTexture[i] = NULL;
			}
		}

		if (_depthStencil)
		{
			renderTarget[grcmrtDepthStencil] = static_cast<const grcRenderTargetGCM*>(_depthStencil);
			gcmTexture[grcmrtDepthStencil] = static_cast<const CellGcmTexture*>(_depthStencil->GetTexturePtr());

#if ENABLE_PIX_TAGGING
			formatf(currNameBuffer, sizeof(nameBuffer) - (currNameBuffer - nameBuffer), "d: %s", renderTarget[grcmrtDepthStencil]->GetName());
			currNameBuffer += strlen(currNameBuffer);
#endif // ENABLE_PIX_TAGGING
		}
		else
		{
			renderTarget[grcmrtDepthStencil] = NULL;
			gcmTexture[grcmrtDepthStencil] = NULL;
		}
	}

	// if nothing has changed compared to last time ... bail out and don't flush the graphics card with cellGcmSetSurface() :-)
	BANK_ONLY(g_AreRenderTargetsBound = true;)//This is for debug tracking of rendering without an appropriate target bound
	bool areRenderTargetsBound = true;
	for (u32 i = 0; i < grcmrtCount; ++i)
	{
		if (areRenderTargetsBound)
		{
			areRenderTargetsBound &= (s_RenderTargetState[i] == renderTarget[i]);
		}
		s_RenderTargetState[i] = renderTarget[i];
	}
	if (areRenderTargetsBound)
	{
		return;
	}

#if ENABLE_PIX_TAGGING
	cellGcmSetPerfMonMarker(GCM_CONTEXT,nameBuffer);
#endif // ENABLE_PIX_TAGGING

	MSAAMode MsaaMode = 0;
	CellGcmEnum hasColorCompression = CELL_GCM_FALSE;

	u8 colorSurfaceFormat = 0;
	bool isFloatingPointColorSurfaceFormat = false;
	bool isSupportsAlphaTest = true;
	bool hasMrt = false;

	grcStateBlock::FlushThrough(true);

	// Make sure all formats match, requirement on PS3 at least.
	if (gcmTexture[grcmrtColor0])
	{
		colorSurfaceFormat = renderTarget[grcmrtColor0]->GetSurfaceFormat();
		isFloatingPointColorSurfaceFormat = gcm::IsFloatingPointColorSurfaceFormat(colorSurfaceFormat);
		if ( isFloatingPointColorSurfaceFormat )
		{
			// 16/16/16/16f *does* support alpha test, others do not
			isSupportsAlphaTest = CELL_GCM_SURFACE_F_W16Z16Y16X16 == colorSurfaceFormat ? true : false;
		}

		// Let's hope this isn't really necessary any longer.
		/* g_MaxAlphaRef = colorSurfaceFormat == CELL_GCM_SURFACE_F_W16Z16Y16X16 ? 65535 : 255;
		if (grcState::GetAlphaRef() > g_MaxAlphaRef)
		{
			grcState::SetState(grcsAlphaRef, g_MaxAlphaRef);
		} */

		GCM::cellGcmSetFragmentProgramGammaEnable(GCM_CONTEXT, renderTarget[grcmrtColor0]->IsGammaEnabled());

#		if __ASSERT
			u32 colorSurfaceBitsPerPixel = gcm::SurfaceFormatBitsPerPixel(colorSurfaceFormat, false);
			u8 depthStencilSurfaceFormat = 0;
			u32 textureFormatBitsPerPixel = gcm::TextureFormatBitsPerPixel(gcmTexture[0]->format);
			if (gcm::StripTextureFormat(gcmTexture[0]->format) == CELL_GCM_TEXTURE_COMPRESSED_DXT1)
				textureFormatBitsPerPixel = 32; // surface for a dxt1 should be 32bit
			Assertf(textureFormatBitsPerPixel == colorSurfaceBitsPerPixel, "Surface/texture bpp mismatch %d != %d", textureFormatBitsPerPixel, colorSurfaceBitsPerPixel);
			bool isSwizzled = renderTarget[grcmrtColor0]->IsSwizzled();
			u32 desiredPitch = renderTarget[grcmrtColor0]->IsTiled() ? gcm::GetSurfaceTiledPitch(gcmTexture[0]->width, textureFormatBitsPerPixel, isSwizzled) : gcm::GetSurfacePitch(gcmTexture[0]->width, textureFormatBitsPerPixel, isSwizzled);
			Assertf(!renderTarget[grcmrtColor0]->IsTiled() || desiredPitch <= renderTarget[grcmrtColor0]->GetPitch(), "Incorrect pitch specified for tiled render target \"%s\" (%d <= %d)", renderTarget[grcmrtColor0]->GetName(), desiredPitch, renderTarget[grcmrtColor0]->GetPitch());
			u32 colorFormat = gcm::StripTextureFormat(gcmTexture[grcmrtColor0]->format);
			u32 colorFlags = gcmTexture[grcmrtColor0]->format & ~colorFormat;

			u32 hasMrtMask = 0;
			for (u32 i = 1; i < grcmrtColorCount; ++i)
			{
				hasMrtMask |= reinterpret_cast<u32>(renderTarget[i]);
				if (renderTarget[i])
				{
					Assertf(!renderTarget[grcmrtDepthStencil] || renderTarget[grcmrtDepthStencil]->GetMSAA() == renderTarget[i]->GetMSAA(), "All bound render targets must use the same multisampling mode (\"%s\" %d) (\"%s\" %d)", renderTarget[grcmrtDepthStencil]->GetName(), static_cast<int>(renderTarget[grcmrtDepthStencil]->GetMSAA()), renderTarget[i]->GetName(), static_cast<int>(renderTarget[i]->GetMSAA()));
					AssertMsg(!renderTarget[i] ||
						(renderTarget[i]->IsSwizzled() == renderTarget[grcmrtColor0]->IsSwizzled() &&
						renderTarget[i]->IsTiled() == renderTarget[grcmrtColor0]->IsTiled() &&
						renderTarget[i]->GetSurfaceFormat() == colorSurfaceFormat &&
						renderTarget[i]->GetWidth() == renderTarget[grcmrtColor0]->GetWidth() &&
						renderTarget[i]->GetHeight() == renderTarget[grcmrtColor0]->GetHeight()), "All MRTs must be the same format and dimensions");
					AssertMsg((gcmTexture[i]->format & colorFlags) != 0, "Mismatching swizzle and normalisation flags");
					AssertMsg(gcmTexture[i]->_padding == gcmTexture[grcmrtColor0]->_padding, "Mismatching gamma correction flags");
				}
			}
			if (renderTarget[grcmrtDepthStencil])
			{
				depthStencilSurfaceFormat = renderTarget[grcmrtDepthStencil]->GetSurfaceFormat();
				AssertMsg(colorFormat != CELL_GCM_TEXTURE_X32_FLOAT || depthStencilSurfaceFormat == CELL_GCM_SURFACE_Z24S8, "Invalid color/depth format combination - see RSX users manual pg. 84");
				AssertMsg(colorFormat != CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT || depthStencilSurfaceFormat == CELL_GCM_SURFACE_Z24S8, "Invalid color/depth format combination - see RSX users manual pg. 84");
				AssertMsg(colorFormat != CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT || depthStencilSurfaceFormat == CELL_GCM_SURFACE_Z24S8, "Invalid color/depth format combination - see RSX users manual pg. 84");
				AssertMsg(colorFormat != CELL_GCM_TEXTURE_B8 || hasMrtMask == 0, "Cannot use B8 render target with MRT - see RSX users manual pg. 84");
				AssertMsg(renderTarget[grcmrtDepthStencil]->GetMSAA() == renderTarget[grcmrtColor0]->GetMSAA(), "All bound render targets must use the same multisampling mode");
				AssertMsg(colorSurfaceFormat != CELL_GCM_SURFACE_F_W32Z32Y32X32 || gcmTexture[grcmrtDepthStencil]->location == CELL_GCM_LOCATION_LOCAL, "When the color format is CELL_GCM_SURFACE_F_W32Z32Y32X32 (128 bits/pixel), the depth buffer cannot be places in main memory - see RSX users manual pg. 84");
				Assert(renderTarget[grcmrtDepthStencil]->GetWidth() == renderTarget[grcmrtColor0]->GetWidth());
				Assert(renderTarget[grcmrtDepthStencil]->GetHeight() == renderTarget[grcmrtColor0]->GetHeight());
			}

#		endif // __ASSERT

		MsaaMode = renderTarget[grcmrtColor0]->GetMSAA();
		AssertMsg(!MsaaMode || !isFloatingPointColorSurfaceFormat, "The RSX does not support multisampling for floating point colour surfaces");

		hasColorCompression = (renderTarget[grcmrtColor0]->IsTiled() && gcm::TextureFormatBitsPerPixel(gcmTexture[grcmrtColor0]->format) == 32) ? CELL_GCM_TRUE : CELL_GCM_FALSE;

		InvalidateSpuGcmState(CachedStates.Blend.ColorMask,    ~0);
		InvalidateSpuGcmState(CachedStates.Blend.ColorMaskMrt, ~0);

		GCM_STATE(SetColorMask,CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A);
		if (gcmTexture[grcmrtColor3])
		{
			hasMrt = true;
			g_GcmSurface.colorTarget = CELL_GCM_SURFACE_TARGET_MRT3;
			GCM_STATE(SetColorMaskMrt,
				CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B | CELL_GCM_COLOR_MASK_MRT1_A |
				CELL_GCM_COLOR_MASK_MRT2_R | CELL_GCM_COLOR_MASK_MRT2_G | CELL_GCM_COLOR_MASK_MRT2_B | CELL_GCM_COLOR_MASK_MRT2_A |
				CELL_GCM_COLOR_MASK_MRT3_R | CELL_GCM_COLOR_MASK_MRT3_G | CELL_GCM_COLOR_MASK_MRT3_B | CELL_GCM_COLOR_MASK_MRT3_A);
		}
		else if (gcmTexture[grcmrtColor2])
		{
			hasMrt = true;
			g_GcmSurface.colorTarget = CELL_GCM_SURFACE_TARGET_MRT2;
			GCM_STATE(SetColorMaskMrt,
				CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B | CELL_GCM_COLOR_MASK_MRT1_A |
				CELL_GCM_COLOR_MASK_MRT2_R | CELL_GCM_COLOR_MASK_MRT2_G | CELL_GCM_COLOR_MASK_MRT2_B | CELL_GCM_COLOR_MASK_MRT2_A);
		}
		else if (gcmTexture[grcmrtColor1])
		{
			if (gcmTexture[grcmrtColor0])
			{
				hasMrt = true;
				g_GcmSurface.colorTarget = CELL_GCM_SURFACE_TARGET_MRT1;
			}
			else
			{
				hasMrt = false;
				g_GcmSurface.colorTarget = CELL_GCM_SURFACE_TARGET_1;
			}
			GCM_STATE(SetColorMaskMrt,
				CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B | CELL_GCM_COLOR_MASK_MRT1_A);
		}
		else
		{
			Assert(gcmTexture[grcmrtColor0]);
			hasMrt = false;
			g_GcmSurface.colorTarget = CELL_GCM_SURFACE_TARGET_0;
			GCM_STATE(SetColorMask,
				CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A);
		}

		if (renderTarget[grcmrtColor0]->IsSwizzled())
		{
			Assert((gcmTexture[grcmrtColor0]->format & CELL_GCM_TEXTURE_LN) == 0);
			AssertMsg(!renderTarget[grcmrtDepthStencil] || gcm::SurfaceFormatBitsPerPixel(depthStencilSurfaceFormat, true) == colorSurfaceBitsPerPixel, "When rendering to a swizzled render target the pixel size for the colour and depth must be the same - see LibGCM Reference pg. 14");
			AssertMsg(!MsaaMode, "Multisampling does not work with swizzled render targets - see RSX users manual pg. 85");

			g_GcmSurface.type = CELL_GCM_SURFACE_SWIZZLE;
			for (u32 i = 0; i < grcmrtColorCount; ++i)
			{
				g_GcmSurface.colorPitch[i] = 64;
			}
		}
		else
		{
			AssertMsg(!isFloatingPointColorSurfaceFormat || !renderTarget[grcmrtDepthStencil] || depthStencilSurfaceFormat == CELL_GCM_SURFACE_Z24S8, "All floating point color render targets must use a D24S8 depth buffer - see LibGCM Reference pg. 15");

			g_GcmSurface.type = CELL_GCM_SURFACE_PITCH;
			for (u32 i = 0; i < grcmrtColorCount; ++i)
			{
				g_GcmSurface.colorPitch[i] = gcmTexture[i] ? renderTarget[i]->GetPitch() : 64;
			}
		}

		g_GcmSurface.colorFormat = colorSurfaceFormat;
		for (u32 i = 0; i < grcmrtColorCount; ++i)
		{
			if (gcmTexture[i])
			{
				g_GcmSurface.colorLocation[i] = gcmTexture[i]->location;
				g_GcmSurface.colorOffset[i] = renderTarget[i]->GetMemoryOffset();
				AssertMsg(!gcmTexture[i]->cubemap || g_GcmSurface.colorOffset[i] % 128 == 0, "Memory offset for cube maps must be 128 byte aligned");
#if __BANK
				grcRenderTarget::LogTexture("SetRenderTarget",renderTarget[i]);
#endif
			}
			else
			{
				// Make sure that the disabled MRTs are always pointing to the same memory type
				// as the enabled MRTs ** hardware bug **
 				g_GcmSurface.colorLocation[i] = gcmTexture[grcmrtColor0]->location;
 				g_GcmSurface.colorOffset[i] = renderTarget[grcmrtColor0]->GetMemoryOffset();
			}
		}
	}
	else
	{
		AssertMsg(renderTarget[grcmrtDepthStencil], "You must have a color or a depth buffer bound");

		// No color buffers should be bound
		g_GcmSurface.colorFormat = CELL_GCM_SURFACE_A8B8G8R8;
		g_GcmSurface.colorTarget = CELL_GCM_SURFACE_TARGET_NONE;
		g_GcmSurface.type = CELL_GCM_SURFACE_PITCH;
		for (u32 i = 0; i < grcmrtColorCount; ++i)
		{
			g_GcmSurface.colorLocation[i] = CELL_GCM_LOCATION_LOCAL;
			g_GcmSurface.colorOffset[i] = 0;
			g_GcmSurface.colorPitch[i] = 64;
		}

		GCM_STATE(SetColorMask, 0);
	}

	if (gcmTexture[grcmrtDepthStencil])
	{
		AssertMsg(!gcmTexture[grcmrtColor0] || (g_GcmSurface.colorFormat != CELL_GCM_SURFACE_B8 && g_GcmSurface.colorFormat != CELL_GCM_SURFACE_G8B8), "Can't have B8 or G8B8 surface format when stencil testing, depth testing or depth bounds testing.");
#		if __ASSERT
			for (u32 i = 0; i < grcmrtColorCount; ++i)
			{
				AssertMsg(!renderTarget[i] || renderTarget[i]->GetMSAA() == renderTarget[grcmrtDepthStencil]->GetMSAA(), "All bound render targets must use the same multisampling mode");
			}
#		endif // __ASSERT

#if __BANK
		grcRenderTarget::LogTexture("SetRenderTargets",renderTarget[grcmrtDepthStencil]);
#endif // __BANK
		MsaaMode = renderTarget[grcmrtDepthStencil]->GetMSAA();

		g_GcmSurface.depthFormat = renderTarget[grcmrtDepthStencil]->GetSurfaceFormat();
		Assertf(g_GcmSurface.depthFormat == CELL_GCM_SURFACE_Z16 || g_GcmSurface.depthFormat == CELL_GCM_SURFACE_Z24S8, "\"%s\" %x", renderTarget[grcmrtDepthStencil]->GetName(), g_GcmSurface.depthFormat);

		const u8 textureFormat = static_cast<u8>(gcm::StripTextureFormat(gcmTexture[grcmrtDepthStencil]->format));
		switch (textureFormat)
		{
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			{
				GCM_STATE(SetDepthFormat,(g_DepthFormatType=CELL_GCM_DEPTH_FORMAT_FLOAT));
			}
			break;
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH16:
			{
				GCM_STATE(SetDepthFormat,(g_DepthFormatType=CELL_GCM_DEPTH_FORMAT_FIXED));
			}
			break;
		default:
			{
				if (g_grcDepthFormat == g_grcDefaultFixedDepthFormat)
				{
					GCM_STATE(SetDepthFormat,(g_DepthFormatType=CELL_GCM_DEPTH_FORMAT_FIXED));
				}
				else
				{
					// Sanity check
					Assert(g_grcDepthFormat == g_grcDefaultFloatDepthFormat);
					GCM_STATE(SetDepthFormat,(g_DepthFormatType=CELL_GCM_DEPTH_FORMAT_FLOAT));
				}
			}
			break;
		}

		g_GcmSurface.depthLocation = gcmTexture[grcmrtDepthStencil]->location;
		g_GcmSurface.depthOffset = renderTarget[grcmrtDepthStencil]->GetMemoryOffset();
		g_GcmSurface.depthPitch = renderTarget[grcmrtDepthStencil]->GetPitch();
	}
	else
	{
		AssertMsg(renderTarget[grcmrtColor0], "You must have a color or a depth buffer bound");

		g_GcmSurface.depthFormat = CELL_GCM_SURFACE_Z24S8;
		g_GcmSurface.depthLocation = CELL_GCM_LOCATION_LOCAL;
		g_GcmSurface.depthOffset = 0;
		g_GcmSurface.depthPitch = 64;
	}

	g_GcmSurface._padding[0] = g_GcmSurface._padding[1] = 0;

	g_GcmSurface.x = 0;
	g_GcmSurface.y = 0;

	const grcRenderTargetGCM *surfaceTarget = renderTarget[grcmrtDepthStencil]? renderTarget[grcmrtDepthStencil] : renderTarget[grcmrtColor0];
	Assert(surfaceTarget);
	g_GcmSurface.width = Max(surfaceTarget->GetWidth() >> surfaceTarget->GetLockedMip(), 1);
	g_GcmSurface.height = Max(surfaceTarget->GetHeight() >> surfaceTarget->GetLockedMip(), 1);

	u16 oldRenderStateBlockMask = g_RenderStateBlockMask;

	if (!gcmTexture[grcmrtDepthStencil])
		g_RenderStateBlockMask |= BLOCK_DEPTHACCESS | BLOCK_STENCILACCESS;
	else
		g_RenderStateBlockMask &= ~(BLOCK_DEPTHACCESS | BLOCK_STENCILACCESS);
	if (!gcmTexture[grcmrtColor0])
		g_RenderStateBlockMask |= BLOCK_COLORWRITE;
	else
		g_RenderStateBlockMask &= ~BLOCK_COLORWRITE;

	// Enable/disable hardware multisampling support
	CellGcmEnum multisamplingBlendOptimizationEnabled = CELL_GCM_FALSE;
	switch (MsaaMode)
	{
	case MSAA_2xMS:
		{
			g_GcmSurface.antialias = CELL_GCM_SURFACE_DIAGONAL_CENTERED_2;
			g_GcmSurface.width /= 2;
		}
		break;
	case MSAA_Centered4xMS:
		{
			g_GcmSurface.antialias = CELL_GCM_SURFACE_SQUARE_CENTERED_4;
			multisamplingBlendOptimizationEnabled = hasColorCompression;
			g_GcmSurface.width /= 2;
			g_GcmSurface.height /= 2;
		}
		break;
	case MSAA_Rotated4xMS:
		{
			g_GcmSurface.antialias = CELL_GCM_SURFACE_SQUARE_ROTATED_4;
			multisamplingBlendOptimizationEnabled = hasColorCompression;
			g_GcmSurface.width /= 2;
			g_GcmSurface.height /= 2;
		}
		break;
	default:
		{
			g_GcmSurface.antialias = CELL_GCM_SURFACE_CENTER_1;
		}
		break;
	}

	if ((isFloatingPointColorSurfaceFormat && !isSupportsAlphaTest) || hasMrt)
		g_RenderStateBlockMask |= BLOCK_ALPHATEST;
	else
		g_RenderStateBlockMask &= ~BLOCK_ALPHATEST;

	if (isFloatingPointColorSurfaceFormat)
	{
		g_RenderStateBlockMask |= BLOCK_ALPHATOMASK;
		g_RenderStateBlockMask &= ~MSAA_ENABLED;
	}
	else
	{
		if (hasMrt)
			g_RenderStateBlockMask |= BLOCK_ALPHATOMASK;
		else
			g_RenderStateBlockMask &= ~BLOCK_ALPHATOMASK;
		if (GetMultiSampleFlag())
			g_RenderStateBlockMask |= MSAA_ENABLED;
		else
			g_RenderStateBlockMask &= ~MSAA_ENABLED;
	}

	// Send the block mask down if it has changed
	if (oldRenderStateBlockMask != g_RenderStateBlockMask) 
	{
		SPU_COMMAND(grcEffect__RenderStateBlockMask,0);
		cmd->Mask = g_RenderStateBlockMask;
	}

	SetSize(g_GcmSurface.width, g_GcmSurface.height);
	// refresh the window data to make the renderer happy
	grcWindow temp;
	SetWindow(temp);

	grcStateBlock::FlushThrough(true);

	// registers the rendering buffer
	GCM_DEBUG(GCM::cellGcmSetSurfaceWindow(GCM_CONTEXT, &g_GcmSurface, PS3_SHADER_VPOS_WINDOW_ORIGIN, PS3_SHADER_VPOS_PIXEL_CENTER));

	// This function specifies to enable or disable blend optimization when using a 4X multi-sample.
	// The blend function can only be used when a 4x multi-sample, as well as color compression, are enabled.
	// When enabled, pixel blending will be processed in units of 4 samples. The destination color will be
	// read by ROP as an averaged pixel, and blended with the source color. This enables the ROP's blend
	// performance, which took 8 clocks per title, to improve to take only 2 clocks per title. For some
	// renderings, however, results will be unexpected. In such a case, disable the blend optimization.
	GCM_STATE(SetBlendOptimization,multisamplingBlendOptimizationEnabled);

	// Invalidate all currently tracked state blocks when block mask is changing
	grcStateBlock::MakeDirty();

	// These states need to be flushed when we (potentially) change surface format
	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_Active,grcStateBlock::ActiveStencilRef);
	grcStateBlock::SetBlendState(grcStateBlock::BS_Active);
	grcStateBlock::FlushThrough(true);

#if GRCORE_ON_SPU
	SPU_COMMAND(grcDevice__SetSurface, 0);
	cmd->colorFormat = g_GcmSurface.colorFormat;
	cmd->depthFormat = g_GcmSurface.depthFormat;
	cmd->depthFormatType = g_DepthFormatType;
#endif // GRCORE_ON_SPU
}

#if !HACK_GTA4 // no shader clip planes
static inline void LookForClipPlanesGlobalVar()
{
	if (!s_ClipPlanesGlobalVarLookedUp)
	{
		s_ClipPlanesGlobalVar = grcEffect::LookupGlobalVar("ClipPlanes", false);
		s_ClipPlanesGlobalVarLookedUp = true;
	}
}
#endif // !HACK_GTA4

u32 grcDevice::SetClipPlaneEnable(u32 enable) {
#if !HACK_GTA4 // no shader clip planes

	Assert(enable <= 0x3f);
	LookForClipPlanesGlobalVar();
	u32 old = sm_ClipPlaneEnable;
	if (s_ClipPlanesGlobalVar)
	{
		sm_ClipPlaneEnable = enable;
	}
	else
	{
		Assert(old == 0);
	}
#if SPU_GCM_FIFO
	SPU_SIMPLE_COMMAND(grcDevice__SetEdgeClipPlaneEnable, enable);
#endif // SPU_GCM_FIFO
	return old;

#else

	Assert(enable <= 0xff);
	u32 old = sm_ClipPlaneEnable;
	sm_ClipPlaneEnable = enable;
#if SPU_GCM_FIFO
	SPU_SIMPLE_COMMAND(grcDevice__SetEdgeClipPlaneEnable, enable);
#endif // SPU_GCM_FIFO
	return old;

#endif
}

void grcDevice::SetClipPlane(int index,Vec4V_In plane) {
#if !HACK_GTA4 // no shader clip planes

	Assert(index >= 0 && index < RAGE_MAX_CLIPPLANES);
	LookForClipPlanesGlobalVar();
	s_ClipPlanes[index] = plane;
	if (s_ClipPlanesGlobalVar)
	{
		Mat44V transpose, inverseTranspose;
		// I'd love to claim I'm really good at linear algebra but I just looked at the Xenon light shaft
		// example and tried different permutations of inverse and transpose until it works.
		Transpose(transpose,grcViewport::GetCurrent()->GetCompositeMtx());
		InvertFull(inverseTranspose,transpose);
		s_TransformedClipPlanes = Multiply(inverseTranspose,plane);
		grcEffect::SetGlobalVar(s_ClipPlanesGlobalVar, s_TransformedClipPlanes, RAGE_MAX_CLIPPLANES);
#if SPU_GCM_FIFO
		SPU_COMMAND(grcDevice__SetEdgeClipPlane, index);
		cmd->plane = plane.GetIntrin128();
#endif // SPU_GCM_FIFO
	}

#else

#if SPU_GCM_FIFO
	if (index < EDGE_NUM_MODEL_CLIP_PLANES)
	{
		SPU_COMMAND(grcDevice__SetEdgeClipPlane, index);
		cmd->plane = plane.GetIntrin128();
	}
#endif // SPU_GCM_FIFO

#endif
}

#if !HACK_GTA4 // no shader clip planes
void grcDevice::GetClipPlane(int index,Vec4V_InOut plane) {
	Assert(index >= 0 && index < RAGE_MAX_CLIPPLANES);

	plane = s_ClipPlanes[index];
}
#endif // !HACK_GTA4

static struct translateTypeStruct
{ u8 type; u8 count; u8 size; }
s_TranslateType[] = {
	{ CELL_GCM_VERTEX_SF, 1, 2 },		// grcdsHalf,
	{ CELL_GCM_VERTEX_SF, 2, 4 },		// grcdsHalf2,
	{ CELL_GCM_VERTEX_SF, 3, 6 },		// grcdsHalf3,
	{ CELL_GCM_VERTEX_SF, 4, 8 },		// grcdsHalf4,

	{ CELL_GCM_VERTEX_F, 1, 4 },		// grcdsFloat,
	{ CELL_GCM_VERTEX_F, 2, 8 },		// grcdsFloat2,
	{ CELL_GCM_VERTEX_F, 3, 12 },		// grcdsFloat3,
	{ CELL_GCM_VERTEX_F, 4, 16 },		// grcdsFloat4,

	{ CELL_GCM_VERTEX_UB256, 4, 4 },	// grcdsUBYTE4,
	{ CELL_GCM_VERTEX_UB, 4, 4 },		// grcdsColor,
	{ CELL_GCM_VERTEX_CMP, 1, 4 },		// grcdsPackedNormal,

	{ 0, 0, 0 },						// grcdsEDGE0,
	{ 0, 0, 0 },						// grcdsEDGE1,
	{ 0, 0, 0 },						// grcdsEDGE2,
	// PS3 specific

	{ CELL_GCM_VERTEX_S32K, 2, 4},		// grcdsShort2
	{ CELL_GCM_VERTEX_S32K, 4, 8},		// grcdsShort4
};
CompileTimeAssert(NELEM(s_TranslateType) == grcFvf::grcdsCount);

static s8
s_TranslateElement[grcVertexElement::grcvetCount][8] = {
	{gcm::POSITION,gcm::TEXCOORD4,-1,-1,-1,-1,-1,-1},	// grcvetPosition,
	{gcm::POSITION,gcm::TEXCOORD4,-1,-1,-1,-1,-1,-1},	// grcvetPositionT,
	{gcm::NORMAL,gcm::TEXCOORD5,-1,-1,-1,-1,-1,-1},	// grcvetNormal,
	{gcm::BINORMAL0,gcm::BINORMAL1,-1,-1,-1,-1,-1,-1},				// grcvetBinormal,
	{gcm::TANGENT0,gcm::TANGENT1,-1,-1,-1,-1,-1,-1},// grcvetTangent,
	{gcm::TEXCOORD0,gcm::TEXCOORD1,gcm::TEXCOORD2,gcm::TEXCOORD3,gcm::TEXCOORD4,gcm::TEXCOORD5,-1,-1},	// grcvetTexture,
	{gcm::BLENDWEIGHT,-1,-1,-1,-1,-1,-1,-1},			// grcvetBlendWeights,
	{gcm::BLENDINDICES,-1,-1,-1,-1,-1,-1,-1},			// grcvetBindings,
	{gcm::COLOR0,gcm::COLOR1,-1,-1,-1,-1,-1,-1},		// grcvetColor,
};

static const CellGcmEnum streamFrequencyModeTranslate[] =
{
	CELL_GCM_FREQUENCY_DIVIDE,
	CELL_GCM_FREQUENCY_MODULO
};

void DumpVertexDeclaration(const grcVertexDeclaration &decl);

grcVertexDeclaration* grcDevice::CreateVertexDeclaration(const grcVertexElement *pVertexElements, int elementCount, int strideOverride)
{
	Assert(elementCount >= 0);

	sysMemStartTemp();
	grcVertexDeclaration* retVal = rage_aligned_new (16) grcVertexDeclaration;
	sysMemEndTemp();

	for (u32 i = 0; i < grcVertexDeclaration::c_MaxAttributes; i++)
	{
		retVal->Format[i].type = CELL_GCM_VERTEX_F;
		retVal->Format[i].count = 0;
		retVal->Format[i].stride = 0;
		retVal->Format[i].divider = grcFvf::s_DefaultDivider;
		retVal->Offset[i] = 0;
		retVal->Stream[i] = 0;
	}

	retVal->StreamFrequencyMode = 0;
	retVal->IsPadded = 0;

	u8 strides[grcVertexDeclaration::c_MaxStreams];
	memset(strides, 0, sizeof(strides));

	for (int i = 0; i < elementCount; i++)
	{
		u32 stream = pVertexElements[i].stream;
		Assert(stream < grcVertexDeclaration::c_MaxStreams);
		int dest = s_TranslateElement[pVertexElements[i].type][pVertexElements[i].channel];
		Assert(dest >= 0 && dest < static_cast<int>(grcVertexDeclaration::c_MaxAttributes));
		retVal->Offset[dest] = strides[stream];
		retVal->Stream[dest] = static_cast<u8>(stream);
		retVal->Format[dest].type = s_TranslateType[pVertexElements[i].format].type;
		if (s_TranslateType[pVertexElements[i].format].size != pVertexElements[i].size)
			retVal->IsPadded = 1;
		Assert(!retVal->Format[dest].count); // make sure we didn't accidentally re-use the same attribute.
		retVal->Format[dest].count = s_TranslateType[pVertexElements[i].format].count;
		retVal->Format[dest].stride = stream + 1; // remember original stream so we can compute final stride
		retVal->Format[dest].divider = pVertexElements[i].streamFrequencyDivider;
		Assert(!(retVal->StreamFrequencyMode & (streamFrequencyModeTranslate[pVertexElements[i].streamFrequencyMode] << dest)));
		retVal->StreamFrequencyMode |= streamFrequencyModeTranslate[pVertexElements[i].streamFrequencyMode] << dest;
		strides[stream] += pVertexElements[i].size;
	}

	// Now that we know final strides, plug it into all enabled channels.
	for (u32 i = 0; i < grcVertexDeclaration::c_MaxAttributes; i++)
	{
		if (retVal->Format[i].stride)
		{
			retVal->Format[i].stride = (retVal->Stream[i] || !strideOverride)? strides[retVal->Format[i].stride-1] : (u8) strideOverride;
		}
	}

	retVal->RefCount = 1;
	retVal->Stream0Size = strideOverride? strideOverride : strides[0];

	// TODO: sample_creature is creating vertex declarations like mad every frame!
	// DumpVertexDeclaration(*retVal);
	return retVal;
}


void grcDevice::DestroyVertexDeclaration(grcVertexDeclaration *decl) {
	decl->Release();
}

#if !__NO_OUTPUT
void DumpVertexDeclaration(const grcVertexDeclaration &decl)
{
#if !__NO_OUTPUT
	static const char *types[] = { "invalid", "GL_SHORT", "GL_FLOAT", "GL_HALF_FLOAT_NV", "GL_UNSIGNED_BYTE", "GL_SHORT", "GL_10_11_11", "GL_UNSIGNED_BYTE" };
	static const char *semantics[] = {
		"POSITION", "BLENDWEIGHT", "NORMAL", "COLOR0",
		"COLOR1", "TANGENT1", "BINORMAL1", "BLENDINDICES",
		"TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3",
		"TEXCOORD4", "TEXCOORD5", "TANGENT0", "BINORMAL0" };

	for (u32 i = 0; i < grcVertexDeclaration::c_MaxAttributes; i++)
	{
		if (decl.Format[i].count)
		{
			grcDisplayf("  ATTR%02d: %s: %sx%d  %u wide",i,semantics[i],types[decl.Format[i].type],	decl.Format[i].count, decl.Format[i].stride);
		}
	}
#endif
}
#endif

extern u16 g_VertexShaderInputs;

// XPARAM(immspu);

static void BindVertexFormat(const grcVertexDeclaration* vertexDeclaration)
{
	grcCurrentVertexDeclaration = vertexDeclaration;
	if (!vertexDeclaration)
		return;

	const bool recording = (GCM_CONTEXT->callback != spuCallback);

	if (/*PARAM_immspu.Get() &&*/ !grcCurrentVertexOffsets[0]) {
		Assert(!recording);
		SPU_COMMAND(grcDevice__BindVertexFormat,0);
		cmd->vertexdecl = (void*) vertexDeclaration;
		return;
	}

	u32 offsets[grcVertexDeclaration::c_MaxStreams];

	// If there are any vertex buffers, use the input mask.
	// Otherwise it's immediate mode and not safe to disable unused inputs.
	u16 inputMask = ~0U;

	// Build a table of memory offsets for each vertex buffer
	for (u32 i = 0; i < grcVertexDeclaration::c_MaxStreams; ++i)
	{

		u32 vertexBufferPtr = grcCurrentVertexOffsets[i];
		if (vertexBufferPtr)
		{
			// If we have at least one vertex buffer, respect the current shader inputs.
			inputMask = g_VertexShaderInputs;

			offsets[i] = vertexBufferPtr;
		}
		else
		{
			offsets[i] = 0;
			// locations[i] = CELL_GCM_LOCATION_LOCAL;
		}
	}

	// Set bit mask for stream frequency modes for each vertex attribute
	// GCM_STATE(SetFrequencyDividerOperation,vertexDeclaration->StreamFrequencyMode);

#if 1	// to hell with TRC, we need the speed
	// Don't bother sending down offsets if they're all zero (common when using formats intended for inline rendering)
	uint32_t *writePtr = gcm::AllocateFifo_Secret(offsets[0]? 36 : 19);
	*(writePtr++) = 0x00040000 | CELL_GCM_NV4097_SET_FREQUENCY_DIVIDER_OPERATION;
	*(writePtr++) = vertexDeclaration->StreamFrequencyMode;

	*(writePtr++) = 0x00400000 | CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_FORMAT;	// Write 64 bytes to 1740
	for (u64 i = 0; i<16; ++i, inputMask >>= 1)
		*(writePtr++) = (inputMask&1)? vertexDeclaration->FormatU[i] : CELL_GCM_VERTEX_F;

	if (offsets[0]) {
		*(writePtr++) = 0x00400000 | CELL_GCM_NV4097_SET_VERTEX_DATA_ARRAY_OFFSET;	// Write 64 bytes to 1680
		for (u64 i = 0; i<16; ++i) {
			const u32 stream = vertexDeclaration->Stream[i];
			*(writePtr++) = offsets[stream] + grcCurrentVertexDeclaration->Offset[i];
		}
	}
#else
	for (u32 i = 0; i < grcVertexDeclaration::c_MaxAttributes; ++i, inputMask >>= 1)
	{
		const u32 stream = vertexDeclaration->Stream[i];
		// If we're lucky, the compiler will figure out we'd already lined everything up appropriately.
		GCM_DEBUG(GCM::cellGcmSetVertexDataArray(GCM_CONTEXT,i,
			vertexDeclaration->Format[i].divider,
			vertexDeclaration->Format[i].stride,
			(inputMask & 1)? vertexDeclaration->Format[i].count : 0,
			(inputMask & 1)? vertexDeclaration->Format[i].type : CELL_GCM_VERTEX_F,
			locations[stream],
			vertexDeclaration->Offset[i] + offsets[stream]));
	}
#endif

	if (Likely(!recording))
	{
		InvalidateSpuGcmState(CachedStates.VertexFormats, ~0);
	}
}

grcDevice::Result grcDevice::SetVertexDeclaration(const grcVertexDeclaration* pDecl)
{
	BindVertexFormat(pDecl);

	return 0;
}

grcDevice::Result grcDevice::RecordSetVertexDeclaration(const grcVertexDeclaration *pDecl)
{
	grcCurrentVertexDeclaration = pDecl;
	return 0;
}

void* grcDevice::BeginVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize)
{
	grcStateBlock::FlushThrough(false);

	BANK_ONLY(AssertMsg(g_AreRenderTargetsBound, "Attempting to draw with no render targets bound");)
	AssertMsg(!grcCurrentVertexOffsets[0],"Vertex Stream 0 still active in BeginVertices");
	AssertMsg(!grcCurrentVertexOffsets[1],"Vertex Stream 1 still active in BeginVertices");
	AssertMsg(grcCurrentVertexDeclaration,"BeginVertices - no declarator active! (did you use immediate mode directly?)");
	AssertMsg(!grcCurrentVertexDeclaration->IsPadded,"BeginVertices - Current vertex declaration contains padding, will crash RSX!");
	Assertf(grcCurrentVertexDeclaration->Stream0Size == vertexSize,"BeginVertices - current declarator is %d bytes, vertex size is %d",grcCurrentVertexDeclaration->Stream0Size,vertexSize);
	if (!Verifyf(vertexCount * vertexSize <= BEGIN_VERTICES_MAX_SIZE,"BeginVertices - buffer size overflow , required size %d bytes, maximum size is %d - Call GetVerticesMaximumBatchSize to get the maximum number of primitives to use",vertexCount * vertexSize,BEGIN_VERTICES_MAX_SIZE))
		return NULL;

	static u8 translate[] = {
		CELL_GCM_PRIMITIVE_POINTS,
		CELL_GCM_PRIMITIVE_LINES,
		CELL_GCM_PRIMITIVE_LINE_STRIP,
		CELL_GCM_PRIMITIVE_TRIANGLES,
		CELL_GCM_PRIMITIVE_TRIANGLE_STRIP,
		CELL_GCM_PRIMITIVE_TRIANGLE_FAN,
		CELL_GCM_PRIMITIVE_QUADS
	};

	void *result = NULL;
	// BEGIN GCM HUD BUG WORKAROUND
	//              SetDrawBegin (has HW bug workaround in it)
	//				|    SetDrawInlineArrayPointer
	//              |          |                          SetDrawEnd
	//				|		   |                              |   NOP alignment
	//				|		   |							  |   |
	//				V		   V							  V   V
	u32 wordCount = 6 + (1 + vertexCount * (vertexSize>>2)) + 2 + 3;
	if (GCM_CONTEXT->current + wordCount > GCM_CONTEXT->end)
		GCM_CONTEXT->callback(GCM_CONTEXT, wordCount);
	// END GCM HUD BUG WORKAROUND

	++g_GcmDisableFlush;

	GCM_DEBUG(GCM::cellGcmSetDrawBegin(GCM_CONTEXT,translate[dm])); // can this be GCMU?

	static const u8 nopCount[16] = { 3,0,0,0, 2,0,0,0,  1,0,0,0, 0,0,0,0 };
	GCM_DEBUG(GCMU::cellGcmSetNopCommand(GCM_CONTEXT,nopCount[(uint32_t)GCM_CONTEXT->current & 15]));

	u32 drawWordCount = vertexCount * (vertexSize >> 2);
	GCM_DEBUG(GCMU::cellGcmSetDrawInlineArrayPointer(GCM_CONTEXT,drawWordCount,&result));

	// Set the END up now so we can catch overruns later.
	u32 *endDraw = (u32*)result + drawWordCount;
	endDraw[0] = CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BEGIN_END, 1);
	endDraw[1] = 0;

	Assert(((u32)result & 15) == 0);

	StoreBufferEnd(result, vertexCount, vertexSize);

	return result;
}

void grcDevice::EndVertices(const void *bufferEnd)
{
	VerifyBufferEnd(bufferEnd);

#if !__FINAL
	if (GCM_CONTEXT->current[0] != CELL_GCM_METHOD(CELL_GCM_NV4097_SET_BEGIN_END, 1) || GCM_CONTEXT->current[1])
		Quitf("grcDevice::EndVertices -- mismatched vertex count, or you overran your array.");
	else
#endif
		GCM_CONTEXT->current += 2;
	// GCM_DEBUG(cellGcmSetDrawEnd(GCM_CONTEXT));

	--g_GcmDisableFlush;

	InvalidateSpuGcmState(CachedStates.VertexFormats, ~0);
}


void grcDevice::SetIndices(const grcIndexBuffer& pBuffer)
{
	grcCurrentIndexBuffer = reinterpret_cast<const grcIndexBufferGCM*>(&pBuffer);
}


void grcDevice::SetStreamSource(u32 stream,const grcVertexBuffer& streamData,u32 ,u32 )
{
	Assert(stream < grcVertexDeclaration::c_MaxStreams);
	grcCurrentVertexOffsets[stream] = (u32) const_cast<grcVertexBuffer&>(streamData).GetVertexData();
	BindVertexFormat(grcCurrentVertexDeclaration);
}

void grcDevice::ClearStreamSource(u32 stream)
{
	Assert(stream < grcVertexDeclaration::c_MaxStreams);
	grcCurrentVertexOffsets[stream] = 0;
}

static int modeMap[] =
{
	CELL_GCM_PRIMITIVE_POINTS,
	CELL_GCM_PRIMITIVE_LINES,
	CELL_GCM_PRIMITIVE_LINE_STRIP,
	CELL_GCM_PRIMITIVE_TRIANGLES,
	CELL_GCM_PRIMITIVE_TRIANGLE_STRIP,
	CELL_GCM_PRIMITIVE_TRIANGLE_FAN,
	CELL_GCM_PRIMITIVE_QUADS,
};

#if __ASSERT && !__OPTIMIZED
static void CheckPreDraw()
{
	Assert(grcCurrentVertexOffsets[0] );
	for (u32 i = 0; i < grcVertexDeclaration::c_MaxAttributes; ++i)
	{
		const u32 stream = grcCurrentVertexDeclaration->Stream[i];
		Assertf(grcCurrentVertexOffsets[stream], "Vertex declaration requires stream %d, but it is not bound", stream);
	}
}
#else
#define CheckPreDraw()
#endif // __ASSERT && !__OPTIMIZED

void grcDevice::DrawIndexedPrimitive(grcDrawMode dm, int startIndex, int indexCount)
{
	grcStateBlock::Flush();
	CheckPreDraw();
	Assert(grcCurrentIndexBuffer);
	u32 offset = grcCurrentIndexBuffer->GetGCMOffset() + startIndex * sizeof(u16);
#if SPU_GCM_FIFO > 2
	SPU_COMMAND(grcDevice__DrawIndexedPrimitive,modeMap[dm]);
	cmd->decl = (spuVertexDeclaration*) grcCurrentVertexDeclaration;
	for (int i = 0; i < grcVertexDeclaration::c_MaxStreams; ++i)
	{
		cmd->vertexData[i] = grcCurrentVertexOffsets[i];
	}
	cmd->indexData = offset;
	cmd->indexCount = indexCount;
#else
	GCM_DEBUG(GCM::cellGcmSetDrawIndexArray(GCM_CONTEXT,modeMap[dm],indexCount,CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16,offset>>31,offset));
#endif
}


void grcDevice::DrawIndexedPrimitive(grcDrawMode dm,const grcVertexDeclaration *decl,const grcVertexBuffer &vb,const grcIndexBuffer &ib, int customIndexCount)
{
	grcStateBlock::Flush();
	if((customIndexCount==0) || (customIndexCount > ib.GetIndexCount()))
		customIndexCount = ib.GetIndexCount();

#if SPU_GCM_FIFO > 2
	SPU_COMMAND(grcDevice__DrawIndexedPrimitive,modeMap[dm]);
	cmd->decl = (spuVertexDeclaration*) decl;
	cmd->vertexData[0] = (u32) const_cast<grcVertexBuffer&>(vb).GetVertexData();
	for (int i = 1; i < grcVertexDeclaration::c_MaxStreams; ++i)
	{
		cmd->vertexData[i] = grcCurrentVertexOffsets[i];
	}
	cmd->indexData = ((grcIndexBufferGCM&)ib).GetGCMOffset();
	cmd->indexCount = customIndexCount;
#else
	grcCurrentVertexDeclaration = decl;
	GRCDEVICE.SetIndices(ib);
	GRCDEVICE.SetStreamSource(0,vb,0,vb.GetVertexStride());

	DrawIndexedPrimitive(dm, 0, customIndexCount);

	GRCDEVICE.ClearStreamSource(0);
#endif
}

void grcDevice::DrawPrimitive(grcDrawMode dm, const grcVertexDeclaration *decl,const grcVertexBuffer &vb, int UNUSED_PARAM(startVertex), int vertexCount)
{
	if (!vertexCount)
		return;

	grcStateBlock::Flush();
	TrapGT(vertexCount,65536);
#if SPU_GCM_FIFO > 2
	SPU_COMMAND(grcDevice__DrawPrimitive,modeMap[dm]);
	cmd->decl = (spuVertexDeclaration*) decl;
	cmd->vertexData[0] = (u32) const_cast<grcVertexBuffer&>(vb).GetVertexData();
	for (int i = 1; i < grcVertexDeclaration::c_MaxStreams; ++i)
	{
		cmd->vertexData[i] = grcCurrentVertexOffsets[i];
	}
	cmd->startVertex = 0;
#if HACK_GTA4
	cmd->vertexCount = vertexCount>65535?65535:vertexCount; // vertexCount is only u16
#else
	cmd->vertexCount = vertexCount;
#endif
#else
	grcCurrentVertexDeclaration = decl;
	GRCDEVICE.SetStreamSource(0,vb,0,vb.GetVertexStride());

	DrawPrimitive(dm, startVertex, vertexCount);

	GRCDEVICE.ClearStreamSource(0);
#endif
}

void grcDevice::DrawPrimitive(grcDrawMode dm, int startVertex, int vertexCount)
{
	if (!vertexCount)
		return;
	grcStateBlock::Flush();
	TrapGT(vertexCount,65536);
#if SPU_GCM_FIFO > 2
	SPU_COMMAND(grcDevice__DrawPrimitive,modeMap[dm]);
	cmd->decl = (spuVertexDeclaration*) grcCurrentVertexDeclaration;
	for (int i = 0; i < grcVertexDeclaration::c_MaxStreams; ++i)
	{
		cmd->vertexData[i] = grcCurrentVertexOffsets[i];
	}
	cmd->startVertex = startVertex;
#if HACK_GTA4
	cmd->vertexCount = vertexCount>65535?65535:vertexCount;	// vertexCount is only u16
#else
	cmd->vertexCount = vertexCount;
#endif
#else
	CheckPreDraw();

	GCM_DEBUG(GCM::cellGcmSetDrawArrays(GCM_CONTEXT,modeMap[dm], startVertex, vertexCount));
#endif
}

void grcDevice::SetVertexShaderConstant(int startRegister,const float *data,int regCount)
{
	GCM_DEBUG(GCM::cellGcmSetVertexProgramConstants(GCM_CONTEXT,startRegister,regCount<<2,data));
}

static u32 s_NextFence;
static const u32 s_MaxFence = 0xF0000000;

grcFenceHandle grcDevice::InsertFence()
{
	if (++s_NextFence == s_MaxFence)
		s_NextFence = 1;
	// tell the gpu to write that value when it gets to it
	GCM_DEBUG(GCM::cellGcmSetWriteBackEndLabel(GCM_CONTEXT, LABEL_NEXT_FENCE, s_NextFence));

	// and kick off the gpu so that we don't block indefinitely if we poll too soon.
	// note that we call Flush, not Finish, because we don't need the gpu to finish before
	// continuing -- rather, we just need to make sure the gpu might get there.
	rageFlush(GCM_CONTEXT);
	return (grcFenceHandle)s_NextFence;
}

bool grcDevice::IsFencePending(grcFenceHandle _fence)
{
	// Handle the wraparound case here.
	uint32_t value = *cellGcmGetLabelAddress(LABEL_NEXT_FENCE);
	uint32_t fence = (uint32_t) _fence;
	// if value is in the first quadrant and fence is in the last quadrant, fence is not pending (ie is done).
	// otherwise the fence is pending if the value is smaller than the fence.
	if ((value<s_MaxFence/4)&&(fence>s_MaxFence-(s_MaxFence/4)))
		return false;
	else
		return value < fence;
}

void grcDevice::GPUBlockOnFence(grcFenceHandle _fence)
{
	GCM_DEBUG(GCM::cellGcmSetWaitLabel(GCM_CONTEXT, LABEL_NEXT_FENCE, (uint32_t)_fence));
}

extern __THREAD u32 g_TaskTimeout;

void grcDevice::BlockOnGcmTasks()
{
	/// TrapNZ(FAULT_STACK_SP); // enabling this will halt the debugger immediately
	// AssertMsg(FAULT_STACK_SP == 0,"Update thread called BlockOnGcmTasks before Render thread called EndFrame!  Fix your render thread sync logic!");
	TaskMutexLock lock;

	// As a special case, don't warn about long tasks here because that's expected.
	u32 before = g_TaskTimeout;
	g_TaskTimeout = 0;
	for(int i=0; i<gcmSegmentCapacity; i++)
	{
		// spuCallback creates a job at slot s_gcmTaskIndex, then increments that (with wrap) and waits on the new slot before returning.
		// so let's duplicate that behavior here by waiting on all the tasks in order starting at s_gcmTaskIndex+1.  This shouldn't matter,
		// but something is going wrong with the drawablespu tasks and this is the only other spot the data gets modified.
		int j = (i + s_gcmTaskIndex + 1) % gcmSegmentCapacity;
		if (s_gcmTasks[j])
		{
			sysTaskManager::Wait(s_gcmTasks[j]);
			s_gcmTasks[j] = NULL;
		}
	}
	g_TaskTimeout = before;

	/// TrapNZ(FAULT_STACK_SP); // enabling this will halt the debugger immediately
	//AssertMsg(FAULT_STACK_SP == 0,"Render thread called BeginFrame before BlockOnGcmTasks was done!  Fix your render thread sync logic!");
}

void grcDevice::BlockOnFence(grcFenceHandle fence)
{
	if (fence) {
		while (IsFencePending(fence))
			sys_timer_usleep(30);
	}
}

void grcDevice::KickOffGpu()
{
	rageFlush(GCM_CONTEXT);
}

grcOcclusionQuery grcDevice::CreateOcclusionQuery(int XENON_ONLY(tileCount) /*= -1*/)
{
	int freeIndex = s_OcclusionQueryFreeList.Allocate();
	return static_cast<grcOcclusionQuery>(freeIndex+1);
}

void grcDevice::DestroyOcclusionQuery(grcOcclusionQuery& query)
{
	if (!query)
		return;
	s_OcclusionQueryFreeList.Free(query-1);
	query = 0xffffffff;
}

void grcDevice::BeginOcclusionQuery(grcOcclusionQuery query)
{
	if (!query)
		return;
	--query;
	Assertf(query < GCM_OCCLUSION_QUERY_COUNT, "Invalid query %x",query);
	AssertMsg(s_OcclusionQueryInProgress == 0xffffffff, "Another query already active");

	GCM_DEBUG(GCM::cellGcmSetZpassPixelCountEnable(GCM_CONTEXT, CELL_GCM_TRUE));
	GCM_DEBUG(GCM::cellGcmSetClearReport(GCM_CONTEXT, CELL_GCM_ZPASS_PIXEL_CNT));

	// When a report gets written the zero field will be set to 0. This is a
	// convenient way to test completion of the query
	s_OcclusionQueryData[query].zero = 1;
	s_OcclusionQueryData[query].value = 0;

	ASSERT_ONLY(s_OcclusionQueryInProgress = query);
}

void grcDevice::EndOcclusionQuery(grcOcclusionQuery query)
{
	if (!query)
		return;
	--query;
	Assertf(query < GCM_OCCLUSION_QUERY_COUNT, "Invalid query %x",query);
	AssertMsg(s_OcclusionQueryInProgress == query, "Another query is already active");

	GCM_DEBUG(GCM::cellGcmSetReport(GCM_CONTEXT, CELL_GCM_ZPASS_PIXEL_CNT, s_OcclusionQueryReportIndex + query));
	GCM_DEBUG(GCM::cellGcmSetZpassPixelCountEnable(GCM_CONTEXT, CELL_GCM_FALSE));

	ASSERT_ONLY(s_OcclusionQueryInProgress = 0xffffffff);
}

bool grcDevice::GetOcclusionQueryData(grcOcclusionQuery query, u32& numDrawn)
{
	if (!query)
		return false;
	--query;

	Assertf(query < GCM_OCCLUSION_QUERY_COUNT, "Invalid query %x",query);
	AssertMsg(s_OcclusionQueryInProgress != query, "You cannot get a query's data whilst it is active");

	numDrawn = s_OcclusionQueryData[query].value;
	// Has the query completed?
	return s_OcclusionQueryData[query].zero == 0;
}

grcConditionalQuery grcDevice::CreateConditionalQuery()
{
	int freeIndex = s_ConditionalQueryFreeList.Allocate();
	return static_cast<grcConditionalQuery>(freeIndex+1);
}

void grcDevice::DestroyConditionalQuery(grcConditionalQuery& query)
{
	if (!query)
		return;
	s_ConditionalQueryFreeList.Free(query-1);
	query = 0xffffffff;
}

void grcDevice::BeginConditionalQuery(grcConditionalQuery query)
{
	if (!query)
		return;
	--query;
	AssertMsg(s_OcclusionQueryInProgress == 0xffffffff, "Another query already active");
	Assertf(query < kMaxConditionalQueries, "Invalid query %x",query);

	GCM_DEBUG(GCM::cellGcmSetZpassPixelCountEnable(GCM_CONTEXT, CELL_GCM_TRUE));
	GCM_DEBUG(GCM::cellGcmSetClearReport(GCM_CONTEXT, CELL_GCM_ZPASS_PIXEL_CNT));
	GCM_DEBUG(GCM::cellGcmSetRenderEnable(GCM_CONTEXT, CELL_GCM_TRUE, 0));

	ASSERT_ONLY(s_OcclusionQueryInProgress = query);
}

void grcDevice::EndConditionalQuery(grcConditionalQuery query)
{
	if (!query)
		return;
	--query;
	AssertMsg(s_OcclusionQueryInProgress == query, "Another query already active");
	Assertf(query < kMaxConditionalQueries, "Invalid query 0x%x",query);

	GCM_DEBUG(GCM::cellGcmSetReport(GCM_CONTEXT, CELL_GCM_ZPASS_PIXEL_CNT, s_ConditionalQueryReportIndex + query));
	GCM_DEBUG(GCM::cellGcmSetZpassPixelCountEnable(GCM_CONTEXT, CELL_GCM_FALSE));

	ASSERT_ONLY(s_OcclusionQueryInProgress = 0xffffffff);
}

void grcDevice::BeginConditionalRender(grcConditionalQuery query)
{
	if (!query)
		return;
	--query;
	Assertf(query < kMaxConditionalQueries, "Invalid query %x",query);

	// double WAR	
	// Avoid cross segment calls
	const int wordCount = 6;
	GCM_DEBUG(GCM::cellGcmReserveMethodSize(GCM_CONTEXT, wordCount));

	// ensure that cellGcmSetRenderEnable is never directly after a writelabel.
	// Turns out SetReport isn't sufficient to avoid the hang.  What we really want to do here is only issue this
	// SetWriteCommandLabel if we already issued a SetWriteTextureLabel in this segment already.  So a second conditional
	// render in the same segment shouldn't need this at all.
	// https://ps3.scedev.net/support/issue/99174/_SetRenderEnable_WAR_workaround_insufficient
	GCM_DEBUG(GCM::cellGcmSetWriteCommandLabel(GCM_CONTEXT, LABEL_COMMAND_DUMMY, 0));
	GCM_DEBUG(GCM::cellGcmSetRenderEnable(GCM_CONTEXT, CELL_GCM_CONDITIONAL, s_ConditionalQueryReportIndex + query));
}

void grcDevice::EndConditionalRender(grcConditionalQuery query)
{
	if (!query)
		return;
	--query;
	Assertf(query < kMaxConditionalQueries, "Invalid query %x",query);
	GCM_DEBUG(GCM::cellGcmSetRenderEnable(GCM_CONTEXT, CELL_GCM_TRUE, 0));
}

u32* grcDevice::CreateDisplayList(u32 wordCount, u32 alignment /*= 128*/)
{
	return g_GcmInitBufferSize ? rage_aligned_new (alignment) u32[wordCount] : reinterpret_cast<u32*>(physical_new(wordCount << 2, alignment));
}

void grcDevice::DeleteDisplayList(u32* ptr)
{
	g_GcmInitBufferSize ? delete [] ptr : physical_delete(ptr);
}

void grcDevice::DeleteDisplayList(u32 offset)
{
	g_GcmInitBufferSize ? delete [] reinterpret_cast<u32*>(gcm::MainPtr(offset)) : physical_delete(gcm::LocalPtr(offset));
}

bool grcDevice::GetMultiSampleFlag()
{
	return g_GcmSurface.antialias != CELL_GCM_SURFACE_CENTER_1;
}


bool grcDevice::HasADepthTarget()
{
	return	!((g_GcmSurface.depthFormat == CELL_GCM_SURFACE_Z24S8 ) &&
			(g_GcmSurface.depthLocation == CELL_GCM_LOCATION_LOCAL ) &&
			(g_GcmSurface.depthOffset == 0 ) &&
			(g_GcmSurface.depthPitch == 64 ));

}
void grcDevice::BeginDXView()
{
	AssertMsg(0,"can't be here\n");
}

void grcDevice::EndDXView()
{
	AssertMsg(0,"can't be here\n");
}

#if DRAWABLESPU_STATS
void grcDevice::SetFetchStatsBuffer(spuDrawableStats* dstPtr) 
{
	SPU_COMMAND(grcEffect__FetchStats,0);
	cmd->dstPtr = dstPtr;
	rageFlush(GCM_CONTEXT);
}
#endif

}	// namespace rage

#endif		// __GCM
