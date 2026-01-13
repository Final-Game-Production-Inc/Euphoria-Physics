// ========================================================
// image_dxt.h
// Copyright (c) 2010 Rockstar North.  All Rights Reserved. 
// ========================================================

#ifndef _IMAGE_DXT_H_
#define _IMAGE_DXT_H_

#if __WIN32
	#pragma warning(push)
	#pragma warning(disable: 4201) // nonstandard extension used : nameless struct/union
#endif

#if __XENON
	#define __XENON_DXT_BYTE_SWAP 1 // this is required because xenon specifies DXT textures as GPUENDIAN_8IN16
#else
	#define __XENON_DXT_BYTE_SWAP 0
#endif

namespace rage {
namespace DXT {

class RGB565
{
public:
	__forceinline RGB565();
	__forceinline RGB565(u16 rgb_);
	__forceinline RGB565(u16 r_, u16 g_, u16 b_);

	__forceinline void ByteSwap();

	union
	{
#if __BE
		struct { u16 r:5, g:6, b:5; };
#else
		struct { u16 b:5, g:6, r:5; };
#endif
		struct { u16 rgb; };
	};
};

class RG88_CTX1 
{
public:
	__forceinline RG88_CTX1();
	__forceinline RG88_CTX1(u16 rg_);
	__forceinline RG88_CTX1(u16 r_, u16 g_);

	__forceinline void ByteSwap();

	union
	{
#if __BE
		struct { u16 r:8, g:8; };
#else
		struct { u16 g:8, r:8; };
#endif
		struct { u16 rg; };
	};
};

class ARGB1555
{
public:
	__forceinline ARGB1555();
	__forceinline ARGB1555(u16 argb_);
	__forceinline ARGB1555(u16 r_, u16 g_, u16 b_, u16 a_);

	union
	{
#if __BE
		struct { u16 a:1, r:5, g:5, b:5; };
#else
		struct { u16 b:5, g:5, r:5, a:1; };
#endif
		struct { u16 argb; };
	};
};

class ARGB4444
{
public:
	__forceinline ARGB4444();
	__forceinline ARGB4444(u16 argb_);
	__forceinline ARGB4444(u16 r_, u16 g_, u16 b_, u16 a_);

	union
	{
#if __BE
		struct { u16 a:4, r:4, g:4, b:4; };
#else
		struct { u16 b:4, g:4, r:4, a:4; };
#endif
		struct { u16 argb; };
	};
};

class ARGB2101010
{
public:
	__forceinline ARGB2101010();
	__forceinline ARGB2101010(u32 argb_);
	__forceinline ARGB2101010(u32 r_, u32 g_, u32 b_, u32 a_);

	union
	{
#if __BE
		struct { u32 a:2, r:10, g:10, b:10; };
#else
		struct { u32 b:10, g:10, r:10, a:2; };
#endif
		struct { u32 argb; };
	};
};

class ABGR2101010
{
public:
	__forceinline ABGR2101010();
	__forceinline ABGR2101010(u32 abgr_);
	__forceinline ABGR2101010(u32 r_, u32 g_, u32 b_, u32 a_);

	union
	{
#if __BE
		struct { u32 a:2, b:10, g:10, r:10; };
#else
		struct { u32 r:10, g:10, b:10, a:2; };
#endif
		struct { u32 abgr; };
	};
};

class ABGR8888;
class ARGB8888
{
public:
	__forceinline ARGB8888();
	__forceinline ARGB8888(u32 argb_);
	__forceinline ARGB8888(u32 r_, u32 g_, u32 b_, u32 a_);
	__forceinline ARGB8888(const RGB565& c);
	__forceinline ARGB8888(const RG88_CTX1& c);
	__forceinline ARGB8888(const ARGB1555& c); // for GetPixel support
	__forceinline ARGB8888(const ARGB4444& c); // for GetPixel support
	__forceinline ARGB8888(const ABGR8888& c); // for GetPixel support
	__forceinline ARGB8888 MergeAlpha(int a_) const;

	static __forceinline int Interpolate(int a0, int a1, int scale0, int scale1, int shift, int bias = 0);
	static __forceinline ARGB8888 Interpolate(const ARGB8888& c0, const ARGB8888& c1, int scale0, int scale1, int shift, int bias = 0);

	union
	{
#if __BE
		struct { u32 a:8, r:8, g:8, b:8; };
#else
		struct { u32 b:8, g:8, r:8, a:8; };
#endif
		struct { u32 argb; };
	};
};

class ABGR8888
{
public:
	__forceinline ABGR8888();
	__forceinline ABGR8888(u32 abgr_);
	__forceinline ABGR8888(u32 r_, u32 g_, u32 b_, u32 a_);

	union
	{
#if __BE
		struct { u32 a:8, b:8, g:8, r:8; };
#else
		struct { u32 r:8, g:8, b:8, a:8; };
#endif
		struct { u32 abgr; };
	};
};

// ================================================================================================

__forceinline RGB565::RGB565() {}
__forceinline RGB565::RGB565(u16 rgb_) : rgb(rgb_) {}
__forceinline RGB565::RGB565(u16 r_, u16 g_, u16 b_)
#if __BE
	: r(r_), g(g_), b(b_) {}
#else
	: b(b_), g(g_), r(r_) {}
#endif

__forceinline void RGB565::ByteSwap()
{
	rgb = (rgb >> 8) | (rgb << 8);
}

__forceinline RG88_CTX1::RG88_CTX1() {}
__forceinline RG88_CTX1::RG88_CTX1(u16 rg_) : rg(rg_) {}
__forceinline RG88_CTX1::RG88_CTX1(u16 r_, u16 g_)
#if __BE
	: r(r_), g(g_) {}
#else
	: g(g_), r(r_) {}
#endif

__forceinline void RG88_CTX1::ByteSwap()
{
	rg = (rg >> 8) | (rg << 8);
}

__forceinline ARGB1555::ARGB1555() {}
__forceinline ARGB1555::ARGB1555(u16 argb_) : argb(argb_) {}
__forceinline ARGB1555::ARGB1555(u16 r_, u16 g_, u16 b_, u16 a_)
#if __BE
	: a(a_), r(r_), g(g_), b(b_) {}
#else
	: b(b_), g(g_), r(r_), a(a_) {}
#endif

__forceinline ARGB4444::ARGB4444() {}
__forceinline ARGB4444::ARGB4444(u16 argb_) : argb(argb_) {}
__forceinline ARGB4444::ARGB4444(u16 r_, u16 g_, u16 b_, u16 a_)
#if __BE
	: a(a_), r(r_), g(g_), b(b_) {}
#else
	: b(b_), g(g_), r(r_), a(a_) {}
#endif

__forceinline ARGB2101010::ARGB2101010() {}
__forceinline ARGB2101010::ARGB2101010(u32 argb_) : argb(argb_) {}
__forceinline ARGB2101010::ARGB2101010(u32 r_, u32 g_, u32 b_, u32 a_)
#if __BE
	: a(a_), r(r_), g(g_), b(b_) {}
#else
	: b(b_), g(g_), r(r_), a(a_) {}
#endif

__forceinline ABGR2101010::ABGR2101010() {}
__forceinline ABGR2101010::ABGR2101010(u32 abgr_) : abgr(abgr_) {}
__forceinline ABGR2101010::ABGR2101010(u32 r_, u32 g_, u32 b_, u32 a_)
#if __BE
	: a(a_), b(b_), g(g_), r(r_) {}
#else
	: r(r_), g(g_), b(b_), a(a_) {}
#endif

__forceinline ARGB8888::ARGB8888() {}
__forceinline ARGB8888::ARGB8888(u32 argb_) : argb(argb_) {}
__forceinline ARGB8888::ARGB8888(u32 r_, u32 g_, u32 b_, u32 a_)
#if __BE
	: a(a_), r(r_), g(g_), b(b_) {}
#else
	: b(b_), g(g_), r(r_), a(a_) {}
#endif

__forceinline ARGB8888::ARGB8888(const RGB565& c)
{
	b = (c.b << 3) | (c.b >> 2);
	g = (c.g << 2) | (c.g >> 4);
	r = (c.r << 3) | (c.r >> 2);
	a = 255;
}

__forceinline ARGB8888::ARGB8888(const RG88_CTX1& c)
{
	b = 0;
	g = c.g;
	r = c.r;
	a = 255;
}

__forceinline ARGB8888::ARGB8888(const ARGB1555& c)
{
	b = (c.b << 3) | (c.b >> 2);
	g = (c.g << 3) | (c.g >> 2);
	r = (c.r << 3) | (c.r >> 2);
	a = (c.a ? 255 : 0);
}

__forceinline ARGB8888::ARGB8888(const ARGB4444& c)
{
	b = (c.b << 4) | (c.b >> 4);
	g = (c.g << 4) | (c.g >> 4);
	r = (c.r << 4) | (c.r >> 4);
	a = (c.a << 4) | (c.a >> 4);
}

__forceinline ARGB8888::ARGB8888(const ABGR8888& c)
{
	b = c.b;
	g = c.g;
	r = c.r;
	a = c.a;
}

__forceinline ARGB8888 ARGB8888::MergeAlpha(int a_) const
{
	return ARGB8888(r, g, b, a_);
}

__forceinline int ARGB8888::Interpolate(int a0, int a1, int scale0, int scale1, int shift, int bias)
{
	return (a0*scale0 + a1*scale1 + bias)>>shift;
}

__forceinline ARGB8888 ARGB8888::Interpolate(const ARGB8888& c0, const ARGB8888& c1, int scale0, int scale1, int shift, int bias)
{
	return ARGB8888
	(
		Interpolate(c0.r, c1.r, scale0, scale1, shift, bias),
		Interpolate(c0.g, c1.g, scale0, scale1, shift, bias),
		Interpolate(c0.b, c1.b, scale0, scale1, shift, bias),
		Interpolate(c0.a, c1.a, scale0, scale1, shift, bias)
	);
}

__forceinline ABGR8888::ABGR8888() {}
__forceinline ABGR8888::ABGR8888(u32 abgr_) : abgr(abgr_) {}
__forceinline ABGR8888::ABGR8888(u32 r_, u32 g_, u32 b_, u32 a_)
#if __BE
	: a(a_), b(b_), g(g_), r(r_) {}
#else
	: r(r_), g(g_), b(b_), a(a_) {}
#endif

// ================================================================================================

class DXT1_BLOCK
{
public:
	__forceinline void SetColour(const ARGB8888& colour); // not that accurate, quantises colour to 565
	__forceinline void GetIndices(int indices[4*4]) const;
	__forceinline void SetIndices(const int indices[4*4]);
	__forceinline void CreateLUT(ARGB8888 lut[4]) const;
	__forceinline void Decompress(void* dst, int dstBytesPerRow = 4*sizeof(ARGB8888)) const;

	__forceinline void InvertIndices();
	__forceinline void InvertRGB();
	__forceinline void TintRGB(int r, int g, int b, float amount);

	RGB565 c[2];
	u8     i[4];
};

class CTX1_BLOCK
{
public:
	__forceinline void GetIndices(int indices[4*4]) const;
	__forceinline void SetIndices(const int indices[4*4]);
	__forceinline void CreateLUT(ARGB8888 lut[4]) const;
	__forceinline void Decompress(void* dst, int dstBytesPerRow = 4*sizeof(ARGB8888)) const;

	static __forceinline CTX1_BLOCK ConvertFromDXT1(const DXT1_BLOCK& dxt1); // experimental conversion

	RG88_CTX1 c[2];
	u8   i[4];
};

class DXT3_ALPHA
{
public:
	__forceinline void GetIndices(int indices[4*4]) const;
	__forceinline void SetIndices(const int indices[4*4]);
	__forceinline void Decompress(void* dst, int dstBytesPerRow = 4*sizeof(ARGB8888)) const;

	static __forceinline const u8* GetLUT();

	u8 i[8];
};

class DXT3_BLOCK
{
public:
	__forceinline void Decompress(void* dst, int dstBytesPerRow = 4*sizeof(ARGB8888)) const;

	DXT3_ALPHA m_alpha;
	DXT1_BLOCK m_colour;
};

class DXT5_ALPHA
{
public:
	__forceinline void SetAlpha(u8 alpha);
	__forceinline void GetIndices(int indices[4*4]) const;
	__forceinline void SetIndices(const int indices[4*4]);
	__forceinline void CreateLUT(u8 lut[8]) const;
	__forceinline void CreateLUT(ARGB8888 lut[8]) const
	{
		u8 lut8[8];
		CreateLUT(lut8);
		for (int i = 0; i < 8; i++)
		{
			lut[i].r = lut8[i];
			lut[i].g = lut8[i];
			lut[i].b = lut8[i];
			lut[i].a = 255;
		}
	}
	__forceinline void Decompress(void* dst, int dstBytesPerRow = 4*sizeof(ARGB8888)) const;

	u8 a[2];
	u8 i[6];
};

class DXT5_BLOCK
{
public:
	__forceinline void SetColour(const ARGB8888& colour);
	__forceinline void Decompress(void* dst, int dstBytesPerRow = 4*sizeof(ARGB8888)) const;

	DXT5_ALPHA m_alpha;
	DXT1_BLOCK m_colour;
};

class DXN_BLOCK
{
public:
	__forceinline void Decompress(void* dst, int dstBytesPerRow = 4*sizeof(ARGB8888)) const;

	DXT5_ALPHA m_x;
	DXT5_ALPHA m_y;
};

// ================================================================================================

__forceinline void DXT1_BLOCK::SetColour(const ARGB8888& colour) // not that accurate, quantises colour to 565
{
	c[0].r = colour.r >> 3;
	c[0].g = colour.g >> 2;
	c[0].b = colour.b >> 3;

	PPU_ONLY(c[0].ByteSwap());

	c[1].rgb = c[0].rgb;

	i[0] = (colour.argb == 0) ? 0xff : 0x00;
	i[1] = i[0];
	i[2] = i[0];
	i[3] = i[0];
} 

__forceinline void DXT1_BLOCK::GetIndices(int indices[4*4]) const
{
	indices[0x00] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 0);
	indices[0x01] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 2);
	indices[0x02] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 4);
	indices[0x03] =   (i[__XENON_DXT_BYTE_SWAP ^ 0] >> 6);
	indices[0x04] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 0);
	indices[0x05] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 2);
	indices[0x06] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 4);
	indices[0x07] =   (i[__XENON_DXT_BYTE_SWAP ^ 1] >> 6);
	indices[0x08] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 0);
	indices[0x09] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 2);
	indices[0x0a] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 4);
	indices[0x0b] =   (i[__XENON_DXT_BYTE_SWAP ^ 2] >> 6);
	indices[0x0c] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 0);
	indices[0x0d] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 2);
	indices[0x0e] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 4);
	indices[0x0f] =   (i[__XENON_DXT_BYTE_SWAP ^ 3] >> 6);
}

__forceinline void DXT1_BLOCK::SetIndices(const int indices[4*4])
{
	i[__XENON_DXT_BYTE_SWAP ^ 0] = (u8)(indices[0x00] | (indices[0x01] << 2) | (indices[0x02] << 4) | (indices[0x03] << 6));
	i[__XENON_DXT_BYTE_SWAP ^ 1] = (u8)(indices[0x04] | (indices[0x05] << 2) | (indices[0x06] << 4) | (indices[0x07] << 6));
	i[__XENON_DXT_BYTE_SWAP ^ 2] = (u8)(indices[0x08] | (indices[0x09] << 2) | (indices[0x0a] << 4) | (indices[0x0b] << 6));
	i[__XENON_DXT_BYTE_SWAP ^ 3] = (u8)(indices[0x0c] | (indices[0x0d] << 2) | (indices[0x0e] << 4) | (indices[0x0f] << 6));
}

// http://www.ludicon.com/castano/blog/2009/03/gpu-dxt-decompression/
// http://www.freepatentsonline.com/7385611.html
__forceinline void evaluatePaletteNV5x(RGB565 col0, RGB565 col1, ARGB8888 palette[4])
{
	palette[0].r = (3 * col0.r * 22) / 8; // same as (col0.r << 3) | (col0.r >> 2)
	palette[0].g = (col0.g << 2) | (col0.g >> 4);
	palette[0].b = (3 * col0.b * 22) / 8;
	palette[0].a = 0xFF;

	palette[1].r = (3 * col1.r * 22) / 8;
	palette[1].g = (col1.g << 2) | (col1.g >> 4);
	palette[1].b = (3 * col1.b * 22) / 8;
	palette[1].a = 0xFF;

	int gdiff = palette[1].g - palette[0].g;

	if (col0.rgb > col1.rgb)
	{
		palette[2].r = ((2 * col0.r + col1.r) * 22) / 8;
		palette[2].g = (256 * palette[0].g + gdiff/4 + 128 + gdiff * 80) / 256;
		palette[2].b = ((2 * col0.b + col1.b) * 22) / 8;
		palette[2].a = 0xFF;

		palette[3].r = ((2 * col1.r + col0.r) * 22) / 8;
		palette[3].g = (256 * palette[1].g - gdiff/4 + 128 - gdiff * 80) / 256;
		palette[3].b = ((2 * col1.b + col0.b) * 22) / 8;
		palette[3].a = 0xFF;
	}
	else
	{
		palette[2].r = ((col0.r + col1.r) * 33) / 8;
		palette[2].g = (256 * palette[0].g + gdiff/4 + 128 + gdiff * 128) / 256;
		palette[2].b = ((col0.b + col1.b) * 33) / 8;
		palette[2].a = 0xFF;

		palette[3].r = 0x00;
		palette[3].g = 0x00;
		palette[3].b = 0x00;
		palette[3].a = 0x00;
	}
}

// reverse-engineered from analysing nvidia shader output (NVIDIA GeForce GTS 450), BC4/5 format writing to u16 render target
__forceinline void evaluatePaletteNV5x_BC45_u16(u8 a0, u8 a1, u16 palette[8])
{
	palette[0] = 0x101*(u16)a0;
	palette[1] = 0x101*(u16)a1;

	if (a0 > a1)
	{
		palette[2] = 0xDD*(u16)a0 + 0x24*(u16)a1;
		palette[3] = 0xB9*(u16)a0 + 0x48*(u16)a1;
		palette[4] = 0x90*(u16)a0 + 0x71*(u16)a1;
		palette[5] = 0x71*(u16)a0 + 0x90*(u16)a1;
		palette[6] = 0x48*(u16)a0 + 0xB9*(u16)a1;
		palette[7] = 0x24*(u16)a0 + 0xDD*(u16)a1;
	}
	else
	{
		palette[2] = 0xD1*(u16)a0 + 0x30*(u16)a1;
		palette[3] = 0xA1*(u16)a0 + 0x60*(u16)a1;
		palette[4] = 0x60*(u16)a0 + 0xA1*(u16)a1;
		palette[5] = 0x30*(u16)a0 + 0xD1*(u16)a1;
		palette[6] = 0x0000;
		palette[7] = 0xFFFF;
	}
}

// reverse-engineered from analysing durango shader output
__forceinline void evaluatePaletteAMD_DXT1(RGB565 col0, RGB565 col1, ARGB8888 palette[4])
{
	palette[0].r = (col0.r << 3) | (col0.r >> 2);
	palette[0].g = (col0.g << 2) | (col0.g >> 4);
	palette[0].b = (col0.b << 3) | (col0.b >> 2);
	palette[0].a = 0xFF;

	palette[1].r = (col1.r << 3) | (col1.r >> 2);
	palette[1].g = (col1.g << 2) | (col1.g >> 4);
	palette[1].b = (col1.b << 3) | (col1.b >> 2);
	palette[1].a = 0xFF;

	if (col0.rgb > col1.rgb)
	{
		palette[2].r = (43*palette[0].r + 21*palette[1].r + 32)/64;
		palette[2].g = (43*palette[0].g + 21*palette[1].g + 32)/64;
		palette[2].b = (43*palette[0].b + 21*palette[1].b + 32)/64;
		palette[2].a = 0xFF;

		palette[3].r = (21*palette[0].r + 43*palette[1].r + 32)/64;
		palette[3].g = (21*palette[0].g + 43*palette[1].g + 32)/64;
		palette[3].b = (21*palette[0].b + 43*palette[1].b + 32)/64;
		palette[3].a = 0xFF;
	}
	else
	{
		palette[2].r = (palette[0].r + palette[1].r + 1)/2;
		palette[2].g = (palette[0].g + palette[1].g + 1)/2;
		palette[2].b = (palette[0].b + palette[1].b + 1)/2;
		palette[2].a = 0xFF;

		palette[3].r = 0x00;
		palette[3].g = 0x00;
		palette[3].b = 0x00;
		palette[3].a = 0x00;
	}
}

// reverse-engineered from analysing durango shader output (note: this is not the same as BC4/BC5 decompression)
__forceinline void evaluatePaletteAMD_DXT5_ALPHA(u8 a0, u8 a1, u8 palette[8])
{
	palette[0] = a0;
	palette[1] = a1;

	if (a0 > a1)
	{
		palette[2] = (u8)((0x37*(u16)a0 + 0x09*(u16)a1 + 32)/64);
		palette[3] = (u8)((0x2E*(u16)a0 + 0x12*(u16)a1 + 32)/64);
		palette[4] = (u8)((0x25*(u16)a0 + 0x1B*(u16)a1 + 32)/64);
		palette[5] = (u8)((0x1B*(u16)a0 + 0x25*(u16)a1 + 32)/64);
		palette[6] = (u8)((0x12*(u16)a0 + 0x2E*(u16)a1 + 32)/64);
		palette[7] = (u8)((0x09*(u16)a0 + 0x37*(u16)a1 + 32)/64);
	}
	else
	{
		palette[2] = (u8)((0x33*(u16)a0 + 0x0D*(u16)a1 + 32)/64);
		palette[3] = (u8)((0x26*(u16)a0 + 0x1A*(u16)a1 + 32)/64);
		palette[4] = (u8)((0x1A*(u16)a0 + 0x26*(u16)a1 + 32)/64);
		palette[5] = (u8)((0x0D*(u16)a0 + 0x33*(u16)a1 + 32)/64);
		palette[6] = 0x00;
		palette[7] = 0xFF;
	}
}

// reverse-engineered from analysing durango shader output, BC4/5 format writing to u16 render target
__forceinline void evaluatePaletteAMD_BC45_u16(u8 a0, u8 a1, u16 palette[8])
{
	// TODO
	(void)a0;
	(void)a1;
	(void)palette;
}

__forceinline void DXT1_BLOCK::CreateLUT(ARGB8888 lut[4]) const
{
	RGB565 c0_temp = c[0];
	RGB565 c1_temp = c[1];

	PPU_ONLY(c0_temp.ByteSwap());
	PPU_ONLY(c1_temp.ByteSwap());

#if 0
	evaluatePaletteNV5x(c[0], c[1], lut);
#else
	lut[0] = c0_temp;
	lut[1] = c1_temp;

	if (c0_temp.rgb > c1_temp.rgb)
	{
		lut[2] = ARGB8888::Interpolate(lut[0], lut[1], 255 - (256*1)/3, (256*1)/3, 8, 0);
		lut[3] = ARGB8888::Interpolate(lut[0], lut[1], 255 - (256*2)/3, (256*2)/3, 8, 0);
	}
	else
	{
		lut[2] = ARGB8888::Interpolate(lut[0], lut[1], 1, 1, 1, 1);
		lut[3] = ARGB8888(0);
	}
#endif
}

__forceinline void DXT1_BLOCK::Decompress(void* dst, int dstBytesPerRow) const
{
	ARGB8888 lut[4];

	CreateLUT(lut);

	ARGB8888* row0 = (ARGB8888*)((u8*)dst + dstBytesPerRow*0);
	ARGB8888* row1 = (ARGB8888*)((u8*)dst + dstBytesPerRow*1);
	ARGB8888* row2 = (ARGB8888*)((u8*)dst + dstBytesPerRow*2);
	ARGB8888* row3 = (ARGB8888*)((u8*)dst + dstBytesPerRow*3);

	row0[0] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 0)];
	row0[1] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 2)];
	row0[2] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 4)];
	row0[3] = lut[  (i[__XENON_DXT_BYTE_SWAP ^ 0] >> 6)];
	row1[0] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 0)];
	row1[1] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 2)];
	row1[2] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 4)];
	row1[3] = lut[  (i[__XENON_DXT_BYTE_SWAP ^ 1] >> 6)];
	row2[0] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 0)];
	row2[1] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 2)];
	row2[2] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 4)];
	row2[3] = lut[  (i[__XENON_DXT_BYTE_SWAP ^ 2] >> 6)];
	row3[0] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 0)];
	row3[1] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 2)];
	row3[2] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 4)];
	row3[3] = lut[  (i[__XENON_DXT_BYTE_SWAP ^ 3] >> 6)];
}

__forceinline void DXT1_BLOCK::InvertIndices()
{
	u32& indices = *reinterpret_cast<u32*>(&i[0]);

	if (c[0].rgb > c[1].rgb) // no alpha
	{
		indices ^= 0x55555555; // {0,1,2,3} -> {1,0,3,2}
	}
	else
	{
		indices ^= (0x55555555 & ~(indices>>1)); // {0,1,2,3} -> {1,0,2,3}
	}
}

__forceinline void DXT1_BLOCK::InvertRGB()
{
	const u16 c0_temp = c[0].rgb ^ 0xffff;
	const u16 c1_temp = c[1].rgb ^ 0xffff;

	c[0].rgb = c1_temp;
	c[1].rgb = c0_temp;

	InvertIndices();
}

__forceinline void DXT1_BLOCK::TintRGB(int r, int g, int b, float amount)
{
	if (amount >= 1.0f)
	{
		SetColour(ARGB8888(r, g, b, 0));
	}
	else if (amount > 0.0f)
	{
		const RGB565 c0_temp = RGB565(
			(u16)(((float)c[0].r + ((float)(r >> 3) - (float)c[0].r)*amount) + 0.5f),
			(u16)(((float)c[0].g + ((float)(g >> 2) - (float)c[0].g)*amount) + 0.5f),
			(u16)(((float)c[0].b + ((float)(b >> 3) - (float)c[0].b)*amount) + 0.5f)
		);
		const RGB565 c1_temp = RGB565(
			(u16)(((float)c[1].r + ((float)(r >> 3) - (float)c[1].r)*amount) + 0.5f),
			(u16)(((float)c[1].g + ((float)(g >> 2) - (float)c[1].g)*amount) + 0.5f),
			(u16)(((float)c[1].b + ((float)(b >> 3) - (float)c[1].b)*amount) + 0.5f)
		);

		if ((c[0].rgb > c[1].rgb) == (c0_temp.rgb > c1_temp.rgb))
		{
			c[0] = c0_temp;
			c[1] = c1_temp;
		}
		else
		{
			c[0] = c1_temp;
			c[1] = c0_temp;

			InvertIndices();
		}
	}
}

__forceinline void CTX1_BLOCK::GetIndices(int indices[4*4]) const
{
	indices[0x00] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 0);
	indices[0x01] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 2);
	indices[0x02] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 4);
	indices[0x03] =   (i[__XENON_DXT_BYTE_SWAP ^ 0] >> 6);
	indices[0x04] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 0);
	indices[0x05] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 2);
	indices[0x06] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 4);
	indices[0x07] =   (i[__XENON_DXT_BYTE_SWAP ^ 1] >> 6);
	indices[0x08] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 0);
	indices[0x09] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 2);
	indices[0x0a] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 4);
	indices[0x0b] =   (i[__XENON_DXT_BYTE_SWAP ^ 2] >> 6);
	indices[0x0c] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 0);
	indices[0x0d] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 2);
	indices[0x0e] = 3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 4);
	indices[0x0f] =   (i[__XENON_DXT_BYTE_SWAP ^ 3] >> 6);
}

__forceinline void CTX1_BLOCK::SetIndices(const int indices[4*4])
{
	i[__XENON_DXT_BYTE_SWAP ^ 0] = (u8)(indices[0x00] | (indices[0x01] << 2) | (indices[0x02] << 4) | (indices[0x03] << 6));
	i[__XENON_DXT_BYTE_SWAP ^ 1] = (u8)(indices[0x04] | (indices[0x05] << 2) | (indices[0x06] << 4) | (indices[0x07] << 6));
	i[__XENON_DXT_BYTE_SWAP ^ 2] = (u8)(indices[0x08] | (indices[0x09] << 2) | (indices[0x0a] << 4) | (indices[0x0b] << 6));
	i[__XENON_DXT_BYTE_SWAP ^ 3] = (u8)(indices[0x0c] | (indices[0x0d] << 2) | (indices[0x0e] << 4) | (indices[0x0f] << 6));
}

__forceinline void CTX1_BLOCK::CreateLUT(ARGB8888 lut[4]) const
{
	RG88_CTX1 c0_temp = c[0];
	RG88_CTX1 c1_temp = c[1];
	
	PPU_ONLY(c0_temp.ByteSwap());
	PPU_ONLY(c1_temp.ByteSwap());

	lut[0] = c0_temp;
	lut[1] = c1_temp;

	if (c0_temp.rg > c1_temp.rg)
	{
		lut[2] = ARGB8888::Interpolate(lut[0], lut[1], 255 - (256*1)/3, (256*1)/3, 8, 0);
		lut[3] = ARGB8888::Interpolate(lut[0], lut[1], 255 - (256*2)/3, (256*2)/3, 8, 0);
	}
	else
	{
		lut[2] = ARGB8888::Interpolate(lut[0], lut[1], 1, 1, 1, 1);
		lut[3] = ARGB8888(0);
	}
}

__forceinline void CTX1_BLOCK::Decompress(void* dst, int dstBytesPerRow) const
{
	ARGB8888 lut[4];

	CreateLUT(lut);

	ARGB8888* row0 = (ARGB8888*)((u8*)dst + dstBytesPerRow*0);
	ARGB8888* row1 = (ARGB8888*)((u8*)dst + dstBytesPerRow*1);
	ARGB8888* row2 = (ARGB8888*)((u8*)dst + dstBytesPerRow*2);
	ARGB8888* row3 = (ARGB8888*)((u8*)dst + dstBytesPerRow*3);

	row0[0] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 0)];
	row0[1] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 2)];
	row0[2] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 4)];
	row0[3] = lut[  (i[__XENON_DXT_BYTE_SWAP ^ 0] >> 6)];
	row1[0] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 0)];
	row1[1] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 2)];
	row1[2] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 4)];
	row1[3] = lut[  (i[__XENON_DXT_BYTE_SWAP ^ 1] >> 6)];
	row2[0] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 0)];
	row2[1] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 2)];
	row2[2] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 4)];
	row2[3] = lut[  (i[__XENON_DXT_BYTE_SWAP ^ 2] >> 6)];
	row3[0] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 0)];
	row3[1] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 2)];
	row3[2] = lut[3&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 4)];
	row3[3] = lut[  (i[__XENON_DXT_BYTE_SWAP ^ 3] >> 6)];
}

__forceinline CTX1_BLOCK CTX1_BLOCK::ConvertFromDXT1(const DXT1_BLOCK& dxt1) // experimental conversion
{
	DXT1_BLOCK temp = dxt1;
	CTX1_BLOCK block;

	temp.c[0].ByteSwap();
	temp.c[1].ByteSwap();

	block.c[0].r = (temp.c[0].r << 3) | (temp.c[0].r >> 2);
	block.c[0].g = (temp.c[0].g << 2) | (temp.c[0].g >> 4);
	block.c[1].r = (temp.c[1].r << 3) | (temp.c[1].r >> 2);
	block.c[1].g = (temp.c[1].g << 2) | (temp.c[1].g >> 4);
	block.i[0] = dxt1.i[0];
	block.i[1] = dxt1.i[1];
	block.i[2] = dxt1.i[2];
	block.i[3] = dxt1.i[3];

	block.c[0].ByteSwap();
	block.c[1].ByteSwap();

	return block;
}

__forceinline void DXT3_ALPHA::GetIndices(int indices[4*4]) const
{
	indices[0x00] = 15&(i[__XENON_DXT_BYTE_SWAP ^ 0] >> 0);
	indices[0x01] =    (i[__XENON_DXT_BYTE_SWAP ^ 0] >> 4);
	indices[0x02] = 15&(i[__XENON_DXT_BYTE_SWAP ^ 1] >> 0);
	indices[0x03] =    (i[__XENON_DXT_BYTE_SWAP ^ 1] >> 4);
	indices[0x04] = 15&(i[__XENON_DXT_BYTE_SWAP ^ 2] >> 0);
	indices[0x05] =    (i[__XENON_DXT_BYTE_SWAP ^ 2] >> 4);
	indices[0x06] = 15&(i[__XENON_DXT_BYTE_SWAP ^ 3] >> 0);
	indices[0x07] =    (i[__XENON_DXT_BYTE_SWAP ^ 3] >> 4);
	indices[0x08] = 15&(i[__XENON_DXT_BYTE_SWAP ^ 4] >> 0);
	indices[0x09] =    (i[__XENON_DXT_BYTE_SWAP ^ 4] >> 4);
	indices[0x0a] = 15&(i[__XENON_DXT_BYTE_SWAP ^ 5] >> 0);
	indices[0x0b] =    (i[__XENON_DXT_BYTE_SWAP ^ 5] >> 4);
	indices[0x0c] = 15&(i[__XENON_DXT_BYTE_SWAP ^ 6] >> 0);
	indices[0x0d] =    (i[__XENON_DXT_BYTE_SWAP ^ 6] >> 4);
	indices[0x0e] = 15&(i[__XENON_DXT_BYTE_SWAP ^ 7] >> 0);
	indices[0x0f] =    (i[__XENON_DXT_BYTE_SWAP ^ 7] >> 4);
}

__forceinline void DXT3_ALPHA::SetIndices(const int indices[4*4])
{
	i[__XENON_DXT_BYTE_SWAP ^ 0] = (u8)(indices[0x00] | (indices[0x01] << 4));
	i[__XENON_DXT_BYTE_SWAP ^ 1] = (u8)(indices[0x02] | (indices[0x03] << 4));
	i[__XENON_DXT_BYTE_SWAP ^ 2] = (u8)(indices[0x04] | (indices[0x05] << 4));
	i[__XENON_DXT_BYTE_SWAP ^ 3] = (u8)(indices[0x06] | (indices[0x07] << 4));
	i[__XENON_DXT_BYTE_SWAP ^ 4] = (u8)(indices[0x08] | (indices[0x09] << 4));
	i[__XENON_DXT_BYTE_SWAP ^ 5] = (u8)(indices[0x0a] | (indices[0x0b] << 4));
	i[__XENON_DXT_BYTE_SWAP ^ 6] = (u8)(indices[0x0c] | (indices[0x0d] << 4));
	i[__XENON_DXT_BYTE_SWAP ^ 7] = (u8)(indices[0x0e] | (indices[0x0f] << 4));
}

__forceinline void DXT3_ALPHA::Decompress(void* dst, int dstBytesPerRow) const
{
	const u8* lut = GetLUT();
	int indices[4*4];

	GetIndices(indices);

	ARGB8888* row0 = (ARGB8888*)((u8*)dst + dstBytesPerRow*0);
	ARGB8888* row1 = (ARGB8888*)((u8*)dst + dstBytesPerRow*1);
	ARGB8888* row2 = (ARGB8888*)((u8*)dst + dstBytesPerRow*2);
	ARGB8888* row3 = (ARGB8888*)((u8*)dst + dstBytesPerRow*3);

	row0[0] = ARGB8888(0,0,0,lut[indices[0x00]]);
	row0[1] = ARGB8888(0,0,0,lut[indices[0x01]]);
	row0[2] = ARGB8888(0,0,0,lut[indices[0x02]]);
	row0[3] = ARGB8888(0,0,0,lut[indices[0x03]]);
	row1[0] = ARGB8888(0,0,0,lut[indices[0x04]]);
	row1[1] = ARGB8888(0,0,0,lut[indices[0x05]]);
	row1[2] = ARGB8888(0,0,0,lut[indices[0x06]]);
	row1[3] = ARGB8888(0,0,0,lut[indices[0x07]]);
	row2[0] = ARGB8888(0,0,0,lut[indices[0x08]]);
	row2[1] = ARGB8888(0,0,0,lut[indices[0x09]]);
	row2[2] = ARGB8888(0,0,0,lut[indices[0x0a]]);
	row2[3] = ARGB8888(0,0,0,lut[indices[0x0b]]);
	row3[0] = ARGB8888(0,0,0,lut[indices[0x0c]]);
	row3[1] = ARGB8888(0,0,0,lut[indices[0x0d]]);
	row3[2] = ARGB8888(0,0,0,lut[indices[0x0e]]);
	row3[3] = ARGB8888(0,0,0,lut[indices[0x0f]]);
}

__forceinline const u8* DXT3_ALPHA::GetLUT()
{
	static const u8 lut[4*4] =
	{
		0x00,0x11,0x22,0x33,
		0x44,0x55,0x66,0x77,
		0x88,0x99,0xaa,0xbb,
		0xcc,0xdd,0xee,0xff,
	};

	return lut;
}

__forceinline void DXT3_BLOCK::Decompress(void* dst, int dstBytesPerRow) const
{
	ARGB8888  colour_lut[4];
	const u8* alpha_lut = m_alpha.GetLUT();
	int       alpha_indices[4*4];

	m_colour.CreateLUT (colour_lut);
	m_alpha .GetIndices(alpha_indices);

	ARGB8888* row0 = (ARGB8888*)((u8*)dst + dstBytesPerRow*0);
	ARGB8888* row1 = (ARGB8888*)((u8*)dst + dstBytesPerRow*1);
	ARGB8888* row2 = (ARGB8888*)((u8*)dst + dstBytesPerRow*2);
	ARGB8888* row3 = (ARGB8888*)((u8*)dst + dstBytesPerRow*3);

	row0[0] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 0] >> 0)].MergeAlpha(alpha_lut[alpha_indices[0x00]]);
	row0[1] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 0] >> 2)].MergeAlpha(alpha_lut[alpha_indices[0x01]]);
	row0[2] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 0] >> 4)].MergeAlpha(alpha_lut[alpha_indices[0x02]]);
	row0[3] = colour_lut[  (m_colour.i[__XENON_DXT_BYTE_SWAP ^ 0] >> 6)].MergeAlpha(alpha_lut[alpha_indices[0x03]]);
	row1[0] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 1] >> 0)].MergeAlpha(alpha_lut[alpha_indices[0x04]]);
	row1[1] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 1] >> 2)].MergeAlpha(alpha_lut[alpha_indices[0x05]]);
	row1[2] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 1] >> 4)].MergeAlpha(alpha_lut[alpha_indices[0x06]]);
	row1[3] = colour_lut[  (m_colour.i[__XENON_DXT_BYTE_SWAP ^ 1] >> 6)].MergeAlpha(alpha_lut[alpha_indices[0x07]]);
	row2[0] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 2] >> 0)].MergeAlpha(alpha_lut[alpha_indices[0x08]]);
	row2[1] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 2] >> 2)].MergeAlpha(alpha_lut[alpha_indices[0x09]]);
	row2[2] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 2] >> 4)].MergeAlpha(alpha_lut[alpha_indices[0x0a]]);
	row2[3] = colour_lut[  (m_colour.i[__XENON_DXT_BYTE_SWAP ^ 2] >> 6)].MergeAlpha(alpha_lut[alpha_indices[0x0b]]);
	row3[0] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 3] >> 0)].MergeAlpha(alpha_lut[alpha_indices[0x0c]]);
	row3[1] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 3] >> 2)].MergeAlpha(alpha_lut[alpha_indices[0x0d]]);
	row3[2] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 3] >> 4)].MergeAlpha(alpha_lut[alpha_indices[0x0e]]);
	row3[3] = colour_lut[  (m_colour.i[__XENON_DXT_BYTE_SWAP ^ 3] >> 6)].MergeAlpha(alpha_lut[alpha_indices[0x0f]]);
}

__forceinline void DXT5_ALPHA::SetAlpha(u8 alpha)
{
	a[0] = a[1] = alpha;
	i[0] = i[1] = i[2] = i[3] = i[4] = i[5] = 0;
}

__forceinline void DXT5_ALPHA::GetIndices(int indices[4*4]) const
{
	// 00011122.23334445 - 55666777.888999aa - abbbcccd.ddeeefff
	// xxxxxxxx.xxxxxxxx - xxxxxxxx.xxxxxxxx - xxxxxxxx.xxxxxxxx

	indices[0x00] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 0] >> 0));
	indices[0x01] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 0] >> 3));
	indices[0x02] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 0] >> 6)|(i[__XENON_DXT_BYTE_SWAP ^ 1] << (8-6)));
	indices[0x03] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 1] >> 1));
	indices[0x04] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 1] >> 4));
	indices[0x05] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 1] >> 7)|(i[__XENON_DXT_BYTE_SWAP ^ 2] << (8-7)));
	indices[0x06] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 2] >> 2));
	indices[0x07] =   ((i[__XENON_DXT_BYTE_SWAP ^ 2] >> 5));
	indices[0x08] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 3] >> 0));
	indices[0x09] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 3] >> 3));
	indices[0x0a] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 3] >> 6)|(i[__XENON_DXT_BYTE_SWAP ^ 4] << (8-6)));
	indices[0x0b] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 4] >> 1));
	indices[0x0c] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 4] >> 4));
	indices[0x0d] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 4] >> 7)|(i[__XENON_DXT_BYTE_SWAP ^ 5] << (8-7)));
	indices[0x0e] = 7&((i[__XENON_DXT_BYTE_SWAP ^ 5] >> 2));
	indices[0x0f] =   ((i[__XENON_DXT_BYTE_SWAP ^ 5] >> 5));
}

__forceinline void DXT5_ALPHA::SetIndices(const int indices[4*4])
{
	i[__XENON_DXT_BYTE_SWAP ^ 0] = (u8)((indices[0x00] << 0) | (indices[0x01] << 3) | (indices[0x02] << 6));
	i[__XENON_DXT_BYTE_SWAP ^ 1] = (u8)((indices[0x02] >> 2) | (indices[0x03] << 1) | (indices[0x04] << 4) | (indices[0x05] << 7));
	i[__XENON_DXT_BYTE_SWAP ^ 2] = (u8)((indices[0x05] >> 1) | (indices[0x06] << 2) | (indices[0x07] << 5));
	i[__XENON_DXT_BYTE_SWAP ^ 3] = (u8)((indices[0x08] << 0) | (indices[0x09] << 3) | (indices[0x0a] << 6));
	i[__XENON_DXT_BYTE_SWAP ^ 4] = (u8)((indices[0x0a] >> 2) | (indices[0x0b] << 1) | (indices[0x0c] << 4) | (indices[0x0d] << 7));
	i[__XENON_DXT_BYTE_SWAP ^ 5] = (u8)((indices[0x0d] >> 1) | (indices[0x0e] << 2) | (indices[0x0f] << 5));
}

__forceinline void DXT5_ALPHA::CreateLUT(u8 lut[8]) const
{
	lut[0] = a[__XENON_DXT_BYTE_SWAP ^ 0];
	lut[1] = a[__XENON_DXT_BYTE_SWAP ^ 1];

	if (lut[0] > lut[1])
	{
		lut[2] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*1)/7, (256*1)/7, 8, 0);
		lut[3] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*2)/7, (256*2)/7, 8, 0);
		lut[4] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*3)/7, (256*3)/7, 8, 0);
		lut[5] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*4)/7, (256*4)/7, 8, 0);
		lut[6] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*5)/7, (256*5)/7, 8, 0);
		lut[7] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*6)/7, (256*6)/7, 8, 0);
	}
	else
	{
		lut[2] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*1)/5, (256*1)/5, 8, 0);
		lut[3] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*2)/5, (256*2)/5, 8, 0);
		lut[4] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*3)/5, (256*3)/5, 8, 0);
		lut[5] = (u8)ARGB8888::Interpolate((int)lut[0], (int)lut[1], 255 - (256*4)/5, (256*4)/5, 8, 0);
		lut[6] = 0;
		lut[7] = 255;
	}
}

__forceinline void DXT5_ALPHA::Decompress(void* dst, int dstBytesPerRow) const
{
	u8  lut[8];
	int indices[4*4];

	CreateLUT(lut);
	GetIndices(indices);

	ARGB8888* row0 = (ARGB8888*)((u8*)dst + dstBytesPerRow*0);
	ARGB8888* row1 = (ARGB8888*)((u8*)dst + dstBytesPerRow*1);
	ARGB8888* row2 = (ARGB8888*)((u8*)dst + dstBytesPerRow*2);
	ARGB8888* row3 = (ARGB8888*)((u8*)dst + dstBytesPerRow*3);

	row0[0] = ARGB8888(0,0,0,lut[indices[0x00]]);
	row0[1] = ARGB8888(0,0,0,lut[indices[0x01]]);
	row0[2] = ARGB8888(0,0,0,lut[indices[0x02]]);
	row0[3] = ARGB8888(0,0,0,lut[indices[0x03]]);
	row1[0] = ARGB8888(0,0,0,lut[indices[0x04]]);
	row1[1] = ARGB8888(0,0,0,lut[indices[0x05]]);
	row1[2] = ARGB8888(0,0,0,lut[indices[0x06]]);
	row1[3] = ARGB8888(0,0,0,lut[indices[0x07]]);
	row2[0] = ARGB8888(0,0,0,lut[indices[0x08]]);
	row2[1] = ARGB8888(0,0,0,lut[indices[0x09]]);
	row2[2] = ARGB8888(0,0,0,lut[indices[0x0a]]);
	row2[3] = ARGB8888(0,0,0,lut[indices[0x0b]]);
	row3[0] = ARGB8888(0,0,0,lut[indices[0x0c]]);
	row3[1] = ARGB8888(0,0,0,lut[indices[0x0d]]);
	row3[2] = ARGB8888(0,0,0,lut[indices[0x0e]]);
	row3[3] = ARGB8888(0,0,0,lut[indices[0x0f]]);
}

__forceinline void DXT5_BLOCK::SetColour(const ARGB8888& colour)
{
	m_alpha .SetAlpha (colour.a);
	m_colour.SetColour(colour);
}

__forceinline void DXT5_BLOCK::Decompress(void* dst, int dstBytesPerRow) const
{
	ARGB8888 colour_lut[4];
	u8       alpha_lut[8];
	int      alpha_indices[4*4];

	m_colour.CreateLUT (colour_lut);
	m_alpha .CreateLUT (alpha_lut);
	m_alpha .GetIndices(alpha_indices);

	ARGB8888* row0 = (ARGB8888*)((u8*)dst + dstBytesPerRow*0);
	ARGB8888* row1 = (ARGB8888*)((u8*)dst + dstBytesPerRow*1);
	ARGB8888* row2 = (ARGB8888*)((u8*)dst + dstBytesPerRow*2);
	ARGB8888* row3 = (ARGB8888*)((u8*)dst + dstBytesPerRow*3);

	row0[0] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 0] >> 0)].MergeAlpha(alpha_lut[alpha_indices[0x00]]);
	row0[1] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 0] >> 2)].MergeAlpha(alpha_lut[alpha_indices[0x01]]);
	row0[2] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 0] >> 4)].MergeAlpha(alpha_lut[alpha_indices[0x02]]);
	row0[3] = colour_lut[  (m_colour.i[__XENON_DXT_BYTE_SWAP ^ 0] >> 6)].MergeAlpha(alpha_lut[alpha_indices[0x03]]);
	row1[0] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 1] >> 0)].MergeAlpha(alpha_lut[alpha_indices[0x04]]);
	row1[1] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 1] >> 2)].MergeAlpha(alpha_lut[alpha_indices[0x05]]);
	row1[2] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 1] >> 4)].MergeAlpha(alpha_lut[alpha_indices[0x06]]);
	row1[3] = colour_lut[  (m_colour.i[__XENON_DXT_BYTE_SWAP ^ 1] >> 6)].MergeAlpha(alpha_lut[alpha_indices[0x07]]);
	row2[0] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 2] >> 0)].MergeAlpha(alpha_lut[alpha_indices[0x08]]);
	row2[1] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 2] >> 2)].MergeAlpha(alpha_lut[alpha_indices[0x09]]);
	row2[2] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 2] >> 4)].MergeAlpha(alpha_lut[alpha_indices[0x0a]]);
	row2[3] = colour_lut[  (m_colour.i[__XENON_DXT_BYTE_SWAP ^ 2] >> 6)].MergeAlpha(alpha_lut[alpha_indices[0x0b]]);
	row3[0] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 3] >> 0)].MergeAlpha(alpha_lut[alpha_indices[0x0c]]);
	row3[1] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 3] >> 2)].MergeAlpha(alpha_lut[alpha_indices[0x0d]]);
	row3[2] = colour_lut[3&(m_colour.i[__XENON_DXT_BYTE_SWAP ^ 3] >> 4)].MergeAlpha(alpha_lut[alpha_indices[0x0e]]);
	row3[3] = colour_lut[  (m_colour.i[__XENON_DXT_BYTE_SWAP ^ 3] >> 6)].MergeAlpha(alpha_lut[alpha_indices[0x0f]]);
}

__forceinline void DXN_BLOCK::Decompress(void* dst, int dstBytesPerRow) const
{
	u8  x_lut[8];
	int x_indices[4*4];
	u8  y_lut[8];
	int y_indices[4*4];

	m_x.CreateLUT (x_lut);
	m_x.GetIndices(x_indices);
	m_y.CreateLUT (y_lut);
	m_y.GetIndices(y_indices);

	ARGB8888* row0 = (ARGB8888*)((u8*)dst + dstBytesPerRow*0);
	ARGB8888* row1 = (ARGB8888*)((u8*)dst + dstBytesPerRow*1);
	ARGB8888* row2 = (ARGB8888*)((u8*)dst + dstBytesPerRow*2);
	ARGB8888* row3 = (ARGB8888*)((u8*)dst + dstBytesPerRow*3);

	row0[0] = ARGB8888(x_lut[x_indices[0x00]], y_lut[y_indices[0x00]], 0, 255);
	row0[1] = ARGB8888(x_lut[x_indices[0x01]], y_lut[y_indices[0x01]], 0, 255);
	row0[2] = ARGB8888(x_lut[x_indices[0x02]], y_lut[y_indices[0x02]], 0, 255);
	row0[3] = ARGB8888(x_lut[x_indices[0x03]], y_lut[y_indices[0x03]], 0, 255);
	row1[0] = ARGB8888(x_lut[x_indices[0x04]], y_lut[y_indices[0x04]], 0, 255);
	row1[1] = ARGB8888(x_lut[x_indices[0x05]], y_lut[y_indices[0x05]], 0, 255);
	row1[2] = ARGB8888(x_lut[x_indices[0x06]], y_lut[y_indices[0x06]], 0, 255);
	row1[3] = ARGB8888(x_lut[x_indices[0x07]], y_lut[y_indices[0x07]], 0, 255);
	row2[0] = ARGB8888(x_lut[x_indices[0x08]], y_lut[y_indices[0x08]], 0, 255);
	row2[1] = ARGB8888(x_lut[x_indices[0x09]], y_lut[y_indices[0x09]], 0, 255);
	row2[2] = ARGB8888(x_lut[x_indices[0x0a]], y_lut[y_indices[0x0a]], 0, 255);
	row2[3] = ARGB8888(x_lut[x_indices[0x0b]], y_lut[y_indices[0x0b]], 0, 255);
	row3[0] = ARGB8888(x_lut[x_indices[0x0c]], y_lut[y_indices[0x0c]], 0, 255);
	row3[1] = ARGB8888(x_lut[x_indices[0x0d]], y_lut[y_indices[0x0d]], 0, 255);
	row3[2] = ARGB8888(x_lut[x_indices[0x0e]], y_lut[y_indices[0x0e]], 0, 255);
	row3[3] = ARGB8888(x_lut[x_indices[0x0f]], y_lut[y_indices[0x0f]], 0, 255);
}

} // namespace DXT
} // namespace rage

#undef __XENON_DXT_BYTE_SWAP

#if __WIN32
	#pragma warning(pop)
#endif

#endif // _IMAGE_DXT_H_
