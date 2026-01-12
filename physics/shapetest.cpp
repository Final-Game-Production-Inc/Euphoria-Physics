//
// physics/shapetest.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//


#include "shapetest.h"

#include "manifoldresult.h"
#include "shapetestspu.h"
#include "overlappingpairarray.h"

#include "math/simplemath.h"

#include "phbound/boundbvh.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundcurvedgeom.h"
#include "phbound/boundcylinder.h"
#include "phbound/boundgeom.h"
#include "phbound/boundsphere.h"
#include "phbound/cullerhelpers.h"
#include "phbound/OptimizedBvh.h"
#include "phbound/support.h"
#include "phbullet/IterativeCast.h"
#include "phbullet/MinkowskiPenetrationDepthSolver.h"
#include "phcore/phmath.h"
#include "CollisionMemory.h"

#if !__SPU
#include "diag/output.h"
#include "phbullet/TriangleShape.h"
#include "phbullet/btConvexConcaveCollisionAlgorithm.h"
#include "phcore/materialmgrflag.h"
#include "physics/collision.h"
#include "physics/simulator.h"
#else
#include "phbullet/CollisionWorkUnit.h"
#include "physics/collision.h"
#endif // !__SPU


#if ENABLE_PHYSICS_LOCK
#include "system/criticalsection.h"
#endif

#include "system/task.h"
#include "system/xtl.h"

#define				FAST_SHAPETEST			1

// PURPOSE: If this is enabled, all shapetests will assert if the result isn't reasonable
#define LOW_MEMORY_BUILDS (__DEV && __XENON)
#define VALIDATE_SHAPETEST_RESULTS (__ASSERT && !__SPU && !LOW_MEMORY_BUILDS)

#if VALIDATE_SHAPETEST_RESULTS
#define TOLERATED_DISTANCE_FROM_BOUND		(0.03f)	// Maximum distance an intersection can be from a bound
#define TOLERATED_DISTANCE_FROM_SHAPE		(0.02f)	// Maximum distance an intersection can be from a shape
#define TOLERATED_PENETRATION_SOLVER_ERROR  (10.0f) // Depth solving with generic shapes isn't the most accurate 
													// NOTE: This value is a bit hacky, the tolerated distance should be based on the
													//       size of the bound but we don't want to reconstruct primitives just for that.
#define TOLERATED_SWEPT_SPHERE_INITIAL_SPHERE_ERROR (100.0f) // T=0 intersections of swept sphere don't keep their penetration depth so we have no way of computing the point on the shape
#define TOLERATED_DISTANCE_FROM_SHAPE_ITERATIVE_BOUND_CAST (0.1f)	// Bound vs. Bound casts have numerical precision issues with large bounds. They will always return an intersection if there is one, it just might be
																	//  with a T value earlier than expected

#define TOLERATED_NORMAL_ERROR              (0.03f) // Maximum difference between normal length and the expected length of 1
#define MAXIMUM_INVALID_RESULTS				(10)	// Maximum number of invalid shapetests, after this limit is reached there won't be any more validation (set to -1 for unlimited validation)
#define INPUT_PRECISION						(10)	// Decimal precision of data required for reproducing shapetests (probe segment, triangle vertices, ...)
#define DEFAULT_PRECISION					(3)		// Decimal precision of data that is important, but doesn't require great accuracy (intersection depth, intersection position, ...)
#endif // VALIDATE_SHAPETEST_RESULTS

DECLARE_TASK_INTERFACE(shapetestcapsulespu);
DECLARE_TASK_INTERFACE(shapetestspherespu);
DECLARE_TASK_INTERFACE(shapetestsweptspherespu);
DECLARE_TASK_INTERFACE(shapetesttaperedsweptspherespu);

namespace rage {

EXT_PFD_DECLARE_GROUP(ShapeTests);
EXT_PFD_DECLARE_ITEM(PhysicsLevelCullShape);
EXT_PFD_DECLARE_ITEM(ShapeTestBoundCull);

EXT_PFD_DECLARE_ITEM(ProbeSegments);
EXT_PFD_DECLARE_ITEM(ProbeIsects);
EXT_PFD_DECLARE_ITEM(ProbeNormals);
EXT_PFD_DECLARE_ITEM(EdgeSegments);
EXT_PFD_DECLARE_ITEM(EdgeIsects);
EXT_PFD_DECLARE_ITEM(EdgeNormals);
EXT_PFD_DECLARE_ITEM(SphereSegments);
EXT_PFD_DECLARE_ITEM(SphereIsects);
EXT_PFD_DECLARE_ITEM(SphereNormals);
EXT_PFD_DECLARE_ITEM(CapsuleSegments);
EXT_PFD_DECLARE_ITEM(CapsuleIsects);
EXT_PFD_DECLARE_ITEM(CapsuleNormals);
EXT_PFD_DECLARE_ITEM(SweptSphereSegments);
EXT_PFD_DECLARE_ITEM(SweptSphereIsects);
EXT_PFD_DECLARE_ITEM(SweptSphereNormals);
EXT_PFD_DECLARE_ITEM(TaperedSweptSphereSegments);
EXT_PFD_DECLARE_ITEM(TaperedSweptSphereIsects);
EXT_PFD_DECLARE_ITEM(TaperedSweptSphereNormals);
EXT_PFD_DECLARE_ITEM(ScalingSweptQuadSegments);
EXT_PFD_DECLARE_ITEM(ScalingSweptQuadIsects);
EXT_PFD_DECLARE_ITEM(ScalingSweptQuadNormals);
EXT_PFD_DECLARE_ITEM(ObjectSegments);
EXT_PFD_DECLARE_ITEM(ObjectIsects);
EXT_PFD_DECLARE_ITEM(ObjectNormals);
EXT_PFD_DECLARE_ITEM(ObjectManifolds);
EXT_PFD_DECLARE_ITEM(BoxSegments);
EXT_PFD_DECLARE_ITEM(BoxIsects);
EXT_PFD_DECLARE_ITEM(BoxNormals);
	
EXT_PFD_DECLARE_ITEM(Solid);
EXT_PFD_DECLARE_ITEM(DrawBoundMaterials);
EXT_PFD_DECLARE_ITEM(SupportPoints);
EXT_PFD_DECLARE_ITEM(CullSolid);
EXT_PFD_DECLARE_ITEM_SLIDER_INT(CullOpacity);

#define ALLOCATE_LOCALIZED_DATA(shapeType,shape) *(typename shapeType::LocalizedData*)Alloca(u8,shape.GetLocalizedDataSize())

#define SHAPE_TEST_TIMERS	0
// Shape test timers are off by default because they need to be started and stopped many times.
#if SHAPE_TEST_TIMERS
PF_PAGE(ShapeTestPage,"Shape Tests");
PF_GROUP(ShapeTests);
PF_LINK(ShapeTestPage,ShapeTests);
PF_TIMER(ProbeTestPolygon,ShapeTests);
PF_TIMER(EdgeTestPolygon,ShapeTests);
PF_TIMER(SphereTestPolygon,ShapeTests);
PF_TIMER(CapsuleTestPolygon,ShapeTests);
PF_TIMER(SweptSphereTestPolygon,ShapeTests);
PF_TIMER(TaperedSweptSphereTestPolygon,ShapeTests);
PF_TIMER(ScalingSweptQuadTestPolygon,ShapeTests);
PF_TIMER(BoxTestPolygon,ShapeTests);
PF_TIMER(ObjectTestPolygon,ShapeTests);
PF_TIMER(TestBoundPrimitive,ShapeTests);
#endif

template <typename ShapeType> const char* GetShapeTypeString();
template <> const char* GetShapeTypeString<phShapeBatch>()				{ return "phShapeBatch"; }
template <> const char* GetShapeTypeString<phShapeCapsule>()			{ return "phShapeCapsule"; }
template <> const char* GetShapeTypeString<phShapeEdge>()				{ return "phShapeEdge"; }
template <> const char* GetShapeTypeString<phShapeObject>()				{ return "phShapeObject"; }
template <> const char* GetShapeTypeString<phShapeProbe>()				{ return "phShapeProbe"; }
template <> const char* GetShapeTypeString<phShapeSphere>()				{ return "phShapeSphere"; }
template <> const char* GetShapeTypeString<phShapeSweptSphere>()		{ return "phShapeSweptSphere"; }
template <> const char* GetShapeTypeString<phShapeTaperedSweptSphere>()	{ return "phTaperedShapeSweptSphere"; }
template <> const char* GetShapeTypeString<phShapeScalingSweptQuad>()	{ return "phShapeScalingSweptQuad"; }

#if VALIDATE_SHAPETEST_RESULTS
namespace ShapetestValidation
{
int g_NumInvalidResults = 0;

// Utility distance functions that give the signed distance between a point and a basic shape (positive: outside, negative: inside)
ScalarV_Out DistanceOfPointFromBox(Vec3V_In point, Vec3V_In boxMin, Vec3V_In boxMax)
{
	ScalarV distanceIfOutside = Dist(point, Clamp(point, boxMin, boxMax));
	Vec3V centerToPoint = Abs(Subtract(point, Average(boxMax, boxMin)));
	Vec3V distancesToSurface = Subtract(Abs(centerToPoint), Scale(Subtract(boxMax,boxMin), ScalarV(V_HALF)));
	ScalarV distanceIfInside = Max(distancesToSurface.GetX(), Max(distancesToSurface.GetY(), distancesToSurface.GetZ()));
	return SelectFT(IsLessThan(distanceIfInside, ScalarV(V_ZERO)), distanceIfOutside, distanceIfInside);
}

ScalarV_Out DistanceOfPointFromSphere(Vec3V_In centroidToPoint, ScalarV_In sphereRadius)
{
	return Subtract(Mag(centroidToPoint), sphereRadius);
}

ScalarV_Out DistanceOfPointFromCylinder(Vec3V_In centroidToPoint, ScalarV_In cylinderRadius, ScalarV_In cylinderHalfHeight)
{
	ScalarV xzDistanceFromCylinder = Subtract(Mag(centroidToPoint.Get<Vec::X, Vec::Z>()), cylinderRadius);
	ScalarV yDistanceFromCylinder = Subtract(Abs(centroidToPoint.GetY()), cylinderHalfHeight);
	ScalarV distanceIfOutside = Mag(Max(Vec2V(V_ZERO), Vec2V(xzDistanceFromCylinder, yDistanceFromCylinder)));
	ScalarV distanceIfInside = Max(yDistanceFromCylinder, xzDistanceFromCylinder);
	return SelectFT(IsLessThan(distanceIfInside, ScalarV(V_ZERO)), distanceIfOutside, distanceIfInside);
}

ScalarV_Out DistanceOfPointFromDisc(Vec3V_In centroidToPoint, ScalarV_In discRadius)
{
	ScalarV yzDistanceFromDisc = Max(Subtract(Mag(centroidToPoint.Get<Vec::Y, Vec::Z>()), discRadius), ScalarV(V_ZERO));
	ScalarV xDistanceFromDisc = centroidToPoint.GetX();
	return Mag(Vec2V(yzDistanceFromDisc, xDistanceFromDisc));
}

ScalarV_Out DistanceOfPointFromSegment(Vec3V_In point, Vec3V_In segmentBegin, Vec3V_In segmentEnd)
{
	Vec3V segment = Subtract(segmentEnd, segmentBegin);
	Vec3V segmentNormalized = NormalizeSafe(segment, Vec3V(V_ZERO));
	ScalarV tSegment = Dot(Subtract(point, segmentBegin), segmentNormalized);
	tSegment = Clamp(tSegment, ScalarV(V_ZERO), Mag(segment));
	Vec3V closestPointOnSegment = AddScaled(segmentBegin, segmentNormalized, tSegment);
	return Dist(point, closestPointOnSegment);
}

ScalarV_Out DistanceOfPointFromTriangle(Vec3V_In p, Vec3V_In a, Vec3V_In b, Vec3V_In c)
{
	Vec3V normal = NormalizeSafe(Cross(Subtract(b, a), Subtract(c, a)), Vec3V(V_ZERO));
	
	if(IsZeroAll(normal))
	{
		// Degenerate triangle, just test distance from both segments
		return Min(DistanceOfPointFromSegment(p, a, b), DistanceOfPointFromSegment(p, a, c));
	}
	else
	{
		// Clamp p to the plane
		Vec3V pInTriangle = p;

		pInTriangle = Subtract(pInTriangle, Scale(Dot(Subtract(pInTriangle, a), normal), normal));

		// Clamp p to each edge normal plane
		Vec3V acNorm = NormalizeSafe(Cross(normal, Subtract(c,a)), Vec3V(V_ZERO));
		Vec3V baNorm = NormalizeSafe(Cross(normal, Subtract(a,b)), Vec3V(V_ZERO));
		Vec3V cbNorm = NormalizeSafe(Cross(normal, Subtract(b,c)), Vec3V(V_ZERO));

		pInTriangle = Subtract(pInTriangle, Scale(Max(Dot(Subtract(pInTriangle, a), acNorm), ScalarV(V_ZERO)), acNorm));
		pInTriangle = Subtract(pInTriangle, Scale(Max(Dot(Subtract(pInTriangle, b), baNorm), ScalarV(V_ZERO)), baNorm));
		pInTriangle = Subtract(pInTriangle, Scale(Max(Dot(Subtract(pInTriangle, c), cbNorm), ScalarV(V_ZERO)), cbNorm));
		
		return Dist(p, pInTriangle);
	}
}

// Gets the signed distance of a point from a polyhedral primitive (positive: outside, negative: inside).
ScalarV_Out DistanceOfPointFromPrimitive(Vec3V_In point, const phBoundPolyhedron& polyhedronBound, u16 partIndex)
{
	const phPrimitive& primitive = polyhedronBound.GetPolygon(partIndex).GetPrimitive();

	switch(primitive.GetType())
	{
		case PRIM_TYPE_POLYGON:
		{			
			const phPolygon& polygon = primitive.GetPolygon();
			Vec3V vert0 = polyhedronBound.GetVertex(polygon.GetVertexIndex(0));
			Vec3V vert1 = polyhedronBound.GetVertex(polygon.GetVertexIndex(1));
			Vec3V vert2 = polyhedronBound.GetVertex(polygon.GetVertexIndex(2));

			// Will test ever give a part index of a polygon when it used a margin?
			return DistanceOfPointFromTriangle(point, vert0, vert1, vert2);
		}
		case PRIM_TYPE_SPHERE:
		{
			const phPrimSphere& sphere = primitive.GetSphere();
			Vec3V center = polyhedronBound.GetVertex(sphere.GetCenterIndex());
			return DistanceOfPointFromSphere(Subtract(point, center), sphere.GetRadiusV());
		}
		case PRIM_TYPE_CAPSULE:
		{
			const phPrimCapsule& capsule = primitive.GetCapsule();
			Vec3V start = polyhedronBound.GetVertex(capsule.GetEndIndex0());
			Vec3V end = polyhedronBound.GetVertex(capsule.GetEndIndex1());
			return Subtract(DistanceOfPointFromSegment(point, start, end), capsule.GetRadiusV());
		}
		case PRIM_TYPE_BOX:
		{
			const phPrimBox& box = primitive.GetBox();						
			Vec3V vert0 = polyhedronBound.GetVertex(box.GetVertexIndex(0));
			Vec3V vert1 = polyhedronBound.GetVertex(box.GetVertexIndex(1));
			Vec3V vert2 = polyhedronBound.GetVertex(box.GetVertexIndex(2));
			Vec3V vert3 = polyhedronBound.GetVertex(box.GetVertexIndex(3));
			Mat34V localBoxMatrix;
			Vec3V boxSize;
			ScalarV margin;
			geomBoxes::ComputeBoxDataFromOppositeDiagonals(vert0, vert1, vert2, vert3, localBoxMatrix, boxSize, margin);
			boxSize = Scale(boxSize, ScalarV(V_HALF));
			
			return DistanceOfPointFromBox(UnTransformOrtho(localBoxMatrix, point), Negate(boxSize), boxSize);
		}
		case PRIM_TYPE_CYLINDER:
		{
			const phPrimCylinder& cylinder = primitive.GetCylinder();
			Vec3V start = polyhedronBound.GetVertex(cylinder.GetEndIndex0());
			Vec3V end = polyhedronBound.GetVertex(cylinder.GetEndIndex1());
			Vec3V shaft = Subtract(start,end);
			Mat34V localCylinderMatrix(V_IDENTITY);
			// Check if the shaft axis is parallel to the y-axis.
			const VecBoolV maskY = VecBoolV(V_F_T_F_F);
			if(!IsZeroAll(SelectFT(maskY, shaft, Vec3V(V_ZERO))))
			{
				localCylinderMatrix.SetCol0(Normalize(Cross(shaft, Vec3V(V_Y_AXIS_WONE))));
				localCylinderMatrix.SetCol1(Normalize(shaft));
				localCylinderMatrix.SetCol2(Cross(localCylinderMatrix.GetCol0(), localCylinderMatrix.GetCol1()));
			}
			localCylinderMatrix.SetCol3(Average(start, end));

			return DistanceOfPointFromCylinder(UnTransformOrtho(localCylinderMatrix, point), cylinder.GetRadiusV(), Scale(Mag(shaft), ScalarV(V_HALF)));
		}
		default:
		{
			return ScalarV(V_ZERO);
		}
	}
}

// Gets the signed distance of a point from a bound (positive: outside, negative: inside)
ScalarV_Out DistanceOfPointFromBound(Vec3V_In point, const phBound& bound, int componentIndex, int partIndex)
{
	Vec3V centroidToPoint = Subtract(point, bound.GetCentroidOffset());
	ScalarV margin = bound.GetMarginV();

	switch(bound.GetType())
	{
		case phBound::SPHERE:
		{
			return DistanceOfPointFromSphere(centroidToPoint, bound.GetMarginV());
		}
		case phBound::DISC:
		{
			const phBoundDisc& discBound = static_cast<const phBoundDisc&>(bound);
			return Subtract(DistanceOfPointFromDisc(centroidToPoint, discBound.GetRadiusV()), margin);
		}
		case phBound::CYLINDER:
		{
			const phBoundCylinder& cylinderBound = static_cast<const phBoundCylinder&>(bound);
			return DistanceOfPointFromCylinder(centroidToPoint, cylinderBound.GetRadiusV(), cylinderBound.GetHalfHeightV());
		}
		case phBound::CAPSULE:
		{
			const phBoundCapsule& capsuleBound = static_cast<const phBoundCapsule&>(bound);
			Vec3V capsuleSegmentTop = Scale(Vec3V(V_Y_AXIS_WZERO), capsuleBound.GetHalfLengthV());
			return Subtract(DistanceOfPointFromSegment(centroidToPoint, capsuleSegmentTop, Negate(capsuleSegmentTop)), margin);
		}

		case phBound::COMPOSITE:
		{
			const phBoundComposite& compositeBound = static_cast<const phBoundComposite&>(bound);
			ScalarV compositeDistance = DistanceOfPointFromBox(point, bound.GetBoundingBoxMin(), bound.GetBoundingBoxMax());

			const phBound* subBound = componentIndex >= 0 ? compositeBound.GetBound(componentIndex) : NULL;
			if(subBound)
			{
				// Choose the largest distance between the sub-bound and the composite.
				Mat34V compositeToComponentSpace = compositeBound.GetCurrentMatrix(componentIndex);
				ScalarV subBoundDistance = DistanceOfPointFromBound(UnTransformOrtho(compositeToComponentSpace, point), *subBound, 0, partIndex);
#if !__TOOL
				// If this isn't on the main thread, then it's possible that the main thread has updated the component matrix since the intersection was filled.
				// Use whichever distance is smaller between the current and last matrix to fix this.
				if(!sysThreadType::IsUpdateThread())
				{
					Mat34V instanceFromComponentLast = compositeBound.GetCurrentMatrix(componentIndex);
					subBoundDistance = Min(subBoundDistance, DistanceOfPointFromBound(UnTransformOrtho(instanceFromComponentLast, point), *subBound, 0, partIndex));
				}
#endif
				return Max(subBoundDistance, compositeDistance);
			}
			else
			{
				return compositeDistance;
			}
		}

		case phBound::BVH:
		case phBound::GEOMETRY:
		{
			const phBoundPolyhedron& polyhedronBound = static_cast<const phBoundPolyhedron&>(bound);
			ScalarV polyhedronDistance = DistanceOfPointFromBox(point, bound.GetBoundingBoxMin(), bound.GetBoundingBoxMax());

			// If this is a valid part, find the the distance to the part and choose the largest distance
			if(partIndex >= 0 && Verifyf(partIndex < polyhedronBound.GetNumPolygons(), "Intersection has part index (%i) greater than the number of polygons (%i) on the bound.", partIndex, polyhedronBound.GetNumPolygons()))
			{
				return Max(polyhedronDistance, DistanceOfPointFromPrimitive(point, polyhedronBound, (u16)partIndex));
			}
			else
			{
				return polyhedronDistance;
			}
		}

		// The remaining bounds (other than boxes) are too complicated to find the distance from a point, so just use their bounding box
		default:
		{
			return DistanceOfPointFromBox(point, bound.GetBoundingBoxMin(), bound.GetBoundingBoxMax());
		}
	}
}


// Gets the signed distance from a point to a shape (positive: inside, negative: outside)
ScalarV_Out DistanceOfIntersectionFromShape(const phIntersection& worldIntersection, const phShapeCapsule& capsuleShape)
{
	const Vec3V pointOnCapsule = SubtractScaled(worldIntersection.GetPosition(),worldIntersection.GetNormal(),worldIntersection.GetDepthV());
	const Vec3V start = capsuleShape.GetWorldSegment().GetStart();
	const Vec3V end = capsuleShape.GetWorldSegment().GetEnd();
	return Subtract(DistanceOfPointFromSegment(pointOnCapsule, start, end), ScalarV(capsuleShape.GetRadius()));
}
ScalarV_Out DistanceOfIntersectionFromShape(const phIntersection& worldIntersection, const phShapeSweptSphere& sweptSphereShape)
{
	if(sweptSphereShape.GetTestInitialSphere() && IsEqualAll(worldIntersection.GetTV(),ScalarV(V_ZERO)))
	{
		// We don't have a valid depth on the sphere intersection
		const Vec3V pointOnBound = worldIntersection.GetPosition();
		return	DistanceOfPointFromSphere(Subtract(pointOnBound, sweptSphereShape.GetWorldSegment().GetStart()), ScalarV(sweptSphereShape.GetRadius()));
	}
	else
	{
		const Vec3V tPosition = Lerp(worldIntersection.GetTV(),sweptSphereShape.GetWorldSegment().GetStart(),sweptSphereShape.GetWorldSegment().GetEnd());
		return	DistanceOfPointFromSphere(Subtract(worldIntersection.GetPosition(),tPosition), sweptSphereShape.GetRadius());
	}
}
ScalarV_Out DistanceOfIntersectionFromShape(const phIntersection& worldIntersection, const phShapeTaperedSweptSphere& taperedSweptSphereShape)
{
	const Vec3V tPosition = Lerp(worldIntersection.GetTV(),taperedSweptSphereShape.GetWorldSegment().GetStart(),taperedSweptSphereShape.GetWorldSegment().GetEnd());
	const ScalarV tRadius = Lerp(worldIntersection.GetTV(),taperedSweptSphereShape.GetInitialRadius(),taperedSweptSphereShape.GetFinalRadius());
	return	DistanceOfPointFromSphere(Subtract(worldIntersection.GetPosition(),tPosition), tRadius);
}
ScalarV_Out DistanceOfIntersectionFromShape(const phIntersection& worldIntersection, const phShapeScalingSweptQuad& scalingSweptQuad)
{
	// Transform the point into the intersection quads space and find the distance between the point and the quad
	const Vec3V tPosition = Lerp(worldIntersection.GetTV(),scalingSweptQuad.GetWorldSegment().GetStart(),scalingSweptQuad.GetWorldSegment().GetEnd());
	const Vec2V tHalfExtents = Lerp(worldIntersection.GetTV(),scalingSweptQuad.GetInitialHalfExtents(),scalingSweptQuad.GetFinalHalfExtents());
	const Vec3V quadBoxHalfExtents = Vec3V(ScalarV(V_ZERO),tHalfExtents.GetY(),tHalfExtents.GetX());
	const Vec3V absPositionInQuadSpace = Abs(UnTransformOrtho(Mat34V(scalingSweptQuad.GetWorldRotation(),tPosition),worldIntersection.GetPosition()));
	return DistanceOfPointFromBox(absPositionInQuadSpace,Negate(quadBoxHalfExtents),quadBoxHalfExtents);
}
ScalarV_Out DistanceOfIntersectionFromShape(const phIntersection& worldIntersection, const phShapeProbe& probeShape)
{
	const Vec3V tPosition = Lerp(worldIntersection.GetTV(),probeShape.GetWorldSegment().GetStart(),probeShape.GetWorldSegment().GetEnd());
	return Dist(worldIntersection.GetPosition(), tPosition);
}
ScalarV_Out DistanceOfIntersectionFromShape(const phIntersection& worldIntersection, const phShapeSphere& sphereShape)
{
	const Vec3V pointOnSphere = SubtractScaled(worldIntersection.GetPosition(),worldIntersection.GetNormal(),worldIntersection.GetDepthV());
	return	DistanceOfPointFromSphere(Subtract(pointOnSphere, RCC_VEC3V(sphereShape.GetWorldCenter())), ScalarV(sphereShape.GetRadius()));
}
ScalarV_Out DistanceOfIntersectionFromShape(const phIntersection& worldIntersection, const phShapeObject& objectShape)
{
	const phBound* bound = objectShape.GetBound();
	if(Verifyf(bound, "phShapeObject does not have a bound but returned a contact."))
	{
		const Vec3V pointOnObject = SubtractScaled(worldIntersection.GetPosition(),worldIntersection.GetNormal(),worldIntersection.GetDepthV());
		return DistanceOfPointFromBound(UnTransformOrtho(RCC_MAT34V(objectShape.GetWorldTransform()),pointOnObject), *bound, -1, -1);
	}
	else
	{
		return ScalarV(V_ZERO);
	}
}

// Get the class name of the bound
// There is already a GetTypeString function for bounds, but it doesn't contain the word "bound" so it isn't
// as clear when reading
// TODO: Change phBound::GetTypeString to return actual class name
const char* GetBoundTypeString(const phBound& bound)
{
	switch(bound.GetType())
	{
	case phBound::BOX:				return "phBoundBox";
	case phBound::SPHERE:			return "phBoundSphere";
	case phBound::DISC:				return "phBoundDisc";
	case phBound::CYLINDER:			return "phBoundCylinder";
	case phBound::CAPSULE:			return "phBoundCapsule";
	case phBound::COMPOSITE:		return "phBoundComposite";
	case phBound::GEOMETRY:			return "phBoundGeometry";
#if USE_GEOMETRY_CURVED
	case phBound::GEOMETRY_CURVED:	return "phBoundCurvedGeometry";
#endif
	case phBound::BVH:				return "phBoundBvh";
	default:						return bound.GetTypeString();
	}
}

// Fills a buffer with information about what type of bound this is
void FillBoundTypeBuffer(char* buffer, const phBound* bound, int componentIndex, int partIndex)
{
	if(bound == NULL)
	{
		sprintf(buffer, "(null)");
	}
	else if(bound->GetType() == phBound::COMPOSITE)
	{
		const phBoundComposite* compositeBound = static_cast<const phBoundComposite*>(bound);
		buffer = buffer + sprintf(buffer, "phBoundComposite.");

		if(componentIndex >= 0)
		{
			FillBoundTypeBuffer(buffer, compositeBound->GetBound(componentIndex), componentIndex, partIndex);
		}
		else
		{
			FillBoundTypeBuffer(buffer, NULL, componentIndex, partIndex);
		}
	}
	else
	{
		buffer = buffer + sprintf(buffer, "%s", GetBoundTypeString(*bound));

		if(bound->IsPolygonal() USE_GEOMETRY_CURVED_ONLY(&& bound->GetType() != phBound::GEOMETRY_CURVED))
		{
			const phBoundPolyhedron* polyhedronBound = static_cast<const phBoundPolyhedron*>(bound);
			if(partIndex >= 0 && partIndex < polyhedronBound->GetNumPolygons())
			{
				sprintf(buffer, ".%s", polyhedronBound->GetPolygon(partIndex).GetPrimitive().GetTypeName());
			}
		}
	}
}

// Fill the given buffer with the type of the shape
template <typename ShapeType> void FillShapeTypeBuffer(char* buffer, const ShapeType& UNUSED_PARAM(shapeType), bool batched)
{
	if(batched)
	{
		buffer = buffer + sprintf(buffer, "phShapeBatch.");
	}
	sprintf(buffer, "%s", GetShapeTypeString<ShapeType>());
}
template <> void FillShapeTypeBuffer<phShapeObject>(char* buffer, const phShapeObject& objectShape, bool batched)
{
	if(batched)
	{
		buffer = buffer + sprintf(buffer, "phShapeBatch.");
	}
	const char* objectBoundTypeString = objectShape.GetBound() ? GetBoundTypeString(*objectShape.GetBound()) : "null";
	sprintf(buffer, "phShapeObject(%s)", objectBoundTypeString);
}

const char g_TabString[] = "\t\t\t\t\t\t\t\t\t\t\t";
const int g_MaxTabs = (sizeof(g_TabString)/sizeof(g_TabString[0])) - 1;

int g_CurrentIndentation = 0;

const char* GetTabString()
{
	return &g_TabString[g_MaxTabs - Max(Min(g_CurrentIndentation, g_MaxTabs), 0)];
}
void IncrementCurrentIndendation()
{
	if(g_CurrentIndentation < g_MaxTabs)
	{
		++g_CurrentIndentation;
	}
}
void DecrementCurrentIndendation()
{
	if(g_CurrentIndentation > 0)
	{
		--g_CurrentIndentation;
	}
}
// Helper functions for printing data
void DisplayInteger(const char* name, s64 i)
{
	Displayf("%s%s: %" I64FMT "d", GetTabString(), name, i);
}

void DisplayUnsignedInteger(const char* name, u64 u)
{
	Displayf("%s%s: %" I64FMT "u", GetTabString(), name, u);
}

void DisplayFloat(const char* name, float f, bool input = false)
{
	int precision = (input ? INPUT_PRECISION : DEFAULT_PRECISION);
	Displayf("%s%s: %.*f", GetTabString(), name, precision, f);
}

void DisplayNormal(const char* name, Vec3V_In v, bool input = false)
{
	int precision = (input ? INPUT_PRECISION : DEFAULT_PRECISION);
	Displayf("%s%s: %.*f, %.*f, %.*f |%f|", GetTabString(), name, precision, v.GetXf(), precision, v.GetYf(), precision, v.GetZf(), Mag(v).Getf());
}

void DisplayVector(const char* name, Vec3V_In v, bool input = false)
{
	int precision = (input ? INPUT_PRECISION : DEFAULT_PRECISION);
	Displayf("%s%s: %.*f, %.*f, %.*f", GetTabString(), name, precision, v.GetXf(), precision, v.GetYf(), precision, v.GetZf());
}

void DisplayVector(const char* name, Vec2V_In v, bool input = false)
{
	int precision = (input ? INPUT_PRECISION : DEFAULT_PRECISION);
	Displayf("%s%s: %.*f, %.*f", GetTabString(), name, precision, v.GetXf(), precision, v.GetYf());
}

void DisplayMatrix(const char* name, Mat34V_In m, bool input = false)
{
	int precision = (input ? INPUT_PRECISION : DEFAULT_PRECISION);
	Displayf("%s%s:", GetTabString(), name);
	IncrementCurrentIndendation();
	Displayf("%sNon-Orthonormality: %f", GetTabString(), RCC_MATRIX34(m).MeasureNonOrthonormality());
	Displayf("%s%.*f, %.*f, %.*f, %.*f", GetTabString(), precision, m.GetCol0().GetXf(), precision, m.GetCol1().GetXf(), precision, m.GetCol2().GetXf(), precision, m.GetCol3().GetXf());
	Displayf("%s%.*f, %.*f, %.*f, %.*f", GetTabString(), precision, m.GetCol0().GetYf(), precision, m.GetCol1().GetYf(), precision, m.GetCol2().GetYf(), precision, m.GetCol3().GetYf());
	Displayf("%s%.*f, %.*f, %.*f, %.*f", GetTabString(), precision, m.GetCol0().GetZf(), precision, m.GetCol1().GetZf(), precision, m.GetCol2().GetZf(), precision, m.GetCol3().GetZf());
	DecrementCurrentIndendation();
}

// Display relevant information about the given primitive
void DisplayPrimitiveDescription(const phBoundPolyhedron& polyhedronBound, u16 partIndex)
{
	const phPrimitive& primitive = polyhedronBound.GetPolygon(partIndex).GetPrimitive();
	Displayf("%s%s:", GetTabString(), primitive.GetTypeName());

	IncrementCurrentIndendation();
	DisplayInteger("Part Index", partIndex);
	switch(primitive.GetType())
	{
		case PRIM_TYPE_POLYGON:
		{
			const phPolygon& polygon = primitive.GetPolygon();
			Vec3V v0 = polyhedronBound.GetVertex(polygon.GetVertexIndex(0));
			Vec3V v1 = polyhedronBound.GetVertex(polygon.GetVertexIndex(1));
			Vec3V v2 = polyhedronBound.GetVertex(polygon.GetVertexIndex(2));
			DisplayVector("Vertex 0", v0, true);
			DisplayVector("Vertex 1", v1, true);
			DisplayVector("Vertex 2", v2, true);
			DisplayNormal("Normal", polygon.ComputeUnitNormal(v0,v1,v2));
			DisplayFloat("Area", polygon.GetArea());
			break;
		}
		case PRIM_TYPE_SPHERE:
		{
			const phPrimSphere& sphere = primitive.GetSphere();
			DisplayFloat("Radius", sphere.GetRadius());
			DisplayVector("Center", polyhedronBound.GetVertex(sphere.GetCenterIndex()), true);
			break;
		}
		case PRIM_TYPE_CAPSULE:
		{
			const phPrimCapsule& capsule = primitive.GetCapsule();
			Vec3V start = polyhedronBound.GetVertex(capsule.GetEndIndex0());
			Vec3V end = polyhedronBound.GetVertex(capsule.GetEndIndex1());
			DisplayFloat("Radius", capsule.GetRadius());
			DisplayFloat("Length", Dist(start,end).Getf());
			DisplayVector("Start", start, true);
			DisplayVector("End", end, true);
			break;
		}
		case PRIM_TYPE_BOX:
		{
			const phPrimBox& box = primitive.GetBox();						
			Vec3V vert0 = polyhedronBound.GetVertex(box.GetVertexIndex(0));
			Vec3V vert1 = polyhedronBound.GetVertex(box.GetVertexIndex(1));
			Vec3V vert2 = polyhedronBound.GetVertex(box.GetVertexIndex(2));
			Vec3V vert3 = polyhedronBound.GetVertex(box.GetVertexIndex(3));
			Mat34V matrix;
			ScalarV margin;
			Vec3V boxSize;
			geomBoxes::ComputeBoxDataFromOppositeDiagonals(vert0, vert1, vert2, vert3, matrix, boxSize, margin);

			DisplayVector("Vertex 0", vert0, true);
			DisplayVector("Vertex 1", vert1, true);
			DisplayVector("Vertex 2", vert2, true);
			DisplayVector("Vertex 3", vert3, true);
			DisplayVector("Size", boxSize);
			DisplayMatrix("Matrix", matrix);
			break;
		}
		case PRIM_TYPE_CYLINDER:
		{
			const phPrimCylinder& cylinder = primitive.GetCylinder();
			Vec3V start = polyhedronBound.GetVertex(cylinder.GetEndIndex0());
			Vec3V end = polyhedronBound.GetVertex(cylinder.GetEndIndex1());;
			DisplayFloat("Radius", cylinder.GetRadius());
			DisplayFloat("Length", Dist(start,end).Getf());
			DisplayVector("Start", start, true);
			DisplayVector("End", end, true);
			break;
		}
		default:
		{
			break;
		}
	}
	DecrementCurrentIndendation();
}

// Display relevant information about the bound in question
void DisplayBoundDescription(const phBound* bound, int componentIndex, int partIndex)
{
	if(bound == NULL)
	{
		return;
	}

	// Information common to all bounds
	Displayf("%s%s:", GetTabString(), GetBoundTypeString(*bound));
	IncrementCurrentIndendation();
	if(bound->GetType() != phBound::COMPOSITE && componentIndex >= 0)
	{
		DisplayInteger("Component Index", componentIndex);
	}
	DisplayFloat("Margin", bound->GetMargin(), true);
	DisplayFloat("Centroid Radius", bound->GetRadiusAroundCentroid());
	DisplayVector("Centroid Offset", bound->GetCentroidOffset());
	DisplayVector("Bounding Box Min", bound->GetBoundingBoxMin());
	DisplayVector("Bounding Box Max", bound->GetBoundingBoxMax());

	switch(bound->GetType())
	{
		case phBound::COMPOSITE:
		{
			const phBoundComposite* compositeBound = static_cast<const phBoundComposite*>(bound);

			DisplayInteger("Num Active Bounds", compositeBound->GetNumActiveBounds());
			DisplayInteger("Num Bounds", compositeBound->GetNumBounds());

			// Display information about the sub-bound, if it exists
			if(componentIndex >= 0)
			{
				DisplayMatrix("Current Matrix", compositeBound->GetCurrentMatrix(componentIndex), true);
				DisplayMatrix("Last Matrix", compositeBound->GetLastMatrix(componentIndex), true);
				DisplayBoundDescription(compositeBound->GetBound(componentIndex), componentIndex, partIndex);
			}
			break;
		}
		case phBound::GEOMETRY:
		case phBound::BVH:
		{
			// Information common to all polyhedral bounds
			const phBoundPolyhedron* polyhedronBound = static_cast<const phBoundPolyhedron*>(bound);
			DisplayInteger("Num Primitives", polyhedronBound->GetNumPolygons());
			DisplayInteger("Num Vertices", polyhedronBound->GetNumVertices());
			if(partIndex >= 0 && Verifyf(partIndex < polyhedronBound->GetNumPolygons(), "Intersection has part index (%i) greater than the number of polygons (%i) on the bound.", partIndex, polyhedronBound->GetNumPolygons()))
			{
				DisplayPrimitiveDescription(*polyhedronBound, (u16)partIndex);
			}
			break;
		}
#if USE_GEOMETRY_CURVED
		case phBound::GEOMETRY_CURVED:
		{
			const phBoundCurvedGeometry* curvedGeomBound = static_cast<const phBoundCurvedGeometry*>(bound);
			DisplayInteger("Num Primitives", curvedGeomBound->GetNumPolygons());
			DisplayInteger("Num Vertices", curvedGeomBound->GetNumVertices());
			DisplayInteger("Num Curved Edges", curvedGeomBound->GetNumCurvedEdges());
			DisplayInteger("Num Curved Faces", curvedGeomBound->GetNumCurvedFaces());
			break;
		}
#endif
		case phBound::BOX:
		{
			DisplayVector("Size", bound->GetBoundingBoxSize(), true);
			break;
		}
		case phBound::SPHERE:
		{
			const phBoundSphere* sphereBound = static_cast<const phBoundSphere*>(bound);
			DisplayFloat("Radius", sphereBound->GetRadius(), true);
			break;
		}
		case phBound::CYLINDER:
		{
			const phBoundCylinder* cylinderBound = static_cast<const phBoundCylinder*>(bound);
			DisplayFloat("Radius", cylinderBound->GetRadius(), true);
			DisplayFloat("Height", cylinderBound->GetHalfHeight()*2.0f, true);
			break;
		}
		case phBound::CAPSULE:
		{
			const phBoundCapsule* capsuleBound = static_cast<const phBoundCapsule*>(bound);
			DisplayFloat("Radius", capsuleBound->GetRadius(), true);
			DisplayFloat("Length", capsuleBound->GetLength(), true);
			break;
		}
		case phBound::DISC:
		{
			const phBoundDisc* discBound = static_cast<const phBoundDisc*>(bound);
			DisplayFloat("Radius", discBound->GetRadius(), true);
			break;
		}
		default:
		{
			break;
		}
	}
	DecrementCurrentIndendation();
}

// Determine how far intersection points can be from the shape. We compute this point using the point on the bound, the normal, and the depth. 
// Any test that relies on a penetration depth solver needs a lot more tolerance unfortunately. 
template <typename ShapeType> float GetToleratedDistanceFromShape(bool UNUSED_PARAM(sweptIntersection), int UNUSED_PARAM(intersectionBoundType)) { return TOLERATED_DISTANCE_FROM_SHAPE; }
template <> float GetToleratedDistanceFromShape<phShapeSphere>(bool UNUSED_PARAM(sweptIntersection), int intersectionBoundType)
{
	switch(intersectionBoundType)
	{
	case phBound::TRIANGLE:
	case phBound::DISC:			
		USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)	
	case phBound::CYLINDER:		
	case phBound::GEOMETRY:		return TOLERATED_PENETRATION_SOLVER_ERROR;
	default:					return TOLERATED_DISTANCE_FROM_SHAPE;
	}
}
template <> float GetToleratedDistanceFromShape<phShapeSweptSphere>(bool sweptIntersection, int UNUSED_PARAM(intersectionBoundType))
{
	if(!sweptIntersection)
	{
		// We don't have the depth of the sphere intersection so we need to be extra tolerant
		return TOLERATED_SWEPT_SPHERE_INITIAL_SPHERE_ERROR;
	}
	else
	{
		return TOLERATED_DISTANCE_FROM_SHAPE;
	}
}
template <> float GetToleratedDistanceFromShape<phShapeCapsule>(bool UNUSED_PARAM(sweptIntersection), int intersectionBoundType)
{
	switch(intersectionBoundType)
	{
	case phBound::TRIANGLE:
	case phBound::DISC:			
	USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)	
	case phBound::CYLINDER:		
	case phBound::GEOMETRY:		return TOLERATED_PENETRATION_SOLVER_ERROR;
	default:					return TOLERATED_DISTANCE_FROM_SHAPE;
	}
}
template <> float GetToleratedDistanceFromShape<phShapeObject>(bool UNUSED_PARAM(sweptIntersection), int UNUSED_PARAM(intersectionBoundType))
{
	return TOLERATED_PENETRATION_SOLVER_ERROR;
}
template <> float GetToleratedDistanceFromShape<phShapeScalingSweptQuad>(bool UNUSED_PARAM(sweptIntersection), int UNUSED_PARAM(intersectionBoundType))
{
	return TOLERATED_DISTANCE_FROM_SHAPE_ITERATIVE_BOUND_CAST;
}

// Displays relevant information about the given shape
void DisplayShapeDescription(const phShapeCapsule& capsuleShape, Mat34V_In localFromWorld)
{
	DisplayFloat("Radius", capsuleShape.GetRadius().Getf(), true);
	DisplayFloat("Length", capsuleShape.GetWorldSegment().GetLength().Getf());
	DisplayVector("Start", Transform(localFromWorld,capsuleShape.GetWorldSegment().GetStart()), true);
	DisplayVector("End", Transform(localFromWorld,capsuleShape.GetWorldSegment().GetEnd()), true);
}
void DisplayShapeDescription(const phShapeTaperedSweptSphere& taperedSweptSphereShape, Mat34V_In localFromWorld)
{
	DisplayFloat("Initial Radius", taperedSweptSphereShape.GetInitialRadius().Getf(), true);
	DisplayFloat("Final Radius", taperedSweptSphereShape.GetFinalRadius().Getf(), true);
	DisplayFloat("Length", taperedSweptSphereShape.GetWorldSegment().GetLength().Getf());
	DisplayVector("Start", Transform(localFromWorld,taperedSweptSphereShape.GetWorldSegment().GetStart()), true);
	DisplayVector("End", Transform(localFromWorld,taperedSweptSphereShape.GetWorldSegment().GetEnd()), true);
}
void DisplayShapeDescription(const phShapeScalingSweptQuad& scalingSweptQuadShape, Mat34V_In localFromWorld)
{
	Mat33V localRotation;
	Multiply(localRotation,localFromWorld.GetMat33ConstRef(),scalingSweptQuadShape.GetWorldRotation());
	DisplayVector("Initial Half Extents", scalingSweptQuadShape.GetInitialHalfExtents(), true);
	DisplayVector("Final Half Extents", scalingSweptQuadShape.GetFinalHalfExtents(), true);
	DisplayFloat("Length", scalingSweptQuadShape.GetWorldSegment().GetLength().Getf());
	DisplayVector("Start", Transform(localFromWorld,scalingSweptQuadShape.GetWorldSegment().GetStart()), true);
	DisplayVector("End", Transform(localFromWorld,scalingSweptQuadShape.GetWorldSegment().GetEnd()), true);
	DisplayMatrix("Rotation", Mat34V(localRotation,Vec3V(V_ZERO)), true);
}
void DisplayShapeDescription(const phShapeObject& objectShape, Mat34V_In localFromWorld)
{
	Mat34V localTransform;
	Transform(localTransform,localFromWorld,RCC_MAT34V(objectShape.GetWorldTransform()));
	DisplayMatrix("Transform Matrix", localTransform, true);
	const phBound* bound = objectShape.GetBound();
	DisplayBoundDescription(bound, -1, -1);
	if(bound->GetType() == phBound::COMPOSITE)
	{
		IncrementCurrentIndendation();
		const phBoundComposite* compositeBound = static_cast<const phBoundComposite*>(bound);
		for(int componentIndex = 0; componentIndex < compositeBound->GetNumBounds(); ++componentIndex)
		{
			if(const phBound* subBound = compositeBound->GetBound(componentIndex))
			{
				DisplayInteger("Bound",componentIndex);
				IncrementCurrentIndendation();
				DisplayMatrix("Current Matrix",compositeBound->GetCurrentMatrix(componentIndex));
				DisplayMatrix("Last Matrix",compositeBound->GetLastMatrix(componentIndex));
				DisplayBoundDescription(subBound,-1,-1);
				DecrementCurrentIndendation();
			}			
		}
		DecrementCurrentIndendation();
	}

}
void DisplayShapeDescription(const phShapeProbe& probeShape, Mat34V_In localFromWorld)
{
	DisplayFloat("Length", probeShape.GetWorldSegment().GetLength().Getf());
	DisplayVector("Start", Transform(localFromWorld,probeShape.GetWorldSegment().GetStart()), true);
	DisplayVector("End", Transform(localFromWorld,probeShape.GetWorldSegment().GetEnd()), true);
}
void DisplayShapeDescription(const phShapeSphere& sphereShape, Mat34V_In localFromWorld)
{
	DisplayFloat("Radius", sphereShape.GetRadius(), true);
	DisplayVector("Center", Transform(localFromWorld,RCC_VEC3V(sphereShape.GetWorldCenter())), true);
}

template <typename ShapeType>
void ValidateIntersection(const phIntersection& worldIntersection, const ShapeType& shapeType, bool batched)
{
	bool isMainThread = true;
#if !__TOOL
	isMainThread = sysThreadType::IsUpdateThread();
#endif // !__TOOL

	const phInst* instance = worldIntersection.GetInstance();
	if(!instance || !instance->GetArchetype() || !instance->GetArchetype()->GetBound())
	{
		return;
	}
	const char* archetypeFilename = instance->GetArchetype()->GetFilename();
	phIntersection intersection = worldIntersection;
	Mat34V worldFrominstance = instance->GetMatrix();
	Mat34V instanceFromWorld;
	InvertTransformOrtho(instanceFromWorld,worldFrominstance);
	intersection.Transform(instanceFromWorld);

	Vec3V position = intersection.GetPosition();
	Vec3V normal = intersection.GetNormal();
	Vec3V polyNormal = intersection.GetIntersectedPolyNormal();
	int componentIndex = intersection.GetComponent();
	int partIndex = intersection.GetPartIndex();

	const phBound& bound = *instance->GetArchetype()->GetBound();
	const phBoundComposite* compositeBound = (bound.GetType() == phBound::COMPOSITE) ? static_cast<const phBoundComposite*>(&bound) : NULL;
	const phBound* intersectionBound = &bound;

	// If this was a composite intersection, get the sub-bound we intersected with. Assert and return if the bound doesn't exist.
	if(compositeBound)
	{
		if(componentIndex < compositeBound->GetNumBounds() || compositeBound->GetBound(componentIndex) == NULL)
		{
			if(isMainThread)
			{	
				Assertf(componentIndex < compositeBound->GetNumBounds(), "Intersection with composite '%s' has out of range component index (%i). Num Bounds = %i",archetypeFilename,componentIndex,compositeBound->GetNumBounds());
				Assertf(compositeBound->GetBound(componentIndex) != NULL, "Intersection with composite '%s' references NULL component (%i). Num Bounds = %i",archetypeFilename,componentIndex,compositeBound->GetNumBounds());
			}
			return;
		}
		else
		{
			intersectionBound = compositeBound->GetBound(componentIndex);
		}
	}
	else
	{
		componentIndex = -1;
	}

	// Invalidate the part index of geometry bounds treated as a convex hull
	if((shapeType.TreatPolyhedralBoundsAsPrimitives() && intersectionBound->GetType() == phBound::GEOMETRY) USE_GEOMETRY_CURVED_ONLY(|| intersectionBound->GetType() == phBound::GEOMETRY_CURVED))
	{
		partIndex = -1;
	}

	// Make sure the part index is valid, just assert return if it isn't
	if(intersectionBound->IsPolygonal() && !Verifyf(partIndex < static_cast<const phBoundPolyhedron*>(intersectionBound)->GetNumPolygons(),"Intersection with polyhedral bound on '%s' has out of range primitive index %i. Num Primitives = %i",archetypeFilename,partIndex,static_cast<const phBoundPolyhedron*>(intersectionBound)->GetNumPolygons()))
	{
		return;
	}

	// Determine what bound type this intersection was made with
	int intersectionBoundType = 0;
	if(intersectionBound->IsPolygonal() && partIndex != -1)
	{
		PrimitiveType primitiveType = static_cast<const phBoundPolyhedron*>(intersectionBound)->GetPolygon(partIndex).GetPrimitive().GetType();
		switch(primitiveType)
		{
		case PRIM_TYPE_BOX: intersectionBoundType = phBound::BOX; break;
		case PRIM_TYPE_CAPSULE: intersectionBoundType = phBound::CAPSULE; break;
		case PRIM_TYPE_CYLINDER: intersectionBoundType = phBound::CYLINDER; break;
		case PRIM_TYPE_SPHERE: intersectionBoundType = phBound::SPHERE; break;
		case PRIM_TYPE_POLYGON: intersectionBoundType = phBound::TRIANGLE; break;
		default: Assert(false);
		}
	}
	else
	{
		intersectionBoundType = intersectionBound->GetType();
	}

	// Find the distance of the intersection from the shape and bound
	float distanceFromBound = DistanceOfPointFromBound(position, bound, componentIndex, partIndex).Getf();
	float distanceFromShape = DistanceOfIntersectionFromShape(worldIntersection, shapeType).Getf();
	float normalMagnitude = Mag(normal).Getf();
	float polyNormalMagnitude = Mag(polyNormal).Getf();

	float toleratedDistanceFromBound = TOLERATED_DISTANCE_FROM_BOUND;
	if(!isMainThread)
	{
		// Since our results are in world space and the instance could be moved we need more tolerance on async shapetests
		toleratedDistanceFromBound += 10.0f;
	}
	float toleratedDistanceFromShape = GetToleratedDistanceFromShape<ShapeType>(worldIntersection.GetT() > 0, intersectionBoundType);

	bool tooFarFromBound = distanceFromBound > toleratedDistanceFromBound;
	bool tooFarFromShape = distanceFromShape > toleratedDistanceFromShape;
	bool nonNormalNormal = Max(Abs(normalMagnitude-1.0f),Abs(polyNormalMagnitude-1.0f)) > TOLERATED_NORMAL_ERROR;
	bool isNonFinite = !worldIntersection.AreAllValuesFinite();
	bool invalidShapeTestResultsDetected = tooFarFromBound || tooFarFromShape || nonNormalNormal || isNonFinite;
	if(invalidShapeTestResultsDetected)
	{
		g_NumInvalidResults++;

		// Check that the bound and instance is set up correctly before asserting that the shapetest result is wrong. 
		bool validBound = true;

		// Make sure the instance matrix is orthonormal
		bool instanceMatrixIsOrthonormal = worldFrominstance.IsOrthonormal3x3(ScalarV(V_FLT_SMALL_2));
		Assertf(instanceMatrixIsOrthonormal,	"Non-orthonormal instance matrix found during shapetest."
												"\n\tArchetype Filename: %s"
												"\n\tInstance Matrix"
												"\n\t\t%f %f %f %f"
												"\n\t\t%f %f %f %f"
												"\n\t\t%f %f %f %f",
												archetypeFilename,MAT34V_ARG_FLOAT_RC(worldFrominstance));
		validBound &= instanceMatrixIsOrthonormal;

		if(compositeBound)
		{
			const char* componentBoundTypeString = GetBoundTypeString(*intersectionBound);
			bool compositeBoundingBoxContainsComponent = compositeBound->DoesBoundingBoxContainComponent(componentIndex);
			// It's possible that the main thread has moved the component and hasn't updated the composite bounding box yet. 
			Assertf(compositeBoundingBoxContainsComponent || !isMainThread,		"Component bounding box not contained by composite bounding box. Somebody forgot to call phBoundComposite::CalculateCompositeExtents."
																				"\n\tArchetype Filename: %s"
																				"\n\tComponent Bound Type: %s"
																				"\n\tComponent Index: %i",
																				archetypeFilename, componentBoundTypeString, componentIndex);

			bool componentBoundingBoxContainsSupports = compositeBound->DoesComponentBoundingBoxContainSupports(componentIndex);
			Assertf(componentBoundingBoxContainsSupports,	"Component bounding box does not contain its support points."
															"\n\tArchetype Filename: %s"
															"\n\tComponent Bound Type: %s"
															"\n\tComponent Index: %i",
															archetypeFilename, componentBoundTypeString, componentIndex);

			Mat34V matrix = compositeBound->GetCurrentMatrix(componentIndex);
			bool componentMatrixIsOrthonormal = matrix.IsOrthonormal3x3(ScalarV(V_FLT_SMALL_2));
			Assertf(componentMatrixIsOrthonormal,	"Component matrix is not orthonormal."
													"\n\tArchetype Filename: %s"
													"\n\tComponent Bound Type: %s"
													"\n\tComponent Index: %i"
													"\n\tComponent Matrix:"
													"\n\t\t%f %f %f %f"
													"\n\t\t%f %f %f %f"
													"\n\t\t%f %f %f %f",
													archetypeFilename, componentBoundTypeString, componentIndex,MAT34V_ARG_FLOAT_RC(matrix));

			bool componentBoundingBoxMatchesComposite = compositeBound->DoesLocalBoundingBoxEqualComponentBoundingBox(componentIndex);
			Assertf(componentBoundingBoxMatchesComposite,	"Component bound's bounding box doesn't match composite's cached bounding box. Somebody forgot to call phBoundComposite::CalculateCompositeExtents."
															"\n\tArchetype Filename: %s"
															"\n\tComponent Bound Type: %s"
															"\n\tComponent Index: %i"
															"\n\tBound Min: %5.3f, %5.3f, %5.3f"
															"\n\tBound Max: %5.3f, %5.3f, %5.3f"
															"\n\tCached Min: %5.3f, %5.3f, %5.3f"
															"\n\tCached Max: %5.3f, %5.3f, %5.3f",
															archetypeFilename, componentBoundTypeString, componentIndex,
															VEC3V_ARGS(intersectionBound->GetBoundingBoxMin()),VEC3V_ARGS(intersectionBound->GetBoundingBoxMax()),
															VEC3V_ARGS(compositeBound->GetLocalBoxMins(componentIndex)),VEC3V_ARGS(compositeBound->GetLocalBoxMaxs(componentIndex)));

			validBound &= componentBoundingBoxContainsSupports && compositeBoundingBoxContainsComponent && componentMatrixIsOrthonormal && componentBoundingBoxMatchesComposite;
		}
		else
		{
			bool boundingBoxContainsSupports = bound.DoesBoundingBoxContainsSupports();
			Assertf(boundingBoxContainsSupports,	"Bounding box found that does not contain bound support points."
													"\n\tArchetype Filename: %s"
													"\n\tBound Type: %s",
													archetypeFilename, GetBoundTypeString(bound));

			validBound &= boundingBoxContainsSupports;
		}

		// If the bounding box wasn't correct, don't bother outputting the shapetest information
		if(validBound)
		{
			g_CurrentIndentation = 0;
			const int MAX_STRING_LENGTH = 512;
			char boundTypeBuffer[MAX_STRING_LENGTH];
			char shapeTypeBuffer[MAX_STRING_LENGTH];
			FillBoundTypeBuffer(boundTypeBuffer, &bound, componentIndex, partIndex);
			FillShapeTypeBuffer(shapeTypeBuffer, shapeType, batched);

			Displayf("Invalid ShapeTest Result detected!");
			if(tooFarFromBound)
			{
				Displayf("%sIntersection position is too far from the bound.", GetTabString());
			}
			if(tooFarFromShape)
			{
				Displayf("%sIntersection position is too far from the shape.", GetTabString());
			}
			if(nonNormalNormal)
			{
				Displayf("%sIntersection normal isn't normalized.",GetTabString());
			}
			if(isNonFinite)
			{
				Displayf("%sIntersection has non-finite values.",GetTabString());
			}
			IncrementCurrentIndendation();
			Displayf("%sBetween %s and %s", GetTabString(), shapeTypeBuffer, boundTypeBuffer);
			Displayf("%sArchetype Filename: %s", GetTabString(), archetypeFilename);
			DisplayFloat("Distance from bound", distanceFromBound);
			DisplayFloat("Tolerated distance from bound", toleratedDistanceFromBound);
			DisplayFloat("Distance from shape", distanceFromShape);
			DisplayFloat("Tolerated distance from shape", toleratedDistanceFromShape);
			DisplayVector("Intersection Position", position);
			DisplayNormal("Intersection Normal", intersection.GetNormal());
			DisplayNormal("Intersection Poly Normal", intersection.GetIntersectedPolyNormal());
			DisplayFloat("Intersection Depth", intersection.GetDepth());
			DisplayFloat("Intersection T Value", intersection.GetT());
			DisplayUnsignedInteger("Intersection Material Id", intersection.GetMaterialId());
			DisplayMatrix("Instance Matrix",worldFrominstance);
			Displayf("%sBound Description:", GetTabString());
			IncrementCurrentIndendation();
			DisplayBoundDescription(&bound, componentIndex, partIndex);
			DecrementCurrentIndendation();
			Displayf("%sShape Description:", GetTabString());
			IncrementCurrentIndendation();
			DisplayShapeDescription(shapeType,instanceFromWorld);
			DecrementCurrentIndendation();
			DecrementCurrentIndendation();
			
			Assertf(!invalidShapeTestResultsDetected, "\n%s - %s\nSee log for more details.", shapeTypeBuffer, boundTypeBuffer);
		}
	}
}

template <typename ShapeType>
void ValidateIntersections(const ShapeType& shapeType, bool batched = false)
{
	if(g_NumInvalidResults < MAXIMUM_INVALID_RESULTS || (MAXIMUM_INVALID_RESULTS < 0))
	{
		phIntersection* intersections;
		int numIntersections;
		shapeType.GetIntersections(intersections, numIntersections);

		// Validate the new intersections found by this shapetest
		for (int intersectionIndex=0; intersectionIndex<numIntersections; ++intersectionIndex)
		{
			ValidateIntersection(intersections[intersectionIndex], shapeType, batched);
		}
	}
}

template <> void ValidateIntersections<phShapeBatch>(const phShapeBatch& shapeBatch, bool UNUSED_PARAM(batched))
{
	// Validate the intersections of each shape in the batch test
	for(int probeIndex = 0; probeIndex < shapeBatch.GetNumProbes(); ++probeIndex)
	{
		ValidateIntersections(shapeBatch.GetProbe(probeIndex), true);
	}

	for(int sphereIndex = 0; sphereIndex < shapeBatch.GetNumSpheres(); ++sphereIndex)
	{
		ValidateIntersections(shapeBatch.GetSphere(sphereIndex), true);
	}

	for(int capsuleIndex = 0; capsuleIndex < shapeBatch.GetNumCapsules(); ++capsuleIndex)
	{
		ValidateIntersections(shapeBatch.GetCapsule(capsuleIndex), true);
	}

	for(int sweptSphereIndex = 0; sweptSphereIndex < shapeBatch.GetNumSweptSpheres(); ++sweptSphereIndex)
	{
		ValidateIntersections(shapeBatch.GetSweptSphere(sweptSphereIndex), true);
	}
}
} // namespace ShapetestValidation
#endif // VALIDATE_SHAPETEST_RESULTS

#if !__SPU
void TestInLevelTask (sysTaskParameters& taskParams)
{
#if __XENON && !__FINAL
	PIXBeginNamedEvent(Color_yellow.GetColor(), "ShapeTest:TestInLevelTask");
#endif

	phShapeTestTaskData& taskData = *static_cast<phShapeTestTaskData*>(taskParams.Input.Data);
	if (taskData.m_NumProbes>0)
	{
		Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_ProbeShapeTest,"Multithreaded probe shape test needs more task data. Current %" SIZETFMT "i, Required %i", taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_ProbeShapeTest + 1);
		phShapeTest<phShapeProbe>* probeTest = static_cast<phShapeTest<phShapeProbe>*>(taskParams.UserData[phShapeTestTaskData::ParamIndex_ProbeShapeTest].asPtr);
		for (int probeIndex=0; probeIndex<taskData.m_NumProbes; probeIndex++)
		{
			probeTest[probeIndex].TestInLevel();
		}
	}

	if (taskData.m_NumEdges>0)
	{
		Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_EdgeShapeTest,"Multithreaded edge shape test needs more task data. Current %" SIZETFMT "i, Required %i", taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_EdgeShapeTest + 1);
		phShapeTest<phShapeEdge>* edgeTest = static_cast<phShapeTest<phShapeEdge>*>(taskParams.UserData[phShapeTestTaskData::ParamIndex_EdgeShapeTest].asPtr);
		for (int edgeIndex=0; edgeIndex<taskData.m_NumEdges; edgeIndex++)
		{
			edgeTest[edgeIndex].TestInLevel();
		}
	}

	if (taskData.m_NumSpheres>0)
	{
		Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_SphereShapeTest,"Multithreaded sphere shape test needs more task data. Current %" SIZETFMT "i, Required %i", taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_SphereShapeTest + 1);
		phShapeTest<phShapeSphere>* sphereTest = static_cast<phShapeTest<phShapeSphere>*>(taskParams.UserData[phShapeTestTaskData::ParamIndex_SphereShapeTest].asPtr);
		for (int sphereIndex=0; sphereIndex<taskData.m_NumSpheres; sphereIndex++)
		{
			sphereTest[sphereIndex].TestInLevel();
		}
	}

	if (taskData.m_NumCapsules>0)
	{
		Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_CapsuleShapeTest,"Multithreaded capsule shape test needs more task data. Current %" SIZETFMT "i, Required %i", taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_CapsuleShapeTest + 1);
		phShapeTest<phShapeCapsule>* capsuleTest = static_cast<phShapeTest<phShapeCapsule>*>(taskParams.UserData[phShapeTestTaskData::ParamIndex_CapsuleShapeTest].asPtr);
		for (int capsuleIndex=0; capsuleIndex<taskData.m_NumCapsules; capsuleIndex++)
		{
			capsuleTest[capsuleIndex].TestInLevel();
		}
	}

	if (taskData.m_NumSweptSpheres>0)
	{
		Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_SweptSphereShapeTest,"Multithreaded swept sphere shape test needs more task data. Current %" SIZETFMT "i, Required %i", taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_SweptSphereShapeTest + 1);
		phShapeTest<phShapeSweptSphere>* sweptSphereTest = static_cast<phShapeTest<phShapeSweptSphere>*>(taskParams.UserData[phShapeTestTaskData::ParamIndex_SweptSphereShapeTest].asPtr);
		for (int sweptSphereIndex=0; sweptSphereIndex<taskData.m_NumSweptSpheres; sweptSphereIndex++)
		{
			sweptSphereTest[sweptSphereIndex].TestInLevel();
		}
	}

	if (taskData.m_NumBatches>0)
	{
		Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_BatchShapeTest,"Multithreaded batch shape test needs more task data. Current %" SIZETFMT "i, Required %i", taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_BatchShapeTest + 1);
		phShapeTest<phShapeBatch>* batchTest = static_cast<phShapeTest<phShapeBatch>*>(taskParams.UserData[phShapeTestTaskData::ParamIndex_BatchShapeTest].asPtr);
		for (int batchIndex=0; batchIndex<taskData.m_NumBatches; batchIndex++)
		{
			batchTest[batchIndex].TestInLevel();
		}
	}

#if __XENON && !__FINAL
	PIXEndNamedEvent();
#endif
}
#endif

template <class ShapeType> bool TestGeometryBound(const typename ShapeType::LocalizedData& localizedData, ShapeType& shape, const phBoundGeometry& geomBound, Vec3V_ConstPtr vertices, u32 boundTypeFlags, u32 boundIncludeFlags, const phShapeBase::PartialIntersection& boundPartialIntersection)
{
	bool wasBoundHit = false;
	phShapeBase::PrimitivePartialIntersection polygonPartialIntersection = boundPartialIntersection;
	u16 numPolygons = (u16)geomBound.GetNumPolygons();
	for (phPolygon::Index polygonIndex=0; polygonIndex<numPolygons; polygonIndex++)
	{
		const phPolygon& polygon = geomBound.GetPolygon(polygonIndex);
		if(!shape.RejectPolygonFromGeometryBoundIteration(localizedData,polygon,RCC_VECTOR3(*vertices)))
		{
			phMaterialMgr::Id materialId = geomBound.phBoundGeometry::GetMaterialId(geomBound.phBoundGeometry::GetPolygonMaterialIndex(polygonIndex));
			polygonPartialIntersection.SetPrimitive(polygonIndex,materialId);
			wasBoundHit |= shape.TestPolygon(localizedData,polygon,RCC_VECTOR3(*vertices),boundTypeFlags,boundIncludeFlags,geomBound,polygonPartialIntersection);
		}
	}
	return wasBoundHit;
}

template <> bool TestGeometryBound(const phShapeBatch::LocalizedData& localizedData, phShapeBatch& batchShape, const phBoundGeometry& geomBound, Vec3V_ConstPtr vertices, u32 boundTypeFlags, u32 boundIncludeFlags, const phShapeBase::PartialIntersection& boundPartialIntersection)
{
	// Batch shapetests get their own implementation so they can iterate "for each shape->for each polygon" instead of "for each polygon->for each shape"
	// This allows us to cull the entire geometry bound if the sub-shape doesn't even come close. We could do this for primitive bounds as well but they're
	//   nowhere near as expensive as testing geometry bounds per-polygon and aren't as common so I don't think the benefit would be as great. 
	return batchShape.TestGeometryBound(localizedData,geomBound,vertices,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
}


void phShapeBase::Init (phIntersection* intersection, int numIntersections)
{
	SetIntersections(intersection,numIntersections);
	m_IgnoreMaterialFlags = 0;
#if __DEV
	if(m_Intersection)
	{
		sysMemSet(m_Intersection,0xCD,sizeof(phIntersection)*m_MaxNumIsects);
	}
#endif // __DEV
}


void phShapeBase::SetIntersections (phIntersection* intersection, int numIntersections, int numHits)
{
	if (!intersection)
	{
		// No intersections were passed in, so make the number of intersections 0. Setting it here allows the default number of intersections to be 1, so that shape test
		// initialization can be done with just an intersection pointer.
		numIntersections = 0;
	}

	m_Intersection = intersection;
	Assertf(numIntersections<=MAX_SHAPE_TEST_ISECTS,"Ignorable - the maximum number of intersections for a shape test is %i but %i were requested.",MAX_SHAPE_TEST_ISECTS,numIntersections);

	// Temporary: BAD_INDEX used to be the default numIntersections, which meant return only the best intersection. Now only the best intersections are always returned,
	// so the default is 1. If anybody is passing in -1 or 65535, change it to 1.
	if (numIntersections==-1 || numIntersections==(u16)BAD_INDEX)
	{
		numIntersections = 1;
	}

	m_MaxNumIsects = Clamp(numIntersections,0,MAX_SHAPE_TEST_ISECTS);
	m_NumHits = numHits;
}

void phShapeBase::GetIntersections (phIntersection*& outIntersections, int& outMaxNumIntersections) const
{
	outIntersections = m_Intersection;
	outMaxNumIntersections = m_MaxNumIsects;
}

void phShapeBase::Reset ()
{
	m_NumHits = 0;
	m_LargestDepth = -FLT_MAX;
	m_SmallestDepth = FLT_MAX;
	m_RetestIntersection = NULL;
	if (m_Intersection)
	{
		// Reset all the intersections.
		for (int isectIndex=0; isectIndex<m_MaxNumIsects; isectIndex++)
		{
			m_Intersection[isectIndex].Zero();
		}
	}
}


phIntersection* phShapeBase::GetNextIntersection (ScalarV_In depth, ScalarV_In depthLimit)
{
	// If the user is calling GetNextIntersection directly then they need to make sure the depth isn't negative
	Assert(IsGreaterThanAll(depth,ScalarV(V_ZERO)));

	if (IsBooleanTest())
	{
		m_NumHits = 1;
		return NULL;
	}

	if (m_RetestIntersection)
	{
		return (phIntersection*)m_RetestIntersection;
	}

	if (m_Intersection)
	{
		// Make the effective depth negative if it is greater than the depth limit.
		ScalarV clampedDepth = SelectFT( IsLessThanOrEqual( depth, depthLimit), Subtract(depthLimit,depth), depth );		// keep in mind that vectorized version has the TRUE second in the arguments
		const float depthF = clampedDepth.Getf();

		// See if there are unused intersections.
		if (m_NumHits < m_MaxNumIsects)
		{
			// There is at least one unused intersection, so use the first one.
			phIntersection* intersection = &m_Intersection[m_NumHits++];

			// Set the new largest depth, if this depth is the largest.
			m_LargestDepth = Max( depthF, m_LargestDepth );

			// Set the new smallest depth, if this depth is the smallest.
			m_SmallestDepth = Min( depthF, m_SmallestDepth );

			// Return the first unused intersection.
			return (phIntersection*)intersection;
		}

		// All the intersections have been used, so see if the depth is larger than the previous smallest.
		if( IsLessThanOrEqualAll( clampedDepth, ScalarV(ScalarVFromF32(m_SmallestDepth) ) ) !=0 )
		{
			// There are no unused intersections, and this depth is smaller than the previous smallest, so return NULL for no available intersection.
			return NULL;
		}

		// Find the worst intersection to write over it.
		ScalarV smallestDepth(V_FLT_MAX);
		ScalarV nextSmallestDepth = ScalarVFromF32( m_LargestDepth );
		int smallestDepthIndex = 0;
		int isectIndex;
		for (isectIndex=0; isectIndex<m_MaxNumIsects; isectIndex++)
		{
			// Get the depth of the previously used intersection and compare it with the smallest depth found so far.
			ScalarV isectDepth = ScalarVFromF32( m_Intersection[isectIndex].GetDepth() );

			// Make the effective depth negative if it is greater than the depth limit.
			isectDepth = SelectFT( IsLessThanOrEqual( isectDepth, depthLimit), Subtract( depthLimit, isectDepth ), isectDepth );

			// See if this is the smallest effective depth so far.
			if( IsLessThanAll(isectDepth,smallestDepth) != 0 )
			{
				// This is the smallest depth intersection found so far, so set the second smallest to the previous smallest, set the new smallest and the index.
				nextSmallestDepth	= smallestDepth;
				smallestDepth		= isectDepth;
				smallestDepthIndex	= isectIndex;
			}
			else if( IsLessThanAll(isectDepth, nextSmallestDepth) != 0 )
			{
				// This isn't the smallest depth found so far, but it is smaller than the second smallest, so make this the second smallest.
				nextSmallestDepth = isectDepth;
			}
		}

		// Set the new largest depth, if this is it.
		if( isectIndex>1 )
		{
			m_SmallestDepth = nextSmallestDepth.Getf();
		}
		else
		{
			m_LargestDepth = depthF;
			m_SmallestDepth = depthF;
		}

		// Return the intersection that previously had the smallest depth.
		return (phIntersection*)&m_Intersection[smallestDepthIndex];
	}
	return NULL;
}

void phShapeBase::SetIntersection(phIntersection& intersection, Vec3V_In localPosition, Vec3V_In localNormal, ScalarV_In depth, ScalarV_In tValue, phMaterialMgr::Id materialId, const PartialIntersection& partialIntersection) const
{
	Assert(IsIntersectionValid(depth,materialId));
	Mat34V_ConstRef worldFromLocal = partialIntersection.GetWorldFromLocal();
	intersection.Set(	partialIntersection.GetHandle().GetLevelIndex(),
						partialIntersection.GetHandle().GetGenerationId(),
						Transform(worldFromLocal,localPosition),
						Transform3x3(worldFromLocal,localNormal),
						tValue,
						depth,
						partialIntersection.GetPrimitiveIndex(),
						partialIntersection.GetComponentIndex(),
						materialId);
}
void phShapeBase::SetPolygonIntersection(phIntersection& intersection, Vec3V_In localPosition, Vec3V_In localNormal, Vec3V_In localPolygonNormal, ScalarV_In depth, ScalarV_In tValue, const PrimitivePartialIntersection& partialIntersection) const
{
	Assert(IsIntersectionValid(depth,partialIntersection));
	Mat34V_ConstRef worldFromLocal = partialIntersection.GetWorldFromLocal();
	intersection.Set(	partialIntersection.GetHandle().GetLevelIndex(),
						partialIntersection.GetHandle().GetGenerationId(),
						Transform(worldFromLocal,localPosition),
						Transform3x3(worldFromLocal,localNormal),
						tValue,
						depth,
						partialIntersection.GetPrimitiveIndex(),
						partialIntersection.GetComponentIndex(),
						partialIntersection.GetMaterialId());

	// TOOD: Add phIntersetion::Set that takes a polygon normal
	intersection.SetIntersectedPolyNormal(Transform3x3(worldFromLocal,localPolygonNormal));
}

bool phShapeBase::AddIntersection(Vec3V_In localPosition, Vec3V_In localNormal, ScalarV_In depth, ScalarV_In tValue, phMaterialMgr::Id materialId, const PartialIntersection& partialIntersection, ScalarV_In depthLimit)
{
	if(IsIntersectionValid(depth,materialId))
	{
		if(phIntersection* intersection = GetNextIntersection(depth,depthLimit))
		{
			SetIntersection(*intersection,localPosition,localNormal,depth,tValue,materialId,partialIntersection);
		}
		return true;
	}
	return false;
}

bool phShapeBase::AddPolygonIntersection(Vec3V_In localPosition, Vec3V_In localNormal, Vec3V_In localPolygonNormal, ScalarV_In depth, ScalarV_In tValue, const PrimitivePartialIntersection& partialIntersection, ScalarV_In depthLimit)
{
	if(IsIntersectionValid(depth,partialIntersection.GetMaterialId()))
	{
		if(phIntersection* intersection = GetNextIntersection(depth,depthLimit))
		{
			SetPolygonIntersection(*intersection,localPosition,localNormal,localPolygonNormal,depth,tValue,partialIntersection);
		}
		return true;
	}
	return false;
}

phIntersection* phShapeBase::GetNextRetestIntersection ()
{
	if (m_RetestIntersection)
	{
		// Find and return the next intersection to retest.
		if (m_RetestIntersection-m_Intersection<m_MaxNumIsects-1)
		{
			m_RetestIntersection++;
		}
		else
		{
			m_RetestIntersection = NULL;
		}
	}
	else
	{
		m_RetestIntersection = m_Intersection;
	}

	return m_RetestIntersection;
}

#if __DEBUGLOG
void phShapeBase::DebugReplay () const
{
	diagDebugLog(diagDebugLogPhysics, 'SBNH',&m_NumHits);
	for (int i=0; i<m_NumHits; i++)
	{
		m_Intersection[i].DebugReplay();
	}
}
#endif


void phShapeComposite::PopulateCullerFromUserProvidedCullShape(const LocalizedData& localizedData, phBoundCuller& culler, const phOptimizedBvh &bvhStructure)
{
	Assert(m_UserProvidedCullShape_WS != NULL);
	const phCullShape::phCullType cullType = m_UserProvidedCullShape_WS->GetCullType();
	switch(cullType)
	{
	case phCullShape::PHCULLTYPE_SPHERE:
		{
			const phCullShape::phCullData_Sphere &cullData = localizedData.m_UserProvidedCullShape.GetSphereData();
			const Vec3V localCenter = cullData.m_Center;
			const ScalarV radius = cullData.m_Radius;

			phBvhAabbOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
			const Vec3V aabbMin = Subtract(localCenter, Vec3V(radius));
			const Vec3V aabbMax = Add(localCenter, Vec3V(radius));
			nodeOverlapCallback.SetAABB(RCC_VECTOR3(aabbMin), RCC_VECTOR3(aabbMax));
			bvhStructure.walkStacklessTree(&nodeOverlapCallback);

			break;
		}
	case phCullShape::PHCULLTYPE_LINESEGMENT:
		{
			const phCullShape::phCullData_LineSegment &cullData = localizedData.m_UserProvidedCullShape.GetLineSegmentData();
			const Vec3V end0 = cullData.m_P0;
			const Vec3V end1 = cullData.m_P1;

			phBvhLineSegmentOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
			nodeOverlapCallback.SetSegment(end0, end1);
			bvhStructure.walkStacklessTree(&nodeOverlapCallback);

			break;
		}
	case phCullShape::PHCULLTYPE_CAPSULE:
		{
			const phCullShape::phCullData_Capsule &cullData = localizedData.m_UserProvidedCullShape.GetCapsuleData();
			const Vec3V end0 = cullData.m_P0;
			const Vec3V end1 = AddScaled(end0, cullData.m_ShaftAxis, cullData.m_ShaftLength);

			phBvhCapsuleOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
			nodeOverlapCallback.SetCapsule(end0, end1, cullData.m_Radius);
			bvhStructure.walkStacklessTree(&nodeOverlapCallback);

			break;
		}
	case phCullShape::PHCULLTYPE_BOX:
		{
			const phCullShape::phCullData_Box &cullData = localizedData.m_UserProvidedCullShape.GetBoxData();
			const Mat34V obbMatrix = cullData.m_BoxAxes;
			const Vec3V obbHalfSize = cullData.m_BoxHalfSize;
			const Vec3V aabbHalfSize = geomBoxes::ComputeAABBExtentsFromOBB(obbMatrix.GetMat33(), obbHalfSize);
			const Vec3V aabbCenter = obbMatrix.GetCol3();
			const Vec3V aabbMin = Subtract(aabbCenter, aabbHalfSize);
			const Vec3V aabbMax = Add(aabbCenter, aabbHalfSize);

			phBvhAabbOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
			nodeOverlapCallback.SetAABB(RCC_VECTOR3(aabbMin), RCC_VECTOR3(aabbMax));
			bvhStructure.walkStacklessTree(&nodeOverlapCallback);

			break;
		}
	default:
		{
			Assertf(false, "Unsupported user-provided cull type: %d", cullType);
			break;
		}
	}
}

void phShapeProbe::InitProbe (const phSegmentV& worldProbe, phIntersection* intersection, int numIntersections)
{
	Assertf(IsLessThanAll(Mag(worldProbe.GetStart()),ScalarV(V_FLT_LARGE_8)),"InitProbe has an impractical segment starting point %f, %f, %f",worldProbe.GetStart().GetElemf(0),worldProbe.GetStart().GetElemf(1),worldProbe.GetStart().GetElemf(2));
	Assertf(IsLessThanAll(Mag(worldProbe.GetEnd()),ScalarV(V_FLT_LARGE_8)),"InitProbe has an impractical segment ending point %f, %f, %f",worldProbe.GetEnd().GetElemf(0),worldProbe.GetEnd().GetElemf(1),worldProbe.GetEnd().GetElemf(2));
	m_WorldSegment.Set(worldProbe.GetStart(),worldProbe.GetEnd());
	phShapeBase::Init(intersection,numIntersections);
	DEBUGLOG_ONLY(DebugReplay());
}

void phShapeProbe::SetupIteratorCull (phIterator& levelIterator)
{
	levelIterator.InitCull_LineSegment(m_WorldSegment.GetStart(),m_WorldSegment.GetEnd());
}

// CMT - 1/27/11 - Removed the segment/sphere test that would only happen when reaching max intersections, 
// instead implement a segment/AABB test that will always execute. Segment/AABB appears to be more efficient 
// at culling unwanted bounds than segment/sphere, and we always want this to execute because this is called 
// recursively from TestBound when encountering composite bounds.
bool phShapeProbe::RejectBound (const LocalizedData& localizedData, const phBound& bound) const
{
	// Get the box center and half-widths.
	Vec3V vecBoxHalfWidth, vecBoxCenter;
	bound.GetBoundingBoxHalfWidthAndCenter(vecBoxHalfWidth,vecBoxCenter);

	// Do a quick reject test by testing the line segment to the phBound's AABB. 
	// Transform the segment start and end to the bound's local coordinates.
	Vec3V segmentStart = Subtract(localizedData.m_Segment.GetStart(),vecBoxCenter);
	Vec3V segmentEnd = Subtract(localizedData.m_Segment.GetEnd(),vecBoxCenter);

	VecBoolV result = geomBoxes::TestSegmentToCenteredAlignedBox(segmentStart,segmentEnd,vecBoxHalfWidth);
	return IsEqualIntAll( result, VecBoolV(V_F_F_F_F) ) != 0;
}


bool phShapeProbe::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 UNUSED_PARAM(primTypeFlags), u32 UNUSED_PARAM(primIncludeFlags), const PartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TestBoundPrimitive);
#endif

	// Compute the segment endpoints relative to each other and to the bound centroid.
	Vec3V centroidOffset = bound.GetCentroidOffset();
	Vec3V segmentAtoB = Subtract(localizedData.m_Segment.GetEnd(),localizedData.m_Segment.GetStart());
	ScalarV invSegmentLengthSquared = InvMagSquared(segmentAtoB);
	ScalarV invSegmentLength = Sqrt(invSegmentLengthSquared);
	ScalarV AtoCentroidT = Scale(Dot(Subtract(centroidOffset, localizedData.m_Segment.GetStart()), segmentAtoB), invSegmentLengthSquared);
	ScalarV boundRadiusT = Scale(Add(ScalarVFromF32(bound.GetRadiusAroundCentroid()), ScalarV(V_FLT_SMALL_2)), invSegmentLength);

	// Variables only used by swept tests
	bool useTestNormals = false;
	Vec3V testNormal = Vec3V(V_ZERO);

	// Create a new segment, clamped by the sphere of the bound. This prevents numerical inaccuracy.
	ScalarV clampedT1 = Max(ScalarV(V_ZERO), Subtract(AtoCentroidT, boundRadiusT));
	ScalarV clampedT2 = Min(ScalarV(V_ONE), Add(AtoCentroidT, boundRadiusT));

	// If the centroid sphere only can only intersect the probe outside of the begin/end, there is no collision
	if(Or(IsGreaterThan(clampedT1, ScalarV(V_ONE)), IsLessThan(clampedT2, ScalarV(V_ZERO))).Getb())
	{
		return false;
	}

	Vec3V clampedLocalA = Add(localizedData.m_Segment.GetStart(), Scale(clampedT1, segmentAtoB));
	Vec3V clampedLocalB = Add(localizedData.m_Segment.GetStart(), Scale(clampedT2, segmentAtoB));
	Vec3V clampedSegmentAtoB = Subtract(clampedLocalB, clampedLocalA);

	// Compute the intersections of the segment with the bound.
	ScalarV segmentT = ScalarV(V_ZERO);
	int partIndex=0,numHits=0;
	const int boundType = bound.GetType();
	switch (boundType)
	{
		case phBound::SPHERE:
		{
			partIndex = 0;
			ScalarV radius = bound.GetRadiusAroundCentroidV();
			ScalarV radius2 = Scale(radius,radius);
			ScalarV tempT2;
			numHits = geomSegments::SegmentToSphereIntersections(Subtract(clampedLocalA,centroidOffset),clampedSegmentAtoB,radius2,segmentT,tempT2,true);
			break;
		}

		case phBound::CAPSULE:
		{
			const phBoundCapsule& capsuleBound = *static_cast<const phBoundCapsule*>(&bound);
			ScalarV capsuleRadius = capsuleBound.GetRadiusV();
			ScalarV capsuleLength = capsuleBound.GetLengthV(capsuleRadius);
			ScalarV capsuleT1,capsuleT2;
			int tempPartIndex2;
			ScalarV tempT2;
			numHits = phBoundCapsule::SegmentToCapsuleIntersections(Subtract(clampedLocalA,centroidOffset),Subtract(clampedLocalB,centroidOffset),capsuleLength,capsuleRadius,segmentT,tempT2,capsuleT1,capsuleT2,partIndex,tempPartIndex2);
			break;
		}

		case phBound::BOX:
		{
			const phBoundBox& boxBound = *static_cast<const phBoundBox*>(&bound);
			Vec3V boxHalfWidths = Scale(boxBound.GetBoxSize(),ScalarV(V_HALF));
			Vec3V normal1;
			numHits = geomBoxes::TestSegmentToBox(Subtract(clampedLocalA,centroidOffset),clampedSegmentAtoB,boxHalfWidths,&segmentT,&normal1,&partIndex,NULL,NULL,NULL);
			break;
		}

		case phBound::CYLINDER:
		{
			const phBoundCylinder& cylinderBound = static_cast<const phBoundCylinder&>(bound);
			useTestNormals = true;
			ScalarV tempT2;
			Vec3V tempNormal2;
			numHits = cylinderBound.TestAgainstSegment(Subtract(clampedLocalA,centroidOffset), clampedSegmentAtoB, segmentT, testNormal, tempT2, tempNormal2);
			break;
		}

		case phBound::DISC:
		USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
		case phBound::GEOMETRY:
		{
			useTestNormals = true;

			IterativeCastResult result;
			if(IterativeCast(CastPointInput(clampedLocalA,clampedSegmentAtoB),bound,result) && IsGreaterThanAll(result.m_IntersectionTime,ScalarV(V_ZERO)))
			{
				numHits = 1;
				partIndex = 0;
				segmentT = result.m_IntersectionTime;
				testNormal = result.m_NormalOnLocalBound;
			}

			break;
		}

		// TODO: Implement closed form solution
		/*case phBound::DISC:
		{
			Assert(0);
			curvedGeomNormal1 = Vec3V(V_ZERO);
			break;
		}*/

		default:
		{
			AssertMsg(0 , "invalid bound type");
		}
	}

	// Quit if the probe did not hit the given primitive bound.
	if (numHits==0)
	{
		return false;
	}

	// Get the T-Values relative to the original segment
	ScalarV clampedDeltaT = Subtract(clampedT2, clampedT1);
	segmentT = Add(clampedT1, Scale(segmentT, clampedDeltaT));


	// There is at least one intersection with the bound, and intersections are given to fill in.
	// Find the normal vector (outward from the bound surface) of the first intersection to see if it is entering or exiting.
	Vec3V hitPosition = AddScaled(localizedData.m_Segment.GetStart(),segmentAtoB,segmentT);
	Vec3V normal;
	if (useTestNormals)
	{
		normal = testNormal;
	}
	else
	{
		normal = ComputeProbeIsectNormal(bound,hitPosition,centroidOffset,partIndex);
	}

	if (IsLessThanOrEqualAll(Dot(normal,segmentAtoB),ScalarV(V_ZERO)))
	{
		ScalarV depth = Dist(hitPosition,localizedData.m_Segment.GetEnd());
		phMaterialMgr::Id materialId = bound.GetMaterialId(partIndex);
		return AddIntersection(hitPosition,normal,depth,segmentT,materialId,partialIntersection);
	}
		
	return false;
}


Vec3V_Out phShapeProbe::ComputeProbeIsectNormal (const phBound& bound, Vec3V_In hitPosition, Vec3V_In centroidOffset, int elementIndex)
{
	switch (bound.GetType())
	{
		case phBound::SPHERE:
		USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
		case phBound::DISC:
		case phBound::CYLINDER:
		case phBound::GEOMETRY:
		{
			// Geometry bounds are included here because curved geometry bounds can be treated as spheres when no flat faces are hit.
			Vec3V normal = Subtract(hitPosition,centroidOffset);
			normal = Normalize(normal);
			return normal;
		}

		case phBound::CAPSULE:
		{
			const phBoundCapsule& capsuleBound = *static_cast<const phBoundCapsule*>(&bound);
			return VECTOR3_TO_VEC3V(capsuleBound.FindCapsuleIsectNormal(VEC3V_TO_VECTOR3(Subtract(hitPosition,centroidOffset)),elementIndex));
		}

		case phBound::BOX:
		{
			return geomBoxes::GetFaceNormal(elementIndex);
		}

		default:
		{
			AssertMsg(0 , "invalid bound type");
			return Vec3V(V_ZERO);
		}
	}
}

bool phShapeProbe::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices,u32 UNUSED_PARAM(polyTypeFlags), u32 UNUSED_PARAM(polyIncludeFlags), const phBoundGeometry& UNUSED_PARAM(geomBound), const PrimitivePartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(ProbeTestPolygon);
#endif
	const Vec3V segmentStart = localizedData.m_Segment.GetStart();
	const Vec3V segmentEnd = localizedData.m_Segment.GetEnd();
	const Vec3V segmentStartToEnd = Subtract(segmentEnd,segmentStart);

	const Vec3V* RESTRICT vertices = &RCC_VEC3V(boundVertices);
	const Vec3V vertex0 = vertices[polygon.GetVertexIndex(0)];
	const Vec3V vertex1 = vertices[polygon.GetVertexIndex(1)];
	const Vec3V vertex2 = vertices[polygon.GetVertexIndex(2)];
#if __SPU
	// Computing the normal with the area on SPU leads to noticeable precision errors
	const Vec3V triangleNormal = NormalizeSafe(polygon.ComputeNonUnitNormal(vertex0,vertex1,vertex2),Vec3V(V_X_AXIS_WZERO));
#else // __SPU
	const Vec3V triangleNormal = polygon.ComputeUnitNormal(vertex0,vertex1,vertex2);
#endif // __SPU

	ScalarV fractionAlongSegment;
	if(geomSegments::SegmentTriangleIntersectDirected(segmentStart,segmentStartToEnd,segmentEnd,triangleNormal,vertex0,vertex1,vertex2,fractionAlongSegment))
	{
		const Vec3V positionOnTriangle = AddScaled(segmentStart,segmentStartToEnd,fractionAlongSegment);
		const ScalarV depth = Dist(positionOnTriangle,segmentEnd);
		return AddPolygonIntersection(positionOnTriangle,triangleNormal,triangleNormal,depth,fractionAlongSegment,partialIntersection);
	}
	return false;
}


// This function is culling against a BVH structure.  Note that it doesn't do any of the expanding the cull shape for trying to handle second surface
//   as this function is use for all BVHs which may not necessarily have come from a phBoundBVH.
void phShapeProbe::PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler)
{
	// Cull against the line segment of the probe.
	phBvhLineSegmentOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
	nodeOverlapCallback.SetSegment(localizedData.m_Segment.GetStart(), localizedData.m_Segment.GetEnd());
	bvhStructure.walkStacklessTree(&nodeOverlapCallback);
}


#if __PFDRAW
void phShapeProbe::DrawShape (bool PF_DRAW_ONLY(retest))
{
	// Draw the test probe, if ProbeSegments is turned on.
	PFD_ProbeSegments.DrawArrow(VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()),VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()),retest?Color_aquamarine1:Color_white);
}


void phShapeProbe::DrawIntersections (bool PF_DRAW_ONLY(retest))
{
	if (m_NumHits==0)
	{
		return;
	}

	if (m_Intersection)
	{
		// Draw the intersections, if ProbeIsects is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				PFD_ProbeIsects.DrawTick(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),0.1f,retest?Color_purple1:Color_red);
			}
		}

		// Draw the normals, if ProbeNormals is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				Vector3 probeNormalEnd(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()));
				probeNormalEnd.AddScaled(RCC_VECTOR3(m_Intersection[hitIndex].GetNormal()),1.0f);
				PFD_ProbeNormals.DrawArrow(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),probeNormalEnd,retest?Color_SeaGreen1:Color_green);
			}
		}
	}
}
#endif


#if __DEBUGLOG
void phShapeProbe::DebugReplay () const
{
	Vec3V start = m_WorldSegment.GetStart();
	diagDebugLog(diagDebugLogPhysics, 'SPWA',&start);
	Vec3V end = m_WorldSegment.GetEnd();
	diagDebugLog(diagDebugLogPhysics, 'SPWB',&end);

	phShapeBase::DebugReplay();
}
#endif

void phShapeEdge::InitEdge (const phSegmentV& worldProbe, phIntersection* intersection, int numIntersections)
{
	Assertf(IsLessThanAll(Mag(worldProbe.GetStart()),ScalarV(V_FLT_LARGE_8)),"InitEdge has an impractical segment starting point %f, %f, %f",worldProbe.GetStart().GetElemf(0),worldProbe.GetStart().GetElemf(1),worldProbe.GetStart().GetElemf(2));
	Assertf(IsLessThanAll(Mag(worldProbe.GetEnd()),ScalarV(V_FLT_LARGE_8)),"InitEdge has an impractical segment ending point %f, %f, %f",worldProbe.GetEnd().GetElemf(0),worldProbe.GetEnd().GetElemf(1),worldProbe.GetEnd().GetElemf(2));
	m_WorldSegment.Set(worldProbe.GetStart(),worldProbe.GetEnd());
	phShapeBase::Init(intersection,numIntersections);
	DEBUGLOG_ONLY(DebugReplay());
}


bool phShapeEdge::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 UNUSED_PARAM(primTypeFlags), u32 UNUSED_PARAM(primIncludeFlags), const PartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TestBoundPrimitive);
#endif

	// Compute the segment endpoints relative to each other and to the bound centroid.
	Vec3V centroidOffset = bound.GetCentroidOffset();
	Vec3V segmentAtoB = Subtract(localizedData.m_Segment.GetEnd(),localizedData.m_Segment.GetStart());
	ScalarV invSegmentLengthSquared = InvMagSquared(segmentAtoB);
	ScalarV invSegmentLength = Sqrt(invSegmentLengthSquared);
	ScalarV AtoCentroidT = Scale(Dot(Subtract(centroidOffset, localizedData.m_Segment.GetStart()), segmentAtoB), invSegmentLengthSquared);
	ScalarV boundRadiusT = Scale(Add(ScalarVFromF32(bound.GetRadiusAroundCentroid()), ScalarV(V_FLT_SMALL_2)), invSegmentLength);

	// Variables only used by swept tests
	bool useTestNormals = false;
	Vec3V testNormal1 = Vec3V(V_ZERO);
	Vec3V testNormal2 = Vec3V(V_ZERO);

	// Create a new segment, clamped by the sphere of the bound. This prevents numerical inaccuracy.
	ScalarV clampedT1 = Max(ScalarV(V_ZERO), Subtract(AtoCentroidT, boundRadiusT));
	ScalarV clampedT2 = Min(ScalarV(V_ONE), Add(AtoCentroidT, boundRadiusT));

	// If the centroid sphere only can only intersect the probe outside of the begin/end, there is no collision
	if(Or(IsGreaterThan(clampedT1, ScalarV(V_ONE)), IsLessThan(clampedT2, ScalarV(V_ZERO))).Getb())
	{
		return false;
	}

	Vec3V clampedLocalA = Add(localizedData.m_Segment.GetStart(), Scale(clampedT1, segmentAtoB));
	Vec3V clampedLocalB = Add(localizedData.m_Segment.GetStart(), Scale(clampedT2, segmentAtoB));
	Vec3V clampedSegmentAtoB = Subtract(clampedLocalB, clampedLocalA);

	// Compute the intersections of the segment with the bound.
	ScalarV segmentT1 = ScalarV(V_ZERO);
	ScalarV segmentT2 = ScalarV(V_ZERO);
	int partIndex1=0,partIndex2=0,numHits=0;
	const int boundType = bound.GetType();
	switch (boundType)
	{
		case phBound::SPHERE:
		{
			partIndex1 = 0;
			partIndex2 = 0;
			ScalarV radius = bound.GetRadiusAroundCentroidV();
			ScalarV radius2 = Scale(radius,radius);
			numHits = geomSegments::SegmentToSphereIntersections(Subtract(clampedLocalA,centroidOffset),clampedSegmentAtoB,radius2,segmentT1,segmentT2,false);
			break;
		}

		case phBound::CAPSULE:
		{
			const phBoundCapsule& capsuleBound = *static_cast<const phBoundCapsule*>(&bound);
			ScalarV capsuleRadius = capsuleBound.GetRadiusV();
			ScalarV capsuleLength = capsuleBound.GetLengthV(capsuleRadius);
			ScalarV capsuleT1,capsuleT2;
			numHits = phBoundCapsule::SegmentToCapsuleIntersections(Subtract(clampedLocalA,centroidOffset),Subtract(clampedLocalB,centroidOffset),capsuleLength,capsuleRadius,segmentT1,segmentT2,capsuleT1,capsuleT2,partIndex1,partIndex2);
			break;
		}

		case phBound::BOX:
		{
			const phBoundBox& boxBound = *static_cast<const phBoundBox*>(&bound);
			Vec3V boxHalfWidths = Scale(boxBound.GetBoxSize(),ScalarV(V_HALF));
			Vec3V normal1,normal2;
			numHits = geomBoxes::TestSegmentToBox(Subtract(clampedLocalA,centroidOffset),clampedSegmentAtoB,boxHalfWidths,&segmentT1,&normal1,&partIndex1,&segmentT2,&normal2,&partIndex2);
			break;
		}

		case phBound::CYLINDER:
		{
			const phBoundCylinder& cylinderBound = static_cast<const phBoundCylinder&>(bound);
			useTestNormals = true;
			numHits = cylinderBound.TestAgainstSegment(Subtract(clampedLocalA,centroidOffset), clampedSegmentAtoB, segmentT1, testNormal1, segmentT2, testNormal2);
			break;
		}

		case phBound::DISC:
		USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
		case phBound::GEOMETRY:
		{
			useTestNormals = true;

			IterativeCastResult result;
			if(IterativeCast(CastPointInput(clampedLocalA, clampedSegmentAtoB),bound,result) && IsGreaterThanAll(result.m_IntersectionTime,ScalarV(V_ZERO)))
			{
				numHits = 1;
				partIndex1 = 0;
				segmentT1 = result.m_IntersectionTime;
				testNormal1 = result.m_NormalOnLocalBound;
			}

			if(IterativeCast(CastPointInput(clampedLocalB, Negate(clampedSegmentAtoB)),bound,result) && IsGreaterThanAll(result.m_IntersectionTime,ScalarV(V_ZERO)))
			{
				// An outgoing intersection was found by probing backwards from the end.
				numHits++;
				if (numHits==2)
				{
					partIndex2 = 0;
					segmentT2 = Subtract(ScalarV(V_ONE),result.m_IntersectionTime);
					testNormal2 = result.m_NormalOnLocalBound;
				}
				else
				{
					partIndex1 = 0;
					segmentT1 = Subtract(ScalarV(V_ONE),result.m_IntersectionTime);
					testNormal1 = result.m_NormalOnLocalBound;
				}
			}

			break;
		}

		default:
		{
			AssertMsg(0 , "invalid bound type");
		}
	}

	// Quit if the probe did not hit the given primitive bound.
	if (numHits==0)
	{
		return false;
	}

	// Get the T-Values relative to the original segment
	ScalarV clampedDeltaT = Subtract(clampedT2, clampedT1);
	segmentT1 = Add(clampedT1, Scale(segmentT1, clampedDeltaT));
	segmentT2 = Add(clampedT1, Scale(segmentT2, clampedDeltaT));

	// Get the material and see if it should be ignored.
	bool intersectionValid1 = false;
	{
		phMaterialMgr::Id materialId1 = bound.GetMaterialId(partIndex1);

		// There is at least one intersection with the bound, and intersections are given to fill in.
		// Find the normal vector (outward from the bound surface) of the first intersection to see if it is entering or exiting.
		Vec3V hitPosition = AddScaled(localizedData.m_Segment.GetStart(),segmentAtoB,segmentT1);
		Vec3V normal;
		if (useTestNormals)
		{
			normal = testNormal1;
		}
		else
		{
			normal = ComputeProbeIsectNormal(bound,hitPosition,centroidOffset,partIndex1);
		}

		ScalarV depth = Dist(hitPosition,localizedData.m_Segment.GetEnd());

		intersectionValid1 = AddIntersection(hitPosition,normal,depth,segmentT1,materialId1,partialIntersection);
	}

	if (numHits==1)
	{
		// This probe is directed, or there was only one hit, so don't look for another hit.
		return intersectionValid1;
	}

	bool intersectionValid2 = false;
	{
		phMaterialMgr::Id materialId2 = bound.GetMaterialId(partIndex2);

		// This undirected probe has two intersections with the given bound.
		// Find the normal vector (outward from the bound surface) of the second intersection to see if it is entering or exiting.
		Vec3V hitPosition = AddScaled(localizedData.m_Segment.GetStart(),segmentAtoB,segmentT2);
		Vec3V normal;
		if (useTestNormals)
		{
			normal = testNormal2;
		}
		else
		{		
			normal = ComputeProbeIsectNormal(bound,hitPosition,centroidOffset,partIndex2);
		}
		ScalarV depth = Dist(hitPosition,localizedData.m_Segment.GetEnd());

		intersectionValid2 = AddIntersection(hitPosition,normal,depth,segmentT2,materialId2,partialIntersection);
	}

	// Return 1 if there was an intersection or a pair of intersections found between this probe and the given primitive bound, 0 if not.
	return intersectionValid1 || intersectionValid2;
}

bool phShapeEdge::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 UNUSED_PARAM(polyTypeFlags), u32 UNUSED_PARAM(polyIncludeFlags), const phBoundGeometry& UNUSED_PARAM(geomBound), const PrimitivePartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(EdgeTestPolygon);
#endif
	const Vec3V segmentStart = localizedData.m_Segment.GetStart();
	const Vec3V segmentEnd = localizedData.m_Segment.GetEnd();
	const Vec3V segmentStartToEnd = Subtract(segmentEnd,segmentStart);

	const Vec3V* RESTRICT vertices = &RCC_VEC3V(boundVertices);
	const Vec3V vertex0 = vertices[polygon.GetVertexIndex(0)];
	const Vec3V vertex1 = vertices[polygon.GetVertexIndex(1)];
	const Vec3V vertex2 = vertices[polygon.GetVertexIndex(2)];
#if __SPU
	// Computing the normal with the area on SPU leads to noticeable precision errors
	const Vec3V triangleNormal = NormalizeSafe(polygon.ComputeNonUnitNormal(vertex0,vertex1,vertex2),Vec3V(V_X_AXIS_WZERO));
#else // __SPU
	const Vec3V triangleNormal = polygon.ComputeUnitNormal(vertex0,vertex1,vertex2);
#endif // __SPU

	ScalarV fractionAlongSegment;
	if(geomSegments::SegmentTriangleIntersectUndirected(segmentStart,segmentStartToEnd,triangleNormal,vertex0,vertex1,vertex2,fractionAlongSegment))
	{
		const Vec3V positionOnTriangle = AddScaled(segmentStart,segmentStartToEnd,fractionAlongSegment);
		const ScalarV depth = Dist(positionOnTriangle,segmentEnd);
		return AddPolygonIntersection(positionOnTriangle,triangleNormal,triangleNormal,depth,fractionAlongSegment,partialIntersection);
	}

	return false;
}

#if __PFDRAW
void phShapeEdge::DrawShape (bool PF_DRAW_ONLY(retest))
{
	// Draw the test probe, if ProbeSegments is turned on.
	PFD_EdgeSegments.DrawArrow(VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()),VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()),retest?Color_aquamarine1:Color_white);
	PFD_EdgeSegments.DrawArrow(VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()),VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()),retest?Color_aquamarine1:Color_white);
}


void phShapeEdge::DrawIntersections (bool PF_DRAW_ONLY(retest))
{
	if (m_NumHits==0)
	{
		return;
	}

	if (m_Intersection)
	{
		// Draw the intersections, if ProbeIsects is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				PFD_EdgeIsects.DrawTick(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),0.1f,retest?Color_purple1:Color_red);
			}
		}

		// Draw the normals, if ProbeNormals is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				Vector3 probeNormalEnd(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()));
				probeNormalEnd.AddScaled(RCC_VECTOR3(m_Intersection[hitIndex].GetNormal()),1.0f);
				PFD_EdgeNormals.DrawArrow(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),probeNormalEnd,retest?Color_SeaGreen1:Color_green);
			}
		}
	}
}
#endif


#if __DEBUGLOG
void phShapeEdge::DebugReplay () const
{
	Vec3V start = m_WorldSegment.GetStart();
	diagDebugLog(diagDebugLogPhysics, 'SEWA',&start);
	Vec3V end = m_WorldSegment.GetEnd();
	diagDebugLog(diagDebugLogPhysics, 'SEWB',&end);

	phShapeBase::DebugReplay();
}
#endif


void phShapeSphere::InitPoint (Vec3V_In worldCenter, phIntersection* intersection, int numIntersections)
{
	Assertf(IsLessThanAll(Mag(worldCenter),ScalarV(V_FLT_LARGE_8)),"InitPoint has an impractical point %f, %f, %f",worldCenter.GetElemf(0),worldCenter.GetElemf(1),worldCenter.GetElemf(2));
	InitSphere(worldCenter,ScalarV(V_ZERO),intersection,numIntersections);
}


void phShapeSphere::InitSphere (Vec3V_In worldCenter, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	Assertf(IsLessThanAll(Mag(worldCenter),ScalarV(V_FLT_LARGE_8)),"InitSphere has an impractical center %f, %f, %f",worldCenter.GetElemf(0),worldCenter.GetElemf(1),worldCenter.GetElemf(2));
	Assertf(IsGreaterThanOrEqualAll(radius,ScalarV(V_ZERO)) && IsLessThanAll(radius,ScalarV(V_FLT_LARGE_8)),"InitSphere has an impractical radius %f",radius.Getf());
	m_WorldCenter.Set(VEC3V_TO_VECTOR3(worldCenter));
	m_Radius = radius.Getf();
	SetTestBackFacingPolygons(false);
	phShapeBase::Init(intersection,numIntersections);
	DEBUGLOG_ONLY(DebugReplay());
}


void phShapeSphere::SetupIteratorCull (phIterator& levelIterator)
{
	if (m_Radius>0.0f)
	{
		levelIterator.InitCull_Sphere(m_WorldCenter,m_Radius);
	}
	else
	{
		levelIterator.InitCull_Point(m_WorldCenter);
	}
}


bool phShapeSphere::RejectBound (const LocalizedData& localizedData, const phBound& bound) const
{
	// Return true if the bound's radius around its centroid plus the test sphere's radius is less than or equal to the distance from the center of the
	// test sphere to the bound's centroid (if the test sphere does not overlap the bound's sphere), or return false for no quick reject if there is overlap.
	float minRadius = (m_NumHits<m_MaxNumIsects ? m_Radius : m_Radius-m_SmallestDepth);
	return (square(bound.GetRadiusAroundCentroid()+minRadius)<localizedData.m_Center.Dist2(VEC3V_TO_VECTOR3(bound.GetCentroidOffset())));
}

bool phShapeSphere::IntersectSphereAgainstBound(Vec3V_In sphereCenter, ScalarV_In sphereRadius, const phBound& bound, Vec3V_InOut positionOnBound, Vec3V_InOut normalOnBound, ScalarV_InOut depth, phPolygon::Index& primitiveIndex)
{
	// Compute the test sphere center relative to the bound centroid.
	const Vec3V centroidOffset = bound.GetCentroidOffset();
	const Vec3V centerToSphereCenter = Subtract(sphereCenter,centroidOffset);

	primitiveIndex = 0;

	depth = ScalarV(V_NEG_FLT_MAX);
	switch (bound.GetType())
	{
		case phBound::SPHERE:
		{
			const ScalarV sphereBoundRadius = bound.GetRadiusAroundCentroidV();
			// The normal is just the vector between the sphere centers
			const Vec3V normalOnSphereBound = NormalizeSafe(centerToSphereCenter,Vec3V(V_X_AXIS_WZERO));
			const ScalarV distanceBetweenCenters = Dot(normalOnSphereBound,centerToSphereCenter);

			positionOnBound = AddScaled(centroidOffset,normalOnSphereBound,sphereBoundRadius);
			normalOnBound = normalOnSphereBound;
			depth = Subtract(Add(sphereRadius,sphereBoundRadius),distanceBetweenCenters);
			break;
		}

		case phBound::CAPSULE:
		{
			const phBoundCapsule& capsuleBound = *static_cast<const phBoundCapsule*>(&bound);
			const ScalarV capsuleRadius = capsuleBound.GetRadiusV();
			const ScalarV capsuleHalfLength = capsuleBound.GetHalfLengthV(capsuleRadius);

			// Find the closest point on the capsule's segment to the sphere's center
			const ScalarV closestHeightOnCapsuleSegment = Clamp(centerToSphereCenter.GetY(),Negate(capsuleHalfLength),capsuleHalfLength);
			const Vec3V closestPointOnCapsuleSegment = And(Vec3V(V_MASKY),Vec3V(closestHeightOnCapsuleSegment));

			// The vector from capsule segment to the sphere center is the intersection normal
			const Vec3V capsuleSegmentToSphereCenter = Subtract(centerToSphereCenter,closestPointOnCapsuleSegment);
			const Vec3V normalOnCapsule = NormalizeSafe(capsuleSegmentToSphereCenter,Vec3V(V_X_AXIS_WZERO));
			const ScalarV distanceFromCapsuleSegmentToSphereCenter = Dot(normalOnCapsule,capsuleSegmentToSphereCenter);

			positionOnBound = Add(centroidOffset,AddScaled(closestPointOnCapsuleSegment,normalOnCapsule,capsuleRadius));
			normalOnBound = normalOnCapsule;
			depth = Subtract(Add(sphereRadius,capsuleRadius),distanceFromCapsuleSegmentToSphereCenter);
			break;
		}

		case phBound::BOX:
		{
			const phBoundBox& boxBound = *static_cast<const phBoundBox*>(&bound);
			const Vec3V boxHalfSize = Scale(boxBound.GetBoundingBoxSize(),Vec3V(V_HALF));
			Vec3V localPositionOnBox;
			ScalarV sphereCenterDepth;
			geomPoints::FindClosestPointPointBoxSurface(centerToSphereCenter, boxHalfSize, localPositionOnBox, normalOnBound, sphereCenterDepth);
			depth = Add(sphereCenterDepth,sphereRadius);
			positionOnBound = Add(centroidOffset,localPositionOnBox);
			break;
		}

		case phBound::DISC:
		USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
		case phBound::CYLINDER:
		case phBound::GEOMETRY:
		{
			// Create a sphere bound.
			float radius = Max(sphereRadius.Getf(),0.001f);
			CREATE_SIMPLE_BOUND_ON_STACK(phBoundSphere,sphereBound,radius);
			DiscreteCollisionDetectorInterface::ClosestPointInput input(ScalarV(V_FLT_MAX),&bound,&sphereBound,phConvexIntersector::PDSOLVERTYPE_MINKOWSKI,NULL);
			DiscreteCollisionDetectorInterface::SimpleResult result;
			input.m_transformA = Mat34V(V_IDENTITY);
			input.m_transformB = Mat34V(Mat33V(V_IDENTITY),sphereCenter);

			phConvexIntersector::GetClosestPoints(input,result);
			if(result.GetHasResult())
			{
				depth = Negate(SplatX(result.GetDistanceV()));

				// transformA is an identity matrix so we can use the local point on A.
				positionOnBound = result.GetPointOnAInLocal();

				normalOnBound = Negate(result.GetNormalOnBInWorld());
			}

			break;
		}

		// TODO: Implement closed form solution
		/*case phBound::DISC:
		{
			Assert(0);
			break;
		}*/

		default:
		{
			AssertMsg(0 , "invalid bound type");
			return false;
		}
	}

	return IsGreaterThanAll(depth,ScalarV(V_ZERO)) != 0;
}

bool phShapeSphere::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 UNUSED_PARAM(primTypeFlags), u32 UNUSED_PARAM(primIncludeFlags), const PartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TestBoundPrimitive);
#endif
	// Compute the test sphere center relative to the bound centroid.
	const ScalarV radius = ScalarVFromF32(m_Radius);

	Vec3V normalOnBound;
	Vec3V positionOnBound;
	ScalarV depth;
	phPolygon::Index primitiveIndex;
	if(IntersectSphereAgainstBound(RCC_VEC3V(localizedData.m_Center),radius,bound,positionOnBound,normalOnBound,depth,primitiveIndex))
	{
		return AddIntersection(positionOnBound,normalOnBound,depth,ScalarV(V_ZERO),bound.GetMaterialId(primitiveIndex),partialIntersection,radius);
	}
	else
	{
		return false;
	}
}

bool phShapeSphere::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 UNUSED_PARAM(polyTypeFlags), u32 UNUSED_PARAM(polyIncludeFlags), const phBoundGeometry& UNUSED_PARAM(geomBound), const PrimitivePartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(SphereTestPolygon);
#endif

	const Vec3V sphereCenter = RCC_VEC3V(localizedData.m_Center);
	const ScalarV sphereRadius = ScalarVFromF32(m_Radius);
	const ScalarV sphereRadiusSquared = Scale(sphereRadius,sphereRadius);
	const ScalarV sphereBackFaceTolerance = ScalarVFromF32(m_BackFaceTolerance); // This will either be -radius if we want back faces or 0 if we don't

	const Vec3V* vertices = &RCC_VEC3V(boundVertices);
	const Vec3V vertex0 = vertices[polygon.GetVertexIndex(0)];
	const Vec3V vertex1 = vertices[polygon.GetVertexIndex(1)];
	const Vec3V vertex2 = vertices[polygon.GetVertexIndex(2)];
	const Vec3V polygonNormal = polygon.ComputeUnitNormal(vertex0,vertex1,vertex2);

	// Don't allow intersections if the sphere is behind the triangle plane
	const ScalarV sphereCenterDistanceAboveTrianglePlane = Dot(Subtract(sphereCenter,vertex0),polygonNormal);
	const BoolV isSphereTooFarBehindTriangle = IsLessThan(sphereCenterDistanceAboveTrianglePlane,sphereBackFaceTolerance);

	// If the sphere is more than the radius in front of the triangle plane, there is no way for them to touch
	const BoolV isSphereTooFarInfrontOfTriangle = IsGreaterThan(sphereCenterDistanceAboveTrianglePlane,sphereRadius);

	// If any cull check fails, return with no intersection
	if((isSphereTooFarBehindTriangle | isSphereTooFarInfrontOfTriangle).Getb())
	{
		return false;
	}

	// Find the closest point on the triangle to the sphere center
	const Vec3V closestPointOnTriangle = geomPoints::FindClosestPointPointTriangle(sphereCenter,vertex0,vertex1,vertex2);
	const Vec3V triangleToSphereCenter = Subtract(sphereCenter,closestPointOnTriangle);

	// Determine if that point is inside the sphere
	const ScalarV distanceFromSphereCenterSquared = MagSquared(triangleToSphereCenter);
	if(IsLessThanAll(distanceFromSphereCenterSquared,sphereRadiusSquared))
	{
		const ScalarV distanceFromSphereCenter = Sqrt(distanceFromSphereCenterSquared);
		const ScalarV depth = Subtract(sphereRadius,distanceFromSphereCenter);
		if(IsIntersectionValid(depth,partialIntersection))
		{
			if (phIntersection* intersection = GetNextIntersection(depth))
			{
				// If the sphere center is touching the polygon, using the polygon normal should be acceptable
				const Vec3V normalOnTriangle = NormalizeSafe(triangleToSphereCenter,polygonNormal);
				SetPolygonIntersection(*intersection,closestPointOnTriangle,normalOnTriangle,polygonNormal,depth,ScalarV(V_ZERO),partialIntersection);
			}
			return true;
		}
	}
	return false;
}

// This function is culling against a BVH structure.  Note that it doesn't do any of the expanding the cull shape for trying to handle second surface
//   as this function is use for all BVHs which may not necessarily have come from a phBoundBVH.
void phShapeSphere::PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler)
{
	phBvhAabbOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
	const Vec3V aabbMin = RCC_VEC3V(localizedData.m_Center) - Vec3VFromF32(m_Radius);
	const Vec3V aabbMax = RCC_VEC3V(localizedData.m_Center) + Vec3VFromF32(m_Radius);
	nodeOverlapCallback.SetAABB(RCC_VECTOR3(aabbMin), RCC_VECTOR3(aabbMax));
	bvhStructure.walkStacklessTree(&nodeOverlapCallback);
}

#if __PFDRAW
void phShapeSphere::DrawShape (bool PF_DRAW_ONLY(retest))
{
	// Draw the test sphere, if SphereSegments is turned on.
	// Make the radius 10cm for point tests.
	float sphereRadius = (m_Radius>0.0f ? m_Radius : 0.1f);
	PFD_SphereSegments.DrawSphere(sphereRadius,m_WorldCenter,retest?Color_aquamarine1:Color_white);
}


void phShapeSphere::DrawIntersections (bool PF_DRAW_ONLY(retest))
{
	if (m_NumHits==0 || !m_Intersection)
	{
		return;
	}

	if (m_Intersection)
	{
		// Draw the intersections, if SphereIsects is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				PFD_SphereIsects.DrawTick(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),0.1f,retest?Color_purple1:Color_red);
			}
		}

		// Draw the normals, if SphereNormals is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				Vector3 probeNormalEnd(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()));
				probeNormalEnd.AddScaled(RCC_VECTOR3(m_Intersection[hitIndex].GetNormal()),1.0f);
				PFD_SphereNormals.DrawArrow(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),probeNormalEnd,retest?Color_SeaGreen1:Color_green);
			}
		}
	}
}
#endif


#if __DEBUGLOG
void phShapeSphere::DebugReplay () const
{
	diagDebugLog(diagDebugLogPhysics, 'SSWC',&m_WorldCenter);

	diagDebugLog(diagDebugLogPhysics, 'SSRa',&m_Radius);

	phShapeBase::DebugReplay();
}
#endif


void phShapeCapsule::InitCapsule (const phSegmentV& worldSegment, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	Assertf(IsLessThanAll(Mag(worldSegment.GetStart()),ScalarV(V_FLT_LARGE_8)),"InitCapsule has an impractical segment starting point %f, %f, %f",worldSegment.GetStart().GetElemf(0),worldSegment.GetStart().GetElemf(1),worldSegment.GetStart().GetElemf(2));
	Assertf(IsLessThanAll(Mag(worldSegment.GetEnd()),ScalarV(V_FLT_LARGE_8)),"InitCapsule has an impractical segment ending point %f, %f, %f",worldSegment.GetEnd().GetElemf(0),worldSegment.GetEnd().GetElemf(1),worldSegment.GetEnd().GetElemf(2));
	Assertf(IsGreaterThanOrEqualAll(radius,ScalarV(V_ZERO)) && IsLessThanAll(radius,ScalarV(V_FLT_LARGE_8)),"InitCapsule has an impractical radius %f",radius.Getf());
	m_WorldSegment.Set(worldSegment);
	m_Radius = radius;
	phShapeBase::Init(intersection,numIntersections);
	DEBUGLOG_ONLY(DebugReplay());
}




void phShapeCapsule::SetupIteratorCull (phIterator& levelIterator)
{
	Vec3V segmentAtoB = Subtract(m_WorldSegment.GetEnd(),m_WorldSegment.GetStart());
	levelIterator.InitCull_Capsule(m_WorldSegment.GetStart(),segmentAtoB,Mag(segmentAtoB),m_Radius);
}


bool phShapeCapsule::RejectBound (const LocalizedData& localizedData, const phBound& bound) const
{
	return !geomBoxes::TestCapsuleToAlignedBoxFP(localizedData.m_Segment.GetStart().GetIntrin128(),localizedData.m_Segment.GetEnd().GetIntrin128(),m_Radius.GetIntrin128(),bound.GetBoundingBoxMin().GetIntrin128(),bound.GetBoundingBoxMax().GetIntrin128());
}

bool phShapeCapsule::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 UNUSED_PARAM(primTypeFlags), u32 UNUSED_PARAM(primIncludeFlags), const PartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TestBoundPrimitive);
#endif

	// Compute the segment endpoints relative to each other and to the bound centroid.
	const Vec3V centroidOffset = bound.GetCentroidOffset();
	const Vec3V centerToA = Subtract(localizedData.m_Segment.GetStart(),centroidOffset);
	const Vec3V centerToB = Subtract(localizedData.m_Segment.GetEnd(),centroidOffset);
	const Vec3V segmentAtoB = Subtract(centerToB,centerToA);
	const ScalarV radius = GetRadius();

	ScalarV depth(V_NEG_FLT_MAX);
	Vec3V normalOnBound(V_ZERO);
	Vec3V positionOnBound(V_ZERO);
	int partIndex = 0;
	switch (bound.GetType())
	{
		case phBound::SPHERE:
		{
			Vec3V positionOnLocalSphere;
			geomCapsule::CapsuleSphereIntersect(centerToA,segmentAtoB,radius,Vec3V(V_ZERO),bound.GetRadiusAroundCentroidV(),positionOnLocalSphere,normalOnBound,depth);
			positionOnBound = Add(positionOnLocalSphere,centroidOffset);
			break;
		}

		case phBound::CAPSULE:
		{
			const phBoundCapsule& capsuleBound = static_cast<const phBoundCapsule&>(bound);
			const ScalarV capsuleBoundRadius = capsuleBound.GetRadiusV();
			const ScalarV capsuleBoundHalfLength = capsuleBound.GetHalfLengthV(capsuleBoundRadius);

			Vec3V positionOnLocalCapsule;
			geomCapsule::CapsuleAlignedCapsuleIntersect(centerToA,segmentAtoB,radius,capsuleBoundHalfLength,capsuleBoundRadius,positionOnLocalCapsule,normalOnBound,depth);
			positionOnBound = Add(positionOnLocalCapsule,centroidOffset);
			break;
		}

		case phBound::BOX:
		{
			const Vec3V boxHalfSize = Subtract(bound.GetBoundingBoxMax(),centroidOffset);

			Vec3V positionOnLocalBox;
			geomCapsule::CapsuleAABBIntersect(centerToA,centerToB,radius,boxHalfSize,positionOnLocalBox,normalOnBound,depth);
			positionOnBound = Add(positionOnLocalBox,centroidOffset);
			break;
		}

		case phBound::DISC:
		USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
		case phBound::CYLINDER:
		case phBound::GEOMETRY:
		{
			const ScalarV capsuleLength = Mag(segmentAtoB);
			// Create a matrix pose for the capsule. The Y-axis should be aligned with the capsule segment. 
			Mat34V capsulePose;
			capsulePose.SetCol1(InvScaleSafe(segmentAtoB,capsuleLength,Vec3V(V_X_AXIS_WZERO)));
			MakeOrthonormals(capsulePose.GetCol1(),capsulePose.GetCol0Ref(),capsulePose.GetCol2Ref());
			capsulePose.SetCol3(Average(localizedData.m_Segment.GetStart(),localizedData.m_Segment.GetEnd()));

			// Create a capsule bound.
			CREATE_SIMPLE_BOUND_ON_STACK(phBoundCapsule,capsuleBound);
			capsuleBound.SetCapsuleSize(radius,capsuleLength);

			// Compute any penetration of the capsule in the curved geometry bound.
			DiscreteCollisionDetectorInterface::ClosestPointInput input(ScalarV(V_FLT_MAX),&bound,&capsuleBound,phConvexIntersector::PDSOLVERTYPE_MINKOWSKI,NULL);
			DiscreteCollisionDetectorInterface::SimpleResult result;
			input.m_transformA = Mat34V(V_IDENTITY);
			input.m_transformB = capsulePose;
			phConvexIntersector::GetClosestPoints(input,result);
			if(result.GetHasResult())
			{
				depth = Negate(result.GetDistanceV().GetX());
				normalOnBound = Negate(result.GetNormalOnBInWorld());

				// transformA is an identity matrix so we can use the local point on A.
				positionOnBound = result.GetPointOnAInLocal();
			}

			break;
		}

		// TODO: Implement closed form solution
		/*case phBound::DISC:
		{
			Assert(0);
			break;
		}*/

		default:
		{
			AssertMsg(0 , "invalid bound type");
			return false;
		}
	}	// end of switch (bound.GetType())

	// TODO: Make sure nobody depends on the T-Value being set and get rid of this. No other non-swept shapetest sets intersection T-Values to non-zero values. 
	const Vec3V positionOnCapsule = SubtractScaled(positionOnBound,normalOnBound,depth);
	const ScalarV capsuleSegmentT = InvScaleSafe(Dot(Subtract(positionOnCapsule,localizedData.m_Segment.GetStart()), segmentAtoB), MagSquared(segmentAtoB), ScalarV(V_ZERO));

	// Get the material and see if it should be ignored.
	phMaterialMgr::Id materialId = bound.GetMaterialId(partIndex);
	return AddIntersection(positionOnBound,normalOnBound,depth,capsuleSegmentT,materialId,partialIntersection,radius);
}


bool phShapeCapsule::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 UNUSED_PARAM(polyTypeFlags), u32 UNUSED_PARAM(polyIncludeFlags), const phBoundGeometry& UNUSED_PARAM(geomBound), const PrimitivePartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(CapsuleTestPolygon);
#endif

	const ScalarV capsuleRadius = m_Radius;
	Vec3V localSegmentA = localizedData.m_Segment.GetStart();
	Vec3V localSegmentB = localizedData.m_Segment.GetEnd();

	const Vec3V * RESTRICT triangleVertices = (const Vec3V *)(&boundVertices);
	Vec3V v0 = triangleVertices[polygon.GetVertexIndex(0)];
	Vec3V v1 = triangleVertices[polygon.GetVertexIndex(1)];
	Vec3V v2 = triangleVertices[polygon.GetVertexIndex(2)];
	const Vec3V triangleNormal = polygon.ComputeUnitNormal(v0, v1, v2);

	// Do an AABB-AABB test
	const Vec3V capsuleAABBMin = localizedData.m_Min;
	const Vec3V capsuleAABBMax = localizedData.m_Max;
	const Vec3V triAABBMin = Min(Min(v0,v1), v2);
	const Vec3V triAABBMax = Max(Max(v0,v1), v2);
	const VecBoolV boundingBoxesIntersectV = IsLessThan(capsuleAABBMin, triAABBMax) & IsLessThan(triAABBMin, capsuleAABBMax);
	const BoolV boundingBoxesIntersect = boundingBoxesIntersectV.GetX() & boundingBoxesIntersectV.GetY() & boundingBoxesIntersectV.GetZ();

	// Do a capsule-plane test
	const ScalarV dPlane = Dot(triangleNormal, v0);
	const ScalarV dSegmentA = Subtract(Dot(triangleNormal, localSegmentA), dPlane);
	const ScalarV dSegmentB = Subtract(Dot(triangleNormal, localSegmentB), dPlane);
	const BoolV capsuleIntersectsPlane = IsLessThan(Abs(dSegmentA), capsuleRadius) | IsLessThan(Abs(dSegmentB), capsuleRadius) | IsLessThan(Scale(dSegmentA,dSegmentB), ScalarV(V_ZERO));

	// If either the capsule-plane or AABB-AABB test fails, return no intersection
	if(InvertBits(capsuleIntersectsPlane & boundingBoxesIntersect).Getb())
	{
		return false;
	}

	// phConvexIntersector expects v0 to be the origin
	const Vec3V triangleOffset = v0;
	v0 = Vec3V(V_ZERO);
	v1 = Subtract(v1,triangleOffset);
	v2 = Subtract(v2,triangleOffset);

	localSegmentA = Subtract(localSegmentA,triangleOffset);
	localSegmentB = Subtract(localSegmentB,triangleOffset);

	const Vec3V capsuleSegment = Subtract(localSegmentB, localSegmentA);
	const ScalarV capsuleLength = Mag(capsuleSegment);
	const Vec3V capsuleSegmentNorm = SelectFT(IsGreaterThan(capsuleLength,ScalarV(V_FLT_SMALL_12)), Vec3V(V_Y_AXIS_WZERO),InvScale(capsuleSegment,capsuleLength));

	// Calculate the closest point on the segment to the triangle
	Vec3V pointOnTri, pointOnSeg;
	geomPoints::FindClosestSegSegToTri(pointOnTri, pointOnSeg, v1, v2, localSegmentA, localSegmentB);

	Vec3V delta = Subtract(pointOnSeg, pointOnTri);
	ScalarV dist = Mag(delta);
	ScalarV invDist = InvertSafe(dist);
	Vec3V normalOnTriangle = Scale(delta, invDist);
	
	// Overlapping closest points means penetration
	if(IsLessThanOrEqualAll(dist, ScalarV(V_FLT_SMALL_5)))
	{
		// Create a matrix for the capsule where v0 is the origin
		const Vec3V yAxis = capsuleSegmentNorm;
		Vec3V xAxis,zAxis;
		MakeOrthonormals(yAxis, xAxis, zAxis);
		const Mat34V capsulePose(xAxis,yAxis,zAxis,Average(localSegmentA, localSegmentB));

		// Create a capsule bound (requires minimum radius)
		CREATE_SIMPLE_BOUND_ON_STACK(phBoundCapsule,capsuleBound);
		capsuleBound.SetCapsuleSize(Add(capsuleRadius, ScalarV(V_FLT_SMALL_6)),capsuleLength);

		// Create a triangle bound
		CREATE_SIMPLE_BOUND_ON_STACK(TriangleShape,triangleBound);
		triangleBound.SetVertices(v0,v1,v2);
		triangleBound.SetNormal(triangleNormal);
		triangleBound.SetMargin(ScalarV(V_ZERO));

		Vec3V depenOffsetDir;
		ScalarV depenOffsetDist;

		MinkowskiPenetrationDepthSolver penetrationDepthSolver;
		penetrationDepthSolver.ComputePenetrationSeparation(&capsuleBound, &triangleBound, capsulePose, Mat34V(V_IDENTITY), depenOffsetDir, depenOffsetDist);

		// Use the best axis from ComputePenetrationDepth to separate the segment and recompute new closest points
		Vec3V depenOffset = Scale(depenOffsetDir, depenOffsetDist);
		Vec3V offPoint1 = Add(localSegmentA, depenOffset);
		Vec3V offPoint2 = Add(localSegmentB, depenOffset);
		Vec3V offPointOnTri, offPointOnSeg;
		geomPoints::FindClosestSegSegToTriNoIntersection(offPointOnTri, offPointOnSeg, v1, v2, offPoint1, offPoint2);

		Vec3V offResDelta = Subtract(offPointOnSeg, offPointOnTri);
		ScalarV offResDist = Mag(offResDelta);
		Assertf(!IsEqualAll(offResDist, ScalarV(V_ZERO)), "phShapeCapsule::TestPolygon - Capsule/Tri penetration depth solver has failed. Still contacting after depenetration.");
		ScalarV offResInvDist = Invert(offResDist);
		Vec3V offResNormal = Scale(offResDelta, offResInvDist);

		// Correcting for the direction we separated the objects
		const ScalarV normalAdjustmentFactor = Dot(depenOffsetDir, offResNormal);
		const ScalarV correctedMinPenAlongNorm = Scale(depenOffsetDist, normalAdjustmentFactor) - offResDist;

		pointOnTri = offPointOnTri;

		// Use normal from separated test
		normalOnTriangle = offResNormal;
		// distance apart is negative because these are penetrating points
		dist = Negate(correctedMinPenAlongNorm);
	}

	ScalarV depth = Subtract(capsuleRadius,dist);
	// See if the intersection should be set.
	Vec3V finalPointOnTriangle = Add(pointOnTri, triangleOffset);
	if(IsIntersectionValid(depth,partialIntersection))
	{
		if (phIntersection* intersection = GetNextIntersection(depth,capsuleRadius))
		{
			ScalarV tValueAlongSegment = Clamp(Dot(capsuleSegmentNorm,Subtract(pointOnTri,localSegmentA)), ScalarV(V_ZERO), ScalarV(V_ONE));
			SetPolygonIntersection(*intersection,finalPointOnTriangle,normalOnTriangle,triangleNormal,depth,tValueAlongSegment,partialIntersection);
		}
		return true;
	}
	return false;
}

// This function is culling against a BVH structure.  Note that it doesn't do any of the expanding the cull shape for trying to handle second surface
//   as this function is use for all BVHs which may not necessarily have come from a phBoundBVH.
void phShapeCapsule::PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler)
{
	phBvhCapsuleOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
	nodeOverlapCallback.SetCapsule(localizedData.m_Segment.GetStart(), localizedData.m_Segment.GetEnd(), m_Radius);
	bvhStructure.walkStacklessTree(&nodeOverlapCallback);
}


#if __PFDRAW
void phShapeCapsule::DrawShape (bool PF_DRAW_ONLY(retest))
{
	// Draw the test capsule, if CapsuleSegments is turned on.
	PFD_CapsuleSegments.DrawCapsule(VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()),VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()),m_Radius.Getf(),retest?Color_aquamarine1:Color_white);
}


void phShapeCapsule::DrawIntersections (bool PF_DRAW_ONLY(retest))
{
	if (m_NumHits==0 || !m_Intersection)
	{
		return;
	}

	if (m_Intersection)
	{
		// Draw the intersections, if CapsuleIsects is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				PFD_CapsuleIsects.DrawTick(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),0.1f,retest?Color_purple1:Color_red);
			}
		}

		// Draw the normals, if CapsuleNormals is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				Vector3 probeNormalEnd(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()));
				probeNormalEnd.AddScaled(RCC_VECTOR3(m_Intersection[hitIndex].GetNormal()),1.0f);
				PFD_CapsuleNormals.DrawArrow(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),probeNormalEnd,retest?Color_SeaGreen1:Color_green);
			}
		}
	}
}
#endif


#if __DEBUGLOG
void phShapeCapsule::DebugReplay () const
{
	diagDebugLog(diagDebugLogPhysics, 'SCWA',&m_WorldSegment.A);
	diagDebugLog(diagDebugLogPhysics, 'SCWB',&m_WorldSegment.B);

	diagDebugLog(diagDebugLogPhysics, 'SCFT',&m_Radius);

	phShapeBase::DebugReplay();
}
#endif


void phShapeSweptSphere::InitSweptSphere (const phSegmentV& worldSegment, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	Assertf(IsLessThanAll(Mag(worldSegment.GetStart()),ScalarV(V_FLT_LARGE_8)),"InitSweptSphere has an impractical segment starting point %f, %f, %f",worldSegment.GetStart().GetElemf(0),worldSegment.GetStart().GetElemf(1),worldSegment.GetStart().GetElemf(2));
	Assertf(IsLessThanAll(Mag(worldSegment.GetEnd()),ScalarV(V_FLT_LARGE_8)),"InitSweptSphere has an impractical segment ending point %f, %f, %f",worldSegment.GetEnd().GetElemf(0),worldSegment.GetEnd().GetElemf(1),worldSegment.GetEnd().GetElemf(2));
	Assertf(IsGreaterThanOrEqualAll(radius,ScalarV(V_ZERO)) && IsLessThanAll(radius,ScalarV(V_FLT_LARGE_8)),"InitSweptSphere has an impractical radius %f",radius.Getf());
	m_WorldSegment.Set(worldSegment);
	m_Radius = radius;
	SetTestInitialSphere(false);
	phShapeBase::Init(intersection,numIntersections);
	DEBUGLOG_ONLY(DebugReplay());
}

bool phShapeSweptSphere::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 UNUSED_PARAM(primTypeFlags), u32 UNUSED_PARAM(primIncludeFlags), const PartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TestBoundPrimitive);
#endif
	// Constants used multiple times
	const ScalarV svOne(V_ONE);
	const ScalarV svZero(V_ZERO);

	// Create local copies of the swept sphere variables
	const Vec3V segmentA = localizedData.m_Segment.GetStart();
	const Vec3V segmentB = localizedData.m_Segment.GetEnd();
	const Vec3V segmentAtoB = Subtract(segmentB,segmentA);
	const ScalarV sweptSphereRadius = m_Radius;
	const ScalarV segmentLength = Mag(segmentAtoB);

	// Clamp the swept-sphere to be as short as possible. 
	// This prevents quadratic error when testing long swept spheres. 
	const Vec3V centroidOffset = bound.GetCentroidOffset();
	const ScalarV boundRadius = bound.GetRadiusAroundCentroidV();
	const ScalarV combinedRadius = Add(boundRadius,sweptSphereRadius);

	const ScalarV centroidT = InvScale(Dot(Subtract(centroidOffset, segmentA), segmentAtoB), Scale(segmentLength,segmentLength));
	const ScalarV radiusT = InvScale(combinedRadius + ScalarV(V_FLT_SMALL_2), segmentLength);

	const ScalarV clampedStartT = Clamp(Subtract(centroidT,radiusT),svZero,svOne);
	const ScalarV clampedEndT = Clamp(Add(centroidT,radiusT),svZero,svOne);
	const ScalarV clampedSegmentT = Subtract(clampedEndT,clampedStartT);

	const Vec3V clampedSegmentA = AddScaled(segmentA,segmentAtoB,clampedStartT);
	const Vec3V clampedSegmentAtoB = Scale(segmentAtoB,clampedSegmentT);

	// Compute the intersections of the swept-sphere with the bound.
	Vec3V normalOnBound;
	Vec3V positionOnBound;
	ScalarV clampedFractionAlongSegment;
	phPolygon::Index partIndex = 0;
	bool wasIntersectionFound = false;
	switch (bound.GetType())
	{
		case phBound::SPHERE:
		{
			wasIntersectionFound = geomSweptSphere::SweptSphereSphereIntersect(Subtract(clampedSegmentA,centroidOffset),clampedSegmentAtoB,sweptSphereRadius,bound.GetRadiusAroundCentroidV(),positionOnBound,normalOnBound,clampedFractionAlongSegment).Getb();
			positionOnBound = Add(positionOnBound,centroidOffset);
			break;
		}

		case phBound::CAPSULE:
		{
			wasIntersectionFound = geomSweptSphere::SweptSphereCapsuleIntersect(Subtract(clampedSegmentA,centroidOffset),clampedSegmentAtoB,sweptSphereRadius,static_cast<const phBoundCapsule&>(bound).GetLengthV(),bound.GetMarginV(),positionOnBound,normalOnBound,clampedFractionAlongSegment).Getb();
			positionOnBound = Add(positionOnBound,centroidOffset);
			break;
		}

		case phBound::BOX:
		{
			wasIntersectionFound = geomSweptSphere::SweptSphereBoxIntersect(Subtract(clampedSegmentA,centroidOffset),clampedSegmentAtoB,sweptSphereRadius,Scale(bound.GetBoundingBoxSize(),ScalarV(V_HALF)),positionOnBound,normalOnBound,clampedFractionAlongSegment).Getb();
			positionOnBound = Add(positionOnBound,centroidOffset);
			break;
		}

		case phBound::DISC:
		USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
		case phBound::CYLINDER:
		case phBound::GEOMETRY:
		{
			IterativeCastResult result;
			wasIntersectionFound = IterativeCast(CastSphereInput(clampedSegmentA,clampedSegmentAtoB, sweptSphereRadius),bound,result) && IsGreaterThanAll(result.m_IntersectionTime,ScalarV(V_ZERO));
			clampedFractionAlongSegment = result.m_IntersectionTime;
			normalOnBound = result.m_NormalOnLocalBound;
			positionOnBound = result.m_Position;
			break;
		}

		// TODO: Implement closed form solution
		/*case phBound::DISC:
		{
			Assert(0);
			break;
		}*/

		default:
		{
			AssertMsg(0 , "invalid bound type");
			return false;
		}
	}	// end of switch (bound.GetType())

	if(!wasIntersectionFound)
	{

		// Only test the initial sphere if the user wants and the sphere is intersecting the bound's cull-sphere
		if(	GetTestInitialSphere() && IsLessThanOrEqualAll(DistSquared(segmentA,centroidOffset),Scale(combinedRadius,combinedRadius)))
		{
			ScalarV depth;
			if(phShapeSphere::IntersectSphereAgainstBound(segmentA,sweptSphereRadius,bound,positionOnBound,normalOnBound,depth,partIndex))
			{
				// Disregard the depth in the sphere and use a T-value based depth. Intersections with the initial sphere are the most important.
				return AddIntersection(positionOnBound,normalOnBound,segmentLength,ScalarV(V_ZERO),bound.GetMaterialId(partIndex),partialIntersection);
			}
		}
		return false;
	}
	// Find the unclamped fraction along the segment. 
	const ScalarV fractionAlongSegment = AddScaled(clampedStartT, clampedSegmentT, clampedFractionAlongSegment);

	// See if the intersection should be set.
	const ScalarV depth = Scale(segmentLength, Subtract(svOne,fractionAlongSegment));

	return AddIntersection(positionOnBound,normalOnBound,depth,fractionAlongSegment,bound.GetMaterialId(partIndex),partialIntersection);
}

bool phShapeSweptSphere::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 UNUSED_PARAM(polyTypeFlags), u32 UNUSED_PARAM(polyIncludeFlags), const phBoundGeometry& UNUSED_PARAM(geomBound), const PrimitivePartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(SweptSphereTestPolygon);
#endif

	const Vec3V * RESTRICT vTriVertices = (const Vec3V *)(&boundVertices);
	const Vec3V v0 = vTriVertices[polygon.GetVertexIndex(0)];
	const Vec3V v1 = vTriVertices[polygon.GetVertexIndex(1)];
	const Vec3V v2 = vTriVertices[polygon.GetVertexIndex(2)];
	Vec3V triangleFaceNormal = polygon.ComputeUnitNormal(v0,v1,v2);

	Vec3V segmentA = localizedData.m_Segment.GetStart();
	Vec3V segmentB = localizedData.m_Segment.GetEnd();
	Vec3V segmentAtoB = Subtract(segmentB, segmentA);
	ScalarV radius = GetRadius();
	ScalarV svZero(V_ZERO);
	ScalarV svOne(V_ONE);

	// Do an AABB-AABB test first
	const Vec3V sweptSphereAABBMin = localizedData.m_Min;
	const Vec3V sweptSphereAABBMax = localizedData.m_Max;
	const Vec3V triAABBMin = Min(Min(v0,v1), v2);
	const Vec3V triAABBMax = Max(Max(v0,v1), v2);
	const VecBoolV boundingBoxesIntersectV = IsLessThan(sweptSphereAABBMin, triAABBMax) & IsLessThan(triAABBMin, sweptSphereAABBMax);
	const BoolV boundingBoxesIntersect = boundingBoxesIntersectV.GetX() & boundingBoxesIntersectV.GetY() & boundingBoxesIntersectV.GetZ();
	if(boundingBoxesIntersect.Getb())
	{
		// Do a swept sphere-plane test
		// The start of the sweep must be in front of the  triangle plane and the end of the sweept must be behind or within the radius
		//   of the backside of the triangle plane
		const ScalarV dPlane = Dot(triangleFaceNormal, v0);
		const ScalarV dSegmentA = Subtract(Dot(triangleFaceNormal, segmentA), dPlane);
		const ScalarV dSegmentB = Subtract(Dot(triangleFaceNormal, segmentB), dPlane);
		const BoolV sweptSphereIntersectsPlane = IsGreaterThan(dSegmentA,svZero) & IsLessThan(dSegmentB,radius);

		// See if the swept sphere is moving away from the triangle
		const ScalarV relativeSpeedAwayFromTriangle = Dot(triangleFaceNormal, segmentAtoB);
		const BoolV sphereIsMovingTowardsTriangle = IsLessThan(relativeSpeedAwayFromTriangle, svZero);

		// Only test the swept sphere if it intersects the plane and it's moving towards the triangle (against the normal)
		if((sweptSphereIntersectsPlane & sphereIsMovingTowardsTriangle).Getb())
		{
			// Clamp the swept sphere to the triangle to prevent large numerical error from quadratic equations
			Vec3V triangleCenter = Average(triAABBMax,triAABBMin);
			ScalarV segmentLength = Mag(segmentAtoB);
			ScalarV invSegmentLength = Invert(segmentLength);
			ScalarV invSegmentLengthSquared = Scale(invSegmentLength,invSegmentLength);
			ScalarV triangleCenterT = Scale(Dot(Subtract(triangleCenter, segmentA), segmentAtoB), invSegmentLengthSquared);
			// For the triangles radius, just use the largest dimension of the bounding box. This isn't exact, but we just need to shrink the 
			//   sweep to ~100m to prevent large errors.
			ScalarV triangleRadiusT = Scale(Add(MaxElement(Subtract(triAABBMax,triAABBMin)), radius), invSegmentLength);

			ScalarV clampedStartT = Clamp(Subtract(triangleCenterT, triangleRadiusT), svZero, svOne);
			ScalarV clampedEndT = Clamp(Add(triangleCenterT, triangleRadiusT), svZero, svOne);
			Vec3V clampedSegmentA = AddScaled(segmentA, segmentAtoB, clampedStartT);
			Vec3V clampedSegmentB = AddScaled(segmentA, segmentAtoB, clampedEndT);

			Vec3V positionOnTriangle;
			Vec3V normalOnTriangle;
			ScalarV clampedIntersectionT;

			if(geomSweptSphere::SweptSphereTriangleIntersect(clampedSegmentA,clampedSegmentB,radius,v0,v1,v2,triangleFaceNormal,positionOnTriangle,normalOnTriangle,clampedIntersectionT).Getb())
			{
				// Convert the clamped T into an unclamped T
				ScalarV intersectionT = AddScaled(clampedStartT, Subtract(clampedEndT,clampedStartT), clampedIntersectionT);

				// Check if this intersection is good enough
				ScalarV depth = Scale(segmentLength, Subtract(svOne, intersectionT));
				return AddPolygonIntersection(positionOnTriangle,normalOnTriangle,triangleFaceNormal,depth,intersectionT,partialIntersection);
			}
		}

		// If we get here it means that the swept sphere didn't hit anything, try the initial sphere if the user wants
		if(GetTestInitialSphere())
		{
			// Do an extra AABB test since we don't want to test all the polys the the swept sphere might have hit
			const Vec3V initialSphereAABBMin = Subtract(segmentA,Vec3V(radius));
			const Vec3V initialSphereAABBMax = Add(segmentA,Vec3V(radius));
			const VecBoolV initialSphereBoundingBoxIntersectsV = IsLessThan(initialSphereAABBMin, triAABBMax) & IsLessThan(triAABBMin, initialSphereAABBMax);
			const BoolV initialSphereBoundingBoxIntersects = initialSphereBoundingBoxIntersectsV.GetX() & initialSphereBoundingBoxIntersectsV.GetY() & initialSphereBoundingBoxIntersectsV.GetZ();
			if(initialSphereBoundingBoxIntersects.Getb())
			{
				// Find the closest point on the triangle to the sphere center
				Vec3V closestPointOnTriangle = geomPoints::FindClosestPointPointTriangle(segmentA,v0,v1,v2);
				Vec3V triangleToSphereCenter = Subtract(segmentA,closestPointOnTriangle);

				// Determine if that point is inside the sphere
				ScalarV distanceFromSphereCenterSquared = MagSquared(triangleToSphereCenter);
				if(IsLessThanAll(distanceFromSphereCenterSquared,Scale(radius,radius)))
				{
					// Disregard the depth in the sphere and use a T-value based depth. Intersections with the initial sphere are the most important.
					ScalarV depth = Mag(segmentAtoB);
					if(IsIntersectionValid(depth,partialIntersection))
					{
						if (phIntersection* intersection = GetNextIntersection(depth))
						{
							// If the sphere center is touching the polygon, using the polygon normal should be acceptable
							Vec3V normalOnTriangle = NormalizeSafe(triangleToSphereCenter,triangleFaceNormal);
							SetPolygonIntersection(*intersection,closestPointOnTriangle,normalOnTriangle,triangleFaceNormal,depth,ScalarV(V_ZERO),partialIntersection);
						}
						return true;
					}
				}
			}
		}
	}

	return false;
}


#if __PFDRAW
void phShapeSweptSphere::DrawShape (bool PF_DRAW_ONLY(retest))
{
	// Draw the swept sphere, if SweptSphereSegments is turned on.
	PFD_SweptSphereSegments.DrawCapsule(VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()),VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()),m_Radius.Getf(),retest?Color_aquamarine1:Color_white);
	PFD_SweptSphereSegments.DrawArrow(VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()),VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()));
}

void phShapeSweptSphere::DrawIntersections (bool PF_DRAW_ONLY(retest))
{
	if (m_NumHits==0 || !m_Intersection)
	{
		return;
	}

	if (m_Intersection)
	{
		// Draw the intersections, if SweptSphereIsects is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				PFD_SweptSphereIsects.DrawTick(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),0.1f,retest?Color_purple1:Color_red);
			}
		}

		// Draw the normals, if SweptSphereNormals is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				Vector3 probeNormalEnd(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()));
				probeNormalEnd.AddScaled(RCC_VECTOR3(m_Intersection[hitIndex].GetNormal()),1.0f);
				PFD_SweptSphereNormals.DrawArrow(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),probeNormalEnd,retest?Color_SeaGreen1:Color_green);
			}
		}
	}
}
#endif


void phShapeTaperedSweptSphere::InitTaperedSweptSphere (const phSegmentV& worldSegment, ScalarV_In initialRadius, ScalarV_In finalRadius, phIntersection* intersection, int numIntersections)
{
	Assertf(IsLessThanAll(Mag(worldSegment.GetStart()),ScalarV(V_FLT_LARGE_8)),"InitTaperedSweptSphere has an impractical segment starting point %f, %f, %f",worldSegment.GetStart().GetElemf(0),worldSegment.GetStart().GetElemf(1),worldSegment.GetStart().GetElemf(2));
	Assertf(IsLessThanAll(Mag(worldSegment.GetEnd()),ScalarV(V_FLT_LARGE_8)),"InitTaperedSweptSphere has an impractical segment ending point %f, %f, %f",worldSegment.GetEnd().GetElemf(0),worldSegment.GetEnd().GetElemf(1),worldSegment.GetEnd().GetElemf(2));
	Assertf(IsGreaterThanOrEqualAll(initialRadius,ScalarV(V_ZERO)) && IsLessThanAll(initialRadius,ScalarV(V_FLT_LARGE_8)),"InitTaperedSweptSphere has an impractical initial radius %f",initialRadius.Getf());
	Assertf(IsGreaterThanOrEqualAll(finalRadius,ScalarV(V_ZERO)) && IsLessThanAll(finalRadius,ScalarV(V_FLT_LARGE_8)),"InitTaperedSweptSphere has an impractical final radius %f",finalRadius.Getf());
	m_WorldSegment.Set(worldSegment.GetStart(),worldSegment.GetEnd());
	m_InitialRadius = initialRadius.Getf();
	m_FinalRadius = finalRadius.Getf();
	phShapeBase::Init(intersection,numIntersections);
	DEBUGLOG_ONLY(DebugReplay());
}




void phShapeTaperedSweptSphere::SetupIteratorCull (phIterator& levelIterator)
{
	Vec3V segmentAtoB = Subtract(m_WorldSegment.GetEnd(), m_WorldSegment.GetStart());
	// Use the larger of the two radii to create a capsule that contains the whole tapered swept sphere
	levelIterator.InitCull_Capsule(m_WorldSegment.GetStart(),segmentAtoB,Mag(segmentAtoB),ScalarVFromF32(Max(m_InitialRadius, m_FinalRadius)));
}


bool phShapeTaperedSweptSphere::RejectBound (const LocalizedData& localizedData, const phBound& bound) const
{
	return !geomBoxes::TestCapsuleToAlignedBoxFP(VEC3V_TO_INTRIN(localizedData.m_Segment.GetStart()),VEC3V_TO_INTRIN(localizedData.m_Segment.GetEnd()),ScalarVFromF32(Max(m_InitialRadius, m_FinalRadius)).GetIntrin128(),bound.GetBoundingBoxMin().GetIntrin128(),bound.GetBoundingBoxMax().GetIntrin128());
}

bool phShapeTaperedSweptSphere::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 UNUSED_PARAM(primTypeFlags), u32 UNUSED_PARAM(primIncludeFlags), const PartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TestBoundPrimitive);
#endif
	Vec3V segmentAtoB = Subtract(localizedData.m_Segment.GetEnd(), localizedData.m_Segment.GetStart());
	ScalarV initialRadius = GetInitialRadius();
	ScalarV finalRadius = GetFinalRadius();
	ScalarV radiusGrowth = Subtract(finalRadius, initialRadius);

	// Compute the intersections of the capsule with the bound.
	IterativeCastResult result;
	if(IterativeCast(CastScalingSphereInput(localizedData.m_Segment.GetStart(),segmentAtoB,initialRadius,radiusGrowth),bound,result) && IsGreaterThanAll(result.m_IntersectionTime, ScalarV(V_ZERO)))
	{
		u16 partIndex = 0;
		phMaterialMgr::Id materialId = bound.GetMaterialId(partIndex);
		ScalarV depth = Scale(Add(Mag(segmentAtoB), radiusGrowth), Subtract(ScalarV(V_ONE), result.m_IntersectionTime));
		return AddIntersection(result.m_Position,result.m_NormalOnLocalBound,depth,result.m_IntersectionTime,materialId,partialIntersection);
	}

	return false;
}


bool phShapeTaperedSweptSphere::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 UNUSED_PARAM(polyTypeFlags), u32 UNUSED_PARAM(polyIncludeFlags), const phBoundGeometry& UNUSED_PARAM(geomBound), const PrimitivePartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TaperedSweptSphereTestPolygon);
#endif

	const Vec3V * RESTRICT vTriVertices = (const Vec3V *)(&boundVertices);
	const Vec3V v0 = vTriVertices[polygon.GetVertexIndex(0)];
	const Vec3V v1 = vTriVertices[polygon.GetVertexIndex(1)];
	const Vec3V v2 = vTriVertices[polygon.GetVertexIndex(2)];
	Vec3V triangleFaceNormal = polygon.ComputeUnitNormal(v0,v1,v2);

	Vec3V segmentA = localizedData.m_Segment.GetStart();
	Vec3V segmentB = localizedData.m_Segment.GetEnd();
	Vec3V segmentAtoB = Subtract(segmentB, segmentA);
	ScalarV initialRadius = GetInitialRadius();
	ScalarV finalRadius = GetFinalRadius();
	ScalarV radiusGrowth = Subtract(finalRadius, initialRadius);
	ScalarV svZero(V_ZERO);
	ScalarV svOne(V_ONE);

	// Do an AABB-AABB test
	const Vec3V taperedSweptSphereAABBMin = localizedData.m_Min;
	const Vec3V taperedSweptSphereAABBMax = localizedData.m_Max;
	const Vec3V triAABBMin = Min(Min(v0,v1), v2);
	const Vec3V triAABBMax = Max(Max(v0,v1), v2);
	const VecBoolV boundingBoxesIntersectV = IsLessThan(taperedSweptSphereAABBMin, triAABBMax) & IsLessThan(triAABBMin, taperedSweptSphereAABBMax);
	const BoolV boundingBoxesIntersect = boundingBoxesIntersectV.GetX() & boundingBoxesIntersectV.GetY() & boundingBoxesIntersectV.GetZ();

	// Do a tapered swept sphere-plane test
	// The start of the sweep must be in front of the  triangle plane and the end of the sweept must be behind or within the radius
	//   of the backside of the triangle plane
	const ScalarV dPlane = Dot(triangleFaceNormal, v0);
	const ScalarV dSegmentA = Subtract(Dot(triangleFaceNormal, segmentA), dPlane);
	const ScalarV dSegmentB = Subtract(Dot(triangleFaceNormal, segmentB), dPlane);
	const BoolV taperedSweptSphereIntersectsPlane = IsGreaterThan(dSegmentA,svZero) & IsLessThan(dSegmentB, finalRadius);

	// See if the swept sphere is moving away from the triangle
	const ScalarV relativeSpeedAwayFromTriangle = Subtract(Dot(triangleFaceNormal, segmentAtoB), radiusGrowth);
	const BoolV sphereIsMovingTowardsTriangle = IsLessThan(relativeSpeedAwayFromTriangle, svZero);

	// If any of the cull tests fail there can be no intersection
	if(InvertBits(boundingBoxesIntersect & taperedSweptSphereIntersectsPlane & sphereIsMovingTowardsTriangle).Getb())
	{
		return false;
	}

	// Clamp the tapered swept sphere to the triangle to prevent large numerical error from quadratic equations
	Vec3V triangleCenter = Average(triAABBMax,triAABBMin);
	ScalarV segmentLength = Mag(segmentAtoB);
	ScalarV invSegmentLength = Invert(segmentLength);
	ScalarV invSegmentLengthSquared = Scale(invSegmentLength,invSegmentLength);
	ScalarV triangleCenterT = Scale(Dot(Subtract(triangleCenter, segmentA), segmentAtoB), invSegmentLengthSquared);
	// For the triangles radius, just use the largest dimension of the bounding box. This isn't exact, but we just need to shrink the 
	//   sweep to ~100m to prevent large errors.
	ScalarV triangleRadiusT = Scale(Add(MaxElement(Subtract(triAABBMax,triAABBMin)), initialRadius), invSegmentLength);
	ScalarV sphereGrowthT = Scale(radiusGrowth, invSegmentLength);

	// Only clamp if the length isn't very small and the length of the sweep is greater than the growth during the sweep. 
	//   Growing faster than moving means the back side of the sphere is moving away from the sweep, which makes clamping it to something further
	//   than the front of the sphere impossible. I use one here as an epsilon because I already constructed it, and the clamping is only to prevent
	//   extremely long sweeps. If the user decides to create a very long swept sphere with a large radius, there isn't much we can do.
	BoolV shouldThisBeClamped = And(IsGreaterThan(segmentLength, svOne), IsGreaterThan(segmentLength, Add(radiusGrowth,svOne)));
	ScalarV clampedStartT = SelectFT(shouldThisBeClamped, svZero, Clamp(InvScale(Subtract(triangleCenterT, triangleRadiusT), Add(svOne,sphereGrowthT)), svZero, svOne));
	ScalarV clampedEndT = SelectFT(shouldThisBeClamped, svOne, Clamp(InvScale(Add(triangleCenterT, triangleRadiusT), Subtract(svOne,sphereGrowthT)), svZero, svOne));
	Vec3V clampedSegmentA = AddScaled(segmentA, segmentAtoB, clampedStartT);
	Vec3V clampedSegmentB = AddScaled(segmentA, segmentAtoB, clampedEndT);
	ScalarV clampedInitialRadius = AddScaled(initialRadius, radiusGrowth, clampedStartT);
	ScalarV clampedFinalRadius = AddScaled(initialRadius, radiusGrowth, clampedEndT);

	Vec3V positionOnTriangle;
	Vec3V normalOnTriangle;
	ScalarV clampedIntersectionT;

	if(geomTaperedSweptSphere::TaperedSweptSphereTriangleIntersect(clampedSegmentA,clampedSegmentB,clampedInitialRadius,clampedFinalRadius,v0,v1,v2,triangleFaceNormal,positionOnTriangle,normalOnTriangle,clampedIntersectionT).Getb())
	{
		// Convert the clamped T into an unclamped T
		ScalarV intersectionT = AddScaled(clampedStartT, Subtract(clampedEndT,clampedStartT), clampedIntersectionT);

		// Check if this intersection is good enough
		ScalarV depth = Scale(Add(segmentLength, radiusGrowth), Subtract(svOne, intersectionT));
		return AddPolygonIntersection(positionOnTriangle,normalOnTriangle,triangleFaceNormal,depth,intersectionT,partialIntersection);
	}

	return false;
}


// This function is culling against a BVH structure.  Note that it doesn't do any of the expanding the cull shape for trying to handle second surface
//   as this function is use for all BVHs which may not necessarily have come from a phBoundBVH.
void phShapeTaperedSweptSphere::PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler)
{
	phBvhCapsuleOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
	// Use the max of the two radii to calculate a Culling capsule that contains the tapered swept sphere.
	nodeOverlapCallback.SetCapsule(localizedData.m_Segment.GetStart(), localizedData.m_Segment.GetEnd(), ScalarVFromF32(Max(m_InitialRadius, m_FinalRadius)));
	bvhStructure.walkStacklessTree(&nodeOverlapCallback);
}


#if __PFDRAW
void phShapeTaperedSweptSphere::DrawShape (bool PF_DRAW_ONLY(retest))
{
	// Draw the test capsule, if TaperedSweptSphereSegments is turned on.
	PFD_TaperedSweptSphereSegments.DrawTaperedCapsule(VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()),VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()),m_FinalRadius,m_InitialRadius,retest?Color_aquamarine1:Color_white);	
	PFD_TaperedSweptSphereSegments.DrawArrow(VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()),VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()));
	PFD_TaperedSweptSphereSegments.DrawSphere(m_InitialRadius, VEC3V_TO_VECTOR3(m_WorldSegment.GetStart()), retest?Color_aquamarine1:Color_white);
	PFD_TaperedSweptSphereSegments.DrawSphere(m_FinalRadius, VEC3V_TO_VECTOR3(m_WorldSegment.GetEnd()), retest?Color_aquamarine1:Color_white);
}


void phShapeTaperedSweptSphere::DrawIntersections (bool PF_DRAW_ONLY(retest))
{
	if (m_NumHits==0 || !m_Intersection)
	{
		return;
	}

	if (m_Intersection)
	{
		// Draw the intersections, if TaperedSweptSphereIsects is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				PFD_TaperedSweptSphereIsects.DrawTick(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),0.1f,retest?Color_purple1:Color_red);
			}
		}

		// Draw the normals, if TaperedSweptSphereNormals is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				Vector3 probeNormalEnd(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()));
				probeNormalEnd.AddScaled(RCC_VECTOR3(m_Intersection[hitIndex].GetNormal()),1.0f);
				PFD_TaperedSweptSphereNormals.DrawArrow(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),probeNormalEnd,retest?Color_SeaGreen1:Color_green);

				// Draw the sphere at the time of intersection
				Vec3V hitPos = m_WorldSegment.GetStart() + ScalarV(m_Intersection[hitIndex].GetT()) * (m_WorldSegment.GetEnd() - m_WorldSegment.GetStart());
				float hitRad = m_InitialRadius  + m_Intersection[hitIndex].GetT()*(m_FinalRadius - m_InitialRadius);
				PFD_TaperedSweptSphereIsects.DrawSphere(hitRad, RCC_VECTOR3(hitPos), retest?Color_SeaGreen1:Color_green);
			}
		}
	}
}
#endif


#if __DEBUGLOG
void phShapeTaperedSweptSphere::DebugReplay () const
{
	Vec3V start = m_WorldSegment.GetStart();
	diagDebugLog(diagDebugLogPhysics, 'SPWA',&start);
	Vec3V end = m_WorldSegment.GetEnd();
	diagDebugLog(diagDebugLogPhysics, 'SPWB',&end);

	diagDebugLog(diagDebugLogPhysics, 'SCRI',&m_InitialRadius);
	diagDebugLog(diagDebugLogPhysics, 'SCRF',&m_FinalRadius);

	phShapeBase::DebugReplay();
}
#endif


void phShapeScalingSweptQuad::InitScalingSweptQuad (Mat33V_In rotation, const phSegmentV& worldSegment, Vec2V_In initialHalfExtents, Vec2V_In finalHalfExtents, phIntersection* intersection, int numIntersections)
{
	Assertf(IsLessThanAll(Mag(worldSegment.GetStart()),ScalarV(V_FLT_LARGE_8)),"InitTaperedSweptSphere has an impractical segment starting point %f, %f, %f",worldSegment.GetStart().GetElemf(0),worldSegment.GetStart().GetElemf(1),worldSegment.GetStart().GetElemf(2));
	Assertf(IsLessThanAll(Mag(worldSegment.GetEnd()),ScalarV(V_FLT_LARGE_8)),"InitTaperedSweptSphere has an impractical segment ending point %f, %f, %f",worldSegment.GetEnd().GetElemf(0),worldSegment.GetEnd().GetElemf(1),worldSegment.GetEnd().GetElemf(2));
	Assertf(IsGreaterThanOrEqualAll(initialHalfExtents,Vec2V(V_ZERO)) && IsLessThanAll(initialHalfExtents,Vec2V(V_FLT_LARGE_8)),"InitTaperedSweptSphere has impractical initial half-extents (%f,%f)",initialHalfExtents.GetXf(),initialHalfExtents.GetYf());
	Assertf(IsGreaterThanOrEqualAll(finalHalfExtents,Vec2V(V_ZERO)) && IsLessThanAll(finalHalfExtents,Vec2V(V_FLT_LARGE_8)),"InitTaperedSweptSphere has impractical final half-extents (%f,%f)",finalHalfExtents.GetXf(),finalHalfExtents.GetYf());
	Assertf(rotation.IsOrthonormal(ScalarVFromF32(REJUVENATE_ERROR_NEW_VEC)),"Non-orthonormal scaling swept quad matrix."
		"\n\t%f %f %f"
		"\n\t%f %f %f"
		"\n\t%f %f %f",
		MAT33V_ARG_FLOAT_RC(rotation));

	m_WorldRotation = rotation;
	m_WorldSegment.Set(worldSegment.GetStart(),worldSegment.GetEnd());
	m_InitialHalfExtents = initialHalfExtents;
	m_FinalHalfExtents = finalHalfExtents;
	m_CapsuleRadius = Sqrt(Max(MagSquared(initialHalfExtents),MagSquared(finalHalfExtents)));
	m_SweepStartIntersectionFound = false;
	phShapeBase::Init(intersection,numIntersections);
	DEBUGLOG_ONLY(DebugReplay());
}

void phShapeScalingSweptQuad::SetupIteratorCull (phIterator& levelIterator)
{
	Vec3V segmentAtoB = Subtract(m_WorldSegment.GetEnd(), m_WorldSegment.GetStart());
	levelIterator.InitCull_Capsule(m_WorldSegment.GetStart(),segmentAtoB,Mag(segmentAtoB),m_CapsuleRadius);
}

bool phShapeScalingSweptQuad::RejectBound (const LocalizedData& localizedData, const phBound& bound) const
{
	return !geomBoxes::TestCapsuleToAlignedBoxFP(VEC3V_TO_INTRIN(localizedData.m_Segment.GetStart()),VEC3V_TO_INTRIN(localizedData.m_Segment.GetEnd()),m_CapsuleRadius.GetIntrin128(),bound.GetBoundingBoxMin().GetIntrin128(),bound.GetBoundingBoxMax().GetIntrin128());
}

bool phShapeScalingSweptQuad::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 UNUSED_PARAM(primTypeFlags), u32 UNUSED_PARAM(primIncludeFlags), const PartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TestBoundPrimitive);
#endif
	const Vec3V segmentAtoB = Subtract(localizedData.m_Segment.GetEnd(), localizedData.m_Segment.GetStart());
	const Vec2V initialHalfExtents = GetInitialHalfExtents();
	const Vec2V halfExtentsGrowth = Subtract(GetFinalHalfExtents(),initialHalfExtents);
	const Vec3V initialExtents = Scale(Vec3V(ScalarV(V_ZERO),initialHalfExtents.GetY(),initialHalfExtents.GetX()), ScalarV(V_TWO));
	const Vec3V extentsGrowth = Scale(Vec3V(ScalarV(V_ZERO),halfExtentsGrowth.GetY(),halfExtentsGrowth.GetX()), ScalarV(V_TWO));

	CREATE_SIMPLE_BOUND_ON_STACK(phBoundBox,quadBound);
	quadBound.SetMargin(ScalarV(V_ZERO));
	quadBound.SetBoxSize(Vec3V(V_ONE));

	// Compute the intersections of the capsule with the bound.
	IterativeCastResult result;
	bool didCastIntersect;
	// NOTE: Boxes and cylinders create copies since we want to test their exact surface for this test. At the moment there is no official rule on whether
	//         we test the shrunk + margin shape or the exact shape. It pretty much depends on if there is a special case. 
	switch(bound.GetType())
	{
	case phBound::BOX:
	{
		CREATE_SIMPLE_BOUND_ON_STACK(phBoundBox,marginlessBox,static_cast<const phBoundBox&>(bound));
		marginlessBox.SetMargin(ScalarV(V_ZERO));
		didCastIntersect = IterativeCast(CastScalingBoundInput(localizedData.m_Segment.GetStart(),segmentAtoB,initialExtents,extentsGrowth,localizedData.m_Rotation,quadBound),marginlessBox,result);
		break;
	}
	case phBound::CYLINDER:
	{
		CREATE_SIMPLE_BOUND_ON_STACK(phBoundCylinder,marginlessCylinder,static_cast<const phBoundCylinder&>(bound));
		marginlessCylinder.SetMargin(ScalarV(V_ZERO));
		didCastIntersect = IterativeCast(CastScalingBoundInput(localizedData.m_Segment.GetStart(),segmentAtoB,initialExtents,extentsGrowth,localizedData.m_Rotation,quadBound),marginlessCylinder,result);
		break;
	}
	default:
	{
		didCastIntersect = IterativeCast(CastScalingBoundInput(localizedData.m_Segment.GetStart(),segmentAtoB,initialExtents,extentsGrowth,localizedData.m_Rotation,quadBound),bound,result);
		break;
	}
	}
	if(didCastIntersect)
	{
		if(IsGreaterThanAll(result.m_IntersectionTime, ScalarV(V_ZERO)))
		{
			// Get the material and see if it should be ignored.
			u16 partIndex = 0;
			phMaterialMgr::Id materialId = bound.GetMaterialId(partIndex);

			// See if the intersection should be set.
			ScalarV depth = Scale(Add(Mag(segmentAtoB), Mag(halfExtentsGrowth)), Subtract(ScalarV(V_ONE), result.m_IntersectionTime));

			return AddIntersection(result.m_Position,result.m_NormalOnLocalBound,depth,result.m_IntersectionTime,materialId,partialIntersection);
		}
		else
		{
			// We found a T=0 intersection
			m_SweepStartIntersectionFound = true;
		}
	}

	return false;
}


bool phShapeScalingSweptQuad::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 UNUSED_PARAM(polyTypeFlags), u32 UNUSED_PARAM(polyIncludeFlags), const phBoundGeometry& UNUSED_PARAM(geomBound), const PrimitivePartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(ScalingSweptQuadeTestPolygon);
#endif
	const Vec3V * RESTRICT vertices = (const Vec3V *)(&boundVertices);
	CREATE_SIMPLE_BOUND_ON_STACK(TriangleShape,triangleBound);
	triangleBound.SetVertices(vertices[polygon.GetVertexIndex(0)],vertices[polygon.GetVertexIndex(1)],vertices[polygon.GetVertexIndex(2)]);
	triangleBound.TriangleShape::SetMaterial(partialIntersection.GetMaterialId());
	triangleBound.SetMargin(ScalarV(V_ZERO));
	// Set a lazy centroid offset, at the moment it's just zero along with the radius for TriangleShape bounds. 
	// This is necessary for the IterativeCast algorithm to relocate the polygon near the origin for more accurate tests.
	triangleBound.phBound::SetCentroidOffset(vertices[polygon.GetVertexIndex(0)]);

	Mat34V identity(V_IDENTITY);
	return TestBoundPrimitive(localizedData, triangleBound, 0, 0, partialIntersection);
}


// This function is culling against a BVH structure.  Note that it doesn't do any of the expanding the cull shape for trying to handle second surface
//   as this function is use for all BVHs which may not necessarily have come from a phBoundBVH.
void phShapeScalingSweptQuad::PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler)
{
	phBvhCapsuleOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
	// Use the max of the two radii to calculate a Culling capsule that contains the tapered swept sphere.
	nodeOverlapCallback.SetCapsule(localizedData.m_Segment.GetStart(), localizedData.m_Segment.GetEnd(), m_CapsuleRadius);
	bvhStructure.walkStacklessTree(&nodeOverlapCallback);
}


#if __PFDRAW
void phShapeScalingSweptQuad::DrawShape (bool PF_DRAW_ONLY(retest))
{
	Mat34V transformInitial(m_WorldRotation,m_WorldSegment.GetStart());

	Vec3V upperRightInitial = Transform(transformInitial, Vec3V(ScalarV(V_ZERO), m_InitialHalfExtents.GetY(), m_InitialHalfExtents.GetX()));
	Vec3V upperLeftInitial = Transform(transformInitial, Vec3V(ScalarV(V_ZERO), m_InitialHalfExtents.GetY(), -m_InitialHalfExtents.GetX()));
	Vec3V lowerRightInitial = Transform(transformInitial, Vec3V(ScalarV(V_ZERO), -m_InitialHalfExtents.GetY(), m_InitialHalfExtents.GetX()));
	Vec3V lowerLeftInitial = Transform(transformInitial, Vec3V(ScalarV(V_ZERO), -m_InitialHalfExtents.GetY(), -m_InitialHalfExtents.GetX()));

	Mat34V transformFinal(m_WorldRotation,m_WorldSegment.GetEnd());
	Vec3V upperRightFinal = Transform(transformFinal, Vec3V(ScalarV(V_ZERO), m_FinalHalfExtents.GetY(), m_FinalHalfExtents.GetX()));
	Vec3V upperLeftFinal = Transform(transformFinal, Vec3V(ScalarV(V_ZERO), m_FinalHalfExtents.GetY(), -m_FinalHalfExtents.GetX()));
	Vec3V lowerRightFinal = Transform(transformFinal, Vec3V(ScalarV(V_ZERO), -m_FinalHalfExtents.GetY(), m_FinalHalfExtents.GetX()));
	Vec3V lowerLeftFinal = Transform(transformFinal, Vec3V(ScalarV(V_ZERO), -m_FinalHalfExtents.GetY(), -m_FinalHalfExtents.GetX()));

	PFD_ScalingSweptQuadSegments.DrawArrow(RCC_VECTOR3(upperRightInitial),RCC_VECTOR3(upperRightFinal));
	PFD_ScalingSweptQuadSegments.DrawArrow(RCC_VECTOR3(upperLeftInitial),RCC_VECTOR3(upperLeftFinal));
	PFD_ScalingSweptQuadSegments.DrawArrow(RCC_VECTOR3(lowerRightInitial),RCC_VECTOR3(lowerRightFinal));
	PFD_ScalingSweptQuadSegments.DrawArrow(RCC_VECTOR3(lowerLeftInitial),RCC_VECTOR3(lowerLeftFinal));

	PFD_ScalingSweptQuadSegments.DrawBox(RCC_MATRIX34(transformInitial), VEC3V_TO_VECTOR3(Vec3V(ScalarV(V_ZERO), m_InitialHalfExtents.GetY(), m_InitialHalfExtents.GetX())), retest?Color_aquamarine1:Color_white);
	PFD_ScalingSweptQuadSegments.DrawBox(RCC_MATRIX34(transformFinal), VEC3V_TO_VECTOR3(Vec3V(ScalarV(V_ZERO), m_FinalHalfExtents.GetY(), m_FinalHalfExtents.GetX())), retest?Color_aquamarine1:Color_white);
}


void phShapeScalingSweptQuad::DrawIntersections (bool PF_DRAW_ONLY(retest))
{
	if (m_NumHits==0 || !m_Intersection)
	{
		return;
	}

	if (m_Intersection)
	{
		// Draw the intersections, if TaperedSweptSphereIsects is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				PFD_ScalingSweptQuadIsects.DrawTick(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),0.1f,retest?Color_purple1:Color_red);
			}
		}

		// Draw the normals, if TaperedSweptSphereNormals is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				Vector3 probeNormalEnd(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()));
				probeNormalEnd.AddScaled(RCC_VECTOR3(m_Intersection[hitIndex].GetNormal()),1.0f);
				PFD_ScalingSweptQuadNormals.DrawArrow(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),probeNormalEnd,retest?Color_SeaGreen1:Color_green);

				// Draw the quad at the time of intersection
				Vec3V hitPos = m_WorldSegment.GetStart() + ScalarV(m_Intersection[hitIndex].GetT()) * (m_WorldSegment.GetEnd() - m_WorldSegment.GetStart());
				Mat34V hitMatrix(m_WorldRotation,hitPos);

				Vec2V hitQuadExtents = m_InitialHalfExtents  + ScalarV(m_Intersection[hitIndex].GetT())*(m_FinalHalfExtents - m_InitialHalfExtents);
				Vec3V hitBoxExtents = Vec3V(ScalarV(V_ZERO), hitQuadExtents.GetY(), hitQuadExtents.GetX());
				PFD_ScalingSweptQuadIsects.DrawBox(RCC_MATRIX34(hitMatrix), RCC_VECTOR3(hitBoxExtents), retest?Color_SeaGreen1:Color_green);			
			}
		}
	}
}
#endif


#if __DEBUGLOG
void phShapeScalingSweptQuad::DebugReplay () const
{
	Vec3V start = m_WorldSegment.GetStart();
	diagDebugLog(diagDebugLogPhysics, 'SPWA',&start);
	Vec3V end = m_WorldSegment.GetEnd();
	diagDebugLog(diagDebugLogPhysics, 'SPWB',&end);

	diagDebugLog(diagDebugLogPhysics, 'SCHI',&m_InitialHalfExtents);
	diagDebugLog(diagDebugLogPhysics, 'SCHF',&m_FinalHalfExtents);

	diagDebugLog(diagDebugLogPhysics, 'SCWR',&m_WorldRotation);

	phShapeBase::DebugReplay();
}
#endif

void phShapeObject::InitObject (const phBound& bound, Mat34V_In transform, Mat34V_In transformLast, phIntersection* intersection, int numIntersections)
{
	m_Bound = &bound;
	m_TransformWorld = MAT34V_TO_MATRIX34(transform);
	
	// Set the matrix transform from the current pose to the last pose. This will be used to compute the last pose in the tested object's coordinates.
	m_CurrentToLastTransform = MAT34V_TO_MATRIX34(transformLast);
	m_CurrentToLastTransform.DotTranspose(m_TransformWorld);

	// Set the cull type.
	m_ObjectCullType = phCullShape::PHCULLTYPE_BOX;

	phShapeBase::Init(intersection,numIntersections);
	DEBUGLOG_ONLY(DebugReplay());
}


void phShapeObject::ComputeCullBox (const Matrix34& shapePose, Matrix34& centroidTransform, Vector3& halfWidth)
{
	// Get the half-width and center in the shape's coordinates.
	Vector3 center;
	m_Bound->GetBoundingBoxHalfWidthAndCenter(RC_VEC3V(halfWidth),RC_VEC3V(center));

	// Compute the previous pose of the shape.
	Matrix34 lastShapePose;
	lastShapePose.Dot(m_CurrentToLastTransform,shapePose);

	// Expand the box from motion of the shape.
	geomBoxes::ExpandOBBFromMotion(RCC_MAT34V(shapePose),RCC_MAT34V(lastShapePose),RC_VEC3V(halfWidth),RC_VEC3V(center));

	// Compute the pose of the expanded shape.
	centroidTransform.Set3x3(shapePose);
	shapePose.Transform(center,centroidTransform.d);
}


void phShapeObject::ComputeCullCapsule (const Matrix34& shapePose, Vector3& capsuleStart, Vector3& capsuleAxis, float& capsuleRadius)
{
	// Get the world position of the shape's centroid.
	Vec3V center = m_Bound->GetWorldCentroid(RCC_MAT34V(shapePose));

	// Compute the previous world pose of the shape.
	Matrix34 lastShapePose;
	lastShapePose.Dot(m_CurrentToLastTransform,shapePose);

	// Get the previous world position of the shape's centroid.
	Vec3V lastCenter = m_Bound->GetWorldCentroid(RCC_MAT34V(lastShapePose));

	capsuleStart = RCC_VECTOR3(lastCenter);
	Vector3 axis(RCC_VECTOR3(center));
	axis.Subtract(capsuleStart);
	capsuleAxis = axis;
	capsuleRadius = m_Bound->GetRadiusAroundCentroid() + m_Bound->GetMargin();
}


void phShapeObject::SetupIteratorCull (phIterator& levelIterator)
{
	if(m_UserProvidedCullShape_WS != NULL)
	{
		// For object shape tests hopefully this is the more common case.
		levelIterator.SetCullShape(*m_UserProvidedCullShape_WS);
	}
	else
	{
		switch (m_ObjectCullType)
		{
			case phCullShape::PHCULLTYPE_UNSPECIFIED:
			case phCullShape::PHCULLTYPE_BOX:
			case phCullShape::PHCULLTYPE_AABB:
			case phCullShape::PHCULLTYPE_LINESEGMENT:
			case phCullShape::PHCULLTYPE_XZCIRCLE:
			case phCullShape::PHCULLTYPE_ALL:
			{
				// Default: use a box cull shape.
				Matrix34 centroidTransform;
				Vector3 halfWidth;
				ComputeCullBox(m_TransformWorld,centroidTransform,halfWidth);

				// Set the cull box.
				levelIterator.InitCull_Box(centroidTransform,halfWidth);
				break;
			}

			case phCullShape::PHCULLTYPE_CAPSULE:
			{
				Vector3 capsuleStart,capsuleAxis;
				float capsuleRadius;
				ComputeCullCapsule(m_TransformWorld,capsuleStart,capsuleAxis,capsuleRadius);
				float shaftLength = capsuleAxis.Mag();

				// Set the cull capsule.
				levelIterator.InitCull_Capsule(capsuleStart,capsuleAxis,shaftLength,capsuleRadius);
				break;
			}

			case phCullShape::PHCULLTYPE_SPHERE:
			case phCullShape::PHCULLTYPE_POINT:
			{
				// Use a sphere cull shape.
				// Compute the current and last world positions.
				Vector3 centroidOffset(VEC3V_TO_VECTOR3(m_Bound->GetCentroidOffset()));
				Vector3 worldCenter;
				m_TransformWorld.Transform(centroidOffset,worldCenter);
				Matrix34 lastTransformWorld;
				lastTransformWorld.Dot(m_CurrentToLastTransform,m_TransformWorld);
				Vector3 lastWorldCenter;
				lastTransformWorld.Transform(centroidOffset,lastWorldCenter);

				// Find the center and radius, including the current and last positions.
				Vector3 center,radius;
				radius.Set(m_Bound->GetRadiusAroundCentroid());
				radius.Add(VEC3V_TO_VECTOR3(m_Bound->GetMarginV()));
				geomSpheres::ExpandRadiusFromMotion(worldCenter,lastWorldCenter,radius,worldCenter);

				// Set the cull sphere.
				levelIterator.InitCull_Sphere(worldCenter,radius.x);
				break;
			}
		}
	}
}

bool phShapeObject::RejectBound (const LocalizedData& UNUSED_PARAM(localizedData), const phBound& UNUSED_PARAM(bound)) const
{
    // Don't bother doing a quick reject test because the physics level culling does a box-sphere test.
    return false;
}


#if __SPU
bool phShapeObject::TestBoundPrimitive (const LocalizedData& UNUSED_PARAM(localizedData), const phBound& UNUSED_PARAM(bound), u32 UNUSED_PARAM(primTypeFlags), u32 UNUSED_PARAM(primIncludeFlags), const PartialIntersection& UNUSED_PARAM(partialIntersection))
{
    return false;
}
#else
bool phShapeObject::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(TestBoundPrimitive);
#endif

	phCollisionMemory collisionMemory;
    phCollisionInput input(&collisionMemory,false);
    input.boundA = m_Bound;
    input.boundB = &bound;
	Matrix34 lastTransformLocal = m_CurrentToLastTransform;
	lastTransformLocal.Dot(localizedData.m_Transform);
    input.currentA = RCC_MAT34V(localizedData.m_Transform);
	input.lastA = RCC_MAT34V(lastTransformLocal);
	input.currentB = Mat34V(V_IDENTITY);
	input.lastB = Mat34V(V_IDENTITY);
	input.typeFlagsA = TYPE_FLAGS_ALL;
	input.includeFlagsA = INCLUDE_FLAGS_ALL;
	input.typeFlagsB = primTypeFlags;
	input.includeFlagsB = primIncludeFlags;
    input.highPriority = true;

	// TODO: This is way too much hassle to go through just to get some intersection results.
    phManifold manifold;
    manifold.SetInstanceA(NULL);
    manifold.SetInstanceB(NULL);
    manifold.SetLevelIndexA(phInst::INVALID_INDEX, phInst::INVALID_INDEX);
	manifold.SetLevelIndexB(phInst::INVALID_INDEX, phInst::INVALID_INDEX);
	manifold.SetColliderA(NULL);
	manifold.SetColliderB(NULL);

	input.rootManifold = &manifold;

	// Call the same collision function the simulator uses
	phMidphase midphaseProcessor;
	midphaseProcessor.ProcessCollision(input);

	return ConvertContactPointsToIntersections(manifold, &bound, partialIntersection);
}
#endif // __SPU


// May want to redo/rethink some of this given in the context of recent changes to the companion function in collision.cpp that this was derived from
#define USE_GETCLOSESTPOINTS_TRIANGLE_TEST 1
#define USE_TRIANGLE_PD_SOLVER 0 // Cannot use this as long as COLLISION_MAY_USE_TRIANGLE_PD_SOLVER is off - they should probably be linked if we cared to keep TriPenDepthSolve around
#define TRIANGLEPOSE_IS_IDENTITY 1
bool phShapeObject::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection)
{
#if SHAPE_TEST_TIMERS
	PF_FUNC(ObjectTestPolygon);
#endif

	const Mat34V trianglePose(V_IDENTITY);

	// Set the number of bounds in this shape test (more than one only if it is a composite).
	bool wasPolygonHit = false;
	int numBounds = 1;
	const phBound* objectBound = m_Bound;
	const phBoundComposite* compositeBound = NULL;
	Mat34V transformLocal = RCC_MAT34V(localizedData.m_Transform);
	if (m_Bound->GetType()==phBound::COMPOSITE)
	{
		compositeBound = static_cast<const phBoundComposite*>(m_Bound);
		numBounds = compositeBound->GetNumBounds();
	}

	for (int objectBoundIndex=0; objectBoundIndex<numBounds; objectBoundIndex++)
	{
		if (compositeBound)
		{
			objectBound = compositeBound->GetBound(objectBoundIndex);
			if (!objectBound)
			{
				continue;
			}

			if (compositeBound->GetTypeAndIncludeFlags())
			{
				if (!phLevelNew::MatchFlags(polyTypeFlags, polyIncludeFlags, compositeBound->GetTypeFlags(objectBoundIndex), compositeBound->GetIncludeFlags(objectBoundIndex)))
				{
					continue;
				}
			}

			Transform( transformLocal, RCC_MAT34V(localizedData.m_Transform), compositeBound->GetCurrentMatrix(objectBoundIndex) );
		}

		Assertf(objectBound->IsConvex() || geomBound.IsConvex(),"Concave bounds are only allowed in object shapetests if the user guarantees they won't collide with other BVHs through type/include flags.");
		Assertf(objectBound->IsConvex() || geomBound.IsConcave(), "Object shapetests with concave bounds aren't allowed to set TreatPolyhedralBoundsAsPrimitives to false.");

		DiscreteCollisionDetectorInterface::SimpleResult collisionResult;

#if USE_TRIANGLE_PD_SOLVER
		Vec3V edgeNormals0, edgeNormals1, edgeNormals2;
		Vec3V neighborNormals0, neighborNormals1, neighborNormals2;
#endif // USE_TRIANGLE_PD_SOLVER

		// We can't just call geomBound.GetPolygonUnitNormal() with a polygon index because that will attempt to re-fetch the
		//   polygon from the bound, and we may have a modified phPolygon here (if we're on the SPU or are using compressed
		//   vertices).
	    const rage::Vector3 polyUnitNormal = VEC3V_TO_VECTOR3(polygon.ComputeUnitNormal((Vec3V*)&boundVertices));

		// Get the vertex positions for our polygons and store them nice and continuous-like
		const Vec3V& v_boundVertices = RCC_VEC3V(boundVertices);
		Vec3V vertices0, vertices1, vertices2;
		vertices0 = (&v_boundVertices)[polygon.GetVertexIndex(0)];
		vertices1 = (&v_boundVertices)[polygon.GetVertexIndex(1)];
		vertices2 = (&v_boundVertices)[polygon.GetVertexIndex(2)];

#if USE_TRIANGLE_PD_SOLVER
		// Set up the edge normals.
#if !__SPU
		int hasNeighbor = 0;
		hasNeighbor += btConvexConcaveCollisionAlgorithm::ComputeEdgeNormal(vertices0.GetIntrin128(),vertices1.GetIntrin128(),0,
																			1,polygon,polyUnitNormal,&geomBound,edgeNormals0,neighborNormals0);

		hasNeighbor += btConvexConcaveCollisionAlgorithm::ComputeEdgeNormal(vertices1.GetIntrin128(),vertices2.GetIntrin128(),1,
																			2,polygon,polyUnitNormal,&geomBound,edgeNormals1,neighborNormals1);

		hasNeighbor += btConvexConcaveCollisionAlgorithm::ComputeEdgeNormal(vertices2.GetIntrin128(),vertices0.GetIntrin128(),2,
																			4,polygon,polyUnitNormal,&geomBound,edgeNormals2,neighborNormals2);
#else	//	!__SPU
#if !USE_GETCLOSESTPOINTS_TRIANGLE_TEST
		int hasNeighbor = 7;
#endif // !USE_GETCLOSESTPOINTS_TRIANGLE_TEST
		edgeNormals0 = RCC_VEC3V(polyUnitNormal);
		edgeNormals1 = RCC_VEC3V(polyUnitNormal);
		edgeNormals2 = RCC_VEC3V(polyUnitNormal);
		neighborNormals0 = RCC_VEC3V(polyUnitNormal);
		neighborNormals1 = RCC_VEC3V(polyUnitNormal);
		neighborNormals2 = RCC_VEC3V(polyUnitNormal);
#endif	// !__SPU		
#endif // USE_TRIANGLE_PD_SOLVER

#if USE_GETCLOSESTPOINTS_TRIANGLE_TEST
		// This code was taken from ProcessBoundVsTriangle.
		
		// Create a triangle bound for use with collision.
		// Translate the vertices of the triangle such that vertex 0 lies at the ORIGIN in the local space of the bound it's in.
		// This is only needed because some of the special case collision functions require it.
		const Vec3V localizedVertices0 = Vec3V(V_ZERO);
		const Vec3V localizedVertices1 = vertices1 - vertices0;
		const Vec3V localizedVertices2 = vertices2 - vertices0;

		// Setup the triangle shape. 
		CREATE_SIMPLE_BOUND_ON_STACK(TriangleShape,tm,RCC_VECTOR3(localizedVertices0), RCC_VECTOR3(localizedVertices1), RCC_VECTOR3(localizedVertices2));
		tm.SetIndexInBound(partialIntersection.GetPrimitiveIndex());
		tm.m_PolygonNormal = RCC_VEC3V(polyUnitNormal);
#if USE_TRIANGLE_PD_SOLVER		
		// These are only needed by the triangle penetration depth solver.
		tm.m_EdgeNormals[0] = edgeNormals0;
		tm.m_EdgeNormals[1] = edgeNormals1;
		tm.m_EdgeNormals[2] = edgeNormals2;
		tm.m_VertexNormalCodes[0] = polygon.GetVertexNormalCode(0);
		tm.m_VertexNormalCodes[1] = polygon.GetVertexNormalCode(1);
		tm.m_VertexNormalCodes[2] = polygon.GetVertexNormalCode(2);
#endif // USE_TRIANGLE_PD_SOLVER
		tm.SetMargin(geomBound.GetMarginV());

#if USE_TRIANGLE_PD_SOLVER
		DiscreteCollisionDetectorInterface::ClosestPointInput input(ScalarV(V_FLT_MAX),objectBound,&tm,phConvexIntersector::PDSOLVERTYPE_TRIANGLE,NULL);
#else // USE_TRIANGLE_PD_SOLVER
		DiscreteCollisionDetectorInterface::ClosestPointInput input(ScalarV(V_FLT_MAX),objectBound,&tm,phConvexIntersector::PDSOLVERTYPE_MINKOWSKI,NULL);
#endif // USE_TRIANGLE_PD_SOLVER
		
		input.m_transformA = transformLocal;

		// Have to setup a different matrix for the triangle since the vertices have been translated so that vertex0 is at the origin.
		const Vec3V localTrianglePosition = vertices0;
		input.m_transformB.Set3x3(trianglePose);
#if TRIANGLEPOSE_IS_IDENTITY
		input.m_transformB.SetCol3(localTrianglePosition);
#else
		input.m_transformB.SetCol3(Transform(trianglePose,localTrianglePosition));
#endif

#if USE_RELOCATE_MATRICES
		// I really don't think this is necessary but we'll keep it anyway.
		const Vec3V relocationOffset = input.m_transformB.GetCol3();
		input.m_transformA.SetCol3(input.m_transformA.GetCol3() - relocationOffset);
		input.m_transformB.SetCol3(Vec3V(V_ZERO));
#endif

		phConvexIntersector::GetClosestPoints(input,collisionResult);
		if (collisionResult.GetHasResult())
		{
			// Translate the local point back to original position.
			collisionResult.SetPointOnBInLocal(collisionResult.GetPointOnBInLocal() + localTrianglePosition);
		}

#else // USE_GETCLOSESTPOINTS_TRIANGLE_TEST

		NewCollisionObject collisionObject0;
		NewCollisionObject collisionObject1;
		collisionObject0.m_current = transformLocal;
		collisionObject0.m_last = transformLocal;
		collisionObject0.m_bound = objectBound;
		collisionObject1.m_current = trianglePose;
		collisionObject1.m_last = trianglePose;
		collisionObject1.m_bound = &geomBound;	// This member isn't actually used except for in an assert.
		NewCollisionInput cInput;
		cInput.set(&collisionObject0, &collisionObject1, ScalarV(V_FLT_MAX), NULL);

#if __ASSERT
		// Grabbing debug info for asserts within ProcessBoundVsTriangle
		const phArchetype* rootBVHinstArch = partialIntersection.GetInst() != NULL ? partialIntersection.GetInst()->GetArchetype() : NULL;
#endif

		phPairwiseCollisionProcessor collisionProcessor;
		collisionProcessor.ProcessBoundVsTriangle(collisionResult, vertices0, vertices1, vertices2, geomBound.GetMarginV(), RCC_VEC3V(polyUnitNormal), edgeNormals0, edgeNormals1, edgeNormals2,
			neighborNormals0, neighborNormals1, neighborNormals2, polygon.GetVertexNormalCode(0), polygon.GetVertexNormalCode(1), polygon.GetVertexNormalCode(2), partialIntersection.GetPrimitiveIndex(), hasNeighbor, cInput
#if __ASSERT
			, polygon, rootBVHinstArch
#endif
			);

#endif // USE_GETCLOSESTPOINTS_TRIANGLE_TEST

		if(collisionResult.GetHasResult())
		{
			const Vec3V vNormal = collisionResult.GetNormalOnBInWorld();
			const ScalarV vsDepth = Negate(collisionResult.GetDistanceV().GetX());
#if TRIANGLEPOSE_IS_IDENTITY
			const Vec3V vPosB_WS = collisionResult.GetPointOnBInLocal();
#else
			const Vec3V vPosB_WS = Transform(trianglePose,collisionResult.GetPointOnBInLocal());
#endif

			wasPolygonHit |= AddPolygonIntersection(vPosB_WS,vNormal,RCC_VEC3V(polyUnitNormal),vsDepth,ScalarV(V_ZERO),partialIntersection);
		}
	}

    return wasPolygonHit;
}

bool phShapeObject::ConvertContactPointsToIntersections (phManifold& manifold, const phBound* bound, const PartialIntersection& partialIntersection)
{
    bool anyHits = false;

#if !__SPU
	// Composite case, we just iterate over all the manifolds in the array.
	// Note that our root manifold is the zero element of the array, so we skip it
	if (manifold.CompositeManifoldsEnabled())
	{
		for (int manifoldIndex = 0; manifoldIndex < manifold.GetNumCompositeManifolds(); ++manifoldIndex)
		{
			phManifold& compositeManifold = *manifold.GetCompositeManifold(manifoldIndex);

			ConvertContactPointsToIntersections(compositeManifold, bound, partialIntersection);
		}
	}
    else if (manifold.GetNumContacts() > 0)
    {
#else // !__SPU
	Assert(!manifold.CompositeManifoldsEnabled());
	if (manifold.GetNumContacts() > 0)
	{
#endif // !__SPU

        anyHits = true;

        for (int contactIndex = 0; contactIndex < manifold.GetNumContacts(); ++contactIndex)
        {
 			phContact& contact = manifold.GetContactPoint(contactIndex);
			// Continuous collision detection can return negative depth, ensure that it is positive so this valid contact doesn't get thrown out
			float depth = Max(contact.GetDepth(), FLT_EPSILON);

			// Get the material and see if it should be ignored.
			// Note: I didn't bother if !__SPU'ing this out because this isn't going to run on the SPU without a significant amount of work anyway, and it would
			//   have required some extra work just to make it compile.  Really, the whole phShapeObject class should probably just be !__SPU'd out.
			phMaterialIndex materialIndex = 0;
			Assertf(m_Bound != manifold.GetBoundB() || manifold.GetBoundA() == manifold.GetBoundB(), "phShapeObject::ConvertContactPointsToIntersections - The manifold bounds have been swapped.");
			const phBound* boundToTest = manifold.GetBoundB();
			int boundType = boundToTest->GetType();

			// If the bound is a triangle, we need to get the original polyhedral bound from the instance
			if (boundType == phBound::TRIANGLE)
			{
				Assert(bound);
				boundToTest = bound;
				boundType = boundToTest->GetType();
			}

			if (boundType == phBound::BVH ||
				boundType == phBound::GEOMETRY 
				USE_GEOMETRY_CURVED_ONLY(|| boundType == phBound::GEOMETRY_CURVED))
			{
				PrimitivePartialIntersection polygonPartialIntersection = partialIntersection;
				polygonPartialIntersection.SetPrimitive((phPolygon::Index)contact.GetElementB(), boundToTest->GetMaterialIdFromPartIndex(contact.GetElementB()));
				AddPolygonIntersection(contact.GetLocalPosB(),contact.GetWorldNormal(),contact.GetWorldNormal(),ScalarVFromF32(depth),ScalarV(V_ZERO),polygonPartialIntersection);
			}
			else
			{
				phMaterialMgr::Id materialId = boundToTest->GetMaterialId(materialIndex);
				AddIntersection(contact.GetLocalPosB(),contact.GetWorldNormal(),ScalarVFromF32(depth),ScalarV(V_ZERO),materialId,partialIntersection);
			}
        }

#if __PFDRAW
        if (PFD_ObjectManifolds.Begin())
        {
            manifold.ProfileDraw();

            PFD_ObjectManifolds.End();
        }
#endif
    }

	manifold.RemoveAllContacts();

    return anyHits;
}

// This function is culling against a BVH structure.  Note that it doesn't do any of the expanding the cull shape for trying to handle second surface
//   as this function is use for all BVHs which may not necessarily have come from a phBoundBVH.
void phShapeObject::PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler)
{
	if(m_UserProvidedCullShape_WS != NULL)
	{
		PopulateCullerFromUserProvidedCullShape(localizedData, culler, bvhStructure);
	}
	else
	{
		switch (m_ObjectCullType)
		{
		case phCullShape::PHCULLTYPE_UNSPECIFIED:
		case phCullShape::PHCULLTYPE_BOX:
		case phCullShape::PHCULLTYPE_AABB:
		case phCullShape::PHCULLTYPE_LINESEGMENT:
		case phCullShape::PHCULLTYPE_XZCIRCLE:
		case phCullShape::PHCULLTYPE_ALL:
			{
				// Default: use a box cull shape.
				Matrix34 centroidTransform;
				Vector3 halfWidth;
				ComputeCullBox(localizedData.m_Transform,centroidTransform,halfWidth);

				// Construct the AABB around the OBB.  I think this is cheaper than TransformAABB in geometry.h.
				const Vector3 centroid = centroidTransform.d;
				const Vec3V vHalfWidth = RCC_VEC3V(halfWidth);
				const Vec3V absObbX = Abs(Scale(vHalfWidth.GetX(), RCC_VEC3V(centroidTransform.a)));
				const Vec3V absObbY = Abs(Scale(vHalfWidth.GetY(), RCC_VEC3V(centroidTransform.b)));
				const Vec3V absObbZ = Abs(Scale(vHalfWidth.GetZ(), RCC_VEC3V(centroidTransform.c)));
				const Vec3V aabbHalfWidth = Vec3V(absObbX.GetX() + absObbY.GetX() + absObbZ.GetX(), absObbX.GetY() + absObbY.GetY() + absObbZ.GetY(), absObbX.GetZ() + absObbY.GetZ() + absObbZ.GetZ());
				const Vec3V aabbMin = RCC_VEC3V(centroid) - aabbHalfWidth;
				const Vec3V aabbMax = RCC_VEC3V(centroid) + aabbHalfWidth;

				phBvhAabbOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
				nodeOverlapCallback.SetAABB(RCC_VECTOR3(aabbMin), RCC_VECTOR3(aabbMax));
				bvhStructure.walkStacklessTree(&nodeOverlapCallback);

#if __PFDRAW
				if (PFDGROUP_ShapeTests.WillDraw() && PFD_ShapeTestBoundCull.WillDraw())
				{
					Matrix34 objectPose;
					objectPose.Transpose3x4(localizedData.m_Transform);
					objectPose.Dot(m_TransformWorld);
					Matrix34 worldBoxPose(centroidTransform);
					worldBoxPose.Dot3x3(objectPose);
					objectPose.Transform(centroid,worldBoxPose.d);
					PFD_ShapeTestBoundCull.DrawBox(worldBoxPose,halfWidth);
				}
#endif

				break;
			}

		case phCullShape::PHCULLTYPE_CAPSULE:
			{
				Vector3 capsuleStart,capsuleAxis;
				float capsuleRadius;
				ComputeCullCapsule(localizedData.m_Transform,capsuleStart,capsuleAxis,capsuleRadius);

				phBvhCapsuleOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
				const Vector3 capsuleEnd = capsuleStart + capsuleAxis;
				nodeOverlapCallback.SetCapsule(RCC_VEC3V(capsuleStart), RCC_VEC3V(capsuleEnd), ScalarVFromF32(capsuleRadius));
				bvhStructure.walkStacklessTree(&nodeOverlapCallback);

#if __PFDRAW
				if (PFDGROUP_ShapeTests.WillDraw() && PFD_ShapeTestBoundCull.WillDraw())
				{
					Matrix34 objectPose;
					objectPose.Transpose3x4(localizedData.m_Transform);
					objectPose.Dot(m_TransformWorld);
					Vector3 worldCapsuleStart,worldCapsuleEnd;
					objectPose.Transform(capsuleStart,worldCapsuleStart);
					objectPose.Transform(capsuleEnd,worldCapsuleEnd);
					PFD_ShapeTestBoundCull.DrawCapsule(worldCapsuleStart,worldCapsuleEnd,capsuleRadius);
				}
#endif

				break;
			}

		case phCullShape::PHCULLTYPE_SPHERE:
		case phCullShape::PHCULLTYPE_POINT:
			{
				// Use a sphere cull shape. Get the radius, and the world center.
				Vector3 localCentroidOffset(VEC3V_TO_VECTOR3(m_Bound->GetCentroidOffset()));
				Vector3 worldCenter;
				m_TransformWorld.Transform(localCentroidOffset,worldCenter);
				float radius = m_Bound->GetRadiusAroundCentroid() + m_Bound->GetMargin();

				// Compute the previous world matrix.
				Matrix34 lastTransformWorld;
				lastTransformWorld.Dot(m_CurrentToLastTransform,m_TransformWorld);

				// Compute a sphere to surround the previous and current locations.
				Vector3 lastWorldCenter;
				lastTransformWorld.Transform(localCentroidOffset,lastWorldCenter);
				Vector3 halfDisplacement(worldCenter);
				halfDisplacement.Subtract(lastWorldCenter);
				halfDisplacement.Scale(0.5f);
				Vector3 expandedWorldCenter(lastWorldCenter);
				expandedWorldCenter.Add(halfDisplacement);
				radius += halfDisplacement.Mag();

				// Compute the tested object's pose.
				Matrix34 objectPose;
				objectPose.Transpose3x4(localizedData.m_Transform);
				objectPose.Dot(m_TransformWorld);

				// Find the expanded center in the tested object's coordinates.
				Vector3 centerInBvh;
				objectPose.UnTransform(worldCenter,centerInBvh);

				// Initialize and use the culler.
				phBvhAabbOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
				const Vec3V vRadius = Vec3VFromF32(radius);
				const Vec3V aabbMin = RCC_VEC3V(centerInBvh) - vRadius;
				const Vec3V aabbMax = RCC_VEC3V(centerInBvh) + vRadius;
				nodeOverlapCallback.SetAABB(RCC_VECTOR3(aabbMin), RCC_VECTOR3(aabbMax));
				bvhStructure.walkStacklessTree(&nodeOverlapCallback);

#if __PFDRAW
				if (PFDGROUP_ShapeTests.WillDraw() && PFD_ShapeTestBoundCull.WillDraw())
				{
					PFD_ShapeTestBoundCull.DrawSphere(radius,expandedWorldCenter);
				}
#endif

				break;
			}
		}
	}
}


#if __PFDRAW
void phShapeObject::DrawShape (bool PF_DRAW_ONLY(retest))
{
    // Draw the test capsule, if CapsuleSegments is turned on.
    if (PFD_ObjectSegments.Begin())
    {
        bool oldLighting = false;
        bool supportDrawing = PFD_SupportPoints.WillDraw() && !m_Bound->IsConcave();

        if (supportDrawing)
        {
            oldLighting = grcLighting(false);
        }
        else if (PFD_Solid.WillDraw())
        {
            oldLighting = grcLighting(true);
        }

		grcColor(retest ? Color_aquamarine1 : Color_white);

        if (!supportDrawing)
        {
            m_Bound->Draw(RCC_MAT34V(m_TransformWorld), PFD_DrawBoundMaterials.WillDraw(), PFD_Solid.WillDraw());
        }
        else
        {
            m_Bound->DrawSupport(RCC_MAT34V(m_TransformWorld));
        }

        if (PFD_Solid.WillDraw() || supportDrawing)
        {
            grcLighting(oldLighting);
        }

        PFD_ObjectSegments.End();
    }
}


void phShapeObject::DrawIntersections (bool PF_DRAW_ONLY(retest))
{
    if (m_NumHits==0 || !m_Intersection)
    {
        return;
    }

	if (m_Intersection)
	{
		// Draw the intersections.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				PFD_ObjectIsects.DrawTick(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),0.1f,retest?Color_purple1:Color_red);
			}
		}

		// Draw the normals, if CapsuleNormals is turned on.
		for (int hitIndex=0; hitIndex<m_NumHits; hitIndex++)
		{
			if (!retest || m_Intersection[hitIndex].IsAHit())
			{
				Vector3 probeNormalEnd(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()));
				probeNormalEnd.Add(RCC_VECTOR3(m_Intersection[hitIndex].GetNormal()));
				PFD_ObjectNormals.DrawArrow(RCC_VECTOR3(m_Intersection[hitIndex].GetPosition()),probeNormalEnd,retest?Color_SeaGreen1:Color_green);
			}
		}
	}
}
#endif

#if __DEBUGLOG
// PURPOSE: Record replay debugging data about all intersections.
void phShapeObject::DebugReplay () const
{
	phShapeBase::DebugReplay();
}
#endif

// phShapeBatch::phShapeGroup methods

template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::DeleteIfOwned ()
{
	if(m_OwnedShapes)
		Delete();
}

template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::SetShapes (ShapeType* shapeList, int maxNumShapes)
{
	Assert(maxNumShapes<=MAX_BATCHED_SHAPES);
	m_MaxNumShapes = (u8)maxNumShapes;
	m_NumShapes = 0;
	m_Shapes = shapeList;
}

template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::Allocate (int maxNumShapes)
{
	if (maxNumShapes>0)
	{
		SetShapes(rage_new ShapeType[maxNumShapes],maxNumShapes);
		m_OwnedShapes = true;
	}
}

template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::Delete ()
{
	m_OwnedShapes = false;
	if (m_Shapes)
	{
		delete[] m_Shapes;
	}
}

template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::Reset ()
{
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		m_Shapes[shapeIndex].Reset();
	}
}

template <class ShapeType> int phShapeBatch::phShapeGroup<ShapeType>::GetNumHits () const
{
	int numHits = 0;
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		numHits += m_Shapes[shapeIndex].GetNumHits();
	}

	return numHits;
}

#if __DEV
template <class ShapeType> bool phShapeBatch::phShapeGroup<ShapeType>::DoIntersectionsConflict(const phShapeBase& otherShape) const
{
	// Test each shape in this group against the given shape
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		if(m_Shapes[shapeIndex].DoIntersectionsConflict(otherShape))
		{
			return true;
		}
	}
	return false;
}

template <class ShapeType> bool phShapeBatch::phShapeGroup<ShapeType>::DoIntersectionsConflict(const phShapeBatch& otherBatchShape) const
{
	// Test each shape in this group against every shape in the batch test
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		if(otherBatchShape.DoIntersectionsConflict(m_Shapes[shapeIndex]))
		{
			return true;
		}
	}
	return false;
}
#endif // __DEV

#if __PFDRAW
template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::Draw (bool retest)
{
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		m_Shapes[shapeIndex].DrawShape(retest);
	}
}

template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::DrawIntersections (bool retest)
{
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		m_Shapes[shapeIndex].DrawIntersections(retest);
	}
}
#endif // __PFDRAW

template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::SetIgnoreMaterialFlags (phMaterialFlags ignoreFlags)
{
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		m_Shapes[shapeIndex].SetIgnoreMaterialFlags(ignoreFlags);
	}
}

#if __DEBUGLOG
template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::DebugReplay () const
{
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		m_Shapes[shapeIndex].DebugReplay();
	}
}
#endif // __DEBUGLOG
template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::InitLocalizedData(u8*& localizedDataBuffer) const
{
	typename ShapeType::LocalizedData*& localizedData = reinterpret_cast<typename ShapeType::LocalizedData*&>(localizedDataBuffer);
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		GetShape(shapeIndex).InitLocalizedData(localizedData[shapeIndex]);
	}
	localizedData += m_NumShapes;
}

template <class ShapeType> void phShapeBatch::phShapeGroup<ShapeType>::TransformLocalizedData(const u8*& oldLocalizedDataBuffer, u8*& newLocalizedDataBuffer, Mat34V_In oldFromNew) const
{
	const typename ShapeType::LocalizedData*& oldLocalizedData = reinterpret_cast<const typename ShapeType::LocalizedData*&>(oldLocalizedDataBuffer);
	typename ShapeType::LocalizedData*& newLocalizedData = reinterpret_cast<typename ShapeType::LocalizedData*&>(newLocalizedDataBuffer);
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		GetShape(shapeIndex).TransformLocalizedData(oldLocalizedData[shapeIndex],newLocalizedData[shapeIndex],oldFromNew);
	}
	oldLocalizedData += m_NumShapes;
	newLocalizedData += m_NumShapes;
}

template <class ShapeType> bool phShapeBatch::phShapeGroup<ShapeType>::TestBoundPrimitive (const u8*& localizedDataBuffer, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const phShapeBase::PartialIntersection& partialIntersection)
{
	const typename ShapeType::LocalizedData*& localizedData = reinterpret_cast<const typename ShapeType::LocalizedData*&>(localizedDataBuffer);
	bool wasBoundHit = false;
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		wasBoundHit |= m_Shapes[shapeIndex].TestBoundPrimitive(localizedData[shapeIndex],bound,primTypeFlags,primIncludeFlags,partialIntersection);
	}
	localizedData += m_NumShapes;
	return wasBoundHit;
}

template <class ShapeType> bool phShapeBatch::phShapeGroup<ShapeType>::TestPolygon (const u8*& localizedDataBuffer, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const phShapeBase::PrimitivePartialIntersection& partialIntersection)
{
	const typename ShapeType::LocalizedData*& localizedData = reinterpret_cast<const typename ShapeType::LocalizedData*&>(localizedDataBuffer);
	bool wasBoundHit = false;
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		wasBoundHit |= m_Shapes[shapeIndex].TestPolygon(localizedData[shapeIndex],polygon,boundVertices,polyTypeFlags,polyIncludeFlags,geomBound,partialIntersection);
	}
	localizedData += m_NumShapes;
	return wasBoundHit;
}
template <class ShapeType> bool phShapeBatch::phShapeGroup<ShapeType>::TestGeometryBound(const u8*& localizedDataBuffer, const phBoundGeometry& geomBound, Vec3V_ConstPtr vertices, u32 boundTypeFlags, u32 boundIncludeFlags, const phShapeBase::PartialIntersection& boundPartialIntersection)
{
	const typename ShapeType::LocalizedData*& localizedData = reinterpret_cast<const typename ShapeType::LocalizedData*&>(localizedDataBuffer);
	bool wasBoundHit = false;
	for (int shapeIndex=0; shapeIndex<m_NumShapes; shapeIndex++)
	{
		if(!m_Shapes[shapeIndex].RejectBound(localizedData[shapeIndex],geomBound))
		{
			wasBoundHit |= rage::TestGeometryBound(localizedData[shapeIndex],m_Shapes[shapeIndex],geomBound,vertices,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
		}
	}
	localizedData += m_NumShapes;
	return wasBoundHit;
}

// instantiating phShapeGroup to force the implementations to compile
template class phShapeBatch::phShapeGroup<phShapeSweptSphere>;
template class phShapeBatch::phShapeGroup<phShapeProbe>;
template class phShapeBatch::phShapeGroup<phShapeEdge>;
template class phShapeBatch::phShapeGroup<phShapeCapsule>;
template class phShapeBatch::phShapeGroup<phShapeSphere>;


// phShapeBatch Methods
phShapeBatch::~phShapeBatch ()
{
	m_Probes.DeleteIfOwned();
	m_Edges.DeleteIfOwned();
	m_Spheres.DeleteIfOwned();
	m_Capsules.DeleteIfOwned();
	m_SweptSpheres.DeleteIfOwned();
}

phShapeBatch::phShapeBatch ()
:	m_Probes(0),
	m_Edges(0),
	m_Spheres(0),
	m_Capsules(0),
	m_SweptSpheres(0)
{
	// Initialize the intersection to NULL.
	m_Intersection = NULL;

	// Set the cull type.
	m_BatchCullType = phCullShape::PHCULLTYPE_UNSPECIFIED;
}

phShapeBatch::phShapeBatch (int maxNumProbes, int maxNumEdges, int maxNumSpheres, int maxNumCapsules, int maxNumSweptSpheres)
:	m_Probes(maxNumProbes),
	m_Edges(maxNumEdges),
	m_Spheres(maxNumSpheres),
	m_Capsules(maxNumCapsules),
	m_SweptSpheres(maxNumSweptSpheres)
{
	// Initialize the intersection to NULL.
	m_Intersection = NULL;

	// Set the cull type.
	m_BatchCullType = phCullShape::PHCULLTYPE_UNSPECIFIED;
}


void phShapeBatch::InitProbe (const phSegmentV& worldProbe, phIntersection* intersection, int numIntersections)
{
	int probeIndex = m_Probes.m_NumShapes++;

	// Make sure the list of probes exists and is long enough.
	Assertf(m_Probes.m_Shapes,"Call GetProbeGroup().Allocate or SetProbes before initializing probes.");
	Assertf(probeIndex<m_Probes.m_MaxNumShapes,"This batch of shapes can only hold %i probes.",m_Probes.m_MaxNumShapes);

	// Initialize the probe test.
	m_Probes.m_Shapes[probeIndex].InitProbe(worldProbe,intersection,numIntersections);
}


void phShapeBatch::InitEdge (const phSegmentV& worldProbe, phIntersection* intersection, int numIntersections)
{
	int edgeIndex = m_Edges.m_NumShapes++;

	// Make sure the list of probes exists and is long enough.
	Assertf(m_Edges.m_Shapes,"Call GetEdgeGroup().Allocate or SetEdges before initializing probes.");
	Assertf(edgeIndex<m_Edges.m_MaxNumShapes,"This batch of shapes can only hold %i edges.",m_Edges.m_MaxNumShapes);

	// Initialize the edge test.
	m_Edges.m_Shapes[edgeIndex].InitEdge(worldProbe,intersection,numIntersections);
}


void phShapeBatch::InitPoint (Vec3V_In worldCenter, phIntersection* intersection, int numIntersections)
{
	int sphereIndex = m_Spheres.m_NumShapes++;

	// Make sure the list of spheres exists and is long enough.
	Assertf(m_Spheres.m_Shapes,"Call GetSphereGroup().Allocate or SetSpheres before initializing spheres.");
	Assertf(sphereIndex<m_Spheres.m_MaxNumShapes,"This batch of shapes can only hold %i spheres.",m_Spheres.m_MaxNumShapes);

	// Initialize the point test.
	m_Spheres.m_Shapes[sphereIndex].InitPoint(worldCenter,intersection,numIntersections);
}


void phShapeBatch::InitSphere (Vec3V_In worldCenter, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	int sphereIndex = m_Spheres.m_NumShapes++;

	// Make sure the list of spheres exists and is long enough.
	Assertf(m_Spheres.m_Shapes,"Call GetSphereGroup().Allocate or SetSpheres before initializing spheres.");
	Assertf(sphereIndex<m_Spheres.m_MaxNumShapes,"This batch of shapes can only hold %i spheres.",m_Spheres.m_MaxNumShapes);

	// Initialize the sphere test.
	m_Spheres.m_Shapes[sphereIndex].InitSphere(worldCenter,radius,intersection,numIntersections);
}


void phShapeBatch::InitSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	int sweptSphereIndex = m_SweptSpheres.m_NumShapes++;

	// Make sure the list of capsules exists and is long enough.
	Assertf(m_SweptSpheres.m_Shapes,"Call GetSweptSphereGroup().Allocate before initializing swept spheres.");
	Assertf(sweptSphereIndex<m_SweptSpheres.m_MaxNumShapes,"This batch of shapes can only hold %i swept spheres.",m_SweptSpheres.m_MaxNumShapes);

	// Initialize the directed capsule test.
	m_SweptSpheres.m_Shapes[sweptSphereIndex].InitSweptSphere(worldProbe,radius,intersection,numIntersections);
}

void phShapeBatch::InitCapsule (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	int capsuleIndex = m_Capsules.m_NumShapes++;

	// Make sure the list of capsules exists and is long enough.
	Assertf(m_Capsules.m_Shapes,"Call GetCapsuleGroup().Allocate or SetCapsules before initializing capsules.");
	Assertf(capsuleIndex<m_Capsules.m_MaxNumShapes,"This batch of shapes can only hold %i capsules.",m_Capsules.m_MaxNumShapes);

	// Initialize the directed capsule test.
	m_Capsules.m_Shapes[capsuleIndex].InitCapsule(worldProbe,radius,intersection,numIntersections);
}


void phShapeBatch::InitSphereAndSweptSphere (const phSegment& worldProbe, float radius, phIntersection* sphereIntersection, int numSphereIntersections, phIntersection* capsuleIntersection, int numCapsuleIntersections)
{
	// Initialize a sphere test.
	InitSphere(RCC_VEC3V(worldProbe.A),ScalarVFromF32(radius),sphereIntersection,numSphereIntersections);

	// Initialize a swept sphere (directed capsule) test.
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	InitSweptSphere(worldProbeV,ScalarVFromF32(radius),capsuleIntersection,numCapsuleIntersections);

	Vector3 axis(worldProbe.B);
	axis.Subtract(worldProbe.A);
	float length = SqrtfSafe(axis.Mag2());
	SetCullCapsule(worldProbe.A,axis,length,radius);
}


void phShapeBatch::InitSphereAndSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* sphereIntersection, int numSphereIntersections, phIntersection* capsuleIntersection, int numCapsuleIntersections)
{
	// Initialize a sphere test.
	InitSphere(worldProbe.GetStart(),radius,sphereIntersection,numSphereIntersections);

	// Initialize a swept sphere (directed capsule) test.
	InitSweptSphere(worldProbe,radius,capsuleIntersection,numCapsuleIntersections);

	Vec3V axis = Subtract(worldProbe.GetEnd(),worldProbe.GetStart());
	ScalarV length = Mag(axis);
	SetCullCapsule(VEC3V_TO_VECTOR3(worldProbe.GetStart()),VEC3V_TO_VECTOR3(axis),length.Getf(),radius.Getf());
}


void phShapeBatch::ComputeCentroid ()
{
	// Find the average position of all the shape centers.
	m_BatchWorldCenter.Zero();
	int numShapes = 0;
	for (int probeIndex=0; probeIndex<m_Probes.m_NumShapes; probeIndex++)
	{
		const phSegmentV& segment = m_Probes.m_Shapes[probeIndex].GetWorldSegment();
		m_BatchWorldCenter.Add(VEC3V_TO_VECTOR3(Average(segment.GetStart(),segment.GetEnd())));
		numShapes++;
	}

	for (int edgeIndex=0; edgeIndex<m_Edges.m_NumShapes; edgeIndex++)
	{
		const phSegmentV& segment = m_Edges.m_Shapes[edgeIndex].GetWorldSegment();
		m_BatchWorldCenter.Add(VEC3V_TO_VECTOR3(Average(segment.GetStart(),segment.GetEnd())));
		numShapes++;
	}

	for (int sphereIndex=0; sphereIndex<m_Spheres.m_NumShapes; sphereIndex++)
	{
		m_BatchWorldCenter.Add(m_Spheres.m_Shapes[sphereIndex].GetWorldCenter());
		numShapes++;
	}

	for (int capsuleIndex=0; capsuleIndex<m_Capsules.m_NumShapes; capsuleIndex++)
	{
		const phSegmentV& segment = m_Capsules.m_Shapes[capsuleIndex].GetWorldSegment();
		m_BatchWorldCenter.Add(VEC3V_TO_VECTOR3(Average(segment.GetStart(),segment.GetEnd())));
		numShapes++;
	}

	for (int sweptSphereIndex=0; sweptSphereIndex<m_SweptSpheres.m_NumShapes; sweptSphereIndex++)
	{
		const phSegmentV& segment = m_SweptSpheres.m_Shapes[sweptSphereIndex].GetWorldSegment();
		m_BatchWorldCenter.Add(VEC3V_TO_VECTOR3(Average(segment.GetStart(),segment.GetEnd())));
		numShapes++;
	}

	AssertMsg(numShapes>0, "Ignorable - no shapes in batch test");
	float invNumShapes = (numShapes>0 ? 1.0f/(float)numShapes : 1.0f);
	m_BatchWorldCenter.Scale(invNumShapes);
}


void phShapeBatch::ComputeCullSphere ()
{
	// Set the culling shape type.
	m_BatchCullType = phCullShape::PHCULLTYPE_SPHERE;

	// Find the average position of all the shape centers.
	ComputeCentroid();

	// Find the farthest distance of any shape from the average position of the centers.
	m_BatchRadius = 0.0f;
	for (int probeIndex=0; probeIndex<m_Probes.m_NumShapes; probeIndex++)
	{
		const phSegmentV& segment = m_Probes.m_Shapes[probeIndex].GetWorldSegment();
		m_BatchRadius = Max(m_BatchRadius,VEC3V_TO_VECTOR3(segment.GetStart()).Dist(m_BatchWorldCenter));
		m_BatchRadius = Max(m_BatchRadius,VEC3V_TO_VECTOR3(segment.GetEnd()).Dist(m_BatchWorldCenter));
	}

	for (int edgeIndex=0; edgeIndex<m_Edges.m_NumShapes; edgeIndex++)
	{
		const phSegmentV& segment = m_Edges.m_Shapes[edgeIndex].GetWorldSegment();
		m_BatchRadius = Max(m_BatchRadius,VEC3V_TO_VECTOR3(segment.GetStart()).Dist(m_BatchWorldCenter));
		m_BatchRadius = Max(m_BatchRadius,VEC3V_TO_VECTOR3(segment.GetEnd()).Dist(m_BatchWorldCenter));
	}

	for (int sphereIndex=0; sphereIndex<m_Spheres.m_NumShapes; sphereIndex++)
	{
		m_BatchRadius = Max(m_BatchRadius,m_Spheres.m_Shapes[sphereIndex].GetRadius()+m_Spheres.m_Shapes[sphereIndex].GetWorldCenter().Dist(m_BatchWorldCenter));
	}

	for (int capsuleIndex=0; capsuleIndex<m_Capsules.m_NumShapes; capsuleIndex++)
	{
		const phSegmentV& segment = m_Capsules.m_Shapes[capsuleIndex].GetWorldSegment();
		ScalarV capsuleRadius = m_Capsules.m_Shapes[capsuleIndex].GetRadius();
		m_BatchRadius = Max(m_BatchRadius,Add(Dist(segment.GetStart(), RCC_VEC3V(m_BatchWorldCenter)),capsuleRadius).Getf());
		m_BatchRadius = Max(m_BatchRadius,Add(Dist(segment.GetEnd(), RCC_VEC3V(m_BatchWorldCenter)),capsuleRadius).Getf());
	}

	for (int sweptSphereIndex=0; sweptSphereIndex<m_SweptSpheres.m_NumShapes; sweptSphereIndex++)
	{
		const phSegmentV& segment = m_SweptSpheres.m_Shapes[sweptSphereIndex].GetWorldSegment();
		ScalarV sweptSphereRadius = m_SweptSpheres.m_Shapes[sweptSphereIndex].GetRadius();
		m_BatchRadius = Max(m_BatchRadius,Add(Dist(segment.GetStart(), RCC_VEC3V(m_BatchWorldCenter)),sweptSphereRadius).Getf());
		m_BatchRadius = Max(m_BatchRadius,Add(Dist(segment.GetEnd(), RCC_VEC3V(m_BatchWorldCenter)),sweptSphereRadius).Getf());
	}
}


void phShapeBatch::SetCullCapsule (const Vector3& capsuleEnd, const Vector3& capsuleAxis, float length, float radius)
{
	// Set the culling shape type.
	m_BatchCullType = phCullShape::PHCULLTYPE_CAPSULE;

	m_BatchWorldCenter.AddScaled(capsuleEnd,capsuleAxis,0.5f);
	m_BatchCapsuleEnd.Set(capsuleEnd);
	m_BatchCapsuleAxis.Set(capsuleAxis);
	m_BatchCapsuleLength = length;
	m_BatchRadius = radius;
}


void phShapeBatch::ComputeAxisAlignedCullBox ()
{
	// Set the culling shape type.
	m_BatchCullType = phCullShape::PHCULLTYPE_BOX;

	// Find the average position of all the shape centers.
	ComputeCentroid();

	// Find the farthest distance of any shape from the average position along each axis.
	m_BatchBoxHalfSize.Zero();
	for (int probeIndex=0; probeIndex<m_Probes.m_NumShapes; probeIndex++)
	{
		const phSegmentV& segment = m_Probes.m_Shapes[probeIndex].GetWorldSegment();
		m_BatchBoxHalfSize = VEC3V_TO_VECTOR3(Max(RCC_VEC3V(m_BatchBoxHalfSize), Abs(Subtract(segment.GetStart(),RCC_VEC3V(m_BatchWorldCenter)))));
		m_BatchBoxHalfSize = VEC3V_TO_VECTOR3(Max(RCC_VEC3V(m_BatchBoxHalfSize), Abs(Subtract(segment.GetEnd(),RCC_VEC3V(m_BatchWorldCenter)))));
	}

	for (int edgeIndex=0; edgeIndex<m_Edges.m_NumShapes; edgeIndex++)
	{
		const phSegmentV& segment = m_Edges.m_Shapes[edgeIndex].GetWorldSegment();
		m_BatchBoxHalfSize = VEC3V_TO_VECTOR3(Max(RCC_VEC3V(m_BatchBoxHalfSize), Abs(Subtract(segment.GetStart(),RCC_VEC3V(m_BatchWorldCenter)))));
		m_BatchBoxHalfSize = VEC3V_TO_VECTOR3(Max(RCC_VEC3V(m_BatchBoxHalfSize), Abs(Subtract(segment.GetEnd(),RCC_VEC3V(m_BatchWorldCenter)))));
	}

	for (int sphereIndex=0; sphereIndex<m_Spheres.m_NumShapes; sphereIndex++)
	{
		const Vector3 sphereCenter(m_Spheres.m_Shapes[sphereIndex].GetWorldCenter());
		float sphereRadius = m_Spheres.m_Shapes[sphereIndex].GetRadius();
		for (int axisIndex=0; axisIndex<3; axisIndex++)
		{
			m_BatchBoxHalfSize[axisIndex] = Max(m_BatchBoxHalfSize[axisIndex],sphereRadius+fabsf(sphereCenter[axisIndex]-m_BatchWorldCenter[axisIndex]));
		}
	}

	for (int capsuleIndex=0; capsuleIndex<m_Capsules.m_NumShapes; capsuleIndex++)
	{
		const phSegmentV& segment = m_Capsules.m_Shapes[capsuleIndex].GetWorldSegment();
		Vec3V vCapsuleRadius = Vec3V(m_Capsules.m_Shapes[capsuleIndex].GetRadius());
		m_BatchBoxHalfSize = VEC3V_TO_VECTOR3(Max(RCC_VEC3V(m_BatchBoxHalfSize), Add(vCapsuleRadius, Abs(Subtract(segment.GetStart(),RCC_VEC3V(m_BatchWorldCenter))))));
		m_BatchBoxHalfSize = VEC3V_TO_VECTOR3(Max(RCC_VEC3V(m_BatchBoxHalfSize), Add(vCapsuleRadius, Abs(Subtract(segment.GetEnd(),RCC_VEC3V(m_BatchWorldCenter))))));
	}

	for (int sweptSphereIndex=0; sweptSphereIndex<m_SweptSpheres.m_NumShapes; sweptSphereIndex++)
	{
		const phSegmentV& segment = m_SweptSpheres.m_Shapes[sweptSphereIndex].GetWorldSegment();
		Vec3V vSweptSphereRadius = Vec3V(m_SweptSpheres.m_Shapes[sweptSphereIndex].GetRadius());
		m_BatchBoxHalfSize = VEC3V_TO_VECTOR3(Max(RCC_VEC3V(m_BatchBoxHalfSize), Add(vSweptSphereRadius, Abs(Subtract(segment.GetStart(),RCC_VEC3V(m_BatchWorldCenter))))));
		m_BatchBoxHalfSize = VEC3V_TO_VECTOR3(Max(RCC_VEC3V(m_BatchBoxHalfSize), Add(vSweptSphereRadius, Abs(Subtract(segment.GetEnd(),RCC_VEC3V(m_BatchWorldCenter))))));
	}

	// Set the cull box matrix.
	m_BatchBoxAxesWorld.Identity3x3();
	m_BatchBoxAxesWorld.d.Set(m_BatchWorldCenter);
	m_BatchRadius = m_BatchBoxHalfSize.Mag();
}


void phShapeBatch::SetCullBox (const Matrix34& boxAxes, const Vector3& boxHalfWidths)
{
	m_BatchBoxAxesWorld.Set(boxAxes);
	m_BatchBoxHalfSize.Set(boxHalfWidths);
	m_BatchBoxHalfSize.And(VEC3_ANDW);
	m_BatchRadius = boxHalfWidths.Mag();
	m_BatchWorldCenter.Set(m_BatchBoxAxesWorld.d);
	m_BatchCullType = phCullShape::PHCULLTYPE_BOX;
}


void phShapeBatch::DeleteShapes ()
{
	m_Probes.Delete();
	m_Edges.Delete();
	m_Spheres.Delete();
	m_Capsules.Delete();
	m_SweptSpheres.Delete();
}


void phShapeBatch::Reset ()
{
	phShapeBase::Reset();
	m_Probes.Reset();
	m_Edges.Reset();
	m_Spheres.Reset();
	m_Capsules.Reset();
	m_SweptSpheres.Reset();

	// Make sure we allocate enough memory for the sub-shapes localized data
	m_LocalizedDataSize =	sizeof(LocalizedData) +
							m_Probes.GetLocalizedDataSize() +
							m_Edges.GetLocalizedDataSize() +
							m_Spheres.GetLocalizedDataSize() +
							m_Capsules.GetLocalizedDataSize() +
							m_SweptSpheres.GetLocalizedDataSize();
}


void phShapeBatch::SetupIteratorCull (phIterator& levelIterator)
{
	if(m_UserProvidedCullShape_WS != NULL)
	{
		// For batch shape tests hopefully this is the more common case.
		levelIterator.SetCullShape(*m_UserProvidedCullShape_WS);
	}
	else
	{

		switch (m_BatchCullType)
		{
		case phCullShape::PHCULLTYPE_SPHERE:
			{
				levelIterator.InitCull_Sphere(m_BatchWorldCenter,m_BatchRadius);
				break;
			}

		case phCullShape::PHCULLTYPE_CAPSULE:
			{
				levelIterator.InitCull_Capsule(m_BatchCapsuleEnd,m_BatchCapsuleAxis,m_BatchCapsuleLength,m_BatchRadius);
				break;
			}

		case phCullShape::PHCULLTYPE_BOX:
			{
				levelIterator.InitCull_Box(m_BatchBoxAxesWorld,m_BatchBoxHalfSize);
				break;
			}

		default:
			{
				AssertMsg(0,"How did the batch cull type get unspecified?");
			}
		}
	}
}

int phShapeBatch::GetNumHits () const
{
	int numHits = 0;

	numHits += m_Probes.GetNumHits();
	numHits += m_Edges.GetNumHits();
	numHits += m_Spheres.GetNumHits();
	numHits += m_Capsules.GetNumHits();
	numHits += m_SweptSpheres.GetNumHits();

	return numHits;
}

bool phShapeBatch::RejectBound (const LocalizedData& localizedData, const phBound& bound) const
{
	if(m_UserProvidedCullShape_WS != NULL)
	{
		const Vec3V boundCenter = bound.GetCentroidOffset();
		const ScalarV boundRadius = bound.GetRadiusAroundCentroidV();
		return !localizedData.m_UserProvidedCullShape.CheckSphere(boundCenter, boundRadius);
	}
	else if(m_BatchCullType == phCullShape::PHCULLTYPE_BOX)
	{
		Vector3 boundCenter(VEC3V_TO_VECTOR3(bound.GetCentroidOffset()));
		float boundRadius = bound.GetRadiusAroundCentroid();
		Vector3 boxMin(m_BatchBoxHalfSize);
		boxMin.Negate();
		return !geomBoxes::TestSphereToBox(boundCenter,boundRadius,boxMin,m_BatchBoxHalfSize,localizedData.m_BoxAxes);
	}
	else
	{
		// Return true if the bound's radius around its centroid plus the batch's radius is less than or equal to the distance from the center of the
		// batch to the bound's centroid (if the batch does not overlap the bound's sphere), or return false for no quick reject if there is overlap.
		return (square(bound.GetRadiusAroundCentroid()+m_BatchRadius)<localizedData.m_Center.Dist2(VEC3V_TO_VECTOR3(bound.GetCentroidOffset())));
	}
}

bool phShapeBatch::RejectPolygon(const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices)
{
	const Vec3V* RESTRICT vertices = &RCC_VEC3V(boundVertices);
	const Vec3V vertex0 = vertices[polygon.GetVertexIndex(0)];
	const Vec3V vertex1 = vertices[polygon.GetVertexIndex(1)];
	const Vec3V vertex2 = vertices[polygon.GetVertexIndex(2)];

	const Vec3V triangleCenter = Scale(vertex0 + vertex1 + vertex2,ScalarV(V_THIRD));
	const ScalarV triangleRadius = Sqrt(Max(DistSquared(vertex0,triangleCenter),DistSquared(vertex1,triangleCenter),DistSquared(vertex2,triangleCenter)));

	if(m_UserProvidedCullShape_WS != NULL)
	{
		return !localizedData.m_UserProvidedCullShape.CheckSphere(triangleCenter,triangleRadius);
	}
	else if(m_BatchCullType == phCullShape::PHCULLTYPE_BOX)
	{
		const Vec3V boxHalfExtents = RCC_VEC3V(m_BatchBoxHalfSize);
		return !geomBoxes::TestSphereToBox(triangleCenter,triangleRadius,Negate(boxHalfExtents),boxHalfExtents,localizedData.m_BoxAxes);
	}
	else
	{
		const ScalarV combinedRadius = Add(ScalarVFromF32(m_BatchRadius),triangleRadius);
		return IsGreaterThanAll(DistSquared(RCC_VEC3V(localizedData.m_Center),triangleCenter),Scale(combinedRadius,combinedRadius)) != 0;
	}
}


void phShapeBatch::PrepareForShapeTest()
{
	if (m_UserProvidedCullShape_WS == NULL && m_BatchCullType==phCullShape::PHCULLTYPE_UNSPECIFIED)
	{
		// The cull type has not been set, so compute a surrounding sphere.
		ComputeCullSphere();
	}

#if __DEV
	bool doIntersectionsConflict =	m_Probes.DoIntersectionsConflict(*this) ||
									m_Edges.DoIntersectionsConflict(*this) ||
									m_Spheres.DoIntersectionsConflict(*this) ||
									m_SweptSpheres.DoIntersectionsConflict(*this) ||
									m_Capsules.DoIntersectionsConflict(*this);

	Assertf(!doIntersectionsConflict, "Attempting to perform a phShapeBatch test where two or more shapes share the same phIntersection memory.");
#endif // __DEV
}

void phShapeBatch::InitLocalizedData(LocalizedData& localizedData) const
{
	if(GetUserProvidedCullShape())
	{
		localizedData.m_UserProvidedCullShape = *GetUserProvidedCullShape();
	}
	else
	{
		Assert(GetCullType()!=phCullShape::PHCULLTYPE_UNSPECIFIED);
		localizedData.m_Center = GetCullSphereWorldCenter();
		localizedData.m_BoxAxes = GetCullBoxAxes();
	}

	// The localized data of the sub shapes is stored in the memory after the localized data of the phShapeBatch::LocalizedData. See phShapeBatch::Reset.
	u8* subShapeLocalizedDataBuffer = &((u8*)&localizedData)[sizeof(localizedData)];
	m_Probes.InitLocalizedData(subShapeLocalizedDataBuffer);
	m_Edges.InitLocalizedData(subShapeLocalizedDataBuffer);
	m_Spheres.InitLocalizedData(subShapeLocalizedDataBuffer);
	m_Capsules.InitLocalizedData(subShapeLocalizedDataBuffer);
	m_SweptSpheres.InitLocalizedData(subShapeLocalizedDataBuffer);
}

void phShapeBatch::TransformLocalizedData(const LocalizedData& oldLocalizedData, LocalizedData& newLocalizedData, Mat34V_In oldFromNew) const
{
	if(GetUserProvidedCullShape())
	{
		oldLocalizedData.m_UserProvidedCullShape.Localize(newLocalizedData.m_UserProvidedCullShape,oldFromNew);
	}
	else
	{
		if(GetCullType()== phCullShape::PHCULLTYPE_BOX)
		{
			UnTransformOrtho(RC_MAT34V(newLocalizedData.m_BoxAxes),oldFromNew,RCC_MAT34V(oldLocalizedData.m_BoxAxes));
		}
		else
		{
			RC_VEC3V(newLocalizedData.m_Center) = UnTransformOrtho(oldFromNew,RCC_VEC3V(oldLocalizedData.m_Center));
		}
	}

	// The localized data of the sub shapes is stored in the memory after the localized data of the phShapeBatch::LocalizedData. See phShapeBatch::Reset.
	const u8* subShapeOldLocalizedDataBuffer = &((u8*)&oldLocalizedData)[sizeof(oldLocalizedData)];
	u8* subShapeNewLocalizedDataBuffer = &((u8*)&newLocalizedData)[sizeof(newLocalizedData)];
	m_Probes.TransformLocalizedData(subShapeOldLocalizedDataBuffer, subShapeNewLocalizedDataBuffer, oldFromNew);
	m_Edges.TransformLocalizedData(subShapeOldLocalizedDataBuffer, subShapeNewLocalizedDataBuffer, oldFromNew);
	m_Spheres.TransformLocalizedData(subShapeOldLocalizedDataBuffer, subShapeNewLocalizedDataBuffer, oldFromNew);
	m_Capsules.TransformLocalizedData(subShapeOldLocalizedDataBuffer, subShapeNewLocalizedDataBuffer, oldFromNew);
	m_SweptSpheres.TransformLocalizedData(subShapeOldLocalizedDataBuffer, subShapeNewLocalizedDataBuffer, oldFromNew);
}

bool phShapeBatch::TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection)
{
	bool wasBoundHit = false;

	// The localized data of the sub shapes is stored in the memory after the localized data of the phShapeBatch::LocalizedData. See phShapeBatch::Reset.
	const u8* subShapeLocalizedDataBuffer = &((u8*)&localizedData)[sizeof(localizedData)];
	wasBoundHit |= m_Probes.TestBoundPrimitive(subShapeLocalizedDataBuffer,bound,primTypeFlags,primIncludeFlags,partialIntersection);
	wasBoundHit |= m_Edges.TestBoundPrimitive(subShapeLocalizedDataBuffer,bound,primTypeFlags,primIncludeFlags,partialIntersection);
	wasBoundHit |= m_Spheres.TestBoundPrimitive(subShapeLocalizedDataBuffer,bound,primTypeFlags,primIncludeFlags,partialIntersection);
	wasBoundHit |= m_Capsules.TestBoundPrimitive(subShapeLocalizedDataBuffer,bound,primTypeFlags,primIncludeFlags,partialIntersection);
	wasBoundHit |= m_SweptSpheres.TestBoundPrimitive(subShapeLocalizedDataBuffer,bound,primTypeFlags,primIncludeFlags,partialIntersection);

	return wasBoundHit;
}

bool phShapeBatch::TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection)
{
	bool wasPolygonHit = false;

	// The localized data of the sub shapes is stored in the memory after the localized data of the phShapeBatch::LocalizedData. See phShapeBatch::Reset.
	const u8* subShapeLocalizedDataBuffer = &((u8*)&localizedData)[sizeof(localizedData)];
	wasPolygonHit |= m_Probes.TestPolygon(subShapeLocalizedDataBuffer,polygon,boundVertices,polyTypeFlags,polyIncludeFlags,geomBound, partialIntersection);
	wasPolygonHit |= m_Edges.TestPolygon(subShapeLocalizedDataBuffer,polygon,boundVertices,polyTypeFlags,polyIncludeFlags,geomBound, partialIntersection);
	wasPolygonHit |= m_Spheres.TestPolygon(subShapeLocalizedDataBuffer,polygon,boundVertices,polyTypeFlags,polyIncludeFlags,geomBound, partialIntersection);
	wasPolygonHit |= m_Capsules.TestPolygon(subShapeLocalizedDataBuffer,polygon,boundVertices,polyTypeFlags,polyIncludeFlags,geomBound, partialIntersection);
	wasPolygonHit |= m_SweptSpheres.TestPolygon(subShapeLocalizedDataBuffer,polygon,boundVertices,polyTypeFlags,polyIncludeFlags,geomBound, partialIntersection);

	return wasPolygonHit;
}

bool phShapeBatch::TestGeometryBound(const LocalizedData& localizedData, const phBoundGeometry& geomBound, Vec3V_ConstPtr vertices, u32 boundTypeFlags, u32 boundIncludeFlags, const phShapeBase::PartialIntersection& boundPartialIntersection)
{
	bool wasBoundHit = false;

	// The localized data of the sub shapes is stored in the memory after the localized data of the phShapeBatch::LocalizedData. See phShapeBatch::Reset.
	const u8* subShapeLocalizedDataBuffer = &((u8*)&localizedData)[sizeof(localizedData)];
	wasBoundHit |= m_Probes.TestGeometryBound(subShapeLocalizedDataBuffer,geomBound,vertices,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
	wasBoundHit |= m_Edges.TestGeometryBound(subShapeLocalizedDataBuffer,geomBound,vertices,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
	wasBoundHit |= m_Spheres.TestGeometryBound(subShapeLocalizedDataBuffer,geomBound,vertices,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
	wasBoundHit |= m_Capsules.TestGeometryBound(subShapeLocalizedDataBuffer,geomBound,vertices,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
	wasBoundHit |= m_SweptSpheres.TestGeometryBound(subShapeLocalizedDataBuffer,geomBound,vertices,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);

	return wasBoundHit;	
}

// This function is culling against a BVH structure.  Note that it doesn't do any of the expanding the cull shape for trying to handle second surface
//   as this function is use for all BVHs which may not necessarily have come from a phBoundBVH.
void phShapeBatch::PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler)
{
	if(m_UserProvidedCullShape_WS != NULL)
	{
		PopulateCullerFromUserProvidedCullShape(localizedData, culler, bvhStructure);
	}
	else
	{
		// Determine the AABB to use for culling the BVH.
		Vec3V vBoxHalfWidth, vAabbCenter;
		if (m_BatchCullType==phCullShape::PHCULLTYPE_BOX)
		{
			vBoxHalfWidth = geomBoxes::ComputeAABBExtentsFromOBB(RCC_VEC3V(localizedData.m_BoxAxes.a), RCC_VEC3V(localizedData.m_BoxAxes.b), RCC_VEC3V(localizedData.m_BoxAxes.c), RCC_VEC3V(m_BatchBoxHalfSize));
			vAabbCenter = RCC_VEC3V(localizedData.m_BoxAxes.d);
		}
		else
		{
			vBoxHalfWidth = Vec3VFromF32(m_BatchRadius);
			vAabbCenter = RCC_VEC3V(localizedData.m_Center);
		}
		const Vec3V vAabbMin = Subtract(vAabbCenter, vBoxHalfWidth);
		const Vec3V vAabbMax = Add(vAabbCenter, vBoxHalfWidth);

		phBvhAabbOverlapCallback nodeOverlapCallback(&culler, &bvhStructure);
		nodeOverlapCallback.SetAABB(RCC_VECTOR3(vAabbMin), RCC_VECTOR3(vAabbMax));
		bvhStructure.walkStacklessTree(&nodeOverlapCallback);
	}
}

#if __DEV
bool phShapeBatch::DoIntersectionsConflict(const phShapeBase& otherShape) const
{
	return	m_Probes.DoIntersectionsConflict(otherShape) ||
			m_Edges.DoIntersectionsConflict(otherShape) ||
			m_Spheres.DoIntersectionsConflict(otherShape) ||
			m_Capsules.DoIntersectionsConflict(otherShape) ||
			m_SweptSpheres.DoIntersectionsConflict(otherShape);
}
#endif // __DEV

#if __PFDRAW
void phShapeBatch::DrawShape (bool retest)
{
	if (PFD_ProbeSegments.GetEnabled())
	{
		// Draw the test probes.
		m_Probes.Draw(retest);
	}

	if (PFD_EdgeSegments.GetEnabled())
	{
		// Draw the test edges.
		m_Edges.Draw(retest);
	}

	if (PFD_SphereSegments.GetEnabled())
	{
		// Draw the test spheres.
		m_Spheres.Draw(retest);
	}

	if (PFD_CapsuleSegments.GetEnabled())
	{
		// Draw the test capsules.
		m_Capsules.Draw(retest);
	}

	if (PFD_SweptSphereSegments.GetEnabled())
	{
		// Draw the test swept spheres.
		m_SweptSpheres.Draw(retest);
	}
}


void phShapeBatch::DrawIntersections (bool retest)
{
	m_Probes.DrawIntersections(retest);
	m_Edges.DrawIntersections(retest);
	m_Spheres.DrawIntersections(retest);
	m_Capsules.DrawIntersections(retest);
	m_SweptSpheres.DrawIntersections(retest);
}
#endif


void phShapeBatch::SetIgnoreMaterialFlags (phMaterialFlags ignoreFlags)
{
	m_Probes.SetIgnoreMaterialFlags(ignoreFlags);
	m_Edges.SetIgnoreMaterialFlags(ignoreFlags);
	m_Spheres.SetIgnoreMaterialFlags(ignoreFlags);
	m_Capsules.SetIgnoreMaterialFlags(ignoreFlags);
	m_SweptSpheres.SetIgnoreMaterialFlags(ignoreFlags);
}

#if __DEBUGLOG
void phShapeBatch::DebugReplay () const
{
	diagDebugLog(diagDebugLogPhysics, 'SBNP',&m_Probes.m_NumShapes);
	m_Probes.DebugReplay();

	diagDebugLog(diagDebugLogPhysics, 'SBNE',&m_Edges.m_NumShapes);
	m_Edges.DebugReplay();

	diagDebugLog(diagDebugLogPhysics, 'SBNS',&m_Spheres.m_NumShapes);
	m_Spheres.DebugReplay();

	diagDebugLog(diagDebugLogPhysics, 'SBNC',&m_Capsules.m_NumShapes);
	m_Capsules.DebugReplay();

	diagDebugLog(diagDebugLogPhysics, 'SBNs',&m_SweptSpheres.m_NumShapes);
	m_SweptSpheres.DebugReplay();

	phShapeBase::DebugReplay();
}
#endif

template <class ShapeType> phShapeTest<ShapeType>::phShapeTest ()
{
	m_NumExcludeInstances = 0;

	m_UseIncludeInstanceCallback = false;
	m_UseIncludePrimitiveCallback = false;

	m_ExcludeInstance = NULL;
	m_IncludeFlags = INCLUDE_FLAGS_ALL;
	m_TypeFlags = TYPE_FLAGS_ALL;
	m_StateIncludeFlags = phLevelBase::STATE_FLAGS_ALL;
	m_TypeExcludeFlags = TYPE_FLAGS_NONE;
	m_Level = NULL;
	m_TreatPolyhedralBoundsAsPrimitives = GetShape().TreatPolyhedralBoundsAsPrimitives();

	m_CullResults = NULL;
	m_CullState = phShapeTestCull::DEFAULT;

#if PROFILE_INDIVIDUAL_SHAPETESTS
	m_CompletionTime = -1.0f;
#endif // PROFILE_INDIVIDUAL_SHAPETESTS
}


template <class ShapeType> void phShapeTest<ShapeType>::InitPoint (const Vector3& UNUSED_PARAM(worldCenter), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitPoint on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeSphere>::InitPoint (const Vector3& worldCenter, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitPoint(RCC_VEC3V(worldCenter),intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitPoint (const Vector3& worldCenter, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitPoint(RCC_VEC3V(worldCenter),intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitPoint (Vec3V_In UNUSED_PARAM(worldCenter), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitPoint on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeSphere>::InitPoint (Vec3V_In worldCenter, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitPoint(worldCenter,intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitPoint (Vec3V_In worldCenter, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitPoint(worldCenter,intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitSphere (const Vector3& UNUSED_PARAM(worldCenter), float UNUSED_PARAM(radius), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitSphere on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeSphere>::InitSphere (const Vector3& worldCenter, float radius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitSphere(RCC_VEC3V(worldCenter),ScalarVFromF32(radius),intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitSphere (const Vector3& worldCenter, float radius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitSphere(RCC_VEC3V(worldCenter),ScalarVFromF32(radius),intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitSphere (Vec3V_In UNUSED_PARAM(worldCenter), ScalarV_In UNUSED_PARAM(radius), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitSphere on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeSphere>::InitSphere (Vec3V_In worldCenter, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitSphere(worldCenter,radius,intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitSphere (Vec3V_In worldCenter, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitSphere(worldCenter,radius,intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitProbe (const phSegment& UNUSED_PARAM(worldProbe), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitProbe on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeProbe>::InitProbe (const phSegment& worldProbe, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	m_ShapeType.InitProbe(worldProbeV,intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitProbe (const phSegment& worldProbe, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	m_ShapeType.InitProbe(worldProbeV,intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitProbe (const phSegmentV& UNUSED_PARAM(worldProbe), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitProbe on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeProbe>::InitProbe (const phSegmentV& worldProbe, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitProbe(worldProbe,intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitProbe (const phSegmentV& worldProbe, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitProbe(worldProbe,intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitEdge (const phSegment& UNUSED_PARAM(worldProbe), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitEdge on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}

template <> void phShapeTest<phShapeEdge>::InitEdge (const phSegment& worldEdge, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldEdgeV;
	worldEdgeV.Set(RCC_VEC3V(worldEdge.A),RCC_VEC3V(worldEdge.B));
	m_ShapeType.InitEdge(worldEdgeV,intersection,numIntersections);
}

template <> void phShapeTest<phShapeBatch>::InitEdge (const phSegment& worldProbe, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	m_ShapeType.InitEdge(worldProbeV,intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitEdge (const phSegmentV& UNUSED_PARAM(worldProbe), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitEdge on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}

template <> void phShapeTest<phShapeEdge>::InitEdge (const phSegmentV& worldEdge, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitEdge(worldEdge,intersection,numIntersections);
}

template <> void phShapeTest<phShapeBatch>::InitEdge (const phSegmentV& worldEdge, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitEdge(worldEdge,intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitSweptSphere (const phSegment& UNUSED_PARAM(worldProbe), float UNUSED_PARAM(radius), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitSweptSphere on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeSweptSphere>::InitSweptSphere (const phSegment& worldProbe, float radius, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	m_ShapeType.InitSweptSphere(worldProbeV,ScalarVFromF32(radius),intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitSweptSphere (const phSegment& worldProbe, float radius, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	m_ShapeType.InitSweptSphere(worldProbeV,ScalarVFromF32(radius),intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitSweptSphere (const phSegmentV& UNUSED_PARAM(worldProbe), ScalarV_In UNUSED_PARAM(radius), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitSweptSphere on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeSweptSphere>::InitSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitSweptSphere(worldProbe,radius,intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitSweptSphere(worldProbe,radius,intersection,numIntersections);
}

template <class ShapeType> void phShapeTest<ShapeType>::InitTaperedSweptSphere (const phSegment& UNUSED_PARAM(worldProbe), float UNUSED_PARAM(initialRadius), float UNUSED_PARAM(finalRadius), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitTaperedSweptSphere on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}
template <class ShapeType> void phShapeTest<ShapeType>::InitTaperedSweptSphere (const phSegmentV& UNUSED_PARAM(worldProbe), ScalarV_In UNUSED_PARAM(initialRadius), ScalarV_In UNUSED_PARAM(finalRadius), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitTaperedSweptSphere on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}
template <> void phShapeTest<phShapeTaperedSweptSphere>::InitTaperedSweptSphere (const phSegment& worldProbe, float initialRadius, float finalRadius, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	m_ShapeType.InitTaperedSweptSphere(worldProbeV,ScalarVFromF32(initialRadius), ScalarVFromF32(finalRadius),intersection,numIntersections);
}
template <> void phShapeTest<phShapeTaperedSweptSphere>::InitTaperedSweptSphere (const phSegmentV& worldProbe, ScalarV_In initialRadius, ScalarV_In finalRadius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitTaperedSweptSphere(worldProbe,initialRadius, finalRadius,intersection,numIntersections);
}
template <class ShapeType> void phShapeTest<ShapeType>::InitScalingSweptQuad (Mat33V_In UNUSED_PARAM(rotation), const phSegmentV& UNUSED_PARAM(worldProbe), Vec2V_In UNUSED_PARAM(initialHalfExtents), Vec2V_In UNUSED_PARAM(finalHalfExtents), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitScalingSweptQuad on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}

template <> void phShapeTest<phShapeScalingSweptQuad>::InitScalingSweptQuad (Mat33V_In rotation, const phSegmentV& worldProbe, Vec2V_In initialHalfExtents, Vec2V_In finalHalfExtents, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitScalingSweptQuad(rotation,worldProbe,initialHalfExtents,finalHalfExtents,intersection,numIntersections);
}

template <class ShapeType> void phShapeTest<ShapeType>::InitCapsule (const phSegment& UNUSED_PARAM(worldProbe), float UNUSED_PARAM(radius), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitCapsule on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeCapsule>::InitCapsule (const phSegment& worldProbe, float radius, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	m_ShapeType.InitCapsule(worldProbeV,ScalarVFromF32(radius),intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitCapsule (const phSegment& worldProbe, float radius, phIntersection* intersection, int numIntersections)
{
	phSegmentV worldProbeV;
	worldProbeV.Set(RCC_VEC3V(worldProbe.A),RCC_VEC3V(worldProbe.B));
	m_ShapeType.InitCapsule(worldProbeV,ScalarVFromF32(radius),intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitCapsule (const phSegmentV& UNUSED_PARAM(worldProbe), ScalarV_In UNUSED_PARAM(radius), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitCapsule on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeCapsule>::InitCapsule (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitCapsule(worldProbe,radius,intersection,numIntersections);
}


template <> void phShapeTest<phShapeBatch>::InitCapsule (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitCapsule(worldProbe,radius,intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitObject (const phBound& UNUSED_PARAM(bound), const Matrix34& UNUSED_PARAM(transform),const Matrix34& UNUSED_PARAM(lastTransform), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitObject on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}

template <class ShapeType> void phShapeTest<ShapeType>::InitObject (const phBound& bound, const Matrix34& transform, phIntersection* intersection, int numIntersections)
{
	InitObject(bound,transform,transform,intersection,numIntersections);
}


template <> void phShapeTest<phShapeObject>::InitObject (const phBound& bound, const Matrix34& transform,const Matrix34& lastTransform, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitObject(bound,RCC_MAT34V(transform),RCC_MAT34V(lastTransform),intersection,numIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitObject (const phBound& UNUSED_PARAM(bound), Mat34V_In UNUSED_PARAM(transform), Mat34V_In UNUSED_PARAM(lastTransform), phIntersection* UNUSED_PARAM(intersection), int UNUSED_PARAM(numIntersections))
{
	Errorf("Calling InitObject on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}

template <class ShapeType> void phShapeTest<ShapeType>::InitObject (const phBound& bound, Mat34V_In transform, phIntersection* intersection, int numIntersections)
{
	InitObject(bound,transform,transform,intersection,numIntersections);
}


template <> void phShapeTest<phShapeObject>::InitObject (const phBound& bound, Mat34V_In transform, Mat34V_In lastTransform, phIntersection* intersection, int numIntersections)
{
	m_ShapeType.InitObject(bound,transform,lastTransform,intersection,numIntersections);
}


//	Box tests are not yet implemented
//template <class ShapeType> void phShapeTest<ShapeType>::InitBox (Mat34V_In boxAxesWorld, Vec3V_In boxHalfSize, phIntersection* intersection, int numIntersections)
//{
//	m_ShapeType.InitBox(boxAxesWorld,boxHalfSize,intersection,numIntersections);
//}


template <class ShapeType> void phShapeTest<ShapeType>::InitSphereAndSweptSphere (const phSegment& UNUSED_PARAM(worldProbe), float UNUSED_PARAM(radius), phIntersection* UNUSED_PARAM(sphereIntersection), int UNUSED_PARAM(numSphereIntersections), phIntersection* UNUSED_PARAM(capsuleIntersection), int UNUSED_PARAM(numCapsuleIntersections))
{
	Errorf("Calling InitSphereAndSweptSphere on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeBatch>::InitSphereAndSweptSphere (const phSegment& worldProbe, float radius, phIntersection* sphereIntersection, int numSphereIntersections, phIntersection* capsuleIntersection, int numCapsuleIntersections)
{
	m_ShapeType.InitSphereAndSweptSphere(worldProbe,radius,sphereIntersection,numSphereIntersections,capsuleIntersection,numCapsuleIntersections);
}


template <class ShapeType> void phShapeTest<ShapeType>::InitSphereAndSweptSphere (const phSegmentV& UNUSED_PARAM(worldProbe), ScalarV_In UNUSED_PARAM(radius), phIntersection* UNUSED_PARAM(sphereIntersection), int UNUSED_PARAM(numSphereIntersections), phIntersection* UNUSED_PARAM(capsuleIntersection), int UNUSED_PARAM(numCapsuleIntersections))
{
	Errorf("Calling InitSphereAndSweptSphere on phShapeTest<%s> is not allowed.", GetShapeTypeString<ShapeType>());
}


template <> void phShapeTest<phShapeBatch>::InitSphereAndSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* sphereIntersection, int numSphereIntersections, phIntersection* capsuleIntersection, int numCapsuleIntersections)
{
	m_ShapeType.InitSphereAndSweptSphere(worldProbe,radius,sphereIntersection,numSphereIntersections,capsuleIntersection,numCapsuleIntersections);
}

template <class ShapeType> void phShapeTest<ShapeType>::SetExcludeInstanceList (const phInst* const * instanceList, int numInstances)
{
	Assertf(numInstances<MAX_EXCLUDE_INSTANCES,"Ignorable - too many exclude instances in SetExcludeInstanceList");
	int numInstancesToExclude = Min(numInstances,MAX_EXCLUDE_INSTANCES);
	m_NumExcludeInstances = numInstancesToExclude;
	sysMemCpy(m_ExcludeInstanceList,instanceList,numInstancesToExclude*sizeof(m_ExcludeInstanceList[0]));
}


template <class ShapeType> void phShapeTest<ShapeType>::ClearExcludeInstanceList ()
{
	m_NumExcludeInstances = 0;
}


template <class ShapeType> bool phShapeTest<ShapeType>::NotInExclusionList (const phInst* instance) const
{
	// See if the given instance is in the list.
	for (int instanceIndex=0; instanceIndex<m_NumExcludeInstances; instanceIndex++)
	{
		if (instance==m_ExcludeInstanceList[instanceIndex])
		{
			// The given instance is in the list, so return false.
			return false;
		}
	}

	// There is no exclude instance list, or the given instance is not in the list, so return true.
	return true;
}


#if !__SPU
template <class ShapeType> void phShapeTest<ShapeType>::SetIncludeInstanceCallback (const IncludeInstanceCallback& includeInstanceCallback)
{
	m_IncludeInstanceCallback = includeInstanceCallback;
	m_UseIncludeInstanceCallback = true;
}

template <class ShapeType> void phShapeTest<ShapeType>::SetUseIncludeInstanceCallback (bool use)
{
	m_UseIncludeInstanceCallback = use;
}

template <class ShapeType> void phShapeTest<ShapeType>::SetIncludePrimitiveCallback (const IncludePrimitiveCallback& includePrimitiveCallback)
{
	m_IncludePrimitiveCallback = includePrimitiveCallback;
}

template <class ShapeType> void phShapeTest<ShapeType>::SetUseIncludePrimitiveCallback (bool use)
{
	m_UseIncludePrimitiveCallback = use;
}
#endif

template <class ShapeType> int phShapeTest<ShapeType>::TestOneObject (const phBound& bound, const phInst* instance)
{
#if PROFILE_INDIVIDUAL_SHAPETESTS
	sysTimer testOneObjectTime;
#endif // PROFILE_INDIVIDUAL_SHAPETESTS

	m_ShapeType.Reset();
	m_ShapeType.PrepareForShapeTest();
	const Mat34V instanceMatrixToUse = instance ? instance->GetMatrix() : Mat34V(V_IDENTITY);
	typename ShapeType::LocalizedData& worldLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,m_ShapeType);
	m_ShapeType.InitLocalizedData(worldLocalizedData);
	typename ShapeType::LocalizedData& instanceLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,m_ShapeType);
	m_ShapeType.TransformLocalizedData(worldLocalizedData,instanceLocalizedData,instanceMatrixToUse);

	int levelIndex = phInst::INVALID_INDEX;
	int generationID = 0;
	u32 boundTypeFlags = TYPE_FLAGS_ALL;
	u32 boundIncludeFlags = INCLUDE_FLAGS_ALL;

#if !__SPU
	const phLevelNew *level = GetLevel();
	if(instance && instance->IsInLevel() && level != NULL)
	{
		levelIndex = instance->GetLevelIndex();
#if LEVELNEW_GENERATION_IDS
		generationID = level->GetGenerationID(levelIndex);
#endif	// LEVELNEW_GENERATION_IDS
		boundTypeFlags = level->GetInstanceTypeFlags(levelIndex);
		boundIncludeFlags = level->GetInstanceIncludeFlags(levelIndex);
	}
#endif // !__SPU

	phShapeBase::PartialIntersection partialIntersection;
	partialIntersection.SetInstance(instance,phHandle((u16)levelIndex,(u16)generationID),instanceMatrixToUse);
	if (TestBound(instanceLocalizedData,bound,boundTypeFlags,boundIncludeFlags,m_IncludeFlags,m_TypeFlags,partialIntersection))
	{
#if VALIDATE_SHAPETEST_RESULTS
		ShapetestValidation::ValidateIntersections(m_ShapeType);
#endif // VALIDATE_SHAPETEST_RESULTS
	}

#if PROFILE_INDIVIDUAL_SHAPETESTS
	m_CompletionTime = testOneObjectTime.GetMsTime()*1000.0f;
#endif // PROFILE_INDIVIDUAL_SHAPETESTS

	return m_ShapeType.GetNumHits();
}


template <class ShapeType> int phShapeTest<ShapeType>::TestOneObject (const phBound& bound, const phInst& instance)
{
	return TestOneObject(bound,&instance);
}


template <class ShapeType> int phShapeTest<ShapeType>::TestOneObject (const phInst& instance)
{
	FastAssert(instance.GetArchetype());
	FastAssert(instance.GetArchetype()->GetBound());
	return TestOneObject(*instance.GetArchetype()->GetBound(),instance);
}

template <class ShapeType> bool phShapeTest<ShapeType>::TestInstanceInLevel(const typename ShapeType::LocalizedData& worldLocalizedData, const phInst* instance, u32 includeFlags, u32 typeFlags, const phInst* excludeInstance, const phLevelNew* physicsLevel SPU_ONLY(,phIterator& levelIterator))
{
#if __ASSERT && !__SPU
	DIAG_CONTEXT_MESSAGE("Testing against %s", instance ? instance->GetArchetype()->GetFilename() : "");
#endif

#if !__SPU
	// See if this instance should be included.
	if (instance!=excludeInstance && NotInExclusionList(instance) && (!m_UseIncludeInstanceCallback || m_IncludeInstanceCallback(instance)))
#else
	const phInst *ppuCurInst = levelIterator.GetPPUInstance();
	if (ppuCurInst!=excludeInstance && NotInExclusionList(ppuCurInst))
#endif
	{
#if __SPU
		// Wait on an instance DMA that was kicked off earlier.  Then kick off an archetype DMA.
		levelIterator.WaitCompleteInstanceDMA();
		levelIterator.InitiateArchetypeDMA(levelIterator.GetInstanceBuffer()->GetArchetype());
#endif

		// Get the level index
#if __SPU
		// 'instance' is a PPU addr 
		const int levelIndex = levelIterator.GetInstanceBuffer()->GetLevelIndex();
		const u32 instanceTypeFlags = levelIterator.GetCurObjectData()->m_CachedArchTypeFlags;
		const u32 instanceIncludeFlags = levelIterator.GetCurObjectData()->m_CachedArchIncludeFlags;
		ASSERT_ONLY(const u32 instanceStateFlags = BIT(levelIterator.GetCurObjectData()->GetState()));
#else
		const int levelIndex = instance->GetLevelIndex();
		const u32 instanceTypeFlags = physicsLevel->GetInstanceTypeFlags(levelIndex);
		const u32 instanceIncludeFlags = physicsLevel->GetInstanceIncludeFlags(levelIndex);
		ASSERT_ONLY(const u32 instanceStateFlags = BIT(physicsLevel->GetState(levelIndex)));
#endif

#if __SPU && LEVELNEW_GENERATION_IDS
		// Find the starting address of the 16-byte chunk that contains the generation ID that we want, and DMA in that entire chunk.
		u8 generationIDBuffer[16];
		u32 mainMemoryGenerationIDAddress = (u32)(&physicsLevel->GetGenerationIDRef(levelIndex));
		const u8 *ppuGenerationIDs = (u8 *)(mainMemoryGenerationIDAddress & ~15);
		cellDmaLargeGet(generationIDBuffer, (uint64_t)(ppuGenerationIDs), 16, DMA_TAG(15), 0, 0);
#endif

		// This object's instance is not the exclude instance. Transform the test parameters to the object's coordinates.
		// NOTE: Store a deep copy of the instance matrix in case we're on a separate thread. Otherwise we could localize with
		//         one matrix, and unlocalize with another.
		const Mat34V worldFromInstance = instance->GetMatrix();
		typename ShapeType::LocalizedData& instanceLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,m_ShapeType);
		m_ShapeType.TransformLocalizedData(worldLocalizedData,instanceLocalizedData,worldFromInstance);

		Assertf(worldFromInstance.IsOrthonormal3x3(ScalarVFromF32(REJUVENATE_ERROR_NEW_VEC)),"Non-orthonormal instance matrix on '%s' found during shapetest."
			"\n\t%f %f %f"
			"\n\t%f %f %f"
			"\n\t%f %f %f",
			instance->GetArchetype()->GetFilename(), MAT33V_ARG_FLOAT_RC(worldFromInstance.GetMat33ConstRef()));

#if __DEBUGLOG
		if (diagDebugLogIsActive(diagDebugLogPhysics))
		{
			diagDebugLog(diagDebugLogPhysics, 'STiM', &RCC_MATRIX34(worldFromInstance));
			m_ShapeType.DebugReplay();
		}
#endif

#if __SPU
		levelIterator.WaitCompleteArchetypeDMA();

		levelIterator.InitiateBoundDMA(levelIterator.GetArchetypeBuffer()->GetBound());
		levelIterator.WaitCompleteBoundDMA();
#endif

		// Get this object's bound.
#if !__SPU
		FastAssert(instance->GetArchetype());
		FastAssert(instance->GetArchetype()->GetBound());
		const phBound& bound = *instance->GetArchetype()->GetBound();
#else
		const phBound &bound = *levelIterator.GetBoundBuffer();
#endif

		phShapeBase::PartialIntersection partialIntersection;

		// Try a quick reject test, and if it fails then find the number of hits on the given bound.
#if !__SPU
#if LEVELNEW_GENERATION_IDS
		const int generationID = physicsLevel->GetGenerationID(levelIndex);
#else
		const int generationID = 0;
#endif
		partialIntersection.SetInstance(instance,phHandle((u16)levelIndex,(u16)generationID),worldFromInstance);
#else
#if LEVELNEW_GENERATION_IDS
		cellDmaWaitTagStatusAll(DMA_MASK(15));
		const int generationID = (int)(*(u16 *)(&generationIDBuffer[mainMemoryGenerationIDAddress & 15]));
#else
		const int generationID = 0;
#endif
		partialIntersection.SetInstance(ppuCurInst,phHandle((u16)levelIndex,(u16)generationID),worldFromInstance);
#endif
		Assert(phLevelNew::MatchFlags(instanceTypeFlags,instanceIncludeFlags,typeFlags,includeFlags));
		Assert((instanceStateFlags & GetStateIncludeFlags()) != 0);
		if (!m_ShapeType.RejectBoundFromLevelCull(instanceLocalizedData,bound) && TestBound(instanceLocalizedData,bound,instanceTypeFlags,instanceIncludeFlags,includeFlags,typeFlags,partialIntersection))

		{
			if(m_ShapeType.IsBooleanTest())
			{
				// If we hit the bound and this is a boolean test, just return.
				return false;
			}
		}
	}

	return true;
}

template <class ShapeType> int phShapeTest<ShapeType>::TestInLevel (const phInst* excludeInstance
#if ENABLE_PHYSICS_LOCK && __SPU
																	, phIterator& levelIterator
#endif	// ENABLE_PHYSICS_LOCK && __SPU
																	, u32 includeFlags, u32 typeFlags, u8 stateIncludeFlags, u32 typeExcludeFlags, const phLevelNew* level
																	)
{
#if PROFILE_INDIVIDUAL_SHAPETESTS
	sysTimer testInLevelTime;
#endif // PROFILE_INDIVIDUAL_SHAPETESTS

#if __SPU || EMULATE_SPU
	u8 instanceBuffer[sizeof(phInst)];
	u8 archetypeBuffer[sizeof(phArchetypeDamp)];
	u8 boundBuffer[phBound::MAX_BOUND_SIZE];			
#endif // __SPU || EMULATE_SPU

	// Create a physics level iterator (if there is a physics level lock it is only used on PS3).
#if ENABLE_PHYSICS_LOCK
#	if !__SPU
	phIterator levelIterator(phIterator::PHITERATORLOCKTYPE_NOLOCK);
#	endif	// !__SPU
#else	// ENABLE_PHYSICS_LOCK
	phIterator levelIterator;
#endif	// ENABLE_PHYSICS_LOCK

#if __SPU || EMULATE_SPU
	// Set up the iterator with the information on where to put instance 
	levelIterator.SetInstanceBuffer(instanceBuffer, sizeof(phInst));
	levelIterator.SetArchetypeBuffer(archetypeBuffer, sizeof(phArchetypeDamp));
	levelIterator.SetBoundBuffer(boundBuffer, phBound::MAX_BOUND_SIZE);
#endif // __SPU || EMULATE_SPU

	// Initialize the number of hits.
	m_ShapeType.Reset();
	m_ShapeType.PrepareForShapeTest();

	typename ShapeType::LocalizedData& worldLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,m_ShapeType);
	m_ShapeType.InitLocalizedData(worldLocalizedData);

	// Set the pointer to the physics level. If none is passed in, use the active level.
	const phLevelNew* physicsLevel = level;
#if !__SPU
	if (level == NULL)
	{
		physicsLevel = PHLEVEL;
	}
#endif // !__SPU

	// This ensures we won't crash on a NULL/PPU pointer
#if __SPU
	if(!Verifyf(GetCullState() == phShapeTestCull::DEFAULT, "We don't support non-default cull states on SPU currently."))
#else // __SPU
	if(!Verifyf(GetCullState() == phShapeTestCull::DEFAULT || (m_CullResults != NULL && m_CullResults->GetIncludeInstances()),"Using shapetest cull settings that don't make sense, changing cullstate to default."))
#endif // __SPU
	{
		SetCullState(phShapeTestCull::DEFAULT);
	}

	// Determine if we're culling from the level or reading from the saved cull results
	// TODO: These should just be separate functions to reduce branches and increase clarity. 
	if(GetCullState() != phShapeTestCull::READ)
	{
		// Set the level iterator's culling parameters.
		m_ShapeType.SetupIteratorCull(levelIterator);

		// Set the level iterator's flags.
		levelIterator.SetIncludeFlags(includeFlags);
		levelIterator.SetTypeExcludeFlags(typeExcludeFlags);
		levelIterator.SetTypeFlags(typeFlags);
		levelIterator.SetStateIncludeFlags(stateIncludeFlags);

		if(GetCullState() == phShapeTestCull::WRITE)
		{
			m_CullResults->ResetForWrite();
		}
		FastAssert(physicsLevel);

#if !__SPU
		levelIterator.GetLock(phIterator::PHITERATORLOCKTYPE_READLOCK);
#endif // __SPU
		for(u16 levelIndex = physicsLevel->GetFirstCulledObject(levelIterator); levelIndex != phInst::INVALID_INDEX; levelIndex = physicsLevel->GetNextCulledObject(levelIterator))
		{
			// Save the new level index
#if !__SPU && !EMULATE_SPU 
			const phInst* instance = PHLEVEL->GetInstance(levelIndex);
#else //  !__SPU && !EMULATE_SPU
			const phInst* instance = levelIterator.GetInstanceBuffer();
#endif //  !__SPU && !EMULATE_SPU
			
			if(GetCullState() == phShapeTestCull::WRITE)
			{
				// Save the instances if the user wants it
				Assert(m_CullResults->GetIncludeInstances());
				m_CullResults->AddInstance(instance);
			}

			if(!TestInstanceInLevel(worldLocalizedData, instance, includeFlags, typeFlags, excludeInstance, physicsLevel SPU_ONLY(,levelIterator)))
			{
				break;
			}
		}
#if VALIDATE_SHAPETEST_RESULTS
		ShapetestValidation::ValidateIntersections(m_ShapeType);
#endif // VALIDATE_SHAPETEST_RESULTS

#if !__SPU
		levelIterator.ReleaseLock();
#endif // !__SPU
	}
	else
	{
		// Loop over and test the saved culled instances until we run out, or TestInstanceInLevel returns false
		m_CullResults->ResetForRead();
		u16 numCulledInstances = m_CullResults->GetNumInstances();
		for(u16 cullInstanceIndex = 0; cullInstanceIndex < numCulledInstances; ++cullInstanceIndex)
		{
			m_CullResults->SetCurrentInstanceIndex(cullInstanceIndex);
			if(const phInst* instance = m_CullResults->GetCurrentInstance()) 
			{
				// We need to recheck the instance against the shapetest filters in case either has changed since the instances were saved. 
				u16 levelIndex = instance->GetLevelIndex();
				if(	physicsLevel->LegitLevelIndex(levelIndex) && 
					phLevelNew::MatchFlags(typeFlags,includeFlags,physicsLevel->GetInstanceTypeFlags(levelIndex),physicsLevel->GetInstanceIncludeFlags(levelIndex)) &&
					(BIT(physicsLevel->GetState(levelIndex)) & GetStateIncludeFlags()) != 0)
				{
					if(!TestInstanceInLevel(worldLocalizedData, instance, includeFlags, typeFlags, excludeInstance, physicsLevel SPU_ONLY(,levelIterator)))
					{
						break;
					}
				}
			}
		}
#if VALIDATE_SHAPETEST_RESULTS
		ShapetestValidation::ValidateIntersections(m_ShapeType);
#endif // VALIDATE_SHAPETEST_RESULTS
	}

#if __PFDRAW
	// Draw the shape (active only with profile drawing turned on).
	if (PFDGROUP_ShapeTests.WillDraw())
	{
		m_ShapeType.DrawShape();
		m_ShapeType.DrawIntersections();
		if (PFD_PhysicsLevelCullShape.WillDraw())
		{
			levelIterator.DrawCullShape();
		}
	}
#endif // __PFDRAW

#if __DEBUGLOG
	if (diagDebugLogIsActive(diagDebugLogPhysics))
	{
		m_ShapeType.DebugReplay();
	}
#endif // __DEBUGLOG

#if PROFILE_INDIVIDUAL_SHAPETESTS
	m_CompletionTime = testInLevelTime.GetMsTime()*1000.0f;
#endif // PROFILE_INDIVIDUAL_SHAPETESTS

	// Return the number of hits (it's possible to have more than one hit per hit object).
	return m_ShapeType.GetNumHits();
}


#	if !__SPU || !ENABLE_PHYSICS_LOCK
template <class ShapeType> int phShapeTest<ShapeType>::TestInLevel ()
{
	return TestInLevel(m_ExcludeInstance, m_IncludeFlags, m_TypeFlags, m_StateIncludeFlags, m_TypeExcludeFlags, m_Level);
}
#	else	// !__SPU || ENABLE_PHYSICS_LOCK
template <class ShapeType> int phShapeTest<ShapeType>::TestInLevel (phIterator& levelIterator)
{
	return TestInLevel(m_ExcludeInstance, levelIterator, m_IncludeFlags, m_TypeFlags, m_StateIncludeFlags, m_TypeExcludeFlags, m_Level);
}
#	endif	// !__SPU || ENABLE_PHYSICS_LOCK


template <class ShapeType> bool phShapeTest<ShapeType>::TestBound (const typename ShapeType::LocalizedData& boundLocalizedData, const phBound& bound, u32 boundTypeFlags, u32 boundIncludeFlags, u32 includeFlags, u32 typeFlags, const phShapeBase::PartialIntersection& boundPartialIntersection)
{
	bool wasBoundHit = false;

	switch (bound.GetType())
	{
	case phBound::SPHERE:
	case phBound::CAPSULE:
	case phBound::BOX:
	case phBound::DISC:
	case phBound::CYLINDER:
		{
			wasBoundHit = m_ShapeType.TestBoundPrimitive(boundLocalizedData,bound,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
			break;
		}
#if USE_GEOMETRY_CURVED
	case phBound::GEOMETRY_CURVED:
		{
#if __SPU
			const phBoundCurvedGeometry& curvedGeomBound = *static_cast<const phBoundCurvedGeometry*>(&bound);
			phBoundCurvedGeometry& mutableCurvedGeomBound = *const_cast<phBoundCurvedGeometry*>(&curvedGeomBound);

			const int numCurvedEdges = mutableCurvedGeomBound.GetNumCurvedEdges();
			phCurvedEdge *edgeBuffer = NULL;
			if (numCurvedEdges > 0)
			{
				// DMA the curved edges over from PPU memory.
				edgeBuffer = Alloca(phCurvedEdge, numCurvedEdges);
				cellDmaLargeGet(edgeBuffer, (uint64_t)(&mutableCurvedGeomBound.GetCurvedEdge(0)), numCurvedEdges * sizeof(phCurvedEdge), DMA_TAG(15), 0, 0);
			}
			mutableCurvedGeomBound.SetCurvedEdges(edgeBuffer);

			const int numCurvedFaces = mutableCurvedGeomBound.GetNumCurvedFaces();
			phCurvedFace *faceBuffer = NULL;
			if(numCurvedFaces > 0)
			{
				// DMA the curved faces over from PPU memory.
				// @RSNE @MTD make sure we are aligned on the size of phCurvedFace
				faceBuffer= AllocaAligned(phCurvedFace, numCurvedFaces, __alignof(phCurvedFace));
				cellDmaLargeGet(faceBuffer, (uint64_t)(&mutableCurvedGeomBound.GetCurvedFace(0)), numCurvedFaces * sizeof(phCurvedFace), DMA_TAG(15), 0, 0);
			}
			mutableCurvedGeomBound.SetCurvedFaces(faceBuffer);
#endif

			// Intentional fall-through.
		}
#endif // USE_GEOMETRY_CURVED
	case phBound::GEOMETRY:
		{
			const phBoundGeometry& geomBound = *static_cast<const phBoundGeometry*>(&bound);

#if __SPU
			// Handle any DMA'ing of vertices (compressed or not) to the SPU.
			phBoundGeometry& mutableGeomBound = *const_cast<phBoundGeometry*>(&geomBound);
#if COMPRESSED_VERTEX_METHOD == 0
			Vector3 *vertexBuffer = Alloca(Vector3, geomBound.GetNumVertices());
			cellDmaLargeGet(vertexBuffer, (uint64_t)(&geomBound.GetVertexPointer()[0]), geomBound.GetNumVertices() * sizeof(Vector3), DMA_TAG(15), 0, 0);
			mutableGeomBound.m_Vertices = vertexBuffer;
#else
			CompressedVertexType *compressedVertexBuffer = Alloca(CompressedVertexType, geomBound.GetNumVertices() * 3);
			cellDmaLargeGet(compressedVertexBuffer, (uint64_t)(&geomBound.GetCompressedVertexPointer()[0]), 3 * geomBound.GetNumVertices() * sizeof(CompressedVertexType), DMA_TAG(15), 0, 0);
			mutableGeomBound.m_CompressedVertices = compressedVertexBuffer;
#endif
			cellDmaWaitTagStatusAll(DMA_MASK(15));
#endif	// __SPU

#if COMPRESSED_VERTEX_METHOD == 0
			Vec3V_ConstPtr vertices = geomBound.GetVertexPointer();
			FastAssert(vertices);
#else
			// Let's decompress the vertices into a local buffer.
			Vec3V_Ptr vertices = Alloca(Vec3V, geomBound.GetNumVertices());
			// TODO: This would be the perfect opportunity to use an as-of-yet-nonexistent batch-decompession feature.
			for(int vertexIndex = geomBound.GetNumVertices() - 1; vertexIndex >= 0; --vertexIndex)
			{
				vertices[vertexIndex] = geomBound.GetCompressedVertex(vertexIndex);
			}
#endif

#if __SPU
			phPolygon *polygonBuffer = NULL;
			const int numPolygons = geomBound.GetNumPolygons();
			if (numPolygons > 0)
			{
				// DMA the polygons over from PPU memory.
				polygonBuffer = Alloca(phPolygon, numPolygons);
				cellDmaLargeGet(polygonBuffer, (uint64_t)(&geomBound.GetPolygon(0)), numPolygons * sizeof(phPolygon), DMA_TAG(15), 0, 0);

				u8 *polygonMaterialIndexBuffer = Alloca(u8, numPolygons);
				cellDmaLargeGet(polygonMaterialIndexBuffer, (uint64_t)(&geomBound.m_PolyMatIndexList[0]), numPolygons * sizeof(u8), DMA_TAG(15), 0, 0);
				mutableGeomBound.m_PolyMatIndexList = polygonMaterialIndexBuffer;
				mutableGeomBound.m_Polygons = polygonBuffer;
			}

			const int numMaterials = geomBound.m_NumMaterials;
			if(numMaterials > 0)
			{
				phMaterialMgr::Id *materialIdBuffer = Alloca(phMaterialMgr::Id, numMaterials);
				cellDmaLargeGet(materialIdBuffer, (uint64_t)(&geomBound.m_MaterialIds[0]), numMaterials * sizeof(phMaterialMgr::Id), DMA_TAG(15), 0, 0);
				mutableGeomBound.m_MaterialIds = materialIdBuffer;

			}

			cellDmaWaitTagStatusAll(DMA_MASK(15));
#endif	// __SPU

			if(!GetTreatPolyhedralBoundsAsPrimitives())
			{
				wasBoundHit |= rage::TestGeometryBound(boundLocalizedData,GetShape(),geomBound,vertices,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
			}

			if (USE_GEOMETRY_CURVED_ONLY(bound.GetType()==phBound::GEOMETRY_CURVED ||) GetTreatPolyhedralBoundsAsPrimitives())
			{
				// This curved geometry bound just had its flat triangles tested, if it has any. Test the curved parts next.
				wasBoundHit |= m_ShapeType.TestBoundPrimitive(boundLocalizedData,bound,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
			}

			break;
		}

	case phBound::BVH:
		{
			const phBoundGeometry& geomBound = *static_cast<const phBoundGeometry*>(&bound);

#if __SPU
			// DMA material-related data from PPU memory.
			phBoundGeometry& mutableGeomBound = *const_cast<phBoundGeometry*>(&geomBound);
			const int numMaterials = geomBound.m_NumMaterials;
			phMaterialMgr::Id *materialIdBuffer = Alloca(phMaterialMgr::Id, numMaterials);
			cellDmaLargeGet(materialIdBuffer, (uint64_t)(&geomBound.m_MaterialIds[0]), numMaterials * sizeof(phMaterialMgr::Id), DMA_TAG(15), 0, 0);
			mutableGeomBound.m_MaterialIds = materialIdBuffer;
#endif

			// Why do we have this check here?  Do we sometimes come across bounds here that don't have any vertex pointer?
#if COMPRESSED_VERTEX_METHOD == 0
			const Vector3* boundVertices = (const Vector3*)geomBound.GetVertexPointer();
			if (boundVertices)
#else
			if(geomBound.GetCompressedVertexPointer())
#endif
			{
#if __SPU
				// On the SPU we need to DMA in the btOptimizedBVH structure.
				Assert(bound.GetType()==phBound::BVH);
				phBoundBVH &bvhBound = *static_cast<phBoundBVH *>(&mutableGeomBound);
				u8 bvhStructureBuffer[sizeof(phOptimizedBvh)];
				const phOptimizedBvh *ppuAddress = bvhBound.GetBVH();
				Assert(((int)(ppuAddress) & 15) == 0);
				Assert(((int)(&bvhStructureBuffer[0]) & 15) == 0);
				cellDmaLargeGet(bvhStructureBuffer, (uint64_t)(ppuAddress), sizeof(phOptimizedBvh), DMA_TAG(15), 0, 0);
				cellDmaWaitTagStatusAll(DMA_MASK(15));
				const phOptimizedBvh& bvhStructure = reinterpret_cast<const phOptimizedBvh&>(*bvhStructureBuffer);
#else // __SPU
				const phBoundBVH &bvhBound = static_cast<const phBoundBVH&>(geomBound);
				const phOptimizedBvh& bvhStructure = *bvhBound.GetBVH();
#endif // __SPU

				// Get a list of culled primitive indices to test against
				const phPolygon::Index* culledPrimtiveIndices;
				u16 numCulledPrimitives;
				if(GetCullState() == phShapeTestCull::READ && m_CullResults->GetIncludePrimitives())
				{
					// There are already culled primitive results for this instance
					culledPrimtiveIndices = m_CullResults->GetCurrentPrimitiveIndices();
					numCulledPrimitives = m_CullResults->GetCurrentNumPrimitives();
				}
				else
				{
					// We need to perform a full cull
					// Should we allocate the giant primitive array first to reduce cache misses?
					u16 maxCulledPolygons = Min((u16)bvhBound.GetNumPolygons(),(u16)DEFAULT_MAX_CULLED_POLYS);
					phPolygon::Index* localCulledPrimitiveIndices = Alloca(phPolygon::Index,maxCulledPolygons);
					culledPrimtiveIndices = localCulledPrimitiveIndices;
					phBoundCuller bvhCuller;
					bvhCuller.SetArrays(localCulledPrimitiveIndices,maxCulledPolygons);
					m_ShapeType.PopulateCuller(boundLocalizedData, bvhStructure, bvhCuller);
					numCulledPrimitives = bvhCuller.GetNumCulledPolygons();

					if(GetCullState() == phShapeTestCull::WRITE && m_CullResults->GetIncludePrimitives())
					{
						// The user wants us to save culled primitives for this instance
						m_CullResults->AddPrimitives(culledPrimtiveIndices,numCulledPrimitives);
					}
				}

#if __SPU
				if(numCulledPrimitives == 0)
				{
					return false;
				}
				u8* ppuPolyMatIndexList = &geomBound.m_PolyMatIndexList[0];
				// Create a buffer to DMA primitives into
				phPrimitive dmaPrimitive ;

				// numMaterialIndicesPerDMA should be a multiple of 16. I chose 32 because it will
				//  prevent multiple DMAs if indices of a node straddle 16 byte alignment. The larger this buffer
				//  is the more likely we are to not need another DMA after processing a node. This chance is 
				//  small in BVHs with thousands of nodes and increasing the size of the buffer increases
				//  how much we're DMAing. As long as the DMA is done before the next primitive that doesn't really
				//  matter though. 
				const u16 numMaterialIndicesPerDma = 16*2;
				u8 materialIndexBuffer[numMaterialIndicesPerDma] ;
				phPolygon::Index nextPrimitiveIndex = culledPrimtiveIndices[0];

				// Start the DMAs for the first primitive
				StartPrimitiveDMA(nextPrimitiveIndex,&dmaPrimitive,bvhBound);
				phPolygon::Index firstPrimitiveIndexInMaterialIndexArray = StartMaterialIndexDMA(nextPrimitiveIndex,materialIndexBuffer,ppuPolyMatIndexList,numMaterialIndicesPerDma);
#endif // __SPU

#if __PFDRAW
				// See what polygons we are going to process
				static_cast<const phBoundBVH*>(&geomBound)->DrawActivePolygons(boundPartialIntersection.GetInst()->GetMatrix());
#endif // __PFDRAW

#if __SPU || COMPRESSED_VERTEX_METHOD > 0
				// We're either running on the SPU or using compressed vertices.  Either way, we need to get these vertices
				//   into a local buffer because they're not currently directly accessible to us.
				ALIGNAS(16) u8  vertexBuffer[sizeof(Vector3) * 3];
				Vector3 *localVertices = reinterpret_cast<Vector3 *>(&vertexBuffer[0]);

				const Vector3 *verticesToUse = localVertices;

#if !__SPU && COMPRESSED_VERTEX_METHOD > 0
				phPolygon localPolygon;
#endif	// COMPRESSED_VERTEX_METHOD > 0

#else	// __SPU || COMPRESSED_VERTEX_METHOD > 0
				// We're not on the SPU, and we're not using compressed vertices.  Life couldn't be simpler.
				const Vector3 *verticesToUse = boundVertices;
#endif	// __SPU || COMPRESSED_VERTEX_METHOD > 0

				phShapeBase::PrimitivePartialIntersection primitivePartialIntersection = boundPartialIntersection;
				typename ShapeType::LocalizedData& primitiveLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,m_ShapeType);
				for(u16 culledPrimitiveIndex = 0; culledPrimitiveIndex < numCulledPrimitives; ++culledPrimitiveIndex)
				{
#if __SPU
					// Wait for the material indices and primitive to finish DMAing
					WaitForBvhLoopDMAs();
					phPolygon::Index primitiveIndex = nextPrimitiveIndex;
					// Get the index into the local material index array
					FastAssert(primitiveIndex-firstPrimitiveIndexInMaterialIndexArray < numMaterialIndicesPerDma);
					phMaterialMgr::Id materialId = bvhBound.phBoundGeometry::GetMaterialId(materialIndexBuffer[primitiveIndex-firstPrimitiveIndexInMaterialIndexArray]);
					phPrimitive primitive = dmaPrimitive; // Make a full copy of the primitive so we can start a new DMA
					// TODO: We should be able to rearrange this code so that this branch doesn't exist. Just loop over the first N-1, then test the last one. 
					if(culledPrimitiveIndex < numCulledPrimitives - 1) 
					{
						// If there are more primitives after this, start grabbing the next one
						nextPrimitiveIndex = culledPrimtiveIndices[culledPrimitiveIndex+1];
						StartPrimitiveDMA(nextPrimitiveIndex,&dmaPrimitive,bvhBound);
						// If the next primitive doesn't map into our material index buffer we need to grab material indices that it does map to
						int nextPrimitiveMaterialIndexOffset = (int)nextPrimitiveIndex - (int)firstPrimitiveIndexInMaterialIndexArray;
						if(nextPrimitiveMaterialIndexOffset >= numMaterialIndicesPerDma || nextPrimitiveMaterialIndexOffset < 0)
						{
							firstPrimitiveIndexInMaterialIndexArray = StartMaterialIndexDMA(nextPrimitiveIndex,materialIndexBuffer,ppuPolyMatIndexList,numMaterialIndicesPerDma);
						}
					}
					Assertf(m_UseIncludePrimitiveCallback == false, "Primitive callbacks are not allowed on SPU currently.");
#else // __SPU
					phPolygon::Index primitiveIndex = culledPrimtiveIndices[culledPrimitiveIndex];
					phMaterialMgr::Id materialId = bvhBound.phBoundGeometry::GetMaterialId(bvhBound.phBoundGeometry::GetPolygonMaterialIndex(primitiveIndex));
					const phPrimitive& primitive = bvhBound.GetPrimitive(primitiveIndex);
					// First check if the user wants to reject this primitive
					if(m_UseIncludePrimitiveCallback && !m_IncludePrimitiveCallback(boundPartialIntersection.GetInst(),primitive,materialId))
					{
						continue;
					}
#endif // __SPU
					primitivePartialIntersection.SetPrimitive(primitiveIndex,materialId);
					
					if(primitive.GetType() == PRIM_TYPE_POLYGON)
					{
						const phPolygon* polygon = &primitive.GetPolygon();

#if __SPU || COMPRESSED_VERTEX_METHOD > 0
						HandleDMADecompressVerticesForBVHPolygon(localVertices, *polygon, geomBound);
						polygon = CloneAndRewireLocalBVHPolygon(SPU_ONLY(primitive.GetPolygon()) NON_SPU_ONLY(localPolygon), *polygon);
#endif // __SPU || COMPRESSED_VERTEX_METHOD > 0

						if(!m_ShapeType.RejectPolygonFromBvhBoundCull(boundLocalizedData, *polygon,*verticesToUse))
						{
							wasBoundHit |= m_ShapeType.TestPolygon(boundLocalizedData,*polygon,*verticesToUse,boundTypeFlags,boundIncludeFlags,geomBound,primitivePartialIntersection);
						}
					}
					else if(primitive.GetType() == PRIM_TYPE_SPHERE)
					{
						Mat34V localSphereMatrix;
						CREATE_SIMPLE_BOUND_ON_STACK(phBoundSphere,sphereBound);
						bvhBound.ConstructBoundFromPrimitive(primitive.GetSphere(),sphereBound,localSphereMatrix);
						sphereBound.phBoundSphere::SetMaterial(materialId);
						primitivePartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal(),localSphereMatrix);
						m_ShapeType.TransformLocalizedData(boundLocalizedData,primitiveLocalizedData,localSphereMatrix);
						if(!m_ShapeType.RejectBoundFromBvhBoundCull(primitiveLocalizedData, sphereBound))
						{
							wasBoundHit |= m_ShapeType.TestBoundPrimitive(primitiveLocalizedData, sphereBound, boundTypeFlags, boundIncludeFlags, primitivePartialIntersection);
						}
						primitivePartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal());
					}
					else if(primitive.GetType() == PRIM_TYPE_CAPSULE)
					{
						Mat34V localCapsuleMatrix;
						CREATE_SIMPLE_BOUND_ON_STACK(phBoundCapsule,capsuleBound);
						bvhBound.ConstructBoundFromPrimitive(primitive.GetCapsule(),capsuleBound,localCapsuleMatrix);
						capsuleBound.phBoundCapsule::SetMaterial(materialId);
						primitivePartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal(),localCapsuleMatrix);
						m_ShapeType.TransformLocalizedData(boundLocalizedData,primitiveLocalizedData,localCapsuleMatrix);
						if(!m_ShapeType.RejectBoundFromBvhBoundCull(primitiveLocalizedData, capsuleBound))
						{
							wasBoundHit |= m_ShapeType.TestBoundPrimitive(primitiveLocalizedData, capsuleBound, boundTypeFlags, boundIncludeFlags, primitivePartialIntersection);
						}
						primitivePartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal());
					}
					else if(primitive.GetType() == PRIM_TYPE_CYLINDER)
					{
						Mat34V localCylinderMatrix;
						CREATE_SIMPLE_BOUND_ON_STACK(phBoundCylinder,cylinderBound);
						bvhBound.ConstructBoundFromPrimitive(primitive.GetCylinder(),cylinderBound,localCylinderMatrix);
						cylinderBound.phBoundCylinder::SetMaterial(materialId);
						primitivePartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal(),localCylinderMatrix);
						m_ShapeType.TransformLocalizedData(boundLocalizedData,primitiveLocalizedData,localCylinderMatrix);
						if(!m_ShapeType.RejectBoundFromBvhBoundCull(primitiveLocalizedData, cylinderBound))
						{
							wasBoundHit |= m_ShapeType.TestBoundPrimitive(primitiveLocalizedData, cylinderBound, boundTypeFlags, boundIncludeFlags, primitivePartialIntersection);
						}
						primitivePartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal());
					}
					else if(primitive.GetType() == PRIM_TYPE_BOX)
					{
						Mat34V localBoxMatrix;
						CREATE_SIMPLE_BOUND_ON_STACK(phBoundBox,boxBound);
						bvhBound.ConstructBoundFromPrimitive(primitive.GetBox(),boxBound,localBoxMatrix);
						boxBound.phBoundBox::SetMaterial(materialId);
						primitivePartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal(),localBoxMatrix);
						m_ShapeType.TransformLocalizedData(boundLocalizedData,primitiveLocalizedData,localBoxMatrix);
						if(!m_ShapeType.RejectBoundFromBvhBoundCull(primitiveLocalizedData, boxBound))
						{
							wasBoundHit |= m_ShapeType.TestBoundPrimitive(primitiveLocalizedData, boxBound, boundTypeFlags, boundIncludeFlags, primitivePartialIntersection);
						}
						primitivePartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal());
					}
				}
			}

			break;
		}

#if USE_GRIDS
	case phBound::GRID:
		{
			Assertf(bound.GetType() != phBound::GRID, "Grid bounds aren't supported by shapetests.");
			break;
		}
#endif

	case phBound::COMPOSITE:
		{
			const phBoundComposite& compositeBound = *static_cast<const phBoundComposite*>(&bound);
			int numCompositeParts = compositeBound.GetNumBounds();
#if __SPU
			// DMA in the array of pointers to bounds.
			phBoundComposite& mutableCompositeBound = *const_cast<phBoundComposite*>(&compositeBound);
			datOwner<phBound> *compositeBoundArray = Alloca(datOwner<phBound>, numCompositeParts);
			cellDmaLargeGet(compositeBoundArray, (uint64_t)(compositeBound.m_Bounds), sizeof(phBound *) * numCompositeParts, DMA_TAG(15), 0, 0);

			// DMA in the matrices.
			Mat34V *compositeMatrixArray = Alloca(Mat34V, numCompositeParts);
			cellDmaLargeGet(compositeMatrixArray, (uint64_t)(compositeBound.m_CurrentMatrices), sizeof(Matrix34) * numCompositeParts, DMA_TAG(15), 0, 0);

			// DMA in the instance and type flag arrays, if they exist
			if (compositeBound.m_TypeAndIncludeFlags)
			{
				u32* typeAndIncludeFlags = Alloca(u32, numCompositeParts * 2);
				cellDmaLargeGet(typeAndIncludeFlags, (uint64_t)(compositeBound.m_TypeAndIncludeFlags), sizeof(u32) * numCompositeParts * 2, DMA_TAG(15), 0, 0);
				mutableCompositeBound.m_TypeAndIncludeFlags = typeAndIncludeFlags;
			}

			// Fix up (the SPU version of) the composite bound to point to the local arrays that we DMA'd in.
			mutableCompositeBound.m_Bounds = compositeBoundArray;
			mutableCompositeBound.m_CurrentMatrices = compositeMatrixArray;

			// Push off the DMA waits as long as we can.
			cellDmaWaitTagStatusAll(DMA_MASK(15));
#endif
			const phOptimizedBvh *bvhStructure = compositeBound.GetBVHStructure();
			int numComponentsToCheck;
			// Create our own culler that we use only for figuring out which parts of a composite bound we want to process.  We don't want to mess with (and
			//   confuse) the culler that we're already using to record culled polygon indices.
			phBoundCuller compositeCuller;

			// We always allocate and populate this array (even when no BVH structure is present) just to avoid branching in the loop below.  Maybe that's
			//   overkill but worst case it should be a wash.
			phPolygon::Index *culledComponentIndices = Alloca(phPolygon::Index, numCompositeParts);

			if(bvhStructure != NULL)
			{
#if __SPU
				// On the SPU we need to DMA in the btOptimizedBVH structure.
				u8 bvhStructureBuffer[sizeof(phOptimizedBvh)];
				Assert(((int)(bvhStructure) & 15) == 0);
				Assert(((int)(&bvhStructureBuffer[0]) & 15) == 0);
				cellDmaLargeGet(bvhStructureBuffer, (uint64_t)(bvhStructure), sizeof(phOptimizedBvh), DMA_TAG(15), 0, 0);
				bvhStructure = reinterpret_cast<const phOptimizedBvh *>(bvhStructureBuffer);
				cellDmaWaitTagStatusAll(DMA_MASK(15));
#endif

				compositeCuller.SetArrays(culledComponentIndices, numCompositeParts);
				m_ShapeType.PopulateCuller(boundLocalizedData,*bvhStructure,compositeCuller);
				numComponentsToCheck = compositeCuller.GetNumCulledPolygons();
			}
			else
			{
				numComponentsToCheck = numCompositeParts;
				for(int i = numCompositeParts - 1; i >= 0; --i)
				{
					culledComponentIndices[i] = (phPolygon::Index)i;
				}
			}

			phShapeBase::PartialIntersection componentPartialIntersection = boundPartialIntersection;
			typename ShapeType::LocalizedData& componentLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,GetShape());
			for (int culledComponentIndex=0; culledComponentIndex<numComponentsToCheck; culledComponentIndex++)
			{
				// Because we switched the pointer (to point to the buffer we DMA'd in) we can access the bound pointer in the usual way here on the SPU.
				// Note that the partBound itself still points to a PPU object, though.
				u16 partIndex = culledComponentIndices[culledComponentIndex];
				const phBound* partBound = compositeBound.GetBound(partIndex);
				if (partBound)
				{
					// If we have per-part type and include flags, test there here before we decide to continue processing this part
					u32 partTypeFlags = boundTypeFlags;
					u32 partIncludeFlags = boundIncludeFlags;
					if (compositeBound.GetTypeAndIncludeFlags())
					{
						partTypeFlags = compositeBound.GetTypeFlags(partIndex);
						partIncludeFlags = compositeBound.GetIncludeFlags(partIndex);
						if (!phLevelNew::MatchFlags(typeFlags, includeFlags, partTypeFlags, partIncludeFlags))
						{
							continue;
						}
					}

#if __SPU
					// DMA in the sub-bound object.
					u8 partBoundBuffer[phBound::MAX_BOUND_SIZE];
					cellDmaLargeGet(partBoundBuffer, (uint64_t)(compositeBound.GetBound(partIndex)), phBound::MAX_BOUND_SIZE, DMA_TAG(16), 0, 0);
					cellDmaWaitTagStatusAll(DMA_MASK(16));
					partBound = reinterpret_cast<phBound *>(partBoundBuffer);
#endif

					// Transform the test shape from the composite bound object's coordinate system to the composite bound part's coordinate system.
					// Since we've switched the 'current matrices' pointer in the composite bound we can access the matrix the usual way.
					// NOTE: We want to store a deep copy of the component matrix because if we're on a separate thread it is possible for the user to
					//         change it during testbound, so we would localize with one matrix, and unlocalize with another. 
					const Mat34V instanceFromComponent = compositeBound.GetCurrentMatrix(partIndex);

					Assertf(instanceFromComponent.IsOrthonormal3x3(ScalarVFromF32(0.2f)),"Non-orthonormal component matrix %i on '%s' found during shapetest."
						"\n numCompositeParts: %d   BoundType: %s"
						"\n\t%f %f %f"
						"\n\t%f %f %f"
						"\n\t%f %f %f",
						partIndex, boundPartialIntersection.GetInst()->GetArchetype()->GetFilename(), numCompositeParts, partBound->GetTypeString() ,MAT33V_ARG_FLOAT_RC(instanceFromComponent.GetMat33ConstRef()));

					componentPartialIntersection.SetComponent(partIndex);
					componentPartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal(),instanceFromComponent);
					m_ShapeType.TransformLocalizedData(boundLocalizedData,componentLocalizedData,instanceFromComponent);

					// Try a quick reject test on the composite bound part, and if it fails then find the number of hits.
					if (!m_ShapeType.RejectBoundFromCompositeBoundIteration(componentLocalizedData,*partBound) && TestBound(componentLocalizedData,*partBound,partTypeFlags,partIncludeFlags,includeFlags,typeFlags,componentPartialIntersection))
					{
						wasBoundHit = true;
						if(m_ShapeType.IsBooleanTest())
						{
							// Don't bother testing other bounds if this is a boolean test and we found a hit
							break;
						}
					}
				}
			}
			break;
		}
	}

	return wasBoundHit;
}

template <class ShapeType> int phShapeTest<ShapeType>::RetestIntersections ()
{
	typename ShapeType::LocalizedData& worldLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,m_ShapeType);
	m_ShapeType.InitLocalizedData(worldLocalizedData);
	typename ShapeType::LocalizedData& instanceLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,m_ShapeType);

	// Get the first intersection for retesting.
	phIntersection* intersection = m_ShapeType.GetNextRetestIntersection();
	int numHits = 0;
	while (intersection)
	{
		if (intersection->IsAHit())
		{
			// This is the instance we are testing against.
			phInst& instance = *intersection->GetInstance();
			Mat34V instanceFromWorld = instance.GetMatrix();

			m_ShapeType.TransformLocalizedData(worldLocalizedData,instanceLocalizedData,instanceFromWorld);

			phShapeBase::PartialIntersection instancePartialIntersection;
			instancePartialIntersection.SetInstance(&instance, phHandle(intersection->GetLevelIndex(),intersection->GetGenerationID()),instanceFromWorld);

			// Retest this intersection.
			phBound& bound = *instance.GetArchetype()->GetBound();
			numHits += RetestIntersection(instanceLocalizedData,*intersection,bound,instancePartialIntersection);
		}

		// Get the next intersection for retesting.
		intersection = m_ShapeType.GetNextRetestIntersection();
	}

#if __PFDRAW
	if (PFDGROUP_ShapeTests.WillDraw())
	{
		m_ShapeType.DrawIntersections(true);
	}
#endif // __PFDRAW

	// Return the number of hits on this retest.
	return numHits;
}

template <class ShapeType> int phShapeTest<ShapeType>::RetestIntersection (const typename ShapeType::LocalizedData& boundLocalizedData, phIntersection& intersection, const phBound& bound, const phShapeBase::PartialIntersection& boundPartialIntersection)
{
	// Remove the instance from the intersection, so that it will only count as a hit if this repeat test hits.
	intersection.SetInstance(phInst::INVALID_INDEX, 0);

#if __PFDRAW
	// Draw the shape (active only with profile drawing turned on).
	if (PFDGROUP_ShapeTests.WillDraw())
	{
		m_ShapeType.DrawShape(true);
	}
#endif // __PFDRAW

	u32 boundTypeFlags = TYPE_FLAGS_ALL;
	u32 boundIncludeFlags = INCLUDE_FLAGS_ALL;
#if !__SPU
	int levelIndex = phInst::INVALID_INDEX;
	const phLevelNew *level = GetLevel();
	const phInst& instance = *boundPartialIntersection.GetInst();
	if(instance.IsInLevel() && level != NULL)
	{
		levelIndex = instance.GetLevelIndex();
		boundTypeFlags = level->GetInstanceTypeFlags(levelIndex);
		boundIncludeFlags = level->GetInstanceIncludeFlags(levelIndex);
	}
#endif	// !__SPU

	// Check the bound type.
	switch (bound.GetType())
	{
	case phBound::SPHERE:
	case phBound::CAPSULE:
	case phBound::BOX:
	case phBound::DISC:
	case phBound::CYLINDER:
		{
			m_ShapeType.TestBoundPrimitive(boundLocalizedData,bound,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
			break;
		}

	case phBound::GEOMETRY:
		if(GetTreatPolyhedralBoundsAsPrimitives())
		{
			m_ShapeType.TestBoundPrimitive(boundLocalizedData,bound,boundTypeFlags,boundIncludeFlags,boundPartialIntersection);
			break;
		}
	USE_GEOMETRY_CURVED_ONLY(case phBound::GEOMETRY_CURVED:)
	case phBound::BVH:
		{
			phPolygon::Index polygonIndex = intersection.GetPartIndex();
			const phBoundGeometry& geomBound = *static_cast<const phBoundGeometry*>(&bound);

			const phPolygon* polygon = &geomBound.GetPolygon(polygonIndex);

			phShapeBase::PrimitivePartialIntersection primitivePartialIntersection = boundPartialIntersection;
			primitivePartialIntersection.SetPrimitive(polygonIndex,geomBound.phBoundGeometry::GetMaterialId(geomBound.phBoundGeometry::GetPolygonMaterialIndex(polygonIndex)));

#if COMPRESSED_VERTEX_METHOD == 0
			// Un-compressed vertices, just grab the vertex pointer from the bound.
			const Vector3 *vertices = (const Vector3*)geomBound.GetVertexPointer();
			FastAssert(vertices);
			m_ShapeType.TestPolygon(boundLocalizedData,*polygon,*vertices,boundTypeFlags,boundIncludeFlags,geomBound,primitivePartialIntersection);
#else //COMPRESSED_VERTEX_METHOD >0
			// We're dealing with compressed vertices so let's decompress the vertices into a local buffer.
			Vector3 localVertices[3];
			for(int vertexIndex = 2; vertexIndex >= 0; --vertexIndex)
			{
				const int curVertInBoundIndex = polygon->GetVertexIndex(vertexIndex);
				localVertices[vertexIndex].Set(VEC3V_TO_VECTOR3(geomBound.GetCompressedVertex(curVertInBoundIndex)));
			}

			// Create a temporary polygon that's a copy of the original and re-wire that polygon's vertex indices to be indices into our local vertex
			//   array (the ones we just decompressed).
			phPolygon localPolygon;
			memcpy(&localPolygon, polygon, sizeof(phPolygon));
			localPolygon.SetIndex(0, 0);
			localPolygon.SetIndex(1, 1);
			localPolygon.SetIndex(2, 2);
			m_ShapeType.TestPolygon(boundLocalizedData,localPolygon,*localVertices,boundTypeFlags,boundIncludeFlags,geomBound,primitivePartialIntersection);
#endif

			break;
		}

#if USE_GRIDS
	case phBound::GRID:
		{
			Assertf(bound.GetType() != phBound::GRID, "Grid bounds aren't supported by shapetests.");
			break;
		}
#endif

	case phBound::COMPOSITE:
		{
			const phBoundComposite& compositeBound = *static_cast<const phBoundComposite*>(&bound);
			u16 componentIndex = intersection.GetComponent();
			const phBound* subBound = compositeBound.GetBound(componentIndex);
			Assert(subBound);


			// Transform the test shape from the composite bound object's coordinate system to the composite bound part's coordinate system.
			// Since we've switched the 'current matrices' pointer in the composite bound we can access the matrix the usual way.
			Mat34V componentMatrix = compositeBound.GetCurrentMatrix(componentIndex);

			phShapeBase::PartialIntersection componentPartialIntersection = boundPartialIntersection;
			componentPartialIntersection.SetComponent(componentIndex);
			componentPartialIntersection.SetWorldFromLocal(boundPartialIntersection.GetWorldFromLocal(),componentMatrix);

			typename ShapeType::LocalizedData& componentLocalizedData = ALLOCATE_LOCALIZED_DATA(ShapeType,m_ShapeType);
			m_ShapeType.TransformLocalizedData(boundLocalizedData,componentLocalizedData,componentMatrix);

			// Try a quick reject test on the composite bound part, and if it fails then find the number of hits.
			if (!m_ShapeType.RejectBoundFromCompositeBoundIteration(componentLocalizedData,*subBound))
			{
				RetestIntersection(componentLocalizedData,intersection,*subBound,componentPartialIntersection);
			}
			break;
		}
	}

	if (intersection.IsAHit())
	{
		return 1;
	}

	return 0;
}

template <class ShapeType> ShapeType& phShapeTest<ShapeType>::GetShape ()
{
	return m_ShapeType;
}

template <class ShapeType> ShapeType const& phShapeTest<ShapeType>::GetShape () const
{
	return m_ShapeType;
}

template <class ShapeType> void phShapeTest<ShapeType>::GetExcludeInstanceList(phInst const * const * & outExcludeInstanceList, int& numExcludeInstances) const
{
	outExcludeInstanceList = m_ExcludeInstanceList;
	numExcludeInstances = m_NumExcludeInstances;
}

template <class ShapeType> void phShapeTest<ShapeType>::HandleDMADecompressVerticesForBVHPolygon(Vector3 *localVertices, const phPolygon &polygon, const phBoundGeometry &geomBound)
{
#if __SPU && COMPRESSED_VERTEX_METHOD != 0
	// Do the DMAs for the vertices in parallel and wait for them all at the end.
	// NOTE: This code is making the assumption that polygons have three vertices, which should always be the case now.
	u8 vertexBuffer0[32], vertexBuffer1[32], vertexBuffer2[32];
	const int vertInBoundIndex0 = polygon.GetVertexIndex(0);
	const CompressedVertexType *compressedVertexPointer0 = &geomBound.GetCompressedVertexPointer()[3 * vertInBoundIndex0];
	const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer0) & ~15);
	cellDmaLargeGet(vertexBuffer0, (uint64_t)(ppuDMAAddress0), 32, DMA_TAG(16), 0, 0);

	const int vertInBoundIndex1 = polygon.GetVertexIndex(1);
	const CompressedVertexType *compressedVertexPointer1 = &geomBound.GetCompressedVertexPointer()[3 * vertInBoundIndex1];
	const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer1) & ~15);
	cellDmaLargeGet(vertexBuffer1, (uint64_t)(ppuDMAAddress1), 32, DMA_TAG(16), 0, 0);

	const int vertInBoundIndex2 = polygon.GetVertexIndex(2);
	const CompressedVertexType *compressedVertexPointer2 = &geomBound.GetCompressedVertexPointer()[3 * vertInBoundIndex2];
	const CompressedVertexType *ppuDMAAddress2 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer2) & ~15);
	cellDmaLargeGet(vertexBuffer2, (uint64_t)(ppuDMAAddress2), 32, DMA_TAG(16), 0, 0);

	const int bufferOffset0 = (int)(compressedVertexPointer0) & 15;
	const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer0[bufferOffset0]);
	const int bufferOffset1 = (int)(compressedVertexPointer1) & 15;
	const CompressedVertexType *spuCompressedVector1 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer1[bufferOffset1]);
	const int bufferOffset2 = (int)(compressedVertexPointer2) & 15;
	const CompressedVertexType *spuCompressedVector2 = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer2[bufferOffset2]);

	cellDmaWaitTagStatusAll(DMA_MASK(16));

	localVertices[0].Set(VEC3V_TO_VECTOR3(geomBound.DecompressVertex(spuCompressedVector0)));
	localVertices[1].Set(VEC3V_TO_VECTOR3(geomBound.DecompressVertex(spuCompressedVector1)));
	localVertices[2].Set(VEC3V_TO_VECTOR3(geomBound.DecompressVertex(spuCompressedVector2)));
#else // __SPU && COMPRESSED_VERTEX_METHOD != 0
	const int kNumVerts = POLY_MAX_VERTICES;
	for(int curVertInPolyIndex = 0; curVertInPolyIndex < kNumVerts; ++curVertInPolyIndex)
	{
#if __SPU
		// SPU, non-compressed vertices.
		cellDmaLargeGet(&localVertices[curVertInPolyIndex], (uint64_t)(&geomBound.GetVertexPointer()[polygon.GetVertexIndex(curVertInPolyIndex)]), sizeof(Vector3), DMA_TAG(1 + curVertInPolyIndex), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(1 + curVertInPolyIndex));
#else	// __SPU

		const int curVertInBoundIndex = polygon.GetVertexIndex(curVertInPolyIndex);

#	if COMPRESSED_VERTEX_METHOD != 0
		// Compressed vertices, non-SPU.  Just decompress.
		localVertices[curVertInPolyIndex].Set(VEC3V_TO_VECTOR3(geomBound.GetCompressedVertex(curVertInBoundIndex)));
#	else
		// Uncompressed vertices, non-SPU.  Just copy.
		localVertices[curVertInPolyIndex].Set(VEC3V_TO_VECTOR3(geomBound.GetVertex(curVertInBoundIndex)));
#	endif

#endif	// __SPU
	}
#endif // __SPU && COMPRESSED_VERTEX_METHOD != 0
}


template <class ShapeType> phPolygon* phShapeTest<ShapeType>::CloneAndRewireLocalBVHPolygon (phPolygon& localPolygon, const phPolygon&
																													#if (COMPRESSED_VERTEX_METHOD>0 & !__SPU) | (__ASSERT & __SPU)
																																	 sourcePolygon
																													#endif
																																	 )
{
#if COMPRESSED_VERTEX_METHOD > 0 && !__SPU
	// We've got compressed vertices but we're not on the SPU, so we need to make a temporary polygon so
	//   that we can muck around with its vertex indices.
	sysMemCpy(&localPolygon, &sourcePolygon, sizeof(phPolygon));
#elif __SPU	// COMPRESSED_VERTEX_METHOD > 0 && !__SPU
	// We're on the SPU, with or without compressed vertices.
	// Re-wire the polygon vertex indices to be indices into our local vertex array (the one we just DMA'd over).  Since we get a const phPolygon
	//   pointer back from Get[First/Next]Polygon() we can't modify the object using that pointer.  On the SPU (and only with BVHs), however,
	//   we know that the object at the other end of that pointer is just a temporary object that we can modify without any problem.
	Assert(&localPolygon == &sourcePolygon);
#endif	// COMPRESSED_VERTEX_METHOD > 0 && !__SPU

	// Re-wire the polygon vertex indices to be indices into our local vertex array (the ones we just decompressed).
	localPolygon.SetIndex(0, 0);
	localPolygon.SetIndex(1, 1);
	localPolygon.SetIndex(2, 2);
	return &localPolygon;
}

#if __SPU
template <class ShapeType> void phShapeTest<ShapeType>::WaitForBvhLoopDMAs() const
{
	cellDmaWaitTagStatusAll(DMA_MASK(13));
}
template <class ShapeType> void phShapeTest<ShapeType>::StartPrimitiveDMA(phPolygon::Index primitiveIndex, phPrimitive* spuPrimitive, const phBoundBVH& bvhBound) const
{
	FastAssert(((u32)spuPrimitive & 0xF) == 0);
	cellDmaLargeGet(spuPrimitive,(uint64_t)&bvhBound.GetPrimitive(primitiveIndex), sizeof(phPrimitive), DMA_TAG(13), 0, 0);
}
template <class ShapeType> phPolygon::Index phShapeTest<ShapeType>::StartMaterialIndexDMA(phPolygon::Index firstPrimitiveIndex, u8* spuMaterialIndices, const u8* ppuMaterialIndices, int numIndicesToGrab) const
{
	FastAssert(((u32)spuMaterialIndices & 0xF) == 0);
	FastAssert(((u32)ppuMaterialIndices & 0xF) == 0);
	FastAssert((numIndicesToGrab & 0xF) == 0);

	// Find the first 16 byte aligned material index coming before firstPolygonIndex
	const u8* closestAlignedMaterialIndex = (ppuMaterialIndices + firstPrimitiveIndex) - ((firstPrimitiveIndex) & 0xF);
	FastAssert(((u32)closestAlignedMaterialIndex & 0xF) == 0);
	cellDmaLargeGet(spuMaterialIndices, (uint64_t)(closestAlignedMaterialIndex), numIndicesToGrab, DMA_TAG(13), 0, 0);

	// Give back a primitive index offset so the user can look up into spuMaterialIndices
	// spuMaterialIndices[firstPrimitiveIndex-retValue] should be the same as ppuMaterialIndices[firstPrimitiveIndex]
	return (phPolygon::Index)(closestAlignedMaterialIndex-ppuMaterialIndices);
}
#endif // __SPU

// Explicit instantiation, which allows us to put the code in the .cpp file
template class phShapeTest<phShapeProbe>;
template class phShapeTest<phShapeEdge>;
template class phShapeTest<phShapeSphere>;
template class phShapeTest<phShapeObject>;
template class phShapeTest<phShapeCapsule>;
template class phShapeTest<phShapeSweptSphere>;
template class phShapeTest<phShapeTaperedSweptSphere>;
template class phShapeTest<phShapeScalingSweptQuad>;
template class phShapeTest<phShapeBatch>;


#if !__SPU
phShapeTestTaskData phShapeTestTaskData::sm_DefaultShapeTestTaskData;
bool phShapeTestTaskData::sm_DefaultShapeTestTaskDataInitialized = false;

void phShapeTestTaskData::Init ()
{
	// Clear the task parameters.
	sysMemZeroBytes<sizeof(sysTaskParameters)>(&m_TaskParameters);

	// Zero the numbers of each shape test type.
	m_NumProbes = 0;
	m_NumEdges = 0;
	m_NumSpheres = 0;
	m_NumCapsules = 0;
	m_NumBatches = 0;
	m_NumSweptSpheres = 0;
	m_NumTaperedSweptSpheres = 0;
	m_NumScalingSweptQuads = 0;

	// Set the task input and output data to this.
	m_TaskParameters.Input.Data = (void*)(this);
	m_TaskParameters.Input.Size = sizeof(*this);
	m_TaskParameters.Output.Data = (void*)(this);
	m_TaskParameters.Output.Size = sizeof(*this);
#if __PPU
	//GY - 20*1024 wasn't enough for all the possible allocations in the 
	//spu code.  The complete set of coded allocations will need the following amount
	//of memory.  This amounts to about 25*1024.
	m_TaskParameters.Scratch.Size=
		sizeof(phLevelNew) + 
		sizeof(phPolygon::Index)*MAX_SPU_NUM_CULLED_POLYS +
		sizeof(phShapeTest<phShapeProbe>) +
		sizeof(phShapeTest<phShapeSphere>) + 
		sizeof(phIntersection)*MAX_SHAPE_TEST_ISECTS;
#if HACK_GTA4
	m_TaskParameters.SpuStackSize = 64 * 1024; // KS - attempt at a fix
#else
	m_TaskParameters.SpuStackSize = 50 * 1024;
#endif
#endif

	// Set the task parameter user data. There are always 8, and they can be NULL.
	CompileTimeAssert(ParamIndex_Count<=TASK_MAX_USER_DATA);
	m_TaskParameters.UserDataCount = ParamIndex_Count;

	// Set the first pointers to NULL. They will be used for the shape types tested.
	for(int i = 0; i < ParamIndex_ShapeTestCount; ++i)
		m_TaskParameters.UserData[i].asPtr = NULL;

	// Set the 6th to the physics level (only for PS3).
#if __PPU
	m_TaskParameters.UserData[ParamIndex_PhysicsLevel].asPtr = PHLEVEL;
#else
	m_TaskParameters.UserData[ParamIndex_PhysicsLevel].asPtr = NULL;
#endif

	// New config doesn't need this passed around like this except on SPU.
#if ENABLE_PHYSICS_LOCK
#if __PPU
	m_TaskParameters.UserData[ParamIndex_GlobalReaderCount].asPtr = g_GlobalPhysicsLock.GetGlobalReaderCountPtr();
	m_TaskParameters.UserData[ParamIndex_PhysicsMutex].asPtr = g_GlobalPhysicsLock.GetPhysicsMutexPtr();
#if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	m_TaskParameters.UserData[ParamIndex_AllowNewReaderMutex].asPtr = g_GlobalPhysicsLock.GetAllowNewReaderMutex();
#else	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	m_TaskParameters.UserData[ParamIndex_AllowNewReaderMutex].asPtr = NULL;
#endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	m_TaskParameters.UserData[ParamIndex_ModifyReaderCountMutex].asPtr = g_GlobalPhysicsLock.GetModifyReaderCountMutexPtr();
#else
	// Don't need to pass these via parameters on 360/PC.
	CompileTimeAssert(__WIN32 || RSG_ORBIS);
	m_TaskParameters.UserData[ParamIndex_GlobalReaderCount].asPtr = NULL;
	m_TaskParameters.UserData[ParamIndex_PhysicsMutex].asPtr = NULL;
	m_TaskParameters.UserData[ParamIndex_AllowNewReaderMutex].asPtr = NULL;
	m_TaskParameters.UserData[ParamIndex_ModifyReaderCountMutex].asPtr = NULL;
#endif

#else
	// Physics level locking is not used, so set the last 4 to NULL.
	m_TaskParameters.UserData[ParamIndex_GlobalReaderCount].asPtr = NULL;
	m_TaskParameters.UserData[ParamIndex_PhysicsMutex].asPtr = NULL;
	m_TaskParameters.UserData[ParamIndex_AllowNewReaderMutex].asPtr = NULL;
    m_TaskParameters.UserData[ParamIndex_ModifyReaderCountMutex].asPtr = NULL;
#endif

}

void phShapeTestTaskData::InitFromDefault()
{
	FastAssert(sm_DefaultShapeTestTaskDataInitialized);
	sysMemCpy(this,&sm_DefaultShapeTestTaskData,sizeof(phShapeTestTaskData));
	m_TaskParameters.Input.Data = m_TaskParameters.Output.Data = (void*)(this);
}

void phShapeTestTaskData::InitDefault()
{
	sm_DefaultShapeTestTaskData.Init();
	sm_DefaultShapeTestTaskDataInitialized = true;
}

#endif


void phShapeTestTaskData::SetProbes (phShapeTest<phShapeProbe>* probeTestList, int numProbes)
{
	Assertf(m_TaskParameters.UserDataCount>ParamIndex_ProbeShapeTest,"Call phShapeTestTaskData::Init before SetProbes");
	Assert(numProbes < MAX_SHAPETESTS_IN_TASK);
	m_NumProbes = (u8)numProbes;
	m_TaskParameters.UserData[ParamIndex_ProbeShapeTest].asPtr = probeTestList;
#if __DEBUGLOG
	for (int i = 0; i < numProbes; i++)
		probeTestList[i].GetShape().DebugReplay();
#endif
}

void phShapeTestTaskData::SetEdges (phShapeTest<phShapeEdge>* edgeTestList, int numEdges)
{
	Assertf(m_TaskParameters.UserDataCount>ParamIndex_EdgeShapeTest,"Call phShapeTestTaskData::Init before SetEdges");
	Assert(numEdges < MAX_SHAPETESTS_IN_TASK);
	m_NumEdges = (u8)numEdges;
	m_TaskParameters.UserData[ParamIndex_EdgeShapeTest].asPtr = edgeTestList;
#if __DEBUGLOG
	for (int i = 0; i < numEdges; i++)
		edgeTestList[i].GetShape().DebugReplay();
#endif
}

void phShapeTestTaskData::SetSpheres (phShapeTest<phShapeSphere>* sphereTestList, int numSpheres)
{
	Assertf(m_TaskParameters.UserDataCount>ParamIndex_SphereShapeTest,"Call phShapeTestTaskData::Init before SetSpheres");
	Assert(numSpheres < MAX_SHAPETESTS_IN_TASK);
	m_NumSpheres = (u8)numSpheres;
	m_TaskParameters.UserData[ParamIndex_SphereShapeTest].asPtr = sphereTestList;
#if __DEBUGLOG
	for (int i = 0; i < numSpheres; i++)
		sphereTestList[i].GetShape().DebugReplay();
#endif
}


void phShapeTestTaskData::SetCapsules (phShapeTest<phShapeCapsule>* capsuleTestList, int numCapsules)
{
	Assertf(m_TaskParameters.UserDataCount>ParamIndex_CapsuleShapeTest,"Call phShapeTestTaskData::Init before SetCapsules");
	Assert(numCapsules < MAX_SHAPETESTS_IN_TASK);
	m_NumCapsules = (u8)numCapsules;
	m_TaskParameters.UserData[ParamIndex_CapsuleShapeTest].asPtr = capsuleTestList;
#if __DEBUGLOG
	for (int i = 0; i < numCapsules; i++)
		capsuleTestList[i].GetShape().DebugReplay();
#endif
}

void phShapeTestTaskData::SetSweptSpheres (phShapeTest<phShapeSweptSphere>* sweptSphereTestList, int numSweptSpheres)
{
	Assertf(m_TaskParameters.UserDataCount>ParamIndex_SweptSphereShapeTest,"Call phShapeTestTaskData::Init before SetSweptSpheres");
	Assert(numSweptSpheres < MAX_SHAPETESTS_IN_TASK);
	m_NumSweptSpheres = (u8)numSweptSpheres;
	m_TaskParameters.UserData[ParamIndex_SweptSphereShapeTest].asPtr = sweptSphereTestList;
#if __DEBUGLOG
	for (int i = 0; i < numSweptSpheres; i++)
		sweptSphereTestList[i].GetShape().DebugReplay();
#endif
}

void phShapeTestTaskData::SetTaperedSweptSpheres (phShapeTest<phShapeTaperedSweptSphere>* taperedSweptSphereTestList, int numTaperedSweptSpheres)
{
	Assertf(m_TaskParameters.UserDataCount>ParamIndex_TaperedSweptSphereShapeTest,"Call phShapeTestTaskData::Init before SetTaperedSweptSpheres");
	Assert(numTaperedSweptSpheres < MAX_SHAPETESTS_IN_TASK);
	m_NumTaperedSweptSpheres = (u8)numTaperedSweptSpheres;
	m_TaskParameters.UserData[ParamIndex_TaperedSweptSphereShapeTest].asPtr = taperedSweptSphereTestList;
#if __DEBUGLOG
	for (int i = 0; i < numTaperedSweptSpheres; i++)
		taperedSweptSphereTestList[i].GetShape().DebugReplay();
#endif
}

void phShapeTestTaskData::SetScalingSweptQuads (phShapeTest<phShapeScalingSweptQuad>* scalingSweptQuadTestList, int numScalingSweptQuads)
{
	Assertf(m_TaskParameters.UserDataCount>ParamIndex_ScalingSweptQuadShapeTest,"Call phShapeTestTaskData::Init before SetScalingSweptQuads");
	Assert(numScalingSweptQuads < MAX_SHAPETESTS_IN_TASK);
	m_NumScalingSweptQuads = (u8)numScalingSweptQuads;
	m_TaskParameters.UserData[ParamIndex_ScalingSweptQuadShapeTest].asPtr = scalingSweptQuadTestList;
#if __DEBUGLOG
	for (int i = 0; i < numScalingSweptQuads; i++)
		scalingSweptQuadTestList[i].GetShape().DebugReplay();
#endif
}
void phShapeTestTaskData::SetBatches (phShapeTest<phShapeBatch>* batchTestList, int numBatches)
{
	Assertf(m_TaskParameters.UserDataCount>ParamIndex_BatchShapeTest,"Call phShapeTestTaskData::Init before SetBatches");
	Assert(numBatches < MAX_SHAPETESTS_IN_TASK);
	m_NumBatches = (u8)numBatches;
	m_TaskParameters.UserData[ParamIndex_BatchShapeTest].asPtr = batchTestList;
#if __DEBUGLOG
	for (int i = 0; i < numBatches; i++)
		batchTestList[i].GetShape().DebugReplay();
#endif
}


void phShapeTestTaskData::SetProbe (phShapeTest<phShapeProbe>& probeTest)
{
	m_NumProbes = 1;
	m_TaskParameters.UserData[ParamIndex_ProbeShapeTest].asPtr = &probeTest;
	DEBUGLOG_ONLY(probeTest.GetShape().DebugReplay());
}

void phShapeTestTaskData::SetEdge (phShapeTest<phShapeEdge>& edgeTest)
{
	m_NumEdges = 1;
	m_TaskParameters.UserData[ParamIndex_EdgeShapeTest].asPtr = &edgeTest;
	DEBUGLOG_ONLY(edgeTest.GetShape().DebugReplay());
}

void phShapeTestTaskData::SetSphere (phShapeTest<phShapeSphere>& sphereTest)
{
	m_NumSpheres = 1;
	m_TaskParameters.UserData[ParamIndex_SphereShapeTest].asPtr = &sphereTest;
	DEBUGLOG_ONLY(sphereTest.GetShape().DebugReplay());
}


void phShapeTestTaskData::SetCapsule (phShapeTest<phShapeCapsule>& capsuleTest)
{
	m_NumCapsules = 1;
	m_TaskParameters.UserData[ParamIndex_CapsuleShapeTest].asPtr = &capsuleTest;
	DEBUGLOG_ONLY(capsuleTest.GetShape().DebugReplay());
}

void phShapeTestTaskData::SetSweptSphere (phShapeTest<phShapeSweptSphere>& sweptSphereTest)
{
	m_NumSweptSpheres = 1;
	m_TaskParameters.UserData[ParamIndex_SweptSphereShapeTest].asPtr = &sweptSphereTest;
	DEBUGLOG_ONLY(sweptSphereTest.GetShape().DebugReplay());
}

void phShapeTestTaskData::SetTaperedSweptSphere (phShapeTest<phShapeTaperedSweptSphere>& taperedSweptSphereTest)
{
	m_NumTaperedSweptSpheres = 1;
	m_TaskParameters.UserData[ParamIndex_TaperedSweptSphereShapeTest].asPtr = &taperedSweptSphereTest;
	DEBUGLOG_ONLY(taperedSweptSphereTest.GetShape().DebugReplay());
}

void phShapeTestTaskData::SetScalingSweptQuad (phShapeTest<phShapeScalingSweptQuad>& scalingSweptQuadTest)
{
	m_NumScalingSweptQuads = 1;
	m_TaskParameters.UserData[ParamIndex_ScalingSweptQuadShapeTest].asPtr = &scalingSweptQuadTest;
	DEBUGLOG_ONLY(scalingSweptQuadTest.GetShape().DebugReplay());
}

void phShapeTestTaskData::SetBatch (phShapeTest<phShapeBatch>& batchTest)
{
	m_NumBatches = 1;
	m_TaskParameters.UserData[ParamIndex_BatchShapeTest].asPtr = &batchTest;
	DEBUGLOG_ONLY(batchTest.GetShape().DebugReplay());
}


sysTaskHandle phShapeTestTaskData::CreateTask (int iSchedulerIndex)
{
#if __PPU
#if __ASSERT
	int numShapeTestTypes = 0;
	if(m_NumProbes > 0) ++numShapeTestTypes;
	if(m_NumSpheres > 0) ++numShapeTestTypes;
	if(m_NumCapsules > 0) ++numShapeTestTypes;
	if(m_NumSweptSpheres > 0) ++numShapeTestTypes;
	if(m_NumTaperedSweptSpheres > 0) ++numShapeTestTypes;
	if(!Verifyf(m_NumEdges == 0, "SPU edge shapetests aren't supported yet.")) ++numShapeTestTypes;
	if(!Verifyf(m_NumScalingSweptQuads == 0, "SPU scaling swept quad shapetests aren't supported yet.")) ++numShapeTestTypes;
	if(!Verifyf(m_NumBatches == 0, "SPU batch shapetests aren't supported yet.")) ++numShapeTestTypes;
	Assertf(numShapeTestTypes > 0, "Trying to create a shapetest task with 0 shapetests.");
	Assertf(numShapeTestTypes < 2, "Trying to create heterogenous shapetest task on SPU.");
#endif // __ASSERT


	if( m_NumProbes > 0 )
	{
		return sysTaskManager::Create(TASK_INTERFACE_RELOADABLE(shapetestspu),m_TaskParameters,iSchedulerIndex);
	}
	else if(m_NumSpheres > 0)
	{
		return sysTaskManager::Create(TASK_INTERFACE_RELOADABLE(shapetestspherespu),m_TaskParameters,iSchedulerIndex);
	}
	else if(m_NumCapsules > 0)
	{
		return sysTaskManager::Create(TASK_INTERFACE_RELOADABLE(shapetestcapsulespu),m_TaskParameters,iSchedulerIndex);
	}
	else if(m_NumSweptSpheres > 0)
	{
		return sysTaskManager::Create(TASK_INTERFACE_RELOADABLE(shapetestsweptspherespu),m_TaskParameters,iSchedulerIndex);
	}
	else if(m_NumTaperedSweptSpheres > 0)
	{
		return sysTaskManager::Create(TASK_INTERFACE_RELOADABLE(shapetesttaperedsweptspherespu),m_TaskParameters,iSchedulerIndex);
	}
	else
	{
		return NULL;
	}
#else
	return sysTaskManager::Create(TASK_INTERFACE(TestInLevelTask),m_TaskParameters,iSchedulerIndex);
#endif
}


#if __DEBUGLOG
void phShapeTestTaskData::DebugReplay() const
{
	for (int i = 0; i < m_NumProbes; i++)
		reinterpret_cast<phShapeTest<phShapeProbe>*>(m_TaskParameters.UserData[ParamIndex_ProbeShapeTest].asPtr)[i].GetShape().DebugReplay();
	for (int i = 0; i < m_NumEdges; i++)
		reinterpret_cast<phShapeTest<phShapeEdge>*>(m_TaskParameters.UserData[ParamIndex_EdgeShapeTest].asPtr)[i].GetShape().DebugReplay();
	for (int i = 0; i < m_NumSpheres; i++)
		reinterpret_cast<phShapeTest<phShapeSphere>*>(m_TaskParameters.UserData[ParamIndex_SphereShapeTest].asPtr)[i].GetShape().DebugReplay();
	for (int i = 0; i < m_NumCapsules; i++)
		reinterpret_cast<phShapeTest<phShapeCapsule>*>(m_TaskParameters.UserData[ParamIndex_CapsuleShapeTest].asPtr)[i].GetShape().DebugReplay();
	for (int i = 0; i < m_NumSweptSpheres; i++)
		reinterpret_cast<phShapeTest<phShapeSweptSphere>*>(m_TaskParameters.UserData[ParamIndex_SweptSphereShapeTest].asPtr)[i].GetShape().DebugReplay();
	for (int i = 0; i < m_NumTaperedSweptSpheres; i++)
		reinterpret_cast<phShapeTest<phShapeTaperedSweptSphere>*>(m_TaskParameters.UserData[ParamIndex_TaperedSweptSphereShapeTest].asPtr)[i].GetShape().DebugReplay();
	for (int i = 0; i < m_NumScalingSweptQuads; i++)
		reinterpret_cast<phShapeTest<phShapeScalingSweptQuad>*>(m_TaskParameters.UserData[ParamIndex_ScalingSweptQuadShapeTest].asPtr)[i].GetShape().DebugReplay();
	for (int i = 0; i < m_NumBatches; i++)
		reinterpret_cast<phShapeTest<phShapeBatch>*>(m_TaskParameters.UserData[ParamIndex_BatchShapeTest].asPtr)[i].GetShape().DebugReplay();
}
#endif


void phShapeTestTaskManager::Init ()
{
	for (int taskIndex=0; taskIndex<s_NumShapeTestTasks; taskIndex++)
	{
		m_TaskData[taskIndex].Init();
	}
}


void phShapeTestTaskManager::SetProbes (int taskIndex, phShapeTest<phShapeProbe>* probeTestList, int numProbes)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetProbes(probeTestList,numProbes);
}


void phShapeTestTaskManager::SetSpheres (int taskIndex, phShapeTest<phShapeSphere>* sphereTestList, int numSpheres)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetSpheres(sphereTestList,numSpheres);
}


void phShapeTestTaskManager::SetCapsules (int taskIndex, phShapeTest<phShapeCapsule>* capsuleTestList, int numCapsules)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetCapsules(capsuleTestList,numCapsules);
}

void phShapeTestTaskManager::SetSweptSpheres (int taskIndex, phShapeTest<phShapeSweptSphere>* sweptSphereTestList, int numSweptSpheres)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetSweptSpheres(sweptSphereTestList,numSweptSpheres);
}

void phShapeTestTaskManager::SetTaperedSweptSpheres (int taskIndex, phShapeTest<phShapeTaperedSweptSphere>* taperedSweptSphereTestList, int numTaperedSweptSpheres)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetTaperedSweptSpheres(taperedSweptSphereTestList,numTaperedSweptSpheres);
}

void phShapeTestTaskManager::SetScalingSweptQuads (int taskIndex, phShapeTest<phShapeScalingSweptQuad>* scalingSweptQuadTestList, int numScalingSweptQuads)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetScalingSweptQuads(scalingSweptQuadTestList,numScalingSweptQuads);
}

void phShapeTestTaskManager::SetBatches (int taskIndex, phShapeTest<phShapeBatch>* batchTestList, int numBatches)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetBatches(batchTestList,numBatches);
}


void phShapeTestTaskManager::SetProbe (int taskIndex, phShapeTest<phShapeProbe>& probeTest)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetProbe(probeTest);
}


void phShapeTestTaskManager::SetSphere (int taskIndex, phShapeTest<phShapeSphere>& sphereTest)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetSphere(sphereTest);
}


void phShapeTestTaskManager::SetCapsule (int taskIndex, phShapeTest<phShapeCapsule>& capsuleTest)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetCapsule(capsuleTest);
}

void phShapeTestTaskManager::SetSweptSphere (int taskIndex, phShapeTest<phShapeSweptSphere>& sweptSphereTest)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetSweptSphere(sweptSphereTest);
}

void phShapeTestTaskManager::SetTaperedSweptSphere (int taskIndex, phShapeTest<phShapeTaperedSweptSphere>& taperedSweptSphereTest)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetTaperedSweptSphere(taperedSweptSphereTest);
}

void phShapeTestTaskManager::SetScalingSweptQuad (int taskIndex, phShapeTest<phShapeScalingSweptQuad>& scalingSweptQuadTest)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetScalingSweptQuad(scalingSweptQuadTest);
}

void phShapeTestTaskManager::SetBatch (int taskIndex, phShapeTest<phShapeBatch>& batchTest)
{
	Assertf(taskIndex>=0 && taskIndex<s_NumShapeTestTasks,"%i is an invalid task index.", taskIndex);
	m_TaskData[taskIndex].SetBatch(batchTest);
}


void phShapeTestTaskManager::CreateTasks ()
{
	// Start the tasks, assign their handles to the managers task handle list, and don't wait for the tasks to finish.
	for (int taskIndex=0; taskIndex<s_NumShapeTestTasks; taskIndex++)
	{
		m_TaskHandles[taskIndex] = m_TaskData[taskIndex].CreateTask();
	}
}


void phShapeTestTaskManager::CompleteTasks ()
{
	// Start the tasks.
	CreateTasks();

	// Wait for the tasks to finish.
	WaitTasks();

#if __DEBUGLOG
	for (int taskIndex=0; taskIndex<s_NumShapeTestTasks; taskIndex++)
		m_TaskData[taskIndex].DebugReplay();
#endif
}


void phShapeTestTaskManager::WaitTasks ()
{
	// Wait for the tasks to finish.
	sysTaskManager::WaitMultiple(s_NumShapeTestTasks,m_TaskHandles);
}


} // namespace rage
