//
// phbound/boundcapsule.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundcapsule.h"

#include "boundsphere.h"

#include "data/resource.h"
#include "data/struct.h"
#include "file/token.h"
#include "math/simplemath.h"
#include "phcore/material.h"
#include "phcore/materialmgr.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "vector/geometry.h"

#if __NMDRAW
#include "nmext/NMRenderBuffer.h"
#endif

#include "boundcapsule_parser.h"

namespace rage {

#if !__TOOL 
CompileTimeAssert(sizeof(phBoundCapsule) <= phBound::MAX_BOUND_SIZE);
CompileTimeAssert(sizeof(phBoundCapsule) <= 128 || __64BIT);
#endif

#if !__SPU
const Vector3 VEC3_ZERO_HALF_ZERO(0.0f, 0.5f, 0.0f);
const Vector3 VEC3_ZERO_MINUS_HALF_ZERO(0.0f, -0.5f, 0.0f);

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
	EXT_PF_TIMER(Capsule_Sphere);
	EXT_PF_TIMER(Capsule_Capsule);
};

using namespace phBoundStats;

#endif // !__SPU

////////////////////////////////////////////////////////////////

phBoundCapsule::phBoundCapsule ()
#if USE_CAPSULE_EXTRA_EXTENTS
	:	m_CapsuleHalfHeight(0.0f)
#endif
{
	m_Type = CAPSULE;
	phBound::SetCentroidOffset(Vec3V(V_ZERO));
	SetCapsuleSize(ScalarV(V_ONE),ScalarV(V_ONE));
	memset(pad, 0, sizeof(pad));
}


void phBoundCapsule::SetCapsuleRadius (float radius)
{
	SetCapsuleSize(ScalarVFromF32(radius),GetLengthV());
}


void phBoundCapsule::SetCapsuleRadius (ScalarV_In radius)
{
	SetCapsuleSize(radius,GetLengthV());
}

void phBoundCapsule::SetMargin (float margin)
{
	SetCapsuleRadius(margin);
}

void phBoundCapsule::SetMargin (ScalarV_In margin)
{
	SetCapsuleRadius(margin);
}

void phBoundCapsule::SetCapsuleLength (float length)
{
	SetCapsuleSize(GetRadiusV(),ScalarVFromF32(length));
}


void phBoundCapsule::SetCapsuleLength (ScalarV_In length)
{
	SetCapsuleSize(GetRadiusV(),length);
}


void phBoundCapsule::SetCapsuleSize (float radius, float length)
{
	SetCapsuleSize(ScalarVFromF32(radius),ScalarVFromF32(length));
}


void phBoundCapsule::SetCapsuleSize (ScalarV_In radius, ScalarV_In length)
{
	Assert(IsGreaterThanAll(radius,ScalarV(V_ZERO)));
	Assert(IsGreaterThanOrEqualAll(length,ScalarV(V_ZERO)));
	phBound::SetMargin(radius);
	SetRadiusAroundCentroid(Add(Scale(length,ScalarV(V_HALF)),radius));
	phBound::SetCentroidOffset(Vec3V(V_ZERO));
	CalculateExtents();
}


phBoundCapsule::~phBoundCapsule ()
{
}

#if !__SPU
void phBoundCapsule::Copy (const phBound* original)
{
	Assert(phBound::CAPSULE==original->GetType());
	*this = *static_cast<const phBoundCapsule*>(original);
	SetRefCount(1);
}


////////////////////////////////////////////////////////////////
// resources


phBoundCapsule::phBoundCapsule (datResource & rsc) : phBound(rsc)
{
}

#if __DECLARESTRUCT
void phBoundCapsule::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);
	STRUCT_BEGIN(phBoundCapsule);
#if USE_CAPSULE_EXTRA_EXTENTS
	STRUCT_FIELD(m_CapsuleHalfHeight);
#endif
	STRUCT_CONTAINED_ARRAY(pad);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

#endif // !__SPU

void phBoundCapsule::CalculateExtents ()
{
	ScalarV radius = GetRadiusV();
	ScalarV length = GetLengthV();
	Vec3V halfExtents(radius,GetRadiusAroundCentroidV(),radius);
	Vec3V cenOffSet = GetCentroidOffset();
	SetBoundingBoxMin(cenOffSet - halfExtents);
	SetBoundingBoxMax(cenOffSet + halfExtents);

	// Compute the volume distribution from the capsule shape.
	Vector3 angInertia;
	phMathInertia::FindCapsuleAngInertia(1.0f,radius.Getf(),length.Getf(),&angInertia);
	m_VolumeDistribution.SetXYZ(RCC_VEC3V(angInertia));
	m_VolumeDistribution.SetWf(PI*square(radius.Getf())*(length.Getf()+1.33333333333f*radius.Getf()));
}


void phBoundCapsule::SetCentroidOffset (Vec3V_In offset)
{
	phBound::SetCentroidOffset(offset);
	CalculateExtents();
}


void phBoundCapsule::ShiftCentroidOffset (Vec3V_In offset)
{
	SetCentroidOffset(GetCentroidOffset() + offset);
}

#if !__SPU

////////////////////////////////////////////////////////////////////////////////


const Vec3V phBoundCapsule::GetEndPointA () const
{
	return AddScaled(GetCentroidOffset(), Vec3V(V_Y_AXIS_WZERO), GetHalfLengthV());
}

const Vec3V phBoundCapsule::GetEndPointB () const
{
	return SubtractScaled(GetCentroidOffset(), Vec3V(V_Y_AXIS_WZERO), GetHalfLengthV());
}


////////////////////////////////////////////////////////////////////////////////


/*
Purpose: Find out if the given point is inside the capsule.
Parameters:
	point	- the point to test, relative to the capsule's center
Return: true if the given point is inside the capsule or on the surface, false if it is outside
Notes:
1.	This does not use Offset, so point must be relative to the capsule center. */
bool phBoundCapsule::IsInsideCapsule (const Vector3& point) const
{
	// Get the axis half-length, from the center of the capsule to the center of one hemisphere,
	// and the total half-length, from the center of the capsule to the edge of one hemisphere.
	float axisHalfLength = 0.5f*GetLength();
	float totalHalfLength = axisHalfLength+GetRadius();

	if (point.y>totalHalfLength)
	{
		// The point is off the top of the capsule.
		return false;
	}

	if (point.y<-totalHalfLength)
	{
		// The point is under the bottom of the capsule.
		return false;
	}

	float capsuleRadius2 = square(GetRadius());
	if (point.XZMag2()>capsuleRadius2)
	{
		// The point is outside the capsule's radius.
		return false;
	}

	if (point.y>axisHalfLength)
	{
		if (point.XZMag2()+square(point.y-axisHalfLength)>capsuleRadius2)
		{
			// The point is outside the top hemisphere.
			return false;
		}
	}
	else if (point.y<-axisHalfLength && (point.XZMag2()+square(point.y+axisHalfLength)>capsuleRadius2))
	{
		// The point is outside the bottom hemisphere.
		return false;
	}

	// The point is inside the capsule.
	return true;
}

#endif // !__SPU

int phBoundCapsule::SegmentToCapsuleIntersections (Vec3V_In point1, Vec3V_In point2, ScalarV_In capsuleLength, ScalarV_In capsuleRadius, ScalarV_InOut segT1, ScalarV_InOut segT2,
													ScalarV_InOut capsuleT1, ScalarV_InOut capsuleT2, int& index1, int& index2)
{

	// Initialize the t-values and index numbers.
	segT1 = ScalarV(V_NEGONE);
	segT2 = ScalarV(V_NEGONE);
	index1 = BAD_INDEX;
	index2 = BAD_INDEX;

	// Get the capsule axis half-length.
	ScalarV axisHalfLength = Scale(capsuleLength,ScalarV(V_HALF));

	// Get the segment.
	Vec3V segment = Subtract(point2,point1);

	ScalarV point1Y = SplatY(point1);
	ScalarV segmentX = SplatX(segment);
	ScalarV segmentY = SplatY(segment);
	ScalarV segmentZ = SplatZ(segment);
	ScalarV segFlatMag2 = Add(Scale(segmentX,segmentX),Scale(segmentZ,segmentZ));
	ScalarV isect1y,isect2y;
	ScalarV capsuleRadius2 = Scale(capsuleRadius,capsuleRadius);
	if (IsGreaterThanAll(segFlatMag2,ScalarV(V_FLT_SMALL_6)))
	{
		// The segment has a x-z component that is not nearly zero.
		ScalarV point1X = SplatX(point1);
		ScalarV point1Z = SplatZ(point1);
		ScalarV crossY = Subtract(Scale(segmentX,point1Z),Scale(segmentZ,point1X));
		if (IsGreaterThanAll(Scale(crossY,crossY),Scale(capsuleRadius2,segFlatMag2)))
		{
			// The segment's infinite line does not come close enough to the axis to intersect the capsule.
			return 0;
		}

		// Find the y-values of the two points where the segment's infinite line passes through the capsule's inifinite shaft.
		ScalarV p1FlatMag2 = Add(Scale(point1X,point1X),Scale(point1Z,point1Z));
		ScalarV dot = Add(Scale(point1X,segmentX),Scale(point1Z,segmentZ));
		ScalarV b2m4ac = SqrtSafe(Subtract(Scale(dot,dot),Scale(segFlatMag2,Subtract(p1FlatMag2,capsuleRadius2))));
		ScalarV inverse = Invert(segFlatMag2);
		segT1 = -Scale(Add(b2m4ac,dot),inverse);
		segT2 = Scale(Subtract(b2m4ac,dot),inverse);
		isect1y = Add(point1Y,Scale(segT1,segmentY));
		isect2y = Add(point1Y,Scale(segT2,segmentY));

		// See if the first intersection is in the capsule's shaft.
		if (IsGreaterThanOrEqualAll(isect1y,-axisHalfLength) && IsLessThanOrEqualAll(isect1y,axisHalfLength))
		{
			// The first intersection is with the capsule shaft.
			index1 = CAPSULE_SHAFT;
		}

		// See if the second intersection is in the capsule's shaft, and find out whether hemispheres should be tested
		// (if less than both intersections are with the shaft).
		bool hemisphereTest = true;
		if (IsGreaterThanOrEqualAll(isect2y,-axisHalfLength) && IsLessThanOrEqualAll(isect2y,axisHalfLength))
		{
			// The second intersection is with the capsule shaft.
			index2 = 2;
			hemisphereTest = (index1!=2);
		}

		if (hemisphereTest)
		{
			// At least one intersection is not with the capsule shaft, so check the hemispheres.
			ScalarV segMag2 = Add(segFlatMag2,Scale(segmentY,segmentY));
			inverse = Invert(segMag2);
			if (IsLessThanAll(isect1y,-axisHalfLength) || IsLessThanAll(isect2y,-axisHalfLength))
			{
				// One or both intersection might be with the bottom hemisphere.
				Vec3V tailP1 = AddScaled(point1,Vec3V(V_Y_AXIS_WZERO),axisHalfLength);
				ScalarV p1Mag2 = MagSquared(tailP1);
				dot = Dot(tailP1,segment);
				b2m4ac = Subtract(Scale(dot,dot),Scale(segMag2,Subtract(p1Mag2,capsuleRadius2)));
				if (IsGreaterThanAll(b2m4ac,ScalarV(V_FLT_SMALL_12)))
				{
					b2m4ac = Sqrt(b2m4ac);
					if (IsLessThanAll(isect1y,-axisHalfLength))
					{
						// The first intersection is with the bottom hemisphere.
						index1 = BOTTOM_HEMISPHERE;
						segT1 = -Scale(Add(b2m4ac,dot),inverse);
					}
					if (IsLessThanAll(isect2y,-axisHalfLength))
					{
						// The second intersection is with the bottom hemisphere.
						index2 = BOTTOM_HEMISPHERE;
						segT2 = Scale(Subtract(b2m4ac,dot),inverse);
					}
				}
			}

			if (IsGreaterThanAll(isect1y,axisHalfLength) || IsGreaterThanAll(isect2y,axisHalfLength))
			{
				// One or both intersection might be with the top hemisphere.
				Vec3V tailP1 = SubtractScaled(point1,Vec3V(V_Y_AXIS_WZERO),axisHalfLength);
				ScalarV p1Mag2 = MagSquared(tailP1);
				dot = Dot(tailP1,segment);
				b2m4ac = Subtract(Scale(dot,dot),Scale(segMag2,Subtract(p1Mag2,capsuleRadius2)));
				if (IsGreaterThanAll(b2m4ac,ScalarV(V_FLT_SMALL_12)))
				{
					b2m4ac = Sqrt(b2m4ac);
					if (IsGreaterThanAll(isect1y,axisHalfLength))
					{
						// The first intersection is with the top hemisphere.
						index1 = TOP_HEMISPHERE;
						segT1 = -Scale(Add(b2m4ac,dot),inverse);
					}
					if (IsGreaterThanAll(isect2y,axisHalfLength))
					{
						// The second intersection is with the top hemisphere.
						index2 = TOP_HEMISPHERE;
						segT2 = Scale(Subtract(b2m4ac,dot),inverse);
					}
				}
			}
		}
	}
	else if (IsGreaterThanAll(segmentY,ScalarV(V_FLT_SMALL_6)) || IsLessThanAll(segmentY,-ScalarV(V_FLT_SMALL_6)))
	{
		// The segment is nearly parallel to the capsule axis.
		ScalarV point2X = SplatX(point2);
		ScalarV point2Z = SplatZ(point2);
		isect2y = Subtract(capsuleRadius2,Add(Scale(point2X,point2X),Scale(point2Z,point2Z)));
		if (IsLessThanOrEqualAll(isect2y,ScalarV(V_FLT_SMALL_12)))
		{
			// The segment is on or outside the capsule surface, so there is no intersection.
			return 0;
		}

		// Find the y-values of the two intersections.
		isect2y = Sqrt(isect2y);
		isect2y = Add(isect2y,axisHalfLength);
		isect1y = -isect2y;
		index1 = BOTTOM_HEMISPHERE;
		index2 = TOP_HEMISPHERE;
		ScalarV point2Y = SplatY(point2);
		if (IsGreaterThanAll(point1Y,point2Y))
		{
			SwapEm(isect1y,isect2y);
			SwapEm(index1,index2);
		}

		ScalarV inverse = Invert(segmentY);
		segT1 = Scale(Subtract(isect1y,point1Y),inverse);
		segT2 = Scale(Subtract(isect2y,point1Y),inverse);
	}
	else
	{
		// The segment has nearly zero length, so there are no intersections.
		return 0;
	}

	ScalarV inverseLength = Invert(capsuleLength);
	if (index1!=BAD_INDEX && IsGreaterThanOrEqualAll(segT1,ScalarV(V_ZERO)) && IsLessThanOrEqualAll(segT1,ScalarV(V_ONE)))
	{
		// The first intersection is on the segment.
		capsuleT1 = (IsGreaterThanAll(capsuleLength,ScalarV(V_FLT_SMALL_6)) ? Scale(Add(isect1y,axisHalfLength),inverseLength) : ScalarV(V_HALF));
		if (index2!=BAD_INDEX && IsGreaterThanOrEqualAll(segT2,ScalarV(V_ZERO)) && IsLessThanOrEqualAll(segT2,ScalarV(V_ONE)))
		{
			// The second intersection is on the segment.
			capsuleT2 = (IsGreaterThanAll(capsuleLength,ScalarV(V_FLT_SMALL_6)) ? Scale(Add(isect2y,axisHalfLength),inverseLength) : ScalarV(V_HALF));
			if (IsGreaterThanAll(segT1,segT2))
			{
				SwapEm(segT1,segT2);
				SwapEm(capsuleT1,capsuleT2);
				SwapEm(index1,index2);
			}

			// There are two intersections of the segment with the capsule surface.
			return 2;
		}

		// There is one intersection of the segment with the capsule surface.
		return 1;
	}

	if (index2!=BAD_INDEX && IsGreaterThanOrEqualAll(segT2,ScalarV(V_ZERO)) && IsLessThanOrEqualAll(segT2,ScalarV(V_ONE)))
	{
		// The second intersection is on the segment and the first is not, so swap them.
		segT1 = segT2;
		capsuleT1 = (IsGreaterThanAll(capsuleLength,ScalarV(V_FLT_SMALL_6)) ? Scale(Add(isect2y,axisHalfLength),inverseLength) : ScalarV(V_HALF));
		index1 = index2;

		// There is one intersection of the segment with the capsule surface.
		return 1;
	}

	// There are no intersections of the segment with the capsule surface.
	return 0;
}

#if !__SPU
bool phBoundCapsule::CanBecomeActive () const
{
	return true;
}

#if __PFDRAW
void phBoundCapsule::Draw(Mat34V_In mtxIn, bool colorMaterials, bool solid, int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlags), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	if(PFDGROUP_PolygonDensity.GetEnabled() || PFDGROUP_PrimitiveDensity.GetEnabled())
	{
		return;
	}

	const Matrix34 mtx = RCC_MATRIX34(mtxIn);

	const int STEPS = 8;

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
	else if (IsInsideCapsule(relPos))
	{
		// The camera is inside the capsule
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
	grcDrawCapsule(GetLength(), GetRadius(), offsetMtx, STEPS, solid); 

	if (PFD_DrawBoundMaterialNames.GetEnabled())
	{
		// Write the polygon index number on the screen at the average location of its vertices.
		grcColor(Color_white);
		grcDrawLabelf(position,GetMaterial(0).GetName());
	}

	if (!solid)
	{
		grcLighting(oldLighting);
	}
}
#endif // __PFDRAW

#if __NMDRAW
void phBoundCapsule::NMRender(const Matrix34& mtx) const
{
  phBound::NMRender(mtx);
  Matrix34 offsetMtx;
  mtx.Transform(m_CentroidOffset, offsetMtx.d);
  offsetMtx.Set3x3(mtx);
  NMRenderBuffer::getInstance()->addCapsule(NMDRAW_BOUNDS, GetLength(), GetRadius(), offsetMtx, 6, Vector3(1, 0, 1));
}
#endif // __NMDRAW
/////////////////////////////////////////////////////////////////
// load / save

bool phBoundCapsule::Load_v110 (fiAsciiTokenizer & token)
{
	float length, radius;
	token.MatchToken("length:");
	length = token.GetFloat();
	token.MatchToken("radius:");
	radius = token.GetFloat();
	SetCapsuleSize (radius,length);

#if USE_CAPSULE_EXTRA_EXTENTS
	if (token.CheckToken("halfheight:"))
	{
		SetHalfHeight(token.GetFloat());
	}
#endif

	if (token.CheckToken("centroid:"))
	{
		Vector3 offset;
		token.GetVector(offset);
		SetCentroidOffset(RCC_VEC3V(offset));
	}

	if (token.CheckToken("cg:"))
	{
		Vector3 cg;
		token.GetVector(cg);
		SetCGOffset(RCC_VEC3V(cg));
	}
	else
	{
		// No center of gravity offset was specified, so use Offset as default m_CGOffset.
		SetCGOffset(GetCentroidOffset());
	}

	if (token.CheckToken("materials:"))
	{
		// NumMaterials is always 1 in capsules
		int numLoadedMaterials;
		numLoadedMaterials = token.GetInt();

		Assert(numLoadedMaterials <= 1);
		if (numLoadedMaterials == 1)
		{
			SetPrimitiveMaterialId(GetMaterialIdFromFile(token));
		}
		else
		{
			// keep the default material
		}
	}

	return true;
}


#if !__FINAL && !IS_CONSOLE
bool phBoundCapsule::Save_v110 (fiAsciiTokenizer & token)
{
	// capsule dimensions
	token.PutDelimiter("\n");
	token.PutDelimiter("length: ");
	token.Put(GetLength());
	token.PutDelimiter("\n");
	token.PutDelimiter("radius: ");
	token.Put(GetRadius());
	token.PutDelimiter("\n");

#if USE_CAPSULE_EXTRA_EXTENTS
	token.PutDelimiter("halfheight: ");
	token.Put(GetHalfHeight());
	token.PutDelimiter("\n");
#endif

	// centroid offset
	if (!IsEqualAll(GetCentroidOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("centroid: ");
		token.Put(GetCentroidOffset());
		token.PutDelimiter("\n");
	}

	// cg offset
	if (!IsEqualAll(GetCGOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("cg: ");
		token.Put(GetCGOffset());
		token.PutDelimiter("\n");
	}

	token.PutDelimiter("\n");

	// materials
	token.PutDelimiter("materials: ");
	token.Put(GetNumMaterials());
	token.PutDelimiter("\n");
	WriteMaterialIdToFile(token, GetMaterialId(0));
	token.PutDelimiter("\n");

	return true;
}

#endif	// end of #if !__FINAL && !IS_CONSOLE

#endif // !__SPU

} // namespace rage

