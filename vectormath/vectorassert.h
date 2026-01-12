// 
// vectormath/vectorassert.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTORMATH_VECTORASSERT_H 
#define VECTORMATH_VECTORASSERT_H 

namespace rage {

	__forceinline void FastAssertMagnitude(rage::ScalarV_In x, rage::ScalarV_In magnitude) { FastAssert(IsLessThanAll(Abs(x), magnitude)); }
	__forceinline void FastAssertMagnitude(rage::Vec2V_In x, rage::ScalarV_In magnitude) { FastAssert(IsLessThanAll(Abs(x), rage::Vec2V(magnitude))); }
	__forceinline void FastAssertMagnitude(rage::Vec3V_In x, rage::ScalarV_In magnitude) { FastAssert(IsLessThanAll(Abs(x), rage::Vec3V(magnitude))); }
	__forceinline void FastAssertMagnitude(rage::Vec4V_In x, rage::ScalarV_In magnitude) { FastAssert(IsLessThanAll(Abs(x), rage::Vec4V(magnitude))); }

	__forceinline void FastAssertMagnitude(rage::ScalarV_In x, float magnitude) { FastAssert(fabsf(x.Getf()) < magnitude); }
	__forceinline void FastAssertMagnitude(rage::Vec2V_In x, float magnitude) { FastAssert(IsLessThanAll(Abs(x), rage::Vec2VFromF32(magnitude))); }
	__forceinline void FastAssertMagnitude(rage::Vec3V_In x, float magnitude) { FastAssert(IsLessThanAll(Abs(x), rage::Vec3VFromF32(magnitude))); }
	__forceinline void FastAssertMagnitude(rage::Vec4V_In x, float magnitude) { FastAssert(IsLessThanAll(Abs(x), rage::Vec4VFromF32(magnitude))); }

	__forceinline void FastAssertMagnitude(float x, float magnitude) { FastAssert(fabsf(x) < magnitude); }
	__forceinline void FastAssertMagnitude(const rage::Vector3& v, float magnitude) { FastAssertMagnitude(RCC_VEC3V(v), magnitude); }

#if HACK_RDR2
	__forceinline void AssertMagnitude(rage::ScalarV_In x, rage::ScalarV_In magnitude) { Assert(IsLessThanAll(Abs(x), magnitude)); }
	__forceinline void AssertMagnitude(rage::Vec2V_In x, rage::ScalarV_In magnitude) { Assert(IsLessThanAll(Abs(x), rage::Vec2V(magnitude))); }
	__forceinline void AssertMagnitude(rage::Vec3V_In x, rage::ScalarV_In magnitude) { Assert(IsLessThanAll(Abs(x), rage::Vec3V(magnitude))); }
	__forceinline void AssertMagnitude(rage::Vec4V_In x, rage::ScalarV_In magnitude) { Assert(IsLessThanAll(Abs(x), rage::Vec4V(magnitude))); }

	__forceinline void AssertMagnitude(rage::ScalarV_In x, float magnitude) { Assert(fabsf(x.Getf()) < magnitude); }
	__forceinline void AssertMagnitude(rage::Vec2V_In x, float magnitude) { Assert(IsLessThanAll(Abs(x), rage::Vec2VFromF32(magnitude))); }
	__forceinline void AssertMagnitude(rage::Vec3V_In x, float magnitude) { Assert(IsLessThanAll(Abs(x), rage::Vec3VFromF32(magnitude))); }
	__forceinline void AssertMagnitude(rage::Vec4V_In x, float magnitude) { Assert(IsLessThanAll(Abs(x), rage::Vec4VFromF32(magnitude))); }

	__forceinline void AssertMagnitude(float x, float magnitude) { Assert(fabsf(x) < magnitude); }
	__forceinline void AssertMagnitude(const rage::Vector3& v, float magnitude) { AssertMagnitude(RCC_VEC3V(v), magnitude); }
#endif

} // namespace rage

#endif // VECTORMATH_VECTORASSERT_H 
