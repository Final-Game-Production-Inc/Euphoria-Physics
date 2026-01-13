//
// grcore/device_gnm.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//
#include "device.h"

#if __GNM
#include "atl/pool.h"
#include "profile/timebars.h"

#include "AA_shared.h"
#include "allocscope.h"
#include "fastquad.h"
#include "fencepool.h"
#include "image.h"
#include "quads.h"
#include "stateblock.h"
#include "buffer_gnm.h"
#include "gfxcontext_gnm.h"
#include "texture_gnm.h"
#include "texturefactory_gnm.h"
#include "rendertarget_gnm.h"
#include "viewport.h"
#include "vertexdecl.h"
#include "atl/freelist.h"
#include "system/bootmgr.h"
#include "system/customhangdetection.h"
#include "system/externalallocator.h"
#include "system/membarrier.h"
#include "system/timer.h"
#include "system/tmcommands.h"
#include "system/wndproc.h"
#include "gnmx/shader_parser.h"

#include "wrapper_gnm.h"

#include "vertexbuffer.h"
#include "indexbuffer.h"

#include <gnm.h>
#include "gnmx.h"
#include <pm4_dump.h>
#include <kernel.h>
#include <video_out.h>
#include <user_service.h>
#include <shader/binary.h>

#define ENABLE_FAST_CLEAR			(1)
#define ENABLE_FAST_CLEAR_COLOR		(1 && ENABLE_FAST_CLEAR)
#define ENABLE_FAST_CLEAR_DEPTH		(1 && ENABLE_FAST_CLEAR)
#define ENABLE_MSAA_BACK_BUFFER		(AA_BACK_BUFFER && DEVICE_MSAA)
#define ENABLE_EXTRA_CLEAN_DUMP		(1 && SUPPORT_RENDERTARGET_DUMP)
#define ENABLE_DEBUG_RUNTIME		(1 && __DEV)

// The Sony command buffer validation has an bug where it is triggering false positive validation errors,
// https://ps4.scedev.net/support/issue/26083/_V_end_address_is_not_within_GPU_accessible_mapped_memory.
// ENABLE_VERTEX_BUFFER_PAD will pad vertex buffers to work around this.
// Supposedly the validation error will be fixed in SDK2.0.
#define ENABLE_VERTEX_BUFFER_PAD	(ENABLE_DEBUG_RUNTIME)

#include "system/simpleallocator.h"

#if ENABLE_LCUE
#include "lcue.cpp"
#endif

#define MEMORYCS_COMPILING_SHADER   0
#include "shaderlib/memorycs.fxh"

using namespace sce;

#pragma comment(lib,"SceGnmDriver_stub_weak")
#pragma comment(lib,"ScePm4Dump")
#pragma comment(lib,"SceShaderBinary")
#pragma comment(lib,"SceGpuAddress")
#if ENABLE_DEBUG_RUNTIME
#pragma comment(lib,"SceGnm_debug")
#else
#pragma comment(lib,"SceGnm")
#endif
#pragma comment(lib,"SceVideoOut_stub_weak")
#if ENABLE_GPU_DEBUGGER
#pragma comment(lib,"SceGpuDebugger_stub_weak")
#endif

namespace rage {

PARAM(frameLimit, "[grcore] number of vertical synchronizations to limit game to");
NOSTRIP_PARAM(noFastClearColor, "[grcore] disable CMASK meta-buffer setup (RT clears are slower)");
NOSTRIP_PARAM(noFastClearDepth,	"[grcore] disable HTILE meta-buffer setup (helps debugging with Razor GPU)");
PARAM(noStencil, "[grcore] disable stencil buffers (so depth captures look right in Razor GPU)");
NOSTRIP_PARAM(multiSample,"[grcore] Number of AA samples (1, 2, 4, 8, or 16)");
NOSTRIP_PARAM(multiFragment,"[grcore] Number of AA fragments (1, 2, 4, or 8)");
NOSTRIP_PARAM(width, "[grcore] Screen width");
NOSTRIP_PARAM(height, "[grcore] Screen height");
PARAM(gpuDebugger, "[grcore] enable GPU debugger");

extern void grcInit();

class grcOcclusionQueryGnm   : public sce::Gnm::OcclusionQueryResults {};
class grcConditionalQueryGnm : public sce::Gnm::OcclusionQueryResults {};
static atPool<sce::Gnm::OcclusionQueryResults> *g_GPUQueries;
static sysCriticalSectionToken s_GPUQueryCs;
static int s_ResolutionX = 1920, s_ResolutionY = 1080;

volatile u32 grcDevice::sm_KillSwitch = 0;
grcDisplayWindow grcDevice::sm_CurrentWindows[NUMBER_OF_RENDER_THREADS];
grcDisplayWindow grcDevice::sm_GlobalWindow = grcDisplayWindow(s_ResolutionX, s_ResolutionY, 0, 0, 1);
MonitorConfiguration grcDevice::sm_MonitorConfig;

#if GPU_DEBUGGER_MANUAL_LOAD
SceKernelModule s_DebuggerHandle;
#endif
#if TRACK_DEPTH_BOUNDS_STATE
static DECLARE_MTR_THREAD bool s_DepthBoundsActive = false;
#endif

#if ENABLE_MSAA_BACK_BUFFER
static sce::Gnm::RenderTarget		s_MSAA_Color;
static sce::Gnm::DepthRenderTarget	s_MSAA_Depth;
#endif	//ENABLE_MSAA_BACK_BUFFER
#if DEVICE_MSAA
const bool s_ResolveBackBufferHW = false, s_ResolveWait = true;
#endif	//DEVICE_MSAA

// DbRenderControl provides a way to clear depth/stencil, even though
// it's not clear how it interops with DepthStencilControl
bool s_UseClearDbControl = false;

const char *grcDevice::sm_DefaultEffectName = "x:\\rage\\assets\\tune\\shaders\\lib\\rage_im";
grcEffect *g_DefaultEffect;
grcEffectVar g_DefaultSampler;
grcEffectVar g_DefaultColor;
grcCommandBuffer* g_grcCommandBuffer;
static grcEffectTechnique s_DefaultLit;
static grcEffectTechnique s_DefaultUnlit;
static grcEffectTechnique s_DefaultLitSkinned;
static grcEffectTechnique s_DefaultUnlitSkinned;
static grcEffectTechnique s_DefaultClear;
static grcEffectTechnique s_DefaultClearR32;
static grcEffectTechnique s_DefaultClearArray;
static grcEffectTechnique s_DefaultClearMRT2;
static grcEffectTechnique s_DefaultClearMRT3;
static grcEffectTechnique s_DefaultClearMRT4;

static grcEffect *s_MemoryCS;

static DECLARE_MTR_THREAD const grcVertexDeclaration *s_CurrentDecl;
extern DECLARE_MTR_THREAD grcVertexProgram::DeclSetup *g_VertexDeclSetup;

// TODO: find a way to decrease the number of AA state changes
static DECLARE_MTR_THREAD u32	s_ActiveNumSamples		= 0;
static DECLARE_MTR_THREAD u32	s_ActiveNumFragments	= 0;
static DECLARE_MTR_THREAD u32	s_ActiveNumIterations	= 0;
static DECLARE_MTR_THREAD bool	s_ActiveSuperSample		= false;

DECLARE_MTR_THREAD bool grcDevice::s_bScanMode_ViewportScissor = false;

static void *sm_esgsRing = NULL;
static void *sm_gsvsRing = NULL;
static void *sm_globalResourceTablePtr = NULL;
static void *sm_globalTesselationFactorsPtr = NULL;
static sce::Gnm::Buffer sm_tessFactorBuffer;

static FencePool s_FencePool(64);

#if ENABLE_FAST_CLEAR
const bool s_UseCmaskBackBuffer = false, s_UseHtileBackDepth = false;
bool s_AllowGPUClear = true, s_AllowFastClearStencil = false;
// shared variables
#if ENABLE_MSAA_BACK_BUFFER
extern bool s_UseStencilCompression;
#endif
#endif	//ENABLE_FAST_CLEAR

static void RelockRenderTargets()
{
	static_cast<grcTextureFactoryGNM&>( grcTextureFactory::GetInstance() ).RelockRenderTargets();
}

//--------< MSAAMode>----------//

static u32 getIntLog(u32 num)	{
	//relying on the compiler to optimize
	switch (num)
	{
	case 0:
	case 1:		return 0;
	case 2:		return 1;
	case 4:		return 2;
	case 8:		return 3;
	case 16:	return 4;
	default:
		Assertf(0,"Invalid pow2: %d",num);
		return 0;
	}
}

static inline sce::Gnm::NumSamples castNumSamples(u32 num)	{
	return static_cast<sce::Gnm::NumSamples>( getIntLog(num) );
}

grcDevice::MSAAMode::MSAAMode(const sce::Gnm::RenderTarget &target)
: m_bEnabled( target.getNumSamples() != sce::Gnm::kNumSamples1 )
, m_uSamples( 1 << target.getNumSamples() )
, m_uFragments(  1 << target.getNumFragments() )
{}

#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
grcDevice::MSAAMode::MSAAMode(const sce::Gnm::Texture &texture)
: m_bEnabled( texture.getNumFragments() != sce::Gnm::kNumFragments1 )
, m_uSamples( 1 << texture.getNumFragments() )
, m_uFragments( 1 << texture.getNumFragments() )
{}
#else
grcDevice::MSAAMode::MSAAMode(const sce::Gnm::Texture &texture)
: m_bEnabled( texture.getNumSamples() != sce::Gnm::kNumSamples1 )
, m_uSamples( 1 << texture.getNumSamples() )
, m_uFragments( 1 << texture.getNumSamples() )
{}
#endif //SCE_ORBIS_SDK_VERSION

sce::Gnm::NumSamples	grcDevice::MSAAMode::GetSamplesEnum() const
{
	return static_cast<sce::Gnm::NumSamples>(getIntLog( m_uSamples ));
}

sce::Gnm::NumFragments	grcDevice::MSAAMode::GetFragmentsEnum() const
{
	return static_cast<sce::Gnm::NumFragments>(getIntLog( m_uFragments ));
}

u32 grcDevice::MSAAMode::GetFmaskShift() const
{
	return 4;	// Orbis always decompresses Fmask into full 4 bits
}

grcDevice::MSAAMode grcDevice::sm_MSAA = grcDevice::MSAA_None;
#if DEVICE_EQAA
const grcDevice::MSAAMode grcDevice::MSAA_None(0);
bool grcDevice::sm_EQAA = false;
#else // DEVICE_EQAA
u32 grcDevice::sm_MultisampleQuality = 0;
#endif // DEVICE_EQAA

//--------</MSAAMode>----------//


#define DEBUG_DRAW_CALL 0
#if __DEV && DEBUG_DRAW_CALL
	static DECLARE_MTR_THREAD u32 s_drawCallIndex = 0, s_drawExcludeMin=1, s_drawExcludeMax=0;

	bool grcDevice::NotifyDrawCall(int)
	{
		if (++s_drawCallIndex == DEBUG_DRAW_CALL)
		{
			s_drawCallIndex = DEBUG_DRAW_CALL;
		}
		return s_drawCallIndex<s_drawExcludeMin || s_drawCallIndex>s_drawExcludeMax;
	}
	void grcDevice::ResetDrawCallCount()
	{
		s_drawCallIndex = 0;
	}
#elif EFFECT_TRACK_INSTANCING_ERRORS
	bool grcDevice::NotifyDrawCall(int numInstances)
	{
		if ( numInstances!=0 || !grcVertexProgram::GetCurrent()->IsPerInstance() )
			return true;
		Assertf( 0, "A non-instance call is detected with an instanced shader: %s",
			grcVertexProgram::GetCurrent()->GetEntryName() );
		return false;
	}
	void grcDevice::ResetDrawCallCount(){}
#else
	bool grcDevice::NotifyDrawCall(int)	{return true;}
	void grcDevice::ResetDrawCallCount(){}
#endif	//DEBUG_DRAW_CALL,EFFECT_TRACK_INSTANCING_ERRORS

int g_JpegSaveQuality = 75;

__THREAD grcContextHandle *g_grcCurrentContext;
static __THREAD grcContextHandle *s_savedGrcCurrentContext;
static volatile uint32_t *s_label;
static volatile uint64_t *s_nextfence;

Gnm::RenderTarget *s_renderTargets, *s_frontBuffer, *s_backBuffer;
Gnm::DepthRenderTarget s_depthTarget;

u32 grcDevice::sm_ClipPlaneEnable[NUMBER_OF_RENDER_THREADS] = {0};
#if DEVICE_CLIP_PLANES
u32 grcDevice::sm_PreviousClipPlaneEnable[NUMBER_OF_RENDER_THREADS] = {0};
u32 grcDevice::sm_ClipPlanesChanged[NUMBER_OF_RENDER_THREADS] = {0};

// World space clip planes.
Vec4V grcDevice::sm_ClipPlanes[RAGE_MAX_CLIPPLANES][NUMBER_OF_RENDER_THREADS];
// Constant buffer containing clipping space plane equations.
grcCBuffer *grcDevice::sm_pClipPlanesConstBuffer[NUMBER_OF_RENDER_THREADS] = {NULL};
#endif	//DEVICE_CLIP_PLANES

// For GsVsRingBuffer data
uint32_t s_biggestExportVertexSizeInDWord[4];
uint32_t s_biggestMaxOutputVertexCount = 0xffffffff;

// For EsGsRingBuffer data
uint32_t s_biggestMaxExportVertexSizeInDword = 0xffffffff;

#if __DEV
extern u64 s_TotalAllocatedMemory;
extern u64 s_PeakAllocatedMemory;
#endif // __DEV

#if DRAWABLE_STATS
__THREAD drawableStats* g_pCurrentStatsBucket = NULL;
#endif

void *allocateSystemSharedMemory(uint32_t size,uint32_t align)
{
	return (void*) rage_aligned_new(align) char[size];
}

static sysMemAllocator &videoShared()
{
	// This is basically the same as system shared memory, above.
	return *sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_GAME_VIRTUAL);
}

static void allocateDirectMemory(void* &pointer, uint32_t size, uint32_t align)
{
	off_t offset = 0;
	int ret = sceKernelAllocateDirectMemory(0, SCE_KERNEL_MAIN_DMEM_SIZE, size, align,
#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
		SCE_KERNEL_WC_GARLIC_NONVOLATILE,
#else
		SCE_KERNEL_WC_GARLIC,
#endif
		&offset);
	if (ret)
		Quitf("Unabled to allocate direct memory %u, error 0x%X", size, ret);
	
	ret = sceKernelMapDirectMemory(&pointer, size,
		SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_ALL,
		0, offset, align);
	if (ret)
		Quitf("Unabled to map direct memory, error 0x%X", ret);

#if __DEV
	s_TotalAllocatedMemory += size;
	if (s_PeakAllocatedMemory < s_TotalAllocatedMemory)
	{
		s_PeakAllocatedMemory = s_TotalAllocatedMemory;
		Displayf("Peak %" I64FMT "d", s_PeakAllocatedMemory);
	}
#endif // __DEV
}

static sysMemAllocator &videoPrivate()
{
	//const sce::Gnm::NumFragments logFragments = GRCDEVICE.GetMSAA().m_EnumFragments;
	//const int maxNonAASize = 100, maxAAsize = 300;
	//const int heapSize = (maxNonAASize + (maxAAsize << logFragments)) << 20;
	const int heapSize = 1044 * 1024 * 1024; // 1920 * 1080 * MSAA 4X
	static void *privateHeapStorage = NULL;
	if (!privateHeapStorage)	{
		allocateDirectMemory( privateHeapStorage, heapSize, 2<<20 );
	}
	static const size_t maxPointers = 16384;
	static char workspace[COMPUTE_WORKSPACE_SIZE(maxPointers)];
	static sysMemExternalAllocator privateHeap(heapSize, privateHeapStorage, maxPointers, workspace);
	return privateHeap;
}

sysMemAllocator& getVideoPrivateMemory()
{
	return videoPrivate();
}

void *allocateVideoSharedMemory(uint32_t size,uint32_t align)
{
	void *result = videoShared().Allocate(size,align);
	memset(result, 0, size);
	return result;
}

void freeVideoSharedMemory(void *ptr)
{
	if (ptr && AssertVerify(videoShared().IsValidPointer(ptr)))
		videoShared().Free(ptr);
}


// Video private memory is game physical
void* allocateVideoPrivateMemory(uint32_t size,uint32_t align)
{
	return videoPrivate().Allocate(size,align);
}

void* allocateVideoPrivateMemory(const Gnm::SizeAlign &sa)
{
	return allocateVideoPrivateMemory(sa.m_size,sa.m_align);
}

void freeVideoPrivateMemory(void *addr)
{
	if (addr)
		videoPrivate().Free(addr);
}

bool isAllocatedVideoMemory(void *ptr)
{
	if (ptr && (sysMemAllocator::GetCurrent().IsValidPointer(ptr) || videoPrivate().IsValidPointer(ptr)))
		return true;

	return false;
}



void copyToVideoMemory(void *dest,const void *src,uint32_t bytes)
{
	memcpy(dest, src, bytes);
}

void grcDevice::SetGsVsRingBufferData(const uint32_t *exportVertexSizeInDWord, const uint32_t maxOutputVertexCount)
{
	if (s_biggestMaxOutputVertexCount == 0xffffffff)
	{
		memcpy(&s_biggestExportVertexSizeInDWord[0], exportVertexSizeInDWord, sizeof(uint32_t)*4);
		s_biggestMaxOutputVertexCount = maxOutputVertexCount;
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			if (s_biggestExportVertexSizeInDWord[i] < exportVertexSizeInDWord[i])
				s_biggestExportVertexSizeInDWord[i] = exportVertexSizeInDWord[i];
		}
		if (s_biggestMaxOutputVertexCount < maxOutputVertexCount)
			s_biggestMaxOutputVertexCount = maxOutputVertexCount;
	}
}

void grcDevice::SetEsGsRingBufferData(const uint32_t maxExportVertexSizeInDword)
{
	if ((s_biggestMaxExportVertexSizeInDword == 0xffffffff) || (s_biggestMaxExportVertexSizeInDword < maxExportVertexSizeInDword))
		s_biggestMaxExportVertexSizeInDword = maxExportVertexSizeInDword;

}

#if RAGE_INSTANCED_TECH
void grcDevice::SetMultiWindow(const grcWindow* InstWindows[], u32 uNumVPInst)
{
	for (u32 i = 0; i < uNumVPInst; i++)
		grcDevice::SetWindow(*(InstWindows[i]),i);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////
void grcDevice::SetWindow(const grcWindow& w, int uVPIndex)
{
	uint32_t left = w.GetX(), top = w.GetY(), right = w.GetX() + w.GetWidth(), bottom = w.GetY() + w.GetHeight();
	float minDepth = FixViewportDepth(w.GetMinZ());
	float maxDepth = FixViewportDepth(w.GetMaxZ());
	float scale[3] = { w.GetWidth() * 0.5f, -w.GetHeight() * 0.5f, maxDepth - minDepth };
	float offset[3] = { w.GetWidth() * 0.5f + left, w.GetHeight() * 0.5f + top, minDepth };

	gfxc.setViewport(uVPIndex,w.GetMinZ(),w.GetMaxZ(),scale,offset);
	gfxc.setViewportScissor(uVPIndex,left,top,right,bottom,Gnm::kWindowOffsetDisable);
}

__THREAD float s_ZScale, s_ZOffset;

void grcDevice::SetWindow(const grcWindow &w)
{
	uint32_t left = w.GetX(), top = w.GetY(), right = w.GetX() + w.GetWidth(), bottom = w.GetY() + w.GetHeight();
	//float scale[3] = { w.GetWidth() * 0.5f, -w.GetHeight() * (0.5f), (w.GetMaxZ() - w.GetMinZ()) };
	//float offset[3] = { w.GetWidth() * 0.5f + left, w.GetHeight() * 0.5f + top, w.GetMinZ() };

	// There are three different scissors, let's just use the window one for now.
	// The global scissor is always tied to the size of the current render target.
	// For DX-style projection matrices (like we use), we want 1, 0 for last two parameters in normal case.
	float zMin = FixViewportDepth(w.GetMinZ());
	float zMax = FixViewportDepth(w.GetMaxZ());
	s_ZScale = (zMax-zMin);
	s_ZOffset = zMin;

	gfxc.setupScreenViewport(left,top,right,bottom,s_ZScale, s_ZOffset);
}

void grcDevice::SetScissor(int x,int y,int width,int height)
{
	// setupScreenViewport uses screen scissor, so let's use a different one (window or generic) to keep the logic nicely separated
	gfxc.setWindowScissor(x,y,x+width,y+height,Gnm::kWindowOffsetDisable);
}

void grcDevice::DisableScissor() 
{ 
	// setupScreenViewport uses screen scissor, so let's use a different one (window or generic) to keep the logic nicely separated
	gfxc.setWindowScissor(0,0,16383,16383,Gnm::kWindowOffsetDisable);
}

const int gnmDisplayBuffers = 3;

// static char *s_BeginVerticesBuffer, *s_BeginVertices;
// const u32 s_BeginVerticesBufferSize = 65536;

struct VideoInfo {
	int32_t handle;
	uint64_t* label;
	uint32_t label_num;
	uint32_t flip_base;
	uint32_t flip_index;
	uint32_t buffer_num;
	SceKernelEqueue eq;
};
static VideoInfo s_videoInfo;


struct BufferWrap
{
	const void	                    *ptr;
	unsigned	                    count;
	unsigned	                    stride;
	sce::Gnm::ResourceMemoryType    memType;
	sce::Gnm::L1CachePolicy         l1CachePolicy;
	sce::Gnm::KCachePolicy          kCachePolicy;

	BufferWrap()
	: ptr(NULL)
	, count(0)
	, stride(0)
	, memType(sce::Gnm::kResourceMemoryTypeRO)
	, l1CachePolicy(sce::Gnm::kL1CachePolicyLru)
	, kCachePolicy(sce::Gnm::kKCachePolicyLru)
	{}

	BufferWrap(const void *p, unsigned c, unsigned s, sce::Gnm::ResourceMemoryType mt, sce::Gnm::L1CachePolicy l1, sce::Gnm::KCachePolicy k)
	: ptr(p)
	, count(c)
	, stride(s)
	, memType(mt)
	, l1CachePolicy(l1)
	, kCachePolicy(k)
	{}

	explicit BufferWrap(const grcVertexBuffer &vb)
	: ptr(vb.GetLockPtr())
	, count(vb.GetVertexCount())
	, stride(vb.GetVertexStride())
	, memType(sce::Gnm::kResourceMemoryTypeRO)
	, l1CachePolicy(sce::Gnm::kL1CachePolicyLru)
	, kCachePolicy(sce::Gnm::kKCachePolicyLru)
	{}
};


grcDepthStencilStateHandle clearStates_DSS[2][2];	// clearDepth, clearStencil
grcBlendStateHandle clearStates_BS[2];

const int MAX_VERTEX_STREAMS=4;
BufferWrap s_Streams[NUMBER_OF_RENDER_THREADS][MAX_VERTEX_STREAMS+1];
const u16 *s_IndexBuffer[NUMBER_OF_RENDER_THREADS];


#if ENABLE_FAST_CLEAR

static void DrawClearQuad(eqaa::pass::Clear pass, sce::Gnm::CbMode cbMode = Gnm::kCbModeNormal)
{
	gfxc.setCbControl( cbMode, Gnm::kRasterOpSrcCopy );
	eqaa::DrawClear(pass);
	gfxc.setCbControl(Gnm::kCbModeNormal, Gnm::kRasterOpSrcCopy);
}

#endif	//ENABLE_FAST_CLEAR

#if ENABLE_FAST_CLEAR
extern DECLARE_MTR_THREAD const sce::Gnm::DepthRenderTarget *s_CurDepth;
extern DECLARE_MTR_THREAD const sce::Gnm::RenderTarget* s_CurColors[grcmrtColorCount];
extern DECLARE_MTR_THREAD sce::Gnm::NumSamples s_CurSupersampleFrequency;
extern DECLARE_MTR_THREAD u32 s_CmaskDirty;

static void ClearBuffer(const char debugMessage[], void *const dest, uint32_t size, uint32_t value, bool isColor, bool bGPU = true)
{
	if (s_AllowGPUClear && bGPU)
	{
		gfxc.triggerEvent( isColor ? Gnm::kEventTypeFlushAndInvalidateCbMeta : Gnm::kEventTypeFlushAndInvalidateDbMeta );

		eqaa::ClearBuffer(debugMessage, dest, size, value);

		if (isColor)	// used in Orbis samples, but seems to be an overkill for us
		{
			// Flush the CB color cache
			GRCDEVICE.GpuWaitOnPreviousWrites();
		}
	}else
	{
#if ENABLE_LCUE
		Assertf(0,"fillData() not supported for LCUE");
#else
		gfxc.fillData(dest, value, size, Gnm::kDmaDataBlockingEnable);
#endif	//ENABLE_LCUE
	}
}

static const uint32_t* makeClearColor(const sce::Gnm::DataFormat &format, const Color32& color)
{
	static uint32_t value[2] = {0};
	const uint32_t uc = color.GetDeviceColor();

	switch(format.getBitsPerElement())
	{
	case 8:
	case 16:
		Assert( !uc );
	case 32:
		value[0] = uc;
		break;
	case 64:
	default:
		Assert( !uc );
	}
	return value;
}
#endif	//ENABLE_FAST_CLEAR

#if DEVICE_MSAA
#if __ASSERT
static bool IsTileModeLinear(Gnm::TileMode tileMode)
{
	return (tileMode == Gnm::kTileModeDisplay_LinearAligned);
}

static bool AreTileModesSafeForMsaaResolve(Gnm::TileMode destMode, Gnm::TileMode srcMode)
{
	return IsTileModeLinear(destMode) == IsTileModeLinear(srcMode);
}
#endif	//__ASSERT

#if 0	// this might be helpful when we optimize AA processing
// since Fmask contains meta information about the tiles, we can narrow down the supersampling stages
// (directional light, postFx) to process only relevant edge samples
void grcDevice::CopyFmaskSurface(const Gnm::RenderTarget *const target, const Gnm::RenderTarget *const source)
{
	Assert(target && target->getFmaskAddress()); 
	Assert(source && source->getFmaskAddress());
	Assert(target->getFmaskTileMode() == source->getFmaskTileMode());
	Assert(target->getFmaskCompressionEnable() == source->getFmaskCompressionEnable());
	Assert(target->getNumFragments() == source->getNumFragments());
	Assert(target->getSizeInBytes() == source->getSizeInBytes());

	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta); // Flush the FMask cache
	
	sce::Gnm::Texture tempTexture;
	tempTexture.initAsFmask( target->getWidth(), target->getHeight(), 1, target->getFmaskTileMode(), target->getNumSamples(), target->getNumFragments() );

	eqaa::CopyBuffer( "Copying FMask surface", target->getFmaskAddress(), source->getFmaskAddress(), tempTexture.getSizeInBytes() );
}
#endif	//0

void grcDevice::DecompressDepthSurface(const Gnm::DepthRenderTarget *const depthTarget, bool bKeepCompressionDisabled)
{
	const Gnm::DepthRenderTarget *const target = depthTarget ? depthTarget : s_CurDepth;
	if (!target || !target->getHtileAccelerationEnable() || !target->getHtileAddress())
		return;

#if SUPPORT_RENDERTARGET_DUMP
	if (grcSetupInstance && grcSetupInstance->ShouldCaptureRenderTarget())
	{
		grcRenderTargetGNM::SaveDepthTarget( "DecompressDepth", target, DUMP_HTILE );
	}
#endif	//SUPPORT_RENDERTARGET_DUMP

	PF_AUTO_PUSH_TIMEBAR("Decompressing depth surface");
	gfxc.triggerEvent( Gnm::kEventTypeFlushAndInvalidateDbMeta );

	if (depthTarget)
	{
		grcAssertf(depthTarget != s_CurDepth, "Decompressing depth during rendering is not allowed");
		gfxc.setRenderTarget(0, NULL);
		gfxc.setDepthRenderTarget(depthTarget);
		gfxc.setupScreenViewport(0, 0, depthTarget->getWidth(), depthTarget->getHeight(), 0.5f, 0.5f);
		ResetAACount();
	}else
	{
		gfxc.waitForGraphicsWrites(target->getZWriteAddress256ByteBlocks(), target->getZSliceSizeInBytes()>>8, sce::Gnm::kWaitTargetSlotDb,
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kExtendedCacheActionFlushAndInvalidateDbCache,
			sce::Gnm::kStallCommandBufferParserDisable );
	}

	// Z_ENABLE = 0, STENCIL_ENABLE = 0
	// DEPTH_COMPRESS_DISABLE = 1, STENCIL_COMPRESS_DISABLE = 1
	Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	dbRenderControl.setStencilTileWriteBackPolicy(Gnm::kDbTileWriteBackPolicyCompressionForbidden);
	dbRenderControl.setDepthTileWriteBackPolicy(Gnm::kDbTileWriteBackPolicyCompressionForbidden);
	gfxc.setDbRenderControl(dbRenderControl);
	// DB_RENDER_OVERRIDE.DECOMPRESS_Z_ON_FLUSH = 0

	// draw full screen quad
	DrawClearQuad( eqaa::pass::quad_clear, Gnm::kCbModeDisable );

	// sync
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateDbMeta);
	GpuWaitOnPreviousWrites();

	if (!bKeepCompressionDisabled)
	{
		// restore render control
		dbRenderControl.init();
		gfxc.setDbRenderControl(dbRenderControl);
	}
}

void grcDevice::ReenableDepthSurfaceCompression()
{
	// sync
	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateDbMeta);
	GpuWaitOnPreviousWrites();

	// restore render control
	Gnm::DbRenderControl dbRenderControl;
	dbRenderControl.init();
	gfxc.setDbRenderControl(dbRenderControl);
}

void grcDevice::UntileFmaskSurface(sce::Gnm::Texture *const destTexture, const sce::Gnm::RenderTarget *const source, uint32_t sampleOffset)
{
	if (!destTexture || !source || !source->getFmaskAddress())
		return;

	sce::Gnm::RenderTarget target;
	uint32_t status = target.initFromTexture( destTexture, 0 );
	if (status)
	{
		Assertf(!status,"initFromTexture failed with code 0x%x",status);
		return;
	}

	grcTextureGNM texFmask( *source, 'f' );
	const grcDevice::MSAAMode mode( *source );
	char name[16];
	snprintf(name, sizeof(name), "Fmask%d", sampleOffset/4);

	grcRenderTarget *const targetRT = static_cast<grcTextureFactoryGNM&>(grcTextureFactory::GetInstance()).
		CreateProxyRenderTarget( name, &target, NULL, NULL );

	eqaa::UntileFmask(targetRT, &texFmask, mode, sampleOffset);

	delete targetRT; 
}

void grcDevice::LockSingleTarget(const Gnm::RenderTarget *const target)
{
	gfxc.setRenderTarget(0, target);
	gfxc.setRenderTarget(1, NULL);
	gfxc.setRenderTarget(2, NULL);
	gfxc.setRenderTarget(3, NULL);
	gfxc.setupScreenViewport(0, 0, target->getWidth(), target->getHeight(), s_ZScale, s_ZOffset);
	gfxc.setDepthRenderTarget( static_cast<Gnm::DepthRenderTarget*>(NULL) );
	
	//SetAACount( 1<<target->getNumSamples(), 1<<target->getNumFragments(), 1 );
	ResetAACount();
	// having scissor enabled ruins PS4 special passes (EliminateFastClear, DecompressFmask, etc)
	DisableScissor();

#if __ASSERT
	grcTextureFactoryGNM::SetLockSingleTarget(target);
#endif
}

static void DecompressFmaskSurface(const Gnm::RenderTarget *const target, bool relockTargets)
{
	if (!target || !target->getFmaskAddress())
		return;

	PF_AUTO_PUSH_TIMEBAR("Decompressing Fmask");
	GRCDEVICE.LockSingleTarget(target);

	gfxc.triggerEvent( Gnm::kEventTypeFlushAndInvalidateCbMeta ); // Flush the FMask cache

	// draw full screen quad using a dummy shader.
	DrawClearQuad( eqaa::pass::quad_clear, Gnm::kCbModeFmaskDecompress );

	// Flush caches again
	if (!target->getCmaskAddress())
	{
		// Flush the CB color cache
		gfxc.triggerEvent( Gnm::kEventTypeFlushAndInvalidateCbPixelData ); // FLUSH_AND_INV_CB_PIXEL_DATA
	}

	gfxc.triggerEvent( Gnm::kEventTypeFlushAndInvalidateCbMeta );
	//gfxc.waitForGraphicsWrites( target->getBaseAddress256ByteBlocks(), target->getSizeInBytes()>>8, sce::Gnm::kWaitTargetSlotCb0,
	//	sce::Gnm::kCacheActionNone, sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache );

	if (relockTargets)
	{
		RelockRenderTargets();
	}
}

static void FinalizeFastClear(const sce::Gnm::RenderTarget *const target, bool relockTargets)
{
	if (!target || !target->getCmaskAddress())
		return;

#if SUPPORT_RENDERTARGET_DUMP
	if (grcSetupInstance && grcSetupInstance->ShouldCaptureRenderTarget())
	{
		grcRenderTargetGNM::SaveColorTarget( "EliminateFastClear", target, DUMP_CMASK );
	}
#endif	//SUPPORT_RENDERTARGET_DUMP

	PF_AUTO_PUSH_TIMEBAR("Finalizing fast clear");
	GRCDEVICE.LockSingleTarget(target);

	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta);	// Flush the CMask cache

	DrawClearQuad( eqaa::pass::quad_clear, Gnm::kCbModeEliminateFastClear );

	// Wait for the draw to be finished, and flush the Cb/Db caches:
	GRCDEVICE.GpuWaitOnPreviousWrites();

	if (relockTargets)
	{
		RelockRenderTargets();
	}
}

void grcDevice::EliminateFastClear()
{
	PF_AUTO_PUSH_TIMEBAR("Finalizing fast clear (all targets)");

	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta);	// Flush the CMask cache

	DrawClearQuad( eqaa::pass::quad_clear, Gnm::kCbModeEliminateFastClear );

	// Wait for the draw to be finished, and flush the Cb/Db caches:
	GpuWaitOnPreviousWrites();
}

void grcDevice::FinishRendering(const Gnm::RenderTarget *const target, bool cmaskDirty)
{
	if (!target)
		return;
	if (target->getFmaskCompressionEnable())
		DecompressFmaskSurface(target,false);
	else if (cmaskDirty && target->getCmaskFastClearEnable())
		FinalizeFastClear(target,false);
}


static void ResolveMsaaBufferHW(Gnm::RenderTarget *const destTarget, Gnm::RenderTarget *const srcTarget)
{
	PF_AUTO_PUSH_TIMEBAR("Resolving MSAA buffer in hardware");
	GRCDEVICE.ResetAACount();

	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta);
	Assertf( AreTileModesSafeForMsaaResolve(destTarget->getTileMode(), srcTarget->getTileMode()), "MSAA: src and target must both be tiled, or both be linear" );
	Assertf( !srcTarget->getFmaskCompressionEnable() || (srcTarget->getFmaskAddress() != NULL), "MSAA: compressed render target (0x%p) must have a non-NULL FMASK surface.", srcTarget );
	//Assertf( s_dummy.m_shader->m_psStageRegisters.m_cbShaderMask == 0x0000000F, "MSAA: pixel shader must only write to MRT0." );
	
	gfxc.setRenderTarget(0, srcTarget);
	gfxc.setRenderTarget(1, destTarget);
	gfxc.setDepthRenderTarget( static_cast<Gnm::DepthRenderTarget*>(NULL) );
	gfxc.setupScreenViewport(0, 0, destTarget->getWidth(), destTarget->getHeight(), s_ZScale, s_ZOffset);
	
	// draw full screen quad using a dummy shader.
	DrawClearQuad( eqaa::pass::quad_dummy, Gnm::kCbModeResolve );

	gfxc.triggerEvent(Gnm::kEventTypeFlushAndInvalidateCbMeta);
	RelockRenderTargets();
}

struct EqaaMeta	{
	sce::Gnm::NumSamples numSamples;
	bool fmaskEnable;
	bool cmaskEnable;
	void *fmaskAddress;
	void *cmaskAddress;
	sce::Gnm::TileMode fmaskTileMode;
	bool cmaskLinear;
	u32 fmaskTiles;
	u32 cmaskBlocks;

	EqaaMeta(const sce::Gnm::RenderTarget &rt)
	: numSamples( rt.getNumSamples() )
	, fmaskEnable( rt.getFmaskCompressionEnable() )
	, cmaskEnable( rt.getCmaskFastClearEnable() )
	, fmaskAddress( rt.getFmaskAddress() )
	, cmaskAddress( rt.getCmaskAddress() )
	, fmaskTileMode( rt.getFmaskTileMode() )
	, cmaskLinear( rt.getLinearCmask() )
	, fmaskTiles( rt.getFmaskSliceNumTilesMinus1() )
	, cmaskBlocks( rt.getCmaskSliceNumBlocksMinus1() )
	{}

	void apply(sce::Gnm::RenderTarget &rt)	{
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbMeta );
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbPixelData );
		
		rt.setNumSamples( numSamples );
		rt.setFmaskCompressionEnable( fmaskEnable );
		rt.setCmaskFastClearEnable( cmaskEnable );
		rt.setFmaskAddress( fmaskAddress );
		rt.setCmaskAddress( cmaskAddress );
		rt.setFmaskTileMode( fmaskTileMode );
		rt.setLinearCmask( cmaskLinear );
		rt.setFmaskSliceNumTilesMinus1( fmaskTiles );
		rt.setCmaskSliceNumBlocksMinus1( cmaskBlocks );
	}
};

void grcDevice::ResolveMsaaBuffer(
	grcRenderTargetMSAA *const destination, 
	grcRenderTargetMSAA *const source,
	int destSliceIndex)
{
	const CoverageData &coverage = source->GetCoverageData();
	sce::Gnm::RenderTarget *const destTarget = destination->GetResolveTarget();
	sce::Gnm::RenderTarget *const srcTarget = source->GetResolveTarget();

	destTarget->setArrayView(destSliceIndex, destSliceIndex);
	
	if (!source->GetMSAA() && !coverage.donor)
	{
		//Assertf(0,"Attempted to resolve a non-AA target");
		destination->Copy(source,0, 0);
	}else if (eqaa::ResolveMeSoftly(destination,source,destSliceIndex))
	{
		//done
	}else
	if (coverage.donor)
	{
		const sce::Gnm::RenderTarget *const donor = coverage.donor->GetColorTarget();
		EqaaMeta original( *srcTarget );
		EqaaMeta( *donor ).apply( *srcTarget );
		
		ResolveMsaaBufferHW( destTarget, srcTarget );

		original.apply( *srcTarget );
	}else
	{
		ResolveMsaaBufferHW( destTarget, srcTarget );
	}

	if (s_ResolveWait)
	{
		gfxc.waitForGraphicsWrites( destTarget->getBaseAddress256ByteBlocks(), destTarget->getSliceSizeInBytes()>>8, sce::Gnm::kWaitTargetSlotCb0,
			sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2, sce::Gnm::kExtendedCacheActionFlushAndInvalidateCbCache,
			sce::Gnm::kStallCommandBufferParserDisable );
	}

	destTarget->setArrayView(0, 0);

#if SUPPORT_RENDERTARGET_DUMP
	if (grcSetupInstance && grcSetupInstance->ShouldCaptureRenderTarget())
	{
		grcRenderTargetGNM::SaveColorTarget( "ResolveSource", source->GetColorTarget(), DUMP_COLOR );
		if (coverage.donor || coverage.texture)
		{
			const sce::Gnm::RenderTarget *const meta = (coverage.donor ? coverage.donor : source)->GetColorTarget();
			grcRenderTargetGNM::SaveColorTarget( "Resolve", meta, DUMP_FMASK );
		}
		grcRenderTargetGNM::SaveColorTarget( "ResolveDest", destTarget, DUMP_COLOR );
	}
#endif	//SUPPORT_RENDERTARGET_DUMP
}
#endif	//DEVICE_MSAA

void grcDevice::FlushCaches(int cacheMask)
{
	if (cacheMask & CACHE_COLOR_DATA)
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbPixelData );
	if (cacheMask & CACHE_COLOR_META)
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateCbMeta );
	if (cacheMask & CACHE_DEPTH_META)
		gfxc.triggerEvent( sce::Gnm::kEventTypeFlushAndInvalidateDbMeta );
}

void grcDevice::SetSamplesAndFragments(u32 sampleCount
#if DEVICE_EQAA
									   , u32 fragmentCount
#endif
									   )
{
#if DEVICE_EQAA
	sm_EQAA = fragmentCount>0;
	if (!sampleCount)
		sampleCount = fragmentCount;
	else if (!sm_EQAA)
		fragmentCount = sampleCount;
	sm_MSAA = MSAAMode(sampleCount,fragmentCount);
#else
	sm_MSAA = MSAAMode(sampleCount);
#endif // DEVICE_EQAA
}

void grcDevice::InitClass(bool,bool)
{
#if GPU_DEBUGGER_MANUAL_LOAD
	if (PARAM_gpuDebugger.Get())
	{
		int startResult = 0;
		s_DebuggerHandle = sceKernelLoadStartModule("/host/x:/gta5/build/dev_ng/libSceGpuDebugger.prx", 0, NULL, 0, NULL, &startResult);
		grcAssertf(s_DebuggerHandle >= 0, "Unable to load GPU Debugger module, handle %x", s_DebuggerHandle);
		grcAssertf(startResult == SCE_OK, "Unable to start GPU Debugger, code %x", startResult);
	}
#endif	//GPU_DEBUGGER_MANUAL_LOAD

	PARAM_width.Get(s_ResolutionX);
	PARAM_height.Get(s_ResolutionY);
	
	sm_GlobalWindow = grcDisplayWindow(s_ResolutionX, s_ResolutionY, 0, 0, 1);

	for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++) {
		sm_CurrentWindows[i].Init(s_ResolutionX, s_ResolutionY, 0, 0, 1);
	}

	g_WindowWidth = sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	g_WindowHeight = sm_CurrentWindows[g_RenderThreadIndex].uHeight;
	g_InvWindowWidth = 1.0f / float(sm_CurrentWindows[g_RenderThreadIndex].uWidth);
	g_InvWindowHeight = 1.0f / float(sm_CurrentWindows[g_RenderThreadIndex].uHeight);

#if !ENABLE_FAST_CLEAR_COLOR
	PARAM_noFastClearColor.Set("");
#endif
#if !ENABLE_FAST_CLEAR_DEPTH
	PARAM_noFastClearDepth.Set("");
#endif

#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	if (Gnm::initialize()) 
		Quitf("GNM didn't initialize properly");
#endif

#if !__FINAL
	sce::Gnm::setErrorResponseLevel(sce::Gnm::kErrorResponseLevelPrintAndBreak);
#endif

	if (PARAM_multiSample.Get() || PARAM_multiFragment.Get())
	{
		u32 sampleCount=0, fragmentCount=0;
		PARAM_multiSample.Get(sampleCount);
		PARAM_multiFragment.Get(fragmentCount);
		SetSamplesAndFragments(sampleCount, fragmentCount);
	}

	// Empirical testing shows that a typical DCB:CCB size is about 6:1
	// DCB needs to be at least 64k for inline rendering.  Alignment seems to only need to be to nearest four bytes.
	// Neither can be larger than 4M-4 bytes (the indirect hardware limit).
#if MULTIPLE_RENDER_THREADS
	// Use these numbers for MT rendering
	grcGfxContext::initRing(
			144,						// context count
			16,							// CUE ring entries
			128,						// segment count
			4 * 65536,					// DCB segment size
			1 * 65536					// CCB segment size
			);
#else
	// Use these for "old school" rendering
	grcGfxContext::initRing(
			8,							// context count
			64,							// CUE ring entries
			128,						// segment count
			1 * 1024 * 1024,			// DCB segment size
			256 * 1024					// CCB segment size
		);
#endif	//MULTIPLE_RENDER_THREADS

	sm_FrameCounter = 0;

	// Noticed that even though we are only allocating 4-bytes here, it needs to
	// be 8-byte aligned for the writeAtEndOfPipe
	s_label = (volatile uint32_t*)allocateVideoSharedMemory(sizeof(uint32_t), __alignof__(uint64_t));
	*s_label = -1;

	s_nextfence = (volatile uint64_t*)allocateVideoSharedMemory(sizeof(uint64_t), __alignof__(uint64_t));

	s_renderTargets = rage_new Gnm::RenderTarget[gnmDisplayBuffers];

	s_frontBuffer = s_renderTargets+gnmDisplayBuffers-1;
	s_backBuffer = s_renderTargets;

	uint32_t width = sm_GlobalWindow.uWidth, height = sm_GlobalWindow.uHeight;
	void *buffer_address[gnmDisplayBuffers];
	u32 uFrameLimit = sm_GlobalWindow.uFrameLock;
	PARAM_frameLimit.Get(uFrameLimit);
	sm_GlobalWindow.uFrameLock = uFrameLimit;

	// Currently, display render targets must be BGRA 8888 UNORM, LinearAligned
	const Gnm::DataFormat colorFormat = Gnm::kDataFormatB8G8R8A8Unorm;
	const bool useCmask = !PARAM_noFastClearColor.Get() && s_UseCmaskBackBuffer;
	const bool stencil = !PARAM_noStencil.Get();
	const Gnm::StencilFormat stencilFormat = stencil? Gnm::kStencil8 : Gnm::kStencilInvalid;		// kStencilInvalid to disable
	const Gnm::DataFormat depthFormat = Gnm::DataFormat::build(Gnm::kZFormat32Float);

#if ENABLE_MSAA_BACK_BUFFER
	if (sm_MSAA)	// create MSAA color target
	{
		MSAAMode backMSAA = sm_MSAA;
		backMSAA.DisableCoverage();
		Gnm::SizeAlign cmaskSizeAlign, fmaskSizeAlign;
		Gnm::TileMode colorTileMode;
		const bool useFmask = sm_EQAA && backMSAA.NeedFmask();
		const bool useCmask = useFmask || (s_UseCmaskAA && s_UseFastClearBackBuffer);	//no need to fast clear the back buffer

		GpuAddress::computeSurfaceTileMode(&colorTileMode,
			s_ResolveBackBufferHW ? GpuAddress::kSurfaceTypeColorTargetDisplayable : GpuAddress::kSurfaceTypeColorTarget,
			colorFormat, sm_MSAA);
		const Gnm::SizeAlign sizeAlign = s_MSAA_Color.init( width, height, 1,
			colorFormat, colorTileMode, backMSAA.m_EnumSamples, backMSAA.m_EnumFragments,
			useCmask ? &cmaskSizeAlign : NULL,
			useFmask ? &fmaskSizeAlign : NULL );

		void *const buffer = allocateVideoPrivateMemory( sizeAlign );
		void *const cmaskAddr = useCmask ? allocateVideoPrivateMemory( cmaskSizeAlign ) : NULL;
		void *const fmaskAddr = useFmask ? allocateVideoPrivateMemory( fmaskSizeAlign ) : NULL;
		s_MSAA_Color.setAddresses( buffer, cmaskAddr, fmaskAddr );
	}

	if (sm_MSAA)	// create MSAA depth target
	{
		Gnm::SizeAlign stencilSizeAlign, htileSizeAlign;
		Gnm::TileMode depthTileMode;
		const bool useHTile = s_UseHtileAA && s_UseFastClearBackBuffer;
		
		GpuAddress::computeSurfaceTileMode(&depthTileMode, GpuAddress::kSurfaceTypeDepthOnlyTarget, depthFormat, sm_MSAA);
		const Gnm::SizeAlign depthTargetSizeAlign = s_MSAA_Depth.init( width, height, depthFormat.getZFormat(), stencilFormat, depthTileMode,
			sm_MSAA.m_EnumFragments, &stencilSizeAlign, &htileSizeAlign );

		void *const depthBufferBaseAddr = allocateVideoPrivateMemory( depthTargetSizeAlign );
		void *const stencilAddr = stencil? allocateVideoPrivateMemory( stencilSizeAlign ) : NULL;
		s_MSAA_Depth.setAddresses( depthBufferBaseAddr, stencilAddr );

		if (useHTile) {
			void *const htileAddr = allocateVideoPrivateMemory( htileSizeAlign );
			s_MSAA_Depth.setHtileAddress( htileAddr );
		}
		s_MSAA_Depth.setHtileAccelerationEnable( useHTile );
		s_MSAA_Depth.setHtileStencilDisable(!(stencil && s_UseStencilCompression));
	}
#endif	//ENABLE_MSAA_BACK_BUFFER

	for( uint32_t buffer=0; buffer<gnmDisplayBuffers; ++buffer )
	{
		Gnm::SizeAlign cmaskSizeAlign;
		Gnm::TileMode tileMode;

		GpuAddress::computeSurfaceTileMode(&tileMode, GpuAddress::kSurfaceTypeColorTargetDisplayable, colorFormat, 1);
		const Gnm::SizeAlign sizeAlign = s_renderTargets[buffer].init( width, height, 1, colorFormat, tileMode, Gnm::kNumSamples1, Gnm::kNumFragments1,
			useCmask ? &cmaskSizeAlign : NULL, NULL );
		Assert( s_renderTargets[buffer].getCmaskFastClearEnable() == useCmask );

		buffer_address[buffer] = allocateVideoPrivateMemory( sizeAlign );
		void *cmaskAddr = useCmask ? allocateVideoPrivateMemory( cmaskSizeAlign ) : NULL;
		s_renderTargets[buffer].setAddresses( buffer_address[buffer], cmaskAddr, 0 );
	}

	Gnm::SizeAlign stencilSizeAlign, htileSizeAlign;
	Gnm::TileMode depthTileMode;
	GpuAddress::computeSurfaceTileMode(&depthTileMode, GpuAddress::kSurfaceTypeDepthOnlyTarget, depthFormat, 1);
	const bool useHTile = !PARAM_noFastClearDepth.Get() && s_UseHtileBackDepth;

	const Gnm::SizeAlign depthTargetSizeAlign = s_depthTarget.init( width, height, depthFormat.getZFormat(), stencilFormat, depthTileMode, Gnm::kNumFragments1, &stencilSizeAlign, &htileSizeAlign );

	void *depthBufferBaseAddr = allocateVideoPrivateMemory( depthTargetSizeAlign );
	void *stencilAddr = stencil? allocateVideoPrivateMemory( stencilSizeAlign ) : NULL;
	s_depthTarget.setAddresses( depthBufferBaseAddr, stencilAddr );

	if (useHTile) {
		void *htileAddr = allocateVideoPrivateMemory( htileSizeAlign );
		s_depthTarget.setHtileAddress( htileAddr );
		s_depthTarget.setHtileAccelerationEnable(true);
	}
	else {
		grcWarningf("DISABLED HTILE SO RAZOR GPU CAPTURES WORK CORRECTLY");
		s_depthTarget.setHtileAccelerationEnable(false);
	}

	grcTextureFactoryGNM::sm_FrontBuffer = rage_new grcRenderTargetGNM();
	grcTextureFactoryGNM::sm_FrontBufferDepth = rage_new grcRenderTargetGNM();
	grcTextureFactoryGNM::sm_BackBuffer = rage_new grcRenderTargetGNM();
	grcTextureFactoryGNM::sm_BackBuffer->SetName("Screen");
	grcTextureFactoryGNM::sm_BackBufferDepth = rage_new grcRenderTargetGNM();
	grcTextureFactoryGNM::sm_BackBufferDepth->SetName("ScreenDepth");
	
	grcTextureFactoryGNM::SetPrivateSurfaces( s_frontBuffer, &s_depthTarget,
#if ENABLE_MSAA_BACK_BUFFER
		sm_MSAA ? &s_MSAA_Color : s_backBuffer,
		sm_MSAA ? &s_MSAA_Depth : &s_depthTarget );
#else
		s_backBuffer, &s_depthTarget );
#endif

	s_videoInfo.handle = sceVideoOutOpen(SCE_USER_SERVICE_USER_ID_SYSTEM, 
#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
		SCE_VIDEO_OUT_BUS_MAIN, 
#else
		SCE_VIDEO_OUT_BUS_TYPE_MAIN,
#endif
		0, NULL);
	SCE_GNM_ASSERT(s_videoInfo.handle >= 0);
	ASSERT_ONLY(int ret =) sceVideoOutSetFlipRate(s_videoInfo.handle, sm_GlobalWindow.uFrameLock? sm_GlobalWindow.uFrameLock-1 : 0);
	Assert(ret >= 0);
	// Prepare Equeue for Flip Sync
	// ret = sceKernelCreateEqueue(&s_videoInfo.eq, __FUNCTION__);
	// SCE_GNM_ASSERT(ret >= 0);
	// ret=sceVideoOutAddFlipEvent(s_videoInfo.eq, s_videoInfo.handle, NULL);
	// ret = Gnm::addEqEvent(s_videoInfo.eq, Gnm::kEqEventGfxEop, NULL);
	// SCE_GNM_ASSERT(ret >= 0);
	s_videoInfo.flip_index = 0;
	s_videoInfo.buffer_num = gnmDisplayBuffers;

	SceVideoOutBufferAttribute attribute;
	sceVideoOutSetBufferAttribute(&attribute,
		SCE_VIDEO_OUT_PIXEL_FORMAT_B8_G8_R8_A8_SRGB,
		SCE_VIDEO_OUT_TILING_MODE_TILE,
		SCE_VIDEO_OUT_ASPECT_RATIO_16_9,
		width, height, width);
	s_videoInfo.flip_base = sceVideoOutRegisterBuffers(s_videoInfo.handle, 0, buffer_address, gnmDisplayBuffers, &attribute );
	SCE_GNM_ASSERT(s_videoInfo.flip_base >= 0);

	grcEffect::InitClass();
	grcInitQuads();
	grcStateBlock::InitClass();

	grcDepthStencilStateDesc desc;
	desc.DepthEnable = desc.DepthWriteMask = false;
	desc.DepthFunc = grcRSV::CMP_ALWAYS;
	desc.FrontFace.StencilPassOp = desc.BackFace.StencilPassOp = grcRSV::STENCILOP_REPLACE;
	clearStates_DSS[false][false] = grcStateBlock::CreateDepthStencilState(desc);
	desc.DepthEnable = desc.DepthWriteMask = true;
	clearStates_DSS[true][false] = grcStateBlock::CreateDepthStencilState(desc);
	desc.StencilEnable = true;
	clearStates_DSS[true][true] = grcStateBlock::CreateDepthStencilState(desc);
	desc.DepthEnable = desc.DepthWriteMask = false;
	clearStates_DSS[false][true] = grcStateBlock::CreateDepthStencilState(desc);

	grcBlendStateDesc bdesc;
	clearStates_BS[true] = grcStateBlock::CreateBlendState(bdesc);
	bdesc.BlendRTDesc[0].RenderTargetWriteMask = 0;
	clearStates_BS[false] = grcStateBlock::CreateBlendState(bdesc);

	g_DefaultEffect = grcEffect::Create(sm_DefaultEffectName);
	if (!g_DefaultEffect)
		Quitf("Unable to create default effect '%s', cannot continue.",sm_DefaultEffectName);

	s_DefaultLit = g_DefaultEffect->LookupTechnique("draw");
	s_DefaultUnlit = g_DefaultEffect->LookupTechnique("unlit_draw");
	s_DefaultLitSkinned = g_DefaultEffect->LookupTechnique("drawskinned");
	s_DefaultUnlitSkinned = g_DefaultEffect->LookupTechnique("unlit_drawskinned");
	s_DefaultClear = g_DefaultEffect->LookupTechnique("Clear");
	s_DefaultClearR32 = g_DefaultEffect->LookupTechnique("ClearR32");
	s_DefaultClearArray = g_DefaultEffect->LookupTechnique("ClearArray");
	s_DefaultClearMRT2 = g_DefaultEffect->LookupTechnique("ClearMrt2");
	s_DefaultClearMRT3 = g_DefaultEffect->LookupTechnique("ClearMrt3");
	s_DefaultClearMRT4 = g_DefaultEffect->LookupTechnique("ClearMrt4");

	g_DefaultSampler = g_DefaultEffect->LookupVar("DiffuseTex");
	g_DefaultColor = g_DefaultEffect->LookupVar("GeneralParams0");

	s_MemoryCS = grcEffect::Create("common:/shaders/MemoryCS");

	grcViewport::InitClass();

	g_GPUQueries = rage_new atPool<sce::Gnm::OcclusionQueryResults>(1024);	// These are 128 bytes a pop, do we really need this many?

	// Set up secret internal stream 4 which is used for missing vertex data.
	grcFvf fvf;
	fvf.SetDiffuseChannel(true,grcFvf::grcdsColor);
	grcVertexBuffer *dummy = rage_new grcVertexBufferGNM(1,fvf,false,false,NULL);
	u32 *cpv = (u32*) dummy->LockWO();
	*cpv = 0;
	dummy->UnlockWO();
	for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++)
		s_Streams[i][MAX_VERTEX_STREAMS] = BufferWrap(*dummy);

	grcInit();

	// s_BeginVerticesBuffer = (char*) allocateVideoSharedMemory(s_BeginVerticesBufferSize, 16);
	g_grcCurrentContext = grcGfxContext::openContext(false, GetAndIncContextSequenceNumber());

	// initialize es->gs and gs->vs ring buffers (needed for geometry shader)
	sm_esgsRing = allocateVideoPrivateMemory(sce::Gnm::kGsRingSizeSetup4Mb, sce::Gnm::kAlignmentOfBufferInBytes);
	sm_gsvsRing = allocateVideoPrivateMemory(sce::Gnm::kGsRingSizeSetup4Mb, sce::Gnm::kAlignmentOfBufferInBytes);
	Assertf(sm_esgsRing && sm_gsvsRing, "esgs/gsvs ring buffer must be allocated.\n");

	// initialize global rsc table ptr
	sm_globalResourceTablePtr = allocateVideoPrivateMemory(SCE_GNM_SHADER_GLOBAL_TABLE_SIZE, sce::Gnm::kAlignmentOfBufferInBytes);
	Assertf(sm_globalResourceTablePtr, "global resource table must be allocated.\n");

	// initialize the tesselation factors buffer
#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
	sm_globalTesselationFactorsPtr = sce::Gnm::getTessellationFactorRingBufferBaseAddress();
	allocateDirectMemory( sm_globalTesselationFactorsPtr, sce::Gnm::kTfRingSizeInBytes, 64<<10 );
		//sce::Gnm::kAlignmentOfTessFactorBufferInBytes );
	Assert( sm_globalTesselationFactorsPtr == sce::Gnm::getTessellationFactorRingBufferBaseAddress() );
#else
	sm_globalTesselationFactorsPtr = sce::Gnm::getTheTessellationFactorRingBufferBaseAddress();
	allocateDirectMemory( sm_globalTesselationFactorsPtr, sce::Gnm::kTfRingSizeInBytes, 64<<10 );
		//sce::Gnm::kAlignmentOfTessFactorBufferInBytes );
	Assert( sm_globalTesselationFactorsPtr == sce::Gnm::getTheTessellationFactorRingBufferBaseAddress() );
#endif //SCE_ORBIS_SDK_VERSION
	sm_tessFactorBuffer.initAsTessellationFactorBuffer( sm_globalTesselationFactorsPtr, sce::Gnm::kTfRingSizeInBytes );
	sm_tessFactorBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
}

void grcDevice::ShutdownClass()
{

}

void grcDevice::InitSubThread()
{
	DisableScissor();		// make sure window scissor has valid extents for Razor GPU captures.
	gfxc.setGenericScissor(0,0,16382,16382,sce::Gnm::kWindowOffsetDisable);	// likewise for generic, which we otherwise don't use

#if ENABLE_MSAA_BACK_BUFFER
	if (sm_MSAA)
	{
		s_CurColors[0] = &s_MSAA_Color;
		s_CurDepth = &s_MSAA_Depth;
		s_CurSupersampleFrequency = s_MSAA_Color.getNumSamples();
	}else
#endif // ENABLE_MSAA_BACK_BUFFER
	{
		s_CurColors[0] = s_backBuffer;
		s_CurDepth = &s_depthTarget;
		s_CurSupersampleFrequency = sce::Gnm::kNumSamples1;
	}
	
	for(int i=1; i<grcmrtColorCount; ++i)
		s_CurColors[i] = NULL;
	RelockRenderTargets();
	
	gfxc.setLineWidth(8);
	gfxc.setPointSize(8,8);
	gfxc.setIndexSize(sce::Gnm::kIndexSize16);

	gfxc.setDbCountControl(
		sce::Gnm::kDbCountControlZPassIncrementEnable,		//DbCountControlZPassIncrement zPassIncrement,
		sce::Gnm::kDbCountControlPerfectZPassCountsEnable,	//DbCountControlPerfectZPassCounts perfectZPassCounts,
		0													//uint32_t log2SampleRate
	);

	grcWindow w;
	SetWindow(w);

#if ENABLE_MSAA_BACK_BUFFER
	if (sm_MSAA)
	{
		SetAACount( 1<<s_MSAA_Color.getNumSamples(), 1<<s_MSAA_Color.getNumFragments(), 1<<s_MSAA_Color.getNumFragments() );
	}
	else
#endif // ENABLE_MSAA_BACK_BUFFER
	{
		ResetAACount();
	}

	ResetClipPlanes();
	ResetDrawCallCount();

	// set up global rsc table ptr as well as esgs/gsvs ring buffer
	SetGlobalResources();

	// s_BeginVertices = s_BeginVerticesBuffer;
}

bool grcDevice::BeginFrame()
{
	PF_PUSH_TIMEBAR("sceKernelWaitEqueue");
#if 1
	{
#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
			CustomHangDetection watchdog("grcDevice::BeginFrame", GRCGFXCONTEXT_GNM_GPU_HANG_TIMEOUT_SEC);
#		endif
		// s_label is the last frame the GPU has already finished.
		// sm_FrameCounter is the next frame the CPU is about to start.
		while ((s32)(sm_FrameCounter - (*s_label+1)) > MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU) {
#			if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
				if (watchdog.pollHasTimedOut())
					grcGfxContext::reportGpuHang();
#			endif
			sceKernelUsleep(30);		// TODO: total guess
		}
	}
#else
	SceKernelEvent eopEvent;
	int eopEventCount;
	if (sceKernelWaitEqueue(s_videoInfo.eq, &eopEvent, 1, &eopEventCount, NULL))
		grcErrorf("sceKernelWaitEqueue failed.");
#endif	//1
	PF_POP_TIMEBAR();

	PF_PUSH_TIMEBAR("waitUntilSafeForRendering");
	gfxc.waitUntilSafeForRendering(s_videoInfo.handle,s_videoInfo.flip_base + s_videoInfo.flip_index);
	PF_POP_TIMEBAR();

	grcStateBlock::MakeDirty();
	grcStateBlock::Default();

	sce::Gnm::ClipControl clipControl;
	clipControl.init();
	clipControl.setClipSpace(sce::Gnm::kClipControlClipSpaceDX);
	gfxc.setClipControl(clipControl);

	InitSubThread();

	s_FencePool.CleanUpOverflow();

	return true;
}


void grcDevice::EndFrame(const grcResolveFlags*)
{
	// Assert(s_BeginVertices <= s_BeginVerticesBuffer + s_BeginVerticesBufferSize);
#if DEVICE_EQAA
	eqaa::Init();
#endif

#if ENABLE_MSAA_BACK_BUFFER
	if (sm_MSAA)
	{
		grcRenderTargetGNM backProxy( "BackBufferProxy", s_backBuffer, NULL, NULL );
		static_cast<grcRenderTargetGNM*>( grcTextureFactory::GetInstance().GetBackBuffer() )->Resolve( &backProxy );
		//ResolveMsaaBuffer( s_backBuffer, &s_MSAA_Color );
	}else
#endif	//ENABLE_MSAA_BACK_BUFFER
	{
		FinalizeFastClear( s_backBuffer, false );
	}

#if GRCGFXCONTEXT_GNM_CONTEXT_STACK
	// This is sad.  grcDevice::EndFrame is NOT called at the end of the frame.
	// For now rather than making the drastic change of fixing that, just hack
	// the fault context stack so we don't get it all messed up.
	const u32 hackFaultContextSP = grcGfxContext::HackGetFaultContextSP();
	for(u32 i=0; i<hackFaultContextSP; ++i)
	{
		grcGfxContext::PopFaultContext();
	}
#endif

	gfxc.writeAtEndOfPipe(Gnm::kEopFlushCbDbCaches, Gnm::kEventWriteDestMemory, const_cast<uint32_t*>(s_label), Gnm::kEventWriteSource32BitsImmediate, sm_FrameCounter, Gnm::kCacheActionNone, Gnm::kCachePolicyLru);

	grcGfxContext::submitAndFlip(s_videoInfo.handle, s_videoInfo.flip_base + s_videoInfo.flip_index, sm_GlobalWindow.uFrameLock? SCE_VIDEO_OUT_FLIP_MODE_VSYNC : SCE_VIDEO_OUT_FLIP_MODE_HSYNC, 0);

	g_grcCurrentContext = grcGfxContext::openContext(false, GetAndIncContextSequenceNumber());
	gfxc.initializeDefaultHardwareState();

	// Ensure the correct CU masks are setup properly for each shader stage.  At
	// least on SDK1.7*, initializeDefaultHardwareState is not doing it
	// correctly.
	const u32 waveLimit     = 0;
	const u32 lockThreshold = 0;
	gfxc.setGraphicsShaderControl(sce::Gnm::kShaderStagePs, grcGfxContext::PS_CU_MASK_DEFAULT, waveLimit, lockThreshold);
	gfxc.setGraphicsShaderControl(sce::Gnm::kShaderStageVs, grcGfxContext::VS_CU_MASK_DEFAULT, waveLimit, lockThreshold);
	gfxc.setGraphicsShaderControl(sce::Gnm::kShaderStageGs, grcGfxContext::GS_CU_MASK_DEFAULT, waveLimit, lockThreshold);
	gfxc.setGraphicsShaderControl(sce::Gnm::kShaderStageEs, grcGfxContext::ES_CU_MASK_DEFAULT, waveLimit, lockThreshold);
	gfxc.setGraphicsShaderControl(sce::Gnm::kShaderStageHs, grcGfxContext::HS_CU_MASK_DEFAULT, waveLimit, lockThreshold);
	gfxc.setGraphicsShaderControl(sce::Gnm::kShaderStageLs, grcGfxContext::LS_CU_MASK_DEFAULT, waveLimit, lockThreshold);


#if GRCGFXCONTEXT_GNM_CONTEXT_STACK
	for(u32 i=0; i<hackFaultContextSP; ++i)
	{
		grcGfxContext::PushFaultContext("HACK");
	}
#endif

	++sm_FrameCounter;

	if (++s_videoInfo.flip_index == gnmDisplayBuffers)
		s_videoInfo.flip_index = 0;

	s_frontBuffer = s_backBuffer;
	if (++s_backBuffer == s_renderTargets + gnmDisplayBuffers)
		s_backBuffer = s_renderTargets;

	grcTextureFactoryGNM::SetPrivateSurfaces( s_frontBuffer, &s_depthTarget,
#if ENABLE_MSAA_BACK_BUFFER
		sm_MSAA ? &s_MSAA_Color : s_backBuffer,
		sm_MSAA ? &s_MSAA_Depth : &s_depthTarget
#else
		s_backBuffer, &s_depthTarget
#endif
		);
}

void grcDevice::SetViewportScissor(bool bEnable)
{
	if (s_bScanMode_ViewportScissor != bEnable)
	{
		s_bScanMode_ViewportScissor = bEnable;

		gfxc.setScanModeControl(
				s_ActiveNumSamples ? sce::Gnm::kScanModeControlAaEnable : sce::Gnm::kScanModeControlAaDisable,
				(Gnm::ScanModeControlViewportScissor)s_bScanMode_ViewportScissor
#if SCE_ORBIS_SDK_VERSION < (0x01700000u)
				, s_ActiveNumSamples&&0 ? sce::Gnm::kScanModeControlLineStippleEnable : sce::Gnm::kScanModeControlLineStippleDisable
#endif
				);
	}
}

void grcDevice::SetGlobalResources()
{
	gfxc.setGlobalResourceTableAddr(sm_globalResourceTablePtr);
	
	gfxc.setGlobalDescriptor(Gnm::kShaderGlobalResourceTessFactorBuffer, &sm_tessFactorBuffer);

	if (s_biggestMaxOutputVertexCount != 0xffffffff)
	{
		Assertf(sm_gsvsRing, "gsvs ring buffer is not allocated.\n");
		gfxc.setGsVsRingBuffers(sm_gsvsRing,sce::Gnm::kGsRingSizeSetup4Mb,s_biggestExportVertexSizeInDWord,s_biggestMaxOutputVertexCount);
	}

	if (s_biggestMaxExportVertexSizeInDword != 0xffffffff)
	{
		Assertf(sm_esgsRing, "esgs ring buffer is not allocated.\n");
		gfxc.setEsGsRingBuffer(sm_esgsRing,sce::Gnm::kGsRingSizeSetup4Mb,s_biggestMaxExportVertexSizeInDword);
	}
}

/* How to use this in a shader:
	### Case "s_DebugAccumulate == false": ###
	passthrough {
		RW_RegularBuffer<float4> out_vb : register(u0);	// ...or change vtx_debug below to whatever you need, even the entire output struct
	}

	Add uint idx:S_VERTEX_ID as the last parameter to your vertex function.
	out_vb[idx] = transPos;		// where transPos is the output data you want to debug.

	### Case "s_DebugAccumulate == true": ###
	AppendStructuredBuffer<float4> out_buf : register(u0);
	...
	out_vb.Append(transPos);	// or any other data
*/
bool g_EnableShaderDebugging;
#if __DEV && 0
struct vtx_debug { float x,y,z,w; } static *out_vb;
static int *out_count;
static const bool s_DebugAccumulate = true;
static sce::Gnm::ShaderStage getDebugStage()
{
	//return grcVertexProgram::GetCurrent()->GetGnmStage();
	return sce::Gnm::kShaderStagePs;
	//return grcDomainProgram::GetCurrentShaderStage();
}
static void PreDebugCall()
{
	const uint32_t numElements = 65536;
	if (!out_vb) out_vb = (vtx_debug*) allocateVideoSharedMemory(numElements*sizeof(vtx_debug), 256);
	memset(out_vb,-1,numElements*sizeof(vtx_debug));
	grcBuffer outBuffer; 
	outBuffer.initAsRegularBuffer(out_vb,sizeof(vtx_debug),numElements);
	outBuffer.setResourceMemoryType( sce::Gnm::kResourceMemoryTypeGC );
	Assertf(grcVertexProgram::GetCurrent(), "no currently bound vs.\n");
	const sce::Gnm::ShaderStage stage = getDebugStage();
	gfxc.setRwBuffers(stage, 0, 1, &outBuffer);
	if (s_DebugAccumulate)
	{
		gfxc.clearAppendConsumeCounters( 0, 0, 1, 0 );
		gfxc.setAppendConsumeCounterRange( stage, 0, 4 );
	}
}
static void PostDebugCall(int c)
{
	const sce::Gnm::ShaderStage stage = getDebugStage();
	if (s_DebugAccumulate)
	{
		if (!out_count) out_count = (int*) allocateVideoSharedMemory(sizeof(int), 256);
		*out_count = -1;
		GRCDEVICE.GpuWaitOnPreviousWrites();
		gfxc.readAppendConsumeCounters(out_count, 0, 0, 1);
	}
	gfxc.setRwBuffers(stage, 0, 1, NULL);
	gfxc.setAppendConsumeCounterRange(stage, 0, 0);

	// This forces the kick and waits for completion.
	GRCDEVICE.CpuWaitOnGpuIdle();
	static const int maxVerts = 16;
	Assert( !s_DebugAccumulate );
	const int total = s_DebugAccumulate ? *out_count : c<maxVerts ? c : maxVerts;
	for (int i=0; i<total; i++)
		Displayf("%02d. %f %f %f %f",i,out_vb[i].x,out_vb[i].y,out_vb[i].z,out_vb[i].w);
}
#define VERTEX_DEBUG_CALL(x,c) do { if (g_EnableShaderDebugging) { PreDebugCall(); x; PostDebugCall(c); } else x; } while (0)
#else
#define VERTEX_DEBUG_CALL(x,c) x
#endif	//__DEV && 0


static DECLARE_MTR_THREAD u32 s_DrawCount;
static DECLARE_MTR_THREAD grcDrawMode s_grcDrawMode;
static Gnm::PrimitiveType s_RemapPrim[] = {
	Gnm::kPrimitiveTypePointList,
	Gnm::kPrimitiveTypeLineList,
	Gnm::kPrimitiveTypeLineStrip,
	Gnm::kPrimitiveTypeTriList,
	Gnm::kPrimitiveTypeTriStrip,
	Gnm::kPrimitiveTypeTriFan,
	Gnm::kPrimitiveTypeQuadList,
	Gnm::kPrimitiveTypeRectList,
	Gnm::kPrimitiveTypeTriListAdjacency,
};
CompileTimeAssert(NELEM(s_RemapPrim) == drawModesTotal);

// Map an index count to a primitive count for a given grcDrawMode
#if DRAWABLE_STATS
static u32 IndexCountToPrimitiveCount(u32 idxCount, grcDrawMode dm)
{
	switch(dm) {
	case drawPoints: return idxCount;
	case drawLines: return idxCount/2;
	case drawLineStrip: return idxCount-1;
	case drawTris: return idxCount/3;
	case drawTriStrip: return idxCount-2;
	case drawTriFan: return idxCount-2;
	case drawQuads: return idxCount / 4;
	case drawRects: return idxCount / 4;
	case drawTrisAdj: return idxCount / 6;
	default: return 0;
	}
}
#endif // DRAWABLE_STATS

static void BindVertexStreams()
{
	const grcVertexDeclaration *decl = s_CurrentDecl;
	const BufferWrap *const wraps = s_Streams[g_RenderThreadIndex];

	grcVertexProgram::GetCurrent()->Bind(decl);
	if (!g_VertexDeclSetup)
			return;
	
	const sce::Gnm::ShaderStage stage = grcVertexProgram::GetCurrent()->GetGnmStage();
	static const unsigned invalidBuffer = 15;
	sce::Gnm::Buffer buffers[invalidBuffer+1] = {0};
	int numBuffers = 0;
	if (g_VertexDeclSetup->IsFetchShaderRemapped)
	{
		for (int j=0; j<g_VertexDeclSetup->InputCount; j++) {
			const int i = g_VertexDeclSetup->RemapTable[j];
			const BufferWrap &wrap = wraps[decl->Streams[i]];
			initAsVertexBuffer(buffers[j], (char*)wrap.ptr + decl->Offsets[i],
				i==invalidBuffer ? 0 : wrap.stride,
				i==invalidBuffer ? 4 : wrap.count, decl->VertexDword3[i]
				WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(,s_CurrentDecl->DataFormats[i]));
			buffers[j].setResourceMemoryType(wrap.memType, wrap.l1CachePolicy, wrap.kCachePolicy);
		}
		numBuffers = g_VertexDeclSetup->InputCount;
	}else
	{
		u32 fillMask = 0;
		for (int j=0; j<g_VertexDeclSetup->InputCount; j++) {
			const u8 bufId = g_VertexDeclSetup->RemapSemanticTable[j];
			const int i = g_VertexDeclSetup->RemapTable[j];
			if (bufId == 0xFF || i == invalidBuffer)
				continue;
			Assert( bufId <= invalidBuffer );
			const BufferWrap &wrap = wraps[decl->Streams[i]];
			initAsVertexBuffer(buffers[bufId], (char*)wrap.ptr + decl->Offsets[i],
				wrap.stride, wrap.count, decl->VertexDword3[i]
				WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(,s_CurrentDecl->DataFormats[i]));
			buffers[bufId].setResourceMemoryType(wrap.memType, wrap.l1CachePolicy, wrap.kCachePolicy);
			fillMask |= 1<<bufId;
			if (numBuffers <= bufId)
				numBuffers = bufId+1;
		}
		// patch the "holes" in the buffer array
		for (int j=0; (fillMask+1) < (1<<numBuffers); ++j)
		{
			if (fillMask & (1<<j))
				continue;
			const BufferWrap &wrap = wraps[decl->Streams[15]];
			initAsVertexBuffer(buffers[j], (char*)wrap.ptr, 0, 4, decl->VertexDword3[15]
				WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(,s_CurrentDecl->DataFormats[15]));
			buffers[j].setResourceMemoryType(wrap.memType, wrap.l1CachePolicy, wrap.kCachePolicy);
			fillMask |= 1<<j;
		}
	}
	gfxc.setVertexBuffers(stage, 0, numBuffers, buffers);
}

//forward declaration
static void SetStreamSourceRaw(u32, const void *ptr, unsigned, unsigned, sce::Gnm::ResourceMemoryType, sce::Gnm::L1CachePolicy, sce::Gnm::KCachePolicy);

GRC_ALLOC_SCOPE_DECLARE_GLOBAL(static, s_BeginVerticesAllocScope)

void* grcDevice::BeginVertices(grcDrawMode dm,u32 count,u32 size)
{
	GRC_ALLOC_SCOPE_PUSH(s_BeginVerticesAllocScope)

	grcStateBlock::Flush();
	s_DrawCount = count;
	s_grcDrawMode = dm;
	Assert(size == s_CurrentDecl->Stream0Size);
	u32 total = count * size;
	TrapGT(total,BEGIN_VERTICES_MAX_SIZE);
	total += ENABLE_VERTEX_BUFFER_PAD ? size : 0;
	total = (total + 15) & ~15;

	gfxc.setPrimitiveType(s_RemapPrim[dm]);
	void *result = gfxc.allocateFromCommandBuffer(total, sce::Gnm::kEmbeddedDataAlignment16);

	StoreBufferEnd(result, count, size);
	// LCUE requires the shader to be bound BEFORE resources are set.  We also need g_VertexShaderInputCount/Remap to be updated.
	SetStreamSourceRaw(0, result, count, size, grcGfxContext::BUFFER_MEM_TYPE, grcGfxContext::BUFFER_L1_CACHE_POLICY, grcGfxContext::BUFFER_K_CACHE_POLICY);
	BindVertexStreams();	//sometimes done in the End* functions as well, so we can optimize this out

	return result;
}

void* grcDevice::BeginIndexedVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize,u32 indexCount,void** vertexPtr,void** indexPtr, u32 streamID)
{
	GRC_ALLOC_SCOPE_PUSH(s_BeginVerticesAllocScope)

	Assert(streamID || vertexSize == s_CurrentDecl->Stream0Size);	//can't really check for non-0 steamID

	grcStateBlock::Flush();
	s_DrawCount = indexCount;
	s_grcDrawMode = dm;
	const u32 indexSize = sizeof(u16);
	u32 total = vertexCount*vertexSize + indexCount*indexSize;
	TrapGT(total,BEGIN_VERTICES_MAX_SIZE);
	total = (total + 15) & ~15;

	void *const result = gfxc.allocateFromCommandBuffer(total, sce::Gnm::kEmbeddedDataAlignment16);
	void *const indices = (char*)result + vertexCount*vertexSize;

	StoreBufferEnd(result, vertexCount, vertexSize);
	// LCUE requires the shader to be bound BEFORE resources are set.  We also need g_VertexShaderInputCount/Remap to be updated.
	SetStreamSourceRaw(streamID, result, vertexCount, vertexSize,
		grcGfxContext::BUFFER_MEM_TYPE, grcGfxContext::BUFFER_L1_CACHE_POLICY, grcGfxContext::BUFFER_K_CACHE_POLICY);

	gfxc.setPrimitiveType(s_RemapPrim[dm]);

	if (vertexPtr)
		*vertexPtr = result;
	if (indexPtr)
		*indexPtr = indices;

	return result;
}

void* grcDevice::BeginIndexedInstancedVertices(grcDrawMode dm, u32 instanceCount, u32 instanceSize, u32 indexCount, void** instancePtr, void** indexPtr, u32 streamID) 
{
	Assert(dm != drawTriFan);
	return BeginIndexedVertices(dm, instanceCount, instanceSize, indexCount, instancePtr, indexPtr, streamID);
}

void grcDevice::EndCreateVertices(const void *UNUSED_PARAM(bufferEnd))
{
}

void grcDevice::EndCreateVertices(u32 UNUSED_PARAM(vertexCount))
{
}

void grcDevice::EndCreateVertices(const void *UNUSED_PARAM(bufferEnd), u32 UNUSED_PARAM(noOfIndices), const grcIndexBuffer &UNUSED_PARAM(indexBuffer))
{
}

#if RAGE_INSTANCED_TECH
void grcDevice::EndCreateInstancedVertices(const void *UNUSED_PARAM(bufferEnd))
{
}

void grcDevice::EndCreateInstancedVertices(const void* UNUSED_PARAM(bufferEnd), int UNUSED_PARAM(numInst))
{
}
#endif // RAGE_INSTANCED_TECH

void grcDevice::EndCreateIndexedVertices(u32 UNUSED_PARAM(indexCount), u32 UNUSED_PARAM(vertexCount))
{
}

void grcDevice::EndCreateIndexedInstancedVertices(const void *UNUSED_PARAM(bufferEnd), u32 UNUSED_PARAM(vertexSize), u32 UNUSED_PARAM(noOfIndices), const grcVertexBuffer &UNUSED_PARAM(vertexBuffer), const grcIndexBuffer &UNUSED_PARAM(indexBuffer), u32 UNUSED_PARAM(numInstances), u32 UNUSED_PARAM(startIndex), u32 UNUSED_PARAM(startVertex), u32 UNUSED_PARAM(startInstance))
{
}

void grcDevice::DrawVertices(const void *bufferEnd)
{
	VerifyBufferEnd(bufferEnd);

	// TODO: Do we need fences here to make sure the data isn't tossed too quickly?
	// Or do a reserve call in BeginVertices to make sure allocateFromCommandBuffer + draw call is in same packet?
	if (NotifyDrawCall(0))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexAuto(s_DrawCount),s_DrawCount);

#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += s_DrawCount;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(s_DrawCount, s_grcDrawMode);
		}
#endif
	}
}

void grcDevice::DrawVertices(u32 vertexCount)
{
	Assert(vertexCount < s_DrawCount);
	s_DrawCount = Min(vertexCount, s_DrawCount);

	EndVertices((void*)0);
}

void grcDevice::DrawVertices(const void *bufferEnd, u32 noOfIndices, const grcIndexBuffer &indexBuffer)
{
	VerifyBufferEnd(bufferEnd);

	//SetUpPriorToDraw(s_grcDrawMode);
	SetIndices(indexBuffer);
	BindVertexStreams();

	if(noOfIndices && NotifyDrawCall(0))
	{
		VERTEX_DEBUG_CALL( gfxc.drawIndex(noOfIndices,indexBuffer.GetUnsafeReadPtr()), noOfIndices );

#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += noOfIndices;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(noOfIndices, s_grcDrawMode);
		}
#endif
	}
}

#if RAGE_INSTANCED_TECH
void grcDevice::DrawInstancedVertices(const void *bufferEnd)
{
	VerifyBufferEnd(bufferEnd);
	
	gfxc.setNumInstances(grcViewport::GetNumInstVP());
	if (NotifyDrawCall(1))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexAuto(s_DrawCount),s_DrawCount);

#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += s_DrawCount;
			g_pCurrentStatsBucket->TotalPrimitives +=  IndexCountToPrimitiveCount(s_DrawCount, s_grcDrawMode);
		}
#endif
	}
	gfxc.setNumInstances(1);
}

void grcDevice::DrawInstancedVertices(const void* bufferEnd, int numInst)
{
	VerifyBufferEnd(bufferEnd);
	
	// TODO: Do we need fences here to make sure the data isn't tossed too quickly?
	// Or do a reserve call in BeginVertices to make sure allocateFromCommandBuffer + draw call is in same packet?

	gfxc.setNumInstances(numInst);
	if (NotifyDrawCall(1))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexAuto(s_DrawCount),s_DrawCount);

#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += s_DrawCount;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(s_DrawCount, s_grcDrawMode);
		}
#endif
	}
	gfxc.setNumInstances(1);
}
#endif // RAGE_INSTANCED_TECH

void grcDevice::DrawIndexedVertices(u32 indexCount, u32 vertexCount)
{
	(void)vertexCount;

	if (NotifyDrawCall(0))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexOffset(0,indexCount),indexCount);

#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += indexCount;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(indexCount, s_grcDrawMode);
		}
#endif
	}
}

void grcDevice::DrawIndexedInstancedVertices(const void *bufferEnd, u32 vertexSize, u32 noOfIndices, const grcVertexBuffer &vertexBuffer, const grcIndexBuffer &indexBuffer, u32 numInstances, u32 startIndex, u32 startVertex, u32 startInstance)
{
	VerifyBufferEnd(bufferEnd);
	Assert(startVertex==0 && startInstance==0);

	//gfxc.setPrimitiveType(s_RemapPrim[dm]);	//was called by Begin*

	SetIndices(indexBuffer);
	gfxc.setIndexBuffer(indexBuffer.GetUnsafeReadPtr());
	SetStreamSource(0, vertexBuffer, 0, vertexSize);
	BindVertexStreams();

	(void)startVertex;
	(void)startInstance;

	gfxc.setNumInstances(numInstances);
	if (NotifyDrawCall(1))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexOffset(startIndex,noOfIndices),noOfIndices);

#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += noOfIndices;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(noOfIndices, s_grcDrawMode);
		}
#endif
	}
	gfxc.setNumInstances(1);
}

void grcDevice::ReleaseVertices(const void *UNUSED_PARAM(bufferEnd))
{
	GRC_ALLOC_SCOPE_POP(s_BeginVerticesAllocScope)
}

void grcDevice::ReleaseVertices(u32 UNUSED_PARAM(vertexCount))
{
	ReleaseVertices((void*)0);
}

void grcDevice::ReleaseVertices(const void *UNUSED_PARAM(bufferEnd), u32 UNUSED_PARAM(noOfIndices), const grcIndexBuffer &UNUSED_PARAM(indexBuffer))
{
	ReleaseVertices((void*)0);
}

#if RAGE_INSTANCED_TECH
void grcDevice::ReleaseInstancedVertices(const void *UNUSED_PARAM(bufferEnd))
{
	ReleaseVertices((void*)0);
}

void grcDevice::ReleaseInstancedVertices(const void* UNUSED_PARAM(bufferEnd), int UNUSED_PARAM(numInst))
{
	ReleaseVertices((void*)0);
}
#endif // RAGE_INSTANCED_TECH

void grcDevice::ReleaseIndexedVertices(u32 UNUSED_PARAM(indexCount), u32 UNUSED_PARAM(vertexCount))
{
	ReleaseVertices((void*)0);
}

void grcDevice::ReleaseIndexedInstancedVertices(const void *UNUSED_PARAM(bufferEnd), u32 UNUSED_PARAM(vertexSize), u32 UNUSED_PARAM(noOfIndices), const grcVertexBuffer &UNUSED_PARAM(vertexBuffer), const grcIndexBuffer &UNUSED_PARAM(indexBuffer), u32 UNUSED_PARAM(numInstances), u32 UNUSED_PARAM(startIndex), u32 UNUSED_PARAM(startVertex), u32 UNUSED_PARAM(startInstance))
{
	ReleaseVertices((void*)0);
}

void grcDevice::ClearStreamSource(u32 s)
{
	s_Streams[g_RenderThreadIndex][s] = BufferWrap();
}

static void SetStreamSourceRaw(u32 s, const void *ptr, unsigned count, unsigned stride, sce::Gnm::ResourceMemoryType memType, sce::Gnm::L1CachePolicy l1CachePolicy, sce::Gnm::KCachePolicy kCachePolicy)
{
	s_Streams[g_RenderThreadIndex][s] = BufferWrap(ptr,count,stride,memType,l1CachePolicy,kCachePolicy);
}

void grcDevice::SetStreamSource(u32 s,const grcVertexBuffer& v, u32 ASSERT_ONLY(offset), u32 ASSERT_ONLY(stride))
{
	Assert( offset == 0 && stride == v.GetVertexStride() );
	s_Streams[g_RenderThreadIndex][s] = BufferWrap(v);
}

void grcDevice::SetIndices(const u16 *indices)
{
	s_IndexBuffer[g_RenderThreadIndex] = indices;
}

void grcDevice::SetIndices(const grcIndexBuffer &ib)
{
	SetIndices((u16*)ib.GetUnsafeReadPtr());
}

void grcDevice::SetVertexShaderConstant(int,const float*,int)
{

}

void* grcDevice::BeginVertexShaderConstantF(SKINNING_CBUFFER_TYPE address,u32 sizeBytes)
{
	return address->BeginUpdate(sizeBytes);
}

void grcDevice::EndVertexShaderConstantF(SKINNING_CBUFFER_TYPE address)
{
	address->EndUpdate();
}

void grcDevice::SetVertexShaderConstantF(int /*address*/, const float *data, int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*4*count);
}

void grcDevice::DrawIndexedPrimitive(grcDrawMode dm,const grcVertexDeclaration *decl,const grcVertexBuffer &vb,const grcIndexBuffer &ib, int customIndexCount)
{
	AssertMsg(customIndexCount > 0, "DrawIndexedPrimitive : Index count is less than one\n");

	s_Streams[g_RenderThreadIndex][0] = BufferWrap(vb);
	s_CurrentDecl = decl;

	SetUpPriorToDraw(dm);

#if RAGE_INSTANCED_TECH
	if (grcViewport::GetInstancing())
	{
		gfxc.setNumInstances(grcViewport::GetVPInstCount());
		if (NotifyDrawCall(1))
		{
			int indexCount = customIndexCount? customIndexCount : ib.GetIndexCount();
			VERTEX_DEBUG_CALL(gfxc.drawIndex(indexCount, ib.GetUnsafeReadPtr()),vb.GetVertexCount());

#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += indexCount;
			g_pCurrentStatsBucket->TotalPrimitives += indexCount;
		}
#endif
		}
		gfxc.setNumInstances(1);
	}
	else
#endif
	{
		if (NotifyDrawCall(0))
		{
			int indexCount = customIndexCount? customIndexCount : ib.GetIndexCount();
			VERTEX_DEBUG_CALL(gfxc.drawIndex(indexCount, ib.GetUnsafeReadPtr()),vb.GetVertexCount());
#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += indexCount;
			g_pCurrentStatsBucket->TotalPrimitives += indexCount;
		}
#endif
		}
	}
}

void grcDevice::DrawPrimitive(grcDrawMode dm,int /*startVertex*/,int vertexCount)
{
#if RAGE_INSTANCED_TECH
	Assertf(grcViewport::GetInstancing() == false, "shouldn't be vp instancing.\n");
#endif
	AssertMsg(vertexCount > 0, "DrawPrimitive : Vertex count is less than one\n");

	SetUpPriorToDraw(dm);

	if (NotifyDrawCall(0))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexAuto(vertexCount),vertexCount);
#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += vertexCount;
			g_pCurrentStatsBucket->TotalPrimitives += vertexCount;
		}
#endif
	}
}

void grcDevice::DrawPrimitive(grcDrawMode dm, const grcVertexDeclaration *decl,const grcVertexBuffer &vb,int /*startVertex*/, int vertexCount)
{
#if RAGE_INSTANCED_TECH
	Assertf(grcViewport::GetInstancing() == false, "shouldn't be vp instancing.\n");
#endif
	AssertMsg(vertexCount > 0, "DrawPrimitive : Vertex count is less than one\n");

	// optimally, we should better pass *vb as an argument, since it's really optional
	s_Streams[g_RenderThreadIndex][0] = &vb ? BufferWrap(vb) : BufferWrap();
	s_CurrentDecl = decl;

	SetUpPriorToDraw(dm);

	if (NotifyDrawCall(0))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexAuto(vertexCount),vertexCount);
#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += vertexCount;
			g_pCurrentStatsBucket->TotalPrimitives += vertexCount;
		}
#endif
	}
}

void grcDevice::DrawIndexedPrimitive(grcDrawMode dm,int startIndex,int indexCount)
{
	AssertMsg(indexCount > 0, "DrawIndexedPrimitive : Index count is less than one\n");

	SetUpPriorToDraw(dm);
	gfxc.setIndexBuffer(s_IndexBuffer[g_RenderThreadIndex]);

#if RAGE_INSTANCED_TECH
	if (grcViewport::GetInstancing())
	{
		gfxc.setNumInstances(grcViewport::GetVPInstCount());
		if (NotifyDrawCall(1))
		{
			VERTEX_DEBUG_CALL(gfxc.drawIndexOffset(startIndex, indexCount), s_Streams[g_RenderThreadIndex][0].count);
#if DRAWABLE_STATS
			if(g_pCurrentStatsBucket)
			{
				u32 numIndices = indexCount * grcViewport::GetVPInstCount();
				g_pCurrentStatsBucket->TotalIndices += numIndices;
				g_pCurrentStatsBucket->TotalPrimitives += numIndices;
			}
#endif
		}
		gfxc.setNumInstances(1);
	}
	else
#endif
	{
		if (NotifyDrawCall(0))
		{
			VERTEX_DEBUG_CALL(gfxc.drawIndexOffset(startIndex, indexCount), s_Streams[g_RenderThreadIndex][0].count);
#if DRAWABLE_STATS
			if(g_pCurrentStatsBucket)
			{
				g_pCurrentStatsBucket->TotalIndices += indexCount;
				g_pCurrentStatsBucket->TotalPrimitives += indexCount;
			}
#endif
		}
	}
}

void grcDevice::DrawInstancedPrimitive(grcDrawMode dm, int vertexCountPerInstance, int instanceCount, int startVertex, int startInstance, bool alreadySetupPriorToDraw)
{
	AssertMsg(instanceCount > 0, "DrawInstancedPrimitive : Instance count is less than one\n");
	AssertMsg(vertexCountPerInstance > 0, "DrawInstancedPrimitive : vertexCountPerInstance is less than one\n");
	AssertMsg(startVertex >= 0, "DrawInstancedPrimitive : startVertex is less than zero\n");
	AssertMsg(startInstance >= 0, "DrawInstancedPrimitive : startInstance is less than zero\n");

	if (!alreadySetupPriorToDraw)
		gfxc.setNumInstances(instanceCount);

	SetUpPriorToDraw(dm);

	if (NotifyDrawCall(1))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexAuto(vertexCountPerInstance),vertexCountPerInstance);
#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			u32 numVertices = vertexCountPerInstance * instanceCount;
			g_pCurrentStatsBucket->TotalIndices += numVertices;
			g_pCurrentStatsBucket->TotalPrimitives += numVertices;
		}
#endif
	}

	gfxc.setNumInstances(1);
}

void grcDevice::DrawInstancedIndexedPrimitive(grcDrawMode dm, int indexCountPerInstance, int instanceCount, int startIndex, int startVertex, int startInstance, bool alreadySetupPriorToDraw)
{
	AssertMsg(indexCountPerInstance > 0, "DrawInstancedIndexedPrimitive : indexCountPerInstance is less than one\n");
	AssertMsg(instanceCount > 0, "DrawInstancedIndexedPrimitive : Instance count is less than one\n");
	AssertMsg(startIndex >= 0, "DrawInstancedIndexedPrimitive : startIndex is less than zero\n");
	AssertMsg(startVertex >= 0, "DrawInstancedIndexedPrimitive : startVertex is less than zero\n");
	AssertMsg(startInstance >= 0, "DrawInstancedIndexedPrimitive : startInstance is less than zero\n");

	if( !alreadySetupPriorToDraw )
		SetUpPriorToDraw(dm);

	gfxc.setNumInstances(instanceCount);
	gfxc.setIndexBuffer(s_IndexBuffer[g_RenderThreadIndex]);
	if (NotifyDrawCall(1))
	{
		VERTEX_DEBUG_CALL(gfxc.drawIndexOffset(startIndex, indexCountPerInstance), s_Streams[g_RenderThreadIndex][0].count);
#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			u32 numIndices = indexCountPerInstance * instanceCount;
			g_pCurrentStatsBucket->TotalIndices += numIndices;
			g_pCurrentStatsBucket->TotalPrimitives += numIndices;
		}
#endif
	}
	gfxc.setNumInstances(1);
}


void grcDevice::CSEnableAutomaticGpuFlush(bool UNUSED_PARAM(enable))
{
}

// copied from samples/toolkit.cpp
void grcDevice::SynchronizeComputeToGraphics()
{
	volatile u64 *const label = gfxc.allocateFromCommandBuffer<u64>();
	const u64 startValue = 0x0, testValue = 0x1;
	*label = startValue;
	gfxc.writeAtEndOfPipe(sce::Gnm::kEopCsDone, sce::Gnm::kEventWriteDestMemory, const_cast<uint64_t*>(label), sce::Gnm::kEventWriteSource64BitsImmediate, testValue, sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
	//gfxc.writeAtEndOfShader( Gnm::kEosCsDone, const_cast<uint64_t*>(label), testValue ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
	gfxc.waitOnAddress( const_cast<uint64_t*>(label), ~0U, Gnm::kWaitCompareFuncNotEqual, startValue );
	gfxc.flushShaderCachesAndWait(Gnm::kCacheActionWriteBackAndInvalidateL1andL2, 0, Gnm::kStallCommandBufferParserDisable); // tell the CP to flush the L1$ and L2$
	// Assert( *label == testValue ); // Orbis sample crashes on that
}

// copied from samples/toolkit.cpp
void grcDevice::SynchronizeComputeToCompute()
{
	volatile u64 *const label = gfxc.allocateFromCommandBuffer<u64>();
	const u64 startValue = 0x0;
	*label = startValue;
	gfxc.writeAtEndOfShader( Gnm::kEosCsDone, const_cast<uint64_t*>(label), 0x1 ); // tell the CP to write a 1 into the memory only when all compute shaders have finished
	gfxc.waitOnAddress( const_cast<uint64_t*>(label), 0xffffffff, Gnm::kWaitCompareFuncEqual, 0x1 ); // tell the CP to wait until the memory has the val 1
	gfxc.flushShaderCachesAndWait(Gnm::kCacheActionInvalidateL1, 0, Gnm::kStallCommandBufferParserDisable); // tell the CP to flush the L1$, because presumably the consumers of compute shader output may run on different CUs
}

void grcDevice::FlushAndWait() 
{
	gfxc.flushStreamout();
}

void grcDevice::Dispatch(u32 groupX, u32 groupY, u32 groupZ)
{
#if 0	// validate?
#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	ASSERT_ONLY(sce::Gnm::ValidateState state = gfxc.validate());
#else
	ASSERT_ONLY(int state = gfxc.validate());
#endif
	Assert( state == sce::Gnm::kValidateSuccess );
#endif	//0

	if (NotifyDrawCall(-1))
		gfxc.dispatch( groupX, groupY, groupZ );
}

void grcDevice::RunComputation( const char* pDebugStr, grmShader &shader, u32 programId, u32 groupX, u32 groupY, u32 groupZ )
{
	PF_AUTO_PUSH_TIMEBAR(pDebugStr);
#if 0	// validate?
#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	ASSERT_ONLY(sce::Gnm::ValidateState state = gfxc.validate());
#else
	ASSERT_ONLY(int state = gfxc.validate());
#endif
	Assert( state == sce::Gnm::kValidateSuccess );
#endif	//0
	
	gfxc.setShaderType( Gnm::kShaderTypeCompute );
	
	grcComputeProgram *const pComputeProgram = shader.GetComputeProgram( programId );
	pComputeProgram->Bind( shader.GetInstanceData(), shader.GetInstanceData().GetBasis().GetLocalVar() );

	if (NotifyDrawCall(-1))
		gfxc.dispatch( groupX, groupY, groupZ );

	gfxc.setShaderType( Gnm::kShaderTypeGraphics );
}


void grcDevice::SetUpPriorToDraw(grcDrawMode dm)
{
	grcStateBlock::Flush();
	BindVertexStreams();
	sce::Gnm::PrimitiveType pm = s_RemapPrim[dm];
	if (grcVertexProgram::GetCurrent()->IsLsShader())
		pm = sce::Gnm::kPrimitiveTypePatch;
	gfxc.setPrimitiveType(pm);

#if TRACK_DEPTH_BOUNDS_STATE
	grcDepthStencilStateDesc DSS_desc;
	grcStateBlock::GetDepthStencilStateDesc(grcStateBlock::DSS_Active, DSS_desc);
	if(DSS_desc.DepthBoundsEnable) // assert only when state block requires enabled depth bounds: B*7117624 - FAILED: Depth bounds mismatch detected between the state block (off) and the device (on);
	{
		grcAssertf(DSS_desc.DepthBoundsEnable == s_DepthBoundsActive,
			"Depth bounds mismatch detected between the state block (%s) and the device (%s)",
			DSS_desc.DepthBoundsEnable ? "on" : "off", s_DepthBoundsActive ? "on" : "off");
	}
#endif //TRACK_DEPTH_BOUNDS_STATE
}

void grcDevice::SetWindowTitle(const char*) 
{

}


#if ENABLE_FAST_CLEAR && 0	//currently unused
static void DebugPrintRenderTarget(const char description[], const sce::Gnm::RenderTarget &rt)
{
	OUTPUT_ONLY(const sce::Gnm::DataFormat format = rt.getDataFormat();)
	Displayf( "Debug RT (%s): w=%u, h=%u, samples=%u, fragments=%u, address=0x%p, pitch=%u, tile_mode=%d"
		", cmask(fast_clear=%d, address=0x%p, size=%u), data_format(bits=%u, components=%u, surface=%u, compressed=%d)", description,
		rt.getWidth(), rt.getHeight(), rt.getNumSamples(), rt.getNumFragments(),
		rt.getBaseAddress(), rt.getPitch(), rt.getTileMode(),
		rt.getCmaskFastClearEnable(), rt.getCmaskAddress(), rt.getCmaskSliceSizeInBytes(),
		format.getBitsPerElement(), format.getNumComponents(), format.getSurfaceFormat(), format.isBlockCompressedFormat()
		);
}
#endif	//ENABLE_FAST_CLEAR

#if ENABLE_EXTRA_CLEAN_DUMP && 0
static uint32_t ComputeFmaskTotalSize(const sce::Gnm::RenderTarget &target)
{
	const uint32_t bits = sce::Gnmx::getFmaskShiftBits( target.getNumSamples(), target.getNumFragments() );
	return ((bits << target.getNumSamples()) * target.getWidth() * target.getHeight()) >> 3;
}
#endif	//ENABLE_EXTRA_CLEAN_DUMP

bool grcDevice::ClearCmask(u32 slot, const sce::Gnm::RenderTarget *const col, Color32 clearColor)
{
	if (!col || !col->getCmaskFastClearEnable())
		return false;

	// NB: Debug name must not be on the stack, since it's put into a timebar, which stores names by pointer
	const char* debugName = "";
#if !__FINAL
	switch(slot)
	{
	case 0: debugName = "MSAA color[0] CMASK clear CS"; break;
	case 1: debugName = "MSAA color[1] CMASK clear CS"; break;
	case 2: debugName = "MSAA color[2] CMASK clear CS"; break;
	case 3: debugName = "MSAA color[3] CMASK clear CS"; break;
	case 4: debugName = "MSAA color[4] CMASK clear CS"; break;
	case 5: debugName = "MSAA color[5] CMASK clear CS"; break;
	case 6: debugName = "MSAA color[6] CMASK clear CS"; break;
	case 7: debugName = "MSAA color[7] CMASK clear CS"; break;
	default: Assertf(false, "Unexpected CMASK slot, %d", slot);
	}
#endif // !__FINAL
	gfxc.setCmaskClearColor( slot, makeClearColor(col->getDataFormat(), clearColor) );
	
	ClearBuffer( debugName, col->getCmaskAddress(), col->getCmaskSliceSizeInBytes(), 0, s_AllowGPUClear );

	return true;
}

void grcDevice::Clear(bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth_,bool enableClearStencil,u32 clearStencil)
{
	float clearDepth = FixDepth(clearDepth_);

#if ENABLE_FAST_CLEAR_DEPTH
	if ((enableClearDepth || enableClearStencil) && s_CurDepth && s_CurDepth->getHtileAccelerationEnable())
	{
		const uint32_t hiDepth = static_cast<uint32_t>( clearDepth * Gnm::Htile::kMaximumZValue );
		gfxc.setStencilClearValue(clearStencil);
		gfxc.setDepthClearValue(clearDepth);

		if (s_CurDepth->getHtileStencilDisable())	//HTile has only depth info => clear stencil separately, if needed
		{
			if (enableClearDepth)
			{
				Gnm::Htile htile = {0};
				// no depth sample in tile is > or < hiDepth
#	if SCE_ORBIS_SDK_VERSION >= 0x01700000u
				htile.m_hiZ.m_maxZ = htile.m_hiZ.m_minZ = hiDepth;
#	else
				htile.HiZ.m_maxZ = htile.HiZ.m_minZ = hiDepth;
#	endif
				ClearBuffer( "HTile (depth) clear CS", s_CurDepth->getHtileAddress(), s_CurDepth->getHtileSliceSizeInBytes(), htile.m_asInt, false );
				enableClearDepth = false;
			}
			if (enableClearStencil && s_CurDepth->getStencilWriteAddress() && s_AllowFastClearStencil)
			{
				uint32_t value = (clearStencil<<24) | (clearStencil<<16) | (clearStencil<<8) | clearStencil;
				ClearBuffer( "Stencil clear CS", s_CurDepth->getStencilWriteAddress(), s_CurDepth->getStencilSliceSizeInBytes(), value, false );
				enableClearStencil = false;
			}
		}else if (enableClearDepth && enableClearStencil)	//Htile contains both depth & stencil data => have to clear both
		{
			Gnm::Htile htile = {0};
			// base Z is hiDepth
#	if SCE_ORBIS_SDK_VERSION >= 0x01700000u
			htile.m_hiZHiS.m_zRangeBase = hiDepth;
#	else
			htile.HiZHiS.m_zRangeBase = hiDepth;
#	endif
			ClearBuffer( "HTile (depth+stencil) clear CS", s_CurDepth->getHtileAddress(), s_CurDepth->getHtileSliceSizeInBytes(), htile.m_asInt, false );
			enableClearDepth = enableClearStencil = false;
		}
	}
#endif //ENABLE_FAST_CLEAR_DEPTH
#if ENABLE_FAST_CLEAR_COLOR
	for (int i=0; (enableClearColor || i) && i<grcmrtColorCount; ++i)
	{
		const sce::Gnm::RenderTarget *const rt = s_CurColors[i];
		if (!ClearCmask( i, rt, clearColor ))
			continue;
		enableClearColor = false;
		s_CmaskDirty |= 1<<i;
#if ENABLE_EXTRA_CLEAN_DUMP
		if (grcSetupInstance && grcSetupInstance->ShouldCaptureRenderTarget())
		{
			char descriptor[32];
			//gfxc.flushStreamout();
			snprintf( descriptor, sizeof(descriptor), "Color[%d] clear CS", i );
			ClearBuffer( descriptor, rt->getBaseAddress(), rt->getSliceSizeInBytes(), 0, true, false );
			snprintf( descriptor, sizeof(descriptor), "Color[%d] FMASK clear CS", i );
			//ClearBuffer( descriptor, rt->getFmaskAddress(), ComputeFmaskTotalSize(*rt), 0, true, false );
			//gfxc.flushStreamout();
			if (0)
			{
				grcRenderTargetGNM::SaveColorTarget( "Cleared", rt, DUMP_ALL );
			}
		}
#endif	//ENABLE_EXTRA_CLEAN_DUMP
	}
#endif //ENABLE_FAST_CLEAR_COLOR

	if (!enableClearColor && !enableClearDepth && !enableClearStencil)
		return;

	// If we get here, we've failed to hit one or more of the fast clear paths above so we'll have to fall back to the slow clear. Gnm doesn't provide a slow clear API so we have to use our own.
	ClearRect(0,0,sm_CurrentWindows[g_RenderThreadIndex].uWidth,sm_CurrentWindows[g_RenderThreadIndex].uHeight,enableClearColor,clearColor,enableClearDepth,clearDepth_,enableClearStencil,clearStencil);
}

void grcDevice::ClearUAV(bool bAsFloat, grcBufferUAV* pBuffer)
{
	char *pSortOffsetsDest = (char *)pBuffer->GetAddress();
	sysMemSet(pSortOffsetsDest, 0, pBuffer->GetSize() );
}

#if DEVICE_CLIP_PLANES

void grcDevice::ResolveClipPlanes()
{
	// Has the clip plane state not changed ?
	if((sm_ClipPlanesChanged[g_RenderThreadIndex] == 0) && (sm_PreviousClipPlaneEnable[g_RenderThreadIndex] == sm_ClipPlaneEnable[g_RenderThreadIndex]))
	{
		return;
	}

	grcAssertf(sm_pClipPlanesConstBuffer[g_RenderThreadIndex] != NULL, "grcDevice::ResolveClipPlanes()...We require a clip plane constant buffer!");

	Mat44V transpose, inverseTranspose;
	u32 SwitchedOn = (sm_ClipPlaneEnable[g_RenderThreadIndex] ^ sm_PreviousClipPlaneEnable[g_RenderThreadIndex]) & sm_ClipPlaneEnable[g_RenderThreadIndex];
	u32 SwitchedOff = (sm_ClipPlaneEnable[g_RenderThreadIndex] ^ sm_PreviousClipPlaneEnable[g_RenderThreadIndex]) & ~sm_ClipPlaneEnable[g_RenderThreadIndex];
	u32 WrittenToAndOn = (sm_ClipPlaneEnable[g_RenderThreadIndex] & sm_ClipPlanesChanged[g_RenderThreadIndex]);
	u32 PlanesToRecompute = SwitchedOn | WrittenToAndOn;

	if(PlanesToRecompute)
	{
		Mat44V WorldSpaceToClippingSpace;
		// Clip plane distance calculations are performed on points in clipping space as the last stage of vertex shaders.
		Multiply(WorldSpaceToClippingSpace, grcViewport::GetCurrent()->GetProjection(), grcViewport::GetCurrent()->GetViewMtx());
		// Compute the matrix to transform the plane by (better understood by thinking of it as "the matrix which transforms points into plane space" transposed).
		Transpose(transpose, WorldSpaceToClippingSpace);
		InvertFull(inverseTranspose,transpose);
	}

	u32 i;
	u32 Bit = 0x1;
	grcClipPlanes *pDest = (grcClipPlanes *)sm_pClipPlanesConstBuffer[g_RenderThreadIndex]->GetDataPtr();

	for(i=0; i<RAGE_MAX_CLIPPLANES; i++, Bit<<=1)
	{
		if(PlanesToRecompute & Bit)
		{
			pDest->ClipPlanes[i] = Multiply(inverseTranspose, sm_ClipPlanes[i][g_RenderThreadIndex]);
		}
		else if(SwitchedOff & Bit)
		{
			pDest->ClipPlanes[i] = Vec4V(V_ZERO);
		}
	}

	sm_pClipPlanesConstBuffer[g_RenderThreadIndex]->Unlock();

	sm_PreviousClipPlaneEnable[g_RenderThreadIndex] = sm_ClipPlaneEnable[g_RenderThreadIndex];
	sm_ClipPlanesChanged[g_RenderThreadIndex] = 0x0;
}


void grcDevice::ResetClipPlanes()
{
	sm_ClipPlaneEnable[g_RenderThreadIndex] = 0;
	sm_PreviousClipPlaneEnable[g_RenderThreadIndex] = 0;
	sm_ClipPlanesChanged[g_RenderThreadIndex] = 0;

	if(sm_pClipPlanesConstBuffer[g_RenderThreadIndex])
	{
		u32 i;
		grcClipPlanes *pDest = (grcClipPlanes *)sm_pClipPlanesConstBuffer[g_RenderThreadIndex]->GetDataPtr();
	
		for(i=0; i<RAGE_MAX_CLIPPLANES; i++)
		{
			pDest->ClipPlanes[i] = Vec4V(V_ZERO);
		}

		sm_pClipPlanesConstBuffer[g_RenderThreadIndex]->Unlock();
	}
}


void grcDevice::SetClipPlanesConstBuffer(grcCBuffer *pConstBuffer)
{
	for (long index = 0; index < NUMBER_OF_RENDER_THREADS; index++)
		sm_pClipPlanesConstBuffer[index] = pConstBuffer;
}


u32 grcDevice::SetClipPlaneEnable(u32 enable) 
{
	u32 Ret = sm_ClipPlaneEnable[g_RenderThreadIndex];
	sm_ClipPlaneEnable[g_RenderThreadIndex] = enable;
	return Ret;
}

void grcDevice::SetClipPlane(int index,Vec4V_In plane) 
{
	Assert(index >= 0 && index < RAGE_MAX_CLIPPLANES);
	sm_ClipPlanes[index][g_RenderThreadIndex] = plane;
	sm_ClipPlanesChanged[g_RenderThreadIndex] |= 0x1 << index;
}

#if !HACK_GTA4 // no shader clip planes
void grcDevice::GetClipPlane(int index,Vec4V_InOut plane) 
{
	Assert(index >= 0 && index < RAGE_MAX_CLIPPLANES);
	plane = sm_ClipPlanes[index][g_RenderThreadIndex];
}
#endif //!HACK_GTA4 

#endif	//DEVICE_CLIP_PLANES


void grcDevice::SetLetterBox(bool /*enable*/) { AssertMsg(0, "Not Implemented Yet"); }


grcOcclusionQuery grcDevice::CreateOcclusionQuery(int /*tileCount*/) 
{  
	SYS_CS_SYNC(s_GPUQueryCs);
	sce::Gnm::OcclusionQueryResults *hQueryHandle = g_GPUQueries->New();
	Assert(hQueryHandle != NULL);
	if (hQueryHandle)
		hQueryHandle->reset();
	return (grcOcclusionQuery) hQueryHandle;
}

void grcDevice::BeginOcclusionQuery(grcOcclusionQuery query) 
{ 
	FatalAssertMsg(query != NULL, "BeginOcclusionQuery input can not be NULL"); 

	// Clear out the struct on the CPU, so an immediate call to
	// GetOcclusionQueryData() will return false.
	query->reset();

	// Set one of the counters to nonzero (but without the high bit set) so we know a query is in progress.
	query->m_results[0].m_zPassCountBegin = query->m_results[0].m_zPassCountEnd = 1;

#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
	gfxc.writeOcclusionQuery(Gnm::kOcclusionQueryOpBeginWithoutClear, query);
#else
	gfxc.writeOcclusionQuery(Gnm::kOcclusionQueryOpBegin, query);
#endif
}

void grcDevice::EndOcclusionQuery(grcOcclusionQuery query) 
{ 
	FatalAssertMsg(query != NULL, "EndOcclusionQuery input can not be NULL"); 
	gfxc.writeOcclusionQuery(Gnm::kOcclusionQueryOpEnd, query);
}

void grcDevice::DestroyOcclusionQuery(grcOcclusionQuery& query) 
{ 
	AssertMsg(query != NULL, "DestroyOcclusionQuery input can not be NULL"); 
	if (query != NULL) {
		for (;;) {
			// The fields must be all zero (query never used) or all the high bits must be set.
			bool allZero = true;
			for (int i=0; i<8; i++)
				if (query->m_results[i].m_zPassCountBegin || query->m_results[i].m_zPassCountEnd)
					allZero = false;
			if (allZero || query->isReady())
				break;
			grcWarningf("Attempting to delete an occlusion query still in flight");
			sysIpcSleep(10);
		}
		SYS_CS_SYNC(s_GPUQueryCs);
		g_GPUQueries->Delete(query);
		query = NULL;
	}
}

bool grcDevice::GetOcclusionQueryData(grcOcclusionQuery query, u32& numDrawn) 
{ 
	AssertMsg(query != NULL, "EndOcclusionQuery input can not be NULL"); 
	if ((query == NULL) || (!query->isReady()))
	{
		numDrawn = 0x0;
		return false;
	}
	
	numDrawn = query->getZPassCount();
	return true;
}

grcConditionalQuery grcDevice::CreateConditionalQuery() 
{  
	return (grcConditionalQuery)CreateOcclusionQuery(0); //return (grcConditionalQuery)CreateOcclusionQuery();
}

void grcDevice::BeginConditionalQuery(grcConditionalQuery query)
{
	// This is not the same as BeginOcclusionQuery, since we do not want to mess
	// with the query contents on the CPU.  writeOcclusionQuery(OpBegin) will
	// create an IT_DMA_DATA command to zero the struct on the GPU.
	FatalAssertMsg(query != NULL, "BeginConditionalQuery input can not be NULL"); 
#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
	gfxc.writeOcclusionQuery(Gnm::kOcclusionQueryOpClearAndBegin, query);
#else
	gfxc.writeOcclusionQuery(Gnm::kOcclusionQueryOpBegin, query);
#endif
}

void grcDevice::EndConditionalQuery(grcConditionalQuery query) 
{ 
	EndOcclusionQuery((grcOcclusionQuery)query);
}

void grcDevice::DestroyConditionalQuery(grcConditionalQuery& query) 
{ 
	DestroyOcclusionQuery((grcOcclusionQuery&)query);
}

void grcDevice::BeginConditionalRender(grcConditionalQuery query)
{ 
	gfxc.setZPassPredicationEnable(query, Gnm::kPredicationZPassHintWait, Gnm::kPredicationZPassActionDrawIfVisible);
}

void grcDevice::EndConditionalRender(grcConditionalQuery UNUSED_PARAM(query)) 
{ 
	gfxc.setZPassPredicationDisable();
}


void grcDevice::CopyStructureCount( grcBufferBasic* pDestBuffer, u32 DstAlignedByteOffset, grcBufferUAV* pSrcBuffer )
{
	Assert( DstAlignedByteOffset + 4 <= pDestBuffer->GetSize() );
	Assert( (pSrcBuffer->m_GdsCounterOffset & 3) == 0 );
	gfxc.readAppendConsumeCounters( (char*)pDestBuffer->GetAddress() + DstAlignedByteOffset, pSrcBuffer->m_GdsCounterOffset, 0, 1 );
}

void grcDevice::DrawWithGeometryShader( grcBufferBasic* pIndirectBuffer )
{
	grcStateBlock::Flush();

	SetVertexDeclaration( eqaa::GetVertexDeclaration() );	//ORBIS FIXME
	SetUpPriorToDraw( drawPoints );

	// Draw the bokeh points
	if (NotifyDrawCall(0))
	{
		Assert( pIndirectBuffer->GetSize() >= 16 );
		if (1)
		{
			gfxc.setBaseIndirectArgs( pIndirectBuffer->GetAddress() );
			gfxc.stallCommandBufferParser();	//used in drawindirect-sample
			VERTEX_DEBUG_CALL( gfxc.drawIndirect(0), 0 );
		}else
		{	// that code paths screws up bound shader, but the numVerts is correct
			CpuWaitOnGpuIdle();
			const uint32_t numVerts = reinterpret_cast<int*>(pIndirectBuffer->GetAddress())[3];
			VERTEX_DEBUG_CALL( gfxc.drawIndexAuto(numVerts), 0 );
		}
	}

#if DEBUG_SEALING_OF_DRAWLISTS
	grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
}

void grcDevice::DrawIndexedInstancedIndirect(grcBufferBasic* pIndirectBuffer, grcDrawMode dm, bool alreadySetupPriorToDraw, u32 argsOffsetInBytes)
{
	if(pIndirectBuffer)
	{
		Assert(pIndirectBuffer->GetSize() >= sizeof(sce::Gnm::DrawIndexIndirectArgs));
		DrawIndexedInstancedIndirect(pIndirectBuffer->GetAddress(), dm, alreadySetupPriorToDraw, 0);
	}
}

void grcDevice::DrawIndexedInstancedIndirect(void *pIndirectBufferMem, grcDrawMode dm, bool alreadySetupPriorToDraw, u32 argsOffsetInBytes)
{
	if(pIndirectBufferMem)
	{
		void *indirectArgs = reinterpret_cast<void *>(reinterpret_cast<char *>(pIndirectBufferMem) + argsOffsetInBytes);

		if( !alreadySetupPriorToDraw )
			SetUpPriorToDraw(dm);

		gfxc.setIndexBuffer(s_IndexBuffer[g_RenderThreadIndex]);

		// sce::Gnm::DrawCommandBuffer::setIndexCount generates an
		// INDEX_BUFFER_SIZE PM4 packet.  According to
		// si_programming_guide_v2.pdf, this packet sets VGT_DMA_MAX_SIZE.  So
		// this doesn't actually specify how many indices to render (that is in
		// the indirect args buffer), instead it clamps the index values.  So we
		// can just set it to max to disable all clamping, and rely on the
		// caller setting a valid number of indices in the indirect args.
		gfxc.setIndexCount(0xffffffff);

		if (NotifyDrawCall(0))
		{
			gfxc.setBaseIndirectArgs(indirectArgs);
			gfxc.stallCommandBufferParser();	//used in drawindirect-sample
			VERTEX_DEBUG_CALL(gfxc.drawIndexIndirect(argsOffsetInBytes), 0);
		}

		//Must remember to set instances back to 1.
		gfxc.setNumInstances(1);

#if DEBUG_SEALING_OF_DRAWLISTS
		grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
	}
}

void grcDevice::ClearRect(int x, int y,int width,int height,float depth,const Color32 &color) 
{ 
	ClearRect(x,y,width,height,true, color, true, depth, true, 0);
}

void grcDevice::ClearRect(int x,int y,int width,int height,bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth_,bool enableClearStencil,u32 clearStencil ORBIS_ONLY(,bool yFlipped)) 
{ 
	float clearDepth = FixDepth(clearDepth_);

	GRC_ALLOC_SCOPE_AUTO_PUSH_POP();

	using namespace grcStateBlock;
	PIXBeginN(0,"ClearRect(%d,%d,%d,%d,%08x(%s),%.1f(%s),%d(%s))",x,y,width,height,clearColor.GetDeviceColor(),enableClearColor?"on":"off",clearDepth,enableClearDepth?"on":"off",clearStencil,enableClearStencil?"on":"off");
	grcBlendStateHandle BS_prev = BS_Active;
	grcDepthStencilStateHandle DSS_prev = DSS_Active;
	
	const bool useDbControl = s_UseClearDbControl && (enableClearDepth || enableClearStencil);
	if (useDbControl)
	{
		Gnm::DbRenderControl dbRenderControl;
		dbRenderControl.init();
		if (enableClearDepth)
		{
			dbRenderControl.setDepthClearEnable(true);
			gfxc.setDepthClearValue(clearDepth);
		}
		if (enableClearStencil)
		{
			dbRenderControl.setStencilClearEnable(true);
			gfxc.setStencilClearValue(clearStencil);
		}
		gfxc.setDbRenderControl(dbRenderControl);
	}
	
	SetBlendState(clearStates_BS[enableClearColor]);
	SetDepthStencilState(clearStates_DSS[enableClearDepth][enableClearStencil],clearStencil);

	float fLeft = ((2 * x) / (float)sm_CurrentWindows[g_RenderThreadIndex].uWidth) - 1.0f;
	float fRight = ((2 * (x + width)) / (float)sm_CurrentWindows[g_RenderThreadIndex].uWidth) - 1.0f;

	float fTop = ((2 * y) / (float)sm_CurrentWindows[g_RenderThreadIndex].uHeight) - 1.0f;
	float fBottom = ((2 * (y + height)) / (float)sm_CurrentWindows[g_RenderThreadIndex].uHeight) - 1.0f;
	if (yFlipped)
	{
		fTop = -fTop;
		fBottom = -fBottom;
	}

	const sce::Gnm::RenderTarget *const pGnmRT = s_CurColors[0];
	const int numArraySlices = pGnmRT == NULL ? 0 : (pGnmRT->getBaseArraySliceIndex() - pGnmRT->getLastArraySliceIndex()) + 1;
	g_DefaultEffect->SetVar(g_DefaultColor, clearColor);
	
	if (numArraySlices > 1)
	{
		// slow path: uses GS to clear all slices of the array
		g_DefaultEffect->Bind(s_DefaultClearArray);
		grcBegin(drawTris,6);
		grcVertex3f(fLeft, fTop, clearDepth_);
		grcVertex3f(fRight, fTop, clearDepth_);
		grcVertex3f(fLeft, fBottom, clearDepth_);
		grcVertex3f(fRight, fBottom, clearDepth_);
		grcVertex3f(fLeft, fBottom, clearDepth_);
		grcVertex3f(fRight, fTop, clearDepth_);
	}
	else
	{
		// faster path: just clears 1 array slice
		if( s_CurColors[3] )
			g_DefaultEffect->Bind(s_DefaultClearMRT4);
		else if( s_CurColors[2] )
			g_DefaultEffect->Bind(s_DefaultClearMRT3);
		else if( s_CurColors[1] )
			g_DefaultEffect->Bind(s_DefaultClearMRT2);
		else if ( s_CurColors[0] && s_CurColors[0]->getDataFormat().getNumComponents() == 1 && s_CurColors[0]->getDataFormat().getBitsPerElement() == 32 )
			g_DefaultEffect->Bind(s_DefaultClearR32);
		else
			g_DefaultEffect->Bind(s_DefaultClear);

		grcBegin(drawRects,3);
		grcVertex3f(fLeft, fTop, clearDepth_);
		grcVertex3f(fRight, fTop, clearDepth_);
		grcVertex3f(fLeft, fBottom, clearDepth_);
	}
	grcEnd();
	g_DefaultEffect->UnBind();

	if (useDbControl) {
		Gnm::DbRenderControl dbRenderControl;
		dbRenderControl.init();
		gfxc.setDbRenderControl(dbRenderControl);
	}

	SetDepthStencilState(DSS_prev);
	SetBlendState(BS_prev);
	PIXEnd();
}

void grcDevice::SetProgramResources()
{
	// used for multi draw interface particle effect
	grcVertexProgram::GetCurrent()->Bind(s_CurrentDecl);

	// TODO : bind other stages if needed
}

void grcDevice::SetFrameLock(int frameLock,bool /*swapImmediateIfLate*/) 
{
#if !__FINAL
	PARAM_frameLimit.Get(frameLock);
#endif
	sm_GlobalWindow.uFrameLock = frameLock;
	sceVideoOutSetFlipRate(s_videoInfo.handle, sm_GlobalWindow.uFrameLock? sm_GlobalWindow.uFrameLock-1 : 0);
}

grcImage* grcDevice::CaptureScreenShot(grcImage* UNUSED_PARAM(image))
{
	// TODO: Implement
	Assertf(false, "CaptureScreenShot not implemented for this platform.");
	return NULL;
}

bool grcDevice::CaptureScreenShotToJpegFile(const char* outFile)
{
	const grcRenderTargetGNM *front = static_cast<const grcRenderTargetGNM*>(grcTextureFactory::GetInstance().GetFrontBuffer(true));
	grcAssertf(front,"no front buffer");
	
	const sce::Gnm::Texture *gnmTexture = reinterpret_cast<const sce::Gnm::Texture*>(front->GetTexturePtr());

	int width = gnmTexture->getWidth();
	int height = gnmTexture->getHeight();
	int pitch = gnmTexture->getPitch();

	OUTPUT_ONLY(sce::Gnm::DataFormat format = gnmTexture->getDataFormat();)
	Displayf("%p", &format);

	OUTPUT_ONLY(Gnm::TileMode tileMode = gnmTexture->getTileMode();)
	Displayf("%d", tileMode);

	uint64_t offset;
	uint64_t size;

#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
	sce::GpuAddress::computeSurfaceOffsetAndSize(
#else
	sce::GpuAddress::computeTextureSurfaceOffsetAndSize(
#endif
		&offset,&size,gnmTexture,0,0);

	sce::GpuAddress::TilingParameters tp;
	tp.initFromTexture(gnmTexture, 0, 0);

	u8 *const buffer = rage_new u8[size];
#if __DEV
	memset(buffer,0,size);
#endif

	sce::GpuAddress::detileSurface(buffer, ((char*)gnmTexture->getBaseAddress() + offset),&tp);

	pitch = width;	//detileSurface takes care of it, fixes B#1720244

	grcImage::SaveStreamToJPEG(outFile, buffer, width, height, pitch * 4, g_JpegSaveQuality);

	delete [] buffer;
	return true;
}

bool grcDevice::CaptureScreenShotToFile(const char *,float)
{
	return false;
}

void grcDevice::GetSafeZone(int &x0, int &y0, int &x1, int &y1)
{
	x0 = 0;
	y0 = 0;
	x1 = sm_CurrentWindows[g_RenderThreadIndex].uWidth-1;
	y1 = sm_CurrentWindows[g_RenderThreadIndex].uHeight-1;
}

bool grcDevice::GetWideScreen()
{
	return true;
}

grcDevice::Result grcDevice::SetVertexDeclaration(const grcVertexDeclaration *decl)
{
	s_CurrentDecl = decl;
	return 0;	// S_OK
}

#if WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES
// IF YOU CHANGE ANY OF THESE FORMATS, you must update vertexDwords3s below:
static const sce::Gnm::DataFormat dataFormats[] = {
	sce::Gnm::kDataFormatR16Float,				// grcdsHalf,
	sce::Gnm::kDataFormatR16G16Float,			// grcdsHalf2,
	{{{sce::Gnm::kSurfaceFormat16_16_16_16, sce::Gnm::kTextureChannelTypeFloat, sce::Gnm::kTextureChannelX,  sce::Gnm::kTextureChannelY,  sce::Gnm::kTextureChannelZ,  sce::Gnm::kTextureChannelConstant1 }}},		// grcdsHalf3,
	sce::Gnm::kDataFormatR16G16B16A16Float,		// grcdsHalf4,

	sce::Gnm::kDataFormatR32Float,				// grcdsFloat,
	sce::Gnm::kDataFormatR32G32Float,			// grcdsFloat2,
	sce::Gnm::kDataFormatR32G32B32Float,		// grcdsFloat3,
	sce::Gnm::kDataFormatR32G32B32A32Float,		// grcdsFloat4,

	sce::Gnm::kDataFormatR8G8B8A8Uint,			// grcdsUBYTE4,
	sce::Gnm::kDataFormatR8G8B8A8Unorm,			// grcdsColor,
	sce::Gnm::kDataFormatR8G8B8A8Snorm,			// grcdsPackedNormal, -- why didn't kDataFormatR11G11B10Float work?

	sce::Gnm::kDataFormatR16Unorm,				// grcdsShort_unorm,
	sce::Gnm::kDataFormatR16G16Unorm,			// grcdsShort2_unorm,
	sce::Gnm::kDataFormatR8G8Unorm,				// grcdsByte2_unorm,

	{{{sce::Gnm::kSurfaceFormat16_16,       sce::Gnm::kTextureChannelTypeSInt,  sce::Gnm::kTextureChannelX,  sce::Gnm::kTextureChannelY,  sce::Gnm::kTextureChannelConstant0, sce::Gnm::kTextureChannelConstant1}}},
	sce::Gnm::kDataFormatR16G16B16A16Sint,		// grcdsShort4,
};
#endif

// The GPU hardware isn't going to change at this point, so it's safe to precompute these
static const uint32_t vertexDword3s[] = {
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(FLOAT, 16,          R,0,0,1),  // 0x20017204 half
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(FLOAT, 16_16,       R,G,0,1),  // 0x2002f22c half2
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(FLOAT, 16_16_16_16, R,G,B,1),  // 0x200673ac half3
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(FLOAT, 16_16_16_16, R,G,B,A),  // 0x20067fac half4
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(FLOAT, 32,          R,0,0,1),  // 0x20027204 float
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(FLOAT, 32_32,       R,G,0,1),  // 0x2005f22c float2
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(FLOAT, 32_32_32,    R,G,B,1),  // 0x2006f3ac float3
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(FLOAT, 32_32_32_32, R,G,B,A),  // 0x20077fac float4
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(UINT,  8_8_8_8,     R,G,B,A),  // 0x20054fac UBYTE4
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(UNORM, 8_8_8_8,     R,G,B,A),  // 0x20050f2e Color
// 	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(SNORM, 10_11_11,    R,G,B,1),  // 0x200313ac PackedNormal
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(SNORM, 8_8_8_8,     R,G,B,A),  // 0x20051fac PackedNormal

	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(UNORM,  16,			R,0,0,1),  // 0x???????? short_unorm
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(UNORM,  16_16,		R,G,0,1),  // 0x???????? short2_unorm
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(UNORM,  8_8,			R,G,0,1),  // 0x???????? byte2_unorm

	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(SINT,  16_16,       R,G,0,1),  // 0x2002d22c short2
	AMDSISLANDS_VERTEX_BUFFER_RESOURCE_REMAP_U32_3(SINT,  16_16_16_16, R,G,B,A),  // 0x20065fac short4
};
CompileTimeAssert(NELEM(vertexDword3s) == grcFvf::grcdsCount);

grcVertexDeclaration* grcDevice::CreateVertexDeclaration(const grcVertexElement *elements,int count,int stride0Override)
{
	grcVertexDeclaration *decl = rage_new grcVertexDeclaration;
	u32 offsets[MAX_VERTEX_STREAMS] = { 0 };

	static u8 remapToFvf[] = { 
		grcFvf::grcfcPosition, 
		grcFvf::grcfcPosition,
		grcFvf::grcfcNormal,
		grcFvf::grcfcBinormal0,
		grcFvf::grcfcTangent0,
		grcFvf::grcfcTexture0,
		grcFvf::grcfcWeight,
		grcFvf::grcfcBinding,
		grcFvf::grcfcDiffuse
	};
	/* static const char *fstrings[] = { "half", "half2", "half3", "half4", "float", "float2", "float3", "float4", "UBYTE4", "Color", "PackedNormal", "EDGE0", "EDGE1", "EDGE2", "short2", "short4" };
	for (int i=0; i<=15; i++) {
		sce::Gnm::Buffer tmp;
		tmp.initAsVertexBuffer(0,dataFormats[i],16,1);
		// Displayf("Format %s VertexWord3 = %08x",fstrings[i],tmp.m_regs[3]);
		Displayf("\t0x%08x,",tmp.m_regs[3]);
	} */

	Assert(count <= 15);
	// Set up dummy slot for all missing data.
	WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(decl->DataFormats[15] = dataFormats[grcFvf::grcdsColor];)
	decl->VertexDword3[15] = vertexDword3s[grcFvf::grcdsColor];
	decl->Offsets[15] = 0;
	decl->Semantics[15] = grcFvf::grcfcDiffuse;
	decl->Streams[15] = MAX_VERTEX_STREAMS;		// special magic stream!
	decl->Dividers[15] = 0;

	for (int i=0; i<count; i++) {
		const grcVertexElement &e = elements[i];
		WRAPPER_GNM_VALIDATE_BUFFER_RESOURCES_ONLY(decl->DataFormats[i] = dataFormats[e.format];)
		decl->VertexDword3[i] = vertexDword3s[e.format];
		decl->Offsets[i] = offsets[e.stream];
		decl->Semantics[i] = remapToFvf[e.type] + e.channel;
		decl->Streams[i] = e.stream;
		Assert( e.streamFrequencyDivider == 0 || e.streamFrequencyMode == grcFvf::grcsfmIsInstanceStream );
		decl->Dividers[i] = e.streamFrequencyDivider;
		offsets[e.stream] += e.size;
	}
	decl->Stream0Size = stride0Override? stride0Override : offsets[0];
	decl->ElementCount = count;
	decl->RefCount = 1;
	return decl;
}

int grcVertexDeclaration::Release() const
{
	Assert(RefCount > 0 && RefCount < 10000);
	if (--RefCount == 0) {
		delete[] (char*) this;
		return 0;
	}
	else
		return RefCount;
}


void grcDevice::DestroyVertexDeclaration(grcVertexDeclaration *decl) 
{
	decl->Release();
}

#if GRCDBG_IMPLEMENTED
void grcDevice::PushFaultContext(const char *label)
{
	grcGfxContext::PushFaultContext(label);
}

void grcDevice::PopFaultContext()
{
	grcGfxContext::PopFaultContext();
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

grcFenceHandle grcDevice::AllocFence(u32 flags)
{
	const u32 init = (flags & ALLOC_FENCE_INIT_AS_PENDING) ? __grcFenceHandle::STATE_PENDING : __grcFenceHandle::STATE_DONE;
	if (flags & ALLOC_FENCE_ALLOC_SCOPE_LIFETIME)
	{
		CompileTimeAssert(offsetof(__grcFenceHandle, state) == 0);
		CompileTimeAssert(sizeof(((__grcFenceHandle*)0)->state) == 4);
		u32 *const fence = (u32*)g_grcCurrentContext->allocateFromCommandBufferAlignBytes(4, 8);
		*fence = init;
		return (grcFenceHandle) fence;
	}
	else
	{
		grcFenceHandle fence = s_FencePool.Alloc();
		fence->state = init;
		return fence;
	}
}

void grcDevice::CpuMarkFencePending(grcFenceHandle fence)
{
	fence->state = __grcFenceHandle::STATE_PENDING;
}

void grcDevice::CpuMarkFenceDone(grcFenceHandle fence)
{
	// CpuMarkFenceDone can be used to unblock the GPU that is waiting on the CPU
	// to write some data.  So use a write barrier to ensure the GPU sees writes
	// in the correct order.
	sysMemWriteBarrier();

	fence->state = __grcFenceHandle::STATE_DONE;
}

void grcDevice::GpuMarkFencePending(grcFenceHandle fence)
{
	static const ALIGNAS(8) u32 state = __grcFenceHandle::STATE_PENDING;
	g_grcCurrentContext->writeDataInline(const_cast<u32*>(&(fence->state)), &state, sizeof(state), sce::Gnm::kWriteDataConfirmEnable);
}

void grcDevice::GpuMarkFenceDone(grcFenceHandle fence, u32 flags)
{
	const sce::Gnm::EndOfPipeEventType etype = (flags & GPU_WRITE_FENCE_AFTER_SHADER_READS)
		? sce::Gnm::kEopCbDbReadsDone : sce::Gnm::kEopFlushCbDbCaches;
	g_grcCurrentContext->writeAtEndOfPipe(etype,
		sce::Gnm::kEventWriteDestMemory, const_cast<u32*>(&(fence->state)),
		sce::Gnm::kEventWriteSource32BitsImmediate, __grcFenceHandle::STATE_DONE,
		sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
}

bool grcDevice::IsFencePending(grcFenceHandle fence, u32 flags)
{
	if (!(flags & IS_FENCE_PENDING_HINT_NO_KICK))
	{
		// TODO: Going to need to revive the code for not kicking contexts that are "empty".
		if (g_grcCurrentContext)
		{
			grcGfxContext::kickContextIfPartialSubmitsAllowed();
		}
	}

	return fence->state == (u32)__grcFenceHandle::STATE_PENDING;
}

void grcDevice::CpuWaitOnFence(grcFenceHandle fence)
{
#	if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
		CustomHangDetection watchdog("grcDevice::CpuWaitOnFence", GRCGFXCONTEXT_GNM_GPU_HANG_TIMEOUT_SEC);
#	endif

	while (Unlikely(IsFencePending(fence)))
	{
		sysIpcSleep(1);

#		if GRCGFXCONTEXT_GNM_BASIC_ERROR_HANDLING
			if (watchdog.pollHasTimedOut())
			{
				grcGfxContext::reportGpuHang();
			}
#		endif
	}
}

void grcDevice::CpuFreeFence(grcFenceHandle fence)
{
	s_FencePool.ImmediateFree(fence);
}

void grcDevice::GpuWaitOnPreviousWrites()
{
	GRC_ALLOC_SCOPE_AUTO_PUSH_POP();
	GpuWaitOnFence(AllocFenceAndGpuWrite(grcDevice::ALLOC_FENCE_ALLOC_SCOPE_LIFETIME));
}

void grcDevice::GpuWaitOnFence(grcFenceHandle fence, u32 inputs)
{
	// waitOnAddressAndStallCommandBufferParser can only use the compare
	// function kWaitCompareFuncGreaterEqual (not kWaitCompareEqual),
	// so compile time assert that values are orderred correctly.
	CompileTimeAssert(__grcFenceHandle::STATE_DONE > __grcFenceHandle::STATE_PENDING);

	grcContextHandle *const ctx = g_grcCurrentContext;
	if (inputs & GPU_WAIT_ON_FENCE_INPUT_INDEX_BUFFER)
	{
		// Indices are fetched by the CPG ("Command Processor Graphics"), so
		// that needs to be stalled as well.
		ctx->m_dcb.waitOnAddressAndStallCommandBufferParser(const_cast<u32*>(&(fence->state)), ~0, __grcFenceHandle::STATE_DONE);
	}
	else
	{
		ctx->m_dcb.waitOnAddress(const_cast<u32*>(&(fence->state)), ~0, sce::Gnm::kWaitCompareFuncEqual, __grcFenceHandle::STATE_DONE);
	}
}

void grcDevice::GpuFreeFence(grcFenceHandle fence)
{
	g_grcCurrentContext->writeAtEndOfPipe(sce::Gnm::kEopCbDbReadsDone,
		sce::Gnm::kEventWriteDestMemory, const_cast<u32*>(&(fence->state)),
		sce::Gnm::kEventWriteSource32BitsImmediate, __grcFenceHandle::STATE_FREE,
		sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
	s_FencePool.PendingFree(fence);
}

// This is an experimentally found value.  Initially tried setting to the
// total number of SIMDs (which is 72 = 2xSEs*9xCUs*4xSIMDs on Orbis), but
// that was way too few wavefronts.
static bank_u32 MEMORYCS_MAX_WAVEFRONTS = 0x10000;

void grcDevice::ShaderMemcpy(void *dst, const void *src, size_t size, u32 maxWavefronts)
{
	grcAssert(((uptr)dst&3) == 0);
	grcAssert(((uptr)src&3) == 0);
	grcAssert((size&15) == 0);
	grcAssert(size < 0x100000000uLL);

	GRC_ALLOC_SCOPE_AUTO_PUSH_POP();

	maxWavefronts = Min(maxWavefronts, MEMORYCS_MAX_WAVEFRONTS);

	grcContextHandle *const ctx = g_grcCurrentContext;

	ctx->setShaderType(sce::Gnm::kShaderTypeCompute);

	s_MemoryCS->Bind((grcEffectTechnique)CS_GpuCopy512);

	auto *const cbufData = ctx->allocateFromCommandBuffer<GpuCopy512_cbuf>();
	const u32 numDqwords    = (u32)(size>>6);
	const u32 numWavefronts = Min((numDqwords+63)>>6, maxWavefronts);
	const u32 numThreads    = Min(numDqwords, numWavefronts<<6);
	cbufData->Size          = size;
	cbufData->Stride        = numWavefronts<<12;
	sce::Gnm::Buffer cbuf;
	grcGfxContext::initAsConstantBuffer(cbuf, cbufData, sizeof(*cbufData));
	ctx->setConstantBuffers(sce::Gnm::kShaderStageCs, MEMORYCS_CBUF_ARGS_SLOT, 1, &cbuf);

	sce::Gnm::Buffer sbuf, dbuf;
	sbuf.initAsByteBuffer(const_cast<void*>(src), size);
	dbuf.initAsByteBuffer(dst, size);
	sbuf.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeRO, sce::Gnm::kL1CachePolicyBypass, sce::Gnm::kKCachePolicyBypass);
	dbuf.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC, sce::Gnm::kL1CachePolicyBypass, sce::Gnm::kKCachePolicyBypass);
	ctx->setBuffers  (sce::Gnm::kShaderStageCs, MEMORYCS_SRC_BUF_SLOT, 1, &sbuf);
	ctx->setRwBuffers(sce::Gnm::kShaderStageCs, MEMORYCS_DST_BUF_SLOT, 1, &dbuf);

	ctx->dispatch(numThreads, 1, 1);

	s_MemoryCS->UnBind();

	gfxc.setShaderType(sce::Gnm::kShaderTypeGraphics);
}

void grcDevice::ShaderMemset32(void *dst, u32 value, size_t size, u32 maxWavefronts)
{
	grcAssert(((uptr)dst&3) == 0);
	grcAssert((size&63) == 0);

	GRC_ALLOC_SCOPE_AUTO_PUSH_POP();

	maxWavefronts = Min<size_t>(maxWavefronts, MEMORYCS_MAX_WAVEFRONTS);

	grcContextHandle *const ctx = g_grcCurrentContext;

	ctx->setShaderType(sce::Gnm::kShaderTypeCompute);

	s_MemoryCS->Bind((grcEffectTechnique)CS_GpuFill512);

	auto *const cbufData = ctx->allocateFromCommandBuffer<GpuFill512_cbuf>();
	const u32 numThreadsWorthOfData = (u32)(size>>6);
	const u32 numWavefronts         = Min((numThreadsWorthOfData+63)>>6, maxWavefronts);
	const u32 numThreads            = Min(numThreadsWorthOfData, numWavefronts<<6);
	cbufData->Size                  = size;
	cbufData->Stride                = numWavefronts<<12;
	cbufData->Value                 = value;
	sce::Gnm::Buffer cbuf;
	grcGfxContext::initAsConstantBuffer(cbuf, cbufData, sizeof(*cbufData));
	ctx->setConstantBuffers(sce::Gnm::kShaderStageCs, MEMORYCS_CBUF_ARGS_SLOT, 1, &cbuf);

	sce::Gnm::Buffer dbuf;
	dbuf.initAsByteBuffer(dst, size);
	dbuf.setResourceMemoryType(sce::Gnm::kResourceMemoryTypeGC, sce::Gnm::kL1CachePolicyBypass, sce::Gnm::kKCachePolicyBypass);
	ctx->setRwBuffers(sce::Gnm::kShaderStageCs, MEMORYCS_DST_BUF_SLOT, 1, &dbuf);

	ctx->dispatch(numThreads, 1, 1);

	s_MemoryCS->UnBind();

	ctx->setShaderType(sce::Gnm::kShaderTypeGraphics);
}

bool grcDevice::GetHiDef()
{
	return true;
}

u32 grcDevice::GetAndIncContextSequenceNumber()
{
	static u32 s_contextSequenceNumber;
	return ++s_contextSequenceNumber;
}

void grcDevice::CreateContextForThread()
{
}

void grcDevice::DestroyContextForThread()
{
	// TODO: Free up all the private buffers
	// delete g_grcCurrentContext;
	g_grcCurrentContext = NULL;
}

void grcDevice::BeginContext(grcGfxContextControl *control)
{
	Assert(!g_grcCurrentContext);
	g_grcCurrentContext = grcGfxContext::openContext(true, control->getSequenceNumber(), control);
}

grcContextCommandList *grcDevice::EndContext()
{
	grcGfxContext::closeContext(false);
	// Make sure we don't try any more rendering in the subthread.
	grcContextCommandList *list = g_grcCurrentContext;
	g_grcCurrentContext = NULL;
	return list;
}

void grcDevice::ExecuteCommandList(grcContextCommandList *list)
{
	if (!list)
		return;

	grcGfxContext::kickContext();
	list->submitPending(grcGfxContext::SUBMIT_FULL);
}

void grcDevice::BeginCommandList()
{
	// Prevent any rendering from main thread before we call EndCommandList again
	grcGfxContext::kickContext();
	FatalAssert(!s_savedGrcCurrentContext);
	s_savedGrcCurrentContext = g_grcCurrentContext;
	g_grcCurrentContext = NULL;
}

void grcDevice::EndCommandList()
{
	// Make sure the main context is usable again
	FatalAssert(s_savedGrcCurrentContext);
	g_grcCurrentContext = s_savedGrcCurrentContext;
	s_savedGrcCurrentContext = NULL;
}

static void SetEqaaControl(sce::Gnm::NumSamples nsamp, sce::Gnm::NumSamples nanchors, sce::Gnm::NumSamples niter)
{
	sce::Gnm::DepthEqaaControl dec;
	dec.init();
	if (nsamp)
	{
		const eqaa::Control &ct = eqaa::GetControl();
		dec.setAlphaToMaskSamples( nsamp );
		dec.setMaskExportNumSamples( nsamp );
		dec.setMaxAnchorSamples( nanchors );
		dec.setPsSampleIterationCount( niter );
		dec.setIncoherentEqaaReads( ct.incoherentEqaaReads );
		dec.setStaticAnchorAssociations( ct.staticAnchorAssociations );
		dec.setInterpolateCompressedZ( ct.interpolateCompressedZ );
		dec.setHighQualityTileIntersections( ct.highQualityTileIntersections );
	}
	gfxc.setDepthEqaaControl(dec);
}

void grcDevice::SetAACount(u32 nsamp, u32 nfrag, u32 niter)
{
	if (s_ActiveNumSamples != nsamp || s_ActiveNumFragments != nfrag || s_ActiveNumIterations != niter)
	{
		s_ActiveNumSamples		= nsamp;
		s_ActiveNumFragments	= nfrag;
		s_ActiveNumIterations	= niter;

		gfxc.setScanModeControl(
			nsamp>1 ? sce::Gnm::kScanModeControlAaEnable : sce::Gnm::kScanModeControlAaDisable,
			(sce::Gnm::ScanModeControlViewportScissor)s_bScanMode_ViewportScissor
#if SCE_ORBIS_SDK_VERSION < (0x01700000u)
			, s_ActiveNumSamples&&0 ? sce::Gnm::kScanModeControlLineStippleEnable : sce::Gnm::kScanModeControlLineStippleDisable
#endif
			);
		
		gfxc.setAaSampleCount( castNumSamples(nsamp) );

		SetAALocations( nsamp, nfrag );
		SetEqaaControl( castNumSamples(nsamp), castNumSamples(nfrag), castNumSamples(niter)	);
	}
}


#define FIXED_SAMPLE_LOCATIONS	(1 || ENABLE_LCUE)

// Blocked by Sony internal issue 13035
#if FIXED_SAMPLE_LOCATIONS
static uint32_t EncodeOffset(int off)
{
	Assert( off>=-8 && off<8 );
	return uint32_t( off>=0 ? off : off+16 );
}

static uint32_t PackLocations(int s0x, int s0y, int s1x, int s1y, int s2x, int s2y, int s3x, int s3y)
{
	return EncodeOffset(s0x)		|
		(EncodeOffset(s0y) << 4)	|
		(EncodeOffset(s1x) << 8)	|
		(EncodeOffset(s1y) << 12)	|
		(EncodeOffset(s2x) << 16)	|
		(EncodeOffset(s2y) << 20)	|
		(EncodeOffset(s3x) << 24)	|
		(EncodeOffset(s3y) << 28);
}

static uint32_t SwapLocations(uint32_t loc)
{
	return (((loc & 0xF0F0F0F0)>>4) | ((loc & 0x0F0F0F0F)<<4));
}

static uint32_t MirrorShiftYLocations(uint32_t loc)
{
	return loc ^ 0xF0F0F0F0;
}

# if AA_SAMPLE_DEBUG
static uint32_t PackRotatedLocations(float radius, float angle)
{
	int sx[4] = {0}, sy[4] = {0};

	for (int i=1; i<4; ++i)
	{
		float rad = (angle + (i-1)*120.f) * PI/180.f;
		float x = radius * cosf(rad);
		float y = radius * sinf(rad);
		sx[i] = (int)roundf(x * 8.f);
		sy[i] = (int)roundf(y * 8.f);
	}

	return PackLocations(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2], sx[3], sy[3]);
}
# endif //AA_SAMPLE_DEBUG

#endif	// FIXED_SAMPLE_LOCATIONS

void grcDevice::SetAALocations(u32 ns, u32 nf)
{
#if FIXED_SAMPLE_LOCATIONS
	const bool fixedLocs = nf==1 && eqaa::CENTERED;
	if (fixedLocs)
	{
		const bool dithered = true;
		uint32_t locs[16];
		if (nf > 1 || !eqaa::CENTERED)
		{
			// using the pattern from slice 75 of Chapter_3 (R10xx_Block_Detail)
			locs[0] = PackLocations(-7,-3, 7,3, -5,5, 1,-5);
			locs[1] = PackLocations(-3,-7, 3,7, -1,1, 5,-1);
			locs[2] = PackLocations(-4,-2, 4,2, -2,6, 2,-8);
			locs[3] = PackLocations(-8,-6, 0,4, -6,0, 6,-4);
		}else
		{
			Assert( ns <= 4 );
			// using a triangle pattern, requires a *_Centered resolve shader
			locs[0] = PackLocations(0,0, 3,-7, 1,6, -6,-1);
			locs[1] = 0;	//PackLocations(0,0, 0,0, 0,0, 0,0);
			locs[2] = 0;
			locs[3] = 0;
		}
	
		if (dithered && eqaa::CENTERED && nf==1)
		{
			for (uint i=4; i<16; ++i)
			{
				locs[i] = locs[i&3];
			}
# if AA_SAMPLE_DEBUG
			const eqaa::Debug &ed = eqaa::GetDebug();
			if (ed.useDeprecatedPattern)
			{
				// the old deprecated pattern to compare to
				locs[4] = PackLocations(0,0, -7,-3, 6,-1, -1,6);
				locs[8] = PackLocations(0,0, -7,3, 6,1, -1,-6);
				locs[12] = PackLocations(0,0, 3,7, 1,-6, -6,1);
				
			}else
			{
				const float offset[4] = {0.f, 90.f, 270.f, 180.f};
				for (uint i=0; i<4; ++i)
				{
					locs[4*i] = PackRotatedLocations(ed.patternRadius, ed.patternOffset + offset[i]);
				}
			}
# else // AA_SAMPLE_DEBUG
			{
				// PackRotatedLocations(0.9, 15+offset[i])
				locs[0]		= PackLocations(0,0, +7,+2, -5,+5, -2,-7);
				locs[4]		= PackLocations(0,0, -2,+7, -5,-5, +7,-2);
				locs[8]		= PackLocations(0,0, +2,-7, +5,+5, -7,+2);
				locs[12]	= PackLocations(0,0, -7,-2, +5,-5, +2,+7);
			}
#endif // AA_SAMPLE_DEBUG
		}else
		if (dithered)
		{
			for(uint i=4; i<8; ++i)
			{
				locs[i] = MirrorShiftYLocations(SwapLocations( locs[i-4] ));
			}
			for(uint i=8; i<12; ++i)
			{
				locs[i] = MirrorShiftYLocations( locs[i-4] );
			}
			for(uint i=12; i<16; ++i)
			{
				locs[i] = MirrorShiftYLocations(SwapLocations( locs[i-4] ));
			}
		}else
		{
			for (uint i=4; i<16; ++i)
			{
				locs[i] = locs[i&3];
			}
		}

		gfxc.setAaSampleLocations( locs );
	}else
#endif	//FIXED_SAMPLE_LOCATIONS 
	{
		gfxc.setAaDefaultSampleLocations( castNumSamples(ns) );
	}
}

void grcDevice::SetAASuperSample(bool superSample)
{
	Assertf(!superSample || s_ActiveNumSamples, "Attempted to process a non-multisamplet target on a sample frequency");
	if (s_ActiveSuperSample != superSample)
	{
		gfxc.setPsShaderRate( superSample ? sce::Gnm::kPsShaderRatePerSample : sce::Gnm::kPsShaderRatePerPixel );
		s_ActiveSuperSample = superSample;
	}
}

#if ENABLE_GPU_DEBUGGER
bool grcDevice::IsGpuDebugged()
{
#if GPU_DEBUGGER_MANUAL_LOAD
	return s_DebuggerHandle>0;
#else
	return PARAM_gpuDebugger.Get();
#endif
}
#endif //ENABLE_GPU_DEBUGGER

#if DEPTH_BOUNDS_SUPPORT
void grcDevice::SetDepthBoundsTestEnable(u32 enable)
{
	//Handled by state block
#if TRACK_DEPTH_BOUNDS_STATE
	s_DepthBoundsActive = enable;
#else
	(void)enable;
#endif
}

void grcDevice::SetDepthBounds(float zmin, float zmax)
{
	float fixedZmin = Clamp(FixDepth(zmin),0.0f,1.0f);
	float fixedZmax = Clamp(FixDepth(zmax),0.0f,1.0f);

	gfxc.setDepthBoundsRange(Min(fixedZmin, fixedZmax),Max(fixedZmin, fixedZmax));
}
#endif //DEPTH_BOUNDS_SUPPORT

#if DRAWABLE_STATS
void grcDevice::SetFetchStatsBuffer(drawableStats* dstPtr) 
{
	g_pCurrentStatsBucket = dstPtr;
}
#endif



} // namespace rage

#endif		// __GNM
