//
// phbound/boundbox.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDBOX_H
#define PHBOUND_BOUNDBOX_H

////////////////////////////////////////////////////////////////
// external defines

#include "boundpolyhedron.h"

#include "phcore/materialmgr.h"
#include "vector/geometry.h"

namespace rage {

// PURPOSE
//   A class to represent a physics bound in the shape of a rectangular prism.  A phBoundBox is specified by its length, width and height.  The principal axes of the box 
//   are always the local axes of the bound.  Boxes can be created used polygons (via a phBoundPolyhedron) but using a phBoundBox allows for greater efficiency, both in 
//   calculating intersections and in memory usage.
//
// <FLAG Component>

class phBoundBox : public phBound
{

public:

	phBoundBox (const Vector3& size = Vector3(1.0f,1.0f,1.0f));

	PH_NON_SPU_VIRTUAL ~phBoundBox ();

	/////////////////////////////////////////////////////////////
	// manipulators

	// PURPOSE: Set the size of the box along all three axes.
	// PARAMS:
	//	size - the new size of the box
	void SetBoxSize (Vec3V_In size);

	void SetCentroidOffset (Vec3V_In  offset);								// set the centroid to be at offset
	void ShiftCentroidOffset (Vec3V_In offset);								// move the centroid (and verts) by offset
	void CalculateExtents ();												// calculate box and sphere extents

	// PURPOSE:	Get the size of this box bound.
	// RETURN:	the size of this box bound
	Vec3V_Out GetBoxSize () const;

	// <COMBINE phBound::GetMaterialIdFromPartIndex>
	phMaterialMgr::Id GetMaterialIdFromPartIndex (int UNUSED_PARAM(partIndex)) const { return GetPrimitiveMaterialId(); }
	void SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex UNUSED_PARAM(materialIndex)=-1) { SetPrimitiveMaterialId(materialId); }

#if !__SPU

#if __PFDRAW
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = phBound::ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
#endif

	virtual const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) const { return MATERIALMGR.GetMaterial(GetPrimitiveMaterialId()); }
	virtual phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const { return GetPrimitiveMaterialId(); }
	const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) { return MATERIALMGR.GetMaterial(GetPrimitiveMaterialId()); }
#else
	phMaterialMgr::Id GetMaterialId (int UNUSED_PARAM(index)) const
	{
		return GetPrimitiveMaterialId();
	}
#endif

	////////////////////////////////////////////////////////////
	// intersection test functions

#if !__SPU
	// <COMBINE phBound::CanBecomeActive>
	virtual bool CanBecomeActive () const;
#endif // !__SPU

	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
	void LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 localDirection, SupportPoint & sp) const;

	////////////////////////////////////////////////////////////
	// resources
	phBoundBox (datResource & rsc);											// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	static const bool ms_RequiresDestructorOnStack = false;

	static void FindBoxPlaneRegion (const Vector3& halfWidth, const Vector3& point, int& type, int& index);
	static void FindIsectBoxPlaneRegion (float borderT, int borderIndex, const Vector3& halfWidths, float radius, Vector3& startPos, Vector3& endPos, Vector3& axisPart,
											int* regionType, int* regionIndex, bool directed);

protected:
	///////////////////////////////////////////////////////////
	// box data now in base class

	///////////////////////////////////////////////////////////
	// load / save
	bool Load_v110 (fiAsciiTokenizer & token);

#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);
#endif

	///////////////////////////////////////////////////////////
public:
	static const u16 FaceFromEdge[12][2];						// the indices of the faces that join at a particular edge
	static const u16 CornerFromEdge[12][2];						// the indices of the corners that make up a particular edge
	static const Vector3 FaceNormals[6];									// the normals for each of the faces (normalized)
	static const Vector3 CornerNormals[8];									// the normals coming out of the corners (NOT NORMALIZED)
	static const Vector3 UnitCorners[8];									// the corners of a unit cube (e.g. <0.5f,0.5f,0.5f>)
};


FORCE_INLINE_SIMPLE_SUPPORT void phBoundBox::LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 localDirection, SupportPoint & sp) const
{
	const VecBoolV isPositive = IsGreaterThan(Vec3V(localDirection), Vec3V(V_ZERO));
	const Vec3V margin(GetMarginV());
	sp.m_vertex = SelectFT(isPositive, GetBoundingBoxMin() + margin, GetBoundingBoxMax() - margin);
	sp.m_index = isPositive.GetIntrin128ConstRef();
}

// Find the part of the box (corner, edge or face with the index number) that the given point
// outside the box is closest to.
inline void phBoundBox::FindBoxPlaneRegion (const Vector3& halfWidth, const Vector3& point, int& type, int& index)
{
	// Find the region index of the closest element, out of 27 (8 vertices, 12 edges, 6 faces and 1 interior,
	// in the order specified in the switch statement below).
	float pointElement,halfWidthElement;
	int region=0,trinary=1;
	for (int axisIndex=0; axisIndex<3; axisIndex++)
	{
		pointElement = point[axisIndex];
		halfWidthElement = halfWidth[axisIndex];
		region += trinary*(pointElement>-halfWidthElement ? (pointElement>=halfWidthElement ? 2 : 1) : 0);
		trinary *= 3;
	}

	switch (region)
	{
	case 0: type = GEOM_VERTEX; index = 6; break;		// corner 6
	case 1: type = GEOM_EDGE; index = 6; break;			// edge 6
	case 2: type = GEOM_VERTEX; index = 7; break;		// corner 7
	case 3: type = GEOM_EDGE; index = 5; break;			// edge 5
	case 4: type = GEOM_POLYGON; index = 5; break;		// face 5
	case 5: type = GEOM_EDGE; index = 7; break;			// edge 7
	case 6: type = GEOM_VERTEX; index = 5; break;		// corner 5
	case 7: type = GEOM_EDGE; index = 4; break;			// edge 4
	case 8: type = GEOM_VERTEX; index = 4; break;		// corner 4
	case 9: type = GEOM_EDGE; index = 10; break;		// edge 10
	case 10: type = GEOM_POLYGON; index = 3; break;		// face 3
	case 11: type = GEOM_EDGE; index = 11; break;		// edge 11
	case 12: type = GEOM_POLYGON; index = 1; break;		// face 1
	default: type = BAD_INDEX; index = BAD_INDEX; break;// inside the box
	case 14: type = GEOM_POLYGON; index = 0; break;		// face 0
	case 15: type = GEOM_EDGE; index = 9; break;		// edge 9
	case 16: type = GEOM_POLYGON; index = 2; break;		// face 2
	case 17: type = GEOM_EDGE; index = 8; break;		// edge 8
	case 18: type = GEOM_VERTEX; index = 2; break;		// corner 2
	case 19: type = GEOM_EDGE; index = 2; break;		// edge 2
	case 20: type = GEOM_VERTEX; index = 3; break;		// corner 3
	case 21: type = GEOM_EDGE; index = 1; break;		// edge 1
	case 22: type = GEOM_POLYGON; index = 4; break;		// face 4
	case 23: type = GEOM_EDGE; index = 3; break;		// edge 3
	case 24: type = GEOM_VERTEX; index = 1; break;		// corner 1
	case 25: type = GEOM_EDGE; index = 0; break;		// edge 0
	case 26: type = GEOM_VERTEX; index = 0; break;		// corner 0
	}
}

inline Vec3V_Out phBoundBox::GetBoxSize () const
{
	return GetBoundingBoxSize();
}

} // namespace rage

#endif
