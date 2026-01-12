//
// math/polynomial.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef MATH_POLYNOMIAL_H
#define MATH_POLYNOMIAL_H

#define	DEFAULT_ROOT_TOLERANCE	0.001f	// for a float to be near zero

namespace rage {

/*
	PURPOSE
		This file provides a collection of functions for solving polynomial equations,
		including second, third and fourth order. Fifth and higher order equations
		are better solved by iterative methods if they are needed.
	<FLAG Component>
*/


// PURPOSE: Find the real solutions to x^2+a1*x+a0=0.
// PARAMS:
//	a1 -		the multiplier for x in x^2+a1*x+a0=0
//	a0 -		the constant term in x^2+a1*x+a0=0
//	solution1 -	optional pointer in which to copy the first real solution, if there is one
//	solution2 -	optional pointer in which to copy the second real solution, if there are two
//	tolerance -	optional maximum absolute value for a number to be considered non-zero
// RETURN:	the number of distinct real solutons (0, 1 or 2)
// NOTES:	The method is chosen for numerical accuracy, from Numerical Recipes in C. 
int RealQuadratic (float a1, float a0, float* solution1=NULL, float* solution2=NULL,
					float tolerance=DEFAULT_ROOT_TOLERANCE);

// PURPOSE: Find the real solutions to x^3+a2*x^2+a1*x+a0=0.
// PARAMS:
//	a2 -		the multiplier for x^2 in x^3+a2*x^2+a1*x+a0=0
//	a1 -		the multiplier for x in x^3+a2*x^2+a1*x+a0=0
//	a0 -		the constant term in x^3+a2*x^2+a1*x+a0=0
//	solution1 -	optional pointer in which to copy the first real solution, if there is one
//	solution2 -	optional pointer in which to copy the second real solution, if there are two
//	solution3 -	optional pointer in which to copy the third real solution, if there are three
//	tolerance -	optional maximum absolute value for a number to be considered non-zero
// RETURN:	the number of distinct real solutons (1, 2 or 3, never zero)
// NOTES:	The method is from Numerical Recipes in C. 
int RealCubic (float a2, float a1, float a0, float* solution1=NULL, float* solution2=NULL, float* solution3=NULL,
				float tolerance=DEFAULT_ROOT_TOLERANCE);

// PURPOSE:	Find the real solutions to x^4+a3*x^3+a2*x^2+a1*x+a0=0.
// PARAMS:
//	a3 -		the multiplier for x^3 in x^4+a3*x^3+a2*x^2+a1*x+a0=0
//	a2 -		the multiplier for x^2 in x^4+a3*x^3+a2*x^2+a1*x+a0=0
//	a1 -		the multiplier for x in x^4+a3*x^3+a2*x^2+a1*x+a0=0
//	a0 -		the constant term in x^4+a3*x^3+a2*x^2+a1*x+a0=0
//	solution1 -	optional pointer in which to copy the first real solution, if there is one
//	solution2 -	optional pointer in which to copy the second real solution, if there are two
//	solution3 -	optional pointer in which to copy the third real solution, if there are three
//	solution4 -	optional pointer in which to copy the fourth real solution, if there are four
//	tolerance -	optional maximum absolute value for a number to be considered non-zero
// RETURN: the number of distinct real solutons (0, 1, 2, 3 or 4)
// NOTES:	This uses a solution method from Mathematica (http://mathworld.wolfram.com/QuarticEquation.html) 
int RealQuartic (float a3, float a2, float a1, float a0, float* solution1=NULL, float* solution2=NULL,
					float* solution3=NULL, float* solution4=NULL, float tolerance=DEFAULT_ROOT_TOLERANCE);

} // namespace rage
#endif
