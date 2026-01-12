//
// phbound/boundsphere.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDSPHERE_H
#define PHBOUND_BOUNDSPHERE_H


////////////////////////////////////////////////////////////////
// external defines

#include "bound.h"

#include "phcore/constants.h"
#include "phcore/materialmgr.h"
#include "vector/colors.h"


namespace rage {

class phSegment;

//=============================================================================
// phBoundSphere
// PURPOSE
//   A physics bound in the shape of a sphere.
// <FLAG Component>
//
class phBoundSphere : public phBound
{
public:
	//=========================================================================
	// Construction

	phBoundSphere (float radius=1.0f);
	PH_NON_SPU_VIRTUAL ~phBoundSphere ();

#if !__SPU
	phBoundSphere (datResource & rsc);										// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	virtual void Copy (const phBound* original);
#endif

	static const bool ms_RequiresDestructorOnStack = false;

	//=========================================================================
	// Accessors
    float GetRadius () const												{ return m_RadiusAroundCentroid; }
    ScalarV GetRadiusV () const												{ return ScalarVFromF32(m_RadiusAroundCentroid); }

	// PURPOSE: Set the radius of this sphere.
	// PARAMS:
	//	radius - the new radius of this sphere
	void SetSphereRadius (float radius);
	void SetSphereRadius (ScalarV_In radius);

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

	//=========================================================================
	// Data is in base class.
};

FORCE_INLINE_SIMPLE_SUPPORT void phBoundSphere::LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 UNUSED_PARAM(vec), SupportPoint & sp) const
{
    sp.m_vertex = GetCentroidOffset();
	sp.m_index = DEFAULT_SUPPORT_INDEX;
}

} // namespace rage

#endif
