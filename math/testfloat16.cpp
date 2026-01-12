// 
// math/testfloat16.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "float16.h"

#include "system/main.h"

using namespace rage;

int Main()
{
	float f;
	for (f=-63; f<63; f+=0.25f)
	{
		Float16 f16(f);
		if (f16.GetFloat32_FromFloat16() != f) {
			mthErrorf("Test failed at %f, got %f instead",f,f16.GetFloat32_FromFloat16());
			return 0;
		}
	}

	const float kFloat16_Max = 65504.0f;
	const float kFloat16_Min = 6.1035156e-5f;

	Float16 test;
	test = 123.456f;
	mthDisplayf("Test value is %f",test.GetFloat32_FromFloat16());

	test = kFloat16_Min;
	mthDisplayf("Test value is %f",test.GetFloat32_FromFloat16());

	test = kFloat16_Max;
	mthDisplayf("Test value is %f",test.GetFloat32_FromFloat16());
	return 1;
}
