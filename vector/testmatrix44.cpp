//
// vector/testmatrix44.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "matrix44.h"
#include "system/main.h"
#include "math/random.h"
#include <stdio.h>

using namespace rage;

inline bool CheckFloat(float a,float b,const char* element,const char* testName)
{
	if (a!=b)
	{
		mthDisplayf("'%s' TEST FAILED, %s NOT EQUAL (%f!=%f)",testName,element,a,b);
		return false;
	}
	return true;
}

bool TestEquality(const Matrix44& mat1,const Matrix44& mat2,const char* testName)
{
	bool result=true;
	result=CheckFloat(mat1.a.x,mat2.a.x,"a.x",testName) && result;
	result=CheckFloat(mat1.a.y,mat2.a.y,"a.y",testName) && result;
	result=CheckFloat(mat1.a.z,mat2.a.z,"a.z",testName) && result;
	result=CheckFloat(mat1.a.w,mat2.a.w,"a.w",testName) && result;

	result=CheckFloat(mat1.b.x,mat2.b.x,"b.x",testName) && result;
	result=CheckFloat(mat1.b.y,mat2.b.y,"b.y",testName) && result;
	result=CheckFloat(mat1.b.z,mat2.b.z,"b.z",testName) && result;
	result=CheckFloat(mat1.b.w,mat2.b.w,"b.w",testName) && result;

	result=CheckFloat(mat1.c.x,mat2.c.x,"c.x",testName) && result;
	result=CheckFloat(mat1.c.y,mat2.c.y,"c.y",testName) && result;
	result=CheckFloat(mat1.c.z,mat2.c.z,"c.z",testName) && result;
	result=CheckFloat(mat1.c.w,mat2.c.w,"c.w",testName) && result;

	result=CheckFloat(mat1.d.x,mat2.d.x,"d.x",testName) && result;
	result=CheckFloat(mat1.d.y,mat2.d.y,"d.y",testName) && result;
	result=CheckFloat(mat1.d.z,mat2.d.z,"d.z",testName) && result;
	result=CheckFloat(mat1.d.w,mat2.d.w,"d.w",testName) && result;

	return result;
}

static Matrix44 mat1(1.0f,	2.0f,	3.0f,	4.0f,
					 5.0f,	6.0f,	7.0f,	8.0f,
					 9.0f,	10.0f,	11.0f, 12.0f,
					 13.0f,	14.0f,	15.0f, 16.0f);

static Matrix44 mat2(17.0f,	18.0f,	19.0f,	20.0f,
					 21.0f,	22.0f,	23.0f,	24.0f,
					 25.0f,	26.0f,	27.0f,	28.0f,
					 29.0f,	30.0f,	31.0f,	32.0f);

static Matrix44 matZero(0.0f,0.0f,0.0f,0.0f,
						0.0f,0.0f,0.0f,0.0f,
						0.0f,0.0f,0.0f,0.0f,
						0.0f,0.0f,0.0f,0.0f);

static void Matrix44Add(Matrix44& mat1, const Matrix44& mat2)
{
	mat1.a.x=mat1.a.x+mat2.a.x;
	mat1.a.y=mat1.a.y+mat2.a.y;	
	mat1.a.z=mat1.a.z+mat2.a.z;	
	mat1.a.w=mat1.a.w+mat2.a.w;

	mat1.b.x=mat1.b.x+mat2.b.x,	
	mat1.b.y=mat1.b.y+mat2.b.y,	
	mat1.b.z=mat1.b.z+mat2.b.z,	
	mat1.b.w=mat1.b.w+mat2.b.w,

	mat1.c.x=mat1.c.x+mat2.c.x;
	mat1.c.y=mat1.c.y+mat2.c.y;	
	mat1.c.z=mat1.c.z+mat2.c.z;	
	mat1.c.w=mat1.c.w+mat2.c.w;

	mat1.d.x=mat1.d.x+mat2.d.x;	
	mat1.d.y=mat1.d.y+mat2.d.y;
	mat1.d.z=mat1.d.z+mat2.d.z;
	mat1.d.w=mat1.d.w+mat2.d.w;
}

static void Matrix44AddScaled(Matrix44& mat1, const Matrix44& mat2,float scale)
{
	mat1.a.x=mat1.a.x+mat2.a.x*scale;
	mat1.a.y=mat1.a.y+mat2.a.y*scale;	
	mat1.a.z=mat1.a.z+mat2.a.z*scale;	
	mat1.a.w=mat1.a.w+mat2.a.w*scale;	

	mat1.b.x=mat1.b.x+mat2.b.x*scale;	
	mat1.b.y=mat1.b.y+mat2.b.y*scale;	
	mat1.b.z=mat1.b.z+mat2.b.z*scale;	
	mat1.b.w=mat1.b.w+mat2.b.w*scale;	

	mat1.c.x=mat1.c.x+mat2.c.x*scale;
	mat1.c.y=mat1.c.y+mat2.c.y*scale;
	mat1.c.z=mat1.c.z+mat2.c.z*scale;
	mat1.c.w=mat1.c.w+mat2.c.w*scale;

	mat1.d.x=mat1.d.x+mat2.d.x*scale;
	mat1.d.y=mat1.d.y+mat2.d.y*scale;
	mat1.d.z=mat1.d.z+mat2.d.z*scale;
	mat1.d.w=mat1.d.w+mat2.d.w*scale;
}

static void Matrix44Subtract(Matrix44& mat1, const Matrix44& mat2)
{
	mat1.a.x=mat1.a.x-mat2.a.x;
	mat1.a.y=mat1.a.y-mat2.a.y;	
	mat1.a.z=mat1.a.z-mat2.a.z;	
	mat1.a.w=mat1.a.w-mat2.a.w;

	mat1.b.x=mat1.b.x-mat2.b.x,	
	mat1.b.y=mat1.b.y-mat2.b.y,	
	mat1.b.z=mat1.b.z-mat2.b.z,	
	mat1.b.w=mat1.b.w-mat2.b.w,

	mat1.c.x=mat1.c.x-mat2.c.x;
	mat1.c.y=mat1.c.y-mat2.c.y;	
	mat1.c.z=mat1.c.z-mat2.c.z;	
	mat1.c.w=mat1.c.w-mat2.c.w;

	mat1.d.x=mat1.d.x-mat2.d.x;	
	mat1.d.y=mat1.d.y-mat2.d.y;
	mat1.d.z=mat1.d.z-mat2.d.z;
	mat1.d.w=mat1.d.w-mat2.d.w;
}

// NOTE that PS2 tests will fail by a slight amount due to the 1 bit multiplication
// in the optimized assembly functions VMULA, VMUL, etc.
static void Matrix44Dot(Matrix44& mat1, const Matrix44& mat2)
{
	float ax=mat1.a.x*mat2.a.x+mat1.a.y*mat2.b.x+mat1.a.z*mat2.c.x+mat1.a.w*mat2.d.x;
	float ay=mat1.a.x*mat2.a.y+mat1.a.y*mat2.b.y+mat1.a.z*mat2.c.y+mat1.a.w*mat2.d.y;
	float az=mat1.a.x*mat2.a.z+mat1.a.y*mat2.b.z+mat1.a.z*mat2.c.z+mat1.a.w*mat2.d.z;
	float aw=mat1.a.x*mat2.a.w+mat1.a.y*mat2.b.w+mat1.a.z*mat2.c.w+mat1.a.w*mat2.d.w;

	float bx=mat1.b.x*mat2.a.x+mat1.b.y*mat2.b.x+mat1.b.z*mat2.c.x+mat1.b.w*mat2.d.x;
	float by=mat1.b.x*mat2.a.y+mat1.b.y*mat2.b.y+mat1.b.z*mat2.c.y+mat1.b.w*mat2.d.y;
	float bz=mat1.b.x*mat2.a.z+mat1.b.y*mat2.b.z+mat1.b.z*mat2.c.z+mat1.b.w*mat2.d.z;
	float bw=mat1.b.x*mat2.a.w+mat1.b.y*mat2.b.w+mat1.b.z*mat2.c.w+mat1.b.w*mat2.d.w;

	float cx=mat1.c.x*mat2.a.x+mat1.c.y*mat2.b.x+mat1.c.z*mat2.c.x+mat1.c.w*mat2.d.x;
	float cy=mat1.c.x*mat2.a.y+mat1.c.y*mat2.b.y+mat1.c.z*mat2.c.y+mat1.c.w*mat2.d.y;
	float cz=mat1.c.x*mat2.a.z+mat1.c.y*mat2.b.z+mat1.c.z*mat2.c.z+mat1.c.w*mat2.d.z;
	float cw=mat1.c.x*mat2.a.w+mat1.c.y*mat2.b.w+mat1.c.z*mat2.c.w+mat1.c.w*mat2.d.w;

	float dx=mat1.d.x*mat2.a.x+mat1.d.y*mat2.b.x+mat1.d.z*mat2.c.x+mat1.d.w*mat2.d.x;
	float dy=mat1.d.x*mat2.a.y+mat1.d.y*mat2.b.y+mat1.d.z*mat2.c.y+mat1.d.w*mat2.d.y;
	float dz=mat1.d.x*mat2.a.z+mat1.d.y*mat2.b.z+mat1.d.z*mat2.c.z+mat1.d.w*mat2.d.z;
	float dw=mat1.d.x*mat2.a.w+mat1.d.y*mat2.b.w+mat1.d.z*mat2.c.w+mat1.d.w*mat2.d.w;

	mat1.a.x=ax;
	mat1.a.y=ay;
	mat1.a.z=az;
	mat1.a.w=aw;

	mat1.b.x=bx;
	mat1.b.y=by;
	mat1.b.z=bz;
	mat1.b.w=bw;

	mat1.c.x=cx;
	mat1.c.y=cy;
	mat1.c.z=cz;
	mat1.c.w=cw;

	mat1.d.x=dx;
	mat1.d.y=dy;
	mat1.d.z=dz;
	mat1.d.w=dw;
}


void RandomizeVector(Vector4 &outVect, float maxVal) {
	float x, y, z, w;
	x = g_ReplayRand.GetFloat()*maxVal;
	y = g_ReplayRand.GetFloat()*maxVal;
	z = g_ReplayRand.GetFloat()*maxVal;
	w = g_ReplayRand.GetFloat()*maxVal;
	outVect.Set(x, y, z, w);
}

int Main() 
{
	// identity test:
	Matrix44 testIdentity;
	testIdentity.Identity();
	bool resultIdentity=TestEquality(testIdentity,M44_IDENTITY,"Matrix44::Identity()");

	// zero test:
	Matrix44 testZero;
	testZero.Zero();
	bool resultZero=TestEquality(testZero,matZero,"Matrix44::Zero()");


	bool resultSet		=true;
	bool resultAdd		=true;
	bool resultSubtract	=true;
	bool resultDot		=true;

	for (int i=0;i<1000;i++)
	{
		Matrix44	testMat1;
		Matrix44	testMat2;
		Matrix44	testResult;
		Matrix44	testCorrect;
		float		testScale=3.0f;
		if (i==0)
		{
			testMat1=mat1;
			testMat2=mat2;
		}
		else 
		{
			RandomizeVector(testMat1.a, 100.f);
			RandomizeVector(testMat1.b, 100.f);
			RandomizeVector(testMat1.c, 100.f);
			RandomizeVector(testMat1.d, 100.f);

			RandomizeVector(testMat2.a, 100.f);
			RandomizeVector(testMat2.b, 100.f);
			RandomizeVector(testMat2.c, 100.f);
			RandomizeVector(testMat2.d, 100.f);
			testScale=g_ReplayRand.GetFloat() * 100.f;
		}

		// set test:
		testResult.Set(testMat1);
		resultSet&=TestEquality(testResult,testMat1,"Matrix44::Set(const Matrix44 &m)");

		// add tests:
		testResult.Add(testMat1,testMat2);
		testCorrect.Set(testMat1);
		Matrix44Add(testCorrect,testMat2);
		resultAdd&=TestEquality(testResult,testCorrect,"Matrix44::Add(const Matrix44 &m,const Matrix44 &n)");
		testResult.Set(testMat1);
		testResult.Add(testMat2);
		resultAdd&=TestEquality(testResult,testCorrect,"Matrix44::Add(const Matrix44 &m)");
		testResult.Set(testMat1);
		testResult.AddScaled3x4(testMat2,testScale);
		testCorrect=testMat1;
		Matrix44AddScaled(testCorrect,testMat2,testScale);
		resultAdd&=TestEquality(testResult,testCorrect,"Matrix44::AddScaled(const Matrix44 &m, float f)");
		
		// subtract tests:
		testCorrect=testMat1;
		Matrix44Subtract(testCorrect,testMat2);
		testResult.Subtract(testMat1,testMat2);
		resultSubtract&=TestEquality(testResult,testCorrect,"Matrix44::Subtract(const Matrix44 &m,const Matrix44 &n)");
		testResult.Set(testMat1);
		testResult.Subtract(testMat2);
		resultSubtract&=TestEquality(testResult,testCorrect,"Matrix44::Subtract(const Matrix44 &m)");

		// dot tests:
		testCorrect=testMat1;
		Matrix44Dot(testCorrect,testMat2);
		testResult.Dot(testMat2,testMat1);
		resultDot&=TestEquality(testResult,testCorrect,"Matrix44::Dot(const Matrix44 &m1,const Matrix44 &m2)");
		testResult.Set(testMat1);
		testResult.Dot(testMat2);
		resultDot&=TestEquality(testResult,testCorrect,"Matrix44::Dot(const Matrix44 &m)");
	}

	if (resultIdentity==true)
		mthDisplayf("identity test passed.");
	else
		mthErrorf("identity test failed.");
	if (resultZero==true)
		mthDisplayf("zero test passed.");
	else
		mthErrorf("zero test failed.");
	if (resultSet==true)
		mthDisplayf("set test passed.");
	else
		mthErrorf("set test failed.");
	if (resultAdd==true)
		mthDisplayf("add test passed.");
	else
		mthErrorf("add test failed.");
	if (resultSubtract==true)
		mthDisplayf("subtract test passed.");
	else
		mthErrorf("subtract test failed.");
	if (resultDot==true)
		mthDisplayf("dot test passed.");
	else
		mthErrorf("dot test failed.");

#if __XBOX
	while(1) {} // prevent return on Xbox
#endif

	return 0;
}
