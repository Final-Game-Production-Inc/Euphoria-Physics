// 
// vector/matrix34_default.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_MATRIX34_DEFAULT_H
#define VECTOR_MATRIX34_DEFAULT_H

#if __RESOURCECOMPILER
	#define EXTRA_TOEULERS_CHECKS 1
#else
#if HACK_GTA4
	#define EXTRA_TOEULERS_CHECKS 1
#else // HACK_GTA4
	#define EXTRA_TOEULERS_CHECKS 0
#endif // HACK_GTA4
#endif

#if EXTRA_TOEULERS_CHECKS
#include "math/simplemath.h"
#endif

// Default Matrix34 Implementations

namespace rage
{

inline float Det22(float a,float b,float c,float d) {return a*d-b*c;}

inline float Det33(float ax,float ay,float az,float bx,float by,float bz,float cx,float cy,float cz) 
{
	return ax*by*cz+ay*bz*cx+az*bx*cy-ax*bz*cy-ay*bx*cz-az*by*cx;
}

	//=============================================================================
	// Implementations

#ifndef MATRIX34_IDENTITY3X3
#define MATRIX34_IDENTITY3X3
inline void Matrix34::Identity3x3()
{
	a.Set(1.0f,0.0f,0.0f);
	b.Set(0.0f,1.0f,0.0f);
	c.Set(0.0f,0.0f,1.0f);
}
#endif	// MATRIX34_IDENTITY3X3

#ifndef MATRIX34_IDENTITY
#define MATRIX34_IDENTITY
inline void Matrix34::Identity()
{
	Identity3x3();
	d.Zero();
}
#endif // MATRIX34_IDENTITY

#ifndef MATRIX34_ZERO3X3
#define MATRIX34_ZERO3X3
inline void Matrix34::Zero3x3()
{
	a.Zero();
	b.Zero();
	c.Zero();
}
#endif // MATRIX34_ZERO3X3

#ifndef MATRIX34_ZERO
#define MATRIX34_ZERO
inline void Matrix34::Zero()
{
	Zero3x3();
	d.Zero();
}
#endif // MATRIX34_ZERO

#ifndef MATRIX34_SET3X3
#define MATRIX34_SET3X3
inline void Matrix34::Set3x3(const Matrix34 &m)
{
	a.Set(m.a);
	b.Set(m.b);
	c.Set(m.c);
}
#endif // MATRIX34_SET3X3

#ifndef MATRIX34_SET_M
#define MATRIX34_SET_M
inline void Matrix34::Set(const Matrix34 &m)
{
	Set3x3(m);
	d.Set(m.d);
}
#endif // MATRIX34_SET_M

#ifndef MATRIX34_SET_V
#define MATRIX34_SET_V
inline void Matrix34::Set(const Vector3& newA, const Vector3& newB, const Vector3& newC, const Vector3& newD)
{
	a.Set(newA);
	b.Set(newB);
	c.Set(newC);
	d.Set(newD);
}
#endif // MATRIX34_SET_V

#ifndef MATRIX34_SET_F
#define MATRIX34_SET_F
inline void Matrix34::Set(float ax, float ay, float az, float bx, float by, float bz,
						  float cx, float cy, float cz, float dx, float dy, float dz)
{
	a.Set(ax,ay,az);
	b.Set(bx,by,bz);
	c.Set(cx,cy,cz);
	d.Set(dx,dy,dz);
}
#endif // MATRIX34_SET_F

#ifndef MATRIX34_SET_DIAGONAL
#define MATRIX34_SET_DIAGONAL
VEC3_INLINE void Matrix34::SetDiagonal( Vector3::Vector3Param d )
{
#if __PPU
	Vector3 dd = d;
	dd.And( VEC3_ANDW );
	a.Permute<VEC_PERM_X, VEC_PERM_W, VEC_PERM_W, VEC_PERM_W>(dd);
	b.Permute<VEC_PERM_W, VEC_PERM_Y, VEC_PERM_W, VEC_PERM_W>(dd);
	c.Permute<VEC_PERM_W, VEC_PERM_W, VEC_PERM_Z, VEC_PERM_W>(dd);
#else
	Vector3 dd(d);
	a.Set(dd.x,0.0f,0.0f);
	b.Set(0.0f,dd.y,0.0f);
	c.Set(0.0f,0.0f,dd.z);
#endif	
}
#endif

#ifndef MATRIX34_ADD
#define MATRIX34_ADD
inline void Matrix34::Add(const Matrix34 &m) 
{
	a.Add(m.a);
	b.Add(m.b);
	c.Add(m.c);
	d.Add(m.d);
}
#endif // MATRIX34_ADD

#ifndef MATRIX34_ADD_M
#define MATRIX34_ADD_M
inline void Matrix34::Add(const Matrix34 &m,const Matrix34 &n) 
{
	a.Add(m.a,n.a);
	b.Add(m.b,n.b);
	c.Add(m.c,n.c);
	d.Add(m.d,n.d);
}
#endif // MATRIX34_ADD_M

#ifndef MATRIX34_ADD3X3
#define MATRIX34_ADD3X3
inline void Matrix34::Add3x3(const Matrix34 &m) 
{
	a.Add(m.a);
	b.Add(m.b);
	c.Add(m.c);
}
#endif // MATRIX34_ADD3X3

#ifndef MATRIX34_ADD3X3_M
#define MATRIX34_ADD3X3_M
inline void Matrix34::Add3x3(const Matrix34 &m,const Matrix34 &n) 
{
	a.Add(m.a,n.a);
	b.Add(m.b,n.b);
	c.Add(m.c,n.c);
}
#endif // MATRIX34_ADD3X3_M

#ifndef MATRIX34_SUBTRACT
#define MATRIX34_SUBTRACT
inline void Matrix34::Subtract(const Matrix34 &m) 
{
	a.Subtract(m.a);
	b.Subtract(m.b);
	c.Subtract(m.c);
	d.Subtract(m.d);
}
#endif // MATRIX34_SUBTRACT

#ifndef MATRIX34_SUBTRACT_M
#define MATRIX34_SUBTRACT_M
inline void Matrix34::Subtract(const Matrix34 &m,const Matrix34 &n) 
{
	a.Subtract(m.a,n.a);
	b.Subtract(m.b,n.b);
	c.Subtract(m.c,n.c);
	d.Subtract(m.d,n.d);
}
#endif // MATRIX34_SUBTRACT_M

#ifndef MATRIX34_SUBTRACT3X3
#define MATRIX34_SUBTRACT3X3
inline void Matrix34::Subtract3x3(const Matrix34 &m) 
{
	a.Subtract(m.a);
	b.Subtract(m.b);
	c.Subtract(m.c);
}
#endif // MATRIX34_SUBTRACT3X3

#ifndef MATRIX34_SUBTRACT3X3_M
#define MATRIX34_SUBTRACT3X3_M
inline void Matrix34::Subtract3x3(const Matrix34 &m,const Matrix34 &n) 
{
	a.Subtract(m.a,n.a);
	b.Subtract(m.b,n.b);
	c.Subtract(m.c,n.c);
}
#endif // MATRIX34_SUBTRACT3X3_M

#ifndef MATRIX34_ADDSCALED3X3
#define MATRIX34_ADDSCALED3X3
inline void Matrix34::AddScaled3x3(const Matrix34 &m, float f )
{
	a.AddScaled(m.a,f);
	b.AddScaled(m.b,f);
	c.AddScaled(m.c,f);
}
#endif // MATRIX34_ADDSCALED3X3

#ifndef MATRIX34_ABS
#define MATRIX34_ABS
inline void Matrix34::Abs() 
{
	a.Abs();
	b.Abs();
	c.Abs();
	d.Abs();
}
#endif // MATRIX34_ABS

#ifndef MATRIX34_NEGATE3X3
#define MATRIX34_NEGATE3X3
inline void Matrix34::Negate3x3()
{
	a.Negate();
	b.Negate();
	c.Negate();
}
#endif // MATRIX34_NEGATE3X3

#ifndef MATRIX34_NEGATE
#define MATRIX34_NEGATE
inline void Matrix34::Negate()
{
	Negate3x3();
	d.Negate();
}
#endif // MATRIX34_NEGATE

#ifndef MATRIX34_DOT
#define MATRIX34_DOT
inline void Matrix34::Dot(const Matrix34 &m) 
{
	float ax=a.x*m.a.x+a.y*m.b.x+a.z*m.c.x;
	float ay=a.x*m.a.y+a.y*m.b.y+a.z*m.c.y;
	float az=a.x*m.a.z+a.y*m.b.z+a.z*m.c.z;

	float bx=b.x*m.a.x+b.y*m.b.x+b.z*m.c.x;
	float by=b.x*m.a.y+b.y*m.b.y+b.z*m.c.y;
	float bz=b.x*m.a.z+b.y*m.b.z+b.z*m.c.z;

	float cx=c.x*m.a.x+c.y*m.b.x+c.z*m.c.x;
	float cy=c.x*m.a.y+c.y*m.b.y+c.z*m.c.y;
	float cz=c.x*m.a.z+c.y*m.b.z+c.z*m.c.z;

	float dx=d.x*m.a.x+d.y*m.b.x+d.z*m.c.x+m.d.x;
	float dy=d.x*m.a.y+d.y*m.b.y+d.z*m.c.y+m.d.y;
	float dz=d.x*m.a.z+d.y*m.b.z+d.z*m.c.z+m.d.z;

	a.Set(ax,ay,az);
	b.Set(bx,by,bz);
	c.Set(cx,cy,cz);
	d.Set(dx,dy,dz);
}
#endif // MATRIX34_DOT

#ifndef MATRIX34_DOTFROMLEFT
#define MATRIX34_DOTFROMLEFT
inline void Matrix34::DotFromLeft(const Matrix34& m)
{
	float ax=m.a.x*a.x+m.a.y*b.x+m.a.z*c.x;
	float ay=m.a.x*a.y+m.a.y*b.y+m.a.z*c.y;
	float az=m.a.x*a.z+m.a.y*b.z+m.a.z*c.z;

	float bx=m.b.x*a.x+m.b.y*b.x+m.b.z*c.x;
	float by=m.b.x*a.y+m.b.y*b.y+m.b.z*c.y;
	float bz=m.b.x*a.z+m.b.y*b.z+m.b.z*c.z;

	float cx=m.c.x*a.x+m.c.y*b.x+m.c.z*c.x;
	float cy=m.c.x*a.y+m.c.y*b.y+m.c.z*c.y;
	float cz=m.c.x*a.z+m.c.y*b.z+m.c.z*c.z;

	float dx=m.d.x*a.x+m.d.y*b.x+m.d.z*c.x+d.x;
	float dy=m.d.x*a.y+m.d.y*b.y+m.d.z*c.y+d.y;
	float dz=m.d.x*a.z+m.d.y*b.z+m.d.z*c.z+d.z;

	a.Set(ax,ay,az);
	b.Set(bx,by,bz);
	c.Set(cx,cy,cz);
	d.Set(dx,dy,dz);
}
#endif // MATRIX34_DOTFROMLEFT

#ifndef MATRIX34_DOT_M
#define MATRIX34_DOT_M
inline void Matrix34::Dot(const Matrix34 &m,const Matrix34 &n)
{
	FastAssert(this!=&m && this!=&n && "Don't use Dot with this as an argument.");	// lint !e506 constant value boolean

	a.x=m.a.x*n.a.x+m.a.y*n.b.x+m.a.z*n.c.x;
	a.y=m.a.x*n.a.y+m.a.y*n.b.y+m.a.z*n.c.y;
	a.z=m.a.x*n.a.z+m.a.y*n.b.z+m.a.z*n.c.z;

	b.x=m.b.x*n.a.x+m.b.y*n.b.x+m.b.z*n.c.x;
	b.y=m.b.x*n.a.y+m.b.y*n.b.y+m.b.z*n.c.y;
	b.z=m.b.x*n.a.z+m.b.y*n.b.z+m.b.z*n.c.z;

	c.x=m.c.x*n.a.x+m.c.y*n.b.x+m.c.z*n.c.x;
	c.y=m.c.x*n.a.y+m.c.y*n.b.y+m.c.z*n.c.y;
	c.z=m.c.x*n.a.z+m.c.y*n.b.z+m.c.z*n.c.z;

	d.x=m.d.x*n.a.x+m.d.y*n.b.x+m.d.z*n.c.x+n.d.x;
	d.y=m.d.x*n.a.y+m.d.y*n.b.y+m.d.z*n.c.y+n.d.y;
	d.z=m.d.x*n.a.z+m.d.y*n.b.z+m.d.z*n.c.z+n.d.z;
}
#endif // MATRIX34_DOT_M

#ifndef MATRIX34_DOTTRANSPOSE
#define MATRIX34_DOTTRANSPOSE
inline void Matrix34::DotTranspose(const Matrix34 &n)
{
	FastAssert(this!=&n && "Don't use DotTranspose with this as an argument.");	// lint !e506 constant value boolean

	a.Dot3x3Transpose(n);
	b.Dot3x3Transpose(n);
	c.Dot3x3Transpose(n);
	d.Subtract(n.d);
	d.Dot3x3Transpose(n);
}
#endif // MATRIX34_DOTTRANSPOSE

#ifndef MATRIX34_DOTTRANSPOSE_M
#define MATRIX34_DOTTRANSPOSE_M
inline void Matrix34::DotTranspose(const Matrix34 &m, const Matrix34 &n)
{
	FastAssert(this!=&m && this!=&n && "Don't use DotTranspose with this as an argument.");	// lint !e506 constant value boolean

	a.x=m.a.x*n.a.x+m.a.y*n.a.y+m.a.z*n.a.z;
	a.y=m.a.x*n.b.x+m.a.y*n.b.y+m.a.z*n.b.z;
	a.z=m.a.x*n.c.x+m.a.y*n.c.y+m.a.z*n.c.z;

	b.x=m.b.x*n.a.x+m.b.y*n.a.y+m.b.z*n.a.z;
	b.y=m.b.x*n.b.x+m.b.y*n.b.y+m.b.z*n.b.z;
	b.z=m.b.x*n.c.x+m.b.y*n.c.y+m.b.z*n.c.z;

	c.x=m.c.x*n.a.x+m.c.y*n.a.y+m.c.z*n.a.z;
	c.y=m.c.x*n.b.x+m.c.y*n.b.y+m.c.z*n.b.z;
	c.z=m.c.x*n.c.x+m.c.y*n.c.y+m.c.z*n.c.z;

	Vector3 delD;
	delD.Subtract(m.d,n.d);
	d.x=delD.x*n.a.x+delD.y*n.a.y+delD.z*n.a.z;
	d.y=delD.x*n.b.x+delD.y*n.b.y+delD.z*n.b.z;
	d.z=delD.x*n.c.x+delD.y*n.c.y+delD.z*n.c.z;
}
#endif // MATRIX34_DOTTRANSPOSE_M

#ifndef MATRIX34_DOT3X3
#define MATRIX34_DOT3X3
inline void Matrix34::Dot3x3(const Matrix34 &m)
{
	FastAssert(this!=&m && "Don't use Dot3x3 with this as an argument.");	// lint !e506 constant value boolean

	float ax=a.x*m.a.x+a.y*m.b.x+a.z*m.c.x;
	float ay=a.x*m.a.y+a.y*m.b.y+a.z*m.c.y;
	float az=a.x*m.a.z+a.y*m.b.z+a.z*m.c.z;

	float bx=b.x*m.a.x+b.y*m.b.x+b.z*m.c.x;
	float by=b.x*m.a.y+b.y*m.b.y+b.z*m.c.y;
	float bz=b.x*m.a.z+b.y*m.b.z+b.z*m.c.z;

	float cx=c.x*m.a.x+c.y*m.b.x+c.z*m.c.x;
	float cy=c.x*m.a.y+c.y*m.b.y+c.z*m.c.y;
	float cz=c.x*m.a.z+c.y*m.b.z+c.z*m.c.z;

	a.Set(ax,ay,az);
	b.Set(bx,by,bz);
	c.Set(cx,cy,cz);
}
#endif // MATRIX34_DOT3X3

#ifndef MATRIX34_DOT3X3FROMLEFT
#define MATRIX34_DOT3X3FROMLEFT
inline void Matrix34::Dot3x3FromLeft(const Matrix34& m)
{
	FastAssert(this!=&m && "Don't use Dot3x3FromLeft with this as an argument.");	// lint !e506 constant value boolean

	float ax=m.a.x*a.x+m.a.y*b.x+m.a.z*c.x;
	float ay=m.a.x*a.y+m.a.y*b.y+m.a.z*c.y;
	float az=m.a.x*a.z+m.a.y*b.z+m.a.z*c.z;

	float bx=m.b.x*a.x+m.b.y*b.x+m.b.z*c.x;
	float by=m.b.x*a.y+m.b.y*b.y+m.b.z*c.y;
	float bz=m.b.x*a.z+m.b.y*b.z+m.b.z*c.z;

	float cx=m.c.x*a.x+m.c.y*b.x+m.c.z*c.x;
	float cy=m.c.x*a.y+m.c.y*b.y+m.c.z*c.y;
	float cz=m.c.x*a.z+m.c.y*b.z+m.c.z*c.z;

	a.Set(ax,ay,az);
	b.Set(bx,by,bz);
	c.Set(cx,cy,cz);
}
#endif // MATRIX34_DOT3X3FROMLEFT

#ifndef MATRIX34_DOT3X3_M
#define MATRIX34_DOT3X3_M
inline void Matrix34::Dot3x3(const Matrix34 &m,const Matrix34 &n)
{
	FastAssert(this!=&m && this!=&n && "Don't use Dot3x3 with this as an argument.");	// lint !e506 constant value boolean

	a.x=m.a.x*n.a.x+m.a.y*n.b.x+m.a.z*n.c.x;
	a.y=m.a.x*n.a.y+m.a.y*n.b.y+m.a.z*n.c.y;
	a.z=m.a.x*n.a.z+m.a.y*n.b.z+m.a.z*n.c.z;

	b.x=m.b.x*n.a.x+m.b.y*n.b.x+m.b.z*n.c.x;
	b.y=m.b.x*n.a.y+m.b.y*n.b.y+m.b.z*n.c.y;
	b.z=m.b.x*n.a.z+m.b.y*n.b.z+m.b.z*n.c.z;

	c.x=m.c.x*n.a.x+m.c.y*n.b.x+m.c.z*n.c.x;
	c.y=m.c.x*n.a.y+m.c.y*n.b.y+m.c.z*n.c.y;
	c.z=m.c.x*n.a.z+m.c.y*n.b.z+m.c.z*n.c.z;
}
#endif // MATRIX34_DOT3X3_M

#ifndef MATRIX34_DOT3X3TRANSPOSE
#define MATRIX34_DOT3X3TRANSPOSE
inline void Matrix34::Dot3x3Transpose( const Matrix34 &n )
{
	FastAssert(this!=&n && "Don't use Dot3x3Transpose with this as an argument.");	// lint !e506 constant value boolean

	a.Dot3x3Transpose(n);
	b.Dot3x3Transpose(n);
	c.Dot3x3Transpose(n);
}
#endif // MATRIX34_DOT3X3TRANSPOSE

#ifndef MATRIX34_DOT3X3TRANSPOSE_M
#define MATRIX34_DOT3X3TRANSPOSE_M
inline void Matrix34::Dot3x3Transpose( const Matrix34 &m, const Matrix34 &n )
{
	FastAssert(this!=&m && this!=&n && "Don't use Dot3x3Transpose with this as an argument.");	// lint !e506 constant value boolean

	a.x=m.a.x*n.a.x+m.a.y*n.a.y+m.a.z*n.a.z;
	a.y=m.a.x*n.b.x+m.a.y*n.b.y+m.a.z*n.b.z;
	a.z=m.a.x*n.c.x+m.a.y*n.c.y+m.a.z*n.c.z;

	b.x=m.b.x*n.a.x+m.b.y*n.a.y+m.b.z*n.a.z;
	b.y=m.b.x*n.b.x+m.b.y*n.b.y+m.b.z*n.b.z;
	b.z=m.b.x*n.c.x+m.b.y*n.c.y+m.b.z*n.c.z;

	c.x=m.c.x*n.a.x+m.c.y*n.a.y+m.c.z*n.a.z;
	c.y=m.c.x*n.b.x+m.c.y*n.b.y+m.c.z*n.b.z;
	c.z=m.c.x*n.c.x+m.c.y*n.c.y+m.c.z*n.c.z;
}
#endif // MATRIX34_DOT3X3TRANSPOSE_M

#ifndef MATRIX34_ISEQUAL
#define MATRIX34_ISEQUAL
inline bool Matrix34::IsEqual(const Matrix34& m) const
{
	return (a.IsEqual(m.a) && b.IsEqual(m.b) && c.IsEqual(m.c) && d.IsEqual(m.d));
}
#endif // MATRIX34_ISEQUAL

#ifndef MATRIX34_ISNOTEQUAL
#define MATRIX34_ISNOTEQUAL
inline bool Matrix34::IsNotEqual(const Matrix34& m) const
{
	return (a.IsNotEqual(m.a) || b.IsNotEqual(m.b) || c.IsNotEqual(m.c) || d.IsNotEqual(m.d));
}
#endif // MATRIX34_ISNOTEQUAL

#ifndef MATRIX34_ISCLOSE
#define MATRIX34_ISCLOSE
inline bool Matrix34::IsClose(const Matrix34& m, float error) const
{
	return (a.IsClose(m.a,error) && b.IsClose(m.b,error) && c.IsClose(m.c,error) && d.IsClose(m.d,error));
}
#endif // MATRIX34_ISCLOSE

#ifndef MATRIX34_ISORTHONORMAL
#define MATRIX34_ISORTHONORMAL
inline bool Matrix34::IsOrthonormal (float error) const
{
	// Convert the fractional error to the difference in the square of the magnitude (to first order).
	// square(1+error)-1 = 2*error
	float squareError = 2.0f*error;
	if (fabsf(a.Mag2()-1.0f)>squareError)
	{
		// The a vector does not have length close enough to 1.
		return false;
	}
	if (fabsf(b.Mag2()-1.0f)>squareError)
	{
		// The b vector does not have length close enough to 1.
		return false;
	}
	if (fabsf(c.Mag2()-1.0f)>squareError)
	{
		// The c vector does not have length close enough to 1.
		return false;
	}
	if (fabsf(a.Dot(b))>squareError)
	{
		// a is not sufficiently perpendicular to b.
		return false;
	}
	if (fabsf(a.Dot(c))>squareError)
	{
		// a is not sufficiently perpendicular to c.
		return false;
	}
	if (fabsf(b.Dot(c))>squareError)
	{
		// b is not sufficiently perpendicular to c.
		return false;
	}

	// The three coordinate vectors in this matrix are all mutually nearly perpendicular and all have length nearly 1.
	return true;
}
#endif // MATRIX34_ISORTHONORMAL

#ifndef MATRIX34_MEASURENONORTHONORMALITY
#define MATRIX34_MEASURENONORTHONORMALITY
inline float Matrix34::MeasureNonOrthonormality(float errorThreshold) const
{
	float maxError = 0.0f;

	//Diagonal terms of M * M^T - I
	maxError = Max(maxError, fabsf(a.Mag2()-1.0f));
	maxError = Max(maxError, fabsf(b.Mag2()-1.0f));
	maxError = Max(maxError, fabsf(c.Mag2()-1.0f));

	//Off-diagonal terms of M * M^T - I
	maxError = Max(maxError, fabsf(a.Dot(b)));
	maxError = Max(maxError, fabsf(a.Dot(c)));
	maxError = Max(maxError, fabsf(b.Dot(c)));

	return maxError / (2.0f * errorThreshold);
}
#endif // MATRIX34_MEASURENONORTHONORMALITY

#ifndef MATRIX34_CROSSPRODUCT
#define MATRIX34_CROSSPRODUCT
inline void Matrix34::CrossProduct(Vector3::Param vr)
{
	Vector3 r(vr);
	a.Set(0.0f,-r.z,r.y);
	b.Set(r.z,0.0f,-r.x);
	c.Set(-r.y,r.x,0.0f);
}
#endif

#ifndef MATRIX34_OUTERPRODUCT
#define MATRIX34_OUTERPRODUCT
inline void Matrix34::OuterProduct(const Vector3& u, const Vector3& v)
{
	a.x = u.x*v.x;
	a.y = u.x*v.y;
	a.z = u.x*v.z;
	b.x = u.y*v.x;
	b.y = u.y*v.y;
	b.z = u.y*v.z;
	c.x = u.z*v.x;
	c.y = u.z*v.y;
	c.z = u.z*v.z;
}
#endif // MATRIX34_OUTERPRODUCT

#ifndef MATRIX34_TRANSFORMXZ
#define MATRIX34_TRANSFORMXZ
inline void Matrix34::TransformXZ(const Vector3 &in,Vector2 &out) const 
{
	out.x=in.x*a.x+in.y*b.x+in.z*c.x+d.x;
	out.y=in.x*a.z+in.y*b.z+in.z*c.z+d.z;
}
#endif // MATRIX34_TRANSFORMXZ

#ifndef MATRIX34_TRANSFORM_V
#define MATRIX34_TRANSFORM_V
inline void Matrix34::Transform(Vector3::Param vin,Vector3::Vector3Ref vout) const 
{
	FastAssert(&vin != &vout);
	Vector3 in(vin);
	Vector3 out;

	out.x=in.x*a.x+in.y*b.x+in.z*c.x+d.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y+d.y;
	out.z=in.x*a.z+in.y*b.z+in.z*c.z+d.z;
	vout = out;
}
#endif // MATRIX34_TRANSFORM_V

#ifndef MATRIX34_TRANSFORM
#define MATRIX34_TRANSFORM
inline void Matrix34::Transform(Vector3::Vector3Ref vinAndOut) const 
{
	Vector3 inAndOut(vinAndOut);
	float newX=inAndOut.x*a.x+inAndOut.y*b.x+inAndOut.z*c.x+d.x;
	float newY=inAndOut.x*a.y+inAndOut.y*b.y+inAndOut.z*c.y+d.y;
	float newZ=inAndOut.x*a.z+inAndOut.y*b.z+inAndOut.z*c.z+d.z;

	vinAndOut = Vector3(newX, newY, newZ);
}
#endif // MATRIX34_TRANSFORM

#ifndef MATRIX34_TRANSFORM3X3_V
#define MATRIX34_TRANSFORM3X3_V
inline void Matrix34::Transform3x3(Vector3::Param vin,Vector3::Vector3Ref vout) const 
{
	Vector3 in(vin);
	Vector3 out;

	out.x=in.x*a.x+in.y*b.x+in.z*c.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y;
	out.z=in.x*a.z+in.y*b.z+in.z*c.z;
	vout = out;
}
#endif // MATRIX34_TRANSFORM3X3_V

#ifndef MATRIX34_TRANSFORM3X3
#define MATRIX34_TRANSFORM3X3
inline void Matrix34::Transform3x3(Vector3::Vector3Ref vinAndOut) const
{
	Vector3 inAndOut(vinAndOut);
	float newX=inAndOut.x*a.x+inAndOut.y*b.x+inAndOut.z*c.x;
	float newY=inAndOut.x*a.y+inAndOut.y*b.y+inAndOut.z*c.y;
	float newZ=inAndOut.x*a.z+inAndOut.y*b.z+inAndOut.z*c.z;

	vinAndOut = Vector3(newX, newY, newZ);
}
#endif // MATRIX34_TRANSFORM3X3

#ifndef MATRIX34_TRANSFORM3X3_V2
#define MATRIX34_TRANSFORM3X3_V2
inline void Matrix34::Transform3x3(Vector3::Param vin,Vector2 &out) const 
{
	Vector3 in(vin);
	out.x=in.x*a.x+in.y*b.x+in.z*c.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y;
}
#endif // MATRIX34_TRANSFORM3X3_V2

#ifndef MATRIX34_UNTRANSFORM_V
#define MATRIX34_UNTRANSFORM_V
inline void Matrix34::UnTransform(Vector3::Param in,Vector3::Ref out) const
{
	FastAssert(&in != &out);

	Vector3 temp;
	temp.Subtract(in,d);
	out.x = a.Dot(temp);
	out.y = b.Dot(temp);
	out.z = c.Dot(temp);
}
#endif // MATRIX34_UNTRANSFORM_V

#ifndef MATRIX34_UNTRANSFORM
#define MATRIX34_UNTRANSFORM
inline void Matrix34::UnTransform(Vector3::Ref inAndOut) const 
{
	Vector3 temp(inAndOut);
	temp.Subtract(d);
	inAndOut.x=a.Dot(temp);
	inAndOut.y=b.Dot(temp);
	inAndOut.z=c.Dot(temp);
}
#endif // MATRIX34_UNTRANSFORM

#ifndef MATRIX34_UNTRANSFORM3X3_V
#define MATRIX34_UNTRANSFORM3X3_V
inline void Matrix34::UnTransform3x3(Vector3::Param in,Vector3::Ref out) const
{
	FastAssert(&in != &out);

	out.x = a.Dot(in);
	out.y = b.Dot(in);
	out.z = c.Dot(in);
}
#endif // MATRIX34_UNTRANSFORM3X3_V

#ifndef MATRIX34_UNTRANSFORM3X3
#define MATRIX34_UNTRANSFORM3X3
inline void Matrix34::UnTransform3x3(Vector3::Ref inAndOut) const 
{
	float newX=a.Dot(inAndOut);
	float newY=b.Dot(inAndOut);
	inAndOut.z=c.Dot(inAndOut);
	inAndOut.x=newX;
	inAndOut.y=newY;
}
#endif // MATRIX34_UNTRANSFORM3X3

#ifndef MATRIX34_UNTRANSFORM3X3_V2
#define MATRIX34_UNTRANSFORM3X3_V2
inline void Matrix34::UnTransform3x3(Vector3::Param in,Vector2 &out) const
{
	out.x = a.Dot(in);
	out.y = b.Dot(in);
}
#endif // MATRIX34_UNTRANSFORM3X3_V2

#ifndef MATRIX34_TRANSFORM4
#define MATRIX34_TRANSFORM4
inline void Matrix34::Transform4(const Vector3 *in, Vector4 *out, int count) const
{
	int i;
	FastAssert((count & 3) == 0);	// lint !e506 constant value boolean
	for (i = count-1; i >= 0; i--)
	{
		out[i].x = in[i].x*a.x + in[i].y*b.x + in[i].z*c.x + d.x;
		out[i].y = in[i].x*a.y + in[i].y*b.y + in[i].z*c.y + d.y;
		out[i].z = in[i].x*a.z + in[i].y*b.z + in[i].z*c.z + d.z;
		out[i].w = 0;
	}
}
#endif // MATRIX34_TRANSFORM4

#ifndef MATRIX34_GETLOCALUP
#define MATRIX34_GETLOCALUP
inline Vector3 Matrix34::GetLocalUp() const
{
	Vector3 localUp;
	Transform3x3(g_UnitUp,localUp);
	return localUp;
}
#endif

#ifndef MATRIX34_ROTATEX
#define MATRIX34_ROTATEX
inline void Matrix34::RotateX(float t) 
{
	Matrix34 mtx;
	mtx.MakeRotateX(t);
	this->Dot3x3(mtx);
}
#endif // MATRIX34_ROTATEX

#ifndef MATRIX34_ROTATEY
#define MATRIX34_ROTATEY
inline void Matrix34::RotateY(float t) 
{
	Matrix34 mtx;
	mtx.MakeRotateY(t);
	this->Dot3x3(mtx);
}
#endif // MATRIX34_ROTATEY

#ifndef MATRIX34_ROTATEZ
#define MATRIX34_ROTATEZ
inline void Matrix34::RotateZ(float t) 
{
	Matrix34 mtx;
	mtx.MakeRotateZ(t);
	this->Dot3x3(mtx);
}
#endif // MATRIX34_ROTATEZ

#ifndef MATRIX34_ROTATEUNITAXIS
#define MATRIX34_ROTATEUNITAXIS
inline void Matrix34::RotateUnitAxis(const Vector3 &va,float t) 
{
	Assertf(va.Mag2() >= square(0.999f) && va.Mag2() <= square(1.001f), "Vector3 <%f, %f, %f> does not have length 1",va.x,va.y,va.z);
	Matrix34 mtx;
	mtx.MakeRotateUnitAxis(va,t);
	this->Dot3x3(mtx);
}
#endif // MATRIX34_ROTATEUNITAXIS

#ifndef MATRIX34_ROTATE
#define MATRIX34_ROTATE
inline void Matrix34::Rotate(const Vector3 &va,float t) 
{
	Matrix34 mtx;
	mtx.MakeRotate(va,t);
	this->Dot3x3(mtx);
}
#endif // MATRIX34_ROTATE

#ifndef MATRIX34_ROTATELOCALX
#define MATRIX34_ROTATELOCALX
inline void Matrix34::RotateLocalX(float angle)
{
	float cosine,sine;
	cos_and_sin(cosine,sine,angle);
	Vector3 rotatedB(b);
	rotatedB.Scale(cosine);
	rotatedB.AddScaled(c,sine);
	c.Scale(cosine);
	c.SubtractScaled(b,sine);
	b.Set(rotatedB);
}
#endif // MATRIX34_ROTATELOCALX

#ifndef MATRIX34_ROTATELOCALY
#define MATRIX34_ROTATELOCALY
inline void Matrix34::RotateLocalY(float angle)
{
	float cosine,sine;
	cos_and_sin(cosine,sine,angle);
	Vector3 rotatedA(a);
	rotatedA.Scale(cosine);
	rotatedA.SubtractScaled(c,sine);
	c.Scale(cosine);
	c.AddScaled(a,sine);
	a.Set(rotatedA);
}
#endif // MATRIX34_ROTATELOCALY

#ifndef MATRIX34_ROTATELOCALZ
#define MATRIX34_ROTATELOCALZ
inline void Matrix34::RotateLocalZ(float angle)
{
	float cosine,sine;
	cos_and_sin(cosine,sine,angle);
	Vector3 rotatedA(a);
	rotatedA.Scale(cosine);
	rotatedA.AddScaled(b,sine);
	b.Scale(cosine);
	b.SubtractScaled(a,sine);
	a.Set(rotatedA);
}
#endif // MATRIX34_ROTATELOCALZ

#ifndef MATRIX34_ROTATELOCALAXIS
#define MATRIX34_ROTATELOCALAXIS
inline void Matrix34::RotateLocalAxis (float angle, int axisIndex)
{
	switch (axisIndex)
	{
		case 0:
		{
			RotateLocalX(angle);
			break;
		}
		case 1:
		{
			RotateLocalY(angle);
			break;
		}
		case 2:
		{
			RotateLocalZ(angle);
			break;
		}
		default:
		{
			VecAssertMsg(0,"Invalid axis index");
		}
	}
}
#endif // MATRIX34_ROTATELOCALAXIS

#ifndef MATRIX34_ROTATEFULL
#define MATRIX34_ROTATEFULL
inline void Matrix34::RotateFull(const Vector3 &va,float t) 
{
	Matrix34 mtx;
	mtx.MakeRotate(va,t);
	mtx.d.Zero();
	this->Dot(mtx);
}
#endif // MATRIX34_ROTATEFULL

#ifndef MATRIX34_ROTATEFULLX
#define MATRIX34_ROTATEFULLX
inline void Matrix34::RotateFullX(float t) 
{
	Matrix34 mtx;
	mtx.MakeRotateX(t);
	mtx.d.Zero();
	this->Dot(mtx);
}
#endif // MATRIX34_ROTATEFULLX

#ifndef MATRIX34_ROTATEFULLY
#define MATRIX34_ROTATEFULLY
inline void Matrix34::RotateFullY(float t) 
{
	Matrix34 mtx;
	mtx.MakeRotateY(t);
	mtx.d.Zero();
	this->Dot(mtx);
}
#endif // MATRIX34_ROTATEFULLY

#ifndef MATRIX34_ROTATEFULLZ
#define MATRIX34_ROTATEFULLZ
inline void Matrix34::RotateFullZ(float t) 
{
	Matrix34 mtx;
	mtx.MakeRotateZ(t);
	mtx.d.Zero();
	this->Dot(mtx);
}
#endif // MATRIX34_ROTATEFULLZ

#ifndef MATRIX34_ROTATEFULLUNITAXIS
#define MATRIX34_ROTATEFULLUNITAXIS
inline void Matrix34::RotateFullUnitAxis(const Vector3 &va,float t) 
{
	Assertf(va.Mag2() >= square(0.999f) && va.Mag2() <= square(1.001f), "Vector3 <%f, %f, %f> does not have length 1",va.x,va.y,va.z);
	Matrix34 mtx;
	mtx.MakeRotateUnitAxis(va,t);
	mtx.d.Zero();
	this->Dot(mtx);
}
#endif

#ifndef MATRIX34_MAKEROTATE
#define MATRIX34_MAKEROTATE
inline void Matrix34::MakeRotate(const Vector3 &va,float t) 
{
	if(t==0.0f) {
		Identity3x3();
		return;
	}
	if(va.x==0.0f) {
		if(va.y==0.0f) {
			if(va.z>0.0f) MakeRotateZ(t);
			else MakeRotateZ(-t);
			return;
		}
		if(va.z==0.0f) {
			if(va.y>0.0f) MakeRotateY(t);
			else MakeRotateY(-t);
			return;
		}
	}
	else if(va.y==0.0f && va.z==0.0f) {
		if(va.x>0.0f) MakeRotateX(t);
		else MakeRotateX(-t);
		return;
	}

	Vector3 v;
	v.Normalize(va);
	MakeRotateUnitAxis(v,t);
}
#endif // MATRIX34_MAKEROTATE

#ifndef MATRIX34_MAKEROTATEX
#define MATRIX34_MAKEROTATEX
inline void Matrix34::MakeRotateX(float t) 
{
	float cost,sint;
	//float cost=rage::Cosf(t);
	//float sint=rage::Sinf(t);
	rage::cos_and_sin( cost,sint,t );

	a.Set(1.0f, 0.0f, 0.0f);
	b.Set(0.0f, cost, sint);
	c.Set(0.0f,-sint, cost);
}
#endif // MATRIX34_MAKEROTATEX

#ifndef MATRIX34_MAKEROTATEY
#define MATRIX34_MAKEROTATEY
inline void Matrix34::MakeRotateY(float t) 
{
	float cost,sint;
	//float cost=rage::Cosf(t);
	//float sint=rage::Sinf(t);
	rage::cos_and_sin( cost,sint,t );

	a.Set(cost, 0.0f,-sint);
	b.Set(0.0f, 1.0f, 0.0f);
	c.Set(sint, 0.0f, cost);
}
#endif // MATRIX34_MAKEROTATEY

#ifndef MATRIX34_MAKEROTATEZ
#define MATRIX34_MAKEROTATEZ
inline void Matrix34::MakeRotateZ(float t) 
{
	float cost,sint;
	//float cost=rage::Cosf(t);
	//float sint=rage::Sinf(t);
	rage::cos_and_sin( cost,sint,t );

	a.Set( cost, sint, 0.0f);
	b.Set(-sint, cost, 0.0f);
	c.Set( 0.0f, 0.0f, 1.0f);
}
#endif // MATRIX34_MAKEROTATEZ

#ifndef MATRIX34_MAKEROTATEUNITAXIS
#define MATRIX34_MAKEROTATEUNITAXIS
inline void Matrix34::MakeRotateUnitAxis(const Vector3 &v,float t) 
{
	Assertf(v.Mag2() >= square(0.999f) && v.Mag2() <= square(1.001f), "Vector3 <%f, %f, %f> does not have length 1",v.x,v.y,v.z);

	float cost,sint;
	//float cost=rage::Cosf(t);
	//float sint=rage::Sinf(t);
	rage::cos_and_sin( cost,sint,t );
	float omc=1.0f-cost;

	a.x=omc*v.x*v.x+cost;
	b.y=omc*v.y*v.y+cost;
	c.z=omc*v.z*v.z+cost;
	a.y=omc*v.x*v.y+sint*v.z;
	b.x=omc*v.x*v.y-sint*v.z;
	a.z=omc*v.x*v.z-sint*v.y;
	c.x=omc*v.x*v.z+sint*v.y;
	b.z=omc*v.y*v.z+sint*v.x;
	c.y=omc*v.y*v.z-sint*v.x;
}
#endif // MATRIX34_MAKEROTATEUNITAXIS

#ifndef MATRIX34_ROTATETO
#define MATRIX34_ROTATETO
inline void Matrix34::RotateTo (const Vector3& unitFrom, const Vector3& unitTo) 
{
	Vector3 axis;
	float angle;
	if (ComputeRotation(unitFrom,unitTo,axis,angle))
	{
		// Rotate the matrix.
		RotateUnitAxis(axis,angle);
	}
}
#endif // MATRIX34_ROTATETO

#ifndef MATRIX34_ROTATETO_F
#define MATRIX34_ROTATETO_F
inline void Matrix34::RotateTo (const Vector3& unitFrom, const Vector3& unitTo, float t)
{
	Vector3 axis;
	float angle;
	if (ComputeRotation(unitFrom,unitTo,axis,angle))
	{
		// Rotate the matrix.
		RotateUnitAxis(axis,angle*t);
	}
}
#endif // MATRIX34_ROTATETO_F

#ifndef MATRIX34_MAKEROTATETO
#define MATRIX34_MAKEROTATETO
inline void Matrix34::MakeRotateTo (const Vector3& unitFrom, const Vector3& unitTo) 
{
	Vector3 axis;
	float angle;
	if (ComputeRotation(unitFrom,unitTo,axis,angle))
	{
		// Make the rotation matrix.
		MakeRotateUnitAxis(axis,angle);
	}
	else
	{
		Identity3x3();
	}
}
#endif // MATRIX34_MAKEROTATETO

#ifndef MATRIX34_MAKEUPRIGHT
#define MATRIX34_MAKEUPRIGHT
inline void Matrix34::MakeUpright()
{
	// Find this matrix's up direction in world coordinates.
	Vector3 upDirection;
	Transform3x3(g_UnitUp,upDirection);

	// Rotate this matrix so that it's up direction is the world up direction.
	RotateTo(upDirection,g_UnitUp);
}
#endif // MATRIX34_MAKEUPRIGHT

#ifndef MATRIX34_GETEULERS
#define MATRIX34_GETEULERS
inline Vector3 Matrix34::GetEulers() const // xyz order
{
	Vector3 out;
	out.Zero();
	ToEulersXYZ(out);
	return out;
}
#endif // MATRIX34_GETEULERS

#ifndef MATRIX34_GETEULERSFAST
#define MATRIX34_GETEULERSFAST
inline Vector3 Matrix34::GetEulersFast() const // xyz order
{
	return Vector3(rage::Atan2f(b.z,c.z),rage::Asinf(-a.z),rage::Atan2f(a.y,a.x));
}
#endif // MATRIX34_GETEULERSFAST

#ifndef MATRIX34_GETEULERS_S
#define MATRIX34_GETEULERS_S
inline Vector3 Matrix34::GetEulers(const char *order) const 
{
	Vector3 out;
	out.Zero();
	ToEulers(out, order);
	return out;
}
#endif // MATRIX34_GETEULERS_S

#ifndef MATRIX34_GETEULERSFAST_S
#define MATRIX34_GETEULERSFAST_S
inline Vector3 Matrix34::GetEulersFast(const char *order) const 
{
	if (order[2]=='x')
	{
		if (order[1] == 'y' && order[0] == 'z')	   // zyx order 
			return Vector3(rage::Atan2f(-c.y,c.z),rage::Asinf(c.x),rage::Atan2f(-b.x,a.x));
		else if (order[1] == 'z' && order[0] == 'y')  // yzx order
			return Vector3(rage::Atan2f(b.z,b.y),rage::Atan2f(c.x,a.x),rage::Asinf(-b.x));
	} 
	else if (order[2]=='y')
	{
		if (order[1]=='x' && order[0] == 'z')		 // zxy order
			return Vector3(rage::Asinf(-c.y),rage::Atan2f(c.x,c.z),rage::Atan2f(a.y,b.y));
		else if (order[1] == 'z' && order[0] == 'x')  // xzy order
			return Vector3(rage::Atan2f(-c.y,b.y),rage::Atan2f(-a.z,a.x),rage::Asinf(a.y));
	}
	else if (order[2]=='z')
	{
		if (order[1]=='x' && order[0] == 'y')		// yxz order
			return Vector3(rage::Asinf(b.z),rage::Atan2f(-a.z,c.z), rage::Atan2f(-b.x,b.y));
		else if (order[1]=='y' &&  order[0] == 'x')  // xyz order
			return Vector3(rage::Atan2f(b.z,c.z),rage::Asinf(-a.z),rage::Atan2f(a.y,a.x));
	}

	Warningf("Matrix34::GetEulers() - order variable is invalid"); 
	return Vector3(0.f,0.f,0.f);
}
#endif // MATRIX34_GETEULERSFAST_S

#ifndef MATRIX34_FROMEULERS
#define MATRIX34_FROMEULERS
inline void Matrix34::FromEulers(const Vector3 &e,const char *order) 
{
	if(order[0]=='x') {
		if(order[1]=='y' && order[2]=='z') FromEulersXYZ(e);
		else if(order[1]=='z' && order[2]=='y') FromEulersXZY(e);
		else Warningf("Matrix34::FromEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='y') {
		if(order[1]=='x' && order[2]=='z') FromEulersYXZ(e);
		else if(order[1]=='z' && order[2]=='x') FromEulersYZX(e);
		else Warningf("Matrix34::FromEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='z') {
		if(order[1]=='x' && order[2]=='y') FromEulersZXY(e);
		else if(order[1]=='y' && order[2]=='x') FromEulersZYX(e);
		else Warningf("Matrix34::FromEulers()- Bad string '%s'",order);
	}
	else Warningf("Matrix34::FromEulers()- Bad string '%s'",order);
}
#endif // MATRIX34_FROMEULERS

#ifndef MATRIX34_FROMEULERSXYZ
#define MATRIX34_FROMEULERSXYZ
inline void Matrix34::FromEulersXYZ(const Vector3 &e) 
{
	float sx,sy,sz,cx,cy,cz;
	if(e.x==0.0f) {sx=0.0f; cx=1.0f;}
	else {cos_and_sin(cx,sx,e.x);}
	if(e.y==0.0f) {sy=0.0f; cy=1.0f;}
	else {cos_and_sin(cy,sy,e.y);}
	if(e.z==0.0f) {sz=0.0f; cz=1.0f;}
	else {cos_and_sin(cz,sz,e.z);}

	a.Set( cy*cz,          cy*sz,          -sy );
	b.Set( sx*sy*cz-cx*sz, sx*sy*sz+cx*cz, sx*cy );
	c.Set( cx*sy*cz+sx*sz, cx*sy*sz-sx*cz, cx*cy );
}
#endif // MATRIX34_FROMEULERSXYZ

#ifndef MATRIX34_FROMEULERSXZY
#define MATRIX34_FROMEULERSXZY
inline void Matrix34::FromEulersXZY(const Vector3 &e) 
{
	float sx,sy,sz,cx,cy,cz;
	if(e.x==0.0f) {sx=0.0f; cx=1.0f;}
	else {cos_and_sin(cx,sx,e.x);}
	if(e.y==0.0f) {sy=0.0f; cy=1.0f;}
	else {cos_and_sin(cy,sy,e.y);}
	if(e.z==0.0f) {sz=0.0f; cz=1.0f;}
	else {cos_and_sin(cz,sz,e.z);}

	a.Set(  cz*cy,             sz,    -cz*sy);
	b.Set( -cx*sz*cy + sx*sy,  cx*cz,  cx*sz*sy + sx*cy);
	c.Set(  sx*sz*cy + cx*sy, -sx*cz, -sx*sz*sy + cx*cy);
}
#endif // MATRIX34_FROMEULERSXZY

#ifndef MATRIX34_FROMEULERSYXZ
#define MATRIX34_FROMEULERSYXZ
inline void Matrix34::FromEulersYXZ(const Vector3 &e) 
{
	float sx,sy,sz,cx,cy,cz;
	if(e.x==0.0f) {sx=0.0f; cx=1.0f;}
	else {cos_and_sin(cx,sx,e.x);}
	if(e.y==0.0f) {sy=0.0f; cy=1.0f;}
	else {cos_and_sin(cy,sy,e.y);}
	if(e.z==0.0f) {sz=0.0f; cz=1.0f;}
	else {cos_and_sin(cz,sz,e.z);}

	a.Set( cy*cz - sy*sx*sz, cy*sz + sy*sx*cz, -sy*cx);
	b.Set(-cx*sz,            cx*cz,             sx);
	c.Set( sy*cz + cy*sx*sz, sy*sz - cy*sx*cz,  cy*cx); 
}
#endif // MATRIX34_FROMEULERSYXZ

#ifndef MATRIX34_FROMEULERSYZX
#define MATRIX34_FROMEULERSYZX
inline void Matrix34::FromEulersYZX(const Vector3 &e) 
{
	float sx,sy,sz,cx,cy,cz;
	if(e.x==0.0f) {sx=0.0f; cx=1.0f;}
	else {cos_and_sin(cx,sx,e.x);}
	if(e.y==0.0f) {sy=0.0f; cy=1.0f;}
	else {cos_and_sin(cy,sy,e.y);}
	if(e.z==0.0f) {sz=0.0f; cz=1.0f;}
	else {cos_and_sin(cz,sz,e.z);}

	a.Set( cy*cz, cy*sz*cx + sy*sx, cy*sz*sx - sy*cx);
	b.Set(   -sz, cz*cx,            cz*sx);
	c.Set( sy*cz, sy*sz*cx - cy*sx, sy*sz*sx + cy*cx);
}
#endif // MATRIX34_FROMEULERSYZX

#ifndef MATRIX34_FROMEULERSZXY
#define MATRIX34_FROMEULERSZXY
inline void Matrix34::FromEulersZXY(const Vector3 &e) 
{
	float sx,sy,sz,cx,cy,cz;
	if(e.x==0.0f) {sx=0.0f; cx=1.0f;}
	else {cos_and_sin(cx,sx,e.x);}
	if(e.y==0.0f) {sy=0.0f; cy=1.0f;}
	else {cos_and_sin(cy,sy,e.y);}
	if(e.z==0.0f) {sz=0.0f; cz=1.0f;}
	else {cos_and_sin(cz,sz,e.z);}

	a.Set(  cz*cy + sz*sx*sy, sz*cx, -cz*sy + sz*sx*cy);
	b.Set( -sz*cy + cz*sx*sy, cz*cx,  sz*sy + cz*sx*cy);
	c.Set(  cx*sy,           -sx,     cx*cy);
}
#endif // MATRIX34_FROMEULERSZXY

#ifndef MATRIX34_FROMEULERSZYX
#define MATRIX34_FROMEULERSZYX
inline void Matrix34::FromEulersZYX(const Vector3 &e) 
{
	float sx,sy,sz,cx,cy,cz;
	if(e.x==0.0f) {sx=0.0f; cx=1.0f;}
	else {cos_and_sin(cx,sx,e.x);}
	if(e.y==0.0f) {sy=0.0f; cy=1.0f;}
	else {cos_and_sin(cy,sy,e.y);}
	if(e.z==0.0f) {sz=0.0f; cz=1.0f;}
	else {cos_and_sin(cz,sz,e.z);}

	a.Set(  cz*cy, sz*cx + cz*sy*sx, sz*sx - cz*sy*cx);
	b.Set( -sz*cy, cz*cx - sz*sy*sx, cz*sx + sz*sy*cx);
	c.Set(	 sy,  -cy*sx,            cy*cx);
}
#endif // MATRIX34_FROMEULERSZYX

#ifndef MATRIX34_TOEULERS
#define MATRIX34_TOEULERS
inline void Matrix34::ToEulers(Vector3 &e,const char *order) const 
{
	if(order[0]=='x') {
		if(order[1]=='y' && order[2]=='z') ToEulersXYZ(e);
		else if(order[1]=='z' && order[2]=='y') ToEulersXZY(e);
		else Warningf("Matrix34::ToEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='y') {
		if(order[1]=='x' && order[2]=='z') ToEulersYXZ(e);
		else if(order[1]=='z' && order[2]=='x') ToEulersYZX(e);
		else Warningf("Matrix34::ToEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='z') {
		if(order[1]=='x' && order[2]=='y') ToEulersZXY(e);
		else if(order[1]=='y' && order[2]=='x') ToEulersZYX(e);
		else Warningf("Matrix34::ToEulers()- Bad string '%s'",order);
	}
	else Warningf("Matrix34::ToEulers()- Bad string '%s'",order);
}
#endif // MATRIX34_TOEULERS

#ifndef MATRIX34_TOEULERSFAST
#define MATRIX34_TOEULERSFAST
inline void Matrix34::ToEulersFast(Vector3 &e,const char *order) const 
{
	if(order[0]=='x') {
		if(order[1]=='y' && order[2]=='z') ToEulersFastXYZ(e);
		else if(order[1]=='z' && order[2]=='y') ToEulersFastXZY(e);
		else Warningf("Matrix34::ToEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='y') {
		if(order[1]=='x' && order[2]=='z') ToEulersFastYXZ(e);
		else if(order[1]=='z' && order[2]=='x') ToEulersFastYZX(e);
		else Warningf("Matrix34::ToEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='z') {
		if(order[1]=='x' && order[2]=='y') ToEulersFastZXY(e);
		else if(order[1]=='y' && order[2]=='x') ToEulersFastZYX(e);
		else Warningf("Matrix34::ToEulers()- Bad string '%s'",order);
	}
	else Warningf("Matrix34::ToEulers()- Bad string '%s'",order);
}
#endif // MATRIX34_TOEULERSFAST

#ifndef MATRIX34_TOEULERSXYZ
#define MATRIX34_TOEULERSXYZ
inline void Matrix34::ToEulersXYZ(Vector3 &e) const 
{
	Matrix34 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
#if EXTRA_TOEULERS_CHECKS
	if ((IsNearZero(NormalizedMtx.b.z, SMALL_FLOAT) && IsNearZero(NormalizedMtx.c.z, SMALL_FLOAT)) || 
		(IsNearZero(NormalizedMtx.a.y, SMALL_FLOAT) && IsNearZero(NormalizedMtx.a.x, SMALL_FLOAT)) ||
		(fabsf(NormalizedMtx.a.z) > (1.0f - SMALL_FLOAT)))
	{
		e.Set(safe_atan2f(-NormalizedMtx.c.y, NormalizedMtx.b.y), rage::AsinfSafe(-NormalizedMtx.a.z), 0);
		return;
	}
#endif
	e.Set(safe_atan2f(NormalizedMtx.b.z,NormalizedMtx.c.z),rage::Asinf(-NormalizedMtx.a.z),safe_atan2f(NormalizedMtx.a.y,NormalizedMtx.a.x));
}
#endif // MATRIX34_TOEULERSXYZ

#ifndef MATRIX34_TOEULERSFASTXYZ
#define MATRIX34_TOEULERSFASTXYZ
inline void Matrix34::ToEulersFastXYZ(Vector3 &e) const 
{
	e.Set(safe_atan2f(b.z,c.z),rage::Asinf(-a.z),safe_atan2f(a.y,a.x));
}
#endif // MATRIX34_TOEULERSFASTXYZ

#ifndef MATRIX34_TOEULERSXZY
#define MATRIX34_TOEULERSXZY
inline void Matrix34::ToEulersXZY(Vector3 &e) const 
{
	Matrix34 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
#if EXTRA_TOEULERS_CHECKS
	if ((IsNearZero(NormalizedMtx.a.x, SMALL_FLOAT) && IsNearZero(NormalizedMtx.a.z, SMALL_FLOAT)) || 
		(IsNearZero(NormalizedMtx.b.y, SMALL_FLOAT) && IsNearZero(NormalizedMtx.c.y, SMALL_FLOAT)) ||
		(fabsf(NormalizedMtx.a.y) > (1.0f - SMALL_FLOAT)))
	{
		e.Set(safe_atan2f(NormalizedMtx.b.z, NormalizedMtx.c.z), 0, rage::AsinfSafe(NormalizedMtx.a.y));
		return;
	}
#endif
	e.Set(safe_atan2f(-NormalizedMtx.c.y,NormalizedMtx.b.y),safe_atan2f(-NormalizedMtx.a.z,NormalizedMtx.a.x),rage::Asinf(NormalizedMtx.a.y));
}
#endif // MATRIX34_TOEULERSXZY

#ifndef MATRIX34_TOEULERSFASTXZY
#define MATRIX34_TOEULERSFASTXZY
inline void Matrix34::ToEulersFastXZY(Vector3 &e) const 
{
	e.Set(safe_atan2f(-c.y,b.y),safe_atan2f(-a.z,a.x),rage::Asinf(a.y));
}
#endif // MATRIX34_TOEULERSFASTXZY

#ifndef MATRIX34_TOEULERSYXZ
#define MATRIX34_TOEULERSYXZ
inline void Matrix34::ToEulersYXZ(Vector3 &e) const 
{
	Matrix34 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
#if EXTRA_TOEULERS_CHECKS
	if ((IsNearZero(NormalizedMtx.b.x, SMALL_FLOAT) && IsNearZero(NormalizedMtx.b.y, SMALL_FLOAT)) || 
		(IsNearZero(NormalizedMtx.a.z, SMALL_FLOAT) && IsNearZero(NormalizedMtx.c.z, SMALL_FLOAT)) ||
		(fabsf(NormalizedMtx.b.z) > (1.0f - SMALL_FLOAT)))
	{
		e.Set(rage::AsinfSafe(NormalizedMtx.b.z), safe_atan2f(NormalizedMtx.c.x, NormalizedMtx.a.x), 0);
		return;
	}
#endif
	e.Set(rage::Asinf(NormalizedMtx.b.z),safe_atan2f(-NormalizedMtx.a.z,NormalizedMtx.c.z),safe_atan2f(-NormalizedMtx.b.x,NormalizedMtx.b.y));
}
#endif // MATRIX34_TOEULERSYXZ

#ifndef MATRIX34_TOEULERSFASTYXZ
#define MATRIX34_TOEULERSFASTYXZ
inline void Matrix34::ToEulersFastYXZ(Vector3 &e) const 
{
	e.Set(rage::Asinf(b.z),safe_atan2f(-a.z,c.z),safe_atan2f(-b.x,b.y));
}
#endif // MATRIX34_TOEULERSFASTYXZ

#ifndef MATRIX34_TOEULERSYZX
#define MATRIX34_TOEULERSYZX
inline void Matrix34::ToEulersYZX(Vector3 &e) const 
{
	Matrix34 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
#if EXTRA_TOEULERS_CHECKS
	if ((IsNearZero(NormalizedMtx.b.y, SMALL_FLOAT) && IsNearZero(NormalizedMtx.b.z, SMALL_FLOAT)) || 
		(IsNearZero(NormalizedMtx.a.x, SMALL_FLOAT) && IsNearZero(NormalizedMtx.c.x, SMALL_FLOAT)) ||
		(fabsf(NormalizedMtx.b.x) > (1.0f - SMALL_FLOAT)))
	{
		e.Set(0, safe_atan2f(-NormalizedMtx.a.z, NormalizedMtx.c.z), rage::AsinfSafe(-NormalizedMtx.b.x));
		return;
	}
#endif
	e.Set(safe_atan2f(NormalizedMtx.b.z,NormalizedMtx.b.y),safe_atan2f(NormalizedMtx.c.x,NormalizedMtx.a.x),rage::Asinf(-NormalizedMtx.b.x));
}
#endif // MATRIX34_TOEULERSYZX

#ifndef MATRIX34_TOEULERSFASTYZX
#define MATRIX34_TOEULERSFASTYZX
inline void Matrix34::ToEulersFastYZX(Vector3 &e) const 
{
	e.Set(safe_atan2f(b.z,b.y),safe_atan2f(c.x,a.x),rage::Asinf(-b.x));
}
#endif // MATRIX34_TOEULERSFASTYZX

#ifndef MATRIX34_TOEULERSZXY
#define MATRIX34_TOEULERSZXY
inline void Matrix34::ToEulersZXY(Vector3 &e) const 
{
	Matrix34 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
#if EXTRA_TOEULERS_CHECKS
	if ((IsNearZero(NormalizedMtx.a.y, SMALL_FLOAT) && IsNearZero(NormalizedMtx.b.y, SMALL_FLOAT)) || 
		(IsNearZero(NormalizedMtx.c.x, SMALL_FLOAT) && IsNearZero(NormalizedMtx.c.z, SMALL_FLOAT)) ||
		(fabsf(NormalizedMtx.c.y) > (1.0f - SMALL_FLOAT)))
	{
		e.Set(rage::AsinfSafe(-NormalizedMtx.c.y), 0, safe_atan2f(-NormalizedMtx.b.x, NormalizedMtx.a.x));
		return;
	}
#endif
	e.Set(rage::Asinf(-NormalizedMtx.c.y),safe_atan2f(NormalizedMtx.c.x,NormalizedMtx.c.z),safe_atan2f(NormalizedMtx.a.y,NormalizedMtx.b.y));
}
#endif // MATRIX34_TOEULERSZXY

#ifndef MATRIX34_TOEULERSFASTZXY
#define MATRIX34_TOEULERSFASTZXY
inline void Matrix34::ToEulersFastZXY(Vector3 &e) const 
{
	e.Set(rage::Asinf(-c.y),safe_atan2f(c.x,c.z),safe_atan2f(a.y,b.y));
}
#endif // MATRIX34_TOEULERSFASTZXY

#ifndef MATRIX34_TOEULERSZYX
#define MATRIX34_TOEULERSZYX
inline void Matrix34::ToEulersZYX(Vector3 &e) const 
{
	Matrix34 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
#if EXTRA_TOEULERS_CHECKS
	if ((IsNearZero(NormalizedMtx.a.x, SMALL_FLOAT) && IsNearZero(NormalizedMtx.b.x, SMALL_FLOAT)) || 
		(IsNearZero(NormalizedMtx.c.y, SMALL_FLOAT) && IsNearZero(NormalizedMtx.c.z, SMALL_FLOAT)) ||
		(fabsf(NormalizedMtx.c.x) > (1.0f - SMALL_FLOAT)))
	{
		e.Set(0, rage::AsinfSafe(NormalizedMtx.c.x), safe_atan2f(NormalizedMtx.a.y, NormalizedMtx.b.y));
		return;
	}
#endif
	e.Set(safe_atan2f(-NormalizedMtx.c.y,NormalizedMtx.c.z),rage::Asinf(NormalizedMtx.c.x),safe_atan2f(-NormalizedMtx.b.x,NormalizedMtx.a.x));
}
#endif // MATRIX34_TOEULERSZYX

#ifndef MATRIX34_TOEULERSFASTZYX
#define MATRIX34_TOEULERSFASTZYX
inline void Matrix34::ToEulersFastZYX(Vector3 &e) const 
{
	e.Set(safe_atan2f(-c.y,c.z),rage::Asinf(c.x),safe_atan2f(-b.x,a.x));
}
#endif // MATRIX34_TOEULERSFASTZYX

#ifndef MATRIX34_SCALE
#define MATRIX34_SCALE
inline void Matrix34::Scale(float s) 
{
	a.Scale(s);
	b.Scale(s);
	c.Scale(s);
}
#endif // MATRIX34_SCALE

#ifndef MATRIX34_SCALE_F
#define MATRIX34_SCALE_F
inline void Matrix34::Scale(float x,float y,float z) 
{
	a.x*=x; b.x*=x; c.x*=x;
	a.y*=y; b.y*=y; c.y*=y;
	a.z*=z; b.z*=z; c.z*=z;
}
#endif // MATRIX34_SCALE_F

#ifndef MATRIX34_SCALE_V
#define MATRIX34_SCALE_V
inline void Matrix34::Scale(const Vector3& v)
{
	Scale(v.x,v.y,v.z);
}
#endif // MATRIX34_SCALE_V

#if (__XENON || __PS3) && VECTORIZED 

#ifndef MATRIX34_SCALE_V_NATIVE
#define MATRIX34_SCALE_V_NATIVE
inline void Matrix34::Scale(Vector3::Param v)
{
	Scale(v);
}
#endif // MATRIX34_SCALE_V_NATIVE

#endif // #if (__XENON || __PS3) && VECTORIZED 


#ifndef MATRIX34_SCALEFULL
#define MATRIX34_SCALEFULL
inline void Matrix34::ScaleFull(float s) 
{
	a.Scale(s);
	b.Scale(s);
	c.Scale(s);
	d.Scale(s);
}
#endif // MATRIX34_SCALEFULL

#ifndef MATRIX34_SCALLEFULL_F
#define MATRIX34_SCALLEFULL_F
inline void Matrix34::ScaleFull(float x,float y,float z) 
{
	a.x*=x; b.x*=x; c.x*=x; d.x*=x;
	a.y*=y; b.y*=y; c.y*=y; d.y*=y;
	a.z*=z; b.z*=z; c.z*=z; d.z*=z;
}
#endif // MATRIX34_SCALEFULL_F

#ifndef MATRIX34_SCALLEFULL_V
#define MATRIX34_SCALLEFULL_V
inline void Matrix34::ScaleFull(const Vector3& v)
{
	ScaleFull(v.x,v.y,v.z);
}
#endif // MATRIX34_SCALEFULL_V

#ifndef MATRIX34_MAKESCALE
#define MATRIX34_MAKESCALE
inline void Matrix34::MakeScale(float s) 
{
	a.Set(s,0.0f,0.0f);
	b.Set(0.0f,s,0.0f);
	c.Set(0.0f,0.0f,s);
}
#endif // MATRIX34_MAKESCALE

#ifndef MATRIX34_MAKESCALE_F
#define MATRIX34_MAKESCALE_F
inline void Matrix34::MakeScale(float x,float y,float z) 
{
	a.Set(x,0.0f,0.0f);
	b.Set(0.0f,y,0.0f);
	c.Set(0.0f,0.0f,z);
}
#endif // MATRIX34_MAKESCALE_F

#ifndef MATRIX34_MAKESCALE_V
#define MATRIX34_MAKESCALE_V
inline void Matrix34::MakeScale(const Vector3& v)
{
	MakeScale(v.x,v.y,v.z);
}
#endif // MATRIX34_MAKESCALE_V

#ifndef MATRIX34_TRANSLATE
#define MATRIX34_TRANSLATE
inline void Matrix34::Translate(const Vector3 &v)
{
	d.Add(v);
}
#endif

#ifndef MATRIX34_TRANSLATE_F
#define MATRIX34_TRANSLATE_F
inline void Matrix34::Translate(float x,float y,float z)
{
	d.x+=x; d.y+=y; d.z+=z;
}
#endif // MATRIX34_TRANSLATE_F

#ifndef MATRIX34_MAKETRANSLATE
#define MATRIX34_MAKETRANSLATE
inline void Matrix34::MakeTranslate(const Vector3 &v)
{
	d=v;
}
#endif // MATRIX34_MAKETRANSLATE

#ifndef MATRIX34_MAKETRANSLATE_F
#define MATRIX34_MAKETRANSLATE_F
inline void Matrix34::MakeTranslate(float x,float y,float z)
{
	d.Set(x,y,z);
}
#endif // MATRIX34_MAKETRANSLATE_F

#ifndef MATRIX34_INVERSE
#define MATRIX34_INVERSE
inline bool Matrix34::Inverse ()
{
	Matrix34 original(*this);
	return Inverse(original);
}
#endif // MATRIX34_INVERSE

#ifndef MATRIX34_INVERSE_M
#define MATRIX34_INVERSE_M
inline bool Matrix34::Inverse (const Matrix34& m)
{
	// Get three of the subdeterminants.
	float subDetX = Det22(m.b.y,m.b.z,m.c.y,m.c.z);
	float subDetY = Det22(m.b.x,m.b.z,m.c.x,m.c.z);
	float subDetZ = Det22(m.b.x,m.b.y,m.c.x,m.c.y);

	// Find the largest absolute value element.
	float bigElement = Max(Max(fabsf(m.a.x),fabsf(m.a.y),fabsf(m.a.z)),
							Max(fabsf(m.b.x),fabsf(m.b.y),fabsf(m.b.z)),
							Max(fabsf(m.c.x),fabsf(m.c.y),fabsf(m.c.z)));
	
	// Get the inverse of the determinant.
	float invDet = m.a.x*subDetX - m.a.y*subDetY + m.a.z*subDetZ;
	if (fabsf(invDet)>3.6e-7f*bigElement)
	{
		invDet = 1.0f/invDet;

		// Start making the inverse matrix.
		a.x = subDetX*invDet;
		b.x = -subDetY*invDet;
		c.x = subDetZ*invDet;
		d.x = - (m.d.x*a.x + m.d.y*b.x + m.d.z*c.x);

		// Get three more subdeterminants.
		subDetX = Det22(m.a.y,m.a.z,m.c.y,m.c.z);
		subDetY = Det22(m.a.x,m.a.z,m.c.x,m.c.z);
		subDetZ = Det22(m.a.x,m.a.y,m.c.x,m.c.y);

		// Add more terms to the inverse matrix.
		a.y = -subDetX*invDet;
		b.y = subDetY*invDet;
		c.y = -subDetZ*invDet;
		d.y = - (m.d.x*a.y + m.d.y*b.y + m.d.z*c.y);

		// Get the last three subdeterminants.
		subDetX = Det22(m.a.y,m.a.z,m.b.y,m.b.z);
		subDetY = Det22(m.a.x,m.a.z,m.b.x,m.b.z);
		subDetZ = Det22(m.a.x,m.a.y,m.b.x,m.b.y);

		// Finish making the inverse matrix.
		a.z = subDetX*invDet;
		b.z = -subDetY*invDet;
		c.z = subDetZ*invDet;
		d.z = - (m.d.x*a.z + m.d.y*b.z + m.d.z*c.z);

		// Return true for a successful inverse (the determinant was not too close to zero).
		return true;
	}

	// The determinant of this matrix is too close to zero to do an accurate inverse.
	return false;
}
#endif // MATRIX34_INVERSE_M

#ifndef MATRIX34_INVERSE3X3
#define MATRIX34_INVERSE3X3
inline bool Matrix34::Inverse3x3 ()
{
	Matrix34 original;
	original.Set3x3(*this);
	return Inverse3x3(original);
}
#endif // MATRIX34_INVERSE3X3

#ifndef MATRIX34_INVERSE3X3_M
#define MATRIX34_INVERSE3X3_M
inline bool Matrix34::Inverse3x3 (const Matrix34& m)
{
	// Get three of the subdeterminants.
	float subDetX = Det22(m.b.y,m.b.z,m.c.y,m.c.z);
	float subDetY = Det22(m.b.x,m.b.z,m.c.x,m.c.z);
	float subDetZ = Det22(m.b.x,m.b.y,m.c.x,m.c.y);

	// Find the largest absolute value element.
	float bigElement = Max(Max(fabsf(m.a.x),fabsf(m.a.y),fabsf(m.a.z)),
							Max(fabsf(m.b.x),fabsf(m.b.y),fabsf(m.b.z)),
							Max(fabsf(m.c.x),fabsf(m.c.y),fabsf(m.c.z)));
	
	// Get the inverse of the determinant.
	float invDet = m.a.x*subDetX - m.a.y*subDetY + m.a.z*subDetZ;
	// The value 3.6e-7f is the square of the nearlyZero factor from Matrix34::SolveSVD.
	if (fabsf(invDet)>3.6e-7f*bigElement)
	{
		invDet = 1.0f/invDet;

		// Start making the inverse matrix.
		a.x = subDetX*invDet;
		b.x = -subDetY*invDet;
		c.x = subDetZ*invDet;

		// Get three more subdeterminants.
		subDetX = Det22(m.a.y,m.a.z,m.c.y,m.c.z);
		subDetY = Det22(m.a.x,m.a.z,m.c.x,m.c.z);
		subDetZ = Det22(m.a.x,m.a.y,m.c.x,m.c.y);

		// Add more terms to the inverse matrix.
		a.y = -subDetX*invDet;
		b.y = subDetY*invDet;
		c.y = -subDetZ*invDet;

		// Get the last three subdeterminants.
		subDetX = Det22(m.a.y,m.a.z,m.b.y,m.b.z);
		subDetY = Det22(m.a.x,m.a.z,m.b.x,m.b.z);
		subDetZ = Det22(m.a.x,m.a.y,m.b.x,m.b.y);

		// Finish making the inverse matrix.
		a.z = subDetX*invDet;
		b.z = -subDetY*invDet;
		c.z = subDetZ*invDet;

		// Return true for a successful inverse (the determinant was not too close to zero).
		return true;
	}

	// The determinant of this matrix is too close to zero to do an accurate inverse.
	return false;
}
#endif // MATRIX34_INVERSE3X3_M

#ifndef MATRIX34_FASTINVERSE
#define MATRIX34_FASTINVERSE
inline void Matrix34::FastInverse() 
{
	Matrix34 mtx=*this;
	FastInverse(mtx);
}
#endif // MATRIX34_FASTINVERSE

#ifndef MATRIX34_FASTINVERSE_M
#define MATRIX34_FASTINVERSE_M
inline void Matrix34::FastInverse(const Matrix34 &m) 
{
	register float rx(m.d.x),ry(m.d.y),rz(m.d.z);
	register float r,rd;

	r=m.a.x; rd =r*rx; a.x = r; 
	r=m.a.y; rd+=r*ry; b.x = r; 
	r=m.a.z; rd+=r*rz; c.x = r;
					d.x = -rd;

	r=m.b.x; rd =r*rx; a.y = r; 
	r=m.b.y; rd+=r*ry; b.y = r; 
	r=m.b.z; rd+=r*rz; c.y = r; 
					d.y = -rd;

	r=m.c.x; rd =r*rx; a.z = r; 
	r=m.c.y; rd+=r*ry; b.z = r; 
	r=m.c.z; rd+=r*rz; c.z = r;
					d.z = -rd;
}
#endif // MATRIX34_FASTINVERSE_M

#ifndef MATRIX34_FASTINVERSESCALED
#define MATRIX34_FASTINVERSESCALED
inline void Matrix34::FastInverseScaled(const Matrix34 &m) 
{
	register float rx(m.d.x),ry(m.d.y),rz(m.d.z);
	register float r,rd;
	register float invScale = m.a.InvMag();
	register float invScale2 = invScale*invScale;

	r=m.a.x*invScale2; rd =r*rx; a.x = r; 
	r=m.a.y*invScale2; rd+=r*ry; b.x = r; 
	r=m.a.z*invScale2; rd+=r*rz; c.x = r;
								 d.x = -rd;

	r=m.b.x*invScale2; rd =r*rx; a.y = r; 
	r=m.b.y*invScale2; rd+=r*ry; b.y = r; 
	r=m.b.z*invScale2; rd+=r*rz; c.y = r; 
								 d.y = -rd;

	r=m.c.x*invScale2; rd =r*rx; a.z = r; 
	r=m.c.y*invScale2; rd+=r*ry; b.z = r; 
	r=m.c.z*invScale2; rd+=r*rz; c.z = r;
								 d.z = -rd;
}
#endif // MATRIX34_FASTINVERSESCALED

#ifndef MATRIX34_TRANSPOSE
#define MATRIX34_TRANSPOSE
inline void Matrix34::Transpose() 
{
	float t;
	t=b.x; b.x=a.y; a.y=t;
	t=c.x; c.x=a.z; a.z=t;
	t=c.y; c.y=b.z; b.z=t;
}
#endif // MATRIX34_TRANSPOSE

#ifndef MATRIX34_TRANSPOSE_M
#define MATRIX34_TRANSPOSE_M
inline void Matrix34::Transpose(const Matrix34 &m) 
{
	a.Set(m.a.x,m.b.x,m.c.x);
	b.Set(m.a.y,m.b.y,m.c.y);
	c.Set(m.a.z,m.b.z,m.c.z);
}
#endif // MATRIX34_TRANSPOSE_M

#ifndef MATRIX34_TRANSPOSE3X4
#define MATRIX34_TRANSPOSE3X4
inline void Matrix34::Transpose3x4()
{
	Transpose();
	d.Dot3x3(*this);
	d.Negate();
}
#endif // MATRIX34_TRANSPOSE3X4

#ifndef MATRIX34_TRANSPOSE3X4_M
#define MATRIX34_TRANSPOSE3X4_M
inline void Matrix34::Transpose3x4(const Matrix34 &m)
{
	a.Set(m.a.x,m.b.x,m.c.x);
	b.Set(m.a.y,m.b.y,m.c.y);
	c.Set(m.a.z,m.b.z,m.c.z);
	d.Dot3x3(m.d,*this);
	d.Negate();
}
#endif // MATRIX34_TRANSPOSE3X4_M

#ifndef MATRIX34_COORDINATEINVERSESAFE
#define MATRIX34_COORDINATEINVERSESAFE
inline void Matrix34::CoordinateInverseSafe(float error)
{
	if (IsOrthonormal(error))
	{
		Transpose3x4();
	}
	else
	{
		NormalizeSafe();
		Transpose3x4();
	}
}
#endif // MATRIX34_COORDINATEINVERSESAFE

#ifndef MATRIX34_COORDINATEINVERSESAFE_M
#define MATRIX34_COORDINATEINVERSESAFE_M
inline void Matrix34::CoordinateInverseSafe(const Matrix34& m, float error)
{
	if (m.IsOrthonormal(error))
	{
		Transpose3x4(m);
	}
	else
	{
		Matrix34 normalizedM(m);
		normalizedM.NormalizeSafe();
		Transpose3x4(normalizedM);
	}
}
#endif // MATRIX34_COORDINATEINVERSESAFE_M

#ifndef MATRIX34_LOOKDOWN
#define MATRIX34_LOOKDOWN
inline void Matrix34::LookDown(const Vector3 &dir, const Vector3& up =YAXIS)
{
	// c = -dir;		// replaced this by the following line
	c = dir;
	c.Normalize();
	//a.Cross(c, up);	// replaced this by the following line
	a.Cross(up, c);
	a.Normalize();
	// b.Cross(a, c);	// replaced this by the following line
	b.Cross(c, a);
	// a.Negate();		// replaced this completely
}
#endif // MATRIX34_LOOKDOWN

#ifndef MATRIX34_LOOKAT
#define MATRIX34_LOOKAT
inline void Matrix34::LookAt(const Vector3 &to, const Vector3& up = YAXIS)
{
	//
	// we use exclusively a right-handed coordinate system on all platforms 
	// so we need to have a right-handed LookAt function as well
	// 
	// 
	// D3D documentation for D3DXMatrixLookAtRH
	//
	//	zaxis = normal(Eye - At)
	//	xaxis = normal(cross(Up, zaxis))
	//	yaxis = cross(zaxis, xaxis)
	//	
	//	 xaxis.x           yaxis.x           zaxis.x          0
	//	 xaxis.y           yaxis.y           zaxis.y          0
	//	 xaxis.z           yaxis.z           zaxis.z          0
	//	-dot(xaxis, eye)  -dot(yaxis, eye)  -dot(zaxis, eye)  l
	//
	// in RAGE lingo
	//	 a.x           		b.x           		c.x          		0
	//	 a.y           		b.y           		c.y          		0
	//	 a.z           		b.z           		c.z          		0
	//	-dot(a, eye)		-dot(b, eye)		-dot(c, eye)		l

	Vector3 dir;

	// this is the left-handed version
	// At - Eye
	//dir.Subtract(to,d);

	// this is the right-handed version
	// Eye - At
	dir.Subtract(d, to);

	LookDown(dir, up);
}
#endif // MATRIX34_LOOKAT

#ifndef MATRIX34_LOOKAT_V
#define MATRIX34_LOOKAT_V
inline void Matrix34::LookAt(const Vector3 &from, const Vector3 &to, const Vector3& up)
{
	d=from;
	LookAt(to, up);
}
#endif // MATRIX34_LOOKAT_V

#ifndef MATRIX34_GETLOOKAT
#define MATRIX34_GETLOOKAT
inline void Matrix34::GetLookAt(Vector3 *LookFrom, Vector3 *LookTo, float Dist) const
{
	LookFrom->Set(d);
	LookTo->SubtractScaled(d,c,Dist);	// translate 1 unit in the local -Z direction
}
#endif // MATRIX34_GETLOOKAT

#ifndef MATRIX34_CALCULATEAZIMUTH
#define MATRIX34_CALCULATEAZIMUTH
inline float Matrix34::CalculateAzimuth () const
{
	float azimuth = rage::Acosf(c.z);
	if (c.x<0.0f)
	{
		azimuth = -azimuth;
	}
	return azimuth;
}
#endif // MATRIX34_CALCULATEAZIMUTH

#ifndef MATRIX34_POLARVIEW_F
#define MATRIX34_POLARVIEW_F
inline void Matrix34::PolarView(float dist,float azm,float inc,float twst) 
{
	Vector3 v(-inc,azm,twst);
	FromEulersZXY(v);
	d.Scale(c,dist);
}
#endif // MATRIX34_POLARVIEW_F

#ifndef MATRIX34_POLARVIEW
#define MATRIX34_POLARVIEW
inline void Matrix34::PolarView(const Vector4 &P)
{
	PolarView(P.x,P.y,P.z,P.w);
}
#endif // MATRIX34_POLARVIEW

#ifndef MATRIX34_GETPOLAR_V4
#define MATRIX34_GETPOLAR_V4
inline void Matrix34::GetPolar(Vector4 &PolarCoords, Vector3 &PolarOffset, float Dist) const 
{
	Vector3 LLookFrom, LLookTo, Eulers;	
	GetLookAt(&LLookFrom, &LLookTo, Dist);
	ToEulersZXY(Eulers);
	PolarCoords.Set(Dist, Eulers.y, -Eulers.x, Eulers.z);
	PolarOffset.Set(LLookTo);
}
#endif // MATRIX34_GETPOLAR_V4

#ifndef MATRIX34_GETPOLAR
#define MATRIX34_GETPOLAR
inline void Matrix34::GetPolar(Vector3& outPolarCoords, Vector3& outPolarOffset, float Dist) const 
{
	Vector3 LLookFrom, LLookTo;	
	GetLookAt(&LLookFrom, &LLookTo, Dist);
	LLookFrom.GetPolar(LLookTo, outPolarCoords, outPolarOffset);	
}
#endif // MATRIX34_GETPOLAR

#ifndef MATRIX34_DOT3X3CROSSPRODMTX
#define MATRIX34_DOT3X3CROSSPRODMTX
inline void Matrix34::Dot3x3CrossProdMtx( const Vector3 & v) 	// Dot3x3(A.CrossProduct(v)).
{
	a.Cross(v);
	b.Cross(v);
	c.Cross(v);
}
#endif // MATRIX34_DOT3X3CROSSPRODMTX

#ifndef MATRIX34_DOT3X3CROSSPRODTRANSPOSE
#define MATRIX34_DOT3X3CROSSPRODTRANSPOSE
inline void Matrix34::Dot3x3CrossProdTranspose( const Vector3 & v)
{
	a.CrossNegate(v);
	b.CrossNegate(v);
	c.CrossNegate(v);
}
#endif // MATRIX34_DOT3X3CROSSPRODTRANSPOSE

#ifndef MATRIX34_MAKEDOUBLECROSSMATRIX
#define MATRIX34_MAKEDOUBLECROSSMATRIX
inline void Matrix34::MakeDoubleCrossMatrix (const Vector3& vector)
{
	// Make the diagonal elements.
	a.x = square(vector.y);
	b.y = square(vector.x);
	c.z = a.x + b.y;
	float tempFloat = square(vector.z);
	a.x += tempFloat;
	b.y += tempFloat;

	// Make the off-diagonal elements.
	a.y = -vector.x*vector.y;
	a.z = -vector.x*vector.z;
	b.x = a.y;
	b.z = -vector.y*vector.z;
	c.x = a.z;
	c.y = b.z;
}
#endif // MATRIX34_MAKEDOUBLECROSSMATRIX

#ifndef MATRIX34_MAKEDOUBLECROSSMATRIX_V
#define MATRIX34_MAKEDOUBLECROSSMATRIX_V
inline void Matrix34::MakeDoubleCrossMatrix (const Vector3& vectorA, const Vector3& vectorB)
{
	// Make the diagonal elements.
	a.x = vectorA.y*vectorB.y;
	b.y = vectorA.x*vectorB.x;
	c.z = a.x+b.y;
	float tempFloat = vectorA.z*vectorB.z;
	a.x += tempFloat;
	b.y += tempFloat;

	// Make the off-diagonal elements.
	a.y = -vectorA.x*vectorB.y;
	a.z = -vectorA.x*vectorB.z;
	b.x = -vectorA.y*vectorB.x;
	b.z = -vectorA.y*vectorB.z;
	c.x = -vectorA.z*vectorB.x;
	c.y = -vectorA.z*vectorB.y;
}
#endif // MATRIX34_MAKEDOUBLECROSSMATRIX_V

#ifndef MATRIX34_NORMALIZE
#define MATRIX34_NORMALIZE
inline void Matrix34::Normalize() 
{
	c.Normalize();
	a.Cross(b,c);
	a.Normalize();
	b.Cross(c,a);
}
#endif // MATRIX34_NORMALIZE

#ifndef MATRIX34_NORMALIZESAFE
#define MATRIX34_NORMALIZESAFE
inline void Matrix34::NormalizeSafe() 
{
	// Normalize the c vector, using ZAXIS if it has nearly zero length.
	c.NormalizeSafe(ZAXIS);

	// Make the a vector equal to b cross c.
	a.Cross(b,c);

	// Normalize the a vector, using YAXIS cross c if it has nearly zero length (if b and c are parallel).
	f32 magSq = a.Mag2();
	const float mag2Limit = 1.0e-5f;
	if (magSq>mag2Limit)
	{
		// The a vector is not too short, so normalize it (the minimum length is the default error in Vector3::NormalizeSafe).
		a.Scale(invsqrtf(magSq));
	}
	else
	{
		// The b and c vectors are nearly parallel, so make a default a vector.
		Vector3 defaultA(YAXIS);
		defaultA.Cross(c);
		magSq = defaultA.Mag2();
		if (magSq>mag2Limit)
		{
			// The c vector is not nearly parallel to YAXIS, so normalize the default a vector.
			defaultA.Scale(invsqrtf(magSq));
		}
		else
		{
			// The c vector is nearly parallel to YAXIS, so choose another default for the a vector.
			defaultA.Cross(b,ZAXIS);
			defaultA.NormalizeSafe(XAXIS);
		}

		// Set the a vector to the default value, because the b and c vectors are nearly parallel.
		a.Set(defaultA);
	}

	// Make b equal to c cross a, which will have unit length.
	b.Cross(c,a);
}
#endif // MATRIX34_NORMALIZESAFE

#ifndef MATRIX34_DETERMINANT3X3
#define MATRIX34_DETERMINANT3X3
inline float Matrix34::Determinant3x3() const 
{
	return Det33(a.x,a.y,a.z, b.x,b.y,b.z, c.x,c.y,c.z); 
}
#endif // MATRIX34_DETERMINANT3X3

#ifndef MATRIX34_SOLVESVD
#define MATRIX34_SOLVESVD

#define M34_BAD_INDEX	-1
#define M34_SMALL_FLOAT 1.0e-8f
inline bool Matrix34::SolveSVD (const Vector3& in, Vector3& out) const
{
	// Find maximum absolute value element in this matrix and the smallest usable absolute value for
	// any element to be considered non-zero.
	float nearlyZero = 0.0f;   
	int rowMax = M34_BAD_INDEX;		// the row with the largest absolute value element
	int colMax = M34_BAD_INDEX;		// the column with the largest absolute value element

	float absElement;
	int rowIndex,colIndex;
	for (rowIndex=0; rowIndex<3; rowIndex++)
	{
		const Vector3& rowVector = GetVector(rowIndex);
		for (colIndex=0; colIndex<3; colIndex++)
		{
			absElement = fabsf(rowVector[colIndex]);
			if (nearlyZero<absElement)
			{
				nearlyZero = absElement;
				rowMax = rowIndex;
				colMax = colIndex;
			}
		}
	}

	if (nearlyZero<M34_SMALL_FLOAT || rowMax==M34_BAD_INDEX || colMax==M34_BAD_INDEX)
	{
		// This matrix is all nearly zeros, so there is no answer unless the input is all nearly zeros,
		// in which case any answer will work, so choose all zeros for the answer and return true iff
		// the input is all zeros.
		out.Zero();
		return in.IsClose(ORIGIN,M34_SMALL_FLOAT);
	}

	// Make nearlyZero small, but no smaller than the smallest number that can be cubed without precision errors.
	// This makes the smallest possible nearlyZero 6.0e-12f, which cubed is 2.16e-34f.
	nearlyZero *= 6.0e-4f;

	// Find all the 2x2 subdeterminants.
	Matrix34 subDet;
	subDet.a.x = Det22(b.y,b.z,c.y,c.z);
	subDet.a.y = -Det22(b.x,b.z,c.x,c.z);
	subDet.a.z = Det22(b.x,b.y,c.x,c.y);
	subDet.b.x = -Det22(a.y,a.z,c.y,c.z);
	subDet.b.y = Det22(a.x,a.z,c.x,c.z );
	subDet.b.z = -Det22(a.x,a.y,c.x,c.y);
	subDet.c.x = Det22(a.y,a.z,b.y,b.z);
	subDet.c.y = -Det22(a.x,a.z,b.x,b.z);
	subDet.c.z = Det22(a.x,a.y,b.x,b.y);

	// Find the maximum absolute value subdeterminant in each row, and the column of each one,
	// and find the maximum subdeterminant and its row.
	Vector3 maxSubDetInRow(ORIGIN);
	float maxSubDet = 0.0f;
	int maxSubDetRow = M34_BAD_INDEX;
	int colMaxSubDet[] = { M34_BAD_INDEX, M34_BAD_INDEX, M34_BAD_INDEX };
	for (rowIndex=0; rowIndex<3; rowIndex++)
	{
		const Vector3& rowDet = subDet.GetVector(rowIndex);
		for (colIndex=0; colIndex<3; colIndex++)
		{
			absElement = fabsf(rowDet[colIndex]);
			if (maxSubDetInRow[rowIndex]<absElement)
			{
				maxSubDetInRow[rowIndex] = absElement;
				colMaxSubDet[rowIndex] = colIndex;
			}
		}

		if (maxSubDet<maxSubDetInRow[rowIndex])
		{
			maxSubDet = maxSubDetInRow[rowIndex];
			maxSubDetRow = rowIndex;
		}
	}

	// Find the 3x3 determinant.
	float det3x3 = a.x*subDet.a.x + a.y*subDet.a.y + a.z*subDet.a.z;

	// Find tolerance levels for squares and cubes of elements to be near zero.
	float nearlyZero2 = square(nearlyZero);
	float nearlyZero3 = nearlyZero * nearlyZero2;

	if (fabsf(det3x3)>Max(nearlyZero3,maxSubDet*nearlyZero))
	{
		// The absolute value of the 3x3 determinant is not nearly zero,
		// and it is not too small compared to the largest 2x2 subdeterminant.
		if (maxSubDetInRow[(rowMax+1)%3]>nearlyZero2 && maxSubDetInRow[(rowMax+2)%3]>nearlyZero2)
		{
			// The maximum subdeterminants in the two rows other than the row with the overall maximum subdeterminant
			// are not nearly zero, so this matrix has three linearly independent column vectors (it has full rank),
			// and its inverse is the transpose of its matrix of 2x2 subdeterminants scaled by the inverse 3x3 determinant.
			subDet.UnTransform3x3(in,out);
			out.InvScale(det3x3);
			return true;
		}
	}

	// Copy the row with the maximum 2x2 subdeterminant.
	Vector3 maxSubDetRowVector(GetVector(rowMax));

	// See if the maximum 2x2 subdeterminant is nearly zero.
	if (maxSubDet>nearlyZero2 )
	{
		// The maximum 2x2 subdeterminant is not nearly zero, and the above conditions weren't satisfied,
		// so this matrix has exactly two linearly independent vectors within the precision limits.
		// Get the column of the maximum 2x2 subdeterminant.
		FastAssert(maxSubDetRow!=M34_BAD_INDEX);
		int maxSubDetCol = colMaxSubDet[maxSubDetRow];

		// Copy the row of the 3x3 matrix of 2x2 subdeterminants that contains the element with the maximum 2x2 subdeterminant.
		Vector3 ortho(subDet.GetVector(maxSubDetRow));

		// Copy the two row vectors of this matrix that make the maximum subdeterminant.
		Vector3 maxSubDetRow0(GetVector((maxSubDetRow+1)%3));
		Vector3 maxSubDetRow1(GetVector((maxSubDetRow+2)%3));

		// Project the input vector onto the span of the two rows.
		Vector3 inProjection(in);
		inProjection.SubtractScaled(ortho,in.Dot(ortho)/ortho.Mag2());

		// Find the two columns that make the maximum subdeterminant.
		int colIndex0 = (maxSubDetCol+1)%3;
		int colIndex1 = (maxSubDetCol+2)%3;
		Vector3 maxSubDetCol0(a[colIndex0],b[colIndex0],c[colIndex0]);
		Vector3 maxSubDetCol1(a[colIndex1],b[colIndex1],c[colIndex1]);
		float u0 = inProjection[colIndex0];
		float u1 = inProjection[colIndex1];
		float dA0 = maxSubDetRow0[colIndex0];
		float dA1 = maxSubDetRow0[colIndex1];
		float dB0 = maxSubDetRow1[colIndex0];
		float dB1 = maxSubDetRow1[colIndex1];

		// Solve for the unknown as a linear combination of the two columns.
		out[maxSubDetRow] = 0.0f;
		float invDet22 = 1.0f/Det22(dA0,dA1,dB0,dB1);
		out[(maxSubDetRow+1)%3] = Det22(u0,u1,dB0,dB1)*invDet22;
		out[(maxSubDetRow+2)%3] = -Det22(u0,u1,dA0,dA1)*invDet22;
		ortho.Cross(maxSubDetCol0,maxSubDetCol1);
		out.SubtractScaled(ortho,out.Dot(ortho)/ortho.Mag2());

		// Find the two linearly dependent columns.
		int ldCol0 = maxSubDetCol;
		int otherCol0 = (ldCol0+1)%3;
		int otherCol1 = (ldCol0+2)%3;
		int ldCol1 = (fabsf(subDet.a[otherCol0])+fabsf(subDet.b[otherCol0])+fabsf(subDet.c[otherCol0])<
						fabsf(subDet.a[otherCol1])+fabsf(subDet.b[otherCol1])+fabsf(subDet.c[otherCol1]))
						? otherCol1 : otherCol0;

		// Find the 2x2 subdeterminant of the two elements in the linearly dependent columns in one of the rows,
		// and the corresponding two elements in the input vector. If this is nearly zero, then the output vector
		// is a nearly exact solution. Otherwise it is the best, but wrong, solution.
		int subRowIndex = 2;
		float absA = fabsf(a[ldCol0]);
		float absB = fabsf(b[ldCol0]);
		float absC = fabsf(c[ldCol0]);
		if (absA>absB)
		{
			if (absA>absC)
			{
				subRowIndex = 0;
			}
		}
		else
		{
			if (absB>absC)
			{
				subRowIndex = 1;
			}
		}
		const Vector3& rowVector = GetVector(subRowIndex);
		float det01 = rowVector[ldCol0]*in[ldCol1];
		float smallDet01 = fabsf(det01);
		u1 = rowVector[ldCol1]*in[ldCol0];
		smallDet01 = Max(smallDet01,fabsf(u1));
		det01 -= u1;
		smallDet01 = nearlyZero*Max(1.0f,smallDet01);

		// Return true if this is a good solution (if the overdefined set of linear equations has a solution).
		// Return false if this is an approximate solution (the overdefined set of linear equations has no solution).
		return (fabsf(det01)<=smallDet01);
	}

	// This matrix has only one linearly independent vector within the precision limits.
	// Project the input vector onto the row with the maximum subdeterminant.
	Vector3 inProjection(maxSubDetRowVector);
	inProjection.Scale(in.Dot(maxSubDetRowVector)/maxSubDetRowVector.Mag2());
	Vector3 maxSubDetCol(GetVector(0)[colMax],GetVector(1)[colMax],GetVector(2)[colMax]);
	float u = inProjection[colMax];
	out.Scale(maxSubDetCol,u/maxSubDetCol.Mag2());

	// Find the 2x2 subdeterminants of any two elements (x,y and x,z) in the row with the maximum 2x2 subdeterminant,
	// and the corresponding two elements in the input vector. If these are nearly zero (and the same with y,z must be
	// also), then the output vector is a nearly exact solution. Otherwise it is the best, but wrong, solution.
	float detXY = Det22(maxSubDetRowVector.x,maxSubDetRowVector.y,in.x,in.y);
	float detXZ = Det22(maxSubDetRowVector.x,maxSubDetRowVector.z,in.x,in.z);

	// Return true if this is a good solution (if the overdefined set of linear equations has a solution).
	// Return false if this is an approximate solution (the overdefined set of linear equations has no solution).
	return (fabsf(detXY)<=nearlyZero2 && fabsf(detXZ)<=nearlyZero2);
}
#endif // MATRIX34_SOLVESVD

#ifndef MATRIX34_SOLVESVD_V
#define MATRIX34_SOLVESVD_V
inline Vector3 Matrix34::SolveSVD(const Vector3& in) const
{ 
	Vector3 out; 
	SolveSVD(in,out); 
	return out; 
}
#endif // MATRIX34_SOLVESVD_V

#ifndef MATRIX34_SOLVESVDCONDITION
#define MATRIX34_SOLVESVDCONDITION
inline bool Matrix34::SolveSVDCondition (const Vector3& in, Vector3& out) const
{
	Matrix34 modifiedThis(*this);
	Vector3 modifiedIn(in);
	Vector3 absMag(Max(fabsf(a.x),fabsf(b.x),fabsf(c.x)),
					Max(fabsf(a.y),fabsf(b.y),fabsf(c.y)),
					Max(fabsf(a.z),fabsf(b.z),fabsf(c.z)));
	const float maxRatio = 2.0f;
	int maxAxis = absMag.x>absMag.y ? (absMag.x>absMag.z ? 0 : 2) : (absMag.y>absMag.z ? 1 : 2);
	int minAxis = (maxAxis+1)%3;
	if (absMag[minAxis]>0.0f)
	{
		float ratio = absMag[maxAxis]/absMag[minAxis];
		if (ratio>maxRatio)
		{
			// The components along minAxis of this matrix's vectors and the input vector are too small compared
			// with the components along maxAxis, so scale them up (the output vector will be the same).
			modifiedThis.a[minAxis] *= ratio;
			modifiedThis.b[minAxis] *= ratio;
			modifiedThis.c[minAxis] *= ratio;
			modifiedIn[minAxis] *= ratio;
		}
	}

	minAxis = (maxAxis+2)%3;
	if (absMag[minAxis]>0.0f)
	{
		float ratio = absMag[maxAxis]/absMag[minAxis];
		if (ratio>maxRatio)
		{
			// The components along minAxis of this matrix's vectors and the input vector are too small compared
			// with the components along maxAxis, so scale them up (the output vector will be the same).
			modifiedThis.a[minAxis] *= ratio;
			modifiedThis.b[minAxis] *= ratio;
			modifiedThis.c[minAxis] *= ratio;
			modifiedIn[minAxis] *= ratio;
		}
	}

	return modifiedThis.SolveSVD(modifiedIn,out);
}
#endif // MATRIX34_SOLVESVDCONDITION

#ifndef MATRIX34_4V4
#define MATRIX34_4V4
__forceinline Matrix34::Matrix34( Vector4::Vector4In _a, Vector4::Vector4In _b, Vector4::Vector4In _c, Vector4::Vector4In _d )
{
	a = _a;
	b = _b;
	c = _c;
	d = _d;
}
#endif // MATRIX34_4V4

#ifndef MATRIX34_MATRIX34
#define MATRIX34_MATRIX34
__forceinline Matrix34::Matrix34(const Matrix34 &m)
{
	Set(m);
}
#endif // MATRIX34_MATRIX34

#ifndef MATRIX34_OPERATOREQUAL
#define MATRIX34_OPERATOREQUAL
__forceinline Matrix34& Matrix34::operator=(const Matrix34 &m)
{
	Set(m);
	return *this;
}
#endif // MATRIX34_OPERATOREQUAL

#ifndef MATRIX34_GETELEMENT
#define MATRIX34_GETELEMENT
inline float & Matrix34::GetElement(int i, int j)
{
	FastAssert(i>=0 && i<=3 && j>=0 && j<=2);
	return ((float*)this)[i * VEC3_NUM_STORED_FLOATS + j];
}
#endif // MATRIX34_GETELEMENT

#ifndef MATRIX34_GETELEMENT_CONST
#define MATRIX34_GETELEMENT_CONST
inline const float & Matrix34::GetElement(int i, int j) const
{
	FastAssert(i>=0 && i<=3 && j>=0 && j<=2);
	return ((const float*)this)[i * VEC3_NUM_STORED_FLOATS + j];
}
#endif // MATRIX34_GETELEMENT_CONST

#ifndef MATRIX34_GETVECTOR
#define MATRIX34_GETVECTOR
inline Vector3 & Matrix34::GetVector(int i)
{
	FastAssert(i>=0 && i<=3);
	return ((Vector3 *)this)[i];
}
#endif // MATRIX34_GETVECTOR

#ifndef MATRIX34_GETVECTOR_CONST
#define MATRIX34_GETVECTOR_CONST
inline const Vector3 & Matrix34::GetVector(int i) const
{
	FastAssert(i>=0 && i<=3);
	return ((const Vector3 *)this)[i];
} 
#endif // MATRIX34_GETVECTOR_COSNT










// Vector3 functions.  should these really be here?
#ifndef MATRIX34_GDOT
#define MATRIX34_GDOT
inline Vector3 Dot(const Vector3& v, const Matrix34& mtx) 
{
	Vector3 temp;
	mtx.Transform(v,temp);
	return temp;
}
#endif // MATRIX34_GDOT

#ifndef VECTOR3_DOT_M
#define VECTOR3_DOT_M
inline void Vector3::Dot(const Matrix34& mtx)
{
	mtx.Transform( *this );
}
#endif // VECTOR3_DOT_M

#ifndef VECTOR3_DOT_VM
#define VECTOR3_DOT_VM
inline void Vector3::Dot(const Vector3& v,const Matrix34& mtx)
{
	mtx.Transform( v, *this );
}
#endif // VECTOR3_DOT_VM

#ifndef VECTOR3_DOT3X3_V
#define VECTOR3_DOT3X3_V
inline void Vector3::Dot3x3(const Vector3& v,const Matrix34& mtx)
{
	mtx.Transform3x3( v, *this );
}
#endif // VECTOR3_DOT3X3_V

#ifndef VECTOR3_DOT3X3
#define VECTOR3_DOT3X3
inline void Vector3::Dot3x3(const Matrix34& mtx)
{
	mtx.Transform3x3( *this );
}
#endif // VECTOR3_DOT3X3

#ifndef VECTOR3_DOT3X3TRANSPOSE
#define VECTOR3_DOT3X3TRANSPOSE
inline void Vector3::Dot3x3Transpose(const Matrix34& mtx)
{
	mtx.UnTransform3x3( *this );
}
#endif // VECTOR3_DOT3X3TRANSPOSE


} // namespace rage

#endif // VECTOR_MATRIX34_DEFAULT_H
