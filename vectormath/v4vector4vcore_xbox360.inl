#include "system/floattoint.h"

namespace rage
{
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
#if !__OPTIMIZED
		FastAssert( FPIsFinite( Vec::GetX(inVector) ) == ( (u32)Vec::GetXi(retVal) == 0xFFFFFFFF ) && "Vectorized V4IsFiniteV() X result != scalar FPIsFinite() result!" );
		FastAssert( FPIsFinite( Vec::GetY(inVector) ) == ( (u32)Vec::GetYi(retVal) == 0xFFFFFFFF ) && "Vectorized V4IsFiniteV() Y result != scalar FPIsFinite() result!" );
		FastAssert( FPIsFinite( Vec::GetZ(inVector) ) == ( (u32)Vec::GetZi(retVal) == 0xFFFFFFFF ) && "Vectorized V4IsFiniteV() Z result != scalar FPIsFinite() result!" );
		FastAssert( FPIsFinite( Vec::GetW(inVector) ) == ( (u32)Vec::GetWi(retVal) == 0xFFFFFFFF ) && "Vectorized V4IsFiniteV() W result != scalar FPIsFinite() result!" );
#endif // !__OPTIMIZED
		return retVal;
	}

	__forceinline unsigned int V4IsFiniteAll( Vector_4V_In inVector )
	{
		__vector4 t0 = __vspltisw(-1);
		__vector4 t1 = __vspltisw(-8);          // -8 ≡ 24 mod 32
		__vector4 t2 = __vspltisw(1);
		__vector4 t3 = __vslw(t0, t1);          // 0xff000000
		__vector4 in = __vslw(inVector, t2);
		unsigned int cr;
		__vcmpgtuwR(t3, in, &cr);
		return (cr & 0x80) >> 7;
	}

	template<unsigned FROM> __forceinline void V4Store8(void *dst, Vector_4V_In src)
	{
		__stvebx(__vspltb(src, FROM), dst, 0);
	}

	template<unsigned FROM> __forceinline void V4Store16(void *dst, Vector_4V_In src)
	{
		FastAssert(((uptr)dst&1)==0);
		__stvehx(__vsplth(src, FROM), dst, 0);
	}

	template<unsigned FROM> __forceinline void V4Store32(void *dst, Vector_4V_In src)
	{
		FastAssert(((uptr)dst&3)==0);
		__stvewx(__vspltw(src, FROM), dst, 0);
	}

	template<unsigned FROM> __forceinline void V4Store64(void *dst, Vector_4V_In src)
	{
		FastAssert(((uptr)dst&7)==0);
		__stvewx(__vspltw(src, FROM*2+0), dst, 0);
		__stvewx(__vspltw(src, FROM*2+1), dst, 4);
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
		return __vspltw(inVector, 0);
	}

	__forceinline Vector_4V_Out V4SplatY( Vector_4V_In inVector )
	{
		return __vspltw(inVector, 1);
	}

	__forceinline Vector_4V_Out V4SplatZ( Vector_4V_In inVector )
	{
		return __vspltw(inVector, 2);
	}

	__forceinline Vector_4V_Out V4SplatW( Vector_4V_In inVector )
	{
		return __vspltw(inVector, 3);
	}

	__forceinline void V4Set( Vector_4V_InOut inoutVector, const float& x0, const float& y0, const float& z0, const float& w0 )
	{
		//SetX( inoutVector, x0 );
		//SetY( inoutVector, y0 );
		//SetZ( inoutVector, z0 );
		//SetW( inoutVector, w0 );

		Vector_4V xyzw, zw, y, w;

		// Check for misaligned float.
		FastAssert(((u32)&x0 & 3) == 0);
		FastAssert(((u32)&y0 & 3) == 0);
		FastAssert(((u32)&z0 & 3) == 0);
		FastAssert(((u32)&w0 & 3) == 0);

		xyzw = __lvlx(&x0, 0);
		y = __lvlx(&y0, 0);
		zw = __lvlx(&z0, 0);
		w = __lvlx(&w0, 0);
		xyzw = __vrlimi(xyzw, y, 4, 3);
		zw = __vrlimi(zw, w, 4, 3);
		xyzw = __vrlimi(xyzw, zw, 3, 2);

		inoutVector = xyzw;
	}

	__forceinline Vector_4V_Out V4AddInt( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return __vadduwm( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4SubtractInt( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return __vsubuwm( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Add( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return __vaddfp( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Subtract( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return __vsubfp( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4AddScaled( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In inVector3 )
	{
		return __vmaddfp( inVector2, inVector3, inVector1 );
	}

	__forceinline Vector_4V_Out V4SubtractScaled( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In inVector3 )
	{
		return __vnmsubfp( inVector2, inVector3, inVector1 );
	}

	__forceinline Vector_4V_Out V4Scale( Vector_4V_In inVector1, Vector_4V_In inVector2 )
	{
		return __vmulfp( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4InvertBits(Vector_4V_In inVector)
	{
		return __vnor( inVector, inVector );
	}

	__forceinline Vector_4V_Out V4Negate(Vector_4V_In inVector)
	{
#if !USE_ALTERNATE_NEGATE
		return V4Subtract( V4VConstant(V_ZERO), inVector );
#else
		// Invert the sign bits, by XORing with:
		// 0x80000000 80000000 80000000 80000000
		return V4Xor( inVector, V4VConstant(V_80000000) );
#endif
	}

	__forceinline Vector_4V_Out V4Abs(Vector_4V_In inVector)
	{
		// Zero the sign bits, by ANDing with:
		// 0x7FFFFFFF 7FFFFFFF 7FFFFFFF 7FFFFFFF.
		return V4Andc( inVector, V4VConstant(V_80000000) );
	}

	__forceinline Vector_4V_Out V4DotV(Vector_4V_In a, Vector_4V_In b)
	{
		return __vdot4fp(a, b);
	}

	__forceinline Vector_4V_Out V4MagSquaredV( Vector_4V_In v )
	{
		return __vdot4fp(v, v);
	}

	__forceinline Vector_4V_Out V4Expt( Vector_4V_In x )
	{
		return __vexptefp( x );
	}

	__forceinline Vector_4V_Out V4Log2( Vector_4V_In x )
	{
		return __vlogefp( x );
	}

	//============================================================================
	// Conversion functions

	template <int exponent>
	__forceinline Vector_4V_Out V4FloatToIntRaw(Vector_4V_In inVector)
	{
		return __vctsxs( inVector, exponent );
	}

	template <int exponent>
	__forceinline Vector_4V_Out V4IntToFloatRaw(Vector_4V_In inVector)
	{
		return __vcfsx( inVector, exponent );
	}

	__forceinline Vector_4V_Out V4Uint8ToFloat32(Vector_4V_In inVector)
	{
		__vector4 z = __vspltisw(0);
		__vector4 v = __vmrghb(z, inVector);
		v           = __vmrghh(z, v);
		v           = __vcfux(v, 8);
		return v;
	}

	__forceinline Vector_4V_In V4Float32ToUint8(Vector_4V_In inVector)
	{
		__vector4 v = __vctuxs(inVector, 8);
		v = __vpkuwus(v, v);
		v = __vpkuhus(v, v);
		return v;
	}

	__forceinline Vector_4V_Out V4RoundToNearestInt(Vector_4V_In inVector)
	{
		return __vrfin( inVector );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntZero(Vector_4V_In inVector)
	{
		return __vrfiz( inVector );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntNegInf(Vector_4V_In inVector)
	{
		return __vrfim( inVector );
	}

	__forceinline Vector_4V_Out V4RoundToNearestIntPosInf(Vector_4V_In inVector)
	{
		return __vrfip( inVector );
	}

	//============================================================================
	// Comparison functions

	__forceinline Vector_4V_Out V4IsTrueV(bool b)
	{
 		__vector4 v0 = __lvsl(NULL, 1);
 		__vector4 v1 = __lvsl(NULL, b);
 		__vector4 v2 = __vcmpequb(v0, v1);
		return v2;
	}

	__forceinline Vector_4V_Out V4IsNonZeroV(u32 b)
	{
 		__vector4 v0 = __lvsl(NULL, 0);
 		__vector4 v1 = __lvsl(NULL, _CountLeadingZeros(b)>>5);
 		__vector4 v2 = __vcmpequb(v0, v1);
		return v2;
	}

	__forceinline Vector_4V_Out V4IsEvenV( Vector_4V_In inVector )
	{
		Vector_4V evenOrOdd = V4And( __vctsxs(inVector, 0), V4VConstant(V_INT_1) );
		return V4IsEqualIntV( evenOrOdd, V4VConstant(V_ZERO) );
	}

	__forceinline Vector_4V_Out V4IsOddV( Vector_4V_In inVector )
	{
		Vector_4V _ones = V4VConstant(V_INT_1);
		Vector_4V evenOrOdd = V4And( __vctsxs(inVector, 0), _ones );
		return V4IsEqualIntV( evenOrOdd, _ones );
	}

	__forceinline unsigned int V4IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector )
	{
		unsigned int cr;
		__vcmpbfpR( testVector, boundsVector, &cr );
		return (cr & 0x20) >> 5;
	}

	__forceinline unsigned int V4IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		unsigned int cr;
		__vcmpequwR( inVector1, inVector2, &cr );
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V4IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		unsigned int cr;
		__vcmpequwR( inVector1, inVector2, &cr );
		return (cr & 0x20) >> 5;
	}

	__forceinline Vector_4V_Out V4IsEqualIntV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vcmpequw( inVector1, inVector2 );
	}

	__forceinline unsigned int V4IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		unsigned int cr;
		__vcmpeqfpR(inVector1, inVector2, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline unsigned int V4IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		unsigned int cr;
		__vcmpeqfpR(inVector1, inVector2, &cr);
		return (cr & 0x20) >> 5;
	}

	__forceinline Vector_4V_Out V4IsEqualV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vcmpeqfp( inVector1, inVector2 );
	}

	__forceinline unsigned int V4IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		unsigned int cr;
		__vcmpgtfpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline Vector_4V_Out V4IsGreaterThanV(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		return __vcmpgtfp( bigVector, smallVector );
	}

	__forceinline Vector_4V_Out V4IsGreaterThanIntV(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vcmpgtsw( inVector1, inVector2 );
	}

	__forceinline unsigned int V4IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		unsigned int cr;
		__vcmpgefpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline Vector_4V_Out V4IsGreaterThanOrEqualV(Vector_4V_In bigVector, Vector_4V_In smallVector)
	{
		return __vcmpgefp( bigVector, smallVector );
	}

	__forceinline unsigned int V4IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		unsigned int cr;
		__vcmpgtfpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline Vector_4V_Out V4IsLessThanV(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		return __vcmpgtfp( bigVector, smallVector );
	}

	__forceinline unsigned int V4IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		unsigned int cr;
		__vcmpgefpR(bigVector, smallVector, &cr);
		return (cr & 0x80) >> 7;
	}

	__forceinline Vector_4V_Out V4IsLessThanOrEqualV(Vector_4V_In smallVector, Vector_4V_In bigVector)
	{
		return __vcmpgefp( bigVector, smallVector );
	}

	__forceinline Vector_4V_Out V4SelectFT(Vector_4V_In choiceVector, Vector_4V_In zero, Vector_4V_In nonZero)
	{
		return __vsel( zero, nonZero, choiceVector );
	}

	__forceinline Vector_4V_Out V4Max(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vmaxfp( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Min(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vminfp( inVector1, inVector2 );
	}

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

#define _SIGNED5(n_) _CSigned5<n_>::n

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftLeft( Vector_4V_In inVector )
	{
		return __vslw( inVector, __vspltisw( _SIGNED5(shift) ) );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftRight( Vector_4V_In inVector )
	{
		return __vsrw( inVector, __vspltisw( _SIGNED5(shift) ) );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4ShiftRightAlgebraic( Vector_4V_In inVector )
	{
		return __vsraw( inVector, __vspltisw( _SIGNED5(shift) ) );
	}

#undef _SIGNED5

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128LeftBytes( Vector_4V_In inVector )
	{
		return __vsldoi( inVector, __vspltisw(0), shift );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128RightBytes( Vector_4V_In inVector )
	{
		return __vsldoi( __vspltisw(0), inVector, (-shift)&15 );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128LeftBytes_UndefBytes( Vector_4V_In inVector )
	{
		return __vsldoi( inVector, inVector, shift );
	}

	template <int shift>
	__forceinline Vector_4V_Out V4Shift128RightBytes_UndefBytes( Vector_4V_In inVector )
	{
		return __vsldoi( inVector, inVector, (-shift)&15 );
	}

	__forceinline Vector_4V_Out V4And(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vand( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Or(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vor( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Xor(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vxor( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4Andc(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vandc( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeXY(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vmrghw( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeZW(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vmrglw( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeXYShort(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vmrghh( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeZWShort(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vmrglh( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeXYByte(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vmrghb( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4MergeZWByte(Vector_4V_In inVector1, Vector_4V_In inVector2)
	{
		return __vmrglb( inVector1, inVector2 );
	}

	__forceinline Vector_4V_Out V4UnpackLowUnsignedShort(Vector_4V_In inVector)
	{
		return __vmrghh( __vspltish(0), inVector );
	}

	__forceinline Vector_4V_Out V4UnpackLowUnsignedByte(Vector_4V_In inVector)
	{
		return __vmrghb( __vspltisb(0), inVector );
	}

	__forceinline Vector_4V_Out V4UnpackHighUnsignedShort(Vector_4V_In inVector)
	{
		return __vmrglh( __vspltish(0), inVector );
	}

	__forceinline Vector_4V_Out V4UnpackHighUnsignedByte(Vector_4V_In inVector)
	{
		return __vmrglb( __vspltisb(0), inVector );
	}

	__forceinline Vector_4V_Out V4UnpackLowSignedShort(Vector_4V_In inVector)
	{
		return __vupkhsh(inVector);
	}

	__forceinline Vector_4V_Out V4UnpackLowSignedByte(Vector_4V_In inVector)
	{
		return __vupkhsb(inVector);
	}

	__forceinline Vector_4V_Out V4UnpackHighSignedShort(Vector_4V_In inVector)
	{
		return __vupklsh(inVector);
	}

	__forceinline Vector_4V_Out V4UnpackHighSignedByte(Vector_4V_In inVector)
	{
		return __vupklsb(inVector);
	}

	__forceinline Vector_4V_Out V4PackSignedIntToSignedShort(Vector_4V_In in0, Vector_4V_In in1)
	{
		return __vpkswss(in0, in1);
	}

	__forceinline Vector_4V_Out V4PackSignedIntToUnsignedShort(Vector_4V_In in0, Vector_4V_In in1)
	{
		return __vpkswus(in0, in1);
	}

	__forceinline Vector_4V_Out V4PackSignedShortToSignedByte(Vector_4V_In in0, Vector_4V_In in1)
	{
		return __vpkshss(in0, in1);
	}

	__forceinline Vector_4V_Out V4PackSignedShortToUnsignedByte(Vector_4V_In in0, Vector_4V_In in1)
	{
		return __vpkshus(in0, in1);
	}

	__forceinline Vector_4V_Out V4PackFloatToGpuSignedShortAccurate(Vector_4V_In in)
	{
		// GPU does not use the full range [0x8000..0x7fff], rather [0x8001..0x7fff].

		__vector4 m0 = __vspltisw(-15);         // 0xfffffff1 (-15 ≡ 17 mod 32)
		__vector4 m1 = __vsrw(m0, m0);          // 0x00007fff
		__vector4 m2 = __vcfsx(m1, 22);         // (float)0x7fff/0x400000

		__vector4 a0 = __vspltisw(3);
		__vector4 a1 = __vcfsx(a0, 0);          // 3.f

		__vector4 tmp = __vmaddfp(in, m2, a1);
		tmp = __vpkd3d(tmp, tmp, VPACK_NORMSHORT4, VPACK_64HI, 2);
		return tmp;
	}

	__forceinline Vector_4V_Out V4UnpackHighGpuSignedShortToFloatAccurate(Vector_4V_In in)
	{
		__vector4 m0 = __vspltisw(-15);         // 0xfffffff1 (-15 ≡ 17 mod 32)
		__vector4 m1 = __vsrw(m0, m0);          // 0x00007fff
		__vector4 m2 = __vcfsx(m1, 22);         // (float)0x7fff/0x400000
		__vector4 m3 = __vrefp(m2);             // (float)0x400000/0x7fff

		__vector4 a0 = __vspltisw(-3);
		__vector4 a1 = __vcfsx(a0, 0);          // -3.f
		__vector4 a2 = __vmulfp(a1, m3);        // -3.f*0x400000/0x7fff

		__vector4 tmp = __vupkd3d(in, VPACK_NORMSHORT4);
		tmp = __vmaddfp(tmp, m3, a2);
		return tmp;
	}

	__forceinline Vector_4V_Out V4UnpackLowGpuSignedShortToFloatAccurate(Vector_4V_In in)
	{
		return V4UnpackHighGpuSignedShortToFloatAccurate(__vsldoi(in, in, 8));
	}

	__forceinline Vector_4V_Out V4PackNormFloats_11_11_10(Vector_4V_In in)
	{
		__vector4 v = __vctsxs(in, 31);
		__vector4 t0 = __vspltisw(-11);
		__vector4 t1 = __vspltisw(10);
		__vector4 x = __vsrw(v, t0);                    // shift right 21 bits
		__vector4 y = __vsrw(__vsldoi(v, v, 4), t1);    // shift left  22 bits (32 - 10)
		__vector4 z = __vsldoi(v, v, 8);                // shift left  64 bits
		__vector4 xmask  = __vsrw(t0, t0);              // 0x000007ff
		__vector4 xymask = __vsrw(t0, t1);              // 0x003fffff
		__vector4 zy  = __vsel(z,  y, xymask);
		__vector4 zyx = __vsel(zy, x, xmask);
		return (Vector_4V)zyx;
	}

	__forceinline Vector_4V_Out V4PackNormFloats_10_10_10_2(Vector_4V_In in)
	{
		// The documentation for __vpkd3d is close to useless.
		//
		// The way the values for W are mapped are
		//      3.f          (0x40400000)  0
		//      3.000000238f (0x40400001)  1
		//      3.000000477f (0x40400002)  2
		//      3.000000715f (0x40400003)  3
		// Values outside of these numbers saturate.
		//
		// So despite the fact W is converted unsigned, the base is 3.f not 1.f.
		//
		// Since the GPU uses signed W for D3DDECLTYPE_DEC4N, we need to map
		//      -1.f -> 3.000000477f
		//      +1.f -> 3.000000238f
		// Notice that we are not supporting W == 0.f.
		//
		// w' = (1.5f-w*0.5f)/0x400000 + 3.f
		//    = w*-1.f/0x800000 + 3.f + 3.f/0x800000
		//
		// BUT.....
		//
		// The constant we need to add cannot be represented as a float (because
		// we want to represent half a ulp).  .: it is impossible to create the
		// appropriate input value for __vpkd3d using just a __vmaddfp.
		//
		// So we create the x, y and z components using a __vmaddfp, while in
		// parallel creating the w component, then mask insert before __vpkd3d.
		//

		__vector4 m0 = __vspltisw(-9);          // -9 ≡ 23 mod 32
		__vector4 m1 = __vsrw(m0, m0);          // 0x000001ff
		__vector4 m2 = __vcfsx(m1, 22);         // (float)0x1ff/0x400000

		__vector4 a0 = __vspltisw(3);
		__vector4 a1 = __vcfsx(a0, 0);          // 3.f

		__vector4 v = __vmaddfp(in, m2, a1);

		__vector4 w0 = __vspltisb(4);           // 0x04040404
		__vector4 w1 = __vslb(w0, w0);          // 0x40404040
		__vector4 w2 = __vspltish(1);           // 0x00010001
		__vector4 w3 = __vmrghh(w1, w2);        // 0x40400001
		__vector4 w4 = __vspltisw(-1);          // -1 ≡ 31 mod 32
		__vector4 w5 = __vsrw(in, w4);          // w<0.f ? 0x00000001 : 0x00000000
		__vector4 w6 = __vadduwm(w3, w5);       // w<0.f ? 0x40400002 : 0x40400001
		v = __vrlimi(v, w6, 1, 0);

		v = __vpkd3d(v, v, VPACK_NORMPACKED32, VPACK_32, 3);
		return (Vec::Vector_4V)v;
	}

	__forceinline Vector_4V_Out V4PackNormFloats_10_10_10_X(Vector_4V_In in)
	{
		__vector4 m0 = __vspltisw(-9);          // -9 ≡ 23 mod 32
		__vector4 m1 = __vsrw(m0, m0);          // 0x000001ff
		__vector4 m2 = __vcfsx(m1, 22);         // (float)0x1ff/0x400000

		__vector4 a0 = __vspltisw(3);
		__vector4 a1 = __vcfsx(a0, 0);          // 3.f

		// "Zero" out the the w-component of our multipler.  Actually using a
		// tiny denormal instead since we already have the constant, so can save
		// an additional __vspltisw(0).  After the mad, any finite input w value
		// will result in 3.f, which after packing will go to 0.
		m2 = __vrlimi(m2, a0, 1, 0);

		__vector4 v = __vmaddfp(in, m2, a1);
		v = __vpkd3d(v, v, VPACK_NORMPACKED32, VPACK_32, 3);
		return (Vec::Vector_4V)v;
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_11_11_10(Vector_4V_In in)
	{
		__vector4 v = __vspltw(in, 0);
		__vector4 t0 = __vspltisw(-11);             // -11 ≡ 21 mod 32
		__vector4 t1 = __vspltisw(10);
		__vector4 t2 = __vspltisw(0);
		__vector4 t3 = __vsldoi(t0, t1, 4);
		__vector4 shifts = __vsldoi(t3, t2, 8);     // equivalent to {21, 10, 0, 0}
		__vector4 xyz = __vslw(v, shifts);
		__vector4 t4 = __vspltisw(11);
		__vector4 t5 = __vsrw(t0, t1);              // {0x003fffff, 0x003fffff, 0x003fffff, 0x003fffff}
		__vector4 t6 = __vsrw(t0, t4);              // {0x001fffff, 0x001fffff, 0x001fffff, 0x001fffff}
		__vector4 notMask = __vsldoi(t6, t5, 8);    // {0x001fffff, 0x001fffff, 0x003fffff, 0x003fffff}
		xyz = __vandc(xyz, notMask);
		__vector4 xyzf = __vcfsx(xyz, 31);
		return (Vector_4V)xyzf;
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_10_10_10_2(Vector_4V_In in)
	{
		__vector4 m0 = __vspltisw(-9);          // -9 ≡ 23 mod 32
		__vector4 m1 = __vsrw(m0, m0);          // 0x000001ff
		__vector4 m2 = __vcfsx(m1, 22);         // (float)0x1ff/0x400000
		__vector4 m3 = __vrefp(m2);             // (float)0x400000/0x1ff

		__vector4 a0 = __vspltisw(-3);
		__vector4 a1 = __vcfsx(a0, 0);          // -3.f
		__vector4 a2 = __vmulfp(a1, m3);        // -3.f*0x400000/0x1ff

		// __vupkd3d seems to do something completely weird and undocumented.
		// Not exactly sure, but it seems to be using the logical OR of all
		// words to unpack.
		__vector4 inSplat = __vspltw(in, 0);
		__vector4 v = __vupkd3d(inSplat, VPACK_NORMPACKED32);
		v = __vmaddfp(v, m3, a2);

		__vector4 sign = V4VConstant(V_80000000);
		__vector4 one = V4VConstant(V_ONE);
		__vector4 w = V4And(sign, in);
		w = V4Or(w, one);

		v = __vrlimi(v, w, 1, 1);
		return (Vector_4V)v;
	}

	__forceinline Vector_4V_Out V4UnpackNormFloats_10_10_10_X(Vector_4V_In in)
	{
		__vector4 m0 = __vspltisw(-9);          // -9 ≡ 23 mod 32
		__vector4 m1 = __vsrw(m0, m0);          // 0x000001ff
		__vector4 m2 = __vcfsx(m1, 22);         // (float)0x1ff/0x400000
		__vector4 m3 = __vrefp(m2);             // (float)0x400000/0x1ff

		__vector4 a0 = __vspltisw(-3);
		__vector4 a1 = __vcfsx(a0, 0);          // -3.f
		__vector4 a2 = __vmulfp(a1, m3);        // -3.f*0x400000/0x1ff

		__vector4 inSplat = __vspltw(in, 0);
		__vector4 v = __vupkd3d(inSplat, VPACK_NORMPACKED32);
		v = __vmaddfp(v, m3, a2);

		return (Vector_4V)v;
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
		return __vpermwi( v, SWIZZLE_VAL_XBOX360( MASK_IT(permX), MASK_IT(permY), MASK_IT(permZ), MASK_IT(permW) ) );
#undef MASK_IT
	}

	// Even though the next three specializations aren't needed (__vpermwi() above can handle them), there seems to be slightly less code produced when the
	// vsldoi instructions are used instead. Try using __declspec(noinline) rage::Determinant_Imp() as an example. An extra "vor" is emitted.
	// This may be to move a register away from a destructive operation?
	template <>
	__forceinline Vector_4V_Out V4Permute<Y,Z,W,X>( Vector_4V_In v )
	{
		return __vsldoi( v, v, 0x4 );
	}
	template <>
	__forceinline Vector_4V_Out V4Permute<Z,W,X,Y>( Vector_4V_In v )
	{
		return __vsldoi( v, v, 0x8 );
	}
	template <>
	__forceinline Vector_4V_Out V4Permute<W,X,Y,Z>( Vector_4V_In v )
	{
		return __vsldoi( v, v, 0xC );
	}

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

		return __vperm( v1, v2, V4VConstant<permX, permY, permZ, permW>() );
	}

	// Specialize for cases where it's faster. #include some as necessary. You might think a "lvx" + "not having to include such big headers" is a better
	// option.
#pragma warning ( push )
#pragma warning(disable:4100) // disable warning about not using a parameter
	#include "v4permtwo1instruction_xbox360.inl"
	//#include "v4permtwo2instructions_xbox360.inl"
	//#include "v4permtwo3instructions_xbox360.inl"
#pragma warning ( pop )

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

		return __vperm( v, v, V4VConstant<perm0,perm1,perm2,perm3,perm4,perm5,perm6,perm7,perm8,perm9,perm10,perm11,perm12,perm13,perm14,perm15>() );
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

		return __vperm( v1, v2, V4VConstant<perm0,perm1,perm2,perm3,perm4,perm5,perm6,perm7,perm8,perm9,perm10,perm11,perm12,perm13,perm14,perm15>() );
	}

	__forceinline Vector_4V_Out V4Permute( Vector_4V_In v, Vector_4V_In controlVect )
	{
		return __vperm( v, v, controlVect );
	}

	__forceinline Vector_4V_Out V4PermuteTwo( Vector_4V_In v1, Vector_4V_In v2, Vector_4V_In controlVect )
	{
		return __vperm( v1, v2, controlVect );
	}


	//============================================================================
	// Quaternions

	__forceinline Vector_4V_Out V4QuatMultiply( Vector_4V_In inQ1, Vector_4V_In inQ2 )
	{
		// Reversing names just b/c they were wrong when implementing the function.
		Vector_4V inQuat1 = inQ2;
		Vector_4V inQuat2 = inQ1;

		Vector_4V negQuat1;
		Vector_4V pInputPNPP, qInputPPNP, pInputNPPP, qInputNNNP;
		Vector_4V inQuat2wzyx, inQuat2zwxy, inQuat2yxwz;
		Vector_4V x, y, z, w, xz, yw;

		inQuat2zwxy = V4Permute<Z, W, X, Y>(inQuat2);
		inQuat2yxwz = V4Permute<Y, X, W, Z>(inQuat2);
		inQuat2wzyx = V4Permute<W, Z, Y, X>(inQuat2);

		negQuat1 = V4Negate( inQuat1 );

		// These four become four vrlimi128 instructions.
		qInputNNNP = V4PermuteTwo<X1,Y1,Z1,W2>(negQuat1, inQuat1); // (inquat.___w)
		qInputPPNP = V4PermuteTwo<X2,Y2,Z1,W2>(negQuat1, inQuat1); // (inquat.xy_w)
		pInputNPPP = V4PermuteTwo<X1,Y2,Z2,W2>(negQuat1, inQuat1); // (inquat._yzw)
		pInputPNPP = V4PermuteTwo<X2,Y1,Z2,W2>(negQuat1, inQuat1); // (inquat.x_zw)

		w = V4DotV( qInputNNNP, inQuat2 );
		y = V4DotV( qInputPPNP, inQuat2zwxy );

		z = V4DotV( pInputNPPP, inQuat2yxwz );
		x = V4DotV( pInputPNPP, inQuat2wzyx );

		yw = V4MergeXY(y, w);
		xz = V4MergeXY(x, z);
		return V4MergeXY(xz, yw);
	}

	
} // namespace Vec
} // namespace rage
