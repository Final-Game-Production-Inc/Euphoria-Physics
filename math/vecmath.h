//
// math/amath.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

/* PURPOSE
	vecmath provides basic math functions which use some vectorized code
*/

#ifndef MATH_VECMATH_H
#define MATH_VECMATH_H

#include "channel.h"

#include "math/amath.h"
#include "vector/vector3.h"
#include "vectormath/classes.h"

#include <float.h>
#include <stdlib.h>

#if __XENON
// not sure how else to get this function available, without including xnamath.h (and xtl.h)
extern __vector4        XMVectorPowEst(__vector4 V1, __vector4 V2);
#endif

namespace rage {

// PURPOSE: computes the value of x to the power of y
inline float Powf(const float x, const float y)
{
#if __XENON && __OPTIMIZED
	__vector4 v1,v2;
	v1 = __lvlx(&x,0);
	v2 = __lvlx(&y,0);
	v1 = XMVectorPowEst(v1,v2);
	return v1.v[0];
#elif __PPU				// SPU library function should be fast enough SNC FIXME!!!!!
	Vector3 v1,v2;
	v1 = (__vector4)__lvlx(&x,0);
	v2 = (__vector4)__lvlx(&y,0);
	v1 = (Vector3)::powf4fast(v1,v2);
	return v1.x;
#else
	return powf(x,y);		//stdmath.h?
#endif
}


	//
	// PURPOSE
	//  A fast log10 approximation that's good enough for audio use (accurate to ~0.03 dBs.)
	// PARAMS
	//	linear - The linear value to be converted.
	// RETURNS
	//	The result of the conversion.
	//
inline f32 Log10(const f32 linear)
{
#if __XENON || __PS3
	Vector3 vec = Vector3(linear, 0.0f, 0.0f);
	vec.Log10();
	return vec.x;
#else

	/*
	// 4th order coefficients
	const f32 g_x0CoEff = -2.153620718f;	
	const f32 g_x1CoEff = 3.047884161f;	
	const f32 g_x2CoEff = -1.051875031f;	
	const f32 g_x3CoEff = 0.1582487046f;	
	*/

	// 3rd order coefficients
	const f32 x0CoEff = -1.674903474f;	
	const f32 x1CoEff = 2.024681f;	
	const f32 x2CoEff = -0.3448476634f;	

	// 8-bit accuracy - seems to give a maximum error of about 0.03dBs, (with *20) which is absolutely fine.
	// Use commented out 4th order co-effs above for 10bit accuracy.
	float logResult, square;

	logResult = (float)((*(unsigned*)&linear) >> 23) - 127 + x0CoEff;
	*(unsigned*)&linear = (*(unsigned*)&linear & 0x007FFFFF) | 0x3F800000;
	square = linear;

	logResult += square * x1CoEff;
	square *= linear;
	logResult += square * x2CoEff;

	// That's all in base2, turn it into base10.
	logResult = logResult * 0.30103f; // log10(2);

	return logResult;
#endif
}

template <typename T> __forceinline T SinApprox(const T WIN32PC_ONLY(&) x) // super-fast quadratic approx to Sin(x), accurate to within +/-0.056
{
	const T t = x*T(V_ONE_OVER_PI)*T(V_HALF);
	const T a = SubtractScaled(T(V_ONE), T(V_TWO), t - RoundToNearestIntNegInf(t));
	const T b = T(V_ONE) - Abs(a);
	const T y = T(V_FOUR)*a*b;
	return y;
}

template <> __forceinline float SinApprox<float>(const float WIN32PC_ONLY(&) x)
{
	const float t = x*0.5f/PI;
	const float a = 1.0f - 2.0f*(t - rage::FloorfFast(t));
	const float b = 1.0f - Abs<float>(a);
	const float y = 4.0f*a*b;
	return y;
}

template <typename T> __forceinline T CosApprox(const T WIN32PC_ONLY(&) x) // super-fast quadratic approx to Cos(x), accurate to within +/-0.056
{
	const T t = AddScaled(T(V_QUARTER), x, T(V_ONE_OVER_PI)*T(V_HALF));
	const T a = SubtractScaled(T(V_ONE), T(V_TWO), t - RoundToNearestIntNegInf(t));
	const T b = T(V_ONE) - Abs(a);
	const T y = T(V_FOUR)*a*b;
	return y;
}

template <> __forceinline float CosApprox<float>(const float WIN32PC_ONLY(&) x)
{
	const float t = 0.25f + x*0.5f/PI;
	const float a = 1.0f - 2.0f*(t - rage::FloorfFast(t));
	const float b = 1.0f - Abs<float>(a);
	const float y = 4.0f*a*b;
	return y;
}

__forceinline Vec2V_Out SinCosApprox(ScalarV_In x)
{
	return SinApprox(Vec2V(x) + Vec2VConstant<0, FLOAT_TO_INT(PI/2.0f)>());
}

// same as __powapprox in common.fxh
template <typename T> __forceinline T PowApprox(const T WIN32PC_ONLY(&) a, const T WIN32PC_ONLY(&) b)
{
	return a/AddScaled(b, a, T(V_ONE) - b);
}

template <> __forceinline float PowApprox<float>(const float WIN32PC_ONLY(&) a, const float WIN32PC_ONLY(&) b)
{
	return a/(b + a*(1.0f - b));
}

// same as __powapproxinv in common.fxh
template <typename T> __forceinline T PowApproxInv(const T WIN32PC_ONLY(&) a, const T WIN32PC_ONLY(&) b)
{
	return a*b/AddScaled(T(V_ONE), a, b - T(V_ONE));
}

template <> __forceinline float PowApproxInv<float>(const float WIN32PC_ONLY(&) a, const float WIN32PC_ONLY(&) b)
{
	return a*b/(1.0f + a*(b - 1.0f));
}

} // namespace rage

#endif
