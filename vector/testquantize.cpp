//
// vector/testquantize.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "system/timer.h"
#include "system/main.h"
#include "math/amath.h"
#include "math/random.h"
#include "vector/quantize.h"
#include "vector/vector4.h"
#include <stdio.h>

using namespace rage;


int Main()
{
	int i;
	float x;
	u8 byte;
	u16 chawmp;
	u32 dword;
	int passes;

	////////////////////////////////////////////////////////////
	// test on random numbers

	// check error
	passes=10000;
	float errorMag8_0to1=0, errorMag8_n1to1=0, errorMag8_n2to2=0;
	float errorMag16_0to1=0, errorMag16_n1to1=0, errorMag16_n2to2=0;
	float errorMag10_nPItoPI=0, errorMag11_nPItoPI=0;

	for (i=0; i<passes; i++)
	{
		// -1 to 1
		x = g_ReplayRand.GetRanged(0.0f,1.0f) + g_ReplayRand.GetRanged(0.0f,1.0f) / 16384.0f;
		byte = Pack8_0to1(x);
		chawmp = Pack16_0to1(x);
		errorMag8_0to1 += fabsf(x-Unpack8_0to1(byte));
		errorMag16_0to1 += fabsf(x-Unpack16_0to1(chawmp));

		// -1 to 1
		x = g_ReplayRand.GetRanged(-1.0f,1.0f) + g_ReplayRand.GetRanged(-1.0f,1.0f) / 16384.0f;
		byte = Pack8_n1to1(x);
		chawmp = Pack16_n1to1(x);
		errorMag8_n1to1 += fabsf(x-Unpack8_n1to1(byte));
		errorMag16_n1to1 += fabsf(x-Unpack16_n1to1(chawmp));

		// -2 to 2
		x = g_ReplayRand.GetRanged(-2.0f,2.0f) + g_ReplayRand.GetRanged(-2.0f,2.0f) / 16384.0f;
		byte = Pack8_n2to2(x);
		chawmp = Pack16_n2to2(x);
		errorMag8_n2to2 += fabsf(x-Unpack8_n2to2(byte));
		errorMag16_n2to2 += fabsf(x-Unpack16_n2to2(chawmp));

		// -PI to PI
		x = g_ReplayRand.GetRanged(-PI,PI) + g_ReplayRand.GetRanged(-PI,PI) / 1024.0f;
		dword = Pack10_nPItoPI(x);
		errorMag10_nPItoPI += fabsf(x-Unpack10_nPItoPI(dword));
		dword = Pack11_nPItoPI(x);
		errorMag11_nPItoPI += fabsf(x-Unpack11_nPItoPI(dword));
	};

	printf("Quantize error average, %d runs...\n",passes);
	printf("   8 bits [ 0,1] = %f\n",errorMag8_0to1/passes);
	printf("  16 bits [ 0,1] = %f\n",errorMag16_0to1/passes);
	printf("   8 bits [-1,1] = %f\n",errorMag8_n1to1/passes);
	printf("  16 bits [-1,1] = %f\n",errorMag16_n1to1/passes);
	printf("   8 bits [-2,2] = %f\n",errorMag8_n2to2/passes);
	printf("  16 bits [-2,2] = %f\n",errorMag16_n2to2/passes);
	printf("  10 bits [-PI,PI] = %f\n",errorMag10_nPItoPI/passes);
	printf("  11 bits [-PI,PI] = %f\n",errorMag11_nPItoPI/passes);
	printf("\n");

	////////////////////////////////////////////////////////////
	// quantize some vectors
	printf("Quantizing floats...\n");
	printf("value, 8bit error, 16bit error\n");
	for (i=0; i<5; i++)
	{
		// -1 to 1
		x = g_ReplayRand.GetRanged(0.0f,1.0f) + g_ReplayRand.GetRanged(0.0f,1.0f) / 16384.0f;
		byte = Pack8_0to1(x);
		chawmp = Pack16_0to1(x);
		printf("[ 0,1]: %11.9f, %18.15f, %18.15f\n",x,x-Unpack8_0to1(byte),x-Unpack16_0to1(chawmp));

		// -1 to 1
		x = g_ReplayRand.GetRanged(-1.0f,1.0f) + g_ReplayRand.GetRanged(-1.0f,1.0f) / 16384.0f;
		byte = Pack8_n1to1(x);
		chawmp = Pack16_n1to1(x);
		printf("[-1,1]: %11.9f, %18.15f, %18.15f\n",x,x-Unpack8_n1to1(byte),x-Unpack16_n1to1(chawmp));

		// -2 to 2
		x = g_ReplayRand.GetRanged(-2.0f,2.0f) + g_ReplayRand.GetRanged(-2.0f,2.0f) / 16384.0f;
		byte = Pack8_n2to2(x);
		chawmp = Pack16_n2to2(x);
		printf("[-2,2]: %11.9f, %18.15f, %18.15f\n",x,x-Unpack8_n2to2(byte),x-Unpack16_n2to2(chawmp));

		Printf("\n");
	};

	////////////////////////////////////////////////////////////
	// test on vectors
	u32 packedBytes;
	u64 packedChawmps;

	Vector4 V0, V1, V2;
	//V0.Set(1.0f,-1.0f,g_ReplayRand.GetRanged(-1.0f,1.0f),g_ReplayRand.GetRanged(-1.0f,1.0f));
	V0.Set(1.0f,-1.0f,1.0f/3.0f,sqrtf(3.0f)/2.0f);

	// test byte packing
	PackNormalTo8s(packedBytes,V0);
	printf("%13.11f %13.11f %13.11f %13.11f\n",V0.x,V0.y,V0.z,V0.w);
	UnpackNormalFrom8s(V1,packedBytes);
	printf("%13.11f %13.11f %13.11f %13.11f\n",V1.x,V1.y,V1.z,V1.w);

	// test chawmp packing
	PackNormalTo16s(packedChawmps,V0);
	printf("%13.11f %13.11f %13.11f %13.11f\n",V0.x,V0.y,V0.z,V0.w);
	UnpackNormalFrom16s(V2,packedChawmps);
	printf("%13.11f %13.11f %13.11f %13.11f\n",V2.x,V2.y,V2.z,V2.w);

	Printf("\n");

	////////////////////////////////////////////////////////////
	// test eulers
	mthDisplayf("Testing on Euler vectors...");
	u32 packedEulers;
	Vector3 eulers;

	// test byte packing
	eulers.Set(PI,-PI,g_ReplayRand.GetRanged(-PI,PI));
	packedEulers = PackEulersTo32(eulers);
	printf("%13.11f %13.11f %13.11f\n",eulers.x,eulers.y,eulers.z);
	UnpackEulersFrom32(eulers,packedEulers);
	printf("%13.11f %13.11f %13.11f\n",eulers.x,eulers.y,eulers.z);

	eulers.Set(0.0f,g_ReplayRand.GetRanged(-PI,PI),g_ReplayRand.GetRanged(-PI,PI));
	packedEulers = PackEulersTo32(eulers);
	printf("%13.11f %13.11f %13.11f\n",eulers.x,eulers.y,eulers.z);
	UnpackEulersFrom32(eulers,packedEulers);
	printf("%13.11f %13.11f %13.11f\n",eulers.x,eulers.y,eulers.z);

	Printf("\n");

	////////////////////////////////////////////////////////////
	// speed tests
	printf("Testing speed...\n");

	passes = 10000000;

	sysTimer t;
	float usTime;
	
	for (i=0, x=0.0f, t.Reset(); i<passes; i++, x+=10.0f)
	{
		byte = Pack8_n1to1(x);
	}
	usTime = t.GetMsTime();
	printf("  pack  8bit: %f usec/pass, %f/usec\n",1000.0f*usTime/passes,passes/usTime/1000.0f);

	for (i=0, byte=0, t.Reset(); i<passes; i++, byte++)
	{
		x = Unpack8_n1to1(byte);
	}
	usTime = t.GetMsTime();
	printf("unpack  8bit: %f usec/pass, %f/usec\n",1000.0f*usTime/passes,passes/usTime/1000.0f);

	for (i=0, x=0.0f, t.Reset(); i<passes; i++, x+=10.0f)
	{
		chawmp = Pack16_n1to1(x);
	}
	usTime = t.GetMsTime();
	printf("  pack 16bit: %f usec/pass, %f/usec\n",1000.0f*usTime/passes,passes/usTime/1000.0f);

	for (i=0, chawmp=0, t.Reset(); i<passes; i++, chawmp++)
	{
		x = Unpack16_n1to1(byte);
	}
	usTime = t.GetMsTime();
	printf("unpack 16bit: %f usec/pass, %f/usec\n",1000.0f*usTime/passes,passes/usTime/1000.0f);

	for (i=0, x=0.0f, t.Reset(); i<passes; i++, x+=0.01f)
	{
		dword = Pack11_nPItoPI(x);
	}
	usTime = t.GetMsTime();
	printf("  pack 11bit: %f usec/pass, %f/usec\n",1000.0f*usTime/passes,passes/usTime/1000.0f);

	for (i=0, dword=0, t.Reset(); i<passes; i++, dword++)
	{
		x = Unpack11_nPItoPI(dword);
	}
	usTime = t.GetMsTime();
	printf("unpack 11bit: %f usec/pass, %f/usec\n",1000.0f*usTime/passes,passes/usTime/1000.0f);

	for (i=0, x=0.0f, t.Reset(); i<passes; i++, x+=0.01f)
	{
		dword = Pack10_nPItoPI(x);
	}
	usTime = t.GetMsTime();
	printf("  pack 10bit: %f usec/pass, %f/usec\n",1000.0f*usTime/passes,passes/usTime/1000.0f);

	for (i=0, dword=0, t.Reset(); i<passes; i++, dword++)
	{
		x = Unpack10_nPItoPI(dword);
	}
	usTime = t.GetMsTime();
	printf("unpack 10bit: %f usec/pass, %f/usec\n",1000.0f*usTime/passes,passes/usTime/1000.0f);

	return 0;
}
