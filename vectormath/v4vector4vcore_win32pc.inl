//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "system/floattoint.h"

#define AVX_TRANSCENDENTALS ((0 || RSG_DURANGO || RSG_ORBIS || RSG_PC) && !__RESOURCECOMPILER && !__GAMETOOL)

namespace rage
{
#if AVX_TRANSCENDENTALS

// Conversion types for constants
typedef __m128 _XMVECTOR;

BEGIN_ALIGNED(16) struct _XMVECTORF32
{
    union
    {
        float f[4];
        _XMVECTOR v;
    };

    inline operator _XMVECTOR() const { return v; }
    inline operator const float*() const { return f; }
    inline operator __m128i() const { return _mm_castps_si128(v); }
    inline operator __m128d() const { return _mm_castps_pd(v); }
} ALIGNED(16);

BEGIN_ALIGNED(16) struct _XMVECTORI32
{
    union
    {
        s32 i[4];
        _XMVECTOR v;
    };

    inline operator _XMVECTOR() const { return v; }
    inline operator __m128i() const { return _mm_castps_si128(v); }
    inline operator __m128d() const { return _mm_castps_pd(v); }
} ALIGNED(16);

const float _XM_PI           = 3.141592654f;
const float _XM_2PI          = 6.283185307f;
const float _XM_1DIVPI       = 0.318309886f;
const float _XM_1DIV2PI      = 0.159154943f;
const float _XM_PIDIV2       = 1.570796327f;
const float _XM_PIDIV4       = 0.785398163f;

const u32 _XM_SELECT_0          = 0x00000000;
const u32 _XM_SELECT_1          = 0xFFFFFFFF;

const _XMVECTORI32 g_XMNegativeZero      = {0x80000000, 0x80000000, 0x80000000, 0x80000000};

const _XMVECTORF32 g_XMOne               = { 1.0f, 1.0f, 1.0f, 1.0f};

const _XMVECTORF32 g_XMZero              = { 0.0f, 0.0f, 0.0f, 0.0f};

const _XMVECTORI32 g_XMInfinity          = {0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000};
const _XMVECTORI32 g_XMQNaN              = {0x7FC00000, 0x7FC00000, 0x7FC00000, 0x7FC00000};
const _XMVECTORI32 g_XMQNaNTest          = {0x007FFFFF, 0x007FFFFF, 0x007FFFFF, 0x007FFFFF};
const _XMVECTORI32 g_XMAbsMask           = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};

const _XMVECTORI32 g_XMExponentBias      = {127, 127, 127, 127};
const _XMVECTORI32 g_XMSubnormalExponent = {-126, -126, -126, -126};
const _XMVECTORI32 g_XMNumTrailing       = {23, 23, 23, 23};
const _XMVECTORI32 g_XMMinNormal         = {0x00800000, 0x00800000, 0x00800000, 0x00800000};
const _XMVECTORI32 g_XMNegInfinity       = {0xFF800000, 0xFF800000, 0xFF800000, 0xFF800000};
const _XMVECTORI32 g_XMNegQNaN           = {0xFFC00000, 0xFFC00000, 0xFFC00000, 0xFFC00000};
const _XMVECTORI32 g_XMBin128            = {0x43000000, 0x43000000, 0x43000000, 0x43000000};
const _XMVECTORI32 g_XMBinNeg150         = {0xC3160000, 0xC3160000, 0xC3160000, 0xC3160000};
const _XMVECTORI32 g_XM253               = {253, 253, 253, 253};
const _XMVECTORF32 g_XMExpEst1           = {-6.93147180e-1f, -6.93147180e-1f, -6.93147180e-1f, -6.93147180e-1f};	// 0xbf317218
const _XMVECTORF32 g_XMExpEst2           = {+2.40226854e-1f, +2.40226854e-1f, +2.40226854e-1f, +2.40226854e-1f};	// 0x3e75fe07
const _XMVECTORF32 g_XMExpEst3           = {-5.55041353e-2f, -5.55041353e-2f, -5.55041353e-2f, -5.55041353e-2f};	// 0xbd63584e
const _XMVECTORF32 g_XMExpEst4           = {+9.61640817e-3f, +9.61640817e-3f, +9.61640817e-3f, +9.61640817e-3f};	// 0x3c1d8e24
const _XMVECTORF32 g_XMExpEst5           = {-1.33322351e-3f, -1.33322351e-3f, -1.33322351e-3f, -1.33322351e-3f};	// 0xbaaebf8f
const _XMVECTORF32 g_XMExpEst6           = {+1.56748996e-4f, +1.56748996e-4f, +1.56748996e-4f, +1.56748996e-4f};	// 0x39245cfd 
const _XMVECTORF32 g_XMExpEst7           = {-1.54614474e-5f, -1.54614474e-5f, -1.54614474e-5f, -1.54614474e-5f};	// 0xb781b335
const _XMVECTORF32 g_XMLogEst0           = {+1.442693f, +1.442693f, +1.442693f, +1.442693f};
const _XMVECTORF32 g_XMLogEst1           = {-0.721242f, -0.721242f, -0.721242f, -0.721242f};
const _XMVECTORF32 g_XMLogEst2           = {+0.479384f, +0.479384f, +0.479384f, +0.479384f};
const _XMVECTORF32 g_XMLogEst3           = {-0.350295f, -0.350295f, -0.350295f, -0.350295f};
const _XMVECTORF32 g_XMLogEst4           = {+0.248590f, +0.248590f, +0.248590f, +0.248590f};
const _XMVECTORF32 g_XMLogEst5           = {-0.145700f, -0.145700f, -0.145700f, -0.145700f};
const _XMVECTORF32 g_XMLogEst6           = {+0.057148f, +0.057148f, +0.057148f, +0.057148f};
const _XMVECTORF32 g_XMLogEst7           = {-0.010578f, -0.010578f, -0.010578f, -0.010578f};
const _XMVECTORF32 g_XMLgE               = {+1.442695f, +1.442695f, +1.442695f, +1.442695f};

#define __DENORMALS 0
// split V into integer and fractional part, n+f,
// calculate 2^n/((1/2)^f)
	__forceinline Vec::Vector_4V_Out _XMVectorExp2(Vec::Vector_4V_In V)
	{
		__m128i itrunc = _mm_cvttps_epi32(V);
		__m128 ftrunc = _mm_cvtepi32_ps(itrunc);
		__m128 y = _mm_sub_ps(V, ftrunc);
		__m128 poly = _mm_mul_ps(g_XMExpEst7, y);
		poly = _mm_add_ps(g_XMExpEst6, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst5, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst4, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst3, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst2, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst1, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMOne, poly);

		__m128i biased = _mm_add_epi32(itrunc, g_XMExponentBias);
		biased = _mm_slli_epi32(biased, 23);
		__m128 result0 = _mm_div_ps(_mm_castsi128_ps(biased), poly);

#if __DENORMALS
		biased = _mm_add_epi32(itrunc, g_XM253);
		biased = _mm_slli_epi32(biased, 23);
		__m128 result1 = _mm_div_ps(_mm_castsi128_ps(biased), poly);
		result1 = _mm_mul_ps(g_XMMinNormal.v, result1);
#endif

		// Use selection to handle the cases
		//  if (V is NaN) -> QNaN;
		//  else if (V sign bit set)
		//      if (V > -150)
		//         if (V.exponent < -126) -> result1
		//         else -> result0
		//      else -> +0
		//  else
		//      if (V < 128) -> result0
		//      else -> +inf

		__m128i comp = _mm_cmplt_epi32( _mm_castps_si128(V), g_XMBin128);
		__m128i select0 = _mm_and_si128(comp, _mm_castps_si128(result0));
		__m128i select1 = _mm_andnot_si128(comp, g_XMInfinity);
		__m128i result2 = _mm_or_si128(select0, select1);

		comp = _mm_cmplt_epi32(itrunc, g_XMSubnormalExponent);
#if __DENORMALS
		select1 = _mm_and_si128(comp, _mm_castps_si128(result1));
#else
		select1 = _mm_and_si128(comp, g_XMZero);
#endif
		select0 = _mm_andnot_si128(comp, _mm_castps_si128(result0));
		__m128i result3 = _mm_or_si128(select0, select1);

		comp = _mm_cmplt_epi32(_mm_castps_si128(V), g_XMBinNeg150);
		select0 = _mm_and_si128(comp, result3);
		select1 = _mm_andnot_si128(comp, g_XMZero);
		__m128i result4 = _mm_or_si128(select0, select1);

		__m128i sign = _mm_and_si128(_mm_castps_si128(V), g_XMNegativeZero);
		comp = _mm_cmpeq_epi32(sign, g_XMNegativeZero);
		select0 = _mm_and_si128(comp, result4);
		select1 = _mm_andnot_si128(comp, result2);
		__m128i result5 = _mm_or_si128(select0, select1);

#if __DEV
		__m128i t0 = _mm_and_si128(_mm_castps_si128(V), g_XMQNaNTest);
		__m128i t1 = _mm_and_si128(_mm_castps_si128(V), g_XMInfinity);
		t0 = _mm_cmpeq_epi32(t0, g_XMZero);
		t1 = _mm_cmpeq_epi32(t1, g_XMInfinity);
		__m128i isNaN = _mm_andnot_si128(t0, t1);

		select0 = _mm_and_si128(isNaN, g_XMQNaN);
		select1 = _mm_andnot_si128(isNaN, result5);
		__m128i vResult = _mm_or_si128(select0, select1);

		return _mm_castsi128_ps(vResult);
#else	// __DEV
		return _mm_castsi128_ps(result5);
#endif	// __DEV
	}

	__forceinline Vec::Vector_4V_Out _XMVectorExpE(Vec::Vector_4V_In V)
	{
		// expE(V) = exp2(vin*log2(e))
		__m128 Ve = _mm_mul_ps(g_XMLgE, V);

		__m128i itrunc = _mm_cvttps_epi32(Ve);
		__m128 ftrunc = _mm_cvtepi32_ps(itrunc);
		__m128 y = _mm_sub_ps(Ve, ftrunc);
		__m128 poly = _mm_mul_ps(g_XMExpEst7, y);
		poly = _mm_add_ps(g_XMExpEst6, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst5, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst4, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst3, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst2, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMExpEst1, poly);
		poly = _mm_mul_ps(poly, y);
		poly = _mm_add_ps(g_XMOne, poly);

		__m128i biased;
		biased = _mm_add_epi32(itrunc, g_XMExponentBias);
		biased = _mm_slli_epi32(biased, 23);
		__m128 result0 = _mm_div_ps(_mm_castsi128_ps(biased), poly);

#if __DENORMALS
		biased = _mm_add_epi32(itrunc, g_XM253);
		biased = _mm_slli_epi32(biased, 23);
		__m128 result1 = _mm_div_ps(_mm_castsi128_ps(biased), poly);
		result1 = _mm_mul_ps(g_XMMinNormal.v, result1);
#endif

		// Use selection to handle the cases
		//  if (V is NaN) -> QNaN;
		//  else if (V sign bit set)
		//      if (V > -150)
		//         if (V.exponent < -126) -> result1
		//         else -> result0
		//      else -> +0
		//  else
		//      if (V < 128) -> result0
		//      else -> +inf

		__m128i comp = _mm_cmplt_epi32( _mm_castps_si128(Ve), g_XMBin128);
		__m128i select0 = _mm_and_si128(comp, _mm_castps_si128(result0));
		__m128i select1 = _mm_andnot_si128(comp, g_XMInfinity);
		__m128i result2 = _mm_or_si128(select0, select1);

		comp = _mm_cmplt_epi32(itrunc, g_XMSubnormalExponent);
#if __DENORMALS
		select1 = _mm_and_si128(comp, _mm_castps_si128(result1));
#else
		select1 = _mm_and_si128(comp, g_XMZero);
#endif
		select0 = _mm_andnot_si128(comp, _mm_castps_si128(result0));
		__m128i result3 = _mm_or_si128(select0, select1);

		comp = _mm_cmplt_epi32(_mm_castps_si128(Ve), g_XMBinNeg150);
		select0 = _mm_and_si128(comp, result3);
		select1 = _mm_andnot_si128(comp, g_XMZero);
		__m128i result4 = _mm_or_si128(select0, select1);

		__m128i sign = _mm_and_si128(_mm_castps_si128(Ve), g_XMNegativeZero);
		comp = _mm_cmpeq_epi32(sign, g_XMNegativeZero);
		select0 = _mm_and_si128(comp, result4);
		select1 = _mm_andnot_si128(comp, result2);
		__m128i result5 = _mm_or_si128(select0, select1);

#if __DEV
		__m128i t0 = _mm_and_si128(_mm_castps_si128(Ve), g_XMQNaNTest);
		__m128i t1 = _mm_and_si128(_mm_castps_si128(Ve), g_XMInfinity);
		t0 = _mm_cmpeq_epi32(t0, g_XMZero);
		t1 = _mm_cmpeq_epi32(t1, g_XMInfinity);
		__m128i isNaN = _mm_andnot_si128(t0, t1);

		select0 = _mm_and_si128(isNaN, g_XMQNaN);
		select1 = _mm_andnot_si128(isNaN, result5);
		__m128i vResult = _mm_or_si128(select0, select1);

		return _mm_castsi128_ps(vResult);
#else	// __DEV
		return _mm_castsi128_ps(result5);
#endif	// __DEV
	}

	// for V = m*2^E
	// calculate E+ln(1+x)/ln(2), x = m-1
	__forceinline Vec::Vector_4V_Out _XMVectorLog2(Vec::Vector_4V_In V)
	{
		__m128i rawBiased = _mm_and_si128(_mm_castps_si128(V), g_XMInfinity);
		__m128i trailing = _mm_and_si128(_mm_castps_si128(V), g_XMQNaNTest);
#if __DENORMALS
		__m128i isExponentZero = _mm_cmpeq_epi32(g_XMZero, rawBiased);
#endif

		// Compute exponent and significand for normals.
		__m128i biased = _mm_srli_epi32(rawBiased, 23);
		__m128i exponentNor = _mm_sub_epi32(biased, g_XMExponentBias);
		__m128i trailingNor = trailing;

#if __DENORMALS
		// Compute exponent and significand for subnormals.
		__m128i leading = Internal::GetLeadingBit(trailing);
		__m128i shift = _mm_sub_epi32(g_XMNumTrailing, leading);
		__m128i exponentSub = _mm_sub_epi32(g_XMSubnormalExponent, shift);
		__m128i trailingSub = Internal::multi_sll_epi32(trailing, shift);
		trailingSub = _mm_and_si128(trailingSub, g_XMQNaNTest);

		__m128i select0 = _mm_and_si128(isExponentZero, exponentSub);
		__m128i select1 = _mm_andnot_si128(isExponentZero, exponentNor);
		__m128i e = _mm_or_si128(select0, select1);

		select0 = _mm_and_si128(isExponentZero, trailingSub);
		select1 = _mm_andnot_si128(isExponentZero, trailingNor);
		__m128i t = _mm_or_si128(select0, select1);
#else	// __DENORMALS
		__m128i e = exponentNor;
		__m128i t = trailingNor;
#endif	// __DENORMALS

		// Compute the approximation.
		__m128i tmp = _mm_or_si128(g_XMOne, t);
		__m128 y = _mm_sub_ps(_mm_castsi128_ps(tmp), g_XMOne);

		__m128 log2 = _mm_mul_ps(g_XMLogEst7, y);
		log2 = _mm_add_ps(g_XMLogEst6, log2);
		log2 = _mm_mul_ps(log2, y);
		log2 = _mm_add_ps(g_XMLogEst5, log2);
		log2 = _mm_mul_ps(log2, y);
		log2 = _mm_add_ps(g_XMLogEst4, log2);
		log2 = _mm_mul_ps(log2, y);
		log2 = _mm_add_ps(g_XMLogEst3, log2);
		log2 = _mm_mul_ps(log2, y);
		log2 = _mm_add_ps(g_XMLogEst2, log2);
		log2 = _mm_mul_ps(log2, y);
		log2 = _mm_add_ps(g_XMLogEst1, log2);
		log2 = _mm_mul_ps(log2, y);
		log2 = _mm_add_ps(g_XMLogEst0, log2);
		log2 = _mm_mul_ps(log2, y);
		log2 = _mm_add_ps(log2, _mm_cvtepi32_ps(e));

		//  if (x is NaN) -> QNaN
		//  else if (V is positive)
		//      if (V is infinite) -> +inf
		//      else -> log2(V)
		//  else
		//      if (V is zero) -> -inf
		//      else -> -QNaN

		__m128i isInfinite = _mm_and_si128(_mm_castps_si128(V), g_XMAbsMask);
		isInfinite = _mm_cmpeq_epi32(isInfinite, g_XMInfinity);

		__m128i isGreaterZero = _mm_cmpgt_epi32(_mm_castps_si128(V), g_XMZero);
		__m128i isNotFinite = _mm_cmpgt_epi32(_mm_castps_si128(V), g_XMInfinity);
		__m128i isPositive = _mm_andnot_si128(isNotFinite, isGreaterZero);

		__m128i isZero = _mm_and_si128(_mm_castps_si128(V), g_XMAbsMask);
		isZero = _mm_cmpeq_epi32(isZero, g_XMZero);

#if __DEV
		__m128i t0 = _mm_and_si128(_mm_castps_si128(V), g_XMQNaNTest);
		__m128i t1 = _mm_and_si128(_mm_castps_si128(V), g_XMInfinity);
		t0 = _mm_cmpeq_epi32(t0, g_XMZero);
		t1 = _mm_cmpeq_epi32(t1, g_XMInfinity);
		__m128i isNaN = _mm_andnot_si128(t0, t1);
#endif

#if !__DENORMALS
		__m128i select0;
		__m128i select1;
#endif
		select0 = _mm_and_si128(isInfinite, g_XMInfinity);
		select1 = _mm_andnot_si128(isInfinite, _mm_castps_si128(log2));
		__m128i result = _mm_or_si128(select0, select1);

		select0 = _mm_and_si128(isZero, g_XMNegInfinity);
		select1 = _mm_andnot_si128(isZero, g_XMNegQNaN);
		tmp = _mm_or_si128(select0, select1);

		select0 = _mm_and_si128(isPositive, result);
		select1 = _mm_andnot_si128(isPositive, tmp);
		result = _mm_or_si128(select0, select1);

#if __DEV
		select0 = _mm_and_si128(isNaN, g_XMQNaN);
		select1 = _mm_andnot_si128(isNaN, result);
		result = _mm_or_si128(select0, select1);
#endif	// __DEV

		return _mm_castsi128_ps(result);
	}

#endif	// AVX_TRANSCENDENTALS
namespace Vec
{


	__forceinline Vector_4V_Out V4IsNotNanV( Vector_4V_In inVector )
	{
		// This comparison will fail for NaN components only.
		Vector_4V retVal = V4IsEqualV( inVector, inVector );

		// If INF, all bets are off, since we don't yet have an FPIsNotNan().
		// But we can at least assert for the case when it *should be* finite (avoid false positives).
		ASSERT_ONLY( if( FPIsFinite( Vec::GetX(inVector) ) && ( (u32)Vec::GetXi(retVal) != 0xFFFFFFFF ) ) FastAssert( !"Vectorized V4IsNotNanV() X result != scalar FPIsFinite() results!" ); );
		ASSERT_ONLY( if( FPIsFinite( Vec::GetY(inVector) ) && ( (u32)Vec::GetYi(retVal) != 0xFFFFFFFF ) ) FastAssert( !"Vectorized V4IsNotNanV() Y result != scalar FPIsFinite() results!" ); );
		ASSERT_ONLY( if( FPIsFinite( Vec::GetZ(inVector) ) && ( (u32)Vec::GetZi(retVal) != 0xFFFFFFFF ) ) FastAssert( !"Vectorized V4IsNotNanV() Z result != scalar FPIsFinite() results!" ); );
		ASSERT_ONLY( if( FPIsFinite( Vec::GetW(inVector) ) && ( (u32)Vec::GetWi(retVal) != 0xFFFFFFFF ) ) FastAssert( !"Vectorized V4IsNotNanV() W result != scalar FPIsFinite() results!" ); );

		return retVal;
	}

	__forceinline unsigned int V4IsNotNanAll(Vector_4V_In inVector)
	{
		return V4IsEqualAll(inVector, inVector);
	}

	__forceinline Vector_4V_Out V4IsFiniteV( Vector_4V_In inVector )
	{
		// Multiply by zero.
		Vector_4V v_zero = V4VConstant(V_ZERO);
		Vector_4V retVal = V4IsEqualV( v_zero, V4Scale( inVector, v_zero ) );

		FastAssert( FPIsFinite( Vec::GetX(inVector) ) == ( (u32)Vec::GetXi(retVal) == 0xFFFFFFFF ) && "Vectorized V4IsFiniteV() X result != scalar FPIsFinite() result!" );
		FastAssert( FPIsFinite( Vec::GetY(inVector) ) == ( (u32)Vec::GetYi(retVal) == 0xFFFFFFFF ) && "Vectorized V4IsFiniteV() Y result != scalar FPIsFinite() result!" );
		FastAssert( FPIsFinite( Vec::GetZ(inVector) ) == ( (u32)Vec::GetZi(retVal) == 0xFFFFFFFF ) && "Vectorized V4IsFiniteV() Z result != scalar FPIsFinite() result!" );
		FastAssert( FPIsFinite( Vec::GetW(inVector) ) == ( (u32)Vec::GetWi(retVal) == 0xFFFFFFFF ) && "Vectorized V4IsFiniteV() W result != scalar FPIsFinite() result!" );

		return retVal;
	}

	__forceinline unsigned int V4IsFiniteAll( Vector_4V_In inVector )
	{
		return V4IsLessThanAll(V4Abs(inVector),V4VConstant(V_INF));
	}

	template<unsigned FROM> __forceinline void V4Store8(void *dst, Vector_4V_In src)
	{
		// SSE4.1 would be better implemented with _mm_extract_epi8 directly to memory
		if (FROM & 1)
		{
			register u32 tmp = _mm_extract_epi16(_mm_castps_si128(src), FROM/2);
			*(u8*)dst = (u8)(tmp>>8);
		}
		else
		{
			register u32 tmp = _mm_extract_epi16(_mm_castps_si128(src), FROM/2);
			*(u8*)dst = (u8)tmp;
		}
	}

	template<unsigned FROM> __forceinline void V4Store16(void *dst, Vector_4V_In src)
	{
		FastAssert(((uptr)dst&1)==0);
		*(u16*)dst = (u16)_mm_extract_epi16(_mm_castps_si128(src), FROM);
	}

	template<unsigned FROM> __forceinline void V4Store32(void *dst, Vector_4V_In src)
	{
		// SSE4.1 would be better implemented with _mm_extract_epi32 directly to memory
		// (at least in the FROM!=0 case).
		FastAssert(((uptr)dst&3)==0);
		if (FROM != 0)
		{
			src = _mm_shuffle_ps(src, src, (FROM<<6)|(FROM<<4)|(FROM<<2)|(FROM));
		}
		_mm_store_ss((float*)dst, src);
	}

	template<unsigned FROM> __forceinline void V4Store64(void *dst, Vector_4V_In src)
	{
		// SSE4.1 would be better implemented with _mm_extract_epi64 directly to memory
		// (at least in the FROM!=0 case).
		FastAssert(((uptr)dst&7)==0);
		if (FROM != 0)
		{
			// Assume that src is in the float32 pipe, so even though the syntax
			// is a bit nastier than using _mm_shuffle_pd, this will be a tiny
			// bit faster if this assumption is correct.
			src = _mm_shuffle_ps(src, src, ((FROM*2+1)<<6)|((FROM*2)<<4)|((FROM*2+1)<<2)|(FROM*2));
 		}
		_mm_store_sd((double*)dst, _mm_castps_pd(src));
	}

	__forceinline float GetX( Vector_4V_ConstRef inVector )
	{
		return (reinterpret_cast<const float*>(&inVector))[0];
	}

	__forceinline float GetY( Vector_4V_ConstRef inVector )
	{
		return (reinterpret_cast<const float*>(&inVector))[1];
	}

	__forceinline float GetZ( Vector_4V_ConstRef inVector )
	{
		return (reinterpret_cast<const float*>(&inVector))[2];
	}

	__forceinline float GetW( Vector_4V_ConstRef inVector )
	{
		return (reinterpret_cast<const float*>(&inVector))[3];
	}

	__forceinline int GetXi( Vector_4V_ConstRef inVector )
	{
		return (reinterpret_cast<const int*>(&inVector))[0];
	}

	__forceinline int GetYi( Vector_4V_ConstRef inVector )
	{
		return (reinterpret_cast<const int*>(&inVector))[1];
	}

	__forceinline int GetZi( Vector_4V_ConstRef inVector )
	{
		return (reinterpret_cast<const int*>(&inVector))[2];
	}

	__forceinline int GetWi( Vector_4V_ConstRef inVector )
	{
		return (reinterpret_cast<const int*>(&inVector))[3];
	}

	__forceinline Vector_4V_Out V4SplatX( Vector_4V_In inVector )
	{
		return V4Permute<X, X, X, X>( inVector );
	}

	__forceinline Vector_4V_Out V4SplatY( Vector_4V_In inVector )
	{
		return V4Permute<Y, Y, Y, Y>( inVector );
	}

	__forceinline Vector_4V_Out V4SplatZ( Vector_4V_In inVector )
	{
		return V4Permute<Z, Z, Z, Z>( inVector );
	}

	__forceinline Vector_4V_Out V4SplatW( Vector_4V_In inVector )
	{
		return V4Permute<W, W, W, W>( inVector );
	}

	__forceinline void V4Set( Vector_4V_InOut inoutVector, const float& x0, const float& y0, const float& z0, const float& w0 )
	{
		inoutVector = _mm_setr_ps( x0, y0, z0, w0 );
	}

	__forceinline Vector_4V_Out V4Add( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return _mm_add_ps( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Subtract( Vector_4V_In inVector, Vector_4V_In a )
	{
		return _mm_sub_ps( inVector, a );
	}

	__forceinline Vector_4V_Out V4AddInt( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		__m128i tempA = *(__m128i*)(&inVector1);
		__m128i tempB = *(__m128i*)(&inVector2);
		__m128i tempSum = _mm_add_epi32( tempA, tempB );
		return *(Vector_4V*)(&tempSum);
	}

	__forceinline Vector_4V_Out V4SubtractInt( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		__m128i tempA = *(__m128i*)(&inVector1);
		__m128i tempB = *(__m128i*)(&inVector2);
		__m128i tempDiff = _mm_sub_epi32( tempA, tempB );
		return *(Vector_4V*)(&tempDiff);
	}

	__forceinline Vector_4V_Out V4AddScaled( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In inVector3 )
	{
		return V4Add( inVector1, V4Scale(inVector2, inVector3) );
	}

	__forceinline Vector_4V_Out V4SubtractScaled( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In inVector3 )
	{
		return V4Subtract( inVector1, V4Scale(inVector2, inVector3) );
	}

	__forceinline Vector_4V_Out V4Scale( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return _mm_mul_ps( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4InvertBits(Vector_4V_In inVector)
	{
		return _mm_xor_ps( V4VConstant(V_MASKXYZW), inVector );
	}

	__forceinline Vector_4V_Out V4Negate(Vector_4V_In inVector)
	{
#if 1 || !USE_ALTERNATE_NEGATE
		return V4Subtract( V4VConstant(V_ZERO), inVector );
#else
		// Invert the sign bits, by XORing with:
		// 0x80000000 80000000 80000000 80000000
		return V4Xor( inVector, V4VConstant(V_80000000) );
#endif // !USE_ALTERNATE_NEGATE
	}

	__forceinline Vector_4V_Out V4Abs(Vector_4V_In inVector)
	{
		// Zero the sign bits, by ANDing with:
		// 0x7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF.
		return V4Andc( inVector, V4VConstant(V_80000000) );
	}

	__forceinline Vector_4V_Out V4DotV(Vector_4V_In a, Vector_4V_In b)
	{
		// TODO: SSE3 would help here!
		//Vector_4V temp = _mm_mul_ps(a, b);
		//temp = _mm_hadd_ps(temp, temp);
		//result = _mm_hadd_ps(temp, temp);

		Vector_4V addVect1 = V4Scale( a, b );
		Vector_4V addVect2 = V4Permute<Y, X, W, Z>( addVect1 );
		Vector_4V addVect3 = V4Add( addVect1, addVect2 );
		Vector_4V addVect4 = V4Permute<Z, W, X, Y>( addVect3 );
		return V4Add( addVect3, addVect4 );
	}
	
	__forceinline Vector_4V_Out V4MagSquaredV( Vector_4V_In v )
	{
		Vector_4V addVect1 = V4Scale( v, v );
		Vector_4V addVect2 = V4Permute<Y, X, W, Z>( addVect1 );
		Vector_4V addVect3 = V4Add( addVect1, addVect2 );
		Vector_4V addVect4 = V4Permute<Z, W, X, Y>( addVect3 );
		return V4Add( addVect3, addVect4 );
	}

	__forceinline Vector_4V_Out V4Expt( Vector_4V_In x )
	{
#if AVX_TRANSCENDENTALS
		return _XMVectorExp2(x);
#else
		// TODO: Not supported in SSE. Provide a decent home-made version.

		// Here is SPU version:
		//vec_float4 bias, frac, exp;
		//vec_int4 ia;

		//bias = (vec_float4)(spu_andc((vec_int4)(0x3F7FFFFF), spu_rlmaska((vec_int4)(a), -31)));
		//ia   = spu_convts(spu_add(a, bias), 0);
		//frac = spu_sub(spu_convtf(ia, 0), a);
		//exp  = (vec_float4)(spu_sl(spu_add(ia, 127), 23));

		//return (spu_mul(spu_madd(spu_madd((vec_float4)(0.17157287f), frac, (vec_float4)(-0.67157287f)),
		//	frac, (vec_float4)(1.0f)), exp));

		Vector_4V outVect;
		SetX( outVect, FPPow(2.0f, GetX( x )) );
		SetY( outVect, FPPow(2.0f, GetY( x )) );
		SetZ( outVect, FPPow(2.0f, GetZ( x )) );
		SetW( outVect, FPPow(2.0f, GetW( x )) );
		return outVect;
#endif
	}

	__forceinline Vector_4V_Out V4Log2( Vector_4V_In x )
	{
#if AVX_TRANSCENDENTALS && 0	// could do with a little more work
		return _XMVectorLog2(x);
#else
		// TODO: Not supported in SSE. Provide a decent home-made version.

		// Here is SPU version:
		//vec_int4 exp;
		//vec_float4 frac;

		//exp  = spu_add((vec_int4)(spu_and(spu_rlmask((vec_uint4)(a), -23), 0xFF)), -127);
		//frac = (vec_float4)(spu_sub((vec_int4)(a), spu_sl(exp, 23)));

		//return (spu_madd(spu_madd((vec_float4)(-0.33985), frac, (vec_float4)(2.01955)), 
		//	frac, spu_sub(spu_convtf(exp, 0), (vec_float4)(1.6797))));

		// log2(f) = ln(f)/ln(2)
		Vector_4V outVect;
		SetX( outVect, FPLog2(GetX( x )));
		SetY( outVect, FPLog2(GetY( x )));
		SetZ( outVect, FPLog2(GetZ( x )));
		SetW( outVect, FPLog2(GetW( x )));
		return outVect;
#endif
	}

	//============================================================================
	// Conversion functions

	template <int exponent>
	__forceinline Vector_4V_Out V4FloatToIntRaw(Vector_4V_In inVector)
	{
		if( exponent == 0 )
		{
			__m128i tempIntVect = _mm_cvttps_epi32( inVector );
			return *(__m128*)(&tempIntVect);
		}
		else
		{
			Vector_4V multiplier;
			V4Set( multiplier, static_cast<float>(1 << (exponent)) );

			__m128i tempIntVect = _mm_cvttps_epi32( V4Scale(inVector,multiplier) );
			return *(__m128*)(&tempIntVect);
		}		
	}

	template <int exponent>
	__forceinline Vector_4V_Out V4IntToFloatRaw(Vector_4V_In inVector)
	{
		if( exponent == 0 )
		{
			__m128i tempIntVect = *(__m128i*)(&inVector);
			return _mm_cvtepi32_ps( tempIntVect );
		}
		else
		{
			Vector_4V multiplier;
			V4Set( multiplier, 1.0f/static_cast<float>(1 << (exponent)) );

			__m128i tempIntVect = *(__m128i*)(&inVector);
			return V4Scale(_mm_cvtepi32_ps(tempIntVect), multiplier);
		}
	}

	__forceinline Vector_4V_Out V4Uint8ToFloat32(Vector_4V_In inVector)
	{
		__m128i zi    = _mm_setzero_si128();
		__m128i vi    = _mm_unpacklo_epi8 (_mm_castps_si128(inVector), zi);
		vi            = _mm_unpacklo_epi16(vi, zi);
		__m128  vf    = _mm_cvtepi32_ps(vi);
		__m128  _inv256 = _mm_castsi128_ps(_mm_set1_epi32(FLOAT_TO_INT(1.f/256.f)));
		vf            = _mm_mul_ps(vf, _inv256);
		return vf;
	}

	__forceinline Vector_4V_In V4Float32ToUint8(Vector_4V_In inVector)
	{
		__m128  _256 = _mm_castsi128_ps(_mm_set1_epi32(FLOAT_TO_INT(256.f)));
		__m128 vf    = _mm_mul_ps(inVector, _256);
		__m128i v    = _mm_cvttps_epi32(vf);
		v            = _mm_packs_epi32(v, v); // use signed because unsigned is only supported on SSE4
		v            = _mm_packus_epi16(v, v);
		return _mm_castsi128_ps(v);
	}

	__forceinline Vector_4V_Out V4One()
	{
		return _mm_set_ps1( 1.0f );
	}

	__forceinline Vector_4V_Out V4RoundToNearestInt(Vector_4V_In inVector)
	{
		__m128 a2 = _mm_add_ps( inVector, inVector );
		__m128 trunc = _mm_cvtepi32_ps( _mm_cvttps_epi32( inVector ) );
		__m128 v = _mm_sub_ps( a2, trunc );
		return _mm_cvtepi32_ps( _mm_cvttps_epi32( v ) );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntZero(Vector_4V_In inVector)
	{
		// _mm_cvttps_epi32() always uses the truncate rounding mode.
		return _mm_cvtepi32_ps( _mm_cvttps_epi32( inVector ) );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntNegInf(Vector_4V_In inVector)
	{
		__m128 trunc = _mm_cvtepi32_ps( _mm_cvttps_epi32( inVector ) );
		__m128 gt = _mm_cmpgt_ps( trunc, inVector );
		return _mm_sub_ps( trunc, _mm_and_ps( gt, V4One() ) );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntPosInf(Vector_4V_In inVector)
	{
		__m128 trunc = _mm_cvtepi32_ps( _mm_cvttps_epi32( inVector ) );
		__m128 lt = _mm_cmplt_ps( trunc, inVector );
		return _mm_add_ps( trunc, _mm_and_ps( lt, V4One() ) );

	}


	//============================================================================
	// Comparison functions

	__forceinline Vector_4V_Out V4IsTrueV(bool b)
	{
		__m128i v0 = _mm_setzero_si128();
		__m128i v1 = _mm_insert_epi16(v0, b, 0);
		__m128i v2 = _mm_shuffle_epi32(v1, 0x00);
		__m128i v3 = _mm_cmpgt_epi32(v2, v0);
		return _mm_castsi128_ps(v3);
	}

	__forceinline Vector_4V_Out V4IsNonZeroV(u32 b)
	{
		return V4IsTrueV(!!b);
	}

	__forceinline Vector_4V_Out V4IsEvenV( Vector_4V_In inVector )
	{
		__m128i result = _mm_cvttps_epi32(inVector);
		Vector_4V evenOrOdd = V4And( *(Vector_4V*)(&result), V4VConstant(V_INT_1) );
		return V4IsEqualIntV( evenOrOdd, V4VConstant(V_ZERO) );
	}

	__forceinline Vector_4V_Out V4IsOddV( Vector_4V_In inVector )
	{
		Vector_4V _ones = V4VConstant(V_INT_1);
		__m128i result = _mm_cvttps_epi32(inVector);
		Vector_4V evenOrOdd = V4And( *(Vector_4V*)(&result), _ones );
		return V4IsEqualIntV( evenOrOdd, _ones );
	}

	__forceinline unsigned int V4IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector )
	{
		Vector_4V negatedVector = V4Negate( boundsVector );
		return (V4IsLessThanOrEqualAll( testVector, boundsVector ) & V4IsGreaterThanOrEqualAll( testVector, negatedVector ));
	}

	__forceinline unsigned int V4IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return (result == 0xF ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zw_zw_zw_zwResult = V4Permute<Z, Z, Z, Z>( xy_yy_zw_wwResult );
		//Vector_4V xyzw_yyzw_zwzw_wwzwResult = V4And( xy_yy_zw_wwResult, zw_zw_zw_zwResult );
		//return ((xyzw_yyzw_zwzw_wwzwResult.m128_u32[0]) & (0x1));
	}

	__forceinline unsigned int V4IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return (result == 0x0 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualIntV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4Or( xyzwResult, yywwResult );
		//Vector_4V zw_zw_zw_zwResult = V4Permute<Z, Z, Z, Z>( xy_yy_zw_wwResult );
		//Vector_4V xyzw_yyzw_zwzw_wwzwResult = V4Or( xy_yy_zw_wwResult, zw_zw_zw_zwResult );
		//return ((xyzw_yyzw_zwzw_wwzwResult.m128_u32[0]) & (0x1)) ^ 0x1;
	}

	__forceinline Vector_4V_Out V4IsEqualIntV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V resultVect;
		__m128i tempIntVect1, tempIntVect2, tempIntVect3;
		tempIntVect1 = *(__m128i*)(&inVector1);
		tempIntVect2 = *(__m128i*)(&inVector2);

		tempIntVect3 = _mm_cmpeq_epi32( tempIntVect1, tempIntVect2 );
		resultVect = *(Vector_4V*)(&tempIntVect3);

		return resultVect;
	}

	__forceinline unsigned int V4IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return (result == 0xF ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zw_zw_zw_zwResult = V4Permute<Z, Z, Z, Z>( xy_yy_zw_wwResult );
		//Vector_4V xyzw_yyzw_zwzw_wwzwResult = V4And( xy_yy_zw_wwResult, zw_zw_zw_zwResult );
		//return ((xyzw_yyzw_zwzw_wwzwResult.m128_u32[0]) & (0x1));
	}

	__forceinline unsigned int V4IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		int result = _mm_movemask_ps( xyzwResult );
		return (result == 0x0 ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsEqualV( inVector1, inVector2 );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4Or( xyzwResult, yywwResult );
		//Vector_4V zw_zw_zw_zwResult = V4Permute<Z, Z, Z, Z>( xy_yy_zw_wwResult );
		//Vector_4V xyzw_yyzw_zwzw_wwzwResult = V4Or( xy_yy_zw_wwResult, zw_zw_zw_zwResult );
		//return ((xyzw_yyzw_zwzw_wwzwResult.m128_u32[0]) & (0x1)) ^ 0x1;
	}

	__forceinline Vector_4V_Out V4IsEqualV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_cmpeq_ps( inVector1, inVector2 );
	}

	__forceinline unsigned int V4IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V xyzwResult = V4IsGreaterThanV( bigVector, smallVector );
		int result = _mm_movemask_ps( xyzwResult );
		return (result == 0xF ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsGreaterThanV( bigVector, smallVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zw_zw_zw_zwResult = V4Permute<Z, Z, Z, Z>( xy_yy_zw_wwResult );
		//Vector_4V xyzw_yyzw_zwzw_wwzwResult = V4And( xy_yy_zw_wwResult, zw_zw_zw_zwResult );
		//return ((xyzw_yyzw_zwzw_wwzwResult.m128_u32[0]) & (0x1));
	}

	__forceinline Vector_4V_Out V4IsGreaterThanV(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		return _mm_cmpgt_ps( bigVector, smallVector );
	}

	__forceinline Vector_4V_Out V4IsGreaterThanIntV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_castsi128_ps(_mm_cmpgt_epi32(_mm_castps_si128(inVector1), _mm_castps_si128(inVector2)));
	}

	__forceinline unsigned int V4IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		Vector_4V xyzwResult = V4IsGreaterThanOrEqualV( bigVector, smallVector );
		int result = _mm_movemask_ps( xyzwResult );
		return (result == 0xF ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsGreaterThanOrEqualV( bigVector, smallVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zw_zw_zw_zwResult = V4Permute<Z, Z, Z, Z>( xy_yy_zw_wwResult );
		//Vector_4V xyzw_yyzw_zwzw_wwzwResult = V4And( xy_yy_zw_wwResult, zw_zw_zw_zwResult );
		//return ((xyzw_yyzw_zwzw_wwzwResult.m128_u32[0]) & (0x1));
	}

	__forceinline Vector_4V_Out V4IsGreaterThanOrEqualV(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		return _mm_cmpge_ps( bigVector, smallVector );
	}

	__forceinline unsigned int V4IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V xyzwResult = V4IsLessThanV( smallVector, bigVector );
		int result = _mm_movemask_ps( xyzwResult );
		return (result == 0xF ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsLessThanV( smallVector, bigVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zw_zw_zw_zwResult = V4Permute<Z, Z, Z, Z>( xy_yy_zw_wwResult );
		//Vector_4V xyzw_yyzw_zwzw_wwzwResult = V4And( xy_yy_zw_wwResult, zw_zw_zw_zwResult );
		//return ((xyzw_yyzw_zwzw_wwzwResult.m128_u32[0]) & (0x1));
	}

	__forceinline Vector_4V_Out V4IsLessThanV(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		return _mm_cmplt_ps( smallVector, bigVector );
	}

	__forceinline unsigned int V4IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		Vector_4V xyzwResult = V4IsLessThanOrEqualV( smallVector, bigVector );
		int result = _mm_movemask_ps( xyzwResult );
		return (result == 0xF ? 1u : 0u);

		//Vector_4V xyzwResult = V4IsLessThanOrEqualV( smallVector, bigVector );
		//Vector_4V yywwResult = V4Permute<Y, Y, W, W>( xyzwResult );
		//Vector_4V xy_yy_zw_wwResult = V4And( xyzwResult, yywwResult );
		//Vector_4V zw_zw_zw_zwResult = V4Permute<Z, Z, Z, Z>( xy_yy_zw_wwResult );
		//Vector_4V xyzw_yyzw_zwzw_wwzwResult = V4And( xy_yy_zw_wwResult, zw_zw_zw_zwResult );
		//return ((xyzw_yyzw_zwzw_wwzwResult.m128_u32[0]) & (0x1));
	}

	__forceinline Vector_4V_Out V4IsLessThanOrEqualV(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		return _mm_cmple_ps( smallVector, bigVector );
	}

	__forceinline Vector_4V_Out V4SelectFT(Vector_4V_In choiceVector, Vector_4V_In zero, Vector_4V_In nonZero)
	{
		return _mm_or_ps( _mm_andnot_ps(choiceVector, zero), _mm_and_ps(choiceVector, nonZero) );
	}

	__forceinline Vector_4V_Out V4Max(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_max_ps( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Min(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_min_ps( inVector1, inVector2 );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftLeft( Vector_4V_In inVector )
	{
		return _mm_castsi128_ps( _mm_slli_epi32( _mm_castps_si128( inVector ), shift ) );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftRight( Vector_4V_In inVector )
	{
		return _mm_castsi128_ps( _mm_srli_epi32( _mm_castps_si128( inVector ), shift ) );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftRightAlgebraic( Vector_4V_In inVector )
	{
		return _mm_castsi128_ps( _mm_srai_epi32( _mm_castps_si128( inVector ), shift ) );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128LeftBytes( Vector_4V_In inVector )
	{
		return _mm_castsi128_ps( _mm_slli_si128( _mm_castps_si128( inVector ), shift ) );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128RightBytes( Vector_4V_In inVector )
	{
		return _mm_castsi128_ps( _mm_srli_si128( _mm_castps_si128( inVector ), shift ) );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128LeftBytes_UndefBytes( Vector_4V_In inVector )
	{
		return _mm_castsi128_ps( _mm_slli_si128( _mm_castps_si128( inVector ), shift ) );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128RightBytes_UndefBytes( Vector_4V_In inVector )
	{
		return _mm_castsi128_ps( _mm_srli_si128( _mm_castps_si128( inVector ), shift ) );
	}

	__forceinline Vector_4V_Out V4And(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_and_ps( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Or(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_or_ps( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Xor(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_xor_ps( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Andc(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_andnot_ps( inVector2, inVector1 );
	}

	__forceinline Vector_4V_Out V4MergeXY(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_unpacklo_ps( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeZW(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return _mm_unpackhi_ps( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeXYShort(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		__m128i tempA = *(__m128i*)(&inVector1);
		__m128i tempB = *(__m128i*)(&inVector2);
		__m128i tempVec = _mm_unpacklo_epi16( tempA, tempB );
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4MergeZWShort(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		__m128i tempA = *(__m128i*)(&inVector1);
		__m128i tempB = *(__m128i*)(&inVector2);
		__m128i tempVec = _mm_unpackhi_epi16( tempA, tempB );
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4MergeXYByte(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		__m128i tempA = *(__m128i*)(&inVector1);
		__m128i tempB = *(__m128i*)(&inVector2);
		__m128i tempVec = _mm_unpacklo_epi8( tempA, tempB );
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4MergeZWByte(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		__m128i tempA = *(__m128i*)(&inVector1);
		__m128i tempB = *(__m128i*)(&inVector2);
		__m128i tempVec = _mm_unpackhi_epi8( tempA, tempB );
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4UnpackLowUnsignedShort(Vector_4V_In inVector)
	{
		return _mm_castsi128_ps(_mm_unpacklo_epi16(_mm_castps_si128(inVector), _mm_setzero_si128()));
	}

	__forceinline Vector_4V_Out V4UnpackLowUnsignedByte(Vector_4V_In inVector)
	{
		return _mm_castsi128_ps(_mm_unpacklo_epi8(_mm_castps_si128(inVector), _mm_setzero_si128()));
	}

	__forceinline Vector_4V_Out V4UnpackHighUnsignedShort(Vector_4V_In inVector)
	{
		return _mm_castsi128_ps(_mm_unpackhi_epi16(_mm_castps_si128(inVector), _mm_setzero_si128()));
	}

	__forceinline Vector_4V_Out V4UnpackHighUnsignedByte(Vector_4V_In inVector)
	{
		return _mm_castsi128_ps(_mm_unpackhi_epi8(_mm_castps_si128(inVector), _mm_setzero_si128()));
	}

	__forceinline Vector_4V_Out V4UnpackHighSignedShort(Vector_4V_In inVector)
	{
		const __m128i inputInts = *(__m128i*)(&inVector);
		const __m128i tempVec = _mm_srai_epi32(_mm_unpackhi_epi16(inputInts,inputInts), 16);
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4UnpackHighSignedByte(Vector_4V_In inVector)
	{
		const __m128i inputInts = *(__m128i*)(&inVector);
		const __m128i tempVec = _mm_srai_epi16(_mm_unpackhi_epi8(inputInts,inputInts), 8);
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4UnpackLowSignedShort(Vector_4V_In inVector)
	{
		const __m128i inputInts = *(__m128i*)(&inVector);
		const __m128i tempVec = _mm_srai_epi32(_mm_unpacklo_epi16(inputInts,inputInts), 16);
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4UnpackLowSignedByte(Vector_4V_In inVector)
	{
		const __m128i inputInts = *(__m128i*)(&inVector);
		const __m128i tempVec = _mm_srai_epi16(_mm_unpacklo_epi8(inputInts,inputInts), 8);
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4PackSignedIntToSignedShort(Vector_4V_In in0, Vector_4V_In in1)
	{
		const __m128i inputInts0 = *(__m128i*)(&in0);
		const __m128i inputInts1 = *(__m128i*)(&in1);
		const __m128i tempVec = _mm_packs_epi32(inputInts0, inputInts1);
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4PackSignedIntToUnsignedShort(Vector_4V_In in0, Vector_4V_In in1)
	{
		const __m128i inputInts0 = *(__m128i*)(&in0);
		const __m128i inputInts1 = *(__m128i*)(&in1);
#if 0 // SSE4
		const __m128i tempVec = _mm_packus_epi32(inputInts0, inputInts1);
		return *(Vector_4V*)(&tempVec);
#else
		// http://sseplus.sourceforge.net/group__emulated___s_s_e2.html
		const __m128i mask32 = _mm_set1_epi32(0x00008000);
		const __m128i mask16 = _mm_set1_epi32(0x80008000);
		__m128i temp0, temp1;
		temp0 = _mm_sub_epi32(inputInts0, mask32);
		temp1 = _mm_sub_epi32(inputInts1, mask32);
		temp0 = _mm_packs_epi32(temp0, temp1);
		temp0 = _mm_add_epi16(temp0, mask16);
		return *(Vector_4V*)(&temp0);
#endif
	}

	__forceinline Vector_4V_Out V4PackSignedShortToSignedByte(Vector_4V_In in0, Vector_4V_In in1)
	{
		const __m128i inputInts0 = *(__m128i*)(&in0);
		const __m128i inputInts1 = *(__m128i*)(&in1);
		const __m128i tempVec = _mm_packs_epi16(inputInts0, inputInts1);
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4PackSignedShortToUnsignedByte(Vector_4V_In in0, Vector_4V_In in1)
	{
		const __m128i inputInts0 = *(__m128i*)(&in0);
		const __m128i inputInts1 = *(__m128i*)(&in1);
		const __m128i tempVec = _mm_packus_epi16(inputInts0, inputInts1);
		return *(Vector_4V*)(&tempVec);
	}

	__forceinline Vector_4V_Out V4PackFloatToGpuSignedShortAccurate(Vector_4V_In in)
	{
		// Direct3D 11 specifies that the GPU does uses the range [0x8001..0x7fff]
		// for _SNORM (0x8000 also maps to -1.f).
		Vector_4V negOnef   = V4VConstant(V_NEGONE);
		Vector_4V posOnef   = V4VConstant(V_ONE);
		Vector_4V sat       = V4Clamp(in, negOnef, posOnef);
		Vector_4V scaledPos = V4Scale(sat, V4VConstantSplat<FLOAT_TO_INT(1.f*0x7fff0000)>());
		Vector_4V integer32 = V4FloatToIntRaw<0>(scaledPos);
		Vector_4V shr15     = V4ShiftRight<15>(integer32);
		Vector_4V shr16     = V4ShiftRightAlgebraic<16>(integer32);
		Vector_4V onei      = V4VConstant(V_INT_1);
		Vector_4V rounding  = V4And(shr15, onei);
		Vector_4V integer16 = V4AddInt(shr16, rounding);
		Vector_4V packed    = V4PackSignedIntToSignedShort(integer16, integer16);
		return packed;
	}

	__forceinline Vector_4V_Out V4UnpackLowGpuSignedShortToFloatAccurate(Vector_4V_In in)
	{
		Vector_4V integer32 = V4UnpackLowSignedShort(in);
		Vector_4V scale     = V4VConstantSplat<FLOAT_TO_INT(1.f/0x7fff)>();
		Vector_4V float_    = V4Scale(V4IntToFloatRaw<0>(integer32), scale);
		return float_;
	}

	__forceinline Vector_4V_Out V4UnpackHighGpuSignedShortToFloatAccurate(Vector_4V_In in)
	{
		Vector_4V integer32 = V4UnpackHighSignedShort(in);
		Vector_4V scale     = V4VConstantSplat<FLOAT_TO_INT(1.f/0x7fff)>();
		Vector_4V float_    = V4Scale(V4IntToFloatRaw<0>(integer32), scale);
		return float_;
	}

	__forceinline Vector_4V_Out V4PackNormFloats_11_11_10(Vector_4V_In in)
	{
		__m128 c = _mm_set_ps(0.f, (float)0x1ff, (float)0x3ff, (float)0x3ff);
		in = _mm_mul_ps(in, c);
		__m128 mx = c;
		__m128 mn = _mm_sub_ps(_mm_setzero_ps(), c);
		in = _mm_min_ps(in, mx);
		in = _mm_max_ps(in, mn);
		__m128i i = _mm_cvttps_epi32(in);
		__m128i mask = _mm_set_epi32(0, 0x3ff, 0x7ff, 0x7ff);
		i = _mm_and_si128(i, mask);
		__m128i x = i;
		__m128i y = _mm_shuffle_epi32(i, 0x55);
		__m128i z = _mm_shuffle_epi32(i, 0xaa);
		y = _mm_slli_epi32(y, 11);
		z = _mm_slli_epi32(z, 22);
		__m128i yx = _mm_or_si128(y, x);
		__m128i zyx = _mm_or_si128(z, yx);
		return (Vector_4V)_mm_castsi128_ps(zyx);
	}

	__forceinline Vector_4V_Out V4PackNormFloats_10_10_10_2(Vector_4V_In in)
	{
#		if (RSG_ORBIS && 0)
			__m128 c = _mm_set1_ps((float)0x1ff);
			in = _mm_mul_ps(in, c);
			__m128 zero = _mm_setzero_ps();
			__m128 mx = c;
			__m128 mn = _mm_sub_ps(zero, c);
			in = _mm_min_ps(in, mx);
			in = _mm_max_ps(in, mn);
			__m128i i = _mm_cvttps_epi32(in);
			__m128i mask = _mm_set_epi32(0, 0x3ff, 0x3ff, 0x3ff);
			i = _mm_and_si128(i, mask);
			__m128i x = i;
			__m128i y = _mm_shuffle_epi32(i, 0x55);
			__m128i z = _mm_shuffle_epi32(i, 0xaa);
			y = _mm_slli_epi32(y, 10);
			z = _mm_slli_epi32(z, 20);
			__m128 w = _mm_sub_ps(zero, in);
			w = _mm_mul_ps(zero, w);
			w = _mm_srai_epi32(w, 1);
			w = _mm_shuffle_ps(w, w, 0xff);
			__m128i yx = _mm_or_si128(y, x);
			__m128i wz = _mm_or_si128(_mm_castps_si128(w), z);
			__m128i wzyx = _mm_or_si128(wz, yx);
			return (Vector_4V)_mm_castsi128_ps(wzyx);
#		else
			__m128 c = _mm_set_ps(0x3/2.f, 0x3ff/2.f, 0x3ff/2.f, 0x3ff/2.f);
			in = _mm_mul_ps(in, c);
			in = _mm_add_ps(in, c);
			__m128 mn = _mm_setzero_ps();
			__m128 mx = _mm_set_ps((float)0x3, (float)0x3ff, (float)0x3ff, (float)0x3ff);
			in = _mm_max_ps(in, mn);
			in = _mm_min_ps(in, mx);
			__m128i i = _mm_cvtps_epi32(in);
			__m128i x = i;
			__m128i y = _mm_shuffle_epi32(i, 0x55);
			__m128i z = _mm_shuffle_epi32(i, 0xaa);
			__m128i w = _mm_shuffle_epi32(i, 0xff);
			y = _mm_slli_epi32(y, 10);
			z = _mm_slli_epi32(z, 20);
			w = _mm_slli_epi32(w, 30);
			__m128i yx = _mm_or_si128(y, x);
			__m128i wz = _mm_or_si128(w, z);
			__m128i wzyx = _mm_or_si128(wz, yx);
			return (Vector_4V)_mm_castsi128_ps(wzyx);
#		endif
	}

	__forceinline Vector_4V_Out V4PackNormFloats_10_10_10_X(Vector_4V_In in)
	{
#		if (RSG_ORBIS && 0)
			__m128 c = _mm_set1_ps((float)0x1ff);
			in = _mm_mul_ps(in, c);
			__m128 zero = _mm_setzero_ps();
			__m128 mx = c;
			__m128 mn = _mm_sub_ps(zero, c);
			in = _mm_min_ps(in, mx);
			in = _mm_max_ps(in, mn);
			__m128i i = _mm_cvttps_epi32(in);
			__m128i mask = _mm_set_epi32(0, 0x3ff, 0x3ff, 0x3ff);
			i = _mm_and_si128(i, mask);
			__m128i x = i;
			__m128i y = _mm_shuffle_epi32(i, 0x55);
			__m128i z = _mm_shuffle_epi32(i, 0xaa);
			y = _mm_slli_epi32(y, 10);
			z = _mm_slli_epi32(z, 20);
			__m128i yx = _mm_or_si128(y, x);
			__m128i zyx = _mm_or_si128(z, yx);
			return (Vector_4V)_mm_castsi128_ps(zyx);
#		else
			__m128 c = _mm_set1_ps(0x3ff/2.f);
			in = _mm_mul_ps(in, c);
			in = _mm_add_ps(in, c);
			__m128 mn = _mm_setzero_ps();
			__m128 mx = _mm_set1_ps((float)0x3ff);
			in = _mm_max_ps(in, mn);
			in = _mm_min_ps(in, mx);
			__m128i i = _mm_cvtps_epi32(in);
			__m128i x = i;
			__m128i y = _mm_shuffle_epi32(i, 0x55);
			__m128i z = _mm_shuffle_epi32(i, 0xaa);
			y = _mm_slli_epi32(y, 10);
			z = _mm_slli_epi32(z, 20);
			__m128i yx = _mm_or_si128(y, x);
			__m128i zyx = _mm_or_si128(z, yx);
			return (Vector_4V)_mm_castsi128_ps(zyx);
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_11_11_10(Vector_4V_In in)
	{
		__m128i i = _mm_castps_si128(in);
		__m128i x = _mm_slli_epi32(i, 21);
		__m128i y = _mm_slli_epi32(i, 10);
		__m128i z = i;
		x = _mm_srai_epi32(x, 21);
		y = _mm_srai_epi32(y, 21);
		z = _mm_srai_epi32(z, 22);
		__m128 xxyy = _mm_shuffle_ps(_mm_castsi128_ps(x), _mm_castsi128_ps(y), 0x00);
		__m128 zzzz = _mm_shuffle_ps(_mm_castsi128_ps(z), _mm_castsi128_ps(z), 0x00);
		__m128 xyz  = _mm_shuffle_ps(xxyy, zzzz, 0x88);
		xyz = _mm_cvtepi32_ps(_mm_castps_si128(xyz));
		__m128 cmul = _mm_set_ps(0.f, 1.f/0x1ff, 1.f/0x3ff, 1.f/0x3ff);
		xyz = _mm_mul_ps(xyz, cmul);
		return (Vector_4V)xyz;
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_10_10_10_2(Vector_4V_In in)
	{
#		if (RSG_ORBIS && 0)
			__m128i i = _mm_castps_si128(in);
			__m128i x = _mm_slli_epi32(i, 22);
			__m128i y = _mm_slli_epi32(i, 12);
			__m128i z = _mm_slli_epi32(i, 2);
			__m128i w = i;
			x = _mm_srai_epi32(x, 22);
			y = _mm_srai_epi32(y, 22);
			z = _mm_srai_epi32(z, 22);
			w = _mm_srai_epi32(w, 31);
			__m128 xxyy = _mm_shuffle_ps(_mm_castsi128_ps(x), _mm_castsi128_ps(y), 0x00);
			__m128 zzww = _mm_shuffle_ps(_mm_castsi128_ps(z), _mm_castsi128_ps(w), 0x00);
			__m128 xyzw = _mm_shuffle_ps(xxyy, zzww, 0x88);
			xyzw = _mm_cvtepi32_ps(_mm_castps_si128(xyzw));
			__m128 cmul = _mm_set_ps(-2.f, 1.f/0x1ff, 1.f/0x1ff, 1.f/0x1ff);
			__m128 cadd = _mm_set_ps(-1.f, 0.f, 0.f, 0.f);
			xyzw = _mm_mul_ps(xyzw, cmul);
			xyzw = _mm_add_ps(xyzw, cadd);
			return (Vector_4V)xyzw;
#		else
			__m128i i = _mm_castps_si128(in);
			__m128i xyzmask = _mm_set1_epi32(0x000003ff);
			__m128i x = i;
			__m128i y = _mm_srli_epi32(i, 10);
			__m128i z = _mm_srli_epi32(i, 20);
			__m128i w = _mm_srli_epi32(i, 30);
			x = _mm_and_si128(x, xyzmask);
			y = _mm_and_si128(y, xyzmask);
			z = _mm_and_si128(z, xyzmask);
			__m128 xxyy = _mm_shuffle_ps(_mm_castsi128_ps(x), _mm_castsi128_ps(y), 0x00);
			__m128 zzww = _mm_shuffle_ps(_mm_castsi128_ps(z), _mm_castsi128_ps(w), 0x00);
			__m128 xyzw = _mm_shuffle_ps(xxyy, zzww, 0x88);
			xyzw = _mm_cvtepi32_ps(_mm_castps_si128(xyzw));
			__m128 cmul = _mm_set_ps(2.f/0x3, 2.f/0x3ff, 2.f/0x3ff, 2.f/0x3ff);
			__m128 cadd = _mm_set1_ps(-1.f);
			xyzw = _mm_mul_ps(xyzw, cmul);
			xyzw = _mm_add_ps(xyzw, cadd);
			return (Vector_4V)xyzw;
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_10_10_10_X(Vector_4V_In in)
	{
#		if (RSG_ORBIS && 0)
			__m128i i = _mm_castps_si128(in);
			__m128i x = _mm_slli_epi32(i, 22);
			__m128i y = _mm_slli_epi32(i, 12);
			__m128i z = _mm_slli_epi32(i, 2);
			x = _mm_srai_epi32(x, 22);
			y = _mm_srai_epi32(y, 22);
			z = _mm_srai_epi32(z, 22);
			__m128 xxyy = _mm_shuffle_ps(_mm_castsi128_ps(x), _mm_castsi128_ps(y), 0x00);
			__m128 zzzz = _mm_shuffle_ps(_mm_castsi128_ps(z), _mm_castsi128_ps(z), 0x00);
			__m128 xyz  = _mm_shuffle_ps(xxyy, zzzz, 0x88);
			xyz = _mm_cvtepi32_ps(_mm_castps_si128(xyz));
			__m128 cmul = _mm_set1_ps(1.f/0x1ff);
			xyz = _mm_mul_ps(xyz, cmul);
			return (Vector_4V)xyz;
#		else
			__m128i i = _mm_castps_si128(in);
			__m128i xyzmask = _mm_set1_epi32(0x000003ff);
			__m128i x = i;
			__m128i y = _mm_srli_epi32(i, 10);
			__m128i z = _mm_srli_epi32(i, 20);
			x = _mm_and_si128(x, xyzmask);
			y = _mm_and_si128(y, xyzmask);
			z = _mm_and_si128(z, xyzmask);
			__m128 xxyy = _mm_shuffle_ps(_mm_castsi128_ps(x), _mm_castsi128_ps(y), 0x00);
			__m128 xyz = _mm_shuffle_ps(xxyy, _mm_castsi128_ps(z), 0x08);
			xyz = _mm_cvtepi32_ps(_mm_castps_si128(xyz));
			__m128 cmul = _mm_set1_ps(2.f/0x3ff);
			__m128 cadd = _mm_set1_ps(-1.f);
			xyz = _mm_mul_ps(xyz, cmul);
			xyz = _mm_add_ps(xyz, cadd);
			return (Vector_4V)xyz;
#		endif
	}

	//============================================================================
	// Permute functions

	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline Vector_4V_Out V4Permute( Vector_4V_In v )
	{
		CompileTimeAssert(	(permX==X || permX==Y || permX==Z || permX==W) &&
							(permY==X || permY==Y || permY==Z || permY==W) &&
							(permZ==X || permZ==Y || permZ==Z || permZ==W) &&
							(permW==X || permW==Y || permW==Z || permW==W)  ); // Invalid permute args!

		CompileTimeAssert( !(permX==X && permY==Y && permZ==Z && permW==W) ); // This permute does nothing meaningful!

		// Converts 0x00010203/0x04050607/0x08090A0B/0x0C0D0E0F and 0x10111213/0x14151617/0x18191A1B/0x1C1D1E1F to 0/1/2/3
#define MASK_IT(x) ( ((x) & 0xC) >> 2 )
		__m128i tempInput = *(reinterpret_cast<__m128i*>(&v));
		__m128i tempOutput = _mm_shuffle_epi32( tempInput, SWIZZLE_VAL_WIN32( MASK_IT(permX), MASK_IT(permY), MASK_IT(permZ), MASK_IT(permW) ) );
		return *(reinterpret_cast<__m128*>(&tempOutput));

		// Old way: has two input regs, causing more 'movaps' calls (and thus register spilling) to set up inputs.
		//return _mm_shuffle_ps( v, v, SWIZZLE_VAL_WIN32( MASK_IT(permX), MASK_IT(permY), MASK_IT(permZ), MASK_IT(permW) ) );
#undef MASK_IT
	}

	/// Note: All branches in this function are optimized out at compile-time. Boils down to either 1 or 2 SSE instructions.
	template <u32 permX, u32 permY, u32 permZ, u32 permW>
	__forceinline Vector_4V_Out V4PermuteTwo( Vector_4V_In v1, Vector_4V_In v2 )
	{
		CompileTimeAssert(	(permX==X1 || permX==Y1 || permX==Z1 || permX==W1	||
							permX==X2 || permX==Y2 || permX==Z2 || permX==W2)	&&
							(permY==X1 || permY==Y1 || permY==Z1 || permY==W1	||
							permY==X2 || permY==Y2 || permY==Z2 || permY==W2)	&&
							(permZ==X1 || permZ==Y1 || permZ==Z1 || permZ==W1	||
							permZ==X2 || permZ==Y2 || permZ==Z2 || permZ==W2)	&&
							(permW==X1 || permW==Y1 || permW==Z1 || permW==W1	||
							permW==X2 || permW==Y2 || permW==Z2 || permW==W2)	 );
							// Invalid permute args!

		CompileTimeAssert(	!(permX==X1 && permY==Y1 && permZ==Z1 && permW==W1) &&
							!(permX==X2 && permY==Y2 && permZ==Z2 && permW==W2) );
							// This permute does nothing meaningful!

		CompileTimeAssert(	!(((permX)&(0xF0F0F0F0)) != 0 && ((permY)&(0xF0F0F0F0)) != 0 && ((permZ)&(0xF0F0F0F0)) != 0 && ((permW)&(0xF0F0F0F0)) != 0)	&&
							!(((permX)&(0xF0F0F0F0)) == 0 && ((permY)&(0xF0F0F0F0)) == 0 && ((permZ)&(0xF0F0F0F0)) == 0 && ((permW)&(0xF0F0F0F0)) == 0) );
							// You should be using V4Permute<>()! It's faster on some platforms!

		// Converts 0x00010203/0x04050607/0x08090A0B/0x0C0D0E0F and 0x10111213/0x14151617/0x18191A1B/0x1C1D1E1F to 0/1/2/3
#define MASK_IT(x) ( ((x) & 0xC) >> 2 )

		u32 vectX, vectY, vectZ, vectW;

		switch( permX )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			vectX = 0;
			break;
		default:
			vectX = 1;
			break;
		};
		switch( permY )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			vectY = 0;
			break;
		default:
			vectY = 1;
			break;
		};
		switch( permZ )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			vectZ = 0;
			break;
		default:
			vectZ = 1;
			break;
		};
		switch( permW )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			vectW = 0;
			break;
		default:
			vectW = 1;
			break;
		};

		// TRIVIAL CASE #1 (handled natively by SSE)
		// (1 instruction)
		if( vectX == vectY && vectZ == vectW )
		{
			if( vectX == 0 ) // so vectX=0, vectY=0, vectZ=1, vectW=1
			{
				return _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(permY), MASK_IT(permZ), MASK_IT(permW)) );
			}
			else // so vectX=1, vectY=1, vectZ=0, vectW=0
			{
				return _mm_shuffle_ps( v2, v1, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(permY), MASK_IT(permZ), MASK_IT(permW)) );
			}
		}

		// NONTRIVIAL CASE #1 (Two components are needed from each vector)
		// Note that "( vectX==0 && vectY==0 )" and "( vectZ==0 && vectW==0 )" should already be handled by other cases, and so are not handled here.
		// (2 instructions)
		else if(	(vectX==0 && vectY==1 && vectZ==0 && vectW==1)	||
					(vectX==0 && vectY==1 && vectZ==1 && vectW==0)	||
					(vectX==1 && vectY==0 && vectZ==0 && vectW==1)	||
					(vectX==1 && vectY==0 && vectZ==1 && vectW==0)	)
		{
			if( vectX==0 && vectZ==0 )
			{
				// These lines that are commented out had led to looooong compilation times. So they're inlined here.
				//Vector_4V combinedVect = V4PermuteTwo<permX, PERM_V1, permZ, PERM_V1, permY, PERM_V2, permW, PERM_V2>(v1, v2);
				// return V4PermuteXZYW( combinedVect );
				Vector_4V combinedVect = _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(permZ), MASK_IT(permY), MASK_IT(permW)) );
				__m128i result = _mm_shuffle_epi32( *(reinterpret_cast<__m128i*>(&combinedVect)), SWIZZLE_VAL_WIN32( MASK_IT(X), MASK_IT(Z), MASK_IT(Y), MASK_IT(W) ) );
				return *(reinterpret_cast<__m128*>(&result));
			}
			else if( vectX==0 && vectW==0 )
			{
				//Vector_4V combinedVect = V4PermuteTwo<permX, PERM_V1, permW, PERM_V1, permY, PERM_V2, permZ, PERM_V2>(v1, v2);
				// return V4PermuteXZWY( combinedVect );
				Vector_4V combinedVect = _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permX),MASK_IT( permW), MASK_IT(permY), MASK_IT(permZ)) );
				__m128i result = _mm_shuffle_epi32( *(reinterpret_cast<__m128i*>(&combinedVect)), SWIZZLE_VAL_WIN32( MASK_IT(X), MASK_IT(Z), MASK_IT(W), MASK_IT(Y) ) );
				return *(reinterpret_cast<__m128*>(&result));
			}
			else if( vectY==0 && vectZ==0 )
			{
				//Vector_4V combinedVect = V4PermuteTwo<permY, PERM_V1, permZ, PERM_V1, permX, PERM_V2, permW, PERM_V2>(v1, v2);
				// return V4PermuteZXYW( combinedVect );
				Vector_4V combinedVect = _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permY), MASK_IT(permZ), MASK_IT(permX), MASK_IT(permW)) );
				__m128i result = _mm_shuffle_epi32( *(reinterpret_cast<__m128i*>(&combinedVect)), SWIZZLE_VAL_WIN32( MASK_IT(Z), MASK_IT(X), MASK_IT(Y), MASK_IT(W) ) );
				return *(reinterpret_cast<__m128*>(&result));
			}
			else // vectY==0 && vectW==0
			{
				//Vector_4V combinedVect = V4PermuteTwo<permY, PERM_V1, permW, PERM_V1, permX, PERM_V2, permZ, PERM_V2>(v1, v2);
				// return V4PermuteZXWY( combinedVect );
				Vector_4V combinedVect = _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permY), MASK_IT(permW), MASK_IT(permX), MASK_IT(permZ)) );
				__m128i result = _mm_shuffle_epi32( *(reinterpret_cast<__m128i*>(&combinedVect)), SWIZZLE_VAL_WIN32( MASK_IT(Z), MASK_IT(X), MASK_IT(W), MASK_IT(Y) ) );
				return *(reinterpret_cast<__m128*>(&result));
			}
		}

		// NONTRIVIAL CASE #2 (Three components are needed from one vector, one component is needed from the other)
		// (2 instructions)
		else if(	(vectX==0 && vectY==1 && vectZ==1 && vectW==1)	||
					(vectX==1 && vectY==0 && vectZ==1 && vectW==1)	||
					(vectX==1 && vectY==1 && vectZ==0 && vectW==1)	||
					(vectX==1 && vectY==1 && vectZ==1 && vectW==0)	||
					(vectX==1 && vectY==0 && vectZ==0 && vectW==0)	||
					(vectX==0 && vectY==1 && vectZ==0 && vectW==0)	||
					(vectX==0 && vectY==0 && vectZ==1 && vectW==0)	||
					(vectX==0 && vectY==0 && vectZ==0 && vectW==1)	)
		{
			// If 1 component is needed from v1...
			if(		(vectX==0 && vectY==1 && vectZ==1 && vectW==1)	||
					(vectX==1 && vectY==0 && vectZ==1 && vectW==1)	||
					(vectX==1 && vectY==1 && vectZ==0 && vectW==1)	||
					(vectX==1 && vectY==1 && vectZ==1 && vectW==0)	)
			{
				if( vectX==0 )
				{
					//Vector_4V combinedVect = V4PermuteTwo<permX, PERM_V1, W, PERM_V1, permY, PERM_V2, W, PERM_V2>(v1, v2);
					//return V4PermuteTwo<X, PERM_V1, Z, PERM_V1, permZ, PERM_V2, permW, PERM_V2>( combinedVect, v2 );
					Vector_4V combinedVect = _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(W), MASK_IT(permY), MASK_IT(W)) );
					return _mm_shuffle_ps( combinedVect, v2, SWIZZLE_VAL_WIN32(MASK_IT(X), MASK_IT(Z), MASK_IT(permZ), MASK_IT(permW)) );
				}
				else if( vectY==0 )
				{
					//Vector_4V combinedVect = V4PermuteTwo<permX, PERM_V2, W, PERM_V2, permY, PERM_V1, W, PERM_V1>(v1, v2);
					//return V4PermuteTwo<X, PERM_V1, Z, PERM_V1, permZ, PERM_V2, permW, PERM_V2>( combinedVect, v2 );
					Vector_4V combinedVect = _mm_shuffle_ps( v2, v1, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(W), MASK_IT(permY), MASK_IT(W)) );
					return _mm_shuffle_ps( combinedVect, v2, SWIZZLE_VAL_WIN32(MASK_IT(X), MASK_IT(Z), MASK_IT(permZ), MASK_IT(permW)) );
				}
				else if( vectZ==0 )
				{
					//Vector_4V combinedVect = V4PermuteTwo<permZ, PERM_V1, W, PERM_V1, permW, PERM_V2, W, PERM_V2>(v1, v2);
					//return V4PermuteTwo<permX, PERM_V2, permY, PERM_V2, X, PERM_V1, Z, PERM_V1>( combinedVect, v2 );
					Vector_4V combinedVect = _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permZ), MASK_IT(W), MASK_IT(permW), MASK_IT(W)) );
					return _mm_shuffle_ps( v2, combinedVect, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(permY), MASK_IT(X), MASK_IT(Z)) );
				}
				else // vectW==0
				{
					//Vector_4V combinedVect = V4PermuteTwo<permZ, PERM_V2, W, PERM_V2, permW, PERM_V1, W, PERM_V1>(v1, v2);
					//return V4PermuteTwo<permX, PERM_V2, permY, PERM_V2, X, PERM_V1, Z, PERM_V1>( combinedVect, v2 );
					Vector_4V combinedVect = _mm_shuffle_ps( v2, v1, SWIZZLE_VAL_WIN32(MASK_IT(permZ), MASK_IT(W), MASK_IT(permW), MASK_IT(W)) );
					return _mm_shuffle_ps( v2, combinedVect, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(permY), MASK_IT(X), MASK_IT(Z)) );
				}
			}

			// If 1 component is needed from v2...
			else
			{
				if( vectX==1 )
				{
					//Vector_4V combinedVect = V4PermuteTwo<permX, PERM_V2, W, PERM_V2, permY, PERM_V1, W, PERM_V1>(v1, v2);
					//return V4PermuteTwo<X, PERM_V1, Z, PERM_V1, permZ, PERM_V2, permW, PERM_V2>( combinedVect, v1 );
					Vector_4V combinedVect = _mm_shuffle_ps( v2, v1, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(W), MASK_IT(permY), MASK_IT(W)) );
					return _mm_shuffle_ps( combinedVect, v1, SWIZZLE_VAL_WIN32(MASK_IT(X), MASK_IT(Z), MASK_IT(permZ), MASK_IT(permW)) );
				}
				else if( vectY==1 )
				{
					//Vector_4V combinedVect = V4PermuteTwo<permX, PERM_V1, W, PERM_V1, permY, PERM_V2, W, PERM_V2>(v1, v2);
					//return V4PermuteTwo<X, PERM_V1, Z, PERM_V1, permZ, PERM_V2, permW, PERM_V2>( combinedVect, v1 );
					Vector_4V combinedVect = _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(W), MASK_IT(permY), MASK_IT(W)) );
					return _mm_shuffle_ps( combinedVect, v1, SWIZZLE_VAL_WIN32(MASK_IT(X), MASK_IT(Z), MASK_IT(permZ), MASK_IT(permW)) );
				}
				else if( vectZ==1 )
				{
					//Vector_4V combinedVect = V4PermuteTwo<permZ, PERM_V2, W, PERM_V2, permW, PERM_V1, W, PERM_V1>(v1, v2);
					//return V4PermuteTwo<permX, PERM_V2, permY, PERM_V2, X, PERM_V1, Z, PERM_V1>( combinedVect, v1 );
					Vector_4V combinedVect = _mm_shuffle_ps( v2, v1, SWIZZLE_VAL_WIN32(MASK_IT(permZ), MASK_IT(W), MASK_IT(permW), MASK_IT(W)) );
					return _mm_shuffle_ps( v1, combinedVect, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(permY), MASK_IT(X), MASK_IT(Z)) );
				}
				else // vectW==1
				{
					//Vector_4V combinedVect = V4PermuteTwo<permZ, PERM_V1, W, PERM_V1, permW, PERM_V2, W, PERM_V2>(v1, v2);
					//return V4PermuteTwo<permX, PERM_V2, permY, PERM_V2, X, PERM_V1, Z, PERM_V1>( combinedVect, v1 );
					Vector_4V combinedVect = _mm_shuffle_ps( v1, v2, SWIZZLE_VAL_WIN32(MASK_IT(permZ), MASK_IT(W), MASK_IT(permW), MASK_IT(W)) );
					return _mm_shuffle_ps( v1, combinedVect, SWIZZLE_VAL_WIN32(MASK_IT(permX), MASK_IT(permY), MASK_IT(X), MASK_IT(Z)) );
				}
			}
		}

		// That's all the cases.
		else
		{
			mthErrorf("Unexpected case!" );
			return _mm_shuffle_ps( v1, v1, 0 );
		}
#undef MASK_IT
	}

	// Specialize for cases where it's faster.
	template <>
	__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y1,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
	{
		return V4MergeXY( v1, v2 );
	}
	template <>
	__forceinline Vector_4V_Out V4PermuteTwo<X2,X1,Y2,Y1>( Vector_4V_In v1, Vector_4V_In v2 )
	{
		return V4MergeXY( v2, v1 );
	}
	template <>
	__forceinline Vector_4V_Out V4PermuteTwo<Z1,Z2,W1,W2>( Vector_4V_In v1, Vector_4V_In v2 )
	{
		return V4MergeZW( v1, v2 );
	}
	template <>
	__forceinline Vector_4V_Out V4PermuteTwo<Z2,Z1,W2,W1>( Vector_4V_In v1, Vector_4V_In v2 )
	{
		return V4MergeZW( v2, v1 );
	}

	__forceinline Vector_4V_Out V4PermuteTwo( Vector_4V_In v1, Vector_4V_In v2, Vector_4V_In controlVect )
	{
		float x, y, z, w;
		float* ptrV1 = (float*)&v1;
		float* ptrV2 = (float*)&v2;

		union
		{
			float f;
			int i;
		} Temp1, Temp2, Temp3, Temp4;
		Temp1.f = GetX(controlVect);
		Temp2.f = GetY(controlVect);
		Temp3.f = GetZ(controlVect);
		Temp4.f = GetW(controlVect);

		// Converts 0x00010203/0x04050607/0x08090A0B/0x0C0D0E0F and 0x10111213/0x14151617/0x18191A1B/0x1C1D1E1F to 0/1/2/3
#define MASK_IT(x) ( ((x) & 0xC) >> 2 )

		switch( Temp1.i )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			x = ptrV1[MASK_IT(Temp1.i)];
			break;
		default:
			x = ptrV2[MASK_IT(Temp1.i)];
			break;
		};
		switch( Temp2.i )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			y = ptrV1[MASK_IT(Temp2.i)];
			break;
		default:
			y = ptrV2[MASK_IT(Temp2.i)];
			break;
		};
		switch( Temp3.i )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			z = ptrV1[MASK_IT(Temp3.i)];
			break;
		default:
			z = ptrV2[MASK_IT(Temp3.i)];
			break;
		};
		switch( Temp4.i )
		{
		case X1:
		case Y1:
		case Z1:
		case W1:
			w = ptrV1[MASK_IT(Temp4.i)];
			break;
		default:
			w = ptrV2[MASK_IT(Temp4.i)];
			break;
		};

#undef MASK_IT

		Vector_4V_Out outVect = VECTOR4V_LITERAL(x, y, z, w);
		return outVect;
	}

	//============================================================================
	// Quaternions

	__forceinline Vector_4V_Out V4QuatMultiply( Vector_4V_In inQ1, Vector_4V_In inQ2 )
	{
		// Reversing names just b/c they were wrong when implementing the function.
		Vector_4V inQuat1 = inQ2;
		Vector_4V inQuat2 = inQ1;

		Vector_4V negInQuat1;
		Vector_4V inQuat2x, inQuat2y, inQuat2z, inQuat2w;
		Vector_4V inQuat1wzyx, inQuat1zwxy, inQuat1yxwz;
		Vector_4V outVect;

		negInQuat1 = V4Negate(inQuat1);

		inQuat2w = V4SplatW(inQuat2);
		inQuat2x = V4SplatX(inQuat2);
		inQuat2y = V4SplatY(inQuat2);
		inQuat2z = V4SplatZ(inQuat2);

		outVect = V4Scale( inQuat1, inQuat2w );

		inQuat1wzyx = V4PermuteTwo<W1,Z2,Y1,X2>(inQuat1, negInQuat1);
		inQuat1zwxy = V4PermuteTwo<Z1,W1,X2,Y2>(inQuat1, negInQuat1);
		inQuat1yxwz = V4PermuteTwo<Y2,X1,W1,Z2>(inQuat1, negInQuat1);

		outVect = V4AddScaled(outVect, inQuat1wzyx, inQuat2x);
		outVect = V4AddScaled(outVect, inQuat1zwxy, inQuat2y);
		return V4AddScaled(outVect, inQuat1yxwz, inQuat2z);
	}

	
} // namespace Vec
} // namespace rage
