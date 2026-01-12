#ifndef VECTORMATH_V1VECTOR4V_H
#define VECTORMATH_V1VECTOR4V_H

namespace rage
{
namespace Vec
{

//================================================
//	VECTOR(1) VERSIONS (Vector_4V params)
//================================================

void V1Set( Vector_4V_InOut inoutVector, float x0 );

//============================================================================
// Comparison functions

unsigned int V1IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector );

unsigned int V1IsZero(Vector_4V_In inVector);

unsigned int V1IsEqual(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V1IsNotEqual(Vector_4V_In inVector1, Vector_4V_In inVector2);

unsigned int V1IsEqualInt(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V1IsNotEqualInt(Vector_4V_In inVector1, Vector_4V_In inVector2);

unsigned int V1IsClose(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);

unsigned int V1IsGreaterThan(Vector_4V_In bigVector, Vector_4V_In smallVector);

unsigned int V1IsGreaterThanOrEqual(Vector_4V_In bigVector, Vector_4V_In smallVector);

unsigned int V1IsLessThan(Vector_4V_In smallVector, Vector_4V_In bigVector);

unsigned int V1IsLessThanOrEqual(Vector_4V_In smallVector, Vector_4V_In bigVector);


//============================================================================
// Output

#if !__NO_OUTPUT
void V1Print(Vector_4V_In inVector, bool newline=true);
void V1PrintHex(Vector_4V_In inVector, bool newline=true);
#endif

} // namespace Vec
} // namespace rage

#endif // VECTORMATH_V1VECTOR4V_H
