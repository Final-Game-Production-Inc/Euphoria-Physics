#ifndef VECTORMATH_V4VECTOR4V_H
#define VECTORMATH_V4VECTOR4V_H

namespace rage
{
namespace Vec
{

//================================================
//	VECTOR(4) VERSIONS (Vector_4V params)
//================================================

// USES VECTOR INSTRUCTIONS ONLY
Vector_4V_Out	V4LoadUnaligned( const void* src );
void			V4StoreUnaligned( void* dst, Vector_4V_In v );

// Similar to V4LoadUnaligned, except that the 16-byte boundary will not be
// crossed if at least NUM_BYTES are within the first 16-bytes.  That is, if
// -(uptr)src&15 >= NUM_BYTES, then only the 16-bytes at (uptr)src&~15 will be
// accessed, else the next 16-bytes will also be accessed.  Any bytes in the
// output vector after the first NUM_BYTES, should be considerred undefined.
template<unsigned NUM_BYTES>
Vector_4V_Out   V4LoadUnalignedSafe( const void* src );

// Load from unaligned pointer to left part of vector.
// Will not access the memory past the 16-byte boundary.
// Undefined values in vector register after the 16-byte boundary.
// Must be from cacheable memory.
Vector_4V_Out   V4LoadLeft( const void* src );

// Like V4LoadLeft, but may also access the next 16-bytes.
// 16-bytes.  Can be a bit faster than V4LoadLeft on some platforms if
// it doesn't cause any additional cache miss (ie x86/64).
Vector_4V_Out   V4LoadLeftUnsafe( const void* src );

Vector_4V_Out	V4LoadScalar32IntoSplatted( const float& scalar );
Vector_4V_Out	V4LoadScalar32IntoSplatted( const u32& scalar );
Vector_4V_Out	V4LoadScalar32IntoSplatted( const s32& scalar );
void			V4StoreScalar32FromSplatted( float& fLoc, Vector_4V_In splattedVec );
void			V4StoreScalar32FromSplatted( u32& uLoc, Vector_4V_In splattedVec );
void			V4StoreScalar32FromSplatted( s32& sLoc, Vector_4V_In splattedVec );

Vector_4V_Out	V4LoadScalar16IntoSplatted( const u16& scalar );
Vector_4V_Out	V4LoadScalar16IntoSplatted( const s16& scalar );
void			V4StoreScalar16FromSplatted( u16& uLoc, Vector_4V_In splattedVec );
void			V4StoreScalar16FromSplatted( s16& sLoc, Vector_4V_In splattedVec );

Vector_4V_Out	V4LoadScalar8IntoSplatted( const u8& scalar );
Vector_4V_Out	V4LoadScalar8IntoSplatted( const s8& scalar );
void			V4StoreScalar8FromSplatted( u8& uLoc, Vector_4V_In splattedVec );
void			V4StoreScalar8FromSplatted( s8& sLoc, Vector_4V_In splattedVec );

// Store 8, 16, 32 or 64-bit value from src to dst.  FROM is the source index if
// src is treated like an array.  Destination pointer must be aligned for the store type.
// eg:
//  V4Store16<2>(dst, src);
// is equivalent to
//  *(u16*)dst = ((u16*)&src)[2];
template<unsigned FROM> void V4Store8 (void *dst, Vector_4V_In src);
template<unsigned FROM> void V4Store16(void *dst, Vector_4V_In src);
template<unsigned FROM> void V4Store32(void *dst, Vector_4V_In src);
template<unsigned FROM> void V4Store64(void *dst, Vector_4V_In src);

// PURPOSE:	Gets the floating point X component of a vector.
// PARAMS:	inVector - the vector of which to grab the X component
// RETURNS:	the float value in the X component of inVector
// NOTES:	Performance warning: this function disrupts the vector pipeline.
float GetX( Vector_4V_ConstRef inVector );

// PURPOSE:	Gets the floating point Y component of a vector.
// PARAMS:	inVector - the vector of which to grab the Y component
// RETURNS:	the float value in the Y component of inVector
// NOTES:	Performance warning: this function disrupts the vector pipeline.
float GetY( Vector_4V_ConstRef inVector );

// PURPOSE:	Gets the floating point Z component of a vector.
// PARAMS:	inVector - the vector of which to grab the Z component
// RETURNS:	the float value in the Z component of inVector
// NOTES:	Performance warning: this function disrupts the vector pipeline.
float GetZ( Vector_4V_ConstRef inVector );

// PURPOSE:	Gets the floating point W component of a vector.
// PARAMS:	inVector - the vector of which to grab the W component
// RETURNS:	the float value in the W component of inVector
// NOTES:	Performance warning: this function disrupts the vector pipeline.
float GetW( Vector_4V_ConstRef inVector );

// Same as above, but uses int pipeline.
int GetXi( Vector_4V_ConstRef inVector );
int GetYi( Vector_4V_ConstRef inVector );
int GetZi( Vector_4V_ConstRef inVector );
int GetWi( Vector_4V_ConstRef inVector );

// PURPOSE:	Gets the floating point (elem)'th component of a vector.
// PARAMS:	inVector - the vector of which to grab the (elem)'th component
//			elem - the element to grab (0 to 3)
// RETURNS:	the float value in the (elem)'th component of inVector
// NOTES:	Performance warning: this function disrupts the vector pipeline.
float GetElem( Vector_4V_ConstRef inVector, unsigned int elem );

// PURPOSE:	Gets a reference to the floating point (elem)'th component of a vector.
// PARAMS:	pInVector - a pointer to the vector of which to grab the (elem)'th component
//			elem - the element to grab (0 to 3)
// RETURNS:	a reference to the float value in the (elem)'th component of *pInVector
// NOTES:	Performance warning: this function disrupts the vector pipeline.
float& GetElemRef( Vector_4V_Ptr pInVector, unsigned int elem );

// PURPOSE:	Gets a const reference to the floating point (elem)'th component of a vector.
// PARAMS:	pInVector - a pointer to the vector of which to grab the (elem)'th component
//			elem - the element to grab (0 to 3)
// RETURNS:	a reference to the float value in the (elem)'th component of *pInVector
// NOTES:	Performance warning: this function disrupts the vector pipeline.
const float& GetElemRef( Vector_4V_ConstPtr pInVector, unsigned int elem );

// PURPOSE:	Splats the X component of a vector.
// PARAMS:	inVector - the vector of which to splat the X component
// RETURNS:	the resulting vector
// NOTES:	NONE
Vector_4V_Out GetXV( Vector_4V_In inVector );

// PURPOSE:	Splats the Y component of a vector.
// PARAMS:	inVector - the vector of which to splat the Y component
// RETURNS:	the resulting vector
// NOTES:	NONE
Vector_4V_Out GetYV( Vector_4V_In inVector );

// PURPOSE:	Splats the Z component of a vector.
// PARAMS:	inVector - the vector of which to splat the Z component
// RETURNS:	the resulting vector
// NOTES:	NONE
Vector_4V_Out GetZV( Vector_4V_In inVector );

// PURPOSE:	Splats the W component of a vector.
// PARAMS:	inVector - the vector of which to splat the W component
// RETURNS:	the resulting vector
// NOTES:	NONE
Vector_4V_Out GetWV( Vector_4V_In inVector );

// PURPOSE:	Sets the floating point X component of a vector.
// PARAMS:	inoutVector - the vector of which to set the X component
//			floatVal - the float value to set the X component to
// RETURNS:	NONE
// NOTES:	This uses the vector pipeline completely, IF 'floatVal' is already in
//			memory (e.g., if the float is a member being accessed from a non-inlined
//			mem func, or a global var, etc.) Otherwise, LHS.
void SetX( Vector_4V_InOut inoutVector, const float& floatVal );

// PURPOSE:	Sets the floating point Y component of a vector.
// PARAMS:	inoutVector - the vector of which to set the Y component
//			floatVal - the float value to set the Y component to
// RETURNS:	NONE
// NOTES:	This uses the vector pipeline completely, IF 'floatVal' is already in
//			memory (e.g., if the float is a member being accessed from a non-inlined
//			mem func, or a global var, etc.) Otherwise, LHS.
void SetY( Vector_4V_InOut inoutVector, const float& floatVal );

// PURPOSE:	Sets the floating point Z component of a vector.
// PARAMS:	inoutVector - the vector of which to set the Z component
//			floatVal - the float value to set the Z component to
// RETURNS:	NONE
// NOTES:	This uses the vector pipeline completely, IF 'floatVal' is already in
//			memory (e.g., if the float is a member being accessed from a non-inlined
//			mem func, or a global var, etc.) Otherwise, LHS.
void SetZ( Vector_4V_InOut inoutVector, const float& floatVal );

// PURPOSE:	Sets the floating point W component of a vector.
// PARAMS:	inoutVector - the vector of which to set the W component
//			floatVal - the float value to set the W component to
// RETURNS:	NONE
// NOTES:	This uses the vector pipeline completely, IF 'floatVal' is already in
//			memory (e.g., if the float is a member being accessed from a non-inlined
//			mem func, or a global var, etc.) Otherwise, LHS.
void SetW( Vector_4V_InOut inoutVector, const float& floatVal );

// PURPOSE:	Sets the unsigned integer X component of a vector.
// PARAMS:	inoutVector - the vector of which to set the X component
//			uintVal - the unsigned integer value to set the X component to
// RETURNS:	NONE
// NOTES:	This uses the vector pipeline completely, IF 'uintVal' is already in
//			memory (e.g., if the u32 is a member being accessed from a non-inlined
//			mem func, or a global var, etc.) Otherwise, LHS.
void SetX( Vector_4V_InOut inoutVector, const u32& uintVal );

// PURPOSE:	Sets the unsigned integer Y component of a vector.
// PARAMS:	inoutVector - the vector of which to set the Y component
//			uintVal - the unsigned integer value to set the Y component to
// RETURNS:	NONE
// NOTES:	This uses the vector pipeline completely, IF 'uintVal' is already in
//			memory (e.g., if the u32 is a member being accessed from a non-inlined
//			mem func, or a global var, etc.) Otherwise, LHS.
void SetY( Vector_4V_InOut inoutVector, const u32& uintVal );

// PURPOSE:	Sets the unsigned integer Z component of a vector.
// PARAMS:	inoutVector - the vector of which to set the Z component
//			uintVal - the unsigned integer value to set the Z component to
// RETURNS:	NONE
// NOTES:	This uses the vector pipeline completely, IF 'uintVal' is already in
//			memory (e.g., if the u32 is a member being accessed from a non-inlined
//			mem func, or a global var, etc.) Otherwise, LHS.
void SetZ( Vector_4V_InOut inoutVector, const u32& uintVal );

// PURPOSE:	Sets the unsigned integer W component of a vector.
// PARAMS:	inoutVector - the vector of which to set the W component
//			uintVal - the unsigned integer value to set the W component to
// RETURNS:	NONE
// NOTES:	This uses the vector pipeline completely, IF 'uintVal' is already in
//			memory (e.g., if the u32 is a member being accessed from a non-inlined
//			mem func, or a global var, etc.) Otherwise, LHS.
void SetW( Vector_4V_InOut inoutVector, const u32& uintVal );

// Same as above, except uses all float pipeline, IF the vector is already in
// memory. Otherwise, LHS.
void SetXInMemory( Vector_4V_InOut inoutVector, float floatVal );
void SetYInMemory( Vector_4V_InOut inoutVector, float floatVal );
void SetZInMemory( Vector_4V_InOut inoutVector, float floatVal );
void SetWInMemory( Vector_4V_InOut inoutVector, float floatVal );

// Same as above, except uses all int pipeline, IF the vector is already in
// memory. Otherwise, LHS.
void SetXInMemory( Vector_4V_InOut inoutVector, int intVal );
void SetYInMemory( Vector_4V_InOut inoutVector, int intVal );
void SetZInMemory( Vector_4V_InOut inoutVector, int intVal );
void SetWInMemory( Vector_4V_InOut inoutVector, int intVal );

// Same as above, except uses all vector pipeline, IF the vector is already in
// memory. Otherwise, possible LHS.
void SetXInMemory( Vector_4V_InOut inoutVector, Vector_4V_In splattedVal );
void SetYInMemory( Vector_4V_InOut inoutVector, Vector_4V_In splattedVal );
void SetZInMemory( Vector_4V_InOut inoutVector, Vector_4V_In splattedVal );
void SetWInMemory( Vector_4V_InOut inoutVector, Vector_4V_In splattedVal );

// PURPOSE:	Splats the X component of a vector.
// PARAMS:	inVector - the vector of which to splat the X component
// RETURNS:	the resulting vector
// NOTES:	NONE
Vector_4V_Out V4SplatX( Vector_4V_In inVector );

// PURPOSE:	Splats the Y component of a vector.
// PARAMS:	inVector - the vector of which to splat the Y component
// RETURNS:	the resulting vector
// NOTES:	NONE
Vector_4V_Out V4SplatY( Vector_4V_In inVector );

// PURPOSE:	Splats the Z component of a vector.
// PARAMS:	inVector - the vector of which to splat the Z component
// RETURNS:	the resulting vector
// NOTES:	NONE
Vector_4V_Out V4SplatZ( Vector_4V_In inVector );

// PURPOSE:	Splats the W component of a vector.
// PARAMS:	inVector - the vector of which to splat the W component
// RETURNS:	the resulting vector
// NOTES:	NONE
Vector_4V_Out V4SplatW( Vector_4V_In inVector );

// PURPOSE:	Sets the four floating point components of a vector.
// PARAMS:	inoutVector - the vector of which to set the components
//			x0 - the float value to set the X component to
//			y0 - the float value to set the Y component to
//			z0 - the float value to set the Z component to
//			w0 - the float value to set the W component to
// RETURNS:	NONE
void V4Set( Vector_4V_InOut inoutVector, const float& x0, const float& y0, const float& z0, const float& w0 );

// PURPOSE:	Sets one vector to another.
// PARAMS:	inoutVector - the vector to set
//			inVector - the vector to set the other vector equal to
// RETURNS:	NONE
// NOTES:	This is really the same as using '='.
void V4Set( Vector_4V_InOut inoutVector, Vector_4V_In inVector );

// PURPOSE:	Sets the four floating point components of a vector to 's'.
// PARAMS:	inoutVector - the vector of which to set the components
//			s - the float value to set the X, Y, Z, and W components to
// RETURNS:	NONE
void V4Set( Vector_4V_InOut inoutVector, const float& s );

void V4Set( Vector_4V_InOut inoutVector, const u32& s );
void V4Set( Vector_4V_InOut inoutVector, const int& s );

// PURPOSE:	Sets the four floating point components of a vector to 0.0f.
// PARAMS:	inoutVector - the vector of which to set the components
// RETURNS:	NONE
// NOTES:	NONE
void V4ZeroComponents( Vector_4V_InOut inoutVector );

// PURPOSE:	Sets the W component of a vector to 0.0f.
// PARAMS:	inoutVector - the vector of which to set the W component
// RETURNS:	NONE
// NOTES:	NONE
void V4SetWZero( Vector_4V_InOut inoutVector );

//============================================================================
// Trigonometry

// PURPOSE:	This function brings any angle down within the [-pi,pi] range by subtracting/adding 2*PI as necessary. It's useful for obtaining valid input
//			for V4SinAndCosFast(), V4SinFast(), and V4CosFast().
// PARAMS:	inVector - the vector of which to bring the 4 angles into the [-pi,pi] range
// RETURNS:	a vector with 4 angles in the [-pi,pi] range
// NOTES:	V4CanonicalizeAngle() --> V4<trig>Fast() is still faster than a normal, more accurate V4<trig>() function call.
Vector_4V_Out V4CanonicalizeAngle( Vector_4V_In inVector );

// PURPOSE:	Computes the sine and cosine of of the angle in each component of the input vector.
// PARAMS:	inOutSine - a reference to the vector which will store the resultant sine values
//			inOutCosine - a reference to the vector which will store the resultant cosine values
//			inVector - the vector with the four input values (in radians)
// RETURNS: NONE
// NOTES:	V4SinAndCos() is faster than using V4Sin() and V4Cos() separately. Hence its existence. However, that is no longer true if the function is not inlined
//			(due to the returns by reference), so for this reason it is force-inlined. If that bloats your code too much, then use V4Sin() and V4Cos()
//			sequentially, instead.
void V4SinAndCos( Vector_4V_InOut inOutSine, Vector_4V_InOut inOutCosine, Vector_4V_In inVector );

// PURPOSE:	Computes the sine of the angle in each component of the input vector.
// PARAMS:	inVector - the vector with the four input values (in radians)
// RETURNS: a vector with the resultant sine values
// NOTES:	NONE
Vector_4V_Out V4Sin( Vector_4V_In inVector );

// PURPOSE:	Computes the cosine of the angle in each component of the input vector.
// PARAMS:	inVector - the vector with the four input values (in radians)
// RETURNS: a vector with the resultant cosine values
// NOTES:	NONE
Vector_4V_Out V4Cos( Vector_4V_In inVector );

// PURPOSE:	Computes the tangent of the angle in each component of the input vector.
// PARAMS:	inVector - the vector with the four input values (in radians)
// RETURNS: a vector with the resultant tangent values
// NOTES:	NONE
Vector_4V_Out V4Tan( Vector_4V_In inVector );

// PURPOSE:	Computes the arcsine of the value in each component of the input vector.
// PARAMS:	inVector - the vector with the four input values
// RETURNS: a vector with the resultant values (in radians)
// NOTES:	Input must be in the valid range [-1,1].
Vector_4V_Out V4Arcsin( Vector_4V_In inVector );

// PURPOSE:	Computes the arccosine of the value in each component of the input vector.
// PARAMS:	inVector - the vector with the four input values
// RETURNS: a vector with the resultant values (in radians)
// NOTES:	Input must be in the valid range [-1,1].
Vector_4V_Out V4Arccos( Vector_4V_In inVector );

// PURPOSE:	Computes the arctangent of the value in each component of the input vector. This function returns a result
//			between [-pi/2,pi/2].
// PARAMS:	inVector - the vector with the four input values
// RETURNS: a vector with the resultant values (in radians)
// NOTES:	NONE
Vector_4V_Out V4Arctan( Vector_4V_In inVector );

// PURPOSE:	Computes the arctangent of the value in each component of the input vector. Given both Y and X, this
//			function may find the correct quadrant of the input, and thus returns a result between [-pi,pi].
// PARAMS:	inVectorY - the vector with four input Y values
//			inVectorX - the vector with four input X values
// RETURNS: a vector with the resultant values (in radians)
// NOTES:	NONE
Vector_4V_Out V4Arctan2( Vector_4V_In inVectorY, Vector_4V_In inVectorX );

// PURPOSE:	Same as V4SinAndCos(), only faster, at the cost of some accuracy. The result is only valid when input is
//			within the range [-pi,pi].
void V4SinAndCosFast( Vector_4V_InOut inOutSine, Vector_4V_InOut inOutCosine, Vector_4V_In inVector );

// PURPOSE:	Same as V4Sin(), only faster, at the cost of some accuracy. The result is only valid when input is
//			within the range [-pi,pi].
Vector_4V_Out V4SinFast( Vector_4V_In inVector );

// PURPOSE:	Same as V4Cos(), only faster, at the cost of some accuracy. The result is only valid when input is
//			within the range [-pi,pi].
Vector_4V_Out V4CosFast( Vector_4V_In inVector );

// PURPOSE:	Same as V4Cos(), only faster, at the cost of some accuracy.
Vector_4V_Out V4TanFast( Vector_4V_In inVector );

// PURPOSE:	Same as V4Arcsin(), only faster, at the cost of some accuracy.
Vector_4V_Out V4ArcsinFast( Vector_4V_In inVector );

// PURPOSE:	Same as V4Arccos(), only faster, at the cost of some accuracy.
Vector_4V_Out V4ArccosFast( Vector_4V_In inVector );

// PURPOSE:	Same as V4Arctan(), only faster, at the cost of some accuracy.
Vector_4V_Out V4ArctanFast( Vector_4V_In inVector );

// PURPOSE:	Same as V4Arctan2(), only faster, at the cost of some accuracy.
Vector_4V_Out V4Arctan2Fast( Vector_4V_In inVectorY, Vector_4V_In inVectorX );

// PURPOSE:	Finds if each component is an even floating point number, after each is rounded using Round Towards Zero rounding mode.
// PARAMS:	inVector - the input vector
// RETURNS: a vector whose components are either 0xFFFFFFFF or 0x0, depending on whether each was or was not even, respectively
// NOTES:	Not very useful, but was used in the implementation of some vectorized trig functions.
Vector_4V_Out V4IsEvenV( Vector_4V_In inVector );

// PURPOSE:	Finds if each component is an odd floating point number, after each is rounded using Round Towards Zero rounding mode.
// PARAMS:	inVector - the input vector
// RETURNS: a vector whose components are either 0xFFFFFFFF or 0x0, depending on whether each was or was not odd, respectively
// NOTES:	Not very useful, but was used in the implementation of some vectorized trig functions.
Vector_4V_Out V4IsOddV( Vector_4V_In inVector );

// PURPOSE:	A "slow in out" transition function.
// PARAMS:	t - the input vector, which should have components between 0.0f and 1.0f (they are clamped to these values within the function)
// RETURNS: a smooth transition curve
// NOTES:	NONE
Vector_4V_Out V4SlowInOut( Vector_4V_In t );

// PURPOSE:	A "slow in" transition function.
// PARAMS:	t - the input vector, which should have components between 0.0f and 1.0f (they are clamped to these values within the function)
// RETURNS: a smooth transition curve
// NOTES:	NONE
Vector_4V_Out V4SlowIn( Vector_4V_In t );

// PURPOSE:	A "slow out" transition function.
// PARAMS:	t - the input vector, which should have components between 0.0f and 1.0f (they are clamped to these values within the function)
// RETURNS: a smooth transition curve
// NOTES:	NONE
Vector_4V_Out V4SlowOut( Vector_4V_In t );

// PURPOSE:	A "bell in out" transition function.
// PARAMS:	t - the input vector, which should have components between 0.0f and 1.0f (they are clamped to these values within the function)
// RETURNS: a smooth transition curve
// NOTES:	NONE
Vector_4V_Out V4BellInOut( Vector_4V_In t );

// PURPOSE:	Performs [(t-lower) / (upper-lower)] for each component of the 't', 'upper', and 'lower' vectors. It is basically a LERP with a
//			transformed domain.
// PARAMS:	t - the input vector, which should typically have components between 0.0f and 1.0f (they are not clamped to these values within
//			the function)
//			lower - the lower end of the LERP domain
//			upper - the upper end of the LERP domain
// RETURNS: a range function (values in between [0.0f,1.0f])
// NOTES:	NONE
Vector_4V_Out V4Range( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper );
Vector_4V_Out V4RangeSafe( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper, Vector_4V_In_After3Args errValVect );

// PURPOSE:	Same as V4Range(), only faster, at the cost of some accuracy.
Vector_4V_Out V4RangeFast( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper );

// PURPOSE:	Same as V4Range(), only clamps 't' to [0.0f,1.0f].
Vector_4V_Out V4RangeClamp( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper );

// PURPOSE:	Same as V4RangeClamp(), only faster, at the cost of some accuracy.
Vector_4V_Out V4RangeClampFast( Vector_4V_In t, Vector_4V_In lower, Vector_4V_In upper );

// PURPOSE:	Produces f(x), which is a piecewise linear function with three sections:
//				For x <= funcInA, f(x) = funcOutA.
//				For x > funcInA and x < funcInB, f(x) ramps from funcOutA to funcOutB
//				For x >= funcInB, f(x) = funcOutB
//			This is basically a RangeClamp(), with a subsequent LERP between two values, funcOutA and funcOutB.
// PARAMS:	x - the input vector
//			funcInA - the lower end of the LERP domain
//			funcInB - the upper end of the LERP domain
//			funcOutA - the lower end of the LERP mapping
//			funcOutB - the upper end of the LERP mapping
// RETURNS: a ramping function (values in between [funcOutA,funcOutB])
// NOTES:	NONE
Vector_4V_Out V4Ramp( Vector_4V_In x, Vector_4V_In funcInA, Vector_4V_In funcInB, Vector_4V_In_After3Args funcOutA, Vector_4V_In_After3Args funcOutB );

// PURPOSE:	Same as V4Ramp(), only faster, at the cost of some accuracy.
Vector_4V_Out V4RampFast( Vector_4V_In x, Vector_4V_In funcInA, Vector_4V_In funcInB, Vector_4V_In_After3Args funcOutA, Vector_4V_In_After3Args funcOutB );

//============================================================================
// Standard Algebra

Vector_4V_Out V4Scale( Vector_4V_In inVector1, Vector_4V_In inVector2 );
Vector_4V_Out V4InvScale( Vector_4V_In inVector1, Vector_4V_In inVector2 );
Vector_4V_Out V4InvScaleSafe( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V4InvScaleFast( Vector_4V_In inVector1, Vector_4V_In inVector2 );
Vector_4V_Out V4InvScaleFastSafe( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

Vector_4V_Out V4Add( Vector_4V_In inVector1, Vector_4V_In inVector2 );
Vector_4V_Out V4AddScaled( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In inVector3 );
Vector_4V_Out V4Subtract( Vector_4V_In inVector1, Vector_4V_In inVector2 );
Vector_4V_Out V4SubtractScaled( Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In inVector3 );

Vector_4V_Out V4AddInt( Vector_4V_In inVector1, Vector_4V_In inVector2 );
Vector_4V_Out V4SubtractInt( Vector_4V_In inVector1, Vector_4V_In inVector2 );

Vector_4V_Out V4Negate(Vector_4V_In inVector);

Vector_4V_Out V4Abs(Vector_4V_In inVector);

Vector_4V_Out V4InvertBits(Vector_4V_In inVector);

Vector_4V_Out V4Invert(Vector_4V_In inVector);
Vector_4V_Out V4InvertSafe(Vector_4V_In inVector, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
Vector_4V_Out V4InvertFast(Vector_4V_In inVector);
Vector_4V_Out V4InvertFastSafe(Vector_4V_In inVector, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

Vector_4V_Out V4Average(Vector_4V_In a, Vector_4V_In b);

Vector_4V_Out V4Lerp( const float& t, Vector_4V_In a, Vector_4V_In b );
Vector_4V_Out V4Lerp( Vector_4V_In t, Vector_4V_In a, Vector_4V_In b );

Vector_4V_Out V4Pow( Vector_4V_In x, Vector_4V_In y );
Vector_4V_Out V4PowPrecise( Vector_4V_In x, Vector_4V_In y );
Vector_4V_Out V4Expt( Vector_4V_In x );
Vector_4V_Out V4Log2( Vector_4V_In x );
Vector_4V_Out V4Log10( Vector_4V_In x );
Vector_4V_Out V4Modulus( Vector_4V_In inVector, Vector_4V_In inMod );

//============================================================================
// Magnitude and distance

float V4Dot(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V4DotV(Vector_4V_In a, Vector_4V_In b);

Vector_4V_Out V4Normalize(Vector_4V_In inVector);
Vector_4V_Out V4NormalizeFast(Vector_4V_In inVector);
Vector_4V_Out V4NormalizeSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold = V4VConstant(V_FLT_SMALL_5));
Vector_4V_Out V4NormalizeFastSafe(Vector_4V_In inVector, Vector_4V_In errValVect, Vector_4V_In magSqThreshold = V4VConstant(V_FLT_SMALL_5));

Vector_4V_Out V4Sqrt( Vector_4V_In v );
Vector_4V_Out V4SqrtSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_ZERO));
Vector_4V_Out V4SqrtFast( Vector_4V_In v );
Vector_4V_Out V4SqrtFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_ZERO) );

Vector_4V_Out V4InvSqrt(Vector_4V_In inVector);
Vector_4V_Out V4InvSqrtSafe(Vector_4V_In inVector, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V4InvSqrtFast(Vector_4V_In inVector);
Vector_4V_Out V4InvSqrtFastSafe(Vector_4V_In inVector, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

float V4Mag( Vector_4V_In v );
float V4MagFast( Vector_4V_In v );
Vector_4V_Out V4MagV( Vector_4V_In v );
Vector_4V_Out V4MagVFast( Vector_4V_In v );

float V4MagSquared( Vector_4V_In v );
Vector_4V_Out V4MagSquaredV( Vector_4V_In v );

float V4InvMag( Vector_4V_In v );
float V4InvMagSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
float V4InvMagFast( Vector_4V_In v );
float V4InvMagFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

Vector_4V_Out V4InvMagV( Vector_4V_In v );
Vector_4V_Out V4InvMagVSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V4InvMagVFast( Vector_4V_In v );
Vector_4V_Out V4InvMagVFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

float V4InvMagSquared( Vector_4V_In v );
float V4InvMagSquaredSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
float V4InvMagSquaredFast( Vector_4V_In v );
float V4InvMagSquaredFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

Vector_4V_Out V4InvMagSquaredV( Vector_4V_In v );
Vector_4V_Out V4InvMagSquaredVSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V4InvMagSquaredVFast( Vector_4V_In v );
Vector_4V_Out V4InvMagSquaredVFastSafe( Vector_4V_In v, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );

float V4Dist(Vector_4V_In a, Vector_4V_In b);
float V4DistFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V4DistV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V4DistVFast(Vector_4V_In a, Vector_4V_In b);

float V4InvDist(Vector_4V_In a, Vector_4V_In b);
float V4InvDistSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
float V4InvDistFast(Vector_4V_In a, Vector_4V_In b);
float V4InvDistFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

Vector_4V_Out V4InvDistV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V4InvDistVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
Vector_4V_Out V4InvDistVFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V4InvDistVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

float V4DistSquared(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V4DistSquaredV(Vector_4V_In a, Vector_4V_In b);

float V4InvDistSquared(Vector_4V_In a, Vector_4V_In b);
float V4InvDistSquaredSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
float V4InvDistSquaredFast(Vector_4V_In a, Vector_4V_In b);
float V4InvDistSquaredFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

Vector_4V_Out V4InvDistSquaredV(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V4InvDistSquaredVSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));
Vector_4V_Out V4InvDistSquaredVFast(Vector_4V_In a, Vector_4V_In b);
Vector_4V_Out V4InvDistSquaredVFastSafe(Vector_4V_In a, Vector_4V_In b, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8));

//============================================================================
// Conversion functions

// Before converting to int, we may multiply by (2.0f^exponent) for free. (Usually use "0".)
// (Free on XBox360/PS3 at least... it's emulated (slow!) in Win32 (exponent==0 is cheap in Win32 though).
// If exponent != 0 on Win32, use exponent == 0 then multiply by V4VConstant<[1/2^exp in hex]>().)
template <int exponent>
Vector_4V_Out V4FloatToIntRaw(Vector_4V_In inVector);
// After converting to float, we may divide by (2.0f^exponent) for free. (Usually use "0".)
// (Free on XBox360/PS3 at least... it's emulated (slow!) in Win32 (exponent==0 is cheap in Win32 though).
// If exponent != 0 on Win32, use exponent == 0 then multiply by V4VConstant<[2^exp in hex]>().)
template <int exponent>
Vector_4V_Out V4IntToFloatRaw(Vector_4V_In inVector);

// Convert 8-bit unsigned integers packed into first 32-bits, four 32-bit floats in the range [0.f .. 1.f].
// Conversion is allowed to be slightly inaccurate and divide by 256 instead of 255 if that is more efficient on the target platform.
Vector_4V_Out V4Uint8ToFloat32(Vector_4V_In inVector);
// Convert four 32-bit floats in the range [0.f .. 1.f] into 8-bit unsigned integers packed into the first 32-bits.
// Remaining 96-bits are undefined.
// Conversion is allowed to be slightly inaccurate and multiply by 256 instead of 255 if that is more efficient on the target platform,
// though it must still clamp so that 1.f -> 0xff.
Vector_4V_In V4Float32ToUint8(Vector_4V_In inVector);

Vector_4V_Out V4RoundToNearestInt(Vector_4V_In inVector);
Vector_4V_Out V4RoundToNearestIntZero(Vector_4V_In inVector);
Vector_4V_Out V4RoundToNearestIntNegInf(Vector_4V_In inVector);
Vector_4V_Out V4RoundToNearestIntPosInf(Vector_4V_In inVector);

Vector_4V_Out V4UnpackLowUnsignedShort(Vector_4V_In inVector);
Vector_4V_Out V4UnpackLowUnsignedByte(Vector_4V_In inVector);
Vector_4V_Out V4UnpackHighUnsignedShort(Vector_4V_In inVector);
Vector_4V_Out V4UnpackHighUnsignedByte(Vector_4V_In inVector);

Vector_4V_Out V4UnpackLowSignedShort(Vector_4V_In inVector);
Vector_4V_Out V4UnpackLowSignedByte(Vector_4V_In inVector);
Vector_4V_Out V4UnpackHighSignedShort(Vector_4V_In inVector);
Vector_4V_Out V4UnpackHighSignedByte(Vector_4V_In inVector);

//	V4PackSignedIntToSignedShort       - __vpkswss / _mm_packs_epi32
//	V4PackSignedShortToSignedByte      - __vpkshss / _mm_packs_epi16
//	V4PackSignedIntToUnsignedShort     - __vpkswus / _mm_packus_epi32 (SSE4, emulated on SSE2)
//	V4PackSignedShortToUnsignedByte    - __vpkshus / _mm_packus_epi16 
//	V4PackUnsignedIntToUnsignedShort   - __vpkuhus / no SSE equivalent
//	V4PackUnsignedShortToUnsignedByte  - __vpkuwus / no SSE equivalent
Vector_4V_Out V4PackSignedIntToSignedShort(Vector_4V_In in0, Vector_4V_In in1);
Vector_4V_Out V4PackSignedIntToUnsignedShort(Vector_4V_In in0, Vector_4V_In in1);
Vector_4V_Out V4PackSignedShortToSignedByte(Vector_4V_In in0, Vector_4V_In in1);
Vector_4V_Out V4PackSignedShortToUnsignedByte(Vector_4V_In in0, Vector_4V_In in1);

// Packs 4 normalized 32-bit floats to 4 16-bit signed integers, stored in the
// first half (X and Y components) of the output vector.  The second half of the
// returned vector (Z and W components) is undefined.  Packs to platform
// specific format that matches GPU, using the most accurate calculations
// possible.
Vector_4V_Out V4PackFloatToGpuSignedShortAccurate(Vector_4V_In in);
Vector_4V_Out V4UnpackLowGpuSignedShortToFloatAccurate(Vector_4V_In in);
Vector_4V_Out V4UnpackHighGpuSignedShortToFloatAccurate(Vector_4V_In in);

// Pack the X,Y,Z components (should each be in the range [-1..+1]) into
// 11:11:10 of the first 32-bits of the output vector.  The rest of the
// returned vector is undefined.
//
// Each input float will be clamped to [-1..+1].
//
// This format is compatable with
//  CELL_GCM_VERTEX_CMP on PS3
//  D3DDECLTYPE_HEND3N on 360, and
//  kBufferFormat10_11_11, kBufferChannelTypeSNorm on Orbis
//
// Note that there is no supported format on DX11 (DXGI_FORMAT_R11G11B10_FLOAT
// is not a good choice for precision).  Use V4PackNormFloats_10_10_10_2
// instead.
//
Vector_4V_Out V4PackNormFloats_11_11_10(Vector_4V_In in);

// Pack the X,Y,Z,W components (should each be in the range [-1..+1]) into
// 10:10:10:2 of the first 32-bits of the output vector.  The input W
// component must be either -1 or +1 (notice that 0 is not supported).  The
// remaining 96-bits of the output vector are undefined.
//
// Each input float will be clamped to [-1..+1].
//
// The exact bit pattern is platform specific, and designed to match a valid
// vertex input format.
//
// This format is compatable with
//  D3DDDECLTYPE_DEC4N on 360
//  DXGI_FORMAT_R10G10B10A2_UNORM on DX11, and
//  kBufferFormat2_10_10_10, kBufferChannelTypeSNorm on Orbis
//
// Does not match any RSX vertex formats on PS3.
//
// Note that when used on DX11 as an input to a shader, then shader must
// apply a bias and scale to retrieve the original value.
//
// For Orbis when used as a shader input, the X, Y and Z components do not need
// any transform, but the W component does need a bias and scale applied.
//
Vector_4V_Out V4PackNormFloats_10_10_10_2(Vector_4V_In in);

// Just like V4PackNormFloats_10_10_10_2, but the output compressed W value is 0.
Vector_4V_Out V4PackNormFloats_10_10_10_X(Vector_4V_In in);

// Treat the first 32-bits of the input vector as 11:11:10 format (X,Y,Z)
// and unpack to floats in the range [-1..+1].  The W component of the
// output vector is undefined.
Vector_4V_Out V4UnpackNormFloats_11_11_10(Vector_4V_In in);

// Treat the first 32-bits of the input vector as 10:10:10:2 format
// (X,Y,Z,W) and unpack to floats in the range [-1..+1].
Vector_4V_Out V4UnpackNormFloats_10_10_10_2(Vector_4V_In in);

// Just like V4UnpackNormFloats_10_10_10_2, but the output is undefined.
Vector_4V_Out V4UnpackNormFloats_10_10_10_X(Vector_4V_In in);

//============================================================================
// Comparison functions

Vector_4V_Out V4IsTrueV(bool b);
Vector_4V_Out V4IsNonZeroV(u32 b);

Vector_4V_Out V4SameSignV(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V4SameSignAll(Vector_4V_In inVector1, Vector_4V_In inVector2);

Vector_4V_Out V4IsZeroV(Vector_4V_In inVector1);
unsigned int V4IsZeroAll(Vector_4V_In inVector);
unsigned int V4IsZeroNone(Vector_4V_In inVector);

unsigned int V4IsBetweenNegAndPosBounds( Vector_4V_In testVector, Vector_4V_In boundsVector );

Vector_4V_Out V4IsEqualV(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V4IsEqualAll(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V4IsEqualNone(Vector_4V_In inVector1, Vector_4V_In inVector2);

Vector_4V_Out V4IsEqualIntV(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V4IsEqualIntAll(Vector_4V_In inVector1, Vector_4V_In inVector2);
unsigned int V4IsEqualIntNone(Vector_4V_In inVector1, Vector_4V_In inVector2);

Vector_4V_Out V4IsCloseV(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);
Vector_4V_Out V4IsNotCloseV(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);
unsigned int V4IsCloseAll(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);
unsigned int V4IsCloseNone(Vector_4V_In inVector1, Vector_4V_In inVector2, Vector_4V_In eps);

unsigned int V4IsGreaterThanAll(Vector_4V_In bigVector, Vector_4V_In smallVector);
Vector_4V_Out V4IsGreaterThanV(Vector_4V_In bigVector, Vector_4V_In smallVector);

Vector_4V_Out V4IsGreaterThanIntV(Vector_4V_In inVector1, Vector_4V_In inVector2);
// TODO POST GTAV: V4IsGreaterThanIntAll, V4IsGreaterThanIntNone

unsigned int V4IsGreaterThanOrEqualAll(Vector_4V_In bigVector, Vector_4V_In smallVector);
Vector_4V_Out V4IsGreaterThanOrEqualV(Vector_4V_In bigVector, Vector_4V_In smallVector);

unsigned int V4IsLessThanAll(Vector_4V_In smallVector, Vector_4V_In bigVector);
Vector_4V_Out V4IsLessThanV(Vector_4V_In smallVector, Vector_4V_In bigVector);

unsigned int V4IsLessThanOrEqualAll(Vector_4V_In smallVector, Vector_4V_In bigVector);
Vector_4V_Out V4IsLessThanOrEqualV(Vector_4V_In smallVector, Vector_4V_In bigVector);

Vector_4V_Out V4SelectFT(Vector_4V_In choiceVector, Vector_4V_In zero, Vector_4V_In nonZero);
Vector_4V_Out V4SelectVect(Vector_4V_In choiceVectorX, Vector_4V_In zero, Vector_4V_In nonZero);

Vector_4V_Out V4Max(Vector_4V_In inVector1, Vector_4V_In inVector2);
Vector_4V_Out V4Min(Vector_4V_In inVector1, Vector_4V_In inVector2);

Vector_4V_Out V4MinElement(Vector_4V_In inVector);
Vector_4V_Out V4MaxElement(Vector_4V_In inVector);

Vector_4V_Out V4Clamp( Vector_4V_In inVector, Vector_4V_In lowBound, Vector_4V_In highBound );
Vector_4V_Out V4Saturate( Vector_4V_In inVector );

//============================================================================
// Quaternions

// Quaternion of the form: q = q3 + i*q0 + j*q1 + k*q2
// Represented as: <x,y,z,w> = <q0,q1,q2,q3>
Vector_4V_Out V4QuatMultiply( Vector_4V_In inQuat1, Vector_4V_In inQuat2 );
Vector_4V_Out V4QuatDotV( Vector_4V_In inQuat1, Vector_4V_In inQuat2 );
float V4QuatDot( Vector_4V_In inQuat1, Vector_4V_In inQuat2 );
Vector_4V_Out V4QuatConjugate( Vector_4V_In inQuat );
Vector_4V_Out V4QuatNormalize( Vector_4V_In inQuat );
Vector_4V_Out V4QuatNormalizeSafe( Vector_4V_In inQuat, Vector_4V_In errValVect, Vector_4V_In magSqThreshold = V4VConstant(V_FLT_SMALL_5) );
Vector_4V_Out V4QuatNormalizeFast( Vector_4V_In inQuat );
Vector_4V_Out V4QuatNormalizeFastSafe( Vector_4V_In inQuat, Vector_4V_In errValVect, Vector_4V_In magSqThreshold = V4VConstant(V_FLT_SMALL_5) );
Vector_4V_Out V4QuatInvert( Vector_4V_In inQuat );
Vector_4V_Out V4QuatInvertSafe( Vector_4V_In inQuat, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
Vector_4V_Out V4QuatInvertFast( Vector_4V_In inQuat );
Vector_4V_Out V4QuatInvertFastSafe( Vector_4V_In inQuat, Vector_4V_In errValVect = V4VConstant(V_FLT_LARGE_8) );
// V4QuatInvertNormInput() is fastest if the input is already normalized. Else, V4QuatInvert() is faster
// than a V4QuatNormalize() followed by a V4QuatInvertNormInput().
Vector_4V_Out V4QuatInvertNormInput( Vector_4V_In inNormQuat );
// t must be a splatted value. Slerps in the shortest direction, but slower than V4QuatSlerp(). Use V4PrepareSlerp(), then can use the faster V4QuatSlerp().
Vector_4V_Out V4QuatSlerpNear( Vector_4V_In t, Vector_4V_In inNormQuat1, Vector_4V_In inNormQuat2 );
// t must be a splatted value. May not slerp in the shortest direction unless preceded by a V4QuatPrepareSlerp().
// [NOTE: There can be numerical inaccuracy between this and the old vector library's SLERP if you do not precede this call with a V4QuatPrepareSlerp()!
// Be warned! (See the commented-out unit test in test_quatv.cpp for the failing compatibility test.]
Vector_4V_Out V4QuatSlerp( Vector_4V_In t, Vector_4V_In inNormQuat1, Vector_4V_In inNormQuat2 );
// Faster, but a non-constant velocity. Still fine if you have a substantial amount of animation keyframes.
// t must be a splatted value.
Vector_4V_Out V4QuatNlerp( Vector_4V_In t, Vector_4V_In inNormQuat1, Vector_4V_In inNormQuat2 );
Vector_4V_Out V4QuatNlerpFast( Vector_4V_In t, Vector_4V_In inNormQuat1, Vector_4V_In inNormQuat2 );
// Only 1st 3 components of normAxis are important.
// radians must be a splatted value.
Vector_4V_Out V4QuatFromAxisAngle( Vector_4V_In normAxis, Vector_4V_In radians );
// radians must be a splatted value.
Vector_4V_Out V4QuatFromXAxisAngle( Vector_4V_In radians );
// radians must be a splatted value.
Vector_4V_Out V4QuatFromYAxisAngle( Vector_4V_In radians );
// radians must be a splatted value.
Vector_4V_Out V4QuatFromZAxisAngle( Vector_4V_In radians );
// returns the angle in radians, splatted.
void V4QuatToAxisAngle( Vector_4V_InOut Axis, Vector_4V_InOut radians, Vector_4V_In inQuat );
// returns the angle in radians, splatted.
Vector_4V_Out V4QuatGetAngle( Vector_4V_In inQuat );
// returns relative angle in radians, splatted.
Vector_4V_Out V4QuatRelAngle( Vector_4V_In inQuat1, Vector_4V_In inQuat2 );
Vector_4V_Out V4QuatFromVectors( Vector_4V_In inVec1, Vector_4V_In inVec2, Vector_4V_In inVec3 );
Vector_4V_Out V4QuatFromVectors( Vector_4V_In inVec1, Vector_4V_In inVec2 );

Vector_4V_Out V4QuatScaleAngle( Vector_4V_In inQuat, Vector_4V_In scale );
Vector_4V_Out V4QuatTwistAngle( Vector_4V_In inQuat, Vector_4V_In v );

// Sets x/y/z = V3Normalize(inQuat.x/y/z).
Vector_4V_Out V4QuatGetUnitDirection( Vector_4V_In inQuat );
Vector_4V_Out V4QuatGetUnitDirectionFast( Vector_4V_In inQuat );
// Sets x/y/z = V3Normalize(inQuat.x/y/z), or (0.0f,1.0f,0.0f) if inQuat.x/y/z == 0.0f.
Vector_4V_Out V4QuatGetUnitDirectionSafe( Vector_4V_In inQuat, Vector_4V_In errValVect = V4VConstant(V_Y_AXIS_WONE) );
Vector_4V_Out V4QuatGetUnitDirectionFastSafe( Vector_4V_In inQuat, Vector_4V_In errValVect = V4VConstant(V_Y_AXIS_WONE) );

// Modifies and returns a new copy of the second quat, making sure the slerp goes the shortest route.
Vector_4V_Out V4QuatPrepareSlerp( Vector_4V_In quat1, Vector_4V_In quatToNegate );

//============================================================================
// Output

#if !__NO_OUTPUT
void V4Print(Vector_4V_In inVector, bool newline=true);
void V4PrintHex(Vector_4V_In inVector, bool newline=true);
#endif

//============================================================================
// Validity

// Checks the values for INF or NAN (which are not "finite").
unsigned int  V4IsFiniteAll( Vector_4V_In inVector );
Vector_4V_Out V4IsFiniteV( Vector_4V_In inVector );
// Checks the values for NAN. This is a bit faster than V4IsFiniteV(), so use it when possible.
unsigned int  V4IsNotNanAll( Vector_4V_In inVector );
Vector_4V_Out V4IsNotNanV( Vector_4V_In inVector );

//============================================================================
// Bitwise operations

// Shift each 32-bit element by bit count
template <int shift>
Vector_4V_Out V4ShiftLeft( Vector_4V_In inVector );
template <int shift>
Vector_4V_Out V4ShiftRight( Vector_4V_In inVector );
template <int shift>
Vector_4V_Out V4ShiftRightAlgebraic( Vector_4V_In inVector );

// Shift entire 128-bit vector by byte count
template <int shift>
Vector_4V_Out V4Shift128LeftBytes( Vector_4V_In inVector );
template <int shift>
Vector_4V_Out V4Shift128RightBytes( Vector_4V_In inVector );

// Shift entire 128-bit vector by byte count, undefined bytes shifted in (more efficient on some platforms)
template <int shift>
Vector_4V_Out V4Shift128LeftBytes_UndefBytes( Vector_4V_In inVector );
template <int shift>
Vector_4V_Out V4Shift128RightBytes_UndefBytes( Vector_4V_In inVector );

Vector_4V_Out V4And(Vector_4V_In inVector1, Vector_4V_In inVector2);
Vector_4V_Out V4Or(Vector_4V_In inVector1, Vector_4V_In inVector2);
Vector_4V_Out V4Xor(Vector_4V_In inVector1, Vector_4V_In inVector2);
Vector_4V_Out V4Andc(Vector_4V_In inVector1, Vector_4V_In inVector2);

// A merge of X/Y or Z/W.
Vector_4V_Out V4MergeXY(Vector_4V_In inVector1, Vector_4V_In inVector2);
Vector_4V_Out V4MergeZW(Vector_4V_In inVector1, Vector_4V_In inVector2);

// A more fine-grained merge of X/Y or Z/W.
Vector_4V_Out V4MergeXYShort(Vector_4V_In inVector1, Vector_4V_In inVector2);
Vector_4V_Out V4MergeZWShort(Vector_4V_In inVector1, Vector_4V_In inVector2);

// An even-more fine-grained merge of X/Y or Z/W.
Vector_4V_Out V4MergeXYByte(Vector_4V_In inVector1, Vector_4V_In inVector2);
Vector_4V_Out V4MergeZWByte(Vector_4V_In inVector1, Vector_4V_In inVector2);

// Use Vec::X, Vec::Y, Vec::Z, Vec::W as template arguments.
template <u32 permX, u32 permY, u32 permZ, u32 permW>
Vector_4V_Out V4Permute( Vector_4V_In v );

// Use Vec::X1, Vec::X2, Vec::Y1, Vec::Y2, Vec::Z1, Vec::Z2, Vec::W1, Vec::W2 as template arguments.
template <u32 permX, u32 permY, u32 permZ, u32 permW>
Vector_4V_Out V4PermuteTwo( Vector_4V_In v1, Vector_4V_In v2 );
Vector_4V_Out V4PermuteTwo( Vector_4V_In v1, Vector_4V_In v2, Vector_4V_In controlVect );

// Byte permutes and run-time permutes. Only available on PS3 and XBox360.
#if __XENON || __PS3

// Use [0-15] as template arguments.
template <u8 perm0, u8 perm1, u8 perm2, u8 perm3, u8 perm4, u8 perm5, u8 perm6, u8 perm7, u8 perm8, u8 perm9, u8 perm10, u8 perm11, u8 perm12, u8 perm13, u8 perm14, u8 perm15>
Vector_4V_Out V4BytePermute( Vector_4V_In v );

// Use [0-31] as template arguments.
template <u8 perm0, u8 perm1, u8 perm2, u8 perm3, u8 perm4, u8 perm5, u8 perm6, u8 perm7, u8 perm8, u8 perm9, u8 perm10, u8 perm11, u8 perm12, u8 perm13, u8 perm14, u8 perm15>
Vector_4V_Out V4BytePermuteTwo( Vector_4V_In v1, Vector_4V_In v2 );

Vector_4V_Out V4Permute( Vector_4V_In v, Vector_4V_In controlVect );

#endif // __XENON || __PS3

} // namespace Vec
} // namespace rage

#endif // VECTORMATH_V4VECTOR4V_H
