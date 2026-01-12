//
// phbound/boundcurvedgeom.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundcurvedgeom.h"

#include "atl/bitset.h"
#include "grcore/viewport.h"
#include "math/simplemath.h"
#include "phbound/support.h"
#include "phcore/materialmgrflag.h"
#include "phcore/phmath.h"
#include "grprofile/drawmanager.h"
#include "system/cache.h"
#include "system/memops.h"
#include "vector/colors.h"

namespace rage {

#if USE_GEOMETRY_CURVED

#if !__TOOL
CompileTimeAssert(sizeof(phBoundCurvedGeometry) == phBound::MAX_BOUND_SIZE);
#endif

//////////////////////////////////////////////////////////////
// bound profiler variables
EXT_PFD_DECLARE_ITEM(BvhPolygonIndices);
EXT_PFD_DECLARE_ITEM(NonBvhPolygonIndices);
EXT_PFD_DECLARE_ITEM(Quads);
EXT_PFD_DECLARE_ITEM(DrawBoundMaterialNames);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDistanceOpacity);
EXT_PFD_DECLARE_ITEM_SLIDER(ThinPolyTolerance);
EXT_PFD_DECLARE_GROUP(PolygonDensity);
EXT_PFD_DECLARE_GROUP(PrimitiveDensity);

#if __DECLARESTRUCT
void phCurvedEdge::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(phCurvedEdge);

	STRUCT_FIELD(m_CurvatureCenter);
	STRUCT_FIELD(m_PlaneNormal);
	STRUCT_FIELD(m_Radius);
	STRUCT_CONTAINED_ARRAY(m_VertexIndices);
	STRUCT_FIELD(m_UnusedInt);

	STRUCT_END();
}
#endif // !__FINAL

void phCurvedEdge::Init (int vertIndex0, int vertIndex1, float radius, Vector3::Param planeNormal, Vector3::Param center)
{
	m_CurvatureCenter.Set(center);
	m_PlaneNormal.Set(planeNormal);
	m_Radius = radius;
	m_VertexIndices[0] = vertIndex0;
	m_VertexIndices[1] = vertIndex1;
	Assert(m_VertexIndices[0]!=BAD_INDEX);
}


void phCurvedEdge::ScaleSize (float xScale, float yScale, float zScale, const phBoundCurvedGeometry* curvedBound)
{
	// Scale the center of curvature position.
	m_CurvatureCenter.x *= xScale;
	m_CurvatureCenter.y *= yScale;
	m_CurvatureCenter.z *= zScale;

	// Compute the new plane normal.
	Vector3 centerToVert0(VEC3V_TO_VECTOR3(curvedBound->GetVertex(m_VertexIndices[0])));
	centerToVert0.Subtract(m_CurvatureCenter);
	Vector3 centerToVert1(VEC3V_TO_VECTOR3(curvedBound->GetVertex(m_VertexIndices[1])));
	centerToVert1.Subtract(m_CurvatureCenter);
	Vector3 newPlaneNormal(centerToVert0);
	newPlaneNormal.Cross(centerToVert1);
	float crossMag2 = newPlaneNormal.Mag2();
	if (crossMag2>SMALL_FLOAT)
	{
		newPlaneNormal.Scale(invsqrtf(crossMag2));
		if (newPlaneNormal.Dot(m_PlaneNormal)<0.0f)
		{
			newPlaneNormal.Negate();
		}

		// Set the new plane normal.
		m_PlaneNormal.Set(newPlaneNormal);
	}

	// Compute the new radius of curvature.
	m_Radius = m_CurvatureCenter.Dist(VEC3V_TO_VECTOR3(curvedBound->GetVertex(m_VertexIndices[0])));
}


#if __DECLARESTRUCT
void phCurvedFace::DeclareStruct(datTypeStruct &s)
{
	phPolygon::DeclareStruct(s);

	STRUCT_BEGIN(phCurvedFace);

	STRUCT_FIELD(m_CurvatureCenter);

	STRUCT_FIELD(m_UnitNormal);
	STRUCT_FIELD(m_OuterRadius);
	STRUCT_FIELD(m_InnerRadius);
	STRUCT_FIELD(m_MinCosine);
	STRUCT_CONTAINED_ARRAY(m_CurvedEdgeIndices);
	STRUCT_CONTAINED_ARRAY(m_CurvedEdgePolyIndices);
	STRUCT_FIELD(m_NumCurvedEdges);

	STRUCT_FIELD(m_FourthVertex);

	STRUCT_FIELD(m_IsCircularFace);

	STRUCT_IGNORE(m_Pad);

	STRUCT_END();
}
#endif // __DECLARESTRUCT

#if !(COMPRESSED_VERTEX_METHOD == 0)
void phCurvedFace::Init (int numVerts, int* vertIndices, Vector3::Param v0, Vector3::Param v1, Vector3::Param v2, int numCurvedEdges, int* curvedEdgeIndices,
						 int* curvedEdgePolyIndices, const phCurvedEdge* curvedEdges, float curvature)
{
	Init(numVerts,vertIndices,v0,v1,v2,ORIGIN,numCurvedEdges,curvedEdgeIndices,curvedEdgePolyIndices,curvedEdges,curvature);
}
#endif

#if COMPRESSED_VERTEX_METHOD == 0
void phCurvedFace::Init (int numVerts, int* vertIndices, const Vector3* vertices, int numCurvedEdges, int* curvedEdgeIndices, int* curvedEdgePolyIndices,
						 const phCurvedEdge* curvedEdges, float curvature)
#else
void phCurvedFace::Init (int numVerts, int* vertIndices, Vector3::Param v0, Vector3::Param v1, Vector3::Param v2, Vector3::Param UNUSED_PARAM(v3), int numCurvedEdges,
						 int* curvedEdgeIndices, int* curvedEdgePolyIndices, const phCurvedEdge* curvedEdges, float curvature)
#endif
{
	// Set the vertex index numbers.
	m_VertexIndices[1] = (phPolygon::Index)BAD_INDEX;
	m_VertexIndices[2] = (phPolygon::Index)BAD_INDEX;

	m_FourthVertex = (phPolygon::Index)BAD_INDEX;

	switch (numVerts)
	{
		case 4:
		{
			m_FourthVertex = (phPolygon::Index)vertIndices[3];
		}
		case 3:
		{
			m_VertexIndices[2] = (phPolygon::Index)vertIndices[2];
		}
		case 2:
		{
			m_VertexIndices[1] = (phPolygon::Index)vertIndices[1];
		}
		default:
		{
			m_VertexIndices[0] = (phPolygon::Index)vertIndices[0];
		}
	}

	// Set the curved edge index numbers.
	m_NumCurvedEdges = numCurvedEdges;
	for (int curvedEdgeIndex=0; curvedEdgeIndex<m_NumCurvedEdges; curvedEdgeIndex++)
	{
		m_CurvedEdgeIndices[curvedEdgeIndex] = (phPolygon::Index)curvedEdgeIndices[curvedEdgeIndex];
		m_CurvedEdgePolyIndices[curvedEdgeIndex] = (phPolygon::Index)curvedEdgePolyIndices[curvedEdgeIndex];
	}

	// Set the face's radius of curvature.
	m_OuterRadius = -1.0f;
	m_InnerRadius = 0.0f;
	m_MinCosine = 0.0f;

	// Set the curvature parameters.
	m_CurvatureCenter.Zero();
	const phCurvedEdge& curvedEdge = curvedEdges[m_CurvedEdgeIndices[0]];
	m_UnitNormal = VECTOR3_TO_VEC3V(curvedEdge.GetPlaneNormal());
	if (m_NumCurvedEdges==0)
	{
		Assert(numVerts>2);

#if COMPRESSED_VERTEX_METHOD == 0
		CalculateNormalAndArea(RCC_VEC3V(vertices[GetVertexIndex(0)]), RCC_VEC3V(vertices[GetVertexIndex(1)]), RCC_VEC3V(vertices[GetVertexIndex(2)]));
#else
		CalculateNormalAndArea(RCC_VEC3V(v0), RCC_VEC3V(v1), RCC_VEC3V(v2));
#endif
	}
	else if (curvature>=0.0f)
	{
		if (ComputeIsCircularFace(curvedEdges))
		{
			// This face is an outward-curved circle.
			m_InnerRadius = -1.0f;

			// Set the polygon area.
			SetArea(PI*square(curvedEdge.GetRadius()));

			// Set the face's radius of curvature.
			if (curvature>0.0f)
			{
				// This face has curvature, so set its center and minimum cosine from its single curved edge.
				// Make sure the cuvature is not greater than 1.
				curvature = Min(curvature,1.0f);

				// Make the outer radius vary from the minimum (the curved edge radius) to infinity, with curvature from 0 to 1.
				m_OuterRadius = curvedEdge.GetRadius()*InvertSafe(curvature);
				m_MinCosine = SqrtfSafe(1.0f-square(curvedEdge.GetRadius()/m_OuterRadius));
				m_CurvatureCenter.Set(curvedEdge.GetCurvatureCenter());
				m_CurvatureCenter.SubtractScaled(curvedEdge.GetPlaneNormal(),m_OuterRadius*m_MinCosine);
			}
			else
			{
				// This face has no curvature. Make the curvature center the flat center.
				m_CurvatureCenter.Set(curvedEdge.GetCurvatureCenter());
				m_OuterRadius = -1.0f;
				m_InnerRadius = 0.0f;
				m_MinCosine = 0.0f;
			}
		}
		else
		{
			// This face is an outward-curved ring.
	
			// Set the polygon area.
			const phCurvedEdge& curvedEdge1 = curvedEdges[m_CurvedEdgeIndices[(m_NumCurvedEdges==2 ? 1 : 2)]];
			float ringWidth = curvedEdge.GetCurvatureCenter().Dist(curvedEdge1.GetCurvatureCenter());
			SetArea(2.0f*PI*curvedEdge.GetRadius()*ringWidth);

			if (curvature>0.0f)
			{
				// This face has curvature, so set its center and minimum cosine from its single curved edge.
				// Make sure the cuvature is not greater than 1.
				curvature = Min(curvature,1.0f);

				// Make the outer radius vary from the minimum (the curved edge radius) to infinity, with curvature from 0 to 1.
				m_OuterRadius = curvedEdge.GetRadius()*InvertSafe(curvature);

				// This curved face is the looping face of a ring (the tread face of a tire).
				// Make the center of curvature between the two edge centers (the edges must be circles).
				m_CurvatureCenter.Average(curvedEdge.GetCurvatureCenter(),curvedEdge1.GetCurvatureCenter());

				// Set the minimum cosine.
				m_MinCosine = SqrtfSafe(1.0f-square(0.5f*ringWidth/m_OuterRadius));

				// Set the inner radius (zero if the center of curvature is the center of curvature for the whole face).
				m_InnerRadius = curvedEdge.GetRadius()-m_OuterRadius*m_MinCosine;
			}
		}
	}
}


bool phCurvedFace::ComputeIsCircularFace (const phCurvedEdge* curvedEdges)
{
	// Get the number of vertices, and call this face circular if it only has one vertex and one edge.
	int numVertices = GetNumVertices();
	bool circularFace = (numVertices==1 && m_NumCurvedEdges==1);

	// See if this face has two vertices and two edges. Assume that nobody would make a circular face with more than two vertics and two edges.
	if (numVertices==2 && m_NumCurvedEdges==2)
	{
		// This face has two vertices and two curved edges. Call this face circular if the edges are parallel.
		Vector3 edge0Normal(curvedEdges[m_CurvedEdgeIndices[0]].GetPlaneNormal());
		Vector3 edge1Normal(curvedEdges[m_CurvedEdgeIndices[1]].GetPlaneNormal());
		circularFace = (edge0Normal.Dot(edge1Normal)>0.99f);
	}

	// Set and return the circular face flag.
	m_IsCircularFace = circularFace;
	return m_IsCircularFace;
}


void phCurvedFace::SetCurvatureCenter (Vector3::Ref center)
{
	m_CurvatureCenter.Set(center);
}


void phCurvedFace::ScaleSize (float xScale, float yScale, float zScale, const phBoundCurvedGeometry* UNUSED_PARAM(curvedBound), const phCurvedEdge* curvedEdges)
{
	// Find and scale the distance that the curvature extends beyond the flat face.
	float curvedOutDistance = m_OuterRadius*(1.0f-m_MinCosine)*fabsf(m_UnitNormal.GetXf()*xScale+m_UnitNormal.GetYf()*yScale+m_UnitNormal.GetZf()*zScale);

	// Get a reference to one of the face's curved edges.
	const phCurvedEdge& curvedEdge0 = curvedEdges[m_CurvedEdgeIndices[0]];

	// See if this is a circular face, or a ring-shaped face.
	if (IsCircularFace())
	{
		// This is a circular face.
		// Get the already-scaled radius of a curved edge on this face.
		float edgeRadius = curvedEdge0.GetRadius();

		// Compute the new radius of curvature.
		float oldOuterRadius = m_OuterRadius;
		m_OuterRadius = (square(curvedOutDistance)+square(edgeRadius))/(2.0f*curvedOutDistance);

		// Compute the new cosine.
		m_MinCosine = SqrtfSafe(1.0f-square(edgeRadius/m_OuterRadius));

		// Compute the new center of curvature.
		m_CurvatureCenter.AddScaled(RCC_VECTOR3(m_UnitNormal),oldOuterRadius-m_OuterRadius);

		// Make sure the inner radius is still zero.
		m_InnerRadius = 0.0f;
	}
	else
	{
		// This is a ring-shaped curved face.
		// Compute the new half-width.
		const phCurvedEdge& curvedEdge1 = curvedEdges[m_CurvedEdgeIndices[(m_NumCurvedEdges==2 ? 1 : 2)]];
		Vector3 planeNormal(curvedEdge1.GetCurvatureCenter());
		planeNormal.Subtract(curvedEdge0.GetCurvatureCenter());
		float ringHalfWidth = 0.5f*planeNormal.Mag();

		// Compute the new radius of curvature.
		m_OuterRadius = (square(curvedOutDistance)+square(ringHalfWidth))/(2.0f*curvedOutDistance);

		// Compute the new cosine.
		m_MinCosine = SqrtfSafe(1.0f-square(ringHalfWidth/m_OuterRadius));

		// Compute the new center of curvature.
		m_CurvatureCenter.Average(curvedEdge0.GetCurvatureCenter(),curvedEdge1.GetCurvatureCenter());

		// Compute the new inner radius.
		m_InnerRadius = curvedEdge0.GetRadius()-m_OuterRadius*m_MinCosine;
	}
}

/////////////////////////////////////////////////////////////////
// phBoundCurvedGeometry

phBoundCurvedGeometry::phBoundCurvedGeometry ()
{
	m_Type = GEOMETRY_CURVED;

	sysMemZeroBytes<sizeof(m_Pad)>(m_Pad);
}

#if !__SPU
phBoundCurvedGeometry::phBoundCurvedGeometry (datResource& rsc) : phBoundGeometry (rsc)
{
	rsc.PointerFixup(m_CurvedFaces);
	rsc.PointerFixup(m_CurvedEdges);
	rsc.PointerFixup(m_CurvedFaceMatIndexList);

	if (m_NumCurvedEdges>SPU_MAX_CURVED_EDGES)
	{
		Warningf("This curved geometry bound has too many curved edges to work on the SPU.");
	}
	
	if (m_NumCurvedFaces>SPU_MAX_CURVED_FACES)
	{
		Warningf("This curved geometry bound has too many curved faces to work on the SPU.");
	}
}


phBoundCurvedGeometry::~phBoundCurvedGeometry ()
{
	delete [] m_CurvedFaceMatIndexList;
	delete [] m_CurvedFaces;
	delete [] m_CurvedEdges;
}

#endif // !__SPU

#if __DECLARESTRUCT
void phBoundCurvedGeometry::DeclareStruct(datTypeStruct &s)
{
	phBoundGeometry::DeclareStruct(s);

	STRUCT_BEGIN(phBoundCurvedGeometry);

	STRUCT_DYNAMIC_ARRAY(m_CurvedFaces,m_NumCurvedFaces);
	STRUCT_DYNAMIC_ARRAY(m_CurvedEdges,m_NumCurvedEdges);

	STRUCT_FIELD_VP(m_CurvedFaceMatIndexList); // count on this was already swapped but since it's an array of u8s we don't need to swap contents, so only swap pointer

	STRUCT_FIELD(m_NumCurvedFaces);
	STRUCT_FIELD(m_NumCurvedEdges);

	STRUCT_CONTAINED_ARRAY(m_Pad);

	STRUCT_END();
}
#endif // __DECLARESTRUCT

void phBoundCurvedGeometry::Init (int numVerts, int numMaterials, int numPolys, int numCurvedEdges, int numCurvedPolys)
{
	const int numPerVertAttribs=0;
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE_DATA
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		const int nMaterialColors=0;
		phBoundGeometry::Init(numVerts,numPerVertAttribs,numMaterials,nMaterialColors,numPolys,0);
	#else
		phBoundGeometry::Init(numVerts,numPerVertAttribs,numMaterials,numPolys,0);
	#endif
#else
	#if HACK_GTA4_64BIT_MATERIAL_ID_COLORS
		const int nMaterialColors=0;
		phBoundGeometry::Init(numVerts,numPerVertAttribs,numMaterials,nMaterialColors,numPolys);
	#else
		phBoundGeometry::Init(numVerts,numPerVertAttribs,numMaterials,numPolys);
	#endif
#endif

	Assert(numCurvedEdges < (u16)-1);
	Assert(numCurvedPolys < (u16)-1);

	m_NumCurvedEdges = (u16)numCurvedEdges;
	m_NumCurvedFaces = (u16)numCurvedPolys;

	AllocateCurvedEdgesAndFaces();
}

void phBoundCurvedGeometry::AllocateCurvedEdgesAndFaces ()
{
	if (m_NumCurvedEdges>SPU_MAX_CURVED_EDGES)
	{
		Warningf("This curved geometry bound has too many curved edges to work on the SPU");
	}
	
	if (m_NumCurvedFaces>SPU_MAX_CURVED_FACES)
	{
		Warningf("This curved geometry bound has too many curved faces to work on the SPU");
	}

	m_CurvedEdges = m_NumCurvedEdges ? rage_new phCurvedEdge[m_NumCurvedEdges] : 0;
	m_CurvedFaces = m_NumCurvedFaces ? rage_new phCurvedFace[m_NumCurvedFaces] : 0;

	m_CurvedFaceMatIndexList = rage_new u8[m_NumCurvedFaces];
	sysMemSet(m_CurvedFaceMatIndexList, 0, sizeof(u8) * m_NumCurvedFaces);
}


void phBoundCurvedGeometry::SetCurvedEdge (int curvedEdgeIndex, int vertIndex0, int vertIndex1, float radius, Vector3::Param planeNormal, Vector3::Param curveDirection)
{
	// Get a reference to the curved edge.
	Assert(curvedEdgeIndex<m_NumCurvedEdges);
	phCurvedEdge& edge = m_CurvedEdges[curvedEdgeIndex];

	// Find the center of curvature.
	Vector3 center;
	center.Average(VEC3V_TO_VECTOR3(GetVertex(vertIndex0)),VEC3V_TO_VECTOR3(GetVertex(vertIndex1)));
	if (vertIndex0!=vertIndex1)
	{
		// This curved edge has two vertices.
		Vector3 vert0to1(VEC3V_TO_VECTOR3(GetVertex(vertIndex1)));
		vert0to1.Subtract(VEC3V_TO_VECTOR3(GetVertex(vertIndex0)));
		center.SubtractScaled(curveDirection,SqrtfSafe(square(radius)-0.25f*vert0to1.Mag2()));

		// Make sure that the vertices are in order so that looking into the plane from out in the direction planeNormal with curveDirection pointing up,
		// the first vertex is on the right and the second vertex is on the left.
		Vector3 curveDirCross0to1(curveDirection);
		curveDirCross0to1.Cross(vert0to1);
		if (curveDirCross0to1.Dot(planeNormal)<0.0f)
		{
			SwapEm(vertIndex0,vertIndex1);
		}
	}
	else
	{
		// The given vertex index numbers are the same, so this curved edge only has one vertex (it is a circle).
		center.AddScaled(curveDirection,radius);
	}

	// Initialize the curved edge.
	edge.Init(vertIndex0,vertIndex1,radius,planeNormal,center);
}


void phBoundCurvedGeometry::SetCurvedEdge (int curvedEdgeIndex, int vertIndex0, int vertIndex1, Vector3::Param planeNormal, Vector3::Param curveCenter)
{
	// Get a reference to the curved edge.
	Assert(curvedEdgeIndex<m_NumCurvedEdges);
	phCurvedEdge& edge = m_CurvedEdges[curvedEdgeIndex];

	// Find the radius of curvature.
	Vector3 center(curveCenter);
	float radius2Vert0 = center.Dist2(VEC3V_TO_VECTOR3(GetVertex(vertIndex0)));
	float radius2Vert1 = center.Dist2(VEC3V_TO_VECTOR3(GetVertex(vertIndex1)));
	AssertMsg(!(radius2Vert0<VERY_SMALL_FLOAT),"A curved geometry edge has a bad center of curvature.");
	AssertMsg(!(radius2Vert1<VERY_SMALL_FLOAT),"A curved geometry edge has a bad center of curvature.");
	float radius = 0.5f*(SqrtfSafe(radius2Vert0)+SqrtfSafe(radius2Vert1));

	// Initialize the curved edge.
	edge.Init(vertIndex0,vertIndex1,radius,planeNormal,curveCenter);
}


void phBoundCurvedGeometry::SetCurvedFace (int curvedFaceIndex, int numVerts, int* vertIndices, int numCurvedEdges, int* curvedEdgeIndices, int* curvedEdgePolyIndices, float curvature)
{
	Assert(curvedFaceIndex<m_NumCurvedFaces);
#if COMPRESSED_VERTEX_METHOD == 0
	m_CurvedFaces[curvedFaceIndex].Init(numVerts,vertIndices,(const Vector3*)m_Vertices,numCurvedEdges,curvedEdgeIndices,curvedEdgePolyIndices,m_CurvedEdges,curvature);
#else
	Vector3 v0(VEC3V_TO_VECTOR3(GetVertex(vertIndices[0])));
	Vector3 v1(numVerts>1 ? VEC3V_TO_VECTOR3(GetVertex(vertIndices[1])) : (Vector3)(VEC3_ZERO));
	Vector3 v2(numVerts>2 ? VEC3V_TO_VECTOR3(GetVertex(vertIndices[2])) : (Vector3)(VEC3_ZERO));
	Vector3 v3(numVerts>3 ? VEC3V_TO_VECTOR3(GetVertex(vertIndices[3])) : (Vector3)(VEC3_ZERO));
	m_CurvedFaces[curvedFaceIndex].Init(numVerts,vertIndices,v0,v1,v2,v3,numCurvedEdges,curvedEdgeIndices,curvedEdgePolyIndices,m_CurvedEdges,curvature);
#endif
}


#define COLLIDE_WITH_VERTICES 1

#if COLLIDE_WITH_VERTICES
#define COLLIDE_WITH_VERTICES_ONLY(X) X
#else
#define COLLIDE_WITH_VERTICES_ONLY(X)
#endif

#define COLLIDE_WITH_FACES 1

Vec3V_Out phBoundCurvedGeometry::LocalGetSupportingVertex (Vec::V3Param128 localDirection)const
{
	// Get the unit normal along the local direction.
	Vec3V localDir = Vec3V(localDirection);
	Vec3V vecnorm = NormalizeSafe(localDir,Vec3V(V_X_AXIS_WONE),Vec3V(V_ZERO));

	// Get the supporting vertex without including the collision margin.
	Vec3V supVertex = phBoundCurvedGeometry::LocalGetSupportingVertexWithoutMargin(vecnorm.GetIntrin128());

	// Get the collision margin.
	ScalarV margin = ScalarVFromF32(GetMargin());

	// Extent the supporting vertex along the local direction by the collision margin.
	vecnorm = Scale(vecnorm,margin);
	supVertex = Add(supVertex,vecnorm);

	// Return the supporting vertex position.
	return supVertex;
}

Vec3V_Out phBoundCurvedGeometry::LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 localDirection, int* COLLIDE_WITH_VERTICES_ONLY(vertexIndex)) const
{
	// Get the member variables (multithreaded versions only).
#if COMPRESSED_VERTEX_METHOD == 0
	PrefetchDC( m_Vertices );
#endif
	PrefetchDC( m_CurvedEdges );
	PrefetchDC( m_CurvedFaces );

	// Construct these constants here in registers and hopefully keep them there to avoid loads later.
	const Vector3 vSmallFloat = VEC3V_TO_VECTOR3(Vec3V(V_FLT_SMALL_6));
	const Vector3 vZero = VEC3V_TO_VECTOR3(Vec3V(V_ZERO));
	const Vector3 vAndW = VEC4V_TO_VECTOR3(Vec4V(V_MASKXYZ));
	const Vector3 vFltMax = SCALARV_TO_VECTOR3(ScalarV(V_FLT_MAX));

	// Find the vertex that extends most along the collision direction.
	Vector3 unitLocalDir(localDirection);
	unitLocalDir.Normalize();
	Vector3 supportPoint = vZero;
	Vector3 biggestDot = vFltMax;
	biggestDot.Negate();

#if COLLIDE_WITH_VERTICES
	int biggestDotIndex = BAD_INDEX;
	const int numVertices = m_NumVertices;
	for (int vertIndex=0; vertIndex<numVertices; vertIndex++)
	{
		Vector3 vertDotDir = VEC3V_TO_VECTOR3(GetVertex(vertIndex)).DotV(unitLocalDir);
		if (vertDotDir.IsGreaterThan(biggestDot))
		{
			biggestDot = vertDotDir;
			biggestDotIndex = vertIndex;
			supportPoint = VEC3V_TO_VECTOR3(GetVertex(vertIndex));
		}
	}
#endif

	// See if any curved edges have a better support point than the best vertex.
	const int numCurvedEdges = m_NumCurvedEdges;
	for (int edgeIndex=0; edgeIndex<numCurvedEdges; edgeIndex++)
	{
		// Get a reference to the curved edge.
		const phCurvedEdge& edge = m_CurvedEdges[edgeIndex];

		// Make sure the radius is 16-byte aligned so it can be loaded as a vector splatted.
		Assert( (size_t( &edge.m_Radius ) & 0x0F) == 0 );
		Scalar edgeRadius;
		edgeRadius.SplatX(*((Vector3 *)(&edge.m_Radius)));

		// Save the previous biggest dot product and support point.
		Vector3 biggestDotOriginal = biggestDot;
		Vector3 supportPointOriginal = supportPoint;

		// Find the direction to the support point in the edge's plane.
		const Vector3 planeNormal(edge.GetPlaneNormal());
		Vector3 edgePoint(planeNormal);
		edgePoint.Cross(unitLocalDir);
		edgePoint.Cross(planeNormal);
		Vector3 edgePointMag2 = edgePoint.Mag2V();
		if (edgePointMag2.IsGreaterThan(vSmallFloat))
		{
			// The collision direction is not parallel to the edge's plane normal.
			// Find the colliding point along the curved edge.
			edgePoint.Multiply(edgeRadius);
			edgePoint.Multiply(edgePointMag2.RecipSqrtV());
			Vector3 relEdgePoint(edgePoint);
			Vector3 curveCenter(edge.GetCurvatureCenter());
			edgePoint.Add(curveCenter);

			// Find the two vertex positions relative to the curvature center.
			Vector3 relVertex0(VEC3V_TO_VECTOR3(GetVertex(edge.GetVertexIndex(0))));
			relVertex0.Subtract(curveCenter);
			Vector3 relVertex1(VEC3V_TO_VECTOR3(GetVertex(edge.GetVertexIndex(1))));
			relVertex1.Subtract(curveCenter);

			// Find the cross products between the two relative vertex positions, and between the relative edge and each of the relative vertices,
			// and their dot products with the edge's plane normal, to see if the colliding point is between the two vertices.
			Vector3 cross(relVertex0);
			cross.Cross(relVertex1);
			Vector3 cross01DotNorm = cross.DotV(planeNormal);
			cross.Cross(relVertex0,relEdgePoint);
			Vector3 cross0EDotNorm = cross.DotV(planeNormal);
			cross.Cross(relEdgePoint,relVertex1);
			Vector3 crossE1DotNorm = cross.DotV(planeNormal);

			// Find whether each cross-dot product is non-negative.
			Vector3 neg01Selector = cross01DotNorm.IsLessThanV(vZero);
			Vector3 neg0Eselector = cross0EDotNorm.IsLessThanV(vZero);
			Vector3 negE1selector = crossE1DotNorm.IsLessThanV(vZero);
			Vector3 nonNeg01selector = neg01Selector;
			Vector3 nonNeg0Eselector = neg0Eselector;
			Vector3 nonNegE1selector = negE1selector;
			nonNeg01selector.Xor(vAndW);
			nonNeg0Eselector.Xor(vAndW);
			nonNegE1selector.Xor(vAndW);

			// Find whether all the cross-dot products are non-negative. This is true when the edge curvature is less than PI and the support point
			// is between the two vertices on the edge.
			Vector3 allNonNegSelector = nonNeg01selector;
			allNonNegSelector.And(nonNeg0Eselector);
			allNonNegSelector.And(nonNegE1selector);

			// Find whether the 01 cross dot product is negative and the other two are not both negative. This is true when the edge curvature is greater than PI
			// and the support point is on the edge between the two vertices.
			Vector3 neg01NonNeg0EandE1Sel = neg0Eselector;
			neg01NonNeg0EandE1Sel.And(negE1selector);
			neg01NonNeg0EandE1Sel.Xor(vAndW);
			neg01NonNeg0EandE1Sel.And(neg01Selector);

			// Find out if the support point is on the curved edge between the two vertices.
			Vector3 pointOnEdgeSelector = allNonNegSelector;
			pointOnEdgeSelector.Or(neg01NonNeg0EandE1Sel);

			// If this is a circular curved edge, set the point to always be on the edge.
			Vector3 circularEdgeSelector = edge.IsCircularV();
			pointOnEdgeSelector.Or(circularEdgeSelector);

			// Find the dot product for the best point on this curved edge, and see if it's bigger than the previous biggest.
			Vector3 curvedEdgeDot = edgePoint.DotV(unitLocalDir);
			Vector3 biggestDotSelector = curvedEdgeDot.IsGreaterThanV(biggestDot);
			biggestDotSelector.And(pointOnEdgeSelector);

			// Set the biggest dot product and the new support point if this edge's colliding point has the biggest dot product.
			biggestDot = biggestDotSelector.Select(biggestDot,curvedEdgeDot);
			supportPoint = biggestDotSelector.Select(supportPoint,edgePoint);
		}

		// Set the biggest dot product and support point back to the ones from the loop over vertices if the edge radius is zero.
		// The above work for this edge is wasted in that case, but this is faster than checking for a zero-radius edge first.
		Vector3 edgeRadiusGTZeroSelector = edgeRadius.IsGreaterThanV(vZero);
		biggestDot = edgeRadiusGTZeroSelector.Select(biggestDotOriginal,biggestDot );
		supportPoint = edgeRadiusGTZeroSelector.Select(supportPointOriginal,supportPoint );
	}

#if COLLIDE_WITH_FACES
	// See if any curved faces have a better supporting vertex.
	for (int faceIndex=0; faceIndex<m_NumCurvedFaces; faceIndex++)
	{
		// Get a reference to the curved face.
		const phCurvedFace& face = m_CurvedFaces[faceIndex];

		// Get the inner radius, outer radius and minimum cosine as a single vector, and splat them into local vectors.
		Assert( (size_t(&face.m_OuterRadius) & 0x0F) == 0 );
		Assert( (size_t(&face.m_InnerRadius) & 0x0F) == 4 );
		Assert( (size_t(&face.m_MinCosine) & 0x0F) == 8 );
		Vector3 vParam = *((Vector3 *)(&face.m_OuterRadius));
		Vector3 vRad,vInnerRad,vMinCos;
		vRad.SplatX(vParam);
		vInnerRad.SplatY(vParam);
		vMinCos.SplatZ(vParam);

		// Save the previous biggest dot product and support point.
		Vector3 biggestDotOriginal = biggestDot;
		Vector3 supportPointOriginal = supportPoint;

		// See if this face is circular (like the side of a wheel) or ring-shaped (like the contact surface of a tire).
		if (face.IsCircularFace())
		{
			// This face is a circle. See if the collision direction can hit the face (not on its edge).
			Vector3 normDotDir = unitLocalDir.DotV(VEC3V_TO_VECTOR3(face.GetUnitNormal()));
			if (normDotDir.IsGreaterThan(vMinCos))
			{
				// The collision direction can hit the face, not on its edge.
				// Find the collision point on the curved face.
				Vector3 facePoint(face.GetCurvatureCenter());
				facePoint.AddScaled(unitLocalDir,vRad);
				Vector3 facePointDot = facePoint.DotV(unitLocalDir);
				Vector3 biggestDotSelector = facePointDot.IsGreaterThanV( biggestDot );
				biggestDot = biggestDotSelector.Select( biggestDot, facePointDot );
				supportPoint = biggestDotSelector.Select( supportPoint, facePoint );
			}
		}
		else
		{
			// This face is ring-shaped. Find the collision point on the curved face.
			const Vector3 planeNormal(m_CurvedEdges[face.GetCurvedEdgeIndex(0)].GetPlaneNormal());
			Vector3 facePoint(planeNormal);
			facePoint.Cross(unitLocalDir);
			facePoint.Cross(planeNormal);
			Vector3 facePointNorm(facePoint);
			facePointNorm.NormalizeSafeV(vZero);
			Vector3 normDotDir = facePointNorm.DotV(unitLocalDir);
			Vector3 minCosSelector = normDotDir.IsGreaterThanV(vMinCos);
			Scalar innerRadius;
			innerRadius.Set(face.GetInnerRadius());
			facePoint.AddScaled(face.GetCurvatureCenter(),facePointNorm,innerRadius);
			facePoint.AddScaled(unitLocalDir,vRad);
			Vector3 facePointDot = facePoint.DotV(unitLocalDir);
			Vector3 biggestDotSelector = facePointDot.IsGreaterThanV( biggestDot );
			biggestDotSelector.And(minCosSelector);

			// Set the biggest dot product and the new support point if this edge's colliding point has the biggest dot product.
			biggestDot = biggestDotSelector.Select( biggestDot, facePointDot );
			supportPoint = biggestDotSelector.Select( supportPoint, facePoint );
		}

		// Set the biggest dot product and support point back to the ones from the loop over vertices if the face's outer radius is zero.
		// The above work for this face is wasted in that case, but this is faster than checking for a flat face first.
		Vector3 outerRadGTZeroSelector = vRad.IsGreaterThanV( vZero );
		biggestDot = outerRadGTZeroSelector.Select( biggestDotOriginal, biggestDot );
		supportPoint = outerRadGTZeroSelector.Select(supportPointOriginal, supportPoint );
	}
#endif

#if COLLIDE_WITH_VERTICES
	if (vertexIndex)
	{
		*vertexIndex = biggestDotIndex;
	}
#endif

	return RCC_VEC3V(supportPoint);
}


#if !__SPU
bool phBoundCurvedGeometry::PostLoadCompute ()
{
	return phBoundGeometry::PostLoadCompute();
}


void phBoundCurvedGeometry::Copy (const phBound* original)
{
	Assert(original->GetType() == phBound::GEOMETRY_CURVED);
	const phBoundCurvedGeometry* originalCurvedGeom = static_cast<const phBoundCurvedGeometry*>(original);

	m_CurvedFaces = rage_new phCurvedFace[originalCurvedGeom->m_NumCurvedFaces];
	sysMemCpy(m_CurvedFaces, originalCurvedGeom->m_CurvedFaces, sizeof(phCurvedFace) * originalCurvedGeom->m_NumCurvedFaces);

	m_CurvedEdges = rage_new phCurvedEdge[originalCurvedGeom->m_NumCurvedEdges];
	sysMemCpy(m_CurvedEdges, originalCurvedGeom->m_CurvedEdges, sizeof(phCurvedEdge) * originalCurvedGeom->m_NumCurvedEdges);

	m_CurvedFaceMatIndexList = rage_new u8[originalCurvedGeom->m_NumCurvedFaces];
	sysMemCpy(m_CurvedFaceMatIndexList, originalCurvedGeom->m_CurvedFaceMatIndexList, sizeof(u8) * originalCurvedGeom->m_NumCurvedFaces);

	m_NumCurvedFaces = originalCurvedGeom->m_NumCurvedFaces;
	m_NumCurvedEdges = originalCurvedGeom->m_NumCurvedEdges;

	phBoundGeometry::Copy(original);
}


void phBoundCurvedGeometry::ScaleSize (float xScale, float yScale, float zScale)
{
	// Save the bounding box extents, before phBoundGeometry::ScaleSize recomputes them incorrectly.
	Vec3V boxMin = GetBoundingBoxMin();
	Vec3V boxMax = GetBoundingBoxMax();

	Vec3V scale = Vec3V(xScale,yScale,zScale);

	// Scale the vertex locations, recompute the flat polygon normals and areas, and recompute the bound extents.
	phBoundGeometry::ScaleSize(xScale,yScale,zScale);

	// Scale the extents (they were recomputed incorrectly above when phBoundGeometry::ScaleSize called CalculateExtents).
	SetBoundingBoxMin(Scale(boxMin,scale));
	SetBoundingBoxMax(Scale(boxMax,scale));

#if COMPRESSED_VERTEX_METHOD==2
	m_BoundingBoxCenter = ComputeBoundingBoxCenter();
#endif
	// Scale the curved edges.
	for (int curvedEdgeIndex=0; curvedEdgeIndex<m_NumCurvedEdges; curvedEdgeIndex++)
	{
		m_CurvedEdges[curvedEdgeIndex].ScaleSize(xScale,yScale,zScale,this);
	}

	// Scale the curved faces.
	for (int curvedFaceIndex=0; curvedFaceIndex<m_NumCurvedFaces; curvedFaceIndex++)
	{
		m_CurvedFaces[curvedFaceIndex].ScaleSize(xScale,yScale,zScale,this,m_CurvedEdges);
	}

	// Recompute the bound extents.
	CalculateExtents();
}


void phBoundCurvedGeometry::CalculateExtents ()
{
	Vec3V xMax = phBoundCurvedGeometry::LocalGetSupportingVertex(XAXIS);
	Vec3V xMin = phBoundCurvedGeometry::LocalGetSupportingVertex(-XAXIS);
	Vec3V yMax = phBoundCurvedGeometry::LocalGetSupportingVertex(YAXIS);
	Vec3V yMin = phBoundCurvedGeometry::LocalGetSupportingVertex(-YAXIS);
	Vec3V zMax = phBoundCurvedGeometry::LocalGetSupportingVertex(ZAXIS);
	Vec3V zMin = phBoundCurvedGeometry::LocalGetSupportingVertex(-ZAXIS);

	Vec3V boundingBoxMin = Vec3V(xMin.GetX(), yMin.GetY(), zMin.GetZ());
	Vec3V boundingBoxMax = Vec3V(xMax.GetX(), yMax.GetY(), zMax.GetZ());

	SetBoundingBoxMin(boundingBoxMin);
	SetBoundingBoxMax(boundingBoxMax);

	Vec3V newCentroidOffset = ComputeBoundingBoxCenter();
	phBound::SetCentroidOffset(newCentroidOffset);
	m_RadiusAroundCentroid = Mag(boundingBoxMax - GetCentroidOffset()).Getf();

	Vec3V boundingBoxSize = GetBoundingBoxSize();

	// Compute the volume distribution from the bounding box.
	Vector3 angInertia;
	phMathInertia::FindBoxAngInertia(1.0f,boundingBoxSize.GetXf(),boundingBoxSize.GetYf(),boundingBoxSize.GetZf(),&angInertia);
	m_VolumeDistribution.SetXYZ(RCC_VEC3V(angInertia));
	m_VolumeDistribution.SetW(boundingBoxSize.GetX()*boundingBoxSize.GetY()*boundingBoxSize.GetZ());
}
#endif // !__SPU

Vector3 RotateUnitAxis (const Vector3& rotateFrom, const Vector3& axis, float angle)
{
	Vector3 cross(axis);
	cross.Cross(rotateFrom);
	cross.Normalize();
	Vector3 rotateTo(cross);
	rotateTo.Scale(sinf(angle)*rotateFrom.Mag());
	rotateTo.AddScaled(rotateFrom,cosf(angle));
	return rotateTo;
}

#if !__SPU
bool phBoundCurvedGeometry::Load_v110 (fiAsciiTokenizer& token)
{
	// Initialize the numbers of curved edges and faces.
	m_NumCurvedEdges = 0;
	m_NumCurvedFaces = 0;

	int numCurvedEdges = 0;
	int numCurvedFaces = 0;

	// See if the number of curved edges is specified.
	if (token.CheckToken("curved_edges:"))
	{
		numCurvedEdges = token.GetInt();
		Assert(numCurvedEdges < (u16)-1);
	}

	// See if the number of curved faces is specified.
	if (token.CheckToken("curved_faces:"))
	{
		numCurvedFaces = token.GetInt();
		Assert(numCurvedFaces < (u16)-1);
	}

	// Load the geometry bound.
	phBoundGeometry::Load_v110(token);

	m_NumCurvedEdges = (u16)numCurvedEdges;
	m_NumCurvedFaces = (u16)numCurvedFaces;

	// Allocate the curved edges and faces.
	AllocateCurvedEdgesAndFaces();

	// Load any curved edges.
	for (int curvedEdgeIndex=0; curvedEdgeIndex<m_NumCurvedEdges; curvedEdgeIndex++)
	{
		token.MatchToken("curved_edge");
		if (token.CheckToken("{"))
		{
			// This curved edge is in the new format, with labels and data that matches the class members.
			// Get the curved edge's vertex index numbers.
			if (!token.CheckToken("verts:"))
			{
				token.MatchToken("vertices:");
			}
			int vertIndex0 = token.GetInt();
			int vertIndex1 = token.GetInt();

			// Get the edge's normal to the plane of curvature.
			token.MatchToken("planeNormal:");
			Vector3 planeNormal;
			token.GetVector(planeNormal);

			// Get the edge's center of curvature.
			token.MatchToken("curveCenter:");
			Vector3 curveCenter;
			token.GetVector(curveCenter);

			// Eat the closing bracket.
			token.MatchToken("}");

			// Initialize the curved edge.
			SetCurvedEdge(curvedEdgeIndex,vertIndex0,vertIndex1,planeNormal,curveCenter);
		}
		else
		{
			// This curved edge is in the old format - numbers with no labels and a curve direction from which the center of curvature is computed.
			// Get the curved edge's vertex index numbers.
			int vertIndex0 = token.GetInt();
			int vertIndex1 = token.GetInt();

			// Get the edge's radius of curvature.
			float radius = token.GetFloat();

			// Get the edge's normal to the plane of curvature.
			Vector3 planeNormal;
			token.GetVector(planeNormal);

			// Get the edges direction of curvature.
			Vector3 curveDirection;
			token.GetVector(curveDirection);

			// Initialize the curved edge.
			SetCurvedEdge(curvedEdgeIndex,vertIndex0,vertIndex1,radius,planeNormal,curveDirection);
		}
	}

	// Load any curved faces.
	for (int curvedFaceIndex=0; curvedFaceIndex<m_NumCurvedFaces; curvedFaceIndex++)
	{
		token.MatchToken("curved_face");
		int numVerts,boundVertIndices[4],numCurvedEdges,curvedEdgeIndices[4],curvedEdgePolyIndices[4];
		float radius;
		if (token.CheckToken("{"))
		{
			// This curved face is in the new format, with labels.
			// Get the curved face's vertex index numbers.
			if (!token.CheckToken("numVerts:"))
			{
				token.MatchToken("numVertices:");
			}
			numVerts = token.GetInt();
			if (!token.CheckToken("verts:"))
			{
				token.MatchToken("vertices:");
			}
			for (int polyVertIndex=0; polyVertIndex<numVerts; polyVertIndex++)
			{
				boundVertIndices[polyVertIndex] = token.GetInt();
			}

			// Get the face's curvature.
			token.MatchToken("radius:");
			radius = token.GetFloat();

			// Get the curved face's curved edge index numbers (the bound's index numbers).
			token.MatchToken("numCurvedEdges:");
			numCurvedEdges = token.GetInt();
			token.MatchToken("curvedEdgeIndices:");
			for (int polyEdgeIndex=0; polyEdgeIndex<numCurvedEdges; polyEdgeIndex++)
			{
				curvedEdgeIndices[polyEdgeIndex] = token.GetInt();
			}

			// Get the index numbers of the curved edges in the curved face (the face's index numbers).
			token.MatchToken("curvedEdgePolyIndices:");
			for (int polyEdgeIndex=0; polyEdgeIndex<numCurvedEdges; polyEdgeIndex++)
			{
				curvedEdgePolyIndices[polyEdgeIndex] = token.GetInt();
			}

			// Eat the closing bracket.
			token.MatchToken("}");
		}
		else
		{
			// This curved face is in the old format - numbers with no labels.
			// Get the curved face's vertex index numbers.
			numVerts = token.GetInt();
			for (int polyVertIndex=0; polyVertIndex<numVerts; polyVertIndex++)
			{
				boundVertIndices[polyVertIndex] = token.GetInt();
			}

			// Get the face's radius of curvature.
			radius = token.GetFloat();

			// Get the curved face's curved edge index numbers (the bound's index numbers).
			numCurvedEdges = token.GetInt();
			for (int polyEdgeIndex=0; polyEdgeIndex<numCurvedEdges; polyEdgeIndex++)
			{
				curvedEdgeIndices[polyEdgeIndex] = token.GetInt();
			}

			// Get the index numbers of the curved edges in the curved face (the face's index numbers).
			for (int polyEdgeIndex=0; polyEdgeIndex<numCurvedEdges; polyEdgeIndex++)
			{
				curvedEdgePolyIndices[polyEdgeIndex] = token.GetInt();
			}
		}

		float curvature = 0.0f;
		if (radius>0)
		{
			// Compute the curvature (0 to 1) from the radius of curvature and the edge radius.
			float edgeRadius = m_CurvedEdges[curvedEdgeIndices[0]].GetRadius();
			curvature = edgeRadius*InvertSafe(radius);
		}

		// Initialize the curved face.
		SetCurvedFace(curvedFaceIndex,numVerts,boundVertIndices,numCurvedEdges,curvedEdgeIndices,curvedEdgePolyIndices,curvature);
	}


#if COMPRESSED_VERTEX_METHOD==2
	// The bounding box was set by the base class, without considering curvature. The vertices were compressed using that bounding box.
	// The bounding box has now been recomputed to include curvature, so save the vertex locations before recomputing the boudiung box.
	Vector3* vertices = Alloca(Vector3,m_NumVertices);
	for (int vertexIndex=0; vertexIndex<m_NumVertices; vertexIndex++)
	{
		vertices[vertexIndex] = VEC3V_TO_VECTOR3(GetVertex(vertexIndex));
	}
#endif

	CalculateExtents();

#if COMPRESSED_VERTEX_METHOD==2
	// Now that the vertex locations are saved and the bounding box has been recomputed, recompute the quantization and re-compress the vertices.
	CalculateQuantizationValues();
	for (int vertexIndex=0; vertexIndex<m_NumVertices; vertexIndex++)
	{
		SetVertex(vertexIndex,RCC_VEC3V(vertices[vertexIndex]));
	}
#endif

	// Return true for a successful load.
	return true;
}

#if !__FINAL && !IS_CONSOLE
bool phBoundCurvedGeometry::Save_v110 (fiAsciiTokenizer & token)
{
	// Save the number of curved edges and faces
	token.PutDelimiter("\n");
	token.PutDelimiter("curved_edges: ");
	token.Put(m_NumCurvedEdges);
	token.PutDelimiter("\n");
	token.PutDelimiter("curved_faces: ");
	token.Put(m_NumCurvedFaces);
	token.PutDelimiter("\n");

	// Save the geometry info
	token.PutDelimiter("\n");
	phBoundGeometry::Save_v110(token);

	// Save the curved edges.
	for (int i = 0; i < m_NumCurvedEdges; ++i)
	{
		token.PutDelimiter("\n");
		token.PutDelimiter("curved_edge {");

		phCurvedEdge& edge = m_CurvedEdges[i];

		// Save the curved edge's vertex index numbers.
		token.PutDelimiter("\n\tverts: ");
		token.Put(edge.GetVertexIndex(0));
		token.Put(edge.GetVertexIndex(1));

		// Save the edge's normal to the plane of curvature.
		token.PutDelimiter("\n\tplaneNormal: ");
		token.Put(edge.GetPlaneNormal());

		// Save the edge's center of curvature.
		token.PutDelimiter("\n\tcurveCenter: ");
		token.Put(edge.GetCurvatureCenter());

		// Close brackets.
		token.PutDelimiter("\n}\n");
	}

	token.PutDelimiter("\n");

	// Save the curved faces.
	for (int i = 0; i < m_NumCurvedFaces; ++i)
	{
		token.PutDelimiter("\n");
		token.PutDelimiter("curved_face {");

		phCurvedFace& face = m_CurvedFaces[i];

		// Save the curved face's vertex index numbers.
		token.PutDelimiter("\n\tnumVerts: ");
		int numVerts = face.GetNumVertices();
		token.Put(numVerts);
		token.PutDelimiter("\n\tverts: ");
		for (int j = 0; j < numVerts; ++j)
		{
			token.Put(face.GetCurvedFaceVertexIndex(j));
		}

		// Save the face's radius of curvature.
		token.PutDelimiter("\n\tradius: ");
		token.Put(face.GetOuterRadius());

		// Save the curved face's curved edge index numbers (the bound's index numbers).
		token.PutDelimiter("\n\tnumCurvedEdges: ");
		int numCurvedEdges = face.GetNumCurvedEdges();
		token.Put(numCurvedEdges);
		token.PutDelimiter("\n\tcurvedEdgeIndices: ");
		for (int j = 0; j < numCurvedEdges; ++j)
		{
			token.Put( face.GetCurvedEdgeIndex(j) );
		}

		// Save the index numbers of the curved edges in the curved face (the face's index numbers).
		token.PutDelimiter("\n\tcurvedEdgePolyIndices: ");
		for (int j = 0; j < numCurvedEdges; ++j)
		{
			token.Put(face.GetCurvedEdgePolyIndex(j));
		}

		// Close brackets.
		token.PutDelimiter("\n}\n");
	}

	return true;
}

#endif	// end of #if !__FINAL && !IS_CONSOLE

#endif	// !__SPU


/////////////////////////////////////////////////////////////////////////////////
// draw physics functions

#if __PFDRAW
void DrawCurvedEdge (const Vector3& vertexA, const Vector3& vertexB, int numSegments, const Vector3& planeNormal, const Vector3& edgeCurvatureCenter, bool solid,
						float outwardFaceCurvature, const Vector3& faceCurvatureCenter)
{
	Vector3 edgeRelVertA(vertexA);
	edgeRelVertA.Subtract(edgeCurvatureCenter);
	Vector3 edgeRelVertB(vertexB);
	edgeRelVertB.Subtract(edgeCurvatureCenter);
	float edgeAngle = (vertexA==vertexB ? 2.0f*PI : edgeRelVertA.Angle(edgeRelVertB));
	float edgeSubAngle = edgeAngle/(float)numSegments;
	Vector3 point;
	if (outwardFaceCurvature<=0.0f)
	{
		// The face containing this edge is flat.
		if (solid)
		{
			// Start drawing a solid fan centered at the edge's curvature center.
			grcBegin(drawTriFan,numSegments+2);
			grcNormal3f(planeNormal);
			grcVertex3fv(&edgeCurvatureCenter[0]);
		}
		else
		{
			// Start drawing a line strip.
			grcBegin(drawLineStrip,numSegments+1);
		}

		grcVertex3fv(&vertexA[0]);
		for (int pointIndex=1; pointIndex<numSegments; pointIndex++)
		{
			point.Add(RotateUnitAxis(edgeRelVertA,planeNormal,pointIndex*edgeSubAngle),edgeCurvatureCenter);
			grcVertex3fv(&point[0]);
		}
		
		grcVertex3fv(&vertexB[0]);
		grcEnd();
	}
	else
	{
		// The face containing this edge has outward curvature.
		Vector3 faceRelVertA(vertexA);
		faceRelVertA.Subtract(faceCurvatureCenter);
		Vector3 faceRelVertB(vertexB);
		faceRelVertB.Subtract(faceCurvatureCenter);
		float faceAngle = (vertexA==vertexB ? PI : 0.5f*faceRelVertA.Angle(faceRelVertB));
		float faceSubAngle = faceAngle/(float)numSegments;

		// Compute a set of points on the face's curved surface.
		atArray<Vector3> points;
		points.Resize((numSegments+2)*(numSegments+1)/2);
		Vector3 perpNormal(faceRelVertA);
		perpNormal.Cross(planeNormal);
		perpNormal.Normalize();
		int firstPointThisRow = 0;
		for (int rowIndex=0; rowIndex<numSegments+1; rowIndex++)
		{
			int numPointsThisRow = numSegments-rowIndex+1;
			Vector3 relEndA(RotateUnitAxis(faceRelVertA,perpNormal,rowIndex*faceSubAngle));
			relEndA.Add(faceCurvatureCenter);
			relEndA.Subtract(edgeCurvatureCenter);
			Vector3 planeOffset(planeNormal);
			planeOffset.Scale(planeNormal.Dot(relEndA));
			relEndA.Subtract(planeOffset);
			Vector3 center(edgeCurvatureCenter);
			center.Add(planeOffset);
			float subAngle = edgeAngle/(float)(numPointsThisRow-1);
			for (int pointIndex=0; pointIndex<numPointsThisRow; pointIndex++)
			{
				points[firstPointThisRow+pointIndex].Add(RotateUnitAxis(relEndA,planeNormal,pointIndex*subAngle),center);
			}

			firstPointThisRow += numPointsThisRow;
		}

		// Draw the curved edge and the edge's part of the face's curved surface.
		firstPointThisRow = 0;
		for (int rowIndex=0; rowIndex<numSegments; rowIndex++)
		{
			int numSegmentsThisRow = numSegments-rowIndex;
			if (solid)
			{
				// Start drawing a tristrip for this row.
				grcBegin(drawTriStrip,2*numSegmentsThisRow+1);
			}
			else
			{
				// Draw a curved line for this row.
				grcBegin(drawLineStrip,numSegmentsThisRow+1);
				for (int pointIndex=0; pointIndex<numSegmentsThisRow+1; pointIndex++)
				{
					grcVertex3fv(&points[firstPointThisRow+pointIndex][0]);
				}
				grcEnd();

				// Start drawing a zigzag for the cross pieces in this row.
				grcBegin(drawLineStrip,2*numSegmentsThisRow+1);
			}

			// Finish drawing the solid tristrip or the zigzag pattern.
			for (int segIndex=0; segIndex<numSegmentsThisRow; segIndex++)
			{
				grcVertex3fv(&points[firstPointThisRow+segIndex][0]);
				grcVertex3fv(&points[firstPointThisRow+segIndex+numSegmentsThisRow+1][0]);
			}
			grcVertex3fv(&points[firstPointThisRow+numSegmentsThisRow][0]);
			grcEnd();
			
			firstPointThisRow += numSegmentsThisRow+1;
		}
	}
}


void phBoundCurvedGeometry::Draw (Mat34V_In poseIn, bool colorMaterials, bool solid, int whichPolys, phMaterialFlags highlightFlags, unsigned int typeFilter, unsigned int includeFilter, unsigned int boundTypeFlags, unsigned int boundIncludeFlags) const
{
	const Matrix34& pose = RCC_MATRIX34(poseIn);

	// Draw the non-curved polygons.
	phBoundPolyhedron::Draw(poseIn,colorMaterials,solid,whichPolys,highlightFlags,typeFilter,includeFilter,boundTypeFlags,boundIncludeFlags);

	if(PFDGROUP_PolygonDensity.GetEnabled() || PFDGROUP_PrimitiveDensity.GetEnabled())
	{
		return;
	}
	// Temporary, until solid drawing of curved faces is finished.
	solid = false;

	// Draw the curved polygons.
	grcWorldMtx(pose);
	Color32 oldColor(grcCurrentColor);
	float maxDrawDistance = PFD_BoundDrawDistance.GetValue();
	float squaredDrawDistance = square(maxDrawDistance);
	float inverseDrawDistance = InvertSafe(maxDrawDistance);
	atBitSet polygonInInterest(m_NumCurvedFaces);
	atBitSet polygonFacingCamera(m_NumCurvedFaces);
	atArray<Vector3> polygonPos;
	polygonPos.Resize(m_NumCurvedFaces);
	const int numSegments = 4;
	int faceIndex;
	ScalarV thinPolyTolerance = Expt(ScalarV(PFD_ThinPolyTolerance.GetValue()) / ScalarV(V_LOG2_TO_LOG10));
	for (faceIndex=0; faceIndex<m_NumCurvedFaces; faceIndex++)
	{
		const phCurvedFace& face = m_CurvedFaces[faceIndex];

		if (whichPolys)
		{
			bool thinPoly = (whichPolys & RENDER_THIN_POLYS) && face.IsThin(this, thinPolyTolerance);
			bool badNormalPoly = (whichPolys & RENDER_BAD_NORMAL_POLYS) && face.HasBadNormal(this);
			bool badNeighborPoly = (whichPolys & RENDER_BAD_NEIGHBOR_POLYS) && face.HasBadNeighbors(this);

			if (!thinPoly && !badNormalPoly && !badNeighborPoly)
			{
				continue;
			}
		}

		bool vertsInInterest = false;
		Vector3 vertexPos,camRelWorldCenter(ORIGIN);
		int numVertices = face.GetNumVertices();
		for (int polyVertexIndex=0; polyVertexIndex<numVertices; ++polyVertexIndex)
		{
			vertexPos.Set(VEC3V_TO_VECTOR3(GetVertex(face.GetCurvedFaceVertexIndex(polyVertexIndex))));
			pose.Transform(vertexPos);
			camRelWorldCenter.Add(vertexPos);
			vertsInInterest |= (DISABLE_DRAW_GRCVIEWPORT_GETCURRENT || !grcViewport::GetCurrent() || vertexPos.Dist2(VEC3V_TO_VECTOR3(grcViewport::GetCurrentCameraPosition()))<squaredDrawDistance);
		}

		polygonInInterest.Set(faceIndex,vertsInInterest);
		if (!vertsInInterest)
		{
			continue;
		}

		// Find the center of the polygon (the average vertex position).
		Assert(numVertices==1 || numVertices==2 || numVertices==3 || numVertices==4);
		float invNumVerts = 1.0f/(float)numVertices;
		camRelWorldCenter.Scale(invNumVerts);
		polygonPos[faceIndex].Set(camRelWorldCenter);
		if (grcViewport::GetCurrent())
		{
			camRelWorldCenter.Subtract(VEC3V_TO_VECTOR3(grcViewport::GetCurrentCameraPosition()));
		}

		// See if this polygon is facing the camera.
		Vector3 worldNormal;
		pose.Transform3x3(VEC3V_TO_VECTOR3(face.GetUnitNormal()),worldNormal);
		bool facingTowardsCamera = (worldNormal.Dot(camRelWorldCenter) <= 0.0f);
		polygonFacingCamera.Set(faceIndex,facingTowardsCamera);

		// HACK_GTA4_BOUND_MATERIAL_ID - use phMaterialIndex to avoid confusion with MaterialId
		phMaterialIndex materialIndex = m_CurvedFaceMatIndexList[faceIndex];

		// Find the color.
		Color32 newColor(oldColor);
		if (highlightFlags && (MATERIALMGRFLAG.GetFlags(GetMaterialId(materialIndex)) & highlightFlags))
		{
			newColor = Color_red;
		}
		else if (colorMaterials)
		{
			newColor = MATERIALMGR.GetDebugColor(MATERIALMGR.GetMaterial(GetMaterialIdFromPartIndex(faceIndex)));
		}
		else if (highlightFlags)
		{
			newColor = Color_grey90;
		}

		// Fade the color with distance, and set the color.
		float polygonDistance = Min(SqrtfSafe(camRelWorldCenter.Mag2()),maxDrawDistance);
		newColor.SetAlpha(int(255 * (1.0f - (1.0f-PFD_BoundDistanceOpacity.GetValue())*polygonDistance*inverseDrawDistance)));
		grcColor(newColor);

		// Disable lighting, and save the previous lighting state.
		bool oldLighting = grcLighting(false);

		// Get the curvature center and the flat center of the face (they are the same for faces with no outward curvature).
		Vector3 curvatureCenter(face.GetCurvatureCenter());
		Vector3 faceNormal(VEC3V_TO_VECTOR3(face.GetUnitNormal()));
		Vector3 flatCenter(curvatureCenter);
		flatCenter.AddScaled(faceNormal,face.GetOuterRadius()*face.GetMinCosine());

		int numCurvedEdges = face.GetNumCurvedEdges();
		int numDrawnCurvedEdges = 0;
		for (int edgeIndex=0; edgeIndex<numVertices; edgeIndex++)
		{
			// See if this edge is curved.
			if (numDrawnCurvedEdges<numCurvedEdges && face.GetCurvedEdgePolyIndex(numDrawnCurvedEdges)==edgeIndex)
			{
				// Draw this curved edge.
				const phCurvedEdge& curvedEdge = m_CurvedEdges[face.GetCurvedEdgeIndex(numDrawnCurvedEdges)];
				int vertIndex0 = curvedEdge.GetVertexIndex(0);
				int vertIndex1 = curvedEdge.GetVertexIndex(1);
				Vector3 vertex0(VEC3V_TO_VECTOR3(GetVertex(vertIndex0)));
				Vector3 vertex1(VEC3V_TO_VECTOR3(GetVertex(vertIndex1)));
				Vector3 edgeCurvatureCenter(curvedEdge.GetCurvatureCenter());
				Vector3 edgePlaneNormal(curvedEdge.GetPlaneNormal());
				int numSegmentsThisEdge = (vertIndex0==vertIndex1 ? 2*numSegments : numSegments);
				float radius = (face.IsCircularFace() ? face.GetOuterRadius() : -1.0f);
				if (vertIndex0!=vertIndex1)
				{
					DrawCurvedEdge(vertex0,vertex1,numSegmentsThisEdge,edgePlaneNormal,edgeCurvatureCenter,solid,radius,curvatureCenter);
				}
				else
				{
					// This curved edge is a full circle, so draw it in two parts.
					Vector3 relVert0(vertex0);
					relVert0.Subtract(edgeCurvatureCenter);
					vertex1.Subtract(edgeCurvatureCenter,relVert0);
					DrawCurvedEdge(vertex0,vertex1,numSegmentsThisEdge,edgePlaneNormal,edgeCurvatureCenter,solid,radius,curvatureCenter);
					DrawCurvedEdge(vertex1,vertex0,numSegmentsThisEdge,edgePlaneNormal,edgeCurvatureCenter,solid,radius,curvatureCenter);
				}

				// Increment the number of curved edges drawn so far.
				numDrawnCurvedEdges++;
			}
			else
			{
				// This edge is not curved, so draw it straight.
				if (solid)
				{
					grcBegin(drawTriFan,numSegments+2);
					grcNormal3f(faceNormal);
					grcVertex3fv(&flatCenter[0]);
				}
				else
				{
					grcBegin(drawLines,2);
				}

				grcVertex3fv(&GetVertex(face.GetCurvedFaceVertexIndex(edgeIndex))[0]);
				grcVertex3fv(&GetVertex(face.GetCurvedFaceVertexIndex((edgeIndex+1)%numVertices))[0]);
				grcEnd();
			}

			// Restore the lighting mode.
			grcLighting(oldLighting);
		}
	}

	if (PFD_DrawBoundMaterialNames.WillDraw())
	{
		for (faceIndex=0; faceIndex<m_NumCurvedFaces; faceIndex++)
		{
			if (polygonInInterest.IsSet(faceIndex) && polygonFacingCamera.IsSet(faceIndex))
			{
				// Write the polygon index number on the screen at the average location of its vertices.
				grcDrawLabelf(polygonPos[faceIndex],MATERIALMGR.GetMaterial(GetMaterialIdFromPartIndex(faceIndex)).GetName());
			}
		}
	}

	if ((PFD_BvhPolygonIndices.Begin() && GetType() == phBound::BVH) || (PFD_NonBvhPolygonIndices.Begin() && GetType() != phBound::BVH))
	{
		for (faceIndex=0; faceIndex<m_NumCurvedFaces; faceIndex++)
		{
			if (polygonInInterest.IsSet(faceIndex) && polygonFacingCamera.IsSet(faceIndex))
			{
				// Write the polygon index number on the screen at the average location of its vertices.
				char polygonIndexText[8];
				polygonIndexText[7] = '\0';
				formatf(polygonIndexText,7,"%i",faceIndex);
				grcDrawLabelf(polygonPos[faceIndex],polygonIndexText);
			}
		}

		GetType() == phBound::BVH ? PFD_BvhPolygonIndices.End() : PFD_NonBvhPolygonIndices.End();
	}

	grcColor(oldColor);
}


void phBoundCurvedGeometry::DrawNormals (Mat34V_In poseIn, int normalType, int whichPolys, float length, unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter)) const
{
	const Matrix34& pose = RCC_MATRIX34(poseIn);
	grcWorldMtx(pose);
	Vector3 cameraDirection;
	float squaredDrawDistance = square(PFD_BoundDrawDistance.GetValue());
	ScalarV thinPolyTolerance = Expt(ScalarV(PFD_ThinPolyTolerance.GetValue()) / ScalarV(V_LOG2_TO_LOG10));
	for (int polyIndex1 = GetNumPolygons() - 1; polyIndex1 >= 0; polyIndex1--)
	{
		if (whichPolys)
		{
			bool thinPoly = (whichPolys & RENDER_THIN_POLYS) && m_Polygons[polyIndex1].IsThin(this, thinPolyTolerance);
			bool badNormalPoly = (whichPolys & RENDER_BAD_NORMAL_POLYS) && m_Polygons[polyIndex1].HasBadNormal(this);
			bool badNeighborPoly = (whichPolys & RENDER_BAD_NEIGHBOR_POLYS) && m_Polygons[polyIndex1].HasBadNeighbors(this);

			if (!thinPoly && !badNormalPoly && !badNeighborPoly)
			{
				continue;
			}
		}

		Vector3 normal;
		pose.Transform3x3(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polyIndex1)), normal);

		bool vertsInInterest = false;
		for (int vertex = 0; vertex < POLY_MAX_VERTICES && !vertsInInterest; ++vertex)
		{
			pose.Transform(VEC3V_TO_VECTOR3(GetVertex(m_Polygons[polyIndex1].GetVertexIndex(vertex))),cameraDirection);
			if (!grcViewport::GetCurrent() || cameraDirection.Dist2(VEC3V_TO_VECTOR3(grcViewport::GetCurrentCameraPosition())) < squaredDrawDistance)
			{
				vertsInInterest = true;
			}
		}

		if (!vertsInInterest)
		{
			continue;
		}

		if (normalType == FACE_NORMALS)
		{
			// Find the average position of the polygon's vertices.
			Vector3 center(VEC3V_TO_VECTOR3(GetVertex(m_Polygons[polyIndex1].GetVertexIndex(0))));
			center.Add(VEC3V_TO_VECTOR3(GetVertex(m_Polygons[polyIndex1].GetVertexIndex(1))));
			center.Add(VEC3V_TO_VECTOR3(GetVertex(m_Polygons[polyIndex1].GetVertexIndex(2))));
			center.Multiply(VEC3_THIRD);

			// Make the arrow end one meter out along the polygon normal.
			Vector3 arrowEnd(center);
			arrowEnd.AddScaled(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polyIndex1)), length);

			// Draw a 1m long arrow from the average position of the polygon's vertices, 1m out along the normal.
			pfDrawArrow(center, arrowEnd);
		}
		else
		{
			Assert(normalType == EDGE_NORMALS);

			const phPolygon *poly1 = &GetPolygon(polyIndex1);
			int neighborIndex;
			int polyVertexCnt = POLY_MAX_VERTICES;
			for(neighborIndex = polyVertexCnt - 1; neighborIndex >= 0; neighborIndex--)
			{
				int polyIndex2 = poly1->GetNeighboringPolyNum(neighborIndex);
				if((polyIndex1 > polyIndex2))
				{
					continue;
				}

				// Find the direction of the normal.
				Vector3 vecNormal(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polyIndex1)));
				if(polyIndex2 != (u16)(-1))
				{
					Vector3 vecTemp(VEC3V_TO_VECTOR3(GetPolygonUnitNormal(polyIndex2)));
					vecNormal.Add(vecTemp);
					vecNormal.Normalize();
				}

				// Find the center of the edge.
				Vector3 vecCenter;
				int nPolyVertexIdx1 = neighborIndex;
				int nPolyVertexIdx2 = nPolyVertexIdx1 < polyVertexCnt - 1 ? nPolyVertexIdx1 + 1 : 0;
				int nBoundVertexIdx1 = poly1->GetVertexIndex(nPolyVertexIdx1);
				int nBoundVertexIdx2 = poly1->GetVertexIndex(nPolyVertexIdx2);
				vecCenter.Average(VEC3V_TO_VECTOR3(GetVertex(nBoundVertexIdx1)), VEC3V_TO_VECTOR3(GetVertex(nBoundVertexIdx2)));

				vecNormal.Scale(length);
				vecNormal.Add(vecCenter);
				pfDrawArrow(vecCenter, vecNormal);
			}
		}
	}
}


#endif	// end of #if __PFDRAW

#endif // USE_GEOMETRY_CURVED

} // namespace rage

