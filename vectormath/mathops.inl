
#ifndef MY_PI
#define MY_PI	(3.14159265358979323846264338327950288f)
#endif

#ifndef MY_2_PI
#define MY_2_PI	(6.283185307179586476925286766559f)
#endif

#if RSG_CPU_INTEL && defined(_MSC_VER)
extern "C" unsigned char _BitScanForward(unsigned long* Index, unsigned long Mask);
extern "C" unsigned char _BitScanReverse(unsigned long* Index, unsigned long Mask);
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif // RSG_CPU_INTEL

namespace rage
{
#if __PPU
	extern float __Z_E_R_O__;
#endif

	// PURPOSE:	Count the number (between 0 and 32 inclusive) of consecutive zero bits starting at the most significant bit.
	__forceinline u32 CountLeadingZeros(u32 a)
	{
#if __SPU
		return si_to_int(si_clz(si_from_int(a)));
#elif __PPU
		return __cntlzw(a);
#elif __XENON
		return _CountLeadingZeros( (long)(a) );
#elif RSG_ORBIS
		return __builtin_clz(a);
#else
		unsigned long firstBit;
		return _BitScanReverse(&firstBit, a) ? (31 - firstBit) : 32;
#endif
	}

	// PURPOSE:	Count the number (between 0 and 32 inclusive) of consecutive zero bits starting at the less significant bit.
	__forceinline u32 CountTrailingZeros(u32 a)
	{
#if RSG_ORBIS
		// TODO: Untested.
		return a? __builtin_ctz(a) : 32;
#elif RSG_CPU_INTEL
		unsigned long firstBit;
		return _BitScanForward(&firstBit, a) ? firstBit : 32;
#else
		// taken from PowerPC Compiler Writer's Guide
		return 31u - CountLeadingZeros(a & (-s32(a)));
#endif
	}

	// PURPOSE: Shift left with rotation
	__forceinline u32 RotateLeft(u32 a)
	{
		return a<<1 | a>>31;
	}

	// PURPOSE: Shift right with rotation
	__forceinline u32 RotateRight(u32 a)
	{
		return a>>1 | a<<31;
	}

	// PURPOSE:	Determine whether the given floating point number has a finite value.
	// PARAMS:
	//	i -	the number to test
	// RETURN:	true if the given number has a finite value, false if it does not
	inline bool FPIsFinite(float i) 
	{
#if __SPU
		// SPU doesn't have proper infinity support, but it also has a unified register file
		// so the bitwise test isn't too bad.  The multiply by zero check definitely failed on SPU.
		// Note that on SPU, an exponent of 255 is valid, but chances are any number that large
		// was really an Inf or NaN from the PPU.
		return (si_to_int(si_from_float(i)) & 0x7f800000) != 0x7f800000;
#elif __PPU
		// On PS3 PPU, need this intrinsic instead of the #else code block, to avoid problems with -ffast-math.
		// And with SNC, we can't use a literal 0.0 here or the compiler STILL optimizes it out!
		return __fmul( i, __Z_E_R_O__ ) == __Z_E_R_O__;
#elif RSG_CPU_X64
		// On SSE, a floating-point scalar comparison using a sNaN will not update the EFLAGS register. It's better to check if the exponent equals 255.
		return ((u32&)i & 0x7f800000) != 0x7f800000;
#else
		// On all other architectures, if multiplying by zero doesn't produce zero, then it's not finite.
		return (i * 0.0f) == 0.0f;
#endif
	}

	__forceinline u32 FPPack(float value,u32 size,u32 shift)
	{
		float scale = (float)((1 << (size-1)) - 1);
		return (u32(value * scale) & ((1 << size)-1)) << shift;
	}

	__forceinline bool FloatSameSignAsFloat( const float* floatInMemory1, const float* floatInMemory2 )
	{
		u32 nLocalInt1 = *(reinterpret_cast<const u32*>(floatInMemory1));
		u32 nLocalInt2 = *(reinterpret_cast<const u32*>(floatInMemory2));
		return ((nLocalInt1 & 0x80000000) ^ (nLocalInt2 & 0x80000000)) == 0;
	}

	__forceinline bool FloatEqualsFloatAsInt( const float* floatInMemory1, u32 float2AsInt )
	{
		u32 nLocalInt1 = *(reinterpret_cast<const u32*>(floatInMemory1));
		u32 nLocalInt2 = float2AsInt;
		return (nLocalInt1 == nLocalInt2);
	}

	__forceinline bool FloatEqualsFloat( const float* floatInMemory1, const float* floatInMemory2 )
	{
		u32 nLocalInt1 = *(reinterpret_cast<const u32*>(floatInMemory1));
		u32 nLocalInt2 = *(reinterpret_cast<const u32*>(floatInMemory2));
		return (nLocalInt1 == nLocalInt2);
	}

	__forceinline bool FloatNeqFloat( const float* floatInMemory1, const float* floatInMemory2 )
	{
		u32 nLocalInt1 = *(reinterpret_cast<const u32*>(floatInMemory1));
		u32 nLocalInt2 = *(reinterpret_cast<const u32*>(floatInMemory2));
		return (nLocalInt1 != nLocalInt2);
	}

	__forceinline bool FloatEqualsFloatSafe( const float* floatInMemory1, const float* floatInMemory2 )
	{
		u32 nLocalInt1 = *(reinterpret_cast<const u32*>(floatInMemory1));
		u32 nLocalInt2 = *(reinterpret_cast<const u32*>(floatInMemory2));

		// Might be faster, even though it branches:
		//return (nLocalInt1 == nLocalInt2) || ((nLocalInt1 & 0x7FFFFFFF) == 0 && (nLocalInt2 & 0x7FFFFFFF) == 0);

		u32 result1 = (nLocalInt1 ^ nLocalInt2); // needs to be 0...
		u32 result2 = ((nLocalInt1 & 0x7FFFFFFF) | (nLocalInt2 & 0x7FFFFFFF)); // ...OR, this needs to be 0
		return (((result1 & 0xFFFF) * (result2 & 0xFFFF)) | ((result1 >> 16) * (result2 >> 16))) == 0;
	}

	__forceinline bool FloatNeqFloatSafe( const float* floatInMemory1, const float* floatInMemory2 )
	{
		u32 nLocalInt1 = *(reinterpret_cast<const u32*>(floatInMemory1));
		u32 nLocalInt2 = *(reinterpret_cast<const u32*>(floatInMemory2));

		// Might be faster, even though it branches:
		//return (nLocalInt1 != nLocalInt2) && ((nLocalInt1 & 0x7FFFFFFF) != 0 || (nLocalInt2 & 0x7FFFFFFF) != 0);

		u32 result1 = (nLocalInt1 ^ nLocalInt2); // needs to be nonzero...
		u32 result2 = ((nLocalInt1 & 0x7FFFFFFF) | (nLocalInt2 & 0x7FFFFFFF)); // ...AND, this must not be 0
		return (((result1 & 0xFFFF) * (result2 & 0xFFFF)) | ((result1 >> 16) * (result2 >> 16))) != 0;
	}

	__forceinline bool FloatEqualsZero( const float* floatInMemory )
	{
		u32 nLocalInt = *(reinterpret_cast<const u32*>(floatInMemory));
		// Equals 0, ignoring sign bits.
		return ((nLocalInt & 0x7FFFFFFF) == 0);
	}

	__forceinline bool FloatNeqZero( const float* floatInMemory )
	{
		u32 nLocalInt = *(reinterpret_cast<const u32*>(floatInMemory));
		// Does not equal 0, ignoring sign bits.
		return ((nLocalInt & 0x7FFFFFFF) != 0);
	}

	__forceinline bool FloatGreaterThanZero( const float* floatInMemory )
	{
		u32 nLocalInt = *(reinterpret_cast<const u32*>(floatInMemory));
		// Pos sign and nonzero bits otherwise.
		return ( (nLocalInt & 0x80000000) == 0 && (nLocalInt & 0x7FFFFFFF) != 0);
	}

	__forceinline bool FloatLessThanZero( const float* floatInMemory )
	{
		u32 nLocalInt = *(reinterpret_cast<const u32*>(floatInMemory));
		// Neg sign and nonzero bits otherwise.
		return ( (nLocalInt & 0x80000000) != 0 && (nLocalInt & 0x7FFFFFFF) != 0);
	}

	__forceinline bool FloatGreaterThanOrEqualZero( const float* floatInMemory )
	{
		u32 nLocalInt = *(reinterpret_cast<const u32*>(floatInMemory));
		// Non-negative sign, OR zero'd bits.
		return ( (nLocalInt & 0x80000000) == 0 ) || ((nLocalInt & 0x7FFFFFFF) == 0);
	}

	__forceinline bool FloatLessThanOrEqualZero( const float* floatInMemory )
	{
		u32 nLocalInt = *(reinterpret_cast<const u32*>(floatInMemory));
		// Negative sign, OR zero'd bits.
		return ( (nLocalInt & 0x80000000) != 0 ) || ((nLocalInt & 0x7FFFFFFF) == 0);
	}

	__forceinline float FPIfGteZeroThenElse( float fVal, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( fVal, selectA, selectB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		// "Use ternary operator for SPUs because the compiler turns this into a selb for us" (Ryan Mack)
		// TODO: Confirm this for all operators >, >=, <, <=, ==, !=...
		return ( fVal >= 0.0f ? selectA : selectB );
#endif
	}

	__forceinline float FPIfLtZeroThenElse( float fVal, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( fVal, selectB, selectA );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( fVal < 0.0f ? selectA : selectB );
#endif
	}

	__forceinline float FPIfGtZeroThenElse( float fVal, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( -fVal, selectB, selectA );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( fVal > 0.0f ? selectA : selectB );
#endif
	}

	__forceinline float FPIfLteZeroThenElse( float fVal, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( -fVal, selectA, selectB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( fVal <= 0.0f ? selectA : selectB );
#endif
	}

	__forceinline float FPIfEqZeroThenElse( float fVal, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( fVal, (float)__fsel( -fVal, selectA, selectB ), selectB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( fVal == 0.0f ? selectA : selectB );
#endif
	}

	__forceinline float FPIfNeqZeroThenElse( float fVal, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( fVal, (float)__fsel( -fVal, selectB, selectA ), selectA );
		//#elif __SPU && USE_INTRINSICS
		//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( fVal != 0.0f ? selectA : selectB );
#endif
	}

	__forceinline float FPIfGteThenElse( float opA, float opB, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( (opA-opB), selectA, selectB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
		// Tried using branchless x87 ops, but it's not even close to the Intel-generated conditional branch performance, even with 
		// hard-to-predict (random) comparisons as the benchmark.
		//float retVal;
		//__asm {
		//	fld			dword ptr [d]
		//	fld			dword ptr [c]
		//	fld			dword ptr [a]
		//	fld			dword ptr [b]
		//	fcomi		st(0), st(1)	// b compare a
		//	fstp		st(0)			// pop b
		//	fstp		st(0)			// pop a
		//	fcmovbe		st(0), st(1)	// ST0=c,ST1=d. Move ST1 into ST0 if b compare a returned NBE
		//	fstp		dword ptr [retVal]
		//}
		// (even a simple fcomi+fcmovbe, w/o stack management, is slower)
		//return retVal;
#else
		return ( opA >= opB ? selectA : selectB );
#endif
	}

	__forceinline float FPIfLteThenElse( float opA, float opB, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( (opB-opA), selectA, selectB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( opA <= opB ? selectA : selectB );
#endif
	}

	__forceinline float FPIfEqThenElse( float opA, float opB, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( (opA-opB), (float)__fsel( (opB-opA), selectA, selectB ), selectB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( opA == opB ? selectA : selectB );
#endif
	}

	__forceinline float FPIfNeqThenElse( float opA, float opB, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( (opA-opB), (float)__fsel( (opB-opA), selectB, selectA ), selectA );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( opA != opB ? selectA : selectB );
#endif
	}

	__forceinline float FPIfGtThenElse( float opA, float opB, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( (opA-opB), (float)__fsel( (opB-opA), selectB, selectA ), selectB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( opA > opB ? selectA : selectB );
#endif
	}

	__forceinline float FPIfLtThenElse( float opA, float opB, float selectA, float selectB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( (opB-opA), (float)__fsel( (opA-opB), selectB, selectA ), selectB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( opA < opB ? selectA : selectB );
#endif
	}

	__forceinline float FPMin( float fValA, float fValB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( (fValA-fValB), fValB, fValA );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( fValA < fValB ? fValA : fValB );
#endif
	}

	__forceinline float FPMax( float fValA, float fValB )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel( (fValA-fValB), fValA, fValB );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return ( fValA > fValB ? fValA : fValB );
#endif
	}

	__forceinline float FPClamp( float fVal, float fLow, float fHigh )
	{
		return FPMin( FPMax(fVal, fLow), fHigh );
	}

	__forceinline float FPSign( float fVal )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fsel(fVal, (float)__fsel(-fVal, 0.0f, 1.0f), -1.0f);
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return (fVal < 0.0f) ? -1.0f : (fVal == 0.0f) ? 0.0f : 1.0f;
#endif		
	}

	__forceinline float FPSin( float angle )
	{
#if 0//(__XENON || __PPU) && USE_INTRINSICS
		
		return FPSin_Imp( angle ); // This is not as fast as sinf(), at least on Xenon. Why was this used in amath.h?

//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return sinf( angle );
#endif
	}
	
	__forceinline float FPCos( float angle )
	{
#if 0//(__XENON || __PPU) && USE_INTRINSICS
		return FPSin( (MY_PI * 0.5f)-angle ); // This is not as fast as cosf(), at least on Xenon. Why was this used in amath.h?
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return cosf( angle );
#endif
	}

	__forceinline float FPTan( float angle )
	{
#if 0//(__XENON || __PPU) && USE_INTRINSICS
		return FPSin( angle )/FPCos( angle ); // This is not as fast as tanf(), at least on Xenon. Why was this used in amath.h?
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return tanf( angle );
#endif
	}

	__forceinline void FPSinAndCos( float& sin, float& cos, float angle )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		FPSinAndCos_Imp( sin, cos, angle );
//#elif __SPU && USE_INTRINSICS
#elif RSG_CPU_X86 && USE_INTRINSICS && !RSG_MANAGED
		float tempSin, tempCos;
		__asm {
			fld			dword ptr [angle]
			fsincos
			fstp		dword ptr [tempCos]	// pop cosine
			fstp		dword ptr [tempSin]	// pop sine
		}
		sin = tempSin;
		cos = tempCos;
#else
		sin = sinf( angle );
		cos = cosf( angle );
#endif
	}

	__forceinline float FPASin( float sin )
	{
#if 0//(__XENON || __PPU) && USE_INTRINSICS
		return FPATan2( sin, FPSqrt( 1.0f-sin*sin ) ); // This is not as fast as asinf(), at least on Xenon. Why was this used in amath.h?
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return asinf( sin );
#endif
	}

	__forceinline float FPACos( float cos )
	{
		mthAssertf( cos >= -1.0f && cos <= 1.0f, "cos value %f out of range [-1.0, 1.0]", cos );

#if 0//(__XENON || __PPU) && USE_INTRINSICS
		return FPATan2( FPSqrt( 1.0f-cos*cos ), cos ); // This is not as fast as acosf(), at least on Xenon. Why was this used in amath.h?
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return acosf( cos );
#endif
	}

	__forceinline float FPATan( float tan )
	{
		return atanf( tan );
	}

	__forceinline float FPATan2( float y, float x )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return FPATan2_Imp( y, x );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return atan2f( y, x );
#endif
	}

	__forceinline float FPFloor( float fVal )
	{
		return floorf( fVal );
	}

	__forceinline float FPCeil( float fVal )
	{
		return ceilf( fVal );
	}

	__forceinline float FPAbs( float fVal )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return (float)__fabs( fVal );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return fabsf( fVal ); //return (fVal < 0.0f ? -fVal : fVal);
#endif
	}

	__forceinline float FPInvertFast( float fVal )
	{
		mthAssertf( fVal != 0.0f , "Divide by zero!" );

#if (__XENON || __PPU) && USE_INTRINSICS
#if __PPU && defined(__SNC__)
		return (float) __builtin_fre(fVal);
#else
		return __fres( fVal ); // Accurate to only a couple of decimal places.
#endif
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		// Revert to non-fast version if fast isn't available.
		return 1.0f/fVal;
#endif
	}

	__forceinline float FPInvertFastSafe( float fVal, float errVal )
	{
		return FPIfNeqZeroThenElse( fVal, FPInvertFast(fVal), errVal );
	}

	__forceinline float FPSqrt( float fVal )
	{
		// mthAssert disabled for the scalar fallback, since vectorized code often does sqrt()'s of invalid inputs,
		// but uses a SelectFT() to mask out the bad results later.
#if UNIQUE_VECTORIZED_TYPE
		mthAssertf( fVal >= 0.0f , "fVal %f should be >= zero!", fVal);
#endif

#if (__XENON || __PPU) && USE_INTRINSICS
		return __fsqrts( fVal );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
		// (This is slower...)
		//float tempSqrt;
		//__asm {
		//	fld			dword ptr [fVal]
		//	fsqrt
		//	fstp		dword ptr [tempSqrt]	// pop sqrt
		//}
		//return tempSqrt;
#else
		return sqrtf( fVal );
#endif
	}

	__forceinline float FPSqrtSafe( float fVal, float errVal )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		return FPIfGteZeroThenElse( fVal, __fsqrts(fVal), errVal );
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return FPIfGteZeroThenElse( fVal, sqrtf(fVal), errVal );
#endif
	}

	__forceinline float FPInvSqrt( float fVal )
	{
#if (__XENON || __PPU) && USE_INTRINSICS
		// Two Newton-Raphson iterations here.
#if __PPU && defined(__SNC__)
		float r = __builtin_frsqrte(fVal);
#else
		float r  = (float)__frsqrte(fVal);
#endif
		float rr = r*r;
		float r2 = r*0.5f;
		float nms = 1.0f - fVal*rr;
		r = r + nms*r2;

		rr = r*r;
		r2 = r*0.5f;
		nms = 1.0f - fVal*rr;
		return r + nms*r2;
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
#else
		return 1.0f/FPSqrt(fVal);
#endif
	}

	__forceinline float FPInvSqrtSafe( float fVal, float errVal )
	{
		//return (fVal>0.0f) ? FPInvSqrt(fVal) : errVal;
		return FPIfGtZeroThenElse( fVal, FPInvSqrt(fVal), errVal );
	}

	__forceinline float FPInvSqrtFast( float fVal )
	{
		mthAssertf( fVal > 0.0f , "fVal %f should be > zero!", fVal );

#if (__XENON || __PPU) && USE_INTRINSICS
		// Only one Newton-Raphson iteration here.
#if __PPU && defined(__SNC__)
		float r = __builtin_frsqrte(fVal);
#else
		float r  = (float)__frsqrte(fVal);
#endif
		float rr = r*r;
		float r2 = r*0.5f;
		float nms = 1.0f - fVal*rr;
		return r + nms*r2;
//#elif __SPU && USE_INTRINSICS
//#elif __WIN32PC && USE_INTRINSICS
		// TODO: Use this:
		// http://www.stereopsis.com/computermath101/
#else
		return FPInvertFast( FPSqrt(fVal) );
#endif
	}

	__forceinline float FPInvSqrtFastSafe( float fVal, float errVal )
	{
		//return (fVal>0.0f) ? FPInvSqrtFast(fVal) : errVal;
		return FPIfGtZeroThenElse( fVal, FPInvSqrtFast(fVal), errVal );
	}

	__forceinline float FPSlowIn( float t )
	{
		// output = 1.0 - cos(0.5*t*PI) = 1.0 - cos(t*(PI/2))
		float fClampedInput = FPClamp( t, 0.0f, 1.0f );
		return (1.0f - FPCos(fClampedInput*1.57079632679489661923132169163975145f));
	}

	__forceinline float FPSlowOut( float t )
	{
		// output = sin(0.5*t*PI) = sin(t*(PI/2))
		float fClampedInput = FPClamp( t, 0.0f, 1.0f );
		return FPSin(fClampedInput*1.57079632679489661923132169163975145f);
	}

	__forceinline float FPSlowInOut( float t )
	{
		// output = 0.5 * (1.0 - cos(t*PI)) = 0.5 - 0.5*cos(t*PI)
		float fClampedInput = FPClamp( t, 0.0f, 1.0f );
		return (0.5f - 0.5f*FPCos(fClampedInput*((float)MY_PI)));
	}

	__forceinline float FPBellInOut( float t )
	{
		float fClampedInput = FPClamp( t, 0.0f, 1.0f );
		// output = 0.5( 1.0 - cos(t*(2*PI)) ) = 0.5 - 0.5*cos(t*(2*PI))
		return (0.5f - 0.5f*FPCos(fClampedInput*((float)MY_2_PI)));
	}

	__forceinline float FPLerp( float t, float a, float b )
	{
		mthAssertf( t >= 0.0f && t <= 1.0f , "Warning: Potentially incorrect usage! t value %f is not in between 0.0f and 1.0f!", t );

		return ( a + (b-a)*t );
	}

	__forceinline float FPRamp( float x, float funcInA, float funcInB, float funcOutA, float funcOutB )
	{
		float t = FPRangeClamp( x, funcInA, funcInB );
		return FPLerp( t, funcOutA, funcOutB );
	}

	__forceinline float FPRampFast( float x, float funcInA, float funcInB, float funcOutA, float funcOutB )
	{
		float t = FPRangeClampFast( x, funcInA, funcInB );
		return FPLerp( t, funcOutA, funcOutB );
	}

	__forceinline float FPRange( float t, float fLower, float fUpper )
	{
		mthAssertf( fLower != fUpper , "fLower should not equal fUpper!" );

		// This assert is non-fatal. Since this func is called from inside FPRangeClamp(), this assert goes off too often.
		//mthAssertf( t >= fLower && t <= fUpper, "Warning: Potentially incorrect usage! Maybe use FPRangeClamp()!" );

		// output = (t-lower) / (upper-lower)
		return (t-fLower) / (fUpper-fLower);
	}

	__forceinline float FPRangeSafe( float t, float fLower, float fUpper, float fErrorVal )
	{
		mthAssertf( fLower != fUpper , "fLower should not equal fUpper!" );
		mthAssertf( t >= fLower && t <= fUpper , "Warning: Potentially incorrect usage! Maybe use FPRangeClamp()!" );

		// output = (t-lower) / (upper-lower)
		float fUpperMinusLower = fUpper-fLower;
		return (t-fLower) / FPIfNeqZeroThenElse(fUpperMinusLower, fUpperMinusLower, fErrorVal);
	}

	__forceinline float FPRangeFast( float t, float fLower, float fUpper )
	{
		mthAssertf( fLower != fUpper , "fLower should not equal fUpper!" );
		mthAssertf( t >= fLower && t <= fUpper , "Warning: Potentially incorrect usage! Maybe use FPRangeClamp()!" );

		// output = (t-lower) / (upper-lower)
		return (t-fLower) * FPInvertFast(fUpper-fLower);
	}

	__forceinline float FPRangeFastSafe( float t, float fLower, float fUpper, float fErrorVal )
	{
		mthAssertf( fLower != fUpper , "fLower should not equal fUpper!" );
		mthAssertf( t >= fLower && t <= fUpper , "Warning: Potentially incorrect usage! Maybe use FPRangeClamp()!" );

		// output = (t-lower) / (upper-lower)
		float fUpperMinusLower = fUpper-fLower;
		return (t-fLower) * FPInvertFast(FPIfNeqZeroThenElse(fUpperMinusLower, fUpperMinusLower, fErrorVal));
	}

	__forceinline float FPRangeClamp( float t, float fLower, float fUpper )
	{
		mthAssertf( fLower != fUpper , "fLower should not equal fUpper!" );

		// output = clamp[ (t-lower) / (upper-lower), 0, 1 ]
		return FPClamp( FPRange(t,fLower,fUpper), 0.0f, 1.0f );
	}
	
	__forceinline float FPRangeClampFast( float t, float fLower, float fUpper )
	{
		mthAssertf( fLower != fUpper , "fLower should not equal fUpper!" );

		// output = clamp[ (t-lower) / (upper-lower), 0, 1 ]
		return FPClamp( FPRangeFast(t,fLower,fUpper), 0.0f, 1.0f );
	}

	__forceinline float FPLog2( float a )
	{
		mthAssertf( a >= 0.0f , "Invalid input!" );

		// log2(f) = ln(f)/ln(2)
		return log(a)*1.4426950408889634073604078114048f;
	}

	__forceinline float FPLog10( float a )
	{
		mthAssertf( a >= 0.0f , "Invalid input!" );

		return log10(a);
	}

	__forceinline float FPPow( float x, float y )
	{
		return powf( x, y );
	}

	__forceinline float FPExpt( float a )
	{
		return powf( 2.0f, a );
	}

	


} // namespace rage


