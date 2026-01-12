// 
// math/float16.inl
// 
// Copyright (C) 2010-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "system/endian.h"

/*
[SSE TODO] -- float16 general TODO ..
- make use of and 4-component pack/unpack (not just 8-component)
	consider removing all the PackIntoX, UnpackFromZW etc. methods, do we really need these?
	might be better to just have functions to pack/unpack to and from memory directly

// other misc stuff
[SSE TODO] -- use _mm_storeu_si32 to store unaligned 32-bit value from a vector (if we can't use _mm_store_ss?)
[SSE TODO] -- use _mm_storeu_si64 to store unaligned 64-bit value from a vector (if we can't use _mm_storel_epi64?)
*/

namespace rage {

#if RSG_CPU_INTEL

// SSE utilities
#if RSG_SSE_VERSION < 41
#define _mm_packus_epi32(a,b) _mm_castps_si128(Vec::V4PackSignedIntToUnsignedShort(_mm_castsi128_ps(a), _mm_castsi128_ps(b)))
#endif // RSG_SSE_VERSION < 41

__forceinline __m128i _mm_packhi_epi32(__m128i a, __m128i b)
{
	return _mm_packus_epi32(_mm_srli_epi32(a, 16), _mm_srli_epi32(b, 16));
}

__forceinline __m128i _mm_packlo_epi32(__m128i a, __m128i b)
{
	const __m128i kV_0x0000ffff = _mm_set1_epi32(0x0000ffffu);
	return _mm_packus_epi32(_mm_and_si128(a, kV_0x0000ffff), _mm_and_si128(b, kV_0x0000ffff));
}

__forceinline __m128i _mm_select_si128(__m128i a, __m128i b, __m128i mask)
{
#if 1
	// xor-version has better register allocation (http://markplusplus.wordpress.com/2007/03/14/fast-sse-select-operation/)
	return _mm_xor_si128(a, _mm_and_si128(mask, _mm_xor_si128(a, b)));
#else
	return _mm_or_si128(_mm_andnot_si128(mask, a), _mm_and_si128(mask, b));
#endif
}

#endif // RSG_CPU_INTEL

__forceinline void Float16::SetFloat16_FromFloat32(f32 f) // [seeeeemm.mmmmmmmm] <- [seeeeeee.emmmmmmm.mmmmmmmm.mmmmmmmm]
{
#if 1 && RSG_XENON // from XMConvertFloatToHalf

	Vec::Vector_4V temp;

	temp = __lvlx(&f, 0);
    temp = __vpkd3d(temp, temp, VPACK_FLOAT16_4, VPACK_64LO, 2);
    temp = __vsplth(temp, 0);

    __stvehx(temp, &m_data, 0);

#elif RSG_SSE_VERSION >= 50

#if RSG_DURANGO // [SSE TODO] -- workaround for internal compiler error, wtf
	m_data = _mm_cvtps_ph(_mm_load_ss(&f), _MM_FROUND_TO_ZERO).m128i_u16[0];
#else
	m_data = (u16)_mm_extract_epi16(_mm_cvtps_ph(_mm_load_ss(&f), _MM_FROUND_TO_ZERO), 0);
#endif

#else

	SetFloat16_FromFloat32_NoIntrinsics(f);

#endif
}

__forceinline void Float16::SetFloat16_FromFloat32_NoIntrinsics(f32 f) // [seeeeemm.mmmmmmmm] <- [seeeeeee.emmmmmmm.mmmmmmmm.mmmmmmmm]
{
	u32 i,e,l,o;

	i = *(u32*)&f; // [LHS]
	e = 0x7fffffffu & i;
	e = 0x38000000u - e;
	l = (s32)(0x00000000u + e) >> 31; // underflow
	o = (s32)(0x0fffffffu + e) >> 31; // overflow
	e = (u32)(-(s32)e) >> 13;
	e = e | o;
	e = e & ((0x00007fffu & l));
	e = e | ((0x80000000u & i) >> 16);

	m_data = (u16)e;
}

//#if !__FINAL
// Similar to SetFloat16_FromFloat32_NoIntrinsics, but IEEE754 float nearest style rounding is applied, where ties are rounded towards even.
// As well as the usual argument for breaking ties by rounding towards even reducing the bias towards larger magnitude numbers, it is
// important in the special case of texcoords.  For texcoords we want to ensure that values used for wrapped addressing stay continous mod 1.
// For example say we were rounding to one decimal place (just to keep the example simple), and two vertices had the texcoords
//      -1.25 and 1.75
// even tie break rounds these to
//      -1.2  and 1.8
// so they are still the same value mod 1, keeping the texcoord wrapping continous
//
__forceinline void Float16::SetFloat16_FromFloat32_RndNearest(f32 f)
{
	u32 i,e,l,o,r0,r1;

	i  = *(u32*)&f; // [LHS]
	e  = 0x7fffffffu & i;
	r0 = (0x00001000u & e) << 1;            // rounding bit
	r0 = r0 & e;                            // mask to that ties round to even
	r1 = (0x00001fffu & e) + 0x00000fffu;   // ensure greater than ties round away from zero
	r0 = r0 | (r1 & 0x00002000u);
	e  = e + r0;
	e  = 0x38000000u - e;
	l  = (s32)(0x00000000u + e) >> 31;      // underflow
	o  = (s32)(0x0fffffffu + e) >> 31;      // overflow
	e  = (u32)(-(s32)e) >> 13;
	e  = e | o;
	e  = e & ((0x00007fffu & l));
	e  = e | ((0x80000000u & i) >> 16);

	m_data = (u16)e;
}
//#endif // !__FINAL

__forceinline void Float16::SetFloat16_FromFloat32(ScalarV_In f)
{
#if 1 && RSG_XENON // from XMConvertFloatToHalf

	Vec::Vector_4V temp;

    temp = __vpkd3d(f.GetIntrin128(), f.GetIntrin128(), VPACK_FLOAT16_4, VPACK_64LO, 2);
    temp = __vsplth(temp, 0);

    __stvehx(temp, &m_data, 0);

#elif RSG_SSE_VERSION >= 50

#if RSG_DURANGO && !(__DEV || __PROFILE) // [SSE TODO] -- workaround for internal compiler error, wtf (ok in debug, beta and profile though)
	m_data = _mm_cvtps_ph(f.GetIntrin128(), _MM_FROUND_TO_ZERO).m128i_u16[0];
#else
	m_data = (u16)_mm_extract_epi16(_mm_cvtps_ph(f.GetIntrin128(), _MM_FROUND_TO_ZERO), 0);
#endif

#else

	float lhs;
	Vec::V4StoreScalar32FromSplatted(lhs, f.GetIntrin128());
	SetFloat16_FromFloat32_NoIntrinsics(lhs);

#endif
}

__forceinline f32 Float16::GetFloat32_FromFloat16() const // [seeeeemm.mmmmmmmm] -> [seeeeeee.emmmmmmm.mmm00000.00000000]
{
#if 1 && RSG_XENON

	Vec::Vector_4V temp = GetFloat32_FromFloat16_ScalarV().GetIntrin128();
	float f;

	__stvewx(temp, &f, 0);

	return f;

#elif RSG_SSE_VERSION >= 50

	const __m128i temp = _mm_insert_epi16(_mm_undefined_si128(), (int)m_data, 0);
	const __m128 temp2 = _mm_cvtph_ps(temp);

#if RSG_DURANGO	// [SSE TODO] -- B*1751890 Temporary work around for the _mm_cvtss_f32 compiler bug.
	return temp2.m128_f32[0];
#else
	return _mm_cvtss_f32(temp2);
#endif

#else

	return GetFloat32_FromFloat16_NoIntrinsics();

#endif
}

__forceinline f32 Float16::GetFloat32_FromFloat16_NoIntrinsics() const // [seeeeemm.mmmmmmmm] -> [seeeeeee.emmmmmmm.mmm00000.00000000]
{
	u32 e,z,s;

	e = (u32)m_data << 16; // [seeeeemm.mmmmmmmm.00000000.00000000]
	s = 0x80000000u & e;   // [s0000000.00000000.00000000.00000000]
	e = 0x7fff0000u & e;   // [0eeeeemm.mmmmmmmm.00000000.00000000]
	z = GetNonZeroMask(e); // all 1's if e!=0
	z = 0x38000000u & z;
	e = ((e >> 3) + z)|s;

	return *(f32*)&e; // [LHS]
}

__forceinline ScalarV_Out Float16::GetFloat32_FromFloat16_ScalarV() const // [seeeeemm.mmmmmmmm] -> [seeeeeee.emmmmmmm.mmm00000.00000000]
{
#if 1 && RSG_XENON // from XMConvertHalfToFloat

	Vec::Vector_4V temp;

	temp = __lvlx(&m_data, 0);
	temp = __vsplth(temp, 0);
	temp = __vupkd3d(temp, VPACK_FLOAT16_4);
	temp = __vspltw(temp, 0);

	return ScalarV(temp);

#elif RSG_SSE_VERSION >= 50

	const __m128i temp = _mm_insert_epi16(_mm_undefined_si128(), (int)m_data, 0);
	const __m128 temp2 = _mm_cvtph_ps(temp);
	return ScalarV(_mm_shuffle_ps(temp2, temp2, 0x00));

#else

	using namespace Vec;
#	if !__BE
	// Handle little endian.  Casting a u16 pointer to a float has data in lower 16 bits.
	Vector_4V vec   = V4LoadLeft(&m_data);                              // [????????.????????.seeeeemm.mmmmmmmm]
	vec             = V4ShiftLeft<16>(vec);                             // [seeeeemm.mmmmmmmm.????????.????????]
#	else
	Vector_4V vec   = V4LoadLeft(&m_data);                              // [seeeeemm.mmmmmmmm.????????.????????]
#	endif
	Vector_4V sgn   = V4And(vec, V4VConstantSplat<0x80000000>());       // [s0000000.00000000.00000000.00000000]
	Vector_4V exp   = V4And(vec, V4VConstantSplat<0x7fff0000>());       // [0eeeeemm.mmmmmmmm.00000000.00000000]
	Vector_4V zero  = V4IsEqualIntV(vec, V4VConstant(V_ZERO));          // all 1's if e==0
	Vector_4V bias  = V4Andc(V4VConstantSplat<0x38000000>(), zero);
	Vector_4V flt   = V4Or(V4AddInt(V4ShiftRight<3>(exp), bias), sgn);
	Vector_4V splat = V4SplatX(flt);
	return ScalarV(splat);

#endif
}

namespace sysEndian
{	
	template<> inline void SwapMe(Float16& f)
	{
		f.SetBinaryData(Swap(f.GetBinaryData()));
	}
} // namespace sysEndian

// ================================================================================================

// reference ..
// X:\ps3sdk\dev\usr\local\340_001\cell\target\spu\include\vmx2spu_gcc.h

//#define si_sllhi(a,i) si_shlhi  (a, i) // shift left  logical    halfword immediate
//#define si_srlhi(a,i) si_rothmi (a,-i) // shift right logical    halfword immediate
//#define si_srahi(a,i) si_rotmahi(a,-i) // shift right arithmetic halfword immediate

// by comparison, Sony's SPU implementations do not seem very good .. see \cell\target\spu\include\vectormath\cpp\vec_aos.h
//
//static inline vec_uint4   _vmathVfToHalfFloatsUnpacked (vec_float4  v)
//static inline vec_ushort8 _vmath2VfToHalfFloats        (vec_float4  u, vec_float4 v)
//static inline vec_float4  _vmathHalfFloatsToVfUnpacked (vec_uint4   v)
//static inline vec_uint4   _vmathHalfFloatsToVfUnpacked (vec_ushort8 v)
//static inline void        _vmath2HalfFloatsToVfUnpacked(vec_float4 & vec0, vec_float4 & vec1, vec_uint4 vsrc)

#if RSG_CPU_PPC

__forceinline __vec128 __altivec_vpkuhum_hi(__vec128 a, __vec128 b) // this could be replaced by a single __altivec_vperm
{
	return __altivec_vpkuhum(__altivec_vsldoi(a,a,16-1), __altivec_vsldoi(b,b,16-1));
}

__forceinline __vec128 __altivec_vpkuwum_hi(__vec128 a, __vec128 b) // this could be replaced by a single __altivec_vperm
{
	return __altivec_vpkuwum(__altivec_vsldoi(a,a,16-2), __altivec_vsldoi(b,b,16-2));
}

#endif // RSG_CPU_PPC

namespace Vec {

__forceinline Vector_4V_Out V4Float16Vec8Pack(Vector_4V_In a, Vector_4V_In b)
{
#if RSG_XENON

	Vector_4V temp0 = a;
	Vector_4V temp1 = b;

	temp0 = __altivec_XENON_vpkd3d(temp0, temp0, VPACK_FLOAT16_4, VPACK_64LO, 2);
	temp0 = __altivec_XENON_vpkd3d(temp0, temp1, VPACK_FLOAT16_4, VPACK_64LO, 0);

	return temp0;

#elif RSG_CPU_PPC

	const Vector_4V kV_0x0003 = __altivec_vspltish      (0x0003u); // shift control
	const Vector_4V kV_0x000d = __altivec_vspltish      (0x000du); // shift control
	const Vector_4V kV_0x7fff = __altivec_const_u16_vec8(0x7fffu);
	const Vector_4V kV_0x0fff = __altivec_const_u16_vec8(0x0fffu);
	const Vector_4V kV_0x3800 = __altivec_const_u16_vec8(0x3800u);

	Vector_4V s,e,m,u,o;

	m = __altivec_vpkuwum   (a, b);            // m=[mmmmmmmm.mmmmmmmm.mmmmmmmm.mmmmmmmm] // low 16-bits
	s = __altivec_vpkuwum_hi(a, b);            // s=[seeeeeee.emmmmmmm.seeeeeee.emmmmmmm] // high 16-bits, with sign
	m = __altivec_vsrh      (m, kV_0x000d);    // m=[00000000.00000mmm.00000000.00000mmm]
	e = __altivec_vand      (s, kV_0x7fff);    // e=[0eeeeeee.emmmmmmm.0eeeeeee.emmmmmmm] // high 16-bits, no sign
	u = __altivec_vcmpltuh  (e, kV_0x3800);    // u= e < 0x3800 ? 0xffff : 0x0000
	e = __altivec_vsubuhm   (e, kV_0x3800);    // e=[0eeeeeee.emmmmmmm.0eeeeeee.emmmmmmm] // biased ..
	o = __altivec_vcmpgtuh  (e, kV_0x0fff);    // o= e > 0x0fff ? 0xffff : 0x0000
	e = __altivec_vslh      (e, kV_0x0003);    // e=[eeeeeemm.mmmmm000.eeeeeemm.mmmmm000]
	e = __altivec_vor       (e, m);            // e=[eeeeeemm.mmmmmmmm.eeeeeemm.mmmmmmmm]
	e = __altivec_vor       (e, o);            // e=
	e = __altivec_vandc     (e, u);            // e=
	e = __altivec_vsel      (s, e, kV_0x7fff); // e=[seeeeemm.mmmmmmmm.seeeeemm.mmmmmmmm]

	return e;

#elif RSG_CPU_SPU

	const qword kV_shufb_vpkuwum_hi = (qword)(vec_ushort8){ 0x0001, 0x0405, 0x0809, 0x0c0d, 0x1011, 0x1415, 0x1819, 0x1c1d };
	const qword kV_shufb_vpkuwum_lo = (qword)(vec_ushort8){ 0x0203, 0x0607, 0x0a0b, 0x0e0f, 0x1213, 0x1617, 0x1a1b, 0x1e1f };
	const qword kV_0x7fff           = (qword)(vec_ushort8){ 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff, 0x7fff };
	const qword kV_0x0fff           = (qword)(vec_ushort8){ 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff, 0x0fff };
	const qword kV_0x3800           = (qword)(vec_ushort8){ 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800 };

	qword s,e,m,u,o;

	m = si_shufb ((qword)a, (qword)b, kV_shufb_vpkuwum_lo); // m=[mmmmmmmm.mmmmmmmm.mmmmmmmm.mmmmmmmm] // low 16-bits
	s = si_shufb ((qword)a, (qword)b, kV_shufb_vpkuwum_hi); // s=[seeeeeee.emmmmmmm.seeeeeee.emmmmmmm] // high 16-bits, with sign
	m = si_rothmi(m, -13);                                  // m=[00000000.00000mmm.00000000.00000mmm]
	e = si_and   (s, kV_0x7fff);                            // e=[0eeeeeee.emmmmmmm.0eeeeeee.emmmmmmm] // high 16-bits, no sign
	u = si_clgth (kV_0x3800, e);                            // u= e < 0x3800 ? 0xffff : 0x0000
	e = si_sfh   (kV_0x3800, e);                            // e=[0eeeeeee.emmmmmmm.0eeeeeee.emmmmmmm] // biased ..
	o = si_clgth (e, kV_0x0fff);                            // o= e > 0x0fff ? 0xffff : 0x0000
	e = si_shlhi (e, 3);                                    // e=[eeeeeemm.mmmmm000.eeeeeemm.mmmmm000]
	e = si_or    (e, m);                                    // e=[eeeeeemm.mmmmmmmm.eeeeeemm.mmmmmmmm]
	e = si_or    (e, o);                                    // e=
	e = si_andc  (e, u);                                    // e=
	e = si_selb  (s, e, kV_0x7fff);                         // e=[seeeeemm.mmmmmmmm.seeeeemm.mmmmmmmm]

	return (Vector_4V)e;

#elif __PSP2

	float16x4_t a16 = vcvt_f16_f32((float32x4_t&)a);
	float16x4_t b16 = vcvt_f16_f32((float32x4_t&)b);
	float16x8_t combined = vcombine_f16(a16,b16);
	return vreinterpretq_f32_f16(combined);

#elif RSG_SSE_VERSION >= 50

	const __m256 ab = _mm256_insertf128_ps(_mm256_castps128_ps256(a), b, 1);

	return _mm_castsi128_ps(_mm256_cvtps_ph(ab, _MM_FROUND_TO_ZERO));

#elif RSG_CPU_INTEL

	const __m128i kV_0x7fff = _mm_set1_epi16(0x7fffu);
	const __m128i kV_0x0fff = _mm_set1_epi16(0x0fffu);
	const __m128i kV_0x3800 = _mm_set1_epi16(0x3800u);

	const __m128i x = _mm_castps_si128(a);
	const __m128i y = _mm_castps_si128(b);

	__m128i s,e,m,u,o;

	m = _mm_packlo_epi32(x, y);            // m=[mmmmmmmm.mmmmmmmm.mmmmmmmm.mmmmmmmm] // low 16-bits
	s = _mm_packhi_epi32(x, y);            // s=[seeeeeee.emmmmmmm.seeeeeee.emmmmmmm] // high 16-bits, with sign
	m = _mm_srli_epi16  (m, 13);           // m=[00000000.00000mmm.00000000.00000mmm]
	e = _mm_and_si128   (s, kV_0x7fff);    // e=[0eeeeeee.emmmmmmm.0eeeeeee.emmmmmmm] // high 16-bits, no sign
	u = _mm_cmplt_epi16 (e, kV_0x3800);    // u= e < 0x3800 ? 0xffff : 0x0000
	e = _mm_sub_epi16   (e, kV_0x3800);    // e=[0eeeeeee.emmmmmmm.0eeeeeee.emmmmmmm] // biased ..
	o = _mm_cmpgt_epi16 (e, kV_0x0fff);    // o= e > 0x0fff ? 0xffff : 0x0000
	e = _mm_slli_epi16  (e, 3);            // e=[eeeeeemm.mmmmm000.eeeeeemm.mmmmm000]
	e = _mm_or_si128    (e, m);            // e=[eeeeeemm.mmmmmmmm.eeeeeemm.mmmmmmmm]
	e = _mm_or_si128    (e, o);            // e=
	e = _mm_andnot_si128(u, e);            // e=
	e = _mm_select_si128(s, e, kV_0x7fff); // e=[seeeeemm.mmmmmmmm.seeeeemm.mmmmmmmm]	

	return _mm_castsi128_ps(e);

#else

	union { Vector_4V v; u16 v_u16[8]; } un;
	
	un.v_u16[0] = Float16(((const float*)&a)[0]).GetBinaryData();
	un.v_u16[1] = Float16(((const float*)&a)[1]).GetBinaryData();
	un.v_u16[2] = Float16(((const float*)&a)[2]).GetBinaryData();
	un.v_u16[3] = Float16(((const float*)&a)[3]).GetBinaryData();

	un.v_u16[4] = Float16(((const float*)&b)[0]).GetBinaryData();
	un.v_u16[5] = Float16(((const float*)&b)[1]).GetBinaryData();
	un.v_u16[6] = Float16(((const float*)&b)[2]).GetBinaryData();
	un.v_u16[7] = Float16(((const float*)&b)[3]).GetBinaryData();

	return un.v;

#endif
}

#if RSG_SSE_VERSION < 41
#undef _mm_packus_epi32
#endif

#if RSG_CPU_SPU

// http://www.insomniacgames.com/research_dev/articles/2007/1500725
// SimdFloatToHalf: slightly faster but does not clamp values greater than the max representable
// float16, instead it 'wraps' them .. actually it would be more efficient to clamp the incoming
// vector using vec_min on PPU i think? anyway this function should be cleaned up at some point

__forceinline Vector_4V_Out V4Float16Vec8Pack_INSOMNIAC(Vector_4V_In a_, Vector_4V_In b_)
{
	const qword shuf_hi16 = (qword)(vec_ushort8){ 0x0001, 0x0405, 0x0809, 0x0c0d, 0x1011, 0x1415, 0x1819, 0x1c1d };
	const qword s_mask    = (qword)(vec_ushort8){ 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000 };
	const qword blah      = (qword)(vec_ushort8){ 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000 };
	const qword exp_off   = (qword)(vec_ushort8){ 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800 };

	const qword a = (qword)a_;
	const qword b = (qword)b_;

	qword a_rot5 = si_shlqbii(a,       3                );
	qword b_rot5 = si_shlqbii(b,       3                );
	qword mid    = si_shufb  (a_rot5,  b_rot5, shuf_hi16); // mid = | e5  e4  e3  e2  e1  e0  m23 m22 m21 m20 m19 m18 m17 m16 m15 m14 | 
	qword hi     = si_shufb  (a,       b,      shuf_hi16); // hi  = |  s  e7  e6  e5  e4  e3  e2  e1  e0  m23 m22 m21 m20 m19 m18 m17 | 
	qword h      = si_xor    (mid,     blah             ); // h   = | e5  e4' e3  e2  e1  e0  m23 m22 m21 m20 m19 m18 m17 m16 m15 m14 | 
	qword e      = si_andc   (hi,      s_mask           ); // e   = |  0  e7  e6  e5  e4  e3  e2  e1  e0  m23 m22 m21 m20 m19 m18 m17 | 
	h            = si_selb   (h,       hi,     s_mask   ); // h   = |  s  e4' e3  e2  e1  e0  m23 m22 m21 m20 m19 m18 m17 m16 m15 m14 | 
	qword d_mask = si_cgth   (exp_off, e                );
	h            = si_andc   (h,       d_mask           ); // set to 0 if underflow                                                     

	return (Vector_4V)h;
}

#else // not RSG_CPU_SPU

__forceinline Vector_4V_Out V4Float16Vec8Pack_INSOMNIAC(Vector_4V_In a, Vector_4V_In b)
{
	return V4Float16Vec8Pack(a, b);
}

#endif // not RSG_CPU_SPU

// these can probably be optimised!
__forceinline Vector_4V_Out V4Float16Vec4PackIntoXY(Vector_4V_In v, Vector_4V_In src) { return V4PermuteTwo<X2,Y2,Z1,W1>(v, V4Float16Vec8Pack(src, src)); }
__forceinline Vector_4V_Out V4Float16Vec4PackIntoZW(Vector_4V_In v, Vector_4V_In src) { return V4PermuteTwo<X1,Y1,X2,Y2>(v, V4Float16Vec8Pack(src, src)); }

__forceinline Vector_4V_Out V4Float16Vec2PackIntoX(Vector_4V_In v, Vector_4V_In src) { return V4PermuteTwo<X2,Y1,Z1,W1>(v, V4Float16Vec8Pack(src, src)); }
__forceinline Vector_4V_Out V4Float16Vec2PackIntoY(Vector_4V_In v, Vector_4V_In src) { return V4PermuteTwo<X1,X2,Z1,W1>(v, V4Float16Vec8Pack(src, src)); }
__forceinline Vector_4V_Out V4Float16Vec2PackIntoZ(Vector_4V_In v, Vector_4V_In src) { return V4PermuteTwo<X1,Y1,X2,W1>(v, V4Float16Vec8Pack(src, src)); }
__forceinline Vector_4V_Out V4Float16Vec2PackIntoW(Vector_4V_In v, Vector_4V_In src) { return V4PermuteTwo<X1,Y1,Z1,X2>(v, V4Float16Vec8Pack(src, src)); }

__forceinline void V4Float16Vec8Unpack(Vector_4V_InOut a, Vector_4V_InOut b, Vector_4V_In v)
{
#if RSG_XENON

	const Vector_4V temp0 = v;
	const Vector_4V temp1 = __altivec_vsldoi(v, v, 8);

	b = __altivec_XENON_vupkd3d(temp0, VPACK_FLOAT16_4);
	a = __altivec_XENON_vupkd3d(temp1, VPACK_FLOAT16_4);

#elif RSG_CPU_PPC

	const Vector_4V kV_0x0001 = __altivec_vspltish      (0x0001u); // shift control
	const Vector_4V kV_0x0003 = __altivec_vspltish      (0x0003u); // shift control
	const Vector_4V kV_0x000d = __altivec_vspltish      (0x000du); // shift control
	const Vector_4V kV_0x0000 = __altivec_vspltish      (0x0000u);
	const Vector_4V kV_0x8fff = __altivec_const_u16_vec8(0x8fffu);
	const Vector_4V kV_0x3800 = __altivec_const_u16_vec8(0x3800u);

	Vector_4V z,e,m;

	z = __altivec_vslh    (v, kV_0x0001); // z=[eeeeemmm.mmmmmmm0.eeeeemmm.mmmmmmm0]
	e = __altivec_vsrah   (v, kV_0x0003); // e=[sssseeee.emmmmmmm.sssseeee.emmmmmmm]
	m = __altivec_vslh    (v, kV_0x000d); // m=[mmm00000.00000000.mmm00000.00000000]
	z = __altivec_vcmpequh(kV_0x0000, z); // if (m|e) == 0, result will be 0xffff
	z = __altivec_vandc   (kV_0x3800, z); // if (m|e) != 0, result will be 0x3800 = (127-15)<<7
	e = __altivec_vand    (kV_0x8fff, e); // e=[s000eeee.emmmmmmm.s000eeee.emmmmmmm]
	e = __altivec_vadduhm (e, z);         // 
	a = __altivec_vmrghh  (e, m);         // a=[seeeeeee.emmmmmmm.mmm00000.00000000]
	b = __altivec_vmrglh  (e, m);         // b=[seeeeeee.emmmmmmm.mmm00000.00000000]

#elif RSG_CPU_SPU

	const qword kV_shufb_vmrghh = (qword)(vec_ushort8){ 0x0001, 0x1011, 0x0203, 0x1213, 0x0405, 0x1415, 0x0607, 0x1617 };
	const qword kV_shufb_vmrglh = (qword)(vec_ushort8){ 0x0809, 0x1819, 0x0a0b, 0x1a1b, 0x0c0d, 0x1c1d, 0x0e0f, 0x1e1f };
	const qword kV_0x8fff       = (qword)(vec_ushort8){ 0x8fff, 0x8fff, 0x8fff, 0x8fff, 0x8fff, 0x8fff, 0x8fff, 0x8fff };
	const qword kV_0x3800       = (qword)(vec_ushort8){ 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800 };

	qword e,z,m = (qword)v;

	z = si_shlhi  (m,  1);                 // z=[eeeeemmm.mmmmmmm0.eeeeemmm.mmmmmmm0]
	e = si_rotmahi(m, -3);                 // e=[sssseeee.emmmmmmm.sssseeee.emmmmmmm]
	m = si_shlhi  (m, 13);                 // m=[mmm00000.00000000.mmm00000.00000000]
	z = si_ceqhi  (z,  0);                 // if (m|e) == 0, result will be 0xffff
	z = si_andc   (kV_0x3800, z);          // if (m|e) != 0, result will be 0x3800 = (127-15)<<7
	e = si_and    (kV_0x8fff, e);          // e=[s000eeee.emmmmmmm.s000eeee.emmmmmmm]
	e = si_ah     (e, z);                  // e=
	z = si_shufb  (e, m, kV_shufb_vmrghh); // z=[seeeeeee.emmmmmmm.mmm00000.00000000] (a)
	e = si_shufb  (e, m, kV_shufb_vmrglh); // e=[seeeeeee.emmmmmmm.mmm00000.00000000] (b)

	a = (Vector_4V)z;
	b = (Vector_4V)e;

#elif __PSP2

	float16x8_t v16 = vreinterpretq_f16_f32((float32x4_t&)v);
	a = vcvt_f32_f16(vget_low_f16(v16));
	b = vcvt_f32_f16(vget_high_f16(v16));

#elif RSG_SSE_VERSION >= 50

	const __m256 ab = _mm256_cvtph_ps(_mm_castps_si128(v));

	a = _mm256_castps256_ps128(ab);
	b = _mm256_extractf128_ps(ab, 1);

#elif RSG_CPU_INTEL

	const __m128i kV_0x0000 = _mm_setzero_si128();
	const __m128i kV_0x8fff = _mm_set1_epi16(0x8fffu);
	const __m128i kV_0x3800 = _mm_set1_epi16(0x3800u);

	const __m128i x = _mm_castps_si128(v);

	__m128i z,e,m;

	z = _mm_slli_epi16    (x, 1);         // z=[eeeeemmm.mmmmmmm0.eeeeemmm.mmmmmmm0]
	e = _mm_srai_epi16    (x, 3);         // e=[sssseeee.emmmmmmm.sssseeee.emmmmmmm]
	m = _mm_slli_epi16    (x, 13);        // m=[mmm00000.00000000.mmm00000.00000000]
	z = _mm_cmpeq_epi16   (z, kV_0x0000); // if (m|e) == 0, result will be 0xffff
	z = _mm_andnot_si128  (z, kV_0x3800); // if (m|e) != 0, result will be 0x3800 = (127-15)<<7
	e = _mm_and_si128     (e, kV_0x8fff); // e=[s000eeee.emmmmmmm.s000eeee.emmmmmmm]
	e = _mm_add_epi16     (e, z);         // 
	z = _mm_unpacklo_epi16(m, e);         // a=[seeeeeee.emmmmmmm.mmm00000.00000000]
	m = _mm_unpackhi_epi16(m, e);         // b=[seeeeeee.emmmmmmm.mmm00000.00000000]
	a = _mm_castsi128_ps  (z);
	b = _mm_castsi128_ps  (m);

#else

	union { Vector_4V v; u16 v_u16[8]; } un;

	un.v = v;

	const float ax = Float16(un.v_u16[0]).GetFloat32_FromFloat16();
	const float ay = Float16(un.v_u16[1]).GetFloat32_FromFloat16();
	const float az = Float16(un.v_u16[2]).GetFloat32_FromFloat16();
	const float aw = Float16(un.v_u16[3]).GetFloat32_FromFloat16();

	const float bx = Float16(un.v_u16[4]).GetFloat32_FromFloat16();
	const float by = Float16(un.v_u16[5]).GetFloat32_FromFloat16();
	const float bz = Float16(un.v_u16[6]).GetFloat32_FromFloat16();
	const float bw = Float16(un.v_u16[7]).GetFloat32_FromFloat16();

	V4Set(a, ax, ay, az, aw);
	V4Set(b, bx, by, bz, bw);

#endif
}

__forceinline Vector_4V_Out V4Float16Vec4UnpackFromXY(Vector_4V_In src)
{
#if RSG_XENON

	return __altivec_XENON_vupkd3d(__altivec_vsldoi(src, src, 8), VPACK_FLOAT16_4);

#elif __PSP2

	float16x8_t v16 = vreinterpretq_f16_f32((float32x4_t&)src);
	return vcvt_f32_f16(vget_low_f16(v16));

#else

	// [SSE TODO] -- don't bother with all 8 components if we don't need them ..
	Vector_4V a;
	Vector_4V b;
	V4Float16Vec8Unpack(a, b, src);
	return a;

#endif
}

__forceinline Vector_4V_Out V4Float16Vec4UnpackFromZW(Vector_4V_In src)
{
#if RSG_XENON

	return __altivec_XENON_vupkd3d(src, VPACK_FLOAT16_4);

#elif __PSP2

	float16x8_t v16 = vreinterpretq_f16_f32((float32x4_t&)src);
	return vcvt_f32_f16(vget_high_f16(v16));

#else

	// [SSE TODO] -- don't bother with all 8 components if we don't need them ..
	Vector_4V a;
	Vector_4V b;
	V4Float16Vec8Unpack(a, b, src);
	return b;

#endif
}

__forceinline Vector_4V_Out V4Float16Vec2UnpackFromX(Vector_4V_In src) { return V4Float16Vec4UnpackFromXY(        (src)); }
__forceinline Vector_4V_Out V4Float16Vec2UnpackFromY(Vector_4V_In src) { return V4Float16Vec4UnpackFromZW(V4SplatY(src)); } // unpack from zw saves 1 instr. on 360
__forceinline Vector_4V_Out V4Float16Vec2UnpackFromZ(Vector_4V_In src) { return V4Float16Vec4UnpackFromZW(        (src)); }
__forceinline Vector_4V_Out V4Float16Vec2UnpackFromW(Vector_4V_In src) { return V4Float16Vec4UnpackFromZW(V4SplatW(src)); }

// ================================================================================================
// TODO -- these load/store unaligned functions belong in v4vector4v.inl and need to be implemented
//         for all platforms .. then the Float16Vec4 and Float16Vec2 pack/unpack functions can
//         be exposed for all platforms instead of just for altivec
// ================================================================================================

__forceinline Vector_4V_Out V4LoadXY(const float* src); // loads 8 bytes of vector (into xy components)
__forceinline Vector_4V_Out V4LoadZW(const float* src); // loads 8 bytes of vector (into zw components)
__forceinline Vector_4V_Out V4LoadX (const float* src); // loads 4 bytes of vector (into x component)

__forceinline void V4StoreXY(float* dst, Vector_4V_In v); // stores 8 bytes of vector (from xy components)
//__forceinline void V4StoreX(float* dst, Vector_4V_In v); // stores 4 bytes of vector (from x component)

__forceinline Vector_4V_Out V4LoadXY(const float* src) // loads 8 bytes of vector (into xy components)
{
	FastAssert(((size_t)src & 3) == 0); // must be 4-byte aligned (on some platforms)
#if RSG_CPU_PPC // XMLoadFloat2
	const Vector_4V a = __altivec_lvx(src, 0);
	const Vector_4V b = __altivec_lvx(src, 7);
	return __altivec_vperm(a, b, __altivec_lvsl(src, 0));
#elif RSG_CPU_INTEL
	return _mm_castsi128_ps(_mm_loadl_epi64((const __m128i*)src));
#else
	Vector_4V v;
	V4Set(v, src[0], src[1], 0.0f, 0.0f);
	return v;
#endif
}

__forceinline Vector_4V_Out V4LoadZW(const float* src)
{
	FastAssert(((size_t)src & 3) == 0); // must be 4-byte aligned (on some platforms)
#if RSG_CPU_PPC
	const Vector_4V perm = __altivec_vaddubm(__altivec_lvsl(src, 0), __altivec_vspltisb(-8));
	return __altivec_vperm(__altivec_lvx(src, 0), __altivec_lvx(src, 7), perm);
#elif RSG_CPU_INTEL
	const Vector_4V temp = _mm_castsi128_ps(_mm_loadl_epi64((const __m128i*)src));
	return _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1,0,1,0)); // permute X,Y,X,Y
#else
	Vector_4V v;
	V4Set(v, 0.0f, 0.0f, src[0], src[1]);
	return v;
#endif
}

__forceinline Vector_4V_Out V4LoadX(const float* src) // loads lower 4 bytes of vector (into x component)
{
	FastAssert(((size_t)src & 3) == 0); // must be 4-byte aligned (on some platforms)
#if RSG_CPU_PPC // XMLoadFloat
	return __altivec_lvlx(src, 0);
#elif RSG_CPU_INTEL
	return _mm_load_ss(src);
#else
	Vector_4V v;
	V4Set(v, src[0], 0.0f, 0.0f, 0.0f);
	return v;
#endif
}

__forceinline void V4StoreXY(float* dst, Vector_4V_In v) // stores lower 8 bytes of vector (from xy components)
{
	FastAssert(((size_t)dst & 3) == 0); // must be 4-byte aligned (on some platforms)
#if RSG_CPU_PPC // XMStoreFloat2
	__altivec_stvewx(__altivec_vspltw(v, 0), dst, 0);
	__altivec_stvewx(__altivec_vspltw(v, 1), dst, 4);
#elif RSG_CPU_INTEL
	_mm_storel_epi64((__m128i*)dst, _mm_castps_si128(v)); 
#else
	dst[0] = GetX(v);
	dst[1] = GetY(v);
#endif
}

// equivalent to V4StoreScalar32FromSplatted
/*__forceinline void V4StoreX(float* dst, Vector_4V_In v) // stores lower 4 bytes of vector (from x component)
{
	FastAssert(((size_t)dst & 3) == 0); // must be 4-byte aligned (on some platforms)
#if RSG_CPU_PPC // XMStoreFloat
	__altivec_stvewx(__altivec_vspltw(v, 0), dst, 0);
#elif RSG_CPU_INTEL
	_mm_store_ss(dst, v);
#else
	dst[0] = GetX(v);
#endif
}*/

// ================================================================================================

__forceinline void V4Float16Vec4Pack(Float16Vec4* dst, Vector_4V_In src)
{
	Vector_4V temp;
#if RSG_XENON // XMStoreHalf4
	temp = src; // lame
	temp = __altivec_XENON_vpkd3d(temp, src, VPACK_FLOAT16_4, VPACK_64LO, 2);
#else
	// [SSE TODO] -- don't bother with all 8 components if we don't need them ..
	temp = V4Float16Vec8Pack(src, src);
#endif
	V4StoreXY((float*)dst, temp);
}

__forceinline void V4Float16Vec2Pack(Float16Vec2* dst, Vector_4V_In src)
{
	Vector_4V temp;
#if RSG_XENON // XMStoreHalf2
	temp = src; // lame
	temp = __altivec_XENON_vpkd3d(temp, src, VPACK_FLOAT16_2, VPACK_32, 3);
#else
	// [SSE TODO] -- don't bother with all 8 components if we don't need them ..
	temp = V4Float16Vec8Pack(src, src);
#endif
	V4StoreScalar32FromSplatted(*(u32*)dst, temp);
}

__forceinline Vector_4V_Out V4Float16Vec4Unpack(const Float16Vec4* src)
{
#if RSG_XENON // XMLoadHalf4
	Vector_4V temp = V4LoadZW((const float*)src);
	temp = __altivec_XENON_vupkd3d(temp, VPACK_FLOAT16_4);
#else
	// [SSE TODO] -- don't bother with all 8 components if we don't need them ..
	Vector_4V temp = V4LoadXY((const float*)src);
	Vector_4V scratch;
	V4Float16Vec8Unpack(temp, scratch, temp);
#endif
	return temp;
}

__forceinline Vector_4V_Out V4Float16Vec2Unpack(const Float16Vec2* src)
{
#if RSG_XENON // XMLoadHalf2
	Vector_4V temp = __altivec_vspltw(V4LoadX((const float*)src), 0); // must be splat because vupkd3d expects data in w component .. unless we could load w directly?
	temp = __altivec_XENON_vupkd3d(temp, VPACK_FLOAT16_2);
#else
	// [SSE TODO] -- don't bother with all 8 components if we don't need them ..
	Vector_4V temp = V4LoadX((const float*)src);
	Vector_4V scratch;
	V4Float16Vec8Unpack(temp, scratch, temp);
#endif
	return temp;
}

// ================================================================================================

__forceinline Vector_4V_Out V4PackARGBColor32IntoWComponent(Vector_4V_In vectorXYZ, Vector_4V_In vectorRGBA)
{
#if RSG_CPU_INTEL
	Vector_4V temp = V4Permute<Z,Y,X,W>(vectorRGBA); // RGBA -> ARGB, elements are backwards, ack!
#else
	Vector_4V temp = V4Permute<W,X,Y,Z>(vectorRGBA); // RGBA -> ARGB
#endif
	// [SSE TODO] -- use V4FloatToIntRaw<0> and scale manually (also review usage of PackRGBA/BGRA)
	temp = V4FloatToIntRaw<8>(temp); // scales by 256 (more correct would be to scale by 255 and add 0.5, but that's slower)
	temp = V4PackSignedIntToSignedShort(temp, temp);
	temp = V4PackSignedShortToUnsignedByte(temp, temp);
	temp = V4PermuteTwo<X1,Y1,Z1,W2>(vectorXYZ, temp);
	return temp;
}

__forceinline Vector_4V_Out V4PackABGRColor32IntoWComponent(Vector_4V_In vectorXYZ, Vector_4V_In vectorRGBA)
{
#if RSG_CPU_INTEL
	Vector_4V temp = vectorRGBA;
#else
	Vector_4V temp = V4Permute<W,Z,Y,X>(vectorRGBA); // RGBA -> ABGR
#endif
	// [SSE TODO] -- use V4FloatToIntRaw<0> and scale manually (also review usage of PackRGBA/BGRA)
	temp = V4FloatToIntRaw<8>(temp); // scales by 256 (more correct would be to scale by 255 and add 0.5, but that's slower)
	temp = V4PackSignedIntToSignedShort(temp, temp);
	temp = V4PackSignedShortToUnsignedByte(temp, temp);
	temp = V4PermuteTwo<X1,Y1,Z1,W2>(vectorXYZ, temp);
	return temp;
}

__forceinline Vector_4V_Out V4PackRGBAColor32IntoWComponent(Vector_4V_In vectorXYZ, Vector_4V_In vectorRGBA)
{
#if RSG_CPU_INTEL
	Vector_4V temp = V4Permute<W,Z,Y,X>(vectorRGBA); // elements are backwards, ack!
#else
	Vector_4V temp = vectorRGBA;
#endif
	// [SSE TODO] -- use V4FloatToIntRaw<0> and scale manually (also review usage of PackRGBA/BGRA)
	temp = V4FloatToIntRaw<8>(temp); // scales by 256 (more correct would be to scale by 255 and add 0.5, but that's slower)
	temp = V4PackSignedIntToSignedShort(temp, temp);
	temp = V4PackSignedShortToUnsignedByte(temp, temp);
	temp = V4PermuteTwo<X1,Y1,Z1,W2>(vectorXYZ, temp);
	return temp;
}

__forceinline void V4PackARGBColor32(Color32* dst, Vector_4V_In vectorRGBA)
{
#if RSG_CPU_INTEL
	Vector_4V temp = V4Permute<Z,Y,X,W>(vectorRGBA); // RGBA -> ARGB, elements are backwards, ack!
#else
	Vector_4V temp = V4Permute<W,X,Y,Z>(vectorRGBA); // RGBA -> ARGB
#endif
	// [SSE TODO] -- use V4FloatToIntRaw<0> and scale manually (also review usage of PackRGBA/BGRA)
	temp = V4FloatToIntRaw<8>(temp); // scales by 256 (more correct would be to scale by 255 and add 0.5, but that's slower)
	temp = V4PackSignedIntToSignedShort(temp, temp);
	temp = V4PackSignedShortToUnsignedByte(temp, temp);
	V4StoreScalar32FromSplatted(*(u32*)dst, temp);
}

__forceinline void V4PackABGRColor32(Color32* dst, Vector_4V_In vectorRGBA)
{
#if RSG_CPU_INTEL
	Vector_4V temp = vectorRGBA;
#else
	Vector_4V temp = V4Permute<W,Z,Y,X>(vectorRGBA); // RGBA -> ABGR
#endif
	// [SSE TODO] -- use V4FloatToIntRaw<0> and scale manually (also review usage of PackRGBA/BGRA)
	temp = V4FloatToIntRaw<8>(temp); // scales by 256 (more correct would be to scale by 255 and add 0.5, but that's slower)
	temp = V4PackSignedIntToSignedShort(temp, temp);
	temp = V4PackSignedShortToUnsignedByte(temp, temp);
	V4StoreScalar32FromSplatted(*(u32*)dst, temp);
}

__forceinline void V4PackRGBAColor32(u32* dst, Vector_4V_In vectorRGBA)
{
#if RSG_CPU_INTEL
	Vector_4V temp = V4Permute<W,Z,Y,X>(vectorRGBA); // elements are backwards, ack!
#else
	Vector_4V temp = vectorRGBA;
#endif
	// [SSE TODO] -- use V4FloatToIntRaw<0> and scale manually (also review usage of PackRGBA/BGRA)
	temp = V4FloatToIntRaw<8>(temp); // scales by 256 (more correct would be to scale by 255 and add 0.5, but that's slower)
	temp = V4PackSignedIntToSignedShort(temp, temp);
	temp = V4PackSignedShortToUnsignedByte(temp, temp);
	V4StoreScalar32FromSplatted(*dst, temp);
}

__forceinline Vector_4V_Out V4UnpackARGBColor32(const Color32* src)
{
	Vector_4V temp = V4LoadX((const float*)src);
#if RSG_CPU_SPU
	const qword control = (qword)(vec_uint4){ 0x00000000, 0x01010101, 0x02020202, 0x03030303 };
	temp = (Vector_4V)si_cfltu(si_shufb((qword)temp, (qword)temp, control), 32); // scales by 1/2^32
#elif RSG_CPU_INTEL
	temp = V4MergeXYByte(temp, V4VConstant(V_ZERO));
	temp = V4MergeXYShort(temp, V4VConstant(V_ZERO));
	temp = V4IntToFloatRaw<0>(temp);
	temp = V4Scale(temp, V4VConstantSplat<U32_ONE_OVER_255>());
	temp = V4Permute<X,W,Z,Y>(temp); // ARGB -> RGBA, elements are backwards, ack!
#else
	temp = V4MergeXYByte(V4VConstant(V_ZERO), temp);
	temp = V4MergeXYShort(V4VConstant(V_ZERO), temp);
	temp = V4IntToFloatRaw<0>(temp);
	temp = V4Scale(temp, V4VConstantSplat<U32_ONE_OVER_255>());
	temp = V4Permute<Y,Z,W,X>(temp); // ARGB -> RGBA
#endif
	return temp;
}

__forceinline Vector_4V_Out V4UnpackABGRColor32(const Color32* src)
{
	Vector_4V temp = V4LoadX((const float*)src);
#if RSG_CPU_SPU
	const qword control = (qword)(vec_uint4){ 0x00000000, 0x01010101, 0x02020202, 0x03030303 };
	temp = (Vector_4V)si_cfltu(si_shufb((qword)temp, (qword)temp, control), 32); // scales by 1/2^32
#elif RSG_CPU_INTEL
	temp = V4MergeXYByte(temp, V4VConstant(V_ZERO));
	temp = V4MergeXYShort(temp, V4VConstant(V_ZERO));
	temp = V4IntToFloatRaw<0>(temp);
	temp = V4Scale(temp, V4VConstantSplat<U32_ONE_OVER_255>());
#else
	temp = V4MergeXYByte(V4VConstant(V_ZERO), temp);
	temp = V4MergeXYShort(V4VConstant(V_ZERO), temp);
	temp = V4IntToFloatRaw<0>(temp);
	temp = V4Scale(temp, V4VConstantSplat<U32_ONE_OVER_255>());
	temp = V4Permute<W,Z,Y,X>(temp); // ABGR -> RGBA
#endif
	return temp;
}

__forceinline Vector_4V_Out V4UnpackRGBAColor32(const u32* src)
{
	Vector_4V temp = V4LoadX((const float*)src);
#if RSG_CPU_SPU
	const qword control = (qword)(vec_uint4){ 0x00000000, 0x01010101, 0x02020202, 0x03030303 };
	temp = (Vector_4V)si_cfltu(si_shufb((qword)temp, (qword)temp, control), 32); // scales by 1/2^32
#elif RSG_CPU_INTEL
	temp = V4MergeXYByte(temp, V4VConstant(V_ZERO));
	temp = V4MergeXYShort(temp, V4VConstant(V_ZERO));
	temp = V4IntToFloatRaw<0>(temp);
	temp = V4Scale(temp, V4VConstantSplat<U32_ONE_OVER_255>());
	temp = V4Permute<W,Z,Y,X>(temp); // elements are backwards, ack!
#else
	temp = V4MergeXYByte(V4VConstant(V_ZERO), temp);
	temp = V4MergeXYShort(V4VConstant(V_ZERO), temp);
	temp = V4IntToFloatRaw<0>(temp);
	temp = V4Scale(temp, V4VConstantSplat<U32_ONE_OVER_255>());
#endif
	return temp;
}

// ================================================================================================

#if 0
/*

//Float16
//FracS16 // s16/32767
//FracU16 // u16/65535

// XMLoadShortN4
__forceinline Vector_4V_Out V4FracS16Vec4Unpack(const s64* src)
{
	Vector_4V temp = V4LoadUnaligned8(src);
#if RSG_XENON
	const Vector_4V kV_Scale = V4VConstantSplat<0x43000100>(); //      (float)(1<<22)/32767.0f = 128.00390636921292764061403241066f
	const Vector_4V kV_Bias  = V4VConstantSplat<0x43C00180>(); // 3.0f*(float)(1<<22)/32767.0f = 384.01171910763878292184209723197f

	temp = __altivec_XENON_vupkd3d(temp, VPACK_NORMSHORT4);
	temp = __altivec_vnmsubfp     (temp, kV_Scale, kV_Bias);
#else
	...
#endif
	return v;
}

// XMLoadUShortN4
__forceinline Vector_4V_Out V4FracU16Vec4Unpack(const u64* src)
{
	Vector_4V temp = V4LoadUnaligned8(src);

	const Vector_4V kV_Scale = V4VConstantSplat<0x37800080>(); // 1.0f/65535.0f = 1.5259021896696421759365224689097e-5
	const Vector_4V kV_Zero  = __altivec_vspltisw(0);

	temp = __altivec_vmrghh(kV_Zero, temp);
	temp = __altivec_vcfsx (temp, 0);
	temp = __altivec_vmulfp(temp, kV_Scale);

	return temp;
}

*/
#endif

// ================================================================================================
// ================================================================================================
// ================================================================================================

#if RSG_CPU_INTEL

// note that this function FAILS for input patterns >= 0x48000000, it returns zero instead of clamping to max float16
__forceinline __m128 V4PackFloat16_INSOMNIAC(__m128 a, __m128 b)
{
	const __m128i s_mask  = _mm_set1_epi16(0x8000u);
	const __m128i blah    = _mm_set1_epi16(0x4000u);
	const __m128i exp_off = _mm_set1_epi16(0x3800u);

	const __m128i x = _mm_castps_si128(a);
	const __m128i y = _mm_castps_si128(b);

	__m128i a_rot5 = _mm_slli_epi32(x,3);
	__m128i b_rot5 = _mm_slli_epi32(y,3);
	__m128i mid    = _mm_packhi_epi32(a_rot5,b_rot5); // [eeeeeemm.mmmmmmmm]
	__m128i hi     = _mm_packhi_epi32(x,y);           // [seeeeeee.emmmmmmm]
	__m128i h      = _mm_xor_si128(mid,blah);         // [eEeeeemm.mmmmmmmm]
	__m128i e      = _mm_andnot_si128(s_mask,hi);     // [0eeeeeee.emmmmmmm]
	h              = _mm_select_si128(h,hi,s_mask);   // [sEeeeemm.mmmmmmmm] <- the flipped 0x4000 bit? equiv to sub (127-15)<<10 .. 1.1100.0000.0000.0000
	__m128i d_mask = _mm_cmpgt_epi16(exp_off,e);
	h              = _mm_andnot_si128(d_mask,h);

	return _mm_castsi128_ps(h);
}

#endif // RSG_CPU_INTEL

} // namespace Vec

__forceinline Vec4V_Out Float16Vec8Pack(Vec4V_In src0, Vec4V_In src1) { return Vec4V(Vec::V4Float16Vec8Pack(src0.GetIntrin128(), src1.GetIntrin128())); }

__forceinline Vec4V_Out Float16Vec4PackIntoXY(Vec4V_In v, Vec4V_In src) { return Vec4V(Vec::V4Float16Vec4PackIntoXY(v.GetIntrin128(), src.GetIntrin128())); }
__forceinline Vec4V_Out Float16Vec4PackIntoZW(Vec4V_In v, Vec4V_In src) { return Vec4V(Vec::V4Float16Vec4PackIntoZW(v.GetIntrin128(), src.GetIntrin128())); }

__forceinline Vec4V_Out Float16Vec2PackIntoX(Vec4V_In v, Vec2V_In src) { return Vec4V(Vec::V4Float16Vec2PackIntoX(v.GetIntrin128(), src.GetIntrin128())); }
__forceinline Vec4V_Out Float16Vec2PackIntoY(Vec4V_In v, Vec2V_In src) { return Vec4V(Vec::V4Float16Vec2PackIntoY(v.GetIntrin128(), src.GetIntrin128())); }
__forceinline Vec4V_Out Float16Vec2PackIntoZ(Vec4V_In v, Vec2V_In src) { return Vec4V(Vec::V4Float16Vec2PackIntoZ(v.GetIntrin128(), src.GetIntrin128())); }
__forceinline Vec4V_Out Float16Vec2PackIntoW(Vec4V_In v, Vec2V_In src) { return Vec4V(Vec::V4Float16Vec2PackIntoW(v.GetIntrin128(), src.GetIntrin128())); }

__forceinline void Float16Vec4Pack(Float16Vec4* dst, Vec4V_In src) { Vec::V4Float16Vec4Pack(dst, src.GetIntrin128()); }
__forceinline void Float16Vec4Pack(Float16Vec4* dst, Vec3V_In src) { Vec::V4Float16Vec4Pack(dst, src.GetIntrin128()); }
__forceinline void Float16Vec2Pack(Float16Vec2* dst, Vec2V_In src) { Vec::V4Float16Vec2Pack(dst, src.GetIntrin128()); }

__forceinline void Float16Vec8Unpack(Vec4V_InOut dst0, Vec4V_InOut dst1, Vec4V_In src) { Vec::V4Float16Vec8Unpack(dst0.GetIntrin128Ref(), dst1.GetIntrin128Ref(), src.GetIntrin128()); }

__forceinline Vec4V_Out Float16Vec4UnpackFromXY(Vec4V_In src) { return Vec4V(Vec::V4Float16Vec4UnpackFromXY(src.GetIntrin128())); }
__forceinline Vec4V_Out Float16Vec4UnpackFromZW(Vec4V_In src) { return Vec4V(Vec::V4Float16Vec4UnpackFromZW(src.GetIntrin128())); }

__forceinline Vec2V_Out Float16Vec2UnpackFromX(Vec4V_In src) { return Vec2V(Vec::V4Float16Vec2UnpackFromX(src.GetIntrin128())); }
__forceinline Vec2V_Out Float16Vec2UnpackFromY(Vec4V_In src) { return Vec2V(Vec::V4Float16Vec2UnpackFromY(src.GetIntrin128())); }
__forceinline Vec2V_Out Float16Vec2UnpackFromZ(Vec4V_In src) { return Vec2V(Vec::V4Float16Vec2UnpackFromZ(src.GetIntrin128())); }
__forceinline Vec2V_Out Float16Vec2UnpackFromW(Vec4V_In src) { return Vec2V(Vec::V4Float16Vec2UnpackFromW(src.GetIntrin128())); }

__forceinline Vec4V_Out Float16Vec4Unpack(const Float16Vec4* src) { return Vec4V(Vec::V4Float16Vec4Unpack(src)); }
__forceinline Vec2V_Out Float16Vec2Unpack(const Float16Vec2* src) { return Vec2V(Vec::V4Float16Vec2Unpack(src)); }

__forceinline Vec4V_Out PackARGBColor32IntoWComponent(Vec3V_In vectorXYZ, Vec4V_In vectorRGBA) { return Vec4V(Vec::V4PackARGBColor32IntoWComponent(vectorXYZ.GetIntrin128(), vectorRGBA.GetIntrin128())); }
__forceinline Vec4V_Out PackABGRColor32IntoWComponent(Vec3V_In vectorXYZ, Vec4V_In vectorRGBA) { return Vec4V(Vec::V4PackABGRColor32IntoWComponent(vectorXYZ.GetIntrin128(), vectorRGBA.GetIntrin128())); };
__forceinline Vec4V_Out PackRGBAColor32IntoWComponent(Vec3V_In vectorXYZ, Vec4V_In vectorRGBA) { return Vec4V(Vec::V4PackRGBAColor32IntoWComponent(vectorXYZ.GetIntrin128(), vectorRGBA.GetIntrin128())); }

__forceinline void      PackARGBColor32(Color32* dst, Vec4V_In vectorRGBA) { Vec::V4PackARGBColor32(dst, vectorRGBA.GetIntrin128()); }
__forceinline void      PackABGRColor32(Color32* dst, Vec4V_In vectorRGBA) { Vec::V4PackABGRColor32(dst, vectorRGBA.GetIntrin128()); }
__forceinline void      PackRGBAColor32(u32    * dst, Vec4V_In vectorRGBA) { Vec::V4PackRGBAColor32(dst, vectorRGBA.GetIntrin128()); }

__forceinline Vec4V_Out UnpackARGBColor32(const Color32* src) { return Vec4V(Vec::V4UnpackARGBColor32(src)); }
__forceinline Vec4V_Out UnpackABGRColor32(const Color32* src) { return Vec4V(Vec::V4UnpackABGRColor32(src)); }
__forceinline Vec4V_Out UnpackRGBAColor32(const u32    * src) { return Vec4V(Vec::V4UnpackRGBAColor32(src)); }

__forceinline Vec4V_Out FixedPoint3_13FromVec4(Vec4V_In v)
{
	using namespace Vec;
	const float maxPositive = 4.0f - (1.0f / 8192.0f);

	ScalarV scale(8192.0f);
	ScalarV clampPos(maxPositive);
	Vec4V   clampNeg(V_NEGFOUR);

	Vec4V clampedV = Clamp(v, clampNeg, (Vec4V)clampPos);
	Vector_4V scaledV = V4Scale(scale.GetIntrin128(), clampedV.GetIntrin128());
	Vector_4V scaledAndRoundedV = V4RoundToNearestInt(scaledV);

	const int scalePower = 0;
	Vector_4V  fixedPoint = V4FloatToIntRaw<scalePower>(scaledAndRoundedV);
	return (Vec4V)fixedPoint;
}

__forceinline Vec4V_Out FixedPoint3_13Vec4PackIntoXY(Vec4V_In packed, Vec4V_In toPack)
{	
	using namespace Vec;
	Vec4V fixedPoint = FixedPoint3_13FromVec4(toPack);
	Vector_4V permuteVec = Vec4VConstant<0x02030607, 0x0A0B0E0F, 0x18191A1B, 0x1C1D1E1F>().GetIntrin128();
	Vector_4V result = V4PermuteTwo(fixedPoint.GetIntrin128(), packed.GetIntrin128(), permuteVec );
	return (Vec4V)result;
}

__forceinline Vec4V_Out FixedPoint3_13Vec4PackIntoZW(Vec4V_In packed, Vec4V_In toPack)
{	
	using namespace Vec;
	Vec4V fixedPoint = FixedPoint3_13FromVec4(toPack);
	Vector_4V permuteVec = Vec4VConstant<0x10111213, 0x14151617, 0x02030607, 0x0A0B0E0F>().GetIntrin128();
	Vector_4V result = V4PermuteTwo(fixedPoint.GetIntrin128(), packed.GetIntrin128(), permuteVec );
	return (Vec4V)result;
}

__forceinline Vec4V_Out FixedPoint3_13Vec4UnpackFromXY(Vec4V_In v)
{		
	using namespace Vec;
	// Worst named function ever!!  believe it or not this calls vec_unpackh(vec_short8 v)
	Vector_4V unpacked = V4UnpackLowSignedShort(v.GetIntrin128());

	const int scalePower = 13; 
	Vector_4V convertedToFloat = V4IntToFloatRaw<scalePower>(unpacked);
	return (Vec4V)convertedToFloat;
}

__forceinline Vec4V_Out FixedPoint3_13Vec4UnpackFromZW(Vec4V_In v)
{		
	using namespace Vec;
	// Worst named function ever!!  believe it or not this calls vec_unpackl(vec_short8 v)
	Vector_4V unpacked = V4UnpackHighSignedShort(v.GetIntrin128());

	const int scalePower = 13; 
	Vector_4V convertedToFloat = V4IntToFloatRaw<scalePower>(unpacked);
	return (Vec4V)convertedToFloat;
}

} // namespace rage
