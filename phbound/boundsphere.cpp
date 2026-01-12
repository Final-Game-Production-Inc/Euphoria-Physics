//
// phbound/boundsphere.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundsphere.h"

#include "support.h"

#include "data/resource.h"
#include "data/struct.h"
#include "file/token.h"
#include "math/simplemath.h"
#include "phcore/material.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "vector/geometry.h"

#if __NMDRAW
#include "nmext/NMRenderBuffer.h"
#endif


namespace rage {

CompileTimeAssert(sizeof(phBoundSphere) <= phBound::MAX_BOUND_SIZE);
CompileTimeAssert(sizeof(phBoundSphere) <= 128);

EXT_PFD_DECLARE_ITEM(DrawBoundMaterialNames);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDistanceOpacity);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundSolidOpacity);
EXT_PFD_DECLARE_GROUP(PolygonDensity);
EXT_PFD_DECLARE_GROUP(PrimitiveDensity);


////////////////////////////////////////////////////////////////
// profiling variables

namespace phBoundStats
{
	EXT_PF_TIMER(Sphere_Sphere);
	EXT_PF_TIMER(TP_Sphere);
};

using namespace phBoundStats;


////////////////////////////////////////////////////////////////

phBoundSphere::phBoundSphere (float radius)
{
	m_Type = SPHERE;

	SetMaterial(phMaterialMgr::DEFAULT_MATERIAL_ID);

	phBound::SetCentroidOffset(Vec3V(V_ZERO));
	SetSphereRadius(radius);
}


phBoundSphere::~phBoundSphere()
{
}

#if !__SPU
////////////////////////////////////////////////////////////////
// resources

phBoundSphere::phBoundSphere (datResource & rsc) : phBound(rsc)
{
}

#if __DECLARESTRUCT
void phBoundSphere::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);
	STRUCT_BEGIN(phBoundSphere);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

////////////////////////////////////////////////////////////////

void phBoundSphere::Copy (const phBound* original)
{
	Assert(phBound::SPHERE==original->GetType());
	*this = *static_cast<const phBoundSphere*>(original);
	SetRefCount(1);
}
#endif	// !__SPU

void phBoundSphere::SetSphereRadius (float radius)
{
	Assert(radius>0.0f);
	m_RadiusAroundCentroid = radius;
	SetMargin(ScalarVFromF32(radius));
	CalculateExtents();
}

void phBoundSphere::SetSphereRadius (ScalarV_In radius)
{
	Assert(IsGreaterThanAll(radius,ScalarV(V_ZERO)));
	m_RadiusAroundCentroid = radius.Getf();
	SetMargin(radius);
	CalculateExtents();
}


void phBoundSphere::SetCentroidOffset (Vec3V_In offset)
{
	phBound::SetCentroidOffset(offset);
	CalculateExtents();
}


void phBoundSphere::ShiftCentroidOffset (Vec3V_In offset)
{
	phBound::SetCentroidOffset(GetCentroidOffset() + offset);
	CalculateExtents();
}


void phBoundSphere::CalculateExtents ()
{
	SetBoundingBoxMax(GetCentroidOffset() + Vec3VFromF32(m_RadiusAroundCentroid));
	SetBoundingBoxMin(GetCentroidOffset() - Vec3VFromF32(m_RadiusAroundCentroid));

	Vector3 angInertia;
	phMathInertia::FindSphereAngInertia(1.0f,m_RadiusAroundCentroid,angInertia);
	m_VolumeDistribution.SetXYZ(RCC_VEC3V(angInertia));
	m_VolumeDistribution.SetWf(PI*square(m_RadiusAroundCentroid)*m_RadiusAroundCentroid*1.333333333333f);
}

#if !__SPU

/////////////////////////////////////////////////////////////////
// load / save

bool phBoundSphere::Load_v110 (fiAsciiTokenizer & token)
{
	float radius;
	token.MatchToken("radius:");
	radius = token.GetFloat();
	SetSphereRadius(radius);

	Vector3 temp;

	// centroid offset
	if (token.CheckToken("centroid:"))
	{
		token.GetVector(temp);
		SetCentroidOffset(RCC_VEC3V(temp));
	}
	
	// center of gravity offset
	if (token.CheckToken("cg:"))
	{
		token.GetVector (temp);
		SetCGOffset(RCC_VEC3V(temp));
	}
	else
	{
		// No center of gravity offset was specified, so use Offset as default m_CGOffset.
		SetCGOffset(GetCentroidOffset());
	}

	if (token.CheckToken("materials:"))
	{
		int numMaterials;
		numMaterials = token.GetInt();
		Assert(numMaterials<=1);
		if (numMaterials > 0)
		{
			SetMaterial(GetMaterialIdFromFile(token));
		}
	}

	return true;
}


#if !__FINAL && !IS_CONSOLE
bool phBoundSphere::Save_v110 (fiAsciiTokenizer & t)
{
	t.PutDelimiter("\n");
	t.PutDelimiter("radius: ");
	t.Put(m_RadiusAroundCentroid);
	t.PutDelimiter("\n");

	if (!IsEqualAll(GetCentroidOffset(),Vec3V(V_ZERO)))
	{
		t.PutDelimiter("centroid: ");
		t.Put(GetCentroidOffset());
		t.PutDelimiter("\n");
	}

	if (!IsEqualAll(GetCGOffset(),Vec3V(V_ZERO)))
	{
		t.PutDelimiter("cg: ");
		t.Put(GetCGOffset());
		t.PutDelimiter("\n");
	}

	t.PutDelimiter("\n");

	t.PutDelimiter("materials: ");
	t.Put(GetNumMaterials());
	t.PutDelimiter("\n");

	WriteMaterialIdToFile(t, GetMaterialId(0));
	t.PutDelimiter("\n");

	return true;
}


#endif	// end of #if !__FINAL && !IS_CONSOLE

#endif // !__SPU

////////////////////////////////////////////////////////////////////////////////


#if !__SPU
bool phBoundSphere::CanBecomeActive () const
{
	return true;
}
#endif	// !__SPU


////////////////////////////////////////////////////////////////
// bound line drawing 

#if __PFDRAW
void phBoundSphere::Draw(Mat34V_In mtxIn, bool colorMaterials, bool solid, int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlags), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	if(PFDGROUP_PolygonDensity.GetEnabled())
	{
		return;
	}
	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	const int STEPS = 18;
	const bool DRAW_LONGITUDINAL_CIRCLES = true;

	Color32 newColor(grcCurrentColor);

	Vector3 relPos(ORIGIN);
    if (grcViewport::GetCurrent())
    {
		mtx.UnTransform(VEC3V_TO_VECTOR3(grcViewport::GetCurrentCameraPosition()), relPos);
    }

	if (colorMaterials)
	{
		newColor = MATERIALMGR.GetDebugColor(GetMaterial(0));
	}
	else if (relPos.Dist2(VEC3V_TO_VECTOR3(GetCentroidOffset()))<square(m_RadiusAroundCentroid))
	{
		// The camera is inside the sphere.
		newColor.SetGreen(Max(0, newColor.GetGreen() - 100));
		newColor.SetBlue(Max(0, newColor.GetBlue() - 100));
	}

	bool oldLighting = true;

	if (solid)
	{
		newColor.SetAlpha(int(255 * PFD_BoundSolidOpacity.GetValue()));
	}
	else
	{
		float maxDrawDistance = PFD_BoundDrawDistance.GetValue();
		float distance = Min(maxDrawDistance,SqrtfSafe(relPos.Mag2()));
		float inverseDrawDistance = InvertSafe(maxDrawDistance);
		newColor.SetAlpha(int(255 * (1.0f-(1.0f-PFD_BoundDistanceOpacity.GetValue())*distance*inverseDrawDistance)));
		oldLighting = grcLighting(false);
	}

	grcColor(newColor);

	Vector3 position;
	Matrix34 offsetMtx;
	mtx.Transform(VEC3V_TO_VECTOR3(GetCentroidOffset()), offsetMtx.d);
	offsetMtx.Set3x3(mtx);
	position = offsetMtx.d;
	grcDrawSphere(m_RadiusAroundCentroid, offsetMtx, STEPS, DRAW_LONGITUDINAL_CIRCLES,solid);

	if (PFD_DrawBoundMaterialNames.GetEnabled())
	{
		// Write the polygon index number on the screen at the average location of its vertices.
		grcColor(Color_white);
		grcDrawLabelf(mtx.d,GetMaterial(0).GetName());
	}

	if (!solid)
	{
		grcLighting(oldLighting);
	}
}
#endif // __PFDRAW

#if __NMDRAW
void phBoundSphere::NMRender(const Matrix34& mtx) const
{
  phBound::NMRender(mtx);
  Matrix34 offsetMtx;
  mtx.Transform(m_CentroidOffset, offsetMtx.d);
  offsetMtx.Set3x3(mtx);
  NMRenderBuffer::getInstance()->addSphere(NMDRAW_BOUNDS, m_RadiusAroundCentroid, offsetMtx, 12, Vector3(1, 0, 1));
}
#endif // __NMDRAW

} // namespace rage
