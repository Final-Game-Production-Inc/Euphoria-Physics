// 
// vector/quaternion_default.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "math/simplemath.h"

namespace rage
{


#define		FAST_QUAT		1 && !RSG_CPU_INTEL


#ifndef QUAT_INTRINCONSTRUC
#define QUAT_INTRINCONSTRUC
	__forceinline Quaternion::Quaternion(__vector4 q)
		: xyzw(q)
	{
	}
#endif // QUAT_INTRINCONSTRUC

#ifndef QUAT_COPYCONSTRUC
#define QUAT_COPYCONSTRUC
	__forceinline Quaternion::Quaternion(const Quaternion& q)
		: xyzw(q.xyzw)
	{		
	}
#endif // QUAT_COPYCONSTRUC

#ifndef QUAT_OPERATOR_EQ
#define QUAT_OPERATOR_EQ
	__forceinline Quaternion& Quaternion::operator= (const Quaternion& q)
	{
		xyzw = q.xyzw;
		return *this;
	}
#endif // QUAT_OPERATOR_EQ

#ifndef QUAT_CONST
#define QUAT_CONST
inline Quaternion::Quaternion()
{
#if __INIT_NAN
	if (!g_DisableInitNan)
	{
		MakeNan(x);
		MakeNan(y);
		MakeNan(z);
		MakeNan(w);
	}
#endif
}	//lint !e1541 !e1401 not variable initialized by constructor
#endif // QUAT_CONST

#ifndef QUAT_CONST_IDENTITY
#define QUAT_CONST_IDENTITY
inline Quaternion::Quaternion(_IdentityType)
{
	Identity();
}
#endif // QUAT_CONST_IDENTITY


#ifndef QUAT_CONST_F4
#define QUAT_CONST_F4
inline Quaternion::Quaternion(float x0,float y0,float z0,float w0)
{	//lint  !e578 y0 overrides y0 in <math.h>
	x=x0;
	y=y0;
	z=z0;
	w=w0;
}
#endif // QUAT_CONST_F4

#ifndef QUAT_SET_F4
#define QUAT_SET_F4
inline void Quaternion::Set(float sx,float sy,float sz,float sw)
{
	x=sx;
	y=sy;
	z=sz;
	w=sw;
} 
#endif // QUAT_SET_F4

#ifndef QUAT_SET_Q
#define QUAT_SET_Q
inline void Quaternion::Set(const Quaternion& q)
{
	x=q.x;
	y=q.y;
	z=q.z;
	w=q.w;
}
#endif // QUAT_SET_Q

#ifndef QUAT_IDENTITY
#define QUAT_IDENTITY
inline void Quaternion::Identity()
{
	x=0.0f;
	y=0.0f;
	z=0.0f;
	w=1.0f;
}
#endif // QUAT_IDENTITY

#ifndef QUAT_TO_ROTATION_VF
#define QUAT_TO_ROTATION_VF
inline void Quaternion::ToRotation(Vector3& outAxis, float& outAngle) const
{
	outAngle = GetAngle();

	outAxis.x = x;
	outAxis.y = y;
	outAxis.z = z;

	outAxis.NormalizeSafe(Vector3(0.f,0.f,0.f));
}
#endif // QUAT_TO_ROTATION_VF

#ifndef QUAT_FROM_ROTATION_VF
#define QUAT_FROM_ROTATION_VF
inline void Quaternion::FromRotation(const Vector3& unit, float angle)
{
	float Localcos,Localsin;
	cos_and_sin(Localcos,Localsin,0.5f*angle);
	Set(unit.x*Localsin,unit.y*Localsin,unit.z*Localsin,Localcos);
}
#endif //QUAT_FROM_ROTATION_VF

#ifndef QUAT_FROM_EULERS_V
#define QUAT_FROM_EULERS_V
inline void Quaternion::FromEulers(const Vector3& euler)
{
	// TODO --- write this properly!
	Matrix34 m;
	m.FromEulersXYZ(euler);
	FromMatrix34(m);
}
#endif //QUAT_FROM_EULERS_V


#ifndef QUAT_FROM_EULERS_VP
#define QUAT_FROM_EULERS_VP
inline void Quaternion::FromEulers(const Vector3& euler, const char* order)
{
	// TODO --- write this properly!
	Matrix34 m;
	m.FromEulers(euler, order);
	FromMatrix34(m);
}
#endif //QUAT_FROM_EULERS_VP

#ifndef QUAT_FROM_EULERS_VP2
#define QUAT_FROM_EULERS_VP2
inline void Quaternion::FromEulers(const Vector3& euler, eEulerOrders order)
{
	const __vector4 signs[] = {{-1.f,1.f,-1.f,1.f}, {1.f,1.f,-1.f,-1.f}, {-1.f,1.f,1.f,-1.f}, {-1.f,-1.f,1.f,1.f}, {1.f,-1.f,-1.f,1.f}, {1.f,-1.f,1.f,-1.f}};

#if __PS3
	Vector3 s(sinf4(0.5f*euler));
	Vector3 c(cosf4(0.5f*euler));
#else
	Vector3 s(sinf(0.5f*euler.x), sinf(0.5f*euler.y), sinf(0.5f*euler.z));
	Vector3 c(cosf(0.5f*euler.x), cosf(0.5f*euler.y), cosf(0.5f*euler.z));
#endif

	Vector4 a1(s.x, c.x, c.x, c.x);
	Vector4 a2(c.y, s.y, c.y, c.y);
	Vector4 a3(c.z, c.z, s.z, c.z);
	Vector4 b1(c.x, s.x, s.x, s.x);
	Vector4 b2(s.y, c.y, s.y, s.y);
	Vector4 b3(s.z, s.z, c.z, s.z);

	Vector4& result = *reinterpret_cast<Vector4*>(this);
	result = a1*a2*a3+b1*b2*b3*Vector4(signs[order]);
}
#endif

#ifndef QUAT_FROM_MATRIX34
#define QUAT_FROM_MATRIX34
inline void Quaternion::FromMatrix34(const Matrix34& m)
{
	float temp=m.a.x+m.b.y+m.c.z;
	int largest=MaximumIndex(temp,m.a.x,m.b.y,m.c.z);
	switch(largest)
	{
	case 0:
		w=0.5f*sqrtf(temp+1.0f);
		temp=0.25f/w;
		x=(m.b.z-m.c.y)*temp;
		y=(m.c.x-m.a.z)*temp;
		z=(m.a.y-m.b.x)*temp;
		break;
	case 1:
		x=0.5f*sqrtf(2.0f*m.a.x-temp+1.0f);
		temp=0.25f/x;
		w=(m.b.z-m.c.y)*temp;
		y=(m.a.y+m.b.x)*temp;
		z=(m.c.x+m.a.z)*temp;
		break;
	case 2:
		y=0.5f*sqrtf(2.0f*m.b.y-temp+1.0f);
		temp=0.25f/y;
		w=(m.c.x-m.a.z)*temp;
		x=(m.a.y+m.b.x)*temp;
		z=(m.b.z+m.c.y)*temp;
		break;
	case 3:
		z=0.5f*sqrtf(2.0f*m.c.z-temp+1.0f);
		temp=0.25f/z;
		w=(m.a.y-m.b.x)*temp;
		x=(m.c.x+m.a.z)*temp;
		y=(m.b.z+m.c.y)*temp;
		break;
	default:
		FastAssert(0);	//lint !e506 constant value boolean
	}
}
#endif // QUAT_FROM_MATRIX34

#ifndef QUAT_FROM_VECTORS_VV
#define QUAT_FROM_VECTORS_VV
inline bool Quaternion::FromVectors(const Vector3& from, const Vector3& to)
{
	Vector3 fromN, toN;

	bool result = true;

	fromN.Normalize(from);
	toN.Normalize(to);

	float dot = fromN.Dot(toN);

	if (fabsf(dot - 1) < SMALL_FLOAT)
	{
		// Co-linear in same direction, no rotation.
		Identity();

		return result;
	}

	if (fabsf(dot + 1) < SMALL_FLOAT)
	{
		// Co-linear in opposite direction, find a good axis of rotation.
		result = false;

		switch(MaximumIndex(from.x, from.y, from.z))
		{
		case 0:
			toN.x = toN.y = 0;
			toN.z = 1;
			break;
		case 1:
			toN.x = toN.y = 0;
			toN.z = 1;
			break;
		case 2:
			toN.x = toN.z = 0;
			toN.y = 1;
			break;
		default:
			mthErrorf("Invalid MaximumIndex result %d", MaximumIndex(from.x, from.y, from.z));	//lint !e506 constant value boolean
		}
	}

	x = fromN.y * toN.z - fromN.z * toN.y;
	y = fromN.z * toN.x - fromN.x * toN.z;
	z = fromN.x * toN.y - fromN.y * toN.x;
	w = dot + 1.0f;

	Normalize();

	return result;
}
#endif // QUAT_FROM_VECTORS_VV


#ifndef QUAT_FROM_VECTORS_VVV
#define QUAT_FROM_VECTORS_VVV
inline void Quaternion::FromVectors(const Vector3& from, const Vector3& to, const Vector3& axis)
{
	Vector3 fromN, toN;

	fromN.SubtractScaled(from, axis, from.Dot(axis));
	toN.SubtractScaled(to, axis, to.Dot(axis));

	fromN.Normalize();
	toN.Normalize();

	float dot = fromN.Dot(toN);

	if (fabsf(dot - 1) < SMALL_FLOAT)
	{
		// Co-linear in same direction, no rotation.
		Identity();
	}
	else if (fabsf(dot + 1) < SMALL_FLOAT)
	{
		// Co-linear in opposite direction,
		FromRotation(axis, PI);
	}
	else
	{
		Vector3 cross;
		cross.x = fromN.y * toN.z - fromN.z * toN.y;
		cross.y = fromN.z * toN.x - fromN.x * toN.z;
		cross.z = fromN.x * toN.y - fromN.y * toN.x;

		FromRotation(cross.Dot(axis) >= 0 ? axis : -axis, acosf(dot));
	}
}
#endif // QUAT_FROM_VECTORS_VVV


#ifndef QUAT_TO_EULERS_V
#define QUAT_TO_EULERS_V
inline void Quaternion::ToEulers(Vector3& euler) const
{
	// TOOD --- write this properly!
	Matrix34 m;
	m.FromQuaternion(*this);
	m.ToEulersXYZ(euler);
}
#endif //QUAT_TO_EULERS_V

#ifndef QUAT_TO_EULERS_VP
#define QUAT_TO_EULERS_VP
inline void Quaternion::ToEulers(Vector3& euler, const char* order) const
{
	// TOOD --- write this properly!
	Matrix34 m;
	m.FromQuaternion(*this);
	euler = m.GetEulers(order);
}
#endif //QUAT_TO_EULERS_VP

#ifndef QUAT_TO_EULERS_VP2
#define QUAT_TO_EULERS_VP2
inline void Quaternion::ToEulers(Vector3& e, eEulerOrders order)
{
	switch(order)
	{
	case eEulerOrderXYZ: e.Set(Atan2f(2.f*(y*z+x*w), 1.f-2.f*(x*x+y*y)),AsinfSafe(-2.f*(x*z-y*w)),Atan2f(2.f*(x*y+z*w),1.f-2.f*(y*y+z*z))); break;
	case eEulerOrderXZY: e.Set(Atan2f(-2.f*(y*z-x*w),1.f-2.f*(x*x+z*z)),Atan2f(-2.f*(x*z-y*w),1.f-2.f*(y*y+z*z)),AsinfSafe(2.f*(x*y+z*w))); break;
	case eEulerOrderYXZ: e.Set(AsinfSafe(2.f*(y*z+x*w)),Atan2f(-2.f*(x*z-y*w),1.f-2.f*(x*x+y*y)),Atan2f(-2.f*(x*y-z*w),1.f-2.f*(x*x+z*z))); break;
	case eEulerOrderYZX: e.Set(Atan2f(2.f*(y*z+x*w),1.f-2.f*(x*x+z*z)),Atan2f(2.f*(x*z+y*w),1.f-2.f*(y*y+z*z)),AsinfSafe(-2.f*(x*y-z*w))); break;
	case eEulerOrderZXY: e.Set(AsinfSafe(-2.f*(y*z-x*w)),Atan2f(2.f*(x*z+y*w),1.f-2.f*(x*x+y*y)),Atan2f(2.f*(x*y+z*w),1.f-2.f*(x*x+z*z))); break;
	case eEulerOrderZYX: e.Set(Atan2f(-2.f*(y*z-x*w),1.f-2.f*(x*x+y*y)),AsinfSafe(2.f*(x*z+y*w)),Atan2f(-2.f*(x*y-z*w),1.f-2.f*(y*y+z*z))); break;
	}
}
#endif

#ifndef QUAT_NEGATE
#define QUAT_NEGATE
inline void Quaternion::Negate()
{
	x=-x;
	y=-y;
	z=-z;
	w=-w;
}
#endif // QUAT_NEGATE

#ifndef QUAT_NEGATE_Q
#define QUAT_NEGATE_Q
inline void Quaternion::Negate(const Quaternion &q)
{
	x=-q.x; y=-q.y; z=-q.z; w=-q.w;
}
#endif // QUAT_NEGATE_Q

#ifndef QUAT_INV
#define QUAT_INV
inline void Quaternion::Inverse()
{
	x=-x;
	y=-y;
	z=-z;
	// w is unchanged
}
#endif // QUAT_INV

#ifndef QUAT_INV_Q
#define QUAT_INV_Q
inline void Quaternion::Inverse(const Quaternion &q)
{
	x=-q.x;
	y=-q.y;
	z=-q.z;
	w=q.w;
}
#endif // QUAT_INV_Q

#ifndef QUAT_SCALE_F
#define QUAT_SCALE_F
inline void Quaternion::Scale(float f)
{
	x*=f;
	y*=f;
	z*=f;
	w*=f;
}
#endif // QUAT_SCALE_F

#ifndef QUAT_SCALE_QF
#define QUAT_SCALE_QF
inline void Quaternion::Scale(const Quaternion &q,float f)
{
	x=q.x*f;
	y=q.y*f;
	z=q.z*f;
	w=q.w*f;
}
#endif // QUAT_SCALE_QF

#ifndef QUAT_SCALE_ANGLE_F
#define QUAT_SCALE_ANGLE_F
inline void Quaternion::ScaleAngle(float f)
{
	Vector3 axis;
	float angle;
	ToRotation(axis, angle);
	if(axis.Mag2() > 0.999f && angle*angle > 0.f)
	{
		FromRotation(axis, angle*f);
	}
}
#endif // QUAT_SCALE_ANGLE_F

#ifndef QUAT_MAG
#define QUAT_MAG
inline float Quaternion::Mag() const
{
	return sqrtf(Mag2());
}
#endif // QUAT_MAG

#ifndef QUAT_MAG2
#define QUAT_MAG2
inline float Quaternion::Mag2() const
{
	return x*x + y*y + z*z + w*w;
}
#endif // QUAT_MAG2

#ifndef QUAT_INV_MAG
#define QUAT_INV_MAG
inline float Quaternion::InvMag() const
{
	return invsqrtf(Mag2());
}
#endif // QUAT_INV_MAG

#ifndef QUAT_NORMALIZE
#define QUAT_NORMALIZE
VEC3_INLINE void Quaternion::Normalize()
{
	Scale(InvMag());
}
#endif // QUAT_NORMALIZE

#ifndef QUAT_NORMALIZE_Q
#define QUAT_NORMALIZE_Q
inline void Quaternion::Normalize(const Quaternion& q)
{
	Scale(q,q.InvMag());
}
#endif // QUAT_NORMALIZE_Q

#ifndef QUAT_LERP_FQ
#define QUAT_LERP_FQ
inline void Quaternion::Lerp(float t, const Quaternion &q)
{
	const float s = 1.f-t;
	x = x*s + q.x*t;
	y = y*s + q.y*t;
	z = z*s + q.z*t;
	w = w*s + q.w*t;
}
#endif // QUAT_LERP_FQ

#ifndef QUAT_LERP_FQQ
#define QUAT_LERP_FQQ
inline void Quaternion::Lerp(float t, const Quaternion &q1, const Quaternion &q2)
{
	const float s = 1.f-t;
	x = q1.x*s + q2.x*t;
	y = q1.y*s + q2.y*t;
	z = q1.z*s + q2.z*t;
	w = q1.w*s + q2.w*t;
}
#endif // QUAT_LERP_FQQ

#ifndef QUAT_MULTIPLY_Q
#define QUAT_MULTIPLY_Q
inline void Quaternion::Multiply(const Quaternion &q)
{
	float tempW=w*q.w-x*q.x-y*q.y-z*q.z;
	float tempX=w*q.x+q.w*x+y*q.z-z*q.y;
	float tempY=w*q.y+q.w*y+z*q.x-x*q.z;
	z=w*q.z+q.w*z+x*q.y-y*q.x;
	w=tempW;
	x=tempX;
	y=tempY;
}
#endif //QUAT_MULTIPLY_Q

#ifndef QUAT_MULTIPLY_QQ
#define QUAT_MULTIPLY_QQ
inline void Quaternion::Multiply(const Quaternion &q1, const Quaternion &q2)
{
	FastAssert(this!=&q1 && this!=&q2);	//lint !e506 constant value boolean
	// for two quaternions q1=(float s1,Vector3 v1) and q2=(s2,v2) the quaternion
	// q.Multiply(q1,q2) is (s1*s2-v1.Dot(v2),Scale(v2,s1)+Scale(v1,s2)+Cross(v1,v2))
	w=q1.w*q2.w-q1.x*q2.x-q1.y*q2.y-q1.z*q2.z;
	x=q1.w*q2.x+q2.w*q1.x+q1.y*q2.z-q1.z*q2.y;
	y=q1.w*q2.y+q2.w*q1.y+q1.z*q2.x-q1.x*q2.z;
	z=q1.w*q2.z+q2.w*q1.z+q1.x*q2.y-q1.y*q2.x;
}
#endif //QUAT_MULTIPLY_QQ

#ifndef QUAT_MULTIPLY_INVERSE_Q
#define QUAT_MULTIPLY_INVERSE_Q
inline void Quaternion::MultiplyInverse(const Quaternion &q)
{
	float tempW=w*q.w+x*q.x+y*q.y+z*q.z;
	float tempX=-w*q.x+q.w*x-y*q.z+z*q.y;
	float tempY=-w*q.y+q.w*y-z*q.x+x*q.z;
	z=-w*q.z+q.w*z-x*q.y+y*q.x;
	w=tempW;
	x=tempX;
	y=tempY;
}
#endif //QUAT_MULTIPLY_INVERSE_Q

#ifndef QUAT_MULTIPLY_INVERSE_FROM_LEFT_Q
#define QUAT_MULTIPLY_INVERSE_FROM_LEFT_Q
inline void Quaternion::MultiplyInverseFromLeft(const Quaternion &q)
{
	// TODO: Find a more efficient implementation
	Quaternion r;
	r.Inverse(q);
	MultiplyFromLeft(r);
}
#endif //QUAT_MULTIPLY_INVERSE_FROM_LEFT_Q

#ifndef QUAT_MULTIPLY_INVERSE_QQ
#define QUAT_MULTIPLY_INVERSE_QQ
inline void Quaternion::MultiplyInverse(const Quaternion &q1, const Quaternion &q2)
{
	FastAssert(this!=&q1 && this!=&q2);		//lint !e506 constant value boolean
	w=q1.w*q2.w+q1.x*q2.x+q1.y*q2.y+q1.z*q2.z;
	x=-q1.w*q2.x+q2.w*q1.x-q1.y*q2.z+q1.z*q2.y;
	y=-q1.w*q2.y+q2.w*q1.y-q1.z*q2.x+q1.x*q2.z;
	z=-q1.w*q2.z+q2.w*q1.z-q1.x*q2.y+q1.y*q2.x;
}
#endif //QUAT_MULTIPLY_INVERSE_QQ

#ifndef QUAT_MULTIPLY_FROM_LEFT_Q
#define QUAT_MULTIPLY_FROM_LEFT_Q
inline void Quaternion::MultiplyFromLeft(const Quaternion &q)
{
	float tempW=q.w*w-q.x*x-q.y*y-q.z*z;
	float tempX=q.w*x+w*q.x+q.y*z-q.z*y;
	float tempY=q.w*y+w*q.y+q.z*x-q.x*z;
	z=q.w*z+w*q.z+q.x*y-q.y*x;
	w=tempW;
	x=tempX;
	y=tempY;
}
#endif //QUAT_MULTIPLY_FROM_LEFT

#ifndef QUAT_GET_DIRECTION
#define QUAT_GET_DIRECTION
inline void Quaternion::GetDirection(Vector3& outDirection) const
{
	outDirection.Set(x,y,z);
}
#endif // QUAT_GET_DIRECTION

#ifndef QUAT_GET_UNIT_DIRECTION
#define QUAT_GET_UNIT_DIRECTION
inline void Quaternion::GetUnitDirection(Vector3& outDirection) const
{
	outDirection.Set(x,y,z);
	float mag2 = outDirection.Mag2();
	if (mag2>VERY_SMALL_FLOAT)
	{
		outDirection.Scale(invsqrtf(mag2));
	}
	else
	{
		outDirection.Set(YAXIS);
	}
}
#endif // QUAT_GET_UNIT_DIRECTION

#ifndef QUAT_DOT
#define QUAT_DOT
inline float Quaternion::Dot(const Quaternion &q) const
{
	return x*q.x + y*q.y + z*q.z + w*q.w;
}
#endif // QUAT_DOT

#ifndef QUAT_REL_ANGLE
#define QUAT_REL_ANGLE
inline float Quaternion::RelAngle(const Quaternion &q) const
{
	float c = Dot(q);
	if(c<=-1.0f || c>=1.0f)
	{
		return(0.0f);
	}
	else
	{
		return(2.0f * rage::Acosf(fabsf(c)));
	}
}
#endif // QUAT_REL_ANGLE

#ifndef QUAT_GET_ANGLE
#define QUAT_GET_ANGLE
inline float Quaternion::GetAngle() const
{
	return 2.f * rage::Acosf(Clamp(w, -1.f, 1.f));
}
#endif // QUAT_GET_ANGLE

#ifndef QUAT_REL_COS_HALF_ANGLE
#define QUAT_REL_COS_HALF_ANGLE
inline float Quaternion::RelCosHalfAngle(const Quaternion &q) const
{
	// It turns out that that's exactly what the dot product calculates.
	return Dot(q);
}
#endif // QUAT_REL_COS_HALF_ANGLE

#ifndef QUAT_GET_COS_HALF_ANGLE
#define QUAT_GET_COS_HALF_ANGLE
inline float Quaternion::GetCosHalfAngle() const
{
	return w;
}
#endif // QUAT_GET_COS_HALF_ANGLE

#ifndef QUAT_PREPARE_SLERP
#define QUAT_PREPARE_SLERP
VEC3_INLINE void Quaternion::PrepareSlerp(const Quaternion &a)
{
	// makes Slerp go the short way
	if(Dot(a)<0) Negate();
}
#endif // QUAT_PREPARE_SLERP

#ifndef QUAT_SLERP_NEAR_FQ
#define QUAT_SLERP_NEAR_FQ
inline void Quaternion::SlerpNear(float t, const Quaternion &a)
{
	// makes Slerp go the short way
	if(Dot(a)<0)
	{
		Quaternion q(a);
		q.Negate();
		FastAssert(Dot(q)>-SMALL_FLOAT);
		Slerp(t, q);
	}
	else
	{
		Slerp(t, a);
	}
}
#endif // QUAT_SLERP_NEAR_FQ

#ifndef QUAT_SLERP_NEAR_FQQ
#define QUAT_SLERP_NEAR_FQQ
inline void Quaternion::SlerpNear(float t, const Quaternion &a, const Quaternion &b)
{
	// makes Slerp go the short way
	if(a.Dot(b)<0)
	{
		Quaternion q(b);
		q.Negate();
		FastAssert(a.Dot(q)>-SMALL_FLOAT);
		Slerp(t, a, q);
	}
	else
	{
		Slerp(t, a, b);
	}
}
#endif // QUAT_SLERP_NEAR_FQQ

#ifndef QUAT_ISEQUAL
#define QUAT_ISEQUAL
inline bool Quaternion::IsEqual (const Quaternion& q) const
{
	return (x==q.x && y==q.y && z==q.z && w==q.w);
}
#endif // QUAT_ISEQUAL

#ifndef QUAT_TRANSFORM_VV
#define QUAT_TRANSFORM_VV
inline void Quaternion::Transform(const Vector3& in, Vector3& out) const
{
	FastAssert(&out!=&in);	//lint !e506 constant value boolean
	out.Set(x,y,z);
	out.Scale(2.0f*(x*in.x+y*in.y+z*in.z));
	out.AddScaled(in,2.0f*square(w)-1.0f);
	Vector3 temp(y*in.z-z*in.y,z*in.x-x*in.z,x*in.y-y*in.x);
	out.AddScaled(temp,2.0f*w);
}
#endif // QUAT_TRANSFORM_V

#ifndef QUAT_TRANSFORM_V
#define QUAT_TRANSFORM_V
inline void Quaternion::Transform(Vector3& inout) const
{
	Vector3 in(inout);
	Transform(in,inout);
}
#endif // QUAT_TRANSFORM_V


#ifndef QUAT_UNTRANSFORM_VV
#define QUAT_UNTRANSFORM_VV
inline void Quaternion::UnTransform(const Vector3& in, Vector3& out) const
{
	mthAssertf(&out!=&in, "In and out vectors can't be the same");	//lint !e506 constant value boolean
	out.Set(x,y,z);
	out.Scale(2.0f*(x*in.x+y*in.y+z*in.z));
	out.AddScaled(in,2.0f*square(w)-1.0f);
	Vector3 temp(y*in.z-z*in.y,z*in.x-x*in.z,x*in.y-y*in.x);
	out.SubtractScaled(temp,2.0f*w);
}
#endif // QUAT_UNTRANSFORM_VV


#ifndef QUAT_UNTRANSFORM_V
#define QUAT_UNTRANSFORM_V
inline void Quaternion::UnTransform(Vector3& inout) const
{
	Vector3 in(inout);
	UnTransform(in,inout);
}
#endif // QUAT_UNTRANSFORM_V


} // namespace rage
