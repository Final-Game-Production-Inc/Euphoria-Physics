#ifndef VECTORMATH_V3VECTOR4_H
#define VECTORMATH_V3VECTOR4_H

namespace rage
{
namespace Vec
{


//================================================
//	SCALAR(3) VERSIONS (Vector_4 params)
//================================================

void V3Set( Vector_4_InOut inoutVector, float x0, float y0, float z0 );
void V3Set( Vector_4_InOut inoutVector, Vector_4_In inVector );
void V3Set( Vector_4_InOut inoutVector, float s );

void V3ZeroComponents( Vector_4_InOut inoutVector );

//============================================================================
// Standard Algebra

//////////////////////////////////////
// ANGULAR ADDITIONS HERE
//////////////////////////////////////

Vector_4_Out V3AddNet( Vector_4_In inVector, Vector_4_In toAdd );
Vector_4_Out V3Extend( Vector_4_In inVector, Vector_4_In amount );
Vector_4_Out V3AngleV(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V3AngleVNormInput(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V3AngleXV(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V3AngleYV(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V3AngleZV(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V3AngleXVNormInput(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V3AngleYVNormInput(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V3AngleZVNormInput(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V3ApproachStraight(Vector_4_In position, Vector_4_In goal, float rate, float time, unsigned int& rResult);
void V3MakeOrthonormals(Vector_4_In inVector, Vector_4_InOut ortho1, Vector_4_InOut ortho2);
// assumes inVector and planeNormal are normalized.
Vector_4_Out V3Reflect( Vector_4_In inVector, Vector_4_In planeNormal );
Vector_4_Out V3RotateAboutXAxis( Vector_4_In inVector, float radians );
Vector_4_Out V3RotateAboutYAxis( Vector_4_In inVector, float radians );
Vector_4_Out V3RotateAboutZAxis( Vector_4_In inVector, float radians );

//////////////////////////////////////
// ANGULAR ADDITIONS....
//////////////////////////////////////

Vector_4_Out V3Scale( Vector_4_In inVector, float floatVal );
Vector_4_Out V3Scale( Vector_4_In inVector1, Vector_4_In inVector2 );

Vector_4_Out V3ClampMag( Vector_4_In v, float minMag, float maxMag );

float V3MagXYSquared( Vector_4_In v );
float V3MagXZSquared( Vector_4_In v );
float V3MagYZSquared( Vector_4_In v );
Vector_4_Out V3MagXYSquaredV( Vector_4_In v );
Vector_4_Out V3MagXZSquaredV( Vector_4_In v );
Vector_4_Out V3MagYZSquaredV( Vector_4_In v );

float V3MagXY( Vector_4_In v );
float V3MagXZ( Vector_4_In v );
float V3MagYZ( Vector_4_In v );
Vector_4_Out V3MagXYV( Vector_4_In v );
Vector_4_Out V3MagXZV( Vector_4_In v );
Vector_4_Out V3MagYZV( Vector_4_In v );
Vector_4_Out V3MagXYVFast( Vector_4_In v );
Vector_4_Out V3MagXZVFast( Vector_4_In v );
Vector_4_Out V3MagYZVFast( Vector_4_In v );

Vector_4_Out V3DistXYV( Vector_4_In a, Vector_4_In b );
Vector_4_Out V3DistXZV( Vector_4_In a, Vector_4_In b );
Vector_4_Out V3DistYZV( Vector_4_In a, Vector_4_In b );
Vector_4_Out V3DistXYVFast( Vector_4_In a, Vector_4_In b );
Vector_4_Out V3DistXZVFast( Vector_4_In a, Vector_4_In b );
Vector_4_Out V3DistYZVFast( Vector_4_In a, Vector_4_In b );



Vector_4_Out V3InvScale( Vector_4_In inVector, float floatVal );
Vector_4_Out V3InvScale( Vector_4_In inVector, Vector_4_In floatVal );
Vector_4_Out V3InvScaleSafe( Vector_4_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V3InvScaleSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V3InvScaleFast( Vector_4_In inVector, float floatVal );
Vector_4_Out V3InvScaleFast( Vector_4_In inVector, Vector_4_In floatVal );
Vector_4_Out V3InvScaleFastSafe( Vector_4_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V3InvScaleFastSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal = LARGE_FLOAT );

Vector_4_Out V3Add( Vector_4_In inVector, float sx, float sy, float sz );
Vector_4_Out V3Add( Vector_4_In inVector1, Vector_4_In inVector2 );				

Vector_4_Out V3AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue );
Vector_4_Out V3AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue );

Vector_4_Out V3Subtract( Vector_4_In inVector, float sx, float sy, float sz );
Vector_4_Out V3Subtract( Vector_4_In inVector1, Vector_4_In inVector2 );

Vector_4_Out V3SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue );
Vector_4_Out V3SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue );



Vector_4_Out V3Negate(Vector_4_In inVector);

Vector_4_Out V3Abs(Vector_4_In inVector);

Vector_4_Out V3Invert(Vector_4_In inVector);
Vector_4_Out V3InvertSafe(Vector_4_In inVector);
Vector_4_Out V3InvertFast(Vector_4_In inVector, float errVal = LARGE_FLOAT);
Vector_4_Out V3InvertFastSafe(Vector_4_In inVector, float errVal = LARGE_FLOAT);

// PURPOSE:	Combines the least-significant bits of the X/Y/Z components into the 3 LSBs of an integer.
void V3ResultToIndexZYX( u32& outInt, Vector_4_In maskVector );
// PURPOSE:	Combines the least-significant bits of the X/Y/Z components into the 3 LSBs of an integer.
void V3ResultToIndexXYZ( u32& outInt, Vector_4_In maskVector );

// Note that, although all V3* functions have an undefined result for w, this function will set w = 0.0f.
// This is because it's a free side effect of the vectorized version, and so it happens here for consistency.
Vector_4_Out V3Cross(Vector_4_In a, Vector_4_In b);

// Note that, although all V3* functions have an undefined result for w, this function will set w = toAddTo.w.
// This is because it's a free side effect of the vectorized version, and so it happens here for consistency.
Vector_4_Out V3AddCrossed(Vector_4_In toAddTo, Vector_4_In a, Vector_4_In b);
// Note that, although all V3* functions have an undefined result for w, this function will set w = toSubtractFrom.w.
// This is because it's a free side effect of the vectorized version, and so it happens here for consistency.
Vector_4_Out V3SubtractCrossed(Vector_4_In toSubtractFrom, Vector_4_In a, Vector_4_In b);

Vector_4_Out V3Average(Vector_4_In a, Vector_4_In b);

Vector_4_Out V3Lerp( float t, Vector_4_In a, Vector_4_In b );
Vector_4_Out V3Lerp( Vector_4_In t, Vector_4_In a, Vector_4_In b );

Vector_4_Out V3Pow( Vector_4_In x, Vector_4_In y );
Vector_4_Out V3Expt( Vector_4_In x );
Vector_4_Out V3Log2( Vector_4_In x );
Vector_4_Out V3Log10( Vector_4_In x );

//============================================================================
// Magnitude and distance

float V3Dot(Vector_4_In a, Vector_4_In b);
Vector_4_Out V3DotV(Vector_4_In a, Vector_4_In b);

Vector_4_Out V3Normalize(Vector_4_In inVector);
Vector_4_Out V3NormalizeFast(Vector_4_In inVector);
Vector_4_Out V3NormalizeSafe(Vector_4_In inVector, float errVal, float magSqThreshold = 1e-5f);
Vector_4_Out V3NormalizeSafe(Vector_4_In inVector, Vector_4_In errVal, Vector_4_In magSqThreshold = V4Constant(V_FLT_SMALL_5));
Vector_4_Out V3NormalizeFastSafe(Vector_4_In inVector, float errVal, float magSqThreshold = 1e-5f);

Vector_4_Out V3Sqrt( Vector_4_In v );
Vector_4_Out V3SqrtSafe( Vector_4_In v, float errVal = 0.0f );
Vector_4_Out V3SqrtFast( Vector_4_In v );
Vector_4_Out V3SqrtFastSafe( Vector_4_In v, float errVal = 0.0f );

Vector_4_Out V3InvSqrt( Vector_4_In v );
Vector_4_Out V3InvSqrtSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
Vector_4_Out V3InvSqrtFast( Vector_4_In v );
Vector_4_Out V3InvSqrtFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

float V3Mag( Vector_4_In v );
float V3MagFast( Vector_4_In v );
Vector_4_Out V3MagV( Vector_4_In v );
Vector_4_Out V3MagVFast( Vector_4_In v );

float V3MagSquared( Vector_4_In v );
Vector_4_Out V3MagSquaredV( Vector_4_In v );

float V3InvMag( Vector_4_In v );
float V3InvMagSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
float V3InvMagFast( Vector_4_In v );
float V3InvMagFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

Vector_4_Out V3InvMagV( Vector_4_In v );
Vector_4_Out V3InvMagVSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
Vector_4_Out V3InvMagVFast( Vector_4_In v );
Vector_4_Out V3InvMagVFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

float V3InvMagSquared( Vector_4_In v );
float V3InvMagSquaredSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
float V3InvMagSquaredFast( Vector_4_In v );
float V3InvMagSquaredFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

Vector_4_Out V3InvMagSquaredV( Vector_4_In v );
Vector_4_Out V3InvMagSquaredVSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
Vector_4_Out V3InvMagSquaredVFast( Vector_4_In v );
Vector_4_Out V3InvMagSquaredVFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

float V3Dist(Vector_4_In a, Vector_4_In b);
float V3DistFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V3DistV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V3DistVFast(Vector_4_In a, Vector_4_In b);

float V3InvDist(Vector_4_In a, Vector_4_In b);
float V3InvDistSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
float V3InvDistFast(Vector_4_In a, Vector_4_In b);
float V3InvDistFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

Vector_4_Out V3InvDistV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V3InvDistVSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
Vector_4_Out V3InvDistVFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V3InvDistVFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

float V3DistSquared(Vector_4_In a, Vector_4_In b);
Vector_4_Out V3DistSquaredV(Vector_4_In a, Vector_4_In b);

float V3InvDistSquared(Vector_4_In a, Vector_4_In b);
float V3InvDistSquaredSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
float V3InvDistSquaredFast(Vector_4_In a, Vector_4_In b);
float V3InvDistSquaredFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

Vector_4_Out V3InvDistSquaredV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V3InvDistSquaredVSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
Vector_4_Out V3InvDistSquaredVFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V3InvDistSquaredVFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

//============================================================================
// Conversion functions

template <int exponent>
Vector_4_Out V3FloatToIntRaw(Vector_4_In inVector);
template <int exponent>
Vector_4_Out V3IntToFloatRaw(Vector_4_In inVector);
Vector_4_Out V3RoundToNearestInt(Vector_4_In inVector);
Vector_4_Out V3RoundToNearestIntZero(Vector_4_In inVector);
Vector_4_Out V3RoundToNearestIntNegInf(Vector_4_In inVector);
Vector_4_Out V3RoundToNearestIntPosInf(Vector_4_In inVector);

//============================================================================
// Comparison functions

unsigned int V3IsBetweenNegAndPosBounds( Vector_4_In testVector, Vector_4_In boundsVector );

unsigned int V3SameSignAll(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V3IsZeroAll(Vector_4_In inVector);
unsigned int V3IsZeroNone(Vector_4_In inVector);

unsigned int V3IsEqualAll(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V3IsEqualNone(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V3IsEqualIntAll(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V3IsEqualIntNone(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V3IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
unsigned int V3IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
unsigned int V3IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);
unsigned int V3IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);

unsigned int V3IsGreaterThanAll(Vector_4_In bigVector, Vector_4_In smallVector);

unsigned int V3IsGreaterThanOrEqualAll(Vector_4_In bigVector, Vector_4_In smallVector);

unsigned int V3IsLessThanAll(Vector_4_In smallVector, Vector_4_In bigVector);

unsigned int V3IsLessThanOrEqualAll(Vector_4_In smallVector, Vector_4_In bigVector);

Vector_4_Out V3SelectFT(Vector_4_In choiceVector, Vector_4_In zero, Vector_4_In nonZero);
Vector_4_Out V3SelectVect(Vector_4_In choiceVectorX, Vector_4_In zero, Vector_4_In nonZero);

Vector_4_Out V3Max(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V3Min(Vector_4_In inVector1, Vector_4_In inVector2);

Vector_4_Out V3MinElement(Vector_4_In inVector);
Vector_4_Out V3MaxElement(Vector_4_In inVector);

Vector_4_Out V3Clamp( Vector_4_In inVector, Vector_4_In lowBound, Vector_4_In highBound );
Vector_4_Out V3Saturate( Vector_4_In inVector );

//============================================================================
// Quaternions

Vector_4_Out V3QuatRotate( Vector_4_In inVect3, Vector_4_In inQuat );
Vector_4_Out V3QuatRotateReverse( Vector_4_In inVect3, Vector_4_In inQuat );


//============================================================================
// Output

#if !__NO_OUTPUT
void V3Print(Vector_4_In inVector, bool newline=true);
void V3PrintHex(Vector_4_In inVector, bool newline=true);
#endif

//============================================================================
// Bitwise operations

Vector_4_Out V3And(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V3Or(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V3Xor(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V3Andc(Vector_4_In inVector1, Vector_4_In inVector2);

} // namespace Vec
} // namespace rage


#endif // VECTORMATH_V3VECTOR4_H
