
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


#include "VoronoiSimplexSolver.h"
#include <assert.h>
#include <stdio.h>

#include "phbound/bound.h"
#include "phbound/support.h"

#include "vector/allbitsf.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

GJK_COLLISION_OPTIMIZE_OFF()

#define VERTA  0
#define VERTB  1
#define VERTC  2
#define VERTD  3

namespace rage {


#define CATCH_DEGENERATE_TETRAHEDRON 1

#if !USE_NEW_SIMPLEX_SOLVER
void	VoronoiSimplexSolver::RemoveVertex(int index)
{
	// Decrement the number of vertices.
	FastAssert(m_numVertices > 0);
	FastAssert(m_numVertices <= 4);
	FastAssert(index >= 0 && index < m_numVertices);
	const int numVertices = m_numVertices - 1;
	m_numVertices = numVertices;

	// Put what was the last vertex into the removed vertex's position.
	m_SimplexSeparation[index] = m_SimplexSeparation[numVertices];
	m_VertexIndexA[index] = m_VertexIndexA[numVertices];
	m_VertexIndexB[index] = m_VertexIndexB[numVertices];
	//m_VertexIndexA_4V[index] = m_VertexIndexA_4V[numVertices];
	//m_VertexIndexB_4V[index] = m_VertexIndexB_4V[numVertices];

	m_LocalA[index] = m_LocalA[numVertices];
	m_LocalB[index] = m_LocalB[numVertices];

	m_SimplexCoords[index] = m_SimplexCoords[numVertices];
}

void VoronoiSimplexSolver::ReduceVertices (Vec4V_In usedVerts)
{
	FastAssert(m_numVertices > 0 && m_numVertices <= 4);

	const Vec4V _zero(V_ZERO);
	const Vec4V _maskX(V_MASKX);
	const Vec4V _maskY(V_MASKY);
	const Vec4V _maskZ(V_MASKZ);
	const Vec4V _maskW(V_MASKW);

	switch (m_numVertices)
	{
		//if (m_numVertices >= 4)
		case 4:
		{
			if ( IsEqualAll(And(usedVerts, _maskW), _zero) != 0 )
			{
				// There are at least 4 vertices and the 4th one was not used, so remove it.
				RemoveVertex(3);
			}
		}

		//if (m_numVertices >= 3)
		case 3:
		{
			if ( IsEqualAll(And(usedVerts, _maskZ), _zero) != 0 )
			{
				// There are at least 3 vertices and the 3rd one was not used, so remove it.
				RemoveVertex(2);
			}
		}

		//if (m_numVertices >= 2)
		case 2:
		{
			if ( IsEqualAll(And(usedVerts, _maskY), _zero) != 0 )
			{
				// There are at least 2 vertices and the 2nd one was not used, so remove it.
				RemoveVertex(1);
			}
		}

		//if (m_numVertices >= 1)
		case 1:
		{
			if ( IsEqualAll(And(usedVerts, _maskX), _zero) != 0 )
			{
				// There is at least 1 vertex and the 1st one was not used, so remove it.
				RemoveVertex(0);
			}
		}
	}
}

bool VoronoiSimplexSolver::UpdateClosestVectorAndPoints()
{
	bool validClosest = false;
	if (m_needsUpdate)
	{
		FastAssert(m_numVertices >= 1 && m_numVertices <= 4);
		SubSimplexClosestResult m_CachedResult;

		m_CachedResult.Reset();

		m_needsUpdate = false;

		switch (numVertices())
		{
			case 0:
			{
				validClosest = false;
				break;
			}

			case 1:
			{
				m_SimplexCoords[0] = ScalarV(V_ONE);
				m_cachedV = m_SimplexSeparation[0];
				validClosest = true;// we just set it to something valid...m_CachedResult.IsValid();
				break;
			}

			case 2:
			{
				//closest point origin from line segment
				const Vec3V from = m_SimplexSeparation[0];
				const Vec3V to = m_SimplexSeparation[1];
				
				const Vec3V diff = Vec3V(V_ZERO) - from;
				const Vec3V v = to - from;

				const ScalarV t = Dot(v, diff);

				if ( IsGreaterThanAll(t, ScalarV(V_ZERO)) != 0 )
				{
					const ScalarV dotVV = MagSquared( v );
					if ( IsLessThanAll(t, dotVV) != 0 )
					{
						m_SimplexCoords[1] = t / dotVV;
						m_SimplexCoords[0] = ScalarV(V_ONE) - m_SimplexCoords[1];
						m_cachedV = m_SimplexSeparation[0] + m_SimplexCoords[1] * (m_SimplexSeparation[1] - m_SimplexSeparation[0]);
					}
					else
					{
						m_SimplexCoords[1] = ScalarV(V_ONE);
						m_cachedV = m_SimplexSeparation[1];
						RemoveVertex(0);
					}
				}
				else
				{
					m_SimplexCoords[0] = ScalarV(V_ONE);
					m_cachedV = m_SimplexSeparation[0];
					m_numVertices = 1;
				}

				validClosest = true;//we just set it to something valid...m_CachedResult.IsValid();
				break;
			}

			case 3:
			{
				Vec3V a = m_SimplexSeparation[0];
				Vec3V b = m_SimplexSeparation[1];
				Vec3V c = m_SimplexSeparation[2];

				ClosestPtPointTriangle(	VEC3V_TO_INTRIN(a),
										VEC3V_TO_INTRIN(b),
										VEC3V_TO_INTRIN(c),
										m_CachedResult	);

				m_SimplexCoords[0] = SplatX(m_CachedResult.m_barycentricCoords);
				m_SimplexCoords[1] = SplatY(m_CachedResult.m_barycentricCoords);
				m_SimplexCoords[2] = SplatZ(m_CachedResult.m_barycentricCoords);
				ReduceVertices (  m_CachedResult.m_usedVertices );
				FastAssert(m_numVertices > 0 && m_numVertices <= 3);

				m_cachedV = m_CachedResult.m_closestPointOnSimplex;

				validClosest = m_CachedResult.IsValid();
				break;
			}

			case 4:
			{
				Vec3V a = m_SimplexSeparation[0];
				Vec3V b = m_SimplexSeparation[1];
				Vec3V c = m_SimplexSeparation[2];
				Vec3V d = m_SimplexSeparation[3];

				bool hasSeperation = ClosestPtPointTetrahedron(	a.GetIntrin128(),
																b.GetIntrin128(),
																c.GetIntrin128(),
																d.GetIntrin128(),
																m_CachedResult	);

				if (hasSeperation)
				{
					m_SimplexCoords[0] = SplatX(m_CachedResult.m_barycentricCoords);
					m_SimplexCoords[1] = SplatY(m_CachedResult.m_barycentricCoords);
					m_SimplexCoords[2] = SplatZ(m_CachedResult.m_barycentricCoords);
					m_SimplexCoords[3] = SplatW(m_CachedResult.m_barycentricCoords);
					ReduceVertices (  m_CachedResult.m_usedVertices );
					FastAssert(m_numVertices > 0 && m_numVertices <= 3);

					m_cachedV = m_CachedResult.m_closestPointOnSimplex;

					validClosest = m_CachedResult.IsValid();
				}
				else
				{
//					printf("sub distance got penetration\n");

					if (m_CachedResult.m_degenerate)
					{
						validClosest = false;
					}
					else
					{
						validClosest = true;
						//degenerate case == false, penetration = true + zero
						m_cachedV = Vec3V(V_ZERO);
					}

				}
				break;
			}
		
			default:
			{
				validClosest = false;
			}
		}
	}

	return validClosest;
}

//void VoronoiSimplexSolver::backup_closest(Vec3V_InOut v) 
//{
//	v = m_cachedV;
//}


//bool VoronoiSimplexSolver::emptySimplex() const 
//{
//	return (numVertices() == 0);
//
//}


//#if __WIN32PC
//#pragma optimize("", off)
//
//bool OldClosestPtPointTriangle(Vector3::Param p, Vector3::Param a, Vector3::Param b, Vector3::Param c,SubSimplexClosestResult& result)
//{
//    result.m_usedVertices.Zero();
//
//    // Check if P in vertex region outside A
//    Vector3 ab; ab.Subtract(b, a);
//    Vector3 ac; ac.Subtract(c, a);
//    Vector3 ap; ap.Subtract(p, a);
//    float d1 = ab.Dot(ap);
//    float d2 = ac.Dot(ap);
//    if (d1 <= 0.0f && d2 <= 0.0f) 
//    {
//        result.m_closestPointOnSimplex = a;
//        result.m_usedVertices.x = allBitsF;
//        result.m_barycentricCoords.Set(1,0,0,0);
//        return true;// a; // barycentric coordinates (1,0,0)
//    }
//
//    // Check if P in vertex region outside B
//    Vector3 bp; bp.Subtract(p, b);
//    float d3 = ab.Dot(bp);
//    float d4 = ac.Dot(bp);
//    if (d3 >= 0.0f && d4 <= d3) 
//    {
//        result.m_closestPointOnSimplex = b;
//        result.m_usedVertices.y = allBitsF;
//        result.m_barycentricCoords.Set(0,1,0,0);
//
//        return true; // b; // barycentric coordinates (0,1,0)
//    }
//    // Check if P in edge region of AB, if so return projection of P onto AB
//    float vc = d1*d4 - d3*d2;
//    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
//        float v = d1 / (d1 - d3);
//        result.m_closestPointOnSimplex.AddScaled(a, ab, v);
//        result.m_usedVertices.x = allBitsF;
//        result.m_usedVertices.y = allBitsF;
//        result.m_barycentricCoords.Set(1-v,v,0,0);
//        return true;
//        //return a + v * ab; // barycentric coordinates (1-v,v,0)
//    }
//
//    // Check if P in vertex region outside C
//    Vector3 cp; cp.Subtract(p, c);
//    float d5 = ab.Dot(cp);
//    float d6 = ac.Dot(cp);
//    if (d6 >= 0.0f && d5 <= d6) 
//    {
//        result.m_closestPointOnSimplex = c;
//        result.m_usedVertices.z = allBitsF;
//        result.m_barycentricCoords.Set(0,0,1,0);
//        return true;//c; // barycentric coordinates (0,0,1)
//    }
//
//    // Check if P in edge region of AC, if so return projection of P onto AC
//    float vb = d5*d2 - d1*d6;
//    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
//        float w = d2 / (d2 - d6);
//        result.m_closestPointOnSimplex.AddScaled(a, ac, w);
//        result.m_usedVertices.x = allBitsF;
//        result.m_usedVertices.z = allBitsF;
//        result.m_barycentricCoords.Set(1-w,0,w,0);
//        return true;
//        //return a + w * ac; // barycentric coordinates (1-w,0,w)
//    }
//
//    // Check if P in edge region of BC, if so return projection of P onto BC
//    float va = d3*d6 - d5*d4;
//    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
//        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
//
//        Vector3 bc; bc.Subtract(c, b);
//        result.m_closestPointOnSimplex.AddScaled(b, bc, w);
//        result.m_usedVertices.y = allBitsF;
//        result.m_usedVertices.z = allBitsF;
//        result.m_barycentricCoords.Set(0,1-w,w,0);
//        return true;		
//        // return b + w * (c - b); // barycentric coordinates (0,1-w,w)
//    }
//
//    // P inside face region. Compute Q through its barycentric coordinates (u,v,w)
//    float denom = 1.0f / (va + vb + vc);
//    float v = vb * denom;
//    float w = vc * denom;
//
//    result.m_closestPointOnSimplex.AddScaled(a, ab, v);
//    result.m_closestPointOnSimplex.AddScaled(ac, w);
//	result.m_usedVertices.x = allBitsF;
//	result.m_usedVertices.y = allBitsF;
//	result.m_usedVertices.z = allBitsF;
//    result.m_barycentricCoords.Set(1-v-w,v,w,0);
//
//    return true;
//    //	return a + ab * v + ac * w; // = u*a + v*b + w*c, u = va * denom = 1.0f - v - w
//
//}
//
//#pragma optimize("", on)
//#endif // __WIN32

bool	VoronoiSimplexSolver::ClosestPtPointTriangle(Vec::V3Param128 a, Vec::V3Param128 b, Vec::V3Param128_After3Args c,SubSimplexClosestResult& result)
{
//#if __WIN32PC
//	OldClosestPtPointTriangle(p, a, b, c, result);
//#else
	// New vector generation method (no loads!).
	Vec::Vector_4V v_all0 = Vec::V4VConstant(V_ZERO);
	Vec::Vector_4V v_allF = Vec::V4VConstant(V_MASKXYZW);
	Vec::Vector_4V v_all1 = Vec::V4VConstant(V_ONE);
	// 1,0,0, Fast on PS3/Xenon
	Vec::Vector_4V v_xaxis = Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>(v_all1, v_all0);
	// 0,1,0, Fast on PS3/Xenon
	Vec::Vector_4V v_yaxis = Vec::V4MergeXY(v_all0, v_all1);
	// 0,0,1, Fast on PS3/Xenon
	Vec::Vector_4V v_zaxis = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>(v_all0, v_all1);
	// FFFFFFFF,0,0, Fast on PS3/Xenon
	Vec::Vector_4V v_maskx = Vec::V4PermuteTwo<Vec::W1,Vec::X2,Vec::Y2,Vec::Z2>(v_allF, v_all0);
	// 0,FFFFFFFF,0, Fast on PS3/Xenon
	Vec::Vector_4V v_masky = Vec::V4MergeXY(v_all0, v_allF);
	// 0,0,FFFFFFFF, Fast on PS3/Xenon
	Vec::Vector_4V v_maskz = Vec::V4PermuteTwo<Vec::Z1,Vec::W1,Vec::X2,Vec::Y2>(v_all0, v_allF);
	// FFFFFFFF,FFFFFFFF,FFFFFFFF,0 Fast on PS3/Xenon
	Vec::Vector_4V v_andw = Vec::V4PermuteTwo<Vec::Y1,Vec::Z1,Vec::W1,Vec::X2>(v_allF, v_all0);

	// Convert to class types.
	Vec4V _v4zero(v_all0);
	Vec3V _zero(v_all0);
	Vec3V _1111(v_all1);
	Vec3V _ffffffff(v_allF);
	Vec3V _xaxis(v_xaxis);
	Vec3V _yaxis(v_yaxis);
	Vec3V _zaxis(v_zaxis);
	Vec3V _maskx(v_maskx);
	Vec3V _masky(v_masky);
	Vec3V _maskz(v_maskz);
	Vec3V _andw(v_andw);

	Vec3V _a = INTRIN_TO_VEC3V(a);
	Vec3V _b = INTRIN_TO_VEC3V(b);
	Vec3V _c = INTRIN_TO_VEC3V(c);

	Vec3V ab = _b - _a;
	Vec3V ap = -_a;
	Vec3V d1 = Vec3V(Dot(ab, ap));
	Vec3V ac = _c - _a;
	Vec3V d2 = Vec3V(Dot(ac, ap));

	// Check if P in vertex region outside A (d1 <= 0.0f && d2 <= 0.0f)
	VecBoolV d1GreaterThanZero = IsGreaterThan(d1, _zero);
	VecBoolV d2GreaterThanZero = IsGreaterThan(d2, _zero);

	// If closestPointNotFound ever goes zero, then the computation of closestPoint is complete and
	// the rest of the equations should just "fall through".
	VecBoolV closestPointNotFound = d1GreaterThanZero;
	closestPointNotFound = Or(closestPointNotFound, d2GreaterThanZero);

	Vec3V bp = -_b;
	Vec3V d3 = Vec3V( Dot(ab, bp) );
	Vec3V d4 = Vec3V( Dot(ac, bp) );

	// Check if P in vertex region outside B (d3 >= 0.0f && d4 <= d3)
	VecBoolV d3LessThanZero = IsLessThan(d3, _zero);
	VecBoolV d4GreaterThand3 = IsGreaterThan(d4, d3);

	// closestPointNotFound is currently zero if a should be the result...
	VecBoolV pNotInVertexRegionOutsideB = d3LessThanZero;
	pNotInVertexRegionOutsideB = Or(pNotInVertexRegionOutsideB, d4GreaterThand3);

	Vec3V closestPoint = SelectFT(closestPointNotFound, _a, _b);
	Vec3V barycentricCoordinates = SelectFT(closestPointNotFound, _xaxis, _yaxis);
	closestPointNotFound = And(closestPointNotFound, pNotInVertexRegionOutsideB);
	// ...now closestPointNotFound is currently zero if closestPoint (which is either a or b) should be the result

	// Check if P in edge region of AB, if so return projection of P onto AB (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
	Vec3V d1d4 = Scale( d1, d4 );
	Vec3V d3d2 = Scale( d3, d2 );
	Vec3V vc = d1d4 - d3d2;

	VecBoolV vcGreaterThanZero = IsGreaterThan(vc, _zero);
	VecBoolV d1LessThanZero = IsLessThan(d1, _zero);
	VecBoolV d3GreaterThanZero = IsGreaterThan(d3, _zero);

	VecBoolV pNotInEdgeRegionOfAB = vcGreaterThanZero;
	pNotInEdgeRegionOfAB = Or(pNotInEdgeRegionOfAB, d1LessThanZero);
	pNotInEdgeRegionOfAB = Or(pNotInEdgeRegionOfAB, d3GreaterThanZero);

	Vec3V d1Minusd3 = d1 - d3;
	Vec3V v; v = InvScale( d1, d1Minusd3 );
	Vec3V pointOnAB = AddScaled(_a, ab, v);
	closestPoint = SelectFT(closestPointNotFound, closestPoint, pointOnAB);


	Vec3V oneMinusV_V_Zero(v); // Vector3(1.0f - v.x, v.x, 0.0f)
	oneMinusV_V_Zero = And(oneMinusV_V_Zero, _masky);

	{
		Vec3V oneMinusV_Zero_Zero;
		oneMinusV_Zero_Zero = Subtract(_1111, v);
		oneMinusV_Zero_Zero = And(oneMinusV_Zero_Zero, _maskx);
		oneMinusV_V_Zero = Add(oneMinusV_V_Zero, oneMinusV_Zero_Zero);
	}

	//Assert(IsClose(oneMinusV_V_Zero, Vec3V(1.0f - v.x, v.x, 0.0f), VEC3_SMALL_FLOAT));

	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, oneMinusV_V_Zero);
	closestPointNotFound = And(closestPointNotFound, pNotInEdgeRegionOfAB);

	Vec3V cp = -_c;
	Vec3V d5 = Vec3V( Dot(ab, cp) );
	Vec3V d6 = Vec3V( Dot(ac, cp) );

	// Check if P in vertex region outside C
	VecBoolV d6LessThanZero = IsLessThan(d6, _zero);
	VecBoolV d5GreaterThand6 = IsGreaterThan(d5, d6);

	VecBoolV pNotInVertexRegionOutsideC = d6LessThanZero;
	pNotInVertexRegionOutsideC = Or(pNotInVertexRegionOutsideC, d5GreaterThand6);

	closestPoint = SelectFT(closestPointNotFound, closestPoint, _c);
	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, _zaxis);
	closestPointNotFound = And(closestPointNotFound, pNotInVertexRegionOutsideC);

	// Check if P in edge region of AC, if so return projection of P onto AC
	Vec3V d5d2 = Scale( d5, d2 );
	Vec3V d1d6 = Scale( d1, d6 );
	Vec3V vb = d5d2 - d1d6;

	VecBoolV vbGreaterThanZero = IsGreaterThan(vb, _zero);
	VecBoolV d2LessThanZero = IsLessThan(d2, _zero);
	VecBoolV d6GreaterThanZero = IsGreaterThan(d6, _zero);

	VecBoolV pNotInEdgeRegionOfAC = vbGreaterThanZero;
	pNotInEdgeRegionOfAC = Or(pNotInEdgeRegionOfAC, d2LessThanZero);
	pNotInEdgeRegionOfAC = Or(pNotInEdgeRegionOfAC, d6GreaterThanZero);

	Vec3V d2Minusd6 = d2 - d6;
	Vec3V w; w = InvScale(d2, d2Minusd6);
	Vec3V pointOnAC;
	pointOnAC = AddScaled(_a, ac, w);
	closestPoint = SelectFT(closestPointNotFound, closestPoint, pointOnAC);

	Vec3V oneMinusW_Zero_W(w); // Vector3(1.0f - w.x, 0.0f, w.x)
	oneMinusW_Zero_W = And(oneMinusW_Zero_W, _maskz);

	{
		Vec3V oneMinusW_Zero_Zero;
		oneMinusW_Zero_Zero = Subtract(_1111, w);
		oneMinusW_Zero_Zero = And(oneMinusW_Zero_Zero, _maskx);
		oneMinusW_Zero_W = Add(oneMinusW_Zero_W, oneMinusW_Zero_Zero);
	}

	//Assert(IsClose(oneMinusW_Zero_W, Vec3V(1.0f - w.x, 0.0f, w.x), VEC3_SMALL_FLOAT));

	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, oneMinusW_Zero_W);
	closestPointNotFound = And(closestPointNotFound, pNotInEdgeRegionOfAC);

	// Check if P in edge region of BC, if so return projection of P onto BC
	Vec3V d3d6 = Scale( d3, d6 );
	Vec3V d5d4 = Scale( d5, d4 );
	Vec3V va = d3d6 - d5d4;

	VecBoolV vaGreaterThanZero = IsGreaterThan(va, _zero);
	Vec3V d4Minusd3 = d4 - d3;
	VecBoolV d4Minusd3LessThanZero = IsLessThan(d4Minusd3, _zero);
	Vec3V d5Minusd6 = d5 - d6;
	VecBoolV d5Minusd6LessThanZero = IsLessThan(d5Minusd6, _zero);

	VecBoolV pNotInEdgeRegionOfBC = vaGreaterThanZero;
	pNotInEdgeRegionOfBC = Or(pNotInEdgeRegionOfBC, d4Minusd3LessThanZero);
	pNotInEdgeRegionOfBC = Or(pNotInEdgeRegionOfBC, d5Minusd6LessThanZero);

	Vec3V d4Minusd3Plusd5Minusd6; d4Minusd3Plusd5Minusd6 = Add(d4Minusd3, d5Minusd6);
	Vec3V u; u = InvScale(d4Minusd3, d4Minusd3Plusd5Minusd6);
	Vec3V pointOnBC;
	Vec3V bc = _c - _b;
	pointOnBC = AddScaled(_b, bc, u);

	closestPoint = SelectFT(closestPointNotFound, closestPoint, pointOnBC);

	Vec3V zero_OneMinusU_U(u); // Vector3(0.0f, 1.0f - u.x, u.x)
	zero_OneMinusU_U = And(zero_OneMinusU_U, _maskz);

	{
		Vec3V zero_OneMinusU_Zero;
		zero_OneMinusU_Zero = Subtract(_1111, u);
		zero_OneMinusU_Zero = And(zero_OneMinusU_Zero, _masky);
		zero_OneMinusU_U = Add(zero_OneMinusU_U, zero_OneMinusU_Zero);
	}

	//Assert(IsClose(zero_OneMinusU_U, Vec3V(0.0f, 1.0f - u.x, u.x), VEC3_SMALL_FLOAT));

	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, zero_OneMinusU_U);
	closestPointNotFound = And(closestPointNotFound, pNotInEdgeRegionOfBC);

	// P inside face region. Compute Q through its barycentric coordinates (u,v,w)
	Vec3V vaPlusvbPlusvc;
	vaPlusvbPlusvc = Add(va, vb);
	vaPlusvbPlusvc = Add(vaPlusvbPlusvc, vc);
	Vec3V denom = InvScale(_1111, vaPlusvbPlusvc);

	Vec3V baryV = Scale( vb, denom );
	Vec3V baryW = Scale( vc, denom );

	Vec3V pointOnFace;
	pointOnFace = AddScaled(_a, ab, baryV);
	pointOnFace = AddScaled(pointOnFace, ac, baryW);

	closestPoint = SelectFT(closestPointNotFound, closestPoint, pointOnFace);

	Vec3V oneMinusVW_V_W(baryW); // Vec3V(1.0f - baryV.x - baryW.x, baryV.x, baryW.x)
	oneMinusVW_V_W = And(oneMinusVW_V_W, _maskz);

	{
		Vec3V zero_V_Zero(baryV);
		zero_V_Zero = And(zero_V_Zero, _masky);
		oneMinusVW_V_W = Add(oneMinusVW_V_W, zero_V_Zero);
	}

	{
		Vec3V vPlusW = Add(baryV, baryW);
		Vec3V oneMinusVW_Zero_Zero;
		oneMinusVW_Zero_Zero = Subtract(_1111, vPlusW);
		oneMinusVW_Zero_Zero = And(oneMinusVW_Zero_Zero, _maskx);
		oneMinusVW_V_W = Add(oneMinusVW_V_W, oneMinusVW_Zero_Zero);
	}

	//Assert(IsClose(oneMinusVW_V_W, Vec3V(1.0f - baryV.x - baryW.x, baryV.x, baryW.x), VEC3_SMALL_FLOAT));

	barycentricCoordinates = SelectFT(closestPointNotFound, barycentricCoordinates, oneMinusVW_V_W);
	barycentricCoordinates = And(barycentricCoordinates, _andw);

	result.m_barycentricCoords = Vec4V(barycentricCoordinates);
	result.m_usedVertices = Vec4V( IsGreaterThan(result.m_barycentricCoords, _v4zero) );
	result.m_closestPointOnSimplex = closestPoint;

#if 0
	SubSimplexClosestResult oldResult;
	OldClosestPtPointTriangle(p, a, b, c, oldResult);
	result.m_closestPointOnSimplex.w = 0.0f;
	oldResult.m_closestPointOnSimplex.w = 0.0f;
	ASSERT_ONLY(float epsilon = fabs(result.m_closestPointOnSimplex.x) + fabs(result.m_closestPointOnSimplex.y) + fabs(result.m_closestPointOnSimplex.z));
	Assert(result.m_closestPointOnSimplex.IsClose(oldResult.m_closestPointOnSimplex, fabs(epsilon * 1.0e-3f)));
	result.m_barycentricCoords.w = 0.0f;
	oldResult.m_barycentricCoords.w = 0.0f;
	Assert(result.m_barycentricCoords.IsClose(oldResult.m_barycentricCoords, 1.0e-3f));
	result.m_usedVertices.w = 0.0f;
	oldResult.m_usedVertices.w = 0.0f;
	Assert(reinterpret_cast<u32&>(result.m_usedVertices.x) == reinterpret_cast<u32&>(oldResult.m_usedVertices.x));
	Assert(reinterpret_cast<u32&>(result.m_usedVertices.y) == reinterpret_cast<u32&>(oldResult.m_usedVertices.y));
	Assert(reinterpret_cast<u32&>(result.m_usedVertices.z) == reinterpret_cast<u32&>(oldResult.m_usedVertices.z));
#endif
//#endif

	return true;
}




/// Test if point p and d lie on opposite sides of plane through abc
int VoronoiSimplexSolver::PointOutsideOfPlane(Vec::V3Param128 a, Vec::V3Param128 b, Vec::V3Param128_After3Args c, Vec::V3Param128_After3Args d)
{
	Vec3V _a(a);
	Vec3V _b(b);
	Vec3V _c(c);
	Vec3V _d(d);

    Vec3V ab = Subtract(_b, _a);
    Vec3V ac = Subtract(_c, _a);
	Vec3V normal = Cross(ab, ac);

    Vec3V ap = Negate(_a);
    Vec3V ad = Subtract(_d, _a);
    Vec3V signp = Vec3V(Dot(ap, normal)); // [AP AB AC]
    Vec3V signd = Vec3V(Dot(ad, normal)); // [AD AB AC]

#ifdef CATCH_DEGENERATE_TETRAHEDRON
	if (IsLessThanAll(Vec3V(MagSquared(signd)), Vec3V(V_FLT_SMALL_6)) != 0)
	{
//		printf("affine dependent/degenerate\n");//
		return -1;
	}
#endif
	// Points on opposite sides if expression signs are opposite
	return IsLessThanAll( Scale(signp, signd), Vec3V(V_ZERO) );
}

bool VoronoiSimplexSolver::ClosestPtPointTetrahedron (Vec::V3Param128 a, Vec::V3Param128 b, Vec::V3Param128_After3Args c, Vec::V3Param128_After3Args d, SubSimplexClosestResult& finalResult)
{
	Vec3V _a(a);
	Vec3V _b(b);
	Vec3V _c(c);
	Vec3V _d(d);

	// Start with the given point as the closest point on the simplex.
	finalResult.m_closestPointOnSimplex = Vec3V(V_ZERO);

	// Mark all the simplex vertices as used.
	finalResult.m_usedVertices = Vec4V(V_MASKXYZW);

	// See if the point p is on the opposite side of the plane of 3 tetrahedron points as the fourth tetrahedron point, for all four combinations.
	// 1 means they are on opposite sides, 0 means they are on the same side, and -1 means the tetrahedron is flat.
	const int pointOutsideABC = PointOutsideOfPlane(a,b,c,d);
	if (pointOutsideABC < 0)
	{
		// The tetrahedron is flat, so return false to indicate there is no separation (the point can't be used to remove a tetrahedron corner).
		finalResult.m_degenerate = true;
		return false;
	}

	const int pointOutsideACD = PointOutsideOfPlane(a,c,d,b);
	if (pointOutsideACD < 0)
	{
		// The tetrahedron is flat, so return false to indicate there is no separation (the point can't be used to remove a tetrahedron corner).
		finalResult.m_degenerate = true;
		return false;
	}

	const int pointOutsideADB = PointOutsideOfPlane(a,d,b,c);
	if (pointOutsideADB < 0)
	{
		// The tetrahedron is flat, so return false to indicate there is no separation (the point can't be used to remove a tetrahedron corner).
		finalResult.m_degenerate = true;
		return false;
	}

	const int pointOutsideBDC = PointOutsideOfPlane(b,d,c,a);
	if (pointOutsideBDC < 0)
	{
		// The tetrahedron is flat, so return false to indicate there is no separation (the point can't be used to remove a tetrahedron corner).
		finalResult.m_degenerate = true;
		return false;
	}

	if (!pointOutsideABC  && !pointOutsideACD && !pointOutsideADB && !pointOutsideBDC)
	{
		// The point is on the same side as the fourth corner for all four triangles,
		// so return false to indicate the point can't be used to remove a tetrahedron corner.
		return false;
	}

	// If point outside face abc then compute closest point on abc
	SubSimplexClosestResult tempResult;
	Vec3V bestSqDist(V_FLT_MAX);
	if (pointOutsideABC) 
	{
		ClosestPtPointTriangle(a, b, c, tempResult);
		Vec3V q = tempResult.m_closestPointOnSimplex;

		Vec3V sqDist = Vec3V( MagSquared(q) );

		// TODO: Can eliminate all branches like below, as long as VERTA,VERTB,VERTC reamin 0,1,2.

		// Update best closest point if (squared) distance is less than current best
		if (IsLessThanAll(sqDist, bestSqDist) != 0) {
			bestSqDist = sqDist;
			finalResult.m_closestPointOnSimplex = q;
			//convert result bitmask!
			finalResult.m_usedVertices = tempResult.m_usedVertices;
			finalResult.m_usedVertices.SetWZero();
			finalResult.m_barycentricCoords = Vec4V(tempResult.m_barycentricCoords[VERTA],tempResult.m_barycentricCoords[VERTB],tempResult.m_barycentricCoords[VERTC],0.0f);
		}
	}

	// Repeat test for face acd
	if (pointOutsideACD) 
	{
		ClosestPtPointTriangle(a, c, d, tempResult);
		Vec3V q = tempResult.m_closestPointOnSimplex;
		//convert result bitmask!

		Vec3V sqDist = Vec3V( MagSquared(q) );
		if (IsLessThanAll(sqDist, bestSqDist) != 0)
		{
			bestSqDist = sqDist;
			finalResult.m_closestPointOnSimplex = q;
			finalResult.m_usedVertices.SetXf( tempResult.m_usedVertices.GetXf() );
			finalResult.m_usedVertices.SetZf( tempResult.m_usedVertices.GetYf() );
			finalResult.m_usedVertices.SetWf( tempResult.m_usedVertices.GetZf() );
			finalResult.m_usedVertices.SetYf( 0.0f );
			finalResult.m_barycentricCoords = Vec4V(tempResult.m_barycentricCoords[VERTA],0.0f,tempResult.m_barycentricCoords[VERTB],tempResult.m_barycentricCoords[VERTC]);
		}
	}

	// Repeat test for face adb
	if (pointOutsideADB)
	{
		ClosestPtPointTriangle(a, d, b,tempResult);
		Vec3V q = tempResult.m_closestPointOnSimplex;
		//convert result bitmask!

		Vec3V sqDist = Vec3V( MagSquared(q) );
		if (IsLessThanAll(sqDist, bestSqDist) != 0) 
		{
			bestSqDist = sqDist;
			finalResult.m_closestPointOnSimplex = q;
			finalResult.m_usedVertices.SetXf( tempResult.m_usedVertices.GetXf() );
			finalResult.m_usedVertices.SetWf( tempResult.m_usedVertices.GetYf() );
			finalResult.m_usedVertices.SetYf( tempResult.m_usedVertices.GetZf() );
			finalResult.m_usedVertices.SetZf( 0.0f );
			finalResult.m_barycentricCoords = Vec4V(tempResult.m_barycentricCoords[VERTA],tempResult.m_barycentricCoords[VERTC],0.0f,tempResult.m_barycentricCoords[VERTB]);
		}
	}

	// Repeat test for face bdc
	if (pointOutsideBDC)
	{
		ClosestPtPointTriangle(b, d, c,tempResult);
		Vec3V q = tempResult.m_closestPointOnSimplex;
		//convert result bitmask!
		Vec3V sqDist = Vec3V( MagSquared(q) );
		if (IsLessThanAll(sqDist, bestSqDist) != 0)
		{
			bestSqDist = sqDist;
			finalResult.m_closestPointOnSimplex = q;
			finalResult.m_usedVertices.SetYf( tempResult.m_usedVertices.GetXf() );
			finalResult.m_usedVertices.SetWf( tempResult.m_usedVertices.GetYf() );
			finalResult.m_usedVertices.SetZf( tempResult.m_usedVertices.GetZf() );
			finalResult.m_usedVertices.SetXf( 0.0f );
			finalResult.m_barycentricCoords = Vec4V(0.0f,tempResult.m_barycentricCoords[VERTA],tempResult.m_barycentricCoords[VERTC],tempResult.m_barycentricCoords[VERTB]);
		}
	}

	DebugAssert(/*"All the simplex vertices were marked for removal." && */ 0==IsEqualAll(finalResult.m_usedVertices, Vec4V(V_ZERO)));
	return true;
}

#endif // !USE_NEW_SIMPLEX_SOLVER

#if USE_NEW_SIMPLEX_SOLVER

#define KEEP_CLOSEST_POINT 1
#define CHECK_DIV 1
#define MINIMIZE_ROUNDOFF_ERROR 1

#if KEEP_CLOSEST_POINT
#define UPDATE_CLOSEST() \
	closestPoint = SelectFT(isCloser,closestPoint,dist); \
	closestPointDistSq = SelectFT(isCloser,closestPointDistSq,distSq); \
	closestCoefs = SelectFT(isCloser,closestCoefs,coefs);
#else // KEEP_CLOSEST_POINT
#define UPDATE_CLOSEST() \
	closestPointDistSq = SelectFT(isCloser,closestPointDistSq,distSq); \
	closestCoefs = SelectFT(isCloser,closestCoefs,coefs);
#endif // KEEP_CLOSEST_POINT

// Segment Test:
// Segment(c) = newVert + c * vi_
// c_min = -dot(newVert,vi_) / dot(vi_,vi_)
// coef_vi_ = c_min
// coef_newVert = 1 - c_min
// dist_sq = dot(newVert,newVert) + c_min * dot(new_vert,vi_);
#if KEEP_CLOSEST_POINT
#if CHECK_DIV
#if MINIMIZE_ROUNDOFF_ERROR
#define TEST_SEG(i) \
{ \
	const ScalarV den = MagSquared(v##i##_); \
	const ScalarV coef_i = -Dot(v##i##_,newVert) / den; \
	const ScalarV coef_n = v_one - coef_i; \
	const Vec3V dist = newVert + coef_i * v##i##_; \
	const ScalarV distSq = MagSquared(dist); \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV divOk = IsGreaterThan(den,v_divideEps); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = divOk & coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#else // MINIMIZE_ROUNDOFF_ERROR
#define TEST_SEG(i) \
{ \
	const ScalarV coef_i = -dot##i##n / dot##i##i; \
	const ScalarV coef_n = v_one - coef_i; \
	const Vec3V dist = newVert + coef_i * v##i##_; \
	const ScalarV distSq = MagSquared(dist); \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV divOk = IsGreaterThan(dot##i##i,v_divideEps); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = divOk & coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#endif // MINIMIZE_ROUNDOFF_ERROR
#else // CHECK_DIV
#define TEST_SEG(i) \
{ \
	const ScalarV coef_i = -dot##i##n / dot##i##i; \
	const ScalarV coef_n = v_one - coef_i; \
	const Vec3V dist = newVert + coef_i * v##i##_; \
	const ScalarV distSq = MagSquared(dist); \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#endif // CHECK_DIV
#else // KEEP_CLOSEST_POINT
#if CHECK_DIV
#define TEST_SEG(i) \
{ \
	const ScalarV coef_i = -dot##i##n / dot##i##i; \
	const ScalarV coef_n = v_one - coef_i; \
	const ScalarV distSq = dotnn + coef_i * dot##i##n; \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV divOk = IsGreaterThan(dot##i##i,v_divideEps); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = divOk & coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#else // CHECK_DIV
#define TEST_SEG(i) \
{ \
	const ScalarV coef_i = -dot##i##n / dot##i##i; \
	const ScalarV coef_n = v_one - coef_i; \
	const ScalarV distSq = dotnn + coef_i * dot##i##n; \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#endif // CHECK_DIV
#endif // KEEP_CLOSEST_POINT

// Face Test:
// Face(ci,cj) = newVert + ci * vi_ + cj * vj_
// Solve: dot(vi_,Face(ci,cj)) = 0 and dot(vj_,Face(ci,cj)) = 0
// dot(vi,vi) * ci + dot(vi,vj) * cj = -dot(vi,newVert)
// dot(vi,vj) * ci + dot(vj,vj) * cj = -dot(vj,newVert)
// den = dii * djj - dij * dij
// ci = (-din * djj + djn * dij) / den
// cj = (-dii * djn + dij * din) / den
#if KEEP_CLOSEST_POINT
#if CHECK_DIV
#if MINIMIZE_ROUNDOFF_ERROR
#define TEST_FACE(i,j) \
{ \
	const Vec3V cross_ij = cross_##i##j; \
	const ScalarV den = MagSquared(cross_ij); \
	const Vec3V cross_nij = Cross(newVert,cross_ij); \
	const ScalarV coef_i = Dot(cross_nij,v##j##_); \
	const ScalarV coef_j = -Dot(cross_nij,v##i##_); \
	const ScalarV coef_n = den - coef_i - coef_j; \
	const Vec3V dist = numer_##i##j / den * cross_ij; \
	const ScalarV distSq = MagSquared(dist); \
	const Vec4V coefs = ((coef_mask##i & Vec4V(coef_i)) | (coef_mask##j & Vec4V(coef_j)) | (coef_maskn & Vec4V(coef_n))) / den; \
	const BoolV divOk = IsGreaterThan(den,v_divideEps); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_j,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = divOk & coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#else
#define TEST_FACE(i,j) \
{ \
	const ScalarV den = dot##i##i * dot##j##j - dot##i##j * dot##i##j; \
	const ScalarV coef_i = (dot##j##n * dot##i##j - dot##i##n * dot##j##j) / den; \
	const ScalarV coef_j = (dot##i##n * dot##i##j - dot##j##n * dot##i##i) / den; \
	const ScalarV coef_n = v_one - coef_i - coef_j; \
	const Vec3V dist = newVert + coef_i * v##i##_ + coef_j * v##j##_; \
	const ScalarV distSq = MagSquared(dist); \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_mask##j & Vec4V(coef_j)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV divOk = IsGreaterThan(den,v_divideEps); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_j,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = divOk & coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#endif
#else // CHECK_DIV
#define TEST_FACE(i,j) \
{ \
	const ScalarV den = dot##i##i * dot##j##j - dot##i##j * dot##i##j; \
	const ScalarV coef_i = (dot##j##n * dot##i##j - dot##i##n * dot##j##j) / den; \
	const ScalarV coef_j = (dot##i##n * dot##i##j - dot##j##n * dot##i##i) / den; \
	const ScalarV coef_n = v_one - coef_i - coef_j; \
	const Vec3V dist = newVert + coef_i * v##i##_ + coef_j * v##j##_; \
	const ScalarV distSq = MagSquared(dist); \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_mask##j & Vec4V(coef_j)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_j,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#endif // CHECK_DIV
#else // KEEP_CLOSEST_POINT
#if CHECK_DIV
#define TEST_FACE(i,j) \
{ \
	const ScalarV den = dot##i##i * dot##j##j - dot##i##j * dot##i##j; \
	const ScalarV coef_i = (dot##j##n * dot##i##j - dot##i##n * dot##j##j) / den; \
	const ScalarV coef_j = (dot##i##n * dot##i##j - dot##j##n * dot##i##i) / den; \
	const ScalarV coef_n = v_one - coef_i - coef_j; \
	const ScalarV distSq = dotnn + coef_i * dot##i##n + coef_j * dot##j##n; \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_mask##j & Vec4V(coef_j)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV divOk = IsGreaterThan(den,v_divideEps); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_j,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = divOk & coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#else // CHECK_DIV
#define TEST_FACE(i,j) \
{ \
	const ScalarV den = dot##i##i * dot##j##j - dot##i##j * dot##i##j; \
	const ScalarV coef_i = (dot##j##n * dot##i##j - dot##i##n * dot##j##j) / den; \
	const ScalarV coef_j = (dot##i##n * dot##i##j - dot##j##n * dot##i##i) / den; \
	const ScalarV coef_n = v_one - coef_i - coef_j; \
	const ScalarV distSq = dotnn + coef_i * dot##i##n + coef_j * dot##j##n; \
	const Vec4V coefs = (coef_mask##i & Vec4V(coef_i)) | (coef_mask##j & Vec4V(coef_j)) | (coef_maskn & Vec4V(coef_n)); \
	const BoolV coefsTest = IsGreaterThan(coef_i,v_zero) & IsGreaterThan(coef_j,v_zero) & IsGreaterThan(coef_n,v_zero); \
	const BoolV distTest = IsLessThan(distSq,closestPointDistSq); \
	const BoolV isCloser = coefsTest & distTest; \
	UPDATE_CLOSEST() \
}
#endif // CHECK_DIV
#endif // KEEP_CLOSEST_POINT

/*
void test()
{
	int nv;
	Assert(nv >= 2 && nv <= 4);

	TEST_TETRAHEDRON();

	closest = NEW_VERT;

	if (nv >= 2)
	{
		if (seedSimplex)
			TEST_POINT(0);

		TEST_SEG(0);
		if (nv >= 3)
		{
			if (seedSimplex)
			{
				TEST_POINT(1);
				TEST_SEG_(0,1);
			}

			TEST_SEG(1);
			TEST_FACE(0,1);
			if (nv == 4)
			{
				TEST_SEG(2);
				TEST_FACE(0,2);
				TEST_FACE(1,2);
			}
		}
	}
}
*/

void VoronoiSimplexSolver::CopyVertex(const int nv, const int cur, ScalarV_In coef)
{
	m_SimplexSeparation[nv] = m_SimplexSeparation[cur];
	m_VertexIndexA[nv] = m_VertexIndexA[cur];
	m_VertexIndexB[nv] = m_VertexIndexB[cur];
	m_LocalA[nv] = m_LocalA[cur];
	m_LocalB[nv] = m_LocalB[cur];
	m_SimplexCoords[nv] = coef;
}

void VoronoiSimplexSolver::ReduceVertices1(Vec4V_In barycentricCoords)
{
	FastAssert(m_numVertices >= 3 && m_numVertices <= 4);
	const ScalarV coef0 = SplatX(barycentricCoords);
	const ScalarV coef1 = SplatY(barycentricCoords);
	const ScalarV coef2 = SplatZ(barycentricCoords);

	const ScalarV v_zero(V_ZERO);

	int nv = 0;
	if (IsEqualAll(coef0,v_zero) == 0)
	{
		//CopyVertex(nv,0,coef0);
		m_SimplexCoords[0] = coef0;
		nv++;
	}
	if (IsEqualAll(coef1,v_zero) == 0)
	{
		CopyVertex(nv,1,coef1);
		nv++;
	}
	if (IsEqualAll(coef2,v_zero) == 0)
	{
		CopyVertex(nv,2,coef2);
		nv++;
	}
	m_numVertices = nv;
}

__forceinline ScalarV_Out And(ScalarV_In v, BoolV_In b)
{
	return ScalarV((Vec4V(v.GetIntrin128ConstRef()) & Vec4V(b.GetIntrin128ConstRef())).GetIntrin128ConstRef());
}

PHYSICS_FORCE_INLINE int VoronoiSimplexSolver::ClosestPtPointTriangle1(ScalarV_In currentSquaredDistance, const bool seedSimplex)
{
	const Vec3V vlist0 = m_SimplexSeparation[0];
	const Vec3V vlist1 = m_SimplexSeparation[1];
	const Vec3V newVert = m_SimplexSeparation[2];
	const Vec3V v0_ = vlist0 - newVert;
	const Vec3V v1_ = vlist1 - newVert;

	const ScalarV v_zero(V_ZERO);
	const ScalarV v_one(V_ONE);
	const ScalarV v_divideEps(V_FLT_EPSILON);

	const Vec4V coef_mask0 = Vec4V(V_T_F_F_F);
	const Vec4V coef_mask1 = Vec4V(V_F_T_F_F);
	const Vec4V coef_maskn = Vec4V(V_F_F_T_F);

	const ScalarV dotnn = MagSquared(newVert);

#if KEEP_CLOSEST_POINT
	Vec3V closestPoint = newVert;
#endif
	ScalarV closestPointDistSq = dotnn;
	Vec4V closestCoefs = coef_maskn & Vec4V(v_one);

	if (seedSimplex)
	{
		// Clamped Segment 0-1
		const Vec3V dir = vlist1 - vlist0;
		// Dot(vertList[0] + c_ * dir,dir) = 0
		// c_ = -Dot(vertList[0],dir) / Dot(dir,dir)
		const ScalarV ndirSq = MagSquared(dir);
		const ScalarV dot0d = Dot(vlist0,dir);
		const BoolV divOk = IsGreaterThan(ndirSq,v_divideEps); 
		const ScalarV ratio = And(-dot0d/ndirSq,divOk);
		const ScalarV coef1 = Clamp(ratio,v_zero,v_one);

//		const ScalarV ratio = -dot0d / ndirSq;
//		const BoolV divOk = IsGreaterThan(ndirSq,v_divideEps); 
//		const BoolV gtZero = IsGreaterThan(ratio,v_zero);
//		const ScalarV ratio1 = And(ratio,gtZero & divOk);
//		const ScalarV coef1 = Min(ratio1,v_one);

//		const BoolV gt = IsLessThan(dot0d,v_zero);
//		const ScalarV cf1 = And(-dot0d,gt);
//		const BoolV gte1 = IsGreaterThanOrEqual(cf1,ndirSq);
//		const ScalarV coef1 = SelectFT(gte1,cf1/ndirSq,v_one);
		
		const ScalarV coef0 = v_one - coef1;
		const Vec3V dist = vlist0 + coef1 * dir;
		const ScalarV distSq = MagSquared(dist);
		const Vec4V coefs = (coef_mask0 & Vec4V(coef0)) | (coef_mask1 & Vec4V(coef1));
		const BoolV isCloser = IsLessThan(distSq,closestPointDistSq);
		UPDATE_CLOSEST();
	}

#if !MINIMIZE_ROUNDOFF_ERROR
	const ScalarV dot00 = MagSquared(v0_);
	const ScalarV dot11 = MagSquared(v1_);
	const ScalarV dot01 = Dot(v0_,v1_);
	//const ScalarV dot10 = dot01;
	const ScalarV dot0n = Dot(v0_,newVert);
	const ScalarV dot1n = Dot(v1_,newVert);
#else // !MINIMIZE_ROUNDOFF_ERROR
	const Vec3V cross_01 = Cross(v0_,v1_);
	const ScalarV numer_01 = Dot(cross_01,newVert);
#endif // !MINIMIZE_ROUNDOFF_ERROR

	TEST_SEG(0);
	TEST_SEG(1);
	TEST_FACE(0,1);

	FastAssert(IsGreaterThanOrEqualAll(closestCoefs,Vec4V(v_zero)));
	if (IsLessThanAll(closestPointDistSq,currentSquaredDistance))
	{
#if KEEP_CLOSEST_POINT
		m_cachedV = closestPoint;
#else // KEEP_CLOSEST_POINT
		m_cachedV = newVert + closestCoefs.GetX() * v0_ + closestCoefs.GetY() * v1_;
#endif // KEEP_CLOSEST_POINT

		// Reduce the vertices.
		ReduceVertices1(closestCoefs);
		FastAssert(m_numVertices > 0 && m_numVertices <= 3);

		return VALID;
	}
	else
	{
		m_numVertices--;
		return KEEP;
	}
}

PHYSICS_FORCE_INLINE int VoronoiSimplexSolver::ClosestPtPointTetrahedron1(ScalarV_In currentSquaredDistance, const bool isSeparated)
{
	const Vec3V newVert = m_SimplexSeparation[3];
	const Vec3V v0_ = m_SimplexSeparation[0] - newVert;
	const Vec3V v1_ = m_SimplexSeparation[1] - newVert;
	const Vec3V v2_ = m_SimplexSeparation[2] - newVert;

	const ScalarV v_zero(V_ZERO);
	const ScalarV v_one(V_ONE);
	const ScalarV v_divideEps(V_FLT_EPSILON);

	// Tetrahedron Test:
	// Solve Tetra(ci,cj,ck) = newVert + ci * vi_ + cj * vj_ + ck * vk_ == 0;
#if MINIMIZE_ROUNDOFF_ERROR
	const Vec3V cross_01 = Cross(v0_,v1_);
	const Vec3V cross_02 = Cross(v0_,v2_);
	const Vec3V cross_12 = Cross(v1_,v2_);
	const ScalarV numer_01 = Dot(cross_01,newVert);
	const ScalarV numer_02 = Dot(cross_02,newVert);
	const ScalarV numer_12 = Dot(cross_12,newVert);
#endif // MINIMIZE_ROUNDOFF_ERROR
	if (!isSeparated)
	{
/*
		const ScalarV den = Dot(Cross(v0_,v1_),v2_);
		const ScalarV c0 = Dot(Cross(-newVert,v1_),v2_) / den;
		const ScalarV c1 = Dot(Cross(v0_,-newVert),v2_) / den;
		const ScalarV c2 = Dot(Cross(v0_,v1_),-newVert) / den;
		const ScalarV c3 = 1 - c0 - c1 - c2;
*/
#if MINIMIZE_ROUNDOFF_ERROR
		const ScalarV den = Dot(cross_01,v2_);
		const ScalarV c0 = -numer_12 / den;
		const ScalarV c1 = +numer_02 / den;
		const ScalarV c2 = -numer_01 / den;
#else // MINIMIZE_ROUNDOFF_ERROR
		const Vec3V cr01 = Cross(v0_,v1_);
		const ScalarV den = Dot(cr01,v2_);
		const Vec3V crn2 = Cross(newVert,v2_);
		const ScalarV c0 = Dot(crn2,v1_) / den;
		const ScalarV c1 = -Dot(crn2,v0_) / den;
		const ScalarV c2 = -Dot(cr01,newVert) / den;
#endif // MINIMIZE_ROUNDOFF_ERROR
		const ScalarV c3 = v_one - c0 - c1 - c2;
		//const ScalarV v_divideEps5(V_FLT_EPSILON);
		const BoolV divOk = IsGreaterThan(Abs(den),v_divideEps);
		const BoolV coefsTest = divOk & IsGreaterThanOrEqual(c0,v_zero) & IsGreaterThanOrEqual(c1,v_zero) & IsGreaterThanOrEqual(c2,v_zero) & IsGreaterThanOrEqual(c3,v_zero);
		const bool coefsTestb = coefsTest.Getb();
		if (coefsTestb)
		{
			m_cachedV = Vec3V(v_zero);
			return PENETRATING;
		}
	}

#if !MINIMIZE_ROUNDOFF_ERROR
	const ScalarV dot00 = MagSquared(v0_);
	const ScalarV dot11 = MagSquared(v1_);
	const ScalarV dot22 = MagSquared(v2_);
	const ScalarV dot01 = Dot(v0_,v1_);
	//const ScalarV dot10 = dot01;
	const ScalarV dot02 = Dot(v0_,v2_);
	//const ScalarV dot20 = dot02;
	const ScalarV dot12 = Dot(v1_,v2_);
	//const ScalarV dot21 = dot12;
	const ScalarV dot0n = Dot(v0_,newVert);
	const ScalarV dot1n = Dot(v1_,newVert);
	const ScalarV dot2n = Dot(v2_,newVert);
#endif // !MINIMIZE_ROUNDOFF_ERROR
	const ScalarV dotnn = MagSquared(newVert);

	const Vec4V coef_mask0 = Vec4V(V_T_F_F_F);
	const Vec4V coef_mask1 = Vec4V(V_F_T_F_F);
	const Vec4V coef_mask2 = Vec4V(V_F_F_T_F);
	const Vec4V coef_maskn = Vec4V(V_F_F_F_T);

#if KEEP_CLOSEST_POINT
	Vec3V closestPoint = newVert;
#endif
	ScalarV closestPointDistSq = dotnn;
	Vec4V closestCoefs = coef_maskn & Vec4V(v_one);

	TEST_SEG(0);
	TEST_SEG(1);
	TEST_SEG(2);

	TEST_FACE(0,1);
	TEST_FACE(0,2);
	TEST_FACE(1,2);

	FastAssert(IsGreaterThanOrEqualAll(closestCoefs,Vec4V(v_zero)));
	if (IsLessThanAll(closestPointDistSq,currentSquaredDistance))
	{
#if KEEP_CLOSEST_POINT
		m_cachedV = closestPoint;
#else // KEEP_CLOSEST_POINT
		m_cachedV = newVert + closestCoefs.GetX() * v0_ + closestCoefs.GetY() * v1_ + closestCoefs.GetZ() * v2_;
#endif // KEEP_CLOSEST_POINT

		// Reduce the vertices.
		ReduceVertices1(closestCoefs);
		const ScalarV coef3 = SplatW(closestCoefs);
		FastAssert(IsGreaterThanAll(coef3,v_zero));		// The 4'th vertex coefficient should always be strictly greater than zero.
		CopyVertex(m_numVertices,3,coef3);
		m_numVertices++;
		FastAssert(m_numVertices > 0 && m_numVertices <= 3);

		return VALID;
	}
	else
	{
		m_numVertices--;
		return KEEP;
	}
}

int VoronoiSimplexSolver::UpdateClosestVectorAndPoints(ScalarV_In currentSquaredDistance, const bool isSeparated, const bool seedSimplex)
{
	FastAssert(m_needsUpdate);
	FastAssert(m_numVertices >= 1 && m_numVertices <= 4);
	m_needsUpdate = false;
	switch(m_numVertices)
	{
	case 1:
		{
			m_SimplexCoords[0] = ScalarV(V_ONE);
			m_cachedV = m_SimplexSeparation[0];
			return VALID;
		}

	case 2:
		{
			const ScalarV v_zero(V_ZERO);
			const ScalarV v_one(V_ONE);
			const ScalarV v_divideEps(V_FLT_EPSILON);
			const Vec3V v0_ = m_SimplexSeparation[0];
			const Vec3V v1_ = m_SimplexSeparation[1];
			const Vec3V dir = v1_ - v0_;
			const ScalarV ndirSq = MagSquared(dir);
			const ScalarV dot0d = Dot(v0_,dir);
			const BoolV divOk = IsGreaterThan(ndirSq,v_divideEps); 
			const ScalarV ratio = And(-dot0d/ndirSq,divOk);
			const ScalarV coef1 = Clamp(ratio,v_zero,v_one);
			const ScalarV coef0 = v_one - coef1;
			const Vec3V dist = v0_ + coef1 * dir;
			const ScalarV distSq = MagSquared(dist);
			if (IsLessThanAll(distSq,currentSquaredDistance))
			{
				m_cachedV = dist;

				// Reduce the vertices.
				int nv = 0;
				if (IsEqualAll(coef0,v_zero) == 0)
				{
					//CopyVertex(nv,0,coef0);
					m_SimplexCoords[0] = coef0;
					nv++;
				}
				if (IsEqualAll(coef1,v_zero) == 0)
				{
					CopyVertex(nv,1,coef1);
					nv++;
				}
				m_numVertices = nv;

				return VALID;
			}
			else
			{
				m_numVertices--;
				return KEEP;
			}
		}

	case 3:
		{
			return ClosestPtPointTriangle1(currentSquaredDistance,seedSimplex);
		}

	default:
		{
			return ClosestPtPointTetrahedron1(currentSquaredDistance,isSeparated);
		}
	}
}

#endif // USE_NEW_SIMPLEX_SOLVER

int VoronoiSimplexSolver::RemoveDegenerateIndices (const int* inArray, int numIndices, int* outArray) const
{
	int outIndex = 0;
	for (int firstIndex=0; firstIndex<numIndices; firstIndex++)
	{
		bool duplicate = false;
		for (int secondIndex=0; secondIndex<firstIndex; secondIndex++)
		{
			if (inArray[secondIndex]==inArray[firstIndex])
			{
				duplicate = true;
				break;
			}
		}

		if (!duplicate)
		{
			outArray[outIndex++] = inArray[firstIndex];
		}
	}

	return outIndex;
}


int VoronoiSimplexSolver::FindElementIndexP (Vec::V3Param128 localNormalP, Vec::V3Param128 localContactP, const rage::phBound* shapeP) const
{
	if (shapeP->GetType() == phBound::GEOMETRY)
	{
		int vertexIndices[4];
		int numUniqueVertices = RemoveDegenerateIndices(m_VertexIndexA,m_numVertices,vertexIndices);
		return shapeP->FindElementIndex(localNormalP,localContactP,vertexIndices,numUniqueVertices);
	}
	else
	{
		return shapeP->GetIndexFromBound();
	}
}


int VoronoiSimplexSolver::FindElementIndexQ (Vec::V3Param128 localNormalQ, Vec::V3Param128 localContactQ, const rage::phBound* shapeQ) const
{
	if (shapeQ->GetType() == phBound::GEOMETRY)
	{
		int vertexIndices[4];
		int numUniqueVertices = RemoveDegenerateIndices(m_VertexIndexB,m_numVertices,vertexIndices);
		return shapeQ->FindElementIndex(localNormalQ,localContactQ,vertexIndices,numUniqueVertices);
	}
	else
	{
		return shapeQ->GetIndexFromBound();
	}
}

} // namespace rage
