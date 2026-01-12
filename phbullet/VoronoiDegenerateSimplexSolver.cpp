
/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
	
	Elsevier CDROM license agreements grants nonexclusive license to use the software
	for any purpose, commercial or non-commercial as long as the following credit is included
	identifying the original source of the software:

	Parts of the source are "from the book Real-Time Collision Detection by
	Christer Ericson, published by Morgan Kaufmann Publishers,
	(c) 2005 Elsevier Inc."
		
*/

#include "VoronoiDegenerateSimplexSolver.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#define VERTA  0
#define VERTB  1
#define VERTC  2
#define VERTD  3

namespace rage {

void VoronoiDegenerateSimplexSolver::RemoveVertex(int index)
{
	// Decrement the number of vertices.
	Assert(m_NumVertices>0);
	int numVertices = m_NumVertices - 1;
	m_NumVertices = numVertices;

	// Put what was the last vertex into the removed vertex's position.
	m_SimplexSeparation[index] = m_SimplexSeparation[numVertices];
	m_SimplexPointOnA[index] = m_SimplexPointOnA[numVertices];
	m_SimplexPointOnB[index] = m_SimplexPointOnB[numVertices];
	m_CachedResult.m_BarycentricCoords[index] = m_CachedResult.m_BarycentricCoords[numVertices];
	m_CachedResult.m_BarycentricCoords[numVertices] = 0.0f;
}


void VoronoiDegenerateSimplexSolver::ReduceVertices (VecBoolV_In usedVerts)
{
	VecBoolV unusedVerts = InvertBits(usedVerts);
	if (m_NumVertices >= 4 && unusedVerts.GetW().Getb())
	{
		// We don't need to swap any memory around if we're removing from the end of the array
		m_NumVertices--;
	}

	if (m_NumVertices >= 3 && unusedVerts.GetZ().Getb())
	{
		// There are at least 3 vertices and the 3rd one was not used, so remove it.
		RemoveVertex(2);
	}

	if (m_NumVertices >= 2 && unusedVerts.GetY().Getb())
	{
		// There are at least 2 vertices and the 2nd one was not used, so remove it.
		RemoveVertex(1);
	}

	if (m_NumVertices >= 1 && unusedVerts.GetX().Getb())
	{
		// There is at least 1 vertex and the 1st one was not used, so remove it.
		RemoveVertex(0);
	}
}



//add a vertex if it does not create a degenerate simplex
void VoronoiDegenerateSimplexSolver::AddVertex (Vec3V_In worldPointBtoA, Vec3V_In worldPointOnA, Vec3V_In worldPointOnB)
{
	Assertf(!IsSimplexFull(), "Attempting to add vertex to full simplex");

	// If this vertex doesn't move the simplex more than 0.001m closer to the last point, don't add it.
	if(IsGreaterThanAll(Dot(worldPointBtoA, m_CachedNormal), Add(m_CachedDistanceAlongNormal, ScalarV(V_FLT_SMALL_3))))
	{
		m_SimplexSeparation[m_NumVertices] = worldPointBtoA;
		m_SimplexPointOnA[m_NumVertices] = worldPointOnA;
		m_SimplexPointOnB[m_NumVertices] = worldPointOnB;

		m_NumVertices++;
	}
}

void VoronoiDegenerateSimplexSolver::UpdateClosestVectorAndPoints(Vec3V_In p)
{
	m_CachedResult.Reset();

	switch (GetNumVertices())
	{
	case 1:
		{
			m_CachedV = p-m_SimplexSeparation[0];
			m_CachedResult.m_BarycentricCoords = Vec4V(V_X_AXIS_WZERO);
			break;
		}

	case 2:
		{
			//closest point origin from line segment
			Vec3V from = m_SimplexSeparation[0];
			Vec3V to = m_SimplexSeparation[1];

			Vec3V diff = p - from;
			Vec3V v = to - from;
			ScalarV t = Dot(v, diff);

			if ( IsGreaterThanAll(t, ScalarV(V_ZERO)) != 0 )
			{
				ScalarV dotVV = MagSquared( v );
				if ( IsLessThanAll(t, dotVV) != 0 )
				{
					t = InvScale( t, dotVV );
					m_CachedResult.m_UsedVertices = VecBoolV(V_MASKXY);
				}
				else
				{
					t = ScalarV(V_ONE);
					//reduce to 1 point
					m_CachedResult.m_UsedVertices = VecBoolV(V_MASKY);
				}
			}
			else
			{
				t = ScalarV(V_ZERO);
				//reduce to 1 point
				m_CachedResult.m_UsedVertices = VecBoolV(V_MASKX);
			}

			Vec4V tInY = And( Vec4V(t), Vec4V(V_MASKY) );
			Vec4V oneMinusTInX = And( Vec4V(Subtract(ScalarV(V_ONE), t)), Vec4V(V_MASKX) );
			m_CachedResult.m_BarycentricCoords = Or(tInY, oneMinusTInX);
			m_CachedV = p-AddScaled(from, Subtract(to,from),t);

			ReduceVertices( m_CachedResult.m_UsedVertices );
			break;
		}

	case 3:
		{
			Vec3V a = m_SimplexSeparation[0];
			Vec3V b = m_SimplexSeparation[1];
			Vec3V c = m_SimplexSeparation[2];

			ClosestPtPointTriangle(p, a, b, c, m_CachedResult);
			m_CachedV = p-m_CachedResult.m_ClosestPointOnSimplex;

			ReduceVertices ( m_CachedResult.m_UsedVertices );

			break;
		}

	case 4:
		{
			Vec3V a = m_SimplexSeparation[0];
			Vec3V b = m_SimplexSeparation[1];
			Vec3V c = m_SimplexSeparation[2];
			Vec3V d = m_SimplexSeparation[3];

			if (ClosestPtPointTetrahedron(p, a, b, c, d, m_CachedResult))
			{
				m_CachedV = p-m_CachedResult.m_ClosestPointOnSimplex;
				ReduceVertices ( m_CachedResult.m_UsedVertices );
			}
			else
			{
				m_CachedV = Vec3V(V_ZERO);
			}
			break;
		}

	default:
		{
			Assertf(false, "VoronoiDegenerateSimplexSolver::UpdateClosestVectorAndPoints requires a simplex of at least 1 and no more than 4 points. %u found", GetNumVertices());
		}
	}

	// Calculate a normal. When the cached vector between the simplex and point is small
	//   normalize it will lead to large errors, especially because the point was constructed
	//   with barycentric coordinates. Calculating the normal just from the geometry is much
	//   more precise.
	Vec3V normal;
	switch(GetNumVertices())
	{
	default:
		{
			normal = m_CachedV;
			break;
		}
	case 2:
		{
			Vec3V vector0to1 = Subtract(m_SimplexSeparation[1], m_SimplexSeparation[0]);
			Vec3V vector0toP = Subtract(p, m_SimplexSeparation[0]);
			normal = Cross(vector0to1, Cross(vector0toP, vector0to1));
			break;
		}
	case 3:
		{
			Vec3V vector0to1 = Subtract(m_SimplexSeparation[1], m_SimplexSeparation[0]);
			Vec3V vector0to2 = Subtract(m_SimplexSeparation[2], m_SimplexSeparation[0]);
			Vec3V vector0toP = Subtract(p, m_SimplexSeparation[0]);
			normal = Cross(vector0to1,vector0to2);
			normal = SelectFT(IsLessThan(Dot(vector0toP, normal), ScalarV(V_ZERO)), normal, Negate(normal));
			break;
		}
	}
	m_CachedNormal = NormalizeSafe(normal, Vec3V(V_ZERO), Vec3V(V_FLT_SMALL_12));

	// Store the distance along the normal for later
	m_CachedDistanceAlongNormal = Dot(m_CachedNormal, m_SimplexSeparation[0]);
}

	//return the current simplex
int VoronoiDegenerateSimplexSolver::GetSimplex(Vec3V_Ptr pBuf, Vec3V_Ptr qBuf, Vec3V_Ptr yBuf) const
{
	int numVerts = GetNumVertices();

	int i;
	for (i=0;i<numVerts;i++)
	{
		pBuf[i] = m_SimplexSeparation[i];
		qBuf[i] = m_SimplexPointOnA[i];
		yBuf[i] = m_SimplexPointOnB[i];
	}
	return numVerts;
}

void VoronoiDegenerateSimplexSolver::GetClosestPoints(Vec3V_InOut worldPointOnA, Vec3V_InOut worldPointOnB)
{
	Vec4V bary = m_CachedResult.m_BarycentricCoords;

	Vec3V a0(m_SimplexPointOnA[0]), a1(m_SimplexPointOnA[1]), a2(m_SimplexPointOnA[2]), a3(m_SimplexPointOnA[3]);
	worldPointOnA = AddScaled(AddScaled(AddScaled(Scale(a0,bary.GetX()), a1,bary.GetY()), a2,bary.GetZ()), a3,bary.GetW());

	Vec3V b0(m_SimplexPointOnB[0]), b1(m_SimplexPointOnB[1]), b2(m_SimplexPointOnB[2]), b3(m_SimplexPointOnB[3]);
	worldPointOnB = AddScaled(AddScaled(AddScaled(Scale(b0,bary.GetX()), b1,bary.GetY()), b2,bary.GetZ()), b3,bary.GetW());
}

void VoronoiDegenerateSimplexSolver::ClosestPtPointTriangle(Vec3V_In p, Vec3V_In a, Vec3V_In b, Vec3V_In c,SubSimplexClosestResult& result)
{
	// Convert to class types.
	ScalarV _zero(V_ZERO);
	Vec3V _1111(V_ONE);
	Vec3V _xaxis(V_X_AXIS_WZERO);
	Vec3V _yaxis(V_Y_AXIS_WZERO);
	Vec3V _zaxis(V_Z_AXIS_WZERO);
	Vec3V _maskx(V_MASKX);
	Vec3V _masky(V_MASKY);
	Vec3V _maskz(V_MASKZ);
	Vec3V _andw(V_MASKXYZ);

	Vec3V ab = b - a;
	Vec3V ap = p - a;
	ScalarV d1 = Dot(ab, ap);
	Vec3V ac = c - a;
	ScalarV d2 = Dot(ac, ap);

	// Check if P in vertex region outside A (d1 <= 0.0f && d2 <= 0.0f)
	BoolV d1GreaterThanZero = IsGreaterThan(d1, _zero);
	BoolV d2GreaterThanZero = IsGreaterThan(d2, _zero);

	// If closestPointNotFound ever goes zero, then the computation of closestPoint is complete and
	// the rest of the equations should just "fall through".
	BoolV closestPointNotFound = d1GreaterThanZero;
	closestPointNotFound = Or(closestPointNotFound, d2GreaterThanZero);

	Vec3V bp = p - b;
	ScalarV d3 = Dot(ab, bp);
	ScalarV d4 = Dot(ac, bp);

	// Check if P in vertex region outside B (d3 >= 0.0f && d4 <= d3)
	BoolV d3LessThanZero = IsLessThan(d3, _zero);
	BoolV d4GreaterThand3 = IsGreaterThan(d4, d3);

	// closestPointNotFound is currently zero if a should be the result...
	BoolV pNotInVertexRegionOutsideB = d3LessThanZero;
	pNotInVertexRegionOutsideB = Or(pNotInVertexRegionOutsideB, d4GreaterThand3);

	Vec3V closestPoint = SelectFT(closestPointNotFound, a, b);
	Vec3V barycentricCoordinates = SelectFT(closestPointNotFound, _xaxis, _yaxis);
	closestPointNotFound = And(closestPointNotFound, pNotInVertexRegionOutsideB);
	// ...now closestPointNotFound is currently zero if closestPoint (which is either a or b) should be the result

	// Check if P in edge region of AB, if so return projection of P onto AB (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
	ScalarV d1d4 = Scale( d1, d4 );
	ScalarV d3d2 = Scale( d3, d2 );
	ScalarV vc = d1d4 - d3d2;

	BoolV vcGreaterThanZero = IsGreaterThan(vc, _zero);
	BoolV d1LessThanZero = IsLessThan(d1, _zero);
	BoolV d3GreaterThanZero = IsGreaterThan(d3, _zero);

	BoolV pNotInEdgeRegionOfAB = vcGreaterThanZero;
	pNotInEdgeRegionOfAB = Or(pNotInEdgeRegionOfAB, d1LessThanZero);
	pNotInEdgeRegionOfAB = Or(pNotInEdgeRegionOfAB, d3GreaterThanZero);

	ScalarV d1Minusd3 = d1 - d3;
	ScalarV v; v = InvScale( d1, d1Minusd3 );
	Vec3V pointOnAB = AddScaled(a, ab, v);
	closestPoint = SelectFT(closestPointNotFound, closestPoint, pointOnAB);


	Vec3V oneMinusV_V_Zero = Vec3V(v); // Vector3(1.0f - v.x, v.x, 0.0f)
	oneMinusV_V_Zero = And(oneMinusV_V_Zero, _masky);

	{
		Vec3V oneMinusV_Zero_Zero;
		oneMinusV_Zero_Zero = Subtract(_1111, Vec3V(v));
		oneMinusV_Zero_Zero = And(oneMinusV_Zero_Zero, _maskx);
		oneMinusV_V_Zero = Add(oneMinusV_V_Zero, oneMinusV_Zero_Zero);
	}

	//Assert(IsClose(oneMinusV_V_Zero, Vec3V(1.0f - v.x, v.x, 0.0f), VEC3_SMALL_FLOAT));

	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, oneMinusV_V_Zero);
	closestPointNotFound = And(closestPointNotFound, pNotInEdgeRegionOfAB);

	Vec3V cp = p - c;
	ScalarV d5 = Dot(ab, cp);
	ScalarV d6 = Dot(ac, cp);

	// Check if P in vertex region outside C
	BoolV d6LessThanZero = IsLessThan(d6, _zero);
	BoolV d5GreaterThand6 = IsGreaterThan(d5, d6);

	BoolV pNotInVertexRegionOutsideC = d6LessThanZero;
	pNotInVertexRegionOutsideC = Or(pNotInVertexRegionOutsideC, d5GreaterThand6);

	closestPoint = SelectFT(closestPointNotFound, closestPoint, c);
	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, _zaxis);
	closestPointNotFound = And(closestPointNotFound, pNotInVertexRegionOutsideC);

	// Check if P in edge region of AC, if so return projection of P onto AC
	ScalarV d5d2 = Scale( d5, d2 );
	ScalarV d1d6 = Scale( d1, d6 );
	ScalarV vb = d5d2 - d1d6;

	BoolV vbGreaterThanZero = IsGreaterThan(vb, _zero);
	BoolV d2LessThanZero = IsLessThan(d2, _zero);
	BoolV d6GreaterThanZero = IsGreaterThan(d6, _zero);

	BoolV pNotInEdgeRegionOfAC = vbGreaterThanZero;
	pNotInEdgeRegionOfAC = Or(pNotInEdgeRegionOfAC, d2LessThanZero);
	pNotInEdgeRegionOfAC = Or(pNotInEdgeRegionOfAC, d6GreaterThanZero);

	ScalarV d2Minusd6 = d2 - d6;
	ScalarV w; w = InvScale(d2, d2Minusd6);
	Vec3V pointOnAC;
	pointOnAC = AddScaled(a, ac, w);
	closestPoint = SelectFT(closestPointNotFound, closestPoint, pointOnAC);

	Vec3V oneMinusW_Zero_W = Vec3V(w); // Vector3(1.0f - w.x, 0.0f, w.x)
	oneMinusW_Zero_W = And(oneMinusW_Zero_W, _maskz);

	{
		Vec3V oneMinusW_Zero_Zero;
		oneMinusW_Zero_Zero = Subtract(_1111, Vec3V(w));
		oneMinusW_Zero_Zero = And(oneMinusW_Zero_Zero, _maskx);
		oneMinusW_Zero_W = Add(oneMinusW_Zero_W, oneMinusW_Zero_Zero);
	}

	//Assert(IsClose(oneMinusW_Zero_W, Vec3V(1.0f - w.x, 0.0f, w.x), VEC3_SMALL_FLOAT));

	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, oneMinusW_Zero_W);
	closestPointNotFound = And(closestPointNotFound, pNotInEdgeRegionOfAC);

	// Check if P in edge region of BC, if so return projection of P onto BC
	ScalarV d3d6 = Scale( d3, d6 );
	ScalarV d5d4 = Scale( d5, d4 );
	ScalarV va = d3d6 - d5d4;

	BoolV vaGreaterThanZero = IsGreaterThan(va, _zero);
	ScalarV d4Minusd3 = d4 - d3;
	BoolV d4Minusd3LessThanZero = IsLessThan(d4Minusd3, _zero);
	ScalarV d5Minusd6 = d5 - d6;
	BoolV d5Minusd6LessThanZero = IsLessThan(d5Minusd6, _zero);

	BoolV pNotInEdgeRegionOfBC = vaGreaterThanZero;
	pNotInEdgeRegionOfBC = Or(pNotInEdgeRegionOfBC, d4Minusd3LessThanZero);
	pNotInEdgeRegionOfBC = Or(pNotInEdgeRegionOfBC, d5Minusd6LessThanZero);

	ScalarV d4Minusd3Plusd5Minusd6; d4Minusd3Plusd5Minusd6 = Add(d4Minusd3, d5Minusd6);
	ScalarV u; u = InvScale(d4Minusd3, d4Minusd3Plusd5Minusd6);
	Vec3V pointOnBC;
	Vec3V bc = c - b;
	pointOnBC = AddScaled(b, bc, u);

	closestPoint = SelectFT(closestPointNotFound, closestPoint, pointOnBC);

	Vec3V zero_OneMinusU_U = Vec3V(u); // Vector3(0.0f, 1.0f - u.x, u.x)
	zero_OneMinusU_U = And(zero_OneMinusU_U, _maskz);

	{
		Vec3V zero_OneMinusU_Zero;
		zero_OneMinusU_Zero = Subtract(_1111, Vec3V(u));
		zero_OneMinusU_Zero = And(zero_OneMinusU_Zero, _masky);
		zero_OneMinusU_U = Add(zero_OneMinusU_U, zero_OneMinusU_Zero);
	}

	//Assert(IsClose(zero_OneMinusU_U, Vec3V(0.0f, 1.0f - u.x, u.x), VEC3_SMALL_FLOAT));

	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, zero_OneMinusU_U);
	closestPointNotFound = And(closestPointNotFound, pNotInEdgeRegionOfBC);

	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	ScalarV vaPlusvbPlusvc;
	vaPlusvbPlusvc = Add(va, vb);
	vaPlusvbPlusvc = Add(vaPlusvbPlusvc, vc);
	ScalarV denom = Invert(vaPlusvbPlusvc);

	ScalarV baryV = Scale( vb, denom );
	ScalarV baryW = Scale( vc, denom );

	Vec3V pointOnFace;
	pointOnFace = AddScaled(a, ab, baryV);
	pointOnFace = AddScaled(pointOnFace, ac, baryW);

	closestPoint = SelectFT(closestPointNotFound, closestPoint, pointOnFace);

	Vec3V oneMinusVW_V_W = Vec3V(baryW); // Vec3V(1.0f - baryV.x - baryW.x, baryV.x, baryW.x)
	oneMinusVW_V_W = And(oneMinusVW_V_W, _maskz);

	{
		Vec3V zero_V_Zero = Vec3V(baryV);
		zero_V_Zero = And(zero_V_Zero, _masky);
		oneMinusVW_V_W = Add(oneMinusVW_V_W, zero_V_Zero);
	}

	{
		ScalarV vPlusW = Add(baryV, baryW);
		Vec3V oneMinusVW_Zero_Zero;
		oneMinusVW_Zero_Zero = Subtract(_1111, Vec3V(vPlusW));
		oneMinusVW_Zero_Zero = And(oneMinusVW_Zero_Zero, _maskx);
		oneMinusVW_V_W = Add(oneMinusVW_V_W, oneMinusVW_Zero_Zero);
	}

	//Assert(IsClose(oneMinusVW_V_W, Vec3V(1.0f - baryV.x - baryW.x, baryV.x, baryW.x), VEC3_SMALL_FLOAT));

	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, oneMinusVW_V_W);
	barycentricCoordinates = And(barycentricCoordinates, _andw);

	result.m_BarycentricCoords = Vec4V(barycentricCoordinates);
	result.m_UsedVertices = IsGreaterThan(result.m_BarycentricCoords, Vec4V(_zero));
	result.m_ClosestPointOnSimplex = closestPoint;
}

/// Test if point p and d lie on opposite sides of plane through abc
int VoronoiDegenerateSimplexSolver::PointOutsideOfPlane(Vec3V_In p, Vec3V_In a, Vec3V_In b, Vec3V_In c, Vec3V_In d)
{
	Vec3V ab = Subtract(b, a);
	Vec3V ac = Subtract(c, a);
	Vec3V normal = Cross(ab, ac);

	Vec3V ap = Subtract(p, a);
	Vec3V ad = Subtract(d, a);
	ScalarV signp = Dot(ap, normal); // [AP AB AC]
	ScalarV signd = Dot(ad, normal); // [AD AB AC]

	// Points on opposite sides if expression signs are opposite
	return IsLessThanAll( Scale(signp, signd), ScalarV(V_ZERO) );
}


bool VoronoiDegenerateSimplexSolver::ClosestPtPointTetrahedron (Vec3V_In p, Vec3V_In a, Vec3V_In b, Vec3V_In c, Vec3V_In d, SubSimplexClosestResult& finalResult)
{
	// Start with the given point as the closest point on the simplex.
	finalResult.m_ClosestPointOnSimplex = Vec3V(V_ZERO);

	// Mark all the simplex vertices as used.
	finalResult.m_UsedVertices = VecBoolV(V_MASKXYZW);

	// See if the point p is on the opposite side of the plane of 3 tetrahedron points as the fourth tetrahedron point, for all four combinations.
	// 1 means they are on opposite sides, 0 means they are on the same side.
	int pointOutsideABC = PointOutsideOfPlane(p,a,b,c,d);
	int pointOutsideACD = PointOutsideOfPlane(p,a,c,d,b);
	int pointOutsideADB = PointOutsideOfPlane(p,a,d,b,c);
	int pointOutsideBDC = PointOutsideOfPlane(p,b,d,c,a);

	if (!pointOutsideABC  && !pointOutsideACD && !pointOutsideADB && !pointOutsideBDC)
	{
		// The simplex contains the point, return false since nothing else can be done.
		return false;
	}

	// If point outside face abc then compute closest point on abc
	SubSimplexClosestResult tempResult;
	ScalarV bestSqDist(V_FLT_MAX);
	if (pointOutsideABC) 
	{
		ClosestPtPointTriangle(p, a, b, c, tempResult);
		Vec3V q = tempResult.m_ClosestPointOnSimplex;

		ScalarV sqDist = DistSquared(q,p);

		// TODO: Can eliminate all branches like below, as long as VERTA,VERTB,VERTC reamin 0,1,2.

		// Update best closest point if (squared) distance is less than current best
		if (IsLessThanAll(sqDist, bestSqDist) != 0)
		{
			bestSqDist = sqDist;
			finalResult.m_ClosestPointOnSimplex = q;
			//convert result bitmask!
			finalResult.m_UsedVertices = tempResult.m_UsedVertices;
			finalResult.m_UsedVertices.SetW( BoolV(V_FALSE) );
			finalResult.m_BarycentricCoords = Vec4V(tempResult.m_BarycentricCoords[VERTA],tempResult.m_BarycentricCoords[VERTB],tempResult.m_BarycentricCoords[VERTC],0.0f);
		}
	}

	// Repeat test for face acd
	if (pointOutsideACD) 
	{
		ClosestPtPointTriangle(p, a, c, d, tempResult);
		Vec3V q = tempResult.m_ClosestPointOnSimplex;
		//convert result bitmask!

		ScalarV sqDist = DistSquared(q,p);
		if (IsLessThanAll(sqDist, bestSqDist) != 0)
		{
			bestSqDist = sqDist;
			finalResult.m_ClosestPointOnSimplex = q;
			finalResult.m_UsedVertices.SetX( tempResult.m_UsedVertices.GetX() );
			finalResult.m_UsedVertices.SetZ( tempResult.m_UsedVertices.GetY() );
			finalResult.m_UsedVertices.SetW( tempResult.m_UsedVertices.GetZ() );
			finalResult.m_UsedVertices.SetY( BoolV(V_FALSE) );
			finalResult.m_BarycentricCoords = Vec4V(tempResult.m_BarycentricCoords[VERTA],0.0f,tempResult.m_BarycentricCoords[VERTB],tempResult.m_BarycentricCoords[VERTC]);
		}
	}

	// Repeat test for face adb
	if (pointOutsideADB)
	{
		ClosestPtPointTriangle(p, a, d, b,tempResult);
		Vec3V q = tempResult.m_ClosestPointOnSimplex;
		//convert result bitmask!

		ScalarV sqDist = DistSquared(q,p);
		if (IsLessThanAll(sqDist, bestSqDist) != 0) 
		{
			bestSqDist = sqDist;
			finalResult.m_ClosestPointOnSimplex = q;
			finalResult.m_UsedVertices.SetX( tempResult.m_UsedVertices.GetX() );
			finalResult.m_UsedVertices.SetW( tempResult.m_UsedVertices.GetY() );
			finalResult.m_UsedVertices.SetY( tempResult.m_UsedVertices.GetZ() );
			finalResult.m_UsedVertices.SetZ( BoolV(V_FALSE) );
			finalResult.m_BarycentricCoords = Vec4V(tempResult.m_BarycentricCoords[VERTA],tempResult.m_BarycentricCoords[VERTC],0.0f,tempResult.m_BarycentricCoords[VERTB]);
		}
	}

	// Repeat test for face bdc
	if (pointOutsideBDC)
	{
		ClosestPtPointTriangle(p, b, d, c,tempResult);
		Vec3V q = tempResult.m_ClosestPointOnSimplex;
		//convert result bitmask!
		ScalarV sqDist = DistSquared(q,p);
		if (IsLessThanAll(sqDist, bestSqDist) != 0)
		{
			bestSqDist = sqDist;
			finalResult.m_ClosestPointOnSimplex = q;
			finalResult.m_UsedVertices.SetY( tempResult.m_UsedVertices.GetX() );
			finalResult.m_UsedVertices.SetW( tempResult.m_UsedVertices.GetY() );
			finalResult.m_UsedVertices.SetZ( tempResult.m_UsedVertices.GetZ() );
			finalResult.m_UsedVertices.SetX( BoolV(V_FALSE) );
			finalResult.m_BarycentricCoords = Vec4V(0.0f,tempResult.m_BarycentricCoords[VERTA],tempResult.m_BarycentricCoords[VERTC],tempResult.m_BarycentricCoords[VERTB]);
		}
	}

	Assertf(!IsFalseAll(finalResult.m_UsedVertices), "All the simplex vertices were marked for removal.");
	return true;
}

} // namespace rage
