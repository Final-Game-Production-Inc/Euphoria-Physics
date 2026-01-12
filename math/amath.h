//
// math/amath.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef MATH_AMATH_H
#define MATH_AMATH_H

#include "channel.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#if __XENON
#include <ppcintrinsics.h>		// For __fself
#include <xdk.h>
#elif __PPU
#include <ppu_intrinsics.h>
#elif RSG_CPU_INTEL
#include "intrinsics.h"
#endif

#include "system/codecheck.h"
#include "vectormath/mathops.h"

 //prevent conflict with math defines in math.h which can get pulled in after this include
#if RSG_DURANGO
#define _MATH_DEFINES_DEFINED
#endif

/* PURPOSE
	amath provides basic math functions.

	<FLAG Component>
*/

// PURPOSE:	Stop people from using HUGE_VAL.
#undef HUGE_VAL
#define HUGE_VAL Please_use_FLT_MAX_instead	//lint !e755

// PURPOSE:	Define epsilon as zero on win32 platforms.
#ifndef AMATH_EPSILON
#if __WIN32
/* win32 platforms use libm functions, which do not have epsilons */
#define AMATH_EPSILON 0.0f
#endif
#endif

// PURPOSE:	Define the largest number that results in a zero square root.
#if __WIN32
#define SMALLEST_SQUARE 0.0f
#elif __PS3 || __PSP2
#define SMALLEST_SQUARE 1.17549e-38f
#endif


namespace rage {

// Even __forceinline is not sufficient to stop Microsoft's x64 (and possibly
// x86) compiler from storing vector arguments that are passed by value to a non
// 16-byte aligned address on the stack, then triggereing an exception because
// of using a movaps instruction.
#if defined(_MSC_VER) && RSG_CPU_INTEL
#	define AMATH_ARG(TYPE) const TYPE &
#else
#	define AMATH_ARG(TYPE) TYPE
#endif

// PURPOSE:	integer to float conversions
inline float ITOF0(s32 i) { return (float)(i); }
inline float ITOF4(s32 i) { return ((float)(i))*1.0f/16.0f; }
inline float ITOF12(s32 i) { return ((float)(i))*1.0f/4096.0f; }
inline float ITOF15(s32 i) { return ((float)(i))*1.0f/32768.0f; }
__forceinline float IntToFloatBitwise(u32 u) { union { float f; u32 u; } x; x.u = u; return x.f; }
__forceinline float LoadIntAsFloatBitwise(const u32& u) { 
#if __WIN32
	return *reinterpret_cast<const float*>(&u); 
#elif __GNUC__
	typedef float __attribute__((__may_alias__)) alias_float;
	return *reinterpret_cast<const alias_float*>(&u);
#else 
	return IntToFloatBitwise(u);
#endif
}

// PURPOSE:	float to integer conversions
inline s32 FTOI0(float i) { return (s32)(i); }
inline s32 FTOI4(float i) { return (s32)(i*16.0f); }
inline s32 FTOI12(float i) { return (s32)(i*4096.0f); }
inline s32 FTOI15(float i) { return (s32)(i*32768.0f); }
__forceinline u32 FloatToIntBitwise(float f) { union { float f; u32 u; } x; x.f = f; return x.u; }
__forceinline u32 LoadFloatAsIntBitwise(const float& f) { 
#if __WIN32
	return *reinterpret_cast<const u32*>(&f);
#elif __GNUC__
	typedef u32 __attribute__((__may_alias__)) alias_u32;
	return *reinterpret_cast<const alias_u32*>(&f);
#else 
	return FloatToIntBitwise(f);
#endif
}

// PURPOSE:	numerical constants
// NOTES:	more constants are defined in constants.h
#ifndef PI
#define PI			(3.14159265358979323846264338327950288f)
#endif

#ifndef M_SQRT2
#define M_SQRT2		(1.4142135623730950488016887242097f)
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2	(0.70710678118654752440084436210485f)
#endif

#ifndef M_SQRT3
#define M_SQRT3		(1.7320508075688772935274463415059f)
#endif

#ifndef M_E
#define	M_E			(2.71828182845904523536028747135266249f)
#endif

// PURPOSE:	conversions between radians and degrees
#define RtoD     (180.0f/PI)
#define RtoDx2   (360.0f/PI)
#define DtoR     (PI/180.0f)	//lint !e755
#define Dx2toR   (PI/360.0f)

// PURPOSE:	define floating point mod for non-win32 builds
#if !__WIN32
// inline float fmodf(float x, float y)	{ return y? x - ((int)(x/y) * y) : 0; }
#endif

// PURPOSE: Round up to this power of two
#define RAGE_ALIGN(x, power) ((x+((1<<power)-1))&~((1<<power)-1))

// PURPOSE: Divide and round up to this power of two
#define RAGE_COUNT(x, power) ((x+((1<<power)-1))>>power)

#if !__FINAL
extern bool IsPrime(size_t value);
extern size_t GetNextPrime(size_t value);
#endif

// PURPOSE: Select between two floating point values based on floating point comparison
// PARAMS:
//	condition - The condition to be tested against zero
//	valGE - The value returned if condition is >= 0
//	valLT - The value returned if condition is < 0
// RETURN: The value determined by (condition >= 0)
// NOTES: If condition is NAN, valLT is returned
__forceinline float Selectf(float condition, float valGE, float valLT)
{
#if __XENON
#	if _XDK_VER >= 6995
	return __fself(condition, valGE, valLT);
#	else
	return (float)__fsel(condition, valGE, valLT);
#	endif
#elif __PPU
	return (float)__fsel(condition, valGE, valLT);
#else // use this for SPUs because the compiler turns this into a selb for us.
	return (condition >= 0.0f ? valGE : valLT);
#endif
}

__forceinline double Selectf(double condition, double valGE, double valLT)
{
#if __XENON || __PPU
	return __fsel(condition, valGE, valLT);
#else // use this for SPUs because the compiler turns this into a selb for us.
	return (condition >= 0.0f ? valGE : valLT);
#endif
}

// PURPOSE: Selectf helpers, to do comparisons with the same syntax as the vectormath library (SelectFT(IsLessThan(...), ...))
struct SelectfHelper { 
	SelectfHelper(float v) : val(v) {}
	float val;
};
struct SelectfHelperInv { 
	SelectfHelperInv(float v) : val(v) {}
	float val;
};
__forceinline SelectfHelper IsLessThan(float a, float b) { return SelectfHelper(a - b); }
__forceinline SelectfHelper IsGreaterThan(float a, float b) { return SelectfHelper(b - a); }
__forceinline SelectfHelperInv IsLessThanOrEqual(float a, float b) { return SelectfHelperInv(b - a); }
__forceinline SelectfHelperInv IsGreaterThanOrEqual(float a, float b) { return SelectfHelperInv(a - b); }

// PURPOSE: Used like the vectormath SelectFT functions, but the first argument needs to be a call to one of the comparison
// functions IsLessThan, IsGreaterThan, etc.
// PARAMS:
//		helper - A call to IsLessThan IsGreaterThan, IsLessThanOrEqual, or IsGreaterThanOrEqual
//		ifFalse - value to return if the comparison was false
//		ifTrue - value to return if the comparison was true
__forceinline float SelectFT(SelectfHelper helper, float ifFalse, float ifTrue) { return Selectf(helper.val, ifFalse, ifTrue); }
__forceinline float SelectFT(SelectfHelperInv helper, float ifFalse, float ifTrue) { return Selectf(helper.val, ifTrue, ifFalse); }

// PURPOSE: Returns the sign of the argument
// PARAMS:
//  x - the number to return the sign of
// RETURN: -1 if x is negative, 0 if it equals 0, 1 otherwise
template <typename Type> inline Type Sign(AMATH_ARG(Type) x)
{
	return (x < 0) ? -1 : (x == 0) ? 0 : 1;
}

#if __XENON || __PPU
template <> inline double Sign(double x)
{ 
	return Selectf(x, Selectf(-x, 0., 1.), -1.);
}
template <> inline float Sign(float x)
{ 
	return Selectf(x, Selectf(-x, 0.f, 1.f), -1.f);
}
#else
template <> inline double Sign(AMATH_ARG(double) x)
{ 
	return (x < 0.) ? -1. : (x == 0.) ? 0. : 1.;
}
template <> inline float Sign(AMATH_ARG(float) x)
{ 
	return (x < 0.f) ? -1.f : (x == 0.f) ? 0.f : 1.f;
}
#endif

// PURPOSE:	Replacement basic trig functions for PSN
// PARAMS:
//	f -	angle in radians
// RETURN: Sin/Cos/Tan of angle
#if __PPU 
	float Sinf(float f);
	static inline float Cosf(float f) { return rage::Sinf((PI * 0.5f)-f); }
	static inline float Tanf(float f) { return rage::Sinf(f)/rage::Cosf(f); }
	float Atan2f( float y,float x );

#else
inline float Sinf(float f) {return sinf(f);}
inline float Cosf(float f) {return cosf(f);}
inline float Tanf(float f) {return tanf(f);}
inline float Atan2f( float y,float x ) { return atan2f( y,x ); }

#endif

#if __SPU
inline float Sinhf(float x) { return (expf(x) - expf(-x)) * 0.5f; }
#else
inline float Sinhf(float f) { return sinhf(f); }
#endif

// PURPOSE: Returns true if a < b, using the integer pipeline instead of a floating point comparison.
// NOTE: This is only really useful for floats in memory. If they are in registers, this will cause a load-hit-store.
__forceinline bool PositiveFloatLessThan(const float& a, const float& b)
{
	u32 ia = LoadFloatAsIntBitwise(a);
	u32 ib = LoadFloatAsIntBitwise(b);
	// Make sure sign bit is clear on both a and b, and that they aren't Inf or NaN
	FastAssert(((ia & 0x80000000) == 0) && ((ia & 0x7F800000) != 0x7F800000));
	FastAssert(((ib & 0x80000000) == 0) && ((ib & 0x7F800000) != 0x7F800000));
	return ia < ib;
}

// PURPOSE: Returns true if a <= b, using the integer pipeline instead of a floating point comparison.
// NOTE: This is only really useful for floats in memory. If they are in registers, this will cause a load-hit-store.
__forceinline bool PositiveFloatLessThanOrEqual(const float& a, const float& b)
{
	u32 ia = LoadFloatAsIntBitwise(a);
	u32 ib = LoadFloatAsIntBitwise(b);
	// Make sure sign bit is clear on both a and b, and that they aren't Inf or NaN
	FastAssert(((ia & 0x80000000) == 0) && ((ia & 0x7F800000) != 0x7F800000));
	FastAssert(((ib & 0x80000000) == 0) && ((ib & 0x7F800000) != 0x7F800000));
	return ia <= ib;
}

// PURPOSE: Returns true if a < b, using the integer pipeline instead of a floating point comparison.
// NOTE: This is only really useful for floats in memory. If they are in registers, this will cause a load-hit-store.
// Doesn't fast assert out in the case of NaN arguments.
__forceinline static bool PositiveFloatLessThan_Unsafe(const float& a, const float& b)
{
	u32 ia = LoadFloatAsIntBitwise(a);
	u32 ib = LoadFloatAsIntBitwise(b);
	// Make sure sign bit is clear on both a and b, and that they aren't Inf or NaN
	Assert(((ia & 0x80000000) == 0) && ((ia & 0x7F800000) != 0x7F800000));
	Assert(((ib & 0x80000000) == 0) && ((ib & 0x7F800000) != 0x7F800000));
	return ia < ib;
}

// PURPOSE: Returns true if a <= b, using the integer pipeline instead of a floating point comparison.
// NOTE: This is only really useful for floats in memory. If they are in registers, this will cause a load-hit-store.
// Doesn't fast assert out in the case of NaN arguments.
__forceinline static bool PositiveFloatLessThanOrEqual_Unsafe(const float& a, const float& b)
{
	u32 ia = LoadFloatAsIntBitwise(a);
	u32 ib = LoadFloatAsIntBitwise(b);
	// Make sure sign bit is clear on both a and b, and that they aren't Inf or NaN
	Assert(((ia & 0x80000000) == 0) && ((ia & 0x7F800000) != 0x7F800000));
	Assert(((ib & 0x80000000) == 0) && ((ib & 0x7F800000) != 0x7F800000));
	return ia <= ib;
}


// PURPOSE:	Clamp a number of any type between two other number of the same type.
// PARAMS:
//	t -	the number to clamp
//	a -	the minimum value
//	b -	the maximum value
// RETURN: The number between the given minimum and maximum that is closest to the given number.
template <class Type> inline FASTRETURNCHECK(Type) Clamp(AMATH_ARG(Type) t,AMATH_ARG(Type) a,AMATH_ARG(Type) b)
{
	return (t < a) ? a : (t > b) ? b : t;
}

#if __XENON || __PPU

#define Fpmax(a,b) Selectf((a)-(b), a,b)
#define Fpmin(a,b) Selectf((a)-(b), b,a)

template <> inline FASTRETURNCHECK(float) Clamp<float>(float t,float a,float b)
{
	return Fpmin(Fpmax(t, a), b);
}

template <> inline FASTRETURNCHECK(double) Clamp<double>(double t,double a,double b)
{
	return Fpmin(Fpmax(t, a), b);
}

#endif

// PURPOSE:	Find the absolute value of the given number.
// PARAMS:
//	a -	the number of which to get the absolute value
// RETURN: The given number if it is non-negative, and the number negated if it is negative.
template <class Type> inline FASTRETURNCHECK(Type) Abs(AMATH_ARG(Type) a)
{
	return a<0?-a:a;
}

#if __XENON || __PPU
template <> inline FASTRETURNCHECK(float) Abs<float>(float a)
{
	return __fabs(a);
}

template <> inline FASTRETURNCHECK(double) Abs<double>(double a)
{
	return __fabs(a);
}
#endif
// PURPOSE:	Find the lowest number of the two given numbers.
// PARAMS:
//	a -	the first number
//	b -	the second number
// RETURN:	The lowest number of the given numbers.
// NOTES:	If multiple numbers have the same value, the later number in the input list is returned.
template <class Type> __forceinline FASTRETURNCHECK(Type) Min(AMATH_ARG(Type) a,AMATH_ARG(Type) b)
{
	return a<b?a:b;
}
#if __XENON || __PPU
template <> __forceinline FASTRETURNCHECK(float) Min<float>(float a, float b)
{
	return (float)Fpmin(a,b);
}

template <> __forceinline FASTRETURNCHECK(double) Min<double>(double a, double b)
{
	return Fpmin(a,b);
}
#endif

// PURPOSE:	Find the lowest number of the three given numbers.
// PARAMS:
//	a -	the first number
//	b -	the second number
//	c -	the third number
// RETURN:	The lowest number of the given numbers.
// NOTES:	If multiple numbers have the same value, the later number in the input list is returned.
template <class Type> __forceinline FASTRETURNCHECK(Type) Min(AMATH_ARG(Type) a,AMATH_ARG(Type) b,AMATH_ARG(Type) c)
{
	return Min(Min(a,b),c);
}

#if __XENON || __PPU
template <> __forceinline FASTRETURNCHECK(float) Min<float>(float a, float b, float c)
{
	return (float)Fpmin(Fpmin(a,b),c);
}

template <> __forceinline FASTRETURNCHECK(double) Min<double>(double a, double b, double c)
{
	return Fpmin(Fpmin(a,b),c);
}
#endif

// PURPOSE:	Find the lowest number of the four given numbers.
// PARAMS:
//	a -	the first number
//	b -	the second number
//	c -	the third number
//	d -	the fourth number
// RETURN:	The lowest number of the given numbers.
// NOTES:	If multiple numbers have the same value, the later number in the input list is returned.
template <class Type> __forceinline FASTRETURNCHECK(Type) Min(AMATH_ARG(Type) a,AMATH_ARG(Type) b,AMATH_ARG(Type) c,AMATH_ARG(Type) d)
{
	return Min(Min(a,b),Min(c,d));
}

#if __XENON || __PPU
template <> __forceinline FASTRETURNCHECK(float) Min<float>(float a, float b, float c, float d)
{
	return (float)Fpmin(Fpmin(a,b),Fpmin(c,d));
}

template <> __forceinline FASTRETURNCHECK(double) Min<double>(double a, double b, double c, double d)
{
	return Fpmin(Fpmin(a,b),Fpmin(c,d));
}
#endif

// PURPOSE:	Find the largest number of the two given numbers.
// PARAMS:
//	a -	the first number
//	b -	the second number
// RETURN:	The largest number of the given numbers.
// NOTES:	If multiple numbers have the same value, the later number in the input list is returned.
template <class Type> __forceinline FASTRETURNCHECK(Type) Max(AMATH_ARG(Type) a,AMATH_ARG(Type) b)
{
	return a>b?a:b;
}

#if __XENON || __PPU
template <> __forceinline FASTRETURNCHECK(float) Max<float>(float a, float b)
{
	return (float)Fpmax(a,b);
}

template <> __forceinline FASTRETURNCHECK(double) Max<double>(double a, double b)
{
	return Fpmax(a,b);
}
#endif

// PURPOSE:	Find the lowest number of the two given numbers, with a strong hint
//          to compile as branchless code.
// PARAMS:
//	a -	the first number
//	b -	the second number
// RETURN:	The lowest number of the given numbers.
// NOTES:	This is NOT necessarily faster than regular Min.  Generally only for use when
//          mixing with a lot of longer latency instructions, as it helps compiler scheduling.
//          If unsure, use Min().
//          If multiple numbers have the same value, the later number in the input list is returned.
template <class Type> __forceinline FASTRETURNCHECK(Type) BranchlessMin(AMATH_ARG(Type) a,AMATH_ARG(Type) b)
{
	return Min(a,b);
}

// PURPOSE:	Find the largest number of the two given numbers, with a strong hint
//          to compile as branchless code.
// PARAMS:
//	a -	the first number
//	b -	the second number
// RETURN:	The largest number of the given numbers.
// NOTES:	This is NOT necessarily faster than regular Max.  Generally only for use when
//          mixing with a lot of longer latency instructions, as it helps compiler scheduling.
//          If unsure, use Max().
//          If multiple numbers have the same value, the later number in the input list is returned.
template <class Type> __forceinline FASTRETURNCHECK(Type) BranchlessMax(AMATH_ARG(Type) a,AMATH_ARG(Type) b)
{
	return Max(a,b);
}

#if __PPU || __XENON

// For integer types smaller than 64-bits, the following specializations work.
// This does not work with 64-bit integers as overflow is not correctly handled.
#define BRANCHLESS_SMALL_INTEGER_MIN_MAX(TYPE)                                 \
                                                                               \
template <> __forceinline FASTRETURNCHECK(TYPE) BranchlessMin<TYPE>(TYPE a, TYPE b) \
{                                                                              \
	u64 a64 = a; /* note intentional sign extension here */                    \
	u64 b64 = b;                                                               \
	u64 aSubB = a64 - b64;                                                     \
	u64 aLtB = (s64)aSubB>>63;                                                 \
	return (TYPE)(b64 + (aLtB & aSubB));                                       \
}                                                                              \
                                                                               \
template <> __forceinline FASTRETURNCHECK(TYPE) BranchlessMax<TYPE>(TYPE a, TYPE b) \
{                                                                              \
	u64 a64 = a;                                                               \
	u64 b64 = b;                                                               \
	u64 aSubB = a64 - b64;                                                     \
	u64 aLtB = (s64)aSubB>>63;                                                 \
	return (TYPE)(a64 - (aLtB & aSubB));                                       \
}

BRANCHLESS_SMALL_INTEGER_MIN_MAX(u8)
BRANCHLESS_SMALL_INTEGER_MIN_MAX(u16)
BRANCHLESS_SMALL_INTEGER_MIN_MAX(u32)
BRANCHLESS_SMALL_INTEGER_MIN_MAX(s8)
BRANCHLESS_SMALL_INTEGER_MIN_MAX(s16)
BRANCHLESS_SMALL_INTEGER_MIN_MAX(s32)

#undef BRANCHLESS_SMALL_INTEGER_MIN_MAX

// We should have been able to write branchless 64-bit functions in the
// following manner, but both SNC and CL suck.  SNC has intrinsics, but they
// don't actually generate the code we want, and it ends up slower code that
// still branches.  CL doesn't have intrinsics at all.  This only works with
// GCC.

// template <> __forceinline FASTRETURNCHECK(u64) BranchlessMin<u64>(u64 a, u64 b)
// {
// 	u64 aSubB = __subfc(b, a);
// 	u64 aLtB = __subfe(a, a);
// 	return b + (aLtB & aSubB);
// }

// template <> __forceinline FASTRETURNCHECK(u64) BranchlessMax<u64>(u64 a, u64 b)
// {
// 	u64 aSubB = __subfc(b, a);
// 	u64 aLtB = __subfe(a, a);
// 	return a - (aLtB & aSubB);
// }

// template <> __forceinline FASTRETURNCHECK(s64) BranchlessMin<s64>(s64 a, s64 b)
// {
//  u64 a_ = a ^ 0x8000000000000000uLL;
//  u64 b_ = b ^ 0x8000000000000000uLL;
// 	u64 aSubB = __subfc(b_, a_);
// 	u64 aLtB = __subfe(a, a);
// 	return b + (aLtB & aSubB);
// }

// template <> __forceinline FASTRETURNCHECK(s64) BranchlessMax<s64>(s64 a, s64 b)
// {
//  u64 a_ = a ^ 0x8000000000000000uLL;
//  u64 b_ = b ^ 0x8000000000000000uLL;
// 	u64 aSubB = __subfc(b_, a_);
// 	u64 aLtB = __subfe(a, a);
// 	return a - (aLtB & aSubB);
// }

#endif // __PPU || __XENON

// PURPOSE:	Find the largest number of the three given numbers.
// PARAMS:
//	a -	the first number
//	b -	the second number
//	c -	the third number
// RETURN:	The largest number of the given numbers.
// NOTES:	If multiple numbers have the same value, the later number in the input list is returned.
template <class Type> __forceinline Type Max(AMATH_ARG(Type) a,AMATH_ARG(Type) b,AMATH_ARG(Type) c)
{
	return Max(Max(a,b),c);
}

#if __XENON || __PPU
template <> __forceinline float Max<float>(float a, float b, float c)
{
	return (float)Fpmax(Fpmax(a,b),c);
}

template <> __forceinline double Max<double>(double a, double b, double c)
{
	return Fpmax(Fpmax(a,b),c);
}
#endif

// PURPOSE:	Find the largest number of the three given numbers.
// PARAMS:
//	a -	the first number
//	b -	the second number
//	c -	the third number
//	d -	the fourth number
// RETURN:	The largest number of the given numbers.
// NOTES:	If multiple numbers have the same value, the later number in the input list is returned.
template <class Type> __forceinline Type Max(AMATH_ARG(Type) a,AMATH_ARG(Type) b,AMATH_ARG(Type) c,AMATH_ARG(Type) d)
{
	return Max(Max(a,b),Max(c,d));
}

#if __XENON || __PPU
template <> __forceinline float Max<float>(float a, float b, float c, float d)
{
	return (float)Fpmax(Fpmax(a,b),Fpmax(c,d));
}

template <> __forceinline double Max<double>(double a, double b, double c, double d)
{
	return Fpmax(Fpmax(a,b),Fpmax(c,d));
}
#endif

// PURPOSE:	Find the number that is the given fraction of the distance between the two given numbers.
// PARAMS:
//	t -	the fraction of the distance from a to b
//	a -	the first number
//	b -	the second number
// RETURN:	the number that is the given fraction of the distance between the two given numbers
template <class Type> __forceinline Type Lerp(float t,AMATH_ARG(Type) a,AMATH_ARG(Type) b)
{
	return	(Type)(a+(b-a)*t);
}

// PURPOSE: Defines a linear function from {funcInA,funcOutA} to {funcInB,funcOutB}, clamped to the range {funcOutA,funcOutB}
// PARAMS:
//   funcInA - see return
//   funcInB - see return
//   funcOutA - see return
//   funcOutB - see return
// RETURN
//   Returns f(x), which is a piecewise linear function with three sections:
//     For x <= funcInA, f(x) = funcOutA.
//     for x > funcInA and x < funcInB, f(x) ramps from funcOutA to funcOutB
//     for x >= funcInB, f(x) = funcOutB
// NOTES: This function does not safely handle cases where (funcInB - funcInA) < SMALL_FLOAT, which will fail a FastAssert.
//   If this happens, call RampValueSafe instead. 
inline float RampValue (float x, float funcInA, float funcInB, float funcOutA, float funcOutB)
{
	const float funcInRange = funcInB - funcInA;
	// Call RampValueSafe() instead of RampValue() if this fails. 
	FastAssert(funcInRange >= SMALL_FLOAT);
	float t = Clamp((x - funcInA) / funcInRange,0.0f,1.0f);
	return Lerp(t,funcOutA,funcOutB);
}

// PURPOSE: Defines a linear function from {funcInA,funcOutA} to {funcInB,funcOutB}, clamped to the range {funcOutA,funcOutB},
//   safely handling cases where (funcInB - funcInA) < SMALL_FLOAT.
// PARAMS:
//   funcInA - see return
//   funcInB - see return
//   funcOutA - see return
//   funcOutB - see return
// RETURN
//   Returns f(x), which is a piecewise linear function with three sections:
//     For x <= funcInA, f(x) = funcOutA.
//     for x > funcInA and x < funcInB, f(x) ramps from funcOutA to funcOutB
//     for x >= funcInB, f(x) = funcOutB
inline float RampValueSafe (float x, float funcInA, float funcInB, float funcOutA, float funcOutB)
{
	float result;

	const float funcInRange = funcInB - funcInA;
	if(funcInRange >= SMALL_FLOAT)
	{
		float t = Clamp((x - funcInA) / funcInRange,0.0f,1.0f);
		result = Lerp(t,funcOutA,funcOutB);
	}
	else
	{
		result = (x >= funcInB) ? funcOutB : funcOutA;
	}

	return result;
}

// PURPOSE:	Swap the values of the two given numbers.
// PARAMS:
//	a -	reference to the first number
//	b -	reference to the second number
template <class Type> void SwapEm(Type &a,Type &b)
{
	Type c=a;
	a=b;
	b=c;
}

// PURPOSE:	Make sure a <= b, swapping them if necessary
// PARAMS:
//	a -	reference to the first number
//	b -	reference to the second number
template <class Type> void OrderEm(Type &a,Type &b)
{
	Type c=a;
	a=Min(a, b);
	b=Max(c, b);
}

// PURPOSE:	Find the smallest integer multiple of 4 that is equal to or greater than the given interger.
// PARAMS:
//	x -	the integer to fourtify
// RETURN:	the smallest integer multiple of 4 that is equal to or greater than the given interger
// NOTES:	this is used for memory alignment
inline int Fourtify (const int x)
{
	return (x + 3) & ~3;
}

// PURPOSE: Find the fraction of the distance that the given value is between the other two given values.
// PARAMS:
//	t -	the value to compare with the given range
//	a -	the end of the range
//	b -	the other end of the range
// RETURN:	the fraction of the distance that the given value is between the other two given values
// NOTES:	If a equals b, the problem is undefined and 1.0f is returned
inline float Range (float t,float a,float b)
{
	return((a==b) ? 1.0f : (t-a)/(b-a)); //lint !e777
}

// PURPOSE: Find the fraction of the distance that the given value is between the other two given values,
//			with limits of 0 and 1.
// PARAMS:
//	f -		the value to compare with the given range
//	min -	the lower end of the range
//	max -	the upper end of the range
// RETURN:	the fraction of the distance that the given value is between the other two given values,
//			between 0 and 1
// NOTES:	If min equals max, the problem is undefined and either 0 or 1 is returned
inline float ClampRange (const float f, float min, float max)
{
	//return (f<=min) ? 0.0f : ((f>=max) ? 1.0f : (f-min) / (max-min));
	const float val = (f-min) / (max-min);
	// if f is <= min, return 0.0f
	const float minClamp = Selectf(min - f, 0.0f, val);
	// if f is >= max, return 1.0f;
	return Selectf(f - max, 1.f, minClamp);
}

// PURPOSE: Find the fraction of the distance that the given value is between the other two given values,
//			with limits of 0 and 1.
// PARAMS:
//	f -		the value to compare with the given range
//	min -	the lower end of the range
//	max -	the upper end of the range
// RETURN:	the fraction of the distance that the given value is between the other two given values,
//			between 0 and 1
// NOTES:	If min equals max, the problem is undefined and either 0 or 1 is returned
inline float ClampRange (const int f, int min, int max)
{
	return (f<=min) ? 0.0f : ((f>=max) ? 1.0f : (f-min) / (float)(max-min));
}


// PURPOSE: Map a value by percentage from one range to another range. [0,1] to [-1, 1], for example.
// PARAMS:
// val -	the value to remap
// in_Min -	low end of input range
// in_Max - high end of input range
// out_Min - low end of output range
// out_Max - high end of output range
// RETURN:	value mapped from [in_Min, in_Max] to [out_Min, out_Max]
inline float RemapRange(const float val, const float in_Min, const float in_Max, const float out_Min, const float out_Max)
{
	// an optimization exists where we could return fOut_Min/Max if you're out of range
	return out_Min + ((Clamp(val, in_Min, in_Max) - in_Min) / (in_Max - in_Min)) * (out_Max - out_Min);
}

// PURPOSE:	Find the value the input parameter would have by moving it by an integer multiple of the input range
//			get it within the input range.
// PARAMS:
//	i -		the number to wrap into the range
//	min	-	the lower end of the range
//	max -	the upper end of the range
// RETURN:	the value the input parameter would have by moving it by an integer multiple of the input range
//			get it within the input range
inline int Wrap (int i, int min, int max)
{
	return (min==max) ? min : ((max<min) ? i : ((i-min) % (max-min+1) + ((i<min) ? max+1 : min)));
}


// PURPOSE:	Branch-less integer decrement without crossing zero.  This makes the most sense with non-negative inputs but should still work for negative
//			inputs too - you'll just wrap around from the most negative value to the most positive value and then continue decrementing until you reach
//			zero.  This is particularly useful for countdowns.
// PARAMS:
//  input - the value to be decremented without crossing zero
// RETURN:  if input != 0, (input - 1), else 0.
// NOTES:	Note that this depends on a right shift of a signed quantity *actually* being a signed right shift (this is undefined behavior according to the
//			C/C++ standard but all of the compilers I checked generated the right code).  Should a compiler come along that's not doing the desired thing
//			then either of the FastAssert()'s below should be able to catch it.
__forceinline s32 IDecrementToZero(s32 input)
#if !__SPU
{
	const s32 isNonZeroMask = ((s32)(input | -input)) >> 31;
	FastAssert((input == 0 && isNonZeroMask == 0) || isNonZeroMask == (s32)(-1));
	const s32 output = ((input - 1) & isNonZeroMask);
	FastAssert(input == 0 && output == 0 || output == input - 1);
	return output;
}
#else	// !__SPU
// On SPU just use the trigraph which should generate the selb instruction.
{
	const s32 output = (input == 0 ? 0 : input - 1);
	return output;
}
#endif	// !__SPU

// PURPOSE: Branch-less integer increment-and-wrap function.  Useful for counters that you want to wrap at non-"constant-power-of-two" values.
// PARAMS:
// input - the value (less than the wrap value) to be incremented and then wrapped back to zero if needed
// RETURN:  0 if input == wrapValue - 1, input + 1 otherwise.
// NOTES:   This function still returns input + 1 if input >= wrapValue.  A different function, IIncrementSaturateAndWrap(), is available if you want inputs that are
//          already at or past the wrap value to wrap back to zero (and that function is actually slightly more efficient).  Also note that this is is not a modulus
//          function - the wrapped output value will always be zero.
__forceinline s32 IIncrementAndWrap(s32 input, s32 wrapValue)
{
	const s32 incrementedInputToLimit = ((input + 1) - wrapValue);
	const s32 notAtLimitMask = ((s32)incrementedInputToLimit | -(s32)incrementedInputToLimit) >> 31;
	FastAssert((notAtLimitMask == 0) == (input == wrapValue - 1));
	const s32 output = (input + 1) & (notAtLimitMask);
	return output;
}

// PURPOSE: Branch-less integer increment, saturate, and wrap function.  Useful for counters that you want to wrap at non-"constant-power-of-two" values.
// PARAMS:
// input - the value (less than the wrap value) to be incremented, saturated, and then wrapped back to zero if needed
// RETURN:  0 if input >= wrapValue - 1, input + 1 otherwise.
// NOTES:   This function behaves identically to IIncrementAndWrap() for input <= wrapValue - 1 but is a couple fewer instructions so you'll generally want to use
//          this one instead.  Also note that, like IIncrementAndWrap(), this is is not a modulus function - the wrapped output value will always be zero regardless
//          of how far past the wrap value the input was.
__forceinline s32 IIncrementSaturateAndWrap(s32 input, s32 wrapValue)
{
	const s32 inputToLimit = (input - wrapValue + 1);
	const s32 notAtOrPastLimitMask = (s32)(inputToLimit) >> 31;
	FastAssert((notAtOrPastLimitMask == 0) == (input >= wrapValue - 1));
	const s32 output = (notAtOrPastLimitMask & (input + 1));
	return output;
}

// PURPOSE: Branch-less integer non-zero mask function.  Useful for performing bitwise boolean operations on integer values.
//			(Non-zero but non 0xFFFFFFFF values would otherwise have unexpected results when used with a bitwise AND operation.)
// PARAMS:	input - the value to be masked
// RETURN:  0 if input == 0, 0xFFFFFFFF otherwise
// NOTES:	
__forceinline u32 GenerateMaskNZ(s32 input)
{
	const u32 output = (input | -input) >> 31;
	return output;
}


// PURPOSE: Branch-less function to generate integer greater-than-zero mask.  Useful for performing bitwise boolean operations on integer values.
//			(Non-zero but non 0xFFFFFFFF values would otherwise have unexpected results when used with a bitwise AND operation.)
// PARAMS:	input - the value to be masked
// RETURN:  0 if input >= 0, 0xFFFFFFFF otherwise
// NOTES:	This is slightly cheaper than GenerateMaskNZ() and thus is preferable (and equivalent) to GenerateMaskNZ if you know that your input will not be negative.
__forceinline u32 GenerateMaskGZ(s32 input)
{
	const u32 output = (-input) >> 31;
	return output;
}


// PURPOSE: Branch-less function to generate integer less-than-zero mask.  Useful for performing bitwise boolean operations on integer values.
//			(Non-zero but non 0xFFFFFFFF values would otherwise have unexpected results when used with a bitwise AND operation.)
// PARAMS:	input - the value to be masked
// RETURN:  0 if input >= 0, 0xFFFFFFFF otherwise
// NOTES:	This is slightly cheaper than GenerateMaskNZ() and thus is preferable (and equivalent) to GenerateMaskNZ if you know that your input will not be positive.
__forceinline u32 GenerateMaskLZ(s32 input)
{
	const u32 output = (input) >> 31;
	return output;
}


// PURPOSE: Branch-less function to generate integer equality mask.
// PARAMS:	input, comparitor - the two values to be compared
// RETURN:  0xFFFFFFFF if input == comparitor, 0 otherwise
// NOTES:	
__forceinline size_t GenerateMaskEq(ptrdiff_t input, ptrdiff_t comparitor)
{
	const ptrdiff_t diff = (input - comparitor);
	const size_t output =  ~((diff | -diff) >> 31);
	return output;
}


// PURPOSE: Branch-less function to generate integer inequality mask.
// PARAMS:	input, comparitor - the two values to be compared
// RETURN:  0xFFFFFFFF if input != comparitor, 0 otherwise
// NOTES:	This is very slightly cheaper than GenerateMaskEq().
__forceinline u32 GenerateMaskNE(s32 input, s32 comparitor)
{
	const s32 diff = (input - comparitor);
	const u32 output =  (diff | -diff) >> 31;
	return output;
}


// PURPOSE: Branch-less function to generate integer inequality mask.
// PARAMS:	input, comparitor - the two values to be compared
// RETURN:  0xFFFFFFFF if input < comparitor, 0 otherwise
__forceinline u32 GenerateMaskLT(s32 input, s32 comparitor)
{
	const s32 diff = (input - comparitor);
	const u32 output =  diff >> 31;
//	FastAssert((output == 0xffffffff) == input < comparitor);
//	FastAssert((output == 0x00000000) == input >= comparitor);
	return output;
}


__forceinline u32 ISelectI(u32 selector, u32 input0, u32 input1)
{
	const u32 output = (selector & input1) | (~selector & input0);
	return output;
}


template <class Type> __forceinline Type *ISelectP(size_t selector, Type *input0, Type *input1)
{
	Type *output = (Type *)((selector & (size_t)input1) | (~selector & (size_t)input0));
	return output;
}


// PURPOSE: Splits a floating-point value into fractional and integer parts
// PARAM:
//	f -		Floating point value
//	i -		pointer to stored integer (in floating point)
// RETURN:	the fractional part of f
inline float Modf(float f, float* i)
{
#if __XENON || __PPU
	// include files 
	// Convert to int64 with round-to-zero and back to double, then float
#ifdef __SNC__
	float intPart = (float)__builtin_fcfid(__builtin_fctidz(f));
#else
	float intPart = (float)__fcfid(__fctidz(f));
#endif
	float fracPart = f - intPart;
	*i = intPart;
	return fracPart;
#else
	return ::modf(f, i);
#endif
}

// PURPOSE:	Find the value the input parameter would have by moving it by an integer multiple of the input range
//			get it within the input range.
// PARAMS:
//	f -		the number to wrap into the range
//	min	-	the lower end of the range
//	max -	the upper end of the range
// RETURN:	the value the input parameter would have by moving it by an integer multiple of the input range
//			get it within the input range
inline float Wrap (float f, float min, float max)
{
	if (max<min) return f;
	if (max==min) return min; //lint !e777
	if (f<min) return max-fmodf(min-f,max-min);
	if (f>max) return min+fmodf(f-max,max-min);
	return f;
}

// PURPOSE: Increment the given number by the given rate over the given time step, and find whether or not
//			the new number has just reached the given goal.
// PARAMS:
// value -	An in/out parameter, containing the value that is approaching the goal.
// goal -	The goal, which is being approached by value.
// rate -	How quickly the value is moving toward the goal.
// time -	How much elapsed time to process in this call.
// RETURNS:	true if value reaches the goal in this time step, false otherwise
// NOTES:	The value parameter is changed to the new value, which is clamped to the goal if it passes.
inline bool Approach (float &value, float goal, float rate, float time)
{
	if (value<goal)
	{
		value += rate*time;
		if (value>goal)
		{
			value = goal;
		}
	}
	else if (value>goal)
	{
		value -= rate*time;
		if (value<goal)
		{
			value = goal;
		}
	}

	return (value==goal);
}

// PURPOSE: Find the number that follows a sine curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find a point on the sine curve
// RETURN:	the sine of half Pi times the input parameter, if it is between 0 and 1
// NOTES:
//	1.	If the input parameter is less than zero, then zero is returned. If it is greater than
//		one then one is returned.
//	2.	This is used to follow a curve that smoothly accelerates as the input parameter changes
//		evenly between 0 and 1. It will start slowly, and reach maximum speed as it hits 1.
inline float SlowOut (float t)
{
	if (t>0.0f)
	{
		if (t<1.0f)
		{
			return rage::Sinf(t*PI*0.5f);
		}
		return 1.0f;
	}
	return 0.0f;
}


// PURPOSE: Find derivative of the number that follows a sine curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the speed on the curve
// RETURN:	the cosine of half Pi times the input parameter, if it is between 0 and 1
// NOTES:
//	1.	If the input parameter is less than zero, then one is returned. If it is greater than
//		one then zero is returned.
//	2.	This is used to find the speed along a curve that smoothly accelerates as the input parameter
//		changes evenly between 0 and 1.
//	3.	This is the derivative of SlowOut.
inline float dSlowOut (float t)
{
	if (t>0.0f)
	{
		if (t<1.0f)
		{
			return rage::Cosf(t*PI*0.5f);
		}
		return 0.0f;
	}
	return 1.0f;
}

// PURPOSE: Find the number that follows one minus a sine curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find a point on the curve
// RETURN:	one minus the sine of half Pi times the input parameter, if it is between 0 and 1
// NOTES:
//	1.	If the input parameter is less than zero, then one is returned. If it is greater than
//		one then zero is returned.
//	2.	This is used to follow a curve that smoothly decelerates as the input parameter changes
//		evenly between 0 and 1. It will start quickly, and reach zero speed as it hits 1.
inline float SlowIn(float t)
{
	return 1.0f-dSlowOut(t);
}

// PURPOSE: Find derivative of the number that follows one minus a sine curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the speed on the curve
// RETURN:	the sine of half Pi times the input parameter, if it is between 0 and 1
// NOTES:
//	1.	If the input parameter is less than zero, then zero is returned. If it is greater than
//		one then one is returned.
//	2.	This is used to find the speed along a curve that smoothly decelerates as the input parameter
//		changes evenly between 0 and 1.
//	3.	This is the derivative of SlowIn, and it is the same as SlowOut.
inline float dSlowIn(float t)
{
	return SlowOut(t);
}

// PURPOSE: Find the speed along a smooth curve for a given number between 0 and 1.
//			The curve starts from rest, accelerates until it reaches half way, then decelerates
//			to reach one with zero speed.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the speed on the curve
// RETURN:	the speed on the curve that smoothly accelerates and decelerates from 0 to 1
// NOTES:
//	1.	If the input parameter is less than zero, then one is returned. If it is greater than
//		one then zero is returned.
//	2.	This is used to find the speed along a curve that smoothly accelerates and then decelerates
//		as the input parameter changes evenly between 0 and 1.
inline float dSlowInOut (float t)
{
	if (t>0.0f)
	{
		if (t<1.0f)
		{
			return rage::Cosf(t*PI);
		}
		return -1.0f;
	}
	return 1.0f;
}

// PURPOSE: Find the number that follows a smooth curve for a given number between 0 and 1.
//			The curve starts from rest, accelerates until it reaches half way, then decelerates
//			to reach one with zero speed.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the curve
// RETURN:	the point on the curve that smoothly accelerates and decelerates from 0 to 1
// NOTES:
//	1.	If the input parameter is less than zero, then zero is returned. If it is greater than
//		one then one is returned.
//	2.	This is used to find the point along a curve that smoothly accelerates and then decelerates
//		as the input parameter changes evenly between 0 and 1.
inline float SlowInOut (float t)
{
	return 0.5f*(1.0f-dSlowInOut(t));
}

// PURPOSE: Find the number that follows a quadratic-ease in shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the quadratic-ease in curve
// RETURN:	the point on the quadratic ease out shaped curve
inline float QuadraticEaseIn (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return t * t;
}

// PURPOSE: Find the number that follows a quadratic ease in-shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the quadratic-ease in curve
// RETURN:	the point on the quadratic ease in shaped curve
inline float QuadraticEaseOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return  1.0f - QuadraticEaseIn(1.0f - t);
}

// PURPOSE: Find the number that follows a quadratic ease in/out curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the quadratic ease in/out curve
// RETURN:	the point on the quadratic ease in/out shaped curve
inline float QuadraticEaseInOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	if (t<0.5)
	{
		return QuadraticEaseIn(2.0f * t)/2.0f;
	}
	else
	{
		return 1.0f - QuadraticEaseIn((1.0f - t)*2.0f) /2.0f;
	}
}


// PURPOSE: Find the number that follows a Cubic-ease in shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Cubic-ease in curve
// RETURN:	the point on the Cubic ease out shaped curve
inline float CubicEaseIn (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return t * t * t;
}

// PURPOSE: Find the number that follows a Cubic ease in-shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Cubic-ease in curve
// RETURN:	the point on the Cubic ease in shaped curve
inline float CubicEaseOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return  1.0f - CubicEaseIn(1.0f - t);
}

// PURPOSE: Find the number that follows a Cubic ease in/out curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Cubic ease in/out curve
// RETURN:	the point on the Cubic ease in/out shaped curve
inline float CubicEaseInOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	if (t<0.5)
	{
		return CubicEaseIn(2.0f * t)/2.0f;
	}
	else
	{
		return 1.0f - CubicEaseIn((1.0f - t)*2.0f) /2.0f;
	}
}


// PURPOSE: Find the number that follows a Quartic-ease in shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Quartic-ease in curve
// RETURN:	the point on the Quartic ease out shaped curve
inline float QuarticEaseIn (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return t * t * t * t;
}

// PURPOSE: Find the number that follows a Quartic ease in-shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Quartic-ease in curve
// RETURN:	the point on the Quartic ease in shaped curve
inline float QuarticEaseOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return  1.0f - QuarticEaseIn(1.0f - t);
}

// PURPOSE: Find the number that follows a Quartic ease in/out curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Quartic ease in/out curve
// RETURN:	the point on the Quartic ease in/out shaped curve
inline float QuarticEaseInOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	if (t<0.5)
	{
		return QuarticEaseIn(2.0f * t)/2.0f;
	}
	else
	{
		return 1.0f - QuarticEaseIn((1.0f - t)*2.0f) /2.0f;
	}
}

// PURPOSE: Find the number that follows a Quintic-ease in shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Quintic-ease in curve
// RETURN:	the point on the Quintic ease out shaped curve
inline float QuinticEaseIn (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return t * t * t * t * t;
}

// PURPOSE: Find the number that follows a Quintic ease in-shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Quintic-ease in curve
// RETURN:	the point on the Quintic ease in shaped curve
inline float QuinticEaseOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return  1.0f - QuinticEaseIn(1.0f - t);
}

// PURPOSE: Find the number that follows a Quintic ease in/out curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Quintic ease in/out curve
// RETURN:	the point on the ease Quintic in/out shaped curve
inline float QuinticEaseInOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	if (t<0.5)
	{
		return QuinticEaseIn(2.0f * t)/2.0f;
	}
	else
	{
		return 1.0f - QuinticEaseIn((1.0f - t)*2.0f) /2.0f;
	}
}

// PURPOSE: Find the number that follows a Circular-ease in shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Circular-ease in curve
// RETURN:	the point on the Circular ease out shaped curve
inline float CircularEaseIn (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	return 1.0f - sqrt(1.0f - (t * t));
}

// PURPOSE: Find the number that follows a Circular ease in-shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Circular-ease in curve
// RETURN:	the point on the Circular ease in shaped curve
inline float CircularEaseOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	float oneMinusT = 1.0f - t;
	return sqrt(1.0f - (oneMinusT*oneMinusT));
}

// PURPOSE: Find the number that follows a Circular ease in/out curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the Circular ease in/out curve
// RETURN:	the point on the Circular ease in/out shaped curve
inline float CircularEaseInOut (float t)
{
	Assert(t>=0.0f && t<=1.0f);
	if (t<0.5)
	{
		return CircularEaseIn(2.0f * t)/2.0f;
	}
	else
	{
		return 1.0f - CircularEaseIn((1.0f - t)*2.0f) /2.0f;
	}
}

// PURPOSE: Find the number that follows a bell-shaped curve for a given number between 0 and 1.
// PARAMS:
//	t -	a number that should be between 0 and 1, with which to find the point on the curve
// RETURN:	the point on the bell-shaped curve
// NOTES:
//	1.	If the input parameter is less than zero, then zero is returned. If it is greater than
//		one then one is returned.
inline float BellInOut (float t)
{
	if (t>0.0f)
	{
		if (t<1.0f)
		{
			return 0.5f*(1.0f-rage::Cosf(t*PI*2.0f));
		}
		return 0.0f;
	}
	return 0.0f;
}

//////// Trig Functions /////////
// PURPOSE:	Calculate the cosine and the sine of the given angle.
// PARAMS:
//	cos -	reference in which to put the cosine
//	sin -	reference in which to put the sine
//	angle -	the angle of which to find the cosine and sine
// NOTES:	This is not faster than calling cosf and sinf separately.
#if __PPU
void cos_and_sin( float &cos,float &sin,float angle );

#else
void inline cos_and_sin(float &cos, float &sin, float angle) { cos = rage::Cosf(angle); sin = rage::Sinf(angle); }
#endif
// PURPOSE: Make sure the given sine is valid (between -1 and 1), and calculate its angle.
// RETURN:	the angle between 0 and Pi that has the given sine
// NOTES:	Call asinf instead if the sine is known to be between -1 and 1.
inline float safe_asinf(float sine)				{return asinf(Clamp(sine,-0.99999f,0.99999f));}

// PURPOSE: Calculate the angle that has a tangent that is the ratio of the input parameters (y/x).
// PARAMS:
//	y -	the numerator in the ratio that is the tangent of the angle
//	x -	the denominator in the ratio that is the tangent of the angle
// RETURN:	the angle who's tangent is y/x, in the range -Pi to Pi
// NOTES:
//	1.	This checks for zero values in y and x. Call atan2f if y or x are known to be non-zero.
//	2.	Two input parameters are used to determine which quadrant of the circle the angle is in.
//	3.	The argument names are swapped in atan2f in Microsoft's math.h file, but the meanings
//		are the same. The first argument is the vertical axis (y) and the second argument is the
//		horizontal axis (x) in which (y==0, x==1) corresponds to an angle of 0 or 2PI.
inline float safe_atan2f (float y, float x)		{if (Verifyf(x||y,"Zero inputs to safe_atan2f")) return rage::Atan2f(y,x); return 0.0f; }

// PURPOSE:	Find the nearest integer to the given number.
// PARAMS:
//	f -	the number to round to the nearest integer
// RETURN: The integer nearest the given number.
inline int		round (float f)					{return(f<0?(int)(f-0.5f):(int)(f+0.5f));}
inline int		round (double f)				{return(f<0?(int)(f-0.5f):(int)(f+0.5f));}

// PARAMS:	Find the square root of the given number.
// PARAMS:
//	x -	the number of which to get the square root
// RETURN:	the square root of the given number
// NOTES:	This triggers an assert failure if the number is negative. If this happens, call SqrtfSafe instead.
inline float Sqrtf (float x) 
{ 
	// Call SqrtfSafe() instead of Sqrtf if this fails. 
	FastAssert(x>=0.0f);

#if __XENON || __PPU
	return __fsqrts(x);
#else
	return sqrtf(x); 
#endif
}

// PURPOSE: Calculate one over the square root of the given number.
// PARAMS:
//	f -	the number to square root and invert
// RETURN:	One over the square root of the given number.
// NOTES:	If the given number is 0, then 0 is returned.
inline float	invsqrtf(float f)				
{
#if __PPU
	mthAssertf(f>=0.0f,"invsqrtf(%f) called, that's negative, call InvSqrtfSafe() instead.",f); 

	// Alternative implementation of newton-raphson
	// y1 = y0+0.5*y0*(1-x*y0*y0)
	//    = y0*1.5 - (0.5*x * y0*y0)

	double r;
	{
		double r1,fh,rr,rfh,r3h;

#ifdef __SNC__
		r1 = __builtin_frsqrte(f);
#else
		r1 = __frsqrte(f);
#endif
		fh = f*0.5;

		//fp stall

		rr = r1*r1;
		rfh = r1*fh;
		r3h = r1+(r1*0.5);		// Reuse constant

		//fp stall

		r = r3h-rr*rfh;

		//fp stall

		rr = r*r;
		rfh = r*fh;
		r3h = r+(r*0.5);

		//fp stall

		r = r3h - rr*rfh;




	}
	return float(__fsel( -f, 0.0f, r ));

#elif __XENON
	// Call InvSqrtfSafe() instead of invsqrtf if this fails.
	FastAssert(f>=0.0f);

	// Refinement (Newton-Raphson) for 1.0 / sqrt(x)
	//     y0 = reciprocal_sqrt_estimate(x)
	//     y1 = y0 + 0.5 * y0 * (1.0 - x * y0 * y0) 
	//        = y0 + y0 * (0.5 - 0.5 * x * y0 * y0)

	double r;
	{
		r = __frsqrte(f);
		double rr = r*r;
		double r2 = r*0.5;
		double nms = 1.0 - f*rr;
		r = r + nms*r2;

		rr = r*r;
		r2 = r*0.5;
		nms = 1.0 - f*rr;
		r = r + nms*r2;
	}

	return Selectf( -f, 0.f, float(r) );
#else
	if(f == 0.0f) return 0.0f; return 1.0f/Sqrtf(f);
#endif
}


// PURPOSE: Calculate one over the square root of the given number, 
//			and do two more Newton-Rhapson iterations.
// PARAMS:
//		f -	the number to square root and invert
// RETURN:	One over the square root of the given number.
//
//	http://www.freescale.com/files/32bit/doc/ref_manual/ALTIVECPIM.pdf
//	vec_rsqrte – precision is 1/4096 (12 bits)
//	page 135
//
//	http://moss.csc.ncsu.edu/~mueller/cluster/ps3/SDK3.0/docs/arch/PPC_Vers202_Book1_public.pdf
//	frsqrte – precision is 1/32 (5 bits)
//	page 136
//	

inline float invsqrtf_precise(float f)
{
#if __XENON || __PPU
	DebugAssert(f>=0.0f);

	// Refinement (Newton-Raphson) for 1.0 / sqrt(x)
	//     y0 = reciprocal_sqrt_estimate(x)
	//     y1 = y0 + 0.5 * y0 * (1.0 - x * y0 * y0)
	//        = y0 + y0 * (0.5 - 0.5 * x * y0 * y0)

	double r;
	{
#ifdef __SNC__
		r= __builtin_frsqrte(f);
#else
		r= __frsqrte(f);
#endif	
		double rr = r*r;
		double r2 = r*0.5;
		double nms = 1.0 - f*rr;
		r = r + nms*r2;

		rr = r*r;
		r2 = r*0.5;
		nms = 1.0 - f*rr;
		r = r + nms*r2;

		rr = r*r;
		r2 = r*0.5;
		nms = 1.0 - f*rr;
		r = r + nms*r2;
	}

	return float(__fsel( -f, 0.0f, r ));
#else
	if(f == 0.0f) return 0.0f; return 1.0f/Sqrtf(f);
#endif
}

// PURPOSE: Calculate one over the square root of the given number, using a lookup table.
// PARAMS:
//	f -	the number to square root and invert
// RETURN:	One over the square root of the given number.
// NOTES:	This uses a lookup table for more speed and less accuracy than invsqrtf.
#if __XENON || __PPU
#ifdef __SNC__
inline float	invsqrtf_fast(float f)			{return (float)__builtin_frsqrte(f);}
#else
inline float	invsqrtf_fast(float f)			{return (float)__frsqrte(f);}
#endif
#else
float			invsqrtf_fast(float f);//lint !e765 !e762 ask to be made static, double def (its not)
#endif

// PURPOSE: Calculate the square, third or fourth power of the given number.
// PARAMS:
//	x -	the number to multiply by itself one, two or three times
// RETURN:	the square, third or fourth power of the given number
template <typename _T>
__forceinline _T	square (AMATH_ARG(_T) x)	{return x*x;}

// <COMBINE square>
template <typename _T>
__forceinline _T	power3 (AMATH_ARG(_T) x)	{return x*x*x;}

// <COMBINE square>
template <typename _T>
__forceinline _T	power4 (AMATH_ARG(_T) x)	{return x*x*x*x;}

// PURPOSE: Internal use only, floor of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Floor of log base two, or -1 if a==0
__forceinline unsigned Log2FloorInternal(u64 a)
{
#	if __PPU
		return 63-__cntlzd(a);
#	elif __SPU
		qword t0 = si_sfi(si_clz(si_from_uint(a)), 31);
		qword t1 = si_ceqi(t0, -1);
		qword t2 = si_and(si_rotqmbyi(t0, 4), t1);
		return si_to_uint(si_a(t0, t2));
#	elif RSG_DURANGO
		// Note that AMD documents slightly different behaviour than Intel for
		// BSR when the input is zero.  On AMD, the output register is
		// unmodified, but on Intel it is undefined.  We can take advantage of
		// this on XB1 to help improve Log2Ceil.
		unsigned long bsr = ~0ul;
		_BitScanReverse64(&bsr, a);
		return bsr;
#	elif RSG_CPU_INTEL && defined(_MSC_VER)
		// The more generic, Intel compatible, implementation should not be much
		// worse though, as the compiler should be able to generate a CMOVZ
		// here.
		unsigned long bsr;
		unsigned char valid = _BitScanReverse64(&bsr, a);
		return valid ? bsr : ~0u;
#	elif RSG_ORBIS
		u64 bsr = ~0ull;
		__asm__
		(
			"bsrq   %1, %0\n\t"
			: "+r"(bsr)
			: "r"(a)
			: "cc"
		);
		return (unsigned)bsr;
#	elif __XENON
		return 63-_CountLeadingZeros64(a);
#	else
#		error "not yet implemented"
#	endif
}

// PURPOSE: Internal use only, floor of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Floor of log base two, or -1 if a==0
__forceinline unsigned Log2FloorInternal(u32 a)
{
#	if __PPU
		return 31-__cntlzw(a);
#	elif __SPU
		return 31-si_to_uint(si_clz(si_from_uint(a)));
#	elif RSG_DURANGO
		unsigned long bsr = ~0ul;
		_BitScanReverse(&bsr, a);
		return bsr;
#	elif RSG_CPU_INTEL && defined(_MSC_VER)
		unsigned long bsr;
		unsigned char valid = _BitScanReverse(&bsr, a);
		return valid ? bsr : ~0u;
#	elif RSG_ORBIS
		u32 bsr = ~0u;
		__asm__
		(
			"bsrl   %1, %0\n\t"
			: "+r"(bsr)
			: "r"(a)
			: "cc"
		);
		return bsr;
#	elif __XENON
		return 31-_CountLeadingZeros(a);
#	else
#		error "not yet implemented"
#	endif
}

// PURPOSE: Internal use only, floor of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Floor of log base two, or -1 if a==0
__forceinline unsigned Log2FloorInternal(u16 a)
{
	return Log2FloorInternal((u32)a);
}

// PURPOSE: Internal use only, floor of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Floor of log base two, or -1 if a==0
__forceinline unsigned Log2FloorInternal(u8 a)
{
	return Log2FloorInternal((u32)a);
}

// PURPOSE: Floor of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Floor of log base two
// NOTES:   Undefined for a==0
__forceinline unsigned Log2Floor(u64 a)
{
	Assert(a);
#	if RSG_CPU_INTEL && defined(_MSC_VER)
		// XB1 and PC have slightly unoptimal Log2FloorInternal implementations
		// (they are optimal for implementing Log2Ceil), so re-implement here.
		unsigned long bsr;
		ASSERT_ONLY(unsigned char valid =) _BitScanReverse64(&bsr, a);
		Assert(valid);
		return bsr;
#	elif RSG_ORBIS
		// Same thing for Orbis.
		u64 bsr;
		u8 valid;
		__asm__
		(
			"bsrq   %2, %0\n\t"
#		  if __ASSERT
			"setnz  %1\n\t"
#		  endif
			: "=r"(bsr), "=r"(valid)
			: "r"(a)
			: "cc"
		);
		Assert(valid);
		(void)valid;
		return (unsigned)bsr;
#	else
		return Log2FloorInternal(a);
#	endif
}

// PURPOSE: Floor of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Floor of log base two
// NOTES:   Undefined for a==0
__forceinline unsigned Log2Floor(u32 a)
{
	Assert(a);
#	if RSG_CPU_INTEL && defined(_MSC_VER)
		unsigned long bsr;
		ASSERT_ONLY(unsigned char valid =) _BitScanReverse(&bsr, a);
		Assert(valid);
		return bsr;
#	elif RSG_ORBIS
		u32 bsr;
		u8 valid;
		__asm__
		(
			"bsrl   %2, %0\n\t"
#		  if __ASSERT
			"setnz  %1\n\t"
#		  endif
			: "=r"(bsr), "=r"(valid)
			: "r"(a)
			: "cc"
		);
		Assert(valid);
		(void)valid;
		return bsr;
#	else
		return Log2FloorInternal(a);
#	endif
}

// PURPOSE: Floor of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Floor of log base two
// NOTES:   Undefined for a==0
__forceinline unsigned Log2Floor(u16 a)
{
	return Log2Floor((u32)a);
}

// PURPOSE: Floor of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Floor of log base two
// NOTES:   Undefined for a==0
__forceinline unsigned Log2Floor(u8 a)
{
	return Log2Floor((u32)a);
}

// PURPOSE: Ceil of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Ceil of log base two
// NOTES:   Undefined for a==0
__forceinline unsigned Log2Ceil(u64 a) {
	Assert(a);
	// Note that Log2FloorInternal returns -1 for input 0.
	// This trick is from Chromium source (https://chromium.googlesource.com/chromium/src.git/+/refs/heads/master/base/bits.h).
	return 1+Log2FloorInternal(a-1);
}

// PURPOSE: Ceil of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Ceil of log base two
// NOTES:   Undefined for a==0
__forceinline unsigned Log2Ceil(u32 a) {
	Assert(a);
	return 1+Log2FloorInternal(a-1);
}

// PURPOSE: Ceil of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Ceil of log base two
// NOTES:   Undefined for a==0
__forceinline unsigned Log2Ceil(u16 a) {
	Assert(a);
	return 1+Log2FloorInternal((u32)a-1);
}

// PURPOSE: Ceil of log base two
// PARAMS:
//	a -	integer to get log of
// RETURN:	Ceil of log base two
// NOTES:   Undefined for a==0
__forceinline unsigned Log2Ceil(u8 a) {
	Assert(a);
	return 1+Log2FloorInternal((u32)a-1);
}

// PURPOSE: Compile time floor of log base two
// NOTES: X must be non-zero
template<unsigned long long X>
struct CompileTimeLog2Floor
{
	enum { value = 1+CompileTimeLog2Floor<(X>>1)>::value };
};
template<> struct CompileTimeLog2Floor<1>
{
	enum { value = 0 };
};

// PURPOSE: Compile time ceil of log base two
// NOTES: X must be non-zero
template<unsigned long long X>
class CompileTimeLog2Ceil
{
private:
	enum { floor = CompileTimeLog2Floor<X>::value };
public:
	enum { value = floor+((1<<floor)<X) };
};

template<unsigned long long X>
struct CompileTimeCountTrailingZeroesInternal
{
	enum { value = (X&1) ? 0 : (1+CompileTimeCountTrailingZeroesInternal<(X>>1)>::value) };
};
template<> struct CompileTimeCountTrailingZeroesInternal<0>
{
	enum { value = 0 };
};

// PURPOSE: Compile time count trailing zeroes
// NOTES: X must be non-zero (because the number of bits used to represent X is unknown)
template<unsigned long long X>
struct CompileTimeCountTrailingZeroes
{
	enum { value = CompileTimeCountTrailingZeroesInternal<X>::value };
};
template<> struct CompileTimeCountTrailingZeroes<0> {};

// PURPOSE: Find the determinant of the 2x2 matrix of the four given numbers.
// PARAMS:
//	a -	the first row and first column element of the matrix
//	b -	the second row and second column element of the matrix
//	c -	the first row and second column element of the matrix
//	d - the second row and first column element of the matrix
// RETURN: The determinant of the 2x2 matrix of the four given numbers
inline float __cross2(float a, float b, float c, float d)
{
	return a*b - c*d;
}

// PURPOSE: Find the dot product of the two vectors represented by lists of numbers.
// PARAMS:
//	a - the first element in the first vector
//	b - the first element in the second vector
//	c - the second element in the first vector
//	d -	the second element in the second vector
//	e - the third element in the first vector
//	f -	the third element in the second vector
// RETURN: The dot product of two vectors represented by lists of floats.
inline float __dot3 (float a, float b, float c, float d, float e, float f)
{
	return a*b + c*d + e*f;
}

// PURPOSE: Find the dot product of the two vectors represented by lists of numbers,
//			plus another given number.
// PARAMS:
//	a - the first element in the first vector
//	b - the first element in the second vector
//	c - the second element in the first vector
//	d -	the second element in the second vector
//	e - the third element in the first vector
//	f -	the third element in the second vector
//	t -	the additive number
// RETURN: The dot product of two vectors represented by lists of floats, plus the additive number.
inline float __dot3t (float a, float b, float c, float d, float e, float f, float t)
{
	return a*b + c*d + e*f + t;
}


// PURPOSE: Find the dot product of the two 4-vectors represented by lists of numbers.
// PARAMS:
//	a - the first element in the first vector
//	b - the first element in the second vector
//	c - the second element in the first vector
//	d - the second element in the second vector
//	e -	the third element in the first vector
//	f -	the third element in the second vector
//	g -	the fourth element in the first vector
//	h -	the fourth element in the second vector
// RETURN: the dot product of two vectors represented by lists of floats
inline float __dot4 (float a,float b,float c,float d,float e,float f,float g,float h)
{
	return a*b + c*d + e*f + g*h;
}

#if __WIN32
// PURPOSE: Prevent invalid and infinite floating point values.
// NOTES:	To use, add this to your main program: int _matherr(struct _exception *e) {return MathErr(e);}
int MathErr(_exception *except);
#endif

#if defined(__linux) || defined(WIN32) || defined(_DOS)

// PURPOSE: Set the floating point state to:
//				invalid exception				enabled
//				divide-by zero exception:		enabled
//				overflow exception:				enabled
//				rounding control:				near
//				precision:						single (24 bits)
extern void		SetFPUControlWord();

// <COMBINE SetFPUControlWord>
extern void		GetFPUControlWord();

#endif

// PORPOSE: Set the floating point bit precision to 24
void SetFPU24BitPrecision();		//lint !e757

// PARAMS:	Find the square root of the given number, or zero if the number is negative.
// PARAMS:
//	x -	the number of which to get the square root
// RETURN:	the square root of the given number, clamped to zero if the number is negative
inline float SqrtfSafe (float x) { return (x>0.0f ? sqrtf(x) : 0.0f); }

// PURPOSE: Find the angle that has the given cosine.
// PARAMS:
//	cosine -	the cosine from which to find the angle
// RETURN:	The angle that has the given cosine, in the range 0 to Pi
// NOTES:	This triggers an mthAssert if the cosine is not valid. If this happens, call AcosfSafe instead.
inline float Acosf (float cosine)
{
	mthAssertf(fabsf(cosine)<=1.0f + FLT_EPSILON,"Acosf(%f) called and that's less than -1 or greater than 1, call AcosfSafe() instead.",cosine);
#if __PPU
	return rage::Atan2f(rage::Sqrtf(1-(cosine*cosine)),cosine);
#else
	return acosf(cosine);
#endif
}

// PURPOSE: Find the angle that has the given cosine, after clamping to be sure it's a valid cosine.
// PARAMS:
//	cosine -	the cosine from which to find the angle
// RETURN:	The angle that has the given cosine, in the range 0 to Pi
inline float AcosfSafe (float cosine) { return (cosine>-1.0f ? (cosine<1.0f ? 
#if __PPU
												rage::Atan2f(rage::SqrtfSafe(1-(cosine*cosine)),cosine)
#else
												acosf(cosine)
#endif
												: 0.0f) : PI); }

// PURPOSE: Find the angle that has the given sine.
// PARAMS:
//	sine -	the sine from which to find the angle
// RETURN:	The angle that has the given sine, in the range -Pi/2 to Pi/2
// NOTES:	This triggers an mthAssert if the sine is not valid. If this happens, call AsinfSafe instead.
inline float Asinf (float sine)
{
	mthAssertf(1.0f-(sine*sine) >= 0.0f,"Asinf(%f) called and that's greater than 1, call AsinfSafe() instead.",sine);
#if __PPU
	return rage::Atan2f(sine,rage::Sqrtf(1-(sine*sine)));
#else
	return asinf(sine);
#endif

}

// PURPOSE: Find the angle that has the given sine, after clamping to be sure it's a valid sine.
// PARAMS:
//	sine -	the sine from which to find the angle
// RETURN:	The angle that has the given sine, in the range -Pi/2 to Pi/2
inline float AsinfSafe (float sine) { return (sine>-1.0f ? (sine<1.0f ?
#if __PPU
											rage::Atan2f(sine,rage::SqrtfSafe(1-(sine*sine)))
#else
											asinf(sine)
#endif
											  : 0.5f*PI) : -0.5f*PI); }



// PARAMS:	Find one over the square root of the given number.
// PARAMS:
//	x -	the number of which to get one over the square root
// RETURN:	one over the square root of the given number
// NOTES:	This triggers an mthAssert if the number is negative. If this happens, call InvSqrtfSafe instead.
inline float InvSqrtf (float x)
{
	// Call InvSqrtfSafe() instead of InvSqrtf if this fails.
	FastAssert(x>0.0f);

	return invsqrtf(x);
}

// PARAMS:	Find one over the square root of the given number, or FLT_MAX if the number is negative.
// PARAMS:
//	x -	the number of which to get one over the square root
//	nonPosInvSqrtf - optional return value when x is not positive (default is FLT_MAX)
// RETURN:	one over the square root of the given number, or the given return value if the number is not positive
float InvSqrtfSafe (float x, float nonPosInvSqrtf=FLT_MAX);

// PURPOSE: Convert a floating point number into a fixed point number
// PARAMS:
//	value - The floating point value to be converted
//	size - the number of bits of precision to shift
//	shift - Number of bits the return value should be shifted by
// RETURN: The fixed point number specified by the number of bits in size and the floating point number in value, shifted left by the number of bits in shift
// NOTES: This operation is simply multiplying the float by a constant value and discarding the extra bits
//			fixedPoint = value * (1 << size);
//			The return value is shifted by the amount of bits in the shift parameter
inline u32 PackFixedPoint(float value,u32 size,u32 shift)
{
	float scale = (float)((1 << (size-1)) - 1);
	return (u32(value * scale) & ((1 << size)-1)) << shift;
}

// PURPOSE: Returns the largest integer that is less than or equal to value.
// PARAMS:
//	value - The floating point value.
// RETURN: The largest integer that is less than or equal to value.
// NOTES: The return value of Floorf() is still of type float because the value range of float is usually bigger than that of integer. 
inline float Floorf(float value)
{
#if __XENON /*|| __PPU*/
	float c = Selectf(value, -1 << 23, 1 << 23);
	float result = value - c + c;
	return Selectf(value - result, result, result - 1.f);
#else
	return floorf(value);
#endif
}

// This function is the same as Floorf with the optimization enabled for both platforms
// We are late in the project and fear that just enabling the optimization for the other function is dangerous
inline float FloorfFast(float value)
{
#if __XENON || __PPU
	float c = Selectf(value, -1 << 23, 1 << 23);
	float result = value - c + c;
	return Selectf(value - result, result, result - 1.f);
#else
	return floorf(value);
#endif
}

inline float InvSqrtfSafe (float x, float nonPosInvSqrtf)
{
	return (x>0.0f ? invsqrtf(x) : nonPosInvSqrtf);
}


inline float Saturate( float a )
{
	return Clamp( a, 0.0f, 1.0f );
}

#undef AMATH_ARG

} // namespace rage

#endif // MATH_AMATH_H
