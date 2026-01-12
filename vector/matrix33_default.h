// 
// vector/MATRIX33_default.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef VECTOR_MATRIX33_DEFAULT_H
#define VECTOR_MATRIX33_DEFAULT_H

#include "math/amath.h"

// Default Matrix33 Implementations

namespace rage
{

inline float Det2233(float a,float b,float c,float d) {return a*d-b*c;}

inline float Det3333(float ax,float ay,float az,float bx,float by,float bz,float cx,float cy,float cz) 
{
	return ax*by*cz+ay*bz*cx+az*bx*cy-ax*bz*cy-ay*bx*cz-az*by*cx;
}

	//=============================================================================
	// Implementations

#ifndef MATRIX33_IDENTITY
#define MATRIX33_IDENTITY
inline void Matrix33::Identity()
{
	a.Set(1.0f,0.0f,0.0f);
	b.Set(0.0f,1.0f,0.0f);
	c.Set(0.0f,0.0f,1.0f);
}
#endif	// MATRIX33_IDENTITY

#ifndef MATRIX33_ZERO
#define MATRIX33_ZERO
inline void Matrix33::Zero()
{
	a.Zero();
	b.Zero();
	c.Zero();
}
#endif // MATRIX33_ZERO

#ifndef MATRIX33_SET_M
#define MATRIX33_SET_M
inline void Matrix33::Set(const Matrix33 &m)
{
	a.Set(m.a);
	b.Set(m.b);
	c.Set(m.c);
}
#endif // MATRIX33_SET


#ifndef MATRIX33_SET34
#define MATRIX33_SET34
inline void Matrix33::Set(const Matrix34 &m)
{
	a = m.a;
	b = m.b;
	c = m.c;
}
#endif // MATRIX33_SET



#ifndef MATRIX33_SET_V
#define MATRIX33_SET_V
inline void Matrix33::Set(const Vector3& newA, const Vector3& newB, const Vector3& newC)
{
	a.Set(newA);
	b.Set(newB);
	c.Set(newC);
}
#endif // MATRIX33_SET_V

#ifndef MATRIX33_SET_F
#define MATRIX33_SET_F
inline void Matrix33::Set(float ax, float ay, float az, float bx, float by, float bz,
						  float cx, float cy, float cz)
{
	a.Set(ax,ay,az);
	b.Set(bx,by,bz);
	c.Set(cx,cy,cz);
}
#endif // MATRIX33_SET_F

#ifndef MATRIX33_SET_DIAGONAL
#define MATRIX33_SET_DIAGONAL
VEC3_INLINE void Matrix33::SetDiagonal( Vector3::Vector3Param d )
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

#ifndef MATRIX33_ADD
#define MATRIX33_ADD
inline void Matrix33::Add(const Matrix33 &m) 
{
	a.Add(m.a);
	b.Add(m.b);
	c.Add(m.c);
}
#endif // MATRIX33_ADD

#ifndef MATRIX33_ADD_M
#define MATRIX33_ADD_M
inline void Matrix33::Add(const Matrix33 &m,const Matrix33 &n) 
{
	a.Add(m.a,n.a);
	b.Add(m.b,n.b);
	c.Add(m.c,n.c);
}
#endif // MATRIX33_ADD_M

#ifndef MATRIX33_SUBTRACT
#define MATRIX33_SUBTRACT
inline void Matrix33::Subtract(const Matrix33 &m) 
{
	a.Subtract(m.a);
	b.Subtract(m.b);
	c.Subtract(m.c);
}
#endif // MATRIX33_SUBTRACT

#ifndef MATRIX33_SUBTRACT_M
#define MATRIX33_SUBTRACT_M
inline void Matrix33::Subtract(const Matrix33 &m,const Matrix33 &n) 
{
	a.Subtract(m.a,n.a);
	b.Subtract(m.b,n.b);
	c.Subtract(m.c,n.c);
}
#endif // MATRIX33_SUBTRACT_M

#ifndef MATRIX33_ADDSCALED
#define MATRIX33_ADDSCALED
inline void Matrix33::AddScaled(const Matrix33 &m, float f )
{
	a.AddScaled(m.a,f);
	b.AddScaled(m.b,f);
	c.AddScaled(m.c,f);
}
#endif // MATRIX33_ADDSCALED

#ifndef MATRIX33_ABS
#define MATRIX33_ABS
inline void Matrix33::Abs() 
{
	a.Abs();
	b.Abs();
	c.Abs();
}
#endif // MATRIX33_ABS

#ifndef MATRIX33_NEGATE
#define MATRIX33_NEGATE
inline void Matrix33::Negate()
{
	a.Negate();
	b.Negate();
	c.Negate();
}
#endif // MATRIX33_NEGATE

#ifndef MATRIX33_DOT
#define MATRIX33_DOT
inline void Matrix33::Dot(const Matrix33 &m)
{
	FastAssert(this!=&m && "Don't use Dot with this as an argument.");	// lint !e506 constant value boolean

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
#endif // MATRIX33_DOT

#ifndef MATRIX33_DOT33
#define MATRIX33_DOT33
VEC3_INLINE void Matrix33::Dot3x3(const Matrix34 &m)
{
	Dot( (const Matrix33 &)(m) );
}
#endif

#ifndef MATRIX33_DOTFROMLEFT
#define MATRIX33_DOTFROMLEFT
inline void Matrix33::DotFromLeft(const Matrix33& m)
{
	FastAssert(this!=&m && "Don't use DotFromLeft with this as an argument.");	// lint !e506 constant value boolean

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
#endif // MATRIX33_DOTFROMLEFT

#ifndef MATRIX33_DOT_M
#define MATRIX33_DOT_M
inline void Matrix33::Dot(const Matrix33 &m,const Matrix33 &n)
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
}
#endif // MATRIX33_DOT_M

#ifndef MATRIX33_DOTTRANSPOSE
#define MATRIX33_DOTTRANSPOSE
inline void Matrix33::DotTranspose( const Matrix33 &n )
{
	FastAssert(this!=&n && "Don't use DotTranspose with this as an argument.");	// lint !e506 constant value boolean

	a.DotTranspose(n);
	b.DotTranspose(n);
	c.DotTranspose(n);
}
#endif // MATRIX33_DOTTRANSPOSE

#ifndef MATRIX33_DOTTRANSPOSE_M
#define MATRIX33_DOTTRANSPOSE_M
inline void Matrix33::DotTranspose( const Matrix33 &m, const Matrix33 &n )
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
}
#endif // MATRIX33_DOTTRANSPOSE_M

#ifndef MATRIX33_ISEQUAL
#define MATRIX33_ISEQUAL
inline bool Matrix33::IsEqual(const Matrix33& m) const
{
	return (a.IsEqual(m.a) && b.IsEqual(m.b) && c.IsEqual(m.c));
}
#endif // MATRIX33_ISEQUAL

#ifndef MATRIX33_ISNOTEQUAL
#define MATRIX33_ISNOTEQUAL
inline bool Matrix33::IsNotEqual(const Matrix33& m) const
{
	return (a.IsNotEqual(m.a) || b.IsNotEqual(m.b) || c.IsNotEqual(m.c));
}
#endif // MATRIX33_ISNOTEQUAL

#ifndef MATRIX33_ISCLOSE
#define MATRIX33_ISCLOSE
inline bool Matrix33::IsClose(const Matrix33& m, float error) const
{
	return (a.IsClose(m.a,error) && b.IsClose(m.b,error) && c.IsClose(m.c,error));
}
#endif // MATRIX33_ISCLOSE

#ifndef MATRIX33_ISORTHONORMAL
#define MATRIX33_ISORTHONORMAL
inline bool Matrix33::IsOrthonormal (float error) const
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
#endif // MATRIX33_ISORTHONORMAL

#ifndef MATRIX33_CROSSPRODUCT
#define MATRIX33_CROSSPRODUCT
inline void Matrix33::CrossProduct(Vector3::Param vr)
{
	Vector3 r(vr);
	a.Set(0.0f,-r.z,r.y);
	b.Set(r.z,0.0f,-r.x);
	c.Set(-r.y,r.x,0.0f);
}
#endif

#ifndef MATRIX33_OUTERPRODUCT
#define MATRIX33_OUTERPRODUCT
inline void Matrix33::OuterProduct(const Vector3& u, const Vector3& v)
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
#endif // MATRIX33_OUTERPRODUCT

#ifndef MATRIX33_TRANSFORMXZ
#define MATRIX33_TRANSFORMXZ
inline void Matrix33::TransformXZ(const Vector3 &in,Vector2 &out) const 
{
	out.x=in.x*a.x+in.y*b.x+in.z*c.x;
	out.y=in.x*a.z+in.y*b.z+in.z*c.z;
}
#endif // MATRIX33_TRANSFORMXZ

#ifndef MATRIX33_TRANSFORM_V
#define MATRIX33_TRANSFORM_V
inline void Matrix33::Transform(Vector3::Param vin,Vector3::Vector3Ref out) const 
{
	FastAssert(&vin != &out);
	Vector3 in(vin);

	float x=in.x*a.x+in.y*b.x+in.z*c.x;
	float y=in.x*a.y+in.y*b.y+in.z*c.y;
	float z=in.x*a.z+in.y*b.z+in.z*c.z;
	out = Vector3(x,y,z);
}
#endif // MATRIX33_TRANSFORM_V

#ifndef MATRIX33_TRANSFORM
#define MATRIX33_TRANSFORM
inline void Matrix33::Transform(Vector3::Vector3Ref vinAndOut) const 
{
	Vector3 inAndOut(vinAndOut);
	float newX=inAndOut.x*a.x+inAndOut.y*b.x+inAndOut.z*c.x;
	float newY=inAndOut.x*a.y+inAndOut.y*b.y+inAndOut.z*c.y;
	float newZ=inAndOut.x*a.z+inAndOut.y*b.z+inAndOut.z*c.z;
	vinAndOut = Vector3(newX, newY, newZ);
}
#endif // MATRIX33_TRANSFORM

#ifndef MATRIX33_TRANSFORM_V
#define MATRIX33_TRANSFORM_V
inline void Matrix33::Transform(const Vector3 &in,Vector3::Vector3Ref out) const 
{
	FastAssert(&in != &out);

	out.x=in.x*a.x+in.y*b.x+in.z*c.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y;
	out.z=in.x*a.z+in.y*b.z+in.z*c.z;
}
#endif // MATRIX33_TRANSFORM_V

#ifndef MATRIX33_TRANSFORM
#define MATRIX33_TRANSFORM
inline void Matrix33::Transform(Vector3::Vector3Ref inAndOut) const
{
	float newX=inAndOut.x*a.x+inAndOut.y*b.x+inAndOut.z*c.x;
	float newY=inAndOut.x*a.y+inAndOut.y*b.y+inAndOut.z*c.y;
	inAndOut.z=inAndOut.x*a.z+inAndOut.y*b.z+inAndOut.z*c.z;
	inAndOut.x=newX;
	inAndOut.y=newY;
}
#endif // MATRIX33_TRANSFORM

#ifndef MATRIX33_TRANSFORM_V2
#define MATRIX33_TRANSFORM_V2
inline void Matrix33::Transform(const Vector3 &in,Vector2 &out) const 
{
	out.x=in.x*a.x+in.y*b.x+in.z*c.x;
	out.y=in.x*a.y+in.y*b.y+in.z*c.y;
}
#endif // MATRIX33_TRANSFORM_V2

#ifndef MATRIX33_UNTRANSFORM_V
#define MATRIX33_UNTRANSFORM_V
inline void Matrix33::UnTransform(const Vector3 &in,Vector3 &out) const
{
	FastAssert(&in != &out);

	out.x = a.Dot(in);
	out.y = b.Dot(in);
	out.z = c.Dot(in);
}
#endif // MATRIX33_UNTRANSFORM_V

#ifndef MATRIX33_UNTRANSFORM
#define MATRIX33_UNTRANSFORM
inline void Matrix33::UnTransform(Vector3& inAndOut) const 
{
	float newX=a.Dot(inAndOut);
	float newY=b.Dot(inAndOut);
	inAndOut.z=c.Dot(inAndOut);
	inAndOut.x=newX;
	inAndOut.y=newY;
}
#endif // MATRIX33_UNTRANSFORM

#ifndef MATRIX33_UNTRANSFORM_V2
#define MATRIX33_UNTRANSFORM_V2
inline void Matrix33::UnTransform(Vector3::Param in,Vector2 &out) const
{
	out.x = a.Dot(in);
	out.y = b.Dot(in);
}
#endif // MATRIX33_UNTRANSFORM_V2

#ifndef MATRIX33_TRANSFORM4
#define MATRIX33_TRANSFORM4
inline void Matrix33::Transform4(const Vector3 *in, Vector4 *out, int count) const
{
	int i;
	FastAssert((count & 3) == 0);	// lint !e506 constant value boolean
	for (i = count-1; i >= 0; i--)
	{
		out[i].x = in[i].x*a.x + in[i].y*b.x + in[i].z*c.x;
		out[i].y = in[i].x*a.y + in[i].y*b.y + in[i].z*c.y;
		out[i].z = in[i].x*a.z + in[i].y*b.z + in[i].z*c.z;
		out[i].w = 0;
	}
}
#endif // MATRIX33_TRANSFORM4

#ifndef MATRIX33_GETLOCALUP
#define MATRIX33_GETLOCALUP
inline Vector3 Matrix33::GetLocalUp() const
{
	Vector3 localUp;
	Transform(g_UnitUp,localUp);
	return localUp;
}
#endif

#ifndef MATRIX33_ROTATEX
#define MATRIX33_ROTATEX
inline void Matrix33::RotateX(float t) 
{
	Matrix33 mtx;
	mtx.MakeRotateX(t);
	this->Dot(mtx);
}
#endif // MATRIX33_ROTATEX

#ifndef MATRIX33_ROTATEY
#define MATRIX33_ROTATEY
inline void Matrix33::RotateY(float t) 
{
	Matrix33 mtx;
	mtx.MakeRotateY(t);
	this->Dot(mtx);
}
#endif // MATRIX33_ROTATEY

#ifndef MATRIX33_ROTATEZ
#define MATRIX33_ROTATEZ
inline void Matrix33::RotateZ(float t) 
{
	Matrix33 mtx;
	mtx.MakeRotateZ(t);
	this->Dot(mtx);
}
#endif // MATRIX33_ROTATEZ

#ifndef MATRIX33_ROTATEUNITAXIS
#define MATRIX33_ROTATEUNITAXIS
inline void Matrix33::RotateUnitAxis(const Vector3 &va,float t) 
{
	mthAssertf(va.Mag2() >= square(0.999f) && va.Mag2() <= square(1.001f), "Vector3 <%f, %f, %f> does not have length 1",va.x,va.y,va.z);
	Matrix33 mtx;
	mtx.MakeRotateUnitAxis(va,t);
	this->Dot(mtx);
}
#endif // MATRIX33_ROTATEUNITAXIS

#ifndef MATRIX33_ROTATE
#define MATRIX33_ROTATE
inline void Matrix33::Rotate(const Vector3 &va,float t) 
{
	Matrix33 mtx;
	mtx.MakeRotate(va,t);
	this->Dot(mtx);
}
#endif // MATRIX33_ROTATE

#ifndef MATRIX33_ROTATELOCALX
#define MATRIX33_ROTATELOCALX
inline void Matrix33::RotateLocalX(float angle)
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
#endif // MATRIX33_ROTATELOCALX

#ifndef MATRIX33_ROTATELOCALY
#define MATRIX33_ROTATELOCALY
inline void Matrix33::RotateLocalY(float angle)
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
#endif // MATRIX33_ROTATELOCALY

#ifndef MATRIX33_ROTATELOCALZ
#define MATRIX33_ROTATELOCALZ
inline void Matrix33::RotateLocalZ(float angle)
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
#endif // MATRIX33_ROTATELOCALZ

#ifndef MATRIX33_ROTATELOCALAXIS
#define MATRIX33_ROTATELOCALAXIS
inline void Matrix33::RotateLocalAxis (float angle, int axisIndex)
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
			mthErrorf("Invalid axis index %d", axisIndex);
		}
	}
}
#endif // MATRIX33_ROTATELOCALAXIS

#ifndef MATRIX33_MAKEROTATE
#define MATRIX33_MAKEROTATE
inline void Matrix33::MakeRotate(const Vector3 &va,float t) 
{
	if(t==0.0f) {
		Identity();
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
#endif // MATRIX33_MAKEROTATE

#ifndef MATRIX33_MAKEROTATEX
#define MATRIX33_MAKEROTATEX
inline void Matrix33::MakeRotateX(float t) 
{
	float cost,sint;
	//float cost=rage::Cosf(t);
	//float sint=rage::Sinf(t);
	rage::cos_and_sin( cost,sint,t );

	a.Set(1.0f, 0.0f, 0.0f);
	b.Set(0.0f, cost, sint);
	c.Set(0.0f,-sint, cost);
}
#endif // MATRIX33_MAKEROTATEX

#ifndef MATRIX33_MAKEROTATEY
#define MATRIX33_MAKEROTATEY
inline void Matrix33::MakeRotateY(float t) 
{
	float cost,sint;
	//float cost=rage::Cosf(t);
	//float sint=rage::Sinf(t);
	rage::cos_and_sin( cost,sint,t );
	
	a.Set(cost, 0.0f,-sint);
	b.Set(0.0f, 1.0f, 0.0f);
	c.Set(sint, 0.0f, cost);
}
#endif // MATRIX33_MAKEROTATEY

#ifndef MATRIX33_MAKEROTATEZ
#define MATRIX33_MAKEROTATEZ
inline void Matrix33::MakeRotateZ(float t) 
{
	float cost,sint;
	//float cost=rage::Cosf(t);
	//float sint=rage::Sinf(t);
	rage::cos_and_sin( cost,sint,t );

	a.Set( cost, sint, 0.0f);
	b.Set(-sint, cost, 0.0f);
	c.Set( 0.0f, 0.0f, 1.0f);
}
#endif // MATRIX33_MAKEROTATEZ

#ifndef MATRIX33_MAKEROTATEUNITAXIS
#define MATRIX33_MAKEROTATEUNITAXIS
inline void Matrix33::MakeRotateUnitAxis(const Vector3 &v,float t) 
{
	mthAssertf(v.Mag2() >= square(0.999f) && v.Mag2() <= square(1.001f), "Vector3 <%f, %f, %f> does not have length 1",v.x,v.y,v.z);

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
#endif // MATRIX33_MAKEROTATEUNITAXIS


#ifndef MATRIX33_ROTATETO
#define MATRIX33_ROTATETO
inline void Matrix33::RotateTo (const Vector3& unitFrom, const Vector3& unitTo) 
{
	Vector3 axis;
	float angle;
	if (ComputeRotation(unitFrom,unitTo,axis,angle))
	{
		// Rotate the matrix.
		RotateUnitAxis(axis,angle);
	}
}
#endif // MATRIX33_ROTATETO

#ifndef MATRIX33_ROTATETO_F
#define MATRIX33_ROTATETO_F
inline void Matrix33::RotateTo (const Vector3& unitFrom, const Vector3& unitTo, float t)
{
	Vector3 axis;
	float angle;
	if (ComputeRotation(unitFrom,unitTo,axis,angle))
	{
		// Rotate the matrix.
		RotateUnitAxis(axis,angle*t);
	}
}
#endif // MATRIX33_ROTATETO_F

#ifndef MATRIX33_MAKEROTATETO
#define MATRIX33_MAKEROTATETO
inline void Matrix33::MakeRotateTo (const Vector3& unitFrom, const Vector3& unitTo) 
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
		Identity();
	}
}
#endif // MATRIX33_MAKEROTATETO

#ifndef MATRIX33_MAKEUPRIGHT
#define MATRIX33_MAKEUPRIGHT
inline void Matrix33::MakeUpright()
{
	// Find this matrix's up direction in world coordinates.
	Vector3 upDirection;
	Transform(g_UnitUp,upDirection);

	// Rotate this matrix so that it's up direction is the world up direction.
	RotateTo(upDirection,g_UnitUp);
}
#endif // MATRIX33_MAKEUPRIGHT

#ifndef MATRIX33_GETEULERS
#define MATRIX33_GETEULERS
inline Vector3 Matrix33::GetEulers() const // xyz order
{
	Matrix33 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
	return Vector3(rage::Atan2f(NormalizedMtx.b.z,NormalizedMtx.c.z),rage::Asinf(-NormalizedMtx.a.z),rage::Atan2f(NormalizedMtx.a.y,NormalizedMtx.a.x));
}
#endif // MATRIX33_GETEULERS

#ifndef MATRIX33_GETEULERSFAST
#define MATRIX33_GETEULERSFAST
inline Vector3 Matrix33::GetEulersFast() const // xyz order
{
	return Vector3(rage::Atan2f(b.z,c.z),rage::Asinf(-a.z),rage::Atan2f(a.y,a.x));
}
#endif // MATRIX33_GETEULERSFAST

#ifndef MATRIX33_GETEULERS_S
#define MATRIX33_GETEULERS_S
inline Vector3 Matrix33::GetEulers(const char *order) const 
{
	Matrix33 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
	if (order[2]=='x')
	{
		if (order[1] == 'y' && order[0] == 'z')	   // zyx order 
			return Vector3(rage::Atan2f(-NormalizedMtx.c.y,NormalizedMtx.c.z),rage::Asinf(NormalizedMtx.c.x),rage::Atan2f(-NormalizedMtx.b.x,NormalizedMtx.a.x));
		else if (order[1] == 'z' && order[0] == 'y')  // yzx order
			return Vector3(rage::Atan2f(NormalizedMtx.b.z,NormalizedMtx.b.y),rage::Atan2f(NormalizedMtx.c.x,NormalizedMtx.a.x),rage::Asinf(-NormalizedMtx.b.x));
	} 
	else if (order[2]=='y')
	{
		if (order[1]=='x' && order[0] == 'z')		 // zxy order
			return Vector3(rage::Asinf(-NormalizedMtx.c.y),rage::Atan2f(NormalizedMtx.c.x,NormalizedMtx.c.z),rage::Atan2f(NormalizedMtx.a.y,NormalizedMtx.b.y));
		else if (order[1] == 'z' && order[0] == 'x')  // xzy order
			return Vector3(rage::Atan2f(-NormalizedMtx.c.y,NormalizedMtx.b.y),rage::Atan2f(-NormalizedMtx.a.z,NormalizedMtx.a.x),rage::Asinf(NormalizedMtx.a.y));
	}
	else if (order[2]=='z')
	{
		if (order[1]=='x' && order[0] == 'y')		// yxz order
			return Vector3(rage::Asinf(NormalizedMtx.b.z),rage::Atan2f(-NormalizedMtx.a.z,NormalizedMtx.c.z), rage::Atan2f(-NormalizedMtx.b.x,NormalizedMtx.b.y));
		else if (order[1]=='y' &&  order[0] == 'x')  // xyz order
			return Vector3(rage::Atan2f(NormalizedMtx.b.z,NormalizedMtx.c.z),rage::Asinf(-NormalizedMtx.a.z),rage::Atan2f(NormalizedMtx.a.y,NormalizedMtx.a.x));
	}

	mthWarningf("Matrix33::GetEulers() - order variable is invalid"); 
	return Vector3(0.f,0.f,0.f);
}
#endif // MATRIX33_GETEULERS_S

#ifndef MATRIX33_GETEULERSFAST_S
#define MATRIX33_GETEULERSFAST_S
inline Vector3 Matrix33::GetEulersFast(const char *order) const 
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

	mthWarningf("Matrix33::GetEulers() - order variable is invalid"); 
	return Vector3(0.f,0.f,0.f);
}
#endif // MATRIX33_GETEULERSFAST_S

#ifndef MATRIX33_FROMEULERS
#define MATRIX33_FROMEULERS
inline void Matrix33::FromEulers(const Vector3 &e,const char *order) 
{
	if(order[0]=='x') {
		if(order[1]=='y' && order[2]=='z') FromEulersXYZ(e);
		else if(order[1]=='z' && order[2]=='y') FromEulersXZY(e);
		else mthWarningf("Matrix33::FromEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='y') {
		if(order[1]=='x' && order[2]=='z') FromEulersYXZ(e);
		else if(order[1]=='z' && order[2]=='x') FromEulersYZX(e);
		else mthWarningf("Matrix33::FromEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='z') {
		if(order[1]=='x' && order[2]=='y') FromEulersZXY(e);
		else if(order[1]=='y' && order[2]=='x') FromEulersZYX(e);
		else mthWarningf("Matrix33::FromEulers()- Bad string '%s'",order);
	}
	else mthWarningf("Matrix33::FromEulers()- Bad string '%s'",order);
}
#endif // MATRIX33_FROMEULERS

#ifndef MATRIX33_FROMEULERSXYZ
#define MATRIX33_FROMEULERSXYZ
inline void Matrix33::FromEulersXYZ(const Vector3 &e) 
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
#endif // MATRIX33_FROMEULERSXYZ

#ifndef MATRIX33_FROMEULERSXZY
#define MATRIX33_FROMEULERSXZY
inline void Matrix33::FromEulersXZY(const Vector3 &e) 
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
#endif // MATRIX33_FROMEULERSXZY

#ifndef MATRIX33_FROMEULERSYXZ
#define MATRIX33_FROMEULERSYXZ
inline void Matrix33::FromEulersYXZ(const Vector3 &e) 
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
#endif // MATRIX33_FROMEULERSYXZ

#ifndef MATRIX33_FROMEULERSYZX
#define MATRIX33_FROMEULERSYZX
inline void Matrix33::FromEulersYZX(const Vector3 &e) 
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
#endif // MATRIX33_FROMEULERSYZX

#ifndef MATRIX33_FROMEULERSZXY
#define MATRIX33_FROMEULERSZXY
inline void Matrix33::FromEulersZXY(const Vector3 &e) 
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
#endif // MATRIX33_FROMEULERSZXY

#ifndef MATRIX33_FROMEULERSZYX
#define MATRIX33_FROMEULERSZYX
inline void Matrix33::FromEulersZYX(const Vector3 &e) 
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
#endif // MATRIX33_FROMEULERSZYX

#ifndef MATRIX33_TOEULERS
#define MATRIX33_TOEULERS
inline void Matrix33::ToEulers(Vector3 &e,const char *order) const 
{
	if(order[0]=='x') {
		if(order[1]=='y' && order[2]=='z') ToEulersXYZ(e);
		else if(order[1]=='z' && order[2]=='y') ToEulersXZY(e);
		else mthWarningf("Matrix33::ToEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='y') {
		if(order[1]=='x' && order[2]=='z') ToEulersYXZ(e);
		else if(order[1]=='z' && order[2]=='x') ToEulersYZX(e);
		else mthWarningf("Matrix33::ToEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='z') {
		if(order[1]=='x' && order[2]=='y') ToEulersZXY(e);
		else if(order[1]=='y' && order[2]=='x') ToEulersZYX(e);
		else mthWarningf("Matrix33::ToEulers()- Bad string '%s'",order);
	}
	else mthWarningf("Matrix33::ToEulers()- Bad string '%s'",order);
}
#endif // MATRIX33_TOEULERS

#ifndef MATRIX33_TOEULERSFAST
#define MATRIX33_TOEULERSFAST
inline void Matrix33::ToEulersFast(Vector3 &e,const char *order) const 
{
	if(order[0]=='x') {
		if(order[1]=='y' && order[2]=='z') ToEulersFastXYZ(e);
		else if(order[1]=='z' && order[2]=='y') ToEulersFastXZY(e);
		else mthWarningf("Matrix33::ToEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='y') {
		if(order[1]=='x' && order[2]=='z') ToEulersFastYXZ(e);
		else if(order[1]=='z' && order[2]=='x') ToEulersFastYZX(e);
		else mthWarningf("Matrix33::ToEulers()- Bad string '%s'",order);
	}
	else if(order[0]=='z') {
		if(order[1]=='x' && order[2]=='y') ToEulersFastZXY(e);
		else if(order[1]=='y' && order[2]=='x') ToEulersFastZYX(e);
		else mthWarningf("Matrix33::ToEulers()- Bad string '%s'",order);
	}
	else mthWarningf("Matrix33::ToEulers()- Bad string '%s'",order);
}
#endif // MATRIX33_TOEULERSFAST

#ifndef MATRIX33_TOEULERSXYZ
#define MATRIX33_TOEULERSXYZ
inline void Matrix33::ToEulersXYZ(Vector3 &e) const 
{
	Matrix33 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
	e.Set(safe_atan2f(NormalizedMtx.b.z,NormalizedMtx.c.z),rage::Asinf(-NormalizedMtx.a.z),safe_atan2f(NormalizedMtx.a.y,NormalizedMtx.a.x));
}
#endif // MATRIX33_TOEULERSXYZ

#ifndef MATRIX33_TOEULERSFASTXYZ
#define MATRIX33_TOEULERSFASTXYZ
inline void Matrix33::ToEulersFastXYZ(Vector3 &e) const 
{
	e.Set(safe_atan2f(b.z,c.z),rage::Asinf(-a.z),safe_atan2f(a.y,a.x));
}
#endif // MATRIX33_TOEULERSFASTXYZ

#ifndef MATRIX33_TOEULERSXZY
#define MATRIX33_TOEULERSXZY
inline void Matrix33::ToEulersXZY(Vector3 &e) const 
{
	Matrix33 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
	e.Set(safe_atan2f(-NormalizedMtx.c.y,NormalizedMtx.b.y),safe_atan2f(-NormalizedMtx.a.z,NormalizedMtx.a.x),rage::Asinf(NormalizedMtx.a.y));
}
#endif // MATRIX33_TOEULERSXZY

#ifndef MATRIX33_TOEULERSFASTXZY
#define MATRIX33_TOEULERSFASTXZY
inline void Matrix33::ToEulersFastXZY(Vector3 &e) const 
{
	e.Set(safe_atan2f(-c.y,b.y),safe_atan2f(-a.z,a.x),rage::Asinf(a.y));
}
#endif // MATRIX33_TOEULERSFASTXZY

#ifndef MATRIX33_TOEULERSYXZ
#define MATRIX33_TOEULERSYXZ
inline void Matrix33::ToEulersYXZ(Vector3 &e) const 
{
	Matrix33 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
	e.Set(rage::Asinf(NormalizedMtx.b.z),safe_atan2f(-NormalizedMtx.a.z,NormalizedMtx.c.z),safe_atan2f(-NormalizedMtx.b.x,NormalizedMtx.b.y));
}
#endif // MATRIX33_TOEULERSYXZ

#ifndef MATRIX33_TOEULERSFASTYXZ
#define MATRIX33_TOEULERSFASTYXZ
inline void Matrix33::ToEulersFastYXZ(Vector3 &e) const 
{
	e.Set(rage::Asinf(b.z),safe_atan2f(-a.z,c.z),safe_atan2f(-b.x,b.y));
}
#endif // MATRIX33_TOEULERSFASTYXZ

#ifndef MATRIX33_TOEULERSYZX
#define MATRIX33_TOEULERSYZX
inline void Matrix33::ToEulersYZX(Vector3 &e) const 
{
	Matrix33 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
	e.Set(safe_atan2f(NormalizedMtx.b.z,NormalizedMtx.b.y),safe_atan2f(NormalizedMtx.c.x,NormalizedMtx.a.x),rage::Asinf(-NormalizedMtx.b.x));
}
#endif // MATRIX33_TOEULERSYZX

#ifndef MATRIX33_TOEULERSFASTYZX
#define MATRIX33_TOEULERSFASTYZX
inline void Matrix33::ToEulersFastYZX(Vector3 &e) const 
{
	e.Set(safe_atan2f(b.z,b.y),safe_atan2f(c.x,a.x),rage::Asinf(-b.x));
}
#endif // MATRIX33_TOEULERSFASTYZX

#ifndef MATRIX33_TOEULERSZXY
#define MATRIX33_TOEULERSZXY
inline void Matrix33::ToEulersZXY(Vector3 &e) const 
{
	Matrix33 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
	e.Set(rage::Asinf(-NormalizedMtx.c.y),safe_atan2f(NormalizedMtx.c.x,NormalizedMtx.c.z),safe_atan2f(NormalizedMtx.a.y,NormalizedMtx.b.y));
}
#endif // MATRIX33_TOEULERSZXY

#ifndef MATRIX33_TOEULERSFASTZXY
#define MATRIX33_TOEULERSFASTZXY
inline void Matrix33::ToEulersFastZXY(Vector3 &e) const 
{
	e.Set(rage::Asinf(-c.y),safe_atan2f(c.x,c.z),safe_atan2f(a.y,b.y));
}
#endif // MATRIX33_TOEULERSFASTZXY

#ifndef MATRIX33_TOEULERSZYX
#define MATRIX33_TOEULERSZYX
inline void Matrix33::ToEulersZYX(Vector3 &e) const 
{
	Matrix33 NormalizedMtx(*this);
	NormalizedMtx.Normalize();
	e.Set(safe_atan2f(-NormalizedMtx.c.y,NormalizedMtx.c.z),rage::Asinf(NormalizedMtx.c.x),safe_atan2f(-NormalizedMtx.b.x,NormalizedMtx.a.x));
}
#endif // MATRIX33_TOEULERSZYX

#ifndef MATRIX33_TOEULERSFASTZYX
#define MATRIX33_TOEULERSFASTZYX
inline void Matrix33::ToEulersFastZYX(Vector3 &e) const 
{
	e.Set(safe_atan2f(-c.y,c.z),rage::Asinf(c.x),safe_atan2f(-b.x,a.x));
}
#endif // MATRIX33_TOEULERSFASTZYX

#ifndef MATRIX33_SCALE
#define MATRIX33_SCALE
inline void Matrix33::Scale(float s) 
{
	a.Scale(s);
	b.Scale(s);
	c.Scale(s);
}
#endif // MATRIX33_SCALE

#ifndef MATRIX33_SCALE_F
#define MATRIX33_SCALE_F
inline void Matrix33::Scale(float x,float y,float z) 
{
	a.x*=x; b.x*=x; c.x*=x;
	a.y*=y; b.y*=y; c.y*=y;
	a.z*=z; b.z*=z; c.z*=z;
}
#endif // MATRIX33_SCALE_F

#ifndef MATRIX33_SCALE_V
#define MATRIX33_SCALE_V
inline void Matrix33::Scale(const Vector3& v)
{
	Scale(v.x,v.y,v.z);
}
#endif // MATRIX33_SCALE_V

#ifndef MATRIX33_MAKESCALE
#define MATRIX33_MAKESCALE
inline void Matrix33::MakeScale(float s) 
{
	a.Set(s,0.0f,0.0f);
	b.Set(0.0f,s,0.0f);
	c.Set(0.0f,0.0f,s);
}
#endif // MATRIX33_MAKESCALE

#ifndef MATRIX33_MAKESCALE_F
#define MATRIX33_MAKESCALE_F
inline void Matrix33::MakeScale(float x,float y,float z) 
{
	a.Set(x,0.0f,0.0f);
	b.Set(0.0f,y,0.0f);
	c.Set(0.0f,0.0f,z);
}
#endif // MATRIX33_MAKESCALE_F

#ifndef MATRIX33_MAKESCALE_V
#define MATRIX33_MAKESCALE_V
inline void Matrix33::MakeScale(const Vector3& v)
{
	MakeScale(v.x,v.y,v.z);
}
#endif // MATRIX33_MAKESCALE_V

#ifndef MATRIX33_INVERSE
#define MATRIX33_INVERSE
inline bool Matrix33::Inverse ()
{
	Matrix33 original(*this);
	return Inverse(original);
}
#endif // MATRIX33_INVERSE

#ifndef MATRIX33_INVERSE_M
#define MATRIX33_INVERSE_M
inline bool Matrix33::Inverse (const Matrix33& m)
{
	// Get three of the subdeterminants.
	float subDetX = Det2233(m.b.y,m.b.z,m.c.y,m.c.z);
	float subDetY = Det2233(m.b.x,m.b.z,m.c.x,m.c.z);
	float subDetZ = Det2233(m.b.x,m.b.y,m.c.x,m.c.y);

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

		// Get three more subdeterminants.
		subDetX = Det2233(m.a.y,m.a.z,m.c.y,m.c.z);
		subDetY = Det2233(m.a.x,m.a.z,m.c.x,m.c.z);
		subDetZ = Det2233(m.a.x,m.a.y,m.c.x,m.c.y);

		// Add more terms to the inverse matrix.
		a.y = -subDetX*invDet;
		b.y = subDetY*invDet;
		c.y = -subDetZ*invDet;

		// Get the last three subdeterminants.
		subDetX = Det2233(m.a.y,m.a.z,m.b.y,m.b.z);
		subDetY = Det2233(m.a.x,m.a.z,m.b.x,m.b.z);
		subDetZ = Det2233(m.a.x,m.a.y,m.b.x,m.b.y);

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
#endif // MATRIX33_INVERSE_M

#ifndef MATRIX33_INVERSE
#define MATRIX33_INVERSE
inline bool Matrix33::Inverse ()
{
	Matrix33 original;
	original.Set(*this);
	return Inverse(original);
}
#endif // MATRIX33_INVERSE

#ifndef MATRIX33_INVERSE_M
#define MATRIX33_INVERSE_M
inline bool Matrix33::Inverse (const Matrix33& m)
{
	// Get three of the subdeterminants.
	float subDetX = Det2233(m.b.y,m.b.z,m.c.y,m.c.z);
	float subDetY = Det2233(m.b.x,m.b.z,m.c.x,m.c.z);
	float subDetZ = Det2233(m.b.x,m.b.y,m.c.x,m.c.y);

	// Find the largest absolute value element.
	float bigElement = Max(Max(fabsf(m.a.x),fabsf(m.a.y),fabsf(m.a.z)),
							Max(fabsf(m.b.x),fabsf(m.b.y),fabsf(m.b.z)),
							Max(fabsf(m.c.x),fabsf(m.c.y),fabsf(m.c.z)));
	
	// Get the inverse of the determinant.
	float invDet = m.a.x*subDetX - m.a.y*subDetY + m.a.z*subDetZ;
	// The value 3.6e-7f is the square of the nearlyZero factor from Matrix33::SolveSVD.
	if (fabsf(invDet)>3.6e-7f*bigElement)
	{
		invDet = 1.0f/invDet;

		// Start making the inverse matrix.
		a.x = subDetX*invDet;
		b.x = -subDetY*invDet;
		c.x = subDetZ*invDet;

		// Get three more subdeterminants.
		subDetX = Det2233(m.a.y,m.a.z,m.c.y,m.c.z);
		subDetY = Det2233(m.a.x,m.a.z,m.c.x,m.c.z);
		subDetZ = Det2233(m.a.x,m.a.y,m.c.x,m.c.y);

		// Add more terms to the inverse matrix.
		a.y = -subDetX*invDet;
		b.y = subDetY*invDet;
		c.y = -subDetZ*invDet;

		// Get the last three subdeterminants.
		subDetX = Det2233(m.a.y,m.a.z,m.b.y,m.b.z);
		subDetY = Det2233(m.a.x,m.a.z,m.b.x,m.b.z);
		subDetZ = Det2233(m.a.x,m.a.y,m.b.x,m.b.y);

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
#endif // MATRIX33_INVERSE_M

#ifndef MATRIX33_FASTINVERSE
#define MATRIX33_FASTINVERSE
inline void Matrix33::FastInverse() 
{
	Matrix33 mtx=*this;
	FastInverse(mtx);
}
#endif // MATRIX33_FASTINVERSE

#ifndef MATRIX33_FASTINVERSE_M
#define MATRIX33_FASTINVERSE_M
inline void Matrix33::FastInverse(const Matrix33 &m) 
{
	register float r;

	r=m.a.x; a.x = r; 
	r=m.a.y; b.x = r; 
	r=m.a.z; c.x = r;

	r=m.b.x; a.y = r; 
	r=m.b.y; b.y = r; 
	r=m.b.z; c.y = r; 

	r=m.c.x; a.z = r; 
	r=m.c.y; b.z = r; 
	r=m.c.z; c.z = r;
}
#endif // MATRIX33_FASTINVERSE_M

#ifndef MATRIX33_FASTINVERSESCALED
#define MATRIX33_FASTINVERSESCALED
inline void Matrix33::FastInverseScaled(const Matrix33 &m) 
{
	register float r;
	register float invScale = m.a.InvMag();
	register float invScale2 = invScale*invScale;

	r=m.a.x*invScale2;  a.x = r; 
	r=m.a.y*invScale2;  b.x = r; 
	r=m.a.z*invScale2;  c.x = r;

	r=m.b.x*invScale2;  a.y = r; 
	r=m.b.y*invScale2;  b.y = r; 
	r=m.b.z*invScale2;  c.y = r; 

	r=m.c.x*invScale2;  a.z = r; 
	r=m.c.y*invScale2;  b.z = r; 
	r=m.c.z*invScale2;  c.z = r;
}
#endif // MATRIX33_FASTINVERSESCALED

#ifndef MATRIX33_TRANSPOSE
#define MATRIX33_TRANSPOSE
inline void Matrix33::Transpose() 
{
	float t;
	t=b.x; b.x=a.y; a.y=t;
	t=c.x; c.x=a.z; a.z=t;
	t=c.y; c.y=b.z; b.z=t;
}
#endif // MATRIX33_TRANSPOSE

#ifndef MATRIX33_TRANSPOSE_M
#define MATRIX33_TRANSPOSE_M
inline void Matrix33::Transpose(const Matrix33 &m) 
{
	a.Set(m.a.x,m.b.x,m.c.x);
	b.Set(m.a.y,m.b.y,m.c.y);
	c.Set(m.a.z,m.b.z,m.c.z);
}
#endif // MATRIX33_TRANSPOSE_M

#ifndef MATRIX33_COORDINATEINVERSESAFE
#define MATRIX33_COORDINATEINVERSESAFE
inline void Matrix33::CoordinateInverseSafe(float error)
{
	if (IsOrthonormal(error))
	{
		Transpose();
	}
	else
	{
		NormalizeSafe();
		Transpose();
	}
}
#endif // MATRIX33_COORDINATEINVERSESAFE

#ifndef MATRIX33_COORDINATEINVERSESAFE_M
#define MATRIX33_COORDINATEINVERSESAFE_M
inline void Matrix33::CoordinateInverseSafe(const Matrix33& m, float error)
{
	if (m.IsOrthonormal(error))
	{
		Transpose(m);
	}
	else
	{
		Matrix33 normalizedM(m);
		normalizedM.NormalizeSafe();
		Transpose(normalizedM);
	}
}
#endif // MATRIX33_COORDINATEINVERSESAFE_M

#ifndef MATRIX33_DOTCROSSPRODMTX
#define MATRIX33_DOTCROSSPRODMTX
inline void Matrix33::DotCrossProdMtx( const Vector3 & v) 	// Dot(A.CrossProduct(v)).
{
	a.Cross(v);
	b.Cross(v);
	c.Cross(v);
}
#endif // MATRIX33_DOTCROSSPRODMTX

#ifndef MATRIX33_DOTCROSSPRODTRANSPOSE
#define MATRIX33_DOTCROSSPRODTRANSPOSE
inline void Matrix33::DotCrossProdTranspose( const Vector3 & v)
{
	a.CrossNegate(v);
	b.CrossNegate(v);
	c.CrossNegate(v);
}
#endif // MATRIX33_DOTCROSSPRODTRANSPOSE

#ifndef MATRIX33_MAKEDOUBLECROSSMATRIX
#define MATRIX33_MAKEDOUBLECROSSMATRIX
inline void Matrix33::MakeDoubleCrossMatrix (const Vector3& vector)
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
#endif // MATRIX33_MAKEDOUBLECROSSMATRIX

#ifndef MATRIX33_MAKEDOUBLECROSSMATRIX_V
#define MATRIX33_MAKEDOUBLECROSSMATRIX_V
inline void Matrix33::MakeDoubleCrossMatrix (const Vector3& vectorA, const Vector3& vectorB)
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
#endif // MATRIX33_MAKEDOUBLECROSSMATRIX_V

#ifndef MATRIX33_NORMALIZE
#define MATRIX33_NORMALIZE
inline void Matrix33::Normalize() 
{
	c.Normalize();
	a.Cross(b,c);
	a.Normalize();
	b.Cross(c,a);
}
#endif // MATRIX33_NORMALIZE

#ifndef MATRIX33_NORMALIZESAFE
#define MATRIX33_NORMALIZESAFE
inline void Matrix33::NormalizeSafe() 
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
#endif // MATRIX33_NORMALIZESAFE

#ifndef MATRIX33_DETERMINANT
#define MATRIX33_DETERMINANT
inline float Matrix33::Determinant() const 
{
	return Det3333(a.x,a.y,a.z, b.x,b.y,b.z, c.x,c.y,c.z); 
}
#endif // MATRIX33_DETERMINANT

#ifndef MATRIX33_SOLVESVD
#define MATRIX33_SOLVESVD

#define M34_BAD_INDEX	-1
#define M34_SMALL_FLOAT 1.0e-8f
inline bool Matrix33::SolveSVD (const Vector3& in, Vector3& out) const
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
	Matrix33 subDet;
	subDet.a.x = Det2233(b.y,b.z,c.y,c.z);
	subDet.a.y = -Det2233(b.x,b.z,c.x,c.z);
	subDet.a.z = Det2233(b.x,b.y,c.x,c.y);
	subDet.b.x = -Det2233(a.y,a.z,c.y,c.z);
	subDet.b.y = Det2233(a.x,a.z,c.x,c.z );
	subDet.b.z = -Det2233(a.x,a.y,c.x,c.y);
	subDet.c.x = Det2233(a.y,a.z,b.y,b.z);
	subDet.c.y = -Det2233(a.x,a.z,b.x,b.z);
	subDet.c.z = Det2233(a.x,a.y,b.x,b.y);

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

	// Find the  determinant.
	float det = a.x*subDet.a.x + a.y*subDet.a.y + a.z*subDet.a.z;

	// Find tolerance levels for squares and cubes of elements to be near zero.
	float nearlyZero2 = square(nearlyZero);
	float nearlyZero3 = nearlyZero * nearlyZero2;

	if (fabsf(det)>Max(nearlyZero3,maxSubDet*nearlyZero))
	{
		// The absolute value of the  determinant is not nearly zero,
		// and it is not too small compared to the largest 2x2 subdeterminant.
		if (maxSubDetInRow[(rowMax+1)%3]>nearlyZero2 && maxSubDetInRow[(rowMax+2)%3]>nearlyZero2)
		{
			// The maximum subdeterminants in the two rows other than the row with the overall maximum subdeterminant
			// are not nearly zero, so this matrix has three linearly independent column vectors (it has full rank),
			// and its inverse is the transpose of its matrix of 2x2 subdeterminants scaled by the inverse  determinant.
			subDet.UnTransform(in,out);
			out.InvScale(det);
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

		// Copy the row of the  matrix of 2x2 subdeterminants that contains the element with the maximum 2x2 subdeterminant.
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
		float invDet22 = 1.0f/Det2233(dA0,dA1,dB0,dB1);
		out[(maxSubDetRow+1)%3] = Det2233(u0,u1,dB0,dB1)*invDet22;
		out[(maxSubDetRow+2)%3] = -Det2233(u0,u1,dA0,dA1)*invDet22;
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
	float detXY = Det2233(maxSubDetRowVector.x,maxSubDetRowVector.y,in.x,in.y);
	float detXZ = Det2233(maxSubDetRowVector.x,maxSubDetRowVector.z,in.x,in.z);

	// Return true if this is a good solution (if the overdefined set of linear equations has a solution).
	// Return false if this is an approximate solution (the overdefined set of linear equations has no solution).
	return (fabsf(detXY)<=nearlyZero2 && fabsf(detXZ)<=nearlyZero2);
}
#endif // MATRIX33_SOLVESVD

#ifndef MATRIX33_SOLVESVD_V
#define MATRIX33_SOLVESVD_V
inline Vector3 Matrix33::SolveSVD(const Vector3& in) const
{ 
	Vector3 out; 
	SolveSVD(in,out); 
	return out; 
}
#endif // MATRIX33_SOLVESVD_V

#ifndef MATRIX33_SOLVESVDCONDITION
#define MATRIX33_SOLVESVDCONDITION
inline bool Matrix33::SolveSVDCondition (const Vector3& in, Vector3& out) const
{
	Matrix33 modifiedThis(*this);
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
#endif // MATRIX33_SOLVESVDCONDITION

#ifndef MATRIX33_OPERATOREQUAL
#define MATRIX33_OPERATOREQUAL
__forceinline Matrix33& Matrix33::operator=(const Matrix33 &m)
{
	Set(m);
	return *this;
}
#endif // MATRIX33_OPERATOREQUAL


#ifndef MATRIX33_OPERATOREQUAL33
#define MATRIX33_OPERATOREQUAL33
__forceinline Matrix33& Matrix33::operator=(const Matrix34 &m)
{
	a = m.a;
	b = m.b;
	c = m.c;
	return *this;
}
#endif // MATRIX33_OPERATOREQUAL33

#ifndef MATRIX33_MATRIX33
#define MATRIX33_MATRIX33
__forceinline Matrix33::Matrix33(const Matrix33 &m)
{
	a = m.a;
	b = m.b;
	c = m.c;
}
#endif // MATRIX33_MATRIX33

#ifndef MATRIX33_3V4
#define MATRIX33_3V4
__forceinline Matrix33::Matrix33( Vector4::Vector4In _a, Vector4::Vector4In _b, Vector4::Vector4In _c )
{
	a = _a;
	b = _b;
	c = _c;
}
#endif // MATRIX33_3V4

#ifndef MATRIX33_COPY3433
#define MATRIX33_COPY3433
inline Matrix33::Matrix33(const Matrix34 &m)
{
	a = m.a;
	b = m.b;
	c = m.c;
}
#endif // MATRIX33_COPY3433


#ifndef MATRIX33_GETELEMENT
#define MATRIX33_GETELEMENT
inline float & Matrix33::GetElement(int i, int j)
{
	FastAssert(i>=0 && i<=3 && j>=0 && j<=2);
	return ((float*)this)[i * VEC3_NUM_STORED_FLOATS + j];
}
#endif // MATRIX33_GETELEMENT

#ifndef MATRIX33_GETELEMENT_CONST
#define MATRIX33_GETELEMENT_CONST
inline const float & Matrix33::GetElement(int i, int j) const
{
	FastAssert(i>=0 && i<=3 && j>=0 && j<=2);
	return ((const float*)this)[i * VEC3_NUM_STORED_FLOATS + j];
}
#endif // MATRIX33_GETELEMENT_CONST

#ifndef MATRIX33_GETVECTOR
#define MATRIX33_GETVECTOR
inline Vector3 & Matrix33::GetVector(int i)
{
	FastAssert(i>=0 && i<=3);
	return ((Vector3 *)this)[i];
}
#endif // MATRIX33_GETVECTOR

#ifndef MATRIX33_GETVECTOR_CONST
#define MATRIX33_GETVECTOR_CONST
inline const Vector3 & Matrix33::GetVector(int i) const
{
	FastAssert(i>=0 && i<=3);
	return ((const Vector3 *)this)[i];
} 
#endif // MATRIX33_GETVECTOR_COSNT



// Vector3 functions.  should these really be here?
#ifndef MATRIX33_GDOT
#define MATRIX33_GDOT
inline Vector3 Dot(const Vector3& v, const Matrix33& mtx) 
{
	Vector3 temp;
	mtx.Transform(v,temp);
	return temp;
}
#endif // MATRIX33_GDOT

#ifndef VECTOR3_DOT_M33
#define VECTOR3_DOT_M33
inline void Vector3::Dot(const Matrix33& mtx)
{
	float newX = x*mtx.a.x + y*mtx.b.x + z*mtx.c.x;
	float newY = x*mtx.a.y + y*mtx.b.y + z*mtx.c.y;
	z          = x*mtx.a.z + y*mtx.b.z + z*mtx.c.z;
	x = newX;
	y = newY;
}
#endif // VECTOR3_DOT_M

#ifndef VECTOR3_DOT_VM33
#define VECTOR3_DOT_VM33
inline void Vector3::Dot(const Vector3& v,const Matrix33& mtx)
{
	x = v.x*mtx.a.x + v.y*mtx.b.x + v.z*mtx.c.x;
	y = v.x*mtx.a.y + v.y*mtx.b.y + v.z*mtx.c.y;
	z = v.x*mtx.a.z + v.y*mtx.b.z + v.z*mtx.c.z;
}
#endif // VECTOR3_DOT_VM

#ifndef VECTOR3_DOTTRANSPOSE33
#define VECTOR3_DOTTRANSPOSE33
inline void Vector3::DotTranspose(const Matrix33& mtx)
{
	float newX = x*mtx.a.x + y*mtx.a.y + z*mtx.a.z;
	float newY = x*mtx.b.x + y*mtx.b.y + z*mtx.b.z;
	z          = x*mtx.c.x + y*mtx.c.y + z*mtx.c.z;
	x = newX;
	y = newY;
}
#endif // VECTOR3_DOT3X3TRANSPOSE


} // namespace rage

#endif // VECTOR_MATRIX33_DEFAULT_H
