// Note:
//	Matrix property to remember: transpose(AB) = transpose(B)*transpose(A).
//	So, if you pre-multiply point P by matrix M = A*B*C, if A,B,C are orthonormal,
//	you can get the point back from P' via premultiplication by transpose(M), since:
//		transpose(M) = transpose(C)*transpose(B)*transpose(A),
//		and transpose(C)*transpose(B)*transpose(A)*A*B*C*P = IDENTITY*P = P
#ifndef VECTORMATH_CLASSFREEFUNCSV_H
#define VECTORMATH_CLASSFREEFUNCSV_H

#include "system/codecheck.h"

namespace rage
{
	//============================================================================
	// Assertion functions

	//@@rage::IsFiniteAll
	// PURPOSE: Returns true if all components of the vector are finite (not Inf and not NaN)
	//<GROUP rage::Comparisons>
	bool IsFiniteAll( Vec2V_In );
	bool IsFiniteAll( Vec3V_In );
	bool IsFiniteAll( Vec4V_In );
	bool IsFiniteAll( ScalarV_In );
	bool IsFiniteAll( QuatV_In );
	bool IsFiniteAll( Mat33V_In );
	bool IsFiniteAll( Mat34V_In );

	//@@rage::IsFiniteStable
	// PURPOSE: Returns true if all components of the vector are finite (not Inf and not NaN)
	// NOTES:
	//	Uses float pipeline completely (and stable, proven float is-finite methods).
	bool IsFiniteStable( Vec2V_In );
	bool IsFiniteStable( Vec3V_In );
	bool IsFiniteStable( Vec4V_In );
	

	//============================================================================
	// Creation functions

	// PURPOSE: Converts a single 32-bit value into a vector type
	// PARAMS:
	//		in - The value to convert.
	// RETURNS:
	//		A vector containing the input value replicated across all channels.
	// NOTES:
	//		For best performance the 'in' value should be in memory, not in a register.
	// <GROUP rage::FloatConversion>
	FASTRETURNCHECK(ScalarV_Out) ScalarVFromF32( const float& in);	
	FASTRETURNCHECK(ScalarV_Out) ScalarVFromU32( const u32& in);					// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(ScalarV_Out) ScalarVFromS32( const s32& in);					// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec2V_Out)   Vec2VFromF32( const float& in);					// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec2V_Out)   Vec2VFromU32( const u32& in);					// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec2V_Out)   Vec2VFromS32( const s32& in);					// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec3V_Out)   Vec3VFromF32( const float& in);					// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec3V_Out)   Vec3VFromU32( const u32& in);   				// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec3V_Out)   Vec3VFromS32( const s32& in);   				// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec4V_Out)   Vec4VFromF32( const float& in);					// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec4V_Out)   Vec4VFromU32( const u32& in);   				// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(Vec4V_Out)   Vec4VFromS32( const s32& in);   				// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(QuatV_Out)   QuatVFromF32( const float& in);					// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(QuatV_Out)   QuatVFromU32( const u32& in);   				// <COPY rage::ScalarVFromF32>
	FASTRETURNCHECK(QuatV_Out)   QuatVFromS32( const s32& in);   				// <COPY rage::ScalarVFromF32>

	// PURPOSE: Creates a vector from constant values.
	// NOTES: The value is the integral representation of the float. E.g. ScalarVConstant<0x3f800000>()
	template <u32 floatAsInt>
	const FASTRETURNCHECK(ScalarV_Out) ScalarVConstant();

	template <u32 floatAsIntX, u32 floatAsIntY>
	const FASTRETURNCHECK(Vec2V_Out) Vec2VConstant();		// <COPY rage::ScalarVConstant>

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ>
	const FASTRETURNCHECK(Vec3V_Out) Vec3VConstant();		// <COPY rage::ScalarVConstant>

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ, u32 floatAsIntW>
	const FASTRETURNCHECK(Vec4V_Out) Vec4VConstant();		// <COPY rage::ScalarVConstant>

	template <u32 floatAsIntX, u32 floatAsIntY, u32 floatAsIntZ, u32 floatAsIntW>
	const FASTRETURNCHECK(QuatV_Out) QuatVConstant();		// <COPY rage::ScalarVConstant>

	// PURPOSE: Creates a vector by allowing you to specify the contents of each byte
	template <	u8 byte0, u8 byte1, u8 byte2, u8 byte3,
				u8 byte4, u8 byte5, u8 byte6, u8 byte7,
				u8 byte8, u8 byte9, u8 byte10, u8 byte11,
				u8 byte12, u8 byte13, u8 byte14, u8 byte15	>
	const FASTRETURNCHECK(Vec4V_Out) Vec4VConstant();

	//============================================================================
	// Comparison functions.

	// PURPOSE: Tests whether a BoolV represents a true value
	bool IsTrue( BoolV_In b );

	// PURPOSE: Tests whether a BoolV represents a false value
	bool IsFalse( BoolV_In b );

	// PURPOSE: Tests whether a VecBoolV represents four true values
	bool IsTrueAll( VecBoolV_In b );

	// PURPOSE: Tests whether a VecBoolV represents four false values
	bool IsFalseAll( VecBoolV_In b );


	//@@rage::IsFinite
	// PURPOSE: Checks the elements in a vector for INF or NAN (which are not "finite").
	// PARAMS: in1 - the input vector
	// RETURNS: A vector containing bool results for each component
	// NOTE: IsFinite() has shown to be WRONG in one case on MC4. May have been a fluke.
	// Use IsFinite() and IsNotNan() at your own risk! (Assertions will make sure that
	// the results match the trusted IsFiniteAll(float) though, so don't worry too much.)	
	BoolV_Out    IsFinite( ScalarV_In v );							
	VecBoolV_Out IsFinite( Vec2V_In inVector1 );							
	VecBoolV_Out IsFinite( Vec3V_In inVector1 );							
	VecBoolV_Out IsFinite( Vec4V_In inVector1 );							
	VecBoolV_Out IsFinite( QuatV_In inQuat1 );							

	//@@rage::IsNotNan
	// PURPOSE: Checks the values for NAN. This is a bit faster than IsFinite(), so use it when possible.
	// PARAMS: in1 - The input vector
	// RETURNS: A vector containing bool results for each component
	// NOTE: IsFinite() has shown to be WRONG in one case on MC4. May have been a fluke.
	// Use IsFinite() and IsNotNan() at your own risk! (Assertions will make sure that
	// the results match the trusted IsFiniteAll(float) though, so don't worry too much.)	
	BoolV_Out    IsNotNan( ScalarV_In v );
	VecBoolV_Out IsNotNan( Vec2V_In inVector1 );
	VecBoolV_Out IsNotNan( Vec3V_In inVector1 );
	VecBoolV_Out IsNotNan( Vec4V_In inVector1 );
	VecBoolV_Out IsNotNan( QuatV_In inQuat1 );

	//@@rage::IsEqualAll
	// PURPOSE: Tests whether ALL components of two objects are equal
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: Non-zero (true) if all components of in1 are equal to the corresponding components of in2
	unsigned int IsEqualAll(ScalarV_In inVector1,	ScalarV_In inVector2);
	unsigned int IsEqualAll(Vec2V_In inVector1,		Vec2V_In inVector2);
	unsigned int IsEqualAll(Vec3V_In inVector1,		Vec3V_In inVector2);
	unsigned int IsEqualAll(Vec4V_In inVector1,		Vec4V_In inVector2);
	unsigned int IsEqualAll(Mat33V_In inMat1,		Mat33V_In inMat2);
	unsigned int IsEqualAll(Mat34V_In inMat1,		Mat34V_In inMat2);
	unsigned int IsEqualAll(Mat44V_In inMat1,		Mat44V_In inMat2);
	unsigned int IsEqualAll(QuatV_In inQuat1,		QuatV_In inQuat2);

	//@@rage::IsEqualNone
	// PURPOSE: Tests whether NO components of two objects are equal
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: Non-zero (true) if none of components of in1 are equal to the corresponding components of in2
	unsigned int IsEqualNone(ScalarV_In inVector1, ScalarV_In inVector2);
	unsigned int IsEqualNone(Vec2V_In inVector1, Vec2V_In inVector2);
	unsigned int IsEqualNone(Vec3V_In inVector1, Vec3V_In inVector2);
	unsigned int IsEqualNone(Vec4V_In inVector1, Vec4V_In inVector2);
	unsigned int IsEqualNone(Mat33V_In inMat1, Mat33V_In inMat2);
	unsigned int IsEqualNone(Mat34V_In inMat1, Mat34V_In inMat2);
	unsigned int IsEqualNone(Mat44V_In inMat1, Mat44V_In inMat2);
	unsigned int IsEqualNone(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsEqual
	// PURPOSE: Per-component test for equality
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: A vector containing the boolean result (in1.n == in2.n) for all components in in1 and in2.
	BoolV_Out    IsEqual(ScalarV_In inVector1, ScalarV_In inVector2);
	VecBoolV_Out IsEqual(Vec2V_In inVector1, Vec2V_In inVector2);
	VecBoolV_Out IsEqual(Vec3V_In inVector1, Vec3V_In inVector2);
	VecBoolV_Out IsEqual(Vec4V_In inVector1, Vec4V_In inVector2);
	VecBoolV_Out IsEqual(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsCloseAll
	// PURPOSE: Tests whether ALL components of two objects are close to each other (less than epsilon difference)
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	//		epsilon - the epsilon value to compare with
	//		epsVector - per-component epsilon values to compare with
	// RETURNS: Non-zero (true) if all components of in1 are within epsilon of the corresponding 
	//		component of in2.
	// NOTES:
	//		If an epsVector is specified, each component of the input vectors can use a different epsilon.
	//		If the inputs are matrices, the same epsVector will get used for each column of the matrix.
	unsigned int IsCloseAll(ScalarV_In inVector1, ScalarV_In inVector2, ScalarV_In epsilon);
	unsigned int IsCloseAll(Vec2V_In inVector1, Vec2V_In inVector2, ScalarV_In epsilon);
	unsigned int IsCloseAll(Vec2V_In inVector1, Vec2V_In inVector2, Vec2V_In epsVector);
	unsigned int IsCloseAll(Vec3V_In inVector1, Vec3V_In inVector2, ScalarV_In epsilon);
	unsigned int IsCloseAll(Vec3V_In inVector1, Vec3V_In inVector2, Vec3V_In epsVector);
	unsigned int IsCloseAll(Vec4V_In inVector1, Vec4V_In inVector2, ScalarV_In epsilon);
	unsigned int IsCloseAll(Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In epsVector);
	unsigned int IsCloseAll(Mat33V_In inMat1, Mat33V_In inMat2, ScalarV_In epsilon);
	unsigned int IsCloseAll(Mat33V_In inMat1, Mat33V_In inMat2, Vec3V_In epsVector);
	unsigned int IsCloseAll(Mat34V_In inMat1, Mat34V_In inMat2, ScalarV_In epsilon);
	unsigned int IsCloseAll(Mat34V_In inMat1, Mat34V_In inMat2, Vec3V_In epsVector);
	unsigned int IsCloseAll(Mat44V_In inMat1, Mat44V_In inMat2, ScalarV_In epsilon);
	unsigned int IsCloseAll(Mat44V_In inMat1, Mat44V_In inMat2, Vec4V_In epsVector);
	unsigned int IsCloseAll(QuatV_In inQuat1, QuatV_In inQuat2, ScalarV_In epsilon);
	unsigned int IsCloseAll(QuatV_In inQuat1, QuatV_In inQuat2, Vec4V_In epsVector);

	//@@rage::IsCloseNone
	// PURPOSE: Tests whether NO components of two objects are close to each other (less than epsilon difference)
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	//		epsilon - the epsilon value to compare with
	//		epsVector - per-component epsilon values to compare with
	// RETURNS: Non-zero (true) if none of the components of in1 are within epsilon of the corresponding 
	//		component of in2.
	// NOTES:
	//		If an epsVector is specified, each component of the input vectors can use a different epsilon.
	//		If the inputs are matrices, the same epsVector will get used for each column of the matrix.
	unsigned int IsCloseNone(Vec2V_In inVector1, Vec2V_In inVector2, ScalarV_In epsilon);
	unsigned int IsCloseNone(Vec2V_In inVector1, Vec2V_In inVector2, Vec2V_In epsVector);
	unsigned int IsCloseNone(Vec3V_In inVector1, Vec3V_In inVector2, ScalarV_In epsilon);
	unsigned int IsCloseNone(Vec3V_In inVector1, Vec3V_In inVector2, Vec3V_In epsVector);
	unsigned int IsCloseNone(Vec4V_In inVector1, Vec4V_In inVector2, ScalarV_In epsilon);
	unsigned int IsCloseNone(Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In epsVector);
	unsigned int IsCloseNone(Mat33V_In inMat1, Mat33V_In inMat2, ScalarV_In epsilon);
	unsigned int IsCloseNone(Mat33V_In inMat1, Mat33V_In inMat2, Vec3V_In epsVector);
	unsigned int IsCloseNone(Mat34V_In inMat1, Mat34V_In inMat2, ScalarV_In epsilon);
	unsigned int IsCloseNone(Mat34V_In inMat1, Mat34V_In inMat2, Vec3V_In epsVector);
	unsigned int IsCloseNone(Mat44V_In inMat1, Mat44V_In inMat2, ScalarV_In epsilon);
	unsigned int IsCloseNone(Mat44V_In inMat1, Mat44V_In inMat2, Vec4V_In epsVector);
	unsigned int IsCloseNone(QuatV_In inQuat1, QuatV_In inQuat2, ScalarV_In epsilon);
	unsigned int IsCloseNone(QuatV_In inQuat1, QuatV_In inQuat2, Vec4V_In epsVector);

	//@@rage::IsClose
	// PURPOSE: Per-component test for closeness
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	//		epsilon - the epsilon value to compare with
	//		epsVector - per-component epsilon values to compare with
	// RETURNS: A vector containing the boolean result |in1.n - in2.n| < epsilon for all components in in1 and in2
	// NOTES:
	//		If an epsVector is specified, each component of the input vectors can use a different epsilon.
	BoolV_Out    IsClose(ScalarV_In inVector1, ScalarV_In inVector2, ScalarV_In epsilon);
	VecBoolV_Out IsClose(Vec2V_In inVector1, Vec2V_In inVector2, ScalarV_In epsilon);
	VecBoolV_Out IsClose(Vec2V_In inVector1, Vec2V_In inVector2, Vec2V_In epsVector);
	VecBoolV_Out IsClose(Vec3V_In inVector1, Vec3V_In inVector2, ScalarV_In epsilon);
	VecBoolV_Out IsClose(Vec3V_In inVector1, Vec3V_In inVector2, Vec3V_In epsVector);
	VecBoolV_Out IsClose(Vec4V_In inVector1, Vec4V_In inVector2, ScalarV_In epsilon);
	VecBoolV_Out IsClose(Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In epsVector);
	VecBoolV_Out IsClose(QuatV_In inQuat1, QuatV_In inQuat2, ScalarV_In epsilon);
	VecBoolV_Out IsClose(QuatV_In inQuat1, QuatV_In inQuat2, Vec4V_In epsVector);

	//@@rage::IsGreaterThanAll
	// PURPOSE: Tests whether ALL components of in1 are greater than in2
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: Non-zero (true) if all components of in1 are greater than the corresponding components of in2
	unsigned int IsGreaterThanAll(ScalarV_In inVector1, ScalarV_In inVector2);
	unsigned int IsGreaterThanAll(Vec2V_In inVector1, Vec2V_In inVector2);
	unsigned int IsGreaterThanAll(Vec3V_In inVector1, Vec3V_In inVector2);
	unsigned int IsGreaterThanAll(Vec4V_In inVector1, Vec4V_In inVector2);
	unsigned int IsGreaterThanAll(Mat33V_In inMat1, Mat33V_In inMat2);
	unsigned int IsGreaterThanAll(Mat34V_In inMat1, Mat34V_In inMat2);
	unsigned int IsGreaterThanAll(Mat44V_In inMat1, Mat44V_In inMat2);
	unsigned int IsGreaterThanAll(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsGreaterThan
	// PURPOSE: Per-component 'greater than' test
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: A vector containing the boolean result (in1.n > in2.n) for all components in in1 and in2.
	BoolV_Out    IsGreaterThan(ScalarV_In inVector1, ScalarV_In inVector2);
	VecBoolV_Out IsGreaterThan(Vec2V_In inVector1, Vec2V_In inVector2);
	VecBoolV_Out IsGreaterThan(Vec3V_In inVector1, Vec3V_In inVector2);
	VecBoolV_Out IsGreaterThan(Vec4V_In inVector1, Vec4V_In inVector2);
	VecBoolV_Out IsGreaterThan(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsGreaterThanOrEqualAll
	// PURPOSE: Tests whether ALL components of in1 are greater than or equal to in2
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: Non-zero (true) if all components of in1 are greater than or equal to the corresponding components of in2
	unsigned int IsGreaterThanOrEqualAll(ScalarV_In inVector1, ScalarV_In inVector2);
	unsigned int IsGreaterThanOrEqualAll(Vec2V_In inVector1, Vec2V_In inVector2);
	unsigned int IsGreaterThanOrEqualAll(Vec3V_In inVector1, Vec3V_In inVector2);
	unsigned int IsGreaterThanOrEqualAll(Vec4V_In inVector1, Vec4V_In inVector2);
	unsigned int IsGreaterThanOrEqualAll(Mat33V_In inMat1, Mat33V_In inMat2);
	unsigned int IsGreaterThanOrEqualAll(Mat34V_In inMat1, Mat34V_In inMat2);
	unsigned int IsGreaterThanOrEqualAll(Mat44V_In inMat1, Mat44V_In inMat2);
	unsigned int IsGreaterThanOrEqualAll(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsGreaterThanOrEqual
	// PURPOSE: Per-component 'greater than or equal' test
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: A vector containing the boolean result (in1.n >= in2.n) for all components in in1 and in2.
	BoolV_Out    IsGreaterThanOrEqual(ScalarV_In inVector1, ScalarV_In inVector2);
	VecBoolV_Out IsGreaterThanOrEqual(Vec2V_In inVector1, Vec2V_In inVector2);
	VecBoolV_Out IsGreaterThanOrEqual(Vec3V_In inVector1, Vec3V_In inVector2);
	VecBoolV_Out IsGreaterThanOrEqual(Vec4V_In inVector1, Vec4V_In inVector2);
	VecBoolV_Out IsGreaterThanOrEqual(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsLessThanAll
	// PURPOSE: Tests whether ALL components of in1 are less than in2
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: Non-zero (true) if all components of in1 are less than the corresponding components of in2
	unsigned int IsLessThanAll(ScalarV_In inVector1, ScalarV_In inVector2);
	unsigned int IsLessThanAll(Vec2V_In inVector1, Vec2V_In inVector2);
	unsigned int IsLessThanAll(Vec3V_In inVector1, Vec3V_In inVector2);
	unsigned int IsLessThanAll(Vec4V_In inVector1, Vec4V_In inVector2);
	unsigned int IsLessThanAll(Mat33V_In inMat1, Mat33V_In inMat2);
	unsigned int IsLessThanAll(Mat34V_In inMat1, Mat34V_In inMat2);
	unsigned int IsLessThanAll(Mat44V_In inMat1, Mat44V_In inMat2);
	unsigned int IsLessThanAll(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsLessThan
	// PURPOSE: Per-component 'less than' test
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: A vector containing the boolean result (in1.n < in2.n) for all components in in1 and in2.
	BoolV_Out    IsLessThan(ScalarV_In inVector1, ScalarV_In inVector2);
	VecBoolV_Out IsLessThan(Vec2V_In inVector1, Vec2V_In inVector2);
	VecBoolV_Out IsLessThan(Vec3V_In inVector1, Vec3V_In inVector2);
	VecBoolV_Out IsLessThan(Vec4V_In inVector1, Vec4V_In inVector2);
	VecBoolV_Out IsLessThan(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsLessThanOrEqualAll
	// PURPOSE: Tests whether ALL components of in1 are less than or equal to in2
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: Non-zero (true) if all components of in1 are less than or equal to the corresponding components of in2
	unsigned int IsLessThanOrEqualAll(ScalarV_In inVector1, ScalarV_In inVector2);
	unsigned int IsLessThanOrEqualAll(Vec2V_In inVector1, Vec2V_In inVector2);
	unsigned int IsLessThanOrEqualAll(Vec3V_In inVector1, Vec3V_In inVector2);
	unsigned int IsLessThanOrEqualAll(Vec4V_In inVector1, Vec4V_In inVector2);
	unsigned int IsLessThanOrEqualAll(Mat33V_In inMat1, Mat33V_In inMat2);
	unsigned int IsLessThanOrEqualAll(Mat34V_In inMat1, Mat34V_In inMat2);
	unsigned int IsLessThanOrEqualAll(Mat44V_In inMat1, Mat44V_In inMat2);
	unsigned int IsLessThanOrEqualAll(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsLessThanOrEqual
	// PURPOSE: Per-component 'less than or equal' test
	// PARAMS:
	//		in1 - the first comparand
	//		in2 - the second comparand
	// RETURNS: A vector containing the boolean result (in1.n <= in2.n) for all components in in1 and in2.
	BoolV_Out    IsLessThanOrEqual(ScalarV_In inVector1, ScalarV_In inVector2);
	VecBoolV_Out IsLessThanOrEqual(Vec2V_In inVector1, Vec2V_In inVector2);
	VecBoolV_Out IsLessThanOrEqual(Vec3V_In inVector1, Vec3V_In inVector2);
	VecBoolV_Out IsLessThanOrEqual(Vec4V_In inVector1, Vec4V_In inVector2);
	VecBoolV_Out IsLessThanOrEqual(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsZeroAll
	// PURPOSE: Tests whether ALL components of a vector are 0
	// PARAMS:
	//		in - the input vector
	// RETURNS: Non-zero (true) if all components of in are 0.0f
	unsigned int IsZeroAll(ScalarV_In inVector);
	unsigned int IsZeroAll(Vec2V_In inVector);
	unsigned int IsZeroAll(Vec3V_In inVector);
	unsigned int IsZeroAll(Vec4V_In inVector);
	unsigned int IsZeroAll(QuatV_In inQuat);

	//@@rage::IsZeroNone
	// PURPOSE: Tests whether NO components of a vector are 0
	// PARAMS:
	//		in - the input vector
	// RETURNS: Non-zero (true) if none of the components of in are 0.0
	unsigned int IsZeroNone(Vec2V_In inVector);
	unsigned int IsZeroNone(Vec3V_In inVector);
	unsigned int IsZeroNone(Vec4V_In inVector);
	unsigned int IsZeroNone(QuatV_In inQuat);

	//@@rage::IsZero
	// PURPOSE: Per-component 'equal to zero' test
	// PARAMS:
	//		in - the input vector
	// RETURNS: A vector containing the boolean result (in.n != 0) for each component.
	BoolV_Out IsZero(ScalarV_In inVector);
	VecBoolV_Out IsZero(Vec2V_In inVector);
	VecBoolV_Out IsZero(Vec3V_In inVector);
	VecBoolV_Out IsZero(Vec4V_In inVector);
	VecBoolV_Out IsZero(QuatV_In inQuat);

	//@@rage::IsU32NonZero
	// PURPOSE: Tests whether the input has any bits set
	// PARAMS:
	//		input - Integer to be tested
	// RETURNS: A vector with all bits set if there are any set in the input, otherwise the vector has all bits off
	BoolV_Out IsU32NonZero(u32 input);

	//@@rage::IsBetweenNegAndPosBounds
	// PURPOSE: Tests whether a vector is within some distance of 0.0
	// PARAMS:
	//		in - The input vector
	//		bounds - The (positive) maximum bounds
	// RETURNS: Non-zero (true) if -bounds.n <= in.n <= bounds.n for all components n in the inputs
	unsigned int IsBetweenNegAndPosBounds( ScalarV_In inVector, ScalarV_In boundsVector );
	unsigned int IsBetweenNegAndPosBounds( Vec2V_In inVector, Vec2V_In boundsVector );
	unsigned int IsBetweenNegAndPosBounds( Vec3V_In inVector, Vec3V_In boundsVector );
	unsigned int IsBetweenNegAndPosBounds( Vec4V_In inVector, Vec4V_In boundsVector );

	//@@rage::IsEven
	// PURPOSE: Tests whether the whole-number parts of the input vector are even
	// PARAMS:
	//		in - The input vector
	// RETURNS: A vector containing a true component for each component of in that is even
	// NOTES: Uses round-towards-zero rounding on each component of the input vector
	BoolV_Out    IsEven(ScalarV_In inVector);
	VecBoolV_Out IsEven(Vec2V_In inVector);
	VecBoolV_Out IsEven(Vec3V_In inVector);
	VecBoolV_Out IsEven(Vec4V_In inVector);

	//@@rage::IsOdd
	// PURPOSE: Tests whether the whole-number parts of the input vector are odd
	// PARAMS:
	//		in - The input vector
	// RETURNS: A vector containing a true component for each component of in that is odd
	// NOTES: Uses round-towards-zero rounding on each component of the input vector
	BoolV_Out    IsOdd(ScalarV_In inVector);
	VecBoolV_Out IsOdd(Vec2V_In inVector);
	VecBoolV_Out IsOdd(Vec3V_In inVector);
	VecBoolV_Out IsOdd(Vec4V_In inVector);

	//@@rage::SameSignAll
	// PURPOSE: Tests whether ALL of the components of the input vectors have the same sign
	// PARAMS:
	//		in1 - The first comparand
	//		in2 - The second comparand
	// RETURNS: Non-zero (true) if the sign of in1.n equals the sign of in2.n for all components n in the inputs
	unsigned int SameSignAll( ScalarV_In inVector1, ScalarV_In inVector2 );
	unsigned int SameSignAll( Vec2V_In inVector1, Vec2V_In inVector2 );
	unsigned int SameSignAll( Vec3V_In inVector1, Vec3V_In inVector2 );
	unsigned int SameSignAll( Vec4V_In inVector1, Vec4V_In inVector2 );

	//@@rage::SameSign
	// PURPOSE: Tests whether the components in each input vector have the same sign
	// PARAMS:
	//		in1 - The first comparand
	//		in2 - The second comparand
	// RETURNS: A vector containing a true component for each component of in1 that has the same sign as in2
	BoolV_Out    SameSign( ScalarV_In v1, ScalarV_In v2 );
	VecBoolV_Out SameSign( Vec2V_In inVector1, Vec2V_In inVector2 );
	VecBoolV_Out SameSign( Vec3V_In inVector1, Vec3V_In inVector2 );
	VecBoolV_Out SameSign( Vec4V_In inVector1, Vec4V_In inVector2 );


	//@@rage::IsEqualIntAll
	// PURPOSE: Tests whether ALL elements in two vectors containing integer (not float) values are equal
	// PARAMS:
	//		in1 - The first comparand
	//		in2 - The second comparand
	// RETURNS: Non-zero (true) if all elements in in1 are equal to the corresponding element in in2 using an integer equality test
	unsigned int IsEqualIntAll(BoolV_In inVector1, BoolV_In inVector2);
	unsigned int IsEqualIntAll(VecBoolV_In inVector1, VecBoolV_In inVector2);
	unsigned int IsEqualIntAll(ScalarV_In inVector1, ScalarV_In inVector2);
	unsigned int IsEqualIntAll(Vec2V_In inVector1, Vec2V_In inVector2);
	unsigned int IsEqualIntAll(Vec3V_In inVector1, Vec3V_In inVector2);
	unsigned int IsEqualIntAll(Vec4V_In inVector1, Vec4V_In inVector2);
	unsigned int IsEqualIntAll(Mat33V_In inMat1, Mat33V_In inMat2);
	unsigned int IsEqualIntAll(Mat34V_In inMat1, Mat34V_In inMat2);
	unsigned int IsEqualIntAll(Mat44V_In inMat1, Mat44V_In inMat2);
	unsigned int IsEqualIntAll(QuatV_In inQuat1, QuatV_In inQuat2);
	unsigned int IsEqualIntAll(TransformV_In inTrans1, TransformV_In inTrans2);

	//@@rage::IsEqualIntNone
	// PURPOSE: Tests whether NO elements in two vectors containing integer (not float) values are equal
	// PARAMS:
	//		in1 - The first comparand
	//		in2 - The second comparand
	// RETURNS: Non-zero (true) if no elements in in1 are equal to the corresponding element in in2 using an integer equality test
	unsigned int IsEqualIntNone(BoolV_In inVector1, BoolV_In inVector2);
	unsigned int IsEqualIntNone(VecBoolV_In inVector1, VecBoolV_In inVector2);
	unsigned int IsEqualIntNone(ScalarV_In inVector1, ScalarV_In inVector2);
	unsigned int IsEqualIntNone(Vec2V_In inVector1, Vec2V_In inVector2);
	unsigned int IsEqualIntNone(Vec3V_In inVector1, Vec3V_In inVector2);
	unsigned int IsEqualIntNone(Vec4V_In inVector1, Vec4V_In inVector2);
	unsigned int IsEqualIntNone(Mat33V_In inMat1, Mat33V_In inMat2);
	unsigned int IsEqualIntNone(Mat34V_In inMat1, Mat34V_In inMat2);
	unsigned int IsEqualIntNone(Mat44V_In inMat1, Mat44V_In inMat2);
	unsigned int IsEqualIntNone(QuatV_In inQuat1, QuatV_In inQuat2);

	//@@rage::IsEqualInt
	// PURPOSE: Tests whether the components in two vectors containing integers (not floats) are equal
	// PARAMS:
	//		in1 - The first comparand
	//		in2 - The second comparand
	// RETURNS: A vector containing (in1.n == in2.n) for each component of in1 and in2
	BoolV_Out    IsEqualInt(BoolV_In inVector1, BoolV_In inVector2);
	VecBoolV_Out IsEqualInt(VecBoolV_In inVector1, VecBoolV_In inVector2);
	BoolV_Out    IsEqualInt(ScalarV_In inVector1, ScalarV_In inVector2);
	VecBoolV_Out IsEqualInt(Vec2V_In inVector1, Vec2V_In inVector2);
	VecBoolV_Out IsEqualInt(Vec3V_In inVector1, Vec3V_In inVector2);
	VecBoolV_Out IsEqualInt(Vec4V_In inVector1, Vec4V_In inVector2);
	VecBoolV_Out IsEqualInt(QuatV_In inQuat1, QuatV_In inQuat2);

	//============================================================================
	// Conversion functions

	//@@rage::FloatToIntRaw
	// PURPOSE: Converts the floating point values in a vector into integer values (for fixed-point operations)
	// PARAMS:
	//		inVec - The vector of floating point values
	//		exponent - the fixed-point exponent
	// RETURNS:
	//		A fixed-point version of the values, using the specified exponent
	// NOTES:
	//		The final result is int(inVec.n * (2 ^ exponent)) for each channel n in the input vector
	// EXAMPLE:
	//		FloatToIntRaw<8>(ScalarV(0.25f)).Geti() -> 64    (0.25 * 2^8)
	template <int exponent>	FASTRETURNCHECK(ScalarV_Out) FloatToIntRaw(ScalarV_In inVec);
	template <int exponent>	FASTRETURNCHECK(Vec2V_Out)   FloatToIntRaw(Vec2V_In inVec);
	template <int exponent>	FASTRETURNCHECK(Vec3V_Out)   FloatToIntRaw(Vec3V_In inVec);
	template <int exponent>	FASTRETURNCHECK(Vec4V_Out)   FloatToIntRaw(Vec4V_In inVec);

	//@@rage::IntToFloatRaw
	// PURPOSE: Converts the integer values in a vector into floating point values
	// PARAMS:
	//		inVec - The vector of (fixed-point) integer values
	//		exponent - the fixed-point exponent
	// RETURNS:
	//		A floating point version of the input values, using the specified exponent
	// NOTES:
	//		The final result is float(inVec.n) / (2.0 ^ exponent) for each channel n in the input vector
	// EXAMPLE:
	//		FloatToIntRaw<8>(ScalarVConstant<64>()).Getf() -> 0.25f   (64 / (2^8))
	template <int exponent> FASTRETURNCHECK(ScalarV_Out) IntToFloatRaw(ScalarV_In inVec);
	template <int exponent>	FASTRETURNCHECK(Vec2V_Out)   IntToFloatRaw(Vec2V_In inVec);
	template <int exponent>	FASTRETURNCHECK(Vec3V_Out)   IntToFloatRaw(Vec3V_In inVec);
	template <int exponent>	FASTRETURNCHECK(Vec4V_Out)   IntToFloatRaw(Vec4V_In inVec);

	//@@rage::RoundToNearestInt
	// PURPOSE: Rounds each component of the input vector to the nearest integer value
	// NOTES: Similar to floating point round(). The values remain floats.
	FASTRETURNCHECK(ScalarV_Out) RoundToNearestInt(ScalarV_In inVec);
	FASTRETURNCHECK(Vec2V_Out)   RoundToNearestInt(Vec2V_In inVec);
	FASTRETURNCHECK(Vec3V_Out)   RoundToNearestInt(Vec3V_In inVec);
	FASTRETURNCHECK(Vec4V_Out)   RoundToNearestInt(Vec4V_In inVec);

	//@@rage::RoundToNearestIntZero
	// PURPOSE: Rounds each component of the input vector toward zero
	// NOTES: Similar to floating point trunc(). The values remain floats.
	FASTRETURNCHECK(ScalarV_Out) RoundToNearestIntZero(ScalarV_In inVec);
	FASTRETURNCHECK(Vec2V_Out)   RoundToNearestIntZero(Vec2V_In inVec);
	FASTRETURNCHECK(Vec3V_Out)   RoundToNearestIntZero(Vec3V_In inVec);
	FASTRETURNCHECK(Vec4V_Out)   RoundToNearestIntZero(Vec4V_In inVec);

	//@@rage::RoundToNearestIntZero
	// PURPOSE: Rounds each component of the input vector down toward negative infinity
	// NOTES: Similar to floating point floor(). The values remain floats.
	FASTRETURNCHECK(ScalarV_Out) RoundToNearestIntNegInf(ScalarV_In inVec);
	FASTRETURNCHECK(Vec2V_Out)   RoundToNearestIntNegInf(Vec2V_In inVec);
	FASTRETURNCHECK(Vec3V_Out)   RoundToNearestIntNegInf(Vec3V_In inVec);
	FASTRETURNCHECK(Vec4V_Out)   RoundToNearestIntNegInf(Vec4V_In inVec);

	//@@rage::RoundToNearestIntZero
	// PURPOSE: Rounds each component of the input vector up toward positive infinity
	// NOTES: Similar to floating point ceil(). The values remain floats.
	FASTRETURNCHECK(ScalarV_Out) RoundToNearestIntPosInf(ScalarV_In inVec);
	FASTRETURNCHECK(Vec2V_Out)   RoundToNearestIntPosInf(Vec2V_In inVec);
	FASTRETURNCHECK(Vec3V_Out)   RoundToNearestIntPosInf(Vec3V_In inVec);
	FASTRETURNCHECK(Vec4V_Out)   RoundToNearestIntPosInf(Vec4V_In inVec);


	//============================================================================
	// Standard algebra

	//@@rage::Clamp
	// PURPOSE: Clamps each component of inVect to the specified range
	// PARAMS:
	//		inVect - The vector to clamp
	//		lowBound - Minimum values to clamp to
	//		highBound - Maximum values to clamp to
	// RETURNS: A vector where each component is clamped to the range [lowBound.n, highBound.n]
	FASTRETURNCHECK(ScalarV_Out) Clamp( ScalarV_In inVect, ScalarV_In lowBound, ScalarV_In highBound );
	FASTRETURNCHECK(Vec2V_Out)   Clamp( Vec2V_In inVect, Vec2V_In lowBound, Vec2V_In highBound );
	FASTRETURNCHECK(Vec3V_Out)   Clamp( Vec3V_In inVect, Vec3V_In lowBound, Vec3V_In highBound );
	FASTRETURNCHECK(Vec4V_Out)   Clamp( Vec4V_In inVect, Vec4V_In lowBound, Vec4V_In highBound );

	//@@rage::Saturate
	// PURPOSE: Clamps each component of the input vector to the [0,1] range
	FASTRETURNCHECK(Vec2V_Out) Saturate( Vec2V_In inVect );
	FASTRETURNCHECK(Vec3V_Out) Saturate( Vec3V_In inVect );
	FASTRETURNCHECK(Vec4V_Out) Saturate( Vec4V_In inVect );

	// PURPOSE: Scales a vector so that its magnitude is within a given range
	// PARAMS:
	//		inVect - The input vector
	//		minMag - The minimum magnitude for the vector
	//		maxMag - The maximum magnitude for the vector
	// RETURNS:
	//		A new vector outVect such that |outVect| is in the [minMag, maxMag] range, with the same direction as inVect
	FASTRETURNCHECK(Vec3V_Out) ClampMag( Vec3V_In inVect, ScalarV_In minMag, ScalarV_In maxMag );

	//@@rage::Negate
	// PURPOSE: Returns a vector with each component negated
	FASTRETURNCHECK(ScalarV_Out) Negate(ScalarV_In inVect);
	FASTRETURNCHECK(Vec2V_Out)   Negate(Vec2V_In inVect);
	FASTRETURNCHECK(Vec3V_Out)   Negate(Vec3V_In inVect);
	FASTRETURNCHECK(Vec4V_Out)   Negate(Vec4V_In inVect);
	FASTRETURNCHECK(QuatV_Out)   Negate(QuatV_In inVect);

	//@@rage::Invert
	//<TOCTITLE Invert Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Returns the reciprocal 1/n for each component 'n' in the input vector
	// PARAMS:
	//		inVect - The input vector
	//		errValVect - For 'Safe' variants - the value to return if inVect is near zero
	// RETURNS: A new vector with 1/inVect.n for each component in inVect
	// NOTES:
	//		The Safe variant checks for division by zero, and returns the errValVect value for each component where that would be the case.
	//		The Fast variant uses a faster but less precise inversion function.
	FASTRETURNCHECK(ScalarV_Out) Invert(ScalarV_In inVect);
	FASTRETURNCHECK(Vec2V_Out)   Invert(Vec2V_In inVect);
	FASTRETURNCHECK(Vec3V_Out)   Invert(Vec3V_In inVect);
	FASTRETURNCHECK(Vec4V_Out)   Invert(Vec4V_In inVect);

	FASTRETURNCHECK(ScalarV_Out) InvertSafe(ScalarV_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));		// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec2V_Out)   InvertSafe(Vec2V_In inVect, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8));					// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec3V_Out)   InvertSafe(Vec3V_In inVect, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8));					// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec4V_Out)   InvertSafe(Vec4V_In inVect, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8));					// <COMBINE rage::Invert>

	FASTRETURNCHECK(ScalarV_Out) InvertFast(ScalarV_In inVect);																// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec2V_Out)   InvertFast(Vec2V_In inVect);																	// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec3V_Out)   InvertFast(Vec3V_In inVect);																	// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec4V_Out)   InvertFast(Vec4V_In inVect);																	// <COMBINE rage::Invert>

	FASTRETURNCHECK(ScalarV_Out) InvertFastSafe(ScalarV_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));	// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec2V_Out)   InvertFastSafe(Vec2V_In inVect, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8));				// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec3V_Out)   InvertFastSafe(Vec3V_In inVect, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8));				// <COMBINE rage::Invert>
	FASTRETURNCHECK(Vec4V_Out)   InvertFastSafe(Vec4V_In inVect, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8));				// <COMBINE rage::Invert>

	//============================================================================
	// Magnitude and Distance

	//@@rage::Mag
	//<TOCTITLE Mag Functions (Normal\, Fast)>
	// PURPOSE: Returns the magnitude of the input vector
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Mag(Vec2V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) Mag(Vec3V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) Mag(Vec4V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) Mag(QuatV_In inQuat);

	FASTRETURNCHECK(ScalarV_Out) MagFast(Vec2V_In inVect);											// <COMBINE rage::Mag>
	FASTRETURNCHECK(ScalarV_Out) MagFast(Vec3V_In inVect);											// <COMBINE rage::Mag>
	FASTRETURNCHECK(ScalarV_Out) MagFast(Vec4V_In inVect);											// <COMBINE rage::Mag>
	FASTRETURNCHECK(ScalarV_Out) MagFast(QuatV_In inQuat);

	//@@rage::MagSquared
	// PURPOSE: Returns the squared magnitude of the input vector
	FASTRETURNCHECK(ScalarV_Out) MagSquared(Vec2V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) MagSquared(Vec3V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) MagSquared(Vec4V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) MagSquared(QuatV_In inQuat);


	//@@rage::Dist
	//<TOCTITLE Dist Functions (Normal\, Fast)>
	// PURPOSE: Returns the distance between the input vectors
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Dist(Vec2V_In inVect1, Vec2V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) Dist(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) Dist(Vec4V_In inVect1, Vec4V_In inVect2);

	FASTRETURNCHECK(ScalarV_Out) DistFast(Vec2V_In inVect1, Vec2V_In inVect2);					// <COMBINE rage::Dist>
	FASTRETURNCHECK(ScalarV_Out) DistFast(Vec3V_In inVect1, Vec3V_In inVect2);					// <COMBINE rage::Dist>
	FASTRETURNCHECK(ScalarV_Out) DistFast(Vec4V_In inVect1, Vec4V_In inVect2);					// <COMBINE rage::Dist>

	//@@rage::DistSquared
	// PURPOSE: Returns the squared distance between the input vectors
	FASTRETURNCHECK(ScalarV_Out) DistSquared(Vec4V_In inVect1, Vec4V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) DistSquared(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) DistSquared(Vec2V_In inVect1, Vec2V_In inVect2);

	//@@rage::InvMag
	//<TOCTITLE InvMag Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Returns the inverse magnitude (1/mag) of the input vector
	// PARAMS:
	//		inVect - The input vector
	//		errValVect - For 'Safe' versions - returns errValVect if |inVect| == 0.0
	// RETURNS: A scalar containing 1/|inVect|
	// NOTES:
	//		The Safe variant returns the errValVect when inVect is invalid
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) InvMag(Vec2V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) InvMag(Vec3V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) InvMag(Vec4V_In inVect);

	FASTRETURNCHECK(ScalarV_Out) InvMagSafe(Vec2V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));		// <COMBINE rage::InvMag>
	FASTRETURNCHECK(ScalarV_Out) InvMagSafe(Vec3V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));		// <COMBINE rage::InvMag>
	FASTRETURNCHECK(ScalarV_Out) InvMagSafe(Vec4V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));		// <COMBINE rage::InvMag>

	FASTRETURNCHECK(ScalarV_Out) InvMagFast(Vec2V_In inVect);																// <COMBINE rage::InvMag>
	FASTRETURNCHECK(ScalarV_Out) InvMagFast(Vec3V_In inVect);																// <COMBINE rage::InvMag>
	FASTRETURNCHECK(ScalarV_Out) InvMagFast(Vec4V_In inVect);																// <COMBINE rage::InvMag>

	FASTRETURNCHECK(ScalarV_Out) InvMagFastSafe(Vec2V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));	// <COMBINE rage::InvMag>
	FASTRETURNCHECK(ScalarV_Out) InvMagFastSafe(Vec3V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));	// <COMBINE rage::InvMag>
	FASTRETURNCHECK(ScalarV_Out) InvMagFastSafe(Vec4V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));	// <COMBINE rage::InvMag>

	//@@rage::InvMagSquared
	//<TOCTITLE InvMagSquared Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Returns the squared inverse magnitude (1/(mag^2)) of the input vector
	// PARAMS:
	//		inVect - The input vector
	//		errValVect - For 'Safe' versions, returns errValVect if |inVect| == 0.0
	// RETURNS: A scalar containing 1/|inVect|^2
	// NOTES:
	//		The Safe variant returns the errValVect when inVect is invalid
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) InvMagSquared(Vec2V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) InvMagSquared(Vec3V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) InvMagSquared(Vec4V_In inVect);

	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredSafe(Vec2V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));			// <COMBINE rage::InvMagSquared>	
	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredSafe(Vec3V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));			// <COMBINE rage::InvMagSquared>
	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredSafe(Vec4V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));			// <COMBINE rage::InvMagSquared>

	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFast(Vec2V_In inVect);																// <COMBINE rage::InvMagSquared>
	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFast(Vec3V_In inVect);																// <COMBINE rage::InvMagSquared>
	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFast(Vec4V_In inVect);																// <COMBINE rage::InvMagSquared>

	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFastSafe(Vec2V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));		// <COMBINE rage::InvMagSquared>
	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFastSafe(Vec3V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));		// <COMBINE rage::InvMagSquared>
	FASTRETURNCHECK(ScalarV_Out) InvMagSquaredFastSafe(Vec4V_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));		// <COMBINE rage::InvMagSquared>

	//@@rage::Normalize
	//<TOCTITLE Normalize Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Returns a normalized version of the input vector
	// PARAMS:
	//		inVect - the input vector
	//		errValVect - for 'Safe' variants, the value to return if |inVect| is near 0
	//		magSqThreshold - for 'Safe' variants, the magnitude below which errValVect gets returned
	// RETURNS: inVect / |inVect| - a unit length vector in the same direction as inVect
	// NOTES:
	//		The Safe variant returns the errValVect value when |inVect|^2 < magSqThreshold
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(Vec2V_Out) Normalize(Vec2V_In inVect);
	FASTRETURNCHECK(Vec3V_Out) Normalize(Vec3V_In inVect);
	FASTRETURNCHECK(Vec4V_Out) Normalize(Vec4V_In inVect);

	FASTRETURNCHECK(Vec2V_Out) NormalizeSafe(Vec2V_In inVect, Vec2V_In errValVect, Vec2V_In magSqThreshold = Vec2V(V_FLT_SMALL_5));		// <COMBINE rage::Normalize>
	FASTRETURNCHECK(Vec3V_Out) NormalizeSafe(Vec3V_In inVect, Vec3V_In errValVect, Vec3V_In magSqThreshold = Vec3V(V_FLT_SMALL_5));		// <COMBINE rage::Normalize>
	FASTRETURNCHECK(Vec4V_Out) NormalizeSafe(Vec4V_In inVect, Vec4V_In errValVect, Vec4V_In magSqThreshold = Vec4V(V_FLT_SMALL_5));		// <COMBINE rage::Normalize>

	FASTRETURNCHECK(Vec2V_Out) NormalizeFast(Vec2V_In inVect);																											// <COMBINE rage::Normalize>
	FASTRETURNCHECK(Vec3V_Out) NormalizeFast(Vec3V_In inVect);																											// <COMBINE rage::Normalize>
	FASTRETURNCHECK(Vec4V_Out) NormalizeFast(Vec4V_In inVect);																											// <COMBINE rage::Normalize>

	FASTRETURNCHECK(Vec2V_Out) NormalizeFastSafe(Vec2V_In inVect, Vec2V_In errValVect, Vec2V_In magSqThreshold = Vec2V(V_FLT_SMALL_5));	// <COMBINE rage::Normalize>
	FASTRETURNCHECK(Vec3V_Out) NormalizeFastSafe(Vec3V_In inVect, Vec3V_In errValVect, Vec3V_In magSqThreshold = Vec3V(V_FLT_SMALL_5));	// <COMBINE rage::Normalize>
	FASTRETURNCHECK(Vec4V_Out) NormalizeFastSafe(Vec4V_In inVect, Vec4V_In errValVect, Vec4V_In magSqThreshold = Vec4V(V_FLT_SMALL_5));	// <COMBINE rage::Normalize>

	//@@rage::InvDist
	//<TOCTITLE InvDist Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Returns the inverse of the distance between two vectors (1 / |a-b|)
	// PARAMS:
	//		inVect1 - The first vector
	//		inVect2 - The second vector
	//		errValVect - for 'Safe' variants, the value to return if the dist is 0.0
	// RETURNS: 1 / |inVect1 - inVect2|
	// NOTES:
	//		The Safe variant returns the errValVect value when the dist is 0.0
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) InvDist(Vec2V_In inVect1, Vec2V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) InvDist(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) InvDist(Vec4V_In inVect1, Vec4V_In inVect2);

	FASTRETURNCHECK(ScalarV_Out) InvDistSafe(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8));				// <COMBINE rage::InvDist>
	FASTRETURNCHECK(ScalarV_Out) InvDistSafe(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8));				// <COMBINE rage::InvDist>
	FASTRETURNCHECK(ScalarV_Out) InvDistSafe(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8));				// <COMBINE rage::InvDist>

	FASTRETURNCHECK(ScalarV_Out) InvDistFast(Vec2V_In inVect1, Vec2V_In inVect2);																// <COMBINE rage::InvDist>
	FASTRETURNCHECK(ScalarV_Out) InvDistFast(Vec3V_In inVect1, Vec3V_In inVect2);																// <COMBINE rage::InvDist>
	FASTRETURNCHECK(ScalarV_Out) InvDistFast(Vec4V_In inVect1, Vec4V_In inVect2);																// <COMBINE rage::InvDist>

	FASTRETURNCHECK(ScalarV_Out) InvDistFastSafe(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8));			// <COMBINE rage::InvDist>
	FASTRETURNCHECK(ScalarV_Out) InvDistFastSafe(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8));			// <COMBINE rage::InvDist>
	FASTRETURNCHECK(ScalarV_Out) InvDistFastSafe(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8));			// <COMBINE rage::InvDist>

	//@@rage::InvDist
	//<TOCTITLE InvDistSquared Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Returns the inverse of the squared distance between two vectors (1 / |a-b|^2)
	// PARAMS:
	//		inVect1 - The first vector
	//		inVect2 - The second vector
	//		errValVect - for 'Safe' variants, the value to return if the dist is 0.0
	// RETURNS: 1 / |inVect1 - inVect2|^2
	// NOTES:
	//		The Safe variant returns the errValVect value when the dist is 0.0
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) InvDistSquared(Vec2V_In inVect1, Vec2V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) InvDistSquared(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) InvDistSquared(Vec4V_In inVect1, Vec4V_In inVect2);

	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredSafe(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8));		// <COMBINE rage::InvDistSquared>
	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredSafe(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8));		// <COMBINE rage::InvDistSquared>
	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredSafe(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8));		// <COMBINE rage::InvDistSquared>

	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFast(Vec2V_In inVect1, Vec2V_In inVect2);														// <COMBINE rage::InvDistSquared>
	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFast(Vec3V_In inVect1, Vec3V_In inVect2);														// <COMBINE rage::InvDistSquared>
	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFast(Vec4V_In inVect1, Vec4V_In inVect2);														// <COMBINE rage::InvDistSquared>

	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFastSafe(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8));	// <COMBINE rage::InvDistSquared>
	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFastSafe(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8));	// <COMBINE rage::InvDistSquared>
	FASTRETURNCHECK(ScalarV_Out) InvDistSquaredFastSafe(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8));	// <COMBINE rage::InvDistSquared>


	//<TOCTITLE MagXY Functions (Normal\, Fast)>
	// PURPOSE: Computes the magnitude of a 3d vector in only 2 dimensions
	// PARAMS:
	//		inVect - The input vector
	// RETURNS: A scalar containing the magnitude of the vector once projected onto the 2d plane
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) MagXY( Vec3V_In inVect );
	FASTRETURNCHECK(ScalarV_Out) MagXYSquared( Vec3V_In inVect );
	FASTRETURNCHECK(ScalarV_Out) MagXYFast( Vec3V_In inVect );										// <COMBINE rage::MagXY@Vec3V_In>

	FASTRETURNCHECK(ScalarV_Out) MagXZ( Vec3V_In inVect );											// <TOCTITLE MagXZ Functions (Normal\, Fast)> <COPY rage::MagXY@Vec3V_In>
	FASTRETURNCHECK(ScalarV_Out) MagXZSquared( Vec3V_In inVect );
	FASTRETURNCHECK(ScalarV_Out) MagXZFast( Vec3V_In inVect );										// <COMBINE rage::MagXZ@Vec3V_In>

	FASTRETURNCHECK(ScalarV_Out) MagYZ( Vec3V_In inVect );											// <TOCTITLE MagYZ Functions (Normal\, Fast)> <COPY rage::MagXY@Vec3V_In>
	FASTRETURNCHECK(ScalarV_Out) MagYZSquared( Vec3V_In inVect );
	FASTRETURNCHECK(ScalarV_Out) MagYZFast( Vec3V_In inVect );										// <COMBINE rage::MagYZ@Vec3V_In>

	//<TOCTITLE DistXY Functions (Normal\, Fast)>
	// PURPOSE: Computes the distance between two 3d vectors in only 2 dimensions
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) DistXY( Vec3V_In inVec1, Vec3V_In inVec2 );
	FASTRETURNCHECK(ScalarV_Out) DistXYFast( Vec3V_In inVec1, Vec3V_In inVec2 );					// <COMBINE rage::DistXY@Vec3V_In@Vec3V_In>

	FASTRETURNCHECK(ScalarV_Out) DistXZ( Vec3V_In inVec1, Vec3V_In inVec2 );						// <TOCTITLE DistXZ Functions (Normal\, Fast)> <COPY rage::DistXY@Vec3V_In@Vec3V_In>
	FASTRETURNCHECK(ScalarV_Out) DistXZFast( Vec3V_In inVec1, Vec3V_In inVec2 );					// <COMBINE rage::DistXZ@Vec3V_In@Vec3V_In>

	FASTRETURNCHECK(ScalarV_Out) DistYZ( Vec3V_In inVec1, Vec3V_In inVec2 );						// <TOCTITLE DistYZ Functions (Normal\, Fast)> <COPY rage::DistXY@Vec3V_In@Vec3V_In>
	FASTRETURNCHECK(ScalarV_Out) DistYZFast( Vec3V_In inVec1, Vec3V_In inVec2 );					// <COMBINE rage::DistYZ@Vec3V_In@Vec3V_In>


	//============================================================================
	// General vector/matrix math.

	//@@rage::Add
	// PURPOSE: Adds two vectors and returns the sum
	FASTRETURNCHECK(ScalarV_Out) Add( ScalarV_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out)   Add( Vec2V_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out)   Add( Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out)   Add( Vec4V_In inVect1, Vec4V_In inVect2 );

	//@@rage::Subtract
	// PURPOSE: Subtracts one vector from another and returns the difference
	// PARAMS:
	//		inVect1 - The first vector
	//		inVect2 - The vector to subtract
	// RETURNS: inVect1 - inVect2
	FASTRETURNCHECK(ScalarV_Out) Subtract( ScalarV_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out)   Subtract( Vec2V_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out)   Subtract( Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out)   Subtract( Vec4V_In inVect1, Vec4V_In inVect2 );

	//@@rage::Average
	// PURPOSE: Returns the average (mean) of two vectors
	// PARAMS:
	//		inVect1 - The first vector
	//		inVect2 - The second vector
	// RETURNS: The per-component mean of the two vectors, the midpoint between them.
	FASTRETURNCHECK(ScalarV_Out) Average( ScalarV_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out)   Average( Vec2V_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out)   Average( Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out)   Average( Vec4V_In inVect1, Vec4V_In inVect2 );

	//@@rage::Scale
	// PURPOSE: Scales each component of inVect1 by the corresponding component of inVect2
	FASTRETURNCHECK(ScalarV_Out) Scale( ScalarV_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out)   Scale( Vec2V_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out)   Scale( ScalarV_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out)   Scale( Vec2V_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out)   Scale( Vec3V_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out)   Scale( ScalarV_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out)   Scale( Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out)   Scale( Vec4V_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out)   Scale( ScalarV_In inVect1, Vec4V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out)   Scale( Vec4V_In inVect1, Vec4V_In inVect2 );
	FASTRETURNCHECK(QuatV_Out)   Scale( QuatV_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(QuatV_Out)   Scale( QuatV_In inVect1, Vec4V_In inVect2 );

	//@@rage::Abs
	// PURPOSE: Returns a vector containing the absolute value of each of the input components
	FASTRETURNCHECK(ScalarV_Out) Abs(ScalarV_In inVect);
	FASTRETURNCHECK(Vec2V_Out)   Abs(Vec2V_In inVect);
	FASTRETURNCHECK(Vec3V_Out)   Abs(Vec3V_In inVect);
	FASTRETURNCHECK(Vec4V_Out)   Abs(Vec4V_In inVect);

	//@@rage::Sqrt
	//<TOCTITLE Sqrt Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Computes the square root of each component of a vector
	// PARAMS:
	//		inVect - The input vector
	//		errValVect - For 'Safe' variants - the value to return if inVect is invalid
	// RETURNS: A new vector with sqrt(inVect.n) for each component in inVect
	// NOTES:
	//		The Safe variant returns the errValVect value for each component where sqrt(inVect.n) would be invalid
	//		The Fast variant uses a faster but less precise square root function.
	FASTRETURNCHECK(ScalarV_Out) Sqrt(ScalarV_In inVect);
	FASTRETURNCHECK(Vec2V_Out)   Sqrt(Vec2V_In inVect);
	FASTRETURNCHECK(Vec3V_Out)   Sqrt(Vec3V_In inVect);
	FASTRETURNCHECK(Vec4V_Out)   Sqrt(Vec4V_In inVect);

	FASTRETURNCHECK(ScalarV_Out) SqrtSafe(ScalarV_In inVect, ScalarV_In errValVect = ScalarV(V_ZERO));				// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec2V_Out)   SqrtSafe(Vec2V_In inVect, Vec2V_In errValVect = Vec2V(V_ZERO));						// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec3V_Out)   SqrtSafe(Vec3V_In inVect, Vec3V_In errValVect = Vec3V(V_ZERO));						// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec4V_Out)   SqrtSafe(Vec4V_In inVect, Vec4V_In errValVect = Vec4V(V_ZERO));						// <COMBINE rage::Sqrt>

	FASTRETURNCHECK(ScalarV_Out) SqrtFast(ScalarV_In inVect);																// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec2V_Out)   SqrtFast(Vec2V_In inVect);																	// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec3V_Out)   SqrtFast(Vec3V_In inVect);																	// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec4V_Out)   SqrtFast(Vec4V_In inVect);																	// <COMBINE rage::Sqrt>

	FASTRETURNCHECK(ScalarV_Out) SqrtFastSafe(ScalarV_In inVect, ScalarV_In errValVect = ScalarV(V_ZERO));			// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec2V_Out)   SqrtFastSafe(Vec2V_In inVect, Vec2V_In errValVect = Vec2V(V_ZERO));					// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec3V_Out)   SqrtFastSafe(Vec3V_In inVect, Vec3V_In errValVect = Vec3V(V_ZERO));					// <COMBINE rage::Sqrt>
	FASTRETURNCHECK(Vec4V_Out)   SqrtFastSafe(Vec4V_In inVect, Vec4V_In errValVect = Vec4V(V_ZERO));					// <COMBINE rage::Sqrt>

	//@@rage::InvSqrt
	//<TOCTITLE InvSqrt Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Returns the reciprocal square root for each component 'n' in the input vector
	// PARAMS:
	//		inVect - The input vector
	//		errValVect - For 'Safe' variants, the value to return when inVect is invalid
	// RETURNS: A new vector with 1/sqrt(inVect.n) for each component in inVect
	// NOTES:
	//		The Safe variant returns the errValVect value for each component where 1/sqrt(inVect.n) would be invalid
	//		The Fast variant uses a faster but less precise reciprocal square root function.
	FASTRETURNCHECK(ScalarV_Out) InvSqrt(ScalarV_In inVect);
	FASTRETURNCHECK(Vec2V_Out)   InvSqrt(Vec2V_In inVect);
	FASTRETURNCHECK(Vec3V_Out)   InvSqrt(Vec3V_In inVect);
	FASTRETURNCHECK(Vec4V_Out)   InvSqrt(Vec4V_In inVect);

	FASTRETURNCHECK(ScalarV_Out) InvSqrtSafe(ScalarV_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));		// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec2V_Out)   InvSqrtSafe(Vec2V_In inVect, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8));				// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec3V_Out)   InvSqrtSafe(Vec3V_In inVect, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8));				// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec4V_Out)   InvSqrtSafe(Vec4V_In inVect, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8));				// <COMBINE rage::InvSqrt>

	FASTRETURNCHECK(ScalarV_Out) InvSqrtFast(ScalarV_In inVect);																// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec2V_Out)   InvSqrtFast(Vec2V_In inVect);																	// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec3V_Out)   InvSqrtFast(Vec3V_In inVect);																	// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec4V_Out)   InvSqrtFast(Vec4V_In inVect);																	// <COMBINE rage::InvSqrt>

	FASTRETURNCHECK(ScalarV_Out) InvSqrtFastSafe(ScalarV_In inVect, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8));	// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec2V_Out)   InvSqrtFastSafe(Vec2V_In inVect, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8));			// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec3V_Out)   InvSqrtFastSafe(Vec3V_In inVect, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8));			// <COMBINE rage::InvSqrt>
	FASTRETURNCHECK(Vec4V_Out)   InvSqrtFastSafe(Vec4V_In inVect, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8));			// <COMBINE rage::InvSqrt>



	// PURPOSE: Computes the cross product of inVect1 and inVect2
	// NOTES:
	//		This function sets w = 0, since it happens to be a free vectorized operation.
	FASTRETURNCHECK(Vec3V_Out) Cross( Vec3V_In inVect1, Vec3V_In inVect2 );

	// PURPOSE: Computes the 3d cross product of inVect1.xyz and inVect2.xyz
	// NOTES:
	//		This function sets w = 0, since it happens to be a free vectorized operation.
	FASTRETURNCHECK(Vec4V_Out) Cross3( Vec4V_In inVect1, Vec4V_In inVect2 );

	// PURPOSE: Computes a two dimensional cross product
	// PARAMS:
	//		inVect1 - The first vector
	//		inVect2 - The second vector
	// RETURNS: inVect1.x * inVect2.y - inVect1.y * inVect2.x
	FASTRETURNCHECK(ScalarV_Out) Cross( Vec2V_In inVect1, Vec2V_In inVect2 );

	// PURPOSE: Adds the cross product of two vectors to another: a + b x c
	// PARAMS:
	//		toAddTo - The vector to add to
	//		toCross1 - One of the vectors to take the cross product of
	//		toCross2 - The other cross product vector
	// RETURNS toAddTo + Cross(toCross1, toCross2)
	// NOTES: This function sets w = toAddTo.w, since it happens to be a free vectorized operation. (If this is desirable, go ahead and take advantage.)
	FASTRETURNCHECK(Vec3V_Out) AddCrossed( Vec3V_In toAddTo, Vec3V_In toCross1, Vec3V_In toCross2 );
	FASTRETURNCHECK(Vec4V_Out) AddCrossed3( Vec4V_In toAddTo, Vec4V_In toCross1, Vec4V_In toCross2 );	// <COPY rage::AddCrossed@Vec3V_In@Vec3V_In@Vec3V_In>

	// PURPOSE: Subtracts the cross product of two vectors from another: a - b x c
	// PARAMS:
	//		toSubtractFrom - The vector to subtract from
	//		toCross1 - One of the vectors to take the cross product of
	//		toCross2 - The other cross product vector
	// RETURNS toSubtractFrom - Cross(toCross1, toCross2)
	// NOTES: This function sets w = toSubtractFrom.w, since it happens to be a free vectorized operation. (If this is desirable, go ahead and take advantage.)
	FASTRETURNCHECK(Vec3V_Out) SubtractCrossed( Vec3V_In toSubtractFrom, Vec3V_In toCross1, Vec3V_In toCross2 );
	FASTRETURNCHECK(Vec4V_Out) SubtractCrossed3( Vec4V_In toSubtractFrom, Vec4V_In toCross1, Vec4V_In toCross2 );		// <COPY rage::SubtractCrossed@Vec3V_In@Vec3V_In@Vec3V_In>

	//@@rage::Dot
	// PURPOSE: Computes the dot product of two vectors: 
	// PARAMS:
	//		inVect1 - The first vector
	//		inVect2 - The second vector
	// RETURNS: The dot product, the sum of (inVect1.n * inVect2.n) for all components 'n'.
	FASTRETURNCHECK(ScalarV_Out) Dot( Vec4V_In inVect1, Vec4V_In inVect2 );
	FASTRETURNCHECK(ScalarV_Out) Dot( Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(ScalarV_Out) Dot( Vec2V_In inVect1, Vec2V_In inVect2 );

	//@@rage::AddScaled
	// PURPOSE: Scales two vectors, adds a third: a + b * c
	// PARAMS:
	//		toAdd - The vector to add
	//		toScaleThenAdd - One of the two vectors to scale
	//		scaleValue - The other vector to scale
	// RETURNS: toAdd + (toScaleThenAdd * scaleValue)
	FASTRETURNCHECK(ScalarV_Out) AddScaled( ScalarV_In toAdd, ScalarV_In toScaleThenAdd, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec2V_Out)   AddScaled( Vec2V_In toAdd, Vec2V_In toScaleThenAdd, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec2V_Out)   AddScaled( Vec2V_In toAdd, Vec2V_In toScaleThenAdd, Vec2V_In scaleValues );
	FASTRETURNCHECK(Vec3V_Out)   AddScaled( Vec3V_In toAdd, Vec3V_In toScaleThenAdd, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec3V_Out)   AddScaled( Vec3V_In toAdd, Vec3V_In toScaleThenAdd, Vec3V_In scaleValues );
	FASTRETURNCHECK(Vec4V_Out)   AddScaled( Vec4V_In toAdd, Vec4V_In toScaleThenAdd, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec4V_Out)   AddScaled( Vec4V_In toAdd, Vec4V_In toScaleThenAdd, Vec4V_In scaleValues );

	//@@rage::SubtractScaled
	// PURPOSE: Scales two vectors, subtracts the result from a third: a - b * c
	// PARAMS:
	//		toSubtractFrom - The vector to subtract from
	//		toScaleThenSubtract - One of the two vectors to scale
	//		scaleValue - The other vector to scale
	// RETURNS: toSubtractFrom - (toScaleThenSubtract * scaleValue)
	FASTRETURNCHECK(ScalarV_Out) SubtractScaled( ScalarV_In toSubtractFrom, ScalarV_In toScaleThenSubtract, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec2V_Out)   SubtractScaled( Vec2V_In toSubtractFrom, Vec2V_In toScaleThenSubtract, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec2V_Out)   SubtractScaled( Vec2V_In toSubtractFrom, Vec2V_In toScaleThenSubtract, Vec2V_In scaleValues );
	FASTRETURNCHECK(Vec3V_Out)   SubtractScaled( Vec3V_In toSubtractFrom, Vec3V_In toScaleThenSubtract, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec3V_Out)   SubtractScaled( Vec3V_In toSubtractFrom, Vec3V_In toScaleThenSubtract, Vec3V_In scaleValues );
	FASTRETURNCHECK(Vec4V_Out)   SubtractScaled( Vec4V_In toSubtractFrom, Vec4V_In toScaleThenSubtract, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec4V_Out)   SubtractScaled( Vec4V_In toSubtractFrom, Vec4V_In toScaleThenSubtract, Vec4V_In scaleValues );

	//@@rage::InvScale
	//<TOCTITLE InvScale Functions  (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Scales each component of one vector by the inverse of another: a / b
	// PARAMS:
	//		toScale - the vector to scale
	//		scaleValue - The vector to invert, then scale by
	//		errValVect - Values to return whenever scaleValue.n would be invalid
	// NOTES:
	//		The Safe variant returns errValVect.n whenever scaleValue.n == 0.0
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) InvScale( ScalarV_In toScale, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec2V_Out)   InvScale( Vec2V_In toScale, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec2V_Out)   InvScale( Vec2V_In toScale, Vec2V_In scaleValues );
	FASTRETURNCHECK(Vec3V_Out)   InvScale( Vec3V_In toScale, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec3V_Out)   InvScale( Vec3V_In toScale, Vec3V_In scaleValues );
	FASTRETURNCHECK(Vec4V_Out)   InvScale( Vec4V_In toScale, ScalarV_In scaleValue );
	FASTRETURNCHECK(Vec4V_Out)   InvScale( Vec4V_In toScale, Vec4V_In scaleValues );

	FASTRETURNCHECK(ScalarV_Out) InvScaleSafe( ScalarV_In toScale, ScalarV_In scaleValue, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8) );				// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec2V_Out)   InvScaleSafe( Vec2V_In toScale, ScalarV_In scaleValue, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8) );						// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec2V_Out)   InvScaleSafe( Vec2V_In toScale, Vec2V_In scaleValues, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8) );						// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec3V_Out)   InvScaleSafe( Vec3V_In toScale, ScalarV_In scaleValue, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8) );						// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec3V_Out)   InvScaleSafe( Vec3V_In toScale, Vec3V_In scaleValues, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8) );						// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec4V_Out)   InvScaleSafe( Vec4V_In toScale, ScalarV_In scaleValue, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8) );						// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec4V_Out)   InvScaleSafe( Vec4V_In toScale, Vec4V_In scaleValues, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8) );						// <COMBINE rage::InvScale>

	FASTRETURNCHECK(ScalarV_Out) InvScaleFast( ScalarV_In toScale, ScalarV_In scaleValue );																		// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec2V_Out)   InvScaleFast( Vec2V_In toScale, ScalarV_In scaleValue );																		// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec2V_Out)   InvScaleFast( Vec2V_In toScale, Vec2V_In scaleValues );																			// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec3V_Out)   InvScaleFast( Vec3V_In toScale, ScalarV_In scaleValue );																		// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec3V_Out)   InvScaleFast( Vec3V_In toScale, Vec3V_In scaleValues );																			// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec4V_Out)   InvScaleFast( Vec4V_In toScale, ScalarV_In scaleValue );																		// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec4V_Out)   InvScaleFast( Vec4V_In toScale, Vec4V_In scaleValues );																			// <COMBINE rage::InvScale>

	FASTRETURNCHECK(ScalarV_Out) InvScaleFastSafe( ScalarV_In toScale, ScalarV_In scaleValue, ScalarV_In errValVect = ScalarV(V_FLT_LARGE_8) );			// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec2V_Out)   InvScaleFastSafe( Vec2V_In toScale, ScalarV_In scaleValue, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8) );					// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec2V_Out)   InvScaleFastSafe( Vec2V_In toScale, Vec2V_In scaleValues, Vec2V_In errValVect = Vec2V(V_FLT_LARGE_8) );					// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec3V_Out)   InvScaleFastSafe( Vec3V_In toScale, ScalarV_In scaleValue, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8) );					// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec3V_Out)   InvScaleFastSafe( Vec3V_In toScale, Vec3V_In scaleValues, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8) );					// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec4V_Out)   InvScaleFastSafe( Vec4V_In toScale, ScalarV_In scaleValue, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8) );					// <COMBINE rage::InvScale>
	FASTRETURNCHECK(Vec4V_Out)   InvScaleFastSafe( Vec4V_In toScale, Vec4V_In scaleValues, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8) );					// <COMBINE rage::InvScale>


	//@@rage::AddNet
	// PURPOSE:
	//   Add the toAdd vector to the inVector, but does not add in any part of the toAdd
	//   vector that points in the same direction as inVector.
	// PARAMS:
	//	 inVector - The source vector
	//   toAdd - The vector to sum with the source vector, less any positive parallel component.
	// NOTES:
	//   This is for summing together multiple pushes on the same object, so that parallel
	//   pushes do not add and anti-parallel pushes do add.
	FASTRETURNCHECK(Vec3V_Out) AddNet( Vec3V_In inVector, Vec3V_In toAdd );
	FASTRETURNCHECK(Vec2V_Out) AddNet( Vec2V_In inVector, Vec2V_In toAdd );

	//============================================================================
	// Angular

	// PURPOSE: Rotates a 2d vector
	// PARAMS:
	//		inVect - the vector to rotate
	//		radians - the amount to rotate by
	FASTRETURNCHECK(Vec2V_Out) Rotate( Vec2V_In inVect, ScalarV_In radians );

	// PURPOSE: Rotates a vector about a world axis
	// PARAMS:
	//		inVect - the vector to rotate
	//		radians - the amount to rotate by
	FASTRETURNCHECK(Vec3V_Out) RotateAboutXAxis( Vec3V_In inVect, ScalarV_In radians );
	FASTRETURNCHECK(Vec3V_Out) RotateAboutYAxis( Vec3V_In inVect, ScalarV_In radians );			// <COPY rage::RotateAboutXAxis@Vec3V_In@ScalarV_In>
	FASTRETURNCHECK(Vec3V_Out) RotateAboutZAxis( Vec3V_In inVect, ScalarV_In radians );			// <COPY rage::RotateAboutXAxis@Vec3V_In@ScalarV_In>

	//@@rage::CanonicalizeAngle
	// PURPOSE: Puts an angle between [-PI,PI] by adding/subtracting 2*PI.
	// PARAMS:
	//		radians - A vector containing angles
	// RETURNS: A new vector where each component is now in the range [-PI,PI]
	FASTRETURNCHECK(ScalarV_Out) CanonicalizeAngle( ScalarV_In radians );
	FASTRETURNCHECK(Vec2V_Out)   CanonicalizeAngle( Vec2V_In radians );
	FASTRETURNCHECK(Vec3V_Out)   CanonicalizeAngle( Vec3V_In radians );
	FASTRETURNCHECK(Vec4V_Out)   CanonicalizeAngle( Vec4V_In radians );

	//@@rage::SinAndCos
	//<TOCTITLE SinAndCos Functions (Normal\, Fast)>
	// PURPOSE: Computes the sine and cosine of each component in the input vector
	// PARAMS:
	//		outSin - A reference to a vector that will hold the sines
	//		outCos - A reference to a vector that will hold the cosines
	//		inVect - The input angles
	// NOTES:
	//		This is faster than doing seperate Sin() and Cos() calls.
	//		The Fast variant is faster but less precise
	void SinAndCos( ScalarV_InOut outSin, ScalarV_InOut outCos, ScalarV_In inVect );
	void SinAndCos( Vec2V_InOut outSin, Vec2V_InOut outCos, Vec2V_In inVect );
	void SinAndCos( Vec3V_InOut outSin, Vec3V_InOut outCos, Vec3V_In inVect );
	void SinAndCos( Vec4V_InOut outSin, Vec4V_InOut outCos, Vec4V_In inVect );
	void SinAndCosFast( ScalarV_InOut outSin, ScalarV_InOut outCos, ScalarV_In inVect );		// <COMBINE rage::SinAndCos>
	void SinAndCosFast( Vec2V_InOut outSin, Vec2V_InOut outCos, Vec2V_In inVect );				// <COMBINE rage::SinAndCos>
	void SinAndCosFast( Vec3V_InOut outSin, Vec3V_InOut outCos, Vec3V_In inVect );				// <COMBINE rage::SinAndCos>
	void SinAndCosFast( Vec4V_InOut outSin, Vec4V_InOut outCos, Vec4V_In inVect );				// <COMBINE rage::SinAndCos>

	//@@rage::Sin
	//<TOCTITLE Sin Functions (Normal\, Fast)>
	// PURPOSE: Computes the sine of each component of the input vector
	// PARAMS:
	//		inRadians - The input vector
	// RETURNS: A vector containing sin(inVect.n) for each component n in the input vector
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Sin( ScalarV_In inRadians );
	FASTRETURNCHECK(Vec2V_Out)   Sin( Vec2V_In inRadians );
	FASTRETURNCHECK(Vec3V_Out)   Sin( Vec3V_In inRadians );
	FASTRETURNCHECK(Vec4V_Out)   Sin( Vec4V_In inRadians );
	FASTRETURNCHECK(ScalarV_Out) SinFast( ScalarV_In inRadians );													// <COMBINE rage::Sin>
	FASTRETURNCHECK(Vec2V_Out)   SinFast( Vec2V_In inRadians );														// <COMBINE rage::Sin>
	FASTRETURNCHECK(Vec3V_Out)   SinFast( Vec3V_In inRadians );														// <COMBINE rage::Sin>
	FASTRETURNCHECK(Vec4V_Out)   SinFast( Vec4V_In inRadians );														// <COMBINE rage::Sin>

	//@@rage::Cos
	//<TOCTITLE Cos Functions (Normal\, Fast)>
	// PURPOSE: Computes the cosine of each component of the input vector
	// PARAMS:
	//		inRadians - The input vector
	// RETURNS: A vector containing cos(inVect.n) for each component n in the input vector
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Cos( ScalarV_In inRadians );
	FASTRETURNCHECK(Vec2V_Out)   Cos( Vec2V_In inRadians );
	FASTRETURNCHECK(Vec3V_Out)   Cos( Vec3V_In inRadians );
	FASTRETURNCHECK(Vec4V_Out)   Cos( Vec4V_In inRadians );
	FASTRETURNCHECK(ScalarV_Out) CosFast( ScalarV_In inRadians );														// <COMBINE rage::Cos>
	FASTRETURNCHECK(Vec2V_Out)   CosFast( Vec2V_In inRadians );															// <COMBINE rage::Cos>
	FASTRETURNCHECK(Vec3V_Out)   CosFast( Vec3V_In inRadians );															// <COMBINE rage::Cos>
	FASTRETURNCHECK(Vec4V_Out)   CosFast( Vec4V_In inRadians );															// <COMBINE rage::Cos>

	//@@rage::Tan
	//<TOCTITLE Tan Functions (Normal\, Fast)>
	// PURPOSE: Computes the tangent of each component of the input vector
	// PARAMS:
	//		inRadians - The input vector
	// RETURNS: A vector containing tan(inVect.n) for each component n in the input vector
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Tan( ScalarV_In inRadians );
	FASTRETURNCHECK(Vec2V_Out)   Tan( Vec2V_In inRadians );
	FASTRETURNCHECK(Vec3V_Out)   Tan( Vec3V_In inRadians );
	FASTRETURNCHECK(Vec4V_Out)   Tan( Vec4V_In inRadians );
	FASTRETURNCHECK(ScalarV_Out) TanFast( ScalarV_In inRadians );														// <COMBINE rage::Tan>
	FASTRETURNCHECK(Vec2V_Out)   TanFast( Vec2V_In inRadians );															// <COMBINE rage::Tan>
	FASTRETURNCHECK(Vec3V_Out)   TanFast( Vec3V_In inRadians );															// <COMBINE rage::Tan>
	FASTRETURNCHECK(Vec4V_Out)   TanFast( Vec4V_In inRadians );															// <COMBINE rage::Tan>

	//@@rage::Arcsin
	//<TOCTITLE Arcsin Functions (Normal\, Fast)>
	// PURPOSE: Computes the arcsine of each component of the input vector
	// PARAMS:
	//		inVect - The input vector
	// RETURNS: A vector containing asin(inVect.n) for each component n in the input vector
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Arcsin( ScalarV_In inVect );
	FASTRETURNCHECK(Vec2V_Out)   Arcsin( Vec2V_In inVect );
	FASTRETURNCHECK(Vec3V_Out)   Arcsin( Vec3V_In inVect );
	FASTRETURNCHECK(Vec4V_Out)   Arcsin( Vec4V_In inVect );
	FASTRETURNCHECK(ScalarV_Out) ArcsinFast( ScalarV_In inVect );														// <COMBINE rage::Arcsin>
	FASTRETURNCHECK(Vec2V_Out)   ArcsinFast( Vec2V_In inVect );	   														// <COMBINE rage::Arcsin>
	FASTRETURNCHECK(Vec3V_Out)   ArcsinFast( Vec3V_In inVect );	   														// <COMBINE rage::Arcsin>
	FASTRETURNCHECK(Vec4V_Out)   ArcsinFast( Vec4V_In inVect );	   														// <COMBINE rage::Arcsin>

	//@@rage::Arccos
	//<TOCTITLE Arccos Functions (Normal\, Fast)>
	// PURPOSE: Computes the arccosine of each component of the input vector
	// PARAMS:
	//		inVect - The input vector
	// RETURNS: A vector containing acos(inVect.n) for each component n in the input vector
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Arccos( ScalarV_In inVect );
	FASTRETURNCHECK(Vec2V_Out)   Arccos( Vec2V_In inVect );
	FASTRETURNCHECK(Vec3V_Out)   Arccos( Vec3V_In inVect );
	FASTRETURNCHECK(Vec4V_Out)   Arccos( Vec4V_In inVect );
	FASTRETURNCHECK(ScalarV_Out) ArccosFast( ScalarV_In inVect ); 														// <COMBINE rage::Arccos>
	FASTRETURNCHECK(Vec2V_Out)   ArccosFast( Vec2V_In inVect );															// <COMBINE rage::Arccos>
	FASTRETURNCHECK(Vec3V_Out)   ArccosFast( Vec3V_In inVect );															// <COMBINE rage::Arccos>
	FASTRETURNCHECK(Vec4V_Out)   ArccosFast( Vec4V_In inVect );															// <COMBINE rage::Arccos>

	//@@rage::Arctan
	//<TOCTITLE Arctan Functions (Normal\, Fast)>
	// PURPOSE: Computes the arctangent of each component of the input vector
	// PARAMS:
	//		inVect - The input vector
	// RETURNS: A vector containing atan(inVect.n) for each component n in the input vector
	// NOTES:
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Arctan( ScalarV_In inVect );
	FASTRETURNCHECK(Vec2V_Out)   Arctan( Vec2V_In inVect );
	FASTRETURNCHECK(Vec3V_Out)   Arctan( Vec3V_In inVect );
	FASTRETURNCHECK(Vec4V_Out)   Arctan( Vec4V_In inVect );
	FASTRETURNCHECK(ScalarV_Out) ArctanFast( ScalarV_In inVect ); 														// <COMBINE rage::Arctan>
	FASTRETURNCHECK(Vec2V_Out)   ArctanFast( Vec2V_In inVect );	   														// <COMBINE rage::Arctan>
	FASTRETURNCHECK(Vec3V_Out)   ArctanFast( Vec3V_In inVect );	   														// <COMBINE rage::Arctan>
	FASTRETURNCHECK(Vec4V_Out)   ArctanFast( Vec4V_In inVect );	   														// <COMBINE rage::Arctan>

	//@@rage::Arctan2
	//<TOCTITLE Arctan2 Functions (Normal\, Fast)>
	// PURPOSE: Computes the arctangent of y/x for each component of the input vectors
	// PARAMS:
	//		y - The dividend vector
	//		x - The divisor vector
	// RETURNS: A vector containing atan(inVectY.n / inVectX.n) for each component n in the input vectors
	// NOTES:
	//		This function has the same properties as atan2(), with regards to the treatment of signs on x and y and the range of output values.
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(ScalarV_Out) Arctan2( ScalarV_In y, ScalarV_In x );
	FASTRETURNCHECK(Vec2V_Out)   Arctan2( Vec2V_In y, Vec2V_In x );
	FASTRETURNCHECK(Vec3V_Out)   Arctan2( Vec3V_In y, Vec3V_In x );
	FASTRETURNCHECK(Vec4V_Out)   Arctan2( Vec4V_In y, Vec4V_In x );
	FASTRETURNCHECK(ScalarV_Out) Arctan2Fast( ScalarV_In y, ScalarV_In x );										// <COMBINE rage::Arctan2>
	FASTRETURNCHECK(Vec2V_Out)   Arctan2Fast( Vec2V_In y, Vec2V_In x );	  										// <COMBINE rage::Arctan2>
	FASTRETURNCHECK(Vec3V_Out)   Arctan2Fast( Vec3V_In y, Vec3V_In x );	  										// <COMBINE rage::Arctan2>
	FASTRETURNCHECK(Vec4V_Out)   Arctan2Fast( Vec4V_In y, Vec4V_In x );	  										// <COMBINE rage::Arctan2>

	//@@rage::SlowInOut
	// PURPOSE: Maps a [0,1] value to a [0,1] value on an ease curve which has a low slope near 0.0 and 1.0
	// PARAMS:
	//		t - The input value, in the [0,1] range
	// RETURNS: The value on the ease curve for each component in t
	// NOTES:
	//		Called 'SlowInOut' because if t moves linearly from 0.0 to 1.0 the return value accelerates away from 0.0
	//		then decelerates as it approaches 1.0. The ease curve used here is continuous.
	FASTRETURNCHECK(ScalarV_Out) SlowInOut( ScalarV_In t );
	FASTRETURNCHECK(Vec2V_Out)   SlowInOut( Vec2V_In t );
	FASTRETURNCHECK(Vec3V_Out)   SlowInOut( Vec3V_In t );
	FASTRETURNCHECK(Vec4V_Out)   SlowInOut( Vec4V_In t );

	//@@rage::SlowIn
	// PURPOSE: Maps a [0,1] value to a [0,1] value on an ease curve which has a low slope near 1.0
	// PARAMS:
	//		t - The input value, in the [0,1] range
	// RETURNS: The value on the ease curve for each component in t
	// NOTES:
	//		Called 'SlowIn' because if t moves linearly from 0.0 to 1.0 the return value decelerates toward 1.0.
	//		The ease curve used here is continuous.
	FASTRETURNCHECK(ScalarV_Out) SlowIn( ScalarV_In t );
	FASTRETURNCHECK(Vec2V_Out)   SlowIn( Vec2V_In t );
	FASTRETURNCHECK(Vec3V_Out)   SlowIn( Vec3V_In t );
	FASTRETURNCHECK(Vec4V_Out)   SlowIn( Vec4V_In t );

	//@@rage::SlowOut
	// PURPOSE: Maps a [0,1] value to a [0,1] value on an ease curve which has a low slope near 0.0
	// PARAMS:
	//		t - The input value, in the [0,1] range
	// RETURNS: The value on the ease curve for each component in t
	// NOTES:
	//		Called 'SlowOut' because if t moves linearly from 0.0 to 1.0 the return value accelerates away from 0.0.
	//		The ease curve used here is continuous.
	FASTRETURNCHECK(ScalarV_Out) SlowOut( ScalarV_In t );
	FASTRETURNCHECK(Vec2V_Out)   SlowOut( Vec2V_In t );
	FASTRETURNCHECK(Vec3V_Out)   SlowOut( Vec3V_In t );
	FASTRETURNCHECK(Vec4V_Out)   SlowOut( Vec4V_In t );


	//@@rage::BellInOut
	// PURPOSE: Maps a [0,1] value to value on an ease curve which starts at 0.0, rises to 1.0, then returns to 0.0.
	// PARAMS:
	//		t - The input value, in the [0,1] range
	// RETURNS: The value on the ease curve for each component in t
	// NOTES:
	//		Called 'BellInOut' because if t moves linearly from 0.0 to 1.0 the return value accelerates away from 0.0 reaching
	//		1.0 when t = 0.5, then moves back to 0.0, decelerating as t approaches 1.0. The ease curve used here is continuous
	FASTRETURNCHECK(ScalarV_Out) BellInOut( ScalarV_In t );
	FASTRETURNCHECK(Vec2V_Out)   BellInOut( Vec2V_In t );
	FASTRETURNCHECK(Vec3V_Out)   BellInOut( Vec3V_In t );
	FASTRETURNCHECK(Vec4V_Out)   BellInOut( Vec4V_In t );

	//@@rage::Angle
	//<TOCTITLE Angle Functions (Normal\, NormInput)>
	// PURPOSE: Computes the angle between two vectors.
	// PARAMS:
	//		inVect1 - The first vector
	//		inVect2 - The second vector
	// RETURNS:
	//		The unsigned angle between the two vectors, in radians.
	// NOTES:
	//		The NormInput variant is faster, but assumes the input vectors are already normalized.
	FASTRETURNCHECK(ScalarV_Out) Angle(Vec2V_In inVect1, Vec2V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) Angle(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) AngleNormInput(Vec2V_In inVect1, Vec2V_In inVect2);								// <COMBINE rage::Angle>
	FASTRETURNCHECK(ScalarV_Out) AngleNormInput(Vec3V_In inVect1, Vec3V_In inVect2);								// <COMBINE rage::Angle>

	//@@rage::AngleX
	//<TOCTITLE AngleX Functions (Normal\, NormInput)>
	// PURPOSE: Computes the angle between two vectors along a single axis
	// PARAMS:
	//		inVect1 - the first vector
	//		inVect2 - the second vector
	// RETURNS:
	//		The angle between inVect1 and inVect2. The sign of the angle will be the same as the sign of
	//		Cross(inVect1, inVect2)
	// NOTES:
	//		The NormInput variant is faster, but assumes the input vectors are already normalized.
	FASTRETURNCHECK(ScalarV_Out) AngleX(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) AngleXNormInput(Vec3V_In inVect1, Vec3V_In inVect2);								// <COMBINE rage::AngleX>

	//@@rage::AngleY
	//<TOCTITLE AngleY Functions (Normal\, NormInput)> <COPY rage::AngleX>
	FASTRETURNCHECK(ScalarV_Out) AngleY(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) AngleYNormInput(Vec3V_In inVect1, Vec3V_In inVect2);								// <COMBINE rage::AngleY>

	//@@rage::AngleZ
	//<TOCTITLE AngleZ Functions (Normal\, NormInput)> <COPY rage::AngleX>
	FASTRETURNCHECK(ScalarV_Out) AngleZ(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(ScalarV_Out) AngleZNormInput(Vec3V_In inVect1, Vec3V_In inVect2);								// <COMBINE rage::AngleZ>


	//============================================================================
	// Ranges and Interpolation

	//@@rage::Lerp
	// PURPOSE: Lerps (linearly interpolates) between two vectors
	// PARAMS:
	//		tValue - The interpolation "amount" - generally in the [0,1] range
	//		inVect1 - Value to return at t=0
	//		inVect2 - Value to return at t=1
	// RETURNS:
	//		The per-component interpolated value: (1.0 - t.n) * inVect1.n + t.n * inVect2.n
	FASTRETURNCHECK(ScalarV_Out) Lerp( ScalarV_In tValue, ScalarV_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out) Lerp( ScalarV_In tValue, Vec2V_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out) Lerp( Vec2V_In tValues, Vec2V_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out) Lerp( ScalarV_In tValue, Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out) Lerp( Vec3V_In tValues, Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out) Lerp( ScalarV_In tValue, Vec4V_In inVect1, Vec4V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out) Lerp( Vec4V_In tValues, Vec4V_In inVect1, Vec4V_In inVect2 );
	FASTRETURNCHECK(TransformV_Out) Lerp( ScalarV_In tValue, TransformV_In inTrans1, TransformV_In inTrans2 );

	//@@rage::Range
	//<TOCTITLE Range Functions (Normal\, Safe\, Fast)>
	// PURPOSE: Effectively the opposite of lerp. Given a [lower,upper] range and a value, returns the 't' value
	//	for how far in to the range the input vector lies.
	// PARAMS:
	//		inVect - The input vector, generally in the [lower, upper] range.
	//		lower - The lower bounds of the range. When inVect=lower, Range() returns 0.0
	//		upper - The upper bounds of the range. When inVect=upper, Range() returns 1.0
	//		errValVect - For RangeSafe, the value to return if lower == upper
	// RETURNS:
	//		The 't' value, in the [0,1] range if the input vector is in the [lower,upper] range.
	//		(inVect.n - lower.n) / (upper.n - lower.n) for each component n in the input vectors.
	// NOTES:
	//		The Safe variant checks for division by zero, and returns the errValVect value for each component where that would be the case.
	//		The Fast variant uses a faster but less precise inversion function.
	FASTRETURNCHECK(ScalarV_Out) Range( ScalarV_In inVect, ScalarV_In lower, ScalarV_In upper );
	FASTRETURNCHECK(Vec2V_Out) Range( Vec2V_In inVect, Vec2V_In lower, Vec2V_In upper );
	FASTRETURNCHECK(Vec3V_Out) Range( Vec3V_In inVect, Vec3V_In lower, Vec3V_In upper );
	FASTRETURNCHECK(Vec4V_Out) Range( Vec4V_In inVect, Vec4V_In lower, Vec4V_In upper );
	FASTRETURNCHECK(ScalarV_Out) RangeSafe( ScalarV_In inVect, ScalarV_In lower, ScalarV_In upper, ScalarV_In errValVect = ScalarV(V_ZERO) );				// <COMBINE rage::Range>
	FASTRETURNCHECK(Vec2V_Out) RangeSafe( Vec2V_In inVect, Vec2V_In lower, Vec2V_In upper, Vec2V_In errValVect = Vec2V(V_ZERO) );							// <COMBINE rage::Range>
	FASTRETURNCHECK(Vec3V_Out) RangeSafe( Vec3V_In inVect, Vec3V_In lower, Vec3V_In upper, Vec3V_In errValVect = Vec3V(V_ZERO) );							// <COMBINE rage::Range>
	FASTRETURNCHECK(Vec4V_Out) RangeSafe( Vec4V_In inVect, Vec4V_In lower, Vec4V_In upper, Vec4V_In errValVect = Vec4V(V_ZERO) );							// <COMBINE rage::Range>
	FASTRETURNCHECK(ScalarV_Out) RangeFast( ScalarV_In inVect, ScalarV_In lower, ScalarV_In upper );																// <COMBINE rage::Range>
	FASTRETURNCHECK(Vec2V_Out) RangeFast( Vec2V_In inVect, Vec2V_In lower, Vec2V_In upper );																		// <COMBINE rage::Range>
	FASTRETURNCHECK(Vec3V_Out) RangeFast( Vec3V_In inVect, Vec3V_In lower, Vec3V_In upper );																		// <COMBINE rage::Range>
	FASTRETURNCHECK(Vec4V_Out) RangeFast( Vec4V_In inVect, Vec4V_In lower, Vec4V_In upper );																		// <COMBINE rage::Range>
																																	
	//@@rage::RangeClamp
	//<TOCTITLE RangeClamp Functions (Normal\, Fast)>
	// PURPOSE: Given a [lower,upper] range and a value, returns the 't' value for how far into the range the 
	//			input value is, clamped to [0,1]
	// PARAMS:
	//		inVect - The input vector, generally in the [lower, upper] range.
	//		lower - The lower bounds of the range. When inVect <= lower, Range() returns 0.0
	//		upper - The upper bounds of the range. When inVect >= upper, Range() returns 1.0
	// RETURNS:
	//		The 't' value, in the [0,1] range.
	//		(inVect.n - lower.n) / (upper.n - lower.n) for each component n in the input vectors.
	// NOTES:
	//		The Fast variant uses a faster but less precise inversion function.
	FASTRETURNCHECK(ScalarV_Out) RangeClamp( ScalarV_In inVect, ScalarV_In lower, ScalarV_In upper );
	FASTRETURNCHECK(Vec2V_Out) RangeClamp( Vec2V_In inVect, Vec2V_In lower, Vec2V_In upper );
	FASTRETURNCHECK(Vec3V_Out) RangeClamp( Vec3V_In inVect, Vec3V_In lower, Vec3V_In upper );
	FASTRETURNCHECK(Vec4V_Out) RangeClamp( Vec4V_In inVect, Vec4V_In lower, Vec4V_In upper );
	FASTRETURNCHECK(ScalarV_Out) RangeClampFast( ScalarV_In inVect, ScalarV_In lower, ScalarV_In upper );													// <COMBINE rage::RageClamp>
	FASTRETURNCHECK(Vec2V_Out) RangeClampFast( Vec2V_In inVect, Vec2V_In lower, Vec2V_In upper );															// <COMBINE rage::RageClamp>
	FASTRETURNCHECK(Vec3V_Out) RangeClampFast( Vec3V_In inVect, Vec3V_In lower, Vec3V_In upper );															// <COMBINE rage::RageClamp>
	FASTRETURNCHECK(Vec4V_Out) RangeClampFast( Vec4V_In inVect, Vec4V_In lower, Vec4V_In upper );															// <COMBINE rage::RageClamp>

	//@@rage::Ramp
	//<TOCTITLE Ramp Functions (Normal\, Fast)>
	// PURPOSE: Given an input value and an input range [funcInA, funcInB], returns the corresponding
	//			point in the output range, using linear interpolation.
	// PARAMS:
	//		inVect - The input vector, generally in the [funcInA, funcInB] range
	//		funcInA - The lower bound of the input range
	//		funcInB - The upper bound of the input range
	//		funcOutA - The lower bound of the output range
	//		funcOutB - The upper bound of the output range
	// RETURNS:
	//		A per-component interpolated value. If inVect = funcInA, returns funcOutA. If inVect = funcInB, returns funcOutB. 
	//		Between those points, returns values between funcOutA and funcOutB.
	//		This is effectively the same as Lerp(Range(inVect, funcInA, funcInB), funcOutA, funcOutB)
	FASTRETURNCHECK(ScalarV_Out) Ramp( ScalarV_In inVect, ScalarV_In funcInA, ScalarV_In funcInB, ScalarV_In funcOutA, ScalarV_In funcOutB );
	FASTRETURNCHECK(Vec2V_Out) Ramp( Vec2V_In inVect, Vec2V_In funcInA, Vec2V_In funcInB, Vec2V_In funcOutA, Vec2V_In funcOutB );
	FASTRETURNCHECK(Vec3V_Out) Ramp( Vec3V_In inVect, Vec3V_In funcInA, Vec3V_In funcInB, Vec3V_In funcOutA, Vec3V_In funcOutB );
	FASTRETURNCHECK(Vec4V_Out) Ramp( Vec4V_In inVect, Vec4V_In funcInA, Vec4V_In funcInB, Vec4V_In funcOutA, Vec4V_In funcOutB );
	FASTRETURNCHECK(ScalarV_Out) RampFast( ScalarV_In inVect, ScalarV_In funcInA, ScalarV_In funcInB, ScalarV_In funcOutA, ScalarV_In funcOutB );
	FASTRETURNCHECK(Vec2V_Out) RampFast( Vec2V_In inVect, Vec2V_In funcInA, Vec2V_In funcInB, Vec2V_In funcOutA, Vec2V_In funcOutB );
	FASTRETURNCHECK(Vec3V_Out) RampFast( Vec3V_In inVect, Vec3V_In funcInA, Vec3V_In funcInB, Vec3V_In funcOutA, Vec3V_In funcOutB );
	FASTRETURNCHECK(Vec4V_Out) RampFast( Vec4V_In inVect, Vec4V_In funcInA, Vec4V_In funcInB, Vec4V_In funcOutA, Vec4V_In funcOutB );

	//@@rage::SplatX
	// PURPOSE: Returns the X component of the input vector, as a scalar. Same as vec.GetX()
	BoolV_Out   SplatX( VecBoolV_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatX( Vec2V_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatX( Vec3V_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatX( Vec4V_In inVect1 );

	//@@rage::SplatY
	// PURPOSE: Returns the Y component of the input vector, as a scalar. Same as vec.GetY()
	BoolV_Out   SplatY( VecBoolV_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatY( Vec2V_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatY( Vec3V_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatY( Vec4V_In inVect1 );

	//@@rage::SplatZ
	// PURPOSE: Returns the Z component of the input vector, as a scalar. Same as vec.GetZ()
	BoolV_Out   SplatZ( VecBoolV_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatZ( Vec3V_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatZ( Vec4V_In inVect1 );

	//@@rage::SplatW
	// PURPOSE: Returns the W component of the input vector, as a scalar. Same as vec.GetW()
	BoolV_Out   SplatW( VecBoolV_In inVect1 );
	FASTRETURNCHECK(ScalarV_Out) SplatW( Vec4V_In inVect1 );

	//@@rage::MergeXY
	// PURPOSE: Merges the X and Y components of two input vectors together, returning (a.x, b.x, a.y, b.y)
	// PARAMS:
	//		a - The first input vector
	//		b - The second input vector
	// RETURNS: A vector containing (a.x, b.x, a.y, b.y)
	FASTRETURNCHECK(Vec4V_Out) MergeXY( Vec4V_In a, Vec4V_In b );
	FASTRETURNCHECK(Vec4V_Out) MergeXY( Vec3V_In a, Vec3V_In b );
	FASTRETURNCHECK(Vec4V_Out) MergeXY( Vec2V_In a, Vec2V_In b );
	FASTRETURNCHECK(VecBoolV_Out) MergeXY( VecBoolV_In a, VecBoolV_In b );

	//PURPOSE: Merges the Z and W components of two input vectors together, returning (a.z, b.z, a.w, b.w)
	// PARAMS:
	//		a - The first input vector
	//		b - The second input vector
	// RETURNS: A vector containing (a.z, b.z, a.w, b.w)
	FASTRETURNCHECK(Vec4V_Out) MergeZW( Vec4V_In a, Vec4V_In b );
	FASTRETURNCHECK(VecBoolV_Out) MergeZW( VecBoolV_In a, VecBoolV_In b );

	//@@rage::MergeXYShort
	// PURPOSE: Treats two vectors as arrays of short (16bit) values, merges their first halves.
	// PARAMS:
	//		a - The first input vector, treated as an array of 8 shorts
	//		b - The second input vector, treated as an array of 8 shorts
	// RETURNS: A new vector containing the interleaved shorts: (a[0], b[0], a[1], b[1], a[2], b[2], a[3], b[3])
	// NOTES: WARNING: You should take into account that Win32 (SSE) will
	// act differently if your vectors are in-register. You must take this into account! See changelist 157232.
	FASTRETURNCHECK(Vec4V_Out) MergeXYShort( Vec4V_In a, Vec4V_In b );
	FASTRETURNCHECK(Vec4V_Out) MergeXYShort( Vec3V_In a, Vec3V_In b );
	FASTRETURNCHECK(Vec4V_Out) MergeXYShort( Vec2V_In a, Vec2V_In b );

	//@@rage::MergeXYByte
	// PURPOSE: Treats two vectors as arrays of bytes, merges their first halves.
	// PARAMS:
	//		a - The first input vector, treated as an array of 16 bytes
	//		b - The second input vector, treated as an array of 16 bytes
	// RETURNS: A new vector containing the interleaved bytes: (a[0], b[0], a[1], b[1], a[2], b[2], a[3], b[3], a[4], b[4], a[5], b[5], a[6], b[6], a[7], b[7])
	// NOTES: WARNING: You should take into account that Win32 (SSE) will
	// act differently if your vectors are in-register. You must take this into account! See changelist 157232.
	FASTRETURNCHECK(Vec4V_Out) MergeXYByte( Vec4V_In a, Vec4V_In b );
	FASTRETURNCHECK(Vec4V_Out) MergeXYByte( Vec3V_In a, Vec3V_In b );
	FASTRETURNCHECK(Vec4V_Out) MergeXYByte( Vec2V_In a, Vec2V_In b );

	// PURPOSE: Treats two vectors as arrays of short (16bit) values, merges their last halves.
	// PARAMS:
	//		a - The first input vector, treated as an array of 8 shorts
	//		b - The second input vector, treated as an array of 8 shorts
	// RETURNS: A new vector containing the interleaved shorts: (a[4], b[4], a[5], b[5], a[6], b[6], a[7], b[7])
	// NOTES: WARNING: You should take into account that Win32 (SSE) will
	// act differently if your vectors are in-register. You must take this into account! See changelist 157232.
	FASTRETURNCHECK(Vec4V_Out) MergeZWShort( Vec4V_In a, Vec4V_In b );

	//@@rage::MergeXYByte
	// PURPOSE: Treats two vectors as arrays of bytes, merges their last halves.
	// PARAMS:
	//		a - The first input vector, treated as an array of 16 bytes
	//		b - The second input vector, treated as an array of 16 bytes
	// RETURNS: A new vector containing the interleaved bytes: (a[8], b[8], a[9], b[9], a[10], b[10], a[11], b[11], a[12], b[12], a[13], b[13], a[14], b[14], a[15], b[15])
	// NOTES: WARNING: You should take into account that Win32 (SSE) will
	// act differently if your vectors are in-register. You must take this into account! See changelist 157232.
	FASTRETURNCHECK(Vec4V_Out) MergeZWByte( Vec4V_In a, Vec4V_In b );

	//@@rage::Pow
	// PURPOSE: Computes a^b for each element in an input vector
	// NOTES: The Precise variant returns a more accurate value, but may take significantly longer to execute
	FASTRETURNCHECK(ScalarV_Out) Pow( ScalarV_In a, ScalarV_In b );
	FASTRETURNCHECK(Vec2V_Out)   Pow( Vec2V_In a, Vec2V_In b );
	FASTRETURNCHECK(Vec3V_Out)   Pow( Vec3V_In a, Vec3V_In b );
	FASTRETURNCHECK(Vec4V_Out)   Pow( Vec4V_In a, Vec4V_In b );

	FASTRETURNCHECK(ScalarV_Out) PowPrecise( ScalarV_In a, ScalarV_In b );
	FASTRETURNCHECK(Vec2V_Out)   PowPrecise( Vec2V_In a, Vec2V_In b );
	FASTRETURNCHECK(Vec3V_Out)   PowPrecise( Vec3V_In a, Vec3V_In b );
	FASTRETURNCHECK(Vec4V_Out)   PowPrecise( Vec4V_In a, Vec4V_In b );

	//@@rage::Expt
	// PURPOSE: Computes 2^a for each element in an input vector
	FASTRETURNCHECK(ScalarV_Out) Expt( ScalarV_In a );
	FASTRETURNCHECK(Vec2V_Out)   Expt( Vec2V_In a );
	FASTRETURNCHECK(Vec3V_Out)   Expt( Vec3V_In a );
	FASTRETURNCHECK(Vec4V_Out)   Expt( Vec4V_In a );

	//@@rage::Log2
	// PURPOSE: Computes log2(a) for each element in an input vector
	FASTRETURNCHECK(ScalarV_Out) Log2( ScalarV_In a );
	FASTRETURNCHECK(Vec2V_Out)   Log2( Vec2V_In a );
	FASTRETURNCHECK(Vec3V_Out)   Log2( Vec3V_In a );
	FASTRETURNCHECK(Vec4V_Out)   Log2( Vec4V_In a );

	//@@rage::Log10
	// PURPOSE: Computes log(a) (log base 10) for each element in an input vector
	FASTRETURNCHECK(ScalarV_Out) Log10( ScalarV_In a );
	FASTRETURNCHECK(Vec2V_Out)   Log10( Vec2V_In a );
	FASTRETURNCHECK(Vec3V_Out)   Log10( Vec3V_In a );
	FASTRETURNCHECK(Vec4V_Out)   Log10( Vec4V_In a );

	//@@rage::SelectFT
	// PURPOSE: Builds a vector from components of two input vectors and a choice vector saying which to use
	// PARAMS:
	//		choiceVector - A vector of bools (or a single BoolV) that determine which components of the input vectors to use
	//		ifFalse - The value to return if the choiceVector element is false
	//		ifTrue - The value to return if the choiceVector element is true
	// RETURNS: choiceVector.n ? ifTrue.n : ifFalse.n for each element n in the input vectors
	// NOTES: Note the argument order difference between this and the ?: operator. "FT" stands for "False, True".
	//		This can be significantly faster than doing an ?: test or an if() branch.
	FASTRETURNCHECK(ScalarV_Out) SelectFT(VecBoolV_In choiceVector, ScalarV_In ifFalse, ScalarV_In ifTrue);
	FASTRETURNCHECK(Vec2V_Out)   SelectFT(VecBoolV_In choiceVector, Vec2V_In ifFalse, Vec2V_In ifTrue);
	FASTRETURNCHECK(Vec3V_Out)   SelectFT(VecBoolV_In choiceVector, Vec3V_In ifFalse, Vec3V_In ifTrue);
	FASTRETURNCHECK(Vec4V_Out)   SelectFT(VecBoolV_In choiceVector, Vec4V_In ifFalse, Vec4V_In ifTrue);
	FASTRETURNCHECK(QuatV_Out)   SelectFT(VecBoolV_In choiceVector, QuatV_In ifFalse, QuatV_In ifTrue);
	FASTRETURNCHECK(Mat33V_Out)  SelectFT(VecBoolV_In choiceVector, Mat33V_In ifFalse, Mat33V_In ifTrue);
	FASTRETURNCHECK(ScalarV_Out) SelectFT(BoolV_In choiceVector, ScalarV_In ifFalse, ScalarV_In ifTrue);
	FASTRETURNCHECK(Vec2V_Out)   SelectFT(BoolV_In choiceVector, Vec2V_In ifFalse, Vec2V_In ifTrue);
	FASTRETURNCHECK(Vec3V_Out)   SelectFT(BoolV_In choiceVector, Vec3V_In ifFalse, Vec3V_In ifTrue);
	FASTRETURNCHECK(Vec4V_Out)   SelectFT(BoolV_In choiceVector, Vec4V_In ifFalse, Vec4V_In ifTrue);
	FASTRETURNCHECK(QuatV_Out)   SelectFT(BoolV_In choiceVector, QuatV_In ifFalse, QuatV_In ifTrue);
	FASTRETURNCHECK(Mat33V_Out)  SelectFT(BoolV_In choiceVector, Mat33V_In ifFalse, Mat33V_In ifTrue);
	FASTRETURNCHECK(TransformV_Out) SelectFT( BoolV_In choiceVector, TransformV_In ifFalse, TransformV_In ifTrue );
	FASTRETURNCHECK(TransformV_Out) SelectFT( VecBoolV_In choiceVector, TransformV_In ifFalse, TransformV_In ifTrue );

	//@@rage::Max
	// PURPOSE: Returns a vector where each component contains the maximum of the two input vector components.
	FASTRETURNCHECK(ScalarV_Out) Max(ScalarV_In inVect1, ScalarV_In inVect2);
	FASTRETURNCHECK(Vec2V_Out)   Max(Vec2V_In inVect1, Vec2V_In inVect2);
	FASTRETURNCHECK(Vec3V_Out)   Max(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(Vec4V_Out)   Max(Vec4V_In inVect1, Vec4V_In inVect2);

#if __WIN32PC
	// We have a specific three and four variant for PC as if we rely on the template Max(a,b,c,d), alignment breaks
	FASTRETURNCHECK(ScalarV_Out) Max(ScalarV_In inVect1, ScalarV_In inVect2, ScalarV_In inVect3);
	FASTRETURNCHECK(Vec2V_Out)   Max(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In inVect3);
	FASTRETURNCHECK(Vec3V_Out)   Max(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In inVect3);
	FASTRETURNCHECK(Vec4V_Out)   Max(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In inVect3);

	FASTRETURNCHECK(ScalarV_Out) Max(ScalarV_In inVect1, ScalarV_In inVect2, ScalarV_In inVect3, ScalarV_In inVect4);
	FASTRETURNCHECK(Vec2V_Out)   Max(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In inVect3, Vec2V_In inVect4);
	FASTRETURNCHECK(Vec3V_Out)   Max(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In inVect3, Vec3V_In inVect4);
	FASTRETURNCHECK(Vec4V_Out)   Max(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In inVect3, Vec4V_In inVect4);
#endif

	//@@rage::Min
	// PURPOSE: Returns a vector where each component contains the minimum of the two input vector components.
	FASTRETURNCHECK(ScalarV_Out) Min(ScalarV_In inVect1, ScalarV_In inVect2);
	FASTRETURNCHECK(Vec2V_Out)   Min(Vec2V_In inVect1, Vec2V_In inVect2);
	FASTRETURNCHECK(Vec3V_Out)   Min(Vec3V_In inVect1, Vec3V_In inVect2);
	FASTRETURNCHECK(Vec4V_Out)   Min(Vec4V_In inVect1, Vec4V_In inVect2);

#if __WIN32PC
	FASTRETURNCHECK(ScalarV_Out) Min(ScalarV_In inVect1, ScalarV_In inVect2, ScalarV_In inVect3);
	FASTRETURNCHECK(Vec2V_Out)   Min(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In inVect3);
	FASTRETURNCHECK(Vec3V_Out)   Min(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In inVect3);
	FASTRETURNCHECK(Vec4V_Out)   Min(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In inVect3);

	FASTRETURNCHECK(ScalarV_Out) Min(ScalarV_In inVect1, ScalarV_In inVect2, ScalarV_In inVect3, ScalarV_In inVect4);
	FASTRETURNCHECK(Vec2V_Out)   Min(Vec2V_In inVect1, Vec2V_In inVect2, Vec2V_In inVect3, Vec2V_In inVect4);
	FASTRETURNCHECK(Vec3V_Out)   Min(Vec3V_In inVect1, Vec3V_In inVect2, Vec3V_In inVect3, Vec3V_In inVect4);
	FASTRETURNCHECK(Vec4V_Out)   Min(Vec4V_In inVect1, Vec4V_In inVect2, Vec4V_In inVect3, Vec4V_In inVect4);
#endif // __WIN32PC

	//@@rage::MaxElement
	// PURPOSE: Returns a scalar containing the largest of the components in inVect
	FASTRETURNCHECK(ScalarV_Out) MaxElement(Vec2V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) MaxElement(Vec3V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) MaxElement(Vec4V_In inVect);

	//@@rage::MinElement
	// PURPOSE: Returns a scalar containing the smallest of the components in inVect
	FASTRETURNCHECK(ScalarV_Out) MinElement(Vec2V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) MinElement(Vec3V_In inVect);
	FASTRETURNCHECK(ScalarV_Out) MinElement(Vec4V_In inVect);

	//@@rage::AddInt
	// PURPOSE: Adds two vectors, interpreting the vector contents as integers
	FASTRETURNCHECK(ScalarV_Out) AddInt( ScalarV_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out)   AddInt( Vec2V_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out)   AddInt( Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out)   AddInt( Vec4V_In inVect1, Vec4V_In inVect2 );

	//@@rage::SubtractInt
	// PURPOSE: Subtracts two vectors, interpreting the vector contents as integers
	FASTRETURNCHECK(ScalarV_Out) SubtractInt( ScalarV_In inVect1, ScalarV_In inVect2 );
	FASTRETURNCHECK(Vec2V_Out)   SubtractInt( Vec2V_In inVect1, Vec2V_In inVect2 );
	FASTRETURNCHECK(Vec3V_Out)   SubtractInt( Vec3V_In inVect1, Vec3V_In inVect2 );
	FASTRETURNCHECK(Vec4V_Out)   SubtractInt( Vec4V_In inVect1, Vec4V_In inVect2 );

	//@@rage::InvertBits
	// PURPOSE: Inverts all of the bits in a vector - this can also be used for a logical 'Not' for a bool vector
	BoolV_Out    InvertBits(BoolV_In inVect);
	VecBoolV_Out InvertBits(VecBoolV_In inVect);
	FASTRETURNCHECK(ScalarV_Out)  InvertBits(ScalarV_In inVect);
	FASTRETURNCHECK(Vec2V_Out)    InvertBits(Vec2V_In inVect);
	FASTRETURNCHECK(Vec3V_Out)    InvertBits(Vec3V_In inVect);
	FASTRETURNCHECK(Vec4V_Out)    InvertBits(Vec4V_In inVect);

	//@@rage::And
	// PURPOSE: Returns the bitwise 'And' of the bits in two vectors - like operator&
	BoolV_Out    And(BoolV_In inVector1, BoolV_In inVector2);
	VecBoolV_Out And(VecBoolV_In inVector1, VecBoolV_In inVector2);
	FASTRETURNCHECK(ScalarV_Out)  And(ScalarV_In inVector1, ScalarV_In inVector2);
	FASTRETURNCHECK(Vec2V_Out)    And(Vec2V_In inVector1, Vec2V_In inVector2);
	FASTRETURNCHECK(Vec3V_Out)    And(Vec3V_In inVector1, Vec3V_In inVector2);
	FASTRETURNCHECK(Vec4V_Out)    And(Vec4V_In inVector1, Vec4V_In inVector2);

	//@@rage::Or
	// PURPOSE: Returns the bitwise 'Or' of the bits in two vectors - like operator|
	BoolV_Out    Or(BoolV_In inVector1, BoolV_In inVector2);
	VecBoolV_Out Or(VecBoolV_In inVector1, VecBoolV_In inVector2);
	FASTRETURNCHECK(ScalarV_Out)  Or(ScalarV_In inVector1, ScalarV_In inVector2);
	FASTRETURNCHECK(Vec2V_Out)    Or(Vec2V_In inVector1, Vec2V_In inVector2);
	FASTRETURNCHECK(Vec3V_Out)    Or(Vec3V_In inVector1, Vec3V_In inVector2);
	FASTRETURNCHECK(Vec4V_Out)    Or(Vec4V_In inVector1, Vec4V_In inVector2);

	//@@rage::Xor
	// PURPOSE: Returns the bitwise 'Xor' of the bits in two vectors - like operator^
	BoolV_Out    Xor(BoolV_In inVector1, BoolV_In inVector2);
	VecBoolV_Out Xor(VecBoolV_In inVector1, VecBoolV_In inVector2);
	FASTRETURNCHECK(ScalarV_Out)  Xor(ScalarV_In inVector1, ScalarV_In inVector2);
	FASTRETURNCHECK(Vec2V_Out)    Xor(Vec2V_In inVector1, Vec2V_In inVector2);
	FASTRETURNCHECK(Vec3V_Out)    Xor(Vec3V_In inVector1, Vec3V_In inVector2);
	FASTRETURNCHECK(Vec4V_Out)    Xor(Vec4V_In inVector1, Vec4V_In inVector2);

	//@@rage::Andc
	// PURPOSE: And-with-complement. Returns 'a & !b' for the bits in two vectors
	BoolV_Out    Andc(BoolV_In inVector1, BoolV_In inVector2);
	VecBoolV_Out Andc(VecBoolV_In inVector1, VecBoolV_In inVector2);
	FASTRETURNCHECK(ScalarV_Out)  Andc(ScalarV_In inVector1, ScalarV_In inVector2);
	FASTRETURNCHECK(Vec2V_Out)    Andc(Vec2V_In inVector1, Vec2V_In inVector2);
	FASTRETURNCHECK(Vec3V_Out)    Andc(Vec3V_In inVector1, Vec3V_In inVector2);
	FASTRETURNCHECK(Vec4V_Out)    Andc(Vec4V_In inVector1, Vec4V_In inVector2);

	//@@rage::GetFromTwo
	// PURPOSE: Combines any components of two vectors to form a new vector
	// PARAMS:
	//		permX, permY, permZ, permW - Specifies which components to take from the input vectors - see below.
	//		inVector1 - The first source vector
	//		inVector2 - The second source vector
	//		controlVec - For control vector overloads, contains a vector specifying which bytes go where.
	// RETURNS:
	//		A new vector containing the components of inVector1 and inVector2, where the specific components and their
	//		order are specified by the permN values.
	// EXAMPLE:
	//		GetFromTwo<Vec::X1, Vec::Z2, Vec::W1>(a, b) -> (a.x, b.z, a.w) 
	//		GetFromTwo<Vec::X1, Vec::Y1, Vec::Z1, Vec::W2>(a,b) -> (a.x, a.y, a.z, b.w)
	//		
	//		GetFromTwo(a, b, Vec4VConstant<0x0f0e0d0c, 0x10111213, 0x07060504, 0x10111213>) -> a new vector containing (a.w (with its bytes shuffled), b.x, a.y (with its bytes shuffled), b.x)
	// NOTES:
	//		There are a bunch of specializations of the templates, so that depending on 
	//		what you choose as your perm specifiers we can often do a faster permute than
	//		what a normal vperm intrinsic could do.
	//		When using the controlVec version of this function, each byte in the control vector specifies which 
	//		source vector and which byte from the source vector to copy. Values 0x0 to 0xf represent bytes in inVector1, and
	//		0x10 to 0x1f represent bytes in inVector2
	template <u32 permX, u32 permY>	FASTRETURNCHECK(Vec2V_Out)						GetFromTwo( Vec2V_In inVector1, Vec2V_In inVector2 );
	template <u32 permX, u32 permY>	FASTRETURNCHECK(Vec2V_Out)						GetFromTwo( Vec3V_In inVector1, Vec3V_In inVector2 );
	template <u32 permX, u32 permY>	FASTRETURNCHECK(Vec2V_Out)						GetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2 );
	template <u32 permX, u32 permY, u32 permZ> FASTRETURNCHECK(Vec3V_Out)			GetFromTwo( Vec3V_In inVector1, Vec3V_In inVector2 );
	template <u32 permX, u32 permY, u32 permZ> FASTRETURNCHECK(Vec3V_Out)			GetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2 );
	template <u32 permX, u32 permY, u32 permZ, u32 permW> FASTRETURNCHECK(Vec4V_Out) GetFromTwo( Vec3V_In inVector1, Vec3V_In inVector2 );
	template <u32 permX, u32 permY, u32 permZ, u32 permW> FASTRETURNCHECK(Vec4V_Out) GetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2 );

#if __XENON || __PS3
	FASTRETURNCHECK(Vec4V_Out) GetFromTwo( Vec3V_In inVector1, Vec3V_In inVector2, Vec4V_In controlVec );
	FASTRETURNCHECK(Vec4V_Out) GetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2, Vec4V_In controlVec );

	// PURPOSE: Combines any bytes of two vectors to form a new vector
	// PARAMS:
	//		byte0..byte15 - the specifiers for which bytes to take from which vectors - 0x00 to 0x0f select from the first vector, 0x10 to 0x1f select from the second.
	//		inVector1 - the first source vector
	//		inVector2 - the second source vector
	// RETURNS:
	//		A new vector containing bytes from inVector1 and inVector2, where the specific bytes and their order are specified by
	//		the byteN values.
	// EXAMPLE:
	//		ByteGetFromTwo<0x00, 0x11, 0x12, 0x13, 0x04, 0x15, 0x16, 0x17, 0x08, 0x19, 0x1a, 0x1b, 0x0c, 0x1d, 0x1e, 0x1f>(a, b) - takes the high byte of each word from a, and the low bytes of each word from b.
	template <u8 byte0,u8 byte1,u8 byte2,u8 byte3,u8 byte4,u8 byte5,u8 byte6,u8 byte7,u8 byte8,u8 byte9,u8 byte10,u8 byte11,u8 byte12,u8 byte13,u8 byte14,u8 byte15>
	FASTRETURNCHECK(Vec4V_Out) ByteGetFromTwo( Vec4V_In inVector1, Vec4V_In inVector2 );
#endif

	// PURPOSE: Converts a 3-valued VecBoolV into an integer bitset, with bit 0 representing inVec.x
	// PARAMS:
	//		outInt - The output value, which will contain a value between 0 and 7
	//		inVec - The input vec, containing 3 boolean values.
	// NOTES:
	//		outInt bits are set based on the contents of inVec - outInt bit 0 is set if inVec.x is true, etc.
	void ResultToIndexZYX( u32& outInt,VecBoolV_In inVec );

	// PURPOSE: Converts a 3-valued VecBoolV into an integer bitset, with bit 0 representing inVec.z
	// PARAMS:
	//		outInt - The output value, which will contain a value between 0 and 7
	//		inVec - The input vec, containing 3 boolean values.
	// NOTES:
	//		outInt bits are set based on the contents of inVec - outInt bit 0 is set if inVec.z is true, etc.
	void ResultToIndexXYZ( u32& outInt,VecBoolV_In inVec );

	// PURPOSE: Given one input vector, generate two other vectors such that the three are mutually orthogonal
	// PARAMS:
	//		inVector - The source vector
	//		ortho1 - The first (output) orthogonal vector
	//		ortho2 - The second (output) orthogonal vector
	void MakeOrthonormals(Vec3V_In inVector, Vec3V_InOut ortho1, Vec3V_InOut ortho2);

	//rsTODO: Document this
	FASTRETURNCHECK(ScalarV_Out) WhichSideOfLineV(Vec2V_In point, Vec2V_In lineP1, Vec2V_In lineP2);

	//rsTODO: Document this
	// rate and time should be one value splatted into .x/.y/.z
	FASTRETURNCHECK(Vec3V_Out) ApproachStraight(Vec3V_In inVect, Vec3V_In goal, Vec3V_In rate, Vec3V_In time, unsigned int& rResult);
	FASTRETURNCHECK(Vec2V_Out) ApproachStraight(Vec2V_In inVect, Vec2V_In goal, Vec2V_In rate, Vec2V_In time, unsigned int& rResult);

	//rsTODO: Document this
	// The amount is assumed to be in each of amount.x/.y/.z.
	FASTRETURNCHECK(Vec2V_Out) Extend( Vec2V_In inVect, Vec2V_In amount );
	FASTRETURNCHECK(Vec3V_Out) Extend( Vec3V_In inVect, Vec3V_In amount );

	//@@rage::Reflect
	// PURPOSE: Computes a reflection vector, given an input vector and a vector normal to the plane to reflect along
	// PARAMS:
	//		inVect - the vector to reflect
	//		wallNormal - the vector normal to the line or plane to reflect from
	// RETURNS: The reflected vector
	// NOTES: assumes inVect and wallNormal are normalized.
	FASTRETURNCHECK(Vec2V_Out) Reflect( Vec2V_In inVect, Vec2V_In wallNormal );
	FASTRETURNCHECK(Vec3V_Out) Reflect( Vec3V_In inVect, Vec3V_In wallNormal );

	//@@rage::OuterProduct
	// PURPOSE: Computes the outer product of two vectors (a * b' if a and b are column vectors)
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		a - The first input vector
	//		b - The second input vector
	void OuterProduct( Mat33V_InOut outMat, Vec3V_In a, Vec3V_In b );
	void OuterProduct( Mat44V_InOut outMat, Vec4V_In a, Vec4V_In b);

	//@@rage::Determinant
	//<TOCTITLE Determinant Functions (Normal\, 3x3)>
	// PURPOSE: Computes the determinant of a matrix.
	// NOTES:
	//		The 3x3 variant computes the determinant of the upper left 3x3 portion of the matrix. Other values are copied directly from mat.
	FASTRETURNCHECK(ScalarV_Out) Determinant( Mat33V_In mat );
	FASTRETURNCHECK(ScalarV_Out) Determinant( Mat44V_In mat );
	FASTRETURNCHECK(ScalarV_Out) Determinant3x3( Mat34V_In mat );									// <COMBINE rage::Determinant>
	FASTRETURNCHECK(ScalarV_Out) Determinant3x3( Mat44V_In mat );									// <COMBINE rage::Determinant>

	//<GROUP rage>
	//<TOCTITLE Add (Matrix) Functions (Normal\, 3x3\, 4x3)>
	// PURPOSE: Adds two matrices
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		a - The first input matrix
	//		b - The second input matrix
	// NOTES:
	//		The 3x3 and 4x3 variants only add the upper left portion of b to a. Other values are copied directly from a.
	void Add( Mat33V_InOut outMat, Mat33V_In a, Mat33V_In b );						
	void Add( Mat34V_InOut outMat, Mat34V_In a, Mat34V_In b );						// <COMBINE rage::Add@Mat33V_InOut@Mat33V_In@Mat33V_In>
	void Add( Mat44V_InOut outMat, Mat44V_In a, Mat44V_In b );						// <COMBINE rage::Add@Mat33V_InOut@Mat33V_In@Mat33V_In>
	void Add3x3( Mat34V_InOut outMat, Mat34V_In a, Mat34V_In b );					// <COMBINE rage::Add@Mat33V_InOut@Mat33V_In@Mat33V_In>
	void Add4x3( Mat44V_InOut outMat, Mat44V_In a, Mat44V_In b );					// <COMBINE rage::Add@Mat33V_InOut@Mat33V_In@Mat33V_In>

	//@@rage::Subtract_mtx
	//<TOCTITLE Subtract (Matrix) Function>
	// PURPOSE: Subtracts two matrices
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		a - The first input matrix
	//		b - The second input matrix
	void Subtract( Mat33V_InOut outMat, Mat33V_In a, Mat33V_In b );					// <COMBINE rage::Subtract_mtx>
	void Subtract( Mat34V_InOut outMat, Mat34V_In a, Mat34V_In b );					// <COMBINE rage::Subtract_mtx>
	void Subtract( Mat44V_InOut outMat, Mat44V_In a, Mat44V_In b );					// <COMBINE rage::Subtract_mtx>


	//<TOCTITLE Abs (Matrix) Functions (Normal\, 3x3)>
	// PURPOSE: Computes the absolute value of each component in a matrix
	// PARAMS:
	//		inOutMat - A matrix that gets modified in-place
	//		outMat - A matrix that will contain the result
	//		a - The input matrix
	// NOTES:
	//		The 3x3 variant only modifies the upper left 3x3 portion of the matrix. Other values are copied directly from a.
	void Abs( Mat33V_InOut inOutMat );
	void Abs( Mat33V_InOut outMat, Mat33V_In a );									// <COMBINE rage::Abs@Mat33V_InOut>
	void Abs( Mat34V_InOut outMat, Mat34V_In a );									// <COMBINE rage::Abs@Mat33V_InOut>
	void Abs( Mat44V_InOut outMat, Mat44V_In a );									// <COMBINE rage::Abs@Mat33V_InOut>
	void Abs3x3( Mat34V_InOut outMat, Mat34V_In a );								// <COMBINE rage::Abs@Mat33V_InOut>

	//<TOCTITLE Scale Functions (Normal\, 3x3)>
	// PURPOSE: Scales each column in a matrix by the values in a vector
	// PARAMS:
	//		outMat - A matrix that will contain the result
	//		inVect - A vector containing the scale values
	//		inMtx - A matrix whose columns will be scaled
	// NOTES:
	//		Each element of outmat will contain inMtx[n].m * inVect.m for all columns n in inMtx and elements m
	//		The 3x3 variant only scales the leftmost 3 columns of the input matrix
	void Scale( Mat33V_InOut outMat, Vec3V_In inVect, Mat33V_In inMtx );
	void Scale( Mat33V_InOut outMat, Mat33V_In inMtx, Vec3V_In inVect );			// <COMBINE rage::Scale@Mat33V_InOut@Vec3V_In@Mat33V_In>
	void Scale( Mat34V_InOut outMat, Vec3V_In inVect, Mat34V_In inMtx );			// <COMBINE rage::Scale@Mat33V_InOut@Vec3V_In@Mat33V_In>
	void Scale( Mat34V_InOut outMat, Mat34V_In inMtx, Vec3V_In inVect );			// <COMBINE rage::Scale@Mat33V_InOut@Vec3V_In@Mat33V_In>
	void Scale( Mat44V_InOut outMat, Vec4V_In inVect, Mat44V_In inMtx );			// <COMBINE rage::Scale@Mat33V_InOut@Vec3V_In@Mat33V_In>
	void Scale( Mat44V_InOut outMat, Mat44V_In inMtx, Vec4V_In inVect );			// <COMBINE rage::Scale@Mat33V_InOut@Vec3V_In@Mat33V_In>
	void Scale3x3( Mat34V_InOut outMat, Mat34V_In inMtx, Vec3V_In inVect );			// <COMBINE rage::Scale@Mat33V_InOut@Vec3V_In@Mat33V_In>
	void Scale3x3( Mat34V_InOut outMat, Vec3V_In inVect, Mat34V_In inMtx );			// <COMBINE rage::Scale@Mat33V_InOut@Vec3V_In@Mat33V_In>

	//<TOCTITLE Scale Functions (Normal\, Safe\, Fast\, FastSafe)>
	// PURPOSE: Scales each column in a matrix by the recirprocal of the values in a vector
	// PARAMS:
	//		outMat - A matrix that will contain the result
	// PARAMS:
	//		toScale - the matrix whose columns get scaled
	//		scaleValue - The vector to invert, then scale by
	//		errValVect - Values to return whenever scaleValue.n would be invalid
	// NOTES:
	//		The Safe variant returns errValVect.n whenever scaleValue.n == 0.0
	//		The Fast variant is faster but less precise
	//		Each element of outmat will contain inMtx[n].m / inVect.m for all columns n in inMtx and elements m
	void InvScale( Mat33V_InOut outMat, Mat33V_In toScale, Vec3V_In scaleValue );
	void InvScale( Mat34V_InOut outMat, Mat34V_In toScale, Vec3V_In scaleValue );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScale( Mat44V_InOut outMat, Mat44V_In toScale, Vec4V_In scaleValue );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleSafe( Mat33V_InOut outMat, Mat33V_In toScale, Vec3V_In scaleValue, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8) );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleSafe( Mat34V_InOut outMat, Mat34V_In toScale, Vec3V_In scaleValue, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8) );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleSafe( Mat44V_InOut outMat, Mat44V_In toScale, Vec4V_In scaleValue, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8) );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleFast( Mat33V_InOut outMat, Mat33V_In toScale, Vec3V_In scaleValue );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleFast( Mat34V_InOut outMat, Mat34V_In toScale, Vec3V_In scaleValue );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleFast( Mat44V_InOut outMat, Mat44V_In toScale, Vec4V_In scaleValue );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleFastSafe( Mat33V_InOut outMat, Mat33V_In toScale, Vec3V_In scaleValue, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8) );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleFastSafe( Mat34V_InOut outMat, Mat34V_In toScale, Vec3V_In scaleValue, Vec3V_In errValVect = Vec3V(V_FLT_LARGE_8) );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>
	void InvScaleFastSafe( Mat44V_InOut outMat, Mat44V_In toScale, Vec4V_In scaleValue, Vec4V_In errValVect = Vec4V(V_FLT_LARGE_8) );				// <COMBINE rage::InvScale@Mat33V_InOut@Mat33V_In@Vec3V_In>

	// <TOCTITLE AddScaled Functions (Normal\, 3x3\, 4x3)>
	// PURPOSE: Scales one matrix by a matrix or vector, then adds a third
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		toAdd - The vector to add
	//		toScaleThenAdd - One of the two vectors to scale
	//		scaleValue - The other vector to scale
	// NOTES: 
	//		When scaleValue is a matrix: outMat[n].m = toAdd[n].m + (toScaleThenAdd[n].m * scaleValue[n].m) For all columns n in the matrices and elements m in the vectors
	//		When scaleValue is a vector: outMat[n].m = toAdd[n].m + (toScaleThenAdd[n].m * scaleValue.m) For all columns n in the matrices and elements m in the vectors
	//		The 3x3 and 4x3 variants only do the AddScaled operation on the upper left portion of the matrix. Remaining elements are copied from toAdd.
	void AddScaled( Mat33V_InOut outMat, Mat33V_In toAdd, Mat33V_In toScaleThenAdd, Mat33V_In scaleValue );
	void AddScaled( Mat33V_InOut outMat, Mat33V_In toAdd, Mat33V_In toScaleThenAdd, Vec3V_In scaleValue );							// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>
	void AddScaled( Mat34V_InOut outMat, Mat34V_In toAdd, Mat34V_In toScaleThenAdd, Mat34V_In scaleValue );							// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>
	void AddScaled( Mat34V_InOut outMat, Mat34V_In toAdd, Mat34V_In toScaleThenAdd, Vec3V_In scaleValue );							// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>
	void AddScaled( Mat44V_InOut outMat, Mat44V_In toAdd, Mat44V_In toScaleThenAdd, Mat44V_In scaleValue );							// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>
	void AddScaled( Mat44V_InOut outMat, Mat44V_In toAdd, Mat44V_In toScaleThenAdd, Vec4V_In scaleValue );							// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>

	void AddScaled3x3( Mat34V_InOut outMat, Mat34V_In toAdd, Mat34V_In toScaleThenAdd, Mat34V_In toScaleBy ); 						// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>
	void AddScaled3x3( Mat34V_InOut outMat, Mat34V_In toAdd, Mat34V_In toScaleThenAdd, Vec3V_In toScaleBy ); 						// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>
	void AddScaled4x3( Mat44V_InOut outMat, Mat44V_In toAdd, Mat44V_In toScaleThenAdd, Mat44V_In toScaleBy ); 						// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>
	void AddScaled4x3( Mat44V_InOut outMat, Mat44V_In toAdd, Mat44V_In toScaleThenAdd, Vec4V_In toScaleBy ); 						// <COMBINE rage::AddScaled@Mat33V_InOut@Mat33V_In@Mat33V_In@Mat33V_In>


	// PURPOSE: Translates a transformation matrix - i.e. it adds a translation vector to column 3.
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		inMat - The source matrix
	//		translateAmt - The amount to translate by
	// NOTES: The result is [inMat[0], inMat[1], inMat[2], inMat[3] + translateAmt]
	void Translate( Mat34V_InOut outMat, Mat34V_In inMat, Vec3V_In translateAmt );

	//@@rage::Transpose
	//<TOCTITLE Transpose Functions (Normal\, 3x3)>
	// PURPOSE: Transposes a matrix (swaps rows and columns)
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		a - The source matrix
	// NOTES:
	//	The 3x3 variant only modifies the left 3x3 cells. Column 3 of outMat will be copied from 'a'
	void Transpose( Mat33V_InOut outMat, Mat33V_In a );
	void Transpose( Mat44V_InOut outMat, Mat44V_In a );
	void Transpose3x3( Mat34V_InOut outMat, Mat34V_In a );					// <COMBINE rage::Transpose>

	void Transpose3x3(
		Vec3V_InOut dst0,
		Vec3V_InOut dst1,
		Vec3V_InOut dst2,
		Vec3V_In    src0,
		Vec3V_In    src1,
		Vec3V_In    src2
	);
	void Transpose3x4to4x3(
		Vec4V_InOut dst0,
		Vec4V_InOut dst1,
		Vec4V_InOut dst2,
		Vec3V_In    src0,
		Vec3V_In    src1,
		Vec3V_In    src2,
		Vec3V_In    src3
	);
	void Transpose4x3to3x4(
		Vec3V_InOut dst0,
		Vec3V_InOut dst1,
		Vec3V_InOut dst2,
		Vec3V_InOut dst3,
		Vec4V_In    src0,
		Vec4V_In    src1,
		Vec4V_In    src2
	);
	void Transpose4x4(
		Vec4V_InOut dst0,
		Vec4V_InOut dst1,
		Vec4V_InOut dst2,
		Vec4V_InOut dst3,
		Vec4V_In    src0,
		Vec4V_In    src1,
		Vec4V_In    src2,
		Vec4V_In    src3
	);

	//@@rage::InvertFull
	//<TOCTITLE InvertFull Functions (Normal\, 3x3\, 3x4)>
	// PURPOSE: Computes the inverse of a matrix.
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		a - The source matrix
	// NOTES:
	//		The 3x3 variant will only modify the left 3x3. Column 3 of outMat will be copied from 'a'
	//		The 3x4 variant will only modify the upper 3x4. The last row will be set to (0,0,0,1)
	void InvertFull( Mat44V_InOut outMat, Mat44V_In a );
	void InvertFull( Mat33V_InOut outMat, Mat33V_In a );
	void Invert3x3Full( Mat34V_InOut outMat, Mat34V_In a );						// <COMBINE rage::InvertFull>
	void Invert3x4Full( Mat44V_InOut outMat, Mat44V_In a );						// <COMBINE rage::InvertFull>

	//@@rage::InvertOrtho
	//<TOCTITLE InverOrtho Functions (Normal\, 3x3\, 3x4)>
	// PURPOSE: Inverts an orthonormal matrix
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		a - The source matrix
	// NOTES:
	//		The matrix is assumed to be orthonormal - that is each column is unit length, and orthogonal to the other columns.
	//		Given those restrictions, InvertOrtho is the same as Transpose.
	//		The 3x3 variant will only modify the left 3x3. Column 3 of outMat will be copied from 'a'
	//		The 3x4 variant will only modify the upper 3x4. The last row will be set to (0,0,0,1). Same as InvertTransformOrtho
	void InvertOrtho( Mat33V_InOut outMat, Mat33V_In a );
	void Invert3x3Ortho( Mat34V_InOut outMat, Mat34V_In a );
	void Invert3x4Ortho( Mat44V_InOut outMat, Mat44V_In a );

	// PURPOSE: Inverts a transform matrix
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		a - The source matrix
	// NOTES:
	//		This assumes the matrix is a transformation matrix, a 3x3 rotation/scale/skew component and a translation vector.
	void InvertTransformFull( Mat34V_InOut outMat, Mat34V_In a );

	// PURPOSE: Inverts an orthonormal transform matrix
	// PARAMS:
	//		outMat - Matrix that will contain the result
	//		a - The source matrix
	// NOTES:
	//		This assumes the matrix is a transformation matrix with a 3x3 rotation component and a translation vector.
	void InvertTransformOrtho( Mat34V_InOut outMat, Mat34V_In a );

	//@@rage::Multiply
	// PURPOSE: Multiplies two matrices, or a matrix and a vector.
	// PARAMS:
	//		outMat - Matrix that will contain the result, if the multiplication results in a 2d matrix
	//		a - The first matrix (or vector) to multiply
	//		b - The second matrix (or vector) to multiply
	// NOTES:
	//		These follow standard textbook matrix multiplication conventions.
	//		The vectors are treated as either row vectors (if they are on the left) or as 
	//		column vectors (if they are on the right).
	void Multiply( Mat44V_InOut outMat, Mat44V_In a, Mat44V_In b );
	void Multiply( Mat33V_InOut outMat, Mat33V_In a, Mat33V_In b );
	FASTRETURNCHECK(Vec4V_Out) Multiply( Mat44V_In a, Vec4V_In b );
	FASTRETURNCHECK(Vec4V_Out) Multiply( Vec4V_In a, Mat44V_In b );
	FASTRETURNCHECK(Vec3V_Out) Multiply( Mat33V_In a, Vec3V_In b );
	FASTRETURNCHECK(Vec3V_Out) Multiply( Vec3V_In a, Mat33V_In b );
	FASTRETURNCHECK(Vec3V_Out) Multiply( Mat34V_In a, Vec4V_In b );
	FASTRETURNCHECK(Vec4V_Out) Multiply( Vec3V_In a, Mat34V_In b );

	// PURPOSE: Conversion between matrices and transforms
	void TransformVFromMat34V( TransformV_InOut outT, Mat34V_In inMat );
	void Mat34VFromTransformV( Mat34V_InOut outMat, TransformV_In inT );

	// PURPOSE: Concatenate multiple transforms
	void InvertTransform( TransformV_InOut outT, TransformV_In intT );
	void Transform( TransformV_InOut outT, TransformV_In inT1, TransformV_In inT2 );
	void UnTransform( TransformV_InOut outT, TransformV_In inT1, TransformV_In inT2 );
	void Transform( QuatV_InOut outQ, Vec3V_InOut outT, QuatV_In inQ1, Vec3V_In inT1, QuatV_In inQ2, Vec3V_In inT2);
	void UnTransform( QuatV_InOut outQ, Vec3V_InOut outT, QuatV_In inQ1, Vec3V_In inT1, QuatV_In inQ2, Vec3V_In inT2);

	// PURPOSE: Apply a transform to a quaternion or vector
	FASTRETURNCHECK(QuatV_Out) Transform( TransformV_In inT, QuatV_In inQ );
	FASTRETURNCHECK(QuatV_Out) UnTransform( TransformV_In inT, QuatV_In inQ );
	FASTRETURNCHECK(QuatV_Out) UnTransform( QuatV_In inQToInvert, QuatV_In inQ );
	FASTRETURNCHECK(Vec3V_Out) Transform( TransformV_In inT, Vec3V_In inV );
	FASTRETURNCHECK(Vec3V_Out) UnTransform( TransformV_In inT, Vec3V_In inV );
	FASTRETURNCHECK(Vec3V_Out) Transform3x3( TransformV_In inT, Vec3V_In inV );
	FASTRETURNCHECK(Vec3V_Out) UnTransform3x3( TransformV_In inT, Vec3V_In inV );
	FASTRETURNCHECK(Vec3V_Out) Transform( QuatV_In inQ, Vec3V_In inT, Vec3V_In inV );
	FASTRETURNCHECK(Vec3V_Out) UnTransform( QuatV_In inQ, Vec3V_In inT, Vec3V_In inV );

	// PURPOSE: Multiples two transform matrices: inoutTransformMat = inoutTransformMat * mat
	// NOTES: The matrices are assumed to be transformation matrices, 3x4 matrices where the left 3x3 contains the 
	//		rotation/scale/skew components and the right column is a translation.
	void		Transform( Mat34V_InOut inoutTransformMat, Mat34V_In mat );

	// PURPOSE: Multiplies two transform matrices: outMat = transformMat * mat
	// NOTES: The matrices are assumed to be transformation matrices, 3x4 matrices where the left 3x3 contains the 
	//		rotation/scale/skew components and the right column is a translation.
	void		Transform( Mat34V_InOut outMat, Mat34V_In transformMat, Mat34V_In mat );

	// PURPOSE: Multiplies two transform matrices: outMat = transformMat * mat
	// NOTES: The matrices are assumed to be transformation matrices, 3x4 matrices where the left 3x3 contains the 
	//		rotation/scale/skew components and the right column is a translation.
	//		The translation column in outMat will be copied from transformMat
	void		Transform3x3( Mat34V_InOut outMat, Mat34V_In transformMat, Mat34V_In mat );

	// PURPOSE: Transforms a vector using a transform matrix (excluding the translation component)
	FASTRETURNCHECK(Vec3V_Out)	Transform3x3( Mat34V_In transformMat, Vec3V_In vec );

	// PURPOSE: Transforms a vector using a transform matrix
	FASTRETURNCHECK(Vec3V_Out)	Transform( Mat34V_In transformMat, Vec3V_In point );

	//@@rage::UnTransformFull
	//<TOCTITLE UnTransformFull Functions (Normal\, 3x3)>
	// PURPOSE: Undoes the effect of a transformation on a matrix (i.e. multiplies by the inverse of a matrix)
	// PARAMS:
	//		outMat - Matrix which will contain the result value
	//		matToUntransformBy - The transformation to "remove"
	//		mat - The "transformed" matrix
	//		vec - The "transformed" vector
	// RETURNS:
	//		The untransformed vector. I.e. if you had matB = Transform(xform, matA), then UnTransformFull(xform, matB) = matA.
	//		I.e. UnTransformFull(xform, matB) = xform^-1 * B
	//		The 3x3 variant only untransforms by the left 3x3 of the input matrix. outMat's translation column is copied from mat.
	void		UnTransformFull( Mat44V_InOut outMat, Mat44V_In matToUntransformBy, Mat44V_In mat );
	FASTRETURNCHECK(Vec4V_Out)	UnTransformFull( Mat44V_In matToUntransformBy, Vec4V_In vec ); 
	void		UnTransformFull( Mat33V_InOut outMat, Mat33V_In matToUntransformBy, Mat33V_In mat );
	FASTRETURNCHECK(Vec3V_Out)	UnTransformFull( Mat33V_In matToUntransformBy, Vec3V_In vec );
	void		UnTransformFull( Mat34V_InOut outMat, Mat34V_In matToUntransformBy, Mat34V_In mat );
	FASTRETURNCHECK(Vec3V_Out)	UnTransformFull( Mat34V_In matToUntransformBy, Vec3V_In point );
	void		UnTransform3x3Full( Mat34V_InOut outMat, Mat34V_In matToUntransformBy, Mat34V_In mat );					// <COMBINE rage::UnTransformFull>
	FASTRETURNCHECK(Vec3V_Out)	UnTransform3x3Full( Mat34V_In matToUntransformBy, Vec3V_In vec );										// <COMBINE rage::UnTransformFull>

	//@@rage::UnTransformOrtho
	//<TOCTITLE UnTransformOrtho Functions (Normal\, 3x3)>
	// PURPOSE: Undoes the effect of an orthonormal transformation on a matrix (i.e. multiplies by the inverse of a matrix).
	// PARAMS:
	//		outMat - Matrix which will contain the result value
	//		orthoMatToUntransformBy - The orthonormal transformation to "remove"
	//		mat - The "transformed" matrix
	//		vec - The "transformed" vector
	// NOTES:
	//		This function is faster than UnTransformFull, if you know your transformation matrix is orthonormal. Especially on xenon.
	//		The 3x3 variant only untransforms by the left 3x3 of the input matrix. outMat's translation column is copied from mat.
	void		UnTransformOrtho( Mat44V_InOut outMat, Mat44V_In orthoMatToUntransformBy, Mat44V_In mat ); 
	FASTRETURNCHECK(Vec4V_Out)	UnTransformOrtho( Mat44V_In orthoMatToUntransformBy, Vec4V_In vec );
	void		UnTransformOrtho( Mat33V_InOut outMat, Mat33V_In orthoMatToUntransformBy, Mat33V_In mat );
	FASTRETURNCHECK(Vec3V_Out)	UnTransformOrtho( Mat33V_In orthoMatToUntransformBy, Vec3V_In vec );
	void		UnTransformOrtho( Mat34V_InOut outMat, Mat34V_In orthoMatToUntransformBy, Mat34V_In mat );
	FASTRETURNCHECK(Vec3V_Out)	UnTransformOrtho( Mat34V_In orthoMatToUntransformBy, Vec3V_In point );
	void		UnTransform3x3Ortho( Mat34V_InOut outMat, Mat34V_In orthoMatToUntransformBy, Mat34V_In mat );			// <COMBINE rage::UnTransform3x3Ortho>
	FASTRETURNCHECK(Vec3V_Out)	UnTransform3x3Ortho( Mat34V_In orthoMatToUntransformBy, Vec3V_In vec );									// <COMBINE rage::UnTransform3x3Ortho>

	//@@rage::ReOrthonormalize
	//<TOCTITLE ReOrthonormalize Functions (Normal\, 3x3)>
	// PURPOSE: Ensures the first three columns in a matrix are mutually orthogonal and unit length.
	// PARAMS:
	//		outMat - Matrix which will contain the result value
	//		intMat - Source matrix.
	// NOTES:
	//		The 3x3 variant only re-normalizes the first three columns, it leaves the fourth untouched. If the input is a Mat44V, the 
	//		final three rows of the re-normalized columns will be set to 0.0f
	void	ReOrthonormalize( Mat33V_InOut outMat, Mat33V_In inMat );
	void	ReOrthonormalize3x3( Mat34V_InOut outMat, Mat34V_In inMat );
	void	ReOrthonormalize3x3( Mat44V_InOut outMat, Mat44V_In inMat );

	// Completely initializes outMat based on inputs
	void	LookAt(Mat34V_InOut outMat, Vec3V_In from, Vec3V_In to, Vec3V_In up = Vec3V(V_UP_AXIS_WZERO));

	// Initializes only the directional components of outMat based on the inputs.
	void	LookDown(Mat34V_InOut outMat, Vec3V_In dir, Vec3V_In up);

	//============================================================================
	// Standard quaternion math

	// PURPOSE: Transform (rotate) a vector by a quaternion
	// PARAMS:
	//		unitQuat - The (normalized) quaternion to rotate by
	//		inVect - The vector to rotate
	// NOTES:
	//		Computes Q * V * Q^-1 (using quaternion multiplication)
	FASTRETURNCHECK(Vec3V_Out)	Transform( QuatV_In unitQuat, Vec3V_In inVect );

	// PURPOSE: Untransform (unrotate) a vector by a quaternion
	// PARAMS:
	//		unitQuat - The (normalized) quaternion to rotate by
	//		inVect - The transformed vector to untransform
	// NOTES:
	//		Computes Q^-1 * V * Q  (using quaternion multiplication) 
	FASTRETURNCHECK(Vec3V_Out)	UnTransformFull( QuatV_In unitQuat, Vec3V_In inVect );

	// PURPOSE: Computes the conjugate of a quaternion (negates imaginary components x,y,z)
	FASTRETURNCHECK(QuatV_Out) Conjugate(QuatV_In inQuat);

	//<TOCTITLE Normalize (QuatV) Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Normalizes the quaterion
	// PARAMS:
	//		inQuat - The input quaternion to normalize
	//		errValVect - for 'Safe' variants, the value to return if |inQuat| is near 0
	//		magSqThreshold - for 'Safe' variants, the magnitude below which errValVect gets returned
	// RETURNS: inQuat / |inQuat| - a unit length quaternion
	// NOTES:
	//		The Safe variant returns the errValVect value when |inQuat|^2 < magSqThreshold
	//		The Fast variant is faster but less precise
	FASTRETURNCHECK(QuatV_Out) Normalize(QuatV_In inQuat);
	FASTRETURNCHECK(QuatV_Out) NormalizeSafe(QuatV_In inQuat, QuatV_In errValVect, QuatV_In magSqThreshold = QuatV(V_FLT_SMALL_5));		// <COMBINE rage::Normalize@QuatV_In>
	FASTRETURNCHECK(QuatV_Out) NormalizeFast(QuatV_In inQuat);																											// <COMBINE rage::Normalize@QuatV_In>	
	FASTRETURNCHECK(QuatV_Out) NormalizeFastSafe(QuatV_In inQuat, QuatV_In errValVect, QuatV_In magSqThreshold = QuatV(V_FLT_SMALL_5)); // <COMBINE rage::Normalize@QuatV_In>

	//<TOCTITLE Invert (QuatV) Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Inverts the quaternion - returns a quaternion that represents the inverse rotation
	// PARAMS:
	//		inQuat - The input quaternion to invert
	//		errValVect - for 'Safe' variants, the value to return if |inQuat| is near 0
	// NOTES:
	//		An inverted quaternion accomplishes the opposite rotation of the 
	//		original, i.e. it's like an inverted matrix.
	//		The Safe variant returns the errValVect value when |inQuat|^2 < magSqThreshold
	FASTRETURNCHECK(QuatV_Out) Invert(QuatV_In inQuat);
	FASTRETURNCHECK(QuatV_Out) InvertSafe(QuatV_In inQuat, QuatV_In errValVect = QuatV(V_FLT_LARGE_8));
	FASTRETURNCHECK(QuatV_Out) InvertFast(QuatV_In inQuat);
	FASTRETURNCHECK(QuatV_Out) InvertFastSafe(QuatV_In inQuat, QuatV_In errValVect = QuatV(V_FLT_LARGE_8));

	// PURPOSE: Inverts a normalized quaternion - faster than Invert() if the input is already normalized
	// NOTES:
	//	InvertNormInput() is fastest if the input is already a unit quat. Else, Invert() is faster
	//	than a Normalize() followed by a InvertNormInput().
	FASTRETURNCHECK(QuatV_Out) InvertNormInput(QuatV_In inQuat);

	// PURPOSE: Calculate the inner product (i.e. dot product) of two quaternions.
	// RETURN: The sum of the elements of inQuat1 multiplied by corresponding elements of inQuat2 (same as Dot for Vec4V)
	// NOTES:
	//   - The meaning of the dot product of two quaternions is very similar 
	//     to the meaning of the dot product of two vectors; it represents the 
	//     cosine of 1/2 the angle between the two rotations.
	//   - Apparently, how to multiply two quaternions together, and the meaning 
	//     of such an operation, were hotly debated topics back in the nineteenth century.
	FASTRETURNCHECK(ScalarV_Out) Dot( QuatV_In inQuat1, QuatV_In inQuat2 );

	// PURPOSE: Multiply this quaternion by another quaternion, producing a combined rotation.
	// NOTES: If both quaternions represent rotations, this transforms the second quaternion from the first quaternion's coordinates into world coordinates.
	FASTRETURNCHECK(QuatV_Out) Multiply( QuatV_In inQuat1, QuatV_In inQuat2 );

	// PURPOSE: Prepare two quaternions for a slerp between them by making sure the angle between them is between -PI and PI.
	// RETURNS: quatToNegate or -quatToNegate, depending on the angle between it an quat1
	// NOTES
	//	This method makes sure that the interpolation takes the shortest route. If the angle between the two quaternions is not between
	//	-PI and PI, then quatToNegate is negated to make the angle between -PI and PI. The negated quaternion represents a rotation
	//	in the opposite direction about an opposite unit vector, so it is equivalent to the non-negated quaternion.
	FASTRETURNCHECK(QuatV_Out) PrepareSlerp( QuatV_In quat1, QuatV_In quatToNegate );

	// PURPOSE: Spherical interpolation between two input quaternions using a t value.
	// PARAMS:
	//		t - A value from 0 to 1, for how far between inNormQuat1 and inNormQuat2 to interpolate
	//		inNormQuat1 - The 'start' quaternion - returned if t=0
	//		inNormQuat2 - The 'end' quaternion - returned if t=1
	// RETURNS: The interpolated quaternion value
	// NOTES:
	//		This is faster than Slerp() but you must use PrepareSlerp or otherwise ensure the two rotations lie in the same half-sphere of S^3 before calling
	//		or the returned values won't use the shortest rotation
	FASTRETURNCHECK(QuatV_Out) SlerpNear( ScalarV_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 );
	FASTRETURNCHECK(QuatV_Out) SlerpNear( Vec4V_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 );

	// PURPOSE: Spherical interpolation between two input quaternions using a t value.
	// PARAMS:
	//		t - A value from 0 to 1, for how far between inNormQuat1 and inNormQuat2 to interpolate
	//		inNormQuat1 - The 'start' quaternion - returned if t=0
	//		inNormQuat2 - The 'end' quaternion - returned if t=1
	// RETURNS: The interpolated quaternion value
	// NOTES: There can be numerical inaccuracy between this and the old vector library's SLERP if you do not precede this call with a PrepareSlerp()!
	// Be warned! (See the commented-out unit test in test_quatv.cpp for the failing compatibility test.]
	FASTRETURNCHECK(QuatV_Out) Slerp( ScalarV_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 );
	FASTRETURNCHECK(QuatV_Out) Slerp( Vec4V_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 );

	// PURPOSE: Normalized linear interpolation between two input quaternions
	// PARAMS:
	//		t - A value from 0 to 1, for how far between inNormQuat1 and inNormQuat2 to interpolate
	//		inNormQuat1 - The 'start' quaternion - returned if t=0
	//		inNormQuat2 - The 'end' quaternion - returned if t=1
	// RETURNS: The interpolated quaternion value
	// NOTES: 
	//	Faster than Slerp, but a non-constant velocity. Still fine if you have a substantial amount of animation keyframes.
	FASTRETURNCHECK(QuatV_Out) Nlerp( ScalarV_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 );
	FASTRETURNCHECK(QuatV_Out) Nlerp( Vec4V_In t, QuatV_In inNormQuat1, QuatV_In inNormQuat2 );

	//@@rage::GetUnitDirection
	//<TOCTITLE GetUnitDirection Functions (Normal\, Fast\, Safe\, FastSafe)>
	// PURPOSE: Given an input quaternion representing a rotation, returns the axis of rotation
	// PARAMS:
	//		inQuat - The input quaternion
	//		errValVec - The value to return if the 'axis' would otherwise be near 0
	// NOTES:
	//		The Safe variant checks for division by zero, and returns the errValVect if that would be the case.
	//		The Fast variant is faster but less precise.
	FASTRETURNCHECK(Vec3V_Out) GetUnitDirection( QuatV_In inQuat );
	FASTRETURNCHECK(Vec3V_Out) GetUnitDirectionFast( QuatV_In inQuat );															// <COMBINE rage::GetUnitDirection>
	FASTRETURNCHECK(Vec3V_Out) GetUnitDirectionSafe( QuatV_In inQuat, Vec3V_In errValVec = Vec3V(V_Y_AXIS_WZERO) );			// <COMBINE rage::GetUnitDirection>
	FASTRETURNCHECK(Vec3V_Out) GetUnitDirectionFastSafe( QuatV_In inQuat, Vec3V_In errValVec = Vec3V(V_Y_AXIS_WZERO) );		// <COMBINE rage::GetUnitDirection>

	// PURPOSE: Given a quaternion rotation, computes the angle of rotation about the axis returned by GetUnitDirection or ToAxisAngle
	FASTRETURNCHECK(ScalarV_Out) GetAngle( QuatV_In inQuat );

	// PURPOSE: Preserves the axis of rotation, but scales the angle
	// NOTES: For example: QuatVScaleAngle(q, ScalarV(3.0f)) is equivalent to q * q * q
	FASTRETURNCHECK(QuatV_Out) QuatVScaleAngle( QuatV_In inQuat, ScalarV_In scale );

	//@@rage::QuatVTwistAngle
	// PURPOSE: Computes the amount of twist about the specified axis
	FASTRETURNCHECK(ScalarV_Out) QuatVTwistAngle( QuatV_In inQuat, Vec4V_In axis );
	FASTRETURNCHECK(ScalarV_Out) QuatVTwistAngle( QuatV_In inQuat, Vec3V_In axis );

	//============================================================================
	// Conversion functions

	// PURPOSE: Create a quaternion that represents the rotation between two vectors.
	// PARAMS
	//   from - The starting vector
	//   to - The ending vector
	// NOTES
	//   - This method creates a rotation about an axis perpendicular to the
	//     starting and ending vectors.
	//   - This method returns the identity quaternion if the vectors are co-linear
	//     in the same direction and a 180 degree rotation around an arbitrary
	//     perpendicular axis when the vectors are co-linear in opposite directions.
	FASTRETURNCHECK(QuatV_Out) QuatVFromVectors( Vec3V_In from, Vec3V_In to );

	// PURPOSE: Create a quaternion that represents the rotation between two vectors
	//          about the given axis.
	// PARAMS
	//   from - The starting vector
	//   to - The ending vector
	//   axis - The axis of rotation
	FASTRETURNCHECK(QuatV_Out) QuatVFromVectors( Vec3V_In from, Vec3V_In to, Vec3V_In axis );

	// PURPOSE: Given a quaternion rotation, compute the axis and angle of rotation
	// PARAMS:
	//		outAxis - A vector that will contain the axis result
	//		outRadians - A scalar that will contain the angle result
	//		inQuat - The input quaternion
	void QuatVToAxisAngle( Vec3V_InOut outAxis, ScalarV_InOut outRadians, QuatV_In inQuat );

	//@@rage::QuatVFromAxisAngle
	//<TOCTITLE QuatVFromAxisAngle Functions (Normal\, X\, Y\, Z)>
	// PURPOSE: Creates a quaternion from an axis of rotation and an angle
	// PARAMS:
	//		normAxis - The (normalized) axis vector. 
	//		radians - The angle of rotation about the named axis or the normAxis parameter
	// NOTES:
	//		The X, Y and Z variants rotate about the corresponding world axis
	FASTRETURNCHECK(QuatV_Out) QuatVFromAxisAngle( Vec3V_In normAxis, ScalarV_In radians );
	FASTRETURNCHECK(QuatV_Out) QuatVFromXAxisAngle( ScalarV_In radians );									// <COMBINE rage::QuatVFromAxisAngle>
	FASTRETURNCHECK(QuatV_Out) QuatVFromYAxisAngle( ScalarV_In radians );									// <COMBINE rage::QuatVFromAxisAngle>
	FASTRETURNCHECK(QuatV_Out) QuatVFromZAxisAngle( ScalarV_In radians );									// <COMBINE rage::QuatVFromAxisAngle>

	//@@rage::QuatVFromMatNNV
	//<TOCTITLE QuatVFromMatNNV Functions (Mat33V\, Mat34V)>
	// PURPOSE: Extract the rotation components from a Mat33V and return the rotation as a quaternion
	// NOTES: Andy *may* have noticed some unstable behavior in this function....beware
	FASTRETURNCHECK(QuatV_Out) QuatVFromMat33V( Mat33V_In mat );												// <COMBINE rage::QuatVFromMatNNV>
	FASTRETURNCHECK(QuatV_Out) QuatVFromMat34V( Mat34V_In mat );												// <COMBINE rage::QuatVFromMatNNV>

	//@@rage::QuatVFromMat33VSafe
	//<TOCTITLE QuatVFromMat33VSafe (Mat33V)>
	// PARAMS:
	//		mat - The input matrix
	//		errVal - Error value
	// PURPOSE: Extract the normalized quaternion from a scaled matrix. If matrix is zero, return error value.
	FASTRETURNCHECK(QuatV_Out) QuatVFromMat33VSafe(Mat33V_In mat, QuatV_In errVal);

	enum EulerAngleOrder
	{
		EULER_XYZ,
		EULER_XZY,
		EULER_YXZ,
		EULER_YZX,
		EULER_ZXY,
		EULER_ZYX
	};

	//@@rage::QuatVFromEulers
	//<TOCTITLE QuatVFromEulers Functions (Normal\, Fast\, XYZ\, ZYX\, YXZ\, etc.)>
	// PURPOSE: Creates a quaterion based on a set of euler angles and a specified rotation order
	// PARAMS:
	//		radianAngles - The amount of x-axis, y-axis, and z-axis rotation, in radians
	//		order - If specified, the order to apply the rotations in (otherwise the order is given by the function name)
	// RETURNS:
	//		A quaternion built from the specified sequence of rotations about the world axes.
	// NOTES:
	//		The axis order specifies the order that the world-space rotations are performed in.
	//		so QuatVFromEulersXYZ(r) should give the same results as QuatVFromZAxisAngle(r.GetZ()) * ( QuatVFromYAxisAngle(r.GetY()) * QuatVFromXAxisAngle(r.GetX()) )
	//		I.e. perform a rotation about world-X first, then world-Y, then world-Z.
	//		The Fast variants are faster but less precise
	FASTRETURNCHECK(QuatV_Out) QuatVFromEulersXYZ( Vec3V_In radianAngles );									// <COMBINE rage::QuatVFromEulers>
	FASTRETURNCHECK(QuatV_Out) QuatVFromEulersXZY( Vec3V_In radianAngles );									// <COMBINE rage::QuatVFromEulers>
	FASTRETURNCHECK(QuatV_Out) QuatVFromEulersYXZ( Vec3V_In radianAngles );									// <COMBINE rage::QuatVFromEulers>
	FASTRETURNCHECK(QuatV_Out) QuatVFromEulersYZX( Vec3V_In radianAngles );									// <COMBINE rage::QuatVFromEulers>
	FASTRETURNCHECK(QuatV_Out) QuatVFromEulersZYX( Vec3V_In radianAngles );									// <COMBINE rage::QuatVFromEulers>
	FASTRETURNCHECK(QuatV_Out) QuatVFromEulersZXY( Vec3V_In radianAngles );									// <COMBINE rage::QuatVFromEulers>
	FASTRETURNCHECK(QuatV_Out) QuatVFromEulers( Vec3V_In radianAngles, EulerAngleOrder order );				// <COMBINE rage::QuatVFromEulers>

	//@@rage::QuatVToEulers
	//<TOCTITLE QuatVToEulers Functions (Normal\, Fast\, XYZ\, ZYX\, YXZ\, etc.)>
	// PURPOSE: Creates a vector of euler angle rotations based on a quaternion and an euler rotation order
	// PARAMS:
	//		quatIn - The original rotation
	//		order - If specified, the order to apply the euler rotations in (otherwise the order is given by the function name)
	// RETURNS:
	//		A vector of euler angles (in radians) that, using the specified rotation order, represent the same rotation as the quaternion.
	// NOTES:
	//		The axis order specifies the order that the world-space rotations are performed in to get the same rotation back.
	//		So QuatVToEulersXYZ() returns euler angles (x,y,z) such that if you first rotate about the world X axis by x radians, then world-Y by y radians, then 
	//		world-z by z radians you get the original rotation back.
	//		The Fast variants are faster but less precise
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersXYZ( QuatV_In quatIn );											// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersXZY( QuatV_In quatIn );											// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersYXZ( QuatV_In quatIn );											// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersYZX( QuatV_In quatIn );											// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersZXY( QuatV_In quatIn );											// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersZYX( QuatV_In quatIn );											// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulers( QuatV_In quatIn, EulerAngleOrder order );						// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersXYZFast( QuatV_In quatIn );										// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersXZYFast( QuatV_In quatIn );										// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersYXZFast( QuatV_In quatIn );										// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersYZXFast( QuatV_In quatIn );										// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersZXYFast( QuatV_In quatIn );										// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersZYXFast( QuatV_In quatIn );										// <COMBINE rage::QuatVToEulers>
	FASTRETURNCHECK(Vec3V_Out) QuatVToEulersFast( QuatV_In quatIn, EulerAngleOrder order );					// <COMBINE rage::QuatVToEulers>


	//@@rage::MatNNVFromAxisAngle
	//<TOCTITLE MatNNVFromAxisAngle Functions (Mat33V\, Mat34V\, Mat44V) (Normal\, X\, Y\, Z)>
	// PURPOSE: Creates a matrix from an axis of rotation and an angle
	// PARAMS:
	//		outMat - Matrix which will contain the result value
	//		normAxis - The (normalized) axis vector. 
	//		radians - The angle of rotation about the named axis or the normAxis parameter
	//		translation - The translation (column 3) for the result matrix - for Mat34V and Mat44V
	// NOTES:
	//		The X, Y and Z variants rotate about the corresponding world axis
	//		For Mat44V, M30,M31,M32 (the first 3 elements of the last row) are set to 0.0f.
	void Mat33VFromAxisAngle( Mat33V_InOut outMat, Vec3V_In normAxis, ScalarV_In radians );													// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat33VFromXAxisAngle( Mat33V_InOut outMat, ScalarV_In radians );																	// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat33VFromYAxisAngle( Mat33V_InOut outMat, ScalarV_In radians );																	// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat33VFromZAxisAngle( Mat33V_InOut outMat, ScalarV_In radians );																	// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat34VFromAxisAngle( Mat34V_InOut outMat, Vec3V_In normAxis, ScalarV_In radians, Vec3V_In translation = Vec3V(V_ZERO) );	// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat34VFromXAxisAngle( Mat34V_InOut outMat, ScalarV_In radians, Vec3V_In translation = Vec3V(V_ZERO) );						// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat34VFromYAxisAngle( Mat34V_InOut outMat, ScalarV_In radians, Vec3V_In translation = Vec3V(V_ZERO) );						// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat34VFromZAxisAngle( Mat34V_InOut outMat, ScalarV_In radians, Vec3V_In translation = Vec3V(V_ZERO) );						// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat44VFromAxisAngle( Mat44V_InOut outMat, Vec3V_In normAxis, ScalarV_In radians, Vec4V_In translation = Vec4V(V_ZERO_WONE) );	// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat44VFromXAxisAngle( Mat44V_InOut outMat, ScalarV_In radians, Vec4V_In translation = Vec4V(V_ZERO_WONE) );					// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat44VFromYAxisAngle( Mat44V_InOut outMat, ScalarV_In radians, Vec4V_In translation = Vec4V(V_ZERO_WONE) );					// <COMBINE rage::MatNNVFromAxisAngle>
	void Mat44VFromZAxisAngle( Mat44V_InOut outMat, ScalarV_In radians, Vec4V_In translation = Vec4V(V_ZERO_WONE) );					// <COMBINE rage::MatNNVFromAxisAngle>

	//@@rage::MatNNVFromQuatV
	//<TOCTITLE MatNNVFromQuatV Functions (Mat33V\, Mat34V\, Mat44V)>
	// PURPOSE: Constructs a rotation matrix from a quaternion
	// PARAMS:
	//		outMat - Matrix which will contain the result value
	//		inQuat - The normalized rotation quaternion
	//		translation - The translation (column 3) for the result matrix - for Mat34V and Mat44V
	// NOTES:
	//		For Mat44V, M30,M31,M32 (the first 3 elements of the last row) are set to 0.0f.
	void Mat33VFromQuatV( Mat33V_InOut outMat, QuatV_In inQuat );																			// <COMBINE rage::MatNNVFromQuatV>
	void Mat34VFromQuatV( Mat34V_InOut outMat, QuatV_In inQuat, Vec3V_In translation = Vec3V(V_ZERO) );								// <COMBINE rage::MatNNVFromQuatV>
	void Mat44VFromQuatV( Mat44V_InOut outMat, QuatV_In inQuat, Vec4V_In translation = Vec4V(V_ZERO_WONE) );							// <COMBINE rage::MatNNVFromQuatV>

	//@@rage::MatNNVFromEulers
	//<TOCTITLE MatNNVFromEulers Functions (Mat33V\, Mat34V\, Mat44V) (XYZ\, ZYX)>
	// PURPOSE: Constructs a matrix from a set of euler angle rotations
	// PARAMS:
	//		outMat - Matrix which will contain the result value
	//		radianAngles - The amount of x-axis, y-axis, and z-axis rotation, in radians
	//		translation - The translation (column 3) for the result matrix - for Mat34V and Mat44V
	// NOTES:
	//		The function name specifies the rotation order. XYZ means rotate first about the world X axis, then the world Y axis, then the world Z.
	//		For Mat44V, M30,M31,M32 (the first 3 elements of the last row) are set to 0.0f.
	void Mat33VFromEulersXYZ( Mat33V_InOut outMat, Vec3V_In radianAngles );																	// <COMBINE rage::MatNNVFromEulers>
	void Mat33VFromEulersXZY( Mat33V_InOut outMat, Vec3V_In radianAngles );																	// <COMBINE rage::MatNNVFromEulers>	
	void Mat33VFromEulersYXZ( Mat33V_InOut outMat, Vec3V_In radianAngles );																	// <COMBINE rage::MatNNVFromEulers>
	void Mat33VFromEulersYZX( Mat33V_InOut outMat, Vec3V_In radianAngles );																	// <COMBINE rage::MatNNVFromEulers>	
	void Mat33VFromEulersZXY( Mat33V_InOut outMat, Vec3V_In radianAngles );																	// <COMBINE rage::MatNNVFromEulers>	
	void Mat33VFromEulersZYX( Mat33V_InOut outMat, Vec3V_In radianAngles );																	// <COMBINE rage::MatNNVFromEulers>	
	void Mat34VFromEulersXYZ( Mat34V_InOut outMat, Vec3V_In radianAngles, Vec3V_In translation = Vec3V(V_ZERO) );							// <COMBINE rage::MatNNVFromEulers>
	void Mat34VFromEulersXZY( Mat34V_InOut outMat, Vec3V_In radianAngles, Vec3V_In translation = Vec3V(V_ZERO) );							// <COMBINE rage::MatNNVFromEulers>
	void Mat34VFromEulersYXZ( Mat34V_InOut outMat, Vec3V_In radianAngles, Vec3V_In translation = Vec3V(V_ZERO) );							// <COMBINE rage::MatNNVFromEulers>
	void Mat34VFromEulersYZX( Mat34V_InOut outMat, Vec3V_In radianAngles, Vec3V_In translation = Vec3V(V_ZERO) );							// <COMBINE rage::MatNNVFromEulers>
	void Mat34VFromEulersZXY( Mat34V_InOut outMat, Vec3V_In radianAngles, Vec3V_In translation = Vec3V(V_ZERO) );							// <COMBINE rage::MatNNVFromEulers>
	void Mat34VFromEulersZYX( Mat34V_InOut outMat, Vec3V_In radianAngles, Vec3V_In translation = Vec3V(V_ZERO) );							// <COMBINE rage::MatNNVFromEulers>

	//@@rage::MatNNVToEulers
	//<TOCTITLE MatNNVToEulers Functions (XYZ\, XZY\, YXZ\, etc.)>
	// PURPOSE: Extract euler angle rotations from a matrix
	// PARAMS:
	//		inMat - Input matrix
	// RETURNS:
	//		A vector of euler angles (in radians) using the specified rotation order.
	FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersXYZ( Mat33V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersXZY( Mat33V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersYXZ( Mat33V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersYZX( Mat33V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersZXY( Mat33V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat33VToEulersZYX( Mat33V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersXYZ( Mat34V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersXZY( Mat34V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersYXZ( Mat34V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersYZX( Mat34V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersZXY( Mat34V_In inMat );															// <COMBINE rage::MatNNVToEulers>
	FASTRETURNCHECK(Vec3V_Out) Mat34VToEulersZYX( Mat34V_In inMat );															// <COMBINE rage::MatNNVToEulers>

	//@@rage::MatNNVFromTranslation
	//<TOCTITLE MatNNVFromTranslation Functions (Mat34V\, Mat44V)>
	// PURPOSE: Constructs a matrix from a translation
	// PARAMS:
	//		outMat - Matrix which will contain the result value
	//		translation - The translation (column 3) for the result matrix
	// NOTES:
	//		The first 3 columns are set to the 3x3 identity matrix
	//		For Mat44V, M30,M31,M32 (the first 3 elements of the last row) are set to 0.0f.
	void Mat34VFromTranslation( Mat34V_InOut outMat, Vec3V_In translation );								// <COMBINE rage::MatNNVFromTranslation>
	void Mat44VFromTranslation( Mat44V_InOut outMat, Vec4V_In translation );								// <COMBINE rage::MatNNVFromTranslation>

	//@@rage::MatNNVFromScale
	//<TOCTITLE MatNNVFromScale Functions (Mat33V\, Mat34V\, Mat44V)>
	// PURPOSE: Constructs a matrix from a set of scale values
	// PARAMS:
	//		outMat - Matrix which will contain the result value
	//		scaleAmounts - Amounts to scale on the X, Y and Z axes
	//		scaleX - An amount to scale on the X axis
	//		scaleY - An amount to scale on the Y axis
	//		scaleZ - An amount to scale on the Z axis
	//		translation - The translation (column 3) for the result matrix
	// NOTES:
	//		For Mat44V, M30,M31,M32 (the first 3 elements of the last row) are set to 0.0f.
	void Mat33VFromScale( Mat33V_InOut outMat, Vec3V_In scaleAmounts );											// <COMBINE rage::MatNNVFromScale>
	void Mat33VFromScale( Mat33V_InOut outMat, ScalarV_In scaleX, ScalarV_In scaleY, ScalarV_In scaleZ );		// <COMBINE rage::MatNNVFromScale>
	void Mat44VFromScale( Mat44V_InOut inoutMat, Vec3V_In scaleAmounts, Vec4V_In translation = Vec4V(V_ZERO_WONE) );		// <COMBINE rage::MatNNVFromScale>
	void Mat44VFromScale( Mat44V_InOut inoutMat, ScalarV_In scaleX, ScalarV_In scaleY, ScalarV_In scaleZ, Vec4V_In translation = Vec4V(V_ZERO_WONE) );		// <COMBINE rage::MatNNVFromScale>
	void Mat34VFromScale( Mat34V_InOut outMat, Vec3V_In scaleAmounts, Vec3V_In translation = Vec3V(V_ZERO) );		// <COMBINE rage::MatNNVFromScale>
	void Mat34VFromScale( Mat34V_InOut outMat, ScalarV_In scaleX, ScalarV_In scaleY, ScalarV_In scaleZ, Vec3V_In translation = Vec3V(V_ZERO) );		// <COMBINE rage::MatNNVFromScale>

	//@@rage::ScaleFromMat33VTranspose
	//<TOCTITLE ScaleFromMat33VTranspose>
	// PURPOSE: Extracts the transposed scale values from a 3x3 orthogonal matrix
	// PARAMS:
	//		inMat - the orthogonal matrix to find the scale values for
	// NOTES:
	//		This function returns the length of the column vectors of inMat
	//		(i.e. scale values are always positive)
	FASTRETURNCHECK(Vec3V_Out) ScaleFromMat33VTranspose( Mat33V_In inMat );

	//@@rage::ScaleTranspose
	//<TOCTITLE ScaleTranspose>
	// PURPOSE: Scales each column in a matrix by the scalars generated by the vector components
	// PARAMS:
	//		outMat - A matrix that will contain the result
	//		inVect - A vector containing the scale values
	//		inMtx - A matrix whose columns will be scaled
	void ScaleTranspose( Mat33V_InOut outMat, Vec3V_In inScale, Mat33V_In inMat );

	//@@rage::Mat34VRotateLocal
	//<TOCTITLE Mat34VRotateLocal Functions (X\, Y\, Z)>
	// PURPOSE: Rotate a matrix (in place) about one if its local axes.
	// PARAMS:
	//		inoutMat - Input/output matrix which will be rotated
	//		radians - The amount to rotate by
	void Mat34VRotateLocalX( Mat34V_InOut inoutMat, ScalarV_In radians );
	void Mat34VRotateLocalY( Mat34V_InOut inoutMat, ScalarV_In radians );
	void Mat34VRotateLocalZ( Mat34V_InOut inoutMat, ScalarV_In radians );
	void Mat34VRotateGlobalX( Mat34V_InOut inoutMat, ScalarV_In radians );
	void Mat34VRotateGlobalY( Mat34V_InOut inoutMat, ScalarV_In radians );
	void Mat34VRotateGlobalZ( Mat34V_InOut inoutMat, ScalarV_In radians );

	//============================================================================
	// Utility functions

	//@@rage::LoadScalar32IntoScalarV
	// PURPOSE: Efficiently loads a 32-bit value into a ScalarV
	// NOTES:
	//		Uses pure vector operations. Fast if the scalar is in memory (not a register)
	FASTRETURNCHECK(ScalarV_Out)	LoadScalar32IntoScalarV( const float& scalar );
	FASTRETURNCHECK(ScalarV_Out)	LoadScalar32IntoScalarV( const u32& scalar );
	FASTRETURNCHECK(ScalarV_Out)	LoadScalar32IntoScalarV( const s32& scalar );

	//@@rage::StoreScalar32FromScalarV
	// PURPOSE: Efficiently stores a ScalarV into a 32-bit value
	// NOTES:
	//		Uses pure vector operations. Fast if the scalar is in memory (not a register)
	void		StoreScalar32FromScalarV( float& fLoc, ScalarV_In splattedVec );
	void		StoreScalar32FromScalarV( u32& loc, ScalarV_In splattedVec );
	void		StoreScalar32FromScalarV( s32& loc, ScalarV_In splattedVec );


// DOM-IGNORE-BEGIN

	// The matrix must be orthogonal (Determinant(mat)==1.0f)
	// AND
	// Trace = m00 + m11 + m22 + 1 > 0
	DEPRECATED FASTRETURNCHECK(QuatV_Out) QuatVFromMat33VOrtho( Mat33V_In mat );

	//============================================================================
	// Legacy RAGE functions. Not even sure what the purposes of these are, but
	// they are used. Intentionally undocumented though

	void CrossProduct( Mat33V_InOut outMat, Vec3V_In r );
	void CrossProduct( Mat34V_InOut outMat, Mat34V_In m, Vec3V_In r );
	void DotCrossProduct( Mat33V_InOut outMat, Mat33V_In m, Vec3V_In r );
	void Dot3x3CrossProduct( Mat34V_InOut outMat, Mat34V_In m, Vec3V_In r );
	void Dot3x3CrossProductTranspose( Mat34V_InOut outMat, Mat34V_In m, Vec3V_In r );
// DOM-IGNORE-END


// DOM-IGNORE-BEGIN

namespace Imp
{
	// The _Imp() functions are provided to hide the ugly syntax (macro surrounding a Mat*V argument) that is necessary to help pass via vector registers.
	// The non-_Imp() functions which call the _Imp() functions are very short and are __forceinline'd so that there is no param passing on the stack at all,
	// EVER! (except when the # of arguments exceeds the platform's register passing limits... see README.txt)

	//================================================
	// For private use only...
	//================================================

	Vec::Vector_4V_Out QuatFromMat33V_Imp33( MAT33V_DECL(mat) );
	Vec::Vector_4V_Out QuatFromMat33VOrtho_Imp33( MAT33V_DECL(mat) );
	Vec::Vector_4V_Out QuatVFromEulers_Imp( Vec3V_In radianAngles, Vec::Vector_4V_In sign0, Vec::Vector_4V_In sign1 );
	Vec::Vector_4V_Out QuatVFromEulers_Imp( Vec3V_In radianAngles, EulerAngleOrder order );
	template<u32 x, u32 y, u32 z, u32 sign> Vec::Vector_4V_Out QuatVToEulers_Imp( QuatV_In quatIn );
	Vec::Vector_4V_Out QuatVToEulers_Imp( QuatV_In quatIn, EulerAngleOrder order);

	void Transpose_Imp44( Mat44V_InOut outMat, MAT44V_DECL(mat) );
	void Transpose_Imp33( Mat33V_InOut outMat, MAT33V_DECL(mat) );
	Vec::Vector_4V_Out DeterminantV_Imp44( MAT44V_DECL(mat) );

	void Add_Imp44( Mat44V_InOut outMat, MAT44V_DECL(a), MAT44V_DECL2(b) );
	void Subtract_Imp44( Mat44V_InOut outMat, MAT44V_DECL(a), MAT44V_DECL2(b) );
	void Abs_Imp44( Mat44V_InOut outMat, MAT44V_DECL(a) );
	void Scale_Imp44( Mat44V_InOut outMat, MAT44V_DECL(a), Vec::Vector_4V_In_After3Args b );
	void InvScale_Imp44( Mat44V_InOut outMat, MAT44V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScaleSafe_Imp44( Mat44V_InOut outMat, MAT44V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect );
	void InvScaleFast_Imp44( Mat44V_InOut outMat, MAT44V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScaleFastSafe_Imp44( Mat44V_InOut outMat, MAT44V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect );

	void AddScaled_Imp33( Mat33V_InOut outMat, MAT33V_DECL(toAddTo), MAT33V_DECL2(toScaleThenAdd), MAT33V_DECL2(toScaleBy) );
	void AddScaled_Imp33( Mat33V_InOut outMat, MAT33V_DECL(toAddTo), MAT33V_DECL2(toScaleThenAdd), Vec::Vector_4V_In_After3Args toScaleBy );

	void AddScaled_Imp44( Mat44V_InOut outMat, MAT44V_DECL(toAddTo), MAT44V_DECL2(toScaleThenAdd), MAT44V_DECL2(toScaleBy) );
	void AddScaled_Imp44( Mat44V_InOut outMat, MAT44V_DECL(toAddTo), MAT44V_DECL2(toScaleThenAdd), Vec::Vector_4V_In_After3Args toScaleBy );

	void Add_Imp34( Mat34V_InOut outMat, MAT34V_DECL(a), MAT34V_DECL2(b) );
	void Subtract_Imp34( Mat34V_InOut outMat, MAT34V_DECL(a), MAT34V_DECL2(b) );
	void Abs_Imp34( Mat34V_InOut outMat, MAT34V_DECL(a) );
	void Scale_Imp34( Mat34V_InOut outMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void Scale_Imp33( Mat34V_InOut inoutMat, MAT34V_DECL(a), Vec::Vector_4V_In_After3Args b );
	void InvScale_Imp34( Mat34V_InOut outMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScaleSafe_Imp34( Mat34V_InOut outMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect );
	void InvScaleFast_Imp34( Mat34V_InOut outMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScaleFastSafe_Imp34( Mat34V_InOut outMat, MAT34V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect );

	void Add_Imp33( Mat33V_InOut outMat, MAT33V_DECL(a), MAT33V_DECL2(b) );
	void Subtract_Imp33( Mat33V_InOut outMat, MAT33V_DECL(a), MAT33V_DECL2(b) );
	void Abs_Imp33( Mat33V_InOut outMat, MAT33V_DECL(a) );
	void Scale_Imp33( Mat33V_InOut outMat, MAT33V_DECL(a), Vec::Vector_4V_In_After3Args b );
	void InvScale_Imp33( Mat33V_InOut outMat, MAT33V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScaleSafe_Imp33( Mat33V_InOut outMat, MAT33V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect );
	void InvScaleFast_Imp33( Mat33V_InOut outMat, MAT33V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue );
	void InvScaleFastSafe_Imp33( Mat33V_InOut outMat, MAT33V_DECL(toScale), Vec::Vector_4V_In_After3Args scaleValue, Vec::Vector_4V_In_After3Args errValVect );

	void InvertFull_Imp44( Mat44V_InOut outMat, MAT44V_DECL(a) );
	void InvertTransformFull_Imp34( Mat34V_InOut outMat, MAT34V_DECL(a) );
	void InvertTransformOrtho_Imp34( Mat34V_InOut outMat, MAT34V_DECL(a) );
	void InvertFull_Imp33( Mat33V_InOut outMat, MAT33V_DECL(a) );

	void Mul_Imp_44_44( Mat44V_InOut outMat, MAT44V_DECL(a), MAT44V_DECL2(b) );
	void Mul_Imp_33_33( Mat33V_InOut outMat, MAT33V_DECL(a), MAT33V_DECL2(b) );

	Vec::Vector_4V_Out Mul_Imp_44_4( MAT44V_DECL(a), Vec::Vector_4V_In_After3Args b );
	Vec::Vector_4V_Out Mul_Imp_4_44( Vec::Vector_4V_In a, MAT44V_DECL3(b) );
	Vec::Vector_4V_Out Mul_Imp_33_3( MAT33V_DECL(a), Vec::Vector_4V_In_After3Args b );
	Vec::Vector_4V_Out Mul_Imp_3_33( Vec::Vector_4V_In a, MAT33V_DECL3(b) );
	Vec::Vector_4V_Out Mul_Imp_34_4( MAT34V_DECL(a), Vec::Vector_4V_In_After3Args b );
	Vec::Vector_4V_Out Mul_Imp_3_34( Vec::Vector_4V_In a, MAT34V_DECL3(b) );

	Vec::Vector_4V_Out Transform_Imp34( MAT34V_DECL(transformMat), Vec::Vector_4V_In_After3Args inPoint );

	Vec::Vector_4V_Out UnTransformFull_Imp34( MAT34V_DECL(origTransformMat), Vec::Vector_4V_In_After3Args inPoint );
	Vec::Vector_4V_Out UnTransformOrtho_Imp34( MAT34V_DECL(origOrthoTransformMat), Vec::Vector_4V_In_After3Args inPoint );

	void ReOrthonormalize_Imp33( Mat33V_InOut outMat, MAT33V_DECL(inMat) );

	void UnTransformFull_Imp44( Mat44V_InOut outMat, MAT44V_DECL(origTransformMat), MAT44V_DECL2(concatMat) );
	void UnTransformOrtho_Imp44( Mat44V_InOut outMat, MAT44V_DECL(origOrthoTransformMat), MAT44V_DECL2(concatMat) );

	Vec::Vector_4V_Out UnTransformFull_Imp44( MAT44V_DECL(origTransformMat), Vec::Vector_4V_In_After3Args transformedVect );

	void UnTransformFull_Imp33( Mat33V_InOut outMat, MAT33V_DECL(origTransformMat), MAT33V_DECL2(concatMat) );
	void UnTransformOrtho_Imp33( Mat33V_InOut outMat, MAT33V_DECL(origOrthoTransformMat), MAT33V_DECL2(concatMat) );
	Vec::Vector_4V_Out UnTransformFull_Imp33( MAT33V_DECL(origTransformMat), Vec::Vector_4V_In_After3Args transformedVect );

	void Transform_Imp34( Mat34V_InOut outMat, MAT34V_DECL(transformMat1), MAT34V_DECL2(transformMat2) );
	void UnTransformFull_Imp34( Mat34V_InOut outMat, MAT34V_DECL(origTransformMat), MAT34V_DECL2(concatMat) );
	void UnTransformOrtho_Imp34( Mat34V_InOut outMat, MAT34V_DECL(origOrthoTransformMat), MAT34V_DECL2(concatMat) );
} // namespace Imp

// DOM-IGNORE-END

} // namespace rage

#endif // VECTORMATH_CLASSFREEFUNCSV_H
