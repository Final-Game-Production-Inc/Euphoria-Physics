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

#include "btConvexConcaveCollisionAlgorithm.h"

#if ENABLE_UNUSED_PHYSICS_CODE

#include "btConvexConvexAlgorithm.h"
#include "CollisionObject.h"
#include "ContinuousConvexCollision.h"
#include "ConvexCast.h"
#include "GjkPairDetector.h"
#include "PointCollector.h"
#include "SimdTransformUtil.h"
#include "TriangleShape.h"

#include "phbound/boundbvh.h"			// For phBVHCuller.
#include "phbound/boundcomposite.h"
#include "phbound/boundculler.h"
#include "phbound/support.h"
#include "physics/collision.h"
#include "physics/ManifoldResult.h"
#include "physics/simulator.h"
#include "profile/element.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#include "system/timer.h"

// This version of normal clamping is deprecated.  It is left for now only as a reference.
#define ENABLE_NORMAL_CLAMPING			0
// This is the new version of normal clamping.  It is much simpler and less expensive than the old method.
#define ENABLE_NORMAL_CLAMPING_SIMPLE	(1 && !ENABLE_NORMAL_CLAMPING)
#if ENABLE_NORMAL_CLAMPING
#define ADJUST_DEPTH_POST_NORMAL_CLAMP	0
#endif	// ENABLE_NORMAL_CLAMPING

// Turn on this #define to verify that the pair detector isn't returning collisions that aren't close to aligned with the polygon's face normal but
//   don't lie on an edge either.
#define CHECK_NO_INTERIOR_EDGE_COLLISIONS	0

#define CONTACT_FILTERING_IMPL	1

namespace rage {

namespace phCollisionStats
{
    EXT_PF_TIMER(ContactMaintenance);
    EXT_PF_TIMER(ManifoldMaintenance);
    EXT_PF_TIMER(TriangleConvex);
    EXT_PF_TIMER(ConcaveConvex);
}

// Used to be defined in simulator.cpp, now it's gone from there so the easiest thing is to just define it locally here.  It's not really important though.
bool g_AutoCCDFirstContactOnly = false;
extern bool g_UseNormalFiltering;
extern bool g_UseNormalClamping;


using namespace phCollisionStats;

btConvexConcaveCollisionAlgorithm::btConvexConcaveCollisionAlgorithm()
{
}

btConvexConcaveCollisionAlgorithm::~btConvexConcaveCollisionAlgorithm()
{
}


btConvexTriangleCallback::btConvexTriangleCallback(const phBound* convexShape)
    : m_convexShape(convexShape)
    , m_useNormalFiltering(true)
    , m_useContinuous(false)
	, m_autoCCD(false)
{
}


btConvexTriangleCallback::~btConvexTriangleCallback()
{
}


inline bool LocalIsPointOnSeg(Vec::V3Param128 point1, Vec::V3Param128 point2, Vec::V3Param128 target, Vec::V3Param128_After3Args squaredMargin)
{
	using namespace rage;

#if 1
	Vec3V v_point1(point1);
	Vec3V newPoint1(V_ZERO);
	Vec3V newPoint2(point2);
	newPoint2 = Subtract( newPoint2, v_point1 );

	Vec3V newTarget(target);
	newTarget = Subtract( newTarget, v_point1 );

	Vec3V diff = newPoint2;

	Vec3V closest;
	closest = geomPoints::FindClosestPointSegToPoint(newPoint1, diff, newTarget);

	// Now calculate the vector between that closest point and the target point.
	closest = Subtract(closest, newTarget);

	// Return the squared magnitude of that vector.
	return ( 0 != IsLessThanOrEqualAll( Vec3V(MagSquared(closest)), Vec3V(squaredMargin) ) );
#else
	Vector3 diff;
	diff.Subtract(point2, point1);

	Vector3 closest;
	closest = geomPoints::FindClosestPointSegToPoint(point1,diff,target);

	// Now calculate the vector between that closest point and the target point.
	closest.Subtract(target);

	// Return the squared magnitude of that vector.
	return closest.Mag2V().IsLessOrEqualThan(squaredMargin);
#endif
}


static Vector3 kVecPointOnEdgeToleranceFactorSquared(square(1.02f), square(1.02f), square(1.02f));
#if CONTACT_FILTERING_IMPL == 0
// The smaller we can make this value the better.  I think 1.0e-1f is kind of large but I'll test more with smaller values later.
static ScalarV kVecKindOfSmallFloat = ScalarVFromF32(1.0e-1f);
#else
static ScalarV kVecKindOfSmallFloat = ScalarVFromF32(1.0e-3f);
#endif
//static ScalarV kVecKindOfSmallFloat(1.0e-3f);
//static ScalarV kVecKindOfSmallFloat(1.0e-6f);


bool CheckContactAgainstEdge(Vec3V_In localizedVertex,
							 Vec3V_In localizedVertexNext,
							 Vec3V_In localPoint,
							 Vec3V_In vecMarginSq,
							 Vec3V_In neighborNormal,
							 Vec3V_In edgeNormal,
							 Vec3V_In curContactNormal)
{
	// The triangle has a neighboring triangle connected to this edge, so see if this collision should instead go on the neighboring triangle.
	Vec3V curEdge = Subtract(localizedVertexNext, localizedVertex);
	// First, check to see if this point is on this edge.
	bool res = LocalIsPointOnSeg(localizedVertex.GetIntrin128(), localizedVertexNext.GetIntrin128(), localPoint.GetIntrin128(), vecMarginSq.GetIntrin128());
	if(res)
	{
		// It may not quite be obvious what's going on here so let me explain.
		// Basically, we're trying to determine whether the collision normal is pointed 'away' from the polygon normal more than the edge normal is.
		// If that is the case, then we'll reject the collision because it's happening on an edge in a way that actually means that it shouldn't be
		//   happening because it would be protected by the neighboring polygon.
#if CONTACT_FILTERING_IMPL == 0
		// To determine whether that is the case, we calculate a rotational value (actually the sine of the angle scaled by the length of the edge)
		//   for both of those normals (the edge normal and the collision normal) about the edge in question, relative to the face normal.  So, the
		//   face normal itself would have an angular value of zero, and the values would increase as we rotate clockwise around the edge with the
		//   edge pointing away from you.

		const ScalarV kVecEdgeLength = Mag(curEdge);

		// Calculate the angular value (see above) of the neighbor normal.
		// kThresholdAngularValue < 0.0f means that we're colliding with a concave edge.  I'm still not quite sure what the 'right' thing to do in
		//   this case is.  Perhaps some clamping in this case would be better than 
		Vec3V temp = Cross( faceNormal, neighborNormal );
		ScalarV kVecThesholdAngularValue = Max( Dot(temp, curEdge), Multiply(v_fltsmall2, kVecEdgeLength) );

		// Calculate the angular value (see above) of the contact normal.
		temp = Cross(faceNormal, curContactNormal);
		const ScalarV kVecCollisionAngularValue = Dot(temp, curEdge);

		// Remember that the significance of these values is "sine of the angle between them and the face normal times the length of the common
		//   edge".  So the 1.0e-1f value below denotes that we're allowing contact normals, the sine of whose angle is greater than the maximum
		//   allowed by 0.1.  When the angles themselves are near zero, a change in sine of 0.1 corresponds to a change in angle of about 5.7
		//   degrees.  In other words, we're allowing contact normals that are about five degrees outside of the neighbor normal.
		// Now that I think about it, it would probably be better to measure all of these angular values relative to the neighbor normal and
		//   not relative to the face normal.
		if( IsGreaterThan(kVecCollisionAngularValue, AddScaled(kVecThesholdAngularValue, kVecKindOfSmallFloat, kVecEdgeLength)) != 0 )
		{
			// The collision normal is leaning far enough away from this polygon's normal that the collision should be with the neighboring polygon.
			tm.Release(false);
			return NULL;
		}
#else
		// I could also dot the edge normal with the face normal - would that be better/faster?  (it might already be in a register)
		const ScalarV kEdgeDotNeighbor = Dot(edgeNormal, neighborNormal);

		// Ensure that there's at least a little bit of slop allowed.  This is redundant with the tolerance in the comparison below and they
		//   should really be combined into one value.
		ScalarV kMinDot;
		kMinDot = Min(kEdgeDotNeighbor, ScalarV(V_ONE)-ScalarV(V_FLT_SMALL_6));

		Vec3V curEdgeDir = curEdge;
		curEdgeDir = Normalize(curEdgeDir);

		const ScalarV contactNormalProjectionAlongEdge = Dot(curContactNormal, curEdgeDir);
		Vec3V curContactNormalPerpToEdge = curContactNormal;
		// Should I be using a tolerance here?  Maybe, but this test truly is passing a lot of the time and it saves me from having to do a
		//   Mag2V().
		if( IsEqualNone(contactNormalProjectionAlongEdge, ScalarV(V_ZERO)) != 0 )
		{
			// Generally speaking, contactNormalProjectionAlongEdge is going to be zero because the contact normal is going to be perpendicular
			//   to the edge, the exception being contacts that are at vertices.
			curContactNormalPerpToEdge = SubtractScaled(curContactNormalPerpToEdge, curEdgeDir, Vec3V(contactNormalProjectionAlongEdge));
			curContactNormalPerpToEdge = Normalize(curContactNormalPerpToEdge);
		}

		if( IsLessThanAll( Dot(curContactNormalPerpToEdge, edgeNormal), kMinDot-kVecKindOfSmallFloat ) != 0 )
		{
			// The collision normal is leaning far enough away from this polygon's normal that the collision should be with the neighboring polygon.
			return false;
		}
#endif
	}

	return true;
}

#if 0
phContact* btConvexTriangleCallback::ProcessTriangle (Vec3V_In vertices0,
													  Vec3V_In vertices1,
													  Vec3V_In vertices2,
													  Vec3V_In faceNormal,
													  Vec3V_In edgeNormals0,
													  Vec3V_In edgeNormals1,
													  Vec3V_In edgeNormals2,
													  Vec3V_In neighborNormals0,
													  Vec3V_In neighborNormals1,
													  Vec3V_In neighborNormals2,
													  int partIndexShifted,
													  phMaterialMgr::Id materialId,
													  int hasNeighbor,
													  DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult,
													  phManifold& manifold)
{
    PF_FUNC(TriangleConvex);

//	Vec3V v_zero(V_ZERO);
//	Vec3V v_localVertices[3] = { RCC_VEC3V(vertices[0]), RCC_VEC3V(vertices[1]), RCC_VEC3V(vertices[2]) };

	// Create a triangle bound for use with collision.
	// Translate the vertices of the triangle such that vertex 0 lies at the ORIGIN in the local space of the bound it's in.
	const Vec3V localizedVertices0 = Vec3V(V_ZERO);
	const Vec3V localizedVertices1 = vertices1 - vertices0;
	const Vec3V localizedVertices2 = vertices2 - vertices0;

	TriangleShape tm( VEC3V_TO_VECTOR3(Vec3V(V_ZERO)), VEC3V_TO_VECTOR3(vertices1 - vertices0), VEC3V_TO_VECTOR3(vertices2 - vertices0) );
	int partIndex = partIndexShifted >> 1;
	tm.SetIndexInBound(partIndex);
	tm.m_PolygonNormal = faceNormal;
	tm.m_EdgeNormals[0] = edgeNormals0;
	tm.m_EdgeNormals[1] = edgeNormals1;
	tm.m_EdgeNormals[2] = edgeNormals2;
    tm.SetMargin(m_collisionMarginTriangle);
	tm.SetMaterial(materialId);

    manifold.SetBoundB(&tm);
    manifold.ClearNewestContactPoint();

	m_intersector.SetPenetrationDepthSolverType(phConvexIntersector::PDSOLVERTYPE_TRIANGLE);

    const phBound* minA = manifold.GetBoundA();
    const phBound* minB = manifold.GetBoundB();

    m_intersector.SetBoundA(minA);
    m_intersector.SetBoundB(minB);

    // Initialize the separating axis. The initial separating axis should have no effect on the output, but re-initialization makes the input consistent on replay
    // to avoid difficulties fixing replay bugs.
    //m_gjkPairDetector.InitSeparatingAxis(instanceA->GetMatrix(), instanceA->GetArchetype()->GetBound(), instanceB->GetMatrix(), instanceB->GetArchetype()->GetBound());
	// Should not be needed any more with the convex intersector.
    //m_gjkPairDetector.SetCachedSeperatingAxis(XAXIS);

	// We relocated the triangle vertices in the local space of the instance 
	Vec3V triangleOffset_LS = vertices0;
    Vec3V triangleOffset_WS = Transform3x3( m_triangleMeshTransform, triangleOffset_LS );

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// A little explanation about what follows: Remember that our goal here is to relocate the objects such that the collision occurs near the origin, so
	//   as to reduce numerical precision problems.  Our chosen way to accomplish that is to translate everything such that the world space coordinate of
	//   vertex 0 of the triangle is at the origin.
	// To accomplish this for the triangle bound we translate the vertices of the triangle such that vertex 0 lies at the origin in the local space of
	//   the triangle mesh bound and then we translate the triangle mesh bound such that the origin in its local space corresponds to the origin in world
	//   space.
	// To accomplish this for the non-triangle bound we simply translate the object by the sum (in world space) of the two above-mentioned displacements.
	// IMPORTANT!  Make sure that you don't do anything that would skip over the code below that restores the positions to the matrices that we're
	//   changing here (such as by adding an early out between here and there).  You'll probably notice pretty quickly if you've done that as
	//   collision detection will get totally screwed up.

	// Save off the triangle transform positions that we're about to overwrite.
	const Vec3V oldTriangleTransformPos = m_triangleMeshTransform.GetCol3();
	const Vec3V oldTriangleLastTransformPos = m_triangleMeshLastTransform.GetCol3();
	m_triangleMeshTransform.SetCol3( Vec3V(V_ZERO) );
	m_triangleMeshLastTransform.SetCol3( Subtract( m_triangleMeshLastTransform.GetCol3(), oldTriangleTransformPos ) );

	const Vec3V finalOffset_WS = (triangleOffset_WS + oldTriangleTransformPos);

	// Save off the convex transform positions that we're about to overwrite.
	const Vec3V oldConvexTransformPos = m_convexTransform.GetCol3();
	const Vec3V oldConvexLastTransformPos = m_convexLastTransform.GetCol3();
	m_convexTransform.SetCol3( Subtract(m_convexTransform.GetCol3(), finalOffset_WS) );
	m_convexLastTransform.SetCol3( Subtract(m_convexLastTransform.GetCol3(), finalOffset_WS) );
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////

	// Store our results here until we're ready to put them into manifoldResult.  We need a temporary place to put them because we 
	DiscreteCollisionDetectorInterface::SimpleResult pointCollector;

	bool useContinuous = m_useContinuous;

	if (m_autoCCD)
	{
		useContinuous = true;

		// AutoCCD told us to use CCD, but don't do it if we already have a contact with this polygon
		if (g_AutoCCDFirstContactOnly)
		{
			for (int contactIndex = 0; contactIndex < manifold.GetNumContacts(); ++contactIndex)
			{
				if (manifold.GetContactPoint(contactIndex).GetElementB() == partIndex)
				{
					useContinuous = false;
					break;
				}
			}
		}
	}

	Mat34V transBatT;
    if (useContinuous)
    {
		ContinuousConvexCollision ccd(&m_intersector, manifold.GetManifoldMarginV().GetIntrin128());

		Mat34V offsetConvexLastTransform = m_convexLastTransform;

		ConvexCast::CastResult ccdResult;
		bool ccdHit = ccd.ComputeTimeOfImpact(offsetConvexLastTransform,m_convexTransform,m_triangleMeshLastTransform,m_triangleMeshTransform,ccdResult);

		if (ccdHit)
		{
			// The collision occurs on this frame.
			Vec3V worldA, worldB;
			worldA = Transform(ccdResult.m_hitTransformA, ccdResult.m_pointOnA);
			worldB = Transform(ccdResult.m_hitTransformB, ccdResult.m_pointOnB);

			Vec3V diff = worldB - worldA;
			Vec3V dist = Vec3V( Dot( ccdResult.m_normal, diff ) );

			pointCollector.AddContactPoint(VEC3V_TO_INTRIN(ccdResult.m_normal), VEC3V_TO_INTRIN(worldA), VEC3V_TO_INTRIN(worldB), VEC3V_TO_INTRIN(-dist), ccdResult.m_elementA, ccdResult.m_elementB
				, ccdResult.m_pointOnA.GetIntrin128(), ccdResult.m_pointOnB.GetIntrin128()
				TRACK_COLLISION_TIME_PARAM(ccdResult.m_fraction.Getf()));
			transBatT = ccdResult.m_hitTransformB;
		}
	}
	else
	{
		// Get GJK to give us the collision info.  If we want to filter the results we just collect the info and decide whether we want to create a contact
		//   out of it later on.
		GjkPairDetector::ClosestPointInput input;
		//input.m_maximumDistanceSquared = 1.0e30f;//minA->GetMargin() + minB->GetMargin() + manifold.GetManifoldMargin();
		//input.m_maximumDistanceSquared*= input.m_maximumDistanceSquared;
		// Have GJK leave the 'world space' contact points non-offset for improved precision when using normal filtering.
		input.m_offset = m_useNormalFiltering ? Vec3V(V_ZERO) : finalOffset_WS;
		input.m_transformA = m_convexTransform;
		input.m_transformB = m_triangleMeshTransform;

		// Run GJK.
		m_intersector.GetClosestPoints(input, pointCollector);
		transBatT = m_triangleMeshTransform;
	}

	// Some constants we might need.
	const ScalarV v_one = ScalarV(V_ONE);
	const ScalarV v_fltsmall2 = ScalarV(V_FLT_SMALL_6);

	// Restore the position values that we overwrote earlier.
	m_triangleMeshTransform.SetCol3( oldTriangleTransformPos );
	m_triangleMeshLastTransform.SetCol3( oldTriangleLastTransformPos );
	m_convexTransform.SetCol3( oldConvexTransformPos );
	m_convexLastTransform.SetCol3( oldConvexLastTransformPos );

	if(pointCollector.GetHasResult())
	{
		Vec3V localPoint = pointCollector.GetPointOnBInLocal();

		Assert(minA->IsConvex());
		Assert(minB->GetType() == phBound::TRIANGLE);

		// Note that we are only using the margin from object B in this case (the concave one).  This is because the point we are testing is coming from
		//   the contact point for object B, which will be on (or near) the surface of the margin-expanded concave bound.
		const ScalarV vecMargin = minB->GetMarginV();
		const Vec3V vecMarginSq = Scale( Scale( VECTOR3_TO_VEC3V(kVecPointOnEdgeToleranceFactorSquared), vecMargin ), vecMargin );

		if(m_useNormalFiltering)
		{
			// NOTE: All of the vectors used here in this contact filtering are in the local space of the triangle bound.
			Vec3V curContactNormal = pointCollector.GetNormalOnBInWorld();
			curContactNormal = UnTransform3x3Ortho(transBatT, curContactNormal);

#if CONTACT_FILTERING_IMPL == 0
			// Reject collisions with the back-sides of the triangle.  It's probably debatable whether we actually want to do this or not, but it definitely
			//   seems to help keep small objects from falling through.  That's without CCD on though, so maybe that will improve that.  Also, I haven't checked
			//   how the math below works out in that case.
			// Note that this is only done here with CONTACT_FILTERING_IMPL == 0 because, in that case, the filtering code below doesn't work correctly with
			//   acute edges (which is the case in which it would be legitimate to have a backward facing normal) so we have to cull them out here and with
			//   CONTACT_FILTERING_IMPL == 1 we want to let them continue downward 
			if( IsLessThan( Dot( curContactNormal, faceNormal ), ScalarV(V_ZERO) ) != 0 )
			{
				tm.Release(false);
				return NULL;
			}
#endif
#if 0
			// This here is some code to verify that we didn't get a normal that's not close to the face normal of the triangle unless we're near an edge.
			const float kTestDot = curContactNormal.Dot(faceNormal);
			if(kTestDot < 0.990f)
			{
				// We're not near to the face normal.  Let's check if we're close to any edges.
				int edgeCount = 0;
				for(int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
				{
					bool isOnEdge = LocalIsPointOnSeg(localizedVertices[edgeIndex], localizedVertices[edgeIndex + 1], localPoint, vecMarginSq);
					edgeCount += isOnEdge ? 1 : 0;
				}
				// If our normal wasn't close to the face normal we had better have been close to one of the edges.  Last I checked, CCD seemed to have
				//   have a greater amount of error so we exclude that from this assert.  Maybe I should just have difference tolerances in that case.
				Assert(edgeCount > 0 || m_useContinuous);
			}
#endif
			// Loop through each of the edges and check this contact point against that edge.
			if (hasNeighbor & 1)
			{
				if (!CheckContactAgainstEdge(localizedVertices0, localizedVertices1, localPoint, vecMarginSq, neighborNormals0, edgeNormals0, curContactNormal))
				{
					tm.Release(false);
					return NULL;
				}
			}

			if (hasNeighbor & 2)
			{
				if (!CheckContactAgainstEdge(localizedVertices1, localizedVertices2, localPoint, vecMarginSq, neighborNormals1, edgeNormals1, curContactNormal))
				{
					tm.Release(false);
					return NULL;
				}
			}

			if (hasNeighbor & 4)
			{
				if (!CheckContactAgainstEdge(localizedVertices2, localizedVertices0, localPoint, vecMarginSq, neighborNormals2, edgeNormals2, curContactNormal))
				{
					tm.Release(false);
					return NULL;
				}
			}
		}

#if ENABLE_NORMAL_CLAMPING_SIMPLE
		if(g_UseNormalClamping)
		{
			// Loop over the edges that don't have neighbors and check to see if we're close to any of them.  Note that this loop will never check
			//   the same edge that we checked during filtering because there we only want edges that have neighbors.
			// We care about when we're a contact on an exposed edge because we don't want contacts with exposed edges to have normals pointing out
			//   into 'the abyss' (the area next to the exposed edge that doesn't have a triangle present).  When we find ourselves near an exposed
			//   edge we simply set our contact normal to the polygon normal, which is the correct result 99.999% of the time.  If our contact point
			//   is on a vertex shared by two exposed edges then it's possible that that's not quite the result we want but it's cheap and easy and
			//   good enough.
			if ((~hasNeighbor & 1) && LocalIsPointOnSeg(localizedVertices0.GetIntrin128(), localizedVertices1.GetIntrin128(), localPoint.GetIntrin128(), vecMarginSq.GetIntrin128()) ||
				(~hasNeighbor & 2) && LocalIsPointOnSeg(localizedVertices1.GetIntrin128(), localizedVertices2.GetIntrin128(), localPoint.GetIntrin128(), vecMarginSq.GetIntrin128()) ||
				(~hasNeighbor & 4) && LocalIsPointOnSeg(localizedVertices2.GetIntrin128(), localizedVertices0.GetIntrin128(), localPoint.GetIntrin128(), vecMarginSq.GetIntrin128()))
			{
				// The triangle has no neighboring triangle connected to this edge, so see if we're close to this edge.
				pointCollector.SetNormalOnBInWorld(Transform3x3(transBatT, faceNormal));
			}
		}
#endif

		// If we got here, we've accepted the new contact point, so let's add it for real.
		Vec3V distance = pointCollector.GetDistanceV();
		pointCollector.SetPointOnAInWorld(pointCollector.GetPointOnAInWorld() + finalOffset_WS);
		pointCollector.SetPointOnBInWorld(pointCollector.GetPointOnBInWorld() + finalOffset_WS);
		// Move triangle local space back
		pointCollector.SetPointOnBInLocal(pointCollector.GetPointOnBInLocal() + triangleOffset_LS);
		manifoldResult.ProcessResult(pointCollector);
	}

	tm.Release(false);

    int newestPoint = manifold.GetNewestContactPoint();
	phContact* point = newestPoint >= 0 ? &manifold.GetContactPoint(newestPoint) : NULL;
    return point;
}


phContact* btConvexTriangleCallback::ProcessSphere (Vec3V_In sphereCenter, const float sphereRadius, int partIndexShifted, phMaterialMgr::Id materialId, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult, phManifold& manifold)
{
	using namespace rage;

	int partIndex = partIndexShifted >> 1;

	phBoundSphere sphereBound(sphereRadius);
	sphereBound.SetCentroidOffset(sphereCenter);
	sphereBound.phBoundSphere::SetMaterial(materialId);
	sphereBound.SetIndexInBound(partIndex);

	manifold.SetBoundB(&sphereBound);
	manifold.ClearNewestContactPoint();

	// Note that we take a slightly different approach here right now than we do for capsules, in that we just position the sphere using the centroid
	//   offset since we don't have to worry about orientation.  In the future, for the sake of precision, we should be relocating the collisions
	//   so that they will always be occurring near the origin.
#if 0
	// IMPORTANT!  Make sure that you don't do anything that would skip over the code below that restores the matrices that we're changing
	//   here (such as by adding an early out between here and there).  You'll probably notice pretty quickly if you've done that as
	//   collision detection will get totally screwed up.
	// Save off the triangle transform positions that we're about to overwrite.
	const Vec3V oldTriangleTransformPos = m_triangleMeshTransform.GetCol3();
	const Vec3V oldTriangleLastTransformPos = m_triangleMeshLastTransform.GetCol3();

	m_triangleMeshTransform.SetCol3(Transform(m_triangleMeshTransform, sphereCenter));
	m_triangleMeshLastTransform.SetCol3(Transform(m_triangleMeshLastTransform, sphereCenter));
#endif

	m_intersector.SetPenetrationDepthSolverType(phConvexIntersector::PDSOLVERTYPE_MINKOWSKI);

	const phBound* minA = manifold.GetBoundA();
	const phBound* minB = manifold.GetBoundB();

	m_intersector.SetBoundA(minA);
	m_intersector.SetBoundB(minB);

	// Initialize the separating axis. The initial separating axis should have no effect on the output, but re-initialization makes the input consistent on replay
	// to avoid difficulties fixing replay bugs.
	//m_gjkPairDetector.InitSeparatingAxis(instanceA->GetMatrix(), instanceA->GetArchetype()->GetBound(), instanceB->GetMatrix(), instanceB->GetArchetype()->GetBound());
	// Should not be needed any more with the convex intersector.
	//m_gjkPairDetector.SetCachedSeperatingAxis(XAXIS);

	bool useContinuous = m_useContinuous;

	if (m_autoCCD)
	{
		useContinuous = true;

		// AutoCCD told us to use CCD, but don't do it if we already have a contact with this polygon
		if (g_AutoCCDFirstContactOnly)
		{
			for (int contactIndex = 0; contactIndex < manifold.GetNumContacts(); ++contactIndex)
			{
				if (manifold.GetContactPoint(contactIndex).GetElementB() == partIndex)
				{
					useContinuous = false;
					break;
				}
			}
		}
	}

	if (useContinuous)
	{
		ContinuousConvexCollision ccd(&m_intersector, manifold.GetManifoldMarginV().GetIntrin128());

		Mat34V offsetConvexLastTransform = m_convexLastTransform;

		ConvexCast::CastResult ccdResult;
		bool ccdHit = ccd.ComputeTimeOfImpact(offsetConvexLastTransform, m_convexTransform, m_triangleMeshLastTransform, m_triangleMeshTransform, ccdResult);

		if (ccdHit)
		{
			// The collision occurs on this frame.
			Vec3V worldA, worldB;
			worldA = Transform(ccdResult.m_hitTransformA, ccdResult.m_pointOnA);
			worldB = Transform(ccdResult.m_hitTransformB, ccdResult.m_pointOnB);

			Vec3V diff = worldB - worldA;
			Vec3V dist = Vec3V( Dot( ccdResult.m_normal, diff ) );

			DiscreteCollisionDetectorInterface::SimpleResult result;
			result.AddContactPoint(VEC3V_TO_INTRIN(ccdResult.m_normal), VEC3V_TO_INTRIN(worldA), VEC3V_TO_INTRIN(worldB), VEC3V_TO_INTRIN(-dist), ccdResult.m_elementA, ccdResult.m_elementB
				, ccdResult.m_pointOnA.GetIntrin128(), ccdResult.m_pointOnB.GetIntrin128()
				TRACK_COLLISION_TIME_PARAM(ccdResult.m_fraction.Getf()));
			manifoldResult.ProcessResult(result);
		}
	}
	else
	{
		// Get GJK to give us the collision info.
		GjkPairDetector::ClosestPointInput input;
		//input.m_maximumDistanceSquared = 1.0e30f;//minA->GetMargin() + minB->GetMargin() + manifold.GetManifoldMargin();
		//input.m_maximumDistanceSquared*= input.m_maximumDistanceSquared;
		input.m_offset.ZeroComponents();
		input.m_transformA = m_convexTransform;
		input.m_transformB = m_triangleMeshTransform;

		// Run GJK.
		m_intersector.GetClosestPoints(input, manifoldResult);
	}

#if 0
	m_triangleMeshTransform.SetCol3(oldTriangleTransformPos);
	m_triangleMeshLastTransform.SetCol3(oldTriangleLastTransformPos);
#endif

	sphereBound.Release(false);

	int newestPoint = manifold.GetNewestContactPoint();
	phContact* point = newestPoint >= 0 ? &manifold.GetContactPoint(newestPoint) : NULL;
	return point;
}


phContact* btConvexTriangleCallback::ProcessCapsule (Vec3V_In capsuleEnd0, Vec3V_In capsuleEnd1, Vec3V_In capsuleRadius, int partIndexShifted, phMaterialMgr::Id materialId, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult, phManifold& manifold)
{
	using namespace rage;

	Vec3V capsuleShaft(capsuleEnd1 - capsuleEnd0);
	ScalarV capsuleLength = MagFast(capsuleShaft);

	// Create a matrix to orient the capsule in the local space of the bound.
	Mat34V localCapsuleMatrix(V_IDENTITY);
	// Check if the shaft axis is parallel to the y-axis.
	const VecBoolV maskY = VecBoolV(V_F_T_F_F);
	const Vec3V result = SelectFT(maskY, capsuleShaft, Vec3V(V_ZERO));
	if(!IsZeroAll(result))
	{
		localCapsuleMatrix.SetCol0(Normalize(Cross(capsuleShaft, Vec3V(V_Y_AXIS_WONE))));
		localCapsuleMatrix.SetCol1(Normalize(capsuleShaft));
		localCapsuleMatrix.SetCol2(Cross(localCapsuleMatrix.GetCol0(), localCapsuleMatrix.GetCol1()));
	}
	localCapsuleMatrix.SetCol3(Average(capsuleEnd0, capsuleEnd1));

	// IMPORTANT!  Make sure that you don't do anything that would skip over the code below that restores the matrices that we're changing
	//   here (such as by adding an early out between here and there).  You'll probably notice pretty quickly if you've done that as
	//   collision detection will get totally screwed up.
	// Save off the triangle transform positions that we're about to overwrite.
	const Mat34V oldTriangleTransform = m_triangleMeshTransform;
	const Mat34V oldTriangleLastTransform = m_triangleMeshLastTransform;

	Transform(m_triangleMeshLastTransform, m_triangleMeshLastTransform, localCapsuleMatrix);
	Transform(m_triangleMeshTransform, m_triangleMeshTransform, localCapsuleMatrix);

	Vec3V otherBoundHalfWidth, otherBoundCenter;
	manifold.GetBoundA()->GetBoundingBoxHalfWidthAndCenter(otherBoundHalfWidth, otherBoundCenter);

	Mat34V lastAshiftedByDeltaB = m_convexLastTransform;
	UnTransformOrtho( lastAshiftedByDeltaB, oldTriangleLastTransform, lastAshiftedByDeltaB );
	Transform( lastAshiftedByDeltaB, oldTriangleTransform, lastAshiftedByDeltaB );
	geomBoxes::ExpandOBBFromMotion(m_convexTransform, lastAshiftedByDeltaB, otherBoundHalfWidth, otherBoundCenter);

	Vec3V otherBoundWorldCenter = Transform(m_convexTransform, otherBoundCenter);
	Mat34V otherBoundMatrix = m_convexTransform;
	otherBoundMatrix.SetCol3(otherBoundWorldCenter);

	Mat34V relativeMatrix;
	UnTransformOrtho(relativeMatrix, m_triangleMeshTransform, otherBoundMatrix);

	otherBoundHalfWidth.SetWZero();
	Vec3V boxHalfSize(capsuleRadius.AsScalarV(), capsuleLength, capsuleRadius.AsScalarV());
	boxHalfSize.SetWZero();

	if(!geomBoxes::TestBoxToBoxOBB(RCC_VECTOR3(otherBoundHalfWidth), RCC_VECTOR3(boxHalfSize), RCC_MATRIX34(relativeMatrix)))
	{
		m_triangleMeshTransform = oldTriangleTransform;
		m_triangleMeshLastTransform = oldTriangleLastTransform;

		return NULL;
	}

	int partIndex = partIndexShifted >> 1;

	phBoundCapsule capsuleBound;
	capsuleBound.SetCapsuleSize( capsuleRadius.AsScalarV(), capsuleLength );
	capsuleBound.phBoundCapsule::SetMaterial(materialId);
	capsuleBound.SetIndexInBound(partIndex);

	manifold.SetBoundB(&capsuleBound);
	manifold.ClearNewestContactPoint();

	m_intersector.SetPenetrationDepthSolverType(phConvexIntersector::PDSOLVERTYPE_MINKOWSKI);

	const phBound* minA = manifold.GetBoundA();
	const phBound* minB = manifold.GetBoundB();

	m_intersector.SetBoundA(minA);
	m_intersector.SetBoundB(minB);

	// Initialize the separating axis. The initial separating axis should have no effect on the output, but re-initialization makes the input consistent on replay
	// to avoid difficulties fixing replay bugs.
	//m_gjkPairDetector.InitSeparatingAxis(instanceA->GetMatrix(), instanceA->GetArchetype()->GetBound(), instanceB->GetMatrix(), instanceB->GetArchetype()->GetBound());
	// Should not be needed any more with the convex intersector.
	//m_gjkPairDetector.SetCachedSeperatingAxis(XAXIS);

	bool useContinuous = m_useContinuous;

	if (m_autoCCD)
	{
		useContinuous = true;

		// AutoCCD told us to use CCD, but don't do it if we already have a contact with this polygon
		if (g_AutoCCDFirstContactOnly)
		{
			for (int contactIndex = 0; contactIndex < manifold.GetNumContacts(); ++contactIndex)
			{
				if (manifold.GetContactPoint(contactIndex).GetElementB() == partIndex)
				{
					useContinuous = false;
					break;
				}
			}
		}
	}

	if (useContinuous)
	{
		ContinuousConvexCollision ccd(&m_intersector, manifold.GetManifoldMarginV().GetIntrin128());

		Mat34V offsetConvexLastTransform = m_convexLastTransform;

		ConvexCast::CastResult ccdResult;
		bool ccdHit = ccd.ComputeTimeOfImpact(offsetConvexLastTransform, m_convexTransform, m_triangleMeshLastTransform, m_triangleMeshTransform, ccdResult);

		if (ccdHit)
		{
			// The collision occurs on this frame.
			Vec3V worldA, worldB;
			worldA = Transform(ccdResult.m_hitTransformA, ccdResult.m_pointOnA);
			worldB = Transform(ccdResult.m_hitTransformB, ccdResult.m_pointOnB);

			Vec3V diff = worldB - worldA;
			Vec3V dist = Vec3V( Dot( ccdResult.m_normal, diff ) );

			DiscreteCollisionDetectorInterface::SimpleResult result;
			result.AddContactPoint(VEC3V_TO_INTRIN(ccdResult.m_normal), VEC3V_TO_INTRIN(worldA), VEC3V_TO_INTRIN(worldB), VEC3V_TO_INTRIN(-dist), ccdResult.m_elementA, ccdResult.m_elementB
				, ccdResult.m_pointOnA.GetIntrin128(), ccdResult.m_pointOnB.GetIntrin128()
				TRACK_COLLISION_TIME_PARAM(ccdResult.m_fraction.Getf()));
			manifoldResult.ProcessResult(result);
		}
	}
	else
	{
		// Get GJK to give us the collision info.
		GjkPairDetector::ClosestPointInput input;
		//input.m_maximumDistanceSquared = 1.0e30f;//minA->GetMargin() + minB->GetMargin() + manifold.GetManifoldMargin();
		//input.m_maximumDistanceSquared*= input.m_maximumDistanceSquared;
		input.m_offset.ZeroComponents();
		input.m_transformA = m_convexTransform;
		input.m_transformB = m_triangleMeshTransform;

		// Run GJK.
		m_intersector.GetClosestPoints(input, manifoldResult);
	}

	m_triangleMeshTransform = oldTriangleTransform;
	m_triangleMeshLastTransform = oldTriangleLastTransform;

	capsuleBound.Release(false);

	int newestPoint = manifold.GetNewestContactPoint();
	phContact* point = newestPoint >= 0 ? &manifold.GetContactPoint(newestPoint) : NULL;
	return point;
}


phContact* btConvexTriangleCallback::ProcessBox (Vec::V3Param128 boxVert0, Vec::V3Param128 boxVert1, Vec::V3Param128 boxVert2, Vec::V3Param128_After3Args boxVert3, int partIndexShifted, phMaterialMgr::Id materialId, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult, phManifold& manifold)
{
	using namespace rage;

	int partIndex = partIndexShifted >> 1;

	// Need to extract some data about the box from the vertices that are passed in.
	const Vec3V collisionOffset(boxVert0);
	const Vec3V vert1(Vec3V(boxVert1) - collisionOffset);
	const Vec3V vert2(Vec3V(boxVert2) - collisionOffset);
	const Vec3V vert3(Vec3V(boxVert3) - collisionOffset);

	const Vec3V vecHalf(V_HALF);
	const Vec3V boxX = Scale(vecHalf, vert1 + vert3 - vert2);
	const Vec3V boxY = Scale(vecHalf, vert3 - vert1 - vert2);
	const Vec3V boxZ = Scale(vecHalf, vert1 - vert2 - vert3);

	Vec3V boxSize(Mag(boxX), Mag(boxY), Mag(boxZ));
	Vec3V boxHalfSize(boxSize * ScalarV(V_HALF));

	Vec3V otherBoundHalfWidth, otherBoundCenter;
	manifold.GetBoundA()->GetBoundingBoxHalfWidthAndCenter(otherBoundHalfWidth, otherBoundCenter);

	Mat34V lastAshiftedByDeltaB = m_convexLastTransform;
	UnTransformOrtho( lastAshiftedByDeltaB, m_triangleMeshLastTransform, lastAshiftedByDeltaB );
	Transform( lastAshiftedByDeltaB, m_triangleMeshTransform, lastAshiftedByDeltaB );
	geomBoxes::ExpandOBBFromMotion(m_convexTransform, lastAshiftedByDeltaB, otherBoundHalfWidth, otherBoundCenter);

	Vec3V collisionOffsetInWorldSpace = Transform3x3(m_triangleMeshTransform, collisionOffset);

	Vec3V otherBoundWorldCenter = Transform(m_convexTransform, otherBoundCenter);
	Mat34V otherBoundMatrix = m_convexTransform;
	otherBoundMatrix.SetCol3(otherBoundWorldCenter - collisionOffsetInWorldSpace);

	// IMPORTANT!  Make sure that you don't do anything that would skip over the code below that restores the matrices that we're changing
	//   here (such as by adding an early out between here and there).  You'll probably notice pretty quickly if you've done that as
	//   collision detection will get totally screwed up.
	// Save off the triangle transform positions that we're about to overwrite.
	const Mat34V oldTriangleTransform = m_triangleMeshTransform;
	const Mat34V oldTriangleLastTransform = m_triangleMeshLastTransform;

	Mat34V localBoxMatrix;
	localBoxMatrix.SetCol0(Normalize(boxX));
	localBoxMatrix.SetCol1(Normalize(boxY));
	localBoxMatrix.SetCol2(Cross(localBoxMatrix.GetCol0(), localBoxMatrix.GetCol1()));
	const Vec3V vecQuarter(Scale(vecHalf, vecHalf));
	localBoxMatrix.SetCol3(Scale(vecQuarter, vert1 + vert2 + vert3));
	Transform(m_triangleMeshTransform, m_triangleMeshTransform, localBoxMatrix);

	Mat34V relativeMatrix;
	UnTransformOrtho(relativeMatrix, m_triangleMeshTransform, otherBoundMatrix);

	otherBoundHalfWidth.SetWZero();
	boxHalfSize.SetWZero();

	if(!geomBoxes::TestBoxToBoxOBBFaces(RCC_VECTOR3(otherBoundHalfWidth), RCC_VECTOR3(boxHalfSize), RCC_MATRIX34(relativeMatrix)))
	{
		m_triangleMeshTransform = oldTriangleTransform;

		return NULL;
	}

	phBoundBox boxBound(RC_VECTOR3(boxSize));
	boxBound.phBoundBox::SetMaterial(materialId);
	ScalarV margin = Min(ScalarV(V_HALF)*ScalarV(V_HALF)*ScalarV(V_HALF)*MinElement(boxSize), 	ScalarVFromF32(m_collisionMarginTriangle));

	boxBound.SetMargin(margin);
	boxBound.SetIndexInBound(partIndex);

	manifold.SetBoundB(&boxBound);
	manifold.ClearNewestContactPoint();

	Transform(m_triangleMeshLastTransform, m_triangleMeshLastTransform, localBoxMatrix);

	m_intersector.SetPenetrationDepthSolverType(phConvexIntersector::PDSOLVERTYPE_MINKOWSKI);

	const phBound* minA = manifold.GetBoundA();
	const phBound* minB = &boxBound;

	m_intersector.SetBoundA(minA);
	m_intersector.SetBoundB(minB);

	// Initialize the separating axis. The initial separating axis should have no effect on the output, but re-initialization makes the input consistent on replay
	// to avoid difficulties fixing replay bugs.
	//m_gjkPairDetector.InitSeparatingAxis(instanceA->GetMatrix(), instanceA->GetArchetype()->GetBound(), instanceB->GetMatrix(), instanceB->GetArchetype()->GetBound());
	// Should not be needed any more with the convex intersector.
	//m_gjkPairDetector.SetCachedSeperatingAxis(XAXIS);

	bool useContinuous = m_useContinuous;

	if (m_autoCCD)
	{
		useContinuous = true;

		// AutoCCD told us to use CCD, but don't do it if we already have a contact with this polygon
		if (g_AutoCCDFirstContactOnly)
		{
			for (int contactIndex = 0; contactIndex < manifold.GetNumContacts(); ++contactIndex)
			{
				if (manifold.GetContactPoint(contactIndex).GetElementB() == partIndex)
				{
					useContinuous = false;
					break;
				}
			}
		}
	}

	Mat34V shiftedConvex;
	shiftedConvex.SetCol0(m_convexTransform.GetCol0());
	shiftedConvex.SetCol1(m_convexTransform.GetCol1());
	shiftedConvex.SetCol2(m_convexTransform.GetCol2());
	shiftedConvex.SetCol3(m_convexTransform.GetCol3() - collisionOffsetInWorldSpace);

	if (useContinuous)
	{
		ContinuousConvexCollision ccd(&m_intersector, manifold.GetManifoldMarginV().GetIntrin128());

		Mat34V offsetConvexLastTransform;
		offsetConvexLastTransform.SetCol0(m_convexLastTransform.GetCol0());
		offsetConvexLastTransform.SetCol1(m_convexLastTransform.GetCol1());
		offsetConvexLastTransform.SetCol2(m_convexLastTransform.GetCol2());
		offsetConvexLastTransform.SetCol3(m_convexLastTransform.GetCol3() - collisionOffsetInWorldSpace);

		ConvexCast::CastResult ccdResult;
		bool ccdHit = ccd.ComputeTimeOfImpact(offsetConvexLastTransform, shiftedConvex, m_triangleMeshLastTransform, m_triangleMeshTransform, ccdResult);

		if (ccdHit)
		{
			// The collision occurs on this frame.
			Vec3V worldA, worldB;
			worldA = Transform(ccdResult.m_hitTransformA, ccdResult.m_pointOnA);
			worldB = Transform(ccdResult.m_hitTransformB, ccdResult.m_pointOnB);

			Vec3V diff = worldB - worldA;
			Vec3V dist = Vec3V( Dot( ccdResult.m_normal, diff ) );

			worldA = worldA + collisionOffsetInWorldSpace;
			worldB = worldB + collisionOffsetInWorldSpace;

			Translate(ccdResult.m_hitTransformA, ccdResult.m_hitTransformA, collisionOffsetInWorldSpace);
			Translate(ccdResult.m_hitTransformB, ccdResult.m_hitTransformB, collisionOffsetInWorldSpace);

			DiscreteCollisionDetectorInterface::SimpleResult result;
			result.AddContactPoint(VEC3V_TO_INTRIN(ccdResult.m_normal), VEC3V_TO_INTRIN(worldA), VEC3V_TO_INTRIN(worldB), VEC3V_TO_INTRIN(-dist), ccdResult.m_elementA, ccdResult.m_elementB
				, ccdResult.m_pointOnA.GetIntrin128(), ccdResult.m_pointOnB.GetIntrin128()
				TRACK_COLLISION_TIME_PARAM(ccdResult.m_fraction.Getf()));
			manifoldResult.ProcessResult(result);
		}
	}
	else
	{
		// Get GJK to give us the collision info.
		GjkPairDetector::ClosestPointInput input;
		//input.m_maximumDistanceSquared = 1.0e30f;//minA->GetMargin() + minB->GetMargin() + manifold.GetManifoldMargin();
		//input.m_maximumDistanceSquared*= input.m_maximumDistanceSquared;
		input.m_offset = collisionOffsetInWorldSpace;
		input.m_transformA = shiftedConvex;
		input.m_transformB = m_triangleMeshTransform;

		// Run GJK.
		m_intersector.GetClosestPoints(input, manifoldResult);
	}

	m_triangleMeshTransform = oldTriangleTransform;
	m_triangleMeshLastTransform = oldTriangleLastTransform;

	boxBound.Release(false);

	int newestPoint = manifold.GetNewestContactPoint();
	phContact* point = newestPoint >= 0 ? &manifold.GetContactPoint(newestPoint) : NULL;
	return point;
}
#endif

#if ENABLE_NORMAL_CLAMPING
// Expand and adjust cone to include a new normal.  Note that, this does not necessarily produce a *minimal* cone when given a non-trivial cone.
// To see how this is the case, consider the case where the endpoints of the normals form an equilateral triangle.
Vec::V3Return128 ExpandCone(Vec::V3Param128 coneCenterParam, float& coneDot, Vec::V3Param128 newNormal)
{
	using namespace rage;

	Vec3V v_coneCenter		= Vec3V(coneCenterParam);
	ScalarV v_coneDot	= ScalarVFromF32(coneDot);
	Vec3V v_newNormal		= Vec3V(newNormal);

	ScalarV v_dotBetween	= Dot( v_coneCenter, v_newNormal );

	// Check to see if the normal to add is already included in the cone.
    if ( IsLessThanAll(v_dotBetween, v_coneDot) != 0 )
    {
		// Nope, it's not, so let's figure out how to adjust the cone to fit it in.
		if( IsGreaterThanOrEqualAll(v_coneDot, ScalarV(Vec::V4VConstant<U32_ALMOST_ONE,U32_ALMOST_ONE,U32_ALMOST_ONE,U32_ALMOST_ONE>())) != 0 )
		{
			// We're starting out with the trivial cone (cone angle of zero), so let's do this the easy way.  The new cone axis is going to be halfway
			//   between the old cone axis and the new normal, and the cone angle is going to be half the angle between the old cone axis and the new
			//   normal.  This is actually the situation we have the vast majority of the time (about 98% of the time according to my quick estimate).
			v_coneCenter = Average( v_coneCenter, v_newNormal );
			// TODO: This could probably be a NormalizeFast() since we're already in the general vicinity of a unit vector.
			v_coneCenter = NormalizeSafe( v_coneCenter, Vec3V(V_Z_AXIS_WZERO), Vec3V(V_FLT_SMALL_6) );

			// Use the half-angle formula to find the cosine of half the angle from the cosine of the angle.  This also seems to give somewhat better
			//   precision than the 'long' way of doing things that gets used below.
			ScalarV v_half = ScalarV(V_HALF);
			v_coneDot = SqrtSafe( AddScaled(v_half, v_half, v_dotBetween) );

			StoreScalar32FromScalarV( coneDot, v_coneDot );
		}
		else
		{
			// Figure out the angular deviation of the new normal from the cone, and then adjusted the center by half that angular deviation and increasing
			//   the 'cone angle' by this same magnitude.
			ScalarV _negone = ScalarV(V_NEGONE);
			ScalarV _one = ScalarV(V_ONE);
			ScalarV safeAcosInput;
			safeAcosInput = Clamp( v_dotBetween, _negone, _one );
			ScalarV v_angleBetween = Arccos( safeAcosInput );
			safeAcosInput = Clamp( v_coneDot, _negone, _one );
			ScalarV v_newNormalAngle = Arccos( safeAcosInput );

			ScalarV v_newConeAngle = Average( v_newNormalAngle, v_angleBetween );
			ScalarV v_rotateBy = Scale( Subtract( v_newNormalAngle, v_angleBetween ), ScalarV(V_HALF) );

			Vec3V v_rotateAxis = Cross( v_newNormal, v_coneCenter );
			v_rotateAxis = Normalize( v_rotateAxis );

			QuatV rotation = QuatVFromAxisAngle( v_rotateAxis, v_rotateBy );
			v_coneCenter = Transform( rotation, v_coneCenter );

			v_coneDot = Cos( v_newConeAngle );

			StoreScalar32FromScalarV( coneDot, v_coneDot );
		}
    }
	return v_coneCenter.GetIntrin128();
}

// Ensure that a supplied normal is within the specified cone by clamping it the boundary of the cone if necessary.
Vec::V3Return128 ClampToCone(Vec::V3Param128 clampedNormal, Vec::V3Param128 coneCenter, const float& coneDot)
{
	sysTimer s;

	using namespace rage;

	Vec3V v_clampedNormal = Vec3V(clampedNormal);
	Vec3V v_coneCenter = Vec3V(coneCenter);
	ScalarV v_coneDot = ScalarVFromF32(coneDot);

	// TODO: If coneDot were passed in as a Vector3 then we could avoid needing to switch over the float pipeline until (and unless) we make it into
	//   the innermost if-statement below.  The LHS shouldn't cost us too much because we don't need the value right away.  Alternatively, we could
	//   consider passing in both a Vector3 and float version of coneDot if that's convenient.
	ScalarV v_dotBetween = Dot( v_clampedNormal, v_coneCenter );

	// Don't bother doing the calculations if we're just going to end up rotating by an infinitesimal amount anyway.
	if( IsLessThanAll( v_dotBetween, v_coneDot-ScalarV( Vec::V4VConstant<U32_FLT_SMALL_4,U32_FLT_SMALL_4,U32_FLT_SMALL_4,U32_FLT_SMALL_4>() ) ) != 0 )
    {
		if( IsGreaterThanOrEqualAll( v_coneDot, ScalarV( Vec::V4VConstant<U32_ALMOST_ONE,U32_ALMOST_ONE,U32_ALMOST_ONE,U32_ALMOST_ONE>() ) ) != 0 )
		{
			// We're just going to end up setting the clamped normal to the center of the cone anyway, let's just save ourselves some hassle.
			return v_coneCenter.GetIntrin128();
		}
		else
		{
			// New angle that the normal should have, relative to the cone center.
			ScalarV v_clampedConeDot = Clamp( v_coneDot, ScalarV(V_NEGONE), ScalarV(V_ONE) ); // Makes the arccos "safe".
			ScalarV newNormalAngle = Arccos(v_clampedConeDot);

			Vec3V rotateAxis = Cross( v_coneCenter, v_clampedNormal );
			rotateAxis = Normalize( rotateAxis );

			QuatV rotation = QuatVFromAxisAngle( rotateAxis, newNormalAngle );
			return Transform( rotation, v_coneCenter ).GetIntrin128();
		}
    }
	return clampedNormal;
}
#endif

// TODO: Left off here. Try refactoring this function. It's a big hit showing up on PIX.
#if 0
void btConvexConcaveCollisionAlgorithm::DetectCollision (const phCollisionInput& input, Vector3::Param UNUSED_PARAM(offsetA), phManifold& manifold, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult)
{
	using namespace rage;

	PF_START(ConcaveConvex);

	const phBound* min0 = input.boundA;
	const phBound* min1 = input.boundB;

	Assert(input.boundA->IsConvex() && input.boundB->IsConcave());
	float collisionMarginTriangle = input.boundB->GetMargin();

	btConvexTriangleCallback convexTriangleCallback(input.boundA);

	convexTriangleCallback.setMatrices(input.currentA, input.currentB);
	convexTriangleCallback.setLastMatrices(input.lastA, input.lastB);
	convexTriangleCallback.setTimeStepAndCounters(collisionMarginTriangle);
	convexTriangleCallback.useContinuous(input.useCCD, input.autoCCD);
	convexTriangleCallback.useNormalFiltering(g_UseNormalFiltering);

	Vec3V sphereCenter = min0->GetWorldCentroid(input.currentA);
	sphereCenter = UnTransformOrtho( input.currentB, sphereCenter );

	Vec3V halfWidth, boxCenter;
	min0->GetBoundingBoxHalfWidthAndCenter(halfWidth, boxCenter);

	// Move and expand the matrix for the composite bound part's bounding box to include both this and the previous frame.
	Mat34V lastAshiftedByDeltaB = input.lastA;
	UnTransformOrtho( lastAshiftedByDeltaB, input.lastB, lastAshiftedByDeltaB );
	Transform( lastAshiftedByDeltaB, input.currentB, lastAshiftedByDeltaB );
	geomBoxes::ExpandOBBFromMotion(MAT34V_ARG(input.currentA), MAT34V_ARG(lastAshiftedByDeltaB), halfWidth, boxCenter);

	boxCenter = Transform(input.currentA, boxCenter);
	boxCenter = UnTransformOrtho(input.currentB, boxCenter);

	Mat34V boxData;
	UnTransform3x3Ortho( boxData, input.currentB, input.currentA );
	boxData.SetCol3( halfWidth );

	//min1->ProcessAllTrianglesOBB(constructionInfo.component1,&convexTriangleCallback,sphereCenter,boxCenter,boxData,matrix1,manifoldResult);

	halfWidth = Add(halfWidth, Vec3V(min1->GetMarginV()));

	// Loop over the polygons and call the callback for each one
	phBoundCuller* culler = phSimulator::GetPerThreadBoundCuller();
	Assert(culler != NULL);

	int numCulled = culler->GetNumCulledPolygons();
	Vec3V vertices0;
	Vec3V vertices1;
	Vec3V vertices2;
	const phBoundBVH* culledGeom = static_cast<const phBoundGeomCullable*>(min1);

	Vec3V motionA;
	motionA = Subtract(input.currentA.GetCol3(), input.lastA.GetCol3());
	motionA = UnTransform3x3Ortho( input.currentB, motionA );

	// TODO: These margins probably shouldn't really be here.
	ScalarV motionTolerance = min0->GetMarginV() + min1->GetMarginV() + SplatX(VECTOR3_TO_VEC3V(phSimulator::GetMinimumConcaveThickness()));

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE		
	phBoundGeometrySecondSurfacePolygonCalculator secondSurfaceCalculator;	
	Vector3 avSecondSurfacePolyVertices[POLY_MAX_VERTICES];
	Vector3 vSecondSurfacePolyNormal;
	float fSecondSurfacePolyArea;
#endif

	for (int polyIndex = 0; polyIndex < numCulled; ++polyIndex)
	{
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
		// We want to pick up contacts at top surface and at interpolated surface
		for(int iPassIndex = 0; iPassIndex < 2; iPassIndex ++)
		{			
			//Printf("Starting pass %i\n",i);
			const float fInterp = iPassIndex==0 ? input.secondSurfaceInterpA : 0.0f;
			Assertf(fInterp>=0.0f && fInterp<=1.0f, "Interp value out of bounds");
			secondSurfaceCalculator.SetSecondSurfaceInterp(fInterp);
#endif
		int culledPolyIndex = culler->GetCulledPolygonIndex(polyIndex);
		const phPolygon& poly = culledGeom->GetPolygon(culledPolyIndex);
		const BvhPrimitive &bvhPrimitive = static_cast<const BvhPrimitive &>(poly);		

		switch(bvhPrimitive.GetType())
		{
		case PRIM_TYPE_POLYGON:
			{
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
				secondSurfaceCalculator.ComputeSecondSurfacePolyVertsAndNormal(*culledGeom,poly,avSecondSurfacePolyVertices,vSecondSurfacePolyNormal,fSecondSurfacePolyArea);
				const Vec3V topSurfacePolyUnitNormal=VECTOR3_TO_VEC3V(culledGeom->GetPolygonUnitNormal(culledPolyIndex));
				const Vec3V polyUnitNormal = RCC_VEC3V(vSecondSurfacePolyNormal);
#else
				const Vec3V polyUnitNormal = VECTOR3_TO_VEC3V(culledGeom->GetPolygonUnitNormal(culledPolyIndex));
#endif

#if ENABLE_NORMAL_CLAMPING
				float coneDot = 1.0f;
				Vec3V coneCenter = polyUnitNormal;
#endif
#if ENABLE_NORMAL_CLAMPING || CHECK_NO_INTERIOR_EDGE_COLLISIONS
				Vec3V margin = VECTOR3_TO_VEC3V(culledGeom->GetMarginV());
				Vec3V squaredMargin = Scale( Scale( margin, margin ), RCC_VEC3V(kVecPointOnEdgeToleranceFactorSquared) );
#endif
				Vec3V edgeNormals0;
				Vec3V edgeNormals1;
				Vec3V edgeNormals2;
				Vec3V neighborNormals0;
				Vec3V neighborNormals1;
				Vec3V neighborNormals2;
				Vec3V edgeVector;

				int vert0 = poly.GetVertexIndex(0);
				int vert1 = poly.GetVertexIndex(1);
				int vert2 = poly.GetVertexIndex(2);
	#if COMPRESSED_VERTEX_METHOD > 0
				// If we're going to be in the path when compressed vertices are enabled, we'd better have compressed
				//   vertices.
				Vec3V vertex0 = VECTOR3_TO_VEC3V(culledGeom->GetCompressedVertex(vert0));
				Vec3V vertex1 = VECTOR3_TO_VEC3V(culledGeom->GetCompressedVertex(vert1));
				Vec3V vertex2 = VECTOR3_TO_VEC3V(culledGeom->GetCompressedVertex(vert2));
	#else
				Vec3V vertex0 = VECTOR3_TO_VEC3V(culledGeom->GetVertex(vert0));
				Vec3V vertex1 = VECTOR3_TO_VEC3V(culledGeom->GetVertex(vert1));
				Vec3V vertex2 = VECTOR3_TO_VEC3V(culledGeom->GetVertex(vert2));
	#endif

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
				//Overwriting vertex0 etc because the code would become completely 
				//undreadable (and unmaintainable) with nested #defines in the above lines of code.
				//Obviously, this is slightly inefficient but can be easily remedied later if 
				//necessary.
				Vec3V avTopSurfacePolyVertices[3]={vertex0,vertex1,vertex2};
				vertex0 = VECTOR3_TO_VEC3V(avSecondSurfacePolyVertices[0]);
				vertex1 = VECTOR3_TO_VEC3V(avSecondSurfacePolyVertices[1]);
				vertex2 = VECTOR3_TO_VEC3V(avSecondSurfacePolyVertices[2]);
#endif


				vertices0 = vertex0;
				vertices1 = vertex1;
				vertices2 = vertex2;

				// Triangles get sent unmolested to the callback
				if (geomBoxes::TestPolygonToOrientedBoxFP(RCC_VECTOR3(vertex0),RCC_VECTOR3(vertex1),RCC_VECTOR3(vertex2),RCC_VECTOR3(polyUnitNormal),RCC_VECTOR3(boxCenter),RCC_MATRIX34(boxData),RCC_VECTOR3(halfWidth)))
				{
					// Set up the edge normals.
					int hasNeighbor = 0;
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
					hasNeighbor += ComputeEdgeNormal(	vertices0.GetIntrin128(),
									vertices1.GetIntrin128(),
									0,
									1,
									poly,
									polyUnitNormal.GetIntrin128(),
									culledGeom,
									secondSurfaceCalculator,
									edgeNormals0,
									neighborNormals0	);

					hasNeighbor += ComputeEdgeNormal(	vertices1.GetIntrin128(),
									vertices2.GetIntrin128(),
									1,
									2,
									poly,
									polyUnitNormal.GetIntrin128(),
									culledGeom,
									secondSurfaceCalculator,
									edgeNormals1,
									neighborNormals1	);

					hasNeighbor += ComputeEdgeNormal(	vertices2.GetIntrin128(),
									vertices0.GetIntrin128(),
									2,
									4,
									poly,
									polyUnitNormal.GetIntrin128(),
									culledGeom,
									secondSurfaceCalculator,
									edgeNormals2,
									neighborNormals2	);
#else // HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
					hasNeighbor += ComputeEdgeNormal(	vertices0.GetIntrin128(),
									vertices1.GetIntrin128(),
									0,
									1,
									poly,
									polyUnitNormal.GetIntrin128(),
									culledGeom,
									edgeNormals0,
									neighborNormals0	);

					hasNeighbor += ComputeEdgeNormal(	vertices1.GetIntrin128(),
									vertices2.GetIntrin128(),
									1,
									2,
									poly,
									polyUnitNormal.GetIntrin128(),
									culledGeom,
									edgeNormals1,
									neighborNormals1	);

					hasNeighbor += ComputeEdgeNormal(	vertices2.GetIntrin128(),
									vertices0.GetIntrin128(),
									2,
									4,
									poly,
									polyUnitNormal.GetIntrin128(),
									culledGeom,
									edgeNormals2,
									neighborNormals2	);
#endif // HACK_GTA4_BOUND_GEOM_SECOND_SURFACE

					phMaterialMgr::Id materialId = culledGeom->GetMaterialIdFromPartIndex(culledPolyIndex);
					if (phContact* contact = convexTriangleCallback.ProcessTriangle(vertices0, vertices1, vertices2, polyUnitNormal, edgeNormals0, edgeNormals1, edgeNormals2, neighborNormals0, neighborNormals1, neighborNormals2, culledPolyIndex << 1, materialId, hasNeighbor, manifoldResult, manifold))
					{
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE

						//Work out the distance from the contact point to the top surface

						if(0==fInterp)
						{
							contact->SetDistToTopSurface(ScalarV(V_ZERO));
						}
						else
						{
							//For unknown reasons contact.worldPosB ends up being 5mm above the contact plane so
							//just compute a new point that lies on the plane.
							//Work out the distance of contact.worldPosB to the contact plane.
							ScalarV distToBottomPlane=Dot(polyUnitNormal,contact->GetWorldPosB()-vertices0);
							//Work out a point on the contact plane by moving contact.worldPosB on to the contact plane.
							Vec3V worldPosB=Add(contact->GetWorldPosB(),Scale(polyUnitNormal,-distToBottomPlane));
							//The next bit is a bit ambiguous because it isn't clear which normal to use to work
							//out the distance from the contact point to the top surface.
							//We could use the interpolated surface normal, the top surface normal, 
							//or the displacement direction of the vertices.
							//Lets use the normal of the top surface to work out the depth.
							//Now work out the depth of the contact point.
							ScalarV secondSurfaceDepthB=-Dot(topSurfacePolyUnitNormal,worldPosB-avTopSurfacePolyVertices[0]);
							ScalarV zero(V_ZERO);
							BoolV cond=IsGreaterThan(secondSurfaceDepthB,zero);
							secondSurfaceDepthB=SelectFT(cond, zero, secondSurfaceDepthB);
							contact->SetDistToTopSurface(secondSurfaceDepthB);
						}
#endif


						Vec3V localPoint = contact->GetLocalPosB();

#if CHECK_NO_INTERIOR_EDGE_COLLISIONS
						{
							Vec3V localCollisionNormal = RCC_VEC3V(contact->GetWorldNormal());
							localCollisionNormal = UnTransform3x3Ortho( input.currentB, localCollisionNormal );
							const float kDot = Dot(localCollisionNormal, polyUnitNormal).Getf();
							// If this asserts is means 
							// This isn't strictly an error but generally it's not what we want to happen and it's probably indicating that the penetration 
							Assert(kDot >= -FLT_EPSILON);
							if(FPAbs(kDot) < 0.98f)
							{
								// We've got a normal that near to perpendicular to the face of the polygon.  Let's check its location.
								int edgesFound = 0;
								for(int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
								{
									const bool foundEdge = LocalIsPointOnSeg(vertices[edgeIndex].GetIntrin128(), vertices[(edgeIndex + 1) % 3].GetIntrin128(), localPoint.GetIntrin128(), squaredMargin.GetIntrin128());
									if(foundEdge)
									{
										++edgesFound;
									}
								}

								Assert(edgesFound > 0);
								Assert(edgesFound < 3);
#if ENABLE_NORMAL_CLAMPING
								if(edgesFound == 2)
								{
									Displayf("Hit a vertex!  Clamping may not be strict enough!");
								}
#endif
							}
						}
#endif

#if ENABLE_NORMAL_CLAMPING
						if (g_UseNormalClamping)
						{
							// Check each of the edges to see if the contact points lies on that edge.  If it does lie on that edge, expand the cone to
							//   include the normal of the neighboring polygon, if there is one.
							// NOTE: I think these shapes should really be the intersection of a set of wedges rather than cones.  Of course, in practice,
							//   this might be having the same effect, except perhaps when a contact point lies on a vertex.
							// NOTE: Further, I think there's a potentially faster and better way of doing this, but I don't feel like writing the explanation
							//   right now (I just discussed it with Eugene though).
							phPolygon::Index neighborIndex = poly.GetNeighboringPolyNum(0);
							if (neighborIndex != (phPolygon::Index)(-1))
							{
								if (LocalIsPointOnSeg(vertex0.GetIntrin128(), vertex1.GetIntrin128(), localPoint.GetIntrin128(), squaredMargin.GetIntrin128()))
								{
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
									const phPolygon& neighbourPolygon=culledGeom->GetPolygon(neighborIndex);
									Vector3 avNeighbourPolygonSecondSurfaceVertices[POLY_MAX_VERTICES];
									Vector3 vNeighbourPolygonSecondSurfaceNormal;
									float fNeighbourPolySecondSurfaceArea;
									secondSurfaceCalculator.ComputeSecondSurfacePolyVertsAndNormal(*culledGeom,neighbourPolygon,avNeighbourPolygonSecondSurfaceVertices,vNeighbourPolygonSecondSurfaceNormal,fNeighbourPolySecondSurfaceArea);
									coneCenter.SetIntrin128( ExpandCone(coneCenter.GetIntrin128(), coneDot, VECTOR3_TO_INTRIN(vNeighbourPolygonSecondSurfaceNormal)));
#else
									coneCenter.SetIntrin128( ExpandCone(coneCenter.GetIntrin128(), coneDot, VECTOR3_TO_INTRIN(culledGeom->GetPolygonUnitNormal(neighborIndex))) );
#endif
								}
							}

							neighborIndex = poly.GetNeighboringPolyNum(1);

							if (neighborIndex != (phPolygon::Index)(-1))
							{
								if (LocalIsPointOnSeg(vertex1.GetIntrin128(), vertex2.GetIntrin128(), localPoint.GetIntrin128(), squaredMargin.GetIntrin128()))
								{
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
									const phPolygon& neighbourPolygon=culledGeom->GetPolygon(neighborIndex);
									Vector3 avNeighbourPolygonSecondSurfaceVertices[POLY_MAX_VERTICES];
									Vector3 vNeighbourPolygonSecondSurfaceNormal;
									float fNeighbourPolySecondSurfaceArea;
									secondSurfaceCalculator.ComputeSecondSurfacePolyVertsAndNormal(*culledGeom,neighbourPolygon,avNeighbourPolygonSecondSurfaceVertices,vNeighbourPolygonSecondSurfaceNormal,fNeighbourPolySecondSurfaceArea);
									coneCenter.SetIntrin128( ExpandCone(coneCenter.GetIntrin128(), coneDot, VECTOR3_TO_INTRIN(vNeighbourPolygonSecondSurfaceNormal)));
#else
									coneCenter.SetIntrin128( ExpandCone(coneCenter.GetIntrin128(), coneDot, VECTOR3_TO_INTRIN(culledGeom->GetPolygonUnitNormal(neighborIndex))) );
#endif
								}
							}

							neighborIndex = poly.GetNeighboringPolyNum(2);
							if (neighborIndex != (phPolygon::Index)(-1))
							{
								if (LocalIsPointOnSeg(vertex2.GetIntrin128(), vertex0.GetIntrin128(), localPoint.GetIntrin128(), squaredMargin.GetIntrin128()))
								{
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
									const phPolygon& neighbourPolygon=culledGeom->GetPolygon(neighborIndex);
									Vector3 avNeighbourPolygonSecondSurfaceVertices[POLY_MAX_VERTICES];
									Vector3 vNeighbourPolygonSecondSurfaceNormal;
									float fNeighbourPolySecondSurfaceArea;
									secondSurfaceCalculator.ComputeSecondSurfacePolyVertsAndNormal(*culledGeom,neighbourPolygon,avNeighbourPolygonSecondSurfaceVertices,vNeighbourPolygonSecondSurfaceNormal,fNeighbourPolySecondSurfaceArea);
									coneCenter.SetIntrin128( ExpandCone(coneCenter.GetIntrin128(), coneDot, VECTOR3_TO_INTRIN(vNeighbourPolygonSecondSurfaceNormal)));
#else
									coneCenter.SetIntrin128( ExpandCone(coneCenter.GetIntrin128(), coneDot, VECTOR3_TO_INTRIN(culledGeom->GetPolygonUnitNormal(neighborIndex))) );
#endif
								}
							}

							Vec3V clampedNormal = contact->GetWorldNormal();
							clampedNormal = UnTransform3x3Ortho( input.currentB, clampedNormal );
							clampedNormal.SetIntrin128( ClampToCone(clampedNormal.GetIntrin128(), coneCenter.GetIntrin128(), coneDot) );
#if ADJUST_DEPTH_POST_NORMAL_CLAMP
							// Reduce the depth to go up to the polygon's plane.
							contact->SetDepth(contact->GetDepth() * fabs(clampedNormal.Dot(contact->GetWorldNormal())));
#endif
							contact->SetLocalNormals(clampedNormal);
						}
#endif	// ENABLE_NORMAL_CLAMPING
					}
				}

				break;
			}
		case PRIM_TYPE_SPHERE:
			{
				const BvhPrimSphere &spherePrim = reinterpret_cast<const BvhPrimSphere &>(bvhPrimitive);
				phMaterialMgr::Id materialId = culledGeom->GetMaterialIdFromPartIndex(culledPolyIndex);
				convexTriangleCallback.ProcessSphere(VECTOR3_TO_VEC3V(culledGeom->GetVertex(spherePrim.GetCenterIndex())), spherePrim.GetRadius(), culledPolyIndex << 1, materialId, manifoldResult, manifold);
				break;
			}
		case PRIM_TYPE_CAPSULE:
			{
				const BvhPrimCapsule &capsulePrim = reinterpret_cast<const BvhPrimCapsule &>(bvhPrimitive);
				const Vec3V vertex0 = VECTOR3_TO_VEC3V(culledGeom->GetVertex(capsulePrim.GetEndIndex0()));
				const Vec3V vertex1 = VECTOR3_TO_VEC3V(culledGeom->GetVertex(capsulePrim.GetEndIndex1()));
				const Vec3V vecRadius = Vec3VFromF32(capsulePrim.GetRadius());

				phMaterialMgr::Id materialId = culledGeom->GetMaterialIdFromPartIndex(culledPolyIndex);
				convexTriangleCallback.ProcessCapsule(vertex0, vertex1, vecRadius, culledPolyIndex << 1, materialId, manifoldResult, manifold);
				break;
			}
		default:
			{
				Assert(bvhPrimitive.GetType() == PRIM_TYPE_BOX);
				const BvhPrimBox &boxPrim = reinterpret_cast<const BvhPrimBox &>(bvhPrimitive);
				phMaterialMgr::Id materialId = culledGeom->GetMaterialIdFromPartIndex(culledPolyIndex);
				convexTriangleCallback.ProcessBox(culledGeom->GetVertex(boxPrim.GetVertexIndex(0)), culledGeom->GetVertex(boxPrim.GetVertexIndex(1)), culledGeom->GetVertex(boxPrim.GetVertexIndex(2)), culledGeom->GetVertex(boxPrim.GetVertexIndex(3)), culledPolyIndex << 1, materialId, manifoldResult, manifold);
				break;
			}
		}
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
		}	// End of double pass
#endif
	}

	PF_STOP(ConcaveConvex);
}
#endif

#if 0
float btConvexConcaveCollisionAlgorithm::calculateTimeOfImpact(phInst* ,phInst* ,const btDispatcherInfo& )
{
	//quick approximation using raycast, todo: hook up to the continuous collision detection (one of the btConvexCast)
	btCollisionObject* convexbody = (btCollisionObject* )m_convex.m_clientObject;
	btCollisionObject* triBody = static_cast<btCollisionObject* >(m_concave.m_clientObject);

	//only perform CCD above a certain treshold, this prevents blocking on the long run
	//because object in a blocked ccd state (hitfraction<1) get their linear velocity halved each frame...
	float squareMot0 = (convexbody->m_interpolationWorldTransform.getOrigin() - convexbody->m_worldTransform.getOrigin()).length2();
	if (squareMot0 < convexbody->m_ccdSquareMotionTreshold)
	{
		return 1.f;
	}

	//const btVector3& from = convexbody->m_worldTransform.getOrigin();
	//btVector3 to = convexbody->m_interpolationWorldTransform.getOrigin();
	//todo: only do if the motion exceeds the 'radius'

	btTransform worldToLocalTrimesh = triBody->m_worldTransform.inverse();
	btTransform convexFromLocal = worldToLocalTrimesh * convexbody->m_worldTransform;
	btTransform convexToLocal = worldToLocalTrimesh * convexbody->m_interpolationWorldTransform;

	struct LocalTriangleSphereCastCallback	: public btTriangleCallback
	{
		btTransform m_ccdSphereFromTrans;
		btTransform m_ccdSphereToTrans;
		btTransform	m_meshTransform;

		float	m_ccdSphereRadius;
		float	m_hitFraction;
	

		LocalTriangleSphereCastCallback(const btTransform& from,const btTransform& to,float ccdSphereRadius,float hitFraction)
			:m_ccdSphereFromTrans(from),
			m_ccdSphereToTrans(to),
			m_ccdSphereRadius(ccdSphereRadius),
			m_hitFraction(hitFraction)
		{			
		}
		
		
		virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex)
		{
			//do a swept sphere for now
			btTransform ident;
			ident.setIdentity();
			btConvexCast::CastResult castResult;
			castResult.m_fraction = m_hitFraction;
			btSphereShape	pointShape(m_ccdSphereRadius);
			btTriangleShape	triShape(triangle[0],triangle[1],triangle[2]);
			btVoronoiSimplexSolver	simplexSolver;
			btSubsimplexConvexCast convexCaster(&pointShape,&triShape,&simplexSolver);
			//GjkConvexCast	convexCaster(&pointShape,convexShape,&simplexSolver);
			//ContinuousConvexCollision convexCaster(&pointShape,convexShape,&simplexSolver,0);
			//local space?

			if (convexCaster.ComputeTimeOfImpact(m_ccdSphereFromTrans,m_ccdSphereToTrans,ident,ident,castResult))
			{
				if (m_hitFraction > castResult.m_fraction)
				{
					m_hitFraction = castResult.m_fraction;
				}
			}

		}

	};


	

	
	if (triBody->m_collisionShape->isConcave())
	{
		btVector3 rayAabbMin = convexFromLocal.getOrigin();
		rayAabbMin.setMin(convexToLocal.getOrigin());
		btVector3 rayAabbMax = convexFromLocal.getOrigin();
		rayAabbMax.setMax(convexToLocal.getOrigin());
		rayAabbMin -= btVector3(convexbody->m_ccdSweptShereRadius,convexbody->m_ccdSweptShereRadius,convexbody->m_ccdSweptShereRadius);
		rayAabbMax += btVector3(convexbody->m_ccdSweptShereRadius,convexbody->m_ccdSweptShereRadius,convexbody->m_ccdSweptShereRadius);

		float curHitFraction = 1.f; //is this available?
		LocalTriangleSphereCastCallback raycastCallback(convexFromLocal,convexToLocal,
		convexbody->m_ccdSweptShereRadius,curHitFraction);

		raycastCallback.m_hitFraction = convexbody->m_hitFraction;

		btCollisionObject* concavebody = (btCollisionObject* )m_concave.m_clientObject;

		ConcaveShape* triangleMesh = (ConcaveShape*) concavebody->m_collisionShape;
		
		if (triangleMesh)
		{
			triangleMesh->processAllTriangles(&raycastCallback,rayAabbMin,rayAabbMax);
		}
	


		if (raycastCallback.m_hitFraction < convexbody->m_hitFraction)
		{
			convexbody->m_hitFraction = raycastCallback.m_hitFraction;
			return raycastCallback.m_hitFraction;
		}
	}

	return 1.f;


    return 1.0f;
}
#endif

} // namespace rage

#endif // ENABLE_UNUSED_PHYSICS_CODE
