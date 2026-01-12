#ifndef VECTORMATH_CLASSFREEFUNCSF_H
#define VECTORMATH_CLASSFREEFUNCSF_H

namespace rage
{
	// (No 2-/3-/4-way trig functions -- these aren't usually useful in the scalar case.)
	// (No scalar matrices. It doesn't seem worth it for scalar floats.)

	//============================================================================
	// Utility functions

	Quatf_Out QuatFromAxisAngle( Vec3f_In normAxis, float radians );
	Quatf_Out QuatFromXAxisAngle( float radians );
	Quatf_Out QuatFromYAxisAngle( float radians );
	Quatf_Out QuatFromZAxisAngle( float radians );

	Vec2f_Out Vec2FromF32( float );
	Vec3f_Out Vec3FromF32( float );
	Vec4f_Out Vec4FromF32( float );
	Quatf_Out QuatFromF32( float );

	//============================================================================
	// Comparison functions

	unsigned int IsZeroAll(Quatf_In inQuat);
	unsigned int IsZeroNone(Quatf_In inQuat);
	unsigned int IsEqualAll(Quatf_In inQuat1, Quatf_In inQuat2);
	unsigned int IsEqualNone(Quatf_In inQuat1, Quatf_In inQuat2);
	unsigned int IsEqualIntAll(Quatf_In inQuat1, Quatf_In inQuat2);
	unsigned int IsEqualIntNone(Quatf_In inQuat1, Quatf_In inQuat2);
	unsigned int IsCloseAll(Quatf_In inQuat1, Quatf_In inQuat2, float epsValue);
	unsigned int IsCloseAll(Quatf_In inQuat1, Quatf_In inQuat2, Vec4f_In epsValues);
	unsigned int IsCloseNone(Quatf_In inQuat1, Quatf_In inQuat2, float epsValue);
	unsigned int IsCloseNone(Quatf_In inQuat1, Quatf_In inQuat2, Vec4f_In epsValues);
	unsigned int IsGreaterThanAll(Quatf_In bigQuat, Quatf_In smallQuat);
	unsigned int IsGreaterThanOrEqualAll(Quatf_In bigQuat, Quatf_In smallQuat);
	unsigned int IsLessThanAll(Quatf_In smallQuat, Quatf_In bigQuat);
	unsigned int IsLessThanOrEqualAll(Quatf_In smallQuat, Quatf_In bigQuat);

	unsigned int IsZeroAll(Vec2f_In inVector);
	unsigned int IsZeroNone(Vec2f_In inVector);
	unsigned int IsBetweenNegAndPosBounds( Vec2f_In inVector, Vec2f_In boundsVector );
	unsigned int IsEqualAll(Vec2f_In inVector1, Vec2f_In inVector2);
	unsigned int IsEqualNone(Vec2f_In inVector1, Vec2f_In inVector2);
	unsigned int IsEqualIntAll(Vec2f_In inVector1, Vec2f_In inVector2);
	unsigned int IsEqualIntNone(Vec2f_In inVector1, Vec2f_In inVector2);
	unsigned int IsCloseAll(Vec2f_In inVector1, Vec2f_In inVector2, float epsValue);
	unsigned int IsCloseAll(Vec2f_In inVector1, Vec2f_In inVector2, Vec2f_In epsValues);
	unsigned int IsCloseNone(Vec2f_In inVector1, Vec2f_In inVector2, float epsValue);
	unsigned int IsCloseNone(Vec2f_In inVector1, Vec2f_In inVector2, Vec2f_In epsValues);
	unsigned int IsGreaterThanAll(Vec2f_In bigVector, Vec2f_In smallVector);
	unsigned int IsGreaterThanOrEqualAll(Vec2f_In bigVector, Vec2f_In smallVector);
	unsigned int IsLessThanAll(Vec2f_In smallVector, Vec2f_In bigVector);
	unsigned int IsLessThanOrEqualAll(Vec2f_In smallVector, Vec2f_In bigVector);

	unsigned int IsZeroAll(Vec3f_In inVector);
	unsigned int IsZeroNone(Vec3f_In inVector);
	unsigned int IsBetweenNegAndPosBounds( Vec3f_In inVector, Vec3f_In boundsVector );
	unsigned int IsEqualAll(Vec3f_In inVector1, Vec3f_In inVector2);
	unsigned int IsEqualNone(Vec3f_In inVector1, Vec3f_In inVector2);
	unsigned int IsEqualIntAll(Vec3f_In inVector1, Vec3f_In inVector2);
	unsigned int IsEqualIntNone(Vec3f_In inVector1, Vec3f_In inVector2);
	unsigned int IsCloseAll(Vec3f_In inVector1, Vec3f_In inVector2, float epsValue);
	unsigned int IsCloseAll(Vec3f_In inVector1, Vec3f_In inVector2, Vec3f_In epsValues);
	unsigned int IsCloseNone(Vec3f_In inVector1, Vec3f_In inVector2, float epsValue);
	unsigned int IsCloseNone(Vec3f_In inVector1, Vec3f_In inVector2, Vec3f_In epsValues);
	unsigned int IsGreaterThanAll(Vec3f_In bigVector, Vec3f_In smallVector);
	unsigned int IsGreaterThanOrEqualAll(Vec3f_In bigVector, Vec3f_In smallVector);
	unsigned int IsLessThanAll(Vec3f_In smallVector, Vec3f_In bigVector);
	unsigned int IsLessThanOrEqualAll(Vec3f_In smallVector, Vec3f_In bigVector);

	unsigned int IsZeroAll(Vec4f_In inVector);
	unsigned int IsZeroNone(Vec4f_In inVector);
	unsigned int IsBetweenNegAndPosBounds( Vec4f_In inVector, Vec4f_In boundsVector );
	unsigned int IsEqualAll(Vec4f_In inVector1, Vec4f_In inVector2);
	unsigned int IsEqualNone(Vec4f_In inVector1, Vec4f_In inVector2);
	unsigned int IsEqualIntAll(Vec4f_In inVector1, Vec4f_In inVector2);
	unsigned int IsEqualIntNone(Vec4f_In inVector1, Vec4f_In inVector2);
	unsigned int IsCloseAll(Vec4f_In inVector1, Vec4f_In inVector2, float epsValue);
	unsigned int IsCloseAll(Vec4f_In inVector1, Vec4f_In inVector2, Vec4f_In epsValues);
	unsigned int IsCloseNone(Vec4f_In inVector1, Vec4f_In inVector2, float epsValue);
	unsigned int IsCloseNone(Vec4f_In inVector1, Vec4f_In inVector2, Vec4f_In epsValues);
	unsigned int IsGreaterThanAll(Vec4f_In bigVector, Vec4f_In smallVector);
	unsigned int IsGreaterThanOrEqualAll(Vec4f_In bigVector, Vec4f_In smallVector);
	unsigned int IsLessThanAll(Vec4f_In smallVector, Vec4f_In bigVector);
	unsigned int IsLessThanOrEqualAll(Vec4f_In smallVector, Vec4f_In bigVector);

	//============================================================================
	// Conversion functions

	template <int exponent>
	Vec2f_Out FloatToIntRaw(Vec2f_In inVec);
	template <int exponent>
	Vec2f_Out IntToFloatRaw(Vec2f_In inVec);
	Vec2f_Out RoundToNearestInt(Vec2f_In inVec);
	Vec2f_Out RoundToNearestIntZero(Vec2f_In inVec);
	Vec2f_Out RoundToNearestIntNegInf(Vec2f_In inVec);
	Vec2f_Out RoundToNearestIntPosInf(Vec2f_In inVec);

	template <int exponent>
	Vec3f_Out FloatToIntRaw(Vec3f_In inVec);
	template <int exponent>
	Vec3f_Out IntToFloatRaw(Vec3f_In inVec);
	Vec3f_Out RoundToNearestInt(Vec3f_In inVec);
	Vec3f_Out RoundToNearestIntZero(Vec3f_In inVec);
	Vec3f_Out RoundToNearestIntNegInf(Vec3f_In inVec);
	Vec3f_Out RoundToNearestIntPosInf(Vec3f_In inVec);

	template <int exponent>
	Vec4f_Out FloatToIntRaw(Vec4f_In inVec);
	template <int exponent>
	Vec4f_Out IntToFloatRaw(Vec4f_In inVec);
	Vec4f_Out RoundToNearestInt(Vec4f_In inVec);
	Vec4f_Out RoundToNearestIntZero(Vec4f_In inVec);
	Vec4f_Out RoundToNearestIntNegInf(Vec4f_In inVec);
	Vec4f_Out RoundToNearestIntPosInf(Vec4f_In inVec);

	//============================================================================
	// Standard quaternion math

	Quatf_Out Conjugate(Quatf_In inQuat);
	Quatf_Out Normalize(Quatf_In inQuat);
	Quatf_Out NormalizeSafe(Quatf_In inQuat, float errVal);
	Quatf_Out NormalizeFast(Quatf_In inQuat);
	Quatf_Out NormalizeFastSafe(Quatf_In inQuat, float errVal);
	Quatf_Out Invert(Quatf_In inQuat);
	Quatf_Out InvertSafe(Quatf_In inQuat, float errVal = LARGE_FLOAT);
	Quatf_Out InvertFast(Quatf_In inQuat);
	Quatf_Out InvertFastSafe(Quatf_In inQuat, float errVal = LARGE_FLOAT);
	// InvertNormInput() is fastest if the input is already a unit quat. Else, Invert() is faster
	// than a Normalize() followed by a InvertNormInput().
	Quatf_Out InvertNormInput(Quatf_In inQuat);

	float Dot( Quatf_In inQuat1, Quatf_In inQuat2 );
	Quatf_Out Multiply( Quatf_In inQuat1, Quatf_In inQuat2 );
	Quatf_Out SlerpNear( float t, Quatf_In inNormQuat1, Quatf_In inNormQuat2 );
	Quatf_Out Slerp( float t, Quatf_In inNormQuat1, Quatf_In inNormQuat2 );
	// Faster, but a non-constant velocity. Still fine if you have a substantial amount of animation keyframes.
	Quatf_Out Nlerp( float t, Quatf_In inNormQuat1, Quatf_In inNormQuat2 );

	Quatf_Out PrepareSlerp( Quatf_In quat1, Quatf_In quatToNegate );

	// Returns the angle in radians, splatted.
	void ToAxisAngle( Vec3f_InOut Axis, float& radians, Quatf_In inQuat );
	// Returns the angle in radians, splatted.
	float GetAngle( Quatf_In inQuat );

	//============================================================================
	// Standard algebra

	Vec2f_Out Clamp( Vec2f_In inVect, Vec2f_In lowBound, Vec2f_In highBound );
	Vec3f_Out Clamp( Vec3f_In inVect, Vec3f_In lowBound, Vec3f_In highBound );
	Vec4f_Out Clamp( Vec4f_In inVect, Vec4f_In lowBound, Vec4f_In highBound );

	Vec2f_Out Saturate( Vec2f_In inVect );
	Vec3f_Out Saturate( Vec3f_In inVect );
	Vec4f_Out Saturate( Vec4f_In inVect );

	Vec3f_Out ClampMag( Vec3f_In inVect, float minMag, float maxMag );

	Vec2f_Out Negate(Vec2f_In inVect);
	Vec3f_Out Negate(Vec3f_In inVect);
	Vec4f_Out Negate(Vec4f_In inVect);

	Vec2f_Out InvertBits(Vec2f_In inVect);
	Vec3f_Out InvertBits(Vec3f_In inVect);
	Vec4f_Out InvertBits(Vec4f_In inVect);
	Vec2f_Out Invert(Vec2f_In inVect);
	Vec3f_Out Invert(Vec3f_In inVect);
	Vec4f_Out Invert(Vec4f_In inVect);
	Vec2f_Out InvertSafe(Vec2f_In inVect, float errVal = LARGE_FLOAT);
	Vec3f_Out InvertSafe(Vec3f_In inVect, float errVal = LARGE_FLOAT);
	Vec4f_Out InvertSafe(Vec4f_In inVect, float errVal = LARGE_FLOAT);
	Vec2f_Out InvertFast(Vec2f_In inVect);
	Vec3f_Out InvertFast(Vec3f_In inVect);
	Vec4f_Out InvertFast(Vec4f_In inVect);
	Vec2f_Out InvertFastSafe(Vec2f_In inVect, float errVal = LARGE_FLOAT);
	Vec3f_Out InvertFastSafe(Vec3f_In inVect, float errVal = LARGE_FLOAT);
	Vec4f_Out InvertFastSafe(Vec4f_In inVect, float errVal = LARGE_FLOAT);

	//============================================================================
	// Magnitude

	Vec2f_Out Abs(Vec2f_In inVect);
	Vec3f_Out Abs(Vec3f_In inVect);
	Vec4f_Out Abs(Vec4f_In inVect);

	Vec2f_Out Sqrt(Vec2f_In inVect);
	Vec3f_Out Sqrt(Vec3f_In inVect);
	Vec4f_Out Sqrt(Vec4f_In inVect);
	Vec2f_Out SqrtSafe(Vec2f_In inVect, float errVal = 0.0f);
	Vec3f_Out SqrtSafe(Vec3f_In inVect, float errVal = 0.0f);
	Vec4f_Out SqrtSafe(Vec4f_In inVect, float errVal = 0.0f);
	Vec2f_Out SqrtFast(Vec2f_In inVect);
	Vec3f_Out SqrtFast(Vec3f_In inVect);
	Vec4f_Out SqrtFast(Vec4f_In inVect);
	Vec2f_Out SqrtFastSafe(Vec2f_In inVect, float errVal = 0.0f);
	Vec3f_Out SqrtFastSafe(Vec3f_In inVect, float errVal = 0.0f);
	Vec4f_Out SqrtFastSafe(Vec4f_In inVect, float errVal = 0.0f);

	Vec2f_Out InvSqrt(Vec2f_In inVect);
	Vec3f_Out InvSqrt(Vec3f_In inVect);
	Vec4f_Out InvSqrt(Vec4f_In inVect);
	Vec2f_Out InvSqrtSafe(Vec2f_In inVect, float errVal = LARGE_FLOAT);
	Vec3f_Out InvSqrtSafe(Vec3f_In inVect, float errVal = LARGE_FLOAT);
	Vec4f_Out InvSqrtSafe(Vec4f_In inVect, float errVal = LARGE_FLOAT);
	Vec2f_Out InvSqrtFast(Vec2f_In inVect);
	Vec3f_Out InvSqrtFast(Vec3f_In inVect);
	Vec4f_Out InvSqrtFast(Vec4f_In inVect);
	Vec2f_Out InvSqrtFastSafe(Vec2f_In inVect, float errVal = LARGE_FLOAT);
	Vec3f_Out InvSqrtFastSafe(Vec3f_In inVect, float errVal = LARGE_FLOAT);
	Vec4f_Out InvSqrtFastSafe(Vec4f_In inVect, float errVal = LARGE_FLOAT);

	float Mag(Vec2f_In inVect);
	float Mag(Vec3f_In inVect);
	float Mag(Vec4f_In inVect);
	float MagFast(Vec2f_In inVect);
	float MagFast(Vec3f_In inVect);
	float MagFast(Vec4f_In inVect);
	float MagSquared(Vec2f_In inVect);
	float MagSquared(Vec3f_In inVect);
	float MagSquared(Vec4f_In inVect);
	float InvMag(Vec2f_In inVect);
	float InvMag(Vec3f_In inVect);
	float InvMag(Vec4f_In inVect);
	float InvMagSafe(Vec2f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagSafe(Vec3f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagSafe(Vec4f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagFast(Vec2f_In inVect);
	float InvMagFast(Vec3f_In inVect);
	float InvMagFast(Vec4f_In inVect);
	float InvMagFastSafe(Vec2f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagFastSafe(Vec3f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagFastSafe(Vec4f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagSquared(Vec2f_In inVect);
	float InvMagSquared(Vec3f_In inVect);
	float InvMagSquared(Vec4f_In inVect);
	float InvMagSquaredSafe(Vec2f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagSquaredSafe(Vec3f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagSquaredSafe(Vec4f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagSquaredFast(Vec2f_In inVect);
	float InvMagSquaredFast(Vec3f_In inVect);
	float InvMagSquaredFast(Vec4f_In inVect);
	float InvMagSquaredFastSafe(Vec2f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagSquaredFastSafe(Vec3f_In inVect, float errVal = LARGE_FLOAT);
	float InvMagSquaredFastSafe(Vec4f_In inVect, float errVal = LARGE_FLOAT);

	Vec2f_Out Normalize(Vec2f_In inVect);
	Vec3f_Out Normalize(Vec3f_In inVect);
	Vec4f_Out Normalize(Vec4f_In inVect);
	Vec2f_Out NormalizeSafe(Vec2f_In inVect, float errVal);
	Vec3f_Out NormalizeSafe(Vec3f_In inVect, float errVal);
	Vec4f_Out NormalizeSafe(Vec4f_In inVect, float errVal);
	Vec2f_Out NormalizeFast(Vec2f_In inVect);
	Vec3f_Out NormalizeFast(Vec3f_In inVect);
	Vec4f_Out NormalizeFast(Vec4f_In inVect);
	Vec2f_Out NormalizeFastSafe(Vec2f_In inVect, float errVal);
	Vec3f_Out NormalizeFastSafe(Vec3f_In inVect, float errVal);
	Vec4f_Out NormalizeFastSafe(Vec4f_In inVect, float errVal);

	//============================================================================
	// Angular

	Vec2f_Out Extend( Vec2f_In inVect, Vec2f_In amount );
	Vec2f_Out Rotate( Vec2f_In inVect, float radians );
	Vec2f_Out Reflect( Vec2f_In inVect, Vec2f_In wall2DNormal );
	Vec2f_Out ApproachStraight(Vec2f_In inVect, Vec2f_In goal, float rate, float time, unsigned int& rResult);

	// The amount is assumed to be in each of amount.
	Vec3f_Out Extend( Vec3f_In inVect, Vec3f_In amount );
	// assumes radians has the same value splatted in .x/.y/.z.
	Vec3f_Out RotateAboutXAxis( Vec3f_In inVect, float radians );
	Vec3f_Out RotateAboutYAxis( Vec3f_In inVect, float radians );
	Vec3f_Out RotateAboutZAxis( Vec3f_In inVect, float radians );

	// assumes the current vector and planeNormal are normalized.
	Vec3f_Out Reflect( Vec3f_In inVect, Vec3f_In planeNormal );

	Vec4f_Out SlowInOut( Vec4f_In t );
	Vec3f_Out SlowInOut( Vec3f_In t );
	Vec2f_Out SlowInOut( Vec2f_In t );
	Vec4f_Out SlowIn( Vec4f_In t );
	Vec3f_Out SlowIn( Vec3f_In t );
	Vec2f_Out SlowIn( Vec2f_In t );
	Vec4f_Out SlowOut( Vec4f_In t );
	Vec3f_Out SlowOut( Vec3f_In t );
	Vec2f_Out SlowOut( Vec2f_In t );
	Vec4f_Out BellInOut( Vec4f_In t );
	Vec3f_Out BellInOut( Vec3f_In t );
	Vec2f_Out BellInOut( Vec2f_In t );
	Vec4f_Out Range( Vec4f_In t, Vec4f_In lower, Vec4f_In upper );
	Vec3f_Out Range( Vec3f_In t, Vec3f_In lower, Vec3f_In upper );
	Vec2f_Out Range( Vec2f_In t, Vec2f_In lower, Vec2f_In upper );
	Vec4f_Out RangeFast( Vec4f_In t, Vec4f_In lower, Vec4f_In upper );
	Vec3f_Out RangeFast( Vec3f_In t, Vec3f_In lower, Vec3f_In upper );
	Vec2f_Out RangeFast( Vec2f_In t, Vec2f_In lower, Vec2f_In upper );
	Vec4f_Out RangeClamp( Vec4f_In t, Vec4f_In lower, Vec4f_In upper );
	Vec3f_Out RangeClamp( Vec3f_In t, Vec3f_In lower, Vec3f_In upper );
	Vec2f_Out RangeClamp( Vec2f_In t, Vec2f_In lower, Vec2f_In upper );
	Vec4f_Out RangeClampFast( Vec4f_In t, Vec4f_In lower, Vec4f_In upper );
	Vec3f_Out RangeClampFast( Vec3f_In t, Vec3f_In lower, Vec3f_In upper );
	Vec2f_Out RangeClampFast( Vec2f_In t, Vec2f_In lower, Vec2f_In upper );
	Vec4f_Out Ramp( Vec4f_In x, Vec4f_In funcInA, Vec4f_In funcInB, Vec4f_In funcOutA, Vec4f_In funcOutB );
	Vec3f_Out Ramp( Vec3f_In x, Vec3f_In funcInA, Vec3f_In funcInB, Vec3f_In funcOutA, Vec3f_In funcOutB );
	Vec2f_Out Ramp( Vec2f_In x, Vec2f_In funcInA, Vec2f_In funcInB, Vec2f_In funcOutA, Vec2f_In funcOutB );
	Vec4f_Out RampFast( Vec4f_In x, Vec4f_In funcInA, Vec4f_In funcInB, Vec4f_In funcOutA, Vec4f_In funcOutB );
	Vec3f_Out RampFast( Vec3f_In x, Vec3f_In funcInA, Vec3f_In funcInB, Vec3f_In funcOutA, Vec3f_In funcOutB );
	Vec2f_Out RampFast( Vec2f_In x, Vec2f_In funcInA, Vec2f_In funcInB, Vec2f_In funcOutA, Vec2f_In funcOutB );

	Vec3f_Out AddNet( Vec3f_In inVector, Vec3f_In toAdd );
	float Angle(Vec3f_In v1, Vec3f_In v2);
	float AngleNormInput(Vec3f_In v1, Vec3f_In v2);
	float AngleX(Vec3f_In v1, Vec3f_In v2);
	float AngleY(Vec3f_In v1, Vec3f_In v2);
	float AngleZ(Vec3f_In v1, Vec3f_In v2);
	float AngleXNormInput(Vec3f_In v1, Vec3f_In v2);
	float AngleYNormInput(Vec3f_In v1, Vec3f_In v2);
	float AngleZNormInput(Vec3f_In v1, Vec3f_In v2);
	void MakeOrthonormals(Vec3f_In inVector, Vec3f_InOut ortho1, Vec3f_InOut ortho2);
	// rate and time should be one value splatted into .x/.y/.z
	Vec3f_Out ApproachStraight(Vec3f_In inVect, Vec3f_In goal, float rate, float time, unsigned int& rResult);

	Vec2f_Out AddNet( Vec2f_In inVector, Vec2f_In toAdd );
	Vec2f_Out Angle(Vec2f_In v1, Vec2f_In v2);
	Vec2f_Out AngleNormInput(Vec2f_In v1, Vec2f_In v2);
	float WhichSideOfLine(Vec2f_In point, Vec2f_In lineP1, Vec2f_In lineP2);

	Vec4f_Out Splat( float a );

	// General vector math.
	Vec4f_Out Scale( Vec4f_In a, float b );
	Vec4f_Out Scale( float a, Vec4f_In b );
	Vec3f_Out Scale( Vec3f_In a, float b );
	Vec3f_Out Scale( float a, Vec3f_In b );
	Vec2f_Out Scale( Vec2f_In a, float b );
	Vec2f_Out Scale( float a, Vec2f_In b );

	Vec4f_Out Scale( Vec4f_In a, Vec4f_In b );
	Vec3f_Out Scale( Vec3f_In a, Vec3f_In b );
	Vec2f_Out Scale( Vec2f_In a, Vec2f_In b );

	// This function sets w = 0, since it happens to be a free vectorized operation, so the behavior is preserved here for consistency.
	Vec4f_Out Cross3( Vec4f_In a, Vec4f_In b );
	// This function sets w = 0, since it happens to be a free vectorized operation, so the behavior is preserved here for consistency.
	Vec3f_Out Cross( Vec3f_In a, Vec3f_In b );
	float Cross( Vec2f_In a, Vec2f_In b );

	// This function sets w = toAddTo.w, since it happens to be a free vectorized operation. (If this is desirable, go ahead and take advantage.)
	Vec3f_Out AddCrossed( Vec3f_In toAddTo, Vec3f_In a, Vec3f_In b );
	// This function sets w = toAddTo.w, since it happens to be a free vectorized operation. (If this is desirable, go ahead and take advantage.)
	Vec4f_Out AddCrossed3( Vec4f_In toAddTo, Vec4f_In a, Vec4f_In b );
	// This function sets w = toSubtractFrom.w, since it happens to be a free vectorized operation. (If this is desirable, go ahead and take advantage.)
	Vec3f_Out SubtractCrossed( Vec3f_In toSubtractFrom, Vec3f_In a, Vec3f_In b );
	// This function sets w = toSubtractFrom.w, since it happens to be a free vectorized operation. (If this is desirable, go ahead and take advantage.)
	Vec4f_Out SubtractCrossed3( Vec4f_In toSubtractFrom, Vec4f_In a, Vec4f_In b );

	float Dot( Vec4f_In a, Vec4f_In b );
	float Dot( Vec3f_In a, Vec3f_In b );
	float Dot( Vec2f_In a, Vec2f_In b );

	Vec4f_Out AddInt( Vec4f_In a, Vec4f_In b );
	Vec3f_Out AddInt( Vec3f_In a, Vec3f_In b );
	Vec2f_Out AddInt( Vec2f_In a, Vec2f_In b );
	Vec4f_Out SubtractInt( Vec4f_In a, Vec4f_In b );
	Vec3f_Out SubtractInt( Vec3f_In a, Vec3f_In b );
	Vec2f_Out SubtractInt( Vec2f_In a, Vec2f_In b );

	Vec4f_Out Add( Vec4f_In a, Vec4f_In b );
	Vec3f_Out Add( Vec3f_In a, Vec3f_In b );
	Vec2f_Out Add( Vec2f_In a, Vec2f_In b );
	Vec4f_Out Subtract( Vec4f_In a, Vec4f_In b );
	Vec3f_Out Subtract( Vec3f_In a, Vec3f_In b );
	Vec2f_Out Subtract( Vec2f_In a, Vec2f_In b );
	Vec4f_Out Average( Vec4f_In a, Vec4f_In b );
	Vec3f_Out Average( Vec3f_In a, Vec3f_In b );
	Vec2f_Out Average( Vec2f_In a, Vec2f_In b );

	Vec4f_Out AddScaled( Vec4f_In toAdd, Vec4f_In toScaleThenAdd, float scaleValue );
	Vec4f_Out AddScaled( Vec4f_In toAdd, Vec4f_In toScaleThenAdd, Vec4f_In scaleValues );
	Vec3f_Out AddScaled( Vec3f_In toAdd, Vec3f_In toScaleThenAdd, float scaleValue );
	Vec3f_Out AddScaled( Vec3f_In toAdd, Vec3f_In toScaleThenAdd, Vec3f_In scaleValues );
	Vec2f_Out AddScaled( Vec2f_In toAdd, Vec2f_In toScaleThenAdd, float scaleValue );
	Vec2f_Out AddScaled( Vec2f_In toAdd, Vec2f_In toScaleThenAdd, Vec2f_In scaleValues );
	Vec4f_Out SubtractScaled( Vec4f_In toSubtractFrom, Vec4f_In toScaleThenSubtract, float scaleValue );
	Vec4f_Out SubtractScaled( Vec4f_In toSubtractFrom, Vec4f_In toScaleThenSubtract, Vec4f_In scaleValues );
	Vec3f_Out SubtractScaled( Vec3f_In toSubtractFrom, Vec3f_In toScaleThenSubtract, float scaleValue );
	Vec3f_Out SubtractScaled( Vec3f_In toSubtractFrom, Vec3f_In toScaleThenSubtract, Vec3f_In scaleValues );
	Vec2f_Out SubtractScaled( Vec2f_In toSubtractFrom, Vec2f_In toScaleThenSubtract, float scaleValue );
	Vec2f_Out SubtractScaled( Vec2f_In toSubtractFrom, Vec2f_In toScaleThenSubtract, Vec2f_In scaleValues );
	Vec4f_Out InvScale( Vec4f_In toScale, float scaleValue );
	Vec4f_Out InvScale( Vec4f_In toScale, Vec4f_In scaleValues );
	Vec4f_Out InvScaleSafe( Vec4f_In toScale, float scaleValue, float errVal = LARGE_FLOAT );
	Vec4f_Out InvScaleSafe( Vec4f_In toScale, Vec4f_In scaleValues, float errVal = LARGE_FLOAT );
	Vec4f_Out InvScaleFast( Vec4f_In toScale, float scaleValue );
	Vec4f_Out InvScaleFast( Vec4f_In toScale, Vec4f_In scaleValues );
	Vec4f_Out InvScaleFastSafe( Vec4f_In toScale, float scaleValue, float errVal = LARGE_FLOAT );
	Vec4f_Out InvScaleFastSafe( Vec4f_In toScale, Vec4f_In scaleValues, float errVal = LARGE_FLOAT );
	Vec3f_Out InvScale( Vec3f_In toScale, float scaleValue );
	Vec3f_Out InvScale( Vec3f_In toScale, Vec3f_In scaleValues );
	Vec3f_Out InvScaleSafe( Vec3f_In toScale, float scaleValue, float errVal = LARGE_FLOAT );
	Vec3f_Out InvScaleSafe( Vec3f_In toScale, Vec3f_In scaleValues, float errVal = LARGE_FLOAT );
	Vec3f_Out InvScaleFast( Vec3f_In toScale, float scaleValue );
	Vec3f_Out InvScaleFast( Vec3f_In toScale, Vec3f_In scaleValues );
	Vec3f_Out InvScaleFastSafe( Vec3f_In toScale, float scaleValue, float errVal = LARGE_FLOAT );
	Vec3f_Out InvScaleFastSafe( Vec3f_In toScale, Vec3f_In scaleValues, float errVal = LARGE_FLOAT );
	Vec2f_Out InvScale( Vec2f_In toScale, float scaleValue );
	Vec2f_Out InvScale( Vec2f_In toScale, Vec2f_In scaleValues );
	Vec2f_Out InvScaleSafe( Vec2f_In toScale, float scaleValue, float errValVect = LARGE_FLOAT );
	Vec2f_Out InvScaleSafe( Vec2f_In toScale, Vec2f_In scaleValues, float errVal = LARGE_FLOAT );
	Vec2f_Out InvScaleFast( Vec2f_In toScale, float scaleValue );
	Vec2f_Out InvScaleFast( Vec2f_In toScale, Vec2f_In scaleValues );
	Vec2f_Out InvScaleFastSafe( Vec2f_In toScale, float scaleValue, float errVal = LARGE_FLOAT );
	Vec2f_Out InvScaleFastSafe( Vec2f_In toScale, Vec2f_In scaleValues, float errVal = LARGE_FLOAT );

	Vec4f_Out Lerp( float tValue, Vec4f_In vectA, Vec4f_In vectB );
	Vec4f_Out Lerp( Vec4f_In tValues, Vec4f_In vectA, Vec4f_In vectB );
	Vec3f_Out Lerp( float tValue, Vec3f_In vectA, Vec3f_In vectB );
	Vec3f_Out Lerp( Vec3f_In tValues, Vec3f_In vectA, Vec3f_In vectB );
	Vec2f_Out Lerp( float tValue, Vec2f_In vectA, Vec2f_In vectB );
	Vec2f_Out Lerp( Vec2f_In tValues, Vec2f_In vectA, Vec2f_In vectB );

	Vec4f_Out Pow( Vec4f_In x, Vec4f_In y );
	Vec3f_Out Pow( Vec3f_In x, Vec3f_In y );
	Vec2f_Out Pow( Vec2f_In x, Vec2f_In y );
	Vec4f_Out Expt( Vec4f_In x );
	Vec3f_Out Expt( Vec3f_In x );
	Vec2f_Out Expt( Vec2f_In x );
	Vec4f_Out Log2( Vec4f_In x );
	Vec3f_Out Log2( Vec3f_In x );
	Vec2f_Out Log2( Vec2f_In x );
	Vec4f_Out Log10( Vec4f_In x );
	Vec3f_Out Log10( Vec3f_In x );
	Vec2f_Out Log10( Vec2f_In x );

	float Dist(Vec4f_In a, Vec4f_In b);
	float DistFast(Vec4f_In a, Vec4f_In b);
	float Dist(Vec3f_In a, Vec3f_In b);
	float DistFast(Vec3f_In a, Vec3f_In b);
	float Dist(Vec2f_In a, Vec2f_In b);
	float DistFast(Vec2f_In a, Vec2f_In b);
	float InvDist(Vec4f_In a, Vec4f_In b);
	float InvDistSafe(Vec4f_In a, Vec4f_In b, float errVal = LARGE_FLOAT);
	float InvDistFast(Vec4f_In a, Vec4f_In b);
	float InvDistFastSafe(Vec4f_In a, Vec4f_In b, float errVal = LARGE_FLOAT);
	float InvDist(Vec3f_In a, Vec3f_In b);
	float InvDistSafe(Vec3f_In a, Vec3f_In b, float errVal = LARGE_FLOAT);
	float InvDistFast(Vec3f_In a, Vec3f_In b);
	float InvDistFastSafe(Vec3f_In a, Vec3f_In b, float errVal = LARGE_FLOAT);
	float InvDist(Vec2f_In a, Vec2f_In b);
	float InvDistSafe(Vec2f_In a, Vec2f_In b, float errVal = LARGE_FLOAT);
	float InvDistFast(Vec2f_In a, Vec2f_In b);
	float InvDistFastSafe(Vec2f_In a, Vec2f_In b, float errVal = LARGE_FLOAT);
	float DistSquared(Vec4f_In a, Vec4f_In b);
	float DistSquared(Vec3f_In a, Vec3f_In b);
	float DistSquared(Vec2f_In a, Vec2f_In b);
	float InvDistSquared(Vec4f_In a, Vec4f_In b);
	float InvDistSquaredSafe(Vec4f_In a, Vec4f_In b, float errVal = LARGE_FLOAT);
	float InvDistSquaredFast(Vec4f_In a, Vec4f_In b);
	float InvDistSquaredFastSafe(Vec4f_In a, Vec4f_In b, float errVal = LARGE_FLOAT);
	float InvDistSquared(Vec3f_In a, Vec3f_In b);
	float InvDistSquaredSafe(Vec3f_In a, Vec3f_In b, float errVal = LARGE_FLOAT);
	float InvDistSquaredFast(Vec3f_In a, Vec3f_In b);
	float InvDistSquaredFastSafe(Vec3f_In a, Vec3f_In b, float errVal = LARGE_FLOAT);
	float InvDistSquared(Vec2f_In a, Vec2f_In b);
	float InvDistSquaredSafe(Vec2f_In a, Vec2f_In b, float errVal = LARGE_FLOAT);
	float InvDistSquaredFast(Vec2f_In a, Vec2f_In b);
	float InvDistSquaredFastSafe(Vec2f_In a, Vec2f_In b, float errVal = LARGE_FLOAT);

	Vec4f_Out Max(Vec4f_In inVector1, Vec4f_In inVector2);
	Vec3f_Out Max(Vec3f_In inVector1, Vec3f_In inVector2);
	Vec2f_Out Max(Vec2f_In inVector1, Vec2f_In inVector2);
	Vec4f_Out Min(Vec4f_In inVector1, Vec4f_In inVector2);
	Vec3f_Out Min(Vec3f_In inVector1, Vec3f_In inVector2);
	Vec2f_Out Min(Vec2f_In inVector1, Vec2f_In inVector2);

	Vec4f_Out And(Vec4f_In inVector1, Vec4f_In inVector2);
	Vec3f_Out And(Vec3f_In inVector1, Vec3f_In inVector2);
	Vec2f_Out And(Vec2f_In inVector1, Vec2f_In inVector2);
	Vec4f_Out Or(Vec4f_In inVector1, Vec4f_In inVector2);
	Vec3f_Out Or(Vec3f_In inVector1, Vec3f_In inVector2);
	Vec2f_Out Or(Vec2f_In inVector1, Vec2f_In inVector2);
	Vec4f_Out Xor(Vec4f_In inVector1, Vec4f_In inVector2);
	Vec3f_Out Xor(Vec3f_In inVector1, Vec3f_In inVector2);
	Vec2f_Out Xor(Vec2f_In inVector1, Vec2f_In inVector2);
	Vec4f_Out Andc(Vec4f_In inVector1, Vec4f_In inVector2);
	Vec3f_Out Andc(Vec3f_In inVector1, Vec3f_In inVector2);
	Vec2f_Out Andc(Vec2f_In inVector1, Vec2f_In inVector2);

	// Quaternion transforms and inverse transforms.
	// return A * V * A^-1
	Vec3f_Out Transform( Quatf_In unitQuat, Vec3f_In inVect );
	// return A^-1 * V * A
	Vec3f_Out UnTransform( Quatf_In unitQuat, Vec3f_In inVect );

} // namespace rage

#endif