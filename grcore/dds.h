//
// grcore/dds.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_DDS_H
#define GRCORE_DDS_H

#include "system/magicnumber.h"

namespace rage {

	// DOM-IGNORE-BEGIN

	// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/DDSFileReference/ddsfileformat.asp

	enum {
		DDPF_ALPHAPIXELS = 0x00000001,
		DDPF_ALPHA       = 0x00000002,
		DDPF_FOURCC      = 0x00000004,
		DDPF_RGB         = 0x00000040,
		DDPF_LUMINANCE   = 0x00020000,
	};

	struct DDPIXELFORMAT9 {
		u32 dwSize;        // Size of structure. This member must be set to 32.
		u32 dwFlags;       // Flags to indicate valid fields. Uncompressed formats will usually use DDPF_RGB to indicate an RGB format, while compressed formats will use DDPF_FOURCC with a four-character code.
		u32 dwFourCC;      // This is the four-character code for compressed formats. dwFlags should include DDPF_FOURCC in this case. For DXTn compression, this is set to "DXT1", "DXT2", "DXT3", "DXT4", or "DXT5".
		u32 dwRGBBitCount; // For RGB formats, this is the total number of bits in the format. dwFlags should include DDPF_RGB in this case. This value is usually 16, 24, or 32. For A8R8G8B8, this value would be 32.
		u32 dwRBitMask;
		u32 dwGBitMask;
		u32 dwBBitMask;    // For RGB formats, these three fields contain the masks for the red, green, and blue channels. For A8R8G8B8, these values would be 0x00ff0000, 0x0000ff00, and 0x000000ff respectively.
		u32 dwABitMask;    // For RGB formats, this contains the mask for the alpha channel, if any. dwFlags should include DDPF_ALPHAPIXELS in this case. For A8R8G8B8, this value would be 0xff000000.
	};

	enum {
		DDSCAPS_COMPLEX            = 0x00000008,
		DDSCAPS_TEXTURE            = 0x00001000,
		DDSCAPS_MIPMAP             = 0x00400000,
	};

	enum {
		DDSCAPS2_CUBEMAP           = 0x00000200,
		DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400,
		DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800,
		DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000,
		DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000,
		DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000,
		DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000,
		DDSCAPS2_VOLUME            = 0x00200000,
	};

	struct DDCAPS2 {
		u32 dwCaps1;
		u32 dwCaps2;
		u32 Reserved[2];
	};

	enum {
		DDSD_CAPS        = 0x00000001,
		DDSD_HEIGHT      = 0x00000002,
		DDSD_WIDTH       = 0x00000004,
		DDSD_PITCH       = 0x00000008,
		DDSD_PIXELFORMAT = 0x00001000,
		DDSD_MIPMAPCOUNT = 0x00020000,
		DDSD_LINEARSIZE  = 0x00080000,
		DDSD_DEPTH       = 0x00800000,
	};

	struct DDSURFACEDESC2 {
		u32 dwSize;                    // Size of structure. This member must be set to 124.
		u32 dwFlags;                   // Flags to indicate valid fields. Always include DDSD_CAPS, DDSD_PIXELFORMAT, DDSD_WIDTH, DDSD_HEIGHT.
		u32 dwHeight;                  // Height of the main image in pixels
		u32 dwWidth;                   // Width of the main image in pixels
		u32 dwPitchOrLinearSize;       // For uncompressed formats, this is the number of bytes per scan line (DWORD> aligned) for the main image. dwFlags should include DDSD_PITCH in this case. For compressed formats, this is the total number of bytes for the main image. dwFlags should be include DDSD_LINEARSIZE in this case.
		u32 dwDepth;                   // For volume textures, this is the depth of the volume. dwFlags should include DDSD_DEPTH in this case.
		u32 dwMipMapCount;             // For items with mipmap levels, this is the total number of levels in the mipmap chain of the main image. dwFlags should include DDSD_MIPMAPCOUNT in this case.
		u32 dwReserved1[4];            // Unused
		float fGamma;                  // gamma value that this texture was saved at.
		float fColorExp[3];            // Color scale (pre-calculated exponent) (R,G,B) for HDR values
		float fColorOfs[3];            // Color offset (R,G,B) for HDR values
		DDPIXELFORMAT9 ddpfPixelFormat; // 32-byte value that specifies the pixel format structure.
		DDCAPS2 ddsCaps;               // 16-byte value that specifies the capabilities structure.
		u32 dwReserved2;               // Unused
	};

	enum DDS_D3DFORMAT { // copied from d3d9types.h 'enum _D3DFORMAT', but prefixed "DDS_" to avoid name conflicts
		DDS_D3DFMT_UNKNOWN              = 0,
		DDS_D3DFMT_R8G8B8               = 20,
		DDS_D3DFMT_A8R8G8B8             = 21,
		DDS_D3DFMT_X8R8G8B8             = 22,
		DDS_D3DFMT_R5G6B5               = 23,
		DDS_D3DFMT_X1R5G5B5             = 24,
		DDS_D3DFMT_A1R5G5B5             = 25,
		DDS_D3DFMT_A4R4G4B4             = 26,
		DDS_D3DFMT_R3G3B2               = 27,
		DDS_D3DFMT_A8                   = 28,
		DDS_D3DFMT_A8R3G3B2             = 29,
		DDS_D3DFMT_X4R4G4B4             = 30,
		DDS_D3DFMT_A2B10G10R10          = 31,
		DDS_D3DFMT_A8B8G8R8             = 32,
		DDS_D3DFMT_X8B8G8R8             = 33,
		DDS_D3DFMT_G16R16               = 34,
		DDS_D3DFMT_A2R10G10B10          = 35,
		DDS_D3DFMT_A16B16G16R16         = 36,

		DDS_D3DFMT_A8P8                 = 40,
		DDS_D3DFMT_P8                   = 41,

		DDS_D3DFMT_L8                   = 50,
		DDS_D3DFMT_A8L8                 = 51,
		DDS_D3DFMT_A4L4                 = 52,

		DDS_D3DFMT_V8U8                 = 60,
		DDS_D3DFMT_L6V5U5               = 61,
		DDS_D3DFMT_X8L8V8U8             = 62,
		DDS_D3DFMT_Q8W8V8U8             = 63,
		DDS_D3DFMT_V16U16               = 64,
		DDS_D3DFMT_A2W10V10U10          = 67,

		DDS_D3DFMT_UYVY                 = MAKE_MAGIC_NUMBER('U', 'Y', 'V', 'Y'),
		DDS_D3DFMT_R8G8_B8G8            = MAKE_MAGIC_NUMBER('R', 'G', 'B', 'G'),
		DDS_D3DFMT_YUY2                 = MAKE_MAGIC_NUMBER('Y', 'U', 'Y', '2'),
		DDS_D3DFMT_G8R8_G8B8            = MAKE_MAGIC_NUMBER('G', 'R', 'G', 'B'),
		DDS_D3DFMT_DXT1                 = MAKE_MAGIC_NUMBER('D', 'X', 'T', '1'),
		DDS_D3DFMT_DXT2                 = MAKE_MAGIC_NUMBER('D', 'X', 'T', '2'),
		DDS_D3DFMT_DXT3                 = MAKE_MAGIC_NUMBER('D', 'X', 'T', '3'),
		DDS_D3DFMT_DXT4                 = MAKE_MAGIC_NUMBER('D', 'X', 'T', '4'),
		DDS_D3DFMT_DXT5                 = MAKE_MAGIC_NUMBER('D', 'X', 'T', '5'),

		DDS_D3DFMT_CTX1                 = MAKE_MAGIC_NUMBER('C', 'T', 'X', '1'), // xenon only
		DDS_D3DFMT_DXT3A                = MAKE_MAGIC_NUMBER('A', 'X', 'T', '3'), // xenon only
		DDS_D3DFMT_DXT3A_1111           = MAKE_MAGIC_NUMBER('A', '1', 'T', '3'), // xenon only
		DDS_D3DFMT_DXT5A                = MAKE_MAGIC_NUMBER('A', 'T', 'I', '1'), // xenon/d3d11/orbis
		DDS_D3DFMT_DXN                  = MAKE_MAGIC_NUMBER('A', 'T', 'I', '2'), // xenon/d3d11/orbis
		DDS_D3DFMT_BC6                  = MAKE_MAGIC_NUMBER('B', 'C', '6', ' '), // d3d11/orbis
		DDS_D3DFMT_BC7                  = MAKE_MAGIC_NUMBER('B', 'C', '7', ' '), // d3d11/orbis

		DDS_D3DFMT_R8                   = MAKE_MAGIC_NUMBER('R', '8', ' ', ' '), // custom (works on all platforms)
		DDS_D3DFMT_R16                  = MAKE_MAGIC_NUMBER('R', '1', '6', ' '), // custom (works on all platforms)
		DDS_D3DFMT_G8R8                 = MAKE_MAGIC_NUMBER('G', '8', 'R', '8'), // custom (works on all platforms)

		DDS_D3DFMT_D16_LOCKABLE         = 70,
		DDS_D3DFMT_D32                  = 71,
		DDS_D3DFMT_D15S1                = 73,
		DDS_D3DFMT_D24S8                = 75,
		DDS_D3DFMT_D24X8                = 77,
		DDS_D3DFMT_D24X4S4              = 79,
		DDS_D3DFMT_D16                  = 80,

		DDS_D3DFMT_D32F_LOCKABLE        = 82,
		DDS_D3DFMT_D24FS8               = 83,

		DDS_D3DFMT_D32_LOCKABLE         = 84,
		DDS_D3DFMT_S8_LOCKABLE          = 85,

		DDS_D3DFMT_L16                  = 81,

		DDS_D3DFMT_VERTEXDATA           = 100,
		DDS_D3DFMT_INDEX16              = 101,
		DDS_D3DFMT_INDEX32              = 102,

		DDS_D3DFMT_Q16W16V16U16         = 110,

		DDS_D3DFMT_MULTI2_ARGB8         = MAKE_MAGIC_NUMBER('M','E','T','1'),

		DDS_D3DFMT_R16F                 = 111,
		DDS_D3DFMT_G16R16F              = 112,
		DDS_D3DFMT_A16B16G16R16F        = 113,

		DDS_D3DFMT_R32F                 = 114,
		DDS_D3DFMT_G32R32F              = 115,
		DDS_D3DFMT_A32B32G32R32F        = 116,

		DDS_D3DFMT_CxV8U8               = 117,

		DDS_D3DFMT_A1                   = 118,

		DDS_D3DFMT_A2B10G10R10_XR_BIAS  = 119,

		DDS_D3DFMT_BINARYBUFFER         = 199,

		DDS_D3DFMT_FORCE_DWORD          = 0x7fffffff
	};

	// DOM-IGNORE-END

}	// namespace rage

#endif
