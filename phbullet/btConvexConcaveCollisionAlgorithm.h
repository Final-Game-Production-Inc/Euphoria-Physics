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

#ifndef CONVEX_CONCAVE_COLLISION_ALGORITHM_H
#define CONVEX_CONCAVE_COLLISION_ALGORITHM_H

#include "phcore/constants.h"
#if ENABLE_UNUSED_PHYSICS_CODE

#include "btCollisionAlgorithm.h"
//#include "GjkPairDetector.h"
#include "ConvexIntersector.h"
#include "TriangleCallback.h"
//#include "VoronoiSimplexSolver.h"

#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#include "phbound/boundbvh.h"
#include "phCore/constants.h"
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
#include "phbound/boundGeomSecondSurface.h"
#endif

struct btDispatcherInfo;

namespace rage {

class phContact;
class phInst;

///For each triangle in the concave mesh that overlaps with the AABB of a convex (m_convexProxy), processTriangle is called.
class btConvexTriangleCallback
{
	Mat34V	m_triangleMeshTransform;
	Mat34V	m_triangleMeshLastTransform;
	Mat34V	m_convexTransform;
	Mat34V	m_convexLastTransform;

    float m_collisionMarginTriangle;
    const phBound*	m_convexShape;
    bool m_useNormalFiltering;
    bool m_useContinuous;
	bool m_autoCCD;

//	VoronoiSimplexSolver m_simplexSolver;
//    GjkPairDetector m_gjkPairDetector;
	phConvexIntersector m_intersector;

public:

    btConvexTriangleCallback(const phBound* convexShape);

	__forceinline
		void    setMatrices(Mat34V_In convexTransform, Mat34V_In triangleTransform)
	{
		m_convexTransform = convexTransform;
		m_triangleMeshTransform = triangleTransform;
	}

	__forceinline
    void    setLastMatrices(Mat34V_In convexLastTransform, Mat34V_In triangleLastTransform)
	{
		m_convexLastTransform = convexLastTransform;
		m_triangleMeshLastTransform = triangleLastTransform;
	}

	__forceinline
	void	setTimeStepAndCounters(float collisionMarginTriangle)
	{
		m_collisionMarginTriangle = collisionMarginTriangle;	
	}

	__forceinline
    void    useNormalFiltering(bool filter = true)
	{
		m_useNormalFiltering = filter;
	}

	__forceinline
    void    useContinuous(bool continuous, bool autoCCD)
	{
		m_useContinuous = continuous;
		m_autoCCD = autoCCD;
	}

	~btConvexTriangleCallback();
#if 0
	phContact* ProcessTriangle (Vec3V_In vertices0,
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
								phManifold& manifold);

	phContact* ProcessSphere (Vec3V_In sphereCenter, const float sphereRadius, int partIndexShifted, phMaterialMgr::Id materialId, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult, phManifold& manifold);
	phContact* ProcessCapsule (Vec3V_In capsuleEnd0, Vec3V_In capsuleEnd1, Vec3V_In capsuleRadius, int partIndexShifted, phMaterialMgr::Id materialId, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult, phManifold& manifold);
	phContact* ProcessBox (Vec::V3Param128 boxVert0, Vec::V3Param128 boxVert1, Vec::V3Param128 boxVert2, Vec::V3Param128_After3Args boxVert3, int partIndexShifted, phMaterialMgr::Id materialId, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult, phManifold& manifold);
#endif
	void clearCache();

};




/// btConvexConcaveCollisionAlgorithm  supports collision between convex shapes and (concave) trianges meshes.
class btConvexConcaveCollisionAlgorithm  : public btCollisionAlgorithm
{
public:

	btConvexConcaveCollisionAlgorithm();

	virtual ~btConvexConcaveCollisionAlgorithm();

//    virtual void DetectCollision (const phCollisionInput& input, Vector3::Param offsetA, phManifold& manifold, DiscreteCollisionDetectorInterface::ResultProcessor& manifoldResult);

	float	calculateTimeOfImpact(phInst* proxy0,phInst* proxy1,const btDispatcherInfo& dispatchInfo);

	void	clearCache();

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
	static int ComputeEdgeNormal (	Vec::V3Param128 thisVert, Vec::V3Param128 nextVert, int edgeIndex, int neighborMask, const phPolygon& polygon,
		Vec::V3Param128 polyUnitNormal, const phBoundGeometry* culledGeom, const phBoundGeometrySecondSurfacePolygonCalculator& secondSurfaceCalculator, Vec3V_InOut edgeNormal, Vec3V_InOut neighborNormal);
#else
	static int ComputeEdgeNormal (	Vec::V3Param128 thisVert, Vec::V3Param128 nextVert, int edgeIndex, int neighborMask, const phPolygon& polygon,
									Vec::V3Param128 polyUnitNormal, const phBoundGeometry* culledGeom, Vec3V_InOut edgeNormal, Vec3V_InOut neighborNormal);
#endif
};



#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
inline int btConvexConcaveCollisionAlgorithm::ComputeEdgeNormal (Vec::V3Param128 thisVert, Vec::V3Param128 nextVert, int edgeIndex, int neighborMask, const phPolygon& polygon,
																 Vec::V3Param128 polyUnitNormal, const phBoundGeometry* culledGeom, const phBoundGeometrySecondSurfacePolygonCalculator& secondSurfaceCalculator, Vec3V_InOut edgeNormal, Vec3V_InOut neighborNormal)
#else
inline int btConvexConcaveCollisionAlgorithm::ComputeEdgeNormal (Vec::V3Param128 thisVert, Vec::V3Param128 nextVert, int edgeIndex, int neighborMask, const phPolygon& polygon,
																 Vec::V3Param128 polyUnitNormal, const phBoundGeometry* culledGeom, Vec3V_InOut edgeNormal, Vec3V_InOut neighborNormal)
#endif
{
	using namespace rage;

	// Build up outputs locally before saving to them.
	Vec3V v_edgeNormal;
	Vec3V v_neighborNormal;

	// Save locals.
	Vec3V v_polyUnitNormal(polyUnitNormal);

	const phPolygon::Index neighborIndex = polygon.GetNeighboringPolyNum(edgeIndex);
	int hasNeighbor = 0;
	if (neighborIndex != (phPolygon::Index)(-1))
	{
		Vec3V edgeVector = Vec3V(nextVert) - Vec3V(thisVert);

		// Make the edge normals the average of the two neighboring polygon normals.
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
		if(culledGeom->GetHasSecondSurface())
		{
			const phPolygon& neighbourPolygon=culledGeom->GetPolygon(neighborIndex);
			Vector3 avSecondSurfaceNeighbourPolygonVertices[POLY_MAX_VERTICES];
			Vector3 vSecondSurfaceNeighbourPolygonNormal;
			float fSecondsurfaceNeighbourPolyArea;
			secondSurfaceCalculator.ComputeSecondSurfacePolyVertsAndNormal
				(*culledGeom,neighbourPolygon,
				 avSecondSurfaceNeighbourPolygonVertices,vSecondSurfaceNeighbourPolygonNormal,fSecondsurfaceNeighbourPolyArea); 			
			v_edgeNormal = v_polyUnitNormal;
			v_neighborNormal = VECTOR3_TO_VEC3V(vSecondSurfaceNeighbourPolygonNormal);
		}
		else
		{
			v_edgeNormal = v_polyUnitNormal;
			v_neighborNormal = VECTOR3_TO_VEC3V(culledGeom->GetPolygonUnitNormal(neighborIndex));
		}
#else
		v_edgeNormal = v_polyUnitNormal;
		v_neighborNormal = culledGeom->GetPolygonUnitNormal(neighborIndex);
#endif
		Vec3V normCrossEdge = Cross(v_neighborNormal, edgeVector);
		if( IsGreaterThanAll( Dot(normCrossEdge, v_polyUnitNormal), ScalarV(V_ZERO) ) != 0 )
		{
			// This is a convex edge, so make the edge normal the average of the two polygon normals.  The neighbor normal is already set appropriately.
			v_edgeNormal += v_neighborNormal;
			v_edgeNormal = Normalize(v_edgeNormal);
		}
		else
		{
			// For concave edges, the edge normal is the polygon normal, and the neighborNormal is the polygon normal (we could really go ahead
			//   and leave this as the actual neighbor normal since the contact filtering does some clamping).
			v_neighborNormal = v_polyUnitNormal;
		}

		// To make the edge normal equal the polygon normal, use this line instead of all of the above inside the neighbor-index-if brackets.
		//edgeNormal.Set(culledGeom->GetPolygonUnitNormal(neighborIndex));

		// Set the bit to tell that this polygon has a neighbor along this edge.
		hasNeighbor = neighborMask;
	}
	else
	{
		// There is no neighboring polygon on this side, so the edge and neighbor normals will be the polygon normal.
		// To make the edge normal point outward in the polygon's plane, use the next two lines instead of the one line following.
		//edgeNormal.Cross(edgeVector,polyUnitNormal);
		//edgeNormal.Normalize();
		v_edgeNormal = v_neighborNormal = v_polyUnitNormal;
	}

	edgeNormal = v_edgeNormal;
	neighborNormal = v_neighborNormal;
	return hasNeighbor;
}

} // namespace rage

#endif // ENABLE_UNUSED_PHYSICS_CODE

#endif //CONVEX_CONCAVE_COLLISION_ALGORITHM_H
