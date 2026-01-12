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

#ifndef PHBULLET_VORONOIDEGENERATESIMPLEXSOLVER_H
#define PHBULLET_VORONOIDEGENERATESIMPLEXSOLVER_H

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

namespace rage {

#define VORONOI_DEGENERATE_SIMPLEX_MAX_VERTS 4

	/// VoronoiDegenerateSimplexSolver is an implementation of the closest point distance algorithm from a 1-4 points simplex to a given point.
	//    It is designed to handle degenerate simplices by throwing out bad vertices as they're added.
	class VoronoiDegenerateSimplexSolver
	{
	private:
		struct	SubSimplexClosestResult
		{
			Vec3V m_ClosestPointOnSimplex;
			VecBoolV m_UsedVertices;
			Vec4V m_BarycentricCoords;

			void Reset()
			{
				m_BarycentricCoords = Vec4V(V_ZERO);
				m_UsedVertices = VecBoolV(V_FALSE);
			}

			bool IsValid()
			{
				return ( IsGreaterThanOrEqualAll( m_BarycentricCoords, Vec4V(V_ZERO) ) != 0 );
			}
		};

		Vec3V m_SimplexSeparation[VORONOI_DEGENERATE_SIMPLEX_MAX_VERTS];
		Vec3V m_SimplexPointOnA[VORONOI_DEGENERATE_SIMPLEX_MAX_VERTS];
		Vec3V m_SimplexPointOnB[VORONOI_DEGENERATE_SIMPLEX_MAX_VERTS];

		Vec3V m_CachedV;
		Vec3V m_CachedNormal;
		ScalarV m_CachedDistanceAlongNormal;

		SubSimplexClosestResult m_CachedResult;

		int	m_NumVertices;

		// PURPOSE: Remove the vertex with the specified index from the simplex.
		// PARAMS:
		//	index - the index number of the vertex to remove from the simplex.
		void RemoveVertex (int index);

		// PURPOSE: Remove all unused vertices from the simplex.
		// PARAMS:
		//	usedVerts - list of vertices that have been used (non-zero elements indicate used vertices)
		void ReduceVertices (VecBoolV_In usedVerts);

		// PURPOSE: Find the closest point on a tetrahedron to a given point
		// PARAMS:
		//  p - the result will be the closest point on the tetrahedron to this point
		//  a - first point on the tetrahedron
		//  b - second point on the tetrahedron
		//  c - third point on the tetrahedron
		//  d - fourth point on the tetrahedron
		//  finalResult - structure to put result information into
		// RETURN: true if the point is outside of the tetrahedron, false otherwise
		// NOTE: If this returns false, finalResult will not be modified
		bool ClosestPtPointTetrahedron(Vec3V_In p, Vec3V_In a, Vec3V_In b, Vec3V_In c, Vec3V_In d, SubSimplexClosestResult& finalResult);

		// PURPOSE: Determine if a point is in front of a plane
		// PARAMS:
		//  p - the function will determine which side of the plane this point is on
		//  a - a point in the plane
		//  b - a second point in the plane
		//  c - a third point in the plane
		//  d - a point that is known to be behind the plane
		// RETURN: 1 if the point is outside of the plane, 0 if it is inside
		int PointOutsideOfPlane(Vec3V_In p, Vec3V_In a, Vec3V_In b, Vec3V_In c, Vec3V_In d);

		// PURPOSE: Find the closest point on a triangle to a given point
		// PARAMS:
		//  p - the result will be the closest point on the triangle to this point
		//  a - the first point on the triangle
		//  b - the second point on the triangle
		//  c - the third point on the triangle
		//  result - structure to put result information into
		// NOTE: The winding of the triangle doesn't matter for this.
		void ClosestPtPointTriangle(Vec3V_In p, Vec3V_In a, Vec3V_In b, Vec3V_In c, SubSimplexClosestResult& result);

	public:

		VoronoiDegenerateSimplexSolver()
		{
			Reset();
		}

		// PURPOSE: Reset the data in the simplex solver
		void Reset()
		{
			m_NumVertices = 0;

			m_CachedNormal = Vec3V(V_ZERO);
			m_CachedDistanceAlongNormal = ScalarV(V_NEG_FLT_MAX);
			m_CachedResult.Reset();

			// Zero these values so we can safely assume that multiplying by zero gives zero. This property isn't true for QNANs and
			//   we rely on it in VoronoiDegenerateSimplexSolver::GetClosestPoints
			sysMemZeroBytes<sizeof(m_SimplexPointOnA)>(m_SimplexPointOnA);
			sysMemZeroBytes<sizeof(m_SimplexPointOnB)>(m_SimplexPointOnB);
			sysMemZeroBytes<sizeof(m_SimplexSeparation)>(m_SimplexSeparation);
		}

		// PURPOSE: Find the closest point on the simplex to the given point, and reduce the simplex to be as small as
		//          possible while containing the closest point to the given poitn.
		// PARAMS:
		//  p - the cached information will store information about the closest point on the simplex to this point
		void UpdateClosestVectorAndPoints(Vec3V_In p);

		// PURPOSE: Add a vertex to the simplex
		// PARAMS:
		//  worldPointBtoA - the vector from worldPointOnB to worldPointOnA, this is the actual point in the simplex
		//  worldPointOnA - the vertex on object A that makes up the simplex point
		//  worldPointOnB - the vertex on object B that makes up the simplex point
		// NOTE:
		//  If this vertex does not move the simplex closer to the last point passed in to UpdateClosestVectorAndPoints it 
		//  will be thrown out to prevent a degenerate simplex from forming. 
		void AddVertex (Vec3V_In worldPointBtoA, Vec3V_In worldPointOnA, Vec3V_In worldPointOnB);

		// PURPOSE: Fill the given buffers with the simplex vertices
		// PARAMS:
		//  pBuf - this buffer will be filled with the simplex separation vertices
		//  qBuf - this buffer will be filled with vertices from object A
		//  yBuf - this buffer wil lbe filled with vertices from object B
		// RETURN: number of vertices that were written to each buffer (number of vertices in the simplex)
		int GetSimplex(Vec3V_Ptr pBuf, Vec3V_Ptr qBuf, Vec3V_Ptr yBuf) const;

		// PURPOSE: See if the given point is already in the collection of collision points.
		// PARAMS:
		//	point - the collision point to try to find in the current set
		// NOTES:	This used to compare the points exactly, which worked when the duplicated points were vertex locations. With curved geometry bounds,
		//			roundoff errors result in slight differences, so a tolerance is used to avoid extremely skinny simplex shapes.
		bool IsPointInSimplex (Vec::V3Param128 point);

		void GetClosestPoints(Vec3V_InOut worldPointOnA, Vec3V_InOut worldPointOnB) ;

		template <typename MinkowskiPointGenerator>
		__forceinline void UpdateMinkowskiPoints(const MinkowskiPointGenerator& generator)
		{
			m_SimplexSeparation[0] = generator(m_SimplexPointOnA[0],m_SimplexPointOnB[0]);
			m_SimplexSeparation[1] = generator(m_SimplexPointOnA[1],m_SimplexPointOnB[1]);
			m_SimplexSeparation[2] = generator(m_SimplexPointOnA[2],m_SimplexPointOnB[2]);
			m_SimplexSeparation[3] = generator(m_SimplexPointOnA[3],m_SimplexPointOnB[3]);
		}

		__forceinline bool IsSimplexFull() const
		{
			return (GetNumVertices() == 4);
		}

		__forceinline bool IsSimplexEmpty() const 
		{
			return (GetNumVertices() == 0);
		}

		__forceinline int GetNumVertices() const 
		{
			return m_NumVertices;
		}

		__forceinline Vec3V_Out GetCachedVector() const
		{
			return m_CachedV;
		}
		__forceinline ScalarV_Out GetCachedDistanceSquared() const
		{
			return MagSquared(m_CachedV);
		}

		__forceinline  Vec3V_Out GetCachedNormal() const
		{
			return m_CachedNormal;
		}
	};

	__forceinline bool VoronoiDegenerateSimplexSolver::IsPointInSimplex (Vec::V3Param128 point)
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

		switch (GetNumVertices())
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

#endif // VORONOIDEGENERATESIMPLEXSOLVER_H
