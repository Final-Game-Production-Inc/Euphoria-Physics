//
// phcore/phmath.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

/*************************************************************
	this file contains math functions for the physics library.
*************************************************************/

#include "phmath.h"

#include "constants.h"
#include "phbound/primitives.h"

#include "math/polynomial.h"
#include "math/simplemath.h"
#include "vector/quaternion.h"
#include "vector/matrix33.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

namespace rage {


#define MAX_ANG_INERTIA_RATIO 20.0f

bool CanSlerp(QuatV_In q1, QuatV_In q2)
{
	ScalarV sDot(Dot(q1, q2));
	sDot = Abs(sDot);
	sDot = Clamp(sDot, ScalarV(V_ZERO), ScalarV(V_ONE));
	sDot = Arccos(sDot);
	sDot = Sin(sDot);

	return !IsZeroAll(sDot);
}

QuatV_Out SlerpSafe(ScalarV_In t, QuatV_In q1, QuatV_In q2, QuatV_In errVal)
{
	if(CanSlerp(q1,q2))
	{
		return Slerp(t,q1,PrepareSlerp(q1,q2));
	}
	else
	{
		return errVal;
	}
}

Vec3V_Out ComputeAxisAnglePrecise(QuatV_In q)
{
	// Rotate a point orthonormal to the rotation axis to make an isosceles triangle where the
	//   unique angle is the angle of rotation. The length of the sides is the magnitude of the
	//   orthonormal point vector (1 in this case) and the base is the distance between the original
	//   and rotated point. 
	const Vec3V rotationAxis = GetUnitDirectionSafe(q);
	Assertf(IsLessThanAll(Abs(Subtract(Mag(rotationAxis),ScalarV(V_ONE))),ScalarV(V_FLT_SMALL_2)),"Non-normal rotation axis (%f, %f, %f) computed from quaternion (%f, %f, %f %f)",VEC3V_ARGS(rotationAxis),QUATV_ARGS(q));
	Vec3V rotatingPoint,temp;
	MakeOrthonormals(rotationAxis,rotatingPoint,temp);
	const ScalarV rotationDistance = Dist(rotatingPoint,Transform(q,rotatingPoint));
	const ScalarV halfAngle = Arcsin(Min(Scale(rotationDistance,ScalarV(V_HALF)),ScalarV(V_ONE)));
	return Scale(rotationAxis,Scale(halfAngle,ScalarV(V_TWO)));
}

Vec3V_Out ComputeAxisAnglePrecise(Mat33V_In m0, Mat33V_In m1)
{
	Mat33V delta;
	UnTransformOrtho(delta,m0,m1);
	return ComputeAxisAnglePrecise(QuatVFromMat33V(delta));
}

// Make the 3x3 part of a 3x4 matrix out of a vector angle.
// The matrix represents a rotation in world space by the angle.
void AngleToMatrix(const Vector3& a, Matrix34* m)
{
	float aMag2=a.Mag2();
	float aMag=sqrtf(aMag2);
	float cosine=cosf(aMag);
	float sine=sinf(aMag);
	Vector3 unitA(a);
	unitA.InvScale(aMag);
	float tempXY=unitA.x*unitA.y*(1.0f-cosine);
	float tempXZ=unitA.x*unitA.z*(1.0f-cosine);
	float tempYZ=unitA.y*unitA.z*(1.0f-cosine);
	m->a.x=cosine+square(unitA.x)*(1.0f-cosine);
	m->b.x=tempXY-unitA.z*sine;
	m->c.x=tempXZ+unitA.y*sine;
	m->a.y=tempXY+unitA.z*sine;
	m->b.y=cosine+square(unitA.y)*(1.0f-cosine);
	m->c.y=tempYZ-unitA.x*sine;
	m->a.z=tempXZ-unitA.y*sine;
	m->b.z=tempYZ+unitA.x*sine;
	m->c.z=cosine+square(unitA.z)*(1.0f-cosine);
}


// return the identity matrix rotated about the axis of "rotation", by an angle equal
// to the magnitude of "rotation", located at "location"
Matrix34 CreateRotatedMatrix(const Vector3& location, const Vector3& rotation)
{
	Matrix34 rotatedMatrix;
	rotatedMatrix.Identity();
	float angle=rotation.Mag2();
	if(angle>0.0f)
	{
		angle=sqrtf(angle);
		Vector3 axis(rotation);
		axis.InvScale(angle);
		rotatedMatrix.RotateUnitAxis(axis,angle);
	}
	rotatedMatrix.d.Set(location);
#if VECTORIZED_PADDING
	rotatedMatrix.d.w = 1.0f;
#endif
	return rotatedMatrix;
}


void RotateMatrix(Matrix34* matrix, const Vector3& rotation)
{
	float angle=rotation.Mag2();
	if(angle>0.0f)
	{
		angle=sqrtf(angle);
		Vector3 axis;
		axis.Scale(rotation,1.0f/angle);
		matrix->RotateUnitAxis(axis,angle);
	}
}


// Make a Matrix34 centered half way between the two given end points, with its y axis pointing from
// endA to endB, and set axis to go from endA to endB and length from endA to endB.
void CreateAxiallySymmetricMatrix(const Vector3& endA, const Vector3& endB, Matrix34* mtx, Vector3* axis, float* length)
{
	Assert(mtx && axis && length);

	mtx->d.Average(endA,endB);
	axis->Subtract(endB,endA);
	*length=axis->Mag2();
	if(*length>0.0f)
	{
		*length=sqrtf(*length);
		mtx->b.InvScale(*axis,*length);
		if(fabsf(mtx->b.x)<0.9f)
		{
			// Make mtx->a from mtx->b.Cross((1,0,0).Cross(mtx->b))
			mtx->a.Set(1.0f-square(mtx->b.x),-mtx->b.x*mtx->b.y,-mtx->b.x*mtx->b.z);
			mtx->a.Normalize();
			mtx->c.Cross(mtx->a,mtx->b);
		}
		else
		{
			// Make mtx->a from mtx->b.Cross((0,1,0).Cross(mtx->b))
			mtx->a.Set(-mtx->b.x*mtx->b.y,1.0f-square(mtx->b.y),-mtx->b.y*mtx->b.z);
			mtx->a.Normalize();
			mtx->c.Cross(mtx->a,mtx->b);
		}
	}
	else
	{
		mtx->Identity3x3();
	}
}


// take a list of three vertex locations, and set the order so that the line perpendicular to
// the line between the first two and passing through the third also passes between the first two
// if a pointer to three floats is specified, then change their order to match the vectors' order
void ReOrderVerts(Vector3* threeVertices, float* threeFloats)
{
	while(!VertOrderIsOK(threeVertices))
	{
		// if the order is not right, change it and try again
		Vector3 temp(threeVertices[0]);
		threeVertices[0].Set(threeVertices[1]);
		threeVertices[1].Set(threeVertices[2]);
		threeVertices[2].Set(temp);
		if(threeFloats)
		{
			float tempFloat=threeFloats[0];
			threeFloats[0]=threeFloats[1];
			threeFloats[1]=threeFloats[2];
			threeFloats[2]=tempFloat;
		}
	}
}


// take three vertex locations, and see if the line perpendicular to the line between the first
// two and passing through the third also passes between the first two
bool VertOrderIsOK(const Vector3* threeVertices)
{
	Vector3 testLine1,testLine2;
	testLine1.Subtract(threeVertices[1],threeVertices[0]);
	testLine2.Subtract(threeVertices[2],threeVertices[0]);
	bool isOK=true;
	if(testLine2.Dot(testLine1)<0.0f)
	{
		isOK=false;
	}
	else
	{
		testLine1.Negate();
		testLine2.Subtract(threeVertices[2],threeVertices[1]);
		if(testLine2.Dot(testLine1)<0.0f)
		{
			isOK=false;
		}
	}
	return isOK;
}


/*
PURPOSE
	Rank a list of floats in descending order (largest gets rank 0, smallest gets rank numInList-1).

PARAMS
	list		- the unranked list of floats
	rankList	- the array of integers into which to put the index numbers of the floats in the given list
					(rankList[0]==index of highest, rankList[numInList-1]==index of lowest)
	numInList	- the number of terms in the given list of floats and the list of integer rankings

NOTES
 1,	This does not use the faster QSort in order to preserve the original position of each item in the
	unsorted list. */
#define NOT_YET_RANKED -1
void RankDescending (const float* list, int* rankList, int numInList)
{
	// Initialize the unsorted ranking list.
	int index;
	for (index=0;index<numInList;index++)
	{
		rankList[index] = NOT_YET_RANKED;
	}

	// Loop over rankList, finding the largest item in list that wasn't already ranked, and putting that
	// item's index number in the next spot in rankList.
	float maxFound;
	int nextItem,rankedIndex;
	bool alreadyRanked;
	for (int item=0;item<numInList;item++)
	{
		// Loop over all the items in list, finding the next largest item that wasn't already ranked.
		nextItem = NOT_YET_RANKED;
		maxFound = -FLT_MAX;
		for (index=0;index<numInList;index++)
		{
			// Loop over all the previous rankings to see if this item in list was already ranked.
			alreadyRanked = false;
			for (rankedIndex=0;rankedIndex<item;rankedIndex++)
			{
				if (rankList[rankedIndex]==index)
				{
					// This item in list was already ranked, so skip to the next.
					alreadyRanked = true;
					break;
				}
			}

			// This item in list was not already ranked, so see if it's the largest unranked item so far.
			if (!alreadyRanked && list[index]>maxFound)
			{
				// This is the largest unranked item found so far for the next ranking.
				maxFound = list[index];
				nextItem = index;
			}
		}

		// Put the index number of the largest unranked item into rankList.
		Assert(nextItem>=0 && nextItem<numInList);
		rankList[item] = nextItem;
	}
}

float FindCentroid (const float* coef, const int* termIndices, int numTerms)
{
	float centroid = 0.0f;
	for (int index=0; index<numTerms; index++)
	{
		centroid += coef[termIndices[index]];
	}

	if (numTerms>1)
	{
		centroid /= (float)numTerms;
	}

	return centroid;
}


/*
Purpose:
	Solve a linear system of two equations with two unknowns:
	solutionX + solutionY = sumA
	coefX*solutionX + coefY*solutionY = sumB 

Parameters:
	sumA		- the sum of the two solutions
	coefX		- multiplying factor for the first solution
	coefY		- multiplying factor for the second solution
	sumB		- the sum of the two solutions times their multiplying factors
	solutionX	- pointer to the first solution, filled in by this function
	solutionY	- pointer to the second solution, filled in by this function

Return:
	true if solutions are found, false if there is no solution
	
Notes:
1.	This is the balancing equation to find the equilibrium wheel forces on two-wheeled vehicles, in which
	sumA is the vehicle weight, coefX and coefY are the wheel offsets, sumB is weight times the center of
	mass offset, and the solutions are the equilibrium wheel suspension forces. */
bool SolveLinear1D (float sumA, float coefX, float coefY, float sumB, float* solutionX, float* solutionY)
{
	float difference = coefY-coefX;
	const float nearlyZero = 1.0e-6f*Max(1.0f,fabsf(sumA),fabsf(sumB));
	if (fabsf(difference)>nearlyZero)
	{
		// There is a unique solution, so find it and return true.
		float invDiff = 1.0f/difference;
		(*solutionX) = (coefY*sumA-sumB)*invDiff;
		(*solutionY) = (sumB-coefX*sumA)*invDiff;
		return true;
	}

	float aveCoef = 0.5f*(coefX+coefY);
	if (fabsf(aveCoef*sumA-sumB)<nearlyZero)
	{
		// Anything with solutionX + solutionY = sumA (==sumB/coefX==sumB/coefY) will work, so choose equal
		// solutions and return true.
		if (fabsf(aveCoef)>VERY_SMALL_FLOAT)
		{
			(*solutionX) = 0.25f*(sumA+sumB/aveCoef);
		}
		else
		{
			(*solutionX) = 0.0f;
		}

		(*solutionY) = (*solutionX);
		return true;
	}

	// There is no solution with the given parameters.
	return false;
}


/*
Purpose:
	Solve a linear system of two equations with two unknowns:
	a0*x + b0*y + c0 = 0 and
	a1*x + b1*y + c1 = 0

Parameters:
	a0	- multiplying factor for the first solution in the first equation
	b0	- multiplying factor for the second solution in the first equation
	c0	- addition factor for the first equation
	a1	- multiplying factor for the first solution in the second equation
	b1	- multiplying factor for the second solution in the second equation
	c1	- addition factor for the second equation
	x	- pointer to the first solution, filled in by this function
	y	- pointer to the second solution, filled in by this function

Return:
	true if a solution was found, false if there is no solution */
bool SolveLinear1D (float a0, float b0, float c0, float a1, float b1, float c1, float& x, float& y)
{
	float denom = a0*b1-a1*b0;
	if (fabsf(denom)>VERY_SMALL_FLOAT)
	{
		// A unique solution exists, so solve it and return true.
		denom = 1.0f/denom;
		x = (b0*c1-b1*c0)*denom;
		y = (a1*c0-a0*c1)*denom;
		return true;
	}

	denom = a0*c1-a1*c0;
	if (fabsf(denom)<VERY_SMALL_FLOAT)
	{
		// The two equations are equivalent, so the solution is undefined.
		denom = a0+b0;

		if (fabsf(denom)>VERY_SMALL_FLOAT)
		{
			// Choose x==y as a solution.
			x = -c0/denom;
			y = x;
			return true;
		}

		denom = a1+b1;

		if (fabsf(denom)>VERY_SMALL_FLOAT)
		{
			// Choose x==y as a solution.
			x = -c1/denom;
			y = x;
			return true;
		}

		if (fabsf(a0)>VERY_SMALL_FLOAT)
		{
			// Choose x==2y as a solution.
			y = -c0/a0;
			x = 2.0f*y;
			return true;
		}

		if (fabsf(a1)>VERY_SMALL_FLOAT)
		{
			// Choose x==2y as a solution.
			y = -c1/a1;
			x = 2.0f*y;
			return true;
		}
	}

	// The two equations are inconsistent, so return false.
	return false;
}


/*
Purpose:
	Solve a linear system of two equations with two or more unknowns:
	Sum(solution[i]) = sumA and
	Sum(coef[i]*solution[i])= sumB 

Parameters:
	sumA		- the sum of the solutions
	coef		- list of multiplying factors for the solutions
	dummy		- uninitialized list of integers, for use by this function to rank the coefficient list
	numTerms	- the number of terms in the coefficient list, the dummy (descending rank) list and the solution list
	sumB		- the sum of the solutions times their multiplying factors
	solution	- pointer to a list in which to put the solutions

Return:
	true if a solution was found, false if there is no solution
	
Notes:
1.	This is equivalent to the balancing equation to find the equilibrium wheel forces for vehicles
	with an arbitrary number of wheels along one dimension. It is also used by collisions with three
	or more simultaneous impacts to find the individual impulses from the total impulse and angular impulse.
2.	There is a double loop over the list of coefficients, so don't call this with a large number of coefficients. */
bool SolveLinear1D (float sumA, const float* coef, int* dummy, int numTerms, float sumB, float* solution)
{
	Assert(coef && dummy && numTerms>=2);
	RankDescending(coef,dummy,numTerms);
	return SolveLinear1DRecurse(sumA,coef,dummy,numTerms,sumB,solution);
}


/*
PURPOSE
	Solve a linear system of two equations with two or more unknowns:
	Sum(solution[i]) = sumA and
	Sum(coef[i]*solution[i])= sumB 

PARAMS
	sumA		- the sum of the solutions
	coef		- list of multiplying factors for the solutions
	termIndices	- the rank of each coefficient in descending order
	numTerms	- the number of terms in the coefficient list, the dummy (descending rank) list and the solution list
	sumB		- the sum of the solutions times their multiplying factors
	solution	- pointer to a list in which to put the solutions

RETURN
	true if a solution was found, false if there is no solution
	
NOTES
1.	This function is available but is not intended to be called except through SolveLinear1D (above). */
bool SolveLinear1DRecurse (float sumA, const float* coef, const int* termIndices, int numTerms, float sumB, float* solution)
{
	Assert(solution);
	if (numTerms>2)
	{
		// There are more than two terms, so solve recursively.
		int numAbove = 0;
		bool balancingCentroid = false;
		if (sumA>SMALL_FLOAT)
		{
			// Find the centroid (the balancing center of mass) and the number of terms above the centroid.
			float centroid = sumB/sumA;
			numAbove = 0;
			while (numAbove<numTerms)
			{
				if (coef[termIndices[numAbove]]<centroid)
				{
					break;
				}
				numAbove++;
			}

			balancingCentroid = (numAbove>0 && numAbove<numTerms-1);
		}

		if (!balancingCentroid)
		{
			// This system of equations will not balance about any point, so choose a centroid that will have
			// approximately equal numbers of terms above and below.
			numAbove = numTerms/2;
		}

		// Find the centroids for the groups above and below the given centroid.
		float coefAbove = FindCentroid(coef,termIndices,numAbove);
		int numBelow = numTerms-numAbove;
		const int* termIndicesBelow = &termIndices[numAbove];
		float coefBelow = FindCentroid(coef,termIndicesBelow,numBelow);

		// Find the sums for the groups above and below the centroid.
		float sumAboveA,sumBelowA;
		if (SolveLinear1D(sumA,coefAbove,coefBelow,sumB,&sumAboveA,&sumBelowA))
		{
			// The problem was split into two separate linear sum problems.
			if (numAbove>=2)
			{
				// There are more than two above terms.
				// Find the sum of the coefficients times the solutions for all the above terms.
				float sumAboveB = sumAboveA*coefAbove;

				// Recurse to solve the part above.
				if (!SolveLinear1DRecurse(sumAboveA,coef,termIndices,numAbove,sumAboveB,solution))
				{
					return false;
				}
			}
			else
			{
				// Set the only point above to be the total above.
				solution[termIndices[0]] = sumAboveA;
			}

			if (numBelow>=2)
			{
				// There are more than two below terms.
				// Find the sum of the coefficients times the solutions for all the below terms.
				float sumBelowB = sumBelowA*coefBelow;

				// Recurse to solve the part below.
				if (!SolveLinear1DRecurse(sumBelowA,coef,termIndicesBelow,numBelow,sumBelowB,solution))
				{
					return false;
				}
			}
			else
			{
				// Set the only point below to be the total below.
				solution[termIndicesBelow[0]] = sumBelowA;
			}

			return true;
		}

		return false;
	}
	else
	{
		// There are two terms, so use the solution method for two terms.
		int index0 = termIndices[0];
		int index1 = termIndices[1];
		return SolveLinear1D(sumA,coef[index0],coef[index1],sumB,&solution[index0],&solution[index1]);
	}
}


/*
Purpose:
	Solve a linear system of three equations with three unknowns:
	solutionX + solutionY + solutionZ = sumA
	coefXB*solutionX + coefYB*solutionY + coefZB*solutionZ = sumB 
	coefXC*solutionX + coefYC*solutionY + coefZC*solutionZ = sumC 

Parameters:
	sumA		- the sum of the three solutions
	coefXB		- multiplying factor for the first solution in the second equation
	coefYB		- multiplying factor for the second solution in the second equation
	coefZB		- multiplying factor for the third solution in the second equation
	sumB		- the sum of the two solutions times their multiplying factors in the second equation
	coefXC		- multiplying factor for the first solution in the third equation
	coefYC		- multiplying factor for the second solution in the third equation
	coefZC		- multiplying factor for the third solution in the third equation
	sumC		- the sum of the two solutions times their multiplying factors in the third equation
	solutionX	- pointer to the first solution, filled in by this function
	solutionY	- pointer to the second solution, filled in by this function
	solutionZ	- pointer to the third solution, filled in by this function

Return:
	true if the equations are linearly independent, false if they are not (but a solution is still found)
	
Notes:
1.	This is the balancing equation to find the equilibrium wheel forces on three-wheeled vehicles, in which
	sumA is the vehicle weight, coefXB, coefYB and coefZB are the wheel offsets in one dimension, sumB is weight
	times the center of	mass offset in that dimension, coefXC, coefYC and coefZC are the wheel offsets in the other
	horizontal dimension, sumC is the weight times the center of mass offset in that dimension, and the solutions
	are the three equilibrium wheel suspension forces. */
bool SolveLinear2D (float sumA, float coefXB, float coefYB, float coefZB, float sumB, float coefXC, float coefYC, float coefZC,
					float sumC, float* solutionX, float* solutionY, float* solutionZ)
{
	Matrix34 matrix(1.0f,coefXB,coefXC,1.0f,coefYB,coefYC,1.0f,coefZB,coefZC,0.0f,0.0f,0.0f);
	Vector3 vector(sumA,sumB,sumC);
	Vector3 solution;
	bool goodSolution = matrix.SolveSVDCondition(vector,solution);
	(*solutionX) = solution.x;
	(*solutionY) = solution.y;
	(*solutionZ) = solution.z;
	return goodSolution;
}


/*
Purpose:
	Solve a linear system of three equations with three or more unknowns:
		Sum(solution[i]) = sumA
		Sum(coefB[i]*solution[i]) = sumB
		Sum(coefC[i]*solution[i]) = sumC

Parameters:
	sumA		- the sum of all of the solutions
	coefB		- set of multiplying factors in the second equation
	sumB		- the sum of the solutions times their multiplying factors in the second equation
	coefC		- set of multiplying factors in the third equation
	sumC		- the sum of the solutions times their multiplying factors in the third equation
	dummyB		- uninitialized set of floats
	dummyC		- uninitialized set of floats
	dummyInt	- uninitialized set of integers
	dummyFl0	- uninitialized set of floats
	dummyFl1	- uninitialized set of floats
	solutionX	- pointer to the first solution, filled in by this function
	solutionY	- pointer to the second solution, filled in by this function
	solutionZ	- pointer to the third solution, filled in by this function

Return:
	true if solutions are found, false if there is no solution
	
Notes:
1.	This is the balancing equation to find the equilibrium wheel forces on three-wheeled vehicles, in which
	sumA is the vehicle weight, coefXB, coefYB and coefZB are the wheel offsets in one dimension, sumB is weight
	times the center of	mass offset in that dimension, coefXC, coefYC and coefZC are the wheel offsets in the other
	horizontal dimension, sumC is the weight times the center of mass offset in that dimension, and the solutions
	are the three equilibrium wheel suspension forces. */
bool SolveLinear2D (float sumA, const float* coefB, float sumB, const float* coefC, float sumC, float* dummyB, float* dummyC,
					int* dummyInt, float* dummyFl0, float* dummyFl1, int numTerms, float* solution)
{
	if (numTerms==3)
	{
		return SolveLinear2D(sumA,coefB[0],coefB[1],coefB[2],sumB,coefC[0],coefC[1],coefC[2],sumC,
								&solution[0],&solution[1],&solution[2]);
	}
	Assert(numTerms>3);

	// Find the centroid (the balancing center of mass).
	float centroidB=0.0f,centroidC=0.0f;
	int index;
	if (fabsf(sumA)>SMALL_FLOAT)
	{
		centroidB = sumB/sumA;
		centroidC = sumC/sumA;
	}
	else
	{
		// This system of equations will not balance about any point, so choose a centroid near the middle.
		float highestB=-FLT_MAX,lowestB=FLT_MAX,highestC=-FLT_MAX,lowestC=FLT_MAX;
		for (index=0; index<numTerms; index++)
		{
			const float& coefficientB = coefB[index];
			if (coefficientB>highestB)
			{
				highestB = coefficientB;
			}
			if (coefficientB<lowestB)
			{
				lowestB = coefficientB;
			}
			const float& coefficientC = coefC[index];
			if (coefficientC>highestC)
			{
				highestC = coefficientC;
			}
			if (coefficientC<lowestC)
			{
				lowestC = coefficientC;
			}
		}
		centroidB = 0.5f*(lowestB+highestB);
		centroidC = 0.5f*(lowestC+highestC);
	}

	// Find the angle of each coefficient pair about the centroid, between PI and -PI with zero on the B+ axis,
	// the distance of each coefficient pair from the centroid, and the biggest distance.
	float* angles = dummyB;
	float* distances = dummyC;
	float offsetB,offsetC,thisDist,biggestDist=0.0f;
	for (index=0; index<numTerms; index++)
	{
		offsetB = coefB[index]-centroidB;
		offsetC = coefC[index]-centroidC;
		angles[index] = ArcTangent(offsetC,offsetB);
		thisDist = SqrtfSafe(square(offsetB)+square(offsetC));
		biggestDist = Max(biggestDist,thisDist);
		distances[index] = thisDist;
	}

	// Find the index numbers to arrange the angles in ascending or descending order.
	int* termIndices = dummyInt;
	RankDescending(angles,termIndices,numTerms);

	// Arrange the angles and distances in ascending order of angle.
	float* orderedAngles = dummyFl0;
	float* orderedDistances = dummyFl1;
	int rankedIndex;
	for (index=0; index<numTerms; index++)
	{
		rankedIndex = termIndices[numTerms-1-index];
		orderedAngles[index] = angles[rankedIndex];
		orderedDistances[index] = distances[rankedIndex];
	}

	// Arrange the coefficients in ascending order of angle. These are done in a separate loop because the unordered
	// angles and distances were stored in the dummyB and dummyC arrays, same as the ordered coefficient arrays.
	float* orderedCoefB = dummyB;
	float* orderedCoefC = dummyC;
	for (index=0; index<numTerms; index++)
	{
		rankedIndex = termIndices[numTerms-1-index];
		orderedCoefB[index] = coefB[rankedIndex];
		orderedCoefC[index] = coefC[rankedIndex];
	}

	// Find the largest difference in angle between neighboring terms.
	float previousAngle = orderedAngles[numTerms-1];
	float thisAngle = orderedAngles[0];
	float biggestAngleDiff = thisAngle-previousAngle+2.0f*PI;
	previousAngle = thisAngle;
	float angleDifference;
	for (index=1; index<numTerms; index++)
	{
		thisAngle = orderedAngles[index];
		angleDifference = thisAngle-previousAngle;
		previousAngle = thisAngle;
		biggestAngleDiff = Max(biggestAngleDiff,angleDifference);
	}

	const float nearPi = 0.01f;
	if ((biggestAngleDiff>PI-nearPi && biggestAngleDiff<PI+nearPi) || biggestAngleDiff>2.0f*(PI-nearPi))
	{
		// The points and the centroid are all in a line, so solve in one dimension.
		for (index=0; index<numTerms; index++)
		{
			if (orderedAngles[index]<0.0f)
			{
				orderedDistances[index] = -orderedDistances[index];
			}
		}

		return SolveLinear1D(sumA,orderedDistances,termIndices,numTerms,0.0f,solution);
	}

	bool balancing = biggestAngleDiff<PI;
	if (SolveLinear2DRecurse(sumA,orderedCoefB,sumB,orderedCoefC,sumC,orderedAngles,orderedDistances,numTerms,
								solution,centroidB,centroidC,balancing,biggestDist))
	{
		// Reorder the solution terms because they were found with the ordered coefficients.
		for (index=0; index<numTerms; index++)
		{
			rankedIndex = termIndices[numTerms-1-index];
			dummyFl0[rankedIndex] = solution[index];
		}
		for (index=0; index<numTerms; index++)
		{
			solution[index] = dummyFl0[index];
		}

		// Return true to indicate a solution was found.
		return true;
	}

	// Return false to indicate no solution was found.
	return false;
}


/*
Purpose:
	Solve a linear system of three equations with three or more unknowns:
		Sum(solution[i]) = sumA
		Sum(coefB[i]*solution[i]) = sumB
		Sum(coefC[i]*solution[i]) = sumC

Parameters:
	sumA		- the sum of the two solutions
	coefB		- set of multiplying factors for the solutions in the second equation
	sumB		- the sum of the solutions times their multiplying factors in the second equation
	coefC		- set of multiplying factors for the solutions in the third equation
	sumC		- the sum of the solutions times their multiplying factors in the third equation
	angles		- set of angles of the coefficients about the balancing point
	distances	- set of distances of the coefficients from the balancing point
	numTerms	- the number of terms in the coefficient list, the dummy (descending rank) list and the solution list
	solution	- set of solutions, filled in by this function
	biggestAngleDiff	- the largest angle difference between neighboring coefficients
	biggestDist	- the largest distance difference between any two coefficients
	verify		- debug mode only, whether or not to verify the accuracy of the solution

Return:
	true if solutions are found, false if there is no solution
	
Notes:
1.	This function is available but is not intended to be called except through SolveLinear2D (above).
2.	The elements in the arrays are all arranged in ascending order of angle. */
bool SolveLinear2DRecurse (float sumA, float* coefB, float sumB, float* coefC, float sumC, float* angles, float* distances,
							int numTerms, float* solution, float centroidB, float centroidC, bool balancing, float biggestDist,
							bool verify)
{
	if (numTerms>3)
	{
		// Combine two terms into one and recurse.
		// Find the index numbers of the two terms to combine, and the position of the new point between them.
		// The combined term will be put into combineTerm0, and combineTerm1 will be removed.
		int index,combineTerm0=BAD_INDEX,combineTerm1=BAD_INDEX;
		if (balancing)
		{
			// The terms to combine will be the neighboring pair (by angle) with the least total absolute value angle
			// relative to each of their neighbors (so the angle between them gets counted twice).
			// If the largest angle difference is greater than PI, then a different pair will be combined (below).
			float angleDiff0 = angles[numTerms-2]-angles[numTerms-3];
			float angleDiff1 = angles[numTerms-1]-angles[numTerms-2];
			float angleDiff2 = angles[0]-angles[numTerms-1]+2.0f*PI;
			float angleDiffSum,bestAngleDiffSum=FLT_MAX;
			for (index=0; index<numTerms; index++)
			{
				// Find the angle difference sum (twice this angle difference plus the two neighboring angle differences).
				angleDiffSum = angleDiff0 + 2.0f*angleDiff1 + angleDiff2;

				// See if this angle difference sum is the biggest so far.
				if (angleDiffSum<bestAngleDiffSum)
				{
					// Set the new best angle difference sum.
					bestAngleDiffSum = angleDiffSum;

					// Make combineTerm1 the term with the larger angle that was used to get angleDiff1.
					combineTerm1 = index>0 ? index-1 : numTerms-1;
				}

				if (index<numTerms-1)
				{
					// This isn't the last time in the loop, so set up the next set of index numbers and angle differences.
					angleDiff0 = angleDiff1;
					angleDiff1 = angleDiff2;
					angleDiff2 = angles[index+1]-angles[index];
				}
			}

			// Make combineTerm0 one lower than combineTerm0, so that it will have the next lower angle.
			Assert(combineTerm1!=BAD_INDEX);
			combineTerm0 = combineTerm1>0 ? combineTerm1-1 : numTerms-1;
		}
		else
		{
			// The largest angle difference is not less than PI. This means the balancing point is not enclosed
			// by the given set of points, so the two points with nearest angles will not be good ones to combine.
			// Combine instead the two points that are nearest each other in distance from the balancing point.
			float testDistanceDiff,smallestDistanceDiff=FLT_MAX,angleDiff;
			int secondIndex;
			for (index=numTerms-1; index>0; index--)
			{
				for (secondIndex=index-1; secondIndex>=0; secondIndex--)
				{
					// Get the difference between the distances from the balancing point.
					testDistanceDiff = fabsf(distances[index]-distances[secondIndex]);

					// Get the difference in angle around the balancing point.
					angleDiff = fabsf(angles[index]-angles[secondIndex]);
					if (angleDiff>2.0f*PI)
					{
						angleDiff -= 2.0f*PI;
					}

					// See if this is the closest pair in distance found so far, excluding pairs that are separated in
					// angle by nearly PI.
					if (testDistanceDiff<smallestDistanceDiff && angleDiff<PI-0.1f)
					{
						smallestDistanceDiff = testDistanceDiff;
						combineTerm1 = index;
						combineTerm0 = secondIndex;
					}
				}
			}
		}
		Assert(combineTerm0!=BAD_INDEX && combineTerm1!=BAD_INDEX);

		// Save the coefficients, angles and distances of the two combining terms so they can be restored.
		float combineCoefB0 = coefB[combineTerm0];
		float combineCoefB1 = coefB[combineTerm1];
		float combineCoefC0 = coefC[combineTerm0];
		float combineCoefC1 = coefC[combineTerm1];
		float angle0 = angles[combineTerm0];
		float angle1 = angles[combineTerm1];
		float dist0 = distances[combineTerm0];
		float dist1 = distances[combineTerm1];

		// Find the difference in angle between the two combining terms.
		float angleDiff = angle1-angle0;
		if (angleDiff>PI)
		{
			angleDiff = 2.0f*PI-angleDiff;
		}
		else if (angleDiff<-PI)
		{
			angleDiff = 2.0f*PI+angleDiff;
		}

		// Combine the two terms.
		float tZeroToOne,combineCoefB,combineCoefC;
		float angleT = 0.5f;
		if (!CombineSolveLinear2DTerms(angleT,angleDiff,dist0,dist1,combineTerm0,combineTerm1,coefB,coefC,
										numTerms,biggestDist,tZeroToOne,combineCoefB,combineCoefC))
		{
			angleT = 0.25f;
			if (!CombineSolveLinear2DTerms(angleT,angleDiff,dist0,dist1,combineTerm0,combineTerm1,coefB,coefC,
											numTerms,biggestDist,tZeroToOne,combineCoefB,combineCoefC))
			{
				angleT = 0.75f;
				CombineSolveLinear2DTerms(angleT,angleDiff,dist0,dist1,combineTerm0,combineTerm1,coefB,coefC,
											numTerms,biggestDist,tZeroToOne,combineCoefB,combineCoefC);
			}
		}

		// Change the angle and coefficients of the first combining term to be the angle and coefficients of
		// the new combined term.
		angles[combineTerm0] = angle0+angleT*angleDiff;
		coefB[combineTerm0] = combineCoefB;
		coefC[combineTerm0] = combineCoefC;

		// Find the distance from the centroid of the new term.
		distances[combineTerm0] = sqrtf(square(coefB[combineTerm0]-centroidB)+square(coefC[combineTerm0]-centroidC));

		// Make sure the new angle is between -PI and PI.
		int renumberTerms = 0;
		if (angles[combineTerm0]>PI)
		{
			// combineTerm0 is moving from the highest angle to the lowest.
			angles[combineTerm0] -= 2.0f*PI;
			renumberTerms = 1;
		}
		else if (angles[combineTerm0]<-PI)
		{
			// combineTerm0 is moving from the lowest angle to the highest.
			angles[combineTerm0] += 2.0f*PI;
			renumberTerms = -1;
		}

		// Remove the second combined term by renumbering all the following terms.
		int newNumTerms = numTerms-1;
		for (index=combineTerm1; index<newNumTerms; index++)
		{
			coefB[index] = coefB[index+1];
			coefC[index] = coefC[index+1];
			angles[index] = angles[index+1];
			distances[index] = distances[index+1];
		}

		if (renumberTerms==1)
		{
			// Move combineTerm0 from the last position (highest angle) to the first position (lowest angle).
			int newCombineTerm0 = combineTerm0-1;
			float coefBZero = coefB[newCombineTerm0];
			float coefCZero = coefC[newCombineTerm0];
			float anglesZero = angles[newCombineTerm0];
			float distancesZero = distances[newCombineTerm0];
			for (index=newNumTerms-1; index>0; index--)
			{
				coefB[index] = coefB[index-1];
				coefC[index] = coefC[index-1];
				angles[index] = angles[index-1];
				distances[index] = distances[index-1];
			}
			coefB[0] = coefBZero;
			coefC[0] = coefCZero;
			angles[0] = anglesZero;
			distances[0] = distancesZero;
		}
		else if (renumberTerms==-1)
		{
			// Move combineTerm0 from the first position (lowest angle) to the last position (highest angle).
			float coefBZero = coefB[combineTerm0];
			float coefCZero = coefC[combineTerm0];
			float anglesZero = angles[combineTerm0];
			float distancesZero = distances[combineTerm0];
			for (index=0; index<newNumTerms-1; index++)
			{
				coefB[index] = coefB[index+1];
				coefC[index] = coefC[index+1];
				angles[index] = angles[index+1];
				distances[index] = distances[index+1];
			}
			coefB[newNumTerms-1] = coefBZero;
			coefC[newNumTerms-1] = coefCZero;
			angles[newNumTerms-1] = anglesZero;
			distances[newNumTerms-1] = distancesZero;
		}

		// Try to solve the resulting set of equations with one less equation than before.
		if (SolveLinear2DRecurse(sumA,coefB,sumB,coefC,sumC,angles,distances,newNumTerms,solution,centroidB,centroidC,balancing,
			biggestDist,verify))
		{
			// The resulting set of equations was solved.
			if (renumberTerms==1)
			{
				// Move combineTerm0 back from the first position to the last position.
				float coefBZero = coefB[0];
				float coefCZero = coefC[0];
				float anglesZero = angles[0];
				float distancesZero = distances[0];
				float solutionZero = solution[0];
				for (index=0; index<newNumTerms-1; index++)
				{
					coefB[index] = coefB[index+1];
					coefC[index] = coefC[index+1];
					angles[index] = angles[index+1];
					distances[index] = distances[index+1];
					solution[index] = solution[index+1];
				}
				coefB[newNumTerms-1] = coefBZero;
				coefC[newNumTerms-1] = coefCZero;
				angles[newNumTerms-1] = anglesZero;
				distances[newNumTerms-1] = distancesZero;
				solution[newNumTerms-1] = solutionZero;
			}
			else if (renumberTerms==-1)
			{
				// Move combineTerm0 back from the last position to the first position.
				float coefBZero = coefB[newNumTerms-1];
				float coefCZero = coefC[newNumTerms-1];
				float anglesZero = angles[newNumTerms-1];
				float distancesZero = distances[newNumTerms-1];
				float solutionZero = solution[newNumTerms-1];
				for (index=newNumTerms-1; index>0; index--)
				{
					coefB[index] = coefB[index-1];
					coefC[index] = coefC[index-1];
					angles[index] = angles[index-1];
					distances[index] = distances[index-1];
					solution[index] = solution[index-1];
				}
				coefB[0] = coefBZero;
				coefC[0] = coefCZero;
				angles[0] = anglesZero;
				distances[0] = distancesZero;
				solution[0] = solutionZero;
			}

			// Restore the lists of coefficients, angles and distances.
			for (index=numTerms-1; index>combineTerm1; index--)
			{
				coefB[index] = coefB[index-1];
				coefC[index] = coefC[index-1];
				angles[index] = angles[index-1];
				distances[index] = distances[index-1];
				solution[index] = solution[index-1];
			}

			// Restore the combined solution into the originals and return true.
			float combineSolution = solution[combineTerm0];
			solution[combineTerm0] = (1.0f-tZeroToOne)*combineSolution;
			solution[combineTerm1] = tZeroToOne*combineSolution;
			coefB[combineTerm0] = combineCoefB0;
			coefB[combineTerm1] = combineCoefB1;
			coefC[combineTerm0] = combineCoefC0;
			coefC[combineTerm1] = combineCoefC1;
			distances[combineTerm0] = dist0;
			distances[combineTerm1] = dist1;
			angles[combineTerm0] = angle0;
			angles[combineTerm1] = angle1;
			return true;
		}

		// A solution was not found, so return false.
		return false;
	}

	// There are three terms left, so use the solution for three terms.
	return SolveLinear2D(sumA,coefB[0],coefB[1],coefB[2],sumB,coefC[0],coefC[1],coefC[2],sumC,&solution[0],&solution[1],&solution[2]);
}


bool CombineSolveLinear2DTerms (float angleT, float angleDiff, float dist0, float dist1, int combineTerm0, int combineTerm1,
								float* coefB, float* coefC, int numTerms, float biggestDist, float& tZeroToOne,
								float& combineCoefB, float& combineCoefC)
{
	// Find the t-value for linear distance between the two terms from the angular t-value.
	float absAngleDiff = fabsf(angleDiff);
	if (absAngleDiff>SMALL_FLOAT)
	{
		// The angle difference is not nearly zero.
		float coef0sineAngleT = dist0*sin(angleT*absAngleDiff);
		float coef1sineAngle1mT = dist1*sin((1.0f-angleT)*absAngleDiff);
		tZeroToOne = coef0sineAngleT/(coef0sineAngleT+coef1sineAngle1mT);
	}
	else
	{
		// The angle difference is nearly zero.
		tZeroToOne = angleT;
	}

	// Find the coefficients of the combined term.
	combineCoefB = Lerp(tZeroToOne,coefB[combineTerm0],coefB[combineTerm1]);
	combineCoefC = Lerp(tZeroToOne,coefC[combineTerm0],coefC[combineTerm1]);

	// Make sure the resulting set of pairs of coefficients aren't all in a line.
	int firstIndex = 0;
	if (combineTerm0==0 || combineTerm1==0)
	{
		firstIndex = 1;
		if (combineTerm0==1 || combineTerm1==1)
		{
			firstIndex = 2;
		}
	}

	float coefB0 = combineCoefB;
	float coefC0 = combineCoefC;
	float diffBOne = coefB[firstIndex]-coefB0;
	float diffCOne = coefC[firstIndex]-coefC0;
	const float crossYLimit = 0.01f*square(biggestDist);
	float diffBTwo,diffCTwo;
	for (int index=0; index<numTerms; index++)
	{
		if (index!=firstIndex && index!=combineTerm0 && index!=combineTerm1)
		{
			diffBTwo = coefB[index]-coefB0;
			diffCTwo = coefC[index]-coefC0;
			if (fabsf(diffBOne*diffCTwo-diffCOne*diffBTwo)>crossYLimit)
			{
				return true;
			}
		}
	}

	return false;
}


/*
PURPOSE
	Find a solution for a system of two linear equations with more than two unknowns, defined by
	(1) Sum(coefficients[i]*unknowns[i]) = sum, and
	(2) Sum(unknowns[i]) = 0

PARAMS
	coefficients	- coefficients in the first equation: Sum(coefficients[i]*unknowns[i]) = sum
	sum				- the summation in the first equation
	numTerms		- the number of terms in the summation in each equation
	unknowns		- the list of unknown parameters in the equations to solve
	elements		- an optional list of vectors to restrict the answer; if elements exists, then no
					  solution will be given in which the sum of each element times its corresponding
					  solution is too close to a zero vector
	elementSum		- the sum of elements[i]*unknowns[i]

NOTES
 1,	For the general case (when a solution is found), this problem is underdefined, so there are an
	infinite number of solutions.  This method finds a solution with a set of unknowns that all have
	the same magnitude as close as possible to zero, with half positive and half negative (and one
	equal to zero if there is an odd number).
 2.	The more general case in which the second equation has coefficients that are not 1 and a sum that
	is not 0 can be solved by redefining the unknowns to obtain equations (1) and (2). */
bool SolveLinear (const float* coefficients, float sum, int numTerms, float* unknowns,
				  const Vector3* elements, Vector3* elementSum)
{
	// Make a list of rankings of the coefficients in descending order.
	const int maxNumTerms = 64;
	Assert(numTerms>2 && numTerms<=maxNumTerms);
	int rankList[maxNumTerms];
	RankDescending(coefficients,rankList,numTerms);

	// Find out if there is an even or odd number of terms.
	bool evenNumTerms = (numTerms%2) ? false : true;

	float f,g,h;
	int j,k,index,upIndex,downIndex,numTriesJ=0,numTriesK=0,rankIndex;
	bool zeroSum = (sum==0.0f);
	bool finished = false;
	while (!finished)
	{
		if (!zeroSum)
		{
			// Choose two index numbers to handle separately from the others.
			bool positiveSum = (sum>0.0f) ? true : false;
			if (positiveSum==evenNumTerms)
			{
				// Set j to the index of the next smallest coefficient.
				j = rankList[numTerms-1-numTriesJ];
				// Set k to the index of the next largest coefficient.
				k = rankList[numTriesK];
			}
			else
			{
				// Set j to the index of the next largest coefficient.
				j = rankList[numTriesJ];
				// Set k to the index of the next smallest coefficient.
				k = rankList[numTerms-1-numTriesK];
			}

			// Set all the unknowns but the largest and smallest to 1 (highest half) and -1 (lowest half).
			float topHalf,bottomHalf;
			if (positiveSum)
			{
				topHalf = 1.0f;
				bottomHalf = -1.0f;
			}
			else
			{
				topHalf = -1.0f;
				bottomHalf = 1.0f;
			}
			upIndex = 0;
			downIndex = numTerms-1;
			while (upIndex<=downIndex)
			{
				rankIndex = rankList[upIndex];
				while ((rankIndex==j || rankIndex==k) && upIndex<=downIndex)
				{
					upIndex++;
					rankIndex = rankList[upIndex];
				}
				if (upIndex<=downIndex)
				{
					unknowns[rankIndex] = topHalf;
				}
				rankIndex = rankList[downIndex];
				while ((rankIndex==j || rankIndex==k) && upIndex<downIndex)
				{
					downIndex--;
					rankIndex = rankList[downIndex];
				}
				if (upIndex<downIndex || (upIndex==downIndex && !positiveSum))
				{
					unknowns[rankIndex] = bottomHalf;
				}
				upIndex++;
				downIndex--;
			}

			if (evenNumTerms)
			{
				f = 1.0f;
				g = 0.0f;
				h = 0.0f;
			}
			else
			{
				f = -1.0f;
				g = -coefficients[j];
				h = 1.0f;
			}

			g += f*(coefficients[k]-coefficients[j]);
			for (index=0;index<numTerms;index++)
			{
				if (index!=j && index!=k)
				{
					g += coefficients[index]*unknowns[index];
				}
			}
			g = sum/g;
		}
		else // zeroSum==true
		{
			// Set k to the index of the next largest coefficient.
			k = rankList[numTriesK];
			if (evenNumTerms)
			{
				// Set j to the index of the next largest coefficient that is not the same as the largest.
				upIndex = 1;
				j = rankList[numTriesK+upIndex];
				float coefDiff = fabsf(coefficients[j]-coefficients[k]);
				while ((coefDiff<SMALL_FLOAT || coefDiff<0.01f*fabsf(coefficients[k])) && numTriesK+upIndex<numTerms)
				{
					upIndex++;
					if (numTriesK+upIndex<numTerms)
					{
						j = rankList[numTriesK+upIndex];
						coefDiff = fabsf(coefficients[j]-coefficients[k]);
					}
				}
				if (numTriesK+upIndex>=numTerms)
				{
					return false;
				}

				upIndex = 0;
				while (upIndex<numTerms)
				{
					rankIndex = rankList[upIndex];
					while ((rankIndex==j || rankIndex==k) && upIndex<numTerms)
					{
						upIndex++;
						rankIndex = rankList[upIndex];
					}
					if (upIndex<numTerms)
					{
						unknowns[rankIndex] = 1.0f;
						upIndex++;
						rankIndex = rankList[upIndex];
					}
					while ((rankIndex==j || rankIndex==k) && upIndex<numTerms)
					{
						upIndex++;
						rankIndex = rankList[upIndex];
					}
					if (upIndex<numTerms)
					{
						unknowns[rankIndex] = -1.0f;
						upIndex++;
					}
				}
			}
			else
			{
				if (numTerms-1-numTriesJ<=numTriesK)
				{
					return false;
				}

				// Set j to the index of the smallest coefficient.
				j = rankList[numTerms-1-numTriesJ];
				Assert(j!=k);
				upIndex = 0;
				downIndex = numTerms-1;
				while (upIndex<=downIndex)
				{
					rankIndex = rankList[upIndex];
					while ((rankIndex==j || rankIndex==k) && upIndex<=downIndex)
					{
						upIndex++;
						rankIndex = rankList[upIndex];
					}
					if (upIndex<=downIndex)
					{
						unknowns[rankIndex] = 1.0f;
					}
					rankIndex = rankList[downIndex];
					while ((rankIndex==j || rankIndex==k) && upIndex<downIndex)
					{
						downIndex--;
						rankIndex = rankList[downIndex];
					}
					if (upIndex<downIndex)
					{
						unknowns[rankIndex] = -1.0f;
					}
					upIndex++;
					downIndex--;
				}
			}

			if (evenNumTerms)
			{
				f = 0.0f;
				h = 0.0f;
			}
			else
			{
				f = coefficients[j];
				h = 1.0f;
			}
			float fabCoefSum = 0.0f;
			for (index=0;index<numTerms;index++)
			{
				fabCoefSum += fabsf(coefficients[index]);
				if (index!=j && index!=k)
				{
					f -= coefficients[index]*unknowns[index];
				}
			}
			f /= coefficients[k]-coefficients[j];
			g = 0.1f*fabCoefSum/(float)numTerms;
		}
		unknowns[k] = f;
		unknowns[j] = -f-h;
		for (index=0;index<numTerms;index++)
		{
			unknowns[index] *= g;
		}

		if (!elements)
		{
			// There is no list of element vectors with which to restrict the solution, so keep the first answer.
			finished = true;
		}
		else
		{
			// Make sure the weighted sum of the given element vectors is not zero.  If it is, then
			// find another solution.
			Assert(elementSum);
			elementSum->Zero();
			for (index=0;index<numTerms;index++)
			{
				elementSum->AddScaled(elements[index],unknowns[index]);
			}
			float sumLimit = 0.001f;
			if (fabsf(g)>1.0f)
			{
				sumLimit *= fabsf(g);
			}
			if (fabsf(elementSum->x)>sumLimit || fabsf(elementSum->y)>sumLimit || fabsf(elementSum->z)>sumLimit)
			{
				// The weighted sum of the given element vectors is not near zero, so use this solution.
				finished = true;
			}
			else
			{
				// The weighted sum of the given element vectors is near zero, so find another solution.
				if (!zeroSum && evenNumTerms)
				{
					numTriesK++;
					if (numTriesK>numTerms-1)
					{
						// Index numbers numTriesK and numTriesK+1 are used, so numTerms-2 is the last value for numTriesK.
						// No answer was found, so return false for failure.
						return false;
					}
				}
				else
				{
					if (numTriesJ==numTriesK)
					{
						numTriesJ++;
						numTriesK = 0;
					}
					else if (numTriesJ==numTriesK+1)
					{
						numTriesJ = 0;
						numTriesK++;
					}
					else if (numTriesJ<numTriesK)
					{
						numTriesJ++;
					}
					else
					{
						Assert(numTriesJ>numTriesK);
						numTriesK++;
					}
					if (numTriesJ+numTriesK>=numTerms)
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}


// find the fractions of the distance along a segment that gives the points that are locally
// closest to a circle such that the circle's tangent at that point is perpendicular to the line
// return the number of points found with 0 < t < 1 (0, 1 or 2)
int FindTValuesLineToCircle(const Vector3& point1, const Vector3& point1to2, const Vector3& normal,
							const Vector3& center, float radius, float* edgeT1, Vector3* segToCircle1,
							float* distance1, float* edgeT2, Vector3* segToCircle2, float* distance2) {
	Vector3 e;
	e.Subtract(point1,center);			// vector from the circle center to point1
	float e2=e.Mag2();
	float en=e.Dot(normal);				// distance from point1 to the circle's plane
	float en2=square(en);
	Vector3 q(point1to2);				// segment vector
	float q2=q.Mag2();					// segment length squared
	float qn=q.Dot(normal);				// segment length parallel to circle normal
	float qn2=square(qn);				// segment parallel length squared
	float eq=e.Dot(q);
	float r2=square(radius);
	float inverse=1.0f/(square(q2)*(q2-qn2));
	float a3=2.0f*q2*(eq*(2.0f*q2-qn2)-q2*qn*en)*inverse;
	float a2=(-r2*square(q2-qn2)+square(q2)*(e2-en2)-eq*(4.0f*en*qn*q2-eq*(5.0f*q2-qn2)))*inverse;
	float a1=2.0f*(r2*en*qn*(q2-qn2)+eq*(-r2*(q2-qn2)+q2*(e2-en2)+eq*(eq-en*qn)))*inverse;
	float a0=(square(eq)*(e2-en2)-r2*square(eq-en*qn))*inverse;
	float t[4];
	int num4=RealQuartic(a3,a2,a1,a0,&t[0],&t[1],&t[2],&t[3]);
	// find the solution that is closest to the circle within the range 0<t<1
	// with a circle tangent perpendicular to the edge
	// (if none of the quartic solutions satisfy this then return false)
	Vector3 segmentToCircle,segmentPoint;
	float distance;
	int numFound=0;
	for(int i=0;i<num4;i++)
	{
		if(t[i]<=0.0f || t[i]>=1.0f) continue;
		segmentPoint.AddScaled(point1,point1to2,t[i]);
		segmentPoint.Subtract(center);
		float parallel=segmentPoint.Dot(normal);
		// parallel is the distance from the segment point to the circle's plane
		float perp=sqrtf(segmentPoint.Mag2()-parallel*parallel);
		segmentToCircle.SubtractScaled(segmentPoint,normal,parallel);
		segmentToCircle.Scale(radius/perp);
		segmentToCircle.Subtract(segmentPoint);
		// segmentToCircle is now the vector from the segment point defined by t[i]
		// to the closest point on the circle
		distance=segmentToCircle.Mag();
		if(numFound==0)
		{
			// this is the first answer found
			*edgeT1=t[i];
			segToCircle1->Set(segmentToCircle);
			*distance1=distance;
			numFound=1;
		}
		else
		{
			// this is the second answer found
			*edgeT2=t[i];
			segToCircle2->Set(segmentToCircle);
			*distance2=distance;
			numFound=2;
		}
	}
	return numFound;
}


// Find the acceleration that will bring the current position smoothly to the target position,
// ending with the target velocity in the given time.  If the given time is too small, constant
// acceleration will override and the target velocity and position will be reached in a different time.
// This works for one-dimensional positions and angles.
// If the same time is used with the current position and velocity on every update, then
// the actual time to reach the target will be about 2*time, and the acceleration will taper
// smoothly toward the end.
float FindHomingAccel (float time, float currentPos, float currentSpeed, float targetPos, float targetSpeed)
{
	float accel = 0.0f;
	if (time>SMALL_FLOAT)
	{
		bool accelPhase = true;
		float estTime = targetSpeed+currentSpeed;
		if (estTime!=0.0f)
		{
			estTime = 2.0f*(targetPos-currentPos)/estTime;
			if (estTime>0.0f && estTime<time)
			{
				// The point at which to start constant deceleration has been reached.
				accelPhase = false;
			}
		}

		if (accelPhase)
		{
			// Acceleration phase: accelerate at a rate that would reach the target position in half the time,
			// ignoring the target velocity.
			float scaledTime = 0.5f*time;
			accel = 2.0f*(targetPos-currentPos-currentSpeed*scaledTime)/square(scaledTime);
		}
		else
		{
			// Deceleration phase: accelerate at a rate that would reach the target position with the
			// target velocity ignoring the time - this phase should start at the correct time.
			accel = 0.5f*(square(targetSpeed)-square(currentSpeed))/(targetPos-currentPos);
		}
	}

	return accel;
}


// This calls FindHomingAccel three times for the components of vectors treated independently.
// This will not work well for angles - use FindHomingAngAccel instead.
Vector3 FindHomingAccel3D(float time, const Vector3& currentPos, const Vector3& currentVel,
							const Vector3& targetPos, const Vector3& targetVel)
{
	return Vector3(FindHomingAccel(time,currentPos.x,currentVel.x,targetPos.x,targetVel.x),
					FindHomingAccel(time,currentPos.y,currentVel.y,targetPos.y,targetVel.y),
					FindHomingAccel(time,currentPos.z,currentVel.z,targetPos.z,targetVel.z));
}


// Find the angular acceleration that will bring the current angle smoothly to zero,
// ending with no angular velocity in the given time.
Vector3 FindHomingAngAccel(const Vector3 &angle, const Vector3 &angVel, float time, float invTimeStep)
{
	Vector3 unitParallel(angle);
	float angleMag=angle.Mag();
	unitParallel.InvScale(-angleMag);
	float angVelParallel=unitParallel.Dot(angVel);
	Vector3 unitPerp;
	unitPerp.SubtractScaled(angVel,unitParallel,angVelParallel);
	unitPerp.Normalize();
	float angVelPerp=unitPerp.Dot(angVel);
	float angAccelPerp=-angVelPerp*invTimeStep;
	float angAccelParallel=FindHomingAccel(time,-angleMag,angVelParallel);
	Vector3 angAccel;
	angAccel.Scale(unitPerp,angAccelPerp);
	angAccel.AddScaled(unitParallel,angAccelParallel);
	return angAccel;
}


// Angular version of FindHomingAccel3D, converting the matrices to quaternions for smooth rotations.
Vector3 FindHomingAngAccel3D(float time, float invTimeStep, const Matrix34& currentMtx, const Vector3& currentAngVel,
								const Matrix34& targetMtx, const Vector3& targetAngVel)
{
	Quaternion currentQ,targetQ;
	currentQ.FromMatrix34(currentMtx);
	targetQ.FromMatrix34(targetMtx);
	currentQ.PrepareSlerp (targetQ);
	return FindHomingAngAccel3D(time,invTimeStep,currentQ,currentAngVel,targetQ,targetAngVel);
}


Vector3 FindHomingAngAccel3D(float time, float invTimeStep, const Quaternion& currentQ, const Vector3& currentAngVel,
							 const Quaternion& targetQ, const Vector3& targetAngVel)
{
	Quaternion relQ;
	relQ.Inverse(currentQ);
	relQ.Multiply(targetQ);
	Vector3 temp;
	relQ.GetDirection(temp);
	Vector3 unitParallel;
	currentQ.Transform(temp,unitParallel);
	unitParallel.Normalize();
	float angVelParallel=unitParallel.Dot(currentAngVel);
	float targetAngVelParallel=unitParallel.Dot(targetAngVel);
	Vector3 unitPerp;
	unitPerp.SubtractScaled(currentAngVel,unitParallel,angVelParallel);
	unitPerp.Normalize();
	float angVelPerp=unitPerp.Dot(currentAngVel);
	float targetAngVelPerp=unitPerp.Dot(targetAngVel);
	float angAccelPerp=(targetAngVelPerp-angVelPerp)*invTimeStep;
	float angle=relQ.GetAngle();
	float angAccelParallel=FindHomingAccel(time,0.0f,angVelParallel,angle,targetAngVelParallel);
	Vector3 angAccel;
	angAccel.Scale(unitPerp,angAccelPerp);
	angAccel.AddScaled(unitParallel,angAccelParallel);
	return angAccel;
}


//////////////////////////////////////
////////  Inertia Functions //////////
//////////////////////////////////////

void phMathInertia::ComputeTetrahedronAngInertia (Vector3* threeVerts, float height, float* iXX, float* iYY, float* iZZ, float* iXY, float* iXZ, float* iYZ)
{
	// ReOrderVerts guarantees that vert3 is between verts[0] and verts[1]
	ReOrderVerts(threeVerts);
	Vector3 vert01,vert02;
	vert01.Subtract(threeVerts[1],threeVerts[0]);
	float vert01Mag2 = vert01.Mag2();
	float vert01Mag = sqrtf(vert01Mag2);
	vert02.Subtract(threeVerts[2],threeVerts[0]);

	// vert3 is the point on the line from verts[0] to verts[1] making a line with verts[2] that is perpendicular to vert01
	Vector3 vert3(threeVerts[0]);
	vert3.AddScaled(vert01,vert02.Dot(vert01)/vert01Mag2);

	Vector3 vert32;
	vert32.Subtract(threeVerts[2],vert3);
	float x0 = threeVerts[0].x;
	float x1 = threeVerts[1].x;
	float x2 = threeVerts[2].x;
	float y0 = threeVerts[0].y;
	float y1 = threeVerts[1].y;
	float y2 = threeVerts[2].y;
	float z0 = threeVerts[0].z;
	float z1 = threeVerts[1].z;
	float z2 = threeVerts[2].z;
	float scale = vert32.Mag()*vert01Mag*0.016666666667f*height;
	float xTerm = scale*(square(x0)+square(x1)+square(x2)+x0*x1+x0*x2+x1*x2);
	float yTerm = scale*(square(y0)+square(y1)+square(y2)+y0*y1+y0*y2+y1*y2);
	float zTerm = scale*(square(z0)+square(z1)+square(z2)+z0*z1+z0*z2+z1*z2);
	(*iXX) = yTerm+zTerm;
	(*iYY) = xTerm+zTerm;
	(*iZZ) = xTerm+yTerm;
	scale *= -0.5f;
	(*iXY) = scale*(2.0f*(x0*y0+x1*y1+x2*y2)+x0*y2+y0*x2+x0*y1+y0*x1+x1*y2+y1*x2);
	(*iXZ) = scale*(2.0f*(x0*z0+x1*z1+x2*z2)+x0*z2+z0*x2+x0*z1+z0*x1+x1*z2+z1*x2);
	(*iYZ) = scale*(2.0f*(y0*z0+y1*z1+y2*z2)+y0*z2+z0*y2+y0*z1+z0*y1+y1*z2+z1*y2);
}


void phMathInertia::ComputeTriangleAngInertia (Vector3* threeVerts, float* iXX, float* iYY, float* iZZ, float* iXY, float* iXZ, float* iYZ)
{
	// ReOrderVerts guarantees that vert3 is between verts[0] and verts[1]
	ReOrderVerts(threeVerts);
	Vector3 vert01,vert02;
	vert01.Subtract(threeVerts[1],threeVerts[0]);
	float vert01Mag2 = vert01.Mag2();
	float vert01Mag = sqrtf(vert01Mag2);
	vert02.Subtract(threeVerts[2],threeVerts[0]);

	// vert3 is the point on the line from verts[0] to verts[1] making a line with verts[2] that is perpendicular to vert01
	Vector3 vert3(threeVerts[0]);
	vert3.AddScaled(vert01,vert02.Dot(vert01)/vert01Mag2);

	Vector3 vert32;
	vert32.Subtract(threeVerts[2],vert3);
	float x0 = threeVerts[0].x;
	float x1 = threeVerts[1].x;
	float x2 = threeVerts[2].x;
	float y0 = threeVerts[0].y;
	float y1 = threeVerts[1].y;
	float y2 = threeVerts[2].y;
	float z0 = threeVerts[0].z;
	float z1 = threeVerts[1].z;
	float z2 = threeVerts[2].z;
	float scale = vert32.Mag()*vert01Mag*0.08333333333f;
	float xTerm = scale*(square(x0)+square(x1)+square(x2)+x0*x1+x0*x2+x1*x2);
	float yTerm = scale*(square(y0)+square(y1)+square(y2)+y0*y1+y0*y2+y1*y2);
	float zTerm = scale*(square(z0)+square(z1)+square(z2)+z0*z1+z0*z2+z1*z2);
	(*iXX) = yTerm+zTerm;
	(*iYY) = xTerm+zTerm;
	(*iZZ) = xTerm+yTerm;
	scale *= -0.5f;
	(*iXY) = scale*(2.0f*(x0*y0+x1*y1+x2*y2)+x0*y2+y0*x2+x0*y1+y0*x1+x1*y2+y1*x2);
	(*iXZ) = scale*(2.0f*(x0*z0+x1*z1+x2*z2)+x0*z2+z0*x2+x0*z1+z0*x1+x1*z2+z1*x2);
	(*iYZ) = scale*(2.0f*(y0*z0+y1*z1+y2*z2)+y0*z2+z0*y2+y0*z1+z0*y1+y1*z2+z1*y2);
}


void phMathInertia::FindPrincipalAxes (float iXX, float iYY, float iZZ, float iXY, float iXZ, float iYZ, Vector3& angInertia, Matrix34& colliderMtx, Matrix34& rotation)
{
	// Find a root of the cubic equation that results from diagonalization of the inertia matrix.
	float a2 = -iXX-iYY-iZZ;
	float a1 = iXX*iYY+iXX*iZZ+iYY*iZZ-square(iXY)-square(iXZ)-square(iYZ);
	float a0 = iXX*square(iYZ)+iYY*square(iXZ)+iZZ*square(iXY)-2.0f*iXY*iXZ*iYZ-iXX*iYY*iZZ;
	float root0,root1,root2;
	int numRoots = RealCubic(a2,a1,a0,&root0,&root1,&root2);

	switch(numRoots)
	{
		case 0:
		{
			// This should never happen.
			AssertMsg(0,("phMathInertia::FindPrincipalAxes got zero roots from RealCubic, that's never supposed to happen."));
		}

		case 1:
		{
			// The body is symmetric (any axes will do, so leave them unchanged).
			angInertia.Set(root0,root0,root0);
			rotation.Identity();
			return;
		}

		case 2:
		{
			// The body is symmetric along one axis.
			rotation.a.Set(PrincipalAxis(angInertia,root0,iXY,iXZ,iYZ));

			// Any pair of perpendicular axes will do for the other two.
			if (rotation.a.x>=SQRT3INVERSE)
			{
				// The a-vector is the principal axis closest to x, so leave it as the a-vector.
				angInertia.Set(root0,root1,root1);
		
				// Choose the b-vector to have no z-component.
				rotation.b.Set(-rotation.a.y,rotation.a.x,0.0f);
				rotation.b.Normalize();
				rotation.c.Cross(rotation.a,rotation.b);
			}
			else
			{
				// Make sure the y-component of the a-vector is positive.
				if (rotation.a.y<0.0f)
				{
					rotation.a.Negate();
				}

				if (rotation.a.y>=SQRT3INVERSE)
				{
					// The a-vector is the principal axis closest to y,
					// so make it the b-vector.
					rotation.b.Set(rotation.a);
					angInertia.Set(root1,root0,root1);
				
					// Choose the a-vector to have no z-component.
					rotation.a.Set(rotation.b.y,-rotation.b.x,0.0f);
					rotation.a.Normalize();
					rotation.c.Cross(rotation.a,rotation.b);
				}
				else
				{
					// The a-vector is the principal axis closest to z, so make it the c-vector.
					rotation.c.Set(rotation.a);
					angInertia.Set(root1,root1,root0);

					// Make sure it's z-component is positive.
					if (rotation.c.z<0.0f)
					{
						rotation.c.Negate();
					}

					// Choose the a-vector to have no y-component.
					rotation.a.Set(rotation.c.z,0.0f,-rotation.c.x);
					rotation.a.Normalize();
					rotation.b.Cross(rotation.c,rotation.a);
				}
			}
			break;
		}

		case 3:
		{
			// The body is not symmetric.
			rotation.a.Set(PrincipalAxis(angInertia,root0,iXY,iXZ,iYZ));
			rotation.b.Set(PrincipalAxis(angInertia,root1,iXY,iXZ,iYZ));

			// Make sure principal.b is perpendicular to principal.a
			a0 = rotation.a.Dot(rotation.b);
			if (a0!=0.0f)
			{
				rotation.b.SubtractScaled(rotation.a,a0);
				rotation.b.Normalize();
			}

			if (rotation.a.x>=SQRT3INVERSE)
			{
				// The a-vector is the principal axis closest to x, so leave it as the a-vector.
				// Make sure the b-vector's y-component is positive.
				if (rotation.b.y<0.0f)
				{
					rotation.b.Negate();
				}

				if (rotation.b.y>=SQRT3INVERSE)
				{
					// The b-vector is the principal axis closest to y, so leave it as the b-vector.
					rotation.c.Cross(rotation.a,rotation.b);
					angInertia.Set(root0,root1,root2);
				}
				else
				{
					// The b-vector is the principal axis closest to z, so make it the c-vector.
					rotation.c.Set(rotation.b);

					// Make sure it's z-component is positive.
					if (rotation.c.z<0.0f)
					{
						rotation.c.Negate();
					}

					rotation.b.Cross(rotation.c,rotation.a);
					angInertia.Set(root0,root2,root1);
				}
			}
			else if (rotation.b.x>=SQRT3INVERSE)
			{
				// The b-vector is the principal axis closest to x, so make it the a-vector (and put a into c).
				rotation.c.Set(rotation.a);
				rotation.a.Set(rotation.b);

				// Make sure the c-vector's z-component is positive.
				if (rotation.c.z<0.0f)
				{
					rotation.c.Negate();
				}

				if (rotation.c.z>=SQRT3INVERSE)
				{
					// The c-vector is the principal axis closest to z, so leave it as the c-vector.
					rotation.b.Cross(rotation.c,rotation.a);
					angInertia.Set(root1,root2,root0);
				}
				else
				{
					// The c-vector is the principal axis closest to y, so make it the b-vector.
					rotation.b.Set(rotation.c);

					// Make sure its y-component is positive.
					if (rotation.b.y<0.0f)
					{
						rotation.b.Negate();
					}

					rotation.c.Cross(rotation.a,rotation.b);
					angInertia.Set(root1,root0,root2);
				}
			}
			else if (fabsf(rotation.a.y)>=SQRT3INVERSE)
			{
				// The a-vector is the principal axis closest to y, so put it in b, moving b to c.
				rotation.c.Set(rotation.b);
				rotation.b.Set(rotation.a);

				// Make sure the b-vector's y-component is positive.
				if (rotation.b.y<0.0f)
				{
					rotation.b.Negate();
				}

				// Make sure the c-vector's z-component is positive.
				if (rotation.c.z<0.0f)
				{
					rotation.c.Negate();
				}

				if (rotation.c.z>=SQRT3INVERSE)
				{
					// The c-vector is the principal axis closest to z, so leave it as the c-vector.
					rotation.a.Cross(rotation.b,rotation.c);
					angInertia.Set(root2,root0,root1);
				}
				else
				{
					// The c-vector is the principal axis closest to x, so put it in a.
					rotation.a.Set(rotation.c);

					// Make sure its x-component is positive.
					if (rotation.a.x<0.0f)
					{
						rotation.a.Negate();
					}

					rotation.c.Cross(rotation.a,rotation.b);
					angInertia.Set(root1,root0,root2);
				}
			}
			else
			{
				// The a-vector must be the principal axis closest to z, so put it in c.
				rotation.c.Set(rotation.a);

				// Make sure its z-component is positive.
				if (rotation.c.z<0.0f)
				{
					rotation.c.Negate();
				}

				// Make sure the b-vector's y-component is positive.
				if (rotation.b.y<0.0f)
				{
					rotation.b.Negate();
				}

				if (rotation.b.y>SQRT3INVERSE)
				{
					// the b-vector is the principal axis closest to y
					rotation.a.Cross(rotation.b,rotation.c);
					angInertia.Set(root2,root1,root0);
				}
			}
			break;
		}
	}

	rotation.d.Zero();
	Matrix34 newColliderMtx(rotation);
	newColliderMtx.Dot3x3(colliderMtx);
	colliderMtx.Set3x3(newColliderMtx);

	// Normalize the ICS matrix vectors to cut roundoff errors from multiple AddInertias.
	colliderMtx.a.Normalize();
	colliderMtx.b.Normalize();
	colliderMtx.c.Normalize();
}


// Find the principal axis corresponding to a root of the cubic equation that comes from
// diagonalizing the angular inertia matrix in FindPrincipalAxes.
// The x-component is always positive.
Vector3 phMathInertia::PrincipalAxis(const Vector3& angInertia, float cubicRoot, float iXY, float iXZ, float iYZ)
{
	float inverse=1.0f/(cubicRoot-angInertia.z);
	float temp=1.0f/(cubicRoot-angInertia.y-square(iYZ)*inverse);
	float beta=(iXY+iXZ*iYZ*inverse)*temp;
	float gamma=inverse*(iXZ+iYZ*beta);
	float alpha=1.0f/sqrtf(1.0f+square(beta)+square(gamma));
	beta*=alpha;
	gamma*=alpha;
	Vector3 axis(alpha,beta,gamma);
	return axis;
}

// Add the angular inertia of another object to this angular inertia and give back the matrix
// with which to transform the model and bound to the new principal axes.
// This version assumes the principal axes of the added angular inertia are x, y and z.
void phMathInertia::AddInertia (float mass, const Vector3& angInertia, const Vector3 &location, Matrix34& colliderMtx, float& totalMass, Vector3& totalAngInertia, Matrix34* rotation)
{
	float newMass = totalMass+mass;
	if (newMass==0.0f)
	{
		totalMass = 0.0f;
		totalAngInertia.Zero();
		if (rotation)
		{
			rotation->Identity();
		}

		return;
	}

	Vector3 displacement(location);
	float oldMass = totalMass;
	if (oldMass>0.0f)
	{
		displacement.Subtract(colliderMtx.d);
	}

	Vector3 offset;
	offset.Scale(displacement,mass/newMass);
	if (oldMass>0.0f)
	{
		colliderMtx.d.Add(offset);
	}
	else
	{
		colliderMtx.d.Set(offset);
	}

	displacement.Subtract(offset);
	totalMass = newMass;
	totalAngInertia.Set(angInertia);
	totalAngInertia.x+=mass*(square(displacement.y)+square(displacement.z))+oldMass*(square(offset.y)+square(offset.z));
	totalAngInertia.y+=mass*(square(displacement.x)+square(displacement.z))+oldMass*(square(offset.x)+square(offset.z));
	totalAngInertia.z+=mass*(square(displacement.x)+square(displacement.y))+oldMass*(square(offset.x)+square(offset.y));
	float iXY = -mass*displacement.x*displacement.y-oldMass*offset.x*offset.y;
	float iXZ = -mass*displacement.x*displacement.z-oldMass*offset.x*offset.z;
	float iYZ = -mass*displacement.y*displacement.z-oldMass*offset.y*offset.z;
	if (iXY==0.0f && iXZ==0.0f && iYZ==0.0f)
	{
		// The principal axes are still (x, y, z).
		if (oldMass<=0.0f)
		{
			colliderMtx.Identity3x3();
		}

		if (rotation)
		{
			rotation->Identity3x3();
			rotation->d.Negate(offset);
		}

		return;
	}

	FindPrincipalAxes(angInertia.x,angInertia.y,angInertia.z,iXY,iXZ,iYZ,totalAngInertia,colliderMtx,*rotation);
}


// Add the angular inertia of another object to this angular inertia and return the matrix
// with which to transform the model and bound to the new principal axes.
// This version allows the added angular inertia to have principal axes that are not (x, y, z).
void phMathInertia::AddInertia (float mass, const Vector3& angInertia, const Matrix34& mtx, Matrix34& colliderMtx, float& totalMass, Vector3& totalAngInertia, Matrix34* rotation)
{
	float newMass = totalMass+mass;
	if (newMass==0.0f)
	{
		totalMass=0.0f;
		totalAngInertia.Zero();
		if (rotation)
		{
			rotation->Identity();
		}

		return;
	}

	Vector3 displacement(mtx.d);
	float oldMass = totalMass;
	if (oldMass>0.0f)
	{
		displacement.Subtract(colliderMtx.d);
	}

	Vector3 offset;
	offset.Scale(displacement,mass/newMass);
	if (oldMass>0.0f)
	{
		colliderMtx.d.Add(offset);
	}
	else
	{
		colliderMtx.d.Set(offset);
	}

	displacement.Subtract(offset);
	Matrix34 addedI;
	addedI.Transpose(mtx);
	Matrix34 tempInertia;
	tempInertia.a.Set(angInertia.x,0.0f,0.0f);
	tempInertia.b.Set(0.0f,angInertia.y,0.0f);
	tempInertia.c.Set(0.0f,0.0f,angInertia.z);
	addedI.Dot3x3(tempInertia);
	addedI.Dot3x3(mtx);
	totalMass = newMass;
	totalAngInertia.Add(angInertia);
	totalAngInertia.x+=mass*(square(displacement.y)+square(displacement.z))+oldMass*(square(offset.y)+square(offset.z));
	totalAngInertia.y+=mass*(square(displacement.x)+square(displacement.z))+oldMass*(square(offset.x)+square(offset.z));
	totalAngInertia.z+=mass*(square(displacement.x)+square(displacement.y))+oldMass*(square(offset.x)+square(offset.y));
	float iXY = addedI.a.y-mass*displacement.x*displacement.y-oldMass*offset.x*offset.y;
	float iXZ = addedI.a.z-mass*displacement.x*displacement.z-oldMass*offset.x*offset.z;
	float iYZ = addedI.b.z-mass*displacement.y*displacement.z-oldMass*offset.y*offset.z;
	if (iXY==0.0f && iXZ==0.0f && iYZ==0.0f)
	{
		// The principal axes are still (x, y, z).
		if (oldMass<=0.0f)
		{
			colliderMtx.Identity3x3();
		}

		if (rotation)
		{
			rotation->Identity3x3();
			rotation->d.Negate(offset);
		}

		return;
	}

	if (rotation)
	{
		FindPrincipalAxes(angInertia.x,angInertia.y,angInertia.z,iXY,iXZ,iYZ,totalAngInertia,colliderMtx,*rotation);
	}
}


void phMathInertia::ClampAngInertia (Vector3& angInertia)
{
	int largestDim = MaximumIndex(angInertia.x,angInertia.y,angInertia.z);
	for (int dim=0;dim<3;dim++)
	{
		if (dim!=largestDim && angInertia[dim]*MAX_ANG_INERTIA_RATIO<angInertia[largestDim])
		{
			angInertia[dim] = angInertia[largestDim]/MAX_ANG_INERTIA_RATIO;
		}
	}
}


void phMathInertia::GetInverseInertiaMatrix (Mat33V_In colliderMatrix, Vec::V3Param128 invAngInertia, Mat33V_InOut invInertia)
{
	Mat33V mtxDotInvInertia;
	Vec3V v_invAngInertia(invAngInertia);
	ScalarV invAngInertiaX, invAngInertiaY, invAngInertiaZ;
	invAngInertiaX = SplatX( v_invAngInertia );
	invAngInertiaY = SplatY( v_invAngInertia );
	invAngInertiaZ = SplatZ( v_invAngInertia );
	mtxDotInvInertia.SetCol0( Scale( colliderMatrix.GetCol0(), Vec3V(invAngInertiaX) ) );
	mtxDotInvInertia.SetCol1( Scale( colliderMatrix.GetCol1(), Vec3V(invAngInertiaY) ) );
	mtxDotInvInertia.SetCol2( Scale( colliderMatrix.GetCol2(), Vec3V(invAngInertiaZ) ) );
	Mat33V thisMtx;
	Transpose( thisMtx, colliderMatrix );
	Multiply( invInertia, mtxDotInvInertia, thisMtx );
}

float phMathInertia::FindCylinderMass(float density, float radius, float length)
{
	return PI*square(radius)*length*density*1000.0f;
}


void phMathInertia::FindCylinderAngInertia(float mass, float radius, float length, Vector3* angInertia)
{
	float radialMom=mass*square(radius)*0.5f;
	float centralMom=radialMom*0.5f+mass*square(length)/12.0f;
	angInertia->Set(centralMom,radialMom,centralMom);
}


void phMathInertia::FindCylinderMassAngInertia(float density, float radius, float length, float* mass, Vector3* angInertia)
{
	*mass=FindCylinderMass(density,radius,length);
	FindCylinderAngInertia(*mass,radius,length,angInertia);
}


void phMathInertia::FindGeomAngInertia (float mass, const Vector3* vertex, const phPolygon* polygon, int numPolygons, const Vector3& cgOffset,
										Vector3& angInertia, Matrix34* rotation, Matrix34* colliderMatrix)
{
	// Find the mass and angular inertia with a density of 1.
	float tmpMass;
	FindGeomMassAngInertia(1.0f,vertex,polygon,numPolygons,cgOffset,tmpMass,angInertia,rotation,colliderMatrix);
	if (tmpMass>=SMALL_FLOAT)
	{
		// The mass with a density of 1 is not nearly zero, so scale the angular inertia with a density of 1 by the given mass divided by the density-1 mass
		// to get the real angular inertia with the given mass.
		angInertia.Scale(mass/tmpMass);
	}
	else if (mass<SMALL_FLOAT)
	{
		// The given mass is nearly zero, and the mass with a density of 1 is nearly zero, so make the angular inertia zero.
		angInertia.Zero();
	}
	else
	{
		// The mass with a density of 1 is nearly zero, and the given mass is not nearly zero, so make the angular inertia the same scale as the given mass.
		angInertia.Set(mass);
	}
}


bool phMathInertia::FindGeomMassAngInertia (float density, const Vector3* vertex, const phPolygon* polygon, int numPolygons, const Vector3& cgOffset,
											float& mass, Vector3& angInertia, Matrix34* rotation, Matrix34* colliderMatrix)
{
	Vector3 polyNormal,verts[4];
	float tripleVolume = 0.0f;
	float inertiaXX=0.0f,inertiaYY=0.0f,inertiaZZ=0.0f,inertiaXY=0.0f,inertiaXZ=0.0f,inertiaYZ=0.0f;
	float iXX,iYY,iZZ,iXY,iXZ,iYZ;
	float densityMKS = density*1000.0f;

	// Calculate the total mass by uniformly filling a set of wedge-shaped solids in 3 dimensions,
	// with one corner at the origin and 3 or 4 corners on each polygon.
	// Polygons with normals pointing toward the origin count negatively.
	// Angular inertia is found the same way, but 4-sided polygons are divided into 2 triangles.
	bool flat = true;
	for (int polygonIndex=0; polygonIndex<numPolygons; polygonIndex++)
	{
		const phPolygon& poly = polygon[polygonIndex];

		polyNormal.Set(VEC3V_TO_VECTOR3(poly.ComputeUnitNormal((const Vec3V*)vertex)));

		verts[0].Subtract(vertex[poly.GetVertexIndex(0)],cgOffset);
		verts[1].Subtract(vertex[poly.GetVertexIndex(1)],cgOffset);
		verts[2].Subtract(vertex[poly.GetVertexIndex(2)],cgOffset);
		float normDotVert0 = polyNormal.Dot(verts[0]);
		if (normDotVert0!=0.0f)
		{
			flat = false;
		}

		// Add three times the tetrahedron volume, for both if this is a quad.
		tripleVolume += poly.GetArea()*normDotVert0;

		ComputeTetrahedronAngInertia(verts,normDotVert0,&iXX,&iYY,&iZZ,&iXY,&iXZ,&iYZ);
		inertiaXX += iXX;
		inertiaYY += iYY;
		inertiaZZ += iZZ;
		inertiaXY += iXY;
		inertiaXZ += iXZ;
		inertiaYZ += iYZ;
	}

	if (!flat)
	{
		// Take the absolute values of the volume and inertia, in case roundoff errors made a small number negative or someone made an inside-out bound.
		tripleVolume = fabsf(tripleVolume);
		inertiaXX = fabsf(inertiaXX);
		inertiaYY = fabsf(inertiaYY);
		inertiaZZ = fabsf(inertiaZZ);

		// Multiply the inertia components by the density.
		inertiaXX *= densityMKS;
		inertiaYY *= densityMKS;
		inertiaZZ *= densityMKS;

		// Set the mass from the volume and density.
		mass = fabsf(tripleVolume)*0.33333333f*densityMKS;

		// Set the angular inertia.
		angInertia.Set(inertiaXX,inertiaYY,inertiaZZ);
	}
	else
	{
		// This bound is flat, so find its area instead of volume, and use the density as mass/area instead of mass/volume.
		densityMKS /= 1000.0f;
		float area = 0.0f;
		inertiaXX = inertiaYY = inertiaZZ = inertiaXY = inertiaXZ = inertiaYZ = 0.0f;
		for (int polygonIndex=0; polygonIndex<numPolygons; polygonIndex++)
		{
			const phPolygon& poly = polygon[polygonIndex];
			area += poly.GetArea();

		#if POLY_MAX_VERTICES==4
			polyNormal.Set(VEC3V_TO_VECTOR3(poly.GetUnitNormal()));
		#else
			polyNormal.Set(VEC3V_TO_VECTOR3(poly.ComputeUnitNormal((const Vec3V*)vertex)));
		#endif

			verts[0].Subtract(vertex[poly.GetVertexIndex(0)],cgOffset);
			verts[1].Subtract(vertex[poly.GetVertexIndex(1)],cgOffset);
			verts[2].Subtract(vertex[poly.GetVertexIndex(2)],cgOffset);

			ComputeTriangleAngInertia(verts,&iXX,&iYY,&iZZ,&iXY,&iXZ,&iYZ);
			inertiaXX += iXX;
			inertiaYY += iYY;
			inertiaZZ += iZZ;
			inertiaXY += iXY;
			inertiaXZ += iXZ;
			inertiaYZ += iYZ;
		}

		// Take the absolute values of the volume and inertia, in case roundoff errors made a small number negative or someone made an inside-out bound.
		inertiaXX = fabsf(inertiaXX);
		inertiaYY = fabsf(inertiaYY);
		inertiaZZ = fabsf(inertiaZZ);

		// Multiply the inertia components by the density.
		inertiaXX *= densityMKS;
		inertiaYY *= densityMKS;
		inertiaZZ *= densityMKS;

		// Set the mass from the area and density.
		mass = area*densityMKS;

		// Set the angular inertia.
		angInertia.Set(inertiaXX,inertiaYY,inertiaZZ);
	}

	// See if an output rotation or collider matrix was provided, for computing the principal axes.
	if (!rotation || !colliderMatrix)
	{
		// No output rotation or collider matrix was given, so assume the coordinate axes are the angular inertia principal axes.
		// This is only really true if the off-diagonal inertia matrix components (inertiaXY, inertiaXZ and inertiaYZ) are all zero.
		// Return true if the bound has volume, false if it does not.
		return !flat;
	}

	// See if the angular inertia principal axes are close to the coordinate axes.
	inertiaXY *= densityMKS;
	inertiaXZ *= densityMKS;
	inertiaYZ *= densityMKS;
	if (Min(inertiaXX,inertiaYY,inertiaZZ)<0.05f*Max(inertiaXY,inertiaXZ,inertiaYZ))
	{
		// Realign the collider's coordinate system to match the principal axes.
		FindPrincipalAxes(angInertia.x,angInertia.y,angInertia.z,inertiaXY,inertiaXZ,inertiaYZ,angInertia,*colliderMatrix,*rotation);
	}

	// Return true if the bound has volume, false if it does not.
	return !flat;
}

} // namespace rage
