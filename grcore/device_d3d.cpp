//
// grcore/device_d3d.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//
// #define D3D_DISABLE_9EX	// For testing against older DXSDK's.

#include "device.h"

#include "effect.h"
#include "fastquad.h"
#include "fencepool.h"
#include "im.h"
#include "image.h"
#include "texture.h"
#include "viewport.h"
#include "channel.h"
#include "stateblock.h"
#include "quads.h"

#include "atl/array.h"
#include "atl/freelist.h"
#include "bank/bkmgr.h"
#include "bank/bank.h"
#include "file/device.h"
#include "file/asset.h"
#include "shaderlib/rage_constants.h"
#include "string/string.h"
#include "system/cache.h"
#include "system/externalallocator.h"
#include "system/ipc.h"
#include "system/memory.h"
#include "system/param.h"
#include "system/timer.h"
#include "system/wndproc.h"
#include "system/xtl.h"
#if __XENON
#include "grcore/effect_internal.h"
#endif // __XENON
#include "grcore/indexbuffer.h"
#include "grcore/vertexbuffer.h"
#include "grcore/channel.h"

#include "wrapper_d3d.h"
#include "resourcecache.h"

#if __WIN32PC
#include "grcore/vertexbuffer.h"
#include "grcore/vertexbuffereditor.h"
#include "grcore/quads.h"
#include "grcore/texture.h"
#include "grmodel/shaderfx.h"

#include <dxdiag.h>
#include <strsafe.h>
#include "texture.h"
#include "texturepc.h"
#include "texture_d3d9.h"
#endif

#define STALL_DETECTION	0
#if STALL_DETECTION
#define STALL_ONLY_RENDERTHREAD(x) x
#define STALL_TIME 0.05f
#include "system/timer.h"
#endif
#define STATISTICS 0

#define SURFACE IDirect3DSurface9
#define TEXTURE IDirect3DBaseTexture9
#if __XENON
#define	VERTEXBUFFER d3dVertexBuffer
#define QUERY		 D3DQuery
#elif __WIN32PC
#define	VERTEXBUFFER IDirect3DVertexBuffer9
#define QUERY		 IDirect3DQuery9
#endif

// common code
rage::u32 rage::grcDevice::sm_FrameCounter = 1;
rage::u32 rage::grcDevice::sm_SyncCounter = 1;

#if !__FINAL
DECLARE_MTR_THREAD const void *rage::grcDevice::sm_BeginVerticesBufferEnd = NULL;
DECLARE_MTR_THREAD const void *rage::grcDevice::sm_BeginIndicesBufferEnd = NULL;
#endif // !__FINAL

namespace rage {
#if SUPPORT_INVERTED_PROJECTION
DECLARE_MTR_THREAD bool g_grcDepthInversionInvertProjection = false; 
#endif // SUPPORT_INVERTED_PROJECTION
} // namespace rage 

#if __WIN32PC
rage::u32 rage::grcDevice::sm_TextureResourcePool = RESOURCE_UNMANAGED;
rage::u32 rage::grcDevice::sm_BufferResourcePool = RESOURCE_MANAGED;
grcSwapChain* rage::grcDevice::sm_pSwapChain = NULL;
rage::u32 rage::grcDevice::sm_uFeatures = 0;
rage::FOCUSCALLBACK rage::grcDevice::sm_FocusCallback = NULL;
rage::RENDERCALLBACK rage::grcDevice::sm_RenderCallback = NULL;

rage::u32 rage::grcDevice::sm_DxFeatureLevel = 0;
rage::u32  rage::grcDevice::sm_DxShaderModel[2] = {0,0};
volatile rage::u32 rage::grcDevice::sm_KillSwitch = 0;

rage::u32 rage::grcDevice::ms_MaxQueuedFrames = RSG_XENON ? 2 : 1; // PC must override based on available GPUs
#elif RSG_DURANGO
grcSwapChain* rage::grcDevice::sm_pSwapChain = NULL;
#endif // __WIN32PC

#if __XENON
static const rage::u32 kMaxConditionalQueries = 64;
static rage::atFreeList<rage::u32,kMaxConditionalQueries> s_ConditionalQueryFreeList;
ASSERT_ONLY(rage::u32 s_ConditionalQueryInProgress = 0xFFFFFFFF);
#endif // __XENON

#if __WIN32PC
void (*rage::grcDevice::sm_ResetCallback)(grcDeviceHandle *,grcPresentParameters *);
void (*rage::grcDevice::sm_PresentCallback)(grcDeviceHandle *current,const tagRECT*,const tagRECT*,HWND override,const tagRGNDATA*);
void (*rage::grcDevice::sm_PresentHandler)(int syncInterval, u32 uFlags);
void (*rage::grcDevice::sm_PostPresentCallback)();
#endif

#if __D3D9

#include "system/d3d9.h"
#include "grcore/d3dwrapper.h"

#if !__RESOURCECOMPILER
#if __XENON
#include "grcore/texturexenon.h"

// Enable PIX GPU captures at the cost of a slower render thread
#define ENABLE_GPU_CAPTURES (__PROFILE || __STATS) 

# if USE_D3D_DEBUG_LIBS || defined(_DEBUG)
# pragma comment(lib,"d3d9d.lib")
# elif ENABLE_GPU_CAPTURES
# pragma comment(lib,"d3d9i.lib")
# elif !__DEV
# pragma comment(lib,"d3d9.lib")
// # pragma comment(lib,"d3d9ltcg.lib")
# else
# pragma comment(lib,"d3d9i.lib")
# endif

#if 0
#if !__OPTIMIZED && !HACK_GTA4
#pragma comment(lib,"d3dx9d.lib")
#else
#pragma comment(lib,"d3dx9.lib")
#endif
#endif

#endif // __XENON
#endif // !__RESOURCECOMPILERS

#if __XENON
# if __DEV && !__OPTIMIZED && !HACK_GTA4
# define DBG 1
# include <xgraphics.h>
# undef DBG
# pragma comment(lib,"xgraphicsd.lib")
# else
# define DBG 0
# include <xgraphics.h>
# undef DBG
# pragma comment(lib,"xgraphics.lib")
# endif
// If you get a redefined symbol error here, your XDK install doesn't
// match what offline resourcing is configured for.
#include "xenon_sdk.h"
#endif

#if __WIN32PC
struct grcCommandBuffer {
	grcCommandBuffer(size_t size) { m_List = rage_new DWORD[m_Size = (size/sizeof(DWORD))]; m_Used = 0; }
	grcCommandBuffer(grcCommandBuffer *that) {
		m_List = rage_new DWORD[m_Size = m_Used = that->m_Used];
		memcpy(m_List, that->m_List, m_Size * sizeof(DWORD));
	}
	~grcCommandBuffer() { delete[] m_List; }
	DWORD *m_List;
	size_t m_Used, m_Size;
};
#endif


// Currently active presentation parameters. 
grcPresentParameters s_d3dpp;

template <class T> void CheckRefCount(const char *tag,T *ref) {
	ref->AddRef();
	int newCount = ref->Release();
	grcDisplayf("ref of '%s' is %d",tag,newCount);
}
PARAM(pixannotation,"[grcore] Enable Pix annotation via the PIXBegin()/PIXEnd() macros");
unsigned int g_EnablePixAnnotation = 0xffffffff;

#if __WIN32PC
PARAM(ati,"[device_d3d] Sets ATI specific things");
NOSTRIP_XPARAM(fullscreen);
NOSTRIP_PARAM(dx10,  "[device_d3d] Force DirectX 10, if available");
NOSTRIP_PARAM(dx10_1,"[device_d3d] Force DirectX 10.1, if available");
PARAM(dx9,   "[device_d3d] Force DirectX 9");
PARAM(autodepthstencil, "[device_d3d] Automatically create depth/stencil buffers");
PARAM(ragUseOwnWindow, "[device_d3d] Displays the game in its own window rather than inside rag");
#endif // __WIN32PC

namespace rage {

__THREAD grcContextHandle *g_grcCurrentContext;

XPARAM(srgb);

#if __XENON
	D3DFORMAT g_grcDepthFormat = D3DFMT_D24S8;
#elif __WIN32PC
	unsigned int g_grcDepthFormat = rage::grctfD24S8;
#endif // __WIN32PC

#if DRAWABLE_STATS
drawableStats* g_pCurrentStatsBucket = NULL;
#endif

int grcVertexDeclaration::Release()
{
	int refCount = D3dDecl->Release();
	if (refCount == 0)
	{
		rage::sysMemStartTemp();
		delete this; 
		rage::sysMemEndTemp();
	}
	return refCount;
}

void grcVertexDeclaration::AddRef()
{
	D3dDecl->AddRef();
}


u32 g_GLOBALS_SIZE = GLOBALS_SIZE, g_SKINNING_BASE = SKINNING_BASE;

void grcDevice::ConfigureGlobalConstants(int globalSize,int skinningBase) 
{
	Assert(globalSize >= 16 && skinningBase >= globalSize);
	g_GLOBALS_SIZE = (u32) globalSize;
	g_SKINNING_BASE = (u32) skinningBase;
}

NOSTRIP_PARAM(hdr,"[device_d3d] Set the whole rendering pipeline to 10-bit on 360 and 16-bit on PC");

PARAM(MSAA,"[GRAPHICS] Anti-aliasing (MSAA_NONE, MSAA_2, MSAA_4, MSAA_8)");
PARAM(availablevidmem, "[MEMORY] Percentage of available video memory");

grcDevice::MSAAMode grcDevice::sm_MSAA;
const char *grcDevice::sm_DefaultEffectName = "x:\\rage\\assets\\tune\\shaders\\lib\\rage_im";
grcCommandBuffer *g_grcCommandBuffer;

#if !GRCDEVICE_IS_STATIC
grcDevice GRCDEVICE;
#endif

#if __WIN32PC
grcDisplayWindow grcDevice::sm_CurrentWindows[NUMBER_OF_RENDER_THREADS];
int grcDevice::sm_AdapterOrdinal = 0;
int grcDevice::sm_OutputMonitor = 0;
extern WINDOWPLACEMENT s_WindowedPlacement;

grcDisplayWindow grcDevice::sm_FullscreenWindow = grcDisplayWindow(1280, 720, 0, 1, 1);
grcDisplayWindow grcDevice::sm_DesiredWindow = grcDisplayWindow(1280, 720, 0, 1, 1);
#else
// One means autodetect from console settings.
grcDisplayWindow grcDevice::sm_CurrentWindows[NUMBER_OF_RENDER_THREADS];
#endif
grcDisplayWindow grcDevice::sm_GlobalWindow = grcDisplayWindow(1280, 720, 0, 0, 1);

MonitorConfiguration grcDevice::sm_MonitorConfig;

static bool s_SwapImmediateIfLate = false;
bool grcDevice::sm_HardwareShaders = __XENON;
bool grcDevice::sm_HardwareTransform = true;
bool grcDevice::sm_LetterBox=true; 


grcDeviceHandle *grcDevice::sm_Current;
#if __XENON
PARAM(colorexpbias,"Color exponent bias");
int grcDevice::sm_ColorExpBias = 0;
grcDeviceHandle *grcDevice::sm_Primary;
grcDeviceHandle *grcDevice::sm_Command;
#endif
grcVertexDeclaration *grcDevice::sm_BlitDecl;
grcVertexDeclaration *grcDevice::sm_ImDecl;

// Default pixel shader (modulates texture and color)
grcPixelShader *s_DefaultUnfoggedPixelShader;
grcPixelShader *s_DefaultFoggedPixelShader;

// Default vertex shader for unlit, unskinned geometry
//grcVertexShader *s_UnlitUnskinnedShader;
//grcVertexShader *s_LitUnskinnedShader;
//grcVertexShader *s_LitSkinnedShader;

static const grcVertexBuffer *grcCurrentVertexBuffers[grcVertexDeclaration::c_MaxStreams];
static const grcVertexDeclaration *grcCurrentVertexDeclaration;
#if __WIN32PC
static u32 grcCurrentDividers[grcVertexDeclaration::c_MaxStreams];
#endif

u32 grcDevice::sm_ClipPlaneEnable = 0;
#if !HACK_GTA4 // no shader clip planes
static Vec4V s_ClipPlanes[RAGE_MAX_CLIPPLANES];
#endif // !HACK_GTA4

#if __WIN32PC
extern WINDOWINFO g_WindowInfo;
#endif

#if __XENON  // predicated tiling tile render target for BeginTiledRendering
#define MAX_TILE_SIZES 8 // number of different Predicated tile sizes supported (not all passes need to use the same resolutions, main game can run at one res, the hud at another, the garage/cutscenes in another, etc.)
static grcRenderTarget * s_TileColorTarget[MAX_TILE_SIZES] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
static grcRenderTarget * s_TileDepthTarget[MAX_TILE_SIZES] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
static int s_TileAATarget[MAX_TILE_SIZES] = {-1,-1,-1,-1,-1,-1,-1,-1};

#define MAX_TILES 16
static grcTileRect s_TileRects[MAX_TILES];  
static int s_TileCount = -1;
static int s_TileFlags;
static grcResolveFlags s_ClearParams;
static grcRenderTarget * s_TileCompositeColorTarget = NULL;
static grcRenderTarget * s_TileCompositeDepthTarget = NULL;

#define TILE_HEIGHT_MULTIPLE 32 // tiles need to be a multiple of 32 high, regardless of the AA resolutions (due to edram tile alignment and texture tile alignment)
#define TILE_WIDTH_MULTIPLE 160 // tiles need to be a multiple of 32 high, regardless of the AA resolutions (due to edram tile alignment and texture tile alignment)

#if !__OPTIMIZED
DWORD s_Fence;
#endif

#endif

// Our global Direct3D interface
static IDirect3D9 *s_D3D9;

// Type of device to create (can also be a reference device)
static _D3DDEVTYPE s_DevType = D3DDEVTYPE_HAL;

// DOM-IGNORE-BEGIN
PARAM(debugshaders,"[grcore] Enable D3D shader debugging");
PARAM(nohwtnl,"[grcore] Disable hardware transform & light");
NOSTRIP_PC_PARAM(frameLimit, "[grcore] number of vertical synchronizations to limit game to");
#if __WIN32PC
PARAM(d3dmt,"[grcore] Allow D3D device to be initialized in multithreaded mode (PC ONLY)");
PARAM(d3dfpu,"[grcore] Prevent D3D from messing with FPU state, leaving useful error conditions trappable (PC ONLY).");
PARAM(framelockinwindow,"[grcore] Force framelock to work even in a window (works best with 60Hz monitor refresh)");
// PARAM(nod3dfpu,"[grcore] Allow D3D to mess with FPU state even in __DEV builds (PC ONLY)");
#if	!__FINAL
PARAM(perfhud,"[grcore] Add support for running under nVidia's PerfHud utility (PC ONLY");
#endif
#else
PARAM(blockuntilidle,"[grcore] Do a BlockUntilIdle after the Swap to avoid unexpected CPU stalls");
PARAM(d3dsinglestep,"[grcore] Set D3D_SingelStepper to true, force a BlockUntilIdle() after every D3D call (debug builds only)");


#endif
// DOM-IGNORE-END

#if __WIN32PC
grcTexture* grcDevice::GetTexture(const grcTextureObject* pTexObj)
{
	return grcTextureDX9::GetPrivateData(pTexObj);
}
#endif // __WIN32PC

#if __XENON
u32 grcDevice::GetD3DDepthFormat()
{
	return g_grcDepthFormat;
}	

void grcDevice::SetD3DDepthFormat(u32 eFormat)
{
	g_grcDepthFormat = (D3DFORMAT)eFormat;
}
#endif // __XENON

int grcDevice::SetfpZBuffer(bool b)
{
	D3DFORMAT OldFormat;

	if(b)
	{
		g_grcDepthFormat = D3DFMT_D24FS8;
		OldFormat = D3DFMT_D24S8;
	}
	else
	{
		g_grcDepthFormat = D3DFMT_D24S8;
		OldFormat = D3DFMT_D24FS8;
	}

	// Reset DSS so that z function is proeprly flipped if necessary
	grcStateBlock::SetDepthStencilState(grcStateBlock::DSS_Active);

	return (int) OldFormat;
}

bool grcDevice::IsItfpZBuffer(int format)
{
	return (format == D3DFMT_D24FS8)?true:false;
}

bool grcDevice::IsCurrentDepthFormatFpZ()
{
	return IsItfpZBuffer(g_grcDepthFormat);
}

void grcDevice::SetWindow(const grcWindow &window) {
	WIN32PC_ONLY(if (sm_Current)) {
		D3DVIEWPORT9 d3dViewport = { window.GetX(), window.GetY(),
			window.GetWidth(), window.GetHeight(),
			FixViewportDepth(window.GetMinZ()), FixViewportDepth(window.GetMaxZ()) };
		CHECK_HRESULT(sm_Current->SetViewport(&d3dViewport));
	}
}

#if __XENON || __WIN32PC
static inline void InvertViewportDepth(grcDeviceHandle *dev) {   
	// Direct3D does an internal viewport update which screws up our 1-z transform.
	if (dev)
	{
		D3DVIEWPORT9 vp;
		dev->GetViewport(&vp);
		vp.MinZ = grcDevice::FixViewportDepth(vp.MinZ);
		vp.MaxZ = grcDevice::FixViewportDepth(vp.MaxZ);

		AssertVerify(dev->SetViewport(&vp) == D3D_OK);
	}
}
#endif

#if __WIN32PC
void grcDevice::SetSoftwareVertexProcessing(bool flag) {
	CHECK_HRESULT(sm_Current->SetSoftwareVertexProcessing(flag));
}

grcDeviceHandle* grcDevice::GetCurrentInner()
{
	return static_cast<RageDirect3DDevice9*>(sm_Current)->m_Inner;
}
#endif


void grcDevice::SetStreamSource(u32 streamNumber,const grcVertexBuffer& pStreamData,u32 offsetInBytes,u32 stride) {
	Assert(streamNumber < grcVertexDeclaration::c_MaxStreams);
	// Note -- this code assumes we never bind the same stream again with a different offsetInBytes or stride
	// If that ever happens, code should call ClearStreamSource first instead of incurring the extra testing here.
	if (grcCurrentVertexBuffers[streamNumber] != &pStreamData) {
		grcCurrentVertexBuffers[streamNumber] = &pStreamData;
		CHECK_HRESULT(sm_Current->SetStreamSource(streamNumber,(VERTEXBUFFER*)((grcVertexBufferD3D&)pStreamData).GetD3DBuffer(),offsetInBytes,stride));
	}
}

void grcDevice::ClearStreamSource(u32 streamNumber) {
	Assert(streamNumber < grcVertexDeclaration::c_MaxStreams);
	Assert(sm_Current != NULL);
	if (grcCurrentVertexBuffers[streamNumber]) {
		grcCurrentVertexBuffers[streamNumber] = NULL;
		CHECK_HRESULT(sm_Current->SetStreamSource(streamNumber,NULL,0,0));
	}
}


void grcDevice::SetIndices(const grcIndexBuffer& pBuffer) {
	CHECK_HRESULT(sm_Current->SetIndices((INDEXBUFFERDX9*)((grcIndexBufferD3D&)pBuffer).GetD3DBuffer()));
}

void grcDevice::DeleteTexture(grcDeviceTexture * &pTexture) {
	if (!pTexture)
		return;

#if __XENON
#define D3D_TEXTURE_ALLOC_PHYSICAL_NEW (0)
#define D3D_TEXTURE_ALLOC_XMEM (1)
#define D3D_TEXTURE_ALLOC_RTMEMPOOL (2)
#define D3D_TEXTURE_ALLOC_XMEM_FLAGS MAKE_XALLOC_ATTRIBUTES(0, FALSE, TRUE, FALSE, 128, XALLOC_PHYSICAL_ALIGNMENT_4K, XALLOC_MEMPROTECT_WRITECOMBINE, FALSE, XALLOC_MEMTYPE_PHYSICAL)

	if (!(pTexture->Common & D3DCOMMON_D3DCREATED)) {
#if _XDK_VER >= 2638 && defined (_DEBUG)
		if (!(pTexture->Common & D3DCOMMON_DEBUG_COMMMANDBUFFER_USED))
#endif
			pTexture->BlockUntilNotBusy();

		switch (pTexture->Identifier)
		{
		case D3D_TEXTURE_ALLOC_PHYSICAL_NEW: // Allocated by physical_new
		physical_delete((void*)(pTexture->Format.BaseAddress << 12));
		physical_delete((void*)(pTexture->Format.MipAddress << 12));
			break;
		case D3D_TEXTURE_ALLOC_XMEM: // Allocated using XMemAlloc
			XMemFree((void*)(pTexture->Format.BaseAddress << 12), D3D_TEXTURE_ALLOC_XMEM_FLAGS);
			break;
		case D3D_TEXTURE_ALLOC_RTMEMPOOL: // Allocated through a render target memory pool
			pTexture->Format.BaseAddress = 0;
			pTexture->Format.MipAddress = 0;
			break;
		default:
			grcErrorf("Unknown resource identifier");
			break;
		}
		delete pTexture;
	}
	else
#endif
	SAFE_RELEASE_RESOURCE(*(TEXTURE**)&pTexture);
	pTexture = NULL;
}

void grcDevice::DeleteSurface(grcDeviceSurface * &pSurface) {
	SAFE_RELEASE_RESOURCE(*(SURFACE**)&pSurface);
}

#if __WIN32PC
void grcDevice::SetTexture(int stage,const /*grcDeviceTexture*/grcTexture *pTexture) {
	Assert(stage < 16 || (stage >= D3DVERTEXTEXTURESAMPLER0 && stage <= D3DVERTEXTEXTURESAMPLER3));
	grcDeviceTexture *devTexture = pTexture->GetCachedTexturePtr();
#if __WIN32PC && !__FINAL && USE_RESOURCE_CACHE
	Assert((devTexture == NULL) || grcResourceCache::GetInstance().Validate((TEXTURE*)devTexture));
#endif // __WIN32PC
	WIN32PC_ONLY(if (!sm_Current) return);
	AssertMsg(CheckThreadOwnership(),"Thread that doesn't own the GPU trying to set texture.");
	CHECK_HRESULT(sm_Current->SetTexture(stage, (TEXTURE*)devTexture));
}
#endif

void grcDevice::GetTexture(int stage,grcDeviceTexture **ppTexture) {
	CHECK_HRESULT(sm_Current->GetTexture(stage, (IDirect3DBaseTexture9 **) ppTexture));
}

void grcDevice::GetDepthStencilSurface(grcDeviceSurface **ppSurface) {
	CHECK_HRESULT(sm_Current->GetDepthStencilSurface((SURFACE**)ppSurface));
}

void grcDevice::SetDepthStencilSurface(grcDeviceSurface *pSurface) {
	VERIFY_EDRAM_SURFACE(pSurface);
	CHECK_HRESULT(sm_Current->SetDepthStencilSurface((SURFACE*)pSurface));
#if __XENON
	grcDeviceSurface* pRenderTarget = NULL;
	CHECK_HRESULT(sm_Current->GetRenderTarget(0, &pRenderTarget));

	if (!pRenderTarget)
		InvertViewportDepth(sm_Current);
	else
		pRenderTarget->Release();
#endif
}

void grcDevice::GetRenderTargets(u32, grcDeviceView **, grcDeviceView **) 
{
	AssertMsg(0, "DX10 Only function");
}

void grcDevice::SetRenderTargets(u32, const grcDeviceView **, const grcDeviceView *)
{
	AssertMsg(0, "DX10 Only function");
}

grcDevice::Result grcDevice::CreateTexture(u32 width,u32 height,u32 depth,u32 levels,u32 usage,u32 format,u32 pool,int imageType, grcDeviceTexture **ppTexture
#if __XENON
										   , u32 *poolAllocSize
#endif
										   )								   
{
#if __XENON
	if (sysMemAllocator::GetCurrent().IsBuildingResource()) {
		if ( imageType == grcImage::CUBE ) {
			(*ppTexture) = rage_new D3DCubeTexture;
		}
		else if ( imageType == grcImage::VOLUME ) {
			(*ppTexture) = rage_new D3DVolumeTexture;
		}
		else {
			(*ppTexture) = rage_new D3DTexture;
		}
		UINT uBaseSize, uMipSize;
		if ( imageType == grcImage::CUBE ) {
			Assert("Cube maps must be square" && width == height);
			XGSetCubeTextureHeader(width, levels, 0, (D3DFORMAT)format, D3DPOOL_MANAGED, 
				0, 0 /*XGHEADER_CONTIGUOUS_MIP_OFFSET*/, (D3DCubeTexture *) *ppTexture, &uBaseSize, &uMipSize);
		}
		else if ( imageType == grcImage::VOLUME ) {
			XGSetVolumeTextureHeader(width, height, depth, levels, 0, (D3DFORMAT)format, D3DPOOL_MANAGED, 
				0, 0 /*XGHEADER_CONTIGUOUS_MIP_OFFSET*/, (D3DVolumeTexture *) *ppTexture, &uBaseSize, &uMipSize);
		}
		else {
			XGSetTextureHeader(width, height, levels, 0, (D3DFORMAT)format, D3DPOOL_MANAGED, 
				0, 0 /*XGHEADER_CONTIGUOUS_MIP_OFFSET*/, 0, (D3DTexture *) *ppTexture, &uBaseSize, &uMipSize);
		}
		(*ppTexture)->Common |= D3DCOMMON_CPU_CACHED_MEMORY;
		// Do two separate allocations now so that we don't have our resource chunk size unnecessarily
		// bloated by 33%.  Note that we don't have this luxury on PS3!
		void *base = physical_new(uBaseSize,D3DTEXTURE_ALIGNMENT);
		void *mips = uMipSize? physical_new(uMipSize,D3DTEXTURE_ALIGNMENT) : NULL;
		(*ppTexture)->Format.BaseAddress = ((DWORD)base) >> 12;
		(*ppTexture)->Format.MipAddress = ((DWORD)mips) >> 12;
#if 0	// Enable this code to analyze memory waste due to small textures
		static u32 accum;
		u32 actual = uBaseSize + uMipSize;
		accum += actual - expected;
		grcDisplayf("%ux%u, %u mips, %ubpp expected %uk, got %uk, accum = %uk",width,height,levels,bpp,expected>>10,actual>>10,accum>>10);
		D3DLOCKED_RECT rect;
		for (u32 i=0; i<levels; i++) {
			(*ppTexture)->LockRect(i, &rect, NULL, 0);
			grcDisplayf("  mip %u offset %u",i,(char*)rect.pBits - (char*)storage);
			(*ppTexture)->UnlockRect(i);
		}
#endif // 0
		return S_OK;
	}
	else if (usage==D3DUSAGE_RENDERTARGET && (poolAllocSize || levels != 1)) // poolAllocSize means we're in a pool
	{
			void *base;
			DWORD textureSize;
			switch (imageType)
			{
			case grcImage::CUBE:
				{
					Assert("Cube maps must be square" && width == height);
					(*ppTexture) = rage_new D3DCubeTexture;
					textureSize = XGSetCubeTextureHeaderEx(width, levels, 0, (D3DFORMAT)format, 0, XGHEADEREX_NONPACKED,
						0, XGHEADER_CONTIGUOUS_MIP_OFFSET, (D3DCubeTexture *) *ppTexture, NULL, NULL);
				}
				break;
			case grcImage::VOLUME:
				{
					(*ppTexture) = rage_new D3DVolumeTexture;
					textureSize = XGSetVolumeTextureHeaderEx(width, height, depth, levels, 0, (D3DFORMAT)format, 0, XGHEADEREX_NONPACKED,
						0, XGHEADER_CONTIGUOUS_MIP_OFFSET, (D3DVolumeTexture *) *ppTexture, NULL, NULL);
				}
				break;
			default:
				{
					(*ppTexture) = rage_new D3DTexture;
					textureSize = XGSetTextureHeaderEx(width, height, levels, 0, (D3DFORMAT)format, 0, XGHEADEREX_NONPACKED,
						0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, (D3DTexture *) *ppTexture, NULL, NULL);
				}
				break;
			}

			if (poolAllocSize)	 // memory pool is active
			{
				// Displayf("grcDevice::CreateTexture 0x%08x: baseAddr 0x%08x, mipaddr = 0x%08x",(*ppTexture),(*ppTexture)->Format.BaseAddress,(*ppTexture)->Format.MipAddress);
				*poolAllocSize = textureSize;			
				(*ppTexture)->Identifier = D3D_TEXTURE_ALLOC_RTMEMPOOL;
				return S_OK;
			}
			else
			{
				base = XMemAlloc(textureSize, D3D_TEXTURE_ALLOC_XMEM_FLAGS);
				(*ppTexture)->Identifier = D3D_TEXTURE_ALLOC_XMEM;
				XGOffsetResourceAddress(*ppTexture, base);
				// Displayf("grcDevice::CreateTexture Non Pooled 0x%08x: baseAddr 0x%08x, mipaddr = 0x%08x",(*ppTexture),(*ppTexture)->Format.BaseAddress,(*ppTexture)->Format.MipAddress);
				return base ? S_OK : E_OUTOFMEMORY;
			}
		}
		else
#endif // __XENON
		{
			if ( imageType == grcImage::CUBE ) {
				Assert("Cube textures must be square" && width == height);
				return CHECK_HRESULT(sm_Current->CreateCubeTexture(width,levels,usage,(D3DFORMAT)format,(D3DPOOL)pool,(IDirect3DCubeTexture9 **)ppTexture,NULL));
			}
			else if ( imageType == grcImage::VOLUME ) {
				return CHECK_HRESULT(sm_Current->CreateVolumeTexture(width,height,depth,levels,usage,(D3DFORMAT)format,(D3DPOOL)pool,(IDirect3DVolumeTexture9 **)ppTexture,NULL));
			}
			else {
				HRESULT hr = CHECK_HRESULT(sm_Current->CreateTexture(width,height,levels,usage,(D3DFORMAT)format,(D3DPOOL)pool,(IDirect3DTexture9 **)ppTexture,NULL));
				
				// Displayf("grcDevice::CreateTexture TLF 0x%08x: baseAddr 0x%08x, mipaddr = 0x%08x",(*ppTexture),(*ppTexture)->Format.BaseAddress,(*ppTexture)->Format.MipAddress);
#if __XENON
				// Reset memory to a known state to catch tiling bugs!
				if (SUCCEEDED(hr)) {
					u32 ba = (*ppTexture)->Format.BaseAddress;
					u32 ma = (*ppTexture)->Format.MipAddress;
					if (ba) {
						void *bp = ((void*)(ba<<12));
						u32 bs = XPhysicalSize(bp);
						memset(bp,0,bs);
					}
					if (ma) {
						void *mp = ((void*)(ma<<12));
						u32 ms = XPhysicalSize(mp);
						memset(mp,0,ms);
					}
				}
#endif // __XENON
				return hr;
			}
		}
	}

grcDevice::Result grcDevice::CreateRenderTarget(u32 width,u32 height,u32 format,u32 multisample,u32 multisampleQuality,bool lockable,grcDeviceSurface **ppSurface, const grcDeviceSurfaceParameters *XENON_ONLY(params)) 
{
#if __WIN32PC
	HANDLE *p = 0;
#elif __XENON
	const grcDeviceSurfaceParameters *p = params;
#endif	// __WIN32PC
	return CHECK_HRESULT(sm_Current->CreateRenderTarget(width,height,(D3DFORMAT)format,(D3DMULTISAMPLE_TYPE)multisample,multisampleQuality,lockable,(SURFACE**)ppSurface,p));
}

#if __WIN32PC
static IDirect3DQuery9* s_KickEventQuery = NULL;
static const int kMaxFences = 256;
static IDirect3DQuery9* s_FenceEventQueries[kMaxFences];
static rage::atFixedBitSet<kMaxFences> s_FenceQueriesFreeList;
static FencePool s_FencePool(64);
static bool s_bRetailRuntime;
static	IDirect3DQuery9	*s_pResourceManagerQuery;
static	D3DDEVINFO_RESOURCEMANAGER	s_ResourceInfo;

#if __DEV
#if	__BANK
struct	ResourceTypeInfo
{
	char	*m_szResourceName;
	int		m_iNumObjects;
	int		m_iNumBytes;
};

struct	ResourceTypeInfo	s_ResourceData[] =
{
	{ "None", 0, 0 },
	{ "Surface", 0, 0 },
	{ "Volume", 0, 0 },
	{ "Texture", 0, 0 },
	{ "VolumeTexture", 0, 0 },
	{ "CubeTexture", 0, 0 },
	{ "VertexBuffer", 0, 0 },
	{ "IndexBuffer", 0, 0 }
};

static rage::bkBank	*s_pBank;

static	void	AddResourceManagerWidgets	(void)
{
	char	szWidgetName[256];

	s_pBank = &BANKMGR.CreateBank("rage - D3D Resources");

	for	(int iIndex = 1; iIndex <= 7; iIndex++)
	{
		rage::formatf(szWidgetName, sizeof(szWidgetName), "%s Count", s_ResourceData[iIndex].m_szResourceName);

		s_pBank->AddSlider(szWidgetName, &s_ResourceData[iIndex].m_iNumObjects, 0, 1000000, 0);

		rage::formatf(szWidgetName, sizeof(szWidgetName), "%s Bytes", s_ResourceData[iIndex].m_szResourceName);

		s_pBank->AddSlider(szWidgetName, &s_ResourceData[iIndex].m_iNumBytes, 0, 1024 * 1024 * 1024, 0);
	}
}
#endif // __BANK

static	void	UpdateResourceManagerQuery	(void)
{
	if	(s_bRetailRuntime)
		return;

	if	(SUCCEEDED(s_pResourceManagerQuery->GetData(&s_ResourceInfo, sizeof(s_ResourceInfo), 0)))
	{
		for	(int iIndex = 1; iIndex <= 7; iIndex++)
		{
			s_ResourceData[iIndex].m_iNumObjects = s_ResourceInfo.stats[iIndex].TotalManaged;
			s_ResourceData[iIndex].m_iNumBytes = s_ResourceInfo.stats[iIndex].TotalBytes;
		}

		s_pResourceManagerQuery->Issue(D3DISSUE_END);
	}
}
#endif // __DEV

static void CreateQueries()
{
	using namespace rage;
	if (GRCDEVICE.GetCurrent() != NULL)
	{
		GRCDEVICE.GetCurrent()->CreateQuery(D3DQUERYTYPE_EVENT, &s_KickEventQuery);

		for (int i = 0; i < kMaxFences; ++i)
		{
			GRCDEVICE.GetCurrent()->CreateQuery(D3DQUERYTYPE_EVENT, s_FenceEventQueries + i);
		}
	
#if __DEV
		HRESULT res = GRCDEVICE.GetCurrent()->CreateQuery(D3DQUERYTYPE_RESOURCEMANAGER, NULL);
		s_bRetailRuntime = res == D3DERR_NOTAVAILABLE;

		if	(s_bRetailRuntime)
			return;

#if	__BANK
	if (!s_pBank)
		AddResourceManagerWidgets();
#endif

		CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateQuery(D3DQUERYTYPE_RESOURCEMANAGER, &s_pResourceManagerQuery));
#endif
	}
}


static void ReleaseQueries()
{
#if __DEV
	LastSafeRelease(s_pResourceManagerQuery);
#endif // __DEV

	LastSafeRelease(s_KickEventQuery);
	for (int i = 0; i < kMaxFences; ++i)
	{
		LastSafeRelease(s_FenceEventQueries[i]);
	}
}

bool grcDevice::IsReady ()
{
	return	IsCreated()			&&
		!IsLost()				&&
		!IsPaused()				&&
		!IsInReset()			&&
		!IsInsideDeviceChange()	&&
		!IsMinimized()			&&
		!IsInSizeMove()			&&
		!IsReleasing()			&&
		!IsOccluded()			&&
		!IsDeviceRestored();
}

void grcDevice::DeviceLost() 
{
	GRCDEVICE.GRC_Shutdown();
	GRCDEVICE.Blit_Shutdown();
#if FAST_QUAD_SUPPORT
	FastQuad::Shutdown();
#endif

	ReleaseQueries();
	const int loopCount = sm_DeviceLostCb.GetCount();
	for (int i = 0; i < loopCount; ++i) {
		(sm_DeviceLostCb[i])();		// Make the callback now
	}
}

void grcDevice::DeviceRestored() {
	const int loopCount = sm_DeviceResetCb.GetCount();
	for (int i = 0; i < loopCount; ++i) {
		(sm_DeviceResetCb[i])();		// Make the callback now
	}
	// TODO: This was disabled on WIN32PC due to MT issues, but that breaks any normal app running in rag.
	// Make sure the states are dirty
	if (grcTextureFactoryPC::HasInstance())
		grcStateBlock::Default();

	CreateQueries();

#if FAST_QUAD_SUPPORT
	FastQuad::Init();
#endif
	GRCDEVICE.Blit_Init();
	GRCDEVICE.GRC_Init();
}

#endif	// __WIN32PC

// Protect a region of EDRAM by asserting/warning if someone tries to render to this region
// of EDRAM before it is unprotected.
#if ENABLE_EDRAM_PROTECTION

#define MAX_EDRAM_PROTECTED_REGIONS (1)

typedef struct grcRangeEDRAM
{
	grcRangeEDRAM() : m_nStart(0), m_nEnd(0) {};
	grcRangeEDRAM(u16 nStart, u16 nEnd) : m_nStart(nStart), m_nEnd(nEnd) {};
	u16 m_nStart;
	u16 m_nEnd;
} grcRangeEDRAM;
 
static atFixedArray<grcRangeEDRAM,MAX_EDRAM_PROTECTED_REGIONS> g_ProtectedEDRAM;

// Begin protecting a region of EDRAM that is covered by a render target's surface
void grcDevice::BeginProtectingEDRAM (grcDeviceSurface* pSurface)
{
	if ( g_ProtectedEDRAM.GetAvailable() )
	{
		// dwStard is inclusive, dwEnd is exclusive
		DWORD dwStart = pSurface->ColorInfo.ColorBase; // NOTE: ColorInfo and DepthInfo are in a union together but ColorBase and DepthBase overlap properly.
		DWORD dwEnd = dwStart + ((pSurface->Size + GPU_EDRAM_TILE_SIZE-1) / GPU_EDRAM_TILE_SIZE); // Round to next tile
		g_ProtectedEDRAM.Push(grcRangeEDRAM((u16)dwStart,(u16)dwEnd));
		return;
	}
	Assertf(false, "BeginProtectingEDRAM: out of slots in g_ProtectedEDRAM. You're either missing a call to EndProtectingEDRAM or you need to bump MAX_EDRAM_PROTECTED_REGIONS." );
}

// End protecting a region of EDRAM that is covered by a render target's surface
void grcDevice::EndProtectingEDRAM (grcDeviceSurface* pSurface)
{
	// dwStard is inclusive, dwEnd is exclusive
	DWORD dwStart = pSurface->ColorInfo.ColorBase; // NOTE: ColorInfo and DepthInfo are in a union together but ColorBase and DepthBase overlap properly.
	DWORD dwEnd = dwStart + ((pSurface->Size + GPU_EDRAM_TILE_SIZE-1) / GPU_EDRAM_TILE_SIZE); // Round to next tile
	for ( s32 i=0; i<g_ProtectedEDRAM.GetCount(); i++)
	{
		if ( g_ProtectedEDRAM[i].m_nStart == dwStart && g_ProtectedEDRAM[i].m_nEnd == dwEnd )
		{
			g_ProtectedEDRAM.Delete(i);
			return;
		}
	}
	Assertf(false, "EndProtectingEDRAM: called for a region of EDRAM that is not being protected: Start=0x%x End=0x%x",dwStart, dwEnd );
}

// Test if a region of EDRAM covered by a render target's surface is currently being protected
bool grcDevice::IsProtectedEDRAM (grcDeviceSurface* pSurface)
{
	// dwStard is inclusive, dwEnd is exclusive
	DWORD dwStart = pSurface->ColorInfo.ColorBase; // NOTE: ColorInfo and DepthInfo are in a union together but ColorBase and DepthBase overlap properly.
	DWORD dwEnd = dwStart + ((pSurface->Size + GPU_EDRAM_TILE_SIZE-1) / GPU_EDRAM_TILE_SIZE); // Round to next tile
	for ( s32 i=0; i<g_ProtectedEDRAM.GetCount(); i++)
	{
		if ( dwStart < g_ProtectedEDRAM[i].m_nEnd && dwEnd > g_ProtectedEDRAM[i].m_nStart )
		{
			return true;
		}
	}
	return false;
}
#endif // ENABLE_EDRAM_PROTECTION


grcDevice::Result grcDevice::CreateDepthStencilSurface(u32 width,u32 height,u32 format,bool discard,grcDeviceSurface **ppSurface) {
	return CHECK_HRESULT(sm_Current->CreateDepthStencilSurface(width,height,(D3DFORMAT)format,D3DMULTISAMPLE_NONE,0,discard,(SURFACE**)ppSurface,NULL));
}

void grcDevice::GetRenderTarget(u32 index,grcDeviceSurface **ppTarget) {
	CHECK_HRESULT(sm_Current->GetRenderTarget(index,(SURFACE**)ppTarget));
}

void grcDevice::SetRenderTarget(u32 index,grcDeviceSurface *pTarget) {
	VERIFY_EDRAM_SURFACE(pTarget);
	CHECK_HRESULT(sm_Current->SetRenderTarget(index,(SURFACE*)pTarget));
#if __XENON	|| __WIN32PC // Fix any Z flip 
	InvertViewportDepth(sm_Current);
#endif
}

void grcDevice::Clear(bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth,bool enableClearStencil,u32 clearStencil) {
	WIN32PC_ONLY(if (!sm_Current) return);
	u32 flags = (enableClearColor? D3DCLEAR_TARGET : 0) | (enableClearDepth? D3DCLEAR_ZBUFFER : 0) | (enableClearStencil? D3DCLEAR_STENCIL : 0);
	if (FAILED(sm_Current->Clear(0,NULL,flags,clearColor.GetDeviceColor(),FixDepth(clearDepth),clearStencil)))
		grcErrorf("grcDevice::Clear failed, probably trying to clear a buffer that isn't attached.");
}

#define D3DDECLTYPE_INVALID		(D3DDECLTYPE) 0
#if __XENON
D3DDECLTYPE
#else
BYTE
#endif 
d3dTypeMap[] =
{
	D3DDECLTYPE_INVALID,		// grcdsHalf,
	D3DDECLTYPE_FLOAT16_2,		// grcdsHalf2,
	D3DDECLTYPE_INVALID,		// grcdsHalf3,
	D3DDECLTYPE_FLOAT16_4,		// grcdsHalf4,

 	D3DDECLTYPE_FLOAT1,			// grcdsFloat,

	D3DDECLTYPE_FLOAT2,			// grcdsFloat2,
	D3DDECLTYPE_FLOAT3,			// grcdsFloat3,
	D3DDECLTYPE_FLOAT4,			// grcdsFloat4,

	D3DDECLTYPE_UBYTE4,			// grcdsUBYTE4
	D3DDECLTYPE_D3DCOLOR,		// grcdsColor
#if __XENON
	D3DDECLTYPE_DEC4N,			// grcdsPackedNormal,
#else
	D3DDECLTYPE_INVALID,        // NVidia failing to create this in DX9 modes.  Obsolete in DX10 - Can't change format easily because of implementation with 360
#endif

	D3DDECLTYPE_INVALID,		// grcdsShort_unorm,
	D3DDECLTYPE_USHORT2N,		// grcdsShort2_unorm,
	D3DDECLTYPE_INVALID,		// grcdsByte2_unorm,

	D3DDECLTYPE_SHORT2,			// grcdsShort2,
	D3DDECLTYPE_SHORT4,			// grcdsShort4,
};
CompileTimeAssert(NELEM(d3dTypeMap) == grcFvf::grcdsCount);

BYTE d3dUsageMap[grcVertexElement::grcvetCount] = 
{
	D3DDECLUSAGE_POSITION,
#if __XENON
	D3DDECLUSAGE_POSITION,
#else
	D3DDECLUSAGE_POSITIONT,
#endif
	D3DDECLUSAGE_NORMAL,
	D3DDECLUSAGE_BINORMAL,
	D3DDECLUSAGE_TANGENT,
	D3DDECLUSAGE_TEXCOORD,
	D3DDECLUSAGE_BLENDWEIGHT,
	D3DDECLUSAGE_BLENDINDICES,
	D3DDECLUSAGE_COLOR
};

grcVertexDeclaration* grcDevice::CreateVertexDeclaration(const grcVertexElement *pVertexElements, int elementCount,int UNUSED_PARAM(strideOverride))
{
	if (elementCount <= 0)
	{
		return NULL;
	}

	_D3DVERTEXELEMENT9 d3dElements[64];
	Assert(elementCount < 64);

	sysMemStartTemp();
	grcVertexDeclaration* ret = rage_new grcVertexDeclaration;
	sysMemEndTemp();

	std::fill_n<unsigned short*, size_t, unsigned short>(ret->Divider, NELEM(ret->Divider), grcFvf::s_DefaultDivider);
	ret->Stream0Size = 0;

	int dataOffset = 0;
	u32 currentStream = pVertexElements[0].stream;
	u16 currentDivider = static_cast<u16>(pVertexElements[0].streamFrequencyDivider);
	int i = 0;
	for( i = 0; i < elementCount; i++ )
	{
		WIN32PC_ONLY(AssertMsg(pVertexElements[i].streamFrequencyMode == grcFvf::grcsfmDivide, "Win32 PC only supports the divide operation for stream frequency"));

		if( currentStream != pVertexElements[i].stream )
		{
			AssertMsg(pVertexElements[i].stream > currentStream, "Vertex attributes must be specified in stream order");
			currentStream = pVertexElements[i].stream;
			currentDivider = static_cast<u16>(pVertexElements[i].streamFrequencyDivider);
			ret->Divider[currentStream] = currentDivider;
			dataOffset = 0;
		}

		d3dElements[i].Stream = (WORD)pVertexElements[i].stream;
		d3dElements[i].Offset = (WORD)dataOffset;
		d3dElements[i].Type = d3dTypeMap[pVertexElements[i].format];
		XENON_ONLY(AssertMsg(d3dElements[i].Type,"Trying to use unsupported element type on D3D"));
		d3dElements[i].Method = D3DDECLMETHOD_DEFAULT;
		d3dElements[i].Usage = d3dUsageMap[pVertexElements[i].type];
		d3dElements[i].UsageIndex = (BYTE)pVertexElements[i].channel;
		WIN32PC_ONLY(AssertMsg(pVertexElements[i].streamFrequencyDivider == currentDivider, "All vertex attributes for a given stream index must have the same stream frequency divider"));

		dataOffset += pVertexElements[i].size;

		if (currentStream == 0)
			ret->Stream0Size += pVertexElements[i].size;
	}
	D3DVERTEXELEMENT9 endItem = D3DDECL_END();
	d3dElements[i] = endItem;

	if (FAILED(sm_Current->CreateVertexDeclaration(d3dElements, &ret->D3dDecl)))
		Quitf("Unable to create vtx decl, probably unsupported format");
	XENON_ONLY(Assert(ret->D3dDecl->ReferenceCount == 1));
	return ret;	
}

void grcDevice::DestroyVertexDeclaration(grcVertexDeclaration *decl) 
{
	decl->Release();
}

grcDevice::Result grcDevice::SetVertexDeclaration(const grcVertexDeclaration *pDecl) {
	if (pDecl && pDecl != grcCurrentVertexDeclaration)
	{
		grcCurrentVertexDeclaration = pDecl;
		HRESULT retVal = sm_Current->SetVertexDeclaration(grcCurrentVertexDeclaration->D3dDecl);
#if __WIN32PC
		for (u32 i = 0; i < grcVertexDeclaration::c_MaxStreams; ++i)
		{
			if (grcCurrentDividers[i] != grcCurrentVertexDeclaration->Divider[i])
				retVal |= sm_Current->SetStreamSourceFreq(i, grcCurrentDividers[i] = grcCurrentVertexDeclaration->Divider[i]);
		}
#endif // __WIN32PC
		return retVal;
	}
	return S_OK;
}

grcDevice::Result grcDevice::GetVertexDeclaration(grcVertexDeclaration **ppDecl) {
	*ppDecl = const_cast<grcVertexDeclaration*>(grcCurrentVertexDeclaration);
	return S_OK;
}

#if !__XENON
static IDirect3DVertexBuffer9 *s_grcVB;
static int s_grcOffset, s_VertCount;
static const DWORD s_grcVBSize = grcDevice::BEGIN_VERTICES_MAX_SIZE; // Too small a space with cause vertex corruption
static grcDrawMode s_VertDrawMode;

static IDirect3DIndexBuffer9* s_grcIB = NULL;
static int s_grcIBOffset, s_grcIndexCount;
static const DWORD s_grcIBSize = grcDevice::BEGIN_INDICES_MAX_SIZE * sizeof(u16);

void grcDevice::GRC_Init() 
{
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateVertexBuffer(s_grcVBSize,	D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&s_grcVB,0));
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateIndexBuffer(s_grcIBSize,D3DUSAGE_DYNAMIC,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&s_grcIB,0));
	Assert(s_grcVB);
	Assert(s_grcIB);
	s_grcOffset = 0;
	s_grcIBOffset = 0;
}

void grcDevice::GRC_Shutdown() 
{
	SAFE_RELEASE_RESOURCE(s_grcVB);
	SAFE_RELEASE_RESOURCE(s_grcIB);
}
#endif

void* grcDevice::BeginVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize)
{
	grcStateBlock::Flush();
	ClearStreamSource(0);
	Assert(vertexCount * vertexSize <= BEGIN_VERTICES_MAX_SIZE);
	AssertMsg(!grcCurrentVertexBuffers[0],"Vertex Stream 0 still active in BeginVertices");
	AssertMsg(!grcCurrentVertexBuffers[1],"Vertex Stream 1 still active in BeginVertices");
	AssertMsg(grcCurrentVertexDeclaration,"BeginVertices - no declarator active! (did you use immediate mode directly?)");
	Assertf(grcCurrentVertexDeclaration->Stream0Size == vertexSize,"BeginVertices - current declarator is %d bytes, vertex size is %d",grcCurrentVertexDeclaration->Stream0Size,vertexSize);

#if __XENON
	static D3DPRIMITIVETYPE translate[] = {
		D3DPT_POINTLIST,
		D3DPT_LINELIST,
		D3DPT_LINESTRIP,
		D3DPT_TRIANGLELIST,
		D3DPT_TRIANGLESTRIP,
		D3DPT_TRIANGLEFAN,
		D3DPT_QUADLIST,
		D3DPT_RECTLIST
	};
	void *result = NULL;
	sm_Current->BeginVertices(translate[dm],vertexCount,vertexSize,&result);

	StoreBufferEnd(result, vertexCount, vertexSize);

	return result;
#else
	u32 lockFlags = D3DLOCK_NOOVERWRITE;
	s_VertDrawMode = dm;
	s_VertCount = vertexCount;
	int size = vertexCount * vertexSize;
	// If vertex size is exact multiple of 16, assume we may be using vectorized operations to fill it.
	if ((vertexSize & 15) == 0)
		s_grcOffset = (s_grcOffset + 15) & ~15;

	if ((s_grcOffset + size) > s_grcVBSize) {
		s_grcOffset = 0;
		lockFlags = D3DLOCK_DISCARD;
	}

	// Setting the stream source before you lock seems marginally faster.
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetStreamSource(0,s_grcVB,s_grcOffset,vertexSize));
	void *result = NULL;
#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	if (s_grcVB->Lock(s_grcOffset,size,&result,lockFlags) == D3D_OK)
	{
		Assert(result != NULL && "Driver/Runtime returned NULL Lock - Critical Error");
		if (result != NULL)
			s_grcOffset += size;
		else
			s_grcOffset = 0;
	}
	else
	{
		grcWarningf("Vertex Buffer Lock Failed");
		s_grcOffset = 0;
		result = NULL;
	}

#if STALL_DETECTION
	if ((oTime.GetMsTime() > STALL_TIME) && STALL_ONLY_RENDERTHREAD(GRCDEVICE.CheckThreadOwnership()))
	{
		grcWarningf("Immediate Vertex Buffer Lock took %f milliseconds on frame %d", oTime.GetMsTime(), GRCDEVICE.GetFrameCounter());
	}
#endif // STALL_DETECTION

#if STATISTICS
	static u32 suLastFrame = 0;
	static u32 suNumOfLocksPerFrame = 0;
	static u32 suNumOfOverwritesPerFrame = 0;
	static u32 suNumOfBytesPerFrame = 0;
	static u32 suMaxLocksPerFrame = 0;
	static u32 suMaxOverwritesPerFrame = 0;
	static u32 suMaxBytesPerFrame = 0;
	static u32 suUpdateRate = 200;
	static u32 suLastUpdate = 0;

	suNumOfLocksPerFrame++;
	suNumOfOverwritesPerFrame += (lockFlags & D3DLOCK_DISCARD) ? 1 : 0;
	suNumOfBytesPerFrame += size;

	if (suLastFrame != GRCDEVICE.GetFrameCounter())
	{
		suMaxLocksPerFrame = (suNumOfLocksPerFrame > suMaxLocksPerFrame) ? suNumOfLocksPerFrame : suMaxLocksPerFrame;
		suMaxOverwritesPerFrame = (suNumOfOverwritesPerFrame > suMaxOverwritesPerFrame) ? suNumOfOverwritesPerFrame : suMaxOverwritesPerFrame;
		suMaxBytesPerFrame = (suNumOfBytesPerFrame > suMaxBytesPerFrame) ? suNumOfBytesPerFrame : suMaxBytesPerFrame;

		suLastFrame = GRCDEVICE.GetFrameCounter();

		if ((suLastUpdate + suUpdateRate) < suLastFrame)
		{
			grcWarningf("Locks %u(%u) Overwrites %u(%u) Bytes %u(%u)", suNumOfLocksPerFrame, suMaxLocksPerFrame, suNumOfOverwritesPerFrame, suMaxOverwritesPerFrame, suNumOfBytesPerFrame, suMaxBytesPerFrame);
			suLastUpdate = suLastFrame;
		}
		suNumOfLocksPerFrame = 0;
		suNumOfOverwritesPerFrame = 0;
		suNumOfBytesPerFrame = 0;
	}
#endif // STATISTICS

	StoreBufferEnd(result, vertexCount, vertexSize);

	return result;
#endif
}

#if RSG_PC || RSG_DURANGO
void* grcDevice::BeginIndexedVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize,u32 WIN32PC_ONLY(indexCount),void** WIN32PC_ONLY(vertexPtr),void** WIN32PC_ONLY(indexPtr), u32 streamID)
{
	grcStateBlock::Flush();
	ClearStreamSource(streamID);
	Assert(vertexCount * vertexSize <= BEGIN_VERTICES_MAX_SIZE);
	AssertMsg(!grcCurrentVertexBuffers[0],"Vertex Stream 0 still active in BeginVertices");
	AssertMsg(!grcCurrentVertexBuffers[1],"Vertex Stream 1 still active in BeginVertices");
	AssertMsg(grcCurrentVertexDeclaration,"BeginVertices - no declarator active! (did you use immediate mode directly?)");
	Assertf(grcCurrentVertexDeclaration->Stream0Size == vertexSize,"BeginVertices - current declarator is %d bytes, vertex size is %d",grcCurrentVertexDeclaration->Stream0Size,vertexSize);

#if __XENON
	static D3DPRIMITIVETYPE translate[] = {
		D3DPT_POINTLIST,
		D3DPT_LINELIST,
		D3DPT_LINESTRIP,
		D3DPT_TRIANGLELIST,
		D3DPT_TRIANGLESTRIP,
		D3DPT_TRIANGLEFAN,
		D3DPT_QUADLIST,
		D3DPT_RECTLIST
	};
	void *result = NULL;
	sm_Current->BeginVertices(translate[dm],vertexCount,vertexSize,&result);

	StoreBufferEnd(result, vertexCount, vertexSize);

	return result;
#else
	u32 lockFlags = D3DLOCK_NOOVERWRITE;
	s_VertDrawMode = dm;
	s_VertCount = vertexCount;
	int size = vertexCount * vertexSize;
	// If vertex size is exact multiple of 16, assume we may be using vectorized operations to fill it.
	if ((vertexSize & 15) == 0)
		s_grcOffset = (s_grcOffset + 15) & ~15;

	if ((s_grcOffset + size) > s_grcVBSize) {
		s_grcOffset = 0;
		lockFlags = D3DLOCK_DISCARD;
	}

	u32 IBLockFlags = D3DLOCK_NOOVERWRITE;
	s_grcIndexCount = indexCount;
	int IBSize = indexCount * sizeof(u16);
	if ((s_grcIBOffset + IBSize) > s_grcIBSize){
		s_grcIBOffset = 0;
		IBLockFlags = D3DLOCK_DISCARD;
	}

	// Setting the stream source before you lock seems marginally faster.
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetStreamSource(streamID,s_grcVB,s_grcOffset,vertexSize));
#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	if (s_grcVB->Lock(s_grcOffset,size,vertexPtr,lockFlags) == D3D_OK)
	{
		Assert(*vertexPtr != NULL && "Driver/Runtime returned NULL Lock - Critical Error");
		if (*vertexPtr != NULL)
			s_grcOffset += size;
		else
			s_grcOffset = 0;
	}
	else
	{
		grcWarningf("Vertex Buffer Lock Failed");
		s_grcOffset = 0;
		*vertexPtr = NULL;
	}

	// Do we really need to lock all the time - Oscar
	if (s_grcIB->Lock(s_grcIBOffset,IBSize,indexPtr,IBLockFlags) == D3D_OK)
	{
		Assert(*indexPtr != NULL && "Driver/Runtime returned NULL Lock - Critical Error");
		if (*indexPtr != NULL)
			s_grcIBOffset += IBSize;
		else
			s_grcIBOffset = 0;
	}
	else
	{
		grcWarningf("Index Buffer Lock Failed");
		s_grcIBOffset = 0;
		*indexPtr = NULL;
	}

#if STALL_DETECTION
	if ((oTime.GetMsTime() > STALL_TIME) && STALL_ONLY_RENDERTHREAD(GRCDEVICE.CheckThreadOwnership()))
	{
		grcWarningf("Immediate Vertex Buffer Lock took %f milliseconds on frame %d", oTime.GetMsTime(), GRCDEVICE.GetFrameCounter());
	}
#endif // STALL_DETECTION

#if STATISTICS
	static u32 suLastFrame = 0;
	static u32 suNumOfLocksPerFrame = 0;
	static u32 suNumOfOverwritesPerFrame = 0;
	static u32 suNumOfBytesPerFrame = 0;
	static u32 suMaxLocksPerFrame = 0;
	static u32 suMaxOverwritesPerFrame = 0;
	static u32 suMaxBytesPerFrame = 0;
	static u32 suUpdateRate = 200;
	static u32 suLastUpdate = 0;

	suNumOfLocksPerFrame++;
	suNumOfOverwritesPerFrame += (lockFlags & D3DLOCK_DISCARD) ? 1 : 0;
	suNumOfBytesPerFrame += size;

	if (suLastFrame != GRCDEVICE.GetFrameCounter())
	{
		suMaxLocksPerFrame = (suNumOfLocksPerFrame > suMaxLocksPerFrame) ? suNumOfLocksPerFrame : suMaxLocksPerFrame;
		suMaxOverwritesPerFrame = (suNumOfOverwritesPerFrame > suMaxOverwritesPerFrame) ? suNumOfOverwritesPerFrame : suMaxOverwritesPerFrame;
		suMaxBytesPerFrame = (suNumOfBytesPerFrame > suMaxBytesPerFrame) ? suNumOfBytesPerFrame : suMaxBytesPerFrame;

		suLastFrame = GRCDEVICE.GetFrameCounter();

		if ((suLastUpdate + suUpdateRate) < suLastFrame)
		{
			grcWarningf("Locks %u(%u) Overwrites %u(%u) Bytes %u(%u)", suNumOfLocksPerFrame, suMaxLocksPerFrame, suNumOfOverwritesPerFrame, suMaxOverwritesPerFrame, suNumOfBytesPerFrame, suMaxBytesPerFrame);
			suLastUpdate = suLastFrame;
		}
		suNumOfLocksPerFrame = 0;
		suNumOfOverwritesPerFrame = 0;
		suNumOfBytesPerFrame = 0;
	}
#endif // STATISTICS

	StoreBufferEnd(*vertexPtr, vertexCount, vertexSize);

	return *vertexPtr;
#endif
}
#endif // __WIN32PC

void grcDevice::EndCreateVertices(const void *bufferEnd)
{
#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	VerifyBufferEnd(bufferEnd);
}

void grcDevice::EndCreateVertices(u32 WIN32PC_ONLY(vertexCount))
{
#if __WIN32PC
	Assert((s32)vertexCount < s_VertCount);
	s_VertCount = Min((s32)vertexCount, s_VertCount);
#endif // __WIN32PC
	EndCreateVertices((void *) NULL);
}

#if __WIN32PC
void grcDevice::EndCreateIndexedVertices(u32 UNUSED_PARAM(indexCount), u32 vertexCount)
{
	VerifyBufferEnd(NULL);
	VerifyIBBufferEnd(NULL);

	Assert((s32)vertexCount < s_VertCount);
	s_VertCount = Min((s32)vertexCount, s_VertCount);

	AssertVerify(s_grcVB->Unlock() == D3D_OK);
	AssertVerify(s_grcIB->Unlock() == D3D_OK);

	sm_Current->SetIndices(s_grcIB);
}
#endif // __WIN32PC

void grcDevice::DrawVertices(const void *UNUSED_PARAM(bufferEnd))
{
#if __XENON
	sm_Current->EndVertices();
#else
	AssertVerify(s_grcVB->Unlock() == D3D_OK);
	switch (s_VertDrawMode) {
			case drawPoints:
				CHECK_HRESULT(sm_Current->DrawPrimitive(D3DPT_POINTLIST,0,s_VertCount));
				break;
			case drawLines:
			case drawQuads:	// TOTALLY WRONG, BUT AVOIDS AN ASSERTION
				CHECK_HRESULT(sm_Current->DrawPrimitive(D3DPT_LINELIST,0,s_VertCount>>1));
				break;
			case drawLineStrip:
				CHECK_HRESULT(sm_Current->DrawPrimitive(D3DPT_LINESTRIP,0,s_VertCount-1));
				break;
			case drawTris:
				CHECK_HRESULT(sm_Current->DrawPrimitive(D3DPT_TRIANGLELIST,0,s_VertCount/3));
				break;
			case drawTriStrip:
				CHECK_HRESULT(sm_Current->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,s_VertCount-2));
				break;
			case drawTriFan:
				CHECK_HRESULT(sm_Current->DrawPrimitive(D3DPT_TRIANGLEFAN,0,s_VertCount-2));
				break;
			default:
				Assert(0 && "unhandled prim type");
	}
#endif
#if STALL_DETECTION
	if ((oTime.GetMsTime() > STALL_TIME) && STALL_ONLY_RENDERTHREAD(GRCDEVICE.CheckThreadOwnership()))
	{
		grcWarningf("Immediate Vertex Buffer Unlock took %f milliseconds on frame %d", oTime.GetMsTime(), GRCDEVICE.GetFrameCounter());
	}
#endif // STALL_DETECTION
}

void grcDevice::DrawVertices(u32 UNUSED_PARAM(vertexCount))
{
	DrawVertices((void *) NULL);
}

#if __WIN32PC
void grcDevice::DrawIndexedVertices(u32 indexCount, u32 UNUSED_PARAM(vertexCount))
{
	int IBOffset = s_grcIBOffset - (s_grcIndexCount * sizeof(u16));

	switch (s_VertDrawMode) {
			case drawLines:
			case drawQuads:	// TOTALLY WRONG, BUT AVOIDS AN ASSERTION
				CHECK_HRESULT(sm_Current->DrawIndexedPrimitive(D3DPT_LINELIST,0,0,s_VertCount,IBOffset,indexCount>>1));
				break;
			case drawLineStrip:
				CHECK_HRESULT(sm_Current->DrawIndexedPrimitive(D3DPT_LINESTRIP,0,0,s_VertCount,IBOffset,indexCount-1));
				break;
			case drawTris:
				CHECK_HRESULT(sm_Current->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,s_VertCount,IBOffset,indexCount/3));
				break;
			case drawTriStrip:
				CHECK_HRESULT(sm_Current->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP,0,0,s_VertCount,IBOffset,indexCount-2));
				break;
			case drawTriFan:
				CHECK_HRESULT(sm_Current->DrawIndexedPrimitive(D3DPT_TRIANGLEFAN,0,0,s_VertCount,IBOffset,indexCount-2));
				break;
			default:
				Assert(0 && "unhandled prim type");
	}
}
#endif // __WIN32PC

void grcDevice::ReleaseVertices(const void *UNUSED_PARAM(bufferEnd))
{
}

void grcDevice::ReleaseVertices(u32 UNUSED_PARAM(vertexCount))
{
}

#if __WIN32PC
void grcDevice::ReleaseIndexedVertices(u32 UNUSED_PARAM(indexCount), u32 UNUSED_PARAM(vertexCount))
{
}
#endif // __WIN32PC


PARAM(multiSample,"[grcore] Number of multisamples (1, 2, or 4)");
PARAM(multiSampleQuality,"[grcore] Quality level of multisamples - Video card dependent");
#if __XENON
#if HACK_GTA4
#define ASYNCSWAP_NUM_FRONTBUFFERS (3)
D3DTexture *g_FrontBuffers[ASYNCSWAP_NUM_FRONTBUFFERS]; //triple buffered for async swaps
#else
D3DTexture *g_FrontBuffers[2]; 
#endif
int g_CurrentFrontBuffer;
grcDeviceSurface *g_BackBuffer;
grcDeviceSurface *g_DepthStencil;

bool grcDevice::AllocateSwapChain() {
	AssertVerify(SUCCEEDED(sm_Current->CreateTexture(
	  s_d3dpp.BackBufferWidth, s_d3dpp.BackBufferHeight,
      1, D3DUSAGE_RENDERTARGET, s_d3dpp.FrontBufferFormat, 
      D3DPOOL_DEFAULT, &g_FrontBuffers[0], NULL )));

	AssertVerify(SUCCEEDED(sm_Current->CreateTexture(
		s_d3dpp.BackBufferWidth, s_d3dpp.BackBufferHeight,
		1, D3DUSAGE_RENDERTARGET, s_d3dpp.FrontBufferFormat, 
		D3DPOOL_DEFAULT, &g_FrontBuffers[1], NULL )));

#if HACK_GTA4
	AssertVerify(SUCCEEDED(sm_Current->CreateTexture(
		s_d3dpp.BackBufferWidth, s_d3dpp.BackBufferHeight,
		1, D3DUSAGE_RENDERTARGET, s_d3dpp.FrontBufferFormat, 
		D3DPOOL_DEFAULT, &g_FrontBuffers[2], NULL )));
#endif

    AssertVerify(SUCCEEDED(sm_Current->CreateDepthStencilSurface(
		s_d3dpp.BackBufferWidth, s_d3dpp.BackBufferHeight,
		s_d3dpp.AutoDepthStencilFormat, s_d3dpp.MultiSampleType, s_d3dpp.MultiSampleQuality, FALSE,
		&g_DepthStencil,NULL)));
	AssertVerify(SUCCEEDED(sm_Current->CreateRenderTarget(
		s_d3dpp.BackBufferWidth, s_d3dpp.BackBufferHeight,
		s_d3dpp.BackBufferFormat, s_d3dpp.MultiSampleType, s_d3dpp.MultiSampleQuality, FALSE,
		&g_BackBuffer,NULL)));

#if _XDK_VER >= 1871
	// 8% is the safe area
	CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_PRESENTIMMEDIATETHRESHOLD, s_SwapImmediateIfLate ? 100 : 8));
#endif
	return true;
}

bool grcDevice::FreeSwapChain() {
	SetRenderTarget(0, NULL);
	CHECK_HRESULT(sm_Current->SetDepthStencilSurface(NULL));

#if HACK_GTA4
	LastSafeRelease(g_FrontBuffers[2]);
#endif

	LastSafeRelease(g_FrontBuffers[1]);
	LastSafeRelease(g_FrontBuffers[0]);
	LastSafeRelease(g_BackBuffer);
	LastSafeRelease(g_DepthStencil);
	return true;
}
#endif

#if __WIN32PC
bool grcDevice::AllocateSwapChain() 
{
	UpdatePresentParameters();
	if (sm_Current->GetSwapChain(0, (IDirect3DSwapChain9**)&sm_pSwapChain) != D3D_OK)
	{
		Printf("Failed to create swap chain");
		Quitf("D3D Error - Please re-boot your system");
		// return false;
	}
	SetReleasing(false);
	return true;
}

bool grcDevice::FreeSwapChain() 
{
	SetReleasing(true);
	if (sm_pSwapChain != NULL)
	{
	#if !__RESOURCECOMPILER
		AssertVerify(sm_pSwapChain->Release() == 0);
	#endif //__RESOURCECOMPILER
		sm_pSwapChain = NULL;
	}
	return true;
}
#endif // __WIN32PC


#if __XENON
static int s_PrimaryRingBufferSize = 32 * 1024, s_SecondaryRingBufferSize = 8192 * 1024, s_NumSegments;
static bool s_BufferTwoFrames;
static int sm_DisplayWidth;
static int sm_DisplayHeight;
static bool sm_IsHiDef = false;
static bool sm_IsWideScreen = false;
// 32kb and 2048kb.
void grcDevice::SetRingBufferParameters(int primarySize,int secondarySize,int segments, bool twoFrames) {
 	s_PrimaryRingBufferSize = primarySize << 10;
	s_SecondaryRingBufferSize = secondarySize << 10;
	s_NumSegments = segments;
	s_BufferTwoFrames = twoFrames;
}
#endif

#if __XENON
NOSTRIP_PARAM(fpz,"[grcore] Enable floating-point depth buffers");
#endif

#if __XENON && HACK_GTA4 //Could be useful to other 360 projects - triple buffer the swap chain - look at xdk docs for more info (search QuerySwapStatus)

static D3DSWAPDATA g_swapData; 
static D3DVBLANKDATA g_vblankData;

// Constants for keeping track of visible frames on the display device
const D3DASYNCBLOCK AsyncBlockFrameIsNotVisible = 2;
const D3DASYNCBLOCK AsyncBlockFrameIsVisible = 4;

// In VC++ volatile ensures that the compiler don’t cache values for excessive 
// time. This is useful since we read and write from different threads
volatile D3DASYNCBLOCK g_AsyncBlock[ ASYNCSWAP_NUM_FRONTBUFFERS ] = { AsyncBlockFrameIsNotVisible, AsyncBlockFrameIsNotVisible, AsyncBlockFrameIsNotVisible };

// Called by ASYNC_SWAP_D3D_CB, we've done things this way so that we can have a callback handler with access to grcDevice members without having to 
// expose D3DSWAPDATA in device.h
void grcDevice::SwapCallbackHandler( unsigned long LastVBlank, unsigned long LastSwapVBlank, unsigned long PercentScanned, /*INOUT*/ unsigned long& rSwapVBlank )
{
	// LastVBlank     : The vertical blank count of the most recent vertical blank.
	//
	// LastSwapVBlank : Identifies the vertical blank at which the last swap was scheduled to be shown. 
	//
	// PercentScanned : The percentage between 0 and 100 that indicates where the DAC's current raster is. 
	//                  Use this member to emulate the D3DRS_PRESENTIMMEDIATETHRESHOLD functionality.
	//
	// SwapVBlank     : Identifies the vertical blank at which the front buffer will be shown. This is an IN/OUT
	//                  parameter. If this value is the same as the LastVBlank member, the front buffer will be 
	//                  shown immediately, without waiting for the next vertical blank

	// Respect the frame lock, ensure that at least uFrameLock v-syncs occur between swaps
	//static const int RENDER_THREAD_INDEX = 0; // g_RenderThreadIndex is TLS and we're in DPC so hard code index=0 (__XENON only has 1 render thread anyway).
	//rSwapVBlank = max(LastVBlank, LastSwapVBlank + sm_CurrentWindows[RENDER_THREAD_INDEX].uFrameLock ); 
	
	// On second thought... let's ignore uFrameLock. If we fall behind then let's try and catch up as quickly as possible: if there's a new frame ready,
	// just swap to it now. Higher level code should be frame limiting us to 33ms frames anyway.
	rSwapVBlank = max(LastVBlank, LastSwapVBlank + 1 );

	// If we're about to show the buffer but the DAC is still scanning the frame, then wait until the next VBlank (so we don't tear)
	static const u32 nPercentScannedTop = 0;      // Define region in which we will allow tearing (0/100 to disable tearing)
	static const u32 nPercentScannedBottom = 100;
	if ( (rSwapVBlank == LastVBlank) && (PercentScanned>nPercentScannedTop && PercentScanned<nPercentScannedBottom) )
	{
		rSwapVBlank++;
	}
}

// Called by VBLANK_D3D_CB, we've done things this way so that we can have a callback handler with access to grcDevice members without having to 
// expose D3DVBLANKDATA in device.h
volatile D3DASYNCBLOCK g_StompedBlock = AsyncBlockFrameIsNotVisible;
void grcDevice::VBlankCallbackHandler( unsigned long Swap )
{
	// pData->Swap = Count of the most recent swap that was shown
	const u32 nCurrentFrameIndex = (Swap==0) ? 0 : (Swap-1) % ASYNCSWAP_NUM_FRONTBUFFERS; // This one is on screen
	for ( u32 nBufferIndex = 0; nBufferIndex < ASYNCSWAP_NUM_FRONTBUFFERS; nBufferIndex++ )
	{
		
		if ( nCurrentFrameIndex == nBufferIndex )
		{
			// Set the currently visible frame to AsyncBlockFrameIsVisible. Because this function gets called on every VSync (not just the ones in
			// which we are actually revealing a new frame) it's possible we've already had a VSync for this buffer index and we may already
			// have a GPU block handle stored in g_AsyncBlock[nBufferIndex] so careful not to stomp it! Only set AsyncBlockFrameIsVisible if
			// the current value is AsyncBlockFrameIsNotVisible
			InterlockedCompareExchange64( (PLONG64)&g_AsyncBlock[nBufferIndex], AsyncBlockFrameIsVisible, AsyncBlockFrameIsNotVisible );
		}
		else
		{
			// Unlock the non-visible frames since they are free to be stomped on now that they are off screen
			D3DASYNCBLOCK NonVisibleFrameBlock = InterlockedExchange64( (PLONG64)&g_AsyncBlock[nBufferIndex],	AsyncBlockFrameIsNotVisible );

            if ( NonVisibleFrameBlock != AsyncBlockFrameIsVisible &&
                 NonVisibleFrameBlock != AsyncBlockFrameIsNotVisible )
			{
				// We found a real D3DASYNCBLOCK for frame nBufferIndex, so we unblock the GPU
				sm_Current->SignalAsyncResources( NonVisibleFrameBlock );
			}
		}
	}
}

// The ASYNC_SWAP_D3D_CB callback function is called whenever the GPU completes the rendering of a frame and uses a deferred procedure calls (DPC) and should
// do little processing (for more information about DPC calls, see Kernel DPC Callbacks in the XDK) only a few XTL functions are safe to call at DPC level. 
void ASYNC_SWAP_D3D_CB(D3DSWAPDATA *pData)
{
	grcDevice::SwapCallbackHandler(pData->LastVBlank, pData->LastSwapVBlank, pData->PercentScanned, pData->SwapVBlank);
	g_swapData = *pData;
}

// The VBLANK_D3D_CB callback function is called on each vertical blank and uses a deferred procedure calls (DPC) and should do little processing (for more 
// information about DPC calls, see Kernel DPC Callbacks in the XDK) only a few XTL functions are safe to call at DPC level. 
void VBLANK_D3D_CB(D3DVBLANKDATA *pData)
{
	grcDevice::VBlankCallbackHandler(pData->Swap);
	g_vblankData = *pData;
}

void grcDevice::WaitOnFreeFrontBuffer()
{
	// Use the Direct3D swap counter to select the current frame. This is a new frame, so we should have just recently called swap
	D3DSWAP_STATUS SwapStatus;
	sm_Current->QuerySwapStatus( &SwapStatus );
	const DWORD dwCurrentFrameSwap = SwapStatus.Swap;
	const DWORD dwSwapIndex = dwCurrentFrameSwap % ASYNCSWAP_NUM_FRONTBUFFERS;

	// If no swaps have been submitted by the CPU yet then this is our first frame (congratulations!). For the very first frame it 
	// it will look like we're rendering to the current front buffer (even though we aren't) and normally we would want to insert a
	// GPU block in this situation so that we don't render to the currently visible buffer. BUT this is just a trick perpetrated by
	// D3D to get the ball rolling on the first frame. So, in this special case, we don't actually want to block here or we could 
	// deadlock (or get ourselves into a bad state). Instead we just get right to work submitting
	// GPU commands.
	if ( SwapStatus.Swap == 0 )
	{
		return;
	}

	// Throttle the CPU if necessary
	bool bThrottle = false;
	do 
	{
		// Don't let the CPU get too far ahead of the VSYNC
		D3DSWAP_STATUS SwapStatus;
		sm_Current->QuerySwapStatus( &SwapStatus );
		SwapStatus.EnqueuedCount = SwapStatus.Swap - g_vblankData.Swap; // SwapStatus.Swap = num swaps issued by CPU, g_vblankData.Swap = num swaps we've actually revealed to the screen
		bThrottle = SwapStatus.EnqueuedCount >= ASYNCSWAP_NUM_FRONTBUFFERS;

		// Not sure if this is necessary, it prevents us from stomping a valid block handle (because we let the GPU get too far ahead
		// of the VSYNC). The CPU throttle above should already prevent this kind of stomp.
		//D3DASYNCBLOCK FrameBlock = InterlockedCompareExchange64( (PLONG64)&g_AsyncBlock[dwSwapIndex], 0, 0 );
		//bThrottle = bThrottle || ((FrameBlock != AsyncBlockFrameIsVisible) && (FrameBlock != AsyncBlockFrameIsNotVisible));

	} while ( bThrottle );

	// Atomic read then compare: AsyncBlock[ i ] == AsyncBlockFrameIsVisible
	D3DASYNCBLOCK CurrentFrameBlock = InterlockedCompareExchange64( (PLONG64)&g_AsyncBlock[dwSwapIndex], 0, 0 );
	if ( CurrentFrameBlock == AsyncBlockFrameIsVisible )
	{
		// Insert a block on the GPU if the current frame buffer is still being displayed
		D3DASYNCBLOCK NewFrameBlock = sm_Current->InsertBlockOnAsyncResources( 0, NULL, 0, NULL, 0 );
		D3DASYNCBLOCK OldFrameBlock = InterlockedCompareExchange64( (PLONG64)&g_AsyncBlock[dwSwapIndex], NewFrameBlock, AsyncBlockFrameIsVisible ); 
		if ( OldFrameBlock == AsyncBlockFrameIsNotVisible )
		{
			// This rarely happens. We are too late and the VBlank is gone.  The frame is now hidden, so we need to unblock the GPU here instead
			// of in the VBlank callback.
			sm_Current->SignalAsyncResources( NewFrameBlock );
		}
	}
}

void grcDevice::ASyncSwap()
{
	D3DSWAP_STATUS swap_status;
	sm_Current->QuerySwapStatus(&swap_status);

	if (g_swapData.Flags!=D3DSWAPDATA_ASYNCHRONOUS)
	{
		sm_Current->SetSwapMode(TRUE);
	}

	sm_Current->Swap(g_FrontBuffers[g_CurrentFrontBuffer],NULL);

	g_CurrentFrontBuffer = (g_CurrentFrontBuffer+1) % ASYNCSWAP_NUM_FRONTBUFFERS;
}
#endif

void grcDevice::GetMultiSample(u32 &uType, u32 &uQuality)
{
	uType = 0;
	PARAM_multiSample.Get(uType);

#if __WIN32PC
	static D3DMULTISAMPLE_TYPE lut[] = 
	{
			D3DMULTISAMPLE_NONE,		// 0
			D3DMULTISAMPLE_NONE,
			D3DMULTISAMPLE_2_SAMPLES,
			D3DMULTISAMPLE_3_SAMPLES,
			D3DMULTISAMPLE_4_SAMPLES,
			D3DMULTISAMPLE_5_SAMPLES,
			D3DMULTISAMPLE_6_SAMPLES,
			D3DMULTISAMPLE_7_SAMPLES,
			D3DMULTISAMPLE_8_SAMPLES,
			D3DMULTISAMPLE_9_SAMPLES,
			D3DMULTISAMPLE_10_SAMPLES,
			D3DMULTISAMPLE_11_SAMPLES,
			D3DMULTISAMPLE_12_SAMPLES,
			D3DMULTISAMPLE_13_SAMPLES,
			D3DMULTISAMPLE_14_SAMPLES,
			D3DMULTISAMPLE_15_SAMPLES,
			D3DMULTISAMPLE_16_SAMPLES,
	};
#else
	static D3DMULTISAMPLE_TYPE lut[] = 
	{
			D3DMULTISAMPLE_NONE,		// 0
			D3DMULTISAMPLE_NONE,		// 1
			D3DMULTISAMPLE_2_SAMPLES,	// 2
			D3DMULTISAMPLE_2_SAMPLES,	// 3
			D3DMULTISAMPLE_4_SAMPLES	// 4
	};
#endif

	uType = (lut[uType]);

	if (uType != D3DMULTISAMPLE_NONE)
	{
		PARAM_multiSampleQuality.Get(uQuality);
	}
	else
	{
		uQuality = 0;
	}
}

void grcDevice::UpdatePresentParameters() {

	memset(&s_d3dpp, 0, sizeof(s_d3dpp));

	//int multisamples = 0;
	//PARAM_multiSample.Get(multisamples);

	//static D3DMULTISAMPLE_TYPE lut[] = 
	//{
	//		D3DMULTISAMPLE_NONE,		// 0
	//		D3DMULTISAMPLE_NONE,		// 1
	//		D3DMULTISAMPLE_2_SAMPLES,	// 2
	//		D3DMULTISAMPLE_2_SAMPLES,	// 3
	//		D3DMULTISAMPLE_4_SAMPLES,	// 4
	//		D3DMULTISAMPLE_8_SAMPLES, 
	//		D3DMULTISAMPLE_16_SAMPLES 
	//};

	s_d3dpp.BackBufferWidth = sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	s_d3dpp.BackBufferHeight = sm_CurrentWindows[g_RenderThreadIndex].uHeight;

if(PARAM_hdr.Get())
	#if __WIN32PC
		if (0) /*( (s_D3D9 != NULL) && SUCCEEDED( s_D3D9->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
													D3DDEVTYPE_HAL,
													D3DFMT_A2R10G10B10,
													D3DUSAGE_RENDERTARGET,
													D3DRTYPE_TEXTURE,
													D3DFMT_A2R10G10B10)))*/
		{
			s_d3dpp.BackBufferFormat = D3DFMT_A2R10G10B10;
		}
		else
		{
			s_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
		}
	#elif __XENON
		s_d3dpp.BackBufferFormat = D3DFMT_A2B10G10R10F_EDRAM;
	#elif RSG_DURANGO
		s_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	#endif
else
#if __WIN32PC || RSG_DURANGO
	s_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	s_d3dpp.EnableAutoDepthStencil = PARAM_autodepthstencil.Get() ? TRUE : FALSE;

	if (g_inWindow && !PARAM_framelockinwindow.Get())
		s_d3dpp.PresentationInterval = sm_CurrentWindows[g_RenderThreadIndex].uFrameLock ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	else
		s_d3dpp.PresentationInterval = sm_CurrentWindows[g_RenderThreadIndex].uFrameLock ? sm_CurrentWindows[g_RenderThreadIndex].uFrameLock : D3DPRESENT_INTERVAL_IMMEDIATE;
#elif __XENON
	if (PARAM_srgb.Get())
	{
		s_d3dpp.BackBufferFormat = static_cast<D3DFORMAT>(MAKESRGBFMT(D3DFMT_A8R8G8B8));
	}
	else
	{
		s_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	}
	s_d3dpp.FrontBufferFormat =  D3DFMT_LE_X8R8G8B8;

	if (sm_LetterBox)
		s_d3dpp.Flags &= ~D3DPRESENTFLAG_NO_LETTERBOX;
	else
		s_d3dpp.Flags |= D3DPRESENTFLAG_NO_LETTERBOX;

	if (sm_LetterBox == false)
	{
		// REALLY don't letterbox my output (in vga modes the scaler will still letterbox the output even with D3DPRESENTFLAG_NO_LETTERBOX)
		s_d3dpp.VideoScalerParameters.ScalerSourceRect.x1 = 0;
		s_d3dpp.VideoScalerParameters.ScalerSourceRect.y1 = 0;
		s_d3dpp.VideoScalerParameters.ScalerSourceRect.x2 = sm_CurrentWindows[g_RenderThreadIndex].uWidth;
		s_d3dpp.VideoScalerParameters.ScalerSourceRect.y2 = sm_CurrentWindows[g_RenderThreadIndex].uHeight;
		s_d3dpp.VideoScalerParameters.ScaledOutputWidth = sm_DisplayWidth;
		s_d3dpp.VideoScalerParameters.ScaledOutputHeight = sm_DisplayHeight;
	}

	s_d3dpp.DisableAutoBackBuffer = TRUE;
	s_d3dpp.DisableAutoFrontBuffer = TRUE;
#if _XDK_VER < 1871
	s_d3dpp.PresentationInterval = s_SwapImmediateIfLate? (sm_FrameLock ? sm_FrameLock : D3DPRESENT_INTERVAL_IMMEDIATE) : D3DPRESENT_INTERVAL_IMMEDIATE;
#else
	s_d3dpp.PresentationInterval = (sm_CurrentWindows[g_RenderThreadIndex].uFrameLock ? (1 << (sm_CurrentWindows[g_RenderThreadIndex].uFrameLock-1)) : D3DPRESENT_INTERVAL_IMMEDIATE); 
#endif
#endif	// __XENON

	s_d3dpp.BackBufferCount = 1;
	GetMultiSample(*(u32*)&s_d3dpp.MultiSampleType, *(u32*)&s_d3dpp.MultiSampleQuality);
	s_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	s_d3dpp.hDeviceWindow = g_hwndMain;
	s_d3dpp.Windowed = g_inWindow ? true : false;
#if __WIN32PC || RSG_DURANGO
	s_d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8; // Need to expose conversion from Rage format to D3D
	s_d3dpp.EnableAutoDepthStencil = PARAM_autodepthstencil.Get() ? TRUE : FALSE; // For Deferred Renders that need to sample from Z for DX9 this should be FALSE
#else	
	s_d3dpp.AutoDepthStencilFormat = g_grcDepthFormat;
#endif
	s_d3dpp.Flags = 0;
	s_d3dpp.FullScreen_RefreshRateInHz = (g_inWindow ? D3DPRESENT_RATE_DEFAULT : sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate);

	g_WindowWidth = s_d3dpp.BackBufferWidth;
	g_WindowHeight = s_d3dpp.BackBufferHeight;
	g_InvWindowWidth = 1.0f / float(s_d3dpp.BackBufferWidth);
	g_InvWindowHeight = 1.0f / float(s_d3dpp.BackBufferHeight);

#if __XENON
	if (s_PrimaryRingBufferSize && s_SecondaryRingBufferSize) {
		D3DRING_BUFFER_PARAMETERS params;
		memset(&params,0,sizeof(params));
		params.PrimarySize = s_PrimaryRingBufferSize;
		params.SecondarySize = s_SecondaryRingBufferSize;
		params.SegmentCount = s_NumSegments;
		memcpy(&s_d3dpp.RingBufferParameters,&params,sizeof (D3DRING_BUFFER_PARAMETERS));
	}
#endif
}

#if __WIN32PC || RSG_DURANGO
PARAM(width,"[grcore] Set width of main render window (default is 640)");
PARAM(height,"[grcore] Set height of main render window (default is 480)");
PARAM(refreshrate,"[grcore] Set refresh rate of main render window");
PARAM(unmanaged, "[grcore] Set resource managed to self-managed"); // Use unmanaged resource pool
PARAM(noBlockOnLostFocus,"[grcore] Don't block the window update when it loses focus.");
PARAM(setHwndMain,"[grcore] override the window that DirectX will render to");
NOSTRIP_PC_PARAM(adapter,"[grcore] Use the specified screen adapter number (zero-based)");
PARAM(displaycaps,"[grcore] Show display capabilities");
#endif

#if __XENON
NOSTRIP_PARAM(width,"[grcore] Set width of main render window (default is 640)");
NOSTRIP_PARAM(height,"[grcore] Set height of main render window (default is 480)");
PARAM(noshaderpatching,"[grcore] Tell D3D to assert out if it finds avoidable shader patching.");
#endif

#if __WIN32PC
void grcDevice::GetAdapterDescription(DXGI_ADAPTER_DESC &/*oAdapterDesc*/)
{
	return;
}

void grcDevice::InitAdapterOrdinal() 
{
	sm_AdapterOrdinal = 0;
}

void grcDevice::SetAdapterOrdinal(int ordinal) 
{
	sm_AdapterOrdinal = ordinal;
}


int grcDevice::GetAdapterOrdinal() 
{
	return sm_AdapterOrdinal;
}

void grcDevice::SetOutputMonitor(int monitor)
{
	sm_OutputMonitor = monitor;
}

int grcDevice::GetOutputMonitor()
{
	return sm_OutputMonitor;
}

int grcDevice::GetAdapterCount()
{
#if __RESOURCECOMPILER
	return 0;
#else
	if (!g_Direct3DCreate9)
		Quitf("Direct3D 9 not found, please reinstall app.");

	LPDIRECT3D9 temp = g_Direct3DCreate9(D3D_SDK_VERSION);
	if (!temp)
		return 0;
	else {
		int result = 0;
		result = temp->GetAdapterCount();
		temp->Release();
		return result;
	}
#endif
}

int grcDevice::GetDisplayModeCount(int ordinal)
{
	if (!g_Direct3DCreate9)
		Quitf("Direct3D 9 not found, please reinstall app.");

	LPDIRECT3D9 pD3DDevice = g_Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3DDevice)
		return 0;

	// PC TODO - Should add support for different formats
	int result = 0;
	result = pD3DDevice->GetAdapterModeCount( ordinal, D3DFMT_A8R8G8B8 );
	pD3DDevice->Release();
	return result;
}

bool grcDevice::GetDisplayMode(int ordinal, int mode, int &width, int &height, int &refreshrate)
{
	if (!g_Direct3DCreate9)
	Quitf("Direct3D 9 not found, please reinstall app.");

	LPDIRECT3D9 pD3DDevice = g_Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3DDevice)
		return 0;

	width = 0;
	height = 0;
	refreshrate = 0;

	// PC TODO - Should add support for different formats
	D3DDISPLAYMODE displayMode;

	HRESULT hr;
	hr = pD3DDevice->EnumAdapterModes( ordinal, D3DFMT_A8R8G8B8, mode, &displayMode );
	if (hr == D3D_OK)
	{
		width = displayMode.Width;
		height = displayMode.Height;
		refreshrate = displayMode.RefreshRate;
	}
	pD3DDevice->Release();
	return (hr == D3D_OK) ? true : false;
}

#if !__D3D11

#pragma warning (disable : 4995)
#pragma warning (disable : 4996)
#if defined(__INTEL_COMPILER)
#pragma warning (disable : 1786)
#endif // defined(__INTEL_COMPILER)

int grcDevice::SearchFor( const u16* pwszSearchName, u16* pwszResult, u16* wszParentName, IDxDiagContainer* pDxDiagContainer )
{
    HRESULT hr;      
    IDxDiagContainer* pChildContainer = NULL;
    VARIANT var;
    VariantInit( &var );

	DWORD dwPropCount;
    hr = pDxDiagContainer->GetNumberOfProps( &dwPropCount );
    if( SUCCEEDED( hr ) )
    {
        // Print each property in this container
        for( DWORD dwPropIndex = 0; dwPropIndex < dwPropCount; dwPropIndex++ )
        {
			WCHAR wszPropName[256];
			WCHAR wszPropValue[256];
            hr = pDxDiagContainer->EnumPropNames( dwPropIndex, wszPropName, 256 );
            if( SUCCEEDED( hr ) )
            {
                hr = pDxDiagContainer->GetProp( wszPropName, &var );
                if( SUCCEEDED( hr ) )
                {
                    // Switch off the type.  There's 4 different types:
                    switch( var.vt )
                    {
                        case VT_UI4:
                            StringCchPrintfW( wszPropValue, 256, L"%d", var.ulVal );
                            break;
                        case VT_I4:
                            StringCchPrintfW( wszPropValue, 256, L"%d", var.lVal );
                            break;
                        case VT_BOOL:
                            StringCchCopyW( wszPropValue, 256, ( var.boolVal ) ? L"true" : L"false" );
                            break;
                        case VT_BSTR:
                            StringCchCopyW( wszPropValue, 256, var.bstrVal );
                            break;
                    }

                    // Add the parent name to the front if there's one, so that
                    // its easier to read on the screen
					WCHAR wszFullName[256];
                    if( wszParentName )
					{
						swprintf( wszFullName, L"%s.%s", wszParentName, wszPropName );
                        //wprintf( L"%s.%s = %s\n", wszParentName, wszPropName, wszPropValue );
					}
					else
					{
						swprintf( wszFullName, L"%s", wszPropName );
                        //wprintf( L"%s = %s\n", wszPropName, wszPropValue );
					}
					
					if (!wcscmp((const wchar_t*)pwszSearchName, wszFullName))
					{
						wcscpy((wchar_t*)pwszResult, wszPropValue);
						VariantClear( &var );
						return S_OK;
					}

                    // Clear the variant (this is needed to free BSTR memory)
                    VariantClear( &var );
                }
            }
        }
    }

    // Recursivly call this function for each of its child containers
    DWORD dwChildCount;
    hr = pDxDiagContainer->GetNumberOfChildContainers( &dwChildCount );
    if( SUCCEEDED( hr ) )
    {
        for( DWORD dwChildIndex = 0; dwChildIndex < dwChildCount; dwChildIndex++ )
        {
			WCHAR wszChildName[256];
            hr = pDxDiagContainer->EnumChildContainerNames( dwChildIndex, wszChildName, 256 );
            if( SUCCEEDED( hr ) )
            {
                hr = pDxDiagContainer->GetChildContainer( wszChildName, &pChildContainer );
                if( SUCCEEDED( hr ) )
                {
                    // wszFullChildName isn't needed but is used for text output
                    WCHAR wszFullChildName[256];
                    if( wszParentName )
                        StringCchPrintfW( wszFullChildName, 256, L"%s.%s", wszParentName, wszChildName );
                    else
                        StringCchCopyW( wszFullChildName, 256, wszChildName );
                    SearchFor( pwszSearchName, pwszResult, (u16*)&wszFullChildName[0], pChildContainer );

					if(pChildContainer) 
					{ 
						(pChildContainer)->Release(); 
						(pChildContainer)=NULL; 
					}                     
					if (pwszResult[0] != NULL)
						break; // Found
                }
            }
        }
    }

    return S_OK;
}

bool grcDevice::Query(u16* pszData, const u16* pszField)
{
	pszData[0] = NULL;

	// Used to search DXDiag for info we need - Warning very slow operation
    HRESULT hr;

    CoInitialize( NULL );

    IDxDiagProvider* pDxDiagProvider = NULL;
    IDxDiagContainer* pDxDiagRoot = NULL;

    // CoCreate a IDxDiagProvider*
    hr = CoCreateInstance( CLSID_DxDiagProvider,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IDxDiagProvider,
                           ( LPVOID* )&pDxDiagProvider );
    if( SUCCEEDED( hr ) ) // if FAILED(hr) then DirectX 9 is not installed
    {
        // Fill out a DXDIAG_INIT_PARAMS struct and pass it to IDxDiagContainer::Initialize
        // Passing in TRUE for bAllowWHQLChecks, allows dxdiag to check if drivers are
        // digital signed as logo'd by WHQL which may connect via internet to update
        // WHQL certificates.
        DXDIAG_INIT_PARAMS dxDiagInitParam;
        ZeroMemory( &dxDiagInitParam, sizeof( DXDIAG_INIT_PARAMS ) );

        dxDiagInitParam.dwSize = sizeof( DXDIAG_INIT_PARAMS );
        dxDiagInitParam.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
        dxDiagInitParam.bAllowWHQLChecks = TRUE;
        dxDiagInitParam.pReserved = NULL;

        hr = pDxDiagProvider->Initialize( &dxDiagInitParam );
        if( FAILED( hr ) )
            goto LCleanup;

        hr = pDxDiagProvider->GetRootContainer( &pDxDiagRoot );
        if( FAILED( hr ) )
            goto LCleanup;

        // This function will recursivly print the properties
        // the root node and all its child.
        hr = SearchFor( pszField, pszData, NULL, pDxDiagRoot );
        if( FAILED( hr ) )
            goto LCleanup;
    }

LCleanup:
    if(pDxDiagRoot) 
	{ 
		(pDxDiagRoot)->Release(); 
		(pDxDiagRoot)=NULL; 
	} 
    if(pDxDiagProvider) 
	{ 
		(pDxDiagProvider)->Release(); 
		(pDxDiagProvider)=NULL; 
	} 
    CoUninitialize();
	return (pszData[0] != NULL) ? true : false;
}
#endif // !__D3D11
#endif

bool grcDevice::GetWideScreen()
{
#if __XENON
	return sm_IsWideScreen;
#else
	// Verify thread ownership. [see GetWidth and GetHeight]
	return GetHeight()? ((float)GetWidth() / (float)GetHeight()) >= 1.5f : false;
#endif
}

bool grcDevice::GetHiDef()
{
#if __XENON
	return sm_IsHiDef;
#else
	return GetWidth() * GetHeight() >= (1280 * 720);
#endif
}

static bool s_WeCreatedHwndMain;
grcEffect *g_DefaultEffect;
grcEffectVar g_DefaultSampler;
grcEffectTechnique s_DefaultLit, s_DefaultUnlit, s_DefaultLitSkinned, s_DefaultUnlitSkinned, s_DefaultBlit;


#if __WIN32PC
IDirect3DVertexBuffer9 *s_BlitVB;
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
static int s_BlitOffset;

void grcDevice::Blit_Init() {
	GRCDEVICE.GetCurrent()->CreateVertexBuffer(BlitVertSize * sizeof(BlitVert),
		D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,D3DFVF_XYZW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1,D3DPOOL_DEFAULT,&s_BlitVB,0);
	s_BlitOffset = 0;
}

void grcDevice::Blit_Shutdown() {
	SAFE_RELEASE_RESOURCE(s_BlitVB);

#if __WIN32PC && USE_RESOURCE_CACHE
	grcResourceCache::ShutdownClass();
#endif // __WIN32PC
}
static RageDirect3DDevice9 s_DeviceWrapper;
#endif

#if __XENON
int grcCommandBufferHeapSize = 65536;
int grcCommandBufferHeapNodes = 4096;

static u8 *s_CommandHeapWorkspace;
static sysMemExternalAllocator *s_CommandHeap;
#endif

sysIpcCurrentThreadId grcDevice::sm_Owner = sysIpcCurrentThreadIdInvalid;


void grcDevice::InitClass(bool inWindow, bool topMost) 
{
	sm_Owner = sysIpcGetCurrentThreadId();
#if __WIN32PC
	sm_CurrentWindows[g_RenderThreadIndex].Init(1280,720,0,0,1);
#elif __XENON
	sm_CurrentWindows[g_RenderThreadIndex].Init(1,1,0,0,1);
#endif

#if __WIN32PC
	sm_CreationOwner = sysIpcGetCurrentThreadId();
	sm_TextureResourcePool = RESOURCE_UNMANAGED;
	sm_BufferResourcePool = RESOURCE_MANAGED;
	sm_Controller = sysIpcCreateSema(true);
#endif //__WIN32PC

#if __RESOURCECOMPILER
	Quitf("Shouldn't be starting graphics! (%d,%d)",inWindow,topMost);
#else
	u32 pixAnnotationMask;
	if (PARAM_pixannotation.Get(pixAnnotationMask))
		g_EnablePixAnnotation = (pixAnnotationMask);
#if __XENON
	// See if user wants 720p
	XVIDEO_MODE mode;
	XGetVideoMode(&mode);

	if (PARAM_hdr.Get())
		PARAM_colorexpbias.Get(sm_ColorExpBias);			// get the user set width and height 
	PARAM_width.Get(sm_CurrentWindows[g_RenderThreadIndex].uWidth);			// get the user set width and height 
	PARAM_height.Get(sm_CurrentWindows[g_RenderThreadIndex].uHeight);

	sm_DisplayWidth = mode.dwDisplayWidth;
	sm_DisplayHeight = mode.dwDisplayHeight;

	sm_IsWideScreen = ( mode.fIsWideScreen ) ? true : false;
	sm_IsHiDef = ( mode.fIsHiDef ) ? true : false;

	// clamp front/backbuffer resolution to have a max area of 1280*720
	if (mode.dwDisplayWidth * mode.dwDisplayHeight <= 1280*720)
	{
		if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1) 
			sm_CurrentWindows[g_RenderThreadIndex].uWidth = mode.dwDisplayWidth;
		if (sm_CurrentWindows[g_RenderThreadIndex].uHeight==1)
			sm_CurrentWindows[g_RenderThreadIndex].uHeight = mode.dwDisplayHeight;
	}
	else
	{
		if (sm_CurrentWindows[g_RenderThreadIndex].uWidth==1) 
			sm_CurrentWindows[g_RenderThreadIndex].uWidth = min(mode.dwDisplayWidth,1280);
		if (sm_CurrentWindows[g_RenderThreadIndex].uHeight==1)
			sm_CurrentWindows[g_RenderThreadIndex].uHeight = min(mode.dwDisplayHeight,720);
	}
	
#if defined(_DEBUG)
	if (PARAM_d3dsinglestep.Get())
		D3D__SingleStepper  = true;
#endif

#endif

#if __WIN32PC
#if __FINAL
	// Default to desktop resolution.
	int iScreenWidth  = GetSystemMetrics(SM_CXSCREEN);
	int iScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	if ((iScreenWidth > 0) && (iScreenHeight > 0))
	{
		sm_CurrentWindows[g_RenderThreadIndex].uWidth  = iScreenWidth;
		sm_CurrentWindows[g_RenderThreadIndex].uHeight = iScreenHeight;
		sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate = 0;
	}
#endif // __FINAL

	PARAM_width.Get(sm_CurrentWindows[g_RenderThreadIndex].uWidth);
	PARAM_height.Get(sm_CurrentWindows[g_RenderThreadIndex].uHeight);
	PARAM_refreshrate.Get(sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate);
	sm_BlockOnLostFocus = !PARAM_noBlockOnLostFocus.Get();

	sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex];

	sm_FullscreenWindow = sm_CurrentWindows[g_RenderThreadIndex];
	sm_FullscreenWindow.bFullscreen = true;

	if ((GetVersion() & 255) < 6 &&  GetSystemMetrics(SM_REMOTESESSION)) {
		grcErrorf("Cannot create D3D device when using Remote Desktop.");
		ExitProcess(3);
	}
#else
	inWindow = false;
	topMost = false;
#endif

	g_inWindow = inWindow;
	g_isTopMost = topMost;
	sm_CurrentWindows[g_RenderThreadIndex].bFullscreen = !g_inWindow;

#if __WIN32PC
	// PC: disable noBlockOnLostFocus when in fullscreen
	if (g_inWindow == false)
	{
		sm_BlockOnLostFocus = true;
	}

	if (!g_Direct3DCreate9)
		Quitf("Direct3D 9 not found, please reinstall app.");
	s_D3D9 = g_Direct3DCreate9(D3D_SDK_VERSION);
#else
	s_D3D9 = Direct3DCreate9(D3D_SDK_VERSION);
#endif
	if (!s_D3D9)
		Quitf("Direct3DCreate9 failed.  Do you have DirectX 9c (June 2005 Update) installed? (directx_9c_redist.exe)");

#if !__FINAL && __WIN32PC
	if ( PARAM_debugshaders.Get() ) {
		s_DevType = D3DDEVTYPE_REF;
		PARAM_nohwtnl.Set("");
	}
#endif
		
#if __WIN32PC
	PARAM_setHwndMain.Get((int&)g_hwndMain);
	if (!g_hwndMain || PARAM_ragUseOwnWindow.Get()) {
		g_hwndMain = CreateDeviceWindow(g_hwndParent);
		s_WeCreatedHwndMain = true;
		SetupCursor(false);
	}
#endif

	// Allow overrides of our default framelock
	SetFrameLock(sm_CurrentWindows[g_RenderThreadIndex].uFrameLock, s_SwapImmediateIfLate);
	UpdatePresentParameters();

	// IMPORTANT!!!!! Messing with floating point state in the past
	// has broken Windows 98 builds.  Not sure how relevant that is
	// these days.
	u32 extraFlags = 0; //D3DCREATE_FPU_PRESERVE; // | D3DCREATE_DISABLE_DRIVER_MANAGEMENT_EX;
#if __WIN32PC
// # if __DEV
// 	extraFlags = D3DCREATE_FPU_PRESERVE;
// # endif
	if ( PARAM_d3dfpu.Get() ) {
		extraFlags |= D3DCREATE_FPU_PRESERVE;
	}
//	else if ( PARAM_nod3dfpu.Get() ) {
//		extraFlags &= ~D3DCREATE_FPU_PRESERVE;
//	}
#if !__FINAL && 0
	if ( PARAM_d3dmt.Get() )
#endif
	{
		extraFlags |= D3DCREATE_MULTITHREADED;
	}
#if __TOOL 
	extraFlags |= D3DCREATE_MULTITHREADED;
#endif	// __TOOL

	//extraFlags |= D3DCREATE_DISABLE_DRIVER_MANAGEMENT;

	PARAM_adapter.Get(sm_AdapterOrdinal);
	if (sm_AdapterOrdinal < 0 || sm_AdapterOrdinal >= (int)s_D3D9->GetAdapterCount()) {
		grcErrorf("Invalid adapter ordinal specified, using first display");
		sm_AdapterOrdinal = 0;
	}
#else
	const int sm_AdapterOrdinal = 0;
#endif	// __WIN32PC
#if __XENON
	if (s_BufferTwoFrames)
		extraFlags |= D3DCREATE_BUFFER_2_FRAMES;

#if HACK_GTA4
	extraFlags |= D3DCREATE_ASYNCHRONOUS_SWAPS;
#endif

# ifndef D3DCREATE_NO_SHADER_PATCHING
# define D3DCREATE_NO_SHADER_PATCHING	0
# else
	if (PARAM_noshaderpatching.Get())
		extraFlags |= D3DCREATE_NO_SHADER_PATCHING;
# endif
#endif

#if	__WIN32PC && !__FINAL
	if	(PARAM_perfhud.Get())
	{
        s_DevType = D3DDEVTYPE_REF;
		sm_AdapterOrdinal = s_D3D9->GetAdapterCount() - 1;
	}
	else
	{
		UINT count = s_D3D9->GetAdapterCount();
		for (UINT i = 0; i<count; i++)
		{
			D3DADAPTER_IDENTIFIER9 ai;
			if (SUCCEEDED(s_D3D9->GetAdapterIdentifier(i, 0, &ai)) && (!strcmp(ai.Description, "NVIDIA NVPerfHUD") || !strcmp(ai.Description, "NVIDIA PerfHUD")))
			{
				grcDisplayf("Detected NVPerfHUD!");
		        s_DevType = D3DDEVTYPE_REF;
				sm_AdapterOrdinal = i;
				break;
			}
		}
	}
#endif

	// Allow overrides of our default framelock
	SetFrameLock(sm_CurrentWindows[g_RenderThreadIndex].uFrameLock, s_SwapImmediateIfLate);
	UpdatePresentParameters();

	bool bFailed = true;
	// Disable for Debug Runtime - Live requires getstate functionality
#if !__FINAL
	bFailed = PARAM_nohwtnl.Get() || (FAILED(s_D3D9->CreateDevice(sm_AdapterOrdinal, s_DevType, g_hwndMain, D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING | extraFlags, &s_d3dpp, &sm_Current)));
#else
	bFailed = FAILED(s_D3D9->CreateDevice(sm_AdapterOrdinal, s_DevType, g_hwndMain, D3DCREATE_PUREDEVICE | extraFlags, &s_d3dpp, &sm_Current));
#endif

#if __WIN32PC
	if (bFailed && !PARAM_nohwtnl.Get())
	{
		bFailed = FAILED(s_D3D9->CreateDevice(sm_AdapterOrdinal, s_DevType, g_hwndMain, D3DCREATE_HARDWARE_VERTEXPROCESSING | extraFlags, &s_d3dpp, &sm_Current));
	}
#endif
	sm_HardwareShaders = !bFailed;

#if __XENON
	sm_Primary = sm_Current;
#if _XDK_VER >= 2638
	CHECK_HRESULT(s_D3D9->CreateDevice(sm_AdapterOrdinal, D3DDEVTYPE_COMMAND_BUFFER, NULL, D3DCREATE_NO_SHADER_PATCHING, NULL, &sm_Command));
	Assert(sm_Command);
#endif
#endif

#if __WIN32PC
	if (bFailed && !PARAM_nohwtnl.Get())
	{
		HRESULT hr = s_D3D9->CreateDevice(sm_AdapterOrdinal, s_DevType, g_hwndMain, D3DCREATE_MIXED_VERTEXPROCESSING | extraFlags, &s_d3dpp, &sm_Current);
		bFailed = FAILED(hr);
		switch( hr )
		{
			case D3DERR_DEVICELOST: grcErrorf("Device lost"); break;
			case D3DERR_INVALIDCALL: grcErrorf("Invalid call "); break;
			case D3DERR_NOTAVAILABLE: grcErrorf("Device not available"); break;
			case D3DERR_OUTOFVIDEOMEMORY: grcErrorf("Not enough video memory"); break;
			default : grcErrorf("Unknown error"); break;
		}
	}
#endif

	sm_HardwareTransform = !bFailed;

#if __WIN32PC
	if (bFailed)
	{
		HRESULT hr = s_D3D9->CreateDevice(sm_AdapterOrdinal, s_DevType, g_hwndMain, D3DCREATE_SOFTWARE_VERTEXPROCESSING | extraFlags, &s_d3dpp, &sm_Current);
		bFailed = FAILED(hr);
		switch( hr )
		{
			case D3DERR_DEVICELOST: grcErrorf("Device lost"); break;
			case D3DERR_INVALIDCALL: grcErrorf("Invalid call "); break;
			case D3DERR_NOTAVAILABLE: grcErrorf("Device not available"); break;
			case D3DERR_OUTOFVIDEOMEMORY: grcErrorf("Not enough vido memory"); break;
			default : grcErrorf("Unknown error"); break;
		}
	}
#endif

	if (bFailed)
	{
#if __WIN32PC
		Quitf("Unable to create D3D device.  Try to resolve this problem by:\n\n"
			"1.  Making sure that the desktop bit depth is set to 32 bit color.\n"
			"2.  Closing Maya, 3D Studio Max, or any other application that is using 3D graphics."
			);
#else
		Quitf("Unable to create D3D device.  Try setting desktop to 32 bit color?");
#endif
	}

#if __XENON
	AllocateSwapChain();
	CHECK_HRESULT(sm_Current->SetRenderTarget(0, g_BackBuffer));
	CHECK_HRESULT(sm_Current->SetDepthStencilSurface(g_DepthStencil));

#if HACK_GTA4
	CHECK_HRESULT(sm_Current->SetSwapCallback(ASYNC_SWAP_D3D_CB));
	CHECK_HRESULT(sm_Current->SetVerticalBlankCallback(VBLANK_D3D_CB));
#endif

// 	sm_Command->SetRenderTarget(0, g_BackBuffer);
//	sm_Command->SetDepthStencilSurface(g_DepthStencil);
#endif

#if __WIN32PC
	///s_DeviceWrapper.m_Inner = sm_Current;
	///sm_Current = &s_DeviceWrapper;

	AllocateSwapChain();

	D3DCAPS9 caps;
	if (FAILED(sm_Current->GetDeviceCaps(&caps)))
		Quitf("Unable to retrieve device caps");
	int vsVersion = LOWORD(caps.VertexShaderVersion), psVersion = LOWORD(caps.PixelShaderVersion);
	if (psVersion < 0x300)
		Quitf("DX9 Hardware must support 3.0 pixel shaders");
	if (vsVersion < 0x300)
		Quitf("DX9 Hardware must support 3.0 vertex shaders");
	if (PARAM_displaycaps.Get())
		grcDisplayf("VS v%x (%d const), PS v%x",vsVersion,caps.MaxVertexShaderConst,psVersion);

	ZeroMemory( &s_WindowedPlacement, sizeof( WINDOWPLACEMENT ) );
	s_WindowedPlacement.length = sizeof( WINDOWPLACEMENT );
	GetWindowPlacement( g_hwndMain, &s_WindowedPlacement );
#endif

	g_grcCurrentContext = sm_Current;
	
#if !__XENON
	CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_SPECULARENABLE, TRUE));	// so that oD1 will work properly
#endif

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

	sm_BlitDecl = CreateVertexDeclaration(blitElements, sizeof(blitElements) / sizeof(grcVertexElement));
	sm_ImDecl = CreateVertexDeclaration(imElements, sizeof(imElements) / sizeof(grcVertexElement));

	grcEffect::InitClass();
	grcInitQuads();
	grcStateBlock::InitClass();

	g_DefaultEffect = grcEffect::Create(sm_DefaultEffectName);
	if (!g_DefaultEffect)
		Quitf("Unable to create default effect '%s', cannot continue.",sm_DefaultEffectName);

	s_DefaultLit = g_DefaultEffect->LookupTechnique("draw");
	s_DefaultUnlit = g_DefaultEffect->LookupTechnique("unlit_draw");
	s_DefaultLitSkinned = g_DefaultEffect->LookupTechnique("drawskinned");
	s_DefaultUnlitSkinned = g_DefaultEffect->LookupTechnique("unlit_drawskinned");
	s_DefaultBlit = g_DefaultEffect->LookupTechnique("blit_draw");
	g_DefaultSampler = g_DefaultEffect->LookupVar("DiffuseTex");

	grcViewport::InitClass();

#if __WIN32PC
	DeviceRestored();
	InitializeFeatureSet();
#endif	//__WIN32PC

#if __XENON
	s_CommandHeapWorkspace = rage_new u8[COMPUTE_WORKSPACE_SIZE(grcCommandBufferHeapNodes)];
	++sysMemoryFillWord;	// defeat bogus optimization down in debug_memory_fill, sigh (weird case, nothing else uses write-combined memory)
	s_CommandHeap = rage_new sysMemExternalAllocator(grcCommandBufferHeapSize, XPhysicalAlloc(grcCommandBufferHeapSize,MAXULONG_PTR,0,PAGE_READWRITE | PAGE_WRITECOMBINE),grcCommandBufferHeapNodes,s_CommandHeapWorkspace);
	--sysMemoryFillWord;
#endif // __XENON
#endif	// !__RESOURCECOMPILER
}


void grcDevice::SetDefaultEffect(bool isLit,bool isSkinned) {
	grcEffectTechnique tech = isLit? (isSkinned? s_DefaultLitSkinned : s_DefaultLit) : (isSkinned? s_DefaultUnlitSkinned : s_DefaultUnlit);
	g_DefaultEffect->Bind(tech);
}

#if __WIN32PC
void grcDevice::UpdateStereoTexture(const Vector3& /*vCamOffset*/,const Vector3& /*vCamOffset1*/)
{
}

void grcDevice::DeInitStereo()
{
	return;
}

bool grcDevice::InitializeStereo()
{
	return false;
}

void grcDevice::SetStereoTexture()
{
	return;
}

void grcDevice::ForceStereorizedRT(Stereo_t bForce)
{
	(void)bForce;
}

grcEntry void  SetDefaultConvergenceDistance(float /*fConv*/) {}

void grcDevice::SetConvergenceDistance(float) {}

void grcDevice::SetSeparationPercentage(float /*fSepPercentage*/, bool, bool) {}

float grcDevice::GetSeparationPercentage(bool bUpdate)
{
	(void)bUpdate;
	return sm_fStereoSeparationPercentage;
}
/*
float grcDevice::GetDefaultSeparationPercentage()
{
	return sm_fStereoSeparationPercentage;
}
*/
float grcDevice::GetConvergenceDistance()
{
	return sm_fStereoConvergence;
}

float grcDevice::GetEyeSeparation()
{	
	return sm_fEyeSeparation;
}

#endif


grcEffect& grcDevice::GetDefaultEffect()
{
	return *g_DefaultEffect;
}

void grcDevice::SetFrameLock(int frameLock,bool swapImmediateIfLate) {
	PARAM_frameLimit.Get(frameLock);

	// why is this assert here? the doc do not mention this requirement...
	// AssertMsg(frameLock < 2 || swapImmediateIfLate,"Framelock of 2 must swap immediately if late");
	Assert(frameLock <= 4);	// Could support more easily enough (presentation interval is a bitmask, not an enum)
	sm_CurrentWindows[g_RenderThreadIndex].uFrameLock = frameLock;
	s_SwapImmediateIfLate = swapImmediateIfLate;
#if __XENON && _XDK_VER < 1871
	// This is temporary and is a documented restriction in the XeDK
	s_SwapImmediateIfLate = true;
#endif
	if (sm_Current) {
#if __XENON || __WIN32PC
		FreeSwapChain();
#endif
		UpdatePresentParameters();
		CHECK_HRESULT(sm_Current->Reset(&s_d3dpp));
#if __WIN32PC
		if (sm_ResetCallback)
		{
			sm_ResetCallback(sm_Current,&s_d3dpp);
		}
#endif
#if __XENON || __WIN32PC
		AllocateSwapChain();
#endif
	}
}

void grcDevice::SetFrameLockNoReset(int frameLock)
{ 
	PARAM_frameLimit.Get(frameLock);

	Assert(frameLock <= 4);
	sm_CurrentWindows[g_RenderThreadIndex].uFrameLock = frameLock; 
}

void grcDevice::SetLetterBox(bool enable)
{
	sm_LetterBox = enable;
	if (sm_Current)
	{
	UpdatePresentParameters();
	CHECK_HRESULT(sm_Current->Reset(&s_d3dpp));
	}
#if __WIN32PC
	if (sm_ResetCallback)
		sm_ResetCallback(sm_Current,&s_d3dpp);
#endif
}

int grcDevice::GetFrameLock() {
	return sm_CurrentWindows[g_RenderThreadIndex].uFrameLock;
}

bool grcDevice::IsMessagePumpThreadThatCreatedTheD3DDevice()
{
	return (sm_CreationOwner == sysIpcGetCurrentThreadId());
}

#define PRESERVE_HIZ		1

#if __XENON && _XDK_VER >= 2638 && COMMAND_BUFFER_SUPPORT
void grcDevice::CreateCommandBuffer(size_t size,D3DCommandBuffer **pbuffer) {
	sm_Primary->CreateCommandBuffer(size,0,pbuffer);
}


void grcDevice::DeleteCommandBuffer(D3DCommandBuffer *buffer) {
	if (buffer) {
		if (buffer->Common & D3DCOMMON_D3DCREATED)
			buffer->Release();
		else {
			s_CommandHeap->Free((void*)buffer->Identifier);
			delete (char*) buffer;
		}
	}
}

extern u64 g_VertexShaderConstants, g_PixelShaderConstants, g_SamplerConstants;

/* static void DumpConstants(const char *tag,u64 bits) {
	int offset = 0;
	while (bits) {
		if ((s64)bits<0)
			grcDisplayf("%s constant block %d-%d inherited.",tag,offset,offset+3);
		bits<<=1;
		offset+=4;
	}
} */

#define DUMP_COMMAND_BUFFERS	0

#if DUMP_COMMAND_BUFFERS
void DecodeReg(char *dest,int negate,int swizzle,int regConst,int select,int mask) {
	bool abs = false; // , sat = false;
	if (negate)
		*dest++ = '-';
	if (select) {
		*dest++ = 'r';
		if (regConst & 128) {
			regConst &= 127;
			abs = true;
		}
	}
	else
		*dest++ = 'C';
	sprintf(dest,"%d%s",regConst,abs?"_abs":"");
	dest += strlen(dest);
	if (mask)
		*dest++ = '.';
	static const char swiz[] = "xyzwxyzw";
	if (mask & 1)
		*dest++ = swiz[(swizzle>>0)&3];
	if (mask & 2)
		*dest++ = swiz[((swizzle>>2)&3)+1];
	if (mask & 4)
		*dest++ = swiz[((swizzle>>4)&3)+2];
	if (mask & 8)
		*dest++ = swiz[((swizzle>>6)&3)+3];
	*dest = '\0';
}

void DisassembleBlock(GPUFLOW_INSTRUCTION &flow,const GPUSHADER_INSTRUCTION *start) {
	static const char *scalarops[64] = {
		"adds", "addprevs", "muls", "mulprevs",
		"mulprev2s", "maxs", "mins", "seq",
		"sgt", "sge", "sne", "frc",
		"trunc", "floor", "exp", "logc",
		"log", "rcpc", "rcpf", "rcp",
		"rsqc", "rsqf", "rsq", "maxa",
		"maxaf", "sub", "subprev", "setpeq",
		"setpne", "setpgt", "setpge", "setpinv",
		"setppop", "setpclr", "setprstr", "killeq",
		"killgt", "killge", "killne", "killone",
		"sqrt", "???41", "mulc0", "mulc1", 
		"addc0", "addc1", "subc0", "subc1", 
		"sin", "cos", "retainprev", "???51",
		"???52", "???53", "???54", "???55",
		"???56", "???57", "???58", "???59",
		"???60", "???61", "???62", "???63"
	};
	static const char *vectorops[32] = {
		"add", "mul", "max", "min",
		"seq", "sgt", "sge", "sne",
		"frc", "trunc", "floor", "mad",
		"cndeq", "cndge", "cndgt", "dp4",
		"dp3", "dp2add", "cube", "max4",
		"setpeqp", "setpnep", "setpgtp", "setpgep",
		"killeq", "killgt", "killge", "killne",
		"dst", "maxa", "???30", "???31"
	};
	static const char *masks[16] = { 
		"none", "x", "y", "xy",
		"z", "xz", "yz", "xyz",
		"w", "xw", "yw", "xyw",
		"wz", "xwz", "yzw", "xyzw" };
	static const char *allocs[] = {"???","position","interpolators/colors","export" };

	static unsigned char vector_ops_num_src[] = {
			2,  //add
			2,  //mul
			2,  //max
			2,  //min
			2,  //sete
			2,  //setgt
			2,  //setge
			2,  //setne
			1,  //fract
			1,  //trunc
			1,  //floor
			3,  //muladd
			3,  //cnde
			3,  //cndge
			3,  //cndgt
			2,  //dot4
			2,  //dot3
			3,  //dot2add
			2,  //cube
			1,  //max4
			2,  //pred_sete_push
			2,  //pred_set_ne_push
			2,  //pred_set_gt_push
			2,  //pred_set_ge_push
			2,  //kille
			2,  //killgt
			2,  //killge
			2,  //killne
			2,  //dst
			2,  //maxa
	};

	static unsigned char scalar_ops_num_src[] = {
			1,  //add
			1,  //add_prev
			1,  //mul
			1,  //mul_prev
			1,  //mul_prev2
			1,  //max
			1,  //min
			1,  //sete
			1,  //setgt
			1,  //setge
			1,  //setne
			1,  //fract
			1,  //trunc
			1,  //floor
			1,  //exp
			1,  //log_ieee
			1,  //log_clamp
			1,  //recip_ieee
			1,  //recip_clamp
			1,  //recip_ff
			1,  //recipsqrt_ieee
			1,  //recipsqrt_clamp
			1,  //recipsqrt_ff
			1,  //mova
			1,  //mova_floor
			1,  //sub
			1,  //sub_prev
			1,  //pred_sete
			1,  //pred_setne
			1,  //pred_setgt
			1,  //pred_setge
			1,  //pred_set_inv
			1,  //pred_set_pop
			0,  //pred_set_clr
			1,  //pred_set_restore
			1,  //kille
			1,  //killgt
			1,  //killge
			1,  //killne
			1,  //killone
			1,  //sqrt,
			0,  //opcode_41
			2,  //mul_const_0
			2,  //mul_const_0
			2,  //add_const_0
			2,  //add_const_1
			2,  //sub_const_1
			2,  //sbu_const_1
			1,  //sin,
			1   //cos
	};

	// grcDisplayf("flow = %p, addr = %x",&flow,flow.Exec.Address);

	switch (flow.Op) {
		case GPUFLOWOP_NOP: grcDisplayf("nop"); break;
		case GPUFLOWOP_EXEC: grcDisplayf("exec"); break;
		case GPUFLOWOP_EXEC_END: grcDisplayf("exece"); break;
		case GPUFLOWOP_COND_EXEC_PRED_CLEAN: grcDisplayf("cexec %cb%d","! "[flow.CondExec.Condition],flow.CondExec.BooleanAddress); break;
		case GPUFLOWOP_COND_EXEC_PRED_CLEAN_END: grcDisplayf("cexece %cb%d","! "[flow.CondExec.Condition],flow.CondExec.BooleanAddress); break;
		case GPUFLOWOP_ALLOC: grcDisplayf("alloc %s",allocs[flow.Alloc.BufferSelect]); break;
		default: grcDisplayf("???"); break;
	}
	int types = flow.Exec.TypeAndSerialize;
	const GPUSHADER_INSTRUCTION *base = start + flow.Exec.Address;
	for (DWORD j=0; j<flow.Exec.Count; j++, types >>= 2) {
		// grcDisplayf("%p: %8x %8x %8x",insns,insns[0],insns[1],insns[2]);
		if (types & GPUEXEC_SERIALIZE_MASK)
			grcDisplayf("    serialize");
		if (types & GPUEXEC_TYPE_MASK) {	// fetch
			if (base[j].VertexFetch.Op == GPUVERTEXFETCHOP_FETCH_VERTEX) {
				grcDisplayf("    vfetch [stride=%d offset=%d] r%d, r%d, vf%d",base[j].VertexFetch.Stride,base[j].VertexFetch.Offset,base[j].VertexFetch.DestGPR,base[j].VertexFetch.SrcGPR,base[j].VertexFetch.ConstIndex);
			}
			else
				grcDisplayf("    tfetch r%d, r%d, tf%d",base[j].TextureFetch.DestGPR,base[j].TextureFetch.SrcGPR,base[j].TextureFetch.ConstIndex);
		}
		else {
			char regA[32], regB[32], regC[32];
			const GPUALU_INSTRUCTION *i = &base[j].Alu;
			DecodeReg(regA,i->SrcANegate,i->SrcASwizzle,i->SrcARegConst,i->SrcASelect,i->VectorMask);
			DecodeReg(regB,i->SrcBNegate,i->SrcBSwizzle,i->SrcBRegConst,i->SrcBSelect,i->VectorMask);
			DecodeReg(regC,i->SrcCNegate,i->SrcCSwizzle,i->SrcCRegConst,i->SrcCSelect,
				vector_ops_num_src[i->VectorOp]==3? i->VectorMask:8);
			if (true) {
				int srcCount = vector_ops_num_src[i->VectorOp];
				if (i->VectorOp == GPUALUVECTOROP_MAX && i->VectorMask == 0)
					grcDisplayf("    nop");
				else if (i->VectorOp == GPUALUVECTOROP_MAX && i->SrcARegConst == i->SrcBRegConst &&
					i->SrcASelect == i->SrcBSelect && i->SrcANegate == i->SrcBNegate)
					grcDisplayf("    mov%s r%d.%s, %s",i->VectorSaturate?"_sat":"",i->VectorDest,masks[i->VectorMask], regA);
				else if (srcCount == 1)
					grcDisplayf ("    %s%s r%d.%s, %s",vectorops[i->VectorOp],i->VectorSaturate?"_sat":"",i->VectorDest,masks[i->VectorMask], regA);
				else if (srcCount == 2)
					grcDisplayf ("    %s%s r%d.%s, %s, %s",vectorops[i->VectorOp],i->VectorSaturate?"_sat":"",i->VectorDest,masks[i->VectorMask], regA, regB);
				else if (srcCount == 3)
					grcDisplayf ("    %s%s r%d.%s, %s, %s, %s",vectorops[i->VectorOp],i->VectorSaturate?"_sat":"",i->VectorDest,masks[i->VectorMask], regA, regB, regC);
			}
			if (i->ScalarOp != GPUALUSCALAROP_RETAINPREV) {
				int srcCount = scalar_ops_num_src[i->ScalarOp];
				if (srcCount == 0)
					grcDisplayf("    + %s%s",scalarops[i->ScalarOp],i->ScalarSaturate?"_sat":"");
				else if (srcCount == 1)
					grcDisplayf("    + %s%s r%d.%s, %s",scalarops[i->ScalarOp],i->ScalarSaturate?"_sat":"",i->ScalarDest,masks[i->ScalarMask], regC);
				else if (srcCount == 2) {
					// can't be a constant or be negated
					// TODO: It appears that the swizzle on the final operand matches the destination mask
					grcDisplayf("    + %s%s r%d.%s, %s, r%d",scalarops[i->ScalarOp],i->ScalarSaturate?"_sat":"",i->ScalarDest,masks[i->ScalarMask], regC, GPU_GET_SCALAROP_SOURCE2_REG(i));
				}
			}
		}
	}
}

void DisassembleShader(u32 *insns,int ASSERT_ONLY(size) /*in words*/) {
	/*char fbuf[64];
	sprintf(fbuf,"t:\\shader.%x.raw",insns);
	fiStream *S = fiStream::Create(fbuf);
	S->Write(insns,size*4);
	S->Close(); */
	grcDisplayf("disassembling shader at %p...",insns);
	Assert((size %3) == 0);
	static DWORD end_mask = (1<<GPUFLOWOP_EXEC_END) | (1<<GPUFLOWOP_COND_EXEC_END) |
		(1<<GPUFLOWOP_COND_EXEC_PRED_END) | (1<<GPUFLOWOP_LOOP_END) | (1<<GPUFLOWOP_COND_EXEC_PRED_CLEAN_END) |
		(1<<GPUFLOWOP_VFETCH_END);

	// decode all flow ops first.
	for (GPUSHADER_INSTRUCTION *x = (GPUSHADER_INSTRUCTION*) insns; ; ++x) {
		GPUFLOW_INSTRUCTION f0, f1;
		GPU_GET_FLOW_INSTRUCTIONS(&x->FlowPair,&f0,&f1);
		DisassembleBlock(f0,(GPUSHADER_INSTRUCTION*)insns);
		DisassembleBlock(f1,(GPUSHADER_INSTRUCTION*)insns);
		if (((1<<f0.Op)|(1<<f1.Op)) & end_mask)
			break;
	}

}
#endif

void grcDevice::BeginCommandBuffer(D3DCommandBuffer *buffer,bool /*isSkinned*/) {
	Assert(!g_grcCommandBuffer);
	g_grcCommandBuffer = buffer;

// This hack is no longer necessary -- see D3DTag-related cruft in grcDevice::DrawIndexedPrimitive
#if PRESERVE_HIZ
	D3DSurface*   pDepthStencilSurface = NULL;

	CHECK_HRESULT(sm_Primary->GetDepthStencilSurface( &pDepthStencilSurface ));
#endif

	// Specify the GPU tags to inherit.
	// For now, mark all constants as inherited to avoid wasting time resetting them.
	// In one test case it reduced the command buffer size from 23332 to 15748 bytes.
	D3DTAGCOLLECTION InheritTags = { 0 };
	D3DTagCollection_SetAll( &InheritTags );

	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_VERTEXSHADERCONSTANTS);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_PIXELSHADERCONSTANTS);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_FETCHCONSTANTS);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_MISCCONSTANTS);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_DESTINATIONPACKET);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_WINDOWPACKET);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_VALUESPACKET);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_PROGRAMPACKET);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_CONTROLPACKET);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_TESSELLATORPACKET);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_MISCPACKET);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_POINTPACKET);
	//D3DTagCollection_SetTag(&InheritTags, D3DTAG_CLIPPLANES);



	sm_Current = sm_Command;
	
	// TODO: Get predicated tiling flags correct here.
	int TileCount;
	const grcTileRect* rects= GRCDEVICE.GetTileRects(TileCount);


	if ( TileCount > 0  )
	{
		
		sm_Current->BeginCommandBuffer(buffer, D3DBEGINCB_OVERWRITE_INHERITED_STATE | D3DBEGINCB_TILING_PREDICATE_WHOLE, &InheritTags, NULL, (D3DRECT*)rects, TileCount);
	}
	else
	{
	sm_Current->BeginCommandBuffer(buffer, D3DBEGINCB_OVERWRITE_INHERITED_STATE, &InheritTags, NULL, NULL, 0);
	}

	
	ClearCachedState();

	// Call our version which knows to fix the z range when using floating-point z.
	// If we don't explicitly set the depth surface here, HiZ gets disabled, which
	// hurts pixel fill performance.
#if PRESERVE_HIZ

	// according to presentation don't do this.
	SetDepthStencilSurface(pDepthStencilSurface);   
	
	if (pDepthStencilSurface)
		pDepthStencilSurface->Release();
#endif
}


size_t grcDevice::EndCommandBuffer(grcCommandBuffer **cloneBuffer) {
	Assert(g_grcCommandBuffer);

	DWORD used, remaining;
	sm_Command->QueryBufferSpace(&used,&remaining);

	sm_Command->EndCommandBuffer();

	sm_Command->GpuDisownAll();

	sm_Current = sm_Primary;

	ClearCachedState();

	if (cloneBuffer) {
		DWORD headerSize, physicalSize;
		g_grcCommandBuffer->GetClone(0, NULL, &headerSize, NULL, &physicalSize);
		void *phys = s_CommandHeap->Allocate(physicalSize,D3DCOMMANDBUFFER_ALIGNMENT,0);
		if (phys) {
			char *buffer = rage_new char[headerSize];
			g_grcCommandBuffer->GetClone(0, (D3DCommandBuffer*)buffer, &headerSize, phys, &physicalSize);

#if DUMP_COMMAND_BUFFERS		// Enable to dump command buffers after creation
			grcDisplayf("=== command buffer header %p(%d) phys %p(%d)",buffer,headerSize,phys,physicalSize);
			u32 *cmd = (u32*) phys;
			u32 remain = physicalSize >> 2;
			u32 *stop = cmd + remain;
			while (cmd < stop) {
				if (*cmd == 0x80000000)	{ // nop for alignment?
					// grcDisplayf("%p: NOP",cmd);
					++cmd;
				}
				else if ((*cmd & 0xE0000000) == 0xC0000000) {
					int gpuCmd = (*cmd & 0xFF00) >> 8;
					const char *cmdStr = "unknown";
					if (gpuCmd == GPUCOMMANDOP_DRAW) cmdStr = "DRAW";
					else if (gpuCmd == GPUCOMMANDOP_LOAD_SHADER) cmdStr = "LOAD_SHADER";
					else if (gpuCmd == GPUCOMMANDOP_LOAD_ALU_CONSTANT) cmdStr = "LOAD_ALU_CONSTANT";
					grcDisplayf("%p: Gpu command %x (%s)",cmd,gpuCmd,cmdStr);
					if (gpuCmd == GPUCOMMANDOP_DRAW) {
						GPUCOMMAND_DRAW_INDEX *g = (GPUCOMMAND_DRAW_INDEX*)(cmd+1);
						grcDisplayf("  PrimType=%d NumIndices=%d IndexBase=%x IndexSize=%d",
							g->PrimType,g->NumIndices,g->IndexBase,g->IndexSize);
					}
					else if (gpuCmd == GPUCOMMANDOP_LOAD_ALU_CONSTANT) {
						GPUCOMMAND_LOAD_ALU_CONSTANT *g = (GPUCOMMAND_LOAD_ALU_CONSTANT*)(cmd+1);
						grcDisplayf("  Address=%x Offset=%x(%s%d) Size=%d",
							g->Address,g->Offset,g->Offset >= 1024? "PS C":"VS C",(g->Offset>>2)&255,g->Size);
						float *c = (float*)GPU_CONVERT_GPU_TO_CPU_CACHED_READONLY_ADDRESS(g->Address<<2);
						for (DWORD i=0; i<g->Size; i+=4, c+= 4)
							grcDisplayf("    %8.3f %8.3f %8.3f %8.3f",c[0],c[1],c[2],c[3]);
					}
					else if (gpuCmd == GPUCOMMANDOP_LOAD_SHADER) {
						GPUCOMMAND_LOAD_SHADER *g = (GPUCOMMAND_LOAD_SHADER*)(cmd+1);
						grcDisplayf("  Type=%d Address=%x Size=%d Start=%x",g->Type,g->Address,g->Size,g->Start);
						DisassembleShader((u32*)GPU_CONVERT_GPU_TO_CPU_CACHED_READONLY_ADDRESS(g->Address<<5),g->Size);
					}
					cmd += ((*cmd >> 16) & 4095) + 1 + 1;
				}
				else {
					grcDisplayf("%p: Write %d words to %x (flags = %x)",cmd,((*cmd >> 16) & 4095) + 1,*cmd & 0xFFFF,*cmd >> 28);
					int payload = ((*cmd >> 16) & 4095) + 1;
					int addr = *cmd & 0xFFFF;
					++cmd;
					while (payload) {
						switch (addr) {
							case GPUREG_SURFACEINFO: { GPU_SURFACEINFO *g = (GPU_SURFACEINFO*)cmd; grcDisplayf("  SurfaceInfo Pitch %d Samples %d HiZPitch %d",g->SurfacePitch,g->MsaaSamples,g->HiZPitch); } break;
							case GPUREG_COLOR0INFO: { GPU_COLORINFO *g = (GPU_COLORINFO*)cmd; grcDisplayf("  Color0Info Base %x Format %d ExpBias %d",g->ColorBase,g->ColorFormat,g->ColorExpBias); } break; 
							case GPUREG_DEPTHINFO: { GPU_DEPTHINFO *g = (GPU_DEPTHINFO*)cmd; grcDisplayf("  DepthInfo Base %x Format %d DisableHZClamp %d",g->DepthBase,g->DepthFormat,g->DisableHZClamp); } break;
							case GPUREG_COLOR1INFO: { GPU_COLORINFO *g = (GPU_COLORINFO*)cmd; grcDisplayf("  Color1Info Base %x Format %d ExpBias %d",g->ColorBase,g->ColorFormat,g->ColorExpBias); } break; 
							case GPUREG_COLOR2INFO: { GPU_COLORINFO *g = (GPU_COLORINFO*)cmd; grcDisplayf("  Color2Info Base %x Format %d ExpBias %d",g->ColorBase,g->ColorFormat,g->ColorExpBias); } break; 
							case GPUREG_COLOR3INFO: { GPU_COLORINFO *g = (GPU_COLORINFO*)cmd; grcDisplayf("  Color3Info Base %x Format %d ExpBias %d",g->ColorBase,g->ColorFormat,g->ColorExpBias); } break; 

							case GPUREG_WINDOWOFFSET: grcDisplayf("  WindowOffset %d %d",*cmd>>16,*cmd&0xFFFF); break;
							case GPUREG_WINDOWSCISSORTL: grcDisplayf("  WindowScissorTL %d %d",*cmd>>16,*cmd&0xFFFF); break;
							case GPUREG_WINDOWSCISSORBR: grcDisplayf("  WindowScissorBR %d %d",*cmd>>16,*cmd&0xFFFF); break;

							case GPUREG_MAXVTXINDX: grcDisplayf("  MaxVtxIndx %d",*cmd); break;
							case GPUREG_MINVTXINDX: grcDisplayf("  MinVtxIndx %d",*cmd); break;
							case GPUREG_INDXOFFSET: grcDisplayf("  IndxOffset %d",*cmd); break;
							case GPUREG_MULTIPRIMIBRESETINDX: grcDisplayf("  MultiPrimBrResetIndx %d",*cmd); break;
							case GPUREG_COLORMASK: grcDisplayf("  ColorMask %x",*cmd); break;
							case GPUREG_BLENDRED: grcDisplayf("  BlendRed %f",*(float*)cmd); break;
							case GPUREG_BLENDGREEN: grcDisplayf("  BlendGreen %f",*(float*)cmd); break;
							case GPUREG_BLENDBLUE: grcDisplayf("  BlendBlue %f",*(float*)cmd); break;
							case GPUREG_BLENDALPHA: grcDisplayf("  BlendAlpha %f",*(float*)cmd); break;
							case GPUREG_STENCILREFMASKBF: grcDisplayf("  StencilRefMaskBf %x",*cmd); break;
							case GPUREG_STENCILREFMASK: grcDisplayf("  StencilRefMask %x",*cmd); break;
							case GPUREG_ALPHAREF: grcDisplayf("  AlphaRef %f",*(float*)cmd); break;
							case GPUREG_VPORTXSCALE: grcDisplayf("  VPortXScale %f",*(float*)cmd); break;
							case GPUREG_VPORTXOFFSET: grcDisplayf("  VPortXOffset %f",*(float*)cmd); break;
							case GPUREG_VPORTYSCALE: grcDisplayf("  VPortYScale %f",*(float*)cmd); break;
							case GPUREG_VPORTYOFFSET: grcDisplayf("  VPortYOffset %f",*(float*)cmd); break;
							case GPUREG_VPORTZSCALE: grcDisplayf("  VPortZScale %f",*(float*)cmd); break;
							case GPUREG_VPORTZOFFSET: grcDisplayf("  VPortZOffset %f",*(float*)cmd); break;

							case GPUREG_PROGRAMCONTROL: { GPU_PROGRAMCONTROL *g = (GPU_PROGRAMCONTROL*) cmd; grcDisplayf("  ProgramControl VsMax %d PsMax %d VsRes %d PsRes %d ParamGen %d GenIndexPix %d VsExpCt %d VsExpMode %d PsExportZ %d PsExpColorCt %d GenIndexVtx %d",
															g->VsMaxReg,g->PsMaxReg,g->VsResource,g->PsResource,g->ParamGen,g->GenIndexPix,g->VsExportCount,g->VsExportMode,g->PsExportZ,g->PsExportColorCount,g->GenIndexVtx); } break;
							case GPUREG_CONTEXTMISC: { GPU_CONTEXTMISC *g = (GPU_CONTEXTMISC*) cmd; grcDisplayf("  ContextMisc IPO %d OSXY %d SmpCtl %d PGP %d PCR %d YO %d TxCacheSel %d",
														 g->InstPredOptimize,g->OutputScreenXY,g->SampleControl,g->ParamGenPos,g->PerfCounterRef,g->YieldOptimize,g->TxCacheSelect); } break;
							case GPUREG_INTERPOLATORCONTROL: { GPU_INTERPOLATORCONTROL *g = (GPU_INTERPOLATORCONTROL*) cmd; grcDisplayf("  InterpolatorControl ParamShade %d SamplingPattern %x",g->ParamShade,g->SamplingPattern); } break;
							case GPUREG_WRAPPING0: grcDisplayf("  Wrapping0 %x",*cmd); break;
							case GPUREG_WRAPPING1: grcDisplayf("  Wrapping1 %x",*cmd); break;

							case GPUREG_DEPTHCONTROL: { GPU_DEPTHCONTROL *g = (GPU_DEPTHCONTROL*)cmd; grcDisplayf("  DepthControl SE %d ZE %d ZWE %d ZFunc %d BFE %d",g->StencilEnable,g->ZEnable,g->ZWriteEnable,g->ZFunc,g->BackFaceEnable); } break;
							case GPUREG_BLENDCONTROL0: { GPU_BLENDCONTROL *g = (GPU_BLENDCONTROL*)cmd; grcDisplayf("  BlendControl Color %d/%d/%d Alpha %d/%d/%d",g->ColorSrcBlend,g->ColorBlendOp,g->ColorDestBlend,g->AlphaSrcBlend,g->AlphaBlendOp,g->AlphaDestBlend); } break;
							case GPUREG_COLORCONTROL: { GPU_COLORCONTROL *g = (GPU_COLORCONTROL*)cmd; grcDisplayf("  ColorControl AF %d ATE %d A2ME %d",g->AlphaFunc,g->AlphaTestEnable,g->AlphaToMaskEnable); } break;
							case GPUREG_HICONTROL: { GPU_HICONTROL *g = (GPU_HICONTROL*)cmd; grcDisplayf("  HiControl ZWE %d ZE %d SWE %d SE %d ZF %f SF %d SR %d BA %x",g->HiZWriteEnable,g->HiZEnable,g->HiStencilFunc,g->HiStencilEnable,
													   g->HiZFunc,g->HiStencilFunc,g->HiStencilRef,g->HiBaseAddr); } break;
							case GPUREG_CLIPCONTROL: { GPU_CLIPCONTROL *g = (GPU_CLIPCONTROL*)cmd; grcDisplayf("  ClipControl %d/%d/%d/%d/%d/%d",g->ClipPlaneEnable0,g->ClipPlaneEnable1,g->ClipPlaneEnable2,g->ClipPlaneEnable3,g->ClipPlaneEnable4,g->ClipPlaneEnable5); } break;
							case GPUREG_MODECONTROL: { GPU_MODECONTROL *g = (GPU_MODECONTROL*)cmd; grcDisplayf("  ModeControl CullMode %d PolyMode %d",g->CullMode,g->PolyMode); } break;
							case GPUREG_VTECONTROL: { GPU_VTECONTROL* g = (GPU_VTECONTROL*)cmd; grcDisplayf("  VteControl X %d/%d Y %d/%d Z %d/%d",g->VportXScaleEnable,g->VportXOffsetEnable,g->VportYScaleEnable,g->VportYOffsetEnable,g->VportZScaleEnable,g->VportZOffsetEnable); } break;
							case GPUREG_BLENDCONTROL1: { GPU_BLENDCONTROL *g = (GPU_BLENDCONTROL*)cmd; grcDisplayf("  BlendControl1 Color %d/%d/%d Alpha %d/%d/%d",g->ColorSrcBlend,g->ColorBlendOp,g->ColorDestBlend,g->AlphaSrcBlend,g->AlphaBlendOp,g->AlphaDestBlend); } break;
							case GPUREG_BLENDCONTROL2: { GPU_BLENDCONTROL *g = (GPU_BLENDCONTROL*)cmd; grcDisplayf("  BlendControl2 Color %d/%d/%d Alpha %d/%d/%d",g->ColorSrcBlend,g->ColorBlendOp,g->ColorDestBlend,g->AlphaSrcBlend,g->AlphaBlendOp,g->AlphaDestBlend); } break;
							case GPUREG_BLENDCONTROL3: { GPU_BLENDCONTROL *g = (GPU_BLENDCONTROL*)cmd; grcDisplayf("  BlendControl3 Color %d/%d/%d Alpha %d/%d/%d",g->ColorSrcBlend,g->ColorBlendOp,g->ColorDestBlend,g->AlphaSrcBlend,g->AlphaBlendOp,g->AlphaDestBlend); } break;
							case GPUREG_AACONFIG: { GPU_AACONFIG *g = (GPU_AACONFIG*)cmd; grcDisplayf("  AAConfig MsaaNumSamples %d MaxSampleDist %d",g->MsaaNumSamples,g->MaxSampleDist); } break;

							case GPUREG_FLUSHFETCHCONSTANTS:
							case GPUREG_FLUSHFETCHCONSTANTS+1:
							case GPUREG_FLUSHFETCHCONSTANTS+2: grcDisplayf("  FlushFetchConstants %x",*cmd); break;

							case 0x857E: {
								int maxlen = (payload<<2)-16;
								grcDisplayf("Shader path: %*.*s",maxlen,maxlen,(char*)(cmd+4));
								// manually skip the rest of the block
								cmd += (payload-1);
								payload = 1;
								break;
							}

							default: 
								if (addr >= GPUREG_ALUCONSTANTS && addr < GPUREG_FETCHCONSTANTS) {
									int alu = (addr - GPUREG_ALUCONSTANTS) >> 2;
									grcDisplayf("  AluConstant[%s %d] %f",alu&256?"PS":"VS",alu&255,*(float*)cmd);
								}
								else if (addr >= GPUREG_FETCHCONSTANTS && addr < GPUREG_FLOWCONSTANTS) {
									if (addr < GPUREG_FETCHCONSTANTS + 6*GPU_D3D_TEXTURE_FETCH_CONSTANT_COUNT) {
										int sampler = (addr - GPUREG_FETCHCONSTANTS) / 6;
										grcDisplayf("  Sampler[%d] %x",sampler,*cmd);
									}
									else {
										int stream = 17 - (addr - (GPUREG_FETCHCONSTANTS + 6*GPU_D3D_VERTEX_FETCH_CONSTANT_BASE)) / 2;
										grcDisplayf("  VertexStream[%d] %x",stream,*cmd);
									}
								}
								else
									grcDisplayf("  (unknown - %x) %x",addr,*cmd);
						}
						++cmd;
						++addr;
						--payload;
					}
				}
			}
			if (cmd != stop)
				grcErrorf("bug!");
#endif
			*cloneBuffer = (D3DCommandBuffer*)buffer;
			(*cloneBuffer)->Common &= ~D3DCOMMON_D3DCREATED;
			(*cloneBuffer)->Identifier = (DWORD) phys;
		}
		else {
			static int warner;
			if (++warner == 25) {
				grcDisplayf("EndCommandBuffer - cmd buf alloc of size %d bytes failed.",physicalSize);
				warner = 0;
			}
			*cloneBuffer = NULL;
			used = 0;
		}
	}
	g_grcCommandBuffer = NULL;

	return used;
}

void grcDevice::RunCommandBuffer(D3DCommandBuffer *buffer,u32 predicationSelect) {

#if !PRESERVE_HIZ
	// Only need to execute this code once at the beginning of the
	// rendering pass.
	sm_Current->m_ControlPacket.HiControl.HiZWriteEnable= TRUE;
	sm_Current->m_ControlPacket.HiControl.HiZEnable= TRUE;
	sm_Current->m_ControlPacket.HiControl.HiStencilWriteEnable= FALSE;
	sm_Current->m_ControlPacket.HiControl.HiStencilEnable= FALSE;
	sm_Current->m_ControlPacket.HiControl.HiZFunc= GPUHIZFUNC_LESS_EQUAL;
	sm_Current->m_ControlPacket.HiControl.HiStencilFunc= GPUHISTENCILFUNC_NOT_EQUAL;
	sm_Current->m_ControlPacket.HiControl.HiStencilRef= 0x0;
	sm_Current->m_ControlPacket.HiControl.HiBaseAddr= 0x0;
	D3DTagCollection_Set( &sm_Current->m_Pending, D3DTag_Index(D3DTAG_HICONTROL), D3DTag_Mask(D3DTAG_HICONTROL) );

	sm_Current->m_ControlPacket.EdramModeControl.EdramMode= GPUEDRAMMODE_DOUBLE_DEPTH;
	D3DTagCollection_Set( &sm_Current->m_Pending, D3DTag_Index(D3DTAG_EDRAMMODECONTROL), D3DTag_Mask(D3DTAG_EDRAMMODECONTROL) );

	D3DTagCollection_Clear( &sm_Current->m_Pending, D3DTag_Index(D3DTAG_HIZENABLE), D3DTag_Mask(D3DTAG_HIZENABLE) );

	sm_Current->FlushHiZStencil( D3DFHZS_SYNCHRONOUS );
#endif
	// RAY - Removed this as it saves performance for it to be called only when needed
	//ClearCachedState();
	sm_Current->RunCommandBuffer(buffer, predicationSelect);
}

void grcDevice::SetCommandBufferPredication(u32 tile,u32 run) {
	Assert(IsRecordingCommandBuffer());
	sm_Current->SetCommandBufferPredication(tile,run);
}

#elif __WIN32PC && COMMAND_BUFFER_SUPPORT

void grcDevice::CreateCommandBuffer(size_t size,grcCommandBuffer **pbuffer) {
	*pbuffer = rage_new grcCommandBuffer(size);
}

void grcDevice::DeleteCommandBuffer(grcCommandBuffer *buffer) {
	delete buffer;
}

void grcDevice::BeginCommandBuffer(grcCommandBuffer *buffer,bool /*isSkinned*/) {
	Assert(!g_grcCommandBuffer);
	g_grcCommandBuffer = buffer;
	s_DeviceWrapper.m_Next = buffer->m_List;
	s_DeviceWrapper.m_Remain = buffer->m_Size;
	ClearCachedState();
}


size_t grcDevice::EndCommandBuffer(grcCommandBuffer **cloneBuffer) {
	Assert(g_grcCommandBuffer);
	size_t result = g_grcCommandBuffer->m_Used = s_DeviceWrapper.m_Next - g_grcCommandBuffer->m_List;

	if (cloneBuffer)
		*cloneBuffer = rage_new grcCommandBuffer(g_grcCommandBuffer);

	g_grcCommandBuffer = NULL;
	s_DeviceWrapper.m_Next = NULL;
	ClearCachedState();
	if (s_DeviceWrapper.m_Remain == 0) {
		grcErrorf("grcDevice::EndCommandBuffer - Command buffer overflowed.");
		result = 0;
	}
	s_DeviceWrapper.m_Remain = 0;
	return result * sizeof(DWORD);
}


void grcDevice::RunCommandBuffer(grcCommandBuffer *buffer,u32 predicationSelect) {
	size_t count = buffer->m_Used;
	DWORD *list = buffer->m_List;
	IDirect3DDevice9 *dev = s_DeviceWrapper.m_Inner;	// Avoid overhead on playback.
	bool active = true;
	while (count) {
		DWORD cmdSize = list[0] >> 8;
		DWORD cmd = list[0] & 0xFF;
		if (cmd == REC_SetCommandBufferPredication) {
			// If current predication is zero, or it has at least one bit in common with predicationSelect,
			// do not suppress the command stream.
			u32 currentPredication = list[1];
			cmdSize = 2;
			active = !currentPredication || ((currentPredication & predicationSelect) != 0);
			// TODO: remember the offset of the NEXT SetCommandBufferPredication command
			// and skip directly to it if this test fails.
		}
		else if (active) switch (cmd) {
			case REC_SetVertexDeclaration:
				{
					CHECK_HRESULT(dev->SetVertexDeclaration((IDirect3DVertexDeclaration9*)list[1]));
					break;
				}
			case REC_SetVertexShader:
				{
					CHECK_HRESULT(dev->SetVertexShader((IDirect3DVertexShader9*)list[1]));
					break;
				}
			case REC_SetIndices:
				{
					CHECK_HRESULT(dev->SetIndices((IDirect3DIndexBuffer9*)list[1]));
					break;
				}
			case REC_SetPixelShader:
				{
					CHECK_HRESULT(dev->SetPixelShader((IDirect3DPixelShader9*)list[1]));
					break;
				}
			case REC_SetRenderState:
				{
					CHECK_HRESULT(dev->SetRenderState((D3DRENDERSTATETYPE)list[1],list[2]));
					break;
				}
			case REC_SetTexture:
				{
					CHECK_HRESULT(dev->SetTexture(list[1],(IDirect3DBaseTexture9*)list[2]));
					break;
				}
			case REC_SetSamplerState:
				{
					CHECK_HRESULT(dev->SetSamplerState(list[1],(D3DSAMPLERSTATETYPE)list[2],list[3]));
					break;
				}
			case REC_DrawPrimitive:
				{
					CHECK_HRESULT(dev->DrawPrimitive((D3DPRIMITIVETYPE)list[1],list[2],list[3]));
					break;
				}
			case REC_DrawIndexedPrimitive:
				{
					CHECK_HRESULT(dev->DrawIndexedPrimitive((D3DPRIMITIVETYPE)list[1],list[2],list[3],list[4],list[5],list[6]));
					break;
				}
			case REC_SetStreamSource:
				{
					CHECK_HRESULT(dev->SetStreamSource(list[1],(IDirect3DVertexBuffer9*)list[2],list[3],list[4]));
					break;
				}
			case REC_SetStreamSourceFreq:
				{
					CHECK_HRESULT(dev->SetStreamSourceFreq(list[1],list[2]));
					break;
				}
			case REC_SetVertexShaderConstantF:
				{
					CHECK_HRESULT(dev->SetVertexShaderConstantF(list[1],(float*)list+3,list[2]));
					break;
				}
			case REC_SetVertexShaderConstantI:
				{
					CHECK_HRESULT(dev->SetVertexShaderConstantI(list[1],(int*)list+3,list[2]));
					break;
				}
			case REC_SetVertexShaderConstantB:
				{
					CHECK_HRESULT(dev->SetVertexShaderConstantI(list[1],(BOOL*)list+3,list[2]));
					break;
				}
			case REC_SetPixelShaderConstantF:
				{
					CHECK_HRESULT(dev->SetPixelShaderConstantF(list[1],(float*)list+3,list[2]));
					break;
				}
			case REC_SetPixelShaderConstantI:
				{
					CHECK_HRESULT(dev->SetPixelShaderConstantI(list[1],(int*)list+3,list[2]));
					break;
				}
			case REC_SetPixelShaderConstantB:
				{
					CHECK_HRESULT(dev->SetPixelShaderConstantI(list[1],(BOOL*)list+3,list[2]));
					break;
				}
			default:
				Quitf("Invalid command %x in command buffer",list[0]);
		}
		Assert(cmdSize && count >= cmdSize);
		list += cmdSize;
		count -= cmdSize;
	}
	ClearCachedState();
}

void grcDevice::SetCommandBufferPredication(u32 /*tile*/,u32 run) {
	Assert(IsRecordingCommandBuffer());
	s_DeviceWrapper.SetCommandBufferPredication(0,run);
}
#endif

void grcDevice::ShutdownClass() 
{
	PrepareForShutdown();

	grcStateBlock::ShutdownClass();
	grcShutdownQuads();

#if __XENON
	void *arena = s_CommandHeap->GetHeapBase();
	XPhysicalFree(arena);
	delete s_CommandHeap;
	delete[] s_CommandHeapWorkspace;
#endif

#if __WIN32PC
	DeviceLost();
#endif

#if __XENON
	SetVertexDeclaration(NULL);
#endif
	// grcEffect::ShutdownClass has already been called by now:
	// delete g_DefaultEffect;
	// g_DefaultEffect = NULL;

	grcViewport::ShutdownClass();
	grcEffect::ShutdownClass();

	LastSafeRelease(sm_BlitDecl);
	LastSafeRelease(sm_ImDecl);

#if __XENON
	for (int i=0;i<MAX_TILE_SIZES;i++)
	{
		if (s_TileColorTarget[i] != (grcRenderTarget*)0xffffffff)
			LastSafeRelease(s_TileColorTarget[i]);
		LastSafeRelease(s_TileDepthTarget[i]);
	}	
#endif
	FreeSwapChain();

#if __WIN32PC && __DEV && __BANK
	if (!s_bRetailRuntime)
	{
		for	(int iIndex = 1; iIndex <= 7; iIndex++)
		{
			Printf("%s Count=%d Size=%d bytes\n", s_ResourceData[iIndex].m_szResourceName, s_ResourceData[iIndex].m_iNumObjects, s_ResourceData[iIndex].m_iNumBytes);
		}
	}
#endif // __WIN32PC && __DEV && __BANK

#if !__XENON
	{
		int refCount = sm_Current->Release();
		if (refCount)
		{
			grcWarningf("D3D device has %d dangling references\n", refCount);
			while (sm_Current->Release());
		}
		sm_Current = NULL;
	}
#else
	LastSafeRelease(sm_Primary);
	LastSafeRelease(sm_Command);
#endif

#if !__XENON	// The device is bogus on Xenon and the refcount always comes back as one.
	{
		int refCount = s_D3D9->Release();
		if (refCount)
		{
			grcWarningf("D3D has %d dangling references\n", refCount);
			while (s_D3D9->Release());
		}
	}
#endif

#if __WIN32PC
	if (g_hwndMain && s_WeCreatedHwndMain) {
		if (!::DestroyWindow(g_hwndMain))
			grcErrorf("Could not destroy the window");
		if (!::UnregisterClass("grcWindow", ::GetModuleHandle(0)))
			grcErrorf("Could not un-register the window's class");
		if	(!g_inWindow)
			SetupCursor(true);
		g_hwndMain = 0;
		g_winClass = 0;
		s_WeCreatedHwndMain = false;
	}
#endif
}

void grcDevice::PrepareForShutdown()
{
	for (u32 uSamplers = 0; uSamplers < 16; uSamplers++)
	{
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetTexture(uSamplers, NULL));
	}
	/* Can't set targets to NULL
	for (u32 uTargets = 0; uTargets < 4; uTargets++)
	{
		GRCDEVICE.GetCurrent()->SetRenderTarget(uTargets, NULL);
	}
	*/
	/*
	for (u32 uStreams = 0; uStreams < 4; uStreams++)
	{
		GRCDEVICE.GetCurrent()->SetStreamSource(uStreams, NULL, 0, 0);
	}
	*/

	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetVertexDeclaration(NULL));
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetDepthStencilSurface(NULL));
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetIndices(NULL));
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetPixelShader(NULL));
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetVertexShader(NULL));
	//CHECK_HRESULT(GRCDEVICE.GetCurrent()->SetViewport(NULL));
}

void grcDevice::ClearCachedState() {
	grcCurrentVertexDeclaration = NULL;
	for (u32 i = 0; i < NELEM(grcCurrentVertexBuffers); ++i)
		grcCurrentVertexBuffers[i] = NULL;
#if __WIN32PC
	for (u32 i = 0; i < grcVertexDeclaration::c_MaxStreams; i++)
		grcCurrentDividers[i] = ~0U;
#endif

	grcEffect::ClearCachedState();
}

#if __XENON
void grcDevice::SetColorExpBiasNow(int val)
{
	SetColorExpBias(val);

	rage::g_BackBuffer->ColorInfo.ColorExpBias = sm_ColorExpBias;
	SetRenderTarget(0, g_BackBuffer);

	static grcEffectGlobalVar s_gvColorExpBias;
	float exp = 1.0f/(pow(2.0f,sm_ColorExpBias));
	if (!s_gvColorExpBias)
		s_gvColorExpBias = grcEffect::LookupGlobalVar("ColorExpBias",false);
	Assert(s_gvColorExpBias || exp == 1.0f);
	grcEffect::SetGlobalVar(s_gvColorExpBias, exp);
}
#endif

bool grcDevice::BeginFrame() 
{
	grcFlashEnable = (sm_FrameCounter & 16) != 0;

#if __WIN32PC
	if (!GRCDEVICE.IsCreated() || 
		GRCDEVICE.IsPaused() ||  
		GRCDEVICE.IsInReset() ||  
		GRCDEVICE.IsInsideDeviceChange() ||
		GRCDEVICE.IsMinimized() ||
		GRCDEVICE.IsInSizeMove() ||
		GRCDEVICE.IsReleasing() ||
		GRCDEVICE.IsInReset())
	{
		sysIpcSleep(50);
		if (sm_Owner != sm_CreationOwner)
		{
#if __FINAL
			static bool bOnce = true;
			if (bOnce)
			{
				Displayf("Reset must be handled outside if owner and creation owner do not match");
				bOnce = false;
			}
#endif // __FINAL
			return false;
		}
	}
	if (IsCreated() && IsLost() && !IsPaused())
	{
		HRESULT hResult = sm_Current->TestCooperativeLevel();
		if (D3DERR_DEVICELOST == hResult)
			return false;

		// Desktop may have changed and the display format may be a different color depth
		// you'll need to adjust the color to match the new color format
		// PC TODO - Do the above
		grcDisplayWindow oRestore = sm_CurrentWindows[g_RenderThreadIndex];
		if (!ChangeDevice(oRestore))
		{
			return false;
		}
		else
		{
			SetLost( false );
		}
	}
	sysIpcWaitSema(sm_Controller);
#endif // __WIN32PC

	// Make no assumptions at the start of a frame.
	ClearCachedState();

	CHECK_HRESULT(sm_Current->BeginScene());
#if __BANK
	grcRenderTarget::BeginFrame();
#endif
#if __XENON
	if(rage::g_BackBuffer->Format == D3DFMT_A2B10G10R10F_EDRAM)
		rage::g_BackBuffer->ColorInfo.ColorExpBias = sm_ColorExpBias;
	SetRenderTarget(0, g_BackBuffer);
	sm_Current->SetDepthStencilSurface(g_DepthStencil);

#if HACK_GTA4
	// In GTA5 we share the front buffer memory, so can't start rendering the frame until it's front buffer is no longer visible or
	// we will corrupt the currently visible frame with intermediate data as we start re-using the front buffer memory.
	WaitOnFreeFrontBuffer();
#endif

#if !__OPTIMIZED
	s_Fence = sm_Current->InsertFence();	 // makes debug Xenon libs happy when double buffering vertex buffers...
#endif

#endif
	grcStateBlock::MakeDirty();
	grcStateBlock::Default();

	grcEffect::BeginFrame();
	return true;
}

void grcDevice::BeginDXView()
{
	Assert("can't be here\n");
}

void grcDevice::EndDXView()
{
	Assert("can't be here\n");
}

static bool s_alreadyResolved = false;

void grcDevice::EndFrame(const grcResolveFlags * XENON_ONLY(clearFlags)) 
{
	grcEffect::EndFrame();

	++sm_FrameCounter;
	IncrementSyncCounter();

	g_DefaultEffect->SetVar(g_DefaultSampler, (grcTexture*) NULL );	// remove any references to none texture

	WIN32PC_ONLY(if (!sm_Current) return);
	
	CHECK_HRESULT(sm_Current->SetIndices(NULL));
	CHECK_HRESULT(sm_Current->SetStreamSource(0, NULL, 0, 0));
	CHECK_HRESULT(sm_Current->SetStreamSource(1, NULL, 0, 0));
	CHECK_HRESULT(sm_Current->SetStreamSource(2, NULL, 0, 0));
	CHECK_HRESULT(sm_Current->SetStreamSource(3, NULL, 0, 0));

	CHECK_HRESULT(sm_Current->SetVertexShader(NULL));
	CHECK_HRESULT(sm_Current->SetPixelShader(NULL));

#if __XENON
	// I hate the magic number here but it appears to be configurable on Xenon!
	for (int i=0; i<=D3DVERTEXTEXTURESAMPLER3; i++)
			CHECK_HRESULT(sm_Current->SetTexture(i, NULL));
	sm_Current->SetVertexDeclaration(NULL);
	grcCurrentVertexDeclaration = NULL;
	for (int i = 0; i < NELEM(grcCurrentVertexBuffers); ++i)
		grcCurrentVertexBuffers[i] = NULL;

	// This is the only reliable way I can find to invalidate all the GPU texture sampler registers. Calling SetTexture(sampler,NULL) 
	// doesn't do it! We do this so that we don't leave stale BaseAddress pointers bound to samplers since this appears to lead to a
	// GPU hang when two samplers have the same BaseAddress but different MipAddress values (even if the samplers aren't actually used by a shader).
	GPUTEXTURE_FETCH_CONSTANT* pTexFetchConsts = GRCDEVICE.GetCurrent()->m_Constants.TextureFetch;
	for ( u32 index=0; index < GPU_D3D_TEXTURE_FETCH_CONSTANT_COUNT; index++ )
	{
		DWORD sampler = GPU_CONVERT_D3D_TO_HARDWARE_TEXTUREFETCHCONSTANT(index);
		pTexFetchConsts[sampler].Type = GPUCONSTANTTYPE_INVALID_TEXTURE;
		pTexFetchConsts[sampler].BaseAddress = NULL;
		pTexFetchConsts[sampler].MipAddress = NULL;
		D3DTagCollection_Set(&GRCDEVICE.GetCurrent()->m_Pending, D3DTag_Index(D3DTAG_TEXTUREFETCHCONSTANTS), D3DTag_SamplerMask(sampler));
	}
#else
	CHECK_HRESULT(sm_Current->SetTexture(0, NULL));
#endif

	// Make sure no effect is active.
	g_DefaultEffect->Bind(grcetNONE);

#if __WIN32PC
	if (sm_PresentCallback)
		(*sm_PresentCallback)(sm_Current,NULL,NULL,g_hwndOverride,NULL);
	CHECK_HRESULT(sm_Current->EndScene());
	
	// Use Swap chain
	Assert(sm_pSwapChain != NULL);
	while((sm_uDeviceStatus = ((IDirect3DSwapChain9*)sm_pSwapChain)->Present(NULL, NULL, NULL, NULL, D3DPRESENT_DONOTWAIT)) == D3DERR_WASSTILLDRAWING)
	{
		sysIpcSleep(0);
	}

	if (sm_PostPresentCallback != NULL)
		(*sm_PostPresentCallback)();

	if( D3D_OK != sm_uDeviceStatus )
	{
		grcWarningf("Failed Present - Issuing Device Lost Call - Return Code %x", sm_uDeviceStatus);
		if (D3DERR_DRIVERINTERNALERROR == sm_uDeviceStatus)
		{
			grcErrorf("Present return an internal driver error which indicates the driver crashed");
		}
		SetLost( true );
		SetInReset(false);
	}
#elif __XENON
	CHECK_HRESULT(sm_Current->EndScene());
	bool directTileResolve = false;

	if (s_TileCount>0) // still tiling? we need to end that here...
	{
		directTileResolve = s_TileCompositeColorTarget && (s_TileCompositeColorTarget->GetTexturePtr() == g_FrontBuffers[g_CurrentFrontBuffer]); // if they are resolving directly into the front buffer, we can skip that step next...
		EndTiledRendering();
	}

	// RESOLVE. SWAP. (We WAIT for a free front buffer in BeginFrame)

	// RESOLVE now it's safe
	if (!directTileResolve && !s_alreadyResolved)
	{
		if (clearFlags) {
			D3DVECTOR4 clearColor = { clearFlags->Color.GetRedf(), clearFlags->Color.GetGreenf(), clearFlags->Color.GetBluef(), clearFlags->Color.GetAlphaf() };			
			DWORD flags = D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_CLEARDEPTHSTENCIL | D3DRESOLVE_CLEARRENDERTARGET;
			if(rage::g_BackBuffer->Format == D3DFMT_A2B10G10R10F_EDRAM)
			{
				flags |= D3DRESOLVE_EXPONENTBIAS((DWORD)-sm_ColorExpBias);
			}
			CHECK_HRESULT(sm_Current->Resolve(flags,
				NULL, g_FrontBuffers[g_CurrentFrontBuffer], NULL, 0, NULL, &clearColor, FixDepth(clearFlags->Depth), clearFlags->Stencil, NULL ));
		}
		else
		{
			DWORD flags = D3DRESOLVE_RENDERTARGET0;
			if(rage::g_BackBuffer->Format == D3DFMT_A2B10G10R10F_EDRAM)
			{
				flags |= D3DRESOLVE_EXPONENTBIAS((DWORD)-sm_ColorExpBias);
			}
			CHECK_HRESULT(sm_Current->Resolve(
			flags, NULL, g_FrontBuffers[g_CurrentFrontBuffer], NULL, 0, NULL, NULL, FixDepth(0.0f), 0, NULL ));
		}
	}
	s_alreadyResolved = false;
#if _XDK_VER < 2571
	sm_Current->RenderSystemUI();
#endif
#if HACK_GTA4 //Could be useful to other 360 projects - triple buffer the swap chain - look at xdk docs for more info (search QuerySwapStatus)
	// SWAP 
	ASyncSwap();
#else //HACK_GTA4

	sm_Current->SynchronizeToPresentationInterval();
		
#if !__OPTIMIZED
	sm_Current->BlockOnFence(s_Fence);
#endif

 	sm_Current->Swap(g_FrontBuffers[g_CurrentFrontBuffer],NULL);

	g_CurrentFrontBuffer = !g_CurrentFrontBuffer;

	if (PARAM_blockuntilidle.Get())
		sm_Current->BlockUntilIdle();
#endif	// HACK_GTA4
#endif	// __WIN32PC		

#if	__WIN32PC 
#if __DEV
	UpdateResourceManagerQuery();
#endif // __DEV
	sysIpcSignalSema(sm_Controller);
#endif // __WIN32PC

#if __BANK
	grcRenderTarget::EndFrame();
#endif
}
IDirect3DSurface9 *pictureSurface = 0;
void grcDevice::BeginTakeScreenShot() {
#if	__WIN32PC
	RECT screen;

	GetClientRect(g_hwndMain,&screen);
 
	if(FAILED(sm_Current->CreateRenderTarget(screen.right,screen.bottom,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,TRUE,&pictureSurface,NULL)))
		grcErrorf("Failed to create render target for screenshot.\n");

	if(FAILED(sm_Current->SetRenderTarget(0,pictureSurface)))
		grcErrorf("Failed to set render target for screenshot\n");;
#endif
}
grcImage *grcDevice::EndTakeScreenShot() {
	//restore render target to the back buffer...which I don't know how to do...
	//sm_Current->SetRenderTarget(0, )
	grcImage *dest = NULL;
#if __XENON
	return CaptureScreenShot(dest);
#else
	RECT window;
	GetClientRect(g_hwndMain,&window);

	//POINT origin = { 0,0 };

	//MapWindowPoints(g_hwndMain, HWND_DESKTOP, &origin, 1);
	int width = window.right - window.left;
	int height = window.bottom - window.top;

	
	D3DLOCKED_RECT rect;
	if (SUCCEEDED(pictureSurface->LockRect(&rect,&window,D3DLOCK_READONLY))) {

		dest = grcImage::Create(width,height,1,grcImage::A8R8G8B8,grcImage::STANDARD,0,0);

		u32 *d = (u32*) dest->GetBits();
		u32 *s = (u32*) rect.pBits;
		for (int row=0; row<height; row++,s = (u32*)((char*)s + rect.Pitch),d += width) {
			for (int col=0; col<width; col++)
				d[col] = s[col] | 0xFF000000;
		}
		pictureSurface->UnlockRect();
	}
	else
		grcErrorf("Only 32 bit per pixel (A8R8G8B8) video format supported.");
	pictureSurface->Release();
	pictureSurface = 0;

	// reset back to backbuffer
	CHECK_HRESULT(sm_Current->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_LEFT,&pictureSurface));
	CHECK_HRESULT(sm_Current->SetRenderTarget(0,pictureSurface));
	pictureSurface->Release();
	pictureSurface = 0;

	return dest;
#endif
}
grcImage *grcDevice::CaptureScreenShot(grcImage* pImage) {
#if __WIN32PC
	IDirect3DSurface9 *lpSurface = 0;

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

	grcImage *dest = NULL;
	if (SUCCEEDED(sm_Current->CreateOffscreenPlainSurface(screen.right,screen.bottom, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &lpSurface, NULL))) {
		if (SUCCEEDED(sm_Current->GetFrontBufferData(0, lpSurface))) {
			D3DLOCKED_RECT rect;
			if (SUCCEEDED(lpSurface->LockRect(&rect,&window,D3DLOCK_READONLY))) {
            if (pImage)
            {
               dest = pImage;
            }
            else
            {
   				dest = grcImage::Create(width,height,1,grcImage::A8R8G8B8,grcImage::STANDARD,0,0);
            }
				u32 *d = (u32*) dest->GetBits();
				u32 *s = (u32*) rect.pBits;
				for (int row=0; row<height; row++,s = (u32*)((char*)s + rect.Pitch),d += width) {
					for (int col=0; col<width; col++)
						d[col] = s[col] | 0xFF000000;
				}
				lpSurface->UnlockRect();
			}
		}
		else
			grcErrorf("Only 32 bit per pixel (A8R8G8B8) video format supported.");
		lpSurface->Release();
	}
#else
	XGTEXTURE_DESC desc;
	D3DLOCKED_RECT rect;
	grcImage *dest;
	XGGetTextureDesc(g_FrontBuffers[g_CurrentFrontBuffer], 0, &desc);
   if (pImage)
      dest = pImage;
   else
	   dest = grcImage::Create(desc.Width,desc.Height,1,grcImage::A8R8G8B8,grcImage::STANDARD,0,0);
	sm_Current->BlockOnFence(sm_Current->InsertFence());
	g_FrontBuffers[g_CurrentFrontBuffer]->LockRect(0,&rect,NULL,D3DLOCK_READONLY);

	RECT tilerect = { 0, 0, desc.Width, desc.Height };
	XGUntileSurface(dest->GetBits(), desc.RowPitch, NULL, rect.pBits, desc.WidthInBlocks, desc.HeightInBlocks, &tilerect, desc.BytesPerBlock);
	XGEndianSwapMemory(dest->GetBits(), dest->GetBits(), XGENDIAN_8IN32, 4, desc.HeightInBlocks * desc.RowPitch / 4);

	/*u32 *d = (u32*) dest->GetBits();
	u32 *s = (u32*) rect.pBits;
	u32 area = desc.Width * desc.Height;
	do {
		*d++ = *s++ | 0xFF000000;
	} while (--area);
*/
	g_FrontBuffers[g_CurrentFrontBuffer]->UnlockRect(0);
#endif
	
	return dest;
}


#if __XENON
static inline u32 Address2DTiledOffset(
	u32 x,             // x coordinate of the texel/block
	u32 y,             // y coordinate of the texel/block
	u32 Width          // Width of the image in texels/blocks
	)
{
	u32 AlignedWidth;
	const u32 LogBpp = 2;
	u32 Macro;
	u32 Micro;
	u32 Offset;

	AlignedWidth = (Width + 31) & ~31;
	Macro        = ((x >> 5) + (y >> 5) * (AlignedWidth >> 5)) << (LogBpp + 7);
	Micro        = (((x & 7) + ((y & 6) << 2)) << LogBpp);
	Offset       = Macro + ((Micro & ~15) << 1) + (Micro & 15) + ((y & 8) << (3 + LogBpp)) + ((y & 1) << 4);

	return (((Offset & ~511) << 3) + ((Offset & 448) << 2) + (Offset & 63) +
		((y & 16) << 7) + (((((y & 8) >> 2) + (x >> 3)) & 3) << 6)) >> LogBpp;
}


static void copyscan_360(u8 *dest,void *base,int row,int width,int stride,u8 *gammalut) {
	u32 *src = (u32*)base;
	for (int x=0; x<width; x++) {
		UINT offset = Address2DTiledOffset(x,row,stride>>2);
		u32 n = src[offset];
		dest[0] = gammalut[u8(n >> 8)];
		dest[1] = gammalut[u8(n >> 16)];
		dest[2] = gammalut[u8(n >> 24)];
		dest += 3;
	}
}
#endif

bool grcDevice::CaptureScreenShotToFile(const char *outName,float gamma) {
#if __WIN32PC
	bool result = false;
	grcImage *image = CaptureScreenShot(NULL);
	if (image) {
		result = image->SavePNG(outName,gamma);
		image->Release();
	}
	return result;
#else
	XGTEXTURE_DESC png_desc;
	D3DLOCKED_RECT destRect;

	// This is copied from EndFrame (so set a flag to avoid doing the work twice -- really should just move the capture after EndFrame
	// in grcSetup but that would massively complicate some already messy logic there)
	DWORD flags = D3DRESOLVE_RENDERTARGET0;
	if(rage::g_BackBuffer->Format == D3DFMT_A2B10G10R10F_EDRAM)
		flags |= D3DRESOLVE_EXPONENTBIAS((DWORD)-sm_ColorExpBias);
	CHECK_HRESULT(sm_Current->Resolve(flags, NULL, g_FrontBuffers[g_CurrentFrontBuffer], NULL, 0, NULL, NULL, FixDepth(0.0f), 0, NULL ));
	s_alreadyResolved = true;

	XGGetTextureDesc(g_FrontBuffers[g_CurrentFrontBuffer], 0, &png_desc);
	// sysTimer T;
	sm_Current->BlockOnFence(sm_Current->InsertFence());
	g_FrontBuffers[g_CurrentFrontBuffer]->LockRect(0,&destRect,NULL,D3DLOCK_READONLY);
	// float t1 = T.GetMsTime();
	// T.Reset();

	grcImage::WritePNG(outName,copyscan_360,png_desc.Width,png_desc.Height,destRect.pBits,destRect.Pitch,gamma);
	// float t2 = T.GetMsTime();
	// Displayf("Capture - %f ms in lock, %f ms in write png",t1,t2);

	g_FrontBuffers[g_CurrentFrontBuffer]->UnlockRect(0);

	return true;
#endif
}

#if __WIN32PC
void grcDevice::CreateVertexShader(u8 *programData, u32 programSize, grcVertexShader **Program)
{
	IDirect3DVertexShader9 *ProgramDx9;
	(void)programSize;
	CHECK_HRESULT(sm_Current->CreateVertexShader((DWORD*)programData, &ProgramDx9));
	*Program = ProgramDx9;
}

void grcDevice::CreatePixelShader(u8 *programData, u32 programSize, grcPixelShader **Program)
{
	IDirect3DPixelShader9 *ProgramDx9;
	(void)programSize;
	CHECK_HRESULT(sm_Current->CreatePixelShader((DWORD*)programData, &ProgramDx9));
	*Program = ProgramDx9;
}
void grcDevice::CreateComputeShader(u8 *programData, u32 programSize, grcComputeShader **Program)
{
	(void*)programData;
	(void) programSize;
	(void*) Program;
	AssertMsg(0, "CreateComputeShader is unsupported in DX9");

}
void grcDevice::CreateDomainShader(u8 *programData, u32 programSize, grcDomainShader **Program)
{
	(void*)programData;
	(void) programSize;
	(void*) Program;
	AssertMsg(0, "CreateDomainShader is unsupported in DX9");

}
void grcDevice::CreateGeometryShader(u8 *programData, u32 programSize, grcGeometryShader **Program)
{
	(void*)programData;
	(void) programSize;
	(void*) Program;
	AssertMsg(0, "CreateGeometryShader is unsupported in DX9");
	
}
void grcDevice::CreateGeometryShaderWithStreamOutput(u8 *programData, u32 programSize, grcStreamOutputDeclaration *pSODeclaration, u32 NumEntries, u32 OutputStreamStride, grcGeometryShader **ppGeometryShader)
{
	(void*)programData;
	(void)programSize;
	(void*)pSODeclaration;
	(void)NumEntries;
	(void)OutputStreamStride;
	(void*)ppGeometryShader;
	AssertMsg(0, "CreateGeometryShaderWithStreamOutput is unsupported in DX9");

}
void grcDevice::CreateHullShader(u8 *programData, u32 programSize, grcHullShader **Program)
{
	(void*)programData;
	(void) programSize;
	(void*) Program;
	AssertMsg(0, "CreateHullShader is unsupported in DX9");

}
void grcDevice::SetVertexShader(grcVertexShader *vs, const grcProgram *pProgram)
{
	(void*)pProgram;
	CHECK_HRESULT(sm_Current->SetVertexShader((IDirect3DVertexShader9*)vs));
}

void grcDevice::SetComputeShader(grcComputeShader *gs,const grcProgram *pProgram)
{
	(void*)pProgram;
	(void*) gs;
	AssertMsg(0, "SetComputeShader is unsupported in DX9");
}
void grcDevice::SetDomainShader(grcDomainShader *gs,const grcProgram *pProgram)
{
	(void*)pProgram;
	(void*) gs;
	AssertMsg(0, "SetDomainShader is unsupported in DX9");
}
void grcDevice::SetGeometryShader(grcGeometryShader *gs,const grcProgram *pProgram)
{
	(void*)pProgram;
	(void*) gs;
	AssertMsg(0, "SetGeometryShader is unsupported in DX9");
}
void grcDevice::SetHullShader(grcHullShader *gs,const grcProgram *pProgram)
{
	(void*)pProgram;
	(void*) gs;
	AssertMsg(0, "SetHullShader is unsupported in DX9");
}

void grcDevice::SetPixelShader(grcPixelShader *ps, const grcProgram *pProgram)
{
	(void*)pProgram;
	CHECK_HRESULT(sm_Current->SetPixelShader((IDirect3DPixelShader9*)ps));
}

void grcDevice::CreateShaderConstantBuffer(int ByteSize, grcBuffer **pData NOTFINAL_ONLY(, const char* pName))
{
#if !__FINAL
	(void) pName;
#endif
	(void)ByteSize;
	(void*)pData;
	AssertMsg(0, "CreateShaderConstantBuffer is unsupported in DX9");
}

void grcDevice::SetVertexShaderConstantF(int address, const float *data,int count, u32 offset, void *pvDataBuf)
{
	(void)offset;
	(void*)pvDataBuf;
	CHECK_HRESULT(sm_Current->SetVertexShaderConstantF(address,data,count));
}

void grcDevice::SetVertexShaderConstantFW(int address, const float *data,int count, u32 offset, void *pvDataBuf)
{
	(void)offset;
	(void*)pvDataBuf;
	CHECK_HRESULT(sm_Current->SetVertexShaderConstantF(address,data,count*4));
}

void grcDevice::SetVertexShaderConstantB(int address, bool value,u32 offset,void *pvDataBuf)
{
	(void)offset;
	(void*)pvDataBuf;
	BOOL b = value;
	CHECK_HRESULT(sm_Current->SetVertexShaderConstantB(address,&b,1));
}

void grcDevice::SetVertexShaderConstantI(int address, int value,u32 offset,void *pvDataBuf)
{
	(void)offset;
	(void*)pvDataBuf;
	CHECK_HRESULT(sm_Current->SetVertexShaderConstantI(address,&value,1));
}

void grcDevice::SetPixelShaderConstantF(int address, const float *data, int count, u32 offset, void *pvDataBuf)
{
	(void)offset;
	(void*)pvDataBuf;
	CHECK_HRESULT(sm_Current->SetPixelShaderConstantF(address,data,count));
}

void grcDevice::SetPixelShaderConstantFW(int address, const float *data, int count, u32 offset, void *pvDataBuf)
{
	(void)offset;
	(void*)pvDataBuf;
	CHECK_HRESULT(sm_Current->SetPixelShaderConstantF(address,data,count*4));
}

void grcDevice::SetPixelShaderConstantB(int address, bool value,u32 offset,void *pvDataBuf)
{
	(void)offset;
	(void*)pvDataBuf;
	BOOL b = value;
	CHECK_HRESULT(sm_Current->SetPixelShaderConstantB(address,&b,1));
}
void grcDevice::SetComputeShaderConstantF(int address, float *data,int count, u32 offset, void *pvDataBuf)
{
	(void) address;
	(void*) data;
	(void) count;
	(void) offset;
	(void*) pvDataBuf;
	AssertMsg(0, "SetComputeShaderConstantF is unsupported in DX9");
}

void grcDevice::SetComputeShaderConstantB(int address, bool value,u32 offset,void *pvDataBuf)
{
	(void) address;
	(void) value;
	(void) offset;
	(void*) pvDataBuf;
	AssertMsg(0, "SetComputeShaderConstantB is unsupported in DX9");
}
void grcDevice::SetDomainShaderConstantF(int address, float *data,int count, u32 offset, void *pvDataBuf)
{
	(void) address;
	(void*) data;
	(void) count;
	(void) offset;
	(void*) pvDataBuf;
	AssertMsg(0, "SetDomainShaderConstantF is unsupported in DX9");
}

void grcDevice::SetDomainShaderConstantB(int address, bool value,u32 offset,void *pvDataBuf)
{
	(void) address;
	(void) value;
	(void) offset;
	(void*) pvDataBuf;
	AssertMsg(0, "SetDomainShaderConstantB is unsupported in DX9");
}
void grcDevice::SetGeometryShaderConstantF(int address, float *data,int count, u32 offset, void *pvDataBuf)
{
	(void) address;
	(void*) data;
	(void) count;
	(void) offset;
	(void*) pvDataBuf;
	AssertMsg(0, "SetGeometryShaderConstantF is unsupported in DX9");
}

void grcDevice::SetGeometryShaderConstantB(int address, bool value,u32 offset,void *pvDataBuf)
{
	(void) address;
	(void) value;
	(void) offset;
	(void*) pvDataBuf;
	AssertMsg(0, "SetGeometryShaderConstantB is unsupported in DX9");
}
void grcDevice::SetHullShaderConstantF(int address, float *data,int count, u32 offset, void *pvDataBuf)
{
	(void) address;
	(void*) data;
	(void) count;
	(void) offset;
	(void*) pvDataBuf;
	AssertMsg(0, "SetHullShaderConstantF is unsupported in DX9");
}

void grcDevice::SetHullShaderConstantB(int address, bool value,u32 offset,void *pvDataBuf)
{
	(void) address;
	(void) value;
	(void) offset;
	(void*) pvDataBuf;
	AssertMsg(0, "SetHullShaderConstantB is unsupported in DX9");
}

#if RSG_PC
u32 grcDevice::GetDXFeatureLevelSupported()
{
	Assertf(false, "This have not been implemented for dx9");
	return 0;
}
#endif

bool grcDevice::CanLoadShader(u32 shaderMajor, u32 UNUSED_PARAM(shaderMinor))
{
	return (shaderMajor <= 3);
}

#endif

int g_JpegSaveQuality = 75;

bool grcDevice::CaptureScreenShotToJpegFile(const char *outName) {
#if __WIN32PC
	bool result = false;
	grcImage *image = CaptureScreenShot(NULL);
	if (image) {
		result = image->SaveJPEG(outName);
		image->Release();
	}
	return result;
#else
	XGTEXTURE_DESC desc;
	D3DLOCKED_RECT destRect, srcRect;

	DWORD flags = D3DRESOLVE_RENDERTARGET0;
	if(rage::g_BackBuffer->Format == D3DFMT_A2B10G10R10F_EDRAM)
		flags |= D3DRESOLVE_EXPONENTBIAS((DWORD)-sm_ColorExpBias);
	CHECK_HRESULT(sm_Current->Resolve(flags, NULL, g_FrontBuffers[g_CurrentFrontBuffer], NULL, 0, NULL, NULL, FixDepth(0.0f), 0, NULL ));

	XGGetTextureDesc(g_FrontBuffers[g_CurrentFrontBuffer], 0, &desc);
	sm_Current->BlockOnFence(sm_Current->InsertFence());
	g_FrontBuffers[g_CurrentFrontBuffer]->LockRect(0,&srcRect,NULL,0);
#if HACK_GTA4
	g_FrontBuffers[((g_CurrentFrontBuffer==0)?2:g_CurrentFrontBuffer-1)]->LockRect(0,&destRect,NULL,0);
#else // HACK_GTA4
	g_FrontBuffers[g_CurrentFrontBuffer^1]->LockRect(0,&destRect,NULL,0);
#endif // HACK_GTA4
	RECT tilerect = { 0, 0, desc.Width, desc.Height };
	XGUntileSurface(destRect.pBits, desc.RowPitch, NULL, srcRect.pBits, desc.WidthInBlocks, desc.HeightInBlocks, &tilerect, desc.BytesPerBlock);
	XGEndianSwapMemory(destRect.pBits, destRect.pBits, XGENDIAN_8IN32, 4, desc.HeightInBlocks * desc.RowPitch / 4);
	grcImage::SaveStreamToJPEG(outName, destRect.pBits, desc.Width , desc.Height, desc.RowPitch,g_JpegSaveQuality);
	// Recover original framebuffer (so that a second screenshot in a row doesn't get crap)
	memcpy(destRect.pBits, srcRect.pBits, destRect.Pitch * desc.Height);
	g_FrontBuffers[g_CurrentFrontBuffer]->UnlockRect(0);
#if HACK_GTA4
	g_FrontBuffers[((g_CurrentFrontBuffer==0)?2:g_CurrentFrontBuffer-1)]->UnlockRect(0);
#else // HACK_GTA4
	g_FrontBuffers[g_CurrentFrontBuffer^1]->UnlockRect(0);
#endif // HACK_GTA4
	return true;
#endif
}

void grcDevice::ClearRect(int x,int y,int width,int height,float depth,const Color32 &color) {
	ClearRect(x,y,width,height,true,color,true,depth,true,0);
}

void grcDevice::ClearRect(int x,int y,int width,int height,bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth,bool enableClearStencil,u32 clearStencil, bool yFlipped) {
	WIN32PC_ONLY(if (!sm_Current) return);
	u32 flags = (enableClearColor? D3DCLEAR_TARGET : 0) | (enableClearDepth? D3DCLEAR_ZBUFFER : 0) | (enableClearStencil? D3DCLEAR_STENCIL : 0);
	Assert(width && height);
	D3DRECT	rect;

	rect.x1 = x;
	rect.x2 = x + width;
	rect.y1 = y;
	rect.y2 = y + height;

	if (yFlipped)
	{
		rect.y1 = -rect.y1;
		rect.y2 = -rect.y2;
	}

	if (FAILED(sm_Current->Clear(1,&rect,flags,clearColor.GetDeviceColor(),FixDepth(clearDepth),clearStencil)))
		grcErrorf("grcDevice::Clear failed, probably trying to clear a buffer that isn't attached.");
}


static int s_BeginBlit;


inline void grcDevice::BeginBlit() {
	if (++s_BeginBlit == 1) {
#if __WIN32PC
		CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_CLIPPING, false));
		CHECK_HRESULT(sm_Current->SetStreamSource(0,s_BlitVB,0,sizeof(BlitVert)));
#elif __XENON
		CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_VIEWPORTENABLE, false));
#endif
		if (!grcEffect::IsInDraw())
			g_DefaultEffect->Bind(s_DefaultBlit);
	}
}


inline void grcDevice::EndBlit() {
	if (--s_BeginBlit == 0) {
#if __WIN32PC
		CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_CLIPPING, true));
#elif __XENON
		CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_VIEWPORTENABLE, true));
#endif
	}
}


#pragma warning(disable: 4996)

void grcDevice::BlitRect(int x1, int y1, int x2, int y2, float zVal, int u1, int v1, int u2,int v2, const Color32 &color) {
	WIN32PC_ONLY(if (!sm_Current) return);
	const float shim = 0.5f;
	float itu, itv;
	const grcTexture *tex = grcGetTexture();
	if (tex) {
		itu = 1.0f / tex->GetWidth();
		itv = 1.0f / tex->GetHeight();
	}
	else {
		itu = itv = 0;
	}
	BlitRectf(float(x1)-shim,float(y1)-shim,float(x2)-shim,float(y2)-shim,zVal,
		float(u1)*itu,float(v1)*itv,float(u2)*itu,float(v2)*itv,color);
}


#if __WIN32PC
void grcDevice::BlitRectf(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color) {
	if (!sm_Current) return;
	SetVertexDeclaration(sm_BlitDecl);
	
	grcStateBlock::Flush();

#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	BeginBlit();

	u32 lockFlags = D3DLOCK_NOOVERWRITE;
	if (s_BlitOffset + 4 > BlitVertSize) {
		s_BlitOffset = 0;
		lockFlags = D3DLOCK_DISCARD;
	}
	BlitVert *bv = NULL;
	if (s_BlitVB->Lock(s_BlitOffset*sizeof(BlitVert),4*sizeof(BlitVert),(void**)&bv,lockFlags) == D3D_OK)
	{
		if (bv != NULL)
		{
			bv[0].Set( x1, y1, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u1, v1 );
			bv[1].Set( x1, y2, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u1, v2 );
			bv[2].Set( x2, y1, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u2, v1 );
			bv[3].Set( x2, y2, FixDepth(zVal), 1.0f, color.GetColor(), ~0U, u2, v2 );
		}
		s_BlitVB->Unlock();
	}

#if STALL_DETECTION
	if ((oTime.GetMsTime() > STALL_TIME) && STALL_ONLY_RENDERTHREAD(GRCDEVICE.CheckThreadOwnership()))
	{
		grcWarningf("Vertex Buffer Lock BlitRectf took %f milliseconds", oTime.GetMsTime());
	}
#endif // STALL_DETECTION

	// I'm not crazy about making such small calls, but the alternatives are either
	// generating a bunch of indexed stuff, or independent tri's (sigh)
	CHECK_HRESULT(sm_Current->DrawPrimitive(D3DPT_TRIANGLESTRIP,s_BlitOffset,2));
	s_BlitOffset += 4;

	EndBlit();
}


#elif __XENON
void grcDevice::BlitRectf(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color ) {
	SetVertexDeclaration(sm_BlitDecl);
	
	grcStateBlock::Flush();

	BeginBlit();

	struct { 
		float sx, sy, sz, rhw; 
		u32 cpv; 
		float tu, tv; 
		void Set(float x,float y,float z,u32 c,float u,float v) {
			sx = x; sy = y; sz = z; rhw = 1.0f;
			cpv = c; tu = u; tv = v;
		}
	} *v;
	if (SUCCEEDED(sm_Current->BeginVertices(D3DPT_RECTLIST,3,sizeof(*v),(void**)&v))) {
		// v0 ---- v1
		// |        |
		// v2 ---- v3 (implicit)
		v[0].Set( x1, y1, FixDepth(zVal), color.GetColor(), u1, v1 );
		v[1].Set( x2, y1, FixDepth(zVal), color.GetColor(), u2, v1 );
		v[2].Set( x1, y2, FixDepth(zVal), color.GetColor(), u1, v2 );

		sm_Current->EndVertices();
	}

	EndBlit();
}
void grcDevice::BlitRectfNoSetup(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color ) 
{
	SetVertexDeclaration(sm_BlitDecl);
	
	grcStateBlock::Flush();

	CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_VIEWPORTENABLE, false));

	struct { 
		float sx, sy, sz, rhw; 
		u32 cpv; 
		float tu, tv; 
		void Set(float x,float y,float z,u32 c,float u,float v) {
			sx = x; sy = y; sz = z; rhw = 1.0f;
			cpv = c; tu = u; tv = v;
		}
	} *v;
	if (SUCCEEDED(sm_Current->BeginVertices(D3DPT_RECTLIST,3,sizeof(*v),(void**)&v))) {
		// v0 ---- v1
		// |        |
		// v2 ---- v3 (implicit)
		v[0].Set( x1, y1, FixDepth(zVal), color.GetColor(), u1, v1 );
		v[1].Set( x2, y1, FixDepth(zVal), color.GetColor(), u2, v1 );
		v[2].Set( x1, y2, FixDepth(zVal), color.GetColor(), u1, v2 );

		sm_Current->EndVertices();
	}
	CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_VIEWPORTENABLE, true ));
}
#endif

void grcDevice::BlitText(int posx,int posy,float posz,const s16 *destxywh,const u8 *srcxywh,int count,Color32 color,bool /*bilinear*/) {
	// TODO: Replace with something faster
	while (count--) {
		BlitRect(posx+(destxywh[0]>>4),posy+(destxywh[1]>>4),posx+((destxywh[0]+destxywh[2])>>4),posy+((destxywh[1]+destxywh[3])>>4),posz,
			srcxywh[0],srcxywh[1],srcxywh[0]+srcxywh[2],srcxywh[1]+srcxywh[3],color);
		destxywh += 4;
		srcxywh += 4;
	}
}

#pragma warning(error:4996)

void grcDevice::SetScissor(int x,int y,int width,int height) {
	grcAssertf(x >= 0 && x <= 8192,"Invalid x in grcDevice::SetScissor, x (%d) should be between 0 and 8192 inclusive",x);
	grcAssertf(y >= 0 && y <= 8192,"Invalid y in grcDevice::SetScissor, y (%d) should be between 0 and 8192 inclusive",y);
	grcAssertf(width >= 0 && width <= 8192,"Invalid width in grcDevice::SetScissor, width (%d) should be between 0 and 8192 inclusive",width);
	grcAssertf(height >= 0 && height <= 8192,"Invalid height in grcDevice::SetScissor, height (%d) should be between 0 and 8192 inclusive",height);

	RECT rect;
	rect.left = x;
	rect.right = x + width;
	rect.top = y;
	rect.bottom = y + height;
	Assert(rect.left >= 0);
	Assert(rect.top >= 0);
	Assertf(rect.right <= GetWidth(), "w=%d", GetWidth());
	Assertf(rect.bottom <= GetHeight(), "h=%d", GetHeight());

	if(rect.left < 0)
	{
		rect.left = 0;
	}

	if(rect.top < 0)
	{
		rect.top = 0;
	}

	if(rect.right > GetWidth())
	{
		rect.right = GetWidth();
	}

	if(rect.bottom > GetHeight())
	{
		rect.bottom = GetHeight();
	}

	CHECK_HRESULT(sm_Current->SetScissorRect(&rect));
}


void grcDevice::DisableScissor() {
	RECT rect = { 0,0,8192,8192 };
	CHECK_HRESULT(sm_Current->SetScissorRect(&rect));
}


void grcDevice::GetScissor(int &x,int &y,int &width,int &height) {
	RECT rect;
	CHECK_HRESULT(sm_Current->GetScissorRect(&rect));
	x = rect.left;
	y = rect.top;
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
}

void grcDevice::GetSafeZone(int &x0, int &y0, int &x1, int &y1) {
	
#if __XENON
	float width = (float) sm_CurrentWindows[g_RenderThreadIndex].uWidth;
	float height = (float) sm_CurrentWindows[g_RenderThreadIndex].uHeight;

	// 
	// Xbox360 TCR #023 (as of 1.2 (November 2005): VID Title Safe Area
	// Requirement: Games must display all text and menu items critical to gameplay, using the game's default settings, 
	//			    within the inner 90 percent of the x and y resolution for all display modes.

	// Remarks:     a minimum of 90 percent is required for title safe area, it is recommended that games consider 
	//				displaying all critical text within the inner 80 percent because the safe area varies widely across TVs.
	//
	//
	// IHMO, 80 is way too small. Let's go with the actual requirement, not the "recommendation"

	float safeW = width * 0.9f;
	float safeH = height * 0.9f;  
	float offsetW = (width - safeW) * 0.5f;
	float offsetH = (height - safeH) * 0.5f;
	// Round down to lowest area
	x0 = (int) ceil(offsetW);
	y0 = (int) ceil(offsetH);
	x1 = (int) floor(width - offsetW - 1.f);
	y1 = (int) floor(height - offsetH - 1.f);
#else
	x0 = 0;
	y0 = 0;
	x1 = sm_CurrentWindows[g_RenderThreadIndex].uWidth - 1;
	y1 = sm_CurrentWindows[g_RenderThreadIndex].uHeight - 1;
#endif	
}

u32 grcDevice::SetClipPlaneEnable(u32 enable) {
	u32 old = sm_ClipPlaneEnable;
	sm_ClipPlaneEnable = enable & ((1<<RAGE_MAX_CLIPPLANES) - 1); // hardware supports up to RAGE_MAX_CLIPPLANES clip planes
	CHECK_HRESULT(sm_Current->SetRenderState(D3DRS_CLIPPLANEENABLE, sm_ClipPlaneEnable));
	return old;
}

void grcDevice::SetClipPlane(int index,Vec4V_In plane) 
{
	Assert(index >= 0 && index < RAGE_MAX_CLIPPLANES);

	Vec4V outPlane;
	Mat44V WorldSpaceToClippingSpace;
	Mat44V transpose, inverseTranspose;

	// Clip plane distance calculations are performed on points in clipping space post vertex shader.
	Multiply(WorldSpaceToClippingSpace, grcViewport::GetCurrent()->GetProjection(), grcViewport::GetCurrent()->GetViewMtx());
	// Compute the matrix to transform the plane by (better understood by thinking of it as "the matrix which transforms points into plane space" transposed).
	Transpose(transpose, WorldSpaceToClippingSpace);
	InvertFull(inverseTranspose,transpose);
	outPlane = Multiply(inverseTranspose,plane);

	// Set the plane equation in hardware.
	sm_Current->SetClipPlane(index, (float*)&outPlane);
#if !HACK_GTA4
	// Record the plane equation.
	s_ClipPlanes[index] = plane;
#endif //!HACK_GTA4
}

#if !HACK_GTA4 // no shader clip planes
void grcDevice::GetClipPlane(int index,Vec4V_InOut plane) {
	Assert(index >= 0 && index < RAGE_MAX_CLIPPLANES);
	plane = s_ClipPlanes[index];
}
#endif // !HACK_GTA4

grcPresentParameters * grcDevice::GetPresentParameters()
{
	return &s_d3dpp;
}


#if __XENON
void grcDevice::BeginZPass(u32 flags)
{
	sm_Current->BeginZPass(flags);
}


void grcDevice::EndZPass()
{
	sm_Current->EndZPass();
}


void grcDevice::BeginTiledRendering(grcRenderTarget * colorTarget, grcRenderTarget* depthTarget, 
									grcResolveFlags * clearParams, int aaSamples, u32 flags, int extraPixels)
{
	Assert(aaSamples>=0 && aaSamples<=4);
	// NOTE: this needs to compute the real number of tiles, based on the texture size, etc. for now assume 1280x720 or 640x480
	

	if (clearParams)
	{
		memcpy(&s_ClearParams, clearParams, sizeof(grcResolveFlags));
	}
	else
	{
		s_ClearParams.Color.Set(0,0,0,255);
		s_ClearParams.Depth=1.0f;
		s_ClearParams.Stencil = 0x0;
	}

	s_TileFlags = flags;

	int fullWidth,fullHeight;
	grcTextureFormat tileFormat;

	if (colorTarget)
	{
		fullWidth = colorTarget->GetWidth();
		fullHeight = colorTarget->GetHeight();	
		tileFormat = (static_cast<grcRenderTargetXenon *>(colorTarget))->GetFormat();
	}
	else
	{
		fullWidth = GetWidth();
		fullHeight = GetHeight();
		tileFormat = (PARAM_hdr.Get()) ? grctfA2B10G10R10 : grctfA8R8G8B8;
	}

	int tileWidth = fullWidth;
	int tileHeight = fullHeight;

	int hPixelsPerTile = (aaSamples<=1) ? GPU_EDRAM_TILE_WIDTH_1X : ((aaSamples==2) ? GPU_EDRAM_TILE_WIDTH_2X :GPU_EDRAM_TILE_WIDTH_4X);
	int vPixelsPerTile = (aaSamples<=1) ? GPU_EDRAM_TILE_HEIGHT_1X : ((aaSamples==2) ? GPU_EDRAM_TILE_HEIGHT_2X :GPU_EDRAM_TILE_HEIGHT_4X);
	

	if (flags&TILE_VERTICAL)
	{
		int th = XGNextMultiple(fullHeight,TILE_HEIGHT_MULTIPLE);
		
		int largestVTileSize = (hPixelsPerTile*(GPU_EDRAM_TILES/2))/(th/vPixelsPerTile);
		tileWidth = XGNextMultiple(largestVTileSize-(TILE_WIDTH_MULTIPLE-1),TILE_WIDTH_MULTIPLE);

		// NOTE: full Width should be a multiple of TILE_WIDTH_MULTIPLE as should extraPixels
		s_TileCount =  ((fullWidth-extraPixels)+(tileWidth-extraPixels)-1)/(tileWidth-extraPixels);

		Assert (s_TileCount<=MAX_TILES);
	}
	else // TILE_HORIZONTAL
	{
		int tw = XGNextMultiple(fullWidth,TILE_WIDTH_MULTIPLE);

		int largestHTileSize = (vPixelsPerTile*(GPU_EDRAM_TILES/2))/(tw/hPixelsPerTile);  // the largest tile that fits... 
		tileHeight = XGNextMultiple(largestHTileSize-(TILE_HEIGHT_MULTIPLE-1),TILE_HEIGHT_MULTIPLE);

		// NOTE: full Height should be a multiple of TILE_HEIGHT_MULTIPLE as should extraPixels
		s_TileCount = ((fullHeight-extraPixels)+(tileHeight-extraPixels)-1)/(tileHeight-extraPixels);

		Assert (s_TileCount<=MAX_TILES);
	}

	// Expand tile surface dimensions to texture tile size

	bool newTiles = false;

	int tileTextureWidth  = XGNextMultiple( tileWidth, TILE_WIDTH_MULTIPLE );
	int tileTextureHeight = XGNextMultiple( tileHeight, TILE_HEIGHT_MULTIPLE );

	Assert(((tileTextureWidth/vPixelsPerTile) * (tileTextureHeight/hPixelsPerTile)) <= GPU_EDRAM_TILES/2);

	int tileSizeTarget;
	for (tileSizeTarget=0; tileSizeTarget<MAX_TILE_SIZES;tileSizeTarget++)
	{
		if (s_TileColorTarget[tileSizeTarget]) // already allocated a tile target?
		{
			if (s_TileColorTarget[tileSizeTarget] == (grcRenderTarget*)0xffffffff) // skip it if it was used for shadow tiles
				break;

			grcRenderTargetXenon *tgtXenon = static_cast<grcRenderTargetXenon *>(s_TileColorTarget[tileSizeTarget]);

			if (s_TileAATarget[tileSizeTarget] == aaSamples &&
				s_TileColorTarget[tileSizeTarget]->GetWidth() == tileTextureWidth && 
				s_TileColorTarget[tileSizeTarget]->GetHeight() == tileTextureHeight && 
				tgtXenon->GetFormat() == tileFormat)
				break;
		}
		else	// we need to allocate our tile render targets.
		{
			grcDisplayf("TileCount = %d (%d x %d -> %d x %d)",s_TileCount,fullWidth,fullHeight,tileTextureWidth,tileTextureHeight);
			newTiles = true;

			// allocate a color and zbuffer with AA enabled
			grcTextureFactory::CreateParams cp;
			cp.UseFloat = false;
			cp.HasParent = true;
			cp.Multisample = (u8)aaSamples;
			cp.IsResolvable = false;
			cp.Parent = NULL;

			s_TileDepthTarget[tileSizeTarget] = grcTextureFactory::GetInstance().CreateRenderTarget("PredictedTile Depth", grcrtDepthBuffer,  tileTextureWidth, tileTextureHeight, 32, &cp);
			Assert(s_TileDepthTarget[tileSizeTarget] && "tiling depth buffer render target failed to allocate, not enough tiles specified?");

			cp.Parent = s_TileDepthTarget[tileSizeTarget];
			cp.Format = tileFormat;
			//cp.Format = (PARAM_hdr.Get()) ? grctfA2B10G10R10 : grctfA8R8G8B8;
			
			cp.ColorExpBias = sm_ColorExpBias;
			
			s_TileColorTarget[tileSizeTarget] = grcTextureFactory::GetInstance().CreateRenderTarget("PredictedTile Color", grcrtPermanent,  tileTextureWidth, tileTextureHeight, 32,&cp);
			Assert(s_TileColorTarget[tileSizeTarget] && "tiling color buffer render target failed to allocate, not enough tiles specified?");
		
			s_TileAATarget[tileSizeTarget] = aaSamples;
			break;
		}
	}

	if (tileSizeTarget >= MAX_TILE_SIZES)
	{
		Quitf("Error: grcDevice::BeginTiledRendering() - More than %d different tiled rendering sizes have been used!",MAX_TILE_SIZES);
		// we could release the old tile target and make new ones here, but that could lead to a lot of creating/release, and is not like to be useful
	}

	// save the source and destination full image render targets
	s_TileCompositeColorTarget = colorTarget;
	s_TileCompositeDepthTarget = depthTarget;

	// set up the rectangle array and begin tiling

	for (int i=0; i<s_TileCount;i++)
	{
		if(flags&TILE_VERTICAL)
		{
			s_TileRects[i].x1 = i*(tileTextureWidth-extraPixels);
			s_TileRects[i].y1 = 0;
			s_TileRects[i].x2 = min(fullWidth , (i+1)*(tileTextureWidth-extraPixels) + extraPixels );
			s_TileRects[i].y2 = fullHeight;
		}
		else
		{
			s_TileRects[i].x1 = 0;
			s_TileRects[i].y1 = i*(tileTextureHeight-extraPixels);
			s_TileRects[i].x2 = fullWidth;
			s_TileRects[i].y2 = min(fullHeight, (i+1)*(tileTextureHeight-extraPixels) + extraPixels );
		}
	}
#if 0
	for (int i=0; i<s_TileCount;i++)
	{
		grcDisplayf("Tile%d: (%d,%d) to (%d,%d)", i,s_TileRects[i].x1,s_TileRects[i].y1,s_TileRects[i].x2,s_TileRects[i].y2);
	}
#endif

	D3DVIEWPORT9 vp;
	CHECK_HRESULT(sm_Current->GetViewport(&vp));
	((grcRenderTargetXenon*)s_TileColorTarget[tileSizeTarget])->SetColorExpBias( sm_ColorExpBias );
	grcTextureFactory::GetInstance().LockRenderTarget(0,  s_TileColorTarget[tileSizeTarget], s_TileDepthTarget[tileSizeTarget]);

	// let the everyone think we're still rendering to full resolution image, not a tile
	SetSize(fullWidth,fullHeight);
	
	D3DVECTOR4 clearColor;
	clearColor.x = s_ClearParams.Color.GetRedf();
	clearColor.y = s_ClearParams.Color.GetGreenf();
	clearColor.z = s_ClearParams.Color.GetBluef();
	clearColor.w = s_ClearParams.Color.GetAlphaf();

	u32 tileFlags = ((clearParams && (clearParams->ClearColor||clearParams->ClearDepthStencil)) ?
									0 : D3DTILING_SKIP_FIRST_TILE_CLEAR);

	if (s_TileFlags & TILE_ONE_PASS_ZPASS)
		tileFlags |= D3DTILING_ONE_PASS_ZPASS;
	if (s_TileFlags & TILE_FIRST_TILE_INHERITS_DEPTH_BUFFER)
		tileFlags |= D3DTILING_FIRST_TILE_INHERITS_DEPTH_BUFFER;

	sm_Current->BeginTiling( tileFlags , s_TileCount, (D3DRECT*)s_TileRects, &clearColor, FixDepth(s_ClearParams.Depth), s_ClearParams.Stencil );
	
	// Restore 1-z mapping since BeginTiling trashed it.
	CHECK_HRESULT(sm_Current->SetViewport(&vp));
}

void  grcDevice::EndTiledRendering()
{
	//	Insert pre predicated command to save out the tiles color and (optionally) depth to the compose render target.
	// 	endTiling
	for (int i=0;i<s_TileCount;i++)
	{
		sm_Current->SetPredication( D3DPRED_TILE( i ) );
		// resolve the tile to it's place in the composite buffer
		// Resolve fragment 0 of every pixel in the depth/stencil buffer.
		D3DPOINT dstPoint;
		dstPoint.x = s_TileRects[i].x1;
		dstPoint.y = s_TileRects[i].y1;

		D3DVECTOR4 clearColor;
		clearColor.x = s_ClearParams.Color.GetRedf();
		clearColor.y = s_ClearParams.Color.GetGreenf();
		clearColor.z = s_ClearParams.Color.GetBluef();
		clearColor.w = s_ClearParams.Color.GetAlphaf();

		if (s_TileCompositeDepthTarget)
		{
			D3DTexture * depthTex = static_cast<D3DTexture*>(s_TileCompositeDepthTarget->GetTexturePtr());
			CHECK_HRESULT(sm_Current->Resolve( D3DRESOLVE_DEPTHSTENCIL | D3DRESOLVE_FRAGMENT0,(D3DRECT*)&s_TileRects[i], depthTex, &dstPoint, 0, 0, &clearColor, FixDepth(s_ClearParams.Depth), s_ClearParams.Stencil, NULL )); 
		}

		D3DTexture * colorTex;
		if (s_TileCompositeColorTarget)
		{
			colorTex = reinterpret_cast<D3DTexture*>(s_TileCompositeColorTarget->GetTexturePtr());
		}
		else
		{
			colorTex =  reinterpret_cast<D3DTexture*>(grcTextureFactory::GetInstance().GetBackBuffer(false)->GetTexturePtr());
		}

		u32 flags = D3DRESOLVE_RENDERTARGET0;
		if (s_ClearParams.ClearColor && !sm_ColorExpBias)
		{
			flags |= D3DRESOLVE_CLEARRENDERTARGET;
		}
		else
		{
			flags |= D3DRESOLVE_EXPONENTBIAS(-sm_ColorExpBias); 
		}

		if (s_ClearParams.ClearDepthStencil)
		{
			flags |= D3DRESOLVE_CLEARDEPTHSTENCIL;
		}


		CHECK_HRESULT(sm_Current->Resolve( flags, (D3DRECT*)&s_TileRects[i], colorTex, &dstPoint, 0, 0,
							  &clearColor, FixDepth(s_ClearParams.Depth), s_ClearParams.Stencil, NULL));
		
		// needed to reliably clear hierarchical Z until MS fixes the XDK.
		//	sm_Current->ClearF(D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,&s_TileRects[i],&clearColor, clearZ, clearStencil);
	}
	sm_Current->SetPredication( 0 );   // restore preset predication to off
	
#if 0// lame function does not return actual memory used.
DWORD used,remaining;
sm_Current->QueryBufferSpace(&used,&remaining);
grcDisplayf("command buffer memory used: %d;, remaining:%d",used,remaining);
#endif

	sm_Current->EndTiling( 0, NULL, NULL, NULL, 1.0f, 0L, NULL );

	// NOTE: The textures were created as non resolvable so they won't be resolved
	grcTextureFactory::GetInstance().UnlockRenderTarget(0);

	// if they where using the backbuffer, blit it to the screen:
	if ((s_TileCompositeColorTarget==NULL) && !(s_TileFlags&TILE_DONT_MOVE_TO_EDRAM))
	{
		grcStateBlock::SetStates(grcStateBlock::RS_Default,grcStateBlock::DSS_IgnoreDepth,grcStateBlock::BS_Default);
		grcBindTexture(grcTextureFactory::GetInstance().GetBackBuffer(false));
#pragma warning(disable: 4996)
		BlitRect(0,0,GetWidth(),GetHeight(), 0.0f, 0,0,GetWidth(),GetHeight(), Color32(255,255,255,255));
#pragma warning(error: 4996)
  	}
	s_TileCount = -1;
}
// potentially in a begin tilling block


//
// predicated tiling for the depth buffer -> shadow maps on 360
//
void grcDevice::BeginTiledRenderingDepthBuffer(grcRenderTarget* /*depthTarget*/, grcResolveFlags * clearParams, 
											   u32 flags, int extraPixels, int Width, int Height)
{
	if (clearParams)
	{
		memcpy(&s_ClearParams, clearParams, sizeof(grcResolveFlags));
	}
	else
	{
		s_ClearParams.Color.Set(0,0,0,0);
		s_ClearParams.Depth=1.0f;
		s_ClearParams.Stencil = 0x0;
	}

	int tileWidth = Width;
	int tileHeight = Height;

	int hPixelsPerTile = GPU_EDRAM_TILE_WIDTH_1X;
	int vPixelsPerTile = GPU_EDRAM_TILE_HEIGHT_1X;

	if (flags&TILE_VERTICAL)
	{
		int th = XGNextMultiple(Height,TILE_HEIGHT_MULTIPLE);

		int largestVTileSize = (hPixelsPerTile*(GPU_EDRAM_TILES))/(th/vPixelsPerTile);
		tileWidth = XGNextMultiple(largestVTileSize-(TILE_WIDTH_MULTIPLE-1),TILE_WIDTH_MULTIPLE);

		// NOTE: full Width should be a multiple of TILE_WIDTH_MULTIPLE as should extraPixels
		s_TileCount =  ((Width-extraPixels)+(tileWidth-extraPixels)-1)/(tileWidth-extraPixels);

		Assert (s_TileCount<=MAX_TILES);
	}
	else // TILE_HORIZONTAL
	{
		int tw = XGNextMultiple(Width,TILE_WIDTH_MULTIPLE);

		int largestHTileSize = (vPixelsPerTile*(GPU_EDRAM_TILES))/(tw/hPixelsPerTile);  // the largest tile that fits... 
		tileHeight = XGNextMultiple(largestHTileSize-(TILE_HEIGHT_MULTIPLE-1),TILE_HEIGHT_MULTIPLE);

		// NOTE: full Height should be a multiple of TILE_HEIGHT_MULTIPLE as should extraPixels
		s_TileCount = ((Height-extraPixels)+(tileHeight-extraPixels)-1)/(tileHeight-extraPixels);

		Assert (s_TileCount<=MAX_TILES);
	}

	// Expand tile surface dimensions to texture tile size
	bool newTiles = false;

	// the following functions return multiplies of 160x32
	int tileTextureWidth  = XGNextMultiple( tileWidth, TILE_WIDTH_MULTIPLE );
	int tileTextureHeight = XGNextMultiple( tileHeight, TILE_HEIGHT_MULTIPLE );

	Assert(((tileTextureWidth/vPixelsPerTile) * (tileTextureHeight/hPixelsPerTile)) <= GPU_EDRAM_TILES);

	int tileSizeTarget;
	for (tileSizeTarget = 0; tileSizeTarget < MAX_TILE_SIZES; tileSizeTarget++)
	{
		if (s_TileDepthTarget[tileSizeTarget]) // already allocated a tile target?
		{
			if (s_TileColorTarget[tileSizeTarget] == (grcRenderTarget*)0xffffffff && // was this tile used for tiled depth before
				s_TileDepthTarget[tileSizeTarget]->GetWidth() == tileTextureWidth && 
				s_TileDepthTarget[tileSizeTarget]->GetHeight() == tileTextureHeight)
				break;
		}	
		else	// we need to allocate our tile render targets.
		{
			grcDisplayf("TileCount = %d (%d x %d -> %d x %d)",s_TileCount,Width,Height,tileTextureWidth,tileTextureHeight);
			newTiles = true;

			// allocate a "one tile" depth buffer
			grcTextureFactory::CreateParams cp;
			cp.IsResolvable = true;	
			cp.UseHierZ = false;
			cp.HasParent = true;
			cp.Parent = NULL;
			s_TileColorTarget[tileSizeTarget] = (grcRenderTarget*)0xffffffff;	// so the color tiles don't try to reuse use!
			s_TileAATarget[tileSizeTarget] = 0;
			s_TileDepthTarget[tileSizeTarget] = grcTextureFactory::GetInstance().CreateRenderTarget("Tiled Depth Buffer", grcrtDepthBuffer, tileTextureWidth, tileTextureHeight, 32, &cp);
			Assert(s_TileDepthTarget[tileSizeTarget] && "tiling depth buffer render target failed to allocate, not enough tiles specified?");
			break;
		}
	}

	if (tileSizeTarget>=MAX_TILE_SIZES)
	{
		Quitf("Error: grcDevice::BeginTiledRendering() - More than %d different tiled rendering sizes have been used!",MAX_TILE_SIZES);
		// we could release the old tile target and make new ones here, but that could lead to a lot of creating/release, and is not like to be useful
	}

	// set up the rectangle array and begin tiling
	for (int i=0; i< s_TileCount;i++)
	{
		if(flags&TILE_VERTICAL)
		{
			s_TileRects[i].x1 = i*(tileTextureWidth-extraPixels);
			s_TileRects[i].y1 = 0;
			s_TileRects[i].x2 = min(Width , (i+1)*(tileTextureWidth-extraPixels) + extraPixels );
			s_TileRects[i].y2 = Height;
		}
		else
		{
			s_TileRects[i].x1 = 0;
			s_TileRects[i].y1 = i*(tileTextureHeight-extraPixels);
			s_TileRects[i].x2 = Width;
			s_TileRects[i].y2 = min(Height, (i+1)*(tileTextureHeight-extraPixels) + extraPixels );
		}
	}
#if 0
	for (int i=0; i<s_TileCount;i++)
	{
		grcDisplayf("Tile%d: (%d,%d) to (%d,%d)", i,s_TileRects[i].x1,s_TileRects[i].y1,s_TileRects[i].x2,s_TileRects[i].y2);
	}
#endif

	grcTextureFactory::GetInstance().LockRenderTarget(0, NULL, s_TileDepthTarget[tileSizeTarget]);

	// let the everyone think we're still rendering to full resolution image, not a tile
	// this is here to get around a viewport problem ... it resets the size of the whole render target
	SetSize( Width, Height );

	// it can not clear the depth buffer ... just a BUG(TM), so what Steve R. came up with is to skip clearing here
	// and clearing later
	sm_Current->BeginTiling(D3DTILING_SKIP_FIRST_TILE_CLEAR, 
							s_TileCount, 
							(D3DRECT*)s_TileRects, 
							NULL, 
							FixDepth(s_ClearParams.Depth), 
							s_ClearParams.Stencil );

	// automatic clearing does not work properly for depth only render targets
	GRCDEVICE.Clear(false, s_ClearParams.Color, true, s_ClearParams.Depth, s_ClearParams.Stencil);
}

//
// predicated tiling for the depth buffer -> shadow maps
//
void  grcDevice::EndTiledRenderingDepthBuffer(grcRenderTarget* depthTarget)
{
	//	Insert pre predicated command to save out the tiles color and (optionally) depth to the compose render target.
	// 	endTiling
	for (int i = 0; i < s_TileCount; i++)
	{
		sm_Current->SetPredication( D3DPRED_TILE( i ) );

		// Resolve into the full-size depth texture
		D3DPOINT dstPoint;
		dstPoint.x = s_TileRects[i].x1;
		dstPoint.y = s_TileRects[i].y1;

		CHECK_HRESULT(sm_Current->Resolve(D3DRESOLVE_DEPTHSTENCIL,
							(D3DRECT*)&s_TileRects[i], 
							static_cast<D3DTexture*>(depthTarget->GetTexturePtr()), 
							&dstPoint, 0, 0,
							NULL, 
							FixDepth(s_ClearParams.Depth), s_ClearParams.Stencil, NULL ));
	}

	sm_Current->SetPredication( 0 );   // restore preset predication to off

#if 0// lame function does not return actual memory used.
	DWORD used,remaining;
	sm_Current->QueryBufferSpace(&used,&remaining);
	grcDisplayf("command buffer memory used: %d;, remaining:%d",used,remaining);
#endif

	// do not do anything here
	sm_Current->EndTiling( 0, NULL, NULL, NULL, 1.0f, 0L, NULL );

	// do not clear anything
	grcResolveFlags ResolveFlags;
	ResolveFlags.ClearColor = ResolveFlags.ClearDepthStencil = false;

	// NOTE: we should not really use this, as it resolves back the buffer, but we're going to through it away.
	grcTextureFactory::GetInstance().UnlockRenderTarget(0, &ResolveFlags);

	s_TileCount = -1;
}

// PURPOSE: Get the number of tiles, -1 if there is no predicated tiling going on right now.
int grcDevice::GetTileCount()
{
	return s_TileCount;
}

const grcTileRect * grcDevice::GetTileRects(int & count)
{
	if (s_TileCount>0)
	{
		count = s_TileCount;
		return s_TileRects;
	}
	else  // tiling is not active!
	{
		count = 0;
		return NULL; 
	}
}

void grcDevice::SetPredication( u32 predicationFlag )
{
	sm_Current->SetPredication( predicationFlag );
}

void  grcDevice::SaveBackBuffer(grcRenderTarget * colorTarget, grcRenderTarget* depthTarget, grcResolveFlags * clearParams)
{
	//	Insert pre predicated command to save out the tiles color and (optionally) depth to the composite render target.
	// 	endTiling
	D3DVECTOR4 clearColor;
	u32 clearFlags = 0;
	if (clearParams)
	{
		clearColor.x = clearParams->Color.GetRedf();
		clearColor.y = clearParams->Color.GetGreenf();
		clearColor.z = clearParams->Color.GetBluef();
		clearColor.w = clearParams->Color.GetAlphaf();
		if (clearParams->ClearColor)
			clearFlags |= D3DRESOLVE_CLEARRENDERTARGET;
		if (clearParams->ClearDepthStencil)
			clearFlags |= D3DRESOLVE_CLEARDEPTHSTENCIL;
	}

	if (s_TileCount>0)
	{
		for (int i=0;i<s_TileCount;i++)
		{

			sm_Current->SetPredication( D3DPRED_TILE( i ) );
			// resolve the tile to it's place in the composite buffer
			// Resolve fragment 0 of every pixel in the depth/stencil buffer.
	
			D3DPOINT dstPoint;
			dstPoint.x = s_TileRects[i].x1;
			dstPoint.y = s_TileRects[i].y1;

			if (depthTarget)
			{
				CHECK_HRESULT(sm_Current->Resolve( D3DRESOLVE_DEPTHSTENCIL | D3DRESOLVE_FRAGMENT0, (D3DRECT*)&s_TileRects[i], 
									static_cast<D3DTexture*>(depthTarget->GetTexturePtr()), &dstPoint, 0, 0,
									NULL, 0, 0, NULL ));
			}

			if (colorTarget)
			{
				CHECK_HRESULT(sm_Current->Resolve(D3DRESOLVE_EXPONENTBIAS((DWORD)-sm_ColorExpBias) |  
									D3DRESOLVE_RENDERTARGET0 | clearFlags, (D3DRECT*)&s_TileRects[i],  
									static_cast<D3DTexture*>(colorTarget->GetTexturePtr()), &dstPoint, 0, 0,
									&clearColor, FixDepth(s_ClearParams.Depth), s_ClearParams.Stencil, NULL ));
			}
		}

		sm_Current->SetPredication( 0 );   // restore preset predication to off
	}
	else // not using predicated tiling...
	{
		if (depthTarget)
		{
			CHECK_HRESULT(sm_Current->Resolve( D3DRESOLVE_DEPTHSTENCIL | D3DRESOLVE_FRAGMENT0, NULL, 
				static_cast<D3DTexture*>(depthTarget->GetTexturePtr()), NULL, 0, 0, NULL, 0, 0, NULL ));
		}

		if (colorTarget)
		{
			CHECK_HRESULT(sm_Current->Resolve(D3DRESOLVE_EXPONENTBIAS((DWORD)-sm_ColorExpBias) |  
				D3DRESOLVE_RENDERTARGET0 | clearFlags, NULL,  
				static_cast<D3DTexture*>(colorTarget->GetTexturePtr()), NULL, 0, 0,
				&clearColor, FixDepth(s_ClearParams.Depth), s_ClearParams.Stencil, NULL ));
		}
	}
}

void grcDevice::SetShaderGPRAllocation(int vertexThreads, int pixelThreads )
{
	Assert( (vertexThreads+pixelThreads==128 && vertexThreads>=16 && pixelThreads>=16) || (vertexThreads+pixelThreads==0) );
	sm_Current->SetShaderGPRAllocation(0,vertexThreads,pixelThreads);
}

#endif

static D3DPRIMITIVETYPE translate[] = {
		D3DPT_POINTLIST,
		D3DPT_LINELIST,
		D3DPT_LINESTRIP,
		D3DPT_TRIANGLELIST,
		D3DPT_TRIANGLESTRIP,
		D3DPT_TRIANGLEFAN,
#if __XENON
		D3DPT_QUADLIST,
		D3DPT_RECTLIST
#endif
		D3DPT_FORCE_DWORD,	//drawTrisAdj - unsupported
	};

int GetPrimCount(grcDrawMode dm, int indexCount)
{
	int primCount = 0;
	switch( dm )
	{
	case drawPoints:
		primCount = indexCount;
		break;
	case drawLines:
		primCount = indexCount / 2;
		break;
	case drawLineStrip:
		primCount = indexCount - 1;
		break;
	case drawTris:
		primCount = indexCount / 3;
		break;
	case drawTriStrip:
		primCount = indexCount - 2;
		break;
	case drawTriFan:
		primCount = indexCount - 2;
		break;
	case drawQuads:
		primCount = indexCount / 4;
		break;
	case drawRects:
		primCount = indexCount / 3;
		break;
	default:
		Assert(0);
		break;
	}
	return primCount;
}

void grcDevice::DrawIndexedPrimitive(grcDrawMode dm,const grcVertexDeclaration *decl,const grcVertexBuffer &vb,const grcIndexBuffer &ib, int customIndexCount)
{
	Assertf(customIndexCount && customIndexCount <= ib.GetIndexCount(), "WARNING: Invalid Index Count!\tIndex Count: %d, Range: [1, %d]", customIndexCount, ib.GetIndexCount());

#if __XENON && 0	// This is pointless if grcEffect::SetDeclaration is never being called (currently the case)
	IDirect3DVertexShader9* currentVertexShader;
	CHECK_HRESULT(GetCurrent()->GetVertexShader(&currentVertexShader));
	VertexShaderSecretInfo* secretInfo = reinterpret_cast<VertexShaderSecretInfo*>(currentVertexShader->Identifier);
	if (secretInfo && secretInfo->Decl == decl->D3dDecl && !currentVertexShader->IsBound())
	{
		u32 stride = vb.GetVertexStride();
		currentVertexShader->Bind(0,decl->D3dDecl,(DWORD*)&stride,NULL);
	}
#endif // __XENON

	grcStateBlock::Flush();
	GRCDEVICE.SetVertexDeclaration(decl);
	GRCDEVICE.SetIndices(ib);
	GRCDEVICE.SetStreamSource(0,vb,0,vb.GetVertexStride());
#if __XENON

#if COMMAND_BUFFER_SUPPORT && !PRESERVE_HIZ
	if (g_grcCommandBuffer) {
		// Keep HIZ crap from being recorded into the buffer.  More details here:
		// http://download.microsoft.com/download/d/d/a/dda92d49-f77f-4a18-8e89-5b97977f68e2/From%20the%20Trenches%20-%20Xbox%20360%20Development%20War%20Stories.zip 
		// and particularly here, where I stole the code verbatim:
		// http://download.microsoft.com/download/4/e/e/4ee3b9e2-8d61-49f9-8380-a277d27a78e2/Advanced%20Xbox%20360%20Graphics%20Techniques%20using%20Command%20Buffers%20and%20Predicated%20Tiling.zip

		D3DTagCollection_Clear( &sm_Current->m_Pending, D3DTag_Index(D3DTAG_DESTINATIONPACKET), D3DTag_Mask(D3DTAG_DESTINATIONPACKET) );
		D3DTagCollection_Clear( &sm_Current->m_Pending, D3DTag_Index(D3DTAG_WINDOWPACKET), D3DTag_Mask(D3DTAG_WINDOWPACKET) );
		D3DTagCollection_Clear( &sm_Current->m_Pending, D3DTag_Index(D3DTAG_VALUESPACKET), D3DTag_Mask(D3DTAG_VALUESPACKET) );
		D3DTagCollection_Clear( &sm_Current->m_Pending, D3DTag_Index(D3DTAG_CONTROLPACKET), D3DTag_Mask(D3DTAG_CONTROLPACKET) );
		D3DTagCollection_Clear( &sm_Current->m_Pending, D3DTag_Index(D3DTAG_HIZENABLE), D3DTag_Mask(D3DTAG_HIZENABLE) );
	}
#endif

	D3DDevice_DrawIndexedVertices(GRCDEVICE.GetCurrent(),translate[dm],0,0,customIndexCount);
#else
	int idxCount = customIndexCount;
	int primCount = 0;
	switch (dm) {
		case drawPoints: primCount = idxCount; break;
		case drawLines: primCount = idxCount >> 1; break;
		case drawLineStrip: primCount = idxCount - 1; break;
		case drawTris: primCount = idxCount / 3; break;
		case drawTriStrip: primCount = idxCount - 2; break;
		case drawTriFan: primCount = idxCount - 2; break;
		default: Assert(0);
	}

	CHECK_HRESULT(GRCDEVICE.GetCurrent()->DrawIndexedPrimitive(translate[dm],0,0,vb.GetVertexCount(),0,primCount));
#endif

#if DRAWABLE_STATS
	if (g_pCurrentStatsBucket)
	{
		g_pCurrentStatsBucket->TotalIndices += customIndexCount;
		int primCount = GetPrimCount(dm, customIndexCount);
		g_pCurrentStatsBucket->TotalPrimitives += primCount;
		g_pCurrentStatsBucket->TotalDrawCalls++;
	}
#endif
	// GRCDEVICE.ClearStreamSource(0);
}

void grcDevice::DrawPrimitive(grcDrawMode dm, const grcVertexDeclaration *decl,const grcVertexBuffer &vb, int startVertex, int vertexCount)
{
#if __XENON
	IDirect3DVertexShader9* currentVertexShader;
	CHECK_HRESULT(GetCurrent()->GetVertexShader(&currentVertexShader));
	VertexShaderSecretInfo* secretInfo = reinterpret_cast<VertexShaderSecretInfo*>(currentVertexShader->Identifier);
	if (secretInfo && secretInfo->Decl == decl->D3dDecl && !currentVertexShader->IsBound())
	{
		u32 stride = vb.GetVertexStride();
		currentVertexShader->Bind(0,decl->D3dDecl,(DWORD*)&stride,NULL);
	}
#endif // __XENON

	grcStateBlock::Flush();
	GRCDEVICE.SetVertexDeclaration(decl);
	GRCDEVICE.SetStreamSource(0,vb,0,vb.GetVertexStride());

#if __XENON
	D3DDevice_DrawVertices(GRCDEVICE.GetCurrent(),translate[dm],startVertex,vertexCount);
#else
	int vtxCount = vertexCount;
	int primCount = 0;
	switch (dm) {
		case drawPoints: primCount = vtxCount; break;
		case drawLines: primCount = vtxCount >> 1; break;
		case drawLineStrip: primCount = vtxCount - 1; break;
		case drawTris: primCount = vtxCount / 3; break;
		case drawTriStrip: primCount = vtxCount - 2; break;
		case drawTriFan: primCount = vtxCount - 2; break;
		default: Assert(0);
	}
	CHECK_HRESULT(GRCDEVICE.GetCurrent()->DrawPrimitive(translate[dm],startVertex, primCount));
#endif

#if DRAWABLE_STATS
	if (g_pCurrentStatsBucket)
	{
		int primCount = GetPrimCount(dm, vertexCount);
		g_pCurrentStatsBucket->TotalPrimitives += primCount;
		g_pCurrentStatsBucket->TotalDrawCalls++;
	}
#endif
	// GRCDEVICE.ClearStreamSource(0);
}

#if __XENON
void grcDevice::DrawVertices(grcDrawMode dm, int startVertex, int vertexCount)
{
	Assert(grcCurrentVertexBuffers[0]);

	grcStateBlock::Flush();

	CHECK_HRESULT(sm_Current->DrawVertices(translate[dm], startVertex, vertexCount));
#if DRAWABLE_STATS
	if (g_pCurrentStatsBucket)
	{
		int primCount = GetPrimCount(dm, vertexCount);
		g_pCurrentStatsBucket->TotalPrimitives += primCount;
		g_pCurrentStatsBucket->TotalDrawCalls++;
	}
#endif
}
#endif

void grcDevice::DrawIndexedPrimitive(grcDrawMode dm, int startIndex, int indexCount)
{
	Assert(grcCurrentVertexBuffers[0]);

	grcStateBlock::Flush();

	int primCount = GetPrimCount(dm, indexCount);
	CHECK_HRESULT(sm_Current->DrawIndexedPrimitive(translate[dm], 0, 0, grcCurrentVertexBuffers[0]->GetVertexCount(), startIndex, primCount));


#if DRAWABLE_STATS
	if (g_pCurrentStatsBucket)
	{
		g_pCurrentStatsBucket->TotalIndices += indexCount;
		g_pCurrentStatsBucket->TotalPrimitives += primCount;
		g_pCurrentStatsBucket->TotalDrawCalls++;
	}
#endif
}

void grcDevice::DrawPrimitive(grcDrawMode dm, int startVertex, int vertexCount)
{
	Assert(grcCurrentVertexBuffers[0]);

	grcStateBlock::Flush();

	int primCount = GetPrimCount(dm, vertexCount);
	CHECK_HRESULT(sm_Current->DrawPrimitive(translate[dm], startVertex, primCount));

#if DRAWABLE_STATS
	if (g_pCurrentStatsBucket)
	{
		g_pCurrentStatsBucket->TotalPrimitives += primCount;
		g_pCurrentStatsBucket->TotalDrawCalls++;
	}
#endif
}

void grcDevice::SetVertexShaderConstant(int startRegister, const float *data, int regCount
#if (__D3D11)
										, grcCBuffer* /*poConstantBuffer*/
#endif // __WIN32PC		
										)
{
	sm_Current->SetVertexShaderConstantF(startRegister, data, regCount);
}


#if __XENON
void *grcDevice::BeginVertexShaderConstantF(int address,u32 sizeBytes)
{
	D3DVECTOR4 *pOut;
	sm_Current->BeginVertexShaderConstantF1(address,&pOut,sizeBytes>>4);
	return pOut;
}

# if __BANK
void grcDevice::EndVertexShaderConstantF(int)
{
	sm_Current->EndVertexShaderConstantF1();
}
# endif
#endif // __XENON


#if __WIN32PC

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
		s_FenceEventQueries[queryIndex]->Issue(D3DISSUE_END);
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
				IDirect3DQuery9* query = s_FenceEventQueries[queryIndex];
				if (query->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
				{
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
		grcAssert(fence->state != __grcFenceHandle::STATE_QUERY_ISSUED);
		s_FencePool.ImmediateFree(fence);
	}

	void grcDevice::GpuWaitOnPreviousWrites()
	{
		// NOP.  D3D takes care of this with automagic hazzard tracking.
	}

#endif

void grcDevice::KickOffGpu()
{
#if __WIN32PC
	s_KickEventQuery->GetData(NULL, 0, D3DGETDATA_FLUSH);
#else
	sm_Current->InsertFence();
#endif
}

grcOcclusionQuery grcDevice::CreateOcclusionQuery(int XENON_ONLY(tileCount) /*= -1*/)
{
	grcOcclusionQuery query;
	HRESULT hr = S_OK;

	XENON_ONLY(if (tileCount <= 0))
	{
		hr = sm_Current->CreateQuery(D3DQUERYTYPE_OCCLUSION, (QUERY**)&query);
	}
#if __XENON
	else
	{
		hr = sm_Current->CreateQueryTiled(D3DQUERYTYPE_OCCLUSION, tileCount, &query);
	}
#endif //  __XENON

	AssertMsg( !( hr == E_OUTOFMEMORY), " Query allocation failed due to lack of memory" );
	AssertMsg( !( hr == D3DERR_NOTAVAILABLE), " Query not available" );
	return query;
}

void grcDevice::DestroyOcclusionQuery(grcOcclusionQuery& query)
{
#if !__RESOURCECOMPILER
	query->Release();
#endif // !__RESOURCECOMPILER
	query = NULL;
}

void grcDevice::BeginOcclusionQuery(grcOcclusionQuery query)
{
#if !__RESOURCECOMPILER
	static_cast<QUERY*>(query)->Issue(D3DISSUE_BEGIN);
#else
	query;
#endif // !__RESOURCECOMPILER
}

void grcDevice::EndOcclusionQuery(grcOcclusionQuery query)
{
#if !__RESOURCECOMPILER
	static_cast<QUERY*>(query)->Issue(D3DISSUE_END);
#else
	query;
#endif // !__RESOURCECOMPILER
}

bool grcDevice::GetOcclusionQueryData(grcOcclusionQuery query, u32 &numDrawn)
{
#if !__RESOURCECOMPILER
	DWORD pixelsDrawn;
	HRESULT res = static_cast<QUERY*>(query)->GetData(&pixelsDrawn, sizeof(DWORD), 0);
	if(res == S_OK)
	{
		numDrawn = static_cast<u32>(pixelsDrawn);
		return true;
	}
	else
	{
		numDrawn = 0;
		return false;
	}
#else
	query;
	numDrawn;
	return false;
#endif // !__RESOURCECOMPILER
}

#if __XENON
grcConditionalQuery grcDevice::CreateConditionalQuery()
{
	int freeIndex = s_ConditionalQueryFreeList.Allocate();
	return static_cast<grcConditionalQuery>(freeIndex+1);
}

void grcDevice::DestroyConditionalQuery(grcConditionalQuery& query)
{
	if ( !query)
	{
		return;
	}
	s_ConditionalQueryFreeList.Free(query-1);
	query = 0xffffffff;
}

void grcDevice::BeginConditionalQuery(grcConditionalQuery query)
{
	if ( !query)
	{
		return;
	}
	query-=1;
	AssertMsg(s_ConditionalQueryInProgress == 0xffffffff, "Another query already active");
	Assertf(query < kMaxConditionalQueries, "Invalid query %x",query);

	// Doing this to match the PS3 (render stuff while query is active)
	// We could add an argument to switch the behaviour but we will have to do 
	// the same on PS3. PS3 does not have this flag, so we will have to 
	// explicitly switch off color writes and restore the previous state back
	// in EndConditionalQuery
	sm_Current->BeginConditionalSurvey(query,0);
	//sm_Current->BeginConditionalSurvey(query,D3DSURVEYBEGIN_CULLGEOMETRY);

	ASSERT_ONLY(s_ConditionalQueryInProgress = query);
}

void grcDevice::EndConditionalQuery(grcConditionalQuery query)
{
	if ( !query)
	{
		return;

	}
	query-=1;
	AssertMsg(s_ConditionalQueryInProgress == query, "Another query already active");
	Assertf(query < kMaxConditionalQueries, "Invalid query 0x%x",query);

	sm_Current->EndConditionalSurvey(0);

	ASSERT_ONLY(s_ConditionalQueryInProgress = 0xffffffff);
}

void grcDevice::BeginConditionalRender(grcConditionalQuery query)
{
	if ( !query)
	{
		return;
	}
	query-=1;
	Assertf(query < kMaxConditionalQueries, "Invalid query %x",query);

	sm_Current->BeginConditionalRendering(query);
}

void grcDevice::EndConditionalRender(grcConditionalQuery query)
{
	if ( !query)
	{
		return;
	}
	query-=1;
	sm_Current->EndConditionalRendering();
}
#else
grcConditionalQuery grcDevice::CreateConditionalQuery()
{
	return (grcConditionalQuery)0;
}

void grcDevice::DestroyConditionalQuery(grcConditionalQuery& query)
{
	(void)query;
}

void grcDevice::BeginConditionalQuery(grcConditionalQuery query)
{
	(void)query;
}

void grcDevice::EndConditionalQuery(grcConditionalQuery query)
{
	(void)query;
}

void grcDevice::BeginConditionalRender(grcConditionalQuery query)
{
	(void)query;
}

void grcDevice::EndConditionalRender(grcConditionalQuery query)
{
	(void)query;
}
#endif // __XENON

#if __WIN32PC
#undef grcDevice
int grcDevice::InitSingleton()
{
	// Always try DX9, particularly since D3DPERF_... functions only seem to exist there.
	if (!g_hD3D9)
		g_hD3D9 = ::LoadLibrary("D3D9.DLL");

	if (g_hD3D9 && !g_Direct3DCreate9)
	{
		OutputDebugString("Direct9 DLL's found.\n");
		g_Direct3DCreate9 = (IDirect3D9 * (WINAPI *)(UINT SDKVersion))::GetProcAddress(g_hD3D9, "Direct3DCreate9");
		g_D3DPERF_BeginEvent = (int (WINAPI *)( D3DCOLOR col, LPCWSTR wszName )) ::GetProcAddress(g_hD3D9,"D3DPERF_BeginEvent");
		g_D3DPERF_EndEvent = (int (WINAPI *)( void ))::GetProcAddress(g_hD3D9,"D3DPERF_EndEvent");
		g_D3DPERF_GetStatus = (int (WINAPI *)( void ))::GetProcAddress(g_hD3D9,"D3DPERF_GetStatus");
	}



	// ...if DX10 isn't available, actually try to instantiate a D3D9 device.
	if (g_Direct3DCreate9)
	{
		static grcDevice theDevice9;
		g_pGRCDEVICE = &theDevice9;
		return 900;
	}
	else
		return 0;
}
#endif

#if DRAWABLE_STATS
void grcDevice::SetFetchStatsBuffer(drawableStats* dstPtr) 
{
	g_pCurrentStatsBucket = dstPtr;
}
#endif

}	// namespace rage

#endif	// __D3D
