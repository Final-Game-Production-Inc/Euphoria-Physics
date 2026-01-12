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

#include "VoronoiSingleSimplexSolver.h"

namespace rage
{

__forceinline void VoronoiSingleSimplexSolver::ComputeClosestPointAndNormalOnPoint(Vec3V_In point, Vec3V_In vertex0, Vec3V_InOut closestPointOnPoint, Vec3V_InOut normalOnPoint)
{
	closestPointOnPoint = vertex0;
	normalOnPoint = Subtract(point,vertex0);
}

__forceinline void VoronoiSingleSimplexSolver::ComputeClosestPointAndNormalOnSegment(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_InOut closestPointOnSegment, Vec3V_InOut normalOnSegment)
{
	// Project the point onto the segment
	Vec3V segment01 = Subtract(vertex1,vertex0);
	Vec3V segment0P = Subtract(point,vertex0);
	ScalarV fractionAlongSegment = Scale(Dot(segment0P,segment01),InvMagSquared(segment01));

	// Get the clamped and projected point
	BoolV fractionGreaterThanOne = IsGreaterThan(fractionAlongSegment,ScalarV(V_ONE));
	BoolV fractionLessThanZero = IsLessThan(fractionAlongSegment,ScalarV(V_ZERO));
	closestPointOnSegment = SelectFT(fractionGreaterThanOne, SelectFT(fractionLessThanZero, AddScaled(vertex0,segment01,fractionAlongSegment), vertex0), vertex1);

	BoolV isClosestPointOnVertex = Or(fractionGreaterThanOne, fractionLessThanZero);

	// If the closest point is between the two vertices, we're going to keep the whole segment. To get the most accurate normal, compute through cross products. 
	// If the closest point is a vertex, just use the vector from the vertex to the point. 
	normalOnSegment = SelectFT(isClosestPointOnVertex, Cross(Cross(segment01,segment0P),segment01), Subtract(point,closestPointOnSegment));

	// If a vertex wasn't used, remove it from the simplex
	m_NumVertices = ISelectI(*((u32*)&isClosestPointOnVertex),m_NumVertices,m_NumVertices-1);
	m_Simplex[0] = SelectFT(isClosestPointOnVertex,vertex0,closestPointOnSegment);
}

#if !__DEV
__forceinline 
#endif // !__DEV
	VecBoolV_Out GetUnusedVerticesPointTriangleHelper(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_InOut closestPointOnTriangle, Vec3V_InOut normalOnTriangle)
{
	const ScalarV svZero(V_ZERO);
	const Vec3V vZero(svZero);

	Vec3V segment01 = Subtract(vertex1,vertex0);
	Vec3V segment12 = Subtract(vertex2,vertex1);
	Vec3V segment20 = Subtract(vertex0,vertex2);

	Vec3V normal = Cross(segment01,segment12);

	Vec3V segment0P = Subtract(point, vertex0);
	Vec3V segment1P = Subtract(point, vertex1);
	Vec3V segment2P = Subtract(point, vertex2);

	Vec3V vertexEdgeCross01 = Cross(segment0P,segment01);
	Vec3V vertexEdgeCross12 = Cross(segment1P,segment12);
	Vec3V vertexEdgeCross20 = Cross(segment2P,segment20);

	// In this function:
	//   When a Vec3V or VecBoolV stores vertex information: X = vertex0, Y = vertex1, Z = vertex2
	//   When a Vec3V or VecBoolV stores edge information: X = vertex1->vertex2 edge, Y = vertex2->vertex0 edge, Z = vertex0->vertex1 edge

	// Find out which side of the edge planes the point is. 
	Vec3V scaledDistancesFromEdgePlane = Vec3V( Dot(normal,vertexEdgeCross12), Dot(normal,vertexEdgeCross20), Dot(normal,vertexEdgeCross01) );
	VecBoolV isPointOutsideEdgePlanes = IsGreaterThan(scaledDistancesFromEdgePlane, vZero);

	// Project the point onto the three edges
	Vec3V edgeLengthSquared = Vec3V( MagSquared(segment12), MagSquared(segment20), MagSquared(segment01) );
	Vec3V scaledFractionOnEdge = Vec3V( Dot(segment12,segment1P), Dot(segment20,segment2P), Dot(segment01,segment0P) );
	Vec3V fractionOnEdge = InvScale(scaledFractionOnEdge,edgeLengthSquared);

	// Determine which section of the edge the point projects on. Between the vertices, above the end vertex, or before the start vertex. 
	VecBoolV isPointOutsideEdgeUpper = IsGreaterThan(scaledFractionOnEdge, edgeLengthSquared);
	VecBoolV isPointOutsideEdgeLower = IsLessThan(scaledFractionOnEdge, vZero);

	// The point is in the next vertex region if it is outside the edge across from the previous vertex on the upper end AND is outside the edge across from this vertex
	//   on the lower end
	VecBoolV isPointInNextVertexRegion = And(isPointOutsideEdgeUpper.Get<Vec::Z,Vec::X,Vec::Y,Vec::W>(),isPointOutsideEdgeLower);
	VecBoolV isPointInPreviousVertexRegion = isPointInNextVertexRegion.Get<Vec::Y,Vec::Z,Vec::X,Vec::W>();

	// The point is in the edge region if it is outside of the edge plane, and it projects onto the edge without going outside either vertex
	VecBoolV isPointInEdgeRegion = And(isPointOutsideEdgePlanes,InvertBits(Or(isPointOutsideEdgeUpper,isPointOutsideEdgeLower)));

	// A vertex is unused if the point is in the region owned by the other two vertices, or the edge between them
	VecBoolV unusedVertices = isPointInNextVertexRegion | isPointInPreviousVertexRegion | isPointInEdgeRegion;
	BoolV allVerticesUsed = InvertBits(unusedVertices.GetX() | unusedVertices.GetY() | unusedVertices.GetZ());

	// Compute the closest points on each edge
	Vec3V closestPointEdge01 = AddScaled(vertex0,segment01,fractionOnEdge.GetZ());
	Vec3V closestPointEdge12 = AddScaled(vertex1,segment12,fractionOnEdge.GetX());
	Vec3V closestPointEdge20 = AddScaled(vertex2,segment20,fractionOnEdge.GetY());

	// Compute the closest point on the plane
	ScalarV distanceAbovePlane = Dot(segment0P,normal);
	Vec3V closestPointPlane = SubtractScaled(point, normal, Scale(distanceAbovePlane,InvMagSquared(normal)));

	// Logically, exactly one of the following booleans will be set. Use whichever closest position corresponds 
	//  to that boolean
	closestPointOnTriangle =	(Vec3V(isPointInNextVertexRegion.GetX())&vertex1) |
								(Vec3V(isPointInNextVertexRegion.GetY())&vertex2) |
								(Vec3V(isPointInNextVertexRegion.GetZ())&vertex0) |
								(Vec3V(isPointInEdgeRegion.GetX())&closestPointEdge12) |
								(Vec3V(isPointInEdgeRegion.GetY())&closestPointEdge20) |
								(Vec3V(isPointInEdgeRegion.GetZ())&closestPointEdge01) |
								(Vec3V(allVerticesUsed)&closestPointPlane);

	// Do the same calculation again for the normal instead of the position
	normalOnTriangle =		(Vec3V(isPointInNextVertexRegion.GetX())&segment1P) |
							(Vec3V(isPointInNextVertexRegion.GetY())&segment2P) |
							(Vec3V(isPointInNextVertexRegion.GetZ())&segment0P) |
							(Vec3V(isPointInEdgeRegion.GetX())&Cross(segment12,vertexEdgeCross12)) |
							(Vec3V(isPointInEdgeRegion.GetY())&Cross(segment20,vertexEdgeCross20)) |
							(Vec3V(isPointInEdgeRegion.GetZ())&Cross(segment01,vertexEdgeCross01)) |
							(Vec3V(allVerticesUsed)&SelectFT(IsGreaterThan(distanceAbovePlane,svZero),Negate(normal),normal));
	
	return unusedVertices;
}

void VoronoiSingleSimplexSolver::ComputeClosestPointAndNormalOnTriangle(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_InOut closestPointOnTriangle, Vec3V_InOut normalOnTriangle)
{
	const VecBoolV unusedVertices = GetUnusedVerticesPointTriangleHelper(point,vertex0,vertex1,vertex2,closestPointOnTriangle,normalOnTriangle);

	// Remove any unused vertices from the simplex
	const u32* unusedVertexMasks = (const u32*)&unusedVertices;

	int numVertices = ISelectI(unusedVertexMasks[2],3,2);

	numVertices = ISelectI(unusedVertexMasks[1],numVertices,numVertices-1);
	m_Simplex[1] = m_Simplex[ISelectI(unusedVertexMasks[1],1,numVertices)];

	numVertices = ISelectI(unusedVertexMasks[0],numVertices,numVertices-1);
	m_Simplex[0] = m_Simplex[ISelectI(unusedVertexMasks[0],0,numVertices)];

	m_NumVertices = numVertices;
}


void VoronoiSingleSimplexSolver::ComputeClosestPointAndNormalOnTetrahedron(Vec3V_In point, Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2, Vec3V_In vertex3, Vec3V_InOut closestPointOnTetrahedron, Vec3V_InOut normalOnTetrahedron)
{
	// Determine which triangles the point is in front of. If the point is behind a triangle, we know that the closest feature
	//   isn't that triangles face. 
	Vec3V segment01 = Subtract(vertex1,vertex0);
	Vec3V segment12 = Subtract(vertex2,vertex1);
	Vec3V segment23 = Subtract(vertex3,vertex2);
	Vec3V segment30 = Subtract(vertex0,vertex3);

	Vec3V normal0 = Cross(segment12,segment23);
	Vec3V normal1 = Cross(segment23,segment30);
	Vec3V normal2 = Cross(segment01,segment30);
	Vec3V normal3 = Cross(segment01,segment12);	

	Vec3V segment0P = Subtract(point,vertex0);
	Vec3V segment1P = Subtract(point,vertex1);

	// The point is in front of a triangle if the fourth vertex, not in the triangle, is on the opposite side of the triangle. This
	//   fails once the tetrahedron becomes degenerate, and it is possible for the point to be in front of faces. AddVertex should 
	//   be preventing degenerate simplexes though.
	Vec4V signPoint    = Vec4V( Dot(normal0,segment1P), Dot(normal1,segment0P), Dot(normal2,segment0P), Dot(normal3,segment0P) );
	Vec4V signBackface = Vec4V( Dot(normal0,segment30), Dot(normal1,segment01), Dot(normal2,segment12), Dot(normal3,segment23) );

	const VecBoolV isPointInfrontOfOpposingPlanes = InvertBits(SameSign(signPoint, signBackface));
	Assertf(!IsTrueAll(isPointInfrontOfOpposingPlanes), "Point is in front of all planes in tetrahedron. The simplex must be degenerate.");

	const VecBoolV unusedFourthVertex = VecBoolV(V_F_F_F_T);
	VecBoolV unusedVertices = VecBoolV(V_F_F_F_F);

	ScalarV shortestDistanceSquared(V_FLT_MAX);

	// Initialize the closest point to the users point. If the point isn't in front of any triangle then it is contained
	//   by the tetrahedron. In that case giving the user their own point lets them know the simplex is touching their point. 
	closestPointOnTetrahedron = point;

	// TODO:
	//   We should be able to get rid of these branches. Removing them now makes this a bit slower, but that is because the triangle helper is so large. There
	//     is a large amount of data being recomputed in the helper function (edges, normals, t-values, etc...). If we pulled everything into this function and
	//     shared the computed vectors I think we would save a significant amount.
	if(isPointInfrontOfOpposingPlanes.GetX().Getb())
	{
		// The point is outside the triangle made by vertex 1, vertex 2, and vertex 3. Just overwrite out best position/normal/distance since this is the first
		//   triangle to be tested.
		unusedVertices = Or(GetUnusedVerticesPointTriangleHelper(point,vertex1,vertex2,vertex3,closestPointOnTetrahedron,normalOnTetrahedron),unusedFourthVertex).Get<Vec::W,Vec::X,Vec::Y,Vec::Z>();
		shortestDistanceSquared = DistSquared(closestPointOnTetrahedron,point);
	}

	if(isPointInfrontOfOpposingPlanes.GetY().Getb())
	{
		// The point is outside the triangle made by vertex 0, vertex 2, and vertex 3. See if this triangle is better than the previous ones
		Vec3V closestPoint023;
		Vec3V normal023;
		VecBoolV unusedVertices023 = Or(GetUnusedVerticesPointTriangleHelper(point,vertex0,vertex2,vertex3,closestPoint023,normal023),unusedFourthVertex).Get<Vec::X,Vec::W,Vec::Y,Vec::Z>();
		ScalarV distanceSquared023 = DistSquared(closestPoint023,point);
		BoolV newClosestTriangle = IsLessThan(distanceSquared023,shortestDistanceSquared);
		
		shortestDistanceSquared =   SelectFT(newClosestTriangle, shortestDistanceSquared, distanceSquared023);
		closestPointOnTetrahedron = SelectFT(newClosestTriangle, closestPointOnTetrahedron, closestPoint023);
		normalOnTetrahedron =       SelectFT(newClosestTriangle, normalOnTetrahedron, normal023);
		unusedVertices = (VecBoolV(!newClosestTriangle)&unusedVertices) | (VecBoolV(newClosestTriangle)&unusedVertices023);
	}

	if(isPointInfrontOfOpposingPlanes.GetZ().Getb())
	{
		// The point is outside the triangle made by vertex 0, vertex 1, and vertex 3. See if this triangle is better than the previous ones.
		Vec3V closestPoint013;
		Vec3V normal013;
		VecBoolV unusedVertices013 = Or(GetUnusedVerticesPointTriangleHelper(point,vertex0,vertex1,vertex3,closestPoint013,normal013),unusedFourthVertex).Get<Vec::X,Vec::Y,Vec::W,Vec::Z>();
		ScalarV distanceSquared013 = DistSquared(closestPoint013,point);
		BoolV newClosestTriangle = IsLessThan(distanceSquared013,shortestDistanceSquared);
		
		shortestDistanceSquared =   SelectFT(newClosestTriangle, shortestDistanceSquared, distanceSquared013);
		closestPointOnTetrahedron = SelectFT(newClosestTriangle, closestPointOnTetrahedron, closestPoint013);
		normalOnTetrahedron =       SelectFT(newClosestTriangle, normalOnTetrahedron, normal013);
		unusedVertices = (VecBoolV(!newClosestTriangle)&unusedVertices) | (VecBoolV(newClosestTriangle)&unusedVertices013);		
	}

	if(isPointInfrontOfOpposingPlanes.GetW().Getb())
	{
		// The triangle is outside the vector made by vertex 0, vertex 1, and vertex 2. See if this triangle is better than the previous ones
		Vec3V closestPoint012;
		Vec3V normal012;
		VecBoolV unusedVertices012 = Or(GetUnusedVerticesPointTriangleHelper(point,vertex0,vertex1,vertex2,closestPoint012,normal012),unusedFourthVertex);
		ScalarV distanceSquared012 = DistSquared(closestPoint012,point);
		BoolV newClosestTriangle = IsLessThan(distanceSquared012,shortestDistanceSquared);
		
		shortestDistanceSquared =   SelectFT(newClosestTriangle, shortestDistanceSquared, distanceSquared012);
		closestPointOnTetrahedron = SelectFT(newClosestTriangle, closestPointOnTetrahedron, closestPoint012);
		normalOnTetrahedron =       SelectFT(newClosestTriangle, normalOnTetrahedron, normal012);
		unusedVertices = (VecBoolV(!newClosestTriangle)&unusedVertices) | (VecBoolV(newClosestTriangle)&unusedVertices012);
	}

	// Remove the unused vertices from the simplex
	u32* unusedVertexMasks = (u32*)&unusedVertices;

	int numVertices = ISelectI(unusedVertexMasks[3],4,3);

	numVertices = ISelectI(unusedVertexMasks[2],numVertices,numVertices-1);
	m_Simplex[2] = m_Simplex[ISelectI(unusedVertexMasks[2],2,numVertices)];

	numVertices = ISelectI(unusedVertexMasks[1],numVertices,numVertices-1);
	m_Simplex[1] = m_Simplex[ISelectI(unusedVertexMasks[1],1,numVertices)];

	numVertices = ISelectI(unusedVertexMasks[0],numVertices,numVertices-1);
	m_Simplex[0] = m_Simplex[ISelectI(unusedVertexMasks[0],0,numVertices)];

	m_NumVertices = numVertices;
}

void VoronoiSingleSimplexSolver::ComputeClosestPointAndNormalOnSimplex(Vec3V_In point, Vec3V_InOut closestPointOnSimplex, Vec3V_InOut normalOnSimplex)
{
	// Determine what shape (point, segment, triangle, tetrahedron) the simplex is, and update it accordingly.
	switch(m_NumVertices)
	{
	case 1:
		ComputeClosestPointAndNormalOnPoint(point,m_Simplex[0],closestPointOnSimplex,normalOnSimplex);
		break;
	case 2:
		ComputeClosestPointAndNormalOnSegment(point,m_Simplex[0],m_Simplex[1],closestPointOnSimplex,normalOnSimplex);
		break;
	case 3:
		ComputeClosestPointAndNormalOnTriangle(point,m_Simplex[0],m_Simplex[1],m_Simplex[2],closestPointOnSimplex,normalOnSimplex);
		break;
	default: // 4
		Assertf(m_NumVertices == 4, "A simplex with %i vertices is not allowed in VoronoiSingleSimplexSolver::ComputeClosestPointAndNormalOnSimplex.",m_NumVertices);
		ComputeClosestPointAndNormalOnTetrahedron(point,m_Simplex[0],m_Simplex[1],m_Simplex[2],m_Simplex[3],closestPointOnSimplex,normalOnSimplex);
		break;
	}
	Assertf(m_NumVertices > 0, "VoronoiSingleSimplexSolver::ComputeClosestPointOnSimplex removed all vertices from the simplex.");

	// If the user gives us a point inside the simplex, this is undefined. It's the users responsibility to check
	//   that "closestPointOnSimplex" isn't the same as "point" before using the normal.
	// TODO: 
	//   Should the user be in charge of this? Normalizing isn't cheap.
	normalOnSimplex = Normalize(normalOnSimplex);
}

} // namespace rage