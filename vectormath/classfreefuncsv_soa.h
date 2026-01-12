#ifndef VECTORMATH_CLASSFREEFUNCSV_SOA_H
#define VECTORMATH_CLASSFREEFUNCSV_SOA_H

namespace rage
{

	//============================================================================
	// Utility functions

	// USES VECTOR INSTRUCTIONS ONLY
	SoA_ScalarV_Out SoA_ScalarVFromF32(const float& );

	//============================================================================
	// Magnitude

	SoA_ScalarV_Out Abs(SoA_ScalarV_In inVect);
	void Abs(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect);
	void Abs(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect);
	void Abs(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect);

	SoA_ScalarV_Out Sqrt(SoA_ScalarV_In inVect);
	void Sqrt(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect);
	void Sqrt(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect);
	void Sqrt(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect);
	SoA_ScalarV_Out SqrtSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::ZERO));
	void SqrtSafe(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::ZERO));
	void SqrtSafe(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::ZERO));
	void SqrtSafe(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::ZERO));
	SoA_ScalarV_Out SqrtFast(SoA_ScalarV_In inVect);
	void SqrtFast(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect);
	void SqrtFast(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect);
	void SqrtFast(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect);
	SoA_ScalarV_Out SqrtFastSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::ZERO));
	void SqrtFastSafe(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::ZERO));
	void SqrtFastSafe(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::ZERO));
	void SqrtFastSafe(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::ZERO));

	SoA_ScalarV_Out InvSqrt(SoA_ScalarV_In inVect);
	void InvSqrt(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect);
	void InvSqrt(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect);
	void InvSqrt(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect);
	SoA_ScalarV_Out InvSqrtSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvSqrtSafe(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvSqrtSafe(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvSqrtSafe(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvSqrtFast(SoA_ScalarV_In inVect);
	void InvSqrtFast(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect);
	void InvSqrtFast(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect);
	void InvSqrtFast(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect);
	SoA_ScalarV_Out InvSqrtFastSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvSqrtFastSafe(SoA_Vec2V_InOut outVect, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvSqrtFastSafe(SoA_Vec3V_InOut outVect, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvSqrtFastSafe(SoA_Vec4V_InOut outVect, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));

	SoA_ScalarV_Out Mag(SoA_Vec2V_In inVect);
	SoA_ScalarV_Out Mag(SoA_Vec3V_In inVect);
	SoA_ScalarV_Out Mag(SoA_Vec4V_In inVect);
	SoA_ScalarV_Out MagFast(SoA_Vec2V_In inVect);
	SoA_ScalarV_Out MagFast(SoA_Vec3V_In inVect);
	SoA_ScalarV_Out MagFast(SoA_Vec4V_In inVect);
	SoA_ScalarV_Out MagSquared(SoA_Vec2V_In inVect);
	SoA_ScalarV_Out MagSquared(SoA_Vec3V_In inVect);
	SoA_ScalarV_Out MagSquared(SoA_Vec4V_In inVect);

	SoA_ScalarV_Out InvMag(SoA_Vec2V_In inVect);
	SoA_ScalarV_Out InvMag(SoA_Vec3V_In inVect);
	SoA_ScalarV_Out InvMag(SoA_Vec4V_In inVect);
	SoA_ScalarV_Out InvMagSafe(SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagSafe(SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagSafe(SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagFast(SoA_Vec2V_In inVect);
	SoA_ScalarV_Out InvMagFast(SoA_Vec3V_In inVect);
	SoA_ScalarV_Out InvMagFast(SoA_Vec4V_In inVect);
	SoA_ScalarV_Out InvMagFastSafe(SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagFastSafe(SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagFastSafe(SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagSquared(SoA_Vec2V_In inVect);
	SoA_ScalarV_Out InvMagSquared(SoA_Vec3V_In inVect);
	SoA_ScalarV_Out InvMagSquared(SoA_Vec4V_In inVect);
	SoA_ScalarV_Out InvMagSquaredSafe(SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagSquaredSafe(SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagSquaredSafe(SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagSquaredFast(SoA_Vec2V_In inVect);
	SoA_ScalarV_Out InvMagSquaredFast(SoA_Vec3V_In inVect);
	SoA_ScalarV_Out InvMagSquaredFast(SoA_Vec4V_In inVect);
	SoA_ScalarV_Out InvMagSquaredFastSafe(SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagSquaredFastSafe(SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvMagSquaredFastSafe(SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));

	void Normalize(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect);
	void Normalize(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect);
	void Normalize(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect);
	void NormalizeSafe(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect);
	void NormalizeSafe(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect);
	void NormalizeSafe(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect);
	void NormalizeFast(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect);
	void NormalizeFast(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect);
	void NormalizeFast(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect);
	void NormalizeFastSafe(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect);
	void NormalizeFastSafe(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect);
	void NormalizeFastSafe(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect);

	SoA_ScalarV_Out Dist(SoA_Vec4V_In a, SoA_Vec4V_In b);
	SoA_ScalarV_Out DistFast(SoA_Vec4V_In a, SoA_Vec4V_In b);
	SoA_ScalarV_Out Dist(SoA_Vec3V_In a, SoA_Vec3V_In b);
	SoA_ScalarV_Out DistFast(SoA_Vec3V_In a, SoA_Vec3V_In b);
	SoA_ScalarV_Out Dist(SoA_Vec2V_In a, SoA_Vec2V_In b);
	SoA_ScalarV_Out DistFast(SoA_Vec2V_In a, SoA_Vec2V_In b);

	SoA_ScalarV_Out InvDist(SoA_Vec4V_In a, SoA_Vec4V_In b);
	SoA_ScalarV_Out InvDistSafe(SoA_Vec4V_In a, SoA_Vec4V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvDistFast(SoA_Vec4V_In a, SoA_Vec4V_In b);
	SoA_ScalarV_Out InvDistFastSafe(SoA_Vec4V_In a, SoA_Vec4V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvDist(SoA_Vec3V_In a, SoA_Vec3V_In b);
	SoA_ScalarV_Out InvDistSafe(SoA_Vec3V_In a, SoA_Vec3V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvDistFast(SoA_Vec3V_In a, SoA_Vec3V_In b);
	SoA_ScalarV_Out InvDistFastSafe(SoA_Vec3V_In a, SoA_Vec3V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvDist(SoA_Vec2V_In a, SoA_Vec2V_In b);
	SoA_ScalarV_Out InvDistSafe(SoA_Vec2V_In a, SoA_Vec2V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvDistFast(SoA_Vec2V_In a, SoA_Vec2V_In b);
	SoA_ScalarV_Out InvDistFastSafe(SoA_Vec2V_In a, SoA_Vec2V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out DistSquared(SoA_Vec4V_In a, SoA_Vec4V_In b);
	SoA_ScalarV_Out DistSquared(SoA_Vec3V_In a, SoA_Vec3V_In b);
	SoA_ScalarV_Out DistSquared(SoA_Vec2V_In a, SoA_Vec2V_In b);

	SoA_ScalarV_Out InvDistSquared(SoA_Vec4V_In a, SoA_Vec4V_In b);
	SoA_ScalarV_Out InvDistSquaredSafe(SoA_Vec4V_In a, SoA_Vec4V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvDistSquaredFast(SoA_Vec4V_In a, SoA_Vec4V_In b);
	SoA_ScalarV_Out InvDistSquaredFastSafe(SoA_Vec4V_In a, SoA_Vec4V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));

	SoA_ScalarV_Out InvDistSquared(SoA_Vec3V_In a, SoA_Vec3V_In b);
	SoA_ScalarV_Out InvDistSquaredSafe(SoA_Vec3V_In a, SoA_Vec3V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvDistSquaredFast(SoA_Vec3V_In a, SoA_Vec3V_In b);
	SoA_ScalarV_Out InvDistSquaredFastSafe(SoA_Vec3V_In a, SoA_Vec3V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));

	SoA_ScalarV_Out InvDistSquared(SoA_Vec2V_In a, SoA_Vec2V_In b);
	SoA_ScalarV_Out InvDistSquaredSafe(SoA_Vec2V_In a, SoA_Vec2V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvDistSquaredFast(SoA_Vec2V_In a, SoA_Vec2V_In b);
	SoA_ScalarV_Out InvDistSquaredFastSafe(SoA_Vec2V_In a, SoA_Vec2V_In b, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));


	//============================================================================
	// Comparison functions.

	// Note: The functions that returned a u32 in AoS, SoA_now return a VecBool1V in SoA. Since we are comparing 2
	// vectors (SoA_but doing 4 sets at a time), SoA_we have four results in the VecBool1V. And each component will have
	// either 0x0 or 0xFFFFFFFF (SoA_not 0x0 or 0x1).

	SoA_VecBool1V_Out IsEqualInt(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2);

	SoA_VecBool1V_Out IsEqualInt(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2);
	SoA_VecBool1V_Out IsEqual(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2);
	SoA_VecBool1V_Out IsClose(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsGreaterThan(SoA_ScalarV_In bigVector, SoA_ScalarV_In smallVector);
	SoA_VecBool1V_Out IsGreaterThanOrEqual(SoA_ScalarV_In bigVector, SoA_ScalarV_In smallVector);
	SoA_VecBool1V_Out IsLessThan(SoA_ScalarV_In smallVector, SoA_ScalarV_In bigVector);
	SoA_VecBool1V_Out IsLessThanOrEqual(SoA_ScalarV_In smallVector, SoA_ScalarV_In bigVector);

	void IsEqualInt(SoA_VecBool2V_InOut outVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2);
	void IsEqualInt(SoA_VecBool3V_InOut outVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2);
	void IsEqualInt(SoA_VecBool4V_InOut outVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2);
	SoA_VecBool1V_Out IsZeroAll(SoA_Vec4V_In inVector);
	SoA_VecBool1V_Out IsZeroNone(SoA_Vec4V_In inVector);
	void IsEqual(SoA_VecBool4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	SoA_VecBool1V_Out IsEqualAll(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	SoA_VecBool1V_Out IsEqualNone(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	void IsEqualInt(SoA_VecBool4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	void IsClose(SoA_VecBool4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseAll(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseNone(SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsGreaterThanAll(SoA_Vec4V_In bigVector, SoA_Vec4V_In smallVector);
	void IsGreaterThan(SoA_VecBool4V_InOut outVec, SoA_Vec4V_In bigVector, SoA_Vec4V_In smallVector);
	void IsGreaterThan(SoA_VecBool3V_InOut outVec, SoA_ScalarV_In bigVector, SoA_Vec3V_In smallVector);
	SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Vec4V_In bigVector, SoA_Vec4V_In smallVector);
	void IsGreaterThanOrEqual(SoA_VecBool4V_InOut outVec, SoA_Vec4V_In bigVector, SoA_Vec4V_In smallVector);
	SoA_VecBool1V_Out IsLessThanAll(SoA_Vec4V_In smallVector, SoA_Vec4V_In bigVector);
	void IsLessThan(SoA_VecBool4V_InOut outVec, SoA_Vec4V_In smallVector, SoA_Vec4V_In bigVector);
	SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Vec4V_In smallVector, SoA_Vec4V_In bigVector);
	void IsLessThanOrEqual(SoA_VecBool4V_InOut outVec, SoA_Vec4V_In smallVector, SoA_Vec4V_In bigVector);
	SoA_VecBool1V_Out IsZeroAll(SoA_Vec3V_In inVector);
	SoA_VecBool1V_Out IsZeroNone(SoA_Vec3V_In inVector);
	SoA_VecBool3V_Out IsEqual(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	SoA_VecBool1V_Out IsEqualAll(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	SoA_VecBool1V_Out IsEqualNone(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	SoA_VecBool3V_Out IsEqualInt(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	SoA_VecBool3V_Out IsClose(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseAll(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseNone(SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsGreaterThanAll(SoA_Vec3V_In bigVector, SoA_Vec3V_In smallVector);
	SoA_VecBool3V_Out IsGreaterThan(SoA_Vec3V_In bigVector, SoA_Vec3V_In smallVector);
	SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Vec3V_In bigVector, SoA_Vec3V_In smallVector);
	SoA_VecBool1V_Out IsGreaterThanOrEqual(SoA_Vec3V_In bigVector, SoA_Vec3V_In smallVector);
	SoA_VecBool1V_Out IsLessThanAll(SoA_Vec3V_In smallVector, SoA_Vec3V_In bigVector);
	SoA_VecBool3V_Out IsLessThan(SoA_Vec3V_In smallVector, SoA_Vec3V_In bigVector);
	SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Vec3V_In smallVector, SoA_Vec3V_In bigVector);
	SoA_VecBool3V_Out IsLessThanOrEqual(SoA_Vec3V_In smallVector, SoA_Vec3V_In bigVector);
	SoA_VecBool1V_Out IsZeroAll(SoA_Vec2V_In inVector);
	SoA_VecBool1V_Out IsZeroNone(SoA_Vec2V_In inVector);
	SoA_VecBool2V_Out IsEqual(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_VecBool1V_Out IsEqualAll(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_VecBool1V_Out IsEqualNone(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_VecBool2V_Out IsEqualInt(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_VecBool2V_Out IsClose(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseAll(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseNone(SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsGreaterThanAll(SoA_Vec2V_In bigVector, SoA_Vec2V_In smallVector);
	SoA_VecBool2V_Out IsGreaterThan(SoA_Vec2V_In bigVector, SoA_Vec2V_In smallVector);
	SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Vec2V_In bigVector, SoA_Vec2V_In smallVector);
	SoA_VecBool2V_Out IsGreaterThanOrEqual(SoA_Vec2V_In bigVector, SoA_Vec2V_In smallVector);
	SoA_VecBool1V_Out IsLessThanAll(SoA_Vec2V_In smallVector, SoA_Vec2V_In bigVector);
	SoA_VecBool2V_Out IsLessThan(SoA_Vec2V_In smallVector, SoA_Vec2V_In bigVector);
	SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Vec2V_In smallVector, SoA_Vec2V_In bigVector);
	SoA_VecBool2V_Out IsLessThanOrEqual(SoA_Vec2V_In smallVector, SoA_Vec2V_In bigVector);
	SoA_VecBool1V_Out IsZeroAll(SoA_QuatV_In inQuat);
	SoA_VecBool1V_Out IsZeroNone(SoA_QuatV_In inQuat);
	SoA_VecBool4V_Out IsEqual(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2);
	SoA_VecBool1V_Out IsEqualAll(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2);
	SoA_VecBool1V_Out IsEqualNone(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2);
	SoA_VecBool4V_Out IsEqualInt(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2);
	SoA_VecBool4V_Out IsClose(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseAll(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseNone(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsGreaterThanAll(SoA_QuatV_In bigQuat, SoA_QuatV_In smallQuat);
	SoA_VecBool4V_Out IsGreaterThan(SoA_QuatV_In bigQuat, SoA_QuatV_In smallQuat);
	SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_QuatV_In bigQuat, SoA_QuatV_In smallQuat);
	SoA_VecBool4V_Out IsGreaterThanOrEqual(SoA_QuatV_In bigQuat, SoA_QuatV_In smallQuat);
	SoA_VecBool1V_Out IsLessThanAll(SoA_QuatV_In smallQuat, SoA_QuatV_In bigQuat);
	SoA_VecBool4V_Out IsLessThan(SoA_QuatV_In smallQuat, SoA_QuatV_In bigQuat);
	SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_QuatV_In smallQuat, SoA_QuatV_In bigQuat);
	SoA_VecBool4V_Out IsLessThanOrEqual(SoA_QuatV_In smallQuat, SoA_QuatV_In bigQuat);
	SoA_VecBool1V_Out IsEqualAll(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2);
	SoA_VecBool1V_Out IsEqualNone(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2);
	SoA_VecBool1V_Out IsGreaterThanAll(SoA_Mat44V_In bigMat, SoA_Mat44V_In smallMat);
	SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Mat44V_In bigMat, SoA_Mat44V_In smallMat);
	SoA_VecBool1V_Out IsLessThanAll(SoA_Mat44V_In smallMat, SoA_Mat44V_In bigMat);
	SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Mat44V_In smallMat, SoA_Mat44V_In bigMat);
	SoA_VecBool1V_Out IsCloseAll(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseNone(SoA_Mat44V_In inMat1, SoA_Mat44V_In inMat2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsEqualAll(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2);
	SoA_VecBool1V_Out IsEqualNone(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2);
	SoA_VecBool1V_Out IsGreaterThanAll(SoA_Mat34V_In bigMat, SoA_Mat34V_In smallMat);
	SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Mat34V_In bigMat, SoA_Mat34V_In smallMat);
	SoA_VecBool1V_Out IsLessThanAll(SoA_Mat34V_In smallMat, SoA_Mat34V_In bigMat);
	SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Mat34V_In smallMat, SoA_Mat34V_In bigMat);
	SoA_VecBool1V_Out IsCloseAll(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseNone(SoA_Mat34V_In inMat1, SoA_Mat34V_In inMat2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsEqualAll(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2);
	SoA_VecBool1V_Out IsEqualNone(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2);
	SoA_VecBool1V_Out IsEqualIntAll(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2);
	SoA_VecBool1V_Out IsEqualIntNone(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2);
	SoA_VecBool1V_Out IsGreaterThanAll(SoA_Mat33V_In bigMat, SoA_Mat33V_In smallMat);
	SoA_VecBool1V_Out IsGreaterThanOrEqualAll(SoA_Mat33V_In bigMat, SoA_Mat33V_In smallMat);
	SoA_VecBool1V_Out IsLessThanAll(SoA_Mat33V_In smallMat, SoA_Mat33V_In bigMat);
	SoA_VecBool1V_Out IsLessThanOrEqualAll(SoA_Mat33V_In smallMat, SoA_Mat33V_In bigMat);
	SoA_VecBool1V_Out IsCloseAll(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2, SoA_ScalarV_In epsValues);
	SoA_VecBool1V_Out IsCloseNone(SoA_Mat33V_In inMat1, SoA_Mat33V_In inMat2, SoA_ScalarV_In epsValues);

	//============================================================================
	// Conversion functions

	template <int exponent>
	void FloatToIntRaw(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVec);
	template <int exponent>
	void IntToFloatRaw(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVec);
	void RoundToNearestInt(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVec);
	void RoundToNearestIntZero(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVec);
	void RoundToNearestIntNegInf(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVec);
	void RoundToNearestIntPosInf(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVec);

	template <int exponent>
	void FloatToIntRaw(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVec);
	template <int exponent>
	void IntToFloatRaw(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVec);
	void RoundToNearestInt(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVec);
	void RoundToNearestIntZero(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVec);
	void RoundToNearestIntNegInf(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVec);
	void RoundToNearestIntPosInf(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVec);

	template <int exponent>
	void FloatToIntRaw(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVec);
	template <int exponent>
	void IntToFloatRaw(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVec);
	void RoundToNearestInt(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVec);
	void RoundToNearestIntZero(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVec);
	void RoundToNearestIntNegInf(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVec);
	void RoundToNearestIntPosInf(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVec);

	//============================================================================
	// Standard quaternion math

	void Conjugate(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat);
	void Normalize(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat);
	void NormalizeSafe(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat);
	void NormalizeFast(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat);
	void NormalizeFastSafe(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat);
	void Invert(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat);
	void InvertSafe(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvertFast(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat);
	void InvertFastSafe(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	// InvertNormInput(SoA_) is fastest if the input is already a unit quat. Else, SoA_Invert(SoA_) is faster
	// than a Normalize(SoA_) followed by a InvertNormInput(SoA_).
	void InvertNormInput(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat);

	SoA_ScalarV_Out Dot(SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2 );
	void Multiply(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2 );
	void PrepareSlerp(SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat1, SoA_QuatV_In inQuat2 );
	void Nlerp(SoA_QuatV_InOut outQuat, SoA_ScalarV_In t, SoA_QuatV_In inNormQuat1, SoA_QuatV_In inNormQuat2 );
	void ToAxisAngle( SoA_Vec3V_InOut outAxis, SoA_ScalarV_InOut outRadians, SoA_QuatV_In inQuat );
	void FromAxisAngle( SoA_QuatV_InOut outQuat, SoA_Vec3V_In inNormAxis, SoA_ScalarV_In inRadians );
	void ScaleAngle( SoA_QuatV_InOut outQuat, SoA_QuatV_In inQuat, SoA_ScalarV_In inRadians );
	void Mat33VFromQuatV( SoA_Mat33V_InOut outMat, SoA_QuatV_In inQuat );
	void QuatVFromEulersXYZ( SoA_QuatV_InOut outQuat, SoA_Vec3V_In radianAngles );
	// TODO: Implement when needed.
	//	void Slerp(SoA_QuatV_InOut outQuat, SoA_ScalarV_In t, SoA_QuatV_In inNormQuat1, SoA_QuatV_In inNormQuat2 );

	//============================================================================
	// Standard algebra

	void Add(SoA_Vec4V_InOut outVec, SoA_Vec4V_In a, SoA_Vec4V_In b );
	void Add(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_Vec3V_In b );
	void Add(SoA_Vec2V_InOut outVec, SoA_Vec2V_In a, SoA_Vec2V_In b );
	SoA_ScalarV_Out Add(SoA_ScalarV_In a, SoA_ScalarV_In b );
	void Subtract(SoA_Vec4V_InOut outVec, SoA_Vec4V_In a, SoA_Vec4V_In b );
	void Subtract(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_Vec3V_In b );
	void Subtract(SoA_Vec2V_InOut outVec, SoA_Vec2V_In a, SoA_Vec2V_In b );
	SoA_ScalarV_Out Subtract(SoA_ScalarV_In a, SoA_ScalarV_In b );

	void Average(SoA_Vec4V_InOut outVec, SoA_Vec4V_In a, SoA_Vec4V_In b );
	void Average(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_Vec3V_In b );
	void Average(SoA_Vec2V_InOut outVec, SoA_Vec2V_In a, SoA_Vec2V_In b );
	SoA_ScalarV_Out Average(SoA_ScalarV_In a, SoA_ScalarV_In b );

	SoA_ScalarV_Out Clamp(SoA_ScalarV_In inVect, SoA_ScalarV_In lowBound, SoA_ScalarV_In highBound );
	void Clamp(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect, SoA_ScalarV_In lowBound, SoA_ScalarV_In highBound );
	void Clamp(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect, SoA_ScalarV_In lowBound, SoA_ScalarV_In highBound );
	void Clamp(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect, SoA_ScalarV_In lowBound, SoA_ScalarV_In highBound );

	void ClampMag(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect, SoA_ScalarV_In minMag, SoA_ScalarV_In maxMag );

	SoA_ScalarV_Out Negate(SoA_ScalarV_In inVect);
	void Negate(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect);
	void Negate(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect);
	void Negate(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect);

	SoA_ScalarV_Out InvertBits(SoA_ScalarV_In inVect);
	void InvertBits(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect);
	void InvertBits(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect);
	void InvertBits(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect);

	SoA_VecBool1V_Out InvertBits(SoA_VecBool1V_In inVect);
	void InvertBits(SoA_VecBool2V_InOut outVec, SoA_VecBool2V_In inVect);
	void InvertBits(SoA_VecBool3V_InOut outVec, SoA_VecBool3V_In inVect);
	void InvertBits(SoA_VecBool4V_InOut outVec, SoA_VecBool4V_In inVect);

	SoA_ScalarV_Out Invert(SoA_ScalarV_In inVect);
	void Invert(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect);
	void Invert(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect);
	void Invert(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect);
	SoA_ScalarV_Out InvertSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvertSafe(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvertSafe(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvertSafe(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	SoA_ScalarV_Out InvertFast(SoA_ScalarV_In inVect);
	void InvertFast(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect);
	void InvertFast(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect);
	void InvertFast(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect);
	SoA_ScalarV_Out InvertFastSafe(SoA_ScalarV_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvertFastSafe(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvertFastSafe(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));
	void InvertFastSafe(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVect, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8));

	// Some integer algebraic ops that are fast in all of SSE/Xenon/PS3.
	void AddInt(SoA_Vec4V_InOut outVec, SoA_Vec4V_In a, SoA_Vec4V_In b );
	void AddInt(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_Vec3V_In b );
	void AddInt(SoA_Vec2V_InOut outVec, SoA_Vec2V_In a, SoA_Vec2V_In b );
	SoA_ScalarV_Out AddInt(SoA_ScalarV_In a, SoA_ScalarV_In b );
	void SubtractInt(SoA_Vec4V_InOut outVec, SoA_Vec4V_In a, SoA_Vec4V_In b );
	void SubtractInt(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_Vec3V_In b );
	void SubtractInt(SoA_Vec2V_InOut outVec, SoA_Vec2V_In a, SoA_Vec2V_In b );
	SoA_ScalarV_Out SubtractInt(SoA_ScalarV_In a, SoA_ScalarV_In b );

	void AddScaled(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toAdd, SoA_Vec4V_In toScaleThenAdd, SoA_Vec4V_In scaleValues );
	void AddScaled(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toAdd, SoA_Vec3V_In toScaleThenAdd, SoA_Vec3V_In scaleValues );
	void AddScaled(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toAdd, SoA_Vec3V_In toScaleThenAdd, SoA_ScalarV_In scaleValue );
	void AddScaled(SoA_Vec2V_InOut outVec, SoA_Vec2V_In toAdd, SoA_Vec2V_In toScaleThenAdd, SoA_Vec2V_In scaleValues );
	SoA_ScalarV_Out AddScaled(SoA_ScalarV_In toAdd, SoA_ScalarV_In toScaleThenAdd, SoA_ScalarV_In scaleValues );
	void SubtractScaled(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toSubtractFrom, SoA_Vec4V_In toScaleThenSubtract, SoA_Vec4V_In scaleValues );
	void SubtractScaled(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toSubtractFrom, SoA_Vec3V_In toScaleThenSubtract, SoA_Vec3V_In scaleValues );
	void SubtractScaled(SoA_Vec2V_InOut outVec, SoA_Vec2V_In toSubtractFrom, SoA_Vec2V_In toScaleThenSubtract, SoA_Vec2V_In scaleValues );
	SoA_ScalarV_Out SubtractScaled(SoA_ScalarV_In toSubtractFrom, SoA_ScalarV_In toScaleThenSubtract, SoA_ScalarV_In scaleValues );

	void Scale(SoA_Vec4V_InOut outVec, SoA_Vec4V_In a, SoA_Vec4V_In b );
	void Scale(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_Vec3V_In b );
	void Scale(SoA_Vec2V_InOut outVec, SoA_Vec2V_In a, SoA_Vec2V_In b );
	SoA_ScalarV_Out Scale(SoA_ScalarV_In a, SoA_ScalarV_In b );

	SoA_ScalarV_Out Dot(SoA_Vec4V_In a, SoA_Vec4V_In b );
	SoA_ScalarV_Out Dot(SoA_Vec3V_In a, SoA_Vec3V_In b );
	SoA_ScalarV_Out Dot(SoA_Vec2V_In a, SoA_Vec2V_In b );

	void Cross(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_Vec3V_In b );
	SoA_ScalarV_Out Cross(SoA_Vec2V_In a, SoA_Vec2V_In b );
	
	void Scale(SoA_Vec4V_InOut outVec, SoA_Vec4V_In a, SoA_ScalarV_In b );
	void Scale(SoA_Vec4V_InOut outVec, SoA_ScalarV_In a, SoA_Vec4V_In b );
	void Scale(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_ScalarV_In b );
	void Scale(SoA_Vec3V_InOut outVec, SoA_ScalarV_In a, SoA_Vec3V_In b );
	void Scale(SoA_Vec2V_InOut outVec, SoA_Vec2V_In a, SoA_ScalarV_In b );
	void Scale(SoA_Vec2V_InOut outVec, SoA_ScalarV_In a, SoA_Vec2V_In b );

	void InvScale(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toScale, SoA_ScalarV_In scaleValue );
	void InvScale(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toScale, SoA_Vec4V_In scaleValues );
	void InvScaleSafe(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleSafe(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toScale, SoA_Vec4V_In scaleValues, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleFast(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toScale, SoA_ScalarV_In scaleValue );
	void InvScaleFast(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toScale, SoA_Vec4V_In scaleValues );
	void InvScaleFastSafe(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleFastSafe(SoA_Vec4V_InOut outVec, SoA_Vec4V_In toScale, SoA_Vec4V_In scaleValues, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScale(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toScale, SoA_ScalarV_In scaleValue );
	void InvScale(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toScale, SoA_Vec3V_In scaleValues );
	void InvScaleSafe(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleSafe(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toScale, SoA_Vec3V_In scaleValues, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleFast(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toScale, SoA_ScalarV_In scaleValue );
	void InvScaleFast(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toScale, SoA_Vec3V_In scaleValues );
	void InvScaleFastSafe(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleFastSafe(SoA_Vec3V_InOut outVec, SoA_Vec3V_In toScale, SoA_Vec3V_In scaleValues, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScale(SoA_Vec2V_InOut outVec, SoA_Vec2V_In toScale, SoA_ScalarV_In scaleValue );
	void InvScale(SoA_Vec2V_InOut outVec, SoA_Vec2V_In toScale, SoA_Vec2V_In scaleValues );
	void InvScaleSafe(SoA_Vec2V_InOut outVec, SoA_Vec2V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleSafe(SoA_Vec2V_InOut outVec, SoA_Vec2V_In toScale, SoA_Vec2V_In scaleValues, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleFast(SoA_Vec2V_InOut outVec, SoA_Vec2V_In toScale, SoA_ScalarV_In scaleValue );
	void InvScaleFast(SoA_Vec2V_InOut outVec, SoA_Vec2V_In toScale, SoA_Vec2V_In scaleValues );
	void InvScaleFastSafe(SoA_Vec2V_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	void InvScaleFastSafe(SoA_Vec2V_In toScale, SoA_Vec2V_In scaleValues, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	SoA_ScalarV_Out InvScale(SoA_ScalarV_In toScale, SoA_ScalarV_In scaleValue );
	SoA_ScalarV_Out InvScaleSafe(SoA_ScalarV_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );
	SoA_ScalarV_Out InvScaleFast(SoA_ScalarV_In toScale, SoA_ScalarV_In scaleValue );
	SoA_ScalarV_Out InvScaleFastSafe(SoA_ScalarV_In toScale, SoA_ScalarV_In scaleValue, SoA_ScalarV_In errValVect = SoA_ScalarV(SoA_ScalarV::FLT_LARGE_8) );

	void Lerp(SoA_Vec4V_InOut outVec, SoA_ScalarV_In tValue, SoA_Vec4V_In vectA, SoA_Vec4V_In vectB );
	void Lerp(SoA_Vec3V_InOut outVec, SoA_ScalarV_In tValue, SoA_Vec3V_In vectA, SoA_Vec3V_In vectB );
	void Lerp(SoA_Vec2V_InOut outVec, SoA_ScalarV_In tValue, SoA_Vec2V_In vectA, SoA_Vec2V_In vectB );
	SoA_ScalarV_Out Lerp(SoA_ScalarV_In tValue, SoA_ScalarV_In vectA, SoA_ScalarV_In vectB );

	void Pow(SoA_Vec4V_InOut outVec, SoA_Vec4V_In x, SoA_Vec4V_In y );
	void Pow(SoA_Vec3V_InOut outVec, SoA_Vec3V_In x, SoA_Vec3V_In y );
	void Pow(SoA_Vec2V_InOut outVec, SoA_Vec2V_In x, SoA_Vec2V_In y );
	SoA_ScalarV_Out Pow(SoA_ScalarV_In x, SoA_ScalarV_In y );
	void PowPrecise(SoA_Vec4V_InOut outVec, SoA_Vec4V_In x, SoA_Vec4V_In y );
	void PowPrecise(SoA_Vec3V_InOut outVec, SoA_Vec3V_In x, SoA_Vec3V_In y );
	void PowPrecise(SoA_Vec2V_InOut outVec, SoA_Vec2V_In x, SoA_Vec2V_In y );
	SoA_ScalarV_Out PowPrecise(SoA_ScalarV_In x, SoA_ScalarV_In y );
	void Expt(SoA_Vec4V_InOut outVec, SoA_Vec4V_In x );
	void Expt(SoA_Vec3V_InOut outVec, SoA_Vec3V_In x );
	void Expt(SoA_Vec2V_InOut outVec, SoA_Vec2V_In x );
	SoA_ScalarV_Out Expt(SoA_ScalarV_In x );
	void Log2(SoA_Vec4V_InOut outVec, SoA_Vec4V_In x );
	void Log2(SoA_Vec3V_InOut outVec, SoA_Vec3V_In x );
	void Log2(SoA_Vec2V_InOut outVec, SoA_Vec2V_In x );
	SoA_ScalarV_Out Log2(SoA_ScalarV_In x );
	void Log10(SoA_Vec4V_InOut outVec, SoA_Vec4V_In x );
	void Log10(SoA_Vec3V_InOut outVec, SoA_Vec3V_In x );
	void Log10(SoA_Vec2V_InOut outVec, SoA_Vec2V_In x );
	SoA_ScalarV_Out Log10(SoA_ScalarV_In x );

	SoA_Vec4V_Out SplatX(SoA_Vec4V_In a );
	SoA_Vec4V_Out SplatY(SoA_Vec4V_In a );
	SoA_Vec4V_Out SplatZ(SoA_Vec4V_In a );
	SoA_Vec4V_Out SplatW(SoA_Vec4V_In a );
	SoA_Vec3V_Out SplatX(SoA_Vec3V_In a );
	SoA_Vec3V_Out SplatY(SoA_Vec3V_In a );
	SoA_Vec3V_Out SplatZ(SoA_Vec3V_In a );
	SoA_Vec2V_Out SplatX(SoA_Vec2V_In a );
	SoA_Vec2V_Out SplatY(SoA_Vec2V_In a );
	SoA_VecBool4V_Out SplatX(SoA_VecBool4V_In a );
	SoA_VecBool4V_Out SplatY(SoA_VecBool4V_In a );
	SoA_VecBool4V_Out SplatZ(SoA_VecBool4V_In a );
	SoA_VecBool4V_Out SplatW(SoA_VecBool4V_In a );
	SoA_VecBool3V_Out SplatX(SoA_VecBool3V_In a );
	SoA_VecBool3V_Out SplatY(SoA_VecBool3V_In a );
	SoA_VecBool3V_Out SplatZ(SoA_VecBool3V_In a );
	SoA_VecBool2V_Out SplatX(SoA_VecBool2V_In a );
	SoA_VecBool2V_Out SplatY(SoA_VecBool2V_In a );

	void Max(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	void Max(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	void Max(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_ScalarV_Out Max(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2);
	void Min(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	void Min(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	void Min(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_ScalarV_Out Min(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2);


	void And(SoA_VecBool4V_InOut outVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2);
	void And(SoA_VecBool3V_InOut outVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2);
	void And(SoA_VecBool2V_InOut outVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2);
	SoA_VecBool1V_Out And(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2);
	void And(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	void And(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	void And(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_ScalarV_Out And(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2);

	void Or(SoA_VecBool4V_InOut outVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2);
	void Or(SoA_VecBool3V_InOut outVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2);
	void Or(SoA_VecBool2V_InOut outVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2);
	SoA_VecBool1V_Out Or(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2);
	void Or(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	void Or(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	void Or(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_ScalarV_Out Or(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2);

	void Xor(SoA_VecBool4V_InOut outVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2);
	void Xor(SoA_VecBool3V_InOut outVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2);
	void Xor(SoA_VecBool2V_InOut outVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2);
	SoA_VecBool1V_Out Xor(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2);
	void Xor(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	void Xor(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	void Xor(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_ScalarV_Out Xor(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2);

	void Andc(SoA_VecBool4V_InOut outVec, SoA_VecBool4V_In inVector1, SoA_VecBool4V_In inVector2);
	void Andc(SoA_VecBool3V_InOut outVec, SoA_VecBool3V_In inVector1, SoA_VecBool3V_In inVector2);
	void Andc(SoA_VecBool2V_InOut outVec, SoA_VecBool2V_In inVector1, SoA_VecBool2V_In inVector2);
	SoA_VecBool1V_Out Andc(SoA_VecBool1V_In inVector1, SoA_VecBool1V_In inVector2);
	void Andc(SoA_Vec4V_InOut outVec, SoA_Vec4V_In inVector1, SoA_Vec4V_In inVector2);
	void Andc(SoA_Vec3V_InOut outVec, SoA_Vec3V_In inVector1, SoA_Vec3V_In inVector2);
	void Andc(SoA_Vec2V_InOut outVec, SoA_Vec2V_In inVector1, SoA_Vec2V_In inVector2);
	SoA_ScalarV_Out Andc(SoA_ScalarV_In inVector1, SoA_ScalarV_In inVector2);
	SoA_ScalarV_Out Andc(SoA_ScalarV_In inVector1, SoA_VecBool1V_In inVector2);


	// One choice vector per component.
	void SelectFT(SoA_Vec4V_InOut outVec, SoA_VecBool4V_In choiceVector, SoA_Vec4V_In zero, SoA_Vec4V_In nonZero);
	void SelectFT(SoA_Vec3V_InOut outVec, SoA_VecBool3V_In choiceVector, SoA_Vec3V_In zero, SoA_Vec3V_In nonZero);
	void SelectFT(SoA_Vec2V_InOut outVec, SoA_VecBool2V_In choiceVector, SoA_Vec2V_In zero, SoA_Vec2V_In nonZero);
	SoA_ScalarV_Out SelectFT(SoA_VecBool1V_In choiceVector, SoA_ScalarV_In zero, SoA_ScalarV_In nonZero);

	// Same choice vector used for all 2/3/4 components.
	void SelectFT(SoA_QuatV_InOut outVec, SoA_VecBool1V_In choiceVector, SoA_QuatV_In zero, SoA_QuatV_In nonZero);
	void SelectFT(SoA_Vec4V_InOut outVec, SoA_VecBool1V_In choiceVector, SoA_Vec4V_In zero, SoA_Vec4V_In nonZero);
	void SelectFT(SoA_Vec3V_InOut outVec, SoA_VecBool1V_In choiceVector, SoA_Vec3V_In zero, SoA_Vec3V_In nonZero);
	void SelectFT(SoA_Vec2V_InOut outVec, SoA_VecBool1V_In choiceVector, SoA_Vec2V_In zero, SoA_Vec2V_In nonZero);
	
	//============================================================================
	// Matrix functions

	SoA_ScalarV_Out Determinant(SoA_Mat33V_In mat );
	SoA_ScalarV_Out Determinant(SoA_Mat44V_In mat );

	void Add(SoA_Mat33V_InOut outMat, SoA_Mat33V_In a, SoA_Mat33V_In b );
	void Subtract(SoA_Mat33V_InOut outMat, SoA_Mat33V_In a, SoA_Mat33V_In b );
	void Abs(SoA_Mat33V_InOut outMat, SoA_Mat33V_In a );
	void Scale(SoA_Mat33V_InOut outMat, SoA_ScalarV_In a, SoA_Mat33V_In b );
	void Scale(SoA_Mat33V_InOut outMat, SoA_Mat33V_In a, SoA_ScalarV_In b );

	void Add(SoA_Mat34V_InOut outMat, SoA_Mat34V_In a, SoA_Mat34V_In b );
	void Subtract(SoA_Mat34V_InOut outMat, SoA_Mat34V_In a, SoA_Mat34V_In b );
	void Abs(SoA_Mat34V_InOut outMat, SoA_Mat34V_In a );
	void Scale(SoA_Mat34V_InOut outMat, SoA_ScalarV_In a, SoA_Mat34V_In b );
	void Scale(SoA_Mat34V_InOut outMat, SoA_Mat34V_In a, SoA_ScalarV_In b );

	void Add(SoA_Mat44V_InOut outMat, SoA_Mat44V_In a, SoA_Mat44V_In b );
	void Subtract(SoA_Mat44V_InOut outMat, SoA_Mat44V_In a, SoA_Mat44V_In b );
	void Abs(SoA_Mat44V_InOut outMat, SoA_Mat44V_In a );
	void Scale(SoA_Mat44V_InOut outMat, SoA_ScalarV_In a, SoA_Mat44V_In b );
	void Scale(SoA_Mat44V_InOut outMat, SoA_Mat44V_In a, SoA_ScalarV_In b );

	// Inversion and transposition.
	void Transpose(SoA_Mat44V_InOut outMat, SoA_Mat44V_In mat );
	void InvertFull(SoA_Mat44V_InOut outMat, SoA_Mat44V_In mat );

	void InvertTransformFull(SoA_Mat34V_InOut outMat, SoA_Mat34V_In mat );
	void InvertTransformOrtho(SoA_Mat34V_InOut outMat, SoA_Mat34V_In mat );

	void Transpose(SoA_Mat33V_InOut outMat, SoA_Mat33V_In mat );
	void InvertFull(SoA_Mat33V_InOut outMat, SoA_Mat33V_In mat );
	void InvertOrtho(SoA_Mat33V_InOut outMat, SoA_Mat33V_In mat );	// Just a transpose

	// "Proper" matrix multiplications.
	void Multiply(SoA_Mat44V_InOut outMat, SoA_Mat44V_In a, SoA_Mat44V_In b );
	void Multiply(SoA_Mat33V_InOut outMat, SoA_Mat33V_In a, SoA_Mat33V_In b );

	void Multiply(SoA_Vec4V_InOut outVec, SoA_Mat44V_In a, SoA_Vec4V_In b );
	void Multiply(SoA_Vec4V_InOut outVec, SoA_Vec4V_In a, SoA_Mat44V_In b );
	void Multiply(SoA_Vec3V_InOut outVec, SoA_Mat33V_In a, SoA_Vec3V_In b );
	void Multiply(SoA_Vec3V_InOut outVec, SoA_Vec3V_In a, SoA_Mat33V_In b );
	void Multiply(SoA_Vec3V_InOut outVec, SoA_Mat34V_In a, SoA_Vec4V_In b );
	void Multiply(SoA_Vec4V_InOut outVec, SoA_Vec3V_In a, SoA_Mat34V_In b );

	// "Proper" matrix^-1 multiplications.
	// NOTE: (SoA_For SoA, SoA_these are all just convenience functions. In AoS, SoA_there are speedup for the *Orhto(SoA_) versions.)
	void UnTransformFull(SoA_Mat44V_InOut outMat, SoA_Mat44V_In matToUntransformBy, SoA_Mat44V_In mat ); // convenience function, SoA_saves no processing.
	void UnTransformOrtho(SoA_Mat44V_InOut outMat, SoA_Mat44V_In orthoMatToUntransformBy, SoA_Mat44V_In mat ); // convenience function, SoA_saves no processing.
	void UnTransformFull(SoA_Vec4V_InOut outVec, SoA_Mat44V_In matToUntransformBy, SoA_Vec4V_In vec ); // convenience function, SoA_saves no processing.
	void UnTransformOrtho(SoA_Vec4V_InOut outVec, SoA_Mat44V_In orthoMatToUntransformBy, SoA_Vec4V_In vec ); // convenience function, SoA_saves no processing.
	void UnTransformFull(SoA_Mat33V_InOut outMat, SoA_Mat33V_In matToUntransformBy, SoA_Mat33V_In mat ); // convenience function, SoA_saves no processing.
	void UnTransformOrtho(SoA_Mat33V_InOut outMat, SoA_Mat33V_In orthoMatToUntransformBy, SoA_Mat33V_In mat ); // convenience function, SoA_saves no processing.
	void UnTransformFull(SoA_Vec3V_InOut outVec, SoA_Mat33V_In matToUntransformBy, SoA_Vec3V_In vec ); // convenience function, SoA_saves no processing.
	void UnTransformOrtho(SoA_Vec3V_InOut outVec, SoA_Mat33V_In orthoMatToUntransformBy, SoA_Vec3V_In vec ); // convenience function, SoA_saves no processing.

	// "Specialized" matrix multiplications (SoA_makes assumption that last col of Mat34V is a translation vector).
	// Enforces pre-multiply convention.
	void Transform(SoA_Mat34V_InOut outMat, SoA_Mat34V_In transformMat, SoA_Mat34V_In mat );
	void Transform3x3(SoA_Mat34V_InOut outMat, SoA_Mat34V_In transformMat, SoA_Mat34V_In mat ); // last 'outMat' col == last 'transformMat' col
	void Transform3x3(SoA_Vec3V_InOut outVec, SoA_Mat34V_In transformMat, SoA_Vec3V_In vec );
	void Transform(SoA_Vec3V_InOut outPoint, SoA_Mat34V_In transformMat, SoA_Vec3V_In point );

	// "Specialized" matrix^-1 multiplications (SoA_makes assumption that last col of Mat34V is a translation vector).
	// Enforces pre-multiply convention.
	// NOTE: (SoA_For SoA, SoA_these are all just convenience functions. In AoS, SoA_there are speedup for the *Orhto(SoA_) versions.)
	void UnTransform3x3Full(SoA_Mat34V_InOut outMat, SoA_Mat34V_In matToUntransformBy, SoA_Mat34V_In mat ); // convenience function, SoA_saves no processing. Last 'outMat' col == last 'concatMat' col.
	void UnTransform3x3Ortho(SoA_Mat34V_InOut outMat, SoA_Mat34V_In orthoMatToUntransformBy, SoA_Mat34V_In mat ); // convenience function, SoA_saves no processing. Last 'outMat' col == last 'concatMat' col.
	void UnTransformFull(SoA_Mat34V_InOut outMat, SoA_Mat34V_In matToUntransformBy, SoA_Mat34V_In mat ); // convenience function, SoA_saves no processing.
	void UnTransformOrtho(SoA_Mat34V_InOut outMat, SoA_Mat34V_In orthoMatToUntransformBy, SoA_Mat34V_In mat ); // convenience function, SoA_saves no processing.
	void UnTransform3x3Full(SoA_Vec3V_InOut outVec, SoA_Mat34V_In matToUntransformBy, SoA_Vec3V_In vec ); // convenience function, SoA_saves no processing.
	void UnTransformFull(SoA_Vec3V_InOut outPoint, SoA_Mat34V_In matToUntransformBy, SoA_Vec3V_In point ); // convenience function, SoA_saves no processing.
	void UnTransform3x3Ortho(SoA_Vec3V_InOut outVec, SoA_Mat34V_In orthoMatToUntransformBy, SoA_Vec3V_In vec ); // convenience function, SoA_saves no processing.
	void UnTransformOrtho(SoA_Vec3V_InOut outPoint, SoA_Mat34V_In orthoMatToUntransformBy, SoA_Vec3V_In point ); // convenience function, SoA_saves no processing.

	// TODO: Implement when needed
	// Quaternion transforms and inverse transforms.
	// return A * V * A^-1
	//SoA_Vec3V_Out	Transform(SoA_QuatV_In unitQuat, SoA_Vec3V_In inVect );
	// return A^-1 * V * A
	//SoA_Vec3V_Out	UnTransformFull(SoA_QuatV_In unitQuat, SoA_Vec3V_In inVect );

	// Re-orthonormalization.
	void ReOrthonormalize(SoA_Mat33V_InOut outMat, SoA_Mat33V_In inMat );
	void ReOrthonormalize3x3(SoA_Mat34V_InOut outMat, SoA_Mat34V_In inMat );	// Leaves the translation untouched.
	void ReOrthonormalize3x3(SoA_Mat44V_InOut outMat, SoA_Mat44V_In inMat );	// Leaves the translation untouched and M30,M31,M32,M33 untouched.

















// DOM-IGNORE-BEGIN

namespace Imp
{

	// The _Imp(SoA_) functions are provided to hide the ugly syntax (SoA_macro surrounding any >128-bit argument) that is necessary to help pass via vector registers.
	// The non-_Imp(SoA_) functions which call the _Imp(SoA_) functions are very short and are __forceinline'd so that there is no param passing on the stack at all,
	// EVER! (SoA_except when the # of arguments exceeds the platform's register passing limits... see README.txt)

	// NOTE: When implementing SoA functions (SoA_or any functions that require many __vector4 arguemnts), SoA_keep in mind that only so many can pass via registers.
	//	Xenon: 13
	//	PS3 PPU: 12
	//	PS3 SPU: 24+ (SoA_didn't test any higher)
	// So, SoA_when there are lots of arguments to an *_Imp(SoA_) function below, SoA_make sure to pass them in the rough chronological order of when they will be used in the function.
	// This way, SoA_some work can be done with the first few arguments, SoA_without waiting on an important variable that must be loaded from the stack.
	//	e.g. 
	//	Do this:
	// void Scale_Imp44(SoA_Mat44V_InOut outMat, Vec::Vector_4V_In a, MAT44V_SOA_DECL(b) );
	//	Not this:
	// void Scale_Imp44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(a), Vec::Vector_4V_In b );
	

	//================================================
	// For private use only...
	//================================================

	void Abs_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect));
	void Abs_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect));
	void Abs_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect));

	void Sqrt_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect));
	void Sqrt_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect));
	void Sqrt_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect));
	void SqrtSafe_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec);
	void SqrtSafe_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec);
	void SqrtSafe_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec);
	void SqrtFast_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect));
	void SqrtFast_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect));
	void SqrtFast_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect));
	void SqrtFastSafe_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec);
	void SqrtFastSafe_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec);
	void SqrtFastSafe_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec);

	void InvSqrt_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect));
	void InvSqrt_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect));
	void InvSqrt_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect));
	void InvSqrtSafe_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec);
	void InvSqrtSafe_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec);
	void InvSqrtSafe_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec);
	void InvSqrtFast_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect));
	void InvSqrtFast_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect));
	void InvSqrtFast_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect));
	void InvSqrtFastSafe_Imp(SoA_Vec2V_InOut outVect, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec);
	void InvSqrtFastSafe_Imp(SoA_Vec3V_InOut outVect, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec);
	void InvSqrtFastSafe_Imp(SoA_Vec4V_InOut outVect, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec);

	void Scale_Imp_1_4(SoA_Vec4V_InOut outVec, Vec::Vector_4V_In a, VEC4V_SOA_DECL3(b) );
	void Scale_Imp_1_3(SoA_Vec3V_InOut outVec, Vec::Vector_4V_In a, VEC3V_SOA_DECL3(b) );
	void Scale_Imp_1_2(SoA_Vec2V_InOut outVec, Vec::Vector_4V_In a, VEC2V_SOA_DECL(b) );

	void AddInt_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	void AddInt_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	void AddInt_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) );
	void SubtractInt_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	void SubtractInt_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	void SubtractInt_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) );
	void AddScaled_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toAdd), VEC4V_SOA_DECL2(toScaleThenAdd), VEC4V_SOA_DECL2(scaleValues) );
	void AddScaled_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toAdd), VEC3V_SOA_DECL2(toScaleThenAdd), VEC3V_SOA_DECL2(scaleValues) );
	void AddScaled_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toAdd), VEC3V_SOA_DECL2(toScaleThenAdd), Vec::Vector_4V_In_After3Args scaleValue );
	void AddScaled_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toAdd), VEC2V_SOA_DECL2(toScaleThenAdd), VEC2V_SOA_DECL3(scaleValues) );
	void SubtractScaled_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toSubtractFrom), VEC4V_SOA_DECL2(toScaleThenSubtract), VEC4V_SOA_DECL2(scaleValues) );
	void SubtractScaled_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toSubtractFrom), VEC3V_SOA_DECL2(toScaleThenSubtract), VEC3V_SOA_DECL2(scaleValues) );
	void SubtractScaled_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toSubtractFrom), VEC2V_SOA_DECL2(toScaleThenSubtract), VEC2V_SOA_DECL3(scaleValues) );

	SoA_ScalarV_Out Dot_Imp_4_4(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	SoA_ScalarV_Out Dot_Imp_3_3(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	SoA_ScalarV_Out Dot_Imp_2_2(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) );

	void Scale_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	void Scale_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	void Scale_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) );

	void InvScale_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScale_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toScale), VEC4V_SOA_DECL2(scaleValues) );
	void InvScaleSafe_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleSafe_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toScale), VEC4V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleFast_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScaleFast_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toScale), VEC4V_SOA_DECL2(scaleValues) );
	void InvScaleFastSafe_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleFastSafe_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(toScale), VEC4V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect);
	void InvScale_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScale_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toScale), VEC3V_SOA_DECL2(scaleValues) );
	void InvScaleSafe_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleSafe_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toScale), VEC3V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleFast_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScaleFast_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toScale), VEC3V_SOA_DECL2(scaleValues) );
	void InvScaleFastSafe_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleFastSafe_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(toScale), VEC3V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect);
	void InvScale_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toScale), Vec::Vector_4V_In scaleValue );
	void InvScale_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toScale), VEC2V_SOA_DECL2(scaleValues) );
	void InvScaleSafe_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toScale), Vec::Vector_4V_In scaleValue, Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleSafe_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toScale), VEC2V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleFast_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toScale), Vec::Vector_4V_In scaleValue );
	void InvScaleFast_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toScale), VEC2V_SOA_DECL2(scaleValues) );
	void InvScaleFastSafe_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toScale), Vec::Vector_4V_In scaleValue, Vec::Vector_4V_In_After3Args errValVect);
	void InvScaleFastSafe_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(toScale), VEC2V_SOA_DECL2(scaleValues), Vec::Vector_4V_In_After3Args errValVect);

	Vec::Vector_4V_Out Mag_Imp(VEC2V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out Mag_Imp(VEC3V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out Mag_Imp(VEC4V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out MagFast_Imp(VEC2V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out MagFast_Imp(VEC3V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out MagFast_Imp(VEC4V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out MagSquared_Imp(VEC2V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out MagSquared_Imp(VEC3V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out MagSquared_Imp(VEC4V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMag_Imp(VEC2V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMag_Imp(VEC3V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMag_Imp(VEC4V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagSafe_Imp(VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec );
	Vec::Vector_4V_Out InvMagSafe_Imp(VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec );
	Vec::Vector_4V_Out InvMagSafe_Imp(VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec );
	Vec::Vector_4V_Out InvMagFast_Imp(VEC2V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagFast_Imp(VEC3V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagFast_Imp(VEC4V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagFastSafe_Imp(VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec );
	Vec::Vector_4V_Out InvMagFastSafe_Imp(VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec );
	Vec::Vector_4V_Out InvMagFastSafe_Imp(VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec );
	Vec::Vector_4V_Out InvMagSquared_Imp(VEC2V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagSquared_Imp(VEC3V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagSquared_Imp(VEC4V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagSquaredSafe_Imp(VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec );
	Vec::Vector_4V_Out InvMagSquaredSafe_Imp(VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec );
	Vec::Vector_4V_Out InvMagSquaredSafe_Imp(VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec );
	Vec::Vector_4V_Out InvMagSquaredFast_Imp(VEC2V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagSquaredFast_Imp(VEC3V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagSquaredFast_Imp(VEC4V_SOA_DECL(inVect) );
	Vec::Vector_4V_Out InvMagSquaredFastSafe_Imp(VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVec );
	Vec::Vector_4V_Out InvMagSquaredFastSafe_Imp(VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec );
	Vec::Vector_4V_Out InvMagSquaredFastSafe_Imp(VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVec );

	void Normalize_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect));
	void Normalize_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect));
	void Normalize_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect));
	void NormalizeSafe_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect);
	void NormalizeSafe_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);
	void NormalizeSafe_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);
	void NormalizeFast_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect));
	void NormalizeFast_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect));
	void NormalizeFast_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect));
	void NormalizeFastSafe_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect);
	void NormalizeFastSafe_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);
	void NormalizeFastSafe_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);

	Vec::Vector_4V_Out Dist_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b));
	Vec::Vector_4V_Out DistFast_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b));
	Vec::Vector_4V_Out Dist_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b));
	Vec::Vector_4V_Out DistFast_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b));
	Vec::Vector_4V_Out Dist_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b));
	Vec::Vector_4V_Out DistFast_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b));

	Vec::Vector_4V_Out InvDist_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSafe_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDistFast_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistFastSafe_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDist_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSafe_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDistFast_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistFastSafe_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDist_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSafe_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDistFast_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistFastSafe_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out DistSquared_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b));
	Vec::Vector_4V_Out DistSquared_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b));
	Vec::Vector_4V_Out DistSquared_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSquared_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSquaredSafe_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDistSquaredFast_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSquaredFastSafe_Imp(VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDistSquared_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSquaredSafe_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDistSquaredFast_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSquaredFastSafe_Imp(VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDistSquared_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSquaredSafe_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);
	Vec::Vector_4V_Out InvDistSquaredFast_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b));
	Vec::Vector_4V_Out InvDistSquaredFastSafe_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b), Vec::Vector_4V_In_After3Args errValVect);

	void IsEqualIntV_Imp(SoA_VecBool4V_InOut outVect, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	void IsEqualIntV_Imp(SoA_VecBool3V_InOut outVect, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	void IsEqualIntV_Imp(SoA_VecBool2V_InOut outVect, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualIntAll_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualIntNone_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsZeroAll_Imp(VEC4V_SOA_DECL(inVector));
	Vec::Vector_4V_Out IsZeroAll_Imp(VEC3V_SOA_DECL(inVector));
	Vec::Vector_4V_Out IsZeroAll_Imp(VEC2V_SOA_DECL(inVector));
	Vec::Vector_4V_Out IsZeroNone_Imp(VEC4V_SOA_DECL(inVector));
	Vec::Vector_4V_Out IsZeroNone_Imp(VEC3V_SOA_DECL(inVector));
	Vec::Vector_4V_Out IsZeroNone_Imp(VEC2V_SOA_DECL(inVector));
	void IsEqualV_Imp(SoA_VecBool4V_InOut outVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	void IsEqualV_Imp(SoA_VecBool3V_InOut outVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	void IsEqualV_Imp(SoA_VecBool2V_InOut outVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualAll_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualAll_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualAll_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualNone_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualNone_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualNone_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));
	void IsCloseV_Imp(SoA_VecBool4V_InOut outVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	void IsCloseV_Imp(SoA_VecBool3V_InOut outVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	void IsCloseV_Imp(SoA_VecBool2V_InOut outVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	Vec::Vector_4V_Out IsCloseAll_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	Vec::Vector_4V_Out IsCloseAll_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	Vec::Vector_4V_Out IsCloseAll_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	Vec::Vector_4V_Out IsCloseNone_Imp(VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	Vec::Vector_4V_Out IsCloseNone_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	Vec::Vector_4V_Out IsCloseNone_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2), Vec::Vector_4V_In_After3Args eps);
	Vec::Vector_4V_Out IsGreaterThanAll_Imp(VEC4V_SOA_DECL(bigVector), VEC4V_SOA_DECL2(smallVector));
	Vec::Vector_4V_Out IsGreaterThanAll_Imp(VEC3V_SOA_DECL(bigVector), VEC3V_SOA_DECL2(smallVector));
	Vec::Vector_4V_Out IsGreaterThanAll_Imp(VEC2V_SOA_DECL(bigVector), VEC2V_SOA_DECL2(smallVector));
	void IsGreaterThanV_Imp(SoA_VecBool4V_InOut outVec, VEC4V_SOA_DECL(bigVector), VEC4V_SOA_DECL2(smallVector));
	void IsGreaterThanV_Imp(SoA_VecBool3V_InOut outVec, VEC3V_SOA_DECL(bigVector), VEC3V_SOA_DECL2(smallVector));
	void IsGreaterThanV_Imp(SoA_VecBool3V_InOut outVec, Vec::Vector_4V_In_After3Args bigVector, VEC3V_SOA_DECL(smallVector));
	void IsGreaterThanV_Imp(SoA_VecBool2V_InOut outVec, VEC2V_SOA_DECL(bigVector), VEC2V_SOA_DECL2(smallVector));
	Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp(VEC4V_SOA_DECL(bigVector), VEC4V_SOA_DECL2(smallVector));
	Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp(VEC3V_SOA_DECL(bigVector), VEC3V_SOA_DECL2(smallVector));
	Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp(VEC2V_SOA_DECL(bigVector), VEC2V_SOA_DECL2(smallVector));
	void IsGreaterThanOrEqualV_Imp(SoA_VecBool4V_InOut outVec, VEC4V_SOA_DECL(bigVector), VEC4V_SOA_DECL2(smallVector));
	void IsGreaterThanOrEqualV_Imp(SoA_VecBool3V_InOut outVec, VEC3V_SOA_DECL(bigVector), VEC3V_SOA_DECL2(smallVector));
	void IsGreaterThanOrEqualV_Imp(SoA_VecBool2V_InOut outVec, VEC2V_SOA_DECL(bigVector), VEC2V_SOA_DECL2(smallVector));
	Vec::Vector_4V_Out IsLessThanAll_Imp(VEC4V_SOA_DECL(smallVector), VEC4V_SOA_DECL2(bigVector));
	Vec::Vector_4V_Out IsLessThanAll_Imp(VEC3V_SOA_DECL(smallVector), VEC3V_SOA_DECL2(bigVector));
	Vec::Vector_4V_Out IsLessThanAll_Imp(VEC2V_SOA_DECL(smallVector), VEC2V_SOA_DECL2(bigVector));
	void IsLessThanV_Imp(SoA_VecBool4V_InOut outVec, VEC4V_SOA_DECL(smallVector), VEC4V_SOA_DECL2(bigVector));
	void IsLessThanV_Imp(SoA_VecBool3V_InOut outVec, VEC3V_SOA_DECL(smallVector), VEC3V_SOA_DECL2(bigVector));
	void IsLessThanV_Imp(SoA_VecBool2V_InOut outVec, VEC2V_SOA_DECL(smallVector), VEC2V_SOA_DECL2(bigVector));
	Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp(VEC4V_SOA_DECL(smallVector), VEC4V_SOA_DECL2(bigVector));
	Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp(VEC3V_SOA_DECL(smallVector), VEC3V_SOA_DECL2(bigVector));
	Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp(VEC2V_SOA_DECL(smallVector), VEC2V_SOA_DECL2(bigVector));
	void IsLessThanOrEqualV_Imp(SoA_VecBool4V_InOut outVec, VEC4V_SOA_DECL(smallVector), VEC4V_SOA_DECL2(bigVector));
	void IsLessThanOrEqualV_Imp(SoA_VecBool3V_InOut outVec, VEC3V_SOA_DECL(smallVector), VEC3V_SOA_DECL2(bigVector));
	void IsLessThanOrEqualV_Imp(SoA_VecBool2V_InOut outVec, VEC2V_SOA_DECL(smallVector), VEC2V_SOA_DECL2(bigVector));
	void IsEqualIntV_Imp(SoA_VecBool3V_InOut outVect, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualIntAll_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualIntNone_Imp(VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	void IsEqualIntV_Imp(SoA_VecBool2V_InOut outVect, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualIntAll_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));
	Vec::Vector_4V_Out IsEqualIntNone_Imp(VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));

	Vec::Vector_4V_Out IsEqualAll_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualAll_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualAll_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualNone_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualNone_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualNone_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualIntAll_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualIntAll_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualIntAll_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualIntNone_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualIntNone_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsEqualIntNone_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2));
	Vec::Vector_4V_Out IsGreaterThanAll_Imp_44_44(MAT44V_SOA_DECL(bigMat), MAT44V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsGreaterThanAll_Imp_34_34(MAT34V_SOA_DECL(bigMat), MAT34V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsGreaterThanAll_Imp_33_33(MAT33V_SOA_DECL(bigMat), MAT33V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp_44_44(MAT44V_SOA_DECL(bigMat), MAT44V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp_34_34(MAT34V_SOA_DECL(bigMat), MAT34V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsGreaterThanOrEqualAll_Imp_33_33(MAT33V_SOA_DECL(bigMat), MAT33V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsLessThanAll_Imp_44_44(MAT44V_SOA_DECL(bigMat), MAT44V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsLessThanAll_Imp_34_34(MAT34V_SOA_DECL(bigMat), MAT34V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsLessThanAll_Imp_33_33(MAT33V_SOA_DECL(bigMat), MAT33V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp_44_44(MAT44V_SOA_DECL(bigMat), MAT44V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp_34_34(MAT34V_SOA_DECL(bigMat), MAT34V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsLessThanOrEqualAll_Imp_33_33(MAT33V_SOA_DECL(bigMat), MAT33V_SOA_DECL2(smallMat));
	Vec::Vector_4V_Out IsCloseAll_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues);
	Vec::Vector_4V_Out IsCloseAll_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues);
	Vec::Vector_4V_Out IsCloseAll_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues);
	Vec::Vector_4V_Out IsCloseNone_Imp_44_44(MAT44V_SOA_DECL(inMat1), MAT44V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues);
	Vec::Vector_4V_Out IsCloseNone_Imp_34_34(MAT34V_SOA_DECL(inMat1), MAT34V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues);
	Vec::Vector_4V_Out IsCloseNone_Imp_33_33(MAT33V_SOA_DECL(inMat1), MAT33V_SOA_DECL2(inMat2), Vec::Vector_4V_In_After3Args epsValues);

	//============================================================================
	// Conversion functions

	template <int exponent>
	void FloatToIntRaw_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVec));
	template <int exponent>
	void IntToFloatRaw_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVec));
	void RoundToNearestInt_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVec));
	void RoundToNearestIntZero_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVec));
	void RoundToNearestIntNegInf_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVec));
	void RoundToNearestIntPosInf_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVec));

	template <int exponent>
	void FloatToIntRaw_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVec));
	template <int exponent>
	void IntToFloatRaw_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVec));
	void RoundToNearestInt_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVec));
	void RoundToNearestIntZero_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVec));
	void RoundToNearestIntNegInf_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVec));
	void RoundToNearestIntPosInf_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVec));

	template <int exponent>
	void FloatToIntRaw_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVec));
	template <int exponent>
	void IntToFloatRaw_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVec));
	void RoundToNearestInt_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVec));
	void RoundToNearestIntZero_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVec));
	void RoundToNearestIntNegInf_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVec));
	void RoundToNearestIntPosInf_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVec));

	//============================================================================
	// Standard quaternion math

	void Conjugate_Imp_Q(SoA_QuatV_InOut outQuat, QUATV_SOA_DECL(inQuat));
	void Normalize_Imp(SoA_QuatV_InOut outVec, QUATV_SOA_DECL(inVect));
	void NormalizeSafe_Imp(SoA_QuatV_InOut outVec, QUATV_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);
	void NormalizeFast_Imp(SoA_QuatV_InOut outVec, QUATV_SOA_DECL(inVect));
	void NormalizeFastSafe_Imp(SoA_QuatV_InOut outVec, QUATV_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);
	void Invert_Imp_Q(SoA_QuatV_InOut outQuat, QUATV_SOA_DECL(inQuat));
	void InvertSafe_Imp_Q(SoA_QuatV_InOut outQuat, QUATV_SOA_DECL(inQuat), Vec::Vector_4V_In_After3Args errValVect);
	void InvertFast_Imp_Q(SoA_QuatV_InOut outQuat, QUATV_SOA_DECL(inQuat));
	void InvertFastSafe_Imp_Q(SoA_QuatV_InOut outQuat, QUATV_SOA_DECL(inQuat), Vec::Vector_4V_In_After3Args errValVect);
	void PrepareSlerp_Imp_Q( SoA_QuatV_InOut inoutQuat, QUATV_SOA_DECL2(inQuat1), QUATV_SOA_DECL2(inQuat2) );
	void Nlerp_Imp_Q( SoA_QuatV_InOut inoutQuat, Vec::Vector_4V_In tValue, QUATV_SOA_DECL2(inQuat1), QUATV_SOA_DECL2(inQuat2) );
	void ToAxisAngle_Imp_Q( SoA_Vec3V_InOut outAxis, SoA_ScalarV_InOut outRadians, QUATV_SOA_DECL(inQuat) );
	void FromAxisAngle_Imp_Q( SoA_QuatV_InOut outQuat, VEC3V_SOA_DECL3(inNormAxis), Vec::Vector_4V_In_After3Args inRadians );
	void ScaleAngle_Imp_Q( SoA_QuatV_InOut outQuat, QUATV_SOA_DECL(inQuat), Vec::Vector_4V_In_After3Args inRadians );
	void Mat33VFromQuatV_Imp_Q( SoA_Mat33V_InOut outMat, QUATV_SOA_DECL(inQuat) );
	void QuatVFromEulersXYZ_Imp_Q(SoA_QuatV_InOut outQuat, VEC3V_SOA_DECL(radianAngles) );
	void Multiply_Imp_Q(SoA_QuatV_InOut outQuat, QUATV_SOA_DECL(inQuat1), QUATV_SOA_DECL2(inQuat2) );
	// TODO: Implement when needed.
	//void Slerp_Imp_Q(SoA_QuatV_InOut outQuat, Vec::Vector_4V_In t, QUATV_SOA_DECL(inQuat1), QUATV_SOA_DECL(inQuat2) );

	//============================================================================
	// Standard Algebra

	void Add_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	void Add_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	void Add_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) );
	void Subtract_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	void Subtract_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	void Subtract_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) );

	void Average_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	void Average_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	void Average_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) );

	void Pow_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(x), VEC4V_SOA_DECL2(y) );
	void Pow_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(x), VEC3V_SOA_DECL2(y) );
	void Pow_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(x), VEC2V_SOA_DECL2(y) );
	void PowPrecise_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(x), VEC4V_SOA_DECL2(y) );
	void PowPrecise_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(x), VEC3V_SOA_DECL2(y) );
	void PowPrecise_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(x), VEC2V_SOA_DECL2(y) );
	void Expt_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(x) );
	void Expt_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(x) );
	void Expt_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(x) );
	void Log2_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(x));
	void Log2_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(x) );
	void Log2_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(x) );
	void Log10_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(x) );
	void Log10_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(x) );
	void Log10_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(x) );

	void Max_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2) );
	void Max_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2) );
	void Max_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2) );
	void Min_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2) );
	void Min_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2) );
	void Min_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2) );

	void And_Imp(SoA_VecBool4V_InOut outVec, VECBOOL4V_SOA_DECL(inVector1), VECBOOL4V_SOA_DECL2(inVector2));
	void And_Imp(SoA_VecBool3V_InOut outVec, VECBOOL3V_SOA_DECL(inVector1), VECBOOL3V_SOA_DECL2(inVector2));
	void And_Imp(SoA_VecBool2V_InOut outVec, VECBOOL2V_SOA_DECL(inVector1), VECBOOL2V_SOA_DECL2(inVector2));
	void And_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	void And_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	void And_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));

	void Or_Imp(SoA_VecBool4V_InOut outVec, VECBOOL4V_SOA_DECL(inVector1), VECBOOL4V_SOA_DECL2(inVector2));
	void Or_Imp(SoA_VecBool3V_InOut outVec, VECBOOL3V_SOA_DECL(inVector1), VECBOOL3V_SOA_DECL2(inVector2));
	void Or_Imp(SoA_VecBool2V_InOut outVec, VECBOOL2V_SOA_DECL(inVector1), VECBOOL2V_SOA_DECL2(inVector2));
	void Or_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	void Or_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	void Or_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));

	void Xor_Imp(SoA_VecBool4V_InOut outVec, VECBOOL4V_SOA_DECL(inVector1), VECBOOL4V_SOA_DECL2(inVector2));
	void Xor_Imp(SoA_VecBool3V_InOut outVec, VECBOOL3V_SOA_DECL(inVector1), VECBOOL3V_SOA_DECL2(inVector2));
	void Xor_Imp(SoA_VecBool2V_InOut outVec, VECBOOL2V_SOA_DECL(inVector1), VECBOOL2V_SOA_DECL2(inVector2));
	void Xor_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	void Xor_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	void Xor_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));

	void Andc_Imp(SoA_VecBool4V_InOut outVec, VECBOOL4V_SOA_DECL(inVector1), VECBOOL4V_SOA_DECL2(inVector2));
	void Andc_Imp(SoA_VecBool3V_InOut outVec, VECBOOL3V_SOA_DECL(inVector1), VECBOOL3V_SOA_DECL2(inVector2));
	void Andc_Imp(SoA_VecBool2V_InOut outVec, VECBOOL2V_SOA_DECL(inVector1), VECBOOL2V_SOA_DECL2(inVector2));
	void Andc_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVector1), VEC4V_SOA_DECL2(inVector2));
	void Andc_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVector1), VEC3V_SOA_DECL2(inVector2));
	void Andc_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVector1), VEC2V_SOA_DECL2(inVector2));

	void Clamp_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args lowBound, Vec::Vector_4V_In_After3Args highBound );
	void Clamp_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args lowBound, Vec::Vector_4V_In_After3Args highBound );
	void Clamp_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args lowBound, Vec::Vector_4V_In_After3Args highBound );

	void ClampMag_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args minMag, Vec::Vector_4V_In_After3Args maxMag );

	void Negate_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect));
	void Negate_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect));
	void Negate_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect));

	void InvertBits_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect));
	void InvertBits_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect));
	void InvertBits_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect));

	void InvertBits_Imp(SoA_VecBool2V_InOut outVec, VECBOOL2V_SOA_DECL(inVect));
	void InvertBits_Imp(SoA_VecBool3V_InOut outVec, VECBOOL3V_SOA_DECL(inVect));
	void InvertBits_Imp(SoA_VecBool4V_InOut outVec, VECBOOL4V_SOA_DECL(inVect));

	void Invert_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect));
	void Invert_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect));
	void Invert_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect));
	void InvertSafe_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect);
	void InvertSafe_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);
	void InvertSafe_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);
	void InvertFast_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect));
	void InvertFast_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect));
	void InvertFast_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect));
	void InvertFastSafe_Imp(SoA_Vec2V_InOut outVec, VEC2V_SOA_DECL(inVect), Vec::Vector_4V_In errValVect);
	void InvertFastSafe_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);
	void InvertFastSafe_Imp(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(inVect), Vec::Vector_4V_In_After3Args errValVect);

	void Lerp_Imp(SoA_Vec4V_InOut outVec, Vec::Vector_4V_In tValue, VEC4V_SOA_DECL3(vectA), VEC4V_SOA_DECL2(vectB) );
	void Lerp_Imp(SoA_Vec3V_InOut outVec, Vec::Vector_4V_In tValue, VEC3V_SOA_DECL3(vectA), VEC3V_SOA_DECL2(vectB) );
	void Lerp_Imp(SoA_Vec2V_InOut outVec, Vec::Vector_4V_In tValue, VEC2V_SOA_DECL(vectA), VEC2V_SOA_DECL3(vectB) );

	void Select_Imp(SoA_Vec4V_InOut outVec, VECBOOL4V_SOA_DECL(choiceVector), VEC4V_SOA_DECL2(zero), VEC4V_SOA_DECL2(nonZero));
	void Select_Imp(SoA_Vec3V_InOut outVec, VECBOOL3V_SOA_DECL(choiceVector), VEC3V_SOA_DECL2(zero), VEC3V_SOA_DECL2(nonZero));
	void Select_Imp(SoA_Vec2V_InOut outVec, VECBOOL2V_SOA_DECL(choiceVector), VEC2V_SOA_DECL2(zero), VEC2V_SOA_DECL3(nonZero));

	void Select_Imp(SoA_QuatV_InOut outVec, Vec::Vector_4V_In choiceVector, QUATV_SOA_DECL2(zero), QUATV_SOA_DECL2(nonZero));
	void Select_Imp(SoA_Vec4V_InOut outVec, Vec::Vector_4V_In choiceVector, VEC4V_SOA_DECL3(zero), VEC4V_SOA_DECL2(nonZero));
	void Select_Imp(SoA_Vec3V_InOut outVec, Vec::Vector_4V_In choiceVector, VEC3V_SOA_DECL3(zero), VEC3V_SOA_DECL2(nonZero));
	void Select_Imp(SoA_Vec2V_InOut outVec, Vec::Vector_4V_In choiceVector, VEC2V_SOA_DECL(zero), VEC2V_SOA_DECL3(nonZero));

	void Cross_Imp(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	Vec::Vector_4V_Out Cross_Imp(VEC2V_SOA_DECL(a), VEC2V_SOA_DECL2(b) );

	//============================================================================
	// Matrix functions

	Vec::Vector_4V_Out Determinant_Imp33(MAT33V_SOA_DECL(mat) );
	Vec::Vector_4V_Out Determinant_Imp44(MAT44V_SOA_DECL(mat) );

	void Add_Imp33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(a), MAT33V_SOA_DECL2(b) );
	void Subtract_Imp33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(a), MAT33V_SOA_DECL2(b) );
	void Abs_Imp33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(a) );
	void Scale_Imp33(SoA_Mat33V_InOut outMat, Vec::Vector_4V_In a, MAT33V_SOA_DECL3(b) );

	void Add_Imp34(SoA_Mat34V_InOut outMat, MAT34V_SOA_DECL(a), MAT34V_SOA_DECL2(b) );
	void Subtract_Imp34(SoA_Mat34V_InOut outMat, MAT34V_SOA_DECL(a), MAT34V_SOA_DECL2(b) );
	void Abs_Imp34(SoA_Mat34V_InOut outMat, MAT34V_SOA_DECL(a) );
	void Scale_Imp34(SoA_Mat34V_InOut outMat, Vec::Vector_4V_In a, MAT34V_SOA_DECL3(b) );
	
	void Add_Imp44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(a), MAT44V_SOA_DECL2(b) );
	void Subtract_Imp44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(a), MAT44V_SOA_DECL2(b) );
	void Abs_Imp44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(a) );
	void Scale_Imp44(SoA_Mat44V_InOut outMat, Vec::Vector_4V_In a, MAT44V_SOA_DECL3(b) );

	// Inversion and transposition.
	void Transpose_Imp44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(mat) );
	void InvertFull_Imp44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(mat) );

	void InvertTransformFull_Imp34(SoA_Mat34V_InOut outMat, MAT34V_SOA_DECL(mat) );
	void InvertTransformOrtho_Imp34(SoA_Mat34V_InOut outMat, MAT34V_SOA_DECL(mat) );

	void Transpose_Imp33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(mat) );
	void InvertFull_Imp33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(mat) );

	// "Proper" matrix multiplications.
	void Mul_Imp_44_44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(a), MAT44V_SOA_DECL2(b) );
	void Mul_Imp_33_33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(a), MAT33V_SOA_DECL2(b) );

	// Other "proper" multiplications.
	void Mul_Imp_44_4(SoA_Vec4V_InOut outVec, MAT44V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	void Mul_Imp_4_44(SoA_Vec4V_InOut outVec, VEC4V_SOA_DECL(a), MAT44V_SOA_DECL2(b) );
	void Mul_Imp_33_3(SoA_Vec3V_InOut outVec, MAT33V_SOA_DECL(a), VEC3V_SOA_DECL2(b) );
	void Mul_Imp_3_33(SoA_Vec3V_InOut outVec, VEC3V_SOA_DECL(a), MAT33V_SOA_DECL2(b) );
	void Mul_Imp_34_4(SoA_Vec3V_InOut outVec, MAT34V_SOA_DECL(a), VEC4V_SOA_DECL2(b) );
	void Mul_Imp_3_34(SoA_Vec4V_InOut outVec, VEC3V_SOA_DECL(a), MAT34V_SOA_DECL2(b) );

	// "Proper" matrix^-1 multiplications.
	void UnTransformFull_Imp_44_44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(origTransformMat), MAT44V_SOA_DECL2(concatMat) );
	void UnTransformOrtho_Imp_44_44(SoA_Mat44V_InOut outMat, MAT44V_SOA_DECL(origOrthoTransformMat), MAT44V_SOA_DECL2(concatMat) );
	void UnTransformFull_Imp_44_4(SoA_Vec4V_InOut outVec, MAT44V_SOA_DECL(origTransformMat), VEC4V_SOA_DECL2(transformedVect) );
	void UnTransformOrtho_Imp_44_4(SoA_Vec4V_InOut outVec, MAT44V_SOA_DECL(origOrthoTransformMat), VEC4V_SOA_DECL2(transformedVect) );
	void UnTransformFull_Imp_33_33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(origTransformMat), MAT33V_SOA_DECL2(concatMat) );
	void UnTransformOrtho_Imp_33_33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(origOrthoTransformMat), MAT33V_SOA_DECL2(concatMat) );
	void UnTransformFull_Imp_33_3(SoA_Vec3V_InOut outVec, MAT33V_SOA_DECL(origTransformMat), VEC3V_SOA_DECL2(transformedVect) );
	void UnTransformOrtho_Imp_33_3(SoA_Vec3V_InOut outVec, MAT33V_SOA_DECL(origOrthoTransformMat), VEC3V_SOA_DECL2(transformedVect) );

	// "Specialized" matrix multiplications (SoA_makes assumption that last col of Mat34V is a translation vector).
	// Enforces pre-multiply convention.
	void Transform_Imp_34_34(SoA_Mat34V_InOut outMat, MAT34V_SOA_DECL(transformMat1), MAT34V_SOA_DECL2(transformMat2) );
	void Transform3x3_Imp_34_3(SoA_Vec3V_InOut outVec, MAT33V_SOA_DECL(transformMat), VEC3V_SOA_DECL2(inVec) ); // No, MAT33V_SOA_DECL isn't a typo.
	void Transform_Imp_34_3(SoA_Vec3V_InOut outPoint, MAT34V_SOA_DECL(transformMat), VEC3V_SOA_DECL2(inPoint) );

	// "Specialized" matrix^-1 multiplications (SoA_makes assumption that last col of Mat34V is a translation vector).
	// Enforces pre-multiply convention.
	// NOTE: (SoA_For SoA, SoA_these are all just convenience functions. In AoS, SoA_there is a speedup for the *Orhto(SoA_) versions.)
	void UnTransformFull_Imp_34_34(SoA_Mat34V_InOut outMat, MAT34V_SOA_DECL(origTransformMat), MAT34V_SOA_DECL2(concatMat) );
	void UnTransformOrtho_Imp_34_34(SoA_Mat34V_InOut outMat, MAT34V_SOA_DECL(origOrthoTransformMat), MAT34V_SOA_DECL2(concatMat) );
	void UnTransform3x3Full_Imp_34_3(SoA_Vec3V_InOut outVec, MAT33V_SOA_DECL(origTransformMat), VEC3V_SOA_DECL2(transformedVect) ); // No, MAT33V_SOA_DECL isn't a typo.
	void UnTransformFull_Imp_34_3(SoA_Vec3V_InOut outPoint, MAT34V_SOA_DECL(origTransformMat), VEC3V_SOA_DECL2(SoA_transformedPoint) );
	void UnTransform3x3Ortho_Imp_34_3(SoA_Vec3V_InOut outVec, MAT33V_SOA_DECL(origOrthoTransformMat), VEC3V_SOA_DECL2(transformedVect) ); // No, MAT33V_SOA_DECL isn't a typo.
	void UnTransformOrtho_Imp_34_3(SoA_Vec3V_InOut outPoint, MAT34V_SOA_DECL(origOrthoTransformMat), VEC3V_SOA_DECL2(SoA_transformedPoint) );

	void ReOrthonormalize_Imp33(SoA_Mat33V_InOut outMat, MAT33V_SOA_DECL(inMat) );

} // namespace Imp

// DOM-IGNORE-END

} // namespace rage

#endif // !defined VECTORMATH_CLASSFREEFUNCSV_SOA_H