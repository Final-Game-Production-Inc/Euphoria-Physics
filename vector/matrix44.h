//
// vector/matrix44.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef VECTOR_MATRIX44_H
#define VECTOR_MATRIX44_H

#include "data/serialize.h"
#include "math/amath.h"
#include "vector/vector4.h"
#include "vector/matrix34.h"
#include "math/intrinsics.h"

namespace rage {

//=============================================================================
// Matrix44
//
// PURPOSE:
//   Matrix44 is a matrix with 4 columns and 4 rows. Although it can do 
//   general 4x4-matrix math, it is commonly used as a Matrix34 with 16-byte 
//   memory alignment.
// NOTES:
//   - Matrix44 operates on vectors from their right side. 
// <FLAG Component>
//
class Matrix44
{
public:
	//==========================================================
	// Construction

	// PURPOSE: Default constuctor.
	// NOTES: As an optimization, this does not initialized the members (nor do the contained vector constructors).
	inline Matrix44();

	Matrix44( const Matrix44& m44 );
	Matrix44( Vector4::Vector4In _a, Vector4::Vector4In _b, Vector4::Vector4In _c, Vector4::Vector4In _d );
	Matrix44& operator=( const Matrix44& m44 );
	

	enum _IdentityType	{ IdentityType };
	enum _ZeroType		{ ZeroType };

	// PURPOSE: Constructor to perform a particular initialization operator
	// USAGE
	//	Matrix44 mat(Matrix44::IdentityType);	// Create an indentity matrix
	//	Matrix44 mat(Matrix44::ZeroType);		// Create a zeroed matrix
	Matrix44(_IdentityType) { Identity(); }
	Matrix44(_ZeroType) { Zero(); }

	// PURPOSE: Constructor.
	inline Matrix44(float ax, float ay, float az, float aw, float bx, float by, float bz, float bw,
	                float cx, float cy, float cz, float cw, float dx, float dy, float dz, float dw);

	// PURPOSE: Resource constructor.
	// NOTES:
	//   - Does not modify data.
	//   - This is here so that we can bypass the default ctor when using placement new.
	Matrix44(class datResource&);

	DECLARE_DUMMY_PLACE(Matrix44);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif


	//==========================================================

	// PURPOSE: Set the current matrix to the identity matrix.
	void Identity();

	// PURPOSE: Zero all the elements of the current matrix.
	void Zero();

	// PURPOSE: Set the current matrix to the value of another matrix.
	// PARAMS:
	//	m - The matrix to be copied.
	void Set(const Matrix44 &m);

	// PURPOSE: Set the elements of the current matrix to the given elements.
	// PARAMS
	//   ax, ay, az, aw - The new values for the a-vector of the current matrix.
	//   bx, by, bz, bw - The new values for the b-vector of the current matrix.
	//   cx, cy, cz, cw - The new values for the c-vector of the current matrix.
	//   dx, dy, dz, dw - The new values for the d-vector of the current matrix.
	void Set(float ax, float ay, float az, float aw, float bx, float by, float bz, float bw,
			float cx, float cy, float cz, float cw, float dx, float dy, float dz, float dw);

	// PURPOSE: Set the current matrix from a 3x4 matrix.
	// PARAMS:
	//	m34 - The 3x4 matrix to be converted into a 4x4 matrix.
	// See also:
	//	ToMatrix34,Convert
	void FromMatrix34(const Matrix34 &m34);

	// PURPOSE: Convert the current matrix into a 3x4 matrix.
	// PARAMS:
	//	m34 - The converted 3x4 matrix.
	// See also:
	//	FromMatrix34,Convert
	// NOTES:
	//	- This method should probably assert that a.w = b.w = c.w = 0 and d.w = 1, to be absolutely correct.
	void ToMatrix34(Matrix34 &m34) const;

	// PURPOSE: Get one of the four elements of this vector.
	// PARAMS:
	//	i -	the index number of the element to get (0 to 3)
	// RETURN:	the value of the specified element
	// NOTES: the argument can be higher than 3 to get an element from an array of Vector4s
	float &GetFloat(int i) const	{return ((float*)this)[i];}				//lint !e740 unusual pointer cast

	// PURPOSE: Get a Vector4 from an array of Vector4s
	// PARAMS:
	//	i -	the index number of the Vector4 to get from the array
	// RETURN: a reference to the specified Vector4 in the array
	Vector4 &GetVector(int i)		{return *(Vector4*)(&GetFloat(i*4));}	//lint !e740 !e826 unusual pointer cast


	//==========================================================
	// Algebra

	// PURPOSE: Add a matrix to the current matrix.
	// PARAMS:
	//	m - The matrix to add to the current matrix.
	// See also:
	//	Subtract,AddScaled
	void Add(const Matrix44 &m);

	// PURPOSE: Set the current matrix to the sum of two other matrices.
	// PARAMS:
	//	m,n - The two matrices to add.
	// See also:
	//	Subtract,AddScaled
	void Add(const Matrix44 &m,const Matrix44 &n);

	// PURPOSE: Subtract a matrix from the current matrix.
	// PARAMS:
	//	m - The matrix to subtract from the current matrix.
	// See also:
	//	Add,AddScaled
	void Subtract(const Matrix44 &m);

	// PURPOSE: Set the current matrix to the difference between two other matrices.
	// PARAMS:
	//	m - The minuend (i.e. the matrix that gets subtracted from).
	//	n - The subtrahend (i.e. the matrix that gets subtracted).
	// See also:
	//	Subtract,AddScaled
	void Subtract(const Matrix44 &m,const Matrix44 &n);

	// PURPOSE: Add a scaled version of another matrix to the current matrix.
	// PARAMS:
	//	m - The matrix to add to the current matrix.
	//	f - The factor by which to scale the matrix before adding it to the current matrix.
	// NOTES:
	//	- This method does not affect the W column's value.
	// See also:
	//	Add,Subtract
	void AddScaled3x4(const Matrix44 &m, float f);

	// PURPOSE: Swap all the off-diagonal elements in pairs across the diagonal.
	void Transpose();

	// PURPOSE: Swap all the off-diagonal elements in pairs across the diagonal.
	// PARAMS:	m - Matrix to take the transpose of.
	void Transpose(const Matrix44& m);

	// PURPOSE: Set the current matrix to the inverse of another matrix.
	// PARAMS:
	//	m - The matrix to be inverted.
	// NOTES:
	//	- This method only works on orthonormal matrices with no scale applied. 
	//	If you can't guarantee that the matrix being inverted is orthonormal
	//	use Inverse() instead. If the matrix is orthonormal and has only a uniform
	//	scale applied use FastInverseScaled()
	// See also:
	//	Inverse, FastInverseScaled
	void FastInverse(const Matrix34& m);

	// PURPOSE: Set the current matrix to the inverse of another matrix.
	// PARAMS:
	//	m - The matrix to be inverted.
	// NOTES:
	//	- This method only works on orthonormal matrices with no scale applied. 
	//	If you can't guarantee that the matrix being inverted is orthonormal
	//	use Inverse() instead. If the matrix is orthonormal and has only a uniform
	//	scale applied use FastInverseScaled()
	// See also:
	//	Inverse, FastInverseScaled
	void FastInverse(const Matrix44& m);			// Assumes W column is 0 0 0 1, and no scaling applied

	// PURPOSE: Set the current matrix to the inverse of another matrix (uniform scale allowed).
	// PARAMS:
	//	m - The matrix to be inverted.
	// NOTES:
	//	- This method only works on orthonormal matrices and orthonormal matrices 
	//	that have a uniform scale applied. If you can't guarantee that the matrix being
	//	inverted is orthonormal use Inverse() instead. If the matrix is orthonormal and 
	//	has no scale applied use FastInverse()
	// See also:
	//	Inverse, FastInverse
	void FastInverseScaled(const Matrix44& m);	// Assumes W column is 0 0 0 1, but supports mtx with uniform scale


	//==========================================================
	// Dot products

	// PURPOSE: Transform the current matrix by another matrix.
	// PARAMS:
	//	m - The matrix containing the transformation to apply to the current matrix.
	// RETURNS: the current matrix, after being transformed.
	// See also:
	//	DotNoVu0
	Matrix44& Dot(const Matrix44 &m);

	// PURPOSE: Set the current matrix to the transformation of one matrix by another matrix.
	// PARAMS:
	//	m - The matrix being transformed.
	//	n - The matrix doing the transforming.
	// RETURNS: The current matrix, after being transformed.
	// NOTES: Neither m nor n can be this.
	// See also:
	//	DotNoVu0
	Matrix44& Dot(const Matrix44 &m, const Matrix44 &n);

	// PURPOSE: Transform a vector by the current matrix.
	// PARAMS:
	//	in - The vector to be transformed.
	//	out - The transformed vector.
	// See also:
	//	Transform3x3,Transform3x3,Transform4,FullTransform
	void Transform(const Vector4 &in,Vector4 &out) const;

	// PURPOSE: Transform a vector by the current matrix.
	// PARAMS:
	//	in - The vector to be transformed.
	//	out - The transformed vector.
	// NOTES:
	//	- If you don't need the homogeneous divide (i.e. if you can guarantee that a.w, b.w,
	//	and c.w are all 0, and d.w is 1), you can use Transform() instead.
	// See also:
	//	Transform3x3,Transform3x3,Transform4,FullTransform
	Vector3 FullTransform(Vector3::Vector3Param in) const;

	// PURPOSE: Transform an array of vectors by the current matrix, producing a different array
	//	containing transformed versions of the vectors.
	// PARAMS:
	//	in - An array of vectors, represented as Vector3 objects, containing the vectors to transform.
	//	out - An output parameter; it points to an array of vectors, represented as Vector4 objects,
	//	containing the transformed vectors.
	//	count - The number of vectors to tranform.  Must be a non-zero multiple of 4.
	// NOTES:
	//	- The w field of the output vector is set to 0.  It should probably be set to 1, but it's not.
	// See also:
	//	Transform
	void Transform4(const Vector3 *in, Vector4 *out, int count) const;

	// PURPOSE: Transform a vector by the current matrix.
	// PARAMS:
	//	in - The vector to be transformed.
	//	out - The transformed vector.
	// NOTES:
	//	- This method doesn't take the matrix's w components into account.  If you need
	//	the homogeneous divide, use FullTransform() instead.
	// See also:
	//	Transform3x3,Transform3x3,Transform4,FullTransform
	void Transform(Vector3::Param in,Vector3::Ref out) const;

	// PURPOSE: Transform a vector by the 3x3 part of the current matrix.
	// PARAMS:
	//	in - The vector to be transformed.
	//	out - The transformed vector.
	// See also:
	//	Transform,Transform3x3,Transform4,FullTransform
	void Transform3x3(const Vector3 & in,Vector3 &out) const;

	// PURPOSE: Calculate the matrix's determinant.
	// RETURN: the matrix's determinant.
	float Determinant() const;

	// PURPOSE: Set another matrix to the inverse of the current matrix.
	// PARAMS:
	//	mtx - The matrix that gets set to the inverse of the current matrix.
	// NOTES:
	//	- This method's calling convention is reversed compared to our normal conventions.
	//	Use Inverse() to get the normal calling convention.
	//	- This method works on all matrices.  If you can guarantee that your matrix is
	//	orthonormal, you may use FastInverse() instead.
	// See also:
	//	Inverse,FastInverse
	void InvertTo(Matrix44&) const;	// screwy order because it's the way the code I copied this from worked

	// PURPOSE: Invert the current matrix.
	// NOTES:
	//	- This method works on all matrices.  If you can guarantee that your matrix is
	//	orthonormal, you may use FastInverse() instead.
	// See also:
	//	FastInverse
	void Inverse();

	// PURPOSE: Set the current matrix to the inverse of another matrix.
	// PARAMS:
	//	mtx - The matrix to be inverted.
	// NOTES:
	//	- This method works on all matrices.  If you can guarantee that your matrix is
	//	orthonormal, you may use FastInverse() instead.
	// See also:
	//	FastInverse
	void Inverse(const Matrix44 &mtx);

	// PURPOSE: Calculate by singular value decomposition the solution to out*this = in.
	// PARAMS:
	//	in - the vector result of out*this, where out is the return value
	// RETURN: the vector out satisfying out*this = in
	// NOTES:
	//	- This method works on non-orthonormal matrices, e.g. those representing the mass or angular inertia.
	//	UnTransform() should be used for orthonormal (coordinate) matrices.
	//	- If there is no solution then the vector that is as close as possible to solving the equation is returned.
	//	- If there are multiple solutions then the one with the smallest magnitude is returned.
	Vector4 SolveSVD (const Vector4& in) const;

	// PURPOSE: Set the 4x3 part of the current matrix to a scaling matrix.
	// PARAMS:
	//	s - The scaling factor for the matrix.
	// NOTES:
	//	- A scaling matrix is simply a matrix that, when multiplied with another matrix, will
	//	produce a scaled version of the other matrix.
	//	- This method sets only the 4x3 part of the matrix.  If you want to set the entire matrix,
	//	use MakeScaleFull() instead.
	// See also:
	//	MakeScaleFull
	void MakeScale(const float s);

	// PURPOSE: Set the 4x3 part of the current matrix to a scaling matrix.
	// PARAMS:
	//	x,y,z - The three scaling factors for the matrix.
	// NOTES:
	//	- A scaling matrix is simply a matrix that, when multiplied with another matrix, will
	//	produce a scaled version of the other matrix.
	//	- This method sets only the 4x3 part of the matrix.  If you want to set the entire matrix,
	//	use MakeScaleFull() instead.
	// See also:
	//	MakeScaleFull
	void MakeScale(const float x, const float y, const float z);

	// PURPOSE: Set the current matrix to a scaling matrix.
	// PARAMS:
	//	s - The scaling factor for the matrix.
	// NOTES:
	//	- A scaling matrix is simply a matrix that, when multiplied with another matrix, will
	//	produce a scaled version of the other matrix.
	//	- This method sets the entire matrix.  If you only want to set the upper 4x3 part, use
	//	MakeScale() instead.
	// See also:
	//	MakeScale
	void MakeScaleFull(const float s);

	// PURPOSE: Set the current matrix to a scaling matrix.
	// PARAMS:
	//	x,y,z - The three scaling factors for the matrix.
	// NOTES:
	//	- A scaling matrix is simply a matrix that, when multiplied with another matrix, will
	//	produce a scaled version of the other matrix.
	//	- This method sets the entire matrix.  If you only want to set the upper 4x3 part, use
	//	MakeScale() instead.
	// See also:
	//	MakeScale
	void MakeScaleFull(const float x, const float y, const float z);

	// PURPOSE: Set the current matrix to represent the given position with zero orientation.
	// PARAMS:
	//	x,y,z - The position to set into the matrix.
	// See also:
	//	MakePosRotY
	void MakePos(float x,float y,float z);

	// PURPOSE: Set the current matrix to represent the given position, with an orientation rotated
	// PARAMS:
	//	x,y,z - The position to set into the matrix.
	// See also:
	//	MakePos
	void MakePosRotY(float x,float y,float z,float cosTheta,float sinTheta);

	// Rotation

	// PURPOSE: Set the current matrix to a rotation about the world's +X axis.
	// PARAMS:
	//	theta - The angle of rotation, in radians.
	// See also:
	//	MakeRotY,MakeRotZ
	Matrix44& MakeRotX(float theta);

	// PURPOSE: Set the current matrix to a rotation about the world's +Y axis.
	// PARAMS:
	//	theta - The angle of rotation, in radians.
	// See also:
	//	MakeRotX,MakeRotZ
	Matrix44& MakeRotY(float theta);

	// PURPOSE: Set the current matrix to a rotation about the world's +Z axis.
	// PARAMS:
	//	theta - The angle of rotation, in radians.
	// See also:
	//	MakeRotX,MakeRotY
	Matrix44& MakeRotZ(float theta);

	// PURPOSE: Set the current matrix to a reflection about an arbitrary plane
	// PARAMS:
	//	plane - The plane to reflect about.
	Matrix44& MakeReflect(const Vector4& plane);

	// PURPOSE: Print the value of a matrix.
	// PARAMS:
	//	s - A string label to print before the matrix.
	void Print(const char *s=0) const;


	//==========================================================
	// Data

	Vector4 a;
	Vector4 b;
	Vector4 c;
	Vector4 d;
} ;

// PURPOSE: Global constant matrix object representing the identity of a 4x4 matrix.
extern const Matrix44 M44_IDENTITY;

//=============================================================================
// Conversion functions

// PURPOSE: Convert a Matrix34 into a Matrix44.
// PARAMS:
//	dest - The converted matrix.
//	src - The matrix to convert.
inline Matrix44& Convert(Matrix44 & dest, const Matrix34 & src)
{
#if __XENON || __PS3
	dest.a.xyzw = __vand(src.a.xyzw, VEC3_ANDW);
	dest.b.xyzw = __vand(src.b.xyzw, VEC3_ANDW);
	dest.c.xyzw = __vand(src.c.xyzw, VEC3_ANDW);
	dest.d.xyzw = __vor(VEC3_ONEW, __vand(src.d.xyzw, VEC3_ANDW));
#else
	// Access these in memory order
	dest.a.x = src.a.x;
	dest.a.y = src.a.y;
	dest.a.z = src.a.z;
	dest.a.w = 0.0f;
	dest.b.x = src.b.x;
	dest.b.y = src.b.y;
	dest.b.z = src.b.z;
	dest.b.w = 0.0f;
	dest.c.x = src.c.x;
	dest.c.y = src.c.y;
	dest.c.z = src.c.z;
	dest.c.w = 0.0f;
	dest.d.x = src.d.x;
	dest.d.y = src.d.y;
	dest.d.z = src.d.z;
	dest.d.w = 1.0f;
#endif
	return dest;
}


// PURPOSE: Convert a Matrix44 into a Matrix34.
// PARAMS:
//	ddest - The converted matrix.
//	dsrc - The matrix to convert.
// NOTES:
//	d- This method should probably assert that a.w = b.w = c.w = 0 and d.w = 1, to be absolutely correct.
inline Matrix34& Convert(Matrix34 & dest, const Matrix44 & src)
{
#if __XENON || __PS3
	dest.a.xyzw = src.a.xyzw;
	dest.b.xyzw = src.b.xyzw;
	dest.c.xyzw = src.c.xyzw;
	dest.d.xyzw = src.d.xyzw;
#else
	dest.a.x = src.a.x;
	dest.a.y = src.a.y;
	dest.a.z = src.a.z;
	dest.b.x = src.b.x;
	dest.b.y = src.b.y;
	dest.b.z = src.b.z;
	dest.c.x = src.c.x;
	dest.c.y = src.c.y;
	dest.c.z = src.c.z;
	dest.d.x = src.d.x;
	dest.d.y = src.d.y;
	dest.d.z = src.d.z;
#endif
	return dest;
}

// PURPOSE: Serialize a matrix object
inline datSerialize & operator<< (datSerialize &s, Matrix44 &m) {
	s << m.a << m.b << m.c << m.d;
	return s;
}

namespace sysEndian
{
	template<> inline void SwapMe(Matrix44& m) {
		SwapMe(m.a);
		SwapMe(m.b);
		SwapMe(m.c);
		SwapMe(m.d);
	}
} // namespace sysEndian


//=============================================================================
// Implemenation


Matrix44::Matrix44()
{
}


Matrix44::Matrix44
(float ax, float ay, float az, float aw,
 float bx, float by, float bz, float bw,
 float cx, float cy, float cz, float cw,
 float dx, float dy, float dz, float dw)
	: a(ax,ay,az,aw)
	, b(bx,by,bz,bw)
	, c(cx,cy,cz,cw)
	, d(dx,dy,dz,dw)
{
}


inline void Matrix44::Transform(const Vector4 &in, Vector4 &out) const
{

#if __PS3 || __XENON	
	__vector4 xxxx = __vspltw(in, 0);
	__vector4 yyyy = __vspltw(in, 1);
	__vector4 zzzz = __vspltw(in, 2);
	__vector4 wwww = __vspltw(in, 3);
	__vector4 result = __vmaddfp(xxxx, a.xyzw, __vmaddfp(yyyy, b.xyzw, __vmaddfp(zzzz, c.xyzw, __vmulfp( wwww, d.xyzw))));
	out = result;

#else

	out.x=in.x*a.x+in.y*b.x+in.z*c.x+in.w*d.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y+in.w*d.y;
	out.z=in.x*a.z+in.y*b.z+in.z*c.z+in.w*d.z;
	out.w=in.x*a.w+in.y*b.w+in.z*c.w+in.w*d.w;
#endif

}


inline void Matrix44::Transform(Vector3::Param in, Vector3::Ref out) const
{

#if __PS3 || __XENON	
	__vector4 xxxx = __vspltw(in, 0);
	__vector4 yyyy = __vspltw(in, 1);
	__vector4 zzzz = __vspltw(in, 2);
	__vector4 result = __vmaddfp(xxxx, a.xyzw, __vmaddfp(yyyy, b.xyzw, __vmaddfp(zzzz, c.xyzw, d.xyzw)));
	out = result;

#else

	out.x=in.x*a.x+in.y*b.x+in.z*c.x+d.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y+d.y;
	out.z=in.x*a.z+in.y*b.z+in.z*c.z+d.z;
#endif
}


inline void Matrix44::Transform3x3(const Vector3 &in, Vector3 &out) const
{
#if __PS3 || __XENON	
	__vector4 xxxx = __vspltw(in, 0);
	__vector4 yyyy = __vspltw(in, 1);
	__vector4 zzzz = __vspltw(in, 2);
	__vector4 result = __vmaddfp(xxxx, a.xyzw, __vmaddfp(yyyy, b.xyzw, __vmulfp(zzzz, c.xyzw)));
	out = result;

#else

	out.x=in.x*a.x+in.y*b.x+in.z*c.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y;
	out.z=in.x*a.z+in.y*b.z+in.z*c.z;
#endif
}


inline Vector3  Matrix44::FullTransform( Vector3::Vector3Param vin) const
{
	Vector3 out;
	Vector3 in(vin);
	out.x=in.x*a.x+in.y*b.x+in.z*c.x+d.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y+d.y;
	out.z=in.x*a.z+in.y*b.z+in.z*c.z+d.z;
	float w=in.x*a.w+in.y*b.w+in.z*c.w+d.w;
	Vector3 vout;
	vout.Scale( out, 1.0f/w );
	return vout;
}


inline void Matrix44::MakeScale(const float s)
{
	a.Set(s,0.0f,0.0f,0.0f);
	b.Set(0.0f,s,0.0f,0.0f);
	c.Set(0.0f,0.0f,s,0.0f);
}


inline void Matrix44::MakeScale(const float x, const float y, const float z)
{
	a.Set(x,0.0f,0.0f,0.0f);
	b.Set(0.0f,y,0.0f,0.0f);
	c.Set(0.0f,0.0f,z,0.0f);
}


inline void Matrix44::MakeScaleFull(const float s)
{
	a.Set(s,0.0f,0.0f,0.0f);
	b.Set(0.0f,s,0.0f,0.0f);
	c.Set(0.0f,0.0f,s,0.0f);
	d.Set(0.0f,0.0f,0.0f,1.0f);
}


inline void Matrix44::MakeScaleFull(const float x, const float y, const float z)
{
	a.Set(x,0.0f,0.0f,0.0f);
	b.Set(0.0f,y,0.0f,0.0f);
	c.Set(0.0f,0.0f,z,0.0f);
	d.Set(0.0f,0.0f,0.0f,1.0f);
}


inline void Matrix44::MakePos(float x, float y, float z)
{
	a.Set(1.0f,0.0f,0.0f,0.0f);
	b.Set(0.0f,1.0f,0.0f,0.0f);
	c.Set(0.0f,0.0f,1.0f,0.0f);
	d.Set(x,y,z,1.0f);
}


inline void Matrix44::MakePosRotY(float x, float y, float z, float cosTheta, float sinTheta)
{
	a.Set(cosTheta,0.0f,sinTheta,0.0f);
	b.Set(0.0f,1.0f,0.0f,0.0f);
	c.Set(-sinTheta,0.0f,cosTheta,0.f);
	d.Set(x,y,z,1.0f);
}

////////////////////////////////////////////////////////////////////////////////

inline Matrix44& Matrix44::Dot(const Matrix44 &m,const Matrix44 &n)
{
#if __XENON || __PS3
	// Load the input matrices into registers. /FF
	// Matrix34::Dot and Matrix44::Dot are defined inconsistently!!!
	// Perform the arg swap here.
	const __vector4 q_a = m.a.xyzw;
	const __vector4 q_b = m.b.xyzw;
	const __vector4 q_c = m.c.xyzw;
	const __vector4 q_d = m.d.xyzw;

	const __vector4 p_a = n.a.xyzw;
	const __vector4 p_b = n.b.xyzw;
	const __vector4 p_c = n.c.xyzw;
	const __vector4 p_d = n.d.xyzw;

	// Xenon doesn't need a zero vector since it's got the vmulfp instruction. /FF
#if !__XENON
	const __vector4 zero = _vzerofp;
#endif

	// Splat the elements of the p matrix. /FF
	const __vector4 splat_pax = __vspltw(p_a, 0);
	const __vector4 splat_pay = __vspltw(p_a, 1);
	const __vector4 splat_paz = __vspltw(p_a, 2);
	const __vector4 splat_paw = __vspltw(p_a, 3);

	const __vector4 splat_pbx = __vspltw(p_b, 0);
	const __vector4 splat_pby = __vspltw(p_b, 1);
	const __vector4 splat_pbz = __vspltw(p_b, 2);
	const __vector4 splat_pbw = __vspltw(p_b, 3);

	const __vector4 splat_pcx = __vspltw(p_c, 0);
	const __vector4 splat_pcy = __vspltw(p_c, 1);
	const __vector4 splat_pcz = __vspltw(p_c, 2);
	const __vector4 splat_pcw = __vspltw(p_c, 3);

	const __vector4 splat_pdx = __vspltw(p_d, 0);
	const __vector4 splat_pdy = __vspltw(p_d, 1);
	const __vector4 splat_pdz = __vspltw(p_d, 2);
	const __vector4 splat_pdw = __vspltw(p_d, 3);

	// Do the multiplications and additions. /FF
#if __XENON
	const __vector4 da1 = __vmulfp(q_a, splat_pax);
	const __vector4 db1 = __vmulfp(q_a, splat_pbx);
	const __vector4 dc1 = __vmulfp(q_a, splat_pcx);
	const __vector4 dd1 = __vmulfp(q_a, splat_pdx);
#else
	const __vector4 da1 = __vmaddfp(q_a, splat_pax, zero);
	const __vector4 db1 = __vmaddfp(q_a, splat_pbx, zero);
	const __vector4 dc1 = __vmaddfp(q_a, splat_pcx, zero);
	const __vector4 dd1 = __vmaddfp(q_a, splat_pdx, zero);
#endif

	const __vector4 da2 = __vmaddfp(q_b, splat_pay, da1);
	const __vector4 db2 = __vmaddfp(q_b, splat_pby, db1);
	const __vector4 dc2 = __vmaddfp(q_b, splat_pcy, dc1);
	const __vector4 dd2 = __vmaddfp(q_b, splat_pdy, dd1);

	const __vector4 da3 = __vmaddfp(q_c, splat_paz, da2);
	const __vector4 db3 = __vmaddfp(q_c, splat_pbz, db2);
	const __vector4 dc3 = __vmaddfp(q_c, splat_pcz, dc2);
	const __vector4 dd3 = __vmaddfp(q_c, splat_pdz, dd2);

	const __vector4 da = __vmaddfp(q_d, splat_paw, da3);
	const __vector4 db = __vmaddfp(q_d, splat_pbw, db3);
	const __vector4 dc = __vmaddfp(q_d, splat_pcw, dc3);
	const __vector4 dd = __vmaddfp(q_d, splat_pdw, dd3);
	// Store the output. /FF

	a.xyzw = da;
	b.xyzw = db;
	c.xyzw = dc;
	d.xyzw = dd;
#else
	FastAssert(this!=&m && this!=&n && "Don't use Dot with this as an argument.");	// lint !e506 constant value boolean
	a.x=n.a.x*m.a.x+n.a.y*m.b.x+n.a.z*m.c.x+n.a.w*m.d.x;
	a.y=n.a.x*m.a.y+n.a.y*m.b.y+n.a.z*m.c.y+n.a.w*m.d.y;
	a.z=n.a.x*m.a.z+n.a.y*m.b.z+n.a.z*m.c.z+n.a.w*m.d.z;
	a.w=n.a.x*m.a.w+n.a.y*m.b.w+n.a.z*m.c.w+n.a.w*m.d.w;

	b.x=n.b.x*m.a.x+n.b.y*m.b.x+n.b.z*m.c.x+n.b.w*m.d.x;
	b.y=n.b.x*m.a.y+n.b.y*m.b.y+n.b.z*m.c.y+n.b.w*m.d.y;
	b.z=n.b.x*m.a.z+n.b.y*m.b.z+n.b.z*m.c.z+n.b.w*m.d.z;
	b.w=n.b.x*m.a.w+n.b.y*m.b.w+n.b.z*m.c.w+n.b.w*m.d.w;

	c.x=n.c.x*m.a.x+n.c.y*m.b.x+n.c.z*m.c.x+n.c.w*m.d.x;
	c.y=n.c.x*m.a.y+n.c.y*m.b.y+n.c.z*m.c.y+n.c.w*m.d.y;
	c.z=n.c.x*m.a.z+n.c.y*m.b.z+n.c.z*m.c.z+n.c.w*m.d.z;
	c.w=n.c.x*m.a.w+n.c.y*m.b.w+n.c.z*m.c.w+n.c.w*m.d.w;

	d.x=n.d.x*m.a.x+n.d.y*m.b.x+n.d.z*m.c.x+n.d.w*m.d.x;
	d.y=n.d.x*m.a.y+n.d.y*m.b.y+n.d.z*m.c.y+n.d.w*m.d.y;
	d.z=n.d.x*m.a.z+n.d.y*m.b.z+n.d.z*m.c.z+n.d.w*m.d.z;
	d.w=n.d.x*m.a.w+n.d.y*m.b.w+n.d.z*m.c.w+n.d.w*m.d.w;
#endif
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

inline Matrix44& Matrix44::Dot(const Matrix44 &m)
{
#if	__XENON || __PS3
	return Dot(m,*this);
#else
	float ax=a.x*m.a.x+a.y*m.b.x+a.z*m.c.x+a.w*m.d.x;
	float ay=a.x*m.a.y+a.y*m.b.y+a.z*m.c.y+a.w*m.d.y;
	float az=a.x*m.a.z+a.y*m.b.z+a.z*m.c.z+a.w*m.d.z;
	float aw=a.x*m.a.w+a.y*m.b.w+a.z*m.c.w+a.w*m.d.w;

	float bx=b.x*m.a.x+b.y*m.b.x+b.z*m.c.x+b.w*m.d.x;
	float by=b.x*m.a.y+b.y*m.b.y+b.z*m.c.y+b.w*m.d.y;
	float bz=b.x*m.a.z+b.y*m.b.z+b.z*m.c.z+b.w*m.d.z;
	float bw=b.x*m.a.w+b.y*m.b.w+b.z*m.c.w+b.w*m.d.w;

	float cx=c.x*m.a.x+c.y*m.b.x+c.z*m.c.x+c.w*m.d.x;
	float cy=c.x*m.a.y+c.y*m.b.y+c.z*m.c.y+c.w*m.d.y;
	float cz=c.x*m.a.z+c.y*m.b.z+c.z*m.c.z+c.w*m.d.z;
	float cw=c.x*m.a.w+c.y*m.b.w+c.z*m.c.w+c.w*m.d.w;

	float dx=d.x*m.a.x+d.y*m.b.x+d.z*m.c.x+d.w*m.d.x;
	float dy=d.x*m.a.y+d.y*m.b.y+d.z*m.c.y+d.w*m.d.y;
	float dz=d.x*m.a.z+d.y*m.b.z+d.z*m.c.z+d.w*m.d.z;
	float dw=d.x*m.a.w+d.y*m.b.w+d.z*m.c.w+d.w*m.d.w;

	a.Set(ax,ay,az,aw);
	b.Set(bx,by,bz,bw);
	c.Set(cx,cy,cz,cw);
	d.Set(dx,dy,dz,dw);
	return *this;
#endif
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::Identity()
{
#if __XENON // take advantage of __vupkd3d
	__vector4 zeroInZ_oneInW = __vupkd3d( _vzerofp, VPACK_NORMSHORT2 );

	a = Vec4VectorSwizzle( zeroInZ_oneInW, VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Z, VEC_PERM_Z );
	b = Vec4VectorSwizzle( zeroInZ_oneInW, VEC_PERM_Z, VEC_PERM_W, VEC_PERM_Z, VEC_PERM_Z );
	c = Vec4VectorSwizzle( zeroInZ_oneInW, VEC_PERM_Z, VEC_PERM_Z, VEC_PERM_W, VEC_PERM_Z );
	d = Vec4VectorSwizzle( zeroInZ_oneInW, VEC_PERM_Z, VEC_PERM_Z, VEC_PERM_Z, VEC_PERM_W );
#elif __PS3
	__vector4 _zero = _vzerofp;
	__vector4 _0001 = (__vector4)vec_sld( vec_splat_s32(0), (_ivector4)vec_ctf( vec_splat_s32(1), 0 ), 0x4 );
	__vector4 _0010 = __vmrglw( _0001, _zero );
	__vector4 _0100 = __vmrglw( _zero, _0010 );
	__vector4 _1000 = __vmrglw( _0010, _zero );
	a = _1000;
	b = _0100;
	c = _0010;
	d = _0001;
#else
	a.Set(1.0f,0.0f,0.0f,0.0f);
	b.Set(0.0f,1.0f,0.0f,0.0f);
	c.Set(0.0f,0.0f,1.0f,0.0f);
	d.Set(0.0f,0.0f,0.0f,1.0f);
#endif
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::Zero()
{
	a.Zero();
	b.Zero();
	c.Zero();
	d.Zero();
}

////////////////////////////////////////////////////////////////////////////////

/*
Purpose: Set the current matrix to the value of another matrix.
Parameters:
m - The matrix to be copied.
*/
inline void Matrix44::Set(const Matrix44 &m)
{
	a.Set(m.a);
	b.Set(m.b);
	c.Set(m.c);
	d.Set(m.d);
}

inline void Matrix44::Set(float ax, float ay, float az, float aw, float bx, float by, float bz, float bw,
		 float cx, float cy, float cz, float cw, float dx, float dy, float dz, float dw)
{
	a.Set(ax,ay,az,aw);
	b.Set(bx,by,bz,bw);
	c.Set(cx,cy,cz,cw);
	d.Set(dx,dy,dz,dw);
}


////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::Transpose()
{
#if __XENON || __PPU
	__vector4 tempVect0 = __vmrglw( a, c );
	__vector4 tempVect1 = __vmrglw( b, d );
	__vector4 tempVect2 = __vmrghw( a, c );
	__vector4 tempVect3 = __vmrghw( b, d );
	d = __vmrglw( tempVect0, tempVect1 );
	c = __vmrghw( tempVect0, tempVect1 );
	b = __vmrglw( tempVect2, tempVect3 );
	a = __vmrghw( tempVect2, tempVect3 );
#else
	SwapEm(b.x,a.y);
	SwapEm(c.x,a.z);
	SwapEm(c.y,b.z);
	SwapEm(d.x,a.w);
	SwapEm(d.y,b.w);
	SwapEm(d.z,c.w);
#endif
}

inline void Matrix44::Transpose(const Matrix44 &m)
{
#if __XENON || __PPU
	__vector4 tempVect0 = __vmrglw( m.a, m.c );
	__vector4 tempVect1 = __vmrglw( m.b, m.d );
	__vector4 tempVect2 = __vmrghw( m.a, m.c );
	__vector4 tempVect3 = __vmrghw( m.b, m.d );
	d = __vmrglw( tempVect0, tempVect1 );
	c = __vmrghw( tempVect0, tempVect1 );
	b = __vmrglw( tempVect2, tempVect3 );
	a = __vmrghw( tempVect2, tempVect3 );
#else
	FastAssert(this!=&m);
	a.Set(m.a.x,m.b.x,m.c.x,m.d.x);
	b.Set(m.a.y,m.b.y,m.c.y,m.d.y);
	c.Set(m.a.z,m.b.z,m.c.z,m.d.z);
	d.Set(m.a.w,m.b.w,m.c.w,m.d.w);
#endif	
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::FromMatrix34(const Matrix34 &m34) {
	::rage::Convert(*this,m34);
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::ToMatrix34(Matrix34 &m34) const
{
	::rage::Convert(m34,*this);
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::Add(const Matrix44 &m)
{
	a.Add(m.a);
	b.Add(m.b);
	c.Add(m.c);
	d.Add(m.d);
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::Add(const Matrix44 &m,const Matrix44 &n)
{
	a.Add(m.a,n.a);
	b.Add(m.b,n.b);
	c.Add(m.c,n.c);
	d.Add(m.d,n.d);
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::Subtract(const Matrix44 &m)
{
	a.Subtract(m.a);
	b.Subtract(m.b);
	c.Subtract(m.c);
	d.Subtract(m.d);
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::Subtract(const Matrix44 &m,const Matrix44 &n)
{
	a.Subtract(m.a,n.a);
	b.Subtract(m.b,n.b);
	c.Subtract(m.c,n.c);
	d.Subtract(m.d,n.d);
}

////////////////////////////////////////////////////////////////////////////////

inline void Matrix44::AddScaled3x4(const Matrix44 &m, float f )
{
	a.AddScaled(m.a,f);
	b.AddScaled(m.b,f);
	c.AddScaled(m.c,f);
	d.AddScaled(m.d,f);
}

__forceinline Matrix44::Matrix44( const Matrix44& m44 )
{
	a = m44.a;
	b = m44.b;
	c = m44.c;
	d = m44.d;
}

#ifndef MATRIX44_4V4
#define MATRIX44_4V4
__forceinline Matrix44::Matrix44( Vector4::Vector4In _a, Vector4::Vector4In _b, Vector4::Vector4In _c, Vector4::Vector4In _d )
{
	a = _a;
	b = _b;
	c = _c;
	d = _d;
}
#endif // MATRIX44_4V4

__forceinline Matrix44& Matrix44::operator=( const Matrix44& m44 )
{
	a = m44.a;
	b = m44.b;
	c = m44.c;
	d = m44.d;
	return *this;
}

}	// namespace rage

#endif // VECTOR_MATRIX44_H
