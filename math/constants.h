//
// math/constants.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// PURPOSE: This file contains common math constants that are not contained
// in math/amath.h.

#ifndef MATH_CONSTANTS_H
#define MATH_CONSTANTS_H


//=============================================================================
// Floating-point precision related constants.

// PURPOSE: Two small values that are used to determine if numbers are large enough to be considered non-zero.
#define	SMALL_FLOAT			1.0e-6f
#define VERY_SMALL_FLOAT	1.0e-12f

// PURPOSE: A large value that is small enough to be useful in calculations.
// NOTE: Chosen so that the determinant of a 3x3 matrix of these does not exceed FLT_MAX.
#define	LARGE_FLOAT			1.0e8f


//=============================================================================
// Pre-calculated values

// PURPOSE: The square root of two over two
#define SQRT2DIV2		0.707106781186547524400844362104849f

// PURPOSE: The square root of three and its inverse
#define SQRT3			1.73205080757f
#define	SQRT3INVERSE	0.57735026919f

// PURPOSE: The cosine and sine of one eighth Pi (22.5 degrees)
#define	COSPIDIV8		0.923879532511286
#define	SINPIDIV8		0.382683432365089

// just to make checkheaders succeed
namespace rage {
}

#endif // MATH_CONSTANTS_H
