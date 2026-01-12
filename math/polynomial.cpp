//
// math/polynomial.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "polynomial.h"

#include "amath.h"
#include "constants.h"
#include "simplemath.h"

namespace rage {


int RealQuadratic (float a1, float a0, float* solution1, float* solution2, float tolerance)
{
	float b2Minus4ac = square(a1)-4.0f*a0;
	float nearlyZero = tolerance*Max(fabsf(a1),fabsf(a0));
	if (b2Minus4ac>nearlyZero)
	{
		// The quantity in the square root is sufficiently positive, so there are exactly two distinct real solutions.
		float q = -0.5f*(a1+(a1>0.0f ? sqrtf(b2Minus4ac) : -sqrtf(b2Minus4ac)));
		if (solution1)
		{
			(*solution1) = q;
		}
		if (solution2)
		{
			(*solution2) = a0/q;
		}

		// Return 2 to indicate there are 2 distinct real solutions.
		return 2;
	}

	if (b2Minus4ac>=-nearlyZero)
	{
		// The quantity in the square root is nearly zero, so there is exactly one distinct real solution.
		if (solution1)
		{
			(*solution1) = -0.5f*a1;
		}

		// Return 1 to indicate there is only 1 distinct real solution.
		return 1;
	}

	// The quantity inside the square root is negative, so there are no real solutions.
	return 0;
}


int RealCubic (float a2, float a1, float a0, float* solution1, float* solution2, float* solution3, float tolerance)
{
	float a2Square = square(a2);
	float q = 0.111111111111f*a2Square-0.333333333333f*a1;
	float r = 0.5f*a0+0.037037037037f*a2*a2Square-0.166666666666f*a2*a1;		// 0.037037037037 is 1/27
	float r2 = square(r);
	float q3MinusR2 = power3(q)-r2;
	float nearlyZero = tolerance*Max(fabsf(a2),fabsf(a1),fabsf(a0));
	if (q3MinusR2>nearlyZero)
	{
		// There are three distinct real solutions.
		float sqrtQ = sqrtf(q);
		float thirdAngle = 0.333333333333f*AcosfSafe(r/(q*sqrtQ));
		float twoSqrtQ = 2.0f*sqrtQ;
		float thirdA2 = 0.333333333333f*a2;
		if (solution1)
		{
			(*solution1) = -twoSqrtQ*rage::Cosf(thirdAngle)-thirdA2;
		}
		float twoThirdsPi = 0.666666666667f*PI;
		if (solution2)
		{
			(*solution2) = -twoSqrtQ*rage::Cosf(thirdAngle+twoThirdsPi)-thirdA2;
		}
		if (solution3)
		{
			(*solution3) = -twoSqrtQ*rage::Cosf(thirdAngle-twoThirdsPi)-thirdA2;
		}
		return 3;
	}

	// There is exactly one distinct real solution.
	float s = -CubeRoot(r + (r>0.0f ? SqrtfSafe(-q3MinusR2) : -SqrtfSafe(-q3MinusR2)));
	if (solution1)
	{
		(*solution1) = s + (fabsf(s)>VERY_SMALL_FLOAT ? q/s : 0.0f) - 0.333333333333f*a2;
	}
	return 1;
}


/*
PURPOSE:	Get a pointer to fill in the next polynomial solution.
NOTES:		This used by RealQuartic. 
*/
float* GetNextSolution (int numSolutions, float* solution1, float* solution2, float* solution3, float* solution4)
{
	switch (numSolutions)
	{
		case 0:
			return solution1;
		case 1:
			return solution2;
		case 2:
			return solution3;
		case 3:
			return solution4;
		default:
			return NULL;
	}
}


int RealQuartic (float a3, float a2, float a1, float a0, float* solution1, float* solution2, float* solution3,
					float* solution4, float tolerance)
{
	// Set up and solve the resolvent cubic equation.
	float fourA0 = 4.0f*a0;
	float squareA3 = square(a3);
	float cubicSol;
	RealCubic(-a2,a1*a3-fourA0,fourA0*a2-square(a1)-squareA3*a0,&cubicSol);

	float* nextSolution;
	int numSolutions = 0;
	float r2 = 0.25f*squareA3-a2+cubicSol;
	float nearlyZero = 0.01f*tolerance*Max(fabsf(a3),fabsf(a2),fabsf(a1),fabsf(a0));
	if (r2>nearlyZero)
	{
		float r = sqrtf(r2);
		float halfR = 0.5f*r;
		float minusQuarterA3 = -0.25f*a3;
		float part1 = 0.75f*squareA3-r2-2.0f*a2;
		float part2 = (a3*a2-2.0f*a1-0.25f*a3*square(a3))/r;
		float d2 = part1+part2;
		if (d2>SMALL_FLOAT)
		{
			float halfD = 0.5f*sqrtf(d2);
			nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
			numSolutions++;
			if (nextSolution)
			{
				(*nextSolution) = minusQuarterA3+halfR+halfD;
			}
			nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
			numSolutions++;
			if (nextSolution)
			{
				(*nextSolution) = minusQuarterA3+halfR-halfD;
			}
		}
		else if (d2>-SMALL_FLOAT)
		{
			nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
			numSolutions++;
			if (nextSolution)
			{
				(*nextSolution) = minusQuarterA3+halfR;
			}
		}

		if (fabsf(part2)>SMALL_FLOAT)
		{
			// d and e are not nearly the same.
			float e2 = part1-part2;
			if (e2>SMALL_FLOAT)
			{
				float halfE = 0.5f*sqrtf(e2);
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3-halfR+halfE;
				}
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3-halfR-halfE;
				}
			}
			else if (e2>-SMALL_FLOAT)
			{
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3-halfR;
				}
			}
		}
	}
	else if (r2>-nearlyZero)
	{
		// This is inaccurate, don't know why. Until it's fixed, return no solutions.
/*		float minusQuarterA3 = -0.25f*a3;
		float part2 = square(cubicSol)-fourA0;
		if (part2>SMALL_FLOAT)
		{
			part2 = 2.0f*sqrtf(part2);
			float part1 = 0.75f*squareA3-2.0f*a2;
			float d2 = part1+part2;
			if (d2>SMALL_FLOAT)
			{
				float halfD = 0.5f*sqrtf(d2);
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3+halfD;
				}
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3-halfD;
				}
			}
			else if (d2>-SMALL_FLOAT)
			{
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3;
				}
			}

			float e2 = part1-part2;
			if (e2>SMALL_FLOAT)
			{
				float halfE = 0.5f*sqrtf(e2);
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3+halfE;
				}
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3-halfE;
				}
			}
			else if (e2>-SMALL_FLOAT && fabsf(d2)>SMALL_FLOAT)
			{
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3;
				}
			}
		}
		else if (part2>-SMALL_FLOAT)
		{
			float d2 = 0.75f*squareA3-2.0f*a2;
			if (d2>SMALL_FLOAT)
			{
				float halfD = 0.5f*sqrtf(d2);
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3+halfD;
				}
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3-halfD;
				}
			}
			else if (d2>-SMALL_FLOAT)
			{
				nextSolution = GetNextSolution(numSolutions,solution1,solution2,solution3,solution4);
				numSolutions++;
				if (nextSolution)
				{
					(*nextSolution) = minusQuarterA3;
				}
			}
		}*/
	}

	// Return the number of real solutions.
	return numSolutions;
}

} //namespace rage
