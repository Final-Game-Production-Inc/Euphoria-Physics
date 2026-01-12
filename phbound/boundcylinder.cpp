//
// phbound/boundcylinder.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#include "boundcylinder.h"

#include "support.h"
#include "grprofile/drawmanager.h"

#include "vector/colors.h"

namespace rage {

CompileTimeAssert(sizeof(phBoundCylinder) <= phBound::MAX_BOUND_SIZE);
CompileTimeAssert(__TOOL || __64BIT || sizeof(phBoundCylinder) <= 128);


EXT_PFD_DECLARE_ITEM(DrawBoundMaterialNames);
EXT_PFD_DECLARE_GROUP(PolygonDensity);
EXT_PFD_DECLARE_GROUP(PrimitiveDensity);

#if __PFDRAW
void phBoundCylinder::Draw(Mat34V_In mtxIn, bool UNUSED_PARAM(colorMaterials), bool UNUSED_PARAM(solid), int UNUSED_PARAM(whichPolys), phMaterialFlags UNUSED_PARAM(highlightFlags), unsigned int UNUSED_PARAM(typeFilter), unsigned int UNUSED_PARAM(includeFilter), unsigned int UNUSED_PARAM(boundTypeFlags), unsigned int UNUSED_PARAM(boundIncludeFlags)) const
{
	if(PFDGROUP_PolygonDensity.GetEnabled() || PFDGROUP_PrimitiveDensity.GetEnabled())
	{
		return;
	}
	Mat34V mtxToUse(mtxIn.GetMat33(), Transform(mtxIn, GetCentroidOffset()));

	grcDrawCylinder(2.0f * GetHalfHeight(), GetRadius(), RCC_MATRIX34(mtxToUse), 20, false);

	if(PFD_DrawBoundMaterialNames.GetEnabled())
	{
		// Write the material name on the screen at the average location of its vertices.
		grcColor(Color_white);
		grcDrawLabelf(VEC3V_TO_VECTOR3(mtxToUse.d()),GetMaterial(0).GetName());
	}
}
#endif	// __PFDRAW


#if !__SPU

/////////////////////////////////////////////////////////////////
// load / save

bool phBoundCylinder::Load_v110 (fiAsciiTokenizer & token)
{
	// Need to set margin first - SetCylinderRadiusAndHalfHeight recalculates extents and will be wrong if the margin isn't set yet
	float margin = CONVEX_DISTANCE_MARGIN;
	if (token.CheckToken("margin:"))
	{
		margin = token.GetFloat();
	}
	SetMargin(margin);

	float halfHeight, radius;
	token.MatchToken("length:");
	halfHeight = 0.5f * token.GetFloat();
	token.MatchToken("radius:");
	radius = token.GetFloat();

	SetCylinderRadiusAndHalfHeight(radius, halfHeight);

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
bool phBoundCylinder::Save_v110 (fiAsciiTokenizer & token)
{
	token.PutDelimiter("\n");
	token.PutDelimiter("margin: ");
	token.Put(GetMargin());

	// Cylinder dimensions
	token.PutDelimiter("\n");
	token.PutDelimiter("length: ");
	token.Put(GetHeight());
	token.PutDelimiter("\n");
	token.PutDelimiter("radius: ");
	token.Put(GetRadius());
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
	WriteMaterialIdToFile(token, GetMaterialId(0));
	token.PutDelimiter("\n");

	return true;
}


#endif	// end of #if !__FINAL && !IS_CONSOLE

#endif // !__SPU

int phBoundCylinder::TestAgainstSegment(Vec3V_In point, Vec3V_In segment, ScalarV_InOut segmentT1, Vec3V_InOut normal1, ScalarV_InOut segmentT2, Vec3V_InOut normal2) const
{
	ScalarV squaredRadius = ScalarVFromF32(GetRadius()*GetRadius());
	ScalarV halfHeight = ScalarVFromF32(GetHalfHeight());

	// Calculate the segment projected onto the y-axis and the xz-plane
	Vec3V projectedXZpoint = Vec3V(point.GetX(), ScalarV(V_ZERO), point.GetZ());
	Vec3V projectedXZsegment = Vec3V(segment.GetX(), ScalarV(V_ZERO), segment.GetZ());
	ScalarV projectedYpoint = point.GetY();
	ScalarV projectedYsegment = segment.GetY();

	// Determine the T values of the infinite line intersecting the infinite tube of the cylinder using the quadratic formula
	ScalarV tubeT1, tubeT2;
	BoolV areQuadraticResultsValid = geomTValues::RealQuadraticRoots(Dot(projectedXZsegment, projectedXZsegment), Dot(projectedXZpoint, projectedXZsegment), Subtract(Dot(projectedXZpoint, projectedXZpoint), squaredRadius), tubeT1, tubeT2);

	// If the quadratic results are invalid, explicitly set the T values
	tubeT1 = SelectFT(areQuadraticResultsValid, ScalarV(V_NEG_FLT_MAX), tubeT1);
	tubeT2 = SelectFT(areQuadraticResultsValid, ScalarV(V_NEG_FLT_MAX), tubeT2);
	
	// The normals of the tube intersection are the hit positions projected onto the xz-plane
	Vec3V tubeNormal1 = Add(projectedXZpoint, Scale(projectedXZsegment, tubeT1));
	Vec3V tubeNormal2 = Add(projectedXZpoint, Scale(projectedXZsegment, tubeT2));

	// Calculate the T values of the infinite line intersecting the planes at the top and bottom of the cylinder
	ScalarV inverseProjectedYsegment = Invert(projectedYsegment);

	// If the segment doesn't lie in the xy-plane then the infinite line intersects the planes at the top and bottom of the cylinder
	BoolV isSegmentInXYplane = IsClose(projectedYsegment, ScalarV(V_ZERO), ScalarV(V_FLT_SMALL_6));
	ScalarV planeTtop = SelectFT(isSegmentInXYplane, Scale(Subtract(halfHeight, projectedYpoint), inverseProjectedYsegment), ScalarV(V_NEG_FLT_MAX));
	ScalarV planeTbottom = SelectFT(isSegmentInXYplane, Scale(Subtract(Negate(halfHeight), projectedYpoint), inverseProjectedYsegment), ScalarV(V_NEG_FLT_MAX));

	// Decide which plane was intersected first
	BoolV isPlaneTtopCloser = IsLessThan(planeTtop, planeTbottom);
	ScalarV planeT1 = SelectFT(isPlaneTtopCloser, planeTbottom, planeTtop);
	ScalarV planeT2 = SelectFT(isPlaneTtopCloser, planeTtop, planeTbottom);

	// The normals of the plane intersection are just the hit positions projected onto the y-axis
	Vec3V planeNormal1 = Vec3V(ScalarV(V_ZERO), Add(projectedYpoint, Scale(planeT1, projectedYsegment)), ScalarV(V_ZERO));
	Vec3V planeNormal2 = Vec3V(ScalarV(V_ZERO), Add(projectedYpoint, Scale(planeT2, projectedYsegment)), ScalarV(V_ZERO));

	// Tube T values are only valid if the position they represent is inside the top and bottom planes of the cylinder
	BoolV isTubeT1Valid = IsLessThan(Abs(Add(projectedYpoint, Scale(tubeT1, projectedYsegment))), halfHeight);
	BoolV isTubeT2Valid = IsLessThan(Abs(Add(projectedYpoint, Scale(tubeT2, projectedYsegment))), halfHeight);

	// Plane T values are only valid if the position they represent is inside the infinite tube of the cylinder
	BoolV isPlaneT1Valid = IsLessThan(MagSquared(Add(projectedXZpoint, Scale(planeT1, projectedXZsegment))), squaredRadius);
	BoolV isPlaneT2Valid = IsLessThan(MagSquared(Add(projectedXZpoint, Scale(planeT2, projectedXZsegment))), squaredRadius);

	// Use whichever of the tube or plane T value is valid
	// If both of them are valid, they should give the same point
	// If neither the plane nor tube T value is valid, return -1
	ScalarV lineT1 = SelectFT(isTubeT1Valid, SelectFT(isPlaneT1Valid, ScalarV(V_NEGONE), planeT1), tubeT1);
	ScalarV lineT2 = SelectFT(isTubeT2Valid, SelectFT(isPlaneT2Valid, ScalarV(V_NEGONE), planeT2), tubeT2);

	// Choose normals the same way T values were chosen
	Vec3V lineNormal1 = Normalize(SelectFT(isTubeT1Valid, SelectFT(isPlaneT1Valid, Vec3V(V_ZERO), planeNormal1), tubeNormal1));
	Vec3V lineNormal2 = Normalize(SelectFT(isTubeT2Valid, SelectFT(isPlaneT2Valid, Vec3V(V_ZERO), planeNormal2), tubeNormal2));

	// Determine if the T values on the infinite line are inside the segment (0 <= T <= 1)
	BoolV isLineT1Valid = And(IsGreaterThanOrEqual(lineT1, ScalarV(V_ZERO)), IsLessThanOrEqual(lineT1, ScalarV(V_ONE)));
	BoolV isLineT2Valid = And(IsGreaterThanOrEqual(lineT2, ScalarV(V_ZERO)), IsLessThanOrEqual(lineT2, ScalarV(V_ONE)));
	
	// If the second intersection is valid and the first isn't (segment starts inside cylinder) swap the results
	BoolV swapResults = And(!isLineT1Valid, isLineT2Valid);
	segmentT1 = SelectFT(swapResults, lineT1, lineT2);
	segmentT2 = SelectFT(swapResults, lineT2, lineT1);
	normal1 = SelectFT(swapResults, lineNormal1, lineNormal2);
	normal2 = SelectFT(swapResults, lineNormal2, lineNormal1);

	// Calculate the number of valid intersections
	ScalarV numValidIntersectionsV = SelectFT(isLineT1Valid, SelectFT(isLineT2Valid, ScalarV(V_ZERO), ScalarV(V_INT_1)), SelectFT(isLineT2Valid, ScalarV(V_INT_1), ScalarV(V_INT_2)));

	// Return the number of valid intersections as an int
	int numValidIntersections;
	StoreScalar32FromScalarV(numValidIntersections, numValidIntersectionsV);
	return numValidIntersections;
}

} // namespace rage