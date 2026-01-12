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

#ifndef PHBULLET_VORONOISINGLESIMPLEXSOLVER_H
#define PHBULLET_VORONOISINGLESIMPLEXSOLVER_H

#include "math/amath.h"

#include "vectormath/classes.h"

namespace rage {

#define VORONOI_SINGLE_SIMPLEX_MAX_VERTS 4

	// This is a simplex solver just like VoronoiDegenerateSimplexSolver, the only real difference is that this only handles
	//   a single object. This means that it gives more accurate results because it doesn't have to use barycentric coordinates
	//   to get points on the object. It is also faster since it doesn't have to track as many vertices.
	class VoronoiSingleSimplexSolver
	{
	private:

		Vec3V m_Simplex[VORONOI_SINGLE_SIMPLEX_MAX_VERTS];
		u32	m_NumVertices;

		void ComputeClosestPointAndNormalOnPoint(Vec3V_In point, Vec3V_In vertex0, Vec3V_InOut closestPointOnPoint, Vec3V_InOut normalOnPoint);
		void ComputeClosestPointAndNormalOnSegment(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_InOut closestPointOnSegment, Vec3V_InOut normalOnSegment);
		void ComputeClosestPointAndNormalOnTriangle(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_InOut closestPointOnTriangle, Vec3V_InOut normalOnTriangle);
		void ComputeClosestPointAndNormalOnTetrahedron(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_In vertex3, Vec3V_InOut closestPointOnTetrahedron, Vec3V_InOut normalOnTetrahedron);
	public:

		__forceinline VoronoiSingleSimplexSolver()
		{
			Reset();
		}

		// PURPOSE: Reset the data in the simplex solver
		__forceinline void Reset()
		{
			m_NumVertices = 0;
		}

		__forceinline int GetNumVertices() const
		{
			return m_NumVertices;
		}
		
		// PURPOSE:
		//   Add a new vertex to the simplex.
		// PARAMS:
		//   vertex - the vertex to add
		//   simplexNormal - the last normal on the simplex
		// NOTES:
		//   You are not allowed to add a vertex when there are already four.
		//   For the first vertex, simplexNormal doesn't matter. 
		__forceinline void AddVertex(Vec3V_In vertex, Vec3V_In simplexNormal)
		{
			Assertf(m_NumVertices < VORONOI_SINGLE_SIMPLEX_MAX_VERTS, "Attempting to add vertex to full simplex");

			int numVertices = m_NumVertices;

			// Overwrite the next vertex in the simplex, even if we don't increase the number of vertices. If we're going outside
			//   the bounds of the array, the above assert will fire. 
			m_Simplex[numVertices] = vertex;

			// The added vertex should be the supporting point of the last normal on the simplex. If that vertex is not further 
			//   along the normal, then it can't give a better closest point the next time ComputeClosestPointAndNormalOnSimplex 
			//   is called. It can however make the simplex degenerate, so we're better off not adding it.
			BoolV isNotDegenerate = IsGreaterThan(Dot(Subtract(vertex,m_Simplex[0]), simplexNormal), ScalarV(V_FLT_SMALL_3));
			u32 isNotDegenerateMask = *((u32*)&isNotDegenerate);

			u32 isFirstVertexMask = ~GenerateMaskGZ(numVertices);

			// Increase the number of vertices in the simplex to include the added one if it won't make the simplex degenerate, 
			//   or if this is the first vertex to be added. 
			numVertices += (isNotDegenerateMask | isFirstVertexMask) & 1;

			m_NumVertices = numVertices;

			Assertf(m_NumVertices > 0, "VoronoiSingleSimplexSolver::AddVertex failed to add the first vertex.");
		}

		// PURPOSE:
		//   Find the closest point on the simplex to the given point, and the normal on the simplex at the closest point. 
		// PARAMS:
		//   point - the point to find the closest point on the simplex of.
		//   closestPointOnSimplex - out parameter that is filled with the closest point on the simplex to point
		//   normalOnSimplex - out parameter that is filled with the normal at the closest point.
		// NOTES:
		//   If the point is touching the simplex, normalOnSimplex is undefined. You know this is happening if point ~= closestPointOnSimplex. 
		void ComputeClosestPointAndNormalOnSimplex(Vec3V_In point, Vec3V_InOut closestPointOnSimplex, Vec3V_InOut normalOnSimplex);
	};

} // namespace rage

#endif // SIMPLESIMPLEXSOLVER_H
