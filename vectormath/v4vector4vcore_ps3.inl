#include "system/floattoint.h"

namespace rage
{
namespace Vec
{



	__forceinline Vector_4V_Out V4IsNotNanV( Vector_4V_In inVector )
	{
#if !__SPU
		// This comparison will fail for NaN components only.
		Vector_4V retVal = V4IsEqualV( inVector, inVector );

		// If INF, all bets are off, since we don't yet have an FPIsNotNan().
		// But we can at least assert for the case when it *should be* finite (avoid false positives).
		ASSERT_ONLY( if( FPIsFinite( Vec::GetX(inVector) ) && ( (u32)Vec::GetXi(retVal) != 0xFFFFFFFF ) ) mthErrorf("Vectorized V4IsNotNanV() X result != scalar FPIsFinite() results!" ); );
		ASSERT_ONLY( if( FPIsFinite( Vec::GetY(inVector) ) && ( (u32)Vec::GetYi(retVal) != 0xFFFFFFFF ) ) mthErrorf("Vectorized V4IsNotNanV() Y result != scalar FPIsFinite() results!" ); );
		ASSERT_ONLY( if( FPIsFinite( Vec::GetZ(inVector) ) && ( (u32)Vec::GetZi(retVal) != 0xFFFFFFFF ) ) mthErrorf("Vectorized V4IsNotNanV() Z result != scalar FPIsFinite() results!" ); );
		ASSERT_ONLY( if( FPIsFinite( Vec::GetW(inVector) ) && ( (u32)Vec::GetWi(retVal) != 0xFFFFFFFF ) ) mthErrorf("Vectorized V4IsNotNanV() W result != scalar FPIsFinite() results!" ); );

#elif __SPU
		// SPUs don't have NaNs, but we want to compare check if inVector would be a NaN on the PPU.
		qword firstNanShl1 = si_iohl(si_ilhu(0xff00),2);
		qword inVectorShl1 = si_a((qword)inVector,(qword)inVector);
		Vector_4V retVal = (Vector_4V)si_clgt(firstNanShl1,inVectorShl1);
#endif

		return retVal;
	}

	__forceinline unsigned int V4IsNotNanAll(Vector_4V_In inVector)
	{
#if !__SPU
		return V4IsEqualAll(inVector, inVector);
#else
		qword firstNanShl1 = si_iohl(si_ilhu(0xff00),2);
		qword inVectorShl1 = si_a((qword)inVector,(qword)inVector);
		qword mask = si_clgt(firstNanShl1,inVectorShl1);
		return si_to_uint(si_ceqi(si_gb(mask),15));
#endif
	}

	__forceinline Vector_4V_Out V4IsFiniteV( Vector_4V_In inVector )
	{
		// See if the exponent is 255 (7F8)
		Vector_4V inf = V4VConstant(V_INF);
		Vector_4V retVal = V4InvertBits( V4IsEqualIntV( V4And(inVector, inf), inf ) );
#if !__OPTIMIZED
		mthAssertf( FPIsFinite( Vec::GetX(inVector) ) == ( (u32)Vec::GetXi(retVal) == 0xFFFFFFFF ) , "Vectorized V4IsFiniteV() X result != scalar FPIsFinite() result!" );
		mthAssertf( FPIsFinite( Vec::GetY(inVector) ) == ( (u32)Vec::GetYi(retVal) == 0xFFFFFFFF ) , "Vectorized V4IsFiniteV() Y result != scalar FPIsFinite() result!" );
		mthAssertf( FPIsFinite( Vec::GetZ(inVector) ) == ( (u32)Vec::GetZi(retVal) == 0xFFFFFFFF ) , "Vectorized V4IsFiniteV() Z result != scalar FPIsFinite() result!" );
		mthAssertf( FPIsFinite( Vec::GetW(inVector) ) == ( (u32)Vec::GetWi(retVal) == 0xFFFFFFFF ) , "Vectorized V4IsFiniteV() W result != scalar FPIsFinite() result!" );
#endif // !__OPTIMIZED
		return retVal;
	}

	__forceinline unsigned int V4IsFiniteAll( Vector_4V_In inVector )
	{
#if !__SPU
		// The AltiVec PIM specifies that vec_all_lt(vec_float4 a, vec_float4 b)
		// maps to vcmpgtfp. x,b,a but in the coredump for B*850904, SNC has
		// used vcmpgefp. x,a,b and negated the result.  This looks like a
		// compiler bug, since it changes the behaviour for NaNs, which we were
		// relying on here with the code
		//      return V4IsLessThanAll(V4Abs(inVector),V4VConstant(V_INF));
		// To work around this, use unsigned integer comparison.  The following
		// code while a little harder to read, is better anyways, since the VMX
		// constants are easier to generate.
		vec_uint4 t0 = (vec_uint4)vec_vspltisw(-1);
		vec_uint4 t1 = (vec_uint4)vec_vspltisw(-8);         // -8 ≡ 24 mod 32
		vec_uint4 t2 = (vec_uint4)vec_vspltisw(1);
		vec_uint4 t3 = vec_vslw(t0, t1);                    // 0xff000000
		vec_uint4 in = vec_vslw((vec_uint4)inVector, t2);
		return vec_all_gt(t3, in);
#else
		qword infShl1 = si_ilhu(0xff00);
		qword inVectorShl1 = si_a((qword)inVector,(qword)inVector);
		qword mask = si_clgt(infShl1,inVectorShl1);
		return si_to_uint(si_ceqi(si_gb(mask),15));
#endif
	}

	template<unsigned FROM> __forceinline void V4Store8(void *dst, Vector_4V_In src)
	{
#		if __PPU
			vec_stvebx(vec_vspltb((vec_char16)src, FROM), 0, (s8*)dst);
#		else
			qword q = si_lqd(si_from_ptr(dst), 0);
			qword shuf = si_cbd(si_from_ptr(dst), 0);
			if (FROM != 3)
			{
				// One implementation option is to minimize the latency assuming
				// that src is on the critical path.  This approach modifies
				// shuf so that still just a single si_shufb is required.
				// Instead, though, we'll go for the code size optimization of
				// just shifting src left.
// 				qword t = si_clgtbi(shuf, 15);
// 				t = si_andc(si_ilh(((FROM<<8)|FROM)-0x0303), t);
// 				shuf = si_a(shuf, t);
				src = (Vector_4V)si_rotqbyi((qword)src, FROM-3);
			}
			q = si_shufb((qword)src, q, shuf);
			si_stqd(q, si_from_ptr(dst), 0);
#		endif
	}

	template<unsigned FROM> __forceinline void V4Store16(void *dst, Vector_4V_In src)
	{
		FastAssert(((uptr)dst&1)==0);
#		if __PPU
			vec_stvehx(vec_vsplth((vec_short8)src, FROM), 0, (s16*)dst);
#		else
			qword q = si_lqd(si_from_ptr(dst), 0);
			qword shuf = si_chd(si_from_ptr(dst), 0);
			if (FROM != 1)
			{
				src = (Vector_4V)si_rotqbyi((qword)src, FROM*2-2);
			}
			q = si_shufb((qword)src, q, shuf);
			si_stqd(q, si_from_ptr(dst), 0);
#		endif
	}

	template<unsigned FROM> __forceinline void V4Store32(void *dst, Vector_4V_In src)
	{
		FastAssert(((uptr)dst&3)==0);
#		if __PPU
			vec_stvewx(vec_vspltw((vec_int4)src, FROM), 0, (s32*)dst);
#		else
			qword q = si_lqd(si_from_ptr(dst), 0);
			qword shuf = si_cwd(si_from_ptr(dst), 0);
			if (FROM != 0)
			{
				src = (Vector_4V)si_shlqbyi((qword)src, FROM*4);
			}
			q = si_shufb((qword)src, q, shuf);
			si_stqd(q, si_from_ptr(dst), 0);
#		endif
	}

	template<unsigned FROM> __forceinline void V4Store64(void *dst, Vector_4V_In src)
	{
		FastAssert(((uptr)dst&7)==0);
#		if __PPU
			vec_stvewx(vec_vspltw((vec_int4)src, FROM*2+0), 0, (s32*)dst);
			vec_stvewx(vec_vspltw((vec_int4)src, FROM*2+1), 4, (s32*)dst);
#		else
			qword q = si_lqd(si_from_ptr(dst), 0);
			qword shuf = si_cdd(si_from_ptr(dst), 0);
			if (FROM != 0)
			{
				src = (Vector_4V)si_shlqbyi((qword)src, FROM*8);
			}
			q = si_shufb((qword)src, q, shuf);
			si_stqd(q, si_from_ptr(dst), 0);
#		endif
	}

	__forceinline float GetX( Vector_4V_ConstRef inVector )
	{
		// ONE
		return (reinterpret_cast<const float*>(&inVector))[0];

		// TWO	(like si_to_float() in si2vmx_gcc.h)'
		//		(also like vec_extract())
//		union {
//			Vector_4V_In v;
//			float f[4];
//		} u;
//		u.v = inVector;
//		return u.f[0];

		// THREE (VMX only)
		// (RAGE uses this)
//		float f;
//		vec_ste(inVector, 0, &f);
//		return f;

		// FOUR (SPU only. Implemented same as THREE... THREE is better ('0' is a literal). Good for a GetElement() though!).
		// (RAGE uses this)
//		return spu_extract(inVector, 0);

		// FIVE
		// What we really want is vec_extract(). But where is it? (newest version of tools! It is an extension, for Cell's Altivec only.)
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
#if __PPU
		return vec_splat(inVector, 0);
#else // __SPU
		// Shuffling beats the vmx2spu mapping, at least in the case when inside a loop
		// where the control vector generation can be taken outside of the loop.
		Vector_4V_uchar _xxxxControl = (Vector_4V_uchar)spu_splats((int)0x00010203);
		return spu_shuffle( inVector, inVector, _xxxxControl );
#endif
	}

	__forceinline Vector_4V_Out V4SplatY( Vector_4V_In inVector )
	{
#if __PPU
		return vec_splat(inVector, 1);
#else // __SPU
		// Shuffling beats the vmx2spu mapping, at least in the case when inside a loop
		// where the control vector generation can be taken outside of the loop.
		Vector_4V_uchar _yyyyControl = (Vector_4V_uchar)spu_splats((int)0x04050607);
		return spu_shuffle( inVector, inVector, _yyyyControl );
#endif
	}

	__forceinline Vector_4V_Out V4SplatZ( Vector_4V_In inVector )
	{
#if __PPU
		return vec_splat(inVector, 2);
#else // __SPU
		// Shuffling beats the vmx2spu mapping, at least in the case when inside a loop
		// where the control vector generation can be taken outside of the loop.
		Vector_4V_uchar _zzzzControl = (Vector_4V_uchar)spu_splats((int)0x08090a0b);
		return spu_shuffle( inVector, inVector, _zzzzControl );
#endif
	}

	__forceinline Vector_4V_Out V4SplatW( Vector_4V_In inVector )
	{
#if __PPU
		return vec_splat(inVector, 3);
#else // __SPU
		// Shuffling beats the vmx2spu mapping, at least in the case when inside a loop
		// where the control vector generation can be taken outside of the loop.
		Vector_4V_uchar _wwwwControl = (Vector_4V_uchar)spu_splats((int)0x0c0d0e0f);
		return spu_shuffle( inVector, inVector, _wwwwControl );
#endif
	}

	__forceinline void V4Set( Vector_4V_InOut inoutVector, const float& x0, const float& y0, const float& z0, const float& w0 )
	{
		inoutVector = VECTOR4V_LITERAL(x0, y0, z0, w0);

		// (The "else" part of the code below is causing "uninitialized variable" warnings.)

		//if (__builtin_constant_p(x0) & __builtin_constant_p(y0) & __builtin_constant_p(z0) & __builtin_constant_p(w0))
		//{
		//	inoutVector = VECTOR4V_LITERAL(x0, y0, z0, w0);
		//}
		//else
		//{
		//	f32 *pVect = (f32*)&inoutVector;
		//	pVect[0] = x0;
		//	pVect[1] = y0;
		//	pVect[2] = z0;
		//	pVect[3] = w0;
		//}
	}

	__forceinline Vector_4V_Out V4Add( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return vec_add( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Subtract( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return vec_sub( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4AddInt( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return (Vector_4V)vec_add((Vector_4V_int)inVector1, (Vector_4V_int)inVector2);
	}

	__forceinline Vector_4V_Out V4SubtractInt( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return (Vector_4V)vec_sub((Vector_4V_int)inVector1, (Vector_4V_int)inVector2);
	}

	__forceinline Vector_4V_Out V4AddScaled( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In inVector3 )
	{
		return vec_madd( inVector2, inVector3, inVector1 );
	}

	__forceinline Vector_4V_Out V4SubtractScaled( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In inVector3 )
	{
		return vec_nmsub( inVector2, inVector3, inVector1 );
	}

	__forceinline Vector_4V_Out V4Scale( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
#if __PPU
		//return vec_madd( inVector1, inVector2, (Vector_4V)vec_splat_s32(0) );
		return (inVector1 * inVector2);  // Need this notation so the PS3 PPU compiler will combine V4Scale() and V4Add() into one instruction. [really?]
#else // __SPU
		return spu_mul( inVector1, inVector2 );
#endif
	}

	__forceinline Vector_4V_Out V4InvertBits(Vector_4V_In inVector)
	{
		return vec_nor( inVector, inVector );
	}

	__forceinline Vector_4V_Out V4Negate(Vector_4V_In inVector)
	{
#if !USE_ALTERNATE_NEGATE
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
		// TODO: Faster? (RAGE version)
		Vector_4V prod = V4Scale(a,b);
		return V4Add( V4Add( V4SplatX(prod), V4SplatY(prod) ), V4Add( V4SplatZ(prod), V4SplatW(prod) ) );

		// TODO: Faster? (XMVECTOR version -- 1 instruction less, but more dependencies!)
		//Vector_4V D0;
		//Vector_4V D1;
		//D0 = V4Scale(a, b);
		//D1 = vec_sld(D0, D0, 8);
		//D0 = V4Add(D0, D1);
		//D1 = vec_sld(D0, D0, 4);
		//return V4Add(D0, D1);

		//Vector_4V vecProd = V4Scale(a, b);
		//vecProd = V4AddScaled( vecProd, vec_sld(a, a, 4), vec_sld(b, b, 4) );
		//return V4Add( vec_sld(vecProd, vecProd, 8), vecProd );
	}

	__forceinline Vector_4V_Out V4MagSquaredV( Vector_4V_In v )
	{
		Vector_4V vecProd = V4Scale(v, v);
		Vector_4V tempv = vec_sld(v, v, 4);
		vecProd = V4AddScaled( vecProd, tempv, tempv );
		return V4Add( vec_sld(vecProd, vecProd, 8), vecProd );
	}

	__forceinline Vector_4V_Out V4Expt( Vector_4V_In x )
	{
		return vec_expte( x );
	}

	__forceinline Vector_4V_Out V4Log2( Vector_4V_In x )
	{
		return vec_loge( x );
	}

	//============================================================================
	// Conversion functions

	template <int exponent>
	__forceinline Vector_4V_Out V4FloatToIntRaw(Vector_4V_In inVector)
	{
		return (Vector_4V)vec_cts( inVector, exponent );
	}

	template <int exponent>
	__forceinline Vector_4V_Out V4IntToFloatRaw(Vector_4V_In inVector)
	{
		return vec_ctf( (Vector_4V_int)inVector, exponent );
	}

	__forceinline Vector_4V_Out V4Uint8ToFloat32(Vector_4V_In inVector)
	{
#		if __SPU
			static const qword shuf = {
				0x80,0x80,0x80,0x00, 0x80,0x80,0x80,0x01,
				0x80,0x80,0x80,0x02, 0x80,0x80,0x80,0x03 };
			qword q = si_shufb((qword)inVector, (qword)inVector, shuf);
			return Vector_4V(si_cuflt(q, 8));
#		else
			vec_int4   z = vec_vspltisw(0);
			vec_float4 v = (vec_float4)vec_vmrghb((vec_uchar16)z, (vec_uchar16)inVector);
			v            = (vec_float4)vec_vmrghh((vec_ushort8)z, (vec_ushort8)v);
			v            = vec_vcfux((vec_uint4)v, 8);
			return Vector_4V(v);
#		endif
	}

	__forceinline Vector_4V_In V4Float32ToUint8(Vector_4V_In inVector)
	{
#		if __SPU
			qword q = si_cfltu((qword)inVector, 32);
			qword shuf = si_iohl(si_ilhu(0x0004),0x080c);
			return Vector_4V(si_shufb(q, q, shuf));
#		else
			vec_float4 v = (vec_float4)vec_vctuxs((vec_float4)inVector, 8);
			v = (vec_float4)vec_vpkuwus((vec_uint4)  v, (vec_uint4)  v);
			v = (vec_float4)vec_vpkuhus((vec_ushort8)v, (vec_ushort8)v);
			return Vector_4V(v);
#		endif
	}

	__forceinline Vector_4V_Out V4RoundToNearestInt(Vector_4V_In inVector)
	{
		return vec_round( inVector );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntZero(Vector_4V_In inVector)
	{
		return vec_trunc( inVector );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntNegInf(Vector_4V_In inVector)
	{
		return vec_floor( inVector );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntPosInf(Vector_4V_In inVector)
	{
		return vec_ceil( inVector );
	}

	//============================================================================
	// Comparison functions

	__forceinline Vector_4V_Out V4IsTrueV(bool b)
	{
#		if __SPU
	 		qword splat = si_ilh(0x0303);
	 		qword v0 = si_from_uchar(b);
	 		qword v1 = si_ceqbi(v0, 1);
	 		qword v2 = si_shufb(v1, v1, splat);
			return (vec_float4) v2;
#		else
	 		vec_uchar16 v0 = vec_lvsl(1, (unsigned char*)NULL);
	 		vec_uchar16 v1 = vec_lvsl(b, (unsigned char*)NULL);
	 		vec_float4  v2 = (vec_float4) vec_vcmpequb(v0, v1);
			return v2;
#		endif
	}

	__forceinline Vector_4V_Out V4IsNonZeroV(u32 b)
	{
#		if __SPU
	 		qword splat = si_ila(0x10203);
	 		qword v0 = si_from_uint(b);
	 		qword v1 = si_clgti(v0, 0);
	 		qword v2 = si_shufb(v1, v1, splat);
			return (vec_float4) v2;
#		else
	 		vec_uchar16 v0 = vec_lvsl(0,              (unsigned char*)NULL);
	 		vec_uchar16 v1 = vec_lvsl(__cntlzw(b)>>5, (unsigned char*)NULL);
	 		vec_float4  v2 = (vec_float4) vec_vcmpequb(v0, v1);
			return v2;
#		endif
	}

	__forceinline Vector_4V_Out V4IsEvenV( Vector_4V_In inVector )
	{
		Vector_4V evenOrOdd = V4And( (Vector_4V)vec_cts(inVector, 0), V4VConstant(V_INT_1) );
		return V4IsEqualIntV( evenOrOdd, V4VConstant(V_ZERO) );
	}

	__forceinline Vector_4V_Out V4IsOddV( Vector_4V_In inVector )
	{
		Vector_4V _ones = V4VConstant(V_INT_1);
		Vector_4V evenOrOdd = V4And( (Vector_4V)vec_cts(inVector, 0), _ones );
		return V4IsEqualIntV( evenOrOdd, _ones );
	}

	__forceinline unsigned int V4IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector )
	{
		return vec_all_in( testVector, boundsVector );
	}

	__forceinline unsigned int V4IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_all_eq( (Vector_4V_uint)inVector1, (Vector_4V_uint)inVector2 );
	}

	__forceinline unsigned int V4IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_all_ne( (Vector_4V_uint)inVector1, (Vector_4V_uint)inVector2 );
	}

	__forceinline Vector_4V_Out V4IsEqualIntV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return (Vector_4V)vec_cmpeq( (Vector_4V_uint)inVector1, (Vector_4V_uint)inVector2 );
	}

	__forceinline unsigned int V4IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_all_eq( inVector1, inVector2 );
	}

	__forceinline unsigned int V4IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_all_ne( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4IsEqualV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return (Vector_4V)vec_cmpeq( inVector1, inVector2 );
	}

	__forceinline unsigned int V4IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		return vec_all_gt( bigVector, smallVector );
	}

	__forceinline Vector_4V_Out V4IsGreaterThanV(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		return (Vector_4V)vec_cmpgt( bigVector, smallVector );
	}

	__forceinline Vector_4V_Out V4IsGreaterThanIntV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return (Vector_4V)vec_cmpgt( (Vector_4V_int)inVector1, (Vector_4V_int)inVector2 );
	}

	__forceinline unsigned int V4IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		return vec_all_ge( bigVector, smallVector );
	}

	__forceinline Vector_4V_Out V4IsGreaterThanOrEqualV(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		return (Vector_4V)vec_cmpge( bigVector, smallVector );
	}

	__forceinline unsigned int V4IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		return vec_all_lt( smallVector, bigVector );
	}

	__forceinline Vector_4V_Out V4IsLessThanV(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		return (Vector_4V)vec_cmplt( smallVector, bigVector );
	}

	__forceinline unsigned int V4IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		return vec_all_le( smallVector, bigVector );
	}

	__forceinline Vector_4V_Out V4IsLessThanOrEqualV(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		return (Vector_4V)vec_cmple( smallVector, bigVector );
	}

	__forceinline Vector_4V_Out V4SelectFT(Vector_4V_In choiceVector, Vector_4V_In zero, Vector_4V_In nonZero)
	{
		return vec_sel( zero, nonZero, (Vector_4V_uint)choiceVector );
	}

	__forceinline Vector_4V_Out V4Max(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_max( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Min(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_min( inVector1, inVector2 );
	}

#if __PPU
	template <u32 n_> class _CSigned5 // nasty hack to convert unsigned 5-bit value to signed 5-bit value
	{
	public:
		enum { n = n_ };
	};
	template <> class _CSigned5<16> { public: enum { n = -16 }; };
	template <> class _CSigned5<17> { public: enum { n = -15 }; };
	template <> class _CSigned5<18> { public: enum { n = -14 }; };
	template <> class _CSigned5<19> { public: enum { n = -13 }; };
	template <> class _CSigned5<20> { public: enum { n = -12 }; };
	template <> class _CSigned5<21> { public: enum { n = -11 }; };
	template <> class _CSigned5<22> { public: enum { n = -10 }; };
	template <> class _CSigned5<23> { public: enum { n = -9 }; };
	template <> class _CSigned5<24> { public: enum { n = -8 }; };
	template <> class _CSigned5<25> { public: enum { n = -7 }; };
	template <> class _CSigned5<26> { public: enum { n = -6 }; };
	template <> class _CSigned5<27> { public: enum { n = -5 }; };
	template <> class _CSigned5<28> { public: enum { n = -4 }; };
	template <> class _CSigned5<29> { public: enum { n = -3 }; };
	template <> class _CSigned5<30> { public: enum { n = -2 }; };
	template <> class _CSigned5<31> { public: enum { n = -1 }; };
#endif // __PPU

#define _SIGNED5(n_) _CSigned5<n_>::n

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftLeft( Vector_4V_In inVector )
	{
#if __PPU
		return (Vector_4V)vec_sl( (Vector_4V_uint)inVector, vec_splat_u32( _SIGNED5(shift) ) );
#elif __SPU
		return (Vector_4V)si_shli( (qword)inVector, shift );
#endif
	}

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftRight( Vector_4V_In inVector )
	{
#if __PPU
		return (Vector_4V)vec_sr( (Vector_4V_uint)inVector, vec_splat_u32( _SIGNED5(shift) ) );
#elif __SPU
		return (Vector_4V)si_rotmi( (qword)inVector, -shift );
#endif
	}

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftRightAlgebraic( Vector_4V_In inVector )
	{
#if __PPU
		return (Vector_4V)vec_sra( (Vector_4V_uint)inVector, vec_splat_u32( _SIGNED5(shift) ) );
#elif __SPU
		return (Vector_4V)si_rotmai( (qword)inVector, -shift );
#endif
	}

#undef _SIGNED5

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128LeftBytes( Vector_4V_In inVector )
	{
#if __PPU
		return vec_vsldoi( inVector, (vec_float4)vec_vspltisw(0), shift );
#elif __SPU
		return (Vector_4V)si_shlqbyi( (qword)inVector, shift );
#endif
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128RightBytes( Vector_4V_In inVector )
	{
#if __PPU
		return vec_vsldoi( (vec_float4)vec_vspltisw(0), inVector, (-shift)&15 );
#elif __SPU
		return (Vector_4V)si_rotqmbyi( (qword)inVector, -shift );
#endif
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128LeftBytes_UndefBytes( Vector_4V_In inVector )
	{
#if __PPU
		return vec_vsldoi( inVector, inVector, shift );
#elif __SPU
		return (Vector_4V)si_shlqbyi( (qword)inVector, shift );
#endif
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128RightBytes_UndefBytes( Vector_4V_In inVector )
	{
#if __PPU
		return vec_vsldoi( inVector, inVector, (-shift)&15 );
#elif __SPU
		return (Vector_4V)si_rotqmbyi( (qword)inVector, -shift );
#endif
	}

	__forceinline Vector_4V_Out V4And(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_and( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Or(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_or( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Xor(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_xor( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Andc(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_andc( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeXY(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_mergeh( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeZW(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return vec_mergel( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeXYShort(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return (Vector_4V)vec_mergeh( (Vector_4V_ushort)inVector1, (Vector_4V_ushort)inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeZWShort(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return (Vector_4V)vec_mergel( (Vector_4V_ushort)inVector1, (Vector_4V_ushort)inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeXYByte(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return (Vector_4V)vec_mergeh( (Vector_4V_uchar)inVector1, (Vector_4V_uchar)inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeZWByte(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return (Vector_4V)vec_mergel( (Vector_4V_uchar)inVector1, (Vector_4V_uchar)inVector2 );
	}

	__forceinline Vector_4V_Out V4UnpackLowUnsignedShort(Vector_4V_In inVector)
	{
#		if __PPU
			return (Vector_4V)vec_vmrghh( vec_vspltish(0), (Vector_4V_short)inVector );
#		else
			static const qword shuf = {
				0x80,0x80,0x00,0x01, 0x80,0x80,0x02,0x03,
				0x80,0x80,0x04,0x05, 0x80,0x80,0x06,0x07 };
			return (Vector_4V)si_shufb( (qword)inVector, (qword)inVector, shuf);
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackLowUnsignedByte(Vector_4V_In inVector)
	{
#		if __PPU
			return (Vector_4V)vec_vmrghb( vec_vspltisb(0), (Vector_4V_char)inVector );
#		else
			static const qword shuf = {
				0x80,0x00,0x80,0x01, 0x80,0x02,0x80,0x03,
				0x80,0x04,0x80,0x05, 0x80,0x06,0x80,0x07 };
			return (Vector_4V)si_shufb( (qword)inVector, (qword)inVector, shuf);
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackHighUnsignedShort(Vector_4V_In inVector)
	{
#		if __PPU
			return (Vector_4V)vec_vmrglh( vec_vspltish(0), (Vector_4V_short)inVector );
#		else
			static const qword shuf = {
				0x80,0x80,0x08,0x09, 0x80,0x80,0x0a,0x0b,
				0x80,0x80,0x0c,0x0d, 0x80,0x80,0x0e,0x0f };
			return (Vector_4V)si_shufb( (qword)inVector, (qword)inVector, shuf);
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackHighUnsignedByte(Vector_4V_In inVector)
	{
#		if __PPU
			return (Vector_4V)vec_vmrglb( vec_vspltisb(0), (Vector_4V_char)inVector );
#		else
			static const qword shuf = {
				0x80,0x08,0x80,0x09, 0x80,0x0a,0x80,0x0b,
				0x80,0x0c,0x80,0x0d, 0x80,0x0e,0x80,0x0f };
			return (Vector_4V)si_shufb( (qword)inVector, (qword)inVector, shuf);
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackLowSignedShort(Vector_4V_In inVector)
	{
		return (Vector_4V)vec_unpackh((Vector_4V_short)inVector);
	}

	__forceinline Vector_4V_Out V4UnpackLowSignedByte(Vector_4V_In inVector)
	{
		return (Vector_4V)vec_unpackh((Vector_4V_char)inVector);
	}

	__forceinline Vector_4V_Out V4UnpackHighSignedShort(Vector_4V_In inVector)
	{
		return (Vector_4V)vec_unpackl((Vector_4V_short)inVector);
	}

	__forceinline Vector_4V_Out V4UnpackHighSignedByte(Vector_4V_In inVector)
	{
		return (Vector_4V)vec_unpackl((Vector_4V_char)inVector);
	}

	__forceinline Vector_4V_Out V4PackSignedIntToSignedShort(Vector_4V_In in0, Vector_4V_In in1)
	{
		return (Vector_4V)vec_packs((Vector_4V_int)in0, (Vector_4V_int)in1);
	}

	__forceinline Vector_4V_Out V4PackSignedShortToSignedByte(Vector_4V_In in0, Vector_4V_In in1)
	{
		return (Vector_4V)vec_packs((Vector_4V_short)in0, (Vector_4V_short)in1);
	}

	__forceinline Vector_4V_Out V4PackSignedIntToUnsignedShort(Vector_4V_In in0, Vector_4V_In in1)
	{
		return (Vector_4V)vec_packsu((Vector_4V_int)in0, (Vector_4V_int)in1);
	}

	__forceinline Vector_4V_Out V4PackSignedShortToUnsignedByte(Vector_4V_In in0, Vector_4V_In in1)
	{
		return (Vector_4V)vec_packsu((Vector_4V_short)in0, (Vector_4V_short)in1);
	}

	__forceinline Vector_4V_Out V4PackFloatToGpuSignedShortAccurate(Vector_4V_In in)
	{
		// RSX uses the full range [0x8000..0x7fff] with even spacing and no
		// representation of zero.
		Vector_4V half      = V4VConstant(V_HALF);
		Vector_4V biasScale = V4AddScaled(half, in, half);
		Vector_4V clamped   = V4Clamp(biasScale, V4VConstant(V_ZERO), V4VConstant(V_ONE));
		Vector_4V scaled    = V4Scale(clamped, V4VConstantSplat<FLOAT_TO_INT(1.f*0xffff)>());
		Vector_4V integer32 = V4FloatToIntRaw<0>(scaled);
		Vector_4V shl1      = V4FloatToIntRaw<1>(scaled);
		Vector_4V rounding  = V4And(shl1, V4VConstant(V_INT_1));
				  integer32 = V4AddInt(integer32, rounding);
				  integer32 = V4SubtractInt(integer32, V4VConstantSplat<0x8000>());
		Vector_4V packed    = V4PackSignedIntToSignedShort(integer32, integer32);
		return packed;
	}

	__forceinline Vector_4V_Out V4UnpackLowGpuSignedShortToFloatAccurate(Vector_4V_In in)
	{
		Vector_4V integer16 = V4Xor(in, V4VConstantSplat<0x80008000>());
		Vector_4V integer32 = V4UnpackLowUnsignedShort(integer16);
		Vector_4V scale     = V4VConstantSplat<FLOAT_TO_INT(2.f/0xffff)>();
		Vector_4V bias      = V4VConstant(V_NEGONE);
		Vector_4V float_    = V4AddScaled(bias, V4IntToFloatRaw<0>(integer32), scale);
		return float_;
	}

	__forceinline Vector_4V_Out V4UnpackHighGpuSignedShortToFloatAccurate(Vector_4V_In in)
	{
		Vector_4V integer16 = V4Xor(in, V4VConstantSplat<0x80008000>());
		Vector_4V integer32 = V4UnpackHighUnsignedShort(integer16);
		Vector_4V scale     = V4VConstantSplat<FLOAT_TO_INT(2.f/0xffff)>();
		Vector_4V bias      = V4VConstant(V_NEGONE);
		Vector_4V float_    = V4AddScaled(bias, V4IntToFloatRaw<0>(integer32), scale);
		return float_;
	}

	__forceinline Vector_4V_Out V4PackNormFloats_11_11_10(Vector_4V_In in)
	{
#		if __PPU
			vec_int4 v = vec_vctsxs(in, 31);
			vec_int4 t0 = vec_vspltisw(-11);
			vec_int4 t1 = vec_vspltisw(10);
			vec_int4 x = vec_vsrw(v, (vec_uint4)t0);                    // shift right 21 bits
			vec_int4 y = vec_vsrw(vec_vsldoi(v, v, 4), (vec_uint4)t1);  // shift left  22 bits (32 - 10)
			vec_int4 z = vec_vsldoi(v, v, 8);                           // shift left  64 bits
			vec_int4 xmask  = vec_vsrw(t0, (vec_uint4)t0);              // 0x000007ff
			vec_int4 xymask = vec_vsrw(t0, (vec_uint4)t1);              // 0x003fffff
			vec_int4 zy  = vec_vsel(z,  y, (vec_uint4)xymask);
			vec_int4 zyx = vec_vsel(zy, x, (vec_uint4)xmask);
			return (Vector_4V)zyx;
#		else
			qword v = si_cflts((qword)in, 31);
			qword x = si_rotmi(v, -21);
			qword y = si_rotmi(si_shlqbyi(v, 4), -10);
			qword z = si_shlqbyi(v, 8);
			qword zmask  = si_ilhu(0xffc0);
			qword zymask = si_il(0xfffff800);
			qword zy  = si_selb(y, z,  zmask);
			qword zyx = si_selb(x, zy, zymask);
			return (Vector_4V)zyx;
#		endif
	}

	__forceinline Vector_4V_Out V4PackNormFloats_10_10_10_2(Vector_4V_In in)
	{
#		if __PPU
			vec_int4 t0 = vec_vspltisw(-10);
			vec_int4 t1 = vec_vspltisw(2);
			vec_int4 t2 = vec_vspltisw(12);
			vec_int4 v = vec_vctsxs(in, 31);
			vec_int4 x = vec_vsrw(v, (vec_uint4)t0);
			vec_int4 y = vec_vsrw(vec_vsldoi(v, v, 4), (vec_uint4)t2);
			vec_int4 z = vec_vsrw(vec_vsldoi(v, v, 8), (vec_uint4)t1);
			vec_int4 w = vec_vsldoi(v, v, 12);
			vec_int4 xmask   = vec_vsrw(t0, (vec_uint4)t0);     // 0x000003ff
			vec_int4 yxmask  = vec_vsrw(t0, (vec_uint4)t2);     // 0x000fffff
			vec_int4 zyxmask = vec_vsrw(t0, (vec_uint4)t1);     // 0x3ffffffe notice lsb wrong, but doesn't matter here
			vec_int4 yx = vec_vsel(y, x, (vec_uint4)xmask);
			vec_int4 wz = vec_vsel(w, z, (vec_uint4)zyxmask);
			vec_int4 wzyx = vec_vsel(wz, yx, (vec_uint4)yxmask);
			return (Vector_4V)wzyx;
#		else
			qword v = si_cflts((qword)in, 31);
			static const qword shifts = {-1,-1,-1,-22, -1,-1,-1,-12, -1,-1,-1,-2, 0,0,0,0};
			qword xyz = si_rotm(v, shifts);
			qword x = xyz;
			qword y = si_shlqbyi(xyz, 4);
			qword z = si_shlqbyi(xyz, 8);
			qword w = si_shlqbyi(v, 12);
			qword xmask  = si_il(0x3ff);
			qword wzmask = si_ilhu(0xfff0);
			qword wmask  = si_ilhu(0xc000);
			qword yx = si_selb(y, x, xmask);
			qword wz = si_selb(z, w, wmask);
			qword wzyx = si_selb(yx, wz, wzmask);
			return (Vector_4V)wzyx;
#		endif
	}

	__forceinline Vector_4V_Out V4PackNormFloats_10_10_10_X(Vector_4V_In in)
	{
#		if __PPU
			vec_int4 t0 = vec_vspltisw(-10);
			vec_int4 t1 = vec_vspltisw(2);
			vec_int4 t2 = vec_vspltisw(12);
			vec_int4 v = vec_vctsxs(in, 31);
			vec_int4 x = vec_vsrw(v, (vec_uint4)t0);
			vec_int4 y = vec_vsrw(vec_vsldoi(v, v, 4), (vec_uint4)t2);
			vec_int4 z = vec_vsrw(vec_vsldoi(v, v, 8), (vec_uint4)t1);
			vec_int4 xmask   = vec_vsrw(t0, (vec_uint4)t0);     // 0x000003ff
			vec_int4 yxmask  = vec_vsrw(t0, (vec_uint4)t2);     // 0x000fffff
			vec_int4 zyxmask = vec_vsrw(t0, (vec_uint4)t1);     // 0x3ffffffe notice lsb wrong, but doesn't matter here
			vec_int4 yx = vec_vsel(y, x, (vec_uint4)xmask);
			vec_int4 wz = vec_vandc(z, zyxmask);
			vec_int4 wzyx = vec_vsel(wz, yx, (vec_uint4)yxmask);
			return (Vector_4V)wzyx;
#		else
			qword v = si_cflts((qword)in, 31);
			static const qword shifts = {-1,-1,-1,-22, -1,-1,-1,-12, -1,-1,-1,-2, 0,0,0,0};
			qword xyz = si_rotm(v, shifts);
			qword x = xyz;
			qword y = si_shlqbyi(xyz, 4);
			qword z = si_shlqbyi(xyz, 8);
			qword xmask  = si_il(0x3ff);
			qword wzmask = si_ilhu(0xfff0);
			qword wmask  = si_ilhu(0xc000);
			qword yx = si_selb(y, x, xmask);
			qword wz = si_andc(z, wmask);
			qword wzyx = si_selb(yx, wz, wzmask);
			return (Vector_4V)wzyx;
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_11_11_10(Vector_4V_In in)
	{
#		if __PPU
			vec_int4 v = vec_vspltw((vec_int4)in, 0);
			vec_int4 t0 = vec_vspltisw(-11);            // -11 ≡ 21 mod 32
			vec_int4 t1 = vec_vspltisw(10);
			vec_int4 t2 = vec_vspltisw(0);
			vec_int4 t3 = vec_vsldoi(t0, t1, 4);
			vec_int4 shifts = vec_vsldoi(t3, t2, 8);    // equivalent to {21, 10, 0, 0}
			vec_int4 xyz = vec_vslw(v, (vec_uint4)shifts);
			vec_int4 t4 = vec_vspltisw(11);
			vec_int4 t5 = vec_vsrw(t0, (vec_uint4)t1);  // {0x003fffff, 0x003fffff, 0x003fffff, 0x003fffff}
			vec_int4 t6 = vec_vsrw(t0, (vec_uint4)t4);  // {0x001fffff, 0x001fffff, 0x001fffff, 0x001fffff}
			vec_int4 notMask = vec_vsldoi(t6, t5, 8);   // {0x001fffff, 0x001fffff, 0x003fffff, 0x003fffff}
			xyz = vec_vandc(xyz, notMask);
			vec_float4 xyzf = vec_vcfsx(xyz, 31);
			return (Vector_4V)xyzf;
#		else
			qword shufAAAA = si_ila(0x10203);
			qword v = si_shufb((qword)in, (qword)in, shufAAAA);
			static const qword shift = {0,0,0,21, 0,0,0,10, 0,0,0,0, 0,0,0,0};
			static const qword mask = {0xff,0xe0,0x00,0x00, 0xff,0xe0,0x00,0x00, 0xff,0xc0,0x00,0x00, 0x00,0x00,0x00,0x00};
			v = si_shl(v, shift);
			v = si_and(v, mask);
			v = si_csflt(v, 31);
			return (Vector_4V)v;
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_10_10_10_2(Vector_4V_In in)
	{
#		if __PPU
			vec_int4 v = vec_vspltw((vec_int4)in, 0);
			vec_int4 t0 = vec_vspltisw(0);
			vec_int4 t1 = vec_vspltisw(-10);                // -10 ≡ 22 mod 32
			vec_int4 t2 = vec_vspltisw(12);
			vec_int4 t3 = vec_vspltisw(2);
			vec_int4 t4 = vec_vmrghw(t0, t2);               // {0,  12, 0,  12}
			vec_int4 t5 = vec_vmrghw(t1, t3);               // {22, 2,  22, 2 }
			vec_int4 shift = vec_vmrghw(t4, t5);            // {0,  22, 12, 2 }
			vec_int4 wxyz = vec_vslw(v, (vec_uint4)shift);
			vec_int4 t6 = vec_vspltisw(-1);
			vec_int4 t7 = vec_vslw(t6, (vec_uint4)t1);
			vec_int4 t8 = vec_vslw(t6, (vec_uint4)vec_vspltisw(-2));
			vec_int4 mask = vec_vsldoi(t8, t7, 12);         // {0xc0000000, 0xffc00000, 0xffc00000, 0xffc00000}
			wxyz = vec_vand(wxyz, mask);
			// The precision error on converting w to a float is a factor of 2,
			// so we do a seperate conversion for it, then combine the results.
			vec_float4 wxyzf  = vec_vcfsx(wxyz, 31);
			vec_float4 wxyzf2 = vec_vcfsx(wxyz, 30);
			vec_float4 xyzwf = vec_vsldoi(wxyzf, wxyzf2, 4);
			return (Vector_4V)xyzwf;
#		else
			qword shufAAAA = si_ila(0x10203);
			qword v = si_shufb((qword)in, (qword)in, shufAAAA);
			static const qword shifts = {0,0,0,22, 0,0,0,12, 0,0,0,2, 0,0,0,0};
			static const qword masks  = {0xff,0xc0,0x00,0x00, 0xff,0xc0,0x00,0x00, 0xff,0xc0,0x00,0x00, 0xc0,0x00,0x00,0x00};
			v = si_shl(v, shifts);
			v = si_and(v, masks);
			qword f  = si_csflt(v, 31);
			qword f2 = si_csflt(v, 30);
			f = si_selb(f, f2, si_fsmbi(0x000f));
			return (Vector_4V)f;
#		endif
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_10_10_10_X(Vector_4V_In in)
	{
#		if __PPU
			vec_int4 v = vec_vspltw((vec_int4)in, 0);
			vec_int4 t0 = vec_vspltisw(-10);                // -10 ≡ 22 mod 32
			vec_int4 t1 = vec_vspltisw(2);
			vec_int4 t2 = vec_vmrghw(t0, t1);               // {22, 2,  22, 2 }
			vec_int4 t3 = vec_vspltisw(12);
			vec_int4 shift = vec_vmrghw(t2, t3);            // {22, 12, 2,  12}
			vec_int4 xyz = vec_vslw(v, (vec_uint4)shift);
			vec_int4 t4 = vec_vspltisw(-1);
			vec_int4 mask = vec_vslw(t4, (vec_uint4)t0);    // {0xffc00000, 0xffc00000, 0xffc00000, 0xffc00000}
			xyz = vec_vand(xyz, mask);
			vec_float4 xyzf  = vec_vcfsx(xyz, 31);
			return (Vector_4V)xyzf;
#		else
			qword shufAAAA = si_ila(0x10203);
			qword v = si_shufb((qword)in, (qword)in, shufAAAA);
			static const qword shifts = {0,0,0,22, 0,0,0,12, 0,0,0,2, 0,0,0,0};
			qword mask = si_ilhu(0xffc0);
			v = si_shl(v, shifts);
			v = si_and(v, mask);
			qword f  = si_csflt(v, 31);
			return (Vector_4V)f;
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

		static const Vector_4V_uchar s_controlVect = (Vector_4V_uchar)((Vector_4V_uint){permX, permY, permZ, permW});
		return vec_perm( v, v, s_controlVect );
	}

	// Specialize for cases where it's faster. #include some as necessary. You might think a "lvx" + "not having to include such big headers" is a better
	// option.
	#include "v4perm1instruction_ps3.inl"
//	#include "v4perm2instructions_ps3.inl"
//	#include "v4perm3instructions_ps3.inl"
//	#include "v4perm4instructions_ps3.inl"
//	#include "v4perm5instructions_ps3.inl"
//	#include "v4perm6instructions_ps3.inl"

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

		return vec_perm( v1, v2, (Vector_4V_uchar)V4VConstant<permX, permY, permZ, permW>() );
	}

	// Specialize for cases where it's faster. #include some as necessary. You might think a "lvx" + "not having to include such big headers" is a better
	// option.
	#include "v4permtwo1instruction_ps3.inl"
//	#include "v4permtwo2instructions_ps3.inl"
//	#include "v4permtwo3instructions_ps3.inl"
//	#include "v4permtwo4instructions_ps3.inl"
//	#include "v4permtwo5instructions_ps3.inl"
//	#include "v4permtwo6instructions_ps3.inl"

	// Make sure there are a couple 2-instruction permutes available that are commonly used

	// For Vec4V(Vec2V, Vec2V)
	template <>
	__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,X2,Y2>( Vector_4V_In v1, Vector_4V_In v2 )
	{
		Vector_4V _DOI_v1_v1_8_ = vec_sld( v1, v1, 8 );
		return vec_sld( _DOI_v1_v1_8_, v2, 8 );
	}

	// For Vec4V(Vec3V, ScalarV)
	template <>
	__forceinline Vector_4V_Out V4PermuteTwo<X1,Y1,Z1,X2>( Vector_4V_In v1, Vector_4V_In v2 )
	{
		Vector_4V _DOI_v1_v1_12_ = vec_sld( v1, v1, 12 );
		return vec_sld( _DOI_v1_v1_12_, v2, 4 );
	}

	// For Vec3V(ScalarV, Vec2V)
	template <>
	__forceinline Vector_4V_Out V4PermuteTwo<X1,X2,Y2,Z2>( Vector_4V_In v1, Vector_4V_In v2 )
	{
		Vector_4V _SX_v1_ = V4SplatX( v1 );
		return vec_sld( _SX_v1_, v2, 12 );
	}

	
	__forceinline Vector_4V_Out V4Permute( Vector_4V_In v, Vector_4V_In controlVect )
	{
		return vec_perm( v, v, (Vector_4V_uchar)controlVect );
	}

	__forceinline Vector_4V_Out V4PermuteTwo( Vector_4V_In v1, Vector_4V_In v2, Vector_4V_In controlVect )
	{
		return vec_perm( v1, v2, (Vector_4V_uchar)controlVect );
	}

	template <u8 perm0, u8 perm1, u8 perm2, u8 perm3, u8 perm4, u8 perm5, u8 perm6, u8 perm7, u8 perm8, u8 perm9, u8 perm10, u8 perm11, u8 perm12, u8 perm13, u8 perm14, u8 perm15>
	__forceinline Vector_4V_Out V4BytePermute( Vector_4V_In v )
	{
		CompileTimeAssert(	(perm0 >= 0 && perm0 <= 15) && (perm1 >= 0 && perm1 <= 15) &&
							(perm2 >= 0 && perm2 <= 15) && (perm3 >= 0 && perm3 <= 15) &&
							(perm4 >= 0 && perm4 <= 15) && (perm5 >= 0 && perm5 <= 15) &&
							(perm6 >= 0 && perm6 <= 15) && (perm7 >= 0 && perm7 <= 15) &&
							(perm8 >= 0 && perm8 <= 15) && (perm9 >= 0 && perm9 <= 15) &&
							(perm10 >= 0 && perm10 <= 15) && (perm11 >= 0 && perm11 <= 15) &&
							(perm12 >= 0 && perm12 <= 15) && (perm13 >= 0 && perm13 <= 15) &&
							(perm14 >= 0 && perm14 <= 15) && (perm15 >= 0 && perm15 <= 15)	);
							// Invalid permute args!

		CompileTimeAssert(	!(	perm0==0 && perm1==1 && perm2==2 && perm3==3 && perm4==4 && perm5==5 && perm6==6 && perm7==7 &&
								perm8==8 && perm9==9 && perm10==10 && perm11==11 && perm12==12 && perm13==13 && perm14==14 && perm15==16) );
							// This permute does nothing meaningful!

		return vec_perm( v, v, (Vector_4V_uchar)V4VConstant<perm0,perm1,perm2,perm3,perm4,perm5,perm6,perm7,perm8,perm9,perm10,perm11,perm12,perm13,perm14,perm15>() );
	}

	template <u8 perm0, u8 perm1, u8 perm2, u8 perm3, u8 perm4, u8 perm5, u8 perm6, u8 perm7, u8 perm8, u8 perm9, u8 perm10, u8 perm11, u8 perm12, u8 perm13, u8 perm14, u8 perm15>
	__forceinline Vector_4V_Out V4BytePermuteTwo( Vector_4V_In v1, Vector_4V_In v2 )
	{
		CompileTimeAssert(	(perm0 >= 0 && perm0 <= 31) && (perm1 >= 0 && perm1 <= 31) &&
							(perm2 >= 0 && perm2 <= 31) && (perm3 >= 0 && perm3 <= 31) &&
							(perm4 >= 0 && perm4 <= 31) && (perm5 >= 0 && perm5 <= 31) &&
							(perm6 >= 0 && perm6 <= 31) && (perm7 >= 0 && perm7 <= 31) &&
							(perm8 >= 0 && perm8 <= 31) && (perm9 >= 0 && perm9 <= 31) &&
							(perm10 >= 0 && perm10 <= 31) && (perm11 >= 0 && perm11 <= 31) &&
							(perm12 >= 0 && perm12 <= 31) && (perm13 >= 0 && perm13 <= 31) &&
							(perm14 >= 0 && perm14 <= 31) && (perm15 >= 0 && perm15 <= 31)	);
							// Invalid permute args!

		CompileTimeAssert(	!(	perm0==0 && perm1==1 && perm2==2 && perm3==3 && perm4==4 && perm5==5 && perm6==6 && perm7==7 &&
								perm8==8 && perm9==9 && perm10==10 && perm11==11 && perm12==12 && perm13==13 && perm14==14 && perm15==16) &&
							!(	perm0==15 && perm1==16 && perm2==17 && perm3==18 && perm4==19 && perm5==20 && perm6==21 && perm7==22 &&
								perm8==23 && perm9==24 && perm10==25 && perm11==26 && perm12==27 && perm13==28 && perm14==29 && perm15==30) );
							// This permute does nothing meaningful!

		return vec_perm( v1, v2, (Vector_4V_uchar)V4VConstant<perm0,perm1,perm2,perm3,perm4,perm5,perm6,perm7,perm8,perm9,perm10,perm11,perm12,perm13,perm14,perm15>() );
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
