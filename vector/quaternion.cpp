//
// vector/quaternion.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#if __SPU
#include "vector/vector3_consts_spu.cpp"
#endif // __SPU

#include "vector/quaternion.h"

#include "data/struct.h"
#include "math/amath.h"
#include "math/simplemath.h"
#include "vector/geometry.h"

using namespace rage;

#if !__SPU
Quaternion Quaternion::sm_I(0.f, 0.f, 0.f, 1.f);
#endif

/********************** Quaternion ***************************/

void Quaternion::Print() const
{
	Printf("%fi,%fj,%fk,%f\n",x,y,z,w);
}


void Quaternion::Print(const char *OUTPUT_ONLY(label)) const
{
	Printf("%s: %fi,%fj,%fk,%f\n",label,x,y,z,w);
}


void Quaternion::Slerp(float p,const Quaternion &q1,const Quaternion &q2)
{
	register float t=0.5f*q1.RelAngle(q2);
	register float st=rage::Sinf(t);

	if(st!=0.0f)
	{
		st=1.0f/st;
		register float pt=p*t;
		register float k1=rage::Sinf(t-pt)*st;
		register float k2=rage::Sinf(pt)*st;

		x=k1*q1.x+k2*q2.x;
		y=k1*q1.y+k2*q2.y;
		z=k1*q1.z+k2*q2.z;
		w=k1*q1.w+k2*q2.w;

		Normalize();
	}
	else
	{
		*this=q1;
	}
}


void Quaternion::Slerp(float p,const Quaternion &q)
{
	register float t=0.5f*RelAngle(q);
	register float st=rage::Sinf(t);

	if(st!=0.0f)
	{
		st=1.0f/st;
		register float pt=p*t;
		register float k1=rage::Sinf(t-pt)*st;
		register float k2=rage::Sinf(pt)*st;

		x=k1*x+k2*q.x;
		y=k1*y+k2*q.y;
		z=k1*z+k2*q.z;
		w=k1*w+k2*q.w;

		Normalize();
	}
}

void Quaternion::FromRotation(const Vector3& rotation)
{
	Vector3 unit(rotation);
	float angle=unit.Mag2();
	if(angle>0.0f)
	{
		angle=sqrtf(angle);
		unit.InvScale(angle);
		FromRotation(unit,angle);
	}
	else
	{
		Identity();
	}
}

float Quaternion::TwistAngle(const Vector3 & v) const
{
	Vector3 tmpV;
	Transform(v, tmpV);

	Quaternion tmpQ;
	tmpQ.FromVectors(tmpV, v);
	tmpQ.Multiply(*this);

	float dot = v.x * tmpQ.x + v.y * tmpQ.y + v.z * tmpQ.z;

	float angle = 2 * rage::Atan2f(dot, tmpQ.w);

	if (angle > PI)
	{
		angle -= 2.0f * PI;
	}
	else if (angle < -PI)
	{
		angle += 2.0f * PI;
	}

	return angle;
}

void Quaternion::TwistSwingDecomp(const Vector3 & twistAxis, Quaternion& twist, Quaternion& swing) const
{
	Vector3 tmp(x, y, z);
	tmp.Normalize();

	float dot = fabsf(twistAxis.Dot(tmp));

	if (fabsf(dot - 1) < SMALL_FLOAT)
	{
		twist = *this;
		swing.Identity();
	}
	else if (dot < SMALL_FLOAT)
	{
		twist.Identity();
		swing = *this;
	}
	else
	{
		Transform(twistAxis, tmp);
		swing.FromVectors(twistAxis, tmp);

		Quaternion inv;
		inv.Inverse(swing);
		twist.Multiply(inv, *this);

/*
		vector3 rotation_axis( orientation.x,
			orientation.y, orientation.z );
		// return projection v1 on to v2 (parallel component)
		// here can be optimized if default_dir is unit
		vector3 proj = projection( rotation_axis, default_dir );
		twist_rotation.set( proj.x, proj.y, proj.z, orientation.w );
		twist_rotation.normalize();
		dir_rotation = orientation * twist_rotation.inverted();
*/

	}
}


#if __DECLARESTRUCT
void Quaternion::DeclareStruct(datTypeStruct &s) 
{
	STRUCT_BEGIN(Quaternion);
	STRUCT_FIELD(x);
	STRUCT_FIELD(y);
	STRUCT_FIELD(z);
	STRUCT_FIELD(w);
	STRUCT_END();
}
#endif
