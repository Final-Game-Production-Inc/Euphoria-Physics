//
// vector/matrix34.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_MATRIX34_H
#define VECTOR_MATRIX34_H

#include "data/resource.h"
#include "data/serialize.h"
#include "vector/vector3.h"
#include "vector/vector4.h"

namespace rage {

class Quaternion;
class Matrix33;

#define DEFAULT_NEAR_ONE	0.99f

//=============================================================================
// Matrix34
//
// PURPOSE:
//   Matrix34 is a matrix with 3 columns and 4 rows. Although it can do 
//   general 3x4-matrix math, it is used mostly as a position (the 4th row) 
//   and an orientation (the first 3 rows).
// NOTES:
//   - Matrix34 operates on vectors from their right side.
// <FLAG Component>
//

class VECTOR_ALIGN Matrix34
{
public:
	// PURPOSE: Default constructor.
	// NOTES: As an optimization, this does not initialized the members (nor do the contained vector constructors).
	Matrix34() {}

	Matrix34( const Matrix34& m34 );
	Matrix34( Vector4::Vector4In a, Vector4::Vector4In b, Vector4::Vector4In c, Vector4::Vector4In d );

	enum _IdentityType		{ IdentityType };
	enum _Identity3x3Type	{ Identity3x3Type };
	enum _ZeroType			{ ZeroType };
	enum _Zero3x3Type		{ Zero3x3Type };

	// PURPOSE: Constructor to perform a particular initialization operator
	// USAGE
	//	Matrix34 mat(Matrix34::IdentityType);		// Create an indentity matrix
	//	Matrix34 mat(Matrix34::Identity3x3Type);	// Create a 3x3 indentity matrix
	//	Matrix34 mat(Matrix34::ZeroType);			// Create a zeroed matrix
	//	Matrix34 mat(Matrix34::Zero3x3Type);		// Create a 3x3 zeroed matrix
	Matrix34(_IdentityType) { Identity(); }
	Matrix34(_Identity3x3Type) { Identity3x3(); }
	Matrix34(_ZeroType) { Zero(); }
	Matrix34(_Zero3x3Type) { Zero3x3(); }

	// PURPOSE: Constructor.
	Matrix34( float ax, float ay, float az, float bx, float by, float bz,
			  float cx, float cy, float cz, float dx, float dy, float dz )
			  : a(ax,ay,az), b(bx,by,bz), c(cx,cy,cz), d(dx,dy,dz) 	{	}

	// PURPOSE: For the new vec lib scalar fallback option, SCALAR_TYPES_ONLY == 1 in vectorconfig.h.
	Matrix34(const Vec::Vector_4& vec0, const Vec::Vector_4& vec1, const Vec::Vector_4& vec2, const Vec::Vector_4& vec3)
		:	a(vec0.x, vec0.y, vec0.z),
			b(vec1.x, vec1.y, vec1.z),
			c(vec2.x, vec2.y, vec2.z),
			d(vec3.x, vec3.y, vec3.z)
	{
	}

	// PURPOSE: Resource constructor.
	// NOTES:
	//  - Does not modify data.
	//  - This is here so that we can bypass the default constructor when using placement new.
	Matrix34(class datResource&) {}

	DECLARE_DUMMY_PLACE(Matrix34);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	//==========================================================
	// Reset functions

	// PURPOSE: Set the 3x3 part of the current matrix to the identity matrix.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Identity
	void Identity3x3();

	// PURPOSE: Set the current matrix to the identity matrix.
	// SEE ALSO
	//   Identity3x3
	void Identity();

	// PURPOSE: Zero the 3x3 part of the current matrix.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Zero
	void Zero3x3();

	// PURPOSE: Zero the current matrix (i.e. set all of its elements to zero).
	// SEE ALSO
	//   Zero3x3
	void Zero();

	// PURPOSE: Set the 3x3 part of the current matrix to the 3x3 part of another matrix.
	// PARAMS
	//   m - The matrix containing the 3x3 part to be copied.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Set
	void Set3x3(const Matrix34 &m);

	// PURPOSE: Set the value of the current matrix to the value of another matrix.
	// PARAMS
	//   m - The matrix containing the new value for the current matrix.
	// SEE ALSO
	//   Set3x3
	void Set(const Matrix34 &m);

	// PURPOSE: Set the elements of the current matrix to the given elements.
	// PARAMS
	//   newA - new value for the a-vector of the current matrix
	//   newB - new value for the b-vector of the current matrix
	//   newC - new value for the c-vector of the current matrix
	//   newD - optional new value for the d-vector of the current matrix; default is (0.0f,0.0f,0.0f)
	// SEE ALSO
	//   Set3x3
	void Set (const Vector3& newA, const Vector3& newB, const Vector3& newC, const Vector3& newD=ORIGIN);

	// PURPOSE: Set the elements of the current matrix to the given elements.
	// PARAMS
	//   ax, ay, az - The new values for the a-vector of the current matrix.
	//   bx, by, bz - The new values for the b-vector of the current matrix.
	//   cx, cy, cz - The new values for the c-vector of the current matrix.
	//   dx, dy, dz - The new values for the d-vector of the current matrix (default 0.0f).
	// SEE ALSO
	//   Set3x3
	void Set(float ax, float ay, float az, float bx, float by, float bz,
			  float cx, float cy, float cz, float dx=0.0f, float dy=0.0f, float dz=0.0f);


	void SetDiagonal( Vector3::Vector3Param );

	//==========================================================
	// Algebra

	// PURPOSE: Add a matrix to the current matrix.
	// PARAMS
	//   m - The matrix to add to the current matrix.
	// NOTES
	//   This just adds the respective components together.
	// SEE ALSO
	//   Add3x3,Subtract,Subtract3x3,AddScaled3x3
	void Add(const Matrix34 &m);

	// PURPOSE: Set the current matrix to the sum of two other matrices.
	// PARAMS
	//   m,n - The two matrices to add.
	// NOTES
	//   This just adds the respective components together.
	// SEE ALSO
	//   Add3x3,Subtract,Subtract3x3,AddScaled3x3
	void Add(const Matrix34 &m,const Matrix34 &n);

	// PURPOSE: Add the 3x3 part of a matrix to the 3x3 part of the current matrix.
	// PARAMS
	//   m - The matrix to add to the current matrix.
	// NOTES
	//   - This just adds the respective components together.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Add,Subtract,Subtract3x3,AddScaled3x3
	void Add3x3(const Matrix34 &m);

	// PURPOSE: Set the 3x3 part of the current matrix to the sum of the 3x3 parts of two other matrices.
	// PARAMS
	//   m,n - The two matrices to add.
	// NOTES
	//   - This just adds the respective components together.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Add,Subtract,Subtract3x3,AddScaled3x3
	void Add3x3(const Matrix34 &m,const Matrix34 &n);

	// PURPOSE: Subtract a matrix from the current matrix.
	// PARAMS
	//   m - The matrix to subtract from the current matrix.
	// NOTES
	//   - This just subtracts the respective components.
	// SEE ALSO
	//   Add,Add3x3,Subtract3x3,AddScaled3x3
	void Subtract(const Matrix34 &m);

	// PURPOSE: Set the current matrix to the difference between two other matrices.
	// PARAMS
	//   m - The minuend (i.e. the matrix that gets subtracted from).
	//   n - The subtrahend (i.e. the matrix that gets subtracted).
	// NOTES
	//   - This just subtracts the respective components.
	// SEE ALSO
	//   Add,Add3x3,Subtract3x3,AddScaled3x3
	void Subtract(const Matrix34 &m,const Matrix34 &n);

	// PURPOSE: Subtract the 3x3 part of a matrix from the 3x3 part of the current matrix.
	// PARAMS
	//   m - The matrix to subtract from the current matrix.
	// NOTES
	//   - This just subtracts the respective components.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Add,Add3x3,Subtract,AddScaled3x3
	void Subtract3x3(const Matrix34 &m);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the difference between the 3x3 parts of
	//   two other matrices.
	// PARAMS
	//   m - The minuend (i.e. the matrix that gets subtracted from).
	//   n - The subtrahend (i.e. the matrix that gets subtracted).
	// NOTES
	//   - This just subtracts the respective components.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Add,Add3x3,Subtract,AddScaled3x3
	void Subtract3x3(const Matrix34 &m,const Matrix34 &n);

	// PURPOSE
	//   Scale the 3x3 part of another matrix by a given value, and add it to the 3x3 part
	//   of the current matrix.
	// PARAMS
	//   m - The matrix that gets scaled & then added to the current matrix.
	//   f - The value by which to scale the matrix m.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Add,Add3x3,Subtract,Subtract3x3
	void AddScaled3x3(const Matrix34 &m, float f);

	// PURPOSE: Set the components of a matrix to their absolute value.
	void Abs();

	// PURPOSE: Sets the elements of the 3x3 part of the current matrix to their negation
	void Negate3x3();

	// PURPOSE: Sets the elements of the current matrix to their negation
	void Negate();


	//==========================================================
	// Dot products

	// PURPOSE: Transform the current matrix by another matrix.
	// PARAMS:
	//   m - The matrix that transforms the current matrix.
	// NOTES:
	//	1.	If both matrices represent coordinate systems, this transfoms this matrix from the given matrix's coordinates into world coordinates.
	// SEE ALSO:
	//   Dot3x3,DotFromLeft,DotTranspose,Dot3x3Transpose,DotNoVu0
	void Dot(const Matrix34 &m);

	// PURPOSE: Transform the current matrix by another matrix (from the left).
	// PARAMS
	//   m - The matrix that transforms the current matrix (from the left).
	// NOTES:
	//	1.	If both matrices represent coordinate systems, this transfomsthe given matrix from this matrix's coordinates into world coordinates.
	// SEE ALSO
	//   Dot3x3,Dot,DotTranspose,Dot3x3Transpose,DotNoVu0
	void DotFromLeft(const Matrix34 &m);

	// PURPOSE: Set the current matrix to the transformation of one matrix by another matrix.
	// PARAMS
	//   m - The matrix being transformed.
	//   n - The matrix doing the transforming.
	// NOTES
	//	1.	Neither m nor n can be this
	//	2.	If both matrices represent coordinate systems, this transfoms the first matrix from the second matrix's coordinates into world coordinates.
	// SEE ALSO
	//   Dot3x3,DotTranspose,Dot3x3Transpose,DotNoVu0
	void Dot(const Matrix34 &m,const Matrix34 &n);

	// PURPOSE: Transform the current matrix by the inverse of another matrix.
	// PARAMS
	//   m - The matrix whose inverse transforms the current matrix.
	// NOTES
	//	1.	If both matrices represent coordinate systems, this transfoms this matrix from world coordinates into the given matrix's coordinates.
	// SEE ALSO
	//   Dot,Dot3x3,Dot3x3Transpose,DotNoVu0
	void DotTranspose(const Matrix34 &m);

	// PURPOSE
	//   Set the current matrix to the transformation of one matrix by the inverse of
	//   another matrix.
	// PARAMS
	//   m - The matrix being transformed.
	//   n - The matrix whose inverse is doing the transforming.
	// NOTES
	//	1.	If both matrices represent coordinate systems, this transfoms the first matrix from world coordinates into the second matrix's coordinates.
	// SEE ALSO
	//   Dot,Dot3x3,Dot3x3Transpose,DotNoVu0
	void DotTranspose(const Matrix34 &m, const Matrix34 &n);

	// PURPOSE: Transform the 3x3 part of the current matrix by the 3x3 part of another matrix.
	// PARAMS
	//   m - The matrix that transforms the current matrix.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Dot,Dot3x3FromLeft,DotTranspose,Dot3x3Transpose,DotNoVu0
	void Dot3x3(const Matrix34 &m);

	// PURPOSE: Transform the 3x3 part of the current matrix by the 3x3 part of another matrix (from the left).
	// PARAMS
	//   m - The matrix that transforms the current matrix (from the left).
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Dot,Dot3x3,DotTranspose,Dot3x3Transpose,DotNoVu0
	void Dot3x3FromLeft(const Matrix34 &m);

	// PURPOSE: Transform the 3x3 part of the current matrix by the 3x3 part of another matrix.
	// PARAMS
	//   m - The matrix being transformed.
	//   n - The matrix doing the transforming.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Dot,DotTranspose,Dot3x3Transpose,DotNoVu0
	void Dot3x3(const Matrix34 &m,const Matrix34 &n);

	// PURPOSE
	//   Transform the 3x3 part of the current matrix by the 3x3 part of the inverse
	//   of another matrix.
	// PARAMS
	//   m - The matrix whose inverse transforms the current matrix.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Dot,DotTranspose,Dot3x3,DotNoVu0
	void Dot3x3Transpose(const Matrix34 &m);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the transformation of the 3x3 part of one
	//   matrix by the 3x3 part of the inverse of another matrix.
	// PARAMS
	//   m - The matrix being transformed.
	//   n - The matrix whose inverse is doing the transforming.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
    //   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Dot,DotTranspose,Dot3x3,DotNoVu0
	void Dot3x3Transpose(const Matrix34 &m,const Matrix34 &n);


	//==========================================================
	// Comparison functions

	// PURPOSE
	//   Determines whether the current matrix is equal to another.
	// PARAMS
	//   m - The matrix being compared to.
	// NOTES
	// SEE ALSO
	//   Vector3::IsEqual
	bool IsEqual(const Matrix34& m) const;

	// PURPOSE
	//   Determines whether the current matrix is not equal to another.
	// PARAMS
	//   m - The matrix being compared to.
	// NOTES
	// SEE ALSO
	//   Vector3::IsNotEqual
	bool IsNotEqual(const Matrix34& m) const;

	// PURPOSE: Determine whether this matrix is equal to the given matrix, within the given tolerance.
	// PARAMS
	//   m - the matrix to test for closeness with this matrix
	//   error - the tolerance
	// RETURNS: true if the matrix is equal to the given matrix within the given tolerance, false if it is not
	bool IsClose(const Matrix34& m, float error=0.01f) const;

	// PURPOSE: See if the matrix is orthonormal within the given error range.
	// PARAMS
	//   error - the maximum fractional error allowed for a matrix to be considered orthonormal
	// NOTES
	//   Orthonormal means orthogonal and normal. Orthogonal means the axes (the first three vectors) are
	//   all perpendicular to each other, and normal means they all have length one. Coordinate matrices
	//   are always orthonormal.
	bool IsOrthonormal (float error=0.01f) const;

	// PURPOSE: Return a number that measure the non-orthonormality of a matrix
	// PARAMS
	//   errorThreshold - the maximum fractional error allowed for a matrix to be considered orthonormal
	// NOTES
	//   The value returned is the maximum component magnitude of (M * M^T) - I normalised so that
	//		0 means no error and 1 is the error threshold for IsOthonormal to return false
	float MeasureNonOrthonormality(float errorThreshold=0.01f) const;

	// PURPOSE
	//   Sets the current matrix equal to a matrix that will compute cross products with another
	//   vector using the Transform3x3 function.
	// PARAMS
	//   r - the vector used to generate the matrix
	void CrossProduct(Vector3::Param r);

	// PURPOSE
	//   Sets the current matrix to have the second input vector in each of its local coordinate
	//   axes, with each axis scaled by the corresponding element of the first input vector.
	// PARAMS
	//   u - the vector used to scale the matrice's coordinate axes
	//   v - the vector used to set the matrice's coordinate axes
	void OuterProduct(const Vector3& u, const Vector3& v);


	//==========================================================
	// Transform functions

	// PURPOSE: Transform a 3-D vector by the current matrix, producing a 2-D vector.
	// PARAMS
	//   in - The 3-D vector to be transformed.
	//   out - The transformed 2-D vector.
	// NOTES
	//   The 2-D vector represents the X/Z parts of the transformed 3-D vector.
	// SEE ALSO
	//   Transform,Transform3x3,UnTransform,UnTransform3x3
	void TransformXZ(const Vector3 &in,Vector2 &out) const;

	// PURPOSE: Transform a vector by the current matrix.
	// PARAMS
	//   in - The vector to be transformed.
	//   out - The transformed vector.
	// NOTES
	//   This function is not safe to use if you pass in the same vector for in and for out.
	// SEE ALSO
	//   Transform3x3,UnTransform,UnTransform3x3,Transform4
	void Transform(Vector3::Param in,Vector3::Vector3Ref out) const;

	// PURPOSE: Transform a vector by the current matrix.
	// PARAMS
	//   inAndOut - The vector that gets transformed.
	// SEE ALSO
	//   Transform3x3,UnTransform,UnTransform3x3,Transform4
	void Transform(Vector3::Vector3Ref inAndOut) const;

	// PURPOSE: Transform a vector by the 3x3 part of the current matrix.
	// PARAMS
	//	 in - The vector to be transformed.
	//   out - The transformed vector.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   This function is not safe to use if you pass in the same vector for in and for out.
	// SEE ALSO
	//   Transform,UnTransform,UnTransform3x3
	void Transform3x3(Vector3::Param in,Vector3::Vector3Ref out) const;

	// PURPOSE: Transform a vector by the 3x3 part of the current matrix.
	// PARAMS
	//   inAndOut - The vector that gets transformed.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Transform,UnTransform,UnTransform3x3
	void Transform3x3(Vector3::Vector3Ref inAndOut) const;

	// PURPOSE: Transform a 3-D vector by the 3x3 part of the current matrix, producing a 2-D vector.
	// PARAMS
	//   in - The 3-D vector to be transformed.
	//   out - The transformed 2-D vector.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   The 2-D vector represents the X/Y parts of the transformed 3-D vector.
	// SEE ALSO
	//   Transform,Transform3x3,UnTransform,UnTransform3x3
	void Transform3x3(Vector3::Param in,Vector2 &out) const;

	// PURPOSE: Transform a vector by the inverse of the current matrix.
	// PARAMS
	//   in - The vector to be transformed.
	//   out - The transformed vector.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   This function is not safe to use if you pass in the same vector for in and for out.
	// SEE ALSO
	//   Transform,Transform3x3,UnTransform3x3
	void UnTransform(Vector3::Param in,Vector3::Ref out) const;

	// PURPOSE: Transform a vector by the inverse of the current matrix.
	// PARAMS
	//   inAndOut - The vector that gets transformed.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Transform,Transform3x3,UnTransform3x3
	void UnTransform(Vector3::Ref inAndOut) const;

	// PURPOSE: Transform a vector by the inverse of the 3x3 part of the current matrix.
	// PARAMS
	//   in - The vector to be transformed.
	//   out - The transformed vector.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   This function is not safe to use if you pass in the same vector for in and for out.
	// SEE ALSO
	//   Transform,Transform3x3,UnTransform
	void UnTransform3x3(Vector3::Param in,Vector3::Ref out) const;

	// PURPOSE: Transform a vector by the inverse of the 3x3 part of the current matrix.
	// PARAMS
	//   inAndOut - The vector that gets transformed.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	// Transform,Transform3x3,UnTransform
	void UnTransform3x3(Vector3::Ref inAndOut) const;

	// PURPOSE
	//   Transform a 3-D vector by the inverse of the 3x3 part of the current matrix,
	//   producing a 2-D vector.
	// PARAMS
	//   in - The 3-D vector to be transformed.
	//   out - The transformed 2-D vector.
	// NOTES
	//   When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//   upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   The 2-D vector represents the X/Y parts of the transformed 3-D vector.
	// SEE ALSO
    //   Transform,Transform3x3,UnTransform
	void UnTransform3x3(Vector3::Param in,Vector2 &out) const;

	// PURPOSE
	//   Transform an array of vectors by the current matrix, producing a different array
	//   containing transformed versions of the vectors.
	// PARAMS
	//   in - An array of vectors, represented as Vector3 objects, containing the vectors to transform.
	//   out - An output parameter; it points to an array of vectors, represented as Vector4 objects,
	//         containing the transformed vectors.
	//   count - The number of vectors to tranform.  Must be a non-zero multiple of 4.
	// SEE ALSO
	//   Transform
	void Transform4(const Vector3 *in, Vector4 *out, int count) const;

	// PURPOSE: Get the 3x3 transformation of the vertical direction by this matrix.
	// NOTES:
	//	For coordinate matrices, this is the local vertical direction.
	//	The vertical direction is g_UnitUp, which is settable and defaults to YAXIS.
	Vector3 GetLocalUp() const;

	//==========================================================
	// Rotation functions, orientation only.
	// These rotate the existing matrix.  Only the orientation changes (i.e. d is left unchanged).

	// PURPOSE: Rotate the 3x3 part of the current matrix, along the world's +X axis, by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This method only rotates the 3x3 part of the matrix.  If you want to rotate the whole thing,
	//     use RotateFullX() instead.
	//   - This method rotates the matrix by the world's +X axis.  If you want to rotate the matrix by
	//     the matrix's +X axis, use RotateLocalX().
	// SEE ALSO
	//   Rotate,RotateLocalX
	void RotateX(float t);

	// PURPOSE: Rotate the 3x3 part of the current matrix, along the world's +Y axis, by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This method only rotates the 3x3 part of the matrix.  If you want to rotate the whole thing,
	//     use RotateFullY() instead.
	//   - This method rotates the matrix by the world's +Y axis.  If you want to rotate the matrix by
	//     the matrix's +Y axis, use RotateLocalY().
	// SEE ALSO
	//   Rotate,RotateLocalY
	void RotateY(float t);

	// PURPOSE: Rotate the 3x3 part of the current matrix, along the world's +Z axis, by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This method only rotates the 3x3 part of the matrix.  If you want to rotate the whole thing,
	//     use RotateFullZ() instead.
	//   - This method rotates the matrix by the world's +Z axis.  If you want to rotate the matrix by
	//     the matrix's +Z axis, use RotateLocalZ().
	// SEE ALSO
	//   Rotate,RotateLocalZ
	void RotateZ(float t);

	// PURPOSE: Rotate the 3x3 part of the current matrix, along the given axis, by the given angle.
	// PARAMS
	//   va - The axis of rotation.
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method requires the rotation axis to be of unit length.  If you can't guarantee your
	//     rotation axis is of unit length, use Rotate() instead.
	//   - This method only rotates the 3x3 part of the matrix.  If you want to rotate the whole thing,
	//     use RotateFullUnitAxis() instead.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Rotate
	void RotateUnitAxis(const Vector3 &v,float t);

	// PURPOSE: Rotate the 3x3 part of the current matrix, along the given axis, by the given angle.
	// PARAMS
	//   a - The axis of rotation.
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method does not require the rotation axis to be of unit length.  If you know your
	//     rotation axis is of unit length, you can save a little time by calling RotateUnitAxis().
	//   - This method only rotates the 3x3 part of the matrix.  If you want to rotate the whole thing,
	//     use RotateFull() instead.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   RotateX,RotateY,RotateZ,RotateUnitAxis,RotateLocalX,RotateLocalY,RotateLocalZ,RotateFullX,
	//   RotateFullY,RotateFullZ,RotateFullUnitAxis,RotateFull,MakeRotateX,MakeRotateY,MakeRotateZ,
	//   MakeRotateUnitAxis,MakeRotate,RotateTo,MakeRotateTo,RotateTo,MakeUpright
	void Rotate(const Vector3 &v,float t);

	// PURPOSE: Rotate the 3x3 part of the current matrix, along the matrix's +X axis, by the given angle.
	// PARAMS
	//   angle - The angle of rotation, in radians.
	// NOTES
	//   - This is the same as RotateUnitAxis(this->a,angle) but faster.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This method rotates the matrix by the matrix's local +X axis.  If you want to rotate the matrix by
	//     the world's +X axis, use RotateX().
	// SEE ALSO
	//   Rotate,RotateX,RotateLocalAxis
	void RotateLocalX(float angle);

	// PURPOSE: Rotate the 3x3 part of the current matrix, along the matrix's +Y axis, by the given angle.
	// PARAMS
	//   angle - The angle of rotation, in radians.
	// NOTES
	//   - This is the same as RotateUnitAxis(this->b,angle) but faster.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This method rotates the matrix by the matrix's local +Y axis.  If you want to rotate the matrix by
	//     the world's +Y axis, use RotateY().
	// SEE ALSO
	//   Rotate,RotateY,RotateLocalAxis
	void RotateLocalY(float angle);

	// PURPOSE: Rotate the 3x3 part of the current matrix, along the matrix's +Z axis, by the given angle.
	// PARAMS
	// angle - The angle of rotation, in radians.
	// NOTES
	// - This is the same as RotateUnitAxis(this->c,angle) but faster.
	// - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//  upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// - This method rotates the matrix by the matrix's local +Z axis.  If you want to rotate the matrix by
	//  the world's +Z axis, use RotateZ().
	// SEE ALSO
	// Rotate,RotateZ,RotateLocalAxis
	void RotateLocalZ(float angle);

	// PURPOSE: Rotate the 3x3 part of the current matrix, along one of the matrix's axes, by the given angle.
	// PARAMS
	//   angle		- angle of rotation, in radians
	//   axisIndex	- index number of the axis about which to rotate (0==x, 1==y, 2==z)
	// SEE ALSO
	//   RotateLocalX,RotateLocalY,RotateLocalZ
	void RotateLocalAxis(float angle, int axisIndex);


	//==========================================================
	// Rotation functions, orientation and position.
	// These functions rotate the existing matrix and d rotates around the origin.

	// PURPOSE: Rotate the current matrix, along the given axis, by the given angle.
	// PARAMS
	//   va - The axis of rotation.
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method does not require the rotation axis to be of unit length.  If you know your
	//     rotation axis is of unit length, you can save a little time by calling RotateFullUnitAxis().
	//   - This method rotates the entire matrix (i.e. d gets rotated about the origin).  If you only
	//     want to rotate the 3x3 part, use Rotate() instead.
	// SEE ALSO
	//   Rotate
	void RotateFull(const Vector3 &v,float t);

	// PURPOSE: Rotate the current matrix, along the +X axis, by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method rotates the entire matrix (i.e. d gets rotated about the origin).  If you only
	//     want to rotate the 3x3 part, use RotateX() instead.
	// SEE ALSO
	//   Rotate,RotateX
	void RotateFullX(float t);

	// PURPOSE: Rotate the current matrix, along the +Y axis, by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method rotates the entire matrix (i.e. d gets rotated about the origin).  If you only
	//     want to rotate the 3x3 part, use RotateY() instead.
	// SEE ALSO
	//   Rotate,RotateY
	void RotateFullY(float t);

	// PURPOSE: Rotate the current matrix, along the +Z axis, by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method rotates the entire matrix (i.e. d gets rotated about the origin).  If you only
	//     want to rotate the 3x3 part, use RotateZ() instead.
	// SEE ALSO
	//   Rotate,RotateZ
	void RotateFullZ(float t);

	// PURPOSE: Rotate the current matrix, along the given unit-length axis, by the given angle.
	// PARAMS
	//   va - The axis of rotation, which must be of length 1.
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method requires the rotation axis to be of unit length.  If you can't guarantee that
	//     your rotation axis is unit length, use RotateFull() instead.
	//   - This method rotates the entire matrix (i.e. d gets rotated about the origin).  If you only
	//     want to rotate the 3x3 part, use RotateUnitAxis() instead.
	// SEE ALSO
	//   Rotate,RotateUnitAxis
	void RotateFullUnitAxis(const Vector3 &v,float t);


	//==========================================================
	// Functions to make rotation matrices
	// These simply build a rotation matrix. All of these leave the d vector unchanged.

	// PURPOSE
	//   Set the 3x3 part of the current matrix such that, when the matrix is Dot3x3()'d with another
	//   matrix, it rotates the other matrix along the given axis by the given angle.
	// PARAMS
	//   va - The axis of rotation.
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method does not require the rotation axis to be of unit length.  If you know your
	//     rotation axis is of unit length, you can save a little time by calling MakeRotateUnitAxis().
	//   - The matrix produced by this method can only be used to rotate the 3x3 part of another matrix,
	//     i.e. it's not suited for a "full" rotation.  To make it suitable for use in a full rotation,
	//     zero out its d vector (i.e. call "d.Zero()" on the matrix).
	// SEE ALSO
	//   Rotate,MakeRotateUnitAxis
	void MakeRotate(const Vector3 &v,float t);

	// PURPOSE
	//   Set the 3x3 part of the current matrix such that, when the matrix is Dot3x3()'d with another
	//   matrix, it rotates the other matrix along the world's +X axis by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - The matrix produced by this method can only be used to rotate the 3x3 part of another matrix,
	//     i.e. it's not suited for a "full" rotation.  To make it suitable for use in a full rotation,
	//     zero out its d vector (i.e. call "d.Zero()" on the matrix).
	// SEE ALSO
	//   Rotate,MakeRotate
	void MakeRotateX(float t);

	// PURPOSE
	//   Set the 3x3 part of the current matrix such that, when the matrix is Dot3x3()'d with another
	//   matrix, it rotates the other matrix along the world's +Y axis by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - The matrix produced by this method can only be used to rotate the 3x3 part of another matrix,
	//     i.e. it's not suited for a "full" rotation.  To make it suitable for use in a full rotation,
	//     zero out its d vector (i.e. call "d.Zero()" on the matrix).
	// SEE ALSO
	//   Rotate,MakeRotate
	void MakeRotateY(float t);

	// PURPOSE
	//   Set the 3x3 part of the current matrix such that, when the matrix is Dot3x3()'d with another
	//   matrix, it rotates the other matrix along the world's +Z axis by the given angle.
	// PARAMS
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - The matrix produced by this method can only be used to rotate the 3x3 part of another matrix,
	//     i.e. it's not suited for a "full" rotation.  To make it suitable for use in a full rotation,
	//     zero out its d vector (i.e. call "d.Zero()" on the matrix).
	// SEE ALSO
	//   Rotate,MakeRotate
	void MakeRotateZ(float t);

	// PURPOSE
	//   Set the 3x3 part of the current matrix such that, when the matrix is Dot3x3()'d with another
	//   matrix, it rotates the other matrix along the given unit-length axis by the given angle.
	// PARAMS
	//   a - The axis of rotation, which must be of length 1.
	//   t - The angle of rotation, in radians.
	// NOTES
	//   - This method requires the rotation axis to be of unit length.  If you can't guarantee that
	//     your rotation axis is unit length, use MakeRotate() instead.
	//   - The matrix produced by this method can only be used to rotate the 3x3 part of another matrix,
	//     i.e. it's not suited for a "full" rotation.  To make it suitable for use in a full rotation,
	//     zero out its d vector (i.e. call "d.Zero()" on the matrix).
	// SEE ALSO
	//   Rotate,MakeRotate
	void MakeRotateUnitAxis(const Vector3 &v,float t);


	//==========================================================
	// RotateTo
	// Rotates so 'a' lines up with 'b'. They assume a & b are normalized.

	// PURPOSE
	//   Calculate the rotation needed to transform one given vector into another given
	//   vector, and then rotate the 3x3 part of the current matrix by it.
	// PARAMS
	//   va - The initial position of the vector (i.e. before the rotation).
	//   vb - The final position of the vector (i.e. after the rotation).
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Rotate,RotateTo,MakeRotateTo,MakeUpright
	void RotateTo(const Vector3 &a,const Vector3 &b);

	// PURPOSE
	//   Calculate the rotation needed to transform one given vector into another given
	//   vector, scale that rotation, and then rotate the 3x3 part of the current matrix by it.
	// PARAMS
	//   va - The initial position of the vector (i.e. before the rotation).
	//   vb - The final position of the vector (i.e. after the rotation).
	//   t - How much to scale the rotation by.  (A t of 1 makes this method equivalent to RotateTo().)
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Rotate,RotateTo,MakeRotateTo,MakeUpright
	void RotateTo(const Vector3 &a,const Vector3 &b,float t);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the rotation needed to transform one
	//   given vector into another given vector.
	// PARAMS
	//   va - The initial position of the vector (i.e. before the rotation).
	//   vb - The final position of the vector (i.e. after the rotation).
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Rotate,RotateTo,MakeUpright
	void MakeRotateTo(const Vector3 &a,const Vector3 &b);

	// PURPOSE
	//   Rotate the 3x3 part of the current matrix so that its local up direction is up in world coordinates.
	// PARAMS
	//   newLocalZ - the resulting matrix's c-vector (its local Z) will point in this direction projected onto the X-Z plane
	//   nearOne - if b.y is above this tolerance (close to 1) then don't bother with the rotation
	// NOTES
	//   The up direction is g_UnitUp (defined in vector3.h). This rotates this matrix so that its 3x3 transformation of
	//   g_UnitUp equals g_UnitUp in world coordinates.
	void MakeUpright();


	//==========================================================
	// Euler angle related functions

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   matrix, and return them in a vector.
	// NOTES
	//   - Euler angles are returned in XYZ order, so this method is equivalent to GetEulers("xyz").
	// SEE ALSO
	//   FromEulers,ToEulers,GetEulersFast
	Vector3 GetEulers() const;

	// <COMBINE Matrix34::GetEulers>
	Vector3 GetEulersFast() const;

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   matrix, and return them in a vector.
	// PARAMS
	//   order - The order in which to return the Euler angles.  Must be "xyz", "xzy", "yxz",
	//           "yzx", "zxy", or "zyx".
	// NOTES
	//   - This method appears to be nearly identical to ToEulers(), except that ToEulers()
	//     backpatches its result instead of returning it, and ToEulers() calls all the "safe"
	//     variants of the trigonometric functions.
	// SEE ALSO
	//   FromEulers,ToEulers,GetEulersFast
	Vector3 GetEulers(const char * order) const;

	// <COMBINE Matrix34::GetEulers>
	Vector3 GetEulersFast(const char * order) const;
	
	// PURPOSE
	//   Set the 3x3 part of the current matrix to the Euler angles contained in the given
	//   vector and ordered according to the given string.
	// PARAMS
	//   e - The vector that contains the Euler angles.
	//   order - The order that the Euler angles are in.  Must be "xyz", "xzy", "yxz", "yzx",
	//           "zxy", or "zyx".
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,ToEulers
	void FromEulers(const Vector3 &e,const char *order);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the Euler angles contained in the given
	//   vector.
	// PARAMS
	//   e - The vector that contains the Euler angles, in XYZ order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,ToEulers,FromEulersXYZNoVu0
	void FromEulersXYZ(const Vector3 &e);

	// PURPOSE: Set the 3x3 part of the current matrix to the Euler angles contained in the given
	// vector.
	// PARAMS
	// e - The vector that contains the Euler angles, in XZY order.
	// NOTES
	// - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//  upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	// GetEulers,ToEulers,FromEulersXZYNoVu0
	void FromEulersXZY(const Vector3 &e);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the Euler angles contained in the given
	//   vector.
	// PARAMS
	//   e - The vector that contains the Euler angles, in YXZ order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,ToEulers,FromEulersYXZNoVu0
	void FromEulersYXZ(const Vector3 &e);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the Euler angles contained in the given
	//   vector.
	// PARAMS
	//   e - The vector that contains the Euler angles, in YZX order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,ToEulers,FromEulersYZXNoVu0
	void FromEulersYZX(const Vector3 &e);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the Euler angles contained in the given
	//   vector.
	// PARAMS
	//   e - The vector that contains the Euler angles, in ZXY order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,ToEulers,FromEulersZXYNoVu0
	void FromEulersZXY(const Vector3 &e);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the Euler angles contained in the given
	//   vector.
	// PARAMS
	//   e - The vector that contains the Euler angles, in ZYX order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,ToEulers,FromEulersZYXNoVu0
	void FromEulersZYX(const Vector3 &e);

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the rotation described by a unit quaternion.
	// PARAMS
	//   q - The quaternion containing the rotation, which must be normalized.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   ToQuaternion
	void FromQuaternion(const Quaternion& q);

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   current matrix.
	// PARAMS
	//   e - The vector that receives the calculated Euler angles.
	//   order - The order for the generated Euler angles.  Must be "xyz", "xzy", "yxz",
	//           "yzx", "zxy", or "zyx".
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This method appears to be nearly identical to GetEulers(), except that GetEulers()
	//     returns its result instead of backpatching it, and GetEulers() calls all the "unsafe"
	//     variants of the trigonometric functions.
	// SEE ALSO
	//   GetEulers,FromEulers
	void ToEulers(Vector3 &e,const char *order) const;

	// <COMBINE Matrix34::ToEulers>
	void ToEulersFast(Vector3 &e,const char *order) const;

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   current matrix.
	// PARAMS
	//   e - The vector that receives the calculated Euler angles, in XYZ order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,FromEulers,ToEulers
	void ToEulersXYZ(Vector3 &e) const;

	// <COMBINE Matrix34::ToEulersXYZ>
	void ToEulersFastXYZ(Vector3 &e) const;

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   current matrix.
	// PARAMS
	//   e - The vector that receives the calculated Euler angles, in XZY order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,FromEulers,ToEulers
	void ToEulersXZY(Vector3 &e) const;

	// <COMBINE Matrix34::ToEulersXZY>
	void ToEulersFastXZY(Vector3 &e) const;

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   current matrix.
	// PARAMS
	//   e - The vector that receives the calculated Euler angles, in YXZ order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,FromEulers,ToEulers
	void ToEulersYXZ(Vector3 &e) const;

	// <COMBINE Matrix34::ToEulersYXZ>
	void ToEulersFastYXZ(Vector3 &e) const;

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   current matrix.
	// PARAMS
	//   e - The vector that receives the calculated Euler angles, in YZX order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,FromEulers,ToEulers
	void ToEulersYZX(Vector3 &e) const;
	
	// <COMBINE Matrix34::ToEulersYZX>
	void ToEulersFastYZX(Vector3 &e) const;

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   current matrix.
	// PARAMS
	//   e - The vector that receives the calculated Euler angles, in ZXY order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,FromEulers,ToEulers
	void ToEulersZXY(Vector3 &e) const;

	// <COMBINE Matrix34::ToEulersZXY>
	void ToEulersFastZXY(Vector3 &e) const;

	// PURPOSE
	//   Calculate the Euler angles for the rotation described by the 3x3 part of the
	//   current matrix.
	// PARAMS
	//   e - The vector that receives the calculated Euler angles, in ZYX order.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   GetEulers,FromEulers,ToEulers
	void ToEulersZYX(Vector3 &e) const;

	// <COMBINE Matrix34::ToEulersZYX>
	void ToEulersFastZYX(Vector3 &e) const;

	// PURPOSE
	//   Calculate the quaternion for the rotation described by the 3x3 part of the
	//   current matrix.
	// PARAMS
	//   q - The quaternion that received the calculated rotation.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   FromQuaternion
	void ToQuaternion(Quaternion &q) const;
	

	//==========================================================
	// Scaling

	// PURPOSE: Scale the 3x3 part of the current matrix by a given factor.
	// PARAMS
	//   s - The scaling factor for the matrix.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This just multiplies every component by s.
	// SEE ALSO
	//   ScaleFull,MakeScale
	void Scale(float s);

	// PURPOSE: Scale the 3x3 part of the current matrix by three given factors.
	// PARAMS
	//    x,y,z - The three scaling factors for the matrix.
	// NOTES
	//    - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//      upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//    - This just multiplies the first column of the matrix by x, the second column by y, and the
	//      third column by z.
	// SEE ALSO
	//   ScaleFull,MakeScale
	void Scale(float x,float y,float z);

	// PURPOSE: Scale the 3x3 part of the current matrix by the three elements of a vector.
	// PARAMS
	//   v - The vector to use for the three scaling factors for the matrix.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	//   - This just multiplies the first column of the matrix by v.x, the second column by v.y, and the
	//     third column by v.z.
	// SEE ALSO
	//   ScaleFull,MakeScale
	void Scale(const Vector3 &v);
#if (__XENON || __PS3) && VECTORIZED 
	void Scale(Vector3::Param v);
#endif

	// PURPOSE: Scale the current matrix by a given factor.
	// PARAMS
	//   s - The scaling factor for the matrix.
	// NOTES
	//   - This just multiplies every component by s.
	// SEE ALSO
	//   Scale,MakeScale
	void ScaleFull(float s);

	// PURPOSE: Scale the current matrix by three given factors.
	// PARAMS
	//   x,y,z - The three scaling factors for the matrix.
	// NOTES
	//   - This just multiplies the first column of the matrix by x, the second column by y, and the
	//     third column by z.
	// SEE ALSO
	// Scale,MakeScale
	void ScaleFull(float x,float y,float z);

	// PURPOSE: Scale the current matrix by three given factors taken from a vector.
	// PARAMS
	//   v - The vector containing the three scaling factors for the matrix.
	// NOTES
	//   - This just multiplies the first column of the matrix by v.x, the second column by v.y, and the
	//     third column by v.z.
	// SEE ALSO
	// Scale,MakeScale
	void ScaleFull(const Vector3 &v);

	// PURPOSE: Set the 3x3 part of the current matrix to a scaling matrix.
	// PARAMS
	//   s - The scaling factor for the matrix.
	// NOTES
	//   - A scaling matrix is simply a matrix that, when multiplied with another matrix, will
	//     produce a scaled version of the other matrix.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Scale,ScaleFull
	void MakeScale(float s);

	// PURPOSE: Set the 3x3 part of the current matrix to a scaling matrix.
	// PARAMS
	//   x,y,z - The three scaling factors for the matrix.
	// NOTES
	//   - A scaling matrix is simply a matrix that, when multiplied with another matrix, will
	//     produce a scaled version of the other matrix.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Scale,ScaleFull
	void MakeScale(float x,float y,float z);

	// PURPOSE: Set the 3x3 part of the current matrix to a scaling matrix.
	// PARAMS
	//   v - The vector containing the three scaling factors for the matrix.
	// NOTES
	//   - A scaling matrix is simply a matrix that, when multiplied with another matrix, will
	//     produce a scaled version of the other matrix.
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   Scale,ScaleFull
	void MakeScale(const Vector3 &v);


	//==========================================================
	// Translation

	// PURPOSE: Translate the position of the current matrix by a vector.
	// PARAMS
	//   v - The vector to translate by.
	// SEE ALSO
	//   MakeTranslate
	void Translate(const Vector3 &v);

	// PURPOSE: Translate the position of the current matrix.
	// PARAMS
	//   x,y,z - The three components to translate the position by
	// SEE ALSO
	//   MakeTranslate
	void Translate(float x,float y,float z);

	// PURPOSE: Set the position of the current matrix by a vector.
	// PARAMS
	//   v - The vector to set the position of the matrix to.
	// SEE ALSO
	//   Translate
	void MakeTranslate(const Vector3 &v);

	// PURPOSE: Set the position of the current matrix.
	// PARAMS
	//   x,y,z - The three components to set the position of the matrix to.
	// SEE ALSO
	//   Translate
	void MakeTranslate(float x,float y,float z);


	//==========================================================
	// Inversion

	// PURPOSE: Set the current matrix to its inverse.
	// NOTES
	//   - If the matrix is not invertible, this method generates a warning & returns without
	//     changing the matrix.
	//   - This method is not particularly fast.
	// SEE ALSO
	//   FastInverse,Transpose,Transpose3x4,Inverse3x3
	bool Inverse();

	// PURPOSE: Set the current matrix to the inverse of another matrix.
	// PARAMS
	//   m - The matrix to invert.
	// NOTES
	//   - If the matrix is not invertible, this method generates a warning & returns without
	//     changing the matrix.
	//   - This method is not particularly fast.
	// SEE ALSO
	//   FastInverse,FastInverseScaled,Inverse3x3
	bool Inverse(const Matrix34& m);

	// PURPOSE: Set the current matrix to its inverse.
	// NOTES
	//   - If the matrix is not invertible, this method generates a warning & returns without
	//     changing the matrix.
	//   - This method is not particularly fast.
	// SEE ALSO
	//   FastInverse,Transpose,Transpose3x4,Inverse
	bool Inverse3x3();

	// PURPOSE: Set the current matrix to the inverse of another matrix.
	// PARAMS
	//   m - The matrix to invert.
	// NOTES
	//   - If the matrix is not invertible, this method generates a warning & returns without
	//     changing the matrix.
	//   - This method is not particularly fast.
	// SEE ALSO
	//   FastInverse,FastInverseScaled
	bool Inverse3x3(const Matrix34& m);

	// PURPOSE: Set an orthonormal matrix to its inverse.
	// NOTES
	//   - This method only works on orthonormal matrices.  
	// SEE ALSO
	//   Inverse,FastInverseScaled
	void FastInverse();

	// PURPOSE: Set the current matrix to the inverse of another, orthonormal matrix.
	// PARAMS
	//   m - The matrix to invert.
	// NOTES
	//   - This method only works on orthonormal matrices. If only a uniform scale is applied to all 3 axis,
	//     use FastInvertScaled(), otherwise use Inverse()
	// SEE ALSO
	//   Inverse,FastInverseScaled
	void FastInverse(const Matrix34 &m);

	// PURPOSE: Set the current matrix to the inverse of another, orthonormal matrix.
	// PARAMS
	//   m - The matrix to invert.
	// NOTES
	//   - This method only works on orthonormal matricesand orthonormal matrices 
	//     that have a uniform scale applied. For general matrix inversion use Inverse()
	// SEE ALSO
	//   Inverse, FastInverse
	void FastInverseScaled(const Matrix34 &m);

	// PURPOSE: Transpose the 3x3 part of the current matrix.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   FastInverse,Transpose3x4
	void Transpose();

	// PURPOSE
	//   Set the 3x3 part of the current matrix to the transposition of the 3x3 part of
	//   another matrix.
	// PARAMS
	//   m - The matrix to transpose.
	// NOTES
	//   - When a method only uses the upper 3x3 part of a 3x4 matrix, that generally means that the
	//     upper 3x3 part represents a rotation, and the lower 1x3 part represents a position.
	// SEE ALSO
	//   FastInverse,Transpose3x4
	void Transpose(const Matrix34 &m);

	// PURPOSE: Transpose the current matrix.
	// SEE ALSO
	//   FastInverse,Transpose
	void Transpose3x4();

	// PURPOSE: Set the current matrix to the transposition of another matrix.
	// PARAMS
	//   m - The matrix to transpose.
	// SEE ALSO
	//   FastInverse,Transpose
	void Transpose3x4(const Matrix34 &m);

	// PURPOSE: Invert this orthonormal coordinate matrix.
	// NOTES:
	//	If this matrix is not exactly orthonormal, then it is orthonormalized before inverting.
	void CoordinateInverseSafe(float error=1.0e-5f);

	// PURPOSE: Make this matrix the inverse of the given orthonormal coordinate matrix.
	// NOTES:
	//	If the given matrix is not exactly orthonormal, then it is orthonormalized before inverting.
	void CoordinateInverseSafe(const Matrix34& m, float error=1.0e-5f);


	//==========================================================
	// Viewing

	// PURPOSE: Set the current matrix to one that looks from the current position in the direction 'dir'.
	// PARAMS
	//   dir - The direction to look.
	// SEE ALSO
	//   GetLookAt, LookAt
	void LookDown(const Vector3& dir, const Vector3& up);

	// PURPOSE: Set the current matrix to one that looks from the current position toward the point 'to'.
	// PARAMS
	//   to - The point to look towards.
	// SEE ALSO
	//   GetLookAt, LookAt
	void LookAt(const Vector3 &to, const Vector3& up);

	// PURPOSE: Set the current matrix to one that looks from the point <from> toward the point 'to'.
	// PARAMS
	//   from - The point to look from.
	//   to - The point to look towards.
	// SEE ALSO
	//   GetLookAt, LookAt
	void LookAt(const Vector3 &from, const Vector3 &to, const Vector3& up);

	// PURPOSE
	//   Calculate the points that the current matrix is "looking from" and "looking to",
	//   i.e. perform the inverse of LookAt().
	// PARAMS
	//   LookFrom - An output parameter; it receives the matrix's current position (i.e. the d vector).
	//   LookTo - An output parameter; it receives a point that the matrix is "looking to".
	//   Dist - The distance that LookTo needs to be from LookFrom; used to calculate LookTo.
	// SEE ALSO
	//   LookAt
	void GetLookAt(Vector3 *LookFrom, Vector3 *LookTo, float Dist=1.0f) const;

	// PURPOSE
	//   Calculate the angle between a the z-direction of a matrix's coordinate system and the
	// 	 world z-direction.
	// PARAMS
	//   mtx - The Matrix34 representing a local coordinate system from which to find the azimuth.
	// RETURNS
	//   The angle between a the z-direction of a matrix's coordinate system and the
	// 	 world z-direction.
	float CalculateAzimuth () const;

	// PURPOSE: Set the current matrix to one that represents the given polar coordinate.
	// PARAMS
	//   dist - The distance from the origin.
	//   azm - The azimuth (i.e. the angle in the XZ plane).
	//   inc - The inclination (i.e. the angle from the XZ plane).
	//   twst - The twist, or roll, along the axis described by the azimuth and inclination.
	// SEE ALSO
	//   GetPolar
	void PolarView(float dist,float azm,float inc,float twst=0);

	// PURPOSE: Set the current matrix to one that represents the given polar coordinate.
	// PARAMS
	//   P - A vector containing the polar coordinate, with x holding the distance from the
	//       origin, y holding the azimuth, z holding the inclination, and w holding the twist.
	// SEE ALSO
	//   GetPolar
	void PolarView(const Vector4 &P);

	// PURPOSE: Calculate the polar coordinate described by the current matrix.
	// PARAMS
	//   PolarCoords - An output parameter; the vector receives the distance/azimuth/inclination/twist
	//                 in its x/y/z/w fields, respectively.
	//   PolarOffset - An output parameter; the vector receives a point that the polar coordinate
	//                 is "looking at".
	//   Dist - The distance that PolarOffset needs to be from the origin; used to calculate PolarOffset.
	// SEE ALSO
	//   PolarView
	void GetPolar(Vector4 &PolarCoords, Vector3 &PolarOffset, float Dist) const;

	// PURPOSE: Calculate the polar coordinate described by the current matrix.
	// PARAMS
	//   PolarCoords - An output parameter; the vector receives the distance/azimuth/inclination
	//                 in its x/y/z fields, respectively.
	//   PolarOffset - An output parameter; the vector receives a point that the polar coordinate
	//                 is "looking at".
	//   Dist - The distance that PolarOffset needs to be from the origin; used to calculate PolarOffset.
	// SEE ALSO
	//   PolarView
	void GetPolar(Vector3 &PolarCoords, Vector3 &PolarOffset, float Dist) const;


	//==========================================================
	// Miscelaneous useful functions.

	// PURPOSE: Calculate the cross product between a matrix and a vector.
	// PARAMS
	//   v - The vector to cross with the matrix.
	// NOTES
	//   - This is equivalent to Dot3x3(A.CrossProduct(v)), where A is a temporary Matrix34.
	// SEE ALSO
	//   Dot3x3CrossProdTranspose
	void Dot3x3CrossProdMtx(const Vector3 & v);

	// PURPOSE: Calculate the cross product between the transposition of a matrix and a vector.
	// PARAMS
	//   v - The vector to cross with the transposed matrix.
	// NOTES
	//   - This is equivalent to Dot3x3(A.CrossProduct(-v)), where A is a temporary Matrix34.
	// SEE ALSO
	//   Dot3x3CrossProdMtx
	void Dot3x3CrossProdTranspose(const Vector3 & v);

	// PURPOSE: Create a matrix that is defined by: vector cross (v3 cross vector) = v3 * this
	// PARAMS
	//   vector	- the Vector3 in the above equation 
	void MakeDoubleCrossMatrix(const Vector3& vector);

	// PURPOSE: Create a matrix that is defined by: vectorA cross (v3 cross vectorB) = v3 * this
	// PARAMS
	// 	 vectorA	- the first Vector3 in the above equation
	// 	 vectorB	- the last Vector3 in the above equation 
	void MakeDoubleCrossMatrix(const Vector3& vectorA, const Vector3& vectorB);

	// PURPOSE: Orthonormalize a matrix.
	// NOTES:
	//	1.	An orthonormal matrix has all three vectors with unit length, perpendicular to each other in a right-handed coordinate system
	//		(a.Cross(b)==c).
	//	2.	This is for correcting matrices that are not too far off from orthonormal. To handle extreme cases, such as zero-length vectors
	//		or parallel vectors, use NormalizeSafe() instead.
	void Normalize();

	// PURPOSE: Orthonormalize a matrix, with default values if this matrix has zero-length or parallel vectors.
	void NormalizeSafe();

	// PURPOSE: Calculate a matrix that represents the mirror transform on the given plane
	// PARAMS
	//   plane - plane on which to mirror
	//
	void MirrorOnPlane( const Vector4& plane );

	// PURPOSE: Calculate a matrix that represents an interpolated position between two other matrices.
	// PARAMS
	//   source - The matrix that represents an interpolated position of 0.
	//   goal - The matrix that represents an interpolated position of 1.
	//   t - The interpolation value for the resulting matrix.
	// NOTES
	//   - This uses quaternions to accomplish the interpolated rotation.
	void Interpolate(const Matrix34 &source,const Matrix34 &goal,float t);

	// PURPOSE: Calculate the determinant of the 3x3 part of the current matrix.
	// RETURNS: the determinant.
	float Determinant3x3() const;

	// PURPOSE: Print the value of a matrix.
	// PARAMS
	//   s - A string label to print before the matrix.
	void Print(const char *s=0) const;


	//==========================================================
	// SVD (Singular Value Decomposition) functions

	// PURPOSE: Calculate by singular value decomposition the solution to out*this = in.
	// PARAMS
	//   in		- reference to the vector in in out*this=in
	//   out	- reference to the vector out in out*this=in
	// RETURNS
	//   true if a correct solution is found, false if there is no accurate solution (a closest solution is still found
	// 	 if the return value is false)
	// NOTES
	//   - This method is for non-orthonormal matrices, e.g. those representing angular inertia or inverse mass matrices
	//     UnTransform() should be used for orthonormal (coordinate) matrices.
	//   - Only the 3x3 part of this matrix is used.
	//   - If there is no solution then the vector that is as close as possible to solving the equation is returned.
	//   - If there are multiple solutions then the one with the smallest magnitude is returned. 
	// SEE ALSO
	//   SolveSVDCondition 
	bool SolveSVD(const Vector3& in, Vector3& out) const;
	Vector3 SolveSVD(const Vector3& in) const;

	// PURPOSE: Calculate by singular value decomposition the solution to out*this = in, using a modified version
	// 			of this matrix that is more likely to result in an accurate solution (and that gives the same solution).
	// PARAMS
	//   in		- reference to the vector in in out*this=in
	//   out	- reference to the vector out in out*this=in
	// RETURNS
	//   true if a correct solution is found, false if there is no accurate solution (a closest solution is still found
	// 	 if the return value is false)
	// NOTES
	//   - This method is for non-orthonormal matrices, e.g. those representing angular inertia or inverse mass matrices
	//     UnTransform() should be used for orthonormal (coordinate) matrices.
	//   - Only the 3x3 part of this matrix is used.
	//   - If there is no solution then the vector that is as close as possible to solving the equation is returned.
	//   - If there are multiple solutions then the one with the smallest magnitude is returned. 
	// SEE ALSO
	//   SolveSVD 
	bool SolveSVDCondition(const Vector3& in, Vector3& out) const;


	//==========================================================
	// Operator overloads
	
	// PURPOSE: Copy the value of another matrix to the current matrix
	// PARAMS:
	//	matrix - The matrix to copy into this matrix.
	Matrix34& operator=(const Matrix34& matrix);


	Matrix34& operator=(const Matrix33& matrix);


	//==========================================================
	// Element access

	// PURPOSE: Take a reference to a matrix element.
	//	i -	the index number of the vector in the matrix (the local coordinate axis for orthonormal matrices)
	//	j -	the index number of the element in the vector
	float & GetElement(int i, int j);

	// PURPOSE: Take a const reference to a matrix element.
	// PARAMS:
	//	i -	the index number of the vector in the matrix (the local coordinate axis for orthonormal matrices)
	//	j -	the index number of the element in the vector
	const float & GetElement(int i, int j)	const;

	// PURPOSE: Take a reference to a matrix vector.
	// PARAMS:
	//	i -	the index number of the vector in the matrix (the local coordinate axis for orthonormal matrices)
	Vector3 & GetVector(int i);

	// PURPOSE: Take a const reference to a matrix vector.
	// PARAMS:
	//	i -	the index number of the vector in the matrix (the local coordinate axis for orthonormal matrices)
	const Vector3 & GetVector(int i) const;


	//==========================================================
	// Data

	Vector3 a;
	Vector3 b;
	Vector3 c;
	Vector3 d;

} ;


//=============================================================================

// PURPOSE: global constant matrix object representing the identity 3x4 matrix
extern const Matrix34 M34_IDENTITY;

// PURPOSE: global constant matrix object with all zeros
extern const Matrix34 M34_ZERO;

// PURPOSE: Transform a vector by a matrix.
// PARAMS
//   v - The vector to be transformed.
//   mtx - The matrix that contains the transformation to apply to the vector.
// RETURNS
//   The transformed vector.
// NOTES
//   This is equivalent to Matrix34::Transform().
// SEE ALSO
//   Matrix34::Transform,Matrix34::Transform3x3,Matrix34::UnTransform,Matrix34::UnTransform3x3
Vector3 Dot(const Vector3& v, const Matrix34& mtx);


// PURPOSE: Serialize a matrix object
inline datSerialize & operator<< (datSerialize &s, Matrix34 &m) {
	s << m.a << m.b << m.c << m.d;
	return s;
}

namespace sysEndian
{
	template<> inline void SwapMe(Matrix34& m) {
		SwapMe(m.a);
		SwapMe(m.b);
		SwapMe(m.c);
		SwapMe(m.d);
	}
} // namespace sysEndian

}	// namespace rage

//=============================================================================
// Implementations

// First, include platform specific implementation
#if (__XENON || __PS3) && VECTORIZED
#include "vector/matrix34_xenon.h"
#endif

// Second, include standard version for any that weren't implemented in 
// the platform specific section
#include "vector/matrix34_default.h"

#endif // VECTOR_MATRIX34_H
