#ifndef VECTORMATH_V1VECTOR2_H
#define VECTORMATH_V1VECTOR2_H

namespace rage
{
namespace Vec
{


//================================================
//	SCALAR(2) VERSIONS (Vector_2 params)
//================================================

float GetElem( Vector_2_In inVector, unsigned int elem );
float& GetElemRef( Vector_2_Ptr pInVector, unsigned int elem );
const float& GetElemRef( Vector_2_ConstPtr pInVector, unsigned int elem );
float GetX( Vector_2_In inVector );
float GetY( Vector_2_In inVector );

Vector_2_Out GetXV( Vector_2_In inVector );
Vector_2_Out GetYV( Vector_2_In inVector );

void SetX( Vector_2_InOut inoutVector, float floatVal );
void SetY( Vector_2_InOut inoutVector, float floatVal );

Vector_2_Out V2SplatX( Vector_2_In inVector );
Vector_2_Out V2SplatY( Vector_2_In inVector );

void V2Set( Vector_2_InOut inoutVector, float x0, float y0 );
void V2Set( Vector_2_InOut inoutVector, Vector_2_In inVector );
void V2Set( Vector_2_InOut inoutVector, float s );

void V2ZeroComponents( Vector_2_InOut inoutVector );

//============================================================================
// Standard Algebra

//////////////////////////////////////
// ANGULAR ADDITIONS HERE
//////////////////////////////////////

float V2Angle(Vector_2_In v1, Vector_2_In v2);
float V2AngleNormInput(Vector_2_In v1, Vector_2_In v2);
Vector_2_Out V2AngleV(Vector_2_In v1, Vector_2_In v2);
Vector_2_Out V2AngleVNormInput(Vector_2_In v1, Vector_2_In v2);
float V2WhichSideOfLine(Vector_2_In point, Vector_2_In lineP1, Vector_2_In lineP2);
Vector_2_Out V2WhichSideOfLineV(Vector_2_In point, Vector_2_In lineP1, Vector_2_In lineP2);
Vector_2_Out V2Rotate(Vector_2_In inVector, float radians);

Vector_2_Out V2AddNet( Vector_2_In inVector, Vector_2_In toAdd );
Vector_2_Out V2Extend( Vector_2_In inVector, Vector_2_In amount );

Vector_2_Out V2ApproachStraight(Vector_2_In position, Vector_2_In goal, float rate, float time, unsigned int& rResult);

// assumes inVector and planeNormal are normalized.
Vector_2_Out V2Reflect( Vector_2_In inVector, Vector_2_In wall2DNormal );

//////////////////////////////////////
// ANGULAR ADDITIONS...
//////////////////////////////////////

Vector_2_Out V2Scale( Vector_2_In inVector, float floatVal );
Vector_2_Out V2Scale( Vector_2_In inVector1, Vector_2_In inVector2 );

Vector_2_Out V2InvScale( Vector_2_In inVector, float floatVal );
Vector_2_Out V2InvScale( Vector_2_In inVector, Vector_2_In floatVal );
Vector_2_Out V2InvScaleSafe( Vector_2_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_2_Out V2InvScaleSafe( Vector_2_In inVector, Vector_2_In floatVal, float errVal = LARGE_FLOAT );
Vector_2_Out V2InvScaleFast( Vector_2_In inVector, float floatVal );
Vector_2_Out V2InvScaleFast( Vector_2_In inVector, Vector_2_In floatVal );
Vector_2_Out V2InvScaleFastSafe( Vector_2_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_2_Out V2InvScaleFastSafe( Vector_2_In inVector, Vector_2_In floatVal, float errVal = LARGE_FLOAT );

Vector_2_Out V2Add( Vector_2_In inVector, float sx, float sy );
Vector_2_Out V2Add( Vector_2_In inVector1, Vector_2_In inVector2 );				

Vector_2_Out V2AddInt( Vector_2_In inVector1, Vector_2_In inVector2 );
Vector_2_Out V2SubtractInt( Vector_2_In inVector1, Vector_2_In inVector2 );

Vector_2_Out V2AddScaled( Vector_2_In inVector1, Vector_2_In inVector2, float floatValue );
Vector_2_Out V2AddScaled( Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In floatValue );

Vector_2_Out V2Subtract( Vector_2_In inVector, float sx, float sy, float sz, float sw);
Vector_2_Out V2Subtract( Vector_2_In inVector1, Vector_2_In inVector2 );

Vector_2_Out V2SubtractScaled( Vector_2_In inVector1, Vector_2_In inVector2, float floatValue );
Vector_2_Out V2SubtractScaled( Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In floatValue );	

Vector_2_Out V2Negate(Vector_2_In inVector);

Vector_2_Out V2Abs(Vector_2_In inVector);

Vector_2_Out V2InvertBits(Vector_2_In inVector);

Vector_2_Out V2Invert(Vector_2_In inVector);
Vector_2_Out V2InvertSafe(Vector_2_In inVector, float errVal = LARGE_FLOAT);
Vector_2_Out V2InvertFast(Vector_2_In inVector);
Vector_2_Out V2InvertFastSafe(Vector_2_In inVector, float errVal = LARGE_FLOAT);

// PURPOSE: Calculate the cross product of two vectors.
// PARAMS
// NOTE
//	 This function treats this 2D vector as in the XY plane, and this function returns
//   the Z value of the cross product.
float V2Cross(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2CrossV(Vector_2_In a, Vector_2_In b);

Vector_2_Out V2Average(Vector_2_In a, Vector_2_In b);

Vector_2_Out V2Lerp( float t, Vector_2_In a, Vector_2_In b );
Vector_2_Out V2Lerp( Vector_2_In t, Vector_2_In a, Vector_2_In b );

Vector_2_Out V2Pow( Vector_2_In x, Vector_2_In y );
Vector_2_Out V2Expt( Vector_2_In x );
Vector_2_Out V2Log2( Vector_2_In x );
Vector_2_Out V2Log10( Vector_2_In x );

Vector_2_Out V2SlowInOut( Vector_2_In t );
Vector_2_Out V2SlowIn( Vector_2_In t );
Vector_2_Out V2SlowOut( Vector_2_In t );
Vector_2_Out V2BellInOut( Vector_2_In t );
//   Returns f(x), which is a piecewise linear function with three sections:
//     For x <= funcInA, f(x) = funcOutA.
//     for x > funcInA and x < funcInB, f(x) ramps from funcOutA to funcOutB
//     for x >= funcInB, f(x) = funcOutB
Vector_2_Out V2Ramp( Vector_2_In x, Vector_2_In funcInA, Vector_2_In funcInB, Vector_2_In funcOutA, Vector_2_In funcOutB );
Vector_2_Out V2RampFast( Vector_2_In x, Vector_2_In funcInA, Vector_2_In funcInB, Vector_2_In funcOutA, Vector_2_In funcOutB );
Vector_2_Out V2Range( Vector_2_In t, Vector_2_In lower, Vector_2_In upper );
Vector_2_Out V2RangeFast( Vector_2_In t, Vector_2_In lower, Vector_2_In upper );
Vector_2_Out V2RangeClamp( Vector_2_In t, Vector_2_In lower, Vector_2_In upper );
Vector_2_Out V2RangeClampFast( Vector_2_In t, Vector_2_In lower, Vector_2_In upper );

//============================================================================
// Magnitude and distance

float V2Dot(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2DotV(Vector_2_In a, Vector_2_In b);

Vector_2_Out V2Normalize(Vector_2_In inVector);
Vector_2_Out V2NormalizeFast(Vector_2_In inVector);
Vector_2_Out V2NormalizeSafe(Vector_2_In inVector, float errVal, float magSqThreshold = 1e-5f);
Vector_2_Out V2NormalizeFastSafe(Vector_2_In inVector, float errVal, float magSqThreshold = 1e-5f);

Vector_2_Out V2Sqrt( Vector_2_In v );
Vector_2_Out V2SqrtSafe( Vector_2_In v, float errVal = 0.0f );
Vector_2_Out V2SqrtFast( Vector_2_In v );
Vector_2_Out V2SqrtFastSafe( Vector_2_In v, float errVal = 0.0f );

Vector_2_Out V2InvSqrt( Vector_2_In v );
Vector_2_Out V2InvSqrtSafe( Vector_2_In v, float errVal = LARGE_FLOAT );
Vector_2_Out V2InvSqrtFast( Vector_2_In v );
Vector_2_Out V2InvSqrtFastSafe( Vector_2_In v, float errVal = LARGE_FLOAT );

float V2Mag( Vector_2_In v );
float V2MagFast( Vector_2_In v );
Vector_2_Out V2MagV( Vector_2_In v );
Vector_2_Out V2MagVFast( Vector_2_In v );

float V2MagSquared( Vector_2_In v );
Vector_2_Out V2MagSquaredV( Vector_2_In v );

float V2InvMag( Vector_2_In v );
float V2InvMagSafe( Vector_2_In v, float errVal = LARGE_FLOAT );
float V2InvMagFast( Vector_2_In v );
float V2InvMagFastSafe( Vector_2_In v, float errVal = LARGE_FLOAT );

Vector_2_Out V2InvMagV( Vector_2_In v );
Vector_2_Out V2InvMagVSafe( Vector_2_In v, float errVal = LARGE_FLOAT );
Vector_2_Out V2InvMagVFast( Vector_2_In v );
Vector_2_Out V2InvMagVFastSafe( Vector_2_In v, float errVal = LARGE_FLOAT );

float V2InvMagSquared( Vector_2_In v );
float V2InvMagSquaredSafe( Vector_2_In v, float errVal = LARGE_FLOAT );
float V2InvMagSquaredFast( Vector_2_In v );
float V2InvMagSquaredFastSafe( Vector_2_In v, float errVal = LARGE_FLOAT );

Vector_2_Out V2InvMagSquaredV( Vector_2_In v );
Vector_2_Out V2InvMagSquaredVSafe( Vector_2_In v, float errVal = LARGE_FLOAT );
Vector_2_Out V2InvMagSquaredVFast( Vector_2_In v );
Vector_2_Out V2InvMagSquaredVFastSafe( Vector_2_In v, float errVal = LARGE_FLOAT );

float V2Dist(Vector_2_In a, Vector_2_In b);
float V2DistFast(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2DistV(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2DistVFast(Vector_2_In a, Vector_2_In b);

float V2InvDist(Vector_2_In a, Vector_2_In b);
float V2InvDistSafe(Vector_2_In a, Vector_2_In b, float errVal = LARGE_FLOAT);
float V2InvDistFast(Vector_2_In a, Vector_2_In b);
float V2InvDistFastSafe(Vector_2_In a, Vector_2_In b, float errVal = LARGE_FLOAT);

Vector_2_Out V2InvDistV(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2InvDistVSafe(Vector_2_In a, Vector_2_In b, float errVal = LARGE_FLOAT);
Vector_2_Out V2InvDistVFast(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2InvDistVFastSafe(Vector_2_In a, Vector_2_In b, float errVal = LARGE_FLOAT);

float V2DistSquared(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2DistSquaredV(Vector_2_In a, Vector_2_In b);

float V2InvDistSquared(Vector_2_In a, Vector_2_In b);
float V2InvDistSquaredSafe(Vector_2_In a, Vector_2_In b, float errVal = LARGE_FLOAT);
float V2InvDistSquaredFast(Vector_2_In a, Vector_2_In b);
float V2InvDistSquaredFastSafe(Vector_2_In a, Vector_2_In b, float errVal = LARGE_FLOAT);

Vector_2_Out V2InvDistSquaredV(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2InvDistSquaredVSafe(Vector_2_In a, Vector_2_In b, float errVal = LARGE_FLOAT);
Vector_2_Out V2InvDistSquaredVFast(Vector_2_In a, Vector_2_In b);
Vector_2_Out V2InvDistSquaredVFastSafe(Vector_2_In a, Vector_2_In b, float errVal = LARGE_FLOAT);

//============================================================================
// Conversion functions

template <int exponent>
Vector_2_Out V2FloatToIntRaw(Vector_2_In inVector);
template <int exponent>
Vector_2_Out V2IntToFloatRaw(Vector_2_In inVector);
Vector_2_Out V2RoundToNearestInt(Vector_2_In inVector);
Vector_2_Out V2RoundToNearestIntZero(Vector_2_In inVector);
Vector_2_Out V2RoundToNearestIntNegInf(Vector_2_In inVector);
Vector_2_Out V2RoundToNearestIntPosInf(Vector_2_In inVector);

//============================================================================
// Trigonometry

void V2SinAndCos( Vector_2_InOut inOutSine, Vector_2_InOut inOutCosine, Vector_2_In inVector );
Vector_2_Out V2Sin( Vector_2_In inVector );
Vector_2_Out V2Cos( Vector_2_In inVector );
Vector_2_Out V2Tan( Vector_2_In inVector );
Vector_2_Out V2Arcsin( Vector_2_In inVector );
Vector_2_Out V2Arccos( Vector_2_In inVector );
Vector_2_Out V2Arctan( Vector_2_In inVector );

//============================================================================
// Comparison functions

unsigned int V2IsBetweenNegAndPosBounds( Vector_2_In testVector, Vector_2_In boundsVector );

unsigned int V2IsZeroAll(Vector_2_In inVector);
unsigned int V2IsZeroNone(Vector_2_In inVector);

Vector_2_Out V2IsEqualV(Vector_2_In inVector1, Vector_2_In inVector2);
unsigned int V2IsEqualAll(Vector_2_In inVector1, Vector_2_In inVector2);
unsigned int V2IsEqualNone(Vector_2_In inVector1, Vector_2_In inVector2);

Vector_2_Out V2IsEqualIntV(Vector_2_In inVector1, Vector_2_In inVector2);
unsigned int V2IsEqualIntAll(Vector_2_In inVector1, Vector_2_In inVector2);
unsigned int V2IsEqualIntNone(Vector_2_In inVector1, Vector_2_In inVector2);

Vector_2_Out V2IsCloseV(Vector_2_In inVector1, Vector_2_In inVector2, float eps);
Vector_2_Out V2IsCloseV(Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In eps);
unsigned int V2IsCloseAll(Vector_2_In inVector1, Vector_2_In inVector2, float eps);
unsigned int V2IsCloseNone(Vector_2_In inVector1, Vector_2_In inVector2, float eps);
unsigned int V2IsCloseAll(Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In eps);
unsigned int V2IsCloseNone(Vector_2_In inVector1, Vector_2_In inVector2, Vector_2_In eps);

unsigned int V2IsGreaterThanAll(Vector_2_In bigVector, Vector_2_In smallVector);
Vector_2_Out V2IsGreaterThanV(Vector_2_In bigVector, Vector_2_In smallVector);

unsigned int V2IsGreaterThanOrEqualAll(Vector_2_In bigVector, Vector_2_In smallVector);
Vector_2_Out V2IsGreaterThanOrEqualV(Vector_2_In bigVector, Vector_2_In smallVector);

unsigned int V2IsLessThanAll(Vector_2_In smallVector, Vector_2_In bigVector);
Vector_2_Out V2IsLessThanV(Vector_2_In smallVector, Vector_2_In bigVector);

unsigned int V2IsLessThanOrEqualAll(Vector_2_In smallVector, Vector_2_In bigVector);
Vector_2_Out V2IsLessThanOrEqualV(Vector_2_In smallVector, Vector_2_In bigVector);

Vector_2_Out V2SelectFT(Vector_2_In choiceVector, Vector_2_In zero, Vector_2_In nonZero);
Vector_2_Out V2SelectVect(Vector_2_In choiceVectorX, Vector_2_In zero, Vector_2_In nonZero);

Vector_2_Out V2Max(Vector_2_In inVector1, Vector_2_In inVector2);
Vector_2_Out V2Min(Vector_2_In inVector1, Vector_2_In inVector2);
Vector_2_Out V2Clamp( Vector_2_In inVector, Vector_2_In lowBound, Vector_2_In highBound );
Vector_2_Out V2Saturate( Vector_2_In inVector );

//============================================================================
// Output

#if !__NO_OUTPUT
void V2Print(Vector_2_In inVector, bool newline=true);
void V2PrintHex(Vector_2_In inVector, bool newline=true);
#endif

//============================================================================
// Bitwise operations

template <int shift>
Vector_2_Out V2ShiftLeft( Vector_2_In inVector );
template <int shift>
Vector_2_Out V2ShiftRight( Vector_2_In inVector );
template <int shift>
Vector_2_Out V2ShiftRightAlgebraic( Vector_2_In inVector );

Vector_2_Out V2And(Vector_2_In inVector1, Vector_2_In inVector2);
Vector_2_Out V2Or(Vector_2_In inVector1, Vector_2_In inVector2);
Vector_2_Out V2Xor(Vector_2_In inVector1, Vector_2_In inVector2);
Vector_2_Out V2Andc(Vector_2_In inVector1, Vector_2_In inVector2);

} // namespace Vec
} // namespace rage


#endif // VECTORMATH_V2VECTOR4_H
