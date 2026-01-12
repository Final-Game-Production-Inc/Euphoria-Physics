//
// vector/vector3.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "vector3_consts_spu.cpp"

#include "vector3.h"
#include "matrix34.h"

#include "data/struct.h"
#include "qa/qa.h"

namespace rage {

//=============================================================================
// Vector3

#if VECTORIZED_PADDING
CompileTimeAssert(sizeof(Vector3)==16);
#else
CompileTimeAssert(sizeof(Vector3)==12);
#endif // VECTORIZED_PADDING

#if !__SPU
const Vector3 ORIGIN(0.0f,0.0f,0.0f);
const Vector3 XAXIS(1.0f,0.0f,0.0f);
const Vector3 YAXIS(0.0f,1.0f,0.0f);
const Vector3 ZAXIS(0.0f,0.0f,1.0f);
const Vector3 VEC3_IDENTITY(1.0f,1.0f,1.0f);
const Vector3 VEC3_ZERO(0.0f,0.0f,0.0f);
const Vector3 VEC3_HALF(0.5f,0.5f,0.5f);
const Vector3 VEC3_THIRD(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f);
const Vector3 VEC3_QUARTER(0.25f, 0.25f, 0.25f);
const Vector3 VEC3_SQRTTWO(1.41421356237309504880f, 1.41421356237309504880f, 1.41421356237309504880f);
const Vector3 VEC3_SQRTTHREE(1.73205080756887729352f, 1.73205080756887729352f, 1.73205080756887729352f);
const Vector3 VEC3_MAX(FLT_MAX,FLT_MAX,FLT_MAX);
const Vector3 VEC3_SMALL_FLOAT(SMALL_FLOAT,SMALL_FLOAT,SMALL_FLOAT);
const Vector3 VEC3_VERY_SMALL_FLOAT(VERY_SMALL_FLOAT,VERY_SMALL_FLOAT,VERY_SMALL_FLOAT);
const Vector3 VEC3_LARGE_FLOAT(LARGE_FLOAT,LARGE_FLOAT,LARGE_FLOAT);

Vector3 g_UnitUp(YAXIS);

void SetUnitUp (const Vector3& unitUp)
{
	g_UnitUp.Set(unitUp);
}
#endif

#if (__XENON || __PPU ) && VECTORIZED
#if !__SPU
__vector4 __VECTOR4_ANDX = {0.0f, allBitsF, allBitsF, allBitsF};
__vector4 __VECTOR4_ANDY = {allBitsF, 0.0f, allBitsF, allBitsF};
__vector4 __VECTOR4_ANDZ = {allBitsF, allBitsF, 0.0f, allBitsF};
__vector4 __VECTOR4_ALLBITS = {allBitsF, allBitsF, allBitsF, 0.0f};
__vector4 __VECTOR4_MASKX = {allBitsF, 0.0f, 0.0f, 0.0f};
__vector4 __VECTOR4_MASKY = {0.0f, allBitsF, 0.0f, 0.0f};
__vector4 __VECTOR4_MASKZ = {0.0f, 0.0f, allBitsF, 0.0f};
__vector4 __VECTOR4_MASKW = {0.0f, 0.0f, 0.0f, allBitsF};
__vector4 __VECTOR4_MASKXYZW = {allBitsF, allBitsF,allBitsF, allBitsF};
__vector4 __VECTOR4_ONLYW = {0.0f, 0.0f, 0.0f, 1.0f};

__vector4 __VECTOR4_ZERO = {0.0f, 0.0f, 0.0f, 0.0f};
__vector4 __VECTOR4_ONE = {1.0f, 1.0f, 1.0f, 1.0f};
__vector4 __VECTOR4_HALF = {0.5f, 0.5f, 0.5f, 0.5f};
__vector4 __VECTOR4_INF = {finf, finf, finf, finf};
__vector4 __VECTOR4_NAN = {fnan, fnan, fnan, fnan};

const Vector3 VEC3_ANDX(__VECTOR4_ANDX);
const Vector3 VEC3_ANDY(__VECTOR4_ANDY);
const Vector3 VEC3_ANDZ(__VECTOR4_ANDZ);
const Vector3 VEC3_ANDW(__VECTOR4_ALLBITS);
const Vector3 VEC3_MASKX(__VECTOR4_MASKX);
const Vector3 VEC3_MASKY(__VECTOR4_MASKY);
const Vector3 VEC3_MASKZ(__VECTOR4_MASKZ);
const Vector3 VEC3_MASKW(__VECTOR4_MASKW);
const Vector3 VEC3_MASKXYZW(__VECTOR4_MASKXYZW);
const Vector3 VEC3_ONEW(__VECTOR4_ONLYW);
const Vector3 VEC3_INF(__VECTOR4_INF);
const Vector3 VEC3_NAN(__VECTOR4_NAN);
#endif
#elif !__SPU
const Vector3 VEC3_ANDX(0.0f, allBitsF, allBitsF, allBitsF);
const Vector3 VEC3_ANDY(allBitsF, 0.0f, allBitsF, allBitsF);
const Vector3 VEC3_ANDZ(allBitsF, allBitsF, 0.0f, allBitsF);
const Vector3 VEC3_ANDW(allBitsF, allBitsF, allBitsF, 0.0f);
const Vector3 VEC3_MASKX(allBitsF, 0.0f, 0.0f, 0.0f);
const Vector3 VEC3_MASKY(0.0f, allBitsF, 0.0f, 0.0f);
const Vector3 VEC3_MASKZ(0.0f, 0.0f, allBitsF, 0.0f);
const Vector3 VEC3_MASKW(0.0f, 0.0f, 0.0f, allBitsF);
const Vector3 VEC3_MASKXYZW(allBitsF, allBitsF, allBitsF, allBitsF);
const Vector3 VEC3_ONEW(0.0f, 0.0f, 0.0f, 1.0f);
const Vector3 VEC3_INF(finf, finf, finf, finf);
const Vector3 VEC3_NAN(fnan, fnan, fnan, fnan);
#endif


Vector3::Vector3(const Vector2 & v2d, Vector2::eVector3Axes axes)
{
	switch(axes)
	{
		case Vector2::kXY:
			x = v2d.x;
			y = v2d.y;
			z = 0.0f;
			break;
		case Vector2::kXZ:
			x = v2d.x;
			y = 0.0f;
			z = v2d.y;
			break;
		case Vector2::kYZ:
			x = 0.0f;
			y = v2d.x;
			z = v2d.y;
			break;
		default:
			mthErrorf("Invalid axis selector %d", axes);
	}
}

#if !__NO_OUTPUT
void Vector3::Print(bool newline) const
{
	Printf("%f,%f,%f",x,y,z);
	if (newline)
	{
		Printf("\n");
	}
}


void Vector3::Print(const char *label, bool newline) const
{
	Printf("%s: %f,%f,%f",label,x,y,z);
	if (newline)
	{
		Printf("\n");
	}
}
#else
void Vector3::Print(bool ) const { }
void Vector3::Print(const char *, bool ) const {}
#endif


bool Vector3::ApproachStraight(const Vector3 &goal,float rate,float time)
{
	float rx, ry, rz;

	float dx, dy, dz;
	dx = goal.x - x;
	dy = goal.y - y;
	dz = goal.z - z;

	float ax, ay, az;
	ax = fabsf(dx);
	ay = fabsf(dy);
	az = fabsf(dz);

	if (dx==0.0f && dy==0.0f && dz==0.0f)
	{
		return true;
	}
	else if (ax>=ay && ax>=az)
	{
		// dx is the largest
		float yx = dy / dx;
		float zx = dz / dx;
		rx = rate / sqrtf(1.0f + square(yx) + square(zx));
		ry = fabsf(yx * rx);
		rz = fabsf(zx * rx);
	}
	else if (ay>=ax && ay>=az)
	{
		// dy is the largest
		float xy = dx / dy;
		float zy = dz / dy;
		ry = rate / sqrtf(1.0f + square(xy) + square(zy));
		rx = fabsf(xy * ry);
		rz = fabsf(zy * ry);
	}
	else
	{
		// dz is the largest
		float xz = dx / dz;
		float yz = dy / dz;
		rz = rate / sqrtf(1.0f + square(xz) + square(yz));
		rx = fabsf(xz * rz);
		ry = fabsf(yz * rz);
	}

	return(Approach(x,goal.x,rx,time)&Approach(y,goal.y,ry,time)&Approach(z,goal.z,rz,time));
}


void Vector3::RotateAboutAxis(float radians, int axis)
{
	float t, tsin, tcos;
	tsin=sinf(radians);
	tcos=cosf(radians);
	if(axis=='z') {
		t = x * tcos - y * tsin;
		y = x * tsin + y * tcos;
		x = t;
	}
	else if(axis=='x') {
		t = y * tcos - z * tsin;
		z = y * tsin + z * tcos;
		y = t;
	}
	else if (axis=='y') {
		t = z * tcos - x * tsin;
		x = z * tsin + x * tcos;
		z = t;
	}
}


void Vector3::MakeOrthonormals (Vector3& ortho1, Vector3& ortho2) const
{
	// Make sure this vector has a length near 1.
	DebugAssert(fabsf(Mag2()-1.0f)<0.01f);

	// See which coordinate axis is most parallel to this vector.
	if (fabsf(x)>=fabsf(y))
	{
		// This vector is as close or closer to the x axis than to the y axis, so it must be closest to either x or z.
		// Make the second unit axis vector equal to this cross y.
		ortho1.Set(-z,0.0f,x);
	}
	else
	{
		// This vector is closer to the y axis than to the x axis, so it must be closest to either y or z.
		// Make the second unit axis vector equal to this cross x.
		ortho1.Set(0.0f,z,-y);
	}

	// Normalize the second unit axis vector.
	ortho1.Normalize();
	DebugAssert(fabsf(ortho1.Mag2()-1.0f)<0.01f);

	// Make the third unit axis vector the cross product of the first two.
	ortho2.Cross(*this,ortho1);
	DebugAssert(fabsf(ortho2.Mag2()-1.0f)<0.01f);
}


void Vector3::GetVector2(int axis, Vector2& vec) const {
	switch(axis) {
	case 0:
		GetVector2YZ(vec);
		break;
	case 1:
		GetVector2ZY(vec);
		break;
	case 2:
		GetVector2ZX(vec);
		break;
	case 3:
		GetVector2XZ(vec);
		break;
	case 4:
		GetVector2XY(vec);
		break;
	case 5:
		GetVector2YX(vec);
		break;
	default:
		break;
	}
}


void Vector3::GetPolar(const Vector3& lookTo, Vector3& outPolarCoords, Vector3& outPolarOffset) const 
{
	Vector3 V = (*this) - lookTo;
	float AZ = atan2f(V.x, V.z);
	float dist = fabsf(Dist(lookTo));
	float Inc = acosf(sqrtf(V.x*V.x + V.z*V.z)/dist);

	outPolarCoords.Set(dist, AZ, Inc);
	outPolarOffset = lookTo;
}


void Vector3::Extend(float distance)
{
	float mag=Mag();
	Scale((mag+distance)/mag);
}


void Vector3::Extend(const Vector3& v, float distance)
{
	float mag=v.Mag();
	Scale(v,(mag+distance)/mag);
}


void Vector3::ReflectAbout(const Vector3& normal)
{
	float thisMag=1.0f/Mag();
	float mag2=1.0f/normal.Mag2();
	x=thisMag*(x-2.0f*x*normal.x*normal.x*mag2);
	y=thisMag*(y-2.0f*y*normal.y*normal.y*mag2);
	z=thisMag*(z-2.0f*z*normal.z*normal.z*mag2);	
}


void Vector3::ReflectAboutFast(const Vector3& normal)
{
	x=x-2.0f*x*normal.x*normal.x;
	y=y-2.0f*y*normal.y*normal.y;
	z=z-2.0f*z*normal.z*normal.z;	
}


void Vector3::AddNet (Vector3Param add)
{
	Vector3 modifiedAdd(add);
	float dot = Dot(modifiedAdd);
	if (dot>VERY_SMALL_FLOAT)
	{
		float mag2 = Mag2();
		if(mag2>VERY_SMALL_FLOAT)
		{
			// The given vector has a positive dot product with this vector, so remove its parallel component.
			modifiedAdd.SubtractScaled(*this,dot/Mag2());
		}
	}

	// Add the given vector to this vector, less any positive parallel component.
	Add(modifiedAdd);
}


void Vector3::FindCloseAngleXYZ(Vector3 &ioR)
{
	Vector3 r1, r2;
	r1.Set(ioR.x, ioR.y, ioR.z);
	r1.FindAlternateXYZ(r2);

	// Find the solution in range [-PI, PI] of the newEuler.
	float x1 = r1.x + floorf((x - r1.x + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);
	float y1 = r1.y + floorf((y - r1.y + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);
	float z1 = r1.z + floorf((z - r1.z + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);

	// Find the alternate solution in range [-PI, PI] of the newEuler.
	float x2 = r2.x + floorf((x - r2.x + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);
	float y2 = r2.y + floorf((y - r2.y + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);
	float z2 = r2.z + floorf((z - r2.z + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);

	// Compare E1 with E2 to determine which one is close to this one.
	if( (fabs(x1-x) + fabs(y1-y) + fabs(z1-z)) < (fabs(x2-x) + fabs(y2-y) + fabs(z2-z)) )
		ioR.Set(x1, y1, z1);
	else
		ioR.Set(x2, y2, z2);
}

void Vector3::FindCloseAngleXZY(Vector3 &ioR)
{
	Vector3 r1, r2;
	r1.Set(ioR.x, ioR.y, ioR.z);
	r1.FindAlternateXZY(r2);

	// Find the solution in range [-PI, PI] of the newEuler.
	float x1 = r1.x + floorf((x - r1.x + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);
	float y1 = r1.y + floorf((y - r1.y + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);
	float z1 = r1.z + floorf((z - r1.z + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);

	// Find the alternate solution in range [-PI, PI] of the newEuler.
	float x2 = r2.x + floorf((x - r2.x + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);
	float y2 = r2.y + floorf((y - r2.y + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);
	float z2 = r2.z + floorf((z - r2.z + PI) * (1.0f/(2.0f*PI))) * (2.0f*PI);

	// Compare E1 with E2 to determine which one is close to this one.
	if( (fabs(x1-x) + fabs(y1-y) + fabs(z1-z)) < (fabs(x2-x) + fabs(y2-y) + fabs(z2-z)) )
		ioR.Set(x1, y1, z1);
	else
		ioR.Set(x2, y2, z2);
}

#if __DECLARESTRUCT
void Vector3::DeclareStruct(datTypeStruct &s) {
	STRUCT_BEGIN(Vector3);
	STRUCT_FIELD(x);
	STRUCT_FIELD(y);
	STRUCT_FIELD(z);
# if VECTORIZED_PADDING
	STRUCT_FIELD(w);
# endif
	STRUCT_END();
}
#endif

} // namespace rage

using namespace rage;

#if __QA

#ifndef IS_COMBINED
inline bool FloatEqual(float fVal1, float fVal2, float eps = SMALL_FLOAT)
{
	float fTest = fVal1 - fVal2;
	return (fTest < eps && fTest > -eps);
}

inline bool VectorEqual(const Vector3& vec, float fVal1, float fVal2, float fVal3, float eps = SMALL_FLOAT)
{
	return (FloatEqual(vec.x, fVal1, eps) && FloatEqual(vec.y, fVal2, eps) && FloatEqual(vec.z, fVal3, eps));
}
#endif

inline bool VectorBitwiseEqual(const Vector3& vec, float fVal1, float fVal2, float fVal3)
{
	union { 
		struct { u32 x, y, z; } u;
		struct { f32 x, y, z; } f;
	} uvec, uval;
	uvec.f.x = vec.x;
	uvec.f.y = vec.y;
	uvec.f.z = vec.z;
	uval.f.x = fVal1;
	uval.f.y = fVal2;
	uval.f.z = fVal3;
	return uvec.u.x == uval.u.x && uvec.u.y == uval.u.y && uvec.u.z == uval.u.z;
}

#define V3QA_ITEM_BEGIN(Name)					\
class qa##Name : public qaItem					\
{ public:										\
	void Init() {};								\
	void Update(qaResult& result)

#define V3QA_ITEM_END(Name)			};

V3QA_ITEM_BEGIN(V3T_ScaleF)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	test.Scale(3.5f);
	bool bPass = VectorEqual(test, 3.5f, 7.0f, 10.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_ScaleF);


V3QA_ITEM_BEGIN(V3T_ScaleVF)
{
	Vector3 test;
	Vector3 scale(1.0f, 2.0f, 3.0f);
	test.Scale(scale, 3.5f);
	bool bPass = VectorEqual(test, 3.5f, 7.0f, 10.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_ScaleVF);
V3QA_ITEM_BEGIN(V3T_InvScaleF)
{
	Vector3 test(15.6f, 12.9f, 9.0f);
	test.InvScale(3.0f);
	bool bPass = VectorEqual(test, 5.2f, 4.3f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvScaleF);
V3QA_ITEM_BEGIN(V3T_InvScaleV)
{
	Vector3 test(15.6f, 12.9f, 9.0f);
	Vector3 scalar(3.0f, 3.0f, 3.0f);
	test.InvScale(scalar);
	bool bPass = VectorEqual(test, 5.2f, 4.3f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvScaleV);
V3QA_ITEM_BEGIN(V3T_InvScaleVF)
{
	Vector3 test;
	Vector3 scale(15.6f, 12.9f, 9.0f);
	test.InvScale(scale, 3.0f);
	bool bPass = VectorEqual(test, 5.2f, 4.3f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvScaleVF);
V3QA_ITEM_BEGIN(V3T_InvScaleVV)
{
	Vector3 test;
	Vector3 scale(15.6f, 12.9f, 9.0f);
	Vector3 scalar(3.0f, 3.0f, 3.0f);
	test.InvScale(scale, scalar);
	bool bPass = VectorEqual(test, 5.2f, 4.3f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvScaleVV);
V3QA_ITEM_BEGIN(V3T_AddF)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	test.Add(4.0f, 5.0f, 6.0f);
	bool bPass = VectorEqual(test, 5.0f, 7.0f, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AddF);
V3QA_ITEM_BEGIN(V3T_AddV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Add(test2);
	bool bPass = VectorEqual(test, 5.0f, 7.0f, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AddV);
V3QA_ITEM_BEGIN(V3T_AddVV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Add(test1, test2);
	bool bPass = VectorEqual(test, 5.0f, 7.0f, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AddVV);
V3QA_ITEM_BEGIN(V3T_AddScaledF)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.AddScaled(test2, 0.5f);
	bool bPass = VectorEqual(test, 3.0f, 4.5f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AddScaledF);
V3QA_ITEM_BEGIN(V3T_AddScaledV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.AddScaled(test2, scalar);
	bool bPass = VectorEqual(test, 3.0f, 4.5f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AddScaledV);
V3QA_ITEM_BEGIN(V3T_AddScaledVF)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.AddScaled(test1, test2, 0.5f);
	bool bPass = VectorEqual(test, 3.0f, 4.5f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AddScaledVF);
V3QA_ITEM_BEGIN(V3T_AddScaledVV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.AddScaled(test1, test2, scalar);
	bool bPass = VectorEqual(test, 3.0f, 4.5f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AddScaledVV);
V3QA_ITEM_BEGIN(V3T_SubtractF)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	test.Subtract(4.0f, 5.0f, 6.0f);
	bool bPass = VectorEqual(test, -3.0f, -3.0f, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_SubtractF);
V3QA_ITEM_BEGIN(V3T_SubtractV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Subtract(test2);
	bool bPass = VectorEqual(test, -3.0f, -3.0f, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_SubtractV);
V3QA_ITEM_BEGIN(V3T_SubtractVV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Subtract(test1, test2);
	bool bPass = VectorEqual(test, -3.0f, -3.0f, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_SubtractVV);
V3QA_ITEM_BEGIN(V3T_SubtractScaledF)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.SubtractScaled(test2, 0.5f);
	bool bPass = VectorEqual(test, -1.0f, -0.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_SubtractScaledF);
V3QA_ITEM_BEGIN(V3T_SubtractScaledV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.SubtractScaled(test2, scalar);
	bool bPass = VectorEqual(test, -1.0f, -0.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_SubtractScaledV);
V3QA_ITEM_BEGIN(V3T_SubtractScaledVF)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.SubtractScaled(test1, test2, 0.5f);
	bool bPass = VectorEqual(test, -1.0f, -0.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_SubtractScaledVF);
V3QA_ITEM_BEGIN(V3T_SubtractScaledVV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.SubtractScaled(test1, test2, scalar);
	bool bPass = VectorEqual(test, -1.0f, -0.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_SubtractScaledVV);
V3QA_ITEM_BEGIN(V3T_MultiplyV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Multiply(test2);
	bool bPass = VectorEqual(test, 4.0f, 10.0f, 18.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_MultiplyV);
V3QA_ITEM_BEGIN(V3T_MultiplyVV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Multiply(test1, test2);
	bool bPass = VectorEqual(test, 4.0f, 10.0f, 18.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_MultiplyVV);
V3QA_ITEM_BEGIN(V3T_Negate)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	test.Negate();
	bool bPass = VectorEqual(test, -1.0f, -2.0f, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Negate);
V3QA_ITEM_BEGIN(V3T_NegateV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	test.Negate(test1);
	bool bPass = VectorEqual(test, -1.0f, -2.0f, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_NegateV);
V3QA_ITEM_BEGIN(V3T_Abs)
{
	Vector3 test(-1.0f, 2.0f, -3.0f);
	test.Abs();
	bool bPass = VectorEqual(test, 1.0f, 2.0f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Abs);
V3QA_ITEM_BEGIN(V3T_AbsV)
{
	Vector3 test;
	Vector3 test1(-1.0f, 2.0f, -3.0f);
	test.Abs(test1);
	bool bPass = VectorEqual(test, 1.0f, 2.0f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AbsV);
V3QA_ITEM_BEGIN(V3T_Invert)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	test.Invert();
	bool bPass = VectorEqual(test, 1.0f, 0.5f, 0.33333333333f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Invert);
V3QA_ITEM_BEGIN(V3T_InvertV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	test.Invert(test1);
	bool bPass = VectorEqual(test, 1.0f, 0.5f, 0.33333333333f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvertV);
V3QA_ITEM_BEGIN(V3T_InvertSafe)
{
	Vector3 test(1.0f, 2.0f, 0.0f);
	test.InvertSafe();
	bool bPass = VectorEqual(test, 1.0f, 0.5f, FLT_MAX);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvertSafe);
V3QA_ITEM_BEGIN(V3T_InvertSafeV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 0.0f);
	test.InvertSafe(test1);
	bool bPass = VectorEqual(test, 1.0f, 0.5f, FLT_MAX);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvertSafeV);
V3QA_ITEM_BEGIN(V3T_Normalize)
{
	Vector3 test(12.0f, 12.0f, 0.0f);
	test.Normalize();
	bool bPass = VectorEqual(test, 0.70710678f, 0.70710678f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Normalize);
V3QA_ITEM_BEGIN(V3T_NormalizeFast)
{
	Vector3 test(12.0f, 12.0f, 0.0f);
	test.NormalizeFast();
	bool bPass = VectorEqual(test, 0.70710678f, 0.70710678f, 0.0f, 0.001f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_NormalizeFast);
V3QA_ITEM_BEGIN(V3T_NormalizeSafe)
{
	Vector3 test(0.0f, 0.0f, 0.0f);
	test.NormalizeSafe();
	bool bPass = VectorEqual(test, 1.0f, 0.0f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_NormalizeSafe);
V3QA_ITEM_BEGIN(V3T_NormalizeV)
{
	Vector3 test;
	Vector3 test1(12.0f, 12.0f, 0.0f);
	test.Normalize(test1);
	bool bPass = VectorEqual(test, 0.70710678f, 0.70710678f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_NormalizeV);
V3QA_ITEM_BEGIN(V3T_NormalizeFastV)
{
	Vector3 test;
	Vector3 test1(12.0f, 12.0f, 0.0f);
	test.NormalizeFast(test1);
	bool bPass = VectorEqual(test, 0.70710678f, 0.70710678f, 0.0f, 0.001f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_NormalizeFastV);
V3QA_ITEM_BEGIN(V3T_Dot)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	float fDot = test.Dot(test2);
	bool bPass = FloatEqual(fDot, 32.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Dot);
V3QA_ITEM_BEGIN(V3T_DotV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 vDot = test.DotV(test2);
	bool bPass = VectorEqual(vDot, 32.0f, 32.0f, 32.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_DotV);
V3QA_ITEM_BEGIN(V3T_FlatDot)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	float fDot = test.FlatDot(test2);
	bool bPass = FloatEqual(fDot, 22.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_FlatDot);
V3QA_ITEM_BEGIN(V3T_Cross)
{
	Vector3 test(1.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.Cross(test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Cross);
V3QA_ITEM_BEGIN(V3T_CrossV)
{
	Vector3 test;
	Vector3 test1(1.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.Cross(test1, test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_CrossV);
V3QA_ITEM_BEGIN(V3T_CrossSafe)
{
	Vector3 test(1.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.CrossSafe(test, test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_CrossSafe);
V3QA_ITEM_BEGIN(V3T_CrossNegate)
{
	Vector3 test(1.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.CrossNegate(test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, -1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_CrossNegate);
V3QA_ITEM_BEGIN(V3T_CrossX)
{
	Vector3 test(0.0f, 1.0f, 2.0f);
	Vector3 test2(3.0f, 4.0f, 5.0f);
	float cross = test.CrossX(test2);
	bool bPass = FloatEqual(cross, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_CrossX);
V3QA_ITEM_BEGIN(V3T_CrossY)
{
	Vector3 test(0.0f, 1.0f, 2.0f);
	Vector3 test2(3.0f, 4.0f, 5.0f);
	float cross = test.CrossY(test2);
	bool bPass = FloatEqual(cross, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_CrossY);
V3QA_ITEM_BEGIN(V3T_CrossZ)
{
	Vector3 test(0.0f, 1.0f, 2.0f);
	Vector3 test2(3.0f, 4.0f, 5.0f);
	float cross = test.CrossZ(test2);
	bool bPass = FloatEqual(cross, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_CrossZ);
V3QA_ITEM_BEGIN(V3T_AddCrossed)
{
	Vector3 test(7.0f, 8.0f, 9.0f);
	Vector3 test1(1.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.AddCrossed(test1, test2);
	bool bPass = VectorEqual(test, 7.0f, 8.0f, 10.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AddCrossed);
V3QA_ITEM_BEGIN(V3T_SubtractCrossed)
{
	Vector3 test(7.0f, 8.0f, 9.0f);
	Vector3 test1(1.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.SubtractCrossed(test1, test2);
	bool bPass = VectorEqual(test, 7.0f, 8.0f, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_SubtractCrossed);
V3QA_ITEM_BEGIN(V3T_Average)
{
	Vector3 test(0.0f, 1.0f, 2.0f);
	Vector3 test2(3.0f, 4.0f, 5.0f);
	test.Average(test2);
	bool bPass = VectorEqual(test, 1.5f, 2.5f, 3.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Average);
V3QA_ITEM_BEGIN(V3T_AverageV)
{
	Vector3 test;
	Vector3 test1(0.0f, 1.0f, 2.0f);
	Vector3 test2(3.0f, 4.0f, 5.0f);
	test.Average(test1, test2);
	bool bPass = VectorEqual(test, 1.5f, 2.5f, 3.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_AverageV);
V3QA_ITEM_BEGIN(V3T_LerpFV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Lerp(0.5f, test1, test2);
	bool bPass = VectorEqual(test, 2.5f, 3.5f, 4.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_LerpFV);
V3QA_ITEM_BEGIN(V3T_LerpVV)
{
	Vector3 test;
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.Lerp(scalar, test1, test2);
	bool bPass = VectorEqual(test, 2.5f, 3.5f, 4.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_LerpVV);
V3QA_ITEM_BEGIN(V3T_LerpF)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test.Lerp(0.5f, test2);
	bool bPass = VectorEqual(test, 2.5f, 3.5f, 4.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_LerpF);
V3QA_ITEM_BEGIN(V3T_LerpV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 scalar(0.5f, 0.5f, 0.5f);
	test.Lerp(scalar, test2);
	bool bPass = VectorEqual(test, 2.5f, 3.5f, 4.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_LerpV);
V3QA_ITEM_BEGIN(V3T_Mag)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(20.0f, 0.0f, 0.0f);
	float ftest1 = test1.Mag();
	float ftest2 = test2.Mag();
	float ftest3 = test3.Mag();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 20.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Mag);
V3QA_ITEM_BEGIN(V3T_MagV)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(20.0f, 0.0f, 0.0f);
	Vector3 ftest1 = test1.MagV();
	Vector3 ftest2 = test2.MagV();
	Vector3 ftest3 = test3.MagV();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 20.0f, 20.0f, 20.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_MagV);
V3QA_ITEM_BEGIN(V3T_InvMag)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(20.0f, 0.0f, 0.0f);
	float ftest1 = test1.InvMag();
	float ftest2 = test2.InvMag();
	float ftest3 = test3.InvMag();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 0.05f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvMag);
V3QA_ITEM_BEGIN(V3T_InvMagV)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(20.0f, 0.0f, 0.0f);
	Vector3 ftest1 = test1.InvMagV();
	Vector3 ftest2 = test2.InvMagV();
	Vector3 ftest3 = test3.InvMagV();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 0.05f, 0.05f, 0.05f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvMagV);
V3QA_ITEM_BEGIN(V3T_InvMagFast)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(20.0f, 0.0f, 0.0f);
	float ftest1 = test1.InvMagFast();
	float ftest2 = test2.InvMagFast();
	float ftest3 = test3.InvMagFast();
#if __XENON
	// Im special casing this on Xenon since this is a fast function, the resulting INF is valid
	u32 utest1 = *(u32*)&ftest1;
	bool bPass = (utest1 == 0x7f800000);
#else
	bool bPass = FloatEqual(ftest1, 0.0f); 
#endif
	bPass = bPass && FloatEqual(ftest2, 1.0f, 0.001f) && FloatEqual(ftest3, 0.05f, 0.001f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvMagFast);
V3QA_ITEM_BEGIN(V3T_InvMagFastV)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(20.0f, 0.0f, 0.0f);
	Vector3 ftest1 = test1.InvMagFastV();
	Vector3 ftest2 = test2.InvMagFastV();
	Vector3 ftest3 = test3.InvMagFastV();
#if __XENON
	float rsqrtZeroResult = finf;
#else
	float rsqrtZeroResult = 0.0f;
#endif
	bool bPass = (VectorBitwiseEqual(ftest1, rsqrtZeroResult, rsqrtZeroResult, rsqrtZeroResult) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f, 0.001f) && VectorEqual(ftest3, 0.05f, 0.05f, 0.05f, 0.001f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvMagFastV);
V3QA_ITEM_BEGIN(V3T_Mag2)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(20.0f, 0.0f, 0.0f);
	float ftest1 = test1.Mag2();
	float ftest2 = test2.Mag2();
	float ftest3 = test3.Mag2();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 400.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Mag2);
V3QA_ITEM_BEGIN(V3T_Mag2V)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(20.0f, 0.0f, 0.0f);
	Vector3 ftest1 = test1.Mag2V();
	Vector3 ftest2 = test2.Mag2V();
	Vector3 ftest3 = test3.Mag2V();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 400.0f, 400.0f, 400.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Mag2V);
V3QA_ITEM_BEGIN(V3T_FlatMag)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(3.5355339f, 20.0f, 3.5355339f);
	float ftest1 = test1.FlatMag();
	float ftest2 = test2.FlatMag();
	float ftest3 = test3.FlatMag();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 0.0f) && FloatEqual(ftest3, 5.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_FlatMag);
V3QA_ITEM_BEGIN(V3T_FlatMag2)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(3.5355339f, 20.0f, 3.5355339f);
	float ftest1 = test1.FlatMag2();
	float ftest2 = test2.FlatMag2();
	float ftest3 = test3.FlatMag2();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 0.0f) && FloatEqual(ftest3, 25.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_FlatMag2);
V3QA_ITEM_BEGIN(V3T_XYMag)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(3.5355339f, 3.5355339f, 3.5355339f);
	float ftest1 = test1.XYMag();
	float ftest2 = test2.XYMag();
	float ftest3 = test3.XYMag();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 5.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_XYMag);
V3QA_ITEM_BEGIN(V3T_XYMag2)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(3.5355339f, 3.5355339f, 3.5355339f);
	float ftest1 = test1.XYMag2();
	float ftest2 = test2.XYMag2();
	float ftest3 = test3.XYMag2();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 25.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_XYMag2);
V3QA_ITEM_BEGIN(V3T_Dist)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.7320508f, 3.7320508f, 4.7320508f);
	float dist = test.Dist(test2);
	bool bPass = FloatEqual(dist, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Dist);
V3QA_ITEM_BEGIN(V3T_DistV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.7320508f, 3.7320508f, 4.7320508f);
	Vector3 dist = test.DistV(test2);
	bool bPass = VectorEqual(dist, 3.0f, 3.0f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_DistV);
V3QA_ITEM_BEGIN(V3T_InvDist)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.7320508f, 3.7320508f, 4.7320508f);
	float dist = test.InvDist(test2);
	bool bPass = FloatEqual(dist, 0.33333333333f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvDist);
V3QA_ITEM_BEGIN(V3T_InvDistV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.7320508f, 3.7320508f, 4.7320508f);
	Vector3 dist = test.InvDistV(test2);
	bool bPass = VectorEqual(dist, 0.33333333333f, 0.33333333333f, 0.33333333333f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvDistV);
V3QA_ITEM_BEGIN(V3T_Dist2)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.7320508f, 3.7320508f, 4.7320508f);
	float dist = test.Dist2(test2);
	bool bPass = FloatEqual(dist, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Dist2);
V3QA_ITEM_BEGIN(V3T_Dist2V)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.7320508f, 3.7320508f, 4.7320508f);
	Vector3 dist = test.Dist2V(test2);
	bool bPass = VectorEqual(dist, 9.0f, 9.0f, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Dist2V);
V3QA_ITEM_BEGIN(V3T_InvDist2)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.7320508f, 3.7320508f, 4.7320508f);
	float dist = test.InvDist2(test2);
	bool bPass = FloatEqual(dist, 0.111111111111f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvDist2);
V3QA_ITEM_BEGIN(V3T_InvDist2V)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.7320508f, 3.7320508f, 4.7320508f);
	Vector3 dist = test.InvDist2V(test2);
	bool bPass = VectorEqual(dist, 0.111111111111f, 0.111111111111f, 0.111111111111f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_InvDist2V);
V3QA_ITEM_BEGIN(V3T_FlatDist)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.5355339f, 5.5355339f, 6.5355339f);
	float dist = test.XZDist(test2);
	bool bPass = FloatEqual(dist, 5.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_FlatDist);
V3QA_ITEM_BEGIN(V3T_FlatDist2)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.5355339f, 5.5355339f, 6.5355339f);
	float dist = test.XZDist2(test2);
	bool bPass = FloatEqual(dist, 25.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_FlatDist2);

V3QA_ITEM_BEGIN(V3T_FlatDistV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.5355339f, 5.5355339f, 6.5355339f);
	Vector3 dist = test.FlatDistV(test2);
	bool bPass = FloatEqual(dist.x, 5.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_FlatDistV);
V3QA_ITEM_BEGIN(V3T_FlatDist2V)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.5355339f, 5.5355339f, 6.5355339f);
	Vector3 dist = test.FlatDist2V(test2);
	bool bPass = FloatEqual(dist.x, 25.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_FlatDist2V);

V3QA_ITEM_BEGIN(V3T_ClampMag)
{
//	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(100.0f, 100.0f, 100.0f);
//	test1.ClampMag(1.0f, 100.0f);
	test2.ClampMag(1.0f, 100.0f);
	test3.ClampMag(1.0f, 100.0f);
	bool bPass = (/*VectorEqual(test1, 0.0f, 1.0f, 0.0f) &&*/ VectorEqual(test2, 1.0f, 1.0f, 1.0f) && VectorEqual(test3, 57.735027f, 57.735027f, 57.735027f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_ClampMag);
V3QA_ITEM_BEGIN(V3T_IsZero)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	bool bPass = (test1.IsZero() && !test2.IsZero());
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsZero);
V3QA_ITEM_BEGIN(V3T_IsNonZero)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	bool bPass = (!test1.IsNonZero() && test2.IsNonZero());
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsNonZero);
V3QA_ITEM_BEGIN(V3T_IsEqual)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(1.0f, 1.0f, 1.0f);
	bool bPass = (!test1.IsEqual(test2) && test2.IsEqual(test3));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsEqual);
V3QA_ITEM_BEGIN(V3T_IsNotEqual)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(1.0f, 1.0f, 1.0f);
	bool bPass = (test1.IsNotEqual(test2) && !test2.IsNotEqual(test3));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsNotEqual);
V3QA_ITEM_BEGIN(V3T_IsClose)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.9999999999f, 0.9999999999f, 0.9999999999f);
	bool bPass = (!test1.IsClose(test2, SMALL_FLOAT) && test2.IsClose(test3, SMALL_FLOAT));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsClose);
V3QA_ITEM_BEGIN(V3T_IsCloseV)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.9999999999f, 0.9999999999f, 0.9999999999f);
	Vector3 range(SMALL_FLOAT, SMALL_FLOAT, SMALL_FLOAT);
	bool bPass = (!test1.IsClose(test2, range) && test2.IsClose(test3, range));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsCloseV);
V3QA_ITEM_BEGIN(V3T_IsGreaterThan)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.9999999f, 0.9999999f, 0.9999999f);
	bool bTest1 = test1.IsGreaterThan(test2);
	bool bTest2 = test2.IsGreaterThan(test3);
	bool bTest3 = test2.IsGreaterThan(test2);
	bool bPass = (!bTest1 && bTest2 && !bTest3);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsGreaterThan);
V3QA_ITEM_BEGIN(V3T_IsGreaterThanV)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.9999999f, 0.9999999f, 0.9999999f);
	test1 = test1.IsGreaterThanV(test2);
	test3 = test2.IsGreaterThanV(test3);
	test2 = test2.IsGreaterThanV(test2);
	bool bPass = (VectorEqual(test1, 0.0f, 0.0f, 0.0f) && VectorEqual(test2, 0.0f, 0.0f, 0.0f) && VectorBitwiseEqual(test3, allBitsF, allBitsF, allBitsF));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsGreaterThanV);
V3QA_ITEM_BEGIN(V3T_IsGreaterThanVR)
{
	u32 r1, r2, r3;
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.9999999f, 0.9999999f, 0.9999999f);
	test1 = test1.IsGreaterThanVR(test2, r1);
	test3 = test2.IsGreaterThanVR(test3, r2);
	test2 = test2.IsGreaterThanVR(test2, r3);
	bool bPass = (VectorEqual(test1, 0.0f, 0.0f, 0.0f) && VectorEqual(test2, 0.0f, 0.0f, 0.0f) && VectorBitwiseEqual(test3, allBitsF, allBitsF, allBitsF));
	bPass = bPass && ((r1 & VEC3_CMP_VAL) == 0) && ((r3 & VEC3_CMP_VAL) == 0) && ((r2 & VEC3_CMP_VAL) == VEC3_CMP_VAL);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsGreaterThanVR);
V3QA_ITEM_BEGIN(V3T_IsLessThan)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.9999999f, 0.9999999f, 0.9999999f);
	bool bPass = (test1.IsLessThanDoNotUse(test2) && !test2.IsLessThanDoNotUse(test3) && !test2.IsLessThanDoNotUse(test2));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsLessThan);
V3QA_ITEM_BEGIN(V3T_IsLessThanV)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.9999999f, 0.9999999f, 0.9999999f);
	test1 = test1.IsLessThanV(test2);
	test3 = test2.IsLessThanV(test3);
	test2 = test2.IsLessThanV(test2);
	bool bPass = (VectorBitwiseEqual(test1, allBitsF, allBitsF, allBitsF) && VectorEqual(test2, 0.0f, 0.0f, 0.0f) && VectorEqual(test3, 0.0f, 0.0f, 0.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsLessThanV);
V3QA_ITEM_BEGIN(V3T_IsLessThanVR)
{
	u32 r1, r2, r3;
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.9999999f, 0.9999999f, 0.9999999f);
	test1 = test1.IsLessThanVR(test2, r1);
	test3 = test2.IsLessThanVR(test3, r2);
	test2 = test2.IsLessThanVR(test2, r3);
	bool bPass = (VectorBitwiseEqual(test1, allBitsF, allBitsF, allBitsF) && VectorEqual(test2, 0.0f, 0.0f, 0.0f) && VectorEqual(test3, 0.0f, 0.0f, 0.0f));
	bPass = bPass && ((r1 & VEC3_CMP_VAL) == VEC3_CMP_VAL) && ((r2 & VEC3_CMP_VAL) == 0) && ((r3 & VEC3_CMP_VAL) == 0);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_IsLessThanVR);
V3QA_ITEM_BEGIN(V3T_Select)
{
	Vector3 test1(0.0f, 0.0f, 0.0f);
	Vector3 test2(1.0f, 1.0f, 1.0f);
	Vector3 test3(0.5f, 0.5f, 0.5f);
	Vector3 sel1 = test1.Select(test2, test3);
	Vector3 sel2 = test2.Select(test1, test3);
	bool bPass = VectorEqual(sel1, 1.0f, 1.0f, 1.0f) && VectorEqual(sel2, 0.5f, 0.5f, 0.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_Select);
V3QA_ITEM_BEGIN(V3T_operatorPlus)
{
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 test = test1 + test2;
	bool bPass = VectorEqual(test, 5.0f, 7.0f, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorPlus);
V3QA_ITEM_BEGIN(V3T_operatorMinus)
{
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 test = test1 - test2;
	bool bPass = VectorEqual(test, -3.0f, -3.0f, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorMinus);
V3QA_ITEM_BEGIN(V3T_operatorNeg)
{
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test = -test1;
	bool bPass = VectorEqual(test, -1.0f, -2.0f, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorNeg);
V3QA_ITEM_BEGIN(V3T_operatorTimesF)
{
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test = test1 * 0.5f;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorTimesF);
V3QA_ITEM_BEGIN(V3T_operatorTimesV)
{
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	Vector3 test = test1 * test2;
	bool bPass = VectorEqual(test, 4.0f, 10.0f, 18.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorTimesV);
V3QA_ITEM_BEGIN(V3T_operatorDivF)
{
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test = test1 / 2.0f;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorDivF);
V3QA_ITEM_BEGIN(V3T_operatorDivFV)
{
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.0f, 2.0f, 2.0f);
	Vector3 test = test1 / test2;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorDivFV);
V3QA_ITEM_BEGIN(V3T_operatorPlusEq)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test += test2;
	bool bPass = VectorEqual(test, 5.0f, 7.0f, 9.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorPlusEq);
V3QA_ITEM_BEGIN(V3T_operatorMinusEq)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test -= test2;
	bool bPass = VectorEqual(test, -3.0f, -3.0f, -3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorMinusEq);
V3QA_ITEM_BEGIN(V3T_operatorTimesEqF)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	test *= 0.5f;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorTimesEqF);
V3QA_ITEM_BEGIN(V3T_operatorTimesEqV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(4.0f, 5.0f, 6.0f);
	test *= test2;
	bool bPass = VectorEqual(test, 4.0f, 10.0f, 18.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorTimesEqV);
V3QA_ITEM_BEGIN(V3T_operatorDivEqF)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	test /= 2.0f;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorDivEqF);
V3QA_ITEM_BEGIN(V3T_operatorDivEqV)
{
	Vector3 test(1.0f, 2.0f, 3.0f);
	Vector3 test2(2.0f, 2.0f, 2.0f);
	test /= test2;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V3QA_ITEM_END(V3T_operatorDivEqV);

QA_ITEM_FAMILY(qaV3T_ScaleF, (), ());
QA_ITEM_FAST(qaV3T_ScaleF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_ScaleVF, (), ());
QA_ITEM_FAST(qaV3T_ScaleVF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvScaleF, (), ());
QA_ITEM_FAST(qaV3T_InvScaleF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvScaleV, (), ());
QA_ITEM_FAST(qaV3T_InvScaleV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvScaleVF, (), ());
QA_ITEM_FAST(qaV3T_InvScaleVF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvScaleVV, (), ());
QA_ITEM_FAST(qaV3T_InvScaleVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AddF, (), ());
QA_ITEM_FAST(qaV3T_AddF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AddV, (), ());
QA_ITEM_FAST(qaV3T_AddV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AddVV, (), ());
QA_ITEM_FAST(qaV3T_AddVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AddScaledF, (), ());
QA_ITEM_FAST(qaV3T_AddScaledF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AddScaledV, (), ());
QA_ITEM_FAST(qaV3T_AddScaledV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AddScaledVF, (), ());
QA_ITEM_FAST(qaV3T_AddScaledVF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AddScaledVV, (), ());
QA_ITEM_FAST(qaV3T_AddScaledVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_SubtractF, (), ());
QA_ITEM_FAST(qaV3T_SubtractF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_SubtractV, (), ());
QA_ITEM_FAST(qaV3T_SubtractV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_SubtractVV, (), ());
QA_ITEM_FAST(qaV3T_SubtractVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_SubtractScaledF, (), ());
QA_ITEM_FAST(qaV3T_SubtractScaledF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_SubtractScaledV, (), ());
QA_ITEM_FAST(qaV3T_SubtractScaledV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_SubtractScaledVF, (), ());
QA_ITEM_FAST(qaV3T_SubtractScaledVF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_SubtractScaledVV, (), ());
QA_ITEM_FAST(qaV3T_SubtractScaledVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_MultiplyV, (), ());
QA_ITEM_FAST(qaV3T_MultiplyV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_MultiplyVV, (), ());
QA_ITEM_FAST(qaV3T_MultiplyVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Negate, (), ());
QA_ITEM_FAST(qaV3T_Negate, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_NegateV, (), ());
QA_ITEM_FAST(qaV3T_NegateV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Abs, (), ());
QA_ITEM_FAST(qaV3T_Abs, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AbsV, (), ());
QA_ITEM_FAST(qaV3T_AbsV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Invert, (), ());
QA_ITEM_FAST(qaV3T_Invert, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvertV, (), ());
QA_ITEM_FAST(qaV3T_InvertV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvertSafe, (), ());
QA_ITEM_FAST(qaV3T_InvertSafe, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvertSafeV, (), ());
QA_ITEM_FAST(qaV3T_InvertSafeV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Normalize, (), ());
QA_ITEM_FAST(qaV3T_Normalize, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_NormalizeFast, (), ());
QA_ITEM_FAST(qaV3T_NormalizeFast, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_NormalizeSafe, (), ());
QA_ITEM_FAST(qaV3T_NormalizeSafe, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_NormalizeV, (), ());
QA_ITEM_FAST(qaV3T_NormalizeV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_NormalizeFastV, (), ());
QA_ITEM_FAST(qaV3T_NormalizeFastV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Dot, (), ());
QA_ITEM_FAST(qaV3T_Dot, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_DotV, (), ());
QA_ITEM_FAST(qaV3T_DotV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_FlatDot, (), ());
QA_ITEM_FAST(qaV3T_FlatDot, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Cross, (), ());
QA_ITEM_FAST(qaV3T_Cross, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_CrossV, (), ());
QA_ITEM_FAST(qaV3T_CrossV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_CrossSafe, (), ());
QA_ITEM_FAST(qaV3T_CrossSafe, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_CrossNegate, (), ());
QA_ITEM_FAST(qaV3T_CrossNegate, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_CrossX, (), ());
QA_ITEM_FAST(qaV3T_CrossX, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_CrossY, (), ());
QA_ITEM_FAST(qaV3T_CrossY, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_CrossZ, (), ());
QA_ITEM_FAST(qaV3T_CrossZ, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AddCrossed, (), ());
QA_ITEM_FAST(qaV3T_AddCrossed, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_SubtractCrossed, (), ());
QA_ITEM_FAST(qaV3T_SubtractCrossed, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Average, (), ());
QA_ITEM_FAST(qaV3T_Average, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_AverageV, (), ());
QA_ITEM_FAST(qaV3T_AverageV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_LerpFV, (), ());
QA_ITEM_FAST(qaV3T_LerpFV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_LerpVV, (), ());
QA_ITEM_FAST(qaV3T_LerpVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_LerpF, (), ());
QA_ITEM_FAST(qaV3T_LerpF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_LerpV, (), ());
QA_ITEM_FAST(qaV3T_LerpV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Mag, (), ());
QA_ITEM_FAST(qaV3T_Mag, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_MagV, (), ());
QA_ITEM_FAST(qaV3T_MagV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvMag, (), ());
QA_ITEM_FAST(qaV3T_InvMag, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvMagV, (), ());
QA_ITEM_FAST(qaV3T_InvMagV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvMagFast, (), ());
QA_ITEM_FAST(qaV3T_InvMagFast, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvMagFastV, (), ());
QA_ITEM_FAST(qaV3T_InvMagFastV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Mag2, (), ());
QA_ITEM_FAST(qaV3T_Mag2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Mag2V, (), ());
QA_ITEM_FAST(qaV3T_Mag2V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_FlatMag, (), ());
QA_ITEM_FAST(qaV3T_FlatMag, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_FlatMag2, (), ());
QA_ITEM_FAST(qaV3T_FlatMag2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_XYMag, (), ());
QA_ITEM_FAST(qaV3T_XYMag, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_XYMag2, (), ());
QA_ITEM_FAST(qaV3T_XYMag2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Dist, (), ());
QA_ITEM_FAST(qaV3T_Dist, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_DistV, (), ());
QA_ITEM_FAST(qaV3T_DistV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvDist, (), ());
QA_ITEM_FAST(qaV3T_InvDist, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvDistV, (), ());
QA_ITEM_FAST(qaV3T_InvDistV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Dist2, (), ());
QA_ITEM_FAST(qaV3T_Dist2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Dist2V, (), ());
QA_ITEM_FAST(qaV3T_Dist2V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvDist2, (), ());
QA_ITEM_FAST(qaV3T_InvDist2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_InvDist2V, (), ());
QA_ITEM_FAST(qaV3T_InvDist2V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_FlatDist, (), ());
QA_ITEM_FAST(qaV3T_FlatDist, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_FlatDist2, (), ());
QA_ITEM_FAST(qaV3T_FlatDist2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_ClampMag, (), ());
QA_ITEM_FAST(qaV3T_ClampMag, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsZero, (), ());
QA_ITEM_FAST(qaV3T_IsZero, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsNonZero, (), ());
QA_ITEM_FAST(qaV3T_IsNonZero, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsEqual, (), ());
QA_ITEM_FAST(qaV3T_IsEqual, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsNotEqual, (), ());
QA_ITEM_FAST(qaV3T_IsNotEqual, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsClose, (), ());
QA_ITEM_FAST(qaV3T_IsClose, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsCloseV, (), ());
QA_ITEM_FAST(qaV3T_IsCloseV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsGreaterThan, (), ());
QA_ITEM_FAST(qaV3T_IsGreaterThan, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsGreaterThanV, (), ());
QA_ITEM_FAST(qaV3T_IsGreaterThanV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsGreaterThanVR, (), ());
QA_ITEM_FAST(qaV3T_IsGreaterThanVR, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsLessThan, (), ());
QA_ITEM_FAST(qaV3T_IsLessThan, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsLessThanV, (), ());
QA_ITEM_FAST(qaV3T_IsLessThanV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_IsLessThanVR, (), ());
QA_ITEM_FAST(qaV3T_IsLessThanVR, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_Select, (), ());
QA_ITEM_FAST(qaV3T_Select, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorPlus, (), ());
QA_ITEM_FAST(qaV3T_operatorPlus, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorMinus, (), ());
QA_ITEM_FAST(qaV3T_operatorMinus, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorNeg, (), ());
QA_ITEM_FAST(qaV3T_operatorNeg, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorTimesF, (), ());
QA_ITEM_FAST(qaV3T_operatorTimesF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorTimesV, (), ());
QA_ITEM_FAST(qaV3T_operatorTimesV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorDivF, (), ());
QA_ITEM_FAST(qaV3T_operatorDivF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorDivFV, (), ());
QA_ITEM_FAST(qaV3T_operatorDivFV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorPlusEq, (), ());
QA_ITEM_FAST(qaV3T_operatorPlusEq, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorMinusEq, (), ());
QA_ITEM_FAST(qaV3T_operatorMinusEq, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorTimesEqF, (), ());
QA_ITEM_FAST(qaV3T_operatorTimesEqF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorTimesEqV, (), ());
QA_ITEM_FAST(qaV3T_operatorTimesEqV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorDivEqF, (), ());
QA_ITEM_FAST(qaV3T_operatorDivEqF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV3T_operatorDivEqV, (), ());
QA_ITEM_FAST(qaV3T_operatorDivEqV, (), qaResult::FAIL_OR_TOTAL_TIME);

#endif // __QA
