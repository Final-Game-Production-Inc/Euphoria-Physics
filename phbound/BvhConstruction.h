#ifndef BVH_CONSTRUCTION_H
#define BVH_CONSTRUCTION_H

#include "OptimizedBvh.h"

namespace rage
{

struct CVec3
{
	s16 v[3];
	__forceinline s16 & operator[](const int i) { return v[i]; }
	__forceinline s16 operator[](const int i) const { return v[i]; }
};

struct BvhConstructionNode
{
	Vec3V m_AABBMin;
	Vec3V m_AABBMax;

	BvhConstructionNode * m_Child[2];

	s16 m_PrimIndex;
	s16 m_PrimCount;

	CVec3 m_CAABBMin;
	CVec3 m_CAABBMax;

	float m_QVolume;
	
	struct CostInfo
	{
		float m_Cost;
		s16 m_Index;
		s32 m_Ext[2];
		s32 m_Ext1[2];
	};

	CostInfo m_BestCost;
};

phOptimizedBvhNode * BuildBvh(phOptimizedBvh * bvhStructure, BvhPrimitiveData *leafNodes, const int PrimNodeCount, const int MinPrimNodeCount, const int targetPrimitivesPerLeaf, const u32 remapPrimitivesMask);

} // namespace rage

#endif //BVH_CONSTRUCTION_H