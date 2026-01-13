
#if RSG_DURANGO || RSG_ORBIS || (RSG_PC && __64BIT && __RESOURCECOMPILER)

#include	"config.h"
#include	"device.h"
#include	"image.h"
#include	"effect_mrt_config.h"
#include	"texture_durango.h"
#include	"texturefactory_d3d11.h"
#include	"wrapper_d3d.h"
#include	"system/memory.h"
#include	"system/param.h"
#include	"system/virtualallocator.h"
#include	"grprofile/pix.h"
#include	"diag/tracker.h"
#include	"string/string.h"
#include	"data/resource.h"
#include	"data/struct.h"

#if RSG_ORBIS
#include	 "wrapper_gnm.h"
#endif

#include "orbisdurangoresourcebase.h"

namespace rage 
{

/*======================================================================================================================================*/
/* Format helper functions.																												*/
/*======================================================================================================================================*/

// Format conversion functions in case SDK the defines change.
#if RSG_ORBIS
sce::Gnm::TileMode grcOrbisDurangoTileModeToOrbis(grcOrbisDurangoTileMode tileMode)
{
	return (sce::Gnm::TileMode)tileMode;
}

grcOrbisDurangoTileMode OrbisTogrcOrbisDurangoTileMode(sce::Gnm::TileMode tileMode)
{
	return (grcOrbisDurangoTileMode)tileMode;
}
#endif // RSG_ORBIS


#if RSG_DURANGO || (RSG_PC && __RESOURCECOMPILER && __64BIT)
XG_TILE_MODE grcOrbisDurangoTileModeToDurango(grcOrbisDurangoTileMode tileMode)
{
	return (XG_TILE_MODE)tileMode;
}

grcOrbisDurangoTileMode DurangoTogrcOrbisDurangoTileMode(XG_TILE_MODE tileMode)
{
	return (grcOrbisDurangoTileMode)tileMode;
}
#endif // RSG_DURANGO || (RSG_PC && __RESOURCECOMPILER && __64BIT)

static const GRC_TEMP_XG_FORMAT translate[] = 
{
	GRC_TEMP_XG_FORMAT_UNKNOWN,
	GRC_TEMP_XG_FORMAT_B5G6R5_UNORM,
	GRC_TEMP_XG_FORMAT_B8G8R8A8_UNORM, // grctfA8R8G8B8
	GRC_TEMP_XG_FORMAT_R16_FLOAT,

	GRC_TEMP_XG_FORMAT_R32_FLOAT,
	GRC_TEMP_XG_FORMAT_R11G11B10_FLOAT, // grctfA2B10G10R10 (PC TODO - No alpha may be trouble)
	GRC_TEMP_XG_FORMAT_R10G10B10A2_UNORM, // grctfA2B10G10R10ATI
	GRC_TEMP_XG_FORMAT_R16G16B16A16_FLOAT,

	GRC_TEMP_XG_FORMAT_R16G16_UNORM,
	GRC_TEMP_XG_FORMAT_R16G16_FLOAT,
	GRC_TEMP_XG_FORMAT_R32G32B32A32_FLOAT,
	GRC_TEMP_XG_FORMAT_R16G16B16A16_FLOAT,

	GRC_TEMP_XG_FORMAT_R16G16B16A16_UNORM,
	GRC_TEMP_XG_FORMAT_R8_UNORM, // lies .. L8
	GRC_TEMP_XG_FORMAT_R16_UNORM, // lies .. L16
	GRC_TEMP_XG_FORMAT_R8G8_UNORM,

	GRC_TEMP_XG_FORMAT_R8G8_UNORM, // grctfG8R8_XENON
	GRC_TEMP_XG_FORMAT_B5G5R5A1_UNORM,
	GRC_TEMP_XG_FORMAT_D24_UNORM_S8_UINT,
	GRC_TEMP_XG_FORMAT_B4G4R4A4_UNORM,

	GRC_TEMP_XG_FORMAT_R32G32_FLOAT,
	GRC_TEMP_XG_FORMAT_D24_UNORM_S8_UINT,
	GRC_TEMP_XG_FORMAT_D16_UNORM,
	GRC_TEMP_XG_FORMAT_R8G8_UNORM, // grctfG8B8

	GRC_TEMP_XG_FORMAT_D32_FLOAT,
	GRC_TEMP_XG_FORMAT_B8G8R8X8_UNORM, // grctfX8R8G8B8
	GRC_TEMP_XG_FORMAT_UNKNOWN,  // PC TODO - No Need for a NULL format.  Color targets can be NULL for DX11.  Need to handle this everywhere
	GRC_TEMP_XG_FORMAT_X24_TYPELESS_G8_UINT,
	GRC_TEMP_XG_FORMAT_A8_UNORM,
	GRC_TEMP_XG_FORMAT_R11G11B10_FLOAT,
	GRC_TEMP_XG_FORMAT_D32_FLOAT_S8X24_UINT,
	GRC_TEMP_XG_FORMAT_X32_TYPELESS_G8X24_UINT,
	GRC_TEMP_XG_FORMAT_BC1_UNORM,
	GRC_TEMP_XG_FORMAT_BC2_UNORM,
	GRC_TEMP_XG_FORMAT_BC3_UNORM,
	GRC_TEMP_XG_FORMAT_BC4_UNORM,
	GRC_TEMP_XG_FORMAT_BC5_UNORM,
	GRC_TEMP_XG_FORMAT_BC6H_UF16,
	GRC_TEMP_XG_FORMAT_BC7_UNORM,
	GRC_TEMP_XG_FORMAT_R8G8B8A8_SNORM, // grctfA8B8G8R8_SNORM
	GRC_TEMP_XG_FORMAT_R8G8B8A8_UNORM, // grctfA8B8G8R8
#if RSG_DURANGO
	GRC_TEMP_XG_FORMAT_NV12,
#endif
};
CompileTimeAssert(NELEM(translate) == grctfCount);


GRC_TEMP_XG_FORMAT ConvertToXGFormat(grcTextureFormat eFormat)
{
	Assert(eFormat < grctfCount); 

	GRC_TEMP_XG_FORMAT uFormat = translate[eFormat];
	Assert(uFormat != GRC_TEMP_XG_FORMAT_UNKNOWN);
	return uFormat;
}


grcTextureFormat ConvertTogrcFormat(GRC_TEMP_XG_FORMAT fmt)
{
	for(int i=0; i<NELEM(translate); i++)
	{
		if(translate[i] == fmt)
			return (grcTextureFormat)i;
	}

	if      (fmt == GRC_TEMP_XG_FORMAT_R8_UNORM) return grctfL8; // we don't have grctfR8, but we do have grctfL8 ..
	else if (fmt == GRC_TEMP_XG_FORMAT_R16_UNORM) return grctfL16; // we don't have grctfR16, but we do have grctfL16 ..
	else return grctfNone;
}


u32 GetBitsPerPixelFromFormat(grcTextureFormat eFormat)
{
	static u32 translate[] = 
	{
		0, // grctfNone,
		16, // grctfR5G6B5,
		32, // grctfA8R8G8B8,
		16, // grctfR16F,

		32, // grctfR32F,
		32, // grctfA2B10G10R10, // PC TODO - No alpha may be trouble
		32, // grctfA2B10G10R10ATI,
		64, // grctfA16B16G16R16F,

		32, // grctfG16R16,
		32, // grctfG16R16F,
		128, // grctfA32B32G32R32F,
		64, // grctfA16B16G16R16F_NoExpand,

		64, // grctfA16B16G16R16,
		8, // grctfL8,
		16, // grctfL16,
		16, // grctfG8R8,

		16, // grctfG8R8_XENON,
		16, // grctfA1R5G5B5,
		32, // grctfD24S8,
		16, // grctfA4R4G4B4,

		64, // grctfG32R32F,
		32, // grctfD24FS8_ReadStencil,
		16, // grctfD16,
		16, // grctfG8B8,

		32, // grctfD32F,
		32, // grctfX8R8G8B8,
		0, // grctfNULL,  // PC TODO - No Need for a NULL format.  Color targets can be NULL for DX11.  Need to handle this everywhere
		32, // grctfX24G8,
		8, // grctfA8,
		32, // grctfR11G11B10F,
		32, // grctfD32FS8,
		32, // grctfX32S8,
		4, // grctfDXT1,
		8, // grctfDXT3,
		8, // grctfDXT5,
		4, // grctfDXT5A,
		8, // grctfDXN,
		8, // grctfBC6,
		8, // grctfBC7,
		32, // grctfA8B8G8R8_SNORM
		32, // grctfA8B8G8R8
#if RSG_DURANGO
		12, // grctfNV12
#endif
	}; 
	CompileTimeAssert(NELEM(translate) == grctfCount);
	Assert(eFormat < grctfCount); 
	u32 uFormat = translate[eFormat];
	return uFormat;
}

bool IsFormatDXTCompressed(grcTextureFormat eFormat)
{
	switch (eFormat)
	{
	case grctfDXT1:
	case grctfDXT3:
	case grctfDXT5:
	case grctfDXT5A:
	case grctfDXN:
	case grctfBC6:
	case grctfBC7:
		return true;

	default:
		break;
	}

	return false;
}

grcTexture::ChannelBits FindUsedChannelsFromFormat(GRC_TEMP_XG_FORMAT eFormat)
{
	grcTexture::ChannelBits bits(false);

	switch(eFormat)
	{
		// RGBA	
	case GRC_TEMP_XG_FORMAT_R32G32B32A32_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R32G32B32A32_FLOAT:
	case GRC_TEMP_XG_FORMAT_R32G32B32A32_UINT:
	case GRC_TEMP_XG_FORMAT_R32G32B32A32_SINT:
	case GRC_TEMP_XG_FORMAT_R16G16B16A16_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R16G16B16A16_FLOAT:
	case GRC_TEMP_XG_FORMAT_R16G16B16A16_UNORM:
	case GRC_TEMP_XG_FORMAT_R16G16B16A16_UINT:
	case GRC_TEMP_XG_FORMAT_R16G16B16A16_SNORM:
	case GRC_TEMP_XG_FORMAT_R16G16B16A16_SINT:
	case GRC_TEMP_XG_FORMAT_R10G10B10A2_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R10G10B10A2_UNORM:
	case GRC_TEMP_XG_FORMAT_R10G10B10A2_UINT:
	case GRC_TEMP_XG_FORMAT_R8G8B8A8_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R8G8B8A8_UNORM:
	case GRC_TEMP_XG_FORMAT_R8G8B8A8_UNORM_SRGB:
	case GRC_TEMP_XG_FORMAT_R8G8B8A8_UINT:
	case GRC_TEMP_XG_FORMAT_R8G8B8A8_SNORM:
	case GRC_TEMP_XG_FORMAT_R8G8B8A8_SINT:
	case GRC_TEMP_XG_FORMAT_B8G8R8A8_UNORM:
	case GRC_TEMP_XG_FORMAT_B5G5R5A1_UNORM:
	case GRC_TEMP_XG_FORMAT_BC1_TYPELESS:
	case GRC_TEMP_XG_FORMAT_BC1_UNORM:
	case GRC_TEMP_XG_FORMAT_BC1_UNORM_SRGB:
	case GRC_TEMP_XG_FORMAT_BC2_TYPELESS:
	case GRC_TEMP_XG_FORMAT_BC2_UNORM:
	case GRC_TEMP_XG_FORMAT_BC2_UNORM_SRGB:
	case GRC_TEMP_XG_FORMAT_BC3_TYPELESS:
	case GRC_TEMP_XG_FORMAT_BC3_UNORM:
	case GRC_TEMP_XG_FORMAT_BC3_UNORM_SRGB:
	case GRC_TEMP_XG_FORMAT_BC7_TYPELESS:
	case GRC_TEMP_XG_FORMAT_BC7_UNORM:
	case GRC_TEMP_XG_FORMAT_BC7_UNORM_SRGB:
		bits.Set(grcTexture::CHANNEL_RED);
		bits.Set(grcTexture::CHANNEL_GREEN);
		bits.Set(grcTexture::CHANNEL_BLUE);
		bits.Set(grcTexture::CHANNEL_ALPHA);
		break;

		// RGB
	case GRC_TEMP_XG_FORMAT_R32G32B32_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R32G32B32_FLOAT:
	case GRC_TEMP_XG_FORMAT_R32G32B32_UINT:
	case GRC_TEMP_XG_FORMAT_R32G32B32_SINT:
	case GRC_TEMP_XG_FORMAT_R9G9B9E5_SHAREDEXP:
	case GRC_TEMP_XG_FORMAT_R11G11B10_FLOAT:
	case GRC_TEMP_XG_FORMAT_B5G6R5_UNORM:
	case GRC_TEMP_XG_FORMAT_B8G8R8X8_UNORM:
	case GRC_TEMP_XG_FORMAT_R8G8_B8G8_UNORM: 
	case GRC_TEMP_XG_FORMAT_G8R8_G8B8_UNORM: 
	case GRC_TEMP_XG_FORMAT_BC6H_TYPELESS:
	case GRC_TEMP_XG_FORMAT_BC6H_UF16:
	case GRC_TEMP_XG_FORMAT_BC6H_SF16:
		bits.Set(grcTexture::CHANNEL_RED);
		bits.Set(grcTexture::CHANNEL_GREEN);
		bits.Set(grcTexture::CHANNEL_BLUE);
		break;

		// RG
	case GRC_TEMP_XG_FORMAT_R32G32_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R32G32_FLOAT:
	case GRC_TEMP_XG_FORMAT_R32G32_UINT:
	case GRC_TEMP_XG_FORMAT_R32G32_SINT:
	case GRC_TEMP_XG_FORMAT_R32G8X24_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R16G16_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R16G16_FLOAT:
	case GRC_TEMP_XG_FORMAT_R16G16_UNORM:
	case GRC_TEMP_XG_FORMAT_R16G16_UINT:
	case GRC_TEMP_XG_FORMAT_R16G16_SNORM:
	case GRC_TEMP_XG_FORMAT_R16G16_SINT:
	case GRC_TEMP_XG_FORMAT_R24G8_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R8G8_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R8G8_UNORM:
	case GRC_TEMP_XG_FORMAT_R8G8_UINT:
	case GRC_TEMP_XG_FORMAT_R8G8_SNORM:
	case GRC_TEMP_XG_FORMAT_R8G8_SINT:
	case GRC_TEMP_XG_FORMAT_BC5_TYPELESS:
	case GRC_TEMP_XG_FORMAT_BC5_UNORM:
	case GRC_TEMP_XG_FORMAT_BC5_SNORM:
#if RSG_DURANGO
	case GRC_TEMP_XG_FORMAT_NV12:
#endif
		bits.Set(grcTexture::CHANNEL_RED);
		bits.Set(grcTexture::CHANNEL_GREEN);
		break;

		// R
	case GRC_TEMP_XG_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R32_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R32_FLOAT:
	case GRC_TEMP_XG_FORMAT_R32_UINT:
	case GRC_TEMP_XG_FORMAT_R32_SINT:
	case GRC_TEMP_XG_FORMAT_R24_UNORM_X8_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R16_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R16_FLOAT:
	case GRC_TEMP_XG_FORMAT_R16_UNORM:
	case GRC_TEMP_XG_FORMAT_R16_UINT:
	case GRC_TEMP_XG_FORMAT_R16_SNORM:
	case GRC_TEMP_XG_FORMAT_R16_SINT:
	case GRC_TEMP_XG_FORMAT_R8_TYPELESS:
	case GRC_TEMP_XG_FORMAT_R8_UNORM:
	case GRC_TEMP_XG_FORMAT_R8_UINT:
	case GRC_TEMP_XG_FORMAT_R8_SNORM:
	case GRC_TEMP_XG_FORMAT_R8_SINT:
	case GRC_TEMP_XG_FORMAT_R1_UNORM:
	case GRC_TEMP_XG_FORMAT_BC4_TYPELESS:
	case GRC_TEMP_XG_FORMAT_BC4_UNORM:
	case GRC_TEMP_XG_FORMAT_BC4_SNORM:
		bits.Set(grcTexture::CHANNEL_RED);
		break;

		// G
	case GRC_TEMP_XG_FORMAT_X32_TYPELESS_G8X24_UINT:
	case GRC_TEMP_XG_FORMAT_X24_TYPELESS_G8_UINT:
		bits.Set(grcTexture::CHANNEL_GREEN);
		break;

		// A
	case GRC_TEMP_XG_FORMAT_A8_UNORM:
		bits.Set(grcTexture::CHANNEL_ALPHA);
		break;

		// Depth
	case GRC_TEMP_XG_FORMAT_D32_FLOAT:
	case GRC_TEMP_XG_FORMAT_D16_UNORM:
		bits.Set(grcTexture::CHANNEL_DEPTH);
		break;

	case GRC_TEMP_XG_FORMAT_D32_FLOAT_S8X24_UINT:
	case GRC_TEMP_XG_FORMAT_D24_UNORM_S8_UINT:
		bits.Set(grcTexture::CHANNEL_DEPTH);
		bits.Set(grcTexture::CHANNEL_STENCIL);
		break;

	default:
		grcWarningf("Don't know what channels are in use in texture format %d", eFormat);
		break;
	}
	return bits;
}


GRC_TEMP_XG_FORMAT GetXGFormatFromGRCImageFormat(u32 grcImageFormat)
{
	GRC_TEMP_XG_FORMAT fmt = GRC_TEMP_XG_FORMAT_UNKNOWN;
	switch (grcImageFormat)
	{
	case grcImage::UNKNOWN                     : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break;
	case grcImage::DXT1                        : fmt = GRC_TEMP_XG_FORMAT_BC1_UNORM         ; break;
	case grcImage::DXT3                        : fmt = GRC_TEMP_XG_FORMAT_BC2_UNORM         ; break;
	case grcImage::DXT5                        : fmt = GRC_TEMP_XG_FORMAT_BC3_UNORM         ; break;
	case grcImage::CTX1                        : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT3A                       : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT3A_1111                  : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::DXT5A                       : fmt = GRC_TEMP_XG_FORMAT_BC4_UNORM         ; break;
	case grcImage::DXN                         : fmt = GRC_TEMP_XG_FORMAT_BC5_UNORM         ; break;
	case grcImage::BC6                         : fmt = GRC_TEMP_XG_FORMAT_BC6H_UF16         ; break;
	case grcImage::BC7                         : fmt = GRC_TEMP_XG_FORMAT_BC7_UNORM         ; break;
	case grcImage::A8R8G8B8                    : fmt = GRC_TEMP_XG_FORMAT_B8G8R8A8_UNORM    ; break;
	case grcImage::A8B8G8R8                    : fmt = GRC_TEMP_XG_FORMAT_R8G8B8A8_UNORM    ; break;
	case grcImage::A8                          : fmt = GRC_TEMP_XG_FORMAT_A8_UNORM          ; break;
	case grcImage::L8                          : fmt = GRC_TEMP_XG_FORMAT_R8_UNORM          ; break;
	case grcImage::A8L8                        : fmt = GRC_TEMP_XG_FORMAT_R8G8_UNORM        ; break;
	case grcImage::A4R4G4B4                    : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A1R5G5B5                    : fmt = GRC_TEMP_XG_FORMAT_B5G5R5A1_UNORM    ; break;
	case grcImage::R5G6B5                      : fmt = GRC_TEMP_XG_FORMAT_B5G6R5_UNORM      ; break;
	case grcImage::R3G3B2                      : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A8R3G3B2                    : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A4L4                        : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A2R10G10B10                 : fmt = GRC_TEMP_XG_FORMAT_R10G10B10A2_UNORM ; break;
	case grcImage::A2B10G10R10                 : fmt = GRC_TEMP_XG_FORMAT_R10G10B10A2_UNORM ; break;
	case grcImage::A16B16G16R16                : fmt = GRC_TEMP_XG_FORMAT_R16G16B16A16_UNORM; break;
	case grcImage::G16R16                      : fmt = GRC_TEMP_XG_FORMAT_R16G16_UNORM      ; break;
	case grcImage::L16                         : fmt = GRC_TEMP_XG_FORMAT_R16_UNORM         ; break;
	case grcImage::A16B16G16R16F               : fmt = GRC_TEMP_XG_FORMAT_R16G16B16A16_FLOAT; break;
	case grcImage::G16R16F                     : fmt = GRC_TEMP_XG_FORMAT_R16G16_FLOAT      ; break;
	case grcImage::R16F                        : fmt = GRC_TEMP_XG_FORMAT_R16_FLOAT         ; break;
	case grcImage::A32B32G32R32F               : fmt = GRC_TEMP_XG_FORMAT_R32G32B32A32_FLOAT; break;
	case grcImage::G32R32F                     : fmt = GRC_TEMP_XG_FORMAT_R32G32_FLOAT      ; break;
	case grcImage::R32F                        : fmt = GRC_TEMP_XG_FORMAT_R32_FLOAT         ; break;
	case grcImage::D15S1                       : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::D24S8                       : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::D24FS8                      : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::P4                          : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::P8                          : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::A8P8                        : fmt = GRC_TEMP_XG_FORMAT_UNKNOWN           ; break; // not supported
	case grcImage::LINA32B32G32R32F_DEPRECATED : fmt = GRC_TEMP_XG_FORMAT_R32G32B32A32_FLOAT; break;
	case grcImage::LINA8R8G8B8_DEPRECATED      : fmt = GRC_TEMP_XG_FORMAT_R8G8B8A8_UNORM    ; break;
	case grcImage::LIN8_DEPRECATED             : fmt = GRC_TEMP_XG_FORMAT_R8_UNORM          ; break;
	case grcImage::RGBE                        : fmt = GRC_TEMP_XG_FORMAT_R8G8B8A8_UNORM    ; break;
	}

	Assertf(fmt != GRC_TEMP_XG_FORMAT_UNKNOWN, "Unsupported Image Format (%d)", (u32)grcImageFormat);

	return fmt;
}

/*======================================================================================================================================*/
/* grcOrbisDurangoTextureBase classes.																									*/
/*======================================================================================================================================*/

grcOrbisDurangoTextureBase::grcOrbisDurangoTextureBase() : DURANGO_PLACEMENT_TEXTURE_BASE_CLASS(0)
, m_pGraphicsMem(NULL)
, m_GraphicsMemorySize(0)
, m_pLockInfoPtr(NULL)
{
	memset(&m_UserMemory, 0, sizeof(m_UserMemory));
}


grcOrbisDurangoTextureBase::grcOrbisDurangoTextureBase(u8 type) : DURANGO_PLACEMENT_TEXTURE_BASE_CLASS(type)
, m_pGraphicsMem(NULL)
, m_GraphicsMemorySize(0)
, m_pLockInfoPtr(NULL)
{
	memset(&m_UserMemory, 0, sizeof(m_UserMemory));
}


grcOrbisDurangoTextureBase::grcOrbisDurangoTextureBase(GRC_ORBIS_DURANGO_TEXTURE_DESC &info, u8 type) : DURANGO_PLACEMENT_TEXTURE_BASE_CLASS(type)
, m_pGraphicsMem(NULL)
, m_GraphicsMemorySize(0)
, m_pLockInfoPtr(NULL)
{
	SetFromDescription(info);
	memset(&m_UserMemory, 0, sizeof(m_UserMemory));
}


grcOrbisDurangoTextureBase::grcOrbisDurangoTextureBase(datResource &rsc) 
: DURANGO_PLACEMENT_TEXTURE_BASE_CLASS(rsc)
{
	rsc.PointerFixup(m_Name);
	rsc.PointerFixup(m_pGraphicsMem);
	rsc.PointerFixup(m_pLockInfoPtr);
}


grcOrbisDurangoTextureBase::~grcOrbisDurangoTextureBase()
{
#if __RESOURCECOMPILER
	delete m_pLockInfoPtr;
#else // __RESOURCECOMPILER
	if(GetOwnsAllocations() && m_pLockInfoPtr)
		delete m_pLockInfoPtr;
#endif // __RESOURCECOMPILER
	m_pLockInfoPtr = NULL;
}


void grcOrbisDurangoTextureBase::SetFromDescription(GRC_ORBIS_DURANGO_TEXTURE_DESC &desc)
{
	m_Texture.SetWidth(desc.m_Width);
	m_Texture.SetHeight(desc.m_Height);
	m_Texture.SetDepth(desc.m_Depth);
	m_Texture.SetDimension(desc.m_ArrayDimension);
	m_Texture.SetMipMap(desc.m_NoOfMips);
	m_Texture.SetBindFlag((u8)desc.m_ExtraBindFlags);

	// We use this as the "owns allocations" flag.
#if __RESOURCECOMPILER
	m_Texture.SetOwnsMem(0);	// When building a resource we "pretend" to not own the memory so the correct value is saved out.
#else //__RESOURCECOMPILER
	m_Texture.SetOwnsMem(1);
#endif //__RESOURCECOMPILER
	
	m_Texture.SetFormat((u8)desc.m_XGFormat);
	m_Texture.SetImageType((u8)desc.m_ImageType);
	m_Texture.SetTileMode((u8)desc.m_TileMode);
	m_pLockInfoPtr = rage_new OFFSET_AND_PITCH[desc.m_NoOfMips];
	memset(m_pLockInfoPtr,0,sizeof(OFFSET_AND_PITCH) * desc.m_NoOfMips);
}


void grcOrbisDurangoTextureBase::SetLockInfo(u32 mip, u64 offset, u64 pitch)
{
	Assertf(m_pLockInfoPtr, "grcOrbisDurangoTextureBase::SetLockInfo()...Expecting lock info.");
	m_pLockInfoPtr[mip].Set(offset, pitch);
}


void grcOrbisDurangoTextureBase::GetLockInfo(u32 mip, u64 &offset, u64 &pitch) const
{
	Assertf(m_pLockInfoPtr, "grcOrbisDurangoTextureBase::GetLockInfo()...Expecting lock info.");
	m_pLockInfoPtr[mip].Get(offset, pitch);
}


bool grcOrbisDurangoTextureBase::GetOwnsAllocations()
{
	return m_Texture.GetOwnsMem() != 0;
}


void grcOrbisDurangoTextureBase::SetUsesPreAllocatedMemory(bool uses)
{
	m_Texture.SetUsesPreAllocatedMem(uses ? 1 : 0);
}


bool grcOrbisDurangoTextureBase::GetUsesPreAllocatedMemory()
{
	return m_Texture.GetUsesPreAllocatedMem() != 0;
}

bool grcOrbisDurangoTextureBase::Copy2D(const void *pSrc, const grcPoint &oSrcDim, const grcRect &oDstRect, const grcTextureLock &lock, s32 UNUSED_PARAM(iMipLevel))
{
	u8* bits = reinterpret_cast<u8*>(lock.Base);

	u8* destPixel = bits + (lock.Pitch * oDstRect.y1) + (oDstRect.x1 * lock.BitsPerPixel >> 3); // first pixel to modify
	const u8* srcPixel = static_cast<const u8*>(pSrc); // first pixel to read from

	u32 width = ((oDstRect.x2 - oDstRect.x1) * lock.BitsPerPixel) >> 3;
	s32 height = (oDstRect.y2 - oDstRect.y1);

	for(int row = 0; row < height; row++)
	{
		sysMemCpy(destPixel, srcPixel, width);
		destPixel += lock.Pitch;
		srcPixel += oSrcDim.x;
	}
	return true;
}

void grcOrbisDurangoTextureBase::EnsureGpuWritable()
{
// TODO: This should be enabled for Durango when WB/GARLIC is changed to read-only for the GPU
DURANGO_ONLY(CompileTimeAssert((MEMTYPE_DEFAULT&MEMTYPE_GPU_RO)==0);)
#if /*RSG_DURANGO ||*/ RSG_ORBIS
	sysMemVirtualAllocator *vm = sysMemVirtualAllocator::sm_Instance;
	void *block = vm->GetCanonicalBlockPtr(m_pGraphicsMem);
	size_t size = vm->GetSize(block);
	vm->SetMemTypeKeepContents(block, size, MEMTYPE_DEFAULT_GPU_RW);
#endif
}


/*--------------------------------------------------------------------------------------------------*/
/* grcTexture functions.																			*/
/*--------------------------------------------------------------------------------------------------*/


int grcOrbisDurangoTextureBase::GetBitsPerPixel() const 
{ 
	return GetBitsPerPixelFromFormat((grcTextureFormat)ConvertTogrcFormat((GRC_TEMP_XG_FORMAT)m_Texture.GetFormat())); 
}


grcTexture::ChannelBits	grcOrbisDurangoTextureBase::FindUsedChannels() const 
{ 
	return FindUsedChannelsFromFormat((GRC_TEMP_XG_FORMAT)m_Texture.GetFormat()); 
}

int	grcOrbisDurangoTextureBase::GetStride(u32 uMipLevel) const
{
	GRC_TEMP_XG_FORMAT xgFormat = (GRC_TEMP_XG_FORMAT)m_Texture.GetFormat();
	int width = Max<int>(1, GetWidth() >> uMipLevel);

	if ((xgFormat >= GRC_TEMP_XG_FORMAT_BC1_TYPELESS && xgFormat <= GRC_TEMP_XG_FORMAT_BC1_UNORM_SRGB) || // DXT1
		(xgFormat >= GRC_TEMP_XG_FORMAT_BC4_TYPELESS && xgFormat <= GRC_TEMP_XG_FORMAT_BC4_SNORM)) // DXT5A
	{
		// Round width up to next multiple of 4, then multiply by 2 (which is really divide by 4 to get # blocks and multiply by 8 bytes per block)
		return ((width + 3) & 0xFFFC) * 2;
	}
	else
	if ((xgFormat >= GRC_TEMP_XG_FORMAT_BC2_TYPELESS && xgFormat <= GRC_TEMP_XG_FORMAT_BC3_UNORM_SRGB) || // DXT3 or DXT5
		(xgFormat >= GRC_TEMP_XG_FORMAT_BC5_TYPELESS && xgFormat <= GRC_TEMP_XG_FORMAT_BC5_SNORM) || // DXN
		(xgFormat >= GRC_TEMP_XG_FORMAT_BC6H_TYPELESS && xgFormat <= GRC_TEMP_XG_FORMAT_BC7_UNORM_SRGB)) // BC6H or BC7
	{
		// Round width up to next multiple of 4, then multiply by 4 (which is really divide by 4 to get # blocks and multiply by 16 bytes per block)
		return ((width + 3) & 0xFFFC) * 4;
	}
	else
	{
#if RSG_DURANGO
		u64 offset, pitch;
		Assert(m_pLockInfoPtr!=NULL);
		m_pLockInfoPtr[uMipLevel].Get(offset, pitch);
		if (pitch != 0)
			return pitch;
#endif // RSG_DURANGO
		int ret = ((GetWidth() >> uMipLevel)*GetBitsPerPixelFromFormat((grcTextureFormat)ConvertTogrcFormat((GRC_TEMP_XG_FORMAT)m_Texture.GetFormat()))) >> 3;
		if(ret == 0) ret = 1;
		return ret;
	}
}


int grcOrbisDurangoTextureBase::GetRowCount(u32 uMipLevel) const
{
	GRC_TEMP_XG_FORMAT xgFormat = (GRC_TEMP_XG_FORMAT)m_Texture.GetFormat();
	int ret = Max<int>(1, m_Texture.GetHeight() >> uMipLevel);

	if ((xgFormat >= GRC_TEMP_XG_FORMAT_BC1_TYPELESS && xgFormat <= GRC_TEMP_XG_FORMAT_BC5_SNORM) || // DXT1, DXT3, DXT5, DXT5A, DXN
		(xgFormat >= GRC_TEMP_XG_FORMAT_BC6H_TYPELESS && xgFormat <= GRC_TEMP_XG_FORMAT_BC7_UNORM_SRGB)) // BC6H or BC7
	{
		// Account for DXT/BC formats being in 4x4 blocks.
		ret = Max<int>(1, (ret + 3) >> 2);
	}

	return ret;
}


/*--------------------------------------------------------------------------------------------------*/
/* HD swap function.																				*/
/*--------------------------------------------------------------------------------------------------*/

template<class T>
inline void Swap(T& left, T& right)
{
	T temp = left; left = right; right = temp;
}

#define GRC_ORBIS_DURANGO_TEXTURE_BASE_SWAP(_x) Swap(pA->_x, pB->_x)

void* grcOrbisDurangoTextureBase::PeformHDOverrideSwap(grcOrbisDurangoTextureBase *pA, grcOrbisDurangoTextureBase *pB, void* pOldHdAddr, void*& pDeferredFreePtr, size_t& deferredFreeSize)
{
#if RSG_DURANGO || RSG_ORBIS
	pDeferredFreePtr = NULL;

    static const bool remapMipChain = true;
    if (remapMipChain)
	{
		// since we trick the hd texture into thinking it's got its full mip chain, it should always have a higher mip count.
		// toHd will be false when we swap back to the LD texture
		const bool toHd = pA->GetMipMapCount() < pB->GetMipMapCount();

		const size_t pageSize = sysMemVirtualAllocator::BuddyPageSize;
		const size_t pageSizeMask = ~(pageSize - 1);

		sysMemVirtualAllocator *const allocator = sysMemVirtualAllocator::sm_Instance;

		if (toHd)
		{
			const grcImage::Format format = (grcImage::Format)pB->GetImageFormat();
			s32 dim = rage::Min(pB->GetWidth(), pB->GetHeight());
			const bool allowSplit = (dim >= 256 && (format == grcImage::DXT3 || format == grcImage::DXT5)) || (dim >= 512 && format == grcImage::DXT1);
			if (!allowSplit)
			{
				BANK_ONLY(Warningf("[HD SPLIT]: Skipping split for %s, format %d, w %d, h %d", pB->GetName(), format, pB->GetWidth(), pB->GetHeight());)
				return NULL;
			}

			const size_t baseSize = pB->m_GraphicsMemorySize;

			if (!Verifyf((baseSize&(pageSize-1)) == 0 && baseSize >= pageSize, "PerformHDOverrideSwap: HD mip size is %lu, needs to be pageSize aligned and >= %lu! %s bpp: %d", pB->m_GraphicsMemorySize, pageSize, pB->GetName(), pB->GetBitsPerPixel()))
				return NULL;
			if (!Verifyf(!allocator->IsPooled(pA->m_pGraphicsMem), "PerformHDOverrideSwap: LD mip chain is non pooled and can't be remapped! %s", pA->GetName()))
				return NULL;

			const size_t mipSize = (pA->m_GraphicsMemorySize + pageSize - 1) & pageSizeMask;
			const size_t virtSize = baseSize+mipSize;
			void *const virtAddr = allocator->AllocateVirtual(virtSize);
			if (!Verifyf(virtAddr, "PerformHDOverrideSwap: Failed to allocate virtual memory for HD + mip chain!"))
				return NULL;

			size_t *const physAddrSizePairs = Alloca(size_t, MAX_NUM_PHYSICAL_ADDR_SIZE_PAIR_ENTRIES(virtSize));
			const u32 basePairs = allocator->GetPhysicalAddressSizePairs(physAddrSizePairs, pB->m_pGraphicsMem, baseSize);
			Assert(basePairs);
			const u32 mipPairs = allocator->GetPhysicalAddressSizePairs(physAddrSizePairs+basePairs*2, pA->m_pGraphicsMem, mipSize);
			Assert(mipPairs);
			const u32 totalPairs = basePairs+mipPairs;
			Assert(totalPairs <= MAX_NUM_PHYSICAL_ADDR_SIZE_PAIR_ENTRIES(virtSize)/2);

			allocator->MapVirtualToPhysical(virtAddr, physAddrSizePairs, totalPairs, RSG_ORBIS?MEMTYPE_DEFAULT_GPU_RO:MEMTYPE_DEFAULT);

			pOldHdAddr = pB->m_pGraphicsMem;
			pB->m_pGraphicsMem = virtAddr;
#if RSG_ORBIS
			pB->GetTexturePtr()->setBaseAddress256ByteBlocks((uint64_t)pB->m_pGraphicsMem >> 8);
#elif RSG_DURANGO
			grcDurangoPlacementTexture* hdTex = (grcDurangoPlacementTexture*)pB;
			grcOrbisDurangoTextureBase info;
			hdTex->CreateD3DResources(info, pB->m_pGraphicsMem);
#endif
		}

		else
		{
			// we're swapping back to LD, we just need to make sure we unmap the temporary HD virtual space
			// and restore the previous HD virtual pointer
			Assertf(pOldHdAddr, "PerformHDOverrideSwap: Swapping texture to LD but no valid HD pointer to restore!");

			const size_t baseSize = pA->m_GraphicsMemorySize;
			Assert((baseSize&(pageSize-1)) == 0);
			const size_t mipSize = (pB->m_GraphicsMemorySize + pageSize - 1) & pageSizeMask;
			const size_t virtSize = baseSize+mipSize;
			pDeferredFreePtr = pA->m_pGraphicsMem;
			deferredFreeSize = virtSize;

			pA->m_pGraphicsMem = pOldHdAddr;
#if RSG_ORBIS
			pA->GetTexturePtr()->setBaseAddress256ByteBlocks((uint64_t)pA->m_pGraphicsMem >> 8);
#elif RSG_DURANGO
			grcDurangoPlacementTexture* ldTex = (grcDurangoPlacementTexture*)pA;
			grcOrbisDurangoTextureBase info;
			ldTex->CreateD3DResources(info, pA->m_pGraphicsMem);
#endif
			pOldHdAddr = NULL;
		}
	}

	// Stuff in grcTexture.
	GRC_ORBIS_DURANGO_TEXTURE_BASE_SWAP(m_Texture);
#if !RSG_ORBIS
	GRC_ORBIS_DURANGO_TEXTURE_BASE_SWAP(m_CachedTexturePtr);
#endif // !RSG_ORBIS

	// Stuff in grcOrbisDurangoTextureBase.
	GRC_ORBIS_DURANGO_TEXTURE_BASE_SWAP(m_pGraphicsMem);
	GRC_ORBIS_DURANGO_TEXTURE_BASE_SWAP(m_GraphicsMemorySize);
	GRC_ORBIS_DURANGO_TEXTURE_BASE_SWAP(m_pLockInfoPtr);

	for(int i =0; i<GRC_ORBIS_DURANGO_TEXTURE_USER_MEM_SIZE; i++)
		GRC_ORBIS_DURANGO_TEXTURE_BASE_SWAP(m_UserMemory[i]);
#else
	(void)pA;
	(void)pB;
	(void)pDeferredFreePtr;
	(void)deferredFreeSize;
#endif // RSG_DURANGO || RSG_ORBIS

    return pOldHdAddr;
}

#if __DECLARESTRUCT
void grcOrbisDurangoTextureBase::DeclareStruct(class datTypeStruct &s)
{
	DURANGO_PLACEMENT_TEXTURE_BASE_CLASS::DeclareStruct(s);

	STRUCT_BEGIN(grcOrbisDurangoTextureBase);
	STRUCT_FIELD(m_pGraphicsMem);
	STRUCT_FIELD(m_GraphicsMemorySize);
	STRUCT_FIELD_VP(m_pLockInfoPtr);
	STRUCT_IGNORE(m_UserMemory);
	STRUCT_END();
}
#endif //__DECLARESTRUCT


/*======================================================================================================================================*/
/* grcOrbisDurangoBufferBase classes.																									*/
/*======================================================================================================================================*/


grcOrbisDurangoBufferBase::grcOrbisDurangoBufferBase()
: m_Flags(0)
, m_Size(0)
, m_pGraphicsMemory(NULL)
{
	memset(&m_UserMemory, 0, sizeof(m_UserMemory));
}

grcOrbisDurangoBufferBase::grcOrbisDurangoBufferBase(class datResource& rsc)
{
	rsc.PointerFixup(m_pGraphicsMemory);
}


grcOrbisDurangoBufferBase::~grcOrbisDurangoBufferBase()
{

}


#if __DECLARESTRUCT
void grcOrbisDurangoBufferBase::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(grcOrbisDurangoBufferBase);
	STRUCT_FIELD(m_Flags);
	STRUCT_FIELD(m_Size);
	STRUCT_FIELD(m_pGraphicsMemory);
	STRUCT_IGNORE(m_UserMemory[0]);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

} // namespace rage 

#endif // RSG_DURANGO || RSG_ORBIS || (RSG_PC && __64BIT && __RESOURCECOMPILER)
