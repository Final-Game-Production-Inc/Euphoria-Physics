//
// vector/vector4.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "vector4.h"
#include "data/struct.h"
#include "qa/qa.h"

using namespace rage;

/* Purpose: A vector with the value of the coordinate system's origin. */
const Vector4 rage::VECTOR4_ORIGIN(0.0f,0.0f,0.0f,0.0f);
const Vector4 rage::VECTOR4_IDENTITY(1.0f, 1.0f, 1.0f, 1.0f);
const Vector4 rage::VEC4_HALF(0.5f, 0.5f, 0.5f, 0.5f);
const Vector4 rage::VEC4_ANDW(allBitsF, allBitsF, allBitsF, 0.0f);
const Vector4 rage::VEC4_ONEW(0.0f, 0.0f, 0.0f, 1.0f);
const Vector4 rage::VEC4_255(255.0f, 255.0f, 255.0f, 255.0f);
const Vector4 rage::VEC4_MASKW(0.0f, 0.0f, 0.0f, allBitsF);

/******************** Vector4 ************************/

void Vector4::Print(bool newline) const {
	if( newline )
		Printf("%f,%f,%f,%f\n",x,y,z,w);
	else
		Printf("%f,%f,%f,%f",x,y,z,w);
}	

void Vector4::Print(const char *OUTPUT_ONLY(label), bool newline) const {
	if( newline )
		Printf("%s: %f,%f,%f,%f\n",label,x,y,z,w);
	else
		Printf("%s: %f,%f,%f,%f",label,x,y,z,w);
}

#if __DECLARESTRUCT
void Vector4::DeclareStruct(datTypeStruct &s) {
	STRUCT_BEGIN(Vector4);
	STRUCT_FIELD(x);
	STRUCT_FIELD(y);
	STRUCT_FIELD(z);
	STRUCT_FIELD(w);
	STRUCT_END();
}
#endif

#if __QA

#ifndef IS_COMBINED
inline bool FloatEqual(float fVal1, float fVal2, float eps = SMALL_FLOAT)
{
	float fTest = fVal1 - fVal2;
	return (fTest < eps && fTest > -eps);
}
#endif

inline bool VectorEqual(const Vector4& vec, float fVal1, float fVal2, float fVal3, float fVal4, float eps = SMALL_FLOAT)
{
	return (FloatEqual(vec.x, fVal1, eps) && FloatEqual(vec.y, fVal2, eps) && FloatEqual(vec.z, fVal3, eps) && FloatEqual(vec.w, fVal4, eps));
}

inline bool VectorBitwiseEqual(const Vector4& vec, float fVal1, float fVal2, float fVal3, float fVal4)
{
	union { 
		struct { u32 x, y, z, w; } u;
		struct { f32 x, y, z, w; } f;
	} uvec, uval;
	uvec.f.x = vec.x;
	uvec.f.y = vec.y;
	uvec.f.z = vec.z;
	uvec.f.w = vec.w;
	uval.f.x = fVal1;
	uval.f.y = fVal2;
	uval.f.z = fVal3;
	uval.f.w = fVal4;
	return uvec.u.x == uval.u.x && uvec.u.y == uval.u.y && uvec.u.z == uval.u.z && uvec.u.w == uval.u.w;
}

#define V4QA_ITEM_BEGIN(Name)					\
class qa##Name : public qaItem					\
{ public:										\
	void Init() {};								\
	void Update(qaResult& result)

#define V4QA_ITEM_END(Name)			};

V4QA_ITEM_BEGIN(V4T_GetX)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	float x = test.GetX();
	bool bPass = FloatEqual(x, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_GetX)
V4QA_ITEM_BEGIN(V4T_GetY)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	float x = test.GetY();
	bool bPass = FloatEqual(x, 2.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_GetY)
V4QA_ITEM_BEGIN(V4T_GetZ)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	float x = test.GetZ();
	bool bPass = FloatEqual(x, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_GetZ)
V4QA_ITEM_BEGIN(V4T_GetW)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	float x = test.GetW();
	bool bPass = FloatEqual(x, 4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_GetW)
V4QA_ITEM_BEGIN(V4T_SetX)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	test.SetX(3.5f);
	bool bPass = VectorEqual(test, 3.5f, 0.0f, 0.0f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SetX)
V4QA_ITEM_BEGIN(V4T_SetY)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	test.SetY(3.5f);
	bool bPass = VectorEqual(test, 0.0f, 3.5f, 0.0f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SetY)
V4QA_ITEM_BEGIN(V4T_SetZ)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	test.SetZ(3.5f);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 3.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SetZ)
V4QA_ITEM_BEGIN(V4T_SetW)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	test.SetW(3.5f);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 0.0f, 3.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SetW)
V4QA_ITEM_BEGIN(V4T_SplatX)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	test.SplatX(test1);
	bool bPass = VectorEqual(test, 1.0f, 1.0f, 1.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SplatX)
V4QA_ITEM_BEGIN(V4T_SplatY)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	test.SplatY(test1);
	bool bPass = VectorEqual(test, 2.0f, 2.0f, 2.0f, 2.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SplatY)
V4QA_ITEM_BEGIN(V4T_SplatZ)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	test.SplatZ(test1);
	bool bPass = VectorEqual(test, 3.0f, 3.0f, 3.0f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SplatZ)
V4QA_ITEM_BEGIN(V4T_SplatW)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	test.SplatW(test1);
	bool bPass = VectorEqual(test, 4.0f, 4.0f, 4.0f, 4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SplatW)
V4QA_ITEM_BEGIN(V4T_ScaleF)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	test.Scale(3.5f);
	bool bPass = VectorEqual(test, 3.5f, 7.0f, 10.5f, 14.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_ScaleF);
V4QA_ITEM_BEGIN(V4T_ScaleVF)
{
	Vector4 test;
	Vector4 scale(1.0f, 2.0f, 3.0f, 4.0f);
	test.Scale(scale, 3.5f);
	bool bPass = VectorEqual(test, 3.5f, 7.0f, 10.5f, 14.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_ScaleVF);
V4QA_ITEM_BEGIN(V4T_Scale3F)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	test.Scale3(3.5f);
	bool bPass = VectorEqual(test, 3.5f, 7.0f, 10.5f, 4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Scale3F);
V4QA_ITEM_BEGIN(V4T_Scale3VF)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 scale(1.0f, 2.0f, 3.0f, 4.0f);
	test.Scale3(scale, 3.5f);
	bool bPass = VectorEqual(test, 3.5f, 7.0f, 10.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Scale3VF);
V4QA_ITEM_BEGIN(V4T_InvScaleF)
{
	Vector4 test(15.6f, 12.9f, 9.0f, 5.4f);
	test.InvScale(3.0f);
	bool bPass = VectorEqual(test, 5.2f, 4.3f, 3.0f, 1.8f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvScaleF);
V4QA_ITEM_BEGIN(V4T_InvScaleV)
{
	Vector4 test(15.6f, 12.9f, 9.0f, 5.4f);
	Vector4 scalar(3.0f, 3.0f, 3.0f, 3.0f);
	test.InvScale(scalar);
	bool bPass = VectorEqual(test, 5.2f, 4.3f, 3.0f, 1.8f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvScaleV);
V4QA_ITEM_BEGIN(V4T_InvScaleVF)
{
	Vector4 test;
	Vector4 scale(15.6f, 12.9f, 9.0f, 5.4f);
	test.InvScale(scale, 3.0f);
	bool bPass = VectorEqual(test, 5.2f, 4.3f, 3.0f, 1.8f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvScaleVF);
V4QA_ITEM_BEGIN(V4T_InvScaleVV)
{
	Vector4 test;
	Vector4 scale(15.6f, 12.9f, 9.0f, 5.4f);
	Vector4 scalar(3.0f, 3.0f, 3.0f, 3.0f);
	test.InvScale(scale, scalar);
	bool bPass = VectorEqual(test, 5.2f, 4.3f, 3.0f, 1.8f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvScaleVV);
V4QA_ITEM_BEGIN(V4T_AddF)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	test.Add(5.0f, 6.0f, 7.0f, 8.0f);
	bool bPass = VectorEqual(test, 6.0f, 8.0f, 10.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddF);
V4QA_ITEM_BEGIN(V4T_AddV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.Add(test2);
	bool bPass = VectorEqual(test, 6.0f, 8.0f, 10.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddV);
V4QA_ITEM_BEGIN(V4T_AddVV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.Add(test1, test2);
	bool bPass = VectorEqual(test, 6.0f, 8.0f, 10.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddVV);
V4QA_ITEM_BEGIN(V4T_AddVV3)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(5.0f, 6.0f, 7.0f);
	test.Add(test1, test2);
	bool bPass = VectorEqual(test, 6.0f, 8.0f, 10.0f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddVV3);
V4QA_ITEM_BEGIN(V4T_AddScaledF)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.AddScaled(test2, 0.5f);
	bool bPass = VectorEqual(test, 3.5f, 5.0f, 6.5f, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddScaledF);
V4QA_ITEM_BEGIN(V4T_AddScaledV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 scalar(0.5f, 0.5f, 0.5f, 0.5f);
	test.AddScaled(test2, scalar);
	bool bPass = VectorEqual(test, 3.5f, 5.0f, 6.5f, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddScaledV);
V4QA_ITEM_BEGIN(V4T_AddScaledVF)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.AddScaled(test1, test2, 0.5f);
	bool bPass = VectorEqual(test, 3.5f, 5.0f, 6.5f, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddScaledVF);
V4QA_ITEM_BEGIN(V4T_AddScaledVV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 scalar(0.5f, 0.5f, 0.5f, 0.5f);
	test.AddScaled(test1, test2, scalar);
	bool bPass = VectorEqual(test, 3.5f, 5.0f, 6.5f, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddScaledVV);
V4QA_ITEM_BEGIN(V4T_AddScaled3F)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.AddScaled3(test2, 0.5f);
	bool bPass = VectorEqual(test, 3.5f, 5.0f, 6.5f, 4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddScaled3F);
V4QA_ITEM_BEGIN(V4T_AddScaled3V)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 scalar(0.5f, 0.5f, 0.5f, 0.5f);
	test.AddScaled3(test2, scalar);
	bool bPass = VectorEqual(test, 3.5f, 5.0f, 6.5f, 4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddScaled3V);
V4QA_ITEM_BEGIN(V4T_AddScaled3VF)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.AddScaled3(test1, test2, 0.5f);
	bool bPass = VectorEqual(test, 3.5f, 5.0f, 6.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddScaled3VF);
V4QA_ITEM_BEGIN(V4T_AddScaled3VV)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 scalar(0.5f, 0.5f, 0.5f, 0.5f);
	test.AddScaled3(test1, test2, scalar);
	bool bPass = VectorEqual(test, 3.5f, 5.0f, 6.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AddScaled3VV);
V4QA_ITEM_BEGIN(V4T_SubtractF)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	test.Subtract(5.0f, 6.0f, 7.0f, 8.0f);
	bool bPass = VectorEqual(test, -4.0f, -4.0f, -4.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SubtractF);
V4QA_ITEM_BEGIN(V4T_SubtractV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.Subtract(test2);
	bool bPass = VectorEqual(test, -4.0f, -4.0f, -4.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SubtractV);
V4QA_ITEM_BEGIN(V4T_SubtractVV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.Subtract(test1, test2);
	bool bPass = VectorEqual(test, -4.0f, -4.0f, -4.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SubtractVV);
V4QA_ITEM_BEGIN(V4T_SubtractVV3)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector3 test1(1.0f, 2.0f, 3.0f);
	Vector3 test2(5.0f, 6.0f, 7.0f);
	test.Subtract(test1, test2);
	bool bPass = VectorEqual(test, -4.0f, -4.0f, -4.0f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SubtractVV3);
V4QA_ITEM_BEGIN(V4T_SubtractScaledF)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.SubtractScaled(test2, 0.5f);
	bool bPass = VectorEqual(test, -1.5f, -1.0f, -0.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SubtractScaledF);
V4QA_ITEM_BEGIN(V4T_SubtractScaledV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 scalar(0.5f, 0.5f, 0.5f, 0.5f);
	test.SubtractScaled(test2, scalar);
	bool bPass = VectorEqual(test, -1.5f, -1.0f, -0.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SubtractScaledV);
V4QA_ITEM_BEGIN(V4T_SubtractScaledVF)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.SubtractScaled(test1, test2, 0.5f);
	bool bPass = VectorEqual(test, -1.5f, -1.0f, -0.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SubtractScaledVF);
V4QA_ITEM_BEGIN(V4T_SubtractScaledVV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 scalar(0.5f, 0.5f, 0.5f, 0.5f);
	test.SubtractScaled(test1, test2, scalar);
	bool bPass = VectorEqual(test, -1.5f, -1.0f, -0.5f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SubtractScaledVV);
V4QA_ITEM_BEGIN(V4T_MultiplyV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.Multiply(test2);
	bool bPass = VectorEqual(test, 5.0f, 12.0f, 21.0f, 32.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_MultiplyV);
V4QA_ITEM_BEGIN(V4T_MultiplyVV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.Multiply(test1, test2);
	bool bPass = VectorEqual(test, 5.0f, 12.0f, 21.0f, 32.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_MultiplyVV);
V4QA_ITEM_BEGIN(V4T_Negate)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	test.Negate();
	bool bPass = VectorEqual(test, -1.0f, -2.0f, -3.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Negate);
V4QA_ITEM_BEGIN(V4T_NegateV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	test.Negate(test1);
	bool bPass = VectorEqual(test, -1.0f, -2.0f, -3.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_NegateV);
V4QA_ITEM_BEGIN(V4T_Abs)
{
	Vector4 test(-1.0f, 2.0f, -3.0f, 4.0f);
	test.Abs();
	bool bPass = VectorEqual(test, 1.0f, 2.0f, 3.0f, 4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Abs);
V4QA_ITEM_BEGIN(V4T_AbsV)
{
	Vector4 test;
	Vector4 test1(-1.0f, 2.0f, -3.0f, 4.0f);
	test.Abs(test1);
	bool bPass = VectorEqual(test, 1.0f, 2.0f, 3.0f, 4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AbsV);
V4QA_ITEM_BEGIN(V4T_Invert)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	test.Invert();
	bool bPass = VectorEqual(test, 1.0f, 0.5f, 0.33333333333f, 0.25f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Invert);
V4QA_ITEM_BEGIN(V4T_InvertV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	test.Invert(test1);
	bool bPass = VectorEqual(test, 1.0f, 0.5f, 0.33333333333f, 0.25f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvertV);
V4QA_ITEM_BEGIN(V4T_InvertSafe)
{
	Vector4 test(1.0f, 2.0f, 0.0f, 4.0f);
	test.InvertSafe();
	bool bPass = VectorEqual(test, 1.0f, 0.5f, FLT_MAX, 0.25f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvertSafe);
V4QA_ITEM_BEGIN(V4T_InvertSafeV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 0.0f, 4.0f);
	test.InvertSafe(test1);
	bool bPass = VectorEqual(test, 1.0f, 0.5f, FLT_MAX, 0.25f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvertSafeV);
V4QA_ITEM_BEGIN(V4T_Normalize)
{
	Vector4 test(12.0f, 12.0f, 0.0f, 12.0f);
	test.Normalize();
	bool bPass = VectorEqual(test, 0.57735027f, 0.57735027f, 0.0f, 0.57735027f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Normalize);
V4QA_ITEM_BEGIN(V4T_NormalizeFast)
{
	Vector4 test(12.0f, 12.0f, 0.0f, 12.0f);
	test.NormalizeFast();
	bool bPass = VectorEqual(test, 0.57735027f, 0.57735027f, 0.0f, 0.57735027f, 0.001f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_NormalizeFast);
V4QA_ITEM_BEGIN(V4T_NormalizeV)
{
	Vector4 test;
	Vector4 test1(12.0f, 12.0f, 0.0f, 12.0f);
	test.Normalize(test1);
	bool bPass = VectorEqual(test, 0.57735027f, 0.57735027f, 0.0f, 0.57735027f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_NormalizeV);
V4QA_ITEM_BEGIN(V4T_NormalizeFastV)
{
	Vector4 test;
	Vector4 test1(12.0f, 12.0f, 0.0f, 12.0f);
	test.NormalizeFast(test1);
	bool bPass = VectorEqual(test, 0.57735027f, 0.57735027f, 0.0f, 0.57735027f, 0.001f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_NormalizeFastV);
V4QA_ITEM_BEGIN(V4T_Normalize3)
{
	Vector4 test(12.0f, 12.0f, 0.0f, 12.0f);
	test.Normalize3();
	bool bPass = VectorEqual(test, 0.70710678f, 0.70710678f, 0.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Normalize3);
V4QA_ITEM_BEGIN(V4T_NormalizeFast3)
{
	Vector4 test(12.0f, 12.0f, 0.0f, 12.0f);
	test.NormalizeFast3();
	bool bPass = VectorEqual(test, 0.70710678f, 0.70710678f, 0.0f, 12.0f, 0.001f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_NormalizeFast3);
V4QA_ITEM_BEGIN(V4T_NormalizeV3)
{
	Vector4 test;
	Vector4 test1(12.0f, 12.0f, 0.0f, 12.0f);
	test.Normalize3(test1);
	bool bPass = VectorEqual(test, 0.70710678f, 0.70710678f, 0.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_NormalizeV3);
V4QA_ITEM_BEGIN(V4T_NormalizeFastV3)
{
	Vector4 test;
	Vector4 test1(12.0f, 12.0f, 0.0f, 12.0f);
	test.NormalizeFast3(test1);
	bool bPass = VectorEqual(test, 0.70710678f, 0.70710678f, 0.0f, 12.0f, 0.001f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_NormalizeFastV3);
V4QA_ITEM_BEGIN(V4T_Dot)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float fDot = test.Dot(test2);
	bool bPass = FloatEqual(fDot, 70.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dot);
V4QA_ITEM_BEGIN(V4T_DotV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 vDot = test.DotV(test2);
	bool bPass = VectorEqual(vDot, 70.0f, 70.0f, 70.0f, 70.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_DotV);
V4QA_ITEM_BEGIN(V4T_Dot3)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float fDot = test.Dot3(test2);
	bool bPass = FloatEqual(fDot, 38.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dot3);
V4QA_ITEM_BEGIN(V4T_Dot33)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector3 test2(5.0f, 6.0f, 7.0f);
	float fDot = test.Dot3(test2);
	bool bPass = FloatEqual(fDot, 38.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dot33);
V4QA_ITEM_BEGIN(V4T_Dot3V)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 vDot = test.Dot3V(test2);
	bool bPass = VectorEqual(vDot, 38.0f, 38.0f, 38.0f, 38.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dot3V);
V4QA_ITEM_BEGIN(V4T_Cross)
{
	Vector4 test(1.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	test.Cross(test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Cross);
V4QA_ITEM_BEGIN(V4T_Cross3)
{
	Vector4 test(1.0f, 0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.Cross(test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Cross3);
V4QA_ITEM_BEGIN(V4T_CrossV33)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector3 test1(1.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.Cross(test1, test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_CrossV33);
V4QA_ITEM_BEGIN(V4T_CrossV43)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test1(1.0f, 0.0f, 0.0f, 0.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.Cross(test1, test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_CrossV43);
V4QA_ITEM_BEGIN(V4T_CrossV34)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector3 test1(1.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	test.Cross(test1, test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_CrossV34);
V4QA_ITEM_BEGIN(V4T_CrossV44)
{
	Vector4 test(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test1(1.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	test.Cross(test1, test2);
	bool bPass = VectorEqual(test, 0.0f, 0.0f, 1.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_CrossV44);
V4QA_ITEM_BEGIN(V4T_Average)
{
	Vector4 test(0.0f, 1.0f, 2.0f, 3.0f);
	Vector4 test2(4.0f, 5.0f, 6.0f, 7.0f);
	test.Average(test2);
	bool bPass = VectorEqual(test, 2.0f, 3.0f, 4.0f, 5.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Average);
V4QA_ITEM_BEGIN(V4T_AverageV)
{
	Vector4 test;
	Vector4 test1(0.0f, 1.0f, 2.0f, 3.0f);
	Vector4 test2(4.0f, 5.0f, 6.0f, 7.0f);
	test.Average(test1, test2);
	bool bPass = VectorEqual(test, 2.0f, 3.0f, 4.0f, 5.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_AverageV);
V4QA_ITEM_BEGIN(V4T_LerpFV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.Lerp(0.5f, test1, test2);
	bool bPass = VectorEqual(test, 3.0f, 4.0f, 5.0f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_LerpFV);
V4QA_ITEM_BEGIN(V4T_LerpVV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 scalar(0.5f, 0.5f, 0.5f, 0.5f);
	test.Lerp(scalar, test1, test2);
	bool bPass = VectorEqual(test, 3.0f, 4.0f, 5.0f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_LerpVV);
V4QA_ITEM_BEGIN(V4T_LerpF)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.Lerp(0.5f, test2);
	bool bPass = VectorEqual(test, 3.0f, 4.0f, 5.0f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_LerpF);
V4QA_ITEM_BEGIN(V4T_LerpV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 scalar(0.5f, 0.5f, 0.5f, 0.5f);
	test.Lerp(scalar, test2);
	bool bPass = VectorEqual(test, 3.0f, 4.0f, 5.0f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_LerpV);
V4QA_ITEM_BEGIN(V4T_SqrtV)
{
	Vector4 test1(0.0f, 1.0f, 4.0f, 9.0f);
	Vector4 test = test1.SqrtV();
	bool bPass = VectorEqual(test, 0.0f, 1.0f, 2.0f, 3.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_SqrtV);
V4QA_ITEM_BEGIN(V4T_Mag)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	float ftest1 = test1.Mag();
	float ftest2 = test2.Mag();
	float ftest3 = test3.Mag();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 28.2842712f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Mag);
V4QA_ITEM_BEGIN(V4T_MagV)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	Vector4 ftest1 = test1.MagV();
	Vector4 ftest2 = test2.MagV();
	Vector4 ftest3 = test3.MagV();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 28.2842712f, 28.2842712f, 28.2842712f, 28.2842712f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_MagV);
V4QA_ITEM_BEGIN(V4T_Mag2)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	float ftest1 = test1.Mag2();
	float ftest2 = test2.Mag2();
	float ftest3 = test3.Mag2();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 800.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Mag2);
V4QA_ITEM_BEGIN(V4T_Mag2V)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	Vector4 ftest1 = test1.Mag2V();
	Vector4 ftest2 = test2.Mag2V();
	Vector4 ftest3 = test3.Mag2V();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 800.0f, 800.0f, 800.0f, 800.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Mag2V);
V4QA_ITEM_BEGIN(V4T_Mag3)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	float ftest1 = test1.Mag3();
	float ftest2 = test2.Mag3();
	float ftest3 = test3.Mag3();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 20.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Mag3);
V4QA_ITEM_BEGIN(V4T_Mag3V)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	Vector4 ftest1 = test1.Mag3V();
	Vector4 ftest2 = test2.Mag3V();
	Vector4 ftest3 = test3.Mag3V();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 20.0f, 20.0f, 20.0f, 20.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Mag3V);
V4QA_ITEM_BEGIN(V4T_Mag32)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	float ftest1 = test1.Mag32();
	float ftest2 = test2.Mag32();
	float ftest3 = test3.Mag32();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 400.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Mag32);
V4QA_ITEM_BEGIN(V4T_Mag32V)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	Vector4 ftest1 = test1.Mag32V();
	Vector4 ftest2 = test2.Mag32V();
	Vector4 ftest3 = test3.Mag32V();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 400.0f, 400.0f, 400.0f, 400.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Mag32V);
V4QA_ITEM_BEGIN(V4T_InvMag)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	float ftest1 = test1.InvMag();
	float ftest2 = test2.InvMag();
	float ftest3 = test3.InvMag();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 0.035355339f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvMag);
V4QA_ITEM_BEGIN(V4T_InvMagV)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	Vector4 ftest1 = test1.InvMagV();
	Vector4 ftest2 = test2.InvMagV();
	Vector4 ftest3 = test3.InvMagV();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 0.035355339f, 0.035355339f, 0.035355339f, 0.035355339f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvMagV);
V4QA_ITEM_BEGIN(V4T_InvMag3)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	float ftest1 = test1.InvMag3();
	float ftest2 = test2.InvMag3();
	float ftest3 = test3.InvMag3();
	bool bPass = (FloatEqual(ftest1, 0.0f) && FloatEqual(ftest2, 1.0f) && FloatEqual(ftest3, 0.05f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvMag3);
V4QA_ITEM_BEGIN(V4T_InvMag3V)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 test3(20.0f, 0.0f, 0.0f, 20.0f);
	Vector4 ftest1 = test1.InvMag3V();
	Vector4 ftest2 = test2.InvMag3V();
	Vector4 ftest3 = test3.InvMag3V();
	bool bPass = (VectorEqual(ftest1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(ftest2, 1.0f, 1.0f, 1.0f, 1.0f) && VectorEqual(ftest3, 0.05f, 0.05f, 0.05f, 0.05f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvMag3V);
V4QA_ITEM_BEGIN(V4T_Dist)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float dist = test.Dist(test2);
	bool bPass = FloatEqual(dist, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dist);
V4QA_ITEM_BEGIN(V4T_DistV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 dist = test.DistV(test2);
	bool bPass = VectorEqual(dist, 8.0f, 8.0f, 8.0f, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_DistV);
V4QA_ITEM_BEGIN(V4T_InvDist)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float dist = test.InvDist(test2);
	bool bPass = FloatEqual(dist, 0.125f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvDist);
V4QA_ITEM_BEGIN(V4T_InvDistV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 dist = test.InvDistV(test2);
	bool bPass = VectorEqual(dist, 0.125f, 0.125f, 0.125f, 0.125f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvDistV);
V4QA_ITEM_BEGIN(V4T_Dist2)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float dist = test.Dist2(test2);
	bool bPass = FloatEqual(dist, 64.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dist2);
V4QA_ITEM_BEGIN(V4T_Dist2V)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 dist = test.Dist2V(test2);
	bool bPass = VectorEqual(dist, 64.0f, 64.0f, 64.0f, 64.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dist2V);
V4QA_ITEM_BEGIN(V4T_InvDist2)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float dist = test.InvDist2(test2);
	bool bPass = FloatEqual(dist, 0.015625f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvDist2);
V4QA_ITEM_BEGIN(V4T_InvDist2V)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 dist = test.InvDist2V(test2);
	bool bPass = VectorEqual(dist, 0.015625f, 0.015625f, 0.015625f, 0.015625f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvDist2V);
V4QA_ITEM_BEGIN(V4T_Dist3)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float dist = test.Dist3(test2);
	bool bPass = FloatEqual(dist, 6.92820323f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dist3);
V4QA_ITEM_BEGIN(V4T_Dist3V)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 dist = test.Dist3V(test2);
	bool bPass = VectorEqual(dist, 6.92820323f, 6.92820323f, 6.92820323f, 6.92820323f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dist3V);
V4QA_ITEM_BEGIN(V4T_InvDist3)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float dist = test.InvDist3(test2);
	bool bPass = FloatEqual(dist, 0.144337567f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvDist3);
V4QA_ITEM_BEGIN(V4T_InvDist3V)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 dist = test.InvDist3V(test2);
	bool bPass = VectorEqual(dist, 0.144337567f, 0.144337567f, 0.144337567f, 0.144337567f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvDist3V);
V4QA_ITEM_BEGIN(V4T_Dist32)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float dist = test.Dist32(test2);
	bool bPass = FloatEqual(dist, 48.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dist32);
V4QA_ITEM_BEGIN(V4T_Dist32V)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 dist = test.Dist32V(test2);
	bool bPass = VectorEqual(dist, 48.0f, 48.0f, 48.0f, 48.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Dist32V);
V4QA_ITEM_BEGIN(V4T_InvDist32)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	float dist = test.InvDist32(test2);
	bool bPass = FloatEqual(dist, 0.02083333f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvDist32);
V4QA_ITEM_BEGIN(V4T_InvDist32V)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 dist = test.InvDist32V(test2);
	bool bPass = VectorEqual(dist, 0.02083333f, 0.02083333f, 0.02083333f, 0.02083333f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_InvDist32V);
V4QA_ITEM_BEGIN(V4T_IsZero)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	bool bPass = (test1.IsZero() && !test2.IsZero());
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsZero);
V4QA_ITEM_BEGIN(V4T_IsNonZero)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	bool bPass = (!test1.IsNonZero() && test2.IsNonZero());
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsNonZero);
V4QA_ITEM_BEGIN(V4T_IsEqual)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(1.0f, 1.0f, 1.0f, 1.0f);
	bool bPass = (!test1.IsEqual(test2) && test2.IsEqual(test3));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsEqual);
V4QA_ITEM_BEGIN(V4T_IsNotEqual)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(1.0f, 1.0f, 1.0f, 1.0f);
	bool bPass = (test1.IsNotEqual(test2) && !test2.IsNotEqual(test3));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsNotEqual);
V4QA_ITEM_BEGIN(V4T_IsClose)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.9999999999f, 0.9999999999f, 0.9999999999f, 0.9999999999f);
	bool bPass = (!test1.IsClose(test2, SMALL_FLOAT) && test2.IsClose(test3, SMALL_FLOAT));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsClose);
V4QA_ITEM_BEGIN(V4T_IsCloseV)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.9999999999f, 0.9999999999f, 0.9999999999f, 0.9999999999f);
	Vector4 range(SMALL_FLOAT, SMALL_FLOAT, SMALL_FLOAT, SMALL_FLOAT);
	bool bPass = (!test1.IsClose(test2, range) && test2.IsClose(test3, range));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsCloseV);
V4QA_ITEM_BEGIN(V4T_IsGreaterThan)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.9999999f, 0.9999999f, 0.9999999f, 0.9999999f);
	bool bTest1 = test1.IsGreaterThan(test2);
	bool bTest2 = test2.IsGreaterThan(test3);
	bool bTest3 = test2.IsGreaterThan(test2);
	bool bPass = (!bTest1 && bTest2 && !bTest3);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsGreaterThan);
V4QA_ITEM_BEGIN(V4T_IsGreaterThanV)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.9999999f, 0.9999999f, 0.9999999f, 0.9999999f);
	test1 = test1.IsGreaterThanV(test2);
	test3 = test2.IsGreaterThanV(test3);
	test2 = test2.IsGreaterThanV(test2);
	bool bPass = (VectorEqual(test1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(test2, 0.0f, 0.0f, 0.0f, 0.0f) && VectorBitwiseEqual(test3, allBitsF, allBitsF, allBitsF, allBitsF));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsGreaterThanV);
V4QA_ITEM_BEGIN(V4T_IsGreaterThanVR)
{
	u32 r1, r2, r3;
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.9999999f, 0.9999999f, 0.9999999f, 0.9999999f);
	test1 = test1.IsGreaterThanVR(test2, r1);
	test3 = test2.IsGreaterThanVR(test3, r2);
	test2 = test2.IsGreaterThanVR(test2, r3);
	bool bPass = (VectorEqual(test1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(test2, 0.0f, 0.0f, 0.0f, 0.0f) && VectorBitwiseEqual(test3, allBitsF, allBitsF, allBitsF, allBitsF));
	bPass = bPass && ((r1 & VEC3_CMP_VAL) == 0) && ((r3 & VEC3_CMP_VAL) == 0) && ((r2 & VEC3_CMP_VAL) == VEC3_CMP_VAL);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsGreaterThanVR);
V4QA_ITEM_BEGIN(V4T_IsLessThan)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.9999999f, 0.9999999f, 0.9999999f, 0.9999999f);
	bool bPass = (test1.IsLessThanDoNotUse(test2) && !test2.IsLessThanDoNotUse(test3) && !test2.IsLessThanDoNotUse(test2));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsLessThan);
V4QA_ITEM_BEGIN(V4T_IsLessThanV)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.9999999f, 0.9999999f, 0.9999999f, 0.9999999f);
	test1 = test1.IsLessThanV(test2);
	test3 = test2.IsLessThanV(test3);
	test2 = test2.IsLessThanV(test2);
	bool bPass = (VectorBitwiseEqual(test1, allBitsF, allBitsF, allBitsF, allBitsF) && VectorEqual(test2, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(test3, 0.0f, 0.0f, 0.0f, 0.0f));
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsLessThanV);
V4QA_ITEM_BEGIN(V4T_IsLessThanVR)
{
	u32 r1, r2, r3;
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.9999999f, 0.9999999f, 0.9999999f, 0.9999999f);
	test1 = test1.IsLessThanVR(test2, r1);
	test3 = test2.IsLessThanVR(test3, r2);
	test2 = test2.IsLessThanVR(test2, r3);
	bool bPass = (VectorBitwiseEqual(test1, allBitsF, allBitsF, allBitsF, allBitsF) && VectorEqual(test2, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(test3, 0.0f, 0.0f, 0.0f, 0.0f));
	bPass = bPass && ((r1 & VEC3_CMP_VAL) == VEC3_CMP_VAL) && ((r2 & VEC3_CMP_VAL) == 0) && ((r3 & VEC3_CMP_VAL) == 0);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_IsLessThanVR);
V4QA_ITEM_BEGIN(V4T_Select)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(0.5f, 0.5f, 0.5f, 0.5f);
	Vector4 sel1 = test1.Select(test2, test3);
	Vector4 sel2 = test2.Select(test1, test3);
	bool bPass = VectorEqual(sel1, 1.0f, 1.0f, 1.0f, 1.0f) && VectorEqual(sel2, 0.5f, 0.5f, 0.5f, 0.5f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Select);
V4QA_ITEM_BEGIN(V4T_Min)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(4.0f, 3.0f, 2.0f, 1.0f);
	test.Min(test1, test2);
	bool bPass = VectorEqual(test, 1.0f, 2.0f, 2.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Min)
V4QA_ITEM_BEGIN(V4T_Max)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(4.0f, 3.0f, 2.0f, 1.0f);
	test.Max(test1, test2);
	bool bPass = VectorEqual(test, 4.0f, 3.0f, 3.0f, 4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Max)
V4QA_ITEM_BEGIN(V4T_ComputePlaneV3)
{
	Vector4 test;
	Vector3 test1(-10.0f, 0.0f, -10.0f);
	Vector3 test2( 10.0f, 0.0f, -10.0f);
	Vector3 test3(-10.0f, 0.0f,  10.0f);
	test.ComputePlane(test1, test2, test3);
	bool bPass = VectorEqual(test, 0.0f, -1.0f, 0.0f, 0.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_ComputePlaneV3);
V4QA_ITEM_BEGIN(V4T_ComputePlaneV2)
{
	Vector4 test;
	Vector3 test1(10.0f, 12.0f, 13.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	test.ComputePlane(test1, test2);
	bool bPass = VectorEqual(test, 0.0f, 1.0f, 0.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_ComputePlaneV2);
V4QA_ITEM_BEGIN(V4T_DistanceToPlane)
{
	Vector4 test;
	Vector3 test1(10.0f, 12.0f, 13.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(35.0f, 5.0f, 0.0f);
	test.ComputePlane(test1, test2);
	float dist = test.DistanceToPlane(test3);
	bool bPass = FloatEqual(dist, -7.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_DistanceToPlane);
V4QA_ITEM_BEGIN(V4T_DistanceToPlaneV3)
{
	Vector4 test;
	Vector3 test1(10.0f, 12.0f, 13.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector3 test3(35.0f, 5.0f, 0.0f);
	test.ComputePlane(test1, test2);
	Vector4 res = test.DistanceToPlaneV(test3);
	bool bPass = FloatEqual(res.x, -7.0f) && FloatEqual(res.y, -7.0f) && FloatEqual(res.z, -7.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_DistanceToPlaneV3);
V4QA_ITEM_BEGIN(V4T_DistanceToPlaneV4)
{
	Vector4 test;
	Vector3 test1(10.0f, 12.0f, 13.0f);
	Vector3 test2(0.0f, 1.0f, 0.0f);
	Vector4 test3(35.0f, 5.0f, 0.0f, 7.0f);
	test.ComputePlane(test1, test2);
	Vector4 res = test.DistanceToPlaneV(test3);
	bool bPass = VectorEqual(res, -7.0f, -7.0f, -7.0f, -7.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_DistanceToPlaneV4);

V4QA_ITEM_BEGIN(V4T_And)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(allBitsF, allBitsF, allBitsF, allBitsF);
	test1.And(test2);
	test3.And(test2);
	bool bPass = VectorEqual(test1, 0.0f, 0.0f, 0.0f, 0.0f) && VectorEqual(test2, 1.0f, 1.0f, 1.0f, 1.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_And);
V4QA_ITEM_BEGIN(V4T_Or)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(allBitsF, allBitsF, allBitsF, allBitsF);
	test1.Or(test2);
	test3.Or(test2);
	bool bPass = VectorEqual(test1, 1.0f, 1.0f, 1.0f, 1.0f) && VectorBitwiseEqual(test3, allBitsF, allBitsF, allBitsF, allBitsF);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Or);
V4QA_ITEM_BEGIN(V4T_Xor)
{
	Vector4 test1(0.0f, 0.0f, 0.0f, 0.0f);
	Vector4 test2(1.0f, 1.0f, 1.0f, 1.0f);
	Vector4 test3(allBitsF, allBitsF, allBitsF, allBitsF);
	test1.Xor(test2);
	test3.Xor(test2);
	bool bPass = VectorEqual(test1, 1.0f, 1.0f, 1.0f, 1.0f) && VectorEqual(test3, -4.0f, -4.0f, -4.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_Xor);
V4QA_ITEM_BEGIN(V4T_MergeXY)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test1(5.0f, 6.0f, 7.0f, 8.0f);
	test.MergeXY(test1);
	bool bPass = VectorEqual(test, 1.0f, 5.0f, 2.0f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_MergeXY);
V4QA_ITEM_BEGIN(V4T_MergeXYV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.MergeXY(test1, test2);
	bool bPass = VectorEqual(test, 1.0f, 5.0f, 2.0f, 6.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_MergeXYV);
V4QA_ITEM_BEGIN(V4T_MergeZW)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test1(5.0f, 6.0f, 7.0f, 8.0f);
	test.MergeZW(test1);
	bool bPass = VectorEqual(test, 3.0f, 7.0f, 4.0f, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_MergeZW);
V4QA_ITEM_BEGIN(V4T_MergeZWV)
{
	Vector4 test;
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test.MergeZW(test1, test2);
	bool bPass = VectorEqual(test, 3.0f, 7.0f, 4.0f, 8.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_MergeZWV);
V4QA_ITEM_BEGIN(V4T_operatorPlus)
{
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 test = test1 + test2;
	bool bPass = VectorEqual(test, 6.0f, 8.0f, 10.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorPlus);
V4QA_ITEM_BEGIN(V4T_operatorMinus)
{
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 test = test1 - test2;
	bool bPass = VectorEqual(test, -4.0f, -4.0f, -4.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorMinus);
V4QA_ITEM_BEGIN(V4T_operatorNeg)
{
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test = -test1;
	bool bPass = VectorEqual(test, -1.0f, -2.0f, -3.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorNeg);
V4QA_ITEM_BEGIN(V4T_operatorTimesF)
{
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test = test1 * 0.5f;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f, 2.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorTimesF);
V4QA_ITEM_BEGIN(V4T_operatorTimesV)
{
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	Vector4 test = test1 * test2;
	bool bPass = VectorEqual(test, 5.0f, 12.0f, 21.0f, 32.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorTimesV);
V4QA_ITEM_BEGIN(V4T_operatorDivF)
{
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test = test1 / 2.0f;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f, 2.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorDivF);
V4QA_ITEM_BEGIN(V4T_operatorDivFV)
{
	Vector4 test1(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(2.0f, 2.0f, 2.0f, 2.0f);
	Vector4 test = test1 / test2;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f, 2.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorDivFV);
V4QA_ITEM_BEGIN(V4T_operatorPlusEq)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test += test2;
	bool bPass = VectorEqual(test, 6.0f, 8.0f, 10.0f, 12.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorPlusEq);
V4QA_ITEM_BEGIN(V4T_operatorMinusEq)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test -= test2;
	bool bPass = VectorEqual(test, -4.0f, -4.0f, -4.0f, -4.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorMinusEq);
V4QA_ITEM_BEGIN(V4T_operatorTimesEqF)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	test *= 0.5f;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f, 2.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorTimesEqF);
V4QA_ITEM_BEGIN(V4T_operatorTimesEqV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(5.0f, 6.0f, 7.0f, 8.0f);
	test *= test2;
	bool bPass = VectorEqual(test, 5.0f, 12.0f, 21.0f, 32.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorTimesEqV);
V4QA_ITEM_BEGIN(V4T_operatorDivEqF)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	test /= 2.0f;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f, 2.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorDivEqF);
V4QA_ITEM_BEGIN(V4T_operatorDivEqV)
{
	Vector4 test(1.0f, 2.0f, 3.0f, 4.0f);
	Vector4 test2(2.0f, 2.0f, 2.0f, 2.0f);
	test /= test2;
	bool bPass = VectorEqual(test, 0.5f, 1.0f, 1.5f, 2.0f);
	if( bPass )
		TST_PASS;
	else
		TST_FAIL;
}
V4QA_ITEM_END(V4T_operatorDivEqV);

QA_ITEM_FAMILY(qaV4T_GetX, (), ());
QA_ITEM_FAST(qaV4T_GetX, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_GetY, (), ());
QA_ITEM_FAST(qaV4T_GetY, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_GetZ, (), ());
QA_ITEM_FAST(qaV4T_GetZ, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_GetW, (), ());
QA_ITEM_FAST(qaV4T_GetW, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SetX, (), ());
QA_ITEM_FAST(qaV4T_SetX, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SetY, (), ());
QA_ITEM_FAST(qaV4T_SetY, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SetZ, (), ());
QA_ITEM_FAST(qaV4T_SetZ, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SetW, (), ());
QA_ITEM_FAST(qaV4T_SetW, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SplatX, (), ());
QA_ITEM_FAST(qaV4T_SplatX, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SplatY, (), ());
QA_ITEM_FAST(qaV4T_SplatY, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SplatZ, (), ());
QA_ITEM_FAST(qaV4T_SplatZ, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SplatW, (), ());
QA_ITEM_FAST(qaV4T_SplatW, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_ScaleF, (), ());
QA_ITEM_FAST(qaV4T_ScaleF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_ScaleVF, (), ());
QA_ITEM_FAST(qaV4T_ScaleVF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Scale3F, (), ());
QA_ITEM_FAST(qaV4T_Scale3F, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Scale3VF, (), ());
QA_ITEM_FAST(qaV4T_Scale3VF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvScaleF, (), ());
QA_ITEM_FAST(qaV4T_InvScaleF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvScaleV, (), ());
QA_ITEM_FAST(qaV4T_InvScaleV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvScaleVF, (), ());
QA_ITEM_FAST(qaV4T_InvScaleVF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvScaleVV, (), ());
QA_ITEM_FAST(qaV4T_InvScaleVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddF, (), ());
QA_ITEM_FAST(qaV4T_AddF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddV, (), ());
QA_ITEM_FAST(qaV4T_AddV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddVV, (), ());
QA_ITEM_FAST(qaV4T_AddVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddVV3, (), ());
QA_ITEM_FAST(qaV4T_AddVV3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddScaledF, (), ());
QA_ITEM_FAST(qaV4T_AddScaledF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddScaledV, (), ());
QA_ITEM_FAST(qaV4T_AddScaledV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddScaledVF, (), ());
QA_ITEM_FAST(qaV4T_AddScaledVF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddScaledVV, (), ());
QA_ITEM_FAST(qaV4T_AddScaledVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddScaled3F, (), ());
QA_ITEM_FAST(qaV4T_AddScaled3F, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddScaled3V, (), ());
QA_ITEM_FAST(qaV4T_AddScaled3V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddScaled3VF, (), ());
QA_ITEM_FAST(qaV4T_AddScaled3VF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AddScaled3VV, (), ());
QA_ITEM_FAST(qaV4T_AddScaled3VV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SubtractF, (), ());
QA_ITEM_FAST(qaV4T_SubtractF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SubtractV, (), ());
QA_ITEM_FAST(qaV4T_SubtractV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SubtractVV, (), ());
QA_ITEM_FAST(qaV4T_SubtractVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SubtractVV3, (), ());
QA_ITEM_FAST(qaV4T_SubtractVV3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SubtractScaledF, (), ());
QA_ITEM_FAST(qaV4T_SubtractScaledF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SubtractScaledV, (), ());
QA_ITEM_FAST(qaV4T_SubtractScaledV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SubtractScaledVF, (), ());
QA_ITEM_FAST(qaV4T_SubtractScaledVF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SubtractScaledVV, (), ());
QA_ITEM_FAST(qaV4T_SubtractScaledVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_MultiplyV, (), ());
QA_ITEM_FAST(qaV4T_MultiplyV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_MultiplyVV, (), ());
QA_ITEM_FAST(qaV4T_MultiplyVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Negate, (), ());
QA_ITEM_FAST(qaV4T_Negate, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_NegateV, (), ());
QA_ITEM_FAST(qaV4T_NegateV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Abs, (), ());
QA_ITEM_FAST(qaV4T_Abs, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AbsV, (), ());
QA_ITEM_FAST(qaV4T_AbsV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Invert, (), ());
QA_ITEM_FAST(qaV4T_Invert, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvertV, (), ());
QA_ITEM_FAST(qaV4T_InvertV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvertSafe, (), ());
QA_ITEM_FAST(qaV4T_InvertSafe, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvertSafeV, (), ());
QA_ITEM_FAST(qaV4T_InvertSafeV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Normalize, (), ());
QA_ITEM_FAST(qaV4T_Normalize, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_NormalizeFast, (), ());
QA_ITEM_FAST(qaV4T_NormalizeFast, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_NormalizeV, (), ());
QA_ITEM_FAST(qaV4T_NormalizeV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_NormalizeFastV, (), ());
QA_ITEM_FAST(qaV4T_NormalizeFastV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Normalize3, (), ());
QA_ITEM_FAST(qaV4T_Normalize3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_NormalizeFast3, (), ());
QA_ITEM_FAST(qaV4T_NormalizeFast3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_NormalizeV3, (), ());
QA_ITEM_FAST(qaV4T_NormalizeV3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_NormalizeFastV3, (), ());
QA_ITEM_FAST(qaV4T_NormalizeFastV3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dot, (), ());
QA_ITEM_FAST(qaV4T_Dot, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_DotV, (), ());
QA_ITEM_FAST(qaV4T_DotV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dot3, (), ());
QA_ITEM_FAST(qaV4T_Dot3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dot33, (), ());
QA_ITEM_FAST(qaV4T_Dot33, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dot3V, (), ());
QA_ITEM_FAST(qaV4T_Dot3V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Cross, (), ());
QA_ITEM_FAST(qaV4T_Cross, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Cross3, (), ());
QA_ITEM_FAST(qaV4T_Cross3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_CrossV33, (), ());
QA_ITEM_FAST(qaV4T_CrossV33, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_CrossV43, (), ());
QA_ITEM_FAST(qaV4T_CrossV43, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_CrossV34, (), ());
QA_ITEM_FAST(qaV4T_CrossV34, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_CrossV44, (), ());
QA_ITEM_FAST(qaV4T_CrossV44, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Average, (), ());
QA_ITEM_FAST(qaV4T_Average, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_AverageV, (), ());
QA_ITEM_FAST(qaV4T_AverageV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_LerpFV, (), ());
QA_ITEM_FAST(qaV4T_LerpFV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_LerpVV, (), ());
QA_ITEM_FAST(qaV4T_LerpVV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_LerpF, (), ());
QA_ITEM_FAST(qaV4T_LerpF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_LerpV, (), ());
QA_ITEM_FAST(qaV4T_LerpV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_SqrtV, (), ());
QA_ITEM_FAST(qaV4T_SqrtV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Mag, (), ());
QA_ITEM_FAST(qaV4T_Mag, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_MagV, (), ());
QA_ITEM_FAST(qaV4T_MagV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Mag2, (), ());
QA_ITEM_FAST(qaV4T_Mag2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Mag2V, (), ());
QA_ITEM_FAST(qaV4T_Mag2V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Mag3, (), ());
QA_ITEM_FAST(qaV4T_Mag3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Mag3V, (), ());
QA_ITEM_FAST(qaV4T_Mag3V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Mag32, (), ());
QA_ITEM_FAST(qaV4T_Mag32, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Mag32V, (), ());
QA_ITEM_FAST(qaV4T_Mag32V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvMag, (), ());
QA_ITEM_FAST(qaV4T_InvMag, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvMagV, (), ());
QA_ITEM_FAST(qaV4T_InvMagV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvMag3, (), ());
QA_ITEM_FAST(qaV4T_InvMag3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvMag3V, (), ());
QA_ITEM_FAST(qaV4T_InvMag3V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dist, (), ());
QA_ITEM_FAST(qaV4T_Dist, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_DistV, (), ());
QA_ITEM_FAST(qaV4T_DistV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvDist, (), ());
QA_ITEM_FAST(qaV4T_InvDist, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvDistV, (), ());
QA_ITEM_FAST(qaV4T_InvDistV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dist2, (), ());
QA_ITEM_FAST(qaV4T_Dist2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dist2V, (), ());
QA_ITEM_FAST(qaV4T_Dist2V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvDist2, (), ());
QA_ITEM_FAST(qaV4T_InvDist2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvDist2V, (), ());
QA_ITEM_FAST(qaV4T_InvDist2V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dist3, (), ());
QA_ITEM_FAST(qaV4T_Dist3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dist3V, (), ());
QA_ITEM_FAST(qaV4T_Dist3V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvDist3, (), ());
QA_ITEM_FAST(qaV4T_InvDist3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvDist3V, (), ());
QA_ITEM_FAST(qaV4T_InvDist3V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dist32, (), ());
QA_ITEM_FAST(qaV4T_Dist32, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Dist32V, (), ());
QA_ITEM_FAST(qaV4T_Dist32V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvDist32, (), ());
QA_ITEM_FAST(qaV4T_InvDist32, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_InvDist32V, (), ());
QA_ITEM_FAST(qaV4T_InvDist32V, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsZero, (), ());
QA_ITEM_FAST(qaV4T_IsZero, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsNonZero, (), ());
QA_ITEM_FAST(qaV4T_IsNonZero, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsEqual, (), ());
QA_ITEM_FAST(qaV4T_IsEqual, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsNotEqual, (), ());
QA_ITEM_FAST(qaV4T_IsNotEqual, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsClose, (), ());
QA_ITEM_FAST(qaV4T_IsClose, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsCloseV, (), ());
QA_ITEM_FAST(qaV4T_IsCloseV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsGreaterThan, (), ());
QA_ITEM_FAST(qaV4T_IsGreaterThan, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsGreaterThanV, (), ());
QA_ITEM_FAST(qaV4T_IsGreaterThanV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsGreaterThanVR, (), ());
QA_ITEM_FAST(qaV4T_IsGreaterThanVR, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsLessThan, (), ());
QA_ITEM_FAST(qaV4T_IsLessThan, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsLessThanV, (), ());
QA_ITEM_FAST(qaV4T_IsLessThanV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_IsLessThanVR, (), ());
QA_ITEM_FAST(qaV4T_IsLessThanVR, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Select, (), ());
QA_ITEM_FAST(qaV4T_Select, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Min, (), ());
QA_ITEM_FAST(qaV4T_Min, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Max, (), ());
QA_ITEM_FAST(qaV4T_Max, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_ComputePlaneV3, (), ());
QA_ITEM_FAST(qaV4T_ComputePlaneV3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_ComputePlaneV2, (), ());
QA_ITEM_FAST(qaV4T_ComputePlaneV2, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_DistanceToPlane, (), ());
QA_ITEM_FAST(qaV4T_DistanceToPlane, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_DistanceToPlaneV3, (), ());
QA_ITEM_FAST(qaV4T_DistanceToPlaneV3, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_DistanceToPlaneV4, (), ());
QA_ITEM_FAST(qaV4T_DistanceToPlaneV4, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_And, (), ());
QA_ITEM_FAST(qaV4T_And, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Or, (), ());
QA_ITEM_FAST(qaV4T_Or, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_Xor, (), ());
QA_ITEM_FAST(qaV4T_Xor, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_MergeXY, (), ());
QA_ITEM_FAST(qaV4T_MergeXY, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_MergeXYV, (), ());
QA_ITEM_FAST(qaV4T_MergeXYV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_MergeZW, (), ());
QA_ITEM_FAST(qaV4T_MergeZW, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_MergeZWV, (), ());
QA_ITEM_FAST(qaV4T_MergeZWV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorPlus, (), ());
QA_ITEM_FAST(qaV4T_operatorPlus, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorMinus, (), ());
QA_ITEM_FAST(qaV4T_operatorMinus, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorNeg, (), ());
QA_ITEM_FAST(qaV4T_operatorNeg, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorTimesF, (), ());
QA_ITEM_FAST(qaV4T_operatorTimesF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorTimesV, (), ());
QA_ITEM_FAST(qaV4T_operatorTimesV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorDivF, (), ());
QA_ITEM_FAST(qaV4T_operatorDivF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorDivFV, (), ());
QA_ITEM_FAST(qaV4T_operatorDivFV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorPlusEq, (), ());
QA_ITEM_FAST(qaV4T_operatorPlusEq, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorMinusEq, (), ());
QA_ITEM_FAST(qaV4T_operatorMinusEq, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorTimesEqF, (), ());
QA_ITEM_FAST(qaV4T_operatorTimesEqF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorTimesEqV, (), ());
QA_ITEM_FAST(qaV4T_operatorTimesEqV, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorDivEqF, (), ());
QA_ITEM_FAST(qaV4T_operatorDivEqF, (), qaResult::FAIL_OR_TOTAL_TIME);
QA_ITEM_FAMILY(qaV4T_operatorDivEqV, (), ());
QA_ITEM_FAST(qaV4T_operatorDivEqV, (), qaResult::FAIL_OR_TOTAL_TIME);

#endif // __QA
