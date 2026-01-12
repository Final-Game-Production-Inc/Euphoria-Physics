//
// math/simplemath.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef MATH_SIMPLEMATH_H
#define MATH_SIMPLEMATH_H

#include "amath.h"
#include "constants.h"

#include "system/bit.h"		// For CountOnBits, which used to be in here

namespace rage {

/*
	PURPOSE
		This file provides a collection of simple mathematical functions. Simpler math
		functions are in math/amath, and more complex functions are in other files in
		the math library.
	
	<FLAG Component>
*/

//=============================================================================

// PURPOSE: Determine whether 2 floating-point numbers have the same sign.
// PARAMS:
//    a - A floating-point number.
//    b - A floating-point number.
// RETURN: True if they have the same sign, false otherwise.
bool SameSign (float a, float b);

// PURPOSE: Find the reciprocal of a floating-point number, but avoid dividing by zero.
// PARAMS:
//   x - A floating-point number.
//	zeroInverse - optional return value to use when x is zero
// RETURNS: x inverted (i.e. 1 / x).  If x is zero, this returns the specified zero inverse (default is FLT_MAX).
// NOTES:
//   Since the IEEE 754 floating-point format has well-defined behavior for dividing
//   by zero, the only differences I see between this function and just dividing by zero
//   are (1) this avoids throwing any floating-point exceptions, should those be turned
//   on, and (2) it'll return +FLT_MAX if given -0, whereas dividing 1 by -0 gives -FLT_MAX.
float InvertSafe (float x, float zeroInverse=FLT_MAX);

// PURPOSE: Divide two floating-point numbers, but avoid dividing by zero.
// PARAMS:
//   y - The numerator in the division.
//   x - The denominator in the division.
// RETURNS: y divided by x.  If x is zero, this returns FLT_MAX, -FLT_MAX or 1 depending on the
//			sign of y.
// NOTES:
//   Since the IEEE 754 floating-point format has well-defined behavior for dividing by
//   zero, the only difference I see between this function and just dividing by zero, is
//   that this avoids throwing any floating-point exceptions, should those be turned on.
float DivideSafe (float y, float x);

// PURPOSE: determine if a float is withing the specified tolerance of Zero
// PARAMS:
//   number - the value to check for near zero status
//   tolerance - How close to zero do we need to be?
// RETURNS: true if float is with specified tolerance of zero, otherwise false.
bool IsNearZero (float number, float tolerance=SMALL_FLOAT);

// PURPOSE: determine if two floats are equal withing the specified tolerance
// PARAMS:
//   x - first float
//   y - second float
//   tolerance - How close to equal do they need to be?
// RETURNS: true if floats are equal within specified tolerance, otherwise false.
// NOTES: Using comparison from "Physics for Games Programmers: Numerical Robustness"
//		  GDC slides by Christer Ericson (SCE):
//		  Abs(x - y) <= tolerance * Max(1.0f, Abs(x), Abs(y))
bool AreNearlyEqual(float x, float y, float tolerance=SMALL_FLOAT);

// PURPOSE: float comparison with specified tolerance
// PARAMS:
//   x - first float
//   y - second float
//   tolerance - How close to equal do they need to be?
//		  Abs(x - y) <= tolerance
bool IsClose(float x, float y, float tolerance=SMALL_FLOAT);

// PURPOSE: Find the cube root of the given number.
// PARAMS:
//   number - the floating point number of which to find the cube root
// RETURNS: the cube root of the given number
// NOTES:	this doesn't do anything special for a cube root, it just calls powf(0.0.333333333333f)
float CubeRoot (float number);

// PURPOSE: Finds the inverse tangent of the value (y/x) in the range -pi to +pi.
// PARAMS:
//	y - the sine of the angle of which to find the inverse tangent
//	x -	the cosine of the angle of which to find the inverse tangent
// RETURNS:	the angle that has a sine and cosine equal to the arguments
float ArcTangent (float y, float x);


// PURPOSE: Find the local normalized flat coordinates (s,t) of a point in a quad.
// PARAMS:
//   x,z - The point whose normalized flat coordinates need to be calculated.
//   ax,az - The corner of the quadrilateral corresponding to the flat coordinates (0,0).
//   bx,bz - The corner of the quadrilateral corresponding to the flat coordinates (1,0).
//   cx,cz - The corner of the quadrilateral corresponding to the flat coordinates (0,1).
//   dx,dz - The corner of the quadrilateral corresponding to the flat coordinates (1,1).
//   s,t - Output parameters, holding the normalized flat version of x,z.
// RETURNS: true if a flat coordinate in the given quad is found, false if not
bool FindInverseBilinear (float x, float z, float ax, float az, float bx, float bz, float cx, float cz,
							float dx, float dz, float &s, float &t);

// PURPOSE: Determine the index of the input parameter with maximum value.
// PARAMS:
//   a - the first of two input values
//   b - the second of two input values
// RETURNS: The index of the biggest value (either 0 or 1).
// NOTES:
//   If the two values are equal, the one that appears first in the argument list is chosen.
template <typename _T>
inline int MaximumIndex (_T a, _T b)
{
	return (a >= b) ? 0 : 1;
}

// PURPOSE: Determine the index of the input parameter with maximum value.
// PARAMS:
//   a - the first of three input values
//   b - the second of three input values
//   c - the third of three input values
// RETURNS: The index of the biggest value (0, 1, or 2).
// NOTES:
//   If any two values are equal, the one that appears earlier in the argument list is chosen.
template <typename _T>
inline int MaximumIndex (_T a, _T b, _T c)
{
	return (a >= b) ? ((a >= c) ? 0 : 2) : ((b >= c) ? 1 : 2);
}

// PURPOSE: Determine the index of the input parameter with maximum value.
// PARAMS:
//   a - the first of four input values
//   b - the second of four input values
//   c - the third of four input values
//   d - the fourth of four input values
// RETURNS: The index of the biggest value (0, 1, 2, or 3).
// NOTES:
//   If any two values are equal, the one that appears earlier in the argument list is chosen.
template <typename _T>
inline int MaximumIndex (_T a, _T b, _T c, _T d)
{
	return (a >= b) ? ((a >= c) ? (((a >= d) ? 0 : 3)) : (((c >= d) ? 2 : 3))) : ((b >= c) ? ((b >= d) ? 1 : 3) : ((c >= d) ? 2 : 3));
}

// PURPOSE: Determine the index of the input parameter with  minimum value.
// PARAMS
//   a - the first of two input values
//   b - the second of two input values
// RETURNS: The index of the smallest value (either 0 or 1).
// NOTES
//   If the two values are equal, the one that appears first in the argument list is chosen.
template <typename _T>
inline int MinimumIndex (_T a, _T b)
{
	return (a <= b) ? 0 : 1;
}

// PURPOSE: Determine the index of the input parameter with  minimum value.
// PARAMS:
//   a - the first of three input values
//   b - the second of three input values
//   c - the third of three input values
// RETURNS: The index of the smallest value (0, 1, or 2).
// NOTES
///   If any two values are equal, the one that appears earlier in the argument list is chosen.
template <typename _T>
inline int MinimumIndex (_T a, _T b, _T c)
{
	return (a <= b) ? ((a <= c) ? 0 : 2) : ((b <= c) ? 1 : 2);
}

// PURPOSE: Determine the index of the input parameter with  minimum value.
// PARAMS:
//   a - the first of four input values
//   b - the second of four input values
//   c - the third of four input values
//   d - the fourth of four input values
// RETURNS: The index of the smallest value (0, 1, 2, or 3).
// NOTES:
//   If any two values are equal, the one that appears earlier in the argument list is chosen.
template <typename _T>
inline int MinimumIndex (_T a, _T b, _T c, _T d)
{
	return (a <= b) ? ((a <= c) ? (((a <= d) ? 0 : 3)) : (((c <= d) ? 2 : 3))) : ((b <= c) ? ((b <= d) ? 1 : 3) : ((c <= d) ? 2 : 3));
}

// PURPOSE: Is value inside the inclusive range provided.
// PARAMS:
//   t - value to test
//   a - lower boundary of the range
//   b - upper boundary of the range
// RETURNS: true - if value greater than equal to lower boundary and less than equal to upper boundary
template <typename _T>
inline bool InRange (_T t, _T a, _T b)
{
	return (t >= a) && (t <= b);
}

// PURPOSE: Is value outside the inclusive range provided.
// PARAMS:
//   t - value to test
//   a - lower boundary of the range
//   b - upper boundary of the range
// RETURNS: true - if value is less than lower boundary or greater than upper boundary
template <typename _T>
inline bool NotInRange (_T t, _T a, _T b)
{
	return (t < a) || (t > b);
}

// PURPOSE: Determine the next power of two larger than the input value.
// PARAMS:
//   x - The value that we want a larger power of two from
// RETURNS: next greater power of two, even if the input is a power of two - don't pass in 2^31...
// NOTES: This returns a larger power of two even if the input is already a power of two (subtract one
//        if you want only a larger power if it's not already a power of two).
template <typename _T>
inline _T GetNextPow2 (_T x)
{
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x++;
	
	return x;
}

// PURPOSE: Align value to the provided power of 2 alignment
// PARAMS:
//   value - The value that we want to align
//   align - The alignment the value needs to respect
// RETURNS: The aligned value
inline size_t AlignPow2(size_t value, size_t align)
{
	return (value+align-1)&(~(align-1));
}

inline void* AlignPow2(void* const ptr, size_t align)
{
	const size_t mask = ~(static_cast<size_t>(align) - 1);
	return reinterpret_cast<void*>((reinterpret_cast<size_t>(ptr) + (align - 1)) & mask);
}

// PURPOSE:	Rounding integers up or down to powers of two. Good for address alignments.
// PARAMS:
//	x -	the integer to round
// RETURN:	the smallest integer multiple of N that is equal to or greater than the given integer
// NOTES:	this is used for memory alignment, N should be a power of 2
template <int N, typename _I32>
__forceinline _I32 RoundUp( _I32 x )
{
	// Make sure that N is a power of 2.
	FastAssert( CountOnBits((u32)N) == 1 );

	return ( x + N - 1 ) & ~(N - 1);
}
template <int N, typename _I32>
__forceinline _I32 RoundDown( _I32 x )
{
	// Make sure that N is a power of 2.
	FastAssert( CountOnBits((u32)N) == 1 );

	return x & ~(N-1);
}

//=============================================================================
// implementations

inline bool SameSign (float a, float b)
{
	return (a*b>=0.0f);
}


inline float InvertSafe (float x, float zeroInverse)
{
	return x!=0.0f ? 1.0f/x : zeroInverse;
}


inline float DivideSafe (float y, float x)
{
	// if x !=0.0f return (y/x)
	// else if y > 0.0f return FLT_MAX (y/0)
	// else if y < 0.0f return -FLT_MAX (y/0)
	// else x==0.0f and y==0.0f so return 1.0f (0/0)
	return x!=0.0f ? y/x : (y!=0.0f ? Selectf(y,FLT_MAX,-FLT_MAX) : 1.0f);
}

inline bool IsNearZero (float number, float tolerance)
{
	return Abs(number)<tolerance;
}

inline bool AreNearlyEqual(float x, float y, float tolerance)
{
	return (Abs(x - y) <= tolerance * Max(1.0f, Abs(x), Abs(y)));
}

inline bool IsClose(float x, float y, float tolerance)
{
	return (Abs(x - y) <= tolerance);
}

} // namespace rage

#endif
