//
// math/testpolynomial.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

// tester for polynomial equation solvers

#include "amath.h"
#include "polynomial.h"
#include "random.h"
#include "simplemath.h"

#include "system/main.h"

using namespace rage;

int Main ()
{
	// Set the number of tests to run and the estimated solution order of magnitude.
	const int numTests = 1000;
	const float estSolMag = 10.0f;
	const float estSolMag2 = square(estSolMag);
	const float estSolMag3 = estSolMag*estSolMag2;
	const float estSolMag4 = estSolMag*estSolMag3;


#if __DEV
	const float fractionErrorQuadratic = 1.0e-6f;
	const float fractionErrorCubic = 1.0e-6f;
	const float fractionErrorQuartic = 1.0e-3f;
	const float allowedErrorQuadratic = fractionErrorQuadratic*estSolMag2;
	const float allowedErrorCubic = fractionErrorCubic*estSolMag3;
	const float allowedErrorQuartic = fractionErrorQuartic*estSolMag4;
#endif

	float testZero;
	float a0,a1,a2,a3;
	float solutionA,solutionB,solutionC,solutionD;
	int numSolutions;
	for (int index=0; index<numTests; index++)
	{
		// Test the second-order equation solver RealQuadratic.
		a0 = g_ReplayRand.GetRanged(-estSolMag2,estSolMag2);
		a1 = g_ReplayRand.GetRanged(-estSolMag,estSolMag);
		numSolutions = RealQuadratic(a1,a0,&solutionA,&solutionB);
		if (numSolutions>0)
		{
			testZero = square(solutionA)+a1*solutionA+a0;
			DebugAssert(fabsf(testZero)<allowedErrorQuadratic);
			if (numSolutions>1)
			{
				testZero = square(solutionB)+a1*solutionB+a0;
				DebugAssert(fabsf(testZero)<allowedErrorQuadratic);
			}
		}

		// Test the third-order equation solver RealCubic.
		a0 = g_ReplayRand.GetRanged(-estSolMag3,estSolMag3);
		a1 = g_ReplayRand.GetRanged(-estSolMag2,estSolMag2);
		a2 = g_ReplayRand.GetRanged(-estSolMag,estSolMag);
		numSolutions = RealCubic(a2,a1,a0,&solutionA,&solutionB,&solutionC);
		if (numSolutions>0)
		{
			testZero = power3(solutionA)+a2*square(solutionA)+a1*solutionA+a0;
			DebugAssert(fabsf(testZero)<allowedErrorCubic);
			if (numSolutions>1)
			{
				testZero = power3(solutionB)+a2*square(solutionB)+a1*solutionB+a0;
				DebugAssert(fabsf(testZero)<allowedErrorCubic);
				if (numSolutions>2)
				{
					testZero = power3(solutionC)+a2*square(solutionC)+a1*solutionC+a0;
					DebugAssert(fabsf(testZero)<allowedErrorCubic);
				}
			}
		}

		// Test the fourth-order equation solver RealQuartic.
		a0 = g_ReplayRand.GetRanged(-estSolMag4,estSolMag4);
		a1 = g_ReplayRand.GetRanged(-estSolMag3,estSolMag3);
		a2 = g_ReplayRand.GetRanged(-estSolMag2,estSolMag2);
		a3 = g_ReplayRand.GetRanged(-estSolMag,estSolMag);
		numSolutions = RealQuartic(a3,a2,a1,a0,&solutionA,&solutionB,&solutionC,&solutionD);
		if (numSolutions>0)
		{
			testZero = power4(solutionA)+a3*power3(solutionA)+a2*square(solutionA)+a1*solutionA+a0;
			DebugAssert(fabsf(testZero)<allowedErrorQuartic);
			if (numSolutions>1)
			{
				testZero = power4(solutionB)+a3*power3(solutionB)+a2*square(solutionB)+a1*solutionB+a0;
				DebugAssert(fabsf(testZero)<allowedErrorQuartic);
				if (numSolutions>2)
				{
					testZero = power4(solutionC)+a3*power3(solutionC)+a2*square(solutionC)+a1*solutionC+a0;
					DebugAssert(fabsf(testZero)<allowedErrorQuartic);
					if (numSolutions>3)
					{
						testZero = power4(solutionD)+a3*power3(solutionD)+a2*square(solutionD)+a1*solutionD+a0;
						DebugAssert(fabsf(testZero)<allowedErrorQuartic);
					}
				}
			}
		}
	}

	return 0;
}
