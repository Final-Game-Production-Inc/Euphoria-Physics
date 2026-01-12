// ======================
// math/vecrand.h
// (c) 2011 RockstarNorth
// ======================

#ifndef _MATH_VECRAND_H_
#define _MATH_VECRAND_H_

#include "vectormath/classes.h"

namespace rage {

// ================================================================================================
// http://en.wikipedia.org/wiki/Xorshift
// http://www.jstatsoft.org/v08/i14/paper
// XorShift advantages:
// - operations trivially vectorisable on all platforms
// - can compute skip-ahead matrix for any increment
// - fast on all platforms (no LHS stalls, no 32-bit integer multiply)

namespace Vec {

__forceinline Vector_4V_Out V4RandInit();
__forceinline Vector_4V_Out V4Rand(Vector_4V_InOut seed);

} // namespace Vec

class mthVecRand
{
public:
	mthVecRand()
	{
		m_seed = Vec::V4RandInit();
	}

	// returns values in range [0,1]
	__forceinline ScalarV_Out GetScalarV() { return ScalarV(Vec::V4SplatX(Vec::V4Rand(m_seed))); }
	__forceinline Vec2V_Out   GetVec2V  () { return Vec2V  (Vec::V4Rand(m_seed)); }
	__forceinline Vec3V_Out   GetVec3V  () { return Vec3V  (Vec::V4Rand(m_seed)); }
	__forceinline Vec4V_Out   GetVec4V  () { return Vec4V  (Vec::V4Rand(m_seed)); }

	// returns values in range [0,maxVal]
	__forceinline ScalarV_Out GetRangedScalarV(ScalarV_In maxVal) { return Scale(maxVal, GetScalarV()); }
	__forceinline Vec2V_Out   GetRangedVec2V  (Vec2V_In   maxVal) { return Scale(maxVal, GetVec2V  ()); }
	__forceinline Vec3V_Out   GetRangedVec3V  (Vec3V_In   maxVal) { return Scale(maxVal, GetVec3V  ()); }
	__forceinline Vec4V_Out   GetRangedVec4V  (Vec4V_In   maxVal) { return Scale(maxVal, GetVec4V  ()); }

	// returns values in range [minVal,maxVal]
	__forceinline ScalarV_Out GetRangedScalarV(ScalarV_In minVal, ScalarV_In maxVal) { return AddScaled(minVal, Subtract(maxVal, minVal), GetScalarV()); }
	__forceinline Vec2V_Out   GetRangedVec2V  (Vec2V_In   minVal, Vec2V_In   maxVal) { return AddScaled(minVal, Subtract(maxVal, minVal), GetVec2V  ()); }
	__forceinline Vec3V_Out   GetRangedVec3V  (Vec3V_In   minVal, Vec3V_In   maxVal) { return AddScaled(minVal, Subtract(maxVal, minVal), GetVec3V  ()); }
	__forceinline Vec4V_Out   GetRangedVec4V  (Vec4V_In   minVal, Vec4V_In   maxVal) { return AddScaled(minVal, Subtract(maxVal, minVal), GetVec4V  ()); }

	// returns values in range [-1,+1]
	__forceinline ScalarV_Out GetSignedScalarV() { return AddScaled(ScalarV(V_NEGONE), ScalarV(V_TWO), GetScalarV()); }
	__forceinline Vec2V_Out   GetSignedVec2V  () { return AddScaled(Vec2V  (V_NEGONE), Vec2V  (V_TWO), GetVec2V  ()); }
	__forceinline Vec3V_Out   GetSignedVec3V  () { return AddScaled(Vec3V  (V_NEGONE), Vec3V  (V_TWO), GetVec3V  ()); }
	__forceinline Vec4V_Out   GetSignedVec4V  () { return AddScaled(Vec4V  (V_NEGONE), Vec4V  (V_TWO), GetVec4V  ()); }

	// returns values in range [-maxVal,+maxVal]
	__forceinline ScalarV_Out GetSignedRangedScalarV(ScalarV_In maxVal) { return Scale(maxVal, GetSignedScalarV()); }
	__forceinline Vec2V_Out   GetSignedRangedVec2V  (Vec2V_In   maxVal) { return Scale(maxVal, GetSignedVec2V  ()); }
	__forceinline Vec3V_Out   GetSignedRangedVec3V  (Vec3V_In   maxVal) { return Scale(maxVal, GetSignedVec3V  ()); }
	__forceinline Vec4V_Out   GetSignedRangedVec4V  (Vec4V_In   maxVal) { return Scale(maxVal, GetSignedVec4V  ()); }

	__forceinline void SetSeed(Vec4V_In seed) { m_seed = seed.GetIntrin128(); }

private:
	Vec::Vector_4V m_seed;
};

// ================================================================================================
// utilities

template <typename T, int n> static __forceinline T XorL(T x);
template <typename T, int n> static __forceinline T XorR(T x);
template <typename T, int n> static __forceinline T InvertXorL(T x);
template <typename T, int n> static __forceinline T InvertXorR(T x);

template <typename T, int a, int b, int c> static __forceinline T XorShiftLRL(T x);
template <typename T, int a, int b, int c> static __forceinline T InvertXorShiftLRL(T x);

// reverse the bits used to represent entity id's in the render target - this massively improves the contrast when viewing the target in testbed maps
__forceinline u32 ReverseBits32(u32 a);
__forceinline u32 ScrambleBits32(u32 a);
__forceinline u32 UnscrambleBits32(u32 a);

// ================================================================================================
// ================================================================================================
// ================================================================================================

#if __DEV

namespace rng_util {

void _test_XorShiftRLR31SkipAhead();
void _test_XorShiftLRL32SkipAhead();

} // namespace rng_util

#endif // __DEV

} // namespace rage

#include "vecrand.inl"

#endif	// _MATH_VECRAND_H_
