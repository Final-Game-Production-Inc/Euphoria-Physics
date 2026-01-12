//
// physics/shapetestspu.h
//
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBULLET_CONVEXINTERSECTOR
#define PHBULLET_CONVEXINTERSECTOR

#include "DiscreteCollisionDetectorInterface.h"

//#if TRACK_COLLISION_TYPE_PAIRS
#include "phbound/bound.h" // for phBound::NUM_BOUND_TYPES
//#endif

#define USE_BOX_BOX_INTERSECT (0 && ENABLE_UNUSED_PHYSICS_CODE)

// Disable the special case collision functions in debug/de-optimized builds because they overflow the stack.
// Also, disable them on the SPU in beta builds to prevent running out of memory (currently only needed in collision task). 
// (If this doesn't work then try setting the compile options to optimize for size)
// If you need to run the special case collision functions in debug then you will need to increase the stack size.
#define DISABLE_SPECIAL_CASE_COLLISION (GJK_COLLISION_DEBUG || !__OPTIMIZED || (REDUCE_SPU_CODE_SIZE && __SPU))	

// Turning this on for SPU disables asserts automatically in order to save enough space
#if DISABLE_SPECIAL_CASE_COLLISION

#define USE_BOX_BOX_DISTANCE (0 && ENABLE_UNUSED_PHYSICS_CODE)

#define CAPSULE_TO_CAPSULE_DISTANCE 0

#define CAPSULE_TO_TRIANGLE_DISTANCE 0

#define DISC_TO_TRIANGLE_DISTANCE 0

#define BOX_TO_TRIANGLE_DISTANCE 0

#else // DISABLE_SPECIAL_CASE_COLLISION

#define USE_BOX_BOX_DISTANCE ( (!__SPU && !USE_FRAME_PERSISTENT_GJK_CACHE) && ENABLE_UNUSED_PHYSICS_CODE )

#define CAPSULE_TO_CAPSULE_DISTANCE 1

#define CAPSULE_TO_TRIANGLE_DISTANCE 1

#define DISC_TO_TRIANGLE_DISTANCE 1

#define BOX_TO_TRIANGLE_DISTANCE 0

#endif // DISABLE_SPECIAL_CASE_COLLISION

namespace rage {

class phBound;
class GjkPairDetector;

// phConvexIntersector is class for finding a contact point between two intersecting convex objects.  To accomplish this, it uses GJK and an optional penetration
//   depth solver to generate contact point information representing the simplest way to de-penetrate the two objects.
// It (as a parameter to GetClosestPoints()) takes a DiscreteCollisionDetectorInterface::ClosestPointInput as input and natively fills in a
//   DiscreteCollisionDetectorInterface::SimpleResult as output.  As a convenience an alternate version of GetClosestPoints() is available that takes a
//   DiscreteCollisionDetectorInterface::ResultProcessor and automatically calls ProcessResult() on the result.
class phConvexIntersector
{
public:
	phConvexIntersector() :
		m_BoundA(NULL),
		m_BoundB(NULL),
		m_PenetrationDepthSolverType(PDSOLVERTYPE_NONE)
	{
	}

	phConvexIntersector(const phBound *boundA, const phBound *boundB) :
		m_BoundA(boundA),
		m_BoundB(boundB),
		m_PenetrationDepthSolverType(PDSOLVERTYPE_NONE)
	{
	}

	~phConvexIntersector()
	{
	}

	enum PenetrationDepthSolverType
	{
		PDSOLVERTYPE_NONE		= 0,
		PDSOLVERTYPE_TRIANGLE	= 1,
		PDSOLVERTYPE_MINKOWSKI	= 2,
		PDSOLVERTYPE_EPA		= 3,
		PDSOLVERTYPE_COUNT
	};

	void SetPenetrationDepthSolverType(PenetrationDepthSolverType newPenetrationDepthSolverType)
	{
		m_PenetrationDepthSolverType = newPenetrationDepthSolverType;
	}

	// This function is mis-named.  It should be called "FindOverlap" or "FindIntersection" or "FindContactPoint" or something like that.
	static void GetClosestPoints(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output);

#if !DISABLE_SPECIAL_CASE_COLLISION
	// This returns true if a special case function processed the collision.
	static bool GetClosestPointsSpecialCase(const DiscreteCollisionDetectorInterface::ClosestPointInput& input, DiscreteCollisionDetectorInterface::SimpleResult& output);
#endif

	static void GjkPenetrationSolve(const DiscreteCollisionDetectorInterface::ClosestPointInput * CCD_RESTRICT gjkInput, DiscreteCollisionDetectorInterface::ClosestPointOutput * CCD_RESTRICT gjkOutput, GjkPairDetector * CCD_RESTRICT gjkdet);

	// This returns true if the penetration solver was executed, false otherwise.
	static bool GjkClosestPoints(const DiscreteCollisionDetectorInterface::ClosestPointInput * CCD_RESTRICT gjkInput, DiscreteCollisionDetectorInterface::ClosestPointOutput * CCD_RESTRICT gjkOutput, GjkPairDetector * CCD_RESTRICT gjkdet);

	static void WriteSimpleResult(const DiscreteCollisionDetectorInterface::ClosestPointInput * CCD_RESTRICT gjkInput, const DiscreteCollisionDetectorInterface::ClosestPointOutput * CCD_RESTRICT gjkOutput, const GjkPairDetector * CCD_RESTRICT gjkdet, DiscreteCollisionDetectorInterface::SimpleResult * CCD_RESTRICT output);

	void SetBoundA(const phBound* boundA)
	{
		m_BoundA = boundA;
	}

	void SetBoundB(const phBound* boundB)
	{
		m_BoundB = boundB;
	}

	const phBound *GetBoundA() const
	{
		return m_BoundA;
	}

	const phBound *GetBoundB() const
	{
		return m_BoundB;
	}

private:
	const phBound *m_BoundA;
	const phBound *m_BoundB;
	PenetrationDepthSolverType m_PenetrationDepthSolverType;
};

#if TRACK_COLLISION_TYPE_PAIRS
extern u32 g_CollisionTypePairTable[][phBound::NUM_BOUND_TYPES];

#if __SPU
void SendTypePairTableToMainMemory(u32 mainMemoryTable[][phBound::NUM_BOUND_TYPES]);
#endif
#endif

#if !DISABLE_SPECIAL_CASE_COLLISION
__forceinline bool UseSpecialCaseCollision(const phBound * boundA, const phBound * boundB)
{

#if __BANK

#if USE_BOX_BOX_DISTANCE
	extern bool g_UseBoxBoxDistance;
#endif

#if CAPSULE_TO_CAPSULE_DISTANCE
	extern bool g_CapsuleCapsuleDistance;
#endif

#if CAPSULE_TO_TRIANGLE_DISTANCE
	extern bool g_CapsuleTriangleDistance;
#endif

#if DISC_TO_TRIANGLE_DISTANCE
	extern bool g_DiscTriangleDistance;
#endif

#if BOX_TO_TRIANGLE_DISTANCE
	extern bool g_BoxTriangleDistance;
#endif

#endif // __BANK

	(void)boundA;
	(void)boundB;
	FastAssert(boundA);
	FastAssert(boundB);

	// Code copied from GetClosestPoints.
	const int typeShift = 4;
	CompileTimeAssert(phBound::NUM_BOUND_TYPES < (1 << typeShift));
	const int testType = (boundA->GetType() << typeShift) | boundB->GetType();
	switch(testType)
	{
#if USE_BOX_BOX_DISTANCE
	case (phBound::BOX << typeShift) | phBound::BOX:
		{
#if __BANK
			const bool useBoxBoxDistance = g_UseBoxBoxDistance;
#else
			const bool useBoxBoxDistance = true;
#endif
			return useBoxBoxDistance;
		}
#endif

#if CAPSULE_TO_CAPSULE_DISTANCE
	case (phBound::CAPSULE << typeShift) | phBound::CAPSULE:
		{
#if __BANK
			const bool useFastCapsuleToCapsule = g_CapsuleCapsuleDistance;
#else
			const bool useFastCapsuleToCapsule = true;
#endif
			return useFastCapsuleToCapsule;
		}
#endif

#if CAPSULE_TO_TRIANGLE_DISTANCE
	case (phBound::CAPSULE << typeShift) | phBound::TRIANGLE:
		{
#if __BANK
			const bool useFastCapsuleToTriangle = g_CapsuleTriangleDistance;
#else
			const bool useFastCapsuleToTriangle = true;
#endif
			return useFastCapsuleToTriangle;
		}
#endif

#if DISC_TO_TRIANGLE_DISTANCE
	case (phBound::DISC << typeShift) | phBound::TRIANGLE:
		{
#if __BANK
			const bool useFastDiscToTriangle = g_DiscTriangleDistance;
#else
			const bool useFastDiscToTriangle = true;
#endif
			return useFastDiscToTriangle;
		}
#endif

#if BOX_TO_TRIANGLE_DISTANCE
	case (phBound::BOX << typeShift) | phBound::TRIANGLE:
		{
#if __BANK
			const bool useFastBoxToTriangle = g_BoxTriangleDistance;
#else
			const bool useFastBoxToTriangle = true;
#endif
			return useFastBoxToTriangle;
		}
#endif
	}
	return false;
}
#endif // !DISABLE_SPECIAL_CASE_COLLISION

} // namespace rage

#endif	// PHBULLET_CONVEXINTERSECTOR
