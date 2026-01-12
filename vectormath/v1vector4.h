#ifndef VECTORMATH_V1VECTOR4_H
#define VECTORMATH_V1VECTOR4_H

namespace rage
{
namespace Vec
{


//================================================
//	SCALAR(1) VERSIONS (Vector_4 params)
//================================================

void V1Set( Vector_4_InOut inoutVector, float x0 );
void V1Set( Vector_4_InOut inoutVector, Vector_4_In inVector );

//============================================================================
// Standard Algebra

Vector_4_Out V1Scale( Vector_4_In inVector, float floatVal );
Vector_4_Out V1Scale( Vector_4_In inVector1, Vector_4_In inVector2 );

Vector_4_Out V1InvScale( Vector_4_In inVector, float floatVal );
Vector_4_Out V1InvScale( Vector_4_In inVector, Vector_4_In floatVal );
Vector_4_Out V1InvScaleSafe( Vector_4_In inVector, float floatVal, float errVal=LARGE_FLOAT );
Vector_4_Out V1InvScaleSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal=LARGE_FLOAT );
Vector_4_Out V1InvScaleFast( Vector_4_In inVector, float floatVal );
Vector_4_Out V1InvScaleFast( Vector_4_In inVector, Vector_4_In floatVal );
Vector_4_Out V1InvScaleFastSafe( Vector_4_In inVector, float floatVal, float errVal=LARGE_FLOAT );
Vector_4_Out V1InvScaleFastSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal=LARGE_FLOAT );

Vector_4_Out V1Add( Vector_4_In inVector, float sx );
Vector_4_Out V1Add( Vector_4_In inVector1, Vector_4_In inVector2 );	

Vector_4_Out V1AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue );
Vector_4_Out V1AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue );

Vector_4_Out V1Subtract( Vector_4_In inVector, float sx, float sy );
Vector_4_Out V1Subtract( Vector_4_In inVector1, Vector_4_In inVector2 );

Vector_4_Out V1SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue );
Vector_4_Out V1SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue );

Vector_4_Out V1Negate(Vector_4_In inVector);

Vector_4_Out V1Abs(Vector_4_In inVector);

Vector_4_Out V1Invert(Vector_4_In inVector);
Vector_4_Out V1InvertSafe(Vector_4_In inVector, float errVal=LARGE_FLOAT);
Vector_4_Out V1InvertFast(Vector_4_In inVector);
Vector_4_Out V1InvertFastSafe(Vector_4_In inVector, float errVal=LARGE_FLOAT);

Vector_4_Out V1Average(Vector_4_In a, Vector_4_In b);

Vector_4_Out V1Lerp( float t, Vector_4_In a, Vector_4_In b );
Vector_4_Out V1Lerp( Vector_4_In t, Vector_4_In a, Vector_4_In b );

Vector_4_Out V1Pow( Vector_4_In x, Vector_4_In y );
Vector_4_Out V1Expt( Vector_4_In x );
Vector_4_Out V1Log2( Vector_4_In x );
Vector_4_Out V1Log10( Vector_4_In x );

//============================================================================
// Magnitude and distance

Vector_4_Out V1Sqrt( Vector_4_In v );
Vector_4_Out V1SqrtSafe( Vector_4_In v, float errVal=0.0f );
Vector_4_Out V1SqrtFast( Vector_4_In v );
Vector_4_Out V1SqrtFastSafe( Vector_4_In v, float errVal=0.0f );

Vector_4_Out V1InvSqrt( Vector_4_In v );
Vector_4_Out V1InvSqrtSafe( Vector_4_In v, float errVal=LARGE_FLOAT );
Vector_4_Out V1InvSqrtFast( Vector_4_In v );
Vector_4_Out V1InvSqrtFastSafe( Vector_4_In v, float errVal=LARGE_FLOAT );

//============================================================================
// Conversion functions

template <int exponent>
Vector_4_Out V1FloatToIntRaw(Vector_4_In inVector);
template <int exponent>
Vector_4_Out V1IntToFloatRaw(Vector_4_In inVector);
Vector_4_Out V1RoundToNearestInt(Vector_4_In inVector);
Vector_4_Out V1RoundToNearestIntZero(Vector_4_In inVector);
Vector_4_Out V1RoundToNearestIntNegInf(Vector_4_In inVector);
Vector_4_Out V1RoundToNearestIntPosInf(Vector_4_In inVector);

//============================================================================
// Comparison functions

unsigned int V1IsBetweenNegAndPosBounds( Vector_4_In testVector, Vector_4_In boundsVector );

unsigned int V1IsZero(Vector_4_In inVector);

unsigned int V1IsEqual(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V1IsNotEqual(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V1IsEqualInt(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V1IsNotEqualInt(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V1IsClose(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
unsigned int V1IsClose(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);

unsigned int V1IsGreaterThan(Vector_4_In bigVector, Vector_4_In smallVector);

unsigned int V1IsGreaterThanOrEqual(Vector_4_In bigVector, Vector_4_In smallVector);

unsigned int V1IsLessThan(Vector_4_In smallVector, Vector_4_In bigVector);

unsigned int V1IsLessThanOrEqual(Vector_4_In smallVector, Vector_4_In bigVector);

Vector_4_Out V1SelectFT(Vector_4_In choiceVector, Vector_4_In zero, Vector_4_In nonZero);
Vector_4_Out V1SelectVect(Vector_4_In choiceVectorX, Vector_4_In zero, Vector_4_In nonZero);

Vector_4_Out V1Max(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V1Min(Vector_4_In inVector1, Vector_4_In inVector2);

Vector_4_Out V1Clamp( Vector_4_In inVector, Vector_4_In lowBound, Vector_4_In highBound );

//============================================================================
// Output

#if !__NO_OUTPUT
void V1Print(Vector_4_In inVector, bool newline=true);
void V1PrintHex(Vector_4_In inVector, bool newline=true);
#endif

//============================================================================
// Bitwise operations

Vector_4_Out V1And(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V1Or(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V1Xor(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V1Andc(Vector_4_In inVector1, Vector_4_In inVector2);

} // namespace Vec
} // namespace rage

#endif // VECTORMATH_V1VECTOR4_H
