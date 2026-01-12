//
// phbound/boundcapsule.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDCAPSULE_H
#define PHBOUND_BOUNDCAPSULE_H

////////////////////////////////////////////////////////////////
// external defines

#include "bound.h"

#include "phcore/materialmgr.h"
#if !__SPU
#include "vector/colors.h"
#endif


namespace rage {


////////////////////////////////////////////////////////////////
// phBoundCapsule

/*
PURPOSE
	A class to represent a physics bound in the shape of a capsule.  Mathematically, the surface of the shape is described as the set of all points
	located a fixed distance (the radius) away from a given line segment (the shaft).  In the case of the phBoundCapsule, the capsule is specified 
	by its radius and the length of its shaft; the orientation of the shaft is always along the bound's local y-axis.
*/
class phBoundCapsule : public phBound
{
public:
	enum { BOTTOM_HEMISPHERE, TOP_HEMISPHERE, CAPSULE_SHAFT };

	phBoundCapsule ();
	~phBoundCapsule ();

#if !__SPU
	virtual void Copy (const phBound* original);
#endif


#if USE_CAPSULE_EXTRA_EXTENTS
	void SetHalfHeight(float fHalfHeght) { m_CapsuleHalfHeight = fHalfHeght; }
	float GetHalfHeight() const { return m_CapsuleHalfHeight; }
#endif

	// PURPOSE: Set the radius and length of this capsule.
	// PARAMS:
	//	radius - the new radius of this capsule
	//	length - the new length of this capsule
	// NOTES: The length is the distance between the two hemisphere centers, so the total object length is length+2*radius.
	void SetCapsuleSize (float radius, float length);
	void SetCapsuleSize (ScalarV_In radius, ScalarV_In length);

	void SetType() { m_Type = CAPSULE; }

	// PURPOSE: Set the radius of this capsule.
	// PARAMS:
	//	radius - the new radius of this capsule
	void SetCapsuleRadius (float radius);
	void SetCapsuleRadius (ScalarV_In radius);

	// PURPOSE: Set the margin without modifying the length of the capsule
	// PARAMS: 
	//   margin - the new margin
	// NOTE: calling phBound::SetMargin will modify the length of the capsule because the length is the 
	//         margin subtracted from the centroid radius, and that SetMargin won't update the centroid radius. 
	void SetMargin (float margin);
	void SetMargin (ScalarV_In margin);

	// PURPOSE: Set the length of this capsule.
	// PARAMS:
	//	length - the new length of this capsule
	// NOTES: The length is the distance between the two hemisphere centers, so the total object length is length+2*radius.
	void SetCapsuleLength (float length);
	void SetCapsuleLength (ScalarV_In length);

	// <COMBINE phBound::SetCentroidOffset>
	void SetCentroidOffset (Vec3V_In offset);

	// <COMBINE phBound::ShiftCentroidOffset>
	void ShiftCentroidOffset (Vec3V_In offsetDelta);

	// PURPOSE: Get the capsule radius.
	// RETURN:	the radius of the capsule
	float GetRadius () const;
	ScalarV_Out GetRadiusV() const;

	// PURPOSE: Get the capsule length.
	// RETURN:	the length of the capsule, not including the end caps (the total length is length + twice radius)
	// NOTES: This is slower than getting the half-length. Passing in the radius makes this function faster. 
	float GetLength () const;
	ScalarV_Out GetLengthV () const;
	ScalarV_Out GetLengthV (ScalarV_In cachedRadius) const;

	// PURPOSE: Get the half length of the capsule
	// NOTES: Getting the half length is faster than getting the length. Passing in the radius makes this function faster.
	ScalarV_Out GetHalfLengthV () const;
	ScalarV_Out GetHalfLengthV (ScalarV_In cachedRadius) const;

	// <COMBINE phBound::GetMaterialFromPartIndex>
	phMaterialMgr::Id GetMaterialIdFromPartIndex (int UNUSED_PARAM(partIndex)) const { return GetPrimitiveMaterialId(); }
	void SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex UNUSED_PARAM(materialIndex)=-1) { SetPrimitiveMaterialId(materialId); }
#if !__SPU
	virtual const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) const { return MATERIALMGR.GetMaterial(GetPrimitiveMaterialId()); }
	virtual phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const { return GetPrimitiveMaterialId(); }
	const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) { return MATERIALMGR.GetMaterial(GetPrimitiveMaterialId()); }
#else
	phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const { return GetPrimitiveMaterialId(); }
#endif

	// PURPOSE: Get one of the capsules end points in local coordinates.
	// PARAMS:
	//	index - the index number of the capsule's end point to get (A==up in y, B==down in y).
	// RETURN:	one of the capsules end points in local coordinates
	const Vec3V_Out GetEndPointA () const;
	const Vec3V_Out GetEndPointB () const;

	////////////////////////////////////////////////////////////
	// intersection test functions
	bool IsInsideCapsule (const Vector3& point) const;

	// PURPOSE: See if the segment connecting the given points intersects the surface of the capsule.
	// PARAMS:
	//	point1 - the first point, relative to the capsule's center
	//	point2 - the second point, relative to the capsule's center
	//	capsuleLength - the axis length of the capsule (between the hemisphere centers)
	//	capsuleRadius - the radius of the capsule
	//	segT1 - reference to the segment t-value of the first intersection of the segment with the capsule's surface
	//	segT2 - reference to the segment t-value of the second intersection of the segment with the capsule's surface
	//	hdT1 - reference to the capsule axis t-value of the first intersection of the segment with the capsule's surface
	//	hdT2 - reference to the capsule axis t-value of the second intersection of the segment with the capsule's surface
	//	index1 - reference to the capsule section index number of the first intersection
	//	index2 - reference to the capsule section index number of the second intersection
	// RETURN: the number of intersections of the segment with the capsule surface
	static int SegmentToCapsuleIntersections (Vec3V_In point1, Vec3V_In point2, ScalarV_In capsuleLength, ScalarV_In capsuleRadius, ScalarV_InOut segT1, ScalarV_InOut segT2,
												ScalarV_InOut capsuleT1, ScalarV_InOut capsuleT2, int& index1, int& index2);

	Vector3 FindCapsuleIsectNormal (const Vector3& position, int partIndex) const;

	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
	void LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, SupportPoint & sp) const;

	static const bool ms_RequiresDestructorOnStack = false;
#if !__SPU
	// <COMBINE phBound::CanBecomeActive>
	virtual bool CanBecomeActive() const;

	////////////////////////////////////////////////////////////
	// visualization
#if __PFDRAW
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
#endif // __PFDRAW

#if __NMDRAW
  virtual void NMRender(const Matrix34& mtx) const;
#endif // __NMDRAW
	////////////////////////////////////////////////////////////
	// resources
	phBoundCapsule (datResource & rsc);									// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

protected:
	////////////////////////////////////////////////////////////
	// load / save
	bool Load_v110 (fiAsciiTokenizer & token);

#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);
#endif

#endif // !__SPU

	////////////////////////////////////////////////////////////
	// helper functions
	void CalculateExtents ();											// calculate the bounding box and sphere

#if USE_CAPSULE_EXTRA_EXTENTS
	float m_CapsuleHalfHeight;
	u8 pad[12];
#else
	u8 pad[16];
#endif

	PAR_SIMPLE_PARSABLE;
};


/*
Purpose: Find the normal vector pointing out from the capsule surface at the given position.
Parameters:
position	- the position at which to get the normal vector, relative to the capsule's center
partIndex	- the part of the capsule in which to get the normal (bottom hemisphere, top hemisphere or shaft)
Return: the normal vector pointing out from the capsule surface at the given position */
inline Vector3 phBoundCapsule::FindCapsuleIsectNormal (const Vector3& position, int partIndex) const
{
    Vector3 normal;
    if (partIndex==CAPSULE_SHAFT)
    {
        // The normal points out from the axis horizontally.
        normal.Set(position.x,0.0f,position.z);
    }
    else if (partIndex==TOP_HEMISPHERE)
    {
        // The normal points out from the top hemisphere.
        normal.Set(position);
        normal.y -= 0.5f*GetLength();
    }
    else
    {
        // The normal points out from the bottom hemisphere.
        FastAssert(partIndex==BOTTOM_HEMISPHERE);
        normal.Set(position);
        normal.y += 0.5f*GetLength();
    }

    // Normalize and return the outward-pointing normal vector.
    normal.Normalize();
    return normal;
}


#if __SPU
static const __vector4 VEC3_ZERO_HALF_ZERO = ((vector float) { 0.0f,  0.5f, 0.0f, 1.0f});
static const __vector4 VEC3_ZERO_MINUS_HALF_ZERO = ((vector float){ 0.0f, -0.5f, 0.0f, 1.0f});
#else
extern const Vector3 VEC3_ZERO_HALF_ZERO;
extern const Vector3 VEC3_ZERO_MINUS_HALF_ZERO;
#endif


FORCE_INLINE_SIMPLE_SUPPORT void phBoundCapsule::LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 v, SupportPoint & sp) const
{
	const Vec4V v_zero(V_ZERO);
	const Vec4V v_one(V_INT_1);
	const ScalarV halfLength = GetHalfLengthV();
	const Vec3V end = Vec3V(halfLength) & Vec3V(V_MASKY);
	const BoolV whichEnd = IsLessThan(Vec3V(v).GetY(), ScalarV(V_ZERO));
	const Vec3V theEnd = SelectFT(whichEnd, end, -end);
	const Vec4V theInd = SelectFT(whichEnd, v_one, v_zero);
	sp.m_vertex = GetCentroidOffset() + theEnd;
	sp.m_index = theInd.GetIntrin128ConstRef();
}

inline float phBoundCapsule::GetRadius () const
{
	return GetMargin();
}

inline ScalarV_Out phBoundCapsule::GetRadiusV () const
{
	return GetMarginV();
}

inline float phBoundCapsule::GetLength () const
{
	return GetLengthV().Getf();
}

inline ScalarV_Out phBoundCapsule::GetHalfLengthV (ScalarV_In cachedRadius) const
{
	return Subtract(GetRadiusAroundCentroidV(),cachedRadius);
}

inline ScalarV_Out phBoundCapsule::GetHalfLengthV () const
{
	return GetHalfLengthV(GetRadiusV());
}

inline ScalarV_Out phBoundCapsule::GetLengthV (ScalarV_In cachedRadius) const
{
	return Scale(GetHalfLengthV(cachedRadius),ScalarV(V_TWO));
}

inline ScalarV_Out phBoundCapsule::GetLengthV () const
{
	return GetLengthV(GetRadiusV());
}





} // namespace rage

#endif	// end of #ifndef PHBOUND_BOUNDCAPSULE_H
