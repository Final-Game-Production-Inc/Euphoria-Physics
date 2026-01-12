//
// vector/vector2.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_VECTOR2_H
#define VECTOR_VECTOR2_H

#include "data/resource.h"
#include "data/serialize.h"
#include "math/amath.h"
#include "math/constants.h"
#include "math/nan.h"
#include "system/endian.h"

namespace rage {

class Vector3;

//=============================================================================
// Vector2
//
// PURPOSE
//   Vector3 represents a vector in two-dimensional space.  The values in
//   dimensions are represented by float member variables: {x, y}
// NOTES
//   - Unlike most other classes in Rage, Vector2 does not store its member
//     variables as "protected" data.  This is for convenience due to the
//     widespread and frequent use of direct access to the data.
// <FLAG Component>
//
class Vector2
{
public:
	//=========================================================================
	// Construction

	DECLARE_DUMMY_PLACE(Vector2);

	// PURPOSE: Default constructor
	// NOTES If __INIT_NAN is defined, the default constructor initializes all components to NaN.
	Vector2();

	// PURPOSE: Constructor taking initial values for each component.
	Vector2(float setX, float setY);

	// PURPOSE: Constructor taking another vector to set initial value to.
	Vector2(const Vector2& vec);

	// PURPOSE: Resource constructor, for vectors created from a resource
	// NOTES: This is here so that we can bypass the default ctor when using placement new, does not init vars.
	Vector2(class datResource&);

	// PURPOSE: Constructor taking a Vector3 to project down into two dimensions specified.
	// PARAMS:
	//   v3d - The Vector3 to initialize from.
	//   axes - Specifies the 2 axes of the Vector3 to use.
	// NOTES:
	//   - The enum eVector3Axes that specifies the dimensions to use is also used in related
	//     functions that convert from Vector3 <-> Vector2.
	enum eVector3Axes { kXY, kXZ, kYZ };
	Vector2(const Vector3 & v3d, eVector3Axes axes);

	//=========================================================================
	// Accessors

	// PURPOSE: Access the i-th component of the vector as if it was an array.
	// PARAMS
	//   i - The array index of the component to access.  x is index 0, y is index 1, and z is index 2.
	const float& operator[](int i) const;

	// <COMBINE Vector2::[]>
	float& operator[](int i);

	// PURPOSE: Set this vector's value from another vector.
	// PARAMS
	//   a - The source vector, containing this vector's new value.
	void Set(const Vector2 &a);

	// PURPOSE: Set this vector's value from two floats.
	// PARAMS
	//   sx - The new x component.
	//   sy - The new y component.
	void Set(float sx, float sy);

	// PURPOSE: Set all components of a vector to the same value.
	// PARAMS
	//   s - The new value for all the vector's components.
	void Set(float s);

	// PURPOSE: Set a vector to the scaled value of another vector.
	// PARAMS
	//   a - The source vector.
	//   s - The factor by which every component in a gets multiplied to produce the final vector.
	void SetScaled(const Vector2 & a, float s);

	// PURPOSE: Set a vector to the origin.
	void Zero();

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
	void Scale(const Vector2 & a, float f);

	// PURPOSE: Scale a vector by the inverse of a value.
	// PARAMS
	//   f - The value to invert and then scale the vector by.
	void InvScale(float f);

	// PURPOSE: Set the current vector to the value of another vector scaled by the inverse of a value.
	// PARAMS
	//   a - The vector to be scaled.
	//   f - The value to invert and then scale the vector by.
	void InvScale(const Vector2 & a, float f);

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	void Add(const Vector2 & a);

	// PURPOSE: Set this vector to the result of adding two vectors together.
	// PARAMS
	//   a - The first vector to add.
	//   b - The second vector to add.
	void Add(const Vector2 & a, const Vector2 & b);

	// PURPOSE: Add the value of another vector, scaled by a value, to the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before adding it to the current vector.
	// SEE ALSO: SubtractScaled, Lerp
	void AddScaled(const Vector2 & a, float s);

	// PURPOSE
	//   Set the current vector to the sum of two other vectors, where one of the other two
	//   vectors is scaled first before adding.
	// PARAMS
	//   a - The vector that gets added without getting scaled first.
	//   b - The vector that gets scaled before being added to the other vector.
	//   s - The factor by which to scale the second vector before adding it to the first vector.
	// SEE ALSO: SubtractScaled,Lerp
	void AddScaled(const Vector2 & a, const Vector2 & b, float s);

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	void Subtract(const Vector2 & a);

	// PURPOSE: Set the current vector to the difference between two other vectors.
	// PARAMS
	//   a - The minuend (i.e. the value that gets subtracted from).
	//   b - The subtrahend (i.e. the value that gets subtracted).
	void Subtract(const Vector2 & a, const Vector2 & b);

	// PURPOSE: Subtract the value of another vector, scaled by a value, from the current vector.
	// PARAMS
	//   a - The other vector.
	//   s - The factor by which to scale the other vector before subtracting it from the current vector.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(const Vector2 & a, float s);

	// PURPOSE
	//   Set the current vector to the difference between two other vectors, where the subtrahend 
	//   is first scaled before subtracting it from the minuend.
	// PARAMS
	//   a - The minuend (i.e. the vector that gets subtracted from without getting scaled first).
	//   b - The subtrahend (i.e. the vector that gets scaled before being subtracted from the other vector).
	//   s - The factor by which to scale the subtrahend before subtracting it from the minuend.
	// SEE ALSO: AddScaled,Lerp
	void SubtractScaled(const Vector2 & a, const Vector2 & b, float s);

	// PURPOSE: Multiply the current vector with another vector.
	// PARAMS
	//   a - The vector to multiply with the current vector.
	// NOTES
	//  - This just multiplies the components, i.e. (a,b,c)*(d,e,f) == (ad,be,cf).
	void Multiply(const Vector2 & a);

	// PURPOSE: Set the current vector to the product of two other vectors.
	// PARAMS
	//   a - One of the vectors to multiply together.
	//   b - One of the vectors to multiply together.
	// NOTES
	//  - This just multiplies the components, i.e. (a,b,c)*(d,e,f) == (ad,be,cf).
	void Multiply(const Vector2 & a, const Vector2 & b);

	// PURPOSE: Negate the current vector.
	void Negate();

	// PURPOSE: Set the current vector to the negation of another vector.
	// PARAMS
	//   a - The vector to negate.
	void Negate(const Vector2 & a);

	// PURPOSE: Normalize a vector, i.e. scale it so that its new magnitude is 1.
	void Normalize();

	// PURPOSE: Set the current vector to the normalization of another vector, i.e. scaled so that
	//    its new magnitude is 1.
	// PARAMS
	//   a - The vector to normalize.
	void Normalize(const Vector2 & a);

	// PURPOSE: Normalize a vector, i.e. scale it so that its new magnitude is 1.
	// RETURNS: True if the vector was normalized, false if the fallback was use.
	// NOTES If the vector is currently near-zero, then return set to fallbackVec instead.
	bool NormalizeSafe(const Vector2 & fallbackVec = Vector2(1.0f,0.0f));

	// PURPOSE: Rotate a vector by a given number of radians around the Z axis.
	// PARAMS
	//   radians - How much to rotate around the Z axis.
	void Rotate(float radians);

	// PURPOSE: Rotate a vector by a given number of radians around the Y axis,
	//   treating this vector as a vector in the XZ plane.
	// PARAMS
	//   radians - How much to rotate around the Y axis.
	void RotateY(float radians);

	// PURPOSE: Calculate the dot product of this vector and another vector.
	// PARAMS
	//   a - The second term in the dot product.  (The current vector is the first term.)
	// RETURNS: the dot product.
	// SEE ALSO: FlatDot
	float Dot(const Vector2 &a) const;

	// PURPOSE: Calculate the cross product of this vector and another vector.
	// PARAMS
	//   a - The second term in the cross product.  (The current vector is the first term.)
	// NOTE
	//	 This function treats this 2D vector as in the XY plane, and this function returns
	//   the Z value of the cross product.
	float Cross(const Vector2 &a) const;

	// PURPOSE: Calculate the cross product of this vector and another vector.
	// PARAMS
	//   a - The second term in the cross product.  (The current vector is the first term.)
	// NOTE
	//	 This function treats this 2D vector as in the XZ plane, and this function returns
	//   the Y value of the cross product.
	// SEE ALSO: Cross
	float CrossY(const Vector2 &a) const;

	// PURPOSE: Set the current vector to the average of two other vectors.
	// PARAMS
	//   a,b - The vectors to average together.
	void Average(const Vector2 & a, const Vector2 & b);

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
	void Lerp(float t, const Vector2 & a, const Vector2 & b);


	//=========================================================================
	// Magnitude and distance

	// PURPOSE: Calculate the magnitude (i.e. length) of this vector.
	// RETURNS: the length of this vector.
	// NOTES
	//   - This function involves a square-root.  If your need for the vector's length could be
	//     satisfied by the square of the vector's length, consider using Mag2() instead.
	// SEE ALSO: InvMag,Mag2
	float Mag() const;

	// PURPOSE: Get the inverse magnitude of a vector.
	// RETURNS: the inverse magnitude (i.e. 1.0f / magnitude).
	// NOTES
	//  - This involves 3 multiplies, 2 adds, and an inverse-square-root.
	// SEE ALSO: Mag,Mag2
	float InvMag() const;

	// PURPOSE: Calculate the squared magnitude (i.e. squared length) of this vector.
	// RETURNS: the squared length of this vector.
	// NOTES
	//   If you actually need the length, consider using Mag() instead.
	// SEE ALSO: Mag,InvMag
	float Mag2() const;

	// PURPOSE: Calculate the distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the distance between the two points described by the vectors.
	// NOTES
	//   - This function involves a square-root.  If your need for the distance could be
	//     satisfied by the square of the distance, consider using Dist2() instead.
	// SEE ALSO: InvDist,Dist2,InvDist2
	float Dist(const Vector2 & a) const;

	// PURPOSE: Calculate the squared distance between the current vector and another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: the squared distance between the two points described by the vectors.
	// NOTES
	//   - If you actually need the distance and not the squared distance, consider using Dist() instead.
	// SEE ALSO: Dist,InvDist,InvDist2
	float Dist2(const Vector2 & a) const;

	// PURPOSE:  Calculate the angle between the current vector and another vector.
	// PARAMS
	//   v - The other vector.
	// NOTES
	//   - If both vectors are guaranteed to be normalized, consider using FastAngle() instead.
	// SEE ALSO
	//   FastAngle
	float Angle(const Vector2 & v) const;

	// PURPOSE:  Calculate the angle between the current vector and another vector.
	// PARAMS
	//   v - The other vector.
	// NOTES
	//   - This method assumes that both vectors are normalized.  If they're not, use Angle() instead.
	// SEE ALSO: Angle
	float FastAngle(const Vector2 & v) const;

	// PURPOSE: Calculate the angle of rotation about the Y axis between this vector and
	//	  another vector, treating these as vectors are in the XZ plane.
	// PARAMS
	//   a - The other vector.
	// RETURNS: The angle of rotation about the Y axis that would change this vector
	//    to point in the same direction as vector a
	// NOTES
	//   - Unlike Angle(), this may return a negative angle, meaning a clockwise rotation
	//     about the Y axis.
	// SEE ALSO: CrossY,Angle
	float AngleY(const Vector2& v) const;

	// PURPOSE: Determine whether or not this point is on the left or right side of
	// another line.
	// PARAMS:
	//  p1 - First point (beginning) of the other line.
	//  p2 - Second point (end) of the other line.
	// RETURNS:
	//  A value >0 if the point is to the left of the line, 0 if the point is
	//  exactly on the line, and <0 if it is on the right side of the line.
	// NOTES:
	//  None of the vectors need to be normalized. The line is considered infinite.
	float ComputeSide(const Vector2 &p1, const Vector2 &p2) const;


	//============================================================================
	// Comparison functions

	// PURPOSE: Determine whether a vector is equal to the origin.
	// RETURNS: true if the vector is equal to the origin, false otherwise.
	// SEE ALSO: IsNonZero
	int IsZero() const;

	// PURPOSE: Determine whether a vector is not equal to the origin.
	// RETURNS: true if the vector is not equal to the origin, false otherwise.
	// SEE ALSO: IsZero
	int IsNonZero() const;

	// PURPOSE: Determine whether a vector is equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is equal to the other vector, false otherwise.
	// SEE ALSO: IsNotEqual,IsClose
	bool IsEqual(const Vector2 & a) const;

	// PURPOSE: Determine whether a vector is not equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is not equal to the other vector, false otherwise.
	// SEE ALSO: IsEqual,IsClose
	bool IsNotEqual(const Vector2 & a) const;

	// PURPOSE: Determine whether a vector is equal to another vector, within some tolerance value.
	// PARAMS
	//   a - The other vector.
	//   eps - The tolerance value.
	// RETURNS: true if the vector is equal to the other vector within the tolerance value, false otherwise.
	// SEE ALSO: IsEqual,IsNotEqual
	bool IsClose(const Vector2 & a, float eps) const;


	//============================================================================
	// Operators

	// PURPOSE: Determine whether a vector is equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is equal to the other vector, false otherwise.
	// NOTES
	//   - This is equivalent to IsEqual().
	// SEE ALSO: IsEqual,IsNotEqual,IsClose
	bool operator==(const Vector2 & a) const;

	// PURPOSE: Determine whether a vector is not equal to another vector.
	// PARAMS
	//   a - The other vector.
	// RETURNS: true if the vector is not equal to the other vector, false otherwise.
	// NOTES
	//   - This is equivalent to IsNotEqual().
	// SEE ALSO: IsEqual,IsNotEqual,IsClose
	bool operator!=(const Vector2 & a) const;

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	// RETURNS: the sum of the two vectors.
	// SEE ALSO: Add
	Vector2 operator+(const Vector2 &a) const;

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	// RETURNS: the difference between the two vectors.
	// SEE ALSO: Subtract
	Vector2 operator-(const Vector2 &a) const;

	// PURPOSE: Negate a vector.
	// RETURNS: the negation of the current vector.
	// SEE ALSO: Negate
	Vector2 operator-() const;

	// PURPOSE: Multiply the current vector by a value.
	// PARAMS
	//   f - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	Vector2 operator*(const float f) const;

	// PURPOSE: Multiply the current vector by another vector
	// PARAMS
	//   a - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	Vector2 operator*(const Vector2& a) const;

	// PURPOSE: Divide the current vector by a value.
	// PARAMS
	//   f - The value to divide the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: InvScale
	Vector2 operator/(const float f) const;

	// PURPOSE: Add another vector to this vector.
	// PARAMS
	//   a - The vector to add to this vector.
	// RETURNS: the sum of the two vectors.
	void operator+=(const Vector2 & a);

	// PURPOSE: Subtract another vector from this vector.
	// PARAMS
	//   a - The vector to subtract from this vector.
	// RETURNS: the difference between the two vectors.
	void operator-=(const Vector2 & a);

	// PURPOSE: Multiply a vector by a value.
	// PARAMS
	//   V - the vector to multiply.
	//   f - The value to multiply the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: Scale
	void operator*=(const float f);

	// PURPOSE: Divide the current vector by a value.
	// PARAMS
	//   f - The value to divide the current vector by.
	// RETURNS: the scaled vector.
	// SEE ALSO: InvScale
	void operator/=(const float f);


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


	//=========================================================================
	// Data

	float x;
	float y;

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif
};

// PURPOSE: Serialize a vector object
inline datSerialize & operator<< (datSerialize &s, Vector2 &v) {
	s << v.x << v.y;
	return s;
}


//=============================================================================
// Implementations

inline Vector2::Vector2()
{
#if __INIT_NAN
	if (!g_DisableInitNan)
	{
		MakeNan(x);
		MakeNan(y);
	}
#endif // __INIT_NAN
}


inline Vector2::Vector2(float setX, float setY)
: x(setX)
, y(setY)
{
}


inline Vector2::Vector2(const Vector2& vec)
: x(vec.x)
, y(vec.y)
{
}


inline Vector2::Vector2(class datResource&)
{
}


inline void Vector2::Set(float sx,float sy)
{
	x=sx;
	y=sy;
}


inline void Vector2::Set(float s)
{
	x=s;
	y=s;
}


inline void Vector2::Set(const Vector2 &a)
{
	x=a.x;
	y=a.y;
}


inline void Vector2::SetScaled(const Vector2 &a, float s)
{
	x=s*a.x;
	y=s*a.y;
}


inline void Vector2::Zero()
{
	x=y=0.0f;
}


inline void Vector2::Add(const Vector2 &a)
{
	x+=a.x;
	y+=a.y;
}


inline void Vector2::Add(const Vector2 &a,const Vector2 &b)
{
	x=a.x+b.x;
	y=a.y+b.y;
}


inline void Vector2::Subtract(const Vector2 &a)
{
	x-=a.x;
	y-=a.y;
}


inline void Vector2::Subtract(const Vector2 &a,const Vector2 &b)
{
	x=a.x-b.x;
	y=a.y-b.y;
}


inline void Vector2::Scale(float f)
{
	x*=f;
	y*=f;
}


inline void Vector2::Scale(const Vector2 &a,float f)
{
	x=a.x*f;
	y=a.y*f;
}


inline void Vector2::InvScale(float f)
{
	float invF = 1.0f / f;
	x*=invF;
	y*=invF;
}


inline void Vector2::InvScale(const Vector2 & a,float f)
{
	float invF = 1.0f / f;
	x=a.x*invF;
	y=a.y*invF;
}


inline void Vector2::Multiply(const Vector2 &a)
{
	x*=a.x;
	y*=a.y;
}


inline void Vector2::Multiply(const Vector2 &a,const Vector2 &b)
{
	x=a.x*b.x;
	y=a.y*b.y;
}


inline void Vector2::Negate()
{
	x=-x;
	y=-y;
}


inline void Vector2::Negate(const Vector2 &a)
{
	x=-a.x;
	y=-a.y;
}


inline float Vector2::Dot(const Vector2 &a) const
{
	return x*a.x+y*a.y;
}


inline float Vector2::Cross(const Vector2 &a) const
{
	return x*a.y-y*a.x;
}


inline float Vector2::CrossY(const Vector2 &a) const
{
	return y*a.x-x*a.y;
}


inline float Vector2::Mag() const
{
	return sqrtf(x*x+y*y);
}


inline float Vector2::Mag2() const
{
	return x*x+y*y;
}


inline float Vector2::InvMag() const
{
	return invsqrtf(x*x+y*y);
}


inline void Vector2::Normalize()
{
	Scale(InvMag());
}


inline void Vector2::Normalize(const Vector2 &a)
{
	Scale(a,a.InvMag());
}


inline bool Vector2::NormalizeSafe(const Vector2 & fallbackVec)
{
	float mag2;

	mag2 = Mag2();

	// Note that we must have the condition for calling normalize be the
	// positive result of a comparison, since comparing with a NaN will always
	// return false.
	if(mag2 >= VERY_SMALL_FLOAT)
	{
		Normalize();
		return true;
	}
	else
	{
		*this = fallbackVec;
		return false;
	}
}


inline float Vector2::Dist(const Vector2 &a) const
{
	register float rx=x-a.x;
	register float ry=y-a.y;
	return sqrtf(rx*rx+ry*ry);
}


inline float Vector2::Dist2(const Vector2 &a) const
{
	register float rx=x-a.x;
	register float ry=y-a.y;
	return rx*rx+ry*ry;
}


inline void Vector2::AddScaled(const Vector2 &a,float s)
{
	x+=s*a.x;
	y+=s*a.y;
}


inline void Vector2::AddScaled(const Vector2 &a,const Vector2 &b,float s)
{
	x=a.x+s*b.x;
	y=a.y+s*b.y;
}


inline void Vector2::SubtractScaled(const Vector2 &a,float s)
{
	x-=s*a.x;
	y-=s*a.y;
}


inline void Vector2::SubtractScaled(const Vector2 &a,const Vector2 &b,float s)
{
	x=a.x-s*b.x;
	y=a.y-s*b.y;
}


inline void Vector2::Average(const Vector2 &a,const Vector2 &b)
{
	x=0.5f*(a.x+b.x);
	y=0.5f*(a.y+b.y);
}


inline void Vector2::Lerp(float t,const Vector2 &a,const Vector2 &b)
{
	x=a.x+(b.x-a.x)*t;
	y=a.y+(b.y-a.y)*t;
}


inline float Vector2::FastAngle(const Vector2 &v) const
{
	return acosf(Dot(v));
}


inline int Vector2::IsNonZero() const
{
	if(x!=0.0f)
		return 1;
	else if(y==0.0f)
		return 0;
	else
		return 1;
}


inline int Vector2::IsZero() const
{
	if(x!=0.0f)
		return 0;
	else if(y==0.0f)
		return 1;
	else
		return 0;
}


inline bool Vector2::IsEqual(const Vector2 &a) const
{
	return (x==a.x) && (y==a.y);
}


inline bool Vector2::operator==(const Vector2 &a) const
{
	return (x==a.x) && (y==a.y);
}


inline bool Vector2::IsNotEqual(const Vector2 &a) const
{
	return (x!=a.x) || (y!=a.y);
}


inline bool Vector2::operator!=(const Vector2 &a) const
{
	return (x!=a.x) || (y!=a.y);
}


inline bool Vector2::IsClose(const Vector2 &a,float eps) const
{
	return (x>=a.x-eps && x<=a.x+eps) && (y>=a.y-eps && y<=a.y+eps);
}


inline float Cross( const Vector2& u, const Vector2& v )
{
	return (u.x*v.y-v.x*u.y);
}


inline float CrossY( const Vector2& u, const Vector2& v )
{
	return (u.y*v.x-u.x*v.y);
}

inline const float& Vector2::operator[](int i) const
{
	FastAssert(i==0 || i==1);
	return(((const float*)this)[i]); //lint !e740 unusual pointer cast
}

inline float& Vector2::operator[](int i)
{
	FastAssert(i==0 || i==1);
	return(((float*)this)[i]); //lint !e740 unusual pointer cast
}


inline Vector2 Vector2::operator+(const Vector2 &a) const
{
	return(Vector2(x+a.x,y+a.y));
}


inline Vector2 Vector2::operator-(const Vector2 &a) const
{
	return(Vector2(x-a.x,y-a.y));
}


inline Vector2 Vector2::operator-() const
{
	return(Vector2(-x,-y));
}


inline Vector2 Vector2::operator*(const float f) const
{
	return(Vector2(x*f,y*f));
}
inline Vector2 Vector2::operator*(const Vector2& a) const
{
	return(Vector2(x*a.x,y*a.y));
}


inline Vector2 Vector2::operator/(const float f) const
{
	float of(1.0f/f);
	return(Vector2(x*of,y*of));
}


inline void Vector2::operator+=(const Vector2 &a)
{
	x+=a.x;
	y+=a.y;
}


inline void Vector2::operator-=(const Vector2 &a)
{
	x-=a.x;
	y-=a.y;
}


inline void Vector2::operator*=(const float f)
{
	x*=f;
	y*=f;
}


inline void Vector2::operator/=(const float f)
{
	float of(1.0f/f);
	x*=of;
	y*=of;
}


inline float Vector2::Angle(const Vector2 &v) const
{
	float mag2 = Mag2() * v.Mag2();

	return (mag2 > square(VERY_SMALL_FLOAT)) ? acosf(Clamp(Dot(v) / sqrtf(mag2), -1.f, 1.f)) : 0.f;
}


inline float Vector2::AngleY(const Vector2& v) const
{
	float mag2 = Mag2() * v.Mag2();

	return (mag2 > square(VERY_SMALL_FLOAT)) ? Selectf(CrossY(v), 1, -1) * acosf(Clamp((x * v.x + y * v.y) / sqrtf(mag2), -1.f, 1.f)) : 0.f;
}


inline void Vector2::Rotate(float radians)
{
	float tsin = sinf(radians);
	float tcos = cosf(radians);

	float t = x * tcos - y * tsin;
	y = x * tsin + y * tcos;
	x = t;
}


inline void Vector2::RotateY(float radians)
{
	float tsin = sinf(radians);
	float tcos = cosf(radians);

	float t = y * tcos - x * tsin;
	x = y * tsin + x * tcos;
	y = t;
}


namespace sysEndian
{
	template<> inline void SwapMe(Vector2& v) {
		SwapMe(v.x);
		SwapMe(v.y);
	}
} // namespace sysEndian


}	// namespace rage

#endif // VECTOR_VECTOR2_H
