//
// phbound/boundcapsule.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundtaperedcapsule.h"

#include "data/resource.h"
#include "data/struct.h"
#include "file/token.h"

#include "phbound/boundcapsule.h"
#include "phbound/boundpolyhedron.h"
#include "phbound/boundsphere.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "profile/profiler.h"
#include "vector/geometry.h"


namespace rage {

#if USE_TAPERED_CAPSULE

CompileTimeAssert(sizeof(phBoundTaperedCapsule) <= phBound::MAX_BOUND_SIZE);

#if !__SPU
////////////////////////////////////////////////////////////////
// profiling variables

namespace phBoundStats
{
	EXT_PF_TIMER(Capsule_Sphere);
	EXT_PF_TIMER(Capsule_Capsule);
};

using namespace phBoundStats;

////////////////////////////////////////////////////////////////

phBoundTaperedCapsule::phBoundTaperedCapsule ()
{
	m_MaterialId = phMaterialMgr::DEFAULT_MATERIAL_ID;
	m_Type = TAPERED_CAPSULE;
	SetCapsuleSize(1.0f,1.0f,1.0f);
}


void phBoundTaperedCapsule::SetCapsuleSize (float radiusA, float radiusB, float length)
{
	m_CapsuleRadius = radiusA;
	m_CapsuleRadius2 = radiusB;
	m_CapsuleLength = length;

	ScalarV radius1 = ScalarVFromF32(radiusA);
	ScalarV radius2 = ScalarVFromF32(radiusB);
	BoolV oneIsLess = IsLessThanOrEqual(radius1,radius2);
	SetMargin(SelectFT(oneIsLess,radius2,radius1));

	float radiusDiff = m_CapsuleRadius-m_CapsuleRadius2;
	float sine = radiusDiff*InvSqrtfSafe(square(radiusDiff)+square(m_CapsuleLength));
	float cosine = Sqrtf(1.0f-sine*sine);
	m_Sine = sine;
	m_Cosine = cosine;

	phBound::SetCentroidOffset(Vec3V(V_ZERO));
	CalculateExtents();
}


// convert a tapered into a tapered inflated by a radius
void phBoundTaperedCapsule::GetTaperedGeometryVsSphere( float otherRad, float &coneOffset, float &coneOffset2, float &coneRadius, float &coneRadius2 ) const
{
	float radius = m_CapsuleRadius+otherRad;
	float radius2 = m_CapsuleRadius2+otherRad;

	// get the offsets for the truncated cone start and end
	coneOffset = m_Sine*(radius);
	coneOffset2 = m_Sine*(radius2);

	coneRadius = m_Cosine*radius;
	coneRadius2 = m_Cosine*radius2;
}

void phBoundTaperedCapsule::GetTaperedGeometryVsSphere (ScalarV_In otherRad, ScalarV_InOut coneOffset, ScalarV_InOut coneOffset2, ScalarV_InOut coneRadius, ScalarV_InOut coneRadius2) const
{
	ScalarV vOtherRad = otherRad;
	ScalarV radius = Add(ScalarVFromF32(m_CapsuleRadius),vOtherRad);
	ScalarV radius2 = Add(ScalarVFromF32(m_CapsuleRadius2),vOtherRad);

	// get the offsets for the truncated cone start and end
	coneOffset = Scale(ScalarVFromF32(m_Sine),radius);
	coneOffset2 = Scale(ScalarVFromF32(m_Sine),radius2);
	coneRadius = Scale(ScalarVFromF32(m_Cosine),radius);
	coneRadius2 = Scale(ScalarVFromF32(m_Cosine),radius2);
}


phBoundTaperedCapsule::~phBoundTaperedCapsule ()
{
}


////////////////////////////////////////////////////////////////
// resources


#if !__SPU
phBoundTaperedCapsule::phBoundTaperedCapsule (datResource & rsc) : phBound(rsc)
{
}

#if __DECLARESTRUCT
void phBoundTaperedCapsule::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);
	STRUCT_BEGIN(phBoundTaperedCapsule);
	STRUCT_FIELD(m_CapsuleRadius);
	STRUCT_FIELD(m_CapsuleRadius2);
	STRUCT_FIELD(m_CapsuleLength);
	STRUCT_FIELD(m_Sine);
	STRUCT_FIELD(m_Cosine);
	STRUCT_FIELD(m_MaterialId);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

bool phBoundTaperedCapsule::ResourceCreate(phBound* Bound, datResource& rsc)
{
	if( Bound->GetType() == phBound::TAPERED_CAPSULE)
	{
		::new ((void *)Bound) phBoundTaperedCapsule(rsc);
		return true;
	}

	return false;
}
#endif // !__SPU

void phBoundTaperedCapsule::CalculateExtents ()
{
	m_RadiusAroundCentroid = 0.5f*m_CapsuleLength+Max(m_CapsuleRadius,m_CapsuleRadius2);
	Vec3V halfExtents = Vec3V(Max(m_CapsuleRadius,m_CapsuleRadius2),m_RadiusAroundCentroid,Max(m_CapsuleRadius,m_CapsuleRadius2));
	Vec3V cenOffset = GetCentroidOffset();
	SetBoundingBoxMin(cenOffset - halfExtents);
	SetBoundingBoxMax(cenOffset + halfExtents);

	Vector3 angInertia;
	phMathInertia::FindCapsuleAngInertia(1.0f,m_CapsuleRadius,m_CapsuleLength,&angInertia);
	m_VolumeDistribution.SetXYZ(RCC_VEC3V(angInertia));
	m_VolumeDistribution.SetWf(PI*square(m_CapsuleRadius)*(m_CapsuleLength+1.33333333333f*m_CapsuleRadius));
}


void phBoundTaperedCapsule::SetCentroidOffset (Vec3V_In offset)
{
	phBound::SetCentroidOffset(offset);
	CalculateExtents();
}


void phBoundTaperedCapsule::ShiftCentroidOffset (Vec3V_In offset)
{
	Vec3V newOffset = offset + GetCentroidOffset();
	SetCentroidOffset(newOffset);
}


////////////////////////////////////////////////////////////////////////////////
#endif // !__SPU

#if __WIN32
#pragma warning(disable:4189)
#endif

/*
Purpose: Find out if the given point is inside the capsule.
Parameters:
	point	- the point to test, relative to the capsule's center
Return: true if the given point is inside the capsule or on the surface, false if it is outside
Notes:
1.	This does not use Offset, so point must be relative to the capsule center. */
bool phBoundTaperedCapsule::IsInsideCapsule (const Vector3& point) const
{
	float m_coneOffset, m_coneOffset2, m_coneRadius, m_coneRadius2;
	//	if( m_otherSphereRadiusCache != radius )
	{
		// all results here have converted the sphere into a point
		GetTaperedGeometryVsSphere( 0.0f, m_coneOffset, m_coneOffset2, m_coneRadius, m_coneRadius2 );
	}

	float cLength = m_CapsuleLength + m_coneOffset2;
	float cStart = m_coneOffset;

	// Get the axis half-length, from the center of the capsule to the center of one hemisphere,
	// and the total half-length, from the center of the capsule to the edge of one hemisphere.
	float axisHalfLength = 0.5f*m_CapsuleLength;

	cLength -= axisHalfLength;
	cStart -= axisHalfLength;

	if( point.y < cStart )
	{
		return ( point.XZMag2()+square(point.y+axisHalfLength)<=square(m_CapsuleRadius) );
	}

	if( point.y > cLength )
	{
		return ( point.XZMag2()+square(point.y-axisHalfLength)<=square(m_CapsuleRadius2) );
	}

	float radiusAtPoint = (point.y - cStart)*m_Sine + m_coneRadius;

	float capsuleRadius2 = square(radiusAtPoint);
	
	return (point.XZMag2()<=capsuleRadius2);

}


/*
Purpose: See if the segment connecting the given points intersects the surface of the capsule.
Parameters:
	point1	- the first point, relative to the capsule's center
	point2	- the second point, relative to the capsule's center
	segT1	- reference to the segment t-value of the first intersection of the segment with the capsule's surface
	segT2	- reference to the segment t-value of the second intersection of the segment with the capsule's surface
	hdT1	- reference to the capsule axis t-value of the first intersection of the segment with the capsule's surface
	hdT2	- reference to the capsule axis t-value of the second intersection of the segment with the capsule's surface
	index1	- reference to the capsule section index number of the first intersection
	index2	- reference to the capsule section index number of the second intersection
Return: the number of intersections of the segment with the capsule surface */
int phBoundTaperedCapsule::SegmentToCapsuleIntersections (const Vector3& point1, const Vector3& point2, float& segT1, float& segT2,
													float& hdT1, float& hdT2, int& index1, int& index2) const
{
	// Initialize the t-values and index numbers.
	segT1 = -1.0f;
	segT2 = -1.0f;
	index1 = BAD_INDEX;
	index2 = BAD_INDEX;

	// Get the capsule axis half-length.
	float axisHalfLength = 0.5f*m_CapsuleLength;

	// Get the segment.
	Vector3 segment(point2);
	segment.Subtract(point1);

	float isect1y,isect2y;
	float segFlatMag2 = segment.XZMag2();
	if (segFlatMag2>SMALL_FLOAT)
	{
		// The segment has a x-z component that is not nearly zero.
		float capsuleRadius2 = square(m_CapsuleRadius);
		if (square(-segment.z*point1.x+segment.x*point1.z)>capsuleRadius2*segFlatMag2)
		{
			// The segment's infinite line does not come close enough to the axis to intersect the capsule.
			return 0;
		}

		// Find the y-values of the two points where the segment's infinite line passes through the capsule's inifinite shaft.
		float p1FlatMag2 = point1.XZMag2();
		float dot = point1.XZDot(segment);
		float b2m4ac = SqrtfSafe(square(dot)-segFlatMag2*(p1FlatMag2-capsuleRadius2));
		float inverse = 1.0f/segFlatMag2;
		segT1 = (-dot-b2m4ac)*inverse;
		segT2 = (-dot+b2m4ac)*inverse;
		isect1y = point1.y+segT1*segment.y;
		isect2y = point1.y+segT2*segment.y;

		// See if the first intersection is in the capsule's shaft.
		if (isect1y>=-axisHalfLength && isect1y<=axisHalfLength)
		{
			// The first intersection is with the capsule shaft.
			index1 = CAPSULE_SHAFT;
		}

		// See if the second intersection is in the capsule's shaft, and find out whether hemispheres should be tested
		// (if less than both intersections are with the shaft).
		bool hemisphereTest = true;
		if (isect2y>=-axisHalfLength && isect2y<=axisHalfLength)
		{
			// The second intersection is with the capsule shaft.
			index2 = 2;
			hemisphereTest = (index1!=2);
		}

		if (hemisphereTest)
		{
			// At least one intersection is not with the capsule shaft, so check hotdot hemispheres.
			float segMag2 = segFlatMag2 + square(segment.y);
			inverse = 1.0f/segMag2;
			if (isect1y<-axisHalfLength || isect2y<-axisHalfLength)
			{
				// One or both intersection might be with the bottom hemisphere.
				Vector3 tailP1(point1.x,point1.y+axisHalfLength,point1.z);
				float p1Mag2 = tailP1.Mag2();
				dot = tailP1.Dot(segment);
				b2m4ac = square(dot)-segMag2*(p1Mag2-square(m_CapsuleRadius));
				if (b2m4ac>SMALLEST_SQUARE)
				{
					b2m4ac = sqrtf(b2m4ac);
					if (isect1y<-axisHalfLength)
					{
						// The first intersection is with the bottom hemisphere.
						index1 = BOTTOM_HEMISPHERE;
						segT1 = (-dot-b2m4ac)*inverse;
					}
					if (isect2y<-axisHalfLength)
					{
						// The second intersection is with the bottom hemisphere.
						index2 = BOTTOM_HEMISPHERE;
						segT2 = (-dot+b2m4ac)*inverse;
					}
				}
			}

			if (isect1y>axisHalfLength || isect2y>axisHalfLength)
			{
				// One or both intersection might be with the top hemisphere.
				Vector3 tailP1(point1.x,point1.y-axisHalfLength,point1.z);
				float p1Mag2 = tailP1.Mag2();
				dot = tailP1.Dot(segment);
				b2m4ac = square(dot)-segMag2*(p1Mag2-square(m_CapsuleRadius));
				if (b2m4ac>SMALLEST_SQUARE)
				{
					b2m4ac = sqrtf(b2m4ac);
					if (isect1y>axisHalfLength)
					{
						// The first intersection is with the top hemisphere.
						index1 = TOP_HEMISPHERE;
						segT1 = (-dot-b2m4ac)*inverse;
					}
					if (isect2y>axisHalfLength)
					{
						// The second intersection is with the top hemisphere.
						index2 = TOP_HEMISPHERE;
						segT2 = (-dot+b2m4ac)*inverse;
					}
				}
			}
		}
	}
	else if (fabsf(segment.y)>SMALL_FLOAT)
	{
		// The segment is nearly parallel to the capsule axis.
		isect2y = square(m_CapsuleRadius)-point1.XZMag2();
		if (isect2y<=SMALLEST_SQUARE)
		{
			// The segment is on or outside the capsule surface, so there is no intersection.
			return 0;
		}

		// Find the y-values of the two intersections.
		isect2y = sqrtf(isect2y);
		isect1y = isect2y-axisHalfLength;
		float inverse = 1.0f/segment.y;
		index1 = BOTTOM_HEMISPHERE;
		segT1 = (isect1y-point1.y)*inverse;
		isect2y += axisHalfLength;
		index2 = TOP_HEMISPHERE;
		segT2 = (isect2y-point1.y)*inverse;
	}
	else
	{
		// The segment has nearly zero length, so there are no intersections.
		return 0;
	}

	if (index1!=BAD_INDEX && segT1>=0.0f && segT1<=1.0f)
	{
		// The first intersection is on the segment.
		hdT1 = m_CapsuleLength>SMALL_FLOAT ? (isect1y+axisHalfLength)/m_CapsuleLength : 0.5f;
		if (index2!=BAD_INDEX && segT2>=0.0f && segT2<=1.0f)
		{
			// The second intersection is on the segment.
			hdT2 = m_CapsuleLength>SMALL_FLOAT ? (isect2y+axisHalfLength)/m_CapsuleLength : 0.5f;
			if (segT1>segT2)
			{
				SwapEm(segT1,segT2);
				SwapEm(hdT1,hdT2);
				SwapEm(index1,index2);
			}

			// There are two intersections of the segment with the capsule surface.
			return 2;
		}

		// There is one intersection of the segment with the capsule surface.
		return 1;
	}

	if (index2!=BAD_INDEX && segT2>=0.0f && segT2<=1.0f)
	{
		// The second intersection is on the segment and the first is not, so swap them.
		segT1 = segT2;
		hdT1 = m_CapsuleLength>SMALL_FLOAT ? (isect2y+axisHalfLength)/m_CapsuleLength : 0.5f;
		index1 = index2;

		// There is one intersection of the segment with the capsule surface.
		return 1;
	}

	// There are no intersections of the segment with the capsule surface.
	return 0;
}


/*
Purpose: Find the normal vector pointing out from the capsule surface at the given position.
Parameters:
	position	- the position at which to get the normal vector, relative to the capsule's center
	partIndex	- the part of the capsule in which to get the normal (bottom hemisphere, top hemisphere or shaft)
Return: the normal vector pointing out from the capsule surface at the given position */
Vector3 phBoundTaperedCapsule::FindCapsuleIsectNormal (const Vector3& position, int partIndex) const
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
		normal.y -= 0.5f*m_CapsuleLength;
	}
	else
	{
		// The normal points out from the bottom hemisphere.
		Assert(partIndex==BOTTOM_HEMISPHERE);
		normal.Set(position);
		normal.y += 0.5f*m_CapsuleLength;
	}

	// Normalize and return the outward-pointing normal vector.
	normal.Normalize();
	return normal;
}

#if !__SPU
bool phBoundTaperedCapsule::CanBecomeActive () const
{
	return true;
}
#endif	// !__SPU

Vec3V_Out phBoundTaperedCapsule::LocalGetSupportingVertexWithoutMargin(Vec::V3Param128 vec, int* UNUSED_PARAM(vertexIndex)) const
{
	// Compute <0, y * 0.5, 0> and <0, y * -0.5, 0>
	Vector3 halfLengthInY = Vector3(m_CapsuleLength,m_CapsuleLength,m_CapsuleLength);
	halfLengthInY.Multiply(YAXIS);
	Vector3 negativeLengthInY;
	negativeLengthInY.Negate(halfLengthInY);
	halfLengthInY.Multiply(VEC3_HALF);

	// One end of the line segment
	Vector3 endA(VEC3V_TO_VECTOR3(GetCentroidOffset()));
	endA.Add(halfLengthInY);

	// The other end of the line segment
	Vector3 endB(endA);
	endB.Add(negativeLengthInY);

	Vector3 minRadius;
	minRadius.Min(Vector3(m_CapsuleRadius,m_CapsuleRadius,m_CapsuleRadius), Vector3(m_CapsuleRadius2,m_CapsuleRadius2,m_CapsuleRadius2));

	Vector3 vecNorm(vec);
	vecNorm.NormalizeSafe();

	Vector3 adjustedRadiusA;
	adjustedRadiusA.Subtract(Vector3(m_CapsuleRadius,m_CapsuleRadius,m_CapsuleRadius), minRadius);
	Vector3 supVecA(endA);
	supVecA.AddScaled(vecNorm, adjustedRadiusA);

	Vector3 adjustedRadiusB;
	adjustedRadiusB.Subtract(Vector3(m_CapsuleRadius2,m_CapsuleRadius2,m_CapsuleRadius2), minRadius);
	Vector3 supVecB(endB);
	supVecB.AddScaled(vecNorm, adjustedRadiusB);

	Vector3 v(vec);
	Vector3 dotA = supVecA.DotV(v);
	Vector3 dotB = supVecB.DotV(v);

	Vector3 maxDot = dotA.IsLessThanV(dotB);

	return VECTOR3_TO_VEC3V(maxDot.Select(supVecA, supVecB));
}

#if __PFDRAW
void phBoundTaperedCapsule::Draw(Mat34V_In mtxIn, bool UNUSED_PARAM(colorMaterials), bool solid, int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlags), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	const int STEPS = 8;

	Color32 newColor(grcCurrentColor);

    if (grcViewport::GetCurrent())
    {
	    Vec3V relPos;
	    relPos = UnTransformFull(mtxIn, grcViewport::GetCurrentCameraPosition());

		if (IsInsideCapsule(VEC3V_TO_VECTOR3(relPos)))
	    {
		    // The camera is inside the sphere.
		    newColor.SetGreen(Max(0, newColor.GetGreen() - 100));
		    newColor.SetBlue(Max(0, newColor.GetBlue() - 100));
	    }
    }

	bool oldLighting = true;

	if (!solid)
	{
		oldLighting = grcLighting(false);
	}

	grcColor(newColor);

	Matrix34 centroid(mtx);
	mtx.Transform3x3(VEC3V_TO_VECTOR3(GetCentroidOffset()),centroid.d);

	grcDrawTaperedCapsule(m_CapsuleLength, m_CapsuleRadius, m_CapsuleRadius2, centroid, STEPS, solid); 

	if (!solid)
	{
		grcLighting(oldLighting);
	}
}
#endif // __PFDRAW

#if !__SPU
/////////////////////////////////////////////////////////////////
// load / save

bool phBoundTaperedCapsule::Load_v110 (fiAsciiTokenizer& token)
{
	// Get the capsule length (the distance between the end sphere centers).
	token.MatchToken("length:");
	float length = token.GetFloat();

	// Get the first radius - it can be called radius, radiusA or radius0.
	if (!token.CheckToken("radiusA:") && !token.CheckToken("radius0:"))
	{
		token.MatchToken("radius:");
	}
	float radiusA = token.GetFloat();

	// Get the second radius, making it equal to the first if it's not specified in the file.
	float radiusB = radiusA;
	if (token.CheckToken("radiusB:") || token.CheckToken("radius2:"))
	{
		radiusB = token.GetFloat();
	}

	// Set the tapered capsule size.
	SetCapsuleSize(radiusA,radiusB,length);

	// See if there is a centroid offset.
	if (token.CheckToken("centroid:"))
	{
		// Get and set the centroid offset.
		Vector3 offset;
		token.GetVector(offset);
		SetCentroidOffset(RCC_VEC3V(offset));
	}

	// See if there is a center of gravity offset.
	if (token.CheckToken("cg:"))
	{
		// Get and set the center of gravity offset.
		Vector3 cg;
		token.GetVector(cg);
		SetCGOffset(RCC_VEC3V(cg));
	}
	else
	{
		// No center of gravity offset was specified, so use the centroid offset as the center of gravity offset.
		SetCGOffset(GetCentroidOffset());
	}

	if (token.CheckToken("materials:"))
	{
		// NumMaterials is always 1 in tapered capsules.
		int numLoadedMaterials;
		numLoadedMaterials = token.GetInt();
		Assert(numLoadedMaterials <= 1);
		if (numLoadedMaterials == 1)
		{
			m_MaterialId = GetMaterialIdFromFile(token);
		}
	}

	return true;
}


#if !__FINAL && !IS_CONSOLE
bool phBoundTaperedCapsule::Save_v110 (fiAsciiTokenizer & token)
{
	// capsule dimensions
	token.PutDelimiter("\n");
	token.PutDelimiter("length: ");
	token.Put(m_CapsuleLength);
	token.PutDelimiter("\n");
	token.PutDelimiter("radius: ");
	token.Put(m_CapsuleRadius);
	if (m_CapsuleRadius2!=m_CapsuleRadius)
	{
		token.PutDelimiter("radiusB: ");
		token.Put(m_CapsuleRadius2);
	}
	token.PutDelimiter("\n");

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
	token.Put(MATERIALMGR.GetMaterial(m_MaterialId).GetName());
	token.PutDelimiter("\n");

	return true;
}

#endif	// end of #if !__FINAL && !IS_CONSOLE
#endif // !__SPU

#endif // USE_TAPERED_CAPSULE

} // namespace rage

