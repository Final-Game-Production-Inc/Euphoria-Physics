//
// phbound/cullerhelpers.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef PHBOUND_CULLERHELPERS_H
#define PHBOUND_CULLERHELPERS_H

#include "boundculler.h"
#include "OptimizedBvh.h"
#include "vector/geometry.h"


namespace rage {

class phBvhOverlapCallbackBase
{
public:
	phBvhOverlapCallbackBase(const phOptimizedBvh* bvh) : m_Culler(NULL), m_BVH(bvh) {}
	phBvhOverlapCallbackBase(phBoundCuller* culler, const phOptimizedBvh* bvh) : m_Culler(culler), m_BVH(bvh) {}

	void SetCuller(phBoundCuller* culler)
	{
		m_Culler = culler;
	}

protected:

	static __forceinline void UnpackAABB(const s16 aabbMin[3], const s16 aabbMax[3], Vec3V_In unQuantizeAddDiv2, Vec3V_In unQuantizeMullDiv2, Vec3V_InOut aabbCenter, Vec3V_InOut aabbHalfExtents)
	{
		using namespace Vec;
		// Un-quantize the bvh node aabb, then get the center and half extents
		const Vec3V quantAabbMin = Vec3V(V4IntToFloatRaw<0>(V4UnpackLowSignedShort(V4LoadUnalignedSafe<6>(aabbMin))));
		const Vec3V quantAabbMax = Vec3V(V4IntToFloatRaw<0>(V4UnpackLowSignedShort(V4LoadUnalignedSafe<6>(aabbMax))));

		const Vec3V aabbMinDiv2 = AddScaled(unQuantizeAddDiv2, quantAabbMin, unQuantizeMullDiv2);
		const Vec3V aabbMaxDiv2 = AddScaled(unQuantizeAddDiv2, quantAabbMax, unQuantizeMullDiv2);

		aabbCenter = Add(aabbMaxDiv2, aabbMinDiv2);
		aabbHalfExtents = Subtract(aabbMaxDiv2, aabbMinDiv2);
	}

	static __forceinline bool IsBoxFullyContained(const s16 innerAABBMin[3], const s16 innerAABBMax[3], const s16 outerAABBMin[3], const s16 outerAABBMax[3])
	{
		return	innerAABBMin[0] >= outerAABBMin[0] && 
				innerAABBMax[0] <= outerAABBMax[0] && 
				innerAABBMin[1] >= outerAABBMin[1] && 
				innerAABBMax[1] <= outerAABBMax[1] && 
				innerAABBMin[2] >= outerAABBMin[2] && 
				innerAABBMax[2] <= outerAABBMax[2];
	}

	__forceinline void AddCulledPolygons(const phOptimizedBvhNode* node)
	{
		// TODO: This should probably call AddCulledPolygonIndices() instead because that would be faster.
		for (int polyInNodeIndex = 0; polyInNodeIndex < node->GetPolygonCount(); ++polyInNodeIndex)
		{
			const int curPolygonIndex = node->GetPolygonStartIndex() + polyInNodeIndex;
			m_Culler->AddCulledPolygonIndex((u16)(curPolygonIndex));
		}
	}

	phBoundCuller* m_Culler;
	const phOptimizedBvh* m_BVH;
};

// phBvhLineSegmentOverlapCallback is a class that culls a phOptimizedBvh object against a line segment and puts the returned values into a (user-supplied)
//   phBoundCuller object.
class phBvhLineSegmentOverlapCallback : public phBvhOverlapCallbackBase
{
public:
	explicit phBvhLineSegmentOverlapCallback(const phOptimizedBvh* bvh) : phBvhOverlapCallbackBase(bvh)
	{
		const ScalarV half(V_HALF);
		m_UnQuantizeMulDiv2 = Scale(RCC_VEC3V(m_BVH->GetUnQuantizeMul()), half);
		m_UnQuantizeAddDiv2 = Scale(RCC_VEC3V(m_BVH->GetUnQuantizeAdd()), half);
	}
	phBvhLineSegmentOverlapCallback(phBoundCuller* culler, const phOptimizedBvh* bvh) : phBvhOverlapCallbackBase(culler,bvh)
	{
		const ScalarV half(V_HALF);
		m_UnQuantizeMulDiv2 = Scale(RCC_VEC3V(m_BVH->GetUnQuantizeMul()), half);
		m_UnQuantizeAddDiv2 = Scale(RCC_VEC3V(m_BVH->GetUnQuantizeAdd()), half);
	}

	void SetSegment(Vec3V_In seg0, Vec3V_In seg1)
	{
		geomBoxes::PrecomputeSegmentToBox(&m_PrecomputedSeg, RCC_VECTOR3(seg0), RCC_VECTOR3(seg1));

		// Construct an AABB around this line segment for quick trivial accepts/rejects.
		m_BVH->QuantizeMin(m_AABBMin, VEC3V_TO_VECTOR3(Min(seg0,seg1)));
		m_BVH->QuantizeMax(m_AABBMax, VEC3V_TO_VECTOR3(Max(seg0,seg1)));
	}

	bool TestAgainstSubtree(const s16 aabbMin[3], const s16 aabbMax[3]) const
	{
		if(!TestQuantizedAabbAgainstAabb(m_AABBMin, m_AABBMax, aabbMin, aabbMax))
		{
			return false;
		}

		return IsBoxFullyContained(m_AABBMin,m_AABBMax,aabbMin, aabbMax) || TestLineSegVersusAABB(aabbMin, aabbMax);
	}

	u32 ProcessNode(const phOptimizedBvhNode* node)
	{
		Vec3V aabbCenter, aabbHalfExtents;
		UnpackAABB(node->m_AABBMin,node->m_AABBMax,m_UnQuantizeAddDiv2,m_UnQuantizeMulDiv2,aabbCenter,aabbHalfExtents);
		if (!geomBoxes::TestSegmentToBox(&m_PrecomputedSeg, aabbCenter.GetIntrin128ConstRef(), aabbHalfExtents.GetIntrin128ConstRef()))
		{
			return 0x00000000;
		}

		AddCulledPolygons(node);
		return 0xffffffff;
	}

	bool TestLineSegVersusAABB(const s16 boxMin[3], const s16 boxMax[3]) const
	{
		Vec3V aabbCenter, aabbHalfExtents;
		UnpackAABB(boxMin,boxMax,m_UnQuantizeAddDiv2,m_UnQuantizeMulDiv2,aabbCenter,aabbHalfExtents);
		return !!geomBoxes::TestSegmentToBox(&m_PrecomputedSeg, aabbCenter.GetIntrin128ConstRef(), aabbHalfExtents.GetIntrin128ConstRef());
	}


private:
	Vec3V m_UnQuantizeMulDiv2;
	Vec3V m_UnQuantizeAddDiv2;
	geomBoxes::PrecomputedSegmentToBox m_PrecomputedSeg;
	s16 m_AABBMin[3], m_AABBMax[3];
};

class phBvhCapsuleOverlapCallback : public phBvhOverlapCallbackBase
{
public:
	explicit phBvhCapsuleOverlapCallback(const phOptimizedBvh* bvh) : phBvhOverlapCallbackBase(bvh)
	{
		const ScalarV half(V_HALF);
		m_UnQuantizeMulDiv2 = Scale(RCC_VEC3V(m_BVH->GetUnQuantizeMul()), half);
		m_UnQuantizeAddDiv2 = Scale(RCC_VEC3V(m_BVH->GetUnQuantizeAdd()), half);
	}
	phBvhCapsuleOverlapCallback(phBoundCuller* culler, const phOptimizedBvh* bvh) : phBvhOverlapCallbackBase(culler,bvh)
	{
		const ScalarV half(V_HALF);
		m_UnQuantizeMulDiv2 = Scale(RCC_VEC3V(m_BVH->GetUnQuantizeMul()), half);
		m_UnQuantizeAddDiv2 = Scale(RCC_VEC3V(m_BVH->GetUnQuantizeAdd()), half);
	}

	void SetCapsule(Vec3V_In segmentStart, Vec3V_In segmentEnd, ScalarV_In radius)
	{
		geomBoxes::PrecomputeSegmentToBox(&m_PrecomputedSeg, RCC_VECTOR3(segmentStart), RCC_VECTOR3(segmentEnd));
		m_Radius = Vec3V(radius);
		// Construct an AABB around this capsule for quick trivial accepts/rejects.
		m_BVH->QuantizeMin(m_AABBMin, VEC3V_TO_VECTOR3(Subtract(Min(segmentEnd,segmentStart),Vec3V(radius))));
		m_BVH->QuantizeMax(m_AABBMax, VEC3V_TO_VECTOR3(Add(Max(segmentEnd,segmentStart),Vec3V(radius))));
	}

	bool TestAgainstSubtree(const s16 aabbMin[3], const s16 aabbMax[3]) const
	{
		if(!TestQuantizedAabbAgainstAabb(m_AABBMin, m_AABBMax, aabbMin, aabbMax))
		{
			return false;
		}

		return IsBoxFullyContained(m_AABBMin,m_AABBMax,aabbMin, aabbMax) || TestLineSegVersusAABB(aabbMin, aabbMax);
	}

	u32 ProcessNode(const phOptimizedBvhNode* node)
	{
		Vec3V aabbCenter, aabbHalfExtents;
		UnpackAABB(node->m_AABBMin,node->m_AABBMax,m_UnQuantizeAddDiv2,m_UnQuantizeMulDiv2,aabbCenter,aabbHalfExtents);
		Vec3V aabbHalfExtentsAndRadius = Add(aabbHalfExtents,m_Radius);
		if (!geomBoxes::TestSegmentToBox(&m_PrecomputedSeg, aabbCenter.GetIntrin128ConstRef(), aabbHalfExtentsAndRadius.GetIntrin128ConstRef()))
		{
			return 0x00000000;
		}

		AddCulledPolygons(node);
		return 0xffffffff;
	}

	bool TestLineSegVersusAABB(const s16 boxMin[3], const s16 boxMax[3]) const
	{
		Vec3V aabbCenter, aabbHalfExtents;
		UnpackAABB(boxMin,boxMax,m_UnQuantizeAddDiv2,m_UnQuantizeMulDiv2,aabbCenter,aabbHalfExtents);
		Vec3V aabbHalfExtentsAndRadius = Add(aabbHalfExtents,m_Radius);
		return !!geomBoxes::TestSegmentToBox(&m_PrecomputedSeg, aabbCenter.GetIntrin128ConstRef(), aabbHalfExtentsAndRadius.GetIntrin128ConstRef());
	}

private:
	Vec3V m_Radius;
	Vec3V m_UnQuantizeMulDiv2;
	Vec3V m_UnQuantizeAddDiv2;
	geomBoxes::PrecomputedSegmentToBox m_PrecomputedSeg;
	s16 m_AABBMin[3], m_AABBMax[3];
};

// phBvhAabbOverlapCallback is a class that culls a phOptimizedBvh object against an AABB (aligned according to the axes of the bvh structure) and puts the
//   returned values into a (user-supplied) phBoundCuller object.
class phBvhAabbOverlapCallback : public phBvhOverlapCallbackBase
{
public:
	phBvhAabbOverlapCallback(const phOptimizedBvh* bvh) : phBvhOverlapCallbackBase(bvh) {}
	phBvhAabbOverlapCallback(phBoundCuller* culler, const phOptimizedBvh* bvh) : phBvhOverlapCallbackBase(culler,bvh) {}

	void SetAABB(const Vector3 &aabbMin, const Vector3 &aabbMax)
	{
		m_AABBMin.Set(aabbMin);
		m_AABBMax.Set(aabbMax);
		m_BVH->QuantizeMin(m_AABBMinQuant, aabbMin);
		m_BVH->QuantizeMax(m_AABBMaxQuant, aabbMax);
	}

	static bool TestAabbAgainstAabb2(const rage::s16 boxMin0[3], const rage::s16 boxMax0[3], const rage::s16 boxMin1[3], const rage::s16 boxMax1[3])
	{
		bool xOverlap = (boxMin0[0] <= boxMax1[0]) && (boxMin1[0] <= boxMax0[0]);
		bool yOverlap = (boxMin0[1] <= boxMax1[1]) && (boxMin1[1] <= boxMax0[1]);
		bool zOverlap = (boxMin0[2] <= boxMax1[2]) && (boxMin1[2] <= boxMax0[2]);

		bool aabbOverlap = xOverlap && yOverlap && zOverlap;
		return aabbOverlap;
	}

	bool TestAgainstSubtree(const s16 aabbMin[3], const s16 aabbMax[3])
	{
		return TestAabbAgainstAabb2(aabbMin, aabbMax, m_AABBMinQuant, m_AABBMaxQuant);
	}

	u32 ProcessNode(const phOptimizedBvhNode* node)
	{
		if (TestAabbAgainstAabb2(m_AABBMinQuant, m_AABBMaxQuant, node->m_AABBMin, node->m_AABBMax) == false)
		{
			return 0x00000000;
		}

		AddCulledPolygons(node);
		return 0xffffffff;
	}

private:

	Vector3 m_AABBMin, m_AABBMax;
	s16 m_AABBMinQuant[3], m_AABBMaxQuant[3];
};

}	// namespace rage

#endif	// #ifndef PHBOUND_CULLERHELPERS_H
