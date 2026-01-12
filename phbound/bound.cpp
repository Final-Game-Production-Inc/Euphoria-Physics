//
// phbound/bound.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "bound.h"

#if !__SPU
#include "boundbox.h"
#include "boundbvh.h"
#include "boundcapsule.h"
#include "boundcomposite.h"
#include "boundcurvedgeom.h"
#include "boundgrid.h"
#include "boundplane.h"
#include "boundribbon.h"
#include "boundsphere.h"
#include "boundsurface.h"
#include "primitives.h"
#include "support.h"

#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#endif

#if !__SPU
#include "phcore/config.h"
#include "phcore/material.h"
#include "phcore/materialmgr.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "system/timer.h"

#if __NMDRAW
#include "nmext/NMRenderBuffer.h"
#endif
#endif

#include "support.h"
#include "vectormath/classes.h"

#include "bound_parser.h"

//const rage::Vector3 PENETRATION_CHECK_EXTRA_MARGIN_V(PENETRATION_CHECK_EXTRA_MARGIN, PENETRATION_CHECK_EXTRA_MARGIN, PENETRATION_CHECK_EXTRA_MARGIN);

//const rage::Vector3 CONVEX_DISTANCE_MARGIN_V(CONVEX_DISTANCE_MARGIN, CONVEX_DISTANCE_MARGIN, CONVEX_DISTANCE_MARGIN);
//const rage::Vector3 CONCAVE_DISTANCE_MARGIN_V(CONCAVE_DISTANCE_MARGIN, CONCAVE_DISTANCE_MARGIN, CONCAVE_DISTANCE_MARGIN);

namespace rage {


int phBound::sm_MemoryBucket = 3;

#if !__SPU
Functor2Ret<bool, phBound*, datResource&>	phBound::sm_CustomResourceConstructor  = NULL;
#endif

//=============================================================================
// profiling variables

#if !__SPU
namespace phBoundStats
{
	PF_PAGE(PHBounds,"ph Bounds");

	PF_GROUP(FindImpactsEffects);
	PF_LINK(PHBounds,FindImpactsEffects);
	PF_TIMER(Liquid_Sphere,FindImpactsEffects);
	PF_TIMER(Liquid_Capsule,FindImpactsEffects);
	PF_TIMER(Liquid_Box,FindImpactsEffects);
	PF_TIMER(Liquid_Poly,FindImpactsEffects);
	PF_TIMER(Liquid_BoxTouchesLiquidBox,FindImpactsEffects);
	PF_TIMER(Liquid_ClipAgainstLiquidBorders,FindImpactsEffects);
	PF_TIMER(Liquid_TriangleBuoyancy,FindImpactsEffects);
	PF_TIMER(Liquid_SubmergedTriangleBuoyancy,FindImpactsEffects);
	PF_TIMER(Liquid_QuadrangleBuoyancy,FindImpactsEffects);
	PF_TIMER(Liquid_ReorderVerts,FindImpactsEffects);
	PF_TIMER(Liquid_GetDepth,FindImpactsEffects);

	// Outdated timers to be removed when no longer used in code.
	PF_GROUP_OFF(FindImpacts);
	PF_TIMER(TestBoundAll,FindImpacts);
	PF_TIMER(Sphere_Sphere,FindImpacts);
	PF_TIMER(Capsule_Sphere,FindImpacts);
	PF_TIMER(Capsule_Capsule,FindImpacts);
	PF_TIMER(Box_Capsule,FindImpacts);
	PF_TIMER(Box_Box,FindImpacts);
	PF_TIMER(Poly_Sphere,FindImpacts);
	PF_TIMER(Poly_Capsule,FindImpacts);
	PF_TIMER(Poly_Box,FindImpacts);
	PF_TIMER(Poly_Poly,FindImpacts);
	PF_TIMER(SplineTer_Sphere,FindImpacts);
	PF_TIMER(SplineTer_Capsule,FindImpacts);
	PF_TIMER(SplineTer_Poly,FindImpacts);
	PF_TIMER(BVH_Poly,FindImpacts);
	PF_TIMER(BVH_Sphere,FindImpacts);
	PF_TIMER(BVH_Capsule,FindImpacts);
	PF_GROUP_OFF(TestProbe);
	PF_TIMER(TP_Sphere,TestProbe);
	PF_TIMER(TP_Capsule,TestProbe);
	PF_TIMER(TP_Box,TestProbe);
	PF_TIMER(TP_Poly,TestProbe);
	PF_TIMER(TP_SplineTer,TestProbe);
};

using namespace phBoundStats;

namespace phCollisionStats
{
    PF_PAGE(PHCollision,"ph Bullet");

    PF_GROUP(Collision);
    PF_LINK(PHCollision,Collision);

    PF_TIMER(SimCollide,Collision);
    PF_TIMER(MidNarrowPhase,Collision);
	PF_TIMER(GridMidphase,Collision);
	PF_TIMER(BVHMidphase,Collision);
	PF_TIMER(BVHTrianglePrep,Collision);
	PF_TIMER(PairwiseBoxTests,Collision);
    PF_TIMER(PairwiseCollisions,Collision);
    PF_TIMER(ConvexConvex,Collision);
    PF_TIMER(ConcaveConvex,Collision);
    PF_TIMER(TriangleConvex,Collision);
    PF_TIMER(GetClosestPoints,Collision);
    PF_TIMER(PenetrationSolve,Collision);
	PF_COUNTER(PenetrationSolveCount,Collision);

    PF_TIMER(ManifoldMaintenance,Collision);
    PF_TIMER(ContactMaintenance,Collision);
    PF_TIMER(IslandMaintenance,Collision);


	PF_GROUP(NarrowPhaseCollision);
	PF_LINK(PHCollision, NarrowPhaseCollision);

	PF_TIMER(NP_CCD_Time,NarrowPhaseCollision);
	PF_COUNTER(NP_CCD_Calls,NarrowPhaseCollision);
	PF_COUNTER(NP_CCD_GCP_Calls,NarrowPhaseCollision);
	
	PF_TIMER(NP_GJK_Time,NarrowPhaseCollision);
	PF_COUNTER(NP_GJK_Calls,NarrowPhaseCollision);
	PF_COUNTER(NP_GJK_Iters,NarrowPhaseCollision);
	PF_TIMER(NP_GJK_CacheSetup,NarrowPhaseCollision);
	PF_TIMER(NP_GJK_IterTime,NarrowPhaseCollision);
	PF_TIMER(SupportFunction,NarrowPhaseCollision);


	PF_GROUP(GJKCache);
	PF_LINK(PHCollision, GJKCache);

	PF_COUNTER(GJKCacheCount,GJKCache);
	PF_COUNTER(GJKCacheCreateCount,GJKCache);
	PF_COUNTER(GJKCacheDBCount,GJKCache);
	PF_COUNTER(GJKCacheDBCreateCount,GJKCache);

	PF_COUNTER(GJKCache_CachedSimplexCount,GJKCache);
	PF_COUNTER(GJKCache_CachedSupportDirCount,GJKCache);

	PF_TIMER(GJKCacheSytemPostCollisionUpdate,GJKCache);
	PF_TIMER(GJKCacheQueryProlog,GJKCache);
	PF_TIMER(GJKCacheQueryEpilog,GJKCache);
};
using namespace phCollisionStats;

namespace phCCDStats
{
	PF_PAGE(PHCCD,"ph CCD");

	PF_GROUP(CCD);
	PF_LINK(PHCCD,CCD);

	PF_VALUE_INT(TotalCCD_Calls,CCD);
	PF_VALUE_INT(TotalCCD_Iters,CCD);
	PF_VALUE_FLOAT(AvgCCD_Iters,CCD);
	PF_VALUE_INT(PeakCCD_Iters,CCD);
	PF_VALUE_INT(MinCCD_Iters,CCD);
};
using namespace phCCDStats;

EXT_PFD_DECLARE_ITEM(SupportMargin);
EXT_PFD_DECLARE_ITEM_SLIDER(SupportPointsSamples);
#endif
//=============================================================================
// phBoundBase

// these magic number are unnecessary, but help check that the resource
// is being loaded as expected
#if !__SPU
const int phBoundBase::sVirtualPointerRsc = 0x0FFFFF00;
const int phBoundBase::sMaterialDefaultRsc = 0x0FFFFF01;
#endif

phBoundBase::~phBoundBase ()
{
}


//=============================================================================
// phBound

u32 phBound::sm_BoundFlags = 0xFFFFFFFF;


#if !__SPU
int phBound::Release (bool deleteAtZero)
{
	if (!phConfig::IsRefCountingEnabled())
	{
		return 1;
	}

	Assertf(GetRefCount()!=0 , "phBound:Release of bound @ %p type:%d already has 0 references", this, m_Type);

	SetRefCount(GetRefCount() - 1);

	if (GetRefCount()==0)
	{
		if (deleteAtZero)
		{
			delete this;
		}
		return 0;
	}
	else
	{
		return GetRefCount();
	}
}

////////////////////////////////////////////////////////////////

const char * phBound::GetTypeString () const
{
	switch (m_Type)
	{
		#undef BOUND_TYPE_INC
		#define BOUND_TYPE_INC(className,enumLabel,enumValue,stringName,isUsed) \
			case enumLabel: FastAssert(isUsed); return stringName; 
		#include "boundtypes.inc"
		#undef BOUND_TYPE_INC

		default:
			return "unknown";
	}
}
#endif


float phBound::GetVolume () const
{
	// Return the volume (the 4th element in the volume distribution).
	return m_VolumeDistribution.GetWf();
}


float phBound::GetComputeVolume () const
{
	// Return the volume (the 4th element in the volume distribution).
	return m_VolumeDistribution.GetWf();
}


Vec3V_Out phBound::GetComputeAngularInertia (float mass) const
{
	// Compute and return the angular inertia.
	return m_VolumeDistribution.GetXYZ() * ScalarV(mass);
}


void phBound::CalculateSphereFromBoundingBox ()
{
	SetCentroidOffset(ComputeBoundingBoxCenter());
	m_RadiusAroundCentroid = Dist(GetBoundingBoxMax(), GetCentroidOffset()).Getf();
}

void phBound::LocalGetSupportingVertexWithoutMarginNotInlined(Vec::V3Param128 vec, SupportPoint & sp) const
{
	switch (m_Type)
	{
		// Implicitly-defined bounds below.
	case SPHERE:
		static_cast<const phBoundSphere*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
	case CAPSULE:
		static_cast<const phBoundCapsule*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
#if USE_TAPERED_CAPSULE
	case TAPERED_CAPSULE:
		support = static_cast<const phBoundTaperedCapsule*>(this)->LocalGetSupportingVertexWithoutMargin(vec, vertexIndex);
		break;
#endif
	case BOX:
		static_cast<const phBoundBox*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
	case DISC:
		static_cast<const phBoundDisc*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
	case CYLINDER:
		static_cast<const phBoundCylinder*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
		// Explicitly-defined bounds below.
	case GEOMETRY:
		static_cast<const phBoundPolyhedron*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
#if USE_GEOMETRY_CURVED
	case GEOMETRY_CURVED:
		support = static_cast<const phBoundCurvedGeometry*>(this)->LocalGetSupportingVertexWithoutMargin(vec, vertexIndex);
		break;
#endif
#if !__SPU
	case COMPOSITE:
		static_cast<const phBoundComposite*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
#endif // !__SPU
	case TRIANGLE:
		static_cast<const TriangleShape*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
	case PLANE:
		static_cast<const phBoundPlane*>(this)->LocalGetSupportingVertexWithoutMargin(vec, sp);
		break;
	default:
		sp.m_vertex = Vec3V(V_ZERO);
		sp.m_index = DEFAULT_SUPPORT_INDEX;
		Assertf(false, "Encountered unknown bound type %d", m_Type);
	}
}

#if __DEV && !__SPU
Vec3V_Out phBound::LocalGetSupportingVertexDebug(Vec::V3Param128 localDirection) const
{
	return LocalGetSupportingVertex(localDirection);
}

bool phBound::DoesBoundingBoxContainsSupports() const
{
	// There is no support function for BVH bounds because that requires building all of the primitives
	if(GetType() == phBound::BVH || (GetType() == phBound::COMPOSITE && static_cast<const phBoundComposite*>(this)->GetContainsBVH()))
		return true;

	Vec3V supportMax;
	supportMax.SetX(LocalGetSupportingVertexDebug(Vec3V(V_X_AXIS_WZERO).GetIntrin128()).GetX());
	supportMax.SetY(LocalGetSupportingVertexDebug(Vec3V(V_Y_AXIS_WZERO).GetIntrin128()).GetY());
	supportMax.SetZ(LocalGetSupportingVertexDebug(Vec3V(V_Z_AXIS_WZERO).GetIntrin128()).GetZ());

	Vec3V supportMin;
	supportMin.SetX(LocalGetSupportingVertexDebug(Negate(Vec3V(V_X_AXIS_WZERO)).GetIntrin128()).GetX());
	supportMin.SetY(LocalGetSupportingVertexDebug(Negate(Vec3V(V_Y_AXIS_WZERO)).GetIntrin128()).GetY());
	supportMin.SetZ(LocalGetSupportingVertexDebug(Negate(Vec3V(V_Z_AXIS_WZERO)).GetIntrin128()).GetZ());

	return IsLessThanAll(supportMax, GetBoundingBoxMax() + Vec3V(V_FLT_SMALL_3)) && IsGreaterThanAll(supportMin, GetBoundingBoxMin() - Vec3V(V_FLT_SMALL_3));	
}
#endif // __DEV && !__SPU

void phBound::SetFlag(u8 mask, bool value)
{
	if (value)
	{
		m_Flags |= mask;
	}
	else
	{
		m_Flags &= ~mask;
	}
}

void phBound::SetCGOffset (Vec3V_In cgOffset)
{
	m_CGOffsetXYZMaterialId1W.SetXYZ(cgOffset);
}

#if !__SPU
phMaterialMgr::Id phBound::GetMaterialIdFromPartIndexAndComponent (int partIndex, int UNUSED_PARAM(component)) const
{
	return GetMaterialIdFromPartIndex(partIndex);
}
#endif

#if !__SPU
phMaterialMgr::Id phBound::GetMaterialIdFromFile(fiAsciiTokenizer& token)
{
	if (token.CheckIToken("type:", true))
	{
		token.IgnoreToken();
	}
	token.CheckIToken("mtl", true);
	char materialName[2048];
	token.GetToken(materialName, sizeof(materialName));
	phMaterialMgr::Id material = MATERIALMGR.FindMaterialId(materialName);
	if (token.CheckToken("{", true))
	{
		token.Pop();
	}
	if (material == phMaterialMgr::MATERIAL_NOT_FOUND)
	{
		if (MessagesEnabled())
		{
			Warningf("Bound %s uses material %s that was not in the materials list", token.GetName(), materialName);
		}

		material = phMaterialMgr::DEFAULT_MATERIAL_ID;
	}

	return material;
}


void phBound::WriteMaterialIdToFile(fiAsciiTokenizer& token, phMaterialMgr::Id materialId)
{
	char name[2048];
	MATERIALMGR.GetMaterialName(materialId, name, sizeof(name));
	token.Put(name);
}


/*
Purpose:
- Calculate the extents of this bound.
Parameters:
- None.
Return:
- None.
Notes:
- See CalculateExtents() in the other bound classes.
- As far as I can see, there is no reason for the method to be virtual or even to be defined at all at this level.
*/
void phBound::CalculateExtents ()
{
	Errorf("phBound:CalculateExtents - not defined for this bound type, %d",m_Type);
}

////////////////////////////////////////////////////////////////////////////////


bool phBound::CanBecomeActive () const
{
	return false;
}


// This virtual function is overridden in derived bound types that contain data
void phBound::Copy (const phBound* original)
{
	Assertf(GetType()==original->GetType(), "Dest Type: %d, Source Type: %d", GetType(), original->GetType());
	*this = *original;
	SetRefCount(1);
}


phBound* phBound::Clone () const
{
	phBound* clone = phBound::CreateOfType(GetType());
	clone->Copy(this);
	return clone;
}

bool phBound::SpewControlAllowMessage ()
{
	static bool firstTime = true;
	if (MessageSpewControlEnabled())
	{
		static sysTimer spewTimer;
		if (firstTime)
		{
			// This is the first time for potentially spewing messages, so return true to allow the message to display.
			firstTime = false;
			spewTimer.Reset();
			return true;
		}
		else if (spewTimer.GetTime() >= 1.0f)
		{
			// The spew timer passed the spew-control time limit, so reset it and return true to allow the message to display.
			spewTimer.Reset();
			return true;
		}
		else
		{
			// The spew timer has not passed the spew-control time limit, so return false to stop the message from displaying.
			return false;
		}
	}
	else
	{
		// Message spew control is off, so return true to allow the current message to display.
		return true;
	}
}


////////////////////////////////////////////////////////////////
// load / save, data only (headers are parsed by static Load/Save

bool phBound::LoadData (fiAsciiTokenizer & token, FileVersion UNUSED_PARAM(version))
{
	// header has already been parsed
	return Load_v110(token);
}


bool phBound::Load_v110 (fiAsciiTokenizer & /*token*/)
{
	if (MessagesEnabled())
	{
		Warningf("phBound::Load_v110 - not defined for this bound type (%d)",GetType());
	}

	return false;
}


#if !__FINAL && !IS_CONSOLE
bool phBound::SaveData (fiAsciiTokenizer & token, FileVersion UNUSED_PARAM(version))
{
	// header has already been written
	return Save_v110(token);
}


bool phBound::Save_v110 (fiAsciiTokenizer & /*token*/)
{
	if (MessagesEnabled())
	{
		Warningf("phBound::Save_v110 - not defined for this bound type (%d)",GetType());
	}

	return false;
}
#endif	// end of #if !__FINAL && !IS_CONSOLE
#endif // !__SPU


#if __PFDRAW
void phBound::Draw(Mat34V_In UNUSED_PARAM(mtx), bool UNUSED_PARAM(colorMaterials), bool UNUSED_PARAM(solid), int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlagss), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
}

void phBound::DrawNormals(Mat34V_In UNUSED_PARAM(mtx), int UNUSED_PARAM(normalType), int UNUSED_PARAM(whichPolys), float UNUSED_PARAM(length), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter)) const
{
}

void phBound::DrawLast(Mat34V_In UNUSED_PARAM(mtx), bool UNUSED_PARAM(colorMaterials), bool UNUSED_PARAM(solid), int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlagss)) const
{
}

void phBound::DrawCentroid(Mat34V_In mtx) const
{
	float size = Clamp(0.18f*m_RadiusAroundCentroid, 0.05f, 0.5f);

	const Vector3 vertices[] =
	{
		Vector3( size,  0,     0),
			Vector3(-size,  0,     0),
			Vector3( 0,     size,  0),
			Vector3( 0,    -size,  0),
			Vector3( 0,     0,     size),
			Vector3( 0,     0,    -size)
	};

	Matrix34 cgMtx;
	cgMtx.Set(RCC_MATRIX34(mtx));

	RCC_MATRIX34(mtx).Transform(VEC3V_TO_VECTOR3(GetCentroidOffset()),cgMtx.d);

	grcWorldMtx(cgMtx);

	grcBegin(drawLines, 6);

	for (int vertex = 0; vertex < 6; ++vertex)
	{
		grcVertex3fv(&vertices[vertex][0]);
	}

	grcEnd();
}

void phBound::DrawCenterOfGravity(Mat34V_In mtx) const
{
	float size = Clamp(0.18f*m_RadiusAroundCentroid, 0.05f, 0.5f);

	const Vector3 vertices[] =
	{
		Vector3( size,  0,     0),
		Vector3(-size,  0,     0),
		Vector3( 0,     size,  0),
		Vector3( 0,    -size,  0),
		Vector3( 0,     0,     size),
		Vector3( 0,     0,    -size)
	};

	Matrix34 cgMtx;
	cgMtx.Set(RCC_MATRIX34(mtx));
	RCC_MATRIX34(mtx).Transform(VEC3V_TO_VECTOR3(GetCGOffset()),cgMtx.d);

	Color32 currentColor = grcGetCurrentColor();
	grcColor(Color_yellow);
	grcDrawSphere(0.07f, cgMtx.d, 8, true, true);
	grcColor( currentColor );

	grcWorldMtx(cgMtx);

	grcBegin(drawLines, 6);

	for (int vertex = 0; vertex < 6; ++vertex)
	{
		grcVertex3fv(&vertices[vertex][0]);
	}

	grcEnd();
}

void phBound::DrawAngularInertia(Mat34V_In mtx, float scale, bool invert, Color32 boxColor) const
{
	Vec3V angInertia = const_cast<phBound*>(this)->GetComputeAngularInertia(1.0f);
	angInertia = Normalize(angInertia);
	if(invert)
	{
		Vec3V invInertia(angInertia);
		invInertia = Invert(invInertia);
		invInertia = Normalize(invInertia);
		angInertia = invInertia;
	}
	angInertia = Scale(angInertia, ScalarV(scale));

	Matrix34 boxMtx = MAT34V_TO_MATRIX34(mtx);
	Vector3 boxExtents = VEC3V_TO_VECTOR3(angInertia);

	grcDrawBox(boxExtents, boxMtx, boxColor);
}

void phBound::DrawSupport(Mat34V_In mtx, unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter)) const
{
    const int POINTS_THETA = (int)PFD_SupportPointsSamples.GetValue();
    const int POINTS_PHI = (int)PFD_SupportPointsSamples.GetValue();

    grcWorldMtx(mtx);
    for (int thetaIndex = 0; thetaIndex < POINTS_THETA; ++thetaIndex)
    {
        grcBegin(drawLineStrip, POINTS_PHI + 1);

        float theta = thetaIndex * PI * 2.0f / POINTS_THETA;
        for (int phiIndex = 0; phiIndex <= POINTS_PHI; ++phiIndex)
        {
            float phi = phiIndex * 2.0f * PI / POINTS_PHI;
            Vector3 direction(cosf(phi) * cosf(theta), sinf(phi), cosf(phi) * sinf(theta));
            Vector3 supportPoint = VEC3V_TO_VECTOR3(LocalGetSupportingVertexWithoutMarginNotInlined(direction));
            if (PFD_SupportMargin.WillDraw())
            {
                supportPoint.AddScaled(direction, VEC3V_TO_VECTOR3(GetMarginV()));
            }
            grcVertex3f(supportPoint);
        }

        grcEnd();
    }

    for (int phiIndex = 0; phiIndex < POINTS_PHI; ++phiIndex)
    {
        float phi = phiIndex * 2.0f * PI / POINTS_PHI;
        grcBegin(drawLineStrip, POINTS_THETA + 1);

        for (int thetaIndex = 0; thetaIndex <= POINTS_THETA; ++thetaIndex)
        {
            float theta = thetaIndex * PI * 2.0f / POINTS_THETA;
            Vector3 direction(cosf(phi) * cosf(theta), sinf(phi), cosf(phi) * sinf(theta));
            Vector3 supportPoint = VEC3V_TO_VECTOR3(LocalGetSupportingVertexWithoutMarginNotInlined(direction));
            if (PFD_SupportMargin.WillDraw())
            {
                supportPoint.AddScaled(direction, VEC3V_TO_VECTOR3(GetMarginV()));
            }
            grcVertex3f(supportPoint);
        }

        grcEnd();
    }
}
#endif

#if __NMDRAW
void phBound::NMRender(const Matrix34& mtx) const
{
  Matrix34 cgMtx;
  cgMtx.Set(mtx);
  mtx.Transform(GetCGOffset(),cgMtx.d);

  NMRenderBuffer::getInstance()->addAxis(NMDRAW_BOUNDS_AXES, 0.4f, cgMtx);
}
#endif // __NMDRAW

#if __DEV && !__SPU
void phBound::FormatBoundInfo(char* BoundStringBuffer, int bufSize, const phBound* Bound, int Component, int Element, Mat34V_In ParentMatrix)
{
	if(Bound == NULL)
	{
		return;
	}

	char* copyBuf = Alloca(char, bufSize);

	const phBound* workingBound = Bound;
	Mat34V workingMatrix = ParentMatrix;
	if(Bound->GetType() == phBound::COMPOSITE)
	{
		const phBoundComposite* selectedComposite = static_cast<const phBoundComposite*>(Bound);
		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%sCOMPOSITE(0x%p,Comp=%d): ", copyBuf, Bound, Component);

		workingBound = selectedComposite->GetBound(Component);
		if(workingBound)
		{
			Transform(workingMatrix, ParentMatrix, selectedComposite->GetCurrentMatrix(Component));
		}
		else
		{
			// This shouldn't happen
			formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
			formatf(BoundStringBuffer, bufSize, "%sNo such bound in that Component!", copyBuf);
			return;
		}
	}

	if(workingBound->GetType() == phBound::GEOMETRY ||
		workingBound->GetType() == phBound::BVH 
		USE_GRIDS_ONLY(|| workingBound->GetType() == phBound::GRID))
	{
		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%s%s(0x%p): ", copyBuf, workingBound->GetTypeString(), workingBound);

		const phBoundPolyhedron* boundPoly = static_cast<const phBoundPolyhedron*>(workingBound);
		const phPrimitive* prim;
		if(workingBound->GetType() == phBound::BVH)
		{
			prim = &static_cast<const phBoundBVH*>(workingBound)->GetPrimitive(Element);
		}
		else
		{
			prim = &boundPoly->GetPolygon(Element).GetPrimitive();
		}

		switch (prim->GetType())
		{
		case PRIM_TYPE_POLYGON:
			{
				const Vec3V normal = boundPoly->GetPolygonUnitNormal(Element);
				const rage::phPolygon* polyPrim = &prim->GetPolygon();
				float area = polyPrim->GetArea();
				Vec3V vert0 = boundPoly->GetVertex(polyPrim->GetVertexIndex(0));
				Vec3V vert1 = boundPoly->GetVertex(polyPrim->GetVertexIndex(1));
				Vec3V vert2 = boundPoly->GetVertex(polyPrim->GetVertexIndex(2));

				vert0 = Transform(workingMatrix, vert0);
				vert1 = Transform(workingMatrix, vert1);
				vert2 = Transform(workingMatrix, vert2);

				formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
				formatf(BoundStringBuffer, bufSize, "%sPOLYGON(Elem=%d): Area=%.5f, Norm=<%.3f, %.3f, %.3f>, Vert0=<%.3f, %.3f, %.3f>, Vert1=<%.3f, %.3f, %.3f>, Vert2=<%.3f, %.3f, %.3f>", copyBuf, Element, area, normal.GetXf(), normal.GetYf(), normal.GetZf(), vert0.GetXf(), vert0.GetYf(), vert0.GetZf(), vert1.GetXf(), vert1.GetYf(), vert1.GetZf(), vert2.GetXf(), vert2.GetYf(), vert2.GetZf());
			}
			break;
		case PRIM_TYPE_SPHERE:
			{
				const rage::phPrimSphere* spherePrim = &prim->GetSphere();
				Vec3V center = boundPoly->GetVertex(spherePrim->GetCenterIndex());
				const ScalarV radius = spherePrim->GetRadiusV();

				center = Transform(workingMatrix, center);

				formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
				formatf(BoundStringBuffer, bufSize, "%sSPHERE(Elem=%d): Center=<%.3f, %.3f, %.3f>, Radius=%.3f", copyBuf, Element, center.GetXf(), center.GetYf(), center.GetZf(), radius.Getf());
			}
			break;
		case PRIM_TYPE_CAPSULE:
			{
				const rage::phPrimCapsule* capsulePrim = &prim->GetCapsule();
				Vec3V end0 = boundPoly->GetVertex(capsulePrim->GetEndIndex0());
				Vec3V end1 = boundPoly->GetVertex(capsulePrim->GetEndIndex1());
				const ScalarV radius = capsulePrim->GetRadiusV();

				end0 = Transform(workingMatrix, end0);
				end1 = Transform(workingMatrix, end1);

				formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
				formatf(BoundStringBuffer, bufSize, "%sCAPSULE(Elem=%d): End0=<%.3f, %.3f, %.3f>, End1=<%.3f, %.3f, %.3f>, Radius=%.3f", copyBuf, Element, end0.GetXf(), end0.GetYf(), end0.GetZf(), end1.GetXf(), end1.GetYf(), end1.GetZf(), radius.Getf());
			}
			break;
		case PRIM_TYPE_BOX:
			{
				const rage::phPrimBox* boxPrim = &prim->GetBox();
				Vec3V vert0 = boundPoly->GetVertex(boxPrim->GetVertexIndex(0));
				Vec3V vert1 = boundPoly->GetVertex(boxPrim->GetVertexIndex(1));
				Vec3V vert2 = boundPoly->GetVertex(boxPrim->GetVertexIndex(2));
				Vec3V vert3 = boundPoly->GetVertex(boxPrim->GetVertexIndex(3));

				vert0 = Transform(workingMatrix, vert0);
				vert1 = Transform(workingMatrix, vert1);
				vert2 = Transform(workingMatrix, vert2);
				vert3 = Transform(workingMatrix, vert3);

				formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
				formatf(BoundStringBuffer, bufSize, "%sBOX(Elem=%d): Vert0=<%.3f, %.3f, %.3f>, Vert1=<%.3f, %.3f, %.3f>, Vert2=<%.3f, %.3f, %.3f>, Vert3=<%.3f, %.3f, %.3f>,", copyBuf, Element, vert0.GetXf(), vert0.GetYf(), vert0.GetZf(), vert1.GetXf(), vert1.GetYf(), vert1.GetZf(), vert2.GetXf(), vert2.GetYf(), vert2.GetZf(), vert3.GetXf(), vert3.GetYf(), vert3.GetZf());
			}
			break;
		case PRIM_TYPE_CYLINDER:
			{
				const rage::phPrimCylinder* cylinderPrim = &prim->GetCylinder();
				Vec3V end0 = boundPoly->GetVertex(cylinderPrim->GetEndIndex0());
				Vec3V end1 = boundPoly->GetVertex(cylinderPrim->GetEndIndex1());
				const ScalarV radius = cylinderPrim->GetRadiusV();

				end0 = Transform(workingMatrix, end0);
				end1 = Transform(workingMatrix, end1);

				formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
				formatf(BoundStringBuffer, bufSize, "%sCYLINDER(Elem=%d): End0=<%.3f, %.3f, %.3f>, End1=<%.3f, %.3f, %.3f>, Radius=%.3f", copyBuf, Element, end0.GetXf(), end0.GetYf(), end0.GetZf(), end1.GetXf(), end1.GetYf(), end1.GetZf(), radius.Getf());
			}
			break;
		default:
			{
				formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
				formatf(BoundStringBuffer, bufSize, "%s?Unsupported Primitive type?", copyBuf);
			}
			break;
		}
	}
	else if(workingBound->GetType() == phBound::SPHERE)
	{
		const phBoundSphere* sphereBound = static_cast<const phBoundSphere*>(workingBound);
		Vec3V center = sphereBound->GetCentroidOffset();
		const ScalarV radius = sphereBound->GetRadiusV();

		center = Transform(workingMatrix, center);

		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%sSPHERE: Center=<%.3f, %.3f, %.3f>, Radius=%.3f", copyBuf, center.GetXf(), center.GetYf(), center.GetZf(), radius.Getf());
	}
	else if(workingBound->GetType() == phBound::CAPSULE)
	{
		const phBoundCapsule* capsuleBound = static_cast<const phBoundCapsule*>(workingBound);
		Vec3V end0 = capsuleBound->GetEndPointA();
		Vec3V end1 = capsuleBound->GetEndPointB();
		const ScalarV radius = capsuleBound->GetRadiusV();

		end0 = Transform(workingMatrix, end0);
		end1 = Transform(workingMatrix, end1);

		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%sCAPSULE: End0=<%.3f, %.3f, %.3f>, End1=<%.3f, %.3f, %.3f>, Radius=%.3f", copyBuf, end0.GetXf(), end0.GetYf(), end0.GetZf(), end1.GetXf(), end1.GetYf(), end1.GetZf(), radius.Getf());
	}
	else if(workingBound->GetType() == phBound::CYLINDER)
	{
		const phBoundCylinder* cylinderBound = static_cast<const phBoundCylinder*>(workingBound);
		Vec3V center = cylinderBound->GetCentroidOffset();
		const ScalarV halfHeight = cylinderBound->GetHalfHeightV();

		center = Transform(workingMatrix, center);

		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%sCYLINDER: Center=<%.3f, %.3f, %.3f>, HalfHeight=%.3f", copyBuf, center.GetXf(), center.GetYf(), center.GetZf(), halfHeight.Getf());
	}
	else if(workingBound->GetType() == phBound::DISC)
	{
		const phBoundDisc* discBound = static_cast<const phBoundDisc*>(workingBound);
		Vec3V center = discBound->GetCentroidOffset();
		Vec3V axis = discBound->GetAxis();
		const ScalarV radius = discBound->GetRadiusV();
		const ScalarV margin = discBound->GetMarginV();

		center = Transform(workingMatrix, center);
		axis = Transform3x3(workingMatrix, axis);

		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%sDISC: Center=<%.3f, %.3f, %.3f>, Axis=<%.3f, %.3f, %.3f>, Radius=%.3f, Margin=%.3f", copyBuf, center.GetXf(), center.GetYf(), center.GetZf(), axis.GetXf(), axis.GetYf(), axis.GetZf(), radius.Getf(), margin.Getf());
	}
	else if(workingBound->GetType() == phBound::BOX)
	{
		const phBoundBox* boxBound = static_cast<const phBoundBox*>(workingBound);
		const Vec3V halfWidths = Scale(Subtract(boxBound->GetBoundingBoxMax(), boxBound->GetBoundingBoxMin()), ScalarV(V_HALF));
		Vec3V center = Add(boxBound->GetBoundingBoxMin(), halfWidths);

		center = Transform(workingMatrix, center);

		// This is actually sort of misleading information -- We transform the center to worldspace but not the halfwidths because halfwidths don't make sense in an oriented space anyway
		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%sBOX: Center=<%.3f, %.3f, %.3f>, HalfWidths=<%.3f, %.3f, %.3f>", copyBuf, center.GetXf(), center.GetYf(), center.GetZf(), halfWidths.GetXf(), halfWidths.GetYf(), halfWidths.GetZf());
	}
#if USE_GEOMETRY_CURVED
	else if(workingBound->GetType() == phBound::GEOMETRY_CURVED)
	{
		//const phBoundCurvedGeometry* boxBound = static_cast<const phBoundCurvedGeometry*>(workingBound);

		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%sCUREVEDGEOM: *", copyBuf);
	}
#endif // USE_GEOMETRY_CURVED
	else
	{
		formatf(copyBuf, bufSize, "%s", BoundStringBuffer);
		formatf(BoundStringBuffer, bufSize, "%sUnsupportedType!", copyBuf);
	}
}
#endif


bool phBound::IsTypeConcave(int type)
{
    switch (type)
    {
    USE_GRIDS_ONLY(case OCTREEGRID:)
	case BVH:
        return true;
    default:
		FastAssert(type < NUM_BOUND_TYPES);
        return false;
    }
}

bool phBound::IsTypeConvex(int type)
{
    switch (type)
    {
    case SPHERE:
    case CAPSULE:
	USE_TAPERED_CAPSULE_ONLY(case TAPERED_CAPSULE:)
    case BOX:
    case GEOMETRY:
	USE_GEOMETRY_CURVED_ONLY(case GEOMETRY_CURVED:)
    case TRIANGLE:
	case DISC:
	case CYLINDER:
	case COMPOSITE:
        return true;
    default:
        return false;
    }
}

bool phBound::IsTypeComposite(int type)
{
    return type == COMPOSITE;
}

#if !__SPU
int	phBound::GetShapeType() const
{
	return GetType();
}


//debugging
const char*	phBound::GetName()const
{
	return "RAGEBOUND";
}

#endif


//
// Resource code (must be at the bottom of the file)
//

#if !__SPU
void phBound::VirtualConstructFromPtr (datResource & rsc, phBound* Bound)
{
    phPolygon::EnableResourceConstructor(true);

    switch (Bound->GetType())
	{

	case SPHERE:
        ::new ((void *)Bound) phBoundSphere(rsc);
		break;
	case CAPSULE:
		::new ((void *)Bound) phBoundCapsule(rsc);
		break;
	case BOX:
		::new ((void *)Bound) phBoundBox(rsc);
		break;
	case GEOMETRY:
		::new ((void *)Bound) phBoundGeometry(rsc);
		break;
#if USE_GEOMETRY_CURVED
	case GEOMETRY_CURVED:
		::new ((void *)Bound) phBoundCurvedGeometry(rsc);
		break;
#endif
	case DISC:
		::new ((void *)Bound) phBoundDisc(rsc);
		break;
	case CYLINDER:
		::new ((void *)Bound) phBoundCylinder(rsc);
		break;
#if USE_GRIDS
	case GRID:
		::new ((void *)Bound) phBoundGrid(rsc);
		break;
#endif
#if USE_RIBBONS
	case RIBBON:
		::new ((void *)Bound) phBoundRibbon(rsc);
		break;
#endif
	case BVH:
		::new ((void *)Bound) phBoundBVH(rsc);
		break;
#if USE_SURFACES
	case SURFACE:
		::new ((void *)Bound) phBoundSurface(rsc);
		break;
#endif
	case COMPOSITE:
		::new ((void *)Bound) phBoundComposite(rsc);
		break;
	case PLANE:
		::new ((void *)Bound) phBoundPlane(rsc);
		break;
/*
	#undef BOUND_TYPE_INC
	#define BOUND_TYPE_INC(className,enumLabel,enumValue,stringName,isUsed) \
		case enumLabel: FastAssert(isUsed); ::new ((void *)Bound) className(rsc); break;
	#include "boundtypes.inc"
	#undef BOUND_TYPE_INC
*/

	default:
		if(sm_CustomResourceConstructor)													// do we want to load a user defined bound?
		{
			if(sm_CustomResourceConstructor(Bound, rsc))									// yep, so call the users function and check if we succeeded.
				break;																		// yep, we successfully loaded a user defined bound so lets get out of here.
		}
        Errorf("Unknown or unsupported bound type %d\n", Bound->GetType());
		AssertMsg(0 , "phBound::VirtualConstructFromPtr - unsupported or unknown bound type");
		break;
	}

    phPolygon::EnableResourceConstructor(false);
}

phBound::phBound (datResource & /*rsc*/)
{
}

#if __DECLARESTRUCT
void phBound::DeclareStruct(datTypeStruct &s)
{
	pgBase::DeclareStruct(s);
	STRUCT_BEGIN(phBound);
	STRUCT_FIELD(m_Type);
	STRUCT_FIELD(m_Flags);
	STRUCT_FIELD(m_PartIndex);
	STRUCT_FIELD(m_RadiusAroundCentroid);
	STRUCT_FIELD(m_BoundingBoxMaxXYZMarginW);
	STRUCT_FIELD(m_BoundingBoxMinXYZRefCountW);
	STRUCT_FIELD(m_CentroidOffsetXYZMaterialId0W);
	STRUCT_FIELD(m_CGOffsetXYZMaterialId1W);
	STRUCT_FIELD(m_VolumeDistribution);
	STRUCT_END();
}
#endif // __DECLARESTRUCT
#endif

#if __RESOURCECOMPILER
char phBound::s_currentBoundFilename[RAGE_MAX_PATH] = "";
char phBound::s_currentFragChildBoneName[64] = "";
#endif // __RESOURCECOMPILER

} // namespace rage
