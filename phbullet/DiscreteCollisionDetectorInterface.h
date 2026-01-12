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


#ifndef DISCRETE_COLLISION_DETECTOR_INTERFACE_H
#define DISCRETE_COLLISION_DETECTOR_INTERFACE_H

#include "phCore/constants.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#define TRACK_COLLISION_TIME (0 && HACK_GTA4)

#if TRACK_COLLISION_TIME
#define TRACK_COLLISION_TIME_ONLY(X) X
#else
#define TRACK_COLLISION_TIME_ONLY(X)
#endif

#if TRACK_COLLISION_TIME
#define TRACK_COLLISION_TIME_PARAM(X) , X
#else
#define TRACK_COLLISION_TIME_PARAM(X)
#endif

namespace rage
{
	class phBound;
};

/// This interface is made to be used by an iterative approach to do TimeOfImpact calculations
/// This interface allows to query for closest points and penetration depth between two (convex) objects.
/// The closest point is on the second object (B), and the normal points from the surface on B towards A.
/// Distance is between closest points on B and closest point on A. So you can calculate closest point on A
/// by taking closestPointInA = closestPointInB + m_distance * m_normalOnSurfaceB
struct DiscreteCollisionDetectorInterface
{
	void operator delete(void* UNUSED_PARAM(ptr)) {};

	class SimpleResult
	{
	public:
		SimpleResult() :
			m_HasResult(false)
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
			, m_SecondSurfaceDepth(0.0f)
#endif // HACK_GTA4_BOUND_GEOM_SECOND_SURFACE

		{
		}


		void AddContactPoint(rage::Vec::V3Param128 normalOnBInWorld,
			rage::Vec::V3Param128_After3Args separation,int elementA,int elementB
			,rage::Vec::V3Param128_After3Args pointOnAInLocal, rage::Vec::V3Param128_After3Args pointOnBInLocal
#if TRACK_COLLISION_TIME
			,float time
#endif // TRACK_COLLISION_TIME
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
			,float SecondSurfaceDepth = 0.0f
#endif // HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
)
		{
			using namespace rage;

			Vec3V v_separation(separation);
			Assert(!GetHasResult());

			m_HasResult = true;
			m_NormalOnBInWorld = Vec3V(normalOnBInWorld);

			m_PointOnAInLocal = Vec3V(pointOnAInLocal);
			m_PointOnBInLocal = Vec3V(pointOnBInLocal);

			//negative means penetration
			m_Distance = v_separation;
			// This assert is very loose and really shouldn't fail, but it does, particularly from the specialized box-box test it seems.
			//Assert(IsLessThanOrEqualAll(v_separation.GetX(), Add(Dist(m_PointOnAInWorld, m_PointOnBInWorld), ScalarV(V_FLT_SMALL_1))));

			m_ElementA = elementA;
			m_ElementB = elementB;
#if TRACK_COLLISION_TIME
			m_Time = time;
#endif
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
			m_SecondSurfaceDepth = SecondSurfaceDepth;
#endif
		}

		__forceinline rage::Vec3V_Out GetNormalOnBInWorld() const
		{
			return m_NormalOnBInWorld;
		}

		__forceinline void SetNormalOnBInWorld(rage::Vec3V_In normalOnB)
		{
			m_NormalOnBInWorld = normalOnB;
		}

		__forceinline rage::Vec3V_Out GetPointOnAInLocal() const
		{
			return m_PointOnAInLocal;
		}

		__forceinline void SetPointOnAInLocal(rage::Vec3V_In pointOnA)
		{
			m_PointOnAInLocal = pointOnA;
		}

		__forceinline rage::Vec3V_Out GetPointOnBInLocal() const
		{
			return m_PointOnBInLocal;
		}

		__forceinline void SetPointOnBInLocal(rage::Vec3V_In pointOnA)
		{
			m_PointOnBInLocal = pointOnA;
		}

		__forceinline rage::Vec3V_Out GetDistanceV() const
		{
			return m_Distance;
		}

		__forceinline void SetDistanceV(rage::Vec3V_In distance)
		{
			m_Distance = distance;
		}

		__forceinline int GetElementA() const
		{
			return m_ElementA;
		}

		__forceinline int GetElementB() const
		{
			return m_ElementB;
		}

#if TRACK_COLLISION_TIME
		__forceinline float GetTime() const
		{
			return m_Time;
		}

		__forceinline void SetTime(float time)
		{
			m_Time = time;
		}

		__forceinline void SetTime(rage::ScalarV_In time)
		{
			StoreScalar32FromScalarV(m_Time, time);
		}
#endif

		__forceinline bool GetHasResult() const
		{
			return m_HasResult;
		}

		__forceinline void SetHasResult(bool hasResult)
		{
			m_HasResult = hasResult;
		}

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
		__forceinline float GetSecondSurfaceDepth() const
		{
			return m_SecondSurfaceDepth;
		}
#endif

	protected:
		rage::Vec3V m_NormalOnBInWorld;
		rage::Vec3V m_Distance; //negative means penetration

		// Using the alt update order contacts can be added/used at frame times other than 1
		// Therefore we need the local coordinates used during detection
		//  It is no longer sufficient to assume final matrices and derive local space 
		//  points from the world space ones above
		rage::Vec3V m_PointOnAInLocal;
		rage::Vec3V m_PointOnBInLocal;

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
		float m_SecondSurfaceDepth;
#endif

		int		m_ElementA, m_ElementB;
		//float m_Friction, m_Elasticity;
#if TRACK_COLLISION_TIME
		float m_Time;
#endif

		bool	m_HasResult;
	};

	class ResultProcessor
	{
	public:
//        virtual ~ResultProcessor() { }
		void operator delete(void* UNUSED_PARAM(ptr)) {};

//		virtual void ProcessResult(rage::Mat34V_In transformA, rage::Mat34V_In transformB, const SimpleResult &collisionDetectionResult) = 0;

	protected:
	};

	struct ClosestPointInput
	{
		ClosestPointInput(rage::ScalarV_In sepThreshold, const rage::phBound * boundA, const rage::phBound * boundB, const rage::u32 pdsType, const void * gjkCache) :
			m_sepThreshold(sepThreshold), m_boundA(boundA), m_boundB(boundB), m_pdsType(pdsType), m_gjkCache(gjkCache)
		{
		}

		rage::Mat34V m_transformA;
        rage::Mat34V m_transformB;
		rage::ScalarV m_sepThreshold;
		const rage::phBound * m_boundA;
		const rage::phBound * m_boundB;
		rage::u32 m_pdsType;
		const void * m_gjkCache;
	};

	struct ClosestPointOutput
	{
		rage::Vec3V worldNormalBtoA;
		rage::ScalarV separationWithMargin;
		bool isValid;
		bool m_needsPenetrationSolve;

		bool isSeparatedBeyondSepThresh;	// If this is true then worldPointA and worldPointB will contain the last support points. worldNormalBtoA will contain the last support direction normalized.
											// separationWithMargin will contain the distance between the margin-expanded objects along the support direction.
		__forceinline void SetSeparationParams(rage::ScalarV_In separationWithMargin_, rage::Vec3V_In worldNormalBtoA_)
		{
			isValid = true;
			isSeparatedBeyondSepThresh = true;
			m_needsPenetrationSolve = false;
			separationWithMargin = separationWithMargin_;
			worldNormalBtoA = worldNormalBtoA_;
		}
	};

};

#endif //DISCRETE_COLLISION_DETECTOR_INTERFACE_H
