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

#ifndef OPTIMIZED_BVH_H
#define OPTIMIZED_BVH_H

#include "primitives.h"
#include "math/amath.h"
#include "grprofile/drawcore.h"
#include "vector/vector3.h"
#include "vectormath/classes.h"

#if RSG_CPU_INTEL
#include "math/amath.h"
#endif

#if RSG_CPU_SPU
#include "system/dma.h"
#include <cell/dma.h>
#endif

//http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclang/html/vclrf__m128.asp

namespace rage
{

	struct BvhConstructionNode;

// BVH structures get built from a list of BvhPrimitiveData.  Note that BvhPrimitiveData are used only during the building process and are not stored anywhere.
class BvhPrimitiveData
{
public:
	void Clear()
	{
		m_PrimitiveIndex = (phPolygon::Index)(-1);
	}

	s16 m_AABBMin[3];
	s16 m_Centroid[3];
#if !POLYGON_INDEX_IS_U32
	u16 m_Pad0;
#endif	// !POLYGON_INDEX_IS_U32
	phPolygon::Index	m_PrimitiveIndex;
	s16 m_AABBMax[3];

} ;


// phOptimizedBvhNode is a single node in a BVH tree as used by phOptimizedBvh.
// Only leaf nodes 'contain' polygons.  Leaf nodes track which polygons they contain by recording a starting polygon
//   index and a count.
#define MAX_SUBTREE_SIZE 2048


static __forceinline bool TestQuantizedAabbAgainstAabb(s32 boxMin00, s32 boxMin01, s32 boxMin02, s32 boxMax00, s32 boxMax01, s32 boxMax02, s32 boxMin10, s32 boxMin11, s32 boxMin12, s32 boxMax10, s32 boxMax11, s32 boxMax12)
{
	int d0 = boxMax10 - boxMin00;
	int d1 = boxMax11 - boxMin01;
	int d2 = boxMax12 - boxMin02;
	int d3 = boxMax00 - boxMin10;
	int d4 = boxMax01 - boxMin11;
	int d5 = boxMax02 - boxMin12;

	// If any of the above values are negative then we want to return false.
	int combinedValue = (d0 | d1 | d2 | d3 | d4 | d5);
	const bool aabbOverlap = (combinedValue > 0);
	return aabbOverlap;
}


// Static function (pilfered from CellMidphaseTaskHandler.cpp) for checking overlap between two quantized AABBs.  
static bool TestQuantizedAabbAgainstAabb(const s16 boxMin0[3], const s16 boxMax0[3], const s16 boxMin1[3], const s16 boxMax1[3])
{
#if RSG_CPU_SPU
	qword q0,q1,q2,q3,q4,q5,q6,q7;
	qword t0,t1,t2,t3;
	qword splat,align;
	//bool Test;
	asm( "cdd %0,0($1)" : "=r"(align) );
	splat = si_ilh( 0x303 );

	q0 = si_lqd( si_from_ptr(boxMin0),0 );
	q1 = si_lqd( si_from_ptr(boxMin0),16 );
	q2 = si_lqd( si_from_ptr(boxMin1),0 );
	q3 = si_lqd( si_from_ptr(boxMin1),16 );
	q4 = si_lqd( si_from_ptr(boxMax0),0 );
	q5 = si_lqd( si_from_ptr(boxMax0),16 );
	q6 = si_lqd( si_from_ptr(boxMax1),0 );
	q7 = si_lqd( si_from_ptr(boxMax1),16 );

	t0 = si_andi( si_from_ptr(boxMin0),15 );
	t1 = si_andi( si_from_ptr(boxMin1),15 );
	t2 = si_andi( si_from_ptr(boxMax0),15 );
	t3 = si_andi( si_from_ptr(boxMax1),15 );

	t0 = si_shufb( t0,t0,splat );
	t1 = si_shufb( t1,t1,splat );
	t2 = si_shufb( t2,t2,splat );
	t3 = si_shufb( t3,t3,splat );

	t0 = si_ah( t0,align );
	t1 = si_ah( t1,align );
	t2 = si_ah( t2,align );
	t3 = si_ah( t3,align );

	t0 = si_shufb( q0,q1,t0 );
	t1 = si_shufb( q2,q3,t1 );
	t2 = si_shufb( q4,q5,t2 );
	t3 = si_shufb( q6,q7,t3 );

	t0 = si_cgth( t0,t3 );			// no a<=b test, so use ~(a>b)
	t1 = si_cgth( t1,t2 );
	t0 = si_or( t0,t1 );			// Combining negatives, so use or rather than and
	t0 = si_gbh( t0 );				// Pack halfword results into bits..
	t0 = si_clgti( t0,255-128-64-32 );			// Fail if result has any X,Y,Z bit set 
	//t0 = si_ai( t0,1 );				// Invert result as bool

	return !( (bool)si_to_int( t0 ) );
	//return si_to_uchar( t0 );
	//asm( "ai %0,%1,1" : "=r"(Test) : "r"(t0) );
	//return Test;
#else	// RSG_CPU_SPU
	return TestQuantizedAabbAgainstAabb(boxMin0[0], boxMin0[1], boxMin0[2], boxMax0[0], boxMax0[1], boxMax0[2], boxMin1[0], boxMin1[1], boxMin1[2], boxMax1[0], boxMax1[1], boxMax1[2]);
#endif	// RSG_CPU_SPU
}



class phOptimizedBvhNode
{
public:
	phOptimizedBvhNode()
	{
		Clear();
	}

	phOptimizedBvhNode(datResource & rsc);
	DECLARE_PLACE(phOptimizedBvhNode);
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

	void Clear()
	{
		SetPolygonCount(0);
		SetEscapeIndex(1);
	}

	// NOTE: Maybe I should just define a operator= instead.
	__forceinline void Clone(const phOptimizedBvhNode &otherNode)
	{
#if !__TOOL
		CompileTimeAssert(sizeof(phOptimizedBvhNode) == 16);
		*(reinterpret_cast<Vec3V *>(this)) = *(reinterpret_cast<const Vec3V *>(&otherNode));
#else	// !__TOOL
		*this = otherNode;
#endif	// !__TOOL
	}

	__forceinline void SetAABB(s16 aabbMin0, s16 aabbMin1, s16 aabbMin2, s16 aabbMax0, s16 aabbMax1, s16 aabbMax2)
	{
		m_AABBMin[0] = aabbMin0;
		m_AABBMin[1] = aabbMin1;
		m_AABBMin[2] = aabbMin2;

		m_AABBMax[0] = aabbMax0;
		m_AABBMax[1] = aabbMax1;
		m_AABBMax[2] = aabbMax2;
	}

	__forceinline void SetAABB(const s16 aabbMin[3], const s16 aabbMax[3])
	{
		SetAABB(aabbMin[0], aabbMin[1], aabbMin[2], aabbMax[0], aabbMax[1], aabbMax[2]);
	}

	void MergeAABB(const s16 aabbMin[3], const s16 aabbMax[3])
	{
		m_AABBMin[0] = Min(m_AABBMin[0], aabbMin[0]);
		m_AABBMin[1] = Min(m_AABBMin[1], aabbMin[1]);
		m_AABBMin[2] = Min(m_AABBMin[2], aabbMin[2]);

		m_AABBMax[0] = Max(m_AABBMax[0], aabbMax[0]);
		m_AABBMax[1] = Max(m_AABBMax[1], aabbMax[1]);
		m_AABBMax[2] = Max(m_AABBMax[2], aabbMax[2]);
	}

	__forceinline void CombineAABBs(const phOptimizedBvhNode &node0, const phOptimizedBvhNode &node1)
	{
		m_AABBMin[0] = Min(node0.m_AABBMin[0], node1.m_AABBMin[0]);
		m_AABBMin[1] = Min(node0.m_AABBMin[1], node1.m_AABBMin[1]);
		m_AABBMin[2] = Min(node0.m_AABBMin[2], node1.m_AABBMin[2]);

		m_AABBMax[0] = Max(node0.m_AABBMax[0], node1.m_AABBMax[0]);
		m_AABBMax[1] = Max(node0.m_AABBMax[1], node1.m_AABBMax[1]);
		m_AABBMax[2] = Max(node0.m_AABBMax[2], node1.m_AABBMax[2]);
	}

	bool DoesQuantizedAABBOverlap(const s16 aabbMin[3], const s16 aabbMax[3]) const
	{
		return TestQuantizedAabbAgainstAabb(aabbMin, aabbMax, m_AABBMin, m_AABBMax);
	}

	bool IsLeafNode() const
	{
		return GetPolygonCount() != 0;
	}

	// Returns an integer mask (either 0x00000000 or 0xffffffff) indicating whether or not this node is a leaf node.
	__forceinline u32 IsLeafNodeMask() const
	{
		return GenerateMaskGZ(GetPolygonCount());
	}

	phPolygon::Index GetPolygonStartIndex() const
	{
		FastAssert(IsLeafNode());
		return (phPolygon::Index)(m_NodeData);
	}

	void SetPolygonStartIndex(const phPolygon::Index startIndex)
	{
		// Ensure that we've been 'set' to a leaf node before they try and set the start index.  This is just to help prevent errors.
		FastAssert(GetPolygonCount() > 0);
		m_NodeData = startIndex;
	}

	__forceinline int GetPolygonCount() const
	{
		return (u32)(m_PolygonCount);
	}

	__forceinline void SetPolygonCount(const u8 polygonCount)
	{
		m_PolygonCount = polygonCount;
	}

	__forceinline int GetEscapeIndex() const
	{
		const s32 isLeafMask = (-(s32)GetPolygonCount()) >> 31;
		const int escapeIndex = (~isLeafMask & m_NodeData) | (isLeafMask & 1);
		return escapeIndex;
	}

	__forceinline int GetEscapeIndexNonLeaf() const
	{
		// For leaf nodes we don't explicitly store an escape index; it's always implicitly 1.
		FastAssert(!IsLeafNode());
		return (int)(m_NodeData);
	}

	__forceinline u32 GetNodeData() const
	{
		return (u32)(m_NodeData);
	}

	void SetEscapeIndex(const u16 escapeIndex)
	{
		FastAssert(GetPolygonCount() == 0);
		m_NodeData = escapeIndex;
	}

	s16 m_AABBMin[3];
	s16 m_AABBMax[3];

private:
	// m_NodeData is a polygon start index if the node is a leaf node, and an escape index if it's not a leaf node.
	phPolygon::Index	m_NodeData;
	u8				m_PolygonCount;
} ;


///OptimizedBvh store an AABB tree that can be quickly traversed on CPU (and SPU, GPU in future)
class phOptimizedBvh
{
	phOptimizedBvhNode*	m_contiguousNodes;
	int					m_NumNodesInUse;

	int					m_numNodes;
	int					m_Pad0[1];

	Vector3 m_AABBMin, m_AABBMax;				// AABB for the space covered by this BVH as a whole.
	Vector3 m_AABBCenter;
	Vector3 m_Quantize, m_InvQuantize;

public:
	// Subtrees are complete subsets of the BVH that happen to be of, at most, a certain size (as specified by a #define in the .cpp file right now).
	// The reason this is useful is because it allows 
	class phBVHSubtreeInfo
	{
	public:
		// NOTE: Maybe I should just define a operator= instead.
		__forceinline void Clone(const phBVHSubtreeInfo &other)
		{
			CompileTimeAssert(sizeof(phBVHSubtreeInfo) == 16);
			*(reinterpret_cast<Vec3V *>(this)) = *(reinterpret_cast<const Vec3V *>(&other));
		}

		s16 m_AABBMin[3], m_AABBMax[3];
		u16 m_RootNodeIndex;
		u16 m_LastIndex;									// This is the index of the node that comes right *after* the last node in this subtree.
															// This should get renamed to m_EndIndex.
		phBVHSubtreeInfo();

		phBVHSubtreeInfo(datResource &rsc);
		DECLARE_PLACE(phBVHSubtreeInfo);
#if __DECLARESTRUCT
		void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

		void SetAABBFromNode(const phOptimizedBvhNode &node);

		bool DoesQuantizedAABBOverlap(const s16 aabbMin[3], const s16 aabbMax[3]) const
		{
			return TestQuantizedAabbAgainstAabb(aabbMin, aabbMax, m_AABBMin, m_AABBMax);
		}
	} ;

	const phBVHSubtreeInfo *GetSubtreeHeaders() const
	{
		return m_SubtreeHeaders;
	}

	int GetNumUsedSubtreeHeaders() const
	{
		return (int)(m_CurSubtreeHeaderIndex);
	}

	int GetNumSubtreeHeaders() const
	{
		return (int)(m_NumSubtreeHeaders);
	}

	Vector3::Param GetQuantize() const
	{
		return m_Quantize;
	}

	Vector3::Param GetInvQuantize() const
	{
		return m_InvQuantize;
	}

#if RSG_CPU_SPU
	void SetBvhNodesPtr(phOptimizedBvhNode *bvhNodes)
	{
		m_contiguousNodes = bvhNodes;
	}

	void SetSubtreeHeadersPtr(phBVHSubtreeInfo *subtreeHeaders)
	{
		m_SubtreeHeaders = subtreeHeaders;
	}
#endif	// RSG_CPU_SPU

private:
	phBVHSubtreeInfo *m_SubtreeHeaders;
	u16 m_NumSubtreeHeaders;
	u16 m_CurSubtreeHeaderIndex;

public:
	phOptimizedBvh();
	~phOptimizedBvh();

	phOptimizedBvh (datResource & rsc);
	DECLARE_PLACE(phOptimizedBvh);
#if __DECLARESTRUCT
	void DeclareStruct(datTypeStruct &s);
#endif // __DECLARESTRUCT

#if !__FINAL
	static float TestBvhBuildPerformance(int numPrimitives, int numTests);
#endif	// !__FINAL

	void SetExtents(Vector3::Param aabbMin, Vector3::Param aabbMax)
	{
		Assert(((Vector3)aabbMax).IsGreaterThan(aabbMin));
		m_AABBMin.Set(aabbMin);
		m_AABBMax.Set(aabbMax);
		m_AABBCenter.Average(aabbMin, aabbMax);

		Vector3 aabbSize(aabbMax);
		aabbSize.Subtract(aabbMin);

		m_Quantize = Vector3(65534.0f, 65534.0f, 65534.0f) / aabbSize;
		m_InvQuantize = aabbSize / Vector3(65534.0f, 65534.0f, 65534.0f);
	}

	// Quantize to the quantum unit closest to the input vector.
	void QuantizeClosest(s16 vecOut[3], const Vector3 &vecIn) const
	{
		Vector3 aabbCenter(m_AABBCenter);
#if RSG_CPU_INTEL || RSG_CPU_SPU
		Vector3 v = (vecIn - aabbCenter);
		v *= m_Quantize;
		v.Min(v, Vector3(32767.0f,32767.0f,32767.0f));
		v.Max(v, Vector3(-32768.0f,-32768.0f,-32768.0f));
		vecOut[0] = (s16)(round(v.x));
		vecOut[1] = (s16)(round(v.y));
		vecOut[2] = (s16)(round(v.z));
#else
		// Vectorized implementation.  Note that no explicit clamping is needed in this version because that is taken care of implicitly by the
		//   'saturate' features of the instructions as noted below.

		// Subtract, multiply, you know the deal.
		__vector4 temp = __vmulfp( __vsubfp(vecIn, aabbCenter), m_Quantize );

		// Round to nearest integer.
		__vector4 rounded = __vrfin( temp );

		// Float x 4 -> s32 x 4 conversion (with saturation).
		_ivector4 ivector = __vctsxs( rounded, 0 );

		// s32 x 8 -> s16 x 8 conversion (with saturation) and packing.
		// Note: __vspltisw(0) is just a way a getting a zero vector without the potential cache miss of using VEC3_ZERO.
		_hvector4 hvector = __vpkswss(ivector, __vspltisw(0));

		// Blast the first three halfwords out of the vector register and into vecOut.  If we could guarantee that vecOut would be at least 4-byte aligned,
		//   then we could do this with only two store instructions (a word and a halfword) and two splat instructions.  If we could guarantee that vecOut
		//   would be at least 16-byte aligned, then we could further eliminate the splat instructions.
		_hvector4 splattedResult = __vsplth(hvector, 0);
		__stvehx(splattedResult, vecOut, 0);
		splattedResult = __vsplth(hvector, 1);
		__stvehx(splattedResult, vecOut, 2);
		splattedResult = __vsplth(hvector, 2);
		__stvehx(splattedResult, vecOut, 4);
#endif
	}

	// Quantize to the largest quantum unit no greater than the input vector.
	void QuantizeMin(s16 vecOut[3], const Vector3 &vecIn) const
	{
		Vector3 aabbCenter(m_AABBCenter);
#if RSG_CPU_INTEL || RSG_CPU_SPU
		Vector3 v = (vecIn - aabbCenter);
		v *= m_Quantize;
		v.Min(v, Vector3(32767.0f,32767.0f,32767.0f));
		v.Max(v, Vector3(-32768.0f,-32768.0f,-32768.0f));
		vecOut[0] = (s16)(Floorf(v.x));
		vecOut[1] = (s16)(Floorf(v.y));
		vecOut[2] = (s16)(Floorf(v.z));
#else
		// Vectorized implementation.  Note that no explicit clamping is needed in this version because that is taken care of implicitly by the
		//   'saturate' features of the instructions as noted below.

		// Subtract, multiply, you know the deal.
		__vector4 temp = __vmulfp(__vsubfp(vecIn, aabbCenter), m_Quantize);

		// Round to nearest integer in the negative direction.
		__vector4 rounded = __vrfim(temp);

		// Float x 4 -> s32 x 4 conversion (with saturation).
		_ivector4 ivector = __vctsxs(rounded, 0);

		// s32 x 8 -> s16 x 8 conversion (with saturation) and packing.
		// Note: __vspltisw(0) is just a way a getting a zero vector without the potential cache miss of using VEC3_ZERO.
		_hvector4 hvector = __vpkswss(ivector, __vspltisw(0));

		// Blast the first three halfwords out of the vector register and into vecOut.  If we could guarantee that vecOut would be at least 4-byte aligned,
		//   then we could do this with only two store instructions (a word and a halfword) and two splat instructions.  If we could guarantee that vecOut
		//   would be at least 16-byte aligned, then we could further eliminate the splat instructions.
		_hvector4 splattedResult = __vsplth(hvector, 0);
		__stvehx(splattedResult, vecOut, 0);
		splattedResult = __vsplth(hvector, 1);
		__stvehx(splattedResult, vecOut, 2);
		splattedResult = __vsplth(hvector, 2);
		__stvehx(splattedResult, vecOut, 4);
#endif
	}

	// Quantize to the smallest quantum unit no less than the input vector.
	void QuantizeMax(s16 vecOut[3], const Vector3 &vecIn) const
	{
		Vector3 aabbCenter(m_AABBCenter);
#if RSG_CPU_INTEL || RSG_CPU_SPU
		Vector3 v = (vecIn - aabbCenter);
		v *= m_Quantize;
		v.Min(v, Vector3(32767.0f,32767.0f,32767.0f));
		v.Max(v, Vector3(-32768.0f,-32768.0f,-32768.0f));
		vecOut[0] = (s16)(ceilf(v.x));
		vecOut[1] = (s16)(ceilf(v.y));
		vecOut[2] = (s16)(ceilf(v.z));
#else
		// Vectorized implementation.  Note that no explicit clamping is needed in this version because that is taken care of implicitly by the
		//   'saturate' features of the instructions as noted below.

		// Subtract, multiply, you know the deal.
		__vector4 temp = __vmulfp(__vsubfp(vecIn, aabbCenter), m_Quantize);

		// Round to nearest integer in the positive direction.
		__vector4 rounded = __vrfip(temp);

		// Float x 4 -> s32 x 4 conversion (with saturation).
		_ivector4 ivector = __vctsxs(rounded, 0);

		// s32 x 8 -> s16 x 8 conversion (with saturation) and packing.
		// Note: __vspltisw(0) is just a way a getting a zero vector without the potential cache miss of using VEC3_ZERO.
		_hvector4 hvector = __vpkswss(ivector, __vspltisw(0));

		// Blast the first three halfwords out of the vector register and into vecOut.  If we could guarantee that vecOut would be at least 4-byte aligned,
		//   then we could do this with only two store instructions (a word and a halfword) and two splat instructions.  If we could guarantee that vecOut
		//   would be at least 16-byte aligned, then we could further eliminate the splat instructions.
		_hvector4 splattedResult = __vsplth(hvector, 0);
		__stvehx(splattedResult, vecOut, 0);
		splattedResult = __vsplth(hvector, 1);
		__stvehx(splattedResult, vecOut, 2);
		splattedResult = __vsplth(hvector, 2);
		__stvehx(splattedResult, vecOut, 4);
#endif
	}


	void UnQuantize(Vector3 &vecOut, const s16 vecIn[3]) const
	{
		const rage ::Vector3 aabbCenter = m_AABBCenter;
#if RSG_CPU_INTEL || RSG_CPU_SPU
		Vector3 vecOut2;
		vecOut2.x = (float)(vecIn[0]) * m_InvQuantize.x;
		vecOut2.y = (float)(vecIn[1]) * m_InvQuantize.y;
		vecOut2.z = (float)(vecIn[2]) * m_InvQuantize.z;
		vecOut2.Add(aabbCenter);
		vecOut = vecOut2;
#else
		__vector4 result = __vor(__vector4(__lvlx(vecIn, 0)), __vector4(__lvrx(vecIn, 16)));

		_ivector4 ixyz = __vupkhsh( _hvector4(result) );
		vecOut = __vmaddfp(__vcsxwfp(ixyz, 0), m_InvQuantize, aabbCenter);
#endif
#if 0
		// Test code to ensure that requantizing gives us the same value back.
		s16 temp[3];
		QuantizeClosest(temp, vecOut);
		FastAssert(temp[0] == vecIn[0]);
		FastAssert(temp[1] == vecIn[1]);
		FastAssert(temp[2] == vecIn[2]);
#endif
	}


	__forceinline const rage::Vector3 &GetUnQuantizeMul() const
	{
		return m_InvQuantize;
	}


	__forceinline const rage::Vector3 &GetUnQuantizeAdd() const
	{
		return m_AABBCenter;
	}


	__forceinline void UnPackToFloat(Vector3 &vecOut, const s16 vecIn[3]) const
	{
		const rage ::Vector3 aabbCenter = m_AABBCenter;
#if RSG_CPU_INTEL || RSG_CPU_SPU
		Vector3 vecOut2;
		vecOut2.x = (float)(vecIn[0]);
		vecOut2.y = (float)(vecIn[1]);
		vecOut2.z = (float)(vecIn[2]);
		vecOut = vecOut2;
#else
		__vector4 result = __vor(__vector4(__lvlx(vecIn, 0)), __vector4(__lvrx(vecIn, 16)));

		_ivector4 ixyz = __vupkhsh( _hvector4(result) );
		vecOut = __vcsxwfp(ixyz, 0);
#endif
	}


	__forceinline void UnPackToFloatShiftedEight(Vector3 &vecOut, const s16 vecIn[3]) const
	{
		const rage ::Vector3 aabbCenter = m_AABBCenter;
#if RSG_CPU_INTEL || RSG_CPU_SPU
		Vector3 vecOut2;
		// NOTE: Probably could have done this with a signed-right-shift too.
		vecOut2.x = (float)(vecIn[0]) / 256.0f;
		vecOut2.y = (float)(vecIn[1]) / 256.0f;
		vecOut2.z = (float)(vecIn[2]) / 256.0f;
		vecOut = vecOut2;
#else
		__vector4 result = __vor(__vector4(__lvlx(vecIn, 0)), __vector4(__lvrx(vecIn, 16)));

		_ivector4 ixyz = __vupkhsh( _hvector4(result) );
		vecOut = __vcsxwfp(ixyz, 8);
#endif
	}


	__forceinline void UnPackAlignedToFloatShiftedEight(Vector3 &vecOut, const s16 vecIn[3]) const
	{
		const rage ::Vector3 aabbCenter = m_AABBCenter;
#if RSG_CPU_INTEL || RSG_CPU_SPU
		Vector3 vecOut2;
		// NOTE: Probably could have done this with a signed-right-shift too.
		vecOut2.x = (float)(vecIn[0]) / 256.0f;
		vecOut2.y = (float)(vecIn[1]) / 256.0f;
		vecOut2.z = (float)(vecIn[2]) / 256.0f;
		vecOut = vecOut2;
#else
		FastAssert(((int)(&vecIn[0]) & 15) == 0);
		__vector4 result = __vector4( __lvlx (vecIn, 0) );

		_ivector4 ixyz = __vupkhsh( _hvector4(result) );
		vecOut = __vcsxwfp(ixyz, 8);
#endif
	}

	const phOptimizedBvhNode &GetNode(const int nodeIndex) const
	{
		FastAssert(nodeIndex >= 0);
		FastAssert(nodeIndex < m_numNodes);
		return m_contiguousNodes[nodeIndex];
	}

	// usedPrimNodeCount - This is the number of primitives in the array pointed to by primNodes.  The building of the BVH structure will use only the number of
	//   primitives specified here.
	// maxPrimNodeCount - This is the maximum number of primitives that this BVH will ever be built to contain.  This is used for the allocation of nodes.  This
	//   cannot be less than usedPrimNodeCount.
	// remapPrimitives - If true, the BVH tree will be built such that will encounter primitives in ascending order if traversed depth-first and left-to-right.
	//   This is good for cache performance and is also necessary in order for the BVH node to contain multiple (and a variable number of) primitives but requires
	//   clients to re-order their primitives after the BVH is built (or keep the mapping around).  In the case where nodes only need to contain one primitive,
	//   however, there is no technical need to have the primitives in any particular order, and it might be the case that a client doesn't actually have the
	//   ability to re-order the primitives.  This parameter can only be false if targetPrimitivesPerNode is also 1.
	void BuildFromPrimitiveData(BvhPrimitiveData *primNodes, int usedPrimNodeCount, int maxPrimNodeCount, phPolygon::Index * const newToOldPolygonIndexMapping, int targetPrimitivesPerNode, bool remapPrimitives);

	// This function is the same as BuildFromPrimitiveData() except that it won't allocate any memory for the nodes or subtree headers but rather will using the
	//   previously allocated arrays for them.  This function should really only be used when you have good reason to expect that the existing allocations of nodes
	//   and subtree headers will be okay - generally this will be true if the number of primitives is no greater than they were then the allocations happened.
	//	 minPrimNodeCount is only used for the new BVH construction and is ignored in the old BVH construction.
	enum { INVALID_MIN_NODE_COUNT = -1 };	// This is only used when the old BVH is enabled to indicate that minPrimNodeCount is not used.
	void BuildFromPrimitiveDataNoAllocate(BvhPrimitiveData *primNodes, int primNodeCount, int minPrimNodeCount, phPolygon::Index * const newToOldPolygonIndexMapping, int targetPrimitivesPerNode, bool remapPrimitives);

	void AllocateAndCopyFrom(const phOptimizedBvh &bvhStructureToCopy);

	phOptimizedBvhNode*	buildTree	(BvhPrimitiveData *polyNodes, int startIndex, int endIndex, int targetPrimitivesPerNode, u32 remapPrimitivesMask, int lastSortAxis);

	struct BuildTreeInfo
	{
		BvhPrimitiveData * leafNodes;
		BvhPrimitiveData * tempLeafNodes;
		int primCount;
		int maxPrimCount;
		int nodeCount;
		int minNodeCount;
		int targetPrimitivesPerLeaf;
		u32 remapPrimitivesMask;
	};
	// Build a phOptimizedBvh tree from a BvhConstruction tree.
	phOptimizedBvhNode*	buildTreeFromBvhConstruction(const BvhConstructionNode * root, BuildTreeInfo * info);
	phOptimizedBvhNode*	buildTreeFromBvhConstructionRecurse(const BvhConstructionNode * root, BuildTreeInfo * info);

	// This function updates the bounding boxes of all the nodes in the tree to match the updated primitive data provided.
	//   The bvh will remain valid, however if the nodes have moved significantly it will degenerate and future queries will be much slower.
	// PARAMS:
	//   primNodes - pointer to array of primitive data, the items stored by leaf nodes
	void UpdateFromPrimitiveData(BvhPrimitiveData *primNodes);

	// helper function for UpdateFromPrimitiveData
	// PARAMS:
	//   primNodes - pointer to array of primitive data, the items stored by leaf nodes
	//   subtreeRoot - pointer to bvh node at root of subtree to update
	//   subtreeSize - number of nodes in the subtree to update
	void updateTree(BvhPrimitiveData *primNodes, phOptimizedBvhNode* subtreeRoot, int subtreeSize);

	int	calcSplittingAxis(const BvhPrimitiveData *leafNodes, int startIndex, int endIndex, int &middleAxis);
	void SortPrimitivesAlongAxes(BvhPrimitiveData *leafNodes, int startIndex, int endIndex, int splitAxis1, int splitAxis2);
	int	CalcSplittingIndex(int startIndex, int endIndex, int targetPrimitivesPerNode);

	void Copy(const phOptimizedBvh* original);

	// Do a traversal of the tree while culling against an AABB.
	// AABB extents must be in the local space of the phOptimizedBvh.
	template <class _T> void walkStacklessTree(_T* nodeCallback) const;

	phOptimizedBvhNode *GetRootNode() const
	{
		return m_contiguousNodes;
	}

	int GetNumNodes() const
	{
		return m_NumNodesInUse;
	}


#if __PFDRAW
	void Draw(Mat34V_In matrix, int depth, bool drawIndices) const;
#endif
};


inline phOptimizedBvh::phBVHSubtreeInfo::phBVHSubtreeInfo()
{
}

inline phOptimizedBvh::phBVHSubtreeInfo::phBVHSubtreeInfo(datResource &UNUSED_PARAM(rsc))
{
	// No pointers or anything to fix up.
}

inline IMPLEMENT_PLACE(phOptimizedBvh::phBVHSubtreeInfo);

#if __DECLARESTRUCT
inline void phOptimizedBvh::phBVHSubtreeInfo::DeclareStruct(datTypeStruct &s)
{
	// It probably makes no difference but I think this should be STRUCT_BEGIN(phOptimizedBvh::phBVHSubtreeInfo).
	STRUCT_BEGIN(phOptimizedBvh);
	STRUCT_CONTAINED_ARRAY(m_AABBMin);
	STRUCT_CONTAINED_ARRAY(m_AABBMax);
	STRUCT_FIELD(m_RootNodeIndex);
	STRUCT_FIELD(m_LastIndex);
	STRUCT_END();
}
#endif // __DECLARESTRUCT

inline void phOptimizedBvh::phBVHSubtreeInfo::SetAABBFromNode(const phOptimizedBvhNode &node)
{
	m_AABBMin[0] = node.m_AABBMin[0];
	m_AABBMin[1] = node.m_AABBMin[1];
	m_AABBMin[2] = node.m_AABBMin[2];

	m_AABBMax[0] = node.m_AABBMax[0];
	m_AABBMax[1] = node.m_AABBMax[1];
	m_AABBMax[2] = node.m_AABBMax[2];
}

template <class _T>
void phOptimizedBvh::walkStacklessTree(_T* testAndProcessCallback) const
{
#if RSG_CPU_SPU
	// Following what's done in CellMidphaseTaskHandler.cpp we're DMAing in the entire array of subtree headers here.  I'm not convinced that this is a good
	//   thing to do, I think I'd rather double buffer them.  Not only would that use less memory, it might reduce the latency just a tiny bit.
	const int kNumSubtreeHeaders = GetNumUsedSubtreeHeaders();
	phBVHSubtreeInfo *spuSubtreeHeaders = Alloca(phBVHSubtreeInfo, kNumSubtreeHeaders);
	cellDmaLargeGet(spuSubtreeHeaders, (uint64_t)(GetSubtreeHeaders()), sizeof(phBVHSubtreeInfo) * kNumSubtreeHeaders, DMA_TAG(16), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(16));

	// Allocate the subtree node buffer on the stack ONCE, here.
	phOptimizedBvhNode *subtreeNodes = (Alloca(phOptimizedBvhNode, MAX_SUBTREE_SIZE / sizeof(phOptimizedBvhNode)));
#endif
	for(int subtreeHeaderIndex = 0; subtreeHeaderIndex < m_CurSubtreeHeaderIndex; ++subtreeHeaderIndex)
	{
#if !RSG_CPU_SPU
		const phBVHSubtreeInfo &curSubtreeHeader = m_SubtreeHeaders[subtreeHeaderIndex];
#else
		const phBVHSubtreeInfo &curSubtreeHeader = spuSubtreeHeaders[subtreeHeaderIndex];
#endif
		if(testAndProcessCallback->TestAgainstSubtree(curSubtreeHeader.m_AABBMin, curSubtreeHeader.m_AABBMax))
		{
			const int curNodeIndex = curSubtreeHeader.m_RootNodeIndex;
			const int lastNodeIndex = curSubtreeHeader.m_LastIndex;
			FastAssert(curNodeIndex != lastNodeIndex);
#if !RSG_CPU_SPU
			const phOptimizedBvhNode *curNode = &GetNode(curNodeIndex);
			const phOptimizedBvhNode *lastNode = &m_contiguousNodes[lastNodeIndex];	// Can't use GetNode() because we're potentially grabbing an out-of-range node.
#else
			const int kNumNodesInSubtree = lastNodeIndex - curNodeIndex;
			Assert(sizeof(phOptimizedBvhNode) * kNumNodesInSubtree <= MAX_SUBTREE_SIZE);
			cellDmaLargeGet(&subtreeNodes[0], (uint64_t)(&GetNode(curNodeIndex)), sizeof(phOptimizedBvhNode) * kNumNodesInSubtree, DMA_TAG(16), 0, 0);
			cellDmaWaitTagStatusAll(DMA_MASK(16));
			const phOptimizedBvhNode *curNode = &subtreeNodes[0];
			const phOptimizedBvhNode *lastNode = &subtreeNodes[kNumNodesInSubtree];
#endif

			// Go through the nodes, processing each one through ProcessNode() and figuring out where to go next.
			do
			{
				u32 aabbOverlapMask = testAndProcessCallback->ProcessNode(curNode);
				u32 isLeafNodeMask = curNode->IsLeafNodeMask();
				u32 combinedMask = (aabbOverlapMask | isLeafNodeMask);

				// Figure out where to go next.  If we passed the ProcessNode() test or we're a leaf node then we just want to go to the next node.
				//   If not, we want to figure out what node to 'escape' to.  Because we're not branching anywhere here, we need to call GetNodeData()
				//   to get the escape increment to avoid a FastAssert() or extra work.
				u32 nodeIncrementAmount = ISelectI(combinedMask, curNode->GetNodeData(), 1);
				curNode += nodeIncrementAmount;
			}
			while(Likely(curNode < lastNode));
		}
	}
}

} // namespace rage

#endif //OPTIMIZED_BVH_H
