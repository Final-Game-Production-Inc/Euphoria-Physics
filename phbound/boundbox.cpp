//
// phbound/boundbox.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "boundbox.h"

#include "boundsphere.h"

#include "data/resource.h"
#include "data/struct.h"
#include "file/token.h"
#include "math/simplemath.h"
#include "phcore/constants.h"
#include "phcore/convexpoly.h"
#include "phcore/material.h"
#include "phcore/materialmgr.h"
#include "phcore/phmath.h"
#include "phcore/segment.h"
#include "profile/profiler.h"
#include "system/timemgr.h"
#include "vector/geometry.h"
#include "phbound/support.h"
#include "grprofile/drawmanager.h"

#if __XENON
#include <xdk.h>
#include "system/xtl.h"
#endif

namespace rage {

CompileTimeAssert(sizeof(phBoundBox) <= phBound::MAX_BOUND_SIZE);
CompileTimeAssert(sizeof(phBoundBox) <= 128);

////////////////////////////////////////////////////////////////
// profiling variables

EXT_PFD_DECLARE_ITEM(DrawBoundMaterialNames);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDistanceOpacity);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundSolidOpacity);
EXT_PFD_DECLARE_GROUP(PolygonDensity);
EXT_PFD_DECLARE_GROUP(PrimitiveDensity);

#if !__SPU
namespace phBoundStats
{
	EXT_PF_TIMER(Box_Capsule);
	EXT_PF_TIMER(Box_Box);
	EXT_PF_TIMER(TP_Box);
};

using namespace phBoundStats;
#endif

using namespace geomPoints;

////////////////////////////////////////////////////////////////

#if !__SPU

// two face index numbers for each edge, in no special order
const u16 phBoundBox::FaceFromEdge[12][2]=
{
	{2,4},
	{1,4},
	{3,4},
	{0,4},
	{2,5},
	{1,5},
	{3,5},
	{0,5},
	{0,2},
	{1,2},
	{1,3},
	{0,3}
};


// two corner index numbers for each edge ordered so that for EdgeIndex<8, EdgeIndex=Corner1 and
// for EdgeIndex>=8, EdgeIndex=Corner1+8
const u16 phBoundBox::CornerFromEdge[12][2]=
{
	{0,1},
	{1,2},
	{2,3},
	{3,0},
	{4,5},
	{5,6},
	{6,7},
	{7,4},
	{0,4},
	{1,5},
	{2,6},
	{3,7}
};

const Vector3 phBoundBox::UnitCorners[8] =
{
	Vector3(0.5f,	0.5f,	0.5f),
	Vector3(-0.5f,	0.5f,	0.5f),
	Vector3(-0.5f,	-0.5f,	0.5f),
	Vector3(0.5f,	-0.5f,	0.5f),
	Vector3(0.5f,	0.5f,	-0.5f),
	Vector3(-0.5f,	0.5f,	-0.5f),
	Vector3(-0.5f,	-0.5f,	-0.5f),
	Vector3(0.5f,	-0.5f,	-0.5f)
};

// six normal vectors for six faces in local coordinates
const Vector3 phBoundBox::FaceNormals[6]=
{
	Vector3(1,	0,	0),
	Vector3(-1,	0,	0),
	Vector3(0,	1,	0),
	Vector3(0,	-1,	0),
	Vector3(0,	0,	1),
	Vector3(0,	0,	-1)
};

// six normal vectors pointing out from the center toward each of 8 corners in local coordinates
const Vector3 phBoundBox::CornerNormals[8]=
{
	Vector3(1,	1,	1),
	Vector3(-1,	1,	1),
	Vector3(-1,	-1,	1),
	Vector3(1,	-1,	1),
	Vector3(1,	1,	-1),
	Vector3(-1,	1,	-1),
	Vector3(-1,	-1,	-1),
	Vector3(1,	-1,	-1)
};

#endif


// Return the index number of the edge in common between the two given faces.
int GetEdgeBetweenTwoFaces(int faceA, int faceB)
{
	int face0,face1;
	for(int index=0;index<12;index++)
	{
		face0=phBoundBox::FaceFromEdge[index][0];
		face1=phBoundBox::FaceFromEdge[index][1];
		if((face0==faceA && face1==faceB) || (face0==faceB && face1==faceA))
		{
			return index;
		}
	}
	return BAD_INDEX;
}


// Find the closest box face to a point inside the box.
int FindClosestBoxFaceInside (const Vector3& halfWidths, const Vector3& point)
{
	float distance,bestDist=FLT_MAX;
	int closestFace = -1;
	for (int axisIndex=0; axisIndex<3; axisIndex++)
	{
		distance = halfWidths[axisIndex]-point[axisIndex];
		if (distance<bestDist)
		{
			bestDist = distance;
			closestFace = 2*axisIndex;
		}

		distance = point[axisIndex]+halfWidths[axisIndex];
		if (distance<bestDist)
		{
			bestDist = distance;
			closestFace = 2*axisIndex+1;
		}
	}

	Assert(closestFace>=0);
	return closestFace;
}


/*
Purpose:
- Constructor that initializes the size of the box.
Parameters:
- size : The dimensions of the box.
Return:
- None.
*/
phBoundBox::phBoundBox (const Vector3 &size/* = Vector3(1.0f,1.0f,1.0f)*/)
{
	m_Type = BOX;
	SetMaterial(phMaterialMgr::DEFAULT_MATERIAL_ID);

	Assertf(size.x>0.0f && size.y>0.0f && size.z>0.0f,"box bound has a bad size: x=%f, y=%f, z=%f",size.x,size.y,size.z);
	Assert(IsZeroAll(GetCentroidOffset()));

	Vec3V boxMax = Scale(ScalarV(V_HALF),  RCC_VEC3V(size));
	Vec3V boxMin = -boxMax;
	SetBoundingBox(boxMin, boxMax);

	CalculateExtents();
}


/*
Purpose:
- Destructor.
Parameters:
- None.
Return:
- None.
*/
phBoundBox::~phBoundBox()
{
}


/*
Purpose:
- Resource constructor.
Parameters:
- rsc : Resource from which to construct.
Return:
- None.
*/
#if !__SPU
phBoundBox::phBoundBox (datResource & rsc) : phBound(rsc)
{
	//CalculateExtents();
}

#if __DECLARESTRUCT
void phBoundBox::DeclareStruct(datTypeStruct &s)
{
	phBound::DeclareStruct(s);
	STRUCT_BEGIN(phBoundBox);
	STRUCT_END();
}
#endif // __DECLARESTRUCT


/////////////////////////////////////////////////////////////////
// load / save

bool phBoundBox::Load_v110 (fiAsciiTokenizer & token)
{
	Vector3 temp;

	// box size
	token.MatchToken("size:");
	token.GetVector(temp);
	Vec3V boxMax = Scale(ScalarV(V_HALF),  RCC_VEC3V(temp));
	Vec3V boxMin = -boxMax;
	SetBoundingBox(boxMin, boxMax);

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

	// materials
	if (token.CheckToken("materials:"))
	{
		int numMaterialsToLoad;
		numMaterialsToLoad = token.GetInt();

		if (numMaterialsToLoad==1)
		{
			SetPrimitiveMaterialId(GetMaterialIdFromFile(token));
		}
	}

	CalculateExtents();

	return true;
}
#endif


#if !__FINAL && !IS_CONSOLE && !__SPU
bool phBoundBox::Save_v110 (fiAsciiTokenizer & token)
{
	token.PutDelimiter("\n");
	token.PutDelimiter("size: ");
	Vector3 boxSize = VEC3V_TO_VECTOR3(GetBoundingBoxSize());
	token.Put(boxSize);
	token.PutDelimiter("\n");

	if(!IsEqualAll(GetCentroidOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("centroid: ");
		token.Put(GetCentroidOffset());
		token.PutDelimiter("\n");
	}

	if(!IsEqualAll(GetCGOffset(),Vec3V(V_ZERO)))
	{
		token.PutDelimiter("cg: ");
		token.Put(GetCGOffset());
		token.PutDelimiter("\n");
	}

	token.PutDelimiter("\n");

	token.PutDelimiter("materials: ");
	token.Put(GetNumMaterials());
	token.PutDelimiter("\n");
	WriteMaterialIdToFile(token, GetMaterialId(0));
	token.PutDelimiter("\n");

	return true;
}


#endif	// end of #if !__FINAL && !IS_CONSOLE


////////////////////////////////////////////////////////////////
// box definition functions

/*
Purpose:
- Adjust the size of the box.  This should handle all the aspects necessary to re-size an already configured box.
Parameters:
- size : Vector3 defining the dimensions of the box along its principal axes.
Return:
- None.
*/
void phBoundBox::SetBoxSize (Vec3V_In size)
{
	Assertf(size.GetXf()>0.0f && size.GetYf()>0.0f && size.GetZf()>0.0f,"box bound has a bad size: x=%f, y=%f, z=%f",size.GetXf(),size.GetYf(),size.GetZf());

	Vec3V halfExtents = Scale(size, ScalarV(V_HALF));
	SetBoundingBoxMin(GetCentroidOffset() - halfExtents);
	SetBoundingBoxMax(GetCentroidOffset() + halfExtents);

	CalculateExtents();
}


/*
Purpose:
- Specify where the origin for the bound is, relative to its 'natural center'.
Parameters:
- offset : The rage_new offset of the origin.
Return:
- None.
Notes:
- See phBound::SetCentroidOffset().
*/
void phBoundBox::SetCentroidOffset (Vec3V_In offset)
{
	phBound::SetCentroidOffset(offset);

	Vec3V halfExtents = Scale(GetBoxSize(), ScalarV(V_HALF));
	SetBoundingBoxMin(GetCentroidOffset() - halfExtents);
	SetBoundingBoxMax(GetCentroidOffset() + halfExtents);

	CalculateExtents();
}


void phBoundBox::ShiftCentroidOffset (Vec3V_In offset)
{
	SetCentroidOffset(GetCentroidOffset() + offset);
}


/*
Purpose:
- Calculate the extents of this bound.
Parameters:
- None.
Return:
- None.
Notes:
- This function calculates both the minimal bounding box and minimal bounding sphere for this bound type.
*/
void phBoundBox::CalculateExtents ()
{
	Vec3V boxSize = GetBoxSize();
	m_RadiusAroundCentroid = 0.5f * Mag(boxSize).Getf();

	// Compute the volume distribution from the bounding box.
	Vector3 angInertia;
	phMathInertia::FindBoxAngInertia(1.0f,boxSize.GetXf(),boxSize.GetYf(),boxSize.GetZf(),&angInertia);
	m_VolumeDistribution.SetXYZ(RCC_VEC3V(angInertia));
	m_VolumeDistribution.SetW(boxSize.GetX()*boxSize.GetY()*boxSize.GetZ());

	// Clamp the collision margin at 1/8 the minimum box extent, so that small boxes don't have effective collision corners that are too round.
	float maxMargin = 0.125f*MinElement(boxSize).Getf();
	float margin = Min(GetMargin(), maxMargin);
	SetMargin(margin);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#if !__SPU
bool phBoundBox::CanBecomeActive () const
{
	return true;
}
#endif

#if __PFDRAW
void phBoundBox::Draw(Mat34V_In mtxIn, bool colorMaterials, bool solid, int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlags), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	if(PFDGROUP_PolygonDensity.GetEnabled() || PFDGROUP_PrimitiveDensity.GetEnabled())
	{
		return;
	}
	const Matrix34& mtx = RCC_MATRIX34(mtxIn);
	Color32 newColor(grcCurrentColor);

	if (colorMaterials)
	{
		newColor = MATERIALMGR.GetDebugColor(GetMaterial(0));
	}

	bool oldLighting = true;

	if (solid)
	{
		newColor.SetAlpha(int(255 * PFD_BoundSolidOpacity.GetValue()));
	}
	else
	{
		Vec3V relPos(V_ZERO);

#if !DISABLE_DRAW_GRCVIEWPORT_GETCURRENT
		if (grcViewport::GetCurrent())
		{
			relPos = UnTransformFull(mtxIn,grcViewport::GetCurrentCameraPosition());
		}
#endif

		float maxDrawDistance = PFD_BoundDrawDistance.GetValue();
		float distance = Min(maxDrawDistance,SqrtfSafe(MagSquared(relPos).Getf()));
		float inverseDrawDistance = InvertSafe(maxDrawDistance);
		newColor.SetAlpha(int(255 * (1.0f-(1.0f-PFD_BoundDistanceOpacity.GetValue())*distance*inverseDrawDistance)));
		oldLighting = grcLighting(false);
	}

	grcColor(newColor);

	Matrix34 centerMtx = mtx;
	mtx.Transform(VEC3V_TO_VECTOR3(GetCentroidOffset()), centerMtx.d);

	if (solid)
	{
		grcDrawSolidBox(VEC3V_TO_VECTOR3(GetBoundingBoxSize()), centerMtx, newColor);
	}
	else
	{
		grcDrawBox(VEC3V_TO_VECTOR3(GetBoundingBoxSize()), centerMtx, newColor);
	}

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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////  BOX-BOX COLLISION CODE  ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Find the closest box face to a line segment that crosses one box face plane.  borderT is the fraction of the distance
// from startPos to endPos where the face plane crossing occurs, borderIndex is the box face index number, regionType
// and regionIndex are vert, edge or poly and its index number for the part of the box hit by the capsule.
void phBoundBox::FindIsectBoxPlaneRegion (float borderT, int borderIndex, const Vector3& halfWidths, float radius, Vector3& startPos, Vector3& endPos, Vector3& axisPart,
											int* regionType, int* regionIndex, bool directed)
{
	Vector3 position(startPos);
	position.AddScaled(axisPart,borderT);
	Vector3 positionAbs(fabsf(position.x),fabsf(position.y),fabsf(position.z));
	// Make axis1 the axis index of the crossed box face plane (0,1,2)==(x,y,z).
	int axis1=borderIndex/2;
	// Make axis2 and axis3 the other two axes.
	int axis2=(axis1+1)%3;
	int axis3=(axis2+1)%3;
	bool startIsCloser,tooClose;
	// For directed capsules, tooClose tells whether the point is closer than the radius from the box
	// (non-directed capsules intersect at the deepest point; directed capsules intersect at the first touch).
	if (positionAbs[axis2]>=halfWidths[axis2])
	{
		// The border crossing's position along axis2 is outside or on the box.
		if (positionAbs[axis3]<halfWidths[axis3])
		{
			// The border crossing's position along axis3 is inside the box.
			startIsCloser = (fabsf(startPos[axis2])<fabsf(endPos[axis2]));
			tooClose = (positionAbs[axis2]<halfWidths[axis2]+radius);
		}
		else
		{
			// The border crossing's position along axis3 is outside or on the box.
			startIsCloser = (square(startPos[axis2])+square(startPos[axis3])<square(endPos[axis2])+square(endPos[axis3]));
			tooClose = (square(positionAbs[axis2]-halfWidths[axis2])+square(positionAbs[axis3]-halfWidths[axis3])<square(radius));
		}
	}
	else
	{
		// The border crossing's position along axis2 is inside the box.
		if (positionAbs[axis3]>=halfWidths[axis3])
		{
			// The border crossing's position along axis3 is outside or on the box.
			startIsCloser = (fabsf(startPos[axis3])<fabsf(endPos[axis3]));
			tooClose = (positionAbs[axis3]<halfWidths[axis3]+radius);
		}
		else
		{
			// The border crossing's position along axis3 is inside the box.
			startIsCloser = fabsf(startPos[axis1])<fabsf(endPos[axis1]);
			tooClose = true;
		}
	}

	if((directed && (tooClose || startIsCloser)) || (!directed && startIsCloser))
	{
		endPos.Set(position);
	}
	else
	{
		startPos.Set(position);
	}

	axisPart.Subtract(endPos,startPos);
	position.Average(startPos,endPos);
	phBoundBox::FindBoxPlaneRegion(halfWidths,position,*regionType,*regionIndex);
	if(*regionType!=BAD_INDEX || directed)
	{
		// A region outside the box was found, or no region outside the box was found and the test capsule is directed.
		return;
	}
	// The the intersection is inside the box (only for non-directed capsules).
	int startFace=FindClosestBoxFaceInside(halfWidths,startPos);
	int endFace=FindClosestBoxFaceInside(halfWidths,endPos);
	if(startFace==endFace)
	{
		// The two end points have the same closest box face, so the intersection is with that face.
		*regionType=GEOM_POLYGON;
		*regionIndex=startFace;
		return;
	}
	if((endFace!=startFace+1 || startFace%2!=0) && (startFace!=endFace+1 || endFace%2!=0))
	{
		// The two end points have as their closest box face index numbers any pair other than the opposite
		// faces along each axis (0,1) (2,3) or (4,5), so the box faces are neighboring, and the intersection
		// will either be with one of the two faces or their connecting edge.
		float length2=startPos.Dist(endPos);
		float distStart2=square(halfWidths[axis2]-fabsf(startPos[axis2]))+square(halfWidths[axis3]-fabsf(startPos[axis3]));
		float distEnd2=square(halfWidths[axis2]-fabsf(endPos[axis2]))+square(halfWidths[axis3]-fabsf(endPos[axis3]));
		if(length2>fabsf(distStart2-distEnd2))
		{
			// The axis portion and the box edge between these two faces overlap.
			*regionType=GEOM_EDGE;
			*regionIndex=GetEdgeBetweenTwoFaces(startFace,endFace);
			return;
		}
		// The axis portion and the box edge do not overlap, so find the box face (startFace or endFace)
		// that has the least penetration.
		*regionType=GEOM_POLYGON;
		int startFaceAxis=startFace/2;
		float distStartFace=halfWidths[startFaceAxis]-Min(fabsf(startPos[startFaceAxis]),fabsf(endPos[startFaceAxis]));
		int endFaceAxis=endFace/2;
		float distEndFace=halfWidths[endFaceAxis]-Min(fabsf(startPos[endFaceAxis]),fabsf(endPos[endFaceAxis]));
		if(distStartFace<distEndFace)
		{
			*regionIndex=startFace;
			return;
		}
		*regionIndex=endFace;
		return;
	}
	// The two end points have opposite faces as their closest box faces, so find which of the other four
	// faces has the shallowest penetration.
	float distance,bestDistance=FLT_MAX,higherEnd,lowerEnd;
	int faceIndex,nearestFace=0;
	for(int axisIndex=0;axisIndex<3;axisIndex++)
	{
		if(startPos[axisIndex]>endPos[axisIndex])
		{
			higherEnd=startPos[axisIndex];
			lowerEnd=endPos[axisIndex];
		}
		else
		{
			higherEnd=endPos[axisIndex];
			lowerEnd=startPos[axisIndex];
		}
		faceIndex=2*axisIndex;
		if(faceIndex!=startFace && faceIndex!=endFace)
		{
			distance=halfWidths[axisIndex]-lowerEnd;
			if(distance<bestDistance)
			{
				nearestFace=faceIndex;
				bestDistance=distance;
			}
		}
		faceIndex++;
		if(faceIndex!=startFace && faceIndex!=endFace)
		{
			distance=halfWidths[axisIndex]+higherEnd;
			if(distance<bestDistance)
			{
				nearestFace=faceIndex;
				bestDistance=distance;
			}
		}
	}
	*regionType=GEOM_POLYGON;
	*regionIndex=nearestFace;
}


} // namespace rage
