/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "vector\vector3_consts_spu.cpp"

#include "ContinuousConvexCollision.h"
#include "phbound/bound.h"
#include "GjkPairDetector.h"
#include "phsolver/forcesolverconfig.h"
#include "PointCollector.h"
#include "SimdTransformUtil.h"
#include "SimplexSolverInterface.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"
#include "profile/element.h"
#include "physics/manifold.h"
#include "physics/collision.h"
#include "physics/physicsprofilecapture.h"
#include "phbound/bounddisc.h"
#include "TriangleShape.h"

GJK_COLLISION_OPTIMIZE_OFF()

#if !__SPU
#include "profile/profiler.h"
#endif

#include "math/simplemath.h"


using namespace rage;

namespace rage
{
namespace phCollisionStats
{
	EXT_PF_TIMER(NP_CCD_Time);
	EXT_PF_COUNTER(NP_CCD_Calls);
	EXT_PF_COUNTER(NP_CCD_GCP_Calls);
}
using namespace phCollisionStats;
}

#if !__SPU
namespace rage
{
namespace phCCDStats
{
	EXT_PF_VALUE_INT(TotalCCD_Calls);
	EXT_PF_VALUE_INT(TotalCCD_Iters);
	EXT_PF_VALUE_FLOAT(AvgCCD_Iters);
	EXT_PF_VALUE_INT(PeakCCD_Iters);
	EXT_PF_VALUE_INT(MinCCD_Iters);
}
}
using namespace phCCDStats;
#if __STATS
#define SAVECCD_PFSTATS {PF_VALUE_VAR(TotalCCD_Iters).Set(PF_VALUE_VAR(TotalCCD_Iters).GetValue() + numIter); \
PF_VALUE_VAR(AvgCCD_Iters).Set(float(PF_VALUE_VAR(TotalCCD_Iters).GetValue()) / PF_VALUE_VAR(TotalCCD_Calls).GetValue()); \
if(PF_VALUE_VAR(PeakCCD_Iters).GetValue() < numIter) {PF_VALUE_VAR(PeakCCD_Iters).Set(numIter);}; \
if(PF_VALUE_VAR(MinCCD_Iters).GetValue() > numIter) {PF_VALUE_VAR(MinCCD_Iters).Set(numIter);};}
#else
#define SAVECCD_PFSTATS
#endif
#else // SPU
#define SAVECCD_PFSTATS
#endif

ContinuousConvexCollision::ContinuousConvexCollision (phConvexIntersector* intersector, rage::Vec::V3Param128 closeEnough)
: m_intersector(intersector)
, m_nearEnoughDistance(closeEnough)
{
}

/// This maximum should not be necessary. It allows for untested/degenerate cases in production code.
/// You don't want your game ever to lock-up.

#define SPEWCCD_ITERATION_OVERFLOWS __ASSERT
#if SPEWCCD_ITERATION_OVERFLOWS
#define SPEWCCD_TEXT(x) if(exceededMaxIter){ static int counter = 0; counter++; if (counter < 100) x;}
#else
#define SPEWCCD_TEXT(x)
#endif

namespace rage {
	int g_MaxContinuousIterations = 32;
	bool g_ExtraCCDCollision = false;
}

// TODO: MOVE THESE CONTACT CONSTANTS INTO A SEPARATE HEADER.
#define HALF_POS_DEPTH_ONLY_THRESH (POSITIVE_DEPTH_ONLY_THRESHOLD/2.0f)
static const Vec3V CCD_DISTANCE_TOLERANCE(-HALF_POS_DEPTH_ONLY_THRESH, -HALF_POS_DEPTH_ONLY_THRESH, -HALF_POS_DEPTH_ONLY_THRESH);
static const ScalarV CCD_DISTANCE_TOLERANCE_SV(-HALF_POS_DEPTH_ONLY_THRESH);

#define USE_CCD_FINAL_STEP USE_ACTIVE_COLLISION_DISTANCE
#if USE_CCD_FINAL_STEP
const float CCD_FINAL_STEP_REL_TRANS_THRESH = .05f;
const ScalarV CCD_FINAL_STEP_REL_TRANS_THRESH_SQ_V(ScalarVFromF32(CCD_FINAL_STEP_REL_TRANS_THRESH*CCD_FINAL_STEP_REL_TRANS_THRESH));
const ScalarV CCD_FINAL_STEP_DIST_THRESH_V(ScalarVFromF32(ACTIVE_COLLISION_DISTANCE));
#endif // USE_CCD_FINAL_STEP

__forceinline void ContinuousConvexCollision::ComputeLinearMotion(Mat34V_In fromA, Mat34V_In toA, Mat34V_In fromB, Mat34V_In toB, Vec3V_InOut linVelA, Vec3V_InOut linVelB) const
{
	// Here we try to determine the translational motion of an object from two matrices.  The reason that this isn't trivial is that (except for
	//   simple cases like no rotation) there are an infinite number of combinations of angular and linear motion that could have resulted in the
	//   given change of orientation/position (remember that rotation about a given axis in world space is equivalent rotation about a parallel axis
	//   combined with a translation perpendicular to that axis).  In order to disambiguate, we need to pick the point that we want to consider to
	//   have moved linearly during the time step.
#if CCD_ROTATION_CENTER == 0
	// This assumes that the axis of rotation passes through the local origin of the bound (aka the instance position).
	linVelA = Subtract(toA.GetCol3(), fromA.GetCol3());
	linVelB = Subtract(toB.GetCol3(), fromB.GetCol3());
#elif CCD_ROTATION_CENTER == 1
	// This assumes that the axis of rotation passes through the center of mass.  This is generally the case with objects whose motion is
	//   unconstrained.  Most of the time, an object whose motion is constrained will also have already-existing contacts, and we currently don't
	//   trigger CCD in those cases.
	linVelA = Subtract(m_intersector->GetBoundA()->GetCenterOfMass(toA), m_intersector->GetBoundA()->GetCenterOfMass(fromA));
	linVelB = Subtract(m_intersector->GetBoundB()->GetCenterOfMass(toB), m_intersector->GetBoundB()->GetCenterOfMass(fromB));
#elif CCD_ROTATION_CENTER == 2
	// This assumes that the axis of rotation passes through the center of the bounding sphere of the bound.
	linVelA = Subtract(m_intersector->GetBoundA()->GetCenter(RCC_MATRIX34(toA)), m_intersector->GetBoundA()->GetCenter(RCC_MATRIX34(fromA)));
	linVelB = Subtract(m_intersector->GetBoundB()->GetCenter(RCC_MATRIX34(toB)), m_intersector->GetBoundB()->GetCenter(RCC_MATRIX34(fromB)));
#endif
}

#if !USE_NEWER_COLLISION_OBJECT
__forceinline Vec3V_Out ComputeLinearMotion_(const NewCollisionObject & collisionObject)
{
	return ComputeLinearMotion_(collisionObject.m_last,collisionObject.m_current,collisionObject.m_bound);
}
#endif

__forceinline void ContinuousConvexCollision::ComputeInterpolatedMotion(Mat34V_In fromA, Mat34V_In toA, Mat34V_In fromB, Mat34V_In toB, Vec3V_InOut linVelA
																   , Vec3V_InOut linVelB, Vec3V_InOut angVelA, Vec3V_InOut angVelB, QuatV_InOut quat0A
																   , QuatV_InOut quat0B, QuatV_InOut quatDeltaA, QuatV_InOut quatDeltaB) const
{
	ComputeLinearMotion(fromA, toA, fromB, toB, linVelA, linVelB);

	angVelA = SimdTransformUtil::CalculateAngularVelocity(MAT33V_ARG(fromA.GetMat33()), MAT33V_ARG(toA.GetMat33()));
	SimdTransformUtil::CalculateQuaternionDelta(MAT33V_ARG(fromA.GetMat33()), MAT33V_ARG(toA.GetMat33()), quat0A.GetIntrin128Ref(), quatDeltaA.GetIntrin128Ref());
	angVelB = SimdTransformUtil::CalculateAngularVelocity(MAT33V_ARG(fromB.GetMat33()), MAT33V_ARG(toB.GetMat33()));
	SimdTransformUtil::CalculateQuaternionDelta(MAT33V_ARG(fromB.GetMat33()), MAT33V_ARG(toB.GetMat33()), quat0B.GetIntrin128Ref(), quatDeltaB.GetIntrin128Ref());
}

#define USE_MOVED_UP_WHEEL_COLLISION (0 && USE_NEW_TRIANGLE_BACK_FACE_CULL)	// Use this to emulate the triangle penetration solver and velocity back-face culling for wheel collision.

#if !DISABLE_SPECIAL_CASE_COLLISION
void ContinuousConvexCollision::ComputeTimeOfImpact (const rage::NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult& pointCollector)
{
	FastAssert(IsGreaterThanOrEqualAll(collisionInput.sepThresh,CCD_DISTANCE_TOLERANCE_SV) != 0);

#if !__SPU
#if __STATS
	PF_VALUE_VAR(TotalCCD_Calls).Set(PF_VALUE_VAR(TotalCCD_Calls).GetValue() + 1);
#endif
#endif

	PF_FUNC(NP_CCD_Time);
	PF_INCREMENT(NP_CCD_Calls);

	const NewCollisionObject * object0 = collisionInput.object0;
	const NewCollisionObject * object1 = collisionInput.object1;

	/// Compute linear translation for this interval, to interpolate.
#if USE_NEWER_COLLISION_OBJECT
	Vec3V linVelA = object0->m_trans;
	const Vec3V linVelB = object1->m_trans;
#else
	 Vec3V linVelA = ComputeLinearMotion_(*object0);
	const Vec3V linVelB = ComputeLinearMotion_(*object1);
#endif

#if USE_MOVED_UP_WHEEL_COLLISION
	static bool enableMoveUp = true;
	// TODO: Implement better logic to determine if we are really colliding with a wheel.
	const bool isWheelCollision = (object0->m_bound->GetType() == phBound::DISC && object1->m_bound->GetType() == phBound::TRIANGLE) && enableMoveUp;
	if (isWheelCollision)
	{
		// Move up the wheel starting position.
		// Any triangle that the wheel touches at the new starting position that it wasn't touching at its original starting
		// position will have its contacts thrown out by either the new back-face culling or the moving-away-from culling at 
		// the end of this function. Any triangle that the wheel is initially touching and moving towards will always return a
		// contact point. The result of all this should be to emulate the triangle penetration solver with velocity back-face culling.
		const phBoundDisc * boundDisc = reinterpret_cast<const phBoundDisc*>(object0->m_bound);
		const ScalarV radius = boundDisc->GetRadiusV();
		static int moveUpType = 1;
		if (moveUpType == 0)
		{
			// Move up the starting position along the sweep direction by an amount equal to the radius.
			const ScalarV nlinVelA =  Mag(linVelA);
			const Vec3V linVelA_movedUp = (ScalarV(V_ONE) + radius / nlinVelA) * linVelA;
			const BoolV divOk = IsGreaterThan(nlinVelA,ScalarV(V_FLT_SMALL_5));
			linVelA = SelectFT(divOk,linVelA,linVelA_movedUp);
		}
		else
		{
			// Move up the starting position along the triangle normal direction by an amount necessary to ensure
			// the disc is on the from side of the triangle.
			const TriangleShape * boundTriangle = reinterpret_cast<const TriangleShape*>(object1->m_bound);
			const Vec3V Apos = object0->m_current.GetCol3() - linVelA;
			const Vec3V Bpos = object1->m_current.GetCol3() - linVelB;
			const ScalarV dist = Dot(Apos-Bpos,boundTriangle->m_PolygonNormal);
			const ScalarV fudge = ScalarVFromF32(.1f) * radius;
			const ScalarV offset = Min(Max(fudge-dist,ScalarV(V_ZERO)),radius);
			const Vec3V linVelA_movedUp = linVelA - offset * boundTriangle->m_PolygonNormal;
			linVelA = linVelA_movedUp;
		}
	}
#endif // USE_MOVED_UP_WHEEL_COLLISION

	const Vec3V relVel = linVelB-linVelA;

#if USE_RELOCATE_MATRICES
	const Vec3V relocationOffset = object1->m_current.GetCol3();
	const Vec3V Apos = object0->m_current.GetCol3() - relocationOffset;
	const Vec3V Bpos = Vec3V(V_ZERO);
#else
	const Vec3V Apos = toA.GetCol3();
	const Vec3V Bpos = toB.GetCol3();
#endif

	const phConvexIntersector::PenetrationDepthSolverType pdsType = (object1->m_bound->GetType() == phBound::TRIANGLE) ? phConvexIntersector::PDSOLVERTYPE_TRIANGLE : phConvexIntersector::PDSOLVERTYPE_MINKOWSKI;
	GjkPairDetector::ClosestPointInput input(collisionInput.sepThresh,object0->m_bound,object1->m_bound,pdsType,NULL);
	input.m_transformA.Set3x3(object0->m_current);
	input.m_transformB.Set3x3(object1->m_current);
	input.m_transformA.SetCol3(Subtract(Apos, linVelA));
	input.m_transformB.SetCol3(Subtract(Bpos, linVelB));

	{
		// Perform an initial GJK step.
		phConvexIntersector::GetClosestPointsSpecialCase(input, pointCollector);
		PF_INCREMENT(NP_CCD_GCP_Calls);
		PPC_STAT_COUNTER_INC(CCDIterationCounter,1);
	}

	Vec3V lambda = Vec3V(V_ZERO);
	Vec3V lastLambda = lambda;

	const int maxIter = g_MaxContinuousIterations;
	int numIter = 0;

#if SPEWCCD_ITERATION_OVERFLOWS
	static int arbitrarilyLargeMaxIter = 1000;
	bool exceededMaxIter = false;
#endif

	Vec3V dist = pointCollector.GetDistanceV();

	// As long as we're able to find closest points but they aren't in our tolerance, we keep iterating
	while( (GenerateMaskNZ(pointCollector.GetHasResult()) & IsGreaterThanAll( dist, CCD_DISTANCE_TOLERANCE )) != 0 )
	{
		numIter++;
		if (Unlikely(numIter > maxIter))
		{
#if SPEWCCD_ITERATION_OVERFLOWS
			exceededMaxIter = true;
		}
		if (Unlikely(numIter > arbitrarilyLargeMaxIter))
		{
			SPEWCCD_TEXT(Displayf("Ultra Max iterations exceeded in ContinuousConvexCollision::ComputeTimeOfImpact. THINGS MAY FALL THROUGH EACH OTHER!!!"));
#endif
			SAVECCD_PFSTATS;
			pointCollector.SetHasResult(false);	// TODO: evaluate if this should be set to true.
			return; //todo: report a failure
		}

		// Find the displacement of the objects 'into' (or toward) each other.
		const Vec3V normal = pointCollector.GetNormalOnBInWorld();
		const ScalarV projectedLinearVelocity = Dot(relVel, normal);
		// dLambda will be the ratio between the current separation distance and the total displacement for this frame (along the axis separating
		//   the two objects).
		const Vec3V dLambda = Vec3V( Scale(dist, InvertSafe(projectedLinearVelocity, ScalarV(V_FLT_MAX))) ); // FLTMAX here just b/c old lib did it. I don't want any side effects right now. /wpfeil
		lambda += dLambda;

		// Due to the fact that normal will change direction if the two objects were to be found penetrating, dLambda being negative either means
		//   that the objects are separating (probably the intention of the below condition) *or* the objects are now inter-penetrating each other
		//   (in which case we probably don't want to bail out saying that there's no collision).
		// In my tests, however, I did not find it to be happening that the normal was changing direction, so we're probably safe there.
#if USE_CCD_FINAL_STEP

		const unsigned int dLNegative = IsLessThanAll(dLambda, Vec3V(V_ZERO));
		const unsigned int LGtrOne = IsGreaterThanAll(lambda, Vec3V(V_ONE));
		if ( (dLNegative | LGtrOne) != 0 )
		{
			SPEWCCD_TEXT(Displayf("CCD MaxIter exceeded with: %d  --  Failed 3", numIter));
			SAVECCD_PFSTATS;
			// If the relative translation is small enough, check the final distance to see if we want to return contact points.
			const ScalarV nrelTransSq = MagSquared(relVel);	
			if (IsLessThanOrEqualAll(nrelTransSq,CCD_FINAL_STEP_REL_TRANS_THRESH_SQ_V))
			{
				ScalarV deltaLamdba;
				if (dLNegative)
					deltaLamdba = ScalarV(V_ZERO);
				else
					deltaLamdba = ScalarV(V_ONE) - lastLambda.GetX();
				// Estimate the final distance.
				const ScalarV finalDist = dist.GetX() - deltaLamdba * projectedLinearVelocity;
				if (IsLessThanOrEqualAll(finalDist,CCD_FINAL_STEP_DIST_THRESH_V))
				{
					// We could call the collision once more but instead we'll just use the last results instead. 
					pointCollector.SetDistanceV(Vec3V(finalDist));
					break;
				}
			}
			pointCollector.SetHasResult(false);
			return;
		}

#else // USE_CCD_FINAL_STEP
		if ( (IsLessThanAll(dLambda, Vec3V(V_ZERO)) | IsGreaterThanAll(lambda, Vec3V(V_ONE))) != 0 )
		{
			if (Unlikely(g_ExtraCCDCollision))
			{
				lambda = Vec3V(V_ONE);
			}
			else
			{
				SPEWCCD_TEXT(Displayf("CCD MaxIter exceeded with: %d  --  Failed 3", numIter));
				SAVECCD_PFSTATS;
				pointCollector.SetHasResult(false);
				return;
			}
		}
#endif // USE_CCD_FINAL_STEP
		//todo: next check with relative epsilon
		if( IsLessThanOrEqualAll(lambda, lastLambda) != 0 )// || lambda.IsGreaterThan(VEC3_IDENTITY))
		{
			break;
		}

		lastLambda = lambda;

		//interpolate to next lambda
		const Vec3V oneMinusLambda = Vec3V(V_ONE) - lambda;
		input.m_transformA.SetCol3(SubtractScaled(Apos, linVelA, oneMinusLambda));
		input.m_transformB.SetCol3(SubtractScaled(Bpos, linVelB, oneMinusLambda));

		pointCollector.SetDistanceV(Vec3V(V_FLT_MAX));
		pointCollector.SetHasResult(false);

		phConvexIntersector::GetClosestPointsSpecialCase(input, pointCollector);
		dist = pointCollector.GetDistanceV();
		PF_INCREMENT(NP_CCD_GCP_Calls);
		PPC_STAT_COUNTER_INC(CCDIterationCounter,1);
	}

#if TRACK_COLLISION_TIME
	// Can point collector's time be a scalar?
	pointCollector.SetTime(lambda.GetX());
#endif

	if (Likely(pointCollector.GetHasResult()))
	{
#if USE_MOVED_UP_WHEEL_COLLISION
		if (isWheelCollision)
		{
			// Discard contact points that the wheel is moving away from.
			const ScalarV dotP = Dot(linVelA,pointCollector.GetNormalOnBInWorld());
			if (IsGreaterThanOrEqualAll(dotP,ScalarV(V_ZERO)))
				pointCollector.SetHasResult(false);
		}
#endif // USE_MOVED_UP_WHEEL_COLLISION
		if (IsGreaterThanAll(collisionInput.sepThresh,ScalarV(pointCollector.GetDistanceV().GetIntrin128())) == 0)
			pointCollector.SetHasResult(false);
	}

	SPEWCCD_TEXT(Displayf("CCD MaxIter exceeded with: %d  --  Returniung result 1", numIter));
	SAVECCD_PFSTATS;
}
#endif // !DISABLE_SPECIAL_CASE_COLLISION

void ContinuousConvexCollision::ComputeTimeOfImpactGJK1 (const rage::NewCollisionInput * CCD_RESTRICT collisionInput, DiscreteCollisionDetectorInterface::SimpleResult * CCD_RESTRICT pointCollector, GjkPairDetector * CCD_RESTRICT gjk)
{
	FastAssert(IsGreaterThanOrEqualAll(collisionInput->sepThresh,CCD_DISTANCE_TOLERANCE_SV) != 0);
	
#if !__SPU
#if __STATS
	PF_VALUE_VAR(TotalCCD_Calls).Set(PF_VALUE_VAR(TotalCCD_Calls).GetValue() + 1);
#endif
#endif

	const NewCollisionObject * CCD_RESTRICT object0 = collisionInput->object0;
	const NewCollisionObject * CCD_RESTRICT object1 = collisionInput->object1;

	/// Compute linear translation for this interval, to interpolate.
#if USE_NEWER_COLLISION_OBJECT
	const Vec3V linVelA = object0->m_trans;
	const Vec3V linVelB = object1->m_trans;
#else
	const Vec3V linVelA = ComputeLinearMotion_(*object0);
	const Vec3V linVelB = ComputeLinearMotion_(*object1);
#endif
	const Vec3V relVel = linVelB-linVelA;

#if USE_RELOCATE_MATRICES
	const Vec3V relocationOffset = object1->m_current.GetCol3();
	const Vec3V Apos = object0->m_current.GetCol3() - relocationOffset;
	const Vec3V Bpos = Vec3V(V_ZERO);
#else
	const Vec3V Apos = toA.GetCol3();
	const Vec3V Bpos = toB.GetCol3();
#endif

	const phConvexIntersector::PenetrationDepthSolverType pdsType = (object1->m_bound->GetType() == phBound::TRIANGLE) ? phConvexIntersector::PDSOLVERTYPE_TRIANGLE : phConvexIntersector::PDSOLVERTYPE_MINKOWSKI;
	GjkPairDetector::ClosestPointInput input(collisionInput->sepThresh,object0->m_bound,object1->m_bound,pdsType,collisionInput->gjkCacheInfo->m_gjkCache);
	input.m_transformA.Set3x3(object0->m_current);
	input.m_transformB.Set3x3(object1->m_current);
	input.m_transformA.SetCol3(Subtract(Apos, linVelA));
	input.m_transformB.SetCol3(Subtract(Bpos, linVelB));

	DiscreteCollisionDetectorInterface::ClosestPointOutput gjkOutput;

	PF_INCREMENT(NP_CCD_GCP_Calls);
	PPC_STAT_COUNTER_INC(CCDIterationCounter,1);

#define CCD_CHECK_PENETRATION_FIRST_ITERATION_ONLY (0 && USE_NEW_SIMPLEX_SOLVER)

	// Perform an initial GJK step.
	gjk->DoGjkIterations(input,false,true,&gjkOutput);
	u32 isPepenetrating = GenerateMaskNZ(gjkOutput.m_needsPenetrationSolve);
	if (isPepenetrating)
		phConvexIntersector::GjkPenetrationSolve(&input,&gjkOutput,gjk);

	const ScalarV v_zero(V_ZERO);
	const ScalarV v_one(V_ONE);
	const ScalarV v_flt_max(V_FLT_MAX);

	ScalarV lambda = v_zero;
	ScalarV lastLambda = lambda;

	const int maxIter = g_MaxContinuousIterations;
	int numIter = 0;

#if SPEWCCD_ITERATION_OVERFLOWS
	static int arbitrarilyLargeMaxIter = 1000;
	bool exceededMaxIter = false;
#endif

	{

	//Vec3V dist = pointCollector.GetDistanceV();
	ScalarV dist = gjkOutput.separationWithMargin;

	// As long as we're able to find closest points but they aren't in our tolerance, we keep iterating
	//while( (GenerateMaskNZ(pointCollector.GetHasResult()) & IsGreaterThanAll( dist, CCD_DISTANCE_TOLERANCE )) != 0 )
	//while ( ((~isPepenetrating) & GenerateMaskNZ(gjkOutput.isValid) & IsGreaterThanAll( dist, CCD_DISTANCE_TOLERANCE_SV )) != 0)
	while ( (GenerateMaskNZ(gjkOutput.isValid) & IsGreaterThanAll( dist, CCD_DISTANCE_TOLERANCE_SV )) != 0)
	{
		numIter++;
		if (Unlikely(numIter > maxIter))
		{
#if SPEWCCD_ITERATION_OVERFLOWS
			exceededMaxIter = true;
		}
		if (Unlikely(numIter > arbitrarilyLargeMaxIter))
		{
			SPEWCCD_TEXT(Displayf("Ultra Max iterations exceeded in ContinuousConvexCollision::ComputeTimeOfImpact. THINGS MAY FALL THROUGH EACH OTHER!!!"));
#endif
			SAVECCD_PFSTATS;
			//pointCollector->SetHasResult(false);	// TODO: evaluate if this should be set to true.
			FastAssert(pointCollector->GetHasResult() == false);
			return; //todo: report a failure
		}

		// Find the displacement of the objects 'into' (or toward) each other.
		const Vec3V normal = gjkOutput.worldNormalBtoA;//pointCollector.GetNormalOnBInWorld();
		const ScalarV projectedLinearVelocity = Dot(relVel, normal);
		// dLambda will be the ratio between the current separation distance and the total displacement for this frame (along the axis separating
		//   the two objects).
		const ScalarV dLambda = Scale(dist, InvertSafe(projectedLinearVelocity, v_flt_max)); // FLTMAX here just b/c old lib did it. I don't want any side effects right now. /wpfeil
		lambda += dLambda;

#if USE_CCD_FINAL_STEP
		const unsigned int dLNegative = IsLessThanAll(dLambda,v_zero);
		const unsigned int LGtrOne = IsGreaterThanAll(lambda,v_one);
		if ( (dLNegative | LGtrOne) != 0 )
		{
			SPEWCCD_TEXT(Displayf("CCD MaxIter exceeded with: %d  --  Failed 3", numIter));
			SAVECCD_PFSTATS;
			// If the relative translation is small enough, check the final distance to see if we want to return contact points.
			const ScalarV nrelTransSq = MagSquared(relVel);
			if (IsLessThanOrEqualAll(nrelTransSq,CCD_FINAL_STEP_REL_TRANS_THRESH_SQ_V))
			{
				ScalarV deltaLamdba;
				if (dLNegative)
					deltaLamdba = v_zero;
				else
					deltaLamdba = v_one - lastLambda;
				// Estimate the final distance.
				const ScalarV finalDist = dist - deltaLamdba * projectedLinearVelocity;
				if (IsLessThanOrEqualAll(finalDist,CCD_FINAL_STEP_DIST_THRESH_V))
				{
					// Try to avoid calling the collision once more if possible.
					if (gjkOutput.isSeparatedBeyondSepThresh)
					{
						// Run GJK once more to get good contact points.
						input.m_sepThreshold = v_flt_max;
						const ScalarV newLambda = lastLambda + deltaLamdba;
						const ScalarV oneMinusLambda = v_one - newLambda;
						input.m_transformA.SetCol3(Apos - oneMinusLambda * linVelA);
						input.m_transformB.SetCol3(Bpos - oneMinusLambda * linVelB);
#if CCD_CHECK_PENETRATION_FIRST_ITERATION_ONLY
						gjk->DoGjkIterations(input,false,&gjkOutput);
						FastAssert(gjkOutput.m_needsPenetrationSolve == false);
#else // CCD_CHECK_PENETRATION_FIRST_ITERATION_ONLY
						gjk->DoGjkIterations(input,false,false,&gjkOutput);
						isPepenetrating = GenerateMaskNZ(gjkOutput.m_needsPenetrationSolve);
						if (isPepenetrating)
							phConvexIntersector::GjkPenetrationSolve(&input,&gjkOutput,gjk);
#endif // CCD_CHECK_PENETRATION_FIRST_ITERATION_ONLY
						break;
					}
					gjkOutput.separationWithMargin = finalDist;
					break;
				}
			}
			FastAssert(pointCollector->GetHasResult() == false);
			gjk->m_simplexSolver.m_isSeparated = gjkOutput.isSeparatedBeyondSepThresh;
			return;
		}
#else // USE_CCD_FINAL_STEP

#if !RSG_DURANGO

		// Due to the fact that normal will change direction if the two objects were to be found penetrating, dLambda being negative either means
		//   that the objects are separating (probably the intention of the below condition) *or* the objects are now inter-penetrating each other
		//   (in which case we probably don't want to bail out saying that there's no collision).
		// In my tests, however, I did not find it to be happening that the normal was changing direction, so we're probably safe there.
		if ( (IsLessThanAll(dLambda, v_zero) | IsGreaterThanAll(lambda, v_one)) != 0 )
#else // !RSG_DURANGO
// temporary workaround for code-gen bug
// compiler loses track of AVX register containing dLambda
		const unsigned int dLNegative = IsLessThanAll(dLambda, v_zero);
		const unsigned int LGtrOne = IsGreaterThanAll(lambda, v_one);

		if ( (dLNegative | LGtrOne) != 0 )
#endif // !RSG_DURANGO
		{
			if (Unlikely(g_ExtraCCDCollision))
			{
				lambda = v_one;
			}
			else
			{
				SPEWCCD_TEXT(Displayf("CCD MaxIter exceeded with: %d  --  Failed 3", numIter));
				SAVECCD_PFSTATS;
				// TODO: Exiting here doesn't take into account the possibility that the distance at time=1 is less than the separating threshold.
				FastAssert(pointCollector->GetHasResult() == false);
				gjk->m_simplexSolver.m_isSeparated = gjkOutput.isSeparatedBeyondSepThresh;
				return;
			}
		}
#endif // USE_CCD_FINAL_STEP

		//todo: next check with relative epsilon
		if( IsLessThanOrEqualAll(lambda, lastLambda) != 0 )// || lambda.IsGreaterThan(VEC3_IDENTITY))
		{
			FastAssert(gjkOutput.isSeparatedBeyondSepThresh == false);
			break;
		}

		lastLambda = lambda;

		//interpolate to next lambda
		const ScalarV oneMinusLambda = v_one - lambda;
		input.m_transformA.SetCol3(Apos - oneMinusLambda * linVelA);
		input.m_transformB.SetCol3(Bpos - oneMinusLambda * linVelB);

		//m_intersector->GetClosestPoints(input, pointCollector);
		//dist = pointCollector.GetDistanceV();

#if CCD_CHECK_PENETRATION_FIRST_ITERATION_ONLY
		const Vec3V normalBToA = gjkOutput.worldNormalBtoA;
		gjk->DoGjkIterations(input,false,&gjkOutput);
		//FastAssert(gjkOutput.m_needsPenetrationSolve == false);
		if (gjkOutput.m_needsPenetrationSolve)
		{
			const Vec3V seperatingAxisInA = UnTransform3x3Ortho( input.m_transformA, -normalBToA );
			const Vec3V seperatingAxisInB = UnTransform3x3Ortho( input.m_transformB, normalBToA );

			const Vec3V pInA = input.m_boundA->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInA.GetIntrin128());
			const Vec3V qInB = input.m_boundB->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInB.GetIntrin128());
			const Vec3V pWorld = Transform3x3(input.m_transformA, pInA);
			const Vec3V qWorld = Transform3x3(input.m_transformB, qInB);

			//const Vec3V separationOffset = Apos - Bpos + relVel;

			//const Vec3V w = (qWorld - pWorld) - separationOffset;

			// ((p0_ + t * v0_ - m1_ * u) - (p1_ + t_ * v1_)) * u = 0;
			// t_ = -(p0_ - p1_) * u / (v0_ - v1_) * u
			// p0_ = p00_ - v0_
			// p1_ = p11_ - v1_
			// p0_ - p1_ = p00_ - p11_ + (v1_ - v0_)
			const Vec3V w = (Apos + pWorld - linVelA) - (Bpos + qWorld - linVelB);
			const Vec3V v = linVelA - linVelB;
			//Dot(-w - lambda * relVel,normalBToA) = 0
			const ScalarV numer = Dot(w,normalBToA) - input.m_boundA->GetMarginV() - input.m_boundB->GetMarginV();
			const ScalarV denom = Dot(v,normalBToA);
			const ScalarV lambda_ = -numer / denom;
			lambda = Clamp(lambda_,v_zero,v_one);
			const ScalarV oneMinusLambda_ = v_one - lambda;
			input.m_transformA.SetCol3(Apos - oneMinusLambda_ * linVelA);
			input.m_transformB.SetCol3(Bpos - oneMinusLambda_ * linVelB);
			gjk->DoGjkIterations(input,false,&gjkOutput);
			int dummy = 1;
			(void)dummy;
		}
#else // CCD_CHECK_PENETRATION_FIRST_ITERATION_ONLY
		gjk->DoGjkIterations(input,false,false,&gjkOutput);
		isPepenetrating = GenerateMaskNZ(gjkOutput.m_needsPenetrationSolve);
		if (isPepenetrating)
			phConvexIntersector::GjkPenetrationSolve(&input,&gjkOutput,gjk);
#endif // CCD_CHECK_PENETRATION_FIRST_ITERATION_ONLY
		dist = gjkOutput.separationWithMargin;

		PF_INCREMENT(NP_CCD_GCP_Calls);
		PPC_STAT_COUNTER_INC(CCDIterationCounter,1);
	}
	} // if (!penetrating)
#if TRACK_COLLISION_TIME
	// Can point collector's time be a scalar?
	pointCollector.SetTime(lambda.GetX());
#endif

	FastAssert(pointCollector->GetHasResult() == false);
	if (Likely(gjkOutput.isValid))
	{
		FastAssert(gjkOutput.isSeparatedBeyondSepThresh == false);
#if 1//!USE_GJK_EARLY_EXIT
		// In principle, with early exiting, we shouldn't get here if the objects are separated. But because of numerical roundoff,
		// the lower-bound-distance that GJK computes will underestimate the actual distance. This discrepancy should be rare and
		// shouldn't create any problems. Code higher up will recheck the distance and discard the point if needed. The only potential
		// problem is that the code might allocate a manifold that is not needed if it discards the point. This is only a problem because
		// it temporarily wastes memory and cpu cycles. There are asserts to check for this that currently fire off. Doing this check 
		// here should fix these asserts.
		if (IsGreaterThanAll(collisionInput->sepThresh,gjkOutput.separationWithMargin) != 0)
#endif
		{
			phConvexIntersector::WriteSimpleResult(&input,&gjkOutput,gjk,pointCollector);
		}
	}
	SPEWCCD_TEXT(Displayf("CCD MaxIter exceeded with: %d  --  Returniung result 1", numIter));
	SAVECCD_PFSTATS;
}

void ContinuousConvexCollision::ComputeTimeOfImpactGJK (const rage::NewCollisionInput * CCD_RESTRICT collisionInput, DiscreteCollisionDetectorInterface::SimpleResult * CCD_RESTRICT pointCollector)
{
	PF_FUNC(NP_CCD_Time);
	PF_INCREMENT(NP_CCD_Calls);
	PPC_STAT_COUNTER_INC(CCDCounter,1);

	FastAssert(pointCollector->GetHasResult() == false);

	const NewCollisionObject * CCD_RESTRICT object0 = collisionInput->object0;
	const NewCollisionObject * CCD_RESTRICT object1 = collisionInput->object1;

	GjkPairDetector gjk;

#if USE_GJK_CACHE
	UpdateGJKCacheProlog(&gjk.m_simplexSolver,collisionInput->gjkCacheInfo,object0->m_bound,object1->m_bound);
#endif

	ContinuousConvexCollision::ComputeTimeOfImpactGJK1(collisionInput, pointCollector, &gjk);

#if USE_GJK_CACHE
	UpdateGJKCacheEpilog(&gjk.m_simplexSolver,collisionInput->gjkCacheInfo);
#endif
}
