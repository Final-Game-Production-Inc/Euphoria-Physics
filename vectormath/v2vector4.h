#ifndef VECTORMATH_V2VECTOR4_H
#define VECTORMATH_V2VECTOR4_H

namespace rage
{
namespace Vec
{


//================================================
//	SCALAR(2) VERSIONS (Vector_4 params)
//================================================

void V2Set( Vector_4_InOut inoutVector, float x0, float y0 );
void V2Set( Vector_4_InOut inoutVector, Vector_4_In inVector );
void V2Set( Vector_4_InOut inoutVector, float s );

void V2ZeroComponents( Vector_4_InOut inoutVector );

//============================================================================
// Standard Algebra

//////////////////////////////////////
// ANGULAR ADDITIONS HERE
//////////////////////////////////////

Vector_4_Out V2AngleV(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V2AngleVNormInput(Vector_4_In v1, Vector_4_In v2);
Vector_4_Out V2WhichSideOfLineV(Vector_4_In point, Vector_4_In lineP1, Vector_4_In lineP2);
Vector_4_Out V2Rotate(Vector_4_In inVector, float radians);
Vector_4_Out V2AddNet( Vector_4_In inVector, Vector_4_In toAdd );
Vector_4_Out V2Extend( Vector_4_In inVector, Vector_4_In amount );

Vector_4_Out V2ApproachStraight(Vector_4_In position, Vector_4_In goal, float rate, float time, unsigned int& rResult);

// assumes inVector and planeNormal are normalized.
Vector_4_Out V2Reflect( Vector_4_In inVector, Vector_4_In wall2DNormal );

//////////////////////////////////////
// ANGULAR ADDITIONS...
//////////////////////////////////////

Vector_4_Out V2Scale( Vector_4_In inVector, float floatVal );
Vector_4_Out V2Scale( Vector_4_In inVector1, Vector_4_In inVector2 );

Vector_4_Out V2InvScale( Vector_4_In inVector, float floatVal );
Vector_4_Out V2InvScale( Vector_4_In inVector, Vector_4_In floatVal );
Vector_4_Out V2InvScaleSafe( Vector_4_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V2InvScaleSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V2InvScaleFast( Vector_4_In inVector, float floatVal );
Vector_4_Out V2InvScaleFast( Vector_4_In inVector, Vector_4_In floatVal );
Vector_4_Out V2InvScaleFastSafe( Vector_4_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V2InvScaleFastSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal = LARGE_FLOAT );

Vector_4_Out V2Add( Vector_4_In inVector, float sx, float sy );
Vector_4_Out V2Add( Vector_4_In inVector1, Vector_4_In inVector2 );	

Vector_4_Out V2AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue );
Vector_4_Out V2AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue );

Vector_4_Out V2Subtract( Vector_4_In inVector, float sx, float sy );
Vector_4_Out V2Subtract( Vector_4_In inVector1, Vector_4_In inVector2 );

Vector_4_Out V2SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue );
Vector_4_Out V2SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue );

Vector_4_Out V2Negate(Vector_4_In inVector);

Vector_4_Out V2Abs(Vector_4_In inVector);

Vector_4_Out V2Invert(Vector_4_In inVector);
Vector_4_Out V2InvertSafe(Vector_4_In inVector, float errVal = LARGE_FLOAT);
Vector_4_Out V2InvertFast(Vector_4_In inVector);
Vector_4_Out V2InvertFastSafe(Vector_4_In inVector, float errVal = LARGE_FLOAT);

float V2Cross(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2CrossV(Vector_4_In a, Vector_4_In b);

Vector_4_Out V2Average(Vector_4_In a, Vector_4_In b);

Vector_4_Out V2Lerp( float t, Vector_4_In a, Vector_4_In b );
Vector_4_Out V2Lerp( Vector_4_In t, Vector_4_In a, Vector_4_In b );

Vector_4_Out V2Pow( Vector_4_In x, Vector_4_In y );
Vector_4_Out V2Expt( Vector_4_In x );
Vector_4_Out V2Log2( Vector_4_In x );
Vector_4_Out V2Log10( Vector_4_In x );

//============================================================================
// Magnitude and distance

float V2Dot(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2DotV(Vector_4_In a, Vector_4_In b);

Vector_4_Out V2Normalize(Vector_4_In inVector);
Vector_4_Out V2NormalizeFast(Vector_4_In inVector);
Vector_4_Out V2NormalizeSafe(Vector_4_In inVector, float errVal, float magSqThreshold = 1e-5f);
Vector_4_Out V2NormalizeFastSafe(Vector_4_In inVector, float errVal, float magSqThreshold = 1e-5f);

Vector_4_Out V2Sqrt( Vector_4_In v );
Vector_4_Out V2SqrtSafe( Vector_4_In v, float errVal = 0.0f );
Vector_4_Out V2SqrtFast( Vector_4_In v );
Vector_4_Out V2SqrtFastSafe( Vector_4_In v, float errVal = 0.0f );

Vector_4_Out V2InvSqrt( Vector_4_In v );
Vector_4_Out V2InvSqrtSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
Vector_4_Out V2InvSqrtFast( Vector_4_In v );
Vector_4_Out V2InvSqrtFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

float V2Mag( Vector_4_In v );
float V2MagFast( Vector_4_In v );
Vector_4_Out V2MagV( Vector_4_In v );
Vector_4_Out V2MagVFast( Vector_4_In v );

float V2MagSquared( Vector_4_In v );
Vector_4_Out V2MagSquaredV( Vector_4_In v );

float V2InvMag( Vector_4_In v );
float V2InvMagSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
float V2InvMagFast( Vector_4_In v );
float V2InvMagFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

Vector_4_Out V2InvMagV( Vector_4_In v );
Vector_4_Out V2InvMagVSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
Vector_4_Out V2InvMagVFast( Vector_4_In v );
Vector_4_Out V2InvMagVFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

float V2InvMagSquared( Vector_4_In v );
float V2InvMagSquaredSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
float V2InvMagSquaredFast( Vector_4_In v );
float V2InvMagSquaredFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

Vector_4_Out V2InvMagSquaredV( Vector_4_In v );
Vector_4_Out V2InvMagSquaredVSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
Vector_4_Out V2InvMagSquaredVFast( Vector_4_In v );
Vector_4_Out V2InvMagSquaredVFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

float V2Dist(Vector_4_In a, Vector_4_In b);
float V2DistFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2DistV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2DistVFast(Vector_4_In a, Vector_4_In b);

float V2InvDist(Vector_4_In a, Vector_4_In b);
float V2InvDistSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
float V2InvDistFast(Vector_4_In a, Vector_4_In b);
float V2InvDistFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

Vector_4_Out V2InvDistV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2InvDistVSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
Vector_4_Out V2InvDistVFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2InvDistVFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

float V2DistSquared(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2DistSquaredV(Vector_4_In a, Vector_4_In b);

float V2InvDistSquared(Vector_4_In a, Vector_4_In b);
float V2InvDistSquaredSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
float V2InvDistSquaredFast(Vector_4_In a, Vector_4_In b);
float V2InvDistSquaredFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

Vector_4_Out V2InvDistSquaredV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2InvDistSquaredVSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
Vector_4_Out V2InvDistSquaredVFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V2InvDistSquaredVFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

//============================================================================
// Conversion functions

template <int exponent>
Vector_4_Out V2FloatToIntRaw(Vector_4_In inVector);
template <int exponent>
Vector_4_Out V2IntToFloatRaw(Vector_4_In inVector);
Vector_4_Out V2RoundToNearestInt(Vector_4_In inVector);
Vector_4_Out V2RoundToNearestIntZero(Vector_4_In inVector);
Vector_4_Out V2RoundToNearestIntNegInf(Vector_4_In inVector);
Vector_4_Out V2RoundToNearestIntPosInf(Vector_4_In inVector);

//============================================================================
// Comparison functions

unsigned int V2IsBetweenNegAndPosBounds( Vector_4_In testVector, Vector_4_In boundsVector );

unsigned int V2SameSignAll(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V2IsZeroAll(Vector_4_In inVector);
unsigned int V2IsZeroNone(Vector_4_In inVector);

unsigned int V2IsEqualAll(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V2IsEqualNone(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V2IsEqualIntAll(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V2IsEqualIntNone(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V2IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
unsigned int V2IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
unsigned int V2IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);
unsigned int V2IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);

unsigned int V2IsGreaterThanAll(Vector_4_In bigVector, Vector_4_In smallVector);

unsigned int V2IsGreaterThanOrEqualAll(Vector_4_In bigVector, Vector_4_In smallVector);

unsigned int V2IsLessThanAll(Vector_4_In smallVector, Vector_4_In bigVector);

unsigned int V2IsLessThanOrEqualAll(Vector_4_In smallVector, Vector_4_In bigVector);

Vector_4_Out V2SelectFT(Vector_4_In choiceVector, Vector_4_In zero, Vector_4_In nonZero);
Vector_4_Out V2SelectVect(Vector_4_In choiceVectorX, Vector_4_In zero, Vector_4_In nonZero);

Vector_4_Out V2Max(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V2Min(Vector_4_In inVector1, Vector_4_In inVector2);

Vector_4_Out V2MinElement(Vector_4_In inVector);
Vector_4_Out V2MaxElement(Vector_4_In inVector);

Vector_4_Out V2Clamp( Vector_4_In inVector, Vector_4_In lowBound, Vector_4_In highBound );
Vector_4_Out V2Saturate( Vector_4_In inVector );

//============================================================================
// Output

#if !__NO_OUTPUT
void V2Print(Vector_4_In inVector, bool newline=true);
void V2PrintHex(Vector_4_In inVector, bool newline=true);
#endif

//============================================================================
// Bitwise operations

Vector_4_Out V2And(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V2Or(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V2Xor(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V2Andc(Vector_4_In inVector1, Vector_4_In inVector2);

} // namespace Vec
} // namespace rage


#endif // VECTORMATH_V2VECTOR4_H
