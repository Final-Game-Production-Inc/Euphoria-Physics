
#include	"texturefactory_d3d11.h"

#include	"texture_durango.h"
#include	"rendertarget_durango.h"
#include	"texture_d3d11.h"
#include	"rendertarget_d3d11.h"

#include	"texturereference.h"
#include	"config.h"
#include	"resourcecache.h"
#include	"channel.h"
#include	"grcore/buffer_d3d11.h"
#include	"grcore/buffer_durango.h"
#include	"grcore/d3dwrapper.h"

#if	RSG_PC || RSG_DURANGO

#if RSG_DURANGO
#include	"buffer_durango.h"
#endif 

#include	"AA_shared.h"
#include	"dds.h"
#include	"device.h"
#include	"image.h"
#include	"viewport.h"

#include	"system/memory.h"
#include	"system/param.h"
#include "grprofile/pix.h"
#include	"diag/tracker.h"
#include	"string/string.h"
#include	"data/resource.h"
#include	"data/struct.h"
#include	"vector/matrix34.h"
#include	"system/param.h"
#include	"system/xtl.h"
#include	<wbemidl.h>

#include	<d3d9.h>
#include	"wrapper_d3d.h"

#if RSG_DURANGO
#include <xg.h>
#endif

// DOM-IGNORE-BEGIN 
#if RSG_PC && NV_SUPPORT
#include "../../../3rdParty/NVidia/nvapi.h"
#include "../../../3rdParty/NVidia/nvstereo.h"
#endif
// DOM-IGNORE-END

PARAM(usetypedformat,"[grcore] Use typed formats for some damned thing");
PARAM(noSRGB,"[gocore] Disable SRGB support");

#if __TOOL
namespace rage
{
	XPARAM(noquits);
}
#endif

#define ENFORCE_UNIQUE_TARGET_NAMES	0
#define	CREATE_DEVICE_DEPTH_STENCIL	0
#define CREATE_DEVICE_MSAA_SURFACES	(AA_BACK_BUFFER && DEVICE_MSAA)

namespace rage 
{

#define D3DFMT_NULL ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')))

extern GUID TextureBackPointerGuid;

/*======================================================================================================================================*/
/* class grcTextureFactoryDX11.																											*/
/*======================================================================================================================================*/

#if __D3D11

s32 grcTextureFactoryDX11::s_RenderTargetStackTop = -1; 
grcTextureFactoryDX11::TARGETINFO grcTextureFactoryDX11::s_RenderTargetStack[grcTextureFactoryDX11::RENDER_TARGET_STACK_SIZE];
grcDeviceTexture* grcTextureFactoryDX11::sm_BackBufferTexture = NULL;

#if DEBUG_SEALING_OF_DRAWLISTS
bool grcTextureFactoryDX11::s_DrawCallsBeenIssuedWithNoTargetsSet = false;
char grcTextureFactoryDX11::s_DrawListDebugString[256];
#endif // DEBUG_SEALING_OF_DRAWLISTS

#endif // __D3D11

#if __BANK && SRC_DST_TRACKING
grcTextureFactoryDX11::MultiMapTargetTextureSources grcTextureFactoryDX11::sm_mmapSourceTextureList;
sysCriticalSectionToken grcTextureFactoryDX11::sm_Lock;
#endif // __BANK

void grcTextureFactoryDX11::CreatePagedTextureFactory() {
	SetInstance(*(rage_new grcTextureFactoryDX11));
}


grcTextureFactoryDX11::grcTextureFactoryDX11() 
{
#if __D3D11
	s_RenderTargetStackTop = -1;
	memset( &s_RenderTargetStack, 0, sizeof(s_RenderTargetStack ) );

	// Create Rage version of back buffer and depth buffer so that I can access the view for them 
	// all the same way avoiding special cases
	Reset();
#endif // __D3D11
}

grcTextureFactoryDX11::~grcTextureFactoryDX11() 
{
#if __D3D11
	Lost();
#endif // __D3D11
}

#if __D3D11

void grcTextureFactoryDX11::Lost()
{
	if (grcDevice::sm_pBackBuffer != NULL)
	{
		grcDevice::sm_pBackBuffer->Release();
		grcDevice::sm_pBackBuffer = NULL;
	}

	LastSafeRelease(sm_BackBufferTexture);

	if (grcDevice::sm_pDepthStencil != NULL)
	{
		grcDevice::sm_pDepthStencil->Release();
		grcDevice::sm_pDepthStencil = NULL;
	}
	GRCDEVICE.SetRenderTargets(0, NULL, NULL );
}

void grcTextureFactoryDX11::Reset()
{
	ID3D11Texture2D* pBackBuffer = NULL;
#if CREATE_DEVICE_MSAA_SURFACES
	if (GRCDEVICE.GetMSAA())
	{
		// Create a MSAA target to act as the back buffer. Then we resolve into the real back buffer at frame end.
		grcTextureFactory::CreateParams params;
		params.Multisample = GRCDEVICE.GetMSAA();
# if !DEVICE_EQAA
		params.MultisampleQuality = (u8)GRCDEVICE.GetMSAAQuality();
# endif
		params.Format = grctfA8R8G8B8;
		grcDevice::sm_pBackBuffer = CreateRenderTarget(	"BackBuf0", grcrtPermanent, GRCDEVICE.GetWidth(), GRCDEVICE.GetHeight(), 32, &params);

		sm_BackBufferTexture = grcDevice::sm_pBackBuffer->GetTexturePtr();
	}else
#endif // CREATE_DEVICE_MSAA_SURFACES
	{
		if (((IDXGISwapChain*)GRCDEVICE.GetSwapChain())->GetBuffer(0, __uuidof( pBackBuffer ), reinterpret_cast<void**>(&pBackBuffer)) == S_OK)
		{
			grcDevice::sm_pBackBuffer = CreateRenderTarget("BackBuf0", (grcTextureObject*)pBackBuffer);

			sm_BackBufferTexture =(grcDeviceTexture*) pBackBuffer;
		}
	}

#if CREATE_DEVICE_DEPTH_STENCIL
	grcTextureFactory::CreateParams params;
# if CREATE_DEVICE_MSAA_SURFACES
	// Create the actual depth buffer MSAA as it's shared everywhere. May need two copies of this: one resolved, one not.
#  if DEVICE_EQAA
	params.Multisample = GRCDEVICE.GetMSAA();
#  else
	GRCDEVICE.GetMultiSample(*(u32*)&params.Multisample, *(u32*)&params.MultisampleQuality);
#  endif // DEVICE_EQAA
	if (params.Multisample)
		params.CreateAABuffer = true;
# endif // CREATE_DEVICE_MSAA_SURFACES
	grcDevice::sm_pDepthStencil = CreateRenderTarget("Depth", grcrtDepthBuffer, GRCDEVICE.GetWidth(), GRCDEVICE.GetHeight(), 32, &params);
#endif	//CREATE_DEVICE_DEPTH_STENCIL

#if DEVICE_RESOLVE_RT_CONFLICTS
	//Let's clear any SRV's in the cache that are already bound
	if(grcDevice::sm_pBackBuffer)
	{
		GRCDEVICE.NotifyTargetLocked(grcDevice::sm_pBackBuffer->GetTextureView());
	}
	if(grcDevice::sm_pDepthStencil)
	{
		GRCDEVICE.NotifyTargetLocked(grcDevice::sm_pDepthStencil->GetTextureView());
# if __D3D11
		GRCDEVICE.NotifyTargetLocked(static_cast<grcRenderTargetDX11*>(grcDevice::sm_pDepthStencil)->GetSecondaryTextureView());
# endif
	}
#endif //DEVICE_RESOLVE_RT_CONFLICTS

	const grcDeviceView *poRenderTargetView = ((grcRenderTargetD3D11*)grcDevice::sm_pBackBuffer)->GetTargetView();
	if (g_grcCurrentContext)
	{
		GRCDEVICE.SetRenderTargets(1, &poRenderTargetView, grcDevice::sm_pDepthStencil ?
			((grcRenderTargetD3D11*)grcDevice::sm_pDepthStencil)->GetTargetView() : NULL);
	}

	s_RenderTargetStackTop = -1;
	memset( &s_RenderTargetStack, 0, sizeof(s_RenderTargetStack ) );

	 for (u32 i = 0; i < grcmrtCount; ++i)
	 {
		 sm_DefaultRenderTargets[i].Resize(1);
		 sm_DefaultRenderTargets[i].End() = NULL;
	 }
	SetDefaultRenderTarget(grcmrtColor0, grcDevice::sm_pBackBuffer);
}

#if RSG_PC
grcTexture* grcTextureFactoryDX11::CreateStereoTexture()
{
	if (GRCDEVICE.CanUseStereo())
	{
#if NV_SUPPORT
		if (GRCDEVICE.GetManufacturer() == NVIDIA)
		{
			grcTextureFactory::TextureCreateParams param(	grcTextureFactory::TextureCreateParams::SYSTEM,
				grcTextureFactory::TextureCreateParams::LINEAR, grcsDiscard | grcsWrite);
			return Create(nv::stereo::D3D11Type::StereoTexWidth, nv::stereo::D3D11Type::StereoTexHeight, grctfA32B32G32R32F, NULL, 1 /*numMips*/, &param);
		}
#endif // NV_SUPPORT
	}
	return NULL;
}
#endif // RSG_PC

#endif // __D3D11

/*--------------------------------------------------------------------------------------------------*/
/* Texture format functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

#if __D3D11

bool grcTextureFactoryDX11::SupportsFormat(grcTextureFormat eFormat)
{
	u32 eD3D10Format = (grcTextureFactoryDX11::ConvertToD3DFormat((grcTextureFormat)eFormat));
	if (eD3D10Format == DXGI_FORMAT_UNKNOWN)
		return false;

	return true;
}


u32 grcTextureFactoryDX11::Translate(grcTextureFormat eFormat)
{
	return grcTextureFactoryDX11::ConvertToD3DFormat(eFormat);
}


u32	grcTextureFactoryDX11::ConvertToD3DFormat(grcTextureFormat eFormat)
{
	// NOTE! This lookup is mirrored inside grcRenderTargetDX11::Resolve() (for some reason, potentially resourcing?)
	// So any changes in values here need mirroring there too.
	static DXGI_FORMAT translate[] = 
	{
		DXGI_FORMAT_UNKNOWN,
		DXGI_FORMAT_B5G6R5_UNORM,
		DXGI_FORMAT_B8G8R8A8_UNORM, // grctfA8R8G8B8
		DXGI_FORMAT_R16_FLOAT,

		DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R11G11B10_FLOAT, // grctfA2B10G10R10 (PC TODO - No alpha may be trouble)
		DXGI_FORMAT_R10G10B10A2_UNORM, // grctfA2B10G10R10ATI
		DXGI_FORMAT_R16G16B16A16_FLOAT,

		DXGI_FORMAT_R16G16_UNORM,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,

		DXGI_FORMAT_R16G16B16A16_UNORM,
		DXGI_FORMAT_R8_UNORM,
		DXGI_FORMAT_B5G5R5A1_UNORM,
		DXGI_FORMAT_R8G8_UNORM,

		DXGI_FORMAT_R8G8_UNORM,
		DXGI_FORMAT_B5G5R5A1_UNORM,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT_B5G5R5A1_UNORM,

		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		DXGI_FORMAT_D16_UNORM,
		DXGI_FORMAT_R8G8_UNORM,

		DXGI_FORMAT_D32_FLOAT,
		DXGI_FORMAT_B8G8R8X8_UNORM, // grctfX8R8G8B8
		DXGI_FORMAT_UNKNOWN,  // PC TODO - No Need for a NULL format.  Color targets can be NULL for DX11.  Need to handle this everywhere
		DXGI_FORMAT_X24_TYPELESS_G8_UINT,
		DXGI_FORMAT_A8_UNORM,
		DXGI_FORMAT_R11G11B10_FLOAT,
		DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
		DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
		DXGI_FORMAT_BC1_UNORM,
		DXGI_FORMAT_BC2_UNORM,
		DXGI_FORMAT_BC3_UNORM,
		DXGI_FORMAT_BC4_UNORM,
		DXGI_FORMAT_BC5_UNORM,
		DXGI_FORMAT_BC6H_UF16,
		DXGI_FORMAT_BC7_UNORM,

		DXGI_FORMAT_R8G8B8A8_SNORM, // grctfA8B8G8R8_SNORM
		DXGI_FORMAT_R8G8B8A8_UNORM,  // grctfA8B8G8R8

#if RSG_DURANGO
		DXGI_FORMAT_NV12
#endif
	};
	CompileTimeAssert(NELEM(translate) == grctfCount);
	Assert(eFormat < grctfCount); 

	u32 uFormat = (u32)translate[eFormat];
	Assert(uFormat != DXGI_FORMAT_UNKNOWN);

#if RSG_PC
	if (GRCDEVICE.GetManufacturer() == INTEL)
	{
		if (uFormat == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
			uFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		else if (uFormat == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)
			uFormat = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
	}
#endif

	return uFormat;
}


u32 grcTextureFactoryDX11::TranslateDX9ToDX11Format(u32 uD3D9Format, bool bSRGB)
{
#if !__FINAL
	if (PARAM_noSRGB.Get())
	{
		bSRGB = false;
	}
#endif // !__FINAL

#if RSG_PC || RSG_DURANGO
	switch (uD3D9Format)
	{
	case DDS_D3DFMT_DXT1			: return (bSRGB) ? DXGI_FORMAT_BC1_UNORM_SRGB		: DXGI_FORMAT_BC1_UNORM;
	case DDS_D3DFMT_DXT2			: return (bSRGB) ? DXGI_FORMAT_BC2_UNORM_SRGB		: DXGI_FORMAT_BC2_UNORM; // DXT2 not really supported
	case DDS_D3DFMT_DXT3			: return (bSRGB) ? DXGI_FORMAT_BC2_UNORM_SRGB		: DXGI_FORMAT_BC2_UNORM;
	case DDS_D3DFMT_DXT4			: return (bSRGB) ? DXGI_FORMAT_BC3_UNORM_SRGB		: DXGI_FORMAT_BC3_UNORM; // DXT4 not really supported
	case DDS_D3DFMT_DXT5			: return (bSRGB) ? DXGI_FORMAT_BC3_UNORM_SRGB		: DXGI_FORMAT_BC3_UNORM;
	case DDS_D3DFMT_DXT5A			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_BC4_UNORM;
	case DDS_D3DFMT_DXN				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_BC5_UNORM;
	case DDS_D3DFMT_BC6				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_BC6H_UF16;
	case DDS_D3DFMT_BC7				: return (bSRGB) ? DXGI_FORMAT_BC7_UNORM_SRGB		: DXGI_FORMAT_BC7_UNORM;
	case DDS_D3DFMT_R8G8B8			: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_A8R8G8B8		: return (bSRGB) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB	: DXGI_FORMAT_B8G8R8A8_UNORM;
	case DDS_D3DFMT_X8R8G8B8		: return (bSRGB) ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB	: DXGI_FORMAT_B8G8R8X8_UNORM;
	case DDS_D3DFMT_R5G6B5			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_B5G6R5_UNORM;
	case DDS_D3DFMT_X1R5G5B5		: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_B5G5R5A1_UNORM; // X->A
	case DDS_D3DFMT_A1R5G5B5		: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_B5G5R5A1_UNORM;
	case DDS_D3DFMT_A4R4G4B4		: return											  DXGI_FORMAT_UNKNOWN; // note: there is a DXGI_FORMAT_B4G4R4A4_UNORM = 115, can we support this?
	case DDS_D3DFMT_R3G3B2			: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_A8				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_A8_UNORM;
	case DDS_D3DFMT_A8R3G3B2		: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_X4R4G4B4		: return											  DXGI_FORMAT_UNKNOWN; // note: there is a DXGI_FORMAT_B4G4R4A4_UNORM = 115, can we support this?
	case DDS_D3DFMT_A2B10G10R10		: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R10G10B10A2_UNORM;
	case DDS_D3DFMT_A8B8G8R8		: return (bSRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB	: DXGI_FORMAT_R8G8B8A8_UNORM;
	case DDS_D3DFMT_X8B8G8R8		: return (bSRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB	: DXGI_FORMAT_R8G8B8A8_UNORM; // X->A
	case DDS_D3DFMT_G32R32F			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R32G32_FLOAT;
	case DDS_D3DFMT_G16R16F			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R16G16_FLOAT;
	case DDS_D3DFMT_G16R16			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R16G16_UNORM;
	case DDS_D3DFMT_G8R8			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R8G8_UNORM; // custom
	case DDS_D3DFMT_R8				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R8_UNORM; // custom
	case DDS_D3DFMT_R16				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R16_UNORM; // custom
	case DDS_D3DFMT_R16F			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R16_FLOAT;
	case DDS_D3DFMT_A32B32G32R32F	: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R32G32B32A32_FLOAT;
	case DDS_D3DFMT_A2R10G10B10		: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R10G10B10A2_UNORM; // lies .. should be DXGI_FORMAT_B10G10R10A2_UNORM
	case DDS_D3DFMT_A16B16G16R16F	: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R16G16B16A16_FLOAT;
	case DDS_D3DFMT_A16B16G16R16	: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R16G16B16A16_UNORM;
	case DDS_D3DFMT_A8P8			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R8G8_UINT; // lies
	case DDS_D3DFMT_P8				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R8_UNORM; // lies
	case DDS_D3DFMT_L8				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R8_UNORM; // lies
	case DDS_D3DFMT_A8L8			: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_A4L4			: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_V8U8			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R8G8_UNORM; // lies
	case DDS_D3DFMT_L6V5U5			: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_X8L8V8U8		: return (bSRGB) ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB	: DXGI_FORMAT_B8G8R8X8_UNORM; // lies
	case DDS_D3DFMT_Q8W8V8U8		: return (bSRGB) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB	: DXGI_FORMAT_B8G8R8A8_UNORM; // lies
	case DDS_D3DFMT_V16U16			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R16G16_UNORM; // lies
	case DDS_D3DFMT_A2W10V10U10		: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R10G10B10A2_UNORM; // lies
	case DDS_D3DFMT_UYVY			: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_R8G8_B8G8		: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R8G8_B8G8_UNORM;
	case DDS_D3DFMT_YUY2			: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_G8R8_G8B8		: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_G8R8_G8B8_UNORM;
	case DDS_D3DFMT_D16_LOCKABLE	: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_D16_UNORM;
	case DDS_D3DFMT_D32				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_D32_FLOAT;
	case DDS_D3DFMT_D15S1			: return											  DXGI_FORMAT_UNKNOWN;
	case DDS_D3DFMT_D24S8			: return											  DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DDS_D3DFMT_D24X8			: return											  DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DDS_D3DFMT_D24X4S4			: return											  DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DDS_D3DFMT_D16				: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_D16_UNORM;
	case DDS_D3DFMT_D32F_LOCKABLE	: return											  DXGI_FORMAT_D32_FLOAT;
	case DDS_D3DFMT_D24FS8			: return											  DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DDS_D3DFMT_D32_LOCKABLE	: return											  DXGI_FORMAT_D32_FLOAT;
	case DDS_D3DFMT_R32F			: return (bSRGB) ? DXGI_FORMAT_UNKNOWN				: DXGI_FORMAT_R32_FLOAT;
	case DDS_D3DFMT_S8_LOCKABLE		: return											  DXGI_FORMAT_UNKNOWN;
	default:
		Assertf(0, "Unknown D3D9 Format %x", uD3D9Format);
		return DXGI_FORMAT_UNKNOWN;
	}
#else
	switch (uD3D9Format)
	{
	case 21: 
		return (bSRGB) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
	case 50:
		return (bSRGB) ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R8_UNORM;
	case '1TXD':         
		return (bSRGB) ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
	case '2TXD':
		return (bSRGB) ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
	case '3TXD':         
		return (bSRGB) ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
	case '4TXD':
		return (bSRGB) ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
	case '5TXD':
		return (bSRGB) ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
	default:
		Assertf(0, "Unhandled D3D9 Format %d", uD3D9Format);
		return DXGI_FORMAT_UNKNOWN;
	}
#endif
}


u32	grcTextureFactoryDX11::TranslateToTypelessFormat(u32 uDX11Format)
{
	switch (uDX11Format)
	{	
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return DXGI_FORMAT_R32G32B32A32_TYPELESS;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return DXGI_FORMAT_R32G32B32_TYPELESS;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
		return DXGI_FORMAT_R16G16B16A16_TYPELESS;

	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
		return DXGI_FORMAT_R32G32_TYPELESS;

	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		return DXGI_FORMAT_R32G8X24_TYPELESS; // I guess

	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
		return DXGI_FORMAT_R10G10B10A2_TYPELESS;

	case DXGI_FORMAT_R11G11B10_FLOAT:
		return DXGI_FORMAT_R11G11B10_FLOAT; // No typeless equivalent

	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;

	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;

	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
		return DXGI_FORMAT_R16G16_TYPELESS;

	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
		return DXGI_FORMAT_R32_TYPELESS;

	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		return DXGI_FORMAT_R24G8_TYPELESS;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
		return DXGI_FORMAT_R8G8_TYPELESS;

	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
		return DXGI_FORMAT_R16_TYPELESS;

	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return uDX11Format; // No typeless equivalent

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
		return DXGI_FORMAT_R8_TYPELESS;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		return DXGI_FORMAT_BC2_TYPELESS;

	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return DXGI_FORMAT_BC3_TYPELESS;

	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return DXGI_FORMAT_BC4_TYPELESS;

	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		return DXGI_FORMAT_BC5_TYPELESS;

	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
		return DXGI_FORMAT_BC6H_TYPELESS;

	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return DXGI_FORMAT_BC7_TYPELESS;

	case DXGI_FORMAT_R1_UNORM:
		return DXGI_FORMAT_R1_UNORM;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		return DXGI_FORMAT_BC1_TYPELESS;

	default:
		Assertf(0, "DX11: Unknown Texture Format %u", uDX11Format);
		return 0;
	}
}


u32	grcTextureFactoryDX11::TranslateToTextureFormat(u32 uDX11Format)
{
	switch (uDX11Format)
	{	
	case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case DXGI_FORMAT_R32G32B32_TYPELESS:    return DXGI_FORMAT_R32G32B32_FLOAT;
	case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_R32G32_FLOAT;
	case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return DXGI_FORMAT_R10G10B10A2_UNORM;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_R16G16_TYPELESS:       return DXGI_FORMAT_R16G16_FLOAT;
	case DXGI_FORMAT_R32_TYPELESS:          return DXGI_FORMAT_R32_FLOAT;
	case DXGI_FORMAT_R8G8_TYPELESS:         return DXGI_FORMAT_R8G8_UNORM;
	case DXGI_FORMAT_R16_TYPELESS:          return DXGI_FORMAT_R16_FLOAT;
	case DXGI_FORMAT_R8_TYPELESS:           return DXGI_FORMAT_R8_UNORM;
	case DXGI_FORMAT_BC1_TYPELESS:          return DXGI_FORMAT_BC1_UNORM;
	case DXGI_FORMAT_BC2_TYPELESS:          return DXGI_FORMAT_BC2_UNORM;
	case DXGI_FORMAT_BC3_TYPELESS:          return DXGI_FORMAT_BC3_UNORM;
	case DXGI_FORMAT_BC4_TYPELESS:          return DXGI_FORMAT_BC4_UNORM;
	case DXGI_FORMAT_BC5_TYPELESS:          return DXGI_FORMAT_BC5_UNORM;
	case DXGI_FORMAT_BC6H_TYPELESS:         return DXGI_FORMAT_BC6H_UF16;
	case DXGI_FORMAT_BC7_TYPELESS:          return DXGI_FORMAT_BC7_UNORM;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:  return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	case DXGI_FORMAT_D32_FLOAT:             return DXGI_FORMAT_R32_FLOAT;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:     return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return DXGI_FORMAT_B8G8R8A8_UNORM;

	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		return DXGI_FORMAT_B8G8R8X8_UNORM;

	case DXGI_FORMAT_D16_UNORM:
		return DXGI_FORMAT_R16_UNORM;

	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:	
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM: 
	case DXGI_FORMAT_B8G8R8X8_UNORM: 
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT: 
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
	case DXGI_FORMAT_R1_UNORM:
#if RSG_DURANGO
	case DXGI_FORMAT_NV12:
#endif
		return uDX11Format;

	default:
		Assertf(0, "DX11: Unknown Texture Format %u", uDX11Format);
		return 0;
	}
}

grcTextureFormat grcTextureFactoryDX11::TranslateToRageFormat(u32 uD3DFormat)
{
	switch (uD3DFormat)
	{	
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return grctfA32B32G32R32F;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return grctfA16B16G16R16F;

	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
		return grctfA16B16G16R16;

	case DXGI_FORMAT_R11G11B10_FLOAT: 
		return grctfR11G11B10F;

	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
		return grctfG32R32F;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
		return grctfA2B10G10R10ATI;

	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SINT:
		return grctfA8B8G8R8;

	case DXGI_FORMAT_R8G8B8A8_SNORM:
		return grctfA8B8G8R8_SNORM;

	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return grctfA8R8G8B8;

	case DXGI_FORMAT_D16_UNORM:
		return grctfD16;

	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
		return grctfG16R16F;

	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
		return grctfG16R16;

	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_R32_FLOAT:
		return grctfR32F;

	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
		return grctfD32FS8;

	case DXGI_FORMAT_X24_TYPELESS_G8_UINT: 
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		return grctfD24S8;

	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
		return grctfR16F;

	case DXGI_FORMAT_B5G6R5_UNORM:
		return grctfR5G6B5;

	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return grctfA1R5G5B5;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
		return grctfL8; // lies .. but we might be relying on this behaviour when creating rendertargets as "L8" format?

	case DXGI_FORMAT_D32_FLOAT:
		return grctfD32F;

	case DXGI_FORMAT_D24_UNORM_S8_UINT:
		return grctfD24S8;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		return grctfDXT1;
	case DXGI_FORMAT_BC2_TYPELESS:	
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		return grctfDXT3;
	case DXGI_FORMAT_BC3_TYPELESS:	
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return grctfDXT5;
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return grctfDXT5A;
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		return grctfDXN;
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
		return grctfBC6;
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return grctfBC7;
#if RSG_DURANGO
	case DXGI_FORMAT_NV12:
		return grctfNV12;
#endif
	default:
		Assertf(0, "DX11: No Rage Conversion for Texture Format %u", uD3DFormat);
		return grctfNone;
	}
}


u32 grcTextureFactoryDX11::GetD3DFormat(grcImage *image)
{	
	return GetD3DFromatFromGRCImageFormat(image->GetFormat());
}


u32 grcTextureFactoryDX11::GetD3DFromatFromGRCImageFormat(u32 grcImageFormat)
{
	DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
	switch (grcImageFormat)
	{
	case grcImage::UNKNOWN                     : fmt = DXGI_FORMAT_UNKNOWN           ; break;
	case grcImage::DXT1                        : fmt = DXGI_FORMAT_BC1_UNORM         ; break;
	case grcImage::DXT3                        : fmt = DXGI_FORMAT_BC2_UNORM         ; break;
	case grcImage::DXT5                        : fmt = DXGI_FORMAT_BC3_UNORM         ; break;
	case grcImage::CTX1                        : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT3A                       : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT3A_1111                  : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT5A                       : fmt = DXGI_FORMAT_BC4_UNORM         ; break;
	case grcImage::DXN                         : fmt = DXGI_FORMAT_BC5_UNORM         ; break;
	case grcImage::BC6                         : fmt = DXGI_FORMAT_BC6H_UF16         ; break;
	case grcImage::BC7                         : fmt = DXGI_FORMAT_BC7_UNORM         ; break;
	case grcImage::A8R8G8B8                    : fmt = DXGI_FORMAT_B8G8R8A8_UNORM    ; break;
	case grcImage::A8B8G8R8                    : fmt = DXGI_FORMAT_R8G8B8A8_UNORM    ; break;
	case grcImage::A8                          : fmt = DXGI_FORMAT_A8_UNORM          ; break;
	case grcImage::L8                          : fmt = DXGI_FORMAT_R8_UNORM          ; break; // lies .. actually not supported
	case grcImage::A8L8                        : fmt = DXGI_FORMAT_R8G8_UNORM        ; break; // lies .. actually not supported
	case grcImage::A4R4G4B4                    : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A1R5G5B5                    : fmt = DXGI_FORMAT_B5G5R5A1_UNORM    ; break; // D3D10-11: not supported D3D11.1: This value is not supported until Windows 8(untested on xbone)
	case grcImage::R5G6B5                      : fmt = DXGI_FORMAT_B5G6R5_UNORM      ; break; // D3D10-11: not supported D3D11.1: This value is not supported until Windows 8(works on xbone)
	case grcImage::R3G3B2                      : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A8R3G3B2                    : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A4L4                        : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A2R10G10B10                 : fmt = DXGI_FORMAT_R10G10B10A2_UNORM ; break; // lies .. actually not supported
	case grcImage::A2B10G10R10                 : fmt = DXGI_FORMAT_R10G10B10A2_UNORM ; break;
	case grcImage::A16B16G16R16                : fmt = DXGI_FORMAT_R16G16B16A16_UNORM; break;
	case grcImage::G16R16                      : fmt = DXGI_FORMAT_R16G16_UNORM      ; break;
	case grcImage::L16                         : fmt = DXGI_FORMAT_R16_UNORM         ; break; // lies .. actually not supported
	case grcImage::A16B16G16R16F               : fmt = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
	case grcImage::G16R16F                     : fmt = DXGI_FORMAT_R16G16_FLOAT      ; break;
	case grcImage::R16F                        : fmt = DXGI_FORMAT_R16_FLOAT         ; break;
	case grcImage::A32B32G32R32F               : fmt = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
	case grcImage::G32R32F                     : fmt = DXGI_FORMAT_R32G32_FLOAT      ; break;
	case grcImage::R32F                        : fmt = DXGI_FORMAT_R32_FLOAT         ; break;
	case grcImage::D15S1                       : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::D24S8                       : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::D24FS8                      : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::P4                          : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::P8                          : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A8P8                        : fmt = DXGI_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::R8                          : fmt = DXGI_FORMAT_R8_UNORM          ; break;
	case grcImage::R16                         : fmt = DXGI_FORMAT_R16_UNORM         ; break;
	case grcImage::G8R8                        : fmt = DXGI_FORMAT_R8G8_UNORM        ; break;
	case grcImage::LINA32B32G32R32F_DEPRECATED : fmt = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
	case grcImage::LINA8R8G8B8_DEPRECATED      : fmt = DXGI_FORMAT_R8G8B8A8_UNORM    ; break;
	case grcImage::LIN8_DEPRECATED             : fmt = DXGI_FORMAT_R8_UNORM          ; break; // lies .. actually not supported
	case grcImage::RGBE                        : fmt = DXGI_FORMAT_R8G8B8A8_UNORM    ; break;
	}

	Assertf(fmt != DXGI_FORMAT_UNKNOWN, "Unsupported Image Format (%d)", (u32)grcImageFormat);

	return fmt;
}


u32 grcTextureFactoryDX11::GetImageFormat(u32) 
{
	AssertMsg(0, "GetImageFormat should not be call via a DX11 interface - Tool should use DX9 path");
	return grcImage::UNKNOWN;
}

u32 grcTextureFactoryDX11::GetImageFormatStatic(u32 eFormat) 
{
	switch(eFormat)
	{
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
		return grcImage::DXT1;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
		return grcImage::DXT3;

	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
		return grcImage::DXT5;

	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return grcImage::DXT5A;

	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		return grcImage::DXN;

	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
		return grcImage::BC6;

	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return grcImage::BC7;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
		return grcImage::R8;

	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
		return grcImage::R16;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
		return grcImage::G8R8;

#if EFFECT_TRACK_COMPARISON_ERRORS
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return grcImage::UNKNOWN;
#endif //EFFECT_TRACK_COMPARISON_ERRORS

	default:
		switch (TranslateToRageFormat(eFormat))
		{
		case grctfR11G11B10F: return DXGI_FORMAT_R11G11B10_FLOAT; // TODO -- not supported for grcImage::Format, we should either support it properly or return grcImage::UNKNOWN
		case grctfA32B32G32R32F: return grcImage::A32B32G32R32F;
		case grctfA16B16G16R16F: return grcImage::A16B16G16R16F;
		case grctfA16B16G16R16: return grcImage::A16B16G16R16;
		case grctfA2B10G10R10ATI: return grcImage::A2B10G10R10;
		case grctfA8R8G8B8: return grcImage::A8R8G8B8;
		case grctfA8B8G8R8: return grcImage::A8B8G8R8;
		case grctfA8B8G8R8_SNORM: return grcImage::A8B8G8R8;
		case grctfD16: return grcImage::L16;
		case grctfG16R16F: return grcImage::G16R16F;
		case grctfG16R16: return grcImage::G16R16;
		case grctfG32R32F: return grcImage::G32R32F;
		case grctfR16F: return grcImage::R16F;
		case grctfR32F: return grcImage::R32F;
		case grctfD24S8: return grcImage::D24S8;
		case grctfR5G6B5: return grcImage::R5G6B5;
		case grctfA1R5G5B5: return grcImage::A1R5G5B5;
		case grctfL8: return grcImage::L8;
		case grctfD32F: return grcImage::R32F; // TODO -- not really supported for grcImage::Format, we should probably return grcImage::UNKNOWN
		default: Assertf(0, "DX11: Unknown DXGI Texture Format %u", eFormat);
		}
	}

	return grcImage::UNKNOWN;
}

u32	grcTextureFactoryDX11::GetBitsPerPixel(u32 uInternalFormat)
{
	return GetD3D10BitsPerPixel(uInternalFormat);
}

u32	grcTextureFactoryDX11::GetD3D10BitsPerPixel(u32 uInternalFormat)
{
	switch (uInternalFormat)
	{	
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
		return 64;

	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
		// According to AMD - Not sure if its the same for NVida but I hope so.
		return 40;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return 32;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return 16;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		return 8;

	case DXGI_FORMAT_R1_UNORM:
		return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 4;

	default:
		Assertf(0, "DX11: Unknown Texture Format %u", uInternalFormat);
		return 0;
	}
}

#endif // __D3D11

/*--------------------------------------------------------------------------------------------------*/
/* Texture creation functions.																		*/
/*--------------------------------------------------------------------------------------------------*/


grcTexture	*grcTextureFactoryDX11::Create(const char *pFilename,grcTextureFactory::TextureCreateParams *params)
{
	char	Buffer[256];

	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
	RAGE_TRACK_NAME(pFilename);
	RAGE_NEW_TRACK(Silly);

	sysMemUseMemoryBucket TEXTURES(grcTexture::sm_MemoryBucket);
	StringNormalize(Buffer, pFilename, sizeof(Buffer));

	grcTexture	*pTex = LookupTextureReference(Buffer);
	if (pTex)
		return pTex;

#if RSG_DURANGO || (__RESOURCECOMPILER && __64BIT) // Durango textures are the only ones built by ragebuilder through grcTextureFactoryDX11.
	pTex = rage_new grcTextureDurango(Buffer,params);
#elif __D3D11
	pTex = rage_new grcTextureDX11(Buffer,params);
#else
	(void)params;
	grcAssertf(0, "grcTextureFactoryDX11::Create()...Not implemented");
#endif
	return(pTex);
}

grcTexture *grcTextureFactoryDX11::Create(grcImage *pImage,grcTextureFactory::TextureCreateParams *params)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
#if RSG_DURANGO || (__RESOURCECOMPILER && __64BIT) // Durango textures are the only ones built by ragebuilder through grcTextureFactoryDX11.
	return rage_new grcTextureDurango(pImage,params);
#elif __D3D11
	return rage_new grcTextureDX11(pImage,params);
#else
	(void)pImage;
	(void)params;
	grcAssertf(0, "grcTextureFactoryDX11::Create()...Not implemented");
	return NULL;
#endif
}

grcTexture *grcTextureFactoryDX11::Create(u32 width, u32 height, u32 eFormat, void* pBuffer, u32 numMips, grcTextureFactory::TextureCreateParams *params)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
#if RSG_DURANGO || (__RESOURCECOMPILER && __64BIT) // Durango textures are the only ones built by ragebuilder through grcTextureFactoryDX11.
	return rage_new grcTextureDurango(width, height, 1, numMips, (grcTextureFormat)eFormat, pBuffer ,params);
#elif __D3D11
	(void)numMips;
	return rage_new grcTextureDX11(width, height, (grcTextureFormat)eFormat, pBuffer ,params);
#else
	(void)numMips;
	(void)(width+height+eFormat);
	(void)pBuffer;
	(void)params;
	grcAssertf(0, "grcTextureFactoryDX11::Create()...Not implemented");
	return NULL;
#endif
}


grcTexture *grcTextureFactoryDX11::Create(u32 width, u32 height, u32 depth, u32 eFormat, void* pBuffer, grcTextureFactory::TextureCreateParams *params)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
#if RSG_DURANGO || (__RESOURCECOMPILER && __64BIT) // Durango textures are the only ones built by ragebuilder through grcTextureFactoryDX11.
	return rage_new grcTextureDurango(width, height, depth, 1, (grcTextureFormat)eFormat, pBuffer ,params);
#elif __D3D11
	return rage_new grcTextureDX11(width, height, depth, (grcTextureFormat)eFormat, pBuffer ,params);
#else
	(void)(width+height+depth+eFormat);
	(void)pBuffer;
	(void)params;
	grcAssertf(0, "grcTextureFactoryDX11::Create()...Not implemented");
	return NULL;
#endif
}


#if DEVICE_EQAA
grcTexture *grcTextureFactoryDX11::CreateAsFmask(grcDeviceTexture *eqaaTexture, const grcDevice::MSAAMode &mode)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
	return rage_new grcTextureDurango(eqaaTexture, mode);
}
#endif // DEVICE_EQAA


/*--------------------------------------------------------------------------------------------------*/
/* Render target functions.																			*/
/*--------------------------------------------------------------------------------------------------*/

#if __DEV
fiStream* grcTextureFactoryDX11::pLoggingStream = NULL;
#endif

#if __D3D11

grcRenderTarget	*grcTextureFactoryDX11::CreateRenderTarget(const char * pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params, grcRenderTarget* originalTarget)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

#if __DEV
	if (pLoggingStream)
	{
		char theText[200];
		formatf(theText, 200, "%s width: %d height: %d bits: %d multisample: %d\r\n", pName, nWidth, nHeight, nBitsPerPixel, (params ? (params->Multisample ? params->Multisample : 1) : 1));
		pLoggingStream->Write(theText, (int)strlen(theText));
	}
#endif

#if !RSG_DURANGO	// on DURANGO, we use the origTarget as a memory source
	if (originalTarget)
	{
		((grcRenderTargetD3D11*)originalTarget)->DestroyInternalData();
		((grcRenderTargetD3D11*)originalTarget)->ReCreate(eType, nWidth, nHeight, nBitsPerPixel, params);
		return originalTarget;
	}
#endif

#if ENFORCE_UNIQUE_TARGET_NAMES && RSG_PC
	// Render Targets must have unique names using this code.  This was to eliminate leaks cause by bad code on recreation
	{
		for (int i = 0; i < sm_ActiveTargets.GetCount(); ++i) {
			grcRenderTargetPC *tgt = sm_ActiveTargets[i];
			if(stricmp(tgt->GetName(), pName) == 0)
			{
				// this render target already exists, recreate it with our new parameters
				// this happens when the device is lost and reset
				tgt->ReCreate(eType,nWidth,nHeight,nBitsPerPixel,params);
				return tgt;
			}
		}
	}
#endif // ENFORCE_UNIQUE_TARGET_NAMES && RSG_PC

	grcRenderTargetD3D11 *tgt = rage_new grcRenderTargetD3D11(pName,eType,nWidth,nHeight,nBitsPerPixel,params
#if RSG_DURANGO 
																											 , (grcRenderTargetDurango *)originalTarget
#endif
		);

#if RSG_PC

#if !__FINAL
	if (sm_ActiveTargets.GetCount() == MAX_RENDERTARGETS)
	{
		for (int i = 0; i < sm_ActiveTargets.GetCount(); i++)
			grcDisplayf("target: %s", sm_ActiveTargets[i]->GetName());
	}
#endif
	tgt->SetSlotId(sm_ActiveTargets.GetCount());
	sm_ActiveTargets.Append() = tgt;
#endif // RSG_DURANGO

	RegisterRenderTarget(tgt);

	return tgt;
}

grcRenderTarget* grcTextureFactoryDX11::CreateRenderTarget(const char *pName, const grcTextureObject *pTexture, grcRenderTarget* originalTarget)
{
	return CreateRenderTarget( pName, pTexture, grctfNone, DepthStencilRW, originalTarget);
}

grcRenderTarget* grcTextureFactoryDX11::CreateRenderTarget(const char *pName, const grcTextureObject *pTexture, grcTextureFormat textureFormat, DepthStencilFlags depthStencilReadOnly, grcRenderTarget* originalTarget, rage::grcTextureFactory::CreateParams *_params)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

	if (originalTarget)
	{
		((grcRenderTargetD3D11*)originalTarget)->DestroyInternalData();
		if( textureFormat != grctfNone )
			((grcRenderTargetD3D11*)originalTarget)->CreateFromTextureObject(pName,pTexture, textureFormat, depthStencilReadOnly);
		else
			((grcRenderTargetD3D11*)originalTarget)->CreateFromTextureObject(pName,pTexture, depthStencilReadOnly);
		return originalTarget;
	}

#if ENFORCE_UNIQUE_TARGET_NAMES
	// Render Targets must have unique names using this code.  This was to eliminate leaks cause by bad code on recreation
	{
		for (int i = 0; i < sm_ActiveTargets.GetCount(); ++i) {
			grcRenderTargetPC *tgt = sm_ActiveTargets[i];
			if(stricmp(tgt->GetName(), pName) == 0)
			{
				Assertf(0, "Creating Render Target that already exists - Either delete and let rage recreate %s", pName);
			}
		}
	}
#endif

	grcRenderTargetD3D11 *tgt = NULL;
	
	if( textureFormat != grctfNone )
		tgt = rage_new grcRenderTargetD3D11(pName,pTexture, textureFormat, depthStencilReadOnly,_params);
	else
		tgt = rage_new grcRenderTargetD3D11(pName,pTexture, depthStencilReadOnly,_params);

#if RSG_PC
	tgt->SetSlotId(sm_ActiveTargets.GetCount());
	sm_ActiveTargets.Append() = tgt;
#endif // RSG_PC

	RegisterRenderTarget(tgt);

	return tgt;
}

void grcTextureFactoryDX11::LockRenderTarget(int index, const grcRenderTarget *target, const grcRenderTarget * depth, u32 layer, bool lockDepth, u32 mipToLock)
{
	(void)lockDepth;
	Assert(target != NULL || depth != NULL);

#if ENABLE_PIX_TAGGING
	if( target && !depth )
		PIXBeginN(0x1, "LockRenderTarget - %s", target->GetName() );
	else if( !target && depth )
		PIXBeginN(0x1, "LockRenderTarget (depth) - %s", depth->GetName() );
	else
		PIXBeginN( 0x1, "LockRenderTarget - %s : %s", target->GetName(), depth->GetName() );
#endif //ENABLE_PIX_TAGGING

#if !__ASSERT
	(void) index;	// keep the compiler happy
#endif
	AssertMsg( index == 0, "On DX11 use LockMRT() instead. LockRenderTarget will only work when just locking the 0th target" );
	const grcRenderTarget *Targets[grcmrtColorCount];
	u32 layersToLock[grcmrtColorCount];
	u32 mipsToLock[grcmrtColorCount];

	for( u32 i=0; i<grcmrtColorCount; i++ )
	{
		Targets[i] = NULL;
		layersToLock[i] = 0;
		mipsToLock[i] = 0;
	}

	Targets[0] = target;
	layersToLock[0] = layer;
	mipsToLock[0] = mipToLock;

	if (target)
		((grcRenderTargetD3D11*)target)->Update();

	if (depth)
		((grcRenderTargetD3D11*)depth)->Update();

	LockMRTInternal( Targets, depth, layersToLock, mipsToLock );

	PIX_TAGGING_ONLY( PIXEnd() );
}

void grcTextureFactoryDX11::UnlockRenderTarget(int index,const grcResolveFlags * poResolveFlags)
{
	PIX_TAGGING_ONLY( PIXBegin(0x1, "UnlockRenderTarget") );

#if !__ASSERT
	(void) index;	// keep the compiler happy
#endif
	AssertMsg((index == 0), "Must use UnLockMRT if locking multiple render targets for DX11");
	
	grcResolveFlagsMrt ResolveFlags;
	((const grcResolveFlags *&)ResolveFlags[0]) = poResolveFlags;
	for (int i = 1; i < grcmrtColorCount; i++)
		ResolveFlags[i] = NULL;
	UnlockMRTInternal( &ResolveFlags );

	// NOTE:- These multiple events produce better formatting in PIX.
	PIX_TAGGING_ONLY( PIXEnd() );
}


void grcTextureFactoryDX11::LockMRT( const grcRenderTarget *color[grcmrtColorCount], const grcRenderTarget *depth, const u32* mipsToLock ) 
{
#if ENABLE_PIX_TAGGING
	PIXBegin( 0x1, "LockMRT" );
	for( u32 i=0; i<grcmrtColorCount; i++)
	{
		if( color[i] == NULL )
			break;
		else
			((grcRenderTargetD3D11*)color[i])->Update();

		PIXBeginN(0x1, "%d) %s", i, color[i]->GetName()); PIXEnd();
	}
	if( depth )
	{
		((grcRenderTargetD3D11*)depth)->Update();
		PIXBeginN(0x1, "depth : %s", depth->GetName()); PIXEnd();
	}
#endif //ENABLE_PIX_TAGGING

	LockMRTInternal( color, depth, NULL, mipsToLock );

	PIX_TAGGING_ONLY( PIXEnd() );
}

void grcTextureFactoryDX11::LockMRTWithUAV( const grcRenderTarget *color[grcmrtColorCount], const grcRenderTarget *depth, const u32* layersToLock, const u32* mipsToLock, const grcBufferUAV* UAVs[grcMaxUAVViews], const u32* pUAVInitialCounts) 
{
#if ENABLE_PIX_TAGGING
	PIXBegin( 0x1, "LockMRT" );
	for( u32 i=0; i<grcmrtColorCount; i++)
	{
		if( color[i] == NULL )
			break;
		else
		{
			((grcRenderTargetD3D11*)color[i])->Update();
		}
		PIXBeginN(0x1, "%d) %s", i, color[i]->GetName()); PIXEnd();
	}
	if( depth )
	{
		PIXBeginN(0x1, "depth : %s", depth->GetName()); PIXEnd();
	}
#endif //ENABLE_PIX_TAGGING

	//work out the number of rtviews for start offset for UAV's
	u32 numRTViews = 0;
	for( u32 i=0; i<grcmrtColorCount; i++)
	{
		if( color[i] == NULL )
			break;
		else
		{
			numRTViews++;
		}
	}

	TARGETINFO *pStackTop = NULL;

	SetupMRTStack( color, depth, layersToLock, mipsToLock, pStackTop);

	grcDeviceView*	UnorderedAccessViews[grcMaxUAVViews];
	u32 numUAVS = 0;
	for( u32 i=0; i<grcMaxUAVViews; i++)
	{
		if( UAVs[i] != NULL )
		{
			UnorderedAccessViews[i] = UAVs[i]->GetUnorderedAccessView();
#if DEVICE_RESOLVE_RT_CONFLICTS
			//Let's clear any SRV's in the cache that are already bound
			GRCDEVICE.NotifyTargetLocked(UAVs[i]->GetShaderResourceView());
#endif //DEVICE_RESOLVE_RT_CONFLICTS
			numUAVS = i+1;
		}
		else
			UnorderedAccessViews[i] = NULL;
	}

	u32 UAVStartSlot = numRTViews;

	// Set the device size.
	GRCDEVICE.SetSize( pStackTop->uWidth, pStackTop->uHeight );
	// Set the render targets.
	GRCDEVICE.SetRenderTargetsWithUAV( pStackTop->uNumViews, &pStackTop->apColorViews[0], (grcDeviceView*)pStackTop->pDepthView, UAVStartSlot, numUAVS, UnorderedAccessViews, pUAVInitialCounts );

	PIX_TAGGING_ONLY( PIXEnd() );
}

void grcTextureFactoryDX11::LockDepthAndCurrentColorTarget( const grcRenderTarget *depth )
{
	TARGETINFO *pStackPrev = &s_RenderTargetStack[s_RenderTargetStackTop];
	s_RenderTargetStackTop++;
	TARGETINFO *pStackTop = &s_RenderTargetStack[s_RenderTargetStackTop];

	Assertf( pStackPrev->uWidth == (u32)depth->GetWidth() && pStackPrev->uHeight == (u32)depth->GetHeight(), "depth needs to be the dimensions as the active colour target (prev=%dx%d, depth=%dx%d)", pStackPrev->uWidth, pStackPrev->uHeight, depth->GetWidth(), depth->GetHeight());
	ASSERT_ONLY( grcDevice::MSAAMode depthMSAAMode = depth->GetMSAA() );

	//Set the colour targets to be the same as the previous
	for( u32 i=0; i<pStackPrev->uNumViews; i++ )
	{
		if( pStackPrev->apColorTargets[i] != NULL )
		{
			pStackTop->apColorTargets[i] = pStackPrev->apColorTargets[i];
			pStackTop->apColorViews[i] = pStackPrev->apColorViews[i];			
			Assertf(pStackTop->apColorTargets[i]->GetMSAA() == depthMSAAMode, "Cannot mix and match depth (%s) and (%s) colour sample counts", depth->GetName(), pStackTop->apColorTargets[i]->GetName() );
		}
	}
	pStackTop->uNumViews = pStackPrev->uNumViews;

	//Setup the new depth buffer
	pStackTop->pDepthTarget = static_cast<const grcRenderTargetD3D11*>( depth );
	grcRenderTargetD3D11 *const depth11 = const_cast<grcRenderTargetD3D11*>( pStackTop->pDepthTarget );
	pStackTop->pDepthView = depth11->GetTargetView();

	pStackTop->uWidth = pStackPrev->uWidth;
	pStackTop->uHeight = pStackPrev->uHeight;
#if DEVICE_RESOLVE_RT_CONFLICTS
	//Let's clear any SRV's in the cache that are already bound
	if(pStackTop->pDepthTarget)
	{
		GRCDEVICE.NotifyTargetLocked(depth11->GetTextureView());
		GRCDEVICE.NotifyTargetLocked(depth11->GetSecondaryTextureView());
	}
#endif //DEVICE_RESOLVE_RT_CONFLICTS
	GRCDEVICE.SetRenderTargets( pStackTop->uNumViews, &pStackTop->apColorViews[0], (grcDeviceView*)pStackTop->pDepthView );
}

void grcTextureFactoryDX11::UnlockDepthAndCurrentColorTarget(const grcResolveFlags * poResolveFlags)
{
	grcResolveFlagsMrt ResolveFlags;
	for(u32 i = 0; i < grcmrtColorCount; i++)
		((const grcResolveFlags *&)ResolveFlags[i]) = poResolveFlags;
	UnlockMRTInternal( &ResolveFlags );
}

void grcTextureFactoryDX11::UnlockMRT( const grcResolveFlagsMrt *pResolveFlags ) 
{
	PIX_TAGGING_ONLY( PIXBegin(0x1, "UnlockMRT") );

	UnlockMRTInternal( pResolveFlags );

	PIX_TAGGING_ONLY( PIXEnd() );
}


void grcTextureFactoryDX11::LockMRTInternal( const grcRenderTarget *color[grcmrtColorCount],const grcRenderTarget *depth, const u32 *layersToLock, const u32 *mipsToLock )
{
	TARGETINFO *pStackTop = NULL;
#if RSG_ASSERT
	if ((depth != NULL) && (color[0] != NULL))
	{
		#if DEVICE_EQAA
			// If EQAA is supported, then only the number of fragments matters 
			u32 nColorFragments = color[0]->GetMSAA().m_uFragments;
			u32 nDepthFragments = depth->GetMSAA().m_uFragments; 
			nColorFragments = nColorFragments>1 ? nColorFragments : 1;
			nDepthFragments = nDepthFragments>1 ? nDepthFragments : 1;
		#else
			u32 nColorFragments = color[0]->GetMSAA();
			u32 nDepthFragments = depth->GetMSAA();
		#endif 

		//Note: on Orbis only the number of fragments is required to match. D3D11 though needs the samples to match as well.
		Assertf((nDepthFragments == nColorFragments),"Depth MSAA Level %d does not match Color MSAA Level %d", nDepthFragments, nColorFragments);
		Assertf((depth->GetWidth() == color[0]->GetWidth()), "Depth Width %d does not match Color Width %d", depth->GetWidth(), color[0]->GetWidth());
		Assertf((depth->GetHeight() == color[0]->GetHeight()), "Depth Height %d does not match Color Height %d", depth->GetHeight(), color[0]->GetHeight());
	}
#endif // __ASSERT

	SetupMRTStack( color, depth, layersToLock, mipsToLock, pStackTop);
	// Set the device size.
	GRCDEVICE.SetSize( pStackTop->uWidth, pStackTop->uHeight );
#if DEVICE_EQAA && 0
	const grcDevice::MSAAMode mode = (color[0] ? color[0] : depth)->GetMSAA();
	GRCDEVICE.SetAACount(mode.m_uSamples, mode.m_uFragments, mode.m_uFragments);
#endif
	// Set the render targets.
	GRCDEVICE.SetRenderTargets( pStackTop->uNumViews, &pStackTop->apColorViews[0], (grcDeviceView*)pStackTop->pDepthView );
}


void grcTextureFactoryDX11::SetupMRTStack( const grcRenderTarget *color[grcmrtColorCount],const grcRenderTarget *depth, const u32 *layersToLock, const u32 *mipsToLock, TARGETINFO*& pStackTop )
{
	u32 i;

	// Check stack bounds.
	Assert( ( s_RenderTargetStackTop < RENDER_TARGET_STACK_SIZE - 1 ) && "grcTextureFactoryDX11::LockMRT()...Out of stack space" );

	// Increment the "stack" top.
	s_RenderTargetStackTop++;
	pStackTop = &s_RenderTargetStack[s_RenderTargetStackTop];

	pStackTop->uNumViews = 0;

	for( i=0; i<grcmrtColorCount; i++ )
	{
		pStackTop->apColorTargets[i] = NULL; 
		pStackTop->apColorViews[i] = NULL;
	}

	// Record the render target details.
	for( i=0; i<grcmrtColorCount; i++ )
	{
		if( color[i] != NULL )
		{
			u32 mipLevel = mipsToLock ? mipsToLock[i] : 0;
			u32 arraySlice = layersToLock ? layersToLock[i] : 0;
			pStackTop->apColorTargets[i] = static_cast < const grcRenderTargetD3D11 * > ( color[i] );
#if	DYNAMIC_ESRAM
			const grcRenderTargetDurango * pTarget = static_cast < const grcRenderTargetDurango * > ( color[i] );
			if (pTarget->GetUseAltTarget())
			{
				pStackTop->apColorTargets[i] = static_cast < const grcRenderTargetD3D11 * > (pTarget->GetAltTarget());
			}
#endif			
			const_cast<grcRenderTargetD3D11*>(pStackTop->apColorTargets[i])->DebugSetUnresolved();
			pStackTop->apColorViews[i] = ((grcRenderTargetD3D11*)pStackTop->apColorTargets[i])->GetTargetView(mipLevel,arraySlice);

#if DEVICE_RESOLVE_RT_CONFLICTS
			//Let's clear any SRV's in the cache that are already bound
			GRCDEVICE.NotifyTargetLocked(const_cast<grcRenderTargetD3D11*>(pStackTop->apColorTargets[i])->GetTextureView());
#endif
			//Num views is the last one in the list, if NULLs in between they need to be counted as views so the last valid one is set.
			pStackTop->uNumViews=i+1;
		}
	}

	// Record the depth-stencil details.
	pStackTop->pDepthTarget = static_cast < const grcRenderTargetD3D11 * >( depth );
	pStackTop->pDepthView = NULL;

	if( pStackTop->pDepthTarget )
	{
		grcRenderTargetD3D11 *const depth11 = const_cast<grcRenderTargetD3D11*>(pStackTop->pDepthTarget);
		depth11->DebugSetUnresolved();
		u32 mipLevel = mipsToLock ? mipsToLock[0] : 0;
		u32 arraySlice = layersToLock ? layersToLock[0] : 0;
		pStackTop->pDepthView = depth11->GetTargetView(mipLevel, arraySlice);

#if DEVICE_RESOLVE_RT_CONFLICTS
		//Let's clear any SRV's in the cache that are already bound
		GRCDEVICE.NotifyTargetLocked(depth11->GetTextureView());
		GRCDEVICE.NotifyTargetLocked(depth11->GetSecondaryTextureView());
#endif //DEVICE_RESOLVE_RT_CONFLICTS
	}

	if ( pStackTop->uNumViews )
	{	
		pStackTop->uWidth = pStackTop->apColorTargets[0]->GetWidth();
		pStackTop->uHeight = pStackTop->apColorTargets[0]->GetHeight();

		//If we're not using the top mip, recalculate the width and height using the first targets mip level
		if( mipsToLock && mipsToLock[0] != 0 )
		{
			pStackTop->uWidth = pStackTop->uWidth >> mipsToLock[0];
			pStackTop->uHeight = pStackTop->uHeight >> mipsToLock[0];
		}
	}
	else 
	{
		pStackTop->uWidth = depth->GetWidth();
		pStackTop->uHeight = depth->GetHeight();
	}
}


void grcTextureFactoryDX11::UnlockMRTInternal(  const grcResolveFlagsMrt *pResolveFlags )
{
	u32 i;
	Assert( ( s_RenderTargetStackTop >= 0 ) && "grcTextureFactoryDX11::UnlockMRT()...Stack under flow." );

	// Collect the stack top (we may need to resolve these targets).
	TARGETINFO *pToResolve = &s_RenderTargetStack[s_RenderTargetStackTop];

	// Decrement the "stack" top.
	--s_RenderTargetStackTop;

	// Are we examining a "live" part of the stack ?
	if( s_RenderTargetStackTop >= 0 )
	{
		TARGETINFO *pStackTop = &s_RenderTargetStack[s_RenderTargetStackTop];
		// Reinstate the previous size...
		GRCDEVICE.SetSize( pStackTop->uWidth, pStackTop->uHeight );
		//...And render targets etc.

#if DEVICE_RESOLVE_RT_CONFLICTS
		//Let's clear any SRV's in the cache that are already bound
		for(u32 i=0; i<pStackTop->uNumViews; i++)
		{
			if(pStackTop->apColorTargets[i])
			{
				GRCDEVICE.NotifyTargetLocked(const_cast<grcRenderTargetD3D11*>(pStackTop->apColorTargets[i])->GetTextureView());
			}
		}
		if(pStackTop->pDepthTarget)
		{
			GRCDEVICE.NotifyTargetLocked(const_cast<grcRenderTargetD3D11*>(pStackTop->pDepthTarget)->GetTextureView());
			GRCDEVICE.NotifyTargetLocked(const_cast<grcRenderTargetD3D11*>(pStackTop->pDepthTarget)->GetSecondaryTextureView());
		}
#endif //DEVICE_RESOLVE_RT_CONFLICTS
		GRCDEVICE.SetRenderTargets( pStackTop->uNumViews, &pStackTop->apColorViews[0], (grcDeviceView*)pStackTop->pDepthView);
	}
	else
	{
		//If there's no targets left of the stack set the size back to the backbuffer size
		GRCDEVICE.SetSize( grcDevice::sm_pBackBuffer->GetWidth(), grcDevice::sm_pBackBuffer->GetHeight() );
		// Hack to recover from the over popping of the stack
		s_RenderTargetStackTop = -1;
	}

	for( i=0; i<pToResolve->uNumViews; i++ )
	{
		grcRenderTargetD3D11 *target = (grcRenderTargetD3D11 *)pToResolve->apColorTargets[i];
		if( target && target->m_MipLevels > 1 && target->m_pShaderResourceView && (!pResolveFlags || !(*pResolveFlags)[i] || (*pResolveFlags)[i]->MipMap))
			g_grcCurrentContext->GenerateMips( (ID3D11ShaderResourceView*) target->m_pShaderResourceView );
	}
}

void grcTextureFactoryDX11::SetArrayView(u32 uArrayIndex)
{
	// we are only locking 0 render target index with 0 mip level for now
	// TODO: implemente for other rt index/mip level if needed

	TARGETINFO* pStackTop = &s_RenderTargetStack[s_RenderTargetStackTop];
	const grcDeviceView *colorView = NULL;
	const grcDeviceView *depthView = NULL;
	if (pStackTop->apColorTargets[0])
		colorView = ((grcRenderTargetD3D11*)pStackTop->apColorTargets[0])->GetTargetView(0,uArrayIndex);

	if (pStackTop->pDepthTarget)
		depthView = ((grcRenderTargetD3D11*)pStackTop->pDepthTarget)->GetTargetView(0, uArrayIndex);

	GRCDEVICE.SetRenderTargets( 1, &colorView, (grcDeviceView*)depthView );
}

bool grcTextureFactoryDX11::IsTargetLocked(const grcRenderTarget *target) const
{
	TARGETINFO &stackTop = s_RenderTargetStack[s_RenderTargetStackTop];
	for( u32 i=0; i<=stackTop.uNumViews; i++ )
	{
		const grcRenderTargetD3D11 *const locked = i ? stackTop.apColorTargets[i-1] : stackTop.pDepthTarget;
		if (locked == target)
			return true;
	}
	return false;
}


void grcTextureFactoryDX11::PlaceTexture(class datResource &rsc,grcTexture &tex) {
	switch (tex.GetResourceType()) {
		case grcTexture::NORMAL: 
			{
			#if RSG_DURANGO
				::new (&tex) grcTextureDurango(rsc); 
			#else // RSG_DURANGO
				::new (&tex) grcTextureDX11(rsc); 
			#endif // RSG_DURANGO
				break;
			}
		case grcTexture::RENDERTARGET: Assert(0 && "unsafe to reference a rendertarget"); break;
		case grcTexture::REFERENCE: ::new (&tex) grcTextureReference(rsc); break;
		default:
			{
				Printf("Bad resource type %d in grcTextureFactoryPC::PlaceTexture",tex.GetResourceType());
				Quitf(ERR_SYS_INVALIDRESOURCE_2,"Invalid resource detected - Please re-install the game.");
			}	
	}
}

u32		grcTextureFactoryDX11::GetTextureDataSize	(u32 width, u32 height, u32 format, u32 mipLevels, u32 numSlices, bool /*bIsCubeMap*/, bool /*bIsLinear*/, bool /*bLocalMemory*/)
{
#if RSG_DURANGO
	D3D11_TEXTURE2D_DESC oDesc = {0};
	oDesc.ArraySize = numSlices+1;
	oDesc.Format = static_cast<DXGI_FORMAT>(GetD3DFromatFromGRCImageFormat(format));
	oDesc.Width = width;
	oDesc.Height = height;
	oDesc.MipLevels = mipLevels;
	oDesc.SampleDesc.Count = 1;
	oDesc.SampleDesc.Quality = 0;
	oDesc.Usage = (D3D11_USAGE)grcUsageDefault;
	oDesc.MiscFlags = (mipLevels > 1 ? grcResourceGenerateMips : grcResourceNone);
	oDesc.CPUAccessFlags = grcCPUNoAccess;
	oDesc.BindFlags = grcBindShaderResource;

	int sizeInBytes = 0;

	XG_RESOURCE_LAYOUT colorLayout;
	XG_TILE_MODE tileMode = XG_TILE_MODE_LINEAR;

	XG_TEXTURE2D_DESC xgDesc;
	*((D3D11_TEXTURE2D_DESC*)&xgDesc) = oDesc;
	tileMode = XGComputeOptimalTileMode(XG_RESOURCE_DIMENSION_TEXTURE2D, xgDesc.Format, xgDesc.Width, xgDesc.Height, xgDesc.ArraySize, xgDesc.SampleDesc.Count, xgDesc.BindFlags);
	Assert(tileMode != XG_TILE_MODE_INVALID);

	xgDesc.TileMode = tileMode;
	xgDesc.Pitch = 0;

	grcDevice::Result uReturnCode;
	XGTextureAddressComputer* pComputer = NULL;
	uReturnCode = XGCreateTexture2DComputer(&xgDesc, &pComputer);
	if (!Verifyf((uReturnCode == 0), "Error %x occurred creating texture computer", uReturnCode))
		return 0U;

	uReturnCode = pComputer->GetResourceLayout(&colorLayout);
	if (!Verifyf((uReturnCode == 0), "Error %x occurred creating resource layout", uReturnCode))
		return 0U;
	
	sizeInBytes = (int)colorLayout.SizeBytes;
	pComputer->Release();

	return (u32)sizeInBytes;
#else
	// TODO -- this code does not consider cubemaps, volumes or arrays .. does it matter?
	int w           = width;
	int h           = height;
	int mips        = mipLevels;
	int layers      = numSlices+1;
	grcImage::Format imgFormat	= (grcImage::Format)format;
	int bpp         = grcImage::GetFormatBitsPerPixel(imgFormat);
	int blockSize   = grcImage::IsFormatDXTBlockCompressed(imgFormat) ? 4 : 1;
	int sizeInBytes = 0;

	while (mips > 0)
	{
		sizeInBytes += (w*h*layers*bpp)/8;

		w = Max<int>(blockSize, w/2);
		h = Max<int>(blockSize, h/2);

		mips--;
	}

	return (u32)sizeInBytes;
#endif
}

#endif // __D3D11

/*--------------------------------------------------------------------------------------------------*/
/* EQAA functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

#if DEVICE_EQAA
void grcTextureFactoryDX11::FinishRendering()
{
	//EQAA TODO
}

void grcTextureFactoryDX11::EnableMultisample(grcRenderTarget *target)
{
	static_cast<grcRenderTargetDurango*>(target)->m_Multisample.m_bEnabled = true;
	static_cast<grcRenderTargetDurango*>(target)->m_Coverage.compressionEnabled = true;
}
#endif // DEVICE_EQAA

/*--------------------------------------------------------------------------------------------------*/
/* Front/Back buffer functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

const grcRenderTarget *grcTextureFactoryDX11::GetBackBuffer(bool realize) const {
	return GetBackBuffer(realize);
}

grcRenderTarget *grcTextureFactoryDX11::GetBackBuffer(bool realize) 
{
#if __D3D11
	if (realize)
	{
		// MSAA: TODO: Check for MSAA target first. If present, resolve first.
	}
	return grcDevice::sm_pBackBuffer;
#else // __D3D11
	(void)realize;
	return grcTextureFactory::GetNotImplementedRenderTarget();
#endif // __D3D11
}

const grcRenderTarget *grcTextureFactoryDX11::GetFrontBuffer(bool nextBuffer) const {
	return GetFrontBuffer(nextBuffer);
}

grcRenderTarget *grcTextureFactoryDX11::GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	AssertMsg( false, "GetFrontBuffer - Not implemented on DX11\n");
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryDX11::GetFrontBufferDepth(bool realize) const {
	return GetFrontBufferDepth(realize);
}

grcRenderTarget *grcTextureFactoryDX11::GetFrontBufferDepth(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	AssertMsg( false, "GetFrontBufferDepth - Not implemented on DX11\n");
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryDX11::GetBackBufferDepth(bool realize) const {
	return GetBackBufferDepth(realize);
}

grcRenderTarget *grcTextureFactoryDX11::GetBackBufferDepth(bool realize) 
{
#if __D3D11
	if (realize)
	{
		// MSAA: TODO: Check for MSAA target first. If present, resolve first.
	}

	return grcDevice::sm_pDepthStencil;
#else // __D3D11
	(void)realize;
	return grcTextureFactory::GetNotImplementedRenderTarget();
#endif // __D3D11
}

#if __D3D11

grcRenderTargetD3D11 *grcTextureFactoryDX11::GetActiveDepthBuffer() {
	TARGETINFO *pStackTop = &s_RenderTargetStack[s_RenderTargetStackTop];
	return const_cast<grcRenderTargetD3D11 *>(pStackTop->pDepthTarget);

}

#endif // __D3D11


#if DEBUG_SEALING_OF_DRAWLISTS

void grcTextureFactoryDX11::SetDrawListDebugString(const char *pStr)
{
	strcpy_s(s_DrawListDebugString, 255, pStr);
	s_DrawCallsBeenIssuedWithNoTargetsSet = false;
}

char *grcTextureFactoryDX11::GetDrawListDebugString()
{
	return s_DrawListDebugString;
}


bool grcTextureFactoryDX11::AreAnyRenderTargetsSet()
{
	if(s_RenderTargetStackTop < 0)
	{
		return false;
	}
	return true;
}


bool grcTextureFactoryDX11::HaveAnyDrawsBeenIssuedWithNoTargetsSet()
{
	return s_DrawCallsBeenIssuedWithNoTargetsSet;
}


void grcTextureFactoryDX11::OutputSetRenderTargets()
{
	s32 i;

	for(i=1; i<=s_RenderTargetStackTop; i++)
	{
		u32 j;
		TARGETINFO *pStackEntry = &s_RenderTargetStack[i];

		for(j=0; j<pStackEntry->uNumViews; j++)
		{
			grcDisplayf("grcTextureFactoryDX11::OutputSetRenderTargets()...%d) %d:%s", i, j, pStackEntry->apColorTargets[j]->GetName());
		}
		if(pStackEntry->pDepthTarget)
		{
			grcDisplayf("grcTextureFactoryDX11::OutputSetRenderTargets()...%d) Depth:%s", i, pStackEntry->pDepthTarget->GetName());
		}
		else
		{
			grcDisplayf("grcTextureFactoryDX11::OutputSetRenderTargets()...%d) Depth:None", i);
		}
	}
}


void grcTextureFactoryDX11::OnDraw()
{
	if(s_RenderTargetStackTop < 0)
	{
		s_DrawCallsBeenIssuedWithNoTargetsSet = true;
	}
}

#endif // DEBUG_SEALING_OF_DRAWLISTS

}	// namespace rage

#endif // RSG_PC || RSG_DURANGO
