#ifndef PHYSICS_COLLISIONOVERLAPTEST_H
#define PHYSICS_COLLISIONOVERLAPTEST_H

#include "phcore/constants.h"
#include "physics/physicsprofilecapture.h"
#include "vector/geometry.h"
#include "phbullet/ContinuousConvexCollision.h"
#include "phbound/bound.h"
#include "contact.h"

namespace rage
{

// **** COT: Collision Overlap Test ****

// **** COT_ACE_Expand: Expand bounding volumes to account for the Active Collision Epsilon. ****
// ****					Call any of these for only 1 of the 2 bounding volumes in an overlap test. ****
#if USE_ACTIVE_COLLISION_DISTANCE

__forceinline Vec3V_Out COT_ACE_ExpandBoxHalfWidthForTest(Vec3V_In boxHalfWidth)
{
	//return boxHalfWidth + Vec3V(CONTACT_PENETRATION_OFFSET_V);
	return boxHalfWidth + Vec3V(ACTIVE_COLLISION_DISTANCE_V);
	//return boxHalfWidth - Vec3V(POSITIVE_DEPTH_ONLY_THRESHOLD_V);
	//return boxHalfWidth + Vec3V(phManifold::GetManifoldMarginV());
}

__forceinline ScalarV_Out COT_ACE_ExpandRadiusForTest(ScalarV_In radius)
{
	//return radius + CONTACT_PENETRATION_OFFSET_V; 
	return radius + ACTIVE_COLLISION_DISTANCE_V; 
	//return radius - POSITIVE_DEPTH_ONLY_THRESHOLD_V;
	//return radius + phManifold::GetManifoldMarginV();
}

__forceinline void COT_ACE_HalfExpandBoxHalfWidthForSAP(Vec3V_InOut boxHalfExtents)
{
	//boxHalfExtents += Vec3V(HALF_CONTACT_PENETRATION_OFFSET_V);
	boxHalfExtents += Vec3V(HALF_ACTIVE_COLLISION_DISTANCE_V);
	//boxHalfExtents -= Vec3V(ScalarV(V_HALF) * POSITIVE_DEPTH_ONLY_THRESHOLD_V);
	//boxHalfExtents += Vec3V(ScalarV(V_HALF) * phManifold::GetManifoldMarginV());
}

#else // USE_ACTIVE_COLLISION_DISTANCE

__forceinline Vec3V_Out COT_ACE_ExpandBoxHalfWidthForTest(Vec3V_In boxHalfWidth)
{
	return boxHalfWidth;
}

__forceinline ScalarV_Out COT_ACE_ExpandRadiusForTest(ScalarV_In radius)
{
	return radius; 
}

__forceinline void COT_ACE_HalfExpandBoxHalfWidthForSAP(Vec3V_InOut boxHalfExtents)
{
	(void)boxHalfExtents;
}

#endif // USE_ACTIVE_COLLISION_DISTANCE


// **** COT_Test: Bounding volume overlap tests. ****
__forceinline bool COT_TestBoxToBoxOBB(Vec3V_In boxAHalfWidths, Vec3V_In boxBHalfWidths, Mat34V_In aToB)
{
	PPC_STAT_COUNTER_INC(NodeOverlapTestCounter,1);
	return geomBoxes::TestBoxToBoxOBB(VEC3V_TO_VECTOR3(COT_ACE_ExpandBoxHalfWidthForTest(boxAHalfWidths)), RCC_VECTOR3(boxBHalfWidths), RCC_MATRIX34(aToB));
}

__forceinline bool COT_TestBoxToBoxOBBFaces(Vec3V_In boxAHalfWidths, Vec3V_In boxBHalfWidths, Mat34V_In aToB)
{
	PPC_STAT_COUNTER_INC(NodeOverlapTestCounter,1);
	return geomBoxes::TestBoxToBoxOBBFaces(VEC3V_TO_VECTOR3(COT_ACE_ExpandBoxHalfWidthForTest(boxAHalfWidths)), RCC_VECTOR3(boxBHalfWidths), RCC_MATRIX34(aToB));
}

/*
__forceinline bool COT_TestBoxToBoxOBBFaces(Vec3V_In boxAHalfWidths, Vec3V_In boxBHalfWidths, QuatV_In aToB_Orientation, Vec3V_In aToB_Position)
{
	return geomBoxes::TestBoxToBoxOBBFaces(boxAHalfWidths, boxBHalfWidths, aToB_Orientation, aToB_Position).Getb();
}
*/

__forceinline bool COT_TestPolygonToOrientedBoxFP(Vec3V_In v0, Vec3V_In v1, Vec3V_In v2, Vec3V_In polygonNormal, Vec3V_In boxCenter, Mat34V_In boxOrientation, Vec3V_In boxHalfExtents)
{
	return geomBoxes::TestPolygonToOrientedBoxFP(RCC_VECTOR3(v0),RCC_VECTOR3(v1),RCC_VECTOR3(v2),RCC_VECTOR3(polygonNormal),RCC_VECTOR3(boxCenter),RCC_MATRIX34(boxOrientation),VEC3V_TO_VECTOR3(COT_ACE_ExpandBoxHalfWidthForTest(boxHalfExtents)));
}

__forceinline int COT_TestSphereToAABB(Vec3V_In sphereCenter, ScalarV_In sphereRadius, Vec3V_In boxMin, Vec3V_In boxMax)
{
	// This is about as simple and cheap of a box vs sphere test that I think there can be.  Just clamp the sphere center such that it is not exterior to the box
	//   (picking the closest point on the surface of the box if the sphere center starts off outside) and then check how much of a move that is.  If it moved by
	//   more than the radius of the sphere then the sphere did not intersect the box in its initial position.
	const Vec3V vClampedPoint = Clamp(sphereCenter, boxMin, boxMax);
	const ScalarV expandedSphereRadius = COT_ACE_ExpandRadiusForTest(sphereRadius);
	const int retVal = IsLessThanOrEqualAll(DistSquared(vClampedPoint, sphereCenter), expandedSphereRadius * expandedSphereRadius);
	return retVal;
}

__forceinline bool COT_TestCapsuleToAlignedBoxFP(Vec3V_In capsuleEnd0, Vec3V_In capsuleEnd1, ScalarV_In capsuleRadius, Vec3V_In boxMin, Vec3V_In boxMax)
{
	return geomBoxes::TestCapsuleToAlignedBoxFP(capsuleEnd0.GetIntrin128(), capsuleEnd1.GetIntrin128(), COT_ACE_ExpandRadiusForTest(capsuleRadius).GetIntrin128(), boxMin.GetIntrin128(), boxMax.GetIntrin128());
}

__forceinline bool COT_TestAABBtoAABB_CenterHalfSize(Vec3V_In boxACenter, Vec3V_In boxAHalfSize, Vec3V_In boxBCenter, Vec3V_In boxBHalfSize)
{
	return geomBoxes::TestAABBtoAABB_CenterHalfSize(boxACenter,COT_ACE_ExpandBoxHalfWidthForTest(boxAHalfSize),boxBCenter,boxBHalfSize).Getb();
}

#define USE_CORRECT_EXPANDED_BOUNDING_VOLUME 1		// This enables computing motion bounding volumes that take into account how CCD interprets the motion of a bound.
#define USE_SAFE_LAST_MATRIX_IN_BOUNDING_VOLUME 1	// This enables taking into account the safe last matrix when computing motion bounding volumes.

// **** COT_Expand: Expand bounding volumes to account for motion. ****
__forceinline void COT_ExpandNonSimpleBoundOBBFromMotion(Mat34V_In current, Mat34V_In last, Vec3V_In boxHalfWidth, Vec3V_In boxCenter, Vec3V_InOut expanded_boxHalfWidth, Vec3V_InOut expanded_boxCenter)
{
	// The bounding box is given in local space.
	
	// To compute the motion bounding volume for a single bound:
	// Translation = (CurrentMat - LastMat) * BoundCenter.
	// TranslationLoc = UnTransform3x3Ortho(CurrentMat,Translation)
	// LastAABBMin = AABBMin - TranslationLoc
	// LastAABBMax = AABBMax - TranslationLoc
	// AABBMotionMin = Min(LastAABBMin,AABBMin)
	// AABBMotionMax = Max(LastAABBMax,AABBMax)

	// To compute the motion bounding volume for a composite or bvh we have to consider
	// all possible centers that could be used to compute the CCD translation:
	// Let L = Identity - CurrentMat^-1 * LastMat, so that TranslationLoc = L * PossibleCenter
	// Let c = PossibleCenter and assume all centers are within the unexpanded bounding volume.
	// LastAABBMin = Minimize (AABBMin - L * c) , s.t. AABBMin <= c <= AABBMax
	// LastAABBMax = Maximize (AABBMax - L * c) , s.t. AABBMin <= c <= AABBMax
	// AABBMotionMin = Min(LastAABBMin,AABBMin)
	// AABBMotionMax = Min(LastAABBMax,AABBMax)
	// The extrema can be determined by calculating the support against the box using the corresponding row of L as the support direction. 
	// LastAABB can be written as:
	// LR is the rotation of L, LP is the position of L.
	// LastAABBMin = Minimize (AABBMin - L * (c + boxCenter)) , s.t. -halfWidth <= c <= +halfWidth
	//			   = Minimize (AABBMin - (LR * c) - (LR * boxCenter + LP)) , s.t. -halfWidth <= c <= +halfWidth
	// LastAABBMax = Maximize (AABBMax - (LR * c) - (LR * boxCenter + LP)) , s.t. -halfWidth <= c <= +halfWidth
	// Everything is constant expect (LR * c).

	// Resolve the last matrix into the current matrix local space.
	Mat34V bToA;
	UnTransformOrtho( bToA, current, last );

	const Vec3V v_zero(V_ZERO);
	const ScalarV v_half(V_HALF);

/*
	// Naive calculation.
	const Mat34V identityMat(V_IDENTITY);

	const Mat34V L = identityMat - bToA;
	Mat34V Ltp;
	Transpose3x3(Ltp,L);

	const VecBoolV bx_ = IsGreaterThanOrEqual(Ltp.GetCol0(),v_zero);
	const VecBoolV by_ = IsGreaterThanOrEqual(Ltp.GetCol1(),v_zero);
	const VecBoolV bz_ = IsGreaterThanOrEqual(Ltp.GetCol2(),v_zero);

	const Vec3V neg_boxHalfWidth = -boxHalfWidth;
	const Vec3V cx_ = SelectFT(bx_,neg_boxHalfWidth,boxHalfWidth);
	const Vec3V cy_ = SelectFT(by_,neg_boxHalfWidth,boxHalfWidth);
	const Vec3V cz_ = SelectFT(bz_,neg_boxHalfWidth,boxHalfWidth);

	const Vec3V mind_ = Vec3V(Dot(Ltp.GetCol0(),cx_),Dot(Ltp.GetCol1(),cy_),Dot(Ltp.GetCol2(),cz_));
*/
	// Optimized calculation
	//const Vec3V mind_ = Vec3V(Dot(Abs(Ltp.GetCol0()),boxHalfWidth),Dot(Abs(Ltp.GetCol1()),boxHalfWidth),Dot(Abs(Ltp.GetCol2()),boxHalfWidth));

	// Optimized calculation without transpose.
	const Mat33V identityMat(V_IDENTITY);
	const Vec3V mind_ = geomBoxes::ComputeAABBExtentsFromOBB(identityMat - bToA.GetMat33(),boxHalfWidth);

/*
	// By symmetry the max is the negative of the min.
	const Vec3V maxd_ = -mind_;

	//const Vec3V offset_ = Transform(L,boxCenter);
	const Vec3V offset_ = boxCenter - Transform(bToA,boxCenter);

	//const Vec3V LastAABBMin = AABBMin - mind_ - offset_;
	//const Vec3V LastAABBMax = AABBMax - maxd_ - offset_;
	//const Vec3V AABBMotionMin = AABBMin + Min(ZERO,-mind_-offset_);
	//const Vec3V AABBMotionMax = AABBMax + Max(ZERO,-maxd_-offset_);

	const Vec3V MinPart = Min(v_zero,-mind_-offset_);
	const Vec3V MaxPart = Max(v_zero,-maxd_-offset_);
	expanded_boxHalfWidth = boxHalfWidth + v_half * (MaxPart - MinPart);
	expanded_boxCenter = boxCenter + v_half * (MinPart + MaxPart);
*/
	const Vec3V lastHalfWidth = boxHalfWidth + mind_;
	const Vec3V lastCenter = Transform(bToA,boxCenter);
	const Vec3V AABBMotionMin = Min(lastCenter-lastHalfWidth,boxCenter-boxHalfWidth);
	const Vec3V AABBMotionMax = Max(lastCenter+lastHalfWidth,boxCenter+boxHalfWidth);
	expanded_boxHalfWidth = v_half * (AABBMotionMax - AABBMotionMin);
	expanded_boxCenter = v_half * (AABBMotionMin + AABBMotionMax);
}

__forceinline void COT_ExpandSimpleBoundOBBFromMotion(Mat34V_In current, Vec3V_In translationFromLastToCurrent, Vec3V_InOut boxHalfWidth, Vec3V_InOut boxCenter)
{
	const Vec3V translationFromLastToCurrentLoc = UnTransform3x3Ortho(current,translationFromLastToCurrent);
	const Vec3V halfTranslationFromLastToCurrentLoc = ScalarV(V_HALF) * translationFromLastToCurrentLoc;
	const Vec3V expanded_boxHalfWidth = boxHalfWidth + Abs(halfTranslationFromLastToCurrentLoc);
	const Vec3V expanded_boxCenter = boxCenter - halfTranslationFromLastToCurrentLoc;
	boxHalfWidth = expanded_boxHalfWidth;
	boxCenter = expanded_boxCenter;
}

__forceinline void COT_ExpandSimpleBoundOBBFromMotion(Mat34V_In current, Mat34V_In last, Vec3V_In boundCG, Vec3V_InOut boxHalfWidth, Vec3V_InOut boxCenter)
{
#if USE_CORRECT_EXPANDED_BOUNDING_VOLUME
	const Vec3V translationFromLastToCurrent = ComputeLinearMotion_(last,current,boundCG);
	COT_ExpandSimpleBoundOBBFromMotion(current,translationFromLastToCurrent,boxHalfWidth,boxCenter);
#else // USE_CORRECT_EXPANDED_BOUNDING_VOLUME
	(void)boundCG;
	geomBoxes::ExpandOBBFromMotion(current,last,boxHalfWidth,boxCenter);
#endif // USE_CORRECT_EXPANDED_BOUNDING_VOLUME
}

__forceinline int COT_IsNonSimpleBoundType(const int boundType)
{
	FastAssert((boundType >= 0) && (boundType < phBound::NUM_BOUND_TYPES) && (phBound::NUM_BOUND_TYPES <= 32));
	const int NON_SIMPLE_BOUND_TYPES = (1 << phBound::COMPOSITE) | (1 << phBound::BVH);
	return ((1 << boundType) & NON_SIMPLE_BOUND_TYPES);
}

__forceinline void COT_ExpandOBBFromMotion(Mat34V_In current, Mat34V_In last, Vec3V_InOut boxHalfWidth, Vec3V_InOut boxCenter)
{
#if USE_CORRECT_EXPANDED_BOUNDING_VOLUME
	COT_ExpandNonSimpleBoundOBBFromMotion(current,last,boxHalfWidth,boxCenter,boxHalfWidth,boxCenter);
#else // USE_CORRECT_EXPANDED_BOUNDING_VOLUME
	geomBoxes::ExpandOBBFromMotion(current,last,boxHalfWidth,boxCenter);
#endif // USE_CORRECT_EXPANDED_BOUNDING_VOLUME
}

__forceinline void COT_ExpandOBBFromMotion(Mat34V_In current, Mat34V_In last0, Mat34V_In last1, Vec3V_InOut boxHalfWidth, Vec3V_InOut boxCenter)
{
#if USE_CORRECT_EXPANDED_BOUNDING_VOLUME
	Vec3V boxCenter0, boxHalfWidth0;
	Vec3V boxCenter1, boxHalfWidth1;
	COT_ExpandNonSimpleBoundOBBFromMotion(current,last0,boxHalfWidth,boxCenter,boxHalfWidth0,boxCenter0);
	COT_ExpandNonSimpleBoundOBBFromMotion(current,last1,boxHalfWidth,boxCenter,boxHalfWidth1,boxCenter1);
	const Vec3V AABBMin = Min(boxCenter0-boxHalfWidth0,boxCenter1-boxHalfWidth1);
	const Vec3V AABBMax = Max(boxCenter0+boxHalfWidth0,boxCenter1+boxHalfWidth1);
#else // USE_CORRECT_EXPANDED_BOUNDING_VOLUME
	Mat34V bToA0;
	Mat34V bToA1;

	UnTransformOrtho( bToA0, current, last0 );
	UnTransformOrtho( bToA1, current, last1 );

	const Vec3V boxCenter0 = Transform(bToA0,boxHalfWidth);
	const Vec3V boxHalfWidth0 = geomBoxes::ComputeAABBExtentsFromOBB(bToA0.GetMat33ConstRef(),boxHalfWidth);

	const Vec3V boxCenter1 = Transform(bToA1,boxHalfWidth);
	const Vec3V boxHalfWidth1 = geomBoxes::ComputeAABBExtentsFromOBB(bToA1.GetMat33ConstRef(),boxHalfWidth);

	const Vec3V AABBMin = Min(boxCenter-boxHalfWidth,Min(boxCenter0-boxHalfWidth0,boxCenter1-boxHalfWidth1));
	const Vec3V AABBMax = Max(boxCenter+boxHalfWidth,Max(boxCenter0+boxHalfWidth0,boxCenter1+boxHalfWidth1));
#endif // USE_CORRECT_EXPANDED_BOUNDING_VOLUME

	const ScalarV v_half(V_HALF);
	boxCenter = v_half * (AABBMin + AABBMax);
	boxHalfWidth = v_half * (AABBMax - AABBMin);
}

__forceinline void COT_ExpandSimpleBoundOBBFromMotion(Mat34V_In current, Mat34V_In last, const phBound * bound, Vec3V_InOut boxHalfWidth, Vec3V_InOut boxCenter)
{
	FastAssert(COT_IsNonSimpleBoundType(bound->GetType()) == 0);
#if USE_CORRECT_EXPANDED_BOUNDING_VOLUME
	COT_ExpandSimpleBoundOBBFromMotion(current,last,bound->GetCGOffset(),boxHalfWidth,boxCenter);
#else // USE_CORRECT_EXPANDED_BOUNDING_VOLUME
	(void)bound;
	COT_ExpandOBBFromMotion(current,last,boxHalfWidth,boxCenter);
#endif // USE_CORRECT_EXPANDED_BOUNDING_VOLUME
}

__forceinline void COT_ExpandBoundOBBFromMotion(Mat34V_In current, Mat34V_In last, const phBound * bound, Vec3V_InOut boxHalfWidth, Vec3V_InOut boxCenter)
{
#define USE_CORRECT_EXPANDED_BOUNDING_VOLUME_WITH_BRANCH 0

#if USE_CORRECT_EXPANDED_BOUNDING_VOLUME_WITH_BRANCH

	if (COT_IsNonSimpleBoundType(bound->GetType()))
		COT_ExpandNonSimpleBoundOBBFromMotion(current,last,boxHalfWidth,boxCenter,boxHalfWidth,boxCenter);
	else
		COT_ExpandSimpleBoundOBBFromMotion(current,last,bound->GetCGOffset(),boxHalfWidth,boxCenter);

#else // USE_CORRECT_EXPANDED_BOUNDING_VOLUME_WITH_BRANCH

#define TEST_CORRECT_EBV 0
#if TEST_CORRECT_EBV
	// Compute the simple expanded bounding volume.
	Vec3V boxHalfWidth_ = boxHalfWidth;
	Vec3V boxCenter_ = boxCenter;
	COT_ExpandSimpleBoundOBBFromMotion(current,last,bound->GetCGOffset(),boxHalfWidth_,boxCenter_);

	// Verify that the CG is inside the unexpanded bounding volume.
	const float eps = 0.001f;
	const Vec3V v_eps(eps,eps,eps);
	Assert(IsGreaterThanOrEqualAll(bound->GetCGOffset()+v_eps,boxCenter-boxHalfWidth));
	Assert(IsLessThanOrEqualAll(bound->GetCGOffset()-v_eps,boxCenter+boxHalfWidth));
#endif // TEST_CORRECT_EBV

	(void)bound;
	// Use this for now to avoid branching. Might be able to rework code higher up to call the appropriate function.
	COT_ExpandNonSimpleBoundOBBFromMotion(current,last,boxHalfWidth,boxCenter,boxHalfWidth,boxCenter);

#if TEST_CORRECT_EBV
	// Verify that the simple bounding volume is within the non simple bounding volume.
	Assert(IsGreaterThanOrEqualAll(boxCenter_-boxHalfWidth_+v_eps,boxCenter-boxHalfWidth));
	Assert(IsLessThanOrEqualAll(boxCenter_+boxHalfWidth_-v_eps,boxCenter+boxHalfWidth));
#endif // TEST_CORRECT_EBV

#endif // USE_CORRECT_EXPANDED_BOUNDING_VOLUME_WITH_BRANCH
}

/*
__forceinline void COT_ExpandOBBFromMotion(QuatV_In currentR, Vec3V_In currentP, QuatV_In lastR, Vec3V_In lastP, Vec3V_InOut boxHalfWidth, Vec3V_InOut boxCenter)
{
	geomBoxes::ExpandOBBFromMotion(currentR, currentP, lastR, lastP, boxHalfWidth, boxCenter);
}
*/

} // namespace rage

#endif // PHYSICS_COLLISIONOVERLAPTEST_H