//
// grcore/device.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_DEVICE_H
#define GRCORE_DEVICE_H

#include "shaderlib/rage_constants.h"
#include "bank/bank.h"
#include "vector/color32.h"
#include "atl/functor.h"
#include "data/safestruct.h"
#include "grcore/allocscope.h"
#include "grcore/config.h"
#include "grcore/drawmode.h"
#include "grcore/gfxcontextcontrol.h"
#include "grcore/vertexdecl.h"
#include "system/nelem.h"
#include "system/service.h"
#include "grcore/effect.h"
#include "grcore/monitor.h"

#if RSG_DURANGO
#include <xdk.h>
#endif // RSG_DURANGO

// It's likely we'll drop the whole wrapper idea for later Durango builds, though it may still provide useful for now
#define DEVICE_USE_D3D_WRAPPER		((RSG_PC && !(__RESOURCECOMPILER || __TOOL)) || (0 && RSG_DURANGO))
#define DEVICE_USE_FAST_TEXTURES	( 1 && RSG_DURANGO && __D3D11_MONO_DRIVER && !(__RESOURCECOMPILER || __TOOL) )
#define DEVICE_RESOLVE_RT_CONFLICTS	( 1 && RSG_PC && __D3D11 )

#define ATI_SUPPORT		(RSG_PC && !__RESOURCECOMPILER && !__TOOL && !__GAMETOOL)
#define ATI_EXTENSIONS	(RSG_PC && __D3D11 && !__RESOURCECOMPILER && !__TOOL && !__GAMETOOL)
#define NV_SUPPORT		(RSG_PC && !__RESOURCECOMPILER && !__TOOL && !__GAMETOOL)

#define DEVICE_CLIP_PLANES	(__D3D11 || RSG_ORBIS)
#define DEVICE_MSAA			(__D3D11 || RSG_ORBIS)
#define DEVICE_EQAA			(RSG_ORBIS || RSG_DURANGO)
#define DEVICE_GPU_WAIT		(__PPU || RSG_DURANGO || RSG_ORBIS)
#define DEVICE_IM_INSTANCED_INDEXED	(__D3D11 || RSG_ORBIS)

#define USE_AMD_SHADOW_LIB	(RSG_PC && 1)

#define USE_NV_TXAA RSG_PC && !__GAMETOOL
#if USE_NV_TXAA
#define TXAA_ONLY(x)	x
#else
#define TXAA_ONLY(x)	
#endif // USE_NV_TXAA

#if DEVICE_MSAA
#define MSAA_ONLY(...)		__VA_ARGS__
#else
#define MSAA_ONLY(...)
#endif

#if DEVICE_EQAA
#define EQAA_ONLY(...)		__VA_ARGS__
#else
#define EQAA_ONLY(...)
#endif

#define DEVICE_INVALID_FENCEHANDLE	NULL

#define API_QUADS_SUPPORT  (__XENON || __PS3)
#define API_QUADS_VERTS_PER_QUAD	((__XENON || __PS3) ? 4 : 6)

#if RSG_PC
#define MAX_GPUS			4
#else
#define MAX_GPUS			1
#endif

#if NV_SUPPORT
#define NV_SUPPORT_ONLY(...)	__VA_ARGS__
#else
#define NV_SUPPORT_ONLY(...)
#endif

#if __WIN32PC || RSG_DURANGO
#define RESOURCE_MANAGED   1 // D3DPOOL_MANAGED
#define RESOURCE_UNMANAGED 0 // D3DPOOL_DEFAULT
#define RESOURCE_SYSTEMMEM 2 // D3DPOOL_SYSTEMMEM
#define RESOURCE_POOL RESOURCE_MANAGED
#define	D3DRUNTIME_MEMORYALIGNMENT (4096)
#define D3D_AMD_LARGEALIGNMENT (65536)

#if NV_SUPPORT
// Message for the windows message pump to know when the stereo state has changed.
#define WM_STEREO_CHANGE (WM_USER + 20)
#endif

#define WIN_MINIMIZABLE 1
#define WIN_MAXIMIZABLE 0
#define WIN_RESIZABLE 0
#define WIN_CLOSEABLE 1

#if RSG_PC
#define DIRECT_CONTROL_OF_ALT_ENTER 1

struct DXGI_ADAPTER_DESC;
namespace rage {
	struct grcDisplayInfo
	{
		DXGI_ADAPTER_DESC* oAdapterDesc;
		int iTargetAdapter;

		s64 numOfCores;
		s64 gpuClockMax;
		s64 memClockMax;
		s64 videoMemSize;
		s64 sharedMemSize;
		s64 memBandwidth;
	};
}
struct tagRECT;
#endif

enum DeviceManufacturer
{
	ATI,
	NVIDIA,
	INTEL,
	UNKNOWN_DEVICE,
	DM_LAST
};

enum DeviceFeatureSet
{
	VERTEX_TEXTURE_SAMPLING					= 0x1,		// Vertex Shader can read textures
	ZSAMPLE_STENCILOPERATIONS_SUPPORT		= 0x2,		// Sampleable Z support stencil operations
	PROPER_ZSAMPLE							= 0x4,		// Sampleable Z Textures return values in 0 - 1 - Low end NVidia does not support
	ZSAMPLE_FROM_VERTEX_SHADER				= 0x8,		// Sampleable Z Textures can be read in Vertex Shader
	Z_MSAA_SAMPLE							= 0x10,		// Z-Texture MSAA Sampleable
	TESSELLATION							= 0x20,		// Tessellation support
	GEOMETRYSHADER							= 0x40,		// Geometry Shader support
	AUTOSTEREO								= 0x80,		// Automatic Stereo in Driver support
	DEPTH_BOUNDS							= 0x100,	// Depth Bound support - NVidia only
	MULTIGPU								= 0x200,	// MultiGPUs support
	RENDER_TO_VERTEX_BUFFER					= 0x400,	// Ati specific R2VB support
	MT_COMMANDLIST							= 0x800,	// command list for multi-threading
	MT_CONCURRENTCREATE						= 0x1000,	// concurrent resource creation for multi-threading
	COMPUTE_SHADER_40						= 0x2000,	// Supports compute shader 4.0 only
	COMPUTE_SHADER_50						= 0x4000	// Supports compute shader 5.0 only
};

#define SEPARATE_FULLSCREEN_RES 0

#endif // __WIN32PC

#define DEPTH_BOUNDS_SUPPORT	(__PPU || RSG_PC || RSG_ORBIS || (RSG_DURANGO && _XDK_VER >= 10812 ))
#define UAV_OVERLAP_SUPPORT		(1 && RSG_PC && ATI_EXTENSIONS)

#if __XENON
// Abstract wrapper for a Direct3D vertex shader
typedef struct D3DVertexShader grcVertexShader;
// Abstract wrapper for a Direct3D pixel shader
typedef struct D3DPixelShader grcPixelShader;
// Abstract wrapper for a Direct3D device
typedef struct D3DDevice grcDeviceHandle;
typedef struct D3DDevice grcContextHandle;
// Abstract wrapper for a Direct3D texture
typedef struct D3DBaseTexture grcDeviceTexture;
// Abstract wrapper for a Direct3D surface
typedef struct D3DSurface grcDeviceSurface;
// Abstract wrapper for a Direct3D surface parameter
typedef struct _D3DSURFACE_PARAMETERS grcDeviceSurfaceParameters;
typedef struct D3DCommandBuffer grcCommandBuffer;
typedef struct D3DQuery* grcOcclusionQuery;
// DX10 Only feature
typedef void grcDeviceView;
typedef void grcResource;
#else
struct IUnknown;
struct DXGI_ADAPTER_DESC;

#if __RESOURCECOMPILER
	#define grcOcclusionQuery int
	#define	grcSwapChain void
#elif __D3D11
	struct DXGI_RATIONAL;
	struct DXGI_SAMPLE_DESC;
	typedef IUnknown*					grcOcclusionQuery;
	typedef IUnknown					grcSwapChain;
#elif RSG_PC
	typedef IUnknown* grcOcclusionQuery;
	typedef IUnknown grcSwapChain;
#elif RSG_ORBIS
	namespace rage { class grcOcclusionQueryGnm; }
	typedef rage::grcOcclusionQueryGnm* grcOcclusionQuery;
#else
	typedef rage::u32 grcOcclusionQuery;
#endif

typedef unsigned long grcDeviceSurfaceParameters;	// Not supported on PC builds
struct grcCommandBuffer;
struct IDxDiagContainer;
#endif

#if RSG_ORBIS
	namespace rage { class grcConditionalQueryGnm; }
	typedef rage::grcConditionalQueryGnm* grcConditionalQuery;
#elif __D3D11
	typedef IUnknown* grcConditionalQuery;
#else
	typedef rage::u32 grcConditionalQuery;
#endif

typedef struct _D3DPRESENT_PARAMETERS_ grcPresentParameters;
namespace rage { struct __grcFenceHandle; }
typedef rage::__grcFenceHandle *grcFenceHandle;

#define COMMAND_BUFFER_SUPPORT	(0)

// DOM-IGNORE-BEGIN
struct ID3DXEffect;
struct HWND__;
struct tagRECT;
struct tagRGNDATA;
class ICgFXEffect;
// DOM-IGNORE-END

#if __RESOURCECOMPILER
#define grcTextureObject void
#elif __XENON
struct D3DBaseTexture;
#define grcTextureObject D3DBaseTexture
#elif __D3D11
struct IUnknown;
struct ID3D11Resource;
typedef ID3D11Resource grcTextureObject;
#elif RSG_PC || RSG_DURANGO
typedef IUnknown grcTextureObject;
#elif __PS3
typedef struct CellGcmContextData grcContextHandle;
struct CellGcmTexture;
struct PackedCellGcmTexture;
#if __PPU || !USE_PACKED_GCMTEX
#define grcTextureObject CellGcmTexture
#else
#define grcTextureObject PackedCellGcmTexture
#endif
#elif RSG_ORBIS
namespace sce { namespace Gnm { class Texture; class RenderTarget; class DepthRenderTarget; }  }
#define grcTextureObject sce::Gnm::Texture
namespace rage { class grcGfxContext; }
typedef rage::grcGfxContext grcContextHandle;
typedef rage::grcGfxContext grcContextCommandList;
#endif

// Toggle support for protecting EDRAM regions
#define ENABLE_EDRAM_PROTECTION	(__XENON && (__BANK||__DEV))

#if ENABLE_EDRAM_PROTECTION
	#define PROTECT_EDRAM_SURFACE(SURFACE_PTR)	GRCDEVICE.BeginProtectingEDRAM(SURFACE_PTR)
	#define RELEASE_EDRAM_SURFACE(SURFACE_PTR)	GRCDEVICE.EndProtectingEDRAM(SURFACE_PTR)
	#define VERIFY_EDRAM_SURFACE(SURFACE_PTR)                                 \
		{                                                                     \
			if (SURFACE_PTR && !AssertVerify(!IsProtectedEDRAM(SURFACE_PTR))) \
			   grcErrorf("Attempting to lock a protected region of EDRAM!");  \
		}
	#define PROTECT_EDRAM_TARGET(TARGET_PTR)   PROTECT_EDRAM_SURFACE(((grcRenderTargetXenon*)TARGET_PTR)->GetSurface(false))
	#define RELEASE_EDRAM_TARGET(TARGET_PTR)   RELEASE_EDRAM_SURFACE(((grcRenderTargetXenon*)TARGET_PTR)->GetSurface(false))
	#define VERIFY_EDRAM_TARGET(TARGET_PTR)    VERIFY_EDRAM_SURFACE(((grcRenderTargetXenon*)TARGET_PTR)->GetSurface(false))
#else // ENABLE_EDRAM_PROTECTION
	#define PROTECT_EDRAM_SURFACE(SURFACE_PTR)
	#define RELEASE_EDRAM_SURFACE(SURFACE_PTR)
	#define VERIFY_EDRAM_SURFACE(SURFACE_PTR)
	#define PROTECT_EDRAM_TARGET(TARGET_PTR)
	#define RELEASE_EDRAM_TARGET(TARGET_PTR)
	#define VERIFY_EDRAM_TARGET(TARGET_PTR)  
#endif // ENABLE_EDRAM_PROTECTION

#if RSG_PC
	#define MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU (GRCDEVICE.GetMaxQueuedFrames())	
#else
	#define MAX_FRAMES_RENDER_THREAD_AHEAD_OF_GPU (RSG_XENON ? 2 : 1)	
#endif

namespace rage {

	extern __THREAD grcContextHandle *g_grcCurrentContext;

#if DEVICE_MSAA
# if RSG_PC
	class grcRenderTargetDX11;
	typedef grcRenderTargetDX11		grcRenderTargetMSAA;
# elif RSG_DURANGO
	class grcRenderTargetDurango;
	typedef grcRenderTargetDurango	grcRenderTargetMSAA;
# elif RSG_ORBIS
	class grcRenderTargetGNM;
	typedef grcRenderTargetGNM		grcRenderTargetMSAA;
# endif // platforms
#endif // DEVICE_MSAA

	extern bool grcFlashEnable;

	enum grcMrt
	{
		grcmrtColor0,
		grcmrtColor1,
		grcmrtColor2,
		grcmrtColor3,
#if __WIN32
		grcmrtColor4,
		grcmrtColor5,
		grcmrtColor6,
		grcmrtColor7,
#endif
		
		grcmrtColorCount,

		grcmrtDepthStencil = grcmrtColorCount,

		grcmrtCount
	};
	enum grcUsage
	{
		grcUsageDefault = 0, // Matches DX10
		grcUsageImmutable,
		grcUsageDynamic,
		grcUsageStage,
		grcUsageCount
	};
	enum grcBindFlag
	{
		grcBindNone				 = 0x0,
		grcBindVertexBuffer		 = 0x1, // Matches DX10
		grcBindIndexBuffer		 = 0x2,
		grcBindConstantBuffer	 = 0x4,
		grcBindShaderResource	 = 0x8,
		grcBindStreamOutput		 = 0x10,
		grcBindRenderTarget		 = 0x20,
		grcBindDepthStencil		 = 0x40,
		grcBindUnorderedAccess	 = 0x80,
	};
	enum grcCPUAccess
	{
		grcCPUNoAccess	= 0x0,
		grcCPUWrite		= 0x10000L, // Matches DX10
		grcCPURead		= 0x20000L,
	};
	enum grcResourceMiscFlag
	{
		grcResourceNone					= 0x0,
		grcResourceGenerateMips			= 0x1, // Matches DX10
		grcResourceSharedBetweenGPUS	= 0x2,
		grcResourceTextureCube			= 0x4, // Tells driver that texture will be treated as a cube map resource
	};
	enum grcUAVViews
	{
		grcMaxUAVViews = 8 //D3D11_PS_CS_UAV_REGISTER_COUNT
	};

	class grcVertexBuffer;
	class grcIndexBuffer;
	class grcRenderTarget;
	class grcEffect;
#if __D3D11 || RSG_ORBIS
	class grmShader;
#endif
#if __WIN32PC
	class grcTexture;

	typedef void (*FOCUSCALLBACK)();
	typedef void (*RENDERCALLBACK)();
#endif
#if __D3D11
	class grcBufferD3D11;
	class grcBufferD3D11Resource;
#endif

#if DEVICE_MSAA
	enum ResolveType
	{
		ResolveHW,					// fast hardware resolve
		ResolveSW_Simple,			// a software resolve that emulates the hardware one
		ResolveSW_NoAnchors,		// resolve non-anchors only
		ResolveSW_HighQuality,		// average neighboring pixels' samples for invalid inner samples
		ResolveSW_Simple_NanDetect	
	};
#endif // DEVICE_MSAA

// Toggle support for inverting depth via the projection matrix (instead of via the viewport, for example).
#if SUPPORT_INVERTED_PROJECTION 
	extern DECLARE_MTR_THREAD bool g_grcDepthInversionInvertProjection;
#endif // SUPPORT_INVERTED_PROJECTION

	/*
	This structure represents the final window transform to the screen.
	It also includes the ability to map to a subrange of the depth buffer.
	*/
	struct grcWindow {
	private:
		// friend class grcDevice;
		friend class grcViewport;
		float m_NormX, m_NormY, m_NormWidth, m_NormHeight;
		float m_MinZ, m_MaxZ;
	public:
		grcWindow() : m_NormX(0), m_NormY(0), m_NormWidth(1), m_NormHeight(1), m_MinZ(0), m_MaxZ(1) { }
		explicit grcWindow(datResource&) {}
		IMPLEMENT_PLACE_INLINE(grcWindow);
#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct& s)
		{
			SSTRUCT_BEGIN(grcWindow)
				SSTRUCT_FIELD(grcWindow, m_NormX)
				SSTRUCT_FIELD(grcWindow, m_NormY)
				SSTRUCT_FIELD(grcWindow, m_NormWidth)
				SSTRUCT_FIELD(grcWindow, m_NormHeight)
				SSTRUCT_FIELD(grcWindow, m_MinZ)
				SSTRUCT_FIELD(grcWindow, m_MaxZ)
			SSTRUCT_END(grcWindow)
		}
#endif
		inline float GetNormX() const { return m_NormX; }
		inline float GetNormY() const { return m_NormY; }
		inline float GetNormWidth() const { return m_NormWidth; }
		inline float GetNormHeight() const { return m_NormHeight; }
		inline float GetMinZ() const { return m_MinZ; }
		inline float GetMaxZ() const { return m_MaxZ; }
		inline int GetX() const;
		inline int GetY() const;
		inline int GetWidth() const;
		inline int GetHeight() const;
	};

#define GRCDEVICE_IS_STATIC		(!__WIN32PC)

#if GRCDEVICE_IS_STATIC
#define grcEntry static
#define grcPure
#else
#define grcEntry static
#define grcPure	// = 0
#endif

	// Abstraction of Xenon clear-during-resolve flags
	struct grcResolveFlags {
		grcResolveFlags()
			: Depth(1.0f)
			, BlurKernelSize( 1.0f )
			, Color(0,0,0)
			, Stencil(0)
			, ColorExpBias(0)
			, ClearColor(false)
			, ClearDepthStencil(false)
			, BlurResult(false)
			, NeedResolve(true)
			, MipMap(true)
		{}

		float Depth;
		float BlurKernelSize; // size of blur
		Color32 Color;
		u32 Stencil;
		int ColorExpBias;
		bool ClearColor;
		bool ClearDepthStencil;
		bool BlurResult;
		bool NeedResolve;
		bool MipMap;
	};

	typedef grcResolveFlags* grcResolveFlagsMrt[grcmrtColorCount];

	// stand in for D3DRect
	struct grcTileRect {
		int x1;
		int y1;
		int x2;
		int y2;
	};

	typedef struct _grcSrcData { // Matches DX10 structure
		const void *pSystemMem;
		unsigned int SysMemPitch;
		unsigned int SysMemSlicePitch;
	} grcSrcData;

#if __D3D11
	struct RefreshRate
	{
		u32 Numerator, Denominator;
		RefreshRate(): Numerator(0), Denominator(1)	{}
		RefreshRate(const u32 value)				{ *this = value; }
		void operator=(u32 value)					{ Numerator=value; Denominator=1; }
		operator u32()								{ return Numerator / Denominator; }
		void operator=(const DXGI_RATIONAL& rat);
		bool operator==(const DXGI_RATIONAL& rat) const;
	};
#else
	typedef u32 RefreshRate;
#endif	//__D3D11

	class grcDisplayWindow 
	{
	public:
		grcDisplayWindow(u32 Width = 0, u32 Height = 0, RefreshRate RefreshRate = 0, bool Fullscreen = false, u32 FrameLock = 0) : uWidth(Width), uHeight(Height), uRefreshRate(RefreshRate), bFullscreen(Fullscreen), uFrameLock(FrameLock) {}
		void Init(u32 Width, u32 Height, RefreshRate RefreshRate, bool Fullscreen, u32 FrameLock) { uWidth=Width; uHeight=Height; uRefreshRate=RefreshRate; bFullscreen=Fullscreen; uFrameLock=FrameLock; }
		void FillMissingFields(const grcDisplayWindow& oSrc) { if (uWidth == 0) uWidth = oSrc.uWidth; if (uHeight == 0) uHeight = oSrc.uHeight; if (uRefreshRate == 0) uRefreshRate = oSrc.uRefreshRate; }
		grcDisplayWindow& operator=(const grcDisplayWindow& oSrc) { memcpy(this, &oSrc, sizeof(grcDisplayWindow)); return *this; }
		bool operator==(const grcDisplayWindow& oSrc) const { return memcmp(&oSrc, this, sizeof(grcDisplayWindow)) ? false : true; }
		bool operator!=(const grcDisplayWindow& oSrc) const { return !(operator==(oSrc)); }

		u32		uWidth;
		u32		uHeight;
		RefreshRate		uRefreshRate;
		bool	bFullscreen;
		u32		uFrameLock;
	};

#if __XENON
	extern int grcCommandBufferHeapSize;		// in bytes
	extern int grcCommandBufferHeapNodes;		// should be max number of command buffers you expect to have around
#elif __PS3
	extern int grcCommandBufferHeapSize;		// in bytes, drawn from game heap memory
#endif

	extern grcCommandBuffer *g_grcCommandBuffer;

	// drawable stats
	// tbr: maybe it'd be better to merge this with spuDrawableStats
	// and have a common interface for drawable stats
#define DRAWABLE_STATS (__BANK && (RSG_PC || RSG_DURANGO || RSG_ORBIS))
#if DRAWABLE_STATS
	enum
	{
		DCC_NO_CATEGORY = 0,
		DCC_LOD,
		DCC_SLOD1,
		DCC_SLOD2,
		DCC_SLOD3,
		DCC_SLOD4,
		DCC_PROPS,
		DCC_VEG,
		DCC_PEDS,
		DCC_VEHICLES,
		DCC_MAX_CONTEXT
	};
	struct drawableStats {
		u32 TotalIndices;							// Number of indices drawn
		u32 TotalPrimitives;						// Number of primitives drawn
		u32 TotalDrawCalls;							// Number of draw calls
		u32 MissingTechnique;						// Number of times grmShader::BeginDraw returned zero passes
		u32 DrawableDrawCalls;						// Number of rmcDrawable::Draw calls processed
		u32 DrawableDrawSkinnedCalls;				// Number of rmcDrawable::DrawSkinned calls processed
		u32 ModelsCulled;							// Number of models that were culled entirely by their root AABB's
		u32 ModelsDrawn;							// Number of models that were rendered (any model not culled is drawn)
		u32 GeomsCulled;							// Number of geometries that were culled by their local AABB's (may include ones that were not even in this bucket because of how the code is set up)
		u32 GeomsDrawn;								// Number of geometries that were drawn (were not culled, and were in current render bucket)
		u16 DrawCallsPerContext[DCC_MAX_CONTEXT];	// Number of draw calls per context of various types

		drawableStats& operator+=(const drawableStats& other)
		{
			TotalIndices += other.TotalIndices;
			TotalPrimitives += other.TotalPrimitives;
			TotalDrawCalls += other.TotalDrawCalls;
			MissingTechnique += other.MissingTechnique;
			DrawableDrawCalls += other.DrawableDrawCalls;
			DrawableDrawSkinnedCalls += other.DrawableDrawSkinnedCalls;
			ModelsCulled += other.ModelsCulled;
			ModelsDrawn += other.ModelsDrawn;
			GeomsCulled += other.GeomsCulled;
			GeomsDrawn += other.GeomsDrawn;
			for(int i=0; i<DCC_MAX_CONTEXT; ++i)
				DrawCallsPerContext[i] += other.DrawCallsPerContext[i];
			return *this;
		}
	};
	extern __THREAD drawableStats* g_pCurrentStatsBucket;
#define DRAWABLE_STATS_INC(x) if(g_pCurrentStatsBucket) { g_pCurrentStatsBucket->x++; }
#define DRAWABLE_STATS_ONLY(...)  __VA_ARGS__
#else
#define DRAWABLE_STATS_INC(x)
#define DRAWABLE_STATS_ONLY(...)
#endif

	#define MAX_CONSTANT_BUFFER				14
#if __D3D11
	#define MAX_RAGE_VERTEXBUFFER_SLOTS		16
	#define MAX_RESOURCES					128

#if !RSG_DURANGO
	struct D3DVERTEXSTREAMINFO
	{
		//const grcVertexBuffer* pVertexBuffer[MAX_RAGE_VERTEXBUFFER_SLOTS];
		IUnknown* pVertexBuffer[MAX_RAGE_VERTEXBUFFER_SLOTS];
		u32 offsetInBytes[MAX_RAGE_VERTEXBUFFER_SLOTS];
		u32 stride[MAX_RAGE_VERTEXBUFFER_SLOTS];
	};
#endif
#endif // __D3D11

	/*
	PURPOSE
	Abstraction of a Direct3D device for higher-level code.
	<FLAG Component>
	*/
	class grcDevice {
		friend class grcEffect;
		friend class grcTextureFactoryDX11;	// temporary, hopefully

	public:
		// Abstraction of a Direct3D HRESULT.
		typedef u32 Result;

#if __WIN32PC
		static int InitSingleton();
#else
		static int InitSingleton() { return 1; }
#endif

#if __WIN32 || RSG_DURANGO
		grcEntry bool IsCreated() { return (GetCurrent() != NULL); }
#endif

		// PURPOSE:	Initialize the class; create the window if necessary and attach the Direct3D device to it.
		// NOTES:	On Xenon, grcCommandBufferHeapSize and grcCommandBufferHeapNodes should be initialized to
		//			the amount of memory you want to allocate for command buffers (in bytes), and the total
		//			number of command buffers you expect to have in use at once.  This gets allocated from the
		//			XTL heap, so adjust your memory reservations accordingly.  It defaults to a small number
		//			(only 64k) so if your application is actively using command buffers it will want to raise
		//			the value.
		grcEntry void InitClass(bool inWindow, bool topMost=false) grcPure;

		// PURPOSE:	Shut down the class; destroy the window if we created it and release the Direct3D device.
		grcEntry void ShutdownClass() grcPure;

		// PURPOSE: Clear up resources that are still bound to the device so that resource can be cleared out properly
		grcEntry void PrepareForShutdown() grcPure;

		// PURPOSE:	Run the windows message pump and update input devices.  Should call this exactly once per frame.
		static void Manage();

		grcEntry void BeginDXView() grcPure;

		grcEntry void EndDXView() grcPure;

		// PURPOSE:	Call this to begin rendering a frame.  Abstraction of BeginScene.
		grcEntry bool BeginFrame() grcPure;

		// PURPOSE:	Call this to finish rendering a frame.  Abstraction of EndScene and Present.
		grcEntry void EndFrame(const grcResolveFlags *flags = NULL) grcPure;

#if RSG_PC && __D3D11
		// PURPOSE: Expose the Swap Chain to determine if there are any illegitimate buffers writing to the screen
		static grcSwapChain * GetBackupSwapChain();
#endif
		// PURPOSE: Flags for AllocFence.  Values may be ORed together unless
		//          specific flags are specified as mutually exclusive.
		//          AllocFence flags defaults to 0, ie, none of these flags set.
		// NOTE:    These values most not overlap the WRITE_FENCE_* flags, since
		//          they are combined together in AllocFenceAndGpuWrite.
		enum
		{
			// By default, fences are initialized as "done".  This flags causes
			// it to be initialized as "pending".  Could be useful for code
			// where the CPU polls IsFencePending().
			ALLOC_FENCE_INIT_AS_PENDING                         = 0x00000001u,

#if GRCCONTEXT_ALLOC_SCOPES_SUPPORTED
			// Instead of the allocated fence's lifetime being valid until
			// *puFreeFence is called, the fence's lifetime is the current
			// grcContextAllocScope.
			//
			// Requires GRCCONTEXT_ALLOC_SCOPES_SUPPORTED.
			ALLOC_FENCE_ALLOC_SCOPE_LIFETIME                    = 0x00000002u,
#endif

			// Internal use only.
			PRIVATE__ALLOC_FENCE_ALL_FLAGS                      = 0x0000ffffu,
		};

		// PURPOSE: Allocate a fence handle.  Must be freed by CpuFreeFence or
		//          GpuFreeFence, unless ALLOC_FENCE_ALLOC_SCOPE_LIFETIME is
		//          used.
		grcEntry grcFenceHandle AllocFence(u32 flags=0) grcPure;

		// PURPOSE: Marks a fence as pending from the CPU.
		grcEntry void CpuMarkFencePending(grcFenceHandle fence) grcPure;

		// PURPOSE: Marks a fence as done from the CPU.
		grcEntry void CpuMarkFenceDone(grcFenceHandle fence) grcPure;

		// PURPOSE: Generates a GPU command to mark the fence as pending.
		grcEntry void GpuMarkFencePending(grcFenceHandle fence) grcPure;

		// PURPOSE: Flags for GpuMarkFenceDone.  Values may be ORed together
		//          unless specific flags are specified as mutually exclusive.
		//          GpuMarkFenceDone flags defaults to 0, ie, none of these
		//          flags set.
		// NOTE:    These values most not overlap the ALLOC_FENCE_* flags, since
		//          they are combined together in AllocFenceAndGpuWrite.
		enum
		{
			// Hint that instead of the default fence being written after all
			// shader reads and writes have completed and reached memory, the
			// fence may be written earlier after all the shader inputs have
			// been read.
			GPU_WRITE_FENCE_AFTER_SHADER_READS                  = 0x00010000u,

			// Hint that the fence can be written after all inputs are read and
			// all buffer writes have completed and reached memory.  This can be
			// earlier than all color and depth buffer writes reaching memory.
			GPU_WRITE_FENCE_AFTER_SHADER_BUFFER_WRITES          = 0x00020000u,

			// Internal use only.
			PRIVATE__GPU_WRITE_FENCE_ALL_FLAGS                  = 0xffff0000u,
		};

		// PURPOSE: Generates a GPU command to mark the fence as done.
		grcEntry void GpuMarkFenceDone(grcFenceHandle fence, u32 flags=0) grcPure;

		// PURPOSE: Conveniece helper to call both AllocFence and GpuWriteFence.
		grcEntry grcFenceHandle AllocFenceAndGpuWrite(u32 flags=0)
		{
			grcFenceHandle fence = AllocFence((flags&PRIVATE__ALLOC_FENCE_ALL_FLAGS)|ALLOC_FENCE_INIT_AS_PENDING);
			GpuMarkFenceDone(fence, flags&PRIVATE__GPU_WRITE_FENCE_ALL_FLAGS);
			return fence;
		}

		// PURPOSE: Flags for IsFencePending.  Values may be ORed together
		//          unless specific flags are specified as mutually exclusive.
		//          IsFencePending flags defaults to 0, ie, none of these flags
		//          set.
		enum
		{
			// Hint that no kicking of generated command buffers to the GPU is
			// required.  Be careful with this, as incorrect usage could cause a
			// deadlock.
			IS_FENCE_PENDING_HINT_NO_KICK                       = 0x00000001u,
		};

		// PURPOSE: Poll from CPU to test if a fence is still not been marked as
		//          done by the GPU.
		grcEntry bool IsFencePending(grcFenceHandle fence, u32 flags=0) grcPure;

		// PURPOSE: Block the CPU until the GPU has marked the fence as done.
		grcEntry void CpuWaitOnFence(grcFenceHandle fence) grcPure;

		// PURPOSE: Free fence handle from the CPU.  Must not still have a
		//          pending write from the GPU.  It is ok to free a handle that
		//          has been marked as pending by ALLOC_FENCE_INIT_AS_PENDING
		//          though.
		grcEntry void CpuFreeFence(grcFenceHandle fence) grcPure;

		// PURPOSE: Conveniece wrapper around CpuWaitOnFence and CpuFreeFence.
		inline grcEntry void CpuWaitOnFenceAndFree(grcFenceHandle fence)
		{
			CpuWaitOnFence(fence);
			CpuFreeFence(fence);
		}

		// PURPOSE: Stall GPU until previous writes have completed.
		// NOTES:   This "may" be implemented as
		//            GpuWaitOnFence(AllocFenceAndGpuWrite(grcDevice::ALLOC_FENCE_ALLOC_SCOPE_LIFETIME));
		//          or differently if the same effect can be achieved more efficiently.
		//          This function is available even if DEVICE_GPU_WAIT is not.
		grcEntry void GpuWaitOnPreviousWrites() grcPure;

		// PURPOSE: Convenience wrapper for CPU to wait until GPU is idle and
		//          all GPU writes are CPU visible.
		// WARNING: This is obviously extremely bad for performance, and should
		//          be used for debugging only.
		inline grcEntry void CpuWaitOnGpuIdle()
		{
#if GRCCONTEXT_ALLOC_SCOPES_SUPPORTED
			GRC_ALLOC_SCOPE_AUTO_PUSH_POP()
			CpuWaitOnFence(AllocFenceAndGpuWrite(grcDevice::ALLOC_FENCE_ALLOC_SCOPE_LIFETIME));
#else
			CpuWaitOnFenceAndFree(AllocFenceAndGpuWrite());
#endif
		}

#if DEVICE_GPU_WAIT
		// PURPOSE: Flags to build a mask of which pipeline stages in the GPU
		//          are to be stalled by GpuWaitOnFence.
		enum
		{
			GPU_WAIT_ON_FENCE_INPUT_INDEX_BUFFER    = 0x00000001u,
			GPU_WAIT_ON_FENCE_INPUT_SHADER_FETCHES  = 0x00000002u,

			GPU_WAIT_ON_FENCE_DEFAULT
				= GPU_WAIT_ON_FENCE_INPUT_INDEX_BUFFER
				| GPU_WAIT_ON_FENCE_INPUT_SHADER_FETCHES
		};

		// PURPOSE: Generate a GPU command to block until a fence is marked as
		//          done.
		grcEntry void GpuWaitOnFence(grcFenceHandle fence, u32 inputs=GPU_WAIT_ON_FENCE_DEFAULT) grcPure;

		// PURPOSE: Free up a fence from the GPU.  Must not have any
		//          GpuWriteFence s still pending.
		grcEntry void GpuFreeFence(grcFenceHandle fence) grcPure;

		// PURPOSE: Convenience wrapper around GpuWaitOnFence and GpuFreeFence.
		inline grcEntry void GpuWaitOnFenceAndFree(grcFenceHandle fence, u32 inputs=GPU_WAIT_ON_FENCE_DEFAULT)
		{
			GpuWaitOnFence(fence, inputs);
			GpuFreeFence(fence);
		}
#endif

#if RSG_DURANGO || RSG_ORBIS
		// PURPOSE: Performs a memcpy using GPU shaders.
		// NOTES:   No synchronization is automatically added.  Manual use of fences is required in most cases.
		//          Size of copy must be a multiple of 64-bytes.
		grcEntry void ShaderMemcpy(void *dst, const void *src, size_t size, u32 maxWavefronts=~0u) grcPure;

		// PURPOSE: Performs a memset like operation using GPU shaders.
		// NOTES:   Unlike memset, the value writen is 32 rather than 8-bits.
		//          No synchronization is automatically added.  Manual use of fences is required in most cases.
		//          Size of set must be a multiple of 64-bytes.
		grcEntry void ShaderMemset32(void *dst, u32 value, size_t size, u32 maxWavefronts=~0u) grcPure;
#endif

#if __XENON
		grcEntry u32 GetD3DDepthFormat() grcPure;
		grcEntry void SetD3DDepthFormat(u32 eFormat) grcPure;
#endif // __XENON

		// PURPOSE:	sets a flag to dynamically change between a integer or a fp Z buffer
		// PARAMS:	true for fp or false for integer
		// RETURNS:	None
		// NOTES:	useful for shadow map rendering: if the whole renderer uses a fp Z buffer you might want to switch it
		//          off to use a integer Z buffer for shadow map rendering because this filters very well
		grcEntry int SetfpZBuffer(bool b) grcPure;

		// PURPOSE:	finds out if a given depth format is floating point depth
		// PARAMS:	NONE
		// RETURNS:	true for fp or false for integer
		// NOTES:	useful for shadow map rendering: if the whole renderer uses a fp Z buffer you might want to switch it
		//          off to use a integer Z buffer for shadow map rendering because this filters very well
		grcEntry bool IsItfpZBuffer(int format) grcPure;

		// PURPOSE:	finds out if the current depth buffer format is floating point depth
		// PARAMS:	NONE
		// RETURNS:	true for fp or false for integer
		static bool IsCurrentDepthFormatFpZ();

		// PURPOSE:	Invert a depth value if depth inversion is enabled. 
		// PARAMS:	Depth value to be "fixed"
		// RETURNS:	Fixed depth value (inverted if inversion is being used)
		// NOTES:	See FixViewportDepth() if you need to invert viewport min/max z conditionally based on the 
		//			current inversion method (viewport inversion vs projection matrix inversion)
		static inline float FixDepth(float z) 
		{
#if SUPPORT_INVERTED_VIEWPORT
				return 1.0f - z;
#else
				return z;
#endif
		}

		// PURPOSE:	Invert a depth value used for viewport min/max z params
		// PARAMS:	Depth value to be "fixed"
		// RETURNS:	Fixed depth value (inverted if *viewport* inversion is being used)
		// NOTES:	There are 2 ways to invert depth, we can either invert it in the viewport or invert it in the projection
		//			matrix. This function will invert a depth value used to setup a viewport if depth inversion is enabled
		//			and we are inverting it with the viewport.
		static inline float FixViewportDepth(float z) 
		{
			#if SUPPORT_INVERTED_PROJECTION
				const bool bFixup = !g_grcDepthInversionInvertProjection;
			#else
				const bool bFixup = false;
			#endif

			return bFixup ? FixDepth(z) : z;
		}

		grcEntry void* BeginVertexShaderConstantF(SKINNING_CBUFFER_TYPE address,u32 sizeBytes);
#if __D3D11 || RSG_ORBIS || (__XENON && __BANK)
		grcEntry void EndVertexShaderConstantF(SKINNING_CBUFFER_TYPE address);
#else
		grcEntry void EndVertexShaderConstantF(SKINNING_CBUFFER_TYPE) { }
#endif

		grcEntry void SetProgramResources() grcPure;

#if RSG_PC || RSG_DURANGO
		grcEntry void CreateShaderConstantBuffer(int ByteSize, grcBuffer **pData NOTFINAL_ONLY(, const char* pName)) grcPure;

		grcEntry void CreateVertexShader(u8 *programData, u32 programSize, grcVertexShader **Program) grcPure;

		grcEntry void CreatePixelShader(u8 *programData, u32 programSize, grcPixelShader **Program) grcPure;

		grcEntry void CreateComputeShader(u8 *programData, u32 programSize, grcComputeShader **Program) grcPure;

		grcEntry void CreateDomainShader(u8 *programData, u32 programSize, grcDomainShader **Program) grcPure;

		grcEntry void CreateGeometryShaderWithStreamOutput(u8 *programData, u32 programSize, grcStreamOutputDeclaration *pSODeclaration, u32 NumEntries, u32 OutputStreamStride, grcGeometryShader **ppGeometryShader) grcPure;

		grcEntry void CreateGeometryShader(u8 *programData, u32 programSize, grcGeometryShader **Program) grcPure;

		grcEntry void CreateHullShader(u8 *programData, u32 programSize, grcHullShader **Program) grcPure;

		grcEntry void SetVertexShader(grcVertexShader *vs, const grcProgram *pProgram) grcPure;

		grcEntry void SetComputeShader(grcComputeShader *gs, const grcProgram *pProgram) grcPure;

		grcEntry void SetDomainShader(grcDomainShader *gs, const grcProgram *pProgram) grcPure;
		
		grcEntry void SetGeometryShader(grcGeometryShader *gs, const grcProgram *pProgram) grcPure;
		
		grcEntry void SetHullShader(grcHullShader *gs, const grcProgram *pProgram) grcPure;
		
		grcEntry void SetPixelShader(grcPixelShader *ps, const grcProgram *pProgram) grcPure;

		grcEntry void SetVertexShaderConstantF(int address, const float *data,int count, u32 offset = 0,void *pvDataBuf = NULL) grcPure;
		grcEntry void SetVertexShaderConstantFW(int address, const float *data,int wordcount, u32 offset = 0,void *pvDataBuf = NULL) grcPure;

		grcEntry void SetVertexShaderConstantB(int address,bool value,u32 offset = 0,void *pvDataBuf = NULL) grcPure;

		grcEntry void SetVertexShaderConstantI(int address,int value,u32 offset = 0,void *pvDataBuf = NULL) grcPure;

#if RSG_PC
		grcEntry void SetVertexShaderConstantBufferOverrides(u32 startSlot, grcCBuffer **ppBuffers, u32 numSlots) grcPure;
		grcEntry void ResetConstantBufferOverrides()  grcPure;
#endif

		grcEntry void SetPixelShaderConstantF(int address, const float *data, int count, u32 offset = 0, void *pvDataBuf = NULL) grcPure;
		grcEntry void SetPixelShaderConstantFW(int address, const float *data, int wordcount, u32 offset = 0, void *pvDataBuf = NULL) grcPure;

		grcEntry void SetPixelShaderConstantB(int address,bool value,u32 offset = 0,void *pvDataBuf = NULL) grcPure;
		
		grcEntry void SetComputeShaderConstantF(int address, float *data,int count, u32 offset = 0, void *pvDataBuf = NULL) grcPure;

		grcEntry void SetComputeShaderConstantB(int address,bool value,u32 offset = 0,void *pvDataBuf = NULL) grcPure;

		grcEntry void SetDomainShaderConstantF(int address, float *data,int count, u32 offset = 0, void *pvDataBuf = NULL) grcPure;

		grcEntry void SetDomainShaderConstantB(int address,bool value,u32 offset = 0,void *pvDataBuf = NULL) grcPure;

		grcEntry void SetGeometryShaderConstantF(int address, float *data,int count, u32 offset = 0, void *pvDataBuf = NULL) grcPure;

		grcEntry void SetGeometryShaderConstantB(int address,bool value,u32 offset = 0,void *pvDataBuf = NULL) grcPure;

		grcEntry void SetHullShaderConstantF(int address, float *data,int count, u32 offset = 0, void *pvDataBuf = NULL) grcPure;

		grcEntry void SetHullShaderConstantB(int address,bool value,u32 offset = 0,void *pvDataBuf = NULL) grcPure;

#if __D3D11
		grcEntry void SetInputLayout() grcPure;
		grcEntry void SetInputLayout(grcVertexProgram *vp,grcVertexDeclaration * _decl) grcPure;
		grcEntry bool CheckDeviceStatus();
		grcEntry bool ResizeBuffers(grcDisplayWindow oNewSettings);
		grcEntry void SetDepthBuffer( const grcDeviceView* pDepthBufferTargetView );
		grcEntry const grcDeviceView* GetPreviousDepthBuffer() { return (grcDeviceView*) sm_PreviousDepthView.pRTView; }

		grcEntry void SetUpPriorToDraw(grcDrawMode dm);
		grcEntry void UpdateCurrentState() {}
		grcEntry void LockContext();
		grcEntry void UnlockContext();
#if NV_SUPPORT
		grcEntry void LockContextNVStereo();
		grcEntry void UnlockContextNVStereo();
#endif
		grcEntry void RecordTextureToBind(ShaderType unit, grcTexture *pTexture);
#if RSG_PC
		grcEntry void LockDeviceResetAvailability();
		grcEntry void UnlockDeviceResetAvailability();
		grcEntry void LockContextIfInitialized();
		grcEntry void UnlockContextIfInitialized();

		grcEntry void RegisterSessionNotification();
		grcEntry void UnregisterSessionNotification();
#endif
#endif	//__D3D11
#if RSG_DURANGO
		static sysIpcSema sm_PlmResume;
		static void HandlePlmChange(sysServiceEvent *evt);
		grcEntry void HandleSuspendResume();
		grcEntry void SetNuiGPUReservation( bool bEnable);
		grcEntry bool GetNuiGPUReservation();
#endif // RSG_DURANGO
#elif RSG_ORBIS
		grcEntry void InitSubThread();
		// draw calls
		grcEntry void SetUpPriorToDraw(grcDrawMode dm);
		grcEntry bool NotifyDrawCall(int numInstances);
		grcEntry void ResetDrawCallCount();
		// auxilary passes
		grcEntry void LockSingleTarget(const sce::Gnm::RenderTarget *const target);	// use with caution
		grcEntry void DecompressDepthSurface(const sce::Gnm::DepthRenderTarget *const depthTarget, bool bKeepCompressionDisabled);
		grcEntry void ReenableDepthSurfaceCompression(); // TBR: temporary fix, ideally we'd control this per render target
		grcEntry void UntileFmaskSurface(sce::Gnm::Texture *const destTexture, const sce::Gnm::RenderTarget *const source, uint32_t sampleOffset);
		grcEntry void EliminateFastClear();
		grcEntry void FinishRendering(const sce::Gnm::RenderTarget *const target, bool cmaskDirty);
		grcEntry bool ClearCmask(u32 slot, const sce::Gnm::RenderTarget *const col, Color32 clearColor);
		// misc commands
		enum Cache{
			CACHE_COLOR_DATA	= 0x1,
			CACHE_COLOR_META	= 0x2,
			CACHE_COLOR			= CACHE_COLOR_DATA | CACHE_COLOR_META,
			CACHE_DEPTH_DATA	= 0x4,
			CACHE_DEPTH_META	= 0x8,
			CACHE_DEPTH			= CACHE_DEPTH_DATA | CACHE_DEPTH_META,
			CACHE_ALL			= CACHE_COLOR | CACHE_DEPTH,
		};
		grcEntry void FlushCaches(int cacheMask);
#endif // RSG_ORBIS
#if DEVICE_EQAA
		grcEntry void ResolveMsaaBuffer(grcRenderTargetMSAA *const destination, grcRenderTargetMSAA *const source, int destSliceIndex = 0);
		grcEntry void SetAACount(u32 nsamp, u32 nfrag, u32 niter);
		grcEntry void ResetAACount()	{SetAACount(0,0,0);}
		grcEntry void SetAALocations(u32 ns, u32 nf);
		grcEntry void SetAASuperSample(bool superSample);
#endif // DEVICE_EQAA
#if ENABLE_GPU_DEBUGGER
		grcEntry bool IsGpuDebugged();
#endif //ENABLE_GPU_DEBUGGER

#if RAGE_INSTANCED_TECH
		grcEntry void SetMultiWindow(const grcWindow* InstWindows[], u32 uNumVPInst);
#endif

#if DEVICE_CLIP_PLANES
		private:
		typedef struct grcClipPlanes
		{
			// Plane equation in clipping space (force it to zero to disable a plane)
			Vec4V ClipPlanes[RAGE_MAX_CLIPPLANES];
		} grcClipPlanes;

		grcEntry void SetClipPlanesConstBuffer(grcCBuffer *pConstBuffer);
		public:
		grcEntry void ResetClipPlanes();
		grcEntry void ResolveClipPlanes();
#endif	//DEVICE_CLIP_PLANES

#if __XENON
		// PURPOSE: Call this to get the ColorExpBias
		grcEntry int GetColorExpBias();

		// PURPOSE: Call this to set the ColorExpBias
		grcEntry void SetColorExpBias(int val);

#if HACK_GTA4 //Could be useful to other 360 projects - allows the backbuffer exp bias to change at different stages of the frame
		grcEntry void SetColorExpBiasNow(int val);
#endif

#if HACK_GTA4 //Could be useful to other 360 projects - triple buffer the swap chain - look at xdk docs for more info (search QuerySwapStatus)
		grcEntry void ASyncSwap();
		grcEntry void WaitOnFreeFrontBuffer();
		static inline void SwapCallbackHandler( unsigned long LastVBlank, unsigned long LastSwapVBlank, unsigned long PercentScanned, /*INOUT*/ unsigned long& pSwapVBlank );
		static inline void VBlankCallbackHandler( unsigned long Swap );
#endif

		// PURPOSE: Wrapper for Xenon BeginZPass function
		grcEntry void BeginZPass(u32 flags = 0);

		// PURPOSE: Wrapper for Xenon EndZPass function
		grcEntry void EndZPass();

		// PURPOSE: Set ring buffer parameters on xenon.  Must call before InitClass.
		// PARAMS:	primarySize - Primary ring buffer size, in kilobytes.  Must be a power of two.  System default is 32kb.
		//			secondarySize - Secondary ring buffer size, in kilobytes.  Does not have to be power of two.
		//					        System default is 2048kb.
		//			segments - Number of command buffer segments (see XeDK docs for details).  Default value of zero
		//					   is interpreted as 32 segments.
		//          twoFrames - allocate two frames worth of command buffer space, recommended if your doing predicated tiled rendering 
		static void SetRingBufferParameters(int primarySize,int secondarySize,int segments = 0, bool twoFrames=false);

		// PURPOSE: A simplified wrapper for Xenon's predicated tiling system, placing BeginTiledRendering() before and EndTiledRendering() 
		//          after the section of code you want rendered in multiple tiles. The results of the multi tile renders are stored in colorTarget
		//
		// PARAMS:  colorTarget - a pointer to a render target to hold the composite of the tiles, will usually be a screen resolution 
		//						  render target (NULL means use the RAGE Backbuffer, which will be Rendered to the EDRAM, as if tiling was never done)
		//			depthTarget - a pointer to a depth render target to resolve the tiles' depth buffers to (NULL means you don't want the Depth resolved)
		//			clearParams - a pointer to a grcResolveFlags structure with the clear color/z/stencil values to clear the tiles to on each pass.
		//						  a NULL value clears to color = (0,0,0,255), z = 1.0 stencil= 0x0;
		//			aaSamples - number of AA samples to the tiled rendering, should be 0(or1), 2 or 4
		//			flags - options for tiling:
		//					TILE_HOZIZONTAL - set up tiles horizontally (the default)
		//					TILE_VERTICAL - set up tiles vertically
		//					TILE_DONT_MOVE_TO_EDRAM - if NULL is used for the color buffer, this flag will tells the EndTile not to move the composite image
		//											  to the EDRAM as a final step. If the next thing you are going to do is use the backbuffer as 
		//											  a source for fullscreen effects, etc, there is no reason to move it to the EDRAM, only to move it Resolve it again
		//					TILE_ONE_PASS_ZPASS - Pass the D3DTILING_ONE_PASS_ZPASS flag to BeginTile, to do the hier z set up once for all tiles, if your using BeginZPass/EndZPass
		//					TILE_FIRST_TILE_INHERITS_DEPTH_BUFFER - lets the first tile Reuse the depth buffer info from the Zprepass.
		//			extraPixels - the number of extra pixels to render past the bottom of the current tile (needed for some shaders that need to sample 
		//                        adjacent pixels such as motion blur, etc.) default is 0 pixels
		//
		// NOTES:   You cannot change render targets between BeginTiledRedneringand()/EndTileRendering(), so make sure you do all your
		//          render target work before the main scene draw or after tiling is done...
		//		    
		enum EnumColorBars {TILE_HORIZONTAL=0x0,TILE_VERTICAL=0x1,TILE_DONT_MOVE_TO_EDRAM=0x2,TILE_ONE_PASS_ZPASS=0x4,TILE_FIRST_TILE_INHERITS_DEPTH_BUFFER=0x8};
		static void BeginTiledRendering(grcRenderTarget * colorTarget=NULL, grcRenderTarget* depthTarget=NULL, grcResolveFlags * clearParams=NULL, int aaSamples=4, u32 flags=TILE_HORIZONTAL, int extraPixels = 0);
		static void EndTiledRendering();

		// PURPOSE: Get the number of tiles, -1 if there is no predicated tiling going on right now.
		static int GetTileCount();

		static void SetPredication( u32 predicationFlag );

		// PURPOSE: A simplified wrapper for Xenon's predicated tiling system for the depth buffer (== shadow maps), placing BeginTiledRenderingDepthBuffer() 
		//          before and EndTiledRenderingDepthBuffer() after the section of code you want rendered in multiple tiles into the depth buffer. 
		//			The results of the multi tile renders are stored in depthTarget
		//
		// PARAMS:	depthTarget - a pointer to a depth render target to resolve the tiles' depth buffers to (NULL means you don't want the Depth resolved)
		//			clearParams - a pointer to a grcResolveFlags structure with the clear color/z/stencil values to clear the tiles to on each pass.
		//						  a NULL value clears to color = (0,0,0,255), z = 1.0 stencil= 0x0;
		//			flags - options for tiling:
		//					TILE_HOZIZONTAL - set up tiles horizontally (the default)
		//					TILE_VERTICAL - set up tiles vertically
		//					TILE_DONT_MOVE_TO_EDRAM - if NULL is used for the color buffer, this flag will tells the EndTile not to move the composite image
		//											  to the EDRAM as a final step. If the next thing you are going to do is use the backbuffer as 
		//											  a source for fullscreen effects, etc, there is no reason to move it to the EDRAM, only to move it Resolve it again
		//			extraPixels - the number of extra pixels to render past the bottom of the current tile (needed for some shaders that need to sample 
		//                        adjacent pixels such as motion blur, etc.) default is 0 pixels
		//			Width - size of target depth buffer
		//			Height - size of target depth buffer
		//
		// NOTES:   You cannot change render targets between BeginTiledRedneringand()/EndTileRendering(), so make sure you do all your
		//          render target work before the main scene draw or after tiling is done...
		//		    
		static void BeginTiledRenderingDepthBuffer(grcRenderTarget* depthTarget, grcResolveFlags * clearParams=NULL, u32 flags=TILE_HORIZONTAL, int extraPixels = 0, int Width=2048, int Height=2048);
		static void EndTiledRenderingDepthBuffer(grcRenderTarget* depthTarget);


		// PURPOSE: Return a pointer to the array of grcTileRects used for the current BeginTiledRendering()/EndTiledRendering() rendering (or NULL if tiling is not active)
		//
		//	PARAMS:
		//			count - the number of a grcTileRects used. returns 0 if tiling is not active.
		static const grcTileRect * GetTileRects(int & count);



		// PURPOSE: Resolve a copy of the back buffer into a texture/rendertarget. This routine is Predicated tiling aware,
		//          so it will save a tile during each pass. useful for grabbing a copy of the backbuffer "in the middle" of rendering.
		//
		//	PARAMS:
		//			colorTarget - a pointer to a render target to hold the copy of the back buffer (NULL means don't resolve color buffer)
		//			depthTarget - a pointer to a depth render target to hold the copy of the active depth buffer (NULL means don't resolve depth)
		//			clearParams - a pointer to a grcResolveFlags structure with the clear color/z/stencil values to clear the back buffer to after copying,
		//                        default is NULL, which means don't clear 

		static void SaveBackBuffer(grcRenderTarget * colorTarget, grcRenderTarget* depthTarget=NULL, grcResolveFlags * clearParams=NULL);


		// PURPOSE: Adjust the balance of pixel and vertex threads on the Xenon GPU. 
		//
		// PARAMS:  vertexThreads - the number of vertex threads, Minimum value is 16, unless 0 is used to specif defaults
		// PARAMS:  pixelThreads - the number of pixel threads,   Minimum value is 16, unless 0 is used to specif defaults
		// NOTE:    (vertexThreads+pixelThreads) must equal exactly 128 (or 0 if selecting the system default values) 
		//          there is significant latency when changing these values so do so sparingly.
		static void SetShaderGPRAllocation(int vertexThreads, int pixelThreads);

		// PURPOSE: Performs a blit which setting up any default shaders so can be used with effects
		//
		static void BlitRectfNoSetup(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color );

#endif

#if DEVICE_EQAA
		struct MSAAMode
		{
			bool					m_bEnabled;
			u8						m_uSamples;
			u8						m_uFragments;

			inline MSAAMode(u32 numSamples, u32 numFragments)
			: m_bEnabled( numSamples && numFragments )
			, m_uSamples( numSamples )
			, m_uFragments( numFragments )
			{
				Assert( !numSamples == !numFragments && numSamples >= numFragments );
			}
			
			inline explicit MSAAMode(u32 numSamples)
			: m_bEnabled( numSamples!=0 )
			, m_uSamples( numSamples )
			, m_uFragments( numSamples )
			{}

#	if RSG_ORBIS
			explicit MSAAMode(const sce::Gnm::RenderTarget &target);
			explicit MSAAMode(const sce::Gnm::Texture &texture);
			sce::Gnm::NumSamples	GetSamplesEnum() const;
			sce::Gnm::NumFragments	GetFragmentsEnum() const;
#	elif __D3D11
			explicit MSAAMode(const DXGI_SAMPLE_DESC &desc);
			u32 DeriveQuality() const;
			u32 GetFmaskFormat() const;
#	endif

			void operator=(const int ASSERT_ONLY(num))
			{
				Assertf(!num, "EQAA mode requires more than just an int");
				*this = MSAA_None;
			}

			inline operator unsigned int() const
			{
				return m_bEnabled ? m_uFragments : 0;
			}

			inline bool NeedFmask() const
			{
				return m_bEnabled && m_uSamples > m_uFragments;
			}

			inline void DisableCoverage()
			{
				m_uSamples = m_uFragments;
			}

			u32 GetFmaskShift() const;
		};
		static const MSAAMode MSAA_None;
#else	//DEVICE_EQAA
		enum MSAAModeEnum
		{
			MSAA_None			= 0,
			MSAA_NonMaskAble	= 1,
			MSAA_2				= 2,
			MSAA_4				= 4,
			MSAA_8				= 8,
			// PS3
			MSAA_2xMS			= 2,	// using 2x multi-sampling with a Quincunx filter while downsampling
			MSAA_Centered4xMS	= 4,	// using 4x multi-sampling with a centered filter kernel and a Gauss filter while downsampling
			MSAA_Rotated4xMS	= 8		// using 4x multi-sampling with a rotated filter kernel and a Gauss filter while downsampling
		};

		class MSAAMode
		{
		public:
			inline MSAAMode()
				: m_Mode(MSAA_None)
			{
			}

			inline MSAAMode(MSAAModeEnum mode)
				: m_Mode(mode)
			{
			}

			inline MSAAMode(int mode)
				: m_Mode(static_cast<MSAAModeEnum>(mode))
			{
			}

			inline MSAAMode operator=(int value)
			{
				m_Mode = static_cast<MSAAModeEnum>(value);
				return *this;
			}

			inline bool operator==(MSAAMode mode) const
			{
				return m_Mode == mode.m_Mode;
			}

			inline bool operator==(MSAAModeEnum mode) const
			{
				return m_Mode == mode;
			}

			inline operator unsigned int() const
			{
				return static_cast<unsigned int>(m_Mode);
			}

		private:
			MSAAModeEnum m_Mode;
		};
#endif	//DEVICE_EQAA

#if __D3D11 || RSG_ORBIS
		grcEntry void SetSamplesAndFragments(u32 Samples
#if DEVICE_EQAA
			, u32 Fragments
#endif
			);
#endif

		// RETURNS:	Current MSAA level
		grcEntry inline MSAAMode GetMSAA();
		grcEntry inline void SetMSAA(MSAAMode i);

#if DEVICE_EQAA
		// RETURNS: is fragment compression enabled
		grcEntry inline bool IsEQAA();
#else
		grcEntry inline u32 GetMSAAQuality();
		grcEntry inline void SetMSAAQuality(u32 quality);
#endif // DEVICE_EQAA

#if DEPTH_BOUNDS_SUPPORT
		// PURPOSE: This function enables/disables the depth boundary test feature of RSX.
		// The depth boundary test is applied to a depth value already written in the 
		// depth buffer; the depth value of the fragment to which you are attempting 
		// to write is unrelated. In other words, the fragment to which you are attempting 
		// to write will be destroyed - regardless of its depth value - if the value of 
		// the depth buffer is outside the range specified in cellGcmSetDepthBounds().
		// RETURNS:	NONE
		grcEntry void SetDepthBoundsTestEnable(u32 enable);

		// PURPOSE: This function specifies the minimum value and the maximum value 
		// for depth boundaries. The range specified by zmin and zmax will be used 
		// as the range within which values will pass the depth boundary test. 
		//
		// You need to switch on the depth bounds test first
		//
		// RETURNS:	NONE
		grcEntry void SetDepthBounds(float zmin,
									 float zmax);
#endif // DEPTH_BOUNDS_SUPPORT

#if __PPU
		// PURPOSE:	implement MSAA for render targets on the PS3
		// RETURNS:	NONE
		// NOTES:	the PS3 requires that we implement MSAA by hand ... this is the function that should blit from 
		//			a source rendertarget to the target rendertarget.
		//			this can be used in device_gcm.cpp or in the post-processing pipeline
		grcEntry void MsaaResolve(grcRenderTarget* target,	// target render target
			grcRenderTarget* source,						// source render target
			int textureAtlasFace = -1,						// texture face
			bool compositeTransparent = false);				// is this a transparent composite?

		grcEntry void MsaaResolveBackToFrontBuffer(bool force = false, bool compositeTransparent = false);

		grcEntry void MsaaResolveTouch();

		grcEntry bool GetMultiSampleFlag();
		
		grcEntry bool HasADepthTarget();

		grcEntry void SetRenderTarget(const grcRenderTarget *color, const grcRenderTarget *depthStencil);
		grcEntry void SetRenderTargets(const grcRenderTarget *color[grcmrtColorCount], const grcRenderTarget *depthStencil);

		// PURPOSE:	patches a 24D_S8 depth buffer to a 8:8:8:8 buffer to make it readable
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry u8 PatchShadowToDepthBuffer(grcRenderTarget *_Buffer, bool patchSurfaceFormat);

		// PURPOSE:	patches a 8:8:8:8 depth buffer to a 24D_S8 buffer to make it readable
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry void PatchDepthToShadowBuffer(grcRenderTarget *_Buffer, u8 format, bool patchSurfaceFormat);

		// PURPOSE:	adjusts the move forward and back ward values based on results from the previous frame gained in 
		//          EndAdaptiveZCulling()
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry void BeginAdaptiveZCulling(u32 MoveForward, u32& Dir, u32& Step, float& Prev, float RatioTilesCulled);

		// PURPOSE:	retrieves the values to adjust the move forward and back ward values for Z culling in BeginAdaptiveCulling
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry float EndAdaptiveZCulling();

		// PURPOSE:	adjusts the move forward and back ward values based on results from the previous frame gained in 
		//          EndSlopeBasedZCulling()
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry void BeginSlopeBasedZCulling(u32 AvgSlope, u32 MaxSlope);

		// PURPOSE:	retrieves the values to adjust the move forward and back ward values for Z culling in BeginSlopeBasedZCulling
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry void EndSlopeBasedZCulling(u32& AvgSlope, u32& MaxSlope);

		// PURPOSE:	freezes Z culling data: Zcull will keep the existing occluders and not attempt to 
		// create new ones (some holes in the occluder mask may be filled, but the Z thresholds will not move).
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry void FreezeZCulling();

		// PURPOSE:	Invalidates z culling data
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry void InvalidateZCulling();

		// PURPOSE:	Out of the processing performed on a primitive calculated by 
		// a vertex shader, when it passes the rasterizer, this function exerts 
		// controls relating to its Z values. In RSX, it is possible to enable/disable
		// culling of a primitive that doesn't have any Z values, of any of its 
		// vertices, within the space defined by the near clipping plane and the far 
		// clipping plane, in the Z direction set by min and max of cellGcmSetViewport() 
		// (in other words, a primitive with all its vertices outside of viewport). 
		// Culling is enabled when cullNearFarEnable is set to CELL_GCM_TRUE and disabled 
		// when CELL_GCM_FALSE is specified.
		// It is also possible to enable (or disable) clamping of a primitive's Z values 
		// if they are outside the viewport so that it falls within the range of the 
		// near clipping plane and the far clipping plane. Clamping will be enabled when 
		// zclampEnable is set to CELL_GCM_TRUE and disabled when CELL_GCM_FALSE is 
		// specified.
		// Normally, when all W values of a primitive are negative, that primitive 
		// will be culled by the rasterizer. The skipping of this step can be 
		// enabled/disabled. This step will be skipped when cullIgnoreW is set to 
		// CELL_GCM_TRUE (in other words, the culling step will be skipped and will 
		// not be performed even when all W values are negative). Skipping the culling 
		// step will be disabled when CELL_GCM_FALSE is specified.
		// 
		// all three parameters are actually boolean values
		//
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry void SetZMinMaxControl(const u32 cullNearFarEnable,
									const u32 zclampEnable,
									const u32 cullIgnoreW);

		enum StencilCullingFunctionEnum
		{
			SFUNC_NEVER    = (0),
			SFUNC_LESS     = (1),
			SFUNC_EQUAL    = (2),
			SFUNC_LEQUAL   = (3),
			SFUNC_GREATER  = (4),
			SFUNC_NOTEQUAL = (5),
			SFUNC_GEQUAL   = (6),
			SFUNC_ALWAYS   = (7),
		};
		// PURPOSE:	controls the stencil culling functionality
		// RETURNS:	NONE
		// NOTES:	PS3 only
		grcEntry void ControlStencilCulling(u32 StencilFunction,
									  u32 StencilReference,
									  u32 StencilMask);

		// PURPOSE:	Returns the most recently drawn depth buffer frame as a 32bpp grcImage object.
		// RETURNS:	Pointer to grcImage containing frame grab, or NULL if feature is not supported.
		//			Don't forget to call Release on the image when you're done with it.
		// NOTES:	Call this after endframe.  Use grcImage::SaveDDS or SaveJPEG to write the image out to disc.
		grcEntry class grcImage *CaptureDepthBufferScreenshot(grcRenderTarget *DepthBuffer);

		// PURPOSE:	Returns the most render target as a 32bpp grcImage object.
		// RETURNS:	Pointer to grcImage containing frame grab, or NULL if feature is not supported.
		//			Don't forget to call Release on the image when you're done with it.
		// NOTES:	Call this after endframe.  Use grcImage::SaveDDS or SaveJPEG to write the image out to disc.
		grcEntry class grcImage* CaptureRenderTargetScreenshot(grcRenderTarget *RenderTarget);

		// PURPOSE:	Returns a screenshot of a cube map face
		// RETURNS:	Pointer to grcImage containing frame grab, or NULL if feature is not supported.
		//			Don't forget to call Release on the image when you're done with it.
		// NOTES:	Call this after endframe.  Use grcImage::SaveDDS or SaveJPEG to write the image out to disc.
		grcEntry class grcImage* CaptureCubeFaceScreenshot(grcRenderTarget *CubeRenderTarget, int face);

		// PURPOSE:	Returns a chunk of memory usable as a display list (FIFO segment)
		// RETURNS:	Pointer to memory.
		// PARAMS:	Size of chunk in words. Optional memory alignment.
		grcEntry u32* CreateDisplayList(u32 wordCount, u32 alignment = 128);

		// PURPOSE:	Frees a chunk of memory usable as a display list (FIFO segment)
		// PARAMS:	Pointer to chunk of memory.
		grcEntry void DeleteDisplayList(u32* ptr);
		grcEntry void DeleteDisplayList(u32 offset);

		// Blocks until all gcm tasks on SPU have completed.
		grcEntry void BlockOnGcmTasks();
		
		// return full range status
		static inline bool IsOutputFullRange();
#endif // __PPU

#if RSG_ORBIS
		grcEntry void SetViewportScissor(bool bEnable) grcPure;
		grcEntry void SetGlobalResources() grcPure;
		grcEntry void SetGsVsRingBufferData(const uint32_t *exportVertexSizeInDWord, const uint32_t maxOutputVertexCount) grcPure;
		grcEntry void SetEsGsRingBufferData(const uint32_t maxExportVertexSizeInDword) grcPure;

		grcEntry bool IsViewportScissor()	{ return s_bScanMode_ViewportScissor; }
		grcEntry void SetVertexShaderConstantF(int address, const float *data,int count, u32 offset = 0,void *pvDataBuf = NULL) grcPure;

		grcEntry void SetWindow(const grcWindow& window, int uVPIndex) grcPure;

		// PURPOSE:	Sets clear effect name for Orbis (usually common:/shaders/ClearCS);
#endif // RSG_ORBIS

	// PURPOSE:	Returns the most recently drawn frame as a 32bpp grcImage object.
	// RETURNS:	Pointer to grcImage containing frame grab, or NULL if feature is not supported.
	//			Don't forget to call Release on the image when you're done with it.
   // PARAMS:  pBuffer - optional pointer to an image that you would like to populate with the captured image
   //          if pBuffer is NULL, then a new image will be allocated to receive the image bits.
	// NOTES:	Call this after endframe.  Use grcImage::SaveDDS or SaveJPEG to write the image out to disc.
	grcEntry class grcImage *CaptureScreenShot(class grcImage* pImage=NULL) grcPure;
#if !__PPU	
	grcEntry       void		 BeginTakeScreenShot() grcPure;
	grcEntry class grcImage *EndTakeScreenShot() grcPure;
#endif

	// PURPOSE:	Takes a screenshot directly to a file, avoiding lots of intermediate memory allocations.
	// PARAMS:	outName - Output file name; .dds extension is added if not already present.
	// RETURNS:	True on success, false if unsupported or unable to create file
	grcEntry bool CaptureScreenShotToFile(const char *outName,float gamma) grcPure;
	grcEntry bool CaptureScreenShotToJpegFile(const char *outName) grcPure;


	// PURPOSE:	Clears the current viewport window (as set by SetWindow)
	// PARAMS:	enableClearColor - True to clear color, else false
	//			clearColor - Color to clear to
	//			enableClearDepth - True to clear depth, else false
	//			clearDepth - Clear value for depth buffer (you probably want grcDepthFarthest)
	//			enableClearStencil - True to clear stencil, else false
	//			clearStencil - Clear value for stencil buffer
	// NOTES:	If enableClearDepth != enableClearStencil, you may experience poor performance.
	grcEntry void Clear(bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth,bool enableClearStencil,u32 clearStencil) grcPure;

	// For backward compatibility
	grcEntry void Clear(bool enableClearColor,Color32 clearColor,bool enableClearDepthStencil,float clearDepth,u32 clearStencil) {
		Clear(enableClearColor,clearColor,enableClearDepthStencil,clearDepth,enableClearDepthStencil,clearStencil);
	}
#if __D3D11 || RSG_ORBIS
	grcEntry void ClearUAV(bool bAsFloat, grcBufferUAV* pBuffer);
#endif
	// PURPOSE:	Sets the current viewport window
	// PARAMS:	window - New window to make current
	// NOTES:	You generally don't need to call this yourself, grcViewport will handle doing this for you.
	grcEntry void SetWindow(const grcWindow& window) grcPure;

	// PURPOSE:	Makes the device forget about any cached state.  Useful when we have to hand control over to
	//			third-party code like an effects framework.
	grcEntry void ClearCachedState() grcPure;

	grcEntry void SetDefaultEffect(bool isLit,bool isSkinned) grcPure;

	grcEntry grcEffect& GetDefaultEffect() grcPure;

	// PURPOSE:	Sets default effect name (usually embedded:/rage_im); must be pointer to constant storage.
	static void SetDefaultEffectName(const char *effectName) { sm_DefaultEffectName = effectName; }

	const char *GetDefaultEffectName() { return sm_DefaultEffectName; }

	// PURPOSE:	Creates a vertex declarator object, which describes the size and formats of each part of a vertex.
	grcEntry grcVertexDeclaration* CreateVertexDeclaration(const grcVertexElement *pVertexElements, int elementCount, int strideOverride = 0) grcPure;

	// PURPOSE:	Destroy a vertex declaration.
	grcEntry void DestroyVertexDeclaration(grcVertexDeclaration*) grcPure;

	// PURPOSE:	Sets the currently active vertex declaration.
	grcEntry Result SetVertexDeclaration(const grcVertexDeclaration *pDecl) grcPure;
#if __PS3
	grcEntry Result RecordSetVertexDeclaration(const grcVertexDeclaration *pDecl) grcPure;
#else
	grcEntry Result RecordSetVertexDeclaration(const grcVertexDeclaration *pDecl) { return SetVertexDeclaration(pDecl); }
#endif

	// PURPOSE:	Returns the currently active vertex declaration
	grcEntry Result GetVertexDeclaration(grcVertexDeclaration **ppDecl) grcPure;

#if RSG_PC
	static u32 GetMaxQueuedFrames() { return ms_MaxQueuedFrames; }
#endif

	// Expect (killSwitch & s_KillSwitchMask) == 0, unless an anti-tamper has fired
	static const u32 s_KillSwitchMask = 0x10101010;
	grcEntry void SetKillSwitch(u32 killSwitch) { Assert((killSwitch&s_KillSwitchMask)==0); sm_KillSwitch = (killSwitch | (sm_KillSwitch & s_KillSwitchMask)); }
	inline static u32 MakeKillSwitch(u32 validData, u32 set); // validData should be 16 bits, set should be 1 bit
	inline static u32 ExtractKillSwitchData(u32 data);

#if __WIN32PC
	grcEntry u32 GetTextureResourcePool() { return sm_TextureResourcePool; } grcPure
	grcEntry u32 GetBufferResourcePool() { return sm_BufferResourcePool; } grcPure
	grcEntry u32 GetDepthFormat();
	grcEntry void SetDepthFormat(u32 eFormat);

	grcEntry bool GetIgnoreStereoMsg()		{ return sm_IgnoreStereoMsg; }
	grcEntry void IgnoreStereoMsg(bool bIgnore = true);
	grcEntry bool UsingMultipleGPUs();
	grcEntry void IncrementGPUIndex();
	grcEntry u32 GPUIndex();
	grcEntry u32 GPUIndexMT();
	grcEntry void CopyGPUIndex();
	grcEntry u32  GetGPUCount(bool bActiveCount = false, bool bResetActiveCount = false);
	grcEntry bool SupportsFeature(DeviceFeatureSet eFeature) { return (sm_uFeatures & eFeature) ? true : false; }
	grcEntry bool InitializeFeatureSet();

	grcEntry bool IsPaused() { return sm_Paused; }
	grcEntry void SetPaused(bool bPause) { sm_Paused = bPause; }

	grcEntry bool IsMinimized() { return sm_Minimized; }
	grcEntry void SetMinimized(bool bMinimize) { sm_Minimized = bMinimize; }
	grcEntry bool IsMaximized() { return sm_Maximized; }
	grcEntry void SetMaximized(bool bMaximized) { sm_Maximized = bMaximized; }	
	grcEntry bool IsOccluded() { return sm_Occluded; }
	grcEntry void SetOccluded(bool bOccluded) { sm_Occluded = bOccluded; }
	grcEntry bool IsMinimizedWhileFullscreen() { return sm_MinimizedWhileFullscreen; }
	grcEntry void SetMinimizedWhileFullscreen(bool bSet) { sm_MinimizedWhileFullscreen = bSet; }
	grcEntry bool IsTopMostWhileWindowed() { return sm_TopMostWhileWindowed; }
	grcEntry void SetTopMostWhileWindowed(bool bTopMost) { sm_TopMostWhileWindowed = bTopMost; }
	grcEntry bool CanIgnoreSizeChange() { return sm_IgnoreSizeChange; }
	grcEntry void SetIgnoreSizeChange(bool bIgnore) { Displayf("SetIgnoreSize %d", bIgnore); sm_IgnoreSizeChange = bIgnore; }
	grcEntry bool IsActive() { return sm_Active; }
	grcEntry void SetActive(bool bActive) { sm_Active = bActive; }
	grcEntry bool IsInSizeMove() { return sm_InSizeMove; }
	grcEntry void SetInSizeMove(bool bInSizeMove) { sm_InSizeMove = bInSizeMove; }
	grcEntry bool IsStereoChangeEnabled() { return sm_bStereoChangeEnable; }
	grcEntry void SetStereoChangeEnable(bool bEnable) { sm_bStereoChangeEnable = bEnable; }
	grcEntry bool IsReleasing() { return sm_ReleasingSwapChain; }
	grcEntry void SetReleasing(bool bRelease) { sm_ReleasingSwapChain = bRelease; }
	grcEntry bool IsLost() { return sm_Lost; }
	grcEntry void SetLost(bool bLost) { sm_Lost = bLost; }
	grcEntry void SetInsideDeviceChange(bool bInside) { sm_InsideDeviceChange = bInside; }
	grcEntry bool IsInsideDeviceChange() { return sm_InsideDeviceChange; }
	grcEntry void SetDoNoStoreNewSize(bool bDoNotStore) { sm_DoNoStoreNewSize = bDoNotStore; }
	grcEntry bool IsDoNoStoreNewSize() { return sm_DoNoStoreNewSize; }
	grcEntry void SetClipCursorWhenFullscreen(bool bClipCursor) { sm_ClipCursorWhenFullscreen = bClipCursor; }
	grcEntry bool ShouldClipCursorWhenFullscreen() { return sm_ClipCursorWhenFullscreen; }
	grcEntry void SetDeviceRestored(bool bRestored) { sm_DeviceRestored = bRestored; }
	grcEntry bool IsDeviceRestored() {return sm_DeviceRestored;}
	grcEntry void SetForceWindowResize(bool bForceResize) { sm_ForceWindowResize = bForceResize; }
	grcEntry bool IsForceWindowResize() {return sm_ForceWindowResize;}

	grcEntry void SetSwapChainFullscreen(bool bFullscreen) {sm_SwapChainFullscreen = bFullscreen;}
	grcEntry bool IsSwapChainFullscreen() {return sm_SwapChainFullscreen;}

	grcEntry void SetMatchDesiredWindow(bool bMatch) { sm_MatchDesiredWindow = bMatch; }
	grcEntry bool IsMatchDesiredWindow() {return sm_MatchDesiredWindow;}
	grcEntry void SetRecheckDeviceChanges(bool bRecheck) { sm_RecheckDeviceChanges = bRecheck; }
	grcEntry bool IsRecheckDeviceChanges() {return sm_RecheckDeviceChanges;}
#if __D3D11
	grcEntry void InitializeWindow(grcDisplayWindow &window) {sm_DesiredWindow = window;}

	grcEntry void SetChangeDeviceRequest(bool bMatch) { sm_ChangeDeviceRequest = bMatch; }
	grcEntry bool IsChangeDeviceRequest() {return sm_ChangeDeviceRequest;}
	grcEntry void SetInPopup(bool bMatch) { sm_IsInPopup = bMatch; }
	grcEntry bool IsInPopup() {return sm_IsInPopup;}
	grcEntry void SetForceChangeDevice(bool bMatch) { sm_ForceChangeDevice = bMatch; }
	grcEntry bool IsForceChangeDevice() {return sm_ForceChangeDevice;}
	grcEntry void SetForceDeviceReset(bool bMatch) { sm_ForceDeviceReset = bMatch; }
	grcEntry bool IsForceDeviceReset() {return sm_ForceDeviceReset;}
	grcEntry void SetRequireDeviceRestoreCallbacks(bool bRestore) { sm_RequireDeviceRestoreCallbacks = bRestore; }
	grcEntry bool IsRequireDeviceRestoreCallbacks() {return sm_RequireDeviceRestoreCallbacks;}
	grcEntry void SetIngoreWindowLimits(bool bIgnoreWindowLimits) {sm_IgnoreMonitorWindowLimits = bIgnoreWindowLimits;}
	grcEntry bool GetIgnoreWindowLimits() {return sm_IgnoreMonitorWindowLimits;}
#endif

#if USE_NV_TXAA 
	grcEntry void SetTXAASupported(bool bSupported) { sm_TXAASupported = bSupported; }
	grcEntry bool GetTXAASupported() { return sm_TXAASupported; }
#endif

#if RSG_PC && __D3D11
	grcEntry void SetWindowDragResized(bool bResized) { sm_DragResized = bResized; }
	grcEntry bool IsWindowDragResized() {return sm_DragResized;}
	grcEntry void SetRecenterWindow(bool bRecenter) { sm_RecenterWindow = bRecenter; }
	grcEntry bool IsRecenterWindow() {return sm_RecenterWindow;}
	grcEntry void SetBorderless(bool bBorderless) { sm_Borderless = bBorderless; }
	grcEntry bool IsBorderless() {return sm_Borderless;}
	grcEntry void SetDesireBorderless(bool bBorderless) { sm_DesireBorderless = bBorderless; }
	grcEntry bool IsDesireBorderless() {return sm_DesireBorderless;}
#if NV_SUPPORT
	grcEntry void BeginRscRendering(grcRenderTarget *pRT, u32 flags = 0);
	grcEntry void EndRscRendering(grcRenderTarget *pRT);
#endif
#endif

	grcEntry void SetFullscreenWindow(grcDisplayWindow oNewSettings) {
		sm_FullscreenWindow = oNewSettings;
		sm_FullscreenWindow.bFullscreen = true;
		sm_AspectRatio = (float)sm_FullscreenWindow.uWidth / (float)sm_FullscreenWindow.uHeight;
	}

	grcEntry bool ChangeDevice(grcDisplayWindow oNewSettings);
	grcEntry bool CreateDevice();
	grcEntry bool CleanupDevice();
	grcEntry bool Reset();

#if __D3D11
	grcEntry void SetBusyAltTabbing(bool bAltTabbing) { sm_BusyAltTabbing = bAltTabbing; }
	grcEntry bool IsBusyAltTabbing() {return sm_BusyAltTabbing;}
	grcEntry void SetIgnoreDeviceReset(bool bIgnoreReset) { sm_IgnoreDeviceReset = bIgnoreReset; }
	grcEntry bool IgnoreDeviceReset() {return sm_IgnoreDeviceReset;}
	grcEntry void ForceDeviceReset();
	grcEntry bool IsReadOnlyDepthAllowed();
#endif

	grcEntry void SetWindowFlags (unsigned long flags) { sm_WindowFlags = flags; }
	grcEntry unsigned long GetWindowFlags () { return sm_WindowFlags; }

	grcEntry void SetRequestPauseMenu (bool bRequestPauseMenu) {sm_RequestPauseMenu = bRequestPauseMenu;}
	grcEntry bool GetRequestPauseMenu () {return sm_RequestPauseMenu;}

	grcEntry bool IsAllowPauseOnFocusLoss() {return sm_AllowPauseOnFocusLoss && !sm_VideoEncodingOverride && !sm_DisablePauseOnFocusLossSystemOverride;}
	grcEntry void SetAllowPauseOnFocusLoss(bool bAllowPause) { sm_AllowPauseOnFocusLoss = bAllowPause; }

	grcEntry void SetDisablePauseOnLostFocusSystemOverride(bool bPauseOverride) {sm_DisablePauseOnFocusLossSystemOverride = bPauseOverride;}

	grcEntry void SetVideoEncodingOverride(bool bOverride) { sm_VideoEncodingOverride = bOverride; }
	grcEntry bool GetVideoEncodingOverride() {return sm_VideoEncodingOverride;}

	grcEntry void SetHeadBlendingOverride(bool bOverride) { sm_HeadBlendingOverride = bOverride; }
	grcEntry bool GetHeadBlendingOverride() {return sm_HeadBlendingOverride;}

	grcEntry bool ContinueRenderingOverride() {return GetVideoEncodingOverride() || GetHeadBlendingOverride();}

	grcEntry void SetDxFeatureLevel(u32 uMajor, u32 uMinor = 0)	{ sm_DxFeatureLevel = uMajor * 100; Assert(uMinor < 100); sm_DxFeatureLevel += (uMinor < 100) ? uMinor : 0; }
	grcEntry u32  GetDxFeatureLevel() { return sm_DxFeatureLevel; }	

#if RSG_PC && __D3D11
	grcEntry void CheckVideoEncodingOverride();
#endif
#if RSG_PC
	static sysIpcThreadId sm_FocusQueryThreadId;
	grcEntry void KillQueryFocusThread();
	grcEntry void QueryFocusThread(void*);
	grcEntry void InitFocusQueryThread();

	static bool	m_bDisableAltEnterChange;
	grcEntry void SetDisableAltEnterChange(bool m_bDisableAltEnter);
	grcEntry bool IsDisableAltEnterChange();
	grcEntry void SuppressAltEnter();
	grcEntry void AllowAltEnter();
	grcEntry u32 GetDXFeatureLevelSupported();
#endif
#if RSG_PC && __DEV
	grcEntry void StartLoggingResets(fiStream *fStream) {sm_DeviceResetLoggingStream = fStream;}
	grcEntry void StopLoggingResets() {sm_DeviceResetLoggingStream = NULL;}
	grcEntry bool GetDeviceResetTestActive() {return sm_DeviceResetTestActive;}
	grcEntry void SetDeviceResetTestActive(bool testValue) {sm_DeviceResetTestActive = testValue;}
#endif

	grcEntry void SetDxShaderModel(u32 uMajor, u32 uMinor = 0)	{ sm_DxShaderModel[0] = uMajor; sm_DxShaderModel[1] = uMinor; Assert(uMinor < 100); }
	grcEntry void GetDxShaderModel(u32 & uMajor, u32 & uMinor) { uMajor=sm_DxShaderModel[0]; uMinor=sm_DxShaderModel[1]; }	

	grcEntry bool CanLoadShader(u32 shaderMajor, u32 shaderMinor);

	// PURPOSE: Set up the mouse cursor with new display mode changes
#if RSG_PC
	static bool (*CursorVisibilityOverride)();
	static void (*RenderFunctionCallback)();
	static void (*DeviceChangeHandler)();
#endif
	grcEntry void SetupCursor(WIN32PC_ONLY(bool bEnable = true));

#elif RSG_DURANGO || RSG_ORBIS
	grcEntry u32 GetDxFeatureLevel() { return 1100; }	 // TODO: Should probably be 1110
	grcEntry bool CanLoadShader(u32,u32) { return true; }
	grcEntry bool IsReadOnlyDepthAllowed() { return true; }
#endif

#if RSG_PC|| RSG_DURANGO
	grcEntry void SetThreadId(int threadId)		{ sm_ThreadId = threadId; }
	grcEntry int GetThreadId()		{ return sm_ThreadId; }

	grcEntry bool BlockUpdateThread() { return (!GetHasFocus() && IsAllowBlockUpdateThread()) WIN32PC_ONLY(&& IsAllowPauseOnFocusLoss()); }
	grcEntry void SetAllowBlockUpdateThread(bool bAllowBlock) { sm_AllowUpdateThreadBlock = bAllowBlock; }
	grcEntry bool IsAllowBlockUpdateThread() {return sm_AllowUpdateThreadBlock;}

	// PURPOSE: Statelessly reports if the game window or RAG is the foreground window.
	static bool GetHasFocus();
#endif

	static bool IsMessagePumpThreadThatCreatedTheD3DDevice();
#if __D3D11
	// Returns true if the current thread is the same thread the device was created on.
	static bool IsCurrentThreadTheDeviceOwnerThread();
#endif //__D3D11

#if __PPU || __PSP2
	static const u32 BEGIN_VERTICES_MAX_SIZE = 8122;
#elif RSG_DURANGO || RSG_ORBIS
	static const u32 BEGIN_VERTICES_MAX_SIZE = 64000*2;
#else
#	if __BANK
	static const u32 BEGIN_VERTICES_MAX_SIZE = 1024*1024*20;
#	else
	static const u32 BEGIN_VERTICES_MAX_SIZE = 1024*1024*4;
#	endif
	static const u32 BEGIN_INDICES_MAX_SIZE = 65535;
#endif

#if __BANK
#define GPU_VERTICES_MAX_SIZE	grcDevice::BEGIN_VERTICES_MAX_SIZE
#else
#if RSG_PC
#define GPU_VERTICES_MAX_SIZE	(grcDevice::BEGIN_VERTICES_MAX_SIZE * GRCDEVICE.GetGPUCount())
#else
#define GPU_VERTICES_MAX_SIZE	grcDevice::BEGIN_VERTICES_MAX_SIZE
#endif
#endif

	// PURPOSE : Calculate the size of a batch for rendering using GetVertices
	grcEntry int GetVerticesMaximumBatchSize(grcDrawMode dm,u32 vertexSize) grcPure;

	// PURPOSE:	Begins rendering a custom stream of vertices.  
	// PARAMS:	dm - Draw mode for primitives
	//			vertexCount - Count of vertices
	//			vertexSize - Size of each vertex
	//			indexCount - Count of indices
	//			vertexPtr - Pointer to storage caller must fill for vertex data before calling EndVertices
	//			indexPtr - Pointer to storage caller must fill for index data before calling EndIndecedVertices
	// RETURNS:	Pointer to storage caller must fill before calling EndVertices.
	//			Will return NULL on error (out of ringbuffer space), in which case you should NOT call EndVertices.
	// NOTES:	The total size of vertexCount*vertexSize must be no larger than BEGIN_VERTICES_MAX_SIZE.
	//			Furthermore, this is typically write-combined memory so the fields should be written in order.
	grcEntry void* BeginVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize) grcPure;
	grcEntry void* BeginIndexedVertices(grcDrawMode dm,u32 vertexCount,u32 vertexSize,u32 indexCount,void** vertexPtr,void** indexPtr, u32 streamID = 0);
#if DEVICE_IM_INSTANCED_INDEXED
	grcEntry void* BeginIndexedInstancedVertices(grcDrawMode dm, u32 instanceCount, u32 instanceSize, u32 indexCount, void** instancePtr, void** indexPtr, u32 streamID);
#endif

	// PURPOSE:	Type-safe version of BeginVertices.  Couldn't get it to work right, my template-fu is weak.
	// template <class _Vtx> grcEntry void BeginTypedVertices(grcDrawMode dm,u32 vertexCount,_Vtx *&vtx) { vtx = (_Vtx*) BeginVertices(dm,vertexCount,sizeof(_Vtx)); }

	// PURPOSE:	Signals that all data for the last BeginVertices has been written.
	grcEntry void EndCreateVertices(const void *bufferEnd = NULL) grcPure;
	grcEntry void EndCreateVertices(u32 vertexCount) grcPure;
	grcEntry void EndCreateIndexedVertices(u32 indexCount, u32 vertexCount) grcPure;
#if RAGE_INSTANCED_TECH
	grcEntry void EndCreateInstancedVertices(const void *bufferEnd = NULL) grcPure;
#if RSG_ORBIS
	grcEntry void EndCreateInstancedVertices(const void* ASSERT_ONLY(bufferEnd), int numInst) grcPure;
#endif
#endif
#if __D3D11 || RSG_ORBIS
	grcEntry void EndCreateVertices(const void *bufferEnd, u32 noOfIndices, const grcIndexBuffer &indexBuffer) grcPure;
#endif
#if DEVICE_IM_INSTANCED_INDEXED
	grcEntry void EndCreateIndexedInstancedVertices(const void *bufferEnd, u32 vertexSize, u32 noOfIndices, const grcVertexBuffer &vertexBuffer, const grcIndexBuffer &indexBuffer, u32 numInstances, u32 startIndex, u32 startVertex, u32 startInstance) grcPure;
#endif

	// PURPOSE: Draw the vertices from the last BeginVertices/EndCreateVertices.
	grcEntry void DrawVertices(const void *bufferEnd = NULL) grcPure;
	grcEntry void DrawVertices(u32 vertexCount) grcPure;
	grcEntry void DrawIndexedVertices(u32 indexCount, u32 vertexCount) grcPure;
#if RAGE_INSTANCED_TECH
	grcEntry void DrawInstancedVertices(const void *bufferEnd = NULL) grcPure;
#if RSG_ORBIS
	grcEntry void DrawInstancedVertices(const void* ASSERT_ONLY(bufferEnd), int numInst) grcPure;
#endif
#endif
#if __D3D11 || RSG_ORBIS
	grcEntry void DrawVertices(const void *bufferEnd, u32 noOfIndices, const grcIndexBuffer &indexBuffer) grcPure;
#endif
#if DEVICE_IM_INSTANCED_INDEXED
	grcEntry void DrawIndexedInstancedVertices(const void *bufferEnd, u32 vertexSize, u32 noOfIndices, const grcVertexBuffer &vertexBuffer, const grcIndexBuffer &indexBuffer, u32 numInstances, u32 startIndex, u32 startVertex, u32 startInstance) grcPure;
#endif

	// PURPOSE: Signals that all data for the last BeginVertices/EndCreateVertices is no longer being renderred.
	grcEntry void ReleaseVertices(const void *bufferEnd = NULL) grcPure;
	grcEntry void ReleaseVertices(u32 vertexCount) grcPure;
	grcEntry void ReleaseIndexedVertices(u32 indexCount, u32 vertexCount) grcPure;
#if RAGE_INSTANCED_TECH
	grcEntry void ReleaseInstancedVertices(const void *bufferEnd = NULL) grcPure;
#if RSG_ORBIS
	grcEntry void ReleaseInstancedVertices(const void* ASSERT_ONLY(bufferEnd), int numInst) grcPure;
#endif
#endif
#if __D3D11 || RSG_ORBIS
	grcEntry void ReleaseVertices(const void *bufferEnd, u32 noOfIndices, const grcIndexBuffer &indexBuffer) grcPure;
#endif
#if DEVICE_IM_INSTANCED_INDEXED
	grcEntry void ReleaseIndexedInstancedVertices(const void *bufferEnd, u32 vertexSize, u32 noOfIndices, const grcVertexBuffer &vertexBuffer, const grcIndexBuffer &indexBuffer, u32 numInstances, u32 startIndex, u32 startVertex, u32 startInstance) grcPure;
#endif

	// PURPOSE:	Equivalent to EndCreateVertices, DrawVertices, ReleaseVertices.
	grcEntry void EndVertices(const void *bufferEnd = NULL) grcPure;
	grcEntry void EndVertices(u32 vertexCount) grcPure;
	grcEntry void EndIndexedVertices(u32 indexCount, u32 vertexCount) grcPure;
#if RAGE_INSTANCED_TECH
	grcEntry void EndInstancedVertices(const void *bufferEnd = NULL) grcPure;
#if RSG_ORBIS
	grcEntry void EndInstancedVertices(const void* ASSERT_ONLY(bufferEnd), int numInst) grcPure;
#endif
#endif
#if __D3D11 || RSG_ORBIS
	grcEntry void EndVertices(const void *bufferEnd, u32 noOfIndices, const grcIndexBuffer &indexBuffer) grcPure;
#endif
#if DEVICE_IM_INSTANCED_INDEXED
	grcEntry void EndIndexedInstancedVertices(const void *bufferEnd, u32 vertexSize, u32 noOfIndices, const grcVertexBuffer &vertexBuffer, const grcIndexBuffer &indexBuffer, u32 numInstances, u32 startIndex, u32 startVertex, u32 startInstance) grcPure;
#endif



#if RSG_DURANGO || RSG_ORBIS
	grcEntry void SetIndices(const u16 *indices) grcPure;
#endif
	grcEntry void SetIndices(const grcIndexBuffer& pBuffer) grcPure;

	grcEntry void SetStreamSource(u32 streamNumber,const grcVertexBuffer& pStreamData,u32 offsetInBytes,u32 stride) grcPure;
#if __D3D11
#if RSG_DURANGO
	grcEntry void SetStreamSource(u32 streamNumber, const void *pStreamData, u32 stride) grcPure;
#else
	grcEntry void SetStreamSource(u32 streamNumber, const ID3D11Buffer* pStreamData, u32 offsetInBytes, u32 stride) grcPure;
#endif
#endif
	grcEntry void ClearStreamSource(u32 streamNumber) grcPure;

#if RSG_DURANGO
	template<ShaderType SHADER_TYPE> grcEntry void SetConstantBuffer(grcContextHandle *ctx, u32 slot, const void *data) grcPure;
	template<ShaderType SHADER_TYPE> grcEntry void SetConstantBuffer(u32 slot, const void *data) grcPure;
#endif

#if __XENON
	grcEntry void DrawVertices(grcDrawMode dm, int startVertex, int vertexCount) grcPure;
#endif
	grcEntry void DrawIndexedPrimitive(grcDrawMode dm, int startIndex, int indexCount) grcPure;
	grcEntry void DrawIndexedPrimitive(grcDrawMode dm,const grcVertexDeclaration *decl,const grcVertexBuffer &vb,const grcIndexBuffer &ib, int indexCount) grcPure;
	grcEntry void DrawPrimitive(grcDrawMode dm, int startVertex, int vertexCount) grcPure;
	grcEntry void DrawPrimitive(grcDrawMode dm, const grcVertexDeclaration *decl,const grcVertexBuffer &vb,int startVertex, int vertexCount) grcPure;
#if __D3D11 || RSG_ORBIS
	grcEntry void DrawInstancedPrimitive(grcDrawMode dm, int vertexCountPerInstance, int instanceCount, int startVertex, int startInstance, bool alreadySetupPriorToDraw = false) grcPure;
	grcEntry void DrawInstancedIndexedPrimitive(grcDrawMode dm, int indexCountPerInstance, int instanceCount, int startIndex, int startVertex, int startInstance, bool alreadySetupPriorToDraw = false) grcPure;
#endif //__D3D11 || RSG_ORBIS

#if __D3D11 || RSG_ORBIS
	grcEntry void Dispatch(u32 groupX, u32 groupY, u32 groupZ) grcPure;
	grcEntry void RunComputation(const char* pDebugStr, grmShader &shader, u32 programId, u32 groupX, u32 groupY, u32 groupZ) grcPure;
	grcEntry void CSEnableAutomaticGpuFlush(bool enable) grcPure;
	grcEntry void SynchronizeComputeToGraphics() grcPure;
	grcEntry void SynchronizeComputeToCompute() grcPure;
	grcEntry void FlushAndWait() grcPure;
#endif
#if __D3D11 || RSG_ORBIS
	grcEntry void CopyStructureCount(grcBufferBasic* pDestBuffer, u32 DstAlignedByteOffset, grcBufferUAV* pSrcBuffer) grcPure;
	grcEntry void DrawWithGeometryShader(grcBufferBasic* pIndirectBuffer) grcPure;
	grcEntry void DrawIndexedInstancedIndirect(grcBufferBasic* pIndirectBuffer, grcDrawMode dm, bool alreadySetupPriorToDraw = false, u32 argsOffsetInBytes = 0) grcPure;
#	if RSG_DURANGO || RSG_ORBIS
	grcEntry void DrawIndexedInstancedIndirect(void *pIndirectBufferMem, grcDrawMode dm, bool alreadySetupPriorToDraw = false, u32 argsOffsetInBytes = 0) grcPure;
#	endif	//RSG_DURANGO || RSG_ORBIS
#endif	//__D3D11	|| RSG_ORBIS

	grcEntry void SetVertexShaderConstant(int startRegister, const float *data, int regCount
#if (__D3D11)
										  , grcCBuffer* poConstantBuffer = NULL
#endif // __D3D11
										  ) grcPure;
	// DOM-IGNORE-BEGIN
#if __D3D
	grcEntry void SetSoftwareVertexProcessing(bool) grcPure;

#if __WIN32PC
	grcEntry void SetTexture(int stage,const /*grcDeviceTexture*/grcTexture *pTexture) grcPure;
#endif
	grcEntry void GetTexture(int stage,grcDeviceTexture **ppTexture) grcPure;

	grcEntry void GetRenderTarget(u32 index,grcDeviceSurface **ppTarget) grcPure;
	grcEntry void SetRenderTarget(u32 index,grcDeviceSurface *pTarget) grcPure;

#if ENABLE_EDRAM_PROTECTION
	// Protect a region of EDRAM by asserting/warning if someone tries to render to this region
	// of EDRAM before it is unprotected.
	grcEntry void BeginProtectingEDRAM (grcDeviceSurface *pTarget) grcPure;
	grcEntry void EndProtectingEDRAM (grcDeviceSurface *pTarget) grcPure;
	grcEntry bool IsProtectedEDRAM (grcDeviceSurface* pTarget) grcPure;
#endif // ENABLE_EDRAM_PROTECTION

#if __D3D11
	grcEntry void SetRenderTargetsWithUAV(u32 numTargets, const grcDeviceView **papRenderTargets, const grcDeviceView *pDepthTarget, u32 UAVStartSlot, u32 numUAVs, grcDeviceView ** ppUnorderedAccessView, const u32* pUAVInitialCounts);
	grcEntry void SetupRenderTargetVars(u32 numTargets, const grcDeviceView **papRenderTargets, const grcDeviceView *pDepthTarget);

	// Ensure any cached shadowed states get re-set for the next draw. It looks like a potential d3d issue whereby these 
	// don't get automatically invalidated when written to as a UAV. So this is for manual management.
	grcEntry void ClearShadowedSRVState(grcDeviceView* pDeviceView);
#endif // __D3D11

	grcEntry void GetDepthStencilSurface(grcDeviceSurface **ppSurface) grcPure;
	grcEntry void SetDepthStencilSurface(grcDeviceSurface *ppSurface) grcPure;	
	grcEntry void GetRenderTargets(u32 numTargets, grcDeviceView **papRenderTargets, grcDeviceView **ppDepthTarget) grcPure;
	grcEntry void SetRenderTargets(u32 numTargets, const grcDeviceView **papRenderTargets, const grcDeviceView *pDepthTarget) grcPure;

	grcEntry Result CreateRenderTarget(u32 width,u32 height,u32 format,u32 multisample,u32 multisampleQuality,bool lockable,grcDeviceSurface **ppSurface, const grcDeviceSurfaceParameters *params) grcPure;
	grcEntry Result CreateTexture(u32 width,u32 height,u32 depth,u32 levels,u32 usage,u32 format,u32 pool,int type,grcDeviceTexture **ppTexture
						#if __XENON
							, u32 *poolAllocSize=NULL
						#endif
					) grcPure;
	grcEntry Result CreateDepthStencilSurface(u32 width,u32 height,u32 format,bool discard,grcDeviceSurface **ppSurface) grcPure;
	grcEntry void DeleteTexture(grcDeviceTexture * &pTexture) grcPure;
	grcEntry void DeleteSurface(grcDeviceSurface * &pSurface) grcPure;

#if __XENON
	DEPRECATED grcEntry Result Resolve(u32 targetMask, grcDeviceTexture *pTarget);
#endif

	// PURPOSE: Callbacks that are made when a device is detected to be lost, and after it has been reset
	grcEntry void RegisterDeviceLostCallbacks(Functor0 lostCallback, Functor0 resetCallback) grcPure;

#if NV_SUPPORT
	// PURPOSE: Callbacks that are made when stereo device changes
	grcEntry void RegisterStereoChangeSepCallbacks(Functor1<float> changeCallback) grcPure;
	grcEntry void RegisterStereoChangeConvCallbacks(Functor1<float> changeCallback) grcPure;
	//grcEntry void RemoveStereoChangeCallbacks() grcPure;
#endif

	//
	// PURPOSE
	//	Remove all device lost callbacks that are called on the callee.  Passing
	//  in NULL will cause all static functions to be removed.
	// PARAMS
	//	callee - the object instance to compare against the functor callees
	//
	grcEntry void RemoveDeviceLostCallbacksThatHaveCallee(void* callee) grcPure;

    grcEntry void UnregisterAllDeviceLostCallbacks() grcPure;
#endif
	// DOM-IGNORE-END


	// PURPOSE:	Specify the screen resolution
	// PARAMS:	width - Screen width
	//			height - Screen height
	// NOTES:	Must call this before InitClass or it will not behave as expected.
	grcEntry void SetSize(int width,int height);
	grcEntry void InitGlobalWindow();

	// RETURNS:	Current screen width
	static inline int GetWidth();

	// RETURNS:	Backbuffer width.  Never changes except on PC window resize.
	static inline int GetGlobalWidth();

	// RETURNS: Current screen height
	static inline int GetHeight();

	// RETURNS:	Backbuffer height.  Never changes except on PC window resize.
	static inline int GetGlobalHeight();

	// RETURNS: Refresh Rate
	grcEntry inline int GetRefreshRate();

	// RETURNS: Backbuffer display window reference.
	static inline const grcDisplayWindow& GetGlobalWindow();

	// RETURNS: MonitorConfiguration instance reference
	static inline const MonitorConfiguration& GetMonitorConfig();

#if __D3D
	// RETURNS:	Current Direct3D device handle
	grcEntry inline grcDeviceHandle* GetCurrent() grcPure;
	grcEntry grcDeviceHandle* GetCurrentInner() grcPure;

#	if __WIN32PC
		// RETURNS: Get Rage's Texture object from device's texture
		grcEntry grcTexture* GetTexture(const grcTextureObject*) grcPure;
#	elif RSG_DURANGO
		grcEntry inline ID3D11DmaEngineContextX* GetDmaEngine1() grcPure;
		grcEntry inline ID3D11DmaEngineContextX* GetDmaEngine2() grcPure;
#	endif

	grcEntry grcPresentParameters* GetPresentParameters() grcPure;
#endif // __D3D

#if DEVICE_RESOLVE_RT_CONFLICTS
	// DX11 runtime on PC nulls out shader resource view if the target is being locked for rendering.
	// We've got to do the same since we cache the current texture bound state.
	grcEntry void NotifyTargetLocked(const grcDeviceView* view) grcPure;
#endif // DEVICE_RESOLVE_RT_CONFLICTS

	// PURPOSE:	Set window title
	// PARAMS:	appName - New window title
	// NOTES:	Can call this at any time.  Only effective on __WIN32PC builds.
	grcEntry void SetWindowTitle(const char *appName) grcPure;

	// PURPOSE:	Sets frame lock (minimum number of vblanks to wait per frame)
	// PARAMS:	frameLock - New framelock value (zero is immediate)
	//			swapImmediateIfLate - If true, swap immediately if we ran over
	//				our frame time.  Otherwise swap only on next vblank.
	grcEntry void SetFrameLock(int frameLock,bool swapImmediateIfLate) grcPure;
	grcEntry void SetFrameLockNoReset(int frameLock);

#if RSG_PC
	grcEntry void SetFrameLockOverride(bool bEnable) { sm_FrameLockOverride = bEnable; }
	grcEntry bool GetFrameLockOverride() { return sm_FrameLockOverride; }
#endif // __WIN32PC

	// RETURNS: Current frame lock value
	grcEntry int GetFrameLock() grcPure;

	// RETURNS:	True if user has clicked the close window button or pressed Alt+F4
	//			(obviously only effective on __WIN32PC builds)
	grcEntry bool QueryCloseWindow() grcPure;

	// PURPOSE:	Clears the internal flag set by QueryCloseWindow
	grcEntry void ClearCloseWindow() grcPure;

#if RSG_PC
	// PURPOSE:	Sets the internal flag set by pressing Alt+F4
	grcEntry void SetCloseWindow() grcPure;
#endif

#if __D3D
	// RETURNS: Vertex declarator suitable for screen blits
	grcEntry inline grcVertexDeclaration* GetBlitDecl();
	grcEntry inline grcVertexDeclaration* GetImmediateModeDecl();
#endif

	// PURPOSE:	Clear a portion of the screen to specified z and color value.
	// PARAMS:	x - Destination x
	//			y - Destination y
	//			width - Width to clear
	//			height - Height to clear
	//			z - Destination z value.  Note that the z value is expected to be in 0..1 space
	//				but is NOT reversed internally on PS2 on the assumption that it's the output
	//				of a vertex projection function that would have already applied this anyway.
	//				See grcDepthClosest and grcDepthFarthest constants.
	//			color - reference to a Packed color value
	// NOTES:	This entry point will be deprecated in favor of the more featured version below.
	grcEntry void ClearRect(int x,int y,int width,int height,float depth,const Color32 & color);

	// PURPOSE:	Clear a portion of the screen to specified z, stencil, and color value.
	// PARAMS:	x - Destination x
	//			y - Destination y
	//			width - Width to clear
	//			height - Height to clear
	//			clearColor - reference to a Packed color value
	//			enableClearColor - True to clear color, else false
	//			clearColor - Color to clear to
	//			enableClearDepth - True to clear depth, else false
	//			clearDepth - Clear value for depth buffer (you probably want grcDepthFarthest)
	//			enableClearStencil - True to clear depth, else false
	//			clearStencil - Clear value for stencil buffer
	grcEntry void ClearRect(int x,int y,int width,int height,bool enableClearColor,Color32 clearColor,bool enableClearDepth,float clearDepth,bool enableClearStencil,u32 clearStencil ORBIS_ONLY(,bool yFlipped = false) DURANGO_ONLY(,bool yFlipped = false) WIN32PC_ONLY(,bool yFlipped = false));

	// For backward compatibility
	grcEntry void ClearRect(int x,int y,int width,int height,bool enableClearColor,Color32 clearColor,bool enableClearDepthStencil,float clearDepth,u32 clearStencil) {
		ClearRect(x,y,width,height,enableClearColor,clearColor,enableClearDepthStencil,clearDepth,enableClearDepthStencil,clearStencil);
	}

	// PURPOSE: Blit a portion of the currently active texture to the specified screen location.
	// PARAMS:	x1 - Destination base x
	//			y1 - Destination base y
	//			x2 - Destination opposite-corner x
	//			y1 - Destination opposite-corner y
	//			z - Destination z value.  Note that the z value is expected to be in 0..1 space
	//				but is NOT reversed internally on PS2 on the assumption that it's the output
	//				of a vertex projection function that would have already applied this anyway.
	//				See grcDepthClosest and grcDepthFarthest constants.
	//			u1 - Source texture base u (in non-normalized texels)
	//			v1 - Source texture base v (in non-normalized texels)
	//			u2 - Source texture opposite-corner u (in non-normalized texels)
	//			v2 - Source texture opposite-corner v (in non-normalized texels)
	//			color - reference to Packed color value
	/*DEPRECATED*/ grcEntry void BlitRect(int x1, int y1, int x2, int y2, float zVal, int u1, int v1, int u2,int v2, const Color32 &color);

	// PURPOSE:	Blit a portion of the currently active texture to the specified screen location.
	// PARAMS:	x1 - Destination base x
	//			y1 - Destination base y
	//			x2 - Destination opposite-corner x
	//			y1 - Destination opposite-corner y
	//			z - Destination z value.  Note that the z value is expected to be in 0..1 space
	//				but is NOT reversed internally on PS2 on the assumption that it's the output
	//				of a vertex projection function that would have already applied this anyway.
	//				See grcDepthClosest and grcDepthFarthest constants.
	//			u1 - Source texture base u (in normalized texels)
	//			v1 - Source texture base v (in normalized texels)
	//			u2 - Source texture opposite-corner u (in normalized texels)
	//			v2 - Source texture opposite-corner v (in normalized texels)
	//			color - reference to Packed color value
	/*DEPRECATED*/ grcEntry void BlitRectf(float x1, float y1, float x2, float y2, float zVal, float u1, float v1, float u2,float v2, const Color32 & color);

	// PURPOSE:	Blit many sections of currently active texture to specified screen location; used by text output
	// PARAMS:	posx - Position, in screen space
	//			posy - Position, in screen space
	//			posz - Position, in screen space
	//			destxywh - Destination rectangle (x, y, width, height; four elements per character; S12.4 fixed point)
	//			srcxywh - Source rectangle (x, y, width, height; four elements per character)
	//			count - Number of characters to render
	//			color - Color of every character
	//			bilinear - True if texture is bilinear (adds a half-pixel offset in this case)
	grcEntry void BlitText(int posx,int posy,float posz,const s16 *destxywh,const u8 *srcxywh,int count,Color32 color,bool bilinear);

	// PURPOSE:	Configure the system to blit faster.  This is called internally by BlitRect and BlitRectf if necessary
	//			but you can see substantial blitting performance increases if you call it yourself.  When blitting is
	//			active, don't do any rendering calls other than Blit functions.  Calls will properly nest.
	/*DEPRECATED*/ grcEntry void BeginBlit();

	// PURPOSE:	Call this after you're done blitting (must match one-for-one with BeginBlit calls)
	/*DEPRECATED*/ grcEntry void EndBlit();

	// PURPOSE:	Set scissor rectangle and enables scissor test
	// PARAMS	x - Upper left column
	//			y - Upper left row
	//			width - width of scissor rectangle
	//			height - height of scissor rectangle
	grcEntry void SetScissor(int x,int y,int width,int height);

	// PURPOSE:	Disable the scissor test (undoes the last SetScissor call)
	grcEntry void DisableScissor();

	// PURPOSE:	Get current scissor rectangle
	// PARAMS:	x - Upper left column
	//			y - Upper left row
	//			width - width of scissor rectangle
	//			height - height of scissor rectangle
	grcEntry void GetScissor(int &x,int &y,int &width,int &height);

	// PURPOSE: Get the extents of the safe zone
	// PARAMS:	x0 = Top Left x coordinate
	//			y0 = Top Left y coordinate
	//			x1 = Bottom right x coordinate
	//			y1 = Bottom right y coordinate
	grcEntry void GetSafeZone(int &x0, int &y0, int &x1, int &y1);

	// PURPOSE:	Set clip plane enable mask
	// PARAMS:	enableMask - bitmask of which of up to six planes to enable
	// RETURNS:	Previous enable mask
	grcEntry u32 SetClipPlaneEnable(u32 enableMask);

	// PURPOSE:	Returns current clip plane enable mask
	grcEntry u32 GetClipPlaneEnable();

	// PURPOSE:	Define a particular clip plane.  See important note below.
	// PARAMS:	index - index of clip plane to set
	//			plane - clip plane value, in world space.
	// NOTES:	The coefficients that this method sets take the form of the general plane equation. 
	//			If the values in the array in the pPlane parameter are labeled A, B, C, and D in the 
	//			order that they appear in the array, they fit into the general plane equation so that 
	//			Ax + By + Cz + Dw = 0. A point with homogeneous coordinates (x, y, z, w) is visible 
	//			in the half-space of the plane if Ax + By + Cz + Dw >= 0. Points that exist behind the 
	//			clipping plane are clipped from the scene.  IMPORTANT: You must re-send down any
	//			clip planes after a camera or world matrix change, because the planes are transformed
	//			by the current composite matrix before being sent down to the hardware.
	grcEntry void SetClipPlane(int index,Vec4V_In plane);

#if !HACK_GTA4 // no shader clip planes
	// PURPOSE:	Retrieve a particular clip plane
	// PARAMS:	index - index of clip plane to retrieve
	//			plane - Destination buffer for clip plane.
	grcEntry void GetClipPlane(int index,Vec4V_InOut plane);
#endif // !HACK_GTA4

	grcEntry u32 GetFrameCounter();

	grcEntry u32 GetSyncCounter() { return sm_SyncCounter; }

	grcEntry void IncrementSyncCounter() { sm_SyncCounter = (sm_SyncCounter + 1) & 0x7FFFFFFF; }

	// PURPOSE:	Defines global shader constant configuration.  Call this before InitClass.
	// PARAMS:	globalSize - Total number of registers reserved for globals (they are always based at zero,
	//				and this number must be at least 16 to leave room for the built-in matrices)
	//			skinningBase - Typically the same as globalSize (and must be at least as large as it),
	//				this defines where the skinning matrices start.
	grcEntry void ConfigureGlobalConstants(int globalSize,int skinningBase);

	enum Stereo_t { MONO, STEREO, AUTO };

#if __WIN32PC
#if __D3D

	grcEntry void GetAdapterDescription(DXGI_ADAPTER_DESC &oAdapterDesc);

	// PURPOSE:	Initializes the current video adapter ordinal (WIN32PC only)
	// RETURNS:	nothing
	grcEntry void InitAdapterOrdinal();

	// PURPOSE:	Set the current video adapter ordinal (WIN32PC only)
	// PARAMS:	ordinal - Zero-based adapter ordinal
	// NOTES:	Must call this before InitClass for it to take effect.
	//			If this number is out of range the code will use zero instead.
	grcEntry void SetAdapterOrdinal(int ordinal);

	// PURPOSE:	Returns the current video adapter ordinal (WIN32PC only)
	// RETURNS:	Current video adapter ordinal
	grcEntry int GetAdapterOrdinal();

	// PURPOSE:	Returns number of video adapters installed on the machine.
	// NOTES:	It is valid to call this before InitClass; the code will
	//			create a temporary D3D interface if necessary.
	grcEntry int GetAdapterCount();

	// PURPOSE:	Returns number of video modes support by a given adapter.
	// NOTES:	You should cache the results internally as this is slow query
	grcEntry int GetDisplayModeCount(int ordinal);

	// PURPOSE:	Get video mode attributes of a specific index of adapter.
	// PARAMS:	ordinal - Zero-based adapter ordinal
	//			mode - Video Mode Index
	//			width - Returns width of video mode
	//			height - Returns height of video mode
	//			refreshrate - Returns refresh rate of video mode
	// NOTES:	You should cache the results internally as this is slow query
	grcEntry bool GetDisplayMode(int ordinal, int mode, int &width, int &height, int &refreshrate);

	grcEntry void SetOutputMonitor(int monitor);

	grcEntry int GetOutputMonitor();

	grcEntry void SetAspectRatio(float fAspectRatio);


	grcEntry void InitNVIDIA();
	grcEntry bool IsUsingNVidiaAPI()		{ return sm_UseNVidiaAPI; }
	grcEntry size_t RetrieveVideoMemory();
	grcEntry bool IsUsingVendorAPI()		{ return sm_UseVendorAPI; }

	// PURPOSE: Get Available Video Memory
	grcEntry s64 GetAvailableVideoMemory(int adapter = -1);

	// PURPOSE: Get Video card manufacturer
	grcEntry DeviceManufacturer GetManufacturer(int adapter = -1);

	// PURPOSE: Handle the device callbacks during device lost state
#if __D3D11
	static void DeviceLostShutdown();
#endif
	static void DeviceLost();

	// PURPOSE: Handle the device callbacks during device restoration
	static void DeviceRestored();

	static unsigned int DeviceStatus() { return sm_uDeviceStatus; }

private:
#if __D3D9
	grcEntry int SearchFor( const u16* pwszSearchName, u16* pwszResult, u16* wszParentName, IDxDiagContainer* pDxDiagContainer );
#endif // __D3D9
public:
#if __D3D9
	grcEntry bool Query(u16* pszData, const u16* pszField);
#endif // __D3D9
#endif // __D3D

	// PURPOSE: Gets member bool that determines if the window has lost it's focus.
	// NOTES:   Use after InitClass.
	static bool GetLostFocus();

	// PURPOSE: Gets member bool that determines if the window has lost it's focus for audio.
	// NOTES:   Use after InitClass.
	static bool GetLostFocusForAudio();

	// PURPOSE: Sets member bool that determines if the window has lost it's focus.
	// NOTES:   Use after InitClass.
	static void SetLostFocus(bool val);

	// PURPOSE: Sets member bool that determines if the window has lost it's focus for audio.
	// NOTES:   Use after InitClass.
	static void SetLostFocusForAudio(bool val);

	// PURPOSE: Sets member bool that determines if the window should block
	//          updating when it loses focus.
	// NOTES:   Use after InitClass.
	static void SetBlockOnLostFocus(bool val);

	// PURPOSE: Gets member bool that determines if fullscreen/windowed toggle is enabled.
	// NOTES:   Use after InitClass.
	grcEntry bool IsFullscreenToggleEnable();

	// PURPOSE: Sets member bool that determines if fullscreen/windowed toggle is enabled.
	// NOTES:   Use after InitClass.
	grcEntry void SetFullscreenToggleEnable(bool val);

	// PURPOSE: Toggle fullscreen/windowed mode
	// NOTES:   Use after InitClass.
	grcEntry bool ToggleFullscreen();

	grcEntry bool GoWindowed();
	grcEntry bool GoFullscreen();

	// PURPOSE: Reports if the application is in windowed or full screen mode
	// NOTES:   Use after InitClass
	grcEntry bool IsWindowed();

	// PURPOSE: Gets member bool that determines if the window should block
	//          updating when it loses focus.
	// NOTES:   Use after InitClass.
	grcEntry bool GetBlockOnLostFocus();

	// PURPOSE: Callback when focus state changes.
	// NOTES:
	grcEntry void SetFocusCallback(FOCUSCALLBACK callback);

	// PURPOSE: Gets member bool that determines if the window should block
	//          updating when it loses focus.
	// NOTES:   Use after InitClass.
	grcEntry RENDERCALLBACK GetRenderCallback();

	// PURPOSE: Callback issed to issue screen update
	grcEntry void SetRenderCallback(RENDERCALLBACK callback);

	grcEntry bool IsInReset() { return sm_bIssuingReset; }
	grcEntry void SetInReset(bool bReset) { sm_bIssuingReset = bReset; }


#if __D3D11
	grcEntry bool ProcessResizeBuffersWhileEncoding();
#endif
	// PURPOSE: Check wheather the device has changed states
	grcEntry bool CheckForDeviceChanges();

	grcEntry bool IsReady();

	grcEntry const grcDisplayWindow& GetCurrentWindow() { return sm_CurrentWindows[g_RenderThreadIndex]; }

#if NV_SUPPORT
	grcEntry void ChangeStereoSep(float fSep);
	grcEntry void ChangeStereoConv(float fConv);
#endif
	grcEntry bool StereoIsPossible();
	//grcEntry bool StereoIsActive();
	grcEntry void UpdateStereoStatus();
	grcEntry void InitializeStereoSystem(bool useStereo);
	grcEntry void DeInitStereo();
	grcEntry bool InitializeStereo();

	grcEntry void ForceStereorizedRT(Stereo_t bForce);
	grcEntry bool IsStereoEnabled() { return sm_bStereoEnabled; }
	grcEntry bool CanUseStereo()	{ return sm_bCanUseStereo; }
	grcEntry void SetStereoScissor(bool bEnable = true)	{ sm_bStereoScissor = bEnable; }
	grcEntry void SetStereoTexture();
	//grcEntry grcTexture* GetStereoTexture() { return sm_StereoTex; }
	grcEntry void UpdateStereoTexture(const Vector3& vCamOffset,const Vector3& vCamOffset1);
	grcEntry bool ActivateStereo(bool bActivate = false);

	grcEntry float GetEyeSeparation();
	grcEntry float GetCachedEyeSeparation() { return sm_fEyeSeparation; }
	grcEntry float GetCachedConvergenceDistance() { return sm_fStereoConvergence; }
	grcEntry float GetConvergenceDistance(); // { return sm_fStereoConvergence; }
	grcEntry float GetDefaultConvergenceDistance()	{ return sm_fDefaultStereoConvergence; } 
	grcEntry void  SetDefaultConvergenceDistance(float fConv);
	grcEntry void  SetConvergenceDistance(float fConv);
	grcEntry void  SetSeparationPercentage(float fSepPercentage, bool bSaveVal = true, bool bForceUpdate = false);
	grcEntry float GetSeparationPercentage(bool bUpdate = true);// { return sm_fStereoSeparationPercentage; }
	grcEntry float GetDefaultSeparationPercentage()	{ return sm_fStereoSeparationPercentage; }

	grcEntry void SetRequestStereoDesired(bool request) { sm_StereoDesired = request;}
	grcEntry int  GetRequestStereoDesired() { return sm_StereoDesired; } 

	grcEntry void SetDesiredSeparation(float fSep)	{ sm_DesiredSeparation = fSep; }
	grcEntry void SetDesiredConvergence(float fConv)	{ sm_DesiredConvergence = fConv; }

#if ATI_EXTENSIONS
	grcEntry void OpenAMDExtensionInterfaces(ID3D11Device *pD3DDevice);
	grcEntry void CloseAMDExtensionInterfaces();
#endif //ATI_EXTENSIONS

	grcEntry void SetUAVSync(bool bEnable = true);
	grcEntry bool GetUAVSync() { return sm_bUAVSync; }

	static bool sm_bUAVSync;
#endif // __WIN32PC

#if RSG_PC || RSG_DURANGO
	// PURPOSE: Functions used to initialize vertex buffers required for immediate mode rendering
	grcEntry void Blit_Init();
	grcEntry void Blit_Shutdown();
	grcEntry void GRC_Init();
	grcEntry void GRC_Shutdown();
#endif

	static void GetMultiSample(u32 &uType, u32 &uQuality);

	// PURPOSE: Returns true if console HW is requesting widescreen video output         
	// NOTES:   Defaults to false for HW that does not have a system level setting for this
	//			This function can be call prior to calling InitClass()
	grcEntry bool GetWideScreen();

	// PURPOSE: Returns true if console HW is Hi definition video output         
	// NOTES:   Defaults to false for HW that does not have a system level setting for this
	//			This function can be call prior to calling InitClass()
	grcEntry bool GetHiDef();
	
#if __PPU
	// PURPOSE: Returns true if console HW is running an interlaced mode.
	grcEntry bool GetInterlaced();
#endif // __PPU	
	// PURPOSE: request the the device automatically letterbox the video output (it the user has selected "widescreen" mode but has a 4:3 TV
	// NOTES: Defaults to true
	grcEntry void SetLetterBox(bool enable);

	// RETURNS:	True if calling thread owns the GPU.
	static bool CheckThreadOwnership() { return g_grcCurrentContext != NULL; }

	// PURPOSE:	Kicks off the GPU even if the current segment isn't full yet.
	// NOTES:	Don't call cellGcmFlush directly yourself, because that might not launch dependent 
	//			SPU tasks and the GPU will wait indefinitely on a label that is never triggered.
	//			This is similar to InsertFence but is much less expensive on PS3 because it doesn't
	//			write a backend synch label.
	grcEntry void KickOffGpu();

	// PURPOSE:	Creates an occlusion query
	// NOTES: On 360 the tile count determines whether or not this query will be executed between Begin/EndTiling
	grcEntry grcOcclusionQuery CreateOcclusionQuery(int tileCount = -1);

	// PURPOSE:	Destroys an occlusion query
	grcEntry void DestroyOcclusionQuery(grcOcclusionQuery& query);

	// PURPOSE:	Issues the beginning of an occlusion query
	// NOTES: On 360 Don't call in between Begin/EndTiling unless the tiled count was specified on-creation
	grcEntry void BeginOcclusionQuery(grcOcclusionQuery query);

	// PURPOSE:	Issues the end of an occlusion query
	// NOTES: On 360 Don't call in between Begin/EndTiling unless the tiled count was specified on-creation
	grcEntry void EndOcclusionQuery(grcOcclusionQuery query);

	// PURPOSE:	Returns the number of pixels that passed the Z test in between the begin and end issue
	// NOTES: Call after EndOcclusionQuery
	grcEntry bool GetOcclusionQueryData(grcOcclusionQuery query, u32& numDrawn);

	// PURPOSE:	Creates a conditional rendering query
	grcEntry grcConditionalQuery CreateConditionalQuery();

	// PURPOSE:	Destroys a conditional rendering query
	grcEntry void DestroyConditionalQuery(grcConditionalQuery& query);

	// PURPOSE:	Issues the beginning of a conditional rendering query
	grcEntry void BeginConditionalQuery(grcConditionalQuery query);

	// PURPOSE:	Issues the end of a conditional rendering query
	grcEntry void EndConditionalQuery(grcConditionalQuery query);

	// PURPOSE:	Issues the beginning of a conditional rendering
	// NOTES: The next drawcalls will be ignored by the GPU based on the result of the query.
	grcEntry void BeginConditionalRender(grcConditionalQuery query);

	// PURPOSE:	Issues the end of a conditional rendering
	grcEntry void EndConditionalRender(grcConditionalQuery query);

#if __PS3
	grcEntry bool GpuMemCpy(void *dest,const void *src,size_t bytes,u32 &setToNonZeroWhenDone);
#endif

#if COMMAND_BUFFER_SUPPORT
	// PURPOSE: Create a command buffer object.
	// PARAMS:	size - size, in bytes, of the buffer.  Try 64k for starters and work
	//				down from there after looking at the return value of EndCommandBuffer.
	//			pbuffer - Pointer to buffer object pointer to fill out
	// NOTES:	*pbuffer will contain a NULL pointer if the allocation failed, presumably due
	//			to a shortage of physical memory.  Be sure to check the pointer before assuming
	//			it is valid.  To avoid double create-and-frees when trying to get the size right,
	//			consider using a single global command buffer that you record into to determine
	//			the size, then create the final command buffer in exactly the correct final size.
	grcEntry void CreateCommandBuffer(size_t size,grcCommandBuffer **pbuffer);

	// PURPOSE:	Delete a command buffer object.
	// PARAMS:	buffer - Pointer to buffer to free; NULL is allowed and ignored
	grcEntry void DeleteCommandBuffer(grcCommandBuffer *buffer);

	// PURPOSE:	Begin recording a command buffer.  All render commands will go into
	//			the command buffer until EndCommandBuffer is called.  The system
	//			will assert out if you overrun your buffer, so you may want to start
	//			with a large buffer, record it, and use the return value of EndCommandBuffer
	//			to create an appropriately-sized final command buffer.
	// PARAMS:	buffer - Buffer to begin recording into.  This can be a buffer you've already
	//				used before for other recordings.
	//			isSkinned - True if the object will be skinned or not; currently ignored,
	//				but may become important so we know to mark the world matrix arrays
	//				as inheritable.  Even then we'll have to somehow limit our matrix count
	//				so that it all fits at once.
	// NOTES:	BeginCommandBuffer calls cannot nest.
	grcEntry void BeginCommandBuffer(grcCommandBuffer *buffer,bool isSkinned);

	// PURPOSE:	End recording a command buffer; further graphics operations are directed
	//			to the normal display device.
	// PARAMS:	cloneBuffer: pointer to a grcCommandBuffer; if not NULL,
	//				this receives a clone of what had just been recorded.  Do not record
	//				inline vertex runs into the buffer you're cloning.
	// RETURNS:	Total number of bytes consumed in the command buffer.
	// NOTES:	The cloned buffer should be freed normally via DeleteCommandBuffer.
	grcEntry size_t EndCommandBuffer(grcCommandBuffer **cloneBuffer);

	// PURPOSE:	Execute the command buffer.
	// PARAMS:	buffer - Pointer to buffer to execute, cannot be NULL.
	//			predicationSelect - See Xenon docs for this, set to zero normally.
	grcEntry void RunCommandBuffer(grcCommandBuffer *buffer,u32 predicationSelect);


	//
	// PURPOSE
	//	Sets the predication that should be used while recording the pre-compiled command buffer. 
	// PARAMS
	//  tile - Bitmask representing which tiles and z-prepass modes the subsequent rendering 
	//		should be done on. A value of zero disables the predication override and resumes 
	//		the normal behavior - that is, having all subsequent rendering done for every tile.
	//	run - If the bitwise AND of the RunPredication parameter (set at record-time) and the 
	//        bitwise AND of the PredicationSelect parameter of the RunCommandBuffer method 
	//		  (set at run-time) is zero, rendering and any state is skipped. A value of zero 
	//		  disables the predication override and resumes the normal behavior.
	grcEntry void SetCommandBufferPredication(u32 tile,u32 run);

	// RETURNS:	True if currently recording a command buffer.  Used by low-level rage
	//			code to suppress sending down certain shader constants which we want to
	//			be inheritable.
	static bool IsRecordingCommandBuffer() { return g_grcCommandBuffer != 0; }
#else
	static bool IsRecordingCommandBuffer() { return false; }
#endif

#if __PPU && DRAWABLESPU_STATS
	// PURPOSE:	Allows higher-level code to gather spuDrawableStats by specifying a buffer to write to
	grcEntry void SetFetchStatsBuffer(spuDrawableStats* dstPtr);
#elif DRAWABLE_STATS
	// PURPOSE:	Allows higher-level code to gather drawableStats by specifying a buffer to write to
	grcEntry void SetFetchStatsBuffer(drawableStats* dstPtr);
#endif

#if __BANK && !__SPU
	static void AddWidgets( bkBank &bank ) 
	{ 
		bank.AddText( "Screen Width", (int*)&sm_CurrentWindows[0].uWidth, true ); 
		bank.AddText( "Screen Height", (int*)&sm_CurrentWindows[0].uHeight, true );
		sm_MonitorConfig.addWidgets(bank);

#if !__RESOURCECOMPILER && !__TOOL
#if RSG_PC
		grcDevice::AddWidgetsPC(bank);
#endif // RSG_PC

#if __PS3
		grcDevice::AddWidgetsPS3();
#endif // __PS3
#endif //!__RESOURCECOMPILER && !__TOOL
	}

#if !__RESOURCECOMPILER && !__TOOL
#if RSG_PC
	static void AddWidgetsPC(bkBank &bank);
#elif __PS3
	static void AddWidgetsPS3();
#endif // __PS3
#endif //!__RESOURCECOMPILER && !__TOOL

#endif // __BANK && !__SPU
#if GRCDBG_IMPLEMENTED

#if RSG_PC && __D3D11
#	define GRCDBG_PUSH(name)            PIXBegin(0,name)
#	define GRCDBG_POP()                 PIXEnd()
#else
#	define GRCDBG_PUSH(name)            GRCDEVICE.PushFaultContext(name)
#	define GRCDBG_POP()                 GRCDEVICE.PopFaultContext()
	grcEntry void PushFaultContext(const char*);
	grcEntry void PopFaultContext();
#endif // __D3D11

#else
#	define GRCDBG_PUSH(name)            (void)0
#	define GRCDBG_POP()                 (void)0
#endif

#if RSG_PC || RSG_DURANGO
	static grcSwapChain* GetSwapChain()
#if RSG_DURANGO || !__D3D11
		{ return sm_pSwapChain; }
#else
		;
#endif // RSG_DURANGO || !__D3D11
#endif // RSG_PC || RSG_DURANGO

#if __WIN32PC	// For LIVE integration	
	static void (*sm_ResetCallback)(grcDeviceHandle *,grcPresentParameters*);
	static void (*sm_PresentCallback)(grcDeviceHandle *device,const tagRECT *pSourceRect,const tagRECT *pDestRect,HWND__ *hwndOverride,const tagRGNDATA *dirtyRegion);
	static void (*sm_PresentHandler)(int syncInterval, u32 uFlags);
	static void (*sm_PostPresentCallback)();
#endif // __WIN32PC

#if __D3D11 || RSG_ORBIS
	// PURPOSE: Get and increment a context sequence number.  Higher level code
	//          can pass this value to worker threads that create contexts.
	grcEntry u32 GetAndIncContextSequenceNumber();

	// PURPOSE: Creates a context in TLS for this thread.  Must be called before you can make most graphics calls.
	// NOTES: Must be called from the worker thread
	grcEntry void CreateContextForThread();

	// PURPOSE: Destroys the context created by CreateContextForThread
	grcEntry void DestroyContextForThread();

	// PURPOSE: Prepares the thread's context for use this frame (similar to BeginFrame)
	// PARAMS:  control - structure that will be used to asynchronously update the behaviour of the context.
	// NOTES:   Must be called from the worker thread
	grcEntry void BeginContext(
		GRCGFXCONTEXT_CONTROL_SUPPORTED_ONLY(grcGfxContextControl *control=NULL));

#if __D3D11
	// PURPOSE: Reset lazy state tracking etc.
	grcEntry void ResetContext();
#endif

	// RETURNS: Pointer to a command list that encapsulates the commands generated between the last BeginContext call and now.
	// NOTES: Must be called from the worker thread
	grcEntry grcContextCommandList *EndContext();

	// PURPOSE: Dispatches a command list to the device
	// NOTES: Must be called from the thread that owns the device
	grcEntry void ExecuteCommandList(grcContextCommandList*);

	// PURPOSE: Call this on the main thread before calling ExecuteCommandList
	grcEntry void BeginCommandList();

	// PURPOSE: Call this on the main thread after calling ExecuteCommandList one or more times.
	grcEntry void EndCommandList();
#endif

#if RSG_DURANGO
	grcEntry void ForceSetContextState();
#endif
#if __D3D11
	static void ComputeMaximizedWindowDimensions(tagRECT &rect);
	static bool ComputeWindowDimensions(tagRECT &rect, unsigned int windowFlags);
#endif
protected:
	static void StoreBufferEnd(const void *bufferStart, int vertexCount, int vertexStride);
	static void VerifyBufferEnd(const void *bufferEnd);

#if RSG_PC || RSG_DURANGO
	static void StoreIBBufferEnd(const void *bufferStart, int vertexCount, int vertexStride);
	static void VerifyIBBufferEnd(const void *bufferEnd);
#endif // __WIN32PC

#if __XENON
	static int sm_ColorExpBias;
#endif

#if !__FINAL
	// This is where the buffer returned by BeginVertices() is expected to end
	static DECLARE_MTR_THREAD const void *sm_BeginVerticesBufferEnd;
	static DECLARE_MTR_THREAD const void *sm_BeginIndicesBufferEnd;
#endif // !__FINAL

	grcEntry bool AllocateSwapChain();
	grcEntry bool FreeSwapChain();

	static grcDisplayWindow sm_CurrentWindows[NUMBER_OF_RENDER_THREADS];
	static grcDisplayWindow sm_GlobalWindow;
#if __WIN32PC
	static grcDisplayWindow sm_FullscreenWindow;
	static grcDisplayWindow sm_DesiredWindow;
#endif


	static MonitorConfiguration sm_MonitorConfig;

	static bool sm_HardwareShaders, sm_HardwareTransform;
	static bool sm_LetterBox;

#if RSG_ORBIS
	static DECLARE_MTR_THREAD bool s_bScanMode_ViewportScissor;
#endif

#if RSG_ORBIS || __D3D11
	static u32  sm_ClipPlaneEnable[NUMBER_OF_RENDER_THREADS];
#if DEVICE_CLIP_PLANES
	static u32 sm_PreviousClipPlaneEnable[NUMBER_OF_RENDER_THREADS];
	static u32 sm_ClipPlanesChanged[NUMBER_OF_RENDER_THREADS];
	// World space clip planes.
	static Vec4V sm_ClipPlanes[RAGE_MAX_CLIPPLANES][NUMBER_OF_RENDER_THREADS];
	// Constant buffer containing clipping space planes.
#if RSG_ORBIS
	static grcCBuffer *sm_pClipPlanesConstBuffer[NUMBER_OF_RENDER_THREADS];
#else
	static grcCBuffer *sm_pClipPlanesConstBuffer;
#endif
#endif	//DEVICE_CLIP_PLANES

#else //RSG_ORBIS || __D3D11

	static u32  sm_ClipPlaneEnable;

#endif //RSG_ORBIS || __D3D11
	
	static u32  sm_FrameCounter;
	static u32  sm_SyncCounter;
	static MSAAMode sm_MSAA;

#if DEVICE_EQAA
	static bool sm_EQAA;
#else
	static u32 sm_MultisampleQuality;
#endif // DEVICE_EQAA

#if __D3D
	static struct sysIpcCurrentThreadId__* sm_Owner;
#if (RSG_PC || RSG_DURANGO)
	static sysIpcSema sm_Controller;
	static struct sysIpcCurrentThreadId__* sm_CreationOwner;
#endif //__WIN32PC
#endif	//__D3D

#if __PPU
	static int sm_LastWidth, sm_LastHeight;
	static bool sm_OutputIsFullRange;
#endif

#if RSG_PC || RSG_DURANGO
	static __THREAD int sm_ThreadId;

	static bool sm_AllowUpdateThreadBlock;
#endif
#if __WIN32PC

	static u32	sm_TextureResourcePool;
	static u32	sm_BufferResourcePool;

	static u32  sm_uFeatures;
	static u32  ms_MaxQueuedFrames;

	static bool	sm_UseNVidiaAPI;
	static bool sm_UseVendorAPI;
	static bool sm_Paused;
	static bool sm_LostFocus;
	static bool sm_LostFocusForAudio;
	static bool sm_Minimized;
	static bool sm_Maximized;
	static bool sm_Occluded;
	static bool sm_MinimizedWhileFullscreen;
	static bool sm_TopMostWhileWindowed;
	static bool sm_IgnoreSizeChange;
	static bool sm_Active;
	static bool sm_InSizeMove;
	static bool sm_bStereoChangeEnable;
	static bool sm_ReleasingSwapChain;
	static bool sm_Lost;
	static u32	sm_WindowStyle;
	static bool sm_InsideDeviceChange;
	static bool sm_DoNoStoreNewSize;
	static bool sm_ClipCursorWhenFullscreen;
	static bool sm_DeviceRestored;
	static bool sm_ForceWindowResize;
	static bool sm_FrameLockOverride;

	static bool sm_SwapChainFullscreen;

	static bool sm_MatchDesiredWindow;
	static bool sm_RecheckDeviceChanges;

#if __D3D11
	static bool sm_ChangeDeviceRequest;
	static bool sm_IsInPopup;
	static bool sm_ForceChangeDevice;
	static bool sm_ForceDeviceReset;
	static bool sm_RequireDeviceRestoreCallbacks;
	static bool sm_IgnoreMonitorWindowLimits;
#endif

#if USE_NV_TXAA
	static bool sm_TXAASupported;
#endif

#if RSG_PC && __D3D11
	static bool sm_DragResized;
	static bool sm_RecenterWindow;
	static bool sm_Borderless;
	static bool sm_DesireBorderless;
#endif

#if __D3D11
	static bool sm_BusyAltTabbing;
	static bool sm_IgnoreDeviceReset;
#endif
#if RSG_PC && __DEV
	static fiStream* sm_DeviceResetLoggingStream;
	static bool sm_DeviceResetTestActive;
#endif

	static unsigned long sm_WindowFlags;

	static bool sm_RequestPauseMenu;

	static bool sm_AllowPauseOnFocusLoss;
	static bool sm_DisablePauseOnFocusLossSystemOverride;
	static bool sm_VideoEncodingOverride;
	static bool sm_HeadBlendingOverride;

	static bool sm_BlockOnLostFocus;
	static bool sm_AllowFullscreenToggle;
	static bool sm_bIssuingReset;
	static u32  sm_uDeviceStatus;
	static grcSwapChain* sm_pSwapChain;

	static const int MAX_DEVICE_CALLBACKS = 32;
	static atFixedArray<Functor0, MAX_DEVICE_CALLBACKS> sm_DeviceLostCb;
	static atFixedArray<Functor0, MAX_DEVICE_CALLBACKS> sm_DeviceResetCb;
#if NV_SUPPORT
	static Functor1<float> sm_StereoSepChangeCb;	// device separation change
	static Functor1<float> sm_StereoConvChangeCb;	// device convergence change
#endif

	static FOCUSCALLBACK sm_FocusCallback;
	static RENDERCALLBACK sm_RenderCallback;


	static HWND__* CreateDeviceWindow(HWND__* parent);

	static u32 sm_DxFeatureLevel;
	static u32 sm_DxShaderModel[2];	// Major, minor

#elif RSG_DURANGO
	static grcSwapChain* sm_pSwapChain;
	static u32  sm_uDeviceStatus;
	static ID3D11DmaEngineContextX*	sm_DmaEngineContext1;
	static ID3D11DmaEngineContextX*	sm_DmaEngineContext2;
	static ServiceDelegate sm_Delegate;
	static volatile bool sm_HasFocus;
#endif
	static volatile u32 sm_KillSwitch;

#if __D3D
	static grcDeviceHandle *sm_Current;
#if __XENON
	static grcDeviceHandle *sm_Primary, *sm_Command;
#endif
	static grcVertexDeclaration *sm_BlitDecl;
	static grcVertexDeclaration *sm_ImDecl;
	grcEntry void UpdatePresentParameters();
# if __WIN32PC
	static int sm_AdapterOrdinal;
	static int sm_OutputMonitor;
# endif
#endif

#if __D3D11
	struct grcRenderTargetView
	{
		ID3D11RenderTargetView* pRTView;
		ID3D11Resource*			pResource; 
	};


	static __THREAD grcRenderTargetView		sm_aRTView[grcmrtColorCount];
	static __THREAD grcRenderTargetView		sm_DepthView;
	static __THREAD grcRenderTargetView		sm_PreviousDepthView;
	static __THREAD u32						sm_numTargets;
	static grcRenderTarget					*sm_pBackBuffer;
	static grcRenderTarget					*sm_pDepthStencil;

	static grcDepthStencilStateHandle		sm_WriteDepthNoStencil;
	static grcDepthStencilStateHandle		sm_WriteStencilNoDepth;
	static grcDepthStencilStateHandle		sm_WriteNoDepthNoStencil;
	static grcDepthStencilStateHandle		sm_WriteDepthAndStencil;

	static DECLARE_MTR_THREAD grcVertexDeclaration	*sm_CurrentVertexDeclaration;
	static DECLARE_MTR_THREAD grcVertexProgram		*sm_CurrentVertexProgram;
	static DECLARE_MTR_THREAD grcFragmentProgram		*sm_CurrentFragmentProgram;
	static DECLARE_MTR_THREAD grcComputeProgram		*sm_CurrentComputeProgram;
	static DECLARE_MTR_THREAD grcDomainProgram		*sm_CurrentDomainProgram;
	static DECLARE_MTR_THREAD grcGeometryProgram		*sm_CurrentGeometryProgram;
	static DECLARE_MTR_THREAD grcHullProgram			*sm_CurrentHullProgram;

#if !RSG_DURANGO
	static D3DVERTEXSTREAMINFO sm_aVertexStreamInfo[NUMBER_OF_RENDER_THREADS];
#endif
#endif // __D3D11

	static const char *sm_DefaultEffectName;

#if __WIN32PC
	static bool  sm_IgnoreStereoMsg;
	static int   sm_StereoDesired;
	static bool  sm_bStereoEnabled;
	static bool  sm_bCanUseStereo;
	static bool  sm_bStereoScissor;

	static void*		sm_pStereoHandle;
	static grcTexture* sm_StereoTex;

	static float sm_fStereoConvergence;
	static float sm_fEyeSeparation;
	static float sm_fDefaultStereoConvergence;	// this value is only adjusted by user (via commanline or UI option)
	static float sm_fStereoSeparationPercentage;

	static float sm_DesiredSeparation;
	static float sm_DesiredConvergence;

public:
	static float sm_AspectRatio;

	static float sm_maxAspectRatio;
	static float sm_minAspectRatio;
#endif
};

#if __D3D
inline grcVertexDeclaration* grcDevice::GetBlitDecl() {
	return sm_BlitDecl;
}

inline grcVertexDeclaration* grcDevice::GetImmediateModeDecl() {
	return sm_ImDecl;
}

inline grcDeviceHandle* grcDevice::GetCurrent() {
	return sm_Current;
}
#endif

#if RSG_DURANGO
inline ID3D11DmaEngineContextX*	grcDevice::GetDmaEngine1() {
	return sm_DmaEngineContext1;
}

inline ID3D11DmaEngineContextX*	grcDevice::GetDmaEngine2() {
	return sm_DmaEngineContext2;
}
#endif

#if __WIN32PC
inline bool grcDevice::GetLostFocus()
{
	return sm_LostFocus;
}

inline bool grcDevice::GetLostFocusForAudio()
{
	return sm_LostFocusForAudio;
}

inline void grcDevice::SetLostFocus(bool val)
{
	sm_LostFocus = val;

	if (sm_FocusCallback)
	{
		sm_FocusCallback();
	}
}

inline void grcDevice::SetLostFocusForAudio(bool val)
{
	sm_LostFocusForAudio = val;
}

inline void grcDevice::SetBlockOnLostFocus(bool val)
{
	sm_BlockOnLostFocus = val;
}

inline bool grcDevice::GetBlockOnLostFocus()
{
	return sm_BlockOnLostFocus;
}

inline void grcDevice::SetFocusCallback(FOCUSCALLBACK callback)
{
	sm_FocusCallback = callback;
}

inline RENDERCALLBACK grcDevice::GetRenderCallback()
{
	return sm_RenderCallback;
}

inline void grcDevice::SetRenderCallback(RENDERCALLBACK callback)
{
#if __FINAL
	if (sm_RenderCallback != NULL)
	{
		Warningf("Overriding Render callback");
	}
#endif

	sm_RenderCallback = callback;
}

inline bool grcDevice::IsFullscreenToggleEnable()
{
	return sm_AllowFullscreenToggle;
}

inline void grcDevice::SetFullscreenToggleEnable(bool val)
{
	sm_AllowFullscreenToggle = val;
}
#endif

#if __XENON
inline int grcDevice::GetColorExpBias()
{
	return sm_ColorExpBias;
}

inline void grcDevice::SetColorExpBias(int val)
{
	sm_ColorExpBias = val;
}


#endif

#if __PPU
// return full range status
inline bool grcDevice::IsOutputFullRange()
{
	return sm_OutputIsFullRange;
}

#endif // __PPU

inline int grcDevice::GetWidth() {
	return CheckThreadOwnership() ? sm_CurrentWindows[g_RenderThreadIndex].uWidth : sm_GlobalWindow.uWidth; 
}

inline int grcDevice::GetHeight() { 
	return CheckThreadOwnership() ? sm_CurrentWindows[g_RenderThreadIndex].uHeight : sm_GlobalWindow.uHeight; 
}

inline int grcDevice::GetRefreshRate() { 
	return sm_CurrentWindows[g_RenderThreadIndex].uRefreshRate; 
}

inline const grcDisplayWindow& grcDevice::GetGlobalWindow()	{
	return sm_GlobalWindow;
}

inline const MonitorConfiguration& grcDevice::GetMonitorConfig()	{
	return sm_MonitorConfig.update();
}

inline void grcDevice::SetSize(int width,int height) { 
	Assert(width > 0);
	Assert(height > 0);
	if (g_IsSubRenderThread) {
		sm_CurrentWindows[g_RenderThreadIndex].uWidth = width;
		sm_CurrentWindows[g_RenderThreadIndex].uHeight = height;
	}
	else {
		for (int i=0; i<NUMBER_OF_RENDER_THREADS; i++) {
			sm_CurrentWindows[i].uWidth = width;
			sm_CurrentWindows[i].uHeight = height;
		}
	}
}

inline void grcDevice::InitGlobalWindow()
{
	if (g_IsSubRenderThread)
		sm_GlobalWindow = sm_CurrentWindows[g_RenderThreadIndex];
	else
		sm_GlobalWindow = sm_CurrentWindows[0];
}

inline grcDevice::MSAAMode grcDevice::GetMSAA() {return sm_MSAA;}
inline void grcDevice::SetMSAA(MSAAMode i) {sm_MSAA = i;}

#if DEVICE_EQAA
inline bool grcDevice::IsEQAA() {return sm_EQAA;}
#else
inline u32 grcDevice::GetMSAAQuality() {return sm_MultisampleQuality;};
inline void grcDevice::SetMSAAQuality(u32 quality) {sm_MultisampleQuality = quality;};
#endif // DEVICE_EQAA

#if GRCDEVICE_IS_STATIC
extern grcDevice GRCDEVICE;
#else
extern grcDevice *g_pGRCDEVICE;
#define GRCDEVICE	(*g_pGRCDEVICE)
#endif

inline int grcWindow::GetX() const {
	const float roundingBias = 0.5f;
	return int(roundingBias + m_NormX*(float)GRCDEVICE.GetWidth());
}

inline int grcWindow::GetY() const {
	const float roundingBias = 0.5f;
	return int(roundingBias + m_NormY*(float)GRCDEVICE.GetHeight());
}

inline int grcWindow::GetWidth() const {
	const float roundingBias = 0.5f;
	return int(roundingBias + m_NormWidth*(float)GRCDEVICE.GetWidth());
}

inline int grcWindow::GetHeight() const {
	const float roundingBias = 0.5f;
	return int(roundingBias + m_NormHeight*(float)GRCDEVICE.GetHeight());
}

inline u32 grcDevice::GetClipPlaneEnable() {
#if RSG_ORBIS || __D3D11
	return sm_ClipPlaneEnable[g_RenderThreadIndex];
#else
	return sm_ClipPlaneEnable;
#endif
}

inline u32 grcDevice::GetFrameCounter() {
	return sm_FrameCounter;
}

// Value for z buffer at near clip plane
const float grcDepthClosest = 0.0f;

// Value for z buffer at far clip plane (usually want this when calling grcDevice::Clear)
const float grcDepthFarthest = 1.0f;

inline int grcDevice::GetVerticesMaximumBatchSize(grcDrawMode dm,u32 vertexSize)
{
	Assert( vertexSize > 0 && vertexSize < BEGIN_VERTICES_MAX_SIZE );
	static int translateToPrim[][2] = {
			{ 1, 0 },//drawPoints,			// Draw one or more single-pixel points
			{ 2, 0 },//drawLines,			// Draw one or more disconnected line segments
			{ 1, -1 },//drawLineStrip,
			{ 3, 0 },//drawTris,
			{ 1, -2 },//drawTriStrip,
			{ 1, -2 },//drawTriFan
			{ 4, 0 },//drawQuads,
			{ 4, 0 },//drawRects,	
			{ 6, 0 },//drawTrisAdj,
	};
	CompileTimeAssert(NELEM(translateToPrim) == drawModesTotal);
	
	int maxVerts = ( BEGIN_VERTICES_MAX_SIZE / vertexSize );
	int primitiveCount = (( maxVerts + translateToPrim[dm][1] ) /translateToPrim[dm][0]) - 1 ;
	return primitiveCount;
}

inline void grcDevice::EndVertices(const void *bufferEnd)
{
	EndCreateVertices(bufferEnd);
	DrawVertices(bufferEnd);
	ReleaseVertices(bufferEnd);
}

inline void grcDevice::EndVertices(u32 vertexCount)
{
	EndCreateVertices(vertexCount);
	DrawVertices(vertexCount);
	ReleaseVertices(vertexCount);
}

inline void grcDevice::EndIndexedVertices(u32 indexCount, u32 vertexCount)
{
	EndCreateIndexedVertices(indexCount, vertexCount);
	DrawIndexedVertices(indexCount, vertexCount);
	ReleaseIndexedVertices(indexCount, vertexCount);
}

#if RAGE_INSTANCED_TECH
inline void grcDevice::EndInstancedVertices(const void *bufferEnd)
{
	EndCreateInstancedVertices(bufferEnd);
	DrawInstancedVertices(bufferEnd);
	ReleaseInstancedVertices(bufferEnd);
}

#if RSG_ORBIS
inline void grcDevice::EndInstancedVertices(const void* bufferEnd, int numInst)
{
	EndCreateInstancedVertices(bufferEnd, numInst);
	DrawInstancedVertices(bufferEnd, numInst);
	ReleaseInstancedVertices(bufferEnd, numInst);
}
#endif // RSG_ORBIS
#endif // RAGE_INSTANCED_TECH

#if __D3D11 || RSG_ORBIS
inline void grcDevice::EndVertices(const void *bufferEnd, u32 noOfIndices, const grcIndexBuffer &indexBuffer)
{
	EndCreateVertices(bufferEnd, noOfIndices, indexBuffer);
	DrawVertices(bufferEnd, noOfIndices, indexBuffer);
	ReleaseVertices(bufferEnd, noOfIndices, indexBuffer);
}
#endif // __D3D11 || RSG_ORBIS

#if DEVICE_IM_INSTANCED_INDEXED
inline void grcDevice::EndIndexedInstancedVertices(const void *bufferEnd, u32 vertexSize, u32 noOfIndices, const grcVertexBuffer &vertexBuffer, const grcIndexBuffer &indexBuffer, u32 numInstances, u32 startIndex, u32 startVertex, u32 startInstance)
{
	EndCreateIndexedInstancedVertices(bufferEnd, vertexSize, noOfIndices, vertexBuffer, indexBuffer, numInstances, startIndex, startVertex, startInstance);
	DrawIndexedInstancedVertices(bufferEnd, vertexSize, noOfIndices, vertexBuffer, indexBuffer, numInstances, startIndex, startVertex, startInstance);
	ReleaseIndexedInstancedVertices(bufferEnd, vertexSize, noOfIndices, vertexBuffer, indexBuffer, numInstances, startIndex, startVertex, startInstance);
}
#endif // DEVICE_IM_INSTANCED_INDEXED

#if !__FINAL
inline void grcDevice::StoreBufferEnd(const void *bufferStart, int vertexCount, int vertexStride)
{
	Assert(sm_BeginVerticesBufferEnd == NULL);	// Called twice?

	if (bufferStart) {
		char *endBuffer = (char *) bufferStart;
		endBuffer += vertexStride * vertexCount;
		sm_BeginVerticesBufferEnd = endBuffer;
	}
}

inline void grcDevice::VerifyBufferEnd(const void * ASSERT_ONLY(bufferEnd))
{
	Assertf(sm_BeginVerticesBufferEnd != NULL, "EndVertices called without preceding BeginVertices");
	Assertf(bufferEnd == NULL || sm_BeginVerticesBufferEnd == bufferEnd, "Mismatch in vertex buffer! This may be a buffer overrun (positive number) or uninitialized polygons (negative number). Buffer difference is %d", (int) ((char *) bufferEnd - (char *) sm_BeginVerticesBufferEnd));
	sm_BeginVerticesBufferEnd = NULL;
}

#if RSG_PC || RSG_DURANGO
inline void grcDevice::StoreIBBufferEnd(const void *bufferStart, int indexCount, int indexStride)
{
	Assert(sm_BeginIndicesBufferEnd == NULL);	// Called twice?

	if (bufferStart) {
		char *endBuffer = (char *) bufferStart;
		endBuffer += indexStride * indexCount;
		sm_BeginIndicesBufferEnd = endBuffer;
	}
}

inline void grcDevice::VerifyIBBufferEnd(const void * ASSERT_ONLY(bufferEnd))
{
	Assertf(sm_BeginIndicesBufferEnd != NULL, "EndIndices called without preceding BeginIndices");
	Assertf(bufferEnd == NULL || sm_BeginIndicesBufferEnd == bufferEnd, "Mismatch in index buffer! This may be a buffer overrun (positive number) or uninitialized polygons (negative number). Buffer difference is %d", (int) ((char *) bufferEnd - (char *) sm_BeginIndicesBufferEnd));
	sm_BeginIndicesBufferEnd = NULL;
}
#endif // __WIN32PC

#else // !__FINAL
inline void grcDevice::StoreBufferEnd(const void * /*bufferStart*/, int /*vertexCount*/, int /*vertexStride*/) {}
inline void grcDevice::VerifyBufferEnd(const void * /*bufferEnd*/) {}

#if RSG_PC || RSG_DURANGO
inline void grcDevice::StoreIBBufferEnd(const void * /*bufferStart*/, int /*vertexCount*/, int /*vertexStride*/) {}
inline void grcDevice::VerifyIBBufferEnd(const void * /*bufferEnd*/) {}
#endif // __WIN32PC

#endif // !__FINAL

#if RSG_DURANGO
template<ShaderType SHADER_TYPE>
inline void grcDevice::SetConstantBuffer(u32 slot, const void *data)
{
	SetConstantBuffer<SHADER_TYPE>(g_grcCurrentContext, slot, data);
}
#endif // RSG_DURANGO

inline u32 grcDevice::MakeKillSwitch(u32 validData, u32 set)
{
	// Hard coded to assume a kill mask of 0x10101010
	Assert(s_KillSwitchMask == 0x10101010);
	Assert((validData & 0xffff0000) == 0);
	u32 isSet = (set << 4) | (set << 12) | (set << 20) | (set << 28);
	return isSet | (validData & 0x0f0f) | ((validData & 0xf0f0) << 12);
}

inline u32 grcDevice::ExtractKillSwitchData(u32 data)
{
	// Hard coded to assume a kill mask of 0x10101010
	Assert(s_KillSwitchMask == 0x10101010);
	return (data & 0x00000f0f) | ((data & 0x0f0f0000) >> 12);
}

}	// namespace rage

#endif
