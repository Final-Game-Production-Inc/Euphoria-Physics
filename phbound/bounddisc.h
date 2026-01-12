//
// phbound/bounddisc.h
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDDISC_H
#define PHBOUND_BOUNDDISC_H


////////////////////////////////////////////////////////////////
// external defines

#include "bound.h"

#include "phcore/constants.h"
#include "phcore/materialmgr.h"
#include "vector/colors.h"


namespace rage {

class phSegment;

//=============================================================================
// phBoundDisc
// PURPOSE
//   A physics bound in the shape of a disc.
// <FLAG Component>
//
class phBoundDisc : public phBound
{
public:
	//=========================================================================
	// Construction

	phBoundDisc (float radius=1.0f);
	PH_NON_SPU_VIRTUAL ~phBoundDisc ();

#if !__SPU
	phBoundDisc (datResource & rsc);										// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	virtual void Copy (const phBound* original);
#endif

	static const bool ms_RequiresDestructorOnStack = false;

	//=========================================================================
	// Accessors
	float GetRadius () const												{ return GetRadiusAroundCentroid()-GetMargin(); }
	ScalarV_Out GetRadiusV (ScalarV_In cachedMargin) const					{ return Subtract(GetRadiusAroundCentroidV(),cachedMargin); }
    ScalarV_Out GetRadiusV () const											{ return GetRadiusV(GetMarginV()); }
	Vec3V_Out GetAxis() const												{ return Vec3V(V_X_AXIS_WZERO); }

	// PURPOSE: Set the radius of this disc.
	// PARAMS:
	//	radius - the new radius of this disc
	void SetDiscRadius (float radius);
	void SetDiscRadius (ScalarV_In radius);

	// PURPOSE: Set the margin of the disc without changing the marginless radius of the disc
	// PARAMS:
	//  margin - the new margin of the disc
	// NOTE: calling phBound::SetMargin will modify the marginless radius of the disc because the radius is the 
	//         margin subtracted from the centroid radius, and that SetMargin won't update the centroid radius. 
	void SetMargin (float margin);
	void SetMargin (ScalarV_In margin);

	void SetCentroidOffset (Vec3V_In offset);								// set the centroid to be at offset
	void ShiftCentroidOffset (Vec3V_In offsetDelta);							// translate the centroid by offsetDelta

	phMaterialMgr::Id GetMaterialIdFromPartIndex (int UNUSED_PARAM(partIndex)) const { return GetPrimitiveMaterialId(); }
	void SetMaterial (phMaterialMgr::Id materialId, phMaterialIndex UNUSED_PARAM(materialIndex)=-1) { SetPrimitiveMaterialId(materialId); }

#if !__SPU
	virtual const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) const { return MATERIALMGR.GetMaterial(GetPrimitiveMaterialId()); }
	virtual phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const { return  GetPrimitiveMaterialId(); }

	// <COMBINE phBound::GetMaterialIdFromPartIndex>
#else
	phMaterialMgr::Id GetMaterialId (int UNUSED_PARAM(index)) const { return GetPrimitiveMaterialId(); }
#endif

	//=========================================================================
	// Operations

#if !__SPU
	// <COMBINE phBound::CanBecomeActive>
	virtual bool CanBecomeActive() const;
#endif	// !__SPU

	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
	void LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, SupportPoint & sp) const;

#if !__SPU
	//=========================================================================
	// Debugging
#if __PFDRAW
	virtual void Draw(Mat34V_In mtx, bool colorMaterials = false, bool solid = false, int whichPolys = phBound::ALL_POLYS, phMaterialFlags highlightFlags = 0, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff, unsigned int boundTypeFlags = 0, unsigned int boundIncludeFlags = 0) const;
#endif // __PFDRAW

#if __NMDRAW
  virtual void NMRender(Mat34V_In mtx) const;
#endif // __NMDRAW
#endif	// !__SPU

#if !__SPU
protected:
#endif
	//=========================================================================
	// load / save
	bool Load_v110 (fiAsciiTokenizer & token);
#if !__FINAL && !IS_CONSOLE
	bool Save_v110 (fiAsciiTokenizer & token);
#endif

	//=========================================================================
	// Protected operations.
	void CalculateExtents ();												// calculate the bounding sphere and box

	u8 m_Pad[16];
};

FORCE_INLINE_SIMPLE_SUPPORT void phBoundDisc::LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 vec, SupportPoint & sp) const
{
	const Vec3V vec3 = RCC_VEC3V(vec);
	const Vec3V squaredVec3 = Scale(vec3, vec3);
	const ScalarV flatLenSqrd = Add(squaredVec3.GetY(), squaredVec3.GetZ());
	const Vec3V vec3InvScaled = Scale(vec3, InvSqrtFast(flatLenSqrd));
	const Vec3V radialVec = SelectFT(IsLessThan(flatLenSqrd, ScalarV(V_FLT_SMALL_12)), Vec3V(ScalarV(V_ZERO), vec3InvScaled.GetY(), vec3InvScaled.GetZ()), Vec3V(V_ZERO));
    sp.m_vertex = Add(GetCentroidOffset(), Scale(radialVec, GetRadiusV()));
	sp.m_index = DEFAULT_SUPPORT_INDEX;
}

} // namespace rage

#endif // PHBOUND_BOUNDDISC_H
