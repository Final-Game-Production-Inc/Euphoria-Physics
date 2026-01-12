// 
// math/intrinsics.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef MATH_INTRINSICS_H
#define MATH_INTRINSICS_H

/*
	Use Xenon names where possible for now.  If we have to invent new types
	for portability, they use a single underscore instead of a double underscore.

	Note that on Xenon, __vector4 is untyped.  On PS3, there are many vector types
	and there is generally stronger type checking -- for example, __vcmpgefp accepts
	two vector floats as parameters but returns a vector signed int, while on Xenon
	it just accepts and returns __vector4 types.  This means that we inherit better
	type checking on Xenon by virtue of getting the code to also build on PS3.
	For example, we define both _vequalfp and  _vequal -- we can't overload based
	on input because on Xenon the inputs are untyped.  But they are typed on PS3,
	and use appropriately typed inline functions which can catch more problems
	at compile time.
*/

#if RSG_CPU_INTEL && defined(_MSC_VER)

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

unsigned char _BitScanForward(unsigned long* Index, unsigned long Mask);
unsigned char _BitScanReverse(unsigned long* Index, unsigned long Mask);
unsigned char _BitScanForward64(unsigned long* Index, rage::u64 Mask);
unsigned char _BitScanReverse64(unsigned long* Index, rage::u64 Mask);

#if defined(_MSC_VER)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)
#endif

#if __64BIT && defined(_MSC_VER)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(_BitScanForward64)
#else
__forceinline unsigned char _BitScanForward64(unsigned long* Index, rage::u64 Mask)
{
	unsigned char ret = _BitScanForward(Index, (unsigned long)Mask);
	if(!ret)
	{
		ret = _BitScanForward(Index, (unsigned long)(Mask >> 32));
		*Index += 32;
	}
	return ret;
}
__forceinline unsigned char _BitScanReverse64(unsigned long* Index, rage::u64 Mask)
{
	unsigned char ret = _BitScanReverse(Index, (unsigned long)(Mask >> 32));
	if(!ret)
		ret = _BitScanReverse(Index, (unsigned long)Mask);
	else
		*Index += 32;

	return ret;
}
#endif

#if defined(__cplusplus)
} // extern "C"
#endif // defined(__cplusplus)
#endif // RSG_CPU_INTEL

#if RSG_ORBIS
// _BitScanForward(&dest,src) is the same as dest = __builtin_ctz(src); 
// _BitScanReverse(&dest,src) is the same as dest = 31 - __builtin_clz(src);
__forceinline unsigned char _BitScanForward(unsigned *Index, unsigned Mask)
{
	*Index = __builtin_ctz(Mask);
	return Mask != 0;
}
__forceinline unsigned char _BitScanReverse(unsigned *Index, unsigned Mask)
{
	*Index = (31 - __builtin_clz(Mask));
	return Mask != 0;
}
__forceinline unsigned char _BitScanForward64(unsigned long *Index, unsigned long Mask)
{
	*Index = __builtin_ctzll(Mask);
	return Mask != 0;
}
__forceinline unsigned char _BitScanReverse64(unsigned long *Index, unsigned long Mask)
{
	*Index = (63 - __builtin_clzll(Mask));
	return Mask != 0;
}
#endif

#if __XENON

#include <ppcintrinsics.h>

// All zero vector, as integers
#define _vzero		__vspltisw(0)

// All zero vector, as floats
#define _vzerofp	__vspltisw(0)

// All 1's vector, as integers
#define _vneg1		__vspltisw(-1)
#define _vall1		__vspltisw(-1)

// All 0x00000001 word's vector
#define _vfour1		__vspltisw(1)

// Xenon doesn't distinguish between types, gcc does.
#define _ivector4	__vector4
#define _uvector4	__vector4
#define _cvector4	__vector4
#define _hvector4	__vector4
#define _ucvector4	__vector4

// Returns 1 if inputs are identical
__forceinline int _vequalfp(__vector4 a,__vector4 b) {
	unsigned int cr;
	__vcmpeqfpR(a, b, &cr);
	return (cr & 0x80) >> 7;
}

// Returns 1 if a >= b for all components.
__forceinline int _vgequalfp(__vector4 a,__vector4 b) {
	unsigned int cr;
	__vcmpgefpR(a, b, &cr);
	return (cr & 0x80) >> 7;
}

// Returns 1 if a > b for all components.
__forceinline int _vgreaterfp(__vector4 a,__vector4 b) {
	unsigned int cr;
	__vcmpgtfpR(a, b, &cr);
	return (cr & 0x80) >> 7;
}

// Returns 1 if a <= b for all components.
__forceinline int _vlequalfp(__vector4 a, __vector4 b)
{
	unsigned int cr;
	__vcmpgtfpR(a, b, &cr);
	return (cr & 0x20) >> 5;
}

// Returns 1 if a < b for all components.
__forceinline int _vlessfp(__vector4 a, __vector4 b)
{
	unsigned int cr;
	__vcmpgefpR(a, b, &cr);
	return (cr & 0x20) >> 5;
}

// Returns 1 if inputs  are identical
__forceinline int _vequal(_ivector4 a,_ivector4 b) {
	unsigned int cr;
	__vcmpequwR(a, b, &cr);
	return (cr & 0x80) >> 7;
}

#define VEC_PERM_X	0
#define VEC_PERM_Y	1
#define VEC_PERM_Z	2
#define VEC_PERM_W	3

// Returns float as a splatted __vector4.
__forceinline __vector4 _vsplatf(float scalar)
{
	return __vspltw(__lvlx(&scalar, 0), 0);
}

// RETURNS:	__vector4 as a float
// NOTE:	The input is assumed to be splatted, in other
//		words, containing the same value in all four channels.
//		If this is not the case, the value returned will depend
//		on the alignment of the destination float!
__forceinline float _vscalar(__vector4 v)
{
	return v.x;
}



#elif RSG_CPU_INTEL && !defined(_NO_INTRINSICS_)

#include <xmmintrin.h>
#include <emmintrin.h>		// SSE2

#if RSG_ORBIS
union __m128_union {
	float               m128_f32[4];
	unsigned long long  m128_u64[2];
	signed char			m128_i8[16];
	signed short		m128_i16[8];
	signed int			m128_i32[4];
	signed long long	m128_i64[2];
	unsigned char		m128_u8[16];
	unsigned short	    m128_u16[8];
	unsigned int		m128_u32[4];
};
#define M128_UNION_CAST(x)	((__m128_union&)(x))
#else
#define M128_UNION_CAST(x)	(x)
#endif

typedef __m128 __vector4;
typedef __m128 _ivector4;
typedef __m128 _uvector4;
typedef __m128 _ucvector4;

#define _vzerofp			_mm_setzero_ps()

#define __vaddfp			_mm_add_ps
#define __vsubfp			_mm_sub_ps
#define __vmulfp			_mm_mul_ps
#define __vnmsubfp(a,b,c)	__vsubfp( c, __vmulfp(a,b) )
#define __vmaddfp(a,b,c)	__vaddfp( __vmulfp(a,b), c )
#define __vand				_mm_and_ps
#define __vrefp				_mm_rcp_ps
#define __vrsqrtefp			_mm_rsqrt_ps
#define __vor				_mm_or_ps
#define __vxor				_mm_xor_ps
#define _vsplatf			_mm_set_ps1
#define _vscalar(v)			((v).m128_f32[0])

#define VEC_PERM_X	0
#define VEC_PERM_Y	1
#define VEC_PERM_Z	2
#define VEC_PERM_W	3

#elif __PS3

// PS3 doesn't have vpermwi, use "normal" altivec version
#define VEC_PERM_X 0x00010203
#define VEC_PERM_Y 0x04050607
#define VEC_PERM_Z 0x08090a0b
#define VEC_PERM_W 0x0c0d0e0f

# if __SPU
#include <vmx2spu.h>
typedef vector float __vector4;
typedef vector signed int _ivector4;
typedef vector unsigned int _uvector4;
typedef vector signed char _cvector4;
typedef vector unsigned char _ucvector4;
typedef vector signed short _hvector4;

#define __vcmpbfp			(_uvector4)spu_cmpabsgt

#define __vsubfp(a,b)		vec_sub(__vector4(a),__vector4(b))
#define __vaddfp(a,b)		vec_add(__vector4(a),__vector4(b))
#define __vmaddfp(a,b,c)	vec_madd(__vector4(a),__vector4(b),__vector4(c))
#define __vnmsubfp(a,b,c)	vec_nmsub(__vector4(a),__vector4(b),__vector4(c))
#define __vmulfp			spu_mul
#define __vrefp(a)			vec_re(__vector4(a))
#define __vrsqrtefp(a)		vec_rsqrte(__vector4(a))
#define __vmaxfp(a,b)		vec_max(__vector4(a),__vector4(b))
#define __vminfp(a,b)		vec_min(__vector4(a),__vector4(b))
#define __vsel(a,b,c)		vec_sel(__vector4(a),__vector4(b),(_uvector4)c)

#define __vmrghw		vec_mergeh
#define __vmrglw		vec_mergel

__forceinline __vector4			__vand(__vector4 a,__vector4 b) { return vec_and(a,b); }
__forceinline _uvector4			__vand(_uvector4 a,_uvector4 b) { return vec_and(a,b); }
__forceinline __vector4			__vandc(__vector4 a,__vector4 b) { return vec_andc(a,b); }
__forceinline _uvector4			__vandc(_uvector4 a,_uvector4 b) { return vec_andc(a,b); }

__forceinline __vector4			__vor(__vector4 a,__vector4 b) { return vec_or(a,b); }
__forceinline _uvector4			__vor(_uvector4 a,_uvector4 b) { return vec_or(a,b); }

#define __vcmpgefp(a,b)		(_uvector4)vec_cmpge(__vector4(a),__vector4(b))
#define __vcmpeqfp(a,b)		(_uvector4)vec_cmpeq(__vector4(a),__vector4(b))
#define __vcmpgtfp(a,b)		(_uvector4)vec_cmpgt(__vector4(a),__vector4(b))
#define __vcmpequw(a,b)		(_uvector4)vec_vcmpequw(__vector4(a),__vector4(b))

__forceinline int _vequal(_ivector4 a,_ivector4 b) {
    return vec_all_eq((_ivector4)a, (_ivector4)b);
}

__forceinline int _vequal(_uvector4 a,_uvector4 b) {
    return vec_all_eq((_uvector4)a, (_uvector4)b);
}

__forceinline int _vequal(__vector4 a,__vector4 b) {
    return vec_all_eq((_uvector4)a, (_uvector4)b);
}

# else // __SPU

#include <altivec.h>
#ifdef __SNC__
typedef vector float __vector4;
typedef vector signed int _ivector4;
typedef vector unsigned int _uvector4;
typedef vector signed char _cvector4;
typedef vector unsigned char _ucvector4;
typedef vector signed short _hvector4;
#else
typedef __attribute__((altivec(vector__))) float __vector4;
typedef __attribute__((altivec(vector__))) signed int _ivector4;
typedef __attribute__((altivec(vector__))) unsigned int _uvector4;
typedef __attribute__((altivec(vector__))) signed char _cvector4;
typedef __attribute__((altivec(vector__))) unsigned char _ucvector4;
typedef __attribute__((altivec(vector__))) signed short _hvector4;
#endif


#define __vcmpbfp(a,b)		(_uvector4)vec_vcmpbfp(__vector4(a),__vector4(b))

#define __vsubfp(a,b)		(__vector4)vec_sub(__vector4(a),__vector4(b))
#define __vaddfp(a,b)		(__vector4)vec_add(__vector4(a),__vector4(b))
#define __vmaddfp(a,b,c)	(__vector4)vec_madd(__vector4(a),__vector4(b),__vector4(c))
#define __vnmsubfp(a,b,c)	(__vector4)vec_nmsub(__vector4(a),__vector4(b),__vector4(c))
#define __vmulfp(a,b)		(__vector4)vec_madd(__vector4(a),__vector4(b),_vzerofp)
#define __vrefp(a)			(__vector4)vec_re(__vector4(a))
#define __vrsqrtefp(a)		(__vector4)vec_rsqrte(__vector4(a))
#define __vmaxfp(a,b)		(__vector4)vec_max(__vector4(a),__vector4(b))
#define __vminfp(a,b)		(__vector4)vec_min(__vector4(a),__vector4(b))

__forceinline __vector4 __vsel( __vector4 a,__vector4 b, _uvector4 c ) { return ((__vector4)vec_sel((a),(b),(c))); }
__forceinline __vector4 __vsel( __vector4 a,__vector4 b, __vector4 c ) { return ((__vector4)vec_sel((a),(b),(_uvector4)(c))); }

__forceinline __vector4 __vmrghw( __vector4 a, __vector4 b) { return  (__vector4)vec_mergeh(_ivector4(a),_ivector4(b)); }
__forceinline __vector4 __vmrglw( __vector4 a, __vector4 b) { return  (__vector4)vec_mergel(_ivector4(a),_ivector4(b)); }
__forceinline _ivector4 __vmrghw( _ivector4 a, _ivector4 b) { return  vec_mergeh(a,b); }
__forceinline _ivector4 __vmrglw( _ivector4 a, _ivector4 b) { return  vec_mergel(a,b); }
__forceinline _hvector4 __vmrghh( _hvector4 a, _hvector4 b) { return  vec_mergeh(a,b); }
__forceinline _hvector4 __vmrglh( _hvector4 a, _hvector4 b) { return  vec_mergel(a,b); }

__forceinline __vector4 __vand( __vector4 a, __vector4 b) { return  (__vector4)vec_and( _ivector4(a),_ivector4(b) ); }
__forceinline _uvector4 __vand( _uvector4 a, _uvector4 b) { return  (_uvector4)vec_and( _ivector4(a),_ivector4(b) ); }
__forceinline __vector4 __vandc( __vector4 a, __vector4 b) { return  (__vector4)vec_andc( _ivector4(a),_ivector4(b) ); }
__forceinline _uvector4 __vandc( _uvector4 a, _uvector4 b) { return  (_uvector4)vec_andc( _ivector4(a),_ivector4(b) ); }

__forceinline __vector4 __vor( __vector4 a, __vector4 b) { return  (__vector4)vec_or(_ivector4(a),_ivector4(b) ); }
__forceinline _uvector4 __vor( _uvector4 a, _uvector4 b) { return  (_uvector4)vec_or(_ivector4(a),_ivector4(b) ); }

#define __vcmpgefp(a,b)		(_uvector4)vec_cmpge(__vector4(a),__vector4(b))
#define __vcmpeqfp(a,b)		(_uvector4)vec_cmpeq(__vector4(a),__vector4(b))
#define __vcmpgtfp(a,b)		(_uvector4)vec_cmpgt(__vector4(a),__vector4(b))
#define __vcmpequw(a,b)		(_uvector4)vec_cmpeq(_uvector4(a),_uvector4(b))

// _vequal ALWAYS compares its inputs as integers, not floats.
__forceinline int _vequal(__vector4 a, __vector4 b) { return vec_all_eq((_ivector4)a,(_ivector4)b); }
__forceinline int _vequal(_uvector4 a, _uvector4 b) { return vec_all_eq((_ivector4)a,(_ivector4)b); }
__forceinline int _vequal(_ivector4 a, _ivector4 b) { return vec_all_eq(a,b); }

#define __vperm(a,b,c) ((__vector4)vec_perm((_ivector4)(a),(_ivector4)(b),(_cvector4)(c)))


#endif // __SPU

#define __vspltw(a,i)		vec_splat(__vector4(a),(i))
#define __vspltisw(i)		vec_splat_s32((i))
#define __vsplth(a,i)		vec_vsplth((a),(i))
#define __vspltish(i)		vec_splat_s16(i)
#define __vspltisb(i)		vec_splat_s8(i)


#define __vxor(a,b)			vec_xor(__vector4(a),__vector4(b))
#define __lvlx(a,b)			vec_lvlx((b),(a))
#define __lvrx(a,b)			vec_lvrx((b),(a))

#define _vzero			((_ivector4)vec_splat_s32(0))
#define _vzerofp		((__vector4)vec_splat_s32(0))
#define _vneg1			(vec_splat_s32(-1))
#define _vall1			((_uvector4)vec_splat_s32(-1))
#define _vfour1			((_ivector4)vec_splat_s32(1))
#define _vequalfp(a,b)		vec_all_eq(__vector4(a),__vector4(b))
#define _vgequalfp(a,b)		vec_all_ge(__vector4(a),__vector4(b))
#define _vgreaterfp(a,b)	vec_all_gt(__vector4(a),__vector4(b))
#define _vlequalfp(a,b)		vec_all_le(__vector4(a),__vector4(b))
#define _vlessfp(a,b)		vec_all_lt(__vector4(a),__vector4(b))





#define __vlogefp(a)	vec_loge(__vector4(a))
#define __vctsxs		vec_cts
#define __vpkuwum		vec_pack
#define	__vpkswss		vec_vpkswss
#define __vexptefp(a)	vec_expte(__vector4(a))
#define __vcfsx			vec_vcfsx
#define __vcfux			vec_vcfux
#define __vrfin			vec_round
#define __vrfim			vec_floor
#define __vrfiz			vec_trunc
#define __vrfip			vec_ceil
#define __vupkhsb(a)	((_hvector4)vec_unpackh((_cvector4) (a)))
#define __vupklsb(a)	((_hvector4)vec_unpackl((_cvector4) (a)))
#define __vupkhsh(a)	((_ivector4)vec_unpackh((_hvector4) (a)))
#define __vupklsh(a)	((_ivector4)vec_unpackl((_hvector4) (a)))
#define __vcsxwfp		vec_vcfsx
#define __vctuxs(a,b)	vec_ctu((a),(b))

#define __vslw(a,b)			vec_sl((a),_uvector4(b))
#define __vslh(a,b)			vec_sl((a),_hvector4(b))
#define __vslb(a,b)			vec_sl((a),_cvector4(b))

#define __vsrw(a,b)			vec_sr((a),_uvector4(b))
#define __vsrh(a,b)			vec_sr((a),_hvector4(b))
#define __vsrb(a,b)			vec_sr((a),_cvector4(b))

#define __vrlw(a,b)			vec_rl((a),_uvector4(b))
#define __vrlh(a,b)			vec_rl((a),_hvector4(b))
#define __vrlb(a,b)			vec_rl((a),_cvector4(b))

#define __vsraw(a,b)		vec_sra((a),_uvector4(b))
#define __vsrah(a,b)		vec_sra((a),_hvector4(b))
#define __vsrab(a,b)		vec_sra((a),_cvector4(b))

#define __vsl(a,b)			vec_sll((a),(b))
#define __vsr(a,b)			vec_srl((a),(b))
#define __vslo(a,b)			vec_slo((a),(b))
#define __vsro(a,b)			vec_sro((a),(b))
#define __vsldoi(a,b,imm)	vec_sld((a),(b),imm)

// Parameter order is swapped!
#define __stvewx(v,ptr,off)	vec_stvewx(v,off,ptr)
#define __stvlx(v,ptr,off)	vec_stvlx(v,off,ptr)
#define __stvrx(v,ptr,off)	vec_stvrx(v,off,ptr)

// Once again, note that the parameter order is swapped!
#define __stvx(a,b,c)	vec_stvx((a),(c),(b))
#define __stvehx(a,b,c)	vec_stvehx((a),(c),(b))

// This is suboptimal!  (Can we improve latency via vmaddfp?)
__forceinline __vector4 __vdot3fp(__vector4 a,__vector4 b) {
 	//__vector4 prod = __vmulfp(a,b);
 	//return __vaddfp(__vaddfp(__vspltw(prod,0),__vspltw(prod,1)),__vspltw(prod,2));
// sce version:
 	__vector4 r = __vmulfp(a, b);
 	r = __vmaddfp(__vsldoi(a, a, 4), __vsldoi(b, b, 4), r);
 	r = __vmaddfp(__vsldoi(a, a, 8), __vsldoi(b, b, 8), r);
 	return __vspltw(r, 0);
}

// This is suboptimal!  (Can we improve latency via vmaddfp?)
__forceinline __vector4 __vdot4fp(__vector4 a,__vector4 b) {
 	//__vector4 prod = __vmulfp(a,b);
 	//return __vaddfp(__vaddfp(__vspltw(prod,0),__vspltw(prod,1)),__vaddfp(__vspltw(prod,2),__vspltw(prod,3)));
// sce version:
 	__vector4 r = __vmulfp(a, b);
 	r = __vmaddfp(__vsldoi(a, a, 4), __vsldoi(b, b, 4), r);
 	return __vaddfp(__vsldoi(r, r, 8), r);
}

#if __SPU
#define _vsplatf(x)	spu_splats((float)(x))
#else
__forceinline __vector4 _vsplatf(float scalar)
{
	__vector4 result;
	if (__builtin_constant_p(scalar))
		result = (__vector4){scalar, scalar, scalar, scalar};
	else
		result = (__vector4) vec_lvlx (0, &scalar);
	return __vspltw(result,0);
}
#endif

#if __SPU
__forceinline float _vscalar(__vector4 v) { return spu_extract(v,0); }
#else
__forceinline float _vscalar(__vector4 v)
{
	float result;
	vec_stvewx (v, 0, &result);
	return result;
}
#endif

// Use Xenon nomenclature
#if !__SPU
#define _CountLeadingZeros		__builtin_clz
#define _CountLeadingZeros64	__builtin_clzll
#else
__forceinline int _CountLeadingZeros(int x) {return si_to_int(si_clz(si_from_int(x)));}
__forceinline int _CountLeadingZeros64(rage::s64 x) {int r = _CountLeadingZeros(x>>32); return r == 32 ? r+_CountLeadingZeros(int(x)) : r;}
#endif

#elif __PSP2

#define _CountLeadingZeros		__builtin_clz

typedef float32x4_t __vector4;
typedef int32x4_t _ivector4;
typedef uint32x4_t _uvector4;
typedef int8x8_t _cvector4;
typedef uint8x8_t _ucvector4;
typedef int16x8_t _hvector4;

#endif	// end of #if __XENON #elif __WIN32PC #elif __PPU

#define _vmuleq(lhs,rhs)	(lhs = __vmulfp(lhs,rhs))
#define _vnegeq(x)			(x = __vsubfp(_vzerofp,(x)))

#if !RSG_CPU_INTEL && !__PSP2
__forceinline __vector4 _vmag2(__vector4 x) { 
	return __vdot3fp(x,x);
}
#endif

#if RSG_CPU_INTEL
#ifdef _MSC_VER
__forceinline int _CountLeadingZeros(int value) {
	unsigned long firstBit=0;
	return _BitScanReverse(&firstBit, value) ? (31 - firstBit) : 32;
}
__forceinline int _CountLeadingZeros64(rage::s64 x) {int r = _CountLeadingZeros(int(x>>32)); return r == 32 ? r+_CountLeadingZeros(int(x)) : r;}
#elif RSG_ORBIS
__forceinline int _CountLeadingZeros(int value) {
	return __builtin_clz(value);
}
__forceinline int _CountLeadingZeros64(rage::s64 value) {
	return __builtin_clzll(value);
}

#else
__forceinline int _CountLeadingZeros(int value) {
	int count = 0;
	while (value >= 0) {
		value = (value<<1)|1;
		++count;
	}
	return count;
}

__forceinline int _CountLeadingZeros64(rage::s64 value) {
	int count = 0;
	while (value >= 0) {
		value = (value<<1)|1;
		++count;
	}
	return count;
}
#endif // 0
#endif

// RETURNS: True if input is a power of two.  Note that zero is considered a power of two.
__forceinline bool _IsPowerOfTwo(int value) { return (value & (value-1)) == 0; }

// RETURNS: Log2 of input, rounded up to next power of two if it wasn't already one
__forceinline int _CeilLog2(int value) {
	int result = 31 - _CountLeadingZeros(value);
	return _IsPowerOfTwo(value)? result : result+1;
}

// RETURNS: Log2 of input, rounded down to next power of two if it wasn't already one
__forceinline int _FloorLog2(int value) {
	return 31 - _CountLeadingZeros(value);
}

// RETURNS: Input rounded up to next power of two if it wasn't already one
__forceinline rage::u32 _RoundUpPowerOf2(rage::u32 value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	return ++value;
}

#if RSG_PC || RSG_DURANGO
__forceinline int _FirstBitIndex(unsigned long word) {
	unsigned long index = 0;
	return _BitScanReverse(&index, word) ? index : -1;
}

__forceinline int _LastBitIndex(unsigned long word) {
	unsigned long index = 0;
	return _BitScanForward(&index, word) ? index : -1;
}
#elif __PS3 || __XENON
__forceinline int _FirstBitIndex(unsigned long word) {
	return 31 - _CountLeadingZeros(word);
}

__forceinline int _LastBitIndex(unsigned long word) {
	long reverse = word & (~word + 1);
	return 31 - _CountLeadingZeros(reverse);
}
#else
__forceinline int _FirstBitIndex(unsigned long word) {
	if (word)
	{
		int bit = 31;

		if (!word) bit -= 1;
		if (!(word & 0xffff0000)) { word <<= 16; bit -= 16; }
		if (!(word & 0xff000000)) { word <<= 8; bit -= 8; }
		if (!(word & 0xf0000000)) { word <<= 4; bit -= 4; }
		if (!(word & 0xc0000000)) { word <<= 2; bit -= 2; }
		if (!(word & 0x80000000)) { word <<= 1; bit -= 1; }

		return bit;
	}
	else
	{
		return -1;
	}
}

__forceinline int _LastBitIndex(unsigned long word) {
	if (word)
	{
		int bit = 31;
		word = word & (~word + 1); // reverse

		if (!word) bit -= 1;
		if (!(word & 0xffff0000)) { word <<= 16; bit -= 16; }
		if (!(word & 0xff000000)) { word <<= 8; bit -= 8; }
		if (!(word & 0xf0000000)) { word <<= 4; bit -= 4; }
		if (!(word & 0xc0000000)) { word <<= 2; bit -= 2; }
		if (!(word & 0x80000000)) { word <<= 1; bit -= 1; }

		return bit;
	}
	else
	{
		return -1;
	}
}
#endif

#if __SPU 
// There is no instruction on the SPU to do a 64 bit integer multiply.
// Doing so in code will cause gcc to insert a call to the function __muldi3.
// This function is used by mthRandom to avoid the function call

__forceinline unsigned long long _spuMulu32Byu32Outu64(unsigned int a, unsigned int b)
{
	unsigned int lowA  = a & 0xFFFF;
	unsigned int highA = a >> 16;

	unsigned int lowB  = b & 0xFFFF;
	unsigned int highB = b >> 16;

	// 0 - 31 range
	unsigned long long lowALowB  = (unsigned long long)(lowA * lowB);

	// 16 - 47 range
	unsigned long long lowAHighB = (unsigned long long)(lowA * highB); 

	// 16 - 47 range
	unsigned long long highALowB  = (unsigned long long)(highA * lowB);

	// 32 - 63 range
	unsigned long long highAHighB = (unsigned long long)(highA * highB);

	return (highAHighB << 32) + ((lowAHighB + highALowB) << 16) + lowALowB;
}
#endif

// just to make checkheaders succeed
namespace rage {
}


#endif	// MATH_INTRINSICS_H
