//
// phbound/primitives.h
//
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_PRIMITIVES_H
#define PHBOUND_PRIMITIVES_H

#include "vectormath/scalarv.h"
#include "phcore/constants.h"
#include "phcore/materialmgr.h"
#include "data/resource.h"
#include "vector/vector4.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

// The only two combinations this class supports are a) POLY_MAX_VERTICES == 3 with non-32-bit polygon indices, and b) POLY_MAX_VERTICES == 4
//   with 32-bit polygon indices.  In the former case these classes can be resourced, in the latter not.
CompileTimeAssert((POLY_MAX_VERTICES == 3) != POLYGON_INDEX_IS_U32);

#define VERT_NORMAL_BITS 1
#define VERT_NORMAL_MASK 0x8000
#define VERT_INDEX_MASK 0x7FFF

namespace rage {

class phBoundPolyhedron;
class phSegment;
class phSegmentV;

enum PrimitiveType
{
	PRIM_TYPE_POLYGON	=	0,
	PRIM_TYPE_SPHERE	=	1,
	PRIM_TYPE_CAPSULE	=	2,
	PRIM_TYPE_BOX		=	3,
	PRIM_TYPE_CYLINDER	=	4,
	PRIM_TYPE_COUNT
};

class phPrimSphere;
class phPrimCapsule;
class phPrimBox;
class phPrimCylinder;
class phPolygon;

// Simple wrapper around a phPolygon where we hijack three bits from the area member to store what kind of primitive this actually is.  This steals a little
//   bit of precision from the area, but it should still have more than enough precision for what it's being used for.
// But it sort of doesn't... /RP
class phPrimitive
{
public:
	// TEMPORARY FUNCTION
	void ClearHighTypeBit()
	{
		u8 value = AreaAndPrimitiveType.pad3;
		AreaAndPrimitiveType.pad3 = (value & 0xFB);
	}

	// TEMPORARY FUNCTION
	bool CheckHighTypeBit() const
	{
		return (AreaAndPrimitiveType.pad3 & 0x4) == 1;
	}

	PrimitiveType GetType() const
	{
		return (PrimitiveType)(AreaAndPrimitiveType.pad3 & 0x7);
	}

	void SetType(PrimitiveType primitiveType)
	{
		u8 value = AreaAndPrimitiveType.pad3;
		AreaAndPrimitiveType.pad3 = (value & 0xF8) | (u8)(primitiveType);
	}

	DECLARE_PLACE(phPrimitive);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	phPrimBox& GetBox();
	phPrimCapsule& GetCapsule();
	phPrimCylinder& GetCylinder();
	phPolygon& GetPolygon();
	phPrimSphere& GetSphere();

	const phPrimBox& GetBox() const;
	const phPrimCapsule& GetCapsule() const;
	const phPrimCylinder& GetCylinder() const;
	const phPolygon& GetPolygon() const;
	const phPrimSphere& GetSphere() const;

	bool IsBig(const phBoundPolyhedron* bound, ScalarV_In tolerance) const;

	const char* GetTypeName() const
	{
		switch(GetType())
		{
		case PRIM_TYPE_BOX:
			return "phPrimBox";
		case PRIM_TYPE_CAPSULE:
			return "phPrimCapsule";
		case PRIM_TYPE_CYLINDER:
			return "phPrimCylinder";
		case PRIM_TYPE_POLYGON:
			return "phPolygon";
		case PRIM_TYPE_SPHERE:
			return "phPrimSphere";
		default:
			Assertf(false, "phPrimitive::GetTypeName type %i has no name.", GetType());
			return NULL;
		}
	}

	union
	{ 
		// PURPOSE: Area of this polygon.
		float m_Area;
		struct
		{
#if __BE
			u8 pad0, pad1, pad2, pad3;											// padding for big endian
#else
			u8 pad3, pad2, pad1, pad0;											// padding for little endian
#endif
		} AreaAndPrimitiveType;
	};
	u8 m_Pad[12];																// padding to line up with largest primitive (phPolygon)
};


inline phPrimBox& phPrimitive::GetBox()
{
	Assert(GetType() == PRIM_TYPE_BOX);
	return *reinterpret_cast<phPrimBox*>(this);
}
inline phPrimCapsule& phPrimitive::GetCapsule()
{
	Assert(GetType() == PRIM_TYPE_CAPSULE);
	return *reinterpret_cast<phPrimCapsule*>(this);
}
inline phPrimCylinder& phPrimitive::GetCylinder()
{
	Assert(GetType() == PRIM_TYPE_CYLINDER);
	return *reinterpret_cast<phPrimCylinder*>(this);
}
inline phPolygon& phPrimitive::GetPolygon()
{
	Assert(GetType() == PRIM_TYPE_POLYGON);
	return *reinterpret_cast<phPolygon*>(this);
}
inline phPrimSphere& phPrimitive::GetSphere()
{
	Assert(GetType() == PRIM_TYPE_SPHERE);
	return *reinterpret_cast<phPrimSphere*>(this);
}

inline const phPrimBox& phPrimitive::GetBox() const
{
	Assert(GetType() == PRIM_TYPE_BOX);
	return *reinterpret_cast<const phPrimBox*>(this);
}
inline const phPrimCapsule& phPrimitive::GetCapsule() const
{
	Assert(GetType() == PRIM_TYPE_CAPSULE);
	return *reinterpret_cast<const phPrimCapsule*>(this);
}
inline const phPrimCylinder& phPrimitive::GetCylinder() const
{
	Assert(GetType() == PRIM_TYPE_CYLINDER);
	return *reinterpret_cast<const phPrimCylinder*>(this);
}
inline const phPolygon& phPrimitive::GetPolygon() const
{
	Assert(GetType() == PRIM_TYPE_POLYGON);
	return *reinterpret_cast<const phPolygon*>(this);
}
inline const phPrimSphere& phPrimitive::GetSphere() const
{
	Assert(GetType() == PRIM_TYPE_SPHERE);
	return *reinterpret_cast<const phPrimSphere*>(this);
}

//=============================================================================
// phPolygon
// 
// PURPOSE
//   phPolygon is the polygon class used by physics bounds, vertex and 
//   neighboring polygon index numbers, and methods for polygon-polygon 
//   intersections and polygon-segment intersections.
// <FLAG Component>
//

class phPolygon
{
public:
	typedef POLYGON_INDEX Index;

	DECLARE_PLACE(phPolygon);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif

	//=========================================================================
	// Construction

	// PURPOSE: Default constructor.
	// NOTE: Used for non-resources or contained resourced objects if sm_ResourceConstructorEnabled is true.
	phPolygon();

	// PURPOSE: Constructor from a resource.  Empty since no data needs fixup.
	phPolygon(datResource & rsc);


	//=========================================================================
	// Resources

	// PURPOSE: Turns on/off the resource constructor version of the default constructor.
	static void EnableResourceConstructor(bool enable);

#if __DEV || __TOOL
	// PURPOSE: Turns on/off the printing of debug messages for polygons
	static void DisableMessages(bool disabled=true)						{ sm_MessagesEnabled = !disabled; }

	// PURPOSE: Return whether debug messages should be printed
	static bool MessagesEnabled()										{ return sm_MessagesEnabled; }
#else
	// PURPOSE: dummy version of DisableMessages so that calls don't have to be wrapped in #if __DEV || __TOOL
	static void DisableMessages(bool UNUSED_PARAM(disabled)=true) { }
#endif


	//=========================================================================
	// Initialization

	// PURPOSE: Initialize this polygon as a triangle.
	// PARAMS
	//   i0 - Index of vertex 0.
	//   i1 - Index of vertex 1.
	//   i2 - Index of vertex 2.
	//   vert0 - vertex location for vertex 0
	//   vert1 - vertex location for vertex 1
	//   vert2 - vertex location for vertex 2
    void InitTriangle(phPolygon::Index i0, phPolygon::Index i1, phPolygon::Index i2, Vec3V_In vert0, Vec3V_In vert1, Vec3V_In vert2 ASSERT_ONLY(, bool assertOnZeroArea = true));

	void SetVertexIndices(phPolygon::Index vertexIndex0, phPolygon::Index vertexIndex1, phPolygon::Index vertexIndex2)
	{
		SetVertexIndex(0, vertexIndex0);
		SetVertexIndex(1, vertexIndex1);
		SetVertexIndex(2, vertexIndex2);
	}


	//=========================================================================
	// Accessors

	// PURPOSE: Copy another polygons data into this polygon.
	void Set(const phPolygon & poly);

	// PURPOSE: Compute and returns the normal using the given vertex locations.
	// PARAMS:
	//	vertex0 - the polygon's first vertex
	//	vertex1 - the polygon's second vertex
	//	vertex2 - the polygon's third vertex
	// RETURN:	the normal of this polygon
	// NOTES:	The polygon normal is a vector perpendicular to its plane using the right-hand rule winding in order of increasing polygon vertex number.
	static Vec3V_Out ComputeNonUnitNormal (Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2);

	// PURPOSE: Compute and return the non-unit normal of this polygon.
	// PARAMS:
	//	boundVertices - pointer to the bound's list of vertices
	// RETURN:	the non-unit normal of this polygon
	// NOTES:	The polygon normal is a vector perpendicular to its plane using the right-hand rule winding in order of increasing polygon vertex number.
	Vec3V_Out ComputeNonUnitNormal (const Vec3V* boundVertices) const;

	// PURPOSE: Compute and return the normal of unit length of this polygon.
	// PARAMS:
	//	vertex0 - the polygon's first vertex
	//	vertex1 - the polygon's second vertex
	//	vertex2 - the polygon's third vertex
	// RETURN:	the normal of unit length of this polygon
	// NOTES:	The polygon unit normal is the  vector of length 1 perpendicular to the polygon's plane using the right-hand rule winding in order of increasing polygon vertex number.
	//Vec3V_Out ComputeUnitNormal (Vec::V3Param128 vertex0, Vec::V3Param128 vertex1, Vec::V3Param128 vertex2) const;
	Vec3V_Out ComputeUnitNormal (Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2) const;

	// PURPOSE: Compute and return the normal of unit length of this polygon.
	// PARAMS:
	//	boundVertices - pointer to the bound's list of vertices
	// RETURN:	the normal of unit length of this polygon
	// NOTES:	The polygon unit normal is the vector of length 1 perpendicular to the polygon's plane using the right-hand rule winding in order of increasing polygon vertex number.
	Vec3V_Out ComputeUnitNormal (const Vec3V* boundVertices) const;

	// PURPOSE: Return the area of this polygon.
	float GetArea() const;
	ScalarV_Out GetAreaV() const;

	// PURPOSE: Set the area of this polygon.
	// PARAMS:
	//	area - the new area of this polygon in square meters
	// NOTES:
	//	This is used by the polygon to restore its area after setting the material index, because the material index is stored in the least significant eight bits
	//	of the area. This makes the area imprecise, which works because it's only used for wind and water interactions, not for collision detection.
	void SetArea (float area);

	// PURPOSE: Get the polygon's vertex index that is not either of the two given indices.
	// PARAMS:
	//	vertexIndexA - one of the polygon's vertex index numbers
	//	vertexIndexB - another of the polygon's vertex index numbers
	// RETURN: the polygon's vertex index that is not either of the two given indices
	phPolygon::Index GetOtherVertexIndex (u32 vertexIndexA, u32 vertexIndexB) const;

	const phPrimitive& GetPrimitive() const;
	phPrimitive& GetPrimitive();

	// PURPOSE: Change the neighbor index number that has the given old neighbor index to the new neighbor index.
	// PARAMS:
	//	oldNeighborIndex - the old neighbor index number to change
	//	newNeighborIndex - the new value to replace the old neighbor index
	// RETURN:	true if the given old neighbor index was in this polygon, false if it was not
	bool ChangeNeighborIndex (Index oldNeighborIndex, Index newNeighborIndex);

	// PURPOSE: Returns the polygon index of the i-th polygon neighbor to this polygon.
	// PARAMS
	//   i - The number of the polygon to return.
	phPolygon::Index GetNeighboringPolyNum(int i) const;

	// PURPOSE: Set the polygon index of the i-th polygon neighbor to this polygon.
	// PARAMS
	//   i - The number of the polygon to set.
	//   p - The index of the polygon that is neighbor i.
	void SetNeighboringPolyNum (int i, phPolygon::Index p);

	// PURPOSE: Set a vertex index directly.
	// NOTES: Requires eventual recalculation of normal, etc.
	void SetIndex(const int i, const phPolygon::Index vert);

	// PURPOSE: Rotate the indices in this polygon forward one (0->1, 1->2, 2->3, 3->4).
	// NOTES: Does not recalculate polygon data (normal, etc), so that must be done by the user.
	void Rotate();

	// PURPOSE: Removes the given vertex from the polygon.
	// NOTES: Does not recalculate polygon data (normal, etc), so that must be done by the user.
	void RemoveVertex(int i);


	//=========================================================================
	// Utility functions

	// PURPOSE: Calculate the normal and area of this polygon using the given vertices.
	// PARAMS
	//   vertices - The array of vertices that this polygons vertex indices point into.
	void CalculateNormalAndArea(const Vec3V* vertices);

	// PURPOSE: Calculate the normal and area of this polygon using the given vertices.
	// PARAMS
	//   vert0..vert2 - The vertices of the triangle.
	void CalculateNormalAndArea(Vec3V_In vert0, Vec3V_In vert1, Vec3V_In vert2 ASSERT_ONLY(, bool assertOnZeroArea = true));

	// PURPOSE: Use the vertex positions to compute the area of this triangle.
	// PARAMS:
	//	vertices - this list of vertex positions in the bound
	void ComputeArea (const Vec3V* vertices);

	void ComputeArea (Vec3V_In vert0, Vec3V_In vert1, Vec3V_In vert2);

	// PURPOSE: Returns true if the vertices in this polygon are unique by index.
	// NOTES: This function verifies uniqueness of the indices and not the positions of the vertices.
	bool VerifyUniqueVertices() const;

	// PURPOSE: Returns true if the vertices in this polygon are unique by index.
	// NOTES: This function verifies uniqueness of the indices and not the positions of the vertices.

	// PURPOSE
	//   Is this polygon "thin".  A thin polygon is one that has an aspect ratio (max dimension/min dimension)
	//   of more than some threshold value (BAD_ASPECT_RATIO).
	bool IsThin(const phBoundPolyhedron* bound, ScalarV_In tolerance = ScalarV(V_TEN) * ScalarV(V_TEN)) const;

	// PURPOSE: Returns true if the normal of this polygon deviates from unit length by more than MAX_DEVIATION.
	bool HasBadNormal(const phBoundPolyhedron* bound, ScalarV_In tolerance = ScalarV(V_FLT_SMALL_2)) const;

	// PURPOSE: Returns true if a neighbor of this polygon has the opposite normal.
	// PARAMS:
	//	spew - if true, print out more detailed info about the errant polygon
	//	fix - if true, disconnect the neighbor connection		
	bool HasBadNeighbors(phBoundPolyhedron* bound, const char* debugName = NULL, bool spew = false, phPolygon::Index primIndex = phPolygon::Index(~0), bool fix = false);
	bool HasBadNeighbors(const phBoundPolyhedron* bound, const char* debugName = NULL, bool spew = false) const;

	// PURPOSE: Count the number of vertices in the given list that are in this polygon.
	// PARAMS:
	//	vertices - a list of vertex index numbers
	//	numVertices - the number of vertices in the given list
	//  containedIndices - a list of indices that gets set inside the function, it records the indices of all the vertices are contained (optional)
	// RETURN: the number of vertices in the given list that are in this polygon
	int ContainsVertices (const int* vertices, int numVertices, int *containedIndices = NULL) const;

	// PURPOSE: Get this polygon's index for the neighbor (or edge, same thing) from the given edge normal.
	// PARAMS:
	//	localEdgeNormal - a vector perpendicular to one of the polygon's edges
	//	polyVertices - list of the polygon's vertex locations
	// NOTES:
	//	1.	The second argument can be got by calling phBoundPolyhedron::GetPolygonVertices.
	//	2.	If the given edge normal is the polygon normal, then the choice is arbitrary and the last index will be returned (3 or 4).
	int GetNeighborFromEdgeNormal (Vec3V_In localEdgeNormal, const Vec3V* polyVertices) const;

	// PURPOSE: Get the neighbor index for the edge that a given point is most outside of
	// PARAMS:
	//	exteriorPoint - a point exterior to this polygon
	//	polyVertices - list of the polygon's vertex locations
	//  polyNormal - this polygon's face normal
	//	    -- (Assumed to have been generated via or in the same handedness as phPolygon::ComputeNonUnitNormal(polyVertices[0], polyVertices[1], polyVertices[2]))
	//      -- Needn't be normalized
	// NOTES:
	//	1.	The second argument can be got by calling phBoundPolyhedron::GetPolygonVertices.
	//	2.	The third argument can be had by calling phPolygon::ComputeNonUnitNormal(polyVertices[0], polyVertices[1], polyVertices[2]).
	//  3.	Can actually be used with any point - if non-exterior -1 will be returned
	int GetNeighborFromExteriorPoint (Vec3V_In exteriorPoint, const Vec3V* polyVertices, Vec3V_In polyNormal) const;

	// PURPOSE: Find the index number of the neighboring polygon that contains the two given vertices.
	// PARAMS:
	//	vertexA - the first shared vertex
	//	vertexB - the second shared vertex
	// RETURN: the index number of the neighboring polygon that contains the two given vertices
	phPolygon::Index FindNeighborWithVertices (int vertexA, int vertexB) const;

	// PURPOSE: Find the index number of the neighboring polygon that contains the given vertex, that is not the given neighboring polygon.
	// PARAMS:
	//	vertex - the shared vertex
	//	otherNeighbor - optional index number of the neighbor to ignore
	// RETURN: the index number of the neighboring polygon that contains the given vertex
	phPolygon::Index FindNeighborWithVertex (int vertex, int otherNeighbor=BAD_INDEX) const;

	// PURPOSE: Find the index number of the neighboring polygon that contains the given vertex, that is not the given neighboring polygon,
    //          but in the other direction as FindNeighborWithVertex.
	// PARAMS:
	//	vertex - the shared vertex
	//	otherNeighbor - optional index number of the neighbor to ignore
	// RETURN: the index number of the neighboring polygon that contains the given vertex
    // NOTES: This functions searches in the direction opposite from FindNeighborWithVertex...if it searches CW, this function searches CCW.
    phPolygon::Index FindNeighborWithVertex2 (int vertex, int otherNeighbor=BAD_INDEX) const;

	//=========================================================================
	// Intersection functions
	// TODO: Document
	int DetectSegmentUndirected (const Vec3V* vertices, Vec4V_In segA, Vec4V_In segB) const;
	int DetectSegmentUndirected (Vec3V_In v0, Vec3V_In v1, Vec3V_In v2, Vec4V_In segA, Vec4V_In segB) const;

	int DetectSegmentDirected (const Vec3V* vertices, Vec3V_In segA, Vec3V_In segB) const;
	int DetectSegmentDirected (Vec3V_In v0, Vec3V_In v1, Vec3V_In v2, Vec3V_In segA, Vec3V_In segB) const;

	static inline bool SegEdgeCheckUndirected(Vec3V_In p0, Vec3V_In p1,
								 Vec4V_In segA, Vec4V_In RaySeg, Vec4V_In edgeNormalCross);

	static inline bool SegEdgeCheckUndirected(Vec3V_In p0, Vec3V_In p1,
								 Vec3V_In segA, Vec4V_In RaySeg, Vec4V_In edgeNormalCross);

	static inline bool SegEdgeCheckUndirectedPreCalc(Vec3V_In p0, Vec3V_In p1,
								 Vec4V_In RaySeg, Vec4V_In edgeNormalCross, Vec4V_In dispEdges);

	static inline bool SegEdgeCheckDirected(Vec3V_In p0, Vec3V_In p1,
								 Vec4V_In segA, Vec4V_In RaySeg, Vec4V_In edgeNormalCross);

	static inline bool SegEdgeCheckDirected(Vec3V_In p0, Vec3V_In p1,
								 Vec3V_In segA, Vec4V_In RaySeg, Vec4V_In edgeNormalCross);

	static inline bool SegEdgeCheckDirectedPreCalc(Vec3V_In p0, Vec3V_In p1,
								 Vec4V_In RaySeg, Vec4V_In edgeNormalCross, Vec4V_In dispEdges);

	static __forceinline bool DetectPointInsideBoundingBox(Vec3V_In v0, Vec3V_In v1, Vec3V_In v2, Vec3V_In vPointToTest, ScalarV_In sMargin);

protected:

	//=========================================================================
	// Static data

	// PURPOSE: True when the system is currently in a resource constructor.
	// NOTES: Potential thread un-safety.
	static bool sm_InResourceConstructor;

#if __DEV || __TOOL
	static bool sm_MessagesEnabled;
#endif


	//=========================================================================
	// Data

	union
	{ 
		// PURPOSE: Area of this polygon.
		float m_Area;
		struct
		{
#if __BE
			u8 pad0, pad1, pad2, pad3;											// padding for big endian
#else
			u8 pad3, pad2, pad1, pad0;											// padding for little endian
#endif
		} AreaAndPrimitiveType;
	};

	// PURPOSE: Indices of the vertices that make up this polygon.
	// NOTES:
	//   These indices are into a vertex array that is not owned or known by this
	//   polygon, but instead passed in by various functions when needed.
    phPolygon::Index m_VertexIndices[POLY_MAX_VERTICES];
public:
	__forceinline phPolygon::Index GetVertexIndex(u32 vertex) const { return (m_VertexIndices[vertex] & VERT_INDEX_MASK); };
	__forceinline void SetVertexIndex(u32 vertex, phPolygon::Index index) { m_VertexIndices[vertex] = ((index) | (m_VertexIndices[vertex] & VERT_NORMAL_MASK)); };
	__forceinline u32 GetVertexNormalCode(u32 vertex) const { return (u32)(m_VertexIndices[vertex] & VERT_NORMAL_MASK); };
	__forceinline void SetVertexNormalCode(u32 vertex, u32 normalCode) { m_VertexIndices[vertex] = ((phPolygon::Index)(normalCode & VERT_NORMAL_MASK) | (m_VertexIndices[vertex] & VERT_INDEX_MASK)); };

	// PURPOSE: Indices of the polygons that are neighbors of this polygon.
	// NOTES:
	//   These indices are into a polygon array that is not owned or known by this polygon.
	phPolygon::Index m_NeighboringPolygons[POLY_MAX_VERTICES];
};
CompileTimeAssert(sizeof(phPolygon) == sizeof(phPrimitive));

#define BAD_POLY_INDEX ((phPolygon::Index)(-1))


CompileTimeAssert(sizeof(phPolygon)==16);


//=============================================================================
// Implementations


inline phPolygon::phPolygon ()
{
#if !__SPU
	// This doesn't compile and isn't needed on the PS3 SPU.
	if (sm_InResourceConstructor)
	{
		// don't change any data loaded from the resource
		return;
	}
#endif

	for (int i=0; i<POLY_MAX_VERTICES; i++)
	{
		//SetVertexIndex(i, 0);
		// Like in InitTriangle, we actually DO want to clear the other bits of the vertex index field rather than preserving undefined values
		// - Further work should set the vertex normal codes if desired
		m_VertexIndices[i] = 0;
		m_NeighboringPolygons[i] = (phPolygon::Index)(-1);
	}

	GetPrimitive().SetType(PRIM_TYPE_POLYGON);
}


inline phPolygon::phPolygon (datResource& UNUSED_PARAM(rsc))
{
}


inline void phPolygon::EnableResourceConstructor(bool enable)
{
	sm_InResourceConstructor = enable;
}


__forceinline Vec3V_Out phPolygon::ComputeNonUnitNormal (Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2)
{

	// Save locals.
	Vec3V v_vertex0(vertex0);
	Vec3V v_vertex1(vertex1);
	Vec3V v_vertex2(vertex2);

	// Cross two edge vectors to make the normal vector.
	Vec3V side12,side10,nonUnitNormal;
	side12 = Subtract(v_vertex2, v_vertex1);
	side10 = Subtract(v_vertex0, v_vertex1);
	nonUnitNormal = Cross(side12, side10);

	// Return the (unnormalized) normal vector.
	return nonUnitNormal;
}


__forceinline Vec3V_Out phPolygon::ComputeNonUnitNormal (const Vec3V* boundVertices) const
{
	return ComputeNonUnitNormal(boundVertices[GetVertexIndex(0)], boundVertices[GetVertexIndex(1)], boundVertices[GetVertexIndex(2)]);
}

__forceinline Vec3V_Out phPolygon::ComputeUnitNormal (Vec3V_In vertex0, Vec3V_In vertex1, Vec3V_In vertex2) const
{
	// Compute the inverse of twice the area. Later, this will be a member in place of the area.
	ScalarV inverseTwiceArea;
	inverseTwiceArea.Set(m_Area);
	inverseTwiceArea = Add(inverseTwiceArea,inverseTwiceArea);
	inverseTwiceArea = Invert(inverseTwiceArea);

	// Compute the unnormalized normal vector.
	Vec3V unitNormal = Vec3V(ComputeNonUnitNormal(vertex0,vertex1,vertex2));

	// Normalize the normal and return it.
	unitNormal = Scale(unitNormal,inverseTwiceArea);
	return unitNormal;
}


__forceinline Vec3V_Out phPolygon::ComputeUnitNormal (const Vec3V* boundVertices) const
{
	return ComputeUnitNormal(boundVertices[GetVertexIndex(0)],
	                         boundVertices[GetVertexIndex(1)],
	                         boundVertices[GetVertexIndex(2)]);
}


inline float phPolygon::GetArea() const
{
	return m_Area;
}

inline ScalarV_Out phPolygon::GetAreaV() const
{
	return ScalarVFromF32(m_Area);
}

inline void phPolygon::SetArea (float area)
{
	//Store the poly type before we trash it by setting the area.
	phPrimitive * pPrimitive = &GetPrimitive();
	const PrimitiveType type=pPrimitive->GetType();
	//Set the area (this will trash the poly type due to the union)
	m_Area = area;
	//Reset the poly type (not stored in particularly significant bits so the area won't be affected too much).
	pPrimitive->SetType(type);
}

__forceinline phPolygon::Index phPolygon::GetOtherVertexIndex (u32 vertexIndexA, u32 vertexIndexB) const
{
#if 0
	int aIndex = (vertexIndexA==GetVertexIndex(0) ? 0 : (vertexIndexA==GetVertexIndex(1) ? 1 : 2));
	int bIndex = (vertexIndexB==GetVertexIndex(0) ? 0 : (vertexIndexB==GetVertexIndex(1) ? 1 : 2));
	Assertf(aIndex+bIndex>0 && aIndex+bIndex<=3 && aIndex!=bIndex,"phPolygon::GetOtherVertexIndex failed with input %i and %i, vertices %i, %i and %i",
																		vertexIndexA,vertexIndexB,GetVertexIndex(0),GetVertexIndex(1),GetVertexIndex(2));
	return GetVertexIndex(3-aIndex-bIndex);
#else
	FastAssert(vertexIndexA == GetVertexIndex(0) || vertexIndexA == GetVertexIndex(1) || vertexIndexA == GetVertexIndex(2));
	FastAssert(vertexIndexB == GetVertexIndex(0) || vertexIndexB == GetVertexIndex(1) || vertexIndexB == GetVertexIndex(2));
//	if(vertexIndexA != GetVertexIndex(0) && vertexIndexA != GetVertexIndex(1) && vertexIndexA != GetVertexIndex(2))
//	{
//		Displayf("A: %d / %d, %d, %d", vertexIndexA, GetVertexIndex(0), GetVertexIndex(1), GetVertexIndex(2));
//	}
//	if(vertexIndexB != GetVertexIndex(0) && vertexIndexB != GetVertexIndex(1) && vertexIndexB != GetVertexIndex(2))
//	{
//		Displayf("B: %d / %d, %d, %d", vertexIndexB, GetVertexIndex(0), GetVertexIndex(1), GetVertexIndex(2));
//	}
	phPolygon::Index returnValue = (phPolygon::Index)(((u32)GetVertexIndex(0) + (u32)GetVertexIndex(1) + (u32)GetVertexIndex(2)) - vertexIndexA - vertexIndexB);
	return returnValue;
#endif
}

inline const phPrimitive& phPolygon::GetPrimitive() const
{
	return *reinterpret_cast<const phPrimitive*>(this);
}
inline phPrimitive& phPolygon::GetPrimitive()
{
	return *reinterpret_cast<phPrimitive*>(this);
}

inline bool phPolygon::ChangeNeighborIndex (Index oldNeighborIndex, Index newNeighborIndex)
{
	// Get the number of neighbors.
	const int numNeighbors =  3;

	// Loop over all the neighbors, and see if any matches the given old neighbor index.
	for (int polyNeighborIndex=0; polyNeighborIndex<numNeighbors; polyNeighborIndex++)
	{
		if (m_NeighboringPolygons[polyNeighborIndex]==oldNeighborIndex)
		{
			// Replace the neighbor index and return true.
			m_NeighboringPolygons[polyNeighborIndex] = (phPolygon::Index)newNeighborIndex;
			return true;
		}
	}

	// The given old neighbor index was not in this polygon, so return false.
	return false;
}

inline phPolygon::Index phPolygon::GetNeighboringPolyNum(int i) const
{
	FastAssert(i >= 0);
	FastAssert(i < POLY_MAX_VERTICES);
	return m_NeighboringPolygons[i];
}


inline void phPolygon::SetNeighboringPolyNum(int i, phPolygon::Index p)
{
	FastAssert(i >= 0);
	FastAssert(i < POLY_MAX_VERTICES);
	m_NeighboringPolygons[i] = p;
}


inline void phPolygon::SetIndex(const int i, const phPolygon::Index vert)
{
	SetVertexIndex(i, vert);
}


inline int phPolygon::ContainsVertices (const int* vertices, int numVertices, int *containedIndices) const
{
	int numInPolygon = 0;
	int numPolyVerts = POLY_MAX_VERTICES;
	for (int givenVertexIndex=0; givenVertexIndex<numVertices; givenVertexIndex++)
	{
		for (int polyVertexIndex=0; polyVertexIndex<numPolyVerts; polyVertexIndex++)
		{
			if ((Index)vertices[givenVertexIndex]==GetVertexIndex(polyVertexIndex))
			{
				if(containedIndices)
				{
					containedIndices[numInPolygon] = givenVertexIndex;
				}
				numInPolygon++;
				break;
			}
		}
	}

	return numInPolygon;
}


inline phPolygon::Index phPolygon::FindNeighborWithVertices (int vertexA, int vertexB) const
{
	int numNeighbors = POLY_MAX_VERTICES;
	for (int neighborIndex=0; neighborIndex<numNeighbors; neighborIndex++)
	{
		int firstVertex = GetVertexIndex(neighborIndex);
		int secondVertex = GetVertexIndex((neighborIndex+1)%numNeighbors);
		if ((firstVertex==vertexA && secondVertex==vertexB) || (firstVertex==vertexB && secondVertex==vertexA))
		{
			return m_NeighboringPolygons[neighborIndex];
		}
	}

	return (phPolygon::Index)BAD_INDEX;
}




inline phPolygon::Index phPolygon::FindNeighborWithVertex (int vertex, int otherNeighbor) const
{
	int numNeighbors = POLY_MAX_VERTICES;
	for (int neighborIndex=0; neighborIndex<numNeighbors; neighborIndex++)
	{
		int nextNeighborIndex = (neighborIndex+1)%numNeighbors;
		int polyVertex = GetVertexIndex(nextNeighborIndex);
		if (polyVertex==vertex)
		{
			phPolygon::Index neighborPoly = m_NeighboringPolygons[neighborIndex];
			return (neighborPoly!=(phPolygon::Index)otherNeighbor ? neighborPoly : m_NeighboringPolygons[nextNeighborIndex]);
		}
	}

	return (phPolygon::Index)BAD_INDEX;
}

__forceinline bool phPolygon::DetectPointInsideBoundingBox(Vec3V_In v0, Vec3V_In v1, Vec3V_In v2, Vec3V_In vPointToTest, ScalarV_In sMargin)
{
	Vec3V vMargin(sMargin.Getf(), sMargin.Getf(), sMargin.Getf());
	Vec3V vMin = Min(v0, v1);
	vMin = Min(vMin, v2);
	vMin = Subtract(vMin, vMargin);
	Vec3V vMax = Max(v0, v1);
	vMax = Max(vMax, v2);
	vMax = Add(vMax, vMargin);

	return (IsGreaterThanAll(vPointToTest, vMin) && IsLessThanAll(vPointToTest, vMax));
}

//.............................

class phPrimSphere
{
public:
	phPrimSphere()
	{
		GetPrimitive().SetType(PRIM_TYPE_SPHERE);
	}

	void SetCenterIndexAndRadius(phPolygon::Index centerIndex, float radius)
	{
		SetCenterIndex(centerIndex);
		m_Radius = radius;
	}

	void SetCenterIndex(phPolygon::Index centerIndex)
	{
		m_CenterIndex = centerIndex;
	}

	phPolygon::Index GetCenterIndex() const
	{
		return m_CenterIndex;
	}

	float GetRadius() const
	{
		return m_Radius;
	}

	__forceinline ScalarV_Out GetRadiusV() const
	{
		return ScalarVFromF32(m_Radius);
	}

	inline const phPrimitive& GetPrimitive() const
	{
		return *reinterpret_cast<const phPrimitive*>(this);
	}
	inline phPrimitive& GetPrimitive()
	{
		return *reinterpret_cast<phPrimitive*>(this);
	}

	DECLARE_PLACE(phPrimSphere);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(phPrimSphere);

#if __BE
		STRUCT_FIELD((u32 &) m_CenterIndex);		// 0..3, this is a nasty hack
#else // __BE
		STRUCT_FIELD((u32 &) m_PrimType);			// 0..3, this is a nasty hack
#endif // __BE
		STRUCT_FIELD(m_Radius);						// 4..7
		STRUCT_CONTAINED_ARRAY(m_Pad1);				// 8..15

		STRUCT_END();
	}
#endif

private:

#if __BE
	phPolygon::Index m_CenterIndex;	// 0..1
	u8 m_Pad0;						// 2
	u8 m_PrimType;					// 3
#else // __BE
	u8 m_PrimType;					// 0
	u8 m_Pad0;						// 1
	phPolygon::Index m_CenterIndex;	// 2..3
#endif // __BE
	float m_Radius;					// 4..7
	u8 m_Pad1[8];					// 8..15
};
CompileTimeAssert(sizeof(phPrimSphere) == sizeof(phPrimitive));


class phPrimCapsule
{
public:
	phPrimCapsule()
	{
		GetPrimitive().SetType(PRIM_TYPE_CAPSULE);
	}

	void SetEndIndicesAndRadius(phPolygon::Index endIndex0, phPolygon::Index endIndex1, float radius)
	{
		SetEndIndices(endIndex0, endIndex1);
		m_Radius = radius;
	}

	void SetEndIndices(phPolygon::Index endIndex0, phPolygon::Index endIndex1)
	{
		m_EndIndex0 = endIndex0;
		m_EndIndex1 = endIndex1;
	}

	phPolygon::Index GetEndIndex0() const
	{
		return m_EndIndex0;
	}

	phPolygon::Index GetEndIndex1() const
	{
		return m_EndIndex1;
	}

	float GetRadius() const
	{
		return m_Radius;
	}

	__forceinline ScalarV_Out GetRadiusV() const
	{
		return ScalarVFromF32(m_Radius);
	}

	inline const phPrimitive& GetPrimitive() const
	{
		return *reinterpret_cast<const phPrimitive*>(this);
	}
	inline phPrimitive& GetPrimitive()
	{
		return *reinterpret_cast<phPrimitive*>(this);
	}

	DECLARE_PLACE(phPrimCapsule);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(phPrimCapsule);

#if __BE
		STRUCT_FIELD((u32 &) m_EndIndex0);			// 0..3, this is a nasty hack
#else // __BE
		STRUCT_FIELD((u32 &) m_PrimType);			// 0..3, this is a nasty hack
#endif // __BE
		STRUCT_FIELD(m_Radius);						// 4..7
		STRUCT_FIELD(m_EndIndex1);					// 8..9
		STRUCT_CONTAINED_ARRAY(m_Pad1);				// 11..15

		STRUCT_END();
	}
#endif

private:

#if __BE
	phPolygon::Index m_EndIndex0;	// 0..1
	u8 m_Pad0;						// 2
	u8 m_PrimType;					// 3
#else // __BE
	u8 m_PrimType;					// 0
	u8 m_Pad0;						// 1
	phPolygon::Index m_EndIndex0;	// 2..3
#endif // __BE
	float m_Radius;					// 4..7
	phPolygon::Index m_EndIndex1;	// 8..9
	u8 m_Pad1[6];					// 11..15
};
CompileTimeAssert(sizeof(phPrimCapsule) == sizeof(phPrimitive));


class phPrimBox
{
public:
	phPrimBox()
	{
		GetPrimitive().SetType(PRIM_TYPE_BOX);
	}

	void SetVertexIndices(phPolygon::Index vertexIndex0, phPolygon::Index vertexIndex1, phPolygon::Index vertexIndex2, phPolygon::Index vertexIndex3)
	{
		m_VertexIndices[0] = vertexIndex0;
		m_VertexIndices[1] = vertexIndex1;
		m_VertexIndices[2] = vertexIndex2;
		m_VertexIndices[3] = vertexIndex3;
	}

	phPolygon::Index GetVertexIndex(int vertexIndexIndex) const
	{
		return m_VertexIndices[vertexIndexIndex];
	}

	inline const phPrimitive& GetPrimitive() const
	{
		return *reinterpret_cast<const phPrimitive*>(this);
	}
	inline phPrimitive& GetPrimitive()
	{
		return *reinterpret_cast<phPrimitive*>(this);
	}

	DECLARE_PLACE(phPrimBox);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(phPrimBox);

#if __BE
		STRUCT_FIELD((u32 &) m_Pad0[0]);			// 0..3, this is a nasty hack
#else // __BE
		STRUCT_FIELD((u32 &) m_PrimType);			// 0..3, this is a nasty hack
#endif // __BE
		STRUCT_CONTAINED_ARRAY(m_VertexIndices);	// 4..11
		STRUCT_CONTAINED_ARRAY(m_Pad1);				// 12..15

		STRUCT_END();
	}
#endif

private:

#if __BE
	u8 m_Pad0[3];							// 0..2
	u8 m_PrimType;							// 3
#else // __BE
	u8 m_PrimType;							// 0
	u8 m_Pad0[2];							// 1..3
#endif // __BE
	phPolygon::Index m_VertexIndices[4];	// 4..11
	u8 m_Pad1[4];							// 12..15
};
CompileTimeAssert(sizeof(phPrimBox) == sizeof(phPrimitive));




class phPrimCylinder
{
public:
	phPrimCylinder()
	{
		GetPrimitive().SetType(PRIM_TYPE_CYLINDER);
	}

	void SetEndIndicesAndRadius(phPolygon::Index endIndex0, phPolygon::Index endIndex1, float radius)
	{
		SetEndIndices(endIndex0, endIndex1);
		m_Radius = radius;
	}

	void SetEndIndices(phPolygon::Index endIndex0, phPolygon::Index endIndex1)
	{
		m_EndIndex0 = endIndex0;
		m_EndIndex1 = endIndex1;
	}

	phPolygon::Index GetEndIndex0() const
	{
		return m_EndIndex0;
	}

	phPolygon::Index GetEndIndex1() const
	{
		return m_EndIndex1;
	}

	float GetRadius() const
	{
		return m_Radius;
	}

	__forceinline ScalarV_Out GetRadiusV() const
	{
		return ScalarVFromF32(m_Radius);
	}

	inline const phPrimitive& GetPrimitive() const
	{
		return *reinterpret_cast<const phPrimitive*>(this);
	}
	inline phPrimitive& GetPrimitive()
	{
		return *reinterpret_cast<phPrimitive*>(this);
	}

	DECLARE_PLACE(phPrimCylinder);

#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s)
	{
		STRUCT_BEGIN(phPrimCapsule);

#if __BE
		STRUCT_FIELD((u32 &) m_EndIndex0);			// 0..3, this is a nasty hack
#else // __BE
		STRUCT_FIELD((u32 &) m_PrimType);			// 0..3, this is a nasty hack
#endif // __BE
		STRUCT_FIELD(m_Radius);						// 4..7
		STRUCT_FIELD(m_EndIndex1);					// 8..9
		STRUCT_CONTAINED_ARRAY(m_Pad1);				// 11..15

		STRUCT_END();
	}
#endif

private:

#if __BE
	phPolygon::Index m_EndIndex0;	// 0..1
	u8 m_Pad0;						// 2
	u8 m_PrimType;					// 3
#else // __BE
	u8 m_PrimType;					// 0
	u8 m_Pad0;						// 1
	phPolygon::Index m_EndIndex0;	// 2..3
#endif // __BE
	float m_Radius;					// 4..7
	phPolygon::Index m_EndIndex1;	// 8..9
	u8 m_Pad1[6];					// 11..15
};
CompileTimeAssert(sizeof(phPrimCylinder) == sizeof(phPrimitive));


#if __DECLARESTRUCT
inline void phPrimitive::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(phPrimitive);

	switch(GetType())
	{
	case PRIM_TYPE_POLYGON:
		{
			GetPolygon().DeclareStruct(s);
			break;
		}
	case PRIM_TYPE_SPHERE:
		{
			GetSphere().DeclareStruct(s);
			break;
		}
	case PRIM_TYPE_CAPSULE:
		{
			GetCapsule().DeclareStruct(s);
			break;
		}
	case PRIM_TYPE_BOX:
		{
			GetBox().DeclareStruct(s);
			break;
		}
	default:
		{
			Assert(GetType() == PRIM_TYPE_CYLINDER);
			GetCylinder().DeclareStruct(s);
			break;
		}
	}

	STRUCT_END();
}
#endif

}

#endif // PHBOUND_PRIMITIVES_H
