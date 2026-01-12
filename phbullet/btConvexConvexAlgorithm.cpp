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
#if 0

#include <stdio.h>

#include "btCollisionDispatcher.h"
#include "btConvexConvexAlgorithm.h"
#include "btGjkEpaPenetrationDepthSolver.h"
#include "CollisionObject.h"
#include "ConvexPenetrationDepthSolver.h"
#include "ContinuousConvexCollision.h"
#include "DiscreteCollisionDetectorInterface.h"
#include "GjkPairDetector.h"
#include "PointCollector.h"
#include "SimdTransformUtil.h"
#include "TrianglePenetrationDepthSolver.h"

#include "phbound/bound.h"
#include "phbound/boundcomposite.h"
#include "phbound/support.h"
#include "physics/collision.h"
#include "physics/manifoldresult.h"
#include "physics/simulator.h"
#include "profile/element.h"

#include "vectormath/classes.h"

//#include "btSubSimplexConvexCast.h"
//#include "btGjkConvexCast.h"

namespace rage {

namespace phCollisionStats
{
    EXT_PF_TIMER(ConvexConvex);
}

}

using namespace rage::phCollisionStats;

#include "MinkowskiSumShape.h"
#include "VoronoiSimplexSolver.h"

#include "MinkowskiPenetrationDepthSolver.h"

//#include "NarrowPhaseCollision/EpaPenetrationDepthSolver.h"

#ifdef WIN32
#if _MSC_VER >= 1310
//only use SIMD Hull code under Win32
#ifdef TEST_HULL
#define USE_HULL 1
#endif //TEST_HULL
#endif //_MSC_VER 
#endif //WIN32


#ifdef USE_HULL

#include "NarrowPhaseCollision/Hull.h"
#include "NarrowPhaseCollision/HullContactCollector.h"


#endif //USE_HULL


#ifdef WIN32
void DrawRasterizerLine(const float* from,const float* to,int color);
#endif




//#define PROCESS_SINGLE_CONTACT
#ifdef WIN32
bool gForceBoxBox = false;//false;//true;

#else
bool gForceBoxBox = false;//false;//true;
#endif
bool gBoxBoxUseGjk = true;//true;//false;
bool gDisableConvexCollision = false;


btConvexConvexAlgorithm::btConvexConvexAlgorithm()
{
}



btConvexConvexAlgorithm::~btConvexConvexAlgorithm()
{
}


#if 0
class FlippedContactResult : public DiscreteCollisionDetectorInterface::ResultProcessor
{
	DiscreteCollisionDetectorInterface::Result* m_org;

public:

	FlippedContactResult(DiscreteCollisionDetectorInterface::Result* org)
		: m_org(org)
	{

	}

	virtual void AddContactPoint(rage::Vec::V3Param128 normalOnBInWorld,rage::Vec::V3Param128 pointOnAInWorld,rage::Vec::V3Param128_After3Args pointOnBInWorld,rage::Vec::V3Param128 separation,int elementA,int elementB)
	{
		using namespace rage;

		Vec::Vector_4V flippedNormal = Vec::V4Negate( normalOnBInWorld );
		m_org->AddContactPoint(flippedNormal, pointOnAInWorld, pointOnBInWorld, separation, elementA, elementB);
	}

};
#endif

//static MinkowskiPenetrationDepthSolver	gMinkowskiPenetrationDepthSolver;
//static TrianglePenetrationDepthSolver gTrianglePenetrationDepthSolver;

//static btGjkEpaPenetrationDepthSolver	gEpaPenetrationDepthSolver;

#ifdef USE_HULL

Transform	GetTransformFrombtTransform(const btTransform& trans)
{
			//const btVector3& rowA0 = trans.getBasis().getRow(0);
			////const btVector3& rowA1 = trans.getBasis().getRow(1);
			//const btVector3& rowA2 = trans.getBasis().getRow(2);

			btVector3 rowA0 = trans.getBasis().getColumn(0);
			btVector3 rowA1 = trans.getBasis().getColumn(1);
			btVector3 rowA2 = trans.getBasis().getColumn(2);


			Vector3	x(rowA0.getX(),rowA0.getY(),rowA0.getZ());
			Vector3	y(rowA1.getX(),rowA1.getY(),rowA1.getZ());
			Vector3	z(rowA2.getX(),rowA2.getY(),rowA2.getZ());
			
			Matrix33 ornA(x,y,z);
	
			Point3 transA(
				trans.getOrigin().getX(),
				trans.getOrigin().getY(),
				trans.getOrigin().getZ());

			return Transform(ornA,transA);
}

class btManifoldResultCollector : public HullContactCollector
{
public:
	btManifoldResult& m_manifoldResult;

	btManifoldResultCollector(btManifoldResult& manifoldResult)
		:m_manifoldResult(manifoldResult)
	{

	}
	

	virtual ~btManifoldResultCollector() {};

	virtual int	BatchAddContactGroup(const btSeparation& sep,int numContacts,const Vector3& normalWorld,const Vector3& tangent,const Point3* positionsWorld,const float* depths)
	{
		for (int i=0;i<numContacts;i++)
		{
			//printf("numContacts = %i\n",numContacts);
			btVector3 normalOnBInWorld(sep.m_axis.GetX(),sep.m_axis.GetY(),sep.m_axis.GetZ());
			//normalOnBInWorld.normalize();
			btVector3 pointInWorld(positionsWorld[i].GetX(),positionsWorld[i].GetY(),positionsWorld[i].GetZ());
			float separation = -depths[i];
			m_manifoldResult.addContactPoint(normalOnBInWorld,pointInWorld,separation);

		}
		return 0;
	}

	virtual int		GetMaxNumContacts() const
	{
		return 4;
	}

};
#endif //USE_HULL

//
// Convex-Convex collision algorithm
//

void btConvexConvexAlgorithm::DetectCollision (const rage::phCollisionInput& input, rage::Vector3::Param offsetA, rage::phManifold& manifold, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult)
{
	PF_START(ConvexConvex);

    int penetrationSolver;
    if (input.boundB->GetType() == rage::phBound::TRIANGLE)
    {
        penetrationSolver = PHSIM->GetConcavePenetration();
    }
    else
    {
        penetrationSolver = PHSIM->GetConvexPenetration();
    }

    const rage::phBound* minA = manifold.GetBoundA();
    Assert(minA);
    if (minA->GetType() == rage::phBound::COMPOSITE)
    {
        minA = static_cast<const rage::phBoundComposite*>(minA)->GetBound(manifold.GetComponentA());
    }

    const rage:: phBound* minB = manifold.GetBoundB();
    Assert(minB);
    if (minB->GetType() == rage::phBound::COMPOSITE)
    {
        minB = static_cast<const rage::phBoundComposite*>(minB)->GetBound(manifold.GetComponentB());
    }

	rage::phConvexIntersector intersector(minA, minB);

    if (penetrationSolver == rage::phSimulator::Penetration_Triangle)
    {
		intersector.SetPenetrationDepthSolverType(rage::phConvexIntersector::PDSOLVERTYPE_TRIANGLE);
    }
    else if (penetrationSolver == rage::phSimulator::Penetration_GJK_EPA)
    {
		intersector.SetPenetrationDepthSolverType(rage::phConvexIntersector::PDSOLVERTYPE_EPA);
    }					
    else
    {
		intersector.SetPenetrationDepthSolverType(rage::phConvexIntersector::PDSOLVERTYPE_MINKOWSKI);
    }

	rage::GjkPairDetector::ClosestPointInput cpInput;

#if 0
	if (constructionInfo.useContinuous)
	{
		// Compute linear and angular velocity for this interval, to interpolate. The linear velocity is in meters per frame, and the angular velocity is in radians per frame.
		rage::Vector3 linVelA,angVelA,linVelB,angVelB;
		SimdTransformUtil::CalculateVelocity(constructionInfo.lastMatrixA,matrixA,rage::Vec::V4VConstant<rage::V_ONE>(),linVelA,angVelA);
		SimdTransformUtil::CalculateVelocity(constructionInfo.lastMatrixB,matrixB,rage::Vec::V4VConstant<rage::V_ONE>(),linVelB,angVelB);

		float boundingRadiusA = minA->GetAngularMotionDisc();
		float boundingRadiusB = minB->GetAngularMotionDisc();

		float maxAngularProjectedVelocity = angVelA.Mag() * boundingRadiusA + angVelB.Mag() * boundingRadiusB;
		float radius = 0.001f;
		float lambda = 0.0f;
		rage::Vector3 v(rage::XAXIS);
		int maxIter = 4;
		rage::Vector3 n(rage::ORIGIN);

		float lastLambda = lambda;
		//float epsilon = 0.001f;

		int numIter = 0;
		//first solution, using GJK

		PointCollector pointCollector;

		rage::GjkPairDetector::ClosestPointInput cpInput;
		cpInput.m_transformA = constructionInfo.lastMatrixA;
		cpInput.m_transformB = constructionInfo.lastMatrixB;
		gjkPairDetector.GetClosestPoints(cpInput,pointCollector);

		bool hasResult = pointCollector.m_hasResult;
		rage::Vector3 c = pointCollector.m_pointInWorld;
		if (hasResult)
		{
			float dist = pointCollector.m_distance;
			n = pointCollector.m_normalOnBInWorld;
			rage::Matrix34 interpolatedTransA = constructionInfo.lastMatrixA;
			rage::Matrix34 interpolatedTransB = constructionInfo.lastMatrixB;
			
			ConvexCast::CastResult ccdResult;

			//not close enough
			bool hit = true;
			while (dist > radius)
			{
				numIter++;
				if (numIter>maxIter)
				{
					//rage::Displayf("Max iterations exceeded in ContinuousConvexCollision::ComputeTimeOfImpact");
					return; //todo: report a failure
				}

				//calculate safe moving fraction from distance / (linear+rotational velocity)
				//float clippedDist  = GEN_min(angularConservativeRadius,dist);
				//float clippedDist  = dist;

				float projectedLinearVelocity = (linVelB-linVelA).Dot(n);
				float dLambda = dist / (projectedLinearVelocity+ maxAngularProjectedVelocity);
				lambda += dLambda;

				if (lambda > 1.0f || lambda < 0.0f)
				{
					return;
				}

				//todo: next check with relative epsilon
				if (lambda <= lastLambda)
				{
					break;
				}

				lastLambda = lambda;

				//interpolate to next lambda
				rage::Matrix34 relativeTrans;

				SimdTransformUtil::IntegrateTransform(constructionInfo.lastMatrixA,linVelA,angVelA,lambda,interpolatedTransA);
				SimdTransformUtil::IntegrateTransform(constructionInfo.lastMatrixB,linVelB,angVelB,lambda,interpolatedTransB);
				relativeTrans.DotTranspose(interpolatedTransB, interpolatedTransA);

				ccdResult.DebugDraw( lambda );

				rage::GjkPairDetector::ClosestPointInput cpInput;
				cpInput.m_transformA = interpolatedTransA;
				cpInput.m_transformB = interpolatedTransB;
				gjkPairDetector.GetClosestPoints(cpInput,pointCollector);
				if (pointCollector.m_hasResult)
				{
					if (pointCollector.m_distance<0.0f)
					{
						//degenerate ?!
						ccdResult.m_fraction = lastLambda;
						ccdResult.m_normal = n;
						interpolatedTransA.UnTransform(pointCollector.m_pointInWorld, ccdResult.m_pointOnA);
						interpolatedTransB.UnTransform(pointCollector.m_pointInWorld, ccdResult.m_pointOnB);
						hit = false;
						break;
					}
					c = pointCollector.m_pointInWorld;		

					dist = pointCollector.m_distance;
				}
				else
				{
					//??
					hit = false;
					break;
				}
			}

			if (hit)
			{
				rage::Vector3 pointA = pointCollector.m_pointInWorld + pointCollector.m_normalOnBInWorld * pointCollector.m_distance;
				interpolatedTransA.UnTransform(pointA, ccdResult.m_pointOnA);
				interpolatedTransB.UnTransform(pointCollector.m_pointInWorld, ccdResult.m_pointOnB);

				ccdResult.m_fraction = lambda;
				ccdResult.m_normal = n;
	//			body1.m_hitFraction = Min(body1.m_hitFraction,ccdResult.m_fraction);
	//			body2.m_hitFraction = Min(body2.m_hitFraction,ccdResult.m_fraction);
	//			if (ccdResult.m_fraction < 1.0f)
	//			{
					// The collision occurs on this frame.
	//				Vector3 worldA,worldB;
	//				body1.m_worldTransform.Transform(ccdResult.m_pointOnA, worldA);
	//				body2.m_worldTransform.Transform(ccdResult.m_pointOnB, worldB);
	//				float dist = worldA.Dist(worldB);

	//				phContact point(ccdResult.m_pointOnA,ccdResult.m_pointOnB,ccdResult.m_normal,-dist,0,0); // TODO: element numbers for CCD
	//				point.m_Manifold = manifold;
	//				point.ActivateContact();

	//				manifold->AddManifoldPoint(point);

	//				manifold->instanceA = pInst1;
	//				manifold->instanceB = pInst2;
	//				manifold->colliderA = collider1;
	//				manifold->colliderB = collider2;

	//				PHSIM->ManifoldCollision(*manifold);
	//			}
			}
		}
	}
	else
#endif
	{
		cpInput.m_offset = RCC_VEC3V(offsetA);
		cpInput.m_transformA = input.currentA;
		cpInput.m_transformA.SetCol3( cpInput.m_transformA.GetCol3() - RCC_VEC3V(offsetA) );
		cpInput.m_transformB = input.currentB;

		intersector.GetClosestPoints(cpInput,manifoldResult);
	}

	PF_STOP(ConvexConvex);
}


#if 0
float	btConvexConvexAlgorithm::calculateTimeOfImpact(rage::phInst* ,rage::phInst* ,const btDispatcherInfo& )
{
    return 0.0f;

	///Rather then checking ALL pairs, only calculate TOI when motion exceeds treshold
    
	///Linear motion for one of objects needs to exceed m_ccdSquareMotionTreshold
	///col0->m_worldTransform,
	float resultFraction = 1.f;

	btCollisionObject* col1 = static_cast<btCollisionObject*>(m_box1.m_clientObject);
	btCollisionObject* col0 = static_cast<btCollisionObject*>(m_box0.m_clientObject);

	float squareMot0 = (col0->m_interpolationWorldTransform.getOrigin() - col0->m_worldTransform.getOrigin()).length2();
    
	if (squareMot0 < col0->m_ccdSquareMotionTreshold && squareMot0 < col0->m_ccdSquareMotionTreshold)
	{
		return resultFraction;
	}

	checkPenetrationDepthSolver();

	//An adhoc way of testing the Continuous Collision Detection algorithms
	//One object is approximated as a sphere, to simplify things
	//Starting in penetration should report no time of impact
	//For proper CCD, better accuracy and handling of 'allowed' penetration should be added
	//also the mainloop of the physics should have a kind of toi queue (something like Brian Mirtich's application of Timewarp for Rigidbodies)

	bool needsCollision = m_dispatcher->needsCollision(m_box0,m_box1);

	if (!needsCollision)
		return 1.f;
	
		
	/// Convex0 against sphere for Convex1
	{
		btConvexShape* convex0 = static_cast<btConvexShape*>(col0->m_collisionShape);

		btSphereShape	sphere1(col1->m_ccdSweptShereRadius); //todo: allow non-zero sphere sizes, for better approximation
		btConvexCast::CastResult result;
		btVoronoiSimplexSolver voronoiSimplex;
		//SubsimplexConvexCast ccd0(&sphere,min0,&voronoiSimplex);
		///Simplification, one object is simplified as a sphere
		btGjkConvexCast ccd1( convex0 ,&sphere1,&voronoiSimplex);
		//ContinuousConvexCollision ccd(min0,min1,&voronoiSimplex,0);
		if (ccd1.ComputeTimeOfImpact(col0->m_worldTransform,col0->m_interpolationWorldTransform,col1->m_worldTransform,col1->m_interpolationWorldTransform,result))
		{
			//store result.m_fraction in both bodies
		
			if (col0->m_hitFraction	> result.m_fraction)
			{
				col0->m_hitFraction  = result.m_fraction;
			}

			if (col1->m_hitFraction > result.m_fraction)
			{
				col1->m_hitFraction  = result.m_fraction;
			}

			if (resultFraction > result.m_fraction)
			{
				resultFraction = result.m_fraction;
			}

		}
		
		


	}

	/// Sphere (for convex0) against Convex1
	{
		btConvexShape* convex1 = static_cast<btConvexShape*>(col1->m_collisionShape);

		btSphereShape	sphere0(col0->m_ccdSweptShereRadius); //todo: allow non-zero sphere sizes, for better approximation
		btConvexCast::CastResult result;
		btVoronoiSimplexSolver voronoiSimplex;
		//SubsimplexConvexCast ccd0(&sphere,min0,&voronoiSimplex);
		///Simplification, one object is simplified as a sphere
		btGjkConvexCast ccd1(&sphere0,convex1,&voronoiSimplex);
		//ContinuousConvexCollision ccd(min0,min1,&voronoiSimplex,0);
		if (ccd1.ComputeTimeOfImpact(col0->m_worldTransform,col0->m_interpolationWorldTransform,col1->m_worldTransform,col1->m_interpolationWorldTransform,result))
		{
			//store result.m_fraction in both bodies
		
			if (col0->m_hitFraction	> result.m_fraction)
			{
				col0->m_hitFraction  = result.m_fraction;
			}

			if (col1->m_hitFraction > result.m_fraction)
			{
				col1->m_hitFraction  = result.m_fraction;
			}

			if (resultFraction > result.m_fraction)
			{
				resultFraction = result.m_fraction;
			}
		}
	}
	
	return resultFraction;
}
#endif

#endif