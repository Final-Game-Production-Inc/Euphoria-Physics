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


#ifndef CONTINUOUS_COLLISION_CONVEX_CAST_H
#define CONTINUOUS_COLLISION_CONVEX_CAST_H

#include "ConvexCast.h"
#include "ConvexIntersector.h"

#include "vectormath/vec3v.h"
#include "vectormath/mat34v.h"

namespace rage
{
	class VoronoiSimplexSolver;
	struct NewCollisionInput;
	class GjkPairDetector;
}

// This determines how we decompose object motion into linear and angular motion displacements.  (For more details, see the comments at the top of
//   ContinuousConvexCollision::ComputeTimeOfImpact().)
// 0 = rotation occurs through instance position
// 1 = rotation occurs through center of mass
// 2 = rotation occurs through center of bounding sphere
#define CCD_ROTATION_CENTER	1

namespace rage
{
__forceinline Vec3V_Out ComputeLinearMotion_(Mat34V_In from, Mat34V_In to, const phBound * bound)
{
#if CCD_ROTATION_CENTER == 0
	(void)bound;
	// This assumes that the axis of rotation passes through the local origin of the bound (aka the instance position).
	return Subtract(to.GetCol3(), from.GetCol3());
#elif CCD_ROTATION_CENTER == 1
	// This assumes that the axis of rotation passes through the center of mass.  This is generally the case with objects whose motion is
	//   unconstrained.  Most of the time, an object whose motion is constrained will also have already-existing contacts, and we currently don't
	//   trigger CCD in those cases.
	//return Subtract(bound->GetCenterOfMass(to), bound->GetCenterOfMass(from));
	return Transform(to-from,bound->GetCGOffset());	// This should be faster and have less roundoff error when the matrix positions are large.
#elif CCD_ROTATION_CENTER == 2
	// This assumes that the axis of rotation passes through the center of the bounding sphere of the bound.
	return Subtract(bound->GetCenter(RCC_MATRIX34(to)), bound->GetCenter(RCC_MATRIX34(from)));
#endif
}

__forceinline Vec3V_Out ComputeLinearMotion_(Mat34V_In from, Mat34V_In to, Vec3V_In cg)
{
	FastAssert(CCD_ROTATION_CENTER == 1);
#if CCD_ROTATION_CENTER == 0
	(void)bound;
	// This assumes that the axis of rotation passes through the local origin of the bound (aka the instance position).
	return Subtract(to.GetCol3(), from.GetCol3());
#elif CCD_ROTATION_CENTER == 1
	// This assumes that the axis of rotation passes through the center of mass.  This is generally the case with objects whose motion is
	//   unconstrained.  Most of the time, an object whose motion is constrained will also have already-existing contacts, and we currently don't
	//   trigger CCD in those cases.
	//return Subtract(bound->GetCenterOfMass(to), bound->GetCenterOfMass(from));
	return Transform(to-from,cg);	// This should be faster and have less roundoff error when the matrix positions are large.
#elif CCD_ROTATION_CENTER == 2
	// This assumes that the axis of rotation passes through the center of the bounding sphere of the bound.
	return Subtract(bound->GetCenter(RCC_MATRIX34(to)), bound->GetCenter(RCC_MATRIX34(from)));
#endif
}

}; // namespace rage

/// ContinuousConvexCollision implements angular and linear time of impact for convex objects.
/// Based on Brian Mirtich's Conservative Advancement idea (PhD thesis).
/// Algorithm operates in worldspace, in order to keep inbetween motion globally consistent.
/// It uses GJK at the moment. Future improvement would use minkowski sum / supporting vertex, merging innerloops
class ContinuousConvexCollision : public ConvexCast
{
    rage::phConvexIntersector* m_intersector;
	rage::Vec3V m_nearEnoughDistance;

public:
    
	ContinuousConvexCollision (rage::phConvexIntersector* intersector, rage::Vec::V3Param128 closeEnough);

#if !DISABLE_SPECIAL_CASE_COLLISION
	static void ComputeTimeOfImpact (const rage::NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult& pointCollector);
#endif // !DISABLE_SPECIAL_CASE_COLLISION
	static void ComputeTimeOfImpactGJK1 (const rage::NewCollisionInput * CCD_RESTRICT collisionInput, DiscreteCollisionDetectorInterface::SimpleResult * CCD_RESTRICT pointCollector, rage::GjkPairDetector * CCD_RESTRICT gjk);
	static void ComputeTimeOfImpactGJK (const rage::NewCollisionInput * CCD_RESTRICT collisionInput, DiscreteCollisionDetectorInterface::SimpleResult * CCD_RESTRICT pointCollector);

protected:
	void ComputeLinearMotion(rage::Mat34V_In fromA, rage::Mat34V_In toA, rage::Mat34V_In fromB, rage::Mat34V_In toB, rage::Vec3V_InOut linVelA
		, rage::Vec3V_InOut linVelB) const;

	void ComputeInterpolatedMotion(rage::Mat34V_In fromA, rage::Mat34V_In toA, rage::Mat34V_In fromB, rage::Mat34V_In toB, rage::Vec3V_InOut linVelA
		, rage::Vec3V_InOut linVelB, rage::Vec3V_InOut angVelA, rage::Vec3V_InOut angVelB, rage::QuatV_InOut quat0A, rage::QuatV_InOut quat0B
		, rage::QuatV_InOut quatDeltaA, rage::QuatV_InOut quatDeltaB) const;
};

#endif //CONTINUOUS_COLLISION_CONVEX_CAST_H

