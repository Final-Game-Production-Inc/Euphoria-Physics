//
// phbound/boundcapsule.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHEFFECTS_BOUNDTAPEREDCAPSULE_H
#define PHEFFECTS_BOUNDTAPEREDCAPSULE_H

////////////////////////////////////////////////////////////////
// external definfes

#include "bound.h"

#include "phcore/materialmgr.h"
#include "vector/colors.h"


namespace rage {

#if USE_TAPERED_CAPSULE

////////////////////////////////////////////////////////////////
// phBoundTaperedCapsule

//!me
	//  NOTE:  only TestSphereWorldSpace and TestProbe are fully implemented.  Any other routine will treat this shape as an ordinary capsule with radius equal to that of it's first sphere.

/*
PURPOSE
	A class to represent a physics bound in the shape of a tapered capsule.  It is the convex hull of the two spheres located at either end of the shaft.
	The orientation of the shaft is always along the bound's local y-axis.
*/
class phBoundTaperedCapsule : public phBound
{
public:
	enum { BOTTOM_HEMISPHERE, TOP_HEMISPHERE, CAPSULE_SHAFT };

	phBoundTaperedCapsule ();
	~phBoundTaperedCapsule ();

	////////////////////////////////////////////////////////////
	// manipulators
	void SetCapsuleSize (float radiusA, float radiusB, float length);
	void SetCentroidOffset (Vec3V_In offset);							// set the centroid to be at offset
	void ShiftCentroidOffset (Vec3V_In offsetDelta);						// move the centroid by offsetDelta

	// PURPOSE: Get the distance between the centers of the tapered capsule ends
	// RETURN:	the distance between the centers of the tapered capsule ends
	// NOTES:	The total length of the bound is larger than this length.
	ScalarV_Out GetAxisLength () const;
	float GetLength () const;
	const Vector3 GetLengthV () const;

#if !__SPU
	virtual const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) const { return MATERIALMGR.GetMaterial(m_MaterialId); }
#endif
	PH_NON_SPU_VIRTUAL phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(materialIndex)) const { return m_MaterialId; }
	const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) { return MATERIALMGR.GetMaterial(m_MaterialId); }
	void SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex UNUSED_PARAM(materialIndex)=-1) { m_MaterialId = materialId; }
	// <COMBINE phBound::GetMaterialIdFromPartIndex>
	phMaterialMgr::Id GetMaterialIdFromPartIndex (int UNUSED_PARAM(partIndex)) const { return m_MaterialId; }

	// PURPOSE: Get the upper end radius if the tapered capsule.
	// RETURN:	the upper end radius if the tapered capsule
	ScalarV_Out GetRadius1() const;
	float GetRadius () const;

	// PURPOSE: Get the lower end radius if the tapered capsule.
	// RETURN:	the lower end radius if the tapered capsule
	ScalarV_Out GetRadius2() const;
	float GetSecondRadius () const;

	ScalarV_Out GetSine () const;
	const Vector3 GetSint () const;
	ScalarV_Out GetCosine () const;
	const Vector3 GetCost () const;

	////////////////////////////////////////////////////////////
	// intersection test functions
	bool IsInsideCapsule (const Vector3& point) const;
	int SegmentToCapsuleIntersections (const Vector3& point1,const Vector3& point2, float& segT1, float& segT2,
										float& hdT1, float& hdT2, int& index1, int& index2) const;
	Vector3 FindCapsuleIsectNormal (const Vector3& position, int partIndex) const;

#if !__SPU
	// <COMBINE phBound::CanBecomeActive>
	virtual bool CanBecomeActive() const;
#endif

	void GetTaperedGeometryVsSphere( float otherRad, float &coneOffset, float &coneOffset2, float &coneRadius, float &coneRadius2 ) const;
	void GetTaperedGeometryVsSphere (ScalarV_In otherRad, ScalarV_InOut coneOffset, ScalarV_InOut coneOffset2, ScalarV_InOut coneRadius, ScalarV_InOut coneRadius2) const;

	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
	Vec3V_Out LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, int* vertexIndex=NULL) const;

#if !__SPU
	////////////////////////////////////////////////////////////
	// visualization
#if __PFDRAW
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
#endif // __PFDRAW

	////////////////////////////////////////////////////////////
	// resources
	phBoundTaperedCapsule (datResource & rsc);									// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	static bool ResourceCreate(phBound* Bound, datResource& rsc);


//protected:
	////////////////////////////////////////////////////////////
	// load / save
	bool Load_v110 (fiAsciiTokenizer & token);

#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);
#endif

#endif	// !__SPU

	////////////////////////////////////////////////////////////
	// helper functions
	void CalculateExtents ();											// calculate the bounding box and sphere

	////////////////////////////////////////////////////////////
	// tapirde data
	// I have created a new species here, so I'm giving it a new name: tapirde in honour of the Belgian naturalist who discovered the tapir
	// a tapirde is the convex hull for two disjoint spheres.  If the spheres have the same radius, then it will be a capsule.
	// if you draw this out you'll see that the cone ends are not the same points along the axis as the centre point for the spheres.

	// PURPOSE: the radius of the sphere on the upper end end
	float m_CapsuleRadius;

	// PURPOSE: the radius of the sphere on the lower end
	float m_CapsuleRadius2;

	// PURPOSE: length from centre of one sphere to the other
	float m_CapsuleLength;

	// PURPOSE: the sine and cosine of the angle made by the cone
	float m_Sine,m_Cosine;

	phMaterialMgr::Id m_MaterialId;			// index of the material in the material manager
};

inline ScalarV_Out phBoundTaperedCapsule::GetRadius1() const
{
	return ScalarVFromF32(m_CapsuleRadius);
}

inline ScalarV_Out phBoundTaperedCapsule::GetRadius2() const
{
	return ScalarVFromF32(m_CapsuleRadius2);
}

inline float phBoundTaperedCapsule::GetRadius () const
{
	return m_CapsuleRadius;
}

inline float phBoundTaperedCapsule::GetSecondRadius () const
{
	return m_CapsuleRadius2;
}

inline ScalarV_Out phBoundTaperedCapsule::GetAxisLength () const
{
	return ScalarVFromF32(m_CapsuleLength);
}

inline float phBoundTaperedCapsule::GetLength () const
{
	return m_CapsuleLength;
}

inline const Vector3 phBoundTaperedCapsule::GetLengthV () const
{
	return Vector3(m_CapsuleLength,m_CapsuleLength,m_CapsuleLength);
}

inline ScalarV_Out phBoundTaperedCapsule::GetSine () const
{
	return ScalarVFromF32(m_Sine);
}

inline const Vector3 phBoundTaperedCapsule::GetSint () const
{
	return Vector3(m_Sine,m_Sine,m_Sine);
}

inline ScalarV_Out phBoundTaperedCapsule::GetCosine () const
{
	return ScalarVFromF32(m_Cosine);
}

inline const Vector3 phBoundTaperedCapsule::GetCost () const
{
	return Vector3(m_Cosine,m_Cosine,m_Cosine);
}

#endif // USE_TAPERED_CAPSULE

} // namespace rage

#endif	// end of #ifndef PHEFFECTS_BOUNDTAPEREDCAPSULE_H
