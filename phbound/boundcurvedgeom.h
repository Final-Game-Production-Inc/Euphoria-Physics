//
// phbound/boundgeom.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDCURVEDGEOM_H
#define PHBOUND_BOUNDCURVEDGEOM_H

/////////////////////////////////////////////////////////////////
// external defines

#include "boundgeom.h"
#if __SPU
#include "vector/vector3_consts_spu.cpp"
#endif

namespace rage {

#if USE_GEOMETRY_CURVED
// PURPOSE: The maximum numbers of curved edges and faces that can be copied to the PS3 SPU for collision detection.
// NOTES:	There are warnings to avoid exceeding these numbers when curved geometry bounds are created, but no assert.
//			If these numbers are exceeded, some collisions can be missed on PS3.
#define SPU_MAX_CURVED_EDGES	128
#define SPU_MAX_CURVED_FACES	128

class phBoundCurvedGeometry;

class phCurvedEdge
{
	friend class phBoundCurvedGeometry;

public:
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	void Init (int vertIndex0, int vertIndex1, float radius, Vector3::Param planeNormal, Vector3::Param center);
	Vector3 GetCurvatureCenter () const;
	Vector3 GetPlaneNormal () const;
	float GetRadius () const;
	int GetVertexIndex (int edgeVertIndex) const;
	void ScaleSize (float xScale, float yScale, float zScale, const phBoundCurvedGeometry* curvedBound);
	bool IsCircular () const;
	Vector3 IsCircularV () const;

protected:

	// PURPOSE: the center of curvature of the edge
	// NOTES:
	//	1.	The center of curvature must be in the plane defined by m_PlaneNormal and the edge's vertices.
	Vector3 m_CurvatureCenter;

	// PURPOSE: unit vector perpendicular to the curved edge's plane
	Vector3 m_PlaneNormal;

	// PURPOSE: the radius of curvature of the curved edge
	float m_Radius;

	// PURPOSE: the polyhedron bound's index numbers of the vertices on this edge (it is possible to use only one)
	int m_VertexIndices[2];

	// PURPOSE: formerly the number of vertices on this edge, no longer needed, don't want to break resources
	int m_UnusedInt;
};


inline Vector3 phCurvedEdge::GetCurvatureCenter () const
{
	return m_CurvatureCenter;
}

inline Vector3 phCurvedEdge::GetPlaneNormal () const
{
	return m_PlaneNormal;
}

inline float phCurvedEdge::GetRadius () const
{
	return m_Radius;
}

inline int phCurvedEdge::GetVertexIndex (int edgeVertIndex) const
{
	FastAssert(edgeVertIndex==0 || edgeVertIndex==1);
	return m_VertexIndices[edgeVertIndex];
}

inline bool phCurvedEdge::IsCircular () const
{
	return (m_VertexIndices[0]==m_VertexIndices[1]);
}

inline Vector3 phCurvedEdge::IsCircularV () const
{
	return (m_VertexIndices[0]==m_VertexIndices[1] ? VEC3_ANDW : VEC3_ZERO);
}


class ALIGNAS(32) phCurvedFace : public phPolygon
{
	friend class phBoundCurvedGeometry;

public:
	// PURPOSE: Default constructor.
	// NOTE: Used for non-resources or contained resourced objects if sm_ResourceConstructorEnabled is true.
	phCurvedFace();

	// PURPOSE: Constructor from a resource.  Empty since no data needs fixup.
	phCurvedFace (datResource & rsc);

	DECLARE_PLACE(phCurvedFace);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	// PURPOSE: Create a curved face for a curved geometry bound.
	// PARAMS:
	//	numVerts - the number of vertices in the curved face
	//	vertIndices - list of vertex indices
	//	vertices - list of vertex locations in the bound
	//	numCurvedEdges - the number of curved edges in the curved face (it can also have straight edges)
	//	curvedEdgeIndices - list of curved edge index numbers, indexed by the bound
	//	curvedEdgePolyIndices - list of curved edge index numbers, indexed by the face
	//	curvedEdges - list of curved edges
	//	curvature - the amount of curvature from 0 (flat) to 1 (half circle or half sphere)
#if COMPRESSED_VERTEX_METHOD == 0
	void Init (int numVerts, int* vertIndices, const Vector3* vertices, int numCurvedEdges, int* curvedEdgeIndices, int* curvedEdgePolyIndices, const phCurvedEdge* curvedEdges, float curvature);
#else
	void Init (int numVerts, int* vertIndices, Vector3::Param v0, Vector3::Param v1, Vector3::Param v2, int numCurvedEdges, int* curvedEdgeIndices, int* curvedEdgePolyIndices, const phCurvedEdge* curvedEdges, float curvature);
	void Init (int numVerts, int* vertIndices, Vector3::Param v0, Vector3::Param v1, Vector3::Param v2, Vector3::Param v3, int numCurvedEdges, int* curvedEdgeIndices, int* curvedEdgePolyIndices, const phCurvedEdge* curvedEdges, float curvature);
#endif

	// PURPOSE: Find out whether this curved face is a curved circle or a curved ring.
	// PARAMS:
	//	curvedEdges - the list of curved edges in the curved geometry bound
	// RETURN:	true if this face is a curved circle, false if it is a curved ring
	bool ComputeIsCircularFace (const phCurvedEdge* curvedEdges);

	// PURPOSE: Tell whether this curved face is circular, like the side of a wheel.
	// RETURN:	whether or not this curved face is circular
	// NOTES:	All curved faces are either circular, like the side of a wheel, or ring-shaped, like the contact surface of a tire.
	bool IsCircularFace () const;

	// PURPOSE: Get the center of curvature of this curved face.
	// RETURN:	the center of curvature of this curved face
	Vector3::Param GetCurvatureCenter () const;

	float GetInnerRadius () const;
	float GetOuterRadius () const;
	float GetMinCosine () const;
	int GetNumCurvedEdges () const;
	int GetCurvedEdgeIndex (int curvedIndex) const;
	int GetCurvedEdgePolyIndex (int curvedIndex) const;

	// <COMBINE phPolygon::GetNumVerts>
	int GetNumVertices () const;

	int GetCurvedFaceVertexIndex (int polyVertIndex) const;

	// PURPOSE: Get the unit-length normal vector for this curved face.
	// RETURN:	the unit-length normal vector
	Vec3V_Out GetUnitNormal () const;

	// PURPOSE: Set the center of curvature of this curved face.
	// PARAMS:
	//	center - the center of curvature of this curved face
	void SetCurvatureCenter (Vector3::Ref center);

	// PURPOSE: Change the size of the curved face.
	// PARAMS:
	//	xScale - scale factor for the x direction
	//	yScale - scale factor for the y direction
	//	zScale - scale factor for the z direction
	//	boundVertices - pointer to the bound's vertex positions
	//	curvedEdges - pointer to the bound's list of curved edges
	void ScaleSize (float xScale, float yScale, float zScale, const phBoundCurvedGeometry* curvedBound, const phCurvedEdge* curvedEdges);

protected:

	// PURPOSE: the center of curvature of the face
	Vector3 m_CurvatureCenter;

	// PURPOSE: unit normal vector for this curved face
	Vec3V m_UnitNormal;

	// PURPOSE: the radius of curvature of the face
	float m_OuterRadius;

	// PURPOSE: the distance from the center of curvature about which to rotate the local center of curvature
	// NOTES:
	//	1.	This only applies to ring-shaped curved faces. The center of curvature of any mid-point on the face is displaced directly away from
	//		the center of curvature (the midpoint between the edge's centers) by the inner radius.
	float m_InnerRadius;

	// PURPOSE: the cosine between the local normal and the midpoint normal at the edge of the curved face
	float m_MinCosine;

	// PURPOSE: the curved polyhedron bound's index numbers for the curved edges in this face
	phPolygon::Index m_CurvedEdgeIndices[4];

	// PURPOSE: this face's index numbers for the curved edges in this face
	// NOTES:
	//	1.	m_CurvedEdgePolyIndices[i]==i if all the edges are curved
	phPolygon::Index m_CurvedEdgePolyIndices[4];

	// PURPOSE: the number of curved edges in this face
	int m_NumCurvedEdges;

	// PURPOSE: index number of the last vertex if this is a curved quad
	phPolygon::Index m_FourthVertex;

	// PURPOSE: Tell whether the curved face is circular, like the side of a wheel, or ring-shaped, like the contact surface of a tire.
	bool m_IsCircularFace;

	datPadding<13> m_Pad;
};



inline phCurvedFace::phCurvedFace ()
{
	m_IsCircularFace = false;
}

inline phCurvedFace::phCurvedFace (datResource& UNUSED_PARAM(rsc))
{
}

inline bool phCurvedFace::IsCircularFace () const
{
	return m_IsCircularFace;
}

inline Vector3::Param phCurvedFace::GetCurvatureCenter () const
{
	return m_CurvatureCenter;
}

inline int phCurvedFace::GetNumVertices () const
{
	return (m_VertexIndices[1]==(phPolygon::Index)BAD_INDEX ? 1 : (m_VertexIndices[2]==(phPolygon::Index)BAD_INDEX ? 2 : (m_FourthVertex==(phPolygon::Index)BAD_INDEX ? 3 : 4)));
}


inline int phCurvedFace::GetCurvedFaceVertexIndex (int polyVertIndex) const
{
	if (polyVertIndex==3)
	{
		return m_FourthVertex;
	}

	return phPolygon::GetVertexIndex(polyVertIndex);
}


inline Vec3V_Out phCurvedFace::GetUnitNormal () const
{
	return m_UnitNormal;
}

inline float phCurvedFace::GetInnerRadius () const
{
	return m_InnerRadius;
}

inline float phCurvedFace::GetOuterRadius () const
{
	return m_OuterRadius;
}

inline float phCurvedFace::GetMinCosine () const
{
	return m_MinCosine;
}

inline int phCurvedFace::GetNumCurvedEdges () const
{
	return m_NumCurvedEdges;
}

inline int phCurvedFace::GetCurvedEdgeIndex (int curvedIndex) const
{
	return m_CurvedEdgeIndices[curvedIndex];
}

inline int phCurvedFace::GetCurvedEdgePolyIndex (int curvedIndex) const
{
	return m_CurvedEdgePolyIndices[curvedIndex];
}


/////////////////////////////////////////////////////////////////
// phBoundCurvedGeometry

/*
PURPOSE
	A class to represent a physics bound with generalized vertex locations and polygons, including curved polygons and curved edges.
*/


class phBoundCurvedGeometry : public phBoundGeometry
{
public:
	// PURPOSE: constructor
	phBoundCurvedGeometry ();

	// PURPOSE: Resource constructor.
	phBoundCurvedGeometry (datResource& rsc);

	// PURPOSE: virtual destructor
	PH_NON_SPU_VIRTUAL ~phBoundCurvedGeometry ();

#if __DECLARESTRUCT
	PH_NON_SPU_VIRTUAL void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	void Init (int numVerts, int numMaterials, int numPolys, int numCurvedEdges, int numCurvedPolys);
	void AllocateCurvedEdgesAndFaces ();

	// PURPOSE: Set a curved edge in this geometry bound.
	// PARAMS:
	//	curvedEdgeIndex - the index number of the new curved edge
	//	vertIndex0 - the curved edge's first vertex index number
	//	vertIndex1 - the curved edge's second vertex index number
	//	radius - the radius of curvature
	//	planeNormal - the unit normal vector out of the plane of curvature
	//	curveDirection - the unit normal vector from the line connecting the two vertices pointing in the direction of the curved edge
	// NOTES:	When the two vertex index numbers are the same, this curved edge is a circle.
	void SetCurvedEdge (int curvedEdgeIndex, int vertIndex0, int vertIndex1, float radius, Vector3::Param planeNormal, Vector3::Param curveDirection);

	// PURPOSE: Set a curved edge in this geometry bound.
	// PARAMS:
	//	curvedEdgeIndex - the index number of the new curved edge
	//	vertIndex0 - the curved edge's first vertex index number
	//	vertIndex1 - the curved edge's second vertex index number
	//	planeNormal - the unit normal vector out of the plane of curvature
	//	curveCenter - the center of curvature
	// NOTES:	When the two vertex index numbers are the same, this curved edge is a circle.
	//			When the two vertex index numbers are not the same, the edge curves out in the direction that is the cross product between
	//				the vector from vertex 0 to vertex 1 and the plane normal.
	void SetCurvedEdge (int curvedEdgeIndex, int vertIndex0, int vertIndex1, Vector3::Param planeNormal, Vector3::Param curveCenter);

	void SetCurvedFace (int curvedFaceIndex, int numVerts, int* vertIndices, int numCurvedEdges, int* curvedEdgeIndices, int* curvedEdgePolyIndices, float outerRadius);

	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
	Vec3V_Out LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 localDirection, int* vertexIndex=NULL) const;
	Vec3V_Out LocalGetSupportingVertex (Vec::V3Param128 localDirection) const;

#if !__SPU
	PH_NON_SPU_VIRTUAL bool PostLoadCompute ();

	PH_NON_SPU_VIRTUAL void Copy (const phBound* original);


	// <COMBINE phBoundGeometry::ScaleSize>
	PH_NON_SPU_VIRTUAL void ScaleSize (float xScale, float yScale, float zScale);

	PH_NON_SPU_VIRTUAL bool Load_v110 (fiAsciiTokenizer & token);								// load, ascii, v1.10
#if !__FINAL && !IS_CONSOLE
	PH_NON_SPU_VIRTUAL bool Save_v110 (fiAsciiTokenizer & token);								// save, ascii, v1.10
#endif
#endif

	// PURPOSE: Get the number of curved faces in this geometry bound.
	// RETURN:	the number of curved faces in this geometry bound
	int GetNumCurvedFaces () const;

	// PURPOSE: Get the number of curved edges in this geometry bound.
	// RETURN:	the number of curved edges in this geometry bound
	int GetNumCurvedEdges () const;

	// PURPOSE: Get a reference to a curved face.
	// PARAMS:
	//	curvedFaceIndex - the index number of the curved face
	// RETURN: a reference to a curved face
	const phCurvedFace& GetCurvedFace (int curvedFaceIndex) const;

	// PURPOSE: Get a reference to a curved edge.
	// PARAMS:
	//	curvedEdgeIndex - the index number of the curved edge
	// RETURN: a reference to a curved edge
	const phCurvedEdge& GetCurvedEdge (int curvedEdgeIndex) const;

	// PURPOSE: Set the curved edge pointer.
	// PARAMS:
	//	curvedEdges - pointer to the list of curved edges
	// NOTES:	This is used for copying bounds to the SPU.
	void SetCurvedEdges (phCurvedEdge* curvedEdges);

	// PURPOSE: Set the curved face pointer.
	// PARAMS:
	//	curvedFaces - pointer to the list of curved faces
	// NOTES:	This is used for copying bounds to the SPU.
	void SetCurvedFaces (phCurvedFace* curvedFaces);

	inline phMaterialMgr::Id GetMaterialIdFromPartIndex (int partIndex) const;

	////////////////////////////////////////////////////////////
	// visualization
#if __PFDRAW
	PH_NON_SPU_VIRTUAL void Draw (Mat34V_In pose, bool colorMaterials=false, bool solid=false, int whichPolys=ALL_POLYS, phMaterialFlags highlightFlags=0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
	PH_NON_SPU_VIRTUAL void DrawNormals (Mat34V_In pose, int normalType=FACE_NORMALS, int whichPolys=ALL_POLYS, float length=1.0f, unsigned int typeFilter=0xffffffff, unsigned int includeFilter = 0xffffffff) const;
#endif // __PFDRAW

protected:
#if !__SPU
	// PURPOSE: Calculate the bounding box and bounding sphere.
	// NOTES:	CalculateGeomExtents() is a public version of this method for geometry bounds.
	PH_NON_SPU_VIRTUAL void CalculateExtents ();
#endif
protected:
	phCurvedFace* m_CurvedFaces;
	phCurvedEdge* m_CurvedEdges;

	// PURPOSE: list of index numbers into this bound's list of material ids, one for each polygon
	// NOTES:	these are stored in phPolygon when POLY_MAX_VERTICES==4
	u8* m_CurvedFaceMatIndexList;

	int m_NumCurvedFaces;
	int m_NumCurvedEdges;

	u8 m_Pad[12];

#if !__SPU
	PH_NON_SPU_VIRTUAL bool HasOpenEdges() const {return false;}
#endif
};


inline int phBoundCurvedGeometry::GetNumCurvedFaces () const
{
	return m_NumCurvedFaces;
}

inline int phBoundCurvedGeometry::GetNumCurvedEdges () const
{
	return m_NumCurvedEdges;
}

inline const phCurvedFace& phBoundCurvedGeometry::GetCurvedFace (int curvedFaceIndex) const
{
	return m_CurvedFaces[curvedFaceIndex];
}

inline const phCurvedEdge& phBoundCurvedGeometry::GetCurvedEdge (int curvedEdgeIndex) const
{
	return m_CurvedEdges[curvedEdgeIndex];
}

inline void phBoundCurvedGeometry::SetCurvedEdges (phCurvedEdge* curvedEdges)
{
	m_CurvedEdges = curvedEdges;
}

inline void phBoundCurvedGeometry::SetCurvedFaces (phCurvedFace* curvedFaces)
{
	m_CurvedFaces = curvedFaces;
}

inline phMaterialMgr::Id phBoundCurvedGeometry::GetMaterialIdFromPartIndex (int partIndex) const
{
	if(partIndex < GetNumPolygons())
	{
		return phBoundGeometry::GetMaterialIdFromPartIndex(partIndex);
	}
	
	int materialIndex = 0;
	int curvedPartIndex = partIndex - GetNumPolygons();
	if(curvedPartIndex < GetNumCurvedFaces())
	{
#if __SPU
		// The bound stores a list of material index numbers with the same length as the list of polygons.
		materialIndex = sysDmaGetUInt8(uint64_t(&m_CurvedFaceMatIndexList[curvedPartIndex]), DMA_TAG(11));
#else
		// The bound stores a list of material index numbers with the same length as the list of polygons.
		materialIndex = m_CurvedFaceMatIndexList[curvedPartIndex];
#endif
	}

	return phBoundGeometry::GetMaterialId(materialIndex);
}

#endif // USE_GEOMETRY_CURVED
} // namespace rage

#endif
