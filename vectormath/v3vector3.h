#ifndef VECTORMATH_V3VECTOR3_H
#define VECTORMATH_V3VECTOR3_H

namespace rage
{
namespace Vec
{


//================================================
//	SCALAR(3) VERSIONS (Vector_3 params)
//================================================

float GetElem( Vector_3_In inVector, unsigned int elem );
float& GetElemRef( Vector_3_Ptr pInVector, unsigned int elem );
const float& GetElemRef( Vector_3_ConstPtr pInVector, unsigned int elem );
float GetX( Vector_3_In inVector );
float GetY( Vector_3_In inVector );
float GetZ( Vector_3_In inVector );

Vector_3_Out GetXV( Vector_3_In inVector );
Vector_3_Out GetYV( Vector_3_In inVector );
Vector_3_Out GetZV( Vector_3_In inVector );

void SetX( Vector_3_InOut inoutVector, float floatVal );
void SetY( Vector_3_InOut inoutVector, float floatVal );
void SetZ( Vector_3_InOut inoutVector, float floatVal );

Vector_3_Out V3SplatX( Vector_3_In inVector );
Vector_3_Out V3SplatY( Vector_3_In inVector );
Vector_3_Out V3SplatZ( Vector_3_In inVector );

void V3Set( Vector_3_InOut inoutVector, float x0, float y0, float z0 );
void V3Set( Vector_3_InOut inoutVector, Vector_3_In inVector );
void V3Set( Vector_3_InOut inoutVector, float s );

void V3ZeroComponents( Vector_3_InOut inoutVector );

//============================================================================
// Standard Algebra

//////////////////////////////////////
// ANGULAR ADDITIONS HERE
//////////////////////////////////////

Vector_3_Out V3AddNet( Vector_3_In inVector, Vector_3_In toAdd );
Vector_3_Out V3Extend( Vector_3_In inVector, Vector_3_In amount );

float V3Angle(Vector_3_In v1, Vector_3_In v2);
float V3AngleNormInput(Vector_3_In v1, Vector_3_In v2);
float V3AngleX(Vector_3_In v1, Vector_3_In v2);
float V3AngleY(Vector_3_In v1, Vector_3_In v2);
float V3AngleZ(Vector_3_In v1, Vector_3_In v2);
float V3AngleXNormInput(Vector_3_In v1, Vector_3_In v2);
float V3AngleYNormInput(Vector_3_In v1, Vector_3_In v2);
float V3AngleZNormInput(Vector_3_In v1, Vector_3_In v2);

Vector_3_Out V3AngleV(Vector_3_In v1, Vector_3_In v2);
Vector_3_Out V3AngleVNormInput(Vector_3_In v1, Vector_3_In v2);
Vector_3_Out V3AngleXV(Vector_3_In v1, Vector_3_In v2);
Vector_3_Out V3AngleYV(Vector_3_In v1, Vector_3_In v2);
Vector_3_Out V3AngleZV(Vector_3_In v1, Vector_3_In v2);
Vector_3_Out V3AngleXVNormInput(Vector_3_In v1, Vector_3_In v2);
Vector_3_Out V3AngleYVNormInput(Vector_3_In v1, Vector_3_In v2);
Vector_3_Out V3AngleZVNormInput(Vector_3_In v1, Vector_3_In v2);
Vector_3_Out V3ApproachStraight(Vector_3_In position, Vector_3_In goal, float rate, float time, unsigned int& rResult);
void V3MakeOrthonormals(Vector_3_In inVector, Vector_3_InOut ortho1, Vector_3_InOut ortho2);
// assumes inVector and planeNormal are normalized.
Vector_3_Out V3Reflect( Vector_3_In inVector, Vector_3_In planeNormal );
Vector_3_Out V3RotateAboutXAxis( Vector_3_In inVector, float radians );
Vector_3_Out V3RotateAboutYAxis( Vector_3_In inVector, float radians );
Vector_3_Out V3RotateAboutZAxis( Vector_3_In inVector, float radians );

//////////////////////////////////////
// ANGULAR ADDITIONS....
//////////////////////////////////////

Vector_3_Out V3Scale( Vector_3_In inVector, float floatVal );
Vector_3_Out V3Scale( Vector_3_In inVector1, Vector_3_In inVector2 );

Vector_3_Out V3ClampMag( Vector_3_In v, float minMag, float maxMag );

Vector_3_Out V3InvScale( Vector_3_In inVector, float floatVal );
Vector_3_Out V3InvScale( Vector_3_In inVector, Vector_3_In floatVal );
Vector_3_Out V3InvScaleSafe( Vector_3_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_3_Out V3InvScaleSafe( Vector_3_In inVector, Vector_3_In floatVal, float errVal = LARGE_FLOAT );
Vector_3_Out V3InvScaleFast( Vector_3_In inVector, float floatVal );
Vector_3_Out V3InvScaleFast( Vector_3_In inVector, Vector_3_In floatVal );
Vector_3_Out V3InvScaleFastSafe( Vector_3_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_3_Out V3InvScaleFastSafe( Vector_3_In inVector, Vector_3_In floatVal, float errVal = LARGE_FLOAT );

Vector_3_Out V3Add( Vector_3_In inVector, float sx, float sy, float sz );
Vector_3_Out V3Add( Vector_3_In inVector1, Vector_3_In inVector2 );				

Vector_3_Out V4AddInt( Vector_3_In inVector1, Vector_3_In inVector2 );
Vector_3_Out V4SubtractInt( Vector_3_In inVector1, Vector_3_In inVector2 );

Vector_3_Out V3AddScaled( Vector_3_In inVector1, Vector_3_In inVector2, float floatValue );
Vector_3_Out V3AddScaled( Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In floatValue );

Vector_3_Out V3Subtract( Vector_3_In inVector, float sx, float sy, float sz, float sw);
Vector_3_Out V3Subtract( Vector_3_In inVector1, Vector_3_In inVector2 );

Vector_3_Out V3SubtractScaled( Vector_3_In inVector1, Vector_3_In inVector2, float floatValue );
Vector_3_Out V3SubtractScaled( Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In floatValue );	

Vector_3_Out V3Negate(Vector_3_In inVector);

Vector_3_Out V3Abs(Vector_3_In inVector);

Vector_3_Out V3InvertBits(Vector_3_In inVector);

Vector_3_Out V3Invert(Vector_3_In inVector);
Vector_3_Out V3InvertSafe(Vector_3_In inVector, float errVal = LARGE_FLOAT);
Vector_3_Out V3InvertFast(Vector_3_In inVector);
Vector_3_Out V3InvertFastSafe(Vector_3_In inVector, float errVal = LARGE_FLOAT);

Vector_3_Out V3Cross(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3AddCrossed(Vector_3_In toAddTo, Vector_3_In a, Vector_3_In b);
Vector_3_Out V3SubtractCrossed(Vector_3_In toSubtractFrom, Vector_3_In a, Vector_3_In b);

Vector_3_Out V3Average(Vector_3_In a, Vector_3_In b);

Vector_3_Out V3Lerp( float t, Vector_3_In a, Vector_3_In b );
Vector_3_Out V3Lerp( Vector_3_In t, Vector_3_In a, Vector_3_In b );

Vector_3_Out V3Pow( Vector_3_In x, Vector_3_In y );
Vector_3_Out V3Expt( Vector_3_In x );
Vector_3_Out V3Log2( Vector_3_In x );
Vector_3_Out V3Log10( Vector_3_In x );

Vector_3_Out V3SlowInOut( Vector_3_In t );
Vector_3_Out V3SlowIn( Vector_3_In t );
Vector_3_Out V3SlowOut( Vector_3_In t );
Vector_3_Out V3BellInOut( Vector_3_In t );
//   Returns f(x), which is a piecewise linear function with three sections:
//     For x <= funcInA, f(x) = funcOutA.
//     for x > funcInA and x < funcInB, f(x) ramps from funcOutA to funcOutB
//     for x >= funcInB, f(x) = funcOutB
Vector_3_Out V3Ramp( Vector_3_In x, Vector_3_In funcInA, Vector_3_In funcInB, Vector_3_In funcOutA, Vector_3_In funcOutB );
Vector_3_Out V3RampFast( Vector_3_In x, Vector_3_In funcInA, Vector_3_In funcInB, Vector_3_In funcOutA, Vector_3_In funcOutB );
Vector_3_Out V3Range( Vector_3_In t, Vector_3_In lower, Vector_3_In upper );
Vector_3_Out V3RangeFast( Vector_3_In t, Vector_3_In lower, Vector_3_In upper );
Vector_3_Out V3RangeClamp( Vector_3_In t, Vector_3_In lower, Vector_3_In upper );
Vector_3_Out V3RangeClampFast( Vector_3_In t, Vector_3_In lower, Vector_3_In upper );

//============================================================================
// Magnitude and distance

float V3Dot(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3DotV(Vector_3_In a, Vector_3_In b);

Vector_3_Out V3Normalize(Vector_3_In inVector);
Vector_3_Out V3NormalizeFast(Vector_3_In inVector);
Vector_3_Out V3NormalizeSafe(Vector_3_In inVector, float errVal, float magSqThreshold = 1e-5f);
Vector_3_Out V3NormalizeFastSafe(Vector_3_In inVector, float errVal, float magSqThreshold = 1e-5f);

Vector_3_Out V3Sqrt( Vector_3_In v );
Vector_3_Out V3SqrtSafe( Vector_3_In v, float errVal = 0.0f );
Vector_3_Out V3SqrtFast( Vector_3_In v );
Vector_3_Out V3SqrtFastSafe( Vector_3_In v, float errVal = 0.0f );

Vector_3_Out V3InvSqrt( Vector_3_In v );
Vector_3_Out V3InvSqrtSafe( Vector_3_In v, float errVal = LARGE_FLOAT );
Vector_3_Out V3InvSqrtFast( Vector_3_In v );
Vector_3_Out V3InvSqrtFastSafe( Vector_3_In v, float errVal = LARGE_FLOAT );

float V3Mag( Vector_3_In v );
float V3MagFast( Vector_3_In v );
Vector_3_Out V3MagV( Vector_3_In v );
Vector_3_Out V3MagVFast( Vector_3_In v );

float V3MagSquared( Vector_3_In v );
Vector_3_Out V3MagSquaredV( Vector_3_In v );

float V3InvMag( Vector_3_In v );
float V3InvMagSafe( Vector_3_In v, float errVal = LARGE_FLOAT );
float V3InvMagFast( Vector_3_In v );
float V3InvMagFastSafe( Vector_3_In v, float errVal = LARGE_FLOAT );

Vector_3_Out V3InvMagV( Vector_3_In v );
Vector_3_Out V3InvMagVSafe( Vector_3_In v, float errVal = LARGE_FLOAT );
Vector_3_Out V3InvMagVFast( Vector_3_In v );
Vector_3_Out V3InvMagVFastSafe( Vector_3_In v, float errVal = LARGE_FLOAT );

float V3InvMagSquared( Vector_3_In v );
float V3InvMagSquaredSafe( Vector_3_In v, float errVal = LARGE_FLOAT );
float V3InvMagSquaredFast( Vector_3_In v );
float V3InvMagSquaredFastSafe( Vector_3_In v, float errVal = LARGE_FLOAT );

Vector_3_Out V3InvMagSquaredV( Vector_3_In v );
Vector_3_Out V3InvMagSquaredVSafe( Vector_3_In v, float errVal = LARGE_FLOAT );
Vector_3_Out V3InvMagSquaredVFast( Vector_3_In v );
Vector_3_Out V3InvMagSquaredVFastSafe( Vector_3_In v, float errVal = LARGE_FLOAT );

float V3Dist(Vector_3_In a, Vector_3_In b);
float V3DistFast(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3DistV(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3DistVFast(Vector_3_In a, Vector_3_In b);

float V3InvDist(Vector_3_In a, Vector_3_In b);
float V3InvDistSafe(Vector_3_In a, Vector_3_In b, float errVal = LARGE_FLOAT);
float V3InvDistFast(Vector_3_In a, Vector_3_In b);
float V3InvDistFastSafe(Vector_3_In a, Vector_3_In b, float errVal = LARGE_FLOAT);

Vector_3_Out V3InvDistV(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3InvDistVSafe(Vector_3_In a, Vector_3_In b, float errVal = LARGE_FLOAT);
Vector_3_Out V3InvDistVFast(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3InvDistVFastSafe(Vector_3_In a, Vector_3_In b, float errVal = LARGE_FLOAT);

float V3DistSquared(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3DistSquaredV(Vector_3_In a, Vector_3_In b);

float V3InvDistSquared(Vector_3_In a, Vector_3_In b);
float V3InvDistSquaredSafe(Vector_3_In a, Vector_3_In b, float errVal = LARGE_FLOAT);
float V3InvDistSquaredFast(Vector_3_In a, Vector_3_In b);
float V3InvDistSquaredFastSafe(Vector_3_In a, Vector_3_In b, float errVal = LARGE_FLOAT);

Vector_3_Out V3InvDistSquaredV(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3InvDistSquaredVSafe(Vector_3_In a, Vector_3_In b, float errVal = LARGE_FLOAT);
Vector_3_Out V3InvDistSquaredVFast(Vector_3_In a, Vector_3_In b);
Vector_3_Out V3InvDistSquaredVFastSafe(Vector_3_In a, Vector_3_In b, float errVal = LARGE_FLOAT);

//============================================================================
// Conversion functions

template <int exponent>
Vector_3_Out V3FloatToIntRaw(Vector_3_In inVector);
template <int exponent>
Vector_3_Out V3IntToFloatRaw(Vector_3_In inVector);
Vector_3_Out V3RoundToNearestInt(Vector_3_In inVector);
Vector_3_Out V3RoundToNearestIntZero(Vector_3_In inVector);
Vector_3_Out V3RoundToNearestIntNegInf(Vector_3_In inVector);
Vector_3_Out V3RoundToNearestIntPosInf(Vector_3_In inVector);

//============================================================================
// Comparison functions

unsigned int V3IsBetweenNegAndPosBounds( Vector_3_In testVector, Vector_3_In boundsVector );

unsigned int V3IsZeroAll(Vector_3_In inVector);
unsigned int V3IsZeroNone(Vector_3_In inVector);

Vector_3_Out V3IsEqualV(Vector_3_In inVector1, Vector_3_In inVector2);
unsigned int V3IsEqualAll(Vector_3_In inVector1, Vector_3_In inVector2);
unsigned int V3IsEqualNone(Vector_3_In inVector1, Vector_3_In inVector2);

Vector_3_Out V3IsEqualIntV(Vector_3_In inVector1, Vector_3_In inVector2);
unsigned int V3IsEqualIntAll(Vector_3_In inVector1, Vector_3_In inVector2);
unsigned int V3IsEqualIntNone(Vector_3_In inVector1, Vector_3_In inVector2);

Vector_3_Out V3IsCloseV(Vector_3_In inVector1, Vector_3_In inVector2, float eps);
Vector_3_Out V3IsCloseV(Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In eps);
unsigned int V3IsCloseAll(Vector_3_In inVector1, Vector_3_In inVector2, float eps);
unsigned int V3IsCloseNone(Vector_3_In inVector1, Vector_3_In inVector2, float eps);
unsigned int V3IsCloseAll(Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In eps);
unsigned int V3IsCloseNone(Vector_3_In inVector1, Vector_3_In inVector2, Vector_3_In eps);

unsigned int V3IsGreaterThanAll(Vector_3_In bigVector, Vector_3_In smallVector);
Vector_3_Out V3IsGreaterThanV(Vector_3_In bigVector, Vector_3_In smallVector);

unsigned int V3IsGreaterThanOrEqualAll(Vector_3_In bigVector, Vector_3_In smallVector);
Vector_3_Out V3IsGreaterThanOrEqualV(Vector_3_In bigVector, Vector_3_In smallVector);


unsigned int V3IsLessThanAll(Vector_3_In smallVector, Vector_3_In bigVector);
Vector_3_Out V3IsLessThanV(Vector_3_In smallVector, Vector_3_In bigVector);

unsigned int V3IsLessThanOrEqualAll(Vector_3_In smallVector, Vector_3_In bigVector);
Vector_3_Out V3IsLessThanOrEqualV(Vector_3_In smallVector, Vector_3_In bigVector);

Vector_3_Out V3SelectFT(Vector_3_In choiceVector, Vector_3_In zero, Vector_3_In nonZero);
Vector_3_Out V3SelectVect(Vector_3_In choiceVectorX, Vector_3_In zero, Vector_3_In nonZero);

Vector_3_Out V3Max(Vector_3_In inVector1, Vector_3_In inVector2);
Vector_3_Out V3Min(Vector_3_In inVector1, Vector_3_In inVector2);
Vector_3_Out V3Clamp( Vector_3_In inVector, Vector_3_In lowBound, Vector_3_In highBound );
Vector_3_Out V3Saturate( Vector_3_In inVector );

//============================================================================
// Quaternions

Vector_3_Out V3QuatRotate( Vector_3_In inVect3, Vector_4_In inQuat );
Vector_3_Out V3QuatRotateReverse( Vector_3_In inVect3, Vector_4_In inQuat );

//============================================================================
// Output

#if !__NO_OUTPUT
void V3Print(Vector_3_In inVector, bool newline=true);
void V3PrintHex(Vector_3_In inVector, bool newline=true);
#endif

//============================================================================
// Bitwise operations

template <int shift>
Vector_3_Out V3ShiftLeft( Vector_3_In inVector );
template <int shift>
Vector_3_Out V3ShiftRight( Vector_3_In inVector );
template <int shift>
Vector_3_Out V3ShiftRightAlgebraic( Vector_3_In inVector );

Vector_3_Out V3And(Vector_3_In inVector1, Vector_3_In inVector2);
Vector_3_Out V3Or(Vector_3_In inVector1, Vector_3_In inVector2);
Vector_3_Out V3Xor(Vector_3_In inVector1, Vector_3_In inVector2);
Vector_3_Out V3Andc(Vector_3_In inVector1, Vector_3_In inVector2);

} // namespace Vec
} // namespace rage


#endif // VECTORMATH_V3VECTOR3_H
