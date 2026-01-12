#ifndef VECTORMATH_V4VECTOR4_H
#define VECTORMATH_V4VECTOR4_H

namespace rage
{
namespace Vec
{


//================================================
//	SCALAR(4) VERSIONS (Vector_4 params)
//================================================

Vector_4_Out V4IsFiniteV( Vector_4_In inVector );
Vector_4_Out V4IsNotNanV( Vector_4_In inVector );

float GetElem( Vector_4_In inVector, unsigned int elem );
float& GetElemRef( Vector_4_Ptr pInVector, unsigned int elem );
const float& GetElemRef( Vector_4_ConstPtr pInVector, unsigned int elem );

float GetX( Vector_4_In inVector );
float GetY( Vector_4_In inVector );
float GetZ( Vector_4_In inVector );
float GetW( Vector_4_In inVector );

Vector_4_Out GetXV( Vector_4_In inVector );
Vector_4_Out GetYV( Vector_4_In inVector );
Vector_4_Out GetZV( Vector_4_In inVector );
Vector_4_Out GetWV( Vector_4_In inVector );

// Same as above, but uses int pipeline.
int GetXi( Vector_4_ConstRef inVector );
int GetYi( Vector_4_ConstRef inVector );
int GetZi( Vector_4_ConstRef inVector );
int GetWi( Vector_4_ConstRef inVector );

void SetX( Vector_4_InOut inoutVector, float floatVal );
void SetY( Vector_4_InOut inoutVector, float floatVal );
void SetZ( Vector_4_InOut inoutVector, float floatVal );
void SetW( Vector_4_InOut inoutVector, float floatVal );

// Same as above, except uses all int pipeline.
void SetXInMemory( Vector_4_InOut inoutVector, int intVal );
void SetYInMemory( Vector_4_InOut inoutVector, int intVal );
void SetZInMemory( Vector_4_InOut inoutVector, int intVal );
void SetWInMemory( Vector_4_InOut inoutVector, int intVal );

Vector_4_Out V4SplatX( Vector_4_In inVector );
Vector_4_Out V4SplatY( Vector_4_In inVector );
Vector_4_Out V4SplatZ( Vector_4_In inVector );
Vector_4_Out V4SplatW( Vector_4_In inVector );

void V4Set( Vector_4_InOut inoutVector, const float& x0, const float& y0, const float& z0, const float& w0 );
void V4Set( Vector_4_InOut inoutVector, Vector_4_In inVector );
void V4Set( Vector_4_InOut inoutVector, const float& s );
void V4Set( Vector_4_InOut inoutVector, const u32& s );
void V4Set( Vector_4_InOut inoutVector, const int& s );

void V4ZeroComponents( Vector_4_InOut inoutVector );

void V4SetWZero( Vector_4_InOut inoutVector );

// These just available as fallbacks for vectorized versions.
#if !UNIQUE_VECTORIZED_TYPE
void SetXInMemory( Vector_4_InOut inoutVector, float floatVal );
void SetYInMemory( Vector_4_InOut inoutVector, float floatVal );
void SetZInMemory( Vector_4_InOut inoutVector, float floatVal );
void SetWInMemory( Vector_4_InOut inoutVector, float floatVal );
Vector_4_Out V4LoadScalar32IntoSplatted( const float& scalar );
Vector_4_Out V4LoadScalar32IntoSplatted( const u32& scalar );
Vector_4_Out V4LoadScalar32IntoSplatted( const int& scalar );
void V4StoreScalar32FromSplatted( float& fLoc, Vector_4_In splattedVec );
void V4StoreScalar32FromSplatted( u32& fLoc, Vector_4_In splattedVec );
void V4StoreScalar32FromSplatted( int& fLoc, Vector_4_In splattedVec );
Vector_4_Out V4LoadUnaligned( const void* ptr );
#endif

//============================================================================
// Standard Algebra

Vector_4_Out V4Scale( Vector_4_In inVector, float floatVal );
Vector_4_Out V4Scale( Vector_4_In inVector1, Vector_4_In inVector2 );//; // __attribute__((noinline));
void V4Scale( Vector_4_InOut c, Vector_4_In inVector1, Vector_4_In inVector2 );//; // __attribute__((noinline));

Vector_4_Out V4InvScale( Vector_4_In inVector, float floatVal );
Vector_4_Out V4InvScale( Vector_4_In inVector, Vector_4_In floatVal );
Vector_4_Out V4InvScaleSafe( Vector_4_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V4InvScaleSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V4InvScaleFast( Vector_4_In inVector, float floatVal );
Vector_4_Out V4InvScaleFast( Vector_4_In inVector, Vector_4_In floatVal );
Vector_4_Out V4InvScaleFastSafe( Vector_4_In inVector, float floatVal, float errVal = LARGE_FLOAT );
Vector_4_Out V4InvScaleFastSafe( Vector_4_In inVector, Vector_4_In floatVal, float errVal = LARGE_FLOAT );

Vector_4_Out V4Add( Vector_4_In inVector, float sx, float sy, float sz, float w0 );
Vector_4_Out V4Add( Vector_4_In inVector1, Vector_4_In inVector2 );				

Vector_4_Out V4AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue );
Vector_4_Out V4AddScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue );

Vector_4_Out V4Subtract( Vector_4_In inVector, float sx, float sy, float sz );
Vector_4_Out V4Subtract( Vector_4_In inVector1, Vector_4_In inVector2 );

Vector_4_Out V4AddInt( Vector_4_In inVector1, Vector_4_In inVector2 );
Vector_4_Out V4SubtractInt( Vector_4_In inVector1, Vector_4_In inVector2 );

Vector_4_Out V4SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, float floatValue );
Vector_4_Out V4SubtractScaled( Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In floatValue );

Vector_4_Out V4Negate( Vector_4_In inVector );

Vector_4_Out V4Abs( Vector_4_In inVector );

Vector_4_Out V4InvertBits( Vector_4_In inVector );

Vector_4_Out V4Invert( Vector_4_In inVector);
Vector_4_Out V4InvertSafe( Vector_4_In inVector, float errVal = LARGE_FLOAT );
Vector_4_Out V4InvertFast( Vector_4_In inVector);
Vector_4_Out V4InvertFastSafe( Vector_4_In inVector, float errVal = LARGE_FLOAT );

Vector_4_Out V4Average( Vector_4_In a, Vector_4_In b );

Vector_4_Out V4Lerp( float t, Vector_4_In a, Vector_4_In b );
Vector_4_Out V4Lerp( Vector_4_In t, Vector_4_In a, Vector_4_In b );

Vector_4_Out V4Pow( Vector_4_In x, Vector_4_In y );
Vector_4_Out V4Expt( Vector_4_In x );
Vector_4_Out V4Log2( Vector_4_In x );
Vector_4_Out V4Log10( Vector_4_In x );
Vector_4_Out V4Modulus( Vector_4_In inVector, Vector_4_In inMod );

//============================================================================
// Magnitude and distance

float V4Dot(Vector_4_In a, Vector_4_In b);
Vector_4_Out V4DotV(Vector_4_In a, Vector_4_In b);

Vector_4_Out V4Normalize(Vector_4_In inVector);
Vector_4_Out V4NormalizeFast(Vector_4_In inVector);
Vector_4_Out V4NormalizeSafe(Vector_4_In inVector, float errVal, float magSqThreshold = 1e-5f);
Vector_4_Out V4NormalizeFastSafe(Vector_4_In inVector, float errVal, float magSqThreshold = 1e-5f);

Vector_4_Out V4Sqrt( Vector_4_In v );
Vector_4_Out V4SqrtSafe( Vector_4_In v, float errVal = 0.0f );
Vector_4_Out V4SqrtFast( Vector_4_In v );
Vector_4_Out V4SqrtFastSafe( Vector_4_In v, float errVal = 0.0f );

Vector_4_Out V4InvSqrt(Vector_4_In inVector);
Vector_4_Out V4InvSqrtSafe(Vector_4_In inVector, float errVal = LARGE_FLOAT);
Vector_4_Out V4InvSqrtFast(Vector_4_In inVector);
Vector_4_Out V4InvSqrtFastSafe(Vector_4_In inVector, float errVal = LARGE_FLOAT);

float V4Mag( Vector_4_In v );
float V4MagFast( Vector_4_In v );
Vector_4_Out V4MagV( Vector_4_In v );
Vector_4_Out V4MagVFast( Vector_4_In v );

float V4MagSquared( Vector_4_In v );
Vector_4_Out V4MagSquaredV( Vector_4_In v );

float V4InvMag( Vector_4_In v );	
float V4InvMagSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
float V4InvMagFast( Vector_4_In v );
float V4InvMagFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

Vector_4_Out V4InvMagV( Vector_4_In v );
Vector_4_Out V4InvMagVSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
Vector_4_Out V4InvMagVFast( Vector_4_In v );
Vector_4_Out V4InvMagVFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

float V4InvMagSquared( Vector_4_In v );
float V4InvMagSquaredSafe( Vector_4_In v );
float V4InvMagSquaredFast( Vector_4_In v );
float V4InvMagSquaredFastSafe( Vector_4_In v );

Vector_4_Out V4InvMagSquaredV( Vector_4_In v );
Vector_4_Out V4InvMagSquaredVSafe( Vector_4_In v, float errVal = LARGE_FLOAT );
Vector_4_Out V4InvMagSquaredVFast( Vector_4_In v );
Vector_4_Out V4InvMagSquaredVFastSafe( Vector_4_In v, float errVal = LARGE_FLOAT );

float V4Dist(Vector_4_In a, Vector_4_In b);
float V4DistFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V4DistV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V4DistVFast(Vector_4_In a, Vector_4_In b);

float V4InvDist(Vector_4_In a, Vector_4_In b);
float V4InvDistSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
float V4InvDistFast(Vector_4_In a, Vector_4_In b);
float V4InvDistFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

Vector_4_Out V4InvDistV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V4InvDistVSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
Vector_4_Out V4InvDistVFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V4InvDistVFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

float V4DistSquared(Vector_4_In a, Vector_4_In b);
Vector_4_Out V4DistSquaredV(Vector_4_In a, Vector_4_In b);

float V4InvDistSquared(Vector_4_In a, Vector_4_In b);
float V4InvDistSquaredSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
float V4InvDistSquaredFast(Vector_4_In a, Vector_4_In b);
float V4InvDistSquaredFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

Vector_4_Out V4InvDistSquaredV(Vector_4_In a, Vector_4_In b);
Vector_4_Out V4InvDistSquaredVSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);
Vector_4_Out V4InvDistSquaredVFast(Vector_4_In a, Vector_4_In b);
Vector_4_Out V4InvDistSquaredVFastSafe(Vector_4_In a, Vector_4_In b, float errVal = LARGE_FLOAT);

//============================================================================
// Conversion functions

template <int exponent>
Vector_4_Out V4FloatToIntRaw(Vector_4_In inVector);
template <int exponent>
Vector_4_Out V4IntToFloatRaw(Vector_4_In inVector);
Vector_4_Out V4RoundToNearestInt(Vector_4_In inVector);
Vector_4_Out V4RoundToNearestIntZero(Vector_4_In inVector);
Vector_4_Out V4RoundToNearestIntNegInf(Vector_4_In inVector);
Vector_4_Out V4RoundToNearestIntPosInf(Vector_4_In inVector);

//============================================================================
// Trigonometry

// This function brings any angle down within the [-pi,pi] range by subtracting/adding 2*PI as necessary. Useful for obtaining valid input
// for V4SinAndCosFast(), V4SinFast(), and V4CosFast().
// NOTES
//	Note that V4CanonicalizeAngle() & V4<trig>Fast() is still faster than a normal V4<trig>();
Vector_4_Out V4CanonicalizeAngle( Vector_4_In inVector );

void V4SinAndCos( Vector_4_InOut inOutSine, Vector_4_InOut inOutCosine, Vector_4_In inVector );
Vector_4_Out V4Sin( Vector_4_In inVector );
Vector_4_Out V4Cos( Vector_4_In inVector );
Vector_4_Out V4Tan( Vector_4_In inVector );
Vector_4_Out V4Arcsin( Vector_4_In inVector );
Vector_4_Out V4Arccos( Vector_4_In inVector );
Vector_4_Out V4Arctan( Vector_4_In inVector );
Vector_4_Out V4Arctan2( Vector_4_In inVectorY, Vector_4_In inVectorX );

void V4SinAndCosFast( Vector_4_InOut inOutSine, Vector_4_InOut inOutCosine, Vector_4_In inVector );
Vector_4_Out V4SinFast( Vector_4_In inVector );
Vector_4_Out V4CosFast( Vector_4_In inVector );
Vector_4_Out V4TanFast( Vector_4_In inVector );
Vector_4_Out V4ArcsinFast( Vector_4_In inVector );
Vector_4_Out V4ArccosFast( Vector_4_In inVector );
Vector_4_Out V4ArctanFast( Vector_4_In inVector );
Vector_4_Out V4Arctan2Fast( Vector_4_In inVectorY, Vector_4_In inVectorX );

Vector_4_Out V4IsEvenV( Vector_4_In inVector );
Vector_4_Out V4IsOddV( Vector_4_In inVector );

Vector_4_Out V4SlowInOut( Vector_4_In t );
Vector_4_Out V4SlowIn( Vector_4_In t );
Vector_4_Out V4SlowOut( Vector_4_In t );
Vector_4_Out V4BellInOut( Vector_4_In t );
Vector_4_Out V4Range( Vector_4_In t, Vector_4_In lower, Vector_4_In upper );
Vector_4_Out V4RangeSafe( Vector_4_In t, Vector_4_In lower, Vector_4_In upper, Vector_4_In errValVect );
Vector_4_Out V4RangeFast( Vector_4_In t, Vector_4_In lower, Vector_4_In upper );
Vector_4_Out V4RangeClamp( Vector_4_In t, Vector_4_In lower, Vector_4_In upper );
Vector_4_Out V4RangeClampFast( Vector_4_In t, Vector_4_In lower, Vector_4_In upper );

//   Returns f(x), which is a piecewise linear function with three sections:
//     For x <= funcInA, f(x) = funcOutA.
//     for x > funcInA and x < funcInB, f(x) ramps from funcOutA to funcOutB
//     for x >= funcInB, f(x) = funcOutB
Vector_4_Out V4Ramp( Vector_4_In x, Vector_4_In funcInA, Vector_4_In funcInB, Vector_4_In funcOutA, Vector_4_In funcOutB );
Vector_4_Out V4RampFast( Vector_4_In x, Vector_4_In funcInA, Vector_4_In funcInB, Vector_4_In funcOutA, Vector_4_In funcOutB );

//============================================================================
// Quaternions

// Quaternion of the form: q = q3 + i*q0 + j*q1 + k*q2
// Represented as: <x,y,z,w> = <q0,q1,q2,q3>
Vector_4_Out V4QuatDotV( Vector_4_In inQuat1, Vector_4_In inQuat2 );
float V4QuatDot( Vector_4_In inQuat1, Vector_4_In inQuat2 );
Vector_4_Out V4QuatMultiply( Vector_4_In inQuat1, Vector_4_In inQuat2 );
Vector_4_Out V4QuatConjugate( Vector_4_In inQuat );
Vector_4_Out V4QuatNormalize( Vector_4_In inQuat );
Vector_4_Out V4QuatNormalizeSafe( Vector_4_In inQuat, float errVal, float magSqThreshold = 1e-5f );
Vector_4_Out V4QuatNormalizeFast( Vector_4_In inQuat );
Vector_4_Out V4QuatNormalizeFastSafe( Vector_4_In inQuat, float errVal, float magSqThreshold = 1e-5f );
Vector_4_Out V4QuatInvert( Vector_4_In inQuat );
Vector_4_Out V4QuatInvertSafe( Vector_4_In inQuat, float errVal = LARGE_FLOAT );
Vector_4_Out V4QuatInvertFast( Vector_4_In inQuat );
Vector_4_Out V4QuatInvertFastSafe( Vector_4_In inQuat, float errVal = LARGE_FLOAT );
// V4QuatInvertNormInput() is fastest if the input is already normalized. Else, V4QuatInvert() is faster
// than a V4QuatNormalize() followed by a V4QuatInvertNormInput().
Vector_4_Out V4QuatInvertNormInput( Vector_4_In inNormQuat );
Vector_4_Out V4QuatSlerpNear( float t, Vector_4_In inNormQuat1, Vector_4_In inNormQuat2 );
// May not slerp in the shortest direction unless preceded by a V4QuatPrepareSlerp().
Vector_4_Out V4QuatSlerp( float t, Vector_4_In inNormQuat1, Vector_4_In inNormQuat2 );
// Faster, but a non-constant velocity. Still fine if you have a substantial amount of animation keyframes.
Vector_4_Out V4QuatNlerp( float t, Vector_4_In inNormQuat1, Vector_4_In inNormQuat2 );
// Only 1st 3 components of normAxis are important.
Vector_4_Out V4QuatFromAxisAngle( Vector_4_In normAxis, float radians );
Vector_4_Out V4QuatFromAxisAngle( Vector_3_In normAxis, float radians );
// Only 1st component of radians is important.
Vector_4_Out V4QuatFromXAxisAngle( float radians );
// Only 1st component of radians is important.
Vector_4_Out V4QuatFromYAxisAngle( float radians );
// Only 1st component of radians is important.
Vector_4_Out V4QuatFromZAxisAngle( float radians );

void V4QuatToAxisAnglef( Vector_3_InOut axis, float& radians, Vector_4_In inQuat );
void V4QuatToAxisAnglef( Vector_4_InOut axis, float& radians, Vector_4_In inQuat );
float V4QuatGetAnglef( Vector_4_In inQuat );
// These two are just for the vectorized backup. Use the abiove two for scalar float functionality.
void V4QuatToAxisAngle( Vector_4_InOut axis, Vector_4_InOut radians, Vector_4_In inQuat );
Vector_4_Out V4QuatGetAngle( Vector_4_In inQuat );
Vector_4_Out V4QuatRelAngle( Vector_4_In inQuat1, Vector_4_In inQuat2 );
Vector_4_Out V4QuatFromVectors( Vector_4_In inVec1, Vector_4_In inVec2, Vector_4_In inVec3 );
Vector_4_Out V4QuatFromVectors( Vector_4_In inVec1, Vector_4_In inVec2 );

Vector_4_Out V4QuatScaleAngle( Vector_4_In inQuat, float scale );
float V4QuatTwistAnglef( Vector_4_In inQuat, Vector_4_In v );

// Sets x/y/z = V3Normalize(inQuat.x/y/z).
Vector_4_Out V4QuatGetUnitDirection( Vector_4_In inQuat );
Vector_4_Out V4QuatGetUnitDirectionFast( Vector_4_In inQuat );
// Sets x/y/z = V3Normalize(inQuat.x/y/z), or (0.0f,1.0f,0.0f) if inQuat.x/y/z == 0.0f.
Vector_4_Out V4QuatGetUnitDirectionSafe( Vector_4_In inQuat, Vector_4_In errValVect = V4Constant(V_Y_AXIS_WONE) );
Vector_4_Out V4QuatGetUnitDirectionFastSafe( Vector_4_In inQuat, Vector_4_In errValVect = V4Constant(V_Y_AXIS_WONE) );

// Modifies the first quat, making sure the slerp goes the shortest route.
Vector_4_Out V4QuatPrepareSlerp( Vector_4_In quat1, Vector_4_In quatToNegate );

//============================================================================
// Comparison functions

unsigned int V4IsBetweenNegAndPosBounds( Vector_4_In testVector, Vector_4_In boundsVector );

Vector_4_Out V4SameSignV(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V4SameSignAll(Vector_4_In inVector1, Vector_4_In inVector2);

unsigned int V4IsZeroAll(Vector_4_In inVector);
unsigned int V4IsZeroNone(Vector_4_In inVector);

Vector_4_Out V4IsEqualV(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V4IsEqualAll(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V4IsEqualNone(Vector_4_In inVector1, Vector_4_In inVector2);

Vector_4_Out V4IsEqualIntV(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V4IsEqualIntAll(Vector_4_In inVector1, Vector_4_In inVector2);
unsigned int V4IsEqualIntNone(Vector_4_In inVector1, Vector_4_In inVector2);

Vector_4_Out V4IsCloseV(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
Vector_4_Out V4IsCloseV(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);
Vector_4_Out V4IsNotCloseV(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
Vector_4_Out V4IsNotCloseV(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);
unsigned int V4IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
unsigned int V4IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, float eps);
unsigned int V4IsCloseAll(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);
unsigned int V4IsCloseNone(Vector_4_In inVector1, Vector_4_In inVector2, Vector_4_In eps);

unsigned int V4IsGreaterThanAll(Vector_4_In bigVector, Vector_4_In smallVector);
Vector_4_Out V4IsGreaterThanV(Vector_4_In bigVector, Vector_4_In smallVector);

unsigned int V4IsGreaterThanOrEqualAll(Vector_4_In bigVector, Vector_4_In smallVector);
Vector_4_Out V4IsGreaterThanOrEqualV(Vector_4_In bigVector, Vector_4_In smallVector);

unsigned int V4IsLessThanAll(Vector_4_In smallVector, Vector_4_In bigVector);
Vector_4_Out V4IsLessThanV(Vector_4_In smallVector, Vector_4_In bigVector);

unsigned int V4IsLessThanOrEqualAll(Vector_4_In smallVector, Vector_4_In bigVector);
Vector_4_Out V4IsLessThanOrEqualV(Vector_4_In smallVector, Vector_4_In bigVector);

Vector_4_Out V4SelectFT(Vector_4_In choiceVector, Vector_4_In zero, Vector_4_In nonZero);
Vector_4_Out V4SelectVect(Vector_4_In choiceVectorX, Vector_4_In zero, Vector_4_In nonZero);

Vector_4_Out V4Max(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V4Min(Vector_4_In inVector1, Vector_4_In inVector2);

Vector_4_Out V4MinElement(Vector_4_In inVector);
Vector_4_Out V4MaxElement(Vector_4_In inVector);

Vector_4_Out V4Clamp( Vector_4_In inVector, Vector_4_In lowBound, Vector_4_In highBound );
Vector_4_Out V4Saturate( Vector_4_In inVector );

//============================================================================
// Output

#if !__NO_OUTPUT
void V4Print(Vector_4_In inVector, bool newline=true);
void V4PrintHex(Vector_4_In inVector, bool newline=true);
#endif

//============================================================================
// Bitwise operations

template <int shift>
Vector_4_Out V4ShiftLeft( Vector_4_In inVector );
template <int shift>
Vector_4_Out V4ShiftRight( Vector_4_In inVector );
template <int shift>
Vector_4_Out V4ShiftRightAlgebraic( Vector_4_In inVector );

Vector_4_Out V4And(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V4Or(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V4Xor(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V4Andc(Vector_4_In inVector1, Vector_4_In inVector2);

Vector_4_Out V4MergeXY(Vector_4_In inVector1, Vector_4_In inVector2);
Vector_4_Out V4MergeZW(Vector_4_In inVector1, Vector_4_In inVector2);

// PURPOSE: Pack a vector into 10, 10, 10, 2 format
void V4Pack1010102( u32& inoutInteger, Vector_4_In inVector );

// PURPOSE: Unpack a vector from 10, 10, 10, 2 format
void V4Unpack1010102( Vector_4_InOut inoutVector, u32 packed );

template <u32 permX, u32 permY, u32 permZ, u32 permW>
Vector_4_Out V4Permute( Vector_4_In v );

template <u32 permX, u32 permY, u32 permZ, u32 permW>
Vector_4_Out V4PermuteTwo( Vector_4_In v1, Vector_4_In v2 );

// Note: These two functions are only meant to be a backup for the vectorized versions. They are horribly inefficient (why would you "permute"
// a scalar vector or two anyways using a cryptic control vector?)
Vector_4_Out V4Permute( Vector_4_In v, Vector_4_In controlVect );
Vector_4_Out V4PermuteTwo( Vector_4_In v1, Vector_4_In v2, Vector_4_In controlVect );

// Note: These two functions are also only meant to be a backup for the vectorized versions.
template <u8 perm0, u8 perm1, u8 perm2, u8 perm3, u8 perm4, u8 perm5, u8 perm6, u8 perm7, u8 perm8, u8 perm9, u8 perm10, u8 perm11, u8 perm12, u8 perm13, u8 perm14, u8 perm15>
Vector_4_Out V4BytePermute( Vector_4_In v );
template <u8 perm0, u8 perm1, u8 perm2, u8 perm3, u8 perm4, u8 perm5, u8 perm6, u8 perm7, u8 perm8, u8 perm9, u8 perm10, u8 perm11, u8 perm12, u8 perm13, u8 perm14, u8 perm15>
Vector_4_Out V4BytePermuteTwo( Vector_4_In v1, Vector_4_In v2 );

} // namespace Vec
} // namespace rage


#endif // VECTORMATH_V4VECTOR4_H
