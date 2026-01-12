#ifndef VECTORMATH_MATHOPS_H
#define VECTORMATH_MATHOPS_H

#if __XENON
#include <ppcintrinsics.h>
#elif __PPU
#include <ppu_intrinsics.h>
#elif __SPU
#include <spu_intrinsics.h>
#endif

#include "math/constants.h"
#include <math.h>
#include "channel.h"
#include "vectorconfig.h"

// Included here are scalar float operations that are either:
//
// (1) Not implemented in amath.h/intrinsics.h/simplemath.h
// (2) Faster than those in amath.h/intrinsics.h/simplemath.h
// (3) Implemented in a more consistent or portable format, than those in amath.h/intrinsics.h/simplemath.h

namespace rage
{

	// Returns from 0 to 32.
	u32 CountLeadingZeros(u32 a);
	u32 CountTrailingZeros(u32 a);

	// Rotate bits
	u32 RotateLeft(u32 a);
	u32 RotateRight(u32 a);

	// PURPOSE: Convert a floating point number into a fixed point number
	// PARAMS:
	//	value - The floating point value to be converted
	//	size - the number of bits of precision to shift
	//	shift - Number of bits the return value should be shifted by
	// RETURN: The fixed point number specified by the number of bits in size and the floating point number in value, shifted left by the number of bits in shift
	// NOTES: This operation is simply multiplying the float by a constant value and discarding the extra bits
	//			fixedPoint = value * (1 << size);
	//			The return value is shifted by the amount of bits in the shift parameter
	u32 FPPack(float value,u32 size,u32 shift);

	/////////////////////////////
	// USES INTEGER INSTRUCTIONS ONLY, good for loading a float (as an int) from memory, and doing a cheap int compare.
	// FROM MEMORY ONLY, or else there will be a LHS!
	// Note that for speed reasons, these don't check for invalid float inputs like INF or NAN.
	/////////////////////////////
	bool FloatSameSignAsFloat( const float* floatInMemory1, const float* floatInMemory2 );
	bool FloatEqualsFloatAsInt( const float* floatInMemory1, u32 float2AsInt );
	bool FloatEqualsFloat( const float* floatInMemory1, const float* floatInMemory2 );
	bool FloatNeqFloat( const float* floatInMemory1, const float* floatInMemory2 );
	bool FloatEqualsFloatSafe( const float* floatInMemory1, const float* floatInMemory2 ); // Allows (-0.0f == 0.0f)
	bool FloatNeqFloatSafe( const float* floatInMemory1, const float* floatInMemory2 ); // Allows (-0.0f == 0.0f)
	bool FloatEqualsZero( const float* floatInMemory );
	bool FloatNeqZero( const float* floatInMemory );
	bool FloatGreaterThanZero( const float* floatInMemory );
	bool FloatLessThanZero( const float* floatInMemory );
	bool FloatGreaterThanOrEqualZero( const float* floatInMemory );
	bool FloatLessThanOrEqualZero( const float* floatInMemory );

	// IMPORTANT NOTE: CONDITIONAL BRANCHES ARE ALMOST AS FAST AS THE BELOW SIX FUNCTIONS!
	// IF YOU NEED TO CHOOSE BETWEEN TWO VALUES, THEN USE THESE SIX. BUT IF YOU ARE PERFORMING
	// TWO COMPLEX CALCULATIONS, AND THEN CHOOSING THE RESULT WITH THESE SIX TO AVOID BRANCHING... DON'T DO THAT!
	// BECNHMARK TO SEE WHICH IS BETTER ON THE AVERAGE INPUT CASE: THESE OR TERNARY OP.

	// These two are fastest on PPC (one fsel).
	float FPIfGteZeroThenElse( float fVal, float selectA, float selectB );
	float FPIfLtZeroThenElse( float fVal, float selectA, float selectB );	

	// These four are 2nd-fastest on PPC (one fsub or fneg, one fsel).
	float FPIfGtZeroThenElse( float fVal, float selectA, float selectB );
	float FPIfLteZeroThenElse( float fVal, float selectA, float selectB );
	float FPIfGteThenElse( float opA, float opB, float selectA, float selectB );
	float FPIfLteThenElse( float opA, float opB, float selectA, float selectB );

	// These two are 3rd-fastest on PPC (one fneg, two fsel).
	float FPIfEqZeroThenElse( float fVal, float selectA, float selectB );
	float FPIfNeqZeroThenElse( float fVal, float selectA, float selectB );

	// These four are 4th-fastest on PPC (two fsub, two fsel).
	float FPIfEqThenElse( float opA, float opB, float selectA, float selectB );
	float FPIfNeqThenElse( float opA, float opB, float selectA, float selectB );
	float FPIfGtThenElse( float opA, float opB, float selectA, float selectB );
	float FPIfLtThenElse( float opA, float opB, float selectA, float selectB );

	float FPMin( float fValA, float fValB );
	float FPMax( float fValA, float fValB );
	float FPClamp( float fVal, float fLow, float fHigh );

	float FPSign( float fVal );
	float FPSin( float angle );
	float FPCos( float angle );
	float FPTan( float angle );
	void FPSinAndCos( float& sin, float& cos, float angle );

	float FPASin( float sin );
	float FPACos( float cos );
	float FPATan( float tan );
	float FPATan2( float y, float x );

	// TODO: Safe trig functions (clamp output to min/max values, for out-of-range inputs.)
	// ...
	// FPSinSafe()
	// FPCosSafe()
	// FPTanSafe()
	// FPSinAndCosSafe()
	// FPASinSafe()
	// FPACosSafe()
	// FPATanSafe()
	// FPATan2Safe()
	
	float FPFloor( float fVal );
	float FPCeil( float fVal );

	float FPAbs( float fVal );
	float FPSqrt( float fVal );
	float FPSqrtSafe( float fVal, float errVal = 0.0f );
	float FPInvSqrt( float fVal );
	float FPInvSqrtSafe( float fVal, float errVal = LARGE_FLOAT );
	float FPInvSqrtFast( float fVal );
	float FPInvSqrtFastSafe( float fVal, float errVal = LARGE_FLOAT );

	float FPInvertFast( float fVal );
	float FPInvertFastSafe( float fVal, float errVal = LARGE_FLOAT );

	// Note: This clamps the input t to [0.0f,1.0f]
	float FPSlowIn( float t );
	float FPSlowOut( float t );
	float FPSlowInOut( float t );
	float FPBellInOut( float t );
	// Note: This doesn't clamp the input t to [0.0f,1.0f], so that you may extrapolate in
	// addition to interpolate. ;)
	float FPLerp( float t, float a, float b );
	float FPRamp( float x, float funcInA, float funcInB, float funcOutA, float funcOutB );
	float FPRampFast( float x, float funcInA, float funcInB, float funcOutA, float funcOutB );
	float FPRange( float t, float fLower, float fUpper );
	float FPRangeSafe( float t, float fLower, float fUpper, float fErrorVal );
	float FPRangeFast( float t, float fLower, float fUpper );
	float FPRangeFastSafe( float t, float fLower, float fUpper, float fErrorVal );
	float FPRangeClamp( float t, float fLower, float fUpper );
	float FPRangeClampFast( float t, float fLower, float fUpper );

	float FPLog2( float a );
	float FPLog10( float a );
	float FPPow( float x, float y );
	float FPExpt( float a );

	// PURPOSE:	Determine whether the given floating point number has a finite value.
	// PARAMS:
	//	i -	the number to test
	// RETURN:	true if the given number has a finite value, false if it does not
	bool FPIsFinite(float i);

	
	//================================================
	// Implementations
	//================================================

#if (__XENON || __PPU) && USE_INTRINSICS
	float FPSin_Imp( float angle );
	void FPSinAndCos_Imp( float& sin, float& cos, float angle );
	float FPATan2_Imp( float y, float x );
#endif

} // namespace rage


// A few fast divide macros.
// High: Speed.
// Low: Accuracy and divide-by-zero safety (if one denominator is zero, they all might as well be zero).
// Note: You can do even faster by using FPInvert() multiple times (on platforms that accelerate it, i.e.
//		Xenon and PS3 PPU), but it is not nearly as accurate as this.
//
// A test (Xenon debug (don't try this exact test code in release mode, as constant-division will be optimized away at compile-time!)):
//
//	float f1=0.0f,f2=0.0f,f3=0.0f,f4=0.0f;
//	float a=1.0f,b=10.0f,c=1.0f,d=2.0f,e=1.0f,f=100.0f,g=1.0f,h=1000.0f;
//	Printf( "Normal division:      f1 = %.25f, f2 = %.25f, f3 = %.25f, f4 = %.25f\n", a/b,c/d,e/f,g/h );
//	FP_FAST_DIVIDE_4( f1,f2,f3,f4,a,b,c,d,e,f,g,h );
//	Printf( "Fast 4-fold division: f1 = %.25f, f2 = %.25f, f3 = %.25f, f4 = %.25f\n", f1,f2,f3,f4 );
//	FP_FAST_DIVIDE_3( f1,f2,f3,a,b,c,d,e,f );
//	Printf( "Fast 3-fold division: f1 = %.25f, f2 = %.25f, f3 = %.25f, f4 = %.25f\n", f1,f2,f3,f4 );
//	FP_FAST_DIVIDE_2( f1,f2,a,b,c,d );
//	Printf( "Fast 2-fold division: f1 = %.25f, f2 = %.25f, f3 = %.25f, f4 = %.25f\n", f1,f2,f3,f4 );
//
//	Normal division:      f1 = 0.1000000014901161200000000, f2 = 0.5000000000000000000000000, f3 = 0.0099999997764825821000000, f4 = 0.0010000000474974513000000
//	Fast 4-fold division: f1 = 0.0999999940395355220000000, f2 = 0.4999999701976776100000000, f3 = 0.0099999997764825821000000, f4 = 0.0009999999310821294800000
//	Fast 3-fold division: f1 = 0.1000000014901161200000000, f2 = 0.5000000596046447800000000, f3 = 0.0100000007078051570000000, f4 = 0.0009999999310821294800000
//	Fast 2-fold division: f1 = 0.1000000014901161200000000, f2 = 0.5000000000000000000000000, f3 = 0.0100000007078051570000000, f4 = 0.0009999999310821294800000
//
#define FP_FAST_DIVIDE_2(quotient1, quotient2, num_a, den_b, num_c, den_d)															\
						do {																										\
							float t0 = 1.0f/((den_b)*(den_d));																		\
							quotient1 = (num_a)*(t0)*(den_d);																		\
							quotient2 = (num_c)*(t0)*(den_b);																		\
						} while(0)
#define FP_FAST_DIVIDE_3(quotient1, quotient2, quotient3, num_a, den_b, num_c, den_d, num_e, den_f)									\
						do {																										\
							float t0 = 1.0f/((den_b)*(den_d)*(den_f));																\
							quotient1 = (num_a)*(t0)*(den_d)*(den_f);																\
							quotient2 = (num_c)*(t0)*(den_b)*(den_f);																\
							quotient3 = (num_e)*(t0)*(den_b)*(den_d);																\
						} while(0)
#define FP_FAST_DIVIDE_4(quotient1, quotient2, quotient3, quotient4, num_a, den_b, num_c, den_d, num_e, den_f, num_g, den_h)		\
						do {																										\
							float t0 = 1.0f/((den_b)*(den_d)*(den_f)*(den_h));														\
							quotient1 = (num_a)*(t0)*(den_d)*(den_f)*(den_h);														\
							quotient2 = (num_c)*(t0)*(den_b)*(den_f)*(den_h);														\
							quotient3 = (num_e)*(t0)*(den_b)*(den_d)*(den_h);														\
							quotient4 = (num_g)*(t0)*(den_b)*(den_d)*(den_f);														\
						} while(0)




#include "mathops.inl"

#endif // VECTORMATH_MATHOPS_H
