// 
// math/float16.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef MATH_FLOAT16_H
#define MATH_FLOAT16_H

#include "vectormath/classes.h"
#include "vector/color32.h"
#include "math/altivec2.h"

// define RSG_SSE_VERSION and include proper intrinsic headers
#if GTA_VERSION && !defined(RSG_SSE_VERSION)
	#if RSG_ORBIS || RSG_DURANGO
		#include <immintrin.h> // AVX
		#define RSG_SSE_VERSION 50
		#if RSG_ORBIS
			#define _mm_undefined_si128 _mm_setzero_si128
		#endif
	#elif RSG_PC && !__TOOL && !__RESOURCECOMPILER
		#include <pmmintrin.h> // SSE3
		#define RSG_SSE_VERSION 30
	#elif RSG_PC
		#include <emmintrin.h> // SSE2
		#define RSG_SSE_VERSION 20
	#else
		#define RSG_SSE_VERSION 0
	#endif
#endif // GTA_VERSION && !defined(RSG_SSE_VERSION)

namespace rage {

class Float16
{
public:
	// resourcing --------------------------------------------------------------
	enum
	{
		RORC_VERSION = 1
	};
	Float16(datResource& UNUSED_PARAM(rsc))
	{
	}
	void Place(datResource& rsc) 
	{ 
		::new (this) Float16(rsc);
	}
	int Release() const {delete this; return 0;} // Not true reference counting yet!
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct& s);
#endif
	// resourcing --------------------------------------------------------------

	// ================================================================ = seee.eemm.mmmm.mmmm = ========================
	// Max (positive) representable Float16 is (1 + 1023/1024)*(2^^+16) = 0111.1111.1111.1111 = 0x7FFF = 131008.0
	// Min (positive) representable Float16 is (1 +    1/1024)*(2^^-15) = 0000.0000.0000.0001 = 0x0001 = 0.00003054738..
	// 
	// Max (positive) .. if e=31 is INF/NaN .. (1 + 1023/1024)*(2^^+15) = 0111.1011.1111.1111 = 0x7BFF = 65504.0
	// Min (positive) .. if e=0 is denormal .. (1 +    0/1024)*(2^^-14) = 0000.0100.0000.0000 = 0x0400 = 0.00006103516..
	// =================================================================================================================
	// 
	// converting float -> Float16:
	// --------------------------------
	// 	float values > +MAXFP16 (including +INF and +NaN) get clamped to +MAXFP16
	// 	float values < -MAXFP16 (including -INF and -NaN) get clamped to -MAXFP16
	// 	positive denormalised floats (including +0.0) get clamped to +0.0
	// 	negative denormalised floats (including -0.0) get clamped to -0.0
	// 
	// converting Float16 -> float
	// --------------------------------
	// 	all exponent values [0..31] are valid and do not represent denormals or INF/NaN
	// 	+0.0 is represented as 0x0000, as expected
	// 	-0.0 is represented as 0x8000, as expected
	// 	converting [seeeeemm.mmmmmmmm] to float is just (<s>?-1:+1)*(1.<mmmmmmmmmm>)*2^(<eeeee>-15)
	// =================================================================================================================

	enum { BINARY_0 = 0x0000 }; // equivalent to 0.0f [0x00000000]
	enum { BINARY_1 = 0x3c00 }; // equivalent to 1.0f [0x3f800000]

	__forceinline Float16() {}

	explicit __forceinline Float16(f32 f) { SetFloat16_FromFloat32(f); }
	explicit __forceinline Float16(u16 u) { m_data = u; }

	__forceinline void        SetFloat16_Zero() { m_data = BINARY_0; }
	__forceinline void        SetFloat16_One()  { m_data = BINARY_1; }
	__forceinline void        SetFloat16_FromFloat32(f32 f);
	__forceinline void        SetFloat16_FromFloat32(ScalarV_In f);
	__forceinline void        SetFloat16_FromFloat32_NoIntrinsics(f32 f);
	__forceinline f32         GetFloat32_FromFloat16() const;
	__forceinline f32         GetFloat32_FromFloat16_NoIntrinsics() const;
	__forceinline ScalarV_Out GetFloat32_FromFloat16_ScalarV() const;
	__forceinline u16         GetBinaryData() const { return m_data; }
	__forceinline void        SetBinaryData(u16 u) { m_data = u; }

//#if !__FINAL
	// Explicit rounding mode conversions.  Currently slower implementations, primarily intended for offline tools.
	__forceinline void SetFloat16_FromFloat32_RndNearest(f32 f);
//#endif

	// http://guru.multimedia.cx/avoiding-branchesifconditionals
	static __forceinline u32 GetNonZeroMask(u32 x) // returns (x ? 0xffffffff : 0) without branching
	{
		return (u32)( ( (s32)(x) | -(s32)(x) ) >> 31 );
	}

	__forceinline bool operator==(const Float16& other) const
	{
		return (m_data == other.m_data);
	}

	__forceinline bool operator!=(const Float16& other) const
	{
		return (m_data != other.m_data);
	}

private:
	u16 m_data;
};

class ALIGNAS(8) Float16Vec4
{
public:
	__forceinline bool operator==(const Float16Vec4& other)
	{
		return (x == other.x) && (y == other.y) && (z == other.z) && (w == other.w);
	}

	__forceinline bool operator!=(const Float16Vec4& other)
	{
		return (x != other.x) || (y != other.y) || (z != other.z) || (w != other.w);
	}

public:
	Float16 x,y,z,w;
} ;

class ALIGNAS(4) Float16Vec2
{
public:
	__forceinline bool operator==(const Float16Vec4& other)
	{
		return (x == other.x) && (y == other.y);
	}

	__forceinline bool operator!=(const Float16Vec4& other)
	{
		return (x != other.x) || (y != other.y);
	}

public:
	Float16 x,y;
} ;

namespace Vec {

void V4Float16StreamPack  (Vector_4V* RESTRICT dstV, const Vector_4V* RESTRICT srcV, int count); //   packs count Float16's (count must be a multiple of 8)
void V4Float16StreamUnpack(Vector_4V* RESTRICT dstV, const Vector_4V* RESTRICT srcV, int count); // unpacks count Float16's (count must be a multiple of 8)

__forceinline Vector_4V_Out V4Float16Vec8Pack          (Vector_4V_In src0, Vector_4V_In src1); // packs 8 Float16's
__forceinline Vector_4V_Out V4Float16Vec8Pack_INSOMNIAC(Vector_4V_In src0, Vector_4V_In src1); // packs 8 Float16's (no clamping)

__forceinline Vector_4V_Out V4Float16Vec4PackIntoXY(Vector_4V_In v, Vector_4V_In src); // packs src.xyzw into v.xy and returns v
__forceinline Vector_4V_Out V4Float16Vec4PackIntoZW(Vector_4V_In v, Vector_4V_In src); // packs src.xyzw into v.zw and returns v

__forceinline Vector_4V_Out V4Float16Vec2PackIntoX(Vector_4V_In v, Vector_4V_In src); // packs src.xy into v.x and returns v
__forceinline Vector_4V_Out V4Float16Vec2PackIntoY(Vector_4V_In v, Vector_4V_In src); // packs src.xy into v.y and returns v
__forceinline Vector_4V_Out V4Float16Vec2PackIntoZ(Vector_4V_In v, Vector_4V_In src); // packs src.xy into v.z and returns v
__forceinline Vector_4V_Out V4Float16Vec2PackIntoW(Vector_4V_In v, Vector_4V_In src); // packs src.xy into v.w and returns v

__forceinline void V4Float16Vec4Pack(Float16Vec4* dst, Vector_4V_In src);
__forceinline void V4Float16Vec2Pack(Float16Vec2* dst, Vector_4V_In src);

__forceinline void V4Float16Vec8Unpack(Vector_4V_InOut dst0, Vector_4V_InOut dst1, Vector_4V_In src); // unpacks 8 Float16's

__forceinline Vector_4V_Out V4Float16Vec4UnpackFromXY(Vector_4V_In src); // unpacks src.xy into v.xyzw and returns v
__forceinline Vector_4V_Out V4Float16Vec4UnpackFromZW(Vector_4V_In src); // unpacks src.zw into v.xyzw and returns v

__forceinline Vector_4V_Out V4Float16Vec2UnpackFromX(Vector_4V_In src); // unpacks src.x into v.xy and returns v
__forceinline Vector_4V_Out V4Float16Vec2UnpackFromY(Vector_4V_In src); // unpacks src.y into v.xy and returns v
__forceinline Vector_4V_Out V4Float16Vec2UnpackFromZ(Vector_4V_In src); // unpacks src.z into v.xy and returns v
__forceinline Vector_4V_Out V4Float16Vec2UnpackFromW(Vector_4V_In src); // unpacks src.w into v.xy and returns v

__forceinline Vector_4V_Out V4Float16Vec4Unpack(const Float16Vec4* src);
__forceinline Vector_4V_Out V4Float16Vec2Unpack(const Float16Vec2* src);

__forceinline Vector_4V_Out V4PackARGBColor32IntoWComponent(Vector_4V_In vectorXYZ, Vector_4V_In vectorRGBA); // packs R,G,B,A vector into 4 bytes and stores in w-component as ARGB (matches Color32)
__forceinline Vector_4V_Out V4PackABGRColor32IntoWComponent(Vector_4V_In vectorXYZ, Vector_4V_In vectorRGBA); // packs R,G,B,A vector into 4 bytes and stores in w-component as ABGR (matches Color32)
__forceinline Vector_4V_Out V4PackRGBAColor32IntoWComponent(Vector_4V_In vectorXYZ, Vector_4V_In vectorRGBA); // packs R,G,B,A vector into 4 bytes and stores in w-component as RGBA

__forceinline void V4PackARGBColor32(Color32* dst, Vector_4V_In vectorRGBA);
__forceinline void V4PackABGRColor32(Color32* dst, Vector_4V_In vectorRGBA);
__forceinline void V4PackRGBAColor32(u32    * dst, Vector_4V_In vectorRGBA);

__forceinline Vector_4V_Out V4UnpackARGBColor32(const Color32* src); // unpacks from ARGB to R,G,B,A vector (matches Color32)
__forceinline Vector_4V_Out V4UnpackABGRColor32(const Color32* src); // unpacks from ABGR to R,G,B,A vector (matches Color32)
__forceinline Vector_4V_Out V4UnpackRGBAColor32(const u32    * src); // unpacks from RGBA to R,G,B,A vector

} // namespace Vec

__forceinline Vec4V_Out Float16Vec8Pack(Vec4V_In src0, Vec4V_In src1);

__forceinline Vec4V_Out Float16Vec4PackIntoXY(Vec4V_In v, Vec4V_In src); // packs src.xyzw into v.xy and returns v
__forceinline Vec4V_Out Float16Vec4PackIntoZW(Vec4V_In v, Vec4V_In src); // packs src.xyzw into v.zw and returns v

__forceinline Vec4V_Out Float16Vec2PackIntoX(Vec4V_In v, Vec2V_In src); // packs src.xy into v.x and returns v
__forceinline Vec4V_Out Float16Vec2PackIntoY(Vec4V_In v, Vec2V_In src); // packs src.xy into v.y and returns v
__forceinline Vec4V_Out Float16Vec2PackIntoZ(Vec4V_In v, Vec2V_In src); // packs src.xy into v.z and returns v
__forceinline Vec4V_Out Float16Vec2PackIntoW(Vec4V_In v, Vec2V_In src); // packs src.xy into v.w and returns v

__forceinline void Float16Vec4Pack(Float16Vec4* dst, Vec4V_In src);
__forceinline void Float16Vec4Pack(Float16Vec4* dst, Vec3V_In src);
__forceinline void Float16Vec2Pack(Float16Vec2* dst, Vec2V_In src);

__forceinline void Float16Vec8Unpack(Vec4V_InOut dst0, Vec4V_InOut dst1, Vec4V_In src);

__forceinline Vec4V_Out Float16Vec4UnpackFromXY(Vec4V_In src); // unpacks src.xy into v.xyzw and returns v
__forceinline Vec4V_Out Float16Vec4UnpackFromZW(Vec4V_In src); // unpacks src.zw into v.xyzw and returns v

__forceinline Vec2V_Out Float16Vec2UnpackFromX(Vec4V_In src); // unpacks src.x into v.xy and returns v
__forceinline Vec2V_Out Float16Vec2UnpackFromY(Vec4V_In src); // unpacks src.y into v.xy and returns v
__forceinline Vec2V_Out Float16Vec2UnpackFromZ(Vec4V_In src); // unpacks src.z into v.xy and returns v
__forceinline Vec2V_Out Float16Vec2UnpackFromW(Vec4V_In src); // unpacks src.w into v.xy and returns v

__forceinline Vec4V_Out Float16Vec4Unpack(const Float16Vec4* src);
__forceinline Vec2V_Out Float16Vec2Unpack(const Float16Vec2* src);

__forceinline Vec4V_Out PackARGBColor32IntoWComponent(Vec3V_In vectorXYZ, Vec4V_In vectorRGBA); // packs R,G,B,A vector into 4 bytes and stores in w-component as ARGB (matches Color32)
__forceinline Vec4V_Out PackABGRColor32IntoWComponent(Vec3V_In vectorXYZ, Vec4V_In vectorRGBA); // packs R,G,B,A vector into 4 bytes and stores in w-component as ABGR (matches Color32)
__forceinline Vec4V_Out PackRGBAColor32IntoWComponent(Vec3V_In vectorXYZ, Vec4V_In vectorRGBA); // packs R,G,B,A vector into 4 bytes and stores in w-component as RGBA

__forceinline void PackARGBColor32(Color32* dst, Vec4V_In vectorRGBA);
__forceinline void PackABGRColor32(Color32* dst, Vec4V_In vectorRGBA);
__forceinline void PackRGBAColor32(u32    * dst, Vec4V_In vectorRGBA);

__forceinline Vec4V_Out UnpackARGBColor32(const Color32* src); // unpacks from ARGB to R,G,B,A vector (matches Color32)
__forceinline Vec4V_Out UnpackABGRColor32(const Color32* src); // unpacks from ABGR to R,G,B,A vector (matches Color32)
__forceinline Vec4V_Out UnpackRGBAColor32(const u32    * src); // unpacks from RGBA to R,G,B,A vector

__forceinline Vec4V_Out FixedPoint3_13FromVec4(Vec4V_In v);
__forceinline Vec4V_Out FixedPoint3_13Vec4PackIntoXY(Vec4V_In packed, Vec4V_In toPack);
__forceinline Vec4V_Out FixedPoint3_13Vec4PackIntoZW(Vec4V_In packed, Vec4V_In toPack);
__forceinline Vec4V_Out FixedPoint3_13Vec4UnpackFromXY(Vec4V_In v);
__forceinline Vec4V_Out FixedPoint3_13Vec4UnpackFromZW(Vec4V_In v);

#if __DEV
float TestFloat16Pack(int n);
float TestFloat16Unpack(int n);
float TestFloat16StreamPack(int n);
float TestFloat16StreamUnpack(int n);
#endif // __DEV

} // namespace rage

#include "float16.inl"

#endif	// MATH_FLOAT16_H
