//
// vector/quaternion.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_QUATERNION_H
#define VECTOR_QUATERNION_H

#include "data/serialize.h"
#include "vector/matrix34.h" //lint !e537 repeated include file

// Enable this to get the vectorized quaternion implementation.  Note: this is likely to be
// alot slower on Xenon
#define VECTORIZED_QUAT		(VECTORIZED && 0)

namespace rage {

enum eEulerOrders { eEulerOrderXYZ, eEulerOrderXZY, eEulerOrderYXZ, eEulerOrderYZX, eEulerOrderZXY, eEulerOrderZYX };

//=============================================================================
// Quaternion
//
// PURPOSE
//   A Quaternion is a 4-dimensional complex number, typically used to define a rotation.
// NOTES
//  - Unit quaternions represent rotations. The first three elements (x, y and z) are the unit axis of
//    rotation scaled by the sine of half the angle of rotation, and the last element (w) is the cosine
//    of half the angle of rotation.
// <FLAG Component>
//
class Quaternion
{
public:
	// PURPOSE: Make identity'ing a quaternion easy, like Quaternion q(Quaternion::IdentityType).
	enum _IdentityType		{ IdentityType };

	//=========================================================================
	// Construction

	// PURPOSE: Default constructor.
	// NOTES
	//   If <c>__INIT_NAN<\c> is defined, all values are initialized to <c>NaN</c>.
	Quaternion();

	// PURPOSE: Initializing constructor.
	// PARAMS
	//   x0, y0, z0, w0 - The components of the new quaternion.
	Quaternion(float x0, float y0, float z0, float w0);

	// PURPOSE: Initialize a quaternion to an identity quaternion.
	explicit Quaternion(_IdentityType type);


	Quaternion(__vector4 q);
	Quaternion(const Quaternion& q);
	Quaternion& operator= (const Quaternion& q);

	DECLARE_DUMMY_PLACE(Quaternion);

	//=========================================================================
	// Accessors

	// PURPOSE: Get this quaternion's angle of rotation.
	// RETURN: the angle of rotation of this quaternion
	// SEE ALSO: GetCosHalfAngle, GetDirection, RelAngle, RelCosHalfAngle, Dot
	float GetAngle() const;

	// PURPOSE: Get the cosine of half of this quaternion's angle of rotation.
	// RETURN: the cosine of half of this quaternion's angle of rotation
	// NOTES
	//   The w-value of a quaternion is exactly this value.
	// SEE ALSO: GetAngle, RelAngle, RelCosHalfAngle, Dot
	float GetCosHalfAngle() const;

	// PURPOSE: Set a vector to the quaternion's non-unit axis of rotation.
	// PARAMS
	//   outDirection - the direction of the quaternion's axis of rotation (not a unit vector)
	// NOTES
	//   - The length of the vector is the sine of 1/2 the angle of rotation.
	//   - This makes the direction (0,0,0) if the quaternion has zero rotation angle.
	// SEE ALSO: GetAngle, GetUnitDirection
	void GetDirection(Vector3 & outDirection) const;

	// PURPOSE: Set a vector to the quaternion's unit axis of rotation.
	// PARAMS
	//   outDirection - the direction of the quaternion's axis of rotation (a unit vector)
	// NOTES
	//   - This makes the direction (0,1,0) if the quaternion has zero rotation angle.
	// SEE ALSO: GetAngle, GetDirection
	void GetUnitDirection(Vector3 & outDirection) const;


	//=========================================================================
	// Modifiers

	// PURPOSE: Set this quaternion's value.
	// PARAMS
	//   sw, sy, sz, sw - The components of the quaternion's new value.
	void Set(float sx, float sy, float sz, float sw);

	// PURPOSE: Set this quaternion's value.
	// PARAMS
	//   q - The source quaternion containing the quaternion's new value.
	void Set(const Quaternion& q);

	// PURPOSE: Set this quaternion to the identity quaternion.
	// NOTES
	//   Like an identity matrix, an identity quaternion represents no rotation.
	void Identity();


	//=========================================================================
	// Conversion

	// PURPOSE: Create a quaternion from a rotation vector.
	// PARAMS
	//   rotation - The rotation vector.
	// NOTES
	//   A rotation vector is apparently a vector where the direction indicates 
	//   the axis of rotation and the magnitude indicates the angle, in radians, to rotate.
	// SEE ALSO: FromMatrix34,FromEulers
	void FromRotation(const Vector3 & rotation);

	// PURPOSE: Create a quaternion from a unit axis of rotation & an angle of rotation.
	// PARAMS
	//   unit - The unit axis of rotation.
	//   angle - The angle of rotation.
	// SEE ALSO: FromMatrix34,FromEulers
	void FromRotation(const Vector3 & unit, float angle);

	// PURPOSE: Create a quaternion from a set of XYZ ordered euler angles.
	// PARAMS
	//   euler - XYZ ordered euler angles in radians.
	// SEE ALSO: ToEulers,FromMatrix34,FromRotation
	void FromEulers(const Vector3& euler);

	// PURPOSE: Create a quaternion from a set of XYZ ordered euler angles.
	// PARAMS
	//   euler - ordered euler angles in radians.
	//	 order - the order of the euler angles. Must be "xyz", "xzy", "yxz",
	//           "yzx", "zxy", or "zyx".
	// SEE ALSO: ToEulers,FromMatrix34,FromRotation
	void FromEulers(const Vector3& euler, const char* order);

	// PURPOSE: Create a quaternion from a set of XYZ ordered euler angles, faster and reduced code size
	void FromEulers(const Vector3& euler, eEulerOrders order);	

	// PURPOSE: Create a quaternion from a Matrix34.
	// PARAMS
	//   m - The matrix containing the quaternion's new rotation.
	// NOTES
	//   - This method assumes that the upper 3x3 part of the Matrix34 represents a rotation.
	//   - To get a matrix from a quaternion, use Matrix34::FromQuaternion().
	// SEE ALSO: FromRotation,FromEulers,Matrix34::FromQuaternion
	void FromMatrix34(const Matrix34 & m);

	// PURPOSE: Create a quaternion that represents the rotation between two vectors.
	// PARAMS
	//   from - The starting vector
	//   to - The ending vector
	// RETURN True if a unique rotation axis was found.  False if the
	//        vectors were co-linear in opposite directions and an
	//        arbitrary perpendicular axis was used.
	// NOTES
	//   - This method creates a rotation about an axis perpendicular to the
	//     starting and ending vectors.
	//   - This method returns the identity quaternion if the vectors are co-linear
	//     in the same direction and a 180 degree rotation around an arbitrary
	//     perpendicular axis when the vectors are co-linear in opposite directions.
	// SEE ALSO: FromRotation,FromEulers
	bool FromVectors(const Vector3& from, const Vector3& to);

	// PURPOSE: Create a quaternion that represents the rotation between two vectors
	//          about the given axis.
	// PARAMS
	//   from - The starting vector
	//   to - The ending vector
	//   axis - The axis of rotation
	// SEE ALSO: FromRotation,FromEulers,FromVectors
	void FromVectors(const Vector3& from, const Vector3& to, const Vector3& axis);

	// PURPOSE: Create a set of XYZ ordered euler angles from a quaternion.
	// PARAMS
	//   outEulers - will return XYZ ordered euler angles in radians.
	// SEE ALSO: FromEulers,FromMatrix34,FromRotation
	void ToEulers(Vector3& outEulers) const;

	// PURPOSE: Create a set of ordered euler angles from a quaternion.
	// PARAMS
	//   outEulers - will return ordered euler angles in radians.
	//	 order - the order of the euler angles. Must be "xyz", "xzy", "yxz",
	//           "yzx", "zxy", or "zyx".
	// SEE ALSO: FromEulers,FromMatrix34,FromRotation
	void ToEulers(Vector3& outEulers, const char* order) const;

	// PURPOSE: Create a set of ordered euler angles from a quaternion, faster and reduced code size
	void ToEulers(Vector3& outEulers, eEulerOrders order);	

	// PURPOSE: Create the unit axis and angle of rotation from this quaternion
	// PARAMS
	//   outAxis - unit axis of rotation
	//   outAngle - angle of rotation
	// NOTES
	//	 Will return an outAxis of (0, 0, 0) if this Quaternion has an axis of (0, 0, 0)
	// SEE ALSO: FromRotation
	void ToRotation(Vector3& outAxis, float& outAngle) const;


	//=========================================================================
	// Algebra and operations

	// PURPOSE: Negate this quaternion.
	// NOTES
	//   Negating a quaternion means that the rotation happens in the opposite 
	//   direction on the opposite axis -- in other words, no net change in the rotation.
	// SEE ALSO: PrepareSlerp
	void Negate();

	// PURPOSE: Set this quaternion to the negation of another quaternion.
	// PARAMS
	//   q - The quaternion to negate.
	// NOTES
	//   Negating a quaternion means that the rotation happens in the opposite 
	//   direction on the opposite axis -- in other words, no net change in the rotation.
	// SEE ALSO: PrepareSlerp
	void Negate(const Quaternion & q);

	// PURPOSE: Invert this quaternion.
	// NOTES
	//   An inverted quaternion accomplishes the opposite rotation of the 
	//   original, i.e. it's like an inverted matrix.
	void Inverse();

	// PURPOSE: Set this quaternion to the inverse of another quaternion.
	// PARAMS
	//   q - The quaternion to invert.
	// NOTES
	//   An inverted quaternion accomplishes the opposite rotation of the 
	//   original, i.e. it's like an inverted matrix.
	void Inverse(const Quaternion & q);

	// PURPOSE: Scale the component floats of this quaternion.
	// NOTES
	//   A unit quaternion (i.e. with length 1) is the only type of quaternion 
	//   that represents a rotation.
	void Scale(float f);

	// PURPOSE: Scale the rotation represented by this quaternion
	// NOTES
	//   This differs from Scale in that it scale's the rotation represented by the
	//   quaternion (i.e. q.ScaleAngle(3) == q*q*q , q.ScaleAngle(-1) == q.Inverse() etc)
	void ScaleAngle(float f);

	// PURPOSE: Set this quaternion to the result of scaling another quaternion.
	// PARAMS
	//   q - The quaternion to scale.
	//   f - The scaling factor.
	// NOTES
	//   A unit quaternion (i.e. with length 1) is the only type of quaternion 
	//   that represents a rotation.
	void Scale(const Quaternion & q, float f);

	// PURPOSE: Get the magnitude of this quaternion.
	// RETURN: the square root of the sum of the squares of this quaternion's elements
	// SEE ALSO: Mag2, InvMag
	float Mag() const;

	// PURPOSE: Get the squared magnitude of this quaternion.
	// RETURN: the sum of the squares of this quaternion's elements
	// SEE ALSO: Mag, InvMag
	float Mag2() const;

	// PURPOSE: Get the inverse of the magnitude of this quaternion.
	// RETURN: one over the square root of the sum of the squares of this quaternion's elements
	// SEE ALSO: Mag, Mag2
	float InvMag() const;

	// PURPOSE: Normalize this quaternion (i.e scale it so that its magnitude is 1).
	// NOTES
	//   A unit quaternion (i.e. with length 1) is the only type of quaternion 
	//   that represents a rotation.
	void Normalize();

	// PURPOSE: Set this quaternion to the normalization of another quaternion.
	// PARAMS
	//   q - The quaternion to normalize.
	// NOTES
	//   A unit quaternion (i.e. with length 1) is the only type of quaternion 
	//   that represents a rotation.
	void Normalize(const Quaternion & q);

	// PURPOSE: Rotate a vector by a quaternion.
	// PARAMS
	//   inout - The vector that gets rotated.
	// NOTES:
	//	1.	If the quaternion represents a rotation, this transforms the given vector from the quaternion's coordinates to world coordinates.
	// SEE ALSO: UnTransform
	void Transform(Vector3 & inout) const;

	// PURPOSE: Rotate a vector by a quaternion.
	// PARAMS
	//   in - The vector to rotate.
	//   out - An output parameter; it receives the rotated vector.
	// NOTES
	//	1.	This method doesn't allow the in vector to be the same as the out vector, and will assert if it is.
	//	2.	If the quaternion represents a rotation, this transforms the given vector from the quaternion's coordinates to world coordinates.
	// SEE ALSO: UnTransform
	void Transform(const Vector3 & in, Vector3 & out) const;

	// PURPOSE: Rotate a vector by the inverse of a quaternion.
	// PARAMS
	//   inout - The vector that gets rotated.
	// NOTES:
	//	1.	If the quaternion represents a rotation, this transforms the given vector from world coordinates to the quaternion's coordinates.
	// SEE ALSO: Transform
	void UnTransform(Vector3 & inout) const;

	// PURPOSE: Rotate a vector by the inverse of a quaternion.
	// PARAMS
	//   in - The vector to rotate.
	//   out - An output parameter; it receives the rotated vector.
	// NOTES
	//	1.	This method doesn't allow the in vector to be the same as the out vector, and will assert if it is.
	//	2.	If the quaternion represents a rotation, this transforms the given vector from world coordinates to the quaternion's coordinates.
	// SEE ALSO: Transform
	void UnTransform(const Vector3 & in, Vector3 & out) const;

	// PURPOSE: Prepare two quaternions for a slerp between them by making sure the angle between them is between -PI and PI.
	// NOTES
	//	This method makes sure that the interpolation takes the shortest route. If the angle between the two quaternions is not between
	//	-PI and PI, then this quaternion is negated to make the angle between -PI and PI. The negated quaternion represents a rotation
	//	in the opposite direction about an opposite unit vector, so it is equivalent to the non-negated quaternion.
	// SEE ALSO: Slerp, SlerpNear
	void PrepareSlerp(const Quaternion &q);

	// PURPOSE: Perform spherical linear interpolation between this quaternion and another.
	// PARAMS
	//   q - The other quaternion, representing a t of 1
	// NOTES:
	//	To make sure the interpolation takes the shorter of two possible paths around a spherical surface,
	//	call PrepareSlerp(q) first, or use SlerpNear.
	// SEE ALSO: PrepareSlerp, SlerpNear
	void Slerp(float t, const Quaternion &q);

	// PURPOSE: Perform spherical linear interpolation between two quaternions.
	// PARAMS
	//   q1 - The beginning of the interpolation range, representing a t of 0.
	//   q2 - The end of the interpolation range, representing a t of 1.
	//   t - The desired interpolation -- 0 represents q1, 1 represents q2, and a number 
	//       between 0 and 1 represents an interpolated rotation between q1 and q2.
	// NOTES:
	//	To make sure the interpolation takes the shorter of two possible paths around a spherical surface,
	//	call q1.PrepareSlerp(q2) or q2.PrepareSlerp(q1) first, or use SlerpNear.
	// SEE ALSO: PrepareSlerp, SlerpNear
	void Slerp(float t, const Quaternion &q1, const Quaternion &q2);

	// PURPOSE: Perform spherical linear interpolation between this quaternion and another.
	// PARAMS
	//   q - The other quaternion, representing a t of 1
	// NOTES
	//	 internally calls PrepareSlerp before performing slerp, so interpolation takes shortest route.
	// SEE ALSO: PrepareSlerp, Slerp
	void SlerpNear(float t, const Quaternion &q);

	// PURPOSE: Perform spherical linear interpolation between two quaternions.
	// PARAMS
	//   q1 - The beginning of the interpolation range, representing a t of 0.
	//   q2 - The end of the interpolation range, representing a t of 1.
	//   t - The desired interpolation -- 0 represents q1, 1 represents q2, and a number 
	//       between 0 and 1 represents an interpolated rotation between q1 and q2.
	// NOTES
	//	 internally calls PrepareSlerp before performing slerp, so interpolation takes shortest route.
	// SEE ALSO: PrepareSlerp, Slerp
	void SlerpNear(float t, const Quaternion &q1, const Quaternion &q2);

	// PURPOSE: Perform regular linear interpolation between this quaternion and another.
	// PARAMS
	//   q - The other quaternion, representing a t of 1
	// NOTES:
	// This can be used as a cheap approximation to slerp, but you must first call PrepareSlerp to 
	// make sure the interpolation takes the shorter path, and you must Normalize the result.
	// SEE ALSO: Slerp
	void Lerp(float t, const Quaternion &q);

	// PURPOSE: Perform regular linear interpolation between two quaternions.
	// PARAMS
	//   q1 - The beginning of the interpolation range, representing a t of 0.
	//   q2 - The end of the interpolation range, representing a t of 1.
	//   t - The desired interpolation -- 0 represents q1, 1 represents q2, and a number 
	//       between 0 and 1 represents an interpolated rotation between q1 and q2.
	// NOTES:
	// This can be used as a cheap approximation to slerp, but you must first call PrepareSlerp to 
	// make sure the interpolation takes the shorter path, and you must Normalize the result.
	// SEE ALSO: Slerp
	void Lerp(float t, const Quaternion &q1, const Quaternion &q2);

	// PURPOSE: Calculate the inner product (i.e. dot product) of two quaternions.
	// PARAMS
	//   q - The other quaternion in the dot product.
	// RETURN: The sum of the elements of this quaternion multiplied in pairs with the given quaternion
	// NOTES:
	//   - The meaning of the dot product of two quaternions is very similar 
	//     to the meaning of the dot product of two vectors; it represents the 
	//     cosine of 1/2 the angle between the two rotations.
	//   - Apparently, how to multiply two quaternions together, and the meaning 
	//     of such an operation, were hotly debated topics back in the nineteenth century.
	// SEE ALSO: RelCosHalfAngle,PrepareSlerp
	float Dot(const Quaternion & q) const;

	// PURPOSE: Multiply this quaternion by another quaternion, producing a combined rotation.
	// PARAMS
	//   q - The quaternion to multiply with this quaternion.
	// NOTES:
	//	1.	If both quaternions represent rotations, this transforms the given quaternion from this quaternion's coordinates into world coordinates.
	// SEE ALSO: MultiplyInverse,MultiplyFromLeft
	void Multiply(const Quaternion & q);

	// PURPOSE: Multiply two quaternions together, producing a combined rotation.
	// PARAMS
	//   q1 - The multiplicand (i.e. the quaternion that gets multiplied).
	//   q2 - The multiplier (i.e. the quaternion that does the multiplying).
	// NOTES
	//	1.	This method doesn't allow either of the parameters to be this quaternion, and will assert if either is.
	//	2.	If both quaternions represent rotations, this transforms the second quaternion from the first quaternion's coordinates into world coordinates.
	// SEE ALSO: MultiplyInverse,MultiplyFromLeft
	void Multiply(const Quaternion & q1, const Quaternion & q2);

	// PURPOSE: Multiply this quaternion by the inverse of another quaternion, producing a combined rotation.
	// PARAMS
	//   q - The inverse of the quaternion to multiply with this quaternion.
	// SEE ALSO: Multiply,MultiplyFromLeft,MultiplyInverseFromLeft
	void MultiplyInverse(const Quaternion & q);

	// PURPOSE: Multiply this quaternion by the inverse of another quaternion, producing a combined rotation.
	// PARAMS
	//   q - The inverse of the quaternion to multiply with this quaternion.
	// SEE ALSO: Multiply,MultiplyFromLeft,MultiplyInverse
	void MultiplyInverseFromLeft(const Quaternion & q);

	// PURPOSE: Multiply a quaternion by the inverse of another quaternion, producing a combined rotation.
	// PARAMS
	//   q1 - The multiplicand (i.e. the quaternion that gets multiplied).
	//   q2 - The inverse of the multiplier (i.e. the inverse of the quaternion that does the multiplying).
	// NOTES
	//   This method doesn't allow either of the parameters to be this quaternion, and
	//   will assert if either is.
	// SEE ALSO:	Multiply,MultiplyFromLeft
	void MultiplyInverse(const Quaternion & q1, const Quaternion & q2);

	// PURPOSE: Multiply another quaternion by the current quaternion, producing a combined rotation.
	// PARAMS
	//   q - The quaternion to multiply with the current quaternion.
	// NOTES
	//   If one considers that Multiply() means "this = this * q", MultiplyFromLeft() means
	//   "this = q * this".
	// SEE ALSO: Multiply,MultiplyInverse
	void MultiplyFromLeft(const Quaternion & q);

	// PURPOSE: Calculate the relative angle between two quaternions.
	// PARAMS
	//   q - The other quaternion.
	// RETURN: the angle between this quaternion and the given quaternion
	// SEE ALSO: GetAngle, RelCosHalfAngle, GetCosHalfAngle, Dot
	float RelAngle(const Quaternion & q) const;

	// PURPOSE: Calculate the cosine of half the relative angle between two quaternions.
	// PARAMS
	//   q - The other quaternion.
	// RETURN: the cosine of half the angle between this quaternion and the given quaternion
	// SEE ALSO: Dot, RelAngle, GetAngle, GetCosHalfAngle
	float RelCosHalfAngle(const Quaternion & q) const;

	// PURPOSE: Calculate the amount of twist around an arbitrary vector.
	// PARAMS
	//   v - The unit vector to calculate the twist around
	// RETURN: angle of twist around the vector
	// SEE ALSO: TwistSwingDecomp
	float TwistAngle(const Vector3 & v) const;

	// PURPOSE: Decomposes this rotation into two rotations, one about the twist axis,
	//          followed by a shortest arc rotation to the desired final rotation
	// PARAMS
	//   twistAxis - The unit vector about which the twist rotation should be about
	//   twist - The output rotation about the twist axis
	//   swing - The output shortest arc from the twist orientation to the final orientation
	// NOTE: The inverse of this would be combined.Multiply(swing, twist)
	// SEE ALSO: TwistAngle
	void TwistSwingDecomp(const Vector3 & twistAxis, Quaternion& twist, Quaternion& swing) const;

	// PURPOSE: Tell if this quaternion equals the given quaternion.
	// PARAMS:
	//	q -	the quaternion to test for equality with this quaternion
	// RETURN: true if this quaternion's elements all equal the corresponding elements in the given quaternion, false if not
	bool IsEqual (const Quaternion& q) const;

	//=========================================================================
	// Output

	// PURPOSE: Print a quaternion.
	void Print() const;

	// PURPOSE: Print a quaternion with a preceding label.
	// PARAMS
	//   label - A label to print before the quaternion.
	void Print(const char *label) const;


	//=========================================================================
	// Data

#if __XENON || __PS3 || __PSP2

#if __XENON
#pragma warning (disable : 4201)			// nonstandard extension used : nameless struct/union
#endif

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
	};
#if __XENON
#pragma warning (default : 4201)			// nonstandard extension used : nameless struct/union
#endif 

#elif RSG_CPU_INTEL
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
	};
#if defined(_MSC_VER)
#pragma warning (default : 4201)			// nonstandard extension used : nameless struct/union
#endif
#else
	float x;
	float y;
	float z;
	float w;
#endif

	// PURPOSE: A quaternion representing no rotation, i.e. X * I == X
	static Quaternion sm_I;

#if __DECLARESTRUCT
	void DeclareStruct(class datTypeStruct &s);
#endif
};


// PURPOSE: Serialize a quaternion object
inline datSerialize & operator<< (datSerialize &s, Quaternion &q) {
	s << q.x << q.y << q.z << q.w;
	return s;
}
}	// namespace rage

//=============================================================================
// Implementation

// First, include platform specific implementation

#if RSG_CPU_INTEL && VECTORIZED_QUAT
#include "vector/quaternion_win32.h"
#elif __XENON && VECTORIZED_QUAT
#include "vector/quaternion_xenon.h"
#endif

// Second, include standard version for any that weren't implemented in 
// the platform specific section
#include "vector/quaternion_default.h"

//=============================================================================

#endif // VECTOR_QUATERNION_H
