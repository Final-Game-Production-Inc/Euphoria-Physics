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
*/



#ifndef VoronoiSimplexSolver_H
#define VoronoiSimplexSolver_H

#include "phcore/constants.h"

#include "SimplexSolverInterface.h"

#include "vector/vector4.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

namespace rage {


class phBound;

#define VORONOI_SIMPLEX_MAX_VERTS 4

/// VoronoiSimplexSolver is an implementation of the closest point distance algorithm from a 1-4 points simplex to the origin.
/// Can be used with GJK, as an alternative to Johnson distance algorithm.
#ifdef NO_VIRTUAL_INTERFACE
class VoronoiSimplexSolver
#else
class VoronoiSimplexSolver : public SimplexSolverInterface
#endif
{
	struct	SubSimplexClosestResult
	{
		Vec3V	m_closestPointOnSimplex;
		//MASK for m_usedVertices
		//stores the simplex vertex-usage, using the MASK, 
		// if m_usedVertices & MASK then the related vertex is used
		Vec4V	m_usedVertices;
		Vec4V	m_barycentricCoords;
		bool	m_degenerate;

		void Reset()
		{
			m_degenerate = false;
			m_barycentricCoords = m_usedVertices = Vec4V(V_ZERO);
		}

		bool IsValid()
		{
			return ( IsGreaterThanOrEqualAll( m_barycentricCoords, Vec4V(V_ZERO) ) != 0 );
		}
	};

public:

    Vec3V m_SimplexSeparation[VORONOI_SIMPLEX_MAX_VERTS];

	Vec3V m_LocalA[VORONOI_SIMPLEX_MAX_VERTS];
	Vec3V m_LocalB[VORONOI_SIMPLEX_MAX_VERTS];

	Vec3V m_cachedV;

	ScalarV m_SimplexCoords[VORONOI_SIMPLEX_MAX_VERTS];

	//Vec::Vector_4V m_VertexIndexA_4V[VORONOI_SIMPLEX_MAX_VERTS];
	//Vec::Vector_4V m_VertexIndexB_4V[VORONOI_SIMPLEX_MAX_VERTS];

	bool m_needsUpdate;

	int m_VertexIndexA[VORONOI_SIMPLEX_MAX_VERTS];
	int m_VertexIndexB[VORONOI_SIMPLEX_MAX_VERTS];

	int	m_numVertices;

	bool m_hasCache;
	bool m_isSeparated;

	VoronoiSimplexSolver()
	{
		m_hasCache = false;
		m_isSeparated = false;
	}
	
#if USE_NEW_SIMPLEX_SOLVER

	void CopyVertex(const int nv, const int cur, ScalarV_In coef);
	void ReduceVertices1(Vec4V_In barycentricCoords);
	int	UpdateClosestVectorAndPoints(ScalarV_In currentSquaredDistance, const bool isSeparated, const bool seedSimplex);
	enum
	{
		KEEP = 0,
		VALID = 1,
		PENETRATING = 2,
	};
	int	ClosestPtPointTetrahedron1(ScalarV_In currentSquaredDistance, const bool isSeparated);
	int ClosestPtPointTriangle1(ScalarV_In currentSquaredDistance, const bool seedSimplex);

#else // USE_NEW_SIMPLEX_SOLVER

	// PURPOSE: Remove the vertex with the specified index from the simplex.
	// PARAMS:
	//	index - the index number of the vertex to remove from the simplex.
	void RemoveVertex (int index);

	// PURPOSE: Remove all unused vertices from the simplex.
	// PARAMS:
	//	usedVerts - list of vertices that have been used (non-zero elements indicate used vertices)
	void ReduceVertices (rage::Vec4V_In usedVerts);
	bool	UpdateClosestVectorAndPoints();

	bool	ClosestPtPointTetrahedron(rage::Vec::V3Param128 a, rage::Vec::V3Param128 b, rage::Vec::V3Param128_After3Args c, rage::Vec::V3Param128_After3Args d, SubSimplexClosestResult& finalResult);
	int		PointOutsideOfPlane(rage::Vec::V3Param128 a, rage::Vec::V3Param128 b, rage::Vec::V3Param128_After3Args c, rage::Vec::V3Param128_After3Args d);
	bool	ClosestPtPointTriangle(rage::Vec::V3Param128 a, rage::Vec::V3Param128 b, rage::Vec::V3Param128_After3Args c,SubSimplexClosestResult& result);

#endif // USE_NEW_SIMPLEX_SOLVER

	int RemoveDegenerateIndices (const int* inArray, int numIndices, int* outArray) const;
	int FindElementIndexP (Vec::V3Param128 localNormalP, Vec::V3Param128 localContactP, const rage::phBound* shapeP) const;
	int FindElementIndexQ (Vec::V3Param128 localNormalQ, Vec::V3Param128 localContactQ, const rage::phBound* shapeQ) const;

public:

	__forceinline int IsSeparated() const
	{
		return m_isSeparated;
	}

	__forceinline int IsSimplexValid() const
	{
		return (m_numVertices > 0 && m_numVertices <= 3);
	}

	__forceinline void SetCachedV(Vec3V_In cachedV)
	{
		m_cachedV = cachedV;
	}

	//clear the simplex, remove all the vertices
	__forceinline void ResetCache(int numVertices)
	{
		FastAssert(numVertices >= 0 && numVertices <= 3);
		m_numVertices = numVertices;
		m_needsUpdate = (numVertices > 0);
	}

	//void AddVertex(const Vec3V & worldPointBtoA, int vertexIndexA, int vertexIndexB, const Vec3V & LocA, const Vec3V & LocB)
	//void AddVertex(const Vec3V & worldPointBtoA, Vec::Vector_4V_In vertexIndexA_4V, Vec::Vector_4V_In vertexIndexB_4V, const Vec3V & LocA, const Vec3V & LocB)
	void AddVertex(const Vec3V & worldPointBtoA, const Vec3V & LocA, const Vec3V & LocB)
	{
		FastAssert(m_numVertices < VORONOI_SIMPLEX_MAX_VERTS);

		m_needsUpdate = true;

		m_SimplexSeparation[m_numVertices] = worldPointBtoA;

		//m_VertexIndexA[m_numVertices] = vertexIndexA;
		//m_VertexIndexB[m_numVertices] = vertexIndexB;
		//m_VertexIndexA_4V[m_numVertices] = vertexIndexA_4V;
		//m_VertexIndexB_4V[m_numVertices] = vertexIndexB_4V;

		m_LocalA[m_numVertices] = LocA;
		m_LocalB[m_numVertices] = LocB;

		m_numVertices++;
	}

	 bool fullSimplex() const
	 {
		 return (m_numVertices == 4);
	 }

	// PURPOSE: See if the given point is already in the collection of collision points.
	// PARAMS:
	//	point - the collision point to try to find in the current set
	// NOTES:	This used to compare the points exactly, which worked when the duplicated points were vertex locations. With curved geometry bounds,
	//			roundoff errors result in slight differences, so a tolerance is used to avoid extremely skinny simplex shapes.
	bool inSimplex (Vec::V3Param128 point);

	//void backup_closest(Vec3V_InOut v) ;

	//bool emptySimplex() const ;


	 int numVertices() const 
	 {
		 return m_numVertices;
	 }
};

__forceinline bool VoronoiSimplexSolver::inSimplex (Vec::V3Param128 point)
{

	Vec3V _point(point);

	const ScalarV one_over_one_thousand(V_ONE_OVER_1024);
	const ScalarV one_over_one_million( one_over_one_thousand * one_over_one_thousand );

	// Find the largest squared distance between existing simplex point pairs, with a minimum of 1, to avoid skinny simplex shapes.
	ScalarV largestSeparation2V = ScalarV(V_ONE);
	ScalarV separation2V;

	// Also find the distance of the new point from simplex points
	ScalarV minDistance2V = ScalarV(V_FLT_MAX);
	ScalarV distance2V;

	switch (m_numVertices)
	{
	//case 3:
	default:
		separation2V = MagSquared( m_SimplexSeparation[2] );
		largestSeparation2V = Max( largestSeparation2V, separation2V );
		distance2V = DistSquared( m_SimplexSeparation[2], _point );
		minDistance2V = Min( minDistance2V, distance2V);
	case 2:
		separation2V = MagSquared( m_SimplexSeparation[1] );
		largestSeparation2V = Max( largestSeparation2V, separation2V );
		distance2V = DistSquared( m_SimplexSeparation[1], _point );
		minDistance2V = Min( minDistance2V, distance2V);
	case 1:
		separation2V = MagSquared( m_SimplexSeparation[0] );
		largestSeparation2V = Max( largestSeparation2V, separation2V );
		distance2V = DistSquared( m_SimplexSeparation[0], _point );
		minDistance2V = Min( minDistance2V, distance2V);
	case 0:
		; // No vertices to process
	}

	// Set the closeness tolerance to 1/1000th of the largest squared distance, or 1mm, whichever is larger.
	const ScalarV close = Scale( one_over_one_million, largestSeparation2V );

	// Return true if the given point is close to one already in the simplex, false if not.
	return IsLessThanAll(minDistance2V, close) != 0;
}


} // namespace rage

#endif // VoronoiSimplexSolver_H
