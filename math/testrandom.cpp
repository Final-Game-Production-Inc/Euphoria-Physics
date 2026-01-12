//
// math/testrandom.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "random.h"

#include "system/main.h"

using namespace rage;


// below = number of gaussian random numbers that are above mean-variance
// below = number of gaussian random numbers that are below mean-variance
// inside = number of gaussian random numbers that are inside mean +- variance
void TestGaussianPart1()
{
	float mean = 1000.f;
	float variance = 100.f;
	float gaussian = g_DrawRand.GetGaussian(mean,variance);
	char *message;
	
	static int above = 0;
	static int below = 0;
	static int inside = 0;
	static int count = 0;

	if (gaussian < mean - variance)
	{
		message = "below",below++;
	}
	else if (gaussian > mean + variance)
	{
		message = "above",above++;
	}
	else
	{
		message = "inside",inside++;
	}

	count++;

	// only print every 100 tests
	if (count % 100 == 0)
	{
		mthDisplayf("Gaussian(%0.0f,%0.0f) = % 4.4f   (%d,%d,%d) \t%s",
			mean,
			variance,
			gaussian,
			below,inside,above,
			message);
	}
}


void TestGaussianPart2()
{
	// Test independence and resettability of Gaussian generation.
	mthRandom randomA;
	mthRandom randomB;
	float g0, g1, g2, g3;

	mthDisplayf("Runs 1 and 2 should be the same");
	randomA.Reset(1234);
	randomB.Reset(4321);
	g0 = randomA.GetGaussian(10.0f,1.0f);
	g1 = randomA.GetGaussian(10.0f,1.0f);
	g2 = randomB.GetGaussian(10.0f,1.0f);
	g3 = randomB.GetGaussian(10.0f,1.0f);
	mthDisplayf("Run 1: %f, %f, %f, %f",g0,g1,g2,g3);

	randomB.Reset(4321);
	g2 = randomB.GetGaussian(10.0f,1.0f);
	randomA.Reset(1234);
	g0 = randomA.GetGaussian(10.0f,1.0f);
	g1 = randomA.GetGaussian(10.0f,1.0f);
	g3 = randomB.GetGaussian(10.0f,1.0f);
	mthDisplayf("Run 2: %f, %f, %f, %f",g0,g1,g2,g3);
}


int Main()
{
	for (int i=0;i<100000;i++)
	{
		TestGaussianPart1();
	}

	TestGaussianPart2();

	return 0;
}
