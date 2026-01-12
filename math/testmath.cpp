//
// math/testmath.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "amath.h"
#include "random.h"
#include "simplemath.h"

#include "system/main.h"

#include <stdio.h>

#define TEST(x)	printf(#x " = %.10f\n", x);

using namespace rage;
/*
	PC:
	sqrtf(123456.789f) = 351.3641829534
	invsqrtf(123456.789f) = 0.0028460498
	invsqrtf_fast(123456.789f) = 0.0028460499

	Gamecube, invsqrtf * f
	sqrtf(123456.789f) = 351.2147483647
	invsqrtf(123456.789f) = 0.0028460500
	invsqrtf_fast(123456.789f) = 0.0028460380	
*/

int Main() {
	TEST(sqrtf(123456.789f));
	TEST(invsqrtf(123456.789f));
	TEST(invsqrtf_fast(123456.789f));
	TEST(invsqrtf(0));
	TEST(invsqrtf_fast(0));
	for (int i = 30; i > -30; --i)
	{
		printf("\n*** 10^%d ***\n", i);
		TEST(sqrtf(pow(10.0f, i)));
		TEST(invsqrtf(pow(10.0f, i)));
		TEST(invsqrtf_fast(pow(10.0f, i)));
	}

	// Test maximum and minimum index functions in math/simplemath.
	const int numTests = 10000;
	const float minValue = -1.0e4f;
	const float maxValue = 1.0e4f;
	float a,b,c,d;
	mthRandom randomGenerator;
	for (int testIndex=0; testIndex<numTests; testIndex++)
	{
		a = randomGenerator.GetRanged(minValue,maxValue);
		b = randomGenerator.GetRanged(minValue,maxValue);
		if (!(((MaximumIndex(a,b)==0) && a>=b) || ((MaximumIndex(a,b)==1) && a<b)))
		{
			mthFatalf("Error in MaximumIndex(a,b)");
		}
		if (!(((MinimumIndex(a,b)==0) && a<=b) || ((MinimumIndex(a,b)==1) && a>b)))
		{
			mthFatalf("Error in MinimumIndex(a,b)");
		}
		c = randomGenerator.GetRanged(minValue,maxValue);
		if (!(((MaximumIndex(a,b,c)==0) && a>=b && a>=c) ||
				((MaximumIndex(a,b,c)==1) && a<b && b>=c) ||
				((MaximumIndex(a,b,c)==2) && a<c && b<c)))
		{
			mthFatalf("Error in MaximumIndex(a,b,c)");
		}
		if (!(((MinimumIndex(a,b,c)==0) && a<=b && a<=c) ||
				((MinimumIndex(a,b,c)==1) && a>b && b<=c) ||
				((MinimumIndex(a,b,c)==2) && a>c && b>c)))
		{
			mthFatalf("Error in MinimumIndex(a,b,c)");
		}
		d = randomGenerator.GetRanged(minValue,maxValue);
		if (!(((MaximumIndex(a,b,c,d)==0) && a>=b && a>=c && a>=d) ||
				((MaximumIndex(a,b,c,d)==1) && a<b && b>=c && b>=d) ||
				((MaximumIndex(a,b,c,d)==2) && a<c && b<c && c>=d) ||
				((MaximumIndex(a,b,c,d)==3) && a<d && b<d && c<d)))
		{
			mthFatalf("Error in MaximumIndex(a,b,c,d)");
		}
		if (!(((MinimumIndex(a,b,c,d)==0) && a<=b && a<=c && a<=d) ||
				((MinimumIndex(a,b,c,d)==1) && a>b && b<=c && b<=d) ||
				((MinimumIndex(a,b,c,d)==2) && a>c && b>c && c<=d) ||
				((MinimumIndex(a,b,c,d)==3) && a>d && b>d && c>d)))
		{
			mthFatalf("Error in MinimumIndex(a,b,c,d)");
		}
	}

	/* This tester attempts to answer the question:

		"What is the smallest number that can be passed to sqrtf that returns a non-zero result?"

	That's a very important question, because the result of the square root is often used as a
	divisor, and zero divisors are bad.  It also has implications for what constitues a valid
	vector.  A vector whose components are so small that the vectors length is zero is effectively
	a zero vector for many purposes.

	The answer to this question is encoded in the AGE code base as SMALLEST_SQUARE.  That's a
	slightly strange name, because it's really the smallest square root, but hey...

	It turns out that on our platforms (PS2, Xbox, and Win32), the smallest such vector is very small
	indeed.  On PS2, it's 1.17549e-38f (0x00800000), which isn't quite the smallest representable number,
	but it's close.  There are smaller numbers that exist, which return zero for square root.

	On Win32, there is no non-zero number that doesn't give a non-zero result. */
#define NEW_TEST_METHOD 1
#if NEW_TEST_METHOD
	float smallestNonZero = 1.0f;

	for (unsigned int testInt = 0; testInt < 0x0000FFFF; ++testInt)
	{
		float testFloat = *reinterpret_cast<float*>(&testInt);

		if (testFloat > 0.0f &&
			smallestNonZero > testFloat &&
			sqrtf(testFloat) > 0)
		{
			smallestNonZero = testFloat;
		}
	}

	if (smallestNonZero>SMALLEST_SQUARE)
	{
		printf("smallest square correct!\n");
	}
	else if (smallestNonZero<SMALLEST_SQUARE)
	{
		printf("smallest square too big!\n");
	}
#else
	float supremum = 1.0f;
	float infimum = 0.0f;
	float test = 0.5f;

	while (true)
	{
		float oldTest = test;
		test = (supremum + infimum) * 0.5f;

		if (test==supremum || test==infimum)
		{
			printf("%1.30f\n", oldTest);

			if (oldTest>SMALLEST_SQUARE)
			{
				printf("smallest square correct!\n");
			}
			else if (oldTest<SMALLEST_SQUARE)
			{
				printf("smallest square too big!\n");
			}

			return 0;
		}

		if (sqrtf(test) > 0.0f)
		{
			supremum = test;
		}
		else
		{
			infimum = test;
		}
	}
#endif
	return 0;
}
