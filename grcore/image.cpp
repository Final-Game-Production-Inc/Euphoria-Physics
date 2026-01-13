//
// grcore/image.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "file/asset.h"
#include "grcore/channel.h"
#include "grcore/config.h"
#include "grcore/dds.h"
#include "grcore/image.h"
#include "grcore/image_dxt.h"
#include "grcore/texture.h"
#include "math/amath.h"
#include "math/float16.h"
#include "vector/vector4.h"
#include "system/param.h"
#include "system/magicnumber.h"
#include "system/memory.h"
#include "system/param.h"

PARAM(removetopmips, "[RAGE] don't load top level mips for any given image");
PARAM(minmiplevels, "[RAGE] the minimum number of mips to load for any given image");
PARAM(maxmiplevels, "[RAGE] the maximum number of mips to load for any given image");
PARAM(minmipsize, "[RAGE] the minimum dimension of mips to load for any given image");
PARAM(maxmipsize, "[RAGE] the maximum dimension of mips to load for any given image");

using namespace rage;

// TODO -- i added functionality to support DXT block decompression in image_dxt.h, which works ..
// it seems like the code below does not work (especially for DXT5 alpha, but even DXT1 colour
// requires additional byte-swapping to get the right data ..) at some point it would be good
// to remove DXT1_BLOCK, DXT3_BLOCK, DXT5_BLOCK below and use the utility classes in image_dxt.h
// instead.

// Look here for DXT format specs:  http://oss.sgi.com/projects/ogl-sample/registry/EXT/texture_compression_s3tc.txt
// Binary format for a DXT1 4x4 texel block.  Note that the rows
// are byte swapped (index XOR 1) on Xenon.
struct DXT1_BLOCK {
	// Anchor colors; if c0<=c1 then use colorkey and one mid-color; else use two mid-colors
	u16 c0, c1; 
	// Row data (two bits per texel)
	// If c0<=c1, 00b->c0, 01b->c1, 10b->Lerp(0.5,c0,c1), 11b->transparent black
	// If c0>c1, 00b->c0, 01b->c1, 10b->Lerp(0.33,c0,c1), 11b->Lerp(0.66,c0,c1);
	u8 rows[4];
};

// Binary format for a DXT3 4x4 texel block.  Note that the rows
// are byte swapped (index XOR 1) on Xenon.
struct DXT3_BLOCK {
	// 64 bits of alpha (4 bits per texel), no interpolation
	u8 alphaBits[8];
	
	// Anchor colors as per DXT1 (except assume c0 > c1 regardless of values)
	u16 c0, c1;
	// Colors as per DXT1
	u8 rows[4];

	u8 GetAlpha(int x,int y) const {
		u8 alphaByte = alphaBits[(y<<1)|(x>>1)];
		if (x&1)
			return u8((alphaByte & 0xF0) | (alphaByte >> 4));
		else
			return u8((alphaByte & 0x0F) | (alphaByte << 4));
	}
};

// Binary format for a DXT5 4x4 texel block.  Note that the rows
// are byte swapped (index XOR 1) on Xenon.
struct DXT5_BLOCK {
	// Anchor alpha values.  If a0 < a1 then 0 and 1 are reserved values.
	u8 a0, a1;
	// 48 bits (3 bits per texel) of alpha interpolation data.
	//  +0  +1  +2  +3
	// 210 543 876 BA9		// row 0
	// EDC 10F 432 765		// row 1
	// A98 DCB 0FE 321		// row 2
	// 654 987 CBA FED		// row 3
	u8 adata[6];
	// Anchor colors as per DXT1 (except assume c0 > c1 regardless of values)
	u16 c0, c1;
	// Colors as per DXT1
	u8 rows[4];

	u8 GetAlpha(int x,int y) const {
		int offset = (x + y * 4) * 3;
		int result = adata[offset >> 3] >> (offset & 7);
		result |= adata[(offset >> 3) + 1] << (8-(offset & 7));
		return (u8) (result & 7);
	}

	void SetAlpha(int x,int y,u8 src) {
		int offset = (x + y * 4) * 3;
		u8 masklow = u8(7 << (offset & 7));
		u8 maskhi = u8(7 >> (8-(offset & 7)));
		adata[offset >> 3] &= ~masklow;
		adata[offset >> 3] |= (src << (offset & 7));
		adata[(offset >> 3)+1] &= ~maskhi;
		adata[(offset >> 3)+1] |= (src << (8-(offset & 7)));
	}
};

#if __PS3 && HACK_GTA4
bool grcImage::sm_gammaCorrect = false;
#endif

grcImage::grcImage()
	: m_Width    (0)
	, m_Height   (0)
	, m_Format   (UNKNOWN)
	, m_Type     (STANDARD)
	, m_Stride   (0)
	, m_Depth    (0)
	, m_StrideHi (0)
	, m_Bits     (NULL)
	, m_Lut      (NULL)
	, m_Next     (NULL)
	, m_NextLayer(NULL)
	, m_RefCount (1)
	, m_ColorExp (1,1,1)
	, m_ColorOfs (0,0,0)
{
#if __RESOURCECOMPILER
	sysMemSet(&GetConstructorInfo(), 0, sizeof(grcImageConstructorInfo));
#endif
}

grcImage::~grcImage() {
	if ( m_NextLayer ) {
		m_NextLayer->Release();
	}
	if (m_Bits) {
		delete [] m_Bits;
	}
	if (!m_Next && !m_NextLayer && m_Lut) { // Lowest layered miplevel owns the lut
		delete[] m_Lut;
	}
	if ( m_Next ) {
		m_Next->Release();
	}
}

static __forceinline void Set565(Color32& dest, u16 src) {
	int red = src & 31;
	int green = (src >> 5) & 63;
	int blue = (src >> 11);
	dest.Set((red<<3)|(red>>2),(green<<2)|(green>>4),(blue<<3)|(blue>>2));
}

grcImage* grcImage::Reformat(grcImage::Format NewFormat) {
	if (m_Format == NewFormat) {
		AddRef();
		return this;
	}
	Format format = GetFormat();
	Format newFormat = (Format)(NewFormat & (u32)FORMAT_MASK);
	if (newFormat != A8R8G8B8 || IsFormatDXTBlockCompressed(format))
		return NULL;
	grcImage *newImage = grcImage::Create(m_Width,m_Height,m_Depth,NewFormat,STANDARD,0,0);
	if (format == DXT1) {
		DXT1_BLOCK *b = (DXT1_BLOCK*) m_Bits;
		for (int y=0; y<m_Height; y+=4) {
			for (int x=0; x<m_Width; x+=4, b++) {
				Color32 lut[4];
				Color32 *dest = x + y*m_Width + (Color32*)newImage->m_Bits;
				Set565(lut[0],b->c0);
				Set565(lut[1],b->c1);
				if (b->c0<b->c1) {
					lut[2].Set(
						(lut[0].GetRed()+lut[1].GetRed())>>1,
						(lut[0].GetGreen()+lut[1].GetGreen())>>1,
						(lut[0].GetBlue()+lut[1].GetBlue())>>1,
						255);
					lut[3].Set(0,0,0,0);
				}
				else {
					lut[2].Set(
						(lut[0].GetRed()*171+lut[1].GetRed()*85)>>8,
						(lut[0].GetGreen()*171+lut[1].GetGreen()*85)>>8,
						(lut[0].GetBlue()*171+lut[1].GetBlue()*85)>>8,
						255);
					lut[3].Set(
						(lut[1].GetRed()*171+lut[0].GetRed()*85)>>8,
						(lut[1].GetGreen()*171+lut[0].GetGreen()*85)>>8,
						(lut[1].GetBlue()*171+lut[0].GetBlue()*85)>>8,
						255);
				}
				for (int row=0; row<4; row++,dest+=m_Width) {
					dest[0] = lut[(b->rows[row]>>0)&3];
					dest[1] = lut[(b->rows[row]>>2)&3];
					dest[2] = lut[(b->rows[row]>>4)&3];
					dest[3] = lut[(b->rows[row]>>6)&3];
				}
			}
		}
	}
	else if (format == DXT3) {
		DXT3_BLOCK *b = (DXT3_BLOCK*) m_Bits;
		for (int y=0; y<m_Height; y+=4) {
			for (int x=0; x<m_Width; x+=4, b++) {
				Color32 lut[4];
				Color32 *dest = x + y*m_Width + (Color32*)newImage->m_Bits;
				Set565(lut[0],b->c0);
				Set565(lut[1],b->c1);
				lut[2].Set(
					(lut[0].GetRed()*171+lut[1].GetRed()*85)>>8,
					(lut[0].GetGreen()*171+lut[1].GetGreen()*85)>>8,
					(lut[0].GetBlue()*171+lut[1].GetBlue()*85)>>8,
					0);
				lut[3].Set(
					(lut[1].GetRed()*171+lut[0].GetRed()*85)>>8,
					(lut[1].GetGreen()*171+lut[0].GetGreen()*85)>>8,
					(lut[1].GetBlue()*171+lut[0].GetBlue()*85)>>8,
					0);
				for (int row=0; row<4; row++,dest+=m_Width) {
					dest[0] = lut[(b->rows[row]>>0)&3].MergeAlpha(b->GetAlpha(0,row));
					dest[1] = lut[(b->rows[row]>>2)&3].MergeAlpha(b->GetAlpha(1,row));
					dest[2] = lut[(b->rows[row]>>4)&3].MergeAlpha(b->GetAlpha(2,row));
					dest[3] = lut[(b->rows[row]>>6)&3].MergeAlpha(b->GetAlpha(3,row));
				}
			}
		}
	}
	else if (format == DXT5) {
		DXT5_BLOCK *b = (DXT5_BLOCK*) m_Bits;
		for (int y=0; y<m_Height; y+=4) {
			for (int x=0; x<m_Width; x+=4, b++) {
				Color32 lut[4];
				Color32 *dest = x + y*m_Width + (Color32*)newImage->m_Bits;
				Set565(lut[0],b->c0);
				Set565(lut[1],b->c1);
				lut[2].Set(
					(lut[0].GetRed()*171+lut[1].GetRed()*85)>>8,
					(lut[0].GetGreen()*171+lut[1].GetGreen()*85)>>8,
					(lut[0].GetBlue()*171+lut[1].GetBlue()*85)>>8,
					0);
				lut[3].Set(
					(lut[1].GetRed()*171+lut[0].GetRed()*85)>>8,
					(lut[1].GetGreen()*171+lut[0].GetGreen()*85)>>8,
					(lut[1].GetBlue()*171+lut[0].GetBlue()*85)>>8,
					0);
				for (int row=0; row<4; row++,dest+=m_Width) {
					dest[0] = lut[(b->rows[row]>>0)&3].MergeAlpha(b->GetAlpha(0,row));
					dest[1] = lut[(b->rows[row]>>2)&3].MergeAlpha(b->GetAlpha(1,row));
					dest[2] = lut[(b->rows[row]>>4)&3].MergeAlpha(b->GetAlpha(2,row));
					dest[3] = lut[(b->rows[row]>>6)&3].MergeAlpha(b->GetAlpha(3,row));
				}
			}
		}
	}
	return newImage;
}

int grcImage::GetExtraMipCount() const {
	const grcImage *i = this;
	int result = 0;
	while (i->GetNext()) {
		++result;
		i = i->GetNext();
	}
	return result;
}

#define SUPPORT_MIPMAPS_SMALLER_THAN_DXT_BLOCK_SIZE (1)
#define SUPPORT_NON_POW2_MIPS                       (1)
#define SUPPORT_NON_POW2_MIPSCALE                   (1 && SUPPORT_NON_POW2_MIPS)

static u32 GetMaxMipCount(u32 w, u32 h, u32 d) {
#if !SUPPORT_NON_POW2_MIPS
	if ((w&(w - 1)) | (h&(h - 1)) | (d&(d - 1)))
		return 1;
#endif 
	for (int i = 0; i < 16; i++) {
		const u32 mw = Max<u32>(1, w>>i);
		const u32 mh = Max<u32>(1, h>>i);
		const u32 md = Max<u32>(1, d>>i);
		if (mw == 1 && mh == 1 && md == 1)
			return i + 1; // this mip index is valid, but the next one won't be
#if !SUPPORT_NON_POW2_MIPSCALE
		else if (w%mw != 0 || h%mh != 0 || d%md != 0)
			return i; // this mip has non-pow2 scale, so the previous one was the last valid one (note - we should can support non-pow-2 mipmaps fully, so this test won't be necessary)
#endif
	}
	Assert(0); // should never get here
	return 1;
}

grcImage* grcImage::Create(u32 width,u32 height,u32 depth,Format format, ImageType type, u32 extraMipmaps, u32 extraLayers) {
	if (!Verifyf(width >= 1 && height >= 1 && depth >= 1, "Image cannot be zero-sized (w=%d,h=%d,d=%d)", width, height, depth))
		return NULL;
	const u32 maxMipCount = GetMaxMipCount(width, height, depth);
	if (!Verifyf(maxMipCount >= extraMipmaps + 1, "Image has too many mipmaps (w=%d,h=%d,d=%d,mips=%d), max for this size is %d", width, height, depth, extraMipmaps + 1, maxMipCount))
		return NULL;
	if (!Verifyf(type != CUBE || (width == height && depth == 1 && (extraLayers + 1)%6 == 0), "Cubemap image must be square with depth=1 and 5+6N extra layers (w=%d,h=%d,d=%d,layers=%d)", width, height, depth, extraLayers + 1))
		return NULL;
	if (!Verifyf(type != VOLUME || extraLayers == 0, "Volume image cannot have extra layers (w=%d,h=%d,d=%d,layers=%d)", width, height, depth, extraLayers + 1))
		return NULL;
	if (!Verifyf(type == VOLUME || depth == 1, "Non-volume image must have depth=1 (w=%d,h=%d,d=%d)", width, height, depth))
		return NULL;
#if !SUPPORT_MIPMAPS_SMALLER_THAN_DXT_BLOCK_SIZE
	if (IsFormatDXTBlockCompressed((Format)(format & (u32)FORMAT_MASK)) && ((width|height)&3) != 0) {
		Warningf("Compressed image must have width and height divisible by four (this one is %dx%d)", width, height);
		return NULL;
	}
#endif
	return CreateInternal(width, height, depth, format, type, extraMipmaps, extraLayers);
}

grcImage* grcImage::CreateInternal(u32 width,u32 height,u32 depth,Format format, ImageType type, u32 extraMipmaps, u32 extraLayers) {
	if (!width || !height || !depth)
		return NULL;
#if !SUPPORT_MIPMAPS_SMALLER_THAN_DXT_BLOCK_SIZE
	if (IsFormatDXTBlockCompressed((Format)(format & (u32)FORMAT_MASK)) && ((width|height)&3) != 0)
		return NULL; // silently remove lower mips smaller than 4x4 .. eventually we might support these
#endif
	grcImage *result = rage_new grcImage;
	result->m_Format = format;
	result->m_Type = type;
	Assign(result->m_Width,width);
	Assign(result->m_Height,height);
	Assign(result->m_Depth,depth);
	result->RecalculateStride();

	unsigned size = result->GetPhysicalHeight() * result->GetStride() * depth;
	result->m_Bits = rage_new u8[size]; // (u8*) sysMemAllocator::GetCurrent().TryAllocate(size, 16);
	result->m_Next = extraMipmaps ? grcImage::CreateInternal(Max(width>>1u,1u),Max(height>>1u,1u),Max(depth>>1u,1u),format,type,extraMipmaps-1,0) : NULL;
	result->m_NextLayer = extraLayers ? grcImage::CreateInternal(width,height,depth,format,type,extraMipmaps,extraLayers-1) : NULL;
	return result;
}

void grcImage::Resize(u32 width,u32 height) {
#define USE_NEW_RESIZE (0)
#if USE_NEW_RESIZE
	Assert(m_Type == DEPTH || m_Type == STANDARD);
	bool isValidMip = width > 0 && height > 0;
	if (isValidMip)
#else
	if (!width || !height )
		return ;
#endif // USE_NEW_RESIZE
	{
		Assign(m_Width,width);
		Assign(m_Height,height);
		RecalculateStride();
	}

#if USE_NEW_RESIZE
	if (m_Next)
		m_Next->Resize(width >> 1u, height >> 1u);
	if (m_NextLayer)
		m_NextLayer->Resize(width, height);

	if (!isValidMip)
		Release();
#endif // USE_NEW_RESIZE
}

void grcImage::Clear(u32 color) {
	Format format = GetFormat();
	if (format == A8R8G8B8 || format == LINA8R8G8B8_DEPRECATED) {
		u32 *pix = (u32*) m_Bits;
		u32 count = GetVolume();
		do {
			*pix++ = color;
		} while (--count);
	}
	else if (format == A8)
		memset(m_Bits, Color32(color).GetAlpha(), GetSize());
	else if (format == DXT1 && color == 0) {
		DXT1_BLOCK src = { 0, 0xFFFF, { 0xFF, 0xFF, 0xFF, 0xFF } };
		DXT1_BLOCK *dest = (DXT1_BLOCK*) m_Bits;
		u32 count = GetVolume() >> 4;
		do {
			*dest++ = src;
		} while (--count);
	}
	else if (format == A4R4G4B4) {
		Color32 color32(color);
		u16 a = color32.GetAlpha() >> 4;
		u16 r = color32.GetRed()   >> 4;
		u16 g = color32.GetGreen() >> 4;
		u16 b = color32.GetBlue()  >> 4;
		u16 color16 = (a << 12) | (r << 8) | (g << 4) | b;
		u16 *pix = (u16*) m_Bits;
		u32 count = GetVolume();
		do {
			*pix++ = color16;
		} while (--count);
	}
	else
		memset(m_Bits, 0, GetSize()); // this should clear to 0,0,0,0 (including DXT3, etc.)

	if (m_Next)
		m_Next->Clear(color);

	if (m_NextLayer)
		m_NextLayer->Clear(color);
}


#if __PPU
#undef __BE
#define __BE 0
#endif

void* grcImage::GetPixelAddr(int x, int y, int z) const {
	const Format format = GetFormat();
	const int bitsPerPixel = GetFormatBitsPerPixel(format);

	int addr = 0;

	if (IsFormatDXTBlockCompressed(format)) {
		const int bx = x/4;
		const int by = y/4;
		const int bytesPerBlock = bitsPerPixel*2;
		const int blocksPerRow  = (m_Width + 3)/4;
		const int rowsPerSlice  = (m_Height + 3)/4;
		addr = (bx + (by + z*rowsPerSlice)*blocksPerRow)*bytesPerBlock; // could use 'm_Stride' here ..
	}
	else {
		addr = x*bitsPerPixel/8 + (y + z*m_Height)*m_Stride;
	}

	return (void*)(const_cast<u8*>(m_Bits) + addr);
}

void grcImage::GetPixelBlock(Vector4 block[4*4], int x, int y, int z) const
{
	FastAssert(x >= 0 && x < m_Width  && (x&3) == 0);
	FastAssert(y >= 0 && y < m_Height && (y&3) == 0);
	FastAssert(z >= 0 && z < m_Depth);

	const Format f = GetFormat();

	if (IsFormatDXTBlockCompressed(f))
	{
		Color32 temp[4*4];

		switch (f)
		{
		case DXT1      : reinterpret_cast<DXT::DXT1_BLOCK*>(GetPixelAddr(x, y, z))->Decompress(temp); break;
		case DXT3      : reinterpret_cast<DXT::DXT3_BLOCK*>(GetPixelAddr(x, y, z))->Decompress(temp); break;
		case DXT5      : reinterpret_cast<DXT::DXT5_BLOCK*>(GetPixelAddr(x, y, z))->Decompress(temp); break;
		case CTX1      : reinterpret_cast<DXT::CTX1_BLOCK*>(GetPixelAddr(x, y, z))->Decompress(temp); break;
		case DXT3A     : reinterpret_cast<DXT::DXT3_ALPHA*>(GetPixelAddr(x, y, z))->Decompress(temp); break;
		case DXT3A_1111: reinterpret_cast<DXT::DXT3_ALPHA*>(GetPixelAddr(x, y, z))->Decompress(temp); break; // TODO -- support this
		case DXT5A     : reinterpret_cast<DXT::DXT5_ALPHA*>(GetPixelAddr(x, y, z))->Decompress(temp); break;
		case DXN       : reinterpret_cast<DXT::DXN_BLOCK *>(GetPixelAddr(x, y, z))->Decompress(temp); break;
		default        : Assertf(false, "unhandled compressed format!");
		}

		for (int yy = 0; yy < 4; yy++)
		{
			for (int xx = 0; xx < 4; xx++)
			{
				block[xx + yy*4] = VEC4V_TO_VECTOR4(temp[xx + yy*4].GetRGBA());
			}
		}
	}
	else
	{
		for (int yy = 0; yy < 4; yy++)
		{
			for (int xx = 0; xx < 4; xx++)
			{
				block[xx + yy*4] = GetPixelVector4(Min<int>(x + xx, m_Width - 1), Min<int>(y + yy, m_Depth - 1), z);
			}
		}
	}
}

void grcImage::GetPixelBlock(Color32 block[4*4], int x, int y, int z) const
{
	FastAssert(x >= 0 && x < m_Width  && (x&3) == 0);
	FastAssert(y >= 0 && y < m_Height && (y&3) == 0);
	FastAssert(z >= 0 && z < m_Depth);

	const Format f = GetFormat();

	if (IsFormatDXTBlockCompressed(f))
	{
		switch (f)
		{
		case DXT1      : reinterpret_cast<DXT::DXT1_BLOCK*>(GetPixelAddr(x, y, z))->Decompress(block); break;
		case DXT3      : reinterpret_cast<DXT::DXT3_BLOCK*>(GetPixelAddr(x, y, z))->Decompress(block); break;
		case DXT5      : reinterpret_cast<DXT::DXT5_BLOCK*>(GetPixelAddr(x, y, z))->Decompress(block); break;
		case CTX1      : reinterpret_cast<DXT::CTX1_BLOCK*>(GetPixelAddr(x, y, z))->Decompress(block); break;
		case DXT3A     : reinterpret_cast<DXT::DXT3_ALPHA*>(GetPixelAddr(x, y, z))->Decompress(block); break;
		case DXT3A_1111: reinterpret_cast<DXT::DXT3_ALPHA*>(GetPixelAddr(x, y, z))->Decompress(block); break; // TODO -- support this
		case DXT5A     : reinterpret_cast<DXT::DXT5_ALPHA*>(GetPixelAddr(x, y, z))->Decompress(block); break;
		case DXN       : reinterpret_cast<DXT::DXN_BLOCK *>(GetPixelAddr(x, y, z))->Decompress(block); break;
		default        : Assertf(false, "unhandled compressed format!");
		}
	}
	else
	{
		for (int yy = 0; yy < 4; yy++)
		{
			for (int xx = 0; xx < 4; xx++)
			{
				block[xx + yy*4] = GetPixelColor32(Min<int>(x + xx, m_Width - 1), Min<int>(y + yy, m_Depth - 1), z);
			}
		}
	}
}

Vector4 grcImage::GetPixelVector4(int x, int y, int z) const
{
	FastAssert(x >= 0 && x < m_Width );
	FastAssert(y >= 0 && y < m_Height);
	FastAssert(z >= 0 && z < m_Depth );

	const Format f = GetFormat();
	Vector4 result(0.0f, 0.0f, 0.0f, 0.0f);

	if (IsFormatDXTBlockCompressed(f))
	{
		Vector4 block[4*4];
		GetPixelBlock(block, x&~3, y&~3, z);
		result = block[(x&3) + (y&3)*4];
	}
	else // TODO -- handle other formats (16-bit float etc.)
	{
		const u8* addr = (const u8*)GetPixelAddr(x, y, z);

		if (f == A32B32G32R32F)
		{
			result = *(const Vector4*)addr;
		}
		else // convert from Color32
		{
			result = VEC4V_TO_VECTOR4(GetPixelColor32(x, y, z).GetRGBA());
		}
	}

	return result;
}

Color32 grcImage::GetPixelColor32(int x, int y, int z) const
{
	FastAssert(x >= 0 && x < m_Width );
	FastAssert(y >= 0 && y < m_Height);
	FastAssert(z >= 0 && z < m_Depth );

	const Format f = GetFormat();
	Color32 result(0);

	if (IsFormatDXTBlockCompressed(f))
	{
		Color32 block[4*4];
		GetPixelBlock(block, x&~3, y&~3, z);
		result = block[(x&3) + (y&3)*4];
	}
	else // TODO -- handle other formats (16-bit float etc.)
	{
		const u8* addr = (const u8*)GetPixelAddr(x, y, z);

		if (f == A32B32G32R32F)
		{
			result = Color32(*(const Vector4*)addr);
		}
		else switch ((int)f)
		{
		case A8R8G8B8 : {                                                       result = *(const Color32*)addr; break; }
		case A8B8G8R8 : { const DXT::ARGB8888 tmp(*(const DXT::ABGR8888*)addr); result = *(const Color32*)&tmp; break; }
		case A4R4G4B4 : { const DXT::ARGB8888 tmp(*(const DXT::ARGB4444*)addr); result = *(const Color32*)&tmp; break; }
		case A1R5G5B5 : { const DXT::ARGB8888 tmp(*(const DXT::ARGB1555*)addr); result = *(const Color32*)&tmp; break; }
		case R5G6B5   : { const DXT::ARGB8888 tmp(*(const DXT::RGB565  *)addr); result = *(const Color32*)&tmp; break; }
#if __XENON
		case A8       : // A8,L8 are both really "R8", A8L8 is really "G8R8"
		case L8       : { result = Color32(addr[0],0,0,0); break; }
		case A8L8     : { result = Color32(addr[0],addr[1],0,0); break; }
#elif __PPU
		case A8       : // A8,L8 are both really "B8", A8L8 is really "G8B8"
		case L8       : { result = Color32(0,0,addr[0],0); break; }
		case A8L8     : { result = Color32(0,addr[0],addr[1],0); break; }
#else
		case A8       : { result = Color32(0,0,0,addr[0]); break; }
		case L8       : { result = Color32(addr[0],addr[0],addr[0],255); break; }
		case A8L8     : { result = Color32(addr[1],addr[1],addr[1],addr[0]); break; } // haven't tested this yet
#endif
		}
	}

	return result;
}

void grcImage::ByteSwapData(void* data, int dataSize, int swapSize)
{
	u8* data0 = (u8*)data;
	u8* data1 = data0 + dataSize;

	if (swapSize == 2)
	{
		for (; data0 < data1; data0 += 2)
		{
			const u8 swap[2] = {data0[0], data0[1]};

			data0[0] = swap[1];
			data0[1] = swap[0];
		}
	}
	else if (swapSize == 4)
	{
		for (; data0 < data1; data0 += 4)
		{
			const u8 swap[4] = {data0[0], data0[1], data0[2], data0[3]};

			data0[0] = swap[3];
			data0[1] = swap[2];
			data0[2] = swap[1];
			data0[3] = swap[0];
		}
	}
}

void grcImage::LoadAsTextureAlias(const grcTexture* texture, const grcTextureLock* lock, int mipCount) // this creates an alias to the texture data, so do not delete or release the image
{
	Assert(m_Width  == 0);
	Assert(m_Height == 0);
	Assert(m_Bits   == NULL);

	u8* data = (u8*)lock->Base;

	m_Width     = (u16)lock->Width;
	m_Height    = (u16)lock->Height;
	m_Format    = (Format)texture->GetImageFormat();
	m_Type      = STANDARD;
	m_Stride    = 0;
	m_Depth     = 1;
	m_Bits      = data;
	m_Lut       = NULL;
	m_Next      = NULL;
	m_NextLayer = NULL;
	m_RefCount  = 0;

	RecalculateStride();

	grcImage* prev = this;
	
	for (int i = 1; i < mipCount; i++)
	{
		data += prev->GetSize();

		grcImage* mip = rage_new grcImage();

		mip->m_Width     = (u16)Max<int>(1, lock->Width>>i);
		mip->m_Height    = (u16)Max<int>(1, lock->Height>>i);
		mip->m_Format    = (Format)texture->GetImageFormat();
		mip->m_Type      = STANDARD;
		mip->m_Stride    = 0;
		mip->m_Depth     = 1;
		mip->m_Bits      = data;
		mip->m_Lut       = NULL;
		mip->m_Next      = NULL;
		mip->m_NextLayer = NULL;
		mip->m_RefCount  = 0;

		mip->RecalculateStride();

		prev->m_Next = mip;
		prev = mip;
	}
}

void grcImage::ReleaseAlias()
{
	Assert(m_RefCount == 0);
	if (m_NextLayer)
		m_NextLayer->ReleaseAlias();
	if (m_Next)
		m_Next->ReleaseAlias();
	sysMemSet(this, 0, sizeof(*this)); // we probably could just set m_Bits to NULL ..
	m_RefCount = 1; // .. so that Release doesn't assert (the dtor calls Release)
	delete this;
}

Color32 grcImage::Tex2D(float u, float v) const
{
	int x = (int)((float)m_Width * u);
	int y = (int)((float)m_Height * v);
	return Color32(GetPixel(x, y));
}

u32 grcImage::GetPixel(int x,int y) const {
	Format format = GetFormat();
	switch (format) {
		case A8R8G8B8:
		case LINA8R8G8B8_DEPRECATED:
		case RGBE:
		case R32F:
		case G16R16:
			return *(((u32*)(m_Bits + y*GetStride())) + x);
		case A8:
			return (m_Bits[x + y*GetStride()] << 24);
		case L8:
		//	return (m_Bits[x + y*GetStride()] * 0x010101) | 0xFF000000;
			return (m_Bits[x + y*GetStride()]) & 0xFF;

		case DXT1:
			{
				DXT1_BLOCK *b = (DXT1_BLOCK*)m_Bits + (x / 4) + ((y / 4) * (m_Width / 4));
				int texel = (b->rows[(y&3)^__BE] >> ((x&3)<<1)) & 3;
				Color32 lut[4];
				Set565(lut[0],b->c0);
				Set565(lut[1],b->c1);
				if (b->c0<b->c1) {
					lut[2].Set(
						(lut[0].GetRed()+lut[1].GetRed())>>1,
						(lut[0].GetGreen()+lut[1].GetGreen())>>1,
						(lut[0].GetBlue()+lut[1].GetBlue())>>1,
						255);
					lut[3].Set(0,0,0,0);
				}
				else {
					lut[2].Set(
						(lut[0].GetRed()*171+lut[1].GetRed()*85)>>8,
						(lut[0].GetGreen()*171+lut[1].GetGreen()*85)>>8,
						(lut[0].GetBlue()*171+lut[1].GetBlue()*85)>>8,
						255);
					lut[3].Set(
						(lut[1].GetRed()*171+lut[0].GetRed()*85)>>8,
						(lut[1].GetGreen()*171+lut[0].GetGreen()*85)>>8,
						(lut[1].GetBlue()*171+lut[0].GetBlue()*85)>>8,
						255);
				}

				return lut[texel].GetColor();
			}

		case DXT3:
			{
				DXT3_BLOCK *b = (DXT3_BLOCK*)m_Bits + (x / 4) + ((y / 4) * (m_Width / 4));
				int texel = (b->rows[(y&3)^__BE] >> ((x&3)<<1)) & 3;
				Color32 lut[4];
				Set565(lut[0],b->c0);
				Set565(lut[1],b->c1);
				lut[2].Set(
					(lut[0].GetRed()*171+lut[1].GetRed()*85)>>8,
					(lut[0].GetGreen()*171+lut[1].GetGreen()*85)>>8,
					(lut[0].GetBlue()*171+lut[1].GetBlue()*85)>>8,
					0);
				lut[3].Set(
					(lut[1].GetRed()*171+lut[0].GetRed()*85)>>8,
					(lut[1].GetGreen()*171+lut[0].GetGreen()*85)>>8,
					(lut[1].GetBlue()*171+lut[0].GetBlue()*85)>>8,
					0);

				return lut[texel].MergeAlpha(b->GetAlpha(x&1,(y&3)^__BE)).GetColor();
			}

		case DXT5:
			{
				DXT5_BLOCK *b = (DXT5_BLOCK*)m_Bits + (x / 4) + ((y / 4) * (m_Width / 4));
				int texel = (b->rows[(y&3)^__BE] >> ((x&3)<<1)) & 3;
				Color32 lut[4];
				Set565(lut[0],b->c0);
				Set565(lut[1],b->c1);
				lut[2].Set(
					(lut[0].GetRed()*171+lut[1].GetRed()*85)>>8,
					(lut[0].GetGreen()*171+lut[1].GetGreen()*85)>>8,
					(lut[0].GetBlue()*171+lut[1].GetBlue()*85)>>8,
					0);
				lut[3].Set(
					(lut[1].GetRed()*171+lut[0].GetRed()*85)>>8,
					(lut[1].GetGreen()*171+lut[0].GetGreen()*85)>>8,
					(lut[1].GetBlue()*171+lut[0].GetBlue()*85)>>8,
					0);

				return lut[texel].MergeAlpha(b->GetAlpha(x&1,(y&3)^__BE)).GetColor();
			}

		case A4R4G4B4:
			{
				u16 texel = (u16&)m_Bits[x + y*GetStride()];
				u16 a = (texel >> 12) & 0xf;
				u16 r = (texel >> 8) & 0xf;
				u16 g = (texel >> 4) & 0xf;
				u16 b = texel & 0xf;
				return (a << 24) | (a << 28) | (r << 16) | (r << 20) | (g << 8) | (g << 12) | b | (b << 4);
			}

		default:
			return 0;
	}
}

void grcImage::SetPixelVector(int x,int y, float color[4] )
{
	Format format = GetFormat();
	switch (format) 
	{
		case A32B32G32R32F:
		case LINA32B32G32R32F_DEPRECATED:
			memcpy(  (u8*)( m_Bits + (x *16 + y*GetStride())), (u8*) color , 4 * 4);
		break;

		case A16B16G16R16:
		{
			Assert( color[0] >= 0.0f && color[0] <= 1.0f );
			Assert( color[1] >= 0.0f && color[1] <= 1.0f );
			Assert( color[2] >= 0.0f && color[2] <= 1.0f );
			Assert( color[3] >= 0.0f && color[3] <= 1.0f );

			u16 res[4];
			res[0] = (u16)( color[0] * (float)( (1<<16)-1)  );
			res[1] = (u16)( color[1] * (float)( (1<<16)-1)  );
			res[2] = (u16)( color[2] * (float)( (1<<16)-1)  );
			res[3] = (u16)( color[3] * (float)( (1<<16)-1)  );
			memcpy(  (u8*)( m_Bits + (x *8 + y*GetStride())), (u8*) res , 2 * 4);
		}
		break;


		default:
			AssertMsg( 0, "format not supported for vector set pixel" );
			return;
	}
}

void grcImage::SetPixel(int x,int y,u32 color) {
	Format format = GetFormat();
	switch (format) {
		case G16R16:
		case G16R16F:
		case A8R8G8B8:
		case A8B8G8R8:	// WRONG
		case LINA8R8G8B8_DEPRECATED:
		case RGBE:
		case R32F:
			*(((u32*)(m_Bits + y*GetStride())) + x) = color;
			break;
		case A8:
			m_Bits[x + y*GetStride()] = u8(color >> 24);
			break;
		case L8:
			m_Bits[x + y*GetStride()] = u8(color & 0xFF);
			break;
		case A8L8:
			*(((u16*)(m_Bits + y*GetStride())) + x) = u16(color & 0xFF) << 8 | u16(color >> 24);
			break;

		case DXT1:
			{
				DXT1_BLOCK *b = (DXT1_BLOCK*)m_Bits + (x / 4) + ((y / 4) * (m_Width / 4));
				b->c0 = 0; b->c1 = 0xFFFF;
				b->rows[(y&3)^__BE] &= ~(3 << ((x&3)<<1));
				if (color == 0xFF000000)	// opaque black
					b->rows[(y&3)^__BE] |= 0 << ((x&3)<<1);
				else if (color == 0xFFFFFFFF) // opaque white
					b->rows[(y&3)^__BE] |= 1 << ((x&3)<<1);
				else if (color == 0)		// transparent black
					b->rows[(y&3)^__BE] |= 3 << ((x&3)<<1);
				else	// 50% grey
					b->rows[(y&3)^__BE] |= 2 << ((x&3)<<1);
			}
			break;
		case DXT3:
			{
				DXT3_BLOCK *b = (DXT3_BLOCK*)m_Bits + (x / 4) + ((y / 4) * (m_Width / 4));
				x &= 3;
				y &= 3;
				b->c0 = 0; b->c1 = 0xFFFF;
				b->rows[y^__BE] &= ~(3 << (x<<1));
				u32 alpha = color >> 28;
				color &= 0xFFFFFF;
				if (color == 0)	// black
					b->rows[y^__BE] |= 0 << (x<<1);
				else // (color == 0xFFFFFF) // white
					b->rows[y^__BE] |= 1 << (x<<1);
				b->alphaBits[((x >> 1) + (y << 1))^__BE] |= alpha << ((x&1)?4:0);
			}
			break;
		case A4R4G4B4:
			{
				Color32 color32(color);
				u8 a = color32.GetAlpha() >> 4;
				u8 r = color32.GetRed() >> 4;
				u8 g = color32.GetGreen() >> 4;
				u8 b = color32.GetBlue() >> 4;
				(u16&)m_Bits[x + y*GetStride()] = (a << 12) | (r << 8) | (g << 4) | b;
			}
			break;
		default:
			Assertf(0, "grcImage::SetPixel no support for format %d", format);
			return;
	}
}
#if __PPU
#undef __BE
#define __BE 1
#endif


u32 grcImage::sm_MinMipSize = 1;
u32 grcImage::sm_MaxMipSize = 0xFFFF;
u32 grcImage::sm_MaxMipLevels = 0xFFFF; //3 or 4 are usually reasonable numbers...
u32 grcImage::sm_MinMipLevels = 0; 
u32 grcImage::sm_MaxMipBytes = 0;
float grcImage::sm_SizeRatio = 1.0f;
bool grcImage::sm_useSRGBTextures = true;

#if __RESOURCECOMPILER

grcImage::CustomLoadFuncType	grcImage::sm_customLoadFunc = NULL;
bool							grcImage::sm_customLoad     = false;
const char*						grcImage::sm_platformString = NULL;

void grcImage::SetCustomLoadFunc( CustomLoadFuncType func )
{
	sm_customLoadFunc = func;
}

bool grcImage::RunCustomLoadFunc( const char* path, grcImage::ImageList& outputs, void** outParams )
{
	if (sm_customLoadFunc)
	{
		return ( sm_customLoadFunc(path, outputs, outParams) );
	}

	return ( false );
}


bool grcImage::SaveDDS(const char* path, const void* bits, int width, int height, Format format)
{
	FILE* fp = fopen(path, "wb");

	if (fp)
	{
		const Format fmt = (Format)(format & (u32)FORMAT_MASK);

		class DDS_PixelFormat // equivalent to DDPIXELFORMAT
		{
		public:
			u32 dwSize;
			u32 dwFlags;
			u32 dwFourCC;
			u32 dwRGBBitCount;
			u32 dwRBitMask;
			u32 dwGBitMask;
			u32 dwBBitMask;
			u32 dwABitMask;
		};

		class DDS_Header // equivalent to DDSURFACEDESC2 (includes dwMagic)
		{
		public:
			u32 dwMagic; // must be "DDS "
			u32 dwSize;  // must be 124
			u32 dwFlags;
			u32 dwHeight;
			u32 dwWidth;
			u32 dwPitchOrLinearSize;
			u32 dwDepth;
			u32 dwMipMapCount;
			u32 dwReserved1[11];
			DDS_PixelFormat ddpf;
			u32 ddsCaps[4];
			u32 dwReserved2[1];
		};

		DDS_Header header;

		memset(&header, 0, sizeof(header));

		header.dwMagic     = MAKE_MAGIC_NUMBER('D','D','S',' ');
		header.dwSize      = sizeof(header) - sizeof(header.dwMagic); // 124
		header.dwFlags     = 0x00001007; // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
		header.dwHeight    = height;
		header.dwWidth     = width;
		header.ddpf.dwSize = sizeof(header.ddpf); // 32
		header.ddsCaps[0]  = 0x00001002; // DDSCAPS_ALPHA | DDSCAPS_TEXTURE

		if (fmt == A8R8G8B8)
		{
			header.ddpf.dwFlags       = DDPF_RGB | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 32;
			header.ddpf.dwRBitMask    = 0x00ff0000;
			header.ddpf.dwGBitMask    = 0x0000ff00;
			header.ddpf.dwBBitMask    = 0x000000ff;
			header.ddpf.dwABitMask    = 0xff000000;
		}
		else if (fmt == A8B8G8R8)
		{
			header.ddpf.dwFlags       = DDPF_RGB | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 32;
			header.ddpf.dwRBitMask    = 0x000000ff;
			header.ddpf.dwGBitMask    = 0x0000ff00;
			header.ddpf.dwBBitMask    = 0x00ff0000;
			header.ddpf.dwABitMask    = 0xff000000;
		}
		else if (fmt == A8)
		{
			header.ddpf.dwFlags       = DDPF_ALPHA;
			header.ddpf.dwRGBBitCount = 8;
			header.ddpf.dwABitMask    = 0xff;
		}
		else if (fmt == L8)
		{
			header.ddpf.dwFlags       = DDPF_LUMINANCE;
			header.ddpf.dwRGBBitCount = 8;
			header.ddpf.dwRBitMask    = 0xff;
		}
		else if (fmt == A8L8)
		{
			header.ddpf.dwFlags       = DDPF_LUMINANCE | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 16;
			header.ddpf.dwRBitMask    = 0x00ff;
			header.ddpf.dwABitMask    = 0xff00;
		}
		else if (fmt == A4R4G4B4)
		{
			header.ddpf.dwFlags       = DDPF_RGB | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 16;
			header.ddpf.dwRBitMask    = 0x0f00;
			header.ddpf.dwGBitMask    = 0x00f0;
			header.ddpf.dwBBitMask    = 0x000f;
			header.ddpf.dwABitMask    = 0xf000;
		}
		else if (fmt == A1R5G5B5)
		{
			header.ddpf.dwFlags       = DDPF_RGB | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 16;
			header.ddpf.dwRBitMask    = 0x7c00;
			header.ddpf.dwGBitMask    = 0x03e0;
			header.ddpf.dwBBitMask    = 0x001f;
			header.ddpf.dwABitMask    = 0x8000;
		}
		else if (fmt == R5G6B5)
		{
			header.ddpf.dwFlags       = DDPF_RGB;
			header.ddpf.dwRGBBitCount = 16;
			header.ddpf.dwRBitMask    = 0xf800;
			header.ddpf.dwGBitMask    = 0x07e0;
			header.ddpf.dwBBitMask    = 0x001f;
		}
		else if (fmt == R3G3B2)
		{
			header.ddpf.dwFlags       = DDPF_RGB;
			header.ddpf.dwRGBBitCount = 8;
			header.ddpf.dwRBitMask    = 0xe0;
			header.ddpf.dwGBitMask    = 0x1c;
			header.ddpf.dwBBitMask    = 0x03;
		}
		else if (fmt == A8R3G3B2)
		{
			header.ddpf.dwFlags       = DDPF_RGB | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 16;
			header.ddpf.dwRBitMask    = 0x00e0;
			header.ddpf.dwGBitMask    = 0x001c;
			header.ddpf.dwBBitMask    = 0x0003;
			header.ddpf.dwABitMask    = 0xff00;
		}
		else if (fmt == A4L4)
		{
			header.ddpf.dwFlags       = DDPF_LUMINANCE | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 8;
			header.ddpf.dwRBitMask    = 0x0f;
			header.ddpf.dwABitMask    = 0xf0;
		}
		else if (fmt == A2R10G10B10)
		{
			header.ddpf.dwFlags       = DDPF_RGB | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 32;
			header.ddpf.dwRBitMask    = 0x3ff00000;
			header.ddpf.dwGBitMask    = 0x000ffc00;
			header.ddpf.dwBBitMask    = 0x000003ff;
			header.ddpf.dwABitMask    = 0xc0000000;
		}
		else if (fmt == A2B10G10R10)
		{
			header.ddpf.dwFlags       = DDPF_RGB | DDPF_ALPHAPIXELS;
			header.ddpf.dwRGBBitCount = 32;
			header.ddpf.dwRBitMask    = 0x000003ff;
			header.ddpf.dwGBitMask    = 0x000ffc00;
			header.ddpf.dwBBitMask    = 0x3ff00000;
			header.ddpf.dwABitMask    = 0xc0000000;
		}
		else if (fmt == G16R16)
		{
			header.ddpf.dwFlags       = DDPF_RGB;
			header.ddpf.dwRGBBitCount = 32;
			header.ddpf.dwRBitMask    = 0x0000ffff;
			header.ddpf.dwGBitMask    = 0xffff0000;
		}
		else if (fmt == L16)
		{
			header.ddpf.dwFlags       = DDPF_LUMINANCE;
			header.ddpf.dwRGBBitCount = 16;
			header.ddpf.dwRBitMask    = 0xffff;
		}
		else if (fmt == R8)
		{
			header.ddpf.dwFlags       = DDPF_RGB;
			header.ddpf.dwRGBBitCount = 8;
			header.ddpf.dwRBitMask    = 0xff;
		}
		else if (fmt == R16)
		{
			header.ddpf.dwFlags       = DDPF_RGB;
			header.ddpf.dwRGBBitCount = 16;
			header.ddpf.dwRBitMask    = 0xffff;
		}
		else if (fmt == G8R8)
		{
			header.ddpf.dwFlags       = DDPF_RGB;
			header.ddpf.dwRGBBitCount = 16;
			header.ddpf.dwRBitMask    = 0x00ff;
			header.ddpf.dwGBitMask    = 0xff00;
		}
		else
		{
			header.ddpf.dwFlags = DDPF_FOURCC;

			switch ((int)fmt)
			{
			case DXT1          : header.ddpf.dwFourCC = DDS_D3DFMT_DXT1         ; break;
			case DXT3          : header.ddpf.dwFourCC = DDS_D3DFMT_DXT3         ; break;
			case DXT5          : header.ddpf.dwFourCC = DDS_D3DFMT_DXT5         ; break;
			case CTX1          : header.ddpf.dwFourCC = DDS_D3DFMT_CTX1         ; break; // custom FOURCC
			case DXT3A         : header.ddpf.dwFourCC = DDS_D3DFMT_DXT3A        ; break; // custom FOURCC
			case DXT3A_1111    : header.ddpf.dwFourCC = DDS_D3DFMT_DXT3A_1111   ; break; // custom FOURCC
			case DXT5A         : header.ddpf.dwFourCC = DDS_D3DFMT_DXT5A        ; break;
			case DXN           : header.ddpf.dwFourCC = DDS_D3DFMT_DXN          ; break;
			case BC6           : header.ddpf.dwFourCC = DDS_D3DFMT_BC6          ; break;
			case BC7           : header.ddpf.dwFourCC = DDS_D3DFMT_BC7          ; break;
			case A16B16G16R16  : header.ddpf.dwFourCC = DDS_D3DFMT_A16B16G16R16 ; break;
			case A16B16G16R16F : header.ddpf.dwFourCC = DDS_D3DFMT_A16B16G16R16F; break;
			case G16R16F       : header.ddpf.dwFourCC = DDS_D3DFMT_G16R16F      ; break;
			case R16F          : header.ddpf.dwFourCC = DDS_D3DFMT_R16F         ; break;
			case A32B32G32R32F : header.ddpf.dwFourCC = DDS_D3DFMT_A32B32G32R32F; break;
			case G32R32F       : header.ddpf.dwFourCC = DDS_D3DFMT_G32R32F      ; break;
			case R32F          : header.ddpf.dwFourCC = DDS_D3DFMT_R32F         ; break;
			}
		}

		fwrite(&header, sizeof(header), 1, fp);

		const int bs = IsFormatDXTBlockCompressed(fmt) ? 4 : 1; // block size in pixels (bs x bs)
		const int bw = (width  + bs - 1)/bs; // width in blocks
		const int bh = (height + bs - 1)/bs; // height in blocks

		fwrite(bits, 1, bw*bh*(bs*bs*GetFormatBitsPerPixel(fmt))/8, fp);
		fclose(fp);

		return true;
	}

	return false;
}

#endif // __RESOURCECOMPILER

void grcImage::SetMinMipSize(u32 minCount) {
	Assert(minCount <= sm_MaxMipSize);
	sm_MinMipSize = minCount;
}


u32 grcImage::GetMinMipSize() {
	return sm_MinMipSize;
}


void grcImage::SetMaxMipSize(u32 maxCount) {
	Assert(maxCount >= sm_MinMipSize);
	sm_MaxMipSize = maxCount;
}


u32 grcImage::GetMaxMipLevels() {
	return sm_MaxMipLevels;
}

void grcImage::SetMaxMipLevels(u32 maxCount) {
	Assert (maxCount>=1);
	sm_MaxMipLevels = maxCount;
}

u32 grcImage::GetMinMipLevels() {
	return sm_MinMipLevels;
}

void grcImage::SetMinMipLevels(u32 minCount) {
	// Assert (minCount>=0);
	sm_MinMipLevels = minCount;
}

u32 grcImage::GetMaxMipSize() {
	return sm_MaxMipSize;
}

void grcImage::SetMaxMipBytes(u32 maxBytes) {
	sm_MaxMipBytes = maxBytes;
}

u32 grcImage::GetMaxMipBytes() {
	return sm_MaxMipBytes;
}


void grcImage::SetSizeRatio(float ratio) {
	Assert(ratio > 0.0f && ratio <= 1.0f);
	sm_SizeRatio = ratio;
}


float grcImage::GetSizeRatio() {
	return sm_SizeRatio;
}


static bool CompareDDSFormatMask(const DDPIXELFORMAT9& ddpf, u32 flags, u32 bitCount, u32 rMask, u32 gMask, u32 bMask, u32 aMask)
{
	return
	(
		ddpf.dwFlags       == flags    &&
		ddpf.dwRGBBitCount == bitCount &&
		ddpf.dwRBitMask    == rMask    &&
		ddpf.dwGBitMask    == gMask    &&
		ddpf.dwBBitMask    == bMask    &&
		ddpf.dwABitMask    == aMask
	);
}

enum { DDS_D3DFMT_P4 = MAKE_MAGIC_NUMBER('P','4',' ',' ') };

static grcImage::Format GetDDSFormat(const DDPIXELFORMAT9& ddpf)
{
	if (ddpf.dwFlags & DDPF_FOURCC) switch (ddpf.dwFourCC)
	{
	case DDS_D3DFMT_DXT1          : return grcImage::DXT1         ;
	case DDS_D3DFMT_DXT3          : return grcImage::DXT3         ;
	case DDS_D3DFMT_DXT5          : return grcImage::DXT5         ;
	case DDS_D3DFMT_CTX1          : return grcImage::CTX1         ; // custom FOURCC
	case DDS_D3DFMT_DXT3A         : return grcImage::DXT3A        ; // custom FOURCC
	case DDS_D3DFMT_DXT3A_1111    : return grcImage::DXT3A_1111   ; // custom FOURCC
	case DDS_D3DFMT_DXT5A         : return grcImage::DXT5A        ;
	case DDS_D3DFMT_DXN           : return grcImage::DXN          ;
	case DDS_D3DFMT_BC6           : return grcImage::BC6          ;
	case DDS_D3DFMT_BC7           : return grcImage::BC7          ;

	case DDS_D3DFMT_A32B32G32R32F : return grcImage::A32B32G32R32F;
	case DDS_D3DFMT_G32R32F       : return grcImage::G32R32F      ;
	case DDS_D3DFMT_R32F          : return grcImage::R32F         ;
	case DDS_D3DFMT_A16B16G16R16F : return grcImage::A16B16G16R16F;
	case DDS_D3DFMT_G16R16F       : return grcImage::G16R16F      ;
	case DDS_D3DFMT_R16F          : return grcImage::R16F         ;
	case DDS_D3DFMT_A16B16G16R16  : return grcImage::A16B16G16R16 ;

	case DDS_D3DFMT_D15S1         : return grcImage::D15S1        ;
	case DDS_D3DFMT_D24S8         : return grcImage::D24S8        ;
	case DDS_D3DFMT_D24FS8        : return grcImage::D24FS8       ;
	case DDS_D3DFMT_P4            : return grcImage::P4           ; // custom FOURCC
	case DDS_D3DFMT_P8            : return grcImage::P8           ;
	case DDS_D3DFMT_A8P8          : return grcImage::A8P8         ;
	}
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)) { return grcImage::A8R8G8B8   ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000)) { return grcImage::A8B8G8R8   ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_ALPHA                       ,  8, 0x00000000, 0x00000000, 0x00000000, 0x000000FF)) { return grcImage::A8         ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_LUMINANCE                   ,  8, 0x000000FF, 0x00000000, 0x00000000, 0x00000000)) { return grcImage::L8         ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_LUMINANCE | DDPF_ALPHAPIXELS, 16, 0x000000FF, 0x00000000, 0x00000000, 0x0000FF00)) { return grcImage::A8L8       ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 16, 0x00000F00, 0x000000F0, 0x0000000F, 0x0000F000)) { return grcImage::A4R4G4B4   ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 16, 0x00007C00, 0x000003E0, 0x0000001F, 0x00008000)) { return grcImage::A1R5G5B5   ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         , 16, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000)) { return grcImage::R5G6B5     ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         ,  8, 0x000000E0, 0x0000001C, 0x00000003, 0x0000FF00)) { return grcImage::R3G3B2     ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 16, 0x000000E0, 0x0000001C, 0x00000003, 0x0000FF00)) { return grcImage::A8R3G3B2   ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_LUMINANCE | DDPF_ALPHAPIXELS,  8, 0x0000000F, 0x00000000, 0x00000000, 0x000000F0)) { return grcImage::A4L4       ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 32, 0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000)) { return grcImage::A2R10G10B10; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 32, 0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000)) { return grcImage::A2B10G10R10; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         , 32, 0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000)) { return grcImage::G16R16     ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_LUMINANCE                   , 16, 0x0000FFFF, 0x00000000, 0x00000000, 0x00000000)) { return grcImage::L16        ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         ,  8, 0x000000FF, 0x00000000, 0x00000000, 0x00000000)) { return grcImage::R8         ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         , 16, 0x0000FFFF, 0x00000000, 0x00000000, 0x00000000)) { return grcImage::R16        ; }
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         , 16, 0x000000FF, 0x0000FF00, 0x00000000, 0x00000000)) { return grcImage::G8R8       ; }
	// support XRGB/XBGR formats too ..
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         , 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000)) { return grcImage::A8R8G8B8   ; } // X8R8G8B8
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         , 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000)) { return grcImage::A8B8G8R8   ; } // X8B8G8R8
	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         , 16, 0x00000F00, 0x000000F0, 0x0000000F, 0x00000000)) { return grcImage::A4R4G4B4   ; } // X4R4G4B4
 	else if (CompareDDSFormatMask(ddpf, DDPF_RGB                         , 16, 0x00007C00, 0x000003E0, 0x0000001F, 0x00000000)) { return grcImage::A1R5G5B5   ; } // X1R5G5B5

	Displayf("unknown dds pixel format:");
	Displayf("  dwSize=%d", ddpf.dwSize);
	Displayf("  dwFlags=%08x", ddpf.dwFlags);
	Displayf("  dwFourCC=%d", ddpf.dwFourCC);
	Displayf("  dwRGBBitCount=%d", ddpf.dwRGBBitCount);
	Displayf("  dwRBitMask=%08x", ddpf.dwRBitMask);
	Displayf("  dwGBitMask=%08x", ddpf.dwGBitMask);
	Displayf("  dwBBitMask=%08x", ddpf.dwBBitMask);
	Displayf("  dwABitMask=%08x", ddpf.dwABitMask);

	return grcImage::UNKNOWN;
}

static void SetDDSFormatMask(DDPIXELFORMAT9& ddpf, u32 flags, u32 bitCount, u32 rMask, u32 gMask, u32 bMask, u32 aMask)
{
	ddpf.dwSize        = sizeof(ddpf);
	ddpf.dwFlags       = flags;
	ddpf.dwRGBBitCount = bitCount;
	ddpf.dwRBitMask    = rMask;
	ddpf.dwGBitMask    = gMask;
	ddpf.dwBBitMask    = bMask;
	ddpf.dwABitMask    = aMask;
}

static bool SetDDSFormat(DDPIXELFORMAT9& ddpf, grcImage::Format format)
{
	ddpf.dwSize = sizeof(ddpf);

	switch (format)
	{
	case grcImage::UNKNOWN       : memset(&ddpf, 0, sizeof(ddpf)); break;

	case grcImage::DXT1          : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_DXT1         ; break;
	case grcImage::DXT3          : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_DXT3         ; break;
	case grcImage::DXT5          : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_DXT5         ; break;
	case grcImage::CTX1          : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_CTX1         ; break; // custom FOURCC
	case grcImage::DXT3A         : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_DXT3A        ; break; // custom FOURCC
	case grcImage::DXT3A_1111    : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_DXT3A_1111   ; break; // custom FOURCC
	case grcImage::DXT5A         : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_DXT5A        ; break;
	case grcImage::DXN           : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_DXN          ; break;
	case grcImage::BC6           : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_BC6          ; break;
	case grcImage::BC7           : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_BC7          ; break;

	case grcImage::A32B32G32R32F : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_A32B32G32R32F; break;
	case grcImage::G32R32F       : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_G32R32F      ; break;
	case grcImage::R32F          : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_R32F         ; break;
	case grcImage::A16B16G16R16F : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_A16B16G16R16F; break;
	case grcImage::G16R16F       : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_G16R16F      ; break;
	case grcImage::R16F          : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_R16F         ; break;
	case grcImage::A16B16G16R16  : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_A16B16G16R16 ; break;

	/*
	case grcImage::D15S1         : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_D15S1        ; break;
	case grcImage::D24S8         : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_D24S8        ; break;
	case grcImage::D24FS8        : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_D24FS8       ; break;
	case grcImage::P4            : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_P4           ; break; // custom FOURCC
	case grcImage::P8            : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_P8           ; break;
	case grcImage::A8P8          : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_A8P8         ; break;
	*/
	case grcImage::D15S1         : SetDDSFormatMask(ddpf, DDPF_RGB	                       , 16, 0x0000FFFE, 0x00000001, 0x00000000, 0x00000000); break;
	case grcImage::D24S8         : SetDDSFormatMask(ddpf, DDPF_RGB	                       , 32, 0xFFFFFF00, 0x000000FF, 0x00000000, 0x00000000); break;
	case grcImage::D24FS8        : SetDDSFormatMask(ddpf, DDPF_RGB	                       , 32, 0xFFFFFF00, 0x000000FF, 0x00000000, 0x00000000); break;
	case grcImage::P4            : SetDDSFormatMask(ddpf, DDPF_RGB | DDPF_ALPHAPIXELS	   ,  4, 0x0000000F, 0x00000000, 0x00000000, 0x000000F0); break;
	case grcImage::P8            : SetDDSFormatMask(ddpf, DDPF_ALPHA                       ,  8, 0x00000000, 0x00000000, 0x00000000, 0x000000FF); break;
	case grcImage::A8P8          : SetDDSFormatMask(ddpf, DDPF_RGB | DDPF_ALPHAPIXELS	   , 16, 0x000000FF, 0x00000000, 0x00000000, 0x0000FF00); break;

	case grcImage::A8R8G8B8      : SetDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000); break;
	case grcImage::A8B8G8R8      : SetDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000); break;
	case grcImage::A8            : SetDDSFormatMask(ddpf, DDPF_ALPHA                       ,  8, 0x00000000, 0x00000000, 0x00000000, 0x000000FF); break;
	case grcImage::L8            : SetDDSFormatMask(ddpf, DDPF_LUMINANCE                   ,  8, 0x000000FF, 0x00000000, 0x00000000, 0x00000000); break;
	case grcImage::A8L8          : SetDDSFormatMask(ddpf, DDPF_LUMINANCE | DDPF_ALPHAPIXELS, 16, 0x000000FF, 0x00000000, 0x00000000, 0x0000FF00); break;
	case grcImage::A4R4G4B4      : SetDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 16, 0x00000F00, 0x000000F0, 0x0000000F, 0x0000F000); break;
	case grcImage::A1R5G5B5      : SetDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 16, 0x00007C00, 0x000003E0, 0x0000001F, 0x00008000); break;
	case grcImage::R5G6B5        : SetDDSFormatMask(ddpf, DDPF_RGB                         , 16, 0x0000F800, 0x000007E0, 0x0000001F, 0x00000000); break;
	case grcImage::R3G3B2        : SetDDSFormatMask(ddpf, DDPF_RGB                         ,  8, 0x000000E0, 0x0000001C, 0x00000003, 0x0000FF00); break;
	case grcImage::A8R3G3B2      : SetDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 16, 0x000000E0, 0x0000001C, 0x00000003, 0x0000FF00); break;
	case grcImage::A4L4          : SetDDSFormatMask(ddpf, DDPF_LUMINANCE | DDPF_ALPHAPIXELS,  8, 0x0000000F, 0x00000000, 0x00000000, 0x000000F0); break;
	case grcImage::A2R10G10B10   : SetDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 32, 0x3FF00000, 0x000FFC00, 0x000003FF, 0xC0000000); break;
	case grcImage::A2B10G10R10   : SetDDSFormatMask(ddpf, DDPF_RGB       | DDPF_ALPHAPIXELS, 32, 0x000003FF, 0x000FFC00, 0x3FF00000, 0xC0000000); break;
	case grcImage::G16R16        : SetDDSFormatMask(ddpf, DDPF_RGB                         , 32, 0x0000FFFF, 0xFFFF0000, 0x00000000, 0x00000000); break;
	case grcImage::L16           : SetDDSFormatMask(ddpf, DDPF_LUMINANCE                   , 16, 0x0000FFFF, 0x00000000, 0x00000000, 0x00000000); break;
	case grcImage::R8            : SetDDSFormatMask(ddpf, DDPF_RGB                         ,  8, 0x000000FF, 0x00000000, 0x00000000, 0x00000000); break;
	case grcImage::R16           : SetDDSFormatMask(ddpf, DDPF_RGB                         , 16, 0x0000FFFF, 0x00000000, 0x00000000, 0x00000000); break;
	case grcImage::G8R8          : SetDDSFormatMask(ddpf, DDPF_RGB                         , 16, 0x000000FF, 0x0000FF00, 0x00000000, 0x00000000); break;

	case grcImage::LINA32B32G32R32F_DEPRECATED : ddpf.dwFlags = DDPF_FOURCC; ddpf.dwFourCC = DDS_D3DFMT_A32B32G32R32F; break;
	case grcImage::LINA8R8G8B8_DEPRECATED      : SetDDSFormatMask(ddpf, DDPF_RGB | DDPF_ALPHAPIXELS, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000); break;
	case grcImage::LIN8_DEPRECATED             : SetDDSFormatMask(ddpf, DDPF_LUMINANCE             ,  8, 0x000000FF, 0x00000000, 0x00000000, 0x00000000); break;
	case grcImage::RGBE                        : SetDDSFormatMask(ddpf, DDPF_RGB | DDPF_ALPHAPIXELS, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000); break;
	}

	return true;
}

#define STREAM_READ_RETURN { delete result; grcErrorf("Stream read on body for '%s'",filename); return NULL; }

struct _DDS_HEADER_DXT10
{
	u32 dxgiFormat;
	u32 resourceDimension;
	u32 miscFlag; // see DDS_RESOURCE_MISC_FLAG
	u32 arraySize;
	u32 miscFlags2; // see DDS_MISC_FLAGS2
};

grcImage* grcImage::LoadDDS(const char *filename) {
	fiSafeStream S = ASSET.Open(filename, "dds", true, true);
	if (!S) {
		ASSET.FindPath(filename, "dds", true);
		return NULL;
	}
	int magic = 0;
	S->ReadInt(&magic, 1);
	if (magic != MAKE_MAGIC_NUMBER('D','D','S',' '))
		return NULL;

	DDSURFACEDESC2 header;
	if (S->ReadInt((u32*)&header,sizeof(header)/sizeof(u32)) != sizeof(header)/sizeof(u32)) {
		grcErrorf("Short read on header for '%s'",filename);
		S->Close();
		return NULL;
	}
	if (header.dwSize != sizeof(header) || header.ddpfPixelFormat.dwSize != sizeof(DDPIXELFORMAT9))
		return NULL;

	// Validate all known types
	grcImage *result = NULL;
	DDPIXELFORMAT9 ddpf = header.ddpfPixelFormat;
	u32 mipcount = (header.dwFlags & DDSD_MIPMAPCOUNT)? header.dwMipMapCount : 1;
	u32 layerCount = 1;
	u32 depth = 1;
	ImageType type = STANDARD;
	if ( header.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP ) {
		u32 faces = header.ddsCaps.dwCaps2;
		layerCount  = (faces & DDSCAPS2_CUBEMAP_POSITIVEX) ? 1 : 0;
		layerCount += (faces & DDSCAPS2_CUBEMAP_NEGATIVEX) ? 1 : 0;
		layerCount += (faces & DDSCAPS2_CUBEMAP_POSITIVEY) ? 1 : 0;
		layerCount += (faces & DDSCAPS2_CUBEMAP_NEGATIVEY) ? 1 : 0;
		layerCount += (faces & DDSCAPS2_CUBEMAP_POSITIVEZ) ? 1 : 0;
		layerCount += (faces & DDSCAPS2_CUBEMAP_NEGATIVEZ) ? 1 : 0;
		type = CUBE;
		AssertMsg(layerCount == 6, "Currently only support cubemaps with all 6 sides");
	}
	else if ( header.ddsCaps.dwCaps2 & DDSCAPS2_VOLUME ) {
		type = VOLUME;
		depth = header.dwDepth;
	}

	Format format = UNKNOWN;

	if (header.ddpfPixelFormat.dwFourCC == MAKE_MAGIC_NUMBER('D','X','1','0')) {
		_DDS_HEADER_DXT10 dx10header;
		if (S->ReadInt((u32*)&dx10header,sizeof(dx10header)/sizeof(u32)) != sizeof(dx10header)/sizeof(u32)) {
			grcErrorf("Short read on dx10 header for '%s'",filename);
			S->Close();
			return NULL;
		}

		bool format_sRGB = false;

		// convert dxgi format to old-style dx9 format
		switch (dx10header.dxgiFormat)
		{
		case 71: format = DXT1         ;                     break; // DXGI_FORMAT_BC1_UNORM          
		case 72: format = DXT1         ; format_sRGB = true; break; // DXGI_FORMAT_BC1_UNORM_SRGB     
		case 74: format = DXT3         ;                     break; // DXGI_FORMAT_BC2_UNORM          
		case 75: format = DXT3         ; format_sRGB = true; break; // DXGI_FORMAT_BC2_UNORM_SRGB     
		case 77: format = DXT5         ;                     break; // DXGI_FORMAT_BC3_UNORM          
		case 78: format = DXT5         ; format_sRGB = true; break; // DXGI_FORMAT_BC3_UNORM_SRGB     
		case 80: format = DXT5A        ;                     break; // DXGI_FORMAT_BC4_UNORM          
		case 83: format = DXN          ;                     break; // DXGI_FORMAT_BC5_UNORM          
		case 95: format = BC6          ;                     break; // DXGI_FORMAT_BC6H_UF16          
		case 98: format = BC7          ;                     break; // DXGI_FORMAT_BC7_UNORM          
		case 99: format = BC7          ; format_sRGB = true; break; // DXGI_FORMAT_BC7_UNORM_SRGB          
		case 87: format = A8R8G8B8     ;                     ddpf.dwABitMask = 0xff000000; break; // DXGI_FORMAT_B8G8R8A8_UNORM     
		case 88: format = A8R8G8B8     ;                     break; // DXGI_FORMAT_B8G8R8X8_UNORM
		case 93: format = A8R8G8B8     ; format_sRGB = true; break; // DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
		case 91: format = A8R8G8B8     ; format_sRGB = true; ddpf.dwABitMask = 0xff000000; break; // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
		case 28: format = A8B8G8R8     ;                     ddpf.dwABitMask = 0xff000000; break; // DXGI_FORMAT_R8G8B8A8_UNORM     
		case 29: format = A8B8G8R8     ; format_sRGB = true; ddpf.dwABitMask = 0xff000000; break; // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
		case 49: format = G8R8         ;                     break; // DXGI_FORMAT_R8G8_UNORM         
		case 61: format = R8           ;                     break; // DXGI_FORMAT_R8_UNORM           
		case 65: format = A8           ;                     ddpf.dwABitMask = 0x000000ff; break; // DXGI_FORMAT_A8_UNORM           
		case 2 : format = A32B32G32R32F;                     break; // DXGI_FORMAT_R32G32B32A32_FLOAT 
		case 10: format = A16B16G16R16F;                     break; // DXGI_FORMAT_R16G16B16A16_FLOAT 
		case 11: format = A16B16G16R16 ;                     break; // DXGI_FORMAT_R16G16B16A16_UNORM 
		case 16: format = G32R32F      ;                     break; // DXGI_FORMAT_R32G32_FLOAT       
		case 34: format = G16R16F      ;                     break; // DXGI_FORMAT_R16G16_FLOAT       
		case 35: format = G16R16       ;                     break; // DXGI_FORMAT_R16G16_UNORM       
		case 41: format = R32F         ;                     break; // DXGI_FORMAT_R32_FLOAT          
		case 54: format = R16F         ;                     break; // DXGI_FORMAT_R16_FLOAT          
		case 56: format = R16          ;                     break; // DXGI_FORMAT_R16_UNORM          
		case 24: format = A2B10G10R10  ;                     ddpf.dwABitMask = 0xc0000000; break; // DXGI_FORMAT_R10G10B10A2_UNORM  
		case 86: format = A1R5G5B5     ;                     ddpf.dwABitMask = 0x00008000; break; // DXGI_FORMAT_B5G5R5A1_UNORM     
		case 85: format = R5G6B5       ;                     break; // DXGI_FORMAT_B5G6R5_UNORM       
		case 115:format = A4R4G4B4     ;                     ddpf.dwABitMask = 0x0000f000; break; // DXGI_FORMAT_B4G4R4A4_UNORM
		default:
			grcErrorf("Unsupported dx10 format %d for '%s'",dx10header.dxgiFormat,filename);
			S->Close();
			return NULL;
		}

		if (format_sRGB)
			format = (Format)((u32)format | FORMAT_FLAG_sRGB);

		layerCount *= dx10header.arraySize;
	}
	else
		format = GetDDSFormat(ddpf);

	u32 minMipLevels = sm_MinMipLevels;
	u32 maxMipLevels = sm_MaxMipLevels;
	u32 minMipSize   = sm_MinMipSize;
	u32 maxMipSize   = sm_MaxMipSize;

	PARAM_minmiplevels.Get(minMipLevels);
	PARAM_maxmiplevels.Get(maxMipLevels);
	PARAM_minmipsize  .Get(minMipSize);
	PARAM_maxmipsize  .Get(maxMipSize);

	// This option overrides -maxmipsize and sm_MaxMipSize
	if (PARAM_removetopmips.Get()) {
		u32 numMipsToRemove = 1; // default to 1 if there is no argument
		PARAM_removetopmips.Get(numMipsToRemove);
		maxMipSize = Max(Max(header.dwWidth, header.dwHeight) >> numMipsToRemove, 1u);
	}

	u32 skip = 0;

#if __RESOURCECOMPILER
	const bool bSupportSizeRatioAndMaxMipSize = !sm_customLoad; // don't do this when using the custom loading!
#else
	const bool bSupportSizeRatioAndMaxMipSize = true; // sure why not
#endif

	// Don't load all miplevels if we're over the maximum.  But it's not safe to skip
	// miplevels if there is more than one layer.
	if (bSupportSizeRatioAndMaxMipSize && layerCount == 1) {
		u32 ignored = 0;
		u32 maxx = (u32)(header.dwWidth * sm_SizeRatio), maxy = (u32)(header.dwHeight * sm_SizeRatio);
		for (u32 i=0; i<mipcount; i++) {
			u32 x = header.dwWidth >> i, y = header.dwHeight >> i;
			if (x==0) x = 1;
			if (y==0) y = 1;
			if (x < minMipSize || y < minMipSize) {
				++ignored;
			}
			else if ((x > maxx || y > maxy) || (x > maxMipSize || y > maxMipSize)) {
				++skip;
			}
		}
		mipcount -= ignored;
		if (!mipcount)
			mipcount = 1;
		if (skip >= mipcount)
			skip = mipcount-1;
		
		if (mipcount>maxMipLevels)  {
			mipcount = maxMipLevels;
			ignored = 0; // so min code below does not mess with us. 
		}
		
		if (mipcount<minMipLevels) {
			u32 needMoreMips = minMipLevels - mipcount;  // how many do we want?
			mipcount += Min(ignored,needMoreMips);		    // we can get back "upto" the ignored amount
			// we could also reclaim some we skipped, but probably don't want to do that...
		}
	}

	if (format != UNKNOWN) {
		if (grcImage::sm_RgbeConversion && ddpf.dwFourCC == DDS_D3DFMT_A16B16G16R16F) {
			format = grcImage::RGBE;
		}
		result = Create(header.dwWidth, header.dwHeight, depth, format, type, mipcount - 1, layerCount - 1);
		grcImage *layer = result;
		while (layer) {
			grcImage *mip = layer;
			while (mip) {
#if !__RESOURCECOMPILER // don't overwrite the constructor info
				if (header.fColorExp[0] || header.fColorOfs[0] ||
					header.fColorExp[1] || header.fColorOfs[1] ||
					header.fColorExp[2] || header.fColorOfs[2])
				{
					mip->m_ColorExp[0] = header.fColorExp[0];
					mip->m_ColorExp[1] = header.fColorExp[1];
					mip->m_ColorExp[2] = header.fColorExp[2];
					mip->m_ColorOfs[0] = header.fColorOfs[0];
					mip->m_ColorOfs[1] = header.fColorOfs[1];
					mip->m_ColorOfs[2] = header.fColorOfs[2];
				}
#endif // !__RESOURCECOMPILER
				const int numBytes = mip->GetSize();

				switch (PPU_ONLY(IsFormatDXTBlockCompressed(format) ? 1 :) GetFormatByteSwapSize(format)) // matching previous behavior, DO NOT BYTE SWAP if running on PS3 .. this is ridiculous
				{
				case 1: if (S->Read     ((s8 *)mip->m_Bits, numBytes/1) != numBytes/1) STREAM_READ_RETURN break;
				case 2: if (S->ReadShort((s16*)mip->m_Bits, numBytes/2) != numBytes/2) STREAM_READ_RETURN break;
				case 4: if (S->ReadInt  ((s32*)mip->m_Bits, numBytes/4) != numBytes/4) STREAM_READ_RETURN break;
				}

				// clear alpha channel to 1 for channels without alpha
				if (ddpf.dwABitMask == 0) {
					if (format == A8R8G8B8 || format == A8B8G8R8) {
						const u32 mask = 0xff000000;
						for (int i = 0; i < numBytes; i += sizeof(u32)) {
							*(u32*)(mip->m_Bits + i) |= mask;
						}
					}
					else if (format == A4R4G4B4 || format == A1R5G5B5) {
						const u16 mask = (format == A4R4G4B4) ? 0xf000 : 0x8000;
						for (int i = 0; i < numBytes; i += sizeof(u16)) {
							*(u16*)(mip->m_Bits + i) |= mask;
						}
					}
				}

				mip = mip->GetNext();
			}
			layer = layer->GetNextLayer();
		}
	}
	else {
		if (ddpf.dwFourCC) {
			if (IS_PRINTABLE_MAGIC_NUMBER(ddpf.dwFourCC)) {
				grcErrorf("%s - Unrecognized FOURCC code %c%c%c%c [%d]", filename, EXPAND_MAGIC_NUMBER(ddpf.dwFourCC), ddpf.dwFourCC);
			}
			else {
				grcErrorf("%s - Unrecognized FOURCC code 0x%08x [%d]", filename, ddpf.dwFourCC, ddpf.dwFourCC);
			}
		}
		else {
			grcErrorf("%s - unsupported DDS format", filename);
		}
	}

	// made the simplest change possible to not screw anyone over to implement MaxMipSize and SizeRatio
	// NOTE -- this only works if the grcImage has mipmaps already! it will not rescale a non-mipmapped image
	if (bSupportSizeRatioAndMaxMipSize && skip && result) {
		while (skip-- && result->GetNext()) {
			grcImage* next = result->GetNext();
			result->m_Next = NULL;
			result->Release();
			result = next;
		}
	}

	if (bSupportSizeRatioAndMaxMipSize && sm_MaxMipBytes) {
		while (result && result->GetSize() > sm_MaxMipBytes && result->GetNext()) {
			grcImage * next = result->GetNext();
			result->m_Next = NULL;
			result->Release();
			result = next;
		}
	}

	return result;
}


bool grcImage::SaveDDS(const char *filename, bool saveAsDX10Format) const {
	fiSafeStream S = ASSET.Create(filename, "dds");
	if (!S)
		return false;
	int magic = MAKE_MAGIC_NUMBER('D','D','S',' ');
	S->WriteInt(&magic, 1);

	DDSURFACEDESC2 header;
	memset(&header,0,sizeof(header));
	header.dwSize = sizeof(header);
	header.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
	header.dwHeight = m_Height;
	header.dwWidth = m_Width;
#if !__RESOURCECOMPILER
	header.fColorExp[0] = m_ColorExp[0];
	header.fColorExp[1] = m_ColorExp[1];
	header.fColorExp[2] = m_ColorExp[2];
	header.fColorOfs[0] = m_ColorOfs[0];
	header.fColorOfs[1] = m_ColorOfs[1];
	header.fColorOfs[2] = m_ColorOfs[2];
#endif // !__RESOURCECOMPILER
	Format format = GetFormat();
	if (IsFormatDXTBlockCompressed(format)) {
		header.dwPitchOrLinearSize = GetSize();
		header.dwFlags |= DDSD_LINEARSIZE;
	}
	else {
		header.dwPitchOrLinearSize = GetStride();
		header.dwFlags |= DDSD_PITCH;
	}
	if (m_Type == VOLUME) {
		header.dwFlags |= DDSD_DEPTH;
		header.dwDepth = m_Depth;
	}
	header.dwMipMapCount = GetExtraMipCount() + 1;
	if (header.dwMipMapCount > 1)
		header.dwFlags |= DDSD_MIPMAPCOUNT;

	if (!SetDDSFormat(header.ddpfPixelFormat, format)) {
		grcErrorf("Unhandled output format (%d) in SaveDDS", format);
		return false;
	}

	_DDS_HEADER_DXT10 dx10header; // only used when saving dx10 format

	header.ddsCaps.dwCaps1 = DDSCAPS_COMPLEX | DDSCAPS_TEXTURE;
	if (header.dwMipMapCount > 1)
		header.ddsCaps.dwCaps1 |= DDSCAPS_MIPMAP;
	if (m_Type == VOLUME)
		header.ddsCaps.dwCaps2 = DDSCAPS2_VOLUME;
	else if (m_Type == CUBE)
		header.ddsCaps.dwCaps2 =
		DDSCAPS2_CUBEMAP |
		DDSCAPS2_CUBEMAP_POSITIVEX |
		DDSCAPS2_CUBEMAP_NEGATIVEX |
		DDSCAPS2_CUBEMAP_POSITIVEY |
		DDSCAPS2_CUBEMAP_NEGATIVEY |
		DDSCAPS2_CUBEMAP_POSITIVEZ |
		DDSCAPS2_CUBEMAP_NEGATIVEZ;
	
	if ((m_Type == CUBE && GetLayerCount() > 6) || (m_Type != CUBE && GetLayerCount() > 1) || saveAsDX10Format) {
		// texture array - must use dx10 format
		sysMemSet(&header.ddpfPixelFormat, 0, sizeof(header.ddpfPixelFormat));
		header.ddpfPixelFormat.dwSize = sizeof(header.ddpfPixelFormat);
		header.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		header.ddpfPixelFormat.dwFourCC = MAKE_MAGIC_NUMBER('D','X','1','0');
		sysMemSet(&dx10header, 0, sizeof(dx10header));
		dx10header.resourceDimension = 3; // DDS_DIMENSION_TEXTURE2D
		dx10header.arraySize = GetLayerCount()/(m_Type == CUBE ? 6 : 1);

		if (IsSRGB()) switch ((int)format)
		{
		case DXT1    : dx10header.dxgiFormat = 72; break; // DXGI_FORMAT_BC1_UNORM_SRGB         
		case DXT3    : dx10header.dxgiFormat = 75; break; // DXGI_FORMAT_BC2_UNORM_SRGB         
		case DXT5    : dx10header.dxgiFormat = 78; break; // DXGI_FORMAT_BC3_UNORM_SRGB         
		case BC7     : dx10header.dxgiFormat = 99; break; // DXGI_FORMAT_BC7_UNORM_SRGB         
		case A8R8G8B8: dx10header.dxgiFormat = 91; break; // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB    
		case A8B8G8R8: dx10header.dxgiFormat = 29; break; // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB    
		}
		else switch ((int)format)
		{
		case DXT1         : dx10header.dxgiFormat = 71; break; // DXGI_FORMAT_BC1_UNORM         
		case DXT3         : dx10header.dxgiFormat = 74; break; // DXGI_FORMAT_BC2_UNORM         
		case DXT5         : dx10header.dxgiFormat = 77; break; // DXGI_FORMAT_BC3_UNORM         
		case DXT5A        : dx10header.dxgiFormat = 80; break; // DXGI_FORMAT_BC4_UNORM         
		case DXN          : dx10header.dxgiFormat = 83; break; // DXGI_FORMAT_BC5_UNORM         
		case BC6          : dx10header.dxgiFormat = 95; break; // DXGI_FORMAT_BC6H_UF16         
		case BC7          : dx10header.dxgiFormat = 98; break; // DXGI_FORMAT_BC7_UNORM         
		case A8R8G8B8     : dx10header.dxgiFormat = 87; break; // DXGI_FORMAT_B8G8R8A8_UNORM    
		case A8B8G8R8     : dx10header.dxgiFormat = 28; break; // DXGI_FORMAT_R8G8B8A8_UNORM    
		case G8R8         : dx10header.dxgiFormat = 49; break; // DXGI_FORMAT_R8G8_UNORM        
		case R8           : dx10header.dxgiFormat = 61; break; // DXGI_FORMAT_R8_UNORM          
		case A8           : dx10header.dxgiFormat = 65; break; // DXGI_FORMAT_A8_UNORM          
		case A32B32G32R32F: dx10header.dxgiFormat = 2 ; break; // DXGI_FORMAT_R32G32B32A32_FLOAT
		case A16B16G16R16F: dx10header.dxgiFormat = 10; break; // DXGI_FORMAT_R16G16B16A16_FLOAT
		case A16B16G16R16 : dx10header.dxgiFormat = 11; break; // DXGI_FORMAT_R16G16B16A16_UNORM
		case G32R32F      : dx10header.dxgiFormat = 16; break; // DXGI_FORMAT_R32G32_FLOAT      
		case G16R16F      : dx10header.dxgiFormat = 34; break; // DXGI_FORMAT_R16G16_FLOAT      
		case G16R16       : dx10header.dxgiFormat = 35; break; // DXGI_FORMAT_R16G16_UNORM      
		case R32F         : dx10header.dxgiFormat = 41; break; // DXGI_FORMAT_R32_FLOAT         
		case R16F         : dx10header.dxgiFormat = 54; break; // DXGI_FORMAT_R16_FLOAT         
		case R16          : dx10header.dxgiFormat = 56; break; // DXGI_FORMAT_R16_UNORM         
		case A2B10G10R10  : dx10header.dxgiFormat = 24; break; // DXGI_FORMAT_R10G10B10A2_UNORM 
		case A1R5G5B5     : dx10header.dxgiFormat = 86; break; // DXGI_FORMAT_B5G5R5A1_UNORM    
		case R5G6B5       : dx10header.dxgiFormat = 85; break; // DXGI_FORMAT_B5G6R5_UNORM      
		}

		if (dx10header.dxgiFormat == 0) { // DXGI_FORMAT_UNKNOWN
			grcErrorf("Unhandled format (%d) in SaveDDS while saving dx10 format", format);
			return false;
		}
	}

	// header consists of 4-byte fields (ints, floats) so can be 4-byte swapped
	CompileTimeAssert(sizeof(header)%sizeof(u32) == 0);
	S->WriteInt((const u32*)&header, sizeof(header)/sizeof(u32));

	if (header.ddpfPixelFormat.dwFourCC == MAKE_MAGIC_NUMBER('D','X','1','0')) {
		CompileTimeAssert(sizeof(dx10header)%sizeof(u32) == 0);
		S->WriteInt((const u32*)&dx10header, sizeof(dx10header)/sizeof(u32));
	}

	const grcImage *layer = this;
	while (layer) {
		const grcImage *mip = layer;
		while (mip) {
			const int numBytes = mip->GetSize();
			switch (GetFormatByteSwapSize(format)) // TODO -- saving DXT compressed images from PS3 is not working .. force byte-swap size to 1 to fix it (haven't tested this on 360 yet)
			{
			case 1: S->Write     ((s8 *)mip->m_Bits, numBytes/1); break;
			case 2: S->WriteShort((s16*)mip->m_Bits, numBytes/2); break;
			case 4: S->WriteInt  ((s32*)mip->m_Bits, numBytes/4); break;
			}
			mip = mip->GetNext();
		}
		layer = layer->GetNextLayer();
	}

	return true;
}


fiStream* grcImage::SaveDDSHeader(const char *filename,int width,int height) {
	fiStream *S = ASSET.Create(filename,"dds");
	if (!S)
		return NULL;
	DDSURFACEDESC2 header;
	memset(&header,0,sizeof(header));
	int magic = MAKE_MAGIC_NUMBER('D','D','S',' ');
	S->WriteInt(&magic, 1);

	header.dwSize = sizeof(header);
	header.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
	header.dwHeight = height;
	header.dwWidth = width;
	header.dwPitchOrLinearSize = width * 4;
	header.dwFlags |= DDSD_PITCH;

	DDPIXELFORMAT9 &ddpf = header.ddpfPixelFormat;
	ddpf.dwSize = sizeof(ddpf);
	ddpf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	ddpf.dwRGBBitCount = 32;
	ddpf.dwRBitMask = 0xFF0000;
	ddpf.dwGBitMask = 0xFF00;
	ddpf.dwBBitMask = 0xFF;
	ddpf.dwABitMask = 0xFF000000;

	header.ddsCaps.dwCaps1 = DDSCAPS_COMPLEX | DDSCAPS_TEXTURE;

	S->WriteInt(&header.dwSize,sizeof(header)/4);
	return S;
}


void grcImage::FlipVertical() {
	Format format = GetFormat();
	switch (format) {
		case DXT1:
			{
				for (int y=0; y<m_Height>>1; y+=4) {
					DXT1_BLOCK *block0 = (DXT1_BLOCK*) (m_Bits + GetStride() * y);
					DXT1_BLOCK *block1 = (DXT1_BLOCK*) (m_Bits + GetStride() * (m_Height - y - 4));
					for (int x=0; x<m_Width; x+=4, ++block0, ++block1) {
						DXT1_BLOCK temp = block0[0];
						block0->c0 = block1->c0;
						block0->c1 = block1->c1;
						block0->rows[0] = block1->rows[3];
						block0->rows[1] = block1->rows[2];
						block0->rows[2] = block1->rows[1];
						block0->rows[3] = block1->rows[0];
						block1->c0 = temp.c0;
						block1->c1 = temp.c1;
						block1->rows[0] = temp.rows[3];
						block1->rows[1] = temp.rows[2];
						block1->rows[2] = temp.rows[1];
						block1->rows[3] = temp.rows[0];
					}
				}
			}
			break;
		case DXT3:
			{
				for (int y=0; y<m_Height>>1; y+=4) {
					DXT3_BLOCK *block0 = (DXT3_BLOCK*) (m_Bits + GetStride() * y);
					DXT3_BLOCK *block1 = (DXT3_BLOCK*) (m_Bits + GetStride() * (m_Height - y - 4));
					for (int x=0; x<m_Width; x+=4, ++block0, ++block1) {
						DXT3_BLOCK temp = block0[0];
						block0->alphaBits[0] = block1->alphaBits[6];
						block0->alphaBits[1] = block1->alphaBits[7];
						block0->alphaBits[2] = block1->alphaBits[4];
						block0->alphaBits[3] = block1->alphaBits[5];
						block0->alphaBits[4] = block1->alphaBits[2];
						block0->alphaBits[5] = block1->alphaBits[3];
						block0->alphaBits[6] = block1->alphaBits[0];
						block0->alphaBits[7] = block1->alphaBits[1];
						block0->c0 = block1->c0;
						block0->c1 = block1->c1;
						block0->rows[0] = block1->rows[3];
						block0->rows[1] = block1->rows[2];
						block0->rows[2] = block1->rows[1];
						block0->rows[3] = block1->rows[0];
						block1->c0 = temp.c0;
						block1->c1 = temp.c1;
						block1->rows[0] = temp.rows[3];
						block1->rows[1] = temp.rows[2];
						block1->rows[2] = temp.rows[1];
						block1->rows[3] = temp.rows[0];
						block1->alphaBits[0] = temp.alphaBits[6];
						block1->alphaBits[1] = temp.alphaBits[7];
						block1->alphaBits[2] = temp.alphaBits[4];
						block1->alphaBits[3] = temp.alphaBits[5];
						block1->alphaBits[4] = temp.alphaBits[2];
						block1->alphaBits[5] = temp.alphaBits[3];
						block1->alphaBits[6] = temp.alphaBits[0];
						block1->alphaBits[7] = temp.alphaBits[1];
					}
				}
			}
			break;
		case DXT5:
			{
				for (int y=0; y<m_Height>>1; y+=4) {
					DXT5_BLOCK *block0 = (DXT5_BLOCK*) (m_Bits + GetStride() * y);
					DXT5_BLOCK *block1 = (DXT5_BLOCK*) (m_Bits + GetStride() * (m_Height - y - 4));
					for (int x=0; x<m_Width; x+=4, ++block0, ++block1) {
						DXT5_BLOCK temp = block0[0];
						block0->a0 = block1->a0;
						block0->a1 = block1->a1;
						for (int i=0; i<4; i++)
							for (int j=0; j<4; j++) {
								block0->SetAlpha(i,j,block1->GetAlpha(i,3-j));
								block1->SetAlpha(i,j,temp.GetAlpha(i,3-j));
							}

						block0->c0 = block1->c0;
						block0->c1 = block1->c1;
						block0->rows[0] = block1->rows[3];
						block0->rows[1] = block1->rows[2];
						block0->rows[2] = block1->rows[1];
						block0->rows[3] = block1->rows[0];
						block1->c0 = temp.c0;
						block1->c1 = temp.c1;
						block1->rows[0] = temp.rows[3];
						block1->rows[1] = temp.rows[2];
						block1->rows[2] = temp.rows[1];
						block1->rows[3] = temp.rows[0];
					}
				}
			}
			break;
		case A8R8G8B8:
		case RGBE:
			{
				Color32 *top = GetColor32();
				Color32 *bottom = top + m_Width * (m_Height-1);
				for (int y=0; y<m_Height>>1; y++, top += m_Width, bottom -= m_Width) {
					for (int x=0; x<m_Width; x++) {
						Color32 tmp = top[x];
						top[x] = bottom[x];
						bottom[x] = tmp;
					}
				}
			}
			break;
		case A4R4G4B4:
			{
				u16 *top = (u16*)m_Bits;
				u16 *bottom = top + m_Width * (m_Height-1);
				for (int y=0; y<m_Height>>1; y++, top += m_Width, bottom -= m_Width) {
					for (int x=0; x<m_Width; x++) {
						u16 tmp = top[x];
						top[x] = bottom[x];
						bottom[x] = tmp;
					}
				}
			}
			break;
		default:
			AssertMsg(0, "not implemented yet");
	}

	if (m_Next)
		m_Next->FlipVertical();

	if (m_NextLayer)
		m_NextLayer->FlipVertical();
}


u32 grcImage::GetLayerCount() const
{
	const grcImage *me = this;
	u32 result = 0;
	while (me) {
		++result;
		me = me->m_NextLayer;
	}
	return result;
}


bool grcImage::sm_RgbeConversion = false;

grcImage* grcImage::MakeChecker(int size,Color32 color1,Color32 color2) {
	grcImage *result = Create(size,size,1,A8R8G8B8,STANDARD,0,0);
	Color32 *p = result->GetColor32();
	for (int y=0; y<size; y++) {
		for (int x = 0; x<size; x++,p++) {
			*p = ((x / 4) ^ (y / 4))&1 ? color2 : color1;
		}
	}
	return result;
}

grcImage* grcImage::Load(const char *filename) {
	grcImage *result = LoadDDS(filename);
	if (!result) {
#if __RESOURCECOMPILER
		grcWarningf("Unable to load image '%s', substituting checker image",filename);
#else
		grcErrorf("Unable to load image '%s', substituting checker image",filename);
#endif
		// Substitute an ugly texture if the asset is missing.
		// ...but make sure it doesn't have a solid red channel, because that
		// can apparently disguise missing bump maps.
		result = MakeChecker(32,Color32(0,255,0),Color32(255,0,255));
	}
	return result;
}


bool grcImage::HasAlpha() const {
	Format format = GetFormat();
	if (format == DXT3 || format == DXT5 || format == A8)
		return true;
	else if (format != A8R8G8B8 && format != A4R4G4B4)
		return false;
	else {
		const Color32 *texels = GetColor32();
		int area = GetArea();
		while (--area)
			if (texels++->GetAlpha() != 255)
				return true;
		return false;
	}
}

grcImage* grcImage::CreateScaledImage(int newWidth,int newHeight , grcImage* srcImage) {
   Assert(srcImage->GetFormat() == A8R8G8B8);
   Assert(newWidth && newHeight);
   Assert(srcImage->GetDepth() == 1);

   grcImage* newImage = grcImage::Create(newWidth,newHeight,srcImage->GetDepth(),srcImage->GetFormat(),srcImage->GetType(),srcImage->GetExtraMipCount(),srcImage->GetLayerCount()-1);

   unsigned sY = 0;
   const int SHIFT = 15;	// can't use 16 because of overflow
   unsigned sdX = (((srcImage->GetWidth()-1) << SHIFT) / (newWidth-1)) - 1;
   unsigned sdY = (((srcImage->GetHeight()-1) << SHIFT) / (newHeight-1)) - 1;

   unsigned char *destBits = newImage->GetBits();
   unsigned char *srcBits = srcImage->GetBits();
   unsigned int srcStride = srcImage->GetStride();

   while (newHeight--) {
      // compute base row to sample
      unsigned row = sY >> SHIFT;
      unsigned char *row1 = srcBits + srcStride * row;
      unsigned char *row2 = row1 + srcStride;

      // compute bilerp for the row.
      unsigned rowfrac = sY & ((1<<SHIFT)-1);
      unsigned omrowfrac = (1<<SHIFT) - rowfrac;

      // Filter the row
      unsigned w = newWidth;
      unsigned sX = 0;
      while (w--) {
         unsigned col = sX >> SHIFT;
         unsigned colfrac = sX & ((1<<SHIFT)-1);
         unsigned omcolfrac = (1<<SHIFT) - colfrac;

         //                     (colfrac,rowfrac)
         //             p3 +------+ p2    <-- rows[2]
         //                |      |
         //                |      |
         //                |      |
         //             p0 +------+ p1    <-- rows[1]
         //(omcolfrac,omrowfrac)

         // Compute fixed-point weights (percentages) of each corner.
         unsigned p0 = (omcolfrac * omrowfrac) >> SHIFT;
         unsigned p1 = (colfrac * omrowfrac) >> SHIFT;
         unsigned p2 = (colfrac * rowfrac) >> SHIFT;
         unsigned p3 = (omcolfrac * rowfrac) >> SHIFT;

         unsigned red = (p0 * row1[col*4 + 0] + p1 * row1[col*4 + 4] + p2 * row2[col*4 + 4] + p3 * row2[col*4 + 0]) >> SHIFT;
         unsigned green = (p0 * row1[col*4 + 1] + p1 * row1[col*4 + 5] + p2 * row2[col*4 + 5] + p3 * row2[col*4 + 1]) >> SHIFT;
         unsigned blue = (p0 * row1[col*4 + 2] + p1 * row1[col*4 + 6] + p2 * row2[col*4 + 6] + p3 * row2[col*4 + 2]) >> SHIFT;
         unsigned alpha = (p0 * row1[col*4 + 3] + p1 * row1[col*4 + 7] + p2 * row2[col*4 + 7] + p3 * row2[col*4 + 3]) >> SHIFT;

         destBits[0] = (u8) red;
         destBits[1] = (u8) green;
         destBits[2] = (u8) blue;
         destBits[3] = (u8) alpha;
         destBits += 4;
         sX += sdX;
      }
      // compute new source row
      sY += sdY;
   }  

   return newImage;
}

void grcImage::Scale(int newWidth,int newHeight) {
	Assert(GetFormat() == A8R8G8B8);
	Assert(newWidth && newHeight);
	Assert(m_Depth == 1);

	unsigned sY = 0;
	const int SHIFT = 15;	// can't use 16 because of overflow
	unsigned sdX = (((m_Width-1) << SHIFT) / (newWidth-1)) - 1;
	unsigned sdY = (((m_Height-1) << SHIFT) / (newHeight-1)) - 1;

	unsigned newStride = (newWidth * 4);

	unsigned char *newBits = rage_new u8[newHeight * newStride * m_Depth];
	unsigned char *dest = newBits;

	m_Width = (u16) newWidth;
	m_Height = (u16) newHeight;

	while (newHeight--) {
		// compute base row to sample
		unsigned row = sY >> SHIFT;
		unsigned char *row1 = m_Bits + GetStride() * row;
		unsigned char *row2 = row1 + GetStride();

		// compute bilerp for the row.
		unsigned rowfrac = sY & ((1<<SHIFT)-1);
		unsigned omrowfrac = (1<<SHIFT) - rowfrac;

		// Filter the row
		unsigned w = newWidth;
		unsigned sX = 0;
		while (w--) {
			unsigned col = sX >> SHIFT;
			unsigned colfrac = sX & ((1<<SHIFT)-1);
			unsigned omcolfrac = (1<<SHIFT) - colfrac;

			//                     (colfrac,rowfrac)
			//             p3 +------+ p2    <-- rows[2]
			//                |      |
			//                |      |
			//                |      |
			//             p0 +------+ p1    <-- rows[1]
			//(omcolfrac,omrowfrac)

			// Compute fixed-point weights (percentages) of each corner.
			unsigned p0 = (omcolfrac * omrowfrac) >> SHIFT;
			unsigned p1 = (colfrac * omrowfrac) >> SHIFT;
			unsigned p2 = (colfrac * rowfrac) >> SHIFT;
			unsigned p3 = (omcolfrac * rowfrac) >> SHIFT;

			unsigned red = (p0 * row1[col*4 + 0] + p1 * row1[col*4 + 4] + p2 * row2[col*4 + 4] + p3 * row2[col*4 + 0]) >> SHIFT;
			unsigned green = (p0 * row1[col*4 + 1] + p1 * row1[col*4 + 5] + p2 * row2[col*4 + 5] + p3 * row2[col*4 + 1]) >> SHIFT;
			unsigned blue = (p0 * row1[col*4 + 2] + p1 * row1[col*4 + 6] + p2 * row2[col*4 + 6] + p3 * row2[col*4 + 2]) >> SHIFT;
			unsigned alpha = (p0 * row1[col*4 + 3] + p1 * row1[col*4 + 7] + p2 * row2[col*4 + 7] + p3 * row2[col*4 + 3]) >> SHIFT;

			dest[0] = (u8) red;
			dest[1] = (u8) green;
			dest[2] = (u8) blue;
			dest[3] = (u8) alpha;
			dest += 4;
			sX += sdX;
		}
		// compute new source row
		sY += sdY;
	}

	delete [] m_Bits;

	m_Bits = newBits;
	Assign(m_Stride, newStride);
	m_StrideHi = 0;
	if (m_Next)
		m_Next->Scale(newWidth>>1,newHeight>>1);
	if (m_NextLayer)
		m_NextLayer->Scale(newWidth,newHeight);
}

void grcImage::RecalculateStride() {
	const u32 stride = (GetFormatBitsPerPixel(GetFormat())*GetPhysicalWidth())/8;
	Assign(m_Stride, stride & 0xFFFF);
	Assign(m_StrideHi, stride>>16);
}

void grcImage::GetDDSInfoFromFile(const char* pFilename, grcImage::Format& format, u32& width, u32& height, u32& numMips, u32& pixelDataOffset, u32& pixelDataSize) {

	// Open file
	fiSafeStream S = ASSET.Open(pFilename, "dds", true, true);
	if (!S) 
	{
		ASSET.FindPath(pFilename, "dds", true);
		format = grcImage::UNKNOWN;
		numMips = 0U;
		return;
	}

	// Double check it's a DDS
	s32 magic;
	S->ReadInt(&magic, 1);

	if (magic != MAKE_MAGIC_NUMBER('D','D','S',' '))
	{
		format = grcImage::UNKNOWN;
		numMips = 0U;
		return;
	}

	// Verify the header is valid
	DDSURFACEDESC2 header;
	if (S->ReadInt(&header.dwSize,sizeof(header)/4) != (sizeof(header)/4)) {
		format = grcImage::UNKNOWN;
		numMips = 0U;
		return;
	}

	if (header.dwSize != sizeof(header) || header.ddpfPixelFormat.dwSize != sizeof(DDPIXELFORMAT9))
	{
		format = grcImage::UNKNOWN;
		numMips = 0U;
		return;
	}

	// Write info back
	width = header.dwWidth;
	height = header.dwHeight;
	numMips = (header.dwFlags & DDSD_MIPMAPCOUNT)? header.dwMipMapCount : 1;
	format = GetDDSFormat(header.ddpfPixelFormat);
	pixelDataOffset = (u32)S->Tell();
	pixelDataSize = (u32)S->Size()-pixelDataOffset;
}


void grcImage::ProcessDDSData(grcImage::Format format, void*  pData, u32 dataSize)
{
	switch (PPU_ONLY(IsFormatDXTBlockCompressed(format) ? 1 :) GetFormatByteSwapSize(format)) // matching previous behavior, DO NOT BYTE SWAP if running on PS3 .. this is ridiculous
	{
		case 2: 
		{
			s16* pS16 = static_cast<s16*>(pData);
			u32 shortSize = dataSize/2;
			for (u32 i = 0; i < shortSize; i++)
			{
				sysEndian::NtoL(*(pS16++));
			}
			break;
		}

		case 4: 
		{
			s32* pS32 = static_cast<s32*>(pData);
			u32 intSize = dataSize/4;
			for (u32 i = 0; i < intSize; i++)
			{
				sysEndian::NtoL(*(pS32++));
			}
			break;
		}

	}

}
