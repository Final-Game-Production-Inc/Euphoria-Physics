//
// physics/levelnew.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_LEVELNEW_H
#define PHYSICS_LEVELNEW_H

#include "leveldefs.h"

#include "handle.h"
#include "inst.h"
#include "sleepisland.h"

#include "atl/atfunctor.h"
#include "atl/bitset.h"
#include "phbound/boundculler.h"
#include "phcore/segment.h"
#include "physics/collisionoverlaptest.h"
#include "system/criticalsection.h"
#include "vectormath/classes.h"


#if !__SPU
#define PHLEVEL (::rage::phLevelNew::GetActiveInstance())						// global singleton accessor macro
#endif // !__SPU


// This is useful for debugging the SPU version of the code with the convenience of a non-SPU debugger like VS.
#define EMULATE_SPU	(0 && !__PPU)
#define LEVELNEW_SPU_CODE (__SPU || EMULATE_SPU)

#if !__SPU
#define SPU_PUBLIC
#define SPU_PROTECTED
#else	// !__SPU
#define SPU_PUBLIC public:
#define SPU_PROTECTED protected:
#endif	// !__SPU

#if __SPU
#define SPU_PARAM(X) , X
#define NON_SPU_ONLY(x)
#else	// __SPU
#define SPU_PARAM(X)
#define NON_SPU_ONLY(x)	x
#endif	// __SPU

#define LEVELNEW_USE_EVERYWHERE_NODE	HACK_GTA4	// HACK_GTA4 - turned this option on for gta, keep for jimmy

#define LEVELNEW_GENERATION_IDS			1

// This controls whether or not you have the option of enabling deferred octree updates.
#define LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE	1

// This enables composite BVH updates and rebuilds to be deferred
//   If set to false BVHs will only be updated on the main thread
#define LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE 1

#define LEVELNEW_ACTIVE_LEVELINDEX_DUPE_CHECKING (__DEV)

////////////////////////////////////////////////////////////////
// external defines


#include "levelbase.h"

#include "vector/geometry.h"

#define DEFAULT_NUM_ISECTS	1


namespace rage {


class phBroadPhase;
class phIterator;
class btOverlappingPairCache;
class phIntersection;
class phBoundComposite;


class phLooseQuadtreeNode
{
public:
	phLooseQuadtreeNode();
	~phLooseQuadtreeNode();

	__forceinline static int GetBranchingFactor() { return 4; }

	void Clear();

	// Determine whether a point is contained within the inner bounds of this node (can be used to determine whether a given sphere 
	//   should be placed in this node).
	__forceinline bool IsPointWithinNode(Vec3V_In krvecPoint) const
	{
		const Vec2V flattenedPoint = krvecPoint.GetXY();
		const Vec2V flattenedPointToNodeCenter = Subtract(flattenedPoint, m_CenterXYZHalfWidthW.GetXY());

		const ScalarV vsHalfWidth = m_CenterXYZHalfWidthW.GetW();
		return IsLessThanOrEqualAll(Abs(flattenedPointToNodeCenter), Vec2V(vsHalfWidth)) != 0;
	}


	// Determine whether a sphere intersects the outer bounds of this node.  You should use this when you think that the sphere is in the general
	//   vicinity of the node (it skips the quicker reject tests).
	__forceinline bool DoesSphereIntersect(Vec3V_In vkrvecCenter, const float& kfRadius) const
	{
		const ScalarV sphereRadius = ScalarVFromF32(kfRadius);
		return DoesSphereIntersectSpecial(vkrvecCenter, sphereRadius);
	}


	// Determine whether a sphere intersects the outer bounds of this node.
	__forceinline bool DoesSphereIntersectSpecial(Vec3V_In vkrvecCenter, ScalarV_In sphereRadius) const
	{
		const Vec2V flattenedSphereCenter = vkrvecCenter.GetXY();

		const Vec2V nodeCenter = m_CenterXYZHalfWidthW.GetXY();
		const ScalarV vsNodeHalfWidth = m_CenterXYZHalfWidthW.GetW();
		const Vec2V realHalfWidth = Vec2V(Add(vsNodeHalfWidth, vsNodeHalfWidth));
		const Vec2V clampedFlattenedSphereCenter = Clamp(flattenedSphereCenter, Subtract(nodeCenter, realHalfWidth), Add(nodeCenter, realHalfWidth));

		const ScalarV sphereRadiusSquared = Scale(sphereRadius, sphereRadius);
		return (IsLessThanAll(DistSquared(clampedFlattenedSphereCenter, flattenedSphereCenter), sphereRadiusSquared) != 0);
	}


#if __ASSERT
	__forceinline u8 GetPointLocationMask(Vector3::Vector3Param vkrvecPoint, const float kfMaxRealHalfWidth) const
	{
		Vector3 krvecPoint(vkrvecPoint);
		Vec3V vCenter = m_CenterXYZHalfWidthW.GetXYZ();
		u8 uRetVal = 0;
		if(krvecPoint.x <= vCenter.GetXf() - kfMaxRealHalfWidth) uRetVal = 1;
		else if(krvecPoint.x >= vCenter.GetXf() + kfMaxRealHalfWidth) uRetVal = 2;
		if(krvecPoint.y <= vCenter.GetYf() - kfMaxRealHalfWidth) uRetVal |= 4;
		else if(krvecPoint.y >= vCenter.GetYf() + kfMaxRealHalfWidth) uRetVal |= 8;
//		if(krvecPoint.z <= vCenter.GetZf() - kfMaxRealHalfWidth) uRetVal |= 16;
//		else if(krvecPoint.z >= vCenter.GetZf() + kfMaxRealHalfWidth) uRetVal |= 32;

		return uRetVal;
	}


	__forceinline bool DoesLineSegmentIntersect2(Vector3::Vector3Param krvecP0, Vector3::Vector3Param krvecP1) const
	{
		const float kfMaxRealHalfWidth = m_CenterXYZHalfWidthW.GetWf() + m_CenterXYZHalfWidthW.GetWf();

		u8 uPointMask0 = GetPointLocationMask(krvecP0, kfMaxRealHalfWidth);
		if(uPointMask0 == 0)
		{
			// This point is already inside of the cube.
			return true;
		}
		u8 uPointMask1 = GetPointLocationMask(krvecP1, kfMaxRealHalfWidth);
		if(uPointMask1 == 0)
		{
			// This point is already inside of the cube.
			return true;
		}
		Vector3 vecTempP0(krvecP0);
		Vector3 vecTempP1(krvecP1);
		Vector3 vecTempMid;
		while(true)
		{
			// Neither of the two end points of our segment are inside of the cube, and they're not both outside for any of the same reasons, so
			//   let's compute the midpoint and replace one of the end points with it.
			vecTempMid.Average(vecTempP0, vecTempP1);
			u8 uPointMaskMid = GetPointLocationMask(vecTempMid, kfMaxRealHalfWidth);

			if(uPointMaskMid == 0)
			{
				// This point is inside of the cube.
				return true;
			}
			if((uPointMaskMid & uPointMask0) != 0)
			{
				if((uPointMaskMid & uPointMask1) != 0)
				{
					// 
					return false;
				}
				uPointMask0 = uPointMaskMid;
				vecTempP0.Set(vecTempMid);
			}
			else
			{
				uPointMask1 = uPointMaskMid;
				vecTempP1.Set(vecTempMid);
			}
		}
	}
#endif	// __ASSERT

	__forceinline bool DoesLineSegmentIntersect(Vec3V_In vkrvecP0, Vec3V_In vkrvecP1) const
	{
		// Check for a separating axis - there are really only three to check.  The first two axes are the box face normals (the x and y axes in our case, since
		//   we know that our box (the node) is axis-aligned) and the third axis is the 'segment normal' (again, this is unique up to the sign since we're in 2d).
		const Vec2V flattenedSegEnd0 = vkrvecP0.GetXY();
		const Vec2V flattenedSegEnd1 = vkrvecP1.GetXY();
		const Vec2V nodeCenter = m_CenterXYZHalfWidthW.GetXY();
		const Vec2V flattenedSetEnd0ToNodeCenter = Subtract(flattenedSegEnd0, nodeCenter);
		// Note: We specifically don't normalize the 'pre normal' specifically for the case where the line segment has no 'xy' component.  This allows us to avoid
		//   having to handle that case in some way.  See the comments below for more information.  Should this become a problem for some reason, it shouldn't be
		//   too costly to just pick an arbitrary axis and use that instead.
		const Vec2V segmentPreNormal = Subtract(flattenedSegEnd1, flattenedSegEnd0).Get<Vec::Y, Vec::X>();
		// TODO: Perhaps multiplying by <-1, 1> would be faster?  Would still need to construct that constant though.
		const Vec2V segmentNormal = Vec2V(Negate(segmentPreNormal.GetX()), segmentPreNormal.GetY());
		const Vec2V absSegmentNormal = Abs(segmentNormal);

		const ScalarV vsNodeHalfWidth = m_CenterXYZHalfWidthW.GetW();
		const ScalarV realHalfWidth = Add(vsNodeHalfWidth, vsNodeHalfWidth);

		const ScalarV boxCenterAlongSegmentNormal = Dot(flattenedSetEnd0ToNodeCenter, segmentNormal);
		// Note: This could actually be formulated as a dot product, but I think this might be slightly faster.
		const ScalarV nodeHalfWidthAlongSegmentNormal = Scale(realHalfWidth, Add(absSegmentNormal.GetX(), absSegmentNormal.GetY()));

		// Put the extents along each axis into Vec3V's so that we can do all of the comparisons at once.
		const Vec3V boxMinAlongAxes = Vec3V(Subtract(nodeCenter, Vec2V(realHalfWidth)), Subtract(boxCenterAlongSegmentNormal, nodeHalfWidthAlongSegmentNormal));
		const Vec3V boxMaxAlongAxes = Vec3V(Add(nodeCenter, Vec2V(realHalfWidth)), Add(boxCenterAlongSegmentNormal, nodeHalfWidthAlongSegmentNormal));
		Assert(IsGreaterThanOrEqualAll(boxMaxAlongAxes, boxMinAlongAxes));
		const Vec3V segMinAlongAxes = Vec3V(Min(flattenedSegEnd0, flattenedSegEnd1), ScalarV(V_ZERO));
		const Vec3V segMaxAlongAxes = Vec3V(Max(flattenedSegEnd0, flattenedSegEnd1), ScalarV(V_ZERO));
		Assert(IsGreaterThanOrEqualAll(segMaxAlongAxes, segMinAlongAxes));

		// Figure along which axes we have overlap.  It's specifically important that these are 'OrEqual' tests since the 'segment normal' separating axis is
		//   allowed to be zero, which results in a zero min/max projection onto that axis, and we don't want to allow that to be considered a separating axis.
		VecBoolV vbOverlap = And(IsGreaterThanOrEqual(boxMaxAlongAxes, segMinAlongAxes), IsGreaterThanOrEqual(segMaxAlongAxes, boxMinAlongAxes));
		// We're overlapping only if none of the axes tested had overlap.
		const bool overlap = IsEqualIntAll(Or(vbOverlap, VecBoolV(V_F_F_F_T)), VecBoolV(V_T_T_T_T)) != 0;
		return overlap;
	}

	__forceinline bool DoesXZCircleIntersect(Vec3V_In vkrvecCenter, const float &kfRadius) const
	{
		// For a quadtree node, testing against a vertical cylinder is the same as testing against a sphere.
		return DoesSphereIntersect(vkrvecCenter, kfRadius);
	}


	__forceinline bool DoesCapsuleIntersect(Vec3V_In krvecP0, Vec3V_In krvecShaftAxis, ScalarV_In kvShaftLength, ScalarV_In kvCapsuleRadius) const
	{
		const ScalarV kvNodeHalfWidthFinal = Add(m_CenterXYZHalfWidthW.GetW(), m_CenterXYZHalfWidthW.GetW());
		const ScalarV vsFifteen(V_FIFTEEN);
		const ScalarV vsFifteenSquared = Scale(vsFifteen, vsFifteen);
		const ScalarV vsFifteenSquaredSquared = Scale(vsFifteenSquared, vsFifteenSquared);

		const Vec3V nodeHalfWidthFinal = Vec3V(Vec2V(kvNodeHalfWidthFinal), vsFifteenSquaredSquared);
		const Vec3V nodeCenter = Vec3V(m_CenterXYZHalfWidthW.GetXY(), ScalarV(V_ZERO));

		Vec3V vecP1 = krvecP0 + krvecShaftAxis * kvShaftLength;
		//return geomBoxes::TestCapsuleToAlignedBoxFP(krvecP0.GetIntrin128(), vecP1.GetIntrin128(), kvCapsuleRadius.GetIntrin128(), Subtract(nodeCenter, nodeHalfWidthFinal).GetIntrin128(), Add(nodeCenter, nodeHalfWidthFinal).GetIntrin128());
		return COT_TestCapsuleToAlignedBoxFP(krvecP0, vecP1, kvCapsuleRadius, Subtract(nodeCenter, nodeHalfWidthFinal), Add(nodeCenter, nodeHalfWidthFinal));
	}


	__forceinline bool DoesBoxIntersect(Mat34V_In boxAxes, Vec3V_In boxHalfSize) const
	{
		// NOTE: I don't really know if it's necessary to go through all of this hassle to set the w-components to zero.  The code that this was based on
		//   (phLooseOctreeNode::DoesBoxIntersect()) did it so I preserved that behavior here.  Really nobody should ever need to do that.
		const ScalarV nodeHalfWidth = Add(m_CenterXYZHalfWidthW.GetW(), m_CenterXYZHalfWidthW.GetW());

		// Why am I constructing this crazy constant?  Rather than write a special function for quadtree node-vs-oriented box (which could get messy) I
		//   decided just to treat this as box-vs-oriented box but with one of the boxes having an 'infinite' size in one dimension.  A natural value to
		//   use for that 'infinite' size would be V_FLT_MAX but, unfortunately, using that values seems to cause NAN's inside of the TestBoxToBoxXXX
		//   functions, which then causes bad results (it says that boxes are never intersecting even if they are).  As a result, I just created a large
		//   value that it hopefully large enough that it should be okay but still avoid any NAN problems.
		const ScalarV vsFifteen(V_FIFTEEN);
		const ScalarV vsFifteenSquared = Scale(vsFifteen, vsFifteen);
		const ScalarV vsFifteenSquaredSquared = Scale(vsFifteenSquared, vsFifteenSquared);
		const Vec4V nodeHalfSize = Vec4V(Vec2V(nodeHalfWidth), vsFifteenSquaredSquared, ScalarV(V_ZERO));
		const Vec3V nodeCenter = Vec3V(m_CenterXYZHalfWidthW.GetXY(), ScalarV(V_ZERO));
		Mat34V relBoxAxes(boxAxes);
		relBoxAxes.SetCol3(Subtract(relBoxAxes.GetCol3(), nodeCenter));
		const Vec4V boxHalfSizeZeroW(boxHalfSize, ScalarV(V_ZERO));
		//	const bool retVal = geomBoxes::TestBoxToBoxOBB(boxHalfSizeZeroW.GetIntrin128(), nodeHalfSize.GetIntrin128(), RCC_MATRIX34(relBoxAxes));
		//	const bool retVal = COT_TestBoxToBoxOBB(boxHalfSizeZeroW.GetXYZ(),nodeHalfSize.GetXYZ(),relBoxAxes);
		//const bool retVal = geomBoxes::TestBoxToBoxOBBFaces(boxHalfSizeZeroW.GetIntrin128(), nodeHalfSize.GetIntrin128(), RCC_MATRIX34(relBoxAxes));
		const bool retVal = COT_TestBoxToBoxOBBFaces(boxHalfSizeZeroW.GetXYZ(),nodeHalfSize.GetXYZ(),relBoxAxes);
		return retVal;
	}

	__forceinline bool DoesAABBIntersect(Vec3V_In boxCenter, Vec3V_In boxHalfSize) const
	{
		// Set the node center to whatever the box center is on the z-axis. This will mean that they will always overlap on that axis. As long as one of
		//  the box half sizes is non-zero it will find an overlap on the z-axis.
		Vec3V nodeCenter = Vec3V(m_CenterXYZHalfWidthW.GetXY(), boxCenter.GetZ());
		Vec3V nodeHalfSize = Vec3V(m_CenterXYZHalfWidthW.GetW());
		nodeHalfSize = Add(nodeHalfSize,nodeHalfSize);
		//return geomBoxes::TestAABBtoAABB_CenterHalfSize(boxCenter,boxHalfSize,nodeCenter,nodeHalfSize).Getb();
		return COT_TestAABBtoAABB_CenterHalfSize(boxCenter,boxHalfSize,nodeCenter,nodeHalfSize);
	}

	// Find out in which of the children [0..7] this point is located.
#if 0
	// Vec passed by const ref in case it's already in memory... since we don't want the values in a vector register within this function at all.
	int SubClassifyPoint(Vec3V_ConstRef vkrvecPoint) const
	{
		int nRetVal = (vkrvecPoint.GetXf() > m_Center.GetXf()) ? 1 : 0;
		if(vkrvecPoint.GetYf() > m_Center.GetYf())
		{
			nRetVal |= 2;
		}
		if(vkrvecPoint.GetZf() > m_Center.GetZf())
		{
			nRetVal |= 4;
		}

		return nRetVal;
	}
#else
	int SubClassifyPoint(Vec3V_In vecPoint) const
	{
//		const Vec3V vecRelativePos(vecPoint - m_CenterXYZHalfWidthW.GetXYZ());
		// NOTE: Could be doing just XY comparison here but we mask out the results from the z-component anyway so it doesn't matter.
		const VecBoolV vecOctantFlags = IsGreaterThan(vecPoint, m_CenterXYZHalfWidthW.GetXYZ());
		u32 retVal;
		ResultToIndexZYX(retVal, vecOctantFlags);
		return (int)retVal & 3;
	}
#endif

	// Init m_Center and m_fHalfWidth as they should be to be the specified child of the specified node.
	void InitExtentsAsChild(const phLooseQuadtreeNode *const kpkParentNode, const int knChildIndex);

	// Establish parent-child relationships.  This adjusts ONLY connectivity information.
	void ConnectParent(const int knParentNodeIndex);
	void ConnectChild(const int knChildIndex, const int knChildNodeIndex);

	// Disestablish parent-child relationships.  This adjusts ONLY connectivity information.
	void DisconnectParent();
	void DisconnectChild(const int knChildIndex);

	int GetVisitingObjectCount(const int knChildIndex) const
	{
		FastAssert(knChildIndex < 4);
		return((m_VisitingObjectCount >> (4 * knChildIndex)) & 15);
	}

	static int GetVisitingObjectMaxCount()
	{
		return 15;
	}

	bool RoomForAdditionalVisitingObject(const int knChildIndex) const
	{
		return GetVisitingObjectCount(knChildIndex) < GetVisitingObjectMaxCount();
	}

	void IncrementVisitingObjectCount(const int knChildIndex)
	{
		FastAssert(knChildIndex < 4);
		FastAssert(GetVisitingObjectCount(knChildIndex) < GetVisitingObjectMaxCount());
		m_VisitingObjectCount += (1 << (4 * knChildIndex));
	}

	void DecrementVisitingObjectCount(const int knChildIndex)
	{
		FastAssert(knChildIndex < 4);
		FastAssert(GetVisitingObjectCount(knChildIndex) > 0);
		m_VisitingObjectCount -= (1 << (4 * knChildIndex));
	}

	void ClearVisitingObjectCount(const int knChildIndex)
	{
		FastAssert(knChildIndex < 4);
		m_VisitingObjectCount &= ~(15 << (4 * knChildIndex));
	}

	void ClearAllVisitingObjectCounts()
	{
		m_VisitingObjectCount = 0;
	}

	// Note: There is a lot of room for packing more data into here if needed.  For example, the z component of m_CenterXYZHalfWidthW is completely unused.
	//   m_ChildNodeCount only need three bits (rather than eight) and could be packed into the upper three bits of m_ParentNodeIndex (capping the number of
	//   quadtree nodes at 8k, which is probably enough).  For a similar reason the upper two or three bits of each entry of m_ChildNodeIndices are unused
	//   as well.  It might be possible to pack m_VisitingObjectCount into the upper bits of m_ChildNodeIndices.
	u16 m_ChildNodeIndices[4];					// The indices of our 4 child nodes, yx.

	u16 m_VisitingObjectCount;					// A 'visiting object' is an object that should be in a subnode of this node,
												//   but is living in this node because there aren't enough objects to create
												//   that child node from here.  This records the number of visiting objects on
												//   a per-child node basis.
												// As these fields only have useful data when the given child node is not present,
												//   they could be reused to store other data about this child nodes when they
												//   are present.

	u16 m_ParentNodeIndex;						// The index of parent node.
	u8 m_ChildNodeCount;						// The total number of child nodes.

	u8 m_ContainedObjectCount;					// The total number of objects contained in this node.

	u16 m_ContainedObjectStartIdx;				// The level index of the first object in this node.

	Vec4V m_CenterXYZHalfWidthW;				// XY: The center point of the node in world space.  W: The 'base' width of the
												//   node.  The actual width could be up to twice this, depending on the radii of
												//   the objects contained in it and its subnodes.
};

// Contains the data pertaining to each loose octree data.  In general there are two purposes for the data in here - the data needed for
//   culling operations and the data needed for update operations (mainly to make them faster by).
class phLooseOctreeNode
{
public:
	phLooseOctreeNode();
	~phLooseOctreeNode();

	__forceinline static int GetBranchingFactor() { return 8; }

	void Clear();

	// Determine whether a point is contained within the inner bounds of this node (can be used to determine whether a given sphere 
	//   should be placed in this node).
	__forceinline bool IsPointWithinNode(Vec3V_In krvecPoint) const
	{
		// Save locals.
		Vec3V v_krvecPoint(krvecPoint);

		// In theory these tests could be replaced by ones using fabs, but for krvecPoints with
		//   components close to zero, these tests are more accurate.
		Vec3V zeroedW = v_krvecPoint;
		zeroedW.SetWZero();

		//FastAssert(m_Center.GetWf() == 0.0f); // if m_Center has non-zero w, we would need to shove that into the point's w for the compare to work

		Vec3V maxs = Vec3VFromF32(m_CenterXYZHalfWidthW.GetWf());

		Vec3V mins = Subtract(m_CenterXYZHalfWidthW.GetXYZ(), maxs);

		maxs = Add(maxs, m_CenterXYZHalfWidthW.GetXYZ());

		return ( IsGreaterThanAll(zeroedW, mins) & IsLessThanOrEqualAll(zeroedW, maxs) ) != 0;
	}


	// Determine whether a sphere intersects the outer bounds of this node.  You should use this when you think that the sphere is in the general
	//   vicinity of the node (it skips the quicker reject tests).
	__forceinline bool DoesSphereIntersect(Vec::V3Param128 vkrvecCenter, const float& kfRadius) const
	{
		// Save locals.
		Vec3V v_vkrvecCenter(vkrvecCenter);
		const ScalarV v_kfRadius = ScalarVFromF32(kfRadius);
		const ScalarV v_kfRealHalfWidth = Add(m_CenterXYZHalfWidthW.GetW(), m_CenterXYZHalfWidthW.GetW());
		const ScalarV v_realHalfWidthPlusRadis = Add( v_kfRealHalfWidth, v_kfRadius );
		const Vec3V v_absOfVecCenterMinusCenter = Abs( Subtract(v_vkrvecCenter, m_CenterXYZHalfWidthW.GetXYZ()) );
 

/*		if(kfRadius > kfRealHalfWidth)
		{
			Vector3 vecTemp;
			vecTemp.x = Clamp(krvecCenter.x - m_Center.x, -kfRealHalfWidth, kfRealHalfWidth);
			vecTemp.y = Clamp(krvecCenter.y - m_Center.y, -kfRealHalfWidth, kfRealHalfWidth);
			vecTemp.z = Clamp(krvecCenter.z - m_Center.z, -kfRealHalfWidth, kfRealHalfWidth);
			vecTemp.Add(m_Center);

			return(vecTemp.Dist2(krvecCenter) < square(kfRadius));
		}
else*/

		//{
		//	bool bRetVal = (fabs(krvecCenter.x - m_Center.x) < kfRealHalfWidth + kfRadius) && (fabs(krvecCenter.y - m_Center.y) < kfRealHalfWidth + kfRadius) && (fabs(krvecCenter.z - m_Center.z) < kfRealHalfWidth + kfRadius);
		//	return bRetVal;
		//}

		return IsLessThanAll( v_absOfVecCenterMinusCenter, Vec3V(v_realHalfWidthPlusRadis) ) != 0;
	}


	// Determine whether a sphere intersects the outer bounds of this node.
#if !PHLEVEL_VECTORIZATION_TEST
	__forceinline bool DoesSphereIntersectSpecial(Vector3::Vector3Param krvecCenter, const float kfRadius) const
	{
		const float kfRealHalfWidth = m_HalfWidth + m_HalfWidth;

		Vector3 vecNodeToSphere(krvecCenter);
		vecNodeToSphere.Subtract(RCC_VECTOR3(m_Center));

		float fNodeToSphereSq = vecNodeToSphere.Mag2();
		if(fNodeToSphereSq >= square(1.7320508f * kfRealHalfWidth + kfRadius))
		{
			return false;
		}

		vecNodeToSphere.Abs();

		Vector3 halfWidth;
		halfWidth.Set(kfRealHalfWidth + kfRadius);
		return vecNodeToSphere.IsLessThan(halfWidth);
	}
#else
	__forceinline bool DoesSphereIntersectSpecial(Vec3V_In krvecCenter, ScalarV_In krvecRadius) const
	{
		////////////////////////////////////////////////////////////////////////////////
		// Check for intersection between the bounding sphere of the node and the sphere provided.

		// Get a vector from the center of this node to the center of the sphere.
		Vector3 vecNodeToSphere = RCC_VECTOR3(krvecCenter);
		vecNodeToSphere.Subtract(VEC3V_TO_VECTOR3(m_CenterXYZHalfWidthW.GetXYZ()));

		// Get the squared magnitude of that vector.
		const Vector3 vecNodeToSphereSq = vecNodeToSphere.Mag2V();

		const Vector3 vecRealHalfWidth = SCALARV_TO_VECTOR3(Add(m_CenterXYZHalfWidthW.GetW(), m_CenterXYZHalfWidthW.GetW()));

		// Create a vector splatted with square(1.7320508f * (m_HalfWidth + m_HalfWidth) + krvecRadius.x).  This value is the radius of the bounding
		//   sphere of this node plus the radius of the sphere against which we're testing for intersection.
		Vector3 vecTemp;
		vecTemp.AddScaled(RCC_VECTOR3(krvecRadius), vecRealHalfWidth, VEC3_SQRTTHREE);
		vecTemp.Multiply(vecTemp);
		bool intersect = vecNodeToSphereSq.IsLessThanDoNotUse(vecTemp);
		if(intersect)
		{
	        ////////////////////////////////////////////////////////////////////////////////
	        // Check for intersection of the node's AABB with the AABB of the sphere provided.
	        vecNodeToSphere.Abs();

	        Vector3 halfWidth;
	        halfWidth.Add(vecRealHalfWidth, RCC_VECTOR3(krvecRadius));
	        intersect = vecNodeToSphere.IsLessThanAll(halfWidth);
		}

		return intersect;
		//
		////////////////////////////////////////////////////////////////////////////////
	}
#endif

#if __ASSERT
	__forceinline u8 GetPointLocationMask(Vector3::Vector3Param vkrvecPoint, const float kfMaxRealHalfWidth) const
	{
		Vector3 krvecPoint(vkrvecPoint);
		Vec3V vCenter = m_CenterXYZHalfWidthW.GetXYZ();
		u8 uRetVal = 0;
		if(krvecPoint.x <= vCenter.GetXf() - kfMaxRealHalfWidth) uRetVal = 1;
		else if(krvecPoint.x >= vCenter.GetXf() + kfMaxRealHalfWidth) uRetVal = 2;
		if(krvecPoint.y <= vCenter.GetYf() - kfMaxRealHalfWidth) uRetVal |= 4;
		else if(krvecPoint.y >= vCenter.GetYf() + kfMaxRealHalfWidth) uRetVal |= 8;
		if(krvecPoint.z <= vCenter.GetZf() - kfMaxRealHalfWidth) uRetVal |= 16;
		else if(krvecPoint.z >= vCenter.GetZf() + kfMaxRealHalfWidth) uRetVal |= 32;

		return uRetVal;
	}


	__forceinline bool DoesLineSegmentIntersect2(Vector3::Vector3Param krvecP0, Vector3::Vector3Param krvecP1) const
	{
		const float kfMaxRealHalfWidth = m_CenterXYZHalfWidthW.GetWf() + m_CenterXYZHalfWidthW.GetWf();

		u8 uPointMask0 = GetPointLocationMask(krvecP0, kfMaxRealHalfWidth);
		if(uPointMask0 == 0)
		{
			// This point is already inside of the cube.
			return true;
		}
		u8 uPointMask1 = GetPointLocationMask(krvecP1, kfMaxRealHalfWidth);
		if(uPointMask1 == 0)
		{
			// This point is already inside of the cube.
			return true;
		}
		Vector3 vecTempP0(krvecP0);
		Vector3 vecTempP1(krvecP1);
		Vector3 vecTempMid;
		while(true)
		{
			// Neither of the two end points of our segment are inside of the cube, and they're not both outside for any of the same reasons, so
			//   let's compute the midpoint and replace one of the end points with it.
			vecTempMid.Average(vecTempP0, vecTempP1);
			u8 uPointMaskMid = GetPointLocationMask(vecTempMid, kfMaxRealHalfWidth);

			if(uPointMaskMid == 0)
			{
				// This point is inside of the cube.
				return true;
			}
			if((uPointMaskMid & uPointMask0) != 0)
			{
				if((uPointMaskMid & uPointMask1) != 0)
				{
					// 
					return false;
				}
				uPointMask0 = uPointMaskMid;
				vecTempP0.Set(vecTempMid);
			}
			else
			{
				uPointMask1 = uPointMaskMid;
				vecTempP1.Set(vecTempMid);
			}
		}
	}
#endif // __ASSERT

	// CMT - 1/27/11 - Refactored this function to use the geomBoxes::TestSegmentToCenteredAlignedBox() helper function.
	// This makes the following test code a lot shorter/more compact and reduces duplicate code. It also seems to produce 
	// a slight performance increase during probe tests, as it appears that geomBoxes::TestSegmentToCenteredAlignedBox() 
	// is faster than the previous code for this function.
	__forceinline bool DoesLineSegmentIntersect(Vec3V_In vkrvecP0, Vec3V_In vkrvecP1) const
	{
		// Find the center and half-widths of the box.
		const ScalarV v_kfMaxRealHalfWidth = Add(m_CenterXYZHalfWidthW.GetW(), m_CenterXYZHalfWidthW.GetW());

		Vec3V boxCenter = m_CenterXYZHalfWidthW.GetXYZ();
		Vec3V boxHalfSize = VECTOR3_TO_VEC3V(SCALARV_TO_VECTOR3(v_kfMaxRealHalfWidth));

		// Find the segment starting point and end point relative to the box center.
		Vec3V relativeP0 = Subtract(INTRIN_TO_VEC3V(vkrvecP0), boxCenter);
		Vec3V relativeP1 = Subtract(INTRIN_TO_VEC3V(vkrvecP1), boxCenter);

		VecBoolV result = geomBoxes::TestSegmentToCenteredAlignedBox(relativeP0, relativeP1,boxHalfSize) ;
		return IsEqualIntAll( result, VecBoolV(V_T_T_T_T) ) != 0;
	}

	__forceinline bool DoesXZCircleIntersect(Vec3V_In vkrvecCenter, const float kfRadius) const
	{
		Vector3 krvecCenter = RCC_VECTOR3(vkrvecCenter);
		const float kfRealHalfWidth = m_CenterXYZHalfWidthW.GetWf() + m_CenterXYZHalfWidthW.GetWf();
		bool bRetVal = true;
		if(VEC3V_TO_VECTOR3(m_CenterXYZHalfWidthW.GetXYZ()).XZDist2(krvecCenter) >= square(1.4142136f * kfRealHalfWidth + kfRadius))
		{
			bRetVal = false;
		}

		if (bRetVal == true)
		{
		    bRetVal = (fabs(krvecCenter.x - VEC3V_TO_VECTOR3(m_CenterXYZHalfWidthW.GetXYZ()).x) < kfRealHalfWidth + kfRadius) && (fabs(krvecCenter.z - VEC3V_TO_VECTOR3(m_CenterXYZHalfWidthW.GetXYZ()).z) < kfRealHalfWidth + kfRadius);
		}

		return bRetVal;
	}


	__forceinline bool DoesCapsuleIntersect(Vec3V_In krvecP0, Vec3V_In krvecShaftAxis, ScalarV_In kvShaftLength, ScalarV_In kvCapsuleRadius) const
	{
		const ScalarV kvRealHalfWidth = Add(m_CenterXYZHalfWidthW.GetW(), m_CenterXYZHalfWidthW.GetW());
		Vec3V vecP1 = krvecP0 + krvecShaftAxis * kvShaftLength;
		//return geomBoxes::TestCapsuleToAlignedBoxFP(krvecP0.GetIntrin128(), vecP1.GetIntrin128(), kvCapsuleRadius.GetIntrin128(), (m_CenterXYZHalfWidthW.GetXYZ() - Vec3V(kvRealHalfWidth)).GetIntrin128(), (m_CenterXYZHalfWidthW.GetXYZ() + Vec3V(kvRealHalfWidth)).GetIntrin128());
		return COT_TestCapsuleToAlignedBoxFP(krvecP0, vecP1, kvCapsuleRadius, (m_CenterXYZHalfWidthW.GetXYZ() - Vec3V(kvRealHalfWidth)), (m_CenterXYZHalfWidthW.GetXYZ() + Vec3V(kvRealHalfWidth)));
	}


	__forceinline bool DoesBoxIntersect(Mat34V_In boxAxes, Vec3V_In boxHalfSize) const
	{
		// TODO: Looks like some bad vector-float stuff here too.
		const float nodeHalfWidth = m_CenterXYZHalfWidthW.GetWf() + m_CenterXYZHalfWidthW.GetWf();
		Vector3 nodeHalfSize(nodeHalfWidth,nodeHalfWidth,nodeHalfWidth);
		nodeHalfSize.w = 0.0f;
		Matrix34 relBoxAxes = RCC_MATRIX34(boxAxes);
		relBoxAxes.d.Subtract(VEC3V_TO_VECTOR3(m_CenterXYZHalfWidthW.GetXYZ()));
		Vector3 boxHalfSizeZeroW = RCC_VECTOR3(boxHalfSize);
		boxHalfSizeZeroW.And(VEC3_ANDW);
		//return geomBoxes::TestBoxToBoxOBB (boxHalfSizeZeroW,nodeHalfSize,relBoxAxes);
		return COT_TestBoxToBoxOBB(RCC_VEC3V(boxHalfSizeZeroW),RCC_VEC3V(nodeHalfSize),RCC_MAT34V(relBoxAxes));
	}

	__forceinline bool DoesAABBIntersect(Vec3V_In boxCenter, Vec3V_In boxHalfSize) const
	{
		Vec3V nodeCenter = m_CenterXYZHalfWidthW.GetXYZ();
		Vec3V nodeHalfSize = Vec3V(m_CenterXYZHalfWidthW.GetW());
		nodeHalfSize = Add(nodeHalfSize,nodeHalfSize);
		//return geomBoxes::TestAABBtoAABB_CenterHalfSize(boxCenter,boxHalfSize,nodeCenter,nodeHalfSize).Getb();
		return COT_TestAABBtoAABB_CenterHalfSize(boxCenter,boxHalfSize,nodeCenter,nodeHalfSize);
	}

	// Find out in which of the children [0..7] this point is located.
#if 0
	// Vec passed by const ref in case it's already in memory... since we don't want the values in a vector register within this function at all.
	int SubClassifyPoint(Vec3V_ConstRef vkrvecPoint) const
	{
		int nRetVal = (vkrvecPoint.GetXf() > m_Center.GetXf()) ? 1 : 0;
		if(vkrvecPoint.GetYf() > m_Center.GetYf())
		{
			nRetVal |= 2;
		}
		if(vkrvecPoint.GetZf() > m_Center.GetZf())
		{
			nRetVal |= 4;
		}

		return nRetVal;
	}
#else
	int SubClassifyPoint(Vec3V_In vecPoint) const
	{
//		const Vec3V vecRelativePos(vecPoint - m_CenterXYZHalfWidthW.GetXYZ());
		const VecBoolV vecOctantFlags = IsGreaterThan(vecPoint, m_CenterXYZHalfWidthW.GetXYZ());
		u32 retVal;
		ResultToIndexZYX(retVal, vecOctantFlags);
		return (int)retVal;
	}
#endif

	// Init m_Center and m_fHalfWidth as they should be to be the specified child of the specified node.
	void InitExtentsAsChild(const phLooseOctreeNode *const kpkParentNode, const int knChildIndex);

	// Establish parent-child relationships.  This adjusts ONLY connectivity information.
	void ConnectParent(const int knParentNodeIndex);
	void ConnectChild(const int knChildIndex, const int knChildNodeIndex);

	// Disestablish parent-child relationships.  This adjusts ONLY connectivity information.
	void DisconnectParent();
	void DisconnectChild(const int knChildIndex);

	int GetVisitingObjectCount(const int knChildIndex) const
	{
		return((m_VisitingObjectCount >> (4 * knChildIndex)) & 15);
	}

	static int GetVisitingObjectMaxCount()
	{
		return 15;
	}

	bool RoomForAdditionalVisitingObject(const int knChildIndex) const
	{
		return GetVisitingObjectCount(knChildIndex) < GetVisitingObjectMaxCount();
	}

	void IncrementVisitingObjectCount(const int knChildIndex)
	{
		FastAssert(GetVisitingObjectCount(knChildIndex) < GetVisitingObjectMaxCount());
		m_VisitingObjectCount += (1 << (4 * knChildIndex));
	}

	void DecrementVisitingObjectCount(const int knChildIndex)
	{
		FastAssert(GetVisitingObjectCount(knChildIndex) > 0);
		m_VisitingObjectCount -= (1 << (4 * knChildIndex));
	}

	void ClearVisitingObjectCount(const int knChildIndex)
	{
		m_VisitingObjectCount &= ~(15 << (4 * knChildIndex));
	}

	void ClearAllVisitingObjectCounts()
	{
		m_VisitingObjectCount = 0;
	}

	u16 m_ChildNodeIndices[8];					// The indices of our 8 child nodes, zyx.

	u32 m_VisitingObjectCount;					// A 'visiting object' is an object that should be in a subnode of this node,
												//   but is living in this node because there aren't enough objects to create
												//   that child node from here.  This records the number of visiting objects on
												//   a per-child node basis.
												// As these fields only have useful data when the given child node is not present,
												//   they could be reused to store other data about this child nodes when they
												//   are present.

	u16 m_ParentNodeIndex;						// The index of parent node.
	u8 m_ChildNodeCount;						// The total number of child nodes.

	u8 m_ContainedObjectCount;					// The total number of objects contained in this node.

	u16 m_ContainedObjectStartIdx;				// The level index of the first object in this node.

	u8 m_Pad[6];

	Vec4V m_CenterXYZHalfWidthW;				// XYZ: The center point of the node in world space.  W: The 'base' width of the
												//   node.  The actual width could be up to twice this, depending on the radii of
												//   the objects contained in it and its subnodes.
};


#if __SPU
// phSpuObjectBuffer is a class for handling DMAs to and from the PPU.  It provides convenience and also makes it possible to communicate with called functions
//   regarding locally-cached objects.
template <class __ObjectType> class phSpuObjectBuffer
{
public:
	// Get the object from the PPU.  Blocking.
	__ObjectType *FetchObject(__ObjectType *ppuObject)
	{
		sysDmaLargeGet(&m_ObjectBuffer[0], (uint64_t)ppuObject, sizeof(__ObjectType), DMA_TAG(1));
		__ObjectType *spuObject = reinterpret_cast<__ObjectType *>(&m_ObjectBuffer[0]);
		m_PpuObject = ppuObject;
		sysDmaWaitTagStatusAll(DMA_MASK(1));
		return spuObject;
	}

	// Sends the object back to the PPU.  Blocking.
	void ReturnObject() const
	{
		sysDmaLargePut(&m_ObjectBuffer[0], (uint64_t)m_PpuObject, sizeof(__ObjectType), DMA_TAG(1));
		sysDmaWaitTagStatusAll(DMA_MASK(1));
	}

	__ObjectType *GetSpuObject()
	{
		__ObjectType *spuObject = reinterpret_cast<__ObjectType *>(&m_ObjectBuffer[0]);
		return spuObject;
	}

	const __ObjectType *GetSpuObject() const
	{
		const __ObjectType *spuObject = reinterpret_cast<const __ObjectType *>(&m_ObjectBuffer[0]);
		return spuObject;
	}

private:
	u8 m_ObjectBuffer[sizeof(__ObjectType)] ;
	__ObjectType *m_PpuObject;
};
#endif	// __SPU

class phObjectData
{
public:
	// PURPOSE:	The bounding sphere of this object, as a 4D vector where XYZ is the center and W the radius.
	Vec4V	m_CachedCenterAndRadius;

	// PURPOSE: Per-instance type and include flags.  These can be different from what would be obtained if one looked at the instance's archetype.
	u32					m_CachedArchIncludeFlags;
	u32					m_CachedArchTypeFlags;

	phLevelBase::eObjectState GetState() const
	{
		return (phLevelBase::eObjectState)(m_uPackedInstAndState & 3);
	}

	void SetState(phLevelBase::eObjectState keObjectState)
	{
		FastAssert((u16)(keObjectState) < 4);
		m_uPackedInstAndState = (size_t)(keObjectState) | (m_uPackedInstAndState & ~3);
		FastAssert(GetState() == keObjectState);
	}

	phInst *GetInstance() const
	{
		return (phInst *)(m_uPackedInstAndState & ~15);
	}

	void SetInstance(const phInst *kpInstance)
	{
		FastAssert(((size_t)(kpInstance) & 15) == 0);
		m_uPackedInstAndState = (size_t)(kpInstance) | (m_uPackedInstAndState & 15);
		FastAssert(GetInstance() == kpInstance);
	}

	Vec4V_Out GetCenterAndRadius () const
	{
		return m_CachedCenterAndRadius;
	}

	int GetOctreeNodeIndex() const
	{
		return(m_uOctreeNodeIndex & 8191);
	}

	void SetOctreeNodeIndex(const u16 kuOctreeNodeIndex)
	{
		FastAssert(kuOctreeNodeIndex < 8192);
		m_uOctreeNodeIndex = kuOctreeNodeIndex | (m_uOctreeNodeIndex & 57344);
		FastAssert(GetOctreeNodeIndex() == kuOctreeNodeIndex);
	}

	int GetVisitingObjectChildIndex() const
	{
		return(m_uOctreeNodeIndex >> 13);
	}

	void SetVisitingObjectChildIndex(const int knVisitingObjectChildIndex)
	{
		FastAssert(knVisitingObjectChildIndex >= 0);
		FastAssert(knVisitingObjectChildIndex < 8);
		m_uOctreeNodeIndex = (u16)(knVisitingObjectChildIndex << 13) | (m_uOctreeNodeIndex & 8191);
		FastAssert(GetVisitingObjectChildIndex() == knVisitingObjectChildIndex);
	}

	void SetCollidesWithInactive()
	{
		m_uPackedInstAndState = (phLevelBase::COLLISIONSTATE_VS_INACTIVE_BIT) | m_uPackedInstAndState;
		FastAssert(GetCollidesWithInactive());
	}

	void ClearCollidesWithInactive()
	{
		m_uPackedInstAndState = ~(phLevelBase::COLLISIONSTATE_VS_INACTIVE_BIT) & m_uPackedInstAndState;
		FastAssert(!GetCollidesWithInactive());
	}

	bool GetCollidesWithInactive()
	{
		return (m_uPackedInstAndState & (phLevelBase::COLLISIONSTATE_VS_INACTIVE_BIT)) != 0;
	}

	u32 GetCollidesWithInactiveU32()
	{
		return (m_uPackedInstAndState & (phLevelBase::COLLISIONSTATE_VS_INACTIVE_BIT));
	}

	void SetCollidesWithFixed()
	{
		m_uPackedInstAndState = (phLevelBase::COLLISIONSTATE_VS_FIXED_BIT) | m_uPackedInstAndState;
		FastAssert(GetCollidesWithFixed());
	}

	void ClearCollidesWithFixed()
	{
		m_uPackedInstAndState = ~(phLevelBase::COLLISIONSTATE_VS_FIXED_BIT) & m_uPackedInstAndState;
		FastAssert(!GetCollidesWithFixed());
	}

	bool GetCollidesWithFixed()
	{
		return (m_uPackedInstAndState & (phLevelBase::COLLISIONSTATE_VS_FIXED_BIT)) != 0;
	}

	u32 GetCollidesWithFixedU32()
	{
		return (m_uPackedInstAndState & (phLevelBase::COLLISIONSTATE_VS_FIXED_BIT));
	}

	// PURPOSE:	The level index of the next object in this object's node
	u16 m_uNextObjectInNodeIndex;

private:
	// PURPOSE:	The index number of the physics level's octree node that this object is in
	// NOTES:	The lower 13 bits are the index of the node in which this object resides.
	//			The upper 3 bits are the index of the child from which this object is a visiting object, if it is a visiting object.
	//			Note that octree node 8191 is reserved to indicate that an object has a level index but is spatially no longer within
	//			the bounds of the physics level (and does not correspond to an actual octree node).
	u16 m_uOctreeNodeIndex;

	// PURPOSE:	The instance pointer and object state (fixed, inactive or active) for this object
	// NOTES:	Since phInsts must be 16-byte aligned (due to the matrix that they have) the lower bits are used here to store the object state.
	size_t m_uPackedInstAndState;
};



////////////////////////////////////////////////////////////////
// phLevelNodeTree
//
// This is a physics level that employs a loose tree to speed up certain spatial operations.
// <FLAG Component>
template <class __NodeType> class phLevelNodeTree : public phLevelBase
{

public:
	friend class phMouseInput;
	friend class phSimulator;

	////////////////////////////////////////////////////////////
	phLevelNodeTree ();													// constructor
	~phLevelNodeTree ();											// destructor

	enum BroadPhaseType { AXISSWEEP3, AXISSWEEP1, LEVEL, SPATIALHASH, NXN, Broadphase_Count }; //, OCTREE };

	static const int knMaxOctreeDepth = 18;

	////////////////////////////////////////////////////////////
	// Initializing.

    // PURPOSE
    //    Prepare the physics level for use.
    // NOTES
    //    Must be called before the level is used, after configuration.
    // SEE ALSO
    //    SetMaxActive
	void Init();													// init structures: non-default configuration needs to be set first

	// init the broadphase
	void InitBroadphase( btOverlappingPairCache *existingPairCache = NULL, bool transferHandles = false );

    // PURPOSE
    //    Inform the physics level that it will no longer be used.
	void Shutdown();												// called before blowing away heap

	// PURPOSE
	//   Set the level back to the state it was in when it was first initialized (aka, make there be no objects).
	void Clear();

	////////////////////////////////////////////////////////////
	// Broadphase
	phBroadPhase * GetBroadPhase()											{ return m_BroadPhase; }

	////////////////////////////////////////////////////////////
	// Singleton.
#if !__SPU
	static phLevelNodeTree<__NodeType>* GetActiveInstance ()						{ return sm_ActiveInstance; }
	static void SetActiveInstance (phLevelNodeTree<__NodeType> * instance)			{ sm_ActiveInstance = instance; }
	void SetActiveInstance ()	{ sm_ActiveInstance = this; }
#endif // !__SPU

	// PURPOSE
	//	Allows application to describe a phInst more informatively than the physics level can
	// PARAMS
	//	rInst - Reference to the instance being queried
	//	pBuffer - Pointer to memory to write description into, assumed simple text string NULL terminated is returned
	//	iBufferSize - Size of buffer
	// RETURNS
	//	Pointer to pBuffer
	PH_NON_SPU_VIRTUAL const char* GetInstDescription(const phInst& rInst, char *pBuffer, int iBufferSize) const;


	////////////////////////////////////////////////////////////
	// Configuring.
	void SetMaxObjects(const int knMaxObjectCount);
	int GetMaxObjects() const { return m_MaxObjects; };

	// call this before Init()
	void SetBroadPhaseType( BroadPhaseType bpt ){ m_BroadPhaseType = bpt; }

	// PURPOSE: Set the total size of the physics level (a 3D cube).
	// PARAMS:
	//	levelMin	- the minimum coordinate location in the physics level
	//	levelMax	- the maximum coordinate location in the physics level
	// NOTES:
	//	1.	The physics level extents are a cube, so the greatest extent from the given vectors are used for all three dimensions.
	//	2.	Collision detection and collision response generally work well for coordinates up to 200km (PC and Xenon).
	//		If any physics level around that size or larger is needed, then the physics simulator should be changed to handle
	//		local coordinate systems, so that objects far from the origin can still be simulated accurately.
	void SetExtents (Vec3V_In levelMin, Vec3V_In levelMax);

	// PURPOSE: Set the total size of the physics level (a 3D cube).
	// PARAMS:
	//	krvecLevelMin	- the minimum coordinate location in the physics level
	//	krvecLevelMax	- the maximum coordinate location in the physics level
	// NOTES:
	//	1.	The physics level extents are a cube, so the greatest extent from the given vectors are used for all three dimensions.
	//	2.	Collision detection and collision response generally work well for coordinates up to 200km (PC and Xenon).
	//		If any physics level around that size or larger is needed, then the physics simulator should be changed to handle
	//		local coordinate systems, so that objects far from the origin can still be simulated accurately.
	void SetExtents (const Vector3& levelCenter, float levelHalfWidth);

	// Sets the number of octree nodes that will allocated in the pool and available for use by the octree level.
	// Note that, due to internal storage limitations, you cannot have more than 8191 octree nodes. (only the lower 13 bits of
	//   phObjectData::m_uOctreeNodeIndex are available for recording the octree index, and 8191 is a reserved value)
	void SetNumOctreeNodes(const int knOctreeNodeCount);

	// Sets a few parameters that determine the conditions under which the level will create a new octree node and remove an octree node.
	// Specifically, the first parameter specifies how many objects we have to have waiting to go into an octree node before we will create
	//   and new one (and push the appropriate objects down into that node_ and the second parameter specifies how empty a leaf node has to
	//   get before we will remove it (and pull the objects that were in it up the octree).
	// Note that the first parameter has to be greater than the second or else we'd be ready to remove an octree node immediately after
	//   creating it  =).
	void SetOctreeNodeCreationParameters(const int kMinObjectsInNodeToCreateChild, const int kMaxObjectsInNodeToCreateChild)
	{
		FastAssert(kMinObjectsInNodeToCreateChild > kMaxObjectsInNodeToCreateChild);
		Assertf(kMinObjectsInNodeToCreateChild <= __NodeType::GetVisitingObjectMaxCount() + 1, "Can't have more visiting objects than the max. kMinObjectsInNodeToCreateChild: %i, Max Visiting objects: %i", kMinObjectsInNodeToCreateChild, __NodeType::GetVisitingObjectMaxCount());
		Assertf(kMaxObjectsInNodeToCreateChild <= __NodeType::GetVisitingObjectMaxCount(), "Cannot collapse a node with more objects than the max number of visiting objects in its parent. kMaxObjectsInNodeToCreateChild: %i, Max Visiting objects: %i", kMaxObjectsInNodeToCreateChild, __NodeType::GetVisitingObjectMaxCount());
		m_nMinObjectsInNodeToCreateChild = Min(kMinObjectsInNodeToCreateChild, __NodeType::GetVisitingObjectMaxCount() + 1);
		m_nMaxObjectsInNodeToCollapseChild = Min(kMaxObjectsInNodeToCreateChild, __NodeType::GetVisitingObjectMaxCount());
	}

    void SetMaxCollisionPairs(int maxCollisionPairs)
    {
        m_MaxCollisionPairs = maxCollisionPairs;
    }
    int GetMaxCollisionPairs()
    {
        return m_MaxCollisionPairs;
    }

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	void SetMaxDeferredInstanceCount(int maxDeferredInstanceCount);
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

	// PURPOSE: Allows one to set a callback delegate which will be called every time an object leaves the level.
	// PARAMS:
	//	callback	- an InstOutOfWorldCallback which will be called an object leaves the world.
	typedef atDelegate<void (phInst* instOutOfWorld)> InstOutOfWorldCallback;

	void SetNotifyOutOfWorldCallback(InstOutOfWorldCallback callback);

	// PURPOSE
	//    Default implementation of Notify out of world which calls phInst::NotifyOutOfWorld()
	static void DefaultNotifyOutOfWorld(phInst* inst);

	// PURPOSE: The maximum number of phInst last matrices that can currently be allocated.
#if (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	enum { MAX_INST_LAST_MATRICES = 264 };
#else
	enum { MAX_INST_LAST_MATRICES = 0xFE };
#endif

	// PURPOSE: Set maximum number of last matrices that can be used.
	void SetMaxInstLastMatrices(int maxLastInstMatrices);

	////////////////////////////////////////////////////////////
	// Accessing data.

	// PURPOSE
	//    Convenience function to determine if an object can potentially become active.
	// RETURNS
	//    Returns true if the object is not fixed, and the phInst does not have the phInst::FLAG_NEVER_ACTIVATE.
	// NOTES
	//    There is no guarantee the phInst will respect the FLAG_NEVER_ACTIVATE, it is possible for an app to disregard it.
	bool CanBecomeActive (int levelIndex) const;

    // PURPOSE
    //    Get the instance that corresponds to a particular level index
    // PARAMS
    //    levelIndex - The level index of the possibly valid instance
    // RETURNS
    //    The instance, or NULL if that level index is not an added object
    // NOTES
    //    If the level index does not correspond to an added object (i.e. !LegitLevelIndex),
    //    the return value is NULL.
	phInst* GetInstance (int levelIndex) const;
#if __DEV || __BANK
	phInst* GetInstanceSafe (int levelIndex) const;
#endif

	// PURPOSE
	//    Determine whether an object is active, inactive, fixed, or nonexistent.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// RETURNS
	//    The eObjectState of the instance
	// SEE ALSO
	//    SetState
	eObjectState GetState(int levelIndex) const;

	// PURPOSE
	//	Get the center and radius of the smallest surrounding sphere for the object with the given level index
	// RETURN
	//	The center (x,y,z) and radius (w) for the given object
	// NOTES
	//	This is used to avoid reporting a new center and radius on every frame for objects that change size
	Vec4V_Out GetCenterAndRadius (int levelIndex) const;

	// PURPOSE
	//   Set entire sets of type/include flags.
	void SetInstanceTypeAndIncludeFlags(int levelIndex, u32 typeFlags, u32 includeFlags);
	void SetInstanceTypeFlags(int levelIndex, u32 typeFlags);
	void SetInstanceIncludeFlags(int levelIndex, u32 includeFlags);

	// PURPOSE
	//    Add/Clear specific sets of type/include flags.
	// NOTES
	//    More than one flag can be set/cleared at a time (and it's better to do them all at once rather that one at a time).
	void AddInstanceTypeFlags(int levelIndex, u32 flagsToAdd);
	void AddInstanceIncludeFlags(int levelIndex, u32 flagsToAdd);
	void ClearInstanceTypeFlags(int levelIndex, u32 flagsToClear);
	void ClearInstanceIncludeFlags(int levelIndex, u32 flagsToClear);

	// PURPOSE
	//    Get entire sets of type/include flags.
	u32 GetInstanceTypeFlags(int levelIndex) const;
	u32 GetInstanceIncludeFlags(int levelIndex) const;

	// PURPOSE
	//    Check to see if specific type/include flag(s) are set.
	// NOTES
	//    More than one flag can be tested at a time - you'll get a true result if any of them are set (it's better to test for them all at once than
	//    to test for them individually and OR the results yourself).
	bool GetInstanceTypeFlag(int levelIndex, u32 flagToCheck) const;
	bool GetInstanceIncludeFlag(int levelIndex, u32 flagToCheck) const;

	static bool MatchFlags(u32 typeFlags0, u32 includeFlags0, u32 typeFlags1, u32 includeFlags1);
	bool MatchFlags(int levelIndex0, int levelIndex1) const;

	// PURPOSE
	//    Determine whether an inactive object would like collisions against other inactive objects.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    1.  Normally, inactive objects do not collide with other inactive objects. When they are under
	//    animation (kinematic) control, then sometimes the user will need collisions generated against
	//    other inactive objects.
	//    2.  The user usually sets this flag by calling SetMovingInstance in the simulator once per frame,
	//    which is the only way to control this type of collision of SWEEP_AND_PRUME == 0
	// SEE ALSO
	//    SetInactiveCollidesAgainstInactive, GetInactiveCollidesAgainstFixed, SetInactiveCollidesAgainstFixed
	bool GetInactiveCollidesAgainstInactive(const int levelIndex) const;
	u32 GetInactiveCollidesAgainstInactiveU32(const int levelIndex) const;

	// PURPOSE
	//    Determine whether an inactive object would like collisions against other inactive objects.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    1.  Normally, inactive objects do not collide with other inactive objects. When they are under
	//    animation (kinematic) control, then sometimes the user will need collisions generated against
	//    other inactive objects.
	//    2.  The user usually sets this flag by calling SetMovingInstance in the simulator once per frame,
	//    which is the only way to control this type of collision of SWEEP_AND_PRUME == 0
	// SEE ALSO
	//    GetInactiveCollidesAgainstInactive, GetInactiveCollidesAgainstFixed, SetInactiveCollidesAgainstFixed
	void SetInactiveCollidesAgainstInactive(const int levelIndex, bool collides);

	// PURPOSE
	//    Determine whether an inactive object would like collisions against fixed objects.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    1.  Normally, inactive objects do not collide with fixed objects. When they are under
	//    animation (kinematic) control, then sometimes the user will need collisions generated against
	//    fixed objects.
	//    2.  The user usually sets this flag by calling SetActiveInstance in the simulator once per frame,
	//    which is the only way to control this type of collision of SWEEP_AND_PRUME == 0
	// SEE ALSO
	//    SetInactiveCollidesAgainstFixed, GetInactiveCollidesAgainstInactive, SetInactiveCollidesAgainstInactive
	bool GetInactiveCollidesAgainstFixed(const int levelIndex) const;
	u32 GetInactiveCollidesAgainstFixedU32(const int levelIndex) const;

	// PURPOSE
	//    Control whether an inactive object would like collisions against fixed objects.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    1.  Normally, inactive objects do not collide with fixed objects. When they are under
	//    animation (kinematic) control, then sometimes the user will need collisions generated against
	//    fixed objects.
	//    2.  The user usually sets this flag by calling SetActiveInstance in the simulator once per frame,
	//    which is the only way to control this type of collision of SWEEP_AND_PRUME == 0
	// SEE ALSO
	//    SetInactiveCollidesAgainstFixed, GetInactiveCollidesAgainstInactive, SetInactiveCollidesAgainstInactive
	void SetInactiveCollidesAgainstFixed(const int levelIndex, bool collides);

#if LEVELNEW_GENERATION_IDS
	// PURPOSE
	//    Get the current generation ID for a given level index.
	// NOTES
	//    If you're looking to determine whether a given level index + generation ID combo that you've been given, make sure that you call !IsNonExistent() to
	//    first check if that level index is even still in use.  If you don't, this will assert on __ASSERT builds and you'll get false positives otherwise.
	//    Alternatively, just use IsLevelIndexGenerationIDCurrent() below.
	u16 GetGenerationID(const int levelIndex) const
	{
		FastAssert(LegitLevelIndex(levelIndex));
		return m_pauGenerationIDs[levelIndex];
	}

	bool IsLevelIndexGenerationIDCurrent(const int levelIndex, const int generationID) const
	{
		FastAssert(LegitLevelIndex(levelIndex));
		return GetGenerationID(levelIndex) == generationID;
	}

#if __SPU
	u16 &GetGenerationIDRef(int levelIndex) const
	{
		FastAssert(LegitLevelIndex(levelIndex));
		return m_pauGenerationIDs[levelIndex];
	}
#endif

	phHandle GetHandle(int levelIndex)
	{
		FastAssert(LegitLevelIndex(levelIndex));
		FastAssert(!IsNonexistent(levelIndex));
		return phHandle(u16(levelIndex), GetGenerationID(levelIndex));
	}

	phHandle GetHandle(phInst* instance)
	{
		return GetHandle(instance->GetLevelIndex());
	}

	__forceinline phInst* GetInstance(int levelIndex, int generationId);
	__forceinline phInst* GetInstance(phHandle handle);
#endif

	phSleepIsland::Id GetSleepIsland(const int levelIndex) const
	{
		FastAssert(LegitLevelIndex(levelIndex));
		FastAssert(!IsNonexistent(levelIndex));
		return m_pauSleepIslands[levelIndex];
	}

	void SetSleepIsland(const int levelIndex, const phSleepIsland::Id sleepIsland)
	{
		FastAssert(LegitLevelIndex(levelIndex));
		FastAssert(!IsNonexistent(levelIndex));
		m_pauSleepIslands[levelIndex] = sleepIsland;
	}

	void ResetSleepIsland(const int levelIndex)
	{
		FastAssert(LegitLevelIndex(levelIndex));
		FastAssert(!IsNonexistent(levelIndex));
		m_pauSleepIslands[levelIndex].index = 0xffff;
		m_pauSleepIslands[levelIndex].generationId = 0xffff;
	}

	// PURPOSE
	//	Find out if a level index corresponds to an instance that is in the level
	// PARAMS
	//	levelIndex - the level index of the instance
	// RETURN
	//	true if the level index corresponds to an object that is in the level, false if it does not
	bool IsInLevel (int levelIndex) const;

	// PURPOSE
	//    Determine whether an object is active, inactive, fixed, or nonexistent.
	// PARAMS
	//    instance - optional instance pointer
	// RETURNS
	//    The eObjectState of the instance, or nonexistent if there is no instance
	phLevelBase::eObjectState GetStateSafe (const phInst* instance) const;

	// PURPOSE
	//    Find out whether an instance is active.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    An object is active if it was added with AddActiveObjectFromSimulator and was not deactivated with
	//    DeactivateObject, or if it was activated with ActivateObjectFromSimulator. Usually, objects become
	//    active when they are physically moving, and then become inactive when they fall asleep.
	// SEE ALSO
	//    GetState
	bool IsActive(int levelIndex) const;

	// PURPOSE
	//    Find out whether an instance is inactive.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    An object is inactive if it was added with AddInactiveObject and was not activated with
	//    ActivateObject, or if it was deactivated with DeactivateObject. Usually, objects become
	//    active when they are physically moving, and then become inactive when they fall asleep.
	// SEE ALSO
	//    GetState
	bool IsInactive(int levelIndex) const;

	// PURPOSE
	//    Find out whether an instance is inactive.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    A fixed object cannot become active, and is usually used for terrain. It is added with
	//    AddFixedObject.
	// SEE ALSO
	//    GetState
	bool IsFixed(int levelIndex) const;

	// PURPOSE
	//    Find out whether an instance exists (has been added to the level).
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    When the level is first initialized, all the level indices are "nonexistent".
	// SEE ALSO
	//    GetState
	bool IsNonexistent(int levelIndex) const;

	bool ShouldCollideByState (int levelIndexA, int levelIndexB) const;
	bool ShouldCollideByState(phLevelBase::eObjectState stateA, phLevelBase::eObjectState stateB, int levelIndexA, int levelIndexB) const;

	bool ShouldCollideByExistentState (int levelIndexA, int levelIndexB) const;
	bool ShouldCollideByExistentState(phLevelBase::eObjectState stateA, phLevelBase::eObjectState stateB, int levelIndexA, int levelIndexB) const;

protected:
	// PURPOSE
	//    Control whether an object is active, inactive, fixed, or nonexistent.
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	//    newObjectState - The new eObjectState of the instance
	// SEE ALSO
	//    GetState
	void SetState(const int levelIndex, eObjectState newObjectState);

public:
    // PURPOSE
    //    Get the user data for an instance.
    // PARAMS
    //    levelIndex - The level index of the instance of interest
    // NOTES
    //    Currently, only active objects can have user data. It stores information to hook the
    //    instance to its phCollider.
    void* GetUserData(int levelIndex) const;

	// PURPOSE
	//    Get the user data from an instance
	// PARAMS
	//    levelIndex - The level index of the instance of interest
	// NOTES
	//    DOES NOT verify that the object is active, the results are undefined if the object is not active.
	__forceinline void* GetActiveObjectUserData(const int levelIndex) const
	{
		return m_pauObjectUserData[levelIndex];
	}

	void** GetActiveObjectUserDataArray()
	{
		return m_pauObjectUserData;
	}

	int GetCurrentMaxLevelIndex()
	{
		return m_CurrentMaxLevelIndex;
	}

    ////////////////////////////////////////////////////////////
    // Cache optimization
    // PURPOSE:	Check if an instance in the level matches the requirements
    //			of an iterator.
    // PARAMS:	iter		- The physics iterator to check against.
    //			levelIndex	- The level index of the instance to check. Must be valid.
    bool CachedObjectDataCheckInstance(const phIterator &iter, int levelIndex) const;

	////////////////////////////////////////////////////////////
	// Adding objects to the level.

// Users should go through the phSimulator to add/remove objects
protected:
	// <COMBINE phLevelBase::AddActiveObjectFromSimulator>
	void AddToBroadphase( phInst * pInst, int uLevelIndex );
	void AddToBroadphase( phInst **pInst, int nCount, int *levelIndex );

	int AddObjectHelper( phInst* pInst, eObjectState type, void* userData = NULL );
	int AddObject( phInst* pInst, eObjectState type, void* userData, bool delaySAPAdd);
	void AddObjects( phInst **pInst, int nCount, int *levelIndexOut, eObjectState type, void** userData = NULL );

    // PURPOSE
    //    Add an instance to the level as an active (moving) object.
    // PARAMS
    //    inst - The instance to be added
    //    userData - The user data to store with the instance, for keeping track of the phCollider it relates to
    //    permanentlyActive - Whether or not the instance is allows to go to sleep (become inactive)
    // NOTES
    //    This method should only be called by the simulator. From game-level code, call PHSIM->AddActiveObject.
	inline int AddActiveObjectFromSimulator (phInst* pInst, void* userData, bool UNUSED_PARAM(permanentlyActive), bool delaySAPAdd)
	{
		return AddObject( pInst, OBJECTSTATE_ACTIVE, userData, delaySAPAdd );
	}

    // PURPOSE
    //    Add an instance to the level as a fixed object.
    // PARAMS
    //    inst - The instance to be added
    // NOTES
    //    This function should only be called by the phSimulator.
	inline int AddFixedObject(phInst* pInst, bool delaySAPAdd)
	{
		return AddObject( pInst, OBJECTSTATE_FIXED, NULL, delaySAPAdd );
	}

    // PURPOSE
    //    Add an instance to the level as an active (moving) object.
    // PARAMS
    //    inst - The instance to be added
    //    testLocation - The user data to store with the instance, for keeping track of the phCollider it relates to
    //    stateIncludeFlags - Whether or not the instance is allows to go to sleep (become inactive)
    // NOTES
    //    This function should only be called by the phSimulator.
	inline int AddInactiveObject(phInst* pInst, bool delaySAPAdd)
	{
		return AddObject( pInst, OBJECTSTATE_INACTIVE, NULL, delaySAPAdd );
	}

	////////////////////////////////////////////////////////////
	// Removing objects from the level.
	void DeleteObject1(int levelIndex);
	void DeleteObject2(int levelIndex);
	void DeleteObjects(int *levelIndex, int count);

    // PURPOSE
    //    Remove an instance from the level.
    // PARAMS
    //    instance - The instance to be deleted
    // NOTES
    //    This function should only be called by the phSimulator.
	void DeleteObject(phInst* pInst, bool delaySAPRemove);

    // PURPOSE
    //    Remove an instance from the level.
    // PARAMS
    //    levelIndex - The level index of the instance to be deleted
    // NOTES
    //    This function should only be called by the phSimulator.
	void DeleteObject(int nLevelIndex, bool delaySAPRemove);						// Remove object with level index nLevelIndex.

	void CommitDelayedDeleteObjects(int *levelIndex, int nCount);

#if !__SPU
    // PURPOSE
    //    Delete every object in a level.
    // NOTES
    //    This function should only be called by the phSimulator.
	void DeleteAll()										// Remove all objects.
	{
#if ENABLE_PHYSICS_LOCK
		PHLOCK_SCOPEDWRITELOCK;
#endif	// ENABLE_PHYSICS_LOCK
		for(int nObjectLevelIndex = 0; nObjectLevelIndex < m_MaxObjects; ++nObjectLevelIndex)
		{
			if(!IsNonexistent(nObjectLevelIndex))
			{
				DeleteObject(nObjectLevelIndex,false);
			}
		}
	}
#endif
public:
	////////////////////////////////////////////////////////////
	// modifying
//	void ResetLocation(const int knLevelIndex, const Matrix34 &rmtxNew = M34_IDENTITY);

	// PURPOSE:	Notify the level that an instance may have had its archetype switched,
	//			or include or type flags inside the archetype changed.
	// PARAMS:	levelIndex		- The index of an instance in the level.
	void UpdateObjectArchetype(int levelIndex);

    // Notify the level that an object has moved.  The level will update its octree to ensure that it is consistent with this object's position.

    // PURPOSE
    //    Inform the physics level that an object has moved.
    // PARAMS
    //    levelIndex - The level index of the instance to be deleted
	void UpdateObjectLocation(int levelIndex, const Matrix34* lastInstMatrix = NULL);
	void UpdateObjectLocationAndRadius(int knLevelIndex, const Matrix34* lastInstMatrix = NULL);
	void UpdateObjectLocationAndRadius(int levelIndex, const Mat34V_Ptr lastInstMatrix=NULL);

	// PURPOSE
	//    Gets and Sets the flag that indicates to the physics system that only the main thread is able to update it.
	// NOTES
	//    Particular overhead, such as locking needed because of multi-threaded requirements, can be very expensive during 
	//    certain operations on the physics level - UpdateObjectLocation() and the like can spend a large amount of time 
	//    inside lock/unlock on critical sections. However, many times these calls are made during a point in the frame when 
	//    only the main thread would be updating the physics system anyways. We allow the game-side code to set this flag 
	//    to indicate to us when this is true, so we can skip the locking overhead in the physics level as an optimization.
	inline bool GetOnlyMainThreadIsUpdatingPhysics() const { return m_bOnlyMainThreadIsUpdatingPhysics; }
	void SetOnlyMainThreadIsUpdatingPhysics(bool bOnlyMainThreadIsUpdatingPhysics) { Assert(m_bOnlyMainThreadIsUpdatingPhysics != bOnlyMainThreadIsUpdatingPhysics); m_bOnlyMainThreadIsUpdatingPhysics = bOnlyMainThreadIsUpdatingPhysics; }

	// PURPOSE
	//   Inform the physics level that the parts of a composite bound have moved
	// PARAMS
	//   levelIndex - the index of the instance owning the composite
	void RebuildCompositeBvh(int levelIndex);
	void UpdateCompositeBvh(int levelIndex);

#if !__SPU
	// These functions will handle grabbing the global physics write lock if they result in deleting any sub-bounds.
	// The lock will only be grabbed if a bound will be deleted, and only a maximum of once per call (we hold onto it until the end of the function).
	static void CompositeBoundSetBoundThreadSafe(phBoundComposite& compositeBound, int componentIndex, phBound* pBound);
	static void CompositeBoundSetBoundsThreadSafe(phBoundComposite& compositeBound, const phBoundComposite& compositeBoundToCopy);
	static void CompositeBoundSetActiveBoundsThreadSafe(phBoundComposite& compositeBound, const phBoundComposite& compositeBoundToCopy);
	static void CompositeBoundRemoveBoundThreadSafe (phBoundComposite& compositeBound, int componentIndex);
	static void CompositeBoundRemoveBoundsThreadSafe (phBoundComposite& compositeBound);
#endif // !__SPU

	// PURPOSE
	//    Suppress asserts and warnings about objects that have been moved without the level being informed.  This is useful and important when
	//    you want to move/resize more than one instance without informing the level between each of them.  Wrap your UpdateObjectLocation[AndRadius]
	//    calls with [Begin/End]MultipleUpdates() and this will prevent the level from complaining about the instances whose changes you were just
	//    about to tell the level about.
	void BeginMultipleUpdates() { m_MultipleUpdates = true; }
	void EndMultipleUpdates() { m_MultipleUpdates = false; }

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	// PURPOSE
	//    Control whether or not deferred octree updates 
	// NOTES
	//    "Deferred octree update" refers to the ability of the to level to not immediately update the octree structure to account for changes to the objects.
	//    This is useful to avoid stalls in situations where you think that a lot of 'read access' may be occurring with the level.  Calling this will queue
	//    up the level indices of objects that have changed and the octree structure itself will be updated later, whenever CommitDeferredOctreeUpdates() is
	//    called.
	void SetEnableDeferredOctreeUpdate(bool enableDeferredOctreeUpdate)
	{
		m_DeferredOctreeUpdateEnabled = enableDeferredOctreeUpdate;
	}

	void CommitDeferredOctreeUpdates();

	__forceinline u32 GetNumDeferredUpdateInstances() const
	{
		return m_uNumDeferredUpdateLevelIndices;
	}
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	// PURPOSE
	//    Control whether or not composites BVH updates are deferreed or done instantly
	void SetEnableDeferredCompositeBvhUpdate(bool enableDeferredCompositeBvhUpdate)
	{
		m_DeferredCompositeBvhUpdateEnabled = enableDeferredCompositeBvhUpdate;
	}

	// PURPOSE
	//   Update or rebuild all of the composite BVHs that were marked for a deferred update
	void ProcessDeferredCompositeBvhUpdates();
#endif

    // PURPOSE
    //    Find out if some number of active objects can be allocated at this time.
    // PARAMS
    //    numToAdd - The number of active objects we hypothetically would like to add
	bool CheckAddActiveObjects(u32 uNumToAdd) const { return m_NumActive + uNumToAdd <= m_MaxActive; }

	////////////////////////////////////////////////////////////
	// iterators

    // PURPOSE
    //    Begins an iteration through the active objects
    // RETURNS
    //    The level index of the "first" active object, or INVALID_INDEX if no objects are active
    // SEE ALSO
    //    GetNextActiveIndex
	int GetFirstActiveIndex ();								// the first active object index

    // PURPOSE
    //    Continue an iteration through the active objects
    // RETURNS
    //    The level index of the next active object, or INVALID_INDEX if no objects are active
    // SEE ALSO
    //    GetFirstActiveIndex
	int GetNextActiveIndex ();								// the the next active object index

	// JTODO: Change this name to GetLevelIndexFromActiveIndex.
	int GetActiveLevelIndex(const int knActiveIndex) const
	{
		FastAssert(knActiveIndex >= 0);
		FastAssert(knActiveIndex < m_NumActive);
		return m_pauActiveObjectLevelIndex[knActiveIndex];
	}

	u16* GetActiveObjectArray() const
	{
		return m_pauActiveObjectLevelIndex;
	}

	////////////////////////////////////////////////////////////
	// phInst Last matrices
#if (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	enum { INVALID_INST_LAST_MATRIX_INDEX = 0xFFFF };
#else
	enum { INVALID_INST_LAST_MATRIX_INDEX = 0xFF };
#endif
	void                         ReserveInstLastMatrix(phInst* inst);
	void                         ReleaseInstLastMatrix(phInst* inst);
	Mat34V_ConstRef  GetLastInstanceMatrix(const phInst* inst) const;
	void                         SetLastInstanceMatrix(const phInst* inst, Mat34V_In lastMtx);
#if !(RSG_PC || RSG_DURANGO || RSG_ORBIS)
	// For setting up SPU code.
	u8*                          GetLastInstanceMatrixIndexMapBaseAddr() { return m_pInstLastMatrixIndices; }
	Mat34V*                      GetLastInstanceMatricesBaseAddr()       { return m_pInstLastMatrices; }
#endif
	phObjectData*                GetObjectDataArray()                    { return m_paObjectData; }

#if __BANK
	void AddLevelWidgets(bkBank &bank);

	static void AddWidgets(bkBank &bank);

	// PURPOSE:
	//  Get the coordinates of the debug poi center. See m_DebugPOICenter for more information on what this is for.
	Vec3V_Out GetDebugPOICenter() const { return m_DebugPOICenter; }

	// PURPOSE:
	//  Set the coordinates of the debug poi center. See m_DebugPOICenter for more information on what this is for.
	void SetDebugPOICenter(Vec3V_In center) { m_DebugPOICenter = center; }

#endif // __BANK

protected:

#if __BANK
 	void DumpObjects();
	void DumpNode(__NodeType& node) const;
#endif // __BANK

	// PURPOSE:	Initialize an object data from its instance, archetype, and bound.
	// PARAMS:	objectDataToInit - The object data to get initialized.
	// NOTES:	In order for this to work properly, the object data must have its instance already set.  That is the only pre-requisite, however.
	void InitObjectData(phObjectData &objectDataToInit);

SPU_PUBLIC

public:
	////////////////////////////////////////////////////////////
	// colliding

    // PURPOSE
    //    Thaw the active state; convert pending active objects to active objects and update active object locations.
    // SEE ALSO
    //    FreezeActiveState, GetFrozenActiveState
	void PostCollideActives();								// process the activation changes and update all active object locations

	////////////////////////////////////////////////////////////
	// culling
#if 0
	bool CullSphere (const Vector3& UNUSED_PARAM(center), float UNUSED_PARAM(radius), const phInst* UNUSED_PARAM(excludeInstance)=NULL,
		u32 UNUSED_PARAM(includeFlags)=INCLUDE_FLAGS_ALL, u32 UNUSED_PARAM(typeFlags)=TYPE_FLAGS_ALL,
		u8 UNUSED_PARAM(stateIncludeFlags)=phLevelBase::STATE_FLAGS_ALL) const
	{
		return false;
	}
	///	// extra version of CullSphere just to leave out the exclude instance pointer
	///	bool CullSphere (const Vector3& center, float radius, u32 includeFlags, u32 typeFlags=TYPE_FLAGS_ALL,
	///		u8 stateIncludeFlags=phLevelBase::STATE_FLAGS_ALL) const
	///	{ return CullSphere(center,radius,NULL,includeFlags,typeFlags,stateIncludeFlags); }

	bool CullXZCircle (const Vector3& UNUSED_PARAM(center), float UNUSED_PARAM(radius), u32 UNUSED_PARAM(includeFlags))
	{
		return false;
	}
#endif
	////////////////////////////////////////////////////////////
	// New object culling methods.
	// You generally won't use any of these, you'll probably use an iterator, but they're here in case you want them.
#if !LEVELNEW_SPU_CODE
	u16 GetFirstCulledNode(phIterator& it) const;
	u16 GetNextCulledNode(phIterator& it) const;
#else
	u16 GetFirstCulledNode(phIterator& it, __NodeType &outSpuNode) const;
	u16 GetNextCulledNode(phIterator& it, __NodeType &outSpuNode) const;
#endif
	u16 GetNextCulledObjectFromNode(phIterator& it) const;

#if !LEVELNEW_SPU_CODE
	u16 GetFirstCulledObjectFromNode(phIterator& it, const int knCurOctreeNodeIndex) const;
#else
	u16 GetFirstCulledObjectFromNode(phIterator& it, const int knCurOctreeNodeIndex, const __NodeType *pCurOctreeNode) const;
#endif

    // PURPOSE
    //    Begin a culling operation, and get the level index of the first culled instance
    // PARAMS
    //    it             - The iterator used to define the culling parameters
    // RETURNS
    //    The level index of the first culled object
    // SEE ALSO
    //    GetNextCulledObject
	u16 GetFirstCulledObject(phIterator& it) const;

    // PURPOSE
    //    Continue a culling operation, and get the level index of the next culled instance
    // PARAMS
    //    it          - The iterator used to define the culling parameters
    // RETURNS
    //    The level index of the next culled object
    // SEE ALSO
    //    GetFirstCulledObject
	u16 GetNextCulledObject(phIterator& it) const;

	phInst* GetFirstCulledInstance(phIterator& it) const;
	phInst* GetNextCulledInstance(phIterator& it) const;

	bool CheckNodeAgainstCullObject(const __NodeType& octreeNode, const phIterator& it) const;

	// You specify the culling type through krCullObject.
	DEPRECATED u16 CullObjects(phIterator& it, u16 *paLevelIndices) const;
	//
	////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////
	// Test methods

    // PURPOSE:	See if the given point in world coordinates is inside any objects.
    // PARAMS:
    //	point - the point to test, in world coordinates
    //	isect - optional pointer to an intersection
    //	excludeInstance	- optional instance that should be ignored
    //	includeFlags - optional flags to specify what types of objects should be included (default is all)
    //	typeFlags - optional flags to specify what types objects should include this test (default is all)
    //	stateIncludeFlags - optional flags to specify what object states should be included (default is all)
    //	numIntersections - optional number of intersections to fill in (see notes)
    // RETURN:	1 if the point is inside any objects, 0 if it isn't, or the number of objects the point is inside if numIntersections is specified
    // NOTES:
    //	1.	If the point is inside more than one object, the one with the smallest depth will be returned.
    //	2.	To get a hit, the include flags of the test must match the type flags of the object's archetype, and the type flags
    //		of the test must match the include flags of the object's archetype.
    //	3.	If numIntersections is passed in then the test will return all objects intersecting the given object in no particular order, up to
    //		the numIntersections. The intersection argument in this case must be an array with at least numIntersections.
	int TestPoint (const Vector3& point, phIntersection* isect, const phInst* excludeInstance=NULL, u32 includeFlags=INCLUDE_FLAGS_ALL,
					u32 typeFlags=TYPE_FLAGS_ALL, u8 stateIncludeFlags=phLevelBase::STATE_FLAGS_ALL, int numIntersections=DEFAULT_NUM_ISECTS, u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

    // PURPOSE:	Test a directed line segment against the world.
    // PARAMS:
    //	segment - The line segment to test.
    //	isect - A pointer to a record that gets filled out with the details of any intersection.
    //	excludeInstance - An entity that should not be considered when probing.
    //	includeFlags - Flags that indicate what type of entities should be considered when probing.
    //	stateIncludeFlags - optional flags that indicate what object states should be be included by this edge test
    //	numIntersections - optional number of intersections to fill in (see notes)
    // RETURN:	1 if there is any intersection, 0 if there isn't, or the number of objects the probe hit if numIntersections is specified
    // NOTES:
    //	1.	If you have several probes to do all in the same area, and with the same exclude-instance and include-flags,
    //		consider using TestProbeBatch().
    //	2.	If numIntersections is passed in then the test will return all objects intersecting the given object in no particular order, up to
    //		the numIntersections. The intersection argument in this case must be an array with at least numIntersections.
	int TestProbe (const phSegment &krsegProbe, phIntersection *pIsect, const phInst *pExcludeInstance = NULL, u32 uIncludeFlags = INCLUDE_FLAGS_ALL,
					u32 uTypeFlags = TYPE_FLAGS_ALL, u8 uStateIncludeFlags = phLevelBase::STATE_FLAGS_ALL, int numIntersections=DEFAULT_NUM_ISECTS,
					u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

    // PURPOSE:	Test an undirected line segment against the world.
    // PARAMS:
    //	segment - The line segment to test.
    //	isect0 - optional pointer to a record that gets filled out with the details of the first intersection.
    //	isect1 - optional pointer to a record that gets filled out with the details of the second intersection.
    //	excludeInstance - An entity that should not be considered when probing.
    //	includeFlags - optional flags that indicate what types of entities should be be included by this edge test
    //	typeFlags - optional flags that indicate what type of entities should include this edge test
    //	stateIncludeFlags - optional flags that indicate what object states should be be included by this edge test
    //	numIsectPairs - optional number of pairs of intersections to fill in (see notes)
    // RETURN:	the number of intersections found (0, 1 or 2), or if numIntersections was specified, then the number of objects intersected
    // NOTES
    //	1.	If you have several probes to do all in the same area, and with the same exclude-instance and include-flags,
    //		consider using TestProbeBatch().
    //	2.	If numIsectPairs is passed in then the test will return all objects intersecting the given object in no particular order, up to
    //		the numIsectPairs. The intersection arguments in this case must be arrays with at least numIsectPairs.
	int TestEdge (const phSegment& segment, phIntersection* isect0=NULL, phIntersection* isect1=NULL, const phInst* excludeInstance=NULL,
					u32 includeFlags=INCLUDE_FLAGS_ALL, u32 typeFlags=TYPE_FLAGS_ALL, u8 stateIncludeFlags=STATE_FLAGS_ALL, int numIsectPairs=DEFAULT_NUM_ISECTS,
					u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

    // PURPOSE:	Test several directed line segments against the world.
    // PARAMS:
    //	numSegs - The number of segments to test.
    //	seg - The segments to test.
    //	segsCenter - The center of a sphere that bounds the segments.
    //	segsRadius - The radius of a sphere that bounds the segments.
    //	segsBox - The oriented box that bounds the segments.  a/b/c are the box's axes, and d is the box's half-extents.
    //	isect - An array of intersection records, one per segment, that receives the result of any intersection.
    //	excludeInstance - An entity that should not be considered when probing.
    //	includeFlags - Flags that indicate what type of entities should be considered when probing.
    // RETURN:	true if there was any intersection, false if there wasn't.
	bool TestProbeBatch (int numSegs, const phSegment** seg, const Vector3 &vecSegsCenter, float fSegsRadius, const Matrix34& UNUSED_PARAM(segsBox),
							phIntersection** isect, const phInst *pInstExclude=NULL, u32 uIncludeFlags=INCLUDE_FLAGS_ALL, u32 uTypeFlags=TYPE_FLAGS_ALL,
							u8 uStateIncludeFlags=phLevelBase::STATE_FLAGS_ALL, u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

    // PURPOSE:	Find intersections with a sphere.
    // PARAMS:
    //	center - the sphere center in world coordinates
    //	radius - the sphere radius
    //	isect - pointer to the class into which the intersection will be recorded
    //	excludeInstance - optional pointer to an instance that should be ignored (default is NULL)
    //	includeFlags - optional flags to indicate what types of objects should be included (default is all flags on)
    //	typeFlags - optional flags to indicate what types of object is doing the test (default is all flags on)
    //	stateIncludeFlags - optional flags to indicate what states should be included (default is all flags on)
    //	numIntersections - optional number of intersections to fill in (see notes)
    // RETURN:	1 if there is an intersection with the given sphere, 0 otherwise, or if numIntersections is specified then the number of objects hit
    // NOTES:
    //	1.	If numIntersections is passed in then the test will return all objects intersecting the given object in no particular order, up to
    //		the numIntersections. The intersection argument in this case must be an array with at least numIntersections.
	int TestSphere (const Vector3& krvecCenter, const float kfRadius, phIntersection *pIsect, const phInst *pExcludeInstance=NULL, u32 uIncludeFlags=INCLUDE_FLAGS_ALL,
						u32 uTypeFlags=TYPE_FLAGS_ALL, u8 uStateIncludeFlags=phLevelBase::STATE_FLAGS_ALL, int numIntersections=DEFAULT_NUM_ISECTS,
						u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

    // PURPOSE:	Find intersections with a capsule, or with a moving sphere (if directed). 
    // PARAMS:
    //	endA - an end of the capsule axis (the starting center of the moving sphere) in world coordinates
    //	endB - an end of the capsule axis (the ending center of the moving sphere) in world coordinates
    //	radius - the capsule radius
    //	isect - pointer to the class into which the intersection will be recorded
    //	excludeInstance - optional pointer to an instance that should be ignored (default is NULL)
    //	includeFlags - optional flags to indicate what types of objects should be included (default is all flags on)
    //	typeFlags - optional flags to indicate what types of object is doing the test (default is all flags on)
    //	stateIncludeFlags - optional flags to indicate what states should be included (default is all flags on)
    //	directed - optional boolean to tell whether the test capsule is directed (if it is a swept sphere)
    //	numIntersections - optional number of intersections to fill in (see notes)
    // RETURN:	1 if there is an intersection with the given capsule, 0 otherwise, or if numIntersections is specified then the number of objects hit
    // NOTES:
    //	1.	Directed capsule tests will detect the first intersection with a sphere moving from the starting position (endA) to
    //		the ending position (endB).  It will not detect intersections with the sphere's initial position.
    //	2.	If numIntersections is passed in then the test will return all objects intersecting the given object in no particular order, up to
    //		the numIntersections. The intersection argument in this case must be an array with at least numIntersections.
	int TestCapsule (Vector3::Vector3Param krvecP0, Vector3::Vector3Param krvecP1, float kfRadius, phIntersection* pIsect, const phInst* pInstExclude=NULL,
						u32 uIncludeFlags=INCLUDE_FLAGS_ALL, u32 uTypeFlags=TYPE_FLAGS_ALL, u8 uStateIncludeFlags=phLevelBase::STATE_FLAGS_ALL, bool bDirected=false,
						int numIntersections=DEFAULT_NUM_ISECTS, u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

	// PURPOSE:	Find intersections with a capsule, or with a moving sphere (if directed). 
	// PARAMS:
	//	axisStart - an end of the capsule axis (the starting center of the moving sphere) in world coordinates
	//	axis - the (unnormalized) capsule axis, from start to end, in world coordinates
	//	shaftLength - the capsule length
	//	radius - the capsule radius
	//	isect - pointer to the class into which the intersection will be recorded
	//	excludeInstance - optional pointer to an instance that should be ignored (default is NULL)
	//	includeFlags - optional flags to indicate what types of objects should be included (default is all flags on)
	//	typeFlags - optional flags to indicate what types of object is doing the test (default is all flags on)
	//	stateIncludeFlags - optional flags to indicate what states should be included (default is all flags on)
	//	directed - optional boolean to tell whether the test capsule is directed (if it is a swept sphere)
	//	numIntersections - optional number of intersections to fill in (see notes)
	// RETURN:	1 if there is an intersection with the given capsule, 0 otherwise, or if numIntersections is specified then the number of objects hit
	// NOTES:
	//	1.	Directed capsule tests will detect the first intersection with a sphere moving from the starting position (endA) to
	//		the ending position (endB).  It will not detect intersections with the sphere's initial position.
	//	2.	If numIntersections is passed in then the test will return all objects intersecting the given object in no particular order, up to
	//		the numIntersections. The intersection argument in this case must be an array with at least numIntersections.
	int TestCapsule (Vector3::Vector3Param axisStart, Vector3::Vector3Param axis, float shaftLength, float radius, phIntersection *pIsect=NULL, const phInst *pInstExclude=NULL,
						u32 uIncludeFlags=INCLUDE_FLAGS_ALL, u32 uTypeFlags=TYPE_FLAGS_ALL, u8 uStateIncludeFlags=phLevelBase::STATE_FLAGS_ALL, bool bDirected=false,
						int numIntersections=DEFAULT_NUM_ISECTS, u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

	// PURPOSE:	Find intersections with a moving sphere.
	// PARAMS:
	//    endA -              the starting center of the moving sphere in world coordinates
	//    endB -              the ending center of the moving sphere in world coordinates
	//    radius -            the sphere radius
	//    isect -		      pointer to the class into which the intersection will be recorded
	//    excludeInstance -   optional pointer to an instance that should be ignored (default is NULL)
	//    includeFlags -      optional flags to indicate what types of objects should be included (default is all flags on)
	//    typeFlags -         optional flags to indicate what types of object is doing the test (default is all flags on)
	//    stateIncludeFlags - optional flags to indicate what states should be included (default is all flags on)
	//    directed -          optional boolean to tell whether the test capsule is directed (if it is a swept sphere)
	//	numIntersections - optional number of intersections to fill in (see notes)
	// RETURN:	1 if there is an intersection with the moving sphere, 0 otherwise, or if numIntersections is specified then the number of objects hit
	// NOTES
	//    1.  Detects the first intersection with a sphere moving from the starting position (endA) to
	//    the ending position (endB).  It will not detect intersections with the sphere's initial position.
	//	2.	If numIntersections is passed in then the test will return all objects intersecting the given object in no particular order, up to
	//		the numIntersections. The intersection argument in this case must be an array with at least numIntersections.
	int TestSweptSphere (const Vector3& endA, const Vector3& endB, float radius, phIntersection* isect, const phInst* excludeInstance=NULL,
							u32 includeFlags=INCLUDE_FLAGS_ALL, u32 typeFlags=TYPE_FLAGS_ALL, u8 includeStateFlags=phLevelBase::STATE_FLAGS_ALL,
							int numIntersections=DEFAULT_NUM_ISECTS, u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

    // PURPOSE: Find the deepest intersection with the given object.
    // PARAMS:
    //	instance - the instance containing the bound, matrix, and type flags to test for intersections
    //	intersection - optional pointer to the class into which the intersection information will be recorded
    //	excludeInstance - optional pointer to an instance that should be ignored (default is NULL)
    //	includeFlags - optional flags to indicate what types of objects should be included (default is all flags on)
    //	typeFlags - optional flags to indicate what types of object is doing the test (default is all flags on)
    //	stateIncludeFlags - optional flags to indicate what states should be included (default is all flags on)
    //	numIntersections - optional number of intersections to fill in (see notes)
    // RETURN:	1 if there is an intersection with the instance, 0 otherwise, or if numIntersections is specified then the number of objects hit
    // NOTES:
    //	If numIntersections is passed in then the test will return all objects intersecting the given object in no particular order, up to
    //	the numIntersections. The intersection argument in this case must be an array with at least numIntersections.
	int TestObject (phInst* instance, phIntersection* intersection=NULL, const phInst* excludeInstance=NULL, u32 includeFlags=INCLUDE_FLAGS_ALL,
						u32 typeFlags=TYPE_FLAGS_ALL, u8 stateIncludeFlags=phLevelBase::STATE_FLAGS_ALL, int numIntersections=DEFAULT_NUM_ISECTS, u32 excludeTypeFlags=TYPE_FLAGS_NONE) const;

	////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////
	// in-place culling
	///	void SetInPlaceCullXZCircle (const Vector3 & center, float rad, u8 stateIncludeFlags)
	///	{ InPlaceCullCenter.Set(center); InPlaceCullRadius = rad; InPlaceCullStateFlags = stateIncludeFlags; InPlaceCullType = IPC_CIRCLE_XZ; }
	///	phInst * GetFirstInPlaceCullCircleXZ () = 0;			// init in-place culling and return first culled object, for circle-xz culls
	///	phInst * GetNextInPlaceCullCircleXZ () = 0;				// get next in-place culled object, for circle-xz culls

	////////////////////////////////////////////////////////////
	// activation/deactivation
// Users should go through the phSimulator to activate/deactivate objects. 
protected:
    // PURPOSE
    //    Convert an object from inactive to active (from stationary to moving).
    // NOTES
    //    Should be called only by the simulator
    // SEE ALSO
    //    DeactivateObject, AddActiveObject, IsActive
	bool ActivateObject(const int knLevelIndex, const void* kuUserData, bool bPermanentlyActive = false);

    // PURPOSE
    //    Convert an object from active to inactive (from moving to stationary).
    // NOTES
    //    Should be called only by the simulator
    // SEE ALSO
    //    ActivateObject, AddInactiveObject, IsInactive
	bool DeactivateObject(const int knLevelIndex);									// deactivate by level index

public:
	// PURPOSE
	//    Add to the physics level's broadphase pairings of this object with other objects that it overlaps.
	// PARAMS
	//    levelIndex - the level index of the object to test for overlap
	void FindAndAddOverlappingPairs (int levelIndex);

	////////////////////////////////////////////////////////////
	// visualization

	////////////////////////////////////////////////////////////
	// visualization

	// PURPOSE
	//    Draw the contents of the level.
#if !__SPU
	typedef atFunctor1<void, const phInst*> ColorChoiceFunc;
	void ProfileDraw(ColorChoiceFunc chooseColor = ColorChoiceFunc::NullFunctor()) const;	// draw bounds for all objects
	void DrawBound(const phInst *pInst, u32 typeFlagFilter, u32 includeFlagFilter) const;
#if __PFDRAW
	void DrawNormals(const phInst *pInst, int normalType = phBound::FACE_NORMALS, float length = 1.0f, unsigned int typeFilter = 0xffffffff, unsigned int includeFilter = 0xffffffff) const;
	void LevelColorChoice(const phInst* inst) const;
#endif //	__PFDRAW
#endif // !__SPU

	void ReturnObjectIndexToPool(u16 uObjectIndex);

	////////////////////////////////////////////////////////////
	// debugging

protected:

	// Utility function to adjust the octree data to account for an object that has moved.
	void UpdateObjectPositionInOctree(phObjectData &objectDataToUpdate, int nLevelIndex, Vec3V_In newCenter, float newRadius);

	// Utility function to adjust the octree data to account for an object has (potentially) changed size.
	void UpdateObjectRadiusInOctree(phObjectData &objectDataToUpdate, int nLevelIndex, Vec3V_In newCenter, float newRadius);

	// Utility function to update the broadphase 
	void UpdateObjectPositionInBroadphase(int nLevelIndex, const Matrix34* lastInstMatrix);

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	void AddLevelIndexToDeferredOctreeUpdateList(int nLevelIndex);

	void ClearDeferredOctreeUpdateList();
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

	// Attempt to update or rebuild a composite's BVH based on the level index of the composite
	void UpdateCompositeBvhFromLevelIndex(int levelIndex, bool fullRebuild);

	// Access flags without acquiring a lock.
	inline phLevelBase::eObjectState GetStateLockFree(int levelIndex) const;

	u16 GetOctreeNodeIndexFromPool();
	void ReturnOctreeNodeIndexToPool(u16 uOctreeNodeIndex);

	u16 GetObjectIndexFromPool();

	u16 AddObjectToOctree(u16 uLevelIndex, phObjectData &objectDataToAdd, Vec3V_In krvecCenter, const float kfRadius);
	void RemoveObjectFromOctree(__NodeType *pCurOctreeNode, const u32 uLevelIndex, phObjectData &objectDataToRemove, const float kfRadius);

	u16 PullSphere(const int knStartingOctreeNodeIndex, __NodeType *pInitialOctreeNode, Vec3V_In krvecCenter, const float kfRadius SPU_PARAM(phSpuObjectBuffer<__NodeType> &octreeNodeBuffer));
	u16 InsertObjectIntoSubtree(const int knStartingOctreeNodeIndex, __NodeType *pInitialOctreeNode, Vec3V_In krvecCenter, const float kfRadius, u16 uLevelIndex, phObjectData &objectDataToAdd SPU_PARAM(phSpuObjectBuffer<__NodeType> &octreeNodeBuffer));
	void PruneObject(__NodeType *pStartingOctreeNode, int startingOctreeNodeIndex);

	void AddObjectToOctreeNode(__NodeType *pInitialOctreeNode, const int knLevelIndex, phObjectData &objectDataToAdd);
	void RemoveFirstObjectFromOctreeNode(__NodeType *pCurOctreeNode, const phObjectData &objectDataToRemove);
	void RemoveObjectFromOctreeNode(__NodeType *pCurOctreeNode, const phObjectData &objectDataToRemove, phObjectData &prevObjectData);
	void RemoveObjectFromOctreeNode(__NodeType *pCurOctreeNode, const u32 knLevelIndex, const phObjectData &objectDataToRemove);

#if LEVELNEW_USE_EVERYWHERE_NODE
	void AddObjectToEverywhereNode(const int knLevelIndex, phObjectData &objectData);
	void RemoveObjectFromEverywhereNode(const int knLevelIndex, phObjectData &objectData);
#endif

	int CreateChild(const int knOctreeNodeIndex, __NodeType *pParentOctreeNode, const int knChildIndex);
	void CollapseChild(__NodeType *octreeNodeToCollapse, const int childOctreeNodeIndex);

	inline __NodeType *GetOctreeNode(const int knOctreeNodeIndex) const
	{
		FastAssert(m_uOctreeNodesInUse > 0);
		FastAssert(knOctreeNodeIndex >= 0);
		FastAssert(knOctreeNodeIndex < m_uTotalOctreeNodeCount);

		return &m_paOctreeNodes[knOctreeNodeIndex];
	}

	inline const __NodeType *GetConstOctreeNode(const int knOctreeNodeIndex) const
	{
		FastAssert(m_uOctreeNodesInUse > 0);
		FastAssert(knOctreeNodeIndex >= 0);
		FastAssert(knOctreeNodeIndex < m_uTotalOctreeNodeCount);

		return &m_paOctreeNodes[knOctreeNodeIndex];
	}

	// m_NumActive should be updated *after* this function is called.
	inline void AddActiveObjectUserData(const int knLevelIndex, const void* kuUserData)
	{
#if LEVELNEW_ACTIVE_LEVELINDEX_DUPE_CHECKING
		// There should never be any duplicate entries
		int nActiveObjectIndex;
		for(nActiveObjectIndex = m_NumActive-1; nActiveObjectIndex >= 0; --nActiveObjectIndex)
		{
			if( Unlikely(knLevelIndex == m_pauActiveObjectLevelIndex[nActiveObjectIndex]) )
			{
				// TODO: Switch to FastAssert whenever that or some equivalent resumes working in Bankrelease
				__debugbreak();
			}
		}
#endif

		m_pauObjectUserData[knLevelIndex] = const_cast<void*>(kuUserData);
		m_pauActiveObjectLevelIndex[m_NumActive] = (u16)(knLevelIndex);
	}

	void RemoveActiveObjectUserData(const int knLevelIndex);

	void DoConsistencyChecks(u16 uExcludeIndex, bool bRadiusIsGood) const;
	void CheckNode(const int knStartingOctreeNodeIndex, const int knParentOctreeNodeIndex, float &rfMaxSubRadius, int &rnNodesVisited, u32 &ruStateFlags, int &rnObjectsFound, u16 uExcludeIndex, bool bRadiusIsGood) const;
	void CheckForDuplicateObject(const phInst *pInst) const;

	bool CanOctreeBeOutOfSync() const;

#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	static bool GetSecondaryBroadphaseForBatchAddEnabled()
	{
		return sm_useSecondaryBroadphaseForBatchAdd;
	}

	static void EnableSecondaryBroadphaseForBatchAdd( bool enable )
	{
		sm_useSecondaryBroadphaseForBatchAdd = enable;
	}

	static int GetBatchAddThresholdForSecondaryBroadphase()
	{
		return sm_batchAddThresholdForSecondaryBroadphase;
	}

	static void SetBatchAddThresholdForSecondaryBroadphase( int t )
	{
		sm_batchAddThresholdForSecondaryBroadphase = t;
	}
#endif // !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE

	void ReserveInstLastMatrixInternal(phInst* inst); 
	void ReleaseInstLastMatrixInternal(phInst* inst);

#if __ASSERT
	bool IsObjectContained(const __NodeType *pCurOctreeNode, const int knLevelIndex SPU_PARAM(const phObjectData &cachedObjectData)) const
	{
		const int numShouldBeInNode = pCurOctreeNode->m_ContainedObjectCount;

		int actualNumInNode = 0;
		bool foundInNode = false;
		int nLevelIndex = pCurOctreeNode->m_ContainedObjectStartIdx;
		while(nLevelIndex != (u16)(-1))
		{
			++actualNumInNode;
			if(nLevelIndex == knLevelIndex)
			{
				foundInNode = true;
				const phObjectData *curObjectData = NON_SPU_ONLY(&m_paObjectData[nLevelIndex]) SPU_ONLY(&cachedObjectData);
				nLevelIndex = curObjectData->m_uNextObjectInNodeIndex;
			}
			else
			{
				SPU_ONLY(phSpuObjectBuffer<phObjectData> objectDataBuffer);
				const phObjectData *curObjectData = NON_SPU_ONLY(&m_paObjectData[nLevelIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&m_paObjectData[nLevelIndex]));
				nLevelIndex = curObjectData->m_uNextObjectInNodeIndex;
			}
		}
		Assertf(actualNumInNode == numShouldBeInNode, "%d / %d", actualNumInNode, numShouldBeInNode);

		return foundInNode;
	}

	bool IsObjectCountCorrect(const __NodeType *pCurOctreeNode, const int knLevelIndex, const phObjectData *cachedObjectData, const int /*id*/) const
	{
		const int numShouldBeInNode = pCurOctreeNode->m_ContainedObjectCount;

		int actualNumInNode = 0;
		int nLevelIndex = pCurOctreeNode->m_ContainedObjectStartIdx;
		while(nLevelIndex != (u16)(-1))
		{
			++actualNumInNode;
			if(nLevelIndex == knLevelIndex)
			{
				nLevelIndex = cachedObjectData->m_uNextObjectInNodeIndex;
			}
			else
			{
				SPU_ONLY(phSpuObjectBuffer<phObjectData> objectDataBuffer);
				const phObjectData *curObjectData = NON_SPU_ONLY(&m_paObjectData[nLevelIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&m_paObjectData[nLevelIndex]));
				nLevelIndex = curObjectData->m_uNextObjectInNodeIndex;
			}
		}
		return (actualNumInNode == numShouldBeInNode);
	}

	int GetActualObjectCount(const __NodeType *pCurOctreeNode, const int knLevelIndex, const phObjectData *cachedObjectData) const
	{
		int actualNumInNode = 0;
		int nLevelIndex = pCurOctreeNode->m_ContainedObjectStartIdx;
		while(nLevelIndex != (u16)(-1))
		{
			++actualNumInNode;
			if(nLevelIndex == knLevelIndex)
			{
				nLevelIndex = cachedObjectData->m_uNextObjectInNodeIndex;
			}
			else
			{
				SPU_ONLY(phSpuObjectBuffer<phObjectData> objectDataBuffer);
				const phObjectData *curObjectData = NON_SPU_ONLY(&m_paObjectData[nLevelIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&m_paObjectData[nLevelIndex]));
				nLevelIndex = curObjectData->m_uNextObjectInNodeIndex;
			}
		}
		return actualNumInNode;
	}
#endif	// __ASSERT

	int GetOctreeNodeIndex(const int knLevelIndex)
	{
		return m_paObjectData[knLevelIndex].GetOctreeNodeIndex();
	}

#if !__SPU
	static phLevelNodeTree<__NodeType> *sm_ActiveInstance;								// singleton for active level
#endif // !__SPU

	int m_nActiveIteratorIndex;

	bool m_MultipleUpdates;

	int m_CurrentMaxLevelIndex;

	// PURPOSE: The center of the physics level extents in world coordinates.
	Vec3V m_LevelCenter;

	// PURPOSE: The half-width of the physics level extents (a cube).
	float m_fLevelHalfWidth;

    int m_MaxCollisionPairs;

	int m_nMinObjectsInNodeToCreateChild;		// Minimum number of objects that a node must have in order to be created.
	int m_nMaxObjectsInNodeToCollapseChild;		// Maximum number of objects that a node can have and still get collapsed into its parent.

	// Indexed by level index.
	phObjectData *m_paObjectData;
#if LEVELNEW_GENERATION_IDS
	u16 *m_pauGenerationIDs;					// Tracks how many times a given level index has been issued - intentionally rolls over on overflow.
#endif
	phSleepIsland::Id *m_pauSleepIslands;
	u16 *m_pauObjectDataIndexList;				// Used to keep track of which level indices are available.
												// Contrary to the comment above, this is *not* indexed by level index.
	void **m_pauObjectUserData;					// These should store something that is meaningful to the user of this level.
												// In general, this is going to be a pointer to a phCollider.

	// Indexed by active object index; the first m_uNumActive of these are valid.
	u16 *m_pauActiveObjectLevelIndex;			// Maps from active object index to level index.
												// These should remain in sorted order for quick searching.

	// The pool of octree nodes.
	u16 m_uTotalOctreeNodeCount;
	u16 m_uOctreeNodesInUse;					// It's not 
	__NodeType *m_paOctreeNodes;

#if LEVELNEW_USE_EVERYWHERE_NODE
	// When an object gets out of the bounds of the level we put it into the imaginary 'node 8191'.  This isn't an actual node in the octree
	//   but is, instead, 
	u16 m_uFirstObjectInEverywhereNodeIndex;
#endif

#if __ASSERT
	sysIpcCurrentThreadId	m_threadId;
#endif
	bool m_bOnlyMainThreadIsUpdatingPhysics;

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
SPU_PUBLIC
	sysCriticalSectionToken m_DeferredOctreeUpdateCriticalSectionToken;
	u16 *m_pauDeferredUpdateLevelIndices;		// List of the level indices of objects that need to have 
	u16	m_uNumDeferredUpdateLevelIndices;
	u16 m_uMaxDeferredUpdateLevelIndices;
	bool m_DeferredOctreeUpdateEnabled;
	atBitSet m_DeferredOctreeUpdateLevelIndices;
SPU_PROTECTED
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	bool m_DeferredCompositeBvhUpdateEnabled;                           // If this is false, composite BVHs can be rebuilt instantly
	sysCriticalSectionToken m_DeferredBvhUpdateCriticalSectionToken;	// Protects against multiple threads adding a deferred composite BVH update
	atBitSet m_UpdateCompositeBvhLevelIndices;							// Each bit represents if that level index requires an update or rebuild
	atBitSet m_FullRebuildCompositeBvhLevelIndices;						// Each bit represents if that level index requires a rebuild
	atArray<u16> m_DeferredBvhUpdateCompositeLevelIndices;				// Handles to composites that need their BVH updated or rebuild
#endif

	// Used to keep track of which node indices are available.
	u16 *m_pauOctreeNodeIndexList;				// 
public:

	BroadPhaseType m_BroadPhaseType;
	phBroadPhase *m_BroadPhase;
	phBroadPhase *m_secondaryBroadphase;
	InstOutOfWorldCallback m_InstOutOfWorldCallback;
private:
#if (RSG_PC || RSG_DURANGO || RSG_ORBIS) 
	u16*		m_pInstLastMatrixIndices;
#else
	u8*			m_pInstLastMatrixIndices;
#endif // (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	int			m_MaxInstLastMatrices;	
	Mat34V*		m_pInstLastMatrices;
#if (RSG_PC || RSG_DURANGO || RSG_ORBIS) 
	atArray<u16>	m_FreeInstLastMatrices;
#else
	atArray<u8>	m_FreeInstLastMatrices;
#endif // (RSG_PC || RSG_DURANGO || RSG_ORBIS)
public:
#if __BANK
	// PURPOSE:
	//  This is used by DumpObjects to calculate distances to the current POI center (usually the player or camera).
	Vec3V m_DebugPOICenter;

	static int sm_MaxOctreeNodesEverUsed;
	static int sm_MaxActiveObjectsEver;
	static int sm_MaxObjectsEver;
	static int sm_TotalObjectsAdded;
	static int sm_TotalObjectsDeleted;
	static bool sm_DisableInactiveCollidesAgainstInactive;
	static bool sm_DisableInactiveCollidesAgainstFixed;
#endif

#if !__SPU && __PFDRAW
	static ColorChoiceFunc sm_ColorChoiceFunc;
#endif

#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	static bool sm_useSecondaryBroadphaseForBatchAdd;
	static int sm_batchAddThresholdForSecondaryBroadphase;
#endif // !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
};


template <class __NodeType> __forceinline phInst* phLevelNodeTree<__NodeType>::GetInstance (int levelIndex) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	FastAssert(GetState(levelIndex)!=OBJECTSTATE_NONEXISTENT);
	return m_paObjectData[levelIndex].GetInstance();
}

#if __DEV || __BANK
// For debug code only -- We don't want to crash if we somehow force a foolish query, they'll be responsible for null checking
template <class __NodeType> inline phInst* phLevelNodeTree<__NodeType>::GetInstanceSafe (int levelIndex) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	return (GetState(levelIndex)!=OBJECTSTATE_NONEXISTENT) ? m_paObjectData[levelIndex].GetInstance() : NULL;
}
#endif

template <class __NodeType> __forceinline phInst* phLevelNodeTree<__NodeType>::GetInstance(int levelIndex, int generationId)
{
	phInst* instance = NULL;

	if (IsLevelIndexGenerationIDCurrent(levelIndex, generationId))
	{
		instance = GetInstance(levelIndex);
	}

	return instance;
}

template <class __NodeType> __forceinline phInst* phLevelNodeTree<__NodeType>::GetInstance(phHandle handle)
{
	return GetInstance(handle.GetLevelIndex(), handle.GetGenerationId());
}

template <class __NodeType> __forceinline phLevelBase::eObjectState phLevelNodeTree<__NodeType>::GetState(int levelIndex) const
{
	//	Functions that call GetState and are performing operations based on the state should have the physics
	//	locked at the time GetState is called and until the code is done manipulating the object. 
	//
	//  NOTE: At this time, GetState does not confirm this thread has the lock. 
	//
	//  TODO: Add a #define macro & support code for asserting that the current thread has a lock on 
	//		the physics mutex at the time of this call.

	FastAssert(LegitLevelIndex(levelIndex));
	return m_paObjectData[levelIndex].GetState();
}


template <class __NodeType> __forceinline Vec4V_Out phLevelNodeTree<__NodeType>::GetCenterAndRadius (int levelIndex) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	return m_paObjectData[levelIndex].GetCenterAndRadius();
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::SetInstanceTypeAndIncludeFlags(int levelIndex, u32 typeFlags, u32 includeFlags)
{
	FastAssert(LegitLevelIndex(levelIndex));
	phObjectData &objectData = m_paObjectData[levelIndex];
	objectData.m_CachedArchTypeFlags = typeFlags;
	objectData.m_CachedArchIncludeFlags = includeFlags;
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::SetInstanceTypeFlags(int levelIndex, u32 typeFlags)
{
	FastAssert(LegitLevelIndex(levelIndex));
	phObjectData &objectData = m_paObjectData[levelIndex];
	objectData.m_CachedArchTypeFlags = typeFlags;
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::SetInstanceIncludeFlags(int levelIndex, u32 includeFlags)
{
	FastAssert(LegitLevelIndex(levelIndex));
	phObjectData &objectData = m_paObjectData[levelIndex];
	objectData.m_CachedArchIncludeFlags = includeFlags;
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::AddInstanceTypeFlags(int levelIndex, u32 flagsToAdd)
{
	FastAssert(LegitLevelIndex(levelIndex));
	phObjectData &objectData = m_paObjectData[levelIndex];
	objectData.m_CachedArchTypeFlags |= flagsToAdd;
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::AddInstanceIncludeFlags(int levelIndex, u32 flagsToAdd)
{
	FastAssert(LegitLevelIndex(levelIndex));
	phObjectData &objectData = m_paObjectData[levelIndex];
	objectData.m_CachedArchIncludeFlags |= flagsToAdd;
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::ClearInstanceTypeFlags(int levelIndex, u32 flagsToClear)
{
	FastAssert(LegitLevelIndex(levelIndex));
	phObjectData &objectData = m_paObjectData[levelIndex];
	objectData.m_CachedArchTypeFlags &= ~flagsToClear;
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::ClearInstanceIncludeFlags(int levelIndex, u32 flagsToClear)
{
	FastAssert(LegitLevelIndex(levelIndex));
	phObjectData &objectData = m_paObjectData[levelIndex];
	objectData.m_CachedArchIncludeFlags &= ~flagsToClear;
}


template <class __NodeType> __forceinline u32 phLevelNodeTree<__NodeType>::GetInstanceTypeFlags(int levelIndex) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	return m_paObjectData[levelIndex].m_CachedArchTypeFlags;
}


template <class __NodeType> __forceinline u32 phLevelNodeTree<__NodeType>::GetInstanceIncludeFlags(int levelIndex) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	return m_paObjectData[levelIndex].m_CachedArchIncludeFlags;
}


template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::GetInstanceTypeFlag(int levelIndex, u32 flagToCheck) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	return (m_paObjectData[levelIndex].m_CachedArchTypeFlags & flagToCheck) != 0;
}


template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::GetInstanceIncludeFlag(int levelIndex, u32 flagToCheck) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	return (m_paObjectData[levelIndex].m_CachedArchIncludeFlags & flagToCheck) != 0;
}


template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::MatchFlags(u32 typeFlagsA, u32 includeFlagsA, u32 typeFlagsB, u32 includeFlagsB)
{
	const u32 flagsTypeAIncludeB = typeFlagsA & includeFlagsB;
	const u32 flagsTypeBIncludeA = typeFlagsB & includeFlagsA;

	FastAssert((flagsTypeAIncludeB == 0 || flagsTypeBIncludeA == 0) == ((u64)flagsTypeAIncludeB * (u64)flagsTypeBIncludeA == 0));
	return ((u64)flagsTypeAIncludeB * (u64)flagsTypeBIncludeA != 0);
}


template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::MatchFlags(int levelIndexA, int levelIndexB) const
{
	const u32 typeFlagsA = GetInstanceTypeFlags(levelIndexA);
	const u32 includeFlagsA = GetInstanceIncludeFlags(levelIndexA);
	const u32 typeFlagsB = GetInstanceTypeFlags(levelIndexB);
	const u32 includeFlagsB = GetInstanceIncludeFlags(levelIndexB);

	return MatchFlags(typeFlagsA, includeFlagsA, typeFlagsB, includeFlagsB);
}


template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::GetInactiveCollidesAgainstInactive(const int levelIndex) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	return BANK_ONLY(!sm_DisableInactiveCollidesAgainstInactive &&) m_paObjectData[levelIndex].GetCollidesWithInactive();
}


template <class __NodeType> __forceinline u32 phLevelNodeTree<__NodeType>::GetInactiveCollidesAgainstInactiveU32(const int levelIndex) const
{
	FastAssert(LegitLevelIndex(levelIndex));
//	return BANK_ONLY(!sm_DisableInactiveCollidesAgainstInactive &&) m_paObjectData[levelIndex].GetCollidesWithInactive();
	return m_paObjectData[levelIndex].GetCollidesWithInactiveU32();
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::SetInactiveCollidesAgainstInactive(const int levelIndex, bool collides)
{
	FastAssert(LegitLevelIndex(levelIndex));

	if(collides)
	{
		m_paObjectData[levelIndex].SetCollidesWithInactive();
	}
	else
	{
		m_paObjectData[levelIndex].ClearCollidesWithInactive();
	}
}


template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::GetInactiveCollidesAgainstFixed(const int levelIndex) const 
{
	FastAssert(LegitLevelIndex(levelIndex));
	return BANK_ONLY(!sm_DisableInactiveCollidesAgainstFixed &&) m_paObjectData[levelIndex].GetCollidesWithFixed();
}


template <class __NodeType> __forceinline u32 phLevelNodeTree<__NodeType>::GetInactiveCollidesAgainstFixedU32(const int levelIndex) const 
{
	FastAssert(LegitLevelIndex(levelIndex));
//	return BANK_ONLY(!sm_DisableInactiveCollidesAgainstFixed &&) m_paObjectData[levelIndex].GetCollidesWithFixed();
	return m_paObjectData[levelIndex].GetCollidesWithFixedU32();
}


template <class __NodeType> __forceinline void phLevelNodeTree<__NodeType>::SetInactiveCollidesAgainstFixed(const int levelIndex, bool collides)
{
	FastAssert(LegitLevelIndex(levelIndex));

	if(collides)
	{
		m_paObjectData[levelIndex].SetCollidesWithFixed();
	}
	else
	{
		m_paObjectData[levelIndex].ClearCollidesWithFixed();
	}
}



template <class __NodeType> inline bool phLevelNodeTree<__NodeType>::IsInLevel (int levelIndex) const
{
	return (LegitLevelIndex(levelIndex) && GetState(levelIndex)!=OBJECTSTATE_NONEXISTENT);
}

template <class __NodeType> __forceinline phLevelBase::eObjectState phLevelNodeTree<__NodeType>::GetStateLockFree(int levelIndex) const
{
	FastAssert(LegitLevelIndex(levelIndex));
	return m_paObjectData[levelIndex].GetState();
}

template <class __NodeType> inline void phLevelNodeTree<__NodeType>::SetState(const int levelIndex, eObjectState newObjectState)
{
	FastAssert(LegitLevelIndex(levelIndex));
	m_paObjectData[levelIndex].SetState(newObjectState);
}

template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::IsActive(int levelIndex) const
{
	return(GetState(levelIndex) == OBJECTSTATE_ACTIVE);
}

template <class __NodeType> inline bool phLevelNodeTree<__NodeType>::IsInactive(int levelIndex) const
{
	return(GetState(levelIndex) == OBJECTSTATE_INACTIVE);
}

template <class __NodeType> inline bool phLevelNodeTree<__NodeType>::IsFixed(int levelIndex) const
{
	return(GetState(levelIndex) == OBJECTSTATE_FIXED);
}

template <class __NodeType> inline bool phLevelNodeTree<__NodeType>::IsNonexistent(int levelIndex) const
{
	return(GetState(levelIndex) == OBJECTSTATE_NONEXISTENT);
}

template <class __NodeType> inline bool phLevelNodeTree<__NodeType>::CanOctreeBeOutOfSync() const
{
#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	return m_MultipleUpdates || m_DeferredOctreeUpdateEnabled;
#else
	return m_MultipleUpdates;
#endif
}

template <class __NodeType> inline Mat34V_ConstRef phLevelNodeTree<__NodeType>::GetLastInstanceMatrix(const phInst* inst) const
{
	u16 levelIndex = inst->GetLevelIndex();

	if(inst->HasLastMatrix() && IsInLevel(levelIndex) && m_pInstLastMatrixIndices[levelIndex] != INVALID_INST_LAST_MATRIX_INDEX)
	{
		return m_pInstLastMatrices[m_pInstLastMatrixIndices[levelIndex]];
	}

	return inst->GetMatrix();
}

#define MATCH_FLAGS_FOR_PAIRS 0

#if __BANK && MATCH_FLAGS_FOR_PAIRS
extern bool g_MatchFlagsForPairs;
#endif

template <class __NodeType> /*__forceinline */inline bool phLevelNodeTree<__NodeType>::ShouldCollideByState(phLevelBase::eObjectState stateA, phLevelBase::eObjectState stateB, int levelIndexA, int levelIndexB) const
{
#if 1
		// HACK -- This whole function is reliant on specific numerical relationships and values
		// - If the state enumeration, inactive collision enumeration, or packed inst and state flags ever change then this code will probably malfunction
		// -- So here are some compile time asserts to help alert us if these assumptions ever change
		//
		// The nonexistent state must be the largest enumerated value for the 'neitherIsNonExistentMask' calculation to work
		CompileTimeAssert((phLevelBase::OBJECTSTATE_NONEXISTENT+1) == phLevelBase::OBJECTSTATE_CNT);
		// Our current state enumeration to state bit conversion breaks down for values over 2
		CompileTimeAssert(phLevelBase::OBJECTSTATE_NONEXISTENT == 3);
		// The test for override flags against the other object's state relies on these specific bits lining up in this way
		CompileTimeAssert((phLevelBase::COLLISIONSTATE_VS_INACTIVE_BIT>>1) == (((phLevelBase::OBJECTSTATE_INACTIVE+phLevelBase::OBJECTSTATE_INACTIVE+phLevelBase::OBJECTSTATE_INACTIVE)>>1)+1));
		CompileTimeAssert((phLevelBase::COLLISIONSTATE_VS_FIXED_BIT>>1) == (((phLevelBase::OBJECTSTATE_FIXED+phLevelBase::OBJECTSTATE_FIXED+phLevelBase::OBJECTSTATE_FIXED)>>1)+1));

		//((phLevelBase::OBJECTSTATE_ACTIVE + phLevelBase::OBJECTSTATE_ACTIVE + phLevelBase::OBJECTSTATE_ACTIVE) >> 1) + 1;
		const u32 stateActiveBits = 1;

		// Bit of magic to line up the output of the state flags with the state bits we'll generate shortly
		const u32 collideAgainstBitsA = (GetInactiveCollidesAgainstInactiveU32(levelIndexA) | GetInactiveCollidesAgainstFixedU32(levelIndexA)) >> 1;
		const u32 collideAgainstBitsB = (GetInactiveCollidesAgainstInactiveU32(levelIndexB) | GetInactiveCollidesAgainstFixedU32(levelIndexB)) >> 1;

		//const u32 stateBitsA = GetBitFromExistentState(stateA);
		// The real function (rightfully) asserts when our state is non-existent (In that the formula used fails for values over 2)
		// - We don't care because our return value in that case will already be sealed (either object nonexistent forces a false return)
		// The goal here is to take specific enumerated values and map them to specific non-overlapping bits
		//  - In this case we take 0->1, 1->2, and 2->4
		const u32 stateBitsA = ((stateA + stateA + stateA) >> 1) + 1;
		const u32 stateBitsB = ((stateB + stateB + stateB) >> 1) + 1;

		//GenerateMaskEq(stateA, phLevelBase::OBJECTSTATE_ACTIVE) | GenerateMaskEq(stateB, phLevelBase::OBJECTSTATE_ACTIVE);
		const u32 eitherIsActiveMask = GenerateMaskNZ((stateBitsA | stateBitsB) & stateActiveBits);
		
		// This would not work except that the value we are subtracting is the largest enumeration of a small list
		// - So our result is either zero or will share the 'negative' bit
		const u32 neitherIsNonExistentMask = GenerateMaskNZ((stateA - phLevelBase::OBJECTSTATE_NONEXISTENT) & (stateB - phLevelBase::OBJECTSTATE_NONEXISTENT));
		
		// The other two values this will combine with are full masks consisting of all on or all off bits
		// - Therefore this one does not need to be a full mask like that and can simply consist of some or no on bits, the end boolean result will be the same
		const u32 eitherCollidesAgainstOtherBits = /*GenerateMaskNZ(*/(collideAgainstBitsA & stateBitsB) | (collideAgainstBitsB & stateBitsA);

		// The end result is to say that we accept this pair:
		//  IFF (at least one is active OR has override flags that match the other's state) AND neither is set non-existent
		bool retVal = ((neitherIsNonExistentMask & (eitherIsActiveMask | eitherCollidesAgainstOtherBits)) != 0);
#else
		const bool oneIsActiveMask = (stateA == phLevelBase::OBJECTSTATE_ACTIVE) | (stateB == phLevelBase::OBJECTSTATE_ACTIVE);
		const bool oneIsNonExistent = (stateA == phLevelBase::OBJECTSTATE_NONEXISTENT) | (stateB == phLevelBase::OBJECTSTATE_NONEXISTENT);
		const bool alternate0a = GetInactiveCollidesAgainstInactive(levelIndexA) & (stateB == phLevelBase::OBJECTSTATE_INACTIVE);
		const bool alternate0b = GetInactiveCollidesAgainstInactive(levelIndexB) & (stateA == phLevelBase::OBJECTSTATE_INACTIVE);
		const bool alternate1a = GetInactiveCollidesAgainstFixed(levelIndexA) & (stateB == phLevelBase::OBJECTSTATE_FIXED);
		const bool alternate1b = GetInactiveCollidesAgainstFixed(levelIndexB) & (stateA == phLevelBase::OBJECTSTATE_FIXED);

		bool retVal = !oneIsNonExistent & (oneIsActiveMask | alternate0a | alternate0b | alternate1a | alternate1b);
#endif

#if MATCH_FLAGS_FOR_PAIRS
		if (retVal BANK_ONLY(&& g_MatchFlagsForPairs))
		{
				retVal = MatchFlags(levelIndexA, levelIndexB);
		}
#endif // MATCH_FLAGS_FOR_PAIRS

		return retVal;
}


template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::ShouldCollideByState(int levelIndexA, int levelIndexB) const
{
	phLevelBase::eObjectState stateA = GetState(levelIndexA);
	phLevelBase::eObjectState stateB = GetState(levelIndexB);

	return ShouldCollideByState(stateA, stateB, levelIndexA, levelIndexB);
}


template <class __NodeType> /*__forceinline */inline bool phLevelNodeTree<__NodeType>::ShouldCollideByExistentState(phLevelBase::eObjectState stateA, phLevelBase::eObjectState stateB, int levelIndexA, int levelIndexB) const
{
	Assert(stateA != phLevelBase::OBJECTSTATE_NONEXISTENT);
	Assert(stateB != phLevelBase::OBJECTSTATE_NONEXISTENT);

#if 0
	// WIP version that might be improved over what's below.  Wouldn't it be nice if it were this simple though.
	const u32 oneIsActiveMask = GenerateMaskNZ((stateA - phLevelBase::OBJECTSTATE_ACTIVE) | (stateB - phLevelBase::OBJECTSTATE_ACTIVE));

	bool retVal = (oneIsActiveMask != 0);
#else
	const bool oneIsActiveMask = (stateA == phLevelBase::OBJECTSTATE_ACTIVE) | (stateB == phLevelBase::OBJECTSTATE_ACTIVE);
	const bool alternate0a = GetInactiveCollidesAgainstInactive(levelIndexA) & (stateB == phLevelBase::OBJECTSTATE_INACTIVE);
	const bool alternate0b = GetInactiveCollidesAgainstInactive(levelIndexB) & (stateA == phLevelBase::OBJECTSTATE_INACTIVE);
	const bool alternate1a = GetInactiveCollidesAgainstFixed(levelIndexA) & (stateB == phLevelBase::OBJECTSTATE_FIXED);
	const bool alternate1b = GetInactiveCollidesAgainstFixed(levelIndexB) & (stateA == phLevelBase::OBJECTSTATE_FIXED);

	bool retVal = (oneIsActiveMask | alternate0a | alternate0b | alternate1a | alternate1b);
#endif

#if MATCH_FLAGS_FOR_PAIRS
	if (retVal BANK_ONLY(&& g_MatchFlagsForPairs))
	{
		retVal = MatchFlags(levelIndexA, levelIndexB);
	}
#endif // MATCH_FLAGS_FOR_PAIRS

	return retVal;
}


template <class __NodeType> __forceinline bool phLevelNodeTree<__NodeType>::ShouldCollideByExistentState(int levelIndexA, int levelIndexB) const
{
	phLevelBase::eObjectState stateA = GetState(levelIndexA);
	phLevelBase::eObjectState stateB = GetState(levelIndexB);

	return ShouldCollideByExistentState(stateA, stateB, levelIndexA, levelIndexB);
}

} // namespace rage

#endif // ndef PHYSICS_LEVELNEW_H
