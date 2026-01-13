// 
// grcore/texture_gnm.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 
#include "dds.h"
#include "device.h"
#include "texture_gnm.h"
#include "texturereference.h"

#include "texture.h"

#if RSG_PC && !__TOOL
#include "texturepc.h"
namespace rage {
CompileTimeAssertSize(grcTexturePC,92,136);
} 
#endif // RSG_PC && !__TOOL

#if __GNM

#include "gfxcontext_gnm.h"
#include <gpu_address.h>

#include "image.h"

namespace rage {

sce::Gnm::DataFormat grcTextureGNMFormats::grctf_to_Orbis[] = {
	sce::Gnm::kDataFormatInvalid,			// grctfNone
	sce::Gnm::kDataFormatB5G6R5Unorm,		// grctfR5G6B5
	sce::Gnm::kDataFormatB8G8R8A8Unorm,		// grctfA8R8G8B8
	sce::Gnm::kDataFormatR16Float,			// grctfR16F
	sce::Gnm::kDataFormatR32Float,			// grctfR32F
	sce::Gnm::kDataFormatR10G10B10A2Unorm,	// grctfA2B10G10R10
	sce::Gnm::kDataFormatR10G10B10A2Unorm,	// grctfA2B10G10R10ATI
	sce::Gnm::kDataFormatB16G16R16A16Float,	// grctfA16B16G16R16F
	sce::Gnm::kDataFormatR16G16Unorm,		// grctfG16R16
	sce::Gnm::kDataFormatR16G16Float,		// grctfG16R16F
	sce::Gnm::kDataFormatR32G32B32A32Float,	// grctfA32B32G32R32F
	sce::Gnm::kDataFormatR32G32B32A32Float,	// grctfA16B16G16R16F_NoExpand
	sce::Gnm::kDataFormatR16G16B16A16Unorm,	// grctfA16B16G16R16
	sce::Gnm::kDataFormatL8Unorm,			// grctfL8
	sce::Gnm::kDataFormatL16Unorm,			// grctfL16
	sce::Gnm::kDataFormatR8G8Unorm,			// grctfG8R8
	sce::Gnm::kDataFormatInvalid,			// grctfG8R8_XENON
	sce::Gnm::kDataFormatB5G5R5A1Unorm,		// grctfA1R5G5B5
	sce::Gnm::kDataFormatInvalid,			// grctfD24S8
	sce::Gnm::kDataFormatB4G4R4A4Unorm,		// grctfA4R4G4B4
	sce::Gnm::kDataFormatR32G32Float,		// grctfG32R32F
	sce::Gnm::kDataFormatInvalid,			// grctfD24FS8_ReadStencil
	sce::Gnm::kDataFormatInvalid,			// grctfD16
	sce::Gnm::kDataFormatInvalid,			// grctfG8B8
	sce::Gnm::kDataFormatInvalid,			// grctfD32F
	sce::Gnm::kDataFormatB8G8R8X8Unorm,		// grctfX8R8G8B8
	sce::Gnm::kDataFormatInvalid,			// grctfNULL
	sce::Gnm::kDataFormatInvalid,			// grctfX24G8 -- note: use kDataFormatX24G8Uint for rendertargets
	sce::Gnm::kDataFormatA8Unorm,			// grctfA8
	sce::Gnm::kDataFormatR11G11B10Float,	// grctfR11G11B10F
	sce::Gnm::kDataFormatInvalid,			// grctfD32FS8
	sce::Gnm::kDataFormatInvalid,			// grctfX32S8
	sce::Gnm::kDataFormatBc1Unorm,			// grctfDXT1
	sce::Gnm::kDataFormatBc2Unorm,			// grctfDXT3
	sce::Gnm::kDataFormatBc3Unorm,			// grctfDXT5
	sce::Gnm::kDataFormatBc4Unorm,			// grctfDXT5A
	sce::Gnm::kDataFormatBc5Unorm,			// grctfDXN
	sce::Gnm::kDataFormatBc6Uf16,			// grctfBC6
	sce::Gnm::kDataFormatBc7Unorm,			// grctfBC7
	sce::Gnm::kDataFormatR8G8B8A8Snorm,		// grctfA8B8G8R8_SNORM
	sce::Gnm::kDataFormatR8G8B8A8Unorm		// grctfA8B8G8R8

};
CompileTimeAssert(NELEM(grcTextureGNMFormats::grctf_to_Orbis) == grctfCount);

const sce::Gnm::DataFormat sce__Gnm__kDataFormatL4A4Unorm = {{{sce::Gnm::kSurfaceFormat4_4, sce::Gnm::kTextureChannelTypeUNorm, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelX, sce::Gnm::kTextureChannelY}}};

sce::Gnm::DataFormat grcTextureGNMFormats::grcImage_to_Orbis[] = {
	sce::Gnm::kDataFormatInvalid,			// UNKNOWN                    , // 0
	sce::Gnm::kDataFormatBc1Unorm,			// DXT1                       , // 1
	sce::Gnm::kDataFormatBc2Unorm,			// DXT3                       , // 2
	sce::Gnm::kDataFormatBc3Unorm,			// DXT5                       , // 3
	sce::Gnm::kDataFormatInvalid,			// CTX1                       , // 4 - like DXT1 but anchor colors are 8.8 instead of 5.6.5 (XENON-specific)
	sce::Gnm::kDataFormatInvalid,			// DXT3A                      , // 5 - alpha block of DXT3 (XENON-specific)
	sce::Gnm::kDataFormatInvalid,			// DXT3A_1111                 , // 6 - alpha block of DXT3, split into four 1-bit channels (XENON-specific)
	sce::Gnm::kDataFormatBc4Unorm,			// DXT5A                      , // 7 - alpha block of DXT5 (aka 'BC4', 'ATI1')
	sce::Gnm::kDataFormatBc5Unorm,			// DXN                        , // 8
	sce::Gnm::kDataFormatBc6Uf16,			// BC6                        , // 8
	sce::Gnm::kDataFormatBc7Unorm,			// BC7                        , // 8
	sce::Gnm::kDataFormatB8G8R8A8Unorm,		// A8R8G8B8                   , // 9
	sce::Gnm::kDataFormatR8G8B8A8Unorm,		// A8B8G8R8                   , // 10
	sce::Gnm::kDataFormatA8Unorm,			// A8                         , // 11 - 8-bit alpha-only (color is black)
	sce::Gnm::kDataFormatL8Unorm,			// L8                         , // 12 - 8-bit luminance (R=G=B=L, alpha is opaque)
	sce::Gnm::kDataFormatL8A8Unorm,			// A8L8                       , // 13 - 16-bit alpha + luminance
	sce::Gnm::kDataFormatB4G4R4A4Unorm,		// A4R4G4B4                   , // 14 - 16-bit color and alpha
	sce::Gnm::kDataFormatB5G5R5A1Unorm,		// A1R5G5B5                   , // 15 - 16-bit color with 1-bit alpha
	sce::Gnm::kDataFormatB5G6R5Unorm,		// R5G6B5                     , // 16 - 16-bit color
	sce::Gnm::kDataFormatInvalid,			// R3G3B2                     , // 17 - 8-bit color (not supported on consoles)
	sce::Gnm::kDataFormatInvalid,			// A8R3G3B2                   , // 18 - 16-bit color with 8-bit alpha (not supported on consoles)
	sce__Gnm__kDataFormatL4A4Unorm,			// A4L4                       , // 19 - 8-bit alpha + luminance (not supported on consoles)
	sce::Gnm::kDataFormatB10G10R10A2Unorm,	// A2R10G10B10                , // 20 - 32-bit color with 2-bit alpha
	sce::Gnm::kDataFormatR10G10B10A2Unorm,	// A2B10G10R10                , // 21 - 32-bit color with 2-bit alpha
	sce::Gnm::kDataFormatR16G16B16A16Unorm,	// A16B16G16R16               , // 22 - 64-bit four channel fixed point (s10e5 per channel -- sign, 5 bit exponent, 10 bit mantissa)
	sce::Gnm::kDataFormatR16G16Unorm,		// G16R16                     , // 23 - 32-bit two channel fixed point
	sce::Gnm::kDataFormatL16Unorm,			// L16                        , // 24 - 16-bit luminance
	sce::Gnm::kDataFormatR16G16B16A16Float,	// A16B16G16R16F              , // 25 - 64-bit four channel floating point (s10e5 per channel)
	sce::Gnm::kDataFormatR16G16Float,		// G16R16F                    , // 26 - 32-bit two channel floating point (s10e5 per channel)
	sce::Gnm::kDataFormatR16Float,			// R16F                       , // 27 - 16-bit single channel floating point (s10e5 per channel)
	sce::Gnm::kDataFormatR32G32B32A32Float,	// A32B32G32R32F              , // 28 - 128-bit four channel floating point (s23e8 per channel)
	sce::Gnm::kDataFormatR32G32Float,		// G32R32F                    , // 29 - 64-bit two channel floating point (s23e8 per channel)
	sce::Gnm::kDataFormatR32Float,			// R32F                       , // 30 - 32-bit single channel floating point (s23e8 per channel)
	sce::Gnm::kDataFormatInvalid,			// D15S1                      , // 31 - 16-bit depth + stencil (depth is 15-bit fixed point, stencil is 1-bit) (not supported on consoles)
	sce::Gnm::kDataFormatInvalid,			// D24S8                      , // 32 - 32-bit depth + stencil (depth is 24-bit fixed point, stencil is 8-bit)
	sce::Gnm::kDataFormatInvalid,			// D24FS8                     , // 33 - 32-bit depth + stencil (depth is 24-bit s15e8, stencil is 8-bit)
	sce::Gnm::kDataFormatInvalid,			// P4                         , // 34 - 4-bit palettized (not supported on consoles)
	sce::Gnm::kDataFormatInvalid,			// P8                         , // 35 - 8-bit palettized (not supported on consoles)
	sce::Gnm::kDataFormatInvalid,			// A8P8                       , // 36 - 16-bit palettized with 8-bit alpha (not supported on consoles)
	sce::Gnm::kDataFormatR8Unorm,			// R8                         , // 37 - non-standard R001 format (8 bits per channel)
	sce::Gnm::kDataFormatR16Unorm,			// R16                        , // 38 - non-standard R001 format (16 bits per channel)
	sce::Gnm::kDataFormatR8G8Unorm,			// G8R8                       , // 39 - non-standard RG01 format (8 bits per channel)
	sce::Gnm::kDataFormatInvalid,			// LINA32B32G32R32F_DEPRECATED, // 40
	sce::Gnm::kDataFormatInvalid,			// LINA8R8G8B8_DEPRECATED     , // 41
	sce::Gnm::kDataFormatInvalid,			// LIN8_DEPRECATED            , // 42
	sce::Gnm::kDataFormatInvalid,			// RGBE                       , // 43
};
CompileTimeAssert(NELEM(grcTextureGNMFormats::grcImage_to_Orbis) == grcImage::FORMAT_COUNT);

grcTextureFormat Orbis_to_grctf(sce::Gnm::DataFormat orbis) 
{
	for(int i=1; i<grctfCount; i++) {
		if(orbis.m_asInt == grcTextureGNMFormats::grctf_to_Orbis[i].m_asInt)
			return (grcTextureFormat)i;
	}
	return grctfNULL;
}

#define GNM_TEXTURE GetUserMemory()->m_GnmTexture

u32 grcTextureGNM::GetImageFormat() const
{
	// note that this corresponds exactly to the resource constructor for grcTextureGNM
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatBc1Unorm         .m_asInt) return grcImage::DXT1;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatBc2Unorm         .m_asInt) return grcImage::DXT3;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatBc3Unorm         .m_asInt) return grcImage::DXT5;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatBc4Unorm         .m_asInt) return grcImage::DXT5A;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatBc5Unorm         .m_asInt) return grcImage::DXN;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatBc7Unorm         .m_asInt) return grcImage::BC7;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatB8G8R8A8Unorm    .m_asInt) return grcImage::A8R8G8B8;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR8G8B8A8Unorm    .m_asInt) return grcImage::A8B8G8R8;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatA8Unorm          .m_asInt) return grcImage::A8;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatL8Unorm          .m_asInt) return grcImage::L8;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatL8A8Unorm        .m_asInt) return grcImage::A8L8;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce__Gnm__kDataFormatL4A4Unorm        .m_asInt) return grcImage::A4L4;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatB4G4R4A4Unorm    .m_asInt) return grcImage::A4R4G4B4;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatB5G5R5A1Unorm    .m_asInt) return grcImage::A1R5G5B5;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatB5G6R5Unorm      .m_asInt) return grcImage::R5G6B5;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatB10G10R10A2Unorm .m_asInt) return grcImage::A2R10G10B10;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR10G10B10A2Unorm .m_asInt) return grcImage::A2B10G10R10;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR16G16B16A16Unorm.m_asInt) return grcImage::A16B16G16R16;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR16G16Unorm      .m_asInt) return grcImage::G16R16;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatL16Unorm         .m_asInt) return grcImage::L16;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR16G16B16A16Float.m_asInt) return grcImage::A16B16G16R16F;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR16G16Float      .m_asInt) return grcImage::G16R16F;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR16Float         .m_asInt) return grcImage::R16F;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR32G32B32A32Float.m_asInt) return grcImage::A32B32G32R32F;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR32G32Float      .m_asInt) return grcImage::G32R32F;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR32Float         .m_asInt) return grcImage::R32F;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR8Unorm          .m_asInt) return grcImage::R8;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR16Unorm         .m_asInt) return grcImage::R16;
	if (GNM_TEXTURE.getDataFormat().m_asInt == sce::Gnm::kDataFormatR8G8Unorm        .m_asInt) return grcImage::G8R8;

	// otherwise, try to deduce format from layout only ..
	sce::Gnm::SurfaceFormat eFormat = GNM_TEXTURE.getDataFormat().getSurfaceFormat();

	switch (eFormat)
	{
	case sce::Gnm::kSurfaceFormatBc1:			return grcImage::DXT1;
	case sce::Gnm::kSurfaceFormatBc2:			return grcImage::DXT3;
	case sce::Gnm::kSurfaceFormatBc3:			return grcImage::DXT5;
	case sce::Gnm::kSurfaceFormatBc4:			return grcImage::DXT5A;
	case sce::Gnm::kSurfaceFormatBc5:			return grcImage::DXN;
	case sce::Gnm::kSurfaceFormat32_32_32_32:	return grcImage::A32B32G32R32F;
	case sce::Gnm::kSurfaceFormat16_16_16_16:	return GNM_TEXTURE.getDataFormat().getTextureChannelType() == sce::Gnm::kTextureChannelTypeFloat ? grcImage::A16B16G16R16F : grcImage::A16B16G16R16;
	case sce::Gnm::kSurfaceFormat2_10_10_10:	return grcImage::A2B10G10R10; // could be A2B10G10R10 or A2R10G10B10 ..
	case sce::Gnm::kSurfaceFormat8_8_8_8:		return grcImage::A8R8G8B8; // could be A8R8G8B8, A8B8G8R8, R8G8BA8 or B8G8R8A8 ..
	case sce::Gnm::kSurfaceFormat16_16:			return GNM_TEXTURE.getDataFormat().getTextureChannelType() == sce::Gnm::kTextureChannelTypeFloat ? grcImage::G16R16F : grcImage::G16R16;
	case sce::Gnm::kSurfaceFormat8_8:			return grcImage::A8L8; // could be A8L8 or G8R8 ..
	case sce::Gnm::kSurfaceFormat24_8:			return grcImage::D24S8;
	case sce::Gnm::kSurfaceFormat5_6_5:			return grcImage::R5G6B5;
	case sce::Gnm::kSurfaceFormat1_5_5_5:		return grcImage::A1R5G5B5;
	case sce::Gnm::kSurfaceFormat4_4_4_4:		return grcImage::A4R4G4B4;
	case sce::Gnm::kSurfaceFormat4_4:			return grcImage::A4L4;
	case sce::Gnm::kSurfaceFormat8:				return grcImage::R8; // could be L8, A8 or R8 ..
	case sce::Gnm::kSurfaceFormat16:			return GNM_TEXTURE.getDataFormat().getTextureChannelType() == sce::Gnm::kTextureChannelTypeFloat ? grcImage::R16F : grcImage::R16; // could also be L16 ..
	case sce::Gnm::kSurfaceFormat32:			return grcImage::R32F;
#if EFFECT_TRACK_COMPARISON_ERRORS
	//Ignore Fmasks formats
	case sce::Gnm::kSurfaceFormatFmask8_S2_F1:
	case sce::Gnm::kSurfaceFormatFmask8_S4_F1:
	case sce::Gnm::kSurfaceFormatFmask8_S8_F1:
	case sce::Gnm::kSurfaceFormatFmask8_S2_F2:
	case sce::Gnm::kSurfaceFormatFmask8_S4_F2:
	case sce::Gnm::kSurfaceFormatFmask8_S4_F4:
	case sce::Gnm::kSurfaceFormatFmask16_S16_F1:
	case sce::Gnm::kSurfaceFormatFmask16_S8_F2:
	case sce::Gnm::kSurfaceFormatFmask32_S16_F2:
	case sce::Gnm::kSurfaceFormatFmask32_S8_F4:
	case sce::Gnm::kSurfaceFormatFmask32_S8_F8:
	case sce::Gnm::kSurfaceFormatFmask64_S16_F4:
	case sce::Gnm::kSurfaceFormatFmask64_S16_F8:
		return grcImage::UNKNOWN;
#endif //EFFECT_TRACK_COMPARISON_ERRORS
	default:
		Assertf(0, "PS4: Unknown Gnm Texture Format %u", eFormat);
		break;
	}

	return grcImage::UNKNOWN;
}

grcTextureGNM::grcTextureGNM(datResource&rsc) 
: grcOrbisDurangoTextureBase(rsc)
{
	if(rsc.IsDefragmentation())
	{
		m_CachedTexturePtr = &GetGnmTexture();
	}else
	{
		ASSERT_ONLY(sce::Gnm::SizeAlign result = )InitGnmTexture();
		ASSERT_ONLY(size_t pAligned = ((size_t)m_pGraphicsMem + ((size_t)result.m_align - 1)) & ~((size_t)result.m_align - 1));
		grcAssertf(pAligned == (size_t)m_pGraphicsMem, "grcTextureGNM::grcTextureGNM(datResource&rsc)...Bad alignment encountered");
	}

	GetGnmTexture().setBaseAddress256ByteBlocks((uint64_t)m_pGraphicsMem >> 8);
}


grcTextureGNM::~grcTextureGNM()
{
	if(m_pGraphicsMem && GetOwnsAllocations())
		freeVideoSharedMemory((void*)m_pGraphicsMem);
	m_pGraphicsMem = NULL;
}


grcTextureGNM::grcTextureGNM(const char *filename,grcTextureFactory::TextureCreateParams *params)
: grcOrbisDurangoTextureBase()
{
	grcImage *image = NULL;
	if (!strcmp(filename,"none") || !strcmp(filename,"nonresident")) {
		const u32 texel = !strcmp(filename,"none")? 0xFFFFFFFF : 0x40FFFFFF;
		const int dummySize = 8;

		sysMemStartTemp();
		image = grcImage::Create(dummySize, dummySize, 1, grcImage::A8R8G8B8, grcImage::STANDARD, 0, 0);
		sysMemEndTemp();
		u32 *texels = (u32*) image->GetBits();

		for (int i = 0; i < dummySize*dummySize; i++)
			texels[i] = texel;
	}
	else
		image = grcImage::Load(filename);

	if (image) {
		grcTextureFactory::TextureCreateParams p(grcTextureFactory::TextureCreateParams::VIDEO,grcTextureFactory::TextureCreateParams::TILED);
		if (!params) {
			params = &p;
			p.MipLevels = image->GetExtraMipCount() + 1;
		}
		else
			params->MipLevels = image->GetExtraMipCount() + 1;
		Create(image->GetWidth(),image->GetHeight(),image->GetDepth(),GetXGFormatFromGRCImageFormat(image->GetFormat()),params);
		Copy(image);
		image->Release();
	}
}

grcTextureGNM::grcTextureGNM(grcImage *image,grcTextureFactory::TextureCreateParams *params)
: grcOrbisDurangoTextureBase()
{
	grcTextureFactory::TextureCreateParams p(grcTextureFactory::TextureCreateParams::VIDEO,grcTextureFactory::TextureCreateParams::TILED);
	if (!params) {
		params = &p;
		p.MipLevels = image->GetExtraMipCount() + 1;
	}
	else
		params->MipLevels = image->GetExtraMipCount() + 1;
	Create(image->GetWidth(),image->GetHeight(),image->GetDepth(),GetXGFormatFromGRCImageFormat(image->GetFormat()),params);
	Copy(image);
}

grcTextureGNM::grcTextureGNM(u32 width,u32 height,u32 format,void *pBuffer,grcTextureFactory::TextureCreateParams *params)
: grcOrbisDurangoTextureBase()
{
	Create(width,height,1,ConvertToXGFormat((grcTextureFormat)format),params);

	if (pBuffer)
	{
		// Copy all mips
		Assertf((grcImage::ImageType)GetImageType() == grcImage::STANDARD, "grcTextureGNM::grcTextureGNM: copy for non-2d texture not implemented");
		u32 mipW		= width;
		u32 mipH		= height;
		u32 numMips		= (u32)GetMipMapCount();
		u32 mipOffset	= 0U;

		grcTextureFormat grcFormat  = (grcTextureFormat)format;
		u32 bpp = GetBitsPerPixelFromFormat(grcFormat);
		u32 blockSize   = IsFormatDXTCompressed(grcFormat) ? 4 : 1;

		for (u32 i=0; i<numMips; i++)
		{
			void* pMipData = ((char*)pBuffer + mipOffset);
			CopyBits(i, pMipData);
			mipOffset += (mipW*mipH*bpp)/8;
			mipW = Max<u32>(blockSize, mipW/2);
			mipH = Max<u32>(blockSize, mipH/2);
		}
	}
}


grcTextureGNM::grcTextureGNM(u32 width,u32 height,u32 depth,u32 format,void *pBuffer,grcTextureFactory::TextureCreateParams *params)
	: grcOrbisDurangoTextureBase()
{
	Assertf(pBuffer == NULL, "grcTextureGNM::grcTextureGNM(widthm height, depth,...)...Initial 3d image not supported yet.");
	Create(width,height,depth,ConvertToXGFormat((grcTextureFormat)format),params);
}


grcTextureGNM::grcTextureGNM(const sce::Gnm::RenderTarget &rt, const char type)
: grcOrbisDurangoTextureBase()
{
	GRC_ORBIS_DURANGO_TEXTURE_DESC placementCreateInfo;

	placementCreateInfo.m_Width = rt.getWidth();
	placementCreateInfo.m_Height = rt.getHeight();
	placementCreateInfo.m_Depth = 1;
	placementCreateInfo.m_ArrayDimension = 1;
	placementCreateInfo.m_NoOfMips = 1;
	// LDS DMC TEMP:- Fix this!
	//placementCreateInfo.m_XGFormat = ConvertToXGFormat(Orbis_to_grctf(rt.getDataFormat()));
	placementCreateInfo.m_ImageType = (u8)grcImage::STANDARD;
	placementCreateInfo.m_ExtraBindFlags = (u16)grcBindRenderTarget;
	placementCreateInfo.m_TileMode = OrbisTogrcOrbisDurangoTileMode(rt.getTileMode());
	SetFromDescription(placementCreateInfo);

	m_pGraphicsMem = NULL;

	switch(type)
	{
	case 0:	// main RT
		GetGnmTexture().initFromRenderTarget( &rt, false );
		break;
	case 'f':	// Fmask
		GetGnmTexture().initAsFmask( rt.getWidth(), rt.getHeight(), 1, rt.getFmaskTileMode(), rt.getNumSamples(), rt.getNumFragments() );

		// Warning: using Orbis hack provided by Sony in bug 7054
		if (rt.getNumSamples() == sce::Gnm::kNumSamples16)
			GetGnmTexture().m_regs[3] |= (5<<3);

		GetGnmTexture().setBaseAddress( rt.getFmaskAddress() );
		break;
	default:
		Assertf(0,"Unknown type: %d",type);
	}
	m_CachedTexturePtr = &GetGnmTexture();
}


bool grcTextureGNM::IsGammaEnabled() const
{
	return false;
}

void grcTextureGNM::SetGammaEnabled(bool enabled)
{

}


bool grcTextureGNM::LockRect(int layer, int mipLevel,grcTextureLock &lock, u32 uLockFlags) const
{
	Assert(layer == 0);
	Assert(mipLevel < GetMipMapCount());

	lock.BitsPerPixel = GetBitsPerPixel();
	lock.Width = GetWidth() >> mipLevel;
	lock.Height = GetHeight() >> mipLevel;
	lock.MipLevel = mipLevel;
	lock.Layer = 0;
	lock.RawFormat = GetGnmTexture().getDataFormat().m_asInt;

	size_t offset = 0;
	size_t pitch = 0;

	if(GetDepth() == 1)
	{
		GetLockInfo(mipLevel, offset, pitch);
	}
	else
	{
		for(int i=0; i<=mipLevel; i++)
		{
			sce::GpuAddress::SurfaceInfo surfaceInfo;
			sce::GpuAddress::TilingParameters tilingParams;
			tilingParams.initFromTexture(&GetGnmTexture(), i, 0);
			sce::GpuAddress::computeSurfaceInfo(&surfaceInfo, &tilingParams);
			pitch = (surfaceInfo.m_pitch*surfaceInfo.m_bitsPerElement) >> 3;

			if(i != mipLevel)
				offset += (lock.Pitch*surfaceInfo.m_height*surfaceInfo.m_depth);
		}
	}
	lock.Base = (char*)m_pGraphicsMem + offset;
	lock.Pitch = pitch;

	return true;
}

void grcTextureGNM::UnlockRect(const grcTextureLock &/*lock*/) const
{

}

void grcTextureGNM::Create(u32 width,u32 height,u32 depth,GRC_TEMP_XG_FORMAT grcTempformat,grcTextureFactory::TextureCreateParams *params)
{
	u32 mipLevels = params? params->MipLevels : 1;
	Assert(mipLevels);

	GRC_ORBIS_DURANGO_TEXTURE_DESC placementCreateInfo;

	placementCreateInfo.m_Width = width;
	placementCreateInfo.m_Height = height;
	placementCreateInfo.m_Depth = depth;
	placementCreateInfo.m_ArrayDimension = 1;
	placementCreateInfo.m_NoOfMips = mipLevels;
	placementCreateInfo.m_XGFormat = grcTempformat;
	placementCreateInfo.m_ImageType = (depth == 1) ? (u8)grcImage::STANDARD : (u8)grcImage::VOLUME;
	placementCreateInfo.m_ExtraBindFlags = grcBindNone;
	placementCreateInfo.m_TileMode = grcodTileMode_Display_LinearAligned;

	if(params)
		if(params->Type == grcTextureFactory::TextureCreateParams::RENDERTARGET)
			placementCreateInfo.m_ExtraBindFlags |= (u16)grcBindRenderTarget;

	bool forceLinear = false;
	if(params && params->Format == grcTextureFactory::TextureCreateParams::LINEAR)
		forceLinear = true;

	if(forceLinear == false)
	{
		sce::Gnm::TileMode tileMode;
		grcTextureFormat grctf = ConvertTogrcFormat((GRC_TEMP_XG_FORMAT)grcTempformat);
		sce::Gnm::DataFormat format = grcTextureGNMFormats::grctf_to_Orbis[grctf];
		const int32_t tilingStatus = sce::GpuAddress::computeSurfaceTileMode(&tileMode, depth>1? sce::GpuAddress::kSurfaceTypeTextureVolume : sce::GpuAddress::kSurfaceTypeTextureFlat,format,1);
		if (tilingStatus != sce::GpuAddress::kStatusSuccess)
		{
			Errorf("computeSurfaceTileMode failed (code 0x%x) for texture '%s', width=%d, height=%d, depth=%d, format=0x%x",
				tilingStatus, GetName(), width, height, depth, format.m_asInt);
			tileMode = sce::Gnm::kTileModeThin_2dThin;
		}
		Assertf(format.m_asInt != sce::Gnm::kDataFormatInvalid.m_asInt,"unsupported Create format %d",format.m_asInt);
		placementCreateInfo.m_TileMode = OrbisTogrcOrbisDurangoTileMode(tileMode);
	}

	// Record this info.
	SetFromDescription(placementCreateInfo);

	sce::Gnm::SizeAlign result = InitGnmTexture();
	m_pGraphicsMem = allocateVideoSharedMemory(result.m_size,result.m_align);
	m_GraphicsMemorySize = result.m_size;
	GetGnmTexture().setBaseAddress256ByteBlocks((uint64_t)m_pGraphicsMem >> 8);
	ComputeMipOffsets();

	//--------------------------------------------------------------------------------------------------//

	/*
	sce::Gnm::TileMode tileMode;
	sce::Gnm::DataFormat format = grctf_to_Orbis[ConvertTogrcFormat(grcTempformat)];
	if (!sce::GpuAddress::computeSurfaceTileMode(&tileMode, depth>1? sce::GpuAddress::kSurfaceTypeTextureVolume : sce::GpuAddress::kSurfaceTypeTextureFlat,format,1))
		Errorf("computeSurfaceTileMode failed");
	Assertf(format.m_asInt != sce::Gnm::kDataFormatInvalid.m_asInt,"unsupported Create format %d",format.m_asInt);
	if (params && params->Format == grcTextureFactory::TextureCreateParams::LINEAR)
		tileMode = sce::Gnm::kTileModeDisplay_LinearAligned;

	sce::Gnm::SizeAlign result;
	if (depth > 1)
		result = GetGnmTexture().initAs3d(width,height,depth,mipLevels,format,tileMode);
	else
		result = GetGnmTexture().initAs2d(width,height,mipLevels,format,tileMode,sce::Gnm::kNumSamples1);

	m_OwnsGraphicMem = true;
	m_pGraphicsMem = allocateVideoSharedMemory(result.m_size,result.m_align);
	GetGnmTexture().setBaseAddress256ByteBlocks((uint64_t)m_pGraphicsMem >> 8);

	for (u32 i=0; i<mipLevels; i++) 
	{
		uint64_t size;
		uint64_t offset;
	#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
		sce::GpuAddress::computeSurfaceOffsetAndSize(&offset,&size,&GetGnmTexture(),i,0);
	#else
		sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&offset,&size,&GetGnmTexture(),i,0);
	#endif
		GetUserMemory()->m_pMemoryOffsets[i] = offset;
	}
	m_GraphicsMemorySize = result.m_size;
	m_CachedTexturePtr = &GetGnmTexture();
	*/

}


sce::Gnm::SizeAlign grcTextureGNM::InitGnmTexture()
{
	sce::Gnm::SizeAlign result;
	sce::Gnm::DataFormat format = grcTextureGNMFormats::grctf_to_Orbis[ConvertTogrcFormat((GRC_TEMP_XG_FORMAT)m_Texture.GetFormat())];

	if (GetDepth() > 1)
		result = GetGnmTexture().initAs3d(GetWidth(),GetHeight(),GetDepth(),GetMipMapCount(),format, grcOrbisDurangoTileModeToOrbis((grcOrbisDurangoTileMode)m_Texture.GetTileMode()));
	else
		result = GetGnmTexture().initAs2d(GetWidth(),GetHeight(),GetMipMapCount(),format,grcOrbisDurangoTileModeToOrbis((grcOrbisDurangoTileMode)m_Texture.GetTileMode()),
#if SCE_ORBIS_SDK_VERSION >= (0x01700000u)
			sce::Gnm::kNumFragments1);
#else
			sce::Gnm::kNumSamples1);
#endif //SCE_ORBIS_SDK_VERSION

	m_CachedTexturePtr = &GetGnmTexture();
	return result;
}


void grcTextureGNM::ComputeMipOffsets()
{
	for (u32 i=0; i<GetMipMapCount(); i++) 
	{
		uint64_t size;
		uint64_t offset;
#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
		sce::GpuAddress::computeSurfaceOffsetAndSize(&offset,&size,&GetGnmTexture(),i,0);
#else
		sce::GpuAddress::computeTextureSurfaceOffsetAndSize(&offset,&size,&GetGnmTexture(),i,0);
#endif
		SetLockInfo(i, (u32)offset, GetStride(i));
	}
}


bool grcTextureGNM::Copy(const grcImage *image)
{
	for (int mip=0; image; ++mip, image=image->GetNext())
		CopyBits(mip,image->GetBits());
	return true;
}


bool grcTextureGNM::Copy(const grcTexture *pTexture, s32 mipLevel)
{
	if (!AssertVerify( pTexture && pTexture!=this &&
		GetMSAA()			== pTexture->GetMSAA() &&
		GetWidth()			== pTexture->GetWidth() &&
		GetHeight()			== pTexture->GetHeight() &&
		GetBitsPerPixel()	== pTexture->GetBitsPerPixel() ))
	{
		return false;
	}

	if (mipLevel<0)
	{
		for (mipLevel=0; mipLevel<GetMipMapCount(); ++mipLevel)
		{
			Copy(pTexture,mipLevel);
		}
	}else	{
		Assert( mipLevel>=0 && mipLevel<GetMipMapCount() && mipLevel<pTexture->GetMipMapCount() );
		void *const dst = (char*)GetGnmTexture().getBaseAddress() + GetMemoryOffset(0,mipLevel);
		void *const src = (char*)pTexture->GetTexturePtr()->getBaseAddress() +
			static_cast<const grcTextureGNM*>(pTexture)->GetMemoryOffset(0,mipLevel);
		const unsigned size = GetMemoryOffset(0,mipLevel+1) - GetMemoryOffset(0,mipLevel);
		gfxc.copyData( dst, src, size, sce::Gnm::kDmaDataBlockingEnable );
	}
	return true;
}


void grcTextureGNM::CopyBits(int mip,const void *bits)
{
	Assert(bits);
	Assert(mip <= GetGnmTexture().getLastMipLevel());
	uint64_t offset, pitch;
	GetLockInfo(mip, offset, pitch);
	sce::GpuAddress::TilingParameters tp;
	tp.initFromTexture(&GetGnmTexture(), mip, 0);
	sce::GpuAddress::tileSurface(((char*)m_pGraphicsMem + offset),bits,&tp);
}

void grcTextureGNM::Resize(u32 width, u32 height)
{
	Assert(false);
}

void grcTextureGNM::TileInPlace()
{
	//note this only fills in the top mip and doesn't deal with volume, also you have to have previously locked this
	Assertf(m_pGraphicsMem, "No texture info to tile!");

	sysMemStartTemp();

	u32 physicalSize = GetStride(0) * GetRowCount(0);

	char* pMipDataLinear = rage_new char[physicalSize];
	sysMemCpy(pMipDataLinear, m_pGraphicsMem, physicalSize);

	CopyBits(0, pMipDataLinear);

	delete [] pMipDataLinear;

	sysMemEndTemp();
}

}	// namespace rage

#endif // __GNM
