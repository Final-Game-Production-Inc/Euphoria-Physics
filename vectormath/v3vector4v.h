#ifndef VECTORMATH_V3VECTOR4V_H
#define VECTORMATH_V3VECTOR4V_H

namespace rage
{
namespace Vec
{

//================================================
//	VECTOR(3) VERSIONS (Vector_4V params)
//================================================

void V3Set( Vector_4V_InOut inoutVector, float x0, float y0, float z0 );

//============================================================================
// Standard Algebra

// Note that, although all V3* functions have an undefined result for w, this function will set w = 0.0f as long as it was originally a valid float.
// It happens to be a free side effect.
Vector_4V_Out V3Cross(Vector_4V_In a, Vector_4V_In b);

Vector_4V_Out V3AddCrossed(Vector_4V_In toAddTo, Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3SubtractCrossed(Vector_4V_In toSubtractFrom, Vector_4V_In a, Vector_4V_In b);

// PURPOSE:	Combines the least-significant bits of the X/Y/Z components into the 3 LSBs of an integer. 
// NOTES:	Incurs one (necessary) LHS.
void V3ResultToIndexZYX( u32& outInt, Vector_4V_In maskVector );
// PURPOSE:	Combines the least-significant bits of the X/Y/Z components into the 3 LSBs of an integer. 
// NOTES:	Incurs one (necessary) LHS.
void V3ResultToIndexXYZ( u32& outInt, Vector_4V_In maskVector );

//////////////////////////////////////
// ANGULAR ADDITIONS HERE
//////////////////////////////////////

Vector_4V_Out V3AddNet( Vector_4V_In inVector, Vector_4V_In toAdd );
// The amount is assumed to be in each of amount.x/y/z.
Vector_4V_Out V3Extend( Vector_4V_In inVector, Vector_4V_In amount );
Vector_4V_Out V3AngleV(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V3AngleVNormInput(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V3AngleXV(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V3AngleYV(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V3AngleZV(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V3AngleXVNormInput(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V3AngleYVNormInput(Vector_4V_In v1, Vector_4V_In v2);
Vector_4V_Out V3AngleZVNormInput(Vector_4V_In v1, Vector_4V_In v2);
// rate and time should be one value splatted into .x/.y/.z
Vector_4V_Out V3ApproachStraight(Vector_4V_In position, Vector_4V_In goal, Vector_4V_In rate, Vector_4V_In_After3Args time, unsigned int& rResult);
void V3MakeOrthonormals(Vector_4V_In inVector, Vector_4V_InOut ortho1, Vector_4V_InOut ortho2);
// assumes inVector and planeNormal are normalized.
Vector_4V_Out V3Reflect( Vector_4V_In inVector, Vector_4V_In planeNormal );
// assumes radians has the same value splatted in .x/.y/.z.
Vector_4V_Out V3RotateAboutXAxis( Vector_4V_In inVector, Vector_4V_In radians );
Vector_4V_Out V3RotateAboutYAxis( Vector_4V_In inVector, Vector_4V_In radians );
Vector_4V_Out V3RotateAboutZAxis( Vector_4V_In inVector, Vector_4V_In radians );
Vector_4V_Out V3RotateAboutXAxis( Vector_4V_In inVector, Vector_4V_In sine, Vector_4V_In cosine );
Vector_4V_Out V3RotateAboutYAxis( Vector_4V_In inVector, Vector_4V_In sine, Vector_4V_In cosine );
Vector_4V_Out V3RotateAboutZAxis( Vector_4V_In inVector, Vector_4V_In sine, Vector_4V_In cosine );


//////////////////////////////////////
// ANGULAR ADDITIONS....
//////////////////////////////////////

//============================================================================
// Magnitude and distance

float V3Dot(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3DotV(Vector_4V_In a, Vector_4V_In b);

Vector_4V_Out V3Normalize(Vector_4V_In inVector);
Vector_4V_Out V3NormalizeFast(Vector_4V_In inVector);
Vector_4V_Out V3NormalizeSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold = V4VConstant(V_FLT_SMALL_5));
Vector_4V_Out V3NormalizeFastSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold = V4VConstant(V_FLT_SMALL_5));

float V3Mag( Vector_4V_In v );
float V3MagFast( Vector_4V_In v );
Vector_4V_Out V3MagV( Vector_4V_In v );
Vector_4V_Out V3MagVFast( Vector_4V_In v );

Vector_4V_Out V3ClampMag( Vector_4V_In v, Vector_4V_In minMagSplatted, Vector_4V_In maxMagSplatted );
// This version does the exact same thing as the more-conservative RAGE version. This should be
// phased out eventually, but I'm scared to atm.
Vector_4V_Out V3ClampMagOld( Vector_4V_In v, Vector_4V_In minMagSplatted, Vector_4V_In maxMagSplatted );

Vector_4V_Out V3MagXYSquaredV( Vector_4V_In v );
Vector_4V_Out V3MagXZSquaredV( Vector_4V_In v );
Vector_4V_Out V3MagYZSquaredV( Vector_4V_In v );

Vector_4V_Out V3MagXYV( Vector_4V_In v );
Vector_4V_Out V3MagXZV( Vector_4V_In v );
Vector_4V_Out V3MagYZV( Vector_4V_In v );
Vector_4V_Out V3MagXYVFast( Vector_4V_In v );
Vector_4V_Out V3MagXZVFast( Vector_4V_In v );
Vector_4V_Out V3MagYZVFast( Vector_4V_In v );

Vector_4V_Out V3DistXYV( Vector_4V_In a, Vector_4V_In b );
Vector_4V_Out V3DistXZV( Vector_4V_In a, Vector_4V_In b );
Vector_4V_Out V3DistYZV( Vector_4V_In a, Vector_4V_In b );
Vector_4V_Out V3DistXYVFast( Vector_4V_In a, Vector_4V_In b );
Vector_4V_Out V3DistXZVFast( Vector_4V_In a, Vector_4V_In b );
Vector_4V_Out V3DistYZVFast( Vector_4V_In a, Vector_4V_In b );

float V3MagSquared( Vector_4V_In v );
Vector_4V_Out V3MagSquaredV( Vector_4V_In v );

float V3InvMag( Vector_4V_In v );
float V3InvMagSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
float V3InvMagFast( Vector_4V_In v );
float V3InvMagFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

Vector_4V_Out V3InvMagV( Vector_4V_In v );
Vector_4V_Out V3InvMagVSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V3InvMagVFast( Vector_4V_In v );
Vector_4V_Out V3InvMagVFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

float V3InvMagSquared( Vector_4V_In v );
float V3InvMagSquaredSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
float V3InvMagSquaredFast( Vector_4V_In v );
float V3InvMagSquaredFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

Vector_4V_Out V3InvMagSquaredV( Vector_4V_In v );
Vector_4V_Out V3InvMagSquaredVSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V3InvMagSquaredVFast( Vector_4V_In v );
Vector_4V_Out V3InvMagSquaredVFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

float V3Dist(Vector_4V_In a, Vector_4V_In b);
float V3DistFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3DistV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3DistVFast(Vector_4V_In a, Vector_4V_In b);

float V3InvDist(Vector_4V_In a, Vector_4V_In b);
float V3InvDistSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
float V3InvDistFast(Vector_4V_In a, Vector_4V_In b);
float V3InvDistFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

Vector_4V_Out V3InvDistV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3InvDistVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
Vector_4V_Out V3InvDistVFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3InvDistVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

float V3DistSquared(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3DistSquaredV(Vector_4V_In a, Vector_4V_In b);

float V3InvDistSquared(Vector_4V_In a, Vector_4V_In b);
float V3InvDistSquaredSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
float V3InvDistSquaredFast(Vector_4V_In a, Vector_4V_In b);
float V3InvDistSquaredFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

Vector_4V_Out V3InvDistSquaredV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3InvDistSquaredVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
Vector_4V_Out V3InvDistSquaredVFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V3InvDistSquaredVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

//============================================================================
// Comparison functions

unsigned int V3SameSignAll(Vector_4V_In inVector1, Vector_4V_In inVector2);

unsigned int V3IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector );

unsigned int V3IsZeroAll(Vector_4V_In inVector);
unsigned int V3IsZeroNone(Vector_4V_In inVector);

unsigned int V3IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V3IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2);

unsigned int V3IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V3IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2);

unsigned int V3IsCloseAll(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);
unsigned int V3IsCloseNone(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);

unsigned int V3IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector);

unsigned int V3IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector);

unsigned int V3IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector);

unsigned int V3IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector);

Vector_4V_Out V3MinElement(Vector_4V_In inVector);
Vector_4V_Out V3MaxElement(Vector_4V_In inVector);

//============================================================================
// Quaternions

// Quaternion of the form: q = q3 + i*q0 + j*q1 + k*q2
// Represented as: <x,y,z,w> = <q0,q1,q2,q3>

Vector_4V_Out V3QuatRotate( Vector_4V_In inVect, Vector_4V_In inQuat );
Vector_4V_Out V3QuatRotateReverse( Vector_4V_In inVect, Vector_4V_In inQuat );

//============================================================================
// Output

#if !__NO_OUTPUT
void V3Print(Vector_4V_In inVector, bool newline=true);
void V3Print(Vector_4V_In inVector, const char * label, bool newline=true);
void V3PrintHex(Vector_4V_In inVector, bool newline=true);
#endif

//============================================================================
// Validity

// Checks the values for INF or NAN (which are not "finite").
unsigned int  V3IsFiniteAll( Vector_4V_In inVector );
// Checks the values for NAN. This is a bit faster than V3IsFiniteAll(), so use it when possible.
unsigned int  V3IsNotNanAll( Vector_4V_In inVector );

} // namespace Vec
} // namespace rage

#endif // VECTORMATH_V3VECTOR4V_H
