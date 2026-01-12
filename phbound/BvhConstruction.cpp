#include "BvhConstruction.h"
#include "system/memory.h"
#include "vectormath/scalarv.h"
#if !__FINAL
#include "system/timer.h"
#endif	// !__FINAL

#include <algorithm>
#include <functional>
#include <limits>

//PRAGMA_OPTIMIZE_OFF()

#define BVH_DEBUG 0
#define BVH_TEST 0
#define USE_OLD_BVH_CONSTRUCTION 0
#define USE_INSIDE_OPTIMIZATION 0

/*
The idea behind this BVH construction algorithm is to find a BVH that minimizes the average number of overlap tests with respect
to a given distribution of query volumes. In general, it is not possible to optimize a BVH for all possible query volumes. Two 
different query volume distributions will have two different optimal BVHs. For our application we want to construct an AABB BVH.
It turns out though that the general idea behind how to construct an optimal BVH for any bounding volume type is the same.

----------------
---- Theory ----
----------------
This section briefly explains the theory behind optimal BVH construction.

Given a BVH we can count the number of overlap tests needed for a particular query volume with the following recursion:
int CountOverLapTests(Node n, QueryVolume QV)
{
	if (IsLeaf(n))
	{
		if (primitiveCount <= 1)
			return 0;				// We don't need to do any overlap tests because the leaf node bounding volume should be tight for 1 primitve.
		else
			return primitiveCount;	// We need to do an additional overlap test for each primitive.
	}
	int overLapTests = numberOfChildren;	// We need to do one overlap test for each child.
	for (each child node child[i])
		if (DoesOverLap(QV,child[i]))
			overLapTests += CountOverLapTests(child[i],QV);
	return overLapTests;
}

With respect to a given probability distribution we can count the average number of overlap tests with the following recursion:
int CountAverageNumberOfOverLapTests(Node n, QueryVolumeDistribution QVD)
{
	if (IsLeaf(n))
	{
		if (primitiveCount <= 1)
			return 0;			// We don't need to do any overlap tests because the leaf node bounding volume should be tight for 1 primitve.
		else
		return primitiveCount;	// We need to do an additional overlap test for each primitive.
	}
	
	int overLapTests = numberOfChildren;	// We need to do one overlap test for each child.
	for (each child node child[i])
		overLapTests += Probability(QVD,child[i],n) * CountAverageNumberOfOverLapTests(child[i],QVD);
	return overLapTests;
}
Probability(QVD,node) is the probability that a query volume overlaps a node.
Probability(QVD,child,parent) is the probability that a query volume overlaps a child given that it overlaps the parent.

The above algorithm can be written more compactly as:
Count(parent) = { Sum[i=0,i=N-1,P(child[i]|parent)*Count(child[i])] + N   |   parent is not a leaf and has N children.		}
				{ 0														  |	  parent is a leaf with 1 primitive.			}
				{ primitiveCount										  |	  parent is a leaf with more than 1 primitive.	}
P(c|p)		  = { The probability that a query volume overlaps a child given that it overlaps the parent.					}

The distribution QVD has a domain that only includes query volumes that overlap the root volume of the BVH.
Therefore Probability(QVD,root) equals 1. Given that a child node volume is completely contained within its ancestor volumes,
we can calculate the probability of query volume overlap for any node given some ancestor that also overlaps with the following:
If node0 is a parent of node1 and node1 is a parent of node2 then Probability(QVD,node2,node0) = Probability(QVD,node2,node1) * Probability(QVD,node1,node0).
(See Bayes' Rule)

The above recursion formula can be rewritten non-recursively as follows:
Count(QVD,root) = Sum Probability(QVD,InternalNode[i]) * NumChildren(InternalNode[i]) + // Sum over all internal nodes.
				  Sum Probability(QVD,LeafNode[i]) * LeafNodeCost(LeafNode[i]);		    // Sum over all leaf nodes.

In words the above formula states that the average number of overlap tests equals the sum of the products of each
internal node with its child count plus the sum of the products of each leaf node with its leaf cost.

If we fix the number of children in all internal nodes to some constant and assume all leaf costs are 0, then the problem
of optimizing a BVH with respect to a query volume distribution comes down to choosing internal nodes such that the sum
of their probabilities are minimized. This is true for any bounding volume type, not just AABBs.

-------------------------------
---- AABB BVH Construction ----
-------------------------------
For practical reasons we choose a very simple query volume distribution for constructing our AABB BVHs. We don't know what the actual 
distribution will be when the BVH is used. Nonetheless, this produces goods results in practice. Our distribution will be a 
fixed size AABB that has a uniformly distributed position. We denote the dimensions of the fixed size AABB as QV. 
The probability formulas for this distribution are:
Prob(child|parent) = Volume(childDims+QV) / Volume(parentDims+QV)
Prob(node) = Volume(nodeDims+QV) / Volume(rootDims+QV)

It is possible to include multiple fixed size AABBs in our distribution assuming each one has a uniformly distributed position.
If, for each query volume, QV[i] is the fixed size and W[i] is a weight s.t. the sum of all the weights equals 1 then the probability is:
Prob(child|parent) = Sum W[i] * Volume(childDims+QV[i]) / Volume(parentDims+QV[i])	// Sum over each query volume
Prob(node) = Sum W[i] * Volume(nodeDims+QV[i]) / Volume(rootDims+QV[i])				// Sum over each query volume
W[i] can be thought of as the probability that a given QV is intersected with the BVH.

Our AABB BVHs currently only have two children per internal node. There is also a desired number of primitives per leaf that we try
to achieve in order to minimize memory overhead.

-------------------------------
---- Top Down Construction ----
-------------------------------
Given a set of nodes, we want to partition the nodes into 2 sets and recursively build a subtree for each set. This recursion ends when
we get a set with a desired number of primitives. It is assumed that each node has an arbitrary cost associated with it.

Given set of N nodes with parent node P, the overlap cost for P before the partition is: 
Pcost = Sum Prob(n[i]|P) * Cost(n[i]) + N		// Sum over all nodes

After the partition we will have two child nodes, C1 and C2, for parent P. Each child will now have its own list of child nodes
taken from the original set. N1 and N2 are the number of children for C1 and C2 resp. The overlap cost for P will be:
Pcost_ = Prob(C1|P) * (Sum Prob(n1[i]|C1) * Cost(n1[i]) + N1) + Prob(C2|P) * (Sum Prob(n2[i]|C2) * Cost(n2[i]) + N2) + 2
Pcost_ = Sum Prob(n1[i]|P) * Cost(n1[i]) + Sum Prob(n2[i],P) * Cost(n2[i]) + N1 * Prob(C1|P) + N2 * Prob(C2|P) + 2
Pcost_ = (Pcost - N + 2) + N1 * Prob(C1|P) + N2 * Prob(C2|P)

Our goal is to minimize cost. From the expression for Pcost_ it becomes clear that we want to choose a partition that
minimizes N1 * Prob(C1|P) + N2 * Prob(C2|P). The first part of the Pcost_ expression is constant so we can ignore it. 

--------------------------------
---- Bottom Up Construction ----
--------------------------------
Given a set of nodes, we want to find pairs that we can merge into a new node. After merging a pair, we remove the pair from the set and
insert the merged node into the set. This decrements the number of nodes in the set by one. We repeat this until we have only one node left.

Given a set of N nodes with a parent volume P, the total overlap cost before a merging 2 nodes is:
Pcost = Sum Prob(n[i]|P) * Cost(n[i]) + N

The cost of the two nodes we want to merge can be separated out:
Mcost = Prob(n1|P) * Cost(n1) + Prob(n2|P) * Cost(n2) + 2

The cost after merging the nodes into node M is:
Mcost_ = Prob(M|P) * (Prob(n1|M) * Cost(n1) + Prob(n2|M) * Cost(n2) + 2) + 1
Mcost_ = Prob(n1|P) * Cost(n1) + Prob(n2|P) * Cost(n2) + 2 * Prob(M|P) + 1
Mcost_ = (Mcost - 1) + 2 * Prob(M|P)

Our goal is to minimize cost. From the expression for Mcost_ it is clear that we want to choose pairs that minimize Prob(M|P).
The first part of the Mcost_ expression is constant so we can ignore it.
*/

namespace rage
{

__forceinline void Copy(CVec3 & v1, const s16 v2[3])
{
	v1.v[0] = v2[0];
	v1.v[1] = v2[1];
	v1.v[2] = v2[2];
}

__forceinline CVec3 Min(const CVec3 & v1, const CVec3 & v2)
{
	CVec3 retv;
	retv.v[0] = Min(v1.v[0],v2.v[0]);
	retv.v[1] = Min(v1.v[1],v2.v[1]);
	retv.v[2] = Min(v1.v[2],v2.v[2]);
	return retv;
}

__forceinline CVec3 Max(const CVec3 & v1, const CVec3 & v2)
{
	CVec3 retv;
	retv.v[0] = Max(v1.v[0],v2.v[0]);
	retv.v[1] = Max(v1.v[1],v2.v[1]);
	retv.v[2] = Max(v1.v[2],v2.v[2]);
	return retv;
}

__forceinline bool CmpLT(const CVec3 & v1, const CVec3 & v2, const int axis1, const int axis2)
{
	return (v1.v[axis1] == v2.v[axis1]) ? (v1.v[axis2] < v2.v[axis2]) : (v1.v[axis1] < v2.v[axis1]);
}

__forceinline bool CmpGT(const CVec3 & v1, const CVec3 & v2, const int axis1, const int axis2)
{
	return (v1.v[axis1] == v2.v[axis1]) ? (v1.v[axis2] > v2.v[axis2]) : (v1.v[axis1] > v2.v[axis1]);
}

__forceinline void Sub(s32 retv[2], const CVec3 & v1, const CVec3 & v2, const int axis1, const int axis2)
{
	retv[0] = (s32)v1.v[axis1] - (s32)v2.v[axis1];
	retv[1] = (s32)v1.v[axis2] - (s32)v2.v[axis2];
}

__forceinline void Add(s32 retv[2], const CVec3 & v1, const CVec3 & v2, const int axis1, const int axis2)
{
	retv[0] = (s32)v1.v[axis1] + (s32)v2.v[axis1];
	retv[1] = (s32)v1.v[axis2] + (s32)v2.v[axis2];
}

__forceinline bool CmpLT(const s32 v1[2], const s32 v2[2])
{
	return (v1[0] == v2[0]) ? (v1[1] < v2[1]) : (v1[0] < v2[0]);
}

__forceinline bool CmpLT(const s16 v1[2], const s16 v2[2])
{
	return (v1[0] == v2[0]) ? (v1[1] < v2[1]) : (v1[0] < v2[0]);
}

const float INFINITE_QVOLUME = 1e30f;
const float INFINITE_COST = 1e30f;

struct BvhConstructor
{
	enum
	{
		INVALID_INDEX = -1,
		MIN_SIDE = 0,
		MAX_SIDE = 1,
		EXT_MAX = (1 << 30) - 1,
		BOTTOM_UP_LOOK_AHEAD = 32,
		TOP_DOWN_LOOK_AHEAD = 16,
	};

	struct ComparePredicate : public std::binary_function<const BvhConstructionNode *, const BvhConstructionNode * , bool>
	{
		int m_axis1;
		int m_axis2;

		ComparePredicate(const int axis1, const int axis2) : m_axis1(axis1), m_axis2(axis2) 
		{
		}

		ComparePredicate()// : m_axis1(INVALID_INDEX), m_axis2(INVALID_INDEX)
		{
		}

		__forceinline void set(const int axis1, const int axis2)
		{
			m_axis1 = axis1;
			m_axis2 = axis2;
		}

		__forceinline void reset()
		{
			m_axis1 = INVALID_INDEX;
			m_axis2 = INVALID_INDEX;
		}

		__forceinline bool operator != (const ComparePredicate & cmp) const
		{
			return (m_axis1 != cmp.m_axis1 || m_axis2 != cmp.m_axis2);
		}

		__forceinline bool operator == (const ComparePredicate & cmp) const
		{
			return (m_axis1 == cmp.m_axis1) && (m_axis2 == cmp.m_axis2);
		}

		__forceinline void Verify() const
		{
			Assert(m_axis1 != m_axis2);
			Assert(m_axis1 >= 0 && m_axis1 <= 2);
			Assert(m_axis2 >= 0 && m_axis2 <= 2);
		}

		__forceinline bool operator()(const BvhConstructionNode * n1, const BvhConstructionNode * n2) const
		{
			s32 c1[2];
			s32 c2[2];
			Add(c1,n1->m_CAABBMin,n1->m_CAABBMax,m_axis1,m_axis2);
			Add(c2,n2->m_CAABBMin,n2->m_CAABBMax,m_axis1,m_axis2);
			return CmpLT(c1,c2);
		}
	};

	struct NodeListInfo
	{
		Vec3V m_AABBMin;
		Vec3V m_AABBMax;
		Vec3V m_CenterSum;
		Vec3V m_CenterSqSum;
		CVec3 m_CAABBMin;
		CVec3 m_CAABBMax;
		float m_QVolume;
		BvhConstructionNode ** m_NodeList;
		int m_NodeCount;
		ComparePredicate m_sortcmp;
	};

	NodeListInfo m_totalNLI;

	Vec3V m_QV;

	BvhConstructionNode * m_NodeMemPool;
	BvhConstructionNode ** m_NodeList_;
	int m_NodeMemPoolCount;
	int m_MaxNodeCount;

#if USE_INSIDE_OPTIMIZATION
	int m_InsideCounter;
#endif
	int m_TopDownAmbiguousCount;
	int m_BottomUpAmbiguousCount;
	int m_budepth;

	BvhPrimitiveData * m_TempLeafNodes;

	BvhConstructor()
	{
		m_QV = Vec3V(V_ONE);
		m_NodeMemPool = NULL;
		m_NodeList_ = NULL;
		m_NodeMemPoolCount = 0;
		m_MaxNodeCount = 0;
		m_TempLeafNodes = NULL;
	}

	void AllocMem(const int PrimNodeCount)
	{
		m_NodeMemPoolCount = 0;
		m_MaxNodeCount = 2 * PrimNodeCount - 1;
		sysMemStartTemp();
		m_NodeMemPool = rage_new BvhConstructionNode[m_MaxNodeCount];
		m_NodeList_ = rage_new BvhConstructionNode*[PrimNodeCount];
		m_TempLeafNodes = rage_new BvhPrimitiveData[PrimNodeCount];
		sysMemEndTemp();
	}

	void FreeMem()
	{
		sysMemStartTemp();
		delete [] m_NodeMemPool;
		delete [] m_NodeList_;
		delete [] m_TempLeafNodes;
		sysMemEndTemp();
		m_NodeMemPool = NULL;
		m_NodeList_ = NULL;
		m_NodeMemPoolCount = 0;
		m_MaxNodeCount = 0;
	}

	BvhConstructionNode * AllocNode()
	{
		Assert(m_NodeMemPoolCount < m_MaxNodeCount);
		BvhConstructionNode * node = m_NodeMemPool + m_NodeMemPoolCount;
		m_NodeMemPoolCount++;
		return node;
	}

	void ResetBottomUpCost(BvhConstructionNode::CostInfo * cost)
	{
		cost->m_Cost = INFINITE_COST;
		cost->m_Index = INVALID_INDEX;
		cost->m_Ext[0] = EXT_MAX;
		cost->m_Ext[1] = EXT_MAX;
	}

	__forceinline float ComputeQVolume(Vec3V_In AABBMin, Vec3V_In AABBMax, Vec3V_In QV)
	{
		const Vec3V size = AABBMax - AABBMin + QV;
		const ScalarV vol = size.GetX() * size.GetY() * size.GetZ();
		return vol.Getf();
	}

	__forceinline float ComputeQCost(const float childQCost, const float childQVolume, const float parentQVolume)
	{
		// This calculation is propagated from the leaves to the root of the tree. As a result,
		// it has some round off error. This shouldn't create any problems and there shouldn't be
		// a need to use higher precision floats. Be mindful though that QCost will be slightly
		// different in optimized builds vs de-optimized builds due to the compiler using faster
		// multiply and add instructions which have lower roundoff error.
		return childQVolume / parentQVolume * childQCost + 1;
	}

	void InitNode(BvhConstructionNode * node, const phOptimizedBvh * bvhStructure, const BvhPrimitiveData * leafNode, const int leafNodeIndex)
	{
		node->m_Child[0] = NULL;
		node->m_Child[1] = NULL;

		node->m_PrimIndex = (s16)leafNodeIndex;
		node->m_PrimCount = 1;
		
		bvhStructure->UnQuantize(RC_VECTOR3(node->m_AABBMin), leafNode->m_AABBMin);
		bvhStructure->UnQuantize(RC_VECTOR3(node->m_AABBMax), leafNode->m_AABBMax);

		Copy(node->m_CAABBMin,leafNode->m_AABBMin);
		Copy(node->m_CAABBMax,leafNode->m_AABBMax);

		node->m_QVolume = ComputeQVolume(node->m_AABBMin,node->m_AABBMax,m_QV);

		ResetBottomUpCost(&node->m_BestCost);
	}

	void InitNodeList(const phOptimizedBvh * bvhStructure, const BvhPrimitiveData *leafNodes, const int PrimNodeCount)
	{
		Assert(PrimNodeCount > 0);
		Assert(m_NodeMemPoolCount == 0);

		BvhConstructionNode * node = AllocNode();
		m_NodeList_[0] = node;
		InitNode(node,bvhStructure,leafNodes,0);
		m_TempLeafNodes[0] = leafNodes[0];
		InitNLI(m_totalNLI,*node,ComparePredicate(INVALID_INDEX,INVALID_INDEX),m_NodeList_);

		for (int node_i = 1 ; node_i < PrimNodeCount ; node_i++)
		{
			node = AllocNode();
			m_NodeList_[node_i] = node;
			InitNode(node,bvhStructure,leafNodes+node_i,node_i);
			m_TempLeafNodes[node_i] = leafNodes[node_i];
			ExpandNLI(m_totalNLI,*node);
		}
		m_totalNLI.m_QVolume = ComputeQVolume(m_totalNLI.m_AABBMin,m_totalNLI.m_AABBMax,m_QV);
	}

	__forceinline Vec3V_Out ComputeScaledVariance(Vec3V_In vSum, Vec3V_In vSumSq, const int vCount)
	{
		Assert(vCount > 0);
		const Vec3V scaledVariance = ScalarV(float(vCount)) * vSumSq - vSum * vSum;
		return scaledVariance;
	}

	void ComputeVarianceParams(BvhConstructionNode ** const NodeList, const int NodeListCount, Vec3V_InOut vSumOut, Vec3V_InOut vSumSqOut)
	{
		// Code taken from OptimizedBvh::CalcSplittingAxis. The idea is to choose the axes with the highest variances to sort along.
		Vec3V vSum(V_ZERO);
		Vec3V vSumSq(V_ZERO);
		Assert(NodeListCount > 0);
		for (int node_i = 0 ; node_i < NodeListCount ; node_i++)
		{
			const BvhConstructionNode * node = NodeList[node_i];
			const Vec3V center = Average(node->m_AABBMin,node->m_AABBMax);
			vSum += center;
			vSumSq += center * center;
		}
		vSumOut = vSum;
		vSumSqOut = vSumSq;
	}

	ComparePredicate DetermineSortAxes(Vec3V_In scaledVariance)
	{
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
		const int middleAxis = (3 - minAxis - maxAxis);

		return ComparePredicate(maxAxis,middleAxis);
	}

	void SortNodeList(BvhConstructionNode ** const NodeList, const int NodeListCount, const ComparePredicate & sortcmp)
	{
		sortcmp.Verify();
		std::sort(NodeList, NodeList + NodeListCount, sortcmp);
	}

	void VerifySort(BvhConstructionNode ** const NodeList, const int NodeListCount, const ComparePredicate & sortcmp)
	{
#if BVH_DEBUG
		sortcmp.Verify();
		for (int i = 0 ; i < NodeListCount - 1 ; i++)
			Assert(!sortcmp(NodeList[i+1],NodeList[i]));
#else
		(void)NodeList;
		(void)NodeListCount;
		(void)sortcmp;
#endif
	}

	void UpdateSort(NodeListInfo & NLI)
	{
		// Determine sort axes and sort if needed.
		const Vec3V scaledVariance = ComputeScaledVariance(NLI.m_CenterSum,NLI.m_CenterSqSum,NLI.m_NodeCount);
		const ComparePredicate sortcmp = DetermineSortAxes(scaledVariance);
#if BVH_DEBUG
		{
			Vec3V vSum;
			Vec3V vSumSq;
			ComputeVarianceParams(NLI.m_NodeList,NLI.m_NodeCount,vSum,vSumSq);
			const Vec3V difSum = NLI.m_CenterSum - vSum;
			const Vec3V difSumSq = NLI.m_CenterSqSum - vSumSq;
			const float ndifSum = Mag(difSum).Getf();
			const float ndifSumSq = Mag(difSumSq).Getf();
			const float eps = .0001f;
			const float ndifSumEps = eps * Max(Mag(NLI.m_CenterSum).Getf(),Mag(vSum).Getf());
			const float ndifSumSqEps = eps * Max(Mag(NLI.m_CenterSqSum).Getf(),Mag(vSumSq).Getf());
			Assert(ndifSum <= ndifSumEps);
			Assert(ndifSumSq <= ndifSumSqEps);
		}
#endif
		if (sortcmp != NLI.m_sortcmp)
		{
			SortNodeList(NLI.m_NodeList,NLI.m_NodeCount,sortcmp);
			NLI.m_sortcmp = sortcmp;
		}
		VerifySort(NLI.m_NodeList,NLI.m_NodeCount,sortcmp);
	}

	void MergeNodes(BvhConstructionNode * parent, BvhConstructionNode * child0, BvhConstructionNode * child1)
	{
		parent->m_Child[0] = child0;
		parent->m_Child[1] = child1;

		parent->m_AABBMin = Min(child0->m_AABBMin,child1->m_AABBMin);
		parent->m_AABBMax = Max(child0->m_AABBMax,child1->m_AABBMax);

		parent->m_CAABBMin = Min(child0->m_CAABBMin,child1->m_CAABBMin);
		parent->m_CAABBMax = Max(child0->m_CAABBMax,child1->m_CAABBMax);

		parent->m_PrimCount = child0->m_PrimCount + child1->m_PrimCount;
		Assert(parent->m_PrimCount > 1);
		parent->m_PrimIndex = INVALID_INDEX;

		parent->m_QVolume = ComputeQVolume(parent->m_AABBMin,parent->m_AABBMax,m_QV);

		ResetBottomUpCost(&parent->m_BestCost);
	}

	BvhConstructionNode::CostInfo ComputeMergeCost(BvhConstructionNode * n1, BvhConstructionNode * n2, const NodeListInfo & NLI, const ComparePredicate & sortcmp)
	{
		const Vec3V AABBMin = Min(n1->m_AABBMin,n2->m_AABBMin);
		const Vec3V AABBMax = Max(n1->m_AABBMax,n2->m_AABBMax);

		// Compute the qcost of the merged node. This is the average number of overlaps if we recurse down the parent node.
		//const float parentQVolume = ComputeQVolume(AABBMin,AABBMax,m_QV);
		//const float parentQCost = ComputeQCost(*n1,parentQVolume) + ComputeQCost(*n2,parentQVolume);
		// Compute the qcost of the merging nodes with respect to the node list volume.
		//const float initialTotalQCost = ComputeQCost(*n1,NLI.m_QVolume) + ComputeQCost(*n2,NLI.m_QVolume);
		// Compute the qcost of the merged node with respect to the node list volume.
		//const float finalTotalQCost = ComputeQCost(parentQCost,parentQVolume,NLI.m_QVolume);
		//cost.m_Cost = float(finalTotalQCost - initialTotalQCost);

		BvhConstructionNode::CostInfo cost;
		cost.m_Cost = ComputeQVolume(AABBMin,AABBMax,m_QV) / NLI.m_QVolume;
		cost.m_Index = INVALID_INDEX;

		const CVec3 CAABBMin = Min(n1->m_CAABBMin,n2->m_CAABBMin);
		const CVec3 CAABBMax = Max(n1->m_CAABBMax,n2->m_CAABBMax);
/*
		s32 minv[2];
		s32 maxv[2];
		//Sub(minv,NLI.m_CAABBMax,CAABBMax,sortcmp.m_axis1,sortcmp.m_axis2);
		//Sub(maxv,CAABBMin,NLI.m_CAABBMin,sortcmp.m_axis1,sortcmp.m_axis2);
		Sub(minv,NLI.m_CAABBMax,CAABBMin,sortcmp.m_axis1,sortcmp.m_axis2);
		Sub(maxv,CAABBMax,NLI.m_CAABBMin,sortcmp.m_axis1,sortcmp.m_axis2);
		const s32 ext1 = Min(minv[0],maxv[0]);
		const s32 ext2 = Min(minv[1],maxv[1]);
*/
		Sub(cost.m_Ext,CAABBMax,NLI.m_CAABBMin,sortcmp.m_axis1,sortcmp.m_axis2);

		Sub(cost.m_Ext1,CAABBMax,CAABBMin,sortcmp.m_axis1,sortcmp.m_axis2);

		return cost;
	}
	
	__forceinline bool IsCostBetter(const BvhConstructionNode::CostInfo & cost1, const BvhConstructionNode::CostInfo & cost2)
	{
		if (cost1.m_Cost < cost2.m_Cost)
			return true;
		else if (cost1.m_Cost == cost2.m_Cost)
		{
			if (CmpLT(cost1.m_Ext1,cost2.m_Ext1))
				return true;
			else if (CmpLT(cost2.m_Ext1,cost1.m_Ext1))
				return false;
			else
			{
				if (CmpLT(cost1.m_Ext,cost2.m_Ext))
					return true;
				else if (CmpLT(cost2.m_Ext,cost1.m_Ext))
					return false;
				else
				{
					m_BottomUpAmbiguousCount++;
					return false;
				}
			}
		}
		else
			return false;
	}

	// BottomUPxxx funcs are optimized versions of ExpandNLI for ConstructBottomUp.
	void BottomUpResetNLI(NodeListInfo & nli)
	{
		nli.m_CenterSum = Vec3V(V_ZERO);
		nli.m_CenterSqSum = Vec3V(V_ZERO);
		nli.m_NodeCount = 0;
	}

	void BottomUpExpandNLI(NodeListInfo & nli, const BvhConstructionNode & node)
	{
		const Vec3V center = Average(node.m_AABBMin,node.m_AABBMax);
		nli.m_CenterSum += center;
		nli.m_CenterSqSum += center * center;
		nli.m_NodeCount++;
	}

	enum
	{
		MAX_PRIMS_MODE_TEST_ALL = 0,
		MAX_PRIMS_MODE_TEST_PRIM_COUNT = 1,
	};

	BvhConstructionNode * ConstructBottomUp(NodeListInfo & NLI, const int targetPrimitivesPerLeaf, const int MaxPrimsPerNode, const int MaxPrimsPerNodeMode, const int LOOK_AHEAD)
	{
		(void)targetPrimitivesPerLeaf;

		BvhConstructionNode ** NodeList = NLI.m_NodeList;
		int iter = 0 ;
		while (NLI.m_NodeCount > 1)
		{
			iter++;
			UpdateSort(NLI);
			const int NodeCount = NLI.m_NodeCount;
			const ComparePredicate sortcmp = NLI.m_sortcmp;

#if BVH_DEBUG
			{
				for (int node_i = 0 ; node_i < NodeCount ; node_i++)
					Assert(NodeList[node_i]->m_BestCost.m_Index == INVALID_INDEX);
			}
#endif

			for (int node_i = 0 ; node_i < NodeCount ; node_i++)
			{
				BvhConstructionNode * ni = NodeList[node_i];
				// Scan ahead to find a node that we can merge with.
				const int node_j_max = Min(node_i+LOOK_AHEAD,NodeCount);
				for (int node_j = node_i + 1 ; node_j < node_j_max ; node_j++)
				{
					BvhConstructionNode * nj = NodeList[node_j];
					Assert(ni->m_BestCost.m_Index != node_j);
					Assert(nj->m_BestCost.m_Index != node_i);
					if (MaxPrimsPerNodeMode == MAX_PRIMS_MODE_TEST_ALL || ni->m_PrimCount + nj->m_PrimCount <= MaxPrimsPerNode)
					{
					// Compute the merge cost and keep the pair if the cost is better than their existing costs.
					// We want to merge a pair only if, for each node in the pair, its merge cost with its sibling
					// node is better than its merge cost with all other nodes. For example, consider nodes n1,n2, and n3. 
					// The cost function is symmetric so cost(ni,nj) == cost(nj,ni). If cost(n1,n2) <= cost(n1,nx) 
					// and cost(n1,n2) <= cost(nx,n2) for all nodes nx then n1 and n2 will be merged. Any other node
					// that has either n1 or n2 as an optimal merge sibling, i.e. cost(n3,n1) <= cost(n3,nx) for all nx,
					// will not be merged on this pass. We don't want to take the next best thing yet. We'll wait for
					// the next pass to see if we get something better.
					const BvhConstructionNode::CostInfo cost = ComputeMergeCost(ni,nj,NLI,sortcmp);
					const bool Better1 = IsCostBetter(cost,ni->m_BestCost);
					const bool Better2 = IsCostBetter(cost,nj->m_BestCost);
					if (Better1)
					{
						// Invalidate ni's current merging pair.
						if (ni->m_BestCost.m_Index != INVALID_INDEX)
							NodeList[ni->m_BestCost.m_Index]->m_BestCost.m_Index = INVALID_INDEX;
						ni->m_BestCost = cost;
					}
					if (Better2)
					{
						// Invalidate nj's current merging pair.
						if (nj->m_BestCost.m_Index != INVALID_INDEX)
							NodeList[nj->m_BestCost.m_Index]->m_BestCost.m_Index = INVALID_INDEX;
						nj->m_BestCost = cost;
					}
					if (Better1 && Better2)
					{
						// Set this pair to merge only if both cost better.
						ni->m_BestCost.m_Index = (s16)node_j;
						nj->m_BestCost.m_Index = (s16)node_i;
					}
					}
				}
			}

			// Iterate over the node list and create new nodes for the merged pairs.
			// Keep the node list sorted as we pull out merged pairs and insert a parent node.
			// Recompute the NLI for the node list.
			BottomUpResetNLI(NLI);
			int dest_i = 0;
			for (int src_i = 0 ; src_i < NodeCount ; src_i++)
			{
				BvhConstructionNode * ni = NodeList[src_i];
				if (ni)
				{
					if (ni->m_BestCost.m_Index != INVALID_INDEX)
					{
						Assert(ni->m_BestCost.m_Index > src_i);
						Assert(ni->m_BestCost.m_Index < NodeCount);
						BvhConstructionNode * nj = NodeList[ni->m_BestCost.m_Index];
						Assert(nj->m_BestCost.m_Index == src_i);
						if (MaxPrimsPerNode == INVALID_INDEX || ni->m_PrimCount + nj->m_PrimCount <= MaxPrimsPerNode)
						{
							BvhConstructionNode * parent = AllocNode();
							MergeNodes(parent,ni,nj);

							NodeList[src_i] = parent;
							NodeList[ni->m_BestCost.m_Index] = NULL;
						}
						else
						{
							ResetBottomUpCost(&ni->m_BestCost);
							nj->m_BestCost.m_Index = INVALID_INDEX;
						}
					}
					else
						ResetBottomUpCost(&ni->m_BestCost);

					// Merge node into the node list and keep the list sorted.
					ni = NodeList[src_i];
					Assert(ni);
					BottomUpExpandNLI(NLI,*ni);
					int index = dest_i;
					while (index > 0 && sortcmp(ni,NodeList[index-1]))
					{
						NodeList[index] = NodeList[index-1];
						index--;
					}
					NodeList[index] = ni;
					dest_i++;
				}
			}
			Assert(NLI.m_NodeCount > 0);
			VerifySort(NodeList,NLI.m_NodeCount,sortcmp);
			if (NLI.m_NodeCount == NodeCount)
			{
				Assert(MaxPrimsPerNode != INVALID_INDEX);
				return NULL;
			}
		}

		BvhConstructionNode * root = NodeList[0];
		return root;
	}

	__forceinline bool CmpAABBMin(const BvhConstructionNode * n1, const BvhConstructionNode * n2, const ComparePredicate & sortcmp)
	{
		return CmpLT(n1->m_CAABBMin,n2->m_CAABBMin,sortcmp.m_axis1,sortcmp.m_axis2);
	}

	__forceinline bool CmpAABBMax(const BvhConstructionNode * n1, const BvhConstructionNode * n2, const ComparePredicate & sortcmp)
	{
		return CmpGT(n1->m_CAABBMax,n2->m_CAABBMax,sortcmp.m_axis1,sortcmp.m_axis2);
	}

	void InitStartNodes(BvhConstructionNode ** NodeList, const int NodeListCount, const ComparePredicate & sortcmp)
	{
		// Determine the best starting nodes for ConstructTopDown.
		Assert(NodeListCount > 1);
		int minNodeIndex = 0;
		int maxNodeIndex = 0;
		for (int node_i = 1 ; node_i < NodeListCount ; node_i++)
		{
			BvhConstructionNode * node = NodeList[node_i];
			if (CmpAABBMin(node,NodeList[minNodeIndex],sortcmp))
				minNodeIndex = node_i;
			if (CmpAABBMax(node,NodeList[maxNodeIndex],sortcmp))
				maxNodeIndex = node_i;
		}
		if (minNodeIndex == maxNodeIndex)
		{
			minNodeIndex = 0;
			maxNodeIndex = NodeListCount - 1;
		}

		// Reposition the starting nodes to be at the beginning and end of NodeList 
		// while keeping the rest of the list sorted.
		BvhConstructionNode * minNode = NodeList[minNodeIndex];
		BvhConstructionNode * maxNode = NodeList[maxNodeIndex];
		for (int node_i = minNodeIndex ; node_i > 0 ; node_i--)
			NodeList[node_i] = NodeList[node_i-1];
		NodeList[0] = minNode;
		if (maxNodeIndex < minNodeIndex)
			maxNodeIndex++;
		for (int node_i = maxNodeIndex ; node_i < NodeListCount - 1 ; node_i++)
			NodeList[node_i] = NodeList[node_i+1];
		NodeList[NodeListCount-1] = maxNode;
	}

	bool IsLeaf(BvhConstructionNode * node)
	{
		Assert(node->m_PrimCount > 0);
		if (node->m_PrimCount == 1)
		{
			Assert(node->m_Child[0] == NULL);
			Assert(node->m_Child[1] == NULL);
			Assert(node->m_PrimIndex != INVALID_INDEX);
			return true;
		}
		else
		{
			Assert(node->m_Child[0]);
			Assert(node->m_Child[1]);
			Assert(node->m_PrimIndex == INVALID_INDEX);
			return false;
		}
	}

	void InitNLI(NodeListInfo & retv, const BvhConstructionNode & node, const ComparePredicate & sortcmp, BvhConstructionNode ** NodeList)
	{
		retv.m_AABBMin = node.m_AABBMin;
		retv.m_AABBMax = node.m_AABBMax;
		const Vec3V center = Average(node.m_AABBMin,node.m_AABBMax);
		retv.m_CenterSum = center;
		retv.m_CenterSqSum = center * center;
		retv.m_CAABBMin = node.m_CAABBMin;
		retv.m_CAABBMax = node.m_CAABBMax;
		retv.m_NodeCount = 1;
		retv.m_QVolume = node.m_QVolume;
		retv.m_sortcmp = sortcmp;
		retv.m_NodeList = NodeList;
	}

	void ExpandNLI(NodeListInfo & nli, const BvhConstructionNode & node)
	{
		nli.m_AABBMin = Min(nli.m_AABBMin,node.m_AABBMin);
		nli.m_AABBMax = Max(nli.m_AABBMax,node.m_AABBMax);
		const Vec3V center = Average(node.m_AABBMin,node.m_AABBMax);
		nli.m_CenterSum = nli.m_CenterSum + center;
		nli.m_CenterSqSum = nli.m_CenterSqSum + center * center;
		nli.m_CAABBMin = Min(nli.m_CAABBMin,node.m_CAABBMin);
		nli.m_CAABBMax = Max(nli.m_CAABBMax,node.m_CAABBMax);
		nli.m_NodeCount = nli.m_NodeCount + 1;
	}

	struct TDCostInfo
	{
		float m_QVolume;
		float m_Cost;
#if USE_INSIDE_OPTIMIZATION
		bool m_Inside;
#endif
	};

	void TopDownExpandNLI(NodeListInfo & nli, const BvhConstructionNode & node, const TDCostInfo & cost)
	{
#if USE_INSIDE_OPTIMIZATION
		if (cost.m_Inside)
		{
			Assert(nli.m_QVolume == cost.m_QVolume);
			const Vec3V center = Average(node.m_AABBMin,node.m_AABBMax);
			nli.m_CenterSum = nli.m_CenterSum + center;
			nli.m_CenterSqSum = nli.m_CenterSqSum + center * center;
			nli.m_NodeCount = nli.m_NodeCount + 1;
		}
		else
#endif
		{
			ExpandNLI(nli,node);
			nli.m_QVolume = cost.m_QVolume;
#if BVH_DEBUG
			Assert(nli.m_QVolume == ComputeQVolume(nli.m_AABBMin,nli.m_AABBMax,m_QV));
#endif
		}
	}

	TDCostInfo InitTDCost()
	{
		TDCostInfo retv;
		retv.m_QVolume = INFINITE_QVOLUME;
		retv.m_Cost = INFINITE_COST;
#if USE_INSIDE_OPTIMIZATION
		retv.m_Inside = false;
#endif
		return retv;
	}

	__forceinline float ComputeSideCost(const float sideQVolume, const float parentQVolume, const int nodeCount)
	{
		return sideQVolume / parentQVolume * nodeCount;
	}

	TDCostInfo ComputeTDCost(const NodeListInfo & sideNLI, const BvhConstructionNode & node, const NodeListInfo & parentNLI)
	{
		// Compute the qcost for the side. This represents the average number of overlap tests for this side if we recurse down this side.
		const Vec3V sideAABBMin = Min(sideNLI.m_AABBMin,node.m_AABBMin);
		const Vec3V sideAABBMax = Max(sideNLI.m_AABBMax,node.m_AABBMax);
		const float sideQVolume = ComputeQVolume(sideAABBMin,sideAABBMax,m_QV);

		//const float sideUnormalizedQCost = sideNLI.m_UnormalizedQCost + node.m_QCost * node.m_QVolume;
		//const int sideNodeCount = sideNLI.m_NodeCount + 1;
		//const float sideQCost = sideUnormalizedQCost / sideQVolume + float(sideNodeCount);
		// Compute the qcost for the side with respect to its parent. This takes into account the probability that the query
		// volume intersects the side and includes the cost of the overlap test.
		//retv.m_QCost = ComputeQCost(sideQCost,sideQVolume,parentNLI.m_QVolume);

		TDCostInfo retv;
		retv.m_QVolume = sideQVolume;
		retv.m_Cost = ComputeSideCost(sideQVolume,parentNLI.m_QVolume,sideNLI.m_NodeCount+1);
#if USE_INSIDE_OPTIMIZATION
		retv.m_Inside = IsGreaterThanOrEqualAll(node.m_AABBMin,sideNLI.m_AABBMin) && IsLessThanOrEqualAll(node.m_AABBMax,sideNLI.m_AABBMax);
		if (retv.m_Inside)
			m_InsideCounter++;
#endif
		return retv;
	}

	__forceinline bool IsTDCostBetter(const TDCostInfo & cost1, const TDCostInfo & cost2, const int side, const BvhConstructionNode * node1, const BvhConstructionNode * node2, const ComparePredicate & sortcmp)
	{
		if (cost1.m_Cost < cost2.m_Cost)
			return true;
		else if (cost1.m_Cost == cost2.m_Cost)
		{
			const bool closer = (side == MIN_SIDE) ? CmpAABBMin(node1,node2,sortcmp) 
												   : CmpAABBMax(node1,node2,sortcmp);
			//const bool closer = (side == MIN_SIDE) ? CmpAABBMax(node2,node1,sortcmp) 
			//									   : CmpAABBMin(node2,node1,sortcmp);
			return closer;
		}
		else
			return false;
	}

	void InitNode(BvhConstructionNode * parent, BvhConstructionNode * child0, BvhConstructionNode * child1, const NodeListInfo & NLI)
	{
		parent->m_Child[0] = child0;
		parent->m_Child[1] = child1;
		parent->m_AABBMin = NLI.m_AABBMin;
		parent->m_AABBMax = NLI.m_AABBMax;
		parent->m_CAABBMin = NLI.m_CAABBMin;
		parent->m_CAABBMax = NLI.m_CAABBMax;
		parent->m_PrimCount = child0->m_PrimCount + child1->m_PrimCount;
		parent->m_PrimIndex = INVALID_INDEX;
		parent->m_QVolume = NLI.m_QVolume;
		ResetBottomUpCost(&parent->m_BestCost);
	}

	int DetermineBestSide(const TDCostInfo bestCost[2], const float currentCost[2], const NodeListInfo sideNLI[2])
	{
		// Determine the cost for adding the node to each side and choose the one with the least cost.
		const float cost0 = bestCost[0].m_Cost + currentCost[1];
		const float cost1 = currentCost[0] + bestCost[1].m_Cost;
		if (cost0 < cost1)
			return MIN_SIDE;
		else if (cost0 > cost1)
			return MAX_SIDE;
		else
		{
			// Both sides increase the cost by the same amount.
			// Pick the side that minimizes the maximum of the 2 costs.
			const float max0 = Max(bestCost[0].m_Cost,currentCost[1]);
			const float max1 = Max(currentCost[0],bestCost[1].m_Cost);
			if (max0 < max1)
				return MIN_SIDE;
			else if (max0 > max1)
				return MAX_SIDE;
			else
			{
				// Total costs are equal: (n1+1)L1_ - n1*L1 == (n2+1)L2_ - n2*L2
				// Next cost1 equals next cost2: (n1+1)L1_ == (n2+1)L2_
				// Initial cost1 equals initial cost2: n1*L1 == n2*L2
				// Both sides have the same resulting cost and same initial cost.

				// Pick the side with the smallest volume.
				if (bestCost[0].m_QVolume < bestCost[1].m_QVolume)
					return MIN_SIDE;
				else if (bestCost[0].m_QVolume > bestCost[1].m_QVolume)
					return MAX_SIDE;
				else
				{
					// Pick the side with least number of nodes.
					if (sideNLI[0].m_NodeCount < sideNLI[1].m_NodeCount)
						return MIN_SIDE;
					else if (sideNLI[0].m_NodeCount > sideNLI[1].m_NodeCount)
						return MAX_SIDE;
					else
					{
						m_TopDownAmbiguousCount++;
						return MIN_SIDE;
					}
				}
			}
		}
	}

	BvhConstructionNode * ConstructTopDownRecurse(NodeListInfo & NLI, const int targetPrimitivesPerLeaf, const int depth, const int LOOK_AHEAD)
	{
		Assert(NLI.m_NodeCount > 0);
		//if (depth % 2 == 1)
		//if (depth >= m_budepth)
		//if (NLI.m_NodeCount <= 64)
		{
		//	ConstructBottomUp(NLI,targetPrimitivesPerLeaf,INVALID_INDEX,BOTTOM_UP_LOOK_AHEAD);//1 << (depth + 1));
		//ConstructBottomUp(NLI,targetPrimitivesPerLeaf,1<<(depth+1),BOTTOM_UP_LOOK_AHEAD);//1 << (depth + 1));
		}
		Assert(NLI.m_NodeCount > 0);
	
		// If we only have one node then were done.
		if (NLI.m_NodeCount == 1)
			return NLI.m_NodeList[0];

		UpdateSort(NLI);

		BvhConstructionNode ** NodeList = NLI.m_NodeList;
		const int NodeCount = NLI.m_NodeCount;
		const ComparePredicate sortcmp = NLI.m_sortcmp;

		// Find the initial nodes for each side. This step is critical. If we pick bad starting nodes
		// then the partition will not be very good no matter how well the subsequent node selection is.
		InitStartNodes(NodeList,NodeCount,sortcmp);

		// Init the Node List Infos and Costs for each side. The min side nodes will be stored in the first
		// part of the node list from [0,startindex-1] while the max side nodes are stored in [endIndex,NodeCount-1].
		// Initial nodes for each side are stored in NodeList[0] and NodeList[NodeCount-1].
		NodeListInfo sideNLI[2];
		float currentCost[2];
		InitNLI(sideNLI[0],*NodeList[0],NLI.m_sortcmp,NULL);
		InitNLI(sideNLI[1],*NodeList[NodeCount-1],NLI.m_sortcmp,NULL);
		currentCost[0] = ComputeSideCost(sideNLI[0].m_QVolume,NLI.m_QVolume,1);
		currentCost[1] = ComputeSideCost(sideNLI[1].m_QVolume,NLI.m_QVolume,1);
		
		// Vars to keep track of the best node for each side. Our strategy is to search for a node to add to each
		// side. From the 2 lowest cost nodes we find, one for each side, we only add the one that keeps the total
		// cost to a minimum. We store the other node for the next iteration so we don't have to search for it again.
		TDCostInfo bestCost[2];
		int bestIndex[2];
		bestIndex[0] = INVALID_INDEX;
		bestCost[0] = InitTDCost();
		bestIndex[1] = INVALID_INDEX;
		bestCost[1] = InitTDCost();

		int startIndex = 1;				// Min nodes are stored in NodeList[0..startIndex-1]
		int endIndex = NodeCount - 1;	// Max nodes are stored in NodeList[startIndex..NodeCount-1]
		while (startIndex < endIndex)
		{
			const int minIndex[2] = {startIndex,Max(startIndex,endIndex-LOOK_AHEAD)};
			const int maxIndex[2] = {Min(startIndex+LOOK_AHEAD,endIndex),endIndex};

			// Scan ahead to find the lowest cost nodes to add to each side.
			for (int side = 0 ; side < 2 ; side++)
			{
				if (bestIndex[side] == INVALID_INDEX)
				{
					// This side doesn't have a cached node so we need to search for one.
					BvhConstructionNode * node = NodeList[minIndex[side]];
					bestIndex[side] = minIndex[side];
					bestCost[side] = ComputeTDCost(sideNLI[side],*node,NLI);
					for (int index = minIndex[side] + 1 ; index < maxIndex[side] ; index++)
					{
						node = NodeList[index];
						const TDCostInfo cost = ComputeTDCost(sideNLI[side],*node,NLI);
						if (IsTDCostBetter(cost,bestCost[side],side,node,NodeList[bestIndex[side]],sortcmp))
						{
							bestIndex[side] = index;
							bestCost[side] = cost;
						}
					}
				}
				FastAssert(bestIndex[side] != INVALID_INDEX);
				FastAssert(bestIndex[side] >= startIndex && bestIndex[side] < endIndex);
			}

			const int bestSide = DetermineBestSide(bestCost,currentCost,sideNLI);
			FastAssert(bestSide == MIN_SIDE || bestSide == MAX_SIDE);

			if (bestSide == MIN_SIDE)
			{
				const int insertIndex = bestIndex[0];
				BvhConstructionNode * insertNode = NodeList[insertIndex];

				// Expand the min NLI.
				TopDownExpandNLI(sideNLI[0],*insertNode,bestCost[0]);
				currentCost[0] = bestCost[0].m_Cost;

				// Insert the node into the min list keeping things sorted.
				int index = startIndex;
				while (index > 0 && sortcmp(insertNode,NodeList[index-1]))
					index--;
				FastAssert(index >= 0);
				for (int node_i = insertIndex ; node_i > index ; node_i--)
					NodeList[node_i] = NodeList[node_i-1];
				NodeList[index] = insertNode;
				startIndex++;

				// Fix up the max side index if it was less than or equal to the min index.
				if (bestIndex[1] == bestIndex[0])
				{
					bestIndex[1] = INVALID_INDEX;
					bestCost[1] = InitTDCost();
				}
				else if (bestIndex[1] < bestIndex[0])
					bestIndex[1]++;

				// Reset cost.
				bestIndex[0] = INVALID_INDEX;
				bestCost[0] = InitTDCost();
			}
			else
			{
				const int insertIndex = bestIndex[1];
				BvhConstructionNode * insertNode = NodeList[insertIndex];
				
				// Expand the max NLI.
				TopDownExpandNLI(sideNLI[1],*insertNode,bestCost[1]);
				currentCost[1] = bestCost[1].m_Cost;

				// Insert the node into the max list keeping things sorted.
				int index = endIndex - 1;
				while (index < NodeCount - 1 && sortcmp(NodeList[index+1],insertNode))
					index++;
				FastAssert(index < NodeCount);
				for (int node_i = insertIndex ; node_i < index ; node_i++)
					NodeList[node_i] = NodeList[node_i+1];
				NodeList[index] = insertNode;
				endIndex--;

				// Fix up the min side index if it was greater than or equal to the max index.
				if (bestIndex[0] == bestIndex[1])
				{
					bestIndex[0] = INVALID_INDEX;
					bestCost[0] = InitTDCost();
				}
				else if (bestIndex[0] > bestIndex[1])
					bestIndex[0]--;

				// Reset cost.
				bestIndex[1] = INVALID_INDEX;
				bestCost[1] = InitTDCost();
			}
		}
		Assert(sideNLI[0].m_NodeCount > 0);
		Assert(sideNLI[1].m_NodeCount > 0);
		Assert(sideNLI[0].m_NodeCount + sideNLI[1].m_NodeCount == NodeCount);
		sideNLI[0].m_NodeList = NodeList;
		sideNLI[1].m_NodeList = NodeList + sideNLI[0].m_NodeCount;
		VerifySort(sideNLI[0].m_NodeList,sideNLI[0].m_NodeCount,sortcmp);
		VerifySort(sideNLI[1].m_NodeList,sideNLI[1].m_NodeCount,sortcmp);

		//if (depth % 2 == 0)
		{
			//ConstructBottomUp(NLI,targetPrimitivesPerLeaf,INVALID_INDEX,BOTTOM_UP_LOOK_AHEAD);//1 << (depth + 1));
		//	ConstructBottomUp(sideNLI[0],targetPrimitivesPerLeaf,1<<(depth),BOTTOM_UP_LOOK_AHEAD);
		//	ConstructBottomUp(sideNLI[1],targetPrimitivesPerLeaf,1<<(depth),BOTTOM_UP_LOOK_AHEAD);
		}

		BvhConstructionNode * parent = AllocNode();
		BvhConstructionNode * child0 = ConstructTopDownRecurse(sideNLI[0],targetPrimitivesPerLeaf,depth+1,LOOK_AHEAD);
		BvhConstructionNode * child1 = ConstructTopDownRecurse(sideNLI[1],targetPrimitivesPerLeaf,depth+1,LOOK_AHEAD);
		InitNode(parent,child0,child1,NLI);
		return parent;
	}

	BvhConstructionNode * ConstructBVH(const int targetPrimitivesPerLeaf)
	{
#if USE_INSIDE_OPTIMIZATION
		m_InsideCounter = 0;
#endif
		m_TopDownAmbiguousCount = 0;
		m_BottomUpAmbiguousCount = 0;
#if 1
		// Bottom up only.
		//ConstructBottomUp(m_totalNLI,targetPrimitivesPerLeaf,targetPrimitivesPerLeaf,MAX_PRIMS_MODE_TEST_ALL,2*BOTTOM_UP_LOOK_AHEAD);
		//ConstructBottomUp(m_totalNLI,targetPrimitivesPerLeaf,targetPrimitivesPerLeaf,MAX_PRIMS_MODE_TEST_PRIM_COUNT,2*BOTTOM_UP_LOOK_AHEAD);
		BvhConstructionNode * root = ConstructBottomUp(m_totalNLI,targetPrimitivesPerLeaf,INVALID_INDEX,MAX_PRIMS_MODE_TEST_ALL,BOTTOM_UP_LOOK_AHEAD);
#elif 1
		// Top down only.
		//ConstructBottomUp(m_totalNLI,targetPrimitivesPerLeaf,targetPrimitivesPerLeaf,MAX_PRIMS_MODE_TEST_ALL,2*BOTTOM_UP_LOOK_AHEAD);
		//ConstructBottomUp(m_totalNLI,targetPrimitivesPerLeaf,targetPrimitivesPerLeaf,MAX_PRIMS_MODE_TEST_PRIM_COUNT,2*BOTTOM_UP_LOOK_AHEAD);
		BvhConstructionNode * root = ConstructTopDownRecurse(m_totalNLI,targetPrimitivesPerLeaf,1,TOP_DOWN_LOOK_AHEAD);
#else
		// Mixed bottom up and top down.
		//ConstructBottomUp(m_totalNLI,targetPrimitivesPerLeaf,targetPrimitivesPerLeaf,64);
		//m_budepth = int(log(float(m_totalNLI.m_NodeCount)) / log(2.0f) / 2.0f + 1.0f);
		m_budepth = int(log(float(2*m_totalNLI.m_NodeCount)) / log(2.0f) / 2.0f);
		// log(2,2n) / 2
		// = log(b,2n)/log(b,2)/2
		// 2^(log(2,2n)/2)
		// = sqrtf(2n)
		//const int maxp = int(sqrtf(float(2*m_totalNLI.m_NodeCount)));
		//ConstructBottomUp(m_totalNLI,targetPrimitivesPerLeaf,maxp,BOTTOM_UP_LOOK_AHEAD);
		BvhConstructionNode * root = ConstructTopDownRecurse(m_totalNLI,targetPrimitivesPerLeaf,1,TOP_DOWN_LOOK_AHEAD);
#endif
		return root;
	}

	int GetNodeCount(BvhConstructionNode * root, const int targetPrimitivesPerLeaf)
	{
		if (root->m_PrimCount <= targetPrimitivesPerLeaf)
			return 1;
		else
		{
			Assert(root->m_Child[0]);
			Assert(root->m_Child[1]);
			return GetNodeCount(root->m_Child[0],targetPrimitivesPerLeaf) + GetNodeCount(root->m_Child[1],targetPrimitivesPerLeaf) + 1;
		}
	}

	phOptimizedBvhNode * BuildTree(BvhConstructionNode * root, phOptimizedBvh * bvhStructure, BvhPrimitiveData * leafNodes, const int PrimNodeCount, const int MinPrimNodeCount, const int targetPrimitivesPerLeaf, const u32 remapPrimitivesMask)
	{
		phOptimizedBvh::BuildTreeInfo info;
		info.leafNodes = leafNodes;
		info.tempLeafNodes = m_TempLeafNodes;
		info.primCount = 0;
		info.maxPrimCount = PrimNodeCount;
		info.nodeCount = GetNodeCount(root,targetPrimitivesPerLeaf);
		info.minNodeCount = MinPrimNodeCount;
		info.targetPrimitivesPerLeaf = targetPrimitivesPerLeaf;
		info.remapPrimitivesMask = remapPrimitivesMask;
		phOptimizedBvhNode * root1 = bvhStructure->buildTreeFromBvhConstruction(root,&info);
		return root1;
	}

	struct BvhStats
	{
		int m_NodeCount;
		int m_PrimCount;
		int m_Depth;
		float m_QCost;
		float m_QVolume;
	};

	__forceinline float ComputeLeafQCost(const int primCount)
	{
		return (primCount <= 1) ? 0 : float(primCount);
	}

	void ComputeStats(BvhConstructionNode * root, BvhStats * stats, Vec3V_In QV, const int targetPrimitivesPerLeaf)
	{
		Assert(root);
		const float QVolume = ComputeQVolume(root->m_AABBMin,root->m_AABBMax,QV);
		if (!IsLeaf(root))
		{
			Assert(root->m_Child[1]);
			BvhStats s1;
			BvhStats s2;
			ComputeStats(root->m_Child[0],&s1,QV,targetPrimitivesPerLeaf);
			ComputeStats(root->m_Child[1],&s2,QV,targetPrimitivesPerLeaf);
			const int primCount = s1.m_PrimCount + s2.m_PrimCount;
			Assert(primCount > 1);
			const float QCost = ComputeQCost(s1.m_QCost,s1.m_QVolume,QVolume) + ComputeQCost(s2.m_QCost,s2.m_QVolume,QVolume);
			const float LeafQCost = ComputeLeafQCost(primCount);
			bool isLeaf = false;
			if (targetPrimitivesPerLeaf == INVALID_INDEX)
			{
				// Use the lesser of subtree qcost or leafnode qcost.
				if (LeafQCost <= QCost)
					isLeaf = true;
			}
			else if (primCount <= targetPrimitivesPerLeaf)
				isLeaf = true;

			if (isLeaf)
			{
				stats->m_NodeCount = 1;
				stats->m_PrimCount = primCount;
				stats->m_Depth = 1;
				stats->m_QCost = LeafQCost;
				stats->m_QVolume = QVolume;
			}
			else
			{
				stats->m_NodeCount = s1.m_NodeCount + s2.m_NodeCount + 1;
				stats->m_PrimCount = primCount;
				stats->m_Depth = Max(s1.m_Depth,s2.m_Depth) + 1;
				stats->m_QCost = QCost;
				stats->m_QVolume = QVolume;
			}
		}
		else
		{
			Assert(root->m_Child[1] == NULL);
			Assert(root->m_PrimCount == 1);
			stats->m_NodeCount = 1;
			stats->m_PrimCount = 1;
			stats->m_Depth = 1;
			stats->m_QCost = ComputeLeafQCost(1);
			stats->m_QVolume = QVolume;
		}
	}
};

#define BVH_TIMER(x) //x

BVH_TIMER(float g_ConstructionTime0;)
BVH_TIMER(float g_ConstructionTime1;)
BVH_TIMER(float g_ConstructionTime2;)
BVH_TIMER(float g_ConstructionTime3;)

phOptimizedBvhNode * BuildBvh(phOptimizedBvh * bvhStructure, BvhPrimitiveData *leafNodes, const int PrimNodeCount, const int MinPrimNodeCount, const int targetPrimitivesPerLeaf, const u32 remapPrimitivesMask)
{
	Assert(PrimNodeCount > 0);

	BvhConstructor g_BC;

#if USE_OLD_BVH_CONSTRUCTION
	
	BVH_TIMER(u64 ElapsedTicks = sysTimer::GetTicks();)
	phOptimizedBvhNode * root1 = bvhStructure->buildTree(leafNodes, 0, PrimNodeCount, targetPrimitivesPerLeaf, remapPrimitivesMask, -1);
	BVH_TIMER(ElapsedTicks = sysTimer::GetTicks() - ElapsedTicks;)
	BVH_TIMER(g_ConstructionTime1 = float(ElapsedTicks) * sysTimer::GetTicksToSeconds();)

#else // USE_OLD_BVH_CONSTRUCTION

	BVH_TIMER(u64 ElapsedTicks = sysTimer::GetTicks();)
	g_BC.AllocMem(PrimNodeCount);
	g_BC.InitNodeList(bvhStructure,leafNodes,PrimNodeCount);
	BVH_TIMER(ElapsedTicks = sysTimer::GetTicks() - ElapsedTicks;)
	BVH_TIMER(g_ConstructionTime0 = float(ElapsedTicks) * sysTimer::GetTicksToSeconds();)

	BVH_TIMER(ElapsedTicks = sysTimer::GetTicks();)
	BvhConstructionNode * root = g_BC.ConstructBVH(targetPrimitivesPerLeaf);
	BVH_TIMER(ElapsedTicks = sysTimer::GetTicks() - ElapsedTicks;)
	BVH_TIMER(g_ConstructionTime1 = float(ElapsedTicks) * sysTimer::GetTicksToSeconds();)

	BVH_TIMER(ElapsedTicks = sysTimer::GetTicks();)
	phOptimizedBvhNode * root1 = g_BC.BuildTree(root,bvhStructure,leafNodes,PrimNodeCount,MinPrimNodeCount,targetPrimitivesPerLeaf,remapPrimitivesMask);
	BVH_TIMER(ElapsedTicks = sysTimer::GetTicks() - ElapsedTicks;)
	BVH_TIMER(g_ConstructionTime2 = float(ElapsedTicks) * sysTimer::GetTicksToSeconds();)

#if BVH_TEST
	static BvhConstructor::BvhStats stats[8];
	g_BC.ComputeStats(root,stats+0,g_BC.m_QV,BvhConstructor::INVALID_INDEX);
	for (int i = 1 ; i < 8 ; i++)
		g_BC.ComputeStats(root,stats+i,g_BC.m_QV,i);
#endif

	BVH_TIMER(ElapsedTicks = sysTimer::GetTicks();)
	g_BC.FreeMem();
	BVH_TIMER(ElapsedTicks = sysTimer::GetTicks() - ElapsedTicks;)
	BVH_TIMER(g_ConstructionTime3 = float(ElapsedTicks) * sysTimer::GetTicksToSeconds();)

#endif // USE_OLD_BVH_CONSTRUCTION

	return root1;
}

}// namespace rage
