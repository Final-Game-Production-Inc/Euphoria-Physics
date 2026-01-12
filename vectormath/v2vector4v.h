#ifndef VECTORMATH_V2VECTOR4V_H
#define VECTORMATH_V2VECTOR4V_H

namespace rage
{
namespace Vec
{

//================================================
//	VECTOR(2) VERSIONS (Vector_4V params)
//================================================

void V2Set( Vector_4V_InOut inoutVector, float x0, float y0 );

//============================================================================
// Standard Algebra

float V2Cross(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2CrossV(Vector_4V_In a, Vector_4V_In b);

//////////////////////////////////////
// ANGULAR ADDITIONS HERE
//////////////////////////////////////

Vector_4V_Out V2AngleV(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V2AngleVNormInput(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V2WhichSideOfLineV(Vector_4V_In point, Vector_4V_In lineP1, Vector_4V_In lineP2);
// The amount, in radians, is assumed to be at least in both .x and .y components.
Vector_4V_Out V2Rotate(Vector_4V_In inVector, Vector_4V_In radians);
Vector_4V_Out V2AddNet( Vector_4V_In inVector, Vector_4V_In toAdd );
Vector_4V_Out V2Extend( Vector_4V_In inVector, Vector_4V_In amount );

// rate and time should be one value splatted into .x/.y
Vector_4V_Out V2ApproachStraight(Vector_4V_In position, Vector_4V_In goal, Vector_4V_In rate, Vector_4V_In_After3Args time, unsigned int& rResult);

// assumes inVector and planeNormal are normalized.
Vector_4V_Out V2Reflect( Vector_4V_In inVector, Vector_4V_In wall2DNormal );

//////////////////////////////////////
// ANGULAR ADDITIONS....
//////////////////////////////////////

//============================================================================
// Magnitude and distance

float V2Dot(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2DotV(Vector_4V_In a, Vector_4V_In b);

Vector_4V_Out V2Normalize(Vector_4V_In inVector);
Vector_4V_Out V2NormalizeFast(Vector_4V_In inVector);
Vector_4V_Out V2NormalizeSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold = V4VConstant(V_FLT_SMALL_5));
Vector_4V_Out V2NormalizeFastSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold = V4VConstant(V_FLT_SMALL_5));

float V2Mag( Vector_4V_In v );
float V2MagFast( Vector_4V_In v );
Vector_4V_Out V2MagV( Vector_4V_In v );
Vector_4V_Out V2MagVFast( Vector_4V_In v );

float V2MagSquared( Vector_4V_In v );
Vector_4V_Out V2MagSquaredV( Vector_4V_In v );

float V2InvMag( Vector_4V_In v );
float V2InvMagSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
float V2InvMagFast( Vector_4V_In v );
float V2InvMagFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

Vector_4V_Out V2InvMagV( Vector_4V_In v );
Vector_4V_Out V2InvMagVSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V2InvMagVFast( Vector_4V_In v );
Vector_4V_Out V2InvMagVFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

float V2InvMagSquared( Vector_4V_In v );
float V2InvMagSquaredSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
float V2InvMagSquaredFast( Vector_4V_In v );
float V2InvMagSquaredFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

Vector_4V_Out V2InvMagSquaredV( Vector_4V_In v );
Vector_4V_Out V2InvMagSquaredVSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V2InvMagSquaredVFast( Vector_4V_In v );
Vector_4V_Out V2InvMagSquaredVFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

float V2Dist(Vector_4V_In a, Vector_4V_In b);
float V2DistFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2DistV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2DistVFast(Vector_4V_In a, Vector_4V_In b);

float V2InvDist(Vector_4V_In a, Vector_4V_In b);
float V2InvDistSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
float V2InvDistFast(Vector_4V_In a, Vector_4V_In b);
float V2InvDistFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

Vector_4V_Out V2InvDistV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2InvDistVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
Vector_4V_Out V2InvDistVFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2InvDistVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

float V2DistSquared(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2DistSquaredV(Vector_4V_In a, Vector_4V_In b);

float V2InvDistSquared(Vector_4V_In a, Vector_4V_In b);
float V2InvDistSquaredSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
float V2InvDistSquaredFast(Vector_4V_In a, Vector_4V_In b);
float V2InvDistSquaredFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

Vector_4V_Out V2InvDistSquaredV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2InvDistSquaredVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
Vector_4V_Out V2InvDistSquaredVFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V2InvDistSquaredVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

//============================================================================
// Comparison functions

unsigned int V2SameSignAll(Vector_4V_In inVector1, Vector_4V_In inVector2);

unsigned int V2IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector );

unsigned int V2IsZeroAll(Vector_4V_In inVector);
unsigned int V2IsZeroNone(Vector_4V_In inVector);

unsigned int V2IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V2IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2);

unsigned int V2IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V2IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2);

unsigned int V2IsCloseAll(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);
unsigned int V2IsCloseNone(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);

unsigned int V2IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector);

unsigned int V2IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector);

unsigned int V2IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector);

unsigned int V2IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector);

Vector_4V_Out V2MinElement(Vector_4V_In inVector);
Vector_4V_Out V2MaxElement(Vector_4V_In inVector);


//============================================================================
// Output

#if !__NO_OUTPUT
void V2Print(Vector_4V_In inVector, bool newline=true);
void V2PrintHex(Vector_4V_In inVector, bool newline=true);
#endif

//============================================================================
// Validity

// Checks the values for INF or NAN (which are not "finite").
unsigned int V2IsFiniteAll( Vector_4V_In inVector );
// Checks the values for NAN. This is a bit faster than V2IsFiniteAll(), so use it when possible.
unsigned int V2IsNotNanAll( Vector_4V_In inVector );

} // namespace Vec
} // namespace rage

#endif // VECTORMATH_V2VECTOR4V_H
