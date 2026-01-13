#ifndef GRCORE_ORBIS_DURANGO_TEXTURE_BASE_H
#define GRCORE_ORBIS_DURANGO_TEXTURE_BASE_H

#include "grcore\config_switches.h"
#include "grcore\texture.h"

#if RSG_DURANGO || RSG_ORBIS || (RSG_PC && __64BIT && __RESOURCECOMPILER)


#if RSG_DURANGO || (RSG_PC && __RESOURCECOMPILER && __64BIT)
enum XG_TILE_MODE;
#endif // RSG_DURANGO || (RSG_PC && __RESOURCECOMPILER && __64BIT)

namespace rage
{

enum GRC_TEMP_XG_FORMAT
{
	GRC_TEMP_XG_FORMAT_UNKNOWN                     = 0,
	GRC_TEMP_XG_FORMAT_R32G32B32A32_TYPELESS       = 1,
	GRC_TEMP_XG_FORMAT_R32G32B32A32_FLOAT          = 2,
	GRC_TEMP_XG_FORMAT_R32G32B32A32_UINT           = 3,
	GRC_TEMP_XG_FORMAT_R32G32B32A32_SINT           = 4,
	GRC_TEMP_XG_FORMAT_R32G32B32_TYPELESS          = 5,
	GRC_TEMP_XG_FORMAT_R32G32B32_FLOAT             = 6,
	GRC_TEMP_XG_FORMAT_R32G32B32_UINT              = 7,
	GRC_TEMP_XG_FORMAT_R32G32B32_SINT              = 8,
	GRC_TEMP_XG_FORMAT_R16G16B16A16_TYPELESS       = 9,
	GRC_TEMP_XG_FORMAT_R16G16B16A16_FLOAT          = 10,
	GRC_TEMP_XG_FORMAT_R16G16B16A16_UNORM          = 11,
	GRC_TEMP_XG_FORMAT_R16G16B16A16_UINT           = 12,
	GRC_TEMP_XG_FORMAT_R16G16B16A16_SNORM          = 13,
	GRC_TEMP_XG_FORMAT_R16G16B16A16_SINT           = 14,
	GRC_TEMP_XG_FORMAT_R32G32_TYPELESS             = 15,
	GRC_TEMP_XG_FORMAT_R32G32_FLOAT                = 16,
	GRC_TEMP_XG_FORMAT_R32G32_UINT                 = 17,
	GRC_TEMP_XG_FORMAT_R32G32_SINT                 = 18,
	GRC_TEMP_XG_FORMAT_R32G8X24_TYPELESS           = 19,
	GRC_TEMP_XG_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
	GRC_TEMP_XG_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
	GRC_TEMP_XG_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
	GRC_TEMP_XG_FORMAT_R10G10B10A2_TYPELESS        = 23,
	GRC_TEMP_XG_FORMAT_R10G10B10A2_UNORM           = 24,
	GRC_TEMP_XG_FORMAT_R10G10B10A2_UINT            = 25,
	GRC_TEMP_XG_FORMAT_R11G11B10_FLOAT             = 26,
	GRC_TEMP_XG_FORMAT_R8G8B8A8_TYPELESS           = 27,
	GRC_TEMP_XG_FORMAT_R8G8B8A8_UNORM              = 28,
	GRC_TEMP_XG_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
	GRC_TEMP_XG_FORMAT_R8G8B8A8_UINT               = 30,
	GRC_TEMP_XG_FORMAT_R8G8B8A8_SNORM              = 31,
	GRC_TEMP_XG_FORMAT_R8G8B8A8_SINT               = 32,
	GRC_TEMP_XG_FORMAT_R16G16_TYPELESS             = 33,
	GRC_TEMP_XG_FORMAT_R16G16_FLOAT                = 34,
	GRC_TEMP_XG_FORMAT_R16G16_UNORM                = 35,
	GRC_TEMP_XG_FORMAT_R16G16_UINT                 = 36,
	GRC_TEMP_XG_FORMAT_R16G16_SNORM                = 37,
	GRC_TEMP_XG_FORMAT_R16G16_SINT                 = 38,
	GRC_TEMP_XG_FORMAT_R32_TYPELESS                = 39,
	GRC_TEMP_XG_FORMAT_D32_FLOAT                   = 40,
	GRC_TEMP_XG_FORMAT_R32_FLOAT                   = 41,
	GRC_TEMP_XG_FORMAT_R32_UINT                    = 42,
	GRC_TEMP_XG_FORMAT_R32_SINT                    = 43,
	GRC_TEMP_XG_FORMAT_R24G8_TYPELESS              = 44,
	GRC_TEMP_XG_FORMAT_D24_UNORM_S8_UINT           = 45,
	GRC_TEMP_XG_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
	GRC_TEMP_XG_FORMAT_X24_TYPELESS_G8_UINT        = 47,
	GRC_TEMP_XG_FORMAT_R8G8_TYPELESS               = 48,
	GRC_TEMP_XG_FORMAT_R8G8_UNORM                  = 49,
	GRC_TEMP_XG_FORMAT_R8G8_UINT                   = 50,
	GRC_TEMP_XG_FORMAT_R8G8_SNORM                  = 51,
	GRC_TEMP_XG_FORMAT_R8G8_SINT                   = 52,
	GRC_TEMP_XG_FORMAT_R16_TYPELESS                = 53,
	GRC_TEMP_XG_FORMAT_R16_FLOAT                   = 54,
	GRC_TEMP_XG_FORMAT_D16_UNORM                   = 55,
	GRC_TEMP_XG_FORMAT_R16_UNORM                   = 56,
	GRC_TEMP_XG_FORMAT_R16_UINT                    = 57,
	GRC_TEMP_XG_FORMAT_R16_SNORM                   = 58,
	GRC_TEMP_XG_FORMAT_R16_SINT                    = 59,
	GRC_TEMP_XG_FORMAT_R8_TYPELESS                 = 60,
	GRC_TEMP_XG_FORMAT_R8_UNORM                    = 61,
	GRC_TEMP_XG_FORMAT_R8_UINT                     = 62,
	GRC_TEMP_XG_FORMAT_R8_SNORM                    = 63,
	GRC_TEMP_XG_FORMAT_R8_SINT                     = 64,
	GRC_TEMP_XG_FORMAT_A8_UNORM                    = 65,
	GRC_TEMP_XG_FORMAT_R1_UNORM                    = 66,
	GRC_TEMP_XG_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
	GRC_TEMP_XG_FORMAT_R8G8_B8G8_UNORM             = 68,
	GRC_TEMP_XG_FORMAT_G8R8_G8B8_UNORM             = 69,
	GRC_TEMP_XG_FORMAT_BC1_TYPELESS                = 70,
	GRC_TEMP_XG_FORMAT_BC1_UNORM                   = 71,
	GRC_TEMP_XG_FORMAT_BC1_UNORM_SRGB              = 72,
	GRC_TEMP_XG_FORMAT_BC2_TYPELESS                = 73,
	GRC_TEMP_XG_FORMAT_BC2_UNORM                   = 74,
	GRC_TEMP_XG_FORMAT_BC2_UNORM_SRGB              = 75,
	GRC_TEMP_XG_FORMAT_BC3_TYPELESS                = 76,
	GRC_TEMP_XG_FORMAT_BC3_UNORM                   = 77,
	GRC_TEMP_XG_FORMAT_BC3_UNORM_SRGB              = 78,
	GRC_TEMP_XG_FORMAT_BC4_TYPELESS                = 79,
	GRC_TEMP_XG_FORMAT_BC4_UNORM                   = 80,
	GRC_TEMP_XG_FORMAT_BC4_SNORM                   = 81,
	GRC_TEMP_XG_FORMAT_BC5_TYPELESS                = 82,
	GRC_TEMP_XG_FORMAT_BC5_UNORM                   = 83,
	GRC_TEMP_XG_FORMAT_BC5_SNORM                   = 84,
	GRC_TEMP_XG_FORMAT_B5G6R5_UNORM                = 85,
	GRC_TEMP_XG_FORMAT_B5G5R5A1_UNORM              = 86,
	GRC_TEMP_XG_FORMAT_B8G8R8A8_UNORM              = 87,
	GRC_TEMP_XG_FORMAT_B8G8R8X8_UNORM              = 88,
	GRC_TEMP_XG_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
	GRC_TEMP_XG_FORMAT_B8G8R8A8_TYPELESS           = 90,
	GRC_TEMP_XG_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
	GRC_TEMP_XG_FORMAT_B8G8R8X8_TYPELESS           = 92,
	GRC_TEMP_XG_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
	GRC_TEMP_XG_FORMAT_BC6H_TYPELESS               = 94,
	GRC_TEMP_XG_FORMAT_BC6H_UF16                   = 95,
	GRC_TEMP_XG_FORMAT_BC6H_SF16                   = 96,
	GRC_TEMP_XG_FORMAT_BC7_TYPELESS                = 97,
	GRC_TEMP_XG_FORMAT_BC7_UNORM                   = 98,
	GRC_TEMP_XG_FORMAT_BC7_UNORM_SRGB              = 99,
	GRC_TEMP_XG_FORMAT_AYUV                        = 100,
	GRC_TEMP_XG_FORMAT_Y410                        = 101,
	GRC_TEMP_XG_FORMAT_Y416                        = 102,
	GRC_TEMP_XG_FORMAT_NV12                        = 103,
	GRC_TEMP_XG_FORMAT_P010                        = 104,
	GRC_TEMP_XG_FORMAT_P016                        = 105,
	GRC_TEMP_XG_FORMAT_420_OPAQUE                  = 106,
	GRC_TEMP_XG_FORMAT_YUY2                        = 107,
	GRC_TEMP_XG_FORMAT_Y210                        = 108,
	GRC_TEMP_XG_FORMAT_Y216                        = 109,
	GRC_TEMP_XG_FORMAT_NV11                        = 110,
	GRC_TEMP_XG_FORMAT_AI44                        = 111,
	GRC_TEMP_XG_FORMAT_IA44                        = 112,
	GRC_TEMP_XG_FORMAT_P8                          = 113,
	GRC_TEMP_XG_FORMAT_A8P8                        = 114,
	GRC_TEMP_XG_FORMAT_B4G4R4A4_UNORM              = 115,
	GRC_TEMP_XG_FORMAT_R10G10B10_7E3_A2_FLOAT      = 116,
	GRC_TEMP_XG_FORMAT_R10G10B10_6E4_A2_FLOAT      = 117,
	GRC_TEMP_XG_FORMAT_FORCE_UINT                  = 0xffffffff
};


// sce::Gnm::TileMode and XG_TILE_MODE are the same (see C:\Program Files (x86)\SCE\ORBIS SDKs\1.000\target\include_common\gnm\constants.h & C:\Program Files (x86)\Microsoft Durango XDK\xdk\include\um\xg.h).
enum grcOrbisDurangoTileMode
{
	// Depth modes (for depth buffers)
	grcodTileMode_Depth_2dThin_64                   = 0x00000000, // XG_TILE_MODE_COMP_DEPTH_0
	grcodTileMode_Depth_2dThin_128                  = 0x00000001, // XG_TILE_MODE_COMP_DEPTH_1
	grcodTileMode_Depth_2dThin_256                  = 0x00000002, // XG_TILE_MODE_COMP_DEPTH_2
	grcodTileMode_Depth_2dThin_512                  = 0x00000003, // XG_TILE_MODE_COMP_DEPTH_3
	grcodTileMode_Depth_2dThin_1K                   = 0x00000004, // XG_TILE_MODE_COMP_DEPTH_4
	grcodTileMode_Depth_1dThin                      = 0x00000005, // XG_TILE_MODE_UNC_DEPTH_5
	grcodTileMode_Depth_2dThinPrt_256               = 0x00000006, // XG_TILE_MODE_UNC_DEPTH_6
	grcodTileMode_Depth_2dThinPrt_1K                = 0x00000007, // XG_TILE_MODE_UNC_DEPTH_7
	// Display modes
	grcodTileMode_Display_LinearAligned				= 0x00000008, // XG_TILE_MODE_LINEAR
	grcodTileMode_Display_1dThin                    = 0x00000009, // XG_TILE_MODE_DISPLAY
	grcodTileMode_Display_2dThin_OrbisOnly          = 0x0000000A, // XG_TILE_MODE_RESERVED_10
	grcodTileMode_Display_ThinPrt_OrbisOnly         = 0x0000000B, // XG_TILE_MODE_RESERVED_11
	grcodTileMode_Display_2dThinPrt_OrbisOnly	    = 0x0000000C, // XG_TILE_MODE_RESERVED_12
	// Thin modes (for non-displayable 1D/2D/3D surfaces)
	grcodTileMode_Thin_1dThin                       = 0x0000000D, // XG_TILE_MODE_1D_THIN
	grcodTileMode_Thin_2dThin                       = 0x0000000E, // XG_TILE_MODE_2D_THIN
	grcodTileMode_Thin_3dThin_OrbisOnly             = 0x0000000F, // XG_TILE_MODE_RESERVED_15
	grcodTileMode_Thin_ThinPrt_OrbisOnly            = 0x00000010, // XG_TILE_MODE_RESERVED_16
	grcodTileMode_Thin_2dThinPrt_OrbisOnly          = 0x00000011, // XG_TILE_MODE_RESERVED_17
	grcodTileMode_Thin_3dThinPrt_OrbisOnly          = 0x00000012, // XG_TILE_MODE_RESERVED_18
	// Thick modes (for 3D textures)
	grcodTileMode_Thick_1dThick_OrbisOnly			= 0x00000013, // XG_TILE_MODE_RESERVED_19
	grcodTileMode_Thick_2dThick_OrbisOnly			= 0x00000014, // XG_TILE_MODE_RESERVED_20
	grcodTileMode_Thick_3dThick_OrbisOnly			= 0x00000015, // XG_TILE_MODE_RESERVED_21
	grcodTileMode_Thick_ThickPrt_OrbisOnly			= 0x00000016, // XG_TILE_MODE_RESERVED_22
	grcodTileMode_Thick_2dThickPrt_OrbisOnly        = 0x00000017, // XG_TILE_MODE_RESERVED_23
	grcodTileMode_Thick_3dThickPrt_OrbisOnly        = 0x00000018, // XG_TILE_MODE_RESERVED_24
	grcodTileMode_Thick_2dXThick_OrbisOnly          = 0x00000019, // XG_TILE_MODE_RESERVED_25
	grcodTileMode_Thick_3dXThick_OrbisOnly			= 0x0000001A, // XG_TILE_MODE_RESERVED_26
	// Rotated modes -- not used
	grcodTileMode_Rotated_1dThin_OrbisOnly			= 0x0000001B, // XG_TILE_MODE_RESERVED_27
	grcodTileMode_Rotated_2dThin_OrbisOnly			= 0x0000001C, // XG_TILE_MODE_RESERVED_28
	grcodTileMode_Rotated_ThinPrt_OrbisOnly			= 0x0000001D, // XG_TILE_MODE_RESERVED_29
	grcodTileMode_Rotated_2dThinPrt_OrbisOnly		= 0x0000001E, // XG_TILE_MODE_RESERVED_30
	// Hugely inefficient linear display mode -- do not use!
	grcodTileMode_Display_LinearGeneral             = 0x0000001F, // XG_TILE_MODE_LINEAR_GENERAL
	grcodTileMode_Max								= 0x00000020
};


/*======================================================================================================================================*/
/* Format helper functions.																												*/
/*======================================================================================================================================*/

// Format conversion functions in case SDK the defines change.
#if RSG_ORBIS
sce::Gnm::TileMode grcOrbisDurangoTileModeToOrbis(grcOrbisDurangoTileMode tileMode);
grcOrbisDurangoTileMode OrbisTogrcOrbisDurangoTileMode(sce::Gnm::TileMode tileMode);
#endif // RSG_ORBIS

#if RSG_DURANGO || (RSG_PC && __RESOURCECOMPILER && __64BIT)
XG_TILE_MODE grcOrbisDurangoTileModeToDurango(grcOrbisDurangoTileMode tileMode);
grcOrbisDurangoTileMode DurangoTogrcOrbisDurangoTileMode(XG_TILE_MODE tileMode);
#endif // RSG_DURANGO || (RSG_PC && __RESOURCECOMPILER && __64BIT)


GRC_TEMP_XG_FORMAT ConvertToXGFormat(grcTextureFormat eFormat);
grcTextureFormat ConvertTogrcFormat(GRC_TEMP_XG_FORMAT fmt);
u32 GetBitsPerPixelFromFormat(grcTextureFormat eFormat);
bool IsFormatDXTCompressed(grcTextureFormat eFormat);
grcTexture::ChannelBits FindUsedChannelsFromFormat(GRC_TEMP_XG_FORMAT eFormat);
GRC_TEMP_XG_FORMAT GetXGFormatFromGRCImageFormat(u32 grcImageFormat);

/*======================================================================================================================================*/
/* grcOrbisDurangoTextureBase classes.																									*/
/*======================================================================================================================================*/

#define DURANGO_PLACEMENT_TEXTURE_BASE_CLASS grcTexture

struct GRC_ORBIS_DURANGO_TEXTURE_DESC
{
	GRC_ORBIS_DURANGO_TEXTURE_DESC()
	{
		memset(this, 0, sizeof(GRC_ORBIS_DURANGO_TEXTURE_DESC));
	}

	u16 m_Width;
	u16 m_Height;
	u8 m_Depth;
	u8 m_ArrayDimension;
	u8 m_NoOfMips;
	GRC_TEMP_XG_FORMAT m_XGFormat;
	grcOrbisDurangoTileMode m_TileMode;
	u16 m_ImageType; // ImageType.
	u16 m_ExtraBindFlags; // grcBindType.
};


class grcTextureDummy
{
public:
	grcTextureDummy(u8 unused) { (void)unused; }
	grcTextureDummy(datResource &rsc) { (void)rsc; }
	~grcTextureDummy() {}
#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s) { (void)s; }
#endif // __DECLARESTRUCT
public:
	grcCellGcmTextureWrapper	m_Texture;
};


// In u64s.
#define GRC_ORBIS_DURANGO_TEXTURE_USER_MEM_SIZE 6

class grcOrbisDurangoTextureBase : public DURANGO_PLACEMENT_TEXTURE_BASE_CLASS
{
public:
	struct OFFSET_AND_PITCH
	{
		void Set(u64 offset, u64 pitch)
		{
			Assertf((pitch & ~(((u64)1 << 20) - (u64)1)) == 0, "grcOrbisDurangoTextureBase::OFFSET_AND_PITCH::Set()...Pitch too big (Max 20 bits)\n");
			Assertf((offset & ~(((u64)1 << 43) - (u64)1)) == 0, "grcOrbisDurangoTextureBase::OFFSET_AND_PITCH::Set()...Offset too big (Max 43 bits)\n");
			offsetAndPitch = (offset << 20) | pitch;
		}
		void Get(u64 &offset, u64 &pitch)
		{
			offset = offsetAndPitch >> 20;
			pitch = offsetAndPitch & ((1 << 20) - 1);
		}
		u64 offsetAndPitch;
	};
public:
	grcOrbisDurangoTextureBase();
	grcOrbisDurangoTextureBase(u8 type);
	grcOrbisDurangoTextureBase(GRC_ORBIS_DURANGO_TEXTURE_DESC &info, u8 type);
	grcOrbisDurangoTextureBase(datResource &rsc);
	~grcOrbisDurangoTextureBase();
public:
	void SetFromDescription(GRC_ORBIS_DURANGO_TEXTURE_DESC &desc);
	void SetLockInfo(u32 Mip, u64 offset, u64 pitch);
	void GetLockInfo(u32 Mip, u64 &offset, u64 &pitch) const;
	bool GetOwnsAllocations();
	void SetUsesPreAllocatedMemory(bool flag);
	bool GetUsesPreAllocatedMemory();
public:
	// grcTexture functions.
	int						GetWidth		(void) const { return(m_Texture.GetWidth());  }
	int						GetHeight		(void) const { return(m_Texture.GetHeight()); }
	int						GetDepth		(void) const { return(m_Texture.GetDepth());  }
	int						GetArraySize	(void) const { return(m_Texture.GetDimension()); }
	int						GetMipMapCount	(void) const { return(m_Texture.GetMipMap()); }
	int						GetImageType	(void) const { return(m_Texture.GetImageType()); }
	bool					Copy(const grcImage *pImage) { grcAssertf(0, "grcOrbisDurangoTextureBase::Copy()...Not implemented"); (void)pImage; return true; };
	int						GetBitsPerPixel (void) const;
	grcTexture::ChannelBits	FindUsedChannels(void) const;
	int						GetStride(u32 uMipLevel) const;
	int						GetRowCount(u32 uMipLevel) const;

	bool					Copy2D(const void *pSrc, const grcPoint & oSrcDim, const grcRect & oDstRect, const grcTextureLock &lock, s32 iMipLevel);

	void					EnsureGpuWritable();

	// HD swap function.
	static					void* PeformHDOverrideSwap(grcOrbisDurangoTextureBase *pA, grcOrbisDurangoTextureBase *pB, void* pOldHdAddr, void*& pDeferredFreePtr, size_t& deferredFreeSize);

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif //__DECLARESTRUCT

protected:
	void *m_pGraphicsMem;
	size_t m_GraphicsMemorySize;
	OFFSET_AND_PITCH *m_pLockInfoPtr;
#if __RESOURCECOMPILER
	datPadding<GRC_ORBIS_DURANGO_TEXTURE_USER_MEM_SIZE, u64> m_UserMemory;
#else // __RESOURCECOMPILER
	u64 m_UserMemory[GRC_ORBIS_DURANGO_TEXTURE_USER_MEM_SIZE];
#endif // __RESOURCECOMPILER
};


/*======================================================================================================================================*/
/* grcOrbisDurangoBufferBase classes.																									*/
/*======================================================================================================================================*/


class grcOrbisDurangoBufferBase
{
public:
	grcOrbisDurangoBufferBase();
	grcOrbisDurangoBufferBase(class datResource& rsc);
	~grcOrbisDurangoBufferBase();
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

public:
	mutable u32 m_Flags;
	size_t m_Size;
	void *m_pGraphicsMemory;
	u64 m_UserMemory[1];
};

} // namespace rage

#endif // RSG_DURANGO || RSG_ORBIS || (RSG_PC && __64BIT && __RESOURCECOMPILER)

#endif // GRCORE_ORBIS_DURANGO_TEXTURE_BASE_H
