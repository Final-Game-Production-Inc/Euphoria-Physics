//
// grcore/wrapper_gcm.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//
#include "wrapper_gcm.h"
#include "system/memory.h"
#include "system/nelem.h"

#include "data/resource.h"
#include "grcore/device.h"
#include "grcore/texture.h"
#include "math/amath.h"
#include "math/intrinsics.h"
#include "string/string.h"

#include <stdio.h>

#if __PPU || __WIN32PC

#pragma warning(push)
#pragma warning(disable: 4995)

class rmcDrawable;
#if __PPU
#include "rmcore/drawablespu.h"
#include <sys/sys_fixed_addr.h>
#else
#include "grcore/grcorespu.h"
#endif

#include "data/struct.h"

#if __DECLARESTRUCT && !__PPU
void CellGcmTexture::DeclareStruct(rage::datTypeStruct &s) {
	STRUCT_BEGIN(CellGcmTexture);
	STRUCT_FIELD(format);
	STRUCT_FIELD(mipmap);
	STRUCT_FIELD(dimension);
	STRUCT_FIELD(cubemap);

	STRUCT_FIELD(remap);

	STRUCT_FIELD(width);
	STRUCT_FIELD(height);
	STRUCT_FIELD(depth);
	STRUCT_FIELD(location);
	STRUCT_FIELD(_padding);

	STRUCT_FIELD(pitch);
#if __64BIT	// Weird case.. it's not really a pointer so don't treat it like one
	STRUCT_IGNORE(offset);
#else
	STRUCT_FIELD_VP(offset);
#endif
	STRUCT_END();
}
#endif

#if GCM_HUD
extern uint32_t _cellGcmHUDGetBaseReport();
extern uint32_t _cellGcmHUDGetNumReports();
#endif // GCM_HUD

namespace rage {

#if __PPU && GCM_HUD

#define	GCM_HUD_HEAP_SIZE	(16 * 1024 * 1024)
#define	GCM_HUD_XDR_SIZE	(16 * 1024 * 1024)

bool grcCellGcmHudIsEnabled = false;
static void* s_GcmHudHeap = NULL;
static void* s_GcmHudXdr = NULL;
static CellVideoOutBufferColorFormat s_GcmHudFormat;

void grcCellGcmHudInitialise(CellVideoOutBufferColorFormat format, bool enable)
{
	Assert(!s_GcmHudXdr);
	grcCellGcmHudIsEnabled = false;
	s_GcmHudHeap = sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_RESOURCE_VIRTUAL)->RAGE_LOG_ALLOCATE(GCM_HUD_HEAP_SIZE,128);
	s_GcmHudXdr = sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_RESOURCE_VIRTUAL)->RAGE_LOG_ALLOCATE(GCM_HUD_XDR_SIZE,128);
	s_GcmHudFormat = format;

	grcCellGcmHudSetEnable(enable);
}

void grcCellGcmHudSetEnable(bool enable)
{
	static bool firstTime = true;

	Assert(s_GcmHudXdr);

	if (enable != grcCellGcmHudIsEnabled)
	{
		if (enable)
		{
			// BUG - HUD can only be enabled and disabled one.
			if (!firstTime)
			{
				AssertMsg(false, "Gcm Hud can only be enabled and disabled once - sorry, Sony needs to fix this");
				return;
			}

			cellGcmHUDSetHeap(s_GcmHudHeap,GCM_HUD_HEAP_SIZE);
			cellGcmHUDSetVideoOutFormat(s_GcmHudFormat);
			cellGcmHUDInitEx(s_GcmHudXdr,GCM_HUD_XDR_SIZE);
			if (firstTime)
			{
				gcm::TimeStampRegistrar::Reserve(_cellGcmHUDGetBaseReport(), _cellGcmHUDGetNumReports());
				firstTime = false;
			}
		}
		else
		{
			cellGcmHUDExit();
		}
		grcCellGcmHudIsEnabled = enable;
		grcDisplayf("EnableGcmHud: %s", enable?"true":"false");
	}
}

#endif // __PPU && GCM_HUD

PPU_ONLY(BANK_ONLY(bool g_AreRenderTargetsBound = false;))

namespace gcm {

u32 g_Finisher;
u8 *g_LocalAddress;	// from CellGcmConfig; origin of local memory mapped into PPU address space.
u8 *g_IoAddress;	// from CellGcmConfig; base address of the first byte of PPU memory visible from RSX
u32 g_LocalSize;	// from CellGcmConfig; size of local memory
u32 g_IoSize;		// from CellGcmConfig; size of host memory

#if __PPU
uint32_t g_LastFlush;
CellGcmControl volatile *g_ControlRegister;
#endif

namespace Shadow {
	u32 g_State[COUNT];
}

void Init()
{
#if __PPU
#if !__OPTIMIZED
	cellGcmSetDebugOutputLevel(CELL_GCM_DEBUG_LEVEL2);
#endif
	CellGcmConfig config;
	cellGcmGetConfiguration(&config);

	g_LocalAddress = reinterpret_cast<u8*>(config.localAddress);
	g_IoAddress = reinterpret_cast<u8*>(config.ioAddress);
	Assert(config.localAddress == (void*)RSX_FB_BASE_ADDR);
	g_LocalSize = config.localSize;
	g_IoSize = config.ioSize;
	grcDisplayf("*********** localsize = %d",config.localSize>>20);
	grcDisplayf("Local memory mapped into PPU address space at [%p,%p)",g_LocalAddress,g_LocalAddress+g_LocalSize);
	grcDisplayf("PPU memory visible to GPU from [%p,%p)",g_IoAddress,g_IoAddress+g_IoSize);
#endif
	// Initialize state cache to crap
	memset(Shadow::g_State,0xCE,sizeof(Shadow::g_State));
}

void Shutdown()
{
}

u32 TextureFormatBitsPerPixel(u8 format)
{
	format = StripTextureFormat(format);

	switch (format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		return 4;
	case CELL_GCM_TEXTURE_B8:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return 8;
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
	case CELL_GCM_TEXTURE_G8B8:
	case CELL_GCM_TEXTURE_R6G5B5:
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
	case CELL_GCM_TEXTURE_X16:
	case CELL_GCM_TEXTURE_R5G5B5A1:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case CELL_GCM_TEXTURE_D1R5G5B5:
		return 16;
	case CELL_GCM_TEXTURE_A8R8G8B8:
	case CELL_GCM_TEXTURE_DEPTH24_D8:
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
	case CELL_GCM_TEXTURE_Y16_X16:
	case CELL_GCM_TEXTURE_X32_FLOAT:
	case CELL_GCM_TEXTURE_D8R8G8B8:
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		return 32;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return 64;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return 128;
	default:
		Quitf(ERR_DEFAULT,"Invalid texture format %x", format);
		return 0;
	}
}

u32 TextureFormatLinesPerPitch(u8 format)
{
	format = StripTextureFormat(format);

	switch (format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return 4;
	case CELL_GCM_TEXTURE_B8:
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
	case CELL_GCM_TEXTURE_G8B8:
	case CELL_GCM_TEXTURE_R6G5B5:
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
	case CELL_GCM_TEXTURE_X16:
	case CELL_GCM_TEXTURE_R5G5B5A1:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case CELL_GCM_TEXTURE_D1R5G5B5:
	case CELL_GCM_TEXTURE_A8R8G8B8:
	case CELL_GCM_TEXTURE_DEPTH24_D8:
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
	case CELL_GCM_TEXTURE_Y16_X16:
	case CELL_GCM_TEXTURE_X32_FLOAT:
	case CELL_GCM_TEXTURE_D8R8G8B8:
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return 1;
	default:
		Quitf(ERR_DEFAULT,"Invalid texture format %x", format);
		return 0;
	}
}

bool TextureFormatSupportsSrgb(u8 format)
{
	format = StripTextureFormat(format);

	switch (format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
	case CELL_GCM_TEXTURE_B8:
	case CELL_GCM_TEXTURE_G8B8:
	case CELL_GCM_TEXTURE_A8R8G8B8:
	case CELL_GCM_TEXTURE_D8R8G8B8:
		return true;
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
	case CELL_GCM_TEXTURE_R6G5B5:
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
	case CELL_GCM_TEXTURE_X16:
	case CELL_GCM_TEXTURE_R5G5B5A1:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case CELL_GCM_TEXTURE_D1R5G5B5:
	case CELL_GCM_TEXTURE_DEPTH24_D8:
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
	case CELL_GCM_TEXTURE_Y16_X16:
	case CELL_GCM_TEXTURE_X32_FLOAT:
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return false;
	default:
		Quitf(ERR_DEFAULT,"Invalid texture format %x", format);
		return false;
	}
}

u32 SurfaceFormatBitsPerPixel(u8 format, bool isDepthFormat)
{
	if (isDepthFormat)
	{
		switch (format)
		{
		case CELL_GCM_SURFACE_Z16:
			return 16;
		case CELL_GCM_SURFACE_Z24S8:
			return 32;
		default:
			Quitf(ERR_DEFAULT,"Invalid surface format %x", format);
			return 0;
		}
	}
	else
	{
		switch (format)
		{
		case CELL_GCM_SURFACE_B8:
			return 8;
		case CELL_GCM_SURFACE_R5G6B5:
		case CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5:
		case CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5:
		case CELL_GCM_SURFACE_G8B8:
			return 16;
		case CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8:
		case CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8:
		case CELL_GCM_SURFACE_A8R8G8B8:
		case CELL_GCM_SURFACE_F_X32:
		case CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8:
		case CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8:
		case CELL_GCM_SURFACE_A8B8G8R8:
			return 32;
		case CELL_GCM_SURFACE_F_W16Z16Y16X16:
			return 64;
		case CELL_GCM_SURFACE_F_W32Z32Y32X32:
			return 128;
		default:
			Quitf(ERR_DEFAULT,"Invalid surface format %x", format);
			return 0;
		}
	}
}

u8 TextureToSurfaceFormat(u8 format)
{
	format = StripTextureFormat(format);

	switch (format)
	{
	case CELL_GCM_TEXTURE_A8R8G8B8:
		{
			return CELL_GCM_SURFACE_A8B8G8R8;
		}
	case CELL_GCM_TEXTURE_R5G6B5:
		{
			return CELL_GCM_SURFACE_R5G6B5;
		}
	case CELL_GCM_TEXTURE_X32_FLOAT:
		{
			return CELL_GCM_SURFACE_F_X32;
		}
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		{
			return CELL_GCM_SURFACE_F_W16Z16Y16X16;
		}
		// you have to pack the values from CELL_GCM_TEXTURE_Y16_X16
		// with the Cg instrinsic unpack_4ubyte(pack_2ushort()) into the CELL_GCM_SURFACE_A8B8G8R8 surface
	case CELL_GCM_TEXTURE_Y16_X16: // Fall through
		// you have to pack the values from CELL_GCM_TEXTURE_Y16_X16_FLOAT
		// with the Cg instrinsic unpack_4ubyte(pack_2half()) into the CELL_GCM_SURFACE_A8B8G8R8 surface
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		{
			return CELL_GCM_SURFACE_A8B8G8R8;
		}
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		{
			return CELL_GCM_SURFACE_F_W32Z32Y32X32;
		}
	case CELL_GCM_TEXTURE_B8:
		{
			return CELL_GCM_SURFACE_B8;
		}
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		{
			return CELL_GCM_SURFACE_Z16;
		}
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		{
			return CELL_GCM_SURFACE_Z24S8;
		}
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		{
			return CELL_GCM_SURFACE_Z24S8;
		}
	case CELL_GCM_TEXTURE_DEPTH16:
		{
			return CELL_GCM_SURFACE_Z16;
		}
	case CELL_GCM_TEXTURE_G8B8:
		{
			return CELL_GCM_SURFACE_G8B8;
		}
	case CELL_GCM_TEXTURE_D1R5G5B5:
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_R5G5B5A1:
		{
			// 15-but unsigned integer format, write A=1, read A=1
			return CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5;
		}
	default:
		{
			Quitf(ERR_DEFAULT,"Invalid texture format %x", format);
			return 0;
		}
	}
}

bool IsFloatingPointTextureFormat(u8 format)
{
	format = StripTextureFormat(format);

	switch (format)
	{
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
	case CELL_GCM_TEXTURE_X32_FLOAT:
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		return true;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
	case CELL_GCM_TEXTURE_B8:
	case CELL_GCM_TEXTURE_G8B8:
	case CELL_GCM_TEXTURE_A8R8G8B8:
	case CELL_GCM_TEXTURE_D8R8G8B8:
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
	case CELL_GCM_TEXTURE_R6G5B5:
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_X16:
	case CELL_GCM_TEXTURE_R5G5B5A1:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case CELL_GCM_TEXTURE_D1R5G5B5:
	case CELL_GCM_TEXTURE_DEPTH24_D8:
	case CELL_GCM_TEXTURE_Y16_X16:
		return false;
	default:
		grcErrorf("Invalid texture format %x", format);
		return false;
	}
}

bool IsFloatingPointColorSurfaceFormat(u8 format)
{
	switch (format)
	{
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:
	case CELL_GCM_SURFACE_F_W32Z32Y32X32:
	case CELL_GCM_SURFACE_F_X32:
		return true;
	case CELL_GCM_SURFACE_X1R5G5B5_Z1R5G5B5:
	case CELL_GCM_SURFACE_X1R5G5B5_O1R5G5B5:
	case CELL_GCM_SURFACE_R5G6B5:
	case CELL_GCM_SURFACE_X8R8G8B8_Z8R8G8B8:
	case CELL_GCM_SURFACE_X8R8G8B8_O8R8G8B8:
	case CELL_GCM_SURFACE_A8R8G8B8:
	case CELL_GCM_SURFACE_B8:
	case CELL_GCM_SURFACE_G8B8:
	case CELL_GCM_SURFACE_X8B8G8R8_Z8B8G8R8:
	case CELL_GCM_SURFACE_X8B8G8R8_O8B8G8R8:
	case CELL_GCM_SURFACE_A8B8G8R8:
		return false;
	default:
		grcErrorf("Invalid surface format %x", format);
		return false;
	}
}

u32 GetSurfaceAlignment(bool inTiledMemory, bool isZCull)
{
	// Keep these in descending order
	if (inTiledMemory)
	{
		return 65536;
	}
	else if (isZCull)
	{
		return 4096;
	}
	else
	{
		return 128;
	}
}

u32 GetSurfaceWidth(u16 width)
{
	return (width + 63) & ~63;
}

u32 GetSurfaceHeight(u32 height, bool inLocalMemory)
{
	const u32 alignment = inLocalMemory ? 31 : 63;
	return (height + alignment) & ~alignment;
}

u32 GetSurfaceTiledPitch(u16 width, u32 bitsPerPixel, bool ASSERT_ONLY(isSwizzled))
{
	Assert(!isSwizzled);
#if !__WIN32PC && __GCM
	return cellGcmGetTiledPitchSize(GetSurfaceWidth(width) * bitsPerPixel / 8);
#else
	// Unused parameters
	width = width;
	bitsPerPixel = bitsPerPixel;
	ASSERT_ONLY(isSwizzled = isSwizzled);
	return 0;
#endif
}

u32 GetSurfacePitch(u16 width, u32 bitsPerPixel, bool isSwizzled)
{
	if (isSwizzled)
	{
		return 0;
	}
	else
	{
		u32 pitch = GetSurfaceWidth(width) * bitsPerPixel / 8;
		Assert(pitch > 0);
		pitch = (pitch + 63) & ~63;
		return pitch;
	}
}

u32 GetSurfaceMipMapCount(u16 width, u16 height, bool isSwizzled)
{
#if 0
	width = width;
	height = height;
	isSwizzled = isSwizzled;
	return 1;
#elif 0
	isSwizzled = isSwizzled;
	return _Log2(_RoundUpPowerOf2(Max(width, height))) + 1;
#elif 1
	isSwizzled = isSwizzled;
	u32 mipCount = 0;
	while (width > 8u || height > 8u)
	{
		width = Max<u16>(width >> 1u, 8u);
		height = Max<u16>(height >> 1u, 8u);
		++mipCount;
	}

	return mipCount + 1;
#else
	const u32 mipCount = _Log2(_RoundUpPowerOf2(Max(width, height))) + 1;
	if (isSwizzled)
	{
		return mipCount;
	}
	else
	{
		const u32 topWidth = width;
		u32 mip = 1;
		for (; mip < mipCount; ++mip)
		{
			width = Max<u16>(width >> 1u, 1u);
			height = Max<u16>(height >> 1u, 1u);

			u32 extraRows = (topWidth - width) * height / width;
			if (extraRows < 8)
			{
				break;
			}
		}
		return mip;
	}
#endif // 0
}

u32 GetSharedSurfaceSizeForPitch(u16 width, u16 height, u32 pitch, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool /*inLocalMemory*/, bool inTiledMemory, bool isSwizzled, bool isCubeMap, bool isZCull, u32* memoryOffsets)
{
	u32 memorySize = 0;

	if (mipCount == 0)
	{
		mipCount = GetSurfaceMipMapCount(width, height, isSwizzled);
		Assert(mipCount > 0);
	}

	if (isSwizzled)
	{
		for (u32 face = 0; face < faceCount; ++face)
		{
			u32 mipWidth = width;
			u32 mipHeight = height;
			for (u32 mip = 0; mip < mipCount; ++mip)
			{
				if (memoryOffsets)
				{
					memoryOffsets[mip * faceCount + face] = memorySize;
				}

				u32 rowSize = mipWidth * bitsPerPixel / 8;
				memorySize += rowSize * mipHeight;
				mipWidth = Max(mipWidth >> 1, 1u);
				mipHeight = Max(mipHeight >> 1, 1u);
			}

			if (isCubeMap)
			{
				memorySize = (memorySize + 127) & ~127;
			}
		}
	}
	else
	{
		for (u32 face = 0; face < faceCount; ++face)
		{
			u32 mipHeight = height;
			for (u32 mip = 0; mip < mipCount; ++mip)
			{
				if (memoryOffsets)
				{
					memoryOffsets[mip * faceCount + face] = memorySize;
				}

				memorySize += mipHeight * pitch;
				mipHeight = Max(mipHeight >> 1, 1u);
			}

			if (isCubeMap)
			{
				memorySize = (memorySize + 127) & ~127;
			}			
		}

		// Make sure we align each surface to the appropriate height
		Assert(memorySize % pitch == 0); // Sanity check
		height = static_cast<u16>(memorySize / pitch);

		if (inTiledMemory)
		{
			// when stacking vertically in a tile region, only 8 line alignment is needed, for both host/local mem
			u32 alignedHeight = ( height + 0x7 ) & ~0x7;
			memorySize = alignedHeight * pitch;
		}
		else
		{
			u32 alignmentMinusOne = GetSurfaceAlignment(inTiledMemory, isZCull) - 1;
			memorySize = (height * pitch + alignmentMinusOne) & ~alignmentMinusOne;
		}
	}

	return memorySize;
}

u32 GetSharedSurfaceSize(u16 width, u16 height, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool inTiledMemory, bool isSwizzled, bool isCubeMap, bool isZCull, u32 surfaceCount, u32* memoryOffsets)
{
	if (surfaceCount == 0)
	{
		return 0;
	}

	u32 pitch = inTiledMemory ? GetSurfaceTiledPitch(width, bitsPerPixel, isSwizzled) : GetSurfacePitch(width, bitsPerPixel, isSwizzled);
	u32 memorySize = GetSharedSurfaceSizeForPitch(width, height, pitch, bitsPerPixel, mipCount, faceCount, inLocalMemory, inTiledMemory, isSwizzled, isCubeMap, isZCull, memoryOffsets) * surfaceCount;

 	const u32 alignment = GetSurfaceAlignment(inTiledMemory, isZCull) - 1;
 	memorySize = (memorySize + alignment) & ~alignment;

	return memorySize;
}

u32 GetSurfaceSize(u16 width, u16 height, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool inTiledMemory, bool isSwizzled, bool isCubeMap, bool isZCull, u32* memoryOffsets)
{
	Assert(!isCubeMap || faceCount == 6);
	Assert(faceCount > 0);

	u32 memorySize = 0;

	if (mipCount == 0)
	{
		mipCount = GetSurfaceMipMapCount(width, height, isSwizzled);
		Assert(mipCount > 0);
	}

	// Mipped render targets must be power of 2 in size
	Assert(mipCount == 1 || (_IsPowerOfTwo(width) && _IsPowerOfTwo(height)));

	if (isSwizzled)
	{
		for (u32 face = 0; face < faceCount; ++face)
		{
			u32 mipWidth = width;
			u32 mipHeight = height;
			for (u32 mip = 0; mip < mipCount; ++mip)
			{
				if (memoryOffsets)
				{
					memoryOffsets[mip * faceCount + face] = memorySize;
				}

				u32 rowSize = mipWidth * bitsPerPixel / 8;
				memorySize += rowSize * mipHeight;
				mipWidth = Max(mipWidth >> 1, 1u);
				mipHeight = Max(mipHeight >> 1, 1u);
			}
			if (isCubeMap)
			{
				memorySize = (memorySize + 127) & ~127;
			}
		}
	}
	else
	{
		const u32 pitch = inTiledMemory ? GetSurfaceTiledPitch(width, bitsPerPixel, false) : GetSurfacePitch(width, bitsPerPixel, false);

		for (u32 face = 0; face < faceCount; ++face)
		{
			u32 mipHeight = height;
			for (u32 mip = 0; mip < mipCount; ++mip)
			{
				if (memoryOffsets)
				{
					memoryOffsets[mip * faceCount + face] = memorySize;
				}

				memorySize += mipHeight * pitch;
				mipHeight = Max(mipHeight >> 1, 1U);
			}
			if (isCubeMap)
			{
				memorySize = (memorySize + 127) & ~127;
			}
		}

		ASSERT_ONLY(u32 temp = memorySize);
		memorySize = GetSurfaceHeight(memorySize / pitch, inLocalMemory) * pitch;
		Assert(memorySize >= temp);
	}

	const u32 alignment = GetSurfaceAlignment(inTiledMemory, isZCull) - 1;
	memorySize = (memorySize + alignment) & ~alignment;

	return memorySize;
}

u32 GetTextureSize(u32 width, u32 height, u32 pitch, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool isSwizzled, bool isCubeMap, u32* memoryOffsets, u32 linesPerPitch)
{
	Assert(!isCubeMap || faceCount == 6);
	Assert(faceCount > 0);

	u32 memorySize = 0;

	if (isSwizzled)
	{
		for (u32 face = 0; face < faceCount; ++face)
		{
			u32 mipWidth = width;
			u32 mipHeight = height;
			for (u32 mip = 0; mip < mipCount; ++mip)
			{
				if (memoryOffsets)
				{
					memoryOffsets[mip * faceCount + face] = memorySize;
				}

				u32 rowSize = mipWidth * bitsPerPixel / 8;
				memorySize += rowSize * mipHeight;
				mipWidth = Max(mipWidth >> 1, 1u);
				mipHeight = Max(mipHeight >> 1, 1u);
			}
			if (isCubeMap)
			{
				memorySize = (memorySize + 127) & ~127;
			}
		}
	}
	else
	{
		for (u32 face = 0; face < faceCount; ++face)
		{
			u32 mipHeight = height;
			for (u32 mip = 0; mip < mipCount; ++mip)
			{
				if (memoryOffsets)
				{
					memoryOffsets[mip * faceCount + face] = memorySize;
				}

				memorySize += mipHeight * pitch / linesPerPitch;
				mipHeight = Max(mipHeight >> 1, 1U);
			}
			if (isCubeMap)
			{
				memorySize = (memorySize + 127) & ~127;
			}
		}

		ASSERT_ONLY(u32 temp = memorySize);
		memorySize = GetSurfaceHeight(memorySize / pitch, inLocalMemory) * pitch;
		Assert(memorySize >= temp);
	}

	return memorySize;
}

u32 GetSurfaceSizeForPitch(u16 width, u16 height, u32 pitch, u32 bitsPerPixel, u32 mipCount, u32 faceCount, bool inLocalMemory, bool inTiledMemory, bool isSwizzled, bool isCubeMap, bool isZCull, u32* memoryOffsets)
{
	Assert(!isCubeMap || faceCount == 6);
	Assert(faceCount > 0);

	u32 memorySize = 0;

	if (mipCount == 0)
	{
		mipCount = GetSurfaceMipMapCount(width, height, isSwizzled);
		Assert(mipCount > 0);
	}

	// Mipped render targets must be power of 2 in size
	Assert(mipCount == 1 || (_IsPowerOfTwo(width) && _IsPowerOfTwo(height)));

	if (isSwizzled)
	{
		for (u32 face = 0; face < faceCount; ++face)
		{
			u32 mipWidth = width;
			u32 mipHeight = height;
			for (u32 mip = 0; mip < mipCount; ++mip)
			{
				if (memoryOffsets)
				{
					memoryOffsets[mip * faceCount + face] = memorySize;
				}

				u32 rowSize = mipWidth * bitsPerPixel / 8;
				memorySize += rowSize * mipHeight;
				mipWidth = Max(mipWidth >> 1, 1u);
				mipHeight = Max(mipHeight >> 1, 1u);
			}
			if (isCubeMap)
			{
				memorySize = (memorySize + 127) & ~127;
			}
		}
	}
	else
	{
		for (u32 face = 0; face < faceCount; ++face)
		{
			u32 mipHeight = height;
			for (u32 mip = 0; mip < mipCount; ++mip)
			{
				if (memoryOffsets)
				{
					memoryOffsets[mip * faceCount + face] = memorySize;
				}

				memorySize += mipHeight * pitch;
				mipHeight = Max(mipHeight >> 1, 1U);
			}
			if (isCubeMap)
			{
				memorySize = (memorySize + 127) & ~127;
			}
		}

		ASSERT_ONLY(u32 temp = memorySize);
		memorySize = GetSurfaceHeight(memorySize / pitch, inLocalMemory) * pitch;
		Assert(memorySize >= temp);
	}

	const u32 alignment = GetSurfaceAlignment(inTiledMemory, isZCull) - 1;
	memorySize = (memorySize + alignment) & ~alignment;

	return memorySize;
}
#if 0
#define NV_CHIP_NUM_MEMPARTS 2

static u32 s_TilePitch		= 0;
static u32 s_baseAddress	= 0;
static u32 s_bankSense		= 0;

static const u32 s_Table4[16] =
{
	0, 1, 2, 3,
	2, 3, 0, 1,
	1, 2, 3, 0,
	3, 0, 1, 2
};
static const u32 s_PitchTable[]  = { 0x200, 0x300, 0x400, 0x500, 0x600, 0x700, 0x800, 0xA00, 0xC00, 0xD00, 0xE00, 0x1000, 0x1400, 0x1800, 0x1A00, 0x1C00, 0x2000, 0x2800, 0x3000, 0x3400, 0x3800, 0x4000, 0x5000, 0x6000, 0x6800, 0x7000, 0x8000, 0xA000, 0xC000, 0xD000, 0xE000, 0x10000 };
static const u32 s_PitchTableCount = sizeof(s_PitchTable) / sizeof(s_PitchTable[0]);
static const u32 s_FactorTable[] = { 0x2, 0x1, 0x4, 0x1, 0x2, 0x1, 0x8, 0x2, 0x4, 0x1, 0x2, 0x10, 0x4, 0x8, 0x2, 0x4, 0x20, 0x8, 0x10, 0x4, 0x8, 0x40, 0x10, 0x20, 0x8, 0x10, 0x80, 0x20, 0x40, 0x10, 0x20, 0x100 };
static const u32 s_PrimeTable[]  = { 0x1, 0x3, 0x1, 0x5, 0x3, 0x7, 0x1, 0x5, 0x3, 0xd, 0x7, 0x1, 0x5, 0x3, 0xd, 0x7, 0x1, 0x5, 0x3, 0xd, 0x7, 0x1, 0x5, 0x3, 0xd, 0x7, 0x1, 0x5, 0x3, 0xd, 0x7, 0x1 };

struct MainMemoryAddressTranslationTilingParams
{
	u32 prime;
	u32 factor;
	u32 baseAddress;
	u32 tileWidth;
	u32 bankSense;
	u32 colBits;
	u32 rowBits;
	u32 bankBits;
	u32 linesInTile;
	u32 numPartitions;
	u32 tileLineBits;
	u32 tileWidthInBytes;
	u32 tileAddressMultiple;

};

static MainMemoryAddressTranslationTilingParams g_MmattParams;

/* helper function for getting a bit slice */
static u32 FbTileBits(u32 d, int j, int i)
{
	if (j >= i) return (((d) >> (i)) & ((0xffffffff)>>(31-((j)-(i)))));
	else return 0;
}

/* helper function for concatenating bit slices */

static u32 CatBits(u32 f3, int e3, int s3,
					 u32 f2, int e2, int s2,
					 u32 f1, int e1, int s1,
					 u32 f0, int e0, int s0)
{
	u32 out = 0;
	/*                  */ out  = FbTileBits(f3, e3, s3);
	out <<= (e2 - s2 + 1); out |= FbTileBits(f2, e2, s2);
	out <<= (e1 - s1 + 1); out |= FbTileBits(f1, e1, s1);
	out <<= (e0 - s0 + 1); out |= FbTileBits(f0, e0, s0);
	return out;
}

static u32 CatBits(
					 u32 f5, int e5, int s5,
					 u32 f4, int e4, int s4,
					 u32 f3, int e3, int s3,
					 u32 f2, int e2, int s2,
					 u32 f1, int e1, int s1,
					 u32 f0, int e0, int s0)
{
	u32 out = 0;
	out <<= (e5 - s5 + 1); out |= FbTileBits(f5, e5, s5);
	out <<= (e4 - s4 + 1); out |= FbTileBits(f4, e4, s4);
	out <<= (e3 - s3 + 1); out |= FbTileBits(f3, e3, s3);
	out <<= (e2 - s2 + 1); out |= FbTileBits(f2, e2, s2);
	out <<= (e1 - s1 + 1); out |= FbTileBits(f1, e1, s1);
	out <<= (e0 - s0 + 1); out |= FbTileBits(f0, e0, s0);
	return out;
}

void SetTileConfiguration(MainMemoryAddressTranslationTilingParams* params)
{
	// copy the tiling params for address space into the params struct.
	params->tileWidth = 256;   // all tiles are 256B wide
	params->colBits = 10;
	params->rowBits = 16;
	params->bankBits = 2;
	params->numPartitions = 2;
	params->linesInTile = 64;
	params->tileLineBits = 6;
	params->tileWidthInBytes = 256;   // An fbtile is 256B wide on screen.
	params->tileAddressMultiple = 0x10000;

	// linesInTile depends on partitions and column bits.
	u32 tileSizeInBytes = 8 * params->numPartitions * (1 << params->colBits);
	params->linesInTile = tileSizeInBytes / params->tileWidth;
	params->tileLineBits = 6;
	params->baseAddress   = s_baseAddress >> 16;
	params->bankSense = s_bankSense;

	int tableNumber = -1;
	for (u32 i = 0 ; i < s_PitchTableCount; ++i)
	{
		if (s_TilePitch == s_PitchTable[i])
		{
			tableNumber = i;
			break;
		}
	}

	if (tableNumber >= 0)
	{
		params->prime  = s_PrimeTable[tableNumber];
		params->factor = s_FactorTable[tableNumber];
	}
	else
	{
		assert(0);
	}
}

u32 LinearToTileAddress(u32 addressIn)
{
	u32 addressOut			= addressIn;
	u32 bankFlip			= 0xdeadbeef;
	u32 columnAddress		= 0xdeadbeef;
	u32 rowAddress			= 0xdeadbeef;
	u32 bankAddress			= 0xdeadbeef;
	u32 partitionAddress	= 0xdeadbeef;

	u32 pitchInBytes = g_MmattParams.prime * g_MmattParams.factor * g_MmattParams.tileWidthInBytes;
	u32 tilesPerRow = pitchInBytes / g_MmattParams.tileWidthInBytes;

	u32 baseAddress = g_MmattParams.baseAddress * g_MmattParams.tileAddressMultiple; // base address in bytes

	u32 L = static_cast<u32>((addressIn - baseAddress) / g_MmattParams.tileWidthInBytes);

	u32 segX = L % tilesPerRow;
	u32 segY = static_cast<u32>(L / tilesPerRow);   // essentially the y coordinate, since the segments are 1 pixel tall

	u32 tileX = segX;
	u32 tileY = static_cast<u32>(segY / g_MmattParams.linesInTile);

	u32 tileNum = tileY * tilesPerRow + tileX;

	u32 tileAddress = FbTileBits(tileNum, 21, 0) + FbTileBits(baseAddress, 31, 14);

	u32 inTileOffset = segY % 64;

	rowAddress = (tileAddress >> g_MmattParams.bankBits ) & ((1 << g_MmattParams.rowBits)-1);

	if (g_MmattParams.factor == 1)
	{
		bankAddress = FbTileBits(tileAddress,1,0);
	}
	else if (g_MmattParams.factor == 2)
	{
		u32 idx = FbTileBits(tileAddress + (tileY % 2 ? 2 : 0), 1, 0) * 4 + FbTileBits(tileY, 1, 0);
		bankAddress = s_Table4[idx];
	}
	else if (g_MmattParams.factor >= 4)
	{
		u32 idx = FbTileBits(tileAddress, 1, 0) * 4 + FbTileBits(tileY, 1, 0);
		bankAddress = s_Table4[idx];
	}
	else
	{
		Assert(0);
	}

	// remember the bank flip
	bankAddress = (bankAddress + g_MmattParams.bankSense) % 4;
	bankFlip = bankAddress ^ FbTileBits(tileAddress, 1, 0);

	columnAddress = CatBits(inTileOffset,	g_MmattParams.tileLineBits - 1,	3,
							addressIn,		7,								5,
							inTileOffset,	1,								0,
							0,				1,								0);

	partitionAddress = (FbTileBits(inTileOffset, 2, 2) + FbTileBits(addressIn, 6, 6)) % 2;
	partitionAddress += ((g_MmattParams.numPartitions < NV_CHIP_NUM_MEMPARTS)) ? (NV_CHIP_NUM_MEMPARTS / 2) : 0;

	addressOut = CatBits(
		rowAddress, 15, 0,
		bankAddress, 1, 0,
		columnAddress, 9, 4,
		partitionAddress, 0, 0,
		columnAddress, 3, 2,
		addressIn, 4, 0);

	addressOut ^= FbTileBits(addressOut, 11, 11) << 10;
	addressOut ^= (FbTileBits(addressOut, 12, 12) ^ FbTileBits(bankFlip, 0, 0) ^ FbTileBits(addressOut, 14, 14)) << 9;

	return addressOut;
}

void ConvertLinearToTileFormat(u32 baseAddress, u32 tilePitch, u32 bankSense, u32 texSize)
{
	// must be set by API
	s_baseAddress = baseAddress;
	s_TilePitch   = tilePitch;
	s_bankSense   = bankSense;

	u32 addressOut = 0;

	SetTileConfiguration(&g_MmattParams);

	u32 textureWidth  = texSize;
	u32 textureHeight = texSize;
	u32 bytePerPixel  = 4;

	for (u32 y = 0 ; y < textureHeight ; ++y)
	{
		for (u32 x = 0 ; x < textureWidth ; ++x)
		{
			// generate linear address for the pixel
			u32 address = baseAddress + (y * textureWidth + x) * bytePerPixel;

			// linear to tile translation
			addressOut = LinearToTileAddress(address);

			// write test pixel data to the translated address
			*reinterpret_cast<u32*>(addressOut) = ((y & 0xff) << 16) + ((x & 0xff) << 8);
		}
	}
}
#endif // 0

#if !__NO_OUTPUT
#define RR(reg,name,fmt)			{ reg, 0, 1, name, fmt },
#define RRR(reg,siz,ct,name,fmt)	{ reg, siz, ct, name, fmt },

struct RsxRegInit { u16 Reg; u8 Siz, Ct; const char *Name, *Fmt; };

static RsxRegInit s_RegInit[] = {
#include "rsx_registers.h"
};

static const char **s_Regs;

inline int hexval(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else
		return 0;
}

inline char as_char(unsigned u) {
	u &= 255;
	if (u >= 32 && u < 127)
		return (char)u;
	else
		return '.';
}

static int indent_level;
const char *indent() {
	////////12345678901234567890123456789012 -- 32 spaces
	return "                                " + 32 - indent_level*2;
}

void HexDump(const char *fn,int line,const char *desc,u32 *list,u32 *end)
{
	if (!s_Regs) {
		const int maxReg = 0x2000/4;
		s_Regs = rage_new const char*[maxReg];
		memset(s_Regs,0,maxReg * sizeof(const char*));
		for (int i=0; i<NELEM(s_RegInit); i++) {
			RsxRegInit &r = s_RegInit[i];
			for (int j=0; j<r.Ct; j++) {
				int slot = (r.Reg + j * r.Siz) >> 2;
				TrapGE(slot,maxReg);
				Assert(!s_Regs[slot]);
				char buf[512];
				strcpy(buf,r.Fmt);
				strcat(buf,"|");
				formatf(buf+strlen(buf),(int)(sizeof(buf)-strlen(buf)),r.Name,j);
				s_Regs[slot] = StringDuplicate(buf);
			}
		}
	}

	grcDisplayf("%s(%d): %s: (%u bytes)",fn,line,desc,(size_t)end-(size_t)list);
	u32 *call = 0;
	while (list < end) {
		u32 cmd = *list++;
		// grcDisplayf("cmd = %08x",cmd);
		if (cmd & 0x1) {	// Our custom secret SPU macros
			static const char *spuCommands[] = {
				// grcorespu.h
				"grcDevice__BeginFrame",
				"grcDevice__EndFrame",
				"grcDevice__Finish",

				"grcVertexProgram__SetFlag",

				"grcEffect__SetGlobalFloatCommon",
				"grcEffect__SetGlobalVar_grcTexture",
				"grcEffect__SetEdgeViewportCullEnable",

				"grcViewport__RegenerateDevice",
				"grcViewport__SetWindow",
				"grcDevice__SetScissor",
				"grcState__SetLightingMode",

				"grcDevice__WriteStateStruct",

				"grcDevice__Jump",
				"grcEffect__RenderStateBlockMask",

				"grcEffect__Bind",
				"grcEffect__IncrementalBind",
				"grcEffect__UnBind",

				"grcEffect__SetVarFloatCommon",
				"grcEffect__SetVarFloatCommonByRef",
				"grcEffect__SetVar_grcTexture",
				"grcEffect__SetVar_Color32",
				// "grcDevice__SetVertexShaderConstantByRef",

				"grcEffect__ClearCachedState",
				// "grcEffect__SetCullMode",
				"grcDevice__ChangeTextureRemap",
				// "grcDevice__RunCommandBuffer",
				// "grcDevice__ReturnFromCommandBuffer",
				// "grcDevice__NextCommandBufferSegment",
				"grcDevice__DrawPrimitive",
				"grcDevice__DrawIndexedPrimitive",
				"grcDevice__DrawInstancedPrimitive",
				"grcDevice__SetSurface",
				"grcGeometryJobs__SetOccluders",
				"grcGeometryJobs__SetEdgeNoPixelTestEnable",
#if __BANK
				"grcGeometryJobs__SetEdgeCullDebugFlags",
#endif // __BANK
				"grcDevice__SetPixelShaderConstantF",
				"grcDevice__SetEdgeClipPlane",
				"grcDevice__SetEdgeClipPlaneEnable",
#if !__FINAL
				"grmModel__SetCullerDebugFlags",
				"grcEffect__SetFragStripDebugFlags",
#endif // !__FINAL
				"grmModel__SetCullerAABB",

#if !__FINAL
				"grcDevice__PushFaultContext",
				"grcDevice__PopFaultContext",
				"grcStateBlock__SetWireframeOverride",
#endif
				"grcDevice__CreateGraphicsJob",
#if HACK_GTA4
				"grcEffect__CopyByte",
#endif
#if DRAWABLESPU_STATS
				"grcEffect__FetchStats",
#endif
				"grcStateBlock__SetDepthStencilState",
				"grcStateBlock__SetRasterizerState",
				"grcStateBlock__SetBlendState",
#if LAZY_STATEBLOCKS
				"grcStateBlock__Flush",
#endif
				"grcEffect__SetSamplerState",
				"grcEffect__PushSamplerState",
				"grcEffect__PopSamplerState",
				"grcDevice__grcBegin",
				"grcDevice__BindVertexFormat",

				// grcorespu_gamestate.h
#if HACK_GTA4
				"GTA4__SetShadowType",
				"GTA4__SetShadowMatrix",
				"GTA4__SetDamageTexture",
				"GTA4__SetDamageTextureOffset",
				"GTA4__SetCharacterClothData",
				"GTA4__SetTintDescriptor",
				"GTA4__SetWriteRsxLabel",
#elif HACK_MC4
				"MC4__SetDamageTexture",
				"MC4__SetDamageParams",
				"MC4__SetRestMatrix",
				"MC4__SetTransformType",
				"MC4__SetTransformParam",
				"MC4__SetBoneIndex",
				"MC4__SendBones",
#endif
				// grmodelspu.h
				"grmShaderFx__SetForcedTechniqueGroupId",
				"grmModel__SetForceShader",

				"grmGeometryEdge__Draw",
				"grmGeometryEdge__DrawSkinned",
				"grmGeometryEdge__SetBlendHeaders",

				"grmShaderGroup__SetVarCommon",
				"grmShaderGroup__SetVarCommonByRef",

				// drawablespu.h
				"rmcDrawable__Draw",
				"rmcDrawable__DrawSkinned",
				"startStatRecord",
				"stopStatRecord",
			};
#if __PPU
			CompileTimeAssert(NELEM(spuCommands) == RMCORE_COMMAND_COUNT);
#endif
			u32 size = (cmd & 0x3FFC0000) >> 16;
			u32 subcmd = (cmd >> 8) & 0xFF;
			cmd = (cmd & 0xFE) >> 1;
			if (cmd >= NELEM(spuCommands))
				Quitf("Invalid spu macro command detected: %d",cmd);
#if !__FINAL && __PPU
			else if (cmd == grcDevice__PushFaultContext)
				grcDisplayf("%sSPU MACRO command %s (subcommand %x) [%*.*s], size %u",indent(),spuCommands[cmd],subcmd,subcmd,subcmd,(char*)list,size);
#endif
			else
				grcDisplayf("%sSPU MACRO command %s (subcommand %x) [%08x], size %u",indent(),spuCommands[cmd],subcmd,size>4?list[0]:0,size);
			list += (size>>2)-1;
		}
		else if (cmd & CELL_GCM_METHOD_FLAG_JUMP) {
			grcDisplayf("%sJUMP to %x  ???????",indent(),cmd & 0x0FFFFFFC);
			/// list = (u32*) (g_IoAddress + (cmd & 0x0FFFFFFC));
		}
		else if (cmd & CELL_GCM_METHOD_FLAG_CALL) {
			grcDisplayf("%sCALL to %x",indent(),cmd & 0x0FFFFFFC);
			// call = list;
			// list = (u32*) (g_IoAddress + (cmd & 0x0FFFFFFC));
			// AssertMsg(!call,"CALL stack overflow!");
		}
		else if (cmd == CELL_GCM_METHOD_FLAG_RETURN) {
			grcDisplayf("%sRETURN",indent());
			if (call)
				list = call;
			else
				return;
			call = NULL;
		}
		else if ((cmd & 0x3) == 0) {
			u32 size = (cmd & 0x3FFC0000) >> 16;
			u32 reg = cmd & 0x00001FFC;
			//// grcDisplayf("cmd %08x  reg=%x, size=%x",cmd,reg,size);
			if (reg == 0x1818 && true) {
				grcDisplayf("%s...(%d bytes of immediate mode data)...",indent(),size);
				list += (size/4);
			}
			else if (reg == 0x0100) {
				if (list[0] == 0x68750002) {
					grcDisplayf("%scellGcmSetPerfMonPushMarker(%s)",indent(),(char*)(list+1));
					++indent_level;

				}
				else if (list[0] == 0x68750003) {
					--indent_level;
					grcDisplayf("%scellGcmSetPerfMonPopMarker",indent());
				}
				list += (size/4);
			}
			else while (size) {
				const char *desc = s_Regs[reg>>2];
				char pretty[256] = "", *pp = pretty;
				if (desc) {
					union { u32 u; float f; } x;
					x.u = *list;
					int offset = 0;
					do {
						char spec = *desc++;
						int width = 32;
						if (spec == 'h')
							width = 16;
						if (*desc == ':') {
							++desc;
							width = *desc++ - '0';
							if (*desc != ':')
								width = 10*width + (*desc++)-'0';
							if (*desc == ':')
								++desc;
						}
						u32 value = (x.u >> offset);
						char fieldname[64] = "";
						if (*desc != ';' && *desc !='|' && *desc != '0') {
							char *fn = fieldname;
							while (*desc != ';' && *desc !='|' && *desc!=':')
								*fn++ = *desc++;
							*fn++ = '=';
							*fn = 0;
							if (*desc==':')
								++desc;
						}
						if (width != 32)
							value &= (1<<width)-1;
						offset += width;
						if (spec == '*')
							/*pad, ignored */;
						else if (spec == 'c')
							sprintf(pp,"%08x",value);
						else if (spec == 'x')
							sprintf(pp,"%s%x ",fieldname,value);
						else if (spec == 'X')
							sprintf(pp,"%s%x%c%c%c%c ",fieldname,value,as_char(value>>24),as_char(value>>16),as_char(value>>8),as_char(value));
						else if (spec == 'h')
							sprintf(pp,"%s%u ",fieldname,value);
						else if (spec == 'f')
							sprintf(pp,"%s%g ",fieldname,x.f);
						else if (spec == 'b')
							sprintf(pp,"%s%s ",fieldname,value?"true":"false");
						else if (spec == 'e') {
							u32 enumerant = ~0U;
							bool found = false;
							while (*desc!=';' && *desc!='|') {
								char label[64]="", *lp = label;
								if (*desc == '0' && desc[1]=='x') {
									desc += 2;
									enumerant = 0;
									while (*desc != '=')
										enumerant = (enumerant<<4) | hexval(*desc++);
									desc++;
								}
								else
									++enumerant;
								while (*desc!=',' && *desc!=';' && *desc!='|')
									*lp++ = *desc++;
								if (*desc==',')
									++desc;
								*lp = 0;
								if (enumerant == value) {
									sprintf(pp,"%s%s ",fieldname,label);
									found = true;
								}
							}
							if (!found)
								sprintf(pp,"%s<INVALID> ",fieldname);
						}
						else
							AssertMsg(false,"invalid spec char");
						pp += strlen(pp);
					} while (*desc++ != '|');
				}
				grcDisplayf("%sCMD %04x:%x [ %s %s]",indent(),reg,*list++,desc,pretty);
				if (!(cmd & 0x40000000))
					reg += 4;
				size -= 4;
			}
		}
		else
			grcDisplayf("%sINVALID %x",indent(),cmd);
	}
}
#endif

#if __PS3

void SetTransferData(CellGcmContextData *ctx,
	u8 mode, u32 dstOffset, s32 dstPitch, u32 srcOffset,
	s32 srcPitch, s32 bytesPerRow, s32 rowCount)
{
	// Argument check
	Assert(bytesPerRow >= 0);
	Assert(rowCount >= 0);

	u32 requiredWords
		= 3             // CELL_GCM_METHOD_COPY2D_SET_CONTEXT_DMA_BUFFER
		+ 2;            // CELL_GCM_METHOD_COPY2D_OFFSET_OUT

	const int32_t  CL0039_MIN_PITCH = -32768;
	const int32_t  CL0039_MAX_PITCH = 32767;
	const int32_t  CL0039_MAX_ROWS  = 0x7ff;
	const uint32_t CL0039_MAX_LINES = 0x3fffff;

	// Can we turn this into a contigous blit ?
	if ((srcPitch == bytesPerRow) && (dstPitch == bytesPerRow))
	{
		bytesPerRow *= rowCount;
		rowCount = 1;
		srcPitch = 0;
		dstPitch = 0;
	}

	// Unusual pitch values
	if ((srcPitch < CL0039_MIN_PITCH) || (srcPitch > CL0039_MAX_PITCH) ||
		(dstPitch < CL0039_MIN_PITCH) || (dstPitch > CL0039_MAX_PITCH))
	{
		// Blit one line at a time
		requiredWords += rowCount * ((bytesPerRow + CL0039_MAX_LINES - 1) / CL0039_MAX_LINES) * 9;
	}
	else
	{
		// Blit batches of lines together
		requiredWords += ((rowCount + CL0039_MAX_ROWS - 1) / CL0039_MAX_ROWS) * ((bytesPerRow + CL0039_MAX_LINES - 1) / CL0039_MAX_LINES) * 9;
	}

	// Single reserve for entire block of RSX commands
	if (Unlikely(ctx->current + requiredWords > ctx->end))
	{
		ctx->callback(ctx, requiredWords);
	}
	ASSERT_ONLY(u32 *const begin = ctx->current;)

	u32 srcHandle, dstHandle;
	switch (mode)
	{
	case CELL_GCM_TRANSFER_MAIN_TO_LOCAL:
		srcHandle = CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
		dstHandle = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		break;
	case CELL_GCM_TRANSFER_LOCAL_TO_MAIN:
		srcHandle = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		dstHandle = CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
		break;
	case CELL_GCM_TRANSFER_LOCAL_TO_LOCAL:
		srcHandle = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		dstHandle = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		break;
	case CELL_GCM_TRANSFER_MAIN_TO_MAIN:
		srcHandle = CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
		dstHandle = CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER;
		break;
	default:
		Quitf("Invalid transfer mode");
	}

	CELL_GCM_METHOD_COPY2D_SET_CONTEXT_DMA_BUFFER(ctx->current,
		srcHandle,
		dstHandle);

	u32 colCount;
	u32 rows;
	u32 cols;

	// Unusual pitch values
	if ((srcPitch < CL0039_MIN_PITCH) || (srcPitch > CL0039_MAX_PITCH) ||
		(dstPitch < CL0039_MIN_PITCH) || (dstPitch > CL0039_MAX_PITCH))
	{
		// Fallback: blit per line (could improve this case)
		// Blit one line at a time
		while(--rowCount >= 0)
		{
			for(colCount = bytesPerRow; colCount>0; colCount -= cols)
			{
				// Clamp to limit
				cols = (colCount > CL0039_MAX_LINES) ? CL0039_MAX_LINES : colCount;

				// Do the blit
				CELL_GCM_METHOD_COPY2D_OFFSET_PITCH_LINE_FORMAT_NOTIFY(ctx->current,
					srcOffset + (bytesPerRow - colCount),
					dstOffset + (bytesPerRow - colCount),
					0,
					0,
					cols,
					1,
					1, 1,
					0);
			}

			dstOffset += dstPitch;
			srcOffset += srcPitch;
		}
	}
	else
	{
		// For each batch of rows
		for(;rowCount>0; rowCount -= rows)
		{
			// clamp to limit ?
			rows = (rowCount > CL0039_MAX_ROWS) ? CL0039_MAX_ROWS : rowCount;

			// for each batch of cols
			for(colCount = bytesPerRow; colCount>0; colCount -= cols)
			{
				// clamp to limit
				cols = (colCount > CL0039_MAX_LINES) ? CL0039_MAX_LINES : colCount;

				// do the blit
				CELL_GCM_METHOD_COPY2D_OFFSET_PITCH_LINE_FORMAT_NOTIFY(ctx->current,
					srcOffset + (bytesPerRow - colCount),
					dstOffset + (bytesPerRow - colCount),
					srcPitch,
					dstPitch,
					cols,
					rows,
					1, 1,
					0);
			}

			// Advance to next set of rows
			srcOffset += rows * srcPitch;
			dstOffset += rows * dstPitch;
		}
	}

	CELL_GCM_METHOD_COPY2D_OFFSET_OUT(ctx->current, 0);

	Assert(ctx->current == begin + requiredWords);
}

#if !__FINAL
void GcmTextureMemSet(const CellGcmTexture *target, int sampleID, u32 value)
{
	GRCDEVICE.CpuWaitOnGpuIdle();

	u16	width=target->width;
	u16	height=target->height;

	// this does not lock a buffer ... just reading it: you are responsible for all side effects
	u32 * RESTRICT s = reinterpret_cast<u32*>(target->location == CELL_GCM_LOCATION_LOCAL ? gcm::LocalPtr(target->offset) : gcm::MainPtr(target->offset));
	if( 0 == sampleID )
	{
		for (int row = 0; row < height; row++, s = (u32*)((char*)s + target->pitch)) 
		{
			for (int col=0; col<width; col++)
			{
				if( !( col & 1 ) )
				{
					s[col] = value;
				}
			}
		}
	}
	else if( 1 == sampleID )
	{
		for (int row = 0; row < height; row++, s = (u32*)((char*)s + target->pitch)) 
		{
			for (int col=0; col<width; col++)
			{
				if ( col & 1 )
				{
					s[col] = value;
				}
			}
		}
	}
	else
	{
		for (int row = 0; row < height; row++, s = (u32*)((char*)s + target->pitch)) 
		{
			for (int col=0; col<width; col++)
			{
				s[col] = value;
			}
		}
	}
}
#endif // !__FINAL

#endif // __PS3

}	// namespace gcm
}	// namespace rage

#pragma warning(pop)

#endif
