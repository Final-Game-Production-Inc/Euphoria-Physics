//
// vector/vector4.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_VECTOR4_H
#define VECTOR_VECTOR4_H

#include "vector3.h"

#include "data/serialize.h"
#include "math/amath.h"
#include "math/intrinsics.h"
#include "math/nan.h"
#include "system/endian.h"

namespace rage {

class Matrix44;


/***************** Vector4 *******************/

#if __XENON
#pragma warning(push, 3)
#endif

/* 
Purpose: Vector4 is a vector with four elements (x, y, z and w). It is usually used as a Vector3
			(x, y and z) with 16-byte alignment. 
<FLAG Component>
*/
class ALIGNAS(16) Vector4 {
public:
	// Define the type of a constant Vector3/Vector4 that we pass to an inline function.
#if (__XENON || __PS3) && VECTORIZED
	typedef const Vector3& Vector3Param;	
	typedef const __vector4 Vector4Param;
	typedef const __vector4 Param;
	typedef Vector3& Vector3Ref;
	typedef __vector4& Vector4Ref;
	typedef __vector4& Ref;

	typedef __vector4 Vector4In;			// Addition by W.A.P.
#else
	typedef const Vector3 &Vector3Param;		// Win32 seems to optimize this better.
	typedef const Vector4 &Vector4Param;
	typedef const Vector4 &Param;
	typedef Vector3& Vector3Ref;
	typedef Vector4& Vector4Ref;
	typedef Vector4& Ref;

	typedef const __vector4& Vector4In;		// Addition by W.A.P.
#endif

	// PURPOSE: Make zero'ing a vector easy, like Vector3 v(Vector3::ZeroType).
	enum _ZeroType		{ ZeroType };

	// PURPOSE: Default constructor
	// NOTES If __INIT_NAN is defined, the default constructor initializes all components to NaN.
	Vector4();

	// PURPOSE: Zero'ing constructor
	Vector4( _ZeroType );

	// PURPOSE: Constructor taking initial values for each component.
	Vector4(float x0,float y0,float z0,float w0);

	// PURPOSE: Constructor taking an initial value for all components.
	Vector4(float v);

	// PURPOSE: Constructor taking another vector to set initial value to.
	Vector4(const Vector4 &vec);

	// PURPOSE: Constructor taking another vector to set initial value to.
	Vector4(const Vector3 &vec)
		: xyzw(vec.xyzw)
	{		
	}

	// PURPOSE: For the new vec lib scalar fallback option, SCALAR_TYPES_ONLY == 1 in vectorconfig.h.
	Vector4(const Vec::Vector_4& vec)
		: x(vec.x), y(vec.y), z(vec.z), w(vec.w)
	{
	}

	// PURPOSE: Resource constructor, for vectors created from a resource
	// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
	Vector4(class datResource&);

	// PURPOSE: Used by the rorc resourcing system
	DECLARE_DUMMY_PLACE(Vector4);

#if RSG_CPU_INTEL
	// PURPOSE: Construct a Vector4 from a platform specific data type
	Vector4(const __m128& set) {xyzw = set;}
#elif __XENON || __PS3 || __PSP2
	// PURPOSE: Construct a Vector4 from a platform specific data type
	Vector4(const __vector4& set) { xyzw = set; }
#endif	// __XENON


	//=========================================================================
	// Get/Set functions

	// PURPOSE: Returns the x component of this vector.
	float GetX() const;

	// PURPOSE: Returns the y component of this vector.
	float GetY() const;

	// PURPOSE: Returns the z component of this vector.
	float GetZ() const;

	// PURPOSE: Returns the w component of this vector.
	float GetW() const;

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

	// PURPOSE: Set the w component of this vector.
	// PARAMS
	//    f - The new value for the w component.
	void SetW(float f);

	// PURPOSE: Set all the components of this vector equal to the x component of this vector
	// SEE ALSO:
	//		SplatY, SplatZ, SplatW
	void SplatX();

	// PURPOSE: Set all the components of this vector equal to the y component of this vector
	// SEE ALSO:
	//		SplatX, SplatZ, SplatW
	void SplatY();

	// PURPOSE: Set all the components of this vector equal to the z component of this vector
	// SEE ALSO:
	//		SplatY, SplatX, SplatW
	void SplatZ();

	// PURPOSE: Set all the components of this vector equal to the w component of this vector
	// SEE ALSO:
	//		SplatY, SplatZ, SplatX
	void SplatW();

	// PURPOSE: Set all the components of this vector equal to the x component of the input vector
	// PARAMS
	//    in - The vector whos x value is to be replicated across all components of this vector
	// SEE ALSO:
	//		SplatY, SplatZ, SplatW
	void SplatX(Vector4Param in);

	// PURPOSE: Set all the components of this vector equal to the y component of the input vector
	// PARAMS
	//    in - The vector whos y value is to be replicated across all components of this vector
	// SEE ALSO:
	//		SplatX, SplatZ, SplatW
	void SplatY(Vector4Param in);

	// PURPOSE: Set all the components of this vector equal to the z component of the input vector
	// PARAMS
	//    in - The vector whos z value is to be replicated across all components of this vector
	// SEE ALSO:
	//		SplatY, SplatX, SplatW
	void SplatZ(Vector4Param in);

	// PURPOSE: Returns the x component of this vector in all components of the returned vector.
	Vector4 GetXV() const;

	// PURPOSE: Returns the y component of this vector in all components of the returned vector.
	Vector4 GetYV() const;

	// PURPOSE: Returns the z component of this vector in all components of the returned vector.
	Vector4 GetZV() const;

	// PURPOSE: Returns the w component of this vector in all components of the returned vector.
	Vector4 GetWV() const;

	// PURPOSE: Set all the components of this vector equal to the w component of the input vector
	// PARAMS
	//    in - The vector whos w value is to be replicated across all components of this vector
	// SEE ALSO:
	//		SplatY, SplatZ, SplatX
	void SplatW(Vector4Param in);

	// PURPOSE: Set this vector's value from four floats.
	// PARAMS
	//	sx - The new x component.
	//	sy - The new y component.
	//	sz - The new z component.
	//	sw - The new w component.
	void Set(float x0,float y0,float z0,float w0);		

	// PURPOSE: Set this vector's value from another vector.
	// PARAMS
	//   a - The source vector, containing this vector's new value.
	void Set(const Vector4& a);

	// PURPOSE: Set all components of a vector to the same value.
	// PARAMS
	//   s - The new value for all the vector's components.
	void Set(float s);
	
	// PURPOSE: Set a vector to the scaled value of another vector.
	// PARAMS
	//   a - The source vector.
	//   s - The factor by which every component in a gets multiplied to produce the final vector.
	void SetScaled(const Vector4& a, float s);

	// PURPOSE: Set a vector to the scaled value of another vector.
	// PARAMS
	//   a - The source vector.
	//   s - The factor by which every component in a gets multiplied to produce the final vector.
	void SetScaled(Vector4Param a, Vector4Param s);

	// PURPOSE: Set a vector to the origin.
	void Zero();

	// PURPOSE: Fill a Vector3 with the x, y, and z components of this vector
	// PARAMS
	//	vec - the Vector3 to be filled
	// NOTES:
	//	If VECTORIZED_PADDING is enabled, the w component will be copied as well
	void GetVector3(Vector3& vec) const;

	// PURPOSE: Returns a Vector3 filles with the x, y, and z components of this vector
	// NOTES:
	//	If VECTORIZED_PADDING is enabled, the w component will be copied as well
	Vector3 GetVector3() const;

	// PURPOSE: Set the x, y, and z components of this vector to the values of the Vector3 being passed in
	// PARAMS
	//	vec - the Vector3 to set from
	// NOTES:
	//	If VECTORIZED_PADDING is enabled, the w component will be copied as well
	void SetVector3(Vector3Param vec);

	// PURPOSE: Set the x, y, and z components of this vector to the values of the Vector3 being passed in
	//			and clear the w component
	// PARAMS
	//	vec - the Vector3 to set from
	void SetVector3ClearW(Vector3Param vec);

	// PURPOSE: Set the x, y, and z components of this vector to the values of the Vector3 being passed in
	// PARAMS
	//	vec - the Vector3 to set from
	void SetVector3LeaveW(Vector3Param vec);

	// PURPOSE: Clears (zeros) the W component of a vector
	void ClearW();

	// PURPOSE: Add a Vector3 to this vector.
	// PARAMS:
	//	vec - The Vector3 to add to this vector.
	// NOTES:
	//	If VECTORIZED_PADDING is enabled, the w component is added as well
	void AddVector3(Vector3Param vec);	

	// PURPOSE: Add the XYZ components of a Vector3 to this vector.
	// PARAMS:
	//	vec - The Vector3 to add to this vector.
	void AddVector3XYZ(Vector3Param vec);

	// PURPOSE: Pack a vector into 10, 10, 10, 2 format
	u32 Pack1010102() const;

	// PURPOSE: Unpack a vector from 10, 10, 10, 2 format
	void Unpack1010102(u32 packed);

	// PURPOSE: Packs the 4 components into the 32 bit x component as 8 bit integers
	void PackColor();

	// PURPOSE: Unpacks each 8 bit color channel of the x component into the 4 components
	void UnpackColor();

	/********** Standard Algebra **********/
	// PURPOSE: Scale (multiply) each component of this vector by a float.
	// PARAMS
	//   f - The value to scale the vector by.
	void Scale(float f);
	
	// VECTORIZED.
	// (Vector4 really needs to be flushed out, or else use the new vec lib).
	void Scale(const Vector4& f);

	// PURPOSE: Set this vector to the value of another vector scaled by a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The value to scale the vector by.
	void Scale(const Vector4& a,float f);					

	// PURPOSE: Scale (multiply) the x, y, and z components of this vector by a float.
	// PARAMS
	//   f - The value to scale the vector by.
	void Scale3(float f);

	// PURPOSE: Set this vector's x, y, and z components to the value of another vector's x, y, and z components scaled by a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The value to scale the vector by.
	void Scale3(const Vector4& a, float f);

	// PURPOSE: Scale a vector by the inverse of a value.
	// PARAMS
	//   f - The value to invert and then scale the vector by.
	void InvScale(float f);

	// PURPOSE: Scale a vector by the inverse of a value.
	// PARAMS
	//   f - The vector to invert and then scale the vector by.
	void InvScale(Vector4Param f);

	// PURPOSE: Set the current vector to the value of another vector scaled by the inverse of a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The value to invert and then scale the vector by.
	void InvScale(Vector4Param a, float f);

	// PURPOSE: Set the current vector to the value of another vector scaled by the inverse of a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The vector to invert and then scale the vector by.
	void InvScale(const Vector4& a, Vector3Param f);

	// PURPOSE: Set the current vector to the value of another vector scaled by the inverse of a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The vector to invert and then scale the vector by.
	void InvScale(Vector4Param a, Vector4Param f);

	// PURPOSE: Add distinct real numbers to the components of this vector.
	// PARAMS
	//   sx - The value to be added to the x component.
	//   sy - The value to be added to the y component.
	//   sz - The value to be added to the z component.
	//   sw - The value to be added to the w component.
	void Add(float sx, float sy, float sz, float sw);

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	void Add(Vector4Param a);				

	// PURPOSE: Set this vector to the result of adding two vectors together.
	// PARAMS
	//   a - The first vector to add.
	//   b - The second vector to add.
	void Add(Vector4Param a,Vector4Param b);

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
	void AddScaled(const Vector4& a,float s);

	// PURPOSE: Add the value of another vector, scaled by a value, to the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before adding it to the current vector.
	// SEE ALSO: SubtractScaled, Lerp
	void AddScaled(Vector4Param a,Vector4Param s);

	// PURPOSE
	//   Set the current vector to the sum of two other vectors, where one of the other two
	//   vectors is scaled first before adding.
	// PARAMS
	//   a - The vector that gets added without getting scaled first.
	//   b - The vector that gets scaled before being added to the other vector.
	//   s - The factor by which to scale the second vector before adding it to the first vector.
	// SEE ALSO: SubtractScaled,Lerp
	void AddScaled(const Vector4& a,const Vector4& b,float s);	

	// PURPOSE
	//   Set the current vector to the sum of two other vectors, where one of the other two
	//   vectors is scaled first before adding.
	// PARAMS
	//   a - The vector that gets added without getting scaled first.
	//   b - The vector that gets scaled before being added to the other vector.
	//   s - The factor by which to scale the second vector before adding it to the first vector.
	// SEE ALSO: SubtractScaled,Lerp
	void AddScaled(Vector4Param a,Vector4Param b,Vector4Param s);

	// PURPOSE: Add the xyz components of another vector, scaled by a value, to the current xyz components.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before adding it to the current vector.
	// SEE ALSO: SubtractScaled, Lerp
	void AddScaled3(const Vector4& a,float s);

	// PURPOSE: Add the xyz components of another vector, scaled by a value, to the current xyz components.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before adding it to the current vector.
	// SEE ALSO: SubtractScaled, Lerp
	void AddScaled3(const Vector4& a,const Vector4& s);

	// PURPOSE
	//   Set the current xyz components to the sum of two other vectors, where one of the other two
	//   vectors is scaled first before adding.
	// PARAMS
	//   a - The vector that gets added without getting scaled first.
	//   b - The vector that gets scaled before being added to the other vector.
	//   s - The factor by which to scale the second vector before adding it to the first vector.
	// SEE ALSO: SubtractScaled,Lerp
	void AddScaled3(const Vector4& a,const Vector4& b,float s);	

	// PURPOSE
	//   Set the current xyz components to the sum of two other vectors, where one of the other two
	//   vectors is scaled first before adding.
	// PARAMS
	//   a - The vector that gets added without getting scaled first.
	//   b - The vector that gets scaled before being added to the other vector.
	//   s - The factor by which to scale the second vector before adding it to the first vector.
	// SEE ALSO: SubtractScaled,Lerp
	void AddScaled3(const Vector4& a,const Vector4& b,const Vector4& s);	

	// PURPOSE: Subtract distinct real numbers from the components of this vector.
	// PARAMS
	//   sx - The value to be added to the x component.
	//   sy - The value to be added to the y component.
	//   sz - The value to be added to the z component.
	//	 sw - The value to be added to the w component.
	void Subtract(float sx, float sy, float sz, float sw);

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	void Subtract(Vector4Param a);

	// PURPOSE: Set the current vector to the difference between two other vectors.
	// PARAMS
	//   a - The minuend (i.e. the value that gets subtracted from).
	//   b - The subtrahend (i.e. the value that gets subtracted).
	void Subtract(Vector4Param a,Vector4Param b);

	// PURPOSE: Set the current vector to the difference between two other vectors.
	// PARAMS
	//   a - The minuend (i.e. the value that gets subtracted from).
	//   b - The subtrahend (i.e. the value that gets subtracted).
	// NOTES:
	//	If VECTORIZED_PADDING is enabled this function will subtract the w component as well
	void Subtract(Vector3Param a,Vector3Param b);		

	// PURPOSE: Subtract the value of another vector, scaled by a value, from the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before subtracting it from the current vector.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(const Vector4& a, float s);

	// PURPOSE: Subtract the value of another vector, scaled by a value, from the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before subtracting it from the current vector.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(Vector4Param a, Vector4Param s);

	// PURPOSE
	//   Set the current vector to the difference between two other vectors, where the subtrahend 
	//   is first scaled before subtracting it from the minuend.
	// PARAMS
	//   a - The minuend (i.e. the vector that gets subtracted from without getting scaled first).
	//   b - The subtrahend (i.e. the vector that gets scaled before being subtracted from the other vector).
	//   s - The factor by which to scale the subtrahend before subtracting it from the minuend.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(const Vector4& a, const Vector4& b,float s);

	// PURPOSE
	//   Set the current vector to the difference between two other vectors, where the subtrahend 
	//   is first scaled before subtracting it from the minuend.
	// PARAMS
	//   a - The minuend (i.e. the vector that gets subtracted from without getting scaled first).
	//   b - The subtrahend (i.e. the vector that gets scaled before being subtracted from the other vector).
	//   s - The factor by which to scale the subtrahend before subtracting it from the minuend.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(Vector4Param a, Vector4Param b,Vector4Param s);

	// PURPOSE: Multiply the current vector with another vector.
	// PARAMS
	//   a - The vector to multiply with the current vector.
	// NOTES
	//  - This just multiplies the components, i.e. (a,b,c,d)*(e,f,g,h) == (ae,bf,cg,dh).
	void Multiply(Vector4Param a);

	// PURPOSE: Set the current vector to the product of two other vectors.
	// PARAMS
	//   a - One of the vectors to multiply together.
	//   b - One of the vectors to multiply together.
	// NOTES
	//  - This just multiplies the components, i.e. (a,b,c,d)*(e,f,g,h) == (ae,bf,cg,dh).
	void Multiply(Vector4Param a,Vector4Param b);

	// PURPOSE: Negate the current vector.
	void Negate();

	// PURPOSE: Set the current vector to the negation of another vector.
	// PARAMS
	//   a - The vector to negate.
	void Negate(Vector4Param a);

	// PURPOSE: Set the components of a vector to their absolute value.
	void Abs();

	// PURPOSE: Set a vector to the absolute value of another vector.
	// PARAMS
	//   a - The vector whose absolute value must be calculated.
	void Abs(Vector4Param a);

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
	void Invert(Vector4Param a);

	// PURPOSE: Invert a vector, i.e. set each component to 1 divided by the component.
	// NOTES
	//   - This method does extra work to avoid dividing by zero.  Use Invert() if potentially dividing
	//     by zero is OK.
	// SEE ALSO: Invert
	void InvertSafe();

	// PURPOSE: Set a vector to the inverse of another vector, i.e. where each new component is 1 divided by the old component.
	// PARAMS
	//   a - The vector to invert.
	// NOTES
	//   - This method does extra work to avoid dividing by zero.  Use Invert() if potentially dividing
	//     by zero is OK.
	// SEE ALSO: Invert
	void InvertSafe(const Vector4& a);

	// PURPOSE: Normalize a vector, i.e. scale it so that its new magnitude is 1.
	void Normalize();

	// PURPOSE: Normalize a vector using a reciprocal sqrt estimate, i.e. scale it so that its new magnitude is 1.
	void NormalizeFast();

	// PURPOSE: Set the current vector to the normalization of another vector, i.e. scaled so that
	//    its new magnitude is 1.
	// PARAMS
	//   a - The vector to normalize.
	void Normalize(Vector4Param a);

	// PURPOSE: Set the current vector to the normalization of another vector, i.e. scaled so that
	//    its new magnitude is 1.
	// PARAMS
	//   a - The vector to normalize.
	// NOTES: This uses a reciprocal sqrt estimate
	void NormalizeFast(Vector4Param a);

	// PURPOSE: Normalize the xyz components of this vector, i.e. scale it so that its new magnitude is 1.
	// NOTES:
	//	 The w component is set to 1
	void Normalize3();

	// PURPOSE: Normalize xyz components of this vector using a reciprocal sqrt estimate, i.e. scale it so that its new magnitude is 1.
	// NOTES:
	//	 The w component is set to 1
	void NormalizeFast3();

	// PURPOSE: Set the current xyz components to the normalization of another vector, i.e. scaled so that
	//    the new magnitude of xyz is 1.
	// PARAMS
	//   a - The vector to normalize.
	// NOTES:
	//	 The w component is set to 1
	void Normalize3(Vector4Param a);

	// PURPOSE: Set the current xyz components to the normalization of another vector, i.e. scaled so that
	//      the new magnitude of xyz is 1.
	// PARAMS
	//   a - The vector to normalize.
	// NOTES: 
	//	This uses a reciprocal sqrt estimate
	//	The w component is set to 1
	void NormalizeFast3(Vector4Param a);

	// PURPOSE: Calculate the dot product of this vector and another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: the dot product.
	// SEE ALSO: DotV, Dot3, Dot3V
	float Dot(Vector4Param a) const;

	// PURPOSE: Calculate the dot product of this vector and another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: a vector with the dot product in all components
	// SEE ALSO: Dot, Dot3, Dot3V
	Vector4 DotV(Vector4Param a) const;

	// PURPOSE: Calculate the dot product of the xyz component of this vector and the xyz component of another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: the dot product.
	// SEE ALSO: DotV, Dot, Dot3V
	float Dot3(Vector4Param a) const;

	// PURPOSE: Calculate the dot product of the xyz component of this vector and the xyz component of another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: the dot product.
	// SEE ALSO: DotV, Dot, Dot3V
	float Dot3(Vector3Param a) const;

	// PURPOSE: Calculate the dot product of the xyz component of this vector and the xyz component of another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: a vector with the dot product in all components.
	// SEE ALSO: DotV, Dot3, Dot3
	Vector4 Dot3V(Vector4Param a) const;

	// PURPOSE: Calculate the dot product of the xyz component of this vector and the xyz component of another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: a vector with the dot product in all components.
	// SEE ALSO: DotV, Dot3, Dot3
	Vector4 Dot3V(Vector3Param a) const;

	// PURPOSE: Calculate the cross product of this vector and another vector.
	// PARAMS
	//   b - The second term in the cross product.  (The current vector is the first term.)
	void Cross(Vector3Param b);

	// PURPOSE: Calculate the cross product of this vector and another vector.
	// PARAMS
	//   b - The second term in the cross product.  (The current vector is the first term.)
	void Cross(const Vector4& b);

	// PURPOSE: Set the current vector to the cross product of two other vectors.
	// PARAMS
	//   a - The first term in the cross product.
	//   b - The second term in the cross product.
	void Cross(Vector3Param a,Vector3Param b);	

	// PURPOSE: Set the current vector to the cross product of two other vectors.
	// PARAMS
	//   a - The first term in the cross product.
	//   b - The second term in the cross product.
	void Cross(const Vector4& a,Vector3Param b);

	// PURPOSE: Set the current vector to the cross product of two other vectors.
	// PARAMS
	//   a - The first term in the cross product.
	//   b - The second term in the cross product.
	void Cross(Vector3Param a,const Vector4& b);

	// PURPOSE: Set the current vector to the cross product of two other vectors.
	// PARAMS
	//   a - The first term in the cross product.
	//   b - The second term in the cross product.			
	void Cross(const Vector4& a,const Vector4& b);	

	// PURPOSE: Average two vectors together.
	// PARAMS
	//   a - The other vector.
	void Average(Vector4Param a);

	// PURPOSE: Set the current vector to the average of two other vectors.
	// PARAMS
	//   a,b - The vectors to average together.
	void Average(Vector4Param a, Vector4Param b);

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
	void Lerp(float t, const Vector4& a, const Vector4& b);

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
	void Lerp(Vector4Param t, Vector4Param a, Vector4Param b);

	// PURPOSE: Set the current vector to a linearly-interpolated value between two other vectors.
	// PARAMS
	//   a - The vector that represents a t value of 1.
	//   t - The interpolation value for the desired point.
	// NOTES
	//   - "Lerp" is a contraction for "linear interpolation".
	//   - If you already have the slope between this and a (i.e. a minus this), AddScaled() is a more
	//     efficient way to accomplish what Lerp() does.
	// SEE ALSO: AddScaled,SubtractScaled
	void Lerp(float t, const Vector4& a);

	// PURPOSE: Set the current vector to a linearly-interpolated value between two other vectors.
	// PARAMS
	//   a - The vector that represents a t value of 1.
	//   t - The interpolation value for the desired point.
	// NOTES
	//   - "Lerp" is a contraction for "linear interpolation".
	//   - If you already have the slope between this and a (i.e. a minus this), AddScaled() is a more
	//     efficient way to accomplish what Lerp() does.
	// SEE ALSO: AddScaled,SubtractScaled
	void Lerp(Vector4Param t, Vector4Param a);

	// PURPOSE: Raise x to the power of y in all components.
	// PARAMS
	//	x - The value to be raised
	//	y - The power to raise to
	// NOTES: This uses estimate functions on the xenon so may not be very accurate
	void Pow(Vector4Param x, Vector4Param y);

	// PURPOSE: Raise 2 to the power of x in all components
	// PARAMS:
	//	x - The power to raise
	// NOTES: This uses estimate functions on the xenon so may not be very accurate
	void Exp(Vector4Param x);

	// PURPOSE:	Compute the base-2 logarithm of each component of this vector
	// NOTES: Each component is set to its base-2 logarithm
	//		log(x), log(y), log(z)
	void Log();

	// PURPOSE:	Compute the base-2 logarithm of each component of the input vector
	// PARAMS:
	//	v - The vector to compute the logarithm of
	// NOTES: Each component is set to the base-2 logarithm of the input vector
	//		log(v.x), log(v.y), log(v.z)
	void Log(Vector4Param v);

	// PURPOSE:	Compute the base-10 logarithm of each component of this vector
	// NOTES: Each component is set to its base-2 logarithm
	//		log10(x), log10(y), log10(z)
	void Log10();

	// PURPOSE:	Compute the base-10 logarithm of each component of the input vector
	// PARAMS:
	//	v - The vector to compute the logarithm of
	// NOTES: Each component is set to the base-10 logarithm of the input vector
	//		log10(v.x), log10(v.y), log10(v.z)
	void Log10(Vector4Param v);

	//============================================================================
	// Magnitude and distance

	// PURPOSE: Compute the sqrt of each of the components
	// RETURNS: A vector containing the sqrt of each component
	Vector4 SqrtV() const;

	// PURPOSE: Calculate the magnitude (i.e. length) of this vector.
	// RETURNS: the length of this vector.
	// NOTES
	//   - This function involves a square-root.  If your need for the vector's length could be
	//     satisfied by the square of the vector's length, consider using Mag2() instead.
	// SEE ALSO: InvMag, Mag2, Mag3, Mag32, MagV, Mag2V, Mag3V, Mag32V
	float Mag() const;

	// PURPOSE: Calculate the magnitude (i.e. length) of this vector.
	// RETURNS: a vector containing the length of this vector in every component.
	// NOTES
	//   - This function involves a square-root.  If your need for the vector's length could be
	//     satisfied by the square of the vector's length, consider using Mag2() instead.
	// SEE ALSO: InvMag, Mag2, Mag3, Mag32, Mag, Mag2V, Mag3V, Mag32V
	Vector4 MagV() const;

	// PURPOSE: Calculate the squared magnitude (i.e. squared length) of this vector.
	// RETURNS: the squared length of this vector.
	// NOTES
	//   If you actually need the length, consider using Mag() instead.
	// SEE ALSO: InvMag, MagV, Mag3, Mag32, Mag, Mag2V, Mag3V, Mag32V
	float Mag2() const;

	// PURPOSE: Calculate the squared magnitude (i.e. squared length) of this vector.
	// RETURNS: a vector containing the squared length of this vector in every component.
	// NOTES
	//   If you actually need the length, consider using Mag() instead.
	// SEE ALSO: InvMag, MagV, Mag3, Mag32, Mag, Mag2, Mag3V, Mag32V
	Vector4 Mag2V() const;

	// PURPOSE: Calculate the magnitude (i.e. length) of the xyz components.
	// RETURNS: the length of the xyz components.
	// NOTES
	//   - This function involves a square-root.  If your need for the vector's length could be
	//     satisfied by the square of the vector's length, consider using Mag2() instead.
	// SEE ALSO: InvMag, Mag2, Mag, Mag32, MagV, Mag2V, Mag3V, Mag32V
	float Mag3() const;

	// PURPOSE: Calculate the magnitude (i.e. length) of the xyz components.
	// RETURNS: a vector containing the length of the xyz components in every component.
	// NOTES
	//   - This function involves a square-root.  If your need for the vector's length could be
	//     satisfied by the square of the vector's length, consider using Mag2() instead.
	// SEE ALSO: InvMag, Mag2, Mag3, Mag32, Mag, Mag2V, Mag, Mag32V
	Vector4 Mag3V() const;

	// PURPOSE: Calculate the squared magnitude (i.e. squared length) of the xyz components.
	// RETURNS: the squared length of the xyz components.
	// NOTES
	//   If you actually need the length, consider using Mag() instead.
	// SEE ALSO: InvMag, MagV, Mag3, Mag2, Mag, Mag2V, Mag3V, Mag32V
	float Mag32() const;

	// PURPOSE: Calculate the squared magnitude (i.e. squared length) of the xyz components.
	// RETURNS: a vector containing the squared length of the xyz components in every component.
	// NOTES
	//   If you actually need the length, consider using Mag() instead.
	// SEE ALSO: InvMag, MagV, Mag3, Mag32, Mag, Mag2, Mag3V, Mag2V
	Vector4 Mag32V() const;

	// PURPOSE: Get the inverse magnitude of a vector.
	// RETURNS: the inverse magnitude (i.e. 1.0f / magnitude).
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag32V, MagV, Mag3, Mag32, Mag, Mag2, Mag3V, Mag2V, InvMagV, InvMag3, InvMag3V
	float InvMag() const;	

	// PURPOSE: Get the inverse magnitude of a vector.
	// RETURNS: a vector containing the inverse magnitude (i.e. 1.0f / magnitude) in every component.
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag32V, MagV, Mag3, Mag32, Mag, Mag2, Mag3V, Mag2V, InvMag, InvMag3, InvMag3V
	Vector4 InvMagV() const;

	// PURPOSE: Get the inverse magnitude of the xyz components.
	// RETURNS: the inverse magnitude (i.e. 1.0f / magnitude).
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag32V, MagV, Mag3, Mag32, Mag, Mag2, Mag3V, Mag2V, InvMagV, InvMag, InvMag3V
	float InvMag3() const;	

	// PURPOSE: Get the inverse magnitude of the xyz components.
	// RETURNS: a vector containing the inverse magnitude (i.e. 1.0f / magnitude) in every component.
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag32V, MagV, Mag3, Mag32, Mag, Mag2, Mag3V, Mag2V, InvMag, InvMag3, InvMag3
	Vector4 InvMag3V() const;

	// PURPOSE: Calculate the distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the distance between the two points described by the vectors.
	// NOTES
	//   - This is equivalent to (*this - a).Mag().
	//   - This function involves a square-root.  If your need for the distance could be
	//     satisfied by the square of the distance, consider using Dist2() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, InvDist, InvDistV, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	float Dist(Vector4Param a) const;

	// PURPOSE: Calculate the distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the distance between the two points described by the vectors in every component
	// NOTES
	//   - This is equivalent to (*this - a).Mag().
	//   - This function involves a square-root.  If your need for the distance could be
	//     satisfied by the square of the distance, consider using Dist2() instead.
	// SEE ALSO: Dist, Dist2, Dist2V, InvDist, InvDistV, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	Vector4 DistV(Vector4Param a) const;
	
	// PURPOSE: Calculate the inverse distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the inverse distance between the two points described by the vectors (i.e. 1 divided by the distance).
	// NOTES
	//   - This function involves an inverse square-root.  If your need for the inverse distance could be
	//     satisfied by the square of the inverse distance, consider using InvDist2() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, Dist, InvDistV, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	float InvDist(Vector4Param a) const;

	// PURPOSE: Calculate the inverse distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the inverse distance between the two points described by the 
	//			vectors (i.e. 1 divided by the distance) in every component.
	// NOTES
	//   - This function involves an inverse square-root.  If your need for the inverse distance could be
	//     satisfied by the square of the inverse distance, consider using InvDist2() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, InvDist, Dist, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	Vector4 InvDistV(Vector4Param a) const;
	
	// PURPOSE: Calculate the squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the squared distance between the two points described by the vectors.
	// NOTES
	//   - This is equivalent to (*this - a).Mag2().
	//   - If you actually need the distance and not the squared distance, consider using Dist() instead.
	// SEE ALSO: DistV, Dist, Dist2V, InvDist, InvDistV, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	float Dist2(Vector4Param a) const;

	// PURPOSE: Calculate the squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the squared distance between the two points described by the vectors in every component
	// NOTES
	//   - This is equivalent to (*this - a).Mag2().
	//   - If you actually need the distance and not the squared distance, consider using Dist() instead.
	// SEE ALSO: DistV, Dist2, Dist, InvDist, InvDistV, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	Vector4 Dist2V(Vector4Param a) const;
	
	// PURPOSE: Calculate the inverse squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the inverse squared distance between the two points described by the vectors.
	// NOTES
	//   - If you actually need the inverse distance and not the inverse squared distance, consider
	//     using InvDist() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, InvDist, InvDistV, Dist, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	float InvDist2(Vector4Param a) const;

	// PURPOSE: Calculate the inverse squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the inverse squared distance between the two points described by the vectors in every component.
	// NOTES
	//   - If you actually need the inverse distance and not the inverse squared distance, consider
	//     using InvDist() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, InvDist, InvDistV, InvDist2, Dist, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	Vector4 InvDist2V(Vector4Param a) const;

	// PURPOSE: Calculate the distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the distance between the two points described by the vectors.
	// NOTES
	//   - This is equivalent to (*this - a).Mag().
	//   - This function involves a square-root.  If your need for the distance could be
	//     satisfied by the square of the distance, consider using Dist2() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, InvDist, InvDistV, InvDist2, InvDist2V, Dist, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	float Dist3(Vector4Param a) const;

	// PURPOSE: Calculate the distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the distance between the two points described by the vectors in every component
	// NOTES
	//   - This is equivalent to (*this - a).Mag().
	//   - This function involves a square-root.  If your need for the distance could be
	//     satisfied by the square of the distance, consider using Dist2() instead.
	// SEE ALSO: Dist, Dist2, Dist2V, InvDist, InvDistV, InvDist2, InvDist2V, Dist3, DistV, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	Vector4 Dist3V(Vector4Param a) const;
	
	// PURPOSE: Calculate the inverse distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the inverse distance between the two points described by the vectors (i.e. 1 divided by the distance).
	// NOTES
	//   - This function involves an inverse square-root.  If your need for the inverse distance could be
	//     satisfied by the square of the inverse distance, consider using InvDist2() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, Dist, InvDistV, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist, InvDist3V, InvDist32, InvDist32V
	float InvDist3(Vector4Param a) const;

	// PURPOSE: Calculate the inverse distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the inverse distance between the two points described by the 
	//			vectors (i.e. 1 divided by the distance) in every component.
	// NOTES
	//   - This function involves an inverse square-root.  If your need for the inverse distance could be
	//     satisfied by the square of the inverse distance, consider using InvDist2() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, InvDist, Dist, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDistV, InvDist32, InvDist32V
	Vector4 InvDist3V(Vector4Param a) const;
	
	// PURPOSE: Calculate the squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the squared distance between the two points described by the vectors.
	// NOTES
	//   - This is equivalent to (*this - a).Mag2().
	//   - If you actually need the distance and not the squared distance, consider using Dist() instead.
	// SEE ALSO: DistV, Dist, Dist2V, InvDist, InvDistV, InvDist2, InvDist2V, Dist3, Dist3V, Dist2, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist32V
	float Dist32(Vector4Param a) const;

	// PURPOSE: Calculate the squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the squared distance between the two points described by the vectors in every component
	// NOTES
	//   - This is equivalent to (*this - a).Mag2().
	//   - If you actually need the distance and not the squared distance, consider using Dist() instead.
	// SEE ALSO: DistV, Dist2, Dist, InvDist, InvDistV, InvDist2, InvDist2V, Dist3, Dist3V, Dist32, Dist2V, InvDist3, InvDist3V, InvDist32, InvDist32V
	Vector4 Dist32V(Vector4Param a) const;
	
	// PURPOSE: Calculate the inverse squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the inverse squared distance between the two points described by the vectors.
	// NOTES
	//   - If you actually need the inverse distance and not the inverse squared distance, consider
	//     using InvDist() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, InvDist, InvDistV, Dist, InvDist2V, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist2, InvDist32V
	float InvDist32(Vector4Param a) const;

	// PURPOSE: Calculate the inverse squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the inverse squared distance between the two points described by the vectors in every component.
	// NOTES
	//   - If you actually need the inverse distance and not the inverse squared distance, consider
	//     using InvDist() instead.
	// SEE ALSO: DistV, Dist2, Dist2V, InvDist, InvDistV, InvDist2, Dist, Dist3, Dist3V, Dist32, Dist32V, InvDist3, InvDist3V, InvDist32, InvDist2V
	Vector4 InvDist32V(Vector4Param a) const;

	//============================================================================
	// Conversion functions

	// PURPOSE: Converts components from float to signed integer in place
	void FloatToInt();

	// PURPOSE: Converts components from signed integer to float in place
	void IntToFloat();

	// PURPOSE: Rounds components to nearest integral float
	void RoundToNearestInt();

	// PURPOSE: Rounds components to nearest integral float toward zero
	void RoundToNearestIntZero();

	// PURPOSE: Rounds components to nearest integral float toward negative infinity
	void RoundToNearestIntNegInf();

	// PURPOSE: Rounds components to nearest integral float toward positive infinity
	void RoundToNearestIntPosInf();

	//============================================================================
	// Comparison functions

	// PURPOSE: Determine whether a vector is equal to the origin.
	// RETURNS: true if the vector is equal to the origin, false otherwise.
	// SEE ALSO: IsNonZero
	bool IsZero() const;

	// PURPOSE: Determine whether a vector is not equal to the origin.
	// RETURNS: true if the vector is not equal to the origin, false otherwise.
	// SEE ALSO: IsZero
	bool IsNonZero() const;

	// PURPOSE: Determine whether a vector is equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is equal to the other vector, false otherwise.
	// SEE ALSO: IsNotEqual,IsClose,IsEqualV
	bool IsEqual(Vector4Param a) const;

	// PURPOSE: Determine whether a vector is equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: all bits set for each component that is equal, all bits cleared for each component that is not equal
	// SEE ALSO: IsNotEqual,IsClose,IsEqual
	Vector4 IsEqualV(Vector4Param a) const;

	// PURPOSE: Determine whether a vector is equal to another vector using integer compares.
	// PARAMS
	//   a - The other vector.
	// RETURNS: all bits set for each component that is equal, all bits cleared for each component that is not equal
	// SEE ALSO: IsNotEqual,IsClose,IsEqual
#if __XENON
	Vector4 IsEqualIV(Vector4Param a) const;
#else
	Vector4 IsEqualIV(const Vector4& a) const;
#endif

	// PURPOSE: Determine whether a vector is not equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is not equal to the other vector, false otherwise.
	// SEE ALSO: IsEqual,IsClose
	bool IsNotEqual(Vector4Param a) const;

	// PURPOSE: Determine whether a vector is equal to another vector, within some tolerance value.
	// PARAMS
	//   a - The other vector.
	//   eps - The tolerance value.
	// RETURNS: true if the vector is equal to the other vector within the tolerance value, false otherwise.
	// SEE ALSO: IsEqual,IsNotEqual
	bool IsClose(const Vector4& a, float eps) const;

	// PURPOSE: Determine whether a vector is equal to another vector, within some tolerance value.
	// PARAMS
	//   a - The other vector.
	//   eps - The tolerance value.
	// RETURNS: true if the vector is equal to the other vector within the tolerance value, false otherwise.
	// SEE ALSO: IsEqual,IsNotEqual
	bool IsClose(Vector4Param a, Vector4Param eps) const;

	// PURPOSE: Determine whether a vector is greater than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is greater than the other vector
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThanV,IsGreaterThanVR
	bool IsGreaterThan(Vector4Param a) const;

	// PURPOSE: Determine whether a vector is greater than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThan,IsGreaterThanVR
	Vector4 IsGreaterThanV(Vector4Param a) const;

	// PURPOSE: Determine whether a vector is greater than another vector
	// PARAMS
	//   a - The other vector.
	//	 r - The register containing flags for this operation
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsLessThan,IsGreaterThanV,IsGreaterThanVR
#if __XENON
	Vector4 IsGreaterThanVR(Vector4Param a, u32& r) const;
#else
	Vector4 IsGreaterThanVR(const Vector4& a, u32& r) const;
#endif

	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is less than the other vector
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThanVR
	bool IsLessThanAll(Vector4Param a) const;	
	
	// PURPOSE: Compatibility with existing code previously written with IsLessThan
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is less than the other vector
	// NOTES: do not use this, as its effects are platform dependent!
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThanVR
	bool IsLessThanDoNotUse(Vector4Param a) const;

	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThan,IsLessThanVR
	Vector4 IsLessThanV(Vector4Param a) const;

	// PURPOSE: Determine whether a vector is less than another vector
	// PARAMS
	//   a - The other vector.
	//	 r - The register containing flags for this operation
	// RETURNS: a vector containing the test result, values are either zero for false or non zero for true
	// SEE ALSO: IsEqual,IsNotEqual,IsClose,IsGreaterThan,IsLessThanV,IsLessThan
#if __XENON
	Vector4 IsLessThanVR(Vector4Param a, u32& r) const;
#else
	Vector4 IsLessThanVR(const Vector4& a, u32& r) const;
#endif

	// PURPOSE: Select a vector based on the value of this vector
	// PARAMS
	//   zero - The vector to be returned if this vector is zero
	//	 nonZero - The vector to be returned if this vector is non zero
	// RETURNS: The parameter selected by the value of this vector
	// NOTES:
	//		This is a bitwise select,  for each bit in this vector it chooses a bit in either the zero vector or the nonZero vector
	//		If the bit is cleared, it uses the bit from the zero vector.
	//		If the bit is set, it uses the bit from the nonZero vector.
	Vector4 Select(Vector4Param zero, Vector4Param nonZero) const;

	// PURPOSE: Set each component to the maximum component from the two passed in vectors
	// PARAMS
	//	a - The first vector
	//	b - The second vector
	void Max(Vector4Param v1, Vector4Param v2);

	// PURPOSE: Set each component to the minimum component from the two passed in vectors
	// PARAMS
	//	a - The first vector
	//	b - The second vector
	void Min(Vector4Param v1, Vector4Param v2);

	//============================================================================
	// Plane Functions

	// PURPOSE: Given 3 points on a plane, calculate a Vector4 that represents the plane.
	// PARAMS:
	//	a, b, c - Three points on the plane.
	// SEE ALSO: DistanceToPlane, DistanceToPlaneV
	// NOTES:
	//	- The normal vector of the plane is stored in the x/y/z fields, and the distance from the origin is
	//		stored in the w field.
	//	- The points must be in counterclockwise order, or else the negative of the desired normal vector
	//		will be calculated.
	void ComputePlane(Vector3::Param a, Vector3::Param b, Vector3::Param c);

	// PURPOSE: Given a point on a plane & the plane's normal vector, calculate a Vector4 that represents the plane.
	// PARAMS:
	//	position - A point on the plane.
	//	direction - The plane's normal vector.
	// SEE ALSO: DistanceToPlane, DistanceToPlaneV
	// NOTES:
	//	- The normal vector of the plane is stored in the x/y/z fields, and the distance from the origin is
	//		stored in the w field.
	void ComputePlane(const Vector3 &position, const Vector3 & direction);

	// PURPOSE: Calculate the distance between a point and a plane.
	// PARAMS:
	//	a - The point, whose distance from the plane is being calculated.
	// RETURNS: The distance from the point to the plane
	// SEE ALSO: DistanceToPlaneV, ComputePlane
	float DistanceToPlane(Vector3Param a) const;

	// PURPOSE: Calculate the distance between a point and a plane.
	// PARAMS:
	//	a - The point, whose distance from the plane is being calculated.
	// RETURNS: The distance from the point to the plane
	// SEE ALSO: DistanceToPlaneV, ComputePlane
	Vector4 DistanceToPlaneV(Vector3Param a) const;	

	// PURPOSE: Calculate the distance between a point and a plane.
	// PARAMS:
	//	a - The point, whose distance from the plane is being calculated.
	// RETURNS: The distance from the point to the plane
	// SEE ALSO: DistanceToPlaneV, ComputePlane
	Vector4 DistanceToPlaneV(Vector4Param a) const;	

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
	void Print(const char *label, bool newline=true) const;

	
	//============================================================================
	// Matrix operations

	// PURPOSE: Transform the current vector by the given matrix.
	// PARAMS
	//   mtx - The matrix containing the transformation to apply to the vector.
	// SEE ALSO
	//   Dot3x3
	// NOTES
	//   Defined in matrix44.h
	Vector4& Dot(const Vector4&,const Matrix44&);

	// PURPOSE:   Set the current vector to the transformation of another vector by the 3x3 part of a given matrix.
	// PARAMS
	// v - The vector to be transformed.
	// mtx - The matrix containing the 3x3 transformation to apply to the vector.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 4x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x4 part represents a position.
	//   Defined in matrix44.h
	// SEE ALSO
	//   Dot
	Vector4& Dot3x3(const Vector4&,const Matrix44&);

	//============================================================================
	// Bitwise operations

	// PURPOSE: Bitwise and of this vector with another vector
	// PARAMS
	//   and - The vector to and with this vector
	// See also:
	//		Or, Xor, MergeXY, MergeZW
	void And(Vector4Param _and);

	// PURPOSE: Bitwise or of this vector with another vector
	// PARAMS
	//   and - The vector to or with this vector
	// See also:
	//		And, Xor, MergeXY, MergeZW
	void Or(Vector4Param _or);

	// PURPOSE: Bitwise xor of this vector with another vector
	// PARAMS
	//   and - The vector to xor with this vector
	// See also:
	//		Or, And, MergeXY, MergeZW
	void Xor(Vector4Param _xor);

	// PURPOSE: Merge the X and Y components of two vectors
	// PARAMS
	//   vY - Operand
	// NOTES:
	//		The resulting vector is laid out as follows
	//			x, vYx, y, vYy
	// See also:
	//		Or, Xor, And, MergeZW
	void MergeXY(Vector4Param vY);

	// PURPOSE: Merge the X and Y components of two vectors
	// PARAMS
	//	vX - Operand
	//	vY - Operand
	// NOTES:
	//		The resulting vector is laid out as follows
	//			vXx, vYx, vXy, vYy
	// See also:
	//		Or, Xor, And, MergeZW
	void MergeXY(Vector4Param vX, Vector4Param vY);

	// PURPOSE: Merge the W and Z components of two vectors
	// PARAMS
	//   vW - Operand
	// NOTES:
	//		The resulting vector is laid out as follows
	//			z, vWz, w, vWw
	// See also:
	//		Or, Xor, MergeXY, And
	void MergeZW(Vector4Param vW);

	// PURPOSE: Merge the Z and W components of two vectors
	// PARAMS
	//	vZ - Operand
	//	vW - Operand
	// NOTES:
	//		The resulting vector is laid out as follows
	//			vZz, vWz, vZw, vWw
	// See also:
	//		Or, Xor, MergeXY, And
	void MergeZW(Vector4Param vZ, Vector4Param vW);

	// PURPOSE: Swap around components of the vector
	// PARAMS
	//  v - The source vector vector to permute
	//  permX - The component of the source vector to choose for the X component
	//  permY - The component of the source vector to choose for the Y component
	//  permZ - The component of the source vector to choose for the Z component
	//  permW - The component of the source vector to choose for the W component
	// NOTES:
	//  x, y, z, and w should be one of VEC_PERM_X, VEC_PERM_Y, VEC_PERM_Z, or VEC_PERM_W
	template <int permX, int permY, int permZ, int permW>
	void Permute(Vector4Param v);

	//============================================================================
	// Operators
#if __XENON || __PS3
	operator __vector4() const					{ return xyzw; }
	operator __vector4&()						{ return xyzw; }
#if __XENON
	operator vec_float4 () const				{ vec_float4 r; r.v = xyzw; return r; }
#endif
#elif RSG_CPU_INTEL
	operator __m128() const						{ return xyzw; }
	operator __m128&()							{ return xyzw; }
#endif

	__forceinline operator Vec::Vector_4() const 	{ return Vec::Vector_4(x,y,z,w); }
	__forceinline operator Vec::Vector_4&()			{ return *((Vec::Vector_4*)(this)); }

	// PURPOSE: Assignment operator usefule for compiler specific optimized implementations
	// PARAMS
	//	v - The vector to assign to this vector
	Vector4& operator=(const Vector4& a);

	// PURPOSE: Determine whether a vector is equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is equal to the other vector, false otherwise.
	// NOTES
	//   - This is equivalent to IsEqual().
	// SEE ALSO: IsEqual,IsNotEqual,IsClose
	bool operator==(Vector4Param a) const;

	// PURPOSE: Determine whether a vector is not equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is not equal to the other vector, false otherwise.
	// NOTES
	//   - This is equivalent to IsNotEqual().
	// SEE ALSO: IsEqual,IsNotEqual,IsClose
	bool operator!=(Vector4Param a) const;

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	// RETURNS: the sum of the two vectors.
	// SEE ALSO: Add
	Vector4 operator+(Vector4Param a) const;

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	// RETURNS: the difference between the two vectors.
	// SEE ALSO: Subtract
	Vector4 operator-(Vector4Param a) const;

	// PURPOSE: Negate a vector.
	// RETURNS: the negation of the current vector.
	// SEE ALSO: Negate
	Vector4 operator-() const;

	// PURPOSE: Multiply the current vector by a value.
	// PARAMS
	//   f - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	Vector4 operator*(const float f) const;	

	// PURPOSE: Multiply the current vector by a value.
	// PARAMS
	//   f - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	Vector4 operator*(Vector4Param f) const;	

	// PURPOSE: Divide the current vector by a value.
	// PARAMS
	//   f - The value to divide the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: InvScale
	Vector4 operator/(const float f) const;

	// PURPOSE: Divide the current vector by a vector.
	// PARAMS
	//   f - The vector to divide the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: InvScale
	Vector4 operator/(Vector4Param f) const;

	// PURPOSE: Bitwise or of this vector with another vector
	// PARAMS
	//   f - The vector to or with this vector
	// RETURNS: the combined vector
	Vector4 operator|(Vector4Param f) const;

	// PURPOSE: Bitwise and of this vector with another vector
	// PARAMS
	//   f - The vector to and with this vector
	// RETURNS: the bitwise and result vector
	Vector4 operator&(Vector4Param f) const;

	// PURPOSE: Bitwise xor of this vector with another vector
	// PARAMS
	//   f - The vector to xor with this vector
	// RETURNS: the bitwise xor result vector
	Vector4 operator^(Vector4Param f) const;

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	// RETURNS: the sum of the two vectors.
	void operator+=(Vector4Param V);

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	// RETURNS: the difference between the two vectors.
	void operator-=(Vector4Param V);

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
	void operator*=(Vector4Param f);

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
	void operator/=(Vector4Param f);

	// PURPOSE: Bitwise or of this vector with another vector
	// PARAMS
	//   f - The vector to or with this vector
	void operator|=(Vector4Param f);

	// PURPOSE: Bitwise and of this vector with another vector
	// PARAMS
	//   f - The vector to and with this vector
	void operator&=(Vector4Param f);

	// PURPOSE: Bitwise xor of this vector with another vector
	// PARAMS
	//   f - The vector to xor with this vector
	void operator^=(Vector4Param f);

	// PURPOSE: Access the i-th component of the vector as if it was an array.
	// PARAMS
	//   i - The array index of the component to access.  x is index 0, y is index 1, and z is index 2.
	const float &operator[](int i) const;

	// PURPOSE: Access the i-th component of the vector as if it was an array.
	// PARAMS
	//   i - The array index of the component to access.  x is index 0, y is index 1, and z is index 2.
    float &operator[](int i);

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif

	int GetSize() const { return 4; }

#if __XENON || __PS3
	union
	{
		__vector4	xyzw;
		_ivector4	ixyzw;
		_uvector4	uxyzw;
		struct 
		{ 
			float x;
			float y;
			float z;
			float w;
		};
		struct
		{
			int		ix;
			int		iy;
			int		iz;
			int		iw;
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
			int		ix;
			int		iy;
			int		iz;
			int		iw;
		};
	};
#elif RSG_CPU_INTEL
#ifdef _MSC_VER
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
			int		ix;
			int		iy;
			int		iz;
			int		iw;
		};
	};
#ifdef _MSC_VER
#pragma warning (default : 4201)			// nonstandard extension used : nameless struct/union
#endif
#else
	union
	{
		struct
		{
			float	x;
			float	y;
			float	z;
			float	w;
		};
		struct
		{
			int		ix;
			int		iy;
			int		iz;
			int		iw;
		};
	}
#endif
} ;

#if __XENON
#pragma warning(pop)
#endif


extern const Vector4 VECTOR4_ORIGIN;

extern const Vector4 VECTOR4_IDENTITY;

// PURPOSE: A vector whose component values are all 0.5f
extern const Vector4 VEC4_HALF;

extern const Vector4 VEC4_ANDW;
extern const Vector4 VEC4_ONEW;
extern const Vector4 VEC4_MASKW;

extern const Vector4 VEC4_255; // (255,255,255,255)

// PURPOSE: Convert a Vector3 to a Vector4.
// PARAMS:
//	dst - A place to write the converted Vector4.
//	src - The Vector3 that's being converted to a Vector4.
// RETURNS: the Vector4 version of the Vector3.
Vector4& Convert(Vector4&dest,Vector4::Vector3Param src);

// PURPOSE: Convert a Vector4 to a Vector3.
// PARAMS:
//	dst - A place to write the converted Vector3.
//	src - The Vector4 that's being converted to a Vector3.
// RETURNS: the Vector3 version of the Vector4.
// NOTES:
//	- This method doesn't divide the Vector4's x/y/z by w before storing them in the Vector3.
//		If you want that behavior, call ConvertFull().
//	- This method is probably a bug & should be removed.
Vector3& Convert(Vector3&dest,const Vector4& src);

// PURPOSE: Convert a Vector4 to a Vector3.
// PARAMS:
//	dst - A place to write the converted Vector3.
//	src - The Vector4 that's being converted to a Vector3.
// RETURNS: the Vector3 version of the Vector4.
Vector3& ConvertFull(Vector3&dest,const Vector4& src);

// PURPOSE: Serialize a vector object
inline datSerialize & operator<< (datSerialize &s, Vector4 &v) {
	s << v.x << v.y << v.z << v.w;
	return s;
}

namespace sysEndian
{
	template<> inline void SwapMe(Vector4& v) {
		SwapMe(v.x);
		SwapMe(v.y);
		SwapMe(v.z);
		SwapMe(v.w);
	}
} // namespace sysEndian

}	// namespace rage


//=============================================================================
// Implementations

// First, include platform specific implementation

#if RSG_CPU_INTEL && VECTORIZED
#include "vector/vector4_win32.h"
#elif __XENON || __PS3
#include "vector/vector4_xenon.h"
#endif

// Second, include standard version for any that weren't implemented in 
// the platform specific section
#include "vector/vector4_default.h"


#endif // VECTOR_VECTOR4_H
