// 
// math/float16.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "float16.h"
#include "system/nelem.h"

namespace rage {

#if __DECLARESTRUCT

void Float16::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(Float16);
	STRUCT_FIELD(m_data);
	STRUCT_END();
}

#endif // __DECLARESTRUCT

#if 0
	#define ADDR_INCR(v,i) *(v++)
	#define ADDR_SKIP(v,i)
#else
	#define ADDR_INCR(v,i) v[i]
	#define ADDR_SKIP(v,i) v += (i)
#endif

namespace Vec  {

// [SSE TODO] -- try this for the timecycle blend
#if RSG_SSE_VERSION >= 50
void V4Float16StreamUnpackBlend4(
	__m256* RESTRICT dst,
	const __m256i* RESTRICT srcA, float scaleA,
	const __m256i* RESTRICT srcB, float scaleB,
	const __m256i* RESTRICT srcC, float scaleC,
	const __m256i* RESTRICT srcD, float scaleD,
	int count)
{
	count /= 8;
	int i = 0;

	const __m256 scaleA_v = _mm256_broadcast_ss(&scaleA);
	const __m256 scaleB_v = _mm256_broadcast_ss(&scaleB);
	const __m256 scaleC_v = _mm256_broadcast_ss(&scaleC);
	const __m256 scaleD_v = _mm256_broadcast_ss(&scaleD);

	for (const int block4 = count - 4; i <= block4; i += 4) // unpack 4 at a time
	{
		__m256 temp0;
		__m256 temp1;
		__m256 temp2;
		__m256 temp3;

		const __m256i a01 = _mm256_load_si256(srcA + 0);
		const __m256i a23 = _mm256_load_si256(srcA + 1);
		const __m256i b01 = _mm256_load_si256(srcB + 0);
		const __m256i b23 = _mm256_load_si256(srcB + 1);
		const __m256i c01 = _mm256_load_si256(srcC + 0);
		const __m256i c23 = _mm256_load_si256(srcC + 1);
		const __m256i d01 = _mm256_load_si256(srcD + 0);
		const __m256i d23 = _mm256_load_si256(srcD + 1);

		temp0 = _mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (a01   )), scaleA_v);
		temp1 = _mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(a01, 1)), scaleA_v);
		temp2 = _mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (a23   )), scaleA_v);
		temp3 = _mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(a23, 1)), scaleA_v);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (b01   )), scaleB_v), temp0);
		temp1 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(b01, 1)), scaleB_v), temp1);
		temp2 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (b23   )), scaleB_v), temp2);
		temp3 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(b23, 1)), scaleB_v), temp3);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (c01   )), scaleC_v), temp0);
		temp1 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(c01, 1)), scaleC_v), temp1);
		temp2 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (c23   )), scaleC_v), temp2);
		temp3 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(c23, 1)), scaleC_v), temp3);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (d01   )), scaleD_v), temp0);
		temp1 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(d01, 1)), scaleD_v), temp1);
		temp2 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (d23   )), scaleD_v), temp2);
		temp3 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(d23, 1)), scaleD_v), temp3);

		_mm256_store_ps((float*)(dst + 0), temp0);
		_mm256_store_ps((float*)(dst + 1), temp1);
		_mm256_store_ps((float*)(dst + 2), temp2);
		_mm256_store_ps((float*)(dst + 3), temp3);

		srcA += 2;
		srcB += 2;
		srcC += 2;
		srcD += 2;
		dst += 4;
	}

	for (const int block2 = count - 2; i <= block2; i += 2) // unpack 2 at a time
	{
		__m256 temp0;
		__m256 temp1;

		const __m256i a01 = _mm256_load_si256(srcA + 0);
		const __m256i b01 = _mm256_load_si256(srcB + 0);
		const __m256i c01 = _mm256_load_si256(srcC + 0);
		const __m256i d01 = _mm256_load_si256(srcD + 0);

		temp0 = _mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (a01   )), scaleA_v);
		temp1 = _mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(a01, 1)), scaleA_v);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (b01   )), scaleB_v), temp0);
		temp1 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(b01, 1)), scaleB_v), temp1);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (c01   )), scaleC_v), temp0);
		temp1 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(c01, 1)), scaleC_v), temp1);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_castsi256_si128  (d01   )), scaleD_v), temp0);
		temp1 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(_mm256_extractf128_si256(d01, 1)), scaleD_v), temp1);

		_mm256_store_ps((float*)(dst + 0), temp0);
		_mm256_store_ps((float*)(dst + 1), temp1);

		srcA += 1;
		srcB += 1;
		srcC += 1;
		srcD += 1;
		dst += 2;
	}

	for (; i < count; i++) // unpack 1 at a time
	{
		__m256 temp0;

		const __m128i a0 = _mm_load_si128((const __m128i*)srcA);
		const __m128i b0 = _mm_load_si128((const __m128i*)srcB);
		const __m128i c0 = _mm_load_si128((const __m128i*)srcC);
		const __m128i d0 = _mm_load_si128((const __m128i*)srcD);

		temp0 = _mm256_mul_ps(_mm256_cvtph_ps(a0), scaleA_v);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(b0), scaleB_v), temp0);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(c0), scaleC_v), temp0);

		temp0 = _mm256_add_ps(_mm256_mul_ps(_mm256_cvtph_ps(d0), scaleD_v), temp0);

		_mm256_store_ps((float*)dst, temp0);

		srcA = (const __m256i*)(((const __m128i*)srcA) + 1);
		srcB = (const __m256i*)(((const __m128i*)srcB) + 1);
		srcC = (const __m256i*)(((const __m128i*)srcC) + 1);
		srcD = (const __m256i*)(((const __m128i*)srcD) + 1);
		dst += 1;
	}
}
#endif // RSG_SSE_VERSION >= 50

// [SSE TODO] -- consider using _mm_stream_load_si128 or _mm256_stream_load_si256?
void V4Float16StreamUnpack(Vector_4V* RESTRICT dstV, const Vector_4V* RESTRICT srcV, int count)
{
	Assertf(count >= 0 && (count&7) == 0, "count must be non-negative multiple of 8");

#if 0 && RSG_XENON
	XMConvertHalfToFloatStream(dstV, srcV, count); // this might be slightly faster?
#elif RSG_SSE_VERSION >= 50
	if (((uptr)dstV & 0x1f) == 0) // check that we're aligned to 32-bytes, otherwise _mm256_store_ps will fail
	{
		count /= 8;
		int i = 0;

		for (const int block4 = count - 4; i <= block4; i += 4) // unpack 4 at a time
		{
			const __m128i v0 = _mm_load_si128((const __m128i*)(srcV + 0));
			const __m128i v1 = _mm_load_si128((const __m128i*)(srcV + 1));
			const __m128i v2 = _mm_load_si128((const __m128i*)(srcV + 2));
			const __m128i v3 = _mm_load_si128((const __m128i*)(srcV + 3));
			const __m256 ab0 = _mm256_cvtph_ps(v0);
			const __m256 ab1 = _mm256_cvtph_ps(v1);
			const __m256 ab2 = _mm256_cvtph_ps(v2);
			const __m256 ab3 = _mm256_cvtph_ps(v3);
			_mm256_store_ps((float*)(dstV + 0*2), ab0);
			_mm256_store_ps((float*)(dstV + 1*2), ab1);
			_mm256_store_ps((float*)(dstV + 2*2), ab2);
			_mm256_store_ps((float*)(dstV + 3*2), ab3);

			srcV += 4;
			dstV += 4*2;
		}

		for (const int block2 = count - 2; i <= block2; i += 2) // unpack 2 at a time
		{
			const __m128i v0 = _mm_load_si128((const __m128i*)(srcV + 0));
			const __m128i v1 = _mm_load_si128((const __m128i*)(srcV + 1));
			const __m256 ab0 = _mm256_cvtph_ps(v0);
			const __m256 ab1 = _mm256_cvtph_ps(v1);
			_mm256_store_ps((float*)(dstV + 0*2), ab0);
			_mm256_store_ps((float*)(dstV + 1*2), ab1);

			srcV += 2;
			dstV += 2*2;
		}

		for (; i < count; i++) // unpack 1 at a time
		{
			const __m128i v0 = _mm_load_si128((const __m128i*)(srcV + 0));
			const __m256 ab0 = _mm256_cvtph_ps(v0);
			_mm256_store_ps((float*)(dstV + 0*2), ab0);

			srcV += 1;
			dstV += 1*2;
		}
	}
	else
#endif // RSG_SSE_VERSION >= 50
	{
		count /= 8;
		int i = 0;

		for (const int block4 = count - 4; i <= block4; i += 4) // unpack 4 at a time
		{
			Vector_4V a0, b0;
			Vector_4V a1, b1;
			Vector_4V a2, b2;
			Vector_4V a3, b3;

			V4Float16Vec8Unpack(a0, b0, ADDR_INCR(srcV,0));
			V4Float16Vec8Unpack(a1, b1, ADDR_INCR(srcV,1));
			V4Float16Vec8Unpack(a2, b2, ADDR_INCR(srcV,2));
			V4Float16Vec8Unpack(a3, b3, ADDR_INCR(srcV,3));

			ADDR_SKIP(srcV,4);

			ADDR_INCR(dstV,0*2+0) = a0; ADDR_INCR(dstV,0*2+1) = b0;
			ADDR_INCR(dstV,1*2+0) = a1; ADDR_INCR(dstV,1*2+1) = b1;
			ADDR_INCR(dstV,2*2+0) = a2; ADDR_INCR(dstV,2*2+1) = b2;
			ADDR_INCR(dstV,3*2+0) = a3; ADDR_INCR(dstV,3*2+1) = b3;

			ADDR_SKIP(dstV,4*2+0);
		}

		for (const int block2 = count - 2; i <= block2; i += 2) // unpack 2 at a time
		{
			Vector_4V a0, b0;
			Vector_4V a1, b1;

			V4Float16Vec8Unpack(a0, b0, ADDR_INCR(srcV,0));
			V4Float16Vec8Unpack(a1, b1, ADDR_INCR(srcV,1));

			ADDR_SKIP(srcV,2);

			ADDR_INCR(dstV,0*2+0) = a0; ADDR_INCR(dstV,0*2+1) = b0;
			ADDR_INCR(dstV,1*2+0) = a1; ADDR_INCR(dstV,1*2+1) = b1;

			ADDR_SKIP(dstV,2*2+0);
		}

		for (; i < count; i++) // unpack 1 at a time
		{
			Vector_4V a0, b0;

			V4Float16Vec8Unpack(a0, b0, ADDR_INCR(srcV,0));

			ADDR_SKIP(srcV,1);

			ADDR_INCR(dstV,0*2+0) = a0; ADDR_INCR(dstV,0*2+1) = b0;

			ADDR_SKIP(dstV,1*2+0);
		}
	}
}

// [SSE TODO] -- consider using _mm256_stream_load_si256?
void V4Float16StreamPack(Vector_4V* RESTRICT dstV, const Vector_4V* RESTRICT srcV, int count)
{
	Assertf(count >= 0 && (count&7) == 0, "count must be non-negative multiple of 8");

#if RSG_SSE_VERSION >= 50
	if (((uptr)srcV & 0x1f) == 0) // check that we're aligned to 32-bytes, otherwise _mm256_load_ps will fail
	{
		count /= 8;
		int i = 0;

		for (const int block4 = count - 4; i <= block4; i += 4) // pack 4 at a time
		{
			const __m256 ab0 = _mm256_load_ps((const float*)(srcV + 0*2));
			const __m256 ab1 = _mm256_load_ps((const float*)(srcV + 1*2));
			const __m256 ab2 = _mm256_load_ps((const float*)(srcV + 2*2));
			const __m256 ab3 = _mm256_load_ps((const float*)(srcV + 3*2));
			const __m128i v0 = _mm256_cvtps_ph(ab0, _MM_FROUND_TO_ZERO);
			const __m128i v1 = _mm256_cvtps_ph(ab1, _MM_FROUND_TO_ZERO);
			const __m128i v2 = _mm256_cvtps_ph(ab2, _MM_FROUND_TO_ZERO);
			const __m128i v3 = _mm256_cvtps_ph(ab3, _MM_FROUND_TO_ZERO);
			_mm_store_si128((__m128i*)(dstV + 0), v0);
			_mm_store_si128((__m128i*)(dstV + 1), v1);
			_mm_store_si128((__m128i*)(dstV + 2), v2);
			_mm_store_si128((__m128i*)(dstV + 3), v3);

			srcV += 4*2;
			dstV += 4;
		}

		for (const int block2 = count - 2; i <= block2; i += 2) // pack 2 at a time
		{
			const __m256 ab0 = _mm256_load_ps((const float*)(srcV + 0*2));
			const __m256 ab1 = _mm256_load_ps((const float*)(srcV + 1*2));
			const __m128i v0 = _mm256_cvtps_ph(ab0, _MM_FROUND_TO_ZERO);
			const __m128i v1 = _mm256_cvtps_ph(ab1, _MM_FROUND_TO_ZERO);
			_mm_store_si128((__m128i*)(dstV + 0), v0);
			_mm_store_si128((__m128i*)(dstV + 1), v1);

			srcV += 2*2;
			dstV += 2;
		}

		for (; i < count; i++) // unpack 1 at a time
		{
			const __m256 ab0 = _mm256_load_ps((const float*)(srcV + 0*2));
			const __m128i v0 = _mm256_cvtps_ph(ab0, _MM_FROUND_TO_ZERO);
			_mm_store_si128((__m128i*)(dstV + 0), v0);

			srcV += 1*2;
			dstV += 1;
		}
	}
	else
#endif // RSG_SSE_VERSION >= 50
	{
		for (int i = 0; i < count; i += 8)
		{
			const Vector_4V a = *(srcV++);
			const Vector_4V b = *(srcV++);

			*(dstV++) = V4Float16Vec8Pack(a, b);
		}
	}
}

} // namespace Vec

#undef ADDR_INCR
#undef ADDR_SKIP

// ================================================================================================

#if RSG_DEV // validation tests

#if 0 && RSG_SSE_VERSION >= 41

namespace Vec {

__forceinline Vector_4V_Out V4Float16Vec8Pack_opt(Vector_4V_In a, Vector_4V_In b)
{
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
}

__forceinline void V4Float16Vec8Unpack_opt(Vector_4V_InOut a, Vector_4V_InOut b, Vector_4V_In v)
{
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
}

} // namespace Vec

__forceinline Vec4V_Out Float16Vec8Pack_opt(Vec4V_In src0, Vec4V_In src1) { return Vec4V(Vec::V4Float16Vec8Pack_opt(src0.GetIntrin128(), src1.GetIntrin128())); }
__forceinline void Float16Vec8Unpack_opt(Vec4V_InOut dst0, Vec4V_InOut dst1, Vec4V_In src) { Vec::V4Float16Vec8Unpack_opt(dst0.GetIntrin128Ref(), dst1.GetIntrin128Ref(), src.GetIntrin128()); }

#else

#define Float16Vec8Pack_opt Float16Vec8Pack
#define Float16Vec8Unpack_opt Float16Vec8Unpack

#endif

float TestFloat16Pack(int n)
{
	float maxErrRel = 0.0f;
	int maxPackedErr = 0;

	for (int denom = -n; denom <= n; denom++)
	{
		if (denom == 0)
			continue;

		for (int numer = -n; numer <= n; numer++)
		{
			const float f[] =
			{
				Clamp<float>((float)numer/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
			};

			Vec4V a(f[0], f[1], f[2], f[3]);
			Vec4V b(f[4], f[5], f[6], f[7]);
			Vec4V c = Float16Vec8Pack_opt(a, b);

			for (int i = 0; i < 8; i++)
			{
				const float f2 = ((const Float16*)&c)[i].GetFloat32_FromFloat16_NoIntrinsics();
				const float err = f[i] - f2;
				const float errRel = Abs<float>(err/(f[i] ? f[i] : 1.0f));

				if (maxErrRel < errRel)
				{
					Displayf("TestFloat16Pack: err for %f -> %f is %f (abs), %f (rel)", f[i], f2, err, errRel);
					maxErrRel = errRel;
				}

				const int packed1 = (int)((const u16*)&c)[i];
				const int packed2 = (int)Float16(f[i]).GetBinaryData();
				const int packedErr = Abs<int>(packed1 - packed2);

				if (maxPackedErr < packedErr)
				{
					Displayf("TestFloat16Pack: %f packed to 0x%04x (%f) using vector code, 0x%04x (%f) using reference code",
						f[i],
						packed1,
						Float16((u16)packed1).GetFloat32_FromFloat16_NoIntrinsics(),
						packed2,
						Float16((u16)packed2).GetFloat32_FromFloat16_NoIntrinsics()
					);

					maxPackedErr = packedErr;
				}
			}
		}
	}

	return maxErrRel;
}

float TestFloat16Unpack(int n)
{
	float maxErrRel = 0.0f;
	float maxUnpackedErr = 0.0f;

	for (int denom = -n; denom <= n; denom++)
	{
		if (denom == 0)
			continue;

		for (int numer = -n; numer <= n; numer++)
		{
			const float f[] =
			{
				Clamp<float>((float)numer/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
				Clamp<float>((float)(numer ^ (rand()&0x0000ffff))/(float)denom, -65504.0f, 65504.0f),
			};

			Vec4V c;

			for (int i = 0; i < 8; i++)
			{
				((Float16*)&c)[i].SetFloat16_FromFloat32_NoIntrinsics(f[i]);
			}

			Vec4V a, b;
			Float16Vec8Unpack_opt(a, b, c);

			for (int i = 0; i < 8; i++)
			{
				const float f2 = (i < 4) ? ((const float*)&a)[i] : ((const float*)&b)[i - 4];
				const float err = f[i] - f2;
				const float errRel = Abs<float>(err/(f[i] ? f[i] : 1.0f));

				if (maxErrRel < errRel)
				{
					Displayf("TestFloat16Unpack: err for %f -> %f is %f (abs), %f (rel)", f[i], f2, err, errRel);
					maxErrRel = errRel;
				}

				const float unpacked1 = f2;
				const float unpacked2 = Float16(((const u16*)&c)[i]).GetFloat32_FromFloat16_NoIntrinsics();
				const float unpackedErr = Abs<float>(unpacked1 - unpacked2);

				if (maxUnpackedErr < unpackedErr)
				{
					Displayf("TestFloat16Unpack: 0x%04x unpacked to %f using vector code, %f using reference code",
						((const u16*)&c)[i],
						unpacked1,
						unpacked2
					);

					maxUnpackedErr = unpackedErr;
				}
			}
		}
	}

	return maxErrRel;
}

float TestFloat16StreamPack(int n)
{
	float maxErrRel = 0.0f;

	n = (n + 1)&~1; // round up to multiple of 2 (2 vectors = 8 floats)

	Vec::Vector_4V* src = rage_aligned_new(32) Vec::Vector_4V[n];
	Vec::Vector_4V* dst = rage_aligned_new(32) Vec::Vector_4V[n/2];

	for (float scale = 1.0f/16.0f; scale <= 16.0f; scale *= 2.0f)
	{
		for (int i = 0; i < n*4; i++) // fill with random data in the range [-scale..scale]
		{
			float f = scale*(-1.0f + 2.0f*(float)(rand()&0x0000ffff)/65535.0f);
			if (Abs<float>(f)*16384.0f <= 1.0f) { f = 0.0f; }
			((float*)src)[i] = f;
		}

		Vec::V4Float16StreamPack(dst, src, n*4);

		for (int i = 0; i < n*4; i++)
		{
			const float f = ((float*)src)[i];
			const float f2 = ((const Float16*)dst)[i].GetFloat32_FromFloat16_NoIntrinsics();

			const float err = f - f2;
			const float errRel = Abs<float>(err/(f ? f : 1.0f));

			if (maxErrRel < errRel)
			{
				Displayf("TestFloat16StreamPack: err for %f -> %f is %f (abs), %f (rel)", f, f2, err, errRel);
				maxErrRel = errRel;
			}
		}
	}

	delete[] src;
	delete[] dst;

	return maxErrRel;
}

float TestFloat16StreamUnpack(int n)
{
	float maxErrRel = 0.0f;

	n = (n + 1)&~1; // round up to multiple of 2 (2 vectors = 8 floats)

	float* srcFloats = rage_new float[n*4];
	Vec::Vector_4V* src = rage_aligned_new(32) Vec::Vector_4V[n/2];
	Vec::Vector_4V* dst = rage_aligned_new(32) Vec::Vector_4V[n];

	for (float scale = 1.0f/16.0f; scale <= 16.0f; scale *= 2.0f)
	{
		for (int i = 0; i < n*4; i++) // fill with random data in the range [-scale..scale]
		{
			float f = scale*(-1.0f + 2.0f*(float)(rand()&0x0000ffff)/65535.0f);
			if (Abs<float>(f)*16384.0f <= 1.0f) { f = 0.0f; }
			srcFloats[i] = f;
			((Float16*)src)[i].SetFloat16_FromFloat32_NoIntrinsics(f);
		}
		
		Vec::V4Float16StreamUnpack(dst, src, n*4);

		for (int i = 0; i < n*4; i++)
		{
			const float f = srcFloats[i];
			const float f2 = ((const float*)dst)[i];

			const float err = f - f2;
			const float errRel = Abs<float>(err/(f ? f : 1.0f));

			if (maxErrRel < errRel)
			{
				Displayf("TestFloat16StreamUnpack: err for %f -> %f is %f (abs), %f (rel)", f, f2, err, errRel);
				maxErrRel = errRel;
			}
		}
	}

	delete[] srcFloats;
	delete[] src;
	delete[] dst;

	return maxErrRel;
}

// TODO -- i've tested these on PPU and WIN32PC, still need to test on XENON and SPU (what about PSP2?)

void TestPackColor32(Vec4V_In v0)
{
	const Color32 c0(
		(int)(Clamp<float>(v0.GetXf(), 0.0f, 1.0f)*255.0f + 0.5f),
		(int)(Clamp<float>(v0.GetYf(), 0.0f, 1.0f)*255.0f + 0.5f),
		(int)(Clamp<float>(v0.GetZf(), 0.0f, 1.0f)*255.0f + 0.5f),
		(int)(Clamp<float>(v0.GetWf(), 0.0f, 1.0f)*255.0f + 0.5f)
	);
	Color32 c1;
	Color32 c2;
	Color32 c3;
	Color32 c4;
	c1.SetFromRGBA(Clamp(v0, Vec4V(V_ZERO), Vec4V(V_ONE)));
	PackARGBColor32(&c2, v0); // should clamp automatically
	c3.SetFromARGB(Clamp(v0, Vec4V(V_ZERO), Vec4V(V_ONE)));
	PackRGBAColor32((u32*)&c4, v0); // should clamp automatically

	// c0,c1,c2 should match, and c3,c4 should match
	Displayf("TestPackColor32 (%f,%f,%f,%f)", VEC4V_ARGS(v0));
	Displayf("  c0 = %d,%d,%d,%d", c0.GetRed(), c0.GetGreen(), c0.GetBlue(), c0.GetAlpha());
	Displayf("  c1 = %d,%d,%d,%d", c1.GetRed(), c1.GetGreen(), c1.GetBlue(), c1.GetAlpha());
	Displayf("  c2 = %d,%d,%d,%d", c2.GetRed(), c2.GetGreen(), c2.GetBlue(), c2.GetAlpha());
	Displayf("  c3 = %d,%d,%d,%d", c3.GetRed(), c3.GetGreen(), c3.GetBlue(), c3.GetAlpha());
	Displayf("  c4 = %d,%d,%d,%d", c4.GetRed(), c4.GetGreen(), c4.GetBlue(), c4.GetAlpha());
}

void TestUnpackColor32(Color32 c0)
{
	const Vec4V v0(
		(float)c0.GetRed  ()/255.0f,
		(float)c0.GetGreen()/255.0f,
		(float)c0.GetBlue ()/255.0f,
		(float)c0.GetAlpha()/255.0f
	);
	const Vec4V v1 = c0.GetRGBA();
	const Vec4V v2 = UnpackARGBColor32(&c0);
	const Vec4V v3 = c0.GetARGB();
	const Vec4V v4 = UnpackRGBAColor32((const u32*)&c0);

	// v0,v1,v2 should match, and v3,v4 should match
	Displayf("TestUnpackColor32(%d,%d,%d,%d)", c0.GetRed(), c0.GetGreen(), c0.GetBlue(), c0.GetAlpha());
	Displayf("  v0 = %f,%f,%f,%f", VEC4V_ARGS(v0));
	Displayf("  v1 = %f,%f,%f,%f", VEC4V_ARGS(v1));
	Displayf("  v2 = %f,%f,%f,%f", VEC4V_ARGS(v2));
	Displayf("  v3 = %f,%f,%f,%f", VEC4V_ARGS(v3));
	Displayf("  v4 = %f,%f,%f,%f", VEC4V_ARGS(v4));
}

void TestPackUnpackColor32()
{
	TestPackColor32(Vec4V(1.0f,0.5f,0.25f,0.0f));
	TestPackColor32(Vec4V(-0.1f,1.5f,-1.5f,2.0f)); // should clamp to {0,1,0,1}
	TestUnpackColor32(Color32(255,128,64,0));

	const Color32 tests[] =
	{
		Color32(33,55,77,99),
		Color32(87,65,43,21),
	};

	float dv1_max = 0.0f;
	float dv2_max = 0.0f;
	float dv3_max = 0.0f;
	int   dc1_max = 0;
	int   dc2_max = 0;
	int   dc3_max = 0;

	for (int i = 0; i < NELEM(tests) + 256*4; i++)
	{
		Color32 c0(0);

		if (i < NELEM(tests))
		{
			c0 = tests[i];
		}
		else if ((i - NELEM(tests) - 256*0) < 256) { const int x = i - NELEM(tests) - 256*0; c0 = Color32(x, 255, 64, 0); }
		else if ((i - NELEM(tests) - 256*1) < 256) { const int x = i - NELEM(tests) - 256*1; c0 = Color32(0, x, 255, 64); }
		else if ((i - NELEM(tests) - 256*2) < 256) { const int x = i - NELEM(tests) - 256*2; c0 = Color32(64, 0, x, 255); }
		else if ((i - NELEM(tests) - 256*3) < 256) { const int x = i - NELEM(tests) - 256*3; c0 = Color32(255, 64, 0, x); }

		const Vec4V v0(
			(float)c0.GetRed  ()/255.0f,
			(float)c0.GetGreen()/255.0f,
			(float)c0.GetBlue ()/255.0f,
			(float)c0.GetAlpha()/255.0f
		);
		const Vec4V v1 = c0.GetRGBA();           // should be the same as v0
		const Vec4V v2 = UnpackARGBColor32(&c0); // should be the same as v0
		const Vec4V v3 = c0.GetARGB();           // should be the same as v3,v4
		const Vec4V v4 = UnpackRGBAColor32((const u32*)&c0); // should be the same as v3,v4
		const float dv1 = MaxElement(Abs(v1 - v0)).Getf();
		const float dv2 = MaxElement(Abs(v2 - v0)).Getf();
		const float dv3 = MaxElement(Abs(v3 - v4)).Getf();

		dv1_max = Max<float>(dv1, dv1_max);
		dv2_max = Max<float>(dv2, dv2_max);
		dv3_max = Max<float>(dv3, dv3_max);

		Color32 c1;
		Color32 c2;
		Color32 c3;
		Color32 c4;
		c1.SetFromRGBA(v0);       // should be the same as c0
		PackARGBColor32(&c2, v0); // should be the same as c0
		c3.SetFromARGB(v0);       // should be the same as c3,c4
		PackRGBAColor32((u32*)&c4, v0); // should be the same as c3,c4

		const int dc1 = Max<int>(
			Max<int>((int)c1.GetRed () - (int)c0.GetRed (), (int)c1.GetGreen() - (int)c0.GetGreen()),
			Max<int>((int)c1.GetBlue() - (int)c0.GetBlue(), (int)c1.GetAlpha() - (int)c0.GetAlpha())
		);
		const int dc2 = Max<int>(
			Max<int>((int)c2.GetRed () - (int)c0.GetRed (), (int)c2.GetGreen() - (int)c0.GetGreen()),
			Max<int>((int)c2.GetBlue() - (int)c0.GetBlue(), (int)c2.GetAlpha() - (int)c0.GetAlpha())
		);
		const int dc3 = Max<int>(
			Max<int>((int)c3.GetRed () - (int)c4.GetRed (), (int)c3.GetGreen() - (int)c4.GetGreen()),
			Max<int>((int)c3.GetBlue() - (int)c4.GetBlue(), (int)c3.GetAlpha() - (int)c4.GetAlpha())
		);
		dc1_max = Max<int>(dc1, dc1_max);
		dc2_max = Max<int>(dc2, dc2_max);
		dc3_max = Max<int>(dc3, dc3_max);
	}

	Displayf("TestPackUnpackColor32");
	Displayf("  dv1_max = %f", dv1_max);
	Displayf("  dv2_max = %f", dv2_max);
	Displayf("  dv3_max = %f", dv3_max);
	Displayf("  dc1_max = %d", dc1_max);
	Displayf("  dc2_max = %d", dc2_max);
	Displayf("  dc3_max = %d", dc3_max);
}

#endif // RSG_DEV

} // namespace rage
