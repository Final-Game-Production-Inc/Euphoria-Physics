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

#include "OptimizedBvh.h"
#include "BvhConstruction.h"

#include "system/memory.h"
#include "vectormath/scalarv.h"
#include "vectormath/legacyconvert.h"

#include <algorithm>
#include <functional>
#include <limits>

#if !__FINAL
#include "system/timer.h"
#endif	// !__FINAL

#if __PFDRAW
	#include "system/timemgr.h"
#endif

#define USE_BVH_METRICS 0

#if USE_BVH_METRICS
#include <sys/timeb.h>
#endif

#define SPLIT_AT_MEDIAN 1
#define SPLIT_AT_MEAN (0 && !SPLIT_AT_MEDIAN)
#define SPLIT_AT_SEARCH (0 && !SPLIT_AT_MEDIAN && !SPLIT_AT_MEAN)

#define USE_NEW_BVH_CONSTRUCTION __RESOURCECOMPILER

namespace rage
{

#if USE_BVH_METRICS 
struct BvhStats
{
	int m_NodeCount;
	int m_PrimCount;
	int m_Depth;
	float m_QCost;
	float m_PCost;
	float m_PCost1;
};

u64 g_BvhConstructionTicks = 0;
float g_BvhConstructionTimer = 0;
int g_BvhNodeCount = 0;
int g_BvhCount = 0;
float g_totalQCost = 0;
float g_totalPCost = 0;
float g_totalPCost1 = 0;
int g_totalDepth = 0;
int g_PrimCount = 0;
float g_totalQCost_ = 0;
float g_totalPCost_ = 0;
float g_totalPCost1_ = 0;

void BvhDataWrite(const BvhStats & stats, const BvhStats & stats_)
{
	const float totalTime = float(double(g_BvhConstructionTicks) * double(sysTimer::GetTicksToSeconds()));
	static char buf[512];
	static bool first_time_called = true;
	if (first_time_called)
	{
		first_time_called = false;
		__timeb32 timeptr;
		_ftime32_s(&timeptr);
		srand(int(sysTimer::GetTicks()%RAND_MAX));
		sprintf_s(buf,"c:\\temp\\bvh\\%d_%d_%d_%d_%d.txt",timeptr.time,timeptr.millitm,rand(),rand(),rand());
	}

	g_totalQCost += stats.m_QCost;
	g_totalPCost += stats.m_PCost;
	g_totalPCost1 += stats.m_PCost1;
	g_totalDepth += stats.m_Depth;
	g_PrimCount += stats.m_PrimCount;
	g_totalQCost_ += stats_.m_QCost;
	g_totalPCost_ += stats_.m_PCost;
	g_totalPCost1_ += stats_.m_PCost1;

	FILE * file = fopen(buf,"w");
	if (file)
	{
		fprintf(file,"%d %d %d %d %f %f %f %f %f %f %f",g_BvhCount,g_PrimCount,g_BvhNodeCount,g_totalDepth,totalTime,
			g_totalQCost,g_totalPCost,g_totalPCost1,
			g_totalQCost_,g_totalPCost_,g_totalPCost1_);
		fclose(file);
	}
}

__forceinline float CalcQVolume(const phOptimizedBvh * bvhStructure, const s16 AABBMin[3], const s16 AABBMax[3], Vec3V_In q)
{
	Vec3V vBoundingBoxMin, vBoundingBoxMax;
	bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMin), AABBMin);
	bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMax), AABBMax);
	const Vec3V size = vBoundingBoxMax - vBoundingBoxMin + q;
	const ScalarV vol = size.GetX() * size.GetY() * size.GetZ();
	return vol.Getf();
}

float g_rootQVolume = 0;
const phOptimizedBvh * g_bvhStructure1 = NULL;
const Vec3V g_QV(1,1,1);

__forceinline float CalcQProbability(const s16 AABBMin[3], const s16 AABBMax[3])
{
	const float QVolume = CalcQVolume(g_bvhStructure1,AABBMin,AABBMax,g_QV);
	return QVolume / g_rootQVolume;
}

void GetBvhInfoRecurse(const phOptimizedBvhNode * root, BvhStats * stats)
{
	Assert(root);
	const float QProb = CalcQProbability(root->m_AABBMin,root->m_AABBMax);
	if (root->IsLeafNode())
	{
		stats->m_NodeCount = 1;
		stats->m_PrimCount = root->GetPolygonCount();
		stats->m_Depth = 1;
		stats->m_QCost = 0;
		stats->m_PCost = QProb * root->GetPolygonCount();
		if (stats->m_PrimCount > 1)
			stats->m_PCost1 = stats->m_PCost;
		else
			stats->m_PCost1 = 0;
	}
	else
	{
		BvhStats stats0;
		BvhStats stats1;
		const phOptimizedBvhNode * child0 = root + 1;
		const phOptimizedBvhNode * child1 = child0 + child0->GetEscapeIndex();
		GetBvhInfoRecurse(child0,&stats0);
		GetBvhInfoRecurse(child1,&stats1);
		stats->m_NodeCount = stats0.m_NodeCount + stats1.m_NodeCount + 1;
		stats->m_PrimCount = stats0.m_PrimCount + stats1.m_PrimCount;
		stats->m_Depth = Max(stats0.m_Depth,stats1.m_Depth) + 1;
		stats->m_QCost = QProb * 2 + stats0.m_QCost + stats1.m_QCost;
		stats->m_PCost = stats0.m_PCost + stats1.m_PCost;
		stats->m_PCost1 = stats0.m_PCost1 + stats1.m_PCost1;
	}
}

void GetBvhInfo(const phOptimizedBvh * bvhStructure, BvhStats * stats)
{
	g_bvhStructure1 = bvhStructure;
	const phOptimizedBvhNode * root = g_bvhStructure1->GetRootNode();
	g_rootQVolume = CalcQVolume(g_bvhStructure1,root->m_AABBMin,root->m_AABBMax,g_QV);
	GetBvhInfoRecurse(root,stats);
	g_bvhStructure1 = NULL;
	g_rootQVolume = 0;
}

Vec3V g_QueryPosAABBMin;
Vec3V g_QueryPosAABBMax;
float g_QueryPosVolume;

void SetupQueryPos(const s16 AABBMin[3], const s16 AABBMax[3])
{
	//Vec3V vBoundingBoxMin, vBoundingBoxMax;
	g_bvhStructure1->UnQuantize(RC_VECTOR3(g_QueryPosAABBMin), AABBMin);
	g_bvhStructure1->UnQuantize(RC_VECTOR3(g_QueryPosAABBMax), AABBMax);
	g_QueryPosAABBMin -= ScalarV(V_HALF) * g_QV;
	g_QueryPosAABBMax += ScalarV(V_HALF) * g_QV;
	const Vec3V size = g_QueryPosAABBMax - g_QueryPosAABBMin;
	const ScalarV vol = size.GetX() * size.GetY() * size.GetZ();
	g_QueryPosVolume = vol.Getf();
}

__forceinline float CalcBProbability(const s16 AABBMin[3], const s16 AABBMax[3])
{
	Vec3V vBoundingBoxMin, vBoundingBoxMax;
	g_bvhStructure1->UnQuantize(RC_VECTOR3(vBoundingBoxMin), AABBMin);
	g_bvhStructure1->UnQuantize(RC_VECTOR3(vBoundingBoxMax), AABBMax);
	vBoundingBoxMin -= ScalarV(V_HALF) * g_QV;
	vBoundingBoxMax += ScalarV(V_HALF) * g_QV;

	if (IsGreaterThanOrEqualAll(vBoundingBoxMax,g_QueryPosAABBMin) && IsLessThanOrEqualAll(vBoundingBoxMin,g_QueryPosAABBMax))
	{
		const Vec3V mn = Max(vBoundingBoxMin,g_QueryPosAABBMin);
		const Vec3V mx = Min(vBoundingBoxMax,g_QueryPosAABBMax);
		const Vec3V size = mx - mn;
		const ScalarV volV = size.GetX() * size.GetY() * size.GetZ();
		const float vol = volV.Getf();
		Assert(vol >= 0);
		return vol / g_QueryPosVolume;
	}
	else
		return 0;
}

void GetBvhInfo1Recurse(const phOptimizedBvhNode * root, BvhStats * stats)
{
	const float BProb = CalcBProbability(root->m_AABBMin,root->m_AABBMax);
	if (BProb == 0)
	{
		stats->m_NodeCount = 0;
		stats->m_PrimCount = 0;
		stats->m_Depth = 0;
		stats->m_QCost = 0;
		stats->m_PCost = 0;
		stats->m_PCost1 = 0;
		return;
	}
	if (root->IsLeafNode())
	{
		stats->m_NodeCount = 1;
		stats->m_PrimCount = root->GetPolygonCount();
		stats->m_Depth = 1;
		stats->m_QCost = 0;
		stats->m_PCost = BProb * root->GetPolygonCount();
		if (stats->m_PrimCount > 1)
			stats->m_PCost1 = stats->m_PCost;
		else
			stats->m_PCost1 = 0;
	}
	else
	{
		BvhStats stats0;
		BvhStats stats1;
		const phOptimizedBvhNode * child0 = root + 1;
		const phOptimizedBvhNode * child1 = child0 + child0->GetEscapeIndex();
		GetBvhInfo1Recurse(child0,&stats0);
		GetBvhInfo1Recurse(child1,&stats1);
		stats->m_NodeCount = stats0.m_NodeCount + stats1.m_NodeCount + 1;
		stats->m_PrimCount = stats0.m_PrimCount + stats1.m_PrimCount;
		stats->m_Depth = Max(stats0.m_Depth,stats1.m_Depth) + 1;
		stats->m_QCost = BProb * 2 + stats0.m_QCost + stats1.m_QCost;
		stats->m_PCost = stats0.m_PCost + stats1.m_PCost;
		stats->m_PCost1 = stats0.m_PCost1 + stats1.m_PCost1;
	}
}

void GetBvhInfo1(const phOptimizedBvh * bvhStructure, BvhStats * stats)
{
	g_bvhStructure1 = bvhStructure;
	const phOptimizedBvhNode * root = g_bvhStructure1->GetRootNode();

	stats->m_NodeCount = 0;
	stats->m_PrimCount = 0;
	stats->m_Depth = 0;
	stats->m_QCost = 0;
	stats->m_PCost = 0;
	stats->m_PCost1 = 0;

	float totalVolume = 0;
	for (int node_i = 0 ; node_i < g_bvhStructure1->GetNumNodes() ; node_i++)
	{
		phOptimizedBvhNode * node = g_bvhStructure1->GetRootNode() + node_i;
		if (node->IsLeafNode())
		{
			SetupQueryPos(node->m_AABBMin,node->m_AABBMax);
			BvhStats stats_;
			GetBvhInfo1Recurse(root,&stats_);
			stats->m_QCost += g_QueryPosVolume * stats_.m_QCost;
			stats->m_PCost += g_QueryPosVolume * stats_.m_PCost;
			stats->m_PCost1 += g_QueryPosVolume * stats_.m_PCost1;
			totalVolume += g_QueryPosVolume;
		}
	}
	Assert(totalVolume > 0);
	stats->m_QCost /= totalVolume;
	stats->m_PCost /= totalVolume;
	stats->m_PCost1 /= totalVolume;

	g_bvhStructure1 = NULL;
}
#endif // USE_BVH_METRICS

phOptimizedBvhNode::phOptimizedBvhNode(datResource & UNUSED_PARAM(rsc))
{
	// No pointers or anything to fix up.
}

IMPLEMENT_PLACE(phOptimizedBvhNode);

#if __DECLARESTRUCT
void phOptimizedBvhNode::DeclareStruct(datTypeStruct &s)
{
	STRUCT_BEGIN(phOptimizedBvhNode);
	STRUCT_CONTAINED_ARRAY(m_AABBMin);
	STRUCT_CONTAINED_ARRAY(m_AABBMax);
	STRUCT_FIELD(m_NodeData);
	STRUCT_FIELD(m_PolygonCount);
	STRUCT_END();
}
#endif // __DECLARESTRUCT


phOptimizedBvh::phOptimizedBvh() : m_contiguousNodes(NULL), m_numNodes(0) 
{
	m_AABBMin.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	m_AABBMax.Set(FLT_MAX, FLT_MAX, FLT_MAX);
	m_SubtreeHeaders = NULL;
	m_NumSubtreeHeaders = 0;
}


phOptimizedBvh::~phOptimizedBvh()
{
	if(m_SubtreeHeaders != NULL)
	{
		delete [] m_SubtreeHeaders;
		m_SubtreeHeaders = NULL;
		m_NumSubtreeHeaders = 0;
	}
	Assert(m_NumSubtreeHeaders == 0);
	Assert(m_SubtreeHeaders == NULL);

	if (m_contiguousNodes)
	{
		delete []m_contiguousNodes;
	}
}


phOptimizedBvh::phOptimizedBvh(datResource &rsc)
{
	// Do any pointer fixups needed.
	rsc.PointerFixup(m_contiguousNodes);
	rsc.PointerFixup(m_SubtreeHeaders);
}

IMPLEMENT_PLACE(phOptimizedBvh);


#if !__FINAL
float phOptimizedBvh::TestBvhBuildPerformance(int numPrimitives, int numTests)
{
	const Vec3V bvhMin(-100.0, -100.0f, -100.0f);
	const Vec3V bvhMax(+100.0, +100.0f, +100.0f);
	phOptimizedBvh *tempBVH = rage_new phOptimizedBvh;
	tempBVH->SetExtents(RCC_VECTOR3(bvhMin), RCC_VECTOR3(bvhMax));

	BvhPrimitiveData *primitives = Alloca(BvhPrimitiveData, numPrimitives);
	for(int i = 0; i < numPrimitives; ++i)
	{
		for(int axis = 0; axis < 3; ++axis)
		{
			const u16 extent0 = (u16)(i * 7919 + axis * 569 + 5);
			const u16 extent1 = (u16)(i * 5323 + axis * 883 + 23);
			primitives[i].m_AABBMin[axis] = Min(extent0, extent1);
			primitives[i].m_AABBMax[axis] = Max(extent0, extent1);
		}
		primitives[i].m_PrimitiveIndex = (phPolygon::Index)(i);
	}

	sysTimer timer;
	tempBVH->BuildFromPrimitiveData(primitives, numPrimitives, numPrimitives, NULL, 1, false);
	for(int i = 0; i < numTests; ++i)
	{
		tempBVH->BuildFromPrimitiveDataNoAllocate(primitives, numPrimitives, INVALID_MIN_NODE_COUNT, NULL, 1, false);
	}
	float totalTime = timer.GetMsTime();

	delete tempBVH;

	return totalTime;
}
#endif	// !__FINAL


#if __DECLARESTRUCT
void phOptimizedBvh::DeclareStruct(datTypeStruct &s)
{
#if __RESOURCECOMPILER
	const u32 maxNumNodes = (128 * g_rscVirtualLeafSize) / sizeof(phOptimizedBvhNode);
	if((u32)m_numNodes > maxNumNodes)
	{
		Warningf("BVH structure has %d nodes.  BVH structures with more than %d will not resource successfully.  Please reduce the number of primitives or increase the primitives per node when building.", m_numNodes, maxNumNodes);
	}
#endif	// __RESOURCECOMPILER
	STRUCT_BEGIN(phOptimizedBvh);
	STRUCT_DYNAMIC_ARRAY(m_contiguousNodes, m_numNodes);
	STRUCT_FIELD(m_NumNodesInUse);
	STRUCT_FIELD(m_numNodes);
	STRUCT_CONTAINED_ARRAY(m_Pad0);
	STRUCT_FIELD(m_AABBMin);
	STRUCT_FIELD(m_AABBMax);
	STRUCT_FIELD(m_AABBCenter);
	STRUCT_FIELD(m_Quantize);
	STRUCT_FIELD(m_InvQuantize);
	STRUCT_DYNAMIC_ARRAY(m_SubtreeHeaders, m_NumSubtreeHeaders);
	STRUCT_FIELD(m_NumSubtreeHeaders);
	STRUCT_FIELD(m_CurSubtreeHeaderIndex);
	STRUCT_END();
}
#endif // __DECLARESTRUCT


void phOptimizedBvh::BuildFromPrimitiveData(BvhPrimitiveData *primNodes, int usedPrimNodeCount, int maxPrimNodeCount, phPolygon::Index * const newToOldPolygonIndexMapping, int targetPrimitivesPerNode, bool remapPrimitives)
{
	Assertf(targetPrimitivesPerNode > 0, "Can't build a BVH with %d primitives in each node.  Must be a positive value.", targetPrimitivesPerNode);
	Assertf(remapPrimitives || targetPrimitivesPerNode == 1, "Can't build a BVH with > 0 primitives per node without being able to re-map the primitives.");
	Assertf(maxPrimNodeCount >= usedPrimNodeCount, "Can't build BVH if you don't want to allocate enough to hold the current number of nodes.");
	Assert(usedPrimNodeCount > 0);

	if(usedPrimNodeCount == 0)
	{
		m_numNodes = 0;
		m_NumNodesInUse = 0;
		m_NumSubtreeHeaders = 0;
		m_CurSubtreeHeaderIndex = 0;
		m_contiguousNodes = NULL;
		m_SubtreeHeaders = NULL;
		return;
	}

#if USE_NEW_BVH_CONSTRUCTION
	// The maximum number of nodes a binary tree with N primitives can have is (2*N-1).
	m_numNodes = 2 * usedPrimNodeCount - 1;		
	Assert(m_contiguousNodes == NULL);
	const int minPrimNodeCount = (2 * maxPrimNodeCount) / targetPrimitivesPerNode + 1;	// Same formula as for m_numNodes in the old BVH construction.
	// Don't allocate nodes here. Nodes are allocated in buildTreeFromBvhConstruction.
#else
	// Now we have an array of leaf nodes called ... leafNodes.  Let's create the tree.
	// These calculations aren't 100% optimal - sometimes they allocate one (or two?) more nodes than we actually need.
	m_numNodes = (2 * maxPrimNodeCount) / targetPrimitivesPerNode + 1;
	Assert(m_contiguousNodes == NULL);
	m_contiguousNodes = rage_new phOptimizedBvhNode[m_numNodes];
	const int minPrimNodeCount = INVALID_MIN_NODE_COUNT;
#endif
	m_NumNodesInUse = 0;

	m_NumSubtreeHeaders = 0;
	Assert(m_SubtreeHeaders == NULL);
	m_CurSubtreeHeaderIndex = 0;

	// Allocate some subtree info objects.  We avoid creating this in the resource heap because we don't actually know how many we're going to need
	//   until we're done building the tree.  We'll create a 'real' array of these guys once we know exactly how many we're going to need.
	m_NumSubtreeHeaders = (u16)(Max(m_numNodes >> 1, 1));
	sysMemStartTemp();
	m_SubtreeHeaders = rage_new phOptimizedBvh::phBVHSubtreeInfo[m_NumSubtreeHeaders];
	sysMemEndTemp();

	BuildFromPrimitiveDataNoAllocate(primNodes, usedPrimNodeCount, minPrimNodeCount, newToOldPolygonIndexMapping, targetPrimitivesPerNode, remapPrimitives);

	// Now that we know exactly how many we're going to need, 
	Assert(m_CurSubtreeHeaderIndex > 0);
	m_NumSubtreeHeaders = m_CurSubtreeHeaderIndex;
	phOptimizedBvh::phBVHSubtreeInfo *realSubtreeHeaders = rage_new phOptimizedBvh::phBVHSubtreeInfo[m_NumSubtreeHeaders];
	for(int subtreeHeaderIndex = 0; subtreeHeaderIndex < m_NumSubtreeHeaders; ++subtreeHeaderIndex)
	{
		realSubtreeHeaders[subtreeHeaderIndex].Clone(m_SubtreeHeaders[subtreeHeaderIndex]);
	}
	sysMemStartTemp();
	delete [] m_SubtreeHeaders;
	sysMemEndTemp();
	m_SubtreeHeaders = realSubtreeHeaders;
}


void phOptimizedBvh::BuildFromPrimitiveDataNoAllocate(BvhPrimitiveData *primNodes, int primNodeCount, int minPrimNodeCount, phPolygon::Index * const newToOldPolygonIndexMapping, int targetPrimitivesPerNode, bool remapPrimitives)
{
	Assertf(targetPrimitivesPerNode > 0, "Can't build a BVH with %d primitives in each node.  Must be a positive value.", targetPrimitivesPerNode);
	Assertf(remapPrimitives || targetPrimitivesPerNode == 1, "Can't build a BVH with > 0 primitives per node without being able to re-map the primitives.");
	Assertf((targetPrimitivesPerNode & (targetPrimitivesPerNode - 1)) == 0, "Target primitives per node (%d) must be a power of two.", targetPrimitivesPerNode);
	Assert(primNodeCount > 0);

	m_NumNodesInUse = 0;
	m_CurSubtreeHeaderIndex = 0;
#if USE_NEW_BVH_CONSTRUCTION
	Assert(m_contiguousNodes == NULL);
#else
	Assert(m_contiguousNodes != NULL);
#endif
	Assert(m_SubtreeHeaders != NULL);

#if __SPU
	CompileTimeAssert(!USE_NEW_BVH_CONSTRUCTION);
	Assertf(m_NumSubtreeHeaders == 1, "BVH is too large to be rebuilt on the SPU: %i nodes, %i subtrees", m_numNodes, m_NumSubtreeHeaders);
	// Get the subtree headers and nodes into SPU memory
	phOptimizedBvhNode*	ppuContiguousNodes = m_contiguousNodes;
	phBVHSubtreeInfo* ppuSubtreeHeaders = m_SubtreeHeaders;

	u8 subtreeHeaderBuffer[sizeof(phBVHSubtreeInfo)] ;
	m_contiguousNodes = Alloca(phOptimizedBvhNode, m_numNodes);
	m_SubtreeHeaders = reinterpret_cast<phBVHSubtreeInfo*>(subtreeHeaderBuffer);
#endif

#if USE_NEW_BVH_CONSTRUCTION
	// Nodes are initialized in buildTreeFromBvhConstruction.
#else
	// Reset all of the nodes so that they don't have any lingering data in them.  In particular we need to have the polygon counts reset to zero.
	const int numAllocatedNodes = m_numNodes;
	for(int nodeIndex = 0; nodeIndex < numAllocatedNodes; ++nodeIndex)
	{
		m_contiguousNodes[nodeIndex].Clear();
	}
#endif

	const u32 remapPrimitivesMask = (u32)(-1) * (u32)(remapPrimitives);
	if(primNodeCount > 0)
	{
#if USE_BVH_METRICS
		u64 BvhConstructionTicks = sysTimer::GetTicks();
#endif

#if USE_NEW_BVH_CONSTRUCTION
		BuildBvh(this,primNodes,primNodeCount,minPrimNodeCount,targetPrimitivesPerNode,remapPrimitivesMask);
#else
		Assert(minPrimNodeCount == INVALID_MIN_NODE_COUNT);
		(void)minPrimNodeCount;
		buildTree(primNodes, 0, primNodeCount, targetPrimitivesPerNode, remapPrimitivesMask, -1);
#endif

#if USE_BVH_METRICS
		BvhConstructionTicks = sysTimer::GetTicks() - BvhConstructionTicks;
		g_BvhConstructionTimer = float(BvhConstructionTicks) * sysTimer::GetTicksToSeconds();
		g_BvhConstructionTicks += BvhConstructionTicks;
		g_BvhNodeCount += m_NumNodesInUse;
		g_BvhCount++;
		BvhStats stats;
		GetBvhInfo(this,&stats);
		BvhStats stats1;
		GetBvhInfo1(this,&stats1);
		BvhDataWrite(stats,stats1);
#endif // USE_BVH_METRICS

		if(newToOldPolygonIndexMapping != NULL)
		{
			for(int polygonIndex = primNodeCount - 1; polygonIndex >= 0; --polygonIndex)
			{
				newToOldPolygonIndexMapping[polygonIndex] = primNodes[polygonIndex].m_PrimitiveIndex;
			}
		}

#if 0
		{
			// This is just a bit of code to verify that each polygon appears exactly once in the tree.
			bool *polyFound = Alloca(bool, primNodeCount);
			for(int curPolygonIndex = 0; curPolygonIndex < primNodeCount; ++curPolygonIndex)
			{
				polyFound[curPolygonIndex] = false;
			}

			for(int curNodeIndex = 0; curNodeIndex < m_NumNodesInUse; ++curNodeIndex)
			{
				const phOptimizedBvhNode &curNode = m_contiguousNodes[curNodeIndex];
				if(curNode.IsLeafNode())
				{
					for(int polyInNodeIndex = 0; polyInNodeIndex < curNode.GetPolygonCount(); ++polyInNodeIndex)
					{
						Assert(!polyFound[curNode.GetPolygonStartIndex() + polyInNodeIndex]);
						polyFound[curNode.GetPolygonStartIndex() + polyInNodeIndex] = true;
					}
				}
			}
		}
#endif

		// We had better either have created some subtree headers or had a tree that's so small that it doesn't require any.
		Assert((m_CurSubtreeHeaderIndex != 0) != (m_contiguousNodes[0].GetEscapeIndex() * sizeof(phOptimizedBvhNode) <= MAX_SUBTREE_SIZE));
		if(m_CurSubtreeHeaderIndex == 0)
		{
			m_SubtreeHeaders[0].SetAABBFromNode(m_contiguousNodes[0]);
			m_SubtreeHeaders[0].m_RootNodeIndex = 0;
			m_SubtreeHeaders[0].m_LastIndex = (u16)(m_NumNodesInUse);
			m_CurSubtreeHeaderIndex = 1;
		}
	}

#if __SPU
	Assertf(m_NumSubtreeHeaders == 1, "BVH grew too large after being rebuilt on the SPU: %i nodes, %i subtrees", m_numNodes, m_NumSubtreeHeaders);
	// Set the PPU subtree headers and nodes to their updated SPU counterparts
	cellDmaLargePut(m_contiguousNodes, (uint64_t)(ppuContiguousNodes), sizeof(phOptimizedBvhNode) * m_numNodes, DMA_TAG(0), 0, 0);
	cellDmaLargePut(m_SubtreeHeaders, (uint64_t)(ppuSubtreeHeaders), sizeof(phBVHSubtreeInfo), DMA_TAG(0), 0, 0);

	m_contiguousNodes = ppuContiguousNodes;
	m_SubtreeHeaders = ppuSubtreeHeaders;

	cellDmaWaitTagStatusAll(DMA_MASK(0));
#endif
}


void phOptimizedBvh::AllocateAndCopyFrom(const phOptimizedBvh &bvhStructureToCopy)
{
	phOptimizedBvh * RESTRICT thisBvhStructure = this;
	const phOptimizedBvh * RESTRICT otherBvhStructure = &bvhStructureToCopy;

	thisBvhStructure->m_AABBMin = otherBvhStructure->m_AABBMin;
	thisBvhStructure->m_AABBMax = otherBvhStructure->m_AABBMax;
	thisBvhStructure->m_AABBCenter = otherBvhStructure->m_AABBCenter;
	thisBvhStructure->m_Quantize = otherBvhStructure->m_Quantize;
	thisBvhStructure->m_InvQuantize = otherBvhStructure->m_InvQuantize;

	const int numNodesAllocated = otherBvhStructure->m_numNodes;
	const int numNodesInUse = otherBvhStructure->m_NumNodesInUse;
	const int numSubtreeHeadersAllocated = otherBvhStructure->m_NumSubtreeHeaders;
	const int numSubtreeHeaderInUse = otherBvhStructure->m_CurSubtreeHeaderIndex;
	Assert(numSubtreeHeadersAllocated == numSubtreeHeaderInUse);

	const phOptimizedBvhNode * RESTRICT otherBvhNodes = otherBvhStructure->m_contiguousNodes;
	const phBVHSubtreeInfo * RESTRICT otherSubtreeHeaders = otherBvhStructure->m_SubtreeHeaders;

	Assert(m_contiguousNodes == NULL);
	phOptimizedBvhNode * RESTRICT thisBvhNodes = rage_new phOptimizedBvhNode[numNodesAllocated];
	thisBvhStructure->m_contiguousNodes = thisBvhNodes;
	thisBvhStructure->m_numNodes = numNodesAllocated;
	thisBvhStructure->m_NumNodesInUse = numNodesInUse;

	Assert(m_SubtreeHeaders == NULL);
	phBVHSubtreeInfo * RESTRICT thisSubtreeHeaders = rage_new phBVHSubtreeInfo[numSubtreeHeadersAllocated];
	thisBvhStructure->m_SubtreeHeaders = thisSubtreeHeaders;
	thisBvhStructure->m_NumSubtreeHeaders = static_cast<u16>(numSubtreeHeadersAllocated);
	thisBvhStructure->m_CurSubtreeHeaderIndex = static_cast<u16>(numSubtreeHeaderInUse);

	// Now copy the data over.
	for(int nodeIndex = 0; nodeIndex < numNodesInUse; ++nodeIndex)
	{
		thisBvhNodes[nodeIndex].Clone(otherBvhNodes[nodeIndex]);
	}


	for(int subtreeHeaderIndex = 0; subtreeHeaderIndex < numSubtreeHeaderInUse; ++subtreeHeaderIndex)
	{
		thisSubtreeHeaders[subtreeHeaderIndex].Clone(otherSubtreeHeaders[subtreeHeaderIndex]);
	}
}


// This is split out into its own function simply because otherwise, according to PIX, we get a LHS...register spilling I guess
void ComputeMinAndMax(const BvhPrimitiveData *leafNodes, int startIndex, int endIndex, phOptimizedBvhNode& bvhNode)
{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Set the bounding box of bvhNode to cover leafNodes[startIndex .. endIndex - 1].
	const BvhPrimitiveData &firstPrimitiveData = leafNodes[startIndex];
	s32 aabbMin0 = firstPrimitiveData.m_AABBMin[0];
	s32 aabbMin1 = firstPrimitiveData.m_AABBMin[1];
	s32 aabbMin2 = firstPrimitiveData.m_AABBMin[2];
	s32 aabbMax0 = firstPrimitiveData.m_AABBMax[0];
	s32 aabbMax1 = firstPrimitiveData.m_AABBMax[1];
	s32 aabbMax2 = firstPrimitiveData.m_AABBMax[2];
	for(int curPolygonIndex = startIndex + 1; curPolygonIndex < endIndex; ++curPolygonIndex)
	{
		const BvhPrimitiveData &curPolygonData = leafNodes[curPolygonIndex];
		aabbMin0 = Min(aabbMin0, (s32)curPolygonData.m_AABBMin[0]);
		aabbMin1 = Min(aabbMin1, (s32)curPolygonData.m_AABBMin[1]);
		aabbMin2 = Min(aabbMin2, (s32)curPolygonData.m_AABBMin[2]);

		aabbMax0 = Max(aabbMax0, (s32)curPolygonData.m_AABBMax[0]);
		aabbMax1 = Max(aabbMax1, (s32)curPolygonData.m_AABBMax[1]);
		aabbMax2 = Max(aabbMax2, (s32)curPolygonData.m_AABBMax[2]);
	}
	bvhNode.SetAABB((s16)aabbMin0, (s16)aabbMin1, (s16)aabbMin2, (s16)aabbMax0, (s16)aabbMax1, (s16)aabbMax2);
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


// BuildTree subdivides and sort indices [startIndex, endIndex) out of the array leafNodes and creates nodes in the tree.  This is a 'top-down'
//   creation process.
phOptimizedBvhNode*	phOptimizedBvh::buildTree(BvhPrimitiveData *leafNodes, int startIndex, int endIndex, int targetPrimitivesPerNode, u32 remapPrimitivesMask, int lastSortAxis)
{
	// If this assert fails the node array for the tree wasn't created big enough.  The splitting algorithm should guarantee that the minimum number
	//   of nodes is always used, so either the splitting algorithm isn't working correctly or the calculation of the minimum number of nodes required
	//   is broken (or possibly, of course, both).
	Assert(m_NumNodesInUse < m_numNodes);
	TrapGE(m_NumNodesInUse, m_numNodes);
	int curIndex = m_NumNodesInUse;

	// Get the node that we're going to create as a result of this buildTree call.  We're either going to stuff it with polygons from indices
	//   [startIndex, endIndex) from 'leafNodes' or we're going to subdivide that set of polygons.
	phOptimizedBvhNode &bvhNode = m_contiguousNodes[m_NumNodesInUse++];

	int numIndices = endIndex - startIndex;
	Assert(numIndices > 0);
	if(numIndices <= targetPrimitivesPerNode)
	{
		ComputeMinAndMax(leafNodes, startIndex, endIndex, bvhNode);
		bvhNode.SetPolygonCount((u8)(numIndices));
		// NOTE: If we are remapping the primitives then startIndex is not the index of the polygon as the polygons are currently organized.  It's the index that
		//   the polygon is going to have *after* we've re-ordered the polygons.
		const u32 newPolygonStartIndex = (remapPrimitivesMask & startIndex) | (~remapPrimitivesMask & (u32)(leafNodes[startIndex].m_PrimitiveIndex));
		bvhNode.SetPolygonStartIndex((u16)(newPolygonStartIndex));

		return &bvhNode;
	}

	//calculate Best Splitting Axis and where to split it. Sort the incoming 'leafNodes' array within range [startIndex, endIndex).
	int secondAxis;
	int splitAxis = calcSplittingAxis(leafNodes,startIndex,endIndex,secondAxis);

	// Sort primitives according to the center of the AABB's component along splitAxis, and determine the node at which we should split.
	if(splitAxis != lastSortAxis)
	{
		SortPrimitivesAlongAxes(leafNodes, startIndex, endIndex, splitAxis, secondAxis);
	}
	int splitIndex = CalcSplittingIndex(startIndex, endIndex, targetPrimitivesPerNode);

	// Build the sub-trees for our left and right children.
	const phOptimizedBvhNode *leftChild = buildTree(leafNodes, startIndex, splitIndex, targetPrimitivesPerNode, remapPrimitivesMask, splitAxis);
	const phOptimizedBvhNode *rightChild = buildTree(leafNodes, splitIndex, endIndex, targetPrimitivesPerNode, remapPrimitivesMask, splitAxis);

	bvhNode.SetEscapeIndex((u16)(m_NumNodesInUse - curIndex));

	// The escape index tells us by how many nodes to skip ahead, thereby telling us the size of the subtree.
	const int leftSubTreeNodeMaxCount = leftChild->GetEscapeIndex();
	const int rightSubTreeNodeMaxCount = rightChild->GetEscapeIndex();
	const int leftSubTreeSize = leftSubTreeNodeMaxCount * sizeof(phOptimizedBvhNode);
	const int rightSubTreeSize = rightSubTreeNodeMaxCount * sizeof(phOptimizedBvhNode);
	if(leftSubTreeSize + rightSubTreeSize >= MAX_SUBTREE_SIZE)
	{
		if(leftSubTreeSize <= MAX_SUBTREE_SIZE)
		{
			phBVHSubtreeInfo &curInfo = m_SubtreeHeaders[m_CurSubtreeHeaderIndex];
			++m_CurSubtreeHeaderIndex;

			curInfo.SetAABBFromNode(*leftChild);
			curInfo.m_RootNodeIndex = (u16)(leftChild - m_contiguousNodes);
			curInfo.m_LastIndex = (u16)((u32)curInfo.m_RootNodeIndex + leftSubTreeNodeMaxCount);
		}

		if(rightSubTreeSize <= MAX_SUBTREE_SIZE)
		{
			phBVHSubtreeInfo &curInfo = m_SubtreeHeaders[m_CurSubtreeHeaderIndex];
			++m_CurSubtreeHeaderIndex;

			curInfo.SetAABBFromNode(*rightChild);
			curInfo.m_RootNodeIndex = (u16)(rightChild - m_contiguousNodes);
			curInfo.m_LastIndex = (u16)((u32)curInfo.m_RootNodeIndex + rightSubTreeNodeMaxCount);
		}
	}

	bvhNode.CombineAABBs(*leftChild, *rightChild);

	return &bvhNode;
}

void WriteLeafPrims(const BvhConstructionNode * root, phOptimizedBvh::BuildTreeInfo * info)
{
	if (root->m_Child[0])
	{
		Assert(root->m_Child[1]);
		WriteLeafPrims(root->m_Child[0],info);
		WriteLeafPrims(root->m_Child[1],info);
	}
	else
	{
		Assert(root->m_PrimCount == 1);
		Assert(info->primCount < info->maxPrimCount);
		info->leafNodes[info->primCount] = info->tempLeafNodes[root->m_PrimIndex];
		info->primCount++;
	}
}

phOptimizedBvhNode*	phOptimizedBvh::buildTreeFromBvhConstructionRecurse(const BvhConstructionNode * root, BuildTreeInfo * info)
{
	// If this assert fails the node array for the tree wasn't created big enough.  The splitting algorithm should guarantee that the minimum number
	//   of nodes is always used, so either the splitting algorithm isn't working correctly or the calculation of the minimum number of nodes required
	//   is broken (or possibly, of course, both).
	Assert(m_NumNodesInUse < m_numNodes);
	TrapGE(m_NumNodesInUse, m_numNodes);
	const int curIndex = m_NumNodesInUse;

	// Get the node that we're going to create as a result of this buildTree call.  We're either going to stuff it with polygons from indices
	//   [startIndex, endIndex) from 'leafNodes' or we're going to subdivide that set of polygons.
	phOptimizedBvhNode &bvhNode = m_contiguousNodes[m_NumNodesInUse++];

	if (root->m_PrimCount <= info->targetPrimitivesPerLeaf)
	{
		const int startIndex = info->primCount;
		WriteLeafPrims(root,info);
		const int endIndex= info->primCount;
		const int numIndices = endIndex - startIndex;
		Assert(numIndices > 0);
		ComputeMinAndMax(info->leafNodes, startIndex, endIndex, bvhNode);
		bvhNode.SetPolygonCount((u8)(numIndices));
		// NOTE: If we are remapping the primitives then startIndex is not the index of the polygon as the polygons are currently organized.  It's the index that
		//   the polygon is going to have *after* we've re-ordered the polygons.
		const u32 newPolygonStartIndex = (info->remapPrimitivesMask & startIndex) | (~info->remapPrimitivesMask & (u32)(info->leafNodes[startIndex].m_PrimitiveIndex));
		bvhNode.SetPolygonStartIndex((u16)(newPolygonStartIndex));
		return &bvhNode;
	}

	// Build the sub-trees for our left and right children.
	Assert(root->m_Child[0]);
	Assert(root->m_Child[1]);
	const phOptimizedBvhNode *leftChild = buildTreeFromBvhConstructionRecurse(root->m_Child[0],info);
	const phOptimizedBvhNode *rightChild = buildTreeFromBvhConstructionRecurse(root->m_Child[1],info);

	bvhNode.SetEscapeIndex((u16)(m_NumNodesInUse - curIndex));

	// The escape index tells us by how many nodes to skip ahead, thereby telling us the size of the subtree.
	const int leftSubTreeNodeMaxCount = leftChild->GetEscapeIndex();
	const int rightSubTreeNodeMaxCount = rightChild->GetEscapeIndex();
	const int leftSubTreeSize = leftSubTreeNodeMaxCount * sizeof(phOptimizedBvhNode);
	const int rightSubTreeSize = rightSubTreeNodeMaxCount * sizeof(phOptimizedBvhNode);
	if(leftSubTreeSize + rightSubTreeSize >= MAX_SUBTREE_SIZE)
	{
		if(leftSubTreeSize <= MAX_SUBTREE_SIZE)
		{
			phBVHSubtreeInfo &curInfo = m_SubtreeHeaders[m_CurSubtreeHeaderIndex];
			++m_CurSubtreeHeaderIndex;

			curInfo.SetAABBFromNode(*leftChild);
			curInfo.m_RootNodeIndex = (u16)(leftChild - m_contiguousNodes);
			curInfo.m_LastIndex = (u16)((u32)curInfo.m_RootNodeIndex + leftSubTreeNodeMaxCount);
		}

		if(rightSubTreeSize <= MAX_SUBTREE_SIZE)
		{
			phBVHSubtreeInfo &curInfo = m_SubtreeHeaders[m_CurSubtreeHeaderIndex];
			++m_CurSubtreeHeaderIndex;

			curInfo.SetAABBFromNode(*rightChild);
			curInfo.m_RootNodeIndex = (u16)(rightChild - m_contiguousNodes);
			curInfo.m_LastIndex = (u16)((u32)curInfo.m_RootNodeIndex + rightSubTreeNodeMaxCount);
		}
	}

	bvhNode.CombineAABBs(*leftChild, *rightChild);

	return &bvhNode;
}

phOptimizedBvhNode*	phOptimizedBvh::buildTreeFromBvhConstruction(const BvhConstructionNode * root, BuildTreeInfo * info)
{
	// Allocate nodes. 
	// info->nodeCount is the exact number of nodes we need for this tree.
	// info->minNodeCount is the minimum number of nodes that we need to allocate.
	m_numNodes = Max(info->nodeCount,info->minNodeCount);
	Assert(m_contiguousNodes == NULL);
	m_contiguousNodes = rage_new phOptimizedBvhNode[m_numNodes];
	m_NumNodesInUse = 0;

	// Reset all of the nodes so that they don't have any lingering data in them.  In particular we need to have the polygon counts reset to zero.
	const int numAllocatedNodes = m_numNodes;
	for(int nodeIndex = 0; nodeIndex < numAllocatedNodes; ++nodeIndex)
	{
		m_contiguousNodes[nodeIndex].Clear();
	}

	phOptimizedBvhNode*	root1 = buildTreeFromBvhConstructionRecurse(root,info);
	Assert(m_NumNodesInUse <= m_numNodes);
	return root1;
}

void phOptimizedBvh::UpdateFromPrimitiveData(BvhPrimitiveData *primData)
{
	const int numSubtrees = m_CurSubtreeHeaderIndex;
#if __SPU
	// Bring the subtree headers over to SPU memory
	phBVHSubtreeInfo* ppuSubtreeHeaders = m_SubtreeHeaders;
	phBVHSubtreeInfo* subtreeHeaders = Alloca(phBVHSubtreeInfo, numSubtrees);
	cellDmaLargeGet(subtreeHeaders, (uint64_t)ppuSubtreeHeaders, sizeof(phBVHSubtreeInfo) * numSubtrees, DMA_TAG(0), 0, 0);

	u8 subtreeBuffer[MAX_SUBTREE_SIZE] ;
	phOptimizedBvhNode* subtreeRootNode = reinterpret_cast<phOptimizedBvhNode*>(subtreeBuffer);
	cellDmaWaitTagStatusAll(DMA_MASK(0));
#else
	phBVHSubtreeInfo* subtreeHeaders = m_SubtreeHeaders;
#endif // __SPU

	// walk backwards through subtrees since updateTree walks backwards through each subtree
	for(int subtreeHeaderIndex = numSubtrees - 1; subtreeHeaderIndex >= 0; --subtreeHeaderIndex)
	{
		phBVHSubtreeInfo& curSubtreeHeader = subtreeHeaders[subtreeHeaderIndex];
		const int numNodesInSubtree = curSubtreeHeader.m_LastIndex - curSubtreeHeader.m_RootNodeIndex;
#if __SPU
		// Bring the subtree into SPU memory
		cellDmaLargeGet(subtreeRootNode, (uint64_t)(&GetNode(curSubtreeHeader.m_RootNodeIndex)), sizeof(phOptimizedBvhNode) * numNodesInSubtree, DMA_TAG(0), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(0));
#else // __SPU
		// Set the subtree root
		phOptimizedBvhNode* subtreeRootNode = GetRootNode() + curSubtreeHeader.m_RootNodeIndex;
#endif // __SPU

		// Update the nodes in the tree first, then update the subtree
		updateTree(primData, subtreeRootNode, numNodesInSubtree);
		curSubtreeHeader.SetAABBFromNode(*subtreeRootNode);

#if __SPU
		// DMA back the updated subtree nodes and header
		cellDmaLargePut(subtreeRootNode, (uint64_t)(&GetNode(curSubtreeHeader.m_RootNodeIndex)), sizeof(phOptimizedBvhNode) * numNodesInSubtree, DMA_TAG(0), 0, 0);
		cellDmaLargePut(&subtreeHeaders[subtreeHeaderIndex],(uint64_t)&ppuSubtreeHeaders[subtreeHeaderIndex],sizeof(phBVHSubtreeInfo), DMA_TAG(0), 0, 0);
		cellDmaWaitTagStatusAll(DMA_MASK(0));
#endif // __SPU
	}
}

void phOptimizedBvh::updateTree(BvhPrimitiveData *primNodes, phOptimizedBvhNode* subtreeRoot, int subtreeSize)
{
	// walk backwards through the nodes updating their bounding boxes
	// it is guaranteed that a node has a lower index than its children
	for(int nodeIndex = subtreeSize - 1; nodeIndex >= 0; --nodeIndex)
	{
		phOptimizedBvhNode& curNode = subtreeRoot[nodeIndex];
		if(curNode.IsLeafNode())
		{
			// leaf nodes are updated directly from the given primitive data
			ComputeMinAndMax(primNodes, curNode.GetPolygonStartIndex(), curNode.GetPolygonStartIndex() + curNode.GetPolygonCount(), curNode);
		}
		else
		{
			// internal node boxes are formed from their children bounding boxes, which should have already been updated
			int leftIndex = nodeIndex + 1;
			const phOptimizedBvhNode& leftNode = subtreeRoot[leftIndex];
			const phOptimizedBvhNode& rightNode = subtreeRoot[leftIndex + leftNode.GetEscapeIndex()];
			curNode.CombineAABBs(leftNode,rightNode);
		}
	}
}

// static const phOptimizedBvh *s_CurBVH = NULL;

// Callback function for std::sort.
struct CompareLeafNodeAxisPredicate : public std::binary_function<const BvhPrimitiveData&, const BvhPrimitiveData&, bool>
{
	int m_Axis1;
	int m_Axis2;

	__forceinline CompareLeafNodeAxisPredicate(int axis1, int axis2)
		: m_Axis1(axis1)
		, m_Axis2(axis2)
	{
		Assert(axis1 != axis2);
	}

	__forceinline bool operator()(const BvhPrimitiveData& node1, const BvhPrimitiveData& node2)
	{
#if 0
		// For some reason this produces a significantly faster BVH tree in my test case, even though they really should be doing the same thing.
		// I really need to look at the difference in the tree that's produced and find out why that is (it might be that this version is just
		//   randomly doing something slightly different that happens to be advantageous).
		Vector3 aabbMin[2], aabbMax[2];
		s_CurBVH->UnQuantize(aabbMin[0], node1.m_AABBMin);
		s_CurBVH->UnQuantize(aabbMin[1], node2.m_AABBMin);
		s_CurBVH->UnQuantize(aabbMax[0], node1.m_AABBMax);
		s_CurBVH->UnQuantize(aabbMax[1], node2.m_AABBMax);

		Vector3 center[2];
		center[0].Add(aabbMin[0], aabbMax[0]);
		center[1].Add(aabbMin[1], aabbMax[1]);

		//	Vector3 sortAxis(VEC3_ZERO);
		//	sortAxis[axis] = 1.0f;
		//	float centerAlongAxis[2];
		//	centerAlongAxis[0] = center[0].Dot(sortAxis);
		//	centerAlongAxis[1] = center[1].Dot(sortAxis);
		//	float centerAlongAxis[2];
		//	centerAlongAxis[0] = center[0][axis];
		//	centerAlongAxis[1] = center[1][axis];

		int axis1 = m_Axis1;
		int axis2 = m_Axis2;

		// Sort based on the centroid of the AABB.
		int retVal = center[0][axis1] > center[1][axis1] ? 1 : (center[0][axis1] < center[1][axis1] ? -1 : 0);
#if 1
		if(retVal == 0)
		{
			retVal = center[0][axis2] > center[1][axis2] ? 1 : (center[0][axis2] < center[1][axis2] ? -1 : 0);
		}
		//	if(retVal == 0)
		//	{
		//		int axis3 = 3 - axis1 - axis2;
		//		Assert(axis3 != axis1);
		//		Assert(axis3 != axis2);
		//		retVal = center[0][axis3] > center[1][axis3] ? 1 : (center[0][axis3] < center[1][axis3] ? -1 : 0);
		//	}
#if 1
		// If sorting based on the centroid of the AABB doesn't help us order them, then sort based on the centroid of the triangle.
		// Note that it is very important that this step be performed *after* the AABB sort and *only* if the AABB sort is inconclusive.
		if(retVal == 0)
		{
			s_CurBVH->UnQuantize(center[0], node1.m_Centroid);
			s_CurBVH->UnQuantize(center[1], node2.m_Centroid);

			retVal = center[0][axis2] > center[1][axis2] ? 1 : (center[0][axis2] < center[1][axis2] ? -1 : 0);
			if(retVal == 0)
			{
				retVal = center[0][axis1] > center[1][axis1] ? 1 : (center[0][axis1] < center[1][axis1] ? -1 : 0);
			}
			//		if(retVal == 0)
			//		{
			//			int axis3 = 3 - axis1 - axis2;
			//			Assert(axis3 != axis1);
			//			Assert(axis3 != axis2);
			//			retVal = center[0][axis3] > center[1][axis3] ? 1 : (center[0][axis3] < center[1][axis3] ? -1 : 0);
			//		}
			Assert(retVal != 0);
		}
#endif
#endif
		return retVal;
#else
		// Sort based on the centroid of the AABB.
		s32 center1 = s32(node1.m_AABBMin[m_Axis1]) + s32(node1.m_AABBMax[m_Axis1]);
		s32 center2 = s32(node2.m_AABBMin[m_Axis1]) + s32(node2.m_AABBMax[m_Axis1]);
		s32 otherCenter1 = s32(node1.m_AABBMin[m_Axis2]) + s32(node1.m_AABBMax[m_Axis2]);
		s32 otherCenter2 = s32(node2.m_AABBMin[m_Axis2]) + s32(node2.m_AABBMax[m_Axis2]);
		const bool oldRetVal = center1 == center2 ? otherCenter1 > otherCenter2 : center1 > center2;

// TODO: It seems like this should be faster but (no ternary operator and therefore no branch) but this is actually measurably slower.
//		const s32 combinedValue1 = (center1 << 15) + (otherCenter1 >> 1);
//		const s32 combinedValue2 = (center2 << 15) + (otherCenter2 >> 1);
//		const bool newRetVal = (combinedValue1 > combinedValue2);
////		Assert(newRetVal == oldRetVal);
//		return newRetVal;

		return oldRetVal;
#endif
	}
};

void phOptimizedBvh::SortPrimitivesAlongAxes(BvhPrimitiveData *leafNodes, int startIndex, int endIndex, int splitAxis1, int splitAxis2)
{
	// First, sort the section in question according the values of the selected splitAxis.
	std::sort(leafNodes + startIndex, leafNodes + endIndex, CompareLeafNodeAxisPredicate(splitAxis1, splitAxis2));
}


int phOptimizedBvh::CalcSplittingIndex(int startIndex, int endIndex, int targetPrimitivesPerNode)
{
#if SPLIT_AT_MEDIAN
	// One option for picking our initial splitting index is simply to chop it in half (splitting at the median).
	int splitIndex = (startIndex + endIndex) >> 1;
#endif

#if SPLIT_AT_MEAN
	// This option calculates the mean and splits there.
	Vector3 means(ORIGIN);
	for (int i = startIndex; i < endIndex; i++)
	{
		Vector3 nodeAABBMin, nodeAABBMax;
		UnQuantize(nodeAABBMin, leafNodes[i].m_AABBMin);
		UnQuantize(nodeAABBMax, leafNodes[i].m_AABBMax);

		Vector3 center;
		center.Average(nodeAABBMin, nodeAABBMax);
		means.Add(center);
	}
	means *= 1.0f / (float)(endIndex - startIndex);

	u16 quantizedMean[3];
	Quantize(quantizedMean, means);

	// TODO: Could replace this with a binary search.
	int splitIndex;
	for(splitIndex = startIndex; splitIndex < endIndex; ++splitIndex)
	{
		const phOptimizedBvhNode &curNode = leafNodes[splitIndex];
		u32 center = (curNode.m_AABBMin[splitAxis] + curNode.m_AABBMax[splitAxis]) >> 1;
		if(center >= quantizedMean[splitAxis])
		{
			break;
		}
	}

	// Ensure that our splitting index lies in the middle third of the range.
	// NOTE: There point in trying to do this when splitting along the median, because we're guaranteed to be in the middle third.
	if(3 * splitIndex < 2 * startIndex + endIndex)
	{
		splitIndex = (2 * startIndex + endIndex) / 3;
	}
	else if(3 * splitIndex > startIndex + 2 * endIndex)
	{
		splitIndex = (startIndex + 2 * endIndex) / 3;
	}
#endif

#if SPLIT_AT_SEARCH
	const int minSplitIndex = (2 * startIndex + endIndex) / 3;
	const int maxSplitIndex = (startIndex + 2 * endIndex) / 3;
//	const int minSplitIndex = (3 * startIndex + 2 * endIndex) / 5;
//	const int maxSplitIndex = (2 * startIndex + 3 * endIndex) / 5;

	int bestSplitIndex = -1;
	u64 bestSplitVolume = (u64)(-1);
	u16 aabbMin[2][3], aabbMax[2][3];
	for(int testSplitIndex = minSplitIndex; testSplitIndex < maxSplitIndex; ++testSplitIndex)
	{
		//if((endIndex - startIndex) < 2 * POLYS_PER_NODE || ((testSplitIndex - startIndex) % POLYS_PER_NODE) == 0 || ((endIndex - testSplitIndex) % POLYS_PER_NODE) == 0)
		{
			aabbMin[0][0] = 0xFFFF;
			aabbMin[0][1] = 0xFFFF;
			aabbMin[0][2] = 0xFFFF;
			aabbMax[0][0] = 0;
			aabbMax[0][1] = 0;
			aabbMax[0][2] = 0;
			aabbMin[1][0] = 0xFFFF;
			aabbMin[1][1] = 0xFFFF;
			aabbMin[1][2] = 0xFFFF;
			aabbMax[1][0] = 0;
			aabbMax[1][1] = 0;
			aabbMax[1][2] = 0;

			// Set up an initial partitioning - just split it basically in half, and then compute the AABB.
			for(int curPolyIndex = startIndex; curPolyIndex < endIndex; ++curPolyIndex)
			{
				const int partitionIndex = curPolyIndex < testSplitIndex ? 0 : 1;
				aabbMin[partitionIndex][0] = Min(aabbMin[partitionIndex][0], leafNodes[curPolyIndex].m_AABBMin[0]);
				aabbMin[partitionIndex][1] = Min(aabbMin[partitionIndex][1], leafNodes[curPolyIndex].m_AABBMin[1]);
				aabbMin[partitionIndex][2] = Min(aabbMin[partitionIndex][2], leafNodes[curPolyIndex].m_AABBMin[2]);
				aabbMax[partitionIndex][0] = Max(aabbMax[partitionIndex][0], leafNodes[curPolyIndex].m_AABBMax[0]);
				aabbMax[partitionIndex][1] = Max(aabbMax[partitionIndex][1], leafNodes[curPolyIndex].m_AABBMax[1]);
				aabbMax[partitionIndex][2] = Max(aabbMax[partitionIndex][2], leafNodes[curPolyIndex].m_AABBMax[2]);
			}

			u64 curVolume[2];
			curVolume[0] = (aabbMax[0][0] - aabbMin[0][0]) * (aabbMax[0][1] - aabbMin[0][1]) * (aabbMax[0][2] - aabbMin[0][2]);
			curVolume[1] = (aabbMax[1][0] - aabbMin[1][0]) * (aabbMax[1][1] - aabbMin[1][1]) * (aabbMax[1][2] - aabbMin[1][2]);

			u64 curTotalVolume = curVolume[0] + curVolume[1];
			if(curTotalVolume < bestSplitVolume)
			{
				bestSplitIndex = testSplitIndex;
				bestSplitVolume = curTotalVolume;
			}
		}
	}

	Assert(bestSplitIndex != -1);
	int splitIndex = bestSplitIndex;
#endif

// Remember to try disabling this.
#if 1
	// When building the tree, we want to avoid splitting things up in a manner that will result in unnecessarily having nodes with fewer than
	//   POLYS_PER_NODE polygons in them.  For example, if we have four polygons per node and 44 polygons left, we don't want to split them 22/22.
	// Our method of choice to accomplish this is to steal from the larger partition in order to make the size of the smaller partition a multiple
	//   of POLYS_PER_NODE.  This is not the only, nor necessarily the best, way to accomplish this.  Another possibility (that might be better) is to
	//   determine the minimum adjustment necessary to make the size of one of the two partitions a multiple of POLYS_PER_NODE (and neither are zero).
	if(endIndex - splitIndex > splitIndex - startIndex)
	{
		// The second half is larger than the first half.
		int oldFirstHalfSize = splitIndex - startIndex;
		int secondHalfSizeAdjustment = (targetPrimitivesPerNode - (oldFirstHalfSize % targetPrimitivesPerNode)) % targetPrimitivesPerNode;
		Assert(secondHalfSizeAdjustment < endIndex - splitIndex);
		splitIndex += secondHalfSizeAdjustment;
	}
	else
	{
		// The first half is no smaller than the second half.
		int oldSecondHalfSize = endIndex - splitIndex;
		int firstHalfSizeAdjustment = (targetPrimitivesPerNode - (oldSecondHalfSize % targetPrimitivesPerNode)) % targetPrimitivesPerNode;
		Assert(firstHalfSizeAdjustment < splitIndex - startIndex);
		splitIndex -= firstHalfSizeAdjustment;
	}
#endif

	return splitIndex;
}


void phOptimizedBvh::Copy(const phOptimizedBvh* original)
{
	Assert(m_contiguousNodes == NULL);
	Assert(m_SubtreeHeaders == NULL);

	*this = *original;

	m_contiguousNodes = rage_new phOptimizedBvhNode[m_numNodes];
	sysMemCpy(m_contiguousNodes, original->m_contiguousNodes, sizeof(phOptimizedBvhNode) * m_numNodes);

	m_SubtreeHeaders = rage_new phBVHSubtreeInfo[m_NumSubtreeHeaders];
	sysMemCpy(m_SubtreeHeaders, original->m_SubtreeHeaders, sizeof(phBVHSubtreeInfo) * m_NumSubtreeHeaders);
}


// Determines the axis along with the variance of the center points of the nodes is greatest.  This axis will be the axis suggested as a splitting plane.
// Another possibility here is to look at the AABB of all of the nodes and return the axis along which it is the largest.
int phOptimizedBvh::calcSplittingAxis(const BvhPrimitiveData *leafNodes, int startIndex, int endIndex, int &middleAxis)
{
	// Compute the variance as the "mean of the square minus the square of the mean".  While somewhat more susceptible to precision problems, this calculation
	//   allows us to avoid iterating over the nodes twice and unpacking the AABB extents twice.
	Vec3V vSum(V_ZERO);
	Vec3V vSumSquares(V_ZERO);
	ScalarV vsNumIndices(V_ZERO);

	FastAssert(startIndex < endIndex);
	const BvhPrimitiveData *curPrimData = &leafNodes[startIndex];
	const BvhPrimitiveData *endPrimData = &leafNodes[endIndex];
#if 0
	const ScalarV vsOne(V_ONE);
	do
	{
		Vec3V nodeAABBMin, nodeAABBMax;
		UnPackAlignedToFloatShiftedEight(RC_VECTOR3(nodeAABBMin), curPrimData->m_AABBMin);
		UnPackAlignedToFloatShiftedEight(RC_VECTOR3(nodeAABBMax), curPrimData->m_AABBMax);

		const Vec3V nodeCenter = Average(nodeAABBMin, nodeAABBMax);
		vSum = Add(vSum, nodeCenter);
		vSumSquares = Add(vSumSquares, nodeCenter * nodeCenter);
		vsNumIndices = Add(vsNumIndices, vsOne);

		++curPrimData;
	} while (curPrimData < endPrimData);
#else
	const ScalarV vsTwo(V_TWO);
	while(curPrimData + 1 < endPrimData)
	{
		Vec3V nodeAABBMin0, nodeAABBMax0, nodeAABBMin1, nodeAABBMax1;
		UnPackAlignedToFloatShiftedEight(RC_VECTOR3(nodeAABBMin0), curPrimData->m_AABBMin);
		UnPackAlignedToFloatShiftedEight(RC_VECTOR3(nodeAABBMax0), curPrimData->m_AABBMax);
		++curPrimData;
		UnPackAlignedToFloatShiftedEight(RC_VECTOR3(nodeAABBMin1), curPrimData->m_AABBMin);
		UnPackAlignedToFloatShiftedEight(RC_VECTOR3(nodeAABBMax1), curPrimData->m_AABBMax);
		++curPrimData;

		const Vec3V nodeCenter0 = Average(nodeAABBMin0, nodeAABBMax0);
		const Vec3V nodeCenter1 = Average(nodeAABBMin1, nodeAABBMax1);
		vSum = Add(vSum, nodeCenter0);
		vSum = Add(vSum, nodeCenter1);
		vSumSquares = Add(vSumSquares, Scale(nodeCenter0, nodeCenter0));
		vSumSquares = Add(vSumSquares, Scale(nodeCenter1, nodeCenter1));
		vsNumIndices = Add(vsNumIndices, vsTwo);
	}
	switch((endIndex - startIndex) & 1)
	{
		case 1:
		{
			const ScalarV vsOne(V_ONE);
			Vec3V nodeAABBMin, nodeAABBMax;
			UnPackAlignedToFloatShiftedEight(RC_VECTOR3(nodeAABBMin), (endPrimData - 1)->m_AABBMin);
			UnPackAlignedToFloatShiftedEight(RC_VECTOR3(nodeAABBMax), (endPrimData - 1)->m_AABBMax);

			const Vec3V nodeCenter = Average(nodeAABBMin, nodeAABBMax);
			vSum = Add(vSum, nodeCenter);
			vSumSquares = Add(vSumSquares, Scale(nodeCenter, nodeCenter));
			vsNumIndices = Add(vsNumIndices, vsOne);
			break;
		}
	}
#endif

	// Compensate for the non-isometry of the quantized space.
	const Vec3V vInvQuantize = RCC_VEC3V(m_InvQuantize);
	vSum = (vSum * vInvQuantize);
	vSumSquares = (vSumSquares * vInvQuantize * vInvQuantize);

	const Vec3V scaledVariance = vsNumIndices * vSumSquares - vSum * vSum;

	// Mathematically variance should be >= 0 but, due to precision limitations, this calculation could result in slightly negative values.  We could do a Max()
	//   here to clamp negative values to zero but that wouldn't really gain anything for us because we're just comparing the components against each other.
	const VecBoolV vbGreaterThanEqualToNextComponent = IsGreaterThan(scaledVariance, scaledVariance.Get<Vec::Y, Vec::Z, Vec::X>());
	u32 index;
	ResultToIndexZYX(index, vbGreaterThanEqualToNextComponent);
	FastAssert(index < 7);		// All 1's should not be possible because we used IsGreaterThan() above.
	static const int extremeIndexFromResult[8] = { 0, 0, 1, 0, 2, 2, 1, 2 };
	const int minAxis = extremeIndexFromResult[7 - index];
	const int maxAxis = extremeIndexFromResult[index];
	Assert(minAxis != maxAxis);
	middleAxis = (3 - minAxis - maxAxis);
	return maxAxis;
}

#if __PFDRAW
void phOptimizedBvh::Draw(Mat34V_In matrix, int depth, bool drawIndices) const
{
	// store previous draw settings
	Color32 oldColor(grcCurrentColor);
	bool oldLighting = grcLighting(false);

	const int depthToDraw = depth;

	Mat34V boxMatrix;
	Vec3V aabbMin, aabbMax, boxCenter;
	Color32 curDepthColor(100, 0, 0);
	Color32 childDepthColor(200, 0, 0);

	int curNodeIndex = 0;
	int curDepth = 0;
	bool isLeftChild[100];
	ASSERT_ONLY(int nodeIndex[100]);

	isLeftChild[0] = true;
	ASSERT_ONLY(nodeIndex[0] = 0);
	while(curNodeIndex < GetNumNodes())
	{
		const phOptimizedBvhNode &curNode = GetNode(curNodeIndex);

		// Check if the current node is one that we're interested in.
		const bool isCurDepth = curDepth == depthToDraw;
		const bool isChildDepth = curDepth == depthToDraw + 1;
		const bool willDrawBox = isCurDepth || isChildDepth;
		if(willDrawBox)
		{
			Color32 colorToUse(isCurDepth ? curDepthColor : childDepthColor);
			UnQuantize(RC_VECTOR3(aabbMin), curNode.m_AABBMin);
			UnQuantize(RC_VECTOR3(aabbMax), curNode.m_AABBMax);

			boxCenter = Average(aabbMin, aabbMax);
			boxMatrix = matrix;
			boxMatrix.SetCol3(Transform(matrix,boxCenter));

			const float scaleFactor = isCurDepth ? 1.0f : 0.994f + 0.005f * sinf(TIME.GetElapsedTime() * 4.0f);

			grcDrawBox(scaleFactor * VEC3V_TO_VECTOR3(aabbMax - aabbMin), RCC_MATRIX34(boxMatrix), colorToUse);

			if (drawIndices)
			{
				// Write the node index number on the screen at the node center.
				char nodeIndexText[8];
				nodeIndexText[7] = '\0';
				formatf(nodeIndexText,7,"%i",curNodeIndex);
				grcDrawLabelf(VEC3V_TO_VECTOR3(boxMatrix.GetCol3()),nodeIndexText);
			}
		}

		// Go on to the next node.
		++curNodeIndex;
		if(curNode.IsLeafNode())
		{
			// We've bottomed out.  We need walk back up the tree until we find somebody who was a left child.
			while(curDepth > 0 && !isLeftChild[curDepth])
			{
				--curDepth;
			}
			isLeftChild[curDepth] = false;
			ASSERT_ONLY(const phOptimizedBvhNode &prevNode = GetNode(nodeIndex[curDepth]));
			Assert(nodeIndex[curDepth] + prevNode.GetEscapeIndex() == curNodeIndex);
		}
		else
		{
			// Not a leaf node, let's just descend further down the tree through the left child.
			++curDepth;
			isLeftChild[curDepth] = true;
			ASSERT_ONLY(nodeIndex[curDepth] = curNodeIndex);
		}
	}

	// set the draw settings to their previous values
	grcLighting(oldLighting);
	grcColor(oldColor);
}
#endif

} // namespace rage
