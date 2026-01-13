//
// system/floattoint.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_FLOATTOINT_H
#define SYSTEM_FLOATTOINT_H

#define F2I_SIGN_BIT(x)			((x < 0.0f) ? 0x80000000 : 0x00000000)

#define F2I_FLOAT_ABS(x)		((x < 0.0f) ? (-(x)) : (x))

// F2I_TWO_TO_POWER_TWO_N = 2^(2^N)
// Hand-coded rather than using general expressions to avoid overflowing
// preprocessor macro limit for complex expressions.
#define F2I_TWO_TO_POWER_TWO_0		2.0f		// 2^1 = 2
#define F2I_TWO_TO_POWER_TWO_1		4.0f		// 2^2 = 4
#define F2I_TWO_TO_POWER_TWO_2		16.0f		// 2^4 = 16
#define F2I_TWO_TO_POWER_TWO_3		256.0f		// 2^8 = 256
#define F2I_TWO_TO_POWER_TWO_4		65536.0f	// 2^16 = 64k
#define F2I_TWO_TO_POWER_TWO_5		(F2I_TWO_TO_POWER_TWO_4 * F2I_TWO_TO_POWER_TWO_4)	// 2^32 = 4G
#define F2I_TWO_TO_POWER_TWO_6		(F2I_TWO_TO_POWER_TWO_5 * F2I_TWO_TO_POWER_TWO_5)	// 2^64

#define F2I_X(x, p)		((p + 127) << 23) + (int)(((x) - 1.0f) * (1 << 23))	// combine exponent and mantissa

#define F2I_0(x, p)		((x >= F2I_TWO_TO_POWER_TWO_0) ? (F2I_X(x / F2I_TWO_TO_POWER_TWO_0, (p + 1)))   :   F2I_X(x, p))
#define F2I_1(x, p)		((x >= F2I_TWO_TO_POWER_TWO_1) ? (F2I_0(x / F2I_TWO_TO_POWER_TWO_1, (p + 2)))   :   F2I_0(x, p))
#define F2I_2(x, p)		((x >= F2I_TWO_TO_POWER_TWO_2) ? (F2I_1(x / F2I_TWO_TO_POWER_TWO_2, (p + 4)))   :   F2I_1(x, p))
#define F2I_3(x, p)		((x >= F2I_TWO_TO_POWER_TWO_3) ? (F2I_2(x / F2I_TWO_TO_POWER_TWO_3, (p + 8)))   :   F2I_2(x, p))
#define F2I_4(x, p)		((x >= F2I_TWO_TO_POWER_TWO_4) ? (F2I_3(x / F2I_TWO_TO_POWER_TWO_4, (p + 16)))  :   F2I_3(x, p))
#define F2I_5(x, p)		((x >= F2I_TWO_TO_POWER_TWO_5) ? (F2I_4(x / F2I_TWO_TO_POWER_TWO_5, (p + 32)))  :   F2I_4(x, p))
#define F2I_6(x, p)		((x >= F2I_TWO_TO_POWER_TWO_6) ? (F2I_5(x / F2I_TWO_TO_POWER_TWO_6, (p + 64)))  :   F2I_5(x, p))

#define F2I_CHECK_SMALL(x, p)			((x >= 1.0f) ? (F2I_6(x, p)) : 0x00000000)
#define F2I_POSITIVE(x)	((x >= 1.0f) ? F2I_6(x, 0) : F2I_CHECK_SMALL(x * F2I_TWO_TO_POWER_TWO_6, -64))
#define F2I_NON_ZERO(x)	(F2I_SIGN_BIT(x) | F2I_POSITIVE(F2I_FLOAT_ABS(x)))


// Converts a float to its IEEE 754 32-bit representation.
// Example:
// FLOAT_TO_INT(0.0f) == 0x00000000
// FLOAT_TO_INT(1.0f) == 0x3f800000
// FLOAT_TO_INT(1.5f) == 0x3fc00000
// FLOAT_TO_INT(2.0f) == 0x40000000
// 
// FLOAT_TO_INT(19.138973f) == 0x41991c9e
// FLOAT_TO_INT(-0.397812f) == 0xbecbae0f
// 
// Like a reinterpret_cast<> except that the expression can be used for compile-time constants.
// 
// If x is very small (|x| < 2^-64) then zero is returned. 
// 
// NOTE: does not support infinities and NaNs.
// NOTE: will not distinguish between positive and negative zeroes;
// i.e. generates 0x00000000 for -0.0f; should generate 0x80000000.

#define _FloatToInt(x) ((x == 0.0f) ? 0x00000000 : F2I_NON_ZERO(x))

#if __WIN32
#define FLOAT_TO_INT(x) \
	__pragma(warning(disable : 4307)) \
	__pragma(warning(disable : 4056)) \
	_FloatToInt(x) \
	__pragma(warning(default : 4056)) \
	__pragma(warning(default : 4307))
#else
#define FLOAT_TO_INT(x) _FloatToInt(x)
#endif // __WIN32

#endif // SYSTEM_FLOATTOINT_H
