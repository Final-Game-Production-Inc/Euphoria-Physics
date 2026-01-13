//
// grcore/texture_d3d9.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include	"texture_d3d9.h"
#include	"texturereference.h"
#include	"config.h"
#include	"resourcecache.h"
#include	"channel.h"

#if	__WIN32PC && __D3D9

#include	"device.h"
#include	"dds.h"
#include	"image.h"
#include	"viewport.h"

#include	"system/memory.h"
#include	"system/param.h"
#include	"diag/tracker.h"
#include	"string/string.h"
#include	"data/resource.h"
#include	"data/struct.h"
#include	"vector/matrix34.h"
#include	"system/param.h"
#include	"system/xtl.h"
#include	<wbemidl.h>

#include	"system/d3d9.h"
#include	"wrapper_d3d.h"

// DOM-IGNORE-BEGIN 
#include "../../../3rdParty/NVidia/nvapi.h"
// DOM-IGNORE-END

#if __TOOL
namespace rage
{
	XPARAM(noquits);
}
#endif

#define ENFORCE_UNIQUE_TARGET_NAMES	0

#define STALL_DETECTION	0
#if STALL_DETECTION
#include "system/timer.h"
#endif

#define SURFACE IDirect3DSurface9
#define TEXTURE IDirect3DBaseTexture9

#if __64BIT && __RESOURCECOMPILER
#pragma comment(lib,"xg.lib")
// TEMP:- Work around until project generation is fixed (see B*1582860).
#include "../../../3rdparty/durango/xg.h"
#endif //__64BIT && __RESOURCECOMPILER

namespace rage {

extern GUID TextureBackPointerGuid;

#define D3DFMT_NULL ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')))

#if __64BIT && __RESOURCECOMPILER

static XG_FORMAT XGFormatFromGRCImageFormat(u32 grcImageFormat)
{
	XG_FORMAT fmt = XG_FORMAT_UNKNOWN;
	switch (grcImageFormat)
	{
	case grcImage::UNKNOWN                     : fmt = XG_FORMAT_UNKNOWN           ; break;
	case grcImage::DXT1                        : fmt = XG_FORMAT_BC1_UNORM         ; break;
	case grcImage::DXT3                        : fmt = XG_FORMAT_BC2_UNORM         ; break;
	case grcImage::DXT5                        : fmt = XG_FORMAT_BC3_UNORM         ; break;
	case grcImage::CTX1                        : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT3A                       : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT3A_1111                  : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT5A                       : fmt = XG_FORMAT_BC4_UNORM         ; break;
	case grcImage::DXN                         : fmt = XG_FORMAT_BC5_UNORM         ; break;
	case grcImage::BC6                         : fmt = XG_FORMAT_BC6H_UF16         ; break;
	case grcImage::BC7                         : fmt = XG_FORMAT_BC7_UNORM         ; break;
	case grcImage::A8R8G8B8                    : fmt = XG_FORMAT_B8G8R8A8_UNORM    ; break;
	case grcImage::A8B8G8R8                    : fmt = XG_FORMAT_R8G8B8A8_UNORM    ; break;
	case grcImage::A8                          : fmt = XG_FORMAT_A8_UNORM          ; break;
	case grcImage::L8                          : fmt = XG_FORMAT_R8_UNORM          ; break;
	case grcImage::A8L8                        : fmt = XG_FORMAT_R8G8_UNORM        ; break;
	case grcImage::A4R4G4B4                    : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A1R5G5B5                    : fmt = XG_FORMAT_B5G5R5A1_UNORM    ; break;
	case grcImage::R5G6B5                      : fmt = XG_FORMAT_B5G6R5_UNORM      ; break;
	case grcImage::R3G3B2                      : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A8R3G3B2                    : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A4L4                        : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A2R10G10B10                 : fmt = XG_FORMAT_R10G10B10A2_UNORM ; break;
	case grcImage::A2B10G10R10                 : fmt = XG_FORMAT_R10G10B10A2_UNORM ; break;
	case grcImage::A16B16G16R16                : fmt = XG_FORMAT_R16G16B16A16_UNORM; break;
	case grcImage::G16R16                      : fmt = XG_FORMAT_R16G16_UNORM      ; break;
	case grcImage::L16                         : fmt = XG_FORMAT_R16_UNORM         ; break;
	case grcImage::A16B16G16R16F               : fmt = XG_FORMAT_R16G16B16A16_FLOAT; break;
	case grcImage::G16R16F                     : fmt = XG_FORMAT_R16G16_FLOAT      ; break;
	case grcImage::R16F                        : fmt = XG_FORMAT_R16_FLOAT         ; break;
	case grcImage::A32B32G32R32F               : fmt = XG_FORMAT_R32G32B32A32_FLOAT; break;
	case grcImage::G32R32F                     : fmt = XG_FORMAT_R32G32_FLOAT      ; break;
	case grcImage::R32F                        : fmt = XG_FORMAT_R32_FLOAT         ; break;
	case grcImage::D15S1                       : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::D24S8                       : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::D24FS8                      : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::P4                          : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::P8                          : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A8P8                        : fmt = XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::LINA32B32G32R32F_DEPRECATED : fmt = XG_FORMAT_R32G32B32A32_FLOAT; break;
	case grcImage::LINA8R8G8B8_DEPRECATED      : fmt = XG_FORMAT_R8G8B8A8_UNORM    ; break;
	case grcImage::LIN8_DEPRECATED             : fmt = XG_FORMAT_R8_UNORM          ; break;
	case grcImage::RGBE                        : fmt = XG_FORMAT_R8G8B8A8_UNORM    ; break;
	}

	Assertf(fmt != DXGI_FORMAT_UNKNOWN, "Unsupported Image Format (%d)", (u32)grcImageFormat);

	return fmt;
}


int ComputeOrbisDurangoPaddedSizeAndAlignment(int width, int height, int depth, int noOfMips, int arraySize, grcImage::Format format, grcImage::ImageType type, int &alignmentOut)
{
	size_t size = 0;
	size_t align = 0;
	XG_RESOURCE_LAYOUT layout;
	XG_FORMAT fmt = XGFormatFromGRCImageFormat(format);

	if(type != grcImage::VOLUME)
	{
		XG_TEXTURE2D_DESC outDesc;

		outDesc.Width = width; outDesc.Height = height; outDesc.ArraySize = arraySize;
		outDesc.MipLevels = noOfMips; outDesc.Format = fmt; 
		outDesc.SampleDesc.Count = 1; outDesc.SampleDesc.Quality = 0;
		outDesc.Usage = XG_USAGE_DEFAULT;
		outDesc.BindFlags = XG_BIND_SHADER_RESOURCE;
		outDesc.CPUAccessFlags = 0;
		outDesc.MiscFlags = 0;
		outDesc.ESRAMOffsetBytes = 0;
#if defined(_XDK_VER) && _XDK_VER >= 10542
		outDesc.ESRAMUsageBytes = 0;
#endif
		outDesc.Pitch = 0;
		outDesc.TileMode = XGComputeOptimalTileMode(XG_RESOURCE_DIMENSION_TEXTURE2D, fmt, width, height, depth, 1, XG_BIND_SHADER_RESOURCE);

		// Compute Durango layout.
		XGComputeTexture2DLayout(&outDesc, &layout);

		size = layout.SizeBytes;
		align = layout.BaseAlignmentBytes;

		// Compute Orbis layout(Orbis just uses XG_TILE_MODE_1D_THIN).
		outDesc.TileMode = XG_TILE_MODE_1D_THIN;
		XGComputeTexture2DLayout(&outDesc, &layout);

		// Maintain the maximum.
		if(layout.SizeBytes > size)
			size = layout.SizeBytes;
		if(layout.BaseAlignmentBytes > align)
			align = layout.BaseAlignmentBytes;
	}
	else
	{
		XG_TEXTURE3D_DESC outDesc;

		outDesc.Width = width; outDesc.Height = height; outDesc.Depth = depth;
		outDesc.MipLevels = noOfMips; outDesc.Format = fmt; 
		outDesc.Usage = XG_USAGE_DEFAULT;
		outDesc.BindFlags = XG_BIND_SHADER_RESOURCE;
		outDesc.CPUAccessFlags = 0;
		outDesc.MiscFlags = 0;
		outDesc.ESRAMOffsetBytes = 0;
#if defined(_XDK_VER) && _XDK_VER >= 10542
		outDesc.ESRAMUsageBytes = 0;
#endif
		outDesc.Pitch = 0;
		outDesc.TileMode = XGComputeOptimalTileMode(XG_RESOURCE_DIMENSION_TEXTURE3D, fmt, width, height, depth, 1, XG_BIND_SHADER_RESOURCE);

		// Compute Durango layout.
		XGComputeTexture3DLayout(&outDesc, &layout);

		size = layout.SizeBytes;
		align = layout.BaseAlignmentBytes;

		// Compute Orbis layout(use XG_TILE_MODE_LINEAR for now).
		outDesc.TileMode = XG_TILE_MODE_LINEAR;
		XGComputeTexture3DLayout(&outDesc, &layout);

		// Maintain the maximum.
		if(layout.SizeBytes > size)
			size = layout.SizeBytes;
		if(layout.BaseAlignmentBytes > align)
			align = layout.BaseAlignmentBytes;

	}
	alignmentOut = align;
	return size;
}

#endif //__64BIT && __RESOURCECOMPILER

//=============================================================================
// Class Methods
//=============================================================================

//=============================================================================
// grcTextureFactoryDX9
//=============================================================================
grcTextureFactoryDX9::grcTextureFactoryDX9() : m_PreviousDepthLock(false), m_PreviousDepth(0), m_CurTexDepthSurface(0), m_DepthTarget(0), m_PreviousWidth(0), m_PreviousHeight(0) {
	for (int i = 0; i < MAX_RENDER_TARGETS; ++i) {
		m_PreviousTargets[i] = 0;
	}

	if (GRCDEVICE.GetCurrent()) {
		D3DCAPS9 caps;
		if (FAILED(GRCDEVICE.GetCurrent()->GetDeviceCaps(&caps)))
			Quitf("Unable to retrieve device caps");
		else
			sm_MaxAnisotropy=caps.MaxAnisotropy;
	}
}

grcTextureFactoryDX9::~grcTextureFactoryDX9() {
}

grcTexture	*grcTextureFactoryDX9::Create(const char *pFilename,grcTextureFactory::TextureCreateParams *params)
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

	pTex = rage_new grcTextureDX9(Buffer,params);
	return(pTex);
}

grcTexture *grcTextureFactoryDX9::Create(grcImage *pImage,grcTextureFactory::TextureCreateParams *params)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
	return rage_new grcTextureDX9(pImage,params);
}

grcTexture *grcTextureFactoryDX9::Create(u32 width, u32 height, u32 eFormat, void* pBuffer, u32 UNUSED_PARAM(numMips), grcTextureFactory::TextureCreateParams *params)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(Texture);
	return rage_new grcTextureDX9(width, height, (grcTextureFormat)eFormat, pBuffer ,params);
}


bool grcTextureFactoryDX9::SupportsFormat(grcTextureFormat eFormat)
{
	// PC TODO - Move this to device

	D3DFORMAT eD3DFormat = static_cast<D3DFORMAT>(Translate(eFormat));

	bool bResult = false;

	IDirect3D9* pDevice;

	if (FAILED(GRCDEVICE.GetCurrent()->GetDirect3D(&pDevice)))
	{
		Printf("Unable to retrieve D3D Device");
		Quitf("D3D Error - Unable to retrieve D3D Device. Please re-boot your system");
		// return 0;
	}

	if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
												D3DDEVTYPE_HAL,
												D3DFMT_X8R8G8B8,
												D3DUSAGE_RENDERTARGET,
												D3DRTYPE_TEXTURE,
												eD3DFormat)))
	{
		bResult = true;
	}

	pDevice->Release();

	return bResult;
}


u32 grcTextureFactoryDX9::Translate(grcTextureFormat eFormat)
{
	// grcTextureFormat_REFERENCE
	static DDS_D3DFORMAT translate[] = 
	{
		DDS_D3DFMT_UNKNOWN,
		DDS_D3DFMT_R5G6B5,
		DDS_D3DFMT_A8R8G8B8,
		DDS_D3DFMT_R16F,
		DDS_D3DFMT_R32F,
		DDS_D3DFMT_A2R10G10B10,
		DDS_D3DFMT_A2R10G10B10,
		DDS_D3DFMT_A16B16G16R16F,
		DDS_D3DFMT_G16R16,
		DDS_D3DFMT_G16R16F,
		DDS_D3DFMT_A32B32G32R32F,
		DDS_D3DFMT_A16B16G16R16F,
		DDS_D3DFMT_A16B16G16R16,
		DDS_D3DFMT_L8,
		DDS_D3DFMT_L16,
		DDS_D3DFMT_A8L8,
		DDS_D3DFMT_A8L8,
		DDS_D3DFMT_A1R5G5B5,
		DDS_D3DFMT_D24S8,
		DDS_D3DFMT_A4R4G4B4,
		DDS_D3DFMT_G32R32F,
		DDS_D3DFMT_D24FS8, // Not readable
		DDS_D3DFMT_D16,					// ps3-specific
		DDS_D3DFMT_A8L8,				// ps3-specific
		DDS_D3DFMT_D32F_LOCKABLE,
		DDS_D3DFMT_X8R8G8B8,
		DDS_D3DFMT_R16F,			// grctfNULL
		DDS_D3DFMT_UNKNOWN,			// DX11 only format X24G8
		DDS_D3DFMT_A8,
		DDS_D3DFMT_UNKNOWN,			// DX11 only format R11G11B10
		DDS_D3DFMT_UNKNOWN,			// DX11 only format D32FS8
		DDS_D3DFMT_UNKNOWN,			// DX11 only format X32S8
		DDS_D3DFMT_UNKNOWN,			// DX11 DXT1
		DDS_D3DFMT_UNKNOWN,			// DX11 DXT3
		DDS_D3DFMT_UNKNOWN,			// DX11 DXT5
		DDS_D3DFMT_UNKNOWN,			// DX11 DXT5A
		DDS_D3DFMT_UNKNOWN,			// DX11 DXN
		DDS_D3DFMT_UNKNOWN,			// DX11 BC6
		DDS_D3DFMT_UNKNOWN,			// DX11 BC7
		DDS_D3DFMT_Q8W8V8U8,		// RGBA SNORM
		DDS_D3DFMT_A8B8G8R8,		// RGBA
	};
	CompileTimeAssert(NELEM(translate) == grctfCount);

	Assert(eFormat < grctfCount); 

	u32 uFormat = (u32)translate[eFormat];

	if (eFormat == grctfNULL)
	{
		IDirect3D9* pDevice;

		if (FAILED(GRCDEVICE.GetCurrent()->GetDirect3D(&pDevice)))
		{
			Printf("Unable to retrieve D3D Device");
			Quitf("D3D Error - Unable to retrieve D3D Device. Please re-boot your system");
			// return 0;
		}

		D3DFORMAT eD3DFormat = D3DFMT_NULL;

		// Check for NULL texture format (Nvidia specific). Otherwise, return default.
		if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
													D3DDEVTYPE_HAL,
													D3DFMT_X8R8G8B8,
													D3DUSAGE_RENDERTARGET,
													D3DRTYPE_TEXTURE,
													eD3DFormat ) ) )
		{
			uFormat = (u32)eD3DFormat;
		}

		pDevice->Release();
	}

	return uFormat;
}

u32 grcTextureFactoryDX9::GetD3DFormat(grcImage *image)
{
	DDS_D3DFORMAT fmt = DDS_D3DFMT_UNKNOWN;

	switch (image->GetFormat())
	{
	case grcImage::UNKNOWN                     : fmt = DDS_D3DFMT_UNKNOWN      ; break;
	case grcImage::DXT1                        : fmt = DDS_D3DFMT_DXT1         ; break;
	case grcImage::DXT3                        : fmt = DDS_D3DFMT_DXT3         ; break;
	case grcImage::DXT5                        : fmt = DDS_D3DFMT_DXT5         ; break;
	case grcImage::CTX1                        : fmt = DDS_D3DFMT_UNKNOWN      ; break; // not supported (xenon only)
	case grcImage::DXT3A                       : fmt = DDS_D3DFMT_UNKNOWN      ; break; // not supported (xenon only)
	case grcImage::DXT3A_1111                  : fmt = DDS_D3DFMT_UNKNOWN      ; break; // not supported (xenon only)
	case grcImage::DXT5A                       : fmt = DDS_D3DFMT_DXT5A        ; break;
	case grcImage::DXN                         : fmt = DDS_D3DFMT_DXN          ; break;
	case grcImage::BC6                         : fmt = DDS_D3DFMT_BC6          ; break;
	case grcImage::BC7                         : fmt = DDS_D3DFMT_BC7          ; break;
	case grcImage::A8R8G8B8                    : fmt = DDS_D3DFMT_A8R8G8B8     ; break;
	case grcImage::A8B8G8R8                    : fmt = DDS_D3DFMT_A8B8G8R8     ; break;
	case grcImage::A8                          : fmt = DDS_D3DFMT_A8           ; break;
	case grcImage::L8                          : fmt = DDS_D3DFMT_L8           ; break;
	case grcImage::A8L8                        : fmt = DDS_D3DFMT_A8L8         ; break;
	case grcImage::A4R4G4B4                    : fmt = DDS_D3DFMT_A4R4G4B4     ; break;
	case grcImage::A1R5G5B5                    : fmt = DDS_D3DFMT_A1R5G5B5     ; break;
	case grcImage::R5G6B5                      : fmt = DDS_D3DFMT_R5G6B5       ; break;
	case grcImage::R3G3B2                      : fmt = DDS_D3DFMT_R3G3B2       ; break;
	case grcImage::A8R3G3B2                    : fmt = DDS_D3DFMT_A8R3G3B2     ; break;
	case grcImage::A4L4                        : fmt = DDS_D3DFMT_A4L4         ; break;
	case grcImage::A2R10G10B10                 : fmt = DDS_D3DFMT_A2R10G10B10  ; break;
	case grcImage::A2B10G10R10                 : fmt = DDS_D3DFMT_A2B10G10R10  ; break;
	case grcImage::A16B16G16R16                : fmt = DDS_D3DFMT_A16B16G16R16 ; break;
	case grcImage::G16R16                      : fmt = DDS_D3DFMT_G16R16       ; break;
	case grcImage::L16                         : fmt = DDS_D3DFMT_L16          ; break;
	case grcImage::A16B16G16R16F               : fmt = DDS_D3DFMT_A16B16G16R16F; break;
	case grcImage::G16R16F                     : fmt = DDS_D3DFMT_G16R16F      ; break;
	case grcImage::R16F                        : fmt = DDS_D3DFMT_R16F         ; break;
	case grcImage::A32B32G32R32F               : fmt = DDS_D3DFMT_A32B32G32R32F; break;
	case grcImage::G32R32F                     : fmt = DDS_D3DFMT_G32R32F      ; break;
	case grcImage::R32F                        : fmt = DDS_D3DFMT_R32F         ; break;
	case grcImage::D15S1                       : fmt = DDS_D3DFMT_D15S1        ; break;
	case grcImage::D24S8                       : fmt = DDS_D3DFMT_D24S8        ; break;
	case grcImage::D24FS8                      : fmt = DDS_D3DFMT_D24FS8       ; break;
	case grcImage::P4                          : fmt = DDS_D3DFMT_UNKNOWN      ; break; // not supported
	case grcImage::P8                          : fmt = DDS_D3DFMT_UNKNOWN      ; break; // not supported
	case grcImage::A8P8                        : fmt = DDS_D3DFMT_UNKNOWN      ; break; // not supported
	case grcImage::R8                          : fmt = DDS_D3DFMT_R8           ; break; // custom
	case grcImage::R16                         : fmt = DDS_D3DFMT_R16          ; break; // custom
	case grcImage::G8R8                        : fmt = DDS_D3DFMT_G8R8         ; break; // custom
	case grcImage::LINA32B32G32R32F_DEPRECATED : fmt = DDS_D3DFMT_A32B32G32R32F; break;
	case grcImage::LINA8R8G8B8_DEPRECATED      : fmt = DDS_D3DFMT_A8R8G8B8     ; break;
	case grcImage::LIN8_DEPRECATED             : fmt = DDS_D3DFMT_L8           ; break;
	case grcImage::RGBE                        : fmt = DDS_D3DFMT_A8R8G8B8     ; break;
	}

	Assertf(fmt != DDS_D3DFMT_UNKNOWN, "Unsupported Image Format (%d)", (u32)image->GetFormat());

	return fmt;
}

u32 grcTextureFactoryDX9::GetImageFormatStaticDX9(u32 eFormat)
{
	switch (eFormat)
	{
	case DDS_D3DFMT_UNKNOWN       : return grcImage::UNKNOWN                    ;
	case DDS_D3DFMT_DXT1          : return grcImage::DXT1                       ;
	case DDS_D3DFMT_DXT3          : return grcImage::DXT3                       ;
	case DDS_D3DFMT_DXT5          : return grcImage::DXT5                       ;
//	case DDS_D3DFMT_UNKNOWN       : return grcImage::CTX1                       ; // not supported (xenon only)
//	case DDS_D3DFMT_UNKNOWN       : return grcImage::DXT3A                      ; // not supported (xenon only)
//	case DDS_D3DFMT_UNKNOWN       : return grcImage::DXT3A_1111                 ; // not supported (xenon only)
	case DDS_D3DFMT_DXT5A         : return grcImage::DXT5A                      ;
	case DDS_D3DFMT_DXN           : return grcImage::DXN                        ;
	case DDS_D3DFMT_BC6           : return grcImage::BC6                        ;
	case DDS_D3DFMT_BC7           : return grcImage::BC7                        ;
	case DDS_D3DFMT_A8R8G8B8      : return grcImage::A8R8G8B8                   ;
	case DDS_D3DFMT_A8B8G8R8      : return grcImage::A8B8G8R8                   ;
	case DDS_D3DFMT_A8            : return grcImage::A8                         ;
	case DDS_D3DFMT_L8            : return grcImage::L8                         ;
	case DDS_D3DFMT_A8L8          : return grcImage::A8L8                       ;
	case DDS_D3DFMT_A4R4G4B4      : return grcImage::A4R4G4B4                   ;
	case DDS_D3DFMT_A1R5G5B5      : return grcImage::A1R5G5B5                   ;
	case DDS_D3DFMT_R5G6B5        : return grcImage::R5G6B5                     ;
	case DDS_D3DFMT_R3G3B2        : return grcImage::R3G3B2                     ;
	case DDS_D3DFMT_A8R3G3B2      : return grcImage::A8R3G3B2                   ;
	case DDS_D3DFMT_A4L4          : return grcImage::A4L4                       ;
	case DDS_D3DFMT_A2R10G10B10   : return grcImage::A2R10G10B10                ;
	case DDS_D3DFMT_A2B10G10R10   : return grcImage::A2B10G10R10                ;
	case DDS_D3DFMT_A16B16G16R16  : return grcImage::A16B16G16R16               ;
	case DDS_D3DFMT_G16R16        : return grcImage::G16R16                     ;
	case DDS_D3DFMT_L16           : return grcImage::L16                        ;
	case DDS_D3DFMT_A16B16G16R16F : return grcImage::A16B16G16R16F              ;
	case DDS_D3DFMT_G16R16F       : return grcImage::G16R16F                    ;
	case DDS_D3DFMT_R16F          : return grcImage::R16F                       ;
	case DDS_D3DFMT_A32B32G32R32F : return grcImage::A32B32G32R32F              ;
	case DDS_D3DFMT_G32R32F       : return grcImage::G32R32F                    ;
	case DDS_D3DFMT_R32F          : return grcImage::R32F                       ;
	case DDS_D3DFMT_D15S1         : return grcImage::D15S1                      ;
	case DDS_D3DFMT_D24S8         : return grcImage::D24S8                      ;
	case DDS_D3DFMT_D24FS8        : return grcImage::D24FS8                     ;
//	case DDS_D3DFMT_UNKNOWN       : return grcImage::P4                         ; // not supported
//	case DDS_D3DFMT_P8            : return grcImage::P8                         ; // not supported
//	case DDS_D3DFMT_A8P8          : return grcImage::A8P8                       ; // not supported
	case DDS_D3DFMT_R8            : return grcImage::R8                         ; // custom
	case DDS_D3DFMT_R16           : return grcImage::R16                        ; // custom
	case DDS_D3DFMT_G8R8          : return grcImage::G8R8                       ; // custom
//	case DDS_D3DFMT_UNKNOWN       : return grcImage::LINA32B32G32R32F_DEPRECATED; // redundant
//	case DDS_D3DFMT_UNKNOWN       : return grcImage::LINA8R8G8B8_DEPRECATED     ; // redundant
//	case DDS_D3DFMT_UNKNOWN       : return grcImage::LIN8_DEPRECATED            ; // redundant
//	case DDS_D3DFMT_UNKNOWN       : return grcImage::RGBE                       ; // redundant
	}

	return grcImage::UNKNOWN;
}

u32 grcTextureFactoryDX9::GetBitsPerPixelStaticDX9(u32 uInternalFormat)
{
	switch(uInternalFormat)
	{
	case DDS_D3DFMT_R8G8B8:
	case DDS_D3DFMT_A8R8G8B8:
	case DDS_D3DFMT_X8R8G8B8:
	case DDS_D3DFMT_A2B10G10R10:
	case DDS_D3DFMT_A8B8G8R8:
	case DDS_D3DFMT_X8B8G8R8:
	case DDS_D3DFMT_G16R16:
	case DDS_D3DFMT_A2R10G10B10:
	case DDS_D3DFMT_X8L8V8U8:     
	case DDS_D3DFMT_Q8W8V8U8:     
	case DDS_D3DFMT_V16U16:       
	case DDS_D3DFMT_A2W10V10U10:  
	case DDS_D3DFMT_R8G8_B8G8:    
	case DDS_D3DFMT_G8R8_G8B8:    
	case DDS_D3DFMT_D32:          
	case DDS_D3DFMT_D24S8:       
	case DDS_D3DFMT_D24X8:        
	case DDS_D3DFMT_D24X4S4:      
	case DDS_D3DFMT_D32F_LOCKABLE:
	case DDS_D3DFMT_D24FS8:
	case DDS_D3DFMT_R32F:
	case DDS_D3DFMT_G16R16F:
		return 32;

	case DDS_D3DFMT_R5G6B5:
	case DDS_D3DFMT_X1R5G5B5:
	case DDS_D3DFMT_A1R5G5B5:
	case DDS_D3DFMT_A4R4G4B4:
	case DDS_D3DFMT_A8R3G3B2:
	case DDS_D3DFMT_X4R4G4B4:
	case DDS_D3DFMT_A8P8:
	case DDS_D3DFMT_A8L8:         
	case DDS_D3DFMT_G8R8: // custom
	case DDS_D3DFMT_V8U8:         
	case DDS_D3DFMT_L6V5U5:       
	case DDS_D3DFMT_D16_LOCKABLE: 
	case DDS_D3DFMT_D15S1:        
	case DDS_D3DFMT_D16:
	case DDS_D3DFMT_R16F:
	case DDS_D3DFMT_R16: // custom
	case DDS_D3DFMT_L16:
		return 16;

	case DDS_D3DFMT_R3G3B2:
	case DDS_D3DFMT_A8:
	case DDS_D3DFMT_P8:
	case DDS_D3DFMT_L8:
	case DDS_D3DFMT_R8: // custom
	case DDS_D3DFMT_A4L4:         
	case DDS_D3DFMT_DXT2:         
	case DDS_D3DFMT_DXT3:         
	case DDS_D3DFMT_DXT4:         
	case DDS_D3DFMT_DXT5:  
	case DDS_D3DFMT_DXN:
		return 8;

	case DDS_D3DFMT_A16B16G16R16:
	case DDS_D3DFMT_A16B16G16R16F:
	case DDS_D3DFMT_G32R32F:
		return 64;

	case DDS_D3DFMT_A32B32G32R32F:
		return 128;

	case DDS_D3DFMT_DXT1: 
		return 4;

	default:
		Assertf(0, "DX9: Unknown Texture Format %u", uInternalFormat);
		return 0;
	}
}

const grcRenderTarget *grcTextureFactoryDX9::GetBackBuffer(bool realize) const {
	return GetBackBuffer(realize);
}

grcRenderTarget *grcTextureFactoryDX9::GetBackBuffer(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryDX9::GetFrontBuffer(bool nextBuffer) const {
	return GetFrontBuffer(nextBuffer);
}

grcRenderTarget *grcTextureFactoryDX9::GetFrontBuffer(bool UNUSED_PARAM(nextBuffer)) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryDX9::GetFrontBufferDepth(bool realize) const {
	return GetFrontBufferDepth(realize);
}

grcRenderTarget *grcTextureFactoryDX9::GetFrontBufferDepth(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

const grcRenderTarget *grcTextureFactoryDX9::GetBackBufferDepth(bool realize) const {
	return GetBackBufferDepth(realize);
}

grcRenderTarget *grcTextureFactoryDX9::GetBackBufferDepth(bool /*realize*/) {
	// TODO: Make this work
	// For now, just make it obvious that it's not yet functional
	return grcTextureFactory::GetNotImplementedRenderTarget();
}

grcRenderTarget	*grcTextureFactoryDX9::CreateRenderTarget(const char * pName, grcRenderTargetType eType, int nWidth, int nHeight, int nBitsPerPixel, CreateParams *params WIN32PC_ONLY(, grcRenderTarget* /*originalTarget*/))
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

#if ENFORCE_UNIQUE_TARGET_NAMES
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
#endif

	grcRenderTargetDX9 *tgt = rage_new grcRenderTargetDX9(pName,eType,nWidth,nHeight,nBitsPerPixel,params);
	tgt->SetSlotId(sm_ActiveTargets.GetCount());
	sm_ActiveTargets.Append() = tgt;

	RegisterRenderTarget(tgt);

	return tgt;
}

grcRenderTarget* grcTextureFactoryDX9::CreateRenderTarget(const char *pName, const grcTextureObject *pTexture WIN32PC_ONLY(, grcRenderTarget* /*originalTarget*/))
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(RenderTarget);
	RAGE_TRACK_NAME(pName);

#if ENFORCE_UNIQUE_TARGET_NAMES
	// Render Targets must have unique names using this code.  This was to eliminate leaks cause by bad code on recreation
	{
		for (int i = 0; i < sm_ActiveTargets.GetCount(); ++i) {
			grcRenderTargetDX9 *tgt = sm_ActiveTargets[i];
			if(stricmp(tgt->GetName(), pName) == 0)
			{
				AssertMsg(0, ("Creating Render Target that already exists - Either delete and let rage recreate %s", pName));
			}
		}
	}
#endif

	grcRenderTargetDX9 *tgt = rage_new grcRenderTargetDX9(pName,pTexture);
	tgt->SetSlotId(sm_ActiveTargets.GetCount());
	sm_ActiveTargets.Append() = tgt;
	return tgt;
}


static grcRenderTargetDX9 *s_LastLocks[/*MAX_RENDER_TARGETS*/ 8];

void grcTextureFactoryDX9::LockRenderTarget(int index, const grcRenderTarget *target, const grcRenderTarget * depth, u32 layer, bool lockDepth, u32 D3D11_ONLY(mipToLock))
{
	const u32 mipLevel = 0;
	Assert(!target || (layer < target->GetLayerCount() && mipLevel < (u32)target->GetMipMapCount()));
	Assert(!depth || depth->GetMipMapCount() == 1);
	Assert(index < MAX_RENDER_TARGETS && "Index is outside range of supported render targets");
	Assert(!m_PreviousTargets[index]  && "Must Unlock previous target before locking new target");
	Assert(index || !m_PreviousDepth);
	Assert((target == NULL) || target->IsValid());
	Assert((depth == NULL) || (depth->IsValid()));

	GRCDEVICE.GetRenderTarget(index, &m_PreviousTargets[index]);

	s_LastLocks[index] = (grcRenderTargetDX9*)target;

	AssertMsg( target || depth, "Lock Render target must be passed either a valid color or depth render target" );
	SURFACE *tsurface = NULL, *dsurface;

	if (target)
		((grcRenderTargetPC*)target)->SetUnresolved();
	if (depth)
		((grcRenderTargetPC*)depth)->SetUnresolved();

	Assert(target == NULL || target->GetType() != grcrtVolume);

	if (target) {
		if (target->GetType() == grcrtCubeMap)
			CHECK_HRESULT(static_cast<IDirect3DCubeTexture9*>(s_LastLocks[index]->m_CachedTexturePtr)->GetCubeMapSurface((D3DCUBEMAP_FACES)layer, mipLevel, &tsurface));
		else if (!s_LastLocks[index]->m_Multisample)
			CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(s_LastLocks[index]->m_CachedTexturePtr)->GetSurfaceLevel(mipLevel, &tsurface));
		else {
			tsurface = (SURFACE*)s_LastLocks[index]->m_Surface;
			tsurface->AddRef();
		}

		GRCDEVICE.SetRenderTarget(index, tsurface);
	}

	if ( index == 0 ) {
		if ( lockDepth ) {
			if (depth)
			{
				m_DepthTarget = depth;
				dsurface = (SURFACE*)((grcRenderTargetDX9*)depth)->m_Surface;
				if (dsurface == NULL)
				{					
					if (FAILED(static_cast<IDirect3DTexture9 *>(((grcRenderTargetDX9*)depth)->m_CachedTexturePtr)->GetSurfaceLevel(0, &dsurface)))
					{
						grcErrorf("LockRenderTarget failed to assign depth surface");
					}
					m_CurTexDepthSurface = dsurface;
				}
			} 
			else
			{
				dsurface = NULL;
			}
			GRCDEVICE.GetDepthStencilSurface(&m_PreviousDepth);
			GRCDEVICE.SetDepthStencilSurface(dsurface);
		}
		m_PreviousWidth = GRCDEVICE.GetWidth();
		m_PreviousHeight = GRCDEVICE.GetHeight();
		if ( target )
		{			
			GRCDEVICE.SetSize(Max(target->GetWidth()>>mipLevel,1),Max(target->GetHeight()>>mipLevel,1));
		}
		else 
		{
			Assert(mipLevel == 0);
			GRCDEVICE.SetSize(depth->GetWidth(),depth->GetHeight());
		}

		m_PreviousDepthLock = lockDepth;
	}
	else {
		// NOTE: What do we want to do here?  Warn that width/height not same as index 0????
		//	 Warn that addition depth surfaces will be ignored???
	}
	if (tsurface) {
		tsurface->Release();
	}
}

void	grcTextureFactoryDX9::UnlockRenderTarget	(int index,const grcResolveFlags*)
{
	Assert(index < MAX_RENDER_TARGETS && "Index is outside range of supported render targets");
	Assert(index || m_PreviousTargets[index]);
	GRCDEVICE.SetRenderTarget(index, m_PreviousTargets[index]);
	if ( m_PreviousTargets[index] ) {
		((SURFACE*)m_PreviousTargets[index])->Release();
	}
	if ( index == 0 ) {
		if ( m_PreviousDepthLock && (m_PreviousDepth != NULL)) 
		{
			m_DepthTarget = NULL;
			GRCDEVICE.SetDepthStencilSurface(m_PreviousDepth);
			((SURFACE*)m_PreviousDepth)->Release();
		}
		else
		{
			Assert(m_PreviousDepth == NULL);
		}
		if (m_CurTexDepthSurface != NULL)
		{
			((SURFACE*)m_CurTexDepthSurface)->Release();
			m_CurTexDepthSurface = NULL;
		}
		m_PreviousDepth = 0;
		m_PreviousDepthLock = false;
		GRCDEVICE.SetSize(m_PreviousWidth,m_PreviousHeight);
	}
	m_PreviousTargets[index] = 0;
	if ((s_LastLocks[index] != NULL) && (s_LastLocks[index]->m_Multisample)) // && poResolveFlags->NeedResolve) 
	{
		s_LastLocks[index]->Resolve(NULL);
	}
	else
	{
		Assert( s_LastLocks[index]->GetResolved() == s_LastLocks[index]->GetTexturePtr() );
		//if (poResolveFlags && poResolveFlags->NeedResolve) // Not really needed but I want to catch missed stretch rects
			//s_LastLocks[index]->SetResolved();
	}
	s_LastLocks[index] = NULL;
}


void grcTextureFactoryDX9::LockMRT(const grcRenderTarget *color[grcmrtColorCount],const grcRenderTarget *depth, const u32* D3D11_ONLY(mipsToLock)) 
{
	u32 uNumViews;
	for (uNumViews = 0; uNumViews < MAX_RENDER_TARGETS; uNumViews++)
	{
		if (color[uNumViews] == NULL)
			break;

		GRCDEVICE.GetRenderTarget(uNumViews, &m_PreviousTargets[uNumViews]);
		s_LastLocks[uNumViews] = (grcRenderTargetDX9*)color[uNumViews];
		s_LastLocks[uNumViews]->SetUnresolved();
		Assert(s_LastLocks[uNumViews]->GetType() != grcrtVolume);
		Assert(s_LastLocks[uNumViews]->GetType() != grcrtCubeMap);
	}
	Assert(uNumViews > 0);

	SURFACE *tsurface[MAX_RENDER_TARGETS], *dsurface;

	for (u32 uIndex = 0; uIndex < uNumViews; uIndex++)
	{
		if (!s_LastLocks[uIndex]->m_Multisample)
		{
			CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(s_LastLocks[uIndex]->m_CachedTexturePtr)->GetSurfaceLevel(0, &tsurface[uIndex]));
		}
		else 
		{
			tsurface[uIndex] = (SURFACE*)s_LastLocks[uIndex]->m_Surface;
			tsurface[uIndex]->AddRef();
		}
		GRCDEVICE.SetRenderTarget(uIndex, tsurface[uIndex]);
	}

	if (depth)
	{
		m_DepthTarget = depth;
		dsurface = (SURFACE*)((grcRenderTargetDX9*)depth)->m_Surface;
		if (dsurface == NULL)
		{					
			if (FAILED(CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(((grcRenderTargetDX9*)depth)->m_CachedTexturePtr)->GetSurfaceLevel(0, &dsurface))))
			{
				grcErrorf("LockRenderTarget failed to assign depth surface");
			}
			m_CurTexDepthSurface = dsurface;
		}
	} 
	else
	{
		dsurface = NULL;
	}
	GRCDEVICE.GetDepthStencilSurface(&m_PreviousDepth);
	GRCDEVICE.SetDepthStencilSurface(dsurface);

	m_PreviousWidth = GRCDEVICE.GetWidth();
	m_PreviousHeight = GRCDEVICE.GetHeight();
	if ( uNumViews )
	{			
		GRCDEVICE.SetSize(color[0]->GetWidth(),color[0]->GetHeight());
	}
	else 
	{
		GRCDEVICE.SetSize(depth->GetWidth(),depth->GetHeight());
	}
	if (depth)
		m_PreviousDepthLock = true;

	for (u32 uIndex = 0; uIndex < uNumViews; uIndex++)
	{
		tsurface[uIndex]->Release();
	}
}

void grcTextureFactoryDX9::UnlockMRT(const grcResolveFlagsMrt* /*poResolveFlags = NULL*/) 
{ 
	for (u32 uIndex = 0; uIndex < MAX_RENDER_TARGETS; uIndex++)
	{
		GRCDEVICE.SetRenderTarget(uIndex, m_PreviousTargets[uIndex]);
		if ( m_PreviousTargets[uIndex] ) {
			((SURFACE*)m_PreviousTargets[uIndex])->Release();
		}
	}

	if ( m_PreviousDepthLock && (m_PreviousDepth != NULL)) 
	{
		m_DepthTarget = NULL;
		GRCDEVICE.SetDepthStencilSurface(m_PreviousDepth);
		((SURFACE*)m_PreviousDepth)->Release();
	}
	else
	{
		Assert(m_PreviousDepth == NULL);
	}
	if (m_CurTexDepthSurface != NULL)
	{
		((SURFACE*)m_CurTexDepthSurface)->Release();
		m_CurTexDepthSurface = NULL;
	}
	m_PreviousDepth = 0;
	m_PreviousDepthLock = false;
	GRCDEVICE.SetSize(m_PreviousWidth,m_PreviousHeight);

	for (u32 uIndex = 0; uIndex < MAX_RENDER_TARGETS; uIndex++)
	{
		m_PreviousTargets[uIndex] = 0;

		if ((s_LastLocks[uIndex] != NULL) && (s_LastLocks[uIndex]->m_Multisample)) // && poResolveFlags[uIndex]->NeedResolve) 
		{
			s_LastLocks[uIndex]->Resolve(NULL);
		}
		else
		{
			Assert( s_LastLocks[uIndex]->GetResolved() == s_LastLocks[uIndex]->GetTexturePtr() );
			//if (poResolveFlags && (*poResolveFlags)[uIndex]->NeedResolve) // Not really needed but I want to catch missed stretch rects
				//s_LastLocks[uIndex]->SetResolved();
		}
		s_LastLocks[uIndex] = NULL;
	}
}


//=============================================================================
// grcTextureDX9
//=============================================================================
grcTextureDX9::grcTextureDX9(const char *pFilename,grcTextureFactory::TextureCreateParams *params) : grcTexturePC(params)
{
	grcImage	*pImage = NULL;
	bool      bImageCreated = false;
#if __RESOURCECOMPILER
	void*     pProcessProxy = NULL;
#endif

#if __RESOURCECOMPILER
	sysMemStartTemp();
#endif

	m_StagingTexture = NULL;

	if	(strcmp(pFilename, "none") && strcmp(pFilename, "nonresident"))
	{		
#if __RESOURCECOMPILER
		grcImage::ImageList images;
		bool result = grcImage::RunCustomLoadFunc( pFilename, images, &pProcessProxy );
		if ( result )
			pImage = images[1];

		if (!result || pImage == NULL) // try loading normally
#endif // __RESOURCECOMPILER
		{
			pImage = grcImage::Load(pFilename);
			bImageCreated = true;
		}
	}

#if __RESOURCECOMPILER
	sysMemEndTemp();
#endif

	if	(pImage)
	{
		int	w = pImage->GetWidth();
		int	h = pImage->GetHeight();

		if	((w & 3) || (h & 3))
		{
			grcErrorf("grcTexturePC - Texture '%s' - invalid resolution %d by %d", pFilename, w, h);
			sysMemStartTemp();
			pImage->Release();
			sysMemEndTemp();
			pImage = NULL;
			bImageCreated = false;
#if __RESOURCECOMPILER
			pProcessProxy = NULL;
#endif
		}
	}

	if	(!pImage)
	{
		u32 texel = strcmp(pFilename, "nonresident") ? 0xFFFFFFFF : 0x40FFFFF;

		sysMemStartTemp();
		pImage = grcImage::Create(4, 4, 1, grcImage::A8R8G8B8, grcImage::STANDARD, false,0);
		bImageCreated = true;
		sysMemEndTemp();
		u32 *texels = (u32*) pImage->GetBits();

		for	(int i = 0; i < 16; i++)
			texels[i] = texel;
	}

	grcDevice::Result uReturnCode =  Init(pFilename,pImage,params);
#if __RESOURCECOMPILER
	RunCustomProcessFunc(pProcessProxy);
#endif
	if(uReturnCode != 0)
	{
		// Errors occured, so just use a checkerboard
		// Free current image
		sysMemStartTemp();
		pImage->Release();

		// Create horrible texture to use
		u32 texel = 0xFFFFFFFF;// uReturnCode; //strcmp(pFilename, "nonresident") ? 0xFFFFFFFF : 0x40FFFFF;

		pImage = grcImage::Create(4, 4, 1, grcImage::A8R8G8B8, grcImage::STANDARD, false,0);
		u32 *texels = (u32*) pImage->GetBits();

		for	(int i = 0; i < 16; i++)
		{
			texels[i] = texel;
		}
		sysMemEndTemp();

		// Use it
		Init(pFilename,pImage,params);
	}

	if (bImageCreated)
	{
		sysMemStartTemp();
		pImage->Release();
		sysMemEndTemp();
	}
}


grcTextureDX9::grcTextureDX9(grcImage *pImage,grcTextureFactory::TextureCreateParams *params) : grcTexturePC(params)
{
	const char* name = "image";
#if __BANK || __RESOURCECOMPILER
	name = grcTexture::GetCustomLoadName(name);
#endif // __BANK || __RESOURCECOMPILER

	m_StagingTexture = NULL;

	Init(name,pImage,params);
}


grcTextureDX9::grcTextureDX9(u32 width, u32 height, grcTextureFormat eFormat, void*, grcTextureFactory::TextureCreateParams *params) : grcTexturePC(params)
{
	Assert(grcTextureFactoryPC::HasInstance());
	//Assert((pBuffer == NULL) && "Not Implemented");

#if __PAGING
	m_BackingStore = NULL;
#endif

	m_IsSRGB = false; // pImage->IsSRGB(); // PC TODO - Implement
	m_CutMipLevels = 0;

	m_Name = NULL; //grcSaveTextureNames ? StringDuplicate(pFilename) : 0;
	m_ImageType = (u8)grcImage::STANDARD; // pImage->GetType();
	Assign(m_LayerCount,1); //pImage->GetLayerCount()-1);
	m_Width = (u16)width;
	m_Height = (u16)height;
	m_Depth = 1;
	m_nMipCount = (u8)1; //(pImage->GetExtraMipCount() + 1);
	m_nFormat = static_cast<D3DFORMAT>(static_cast<grcTextureFactoryPC&>(grcTextureFactoryPC::GetInstance()).Translate(eFormat));
	m_nMipStride = (u16)(GetBitsPerPixel() / 8);
	m_pShaderResourceView = NULL;
	m_StagingTexture = NULL;

	u32 uUsage = 0;

	if (params && (params->Memory == grcTextureFactory::TextureCreateParams::SYSTEM))
	{
		if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED)
		{
			uUsage |= D3DUSAGE_DYNAMIC;
		}
	}

	ASSERT_ONLY(grcDevice::Result uReturnCode = ) GRCDEVICE.CreateTexture(m_Width, m_Height, m_LayerCount, m_nMipCount, uUsage, m_nFormat, 
		GRCDEVICE.GetTextureResourcePool(), m_ImageType, &m_CachedTexturePtr);

	AssertMsg((((HRESULT)uReturnCode) != E_OUTOFMEMORY), ("Error %ud (out of memory) occurred creating texture", uReturnCode));
	AssertMsg((uReturnCode == 0), ("Error %ud occurred creating texture", uReturnCode));
	Assert(m_CachedTexturePtr != NULL);

	SetPrivateData();
	//m_CachedTexturePtr = (TEXTURE*)m_Texture;
}


bool grcTextureDX9::IsValid() const 
{ 
#if __FINAL || !USE_RESOURCE_CACHE
	return true;
#else
	return grcResourceCache::GetInstance().Validate((grcDeviceTexture*)GetTexturePtr()); 
#endif // __FINAL
}

void grcTextureDX9::SetPrivateData()
{
	grcTexture* pTex = this;
	if (m_CachedTexturePtr)
		AssertVerify(((IDirect3DResource9*)m_CachedTexturePtr)->SetPrivateData(TextureBackPointerGuid,&pTex,sizeof(pTex),0) == D3D_OK);
}

grcTexture*	grcTextureDX9::GetPrivateData(const grcTextureObject *pTexObj)
{
	rage::grcTexture *pTexture = NULL;
	UINT sizeofData = sizeof(pTexture);

	if (pTexObj)
		AssertVerify(((IDirect3DResource9*)pTexObj)->GetPrivateData(TextureBackPointerGuid,&pTexture,(DWORD*)&sizeofData) == D3D_OK);

	if (!pTexture)
		grcWarningf("GetPrivateData failed?  obj=%p",pTexObj);
	return pTexture;
}

grcDevice::Result grcTextureDX9::Init(const char *pFilename,grcImage *pImageIn,grcTextureFactory::TextureCreateParams * params)
{
	grcImage *pImage = pImageIn;
	u32	nFormat = static_cast<D3DFORMAT>(static_cast<grcTextureFactoryPC&>(grcTextureFactoryPC::GetInstance()).GetD3DFormat(pImage));

#if __RESOURCECOMPILER
	// Sometimes we encounter DXT style textures marked a scripted RTs, we can`t render to them, they need to be 32bit really.
	if(!strncmp(pFilename, "script_rt", strlen("script_rt")) && grcImage::IsFormatDXTBlockCompressed(pImage->GetFormat()))
	{
		Warningf("WARNING!- [From file %s] grcTextureDX9::Init()...Can`t use DXTn textures as rendertargets (promoting to 32bit).\n", pFilename);
		sysMemStartTemp();
		pImage = grcImage::Create(pImageIn->GetWidth(), pImageIn->GetHeight(), pImageIn->GetDepth(), grcImage::A8R8G8B8, pImageIn->GetType(), 0, 0);
		sysMemEndTemp();
	}
#endif //__RESOURCECOMPILER


	m_IsSRGB = pImage->IsSRGB();
	m_CutMipLevels = 0;
	m_ExtraFlags = pImage->IsSysMem() ? 1 : 0;
#if __64BIT
	m_ExtraFlagsPadding = 0;
#endif //__64BIT

	m_Name = grcSaveTextureNames ? StringDuplicate(pFilename) : 0;
	m_ImageType = (u8) pImage->GetType();
	Assign(m_LayerCount,pImage->GetLayerCount()-1);
	m_Width = pImage->GetWidth();
	m_Height = pImage->GetHeight();
	m_Depth = pImage->GetDepth();
	m_nMipCount = (u8)(pImage->GetExtraMipCount() + 1);
	m_nFormat = nFormat;
	m_nMipStride = (u16)pImage->GetStride();
	m_pShaderResourceView = NULL;

	Assert( m_StagingTexture == NULL );
	m_StagingTexture = NULL;

#if __PAGING
	u8 *pStorage = NULL;
	int nStorageSize = 0;
	bool paging = sysMemAllocator::GetCurrent().IsBuildingResource() || (!GRCDEVICE.IsCreated());
	if (paging) {
		m_CachedTexturePtr = NULL;
		grcImage *tmp, *pLayer = pImage;
		while ( pLayer ) {
			tmp = pLayer;
			while (tmp) {
				nStorageSize += tmp->GetSize();
				tmp = tmp->GetNext();
			}
			pLayer = pLayer->GetNextLayer();
		}

		int nAlignment = 256;

	#if __64BIT && __RESOURCECOMPILER
		int nPaddedAlignment = 0;
		int nPaddedSize = ComputeOrbisDurangoPaddedSizeAndAlignment(pImage->GetWidth(), pImage->GetHeight(), pImage->GetDepth(), m_nMipCount, 1, pImage->GetFormat(), pImage->GetType(), nPaddedAlignment);

		if(nPaddedSize > nStorageSize)
			nStorageSize = nPaddedSize;
		if(nPaddedAlignment > nAlignment)
			nAlignment = nPaddedAlignment;
	#endif //__64BIT && __RESOURCECOMPILER

		pStorage = (u8*)(m_BackingStore = physical_new(nStorageSize,nAlignment));
		SetPhysicalSize(nStorageSize);
	}
	else
#endif
	{
		u32 uUsage = 0;

		if (params && (params->Memory == grcTextureFactory::TextureCreateParams::SYSTEM))
		{
			if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED)
			{
				uUsage |= D3DUSAGE_DYNAMIC;
			}
		}

		grcDevice::Result uReturnCode =  GRCDEVICE.CreateTexture(pImage->GetWidth(), pImage->GetHeight(), pImage->GetDepth(), pImage->GetExtraMipCount() + 1, uUsage, nFormat, 
			GRCDEVICE.GetTextureResourcePool(), pImage->GetType(), &m_CachedTexturePtr);

		SetPrivateData();

		//When using unmanaged pools we need to create a staging texture in System mem to copy the data into then update the main texture from this.
		if ( GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED )
		{
			GRCDEVICE.CreateTexture(pImage->GetWidth(), pImage->GetHeight(), pImage->GetDepth(), pImage->GetExtraMipCount() + 1, uUsage, nFormat, RESOURCE_SYSTEMMEM , pImage->GetType(), &m_StagingTexture);
		}

		//m_CachedTexturePtr = (TEXTURE*)m_CachedTexturePtr;
#if __TOOL
		if(rage::PARAM_noquits.Get())
		{
			if(((HRESULT)uReturnCode) == E_OUTOFMEMORY)
			{
				grcErrorf("Error 0x%08x (out of memory) occurred creating texture %s", uReturnCode, pFilename);
			}
			else if(uReturnCode != 0)
			{
				grcErrorf("Error 0x%08x occurred creating texture %s", uReturnCode, pFilename);
			}
		}
		else
		{
#endif
			Assertf((((HRESULT)uReturnCode) != E_OUTOFMEMORY), "Error %ud (out of memory) occurred creating texture %s", uReturnCode, pFilename);
			Assertf((uReturnCode == 0), "Error %ud occurred creating texture %s", uReturnCode, pFilename);
#if __TOOL
		}
#endif
		if(uReturnCode != 0)
		{
			return uReturnCode;
		}

#if __PAGING
		m_BackingStore = NULL;
#endif
	}

	HRESULT hRes = Copy(pImage)? D3D_OK : E_UNEXPECTED;

#if __RESOURCECOMPILER
	if(pImage != pImageIn)
	{
		sysMemStartTemp();
		pImage->Release();
		sysMemEndTemp();
	}
#endif //__RESOURCECOMPILER

	return hRes;
}

bool grcTextureDX9::Copy(const grcTexture* pSource, s32  UNUSED_PARAM(dstSliceIndex), s32  UNUSED_PARAM(dstMipIndex), s32  UNUSED_PARAM(srcSliceIndex), s32  UNUSED_PARAM(srcMipIndex))
{
	const int              m =                   GetMipMapCount ();
	const grcImage::Format f = (grcImage::Format)pSource->GetImageFormat ();

	for (int i = 0; i < m; i++) // copy each mipmap
	{
		grcTextureLock srcLock;
		grcTextureLock newLock;

		AssertVerify( pSource->LockRect(0, i, srcLock, grcsRead  | grcsAllowVRAMLock) );
		AssertVerify( LockRect(0, i, newLock, grcsWrite | grcsAllowVRAMLock) );

		if (newLock.Base && srcLock.Base)
		{
			const int blockSize = grcImage::IsFormatDXTBlockCompressed( f ) ? 4 : 1;
			sysMemCpy(newLock.Base, srcLock.Base, ( newLock.Pitch / blockSize ) * newLock.Height);
		}

		pSource->UnlockRect(srcLock);
		UnlockRect(newLock);
	}
	return true;
}

bool grcTextureDX9::Copy(const grcImage *pImage)
{
	if (m_LayerCount != pImage->GetLayerCount()-1 || m_Width != pImage->GetWidth() || m_Height != pImage->GetHeight() || 
		m_nMipCount != pImage->GetExtraMipCount()+1)
		return false;

	D3DLOCKED_RECT	rect;

	TEXTURE* poTexture = (TEXTURE*)( m_StagingTexture ? m_StagingTexture : m_CachedTexturePtr );

	const grcImage	*pLayer = pImage;
	int			layer = 0;
#if __PAGING
	bool paging = sysMemAllocator::GetCurrent().IsBuildingResource() || (!GRCDEVICE.IsCreated());
	u8 *pStorage = (u8*) m_BackingStore;
#endif
	while ( pLayer ) {
		const grcImage	*pMip = pLayer;
		for	(int i = 0; i < pLayer->GetExtraMipCount() + 1; i++)
		{
			void	*pDest = NULL;
#if __PAGING
			if (paging) {
				pDest = pStorage; 
			}
			else
#endif
			{
				if ( m_ImageType == grcImage::CUBE ) {
					// MAKE SURE D3D STILL MAINTAINS CUBEMAPS IN SEQUENTIAL ORDER
					Assert(
						D3DCUBEMAP_FACE_NEGATIVE_X==D3DCUBEMAP_FACE_POSITIVE_X+1 &&
						D3DCUBEMAP_FACE_POSITIVE_Y==D3DCUBEMAP_FACE_NEGATIVE_X+1 &&
						D3DCUBEMAP_FACE_NEGATIVE_Y==D3DCUBEMAP_FACE_POSITIVE_Y+1 &&
						D3DCUBEMAP_FACE_POSITIVE_Z==D3DCUBEMAP_FACE_NEGATIVE_Y+1 &&
						D3DCUBEMAP_FACE_NEGATIVE_Z==D3DCUBEMAP_FACE_POSITIVE_Z+1 &&
						"D3D Cubemap face defines are no longer sequential!");

					CHECK_HRESULT(static_cast<IDirect3DCubeTexture9 *>(poTexture)->LockRect((D3DCUBEMAP_FACES)(D3DCUBEMAP_FACE_POSITIVE_X+layer), i, &rect, NULL, 0 ));
				}
				else if ( m_ImageType == grcImage::VOLUME ) {
					D3DBOX inbox = {0,0,pMip->GetWidth(),pMip->GetHeight(),0,pMip->GetDepth()};
					D3DLOCKED_BOX lbox;
					static_cast<IDirect3DVolumeTexture9 *>(poTexture)->LockBox(i,&lbox,&inbox,0);
					rect.pBits = lbox.pBits;
					rect.Pitch = lbox.RowPitch;
				}
				else {		
					AssertVerify(CHECK_HRESULT(static_cast<IDirect3DTexture9*>(poTexture)->LockRect(i, &rect, NULL, 0)) == D3D_OK);
				}
				pDest = rect.pBits;
			}

			void	*pSource = (void*) pMip->GetBits();
			int		mipSize = pMip->GetSize();
			Assert((pDest != NULL) && (pSource != NULL));
			if (pDest != NULL)
			{
				sysMemCpy(pDest, pSource, mipSize);
			}

#if __PAGING
			if (paging)
				pStorage += pMip->GetSize();
			else
#endif
				if ( m_ImageType == grcImage::CUBE ) {
					CHECK_HRESULT(static_cast<IDirect3DCubeTexture9 *>(poTexture)->UnlockRect((D3DCUBEMAP_FACES)layer, i));
				}
				else if ( m_ImageType == grcImage::VOLUME ) {
					CHECK_HRESULT(static_cast<IDirect3DVolumeTexture9 *>(poTexture)->UnlockBox(i));
				}
				else {
					CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(poTexture)->UnlockRect(i));
				}
			pMip = pMip->GetNext();
		}
		pLayer = pLayer->GetNextLayer();
		layer++;
	}

	//If we have a staging texture update the main texture from this and release the staging texture
	if( m_StagingTexture )
	{
		TEXTURE* destTexture = (TEXTURE*)m_CachedTexturePtr;

		if ( m_ImageType == grcImage::CUBE ) {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DCubeTexture9 *>(poTexture), static_cast<IDirect3DCubeTexture9 *>(destTexture) ));
		}
		else if ( m_ImageType == grcImage::VOLUME ) {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DVolumeTexture9 *>(poTexture), static_cast<IDirect3DVolumeTexture9 *>(destTexture) ));
		}
		else {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DTexture9 *>(poTexture), static_cast<IDirect3DTexture9 *>(destTexture) ));
		}

		ASSERT_ONLY(u32 refCount = )((TEXTURE*)m_StagingTexture)->Release();
		Assert( refCount == 0 );
		m_StagingTexture = NULL;
		
	}

	return true;
}

bool grcTextureDX9::Copy(const void * pvSrc, u32 uWidth, u32 uHeight, u32 uDepth)
{
	if (/*m_LayerCount != pImage->GetLayerCount()-1 ||*/ m_Width != uWidth || m_Height != uHeight) // || m_nMipCount != pImage->GetExtraMipCount()+1)
	{
		return false;
	}

	D3DLOCKED_RECT	rect;

	//Note: This function isnt currently used so the staging texture functionality hasnt been tested.
	TEXTURE* poTexture = (TEXTURE*)( m_StagingTexture ? m_StagingTexture : m_CachedTexturePtr );

	//const grcImage	*pLayer = pImage;
	int			layer = 0;
#if __PAGING
	bool paging = sysMemAllocator::GetCurrent().IsBuildingResource() || (!GRCDEVICE.IsCreated());
	u8 *pStorage = (u8*) m_BackingStore;
#endif
	/*while ( pLayer )*/
	{
		//const grcImage	*pMip = pLayer;
		for	(int i = 0; i < 1 /*pLayer->GetExtraMipCount() + 1*/; i++)
		{
			void	*pDest = NULL;
#if __PAGING
			if (paging) {
				pDest = pStorage; 
			}
			else
#endif
			{
				if ( m_ImageType == grcImage::CUBE ) {
					// MAKE SURE D3D STILL MAINTAINS CUBEMAPS IN SEQUENTIAL ORDER
					Assert(
						D3DCUBEMAP_FACE_NEGATIVE_X==D3DCUBEMAP_FACE_POSITIVE_X+1 &&
						D3DCUBEMAP_FACE_POSITIVE_Y==D3DCUBEMAP_FACE_NEGATIVE_X+1 &&
						D3DCUBEMAP_FACE_NEGATIVE_Y==D3DCUBEMAP_FACE_POSITIVE_Y+1 &&
						D3DCUBEMAP_FACE_POSITIVE_Z==D3DCUBEMAP_FACE_NEGATIVE_Y+1 &&
						D3DCUBEMAP_FACE_NEGATIVE_Z==D3DCUBEMAP_FACE_POSITIVE_Z+1 &&
						"D3D Cubemap face defines are no longer sequential!");
					CHECK_HRESULT(static_cast<IDirect3DCubeTexture9 *>(poTexture)->LockRect((D3DCUBEMAP_FACES)(D3DCUBEMAP_FACE_POSITIVE_X+layer), i, &rect, NULL, 0 ));

				}
				else if ( m_ImageType == grcImage::VOLUME ) {
					D3DBOX inbox = {0,0,uWidth,uHeight,0,0/*pMip->GetDepth()*/};
					D3DLOCKED_BOX lbox;
					CHECK_HRESULT(static_cast<IDirect3DVolumeTexture9 *>(poTexture)->LockBox(i,&lbox,&inbox,0));
					rect.pBits = lbox.pBits;
					rect.Pitch = lbox.RowPitch;
				}
				else {	
					AssertVerify(CHECK_HRESULT(static_cast<IDirect3DTexture9*>(poTexture)->LockRect(i, &rect, NULL, 0)) == D3D_OK);
				}
				pDest = rect.pBits;
			}

			void	*pSource = (void*) pvSrc; //pMip->GetBits();
			int		mipSize = (uWidth * uHeight * uDepth) / 8; // pMip->GetSize();
			Assert((pDest != NULL) && (pSource != NULL));
			if (pDest != NULL)
			{
				sysMemCpy(pDest, pSource, mipSize);
			}

#if __PAGING
			if (paging)
				pStorage += mipSize; //pMip->GetSize();
			else
#endif
				if ( m_ImageType == grcImage::CUBE ) {
					CHECK_HRESULT(static_cast<IDirect3DCubeTexture9 *>(poTexture)->UnlockRect((D3DCUBEMAP_FACES)layer, i));
				}
				else if ( m_ImageType == grcImage::VOLUME ) {
					CHECK_HRESULT(static_cast<IDirect3DVolumeTexture9 *>(poTexture)->UnlockBox(i));
				}
				else {
					AssertVerify(CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(poTexture)->UnlockRect(i)) == D3D_OK);
				}

			//pMip = pMip->GetNext();
		}
		//pLayer = pLayer->GetNextLayer();
		//layer++;
	}

	//If we have a staging texture update the main texture from this and release the staging texture
	if( m_StagingTexture )
	{
		TEXTURE* destTexture = (TEXTURE*)m_CachedTexturePtr;

		if ( m_ImageType == grcImage::CUBE ) {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DCubeTexture9 *>(poTexture), static_cast<IDirect3DCubeTexture9 *>(destTexture) ));
		}
		else if ( m_ImageType == grcImage::VOLUME ) {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DVolumeTexture9 *>(poTexture), static_cast<IDirect3DVolumeTexture9 *>(destTexture) ));
		}
		else {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DTexture9 *>(poTexture), static_cast<IDirect3DTexture9 *>(destTexture) ));
		}

		ASSERT_ONLY(u32 refCount = )((TEXTURE*)m_StagingTexture)->Release();
		Assert( refCount == 0 );
		m_StagingTexture = NULL;

	}

	return true;
}

#pragma warning( push )
#pragma warning( disable : 4701 )		// warns about desc being used without being initialized

bool grcTextureDX9::LockRectStaticDX9(grcDeviceTexture* pTexture, void* PAGING_ONLY(pBackingStore), u8 imageType, int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags)
{
	D3DLOCKED_RECT	rect;
	bool isLocked = false;
	D3DSURFACE_DESC desc;

#if STALL_DETECTION
		sysTimer oTime;
		oTime.Reset();
#endif // STALL_DETECTION

	grcDeviceTexture* poTexture = pTexture;

	u32 uFlags = D3DLOCK_NOSYSLOCK;
	if ( uLockFlags == grcsRead)		// only readonly if just read set
		uFlags |= D3DLOCK_READONLY;

#if __PAGING
	if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED)
	{
		if (pBackingStore != NULL)
		{
			poTexture = (grcDeviceTexture*)pBackingStore;
		}
	}
#endif

	Assert(imageType != grcImage::VOLUME);
	if ( imageType == grcImage::CUBE ) {
		Assert(
			D3DCUBEMAP_FACE_NEGATIVE_X==D3DCUBEMAP_FACE_POSITIVE_X+1 &&
			D3DCUBEMAP_FACE_POSITIVE_Y==D3DCUBEMAP_FACE_NEGATIVE_X+1 &&
			D3DCUBEMAP_FACE_NEGATIVE_Y==D3DCUBEMAP_FACE_POSITIVE_Y+1 &&
			D3DCUBEMAP_FACE_POSITIVE_Z==D3DCUBEMAP_FACE_NEGATIVE_Y+1 &&
			D3DCUBEMAP_FACE_NEGATIVE_Z==D3DCUBEMAP_FACE_POSITIVE_Z+1 &&
			"D3D Cubemap face defines are no longer sequential!");
		Assert( "Layer is invalid" && layer >= 0 && layer < 6 );
		if ( SUCCEEDED(static_cast<IDirect3DCubeTexture9 *>(poTexture)->LockRect((D3DCUBEMAP_FACES) layer, mipLevel, &rect, NULL, uFlags)) ) {
			isLocked = true;
			// NOTE: Does "Level" mean miplevel or face layer????
			static_cast<IDirect3DCubeTexture9 *>(poTexture)->GetLevelDesc(mipLevel,&desc);
		}
	}
	else { 
		if ( SUCCEEDED(static_cast<IDirect3DTexture9 *>(poTexture)->LockRect(mipLevel, &rect, NULL, uFlags)) ) {
			isLocked = true;
			static_cast<IDirect3DTexture9 *>(poTexture)->GetLevelDesc(mipLevel,&desc);
		}
	}

#if STALL_DETECTION
		if (oTime.GetMsTime() > 5.0f)
		{
			grcWarningf("Texture %s Lock took %f milliseconds", GetName(), oTime.GetMsTime());
		}
#endif // STALL_DETECTION
	
	if (pTexture && isLocked ) {
		lock.MipLevel = mipLevel;
		lock.Base = rect.pBits;
		lock.Pitch = rect.Pitch;
		lock.BitsPerPixel = grcTextureFactoryDX9::GetBitsPerPixelStaticDX9( desc.Format );
		AssertMsg( lock.BitsPerPixel != 0, "Texture format is not recognised by lock" );
		
		lock.Width = desc.Width;
		lock.Height = desc.Height;
		lock.RawFormat = desc.Format;
		lock.Layer = layer;
		return true;
	}
	else
		return false;
}
#pragma warning( pop )


void grcTextureDX9::UnlockRectStaticDX9(grcDeviceTexture* pTexture, void* PAGING_ONLY(pBackingStore), u8 imageType, const grcTextureLock &lock)
{
#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	TEXTURE* poTexture = (TEXTURE*)pTexture;

#if __PAGING
	if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED)
	{
		if (pBackingStore != NULL)
		{
			poTexture = (TEXTURE*)pBackingStore;
		}
	}
#endif

	if ( imageType == grcImage::CUBE ) {
		CHECK_HRESULT(static_cast<IDirect3DCubeTexture9 *>(poTexture)->UnlockRect((D3DCUBEMAP_FACES) lock.Layer, lock.MipLevel));
	}
	else { 
		CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(poTexture)->UnlockRect(lock.MipLevel));
	}

#if __PAGING
	if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED)
	{
		if (pBackingStore != NULL)
		{
			if FAILED(GRCDEVICE.GetCurrent()->UpdateTexture(poTexture, (TEXTURE*)pTexture))
			{
				Quitf("Failed Texture Update");
			}
		}
	}
#endif

#if STALL_DETECTION
	if (oTime.GetMsTime() > 5.0f)
	{
		grcWarningf("Texture %s Unlock took %f milliseconds", GetName(), oTime.GetMsTime());
	}
#endif // STALL_DETECTION
}



grcTextureDX9::~grcTextureDX9	(void)
{
#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	m_nMipCount = m_nMipCount + m_CutMipLevels;
	m_Width <<= m_CutMipLevels;
	m_Height <<= m_CutMipLevels;
	m_nMipStride <<= m_CutMipLevels;

	UnlinkFromChain();
#if __PAGING
	physical_delete(m_BackingStore);
#endif

	GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
	m_CachedTexturePtr = NULL;

#if STALL_DETECTION
	if (oTime.GetMsTime() > 5.0f)
	{
		grcWarningf("Texture Destroy took %f milliseconds", oTime.GetMsTime());
	}
#endif // STALL_DETECTION

	if( m_StagingTexture )
	{
		//The staging texture shouldnt really still exist at this stage, it should have been released after the main texture was setup.
		Assert( false );
		ASSERT_ONLY(u32 refCount = )((TEXTURE*)m_StagingTexture)->Release();
		Assert( refCount == 0 );
		m_StagingTexture = NULL;
	}
}

bool grcTextureDX9::CopyTo(grcImage* pImage, bool bInvert)
{
	if (pImage == NULL)
	{
		return false;
	}
	
	if ((GetLayerCount() != (int)pImage->GetLayerCount())	||
		(GetWidth() != pImage->GetWidth())					||
		(GetHeight() != pImage->GetHeight())				||
		(GetMipMapCount() != (pImage->GetExtraMipCount() + 1)))
	{
		return false;
	}

	bool bResult = false;

	grcTextureLock oLock;

	if (LockRect(0, 0, oLock))
	{
		u8* pSrc = (u8*)oLock.Base;
		u8* pDst = (u8*)pImage->GetBits();

		if (bInvert)
		{
			pSrc = pSrc + (oLock.Pitch * (GetHeight() - 1));
			oLock.Pitch *= -1;
		}

		int	mipSize = m_nMipStride * GetHeight();
		sysMemCpy(pDst, pSrc, mipSize);

		UnlockRect(oLock);

		bResult = true;
	}
	else
	{
		Assert(false && "grcTexturePC::CopyTo - Could not lock texture surface");
	}
	return bResult;
}


u32 grcRenderTargetDX9::GetRequiredMemory() const
{
	// Only currently only has 1 mip level for render targets	
	return /*(m_Lockable ? 2 : 1) * */ (m_Multisample ? (m_Multisample + 1) : 1 ) * GetWidth() * GetHeight() * m_BitsPerPix / 8 * ((m_Type == grcrtCubeMap) ? 6 : 1); 
}


grcTextureDX9::grcTextureDX9(datResource &rsc) : grcTexturePC(rsc) 
{
#if __PAGING
	rsc.PointerFixup(m_BackingStore);

	if (m_BackingStore && !datResource_IsDefragmentation)
		CreateFromBackingStore();

	if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_MANAGED)
	{
		m_BackingStore = NULL;
	}
#endif
	SetPrivateData();
	m_CachedTexturePtr = GetTexturePtr();

	m_StagingTexture = NULL;
}


void grcTextureDX9::CreateFromBackingStore(bool PAGING_ONLY(bRecreate)) 
{
#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION



#if __PAGING	
	const u8 *pSource = NULL;

	if (bRecreate == false)
	{
		pSource = (u8*) m_BackingStore;
		m_CutMipLevels = (u8)GetMipLevelScaleQuality(m_ImageType, m_Width, m_Height, m_nMipCount, m_nFormat);
		for (u32 i = 0; i < m_CutMipLevels; i++)
		{
			int	mipSize = (m_nMipStride >> i) * (m_Height >> i);
			pSource += mipSize; // Skip Mip Level
			m_nMipCount--;		// Reduce Number of Levels
		}	
		m_Width >>= m_CutMipLevels;
		m_Height >>= m_CutMipLevels;
		m_nMipStride >>= m_CutMipLevels;

		// Another potential way to save memory at the cost of visual memory 
		// Very application specific
		/*
		if (sysMemTotalMemory() < (u64)(1.4f * 1024 * 1024 * 1024))
		{
			m_nMipCount = 1;
		}
		*/
	}

	// Clamp to dimension of 4 for NVidia driver issue
	if ((( m_nFormat == D3DFMT_DXT1 ) || ( m_nFormat == D3DFMT_DXT5 )) && (m_nMipCount > 1))
	{
		s32 iMaxMips = _FloorLog2(Min(m_Width, m_Height));
		if (iMaxMips > 1)
		{
			m_nMipCount = (u8)(((s32)m_nMipCount > (iMaxMips - 2)) ? Max((iMaxMips - 2),1) : m_nMipCount);
		}
	}
	grcDevice::Result uReturnCode = GRCDEVICE.CreateTexture(m_Width, m_Height, 1, m_nMipCount, 0, m_nFormat, GRCDEVICE.GetTextureResourcePool(), m_ImageType, &m_CachedTexturePtr);
	SetPrivateData();

	//When using unmanaged pools we need to create a staging texture in System mem to copy the data into then update the main texture from this.
	if ( GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED )
	{
		GRCDEVICE.CreateTexture(m_Width, m_Height, 1, m_nMipCount, 0, m_nFormat, RESOURCE_SYSTEMMEM, m_ImageType, &m_StagingTexture);
	}

	m_CachedTexturePtr = (TEXTURE*)m_CachedTexturePtr;

#if 1 //this should never happen, but if it does, print out some stuff
	Assert(m_CachedTexturePtr); 
	Assert(uReturnCode == D3D_OK);
	if ((m_CachedTexturePtr==NULL) || (uReturnCode != D3D_OK))
	{
		Printf("CreateTexture() failed return code:0x%08x- m_Width=%d m_Height=%d m_nMipCount=%d m_nFormat=%d m_ImageType=%d", uReturnCode, m_Width, m_Height, m_nMipCount, m_nFormat, m_ImageType);
		Quitf("D3D Error - Failed to create texture - Please restart the game");
	}
#endif

#if 0 // Arthur change here // ndef FINAL
	static bool bVisualMipMap = true;

	if (bVisualMipMap)
	{
		typedef struct _Color565
		{
			u16	uRed	: 5;
			u16 uGreen	: 6;
			u16 uBlue	: 5;
		} Color565;
		
		Color565 auColors[] = { { 31, 63, 31 }, { 31, 63, 30 }, // White	// Level 0
								{ 31, 63, 0  }, { 31, 62, 0  }, // Yellow	// Level 1
								{  0, 63, 31 }, {  0, 63, 30 }, // Cyan		// Level 2
								{  0, 63,  0 }, {  0, 62,  0 }, // Green	// Level 3
								{ 31,  0,  0 }, { 30,  0,  0 }, // Red		// Level 4
								{  0,  0, 31 }, {  0,  0, 30 }, // Blue		// Level 5
								{  0,  0, 31 }, {  0,  0, 30 }, // Blue		// Level 6
								{  0,  0, 31 }, {  0,  0, 30 }, // Blue		// Level 7
								{  0,  0, 31 }, {  0,  0, 30 }, // Blue		// Level 8
								{  0,  0, 31 }, {  0,  0, 30 }, // Blue		// Level 9
								{  0,  0, 31 }, {  0,  0, 30 }, // Blue		// Level 10
								{  0,  0, 31 }, {  0,  0, 30 }, // Blue		// Level 11
								{  0,  0, 31 }, {  0,  0, 30 }  // Blue		// Level 12
							  };
		const u32 uNumColors = sizeof(auColors) / sizeof(Color565);
		/*				
		// Randomize Colors
		for (u32 uColor = 0; uColor < uNumColors; uColor++)
		{
			Color565 &oColor = auColors[uColor];
			oColor.uRed = rand() % 31;
			oColor.uGreen = rand() % 63;
			oColor.uBlue = rand() % 31;
		}
		*/

		u8* pbyImageData = (u8*)pSource;

		// Only support DXT1 and DXT5
		if (( m_nFormat == D3DFMT_DXT1 ) || ( m_nFormat == D3DFMT_DXT5 ))
		{
			static u32 uViewMipLevels = 1; // uNumColors; // 1
			u32 uTextureHeight = GetHeight();
			u32 uTextureWidth  = GetWidth();
			u32 uMipLevels	   = Max(1U, Min(uViewMipLevels, (u32)GetMipMapCount()));
			const u32 uBlockSize = (m_nFormat == D3DFMT_DXT1) ? 8 : 16;
		
			for (u32 uMipLevel = 0; uMipLevel < uMipLevels; uMipLevel++)
			{
				for (u32 uHeight = 0; uHeight < uTextureHeight; uHeight += 8)
				{
					for (u32 uWidth = 0; uWidth < uTextureWidth; uWidth += 8)
					{
						Color565* puColor = (Color565*)(&pbyImageData[((uTextureWidth / 4) * (uHeight / 4) + // Height Offset
												 					   (uWidth / 4)) * // Width Offset
																	   uBlockSize]); // Size of DXT1 or DXT5 Block in bytes (2 * u16 + 4 by 4 * 2 bit blocks) + 4 Bytes for alpha on DXT5
						*puColor = auColors[uMipLevel * 2 + 0];
						puColor++;
						*puColor = auColors[uMipLevel * 2 + 1];
					}
				}

				pbyImageData += (uTextureWidth * uTextureHeight / 2);
				uTextureHeight = Max(uTextureHeight >> 1, 4U);
				uTextureWidth = Max(uTextureWidth >> 1, 4U);
			}
		}		
	}
#endif // FINAL

	TEXTURE *poTexture = (TEXTURE*)( m_StagingTexture ? m_StagingTexture : m_CachedTexturePtr);
	if (poTexture == NULL)
	{
		return;
	}

	if (pSource != NULL)
	{
		D3DLOCKED_RECT	rect;
		const int layerCount = m_LayerCount+1;
		for (int layer = 0; layer < layerCount; ++layer)
		{
			for	(int i = 0; i < m_nMipCount; i++)
			{
				Assert(m_ImageType != grcImage::VOLUME);
				if ( m_ImageType == grcImage::CUBE ) 
				{
					AssertVerify(static_cast<IDirect3DCubeTexture9 *>(poTexture)->LockRect((D3DCUBEMAP_FACES) layer, i, &rect, 0, 0) == D3D_OK);
				}
				else 
				{
					AssertVerify(static_cast<IDirect3DTexture9 *>(poTexture)->LockRect(i, &rect, NULL, D3DLOCK_NOSYSLOCK) == D3D_OK);
				}

				int	mipSize = (m_nMipStride >> i) * (m_Height >> i);

				sysMemCpy(rect.pBits, pSource, mipSize);
				
				pSource += mipSize;
				
				if ( m_ImageType == grcImage::CUBE ) 
				{
					CHECK_HRESULT(static_cast<IDirect3DCubeTexture9 *>(poTexture)->UnlockRect((D3DCUBEMAP_FACES) layer, i));
				}
				else 
				{
					CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(poTexture)->UnlockRect(i));
				}
			}
		}
	}

	if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED)
	{
		D3DLOCKED_RECT	rect;
		HRESULT			hResult;

		// Note: The LockRect/UnlockRect calls fix an issue with corrupted textures appearing on screen
		//        when running with unmanaged resources. Yes, it doesn't make sense, but the cause of the
		//        problem likely stems from some lower level optimization that is out of our control.

		if ( m_ImageType == grcImage::CUBE ) {
			CHECK_HRESULT(static_cast<IDirect3DCubeTexture9 *>(poTexture)->LockRect((D3DCUBEMAP_FACES) 0, 0, &rect, NULL, 0));
			CHECK_HRESULT(static_cast<IDirect3DCubeTexture9 *>(poTexture)->UnlockRect((D3DCUBEMAP_FACES) 0, 0));
			hResult = GRCDEVICE.GetCurrent()->UpdateTexture(poTexture, (TEXTURE*)m_CachedTexturePtr);
		}
		else { 
			CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(poTexture)->LockRect(0, &rect, NULL, D3DLOCK_NOSYSLOCK));
			CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(poTexture)->UnlockRect(0));
			hResult = GRCDEVICE.GetCurrent()->UpdateTexture(poTexture, (TEXTURE*)m_CachedTexturePtr);
		}

		if FAILED( hResult )
		{
			Quitf("Failed Texture Update");
		}
	}

	//If we have a staging texture update the main texture from this and release the staging texture
	if( m_StagingTexture )
	{
		TEXTURE* destTexture = (TEXTURE*)m_CachedTexturePtr;

		if ( m_ImageType == grcImage::CUBE ) {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DCubeTexture9 *>(poTexture), static_cast<IDirect3DCubeTexture9 *>(destTexture) ));
		}
		else if ( m_ImageType == grcImage::VOLUME ) {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DVolumeTexture9 *>(poTexture), static_cast<IDirect3DVolumeTexture9 *>(destTexture) ));
		}
		else {
			CHECK_HRESULT(GRCDEVICE.GetCurrent()->UpdateTexture( static_cast<IDirect3DTexture9 *>(poTexture), static_cast<IDirect3DTexture9 *>(destTexture) ));
		}

		ASSERT_ONLY(u32 refCount = )((TEXTURE*)m_StagingTexture)->Release();
		Assert( refCount == 0 );
		m_StagingTexture = NULL;
	}
#endif // __PAGING

#if STALL_DETECTION
	if (oTime.GetMsTime() > 5.0f)
	{
		grcWarningf("Texture Create took %f milliseconds", oTime.GetMsTime());
	}
#endif // STALL_DETECTION
}


void grcTextureDX9::DeviceLost() 
{
	if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED)
	{
		GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
		m_CachedTexturePtr = NULL;
	}
}


void grcTextureDX9::DeviceReset() 
{
	if (GRCDEVICE.GetTextureResourcePool() == RESOURCE_UNMANAGED)
	{
		CreateFromBackingStore(true);
	}
}

grcTexture::ChannelBits grcTextureDX9::FindUsedChannels() const
{
	u32 fmt = m_nFormat;

	ChannelBits bits(false);

	switch(fmt)
	{
		// RGBA
	case DDS_D3DFMT_DXT1:
	case DDS_D3DFMT_DXT2:
	case DDS_D3DFMT_DXT3:
	case DDS_D3DFMT_DXT4:
	case DDS_D3DFMT_DXT5:
	case DDS_D3DFMT_A16B16G16R16F:
	case DDS_D3DFMT_A32B32G32R32F:
	case DDS_D3DFMT_A8R8G8B8:
	case DDS_D3DFMT_A1R5G5B5:
	case DDS_D3DFMT_A4R4G4B4:
	case DDS_D3DFMT_A8R3G3B2:
	case DDS_D3DFMT_A2B10G10R10:
	case DDS_D3DFMT_A8B8G8R8:
	case DDS_D3DFMT_A2R10G10B10:
	case DDS_D3DFMT_A16B16G16R16:
	case DDS_D3DFMT_A8L8:
	case DDS_D3DFMT_A4L4:
		bits.Set(CHANNEL_RED);
		bits.Set(CHANNEL_GREEN);
		bits.Set(CHANNEL_BLUE);
		bits.Set(CHANNEL_ALPHA);
		break;

		// RGB
	case DDS_D3DFMT_R8G8B8:
	case DDS_D3DFMT_X8R8G8B8:
	case DDS_D3DFMT_R5G6B5:
	case DDS_D3DFMT_X1R5G5B5:
	case DDS_D3DFMT_R3G3B2:
	case DDS_D3DFMT_X4R4G4B4:
	case DDS_D3DFMT_X8B8G8R8:
	case DDS_D3DFMT_L8:
	case DDS_D3DFMT_L16:
		bits.Set(CHANNEL_RED);
		bits.Set(CHANNEL_GREEN);
		bits.Set(CHANNEL_BLUE);
		break;

		// R+G
	case DDS_D3DFMT_G16R16F:
	case DDS_D3DFMT_G32R32F:
	case DDS_D3DFMT_G16R16:
	case DDS_D3DFMT_G8R8: // custom
		bits.Set(CHANNEL_RED);
		bits.Set(CHANNEL_GREEN);
		break;

		// Alpha
	case DDS_D3DFMT_A8:
		bits.Set(CHANNEL_ALPHA);
		break;

	case DDS_D3DFMT_R16F:
	case DDS_D3DFMT_R32F:
	case DDS_D3DFMT_R16: // custom
	case DDS_D3DFMT_R8: // custom
		bits.Set(CHANNEL_RED);
		break;

		// Depth
	case DDS_D3DFMT_D16:
	case DDS_D3DFMT_D24X8:
	case DDS_D3DFMT_D32:
	case DDS_D3DFMT_D32F_LOCKABLE:
		bits.Set(CHANNEL_DEPTH);
		break;

	case DDS_D3DFMT_D24S8:
	case DDS_D3DFMT_D24FS8:
	case DDS_D3DFMT_D15S1:
	case DDS_D3DFMT_D24X4S4:
		bits.Set(CHANNEL_DEPTH);
		bits.Set(CHANNEL_STENCIL);
		break;

	case DDS_D3DFMT_G8R8_G8B8: // alternate pixels carry different color components
	case DDS_D3DFMT_R8G8_B8G8: // alternate pixels carry different color components
	default:
		grcWarningf("Dont' know what channels are used in D3DFORMAT %d", m_nFormat);
		break;
	}

	return bits;

}


//=============================================================================
// grcRenderTargetDX9
//=============================================================================
grcRenderTargetDX9::grcRenderTargetDX9(const char *name,grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params) :
	grcRenderTargetPC()
{
	m_Name = StringDuplicate(name);
	//m_Texture = 0;
	m_CachedTexturePtr = NULL;
	m_Surface = 0;
	m_OffscreenSurface = 0;
	m_ResolvedTarget = NULL;
	m_MipLevels = 1;
	ReCreate(type, width, height, bpp, params);
}

grcRenderTargetDX9::grcRenderTargetDX9(const char *name, const grcTextureObject *pTexture) : grcRenderTargetPC()
{
	AssertMsg(0, "No Support");
	name;
	pTexture;
}

grcRenderTargetDX9::~grcRenderTargetDX9() {
	grcTextureFactory::UnregisterRenderTarget(this);

	sm_TotalMemory -= GetRequiredMemory();
	if ((m_Width != m_Height) &&
		(m_Width >= GRCDEVICE.GetWidth()) && 
		(m_Height >= GRCDEVICE.GetHeight()))
	{
		sm_TotalStereoMemory -= GetRequiredMemory();
	}

	GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
	m_CachedTexturePtr = NULL;
	GRCDEVICE.DeleteSurface(m_Surface);
	m_Surface = NULL;

	if( m_OffscreenSurface )
		GRCDEVICE.DeleteSurface(m_OffscreenSurface);
	
	// If there's a texture factory, tell it we're leaving.
	// (need this check because some render targets don't get destroyed until after the factory is gone)
	if (grcTextureFactory::HasInstance())
		static_cast<grcTextureFactoryDX9 &> (grcTextureFactory::GetInstance()).RemoveRenderTarget(this);
}

void grcRenderTargetDX9::SetPrivateData()
{
	grcTexture* pTex = this;
	if (m_CachedTexturePtr)
		CHECK_HRESULT(((IDirect3DTexture9*)m_CachedTexturePtr)->SetPrivateData(TextureBackPointerGuid,&pTex,sizeof(pTex),0));
}

bool grcRenderTargetDX9::IsValid() const 
{ 
#if __FINAL || !USE_RESOURCE_CACHE
	return true;
#else
	return (GetTexturePtr() != NULL) ? (grcResourceCache::GetInstance().Validate((grcDeviceTexture*)GetTexturePtr())) : grcResourceCache::GetInstance().Validate((grcDeviceSurface*)GetSurfacePtr());
#endif // __FINAL
}

void grcRenderTargetDX9::ReCreate(grcRenderTargetType type,int width,int height,int bpp,grcTextureFactory::CreateParams *params) {
	if(m_CachedTexturePtr)
	{
		sm_TotalMemory -= GetRequiredMemory();
		if ((width != height) &&
			(width >= GRCDEVICE.GetWidth()) && 
			(height >= GRCDEVICE.GetHeight()))
		{
			sm_TotalStereoMemory -= GetRequiredMemory();
		}

		GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
	}
	if(m_Surface)
	{
		GRCDEVICE.DeleteSurface(m_Surface);
	}
	if(m_OffscreenSurface)
	{
		GRCDEVICE.DeleteSurface(m_OffscreenSurface);
	}
	
	m_Width = (u16) width;
	m_Height = (u16) height;
	m_Type = type;
	//m_Texture = 0;
	m_CachedTexturePtr = NULL;
	m_Surface = 0;
	m_OffscreenSurface = 0;
	m_Multisample = 0;
	m_MultisampleQuality = 0;

	bool useFloat = params? params->UseFloat : false;
	if (params && params->CreateAABuffer)
	{
		m_Multisample = (u8)params->Multisample;
		m_MultisampleQuality = (u8)params->MultisampleQuality;
	}

	grcTextureFormat tf = params? params->Format : grctfNone;
	if (tf == grctfNone)
		tf = useFloat? (bpp==32? grctfR32F : grctfR16F) : (bpp==32? grctfA8R8G8B8 : grctfR5G6B5);
	m_Format = (u8) tf;
	m_BitsPerPix = (u8) bpp;
	m_Lockable = params? params->Lockable : false;

	CreateSurface();

	m_ResolvedTarget = m_CachedTexturePtr;
}

XPARAM(debugshaders);

void grcRenderTargetDX9::CreateSurface() {
	D3DFORMAT eFormat = static_cast<D3DFORMAT>(static_cast<grcTextureFactoryPC&>(grcTextureFactoryPC::GetInstance()).Translate((grcTextureFormat)m_Format));

	u32 multisample_quality = 0;
	if (m_Type == grcrtPermanent) {
		Assert(!( m_Multisample && m_Lockable ));
		if (m_Multisample) 
		{
#if !__FINAL
			IDirect3D9* pDevice;
			if (GRCDEVICE.GetCurrent()->GetDirect3D(&pDevice) == D3D_OK)
			{
				DWORD uQualityLevels;
				HRESULT hRes = pDevice->CheckDeviceMultiSampleType(GRCDEVICE.GetAdapterOrdinal(), D3DDEVTYPE_HAL, eFormat, 1, (D3DMULTISAMPLE_TYPE)m_Multisample, &uQualityLevels);
				if (hRes != D3D_OK)
				{
					grcErrorf("Error Unsupported Multisample Format Error %x Format %x", hRes, eFormat);
				}
				else
				{
					Assert(m_MultisampleQuality < uQualityLevels);
				}
				pDevice->Release();
			}
#endif // !__FINAL

			multisample_quality = m_MultisampleQuality;

			if (FAILED(GRCDEVICE.GetCurrent()->CreateRenderTarget(m_Width,m_Height,eFormat,(D3DMULTISAMPLE_TYPE)m_Multisample,multisample_quality,false,(SURFACE**)&m_Surface,NULL)))
				Quitf("Unable to create %d x %d color multisampled render target",m_Width,m_Height);
		}

		if (m_Lockable) {
		
#if __WIN32PC && __BANK
			// This is a terrible, terrible hack -- can't create 128 bit surfaces with reference rasterizer, and gpu ptfx needs a valid rendertarget
			// even if it doesn't actually work as expected.
			if (eFormat == D3DFMT_A32B32G32R32F && PARAM_debugshaders.Get())
				eFormat = D3DFMT_A8R8G8B8;
#endif

			if (FAILED(GRCDEVICE.GetCurrent()->CreateRenderTarget(m_Width,m_Height,eFormat,D3DMULTISAMPLE_NONE,1,true,(SURFACE**)&m_Surface,NULL)))
				Quitf("Unable to create %d x %d color resolve render target",m_Width,m_Height);
			
			if (FAILED(GRCDEVICE.GetCurrent()->CreateOffscreenPlainSurface(m_Width,m_Height,eFormat,D3DPOOL_SYSTEMMEM,(SURFACE**)&m_OffscreenSurface,NULL)))
				Quitf("Unable to create %d x %d color offscreen render target",m_Width,m_Height);
		}

		bool bDepthUsage = false;
		if (m_Format == grctfD24S8)
		{
			eFormat = (D3DFORMAT)GetDepthTextureFormat();
			bDepthUsage = true;
		}

		HRESULT hResult;
		if ((hResult = GRCDEVICE.GetCurrent()->CreateTexture(m_Width,m_Height,1,(bDepthUsage) ? D3DUSAGE_DEPTHSTENCIL : D3DUSAGE_RENDERTARGET,eFormat,D3DPOOL_DEFAULT,(IDirect3DTexture9**)&m_CachedTexturePtr,NULL)) != D3D_OK)
		{
			Quitf("Unable to create %d x %d color render target",m_Width,m_Height);
		}

		SetPrivateData();
		m_CachedTexturePtr = (TEXTURE*)m_CachedTexturePtr;
	}
	else if (m_Type == grcrtDepthBuffer || m_Type == grcrtShadowMap) {
		if (FAILED(GRCDEVICE.GetCurrent()->CreateDepthStencilSurface(m_Width,m_Height,(m_BitsPerPix==32?D3DFMT_D24S8:D3DFMT_D16),(D3DMULTISAMPLE_TYPE)m_Multisample,multisample_quality,false,(SURFACE**)&m_Surface,NULL)))
			Quitf("Unable to create %d x %d depth render target",m_Width,m_Height);
	}
	else if( m_Type == grcrtCubeMap )
	{
		GRCDEVICE.CreateTexture(m_Width, m_Height, 1, 1, D3DUSAGE_RENDERTARGET, eFormat, D3DPOOL_DEFAULT, grcImage::CUBE, (grcDeviceTexture**)&m_CachedTexturePtr);
		SetPrivateData();
		//m_CachedTexturePtr = (TEXTURE*)m_CachedTexturePtr;
	}
	else
	{
		grcWarningf("Unsupported render target type %d\n", m_Type);
	}

	sm_TotalMemory += GetRequiredMemory();

	if ((m_Width != m_Height) &&
		(m_Width >= GRCDEVICE.GetWidth()) && 
		(m_Height >= GRCDEVICE.GetHeight()))
	{
		sm_TotalStereoMemory += GetRequiredMemory();
	}
}

u32 grcRenderTargetDX9::GetDepthTextureFormat()
{
	IDirect3D9* pDevice;

	if (FAILED(GRCDEVICE.GetCurrent()->GetDirect3D(&pDevice)))
	{
		Printf("Unable to retrieve D3D Device");
		Quitf("D3D Error - Unable to retrieve D3D Device. Please re-boot your system");
		// return 0;
	}

	u32 uFormat = (u32)D3DFMT_D24S8;

	if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
												D3DDEVTYPE_HAL,
												D3DFMT_X8R8G8B8,
												D3DUSAGE_DEPTHSTENCIL,
												D3DRTYPE_TEXTURE,
												(D3DFORMAT)(MAKEFOURCC('I','N','T','Z')))))
	{
		uFormat = (u32)(MAKEFOURCC('I','N','T','Z'));
	}
	else if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
													 D3DDEVTYPE_HAL,
													 D3DFMT_X8R8G8B8,
													 D3DUSAGE_DEPTHSTENCIL,
													 D3DRTYPE_TEXTURE,
													 (D3DFORMAT)(MAKEFOURCC('R','A','W','Z')))))
	{
		uFormat = (u32)(MAKEFOURCC('R','A','W','Z'));
	}
	else if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
													 D3DDEVTYPE_HAL,
													 D3DFMT_X8R8G8B8,
													 D3DUSAGE_DEPTHSTENCIL,
													 D3DRTYPE_TEXTURE,
													 (D3DFORMAT)(MAKEFOURCC('D','F','2','4')))))
	{
		uFormat = (u32)(MAKEFOURCC('D','F','2','4'));
	}
	else if ( SUCCEEDED( pDevice->CheckDeviceFormat( GRCDEVICE.GetAdapterOrdinal(),
													 D3DDEVTYPE_HAL,
													 D3DFMT_X8R8G8B8,
													 D3DUSAGE_DEPTHSTENCIL,
													 D3DRTYPE_TEXTURE,
													 (D3DFORMAT)(MAKEFOURCC('D','F','1','6')))))
	{
		uFormat = (u32)(MAKEFOURCC('D','F','1','6'));
	}
	pDevice->Release();

	return uFormat;
}

grcTextureObject*	grcRenderTargetDX9::GetTexturePtr()
{
	//Assert(IsResolved() || !m_Multisample); // Can't do this as you can use as texture and surface at the same time - Can help to track down unresolved usage though
	return m_CachedTexturePtr;
}

const grcTextureObject*	grcRenderTargetDX9::GetTexturePtr() const 
{
	//Assert(IsResolved() || !m_Multisample); // Can't do this as you can use as texture and surface at the same time - Can help to track down unresolved usage though
	return m_CachedTexturePtr;
}

grcDeviceSurface* grcRenderTargetDX9::GetSurfacePtr	(void)
{
	return (SURFACE*)(m_Surface);
}

const grcDeviceSurface* grcRenderTargetDX9::GetSurfacePtr	(void) const
{
	return (SURFACE*)(m_Surface);
}

grcDeviceSurface* grcRenderTargetDX9::GetOffscreenSurfacePtr	(void)
{
	return (SURFACE*)(m_OffscreenSurface);
}

const grcDeviceSurface* grcRenderTargetDX9::GetOffscreenSurfacePtr	(void) const
{
	return (SURFACE*)(m_OffscreenSurface);
}

void grcRenderTargetDX9::DeviceLost() {
	sm_TotalMemory -= GetRequiredMemory();
	if ((m_Width != m_Height) &&
		(m_Width >= GRCDEVICE.GetWidth()) && 
		(m_Height >= GRCDEVICE.GetHeight()))
	{
		sm_TotalStereoMemory -= GetRequiredMemory();
	}

	GRCDEVICE.DeleteTexture(m_CachedTexturePtr);
	m_CachedTexturePtr = NULL;
	GRCDEVICE.DeleteSurface(m_Surface);
	GRCDEVICE.DeleteSurface(m_OffscreenSurface);
}


void grcRenderTargetDX9::DeviceReset() {
	CreateSurface();
}

bool grcRenderTargetDX9::Copy(const grcTexture* pSource, s32  UNUSED_PARAM(dstSliceIndex), s32  UNUSED_PARAM(dstMipIndex), s32  UNUSED_PARAM(srcSliceIndex), s32  UNUSED_PARAM(srcMipIndex))
{
	Assert(pSource != NULL);
	IDirect3DTexture9* poSrc = (IDirect3DTexture9*)(pSource->GetTexturePtr());
	IDirect3DTexture9* poDst = static_cast<IDirect3DTexture9 *>(GetTexturePtr());	
	Assert(poSrc != NULL);
	Assert(poDst != NULL);

	IDirect3DSurface9* poSrcSurface;
	IDirect3DSurface9* poDstSurface;
	if (FAILED(poSrc->GetSurfaceLevel(0, &poSrcSurface)))
	{
		Printf("Failed to copy luminance surface");
		Quitf("D3D Error - Failed to copy surface - Please restart the game");
	}
	if (FAILED(poDst->GetSurfaceLevel(0, &poDstSurface)))
	{
		Printf("Failed to copy luminance surface");
		Quitf("D3D Error - Failed to copy surface - Please restart the game");
	}

	bool bFailed = FAILED(GRCDEVICE.GetCurrent()->StretchRect(poSrcSurface, NULL, poDstSurface, NULL, D3DTEXF_NONE));	
	poSrcSurface->Release();
	poDstSurface->Release();
	return !bFailed;
}

bool grcRenderTargetDX9::CopyTo(grcImage* pImage, bool bInvert, u32 uPixelOffset)
{
	if (pImage == NULL)
	{
		return false;
	}
	
	if ((GetLayerCount() != (int)pImage->GetLayerCount())	||
		(GetWidth() != pImage->GetWidth())					||
		(GetMipMapCount() != (pImage->GetExtraMipCount() + 1)))
	{
		return false;
	}

	if (((GetWidth() * GetHeight()) - (int)uPixelOffset) < (pImage->GetWidth() * pImage->GetHeight()))
	{
		return false;
	}

	bool bResult = false;

	IDirect3DSurface9* pSurface = static_cast<IDirect3DSurface9*>(GetSurfacePtr());

	if (pSurface)
	{
		D3DLOCKED_RECT oRect;

		if (SUCCEEDED(pSurface->LockRect(&oRect, NULL, D3DLOCK_READONLY)))
		{
			u8* pSrc = (u8*)oRect.pBits;
			u8* pDst = (u8*)pImage->GetBits();

			u32 uOffset = (uPixelOffset / (u32)GetWidth()) * (u32)(oRect.Pitch);

			if (bInvert)
			{
				pSrc = pSrc + (oRect.Pitch * (GetHeight() - 1)) - uOffset;
				oRect.Pitch *= -1;
			}
			else
			{
				pSrc = pSrc + uOffset;
			}

			for (u32 uRow = 0; uRow < (u32)pImage->GetHeight(); uRow++)
			{
				sysMemCpy(pDst, pSrc, pImage->GetStride());

				pSrc += oRect.Pitch;
				pDst += pImage->GetStride();
			}

			CHECK_HRESULT(pSurface->UnlockRect());

			bResult = true;
		}
		else
		{
			Assert(false && "grcRenderTargetPC::CopyTo - Could not lock rendertarget surface (get FK)");
		}
	}
	else
	{
		Assert(false && "grcRenderTargetPC::CopyTo - No surface for render target (get FK)");
	}

	return bResult;
}

void grcRenderTargetDX9::Resolve(grcRenderTarget*, int UNUSED_PARAM(destSliceIndex))
{
	if (GetResolved() == m_CachedTexturePtr)
		return;
	
	//grcErrorf("grcRenderTargetDX9::Resolve() - unimplemented function");

	IDirect3DSurface9 *dest = NULL;
	AssertVerify(CHECK_HRESULT(static_cast<IDirect3DTexture9 *>(m_CachedTexturePtr)->GetSurfaceLevel(0, &dest)) == D3D_OK);
	
	if (FAILED(GRCDEVICE.GetCurrent()->StretchRect((SURFACE*)m_Surface,NULL,dest,NULL,D3DTEXF_LINEAR)))
	{
		grcErrorf("StretchRect failed for texture %s", GetName());
	}
	else
	{
		m_ResolvedTarget = m_CachedTexturePtr;
	}
	dest->Release();
}

bool grcRenderTargetDX9::LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 flags) const
{
	Assertf(m_OffscreenSurface,"Trying to lock %s which doesn't have an offscreen representation",GetName());
	AssertMsg(mipLevel == 0,"mip level is ignored right now");
	bool result = false;

#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	if (m_OffscreenSurface)
	{
		grcTextureFactory::GetInstance().LockRenderTarget(0,this,NULL,layer,true);

		SURFACE *src;
		GRCDEVICE.GetCurrent()->GetRenderTarget( 0, &src );

		HRESULT hr = GRCDEVICE.GetCurrent()->GetRenderTargetData( src, (SURFACE*)m_OffscreenSurface );
		if( SUCCEEDED(hr) )
		{
			// Here we have data in offscreenSurface.
			D3DLOCKED_RECT rect;
			DWORD lockFlags = D3DLOCK_NOSYSLOCK;

			lockFlags |= (flags & grcsDiscard) ? D3DLOCK_DISCARD : 0;
			lockFlags |= (flags & grcsDoNotWait) ? D3DLOCK_DONOTWAIT : 0;
			lockFlags |= (flags & grcsNoDirty) ? D3DLOCK_NO_DIRTY_UPDATE : 0;
			lockFlags |= (flags & grcsRead) ? D3DLOCK_READONLY : 0;

			hr = ((SURFACE*)m_OffscreenSurface)->LockRect( &rect, NULL, lockFlags );
			if( SUCCEEDED(hr) )
			{
				D3DSURFACE_DESC desc;
				((SURFACE*)m_Surface)->GetDesc(&desc);

				lock.MipLevel = mipLevel;
				lock.Base = rect.pBits;
				lock.Pitch = rect.Pitch;
				lock.BitsPerPixel = grcTextureFactoryDX9::GetBitsPerPixelStaticDX9( desc.Format );
				AssertMsg( lock.BitsPerPixel != 0, "Texture format is not recognised by lock" );

				lock.Width = desc.Width;
				lock.Height = desc.Height;
				lock.RawFormat = desc.Format;
				lock.Layer = layer;

				result = true;
			}
		}
	}

#if STALL_DETECTION
	if (oTime.GetMsTime() > 5.0f)
	{
		grcWarningf("RenderTarget %s Lock took %f milliseconds", GetName(), oTime.GetMsTime());
	}
#endif // STALL_DETECTION

	return result;
}


void grcRenderTargetDX9::UnlockRect(const grcTextureLock &/*lock*/) const
{
	Assertf(m_OffscreenSurface,"Trying to unlock %s which doesn't have an offscreen representation",GetName());

#if STALL_DETECTION
	sysTimer oTime;
	oTime.Reset();
#endif // STALL_DETECTION

	if (m_OffscreenSurface)
	{
		CHECK_HRESULT(((SURFACE*)m_OffscreenSurface)->UnlockRect());
		grcTextureFactory::GetInstance().UnlockRenderTarget(0);
	}

#if STALL_DETECTION
	if (oTime.GetMsTime() > 5.0f)
	{
		grcWarningf("Texture %s Unlock took %f milliseconds", GetName(), oTime.GetMsTime());
	}
#endif // STALL_DETECTION
}


void grcTextureFactoryDX9::PlaceTexture(class datResource &rsc,grcTexture &tex) {
	switch (tex.GetResourceType()) {
		case grcTexture::NORMAL: ::new (&tex) grcTextureDX9(rsc); break;
		case grcTexture::RENDERTARGET: Assert(0 && "unsafe to reference a rendertarget"); break;
		case grcTexture::REFERENCE: ::new (&tex) grcTextureReference(rsc); break;
		default:
			{
				Printf("Bad resource type %d in grcTextureFactoryPC::PlaceTexture",tex.GetResourceType());
				Quitf("Invalid resource detected - Please re-install the game.");
			}
	}
}


#if __DECLARESTRUCT
void grcTextureDX9::DeclareStruct(datTypeStruct &s) {
	grcTexturePC::DeclareStruct(s);
	STRUCT_BEGIN(grcTextureDX9);
	STRUCT_END();
}
#endif

}	// namespace rage

#endif
