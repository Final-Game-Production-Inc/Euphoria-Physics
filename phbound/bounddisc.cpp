//
// phbound/bounddisc.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "bounddisc.h"

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

CompileTimeAssert(sizeof(phBoundDisc) <= phBound::MAX_BOUND_SIZE);
CompileTimeAssert(__TOOL || __64BIT || sizeof(phBoundDisc) <= 128);

EXT_PFD_DECLARE_ITEM(DrawBoundMaterialNames);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDistanceOpacity);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundSolidOpacity);
EXT_PFD_DECLARE_GROUP(PolygonDensity);
EXT_PFD_DECLARE_GROUP(PrimitiveDensity);



////////////////////////////////////////////////////////////////

phBoundDisc::phBoundDisc (float radius)
{
	Assert(radius>0.0f);

	m_Type = DISC;
	SetDiscRadius(radius);
	SetMaterial(phMaterialMgr::DEFAULT_MATERIAL_ID);
	phBound::SetCentroidOffset(Vec3V(V_ZERO));

	CalculateExtents();
}


phBoundDisc::~phBoundDisc()
{
}

#if !__SPU
////////////////////////////////////////////////////////////////
// resources

phBoundDisc::phBoundDisc (datResource & rsc) : phBound(rsc)
{
	Assertf(IsGreaterThanOrEqualAll(GetRadiusV(), ScalarV(V_ZERO)), "Resourced disc bound with radius less than zero. %f", GetRadius());
}

#if __DECLARESTRUCT
void phBoundDisc::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);
	STRUCT_BEGIN(phBoundDisc);
	STRUCT_CONTAINED_ARRAY(m_Pad);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

////////////////////////////////////////////////////////////////

void phBoundDisc::Copy (const phBound* original)
{
	Assert(phBound::DISC==original->GetType());
	*this = *static_cast<const phBoundDisc*>(original);
	SetRefCount(1);
}
#endif	// !__SPU

void phBoundDisc::SetDiscRadius (float radius)
{
	SetDiscRadius(ScalarVFromF32(radius));
}

void phBoundDisc::SetDiscRadius (ScalarV_In radius)
{
	Assertf(IsGreaterThanOrEqualAll(radius, ScalarV(V_ZERO)), "Setting disc bound radius to less than zero. %f", radius.Getf());
	SetRadiusAroundCentroid(Add(radius,GetMarginV()));
	CalculateExtents();
}

void phBoundDisc::SetMargin (float margin)
{
	SetMargin(ScalarVFromF32(margin));
}

void phBoundDisc::SetMargin (ScalarV_In margin)
{
	// Add the difference in margin to the centroid radius so the marginless disc radius is the same
	SetRadiusAroundCentroid(Add(GetRadiusAroundCentroidV(), Subtract(margin,GetMarginV())));
	phBound::SetMargin(margin);
	CalculateExtents();
}

void phBoundDisc::SetCentroidOffset (Vec3V_In offset)
{
	phBound::SetCentroidOffset(offset);
	CalculateExtents();
}


void phBoundDisc::ShiftCentroidOffset (Vec3V_In offset)
{
	phBound::SetCentroidOffset(GetCentroidOffset() + offset);
	CalculateExtents();
}


void phBoundDisc::CalculateExtents ()
{
	Assert(IsGreaterThanOrEqualAll(GetRadiusAroundCentroidV(),ScalarV(V_ZERO)));

	Vec3V halfExtents = Vec3V(GetMarginV(), Vec2VFromF32(m_RadiusAroundCentroid));
	SetBoundingBoxMin(GetCentroidOffset() - halfExtents);
	SetBoundingBoxMax(GetCentroidOffset() + halfExtents);

	Vector3 angInertia;
	phMathInertia::FindCylinderAngInertia(1.0f,m_RadiusAroundCentroid,2.0f*(GetMarginV().Getf()),&angInertia);
	m_VolumeDistribution.SetXYZ(RCC_VEC3V(angInertia));
	m_VolumeDistribution.SetWf(PI*square(m_RadiusAroundCentroid)*2.0f*(GetMarginV().Getf()));
}

#if !__SPU

/////////////////////////////////////////////////////////////////
// load / save

bool phBoundDisc::Load_v110 (fiAsciiTokenizer & token)
{
	// Need to set margin first - SetDiscRadius recalculates extents and will be wrong if the margin isn't set yet
	float margin = 0.0f;
	if (token.CheckToken("margin:"))
	{
		margin = token.GetFloat();
	}
	phBound::SetMargin(margin);

	float radius;
	token.MatchToken("radius:");
	radius = token.GetFloat();
	SetDiscRadius(radius);

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
bool phBoundDisc::Save_v110 (fiAsciiTokenizer & t)
{
	t.PutDelimiter("\n");
	t.PutDelimiter("margin: ");
	t.Put(GetMargin());

	t.PutDelimiter("\n");
	t.PutDelimiter("radius: ");
	t.Put(GetRadius());

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
bool phBoundDisc::CanBecomeActive () const
{
	return true;
}
#endif	// !__SPU


////////////////////////////////////////////////////////////////
// bound line drawing 

#if __PFDRAW
void phBoundDisc::Draw(Mat34V_In mtxIn, bool colorMaterials, bool solid, int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlags), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	if(PFDGROUP_PolygonDensity.GetEnabled() || PFDGROUP_PrimitiveDensity.GetEnabled())
	{
		return;
	}

	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	const int STEPS = 18;
	const bool DRAW_LONGITUDINAL_CIRCLES = true;

	Color32 newColor(grcCurrentColor);

	Vector3 relPos(ORIGIN);
#if !DISABLE_DRAW_GRCVIEWPORT_GETCURRENT
    if (grcViewport::GetCurrent())
    {
		mtx.UnTransform(VEC3V_TO_VECTOR3(grcViewport::GetCurrentCameraPosition()), relPos);
    }
#endif

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
void phBoundDisc::NMRender(const Matrix34& mtx) const
{
  phBound::NMRender(mtx);
  Matrix34 offsetMtx;
  mtx.Transform(m_CentroidOffset, offsetMtx.d);
  offsetMtx.Set3x3(mtx);
  NMRenderBuffer::getInstance()->addSphere(NMDRAW_BOUNDS, m_RadiusAroundCentroid, offsetMtx, 12, Vector3(1, 0, 1));
}
#endif // __NMDRAW

} // namespace rage
