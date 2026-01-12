//
// phbound/boundplane.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_BOUNDPLANE_H
#define PHBOUND_BOUNDPLANE_H


#include "bound.h"

#include "phcore/constants.h"
#include "phcore/materialmgr.h"


namespace rage {


// phBoundPlane
class phBoundPlane : public phBound
{
public:

	phBoundPlane ();
	PH_NON_SPU_VIRTUAL ~phBoundPlane ();

#if !__SPU
	phBoundPlane (datResource & rsc);										// construct in resource

#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

#endif // !__SPU

	// <COMBINE phBound::LocalGetSupportingVertexWithoutMargin>
	void LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, SupportPoint & sp) const;

#if !__SPU
	virtual const phMaterial& GetMaterial (phMaterialIndex UNUSED_PARAM(index)) const { return MATERIALMGR.GetMaterial(GetPrimitiveMaterialId()); }
	virtual phMaterialMgr::Id GetMaterialId (phMaterialIndex UNUSED_PARAM(index)) const { return  GetPrimitiveMaterialId(); }

	// <COMBINE phBound::GetMaterialIdFromPartIndex>
#else
	phMaterialMgr::Id GetMaterialId (int UNUSED_PARAM(index)) const { return GetPrimitiveMaterialId(); }
#endif

	void SetPosition( Vec3V_In pos ) { SetCentroidOffset(pos); }
	void SetNormal( Vec3V_In n ) { SetCGOffset(n); }

	Vec3V GetPosition() const { return GetCentroidOffset(); }
	Vec3V GetNormal() const { return GetCGOffset(); }

	PAR_SIMPLE_PARSABLE;
};

FORCE_INLINE_SIMPLE_SUPPORT void phBoundPlane::LocalGetSupportingVertexWithoutMargin (Vec::V3Param128 UNUSED_PARAM(vec), SupportPoint & sp) const
{
    sp.m_vertex = GetCentroidOffset();
	sp.m_index = DEFAULT_SUPPORT_INDEX;
}

} // namespace rage

#endif // PHBOUND_BOUNDPLANE_H
