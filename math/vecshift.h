// ======================
// math/vecshift.h
// (c) 2013 RockstarNorth
// ======================

#ifndef _MATH_VECSHIFT_H_
#define _MATH_VECSHIFT_H_

#include "math/altivec2.h"
#include "vectormath/classes.h"

#define BIT64(i) (1ULL<<(i))

namespace rage {

#if RSG_CPU_INTEL

__forceinline __m128i _mm_slli_si128_bits(__m128i a, int i)
{
	if (i <= 64)
	{
		__m128i b;

		b = _mm_slli_epi64(a, i);
		a = _mm_srli_epi64(a, 64 - i);
		a = _mm_slli_si128(a, 8);
		a = _mm_or_si128  (a, b);
	}
	else
	{
		a = _mm_slli_epi64(a, i - 64);
		a = _mm_slli_si128(a, 8);
	}

	return a;
}

__forceinline __m128i _mm_srli_si128_bits(__m128i a, int i)
{
	if (i <= 64)
	{
		__m128i b;

		b = _mm_srli_epi64(a, i);
		a = _mm_slli_epi64(a, 64 - i);
		a = _mm_srli_si128(a, 8);
		a = _mm_or_si128  (a, b);
	}
	else
	{
		a = _mm_srli_epi64(a, i - 64);
		a = _mm_srli_si128(a, 8);
	}

	return a;
}

#endif // RSG_CPU_INTEL

namespace Vec {

#if RSG_CPU_PPC
template <u32 bytes> __forceinline Vector_4V_Out V4ShiftLeftBytesDouble(Vector_4V_In a, Vector_4V_In b) { return __altivec_vsldoi(a, b, bytes); }
#endif // RSG_CPU_PPC

__forceinline Vector_4V_Out V4ShiftLeft(Vector_4V_In a, Vector_4V_In shift)
{
#if RSG_CPU_PPC
	return __altivec_vslw(a, shift);
#elif RSG_CPU_SPU
	return (Vector_4V)si_shl((qword)a, (qword)shift);
#elif RSG_CPU_INTEL
	return _mm_castsi128_ps(_mm_sll_epi32(_mm_castps_si128(a), _mm_castps_si128(shift)));
#endif // RSG_CPU_INTEL
}

__forceinline Vector_4V_Out V4ShiftRight(Vector_4V_In a, Vector_4V_In shift)
{
#if RSG_CPU_PPC
	return __altivec_vsrw(a, shift);
#elif RSG_CPU_SPU
	return (Vector_4V)si_rotm((qword)a, si_sfi((qword)shift, 0));
#elif RSG_CPU_INTEL
	return _mm_castsi128_ps(_mm_srl_epi32(_mm_castps_si128(a), _mm_castps_si128(shift)));
#endif // RSG_CPU_INTEL
}

__forceinline Vector_4V_Out V4ShiftRightAlgebraic(Vector_4V_In a, Vector_4V_In shift)
{
#if RSG_CPU_PPC
	return __altivec_vsraw(a, shift);
#elif RSG_CPU_SPU
	return (Vector_4V)si_rotma((qword)a, si_sfi((qword)shift, 0));
#elif RSG_CPU_INTEL
	return _mm_castsi128_ps(_mm_sra_epi32(_mm_castps_si128(a), _mm_castps_si128(shift)));
#endif // RSG_CPU_INTEL
}

__forceinline u32 V4GetSignMask(Vector_4V_In v) // returns sign bits in lower 4 bits of result
{
#if RSG_CPU_PPC
	const Vector_4V tmpV = V4And(V4ShiftRight<31>(v), V4VConstant<1,2,4,8>());
	const Vector_4V tmp0 = V4SplatX(tmpV);
	const Vector_4V tmp1 = V4SplatY(tmpV);
	const Vector_4V tmp2 = V4SplatZ(tmpV);
	const Vector_4V tmp3 = V4SplatW(tmpV);
	u32 i;
	V4StoreScalar32FromSplatted(i, V4Or(V4Or(tmp0, tmp1), V4Or(tmp2, tmp3)));
	return i;
#elif RSG_CPU_SPU
	return spu_extract((Vector_4V_uint)si_gb((Vector_4V_char)V4ShiftRight<31>(V4Permute<W,Z,Y,X>(v))), 0);
#elif RSG_CPU_INTEL
	return (u32)_mm_movemask_ps(v); // does this require a permute?
#else // reference
	u32 i = 0;
	i |= ((GetXi(v) >> 31) & 1);
	i |= ((GetYi(v) >> 30) & 2);
	i |= ((GetZi(v) >> 29) & 4);
	i |= ((GetWi(v) >> 28) & 8);
	return i;
#endif // reference
}

__forceinline u32 V4GetBoolMask(Vector_4V_In v) // assumes each element of v is all 1's or all 0's
{
#if RSG_CPU_PPC
	const Vector_4V tmpV = V4And(v, V4VConstant<1,2,4,8>());
	const Vector_4V tmp0 = V4SplatX(tmpV);
	const Vector_4V tmp1 = V4SplatY(tmpV);
	const Vector_4V tmp2 = V4SplatZ(tmpV);
	const Vector_4V tmp3 = V4SplatW(tmpV);
	u32 i;
	V4StoreScalar32FromSplatted(i, V4Or(V4Or(tmp0, tmp1), V4Or(tmp2, tmp3)));
	return i;
#elif RSG_CPU_SPU
	return spu_extract((Vector_4V_uint)si_gb((Vector_4V_char)(V4Permute<W,Z,Y,X>(v))), 0);
#elif RSG_CPU_INTEL
	return (u32)_mm_movemask_ps(v); // does this require a permute?
#else // reference
	u32 i = 0;
	i |= (GetXi(v) & 1);
	i |= (GetYi(v) & 2);
	i |= (GetZi(v) & 4);
	i |= (GetWi(v) & 8);
	return i;
#endif // reference
}

__forceinline Vector_4V_Out V4GenerateMaskShiftLeft128(Vector_4V_In boundsInt) // i0 and i1 are in x,y components
{
#if RSG_CPU_PPC
	const __vec128 ones   = __altivec_vspltisw(-1);
	const __vec128 shift0 = __altivec_vspltb(boundsInt, 3);
	const __vec128 shift1 = __altivec_vspltb(boundsInt, 7);
	const __vec128 mask0  = __altivec_vsl(__altivec_vslo(ones, shift0), shift0);
	const __vec128 mask1  = __altivec_vsl(__altivec_vslo(ones, shift1), shift1);
	const __vec128 temp0  = __altivec_vsrab(shift0, __vspltisb(7)); // splat MSB of shift0, will be all 1's if shift0 was >= 128
	const __vec128 temp1  = __altivec_vsrab(shift1, __vspltisb(7)); // splat MSB of shift1, will be all 1's if shift1 was >= 128
	const __vec128 mask   = __altivec_vxor(__altivec_vandc(mask0, temp0), __altivec_vandc(mask1, temp1));
//	const __vec128 eq     = __altivec_vcmpequb(shift0, shift1);
//	const __vec128 mask   = __altivec_vandc(__altivec_vxor(mask0, mask1), eq);
	return mask;
#elif RSG_CPU_SPU
	const qword splat0 = (qword)(vector unsigned int){0x00010203, 0x00010203, 0x00010203, 0x00010203};
	const qword splat1 = (qword)(vector unsigned int){0x04050607, 0x04050607, 0x04050607, 0x04050607};
	const qword ones   = si_il(-1);
	const qword shift  = (qword)boundsInt;
	const qword shift0 = si_shufb(shift, shift, splat0);
	const qword shift1 = si_shufb(shift, shift, splat1);
	const qword mask0  = si_shlqbi(si_shlqbybi(ones, shift0), shift0);
	const qword mask1  = si_shlqbi(si_shlqbybi(ones, shift1), shift1);
	const qword mask   = si_xor(mask0, mask1);
	return (Vector_4V)mask;
#elif RSG_CPU_INTEL
	const __m128i ones  = _mm_set1_epi32(~0);
	const __m128i mask0 = _mm_slli_si128_bits(ones, GetXi(boundsInt));
	const __m128i mask1 = _mm_slli_si128_bits(ones, GetYi(boundsInt));
	const __m128i mask  = _mm_xor_si128(mask0, mask1);
	return _mm_castsi128_ps(mask);
#endif // RSG_CPU_INTEL
}

__forceinline Vector_4V_Out V4GenerateMaskShiftRight128(Vector_4V_In boundsInt) // i0 and i1 are in x,y components
{
#if RSG_CPU_PPC
	const __vec128 ones   = __altivec_vspltisw(-1);
	const __vec128 shift0 = __altivec_vspltb(boundsInt, 3);
	const __vec128 shift1 = __altivec_vspltb(boundsInt, 7);
	const __vec128 mask0  = __altivec_vsr(__altivec_vsro(ones, shift0), shift0);
	const __vec128 mask1  = __altivec_vsr(__altivec_vsro(ones, shift1), shift1);
	const __vec128 temp0  = __altivec_vsrab(shift0, __vspltisb(7)); // splat MSB of shift0, will be all 1's if shift0 was >= 128
	const __vec128 temp1  = __altivec_vsrab(shift1, __vspltisb(7)); // splat MSB of shift1, will be all 1's if shift1 was >= 128
	const __vec128 mask   = __altivec_vxor(__altivec_vandc(mask0, temp0), __altivec_vandc(mask1, temp1));
//	const __vec128 eq     = __altivec_vcmpequb(shift0, shift1);
//	const __vec128 mask   = __altivec_vandc(__altivec_vxor(mask0, mask1), eq);
	return mask;
#elif RSG_CPU_SPU
	const qword splat0 = (qword)(vector unsigned int){0x00010203, 0x00010203, 0x00010203, 0x00010203};
	const qword splat1 = (qword)(vector unsigned int){0x04050607, 0x04050607, 0x04050607, 0x04050607};
	const qword ones   = si_il(-1);
	const qword shift  = si_sfi((qword)boundsInt, 128); // 128-i, so we can shift left instead of right
	const qword shift0 = si_shufb(shift, shift, splat0);
	const qword shift1 = si_shufb(shift, shift, splat1);
	const qword mask0  = si_shlqbi(si_shlqbybi(ones, shift0), shift0);
	const qword mask1  = si_shlqbi(si_shlqbybi(ones, shift1), shift1);
	const qword mask   = si_xor(mask0, mask1);
	return (Vector_4V)mask;
#elif RSG_CPU_INTEL
	const __m128i ones  = _mm_set1_epi32(~0);
	const __m128i mask0 = _mm_srli_si128_bits(ones, GetXi(boundsInt));
	const __m128i mask1 = _mm_srli_si128_bits(ones, GetYi(boundsInt));
	const __m128i mask  = _mm_xor_si128(mask0, mask1);
	return _mm_castsi128_ps(mask);
#endif // RSG_CPU_INTEL
}

#if RSG_CPU_PPC

__forceinline Vector_4V_Out V4ShiftLeftBits128(Vector_4V_In v, Vector_4V_In shift)
{
	return __altivec_vsl(__altivec_vslo(v, shift), shift);
}

__forceinline Vector_4V_Out V4ShiftRightBits128(Vector_4V_In v, Vector_4V_In shift)
{
	return __altivec_vsr(__altivec_vsro(v, shift), shift);
}

#endif // RSG_CPU_PPC

#if RSG_CPU_PPC || RSG_CPU_INTEL

__forceinline Vector_4V_Out V4ShiftLeftBits128(Vector_4V_In v, u32 n)
{
#if RSG_CPU_PPC
	return V4ShiftLeftBits128(v, __altivec_vspltb(__altivec_lvlx(&n, 0), 3));
#elif RSG_CPU_SPU
	#error "not yet implemented"
#elif RSG_CPU_INTEL
	return _mm_castsi128_ps(_mm_slli_si128_bits(_mm_castps_si128(v), (int)n));
#endif // RSG_CPU_INTEL
}

__forceinline Vector_4V_Out V4ShiftRightBits128(Vector_4V_In v, u32 n)
{
#if RSG_CPU_PPC
	return V4ShiftRightBits128(v, __altivec_vspltb(__altivec_lvlx(&n, 0), 3));
#elif RSG_CPU_SPU
	#error "not yet implemented"
#elif RSG_CPU_INTEL
	return _mm_castsi128_ps(_mm_srli_si128_bits(_mm_castps_si128(v), (int)n));
#endif // RSG_CPU_INTEL
}

__forceinline Vector_4V_Out V4ReverseBits128(Vector_4V_In v)
{
#if RSG_CPU_PPC
	const __vec128 shift1 = __altivec_vspltisb(1);
	const __vec128 shift2 = __altivec_vspltisb(2);
	const __vec128 shift4 = __altivec_vspltisb(4);
	const __vec128 mask1  = V4VConstantSplat<0x55555555>();
	const __vec128 mask2  = V4VConstantSplat<0x33333333>();
	const __vec128 rev    = V4VConstant<0x0f0e0d0c,0x0b0a0908,0x07060504,0x03020100>();

	__vec128 temp = v;

	// http://graphics.stanford.edu/~seander/bithacks.html
	// swap odd and even bits
	temp = __altivec_vsel(__altivec_vslb(temp, shift1), __altivec_vsrb(temp, shift1), mask1);
	// swap consecutive pairs
	temp = __altivec_vsel(__altivec_vslb(temp, shift2), __altivec_vsrb(temp, shift2), mask2);
	// swap nibbles
	temp = __altivec_vor(__altivec_vslb(temp, shift4), __altivec_vsrb(temp, shift4));
	// reverse order of bytes
	temp = __altivec_vperm(temp, temp, rev);

	return temp;
#elif RSG_CPU_SPU
	#error "not yet implemented"
#elif RSG_CPU_INTEL
	(void)v;
	return _mm_setzero_ps(); // TODO -- implement this if we need it
#endif // RSG_CPU_INTEL
}

__forceinline u32 V4CountLeadingZeros128(Vector_4V_In v)
{
#if RSG_CPU_PPC
	const u32 z0 = __ppc_cntlzd(((const u64*)&v)[0]);
	const u32 z1 = __ppc_cntlzd(((const u64*)&v)[1]);
	return z0 + (z0 == 64 ? z1 : 0);
#elif RSG_CPU_SPU
	#error "not yet implemented"
#elif RSG_CPU_INTEL
	unsigned long z0; u32 z0_nonzero = (u32)_BitScanReverse64(&z0, ((const u64*)&v)[0]);
	unsigned long z1; u32 z1_nonzero = (u32)_BitScanReverse64(&z1, ((const u64*)&v)[1]);
	z0 = 64 - z0 - z0_nonzero;
	z1 = 64 - z1 - z1_nonzero;
	return z0 + (z0 == 64 ? z1 : 0);
#endif // RSG_CPU_INTEL
}

__forceinline u32 V4CountTrailingZeros128(Vector_4V_In v)
{
#if RSG_CPU_PPC || RSG_CPU_SPU
	return V4CountLeadingZeros128(V4ReverseBits128(v));
#elif RSG_CPU_INTEL
	unsigned long z0; u32 z0_nonzero = (u32)_BitScanForward64(&z0, ((const u64*)&v)[0]);
	unsigned long z1; u32 z1_nonzero = (u32)_BitScanForward64(&z1, ((const u64*)&v)[1]);
	z0 = z0_nonzero ? z0 : 64;
	z1 = z1_nonzero ? z1 : 64;
	return z1 + (z1 == 64 ? z0 : 0);
#endif // RSG_CPU_INTEL
}

#endif // RSG_CPU_PPC || RSG_CPU_INTEL

} // namespace Vec

#if RSG_CPU_PPC
template <u32 bytes> __forceinline Vec4V_Out    ShiftLeftBytesDouble(Vec4V_In    a, Vec4V_In    b) { return Vec4V   (Vec::V4ShiftLeftBytesDouble<bytes>(a.GetIntrin128(), b.GetIntrin128())); }
template <u32 bytes> __forceinline VecBoolV_Out ShiftLeftBytesDouble(VecBoolV_In a, VecBoolV_In b) { return VecBoolV(Vec::V4ShiftLeftBytesDouble<bytes>(a.GetIntrin128(), b.GetIntrin128())); }
#endif // RSG_CPU_PPC

__forceinline Vec4V_Out ShiftLeft          (Vec4V_In a, Vec4V_In shift) { return Vec4V(Vec::V4ShiftLeft          (a.GetIntrin128(), shift.GetIntrin128())); }
__forceinline Vec4V_Out ShiftRight         (Vec4V_In a, Vec4V_In shift) { return Vec4V(Vec::V4ShiftRight         (a.GetIntrin128(), shift.GetIntrin128())); }
__forceinline Vec4V_Out ShiftRightAlgebraic(Vec4V_In a, Vec4V_In shift) { return Vec4V(Vec::V4ShiftRightAlgebraic(a.GetIntrin128(), shift.GetIntrin128())); }

__forceinline Vec3V_Out ShiftLeft          (Vec3V_In a, Vec3V_In shift) { return Vec3V(Vec::V4ShiftLeft          (a.GetIntrin128(), shift.GetIntrin128())); }
__forceinline Vec3V_Out ShiftRight         (Vec3V_In a, Vec3V_In shift) { return Vec3V(Vec::V4ShiftRight         (a.GetIntrin128(), shift.GetIntrin128())); }
__forceinline Vec3V_Out ShiftRightAlgebraic(Vec3V_In a, Vec3V_In shift) { return Vec3V(Vec::V4ShiftRightAlgebraic(a.GetIntrin128(), shift.GetIntrin128())); }

__forceinline Vec2V_Out ShiftLeft          (Vec2V_In a, Vec2V_In shift) { return Vec2V(Vec::V4ShiftLeft          (a.GetIntrin128(), shift.GetIntrin128())); }
__forceinline Vec2V_Out ShiftRight         (Vec2V_In a, Vec2V_In shift) { return Vec2V(Vec::V4ShiftRight         (a.GetIntrin128(), shift.GetIntrin128())); }
__forceinline Vec2V_Out ShiftRightAlgebraic(Vec2V_In a, Vec2V_In shift) { return Vec2V(Vec::V4ShiftRightAlgebraic(a.GetIntrin128(), shift.GetIntrin128())); }

__forceinline ScalarV_Out ShiftLeft          (ScalarV_In a, ScalarV_In shift) { return ScalarV(Vec::V4ShiftLeft          (a.GetIntrin128(), shift.GetIntrin128())); }
__forceinline ScalarV_Out ShiftRight         (ScalarV_In a, ScalarV_In shift) { return ScalarV(Vec::V4ShiftRight         (a.GetIntrin128(), shift.GetIntrin128())); }
__forceinline ScalarV_Out ShiftRightAlgebraic(ScalarV_In a, ScalarV_In shift) { return ScalarV(Vec::V4ShiftRightAlgebraic(a.GetIntrin128(), shift.GetIntrin128())); }

__forceinline u32 GetSignMask(Vec4V_In    v) { return Vec::V4GetSignMask(v.GetIntrin128()); }
__forceinline u32 GetBoolMask(VecBoolV_In v) { return Vec::V4GetBoolMask(v.GetIntrin128()); }

__forceinline Vec4V_Out GenerateMaskShiftLeft128 (Vec4V_In boundsInt) { return Vec4V(Vec::V4GenerateMaskShiftLeft128 (boundsInt.GetIntrin128())); }
__forceinline Vec4V_Out GenerateMaskShiftRight128(Vec4V_In boundsInt) { return Vec4V(Vec::V4GenerateMaskShiftRight128(boundsInt.GetIntrin128())); }

#if RSG_CPU_PPC
__forceinline Vec4V_Out ShiftLeftBits128 (Vec4V_In v, Vec4V_In shift) { return Vec4V(Vec::V4ShiftLeftBits128 (v.GetIntrin128(), shift.GetIntrin128())); }
__forceinline Vec4V_Out ShiftRightBits128(Vec4V_In v, Vec4V_In shift) { return Vec4V(Vec::V4ShiftRightBits128(v.GetIntrin128(), shift.GetIntrin128())); }
#endif // RSG_CPU_PPC

#if RSG_CPU_PPC || RSG_CPU_INTEL
__forceinline Vec4V_Out ShiftLeftBits128 (Vec4V_In v, u32 n) { return Vec4V(Vec::V4ShiftLeftBits128 (v.GetIntrin128(), n)); }
__forceinline Vec4V_Out ShiftRightBits128(Vec4V_In v, u32 n) { return Vec4V(Vec::V4ShiftRightBits128(v.GetIntrin128(), n)); }
__forceinline Vec4V_Out ReverseBits128(Vec4V_In v) { return Vec4V(Vec::V4ReverseBits128(v.GetIntrin128())); }
__forceinline u32 CountLeadingZeros128(Vec4V_In v) { return Vec::V4CountLeadingZeros128(v.GetIntrin128()); }
__forceinline u32 CountTrailingZeros128(Vec4V_In v) { return Vec::V4CountTrailingZeros128(v.GetIntrin128()); }
#endif // RSG_CPU_PPC || RSG_CPU_INTEL

__forceinline u64 ClampedShiftLeft64(u64 x, int n)
{
	if      (n <= 0) { return x; }
	else if (n < 64) { return x << n; }
	else             { return 0; }
}

__forceinline u64 ClampedShiftRight64(u64 x, int n)
{
	if      (n <= 0) { return x; }
	else if (n < 64) { return x >> n; }
	else             { return 0; }
}

#if RSG_DEV && RSG_ASSERT
void GenerateMask128_TEST();
#endif // RSG_DEV && RSG_ASSERT

} // namespace rage

#endif	// _MATH_VECSHIFT_H_
