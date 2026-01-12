//
// physics/shapetest.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_SHAPE_TEST_H
#define PHYSICS_SHAPE_TEST_H

#include "intersection.h"
#include "iterator.h"

#include "atl/delegate.h"
#include "diag/debuglog.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "system/taskheader.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"

#if __SPU
#include "system/dma.h"
#include <cell/dma.h>
#endif


namespace rage {

class phBoundBVH;
class phBoundGeometry;
class phManifold;
class phOptimizedBvh;


#if !__PPU
DECLARE_TASK_INTERFACE(TestInLevelTask);
#endif

// Turning this on will store the time to complete TestInLevel or TestOnObject
//   per shapetest
#define PROFILE_INDIVIDUAL_SHAPETESTS (__BANK && 1)

// PURPOSE: the maximum total number of batched shapes. This is used only for local arrays of integers to store the number of hits in each batched shape
#define MAX_BATCHED_SHAPES		128

// PURPOSE: the maximum number of instances in the optional exclude instance list. Single instances can also be excluded in calls to TestInLevel
#define MAX_EXCLUDE_INSTANCES	64

// PURPOSE: the maximum number of intersections that can be filled in by a shape test
#define MAX_SHAPE_TEST_ISECTS	64

// PURPOSE: the maximum number of culled instance pointers that are saved in a list, in case someone wants to skip culling and use the same list
#define MAX_SAVED_CULLED_INSTANCES	128

#define MAX_SPU_NUM_CULLED_POLYS	8192

// PURPOSE: max number of any given shapetest type in phShapeTestTaskData, if we want more we need to change the type of phShapeTestTaskData::m_NumX
#define MAX_SHAPETESTS_IN_TASK (u8)-1

#if !__SPU
#define NON_SPU_ONLY(x)	x
#else
#define NON_SPU_ONLY(x)
#endif


void TestInLevelTask (sysTaskParameters& taskParams);

class phShapeBase
{
public:
	struct LocalizedData
	{
	};

	class PartialIntersection
	{
	public:
		__forceinline void SetInstance(const phInst* inst, phHandle handle, Mat34V_In worldFromInstance)
		{
			m_WorldFromLocal = worldFromInstance;
			m_Inst = inst;
			m_Handle = handle;
			m_ComponentIndex = 0;
			m_PrimitiveIndex = 0;
		}
		__forceinline void SetComponent(u16 componentIndex) { m_ComponentIndex = componentIndex; }
		__forceinline void SetWorldFromLocal(Mat34V_In worldFromLocal) { m_WorldFromLocal = worldFromLocal; }
		__forceinline void SetWorldFromLocal(Mat34V_In worldFromPreviousLocal, Mat34V_In previousLocalFromLocal) { Transform(m_WorldFromLocal,worldFromPreviousLocal,previousLocalFromLocal); }

		__forceinline Mat34V_ConstRef GetWorldFromLocal() const { return m_WorldFromLocal; }
		__forceinline u16 GetComponentIndex() const { return m_ComponentIndex; }
		__forceinline phPolygon::Index GetPrimitiveIndex() const { return m_PrimitiveIndex; }
		__forceinline phHandle GetHandle() const { return m_Handle; }
		__forceinline const phInst* GetInst() const { return m_Inst; }
	protected:
		Mat34V m_WorldFromLocal;
		const phInst* m_Inst;
		phHandle m_Handle;
		u16 m_ComponentIndex;
		phPolygon::Index m_PrimitiveIndex;
	};

	// Primitives can only have one material, so we precompute it and pass it in
	class PrimitivePartialIntersection : public PartialIntersection
	{
	public:
		__forceinline phMaterialMgr::Id GetMaterialId() const { return m_MaterialId; }
		PrimitivePartialIntersection(const PartialIntersection& geomBoundPartialIntersection) : PartialIntersection(geomBoundPartialIntersection) {}
		void SetPrimitive(phPolygon::Index primitiveIndex, phMaterialMgr::Id materialId)
		{
			m_PrimitiveIndex = primitiveIndex;
			m_MaterialId = materialId;
		}
	protected:
		phMaterialMgr::Id m_MaterialId;
	};


	// PURPOSE: constructor
	phShapeBase() :
		m_Intersection(NULL),
		m_MaxNumIsects(0)
	{
	}

	// PURPOSE: Initialize the base shape test information.
	// PARAMS:
	//	intersections - pointer into which to record intersection information
	//	numIntersections - the maximum number of intersections to find
	// NOTES:	This is normally only called by derived classes.
	void Init (phIntersection* intersection, int numIntersections);

	void SetIntersections (phIntersection* intersection, int numIntersections, int numHits = 0);

	// PURPOSE: Re-initialize the shape test parameters before doing a new test in the physics level.
	void Reset ();

	// PURPOSE: Get the next intersection in the list of intersections.
	// PARAMS:
	//	depth - the depth of the current intersection
	//	isectPosition - the position of the current intersection
	//	depthLimit - optional upper limit for the largest depth to count as the best
	// RETURN:	the next intersection to fill in
	// NOTES:
	//	1.	Frequently there is only one intersection to fill in, and this always returns the same intersection if the new depth is greater.
	//	2.	When there are multiple intersections and not all are used, this returns the next unused intersection in the list.
	//	3.	The best depth is the largest depth that is less than the depth limit, or if there aren't any less than the depth limit, the smallest is the best.
	phIntersection* GetNextIntersection (ScalarV_In depth, ScalarV_In depthLimit=ScalarV(V_FLT_MAX));

	bool IsIntersectionValid(ScalarV_In depth, phMaterialMgr::Id materialId) const
	{
		return IsGreaterThanAll(depth,ScalarV(V_ZERO)) && ((materialId & m_IgnoreMaterialFlags) == 0);
	}
	bool IsIntersectionValid(ScalarV_In depth, const PrimitivePartialIntersection& partialIntersection) const { return IsIntersectionValid(depth, partialIntersection.GetMaterialId()); }

	void SetIntersection(phIntersection& intersection, Vec3V_In localPosition, Vec3V_In localNormal, ScalarV_In depth, ScalarV_In tValue, phMaterialMgr::Id materialId, const PartialIntersection& partialIntersection) const;
	void SetPolygonIntersection(phIntersection& intersection, Vec3V_In localPosition, Vec3V_In localNormal, Vec3V_In localPolygonNormal, ScalarV_In depth, ScalarV_In tValue, const PrimitivePartialIntersection& partialIntersection) const;
	
	bool AddIntersection(Vec3V_In localPosition, Vec3V_In localNormal, ScalarV_In depth, ScalarV_In tValue, phMaterialMgr::Id materialId, const PartialIntersection& partialIntersection, ScalarV_In depthLimit = ScalarV(V_FLT_MAX));
	bool AddPolygonIntersection(Vec3V_In localPosition, Vec3V_In localNormal, Vec3V_In localPolygonNormal, ScalarV_In depth, ScalarV_In tValue, const PrimitivePartialIntersection& partialIntersection, ScalarV_In depthLimit = ScalarV(V_FLT_MAX));

	// PURPOSE: Add an intersection to the end of the array
	// PARAMS: 
	//   intersection - the intersection to copy into the array
	// NOTES: This exists only because shapetests are used to store intersections, so it's somewhat reasonable for a user to treat it like an array. 
	void AppendIntersection(const phIntersection& intersection);

	// PURPOSE:	Get the number of intersections filled in by the shape test.
	// RETURN:	the number of intersections filled in by the shape test
	int GetNumHits () const;

	// PURPOSE:	Get the maximum number of intersections that could be filled by this shapetest
	// RETURN:	the maximum number of intersections that could be filled by this shapetest
	int GetMaxNumIntersections () const;

	bool IsBooleanTest() const { return m_MaxNumIsects == 0; }

	// Rejection tests
	// These tests are all called at specific times. Some tests benefit from certain ones depending on how accurate their BVH
	//  culls are and how fast the test about to be performed is. They default to just returning false and rejecting nothing. 
	// Ideally the user will implement RejectBound and RejectPolygon for each test and then experiment with overloading the
	//  public rejection functions with those. 

	// This is called on the bounds of all instances culled from the level before testing it
	bool RejectBoundFromLevelCull(const LocalizedData& localizedData, const phBound& bound);

	// This is called on each sub-bound that is culled from a composite's BVH before testing it
	// NOTE: This isn't actually being used yet because we haven't split the component iteration loop in TestBound. 
	bool RejectBoundFromCompositeBoundBvhCull(const LocalizedData& localizedData, const phBound& component);

	// This is called on each sub-bound that a BVH-less composite has before testing it
	bool RejectBoundFromCompositeBoundIteration(const LocalizedData& localizedData, const phBound& component);

	// This is called on each primitive bound culled from a BVH bound before testing it
	bool RejectBoundFromBvhBoundCull(const LocalizedData& localizedData, const phBound& primitive);

	// This is called on each polygon that is culled from a BVH bound before testing it
	bool RejectPolygonFromBvhBoundCull(const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& vertices);

	// This is called on each polygon on a geometry bound before testing it
	bool RejectPolygonFromGeometryBoundIteration(const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& vertices);

public:
	// PURPOSE: Set the list of material flags to be ignored.
	// PARAMS:
	//	ignoreFlags - list of flags for materials to ignore
	// NOTES:	This shape test will not intersect with any materials that contain any flags in this list.
	void SetIgnoreMaterialFlags (phMaterialFlags ignoreFlags);

	phIntersection* GetNextRetestIntersection ();

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about all intersections.
	void DebugReplay () const;
#endif

	bool TreatPolyhedralBoundsAsPrimitives() const { return false; }

	void GetIntersections (phIntersection*& outIntersections, int& outMaxNumIntersections) const;

	void PrepareForShapeTest() {}

#if __DEV
	bool DoIntersectionsConflict(const phShapeBase& otherShape) const
	{
		if(this != &otherShape)
		{
			return	(otherShape.m_Intersection >= m_Intersection && otherShape.m_Intersection < m_Intersection + m_MaxNumIsects) ||
				(m_Intersection >= otherShape.m_Intersection && m_Intersection < otherShape.m_Intersection + otherShape.m_MaxNumIsects);
		}
		else
		{
			return false;
		}
	}
#endif // __DEV

protected:
	// These rejection tests won't be called by phShapeTest. phShapeBatch will use them however. 
	bool RejectBound(const phBound& bound);
	bool RejectPolygon(const phPolygon& polygon, const Vector3& vertices);


#if __SPU
// As is often the case on the SPU, we need access to these pointers to be able to handle the DMAs.  Perhaps it would be better to
//   encapsulate that functionality in a method.
public:
#endif
	// PURPOSE: pointer to an intersection or array of intersections
	// NOTES:
	//	1.	If m_MaxNumIsects>0 then this is a list of intersections (could still be one), which will be filled in with all the best intersections.
	phIntersection* m_Intersection;

	// PURPOSE: the number of intersections in the array m_Intersection
	int m_MaxNumIsects;

	phIntersection* m_RetestIntersection;

#if __SPU
protected:
#endif

	// PURPOSE: the number of filled in intersections
	int m_NumHits;

	// PURPOSE: the largest depth found so far
	float m_LargestDepth;

	// PURPOSE: the smallest depth found so far
	// NOTES:	When m_MaxNumIsects is 1, this is not used.
	float m_SmallestDepth;

	// PURPOSE: set of flags to ignore certain materials
	// NOTES:	Any material with a match to any of the flags here will not be intersected.
	phMaterialFlags m_IgnoreMaterialFlags;
};

__forceinline bool phShapeBase::RejectBoundFromLevelCull(const LocalizedData& UNUSED_PARAM(localizedData), const phBound& UNUSED_PARAM(bound))															{ return false; }
__forceinline bool phShapeBase::RejectBoundFromCompositeBoundBvhCull(const LocalizedData& UNUSED_PARAM(localizedData), const phBound& UNUSED_PARAM(component))											{ return false; }
__forceinline bool phShapeBase::RejectBoundFromCompositeBoundIteration(const LocalizedData& UNUSED_PARAM(localizedData), const phBound& UNUSED_PARAM(component))											{ return false; }
__forceinline bool phShapeBase::RejectBoundFromBvhBoundCull(const LocalizedData& UNUSED_PARAM(localizedData), const phBound& UNUSED_PARAM(primitive))														{ return false; }
__forceinline bool phShapeBase::RejectPolygonFromBvhBoundCull(const LocalizedData& UNUSED_PARAM(localizedData), const phPolygon& UNUSED_PARAM(polygon), const Vector3& UNUSED_PARAM(vertices))			{ return false; }
__forceinline bool phShapeBase::RejectPolygonFromGeometryBoundIteration(const LocalizedData& UNUSED_PARAM(localizedData), const phPolygon& UNUSED_PARAM(polygon), const Vector3& UNUSED_PARAM(vertices))	{ return false; }

inline void phShapeBase::SetIgnoreMaterialFlags (phMaterialFlags ignoreFlags)
{
	m_IgnoreMaterialFlags = ignoreFlags;
}


inline void phShapeBase::AppendIntersection(const phIntersection& intersection)
{
	FastAssert(m_NumHits < m_MaxNumIsects);
	m_Intersection[m_NumHits++] = intersection;
}


inline int phShapeBase::GetNumHits () const
{
	return m_NumHits;
}

inline int phShapeBase::GetMaxNumIntersections () const
{
	return m_MaxNumIsects;
}

class phShapeComposite : public phShapeBase
{
public:
	phShapeComposite() : m_UserProvidedCullShape_WS(NULL) {}

	// PURPOSE: Provide a specific cull shape used by this shape test (for both culling instances and culling bounds).
	// NOTES:	This is really only to be used with batch and object shape tests where the shape test can't readily determine the optimal cull shape.
	void SetUserProvidedCullShape(const phCullShape &cullShape_WS);
	const phCullShape* GetUserProvidedCullShape() const { return m_UserProvidedCullShape_WS; }

	struct LocalizedData : public phShapeBase::LocalizedData
	{
		phCullShape m_UserProvidedCullShape;
	};

protected:

	// Two different functions for culling a BVH structures - in the case where we have a phBoundBVH we use a slightly different culling mechanism that might do some
	//   extra triangle culling.
	void PopulateCullerFromUserProvidedCullShape(const LocalizedData& localizedData, phBoundCuller& culler, const phOptimizedBvh &bvhStructure);

	// PURPOSE: the used-provided shape we cull with
	const phCullShape *m_UserProvidedCullShape_WS;
};

__forceinline void phShapeComposite::SetUserProvidedCullShape(const phCullShape &cullShape_WS)
{
	m_UserProvidedCullShape_WS = &cullShape_WS;
}


class phShapeProbe : public phShapeBase
{
public:
	struct LocalizedData;

	// PURPOSE: Initialize this shape test as a directed probe.
	// PARAMS:
	//	worldProbe - the probe's segment (start and end positions) in world coordinates
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find (default is 1, which means find only the earliest intersection)
	// NOTES:
	//		If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in with all the best intersections
	void InitProbe (const phSegmentV& worldProbe, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize the physics level iterator for culling from this shape test.
	// PARAMS:
	//	levelIterator - the physics level iterator used by this shape test
	void SetupIteratorCull (phIterator& levelIterator);

	// PURPOSE: Do a fast test to see if the given bound can intersect this shape test.
	// PARAMS:
	//	bound - a reference to the bound to test
	// RETURN:	true if the given bound can not intersect this shape test, false if it might
	bool RejectBound (const LocalizedData& localizedData, const phBound& bound) const;
	__forceinline bool RejectBoundFromLevelCull(const LocalizedData& localizedData, const phBound& bound) const { return RejectBound(localizedData, bound); }
	__forceinline bool RejectBoundFromCompositeBoundIteration(const LocalizedData& localizedData, const phBound& component) const { return RejectBound(localizedData, component); }

	// PURPOSE: See if this shape tests intersects the given primitive bound (sphere, capsule or box).
	// PARAMS:
	//	bound - the primitive bound to test
	//	instance - the object's instance using this bound
	//	levelIndex - the level index of the instance
	//	generationId - the generation of the level index (see notes)
	// RETURN:	true if this shape test intersects the given bound, false if it does not
	// NOTES:
	//	1. The level index is the same as instance->GetLevelIndex(), but is separate because this is called internally where it is already available.
	//	2. The generation identification is PHLEVEL->GetGenerationID(levelIndex). It is used to handle the physics level removing an object and then
	//		re-using the level index on a different object before the results of this test are used.
	bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

	// PURPOSE: Find the intersection normal for this probe test on the given bound at the given position.
	// PARAMS:
	//	bound - the bound being tested
	//	hitPosition - the position of the intersection of this probe test on the bound
	//	centroidOffset - the position of the bound's centroid in the object's coordinate system
	//	elementIndex - the element on the bound, such as the capsule part or the box face
	// RETURN:	the probe intersection normal on the bound
	Vec3V_Out ComputeProbeIsectNormal (const phBound& bound, Vec3V_In hitPosition, Vec3V_In centroidOffset, int elementIndex);

	// PURPOSE: Test the given polygon with this shape test.
	// PARAMS:
	//	polygon - reference to the polygon to test
	//	boundVertices - the list of vertex positions in the bound
	//	instance - the object's instance using this bound
	//	geomBound - the object's geometry bound
	// RETURN:	the number of intersections of this shape test on the given polygon
	bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

	// <COMBINE phShapeBase::GetNextPolygon>
	void PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler);

	// PURPOSE: Get the probe's segment in world coordinates.
	// RETURN:	a reference to the probe's segment in world coordinates
	const phSegmentV& GetWorldSegment () const;

#if __PFDRAW
	// PURPOSE: Draw the test shape.
	void DrawShape (bool retest = false);

	// PURPOSE: Draw the shape test's intersections.
	void DrawIntersections (bool retest = false);
#endif

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about all intersections.
	void DebugReplay () const;
#endif

	struct LocalizedData : public phShapeBase::LocalizedData
	{
		phSegmentV m_Segment;
	};
	void InitLocalizedData(LocalizedData& localizedData) const
	{
		localizedData.m_Segment = GetWorldSegment();
	}
	void TransformLocalizedData(const LocalizedData& oldLocalizedData, LocalizedData& newLocalizedData, Mat34V_In oldFromNew) const
	{
		newLocalizedData.m_Segment = phSegmentV(UnTransformOrtho(oldFromNew,oldLocalizedData.m_Segment.GetStart()),
												UnTransformOrtho(oldFromNew,oldLocalizedData.m_Segment.GetEnd()) );
	}
	u32 GetLocalizedDataSize() const { return sizeof(LocalizedData); }

protected:
	// PURPOSE: the test segment in world coordinates
	phSegmentV m_WorldSegment;
};

inline const phSegmentV& phShapeProbe::GetWorldSegment () const
{
	return m_WorldSegment;
}

class phShapeEdge : public phShapeProbe
{
public:
	// PURPOSE: Initialize this shape test as an undirected probe.
	// PARAMS:
	//	worldProbe - the edge's segment (start and end positions) in world coordinates
	//	intersection - optional pointer into which to record entry intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//		If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in with all the best intersections
	void InitEdge (const phSegmentV& worldProbe, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: See if this shape tests intersects the given primitive bound (sphere, capsule or box).
	// PARAMS:
	//	bound - the primitive bound to test
	//	instance - the object's instance using this bound
	//	levelIndex - the level index of the instance
	//	generationId - the generation of the level index (see notes)
	// RETURN:	true if this shape test intersects the given bound, false if it does not
	// NOTES:
	//	1. The level index is the same as instance->GetLevelIndex(), but is separate because this is called internally where it is already available.
	//	2. The generation identification is PHLEVEL->GetGenerationID(levelIndex). It is used to handle the physics level removing an object and then
	//		re-using the level index on a different object before the results of this test are used.
	bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

	// PURPOSE: Test the given polygon with this shape test.
	// PARAMS:
	//	polygon - reference to the polygon to test
	//	boundVertices - the list of vertex positions in the bound
	//	instance - the object's instance using this bound
	//	geomBound - the object's geometry bound
	// RETURN:	the number of intersections of this shape test on the given polygon
	bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

#if __PFDRAW
	// PURPOSE: Draw the test shape.
	void DrawShape (bool retest = false);

	// PURPOSE: Draw the shape test's intersections.
	void DrawIntersections (bool retest = false);
#endif

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about all intersections.
	void DebugReplay () const;
#endif

private:
};

class phShapeSphere : public phShapeBase
{
public:
	struct LocalizedData;

	// PURPOSE: Initialize this shape test as a point.
	// PARAMS:
	//	worldCenter - the point's position in world coordinates
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
	//	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitPoint (Vec3V_In worldCenter, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize this shape test as a sphere.
	// PARAMS:
	//	worldCenter - the sphere's position in world coordinates
	//	radius - the sphere's radius
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
	//	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitSphere (Vec3V_In worldCenter, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

	// <COMBINE phShapeProbe::SetupIteratorCull>
	void SetupIteratorCull (phIterator& levelIterator);

	// <COMBINE phShapeBase::RejectBound>
	bool RejectBound (const LocalizedData& localizedData, const phBound& bound) const;
	__forceinline bool RejectBoundFromCompositeBoundIteration(const LocalizedData& localizedData, const phBound& component) const { return RejectBound(localizedData,component); }

	static bool IntersectSphereAgainstBound(Vec3V_In sphereCenter, ScalarV_In sphereRadius, const phBound& bound, Vec3V_InOut positionOnBound, Vec3V_InOut normalOnBound, ScalarV_InOut depth, phPolygon::Index& primitiveIndex);

	// <COMBINE phShapeProbe::TestBoundPrimitive>
	bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::TestPolygon>
	bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);
	
	// <COMBINE phShapeProbe::PopulateCuller>
	void PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler);

	// PURPOSE: Get the test radius.
	// RETURN:	the test radius
	float GetRadius () const;

	// PURPOSE: Get the sphere center in world coordinates.
	// RETURN:	a reference to the test sphere center in world coordinates
	const Vector3& GetWorldCenter () const;

#if __PFDRAW
	// <COMBINE phShapeProbe::DrawShape>
	void DrawShape (bool retest = false);

	// <COMBINE phShapeProbe::DrawIntersections>
	void DrawIntersections (bool retest = false);
#endif

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about all intersections.
	void DebugReplay () const;
#endif

	struct LocalizedData : public phShapeBase::LocalizedData
	{
		Vector3 m_Center;
	};
	void InitLocalizedData(LocalizedData& localizedData) const
	{
		localizedData.m_Center = GetWorldCenter();
	}
	void TransformLocalizedData(const LocalizedData& oldLocalizedData, LocalizedData& newLocalizedData, Mat34V_In oldFromNew) const
	{
		RC_VEC3V(newLocalizedData.m_Center) = UnTransformOrtho(oldFromNew,RCC_VEC3V(oldLocalizedData.m_Center));
	}
	u32 GetLocalizedDataSize() const { return sizeof(LocalizedData); }

	void SetTestBackFacingPolygons(bool testBackFacingPolygons) { m_TestBackFacingPolygons = testBackFacingPolygons; }
	bool GetTestBackFacingPolygons() const { return m_TestBackFacingPolygons; }
	void PrepareForShapeTest() { m_BackFaceTolerance = (GetTestBackFacingPolygons() ? -m_Radius : 0.0f); }

protected:
	// PURPOSE: the center of the test sphere in world coordinates
	Vector3 m_WorldCenter;

	// PURPOSE: the radius of the test sphere
	float m_Radius;

	// PURPOSE: How far behind a polygon the sphere can be before rejecting an intersection. 
	// NOTE: This should only ever be 0 (no back faces) or -radius (allow back faces). 
	float m_BackFaceTolerance;

	bool m_TestBackFacingPolygons;
};

inline float phShapeSphere::GetRadius () const
{
	return m_Radius;
}

inline const Vector3& phShapeSphere::GetWorldCenter () const
{
	return m_WorldCenter;
}

class phShapeObject : public phShapeComposite
{
public:
	struct LocalizedData;

	// PURPOSE: Initialize this shape test as an undirected capsule.
	// PARAMS:
	//	worldProbe - the capsule's segment (start and end hemisphere center positions) in world coordinates
	//	radius - the capsule's radius
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
	//	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitObject (const phBound& bound, Mat34V_In transform, Mat34V_In lastTransform, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Compute the cull box shape for this object.
	// PARAMS:
	//	centroidTransform - the position and orientation of the cull box
	//	halfWidth - half the cull box extents
	void ComputeCullBox (const Matrix34& shapePose, Matrix34& centroidTransform, Vector3& halfWidth);

	void ComputeCullCapsule (const Matrix34& shapePose, Vector3& capsuleStart, Vector3& capsuleAxis, float& capsuleRadius);

	// <COMBINE phShapeProbe::SetupIteratorCull>
	void SetupIteratorCull (phIterator& levelIterator);

	// <COMBINE phShapeProbe::RejectBound>
	bool RejectBound (const LocalizedData& localizedData, const phBound& bound) const;
	__forceinline bool RejectBoundFromCompositeBoundIteration(const LocalizedData& localizedData, const phBound& component) const { return RejectBound(localizedData,component); }

	// <COMBINE phShapeProbe::TestBoundPrimitive>
	bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::TestPolygon>
	bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::PopulateCuller>
	void PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler);

	// <COMBINE phShapeSphere::GetRadius>
	float GetRadius () const;

	// <COMBINE phShapeProbe::GetWorldSegment>
	const phSegment& GetWorldSegment () const;

#if __PFDRAW
	// <COMBINE phShapeProbe::DrawShape>
	void DrawShape (bool retest = false);

	// <COMBINE phShapeProbe::DrawIntersections>
	void DrawIntersections (bool retest = false);
#endif

	// PURPOSE: Set the shape to use for culling from the physics level and from cullable bounds.
	// PARAMS:
	//	cullType - the shape to use for culling from the physics level and from cullable bounds
	// NOTES:	the cull types are defined near the beginning of physics/iterator.h
	void SetCullType (phCullShape::phCullType cullType);

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about all intersections.
	void DebugReplay () const;
#endif

	bool TreatPolyhedralBoundsAsPrimitives() const { return true; }

	// PURPOSE: Not to be used ordinarily
	void SetBound(const phBound* bound) { m_Bound = bound; }
	const phBound* GetBound() const { return m_Bound; }
	const Matrix34& GetWorldTransform() const { return m_TransformWorld; }
	void SetWorldTransform(Matrix34 const& matrix) { m_TransformWorld = matrix; }

	struct LocalizedData : public phShapeComposite::LocalizedData
	{
		Matrix34 m_Transform;
	};
	void InitLocalizedData(LocalizedData& localizedData) const
	{
		if(GetUserProvidedCullShape())
		{
			localizedData.m_UserProvidedCullShape = *GetUserProvidedCullShape();
		}
		localizedData.m_Transform = GetWorldTransform();
	}
	void TransformLocalizedData(const LocalizedData& oldLocalizedData, LocalizedData& newLocalizedData, Mat34V_In oldFromNew) const
	{
		if(GetUserProvidedCullShape())
		{
			oldLocalizedData.m_UserProvidedCullShape.Localize(newLocalizedData.m_UserProvidedCullShape,oldFromNew);
		}
		UnTransformOrtho(RC_MAT34V(newLocalizedData.m_Transform),oldFromNew,RCC_MAT34V(oldLocalizedData.m_Transform));
	}
	u32 GetLocalizedDataSize() const { return sizeof(LocalizedData); }

protected:
	// PURPOSE: Change the collision manifolds and contacts obtained from collision detection into intersections for the shape test.
	// PARAMS:
	//	manifold - the collision manifold
	//	instance - the colliding instance
	//	bound - the colliding bound
	// RETURN:	true if any intersections were filled in, false if not
	bool ConvertContactPointsToIntersections (phManifold& manifold, const phBound* bound, const PartialIntersection& partialIntersection);
protected:

    // PURPOSE: the transform of the bound in world space
    Matrix34 m_TransformWorld;

	Matrix34 m_CurrentToLastTransform;

	// PURPOSE: the test bound
	const phBound* m_Bound;

	// PURPOSE: the shape used for culling in the physics level (sphere, capsule or box)
	phCullShape::phCullType m_ObjectCullType;
};


inline void phShapeObject::SetCullType (phCullShape::phCullType cullType)
{
	m_ObjectCullType = cullType;
}


class phShapeCapsule : public phShapeBase
{
public:
	struct LocalizedData;
	
    // PURPOSE: Initialize the capsule
    // PARAMS:
    //	worldProbe - the capsule's segment (start and end hemisphere center positions) in world coordinates
    //	radius - the capsule's radius
    //	intersection - optional pointer into which to record intersection information
    //	numIntersections - optional maximum number of intersections to find
    // NOTES:
    //	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
    //	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
    //		found, until there are no more objects to hit or the list is full
    void InitCapsule (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

    // <COMBINE phShapeProbe::SetupIteratorCull>
    void SetupIteratorCull (phIterator& levelIterator);

    // <COMBINE phShapeProbe::RejectBound>
	bool RejectBound (const LocalizedData& localizedData, const phBound& bound) const;
	__forceinline bool RejectBoundFromCompositeBoundIteration(const LocalizedData& localizedData, const phBound& component) const { return RejectBound(localizedData,component); }

    // <COMBINE phShapeProbe::TestBoundPrimitive>
    bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

    // <COMBINE phShapeProbe::TestPolygon>
    bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::PopulateCuller>
	void PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler);

    // <COMBINE phShapeSphere::GetRadius>
    ScalarV_Out GetRadius () const;

    // <COMBINE phShapeProbe::GetWorldSegment>
    const phSegmentV& GetWorldSegment () const;

#if __PFDRAW
    // <COMBINE phShapeProbe::DrawShape>
    void DrawShape (bool retest = false);

    // <COMBINE phShapeProbe::DrawIntersections>
    void DrawIntersections (bool retest = false);
#endif

#if __DEBUGLOG
    // PURPOSE: Record replay debugging data about all intersections.
    void DebugReplay () const;
#endif

	struct LocalizedData : public phShapeBase::LocalizedData
	{
		void Initialize(const phSegmentV& segment, ScalarV_In radius)
		{
			m_Segment = segment;
			m_Min = Subtract(Min(segment.GetStart(), segment.GetEnd()), Vec3V(radius));
			m_Max =      Add(Max(segment.GetStart(), segment.GetEnd()), Vec3V(radius));
		}

		void InitFromShape(const phShapeCapsule& capsule)
		{
			Initialize(capsule.GetWorldSegment(),capsule.GetRadius());
		}

		void InitFromOther(const phShapeCapsule& capsule, const LocalizedData& previous, Mat34V_In previousFromThis)
		{
			Initialize(	phSegmentV(	UnTransformOrtho(previousFromThis,previous.m_Segment.GetStart()),
									UnTransformOrtho(previousFromThis,previous.m_Segment.GetEnd())),
						capsule.GetRadius());
		}

		phSegmentV m_Segment;
		Vec3V m_Min;
		Vec3V m_Max;
	};
	void InitLocalizedData(LocalizedData& localizedData) const
	{
		localizedData.Initialize(GetWorldSegment(),GetRadius());
	}
	void TransformLocalizedData(const LocalizedData& oldLocalizedData, LocalizedData& newLocalizedData, Mat34V_In oldFromNew) const
	{
		newLocalizedData.Initialize(phSegmentV(	UnTransformOrtho(oldFromNew,oldLocalizedData.m_Segment.GetStart()),
												UnTransformOrtho(oldFromNew,oldLocalizedData.m_Segment.GetEnd())),
									GetRadius());
	}
	u32 GetLocalizedDataSize() const { return sizeof(LocalizedData); }

protected:
    // PURPOSE: the test segment in world coordinates
    phSegmentV m_WorldSegment;

    // PURPOSE: the radius of the test capsule
    ScalarV m_Radius;
};

inline const phSegmentV& phShapeCapsule::GetWorldSegment () const
{
	return m_WorldSegment;
}

inline ScalarV_Out phShapeCapsule::GetRadius () const
{
	return m_Radius;
}

class phShapeSweptSphere : public phShapeCapsule
{
public:
	// PURPOSE: Initialize the swept sphere.
	// PARAMS:
	//	worldProbe - the capsule's segment (start and end hemisphere center positions) in world coordinates
	//	radius - the capsule's radius
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
	//	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

	// <COMBINE phShapeProbe::TestBoundPrimitive>
	bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::TestPolygon>
	bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

	bool GetTestInitialSphere() const { return m_TestInitialSphere; }
	void SetTestInitialSphere(bool testInitialSphere) { m_TestInitialSphere = testInitialSphere; }

#if __PFDRAW
	// <COMBINE phShapeProbe::DrawShape>
	void DrawShape (bool retest = false);

	// <COMBINE phShapeProbe::DrawIntersections>
	void DrawIntersections (bool retest = false);
#endif

private:
	// NOTE: The only valid reason to have this boolean instead of a derived class that always tests the initial sphere is that
	//         we can get away with 1 SPU job. 
	bool m_TestInitialSphere;
};

class phShapeTaperedSweptSphere : public phShapeBase
{
public:
	struct LocalizedData;

	// PURPOSE: Initialize the tapered swept sphere
	// PARAMS:
	//	worldProbe - the swept sphere's segment (start and end hemisphere center positions) in world coordinates
	//  initalRadius - the radius of the swept sphere at T == 0
	//  finalRadius - the radius of the swept sphere at T == 1
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
	//	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitTaperedSweptSphere (const phSegmentV& worldProbe, ScalarV_In initialRadius, ScalarV_In finalRadius, phIntersection* intersection=NULL, int numIntersections=1);

	// <COMBINE phShapeProbe::SetupIteratorCull>
	void SetupIteratorCull (phIterator& levelIterator);

	// <COMBINE phShapeProbe::RejectBound>
	bool RejectBound (const LocalizedData& localizedData, const phBound& bound) const;
	__forceinline bool RejectBoundFromCompositeBoundIteration(const LocalizedData& localizedData, const phBound& component) const { return RejectBound(localizedData,component); }

	// <COMBINE phShapeProbe::TestBoundPrimitive>
	bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::TestPolygon>
	bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::PopulateCuller>
	void PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler);

	// <COMBINE phShapeProbe::GetWorldSegment>
	const phSegmentV& GetWorldSegment () const;

	ScalarV_Out GetInitialRadius() const;
	ScalarV_Out GetFinalRadius() const;

#if __PFDRAW
	// <COMBINE phShapeProbe::DrawShape>
	void DrawShape (bool retest = false);

	// <COMBINE phShapeProbe::DrawIntersections>
	void DrawIntersections (bool retest = false);
#endif

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about all intersections.
	void DebugReplay () const;
#endif

	struct LocalizedData : public phShapeBase::LocalizedData
	{
		void Initialize(const phSegmentV& segment, ScalarV_In initialRadius, ScalarV_In finalRadius)
		{
			m_Segment = segment;
			m_Min = Min(Subtract(segment.GetStart(), Vec3V(initialRadius)), Subtract(segment.GetEnd(), Vec3V(finalRadius)));
			m_Max = Max(     Add(segment.GetStart(), Vec3V(initialRadius)),      Add(segment.GetEnd(), Vec3V(finalRadius)));
		}

		phSegmentV m_Segment;
		Vec3V m_Min;
		Vec3V m_Max;
	};
	void InitLocalizedData(LocalizedData& localizedData) const
	{
		localizedData.Initialize(GetWorldSegment(),GetInitialRadius(),GetFinalRadius());
	}
	void TransformLocalizedData(const LocalizedData& oldLocalizedData, LocalizedData& newLocalizedData, Mat34V_In oldFromNew) const
	{
		newLocalizedData.Initialize(phSegmentV(	UnTransformOrtho(oldFromNew,oldLocalizedData.m_Segment.GetStart()),
												UnTransformOrtho(oldFromNew,oldLocalizedData.m_Segment.GetEnd())),
									GetInitialRadius(), GetFinalRadius());
	}
	u32 GetLocalizedDataSize() const { return sizeof(LocalizedData); }

protected:
	// PURPOSE: the test segment in world coordinates
	phSegmentV m_WorldSegment;

	// PURPOSE: the initial and final radius of the swept sphere
	float m_InitialRadius;
	float m_FinalRadius;
};

inline const phSegmentV& phShapeTaperedSweptSphere::GetWorldSegment () const
{
	return m_WorldSegment;
}

inline ScalarV_Out phShapeTaperedSweptSphere::GetInitialRadius() const
{
	return ScalarV(m_InitialRadius);
}
inline ScalarV_Out phShapeTaperedSweptSphere::GetFinalRadius() const
{
	return ScalarV(m_FinalRadius);
}


class phShapeScalingSweptQuad : public phShapeBase
{
public:
	struct LocalizedData;

	// PURPOSE: Initialize the scaling swept quad
	// PARAMS:
	//  rotation - The rotation of the quad. Col0 = normal, Col1 = up = Y component of scale, Col3 = right = X component of scale
	//  worldSegment - The segment of the initial and final quad centers
	//  initialHalfExtents - The initial half extents of the quad (cannot be negative)
	//  finalHalfExtents - The final half extents of the quad (cannot be negative)
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
	//	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitScalingSweptQuad (Mat33V_In rotation, const phSegmentV& worldSegment, Vec2V_In initialHalfExtents, Vec2V_In finalHalfExtents, phIntersection* intersection=NULL, int numIntersections=1);

	// <COMBINE phShapeProbe::SetupIteratorCull>
	void SetupIteratorCull (phIterator& levelIterator);

	// <COMBINE phShapeProbe::RejectBound>
	bool RejectBound (const LocalizedData& localizedData, const phBound& bound) const;
	__forceinline bool RejectBoundFromCompositeBoundIteration(const LocalizedData& localizedData, const phBound& component) const { return RejectBound(localizedData,component); }

	// <COMBINE phShapeProbe::TestBoundPrimitive>
	bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::TestPolygon>
	bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::PopulateCuller>
	void PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler);

	Mat33V_ConstRef GetWorldRotation () const;
	const phSegmentV& GetWorldSegment () const;
	Vec2V_Out GetInitialHalfExtents() const;
	Vec2V_Out GetFinalHalfExtents() const;

	ScalarV_Out GetCapsuleRadius() const;

#if __PFDRAW
	// <COMBINE phShapeProbe::DrawShape>
	void DrawShape (bool retest = false);

	// <COMBINE phShapeProbe::DrawIntersections>
	void DrawIntersections (bool retest = false);
#endif

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about all intersections.
	void DebugReplay () const;
#endif

	void Reset ();

	bool SweepStartIntersectionFound() const { return m_SweepStartIntersectionFound; }

	struct LocalizedData : public phShapeBase::LocalizedData
	{
		Mat33V m_Rotation;
		phSegmentV m_Segment;
	};
	void InitLocalizedData(LocalizedData& localizedData) const
	{
		localizedData.m_Rotation = GetWorldRotation();
		localizedData.m_Segment = GetWorldSegment(); 
	}
	void TransformLocalizedData(const LocalizedData& oldLocalizedData, LocalizedData& newLocalizedData, Mat34V_In oldFromNew) const
	{
		newLocalizedData.m_Segment.Set(	UnTransformOrtho(oldFromNew,oldLocalizedData.m_Segment.GetStart()),
										UnTransformOrtho(oldFromNew,oldLocalizedData.m_Segment.GetEnd())	);
		UnTransformOrtho(newLocalizedData.m_Rotation, oldFromNew.GetMat33ConstRef(), oldLocalizedData.m_Rotation);
	}
	u32 GetLocalizedDataSize() const { return sizeof(LocalizedData); }

protected:
	// The orientation of the quad, this doesn't change during the cast. Col0=quad normal, Col1=up, Col2=right
	Mat33V m_WorldRotation;

	// The start and end position of the quad center
	phSegmentV m_WorldSegment;

	// Half extents of the initial and final quad. X=right=Col2, Y=up=Col1
	Vec2V m_InitialHalfExtents;
	Vec2V m_FinalHalfExtents;

	ScalarV m_CapsuleRadius; // The capsules radius that contains the whole cast

	bool m_SweepStartIntersectionFound; // This will be set after an intersection at T=0 is found and cleared at the start of each test. 
};

inline Mat33V_ConstRef phShapeScalingSweptQuad::GetWorldRotation () const
{
	return m_WorldRotation;
}

inline const phSegmentV& phShapeScalingSweptQuad::GetWorldSegment () const
{
	return m_WorldSegment;
}

inline Vec2V_Out phShapeScalingSweptQuad::GetInitialHalfExtents() const
{
	return m_InitialHalfExtents;
}

inline Vec2V_Out phShapeScalingSweptQuad::GetFinalHalfExtents() const
{
	return m_FinalHalfExtents;
}

inline ScalarV_Out phShapeScalingSweptQuad::GetCapsuleRadius() const
{
	return m_CapsuleRadius;
}

inline void phShapeScalingSweptQuad::Reset ()
{
	phShapeBase::Reset();
	m_SweepStartIntersectionFound = false;
}

class phShapeBatch : public phShapeComposite
{
public:
	struct LocalizedData;

	template <class ShapeType>
	class phShapeGroup
	{
		friend class phShapeBatch;
	public:
		// PURPOSE: Set the array of shapes and active flags.
		// PARAMS:
		//	probeList - the list of shape tests
		//	maxNumProbes - the size of the arrays of shapes and active flags.
		void SetShapes (ShapeType* shapeList, int maxNumShapes);

		// PURPOSE: Allocate the array of shapes and active flags.
		// PARAMS:
		//	maxNumProbes - the size of the arrays of shapes and active flags.
		void Allocate (int maxNumShapes);

		// PURPOSE: Deallocate the array of shapes and active flags.
		void Delete ();

		// PURPOSE:	Get a reference to the specified shape in the group.
		// PARAMS:
		//	shapeIndex - the index of the shape to get
		// RETURN: a reference to the specified shape in the batch
		ShapeType& GetShape (int shapeIndex);

		// PURPOSE:	Get a reference to the specified shape in the group.
		// PARAMS:
		//	shapeIndex - the index of the shape to get
		// RETURN: a reference to the specified shape in the batch
		const ShapeType& GetShape (int shapeIndex) const;

		// PURPOSE: Get the maximum number of shapes.
		// RETURN:	the maximum number of shapes in this group.
		int GetMaxNumShapes () const;

		// PURPOSE: Get the number of shapes.
		// RETURN:	the number of shapes in this group.
		int GetNumShapes () const;

		// PURPOSE: Set the number of shapes to 0.
		void ResetNumShapes ();

#if __DEV
		bool DoIntersectionsConflict(const phShapeBase& otherShape) const;
		bool DoIntersectionsConflict(const phShapeBatch& otherBatchShape) const;
#endif // __DEV

	private:
		phShapeGroup(int numMaxShapes);
		void DeleteIfOwned ();
		void Reset ();
		int GetNumHits () const;

#if __PFDRAW
		void Draw (bool retest);
		void DrawIntersections (bool retest);
#endif // __PFDRAW

		void SetIgnoreMaterialFlags (phMaterialFlags ignoreFlags);

#if __DEBUGLOG
		void DebugReplay () const;
#endif // __DEBUGLOG

		void InitLocalizedData(u8*& localizedDataBuffer) const;

		void TransformLocalizedData(const u8*& oldLocalizedDataBuffer, u8*& newLocalizedData, Mat34V_In oldFromNew) const;

		bool TestBoundPrimitive (const u8*& localizedDataBuffer, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

		bool TestPolygon (const u8*& localizedDataBuffer, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

		bool TestGeometryBound(const u8*& localizedDataBuffer, const phBoundGeometry& geomBound, Vec3V_ConstPtr vertices, u32 boundTypeFlags, u32 boundIncludeFlags, const phShapeBase::PartialIntersection& boundPartialIntersection);
	
		u32 GetLocalizedDataSize() const { return sizeof(typename ShapeType::LocalizedData)*m_NumShapes; };
	private:
		// PURPOSE: dynamic array of shapes
		ShapeType* m_Shapes;

		// PURPOSE: number of shapes in group
		u8 m_NumShapes;

		// PURPOSE: maximum number of shapes in this group
		u8 m_MaxNumShapes;

		// PURPOSE: flag to tell whether to delete the shape lists in the destructor
		bool m_OwnedShapes;
	};

	// PURPOSE:	Get a reference to the group of probes in the batch.
	phShapeGroup<phShapeProbe>& GetProbeGroup();

	// PURPOSE:	Get a reference to the group of edges in the batch.
	phShapeGroup<phShapeEdge>& GetEdgeGroup();

	// PURPOSE:	Get a reference to the group of spheres in the batch.
	phShapeGroup<phShapeSphere>& GetSphereGroup();

	// PURPOSE:	Get a reference to the group of capsules in the batch.
	phShapeGroup<phShapeCapsule>& GetCapsuleGroup();

	// PURPOSE:	Get a reference to the group of swept spheres in the batch.
	phShapeGroup<phShapeSweptSphere>& GetSweptSphereGroup();

	// PURPOSE:	Get a reference to the group of probes in the batch.
	const phShapeGroup<phShapeProbe>& GetProbeGroup() const;

	// PURPOSE:	Get a reference to the group of edges in the batch.
	const phShapeGroup<phShapeEdge>& GetEdgeGroup() const;

	// PURPOSE:	Get a reference to the group of spheres in the batch.
	const phShapeGroup<phShapeSphere>& GetSphereGroup() const;

	// PURPOSE:	Get a reference to the group of capsules in the batch.
	const phShapeGroup<phShapeCapsule>& GetCapsuleGroup() const;

	// PURPOSE:	Get a reference to the group of swept spheres in the batch.
	const phShapeGroup<phShapeSweptSphere>& GetSweptSphereGroup() const;

	// PURPOSE:	Get a reference to the specified probe in the batch.
	// PARAMS:
	//	probeIndex - the index of the probe to get
	// RETURN: a reference to the specified probe in the batch
	phShapeProbe& GetProbe(int probeIndex);

	// PURPOSE:	Get a reference to the specified edge in the batch.
	// PARAMS:
	//	probeIndex - the index of the edge to get
	// RETURN: a reference to the specified edge in the batch
	phShapeEdge& GetEdge(int edgeIndex);

	// PURPOSE:	Get a reference to the specified sphere in the batch.
	// PARAMS:
	//	sphereIndex - the index of the sphere to get
	// RETURN: a reference to the specified sphere in the batch
	phShapeSphere& GetSphere(int sphereIndex);

	// PURPOSE:	Get a reference to the specified capsule in the batch.
	// PARAMS:
	//	capsuleIndex - the index of the capsule to get
	// RETURN: a reference to the specified capsule in the batch
	phShapeCapsule& GetCapsule(int capsuleIndex);

	// PURPOSE:	Get a reference to the specified swept sphere in the batch.
	// PARAMS:
	//	sweptSphereIndex - the index of the swept sphere to get
	// RETURN: a reference to the specified swept sphere in the batch
	phShapeSweptSphere& GetSweptSphere(int sweptSphereIndex);

	// PURPOSE:	Get a reference to the specified probe in the batch.
	// PARAMS:
	//	probeIndex - the index of the probe to get
	// RETURN: a reference to the specified probe in the batch
	const phShapeProbe& GetProbe(int probeIndex) const;

	// PURPOSE:	Get a reference to the specified edge in the batch.
	// PARAMS:
	//	probeIndex - the index of the edge to get
	// RETURN: a reference to the specified edge in the batch
	const phShapeEdge& GetEdge(int edgeIndex) const;

	// PURPOSE:	Get a reference to the specified sphere in the batch.
	// PARAMS:
	//	sphereIndex - the index of the sphere to get
	// RETURN: a reference to the specified sphere in the batch
	const phShapeSphere& GetSphere(int sphereIndex) const;

	// PURPOSE:	Get a reference to the specified capsule in the batch.
	// PARAMS:
	//	capsuleIndex - the index of the capsule to get
	// RETURN: a reference to the specified capsule in the batch
	const phShapeCapsule& GetCapsule(int capsuleIndex) const;

	// PURPOSE:	Get a reference to the specified swept sphere in the batch.
	// PARAMS:
	//	sweptSphereIndex - the index of the swept sphere to get
	// RETURN: a reference to the specified swept sphere in the batch
	const phShapeSweptSphere& GetSweptSphere(int sweptSphereIndex) const;

	// PURPOSE: Get the number of batched probes.
	// RETURN:	the number of probes in this batch
	int GetNumProbes() const;

	// PURPOSE: Get the number of batched edges.
	// RETURN:	the number of edges in this batch
	int GetNumEdges() const;

	// PURPOSE: Get the number of batched spheres.
	// RETURN:	the number of spheres in this batch
	int GetNumSpheres() const;

	// PURPOSE: Get the number of batched capsules.
	// RETURN:	the number of capsules in this batch
	int GetNumCapsules() const;

	// PURPOSE: Get the number of batched swept spheres.
	// RETURN:	the number of swept spheres in this batch
	int GetNumSweptSpheres() const;

	// PURPOSE: Get the number of batched boxes.
	// RETURN:	the number of boxes in this batch
//	int GetNumBoxes () const;

	// PURPOSE: Allocate the array of batched probes and active flags.
	// PARAMS:
	//	maxNumProbes - the size of the arrays of batched probes and active flags.
	void AllocateProbes(int maxNumProbes);

	// PURPOSE: Allocate the array of batched edges and active flags.
	// PARAMS:
	//	maxNumProbes - the size of the arrays of batched edges and active flags.
	void AllocateEdges(int maxNumEdges);

	// PURPOSE: Allocate the array of batched spheres and active flags.
	// PARAMS:
	//	maxNumSpheres - the size of the arrays of batched spheres and active flags.
	void AllocateSpheres(int maxNumSpheres);

	// PURPOSE: Allocate the array of batched capsules and active flags.
	// PARAMS:
	//	maxNumCapsules - the size of the arrays of batched capsules and active flags.
	void AllocateCapsules(int maxNumCapsules);

	// PURPOSE: Allocate the array of batched swept spheres and active flags.
	// PARAMS:
	//	maxNumSweptSpheres - the size of the arrays of batched swept spheres and active flags.
	void AllocateSweptSpheres(int maxNumSweptSpheres);

	// PURPOSE: Set the array of batched probes and active flags.
	// PARAMS:
	//	probeList - the list of probe shape tests
	//	maxNumProbes - the size of the arrays of batched probes and active flags.
	void SetProbes(phShapeProbe* probeList, int maxNumProbes);

	// PURPOSE: Set the array of batched edges and active flags.
	// PARAMS:
	//	probeList - the list of edge shape tests
	//	maxNumProbes - the size of the arrays of batched edges and active flags.
	void SetEdges(phShapeEdge* probeList, int maxNumEdges);

	// PURPOSE: Set the array of batched spheres and active flags.
	// PARAMS:
	//	sphereList - the list of sphere shape tests
	//	maxNumSpheres - the size of the arrays of batched spheres and active flags.
	void SetSpheres(phShapeSphere* sphereList, int maxNumSpheres);

	// PURPOSE: Set the array of batched capsules and active flags.
	// PARAMS:
	//	capsuleList - the list of capsule shape tests
	//	maxNumCapsules - the size of the arrays of batched capsules and active flags.
	void SetCapsules(phShapeCapsule* capsuleList, int maxNumCapsules);

	// PURPOSE: Set the array of batched swept spheres and active flags.
	// PARAMS:
	//	sweptSphereList - the list of swept spheres shape tests
	//	maxNumSweptSpheres - the size of the arrays of batched swept spheres and active flags.
	void SetSweptSpheres(phShapeSweptSphere* sweptSphereList, int maxNumSweptSpheres);

	// PURPOSE: default constructor
	phShapeBatch ();


	// PURPOSE: constructor
	// PARAMS:
	//	maxNumProbes - maximum number of batched probes
	//	maxNumEdges - maximum number of batched edges
	//	maxNumSpheres - maximum number of batched spheres
	//	maxNumCapsules - maximum number of batched capsules
	//	maxNumSweptSpheres - maximum number of batched swept spheres
	phShapeBatch (int maxNumProbes, int maxNumEdges, int maxNumSpheres, int maxNumCapsules, int maxNumSweptSpheres);

	// PURPOSE: destructor
	~phShapeBatch ();

	// PURPOSE: Initialize a point in this batch of shape tests.
	// PARAMS:
	//	worldCenter - the point's position in world coordinates
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//		If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitPoint (Vec3V_In worldCenter, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a sphere in this batch of shape tests.
	// PARAMS:
	//	worldCenter - the sphere's position in world coordinates
	//	radius - the sphere's radius
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//		If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitSphere (Vec3V_In worldCenter, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a directed probe in this batch of shape tests.
	// PARAMS:
	//	worldProbe - the probe's segment (start and end positions) in world coordinates
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//		If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitProbe (const phSegmentV& worldProbe, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize an undirected probe in this batch of shape tests.
	// PARAMS:
	//	worldProbe - the probe's segment (start and end positions) in world coordinates
	//	intersection - optional pointer into which to record entry intersection information
	//	exitIntersection - optional pointer into which to record exit intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//		If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitEdge (const phSegmentV& worldProbe, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize an undirected capsule in this batch of shape tests.
	// PARAMS:
	//	worldProbe - the capsule's segment (start and end hemisphere center positions) in world coordinates
	//	radius - the capsule's radius
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//		If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitCapsule (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a swept sphere in this batch of shape tests.
	// PARAMS:
	//	worldProbe - the swept sphere's segment (start and end hemisphere center positions) in world coordinates
	//	radius - the sphere's radius
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	// NOTES:
	//		If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	void InitSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a box in this batch of shape tests.
	// PARAMS:
	//	boxAxesWorld - the box's unit axes and location
	//	boxHalfSize - the box's half size along each of its axes
	//	intersection - optional pointer into which to record intersection information
	//	numIntersections - optional maximum number of intersections to find
	//	boxIndex - optional index number in the list of boxes in this batch of shape tests (default uses the next available number)
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
	//	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
//	Box tests are not yet implemented
//	void InitBox (Mat34V_In boxAxesWorld, Vec3V_In boxHalfSize, phIntersection* intersection=NULL, int numIntersections=1, int boxIndex=BAD_INDEX);

	// PURPOSE: Initialize a sphere and a directed capsule (a swept sphere) in this batch of shape tests.
	// PARAMS:
	//	worldProbe - the capsule's segment (start and end hemisphere center positions) in world coordinates
	//	radius - the capsule's radius
	//	sphereIntersection - optional pointer into which to record the sphere intersection information
	//	numSphereIntersections - optional maximum number of intersections to find
	//	capsuleIntersection - optional pointer into which to record the swept sphere intersection information
	//	numCapsuleIntersections - optional maximum number of intersections to find
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX, then only the earliest intersection will be filled in
	//	2.	If numIntersections>0 then "intersection" must be a list of intersections (could still be only one), which will be filled in in the order
	//		found, until there are no more objects to hit or the list is full
	//	3.	There test sphere is located at the swept sphere's starting point.
	void InitSphereAndSweptSphere (const phSegment& worldProbe, float radius, phIntersection* sphereIntersection=NULL, int numSphereIntersections=-1, phIntersection* capsuleIntersection=NULL, int numCapsuleIntersections=-1);
	void InitSphereAndSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* sphereIntersection=NULL, int numSphereIntersections=-1, phIntersection* capsuleIntersection=NULL, int numCapsuleIntersections=-1);

	// PURPOSE: Compute the average position of the centers of all the batched shapes.
	void ComputeCentroid ();

	// PURPOSE: Compute a surrounding sphere as the cull shape for this batch.
	void ComputeCullSphere ();

	void SetCullCapsule (const Vector3& capsuleEnd, const Vector3& capsuleAxis, float length, float radius);

	// PURPOSE: Compute a surrounding axis-aligned box as the cull shape for this batch.
	void ComputeAxisAlignedCullBox ();

	// PURPOSE: Set the cull shape for this batch to be the specified box.
	// PARAMS:
	//	boxAxes - the coordinate matrix for the cull box
	//	boxHalfWidths - the half-width of the cull box along each axis
	// NOTES:
	//	1.	The default cull shape is a sphere.
	//	2.	boxAxes is a 3x4 matrix for the cull box's coordinates - the 3x3 part must be orthonormal and the d-vector is the center position
	void SetCullBox (const Matrix34& boxAxes, const Vector3& boxHalfWidths);

	phCullShape::phCullType GetCullType () const;
	const Matrix34& GetCullBoxAxes () const;
	const Vector3& GetCullBoxHalfSize () const;
	const Vector3& GetCullSphereWorldCenter () const;
	float GetCullSphereRadius () const;
	float GetCullCapsuleLength () const;
	Vector3 GetCullCapsuleAxis () const;

	// PURPOSE: Clear out all the member shapes in this batch.
	void DeleteShapes ();

	// <COMBINE phShapeBase::Reset>
	void Reset ();

	// <COMBINE phShapeProbe::SetupIteratorCull>
	void SetupIteratorCull (phIterator& levelIterator);

	// <COMBINE phShapeBase::GetNumHits>
	int GetNumHits () const;

	// <COMBINE phShapeProbe::RejectBound>
	bool RejectBound (const LocalizedData& localizedData, const phBound& bound) const;
	__forceinline bool RejectBoundFromCompositeBoundIteration(const LocalizedData& localizedData, const phBound& component) const { return RejectBound(localizedData,component); }
	__forceinline bool RejectBoundFromBvhBoundCull(const LocalizedData& localizedData, const phBound& primitive) const { return RejectBound(localizedData,primitive); }

	// <COMBINE phShapeBase::RejectPolygon>
	bool RejectPolygon(const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& vertices);
	__forceinline bool RejectPolygonFromBvhBoundCull(const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& vertices) { return RejectPolygon(localizedData,polygon,vertices); }
	__forceinline bool RejectPolygonFromGeometryBoundIteration(const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& vertices) { return RejectPolygon(localizedData,polygon,vertices); }

	// <COMBINE phShapeProbe::TestBoundPrimitive>
	bool TestBoundPrimitive (const LocalizedData& localizedData, const phBound& bound, u32 primTypeFlags, u32 primIncludeFlags, const PartialIntersection& partialIntersection);

	// <COMBINE phShapeProbe::TestPolygon>
	bool TestPolygon (const LocalizedData& localizedData, const phPolygon& polygon, const Vector3& boundVertices, u32 polyTypeFlags, u32 polyIncludeFlags, const phBoundGeometry& geomBound, const PrimitivePartialIntersection& partialIntersection);

	bool TestGeometryBound(const LocalizedData& localizedData, const phBoundGeometry& geomBound, Vec3V_ConstPtr vertices, u32 boundTypeFlags, u32 boundIncludeFlags, const phShapeBase::PartialIntersection& boundPartialIntersection);

	// <COMBINE phShapeProbe::PopulateCuller>
	void PopulateCuller(const LocalizedData& localizedData, const phOptimizedBvh& bvhStructure, phBoundCuller& culler);

#if __DEV
	bool DoIntersectionsConflict(const phShapeBase& otherShape) const;
#endif // __DEV

#if __PFDRAW
	// <COMBINE phShapeProbe::DrawShape>
	void DrawShape (bool retest = false);

	// <COMBINE phShapeProbe::DrawIntersections>
	void DrawIntersections (bool retest = false);
#endif

	// <COMBINE phShapeProbe::SetIgnoreMaterialFlags>
	void SetIgnoreMaterialFlags (phMaterialFlags ignoreFlags);

#if __DEBUGLOG
	// PURPOSE: Record replay debugging data about all intersections.
	void DebugReplay () const;
#endif

	bool IsBooleanTest() const { return false; }

	void PrepareForShapeTest();

	struct LocalizedData : public phShapeComposite::LocalizedData
	{
		Vector3 m_Center;
		Matrix34 m_BoxAxes;
	};
	void InitLocalizedData(LocalizedData& localizedData) const;
	void TransformLocalizedData(const LocalizedData& oldLocalizedData, LocalizedData& newLocalizedData, Mat34V_In oldFromNew) const;
	u32 GetLocalizedDataSize() { return m_LocalizedDataSize; }

protected:

	phShapeGroup<phShapeProbe> m_Probes;
	phShapeGroup<phShapeEdge> m_Edges;
	phShapeGroup<phShapeSphere> m_Spheres;
	phShapeGroup<phShapeCapsule> m_Capsules;
	phShapeGroup<phShapeSweptSphere> m_SweptSpheres;
//	phShapeGroup<phShapeBox> m_Boxes;

	// PURPOSE: the center of the sphere surrounding the batch of shapes in world coordinates
	Vector3 m_BatchWorldCenter;

	// PURPOSE: shape information for optional box culling
	Matrix34 m_BatchBoxAxesWorld;
	Vector3 m_BatchBoxHalfSize;

	// PURPOSE: shape information for capsule culling
	Vector3 m_BatchCapsuleEnd,m_BatchCapsuleAxis;
	float m_BatchCapsuleLength;

	// PURPOSE: the radius of the sphere surrounding the batch of shapes
	float m_BatchRadius;

	// PURPOSE: the shape used for culling in the physics level (sphere or box)
	phCullShape::phCullType m_BatchCullType;

	u32 m_LocalizedDataSize;
};

// phShapeBatch::phShapeGroup inlined methods
template <class ShapeType> phShapeBatch::phShapeGroup<ShapeType>::phShapeGroup(int numMaxShapes)
{
	m_NumShapes = 0;
	m_OwnedShapes = false;
	m_Shapes = NULL;
	Allocate(numMaxShapes);
}

template <class ShapeType> __forceinline ShapeType& phShapeBatch::phShapeGroup<ShapeType>::GetShape(int shapeIndex)
{
	Assert(shapeIndex<m_MaxNumShapes);
	return m_Shapes[shapeIndex];
}

template <class ShapeType> __forceinline const ShapeType& phShapeBatch::phShapeGroup<ShapeType>::GetShape(int shapeIndex) const
{
	Assert(shapeIndex<m_MaxNumShapes);
	return m_Shapes[shapeIndex];
}

template <class ShapeType> __forceinline int phShapeBatch::phShapeGroup<ShapeType>::GetMaxNumShapes() const
{
	return m_MaxNumShapes;
}

template <class ShapeType> __forceinline int phShapeBatch::phShapeGroup<ShapeType>::GetNumShapes() const
{
	return m_NumShapes;
}

template <class ShapeType> __forceinline void phShapeBatch::phShapeGroup<ShapeType>::ResetNumShapes()
{
	m_NumShapes = 0;
}

// phShapeBatch inlined methods
__forceinline phShapeBatch::phShapeGroup<phShapeProbe>& phShapeBatch::GetProbeGroup()
{
	return m_Probes;
}
__forceinline phShapeBatch::phShapeGroup<phShapeEdge>& phShapeBatch::GetEdgeGroup()
{
	return m_Edges;
}
__forceinline phShapeBatch::phShapeGroup<phShapeSphere>& phShapeBatch::GetSphereGroup()
{
	return m_Spheres;
}
__forceinline phShapeBatch::phShapeGroup<phShapeCapsule>& phShapeBatch::GetCapsuleGroup()
{
	return m_Capsules;
}
__forceinline phShapeBatch::phShapeGroup<phShapeSweptSphere>& phShapeBatch::GetSweptSphereGroup()
{
	return m_SweptSpheres;
}

__forceinline const phShapeBatch::phShapeGroup<phShapeProbe>& phShapeBatch::GetProbeGroup() const
{
	return m_Probes;
}
__forceinline const phShapeBatch::phShapeGroup<phShapeEdge>& phShapeBatch::GetEdgeGroup() const
{
	return m_Edges;
}
__forceinline const phShapeBatch::phShapeGroup<phShapeSphere>& phShapeBatch::GetSphereGroup() const
{
	return m_Spheres;
}
__forceinline const phShapeBatch::phShapeGroup<phShapeCapsule>& phShapeBatch::GetCapsuleGroup() const
{
	return m_Capsules;
}
__forceinline const phShapeBatch::phShapeGroup<phShapeSweptSphere>& phShapeBatch::GetSweptSphereGroup() const
{
	return m_SweptSpheres;
}

__forceinline phShapeProbe& phShapeBatch::GetProbe(int probeIndex)
{
	return m_Probes.GetShape(probeIndex);
}
__forceinline phShapeEdge& phShapeBatch::GetEdge(int edgeIndex)
{
	return m_Edges.GetShape(edgeIndex);
}
__forceinline phShapeSphere& phShapeBatch::GetSphere(int sphereIndex)
{
	return m_Spheres.GetShape(sphereIndex);
}
__forceinline phShapeCapsule& phShapeBatch::GetCapsule(int capsuleIndex)
{
	return m_Capsules.GetShape(capsuleIndex);
}
__forceinline phShapeSweptSphere& phShapeBatch::GetSweptSphere(int sweptSphereIndex)
{
	return m_SweptSpheres.GetShape(sweptSphereIndex);
}

__forceinline const phShapeProbe& phShapeBatch::GetProbe(int probeIndex) const
{
	return m_Probes.GetShape(probeIndex);
}
__forceinline const phShapeEdge& phShapeBatch::GetEdge(int edgeIndex) const
{
	return m_Edges.GetShape(edgeIndex);
}
__forceinline const phShapeSphere& phShapeBatch::GetSphere(int sphereIndex) const
{
	return m_Spheres.GetShape(sphereIndex);
}
__forceinline const phShapeCapsule& phShapeBatch::GetCapsule(int capsuleIndex) const
{
	return m_Capsules.GetShape(capsuleIndex);
}
__forceinline const phShapeSweptSphere& phShapeBatch::GetSweptSphere(int sweptSphereIndex) const
{
	return m_SweptSpheres.GetShape(sweptSphereIndex);
}

__forceinline int phShapeBatch::GetNumProbes() const
{
	return m_Probes.GetNumShapes();
}
__forceinline int phShapeBatch::GetNumEdges() const
{
	return m_Edges.GetNumShapes();
}
__forceinline int phShapeBatch::GetNumSpheres() const
{
	return m_Spheres.GetNumShapes();
}
__forceinline int phShapeBatch::GetNumCapsules() const
{
	return m_Capsules.GetNumShapes();
}
__forceinline int phShapeBatch::GetNumSweptSpheres() const
{
	return m_SweptSpheres.GetNumShapes();
}

__forceinline void phShapeBatch::AllocateProbes(int maxNumProbes)
{
	m_Probes.Allocate(maxNumProbes);
}
__forceinline void phShapeBatch::AllocateEdges(int maxNumEdges)
{
	m_Edges.Allocate(maxNumEdges);
}
__forceinline void phShapeBatch::AllocateSpheres(int maxNumSpheres)
{
	m_Spheres.Allocate(maxNumSpheres);
}
__forceinline void phShapeBatch::AllocateCapsules(int maxNumCapsules)
{
	m_Capsules.Allocate(maxNumCapsules);
}
__forceinline void phShapeBatch::AllocateSweptSpheres(int maxNumSweptSpheres)
{
	m_SweptSpheres.Allocate(maxNumSweptSpheres);
}

__forceinline void phShapeBatch::SetProbes(phShapeProbe* probeList, int maxNumProbes)
{
	m_Probes.SetShapes(probeList, maxNumProbes);
}
__forceinline void phShapeBatch::SetEdges(phShapeEdge* edgeList, int maxNumEdges)
{
	m_Edges.SetShapes(edgeList, maxNumEdges);
}
__forceinline void phShapeBatch::SetSpheres(phShapeSphere* sphereList, int maxNumSpheres)
{
	m_Spheres.SetShapes(sphereList, maxNumSpheres);
}
__forceinline void phShapeBatch::SetCapsules(phShapeCapsule* capsuleList, int maxNumCapsules)
{
	m_Capsules.SetShapes(capsuleList, maxNumCapsules);
}
__forceinline void phShapeBatch::SetSweptSpheres(phShapeSweptSphere* sweptSphereList, int maxNumSweptSpheres)
{
	m_SweptSpheres.SetShapes(sweptSphereList, maxNumSweptSpheres);
}

inline phCullShape::phCullType phShapeBatch::GetCullType () const
{
	return m_BatchCullType;
}

inline const Matrix34& phShapeBatch::GetCullBoxAxes () const
{
	return m_BatchBoxAxesWorld;
}

inline const Vector3& phShapeBatch::GetCullBoxHalfSize () const
{
	return m_BatchBoxHalfSize;
}

inline const Vector3& phShapeBatch::GetCullSphereWorldCenter () const
{
	return m_BatchWorldCenter;
}

inline float phShapeBatch::GetCullSphereRadius () const
{
	return m_BatchRadius;
}

inline float phShapeBatch::GetCullCapsuleLength () const
{
	return m_BatchCapsuleLength;
}

inline Vector3 phShapeBatch::GetCullCapsuleAxis () const
{
	return m_BatchCapsuleAxis;
}

typedef atDelegate<bool(const phInst*)> IncludeInstanceCallback;
typedef atDelegate<bool(const phInst*, const phPrimitive&, phMaterialMgr::Id materialId)> IncludePrimitiveCallback;

class phShapeTestCullResults
{
public:
	typedef u16 CulledPrimitiveIndex;
	typedef u16 CulledInstanceIndex;

	class CulledInstance
	{
	public:
		CulledInstance(){}
		CulledInstance(const phInst* instance, CulledPrimitiveIndex firstPrimitiveIndex) : m_Instance(instance), m_FirstPrimitiveIndex(firstPrimitiveIndex)
		{}

		const phInst* GetInstancePtr() const { return m_Instance; }
		CulledPrimitiveIndex GetFirstPrimitiveIndex() const { return m_FirstPrimitiveIndex; }

	private:
		const phInst* m_Instance;
		CulledPrimitiveIndex m_FirstPrimitiveIndex;
	};

	enum ResultLevel
	{
		RESULTLEVEL_NONE,
		RESULTLEVEL_INSTANCE,
		RESULTLEVEL_PRIMITIVE
	};


	// Constructors/Destructors
	phShapeTestCullResults() : 
		m_ResultLevel(RESULTLEVEL_NONE),
		m_Instances(NULL),
		m_PrimitiveIndices(NULL),
		m_OwnsInstanceArray(false),
		m_OwnsPrimitiveIndexArray(false)
	{
		ResetForRead();
		ResetForWrite();
	}

	~phShapeTestCullResults()
	{
		ReleaseInstanceArray();
		ReleasePrimitiveIndexArray();
	}

	// Methods for managing how deep of a cull this is
	ResultLevel GetResultLevel() const { return m_ResultLevel; }
	ResultLevel GetDeepestPossibleResultLevel() const
	{ 
		if(m_Instances)
		{
			if(m_PrimitiveIndices)
			{
				return RESULTLEVEL_PRIMITIVE;
			}
			else
			{
				return RESULTLEVEL_INSTANCE;
			}
		}
		else
		{
			return RESULTLEVEL_NONE;
		}
	}
	void SetResultLevel(ResultLevel resultLevel)
	{ 
		ResultLevel deepestPossibleResultLevel = GetDeepestPossibleResultLevel();
		Assertf(resultLevel <= deepestPossibleResultLevel, "Trying to use cull results without required result arrays.");
		m_ResultLevel = Min(deepestPossibleResultLevel,resultLevel); 
	}
	bool GetIncludeInstances() { return GetResultLevel() >= RESULTLEVEL_INSTANCE; }
	bool GetIncludePrimitives() { return GetResultLevel() >= RESULTLEVEL_PRIMITIVE; }

	// Methods for managing array memory
	void ReleaseInstanceArray()
	{
		if(m_OwnsInstanceArray)
		{
			delete [] m_Instances;
		}
		m_Instances = NULL;
		m_OwnsInstanceArray = false;
		m_MaxNumInstances = 0;
		ResetForWrite();
	}
	void AllocateInstanceArray(CulledInstanceIndex maxNumInstances)
	{
		ReleaseInstanceArray();
		if(maxNumInstances > 0)
		{
			m_Instances = rage_new CulledInstance[maxNumInstances];
		}
		m_MaxNumInstances = maxNumInstances;
		m_OwnsInstanceArray = true;
	}
	void SetInstanceArray(CulledInstance* culledInstances, CulledInstanceIndex maxNumInstances)
	{
		ReleaseInstanceArray();
		Assert(culledInstances != NULL || maxNumInstances == 0);
		m_Instances = culledInstances;
		m_MaxNumInstances = maxNumInstances;
		m_OwnsInstanceArray = false;
	}
	void CopyInstanceArray(const phInst** culledInstances, CulledInstanceIndex numInstances)
	{
		Assert(m_Instances);
		Assert(numInstances <= m_MaxNumInstances);
		CulledInstanceIndex numInstancesToCopy = Min(m_MaxNumInstances,numInstances);
		m_NumInstances = numInstancesToCopy;
		for(CulledInstanceIndex instanceIndex = 0; instanceIndex < numInstancesToCopy; ++instanceIndex)
		{
			m_Instances[instanceIndex] = CulledInstance(culledInstances[instanceIndex],0);
		}
	}

	void ReleasePrimitiveIndexArray()
	{
		if(m_OwnsPrimitiveIndexArray)
		{
			delete [] m_PrimitiveIndices;
		}
		m_PrimitiveIndices = NULL;
		m_OwnsPrimitiveIndexArray = false;
		m_MaxNumPrimitives = 0;
		ResetForWrite();
	}
	void AllocatePrimitiveIndexArray(CulledPrimitiveIndex maxNumPrimitives)
	{
		ReleasePrimitiveIndexArray();
		if(maxNumPrimitives > 0)
		{
			m_PrimitiveIndices = rage_new phPolygon::Index[maxNumPrimitives];
		}
		m_MaxNumPrimitives = maxNumPrimitives;
		m_OwnsPrimitiveIndexArray = true;
	}
	void SetPrimitiveIndexArray(phPolygon::Index* primitiveIndices, CulledPrimitiveIndex maxNumPrimitives)
	{
		ReleasePrimitiveIndexArray();
		Assert(primitiveIndices != NULL || maxNumPrimitives == 0);
		m_PrimitiveIndices = primitiveIndices;
		m_MaxNumPrimitives = maxNumPrimitives;
		m_OwnsPrimitiveIndexArray = false;
	}

	// Methods for writing a cull
	void ResetForWrite()
	{
		m_NumInstances = 0;
		m_NumPrimitives = 0;
	}
	void AddInstance(const phInst* instance)
	{
		Assert(m_Instances && m_MaxNumInstances != 0);
		if(!Verifyf(m_NumInstances < m_MaxNumInstances,"Ran out of room for culled instances. Max = %i",m_MaxNumInstances))
		{
			// If we do run out of instances it's easiest to just completely get rid of the previous one, otherwise it causes problems
			//   when we add culled stuff to this instance
			RemoveLastInstance();
		}
		
		m_Instances[m_NumInstances++] = CulledInstance(instance,m_NumPrimitives);
	}

	void AddPrimitives(const phPolygon::Index* primitiveIndices, CulledPrimitiveIndex numPrimitives)
	{
		Assert(m_Instances && m_NumInstances > 0);
		// Figure out how many primitives we can actually add
		CulledPrimitiveIndex currentNumCulledPrimitives = m_NumPrimitives;
		CulledPrimitiveIndex newNumCulledPrimitives = Min(m_MaxNumPrimitives,(CulledPrimitiveIndex)(currentNumCulledPrimitives+numPrimitives));
		m_NumPrimitives = newNumCulledPrimitives;

		// Copy over as many primitives as we can. Assert if we're ignoring any
		CulledPrimitiveIndex numPrimitivesToAdd = newNumCulledPrimitives - currentNumCulledPrimitives;
		Assertf(numPrimitivesToAdd==numPrimitives,"Ran out of room for culled primitives. Max = %i.",m_MaxNumPrimitives);
		memcpy(&m_PrimitiveIndices[currentNumCulledPrimitives],primitiveIndices,sizeof(m_PrimitiveIndices[0])*numPrimitivesToAdd);
	}


	// Methods for reading a cull
	void SetCurrentInstanceIndex(CulledInstanceIndex instanceIndex) const
	{
		TrapGE(instanceIndex,GetNumInstances());
		m_CurrentInstanceIndex = instanceIndex; 
	}
	void ResetForRead() const
	{
		m_CurrentInstanceIndex = 0;
	}
	CulledInstanceIndex GetNumInstances() const { return m_NumInstances; }
	const phInst* GetCurrentInstance() const { return m_Instances[m_CurrentInstanceIndex].GetInstancePtr(); }

	const phPolygon::Index* GetCurrentPrimitiveIndices() const { return &m_PrimitiveIndices[m_Instances[m_CurrentInstanceIndex].GetFirstPrimitiveIndex()]; }
	CulledPrimitiveIndex GetCurrentNumPrimitives() const
	{
		CulledPrimitiveIndex endPrimitiveIndex;
		if(m_CurrentInstanceIndex == m_NumInstances - 1)
		{
			endPrimitiveIndex = m_NumPrimitives;	
		}
		else
		{
			endPrimitiveIndex = m_Instances[m_CurrentInstanceIndex + 1].GetFirstPrimitiveIndex();
		}

		return endPrimitiveIndex - m_Instances[m_CurrentInstanceIndex].GetFirstPrimitiveIndex();
	}

private:
	void RemoveLastInstance()
	{
		if(m_NumInstances > 0)
		{
			// Decrement the number of instances and set the current number of primitives to the first primitive
			//   of the removed instance
			m_NumPrimitives = m_Instances[--m_NumInstances].GetFirstPrimitiveIndex();
		}
	}

	ResultLevel m_ResultLevel;

	CulledInstance* m_Instances;
	CulledInstanceIndex m_NumInstances;
	CulledInstanceIndex m_MaxNumInstances;
	bool m_OwnsInstanceArray;

	phPolygon::Index* m_PrimitiveIndices;
	CulledPrimitiveIndex m_NumPrimitives;
	CulledPrimitiveIndex m_MaxNumPrimitives;
	bool m_OwnsPrimitiveIndexArray;

	mutable CulledInstanceIndex m_CurrentInstanceIndex;
};

namespace phShapeTestCull
{
	enum State
	{
		READ,		// Do a level cull for instances and BVH culls for primitives, the cull results won't be touched and aren't necessary
		WRITE,		// Clear the cull results and fill its arrays, requires a cull results set
		DEFAULT		// Only test the saved culled instances and use the saved culled primitives if they exist, doesn't modify the cull results
	};
}

template <class ShapeType> class phShapeTest
{
public:
	// PURPOSE: default constructor
	phShapeTest ();

	// PURPOSE: Initialize a point shape test.
	// PARAMS:
	//	worldCenter - the point position in world coordinates
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	void InitPoint (const Vector3& worldCenter, phIntersection* intersection=NULL, int numIntersections=1);
	void InitPoint (Vec3V_In worldCenter, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a sphere shape test.
	// PARAMS:
	//	worldCenter - the sphere center in world coordinates
	//	radius - the sphere radius
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	void InitSphere (const Vector3& worldCenter, float radius, phIntersection* intersection=NULL, int numIntersections=1);
	void InitSphere (Vec3V_In worldCenter, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a probe shape test.
	// PARAMS:
	//	worldProbe - the probe's segment in world coordinates
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	void InitProbe (const phSegment& worldProbe, phIntersection* intersection=NULL, int numIntersections=1);
	void InitProbe (const phSegmentV& worldProbe, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize an edge shape test.
	// PARAMS:
	//	worldProbe - the edge's segment in world coordinates
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the first intersection is filled in (if it is provided).
	//	2.	An edge is an indirected probe - it finds entry and exit intersections.
	void InitEdge (const phSegment& worldProbe, phIntersection* intersection=NULL, int numIntersections=1);
	void InitEdge (const phSegmentV& worldProbe, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a swept sphere shape test.
	// PARAMS:
	//	worldProbe - the swept sphere's segment in world coordinates
	//	radius - the sphere radius
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	//	2.	A swept sphere is a sphere moving along the segment, detecting objects on the surface of the moving sphere.
	void InitSweptSphere (const phSegment& worldProbe, float radius, phIntersection* intersection=NULL, int numIntersections=1);
	void InitSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a tapered swept sphere shape test.
	// PARAMS:
	//	worldProbe - the swept sphere's segment in world coordinates
	//  initialRadius - radius of sphere at T == 0
	//  finalRadius - radius of sphere at T == 1
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	//	2.	A swept sphere is a sphere moving along the segment, detecting objects on the surface of the moving sphere.
	void InitTaperedSweptSphere (const phSegment& worldProbe, float initialRadius, float finalRadius, phIntersection* intersection=NULL, int numIntersections=1);
	void InitTaperedSweptSphere (const phSegmentV& worldProbe, ScalarV_In initialRadius, ScalarV_In finalRadius, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a scaling swept quad shape test.
	// PARAMS:
	//  rotation - the rotation of the quad in world space
	//	worldProbe - the swept quads's segment in world coordinates
	//  initialHalfExtents - initial half extents of the quad
	//  finalHalfExtents - final half extents of the quad
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	//	2.	A swept sphere is a sphere moving along the segment, detecting objects on the surface of the moving sphere.
	void InitScalingSweptQuad (Mat33V_In rotation, const phSegmentV& worldProbe, Vec2V_In initialHalfExtents, Vec2V_In finalHalfExtents, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a capsule shape test.
	// PARAMS:
	//	worldProbe - the capsule's segment in world coordinates
	//	radius - the capsule radius
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	void InitCapsule (const phSegment& worldProbe, float radius, phIntersection* intersection=NULL, int numIntersections=1);
	void InitCapsule (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize an object shape test.
	// PARAMS:
	//	bound - the bound to use as a test shape
	//	transform - the world transform of the bound
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	void InitObject (const phBound& bound, const Matrix34& transform, phIntersection* intersection=NULL, int numIntersections=1);
	void InitObject (const phBound& bound, Mat34V_In transform, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize an object shape test.
	// PARAMS:
	//	bound - the bound to use as a test shape
	//	transform - the world transform of the bound
	//	lastTransform - the last transform of the bound. Used to set up a swept test.
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	void InitObject (const phBound& bound, const Matrix34& transform, const Matrix34& lastTransform, phIntersection* intersection=NULL, int numIntersections=1);
	void InitObject (const phBound& bound, Mat34V_In transform, Mat34V_In lastTransform, phIntersection* intersection=NULL, int numIntersections=1);

//	Box tests are not yet implemented
	// PURPOSE: Initialize a box shape test.
	// PARAMS:
	//	boxAxesWorld - the box's unit axes and position in world coordinates
	//	boxHalfSize - the box's half size along each box axis
	//	intersection - optional intersection pointer
	//	numIntersections - optional number of intersections
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
//	void InitBox (const Matrix34& boxAxesWorld, const Vector3& boxHalfSize, phIntersection* intersection=NULL, int numIntersections=1);

	// PURPOSE: Initialize a swept sphere shape test.
	// PARAMS:
	//	worldProbe - the capsule's segment (start and end hemisphere center positions) in world coordinates
	//	radius - the capsule's radius
	//	sphereIntersection - optional pointer into which to record the sphere intersection information
	//	numSphereIntersections - optional maximum number of intersections to find
	//	capsuleIntersection - optional pointer into which to record the swept sphere intersection information
	//	numCapsuleIntersections - optional maximum number of intersections to find
	// NOTES:
	//	1.	If numIntersections is BAD_INDEX (the default) then only the deepest intersection is filled in (if one is provided).
	//	2.	A swept sphere is a sphere moving along the segment, detecting objects on the surface of the moving sphere.
	//	3.	There test sphere is located at the swept sphere's starting point.
	void InitSphereAndSweptSphere (const phSegment& worldProbe, float radius, phIntersection* sphereIntersection=NULL, int numSphereIntersections=-1, phIntersection* capsuleIntersection=NULL, int numCapsuleIntersections=-1);
	void InitSphereAndSweptSphere (const phSegmentV& worldProbe, ScalarV_In radius, phIntersection* sphereIntersection=NULL, int numSphereIntersections=-1, phIntersection* capsuleIntersection=NULL, int numCapsuleIntersections=-1);

	// PURPOSE: Set a list of exclude instances.
	// PARAMS:
	//	instanceList - a list of instance pointers to exclude from shape test results
	//	numInstances - the number of instance pointers
	// NOTES:	This is optional, for excluding multiple instances. Single instances can be excluded with the call to TestInLevel.
	void SetExcludeInstanceList (const phInst* const * instanceList, int numInstances);

	// PURPOSE: Clear the list of exclude instances.
	void ClearExcludeInstanceList ();

	// PURPOSE: Tell if the given instance is in the exclude instance list.
	// PARAMS:
	//	instance - the instance to try to match with the instances in the exclude list
	// RETURN:	false if the given instance is in the exclude list, true if it is not
	bool NotInExclusionList (const phInst* instance) const;

#if !__SPU
	// PURPOSE: Set the callback function to determine if individual physics instances should be tested.
	// PARAMS:
	//	includeInstanceCallback - the callback function to test instances for inclusion
	// NOTES:
	//	The default function is this->NotInExcludeInstanceList.
	void SetIncludeInstanceCallback (const IncludeInstanceCallback& includeInstanceCallback);

	// PURPOSE: Toggle whether or not the include instance callback will be used.
	// PARAMS:
	//	use - whether or not to use the include instance callback
	void SetUseIncludeInstanceCallback (bool use);

	// PURPOSE: Set the callback function to determine if individual physics polygons should be tested.
	// PARAMS:
	//	includeInstanceCallback - the callback function to test polygons for inclusion
	void SetIncludePrimitiveCallback (const IncludePrimitiveCallback& includePrimitiveCallback);

	// PURPOSE: Toggle whether or not the include polygon callback will be used.
	// PARAMS:
	//	use - whether or not to use the include polygon callback
	void SetUseIncludePrimitiveCallback (bool use);
#endif

	void SetExcludeInstance(const phInst * instance) { m_ExcludeInstance = instance; }
	void SetIncludeFlags(const u32 iFlags) { m_IncludeFlags = iFlags; }
	void SetTypeFlags(const u32 iFlags) { m_TypeFlags = iFlags; }
	void SetStateIncludeFlags(const u8 iFlags) { m_StateIncludeFlags = iFlags; }
	void SetTypeExcludeFlags(const u32 iFlags) { m_TypeExcludeFlags = iFlags; }
	void SetLevel(const phLevelNew * pLevel) { m_Level = pLevel; }

	const phInst * GetExcludeInstance() { return m_ExcludeInstance; }
	u32 GetIncludeFlags() { return m_IncludeFlags; }
	u32 GetTypeFlags() { return m_TypeFlags; }
	u8 GetStateIncludeFlags() { return m_StateIncludeFlags; }
	u32 GetTypeExcludeFlags() { return m_TypeExcludeFlags; }
	const phLevelNew * GetLevel() { return m_Level; }


	// PURPOSE:
	//   Setting this to true will make tests against phBoundGeometry and phBoundCurvedGeometry not test each polygon separately. Instead
	//     the bound will be treated as a convex object. 
	// NOTES:
	//   Currently, any geometry/curved geometry intersections generated with this flag will have a polygon index of 0 and a material ID equal to the first
	//     material ID in the material ID array on the bound. 
	void SetTreatPolyhedralBoundsAsPrimitives(bool treatPolyhedralBoundsAsPrimitives) { m_TreatPolyhedralBoundsAsPrimitives = treatPolyhedralBoundsAsPrimitives; }
	bool GetTreatPolyhedralBoundsAsPrimitives() const { return m_TreatPolyhedralBoundsAsPrimitives; }

	// PURPOSE: Test the shape against a single object.
	// PARAMS:
	//	bound - reference to the object's bound
	//	instance - optional pointer to the object's physics instance (default is NULL, in which case the bound is assumed to be upright at the origin)
	// RETURN:	the number of hits on the given object
	// NOTES:	If no instance pointer is given, the bound is assumed to be upright at the origin.
	int TestOneObject (const phBound& bound, const phInst* instance=NULL);

	// PURPOSE: Test the shape against a single object.
	// PARAMS:
	//	bound - reference to the object's bound
	//	instance - reference to the object's physics instance
	// RETURN:	the number of hits on the given object
	int TestOneObject (const phBound& bound, const phInst& instance);

	// PURPOSE: Test the shape against a single object.
	// PARAMS:
	//	instance - reference to the object's physics instance
	// RETURN:	the number of hits on the given object
	int TestOneObject (const phInst& instance);

	// PURPOSE: Perform the shape test in the physics level.
	// PARAMS:
	//	excludeInstance - instance to exclude from the results
	//	includeFlags - optional flags to match with tested objects' type flags
	//	typeFlags - optional flags to match with test objects' include flags
	//	stateIncludeFlags - optional list of states to include (active, inactive, fixed)
	//	typeExcludeFlags - optional flags to exclude an object from testing, any are in an object's type flags
	//	level - optional pointer to the physics level, offered to allow tools to do shape tests on inactive levels
	//	acquirePhysicsLock - optional boolean to tell whether to lock the physics level
	int TestInLevel (const phInst* excludeInstance
#if ENABLE_PHYSICS_LOCK && __SPU
		, phIterator& levelIterator
#endif
		, u32 includeFlags=INCLUDE_FLAGS_ALL, u32 typeFlags=TYPE_FLAGS_ALL, u8 stateIncludeFlags=phLevelBase::STATE_FLAGS_ALL,
						u32 typeExcludeFlags=TYPE_FLAGS_NONE, const phLevelNew* level=NULL
						);

	// PURPOSE: As above, except that the function params are filled in from the include and exclude members in this phShapeTest
	// PARAMS:
	//	acquirePhysicsLock - optional boolean to tell whether to lock the physics level
#if ENABLE_PHYSICS_LOCK
#	if !__SPU
	int TestInLevel ();
#	else	// !__SPU
	int TestInLevel (phIterator& levelIterator);
#	endif	// !__SPU
#else
	int TestInLevel ();
#endif

	// PURPOSE: See if the given bound intersects this shape test.
	// PARAMS:
	//	bound - the bound to check for intersections with this shape test
	//	instance - the object's physics instance using the given bound
	//	levelIndex - the level index of the instance
	//	generationId - the generation of the level index (see notes)
	//	component - the index number of the bound if it is part of a grid or composite bound
	//	includeFlags - flags to match with tested objects' type flags
	//	typeFlags - flags to match with test objects' include flags
	// RETURN:	true if this shape test intersects the given bound, false if it does not
	// NOTES:
	//	1. This is public, but it is generally for internal use. It does not use the instance matrix.
	//	2. Use TestOneObject to find intersections with a single object in world coordinates.
	//	3. The level index is the same as instance->GetLevelIndex(), but is separate because this is called internally where it is already available.
	//	4. The generation identification is PHLEVEL->GetGenerationID(levelIndex). It is used to handle the physics level removing an object and then
	//		re-using the level index on a different object before the results of this test are used.
	bool TestBound (const typename ShapeType::LocalizedData& localizedData, const phBound& bound, u32 boundTypeFlags, u32 boundIncludeFlags, u32 includeFlags, u32 typeFlags, const phShapeBase::PartialIntersection& boundPartialIntersection);

	// PURPOSE:	Repeat a shape test with the same culling information.
	// RETURN:	the number of intersections
	int RetestIntersections ();

	// PURPOSE: Test the shape against the object in the given intersection, when an intersection has already been found so the same culling information can be used.
	// PARAMS:
	//	intersection - reference to the intersection to test again
	//	bound - reference to the bound to test
	//	instance - reference to the instance to test
	// RETURN:	the number of intersections
	// NOTES:	This is called from RetestIntersections, which is used to repeat a shape test with the same culling information.
	int RetestIntersection (const typename ShapeType::LocalizedData& boundLocalizedData, phIntersection& intersection, const phBound& bound, const phShapeBase::PartialIntersection& boundPartialIntersection);

	// PURPOSE: Get the shape for this shape test.
	// RETURN:	the shape for this shape test
	ShapeType& GetShape ();

	// PURPOSE: Get the shape for this shape test.
	// RETURN:	the shape for this shape test
	ShapeType const& GetShape () const;

	// PURPOSE: Read the exclude instance list
	// RETURN:	the exclude instance list
	void GetExcludeInstanceList(phInst const * const * & outExcludeInstanceList, int& numExcludeInstances) const;


#if PROFILE_INDIVIDUAL_SHAPETESTS
	// RETURN: -1 if there was no test completed yet, otherwise the time of the last test in microseconds.
	float GetCompletionTime() const { return m_CompletionTime; }
#endif // PROFILE_INDIVIDUAL_SHAPETESTS

protected:
	void HandleDMADecompressVerticesForBVHPolygon(Vector3 *localVertices, const phPolygon &polygon, const phBoundGeometry &geomBound);
	phPolygon *CloneAndRewireLocalBVHPolygon(phPolygon &localPolygon, const phPolygon &sourcePolygon);
	
#if __SPU
	void WaitForBvhLoopDMAs() const;
	void StartPrimitiveDMA(phPolygon::Index primitiveIndex, phPrimitive* spuPrimitive, const phBoundBVH& bvhBound) const;
	phPolygon::Index StartMaterialIndexDMA(phPolygon::Index firstPrimitiveIndex, u8* spuMaterialIndices, const u8* ppuMaterialIndices, int numIndicesToGrab) const;
#endif // __SPU

	bool TestInstanceInLevel(const typename ShapeType::LocalizedData& worldLocalizedData, const phInst* instance, u32 includeFlags, u32 typeFlags, const phInst* excludeInst, const phLevelNew* physicsLevel SPU_ONLY(,phIterator& levelIterator));

	// PURPOSE: the shape for this shape test
	ShapeType m_ShapeType;

	// PURPOSE: optional exclude instance list (individual instances can also be excluded in calls to TestInLevel)
	const phInst* m_ExcludeInstanceList[MAX_EXCLUDE_INSTANCES];

public:
	phShapeTestCull::State GetCullState() const { return m_CullState; }
	void SetCullState(phShapeTestCull::State cullState) { m_CullState = cullState; }
	void SetCullResults(phShapeTestCullResults* cullResults) { m_CullResults = cullResults; }
	const phShapeTestCullResults* GetCullResults() const { return m_CullResults; }
private:
	phShapeTestCull::State m_CullState;
	phShapeTestCullResults* m_CullResults;

	// PURPOSE: the number of instance pointers in the exclude instance list
	int m_NumExcludeInstances;

	// PURPOSE: if true, shapetests will test against the convex hull of boundGeoms rather than each triangle
	bool m_TreatPolyhedralBoundsAsPrimitives;

	// PURPOSE: Function pointer to exclude instances from testing. The default is this->NotInExclusionList().
	IncludeInstanceCallback m_IncludeInstanceCallback;

	// PURPOSE: Function pointer to exclude polygons from testing.
	IncludePrimitiveCallback m_IncludePrimitiveCallback;

	// PURPOSE: If this is false then don't call the IncludeInstanceCallback because it hasn't been installed.
	bool m_UseIncludeInstanceCallback;

	// PURPOSE: If this is false then don't call the IncludePrimitiveCallback because it hasn't been installed.
	bool m_UseIncludePrimitiveCallback;

	// PURPOSE: single exclude instance, in addition to the list above
	const phInst* m_ExcludeInstance;
	
	// PURPOSE: flags determining which types of instances will be tested or excluded
	u32 m_IncludeFlags;
	u32 m_TypeFlags;
	u8 m_StateIncludeFlags;
	u32 m_TypeExcludeFlags;

	// PURPOSE: the level in which this test will be performed
	const phLevelNew* m_Level;

#if PROFILE_INDIVIDUAL_SHAPETESTS
SPU_ONLY(public:)
	float m_CompletionTime;
#endif // PROFILE_INDIVIDUAL_SHAPETESTS
};

class phShapeTestTaskData
{
public:

	enum ParamIndices
	{
		ParamIndex_ProbeShapeTest = 0,
		ParamIndex_EdgeShapeTest,
		ParamIndex_SphereShapeTest,
		ParamIndex_CapsuleShapeTest,
		ParamIndex_EmptyShapeTest,
		ParamIndex_BatchShapeTest,
		ParamIndex_SweptSphereShapeTest,
		ParamIndex_TaperedSweptSphereShapeTest,
		ParamIndex_ScalingSweptQuadShapeTest,
		ParamIndex_ShapeTestCount,
		ParamIndex_PhysicsLevel = ParamIndex_ShapeTestCount,
		ParamIndex_GlobalReaderCount,
		ParamIndex_PhysicsMutex,
		ParamIndex_AllowNewReaderMutex,
		ParamIndex_ModifyReaderCountMutex,
		ParamIndex_Count
	};

	static const int s_NumBatchedThreaded = 12;

	void Init ();
#if !__SPU
	__forceinline phShapeTestTaskData() : m_TaskParameters(sysTaskParameters::SIMPLE_CONSTRUCTOR) {}
	void InitFromDefault ();
#endif // !__SPU
	void SetProbes (phShapeTest<phShapeProbe>* probeTestList, int numProbes);
	void SetEdges (phShapeTest<phShapeEdge>* edgeTestList, int numEdges);
	void SetSpheres (phShapeTest<phShapeSphere>* sphereTestList, int numSpheres);
	void SetCapsules (phShapeTest<phShapeCapsule>* capsuleTestList, int numCapsules);
	void SetSweptSpheres (phShapeTest<phShapeSweptSphere>* sweptSphereTestList, int numSweptSpheres);
	void SetTaperedSweptSpheres (phShapeTest<phShapeTaperedSweptSphere>* taperedSweptSphereTestList, int numTaperedSweptSpheres);
	void SetScalingSweptQuads (phShapeTest<phShapeScalingSweptQuad>* scalingSweptQuadTestList, int numScalingSweptQuads);
	void SetBatches (phShapeTest<phShapeBatch>* batchTestList, int numBatches);
	void SetProbe (phShapeTest<phShapeProbe>& probeTest);
	void SetEdge (phShapeTest<phShapeEdge>& edgeTest);
	void SetSphere (phShapeTest<phShapeSphere>& sphereTest);
	void SetCapsule (phShapeTest<phShapeCapsule>& capsuleTest);
	void SetSweptSphere (phShapeTest<phShapeSweptSphere>& sweptSphereTest);
	void SetTaperedSweptSphere (phShapeTest<phShapeTaperedSweptSphere>& taperedSweptSphereTest);
	void SetScalingSweptQuad (phShapeTest<phShapeScalingSweptQuad>& scalingSweptQuadTest);
	void SetBatch (phShapeTest<phShapeBatch>& batchTest);

	sysTaskHandle CreateTask (int iSchedulerIndex=0);

#if __DEBUGLOG
	void DebugReplay() const;
#endif

#if !__SPU
	static void InitDefault();
#endif // !__SPU

public:
	sysTaskParameters m_TaskParameters;
	u8 m_NumProbes,m_NumEdges,m_NumSpheres,m_NumCapsules,m_NumSweptSpheres,m_NumTaperedSweptSpheres,m_NumScalingSweptQuads,m_NumBatches;

	u8 m_Padding[4];

#if !__SPU
	static phShapeTestTaskData sm_DefaultShapeTestTaskData;
	static bool sm_DefaultShapeTestTaskDataInitialized;
#endif // !__SPU
} ;


class phShapeTestTaskManager
{
public:
	void Init ();
	void SetProbes (int taskIndex, phShapeTest<phShapeProbe>* probeTestList, int numProbes);
	void SetSpheres (int taskIndex, phShapeTest<phShapeSphere>* sphereTestList, int numSpheres);
	void SetCapsules (int taskIndex, phShapeTest<phShapeCapsule>* capsuleTestList, int numCapsules);
	void SetSweptSpheres (int taskIndex, phShapeTest<phShapeSweptSphere>* sweptSphereTestList, int numSweptSpheres);
	void SetTaperedSweptSpheres (int taskIndex, phShapeTest<phShapeTaperedSweptSphere>* taperedSweptSphereTestList, int numTaperedSweptSpheres);
	void SetScalingSweptQuads (int taskIndex, phShapeTest<phShapeScalingSweptQuad>* scalingSweptQuadTestList, int numScalingSweptQuads);
	void SetBatches (int taskIndex, phShapeTest<phShapeBatch>* batchTestList, int numBatches);
	void SetProbe (int taskIndex, phShapeTest<phShapeProbe>& probeTest);
	void SetSphere (int taskIndex, phShapeTest<phShapeSphere>& sphereTest);
	void SetCapsule (int taskIndex, phShapeTest<phShapeCapsule>& capsuleTest);
	void SetSweptSphere (int taskIndex, phShapeTest<phShapeSweptSphere>& sweptSphereTest);
	void SetTaperedSweptSphere (int taskIndex, phShapeTest<phShapeTaperedSweptSphere>& taperedSweptSphereTest);
	void SetScalingSweptQuad (int taskIndex, phShapeTest<phShapeScalingSweptQuad>& scalingSweptQuadTest);
	void SetBatch (int taskIndex, phShapeTest<phShapeBatch>& batchTest);

	void CreateTasks ();
	void CompleteTasks ();
	void WaitTasks ();

	static const int s_NumShapeTestTasks = 8;

protected:
	phShapeTestTaskData m_TaskData[s_NumShapeTestTasks];

	// PURPOSE: list of task handles
	// NOTES:	The task handles are owned by each phShapeTestTask; this is just a copied list of pointers.
	sysTaskHandle m_TaskHandles[s_NumShapeTestTasks];
};


} // namespace rage

#endif
