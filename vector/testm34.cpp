//
// vector/testm34.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "matrix34.h"
#include "quaternion.h"
#include "vector4.h"

#include "file/stream.h"
#include "file/asset.h"
#include "math/random.h"
#include "math/simplemath.h"
#include "system/main.h"
#include "system/timer.h"

#include <stdio.h>

#define DEFAULT_PATH "C:"

using namespace rage;

/*
matrix { a: 102.000000,108.000000,114.000000
 b: 246.000000,261.000000,276.000000
 c: 390.000000,414.000000,438.000000
 d: 556.000000,590.000000,624.000000
}
*/

/*
C version 260
__dot3 version 184
old version 97
rage_new version	60
*/


void Check(const Matrix34 &m,const Vector3 *in,Vector3 *out,int count) {
	for (int i=0; i<count; i++)
	{
		out[i].x = in[i].x*m.a.x + in[i].y*m.b.x + in[i].z*m.c.x + m.d.x;
		out[i].y = in[i].x*m.a.y + in[i].y*m.b.y + in[i].z*m.c.y + m.d.y;
		out[i].z = in[i].x*m.a.z + in[i].y*m.b.z + in[i].z*m.c.z + m.d.z;
		out[i].w = 0;
	}
}


void MakeInvMassMatrix (const Vector3& relPos, const Matrix34& axes, const Vector3& invAngInertia, float invMass,
						Matrix34& matrix)
{
	// Dot the ICS matrix with the cross product matrix of the response position.
	Matrix34 icsDotCross(axes);
	icsDotCross.Dot3x3CrossProdMtx(relPos);

	// Dot the inverse angular inertia with the previous matrix.
	Matrix34 tranInvAngInertia;
	tranInvAngInertia.a.Scale(icsDotCross.a,invAngInertia.x);
	tranInvAngInertia.b.Scale(icsDotCross.b,invAngInertia.y);
	tranInvAngInertia.c.Scale(icsDotCross.c,invAngInertia.z);

	// Transpose the ICS dotted with the cross product matrix, and dot it with the transformed inverse angular inertia.
	icsDotCross.Transpose();
	matrix.Dot3x3(icsDotCross,tranInvAngInertia);

	// Add the inverse mass to the diagonal elements and zero the position vector.
	matrix.a.x += invMass;
	matrix.b.y += invMass;
	matrix.c.z += invMass;
	matrix.d.Zero();
}


float Rand (float max)
{
	return g_ReplayRand.GetRanged(-max,max);
}


#if 0
inline int unaligned_read(void *ptr) {
	int result;
	__asm__("lwl %0, 3(%1); lwr %0, 0(%1)" : "=&r"(result) : "r"(ptr));
	return result;
}
#endif

int Main()
{

#if __PS2
	iopManager::InitClass();
#endif // __PS2

//	new datAssetManagerHier;
	ASSET.SetPath(DEFAULT_PATH);

	//////////////////////////
	// test Matrix34::Dot(a,b)
	Matrix34 A(1,2,3,4,5,6,7,8,9,10,11,12);
	Matrix34 B(13,14,15,16, 17,18,19,20, 21,22,23,24);
	Matrix34 C;
	Vector3 D(1,2,3);
	Vector4 E;
	Vector3 F = D, G = D, H = D, I;
	C.Dot(A,B);
	utimer_t t = sysTimer::GetTicks();
	C.Dot(A,B);
	// A.Dot(B);
	t = sysTimer::GetTicks() - t;
	A.Print("\n");
	C.Print("\n");
	printf("Matrix34.Dot - %d ticks\n",(int)t);

#if 0
	A.TransformAligned(D,E);
	t = sysTimer::GetTicks();
	A.TransformAligned(D,E);
	t = sysTimer::GetTicks() - t;
	// 40 47 54 is the correct answer
	E.w = 0.0f;
	E.Print("\n");
	printf("TransformAligned - %d ticks\n",t);
	A.TransformUnaligned(F,I);
	A.TransformUnaligned(G,I);
	A.TransformUnaligned(H,I);
	t = sysTimer::GetTicks();
	A.TransformUnaligned(H,I);
	t = sysTimer::GetTicks() - t;
	I.Print("\n");
	mthDisplayf("TransformUnaligned - %d ticks [%p %p %p]",t,&F,&G,&H);
#endif
#if 0
	static char foo[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	mthDisplayf("Test 0 %x",unaligned_read(foo+0));
	mthDisplayf("Test 1 %x",unaligned_read(foo+1));
	mthDisplayf("Test 2 %x",unaligned_read(foo+2));
	mthDisplayf("Test 3 %x",unaligned_read(foo+3));
	mthDisplayf("Test 4 %x",unaligned_read(foo+4));
#endif

	////////////////////////////
	// test Matrix34::Transform4
	const int count = 256;
	Vector3 inv[count];
	Vector3 outv[count];
	Vector3 outv2[count];
	int i;
	for (i=0; i<count; i++)
		inv[i].Set(i*0.333f,i*0.745f,i*-3.34f);
	for (i=0; i<count; i++)
		A.Transform(inv[i],outv[i]);
	t = sysTimer::GetTicks();
	for (i=0; i<count; i+=4) {
		A.Transform(inv[i],outv[i]);
		A.Transform(inv[i+1],outv[i+1]);
		A.Transform(inv[i+2],outv[i+2]);
		A.Transform(inv[i+3],outv[i+3]);
	}
	t = sysTimer::GetTicks() - t;
	// 953 ticks before.
	// 730 ticks after basic cleanup
	// 600 ticks after moving a few more insns around.
	// xenon release: 5.37us new method.  old method 3.3!  sigh.
	// old method, unrolled: 2.36us.
	mthDisplayf("Matrix34::Transform %f us",t * sysTimer::GetTicksToMicroseconds());
	Check(A,inv,outv2,count);
	for (i=0; i<count; i++)
	{
		if (fabsf(outv[i].Mag() - outv2[i].Mag()) > 0.1f * 0.1f)
		{
			Printf("%d. ",i);
			outv[i].Print("outv");
			outv2[i].Print("outv2");
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	// test Matrix34 orthonormality with roundoff errors from repeated small rotations
	Matrix34 rotationTest(M34_IDENTITY);
	Vector3 rotationAxis;
	mthRandom random;
	float rotationAngle;
	float allowedError = 4.0e-2f;		// XBOX 360
	//float allowedError = 1.0e-4f;		// PC
	float maxAngle = 0.1f;
	const int numRotations = 1000000; // 1 million
	for (int rotationIndex=0; rotationIndex<numRotations; rotationIndex++)
	{
		rotationAngle = random.GetRanged(-maxAngle,maxAngle);
		rotationAxis.Set(random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f));
		rotationAxis.Normalize();
		rotationTest.RotateUnitAxis(rotationAxis,rotationAngle);
		if (!rotationTest.IsOrthonormal(allowedError))
		{
			mthErrorf("Matrix34 orthonormality failed after %i rotations",rotationIndex);
			DebugAssert(0);
		}
	}

	//////////////////////////////////////////////
	// test different methods of rotating matrices
	Quaternion prevQ,nextQ;
	rotationTest.Identity();
	Matrix34 rotationTestT(M34_IDENTITY);
	Vector3 unitFrom,unitTo,vertical,axis,localAxis,rotatedLocalAxis;
	maxAngle = 2.0f*PI;
	allowedError = 2.0e-1f;			// XBOX 360
	//allowedError = 1.0e-1f;		// PC
	float makeUprightError = 1.0e-3f;
	int testIndex,numTests=100000;	// hundred thousand
	for (testIndex=0; testIndex<numTests; testIndex++)
	{
		// Test Matrix34::RotateTo (rotates the matrix by the same rotation required to rotate between the two given vectors).
		rotationAngle = random.GetRanged(-maxAngle,maxAngle);
		unitFrom.Set(random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f));
		unitFrom.Normalize();
		unitTo.Set(random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f));
		unitTo.Normalize();
		prevQ.FromMatrix34(rotationTest);
		rotationTest.RotateTo(unitFrom,unitTo);
		nextQ.FromMatrix34(rotationTest);
		float angleQ = nextQ.RelAngle(prevQ);
		float angleM = acosf(unitFrom.Dot(unitTo));
		if (!IsNearZero(angleQ-angleM,allowedError))
		{
			DebugAssert(0);
			mthErrorf("Matrix34::RotateTo conflicted with Quaternion::RelAngle");
		}

		// Test Matrix34::MakeRotateTo (makes the matrix the rotation between the two given vectors).
		rotationTest.Identity();
		rotationTest.MakeRotateTo(unitFrom,unitTo);
		prevQ.Identity();
		nextQ.FromMatrix34(rotationTest);
		angleQ = nextQ.RelAngle(prevQ);
		if (!IsNearZero(angleQ-angleM,allowedError))
		{
			DebugAssert(0);
			mthErrorf("Matrix34::MakeRotateTo conflicted with Quaternion::RelAngle");
		}

		// Test Matrix34::RotateTo with a t-value argument
		float t = random.GetRanged(0.0f,1.0f);
		prevQ.FromMatrix34(rotationTestT);
		rotationTestT.RotateTo(unitFrom,unitTo,t);
		nextQ.FromMatrix34(rotationTestT);
		angleQ = nextQ.RelAngle(prevQ);
		angleM *= t;
		if (!IsNearZero(angleQ-angleM,allowedError))
		{
			DebugAssert(0);
			mthErrorf("Matrix34::RotateTo conflicted with Quaternion::RelAngle");
		}

		// Test Matrix34::MakeUpright
		rotationTest.Identity();
		angleM = random.GetRanged(0.0f,PI);
		rotationTest.RotateLocalX(angleM);
		angleM = random.GetRanged(0.0f,PI);
		rotationTest.RotateLocalZ(angleM);
		rotationTestT.Set(rotationTest);
		rotationTestT.MakeUpright();
		rotationTestT.Transform3x3(g_UnitUp,vertical);
		if (!vertical.IsClose(g_UnitUp,makeUprightError))
		{
			DebugAssert(0);
			mthErrorf("Matrix34::MakeUpright didn't make an upright matrix.");
		}
		axis.Cross(g_UnitUp,vertical);
		rotationTest.UnTransform3x3(axis,localAxis);
		rotationTestT.UnTransform3x3(axis,rotatedLocalAxis);
		if (!rotatedLocalAxis.IsClose(localAxis,makeUprightError))
		{
			DebugAssert(0);
			mthErrorf("Matrix34::MakeUpright did a rotation about the vertical direction.");
		}
	}

	//////////////////////////
	// test orthonormalization
	Matrix34 identity;
	const float nonUnitError = 0.1f;
	const float maxPos = 1000.0f;
	allowedError = 1.0e-4f;
	numTests = 10000;
	for (testIndex=0; testIndex<numTests; testIndex++)
	{
		A.a.Set(random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f));
		A.b.Set(random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f));
		A.c.Set(random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f),random.GetRanged(-1.0f,1.0f));
		A.NormalizeSafe();
		if (!A.IsOrthonormal())
		{
			DebugAssert(0);
			mthErrorf("Matrix34::NormalizeSafe produced a non-orthonormal matrix.");
		}

		A.d.Set(random.GetRanged(-maxPos,maxPos),random.GetRanged(-maxPos,maxPos),random.GetRanged(-maxPos,maxPos));
		B.CoordinateInverseSafe(A);
		C.Set(A);
		C.CoordinateInverseSafe();
		identity.Dot(A,B);
		if (!identity.IsClose(M34_IDENTITY,allowedError))
		{
			DebugAssert(0);
			mthErrorf("Matrix34::CoordinateInverseSafe produced a non-inverse matrix.");
		}

		identity.Dot(A,C);
		if (!identity.IsClose(M34_IDENTITY,allowedError))
		{
			DebugAssert(0);
			mthErrorf("Matrix34::CoordinateInverseSafe produced a non-inverse matrix.");
		}

		A.a.x += random.GetRanged(-nonUnitError,nonUnitError);
		A.a.y += random.GetRanged(-nonUnitError,nonUnitError);
		A.a.z += random.GetRanged(-nonUnitError,nonUnitError);
		A.b.x += random.GetRanged(-nonUnitError,nonUnitError);
		A.b.y += random.GetRanged(-nonUnitError,nonUnitError);
		A.b.x += random.GetRanged(-nonUnitError,nonUnitError);
		A.c.y += random.GetRanged(-nonUnitError,nonUnitError);
		A.c.z += random.GetRanged(-nonUnitError,nonUnitError);
		A.c.z += random.GetRanged(-nonUnitError,nonUnitError);
		C.Set(A);
		B.CoordinateInverseSafe(A);
		A.CoordinateInverseSafe();
		if (!A.IsOrthonormal())
		{
			DebugAssert(0);
			mthErrorf("Matrix34::CoordinateInverseSafe produced a non-orthonormal matrix.");
		}
		if (!B.IsOrthonormal())
		{
			DebugAssert(0);
			mthErrorf("Matrix34::CoordinateInverseSafe produced a non-orthonormal matrix.");
		}
	}

#if 0
	//////////////////////////
	// test Matrix34::SolveSVD
	const int numMatrices = 100;
	const int timesPerMatrix = 400;
	sysTimer timer;
	Matrix34 axes,invMassMatrix,randMatrix;
	Vector3 relPos,invAngInertia(0.2f,0.5f,0.8f),relVel,impulse,impulseOld,relVelVerify,in,out,outOld,inVerify;
	float secsIMM=0.0f,secsOldIMM=0.0f,secsRand=0.0f,secsOldRand=0.0f,invMass=0.1f,ms,RM=10.0f;
	bool rightAnswer;
	for (int mtxIndex=0; mtxIndex<numMatrices; mtxIndex++)
	{
		relPos.Set(Rand(4.0f),Rand(4.0f),Rand(4.0f));
		axes.a.Set(Rand(1.0f),Rand(1.0f),Rand(1.0f));
		axes.a.Normalize();
		axes.b.Set(Rand(1.0f),Rand(1.0f),Rand(1.0f));
		axes.c.Cross(axes.a,axes.b);
		axes.c.Normalize();
		axes.b.Cross(axes.c,axes.a);
		MakeInvMassMatrix(relPos,axes,invAngInertia,invMass,invMassMatrix);
		for (i=0; i<timesPerMatrix; i++)
		{
			relVel.Set(Rand(10.0f),Rand(10.0f),Rand(10.0f));
			ms = timer.MsTime();
			rightAnswer = invMassMatrix.SolveSVD(relVel,impulse);
			ms = timer.MsTime()-ms;
			secsIMM += ms*0.001f;
			ms = timer.MsTime();
			invMassMatrix.SolveSVDOld(relVel,impulseOld);
			ms = timer.MsTime()-ms;
			secsOldIMM += ms*0.001f;
			DebugAssert(impulse.IsClose(impulseOld,0.01f));
			// Verify that this is a nearly exact solution.
			invMassMatrix.Transform3x3(impulse,relVelVerify);
			DebugAssert(rightAnswer && relVel.IsClose(relVelVerify,0.01f));
		}
	}
	for (int mtxIndex=0; mtxIndex<numMatrices; mtxIndex++)
	{
		randMatrix.a.Set(Rand(RM),Rand(RM),Rand(RM));
		randMatrix.b.Set(2.0f*randMatrix.a.x,2.0f*randMatrix.a.y,2.0f*randMatrix.a.z);
		randMatrix.c.Set(5.0f*randMatrix.a.x,5.0f*randMatrix.a.y,5.0f*randMatrix.a.z);
		randMatrix.d.Zero();
		for (i=0; i<timesPerMatrix; i++)
		{
			in.Set(Rand(RM),Rand(RM),Rand(RM));
			ms = timer.MsTime();
			rightAnswer = randMatrix.SolveSVD(in,out);
			ms = timer.MsTime()-ms;
			secsRand += ms*0.001f;
			ms = timer.MsTime();
			randMatrix.SolveSVDOld(in,outOld);
			ms = timer.MsTime()-ms;
			secsOldRand += ms*0.001f;
			DebugAssert(out.IsClose(outOld,0.01f));
			if (!out.IsClose(outOld,0.01f))
			{
				randMatrix.SolveSVD(in,out);
				randMatrix.SolveSVDOld(in,outOld);
			}
			randMatrix.Transform3x3(out,inVerify);
			DebugAssert(rightAnswer==in.IsClose(inVerify,0.00001f));
			if (rightAnswer!=in.IsClose(inVerify,0.00001f))
			{
				randMatrix.SolveSVD(in,out);
			}
		}
	}
	float thousandOverNumTimes = 1000.0f/(float)(numMatrices*timesPerMatrix);
	float aveMsIMM = secsIMM*thousandOverNumTimes;
	float aveMsOldIMM = secsOldIMM*thousandOverNumTimes;
	float aveMsRand = secsRand*thousandOverNumTimes;
	float aveMsOldRand = secsOldRand*thousandOverNumTimes;
	const char* fileName = 0;
	ARGS.Get("file",0,&fileName);
	if (fileName)
	{
		Stream* resultsFile = ASSET.Create("testresults",fileName,"txt");
		if (resultsFile) 
		{
			fprintf(resultsFile,"SolveSVD results\n");
			fprintf(resultsFile,"Inverse Mass Matrix:\n");
			fprintf(resultsFile,"\t new total: %f seconds\n",secsIMM);
			fprintf(resultsFile,"\t old total: %f seconds\n",secsOldIMM);
			fprintf(resultsFile,"\t new average: %f ms\n",aveMsIMM);
			fprintf(resultsFile,"\t old average: %f ms\n",aveMsOldIMM);
			fprintf(resultsFile,"Random Matrix:\n");
			fprintf(resultsFile,"\t new total: %f seconds\n",secsRand);
			fprintf(resultsFile,"\t old total: %f seconds\n",secsOldRand);
			fprintf(resultsFile,"\t new average: %f ms\n",aveMsRand);
			fprintf(resultsFile,"\t old average: %f ms\n",aveMsOldRand);
			ASSET.Close(resultsFile);
		}
	}
	mthDisplayf("\n\nSolveSVD results:");
	mthDisplayf("Inverse Mass Matrix:");
	mthDisplayf("\tnew total: %f seconds",secsIMM);
	mthDisplayf("\told total: %f seconds",secsOldIMM);
	mthDisplayf("\tnew average: %f ms",aveMsIMM);
	mthDisplayf("\told average: %f ms",aveMsOldIMM);
	mthDisplayf("Random Matrix:");
	mthDisplayf("\tnew total: %f seconds",secsRand);
	mthDisplayf("\told total: %f seconds",secsOldRand);
	mthDisplayf("\tnew average: %f ms",aveMsRand);
	mthDisplayf("\told average: %f ms",aveMsOldRand);
#endif

	return 1;
}
