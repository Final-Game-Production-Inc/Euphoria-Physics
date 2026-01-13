// 
// grcore/device_d3d11.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/device.h"
#include "grcore/effect_config.h"
#include "system/xtl.h"

#if (__D3D11)

#include <process.h>
#define DX_ONLY_INCLUDE_ONCE

#include "channel.h"
#include "image.h"
#include "quads.h"

#include "diag/diagerrorcodes.h"
#include "grcore/d3dwrapper.h"
#include "grcore/effect.h"
#include "grcore/fastquad.h"
#include "grcore/fencepool.h"
#include "grcore/texturedefault.h"
#include "grcore/viewport.h"
#include "input/mouse.h"
#include "input/headtracking.h"
#include "profile/element.h"
#include "profile/group.h"
#include "profile/page.h"
#include "profile/timebars.h"
#include "string/unicode.h"
#include "system/customhangdetection.h"
#include "system/d3d11.h"
#include "system/hangdetect.h"
#include "system/membarrier.h"
#include "system/memory.h"
#include "system/nelem.h"
#include "system/param.h"
#include "system/stack.h"
#include "security/obfuscatedtypes.h"
#include "grprofile/pix.h"
#include "wrapper_d3d.h"
#include "..\..\3rdparty\portcullis\portcullis.h"

#if __WIN32PC

#if defined(PORTCULLIS_ENABLE) && PORTCULLIS_ENABLE
#include "fwmaths/random.h"
#endif

//#include <d3DX11async.h>
#pragma warning(disable: 4668)
#include <winsock2.h>
#include <Wtsapi32.h>
#pragma comment(lib,"Wtsapi32.lib")
#pragma warning(error: 4668)

#include <D3D11Shader.h>
#include <D3DCompiler.h>
#include <DXGI.h>
#pragma comment(lib,"DXGI.lib")
#include "system/wndproc.h"
#elif RSG_DURANGO
#include "grcore/gfxcontext_durango.h"
#	include <D3D11Shader_x.h>
#	include <D3DCompiler_x.h>
#	pragma comment(lib,"d3d11_x.lib")
#	pragma comment(lib,"xg_x.lib")
#endif
#include "grpostfx/postfx.h"
#include "grcore/stateblock.h"
#include "grcore/vertexbuffer.h"
#include "grcore/locktypes.h"
#include "grcore/texture.h"
#include "grcore/im.h"
#include "grcore/locktypes.h"
#include "grcore/indexbuffer.h"
#include "grcore/vertexbuffer.h"

#include "grcore/texture_d3d11.h"
#include "grcore/texturefactory_d3d11.h"
#include "grcore/rendertarget_d3d11.h"

#include "grcore/adapter.h"
#include "grcore/adapter_d3d11.h"
#include "grcore/viewport_inline.h"

#if __D3D11
#include "grmodel/model.h"
#endif

#define CHECK_SWAP_CHAIN (0 && RSG_PC)

#define MEMORYCS_COMPILING_SHADER   0
#include "shaderlib/memorycs.fxh"

// Undefine this to turn off all NV shadow changes.  There is also a Rag widget boolean to disable them dynamically.
#define USE_NV_SHADOW_LIB RSG_PC

// Similarly for TXAA.
//#define USE_NV_TXAA RSG_PC && !__GAMETOOL
//#define USE_NV_TXAA 0

#if RSG_PC
#include "../../3rdParty/portcullis/portcullis.h"

#define __GFSDK_OS_WINDOWS__
#define __GFSDK_DX11__

#if USE_NV_TXAA
#include "../../3rdParty/NVidia/NVTXAA/include/GFSDK_TXAA.h"
TxaaCtxDX g_txaaContext;
#if RSG_CPU_X64
#pragma comment(lib,"GFSDK_TXAA_AlphaResolve.win64.lib")
#elif RSG_CPU_X86
#pragma comment(lib,"GFSDK_TXAA_AlphaResolve.win32.lib")
#endif
#endif

#if USE_NV_SHADOW_LIB
// Although an Nvidia lib, this is cross-platform on PC, unlike NvAPI.
#include "../../3rdParty/NVidia/GFSDK_ShadowLib/include/GFSDK_ShadowLib.h"
#if RSG_CPU_X64
#pragma comment(lib,"GFSDK_ShadowLib.win64.lib")
#elif RSG_CPU_X86
#pragma comment(lib,"GFSDK_ShadowLib.win32.lib")
#endif

// TBD: where should this live?  Is equivalent to the D3D context.  Is a global OK?
NV_ShadowLib_Ctx g_shadowContext;
#endif

#if NV_SUPPORT
#include "../../3rdParty/NVidia/nvapi.h"

#define NO_STEREO_D3D9
#define NO_STEREO_D3D10
#include "../../3rdParty/NVidia/nvstereo.h"

#if NV_SUPPORT
#if RSG_CPU_X64
#pragma comment(lib,"nvapi64.lib")
#elif RSG_CPU_X86
#pragma comment(lib,"nvapi.lib")
#endif
#endif

nv::stereo::ParamTextureManager<nv::stereo::D3D11Type> *s_oNVStereoManager = NULL;

#endif // NV_SUPPORT

#if USE_AMD_SHADOW_LIB
// Although an AMD lib, this is cross-platform on PC
#include "../../3rdParty/AMD/AMD_Shadows/include/AMD_Shadows.h"

#if RSG_CPU_X64
#if __OPTIMIZED
#pragma comment(lib, "AMD_Shadows_X64.lib")
#else
#pragma comment(lib, "AMD_Shadows_X64d.lib")
#endif  // __OPTIMIZED
#elif RSG_CPU_X86
#if __OPTIMIZED
#pragma comment(lib, "AMD_Shadows_X86.lib")
#else
#pragma comment(lib, "AMD_Shadows_X86d.lib")
#endif  // __OPTIMIZED
#endif

// TBD: where should this live?  Is a global OK?
AMD::SHADOWS_DESC g_amdShadowDesc;
#endif

#if ATI_EXTENSIONS
#include "../../3rdParty/AMD/AMD_Extensions/AmdDxExtDepthBoundsApi.h"
IAmdDxExt*							g_pExtension = NULL;
IAmdDxExtDepthBounds*				g_pDepthBoundsTest = NULL;

#if UAV_OVERLAP_SUPPORT
	#include "../../3rdParty/AMD/AMD_Extensions/AmdDxExtUAVOverlapApi.h"

	IAmdDxExtUAVOverlap*				g_pUAVOverlap = NULL;
#endif // UAV_OVERLAP_SUPPORT

#endif //ATI_EXTENSIONS

#pragma comment(lib,"d3d9.lib")

#elif RSG_DURANGO
#define PORTCULLIS_ENABLE 0
#include "grcore/durango_window.h"
#pragma comment(lib,"user32.lib")
#endif

#if RSG_PC
NOSTRIP_PARAM(noVendorAPI, "[grcore] Don't use vendor API");
#endif

XPARAM(rag);
NOSTRIP_XPARAM(fullscreen);
NOSTRIP_PARAM(DX11,"[grcore] Force 11.0 feature set");
NOSTRIP_PARAM(DX10_1,"[grcore] Force 10.1 feature set");
NOSTRIP_PARAM(DX10,"[grcore] Force 10.0 feature set");
PARAM(nullDriver,"[grcore] Initialise device with the NULL driver");
NOSTRIP_PARAM(availablevidmem, "[MEMORY] Available video memory (MB)");
PARAM(singleThreaded, "[grcore] Create the device as a single threaded device");
unsigned int g_EnablePixAnnotation = 0xffffffff;

PARAM(autodepthstencil, "[device_d3d] Automatically create depth/stencil buffers");
PARAM(ragUseOwnWindow, "[grcore] Displays the game in its own window rather than inside rag");

namespace rage {

NOSTRIP_PC_XPARAM(allowResizeWindow);
NOSTRIP_PC_XPARAM(disallowResizeWindow);

NOSTRIP_PC_PARAM(frameLimit, "[grcore] number of vertical synchronizations to limit game to");
NOSTRIP_PARAM(hdr,"[device_d3d] Set the whole rendering pipeline to 10-bit on 360 and 16-bit on PC");
NOSTRIP_PARAM(width,"[grcore] Set width of main render window (default is 640)");
NOSTRIP_PARAM(height,"[grcore] Set height of main render window (default is 480)");
PARAM(refreshrate,"[grcore] Set refresh rate of main render window");
PARAM(unmanaged, "[grcore] Set resource managed to self-managed"); // Use unmanaged resource pool
PARAM(noBlockOnLostFocus,"[grcore] Don't block the window update when it loses focus.");
PARAM(setHwndMain,"[grcore] override the window that DirectX will render to");
NOSTRIP_PARAM(adapter,"[grcore] Use the specified screen adapter number (zero-based)");
PARAM(outputMonitor,"[grcore] Use the specified monitor output of the selected adapter number (zero-based)");
PARAM(monitor,"[grcore] Use the specified monitor number (zero-based)");
PARAM(displaycaps,"[grcore] Show display capabilities");
NOSTRIP_PARAM(multiSample,"[grcore] Number of multisamples (1, 2, 4, or 8)");
#if DEVICE_EQAA
NOSTRIP_PARAM(multiFragment,"[grcore] Number of color/depth fragments (1,2,4, or 8)");
#else
PARAM(multiSampleQuality,"[grcore] Quality level of multisamples - Video card dependent");
#endif // DEVICE_EQAA
PARAM(debugshaders,"[grcore] Enable D3D shader debugging");
#if NV_SUPPORT
PARAM(convergence,"[grcore] Set convergence of 3D vision (default is 3.5)");
PARAM(separation,"[grcore] Set separation of 3D vision (default is 20)");
#endif
PARAM(nvstereo,"[grcore] enable/disable 3D stereo for NVIDIA");
XPARAM(borderless);
PARAM(noWaitOnGpu,"Wait for the GPU to finish each frame (slows down max fps we're able to render at");
#if !RSG_DURANGO
PARAM(restoreContextState,"Don't restore the context state after executing a command list (default=true), false is quicker");
PARAM(dynamicVBufferSize,"Set the size of the dynamic vertex buffer");
PARAM(dynamicIBufferSize,"Set the size of the dynamic index buffer");
#endif
#if RSG_DURANGO
PARAM(presentThreshold,"The immediate threshold for presenting, as a percentage (from 0 to 100) of screen coverage in scanlines.");
#endif

PARAM(pixDebug, "Enable extra PIX debug info.");

#if __D3D11_1
PARAM(longLifeCommandLists,"Deferred command lists last over multiple frames. Helps debugging in PIX");
#endif // RSG_DURANGO

static bool s_blockOnFence = true; // (!RSG_PC || __DEV);

#define grcDeviceDisplay(fmt,...) grcDisplayf(fmt,__VA_ARGS__)

static bool s_WeCreatedHwndMain;
static bool s_UseReadOnlyDepth = false;
static bool s_BackBufferLocked = false;
grcSwapChain* s_BackupSwapChain = NULL;

sysIpcCurrentThreadId grcDevice::sm_Owner = sysIpcCurrentThreadIdInvalid;
static grcDeviceHandle *g_pd3dDevice = NULL;
static grcContextHandle *g_pd3dDeviceContext = NULL;

#if DEPTH_BOUNDS_SUPPORT
static DECLARE_MTR_THREAD u32 s_DepthBoundsTestEnabled = false;
static DECLARE_MTR_THREAD float s_DepthBoundsTestMin = 0.0f;
static DECLARE_MTR_THREAD float s_DepthBoundsTestMax = 1.0f;
static bool  s_DepthBoundsTestSupported = true;
#endif //DEPTH_BOUNDS_SUPPORT

const char *grcDevice::sm_DefaultEffectName = "x:\\rage\\assets\\tune\\shaders\\lib\\rage_im";
grcCommandBuffer *g_grcCommandBuffer;
grcDeviceHandle *grcDevice::sm_Current;

static grcEffectTechnique s_DefaultClear;

#if RSG_DURANGO
ID3D11DmaEngineContextX*	grcDevice::sm_DmaEngineContext1;
ID3D11DmaEngineContextX*	grcDevice::sm_DmaEngineContext2;
ServiceDelegate grcDevice::sm_Delegate;
#endif

static RageDirect3DDevice11			s_Device11Wrapper;
#if DEVICE_USE_D3D_WRAPPER
static RageDirect3DDeviceContext11	s_DeviceContext11Wrapper;
#endif // DEVICE_USE_D3D_WRAPPER

#if RAGE_INSTANCED_TECH
#define MAX_NUM_VIEWPORTS       (MAX_NUM_CBUFFER_INSTANCING)
#else
#define MAX_NUM_VIEWPORTS       (1)
#endif
static DECLARE_MTR_THREAD D3D11_VIEWPORT sm_viewports[MAX_NUM_VIEWPORTS];
static DECLARE_MTR_THREAD UINT sm_numViewports;

#if RAGE_INSTANCED_TECH
#define MAX_NUM_SCISSOR_RECTS   (MAX_NUM_CBUFFER_INSTANCING)
#else
#define MAX_NUM_SCISSOR_RECTS   (1)
#endif
static DECLARE_MTR_THREAD D3D11_RECT sm_scissorRects[MAX_NUM_SCISSOR_RECTS];
static DECLARE_MTR_THREAD UINT sm_numScissorRects;


#if !RSG_DURANGO
static ID3D11Buffer *s_MissingInputsVertexBuffer;
#endif
static const DWORD s_BeginVerticesTriFanMaxIndices = 256;

#if RSG_DURANGO
static u16 *s_BeginVerticesTriFanIndices;
#else
static ID3D11Buffer *s_BeginVerticesTriFanIndices;
static DECLARE_MTR_THREAD ID3D11Buffer *s_BeginVertexBuffer=NULL;
static DECLARE_MTR_THREAD ID3D11Buffer *s_BeginIndexBuffer=NULL;
static DECLARE_MTR_THREAD UINT s_grcVBOffset, s_grcIBOffset;
static DECLARE_MTR_THREAD void *s_grcVBLock, *s_grcIBLock;
static DECLARE_MTR_THREAD UINT s_grcIndexCount;
static u32 s_BeginVerticesBufferSize = grcDevice::BEGIN_VERTICES_MAX_SIZE;
static u32 s_BeginIndicesBufferSize = grcDevice::BEGIN_INDICES_MAX_SIZE * sizeof(u16);
#endif

static DECLARE_MTR_THREAD UINT s_grcVertexCount;

#if EFFECT_CACHE_PROGRAMRESOURCES
static DECLARE_MTR_THREAD u32 s_Fingerprints[NUM_TYPES] = {NULL};

#if __BANK
bool sm_useCBFingerprints=true;
#endif // __BANK

#endif // EFFECT_CACHE_PROGRAMRESOURCES

static DECLARE_MTR_THREAD grcDrawMode s_grcDrawMode;
extern unsigned int g_grcDepthFormat;
#if RSG_PC
extern WINDOWPLACEMENT s_WindowedPlacement;
static DXGI_SWAP_CHAIN_DESC g_SwapDesc;
#else
static DXGI_SWAP_CHAIN_DESC1 g_SwapDesc;
#endif

#if DRAWABLE_STATS
__THREAD drawableStats *g_pCurrentStatsBucket = NULL;
#endif

#define DEBUG_FENCES    (1 && RSG_DURANGO && !__PROFILE && !__FINAL)

#if DEBUG_FENCES
static volatile u64 *g_GpuWaitFence;
#endif

#if __BANK && RSG_DURANGO
DXGIX_FRAME_STATISTICS g_FrameStatistics[16];
#endif // __BANK && RSG_DURANGO

#if RSG_PC && __BANK
static int	s_forceSyncInterval = -1;
static bool s_disableUnusedStages=true;
#endif // RSG_PC && __BANK

// ==================================================================================================================
// USE_FRAME_LATENCY_LIMITTING_FENCE : Use fences to prevent the GPU from getting more than
// N-frames behind the CPU. This is important since on Durango, if the GPU gets
// too far behind, the GPU starts consuming junk data since the CPU has already re-used its memory for newer frames.
// A lot of our rendering code seems to assume that the GPU is never too far behind.
//
// This allows the GPU to be, worst case, at the very end of frame N-Max Frame while the
// CPU is just beginning frame N. The CPU will idle if it tries to begin frame N+1. By
// "at the very end of frame (N-Max Frames)" I mean that the GPU has completed all
// (N-Max Frames)'s rendering commands but has not yet done the flip. So the GPU has
// consumed all data for (N-Max Frames) and we are safe to proceed creating commands for N.
#define USE_FRAME_LATENCY_LIMITTING_FENCE ((1 && RSG_DURANGO) || (1 && RSG_PC))
#if USE_FRAME_LATENCY_LIMITTING_FENCE
	static grcFenceHandle *ms_pahFrameLatencyFenceHandles = NULL; //[MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU+1];

	NOSTRIP_PC_PARAM(FrameQueueLimit, "Maxiumum number of frames that can be queued up");
#endif // USE_FRAME_LATENCY_LIMITTING_FENCE
// ==================================================================================================================

// Mutual exclusion mechanism used for locking the D3D context.
//sysCriticalSectionToken s_ContextLock;
sysIpcMutex s_ContextLock;

sysCriticalSectionToken s_SwapChainLock;
#if NV_SUPPORT
// critical section for nvapi 3D stereo
sysIpcMutex s_NVStereoLock;
__THREAD uint32_t s_NVStereoLockReferenceCount = 0;
static grcEffectGlobalVar s_gStereoTextureID;
#endif

// Context lock ref count (one per thread).
__THREAD uint32_t s_ContextLockReferenceCount = 0;

__THREAD grcContextHandle *					g_grcCurrentContext = NULL;
static DECLARE_MTR_THREAD grcCBuffer *				s_VSConstantBufferOverrides[MAX_CONSTANT_BUFFER];
static DECLARE_MTR_THREAD ID3D11ShaderResourceView*	s_ResourceViews[EFFECT_TEXTURE_COUNT];

grcDevice *g_pGRCDEVICE;
#if RSG_DURANGO
#define DEFAULT_WINDOW_WIDTH  (1920)
#define DEFAULT_WINDOW_HEIGHT (1080)
#else
#define DEFAULT_WINDOW_WIDTH  (1280)
#define DEFAULT_WINDOW_HEIGHT (720)
#endif

grcDisplayWindow grcDevice::sm_CurrentWindows[NUMBER_OF_RENDER_THREADS];
grcDisplayWindow grcDevice::sm_GlobalWindow = grcDisplayWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, 0, 1);
#if RSG_PC
grcDisplayWindow grcDevice::sm_FullscreenWindow = grcDisplayWindow(1280, 720, 0, 1, 1);
grcDisplayWindow grcDevice::sm_DesiredWindow = grcDisplayWindow(0, 0, 0, 1, 1);
#endif
MonitorConfiguration grcDevice::sm_MonitorConfig;

PARAM(pixannotation,"[grcore] Enable Pix annotation via the PIXBegin()/PIXEnd() macros");
grcEffect *g_DefaultEffect;
grcEffectVar g_DefaultSampler;
grcEffectVar g_DefaultColor;
grcEffectTechnique s_DefaultLit, s_DefaultUnlit, s_DefaultLitSkinned, s_DefaultUnlitSkinned, s_DefaultBlit;
unsigned int g_grcDepthFormat = rage::grctfD32FS8;
grcVertexDeclaration *grcDevice::sm_BlitDecl;
grcVertexDeclaration *grcDevice::sm_ImDecl;

grcDepthStencilStateHandle clearStates_DSS[2][2];	// clearDepth, clearStencil
grcBlendStateHandle clearStates_BS[2];

u32 grcDevice::sm_ClipPlaneEnable[NUMBER_OF_RENDER_THREADS] = {0};
#if DEVICE_CLIP_PLANES
u32 grcDevice::sm_PreviousClipPlaneEnable[NUMBER_OF_RENDER_THREADS] = {0};
u32 grcDevice::sm_ClipPlanesChanged[NUMBER_OF_RENDER_THREADS] = {0};

// World space clip planes.
Vec4V grcDevice::sm_ClipPlanes[RAGE_MAX_CLIPPLANES][NUMBER_OF_RENDER_THREADS];
// Constant buffer containing clipping space plane equations.
grcCBuffer *grcDevice::sm_pClipPlanesConstBuffer = NULL;
#endif	//DEVICE_CLIP_PLANES

grcDevice::MSAAMode grcDevice::sm_MSAA = grcDevice::MSAA_None;
#if DEVICE_EQAA
const grcDevice::MSAAMode grcDevice::MSAA_None(0);
bool grcDevice::sm_EQAA = false;

static u32 GetIntLog(u32 value)
{
	switch (value)
	{
	case 0:
	case 1:		return 0;
	case 2:		return 1;
	case 4:		return 2;
	case 8:		return 3;
	case 16:	return 4;
	default:
		Assertf(0,"Invalid pow2 value: %u", value);
		return 0;
	}
}

grcDevice::MSAAMode::MSAAMode(const DXGI_SAMPLE_DESC &desc)
: m_bEnabled( desc.Count > 1 || desc.Quality > 0 )
, m_uSamples( 1<<desc.Quality )
, m_uFragments( desc.Count )
{}

u32 grcDevice::MSAAMode::DeriveQuality() const
{
	return GetIntLog( m_bEnabled * m_uSamples );
}

u32 grcDevice::MSAAMode::GetFmaskFormat() const
{
	if (!m_bEnabled)
		return DXGI_FORMAT_UNKNOWN;

	const u32 bitsPerPixel = m_uSamples * GetFmaskShift();
	return
		bitsPerPixel<=8		? DXGI_FORMAT_R8_UINT	:
		bitsPerPixel==16	? DXGI_FORMAT_R16_UINT	:
		bitsPerPixel==32	? DXGI_FORMAT_R32_UINT	:
		bitsPerPixel==64	? DXGI_FORMAT_R32G32_UINT	:
		DXGI_FORMAT_UNKNOWN;
}

u32 grcDevice::MSAAMode::GetFmaskShift() const
{
	u32 uBitsPerSample = GetIntLog(m_uFragments) + (m_uSamples>m_uFragments);
	return uBitsPerSample<=2 ? uBitsPerSample : 4;
}
#else // DEVICE_EQAA
u32 grcDevice::sm_MultisampleQuality = 0;
#endif // DEVICE_EQAA

#if RSG_PC
DECLARE_MTR_THREAD u32			sm_VSConstantBufferOverrideStartSlot=0;
DECLARE_MTR_THREAD u32			sm_VSConstantBufferOverrideCount=0;
DECLARE_MTR_THREAD grcCBuffer**	sm_VSConstantBufferOverrides = NULL;
#endif

PF_PAGE(GfxDeviceStatsPage,"D3D_grcDeviceStats");
PF_GROUP(GfxDynamicResourcesGroup);
PF_LINK(GfxDeviceStatsPage,GfxDynamicResourcesGroup);

PF_TIMER(DynamicVBufferMapTime,GfxDynamicResourcesGroup);
PF_COUNTER(VBDynamicMaps,GfxDynamicResourcesGroup);
PF_COUNTER(VBBytesPerFrame,GfxDynamicResourcesGroup);
PF_COUNTER(VBOverwritesPerFrame,GfxDynamicResourcesGroup);

PF_COUNTER(IBDynamicMaps,GfxDynamicResourcesGroup);
PF_COUNTER(IBBytesPerFrame,GfxDynamicResourcesGroup);
PF_COUNTER(IBOverwritesPerFrame,GfxDynamicResourcesGroup);

PF_COUNTER(CBDynamicMaps,		GfxDynamicResourcesGroup);
PF_COUNTER(CBDynamicMapBytes,	GfxDynamicResourcesGroup);

#if !__FINAL
PARAM(gameWindowFocusLock, "Don't allow other programs to take away the window focus");
PARAM(debugRuntime,"[grcore] Enable DX11 Debug Runtime.");
PARAM(monoRuntime,"[grcore] (Default=1 for non-final) 0: No debugging, 1: Profiling support, for example for PIX captures, 2: Profiling support plus parameter validation, 3: Profiling support, parameter validation, and Debug compilation (supporting asserts, RIPs, and so on).");
#if RSG_DURANGO
PARAM(noFastKickoffs,"Disable the D3D11_CREATE_DEVICE_FAST_KICKOFFS device flag");
#endif // RSG_DURANGO
#endif // !__FINAL

const int BlitVertSize = 2048;

struct BlitVert {
	float x, y, z, invW;
	u32 diffuse, specular;
	float u, v;
	void Set(float _1,float _2,float _3,float _4,u32 _5,u32 _6,float _7,float _8) {
		x=_1; y=_2; z=_3; invW=_4;
		diffuse=_5; specular=_6;
		u=_7; v=_8;
	}
};

#if !RSG_DURANGO
static grcVertexBuffer *s_BlitVB = NULL;
static DECLARE_MTR_THREAD int s_BlitOffset;
#endif


grcDevice::grcRenderTargetView grcDevice::sm_aRTView[grcmrtColorCount] =	{ 
																				{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL }, 
																				{ NULL, NULL }, { NULL, NULL }, { NULL, NULL }, { NULL, NULL } 
																			};
grcDevice::grcRenderTargetView grcDevice::sm_DepthView = { NULL, NULL };
grcDevice::grcRenderTargetView grcDevice::sm_PreviousDepthView = { NULL, NULL };

u32 grcDevice::sm_numTargets = 0;

//ID3D11Texture2D*	grcDevice::sm_pDepthStencil = 0;
//ID3D11DepthStencilView* grcDevice::sm_pDepthStencilView = 0;
grcRenderTarget*	grcDevice::sm_pBackBuffer = NULL;
grcRenderTarget*	grcDevice::sm_pDepthStencil = 0;
DECLARE_MTR_THREAD grcVertexDeclaration* grcDevice::sm_CurrentVertexDeclaration = 0;
DECLARE_MTR_THREAD grcVertexProgram*		grcDevice::sm_CurrentVertexProgram = 0;
DECLARE_MTR_THREAD grcFragmentProgram*	grcDevice::sm_CurrentFragmentProgram = 0;
DECLARE_MTR_THREAD grcComputeProgram*	grcDevice::sm_CurrentComputeProgram = 0;
DECLARE_MTR_THREAD grcDomainProgram*		grcDevice::sm_CurrentDomainProgram = 0;
DECLARE_MTR_THREAD grcGeometryProgram*	grcDevice::sm_CurrentGeometryProgram = 0;
DECLARE_MTR_THREAD grcHullProgram*		grcDevice::sm_CurrentHullProgram = 0;

#if RSG_PC
DECLARE_MTR_THREAD u32						s_ShaderStagesActive=0;			// A bitmask of the stages active for the pending draw
DECLARE_MTR_THREAD u32						s_ShaderStagesLastActive=0;		// A bitmask of the stages active for the previous draw.
#endif // RSG_PC

grcDepthStencilStateHandle grcDevice::sm_WriteDepthNoStencil = grcStateBlock::DSS_Invalid;
grcDepthStencilStateHandle grcDevice::sm_WriteStencilNoDepth = grcStateBlock::DSS_Invalid;
grcDepthStencilStateHandle grcDevice::sm_WriteNoDepthNoStencil = grcStateBlock::DSS_Invalid;
grcDepthStencilStateHandle grcDevice::sm_WriteDepthAndStencil = grcStateBlock::DSS_Invalid;

#if !RSG_DURANGO
D3DVERTEXSTREAMINFO grcDevice::sm_aVertexStreamInfo[NUMBER_OF_RENDER_THREADS];
#endif

static ID3D11Query* s_KickEventQuery = NULL;
#if RSG_DURANGO
static grcEffect* s_MemoryCS;
#elif RSG_PC
static const int kMaxFenceQueries = 512;
static ID3D11Query* s_FenceQueries[kMaxFenceQueries];
static rage::atFixedBitSet<kMaxFenceQueries> s_FenceQueriesFreeList;
#endif
static FencePool s_FencePool(64);

extern grcEffect *g_DefaultEffect;
extern grcEffectVar g_DefaultSampler;
extern grcEffectTechnique s_DefaultLit, s_DefaultUnlit, s_DefaultLitSkinned, s_DefaultUnlitSkinned, s_DefaultBlit;

extern HRESULT GetVideoMemoryViaDXGI( HMONITOR hMonitor, SIZE_T* pDedicatedVideoMemory, SIZE_T* pDedicatedSystemMemory, SIZE_T* pSharedSystemMemory );

// Misc implementations

void RefreshRate::operator =(const DXGI_RATIONAL &rat)
{
	Numerator = rat.Numerator;
	Denominator = rat.Denominator;
}
bool RefreshRate::operator ==(const DXGI_RATIONAL &rat) const
{
	return Numerator==rat.Numerator && Denominator==rat.Denominator;
}


//=============================================================================
// Functions
//=============================================================================

#if RSG_PC && __D3D11
grcSwapChain * grcDevice::GetBackupSwapChain()
{
	return s_BackupSwapChain;
}
#endif
static void CreateQueries()
{
	using namespace rage;
	if (g_grcCurrentContext != NULL)
	{
		D3D11_QUERY_DESC oDesc;
		oDesc.Query = D3D11_QUERY_EVENT;
		oDesc.MiscFlags = 0;
		ID3D11Query *s_KickEventQuery = NULL;

		grcDeviceHandle *dev = GRCDEVICE.GetCurrent();
		HRESULT hRes = dev->CreateQuery(&oDesc, &s_KickEventQuery);
		CHECK_HRESULT(hRes);
		CheckDxHresultFatal(hRes);
#if RSG_PC
		for (int i = 0; i < kMaxFenceQueries; ++i)
		{
			CHECK_HRESULT(dev->CreateQuery(&oDesc, s_FenceQueries + i));
		}
#endif
	}
}

static void ReleaseQueries()
{
	LastSafeRelease(s_KickEventQuery);
#if RSG_PC
	for (int i = 0; i < kMaxFenceQueries; ++i)
	{
		LastSafeRelease(s_FenceQueries[i]);
	}
#endif
}

//=============================================================================
// Class Methods
//=============================================================================

#if DEVICE_EQAA
void grcDevice::ResolveMsaaBuffer(
	grcRenderTargetDefault *const destination, 
	grcRenderTargetDefault *const source,
	int destSliceIndex)
{
	const CoverageData &coverage = source->GetCoverageData();
	const grcTextureFormat sourceFormat = source->GetFormat();

	Assert(sourceFormat != grctfD24S8);
	Assert(sourceFormat != grctfD32FS8);

	if (!source->GetMSAA() && !coverage.donor)
	{
		u32 DstX = 0;
		u32 DstY = 0;
		u32 DstZ = 0;

		ID3D11Resource* poSrc = static_cast<ID3D11Resource*>(const_cast<grcTextureObject*>(source->GetTexturePtr()));
		ID3D11Resource* poDst = static_cast<ID3D11Resource*>(const_cast<grcTextureObject*>(destination->GetTexturePtr()));	
		
		u32 dstSubResource = D3D11CalcSubresource(0, destSliceIndex, destination->GetMipMapCount());
		u32 srcSubResource = D3D11CalcSubresource(0, 0,				 source->GetMipMapCount());

		g_grcCurrentContext->CopySubresourceRegion(poDst, dstSubResource, DstX, DstY, DstZ, poSrc, srcSubResource, NULL);
	}
	else if (!eqaa::ResolveMeSoftly(destination, source, destSliceIndex))
	{
		Assert(!coverage.donor);
		const DXGI_FORMAT fmt = static_cast<DXGI_FORMAT>( grcTextureFactoryDX11::ConvertToD3DFormat(sourceFormat) );

		u32 dstSubResource = D3D11CalcSubresource(0, destSliceIndex, destination->GetMipMapCount());
		u32 srcSubResource = D3D11CalcSubresource(0, 0,				 source->GetMipMapCount());

		g_grcCurrentContext->ResolveSubresource( destination->GetTexturePtr(), dstSubResource, source->GetTexturePtr(), srcSubResource, fmt );
	}

	DURANGO_ONLY(grcGfxContext::current()->insertHangCheckpoint();)
}
#endif // DEVICE_EQAA


void grcDevice::Blit_Init() 
{
#if !RSG_DURANGO
	grcFvf oDummyFVF;
	oDummyFVF.ClearAllChannels();
	oDummyFVF.SetPosChannel(true, grcFvf::grcdsFloat4);
	oDummyFVF.SetDiffuseChannel(true, grcFvf::grcdsColor);
	oDummyFVF.SetSpecularChannel(true, grcFvf::grcdsColor);
	oDummyFVF.SetTextureChannel(0, true, grcFvf::grcdsFloat2);

	// Assuming this vertex buffer is initialized once at start up.
	const bool bReadWrite = true;
	const bool bDynamic = true;
	s_BlitVB = rage_new grcVertexBufferD3D11(BlitVertSize / sizeof(BlitVert), oDummyFVF, bReadWrite, bDynamic, grcsBufferCreate_ReadWriteOnceOnly, grcsBufferSync_None, (u8 *)0, true);
	s_BlitOffset = 0;
#endif // !RSG_DURANGO
}

void grcDevice::Blit_Shutdown() 
{
#if !RSG_DURANGO
	if (s_BlitVB != NULL)
	{
		delete s_BlitVB;
		s_BlitVB = NULL;
	}
#endif
}

void grcDevice::GRC_Init() 
{
	grcFvf oDummyFVF;
	oDummyFVF.ClearAllChannels();
	oDummyFVF.SetPosChannel(true, grcFvf::grcdsFloat4);

#if !RSG_DURANGO
	s_grcVBOffset = 0;
#endif

	CreateQueries();

#if RSG_DURANGO
	s_MemoryCS = grcEffect::Create("common:/shaders/MemoryCS");
#endif

#if USE_FRAME_LATENCY_LIMITTING_FENCE
#if RSG_PC
	ms_MaxQueuedFrames = GetGPUCount();
	PARAM_FrameQueueLimit.Get(ms_MaxQueuedFrames);
#endif // RSG_PC

	Assertf(ms_pahFrameLatencyFenceHandles == NULL, "Leak - Already initialized ms_pahFrameLatencyFenceHandles");
	ms_pahFrameLatencyFenceHandles = rage_new grcFenceHandle[MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU + 1];
	for ( unsigned nIndex=0; nIndex < (MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU + 1); nIndex++ )
	{
		ms_pahFrameLatencyFenceHandles[nIndex] = AllocFence();
	}
#endif // USE_FRAME_LATENCY_LIMITTING_FENCE
}

void grcDevice::GRC_Shutdown() 
{
	ReleaseQueries();

#if USE_FRAME_LATENCY_LIMITTING_FENCE
	for ( unsigned nIndex=0; nIndex < (MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU + 1); nIndex++ )
	{
		CpuFreeFence(ms_pahFrameLatencyFenceHandles[nIndex]);
	}
	Assertf(ms_pahFrameLatencyFenceHandles != NULL, "Wasn't initialized ms_pahFrameLatencyFenceHandles");
	delete[] ms_pahFrameLatencyFenceHandles;
	ms_pahFrameLatencyFenceHandles = NULL;
#endif // USE_FRAME_LATENCY_LIMITTING_FENCE
}

bool grcDevice::IsItfpZBuffer(int format)
{
	return ((format == rage::grctfD32F) || (rage::grctfD32FS8)) ? true : false;
}

bool grcDevice::IsCurrentDepthFormatFpZ()
{
	return IsItfpZBuffer(g_grcDepthFormat);
}

void grcDevice::SetWindow(const grcWindow &window) {
	if (g_grcCurrentContext 
#if RAGE_INSTANCED_TECH
		&& !grcViewport::GetInstancing()
#endif
		)
	{
		D3D11_VIEWPORT *const vp = sm_viewports;
		sm_numViewports = 1;
		u32 width    = window.GetWidth();
		u32 height   = window.GetHeight();
		// DX11 TODO:- Temp work around.
		if( width == 0 )
		{
			width = sm_CurrentWindows[g_RenderThreadIndex].uWidth;
		}
		if( height == 0 )
		{
			height = sm_CurrentWindows[g_RenderThreadIndex].uHeight;
		}

		vp->Width    = (FLOAT)width;
		vp->Height   = (FLOAT)height;
		vp->TopLeftX = (FLOAT)window.GetX();
		vp->TopLeftY = (FLOAT)window.GetY();
		vp->MinDepth = FixViewportDepth(window.GetMinZ());
		vp->MaxDepth = FixViewportDepth(window.GetMaxZ());

		Assert(vp->MinDepth >= 0.0f);
		Assert(vp->MaxDepth <= 1.0f);
		//Assert(vp->MinDepth < vp->MaxDepth);

		g_grcCurrentContext->RSSetViewports(1, (D3D11_VIEWPORT*)vp);

		D3D11_RECT *const rect = sm_scissorRects;
		sm_numScissorRects = 1;
		rect->left = window.GetX();
		rect->right = window.GetX() + width;
		rect->top = window.GetY();
		rect->bottom = window.GetY() + height;

		g_grcCurrentContext->RSSetScissorRects(1, rect);
	}
}

#if DEVICE_EQAA
void grcDevice::SetAACount(u32 nSamp, u32 nFrag, u32 nIter)
{
	D3D11X_MSAA_SCAN_CONVERTER_SETTINGS scanConverterSettings;
	D3D11X_MSAA_EQAA_SETTINGS eqaaSettings;
	CHECK_HRESULT( g_grcCurrentContext->RSGetMSAASettingsForQuality( &scanConverterSettings, &eqaaSettings, NULL, NULL, GetIntLog(nSamp), GetIntLog(nSamp) ) );
		
	eqaaSettings.NumSamplesForAlphaToMaskLog2 = GetIntLog(nSamp);
	eqaaSettings.NumSamplesForMaskExportLog2 = GetIntLog(nSamp);
	eqaaSettings.MaxAnchorSamplesLog2 = GetIntLog(nFrag);
	eqaaSettings.NumSamplesForPSIterationLog2 = GetIntLog(nIter);

	scanConverterSettings.NumSamplesMsaaLog2 = GetIntLog(nSamp);
	scanConverterSettings.NumSamplesMsaaExposedToPSLog2 = GetIntLog(nFrag);
				
	g_grcCurrentContext->RSSetScanConverterMSAASettings( &scanConverterSettings );
	g_grcCurrentContext->RSSetEQAASettings( &eqaaSettings );

	SetAALocations(nSamp, nFrag);
}

static void PackLocations(INT8 (*dest)[2], char x0, char y0, char x1, char y1, char x2, char y2, char x3, char y3)
{
	dest[0][0] = x0; dest[0][1] = y0;
	dest[1][0] = x1; dest[1][1] = y1;
	dest[2][0] = x2; dest[2][1] = y2;
	dest[3][0] = x3; dest[3][1] = y3;
}

void grcDevice::SetAALocations(u32 ns, u32 nf)
{
	if (!(nf==1 && ns==4 && eqaa::CENTERED))
		return;	//leaving the defaults

	D3D11X_MSAA_SAMPLE_PRIORITIES priorities = {0};	//Warning: only works for S/1
	D3D11X_MSAA_SAMPLE_POSITIONS positions = {0};

	bool dithered = true;

	if (dithered)
	{
		PackLocations(positions.SampleLocs00, 0,0, +7,+2, -5,+5, -2,-7);
		PackLocations(positions.SampleLocs10, 0,0, -2,+7, -5,-5, +7,-2);
		PackLocations(positions.SampleLocs01, 0,0, +2,-7, +5,+5, -7,+2);
		PackLocations(positions.SampleLocs11, 0,0, -7,-2, +5,-5, +2,+7);
	}else
	{
		// using a triangle pattern, requires a *_Centered resolve shader
		PackLocations(positions.SampleLocs00, 0,0, 3,-7, 1,6, -6,-1);
		memcpy(positions.SampleLocs01, positions.SampleLocs00, sizeof(positions.SampleLocs00));
		memcpy(positions.SampleLocs10, positions.SampleLocs00, sizeof(positions.SampleLocs00));
		memcpy(positions.SampleLocs11, positions.SampleLocs00, sizeof(positions.SampleLocs00));
	}

	g_grcCurrentContext->RSSetSamplePositions( &priorities, &positions );
}
#endif // DEVICE_EQAA

#if RAGE_INSTANCED_TECH
void grcDevice::SetMultiWindow(const grcWindow* InstWindows[], u32 uNumVPInst)
{
	D3D11_VIEWPORT *const vp = sm_viewports;
	D3D11_RECT *const rect = sm_scissorRects;
	sm_numViewports = uNumVPInst;
	sm_numScissorRects = uNumVPInst;

	for (u32 i = 0; i < uNumVPInst; i++)
	{
		vp[i].Width = (FLOAT)InstWindows[i]->GetWidth();
		vp[i].Height   = (FLOAT)InstWindows[i]->GetHeight();
		vp[i].TopLeftX = (FLOAT)InstWindows[i]->GetX();
		vp[i].TopLeftY = (FLOAT)InstWindows[i]->GetY();
		vp[i].MinDepth = FixViewportDepth(InstWindows[i]->GetMinZ());
		vp[i].MaxDepth = FixViewportDepth(InstWindows[i]->GetMaxZ());

		// DX11 TODO:- Temp work around.
		if( vp[i].Width == 0.0f )
		{
			vp[i].Width = (FLOAT)sm_CurrentWindows[g_RenderThreadIndex].uWidth;
		}
		if( vp[i].Height == 0.0f )
		{
			vp[i].Height = (FLOAT)sm_CurrentWindows[g_RenderThreadIndex].uHeight;
		}
		Assert(vp[i].MinDepth >= 0.0f);
		Assert(vp[i].MaxDepth <= 1.0f);
		//Assert(vp[i].MinDepth < vp[i].MaxDepth);

		rect[i].top = (long)0;//g_InstViewport[1].Window.GetYf();//(long)vp[i].TopLeftY;
		rect[i].left = (long)0;//g_InstViewport[1].Window.GetXf();//(long)vp[i].TopLeftX;
		rect[i].bottom = (long)8192;//g_InstViewport[1].Window.GetZf();//(long)vp[i].Height;
		rect[i].right = (long)8192;//g_InstViewport[1].Window.GetWf();//(long)vp[i].Width;
	}

	g_grcCurrentContext->RSSetViewports(uNumVPInst, vp);
	g_grcCurrentContext->RSSetScissorRects(uNumVPInst, rect);
}
#endif

#if RSG_PC
static const DXGI_ADAPTER_DESC& GetAdapterDescriptor()
{
	const grcAdapterD3D11 *const pAdapter = (const grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(GRCDEVICE.GetAdapterOrdinal());
	AssertMsg(pAdapter, "Could not get the current adapter");

	IDXGIAdapter *const pDeviceAdapter = pAdapter->GetDeviceAdapter();
	AssertMsg(pDeviceAdapter, "Could not get the current device adapter");

	static DXGI_ADAPTER_DESC oAdapterDesc;
	pDeviceAdapter->GetDesc(&oAdapterDesc);
	return oAdapterDesc;
}
#endif // RSG_PC

static bool CheckReadOnlyDepth()
{
	if (GRCDEVICE.GetDxFeatureLevel() < 1100)
	{
		return false;
	}

#if RSG_PC
	// Intel doesn't support read-only depth/stencil at least up to 4th generation (Haswell)
	// It does a copy instead, on each draw call that needs it, which appears extra slow
	if (GRCDEVICE.GetManufacturer() == INTEL)
	{
		return false;
	}
	// There is a bug in ATI screwing up the deferred lighting pass
	// Reported to only affect Radeon 4000/5000/6000/7000(fusion) series
	const wchar_t prefix[] = L"adeon HD ";
	const wchar_t *const sHD = wcsstr( GetAdapterDescriptor().Description, prefix );
	if (sHD)
	{
		const wchar_t c = sHD[sizeof(prefix)/sizeof(prefix[0])-1];
		if (c>=L'5' && c<=L'6')
		{
			return false;
		}
	}
#endif // RSG_PC
	
	return true;
}

#if RSG_PC
void grcDevice::GetAdapterDescription(DXGI_ADAPTER_DESC &oAdapterDesc)
{
	IDXGIAdapter* pDeviceAdapter = ((const grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(GetAdapterOrdinal()))->GetDeviceAdapter();
	AssertMsg(pDeviceAdapter, "Could not get the current device adapter");
	if (pDeviceAdapter)
		pDeviceAdapter->GetDesc(&oAdapterDesc);
}


void grcDevice::InitAdapterOrdinal()
{
	if (sm_AdapterOrdinal != -1) return; 

	sm_AdapterOrdinal = 0;
	PARAM_adapter.Get(sm_AdapterOrdinal);
	if (sm_AdapterOrdinal < 0 || sm_AdapterOrdinal >= grcAdapterManager::GetInstance()->GetAdapterCount()) {
		grcErrorf("Invalid adapter ordinal specified, using first adapter");
		sm_AdapterOrdinal = 0;
	}

	PARAM_outputMonitor.Get(sm_OutputMonitor);
	const grcAdapterD3D11* adapterTest = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(sm_AdapterOrdinal);
	if (sm_OutputMonitor < 0 || sm_OutputMonitor >= adapterTest->GetOutputCount()) {
		grcErrorf("Invalid adapter monitor output specified, using first monitor output");
		sm_OutputMonitor = 0;
	}

	//Don't know that this is needed, but allows you to ignore how many adapters (graphics cards) are in a machine and just number the monitors
	int monitorIndex = 0;
	if (PARAM_monitor.Get(monitorIndex))
	{
		if (monitorIndex >= 0) {
			int outputIndexTotal = 0;
			for (int adapterIndex = 0; adapterIndex < grcAdapterManager::GetInstance()->GetAdapterCount(); adapterIndex++)
			{
				const grcAdapterD3D11* adapter = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(adapterIndex);
				if ((adapter->GetOutputCount() + outputIndexTotal) > monitorIndex)
				{
					sm_AdapterOrdinal = adapterIndex;
					sm_OutputMonitor = monitorIndex - outputIndexTotal;
					break;
				}
				outputIndexTotal += adapter->GetOutputCount();
			}
		}
	}
}
#endif // RSG_PC

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
#else // DEVICE_EQAA
	sm_MSAA = MSAAMode(sampleCount);
#endif // DEVICE_EQAA
}

#if RSG_DURANGO
#define S_COMMON_BUFFER_IB_SIZE         (256*1024)
#define S_COMMON_BUFFER_VB_SIZE         (4*1024*1024)
#define S_COMMON_BUFFER_CB_SIZE         (65536)
#define S_COMMON_BUFFER_UAV_SIZE        (4*1024*1024)
#define S_COMMON_BUFFER_SRV_SIZE        S_COMMON_BUFFER_UAV_SIZE

static ID3D11Buffer *s_CommonBufferIB, *s_CommonBufferVB;
ID3D11Buffer *s_CommonBufferCB;
atRangeArray<ID3D11Buffer *, 512> s_CommonFullAddressPlacementBuffers;	//GPU has 40-bit addressable memory, max buffer size is 4gig, to avoid issues on boundaries, we'll access on 2-gig offsets = 512 buffers
D3D11X_DESCRIPTOR_UNORDERED_ACCESS_VIEW s_CommonBufferUAV;
D3D11X_DESCRIPTOR_SHADER_RESOURCE_VIEW  s_CommonBufferSRV;

D3D11X_DESCRIPTOR_UNORDERED_ACCESS_VIEW s_CommonStructuredBufferUAV;
D3D11X_DESCRIPTOR_SHADER_RESOURCE_VIEW  s_CommonStructuredBufferSRV;
#endif

static void (D3DAPI* s_PrevImmediateContextClearState)(_Inout_ ID3D11DeviceContext* pDeviceContext);
void (D3DAPI HookClearState)(_Inout_ ID3D11DeviceContext* pDeviceContext)
{
	__debugbreak();
	s_PrevImmediateContextClearState(pDeviceContext);
}

// Windows 10 Shore functions 
typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

typedef HRESULT(WINAPI* LPSetProcessDpiAwareness)(PROCESS_DPI_AWARENESS value);
LPSetProcessDpiAwareness fnSetProcessDpiAwareness = nullptr;

void grcDevice::InitClass(bool inWindow, bool topMost/* =false */)
{
#if RSG_PC
	static HINSTANCE hShCore = LoadLibrary("Shcore.dll");
	if (hShCore)
	{
		fnSetProcessDpiAwareness = (LPSetProcessDpiAwareness)GetProcAddress(hShCore, "SetProcessDpiAwareness");
		if (fnSetProcessDpiAwareness)
			fnSetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	}
#endif

#if RSG_DURANGO && _XDK_VER <= 10812
	OutputDebugString("Doing dummy XMemAllocDefault calls to get around thread bug in OS\n");
	XALLOC_ATTRIBUTES attr = {};
	void *ptrs[XALLOC_MEMTYPE_PHYSICAL_UNCACHED+1][2];
	for (unsigned i=XALLOC_MEMTYPE_HEAP_CACHEABLE; i<=XALLOC_MEMTYPE_PHYSICAL_UNCACHED; i++) {
		attr.s.dwMemoryType = i;
		ptrs[i][0] = XMemAllocDefault(1, attr.dwAttributes);
	}
	for (unsigned i=XALLOC_MEMTYPE_HEAP_CACHEABLE; i<=XALLOC_MEMTYPE_PHYSICAL_UNCACHED; i++) {
		attr.s.dwMemoryType = i;
		ptrs[i][1] = XMemAllocDefault(4097, attr.dwAttributes);
	}
	for (unsigned i=XALLOC_MEMTYPE_HEAP_CACHEABLE; i<=XALLOC_MEMTYPE_PHYSICAL_UNCACHED; i++) {
		attr.s.dwMemoryType = i;
		XMemFree(ptrs[i][0], attr.dwAttributes);
		XMemFree(ptrs[i][1], attr.dwAttributes);
	}
#endif

	if (sm_CurrentWindows[g_RenderThreadIndex].uWidth <= 0)
	{
		sm_CurrentWindows[g_RenderThreadIndex].Init(DEFAULT_WINDOW_WIDTH,DEFAULT_WINDOW_HEIGHT,0,0,1);
	}
	sm_Owner = sm_CreationOwner = sysIpcGetCurrentThreadId();
	sm_Controller = sysIpcCreateSema(true);

	s_ContextLock = sysIpcCreateMutex();
#if NV_SUPPORT
	s_NVStereoLock = sysIpcCreateMutex();
#endif

	PARAM_width.Get(sm_CurrentWindows[g_RenderThreadIndex].uWidth);
	PARAM_height.Get(sm_CurrentWindows[g_RenderThreadIndex].uHeight);
#if RSG_DURANGO
	sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex];
#endif
	// Set the defaults for the multisample count, quality (prolly to be overridden by the game data later.
#if DEVICE_EQAA
	if (PARAM_multiSample.Get() || PARAM_multiFragment.Get())
	{
		u32 sampleCount=0, fragmentCount=0;
		PARAM_multiSample.Get(sampleCount);
		PARAM_multiFragment.Get(fragmentCount);
		SetSamplesAndFragments(sampleCount, fragmentCount);
	}
#else
	if (PARAM_multiSample.Get())
	{
		u32 sampleCount=0;
		PARAM_multiSample.Get(sampleCount);
		SetSamplesAndFragments(sampleCount);
		PARAM_multiSampleQuality.Get(sm_MultisampleQuality);
	}
#endif

	if (PARAM_noWaitOnGpu.Get())
		s_blockOnFence = false;

#if __WIN32PC
#if !__FINAL && __WIN32PC
	sm_BlockOnLostFocus = !PARAM_noBlockOnLostFocus.Get();
#else
	if (!inWindow)
	{
		sm_CurrentWindows[g_RenderThreadIndex].uWidth = GetSystemMetrics(SM_CXSCREEN);
		sm_CurrentWindows[g_RenderThreadIndex].uHeight = GetSystemMetrics(SM_CYSCREEN);
	}
#endif // !__FINAL

	g_inWindow = inWindow;
	g_isTopMost = topMost;

#if __FINAL
	int iScreenWidth = 0;
	int iScreenHeight = 0;
	if (!inWindow)
	{
		// Default to desktop resolution.
		iScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
		iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	}

	if ((iScreenWidth > 0) && (iScreenHeight > 0))
	{
		sm_CurrentWindows[g_RenderThreadIndex].uWidth  = iScreenWidth;
		sm_CurrentWindows[g_RenderThreadIndex].uHeight = iScreenHeight;
		sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate = 0;
	}
#endif // __FINAL
	if (PARAM_refreshrate.Get())
	{
		u32 uRate = 0;
		PARAM_refreshrate.Get(uRate);
		sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate = uRate;
	}

	sm_CurrentWindows[g_RenderThreadIndex].bFullscreen = !inWindow;

	InitAdapterOrdinal();

	if (PARAM_nvstereo.Get())
		PARAM_nvstereo.Get(sm_StereoDesired);

	GRCDEVICE.InitializeStereoSystem(sm_StereoDesired!=0);

	const grcAdapterD3D11* adapter = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(sm_AdapterOrdinal);
	const grcAdapterD3D11Output* monitorOutput = adapter->GetOutput(sm_OutputMonitor);

	if ((!PARAM_rag.Get() && !PARAM_setHwndMain.Get()) || PARAM_ragUseOwnWindow.Get())
	{
		DXGI_OUTPUT_DESC desc;
		unsigned int dpiX, dpiY;
		grcAdapterD3D11Output::GetDesc(adapter->GetHighPart(), adapter->GetLowPart(), monitorOutput, desc, dpiX, dpiY);
		RECT rect = {0,0,sm_CurrentWindows[g_RenderThreadIndex].uWidth,sm_CurrentWindows[g_RenderThreadIndex].uHeight};
		bool fullscreen = PARAM_fullscreen.Get();	// Get this from settings file..

		if (PARAM_width.Get() && PARAM_height.Get())
		{
			int w, h;
			PARAM_width.Get(w);
			PARAM_height.Get(h);
			rect.right = w;
			rect.bottom = h;
		}
		else if (fullscreen)
		{
			rect = desc.DesktopCoordinates;
		}
		u32 width = (u32)(rect.right-rect.left);
		u32 height = (u32)(rect.bottom-rect.top);

		pcdDisplayf("DXGI Output %d on adapter %d: %d x %d (%d,%d -> %d,%d)", sm_OutputMonitor, sm_AdapterOrdinal,
			width,
			height,
			rect.left, rect.top,
			rect.right, rect.bottom);

		sm_CurrentWindows[g_RenderThreadIndex].uWidth = width;
		sm_CurrentWindows[g_RenderThreadIndex].uHeight = height;


		if (sm_DesiredWindow.uWidth != 0 && sm_DesiredWindow.uHeight != 0)
		{
			sm_CurrentWindows[g_RenderThreadIndex] = sm_DesiredWindow;
			g_inWindow = !sm_DesiredWindow.bFullscreen;
		}

		const DXGI_MODE_DESC& oModeDesc = monitorOutput->GetClosestMode(sm_CurrentWindows[g_RenderThreadIndex]);
		g_SwapDesc.BufferDesc = oModeDesc;
		if (fullscreen)
		{
			sm_CurrentWindows[g_RenderThreadIndex].uWidth = oModeDesc.Width;
			sm_CurrentWindows[g_RenderThreadIndex].uHeight = oModeDesc.Height;
		}
		sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate = oModeDesc.RefreshRate;

		sm_DesiredWindow = sm_CurrentWindows[g_RenderThreadIndex];
		pcdDisplayf("Closest mode is %d x %d", oModeDesc.Width, oModeDesc.Height);
	}

	sm_FullscreenWindow = sm_CurrentWindows[g_RenderThreadIndex];
	sm_FullscreenWindow.bFullscreen = true;

	sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex];

	if (sm_CurrentWindows[g_RenderThreadIndex].bFullscreen) SetDesireBorderless(false);

	if (PARAM_width.Get() || PARAM_height.Get()) SetIngoreWindowLimits(true);

#if !__FINAL
	PARAM_setHwndMain.Get((int&)g_hwndMain);
#endif // !__FINAL
	if (!g_hwndMain || PARAM_ragUseOwnWindow.Get()) {
		g_hwndMain = CreateDeviceWindow(g_hwndParent);
		s_WeCreatedHwndMain = true;
		SetupCursor(ioMouse::GetAbsoluteOnly() && IsWindowed());
	}
	else
	{
		// We have been passed a hwnd (in g_hwndMain) from rag to display in. The swap chain can only work in a window owned by this process.
		// To get around this issue and display inside rag itself using dx11 we create a new window inside the passed hwnd,
		// this way the window is owned by this application and displayed inside rag. We no longer care about rags hwnd.
		g_hwndMain = CreateDeviceWindow(g_hwndMain);
		s_WeCreatedHwndMain = true;
	}

	RegisterSessionNotification();

#if !__FINAL
	if (PARAM_gameWindowFocusLock.Get())
#endif
	{
		LockSetForegroundWindow(LSFW_LOCK);
	}

	UpdatePresentParameters();

	unsigned int uCreationFlags = 0;
#if !__FINAL
	if (PARAM_debugRuntime.Get())
	{
		uCreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
		uCreationFlags |= D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;
	}
#endif // __FINAL

	if (PARAM_singleThreaded.Get())
	{
		uCreationFlags |= D3D11_CREATE_DEVICE_SINGLETHREADED;
	}

	if (!g_D3D11CreateDeviceAndSwapChain)
	{
		Quitf(ERR_GFX_D3D_NOD3D1X_2,"Couldn't find D3D11CreateDeviceAndSwapChain entry point.");
	}

	D3D_FEATURE_LEVEL featureLevels[] = { 
#if __D3D11_1
		D3D_FEATURE_LEVEL_11_1, 
#endif // __D3D11_1		
		D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

	u32 uNumFeatureLevels = NELEM(featureLevels);
	D3D_FEATURE_LEVEL chosenFeatureLevel;

	// DX10 cards don't work right now for runtime. Will potentially want to renable at a later point for PC.
#if !__TOOL && !__RESOURCECOMPILER && !RSG_PC
	featureLevels[0]  = D3D_FEATURE_LEVEL_11_0;
	uNumFeatureLevels = 1;

	grcAssertf(!PARAM_DX10.Get() && !PARAM_DX10_1.Get(), "Error - dx10.0 or 10.1 specified on commandline which are currently unsupported. Ignoring.");
#else
	if (PARAM_DX11.Get() && uNumFeatureLevels >= 3)
	{
		featureLevels[1]  = D3D_FEATURE_LEVEL_10_1;
		featureLevels[2]  = D3D_FEATURE_LEVEL_10_0;
		uNumFeatureLevels = 3;
	}
	if (PARAM_DX10_1.Get() && uNumFeatureLevels >= 2)
	{
		featureLevels[0]  = D3D_FEATURE_LEVEL_10_1;
		featureLevels[1]  = D3D_FEATURE_LEVEL_10_0;
		uNumFeatureLevels = 2;
	}
	else if (PARAM_DX10.Get() && uNumFeatureLevels >= 1)
	{
		featureLevels[0]  = D3D_FEATURE_LEVEL_10_0;
		uNumFeatureLevels = 1;
	}
#endif // !__TOOL && !__RESOURCECOMPILER

#if RSG_PC
	if(GetManufacturer() == INTEL)
	{
		const wchar_t prefix[] = L"Intel(R) HD Graphics ";
		const wchar_t *const sHD = wcsstr( GetAdapterDescriptor().Description, prefix );
		if (sHD)
		{
			size_t c = sizeof(prefix)/sizeof(prefix[0])-1;
			int version = (sHD[c]-'0')*10 + (sHD[c+1]-'0');
			if (version <= 40)
			{
				featureLevels[0]  = D3D_FEATURE_LEVEL_10_0;
				uNumFeatureLevels = 1;
			}
		}
	}
#endif

	s_SwapChainLock.Lock();
	HRESULT res;
	D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
	if (PARAM_nullDriver.Get())
	{
		sm_DesiredWindow.bFullscreen = false;
		sm_DesiredWindow.uHeight = 100;
		sm_DesiredWindow.uWidth = 100;
		driverType = D3D_DRIVER_TYPE_NULL;
		res =  g_D3D11CreateDeviceAndSwapChain( NULL, driverType, NULL, uCreationFlags, featureLevels, uNumFeatureLevels, D3D11_SDK_VERSION, &g_SwapDesc, ((IDXGISwapChain**)&sm_pSwapChain), &g_pd3dDevice, &chosenFeatureLevel, &g_pd3dDeviceContext );
		s_BackupSwapChain = sm_pSwapChain;
	}
	else
	{
		IDXGIAdapter* dxgiAdapter = adapter->GetDeviceAdapter();
		DXGI_ADAPTER_DESC oDesc;
		dxgiAdapter->GetDesc(&oDesc);
		res =  g_D3D11CreateDeviceAndSwapChain(dxgiAdapter , D3D_DRIVER_TYPE_UNKNOWN, NULL,
			uCreationFlags,  featureLevels, uNumFeatureLevels, D3D11_SDK_VERSION, &g_SwapDesc, ((IDXGISwapChain**)&sm_pSwapChain), &g_pd3dDevice, &chosenFeatureLevel, &g_pd3dDeviceContext );
		s_BackupSwapChain = sm_pSwapChain;
	}
	s_SwapChainLock.Unlock();

	if( FAILED(res ) )
	{
		Quitf(ERR_GFX_D3D_SWAPCHAIN_ALLOC_2,"Unable to create D3D11 device and swap chain: error:%u. Do you have a d3d10/d3d11 compatible graphics card?",res);
	}

	{	// Get back the actual swap chain that DXGI decided to create
		DXGI_SWAP_CHAIN_DESC oDesc;
		res = static_cast<IDXGISwapChain*>(sm_pSwapChain)->GetDesc(&oDesc);
		if(!FAILED(res))
		{
			// the refresh rate is garbage here for some reason
			//const RefreshRate rate = { oDesc.BufferDesc.RefreshRate.Numerator, oDesc.BufferDesc.RefreshRate.Denominator };
			//sm_GlobalWindow.uRefreshRate = rate;
			sm_GlobalWindow.uWidth =  oDesc.BufferDesc.Width;
			sm_GlobalWindow.uHeight = oDesc.BufferDesc.Height;
			pcdDisplayf("Actual swap chain size %d x %d", oDesc.BufferDesc.Width, oDesc.BufferDesc.Height);

			if (!oDesc.Windowed && (oDesc.BufferDesc.Width != sm_DesiredWindow.uWidth || oDesc.BufferDesc.Height != sm_DesiredWindow.uHeight))
			{
				pcdDisplayf("Found inconsistency in the returned swap chain versus the desired one");
				SetMatchDesiredWindow(true);
			}
		}
	}

	grcDisplayf("created D3D11 (feature level %x).",chosenFeatureLevel);

#if __WIN32PC
	switch (chosenFeatureLevel)
	{
#if __D3D11_1
		case D3D_FEATURE_LEVEL_11_1:
			SetDxFeatureLevel(11, 1);
			SetDxShaderModel(5,1);
			break;
#endif // __D3D11_1
		case D3D_FEATURE_LEVEL_11_0:
			SetDxFeatureLevel(11);
			SetDxShaderModel(5,0);
			break;
		case D3D_FEATURE_LEVEL_10_1:
			SetDxFeatureLevel(10, 10);
			SetDxShaderModel(4,1);
			break;
		case D3D_FEATURE_LEVEL_10_0:
			SetDxFeatureLevel(10);
			SetDxShaderModel(4,0);
			break;
		default:
			Assertf(false, "Unknown feature set level %d", chosenFeatureLevel);
			Quitf(ERR_GFX_D3D_NOFEATURELEVEL_1,"Unknown feature set level");
			break;
	}
#endif

	s_Device11Wrapper.m_Inner = g_pd3dDevice;
	sm_Current = &s_Device11Wrapper;

	s_DeviceContext11Wrapper.m_Inner = g_pd3dDeviceContext;
	g_grcCurrentContext = &s_DeviceContext11Wrapper;
	SetReleasing(false);

//#if __DEV
//    g_bClearStateUponBeginCommandList = true;
//    g_bClearStateUponFinishCommandList = true;
//    g_bClearStateUponExecuteCommandList = true;
//#endif

	D3D11_FEATURE_DATA_THREADING ho;
	grcDevice::Result uReturnCode = sm_Current->CheckFeatureSupport(D3D11_FEATURE_THREADING, &ho, sizeof(ho));
	Assert(uReturnCode == S_OK);
	if (uReturnCode == S_OK)
	{
		if (ho.DriverCommandLists)
			sm_uFeatures |= MT_COMMANDLIST;
		if (ho.DriverConcurrentCreates)
			sm_uFeatures |= MT_CONCURRENTCREATE;
	}

	D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS oComputeShaders;
	uReturnCode = sm_Current->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &oComputeShaders, sizeof(oComputeShaders));
	Assert(uReturnCode == S_OK);
	if (uReturnCode == S_OK)
	{
		if (oComputeShaders.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
		{
			sm_uFeatures |= COMPUTE_SHADER_40;
			if (GetDxFeatureLevel() >= 1100)
			{
				sm_uFeatures |= COMPUTE_SHADER_50;
			}
		}
	}

	// Multi-GPU
	sm_uFeatures |= (GetGPUCount() > 1) ? MULTIGPU : 0;

#if DIRECT_CONTROL_OF_ALT_ENTER
	SuppressAltEnter();
#endif

#elif RSG_DURANGO

	topMost;
	inWindow;
	UINT creationFlags = D3D11_CREATE_DEVICE_FAST_KICKOFFS | D3D11_CREATE_DEVICE_MANUAL_GARBAGE_COLLECTION /* | D3D11_CREATE_DEVICE_BGRA_SUPPORT*/;

#if !__FINAL
#if __D3D11_MONO_DRIVER
	u32 debugRuntime = ENABLE_PIX_TAGGING ? 1 : 0;

	PARAM_monoRuntime.Get(debugRuntime);

	if (PARAM_noFastKickoffs.Get())
	{
		creationFlags = creationFlags & ~D3D11_CREATE_DEVICE_FAST_KICKOFFS;
	}

	if (debugRuntime==1)
		creationFlags |= D3D11_CREATE_DEVICE_INSTRUMENTED;
	else if (debugRuntime==2)
		creationFlags |= D3D11_CREATE_DEVICE_VALIDATED;
	else if (debugRuntime==3)
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#else // __D3D11_MONO_DRIVER
    creationFlags |= D3D11_CREATE_DEVICE_PIX_PROFILING;
#endif // __D3D11_MONO_DRIVER

	if (PARAM_debugRuntime.Get())
	{
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif // !__FINAL


	D3D11X_CREATE_DEVICE_PARAMETERS createParams = { 
		D3D11_SDK_VERSION, 
		creationFlags
	};
    // This array defines the ordering of feature levels that D3D should attempt to create.
	HRESULT hr = D3D11XCreateDeviceX(&createParams,&g_pd3dDevice,(ID3D11DeviceContextX**)&g_pd3dDeviceContext);
    if (FAILED(hr))
			Quitf("Unable to create D3D device, hresult 0x%08x", hr);

	// Unlock the extra GPU context. This will be unlocked by default in the July 2014 XDK. Prior to that you have to opt-in to get it. 
	// This provides a small but measurable performance gain in V-NG (0.1ms). See B*1885731 for details. You can simply remove this code
	// once the July 2014 XDK is the default.
	const u32 DURANGO_EXTRA_GPU_CONTEXT_HINT = 0xB;
	const bool bEnableExtraGPUContext = true;
	g_pd3dDevice->SetDriverHint( DURANGO_EXTRA_GPU_CONTEXT_HINT, bEnableExtraGPUContext ? 1 : 0 );

	// Create 2 DMA contexts
	{
		D3D11_DMA_ENGINE_CONTEXT_DESC desc = {};
		desc.RingBufferSizeBytes = 64 * 1024;
		desc.SegmentSizeBytes = 16 * 1024;
		desc.CreateFlags = D3D11_DMA_ENGINE_CONTEXT_CREATE_SDMA_1;
		g_pd3dDevice->CreateDmaEngineContext(&desc, &sm_DmaEngineContext1);
		desc.CreateFlags = D3D11_DMA_ENGINE_CONTEXT_CREATE_SDMA_2;
		g_pd3dDevice->CreateDmaEngineContext(&desc, &sm_DmaEngineContext2);
	}

	D3D11_BUFFER_DESC vbufDesc = { S_COMMON_BUFFER_VB_SIZE, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER , 0, 0, 0, 0, 0 };
	CHECK_HRESULT(g_pd3dDevice->CreatePlacementBuffer(&vbufDesc, (void*)FIXED_PLACEMENT_BASE, &s_CommonBufferVB));
	D3D11_BUFFER_DESC ibufDesc = { S_COMMON_BUFFER_IB_SIZE, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER , 0, 0, 0, 0, 0 };
	CHECK_HRESULT(g_pd3dDevice->CreatePlacementBuffer(&ibufDesc, (void*)FIXED_PLACEMENT_BASE, &s_CommonBufferIB));
	// You CANNOT combine D3D11_BIND_CONSTANT_BUFFER with any other flags.  It won't flag an error, but it won't work either.
	D3D11_BUFFER_DESC cbufDesc = { S_COMMON_BUFFER_CB_SIZE, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER , 0, 0, 0, 0, 0 };
	CHECK_HRESULT(g_pd3dDevice->CreatePlacementBuffer(&cbufDesc, (void*)FIXED_PLACEMENT_BASE, &s_CommonBufferCB));

	//Fully addressable placement buffers.
	static const u64 s2GB = 1ull << 31;
	static const u32 sMaxBufferSize = 0xFFFFFFFF; //4gb-1b = Max
	D3D11_BUFFER_DESC indirectArgBufDesc = { sMaxBufferSize, D3D11_USAGE_DEFAULT, 0, 0, D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS, 0, 0, 0 };
	for(u64 i = 0; i < s_CommonFullAddressPlacementBuffers.size(); ++i)
		CHECK_HRESULT(g_pd3dDevice->CreatePlacementBuffer(&indirectArgBufDesc, reinterpret_cast<void *>(s2GB * i), &(s_CommonFullAddressPlacementBuffers[i])));

	D3D11_BUFFER_DESC uavBufDesc = { S_COMMON_BUFFER_UAV_SIZE, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS, 0, 0, 0 };
	ID3D11Buffer *uavBuf;
	CHECK_HRESULT(g_pd3dDevice->CreatePlacementBuffer(&uavBufDesc, (void*)FIXED_PLACEMENT_BASE, &uavBuf));
	D3D11X_DESCRIPTOR_RESOURCE uavBufDescRes;
	uavBuf->GetDescriptor(&uavBufDescRes);
	D3D11X_RESOURCE_VIEW_DESC uavBufViewDesc;
	uavBufViewDesc.Flags                = D3D11X_RESOURCE_VIEW_SET_ADDRESS_BASE;
	uavBufViewDesc.AddressOffset        = 0;
	uavBufViewDesc.Format.DataFormat    = D3D11X_DATA_FORMAT_32;
	uavBufViewDesc.Format.NumberFormat  = D3D11X_NUMBER_FORMAT_COMPOSITE_TYPELESS__TYPELESS;
	uavBufViewDesc.Swizzle[0]           = D3D11X_SWIZZLE_X;
	uavBufViewDesc.Swizzle[1]           = D3D11X_SWIZZLE_0;
	uavBufViewDesc.Swizzle[2]           = D3D11X_SWIZZLE_0;
	uavBufViewDesc.Swizzle[3]           = D3D11X_SWIZZLE_0;
	uavBufViewDesc.MemoryType           = 0;
	uavBufViewDesc.ViewDimension        = D3D11X_VIEW_DIMENSION_BUFFER;
	uavBufViewDesc.Buffer.Flags         = D3D11X_BUFFER_VIEW_RAW;
	uavBufViewDesc.Buffer.FirstElement  = 0;
	uavBufViewDesc.Buffer.NumElements   = (UINT)-1;
	// DX ERROR: ID3D11DeviceX::ComposeUnorderedAccessView: When creating a RAW Unordered Access View, the format must be 32_TYPELESS. [ STATE_CREATION ERROR #2097343: ]
	g_pd3dDevice->ComposeShaderResourceView (&uavBufDescRes, &uavBufViewDesc, &s_CommonBufferSRV);
	g_pd3dDevice->ComposeUnorderedAccessView(&uavBufDescRes, &uavBufViewDesc, &s_CommonBufferUAV);
	uavBuf->Release();

	D3D11_BUFFER_DESC uavStructuredBufDesc = { S_COMMON_BUFFER_UAV_SIZE, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, 4, 0, 0 };
	CHECK_HRESULT(g_pd3dDevice->CreatePlacementBuffer(&uavStructuredBufDesc, (void*)FIXED_PLACEMENT_BASE, &uavBuf));
	uavBuf->GetDescriptor(&uavBufDescRes);
	uavBufViewDesc.Format.DataFormat    = D3D11X_DATA_FORMAT_INVALID;
	uavBufViewDesc.Format.NumberFormat  = D3D11X_NUMBER_FORMAT_INVALID;
	uavBufViewDesc.Buffer.Flags         = 0;
	g_pd3dDevice->ComposeShaderResourceView (&uavBufDescRes, &uavBufViewDesc, &s_CommonStructuredBufferSRV);	//Necessary?
	g_pd3dDevice->ComposeUnorderedAccessView(&uavBufDescRes, &uavBufViewDesc, &s_CommonStructuredBufferUAV);
	uavBuf->Release();

    g_SwapDesc.Width = sm_CurrentWindows[g_RenderThreadIndex].uWidth;
    g_SwapDesc.Height = sm_CurrentWindows[g_RenderThreadIndex].uHeight;
    g_SwapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;          // xswaplib wants the channels the wrong way around for some reason (not really, that makes it inconsistent with target view)
    g_SwapDesc.Stereo = false; 
    g_SwapDesc.SampleDesc.Count = 1;						// "Multisampled swap chains are unsupported"
    g_SwapDesc.SampleDesc.Quality = 0;
    g_SwapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
    g_SwapDesc.BufferCount = 3; // 1 front buffer and 2 back buffers                              
    g_SwapDesc.Scaling = DXGI_SCALING_STRETCH;
	g_SwapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    g_SwapDesc.Flags = DXGIX_SWAP_CHAIN_FLAG_QUANTIZATION_RGB_FULL;

	IDXGIDevice1* pdxgiDevice;
	g_pd3dDevice->QueryInterface(__uuidof( pdxgiDevice ), reinterpret_cast< void** >( &pdxgiDevice ) );

	IDXGIAdapter* pdxgiAdapter;
	pdxgiDevice->GetAdapter( &pdxgiAdapter );

	IDXGIFactory2* pdxgiFactory;
	pdxgiAdapter->GetParent( __uuidof( pdxgiFactory ), reinterpret_cast< void** >( &pdxgiFactory ) );

	s_SwapChainLock.Lock();
	pdxgiFactory->CreateSwapChainForCoreWindow(g_pd3dDevice, GetWindowDeviceForThread(), &g_SwapDesc, NULL, (IDXGISwapChain1**)&sm_pSwapChain);
	s_BackupSwapChain = sm_pSwapChain;
	s_SwapChainLock.Unlock();

	// Use the wrappers even on Durango for now to keep the code consistent
#if DEVICE_USE_D3D_WRAPPER
	s_Device11Wrapper.m_Inner = g_pd3dDevice;
	sm_Current = &s_Device11Wrapper;

	s_DeviceContext11Wrapper.m_Inner = g_pd3dDeviceContext;
	g_grcCurrentContext = &s_DeviceContext11Wrapper;
#else // DEVICE_USE_D3D_WRAPPER
	// Monolithic driver methods are not virtual - can't be overridden iby wrapper
	sm_Current = g_pd3dDevice;

	g_grcCurrentContext = g_pd3dDeviceContext;
#endif // !DEVICE_USE_D3D_WRAPPER
	sm_PlmResume = sysIpcCreateSema(false);
	sm_Delegate.Bind(&grcDevice::HandlePlmChange);
	g_SysService.AddDelegate(&sm_Delegate);
#endif // RSG_DURANGO

	// m_pRenderState = &m_aRenderState[0];
	// m_pSamplerState =(Ptr2Dim)(m_aSamplerState);
	// Create viewport.
	D3D11_VIEWPORT *const vp = sm_viewports;
	sm_numViewports = 1;
	vp->Width    = (FLOAT)sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	vp->Height   = (FLOAT)sm_CurrentWindows[g_RenderThreadIndex].uHeight;
	vp->TopLeftX = (FLOAT)0;
	vp->TopLeftY = (FLOAT)0;
	vp->MinDepth = FixViewportDepth(0.0f);
	vp->MaxDepth = FixViewportDepth(1.0f);
	g_grcCurrentContext->RSSetViewports(1, vp);

#if DEVICE_EQAA
	const MSAAMode mode = GetMSAA();
	SetAACount(mode.m_uSamples, mode.m_uFragments, mode.m_uFragments);
#endif

#if __WIN32PC
	// Update monitor configuration
	sm_MonitorConfig.queryConfiguration();
#endif

	g_WindowWidth = sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	g_WindowHeight = sm_CurrentWindows[g_RenderThreadIndex].uHeight;
	g_InvWindowWidth = 1.0f / float(sm_CurrentWindows[g_RenderThreadIndex].uWidth);
	g_InvWindowHeight = 1.0f / float(sm_CurrentWindows[g_RenderThreadIndex].uHeight);

	for (u32 uIndex = 0; uIndex < 4; uIndex++)
	{
		sm_aRTView[uIndex].pRTView = NULL;
		sm_aRTView[uIndex].pResource = NULL;
	}

	sm_numTargets = 0;

	for (u32 uIndex = 0; uIndex < (u32)MAX_RAGE_VERTEXBUFFER_SLOTS; uIndex++)
	{
		ClearStreamSource(uIndex);
	}

	sm_DepthView.pRTView = NULL;
	sm_DepthView.pResource = NULL;
	sm_PreviousDepthView.pRTView = NULL;
	sm_PreviousDepthView.pResource = NULL;
	
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

	s_UseReadOnlyDepth = CheckReadOnlyDepth();
	Displayf("Read-only depth support: %s", s_UseReadOnlyDepth?"yes":"no");

	{
		static grcVertexElement blitElements[] =
		{
#if __XENON
			// Stream, type,  channel,     size, format (rage)
			// Stream, usage, usage index, size, type   (D3D)
			grcVertexElement(0, grcVertexElement::grcvetPosition,	0, 16, grcFvf::grcdsFloat3),
#else
			grcVertexElement(0, grcVertexElement::grcvetPositionT,	0, 16, grcFvf::grcdsFloat4),
#endif
			grcVertexElement(0, grcVertexElement::grcvetColor,		0, 4, grcFvf::grcdsColor),
#if !__XENON
			grcVertexElement(0, grcVertexElement::grcvetColor,		1, 4, grcFvf::grcdsColor),
#endif
			grcVertexElement(0, grcVertexElement::grcvetTexture,	0, 8, grcFvf::grcdsFloat2)
		};

		static grcVertexElement imElements[] =
		{
			grcVertexElement(0, grcVertexElement::grcvetPosition,	0, 12, grcFvf::grcdsFloat3),
			grcVertexElement(0, grcVertexElement::grcvetNormal,		0, 12, grcFvf::grcdsFloat3),
			grcVertexElement(0, grcVertexElement::grcvetColor,		0, 4, grcFvf::grcdsColor),
			grcVertexElement(0, grcVertexElement::grcvetTexture,	0, 8, grcFvf::grcdsFloat2)
		};

		sm_BlitDecl = CreateVertexDeclaration(blitElements, sizeof(blitElements) / sizeof(grcVertexElement), 0);
		sm_ImDecl = CreateVertexDeclaration(imElements, sizeof(imElements) / sizeof(grcVertexElement), 0);
	}


	{
		grcEffect::InitClass();

		g_DefaultEffect = grcEffect::Create(sm_DefaultEffectName);
		if (!g_DefaultEffect)
			Quitf(ERR_GFX_D3D_INIT,"Unable to create default effect '%s', cannot continue.",sm_DefaultEffectName);
		s_DefaultLit = g_DefaultEffect->LookupTechnique("draw");
		s_DefaultUnlit = g_DefaultEffect->LookupTechnique("unlit_draw");
		s_DefaultLitSkinned = g_DefaultEffect->LookupTechnique("drawskinned");
		s_DefaultUnlitSkinned = g_DefaultEffect->LookupTechnique("unlit_drawskinned");
		s_DefaultBlit = g_DefaultEffect->LookupTechnique("blit_draw");
		g_DefaultSampler = g_DefaultEffect->LookupVar("DiffuseTex");
		g_DefaultColor = g_DefaultEffect->LookupVar("GeneralParams0");
		s_DefaultClear = g_DefaultEffect->LookupTechnique("Clear");
	}

	grcViewport::InitClass();

	Blit_Init();
	GRC_Init();
#if FAST_QUAD_SUPPORT
	FastQuad::Init();
#endif

#if RSG_PC && __D3D11
	grcBufferD3D11::InitialiseStagingBufferCache();
#endif // RSG_PC && __D3D11

	if (g_grcCurrentContext)
		DisableScissor();

#if !RSG_DURANGO
	D3D11_BUFFER_DESC oDesc = {0};

#if __BANK
	PARAM_dynamicVBufferSize.Get(s_BeginVerticesBufferSize);
	PARAM_dynamicIBufferSize.Get(s_BeginIndicesBufferSize);
#else
#if RSG_PC
	s_BeginVerticesBufferSize = grcDevice::BEGIN_VERTICES_MAX_SIZE * GRCDEVICE.GetGPUCount();
#endif
#endif


	oDesc.ByteWidth = s_BeginVerticesBufferSize;
	oDesc.Usage = D3D11_USAGE_DYNAMIC;
	oDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	oDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	oDesc.MiscFlags = 0;
	oDesc.StructureByteStride = 0;

	CHECK_HRESULT(sm_Current->CreateBuffer(&oDesc, NULL, &s_BeginVertexBuffer));

	oDesc.ByteWidth = s_BeginIndicesBufferSize;
	oDesc.Usage = D3D11_USAGE_DYNAMIC;
	oDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	oDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	oDesc.MiscFlags = 0;
	oDesc.StructureByteStride = 0;

	CHECK_HRESULT(sm_Current->CreateBuffer(&oDesc, NULL, &s_BeginIndexBuffer));
#endif

	// Create and fill the trifan emulation index buffer
#if RSG_DURANGO
	u16 *const indices = s_BeginVerticesTriFanIndices = (u16*)physical_new(
		s_BeginVerticesTriFanMaxIndices*3*sizeof(u16), __alignof(u16));
#else
	u16 indices[s_BeginVerticesTriFanMaxIndices*3];
#endif
	for (int i=0; i<s_BeginVerticesTriFanMaxIndices; i++) {
		indices[i*3] = 0;
		indices[i*3+1] = u16(i+1);
		indices[i*3+2] = u16(i+2);
	}

#if !RSG_DURANGO
	D3D11_SUBRESOURCE_DATA sData;

	oDesc.ByteWidth = sizeof(indices);
	oDesc.Usage = D3D11_USAGE_IMMUTABLE;
	oDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	oDesc.CPUAccessFlags = 0;
	oDesc.MiscFlags = 0;
	oDesc.StructureByteStride = 0;
	sData.pSysMem = indices;
	sData.SysMemPitch = sData.SysMemSlicePitch = 0;
	CHECK_HRESULT(sm_Current->CreateBuffer(&oDesc,&sData,&s_BeginVerticesTriFanIndices));

	char *zeroes = rage_new char[256];
	memset(zeroes,0,sizeof(zeroes));
	oDesc.ByteWidth = sizeof(zeroes);
	oDesc.Usage = D3D11_USAGE_IMMUTABLE;
	oDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	oDesc.CPUAccessFlags = 0;
	oDesc.MiscFlags = 0;
	oDesc.StructureByteStride = 0;
	sData.pSysMem = zeroes;
	sData.SysMemPitch = sData.SysMemSlicePitch = 0;
	CHECK_HRESULT(sm_Current->CreateBuffer(&oDesc,&sData,&s_MissingInputsVertexBuffer));

	for (u32 uThread = 0; uThread < NUMBER_OF_RENDER_THREADS; uThread++)
	{
		sm_aVertexStreamInfo[uThread].pVertexBuffer[s_MissingInputsVertexBufferStream] = (IUnknown*)s_MissingInputsVertexBuffer;
		sm_aVertexStreamInfo[uThread].offsetInBytes[s_MissingInputsVertexBufferStream] = 0;
		sm_aVertexStreamInfo[uThread].stride[s_MissingInputsVertexBufferStream] = 256;
	}
#endif

#if RSG_DURANGO
	grcGfxContext::init(0x80, 0x40000);
	grcGfxContext::openContext(false, GetAndIncContextSequenceNumber());
#endif

	// Create the depth-stencil states required for ClearRect()
	grcDepthStencilStateDesc depthStencilDesc;

	depthStencilDesc.DepthFunc = grcRSV::CMP_ALWAYS;
	depthStencilDesc.FrontFace.StencilFunc = grcRSV::CMP_ALWAYS;
	depthStencilDesc.BackFace.StencilFunc = grcRSV::CMP_ALWAYS;
	depthStencilDesc.FrontFace.StencilPassOp = grcRSV::STENCILOP_REPLACE;
	depthStencilDesc.BackFace.StencilPassOp = grcRSV::STENCILOP_REPLACE;

	// Clear depth, not stencil
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.StencilEnable = false;
	depthStencilDesc.DepthWriteMask = 1;
	sm_WriteDepthNoStencil = grcStateBlock::CreateDepthStencilState(depthStencilDesc);

	// Clear neither depth, nor stencil
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthWriteMask = 0;
	depthStencilDesc.StencilWriteMask = 0;
	sm_WriteNoDepthNoStencil = grcStateBlock::CreateDepthStencilState(depthStencilDesc);

	// Clear depth and stencil
	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = 1;
	depthStencilDesc.StencilWriteMask = 0xff;
	sm_WriteDepthAndStencil = grcStateBlock::CreateDepthStencilState(depthStencilDesc);

	// Clear stencil, not depth
	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.DepthWriteMask = 0;
	depthStencilDesc.StencilWriteMask = 0xff;
	sm_WriteStencilNoDepth = grcStateBlock::CreateDepthStencilState(depthStencilDesc);

#if USE_NV_SHADOW_LIB
	// TBD: check that the lib version in the header matches the DLL?
	if (GetDxFeatureLevel() >= 1100 && IsUsingVendorAPI())
	{
		NV_ShadowLib_Version shadowLibVersion;
		NV_ShadowLib_GetVersion(&shadowLibVersion);

		ASSERT_ONLY(NV_ShadowLib_Status shadowStatus =) NV_ShadowLib_OpenDX(&shadowLibVersion, &g_shadowContext, g_pd3dDevice, g_pd3dDeviceContext);
		Assert(shadowStatus == NV_ShadowLib_Status_Ok);		// TBD: real error handling.
	}
#endif

#if USE_NV_TXAA
	gfsdk_U32 returnValue = TxaaOpenDX(&g_txaaContext, g_pd3dDevice, g_pd3dDeviceContext);
	SetTXAASupported(returnValue == GFSDK_RETURN_OK);
#endif

#if USE_AMD_SHADOW_LIB
	if (GetDxFeatureLevel() >= 1100)
	{
		// The Initialize call just needs the device to be set. The rest of the 
		// SHADOWS_DESC can be filled in later.
		g_amdShadowDesc.m_pDevice = g_pd3dDevice;
		g_amdShadowDesc.m_pContext = g_pd3dDeviceContext;
		ASSERT_ONLY(AMD::SHADOWS_RETURN_CODE shadowStatus =) AMD::SHADOWS_Initialize( g_amdShadowDesc );
		Assert(shadowStatus == AMD::SHADOWS_RETURN_CODE_SUCCESS);		// TBD: real error handling.
	}
#endif

#if ATI_EXTENSIONS
	if( GetManufacturer() == ATI)
		OpenAMDExtensionInterfaces(g_pd3dDevice);
#endif //ATI_EXTENSIONS

#if DEBUG_FENCES
	g_GpuWaitFence = rage_new u64(0);
#endif
}

#if RSG_PC && __BANK
void grcDevice::AddWidgetsPC(bkBank &grcoreBank)
{
	grcoreBank.AddSlider("Vsync interval", &s_forceSyncInterval, -1, 4, 1);
	grcoreBank.AddToggle("Disable unused shader stages", &s_disableUnusedStages);

}
#endif // RSG_PC && __BANK

#if RSG_PC
void grcDevice::UpdateStereoTexture(const Vector3& vCamOffset,const Vector3& vCamOffset1)
{
#if NV_SUPPORT
	if (GetManufacturer() == NVIDIA)
	{
		if (IsStereoEnabled() && sm_StereoTex)
		{
			float temp[6];
			temp[0] = vCamOffset.GetX();
			temp[1] = vCamOffset.GetY();
			temp[2] = vCamOffset.GetZ();
			temp[3] = vCamOffset1.GetX();
			temp[4] = vCamOffset1.GetY();
			temp[5] = vCamOffset1.GetZ();
			s_oNVStereoManager->UpdateStereoTexture(GetCurrent(), g_grcCurrentContext, (ID3D11Texture2D*)sm_StereoTex->GetTexturePtr(), IsLost(),temp);
		}
	}
#else
	(void)vCamOffset;
	(void)vCamOffset1;
#endif
}

float grcDevice::GetEyeSeparation()
{
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

#if NV_SUPPORT
	if (GetManufacturer() == NVIDIA)
	{
		if (sm_pStereoHandle)
		{
			NvAPI_Stereo_GetEyeSeparation((StereoHandle)sm_pStereoHandle, &sm_fEyeSeparation);
		}
	}
#endif
	return sm_fEyeSeparation;
}

#if NV_SUPPORT
void grcDevice::SetDefaultConvergenceDistance(float fConv)
{
	if (!IsMessagePumpThreadThatCreatedTheD3DDevice())
	{
		sm_DesiredConvergence = fConv;
	}
	else
	{
		Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

		sm_fDefaultStereoConvergence = fConv;
		SetConvergenceDistance(fConv);
	}
}

void grcDevice::SetConvergenceDistance(float fConv)
{
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	if (GetManufacturer() == NVIDIA)
	{
		if (sm_fStereoConvergence != fConv)
		{
			if (sm_pStereoHandle)
			{
				NvAPI_Stereo_SetConvergence((StereoHandle)sm_pStereoHandle, fConv);
				sm_fStereoConvergence = fConv;
			}
		}
	}
}
#else
void grcDevice::SetDefaultConvergenceDistance(float) {}
void grcDevice::SetConvergenceDistance(float) {}
#endif

float grcDevice::GetConvergenceDistance()
{
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

#if NV_SUPPORT
	if (GetManufacturer() == NVIDIA)
	{
		if (sm_pStereoHandle)
		{
			NvAPI_Stereo_GetConvergence((StereoHandle)sm_pStereoHandle, &sm_fStereoConvergence);
		}
	}
#endif
	return sm_fStereoConvergence;
}

float grcDevice::GetSeparationPercentage(bool bUpdate)
{
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	float fStereoSeparationPercentage = 1.0f;

#if NV_SUPPORT
	if (GetManufacturer() == NVIDIA)
	{
		if (sm_pStereoHandle)
		{
			NvAPI_Stereo_GetSeparation((StereoHandle)sm_pStereoHandle, &fStereoSeparationPercentage);

			if (bUpdate)
				sm_fStereoSeparationPercentage = fStereoSeparationPercentage;
		}
	}
#else
	(void)bUpdate;
#endif

	return fStereoSeparationPercentage;
}

void grcDevice::SetSeparationPercentage(float fSepPercentage, bool bSaveVal, bool bForceUpdate)
{
	if (!IsMessagePumpThreadThatCreatedTheD3DDevice())
	{
		sm_DesiredSeparation = fSepPercentage;
	}
	else
	{
		Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	#if NV_SUPPORT
		if (GetManufacturer() == NVIDIA)
		{
			if (bForceUpdate || (fabsf(sm_fStereoSeparationPercentage - fSepPercentage) > 100.0f / 65535.0f))
			{
				if (sm_pStereoHandle)
				{
					NvAPI_Stereo_SetSeparation((StereoHandle)sm_pStereoHandle, fSepPercentage);
					if (bSaveVal)
						sm_fStereoSeparationPercentage = fSepPercentage;
				}
			}
		}
	#else
		(void)bSaveVal;
		(void)bForceUpdate;
	#endif
	}
}

void grcDevice::ForceStereorizedRT(Stereo_t bForce)
{
#if NV_SUPPORT
	if (GetManufacturer() == NVIDIA && IsUsingNVidiaAPI())
	{
		if (!CanUseStereo())
			return;

		LockContextNVStereo();

		NvAPI_Status status = NVAPI_ERROR;
		NvU8 isStereoEnabled;
		status = NvAPI_Stereo_IsEnabled(&isStereoEnabled);

		if (status == NVAPI_OK && isStereoEnabled && IsStereoEnabled())
		{
			sm_pStereoHandle = s_oNVStereoManager->GetStereoHandle();

			if (bForce == MONO)
			{
				if (NvAPI_Stereo_SetSurfaceCreationMode((StereoHandle)sm_pStereoHandle, NVAPI_STEREO_SURFACECREATEMODE_FORCEMONO) != NVAPI_OK)
				{
					AssertMsg(false, "forcing stereorized RT failed.\n");
				}
			}
			else if (bForce == STEREO)
			{
				if (NvAPI_Stereo_SetSurfaceCreationMode((StereoHandle)sm_pStereoHandle, NVAPI_STEREO_SURFACECREATEMODE_FORCESTEREO) != NVAPI_OK)
				{
					AssertMsg(false, "forcing stereorized RT failed.\n");
				}
			}
			else
			{
				if (NvAPI_Stereo_SetSurfaceCreationMode((StereoHandle)sm_pStereoHandle, NVAPI_STEREO_SURFACECREATEMODE_AUTO) != NVAPI_OK)
				{
					AssertMsg(false, "automating stereorized RT failed.\n");
				}
			}
		}

		UnlockContextNVStereo();
	}
#else
	(void)bForce;
#endif
}

void grcDevice::DeInitStereo()
{
#if NV_SUPPORT
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	if (GetManufacturer() == NVIDIA)
	{
		LockContextNVStereo();

		if (sm_StereoTex)	{ sm_StereoTex->Release(); }
		if (s_oNVStereoManager)	{ delete s_oNVStereoManager; s_oNVStereoManager = NULL; sm_pStereoHandle = NULL; }

		UnlockContextNVStereo();
	}
#endif
}

bool grcDevice::InitializeStereo()
{
#if NV_SUPPORT
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	if (GetManufacturer() == NVIDIA && IsUsingNVidiaAPI())
	{
		LockContextNVStereo();

		grcTextureFactory::TextureCreateParams param(	grcTextureFactory::TextureCreateParams::SYSTEM,
			grcTextureFactory::TextureCreateParams::LINEAR, grcsDiscard | grcsWrite);
		sm_StereoTex = grcTextureFactory::GetInstance().Create(nv::stereo::D3D11Type::StereoTexWidth, nv::stereo::D3D11Type::StereoTexHeight, grctfA32B32G32R32F, NULL, 1U /*numMips*/, &param);

		s_oNVStereoManager = rage_new nv::stereo::ParamTextureManager<nv::stereo::D3D11Type>;
		s_oNVStereoManager->Init(GetCurrent());
		sm_pStereoHandle = s_oNVStereoManager->GetStereoHandle();

		if (sm_pStereoHandle)
		{
			int iStereoActivated = sm_StereoDesired;
			if (PARAM_nvstereo.Get())
				PARAM_nvstereo.Get(iStereoActivated);

			if (iStereoActivated && sm_bStereoEnabled)
			{
				sm_bCanUseStereo = ((sm_pStereoHandle) ? true : false);

				NvAPI_Stereo_SetNotificationMessage((StereoHandle)sm_pStereoHandle, (NvU64)g_hwndMain, WM_STEREO_CHANGE);

				ActivateStereo(true);
			}
			else
				ActivateStereo(false);
		}

		UnlockContextNVStereo();

		s_gStereoTextureID = grcEffect::LookupGlobalVar("StereoParmsTexture", (sm_bCanUseStereo && sm_bStereoEnabled) ? true : false);

		return sm_bStereoEnabled;
	}
#endif // NV_SUPPORT
	return false;
}

void grcDevice::SetStereoTexture()
{
#if NV_SUPPORT
	if (sm_bStereoEnabled && sm_bCanUseStereo)
	{
		Assertf(sm_StereoTex != NULL, "Stereo texture is null while 3D stereo is enabled & activated.\n");
		grcEffect::SetGlobalVar(s_gStereoTextureID, sm_StereoTex);
	}
	else
	{
		grcEffect::SetGlobalVar(s_gStereoTextureID, grcTexture::NoneBlack);
	}
#endif
}

bool grcDevice::ActivateStereo(bool bActivate)
{
#if NV_SUPPORT
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	if (GetManufacturer() == NVIDIA && IsUsingNVidiaAPI())
	{
		if (StereoIsPossible())
		{
			if (bActivate)
			{
				if (NvAPI_Stereo_Activate(sm_pStereoHandle) == NVAPI_OK)
				{
					sm_bCanUseStereo = true;
					return true;
				}
				else
				{
					sm_bCanUseStereo = false; 
					return false;
				}
			}
			else
			{
				sm_bCanUseStereo = false;
				if (NvAPI_Stereo_Deactivate(sm_pStereoHandle) == NVAPI_OK)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
		}
	}
#else
	(void)bActivate;
#endif // NV_SUPPORT
	return false;
}
#endif		// RSG_PC

void grcDevice::ShutdownClass()
{
	LockContext();

	PrepareForShutdown();

	grcStateBlock::ShutdownClass();
	grcShutdownQuads();

#if USE_NV_SHADOW_LIB
	if (GetDxFeatureLevel() >= 1100 && IsUsingVendorAPI())
	{
		// Is this the correct place to for the Nv lib in the shutdown order?  It's not really clear.
		ASSERT_ONLY(NV_ShadowLib_Status shadowStatus =) NV_ShadowLib_CloseDX(&g_shadowContext);
		Assert(shadowStatus == NV_ShadowLib_Status_Ok);		// TBD: real error handling.
	}
#endif

#if USE_NV_TXAA
	TxaaCloseDX(&g_txaaContext);
#endif

#if USE_AMD_SHADOW_LIB
	if (GetDxFeatureLevel() >= 1100)
	{
		ASSERT_ONLY(AMD::SHADOWS_RETURN_CODE shadowStatus =) SHADOWS_Release( g_amdShadowDesc );
		Assert(shadowStatus == AMD::SHADOWS_RETURN_CODE_SUCCESS);		// TBD: real error handling.
	}
#endif

#if __WIN32PC
#if __D3D11
	DeviceLostShutdown();
#endif
	grcAdapterManager::ShutdownClass();
#endif

	// grcEffect::ShutdownClass has already been called by now:
	// delete g_DefaultEffect;
	// g_DefaultEffect = NULL;

	// Commented out: delete() is called in Release() when refCounter reaches zero.
	//DestroyVertexDeclaration(sm_BlitDecl);
	//DestroyVertexDeclaration(sm_ImDecl);

	// TODO: Free swap chain?
	////FreeSwapChain();

	// TODO: Free DMA Engine?

	grcViewport::ShutdownClass();
	grcEffect::ShutdownClass();

	g_grcCurrentContext->ClearState();

	LastSafeRelease(sm_BlitDecl);
	LastSafeRelease(sm_ImDecl);

#if !RSG_DURANGO
	SAFE_RELEASE_RESOURCE(s_BeginVertexBuffer);
	SAFE_RELEASE_RESOURCE(s_BeginIndexBuffer);
	SAFE_RELEASE_RESOURCE(s_BeginVerticesTriFanIndices);
	SAFE_RELEASE_RESOURCE(s_MissingInputsVertexBuffer);
#endif

#if RSG_PC
	grcBufferD3D11::ShutdownClass();
#endif // RSG_PC

	{
		int refCount = g_grcCurrentContext->Release();
		if (refCount)
		{
			grcWarningf("D3D device has %d dangling references\n", refCount);
			while ((refCount = g_grcCurrentContext->Release()) > 1);
		}
		g_grcCurrentContext = NULL;
	}

	/*
	{
		int refCount = g_grcCurrentContext->Release();
		if (refCount)
		{
			grcWarningf("D3D device has %d dangling references\n", refCount);
			while (g_grcCurrentContext->Release() > 1);
		}
		sm_CurrentContext11 = NULL;
	}
	*/
	/*
	{
		int refCount = g_pd3dDeviceContext->Release();
		if (refCount)
		{
			grcWarningf("D3D has %d dangling references\n", refCount);
			while (g_pd3dDeviceContext->Release());
		}
		g_pd3dDeviceContext = NULL;
	}
	*/

	//{
	//	int refCount = g_pd3dDevice->Release();
	//	if (refCount)
	//	{
	//		grcWarningf("D3D has %d dangling references\n", refCount);
	//		while (g_pd3dDevice->Release());
	//	}
	//	g_pd3dDevice = NULL;
	//}

#if __WIN32PC
	if (g_hwndMain && s_WeCreatedHwndMain) {
		if (!::DestroyWindow(g_hwndMain))
			grcErrorf("Could not destroy the window");
		if (!::UnregisterClass("grcWindow", ::GetModuleHandle(0)))
			grcErrorf("Could not un-register the windows class");
		if	(!g_inWindow)
			SetupCursor(true);
		g_hwndMain = 0;
		g_winClass = 0;
		s_WeCreatedHwndMain = false;
	}
#endif

	UnlockContext();

#if RSG_DURANGO
	g_SysService.RemoveDelegate(&sm_Delegate);
	sm_Delegate.Reset();
	sysIpcDeleteSema(sm_PlmResume);
#endif // RSG_DURANGO

#if USE_RESOURCE_CACHE
	grcResourceCache::ShutdownClass();
#endif // USE_RESOURCE_CACHE

#if ATI_EXTENSIONS
	CloseAMDExtensionInterfaces();
#endif //ATI_EXTENSIONS
}

void grcDevice::PrepareForShutdown()
{
	ID3D11ShaderResourceView *apViews[128];
	memset(apViews, 0, sizeof(apViews));
	memset(s_ResourceViews, NULL, sizeof(s_ResourceViews));

	g_grcCurrentContext->PSSetShaderResources(0, MAX_RESOURCES, apViews);
	g_grcCurrentContext->VSSetShaderResources(0, MAX_RESOURCES, apViews);
	g_grcCurrentContext->GSSetShaderResources(0, MAX_RESOURCES, apViews);
	g_grcCurrentContext->CSSetShaderResources(0, MAX_RESOURCES, apViews);
	g_grcCurrentContext->DSSetShaderResources(0, MAX_RESOURCES, apViews);
	g_grcCurrentContext->HSSetShaderResources(0, MAX_RESOURCES, apViews);
	grcDeviceView *apTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	memset(apTargets, 0, sizeof(apTargets));

	SetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, (const grcDeviceView**)apTargets, NULL);

	ID3D11Buffer* apBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {0};
	UINT auiStrideOffset[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {0};
	g_grcCurrentContext->IASetInputLayout(NULL);
	g_grcCurrentContext->IASetIndexBuffer(NULL, DXGI_FORMAT_R16_UINT, 0);
	g_grcCurrentContext->IASetVertexBuffers(0, (GetDxFeatureLevel() >= 1100) ? D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT : D3D10_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT, apBuffers, auiStrideOffset, auiStrideOffset);

	g_grcCurrentContext->RSSetViewports(0, NULL);

	sm_numViewports = 0;

	g_grcCurrentContext->PSSetShader(NULL, NULL, 0);
	g_grcCurrentContext->VSSetShader(NULL, NULL, 0);
	g_grcCurrentContext->CSSetShader(NULL, NULL, 0);
	g_grcCurrentContext->DSSetShader(NULL, NULL, 0);
	g_grcCurrentContext->GSSetShader(NULL, NULL, 0);
	g_grcCurrentContext->HSSetShader(NULL, NULL, 0);

	sm_CurrentVertexProgram = 0;
	sm_CurrentFragmentProgram = 0;
	sm_CurrentComputeProgram = 0;
	sm_CurrentDomainProgram = 0;
	sm_CurrentHullProgram = 0;

	ClearCachedState();
}

#if __WIN32PC

#if NV_SUPPORT
void grcDevice::InitNVIDIA()
{
	NvAPI_Status status = NVAPI_ERROR;
	status = NvAPI_Initialize();

	sm_UseNVidiaAPI = true;
	if (status != NVAPI_OK)
	{
		sm_UseNVidiaAPI = false;
		s_DepthBoundsTestSupported = false;
		grcWarningf("Failed to Initialize NVidia API Lib");
	}
}
#endif	// NV_SUPPORT

size_t grcDevice::RetrieveVideoMemory()
{
	const SIZE_T uMinVideoMemory = 900 * 1024 * 1024;
	D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_10_0};
	D3D_FEATURE_LEVEL chosenFeatureLevel;
	grcDeviceHandle* device;

	HRESULT hr = g_D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 1, D3D11_SDK_VERSION, &device, &chosenFeatureLevel, NULL);
	if(hr == S_OK)
	{
		IDXGIDevice * pDXGIDevice;
		HRESULT hr = device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
		Assertf(SUCCEEDED(hr), "Failed to query IDXGIDevice interface %x", hr);
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter * pDXGIAdapter;
			hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);
			Assertf(SUCCEEDED(hr), "Failed to query IDXGIAdapter interface %x", hr);
			if (SUCCEEDED(hr))
			{
				DXGI_ADAPTER_DESC	oAdapterDesc;

				if( SUCCEEDED( pDXGIAdapter->GetDesc(&oAdapterDesc) ) )
				{
					device->Release();

					return (s64)(( oAdapterDesc.DedicatedVideoMemory < uMinVideoMemory ) ? ((oAdapterDesc.SharedSystemMemory < uMinVideoMemory) ? (2048ULL * 1024ULL * 1024ULL) : oAdapterDesc.SharedSystemMemory) : oAdapterDesc.DedicatedVideoMemory);
				}
			}
		}

		device->Release();
	}

	return 0;
}

s64 grcDevice::GetAvailableVideoMemory(int adapter)
{
	if (adapter == -1)
	{
		adapter = GetAdapterOrdinal();
	}
	return ((grcAdapterD3D11*) grcAdapterManager::GetInstance()->GetAdapter(adapter))->GetAvailableVideoMemory();
}

#endif

void grcDevice::BeginDXView()
{
	ClearCachedState();

	s_SwapChainLock.Lock();
	if (GetSwapChain() != NULL)
	{
		ID3D11Texture2D* pBackBuffer = NULL;
		if (SUCCEEDED(((IDXGISwapChain*)GetSwapChain())->GetBuffer(0, __uuidof(pBackBuffer)/*IID_ID3D11Texture2D*/ /*__uuidof(ID3D11Texture2D)*/, reinterpret_cast<void**>(&pBackBuffer))))
		{
			pBackBuffer->Release();
			const grcDeviceView *poRenderTargetView = ((grcRenderTargetD3D11*)grcDevice::sm_pBackBuffer)->GetTargetView(0,0);
			SetRenderTargets(1, &poRenderTargetView, grcDevice::sm_pDepthStencil ?
				((grcRenderTargetD3D11*)grcDevice::sm_pDepthStencil)->GetTargetView(0,0) : NULL);
		}
		else
		{
			Assert(0);
		}
	}
	else
	{
		Assert(0);
	}
	s_SwapChainLock.Unlock();

	g_grcCurrentContext->RSSetViewports(sm_numViewports, sm_viewports);

#if 0
	// be very careful ... prolly change this for real multi-threading (worker thread > 2)
	grcTextureFactory::CreateParams params;
	GetMultiSample(*(u32*)&params.Multisample, params.MultisampleQuality);
	if (params.Multisample != MSAA_None) 
	{
		grcState::SetState( grcsMultiSample, true );
	}
#endif
}

void grcDevice::EndDXView()
{
	// Bind render target for PC
	//if(!PARAM_nopostfx.Get())
	//	GRPOSTFX->ReleasePCRenderTarget();

	//if (GetMultiSample()) 
	//{
	//	grcState::SetState( grcsMultiSample, false );
	//}
}

#if RSG_PC
sysIpcSema resetDeviceSema;

void grcDevice::LockDeviceResetAvailability()
{
	if (!resetDeviceSema)
	{
		resetDeviceSema = sysIpcCreateSema(true);
	}
	sysIpcWaitSema(resetDeviceSema);
}

void grcDevice::UnlockDeviceResetAvailability()
{
	if (!resetDeviceSema)
	{
		resetDeviceSema = sysIpcCreateSema(true);
	}
	sysIpcSignalSema(resetDeviceSema);
}

void grcDevice::RegisterSessionNotification()
{
	WTSRegisterSessionNotification(g_hwndMain, NOTIFY_FOR_THIS_SESSION);
}

void grcDevice::UnregisterSessionNotification()
{
	WTSUnRegisterSessionNotification(g_hwndMain);
}
#endif

#if __BANK
// Useful for debugging deadlocks
static sysIpcCurrentThreadId grcDevice_ContextLockOwner = sysIpcCurrentThreadIdInvalid;
#endif

void grcDevice::LockContext()
{
	if( s_ContextLockReferenceCount++ == 0 )
	{
		// Lock() upon the ref count = 0.
		// s_ContextLock.Lock();
		sysIpcLockMutex(s_ContextLock);

		// Set the current context to be the main context.
#if DEVICE_USE_D3D_WRAPPER
 		g_grcCurrentContext = &s_DeviceContext11Wrapper;
#else // DEVICE_USE_D3D_WRAPPER
		g_grcCurrentContext = g_pd3dDeviceContext;
#endif // DEVICE_USE_D3D_WRAPPER

		BANK_ONLY(grcDevice_ContextLockOwner = sysIpcGetCurrentThreadId();)
	}
}

void grcDevice::UnlockContext()
{
	if( --s_ContextLockReferenceCount == 0 )
	{
		BANK_ONLY(grcDevice_ContextLockOwner = sysIpcCurrentThreadIdInvalid;)

		// Unlock when ref count = 0 again.
		//s_ContextLock.Unlock();
		sysIpcUnlockMutex(s_ContextLock);
		// Say we have no current context.
		g_grcCurrentContext = NULL;
	}
}

#if NV_SUPPORT
void grcDevice::LockContextNVStereo()
{
	if( s_NVStereoLockReferenceCount++ == 0 )
	{
		// Lock() upon the ref count = 0.
		sysIpcLockMutex(s_NVStereoLock);
	}
}

void grcDevice::UnlockContextNVStereo()
{
	if( --s_NVStereoLockReferenceCount == 0 )
	{
		sysIpcUnlockMutex(s_NVStereoLock);
	}
}
#endif

#if RSG_PC
void grcDevice::LockContextIfInitialized()
{
	if (s_ContextLock) LockContext();
}

void grcDevice::UnlockContextIfInitialized()
{
	if (s_ContextLock) UnlockContext();
}
#endif

#define  NO_BACKBUFFER_LOCK (1 && RSG_DURANGO)	// Avoid stalling GPU by locking potentially visible backbuffer before needed

bool grcDevice::BeginFrame()
{
	NOTFINAL_ONLY( grcFlashEnable = (sm_FrameCounter & 16) != 0 );

	LockContext();

#if USE_FRAME_LATENCY_LIMITTING_FENCE 
	PF_PUSH_TIMEBAR_BUDGETED("Wait on queued frames", 1.0f/30.0f);
	// Wait on fence from 2 frames back to ensure that the GPU isn't falling too far behind. 
	// See additional comments at definition of USE_FRAME_LATENCY_LIMITTING_FENCE.
	const u32 nFrameFenceIndex = sm_FrameCounter % (MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU + 1);
	CpuWaitOnFence(ms_pahFrameLatencyFenceHandles[nFrameFenceIndex]);
	PF_POP_TIMEBAR();
#endif // USE_FRAME_LATENCY_LIMITTING_FENCE

#if RSG_DURANGO
	// Set the ID3D11Buffer version of IASetIndexBuffer to NULL at the start of
	// the frame, as we only use IASetPlacementIndexBuffer.
	g_grcCurrentContext->IASetIndexBuffer(NULL, DXGI_FORMAT_R16_UINT, 0);
#endif

#if EFFECT_CACHE_PROGRAMRESOURCES
	memset(s_Fingerprints, NULL, sizeof(u32)*NUM_TYPES);
#endif // EFFECT_CACHE_PROGRAMRESOURCES

#if __D3D11_REPORT_RESOURCE_HAZARDS
	static_cast <RageDirect3DDeviceContext11 *>(g_grcCurrentContext)->ResetReportResourceHazards();
#endif //__D3D11_REPORT_RESOURCE_HAZARDS

	if (!sysIpcWaitSemaTimed(sm_Controller, 50))
		return false;

#if __WIN32PC
	if (!IsCreated() || 
		IsPaused() ||  
		IsInReset() ||  
		IsLost() ||
		IsInsideDeviceChange() ||
		(IsMinimized() && !ContinueRenderingOverride()) ||
		IsInSizeMove() ||
		CanIgnoreSizeChange() ||
		IsReleasing() ||
		//IsOccluded() ||
		IsInReset() ||
		IsDeviceRestored())
	{
		sysIpcSleep(50);
		if (1) //sm_Owner != sm_CreationOwner)
		{
#if __FINAL
			static bool bOnce = true;
			if (bOnce)
			{
				Displayf("Reset must be handled outside if owner and creation owner do not match");
				bOnce = false;
			}
#endif // __FINAL
			sysIpcSignalSema(sm_Controller);
			return false;
		}
	}
	if (IsCreated() && (IsLost() || (IsOccluded() && !ContinueRenderingOverride()) || IsReleasing()) && !IsPaused())
	{
		Assert(!IsLost());
		Assert(IsOccluded());
		u32 uFlags = 0;
		if( IsOccluded() )
			uFlags = DXGI_PRESENT_TEST;
		else
			uFlags = 0;

		s_SwapChainLock.Lock();
		if (GetSwapChain() != NULL)
		{
			sm_uDeviceStatus = ((IDXGISwapChain*)GetSwapChain())->Present(!GetFrameLockOverride() ? sm_CurrentWindows[g_RenderThreadIndex].uFrameLock : 0, uFlags);
			if (sm_uDeviceStatus != S_OK)
			{
				CheckDxHresultFatal(sm_uDeviceStatus);
				//Displayf("Not ready to render Device Status %x", sm_uDeviceStatus);
				sysIpcSignalSema(sm_Controller);
				s_SwapChainLock.Unlock();
				if (!GRCDEVICE.IsSwapChainFullscreen())
					return false;
			}
		}
		else
		{
			sysIpcSignalSema(sm_Controller);
			s_SwapChainLock.Unlock();
			return false;
		}
		s_SwapChainLock.Unlock();
		// Desktop may have changed and the display format may be a different color depth
		// you'll need to adjust the color to match the new color format
		// PC TODO - Do the above
		grcDisplayWindow oRestore = sm_CurrentWindows[g_RenderThreadIndex];
		sysIpcSignalSema(sm_Controller);
		if (!ChangeDevice(oRestore))
		{
			return false;
		}
		else
		{
			SetLost( false );
			SetOccluded(false);
			if (!sysIpcWaitSemaTimed(sm_Controller, 50))
				return false;
		}
	}
#endif		// __WIN32PC

	// Make no assumptions at the start of a frame.
	ClearCachedState();

	//GetCurrent11()->BeginScene();
	grcStateBlock::MakeDirty();
	grcStateBlock::Default();

	s_SwapChainLock.Lock();
	if (GetSwapChain() != NULL && grcDevice::sm_pBackBuffer != NULL)
	{
		ID3D11Texture2D* pBackBuffer = NULL;
		if (SUCCEEDED(((IDXGISwapChain*)GetSwapChain())->GetBuffer(0, __uuidof(pBackBuffer)/*IID_ID3D11Texture2D*/ /*__uuidof(ID3D11Texture2D)*/, reinterpret_cast<void**>(&pBackBuffer))))
		{
			pBackBuffer->Release();
			if (!grcDevice::sm_pDepthStencil)
			{
				SetDepthStencilState(sm_WriteNoDepthNoStencil);
			}
#if !NO_BACKBUFFER_LOCK
			grcTextureFactory::GetInstance().LockRenderTarget(0, grcDevice::sm_pBackBuffer, grcDevice::sm_pDepthStencil);
			s_BackBufferLocked = true;
#endif
		}
		else
		{
			sysIpcSignalSema(sm_Controller);
			s_SwapChainLock.Unlock();
			return false;
		}
	}
	else
	{
		sysIpcSignalSema(sm_Controller);
		s_SwapChainLock.Unlock();
		return false;
	}
	s_SwapChainLock.Unlock();

	grcEffect::BeginFrame();
	ResetClipPlanes();

#if RSG_PC
	// Reset any constant buffer over-rides.
	SetVertexShaderConstantBufferOverrides(0, NULL, 0);
#endif

#if defined(PORTCULLIS_ENABLE) && PORTCULLIS_ENABLE
	if(rage::portcullis::DoCheck() == false)
	{
		Displayf("Possible access violation code #%d. Talk to Klaas", fwRandom::GetRandomNumberInRange(101, 999));
		sm_pSwapChain = (ID3D11Device*)((size_t)sm_pSwapChain | 0xE77DEBDE);
	}
#endif // PORTCULLIS_ENABLE

	s_FencePool.CleanUpOverflow();

	return true;
}

// Inline rand() function only used by the frack blend states code. We don't want this used by anything else, so
// the fracking is deterministic (Important for support).
inline u32 frackRand()
{
	static u32 next = 0xCB536E6A;
	next = next * 214013 + 2531011;
	return next;
}

void grcDevice::EndFrame(const grcResolveFlags * /*flags*/ /* = NULL */)
{
#if DEVICE_EQAA
	eqaa::Init();
#endif 

	HRESULT res;
	grcEffect::EndFrame();

	++sm_FrameCounter;

	g_DefaultEffect->SetVar(g_DefaultSampler, (grcTexture*) NULL );	// remove any references to none texture

	WIN32PC_ONLY(if (!g_grcCurrentContext) return);
	
	ClearStreamSource(0);
	ClearStreamSource(1);
	ClearStreamSource(2);
	ClearStreamSource(3);

	// Make sure no effect is active.
	g_DefaultEffect->Bind(grcetNONE);

	u32 uFlags = 0;
#if __WIN32PC
	if( IsOccluded() && !GRCDEVICE.IsSwapChainFullscreen() && !GRCDEVICE.ContinueRenderingOverride() )
		uFlags = DXGI_PRESENT_TEST;
	else
		uFlags = 0;

	if (sm_PresentCallback)
	{
		PF_PUSH_TIMEBAR("Present Callback");
		(*sm_PresentCallback)(sm_Current,NULL,NULL,g_hwndOverride,NULL);
		PF_POP_TIMEBAR();
	}
#endif

	// Cheat and start earlier
	sysIpcSignalSema(sm_Controller);

	// TELEMETRY_TRY_LOCK(sm_pSwapChain, __FILE__, __LINE__, __FUNCTION__);
	// Resolve MSAA to swap chain if necessary
	if (sm_pBackBuffer->GetMSAA())
	{
		grcDeviceTexture* pRealBackBuffer = NULL;
		res = ((IDXGISwapChain*)GetSwapChain())->GetBuffer(0, __uuidof(pRealBackBuffer)/*IID_ID3D11Texture2D*/ /*__uuidof(ID3D11Texture2D)*/, reinterpret_cast<void**>(&pRealBackBuffer));
		grcAssertf(res==S_OK,"Unable to get back buffer!");
		CheckDxHresultFatal(res);

		ID3D11Texture2D* pResolveTo = (ID3D11Texture2D*)pRealBackBuffer;
		ID3D11Texture2D* pResolveFrom = (ID3D11Texture2D*)(sm_pBackBuffer)->GetTexturePtr();
		g_grcCurrentContext->ResolveSubresource(pResolveTo, 0, pResolveFrom, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
	}

#define SLEEP_BEFORE_PRESENT (0 && RSG_PC)

#if SLEEP_BEFORE_PRESENT
	UnlockContext();
#endif // SLEEP_BEFORE_PRESENT
	int syncInterval = sm_CurrentWindows[g_RenderThreadIndex].uFrameLock;
	
#if RSG_PC
	syncInterval = sm_CurrentWindows[g_RenderThreadIndex].uFrameLock;

	float fRefreshRate = 60.0f;
	if (sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate.Numerator > 0)
	{
		fRefreshRate = float(sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate.Numerator) / float(sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate.Denominator);
	}

	static sysTimer presentTimer;	
	if (GetFrameLockOverride())
	{	// Disable vsync
		syncInterval = 0;
	}
	else
	{
		PARAM_frameLimit.Get(syncInterval);
#if __BANK
		if (s_forceSyncInterval != -1)
			syncInterval = s_forceSyncInterval;
#endif // __BANK
	}
#if SLEEP_BEFORE_PRESENT
	// Present immediate wrecks my ability to time this properly.  256fps limit should be good enough to stop queued frame stalls within the driver in load screens.
	// const float updateRate = 1.0f / fRefreshRate;
	const float fFrameEndTime = (updateRate * syncInterval) - fMinDeltaBeforeVSync;
	PF_PUSH_TIMEBAR_DETAIL("Frame Limit Sleep");
	float fSleepTime = fFrameEndTime - presentTimer.GetTime();
	if (fSleepTime > fMinDeltaBeforeVSync)
	{
		sysIpcSleep((u32)(fSleepTime * 1000.0f));
	}
	PF_POP_TIMEBAR_DETAIL();

	PF_PUSH_TIMEBAR_DETAIL("LockContext");
	LockContext();
	PF_POP_TIMEBAR_DETAIL();
#endif // SLEEP_BEFORE_PRESENT

#endif // RSG_PC
	DEVICE_EKG_START(Present);

#if USE_FRAME_LATENCY_LIMITTING_FENCE
	// Set the fence for this frame, we'll wait on it at some future frame in grcDevice::BeginFrame.
	// See additional comments at definition of USE_FRAME_LATENCY_LIMITTING_FENCE
	const u32 nFrameThrottleFenceIndex = (sm_FrameCounter-1) % (MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU + 1);
	grcFenceHandle fence = ms_pahFrameLatencyFenceHandles[nFrameThrottleFenceIndex];
	CpuMarkFencePending(fence);
	GpuMarkFenceDone(fence);
#endif // USE_FRAME_LATENCY_LIMITTING_FENCE
	
#if RSG_DURANGO
	grcGfxContext::endFrame();
#endif

#if (RSG_DURANGO && __D3D11_MONO_DRIVER)
	const DXGIX_PRESENTARRAY_PARAMETERS parameters = 
	{
		FALSE,                                      // BOOL        Disable;
		FALSE,                                      // BOOL        UsePreviousBuffer;
		{
			0,                                      // LONG    left;
			0,                                      // LONG    top;
			g_SwapDesc.Width,						// LONG    right;
			g_SwapDesc.Height,						// LONG    bottom;
		},                                          // D3D11_RECT  SourceRect;
		{
			0,                                      // LONG  x;
			0,                                      // LONG  y;
		},                                          // POINT       DestRectUpperLeft;
		1920.0f/g_SwapDesc.Width,                   // FLOAT       ScaleFactorVert;
		1080.0f/g_SwapDesc.Height,                  // FLOAT       ScaleFactorHorz;
		nullptr,                                    // VOID*       Cookie;
	};
	unsigned PresentImmediateThreshold = 0;
	PARAM_presentThreshold.Get(PresentImmediateThreshold);
	res = DXGIXPresentArray(syncInterval, PresentImmediateThreshold, uFlags, 1, (IDXGISwapChain1 *const *)&sm_pSwapChain, &parameters);

#else // (RSG_DURANGO && __D3D11_MONO_DRIVER)

	s_SwapChainLock.Lock();

#if RAGE_TIMEBARS
	static char *szPresentText[] = { "Present(0)", "Present(1)", "Present(2)", "Present(3)", "Present(4)" };
	float fDesiredFrameRate = fRefreshRate / Max<float>((float)syncInterval, 1.0f);
#endif // RAGE_TIMEBARS

	int d3dSyncInterval = IsWindowed() && !GetFrameLockOverride() ? 0 : syncInterval;
	PF_PUSH_TIMEBAR_BUDGETED(szPresentText[d3dSyncInterval], Max(((1.0f / fDesiredFrameRate) - presentTimer.GetTime()) * 1000.0f, 1.0f));

#if NV_SUPPORT
	LockContextNVStereo();
#endif // NV_SUPPORT
	if (sm_PresentHandler)
	{
		sm_PresentHandler(syncInterval, uFlags);
		res = S_OK;
	} 
	else
	{
		res = ((IDXGISwapChain*)GetSwapChain())->Present(d3dSyncInterval, uFlags);
		CHECK_HRESULT(res);
		CheckDxHresultFatal(res);
	}
#if NV_SUPPORT
	UnlockContextNVStereo();
#endif // NV_SUPPORT

	PF_POP_TIMEBAR();

	presentTimer.Reset();

// 	// Get Telemetry to start/stop right on the presentation interval (rather than the logical frame start)
	// requires including "telemetry/telemetry_rage.h" (remove from app.cpp)
// 	TelemetryUpdate();

	s_SwapChainLock.Unlock();
#endif // (RSG_DURANGO && __D3D11_MONO_DRIVER)

#if RSG_PC
	// rlLivestream post present callback (used for twitch SDK video frame submit)
	PF_PUSH_TIMEBAR_DETAIL("PostPresentCallback");
	if (sm_PostPresentCallback != NULL)
		(*sm_PostPresentCallback)();
	PF_POP_TIMEBAR_DETAIL();
#endif

	DEVICE_EKG_STOP(Present);

	// TELEMETRY_END_TRY_LOCK(LRT_SUCCESS, GetSwapChain(), __FILE__, __LINE__);

#if __WIN32PC
#if NV_SUPPORT
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	static HRESULT sm_EndFrameDeviceStatus = S_FALSE;
	if (sm_bStereoEnabled && (sm_EndFrameDeviceStatus != res) && (res == S_OK) && sm_UseNVidiaAPI)
	{
		// checking 3D stereo is available only after 1st Present function
		NvU8 isStereoActivated = false;
		NvAPI_Stereo_IsActivated(sm_pStereoHandle,&isStereoActivated);
		sm_bCanUseStereo = isStereoActivated ? true : false;

		sm_EndFrameDeviceStatus = res;
	}
#endif // NV_SUPPORT
	sm_uDeviceStatus = res;
	if (res == DXGI_STATUS_OCCLUDED)
	{
		SetOccluded(true);
	}
	else if (res == S_OK)
	{
		SetOccluded(false);
	}
#endif // __WIN32PC
	//sysIpcSignalSema(sm_Controller);
#if !NO_BACKBUFFER_LOCK
	if (s_BackBufferLocked)
	{
		grcTextureFactory::GetInstance().UnlockRenderTarget(0);
		s_BackBufferLocked = false;
	}
#endif // !NO_BACKBUFFER_LOCK
	//The stack size should always be -1 at this point, if not lock/unlocks have gone wrong somewhere.
	Assert(((grcTextureFactoryDX11 *)(&grcTextureFactoryDX11::GetInstance()))->GetRenderTargetStackCount() == -1);

	PF_PUSH_TIMEBAR_DETAIL("UnlockContext");
	UnlockContext();
	PF_POP_TIMEBAR_DETAIL();

#if __BANK && RSG_DURANGO
	DXGIXGetFrameStatistics(16,g_FrameStatistics);
#endif // __BANKS && RSG_DURANGO

#if defined(PORTCULLIS_ENABLE) && PORTCULLIS_ENABLE
	if(rage::portcullis::DoCheck() == false)
	{
		Displayf("Possible access violation code #%d. Talk to Klaas", fwRandom::GetRandomNumberInRange(101, 999));
		sm_pSwapChain = (ID3D11Device*)((size_t)sm_pSwapChain | 0xBDDBB7EB);
	}
#endif // PORTCULLIS_ENABLE

#if RSG_PC
	if((sm_KillSwitch & 0x10101010) && ((frackRand() >> 22) & 0x1))
	{
		static u32 counter = 0;
		if(counter < 1110)
		{
			++counter;
			grcStateBlock::FrackBlendStates(frackRand(), frackRand());
		}
	}

	GRCDEVICE.IncrementGPUIndex();
#endif
}

#if RSG_DURANGO

	grcFenceHandle grcDevice::AllocFence(u32 flags)
	{
		const __grcFenceHandle::StateType init = (__grcFenceHandle::StateType)((flags & ALLOC_FENCE_INIT_AS_PENDING)
			? __grcFenceHandle::STATE_PENDING : __grcFenceHandle::STATE_DONE);
#	  if GRCCONTEXT_ALLOC_SCOPES_SUPPORTED
		if (flags & ALLOC_FENCE_ALLOC_SCOPE_LIFETIME)
		{
			CompileTimeAssert(offsetof(__grcFenceHandle, state) == 0);
			CompileTimeAssert(sizeof(((__grcFenceHandle*)0)->state) == 4);
			u32 *const fence = (u32*)grcGfxContext::current()->allocateTemp(4, 8);
			*fence = init;
			return (grcFenceHandle) fence;
		}
		else
#	  endif
		{
			grcFenceHandle fence = s_FencePool.Alloc();
			fence->state = init;
			return fence;
		}
	}

	void grcDevice::CpuMarkFencePending(grcFenceHandle fence)
	{
		fence->state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_PENDING;
	}

	void grcDevice::CpuMarkFenceDone(grcFenceHandle fence)
	{
		// CpuMarkFenceDone can be used to unblock the GPU that is waiting on the CPU
		// to write some data.  So use a write barrier to ensure the GPU sees writes
		// in the correct order.
		sysMemWriteBarrier();

		fence->state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_DONE;
	}

	void grcDevice::GpuMarkFencePending(grcFenceHandle fence)
	{
		// D3D11.x does not expose CP DMA which we use on PS4.  Here we need the
		// value to be written before the PFP can continue executing.  If we
		// attempted to just naively use an EVENT_WRITE_EOP/BOTTOM_OF_PIPE_TS,
		// then there would be a race condition where a GpuWaitOnFence could
		// succeed before STATE_PENDING was actually written.
		//
		// Because of this API limitation, we need to block the PFP on the write
		// completing.  PFP waits are limited to only doing a >= comparison, so
		// to correctly wait on the STATE_PENDING write, we use a second scratch
		// value (not that EOP events always occur in order).
		//
		GRC_ALLOC_SCOPE_AUTO_PUSH_POP();
		grcContextHandle *const ctx = g_grcCurrentContext;
		u32 *const scratch = grcGfxContext::current()->allocateTemp<u32>();
		*scratch = 0;
		ctx->WriteValueBottomOfPipe(const_cast<u32*>(&(fence->state)), (UINT)__grcFenceHandle::STATE_PENDING);
		ctx->WriteValueBottomOfPipe(scratch, 1);
		ctx->InsertWaitOnMemory(scratch, D3D11X_INSERT_WAIT_ON_MEMORY_USE_PFP, D3D11_COMPARISON_GREATER_EQUAL, 1, ~0u);
	}

	void grcDevice::GpuMarkFenceDone(grcFenceHandle fence, u32 UNUSED_PARAM(flags))
	{
		// WriteValueBottomOfPipe is issuing a CACHE_FLUSH_AND_INV_TS_EVENT
		// rather than BOTTOM_OF_PIPE_TS, so regardless of whether or not the
		// GPU_WRITE_FENCE_AFTER_SHADER_READS flag is set, there is no need for
		// an additional invalidation of the color and depth blocks here.
		g_grcCurrentContext->WriteValueBottomOfPipe(const_cast<u32*>(&(fence->state)), (UINT)__grcFenceHandle::STATE_DONE);
	}

	bool grcDevice::IsFencePending(grcFenceHandle fence, u32 flags)
	{
		if (~flags & IS_FENCE_PENDING_HINT_NO_KICK)
		{
			if (g_grcCurrentContext)
			{
				grcGfxContext::kickContextIfPartialSubmitsAllowed();
			}
		}

		return fence->state == (__grcFenceHandle::StateType)__grcFenceHandle::STATE_PENDING;
	}

	void grcDevice::CpuWaitOnFence(grcFenceHandle fence)
	{
#		if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
			CustomHangDetection watchdog("grcDevice::CpuWaitOnFence", GRCGFXCONTEXT_DURANGO_GPU_HANG_TIMEOUT_SEC);
#		endif

		while (IsFencePending(fence))
		{
			sysIpcSleep(1);

#			if GRCGFXCONTEXT_DURANGO_BASIC_ERROR_HANDLING
				if (watchdog.pollHasTimedOut())
				{
					grcGfxContext::reportGpuHang();
				}
#			endif
		}
	}

	void grcDevice::CpuFreeFence(grcFenceHandle fence)
	{
		s_FencePool.ImmediateFree(fence);
	}

	void grcDevice::GpuWaitOnPreviousWrites()
	{
		g_grcCurrentContext->InsertWaitUntilIdle(0);
	}

	void grcDevice::GpuWaitOnFence(grcFenceHandle fence, u32 inputs)
	{
		grcContextHandle *const ctx = g_grcCurrentContext;

		// July 2014 XDK adds WAIT_REG_MEM support (ID3D11DeviceContextX::InsertWaitOnMemory).
#		if _XDK_VER >= 11274

			// When the PFP is stalled the only comparison function that can be
			// used is >= (cannot use == like we would prefer).  This is a
			// limitation of the GPU.  So this compile time assert checks that
			// values are orderred correctly.
			CompileTimeAssert(__grcFenceHandle::STATE_DONE > __grcFenceHandle::STATE_PENDING);

			// The PFP will prefetch indices, so it needs to be stalled when
			// GPU_WAIT_ON_FENCE_INPUT_INDEX_BUFFER is set.
			if (inputs & GPU_WAIT_ON_FENCE_INPUT_INDEX_BUFFER)
			{
				ctx->InsertWaitOnMemory(const_cast<u32*>(&(fence->state)), D3D11X_INSERT_WAIT_ON_MEMORY_USE_PFP, D3D11_COMPARISON_GREATER_EQUAL, (u32)__grcFenceHandle::STATE_DONE, ~0u);
			}
			else
			{
				ctx->InsertWaitOnMemory(const_cast<u32*>(&(fence->state)), 0, D3D11_COMPARISON_EQUAL, (u32)__grcFenceHandle::STATE_DONE, ~0u);
			}

#		else
			GRC_ALLOC_SCOPE_AUTO_PUSH_POP();

#			if DEBUG_FENCES
				struct Debug : public GpuWaitMem32_cbuf
				{
					Debug  *self;
					size_t  stack[16];
				};
				auto *const cbuf = grcGfxContext::current()->allocateTemp<Debug>();
				cbuf->self = cbuf;
				sysStack::CaptureStackTrace(cbuf->stack, NELEM(cbuf->stack));
				ctx->CopyMemoryToMemory(const_cast<u64*>(g_GpuWaitFence), &cbuf->self, sizeof(*g_GpuWaitFence));
#			else
				auto *const cbuf = grcGfxContext::current()->allocateTemp<GpuWaitMem32_cbuf>();
#			endif
			cbuf->Value = (u32)__grcFenceHandle::STATE_DONE;      // g_WaitMem32Value

			s_MemoryCS->Bind((grcEffectTechnique)CS_GpuWaitMem32);
			ctx->SetFastResources(
				(D3D11X_SET_FAST)(D3D11X_SET_FAST_CS_WITH_OFFSET|D3D11X_SET_FAST_TYPE_BUFFER_VIEW), MEMORYCS_CBUF_ARGS_SLOT, s_CommonBufferCB, (uptr)cbuf-FIXED_PLACEMENT_BASE,
				(D3D11X_SET_FAST)(D3D11X_SET_FAST_CS_WITH_OFFSET|D3D11X_SET_FAST_TYPE_BUFFER_VIEW), MEMORYCS_WAIT_MEM32_SRC_BUF_SLOT, &s_CommonBufferUAV, (uptr)&(fence->state));
			ctx->Dispatch(1, 1, 1);
			s_MemoryCS->UnBind();

			if (inputs & GPU_WAIT_ON_FENCE_INPUT_INDEX_BUFFER)
			{
				// TODO: Check PIX disassembly to verify this does block the CPG
				ctx->InsertWaitUntilIdle(0);
			}
			else
			{
				ctx->GpuSendPipelinedEvent(D3D11X_GPU_PIPELINED_EVENT_CS_PARTIAL_FLUSH);
			}

#			if DEBUG_FENCES
				ctx->FillMemoryWithValue(const_cast<u64*>(g_GpuWaitFence), sizeof(*g_GpuWaitFence), 0);
#			endif
#		endif
	}

	void grcDevice::GpuFreeFence(grcFenceHandle fence)
	{
		g_grcCurrentContext->WriteValueBottomOfPipe(const_cast<u32*>(&(fence->state)), (UINT)__grcFenceHandle::STATE_FREE);
		s_FencePool.PendingFree(fence);
	}

#else // if !RSG_DURANGO

	grcFenceHandle grcDevice::AllocFence(u32 flags)
	{
		const u32 init = (flags & ALLOC_FENCE_INIT_AS_PENDING) ? __grcFenceHandle::STATE_MARKED_PENDING : __grcFenceHandle::STATE_DONE;
		grcFenceHandle fence = s_FencePool.Alloc();
		fence->state = (__grcFenceHandle::StateType)init;
		return fence;
	}

	void grcDevice::CpuMarkFencePending(grcFenceHandle fence)
	{
		grcAssert(fence->state != __grcFenceHandle::STATE_QUERY_ISSUED);
		fence->state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_MARKED_PENDING;
	}

	void grcDevice::CpuMarkFenceDone(grcFenceHandle fence)
	{
		grcAssert(fence->state != __grcFenceHandle::STATE_QUERY_ISSUED);
		fence->state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_DONE;
	}

	void grcDevice::GpuMarkFencePending(grcFenceHandle fence)
	{
		// Because DEVICE_GPU_WAIT is not supported, it is ok to set this
		// immediately on the CPU.
		grcAssert(fence->state != __grcFenceHandle::STATE_QUERY_ISSUED);
		fence->state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_MARKED_PENDING;
	}

	void grcDevice::GpuMarkFenceDone(grcFenceHandle fence, u32 UNUSED_PARAM(flags))
	{
		grcAssert(fence->state == __grcFenceHandle::STATE_MARKED_PENDING);
		const int queryIndex = s_FenceQueriesFreeList.AtomicFindClearBitIndexAndSet();
		FatalAssert(queryIndex >= 0);
		g_grcCurrentContext->End(s_FenceQueries[queryIndex]);
		fence->queryIndex = queryIndex;
		fence->state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_QUERY_ISSUED;
	}

	bool grcDevice::IsFencePending(grcFenceHandle fence, u32 UNUSED_PARAM(flags))
	{
		switch (fence->state)
		{
			case __grcFenceHandle::STATE_MARKED_PENDING:
				return true;

			case __grcFenceHandle::STATE_QUERY_ISSUED:
			{
				const unsigned queryIndex = fence->queryIndex;
				ID3D11Query* query = s_FenceQueries[queryIndex];
				BOOL bIsGPUWorkDone = FALSE;
				const UINT dataSize = sizeof(BOOL);
				HRESULT hr = g_grcCurrentContext->GetData(query, &bIsGPUWorkDone, dataSize, 0);
				if ((hr != S_OK) || !bIsGPUWorkDone)
				{
					CHECK_HRESULT(hr);
					CheckDxHresultFatal(hr); // Check for crashed driver
					return true;
				}
				else
				{
					fence->state = (__grcFenceHandle::StateType)__grcFenceHandle::STATE_DONE;
					s_FenceQueriesFreeList.AtomicClear(queryIndex);
					return false;
				}
			}

			case __grcFenceHandle::STATE_DONE:
				return false;

#		  if __ASSERT
			case __grcFenceHandle::STATE_FREE:
			default:
				grcAssertf(0, "invalid fence state");
#		  endif
		}
		return true;
	}

	void grcDevice::CpuWaitOnFence(grcFenceHandle fence)
	{
		while (Unlikely(IsFencePending(fence)))
		{
			sysIpcSleep(1);
		}
	}

	void grcDevice::CpuFreeFence(grcFenceHandle fence)
	{
		// Allow the free to go through if the device is lost (since the query is lost in this case anyway)
		grcAssert(fence->state != __grcFenceHandle::STATE_QUERY_ISSUED || IsInsideDeviceChange() ); 
		s_FencePool.ImmediateFree(fence);
	}

	void grcDevice::GpuWaitOnPreviousWrites()
	{
		// NOP.  D3D takes care of this with automagic hazzard tracking.
	}

#endif

#if RSG_DURANGO

	// This is an experimentally found value.  Initially tried setting to the
	// total number of SIMDs (which is 48 = 2xSEs*6xCUs*4xSIMDs on Durango), but
	// that was way too few wavefronts.
	static bank_u32 MEMORYCS_MAX_WAVEFRONTS = 0x10000;

	void grcDevice::ShaderMemcpy(void *dst, const void *src, size_t size, u32 maxWavefronts)
	{
		grcAssert(((uptr)dst&3) == 0);
		grcAssert(((uptr)src&3) == 0);
		grcAssert((size&63) == 0);

		GRC_ALLOC_SCOPE_AUTO_PUSH_POP();

		maxWavefronts = Min<size_t>(maxWavefronts, MEMORYCS_MAX_WAVEFRONTS);

		grcContextHandle *const ctx = g_grcCurrentContext;

		s_MemoryCS->Bind((grcEffectTechnique)CS_GpuCopy512);

		while (size)
		{
			CompileTimeAssert(S_COMMON_BUFFER_UAV_SIZE == S_COMMON_BUFFER_SRV_SIZE);
			const size_t sizeThisLoop = Min<size_t>(size, S_COMMON_BUFFER_UAV_SIZE);
			size -= sizeThisLoop;

			auto *const cbuf = grcGfxContext::current()->allocateTemp<GpuCopy512_cbuf>();
			const u32 numThreadsWorthOfData = (u32)(sizeThisLoop>>6);
			const u32 numWavefronts         = Min((numThreadsWorthOfData+63)>>6, maxWavefronts);
			const u32 numThreads            = Min(numThreadsWorthOfData, numWavefronts<<6);
			cbuf->Size                      = sizeThisLoop;
			cbuf->Stride                    = numWavefronts<<12;
			ctx->SetFastResources(
				(D3D11X_SET_FAST)(D3D11X_SET_FAST_CS_WITH_OFFSET|D3D11X_SET_FAST_TYPE_BUFFER_VIEW), MEMORYCS_CBUF_ARGS_SLOT,  s_CommonBufferCB,  (uptr)cbuf-FIXED_PLACEMENT_BASE,
				(D3D11X_SET_FAST)(D3D11X_SET_FAST_CS_WITH_OFFSET|D3D11X_SET_FAST_TYPE_BUFFER_VIEW), MEMORYCS_SRC_BUF_SLOT,   &s_CommonBufferSRV, (uptr)src,
				(D3D11X_SET_FAST)(D3D11X_SET_FAST_CS_WITH_OFFSET|D3D11X_SET_FAST_TYPE_BUFFER_VIEW), MEMORYCS_DST_BUF_SLOT,   &s_CommonBufferUAV, (uptr)dst);

			ctx->Dispatch(numThreads, 1, 1);

			dst = (char*)dst + sizeThisLoop;
			src = (char*)src + sizeThisLoop;
		}

		s_MemoryCS->UnBind();
	}

	void grcDevice::ShaderMemset32(void *dst, u32 value, size_t size, u32 maxWavefronts)
	{
		grcAssert(((uptr)dst&3) == 0);
		grcAssert((size&63) == 0);

		GRC_ALLOC_SCOPE_AUTO_PUSH_POP();

		maxWavefronts = Min<size_t>(maxWavefronts, MEMORYCS_MAX_WAVEFRONTS);

		grcContextHandle *const ctx = g_grcCurrentContext;

		s_MemoryCS->Bind((grcEffectTechnique)CS_GpuFill512);

		while (size)
		{
			const size_t sizeThisLoop = Min<size_t>(size, S_COMMON_BUFFER_UAV_SIZE);
			size -= sizeThisLoop;

			auto *const cbuf = grcGfxContext::current()->allocateTemp<GpuFill512_cbuf>();
			const u32 numThreadsWorthOfData = (u32)(sizeThisLoop>>6);
			const u32 numWavefronts         = Min((numThreadsWorthOfData+63)>>6, maxWavefronts);
			const u32 numThreads            = Min(numThreadsWorthOfData, numWavefronts<<6);
			cbuf->Size                      = sizeThisLoop;
			cbuf->Stride                    = numWavefronts<<12;
			cbuf->Value                     = value;
			ctx->SetFastResources(
				(D3D11X_SET_FAST)(D3D11X_SET_FAST_CS_WITH_OFFSET|D3D11X_SET_FAST_TYPE_BUFFER_VIEW), MEMORYCS_CBUF_ARGS_SLOT,  s_CommonBufferCB,  (uptr)cbuf-FIXED_PLACEMENT_BASE,
				(D3D11X_SET_FAST)(D3D11X_SET_FAST_CS_WITH_OFFSET|D3D11X_SET_FAST_TYPE_BUFFER_VIEW), MEMORYCS_DST_BUF_SLOT,   &s_CommonBufferUAV, (uptr)dst);

			ctx->Dispatch(numThreads, 1, 1);

			dst = (char*)dst + sizeThisLoop;
		}

		s_MemoryCS->UnBind();
	}

#endif // RSG_DURANGO

void grcDevice::KickOffGpu()
{
	g_grcCurrentContext->GetData(s_KickEventQuery, NULL, 0, 0);
}

void grcDevice::SetScissor(int x,int y,int width,int height)
{
	RECT *const rect = sm_scissorRects;
	sm_numScissorRects = 1;
	rect->left = x;
	rect->right = x + width;
	rect->top = y;
	rect->bottom = y + height;
	Assert(rect->left >= 0);
	Assert(rect->top >= 0);
	Assert(rect->right-rect->left <= GetWidth());
	Assert(rect->bottom-rect->top <= GetHeight());

	if(rect->left < 0)
	{
		rect->left = 0;
	}

	if(rect->top < 0)
	{
		rect->top = 0;
	}

	if(rect->right > GetWidth())
	{
		rect->right = GetWidth();
	}

	if(rect->bottom > GetHeight())
	{
		rect->bottom = GetHeight();
	}

#if RSG_PC
	if (GRCDEVICE.CanUseStereo() && GRCDEVICE.IsStereoEnabled() && sm_bStereoScissor)
	{
		int separation = (int) ((sm_fStereoSeparationPercentage * sm_fEyeSeparation) * GetWidth());
		rect->left = Max(0,(int)(rect->left) - separation );
		rect->right = rect->right + separation;
	}
#endif
	g_grcCurrentContext->RSSetScissorRects(1, rect);
}

void grcDevice::DisableScissor()
{
	RECT *const rect = sm_scissorRects;
	sm_numScissorRects = 1;
	rect->top    = 0;
	rect->left   = 0;
	rect->bottom = 8192;
	rect->right  = 8192;
	g_grcCurrentContext->RSSetScissorRects(1, rect);
}

void grcDevice::GetScissor(int &x,int &y,int &width,int &height)
{
	UINT uRectCount = 1;
	D3D11_RECT oRect;
	g_grcCurrentContext->RSGetScissorRects(&uRectCount, &oRect);
	Assert(uRectCount == 1);
	x = oRect.left;
	y = oRect.top;
	width = oRect.right - oRect.left;
	height = oRect.bottom - oRect.top;
}

void grcDevice::SetInputLayout()
{
	SetInputLayout(sm_CurrentVertexProgram, sm_CurrentVertexDeclaration);
}

void grcDevice::SetInputLayout(grcVertexProgram *vp,grcVertexDeclaration * decl)
{
	Assert(vp != NULL);
	Assert(decl != NULL);
	vp->Bind(decl);
}

void grcDevice::CreateShaderConstantBuffer(int ByteSize, grcBuffer **pData NOTFINAL_ONLY(, const char* pName))
{
    D3D11_BUFFER_DESC cbDesc = {0};
    cbDesc.ByteWidth = ByteSize;
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
	*pData = NULL;

	CHECK_HRESULT(sm_Current->CreateBuffer(&cbDesc, NULL, (ID3D11Buffer**)pData));

#if !__FINAL && !__D3D11_MONO_DRIVER
	if( *pData && pName )
		CHECK_HRESULT(((ID3D11DeviceChild*)*pData)->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen(pName)+1,pName));
#elif !__FINAL // !__FINAL && !__D3D11_MONO_DRIVER
	pName;
#endif // !__FINAL && !__D3D11_MONO_DRIVER
}

void grcDevice::GetRenderTargets(u32 numTargets, grcDeviceView **papRenderTargets, grcDeviceView **ppDepthTarget)
{
	AssertMsg(g_grcCurrentContext, "Device not created yet.");
	g_grcCurrentContext->OMGetRenderTargets(numTargets, (ID3D11RenderTargetView**)papRenderTargets, (ID3D11DepthStencilView**)ppDepthTarget);
}

void grcDevice::SetRenderTargets(u32 numTargets, const grcDeviceView **papRenderTargets, const grcDeviceView *pDepthTarget)
{
	SetupRenderTargetVars( numTargets, papRenderTargets, pDepthTarget);

	g_grcCurrentContext->OMSetRenderTargets(numTargets, (ID3D11RenderTargetView**)papRenderTargets, (ID3D11DepthStencilView*)pDepthTarget);
}

void grcDevice::SetRenderTargetsWithUAV(u32 numTargets, const grcDeviceView **papRenderTargets, const grcDeviceView *pDepthTarget, u32 UAVStartSlot, u32 numUAVs, grcDeviceView ** ppUnorderedAccessView, const u32* pUAVInitialCounts)
{
	SetupRenderTargetVars(numTargets, papRenderTargets, pDepthTarget);

	g_grcCurrentContext->OMSetRenderTargetsAndUnorderedAccessViews(numTargets, (ID3D11RenderTargetView**)papRenderTargets, (ID3D11DepthStencilView*)pDepthTarget, UAVStartSlot, numUAVs, (ID3D11UnorderedAccessView**)ppUnorderedAccessView, pUAVInitialCounts);
}

void grcDevice::SetupRenderTargetVars(u32 numTargets, const grcDeviceView **papRenderTargets, const grcDeviceView *pDepthTarget)
{
	AssertMsg(sm_Current, "Device not created yet.");

	u32 uIndex;
	for (uIndex = 0; uIndex < numTargets; uIndex++)
	{
		sm_aRTView[uIndex].pRTView = (ID3D11RenderTargetView*)papRenderTargets[uIndex];
	}
	for (uIndex; uIndex < grcmrtColorCount; uIndex++)
	{
		sm_aRTView[uIndex].pRTView = NULL;
	}

	sm_PreviousDepthView = sm_DepthView;
	sm_DepthView.pRTView = (ID3D11RenderTargetView*)pDepthTarget;

	if( !pDepthTarget )
	{
		#if __ASSERT && 0
		// Find out who tries to do depth/stencil test
		// without a depth/stencil target attached
		// (would produce undefined result)
		ID3D11DepthStencilState *pState = NULL;
		UINT stencilRef = 0;
		g_grcCurrentContext->OMGetDepthStencilState(&pState,&stencilRef);
		if (pState)
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			pState->GetDesc(&desc);
			Assert( !desc.DepthEnable && !desc.StencilEnable );
		}
		#endif
		grcStateBlock::SetDepthStencilState(sm_WriteNoDepthNoStencil);
	}

	sm_numTargets = numTargets;

	//In DX11 the viewport doesn't get set when the render target is set like it does in DX9 so set it here.
	D3D11_VIEWPORT *const vp = sm_viewports;
	sm_numViewports = 1;
	vp->Width = (float)sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	vp->Height = (float)sm_CurrentWindows[g_RenderThreadIndex].uHeight;
	g_grcCurrentContext->RSSetViewports(1, vp);

	DisableScissor();

	//Any reason why we're resetting this to default?
	//TODO: use active viewport's Window here instead of using temp!!
	grcWindow temp;
	SetWindow(temp);
}


void grcDevice::SetDepthBuffer( const grcDeviceView* pDepthBufferTargetView )
{
	ID3D11RenderTargetView *pRenderTargets[grcmrtColorCount];
	for (u32 uIndex = 0; uIndex < sm_numTargets; uIndex++)
	{
		pRenderTargets[uIndex] = sm_aRTView[uIndex].pRTView;
	}

	sm_PreviousDepthView = sm_DepthView;
	sm_DepthView.pRTView = (ID3D11RenderTargetView*)pDepthBufferTargetView;

	g_grcCurrentContext->OMSetRenderTargets( sm_numTargets, pRenderTargets, (ID3D11DepthStencilView*)sm_DepthView.pRTView );
}


#if DEVICE_RESOLVE_RT_CONFLICTS
void grcDevice::NotifyTargetLocked(const grcDeviceView* view)
{
	for(int i=0; i<EFFECT_TEXTURE_COUNT; ++i)
	{
		if (s_ResourceViews[i] == view)
			s_ResourceViews[i] = NULL;
	}
}
#endif // DEVICE_RESOLVE_RT_CONFLICTS

void grcDevice::CopyStructureCount( grcBufferBasic* pDestBuffer, u32 DstAlignedByteOffset, grcBufferUAV* pSrcBuffer )
{
	grcContextHandle *const ctx = g_grcCurrentContext;
	ctx->CopyStructureCount( pDestBuffer->GetD3DBuffer(), DstAlignedByteOffset, (ID3D11UnorderedAccessView*)pSrcBuffer->GetUnorderedAccessView() );

#if RSG_DURANGO
	// Currently there is no way to bind a UAV buffer such that writing to it bypasses L1 (really want something
	// like sce::Gnm::Buffer::setResourceMemoryType(kResourceMemoryTypeGC).  So as a work around, we invalidate
	// the GPU's L1 caches.
	//
	// This is required since otherwise the CS or PS shader that was writing to the append buffer may still have
	// some of the buffer in its L1.  If another shader then reads from it on a different CU, then it will read
	// stale data.
	//
	// Asked about this issue at https://forums.xboxlive.com/AnswerPage.aspx?qid=c4af55b4-f5d1-48d6-8081-8fe1adb5398c&tgt=1.
	// Hopefully we will get a better solution than this L1 invalidate.
	//
	// Note that CopyStructureCount blocks the ME, so that ensures this cache invalidation doesn't occur too early.
	//
	ctx->FlushGpuCacheRange(D3D11_FLUSH_TEXTURE_L1_INVALIDATE, pSrcBuffer->m_pGraphicsMemory, pSrcBuffer->m_Size);
#endif
}

void grcDevice::DrawWithGeometryShader( grcBufferBasic* pIndirectBuffer )
{
	grcStateBlock::Flush();

	ID3D11Buffer* vertexBuffers[2] = { NULL };
	UINT strides[2] = { 0 };
	UINT offsets[2] = { 0 };
	g_grcCurrentContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	g_grcCurrentContext->IASetInputLayout(NULL);
	g_grcCurrentContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
#if !RSG_DURANGO
	g_grcCurrentContext->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);
#endif

	SetProgramResources();

	// Draw the bokeh points
	g_grcCurrentContext->DrawInstancedIndirect(pIndirectBuffer->GetD3DBuffer(), 0);
#if DEBUG_SEALING_OF_DRAWLISTS
	grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
}

void grcDevice::DrawIndexedInstancedIndirect(grcBufferBasic* pIndirectBuffer, grcDrawMode dm, bool alreadySetupPriorToDraw, u32 argsOffsetInBytes)
{
	if(pIndirectBuffer)
	{
		grcStateBlock::Flush();

		if( !alreadySetupPriorToDraw )
			SetUpPriorToDraw(dm);

		g_grcCurrentContext->DrawIndexedInstancedIndirect(pIndirectBuffer->GetD3DBuffer(), argsOffsetInBytes);
#if DEBUG_SEALING_OF_DRAWLISTS
		grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
	}
}

#if RSG_DURANGO
void grcDevice::DrawIndexedInstancedIndirect(void *pIndirectBufferMem, grcDrawMode dm, bool alreadySetupPriorToDraw, u32 argsOffsetInBytes)
{
	if(pIndirectBufferMem)
	{
		void *args = reinterpret_cast<void *>(reinterpret_cast<char *>(pIndirectBufferMem) + argsOffsetInBytes);
		uptr offset = reinterpret_cast<uptr>(args);

		if(Verifyf(offset < 1ull << 40, "Is \'0x%p\' a valid GPU address?!?", args))
		{
			//Find which 2gb boundary contains this pointer.
			static const u64 s2GB = 1ull << 31;
			static const u64 s2GBBoundaryMask = ~(s2GB - 1ull);
			u64 boundary2GB = offset & s2GBBoundaryMask;
			u64 buffIndex = boundary2GB >> 31;
			u64 buffOffset = offset - boundary2GB;

			Assert(buffIndex < static_cast<u64>(s_CommonFullAddressPlacementBuffers.size()));
			Assert(buffOffset < 1ull << 32);

			//Draw
			grcStateBlock::Flush();

			if( !alreadySetupPriorToDraw )
				SetUpPriorToDraw(dm);

			g_grcCurrentContext->DrawIndexedInstancedIndirect(s_CommonFullAddressPlacementBuffers[buffIndex], buffOffset);
#if DEBUG_SEALING_OF_DRAWLISTS
			grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
		}
	}
}
#endif //RSG_DURANGO

void grcDevice::ClearRect(int x,int y,int width,int height,float depth,const Color32 & color)
{
	ClearRect(x,y,width,height,true,color,true,depth,true,0);
}

void grcDevice::ClearRect(int x,int y,int width,int height,bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth,bool enableClearStencil,u32 clearStencil, bool yFlipped)
{
	Assert(width > 0 && height > 0);

	if (!g_grcCurrentContext) 
		return;

	PIXBeginN(0,"ClearRect(%d,%d,%d,%d,%08x(%s),%.1f(%s),%d(%s))",x,y,width,height,clearColor.GetDeviceColor(),enableClearColor?"on":"off",clearDepth,enableClearDepth?"on":"off",clearStencil,enableClearStencil?"on":"off");

	grcBlendStateHandle BS_prev = grcStateBlock::BS_Active;
	grcDepthStencilStateHandle DSS_prev = grcStateBlock::DSS_Active;
	grcRasterizerStateHandle RS_prev = grcStateBlock::RS_Active;

#if __ASSERT
	int numBoundTargets=0; 
	for (u32 uIndex = 0; uIndex < grcmrtColorCount; uIndex++)
	{
		if (sm_aRTView[uIndex].pRTView != NULL)
			numBoundTargets++;
	}
	Assertf( numBoundTargets==1, "ClearRect() currently supports a single target and we have multiple bound. Only target 0 will be cleared" );
#endif // _ASSERT

	grcStateBlock::SetBlendState(clearStates_BS[enableClearColor]);
	grcStateBlock::SetDepthStencilState(clearStates_DSS[enableClearDepth][enableClearStencil],clearStencil);
	grcStateBlock::SetRasterizerState(grcStateBlock::RS_NoBackfaceCull);

	float fLeft = ((2 * x) / (float)sm_CurrentWindows[g_RenderThreadIndex].uWidth) - 1.0f;
	float fRight = ((2 * (x + width)) / (float)sm_CurrentWindows[g_RenderThreadIndex].uWidth) - 1.0f;

	float fTop = ((2 * y) / (float)sm_CurrentWindows[g_RenderThreadIndex].uHeight) - 1.0f;
	float fBottom = ((2 * (y + height)) / (float)sm_CurrentWindows[g_RenderThreadIndex].uHeight) - 1.0f;
	if (yFlipped)
	{
		fTop = -fTop;
		fBottom = -fBottom;
	}
	
	g_DefaultEffect->SetVar(g_DefaultColor, clearColor);

	g_DefaultEffect->Bind(s_DefaultClear);
	grcBegin(drawTriStrip, 4);
		grcColor(clearColor);
		grcTexCoord2f(0.0f, 0.0f);
		grcNormal3f(0.0f, 0.0f, 0.0f);
		grcVertex3f(fLeft,  fTop,	 clearDepth);		// TL
		grcVertex3f(fRight, fTop,	 clearDepth);		// TR
		grcVertex3f(fLeft,  fBottom, clearDepth);		// BL
		grcVertex3f(fRight, fBottom, clearDepth);		// BR
	grcEnd();
	g_DefaultEffect->UnBind();

	grcStateBlock::SetDepthStencilState(DSS_prev);
	grcStateBlock::SetBlendState(BS_prev);
	grcStateBlock::SetRasterizerState(RS_prev);

	PIXEnd();
}


void grcDevice::Clear(bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth,bool enableClearStencil,u32 clearStencil) {
	WIN32PC_ONLY(if (!g_grcCurrentContext) return);

	float fColor[4] = { clearColor.GetRedf(), clearColor.GetGreenf(), clearColor.GetBluef(), clearColor.GetAlphaf() };

	if (enableClearColor)
	{
		// PC TODO - Look for redundant clears
		for (u32 uIndex = 0; uIndex < grcmrtColorCount; uIndex++)
		{
			if (sm_aRTView[uIndex].pRTView != NULL)
			{
				g_grcCurrentContext->ClearRenderTargetView(sm_aRTView[uIndex].pRTView, fColor);
			}
		}
	}

	u32 depthClearFlags = (enableClearDepth ? D3D11_CLEAR_DEPTH : 0) | (enableClearStencil ? D3D11_CLEAR_STENCIL : 0);
	if (enableClearDepth || enableClearStencil)
	{
		if (sm_DepthView.pRTView != NULL)
		{
			g_grcCurrentContext->ClearDepthStencilView((ID3D11DepthStencilView *)sm_DepthView.pRTView, depthClearFlags, FixDepth(clearDepth), (u8)clearStencil);
		}
	}
}

void grcDevice::ClearUAV(bool bAsFloat, grcBufferUAV* pBuffer)
{
	const float afValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	const UINT aiValue[4] = { 0, 0, 0, 0 };

	if (bAsFloat)
		g_grcCurrentContext->ClearUnorderedAccessViewFloat((ID3D11UnorderedAccessView*)pBuffer->GetUnorderedAccessView(), afValue);
	else
		g_grcCurrentContext->ClearUnorderedAccessViewUint((ID3D11UnorderedAccessView*)pBuffer->GetUnorderedAccessView(), aiValue);
}

#if	DEVICE_CLIP_PLANES

// DX11 TODO:- This functionality only works because higher level code sets the viewport prior to setting clip planes. 
// Work needs to be done at viewport level to track and transmit camera/projection changes to device level (RegenerateMatrices() is called
// upon ALL matrix changes, we`re only interested in camera/projection ones).
void grcDevice::ResolveClipPlanes()
{
	ASSERT_ONLY
	(
		if(sm_ClipPlaneEnable[g_RenderThreadIndex])
		{
			grcAssertf(sm_CurrentVertexProgram != NULL, "grcDevice::ResolveClipPlanes()...Vertex program not set!");

			if(sm_CurrentDomainProgram->GetProgram())
			{
				grcAssertf(sm_CurrentDomainProgram->DoesUseCBuffer(sm_pClipPlanesConstBuffer), "grcDevice::ResolveClipPlanes()...%s doesn`t use clip planes!", sm_CurrentDomainProgram->GetEntryName());
			}
			else
			{
				grcAssertf(sm_CurrentVertexProgram->DoesUseCBuffer(sm_pClipPlanesConstBuffer), "grcDevice::ResolveClipPlanes()...%s doesn`t use clip planes!", sm_CurrentVertexProgram->GetEntryName());
			}
		}
	);

	// Has the clip plane state not changed ?
	if((sm_ClipPlanesChanged[g_RenderThreadIndex] == 0) && (sm_PreviousClipPlaneEnable[g_RenderThreadIndex] == sm_ClipPlaneEnable[g_RenderThreadIndex]))
	{
		return;
	}

	grcAssertf(sm_pClipPlanesConstBuffer != NULL, "grcDevice::ResolveClipPlanes()...We require a clip plane constant buffer!");

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
	grcClipPlanes *pDest = (grcClipPlanes *)sm_pClipPlanesConstBuffer->GetDataPtr();

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
	
	sm_pClipPlanesConstBuffer->Unlock();
	sm_PreviousClipPlaneEnable[g_RenderThreadIndex] = sm_ClipPlaneEnable[g_RenderThreadIndex];
	sm_ClipPlanesChanged[g_RenderThreadIndex] = 0x0;
}


void grcDevice::ResetClipPlanes()
{

	sm_ClipPlaneEnable[g_RenderThreadIndex] = 0;
	sm_PreviousClipPlaneEnable[g_RenderThreadIndex] = 0;
	sm_ClipPlanesChanged[g_RenderThreadIndex] = 0;

	if(sm_pClipPlanesConstBuffer)
	{
		u32 i;
		grcClipPlanes *pDest = (grcClipPlanes *)sm_pClipPlanesConstBuffer->GetDataPtr();
	
		for(i=0; i<RAGE_MAX_CLIPPLANES; i++)
		{
			pDest->ClipPlanes[i] = Vec4V(V_ZERO);
		}

		sm_pClipPlanesConstBuffer->Unlock();
	}
}


void grcDevice::SetClipPlanesConstBuffer(grcCBuffer *pConstBuffer)
{
	sm_pClipPlanesConstBuffer = pConstBuffer;
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

#if __WIN32PC
void grcDevice::SetTexture(int stage,const grcTexture *pTexture)
{
	(void)stage;
	(void*)pTexture;
	
	//pProgram->SetTexContainer(slot,pTexture);
	//pEffectVar10->AsShaderResource()->SetResource((ID3D11ShaderResourceView*)pTexture->GetTextureView());
}
#endif

void grcDevice::CreateVertexShader(u8 *programData, u32 programSize, grcVertexShader **Program)
{
	AssertMsg(sm_Current, "Device not created yet.");
	ID3D11VertexShader *ProgramDX11;

	AssertVerify(sm_Current->CreateVertexShader((DWORD*)programData, programSize, NULL, &ProgramDX11) == S_OK);
	*Program = (grcVertexShader*)ProgramDX11;
}

void grcDevice::CreatePixelShader(u8 *programData, u32 programSize, grcPixelShader **Program)
{
	ID3D11PixelShader *ProgramDX11;

	AssertVerify(sm_Current->CreatePixelShader((DWORD*)programData, programSize, NULL, &ProgramDX11) == S_OK);
	*Program = (grcPixelShader*)ProgramDX11;
}

void grcDevice::CreateComputeShader(u8 *programData, u32 programSize, grcComputeShader **Program)
{
	ID3D11ComputeShader *ProgramDx11;

	AssertVerify(sm_Current->CreateComputeShader((DWORD*)programData, programSize, NULL, &ProgramDx11) == S_OK);
	*Program = (grcComputeShader*)ProgramDx11;
}
void grcDevice::CreateDomainShader(u8 *programData, u32 programSize, grcDomainShader **Program)
{
	AssertMsg(GetDxFeatureLevel() >= 1100, "Domain shaders available valid for cards with shader model >= 1100" );
	
	ID3D11DomainShader *ProgramDx11;
	AssertVerify(sm_Current->CreateDomainShader((DWORD*)programData, programSize, NULL, &ProgramDx11) == S_OK);
	*Program = (grcDomainShader*)ProgramDx11;
}
void grcDevice::CreateGeometryShader(u8 *programData, u32 programSize, grcGeometryShader **Program)
{
	ID3D11GeometryShader *ProgramDX11;
	AssertVerify(sm_Current->CreateGeometryShader((DWORD*)programData, programSize, NULL, &ProgramDX11) == S_OK);
	*Program = (grcGeometryShader*)ProgramDX11;
}
void grcDevice::CreateHullShader(u8 *programData, u32 programSize, grcHullShader **Program)
{
	AssertMsg(GetDxFeatureLevel() >= 1100, "Hull shaders available valid for cards with shader model >= 1100" );

	ID3D11HullShader *ProgramDx11;
	AssertVerify(sm_Current->CreateHullShader((DWORD*)programData, programSize, NULL, &ProgramDx11) == S_OK);
	*Program = (grcHullShader*)ProgramDx11;
}
void grcDevice::CreateGeometryShaderWithStreamOutput(u8 *programData, u32 programSize, grcStreamOutputDeclaration *pSODeclaration, u32 NumEntries, u32 OutputStreamStride, grcGeometryShader **ppGeometryShader)
{
	ID3D11GeometryShader *ProgramDX11;
	(void)OutputStreamStride;
	// Ok to use Geometry stage again?
	AssertVerify(sm_Current->CreateGeometryShaderWithStreamOutput((DWORD*)programData, programSize, reinterpret_cast<D3D11_SO_DECLARATION_ENTRY*>(pSODeclaration), NumEntries, NULL, 0, 0, NULL, &ProgramDX11 ) == S_OK);
	*ppGeometryShader = (grcGeometryShader*)ProgramDX11;
}

//--------------------------------------------------------------------------//
//	grcProgram and grcComputeProgram methods								//
//--------------------------------------------------------------------------//

#if RSG_PC

void grcVertexProgram::SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers)		{ g_grcCurrentContext->VSSetConstantBuffers(StartSlot, NumSlots, (ID3D11Buffer**)ppBuffers); }
void grcFragmentProgram::SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers)	{ g_grcCurrentContext->PSSetConstantBuffers(StartSlot, NumSlots, (ID3D11Buffer**)ppBuffers); }
void grcComputeProgram::SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers)		{ g_grcCurrentContext->CSSetConstantBuffers(StartSlot, NumSlots, (ID3D11Buffer**)ppBuffers); }
void grcDomainProgram::SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers)		{ g_grcCurrentContext->DSSetConstantBuffers(StartSlot, NumSlots, (ID3D11Buffer**)ppBuffers); }
void grcGeometryProgram::SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers)	{ g_grcCurrentContext->GSSetConstantBuffers(StartSlot, NumSlots, (ID3D11Buffer**)ppBuffers); }
void grcHullProgram::SetConstantBuffers(u32 StartSlot, u32 NumSlots, grcBuffer ** ppBuffers)		{ g_grcCurrentContext->HSSetConstantBuffers(StartSlot, NumSlots, (ID3D11Buffer**)ppBuffers); }

void grcVertexProgram::ClearConstantBuffers()	{ static_cast <RageDirect3DDeviceContext11 *>(g_grcCurrentContext)->VSClearConstantBuffers(); }
void grcFragmentProgram::ClearConstantBuffers()	{ static_cast <RageDirect3DDeviceContext11 *>(g_grcCurrentContext)->PSClearConstantBuffers(); }
void grcComputeProgram::ClearConstantBuffers()	{ static_cast <RageDirect3DDeviceContext11 *>(g_grcCurrentContext)->CSClearConstantBuffers(); }
void grcDomainProgram::ClearConstantBuffers()	{ static_cast <RageDirect3DDeviceContext11 *>(g_grcCurrentContext)->DSClearConstantBuffers(); }
void grcGeometryProgram::ClearConstantBuffers()	{ static_cast <RageDirect3DDeviceContext11 *>(g_grcCurrentContext)->GSClearConstantBuffers(); }
void grcHullProgram::ClearConstantBuffers()		{ static_cast <RageDirect3DDeviceContext11 *>(g_grcCurrentContext)->HSClearConstantBuffers(); }

bool grcProgram::SetConstantBufferData(ShaderType eType) const
{
	if (m_numCBuffers <= 0)
		return false;

	for (int i = 0; i < m_numCBuffers; i++)
	{
		grcCBuffer * pCBuffer = m_pCBuffers[i];

		if(pCBuffer->DoesUnlockNeedToBeCalled())
		{
			pCBuffer->Unlock();
		}
	}

	if (s_Fingerprints[eType] == m_CBufferFingerprint BANK_ONLY(&& sm_useCBFingerprints))
		return false;
	s_Fingerprints[eType] = m_CBufferFingerprint;
	return true;
}

template <class T>
void grcProgram::SetConstantBuffer(grcCBuffer **ppOverrides, u32 overrideOffset, u32 numOverrides) const
{
	if (!SetConstantBufferData(T::GetShaderType()) && !numOverrides)
		return;

	grcBuffer **	pBuffer = (ID3D11Buffer **)m_ppDeviceCBuffers;
	grcBuffer **	ppBufferStart = &(pBuffer[m_CBufStartSlot]);
	UINT			NumBuffers = (m_CBufEndSlot-m_CBufStartSlot+1);
	Assert(pBuffer != NULL);

	grcBuffer * pOverriddenBuffers[MAX_CONSTANT_BUFFER];
	if (ppOverrides)
	{
		memcpy(&(pOverriddenBuffers[m_CBufStartSlot]), ppBufferStart, sizeof(ID3D11Buffer*)*NumBuffers);

 		pBuffer = (ID3D11Buffer **)&(pOverriddenBuffers[0]);
 		ppBufferStart = &(pOverriddenBuffers[m_CBufStartSlot]);

		for (u32 overrideIdx=0; overrideIdx<numOverrides; ++overrideIdx)
		{
			u32 Slot = overrideIdx+overrideOffset;
			if (ppOverrides[overrideIdx])
			{
				pBuffer[Slot] = (ID3D11Buffer*)(ppOverrides[overrideIdx]->GetBuffer());
			}
		}
	}

	T::SetConstantBuffers(m_CBufStartSlot, NumBuffers, ppBufferStart);
}

template<class T>
void grcProgram::SetConstantBuffer() const
{
	ShaderType eType = T::GetShaderType();
	grcAssertf(eType != VS_TYPE, "Do not call set constant buffer on vertex buffer in case of overrides");

	u32 stagesDisabled = s_ShaderStagesLastActive & ~s_ShaderStagesActive;
	if (BANK_ONLY(s_disableUnusedStages &&) stagesDisabled & (1 << eType)) 
	{
		T::ClearConstantBuffers();
		s_Fingerprints[eType] = 0;
		return;
	}
	if (!SetConstantBufferData(eType))
		return;

	grcBuffer **	pBuffer = (grcBuffer **)m_ppDeviceCBuffers;
	grcBuffer **	ppBufferStart = &(pBuffer[m_CBufStartSlot]);
	UINT			NumBuffers = (m_CBufEndSlot-m_CBufStartSlot+1);
	Assert(pBuffer != NULL);
	
	T::SetConstantBuffers(m_CBufStartSlot, NumBuffers, ppBufferStart);
}

#endif // RSG_PC

#if __DEV
char* astrShaderType[NONE_TYPE] =
{
	"VS_TYPE",
	"PS_TYPE",
	"CS_TYPE",
	"DS_TYPE",
	"GS_TYPE",
	"HS_TYPE"
};
#endif

DECLARE_MTR_THREAD bool g_ProgramResourcesReset = false;
DECLARE_MTR_THREAD grcProgramResource g_ProgramResources[NONE_TYPE][EFFECT_TEXTURE_COUNT];
DECLARE_MTR_THREAD bool g_ComputeProgramResourcesReset = false;
DECLARE_MTR_THREAD grcComputeProgramResource g_ComputeProgramResources[EFFECT_UAV_BUFFER_COUNT];

void grcProgram::RecordProgramResourceForVectorDXAPICall(ShaderType unit, int slot, void *data)
{
	grcAssertf(unit == PS_TYPE, "RecordProgramResourceForVectorSet()...Only supports fragment shaders at the moment.");
	grcAssertf(slot < EFFECT_TEXTURE_COUNT, "grcProgram::RecordProgramResource()...Slot of out bounds.");
	grcAssertf(unit < NONE_TYPE, "grcProgram::RecordProgramResource()...Unit out of bounds.");

	if(g_ProgramResourcesReset == false)
	{
		int i, j;

		for(i=0; i<NONE_TYPE; i++)
			for(j=0; j<EFFECT_TEXTURE_COUNT; j++)
				g_ProgramResources[i][j].pAny = NULL;

		g_ProgramResourcesReset = true;
	}
	g_ProgramResources[unit][slot].pAny = data;
}

void grcProgram::RecordComputeProgramResourceForVectorDXAPICall(int slot, void *data, int initCount)
{
	grcAssertf(slot < EFFECT_UAV_BUFFER_COUNT, "grcProgram::RecordComputeProgramResourceForVectorDXAPICall()...Slot out of bounds.");

	if(g_ComputeProgramResourcesReset == false)
	{
		int i;

		for(i=0; i<EFFECT_UAV_BUFFER_COUNT; i++)
		{
			g_ComputeProgramResources[i].pAny = NULL;
			g_ComputeProgramResources[i].unorderedCount = 0;
		}
		g_ComputeProgramResourcesReset = true;
	}
	g_ComputeProgramResources[slot].pAny = data;
	g_ComputeProgramResources[slot].unorderedCount = initCount;
}

#if defined(SRC_DST_TRACKING) && SRC_DST_TRACKING
void AddDependency(const grcRenderTargetDX11* pTarget, const grcRenderTargetDX11* pSrc)
{
	if ((pTarget == NULL) || !pTarget->IsValid())
		return;
	if ((pSrc == NULL)  || !pSrc->IsValid())
		return;

	grcTextureFactoryDX11::MultiMapTargetTextureSources::iterator itDstLower, itDstUpper, it;

	sysCriticalSection cs(grcTextureFactoryDX11::sm_Lock);
	itDstLower = grcTextureFactoryDX11::sm_mmapSourceTextureList.lower_bound(pTarget);
	itDstUpper = grcTextureFactoryDX11::sm_mmapSourceTextureList.upper_bound(pTarget);

	bool bFound = false;
	bool bListCreated = false;
	for (it=itDstLower; it!=itDstUpper; ++it)
	{
		const grcRenderTargetDX11 *pListItem = static_cast<const grcRenderTargetDX11*>(it->second);
		bListCreated = true;

		if (pListItem == pSrc)
		{
			bFound = true;
			break;
		}
	}

	if (!bListCreated)
	{
		// Didn't find in list - add a new entry
		grcTextureFactoryDX11::sm_mmapSourceTextureList.insert(std::pair<const grcRenderTarget*, const grcRenderTarget*>(pTarget, NULL));
		Displayf("Target %s First Time", pTarget->GetName());
	}

	if (!bFound)
	{
		// Didn't find in list - add a new entry
		grcTextureFactoryDX11::sm_mmapSourceTextureList.insert(std::pair<const grcRenderTarget*, const grcRenderTarget*>(pTarget, pSrc));
		Displayf("Target %s Adding %s", pTarget->GetName(), pSrc->GetName());
	}
}
#endif // SRC_DST_TRACKING

void grcProgram::SetTextureResourcesUsingVectorDXAPICall(ShaderType eType) const
{
	// This means the program doesn't use any texture resources.
	if (m_numTexContainers == 0)
	{
		return;
	}

	u32							texStartSlot = EFFECT_TEXTURE_COUNT-1;
	u32							texEndSlot = 0;
	bool						usesComplexResources=false;

	// Set the valid pointers into the correct slot.
	for (int i = 0; i < m_numTexContainers; i++)
	{
		if (!m_pTexContainers[i])
		{
#if !__FINAL
			//Errorf("%s Texture %d", astrShaderType[eType], i, m_TexContainer[i]->GetName());
#endif // !__FINAL
			continue;
		}

		const grcEffect::VarType varType = static_cast<grcEffect::VarType>( m_pTexContainers[i]->GetType() );
		const u32 cbOffset = m_pTexContainers[i]->GetCBufOffset();
#if DEVICE_USE_FAST_TEXTURES
		// Keep track of 'complex' resources as the Durango runtime seems to call them. They can't be set with the PSSetFastShaderResource func call
		// Mainly, the docs suggest they only mean render targets at the moment.
		bool isComplexResource=false;
#endif // DEVICE_USE_FAST_TEXTURES

		ID3D11ShaderResourceView* pShaderResource = NULL;
		if(varType == grcEffect::VT_TEXTURE && (g_ProgramResources[eType][cbOffset].GetTexPtr()))
		{
			Assert(cbOffset < MAX_RESOURCES && cbOffset < EFFECT_TEXTURE_COUNT);
			const grcTexture * pTexture = g_ProgramResources[eType][cbOffset].GetTexPtr();
#if DEVICE_USE_FAST_TEXTURES
			isComplexResource = (pTexture->GetResourceType() == grcTexture::RENDERTARGET);
#endif // DEVICE_USE_FAST_TEXTURES
#if D3D11_TRACK_RT_VIOLATIONS
			if (pTexture->GetResourceType() == grcTexture::RENDERTARGET && pTexture->GetName())
			{
				// there are textures (grcTextureDX11) with resource type RENDERTARGET
				// they have name == NULL, and we ignore them
				const grcRenderTargetD3D11 *target = static_cast<const grcRenderTargetD3D11*>( pTexture );
				Assertf( !static_cast<grcTextureFactoryDX11&>(grcTextureFactory::GetInstance()).IsTargetLocked(target),
					"Program %s Texture '%s' binding to slot %d - %s failed: it is being currently locked for rendering",
					GetEntryName(), target->GetName(), cbOffset, m_pTexContainers[i]->GetName() );
#if SRC_DST_TRACKING
				// See if dependency is already there
				grcTextureFactoryDX11& textureFactory11 = static_cast<grcTextureFactoryDX11&>(grcTextureFactoryDX11::GetInstance()); 
				const grcTextureFactoryDX11::TARGETINFO &oTargetInfo = grcTextureFactoryDX11::s_RenderTargetStack[textureFactory11.GetRenderTargetStackCount()];

				// Search to see if already in list	
				const grcRenderTargetDX11 *pSrc = static_cast<const grcRenderTargetDX11*>( pTexture );
				for (u32 uTarget = 0; uTarget < oTargetInfo.uNumViews; uTarget++)
				{
					const grcRenderTargetDX11* pTarget = oTargetInfo.apColorTargets[uTarget];
					AddDependency(pTarget, pSrc);
				}
				const grcRenderTargetDX11* pDepth = oTargetInfo.pDepthTarget;
				AddDependency(pDepth, pSrc);

				// If MRT lock then destinations can't be shared as well
				for (u32 uTarget = 0; uTarget < oTargetInfo.uNumViews; uTarget++)
				{
					const grcRenderTargetDX11* pTarget = oTargetInfo.apColorTargets[uTarget];

					for (u32 uSharedTarget = 0; uSharedTarget < oTargetInfo.uNumViews; uSharedTarget++)
					{
						const grcRenderTargetDX11* pSharedTarget = oTargetInfo.apColorTargets[uSharedTarget];
						AddDependency(pTarget, pSharedTarget);
					}
					const grcRenderTargetDX11* pDepth = oTargetInfo.pDepthTarget;
					AddDependency(pDepth, pTarget);
					AddDependency(pTarget, pDepth);
				}

				// Dependency of Source Texture being used at the same time prevent them from being shared potentially
				for (int j = 0; j < m_numTexContainers; j++)
				{
					if (!m_pTexContainers[j])
					{
						continue;
					}

					const grcEffect::VarType varType = static_cast<grcEffect::VarType>( m_pTexContainers[j]->GetType() );
					const u32 cbSrcOffset = m_pTexContainers[j]->GetCBufOffset();
					if(i != j && varType == grcEffect::VT_TEXTURE && (g_ProgramResources[eType][cbSrcOffset].GetTexPtr()))
					{
						const grcTexture * pSrcTexture = g_ProgramResources[eType][cbSrcOffset].GetTexPtr();
						if (pSrcTexture->GetResourceType() == grcTexture::RENDERTARGET && pSrcTexture->GetName())
						{
							const grcRenderTargetD3D11 *pSrcTarget = static_cast<const grcRenderTargetD3D11*>( pSrcTexture );

							AddDependency(target, pSrcTarget);
						}
					}
				}
#endif // SRC_DST_TRACKING
			}

#endif // D3D11_TRACK_RT_VIOLATIONS
			pShaderResource = (ID3D11ShaderResourceView*)( const_cast<grcTexture*>(pTexture)->GetTextureView() );
#if REUSE_RESOURCE
			const_cast<grcTexture*>(pTexture)->UpdateGPUCopy();
#endif // REUSE_RESOURCE
		}
		else if( varType == grcEffect::VT_STRUCTUREDBUFFER )
		{
			pShaderResource	= g_ProgramResources[eType][cbOffset].GetStructuredBufferPtr()->GetShaderResourceView();
#if DEVICE_USE_FAST_TEXTURES
			// Just to be sure. Possibly not needed, but better safe than sorry, and it's really not that expensive the amount we're using these types.
			isComplexResource = true;
#endif // DEVICE_USE_FAST_TEXTURES
		}
#if __DEV
		else if ( varType != grcEffect::VT_UAV_TEXTURE && varType != grcEffect::VT_UAV_STRUCTURED && varType != grcEffect::VT_UAV_BYTEADDRESS )
		{
			pShaderResource = (ID3D11ShaderResourceView*)( const_cast<grcTexture*>(grcTexture::None)->GetTextureView() );
			//Errorf("%s Texture %d - Parameter %s missing", astrShaderType[eType], i, m_TexContainer[i]->GetName());
		}
#endif // __DEV

		if ( pShaderResource && s_ResourceViews[cbOffset] != pShaderResource)
		{
#if DEVICE_USE_FAST_TEXTURES
			if (!isComplexResource && eType == PS_TYPE)
			{
				DEVICE_EKG_COUNTANDTIME(PSSetFastShaderResource);
				g_grcCurrentContext->PSSetFastShaderResource(cbOffset, pShaderResource);
			}
			else
#endif // DEVICE_USE_FAST_TEXTURES
			{
				usesComplexResources = true;
				texStartSlot = Min(texStartSlot,cbOffset);
				texEndSlot = Max(texEndSlot,cbOffset);
			}
			s_ResourceViews[cbOffset] = pShaderResource;
		}
	}

	if (usesComplexResources)
	{
		switch (eType)
		{
		case VS_TYPE:
			//g_grcCurrentContext->VSSetShaderResources(texStartSlot, texEndSlot-texStartSlot+1, &s_VSResourceViews[m_TexStartSlot]);
			grcAssertf(0, "grcProgram::SetTextureResourcesUsingVectorDXAPICall()...Vector API call for unit not supported yet.");
			break;
		case PS_TYPE:
			g_grcCurrentContext->PSSetShaderResources(texStartSlot, texEndSlot-texStartSlot+1, &s_ResourceViews[texStartSlot]);
			break;
		case CS_TYPE:
			//g_grcCurrentContext->CSSetShaderResources(texStartSlot, texEndSlot-texStartSlot+1, &s_CSResourceViews[m_TexStartSlot]);
			grcAssertf(0, "grcProgram::SetTextureResourcesUsingVectorDXAPICall()...Vector API call for unit not supported yet.");
			break;
		case DS_TYPE:
			//g_grcCurrentContext->DSSetShaderResources(texStartSlot, texEndSlot-texStartSlot+1, &s_DSResourceViews[m_TexStartSlot]);
			grcAssertf(0, "grcProgram::SetTextureResourcesUsingVectorDXAPICall()...Vector API call for unit not supported yet.");
			break;
		case GS_TYPE:
			//g_grcCurrentContext->GSSetShaderResources(texStartSlot, texEndSlot-texStartSlot+1, &s_GSResourceViews[m_TexStartSlot]);
			grcAssertf(0, "grcProgram::SetTextureResourcesUsingVectorDXAPICall()...Vector API call for unit not supported yet.");
			break;
		case HS_TYPE:
			//g_grcCurrentContext->HSSetShaderResources(texStartSlot, texEndSlot-texStartSlot+1, &s_HSResourceViews[m_TexStartSlot]);
			grcAssertf(0, "grcProgram::SetTextureResourcesUsingVectorDXAPICall()...Vector API call for unit not supported yet.");
			break;
		default:
			Assert(0);
			break;
		}
	}
}

void grcComputeProgram::SetUnorderedResource() const
{
	if (m_numTexContainers==0)
		return;

	const grcDeviceView	*	pUnorderedView		[EFFECT_UAV_BUFFER_COUNT]= { NULL };
	int						pUnorderedCounts	[EFFECT_UAV_BUFFER_COUNT]= { 0 };
	bool					dirty=false;

	for (int i = 0; i < m_numTexContainers; i++)
	{
		if (!m_pTexContainers[i])
		{
			continue;
		}
		const u32 cbOffset = m_pTexContainers[i]->GetCBufOffset();

		grcDeviceView* textureView = NULL;

		switch (m_pTexContainers[i]->GetType())
		{
		case grcEffect::VT_UAV_TEXTURE:
		{
			Assert( cbOffset < EFFECT_UAV_BUFFER_COUNT );

			const grcTextureUAV * pTextureUAV = g_ComputeProgramResources[cbOffset].GetUnorderedTexturePtr(pUnorderedCounts+cbOffset);
			grcRenderTarget * renderTarget = (grcRenderTarget *)pTextureUAV;
			textureView = renderTarget ? renderTarget->GetTextureView() : NULL;
			pUnorderedView[cbOffset] = pTextureUAV ? pTextureUAV->GetUnorderedAccessView() : NULL;
			//pUnorderedView[cbOffset]	= m_TexContainer[i]->GetUnorderedTexturePtr(pUnorderedCounts+cbOffset)->GetUnorderedAccessView();
			dirty = true;
		}
		break;
		case grcEffect::VT_UAV_STRUCTURED:
		//case grcEffect::VT_STRUCTUREDBUFFER:
		{
			Assert( cbOffset < EFFECT_UAV_BUFFER_COUNT );
			const grcBufferUAV * pBufferUAV = g_ComputeProgramResources[cbOffset].GetStructuredBufferPtr(pUnorderedCounts+cbOffset);
			textureView = pBufferUAV ? pBufferUAV->GetShaderResourceView() : NULL;
			pUnorderedView[cbOffset] = pBufferUAV ? pBufferUAV->GetUnorderedAccessView() : NULL;
			//pUnorderedView[cbOffset]	= m_TexContainer[i]->GetStructuredBufferPtr(pUnorderedCounts+cbOffset)->GetUnorderedAccessView();
			dirty = true;
		}
		break;
		case grcEffect::VT_UAV_BYTEADDRESS:
		//case grcEffect::VT_BYTEADDRESSBUFFER:
		{
			Assert( cbOffset < EFFECT_UAV_BUFFER_COUNT );
			const grcBufferUAV * pDataBufferPtr = g_ComputeProgramResources[cbOffset].GetDataBufferPtr();
			textureView = pDataBufferPtr ? pDataBufferPtr->GetShaderResourceView() : NULL;
			pUnorderedView[cbOffset] = pDataBufferPtr ? pDataBufferPtr->GetUnorderedAccessView() : NULL;
			//pUnorderedView[cbOffset]	= m_TexContainer[i]->GetDataBufferPtr()->GetUnorderedAccessView();
			dirty = true;
		}
		break;
		case grcEffect::VT_STRUCTUREDBUFFER:
		case grcEffect::VT_TEXTURE:
			//This is set when binding the shader.
			break;
		default:
			Errorf("Unrecognised type");
			break;
		}

#if DEVICE_RESOLVE_RT_CONFLICTS
		if (textureView)
		{
			GRCDEVICE.NotifyTargetLocked(textureView);
		}
#endif //DEVICE_RESOLVE_RT_CONFLICTS
	}

	if (dirty)
	{
		CompileTimeAssert( sizeof(UINT) == sizeof(pUnorderedCounts[0]) );
		g_grcCurrentContext->CSSetUnorderedAccessViews( 0, EFFECT_UAV_BUFFER_COUNT,
			(ID3D11UnorderedAccessView**)(pUnorderedView), (UINT*)(pUnorderedCounts) );
	}
}

void grcComputeProgram::ResetUnorderedResource()
{
	ID3D11UnorderedAccessView* ppUAViewNULL[EFFECT_UAV_BUFFER_COUNT] = {NULL};
	g_grcCurrentContext->CSSetUnorderedAccessViews( 0, EFFECT_UAV_BUFFER_COUNT, ppUAViewNULL, NULL );
}

//--------------------------------------------------------------------------//
//	continuing with grcDevice methods										//
//--------------------------------------------------------------------------//


void grcDevice::SetVertexShader(grcVertexShader *vs, const grcProgram *pProgram)
{
	sm_CurrentVertexProgram = (grcVertexProgram*)pProgram;
	g_grcCurrentContext->VSSetShader((ID3D11VertexShader*)vs, NULL, 0);

#if RSG_PC
	if (vs != NULL)
		s_ShaderStagesActive |= (1 << VS_TYPE);
	else
		s_ShaderStagesActive &= ~(1 << VS_TYPE);
#endif // RSG_PC
}

void grcDevice::SetComputeShader(grcComputeShader *cs, const grcProgram *pProgram)
{
	sm_CurrentComputeProgram = (grcComputeProgram*)pProgram;
	g_grcCurrentContext->CSSetShader((ID3D11ComputeShader*)cs, NULL, 0);

#if RSG_PC
	if (cs != NULL)
		s_ShaderStagesActive |= (1 << CS_TYPE);
	else
		s_ShaderStagesActive &= ~(1 << CS_TYPE);
#endif // RSG_PC
}
void grcDevice::SetDomainShader(grcDomainShader *ds, const grcProgram *pProgram)
{
	sm_CurrentDomainProgram = (grcDomainProgram*)pProgram;
	g_grcCurrentContext->DSSetShader((ID3D11DomainShader*)ds, NULL, 0);

#if RSG_PC
	if (ds != NULL)
		s_ShaderStagesActive |= (1 << DS_TYPE);
	else
		s_ShaderStagesActive &= ~(1 << DS_TYPE);
#endif // RSG_PC
}

void grcDevice::SetGeometryShader(grcGeometryShader *gs, const grcProgram *pProgram)
{
	sm_CurrentGeometryProgram = (grcGeometryProgram*)pProgram;
	g_grcCurrentContext->GSSetShader((ID3D11GeometryShader*)gs, NULL, 0);

#if RSG_PC
	if (gs != NULL)
		s_ShaderStagesActive |= (1 << GS_TYPE);
	else
		s_ShaderStagesActive &= ~(1 << GS_TYPE);
#endif // RSG_PC
}

void grcDevice::SetHullShader(grcHullShader *hs, const grcProgram *pProgram)
{
	sm_CurrentHullProgram = (grcHullProgram*)pProgram;
	g_grcCurrentContext->HSSetShader((ID3D11HullShader*)hs, NULL, 0);

#if RSG_PC
	if (hs != NULL)
		s_ShaderStagesActive |= (1 << HS_TYPE);
	else
		s_ShaderStagesActive &= ~(1 << HS_TYPE);
#endif // RSG_PC
}

void grcDevice::SetPixelShader(grcPixelShader *ps, const grcProgram *pProgram)
{
	sm_CurrentFragmentProgram = (grcFragmentProgram*)pProgram;
	g_grcCurrentContext->PSSetShader((ID3D11PixelShader*)ps, NULL, 0);

#if RSG_PC
	if (ps != NULL)
		s_ShaderStagesActive |= (1 << PS_TYPE);
	else
		s_ShaderStagesActive &= ~(1 << PS_TYPE);
#endif // RSG_PC
}

void* grcDevice::BeginVertexShaderConstantF(SKINNING_CBUFFER_TYPE address,u32 sizeBytes)
{
	return address->BeginUpdate(sizeBytes);
}

void grcDevice::EndVertexShaderConstantF(SKINNING_CBUFFER_TYPE address)
{
	address->EndUpdate();
}

void grcDevice::SetVertexShaderConstant(int startRegisterartRegister, const float * data, int regCount, grcCBuffer* poConstantBuffer)
{
	Assert(poConstantBuffer != NULL);
//#if !__FINAL // needs fixing
	if (poConstantBuffer == NULL)
		return;
//#endif
	char* pbyConstantBuffer = (char*)poConstantBuffer->GetDataPtr();
	Assert(pbyConstantBuffer != NULL);
	memcpy(&pbyConstantBuffer[startRegisterartRegister * sizeof(Vector4)], data, sizeof(Vector4) * regCount);
}

void grcDevice::SetVertexShaderConstantF(int /*address*/, const float *data, int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*4*count);
}

void grcDevice::SetVertexShaderConstantFW(int /*address*/, const float *data, int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*count);
}

void grcDevice::SetVertexShaderConstantB(int /*address*/,bool value, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	float fValue = (float)value;
	memcpy(pConstData,&fValue,sizeof(float));
}

void grcDevice::SetVertexShaderConstantI(int /*address*/,int value, u32 offset, void *pvDataBuf)
{
	Assert( pvDataBuf != NULL );
	char *const pConstData = (char *)pvDataBuf + offset;
	memcpy( pConstData, &value, sizeof(int) );
}

#if RSG_PC
void grcDevice::SetVertexShaderConstantBufferOverrides(u32 slot, grcCBuffer **ppBuffers, u32 numBuffers)
{
	sm_VSConstantBufferOverrideCount = numBuffers;
	sm_VSConstantBufferOverrideStartSlot = slot;
	sm_VSConstantBufferOverrides = ppBuffers;
}
#endif

void grcDevice::SetPixelShaderConstantF(int /*address*/,const float *data,int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);

	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*4*count);
}

void grcDevice::SetPixelShaderConstantFW(int /*address*/,const float *data,int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);

	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*count);
}

void grcDevice::SetPixelShaderConstantB(int /*address*/,bool value, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,&value,sizeof(bool));
}
void grcDevice::SetComputeShaderConstantF(int /*address*/,float *data,int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);

	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*4*count);
}
void grcDevice::SetComputeShaderConstantB(int /*address*/,bool value, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,&value,sizeof(bool));
}
void grcDevice::SetDomainShaderConstantF(int /*address*/,float *data,int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);

	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*4*count);
}
void grcDevice::SetDomainShaderConstantB(int /*address*/,bool value, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,&value,sizeof(bool));
}
void grcDevice::SetGeometryShaderConstantF(int /*address*/,float *data,int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);

	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*4*count);
}
void grcDevice::SetGeometryShaderConstantB(int /*address*/,bool value, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,&value,sizeof(bool));
}
void grcDevice::SetHullShaderConstantF(int /*address*/,float *data,int count, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);

	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,data,sizeof(float)*4*count);
}
void grcDevice::SetHullShaderConstantB(int /*address*/,bool value, u32 offset, void *pvDataBuf)
{
	Assert(pvDataBuf != NULL);
	char *pConstData = (char *)pvDataBuf;
	pConstData += offset;
	memcpy(pConstData,&value,sizeof(bool));
}

static const char *typesToSemanticNames[] = 
{
	"POSITION",						// grcvetPosition
	"POSITIONT",					// grcvetPositionT
	"NORMAL",						// grcvetNormal
	"BINORMAL",						// grcvetBinormal
	"TANGENT",						// grcvetTangent
	"TEXCOORD",						// grcvetTexture
	"BLENDWEIGHT",					// grcvetBlendWeights
	"BLENDINDICES",					// grcvetBindings
	"COLOR"							// grcvetColor
};

static DXGI_FORMAT dataSizesToFormats[] =
{
	DXGI_FORMAT_R16_FLOAT,			// grcdsHalf,
	DXGI_FORMAT_R16G16_FLOAT,		// grcdsHalf2,
	DXGI_FORMAT_UNKNOWN,			// grcdsHalf3,
	DXGI_FORMAT_R16G16B16A16_FLOAT,	// grcdsHalf4,

	DXGI_FORMAT_R32_FLOAT,			// grcdsFloat,
	DXGI_FORMAT_R32G32_FLOAT,		// grcdsFloat2,
	DXGI_FORMAT_R32G32B32_FLOAT,	// grcdsFloat3,
	DXGI_FORMAT_R32G32B32A32_FLOAT,	// grcdsFloat4,

	DXGI_FORMAT_R8G8B8A8_UINT,		// grcdsUBYTE4,
	DXGI_FORMAT_R8G8B8A8_UNORM,		// grcdsColor,	
	DXGI_FORMAT_R8G8B8A8_SNORM,		// grcdsPackedNormal,
	
	DXGI_FORMAT_R16_UNORM,			// grcdsShort_unorm,
	DXGI_FORMAT_R16G16_UNORM,		// grcdsShort2_unorm,
	DXGI_FORMAT_R8G8_UNORM,			// grcdsByte2_unorm,

	DXGI_FORMAT_R16G16_SINT,		// grcdsShort2
	DXGI_FORMAT_R16G16B16A16_SINT   // grcdsShort4
};
CompileTimeAssert(NELEM(dataSizesToFormats) == grcFvf::grcdsCount);

grcVertexDeclaration* grcDevice::CreateVertexDeclaration(const rage::grcVertexElement *pVertexElements, int elementCount,int /*strideOverride*/)
{
	grcVertexDeclaration *ret = (grcVertexDeclaration*)(rage_new char[sizeof(grcVertexDeclaration) + elementCount * sizeof(D3D11_INPUT_ELEMENT_DESC)]);
	ret->elementCount = elementCount;
	ret->refCount = 1;
	ret->Stream0Size = 0;
	u32 uOffset[MAX_RAGE_VERTEXBUFFER_SLOTS] = { 0 };
	for (int i=0; i<elementCount; i++)
	{
		ret->desc[i].SemanticName = typesToSemanticNames[pVertexElements[i].type];
		ret->desc[i].SemanticIndex = pVertexElements[i].channel;
		ret->desc[i].Format = dataSizesToFormats[pVertexElements[i].format];
		Assert(ret->desc[i].Format != DXGI_FORMAT_UNKNOWN);
		ret->desc[i].InputSlot = pVertexElements[i].stream;
		ret->desc[i].InputSlotClass = (D3D11_INPUT_CLASSIFICATION) (pVertexElements[i].streamFrequencyMode == grcFvf::grcsfmIsInstanceStream ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA);
		ret->desc[i].AlignedByteOffset = uOffset[pVertexElements[i].stream]; //D3D11_APPEND_ALIGNED_ELEMENT;
		ret->desc[i].InstanceDataStepRate = pVertexElements[i].streamFrequencyDivider;

		uOffset[pVertexElements[i].stream] += pVertexElements[i].size;
		if (pVertexElements[i].stream == 0)
			ret->Stream0Size += pVertexElements[i].size;
	}

	return ret;
}

void grcDevice::DestroyVertexDeclaration(grcVertexDeclaration* pDecl)
{
	if (pDecl)
	{
		if (sm_CurrentVertexDeclaration == pDecl)
			sm_CurrentVertexDeclaration = NULL;

		delete (pDecl);
	}
}

static D3D11_PRIMITIVE_TOPOLOGY TranslateDrawMode[] =
{
	D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,		// drawPoints		- Draw one or more single-pixel points
	D3D11_PRIMITIVE_TOPOLOGY_LINELIST,		// drawLines		- Draw one or more disconnected line segments
	D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,		// drawLineStrip	- Draw a single multivertex line strip
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,	// drawTris			- Draw one or more disconnected triangles
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, // drawTriStrip		- Draw a single tristrip
	D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED,		// drawTriFan		- Draw a single triangle fan; NOT SUPPORTED
	D3D11_PRIMITIVE_TOPOLOGY_LINELIST,		// drawQuads		- Independent quads; ONLY SUPPORTED UNDER Xenon and PS3
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,	// drawRects		- Independent rectanges; ONLY SUPPORTED UNDER Xenon
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ,	// drawTrisAdj
};
CompileTimeAssert(NELEM(TranslateDrawMode) == drawModesTotal);

// Map an index count to a primitive count for a given grcDrawMode
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

//--------------------------------------------------------------------------//
// Immediate mode vertex/index buffer functions.							//
//--------------------------------------------------------------------------//

#if !RSG_DURANGO
static void BeginVertices_OnBeginContext()
{
	// Force a D3D11_MAP_WRITE_DISCARD upon 1st usage.
	s_grcVBOffset = s_BeginVerticesBufferSize;
	s_grcIBOffset = s_BeginIndicesBufferSize;
}
#endif

GRC_ALLOC_SCOPE_DECLARE_GLOBAL(static, s_BeginVerticesAllocScope)

void* grcDevice::BeginVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize) 
{
	void *vtxPtr;
	return BeginIndexedVertices(dm,vertexCount,vertexSize,0,&vtxPtr,NULL);
}

void* grcDevice::BeginIndexedVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize, u32 indexCount,void** vertexPtr,void** indexPtr, u32 streamID) 
{
	GRC_ALLOC_SCOPE_PUSH(s_BeginVerticesAllocScope)

	grcStateBlock::Flush();

	ClearStreamSource(streamID);
	//AssertMsg(grcCurrentVertexDeclaration,"BeginVertices - no declarator active! (did you use immediate mode directly?)");
	//Assertf(grcCurrentVertexDeclaration->Stream0Size == vertexSize,"BeginVertices - current declarator is %d bytes, vertex size is %d",grcCurrentVertexDeclaration->Stream0Size,vertexSize);

	const int vbSize = (vertexCount * vertexSize + 1) & ~1;
	const int ibSize = indexCount * sizeof(u16);

	s_grcDrawMode = dm;
	s_grcVertexCount = vertexCount;

#if !RSG_DURANGO
	s_grcIndexCount = indexCount;
#endif

	PF_INCREMENTBY(VBDynamicMaps, (vbSize > 0) ? 1 : 0);
	PF_INCREMENTBY(VBBytesPerFrame, vbSize);

	PF_INCREMENTBY(IBDynamicMaps, (ibSize > 0) ? 1 : 0);
	PF_INCREMENTBY(IBBytesPerFrame, ibSize);

#if RSG_DURANGO

	Assert(vbSize+ibSize <= BEGIN_VERTICES_MAX_SIZE);

	void *alloc = grcGfxContext::current()->allocateTemp(vbSize+ibSize, 16);
	*vertexPtr = alloc;
	StoreBufferEnd(alloc, vertexCount, vertexSize);
	SetStreamSource(streamID, alloc, vertexSize);

	if (indexPtr && indexCount)
	{
		u16 *ib = (u16*)(((uptr)alloc+1)&~1);
		SetIndices(ib);
		*indexPtr = ib;
	}

	return alloc;

#else

#if __ASSERT
	Assertf(vertexCount * vertexSize <= s_BeginVerticesBufferSize, "Device::s_BeginVertexBuffer is too small (%d bytes) for current batch (%d bytes). Either increase one, or shrink the other.", s_BeginVerticesBufferSize, vertexCount * vertexSize );
	if( vertexCount * vertexSize > s_BeginVerticesBufferSize )
	{
		Quitf("Device::s_BeginVertexBuffer is too small (%d bytes) for current batch (%d bytes). Either increase one, or shrink the other.", s_BeginVerticesBufferSize, vertexCount * vertexSize);
	}
	Assertf((indexCount * sizeof(u16)) <= s_BeginIndicesBufferSize, "Device::s_BeginIndicesBuffer is too small (%d bytes) for current batch (%d bytes). Either increase one, or shrink the other.", s_BeginVerticesBufferSize, vertexCount * vertexSize );
	if( (indexCount * sizeof(u16)) > s_BeginVerticesBufferSize )
	{
		Quitf("Device::s_BeginIndicesBuffer is too small (%d bytes) for current batch (%d bytes). Either increase one, or shrink the other.", s_BeginIndicesBufferSize, indexCount * sizeof(u16));
	}
#endif

	u32 vbOffset = 0;
	u32 ibOffset = 0;

	D3D11_MAPPED_SUBRESOURCE lock;
	D3D11_MAP mapFlags = D3D11_MAP_WRITE_NO_OVERWRITE;
	// If vertex size is exact multiple of 16, assume we may be using vectorized operations to fill it.
	if ((vertexSize & 15) == 0)
		s_grcVBOffset = (s_grcVBOffset + 15) & ~15;
	if ((s_grcVBOffset + vbSize) > s_BeginVerticesBufferSize)
	{
#if __DEV
		static u32 uMinDifference = 3;
		static u32 m_frameSinceLastFlip = 0;
		if (m_frameSinceLastFlip + uMinDifference > GetFrameCounter())
		{
			Errorf("Delta between last flip is too small Last %d Current %d Diff %d", s_BeginVerticesBufferSize, m_frameSinceLastFlip, GetFrameCounter(), GetFrameCounter() - m_frameSinceLastFlip);
		}
		m_frameSinceLastFlip = GetFrameCounter();
#endif // __DEV

		s_grcVBOffset = 0;
		mapFlags = D3D11_MAP_WRITE_NO_OVERWRITE; // D3D11_MAP_WRITE_DISCARD;

		PF_INCREMENT(VBOverwritesPerFrame);
	}
	vbOffset = s_grcVBOffset;

	//PF_AUTO_PUSH_TIMEBAR_BUDGETED("s_BeginVertexBuffer", 0.1f); // Timebar overflows. Without discard its not a problem as long as the buffer is big enough to hold N-GPU frames of data
	CHECK_HRESULT( g_grcCurrentContext->Map(s_BeginVertexBuffer,0,mapFlags,0,&lock) );

	s_grcVBLock = (char*)lock.pData + s_grcVBOffset;
	s_grcVBOffset += vbSize;

	*vertexPtr = s_grcVBLock;

	StoreBufferEnd(s_grcVBLock, vertexCount, vertexSize);

	SetStreamSource(streamID, s_BeginVertexBuffer, vbOffset, vertexSize);

	if (s_grcIndexCount && indexPtr != NULL)
	{
		//PF_AUTO_PUSH_TIMEBAR_BUDGETED("s_BeginIndexBuffer", 0.1f); // Timebar overflows. Without discard its not a problem as long as the buffer is big enough to hold N-GPU frames of data
		D3D11_MAP ibMapFlags = D3D11_MAP_WRITE_NO_OVERWRITE;
		ibMapFlags = D3D11_MAP_WRITE_NO_OVERWRITE;
		if ((s_grcIBOffset + ibSize) > s_BeginIndicesBufferSize) {
			s_grcIBOffset = 0;
			ibMapFlags = D3D11_MAP_WRITE_NO_OVERWRITE; // D3D11_MAP_WRITE_DISCARD;
			PF_INCREMENT(IBOverwritesPerFrame);

#if __DEV
			static u32 uMinDifference = 3;
			static u32 m_frameSinceLastFlip = 0;
			if (m_frameSinceLastFlip + uMinDifference > GetFrameCounter())
			{
				Errorf("Delta between last flip is too small Last %d Current %d Diff %d", s_BeginIndicesBufferSize, m_frameSinceLastFlip, GetFrameCounter(), GetFrameCounter() - m_frameSinceLastFlip);
			}
			m_frameSinceLastFlip = GetFrameCounter();
#endif // __DEV
		}
		ibOffset = s_grcIBOffset;

		D3D11_MAPPED_SUBRESOURCE IBLock;
		CHECK_HRESULT(g_grcCurrentContext->Map(s_BeginIndexBuffer,0,ibMapFlags,0,&IBLock));

		s_grcIBLock = (char*)IBLock.pData + s_grcIBOffset;
		s_grcIBOffset += ibSize;
		*indexPtr = s_grcIBLock;
		StoreIBBufferEnd(s_grcIBLock, indexCount, sizeof(u16));

		g_grcCurrentContext->IASetIndexBuffer(s_BeginIndexBuffer, DXGI_FORMAT_R16_UINT, ibOffset);
	}
	else
	{
		s_grcIBLock = NULL;
	}

	return s_grcVBLock;

#endif
}

void* grcDevice::BeginIndexedInstancedVertices(grcDrawMode dm, u32 instanceCount, u32 instanceSize, u32 indexCount, void** instancePtr, void** indexPtr, u32 streamID) 
{
	return BeginIndexedVertices(dm, instanceCount, instanceSize, indexCount, instancePtr, indexPtr, streamID);
}

void grcDevice::EndCreateVertices(const void *bufferEnd)
{
	VerifyBufferEnd(bufferEnd);

#if !RSG_DURANGO
	// TODO - We can buffer up multiple BeginVertices / EndVertices calls by only mapping the vertex buffer once
	// and then issuing multiple Draw calls with a different start vertex.  However, this will require "flushing"
	// the buffer any time we switch to normal rendering again.
	g_grcCurrentContext->Unmap(s_BeginVertexBuffer, 0);
#endif
}

void grcDevice::EndCreateVertices(u32 vertexCount)
{
	Assert(vertexCount < s_grcVertexCount);
	s_grcVertexCount = Min(vertexCount, s_grcVertexCount);

	EndCreateVertices((void*)0);
}

#if __WIN32PC
void grcDevice::EndCreateIndexedVertices(u32 indexCount, u32 UNUSED_PARAM(vertexCount))
{
	VerifyBufferEnd(NULL);
	VerifyIBBufferEnd(NULL);

	Assert(indexCount < s_grcIndexCount);
	s_grcIndexCount = Min(indexCount, s_grcIndexCount);

	g_grcCurrentContext->Unmap(s_BeginVertexBuffer, 0);
	g_grcCurrentContext->Unmap(s_BeginIndexBuffer, 0);
}
#endif // __WIN32PC

#if RAGE_INSTANCED_TECH
void grcDevice::EndCreateInstancedVertices(const void *bufferEnd)
{
	VerifyBufferEnd(bufferEnd);

#if !RSG_DURANGO
	// TODO - We can buffer up multiple BeginVertices / EndVertices calls by only mapping the vertex buffer once
	// and then issuing multiple Draw calls with a different start vertex.  However, this will require "flushing"
	// the buffer any time we switch to normal rendering again.
	g_grcCurrentContext->Unmap(s_BeginVertexBuffer, 0);
#endif
}
#endif

void grcDevice::EndCreateVertices(const void *bufferEnd, u32 UNUSED_PARAM(noOfIndices), const grcIndexBuffer &UNUSED_PARAM(indexBuffer))
{
	VerifyBufferEnd(bufferEnd);

#if !RSG_DURANGO
	g_grcCurrentContext->Unmap(s_BeginVertexBuffer, 0);
#endif
}

void grcDevice::EndCreateIndexedInstancedVertices(const void *bufferEnd, u32 UNUSED_PARAM(vertexSize), u32 UNUSED_PARAM(noOfIndices), const grcVertexBuffer &UNUSED_PARAM(vertexBuffer), const grcIndexBuffer &UNUSED_PARAM(indexBuffer), u32 UNUSED_PARAM(numInstances), u32 UNUSED_PARAM(startIndex), u32 UNUSED_PARAM(startVertex), u32 UNUSED_PARAM(startInstance))
{
	VerifyBufferEnd(bufferEnd);

#if !RSG_DURANGO
	g_grcCurrentContext->Unmap(s_BeginVertexBuffer, 0);
#endif
}

void grcDevice::DrawVertices(const void *UNUSED_PARAM(bufferEnd))
{
	if(s_grcVertexCount == 0)
		return;

	bool triFan = s_grcDrawMode == drawTriFan;
	if (triFan) {
		s_grcDrawMode = drawTris;
#if RSG_DURANGO
		SetIndices(s_BeginVerticesTriFanIndices);
#else
		g_grcCurrentContext->IASetIndexBuffer(s_BeginVerticesTriFanIndices,DXGI_FORMAT_R16_UINT,0);
#endif
	}

	SetUpPriorToDraw(s_grcDrawMode);

	if (triFan) {	
		g_grcCurrentContext->DrawIndexed((s_grcVertexCount-2) * 3,0,0);
#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += (s_grcVertexCount-2) * 3;
			g_pCurrentStatsBucket->TotalPrimitives += (s_grcVertexCount-2);
		}
#endif
	}
	else {
		g_grcCurrentContext->Draw(s_grcVertexCount, 0);
#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += s_grcVertexCount;
			g_pCurrentStatsBucket->TotalPrimitives += s_grcVertexCount;
		}
#endif
	}
#if DEBUG_SEALING_OF_DRAWLISTS
	grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
}

void grcDevice::DrawVertices(u32 UNUSED_PARAM(vertexCount))
{
	DrawVertices((void*)0);
}

#if __WIN32PC
void grcDevice::DrawIndexedVertices(u32 UNUSED_PARAM(indexCount), u32 UNUSED_PARAM(vertexCount))
{
	Assert(s_grcDrawMode != drawTriFan && "Not supported");
	SetUpPriorToDraw(s_grcDrawMode);

#if !__FINAL
	if (s_grcDrawMode == drawTriFan) {
	}
	else 
#endif // !__FINAL
	{
		g_grcCurrentContext->IASetIndexBuffer(s_BeginIndexBuffer, DXGI_FORMAT_R16_UINT, s_grcIBOffset);
		g_grcCurrentContext->DrawIndexed(s_grcIndexCount,0,0);
#if DEBUG_SEALING_OF_DRAWLISTS
		grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += s_grcIndexCount;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(s_grcIndexCount, s_grcDrawMode);
		}
#endif
	}
}
#endif // __WIN32PC

#if RAGE_INSTANCED_TECH
void grcDevice::DrawInstancedVertices(const void *UNUSED_PARAM(bufferEnd))
{
	bool triFan = s_grcDrawMode == drawTriFan;
	if (triFan) {
		s_grcDrawMode = drawTris;
#if RSG_DURANGO
		SetIndices(s_BeginVerticesTriFanIndices);
#else
		g_grcCurrentContext->IASetIndexBuffer(s_BeginVerticesTriFanIndices,DXGI_FORMAT_R16_UINT,0);
#endif
	}

	SetUpPriorToDraw(s_grcDrawMode);

	if (triFan) {
		g_grcCurrentContext->DrawIndexedInstanced((s_grcVertexCount-2) * 3,grcViewport::GetNumInstVP(),0,0,0);
	#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			u32 numIndices = (s_grcVertexCount-2) * 3 * grcViewport::GetNumInstVP();
			g_pCurrentStatsBucket->TotalIndices += numIndices;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(numIndices, s_grcDrawMode);
		}
	#endif
	}
	else {
		g_grcCurrentContext->DrawInstanced(s_grcVertexCount, grcViewport::GetNumInstVP(),0,0);
	#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			u32 numIndices = s_grcVertexCount * grcViewport::GetNumInstVP();
			g_pCurrentStatsBucket->TotalIndices += numIndices;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(numIndices, s_grcDrawMode);
		}
	#endif
	}

#if DEBUG_SEALING_OF_DRAWLISTS
	grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
}
#endif

void grcDevice::DrawVertices(const void *UNUSED_PARAM(bufferEnd), u32 noOfIndices, const grcIndexBuffer &indexBuffer)
{
	Assert(s_grcDrawMode != drawTriFan);

	SetIndices(indexBuffer);

	SetUpPriorToDraw(s_grcDrawMode);

	if(noOfIndices)
	{
		g_grcCurrentContext->DrawIndexed(noOfIndices, 0, 0);
	#if DEBUG_SEALING_OF_DRAWLISTS
		grcTextureFactory::GetInstance().OnDraw();
	#endif // DEBUG_SEALING_OF_DRAWLISTS
	#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += noOfIndices;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(noOfIndices, s_grcDrawMode);
		}
	#endif
	}
}

void grcDevice::DrawIndexedInstancedVertices(const void *UNUSED_PARAM(bufferEnd), u32 vertexSize, u32 noOfIndices, const grcVertexBuffer &vertexBuffer, const grcIndexBuffer &indexBuffer, u32 numInstances, u32 startIndex, u32 startVertex, u32 startInstance)
{
	Assert(s_grcDrawMode != drawTriFan);

#if RSG_DURANGO
	SetStreamSource(0, (char*)vertexBuffer.GetUnsafeReadPtr(), vertexSize);
#else
	ID3D11Buffer* pVertBuffer = (ID3D11Buffer*)vertexBuffer.GetD3DBuffer();
	SetStreamSource(0, pVertBuffer, 0, vertexSize);
#endif

	SetIndices(indexBuffer);

	SetUpPriorToDraw(s_grcDrawMode);

	if(noOfIndices)
	{
		g_grcCurrentContext->DrawIndexedInstanced(noOfIndices, numInstances, startIndex, startVertex, startInstance);
	#if DEBUG_SEALING_OF_DRAWLISTS
		grcTextureFactory::GetInstance().OnDraw();
	#endif // DEBUG_SEALING_OF_DRAWLISTS
	#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += noOfIndices*numInstances;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(noOfIndices*numInstances, s_grcDrawMode);
		}
	#endif
	}
}

void grcDevice::ReleaseVertices(const void *UNUSED_PARAM(bufferEnd))
{
	GRC_ALLOC_SCOPE_POP(s_BeginVerticesAllocScope)
}

void grcDevice::ReleaseVertices(u32 UNUSED_PARAM(vertexCount))
{
	ReleaseVertices((void*)0);
}

void grcDevice::ReleaseIndexedVertices(u32 UNUSED_PARAM(indexCount), u32 UNUSED_PARAM(vertexCount))
{
	ReleaseVertices((void*)0);
}

#if RAGE_INSTANCED_TECH
void grcDevice::ReleaseInstancedVertices(const void *UNUSED_PARAM(bufferEnd))
{
	ReleaseVertices((void*)0);
}
#endif

void grcDevice::ReleaseVertices(const void *UNUSED_PARAM(bufferEnd), u32 UNUSED_PARAM(noOfIndices), const grcIndexBuffer &UNUSED_PARAM(indexBuffer))
{
	ReleaseVertices((void*)0);
}

void grcDevice::ReleaseIndexedInstancedVertices(const void *UNUSED_PARAM(bufferEnd), u32 UNUSED_PARAM(vertexSize), u32 UNUSED_PARAM(noOfIndices), const grcVertexBuffer &UNUSED_PARAM(vertexBuffer), const grcIndexBuffer &UNUSED_PARAM(indexBuffer), u32 UNUSED_PARAM(numInstances), u32 UNUSED_PARAM(startIndex), u32 UNUSED_PARAM(startVertex), u32 UNUSED_PARAM(startInstance))
{
	ReleaseVertices((void*)0);
}

/*--------------------------------------------------------------------------------------*/

#if __WIN32PC

bool grcDevice::ChangeDevice(grcDisplayWindow oNewSettings)
{
	if ((PARAM_rag.Get() || PARAM_setHwndMain.Get()) && !PARAM_ragUseOwnWindow.Get())
	{
		return false;
	}

	if (!IsMessagePumpThreadThatCreatedTheD3DDevice())
	{
		sm_DesiredWindow = oNewSettings;
		SetChangeDeviceRequest(true);
		SetInPopup(false);
		return false;
	}

	if (!IsCreated() || IsReleasing())
	{
		return false;
	}

	pcdDisplayf("ChangeDevice: Change to %dx%d, fullscreen %d", oNewSettings.uWidth, oNewSettings.uHeight, oNewSettings.bFullscreen);
	sm_DesiredWindow = oNewSettings;
	SetMatchDesiredWindow(true);
	SetChangeDeviceRequest(false);

	s_SwapChainLock.Lock();
	IDXGISwapChain* pSwapChain = (IDXGISwapChain*) GetSwapChain();

	const grcAdapterD3D11* pAdapter = (const grcAdapterD3D11*) grcAdapterManager::GetInstance()->GetAdapter((u32)sm_AdapterOrdinal);
	const grcAdapterD3D11Output* pOutput = pAdapter->GetOutput((u32)sm_OutputMonitor);
	const DXGI_MODE_DESC& oModeDesc = pOutput->GetClosestMode(oNewSettings);

	if (oNewSettings.bFullscreen)
	{
		pSwapChain->SetFullscreenState(oNewSettings.bFullscreen, oNewSettings.bFullscreen ? pOutput->GetDeviceOutput() : NULL);
		pSwapChain->ResizeTarget(&oModeDesc);
	}
	else
	{
		pSwapChain->SetFullscreenState(oNewSettings.bFullscreen, oNewSettings.bFullscreen ? pOutput->GetDeviceOutput() : NULL);

		DXGI_OUTPUT_DESC desc;
		unsigned int dpiX, dpiY;
		grcAdapterD3D11Output::GetDesc(pAdapter->GetHighPart(), pAdapter->GetLowPart(), pOutput, desc , dpiX, dpiY);

		RECT rect = {0,0,oNewSettings.uWidth,oNewSettings.uHeight};

		u32 windowStyle = GetWindowLong( g_hwndMain, GWL_STYLE );
		ComputeWindowDimensions(rect, windowStyle);

		LONG width = rect.right - rect.left;
		LONG height = rect.bottom - rect.top;

		pcdDisplayf("Pre-GetWindowRect Set Window Position: LT: %dx%d RB: %dx%d in ChangeDevice", rect.left, rect.top, rect.right, rect.bottom);

		RECT windowRect;
		GetWindowRect(g_hwndMain, &windowRect);

		UINT flags = SWP_NOZORDER | SWP_FRAMECHANGED;
		if (!GRCDEVICE.IsRecenterWindow())
		{
			flags |= SWP_NOMOVE;
		}

		pcdDisplayf("Set Window Position: %dx%d Dimensions %dx%d in ChangeDevice", rect.left, rect.top, width, height);
		SetWindowPos( g_hwndMain, HWND_TOP, rect.left, rect.top, width, height, flags);
	}
	s_SwapChainLock.Unlock();

	if (IsForceChangeDevice()) 
		SetRecheckDeviceChanges(true);

#if USE_NV_SHADOW_LIB
	//TODO: ???
#endif

#if USE_AMD_SHADOW_LIB
	//TODO: ???
#endif

	return true;
}

void grcDevice::ForceDeviceReset()
{
	if (!IsMessagePumpThreadThatCreatedTheD3DDevice())
	{
		SetForceDeviceReset(true);
		return;
	}

	SetInsideDeviceChange(true);

	SetForceDeviceReset(false);

	LockDeviceResetAvailability();

	LockContext();

	//	grcDisplayf ("ResizeBuffers::PrepareForShutdown");
	PrepareForShutdown();

	DeviceLost();

	RESOURCE_CACHE_ONLY(grcResourceCache::DeviceLost();)

	UpdatePresentParameters();

	RESOURCE_CACHE_ONLY(grcResourceCache::DeviceReset();)

	DeviceRestored();

	SetDeviceRestored(true);
	//grcDisplayf ("ResizeBuffers::DeviceRestored->true");
	SetInsideDeviceChange(false);

	UnlockContext();

	UnlockDeviceResetAvailability();
}

#if RSG_PC && __D3D11

void grcDevice::CheckVideoEncodingOverride()
{
	static bool windowStyleOverride = false;
	if (GRCDEVICE.GetVideoEncodingOverride() && !windowStyleOverride)
	{
		windowStyleOverride = true;
		LONG_PTR result = GetWindowLongPtr( g_hwndMain, GWL_STYLE );
		result &= ~WS_MINIMIZEBOX;
		result = SetWindowLongPtr( g_hwndMain, GWL_STYLE, result);
	}
	else if (!GRCDEVICE.GetVideoEncodingOverride() && windowStyleOverride)
	{
		windowStyleOverride = false;
		LONG_PTR result = GetWindowLongPtr( g_hwndMain, GWL_STYLE );
		result |= (GRCDEVICE.GetWindowFlags() & WS_MINIMIZEBOX);
		result = SetWindowLongPtr( g_hwndMain, GWL_STYLE, result);
	}
}
#endif

#if RSG_PC
bool grcDevice::m_bDisableAltEnterChange = false;

void grcDevice::SetDisableAltEnterChange(bool bOverride)
{
	m_bDisableAltEnterChange = bOverride;
}

bool grcDevice::IsDisableAltEnterChange()
{
	return m_bDisableAltEnterChange;
}

void grcDevice::SuppressAltEnter()
{
	if (m_bDisableAltEnterChange || GetCurrent() == NULL)
		return;

	HRESULT hr;
	IDXGIDevice* pIDXGIDevice;
	hr = GetCurrent()->QueryInterface(__uuidof(IDXGIDevice), (void **)&pIDXGIDevice);
	if (hr != D3D_OK)
	{
		return;
	}

	IDXGIAdapter* pIDXGIAdapter;
	hr = pIDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pIDXGIAdapter);
	if (hr != D3D_OK)
	{
		pIDXGIDevice->Release(); pIDXGIDevice = NULL;
		return;
	}

	IDXGIFactory* pIDXGIFactory;
	hr = pIDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);
	if (hr != D3D_OK)
	{
		pIDXGIAdapter->Release(); pIDXGIAdapter = NULL;
		pIDXGIDevice->Release(); pIDXGIDevice = NULL;
		return;
	}

	hr = pIDXGIFactory->MakeWindowAssociation(g_hwndMain, DXGI_MWA_NO_WINDOW_CHANGES);

	pIDXGIFactory->Release(); pIDXGIFactory = NULL;
	pIDXGIAdapter->Release(); pIDXGIAdapter = NULL;
	pIDXGIDevice->Release(); pIDXGIDevice = NULL;
}

void grcDevice::AllowAltEnter()
{
#if DIRECT_CONTROL_OF_ALT_ENTER
	return;
#else
	if (m_bDisableAltEnterChange || GetCurrent() == NULL)
		return;

	HRESULT hr;
	IDXGIDevice* pIDXGIDevice;
	hr = GetCurrent()->QueryInterface(__uuidof(IDXGIDevice), (void **)&pIDXGIDevice);
	if (hr != D3D_OK)
	{
		return;
	}

	IDXGIAdapter* pIDXGIAdapter;
	hr = pIDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pIDXGIAdapter);
	if (hr != D3D_OK)
	{
		pIDXGIDevice->Release(); pIDXGIDevice = NULL;
		return;
	}

	IDXGIFactory* pIDXGIFactory;
	hr = pIDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);
	if (hr != D3D_OK)
	{
		pIDXGIAdapter->Release(); pIDXGIAdapter = NULL;
		pIDXGIDevice->Release(); pIDXGIDevice = NULL;
		return;
	}

	hr = pIDXGIFactory->MakeWindowAssociation(g_hwndMain, 0);

	pIDXGIFactory->Release(); pIDXGIFactory = NULL;
	pIDXGIAdapter->Release(); pIDXGIAdapter = NULL;
	pIDXGIDevice->Release(); pIDXGIDevice = NULL;
#endif
}

u32 grcDevice::GetDXFeatureLevelSupported()
{
	if (GetCurrent() == NULL)
		return 0;

	HRESULT hr;
	IDXGIDevice* pIDXGIDevice;
	hr = GetCurrent()->QueryInterface(__uuidof(IDXGIDevice), (void **)&pIDXGIDevice);
	if (hr != D3D_OK)
	{
		return 0;
	}

	IDXGIAdapter* pIDXGIAdapter;
	hr = pIDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pIDXGIAdapter);
	if (hr != D3D_OK)
	{
		pIDXGIDevice->Release(); pIDXGIDevice = NULL;
		return 0;
	}

	D3D_FEATURE_LEVEL featureLevels[] = { 
#if __D3D11_1
		D3D_FEATURE_LEVEL_11_1, 
#endif // __D3D11_1		
		D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

	u32 uNumFeatureLevels = NELEM(featureLevels);
	D3D_FEATURE_LEVEL chosenFeatureLevel;

	hr = g_D3D11CreateDevice(pIDXGIAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, featureLevels, uNumFeatureLevels, D3D11_SDK_VERSION, NULL, &chosenFeatureLevel, NULL);

	pIDXGIAdapter->Release(); pIDXGIAdapter = NULL;
	pIDXGIDevice->Release(); pIDXGIDevice = NULL;

	switch (chosenFeatureLevel)
	{
#if __D3D11_1
	case D3D_FEATURE_LEVEL_11_1:
		return 1110;
		break;
#endif // __D3D11_1
	case D3D_FEATURE_LEVEL_11_0:
		return 1100;
		break;
	case D3D_FEATURE_LEVEL_10_1:
		return 1010;
		break;
	case D3D_FEATURE_LEVEL_10_0:
		return 1000;
		break;
	default:
		Assertf(false, "Unknown feature set level %d", chosenFeatureLevel);
		Quitf(ERR_GFX_D3D_NOFEATURELEVEL_2,"Unknown feature set level");
		break;
	}

	return 0;
}
#endif

bool grcDevice::CanLoadShader(u32 shaderMajor, u32 shaderMinor)
{
	AssertMsg(((shaderMajor==4)&&(shaderMinor==0)) 
		   || ((shaderMajor==4)&&(shaderMinor==1))
		   || ((shaderMajor==5)&&(shaderMinor==0)), "Unrecognised shader type");

	// Get the compile target
 	u32 deviceMajor, deviceMinor;
 	GetDxShaderModel(deviceMajor, deviceMinor);

	bool canLoadShader = (shaderMajor < deviceMajor) || (shaderMajor==deviceMajor && shaderMinor<=deviceMinor);
	return canLoadShader;
}


bool grcDevice::IsReadOnlyDepthAllowed()
{
	return s_UseReadOnlyDepth;
}

bool grcDevice::Reset()
{
	UpdatePresentParameters();
	return true;
}

grcDeviceHandle* grcDevice::GetCurrentInner()
{
	return static_cast<RageDirect3DDevice11*>(sm_Current)->m_Inner;
}

void grcDevice::UpdatePresentParameters()
{
	g_grcDepthFormat = grctfD32FS8;

	// Straight outta the tutorial code...
	ZeroMemory( &g_SwapDesc, sizeof(g_SwapDesc) );
	g_SwapDesc.BufferCount = 1;
	g_SwapDesc.Flags = (sm_CurrentWindows[g_RenderThreadIndex].bFullscreen) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0;
	g_SwapDesc.BufferDesc.Width = sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	g_SwapDesc.BufferDesc.Height = sm_CurrentWindows[g_RenderThreadIndex].uHeight;
	g_SwapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // PC TODO - Should expose something to tweak this
	const RefreshRate rate = (sm_CurrentWindows[g_RenderThreadIndex].bFullscreen ? sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate : RefreshRate(0));
	g_SwapDesc.BufferDesc.RefreshRate.Numerator = rate.Numerator;
	g_SwapDesc.BufferDesc.RefreshRate.Denominator = rate.Denominator;
	Assert( rate.Denominator != 0 );
	g_SwapDesc.BufferUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER; // PC TODO - Can't alway use the back buffer as a texture
	g_SwapDesc.OutputWindow = g_hwndMain;
	g_SwapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	u32 multisample_count;
	u32 multisample_quality = 0;
	GetMultiSample(multisample_count, multisample_quality);
	multisample_count = (multisample_count) ? multisample_count : 1;

	g_SwapDesc.SampleDesc.Count = 1;//multisample_count;
	g_SwapDesc.SampleDesc.Quality = 0;//multisample_quality;
	g_SwapDesc.Windowed = !sm_CurrentWindows[g_RenderThreadIndex].bFullscreen;
}
#endif

#if RSG_PC
bool grcDevice::AllocateSwapChain()
{
	HRESULT hr;
	IDXGIDevice * pDXGIDevice;
	hr = g_grcCurrentContext->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
	Assertf(hr == S_OK, "Failed to query IDXGIDevice interface %x", hr);
	      
	IDXGIAdapter * pDXGIAdapter;
	hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);
	Assertf(hr == S_OK, "Failed to query IDXGIAdapter interface %x", hr);

	IDXGIFactory * pIDXGIFactory;
	hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);
	Assertf(hr == S_OK, "Failed to query IDXGIFactory interface %x", hr);

	BOOL bWindowed = g_SwapDesc.Windowed;
	g_SwapDesc.Windowed = true;
	s_SwapChainLock.Lock();
	hr = pIDXGIFactory->CreateSwapChain(g_grcCurrentContext, &g_SwapDesc, (IDXGISwapChain**)&sm_pSwapChain);
	s_BackupSwapChain = sm_pSwapChain;
	s_SwapChainLock.Unlock();
	if (hr != S_OK)
	{
		Errorf("Failed to Create Swap Chain %x", hr);
		//Quitf("Failed to Create Swap Chain");
		return false;
	}
	if (!bWindowed)
	{
		// Note: This method doesn't appear to be called at the moment.
		// Adding multi-monitor support in case this changes.
		IDXGIOutput * pDXGIOutput = NULL;
		IDXGIOutput * pOutput = NULL;
		int i;
		int target = 0;
		PARAM_adapter.Get(target);
		for (i = 0; pDXGIAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND; i++)
		{ 
			if (i == target)
			{
				pDXGIOutput = pOutput;
				Printf("Found desired output.\r\n");
			}
			else
				pOutput->Release();
		} 

		Printf("%d outputs.\r\n", i);

		s_SwapChainLock.Lock();
		hr = ((IDXGISwapChain*)GetSwapChain())->SetFullscreenState(TRUE, pDXGIOutput);
		s_SwapChainLock.Unlock();
		if (pDXGIOutput)
		{
			pDXGIOutput->Release();
			pDXGIOutput = NULL;
		}
		if (hr != S_OK)
		{
			Errorf("Failed to set device into fullscreen mode %x", hr);
			return false;
		}
	}
	Displayf("Successfully Created Swap Chain in %s", bWindowed ? "Windowed" : "Fullscreen");
	return true;
}
#endif


bool grcDevice::FreeSwapChain()
{
	s_SwapChainLock.Lock();
#if __WIN32PC
	if (!IsWindowed())
	{
		HRESULT hr = ((IDXGISwapChain*)GetSwapChain())->SetFullscreenState( FALSE, 0 );
		if (hr != S_OK)
		{
			Warningf("Changing Swap Chain State to Fullscreen %x - Postponing release", hr);
			s_SwapChainLock.Unlock();
			return false;
		}
	}
#endif
	if (GetSwapChain())
	{
		u32 uRefCnt = GetSwapChain()->Release();
		if (uRefCnt > 1)
		{
			Warningf("Swap Chain still has %u references", uRefCnt);
		}
		s_BackupSwapChain = sm_pSwapChain = NULL;
	}
	s_SwapChainLock.Unlock();
	return true;
}

#if RSG_PC
grcSwapChain* grcDevice::GetSwapChain()
{
#if CHECK_SWAP_CHAIN
	if (sm_pSwapChain != s_BackupSwapChain)
	{
		Quitf(rage::ERR_MEM_SPARSEALLOC_ALLOC, "Sparse Memory Allocator Failed - Unable to recover");
	}
#endif // CHECK_SWAP_CHAIN
	return sm_pSwapChain;
}
#endif // RSG_PC

#if RSG_DURANGO
bool grcDevice::CheckDeviceStatus()
{
	if (IsCurrentThreadTheDeviceOwnerThread())
	{
		if (!IsCreated())
		{
			return false;
		}
	}
	return true;
}

#endif // RSG_DURANGO

#if __WIN32PC

bool grcDevice::ProcessResizeBuffersWhileEncoding()
{
	BOOL bFullScreen = FALSE;
	IDXGIOutput *pOutputMonitor = NULL;
	s_SwapChainLock.Lock();
	IDXGISwapChain* pSwapChain = (IDXGISwapChain*)GetSwapChain();
	ASSERT_ONLY(HRESULT result =) pSwapChain->GetFullscreenState(&bFullScreen, &pOutputMonitor);
	Assertf(result == S_OK, "Unable to get the fullscreen state. Error 0x%x", result);
	s_SwapChainLock.Unlock();
	bool currentlyFullscreen = bFullScreen != 0;

	if (currentlyFullscreen != IsWindowed())
	{
		return true;
	}

	grcDisplayWindow oNewSettings = sm_GlobalWindow;
	if (currentlyFullscreen)
	{
		
		pcdDisplayf("ProcessResizeBuffersWhileEncoding: Request fullscreen res of %dx%d", oNewSettings.uWidth, oNewSettings.uHeight);
		pcdDisplayf("ProcessResizeBuffersWhileEncoding: Adapter %d", sm_AdapterOrdinal);
		const grcAdapterD3D11* adapter = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(sm_AdapterOrdinal);
		adapter->GetClosestMode(&oNewSettings, pOutputMonitor);
	}

	s_SwapChainLock.Lock();

 	DXGI_SWAP_CHAIN_DESC oDesc;
	pSwapChain->GetDesc(&oDesc);

	oNewSettings.uWidth = oDesc.BufferDesc.Width;
	oNewSettings.uHeight = oDesc.BufferDesc.Height;
	oNewSettings.bFullscreen = currentlyFullscreen;

	pcdDisplayf("ProcessResizeBuffersWhileEncoding: SwapChain description BEFORE ResizeBuffers is Width: %d, Height: %d, RefreshRate %d//%d", oDesc.BufferDesc.Width, oDesc.BufferDesc.Height, oDesc.BufferDesc.RefreshRate.Numerator, oDesc.BufferDesc.RefreshRate.Denominator);

	RECT rcCurrentClient;
	GetClientRect( g_hwndMain, &rcCurrentClient );
	pcdDisplayf("ProcessResizeBuffersWhileEncoding: Window BEFORE ResizeBuffers is left: %d, right: %d, top: %d, bottom: %d", rcCurrentClient.left, rcCurrentClient.right, rcCurrentClient.top, rcCurrentClient.bottom);

	LockDeviceResetAvailability();

	LockContext();
	
	RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().UnloadCache());

	if (grcTextureFactoryPC::HasInstance())
		((grcTextureFactoryDX11 *)(&grcTextureFactoryDX11::GetInstance()))->Lost();

	RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().UnloadCache());


    // ResizeBuffers
	HRESULT hr = pSwapChain->ResizeBuffers( oDesc.BufferCount,    //0,
											  oDesc.BufferDesc.Width,  //0,
											  oDesc.BufferDesc.Height,  //0,
											  oDesc.BufferDesc.Format,  //DXGI_FORMAT_UNKNOWN
											  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH );
	Assertf(hr == S_OK, "ProcessResizeBuffersWhileEncoding: Resize Buffers failed %x", hr);
	s_SwapChainLock.Unlock();
	if (hr != S_OK)
	{
		UnlockContext();
		pcdDisplayf("ProcessResizeBuffersWhileEncoding: RESIZEBUFFERS FAILED");
		return false;
	}



	ASSERT_ONLY(RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().SetSafeResourceCreate(true);))
		if (grcTextureFactoryPC::HasInstance())
		{
			grcStateBlock::Default();

			((grcTextureFactoryDX11 *)(&grcTextureFactoryDX11::GetInstance()))->Reset();
		}


	ASSERT_ONLY(RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().SetSafeResourceCreate(false);))




	g_inWindow = !currentlyFullscreen;

	pcdDisplayf("ProcessResizeBuffersWhileEncoding: G_InWindow now %d", g_inWindow);

	pcdDisplayf("ProcessResizeBuffersWhileEncoding: RESIZEBUFFERS SUCCEEDED");

	sm_CurrentWindows[g_RenderThreadIndex] = oNewSettings;
	sm_GlobalWindow = oNewSettings;

	UnlockContext();

	UnlockDeviceResetAvailability();

	pSwapChain->GetDesc(&oDesc);
	pcdDisplayf("ProcessResizeBuffersWhileEncoding: SwapChain description AFTER ResizeBuffers is Width: %d, Height: %d, RefreshRate %d//%d", oDesc.BufferDesc.Width, oDesc.BufferDesc.Height, oDesc.BufferDesc.RefreshRate.Numerator, oDesc.BufferDesc.RefreshRate.Denominator);

	GetClientRect( g_hwndMain, &rcCurrentClient );
	pcdDisplayf("ProcessResizeBuffersWhileEncoding: Window AFTER ResizeBuffers is left: %d, right: %d, top: %d, bottom: %d", rcCurrentClient.left, rcCurrentClient.right, rcCurrentClient.top, rcCurrentClient.bottom);

	return true;
}

bool grcDevice::CheckForDeviceChanges()
{
#if HANG_DETECT_THREAD
	static bool bInSafeZone = false;

	// We enter a safe zone when we check for device changes. When everything is fine we exit the safe zone (the end of this function).
	// This is to stop the hang detect thread kicking in when we suspend, when this happens the device is not ready causing other threads
	// to lockup.
	if(bInSafeZone == false)
	{
		HANG_DETECT_SAVEZONE_ENTER();
		bInSafeZone = true;
	}
#endif // HANG_DETECT_THREAD

	if (IsBusyAltTabbing() || IgnoreDeviceReset())
	{
		SetRecheckDeviceChanges(true);
		return false;
	}

	if (!IsCreated() || (GetSwapChain() == NULL) || IsReleasing())
	{
		pcdDisplayf("CheckForDeviceChanges: First check on Device readiness failed, IsCreated: %d, GetSwapChain: %p, IsReleasing %d", IsCreated(), GetSwapChain(), IsReleasing());
		return false;
	}

	if (IsInsideDeviceChange())
	{
		pcdDisplayf("Currently inside a device change, aborting for now");
		SetRecheckDeviceChanges(true);
		return false;
	}

	RECT rcCurrentClient;
	GetClientRect( g_hwndMain, &rcCurrentClient );

	if ((rcCurrentClient.right <= 0) || (rcCurrentClient.bottom <= 0))
	{
		pcdDisplayf("CheckForDeviceChanges: Client rectangle is invalid, aborting.  Right: %d, left: %d", rcCurrentClient.right, rcCurrentClient.bottom);
		return false;
	}

	if (GetVideoEncodingOverride())
	{
		return ProcessResizeBuffersWhileEncoding();
	}

	pcdDisplayf("Window at beginning of CheckForDeviceChanges is left: %d, right: %d, top: %d, bottom: %d", rcCurrentClient.left, rcCurrentClient.right, rcCurrentClient.top, rcCurrentClient.bottom);

	SetInsideDeviceChange(true);

	BOOL bFullScreen = FALSE;
	IDXGIOutput *pOutputMonitor = NULL;
	s_SwapChainLock.Lock();
	IDXGISwapChain* pSwapChain = (IDXGISwapChain*)GetSwapChain();
	ASSERT_ONLY(HRESULT result =) pSwapChain->GetFullscreenState(&bFullScreen, &pOutputMonitor);
	Assertf(result == S_OK, "Unable to get the fullscreen state. Error 0x%x", result);
	s_SwapChainLock.Unlock();
	bool currentlyFullscreen = bFullScreen != 0;

	if (currentlyFullscreen && ((rcCurrentClient.bottom - rcCurrentClient.top) > (rcCurrentClient.right - rcCurrentClient.left)))
	{
		pSwapChain->SetFullscreenState(false, NULL);
		SetRecheckDeviceChanges(false);
		SetInsideDeviceChange(false);
		return false;
	}

	bool switchedFullscreen = IsWindowed() == currentlyFullscreen;
	pcdDisplayf("CheckForDeviceChanges: output monitor is %p, fullscreen is %d, G_InWindow is %d", pOutputMonitor, currentlyFullscreen, IsWindowed());

	grcDisplayWindow oNewSettings = sm_GlobalWindow;
	oNewSettings.bFullscreen = currentlyFullscreen;

	if (IsMatchDesiredWindow())
	{
		oNewSettings = sm_DesiredWindow;
	}

	if (IsBorderless() || IsRecenterWindow())
	{
		SetRecenterWindow(false);

		u32 windowStyle = GetWindowLong( g_hwndMain, GWL_STYLE );

		RECT rect = {0,0,oNewSettings.uWidth,oNewSettings.uHeight};

		ComputeWindowDimensions(rect, windowStyle);

		LONG width = rect.right - rect.left;
		LONG height = rect.bottom - rect.top;

		if (height < 600) height = 600;
		if (width < 800) width = 800;

		pcdDisplayf("Pre-GetWindowRect Set Window Position: LT: %dx%d RB: %dx%d in ChangeDevice", rect.left, rect.top, rect.right, rect.bottom);
		RECT windowRect;
		GetWindowRect(g_hwndMain, &windowRect);

		u32 flags = SWP_NOZORDER | SWP_FRAMECHANGED;

		if (windowRect != rect)
		{
			pcdDisplayf("Set Window Position: %dx%d Dimensions %dx%d in center window", rect.left, rect.top, width, height);
			SetWindowPos( g_hwndMain, HWND_TOP, rect.left, rect.top, width, height, flags);
			SetRecheckDeviceChanges(true);
			SetInsideDeviceChange(false);
			return false;
		}
	}

	SetRecheckDeviceChanges(false);

	bool resizeBuffers = false;

	if (currentlyFullscreen)
	{
		pcdDisplayf("CheckForDeviceChanges: Request fullscreen res of %dx%d", oNewSettings.uWidth, oNewSettings.uHeight);
		pcdDisplayf("CheckForDeviceChanges: Adapter %d", sm_AdapterOrdinal);
		const grcAdapterD3D11* adapter = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(sm_AdapterOrdinal);
		adapter->GetClosestMode(&oNewSettings, pOutputMonitor);

		pcdDisplayf("CheckForDeviceChanges: Change to fullscreen after GetClodestMode to res of %dx%d", oNewSettings.uWidth, oNewSettings.uHeight);

		if (sm_GlobalWindow != oNewSettings) resizeBuffers = true;
		pcdDisplayf("ResizeBuffers currently set to %d", resizeBuffers);
	}
	else
	{
		if (( s32 )rcCurrentClient.right != (LONG) sm_GlobalWindow.uWidth) resizeBuffers = true;
		if (( s32 )rcCurrentClient.bottom != (LONG) sm_GlobalWindow.uHeight) resizeBuffers = true;
		oNewSettings.uWidth = rcCurrentClient.right;
		oNewSettings.uHeight = rcCurrentClient.bottom;
		pcdDisplayf("CheckForDeviceChanges: Request windowed res of %dx%d from %dx%d resizeBuffers %d", oNewSettings.uWidth, oNewSettings.uHeight, sm_GlobalWindow.uWidth, sm_GlobalWindow.uHeight, resizeBuffers);
	}

	if (pOutputMonitor) pOutputMonitor->Release();

	if (IsForceChangeDevice()) 
	{
		resizeBuffers = true;
		pcdDisplayf("Forcing a device change resizeBuffers now true");
	}
	SetForceChangeDevice(false);

	if (resizeBuffers)
	{
		pcdDisplayf("CheckForDeviceChanges: Call ResizeBuffers width: %d height: %d fullscreen: %d", oNewSettings.uWidth, oNewSettings.uHeight, oNewSettings.bFullscreen);
		if (!ResizeBuffers(oNewSettings))
		{
			pcdDisplayf("CheckForDeviceChanges: Resize buffers has failed for some reason");

			SetInsideDeviceChange(false);
			return false;
		}
	}
	else if (IsRequireDeviceRestoreCallbacks() || switchedFullscreen)
	{
		pcdDisplayf("Running ForceDeviceReset");
		ForceDeviceReset();
		sm_CurrentWindows[g_RenderThreadIndex] = oNewSettings;
	}

	SetRequireDeviceRestoreCallbacks(false);

	if (IsMatchDesiredWindow() 
		&& ((oNewSettings.uWidth == sm_DesiredWindow.uWidth	&& oNewSettings.uHeight == sm_DesiredWindow.uHeight)
			|| (oNewSettings.bFullscreen && sm_DesiredWindow.bFullscreen)))
	{
		pcdDisplayf("CheckForDeviceChanges: Matched desired window");
		SetMatchDesiredWindow(false);
	}

	sm_GlobalWindow = oNewSettings;

	if (sm_CurrentWindows[g_RenderThreadIndex].bFullscreen)
	{
		sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate = oNewSettings.uRefreshRate;
	}
	else
	{
		const grcAdapterD3D11* adapter = (grcAdapterD3D11*)grcAdapterManager::GetInstance()->GetAdapter(sm_AdapterOrdinal);
		const grcAdapterD3D11Output* pMonitor = adapter->GetOutput(sm_OutputMonitor);


		IDXGIOutput *pOutput = pMonitor->GetDeviceOutput();

		DXGI_OUTPUT_DESC outputDesc;
		pOutput->GetDesc(&outputDesc);

		MONITORINFOEX monitorInfo;
		monitorInfo.cbSize = sizeof(MONITORINFOEX);
		GetMonitorInfo(outputDesc.Monitor, &monitorInfo);

		HDC monitorHDC = CreateDC(NULL, monitorInfo.szDevice, NULL, NULL);
		float refreshRate = (float)GetDeviceCaps(monitorHDC, VREFRESH);
		DeleteDC(monitorHDC);

		DXGI_OUTPUT_DESC desc;
		unsigned int dpiX, dpiY;
		grcAdapterD3D11Output::GetDesc(adapter->GetHighPart(), adapter->GetLowPart(), pMonitor, desc, dpiX, dpiY);
		grcDisplayWindow desktopWindow;
		desktopWindow.uHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
		desktopWindow.uWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
		desktopWindow.uRefreshRate.Numerator = 60000; desktopWindow.uRefreshRate.Denominator = 1000;

		sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate = pMonitor->GetClosestRefreshRate(refreshRate, desktopWindow);

		pcdDisplayf("Setting refresh rate to match the windowed refresh rate of %d/%d", sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate.Numerator, sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate.Denominator);
	}
	sm_CurrentWindows[g_RenderThreadIndex].uFrameLock = oNewSettings.uFrameLock;  //Make certain we take the new framelock value even if a resizeBuffers didn't occur

	g_inWindow = !oNewSettings.bFullscreen;

	pcdDisplayf("CheckForDeviceChanges: At the end of the function with dimensions %dx%d, fullscreen %d, G_InWindow is %d", sm_GlobalWindow.uWidth, sm_GlobalWindow.uHeight, sm_GlobalWindow.bFullscreen, IsWindowed());

	sm_MonitorConfig.markDirty();

	SetInsideDeviceChange(false);

	static int iRetriesToMatchFullscreen = 4;
	if (oNewSettings.bFullscreen && iRetriesToMatchFullscreen > 0)
	{
		RECT rcCurrentClient;
		GetClientRect( g_hwndMain, &rcCurrentClient );
		IDXGISwapChain* pSwapChain = static_cast<IDXGISwapChain*>(GetSwapChain());
		Assert(pSwapChain != NULL);

		DXGI_SWAP_CHAIN_DESC oDesc;
		pSwapChain->GetDesc(&oDesc);

		if ((rcCurrentClient.right - rcCurrentClient.left) != (LONG)oDesc.BufferDesc.Width || (rcCurrentClient.bottom - rcCurrentClient.top) != (LONG)oDesc.BufferDesc.Height)
		{
			ChangeDevice(oNewSettings);
			pcdDisplayf("Backbuffer doesn't match client window dimensions, re-sending the requested device size");
			iRetriesToMatchFullscreen--;
			return false;
		}
	}

	iRetriesToMatchFullscreen = 4;

#if HANG_DETECT_THREAD
	// NOTE: We use GetForegroundWindow() instead of GetHasFocus() as GetHasFocus() is not always correct!
	if(bInSafeZone && ::GetForegroundWindow() == g_hwndMain)
	{
		HANG_DETECT_SAVEZONE_EXIT(NULL);
		bInSafeZone = false;
	}
#endif // HANG_DETECT_THREAD
	return true;
}

bool grcDevice::CheckDeviceStatus()
{
	if (IsMessagePumpThreadThatCreatedTheD3DDevice())
	{
		if (IsCreated())
		{
			s_SwapChainLock.Lock();
			if (GetSwapChain() != NULL)
			{
//  If you want to see these timebars feel free to uncomment, but the timebars will overflow whenever the renderthread/mainthread are de-coupled
//				PF_PUSH_TIMEBAR("Check Device Status");
				LockContext();
#if NV_SUPPORT
				LockContextNVStereo();
#endif
				sm_uDeviceStatus = ((IDXGISwapChain*)GetSwapChain())->Present(0, DXGI_PRESENT_TEST);
				CheckDxHresultFatal(sm_uDeviceStatus);
#if NV_SUPPORT
				UnlockContextNVStereo();
#endif
				UnlockContext();
//				PF_POP_TIMEBAR(); // "Check Device Status"
				if (FAILED(sm_uDeviceStatus))
				{
					SetLost (true);
					s_SwapChainLock.Unlock();
					return false;
				}
				if (sm_uDeviceStatus == DXGI_STATUS_OCCLUDED)
				{
					SetOccluded(true);
				}
				else
				{
					SetOccluded(false);
				}
			}
			s_SwapChainLock.Unlock();
		}
	}
	else
	{
		if (!IsCreated()			||
			IsLost()				||
			IsPaused()				||
			IsInReset()				||
			IsInsideDeviceChange()	||
			(IsMinimized() && !ContinueRenderingOverride())	||
			IsInSizeMove()			||
			IsReleasing())
		{
			return false;
		}

		if (IsDeviceRestored())
		{
			LockContext();
			Displayf ("STARTING DEVICE RESTORE");
			DeviceRestored();
			Displayf ("FINISHED DEVICE RESTORE");
			SetDeviceRestored(false);
#if RSG_PC && __DEV
			SetDeviceResetTestActive(false);
#endif
			UnlockContext();
			return false;
		}
	}
	return true;
}

bool grcDevice::IsReady ()
{
	return	IsCreated()				&&
			!IsLost()				&&
			!IsPaused()				&&
			!IsInReset()			&&
			!IsInsideDeviceChange()	&&
			(!IsMinimized() || ContinueRenderingOverride())			&&
			!IsInSizeMove()			&&
			!IsReleasing()			&&
			(!IsOccluded() || IsSwapChainFullscreen() || ContinueRenderingOverride())			&&
			!IsDeviceRestored();
}

// PURPOSE: Resize Buffers
bool grcDevice::ResizeBuffers(grcDisplayWindow oNewSettings)
{
	pcdDisplayf("RESIZEBUFFERS %dx%d, fullscreen %d", oNewSettings.uWidth, oNewSettings.uHeight, oNewSettings.bFullscreen);

	s_SwapChainLock.Lock();

	IDXGISwapChain* pSwapChain = static_cast<IDXGISwapChain*>(GetSwapChain());
	Assert(pSwapChain != NULL);

	DXGI_SWAP_CHAIN_DESC oDesc;
	pSwapChain->GetDesc(&oDesc);

	pcdDisplayf("SwapChain description BEFORE ResizeBuffers is Width: %d, Height: %d, RefreshRate %d//%d", oDesc.BufferDesc.Width, oDesc.BufferDesc.Height, oDesc.BufferDesc.RefreshRate.Numerator, oDesc.BufferDesc.RefreshRate.Denominator);

	RECT rcCurrentClient;
	GetClientRect( g_hwndMain, &rcCurrentClient );
	pcdDisplayf("Window BEFORE ResizeBuffers is left: %d, right: %d, top: %d, bottom: %d", rcCurrentClient.left, rcCurrentClient.right, rcCurrentClient.top, rcCurrentClient.bottom);

	/*
	BOOL bFullScreen;
	pSwapChain->GetFullscreenState( &bFullScreen, NULL );

	grcDeviceDisplay("Fullscreen state is %d", bFullScreen);
*/

	LockDeviceResetAvailability();

	LockContext();
	

//	grcDisplayf ("ResizeBuffers::PrepareForShutdown");
	PrepareForShutdown();

	DeviceLost();

	RESOURCE_CACHE_ONLY(grcResourceCache::DeviceLost();)


    // ResizeBuffers
	HRESULT hr = pSwapChain->ResizeBuffers( oDesc.BufferCount,    //0,
											  oNewSettings.uWidth,  //0,
											  oNewSettings.uHeight,  //0,
											  oDesc.BufferDesc.Format,  //DXGI_FORMAT_UNKNOWN
											  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH );
	Assertf(hr == S_OK, "Resize Buffers failed %x", hr);
	s_SwapChainLock.Unlock();
	if (hr != S_OK)
	{
		UnlockContext();
		pcdDisplayf("RESIZEBUFFERS FAILED");
		return false;
	}

	g_inWindow = !oNewSettings.bFullscreen;

	{
		BOOL bFullScreen = false;
		((IDXGISwapChain*)GRCDEVICE.GetSwapChain())->GetFullscreenState(&bFullScreen, NULL);

		Assert(oNewSettings.bFullscreen == (bFullScreen == TRUE));
		SetSwapChainFullscreen((bFullScreen == TRUE));
	}

	pcdDisplayf("G_InWindow now %d", g_inWindow);

	pcdDisplayf("RESIZEBUFFERS SUCCEEDED");

	sm_CurrentWindows[g_RenderThreadIndex] = oNewSettings;
	sm_GlobalWindow = oNewSettings;

//	grcDisplayf ("ResizeBuffers::TextureFactory::Reset");

	UpdatePresentParameters();

	RESOURCE_CACHE_ONLY(grcResourceCache::DeviceReset();)

	DeviceRestored();

	SetDeviceRestored(true);
	//grcDisplayf ("ResizeBuffers::DeviceRestored->true");

	UnlockContext();

	UnlockDeviceResetAvailability();

    // Setup cursor based on current settings (window/fullscreen mode, show cursor state, clip cursor state)
    SetupCursor(false);

    SetInReset( false );
    SetPaused( false );

	ShowWindow( g_hwndMain, SW_SHOW );

	pSwapChain->GetDesc(&oDesc);
	pcdDisplayf("SwapChain description AFTER ResizeBuffers is Width: %d, Height: %d, RefreshRate %d//%d", oDesc.BufferDesc.Width, oDesc.BufferDesc.Height, oDesc.BufferDesc.RefreshRate.Numerator, oDesc.BufferDesc.RefreshRate.Denominator);

	GetClientRect( g_hwndMain, &rcCurrentClient );
	pcdDisplayf("Window AFTER ResizeBuffers is left: %d, right: %d, top: %d, bottom: %d", rcCurrentClient.left, rcCurrentClient.right, rcCurrentClient.top, rcCurrentClient.bottom);

	return true;
}

static bool dont_override() { 
	return false;
}
bool (*grcDevice::CursorVisibilityOverride)() = dont_override;

void grcDevice::SetupCursor(WIN32PC_ONLY(bool bEnable))
{
#if RSG_PC
	if (CursorVisibilityOverride()) return;

	ioMouse::SetCursorVisible(bEnable);
#endif // RSG_PC
}

#if NV_SUPPORT
void grcDevice::BeginRscRendering(grcRenderTarget *pRT, u32 flags)
{
	NvAPI_Status status = NVAPI_ERROR;
	NVDX_ObjectHandle n;
	status = NvAPI_D3D_GetObjectHandleForResource(GetCurrent(), (ID3D11Resource*)(pRT->GetTexturePtr()), &n);
	Assertf(status == NVAPI_OK, "NvAPI_D3D_GetObjectHandleForResource failed.\n");

	status = NvAPI_D3D_BeginResourceRendering(GetCurrent(), n, flags);
	Assertf(status == NVAPI_OK, "NvAPI_D3D_BeginResourceRendering failed.\n");

}

void grcDevice::EndRscRendering(grcRenderTarget *pRT)
{
	NvAPI_Status status = NVAPI_ERROR;
	NVDX_ObjectHandle n;
	status = NvAPI_D3D_GetObjectHandleForResource(GetCurrent(), (ID3D11Resource*)(pRT->GetTexturePtr()), &n);
	Assertf(status == NVAPI_OK, "NvAPI_D3D_GetObjectHandleForResource failed.\n");

	status = NvAPI_D3D_EndResourceRendering(GetCurrent(), n, 0);
	Assertf(status == NVAPI_OK, "NvAPI_D3D_EndResourceRendering failed.\n");
}
#endif	// NV_SUPPORT
#endif	// __WIN32PC

#if RSG_DURANGO
void grcDevice::SetIndices(const u16 *indices)
{
	g_grcCurrentContext->IASetPlacementIndexBuffer(s_CommonBufferIB, const_cast<u16*>(indices), DXGI_FORMAT_R16_UINT);
}
#endif

void grcDevice::SetIndices(const grcIndexBuffer &Buffer)
{
#if RSG_DURANGO
	SetIndices((u16*)Buffer.GetUnsafeReadPtr());
#else
	ID3D11Buffer *pBuffer = (ID3D11Buffer *)Buffer.GetD3DBuffer();
	g_grcCurrentContext->IASetIndexBuffer(pBuffer, DXGI_FORMAT_R16_UINT, 0);
#endif
}

void grcDevice::SetStreamSource(u32 streamNumber, const grcVertexBuffer &streamData, u32 offsetInBytes, u32 stride)
{
#if RSG_DURANGO
	g_grcCurrentContext->IASetPlacementVertexBuffer(streamNumber, s_CommonBufferVB, (char*)streamData.GetUnsafeReadPtr() + offsetInBytes, stride);
#else
	ID3D11Buffer *pBuffer = streamData.GetD3DBuffer();
	SetStreamSource(streamNumber, pBuffer, offsetInBytes, stride);
#endif
}

#if RSG_DURANGO
void grcDevice::SetStreamSource(u32 streamNumber, const void* pStreamData, u32 stride)
{
	g_grcCurrentContext->IASetPlacementVertexBuffer(streamNumber, s_CommonBufferVB, const_cast<void*>(pStreamData), stride);
}
#else
void grcDevice::SetStreamSource(u32 streamNumber, const ID3D11Buffer* pStreamData, u32 offsetInBytes, u32 stride)
{
	sm_aVertexStreamInfo[g_RenderThreadIndex].pVertexBuffer[streamNumber] = (IUnknown*)(pStreamData);
	sm_aVertexStreamInfo[g_RenderThreadIndex].offsetInBytes[streamNumber] = offsetInBytes;
	sm_aVertexStreamInfo[g_RenderThreadIndex].stride[streamNumber] = stride;
}
#endif

void grcDevice::ClearStreamSource(u32 streamNumber)
{
#if RSG_DURANGO
	streamNumber;
	// This is pointless
	//g_grcCurrentContext->IASetPlacementVertexBuffer(streamNumber, s_CommonBufferVB, NULL, 0);
#else
	sm_aVertexStreamInfo[g_RenderThreadIndex].pVertexBuffer[streamNumber] = 0;
	sm_aVertexStreamInfo[g_RenderThreadIndex].offsetInBytes[streamNumber] = 0;
	sm_aVertexStreamInfo[g_RenderThreadIndex].stride[streamNumber] = 0;
#endif
}

grcOcclusionQuery grcDevice::CreateOcclusionQuery(int /*tileCount*/)
{
	D3D11_QUERY_DESC oDesc;
	oDesc.Query = D3D11_QUERY_OCCLUSION;
	oDesc.MiscFlags = 0;

	grcOcclusionQuery query;
	HRESULT hr = S_OK;
	hr = sm_Current->CreateQuery(&oDesc, (ID3D11Query**)&query);
	Assertf(hr == S_OK, "Create Occlussion Query Failed %x", hr);
	return query;
}

void grcDevice::DestroyOcclusionQuery(grcOcclusionQuery& query)
{
	AssertVerify(query->Release() == 0);
	query = NULL;
}

void grcDevice::BeginOcclusionQuery(grcOcclusionQuery query)
{
	Assert(query != NULL);
	g_grcCurrentContext->Begin((ID3D11Query*)query);
}

void grcDevice::EndOcclusionQuery(grcOcclusionQuery query)
{
	Assert(query != NULL);
	g_grcCurrentContext->End((ID3D11Query*)query);
}

bool grcDevice::GetOcclusionQueryData(grcOcclusionQuery query, u32& numDrawn)
{
	UINT64 pixelsDrawn;
	HRESULT res = g_grcCurrentContext->GetData((ID3D11Query*)query,&pixelsDrawn,sizeof(UINT64),0);
	if(res == S_OK)
	{
		numDrawn = static_cast<u32>(pixelsDrawn);
		return true;
	}

	numDrawn = 0;
	return false;	
}

grcConditionalQuery grcDevice::CreateConditionalQuery()
{
	grcConditionalQuery query;

	D3D11_QUERY_DESC oDesc;
	oDesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
	oDesc.MiscFlags = 0;	// This will force a wait, use D3D11_QUERY_MISC_PREDICATEHINT to make it not wait, but this might break things. 
							// may be. and won't match the other platform. but PC being PC, we might want to make it not wait for the results.
	ASSERT_ONLY(HRESULT hRes = )sm_Current->CreatePredicate(&oDesc, (ID3D11Predicate**)&query);
	Assertf(hRes == S_OK, "CreatePredicate Failed %x", hRes);
	return query;
}

void grcDevice::DestroyConditionalQuery(grcConditionalQuery &query)
{
	((ID3D11Predicate*)query)->Release();
	query = NULL;
}

void grcDevice::BeginConditionalQuery(grcConditionalQuery query)
{
	g_grcCurrentContext->Begin(((ID3D11Predicate*)query));
}

void grcDevice::EndConditionalQuery(grcConditionalQuery query)
{
	g_grcCurrentContext->End(((ID3D11Predicate*)query));
}

void grcDevice::BeginConditionalRender(grcConditionalQuery query)
{
	g_grcCurrentContext->SetPredication( (ID3D11Predicate*)query, false );
}

void grcDevice::EndConditionalRender(grcConditionalQuery /*query*/)
{
	g_grcCurrentContext->SetPredication(NULL, false);
}

void grcDevice::SetProgramResources()
{
#if RSG_PC
	// TODO:- Add over-rides for other units.
	sm_CurrentVertexProgram->SetConstantBuffer<grcVertexProgram>(sm_VSConstantBufferOverrides,sm_VSConstantBufferOverrideStartSlot,sm_VSConstantBufferOverrideCount);
	sm_CurrentFragmentProgram->SetConstantBuffer<grcFragmentProgram>();
	sm_CurrentComputeProgram->SetConstantBuffer<grcComputeProgram>();
	sm_CurrentDomainProgram->SetConstantBuffer<grcDomainProgram>();
	sm_CurrentGeometryProgram->SetConstantBuffer<grcGeometryProgram>();
	sm_CurrentHullProgram->SetConstantBuffer<grcHullProgram>();

	s_ShaderStagesLastActive = s_ShaderStagesActive;

	//sm_CurrentVertexProgram->SetTextureResource(VS_TYPE);
	sm_CurrentFragmentProgram->SetTextureResourcesUsingVectorDXAPICall(PS_TYPE);
	//sm_CurrentComputeProgram->SetTextureResource(CS_TYPE);
	//sm_CurrentDomainProgram->SetTextureResource(DS_TYPE);
	//sm_CurrentGeometryProgram->SetTextureResource(GS_TYPE);
	//sm_CurrentHullProgram->SetTextureResource(HS_TYPE);

	// this should be handled by RunComputation()
	//sm_CurrentComputeProgram->SetUnorderedResource();

#elif RSG_DURANGO

	// Yes this is odd only binding the current vertex program, but this
	// behaviour matches the Orbis implementation.

	sm_CurrentVertexProgram->Bind();

#endif
}


void grcDevice::SetUpPriorToDraw(grcDrawMode dm)
{
#if !__FINAL
	if((dm == drawTriFan) || (dm == drawQuads) || (dm == drawRects))
	{
		AssertMsg( false, "grcDevice::SetUpPriorDraw()...Unsupported primitive type!");
	}
#endif // !__FINAL

	SetInputLayout();

	// Determine how many vertex buffers are set.
#if !RSG_DURANGO
	u32 uNumBuffers = 0;
	for (u32 i = 0; i < (u32)MAX_RAGE_VERTEXBUFFER_SLOTS; i++)
	{
		if (sm_aVertexStreamInfo[g_RenderThreadIndex].pVertexBuffer[i])
		{
			uNumBuffers++;
		}
		else
		{
			// For now, assume 'streams' are set starting with stream 0 and there are no gaps.
			break;
		}
	}

	if (uNumBuffers > 0)
	{
		Assert(sm_aVertexStreamInfo[g_RenderThreadIndex].pVertexBuffer[0]);
		g_grcCurrentContext->IASetVertexBuffers(0, uNumBuffers, (ID3D11Buffer* const *)sm_aVertexStreamInfo[g_RenderThreadIndex].pVertexBuffer, sm_aVertexStreamInfo[g_RenderThreadIndex].stride, sm_aVertexStreamInfo[g_RenderThreadIndex].offsetInBytes);
	}
#endif

	if (sm_CurrentHullProgram && sm_CurrentHullProgram->GetProgram() == NULL)
	{
		D3D11_PRIMITIVE_TOPOLOGY ePrimTop = TranslateDrawMode[dm];
		g_grcCurrentContext->IASetPrimitiveTopology(ePrimTop);
	}
	else
	{
		g_grcCurrentContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	}

	UpdateCurrentState();

	SetProgramResources();
	ResolveClipPlanes();

#if D3D11_RENDER_TARGET_COPY_OPTIMISATION_TEST
	grcRenderTargetDX11::CheckDepthWriting();
#endif
}

void grcDevice::DrawIndexedPrimitive(grcDrawMode dm, int startIndex, int indexCount)
{
	grcStateBlock::Flush();

	SetUpPriorToDraw(dm);

#if RAGE_INSTANCED_TECH
	if (grcViewport::GetInstancing()) {
		g_grcCurrentContext->DrawIndexedInstanced(indexCount, grcViewport::GetVPInstCount(), startIndex,0,0);
	#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			u32 numIndices = indexCount*grcViewport::GetVPInstCount();
			g_pCurrentStatsBucket->TotalIndices += numIndices;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(numIndices, dm);
		}
	#endif
	}
	else
#endif
	{
		g_grcCurrentContext->DrawIndexed(indexCount, startIndex, 0);
	#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			g_pCurrentStatsBucket->TotalIndices += indexCount;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(indexCount, dm);
		}
	#endif
	}

#if DEBUG_SEALING_OF_DRAWLISTS
	grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
}

void grcDevice::DrawIndexedPrimitive(grcDrawMode dm,const grcVertexDeclaration *decl,const grcVertexBuffer &vb,const grcIndexBuffer &ib, int customIndexCount)
{
	grcStateBlock::Flush();

	Assert(customIndexCount != 0);
	FatalAssert(customIndexCount <= ib.GetIndexCount());

	SetVertexDeclaration(decl);
	SetUpPriorToDraw(dm);
	SetIndices(ib);
	SetStreamSource(0, vb, 0, vb.GetVertexStride());
	DrawIndexedPrimitive(dm, 0, customIndexCount);
}


void grcDevice::DrawPrimitive(grcDrawMode dm, int startVertex, int vertexCount)
{
	grcStateBlock::Flush();

	SetUpPriorToDraw(dm);
	g_grcCurrentContext->Draw(vertexCount, startVertex);
#if DEBUG_SEALING_OF_DRAWLISTS
	grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
#if DRAWABLE_STATS
	if(g_pCurrentStatsBucket)
	{
		g_pCurrentStatsBucket->TotalIndices += vertexCount;
		g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(vertexCount, dm);
	}
#endif
}

void grcDevice::DrawPrimitive(grcDrawMode dm, const grcVertexDeclaration *decl,const grcVertexBuffer &vb, int startVertex, int vertexCount)
{
	grcStateBlock::Flush();

	SetVertexDeclaration(decl);
	SetUpPriorToDraw(dm);
	// PC TODO - Stride I think needs to come from declaration.  I'm not sure how well this is going to work.
	if (&vb)
	{
		SetStreamSource(0, vb, 0, vb.GetVertexStride());
	}else
	{
		ClearStreamSource(0);
	}

	DrawPrimitive(dm, startVertex, vertexCount);
}

void grcDevice::DrawInstancedPrimitive(grcDrawMode dm, int vertexCountPerInstance,  int instanceCount, int startVertex, int startInstance, bool alreadySetupPriorToDraw)
{
	grcStateBlock::Flush();

	if (!alreadySetupPriorToDraw)
		SetUpPriorToDraw(dm);

	g_grcCurrentContext->DrawInstanced(vertexCountPerInstance, instanceCount, startVertex, startInstance);
#if DEBUG_SEALING_OF_DRAWLISTS
	grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
#if DRAWABLE_STATS
	if(g_pCurrentStatsBucket)
	{
		g_pCurrentStatsBucket->TotalIndices += vertexCountPerInstance*instanceCount;
		g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(vertexCountPerInstance*instanceCount, dm);
	}
#endif
}


void grcDevice::DrawInstancedIndexedPrimitive(grcDrawMode dm, int indexCountPerInstance, int instanceCount, int startIndex, int startVertex, int startInstance, bool alreadySetupPriorToDraw)
{
	grcStateBlock::Flush();

	if( !alreadySetupPriorToDraw )
		SetUpPriorToDraw(dm);

#if RAGE_INSTANCED_TECH
	if (grcViewport::GetInstancing()) {
		g_grcCurrentContext->DrawIndexedInstanced(indexCountPerInstance, instanceCount * grcViewport::GetVPInstCount(), startIndex, startVertex, startInstance);
	#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			u32 numIndices = indexCountPerInstance * instanceCount * grcViewport::GetVPInstCount();
			g_pCurrentStatsBucket->TotalIndices += numIndices;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(numIndices, dm);
		}
	#endif
	}
	else
#endif
	{
		g_grcCurrentContext->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndex, startVertex, startInstance);
	#if DRAWABLE_STATS
		if(g_pCurrentStatsBucket)
		{
			u32 numIndices = indexCountPerInstance * instanceCount;
			g_pCurrentStatsBucket->TotalIndices += numIndices;
			g_pCurrentStatsBucket->TotalPrimitives += IndexCountToPrimitiveCount(numIndices, dm);
		}
	#endif
	}
#if DEBUG_SEALING_OF_DRAWLISTS
	grcTextureFactory::GetInstance().OnDraw();
#endif // DEBUG_SEALING_OF_DRAWLISTS
}

void grcDevice::Dispatch(u32 groupX, u32 groupY, u32 groupZ)
{
#if RSG_PC
	FatalAssert(SupportsFeature(COMPUTE_SHADER_50)); // I haven't seen a 4.0 compute shader yet
#endif // RSG_PC

	g_grcCurrentContext->Dispatch( groupX, groupY, groupZ );

	grcComputeProgram::ResetUnorderedResource();
}

void grcDevice::RunComputation( const char* RAGE_TIMEBARS_ONLY(pDebugStr), grmShader &shader, u32 programId, u32 groupX, u32 groupY, u32 groupZ )
{
	RAGE_TIMEBARS_ONLY(pfAutoMarker pf(pDebugStr,0);)

	// bind resources and UAVs
	grcComputeProgram *const pComputeProgram = shader.GetComputeProgram( programId );
	pComputeProgram->Bind( shader.GetInstanceData(), shader.GetInstanceData().GetBasis().GetLocalVar() );

	Dispatch(groupX, groupY, groupZ);
}

void grcDevice::CSEnableAutomaticGpuFlush(bool DURANGO_ONLY(enable))
{
#if RSG_DURANGO
	grcAssertf(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcDevice::CSEnableAutomaticGpuFlush()...Must call this from the render thread!\n");
	g_grcCurrentContext->CSEnableAutomaticGpuFlush(enable);
#endif
}

void grcDevice::SynchronizeComputeToGraphics()
{
#if RSG_DURANGO
	grcAssertf(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcDevice::SynchronizeComputeToGraphics()...Must call this from the render thread!\n");
	g_grcCurrentContext->GpuSendPipelinedEvent(D3D11X_GPU_PIPELINED_EVENT_CS_PARTIAL_FLUSH);
#endif
}

void grcDevice::SynchronizeComputeToCompute()
{
}

#if !RSG_DURANGO
static DECLARE_MTR_THREAD int s_BeginBlit;
#endif

void grcDevice::BeginBlit() {
#if !RSG_DURANGO
	if (++s_BeginBlit == 1) {
#if __WIN32PC
#if __D3D9
		CHECK_HRESULT(g_grcCurrentContext->SetRenderState(D3DRS_CLIPPING, false));
#endif

		// Setting the stream source before you lock seems marginally faster.
		u32 uVertexSize = sizeof(BlitVert);
		u32 uOffset = 0;
		////g_grcCurrentContext->IASetVertexBuffers(0, 1, (VERTEXBUFFERDX11**)(s_BlitVB->GetD3DBuffer()), &uVertexSize, &uOffset);

		ID3D11Buffer *pBuffer = s_BlitVB->GetD3DBuffer();
		ID3D11Buffer *Buffers[2] = { pBuffer, pBuffer };

		u32 uStrides[2] = { uVertexSize, 0 };
		u32 uOffsets[2] = { uOffset, 0 };
		g_grcCurrentContext->IASetVertexBuffers(0, 2, Buffers, uStrides, uOffsets);

#elif __XENON
		sm_Current->SetRenderState(D3DRS_VIEWPORTENABLE, false);
#endif

		if (!grcEffect::IsInDraw())
			g_DefaultEffect->Bind(s_DefaultBlit);
	}
#endif
}


void grcDevice::EndBlit() {
#if !RSG_DURANGO
	if (--s_BeginBlit == 0) {
#if __WIN32PC && __D3D9
		CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_CLIPPING, true));
#elif __XENON
		sm_Current->SetRenderState(D3DRS_VIEWPORTENABLE, true);
#endif
	}
#endif
}


#pragma warning(disable: 4996)

void grcDevice::BlitRect(int x1, int y1, int x2, int y2, float zVal, int u1, int v1, int u2,int v2, const Color32 &color) {
	WIN32PC_ONLY(if (!sm_Current) return);

#if __D3D11
	float itu=1.0f; 
	float itv=1.0f;
	const float shim = 0.0f;
#else
	const float shim = 0.5f;

	const grcTexture *tex = grcGetTexture();
	if (tex) {
		itu = 1.0f / tex->GetWidth();
		itv = 1.0f / tex->GetHeight();
	}
	else {
		itu = itv = 0;
	}
#endif // __D3D11
	BlitRectf(float(x1)-shim,float(y1)-shim,float(x2)-shim,float(y2)-shim,zVal,
		float(u1)*itu,float(v1)*itv,float(u2)*itu,float(v2)*itv,color);
}


void grcDevice::BlitRectf(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color) {
	if (!sm_Current) return;
	SetVertexDeclaration(sm_BlitDecl);

#if RSG_DURANGO

	GRC_ALLOC_SCOPE_AUTO_PUSH_POP()

	BlitVert *const bv = grcGfxContext::current()->allocateTemp<BlitVert>(4);
	bv[0].Set( x1, y1, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u1, v1 );
	bv[1].Set( x1, y2, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u1, v2 );
	bv[2].Set( x2, y1, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u2, v1 );
	bv[3].Set( x2, y2, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u2, v2 );

	SetStreamSource(0, bv, sizeof(*bv));

	if (!grcEffect::IsInDraw())
		g_DefaultEffect->Bind(s_DefaultBlit);

	DrawPrimitive(drawTriStrip, 0, 4);

#else

	BeginBlit();

	u32 lockFlags = grcsNoOverwrite;
	if ((s_BlitOffset + 4) > BlitVertSize) {
		s_BlitOffset = 0;
		lockFlags = grcsDiscard;
	}

	if (s_BlitVB->Lock(lockFlags, s_BlitOffset*sizeof(BlitVert),4*sizeof(BlitVert)))
	{
		BlitVert *bv = (BlitVert *)s_BlitVB->GetLockPtr();
		if (bv != NULL)
		{
			bv[0].Set( x1, y1, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u1, v1 );
			bv[1].Set( x1, y2, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u1, v2 );
			bv[2].Set( x2, y1, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u2, v1 );
			bv[3].Set( x2, y2, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u2, v2 );
		}
		s_BlitVB->UnlockRW();
	}

	DrawPrimitive(drawTriStrip, s_BlitOffset, 4);

	s_BlitOffset += 4;
	EndBlit();

#endif
}


void grcDevice::BlitText(int posx,int posy,float posz,const s16 *destxywh,const u8 *srcxywh,int count,Color32 color,bool /*bilinear*/) {
	// TODO: Replace with something faster
	while (count--) {
		BlitRect(posx+(destxywh[0]>>4),posy+(destxywh[1]>>4),posx+((destxywh[0]+destxywh[2])>>4),posy+((destxywh[1]+destxywh[3])>>4),posz,
			srcxywh[0],srcxywh[1],srcxywh[0]+srcxywh[2],srcxywh[1]+srcxywh[3],color);
		destxywh += 4;
		srcxywh += 4;
	}
}


grcImage *grcDevice::CaptureScreenShot(grcImage* pImage) 
{
	PIXBegin(0, "CaptureScreenShot");

#if __WIN32PC
	RECT screen, window;
	GetWindowRect(GetDesktopWindow(),&screen);
	GetClientRect(g_hwndMain,&window);
	POINT origin = { 0,0 };
	MapWindowPoints(g_hwndMain, HWND_DESKTOP, &origin, 1);
	window.left += origin.x;
	window.top += origin.y;
	window.right += origin.x;
	window.bottom += origin.y;
	int width = window.right - window.left;
	int height = window.bottom - window.top;
#elif RSG_DURANGO
	int width = grcDevice::sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	int height = grcDevice::sm_CurrentWindows[g_RenderThreadIndex].uHeight;
#else
	int width = 1280, height = 720;		// HACK HACK HACK
#endif
 
	grcImage *dest = NULL;

#if NV_SUPPORT
	Assert(IsMessagePumpThreadThatCreatedTheD3DDevice());

	if (IsStereoEnabled() && CanUseStereo())
	{
		NvAPI_Status status = NVAPI_ERROR;
		status = NvAPI_Stereo_CapturePngImage(sm_pStereoHandle);

		if (status != NVAPI_OK)
			Errorf("Could not take stereo screenshot. Status Code: %d", status);

		Displayf("Stereo screenshot saved to C:/Users/<username>/My Documents/NVStereoscopic3D.IMG/");
	}
	else
#endif // NV_SUPPORT
	{
		grcTextureFactory::TextureCreateParams oParams( grcTextureFactory::TextureCreateParams::STAGING, 
														grcTextureFactory::TextureCreateParams::LINEAR, 
														grcsWrite|grcsRead, 
														NULL, 
														grcTextureFactory::TextureCreateParams::NORMAL, 
														grcTextureFactory::TextureCreateParams::MSAA_NONE);
		u32 format = (u32)(grctfA8R8G8B8);
		ASSERT_ONLY(RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().SetSafeResourceCreate(true);))
		grcTexture* pictureTexture = grcTextureFactoryPC::GetInstance().Create((u32)width, (u32)height, format, NULL, 1U /*numMips*/, &oParams);
		ASSERT_ONLY(RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().SetSafeResourceCreate(false);))

		if (pictureTexture == NULL)
		{
			grcErrorf("Failed to create texture target for screenshot.\n");
			PIXEnd();
			return NULL;
		}
		
#if RSG_DURANGO
		ID3D11Resource* pictureCachedTexturePtr = pictureTexture->GetTexturePtr();
#else
		ID3D11Resource* pictureCachedTexturePtr = (ID3D11Resource*)(pictureTexture->GetCachedTexturePtr());
#endif // RSG_DURANGO;

		// If we are not running on a render thread (ie, have no D3D context),
		// then lock to get one.  Do not lock if we are on a subrender thread
		// though, as the primary render thread already own the lock so
		// LockContext will cause a deadlock.
		bool lockedContext = false;
		grcContextHandle *ctx = g_grcCurrentContext;
		if (!ctx)
		{
			LockContext();
			lockedContext = true;
			ctx = g_grcCurrentContext;
			Assert(ctx);
		}

		if (sm_pBackBuffer->GetMSAA() != grcDevice::MSAA_None)
		{
			ID3D11Resource* backbufferTexturePtr = (sm_pBackBuffer)->GetTexturePtr();
			ctx->ResolveSubresource(pictureCachedTexturePtr, 0, backbufferTexturePtr, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		}
		else
		{
			// Durango could feasibly avoid the copy and just read the bits if required.
			ID3D11Resource* backbufferTexturePtr = sm_pBackBuffer->GetTexturePtr();
			ctx->CopyResource(pictureCachedTexturePtr, backbufferTexturePtr);
		}

		// For XB1 we need to wait for GPU idle.  For PC, this happens
		// automagically when we lock the resource.
#if RSG_DURANGO
		if (lockedContext)
		{
			// Use the D3D fences rather than CpuWaitOnGpuIdle since we don't
			// have a grcGfxContext setup correctly.  ctx is the D3D immediate
			// context, so D3D fences are fine.
			const UINT fenceFlags = 0;
			const UINT64 fence = ctx->InsertFence(fenceFlags);
			grcDeviceHandle *const dev = GetCurrent();
			while (dev->IsFencePending(fence))
			{
				sysIpcSleep(1);
			}
		}
		else
		{
			CpuWaitOnGpuIdle();
		}
#endif

		((grcTextureD3D11*)pictureTexture)->UpdateCPUCopy();

		if (lockedContext)
		{
			UnlockContext();
		}

		grcTextureLock oLock;

		bool lockRect = pictureTexture->LockRect(0,0, oLock);
		u32 pitch = oLock.Pitch;
		u32 *s = (u32*)oLock.Base;

 		if (Verifyf(lockRect, "Couldn't lock rect."))
		{
			if (pImage)
			{
			   dest = pImage;
			}
			else
			{
				dest = grcImage::Create(width,height,1,grcImage::A8R8G8B8,grcImage::STANDARD,0,0);

				if (!dest)
				{
					grcErrorf("Failed to create grcImage for screenshot.\n");
					pictureTexture->UnlockRect(oLock);
					pictureTexture->Release();
					PIXEnd();
					return NULL;
				}
			}
			// Transfer Texture to Image
			u32 *d = (u32*) dest->GetBits();
			for (int row=0; row<height; row++,s = (u32*)((char*)s + pitch),d += width) {
				for (int col=0; col<width; col++)
				{
					Color32 oColor = Color32(s[col]);
#if RSG_DURANGO
					if (GetMSAA() != MSAA_None) // not sure why durango msaa is the only case where we need to _not_ swap red and blue ..
					{
						oColor.SetAlpha(255);
						d[col] = oColor.GetColor();
					}
					else
#endif // RSG_DURANGO
					{
//						d[col] = 0xFF000000 | oColor.GetBlue() << 16 | oColor.GetGreen() << 8 | oColor.GetRed();
						d[col] = 0xFF000000 | oColor.GetRed() << 16 | oColor.GetGreen() << 8 | oColor.GetBlue();
					}
				}
			}
			pictureTexture->UnlockRect(oLock);
		}
		pictureTexture->Release();
	}

	PIXEnd();
	return dest;
}

void grcDevice::BeginTakeScreenShot()
{
	// Just copies the back buffer for screen shot.  No need to mess with render targets this way
}

grcImage *grcDevice::EndTakeScreenShot()
{
#if __WIN32PC || RSG_DURANGO
	grcTextureD3D11 *pictureTexture = NULL;
	grcTextureFactory::TextureCreateParams oParams( grcTextureFactory::TextureCreateParams::STAGING,
													grcTextureFactory::TextureCreateParams::LINEAR,
													0,
													NULL,
													grcTextureFactory::TextureCreateParams::NORMAL,
													grcTextureFactory::TextureCreateParams::MSAA_NONE);

	pictureTexture = static_cast<grcTextureD3D11*>(grcTextureFactoryPC::GetInstance().Create((u32)sm_pBackBuffer->GetWidth(), (u32)sm_pBackBuffer->GetHeight(), (u32)grctfA8R8G8B8, NULL, 1U /*numMips*/, &oParams));
	if (pictureTexture == NULL)
	{
		grcErrorf("Failed to create texture target for screenshot.\n");
		return NULL;;
	}

	grcImage *dest = NULL;

#if __WIN32PC
	RECT window;
	GetClientRect(g_hwndMain,&window);

	int width = window.right - window.left;
	int height = window.bottom - window.top;
#elif RSG_DURANGO
	int width = grcDevice::sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	int height = grcDevice::sm_CurrentWindows[g_RenderThreadIndex].uHeight;
#endif
	g_grcCurrentContext->CopyResource((pictureTexture->GetCachedTexturePtr()), sm_pBackBuffer->GetTexturePtr());

	grcTextureLock oLock;
	if (SUCCEEDED(pictureTexture->LockRect(0,0,oLock))) {

		dest = grcImage::Create(width,height,1,grcImage::A8R8G8B8,grcImage::STANDARD,0,0);

		u32 *d = (u32*) dest->GetBits();
		u32 *s = (u32*) oLock.Base;
		for (int row=0; row<height; row++,s = (u32*)((char*)s + oLock.Pitch),d += width) 
		{
			for (int col=0; col<width; col++)
			{
				Color32 oColor = Color32(s[col]);
				d[col] = 0xFF000000 | oColor.GetRed() << 16 | oColor.GetGreen() << 8 | oColor.GetBlue();
			}
		}
		pictureTexture->UnlockRect(oLock);
	}
	else
		grcErrorf("Only 32 bit per pixel (A8R8G8B8) video format supported.");

	pictureTexture->Release();

	return dest;
#else
	return NULL;
#endif
}

void grcDevice::GetMultiSample(u32 &uType, u32 &uQuality)
{
	uType = sm_MSAA;
#if DEVICE_EQAA
	uQuality = sm_MSAA.DeriveQuality();
#else
	uQuality = sm_MultisampleQuality;
#endif
}

int grcVertexDeclaration::Release() const
{
	if (--refCount == 0) {
		delete this;
		return 0;
	}
	else
		return refCount;
}

void grcVertexDeclaration::AddRef() const
{
	++refCount;
}

#if __WIN32PC
void grcDevice::DeviceLostShutdown()
{

	Blit_Shutdown();
#if FAST_QUAD_SUPPORT
	FastQuad::Shutdown();
#endif

	const int loopCount = sm_DeviceLostCb.GetCount();
	for (int i = 0; i < loopCount; ++i) {
		(sm_DeviceLostCb[i])();		// Make the callback now
	}
}

void grcDevice::DeviceLost() 
{
	DeviceLostShutdown();
	RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().UnloadCache());

	if (grcTextureFactoryPC::HasInstance())
		((grcTextureFactoryDX11 *)(&grcTextureFactoryDX11::GetInstance()))->Lost();

	RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().UnloadCache());
}

void grcDevice::DeviceRestored() {
	if (IsMessagePumpThreadThatCreatedTheD3DDevice())
	{
		sm_MonitorConfig.markDirty();
		// TODO: This was disabled on WIN32PC due to MT issues, but that breaks any normal app running in rag.
		// Make sure the states are dirty
		ASSERT_ONLY(RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().SetSafeResourceCreate(true);))
		if (grcTextureFactoryPC::HasInstance())
		{
			grcStateBlock::Default();

			((grcTextureFactoryDX11 *)(&grcTextureFactoryDX11::GetInstance()))->Reset();
		}


#if FAST_QUAD_SUPPORT
		FastQuad::Init();
#endif
		Blit_Init();
		ASSERT_ONLY(RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().SetSafeResourceCreate(false);))

//#if RSG_PC
//		if (sm_bStereoEnabled && sm_bCanUseStereo)
//		{		
//			GRCDEVICE.DeInitStereo();
//			GRCDEVICE.InitializeStereo();
//		}
//#endif
	}
	else
	{
		ASSERT_ONLY(RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().SetSafeResourceCreate(true);))
		const int loopCount = sm_DeviceResetCb.GetCount();
		for (int i = 0; i < loopCount; ++i) {
			(sm_DeviceResetCb[i])();		// Make the callback now
		}
		ASSERT_ONLY(RESOURCE_CACHE_ONLY(grcResourceCache::GetInstance().SetSafeResourceCreate(false);))
	}

	GRCDEVICE.GetGPUCount(true,true);
}
#endif

#if __WIN32PC
int grcDevice::InitSingleton()
{
#if !NV_SUPPORT || !ATI_SUPPORT
	s_DepthBoundsTestSupported = false;
	sm_UseVendorAPI = false;
#endif

	if(PARAM_noVendorAPI.Get())
	{
		sm_UseVendorAPI				= false;
		s_DepthBoundsTestSupported	= false;
	}

#if __BANK
	/* The ID3DUserDefinedAnnotation interface is published by Microsoft Direct3D 11 device contexts. Therefore, 
		ID3DUserDefinedAnnotation has the same threading rules as the ID3D11DeviceContext interface, or any other 
		context interface. For more information about Direct3D threading, see MultiThreading. To retrieve the 
		ID3DUserDefinedAnnotation interface for the context, call the QueryInterface method for the context 
		(for example, ID3D11DeviceContext::QueryInterface). In this call, you must pass the identifier of 
		ID3DUserDefinedAnnotation. -- NOTE this is a part of DX 11.1, not DX 11, so it's not in the June 2010 DXSDK */
	// NOTE:- D3DPERF_BeginEvent()/D3DPERF_EndEvent() only exist in D3D9.dll.
	if (!g_hD3D9)
		g_hD3D9 = ::LoadLibrary("D3D9.DLL");

	if (g_hD3D9)
	{
		g_D3DPERF_BeginEvent = (int (WINAPI *)( D3DCOLOR col, LPCWSTR wszName )) ::GetProcAddress(g_hD3D9,"D3DPERF_BeginEvent");
		g_D3DPERF_EndEvent = (int (WINAPI *)( void ))::GetProcAddress(g_hD3D9,"D3DPERF_EndEvent");
		g_D3DPERF_GetStatus = (int (WINAPI *)( void ))::GetProcAddress(g_hD3D9,"D3DPERF_GetStatus");
	}
#endif //__BANK

	if (!g_hD3D11)
	{
		{
			g_hD3D11 = ::LoadLibrary("D3D11.DLL");
			if (g_hD3D11 && !g_D3D11CreateDevice)
			{
				OutputDebugString("DirectX11 DLL's found.\n");
				g_D3D11CreateDevice = (HRESULT (WINAPI *)(
					IDXGIAdapter* pAdapter,D3D_DRIVER_TYPE DriverType,HMODULE Software,
					UINT Flags,CONST D3D_FEATURE_LEVEL* pFeatureLevels,UINT FeatureLevels,
					UINT SDKVersion,ID3D11Device** ppDevice,D3D_FEATURE_LEVEL* pFeatureLevel,
					ID3D11DeviceContext** ppImmediateContext ))::GetProcAddress(g_hD3D11,"D3D11CreateDevice");

				g_D3D11CreateDeviceAndSwapChain = (HRESULT (WINAPI *)(
					IDXGIAdapter* pAdapter,D3D_DRIVER_TYPE DriverType,HMODULE Software,
					UINT Flags,CONST D3D_FEATURE_LEVEL* pFeatureLevels,UINT FeatureLevels,
					UINT SDKVersion,CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
					IDXGISwapChain** ppSwapChain,ID3D11Device** ppDevice,D3D_FEATURE_LEVEL* pFeatureLevel,
					ID3D11DeviceContext** ppImmediateContext ))::GetProcAddress(g_hD3D11,"D3D11CreateDeviceAndSwapChain");
			}
		}
	}
	if (g_D3D11CreateDevice)
	{
		static grcDevice theDevice11;
		g_pGRCDEVICE = &theDevice11;
		grcAdapterManager::InitClass(DXGI_FORMAT_R8G8B8A8_UNORM);
		return 1100;
	}
	else
		return 0;
}
#endif		// __WIN32PC


#if __WIN32PC
// Disallow any usage until initialized
int grcDevice::sm_AdapterOrdinal = -1;

void grcDevice::SetAdapterOrdinal(int ordinal) 
{
	sm_AdapterOrdinal = ordinal;
}

int grcDevice::GetAdapterOrdinal() 
{ 
	InitAdapterOrdinal();
	return sm_AdapterOrdinal; 
}

int grcDevice::sm_OutputMonitor;

void grcDevice::SetOutputMonitor(int monitor)
{
	sm_OutputMonitor = monitor;
}

int grcDevice::GetOutputMonitor()
{
	InitAdapterOrdinal();
	return sm_OutputMonitor;
}

void grcDevice::SetAspectRatio(float fAspectRatio)
{
	sm_MonitorConfig.setGlobalAspect(fAspectRatio);

}
#endif


void grcDevice::ClearCachedState()
{
	memset(s_ResourceViews, NULL, sizeof(s_ResourceViews));
	g_grcCurrentContext->PSSetShaderResources(0, EFFECT_TEXTURE_COUNT, &(s_ResourceViews[0]));	

#if RSG_PC
	s_ShaderStagesActive = 0;
	s_ShaderStagesLastActive = 0;
#endif // RSG_PC

	grcEffect::ClearCachedState();
}

u32 grcDevice::SetVertexDeclaration(const grcVertexDeclaration *pDecl)
{ 
	sm_CurrentVertexDeclaration = const_cast<grcVertexDeclaration *>(pDecl); 
	return 0; 
}

u32 grcDevice::GetVertexDeclaration(grcVertexDeclaration **ppDecl)
{ 
	*ppDecl = sm_CurrentVertexDeclaration; 
	return 0; 
}

#if RSG_DURANGO && GRCDBG_IMPLEMENTED
void grcDevice::PushFaultContext(const char *label)
{
(void)label;
// 	grcGfxContext::PushFaultContext(label);
}

void grcDevice::PopFaultContext()
{
// 	grcGfxContext::PopFaultContext();
}
#endif

grcEffect& grcDevice::GetDefaultEffect()
{
	return *g_DefaultEffect;
}

void grcDevice::SetDefaultEffect(bool isLit,bool isSkinned) {
	grcEffectTechnique tech = isLit? (isSkinned? s_DefaultLitSkinned : s_DefaultLit) : (isSkinned? s_DefaultUnlitSkinned : s_DefaultUnlit);
	g_DefaultEffect->Bind(tech);
}

bool grcDevice::CaptureScreenShotToFile(const char* outName,float gamma)
{
#if __WIN32PC || RSG_DURANGO
	bool result = false;
	grcImage *image = CaptureScreenShot(NULL);
	if (image) {
		result = image->SavePNG(outName,gamma);
		image->Release();
	}
	return result;
#else
	(void)outName; (void)gamma;
	return false;
#endif
}

int g_JpegSaveQuality = 75;

bool grcDevice::CaptureScreenShotToJpegFile(const char* outName)
{
// Copied from device_d3d.cpp
#if __WIN32PC || RSG_DURANGO
	bool result = false;
	PIXBegin(0, "Capturing Screenshot for JPEG");
	grcImage *image = CaptureScreenShot(NULL);
	PIXEnd();
	if (image) {
		result = image->SaveJPEG(outName,g_JpegSaveQuality);
		image->Release();
	}
	return result;
#else
	(void)outName;
	return false;
#endif
}

void grcDevice::GetSafeZone(int &x0, int &y0, int &x1, int &y1) 
{
	x0 = 0;
	y0 = 0;
	x1 = sm_CurrentWindows[g_RenderThreadIndex].uWidth - 1;
	y1 = sm_CurrentWindows[g_RenderThreadIndex].uHeight - 1;
}

void grcDevice::DeleteTexture(grcDeviceTexture * &pTexture)
{
	if (!pTexture)
		return;
	SAFE_RELEASE_RESOURCE(*&pTexture);
	pTexture = NULL;
}

bool grcDevice::GetWideScreen()
{
	// Verify thread ownership. [see GetWidth and GetHeight]
	if(!GRCDEVICE.GetHeight())
		return false;

	const float WIDESCREEN_ASPECT = 1.5f;
	float fWidth = (float)GetWidth();
	float fHeight = (float)GetHeight();

#if SUPPORT_MULTI_MONITOR
	if(GetMonitorConfig().isMultihead())
	{
		const GridMonitor &mon = GetMonitorConfig().getLandscapeMonitor();
		fWidth = (float)mon.getWidth();
		fHeight = (float)mon.getHeight();
	}
#endif // SUPPORT_MULTI_MONITOR

	float fAspectRatio = fWidth / fHeight;

	return fAspectRatio > WIDESCREEN_ASPECT;
}

bool grcDevice::GetHiDef()
{
	return GetWidth() * GetHeight() >= (1280 * 720);
}

void grcDevice::SetFrameLock(int frameLock, bool swapImmediate)
{
	(void)swapImmediate;
	PARAM_frameLimit.Get(frameLock);

	Assert(frameLock <= 4);	// Could support more easily enough (presentation interval is a bitmask, not an enum)
	sm_CurrentWindows[g_RenderThreadIndex].uFrameLock = frameLock;
}

void grcDevice::SetLetterBox(bool)
{
}

int grcDevice::GetFrameLock() {
	return sm_CurrentWindows[g_RenderThreadIndex].uFrameLock;
}


bool grcDevice::IsMessagePumpThreadThatCreatedTheD3DDevice()
{
	return (sm_CreationOwner == sysIpcGetCurrentThreadId());
}

// Returns true if the current thread is the same thread the device was created on.
bool grcDevice::IsCurrentThreadTheDeviceOwnerThread()
{
	// return (sm_CreationOwner == sysIpcGetCurrentThreadId());
	return g_grcCurrentContext != 0;
}


#if RSG_PC
DeviceManufacturer grcDevice::GetManufacturer(int adapterIndex)
{
	static DeviceManufacturer manufacturer = UNKNOWN_DEVICE;

	if (manufacturer == UNKNOWN_DEVICE)
	{
		if (adapterIndex == -1)
		{
			adapterIndex = GetAdapterOrdinal();
		}
		const grcAdapterManager* adapterManagerD3D11 = grcAdapterManager::GetInstance();
		Assert(adapterManagerD3D11);
		if (adapterManagerD3D11)
		{
			const grcAdapterD3D11* adapterD3D11 = (const grcAdapterD3D11*)adapterManagerD3D11->GetAdapter(adapterIndex);
			Assert(adapterD3D11);
			if (adapterD3D11)
			{
				manufacturer = adapterD3D11->GetManufacturer();
			}
		}
	}
	return manufacturer;
}
#endif

u32 grcDevice::GetAndIncContextSequenceNumber()
{
	static u32 s_contextSequenceNumber;
	return ++s_contextSequenceNumber;
}

void grcDevice::CreateContextForThread()
{
#if RSG_DURANGO
	grcGfxContext::perThreadInit();
#else

	Assert(!g_grcCurrentContext);

	HRESULT hCreateDeferredContext = S_OK;

	UINT createFlags = 0;
#if !__FINAL && __D3D11_1
	if (PARAM_longLifeCommandLists.Get())
		createFlags |= D3D11_CREATE_DEFERRED_CONTEXT_LONG_LIFE_COMMAND_LISTS;
#endif // !__FINAL && __D3D11_1

#if DEVICE_USE_D3D_WRAPPER
	char *zero = rage_new char[sizeof(RageDirect3DDeviceContext11)];
	memset(zero,0,sizeof(RageDirect3DDeviceContext11));
	RageDirect3DDeviceContext11 *wrapper = rage_placement_new(zero) RageDirect3DDeviceContext11;

	g_grcCurrentContext = wrapper;

#if __D3D11_1
	hCreateDeferredContext = sm_Current->CreateDeferredContext1(createFlags,&wrapper->m_Inner);
#else
	hCreateDeferredContext = sm_Current->CreateDeferredContext(createFlags,&wrapper->m_Inner);
#endif // __D3D11_1
#else	// DEVICE_USE_D3D_WRAPPER
	ID3D11DeviceContext *tmp;
	hCreateDeferredContext = sm_Current->CreateDeferredContext(createFlags,&tmp);
#endif	// DEVICE_USE_D3D_WRAPPER

	if (FAILED(hCreateDeferredContext))
	{
		if (hCreateDeferredContext == E_OUTOFMEMORY)
		{
			Quitf(ERR_GFX_D3D_DEFERRED_MEM,"Unable to create thread context");
		}
		else
		{
			rage::diagErrorCodes::SetExtraReturnCodeNumber(hCreateDeferredContext);
			Quitf(ERR_GFX_D3D_DEFERRED,"Unable to create thread context");
		}
	}

	D3D11_BUFFER_DESC oDesc;

	oDesc.ByteWidth = s_BeginVerticesBufferSize;
	oDesc.Usage = D3D11_USAGE_DYNAMIC;
	oDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	oDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	oDesc.MiscFlags = 0;
	oDesc.StructureByteStride = 0;

	CHECK_HRESULT(sm_Current->CreateBuffer(&oDesc, NULL, &s_BeginVertexBuffer));

	oDesc.ByteWidth = s_BeginIndicesBufferSize;
	oDesc.Usage = D3D11_USAGE_DYNAMIC;
	oDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	oDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	oDesc.MiscFlags = 0;
	oDesc.StructureByteStride = 0;

	CHECK_HRESULT(sm_Current->CreateBuffer(&oDesc, NULL, &s_BeginIndexBuffer));

#endif
}


void grcDevice::DestroyContextForThread()
{
#if !RSG_DURANGO
	SAFE_RELEASE_RESOURCE(s_BeginVertexBuffer);
	SAFE_RELEASE_RESOURCE(s_BeginIndexBuffer);
#endif

#if DEVICE_USE_D3D_WRAPPER
	Assert(g_grcCurrentContext && g_grcCurrentContext != &s_DeviceContext11Wrapper);
#else
	Assert(g_grcCurrentContext && g_grcCurrentContext != g_pd3dDeviceContext);
#endif // DEVICE_USE_D3D_WRAPPER

	g_grcCurrentContext->Release();
	g_grcCurrentContext = NULL;
}

void grcDevice::BeginContext(GRCGFXCONTEXT_CONTROL_SUPPORTED_ONLY(grcGfxContextControl *control))
{
	ResetContext();

#if RSG_DURANGO
	grcGfxContext::openContext(true, control->getSequenceNumber(), control);
#endif
}

void grcDevice::ResetContext()
{
#if DEVICE_USE_D3D_WRAPPER && __D3D11_USE_STATE_CACHING
 	static_cast <RageDirect3DDeviceContext11 *>(g_grcCurrentContext)->ResetCachedStates();
#endif

#if RSG_PC && MULTIPLE_RENDER_THREADS > 0
	grcCBuffer::OnBeginDeferredContext();
#endif

	memset(s_ResourceViews, NULL, sizeof(s_ResourceViews));

#if MULTIPLE_RENDER_THREADS && !RSG_DURANGO
	// Reset immediate mode vertex/index buffers.
	BeginVertices_OnBeginContext();
#endif

	// Reset the clip planes.
	ResetClipPlanes();

#if RSG_PC
	// Reset any constant buffer over-rides.
	SetVertexShaderConstantBufferOverrides(0, NULL, 0);
#endif
}

grcContextCommandList *grcDevice::EndContext()
{
#if RSG_PC && MULTIPLE_RENDER_THREADS > 0
	grcCBuffer::OnEndDeferredContext();
#endif

#if RSG_DURANGO
	return grcGfxContext::closeContext();
#else
	grcContextCommandList *list = NULL;
	g_grcCurrentContext->FinishCommandList(false, &list);
	Assert(list);
	return list;
#endif
}

void grcDevice::ExecuteCommandList(grcContextCommandList* list)
{
#if RSG_DURANGO

	const bool isPartialSubmit = false;
	list->kick(isPartialSubmit);

#else

	bool restoreContextState = PARAM_restoreContextState.Get();

#if DEVICE_USE_D3D_WRAPPER
	Assert(g_grcCurrentContext && g_grcCurrentContext == &s_DeviceContext11Wrapper);
#else
	Assert(g_grcCurrentContext && g_grcCurrentContext == g_pd3dDeviceContext);
#endif // DEVICE_USE_D3D_WRAPPER
	if (AssertVerify(list)) {
		g_grcCurrentContext->ExecuteCommandList(list, restoreContextState);
		list->Release();
	}
#endif
}

void grcDevice::BeginCommandList()
{
}

void grcDevice::EndCommandList()
{
}

#if RSG_DURANGO
void grcDevice::ForceSetContextState()
{
	grcContextHandle *const ctx = g_grcCurrentContext;
#if ENABLE_PIX_TAGGING
	ctx->PIXSetMarker(L"ForceSetContextState");
#endif // ENABLE_PIX_TAGGING

	grcEffect::ForceSetContextShaders(ctx);

	ctx->RSSetViewports(sm_numViewports, sm_viewports);
	ctx->RSSetScissorRects(sm_numScissorRects, sm_scissorRects);

	grcStateBlock::MakeDirty();
	grcStateBlock::Flush();

	ID3D11RenderTargetView *pRenderTargets[grcmrtColorCount];
	for (unsigned i=0; i<sm_numTargets; ++i)
	{
		pRenderTargets[i] = sm_aRTView[i].pRTView;
	}
	g_grcCurrentContext->OMSetRenderTargets(sm_numTargets, pRenderTargets, (ID3D11DepthStencilView*)sm_DepthView.pRTView);
}
#endif

#if DEPTH_BOUNDS_SUPPORT
void grcDevice::SetDepthBoundsTestEnable(u32 enable)
{
	if( !s_DepthBoundsTestSupported )
		return;

	s_DepthBoundsTestEnabled = enable;

#if RSG_DURANGO
	//This is set from the stateblock
#else
	switch (GetManufacturer())
	{
#if NV_SUPPORT
	case NVIDIA:
		{			
			NvAPI_D3D11_SetDepthBoundsTest(GetCurrent(), enable, s_DepthBoundsTestMin, s_DepthBoundsTestMax);
			break;
		}
#endif

#if ATI_SUPPORT
	case ATI:
		{
			g_pDepthBoundsTest->SetDepthBounds(enable, s_DepthBoundsTestMin, s_DepthBoundsTestMax);
			break;
		}
#endif

	case INTEL:
		{
			//TODO if available
			break;
		}
	default:
		{
			Assert(0);
		}
	}
#endif //RSG_DURANGO

}


void grcDevice::SetDepthBounds(float zmin, float zmax)
{
	if(!s_DepthBoundsTestSupported)
		return;

	float zzmin = Clamp<float>(FixDepth(zmin), 0.0f, 1.0f);
	float zzmax = Clamp<float>(FixDepth(zmax), 0.0f, 1.0f);

	s_DepthBoundsTestMin = Min(zzmin,zzmax);
	s_DepthBoundsTestMax = Max(zzmin,zzmax);

	Assertf((zmin < zmax), "SetDepthBounds Min %f Must Be less than Max %f", zmin, zmax);

#if RSG_DURANGO
	g_grcCurrentContext->OMSetDepthBounds(s_DepthBoundsTestMin, s_DepthBoundsTestMax);
#else

	switch (GetManufacturer())
	{
#if NV_SUPPORT
	case NVIDIA:
		{
			//if(CSystem::ms_UseNVidiaAPI)
			{
				ASSERT_ONLY(_NvAPI_Status retVal =) NvAPI_D3D11_SetDepthBoundsTest(GetCurrent(), s_DepthBoundsTestEnabled, s_DepthBoundsTestMin, s_DepthBoundsTestMax);
				Assert(retVal == NVAPI_OK);
			}
			break;
		}
#endif

#if ATI_SUPPORT
	case ATI:
		{
			g_pDepthBoundsTest->SetDepthBounds(s_DepthBoundsTestEnabled, s_DepthBoundsTestMin, s_DepthBoundsTestMax);
			break;
		}
#endif

	case INTEL:
		{
			//TODO if available
			break;
		}
	default:
		{
			Assert(0);
		}
	}
#endif //RSG_DURANGO
}
#endif //DEPTH_BOUNDS_SUPPORT

#if ATI_EXTENSIONS
void grcDevice::OpenAMDExtensionInterfaces(ID3D11Device *pD3DDevice)
{
	PFNAmdDxExtCreate11 AmdDxExtCreate;
	HMODULE hDLL;
	HRESULT hr = S_OK;

#if __64BIT
	hDLL = ::GetModuleHandle("atidxx64.dll");
#else
	hDLL = ::GetModuleHandle("atidxx32.dll");
#endif

	g_pDepthBoundsTest = NULL;

	// Find the DLL entry point
	AmdDxExtCreate = reinterpret_cast<PFNAmdDxExtCreate11>(GetProcAddress(hDLL, "AmdDxExtCreate11"));
	if (AmdDxExtCreate == NULL)
	{
#if DEPTH_BOUNDS_SUPPORT
		s_DepthBoundsTestSupported = false;
#endif //DEPTH_BOUNDS_SUPPORT
		return;
	}

	// Create the extension object
	hr = AmdDxExtCreate(pD3DDevice, &g_pExtension);

#if DEPTH_BOUNDS_SUPPORT
	// Get the Extension Interfaces
	if (SUCCEEDED(hr))
	{
		g_pDepthBoundsTest = static_cast<IAmdDxExtDepthBounds*>(g_pExtension->GetExtInterface(AmdDxExtDepthBoundsID));
	}

	//Depth bounds is only supported on Radeon HD 7000 or higher.
	s_DepthBoundsTestSupported = g_pDepthBoundsTest != NULL;

	if(PARAM_noVendorAPI.Get())
	{
		s_DepthBoundsTestSupported = false;
	}
#endif //DEPTH_BOUNDS_SUPPORT

#if UAV_OVERLAP_SUPPORT
	if (SUCCEEDED(hr))
	{
		g_pUAVOverlap = static_cast<IAmdDxExtUAVOverlap*>(g_pExtension->GetExtInterface(AmdDxExtUAVOverlapID));
	}
#endif // UAV_OVERLAP_SUPPORT
}

void grcDevice::CloseAMDExtensionInterfaces()
{
	if(g_pDepthBoundsTest != NULL)
	{
		g_pDepthBoundsTest->Release();
		g_pDepthBoundsTest = NULL;
	}

	if (g_pExtension != NULL)
	{
		g_pExtension->Release();
		g_pExtension = NULL;
	}
}
#endif //ATI_EXTENSIONS

#if RSG_PC
bool grcDevice::sm_bUAVSync = true;

void grcDevice::SetUAVSync(bool bEnable)
{
	sm_bUAVSync = bEnable;

#if UAV_OVERLAP_SUPPORT
	if (g_pUAVOverlap)
	{
		if (bEnable)
		{
			g_pUAVOverlap->EndUAVOverlap();
		}
		else
		{
			g_pUAVOverlap->BeginUAVOverlap();
		}
	}
#endif // UAV_OVERLAP_SUPPORT
}
#endif

#if DRAWABLE_STATS
void grcDevice::SetFetchStatsBuffer(drawableStats* dstPtr) 
{
	g_pCurrentStatsBucket = dstPtr;
}
#endif


} // namespace rage

#endif	// __D3D11

