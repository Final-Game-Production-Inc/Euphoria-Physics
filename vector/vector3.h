//
// vector/vector3.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_VECTOR3_H
#define VECTOR_VECTOR3_H

#include "vector2.h"
#include "vector3_config.h"

#include "math/intrinsics.h"
#include "math/nan.h"
#if __XENON || __PS3
#include "math/altivec.h"
#endif

#if __SPU
#include "vector/vector3_consts_spu.cpp"
#endif

#include "vectormath/vectortypes.h"

#include "system/endian.h"

namespace rage {

class Matrix34;
class Matrix33;

#if !__OPTIMIZED && VECTORIZED
#define Vec3CheckAlignment()	FastAssert(((size_t)this&15)==0)
#else
#define Vec3CheckAlignment()
#endif

#if !__OPTIMIZED && VECTORIZED
template<class Val>
inline Val vector_cast( void* ptr) { mthAssertf(((size_t)ptr&15)==0,"A vector needs to be 16 bytes aligned so this operation is incorrect"); return reinterpret_cast<Val>(ptr); }

#else
template<class Val>
inline Val vector_cast( void* ptr) { return reinterpret_cast<Val>(ptr); }
#endif

// xenon doesn't seem to want to pass return values for vector3 by register, so make sure they are inlined.
#if __XENON || __PPU
#define VEC3_INLINE __forceinline
#else
#define VEC3_INLINE inline
#endif

// PURPOSE: Set the unit up direction.
// PARAMS:
//	unitUp -	the new unit up direction
// NOTES:
//	1.	The default unit up direction is YAXIS.
//	2.	g_UnitUp can be directly changed; this method is provided just to simplify searching code for setting g_UnitUp.
extern void SetUnitUp (const Vector3& unitUp);


//=============================================================================
// Vector3
//
// PURPOSE
//   Vector3 represents a vector in three-dimensional space.  The values in
//   dimensions are represented by float member variables: {x, y, z}
// NOTES
//   - Unlike most other classes in Rage, Vector3 does not store its member
//     variables as "protected" data.  This is for convenience due to the
//     widespread and frequent use of direct access to the data.
// <FLAG Component>
//
class VECTOR_ALIGN Vector3
{
public:
	// PURPOSE: Define the type of a constant Vector3 that we pass to an inline function.
#if (__XENON || __PS3) && VECTORIZED 
    typedef const __vector4 Vector3Param;		// Only way to get the Xenon compiler to pass by register
    typedef __vector4& Vector3Ref;
    typedef const __vector4 Param;		// Only way to get the Xenon compiler to pass by register
    typedef __vector4& Ref;
	typedef __vector4 Return;
#else
    typedef const Vector3 &Vector3Param;		// Win32 seems to optimize this better.
    typedef Vector3& Vector3Ref;
    typedef const Vector3& Param;		// Win32 seems to optimize this better.
    typedef Vector3& Ref;
	typedef Vector3 Return;
#endif

	// PURPOSE: Make zero'ing a vector easy, like Vector3 v(Vector3::ZeroType).
	enum _ZeroType		{ ZeroType };

	//=========================================================================
	// Construction

	// PURPOSE: Default constructor
	// NOTES If __INIT_NAN is defined, the default constructor initializes all components to NaN.
	Vector3();

	// PURPOSE: Zero'ing constructor
	Vector3( _ZeroType );

	// PURPOSE: Constructor taking initial values for each component.
	Vector3(float setX, float setY, float setZ);

	// PURPOSE: Constructor taking initial values for each component.
	Vector3(float setX, float setY, float setZ, float setW);

	// PURPOSE: Constructor taking another vector to set initial value to.
	Vector3(const Vector3 &vec);

	// PURPOSE: For the new vec lib scalar fallback option, SCALAR_TYPES_ONLY == 1 in vectorconfig.h.
	Vector3(const Vec::Vector_4& vec)
		: x(vec.x), y(vec.y), z(vec.z), w(vec.w)
	{
	}

	// PURPOSE: Resource constructor, for vectors created from a resource
	// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
	Vector3(class datResource&);

	// PURPOSE: Construct a Vector3 from a Vector2 using the axes specified using the Vector2::eVector3Axes enum.
	// SEE ALSO: Vector2(Vector3,eVector3Axes) and GetVector2??.
	explicit Vector3(const Vector2 & v2d, Vector2::eVector3Axes axes);

#if VECTORIZED_PADDING
	// PURPOSE: Initializes a vector3 from the 'unused' w components of the three passed in vectors
	Vector3(Vector3Param vecX, Vector3Param vecY, Vector3Param vecZ);
#endif

#if RSG_CPU_INTEL && !defined(_NO_INTRINSICS_)
	// PURPOSE: Construct a Vector3 from a platform specific data type
	Vector3(const __m128& set);
#elif __XENON || __PS3 || __PSP2
	// PURPOSE: Construct a Vector3 from a platform specific data type
	Vector3(const __vector4& set);
#endif	// __XENON

	DECLARE_DUMMY_PLACE(Vector3);

	//=========================================================================
	// Accessors

	// PURPOSE: Access the i-th component of the vector as if it was an array.
	// PARAMS
	//   i - The array index of the component to access.  x is index 0, y is index 1, and z is index 2.
	const float& operator[](int i) const;

	// <COMBINE Vector3::[]>
	float& operator[](int i);

	// PURPOSE: Returns the x component of this vector.
	float GetX() const;

	// PURPOSE: Returns the y component of this vector.
	float GetY() const;

	// PURPOSE: Returns the z component of this vector.
	float GetZ() const;

	// PURPOSE: Set the x component of this vector.
	// PARAMS
	//    f - The new value for the x component.
	void SetX(float f);

	// PURPOSE: Set the y component of this vector.
	// PARAMS
	//    f - The new value for the y component.
	void SetY(float f);

	// PURPOSE: Set the z component of this vector.
	// PARAMS
	//    f - The new value for the z component.
	void SetZ(float f);

#if VECTORIZED_PADDING
	// PURPOSE: Returns the w component of this vector.
	float GetW() const;

	// PURPOSE: Returns the w component of this vector as an unsigned int.
	//			For reusing the space for other member of classes.
	unsigned int GetWAsUnsignedInt() const;

	// PURPOSE: Set the w component of this vector.
	// PARAMS
	//    f - The new value for the z component.
	void SetW(float f);

	// PURPOSE: Set the w component of this vector as an unsigned int by punning it into the memory for the w component.
	//			For reusing the space for other member of classes.
	// PARAMS
	//    u - The new value for the w component.
	void SetWAsUnsignedInt(unsigned int u);
#endif // VECTORIZED

	// PURPOSE: Get vertical component of this vector (default is the y component).
	// NOTES:
	//	The vertical component is the dot product with g_UnitUp, which is settable and defaults to YAXIS.
	float GetUp();

	// PURPOSE: Get vertical component of this vector (default is the y component).
	// NOTES:
	//	The vertical component is the component along g_UnitUp, which is settable and defaults to YAXIS.
	Vector3 GetUpV();

	// PURPOSE: Get the horizontal part of this vector (default is all but the component along y).
	// NOTES:
	//	The non-horizontal component is the part along g_UnitUp, which is settable and defaults to YAXIS.
	Vector3 GetHorizontalV();

	// PURPOSE: Get the squared magnitude of the horizontal component.
	// NOTES:
	//	The non-horizontal component is the part along g_UnitUp, which is settable and defaults to YAXIS.
	float GetHorizontalMag2();

	// PURPOSE: Get the magnitude of the horizontal component.
	// NOTES:
	//	The non-horizontal component is the part along g_UnitUp, which is settable and defaults to YAXIS.
	float GetHorizontalMag();

	// PURPOSE: Set all the components of this vector equal to the x component of the input vector
	// PARAMS
	//    in - The vector whos x value is to be replicated across all components of this vector
	// SEE ALSO:
	//		SplatY, SplatZ, SplatW
	void SplatX(Vector3Param in);

	// PURPOSE: Set all the components of this vector equal to the y component of the input vector
	// PARAMS
	//    in - The vector whos y value is to be replicated across all components of this vector
	// SEE ALSO:
	//		SplatX, SplatZ, SplatW
	void SplatY(Vector3Param in);

	// PURPOSE: Set all the components of this vector equal to the z component of the input vector
	// PARAMS
	//    in - The vector whos z value is to be replicated across all components of this vector
	// SEE ALSO:
	//		SplatY, SplatX, SplatW
	void SplatZ(Vector3Param in);

	// PURPOSE: Set all the components of this vector equal to the w component of the input vector
	// PARAMS
	//    in - The vector whos w value is to be replicated across all components of this vector
	// SEE ALSO:
	//		SplatY, SplatZ, SplatX
	// NOTES:
	//		This is only implemented when vectorized padding is enabled
#if VECTORIZED_PADDING
	void SplatW(Vector3Param in);
#endif

	// PURPOSE: Set this vector's value from three floats.
	// PARAMS
	//   sx - The new x component.
	//   sy - The new y component.
	//   sz - The new z component.
	void Set(float sx,float sy,float sz);

	// PURPOSE: Set this vector's value from another vector.
	// PARAMS
	//   a - The source vector, containing this vector's new value.
	void Set(const Vector3 &a);

	// PURPOSE: Set a Vector3's value from a Vector2.
	// PARAMS
	//   a - The source Vector2, containing the Vector3's new value.
	// NOTES
	//   - The Vector3's x receives the Vector2's x value, but z is set to the Vector2's y value,
	//     and y gets set to zero.  This is not the reverse of GetVector2(); be careful.
	void SetXZ(const Vector2 &a);

	// PURPOSE: Set all components of a vector to the same value.
	// PARAMS
	//   s - The new value for all the vector's components.
	void Set(float s);

	// PURPOSE: Set a vector to the scaled value of another vector.
	// PARAMS
	//   a - The source vector.
	//   s - The factor by which every component in a gets multiplied to produce the final vector.
	void SetScaled(Vector3Param a, float s);

	// PURPOSE: Set a vector to the scaled value of another vector.
	// PARAMS
	//   a - The source vector.
	//   s - The factor by which every component in a gets multiplied to produce the final vector.
	void SetScaled(Vector3Param a, Vector3Param s);

	// PURPOSE: Set a vector to the origin.
	void Zero();

	// PURPOSE: Pack a vector into 10, 10, 10, 2 format
	u32 Pack1010102() const;

	// PURPOSE: Unpack a vector from 10, 10, 10, 2 format
	void Unpack1010102(u32 packed);


	//=========================================================================
	// Vector2 access

	// PURPOSE
	//  Get the Vector2 from a Vector3 in the plane perpendicular to the given axis.
	// PARAMS
	//  axis - The axis to use (0=x+, 1=x-, 2=y+, 3=y-, 4=z+, 5=z-).
	//  vec - The Vector2 created as a result.
	void GetVector2(int axis, Vector2 & outVec) const;

	// PURPOSE:
	//   Copy this vectors contents into a Vector2. The last two letters of the function name
	//	 specify the components that will be copied into the x and y components of outVec.
	// PARAMS
	//   outVec - The Vector2 whose value gets set from the current vector.
	void GetVector2XY(Vector2 & outVec) const;

	// <COMBINE GetVector2XY>
	void GetVector2YX(Vector2 & outVec) const;

	// <COMBINE GetVector2XY>
	void GetVector2XZ(Vector2 & outVec)	const;

	// <COMBINE GetVector2XY>
	void GetVector2ZX(Vector2 & outVec) const;

	// <COMBINE GetVector2XY>
	void GetVector2YZ(Vector2 & outVec)	const;

	// <COMBINE GetVector2XY>
	void GetVector2ZY(Vector2 & outVec) const;


	//=========================================================================
	// Standard algebra

	// PURPOSE: Scale (multiply) each component of this vector by a float.
	// PARAMS
	//   f - The value to scale the vector by.
	void Scale(float f);

	// PURPOSE: Set this vector to the value of another vector scaled by a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The value to scale the vector by.
	void Scale(Vector3Param a, float f);

	// PURPOSE: Scale a vector by the inverse of a value.
	// PARAMS
	//   f - The value to invert and then scale the vector by.
	void InvScale(float f);

	// PURPOSE: Scale a vector by the inverse of a value.
	// PARAMS
	//   f - The vector to invert and then scale the vector by.
	void InvScale(Vector3Param f);

	// PURPOSE: Set the current vector to the value of another vector scaled by the inverse of a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The value to invert and then scale the vector by.
	void InvScale(Vector3Param a, float f);

	// PURPOSE: Set the current vector to the value of another vector scaled by the inverse of a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The vector to invert and then scale the vector by.
	void InvScale(Vector3Param a, Vector3Param f);

	// PURPOSE: Add distinct real numbers to the components of this vector.
	// PARAMS
	//   sx - The value to be added to the x component.
	//   sy - The value to be added to the y component.
	//   sz - The value to be added to the z component.
	void Add(float sx, float sy, float sz);

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	void Add(Vector3Param a);

	// PURPOSE: Set this vector to the result of adding two vectors together.
	// PARAMS
	//   a - The first vector to add.
	//   b - The second vector to add.
	void Add(Vector3Param a, Vector3Param b);

	// PURPOSE: Add the value of another vector, scaled by a value, to the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before adding it to the current vector.
	// SEE ALSO: SubtractScaled, Lerp
	void AddScaled(Vector3Param a, float s);

	// PURPOSE: Add the value of another vector, scaled by a value, to the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before adding it to the current vector.
	// SEE ALSO: SubtractScaled, Lerp
	void AddScaled(Vector3Param a, Vector3Param s);

	// PURPOSE
	//   Set the current vector to the sum of two other vectors, where one of the other two
	//   vectors is scaled first before adding.
	// PARAMS
	//   a - The vector that gets added without getting scaled first.
	//   b - The vector that gets scaled before being added to the other vector.
	//   s - The factor by which to scale the second vector before adding it to the first vector.
	// SEE ALSO: SubtractScaled,Lerp
	void AddScaled(Vector3Param a, Vector3Param b, float s);

	// PURPOSE
	//   Set the current vector to the sum of two other vectors, where one of the other two
	//   vectors is scaled first before adding.
	// PARAMS
	//   a - The vector that gets added without getting scaled first.
	//   b - The vector that gets scaled before being added to the other vector.
	//   s - The factor by which to scale the second vector before adding it to the first vector.
	// SEE ALSO: SubtractScaled,Lerp
	void AddScaled(Vector3Param a, Vector3Param b, Vector3Param s);

	// PURPOSE: Subtract distinct real numbers from the components of this vector.
	// PARAMS
	//   sx - The value to be added to the x component.
	//   sy - The value to be added to the y component.
	//   sz - The value to be added to the z component.
	void Subtract(float sx, float sy, float sz);

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	void Subtract(Vector3Param a);

	// PURPOSE: Set the current vector to the difference between two other vectors.
	// PARAMS
	//   a - The minuend (i.e. the value that gets subtracted from).
	//   b - The subtrahend (i.e. the value that gets subtracted).
	void Subtract(Vector3Param a, Vector3Param b);

	// PURPOSE: Subtract the value of another vector, scaled by a value, from the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before subtracting it from the current vector.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(Vector3Param a, float s);

	// PURPOSE: Subtract the value of another vector, scaled by a value, from the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before subtracting it from the current vector.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(Vector3Param a, Vector3Param s);

	// PURPOSE
	//   Set the current vector to the difference between two other vectors, where the subtrahend 
	//   is first scaled before subtracting it from the minuend.
	// PARAMS
	//   a - The minuend (i.e. the vector that gets subtracted from without getting scaled first).
	//   b - The subtrahend (i.e. the vector that gets scaled before being subtracted from the other vector).
	//   s - The factor by which to scale the subtrahend before subtracting it from the minuend.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(Vector3Param a, Vector3Param b,float s);

	// PURPOSE
	//   Set the current vector to the difference between two other vectors, where the subtrahend 
	//   is first scaled before subtracting it from the minuend.
	// PARAMS
	//   a - The minuend (i.e. the vector that gets subtracted from without getting scaled first).
	//   b - The subtrahend (i.e. the vector that gets scaled before being subtracted from the other vector).
	//   s - The factor by which to scale the subtrahend before subtracting it from the minuend.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(Vector3Param a, Vector3Param b,Vector3Param s);

	// PURPOSE: Multiply the current vector with another vector.
	// PARAMS
	//   a - The vector to multiply with the current vector.
	// NOTES
	//  - This just multiplies the components, i.e. (a,b,c)*(d,e,f) == (ad,be,cf).
	void Multiply(Vector3Param a);

	// PURPOSE: Set the current vector to the product of two other vectors.
	// PARAMS
	//   a - One of the vectors to multiply together.
	//   b - One of the vectors to multiply together.
	// NOTES
	//  - This just multiplies the components, i.e. (a,b,c)*(d,e,f) == (ad,be,cf).
	void Multiply(Vector3Param a, Vector3Param b);

	// PURPOSE: Negate the current vector.
	void Negate();

	// PURPOSE: Set the current vector to the negation of another vector.
	// PARAMS
	//   a - The vector to negate.
	void Negate(Vector3Param a);

	// PURPOSE: Set the components of a vector to their absolute value.
	void Abs();

	// PURPOSE: Set a vector to the absolute value of another vector.
	// PARAMS
	//   a - The vector whose absolute value must be calculated.
#if __XENON || __PS3
	void Abs(Vector3Param a);
#else
	void Abs(const Vector3& a);
#endif

	// PURPOSE: Invert a vector, i.e. set each component to 1 divided by the component.
	// NOTES
	//   - This method may potentially divide by zero.  Use InvertSafe() to avoid division by zero.
	// SEE ALSO: InvertSafe
	void Invert();

	// PURPOSE
	//   Set a vector to the inverse of another vector, i.e. where each new component is 1
	//   divided by the old component.
	// PARAMS
	//   a - The vector to invert.
	// NOTES
	//   - This method may potentially divide by zero.  Use InvertSafe() to avoid division by zero.
	// SEE ALSO: InvertSafe
	void Invert(Vector3Param a);

	// PURPOSE: Invert a vector, i.e. set each component to 1 divided by the component.
	// NOTES
	//   - This method does extra work to avoid dividing by zero.  Use Invert() if potentially dividing
	//     by zero is OK.
	// SEE ALSO: Invert
	void InvertSafe();

	// PURPOSE: Set a vector to the inverse of another vector, i.e. where each new component is 1 divided by the old component.
	// PARAMS
	//	a - The vector to invert.
	//	zeroInverse - optional vector with components to use when the corresponding component to invert is zero
	// NOTES
	//   - This method does extra work to avoid dividing by zero.  Use Invert() if potentially dividing
	//     by zero is OK.
	// SEE ALSO: Invert
	void InvertSafe(Vector3Param a, Vector3Param zeroInverse=Vector3(FLT_MAX,FLT_MAX,FLT_MAX));

	// PURPOSE: Normalize a vector, i.e. scale it so that its new magnitude is 1.
	void Normalize();

	// PURPOSE: Normalize a vector using a reciprocal sqrt estimate, i.e. scale it so that its new magnitude is 1.
	void NormalizeFast();

	// PURPOSE: Normalize a vector, i.e. scale it so that its new magnitude is 1.
	// PARAMS:
	//	fallbackSafeVec - optional default value, used in case the magnitude of this vector is nearly zero
	//	mag2Limit - optional lower limit on the squared magnitude of this vector, below which to use the default unit vector
	// NOTES If the vector is currently near-zero, then return set to fallbackVec instead.
	void NormalizeSafe();
	void NormalizeSafe(Vector3Param fallbackSafeVec);
	void NormalizeSafe(Vector3Param fallbackSafeVec, float mag2Limit);

	// PURPOSE: Normalize a vector, i.e. scale it so that its new magnitude is 1, return whether it was normalized.
	// PARAMS:
	//	fallbackSafeVec - optional default value, used in case the magnitude of this vector is nearly zero
	//	mag2Limit - optional lower limit on the squared magnitude of this vector, below which to use the default unit vector
	// RETURNS: True if the vector was normalized, false if the fallback was used.
	// NOTES If the vector is currently near-zero, then return set to fallbackVec instead.
	bool NormalizeSafeRet();

	// faster version of NormalizeSafe
	void NormalizeSafeV(Vector3Param fallbackSafeVec=Vector3(1.0f,0.0f,0.0f), Vector3Param mag2Limit=Vector3(1.0e-5f, 1.0e-5f, 1.0e-5f));

	// PURPOSE: Set the current vector to the normalization of another vector, i.e. scaled so that
	//    its new magnitude is 1.
	// PARAMS
	//   a - The vector to normalize.
	void Normalize(Vector3Param a);

	// PURPOSE: Set the current vector to the normalization of another vector, i.e. scaled so that
	//    its new magnitude is 1.
	// PARAMS
	//   a - The vector to normalize.
	// NOTES: This uses a reciprocal sqrt estimate
	void NormalizeFast(Vector3Param a);

	// PURPOSE: Calculate the dot product of this vector and another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: the dot product.
	// SEE ALSO: XZDot
	float Dot(const Vector3& a) const;

	// PURPOSE: Calculate the dot product of this vector and another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: a vector with the dot product in all components
	// SEE ALSO: XZDot
	Vector3 DotV(Vector3Param a) const;

	// PURPOSE: Calculate the dot product of this vector and another vector put the result in the current vector
	// PARAMS
	//   a - The first term in the dot product.  
	//   b - The second term in the dot product.  
	void DotV(Vector3Param a, Vector3Param b);

	// PURPOSE: Calculate the "flat" dot product of this vector and another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: the "flat" dot product.
	// NOTES
	//   - The "flat" dot product is a 2-dimensional dot product that takes only the x and z
	//     components into account.
	// SEE ALSO: Dot
	float XZDot(const Vector3& a) const;

	// PURPOSE: Calculate the cross product of this vector and another vector.
	// PARAMS
	//   a - The second term in the cross product.  (The current vector is the first term.)
	// SEE ALSO: CrossSafe,CrossNegate,CrossX,CrossY,CrossZ
	void Cross(Vector3Param a);	

	// PURPOSE: Set the current vector to the cross product of two other vectors.
	// PARAMS
	//   a - The first term in the cross product.
	//   b - The second term in the cross product.
	// NOTES
	//  - It's not safe to use this method when the current vector is also one of the parameters.
	//    Use CrossSafe() for such situations.
	// SEE ALSO: CrossSafe,CrossNegate,CrossX,CrossY,CrossZ
	void Cross(Vector3Param a, Vector3Param b);

	// PURPOSE: Set the current vector to the cross product of two other vectors.
	// PARAMS
	//   a - The first term in the cross product.
	//   b - The second term in the cross product.
	// NOTES
	//   - This method expends more effort to work when the current vector is also one of the parameters.
	//     Use Cross() if this extra safety is not needed.
	// SEE ALSO: Cross,CrossNegate,CrossX,CrossY,CrossZ
	void CrossSafe(Vector3Param a, Vector3Param b);

	// PURPOSE: Calculate the negative cross product of this vector and another vector.
	// PARAMS
	//   a - The second term in the cross product.  (The current vector is the first term.)
	// NOTES
	//   - The negative cross product is just the normal cross product with each component negated.
	//   - Because of the laws of cross products, a.CrossNegate (b) is equal to b.Cross (a).
	// SEE ALSO: Cross,CrossSafe,CrossX,CrossY,CrossZ
	void CrossNegate(Vector3Param a);

	// PURPOSE: Calculate just the x component of the cross product of this vector and another vector.
	// PARAMS
	//   a - The second term in the cross product.  (The current vector is the first term.)
	// RETURNS: the x component of the cross product.
	// SEE ALSO: Cross,CrossSafe,CrossNegate,CrossY,CrossZ
	float CrossX(const Vector3& a) const;

	// PURPOSE: Calculate just the y component of the cross product of this vector and another vector.
	// PARAMS
	//   a - The second term in the cross product.  (The current vector is the first term.)
	// RETURNS: the y component of the cross product.
	// SEE ALSO: Cross,CrossSafe,CrossNegate,CrossX,CrossZ
	float CrossY(const Vector3& a) const;

	// PURPOSE: Calculate just the z component of the cross product of this vector and another vector.
	// PARAMS
	//   a - The second term in the cross product.  (The current vector is the first term.)
	// RETURNS: the z component of the cross product.
	// SEE ALSO: Cross,CrossSafe,CrossNegate,CrossX,CrossY
	float CrossZ(const Vector3& a) const;

	// PURPOSE: Calculate the angle of rotation about the X axis between this vector and
	//	  another vector.
	// PARAMS
	//   a - The second vector.
	// RETURNS: The angle of rotation about the X axis that would change this vector
	//    to point in the same direction as vector a
	// NOTES
	//   - Unlike Angle(), this may return a negative angle, meaning a clockwise rotation
	//     about the X axis.
	// SEE ALSO: CrossX,Angle
	float AngleX(const Vector3& a) const;

	// PURPOSE: Calculate the angle of rotation about the Y axis between this vector and
	//	  another vector.
	// PARAMS
	//   a - The second vector.
	// RETURNS: The angle of rotation about the Y axis that would change this vector
	//    to point in the same direction as vector a
	// NOTES
	//   - Unlike Angle(), this may return a negative angle, meaning a clockwise rotation
	//     about the Y axis.
	// SEE ALSO: CrossY,Angle
	float AngleY(const Vector3& a) const;

	// PURPOSE: Calculate the angle of rotation about the Z axis between this vector and
	//	  another vector.
	// PARAMS
	//   a - The second vector.
	// RETURNS: The angle of rotation about the Z axis that would change this vector
	//    to point in the same direction as vector a
	// NOTES
	//   - Unlike Angle(), this may return a negative angle, meaning a clockwise rotation
	//     about the Z axis.
	// SEE ALSO: CrossZ,Angle
	float AngleZ(const Vector3& a) const;

	// PURPOSE: Calculate the angle of rotation about the X axis between this vector and
	//	  another vector.
	// PARAMS
	//   a - The second vector.
	// RETURNS: The angle of rotation about the X axis that would change this vector
	//    to point in the same direction as vector a
	// NOTES
	//   - Unlike Angle(), this may return a negative angle, meaning a clockwise rotation
	//     about the X axis.
	//   - This method assumes that both vectors are normalized.  If they're not, use AngleX() instead.
	// SEE ALSO: CrossX,Angle,AngleX
	float FastAngleX(const Vector3& a) const;

	// PURPOSE: Calculate the angle of rotation about the Y axis between this vector and
	//	  another vector.
	// PARAMS
	//   a - The second vector.
	// RETURNS: The angle of rotation about the Y axis that would change this vector
	//    to point in the same direction as vector a
	// NOTES
	//   - Unlike Angle(), this may return a negative angle, meaning a clockwise rotation
	//     about the Y axis.
	//   - This method assumes that both vectors are normalized.  If they're not, use AngleY() instead.
	// SEE ALSO: CrossY,Angle,AngleY
	float FastAngleY(const Vector3& a) const;

	// PURPOSE: Calculate the angle of rotation about the Z axis between this vector and
	//	  another vector.
	// PARAMS
	//   a - The second vector.
	// RETURNS: The angle of rotation about the Z axis that would change this vector
	//    to point in the same direction as vector a
	// NOTES
	//   - Unlike Angle(), this may return a negative angle, meaning a clockwise rotation
	//     about the Z axis.
	//   - This method assumes that both vectors are normalized.  If they're not, use AngleZ() instead.
	// SEE ALSO: CrossZ,Angle,AngleZ
	float FastAngleZ(const Vector3& a) const;

	// PURPOSE: Add to the current vector the cross product of the two given vectors.
	// PARAMS
	//   a - The vector to cross with b and then add to this vector.
	//   b - The vector that gets crossed by a and then added to this vector.
	// SEE ALSO: Cross,CrossNegate,SubtractCrossed
	void AddCrossed(Vector3Param a, Vector3Param b);

	// PURPOSE: Subtract from the current vector the cross product of the two given vectors.
	// PARAMS
	//   a - The vector to cross with b and then subtract from this vector.
	//   b - The vector that gets crossed by a and then subtracted from this vector.
	// SEE ALSO: Cross,CrossNegate,AddCrossed
	void SubtractCrossed(Vector3Param a, Vector3Param b);

	// PURPOSE: Average two vectors together.
	// PARAMS
	//   a - The other vector.
	void Average(Vector3Param a);

	// PURPOSE: Set the current vector to the average of two other vectors.
	// PARAMS
	//   a,b - The vectors to average together.
	void Average(Vector3Param a, Vector3Param b);

	// PURPOSE: Set the current vector to a linearly-interpolated value between two other vectors.
	// PARAMS
	//   a - The vector that represents a t value of 0.
	//   b - The vector that represents a t value of 1.
	//   t - The interpolation value for the desired point.
	// NOTES
	//   - "Lerp" is a contraction for "linear interpolation".
	//   - If you already have the slope between a and b (i.e. b minus a), AddScaled() is a more
	//     efficient way to accomplish what Lerp() does.
	// SEE ALSO: AddScaled,SubtractScaled
	void Lerp(float t, Vector3Param a, Vector3Param b);

	// PURPOSE: Set the current vector to a linearly-interpolated value between two other vectors.
	// PARAMS
	//   a - The vector that represents a t value of 0.
	//   b - The vector that represents a t value of 1.
	//   t - The interpolation value for the desired point.
	// NOTES
	//   - "Lerp" is a contraction for "linear interpolation".
	//   - If you already have the slope between a and b (i.e. b minus a), AddScaled() is a more
	//     efficient way to accomplish what Lerp() does.
	// SEE ALSO: AddScaled,SubtractScaled
	void Lerp(Vector3Param t, Vector3Param a, Vector3Param b);


	// PURPOSE: Set the current vector to a linearly-interpolated value between two other vectors.
	// PARAMS
	//   a - The vector that represents a t value of 1.
	//   t - The interpolation value for the desired point.
	// NOTES
	//   - "Lerp" is a contraction for "linear interpolation".
	//   - If you already have the slope between this and a (i.e. a minus this), AddScaled() is a more
	//     efficient way to accomplish what Lerp() does.
	// SEE ALSO: AddScaled,SubtractScaled
	void Lerp(float t, Vector3Param a);

	// PURPOSE: Set the current vector to a linearly-interpolated value between two other vectors.
	// PARAMS
	//   a - The vector that represents a t value of 1.
	//   t - The interpolation value for the desired point.
	// NOTES
	//   - "Lerp" is a contraction for "linear interpolation".
	//   - If you already have the slope between this and a (i.e. a minus this), AddScaled() is a more
	//     efficient way to accomplish what Lerp() does.
	// SEE ALSO: AddScaled,SubtractScaled
	void Lerp(Vector3Param t, Vector3Param a);

	//============================================================================
	// Magnitude and distance

	// PURPOSE: Compute the sqrt of each of the components
	Vector3 SqrtV() const;

    // PURPOSE: Compute the reciprocal sqrt of each of the components
	Vector3 RecipSqrtV() const;

	// PURPOSE: Calculate the magnitude (i.e. length) of this vector.
	// RETURNS: the length of this vector.
	// NOTES
	//   - This function involves a square-root.  If your need for the vector's length could be
	//     satisfied by the square of the vector's length, consider using Mag2() instead.
	// SEE ALSO: InvMag,Mag2,XZMag,XZMag2
	float Mag() const;

	// PURPOSE: Calculate the magnitude (i.e. length) of this vector.
	// RETURNS: a vector containing the length of this vector in every component.
	// NOTES
	//   - This function involves a square-root.  If your need for the vector's length could be
	//     satisfied by the square of the vector's length, consider using Mag2() instead.
	// SEE ALSO: InvMag,Mag2,XZMag,XZMag2
	Vector3 MagV() const;

	// PURPOSE: Calculate the magnitude (i.e. length) of this vector.
	// RETURNS: a vector containing the length of this vector in every component.
	// NOTES
	//   - This function involves a square-root-estimate.  If your need for the vector's length could be
	//     satisfied by the square of the vector's length, consider using Mag2() instead.
	// SEE ALSO: InvMag,Mag2,XZMag,XZMag2
	Vector3 MagFastV() const;
	
	// PURPOSE: Get the inverse magnitude of a vector.
	// RETURNS: the inverse magnitude (i.e. 1.0f / magnitude).
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag,Mag2,XZMag,XZMag2
	float InvMag() const;

	// PURPOSE: Get the inverse magnitude of a vector.
	// RETURNS: a vector containing the inverse magnitude (i.e. 1.0f / magnitude) in every component.
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag,Mag2,XZMag,XZMag2
	Vector3 InvMagV() const;

	// PURPOSE: Get the inverse magnitude of a vector using a reciprocal sqrt estimate
	// RETURNS: the inverse magnitude (i.e. 1.0f / magnitude).
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag,Mag2,XZMag,XZMag2
	float InvMagFast() const;

	// PURPOSE: Get the inverse magnitude of a vector using a reciprocal sqrt estimate
	// RETURNS: a vector containing the inverse magnitude (i.e. 1.0f / magnitude) in every component.
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag,Mag2,XZMag,XZMag2
	Vector3 InvMagFastV() const;

	// PURPOSE: Calculate the squared magnitude (i.e. squared length) of this vector.
	// RETURNS: the squared length of this vector.
	// NOTES
	//   If you actually need the length, consider using Mag() instead.
	// SEE ALSO: Mag,InvMag,XZMag,XZMag2
	float Mag2() const;

	// PURPOSE: Calculate the squared magnitude (i.e. squared length) of this vector.
	// RETURNS: a vector containing the squared length of this vector in every component.
	// NOTES
	//   If you actually need the length, consider using Mag() instead.
	// SEE ALSO: Mag,InvMag,XZMag,XZMag2
	Vector3 Mag2V() const;

	// PURPOSE: Calculate the flat magnitude (i.e. 2-dimensional length) of this vector.
	// RETURNS: the flat length of this vector.
	// NOTES
	//   - This is like the normal Mag() function, except that only x and z are taken into account.
	//   - This function involves a square-root.  If your need for the vector's flat length could be
	//     satisfied by the square of the vector's flat length, consider using XZMag2() instead.
	// SEE ALSO: Mag,InvMag,Mag2,XZMag2,XYMag,XYMag2
	float XZMag() const;

	// PURPOSE: Calculate the flat magnitude (i.e. 2-dimensional length) of this vector.
	// RETURNS: a vector containing the flat length of this vector in every component.
	// NOTES
	//   - This is like the normal MagV() function, except that only x and z are taken into account.
	//   - This function involves a square-root.  If your need for the vector's flat length could be
	//     satisfied by the square of the vector's flat length, consider using XZMag2V() instead.
	// SEE ALSO: Mag,InvMag,Mag2,XZMag2,XYMag,XYMag2
	Vector3 XZMagV() const;

	// PURPOSE: Calculate the squared flat magnitude (i.e. squared 2-dimensional length) of this vector.
	// RETURNS: the squared flat length of this vector.
	// NOTES
	//   - This is like the normal Mag2() function, except that only x and z are taken into account.
	//   - If you actually need the flat length, consider using Mag() instead.
	// SEE ALSO: Mag,InvMag,Mag2,XZMag,XYMag,XYMag2
	float XZMag2() const;

	// PURPOSE: Calculate the squared flat magnitude (i.e. squared 2-dimensional length) of this vector.
	// RETURNS: a vector containing the squared flat length of this vector in every component.
	// NOTES
	//   - This is like the normal Mag2V() function, except that only x and z are taken into account.
	//   - If you actually need the flat length, consider using MagV() instead.
	// SEE ALSO: Mag,InvMag,Mag2,XZMag,XYMag,XYMag2
	Vector3 XZMag2V() const;

	// PURPOSE: Calculate the flat magnitude (i.e. 2-dimensional length) of this vector in the XY plane.
	// RETURNS: the flat length of this vector.
	// NOTES
	//   - This is like the normal Mag() function, except that only x and y are taken into account.
	//   - This function involves a square-root.  If your need for the vector's flat length could be
	//     satisfied by the square of the vector's flat length, consider using XYMag2() instead.
	// SEE ALSO: Mag,InvMag,Mag2,XZMag2,XYMag2
	float XYMag() const;

	// PURPOSE: Calculate the flat magnitude (i.e. 2-dimensional length) of this vector in the XY plane.
	// RETURNS: a vector containing the flat length of this vector in every component.
	// NOTES
	//   - This is like the normal Mag() function, except that only x and y are taken into account.
	//   - This function involves a square-root.  If your need for the vector's flat length could be
	//     satisfied by the square of the vector's flat length, consider using XYMag2() instead.
	// SEE ALSO: Mag,InvMag,Mag2,XZMag2,XYMag2
	Vector3 XYMagV() const;

	// PURPOSE: Calculate the squared flat magnitude (i.e. squared 2-dimensional length) of this vector in the XY plane.
	// RETURNS: the squared flat length of this vector.
	// NOTES
	//   - This is like the normal Mag2() function, except that only x and y are taken into account.
	//   - If you actually need the flat length, consider using Mag() instead.
	// SEE ALSO: Mag,InvMag,Mag2,XZMag,XYMag
	float XYMag2() const;

	// PURPOSE: Calculate the squared flat magnitude (i.e. squared 2-dimensional length) of this vector in the XY plane.
	// RETURNS: a vector containing the squared flat length of this vector in every component.
	// NOTES
	//   - This is like the normal Mag2() function, except that only x and y are taken into account.
	//   - If you actually need the flat length, consider using XYMagV() instead.
	// SEE ALSO: Mag,InvMag,Mag2,XZMag2,XYMag2
	Vector3 XYMag2V() const;
	
	// PURPOSE: Calculate the distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the distance between the two points described by the vectors.
	// NOTES
	//   - This is equivalent to (*this - a).Mag().
	//   - This function involves a square-root.  If your need for the distance could be
	//     satisfied by the square of the distance, consider using Dist2() instead.
	// SEE ALSO: InvDist,Dist2,InvDist2,XZDist,XZDist2
	float Dist(const Vector3& a) const;

	// PURPOSE: Calculate the distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the distance between the two points described by the vectors in every component
	// NOTES
	//   - This is equivalent to (*this - a).Mag().
	//   - This function involves a square-root.  If your need for the distance could be
	//     satisfied by the square of the distance, consider using Dist2() instead.
	// SEE ALSO: InvDist,Dist2,InvDist2,XZDist,XZDist2
	Vector3 DistV(Vector3Param a) const;
	
	// PURPOSE: Calculate the inverse distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the inverse distance between the two points described by the vectors (i.e. 1 divided by the distance).
	// NOTES
	//   - This function involves an inverse square-root.  If your need for the inverse distance could be
	//     satisfied by the square of the inverse distance, consider using InvDist2() instead.
	// SEE ALSO: Dist,Dist2,InvDist2,XZDist,XZDist2
	float InvDist(const Vector3& a) const;

	// PURPOSE: Calculate the inverse distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the inverse distance between the two points described by the 
	//			vectors (i.e. 1 divided by the distance) in every component.
	// NOTES
	//   - This function involves an inverse square-root.  If your need for the inverse distance could be
	//     satisfied by the square of the inverse distance, consider using InvDist2() instead.
	// SEE ALSO: Dist,Dist2,InvDist2,XZDist,XZDist2
	Vector3 InvDistV(Vector3Param a) const;
	
	// PURPOSE: Calculate the squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the squared distance between the two points described by the vectors.
	// NOTES
	//   - This is equivalent to (*this - a).Mag2().
	//   - If you actually need the distance and not the squared distance, consider using Dist() instead.
	// SEE ALSO: Dist,InvDist,InvDist2,XZDist,XZDist2
	float Dist2(const Vector3& a) const;

	// PURPOSE: Calculate the squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the squared distance between the two points described by the vectors in every component
	// NOTES
	//   - This is equivalent to (*this - a).Mag2().
	//   - If you actually need the distance and not the squared distance, consider using Dist() instead.
	// SEE ALSO: Dist,InvDist,InvDist2,XZDist,XZDist2
	Vector3 Dist2V(Vector3Param a) const;
	
	// PURPOSE: Calculate the inverse squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the inverse squared distance between the two points described by the vectors.
	// NOTES
	//   - If you actually need the inverse distance and not the inverse squared distance, consider
	//     using InvDist() instead.
	// SEE ALSO: Dist,InvDist,Dist2,XZDist,XZDist2
	float InvDist2(const Vector3& a) const;

	// PURPOSE: Calculate the inverse squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the inverse squared distance between the two points described by the vectors in every component.
	// NOTES
	//   - If you actually need the inverse distance and not the inverse squared distance, consider
	//     using InvDist() instead.
	// SEE ALSO: Dist,InvDist,Dist2,XZDist,XZDist2
	Vector3 InvDist2V(Vector3Param a) const;
	
	// PURPOSE: Calculate the "flat" distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the "flat" distance between the two points described by the vectors.
	// NOTES
	//   - The flat distance is like the normal distance, except that only the x and z components are
	//     taken into account.
	//   - This function involves a square-root.  If your need for the flat distance could be
	//     satisfied by the square of the flat distance, consider using XZDist2() instead.
	// SEE ALSO: Dist,InvDist,Dist2,InvDist2,XZDist2, XZDistV
	float XZDist(const Vector3& a) const;
	
	// PURPOSE: Calculate the "flat" distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the "flat" distance between the two points described by the vectors.
	// NOTES
	//   - The flat distance is like the normal distance, except that only the x and z components are
	//     taken into account.
	//   - This function involves a square-root.  If your need for the flat distance could be
	//     satisfied by the square of the flat distance, consider using XZDist2() instead.
	// SEE ALSO: Dist,InvDist,Dist2,InvDist2,XZDist2, XZDist
	Vector3 XZDistV(Vector3Param a) const;

	// PURPOSE: Calculate the squared "flat" distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the squared "flat" distance between the two points described by the vectors.
	// NOTES
	//   - The flat distance is like the normal distance, except that only the x and z components are
	//     taken into account.
	//   - If you actually need the flat distance and not the squared flat distance, consider using
	//     XZDist() instead.
	// SEE ALSO: Dist,InvDist,Dist2,InvDist2,XZDist, XZDist2V
	float XZDist2(const Vector3& a) const;

	// PURPOSE: Calculate the squared "flat" distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the squared "flat" distance between the two points described by the vectors.
	// NOTES
	//   - The flat distance is like the normal distance, except that only the x and z components are
	//     taken into account.
	//   - If you actually need the flat distance and not the squared flat distance, consider using
	//     XZDist() instead.
	// SEE ALSO: Dist,InvDist,Dist2,InvDist2,XZDist, XZDist2
	Vector3 XZDist2V(Vector3Param a) const;

	// PURPOSE: Clamp the magnitude of a vector between the given minimum and maximum values.
	// PARAMS
	//   minMag - the lowest acceptable value for the magnitude of this vector
	//   maxMag - the highest acceptable value for the magnitude of this vector
	void ClampMag(float minMag, float maxMag);


	//============================================================================
	// Comparison functions

	// PURPOSE: Determine whether a vector is equal to the origin.
	// RETURNS: true if the vector is equal to the origin, false otherwise.
	// SEE ALSO: IsNonZero, IsNonZero
	bool IsZero() const;

	// PURPOSE: Determine whether a vector is equal to the origin.
	// RETURNS: a vector containing the test result, values have either all bits cleared for false or all bits set for true
	// SEE ALSO: IsNonZero, IsZero
	Vector3 IsZeroV() const;

	// PURPOSE: Determine whether a vector is equal to the origin.
	// RETURNS: a vector containing the test result, values have either all bits cleared for false or all bits set for true
	// NOTES: This function also compares the w component if it is present
	// SEE ALSO: IsNonZero, IsZero
	Vector3 IsZeroV4() const;

	// PURPOSE: Determine whether a vector is not equal to the origin.
	// RETURNS: true if the vector is not equal to the origin, false otherwise.
	// SEE ALSO: IsZero
	bool IsNonZero() const;

	// PURPOSE: Determine if the vector has only finite elements.
	// RETURN: true if the vector has all finite elements, false if any are not finite
	bool FiniteElements() const;

	// PURPOSE: Determine if the vector has only finite elements.
	// RETURN: a vector containing the test result, values have either all bits cleared for false or all bits set for true
	// NOTES: This function also compares the w component if it is present
	Vector3 FiniteElementsV4() const;

	// PURPOSE: Determine whether a vector is equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is equal to the other vector, false otherwise.
	// SEE ALSO: IsNotEqual,IsClose,IsEqualV
	bool IsEqual(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values have either all bits cleared for false or all bits set for true
	// SEE ALSO: IsNotEqual,IsClose,IsEqual
	Vector3 IsEqualV(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is not equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is not equal to the other vector, false otherwise.
	// SEE ALSO: IsEqual,IsClose
	bool IsNotEqual(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is equal to another vector, within some tolerance value.
	// PARAMS
	//   a - The other vector.
	//   eps - The tolerance value.
	// RETURNS: true if the vector is equal to the other vector within the tolerance value, false otherwise.
	// SEE ALSO: IsEqual,IsNotEqual
	bool IsClose(Vector3Param a,float eps) const;

	// PURPOSE: Determine whether a vector is equal to another vector, within some tolerance value.
	// PARAMS
	//   a - The other vector.
	//   eps - The tolerance value.
	// RETURNS: true if the vector is equal to the other vector within the tolerance value, false otherwise.
	// SEE ALSO: IsEqual,IsNotEqual
	bool IsClose(Vector3Param a, Vector3Param eps) const;

	// PURPOSE: Determine whether a vector is greater than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is greater than the other vector
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThanV,IsGreaterThanVR
	bool IsGreaterThan(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is greater than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThan,IsGreaterThanVR
	Vector3 IsGreaterThanV(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is greater than another vector. This does the same as IsGreaterThanV 
	//			but doesn't mask out the w component
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThan,IsGreaterThanVR
	Vector3 IsGreaterThanV4(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is greater than another vector
	// PARAMS
	//   a - The other vector.
	//	 r - The register containing flags for this operation
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThanV,IsGreaterThanVR
#if __XENON
	Vector3 IsGreaterThanVR(Vector3Param a, u32& r) const;
#else
	Vector3 IsGreaterThanVR(const Vector3& a, u32& r) const;
#endif

	// PURPOSE: Determine whether a vector is greater or equal than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is greater than the other vector
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThanV,IsGreaterThanVR
	bool IsGreaterOrEqualThan(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is greater or equal than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThan,IsGreaterThanVR
	Vector3 IsGreaterOrEqualThanV(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is greater or equal than another vector
	// PARAMS
	//   a - The other vector.
	//	 r - The register containing flags for this operation
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThanV,IsGreaterThanVR
	Vector3 IsGreaterOrEqualThanVR(Vector3Param a, u32& r) const;

	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is less than the other vector
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThanVR
	bool IsLessThanAll(Vector3Param a) const;

	// PURPOSE: Compatibility with existing code previously written with IsLessThan
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is less than the other vector
	// NOTES: do not use this, as its effects are platform dependent!
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThanVR
	bool IsLessThanDoNotUse(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThan,IsLessThanVR
	Vector3 IsLessThanV(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// NOTES: This function does not bother with clearing the w component before doing the compare.
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThan,IsLessThanVR
	Vector3 IsLessThanV4(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	//	 r - The register containing flags for this operation
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThan
#if __XENON
	Vector3 IsLessThanVR(Vector3Param a, u32& r) const;
#else
	Vector3 IsLessThanVR(const Vector3& a, u32& r) const;
#endif
	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	//	 r - The register containing flags for this operation
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// NOTES: This function does not bother with clearing the w component before doing the compare.
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThan
#if __XENON
	Vector3 IsLessThanVR4(Vector3Param a, u32& r) const;
#else
	Vector3 IsLessThanVR4(const Vector3& a, u32& r) const;
#endif

	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is less or equal than the other vector
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThanVR
	bool IsLessOrEqualThanAll(Vector3Param a) const;

	// PURPOSE: Compatibility with existing code previously written with IsLessThan
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is less or equal than the other vector
	// NOTES: do not use this, as its effects are platform dependent!
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThanVR
	bool IsLessOrEqualThanDoNotUse(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is less or equal than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThan,IsLessThanVR
	Vector3 IsLessOrEqualThanV(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is less or equal than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// NOTES: This function does not bother with clearing the w component before doing the compare.
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThan,IsLessThanVR
	Vector3 IsLessOrEqualThanV4(Vector3Param a) const;

	// PURPOSE: Determine whether a vector contains the boolean value (true, true, true)
	// RETURNS: true if the vector contains (true, true, true), such as set by the comparison operators
	//          IsEqual, etc.
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThan
	bool IsTrueTrueTrue() const;

	// PURPOSE: Determine whether a vector contains the boolean value (false, false, false)
	// RETURNS: true if the vector contains (false, false, false), such as set by the comparison operators
	//          IsEqual, etc.
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThan
	bool IsFalseFalseFalse() const;

	// PURPOSE: Select a vector based on the value of this vector
	// PARAMS
	//   zero - The vector to be returned if this vector is zero
	//	 nonZero - The vector to be returned if this vector is (0xFFFFFFFFFFFFFFFF)
	// RETURNS: The parameter selected by the value of this vector
	// NOTES: This is a bitwise selection function... based on the bits of this->xyzw.
	Vector3 Select(Vector3Param zero, Vector3Param nonZero) const;

	// PURPOSE: Set each component to the maximum component from the two passed in vectors
	// PARAMS
	//	a - The first vector
	//	b - The second vector
	void Max(Vector3Param a, Vector3Param b);

	// PURPOSE: Set each component to the minimum component from the two passed in vectors
	// PARAMS
	//	a - The first vector
	//	b - The second vector
	void Min(Vector3Param a, Vector3Param b);


	//============================================================================
	// Output

	// PURPOSE: Print a vector to std output.
	// PARAMS
	//   newline - If true, prints a newline char after printing the vector.
	void Print(bool newline=true) const;

	// PURPOSE: Print a vector to std output with a prefixed label.
	// PARAMS
	//   label - A label to print before the vector.
	//   newline - If true, prints a newline char after printing the vector.
	void Print(const char * label, bool newline=true) const;


	//============================================================================
	// Matrix operations

	// PURPOSE: Transform the current vector by the given matrix.
	// PARAMS
	//   mtx - The matrix containing the transformation to apply to the vector.
	// SEE ALSO
	//   Dot3x3,Transform,Transform3x3
	// NOTES
	//   Defined in matrix34.h
	void Dot(const Matrix34 & mtx);
	void Dot(const Matrix33 & mtx);

	// PURPOSE:  Set the current vector to the transformation of another vector by a given matrix.
	// PARAMS
	//   v - The vector to be transformed.
	//   mtx - The matrix containing the transformation to apply to the vector.
	// SEE ALSO
	//   Dot3x3,Transform,Transform3x3
	// NOTES
	//   Defined in matrix34.h
	void Dot(const Vector3 & v,const Matrix34 & m);
	void Dot(const Vector3 & v,const Matrix33 & m);

	// PURPOSE:   Set the current vector to the transformation of another vector by the 3x3 part of a given matrix.
	// PARAMS
	// v - The vector to be transformed.
	// mtx - The matrix containing the 3x3 transformation to apply to the vector.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   Defined in matrix34.h
	// SEE ALSO
	//   Dot,Transform,Transform3x3
	void Dot3x3(const Matrix34 & mtx);

	// PURPOSE:   Transform the current vector by the 3x3 part of a given matrix.
	// PARAMS
	//   mtx - The matrix containing the 3x3 transformation to apply to the vector.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This is the inverse of Dot3x3Transpose(), i.e. a.Dot3x3Transpose(mtx).Dot3x3(mtx) == a.
	//   - Defined in matrix34.h
	// SEE ALSO
	//   Dot,Dot3x3Transpose,Transform,Transform3x3
	void Dot3x3(const Vector3 & v,const Matrix34 & m);

	// PURPOSE:   Un-transform the current vector by the 3x3 part of a given matrix.
	// PARAMS
	//   mtx - The matrix containing the transpose of the 3x3 transformation to apply to the vector.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This is the inverse of Dot3x3(), i.e. a.Dot3x3(mtx).Dot3x3Transpose(mtx) == a.
	//   - Defined in matrix34.h
	// SEE ALSO
	//   Dot,Dot3x3,Transform,Transform3x3
	void Dot3x3Transpose(const Matrix34 & mtx);
	void DotTranspose(const Matrix33 & mtx);

	// PURPOSE:   Rotate a vector by a given number of radians around a given axis.
	// PARAMS
	//   radians - How much to rotate around the given axis.
	//   axis - The axis to rotate around.  Can be 'x', 'y', or 'z' (i.e. character values).
	// SEE ALSO
	//   RotateAboutAxis,RotateX,RotateY,RotateZ
	void RotateAboutAxis(float radians, int axis);

	// PURPOSE:  Make a pair of unit vectors that make a right-handed coordinate system with this unit vector.
	// PARAMS
	// ortho1 - the second unit axis vector in a right-handed coordinate system (this vector is first)
	// ortho2 - the third unit axis vector in a right-handed coordinate system
	void MakeOrthonormals (Vector3 & ortho1, Vector3 & ortho2) const;


	//============================================================================
	// Nifty Algebra

	// PURPOSE:	Compute the base-2 logarithm of each component of this vector
	// NOTES: Each component is set to its base-2 logarithm
	//		log(x), log(y), log(z)
	void Log();

	// PURPOSE:	Compute the base-2 logarithm of each component of the input vector
	// PARAMS:
	//	v - The vector to compute the logarithm of
	// NOTES: Each component is set to the base-2 logarithm of the input vector
	//		log(v.x), log(v.y), log(v.z)
	void Log(Vector3Param v);

	// PURPOSE:	Compute the base-10 logarithm of each component of this vector
	// NOTES: Each component is set to its base-2 logarithm
	//		log10(x), log10(y), log10(z)
	void Log10();

	// PURPOSE:	Compute the base-10 logarithm of each component of the input vector
	// PARAMS:
	//	v - The vector to compute the logarithm of
	// NOTES: Each component is set to the base-10 logarithm of the input vector
	//		log10(v.x), log10(v.y), log10(v.z)
	void Log10(Vector3Param v);

	// PURPOSE
	//   Translate this vector toward a target vector at the given 
	//   rate for the given amount of time.  Returns true if the vector has
	//   reached the goal.  This version approaches along the line
	//   between this and the goal.
	// PARAMS
	//   goal - The goal vector, which is being approached by the current vector.
	//   rate - How quickly the value is moving toward the goal.
	//   time - How much elapsed time to process in this call.
	// RETURNS: true if the current vector has reached the goal, false otherwise.  The current vector
	//   is updated to contain its new, closer-to-goal, position.
	bool ApproachStraight(const Vector3 &goal,float rate,float time);


	// PURPOSE:  Calculate the angle between the current vector and another vector.
	// PARAMS
	//   v - The other vector.
	// NOTES
	//   - If both vectors are guaranteed to be normalized, consider using FastAngle() instead.
	// SEE ALSO
	//   FastAngle
	float Angle(const Vector3 &v) const;

	// PURPOSE:  Calculate the angle between the current vector and another vector.
	// PARAMS
	//   v - The other vector.
	// NOTES
	//   - This method assumes that both vectors are normalized.  If they're not, use Angle() instead.
	// SEE ALSO: Angle
	float FastAngle(Vector3Param v) const;

	// PURPOSE:  Get the polar version of this vector looking in a certain direction.
	// PARAMS
	//   lookTo - The location to "look to", from the location described by the current vector.
	//   outPolarCoords - An output parameter; the vector it points to receives (distance, azimuth, inclination).
	//   outPolarOffset - An output parameter; it gets set to the value of LookTo.
	void GetPolar(const Vector3& lookTo, Vector3& outPolarCoords, Vector3& outPolarOffset) const;

	// PURPOSE:  Extend a vector by a distance.
	// PARAMS
	//   distance - How much longer to make the vector.
	// NOTES
	//   - The new vector Mag() is equal to the old vector Mag() plus the given distance.
	void Extend(float distance);
	
	// PURPOSE:  Set the current vector to an extended version of another vector.
	// PARAMS
	//   v - The vector to extend.
	//   distance - How much longer to make the vector.
	// NOTES
	//   - The current vector's new Mag() is equal to v.Mag() plus the given distance.
	void Extend(const Vector3& v, float distance);

	// PURPOSE: Calculate what happens to a vector after it bounces off of a plane.
	// PARAMS
	//   normal - The normal vector of the plane.
	// NOTES
	//   This routine assumes that the vectors aren't necessarily normalized.  If both the current
	//   vector and normal vector are known to be normalized, you may use ReflectAboutFast() instead.
	void ReflectAbout(const Vector3& normal);

	// PURPOSE: Calculate what happens to a vector after it bounces off of a plane.
	// PARAMS
	//   normal - The normal vector of the plane.
	// NOTES
	//   This routine assumes that both vectors are normalized.  If they're not, you must
	//   use ReflectAbout() instead.
	void ReflectAboutFast(const Vector3& normal);

	// PURPOSE
	//   Add the given vector to the current vector, but do not add any part of the given
	//   vector that points in the same direction as the current vector.
	// PARAMS
	//   add - The vector to sum with the current vector, less any positive parallel component.
	// NOTES
	//   This is for summing together multiple pushes on the same object, so that parallel
	//   pushes do not add and anti-parallel pushes do add.
	void AddNet (Vector3Param add);

	// PURPOSE: Rotate a vector by a given number of radians around the an axis.
	// PARAMS
	//   radians - How much to rotate around the X axis.
	// See also:
	//   RotateAboutAxis
	void RotateX(float radians);

	// <COMBINE Vector3::RotateX>
	void RotateY(float radians);

	// <COMBINE Vector3::RotateX>
	void RotateZ(float radians);

	//============================================================================
	// Bitwise operations

	// PURPOSE: Bitwise invert operation.
	void Not();

	// PURPOSE: Bitwise and of this vector with another vector
	// PARAMS
	//   and - The vector to and with this vector
	// See also:
	//		Or, Xor, MergeXY, MergeZW
	void And(Vector3Param _and);

	// PURPOSE: Bitwise or of this vector with another vector
	// PARAMS
	//   and - The vector to or with this vector
	// See also:
	//		And, Xor, MergeXY, MergeZW
	void Or(Vector3Param _or);

	// PURPOSE: Bitwise xor of this vector with another vector
	// PARAMS
	//   and - The vector to xor with this vector
	// See also:
	//		Or, And, MergeXY, MergeZW
	void Xor(Vector3Param _xor);

	// PURPOSE: Merge the X and Y components of two vectors
	// PARAMS
	//   vY - Operand
	// NOTES:
	//		The resulting vector is laidout as follows
	//			x, vYx, y, vYy
	// See also:
	//		Or, Xor, And, MergeZW
	void MergeXY(Vector3Param vY);

	// PURPOSE: Merge the X and Y components of two vectors
	// PARAMS
	//	vX - Operand
	//	vY - Operand
	// NOTES:
	//		The resulting vector is laid out as follows
	//			vXx, vYx, vXy, vYy
	// See also:
	//		Or, Xor, And, MergeZW
	void MergeXY(Vector3Param vX, Vector3Param vY);

	// PURPOSE: Merge the Z and W components of two vectors
	// PARAMS
	//   vW - Operand
	// NOTES:
	//		This function is only implemented with vectorized padding enabled
	//		The resulting vector is laid out as follows
	//			z, vWz, w, vWw
	// See also:
	//		Or, Xor, MergeXY, And
#if VECTORIZED_PADDING
	void MergeZW(Vector3Param vW);
#endif

	// PURPOSE: Merge the Z and W components of two vectors
	// PARAMS
	//	vZ - Operand
	//	vW - Operand
	// NOTES:
	//		This function is only implemented with vectorized padding enabled.
	//		The resulting vector is laid out as follows
	//			vZz, vWz, vZw, vWw
	// See also:
	//		Or, Xor, MergeXY, And
#if VECTORIZED_PADDING
	void MergeZW(Vector3Param vZ, Vector3Param vW);
#endif

    // PURPOSE: Swap around components of the vector
    // PARAMS
    //  v - The source vector vector to permute
    //  permX - The component of the source vector to choose for the X component
    //  permY - The component of the source vector to choose for the Y component
    //  permZ - The component of the source vector to choose for the Z component
    //  permW - The component of the source vector to choose for the W component
    // NOTES:
    //  x, y, z, and w should be one of VEC_PERM_X, VEC_PERM_Y, VEC_PERM_Z, or VEC_PERM_W
#if VECTORIZED_PADDING
    template <int permX, int permY, int permZ, int permW>
    void Permute(Vector3Param v);
#endif

	//============================================================================
	// Operators

#if (__XENON || __PS3) && VECTORIZED
	__forceinline operator __vector4() const				{ return xyzw; }
	__forceinline operator __vector4&()						{ return xyzw; }
#if __XENON
	operator vec_float4 () const				{ vec_float4 r; r.v = xyzw; return r; }
#endif
#elif RSG_CPU_INTEL && !defined(_NO_INTRINSICS_)
	__forceinline operator __m128() const					{ return xyzw; }
	__forceinline operator __m128&()						{ return xyzw; }
#elif __PSP2
	__forceinline operator __vector4() const				{ return xyzw; }
	__forceinline operator __vector4&()						{ return xyzw; }
#endif

	__forceinline operator const Vec::Vector_4&() const 	{ return *((Vec::Vector_4*)(this)); }
	__forceinline operator Vec::Vector_4&()					{ return *((Vec::Vector_4*)(this)); }

	// PURPOSE: Assignment operator usefule for compiler specific optimized implementations
	// PARAMS
	//	v - The vector to assign to this vector
	Vector3& operator=(const Vector3& v);

	// PURPOSE: Determine whether a vector is equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is equal to the other vector, false otherwise.
	// NOTES
	//   - This is equivalent to IsEqual().
	// SEE ALSO: IsEqual,IsNotEqual,IsClose
	bool operator==(Vector3Param a) const;

	// PURPOSE: Determine whether a vector is not equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is not equal to the other vector, false otherwise.
	// NOTES
	//   - This is equivalent to IsNotEqual().
	// SEE ALSO: IsEqual,IsNotEqual,IsClose
	bool operator!=(Vector3Param a) const;

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	// RETURNS: the sum of the two vectors.
	// SEE ALSO: Add
	Vector3 operator+(Vector3Param a) const;

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	// RETURNS: the difference between the two vectors.
	// SEE ALSO: Subtract
	Vector3 operator-(Vector3Param a) const;

	// PURPOSE: Negate a vector.
	// RETURNS: the negation of the current vector.
	// SEE ALSO: Negate
	Vector3 operator-() const;

	// PURPOSE: Multiply the current vector by a value.
	// PARAMS
	//   f - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	Vector3 operator*(const float f) const;	

	// PURPOSE: Multiply the current vector by a value.
	// PARAMS
	//   f - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	Vector3 operator*(Vector3Param f) const;	

	// PURPOSE: Multiply a vector by a value.
	// PARAMS
	//   V - the vector to multiply.
	//   f - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	friend Vector3 operator*(const float f, const Vector3& V);

	// PURPOSE: Divide the current vector by a value.
	// PARAMS
	//   f - The value to divide the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: InvScale
	Vector3 operator/(const float f) const;

	// PURPOSE: Divide the current vector by a vector.
	// PARAMS
	//   f - The vector to divide the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: InvScale
	Vector3 operator/(Vector3Param f) const;

	// PURPOSE: Bitwise or of this vector with another vector
	// PARAMS
	//   f - The vector to or with this vector
	// RETURNS: the combined vector
	Vector3 operator|(Vector3Param f) const;

	// PURPOSE: Bitwise and of this vector with another vector
	// PARAMS
	//   f - The vector to and with this vector
	// RETURNS: the bitwise and result vector
	Vector3 operator&(Vector3Param f) const;

	// PURPOSE: Bitwise xor of this vector with another vector
	// PARAMS
	//   f - The vector to xor with this vector
	// RETURNS: the bitwise xor result vector
	Vector3 operator^(Vector3Param f) const;

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	// RETURNS: the sum of the two vectors.
	void operator+=(Vector3Param V);

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	// RETURNS: the difference between the two vectors.
	void operator-=(Vector3Param V);

	// PURPOSE: Multiply a vector by a value.
	// PARAMS
	//   V - the vector to multiply.
	//   f - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	void operator*=(const float f);

	// PURPOSE: Multiply a vector by a vector.
	// PARAMS
	//   V - the vector to multiply.
	//   f - The vector to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	void operator*=(Vector3Param f);

	// PURPOSE: Divide the current vector by a value.
	// PARAMS
	//   f - The value to divide the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: InvScale
	void operator/=(const float f);

	// PURPOSE: Divide the current vector by a vector.
	// PARAMS
	//   f - The vector to divide the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: InvScale
	void operator/=(Vector3Param f);

	// PURPOSE: Bitwise or of this vector with another vector
	// PARAMS
	//   f - The vector to or with this vector
	void operator|=(Vector3Param f);

	// PURPOSE: Bitwise and of this vector with another vector
	// PARAMS
	//   f - The vector to and with this vector
	void operator&=(Vector3Param f);

	// PURPOSE: Bitwise xor of this vector with another vector
	// PARAMS
	//   f - The vector to xor with this vector
	void operator^=(Vector3Param f);


	//============================================================================
	// Euler functions

	// PURPOSE: Clamp rotation angle in range [-PI, PI].
	// PARAMS
	//   r - the rotation angle to be clamped.
	float ClampAngle(float r);

	// PURPOSE: Find the alternate rotation of this one.
	// PARAMS
	//   outRotation - the output rotation angle.
	// NOTES
	//   The function is for XYZ order.
	void FindAlternateXYZ(Vector3 &outRotation);

	// PURPOSE: Find the alternate rotation of this one.
	// PARAMS
	//   outRotation - the output rotation angle.
	// NOTES
	//   The function is for XZY order.
	void FindAlternateXZY(Vector3 &outRotation);

	// PURPOSE: Find a rotation vector which is close to this one.
	// PARAMS
	//   inoutRotation - The input/output rotation vector.
	// NOTES
	//   The close rotation vector is either ioR or its alternate.
	//   This is for XYZ order.
	void FindCloseAngleXYZ(Vector3 &inoutRotation);

	// PURPOSE: Find a rotation vector which is close to this one.
	// PARAMS
	//   inoutRotation - The input/output rotation vector.
	// NOTES
	//   The close rotation vector is either ioR or its alternate.
	//   This is for XZY order.
	void FindCloseAngleXZY(Vector3 &inoutRotation);

	int GetSize() const { return 3; }

	//============================================================================
	// Data

#if RSG_CPU_INTEL && !defined(_NO_INTRINSICS_)
#if defined(_MSC_VER)
#pragma warning (disable : 4201)			// nonstandard extension used : nameless struct/union
#endif
	union 
	{
		__m128	xyzw;
		struct 
		{ 
			float x;
			float y;
			float z;
			float w;
		};
		struct 
		{ 
			unsigned int ux;
			unsigned int uy;
			unsigned int uz;
			unsigned int uw;
		};
	};
#if defined(_MSC_VER)
#pragma warning (default : 4201)			// nonstandard extension used : nameless struct/union
#endif
#elif __XENON && VECTORIZED
#pragma warning (disable : 4201)			// nonstandard extension used : nameless struct/union
	union
	{
		__vector4	xyzw;
		struct 
		{ 
			float x;
			float y;
			float z;
			float w;
		};
		struct 
		{ 
			unsigned int ux;
			unsigned int uy;
			unsigned int uz;
			unsigned int uw;
		};
	};
#pragma warning (default : 4201)			// nonstandard extension used : nameless struct/union
#elif __PS3
	union
	{
		__vector4	xyzw;
		struct 
		{ 
			float x;
			float y;
			float z;
			float w;
		};
		struct 
		{ 
			unsigned int ux;
			unsigned int uy;
			unsigned int uz;
			unsigned int uw;
		};
	};
#elif __PSP2
	union 
	{
		float32x4_t xyzw;
		struct 
		{
			float x;
			float y;
			float z;
			float w;
		};
		struct 
		{ 
			unsigned int ux;
			unsigned int uy;
			unsigned int uz;
			unsigned int uw;
		};
	};
#else // !__VECTORIZED
#pragma warning (disable : 4201)			// nonstandard extension used : nameless struct/union
	union 
	{
		struct 
		{ 
			float x;
			float y;
			float z;
#if VECTORIZED_PADDING
			float w;
#endif
		};
		struct 
		{ 
			unsigned int ux;
			unsigned int uy;
			unsigned int uz;
#if VECTORIZED_PADDING
			unsigned int uw;
#endif
		};
	};
#pragma warning (default : 4201)			// nonstandard extension used : nameless struct/union
#endif

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif
}
#if (__PS3) && VECTORIZED

#endif
;


typedef Vector3 Scalar;


#if 0
class Vector3Array {
private:
	static float *allocate(int count)
	{
		count = count*3;
		count = (count + 4) & ~3;	// Round up to quadword boundary, adding a pad quadword 
									// if it's already a multiple of four to avoid reading
									// past end of data on the last element of the array.
		return rage_new float[count];
	}
public:
	Vector3Array() : data(NULL) { }
	Vector3Array(int count) : data(allocate(count)) { }
	Vector3Array(datResource &rsc) { rsc.PointerFixup(data); }
	~Vector3Array() { delete [] data; }

	void Init(int count)
	{
		delete[] data;
		data = allocate(count);
	}

#if RSG_CPU_INTEL
	Vector3 operator[](size_t index) const 
	{
		float *addr = data + index*3;
		return Vector3(addr[0],addr[1],addr[2]);
	}

	void Set(size_t index,const Vector3 &v)
	{
		float *addr = data + index*3;
		addr[0] = v.x;
		addr[1] = v.y;
		addr[2] = v.z;
	}
#else
	__vector4 operator[](size_t index) const 
	{
		// When reading the last element of an array that is a multiple of
		// four, it's possible the lvrx will read the next quadword.  We handle
		// this by allocating an extra quadword in that case.
		float *addr = data + index*3;
		return (__vector4) __vor(__vector4(__lvlx(addr, 0)), __vector4(__lvrx(addr,16)));
	}

	void Set(size_t index,const __vector4 v)
	{
		float *addr = data + index*3;
		__stvlx(v, addr, 0);
		__stvrx(v, addr,16);
	}
#endif

private:
	float *data;
};
#endif

//=============================================================================
// PURPOSE:
//   Equal to the number of floats that are stored in a Vector3 object.  This 
//   will equal 3 if VECTORIZED_PADDING is false, 4 if true.  Helpful when
//   creating code that is compatible for both 3-float and 4-float versions
//   of the Vector3 class.  E.g. for a stride length in an array of Vector3's
//   that are being referenced as an array of floats.
#define VEC3_NUM_STORED_FLOATS (sizeof(Vector3)/sizeof(float))

// PURPOSE: The bit in the return value of a vector compare indicating success.  This bit will be set on success, and cleared on failure
#define VEC3_CMP_VAL		0x80

// PURPOSE: Serialize a vector object
inline datSerialize & operator<< (datSerialize &s, Vector3 &v) {
	s << v.x << v.y << v.z;
	return s;
}



//=============================================================================
// Global variables defining some commonly used const Vector3 objects.
#if !__SPU

// PURPOSE: A vector with the value of the coordinate system's origin.
extern const Vector3 ORIGIN;

// PURPOSE: A vector with the value of the coordinate system's X axis.
extern const Vector3 XAXIS;

// PURPOSE: A vector with the value of the coordinate system's Y axis.
extern const Vector3 YAXIS;

// PURPOSE: A vector with the value of the coordinate system's Z axis.
extern const Vector3 ZAXIS;

// PURPOSE: A vector whose component values are all 0.
extern const Vector3 VEC3_ZERO;

// PURPOSE: A vector containing all bits set in the y, z, and w components and 0 bits set in the x component
extern const Vector3 VEC3_ANDX;

// PURPOSE: A vector containing all bits set in the x, z, and w components and 0 bits set in the y component
extern const Vector3 VEC3_ANDY;

// PURPOSE: A vector containing all bits set in the x, y, and w components and 0 bits set in the z component
extern const Vector3 VEC3_ANDZ;

// PURPOSE: A vector containing all bits set in the x, y, and z components and 0 bits set in the w component
extern const Vector3 VEC3_ANDW;

// PURPOSE: A vector containing all bits set in the x component and 0 bits set in the other components
extern const Vector3 VEC3_MASKX;

// PURPOSE: A vector containing all bits set in the y component and 0 bits set in the other components
extern const Vector3 VEC3_MASKY;

// PURPOSE: A vector containing all bits set in the z component and 0 bits set in the other components
extern const Vector3 VEC3_MASKZ;

// PURPOSE: A vector containing all bits set in the w component and 0 bits set in the other components
extern const Vector3 VEC3_MASKW;

// PURPOSE: A vector containing all bits set in the all components
extern const Vector3 VEC3_MASKXYZW;

// PURPOSE: A vector whose component values are all 1.
extern const Vector3 VEC3_IDENTITY;

// PURPOSE: A vector whose component values are all 0.5f
extern const Vector3 VEC3_HALF;

// PURPOSE: A vector whose component values are all 0.333...f
extern const Vector3 VEC3_THIRD;

// PURPOSE: A vector whose component values are all 0.25f
extern const Vector3 VEC3_QUARTER;

// PURPOSE: A vector whose component values are all approximately the square root of two.
extern const Vector3 VEC3_SQRTTWO;

// PURPOSE: A vector whose component values are all approximately the square root of three.
extern const Vector3 VEC3_SQRTTHREE;

// PURPOSE: A vector whose component values are all INF
extern const Vector3 VEC3_INF;

// PURPOSE: A vector whose component values are all NAN
extern const Vector3 VEC3_NAN;

// PURPOSE: A vector containing zeros in the x, y, and z components and 1.0 in the w component
extern const Vector3 VEC3_ONEW;

// PURPOSE: A vector whose component values are all the maximum floating-point value.
extern const Vector3 VEC3_MAX;

// PURPOSE: A vector whose component values are SMALL_FLOAT (1e-6).
extern const Vector3 VEC3_SMALL_FLOAT;

// PURPOSE: A vector whose component values are all VERY_SMALL_FLOAT (1e-12).
extern const Vector3 VEC3_VERY_SMALL_FLOAT;

// PURPOSE: A vector whose component values are all LARGE_FLOAT (1e8).
extern const Vector3 VEC3_LARGE_FLOAT;

#endif


extern Vector3 g_UnitUp;


inline static bool ComputeRotation (const Vector3& unitFrom, const Vector3& unitTo, Vector3& unitAxis, float& angle)
{
	const float underOne = square(0.999f);
	ASSERT_ONLY(const float overOne = square(1.001f);)
	mthAssertf(unitFrom.Mag2() >= underOne && unitFrom.Mag2() <= overOne, "Vector3 <%f, %f, %f> does not have length 1",unitFrom.x,unitFrom.y,unitFrom.z);
	mthAssertf(unitTo.Mag2() >= underOne && unitTo.Mag2() <= overOne, "Vector3 <%f, %f, %f> does not have length 1",unitTo.x,unitTo.y,unitTo.z);
	float dot = unitFrom.Dot(unitTo);
	if (dot<underOne)
	{
		// The vectors defining the rotation are not nearly parallel.
		unitAxis.Set(unitFrom);
		angle = PI;
		if (dot>-underOne)
		{
			// The vectors defining the rotation are not nearly anti-parallel.
			unitAxis.Cross(unitTo);
			float sine = unitAxis.Mag();
			if (sine!=0.0f)
			{
				unitAxis.InvScale(sine);
				angle = dot>0.0f ? AsinfSafe(sine) : PI-AsinfSafe(sine);
			}
			else
			{
				unitAxis.Set(XAXIS);
				angle = 0.0f;
			}
		}
		else
		{
			// The vectors defining the rotation are in nearly opposite directions, so choose an axis for a 180 degree rotation.
			Vector3 perpendicular = (fabsf(unitFrom.x)<SQRT3INVERSE ? XAXIS : (fabsf(unitFrom.y)<SQRT3INVERSE ? YAXIS : ZAXIS));
			unitAxis.Cross(perpendicular);
			unitAxis.Normalize();
		}

		// Return true to indicate that the given vectors are not parallel, so a rotation should be done.
		return true;
	}

	// Return false to indicate that the given vectors are parallel, so no rotation is needed.
	return false;
}

namespace sysEndian
{
	template<> inline void SwapMe(Vector3& v) {
		SwapMe(v.x);
		SwapMe(v.y);
		SwapMe(v.z);
		SwapMe(v.w); // just in case someone stored some data in w
	}
} // namespace sysEndian


}	// namespace rage


//=============================================================================
// Implementations

// First, include platform specific implementation

#if RSG_CPU_INTEL && !defined(_NO_INTRINSICS_)
#include "vector/vector3_win32.h"
#elif (__XENON || __PS3) && VECTORIZED

#include "vector/vector3_xenon.h"
#endif

// Second, include standard version for any that weren't implemented in 
// the platform specific section
#include "vector/vector3_default.h"

#endif // VECTOR_VECTOR3_H
