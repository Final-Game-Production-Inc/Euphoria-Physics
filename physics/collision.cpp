// 
// physics/collision.cpp 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

#include "collision.h"

#include "compositepointers.h"
#include "manifoldresult.h"

#include "phbound/boundbvh.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundgrid.h"
#include "phbound/boundbox.h"
#include "phbound/boundcapsule.h"
#include "phbound/boundcylinder.h"
#include "phbound/boundsphere.h"
#include "phbound/OptimizedBvh.h"
#include "phbullet/TriangleShape.h"
#include "phbound/support.h"
#include "phbullet/ContinuousConvexCollision.h"
#include "phbullet/ConvexIntersector.h"
#include "phbullet/GjkPairDetector.h"		// Only needed because it contains the definition of ClosestPointInput.
#include "phcore/constants.h"
#include "phcore/frameallocator.h"
#include "phcore/pool.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"
#include "physics/physicsprofilecapture.h"

#include "collisionoverlaptest.h"
#include "manifold.h"
#include "overlappingpairarray.h"
#include "simulator.h"	// Only needed for PHMANIFOLD #define.

#include "PrimitiveCache.h"
#include "CollisionMemory.h"
#include "phcore/avl_tree.h"

#define PREFETCH_VERTICES	(COMPRESSED_VERTEX_METHOD == 2)

GJK_COLLISION_OPTIMIZE_OFF()

namespace rage {

PARAM(disableFix7459244, "disable nullptr checks for B*7459244");

extern bool g_UseNormalClamping;

// Controls the maximum depth of collision work stack.
#define COLLISION_WORK_STACK_DEPTH	44

#if !__SPU
#define NON_SPU_ONLY(x)	x
#else
#define NON_SPU_ONLY(x)
#endif

// ObjectState operator= shows up pretty high in profiles but switching to (what should be) faster implementations doesn't seem to help total collision
//   time at all, so these are disabled.
// 0 means use the default, 1 means use a sysMemCpy, 2 means use vector operations.
#define OBJECTSTATE_ASSIGNMENT_OPERATOR_TYPE	(__64BIT? 1 : 2)
// 0 means use a SwapEm, 1 means swap using vector instructions
#define OBJECTSTATE_USE_NEW_SWAP	0

#define DISABLE_EXTRA_TIMERS	1
#if !DISABLE_EXTRA_TIMERS
#define PF_FUNC_2(x)	PF_FUNC(x)
#define PF_START_2(x)	PF_START(x)
#define PF_STOP_2(x)	PF_STOP(x)
#define PF_INCREMENT_2(x)	PF_INCREMENT(x)
#define PF_INCREMENTBY_2(x,c) PF_INCREMENTBY(x,c)
#else
#define PF_FUNC_2(x)
#define PF_START_2(x)
#define PF_STOP_2(x)
#define PF_INCREMENT_2(x)
#define PF_INCREMENTBY_2(x,c)
#endif

#define ENABLE_EXTRA_MANIFOLD_MANAGEMENT_TIMERS	0
#if ENABLE_EXTRA_MANIFOLD_MANAGEMENT_TIMERS
#define PF_FUNC_MM(x)	PF_FUNC(x)
#define PF_START_MM(x)	PF_START(x)
#define PF_STOP_MM(x)	PF_STOP(x)
#else
#define PF_FUNC_MM(x)
#define PF_START_MM(x)
#define PF_STOP_MM(x)
#endif

namespace phCollisionStats
{
 	EXT_PF_TIMER(BVHMidphase);
 	EXT_PF_TIMER(GridMidphase);
	EXT_PF_TIMER(PairwiseBoxTests);
    EXT_PF_TIMER(PairwiseCollisions);

//	EXT_PF_COUNTER(ManifoldsAllocd);
//	EXT_PF_COUNTER(ManifoldsReleased);

	PF_PAGE(phCollisionStats_ComputeCollision,"ph CollisionStats_ComputeCollision");
	PF_GROUP(ComputeCollisionGroup);
	PF_LINK(phCollisionStats_ComputeCollision,ComputeCollisionGroup);
	PF_TIMER(ComputeCollision,ComputeCollisionGroup);
	PF_COUNTER(ComputeCollisionCount,ComputeCollisionGroup);
#if !DISABLE_EXTRA_TIMERS
	PF_COUNTER(PrimCacheHighWater,ComputeCollisionGroup);
	PF_COUNTER(TriCount0,ComputeCollisionGroup);
	PF_COUNTER(TriCount1,ComputeCollisionGroup);
	PF_COUNTER(UniqueTriCount0,ComputeCollisionGroup);
	PF_COUNTER(UniqueTriCount1,ComputeCollisionGroup);
	PF_COUNTER(BvhLeafCount,ComputeCollisionGroup);
	PF_COUNTER(UniqueBvhLeafCount,ComputeCollisionGroup);
	PF_COUNTER(ProcessLeafCollisionCount,ComputeCollisionGroup);
	PF_TIMER(TestOverlap,ComputeCollisionGroup);
	PF_COUNTER(TestOverlapCount,ComputeCollisionGroup);
	PF_TIMER(MidphaseLoop,ComputeCollisionGroup);
	PF_TIMER(IterateCulling,ComputeCollisionGroup);
	PF_TIMER(ProcessLeafCollision,ComputeCollisionGroup);
	PF_TIMER(GetCompressedVertex,ComputeCollisionGroup);
	PF_TIMER(BoundVsBound,ComputeCollisionGroup);
	PF_COUNTER(BoundVsBoundCount,ComputeCollisionGroup);
	PF_TIMER(BoundVsTriangle,ComputeCollisionGroup);
	PF_TIMER(BoundVsSphere,ComputeCollisionGroup);
	PF_TIMER(BoundVsCapsule,ComputeCollisionGroup);
	PF_TIMER(BoundVsBox,ComputeCollisionGroup);
	PF_TIMER(BoundVsCylinder,ComputeCollisionGroup);
	PF_TIMER(ProcessPairwiseCollision,ComputeCollisionGroup);
	PF_TIMER(ManifoldManagement0,ComputeCollisionGroup);
	PF_TIMER(ManifoldManagement1,ComputeCollisionGroup);
	PF_TIMER(ManifoldManagement2,ComputeCollisionGroup);
	PF_TIMER(VsTriangle0,ComputeCollisionGroup);
	PF_TIMER(VsTriangle1,ComputeCollisionGroup);
#endif
#if ENABLE_EXTRA_MANIFOLD_MANAGEMENT_TIMERS
	PF_TIMER(ManifoldManagement0a,ComputeCollisionGroup);
	PF_TIMER(ManifoldManagement0b,ComputeCollisionGroup);
	PF_TIMER(ManifoldManagement0c,ComputeCollisionGroup);
#endif
};

EXT_PFD_DECLARE_ITEM(AutoCCD);
EXT_PFD_DECLARE_ITEM(SweptCullBoxes);

using namespace phCollisionStats;


#if __SPU
// Class for managing the buffers used when DMAing collision-related objects onto the SPU.
typedef FrameAllocator<COLLISION_WORK_STACK_DEPTH> phSpuCollisionBuffers;
#endif	// __SPU




// Forward-declare this here so that we can pass it as a parameter to ObjectState::Iterate().
template <int> class phCollisionWorkStack;


#if __SPU
#define SPU_PARAM(X) , X
#else	// __SPU
#define SPU_PARAM(X)
#endif	// __SPU

#if __ASSERT
#define ASSERT_PARAM(X) , X
#else // __ASSERT
#define ASSERT_PARAM(X)
#endif // __ASSERT

#if __SPU
#define DMA_BOUND_GEOMETRY_CHECK(p) if (Unlikely(p == NULL)) return false;
bool DmaBoundGeometry(phBoundGeometry * geomBound, phSpuCollisionBuffers * collisionBuffers)
{
#if OCTANT_MAP_SUPPORT_ACCEL
	const int DMATAG_OCTANTMAP = 1;
	const int hasOctantMap = geomBound->HasOctantMap();
	if(hasOctantMap)
	{
		FastAssert(geomBound->m_OctantVerts != NULL && geomBound->m_OctantVertCounts != NULL);
		phBoundGeometry::OctantMapIndexType *spuOctantMapVertCounts = reinterpret_cast<phBoundGeometry::OctantMapIndexType *>(collisionBuffers->GetBlockSafe(sizeof(phBoundGeometry::OctantMapIndexType) * 8));
		DMA_BOUND_GEOMETRY_CHECK(spuOctantMapVertCounts);
		sysDmaLargeGet(spuOctantMapVertCounts, (uint64_t)geomBound->m_OctantVertCounts, sizeof(phBoundGeometry::OctantMapIndexType) * 8, DMA_TAG(DMATAG_OCTANTMAP));

		phBoundGeometry::OctantMapIndexType **octantVertPtrBuffer = reinterpret_cast<phBoundGeometry::OctantMapIndexType **>(collisionBuffers->GetBlockSafe(sizeof(phBoundGeometry::OctantMapIndexType *) * 8));
		DMA_BOUND_GEOMETRY_CHECK(octantVertPtrBuffer);
		sysDmaLargeGet(octantVertPtrBuffer, (uint64_t)geomBound->m_OctantVerts, sizeof(phBoundGeometry::OctantMapIndexType *) * 8, DMA_TAG(DMATAG_OCTANTMAP));

		geomBound->m_OctantVertCounts = spuOctantMapVertCounts;
		geomBound->m_OctantVerts = octantVertPtrBuffer;
	}
#endif	// OCTANT_MAP_SUPPORT_ACCEL

	const int numVertices = geomBound->GetNumVertices();
#if COMPRESSED_VERTEX_METHOD == 0
	Vec3V *spuVertexBuffer = reinterpret_cast<Vec3V *>(collisionBuffers->GetBlockSafe(sizeof(Vec3V) * numVertices));
	DMA_BOUND_GEOMETRY_CHECK(spuVertexBuffer);
	sysDmaLargeGet(spuVertexBuffer, (uint64_t)geomBound->GetVertexPointer(), sizeof(Vec3V) * numVertices, DMA_TAG(1));
	geomBound->SetVertexPointer(spuVertexBuffer);
#else	// COMPRESSED_VERTEX_METHOD == 0
	CompressedVertexType *spuVertexBuffer = reinterpret_cast<CompressedVertexType *>(collisionBuffers->GetBlockSafe(sizeof(CompressedVertexType) * 3 * numVertices));
	DMA_BOUND_GEOMETRY_CHECK(spuVertexBuffer);
	const CompressedVertexType *ppuCompressedVertices = geomBound->GetShrunkVertexPointer() != NULL ? geomBound->GetShrunkVertexPointer() : geomBound->GetCompressedVertexPointer();
	sysDmaLargeGet(spuVertexBuffer, (uint64_t)ppuCompressedVertices, sizeof(CompressedVertexType) * 3 * numVertices, DMA_TAG(1));
	geomBound->SetCompressedVertexPointer(spuVertexBuffer);
	geomBound->m_CompressedVertices = spuVertexBuffer;
	geomBound->m_CompressedShrunkVertices = spuVertexBuffer;
#endif	// COMPRESSED_VERTEX_METHOD == 0

	// We could probably do these on a separate DMA tag since we don't need them until later 
	const int numPolygons = geomBound->GetNumPolygons();
	phPolygon *spuPolygonBuffer = reinterpret_cast<phPolygon *>(collisionBuffers->GetBlockSafe(sizeof(phPolygon) * numPolygons));
	DMA_BOUND_GEOMETRY_CHECK(spuPolygonBuffer);
	sysDmaLargeGet(spuPolygonBuffer, (uint64_t)geomBound->GetPolygonPointer(), sizeof(phPolygon) * numPolygons, DMA_TAG(1));
	geomBound->m_Polygons = spuPolygonBuffer;

	const int numMaterialIds = geomBound->GetNumMaterials();
	phMaterialMgr::Id *spuMaterialIdBuffer = reinterpret_cast<phMaterialMgr::Id *>(collisionBuffers->GetBlockSafe(sizeof(phMaterial) * numMaterialIds));
	DMA_BOUND_GEOMETRY_CHECK(spuMaterialIdBuffer);
	sysDmaLargeGet(spuMaterialIdBuffer, (uint64_t)geomBound->m_MaterialIds, numMaterialIds * sizeof(phMaterialMgr::Id), DMA_TAG(1));
	geomBound->m_MaterialIds = spuMaterialIdBuffer;

#if OCTANT_MAP_SUPPORT_ACCEL
	sysDmaWaitTagStatusAll(DMA_MASK(DMATAG_OCTANTMAP));
	if(hasOctantMap)
	{
		for(int octantIndex = 0; octantIndex < 8; ++octantIndex)
		{
			phBoundGeometry::OctantMapIndexType *spuOctantMapVerts = reinterpret_cast<phBoundGeometry::OctantMapIndexType *>(collisionBuffers->GetBlockSafe(sizeof(phBoundGeometry::OctantMapIndexType) * geomBound->m_OctantVertCounts[octantIndex]));
			DMA_BOUND_GEOMETRY_CHECK(spuOctantMapVerts);
			sysDmaLargeGet(spuOctantMapVerts, (uint64_t)geomBound->m_OctantVerts[octantIndex], sizeof(phBoundGeometry::OctantMapIndexType) * geomBound->m_OctantVertCounts[octantIndex], DMA_TAG(1));
			geomBound->m_OctantVerts[octantIndex] = spuOctantMapVerts;
		}
	}
#endif	// OCTANT_MAP_SUPPORT_ACCEL

	//sysDmaWaitTagStatusAll(DMA_MASK(1));
	return true;
}

void DmaBoundGeometryWait()
{
	sysDmaWaitTagStatusAll(DMA_MASK(1));
}
#endif	// __SPU

#if !USE_NEW_MID_PHASE || !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP
// The ObjectState class is used to represent a (possibly improper) subset of a bound.
// This is used by the (new) midphase code to keep track of what portion of the bound we're currently working on and to save off information regarding what portion
//   of the bound we need to come back to later.
class ObjectState
{
public:
	// Since waits are often separated from the gets and puts for which they are waiting these tags are used to make these connections clear.
	enum eDmaTags
	{
		DMATAG_COMPOSITECOMPONENT	=	1,
		DMATAG_OCTANTMAP			=	1,

		// The following four tags might be used in parallel and thus should be distinct for optimal performance.
		DMATAG_COMPONENTPOINTERS	=	1,
		DMATAG_PERCOMPONENTDATA		=	1,
		DMATAG_BVHSTRUCTURE			=	2,
		DMATAG_SUBTREEHEADERS		=	1,

		DMATAG_BVHNODES				=	1,
	};

	// Init() returns false if it finds that the bound has nothing to collide with.  Right now this is only checked for 'flat' composites (since we'd end
	//   iterating over them anyway) but, in the future, should probably also be done for subtree headers.
	bool Init(const phBound *curBound, Mat34V_In curMatrix, Mat34V_In lastMatrix, int collisionBoundIndex)
	{
		const Vec3V vBoundingBoxMin = curBound->GetBoundingBoxMin();
		const Vec3V vBoundingBoxMax = curBound->GetBoundingBoxMax();
#if !MIDPHASE_USES_QUATS
		InitBoundingBoxInfo(vBoundingBoxMin, vBoundingBoxMax, curMatrix, lastMatrix);
		SetBoundToWorldMatrices(curMatrix, lastMatrix);
#else	// !MIDPHASE_USES_QUATS
		const QuatV curBoundToWorldOrientation = QuatVFromMat34V(curMatrix);
		const Vec3V curBoundToWorldOffset = curMatrix.GetCol3();
		const QuatV lastBoundToWorldOrientation = QuatVFromMat34V(lastMatrix);
		const Vec3V lastBoundToWorldOffset = lastMatrix.GetCol3();
		InitBoundingBoxInfo(vBoundingBoxMin, vBoundingBoxMax, curBoundToWorldOrientation, curBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset);
		SetBoundToWorldMatrices(curBoundToWorldOrientation, curBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset);
#endif	// !MIDPHASE_USES_QUATS
		SetCollisionBoundIndex(collisionBoundIndex);
		m_ComponentIndex = 0;
		const bool retVal = InitState(curBound);
		Assert(m_ComponentIndex == 0);
		return retVal;
	}

	void SetTypeAndIncludeFlags(u32 typeFlages, u32 includeFlags)
	{
		m_TypeFlags = typeFlages;
		m_IncludeFlags = includeFlags;
	}

#if __SPU
	static void SetSpuCollisionBuffers(phSpuCollisionBuffers *collisionBuffers)
	{
		FastAssert(collisionBuffers != NULL);
		s_SpuCollisionBuffers = collisionBuffers;
	}

	static phSpuCollisionBuffers *GetSpuCollisionBuffers()
	{
		return s_SpuCollisionBuffers;
	}
#endif	// __SPU

	__forceinline NON_SPU_ONLY(const) phOptimizedBvh *GetBvhStructure() const
	{
		return reinterpret_cast<NON_SPU_ONLY(const) phOptimizedBvh *>(m_PackedBVHStructureAndStateType & ~15);
	}

	__forceinline const phOptimizedBvhNode *GetBvhNodes() const
	{
#if __64BIT
		return m_BvhNodes;
#else
#if MIDPHASE_USES_QUATS
		return reinterpret_cast<const phOptimizedBvhNode *>(m_CurBoundToWorldTransform.GetPositionConstRef().GetWi());
#else	// MIDPHASE_USES_QUATS
		return reinterpret_cast<const phOptimizedBvhNode *>(m_CurBoundToWorldMatrix.GetCol0ConstRef().GetWi());
#endif	// MIDPHASE_USES_QUATS
#endif
	}

	__forceinline const phOptimizedBvh::phBVHSubtreeInfo &GetSubtreeHeader(int subtreeIndex) const
	{
		FastAssert(subtreeIndex >= 0 && subtreeIndex < GetBvhStructure()->GetNumUsedSubtreeHeaders());
		return GetBvhStructure()->GetSubtreeHeaders()[subtreeIndex];
	}

	__forceinline const phOptimizedBvhNode &GetBvhNode(int nodeIndex) const
	{
		// We should never be asking for a node that's not even part of this BVH.
		FastAssert(nodeIndex >= 0 && nodeIndex < GetBvhStructure()->GetNumNodes());
		// We should never be asking for a node that's not part of our current subtree.  One exception (for now) is if we're a subtree header that's in the
		//   process of being converted in a subtree because we won't have set the start and end indices yet.
		FastAssert(GetStateType() == OBJECTSTATETYPE_SUBTREEHEADER || nodeIndex >= GetStartIndex() && nodeIndex < GetEndIndex());
		return GetBvhNodes()[nodeIndex];
	}

	__forceinline u32 GetTypeFlags() const
	{
		return m_TypeFlags;
	}

	__forceinline u32 GetIncludeFlags() const
	{
		return m_IncludeFlags;
	}

#if !MIDPHASE_USES_QUATS
	void InitBoundingBoxInfo(Vec3V_In vBoundingBoxMin, Vec3V_In vBoundingBoxMax, Mat34V_In curBoundToWorldMatrix, Mat34V_In lastBoundToWorldMatrix)
#else	// !MIDPHASE_USES_QUATS
	void InitBoundingBoxInfo(Vec3V_In vBoundingBoxMin, Vec3V_In vBoundingBoxMax, QuatV_In curBoundToWorldOrientation, Vec3V_In curBoundToWorldOffset, QuatV_In lastBoundToWorldOrientation, Vec3V_In lastBoundToWorldOffset)
#endif	// !MIDPHASE_USES_QUATS
	{
		const ScalarV vsHalf(V_HALF);
		Vec3V vBoundingBoxDimensions = Subtract(vBoundingBoxMax, vBoundingBoxMin);
		Vec3V vBoundingBoxCenter_IS = Scale(vsHalf, Add(vBoundingBoxMin, vBoundingBoxMax));

		// Sweep the bounding box according to the motion.
		vBoundingBoxDimensions = Scale(vsHalf, vBoundingBoxDimensions);
#if !MIDPHASE_USES_QUATS
		COT_ExpandOBBFromMotion(curBoundToWorldMatrix, lastBoundToWorldMatrix, vBoundingBoxDimensions, vBoundingBoxCenter_IS);
		const Vec3V vBoundingBoxCenter_WS = Transform(curBoundToWorldMatrix, vBoundingBoxCenter_IS);
#else	// !MIDPHASE_USES_QUATS
		COT_ExpandOBBFromMotion(curBoundToWorldOrientation, curBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset, vBoundingBoxDimensions, vBoundingBoxCenter_IS);
		const Vec3V vBoundingBoxCenter_WS = Transform(curBoundToWorldOrientation, curBoundToWorldOffset, vBoundingBoxCenter_IS);
#endif	// !MIDPHASE_USES_QUATS

		const ScalarV vsBoundingBoxVolume = vBoundingBoxDimensions.GetX() * vBoundingBoxDimensions.GetY() * vBoundingBoxDimensions.GetZ();

		m_CurBoundingBoxHalfSizeXYZVolumeW = Vec4V(vBoundingBoxDimensions, vsBoundingBoxVolume);
		m_CurBoundingBoxCenter_WSXYZCurBoundW.SetXYZ(vBoundingBoxCenter_WS);

		ASSERT_ONLY(const u32 float32NonRealExponent = 0x7f800000;)
		Assertf((GetCurBoundingBoxVolume().Getf() > 0.0f && (GetCurBoundingBoxVolumeAsInt() & float32NonRealExponent) != float32NonRealExponent),
				"ObjectState calculated bad volume: %.5f, 0x%x, %d", GetCurBoundingBoxVolume().Getf(), GetCurBoundingBoxVolumeAsInt(), GetCurBoundingBoxVolumeAsInt()
				/*objectStates[0].*/);
	}


#if !MIDPHASE_USES_QUATS
	__forceinline void InitBoundingBoxInfoFromBVHNode(const phOptimizedBvh *bvhStructure, const phOptimizedBvhNode &node, Mat34V_In curBoundToWorldMatrix, Mat34V_In lastBoundToWorldMatrix)
#else	// !MIDPHASE_USES_QUATS
	__forceinline void InitBoundingBoxInfoFromBVHNode(const phOptimizedBvh *bvhStructure, const phOptimizedBvhNode &node, QuatV_In curBoundToWorldOrientation, Vec3V_In curBoundToWorldOffset, QuatV_In lastBoundToWorldOrientation, Vec3V_In lastBoundToWorldOffset)
#endif	// !MIDPHASE_USES_QUATS
	{
		Vec3V vBoundingBoxMin, vBoundingBoxMax;
		bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMin), node.m_AABBMin);
		bvhStructure->UnQuantize(RC_VECTOR3(vBoundingBoxMax), node.m_AABBMax);
#if !MIDPHASE_USES_QUATS
		InitBoundingBoxInfo(vBoundingBoxMin, vBoundingBoxMax, curBoundToWorldMatrix, lastBoundToWorldMatrix);
#else	// !MIDPHASE_USES_QUATS
		InitBoundingBoxInfo(vBoundingBoxMin, vBoundingBoxMax, curBoundToWorldOrientation, curBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset);
#endif	// !MIDPHASE_USES_QUATS
	}


	// Use this function to tell "us" about the other object being culled against.  Basically this means that we're going to gather whatever information that
	//   we need from the other object in order to be able to efficiently cull ourself against them.
	void InitCullingBoxInfo(const ObjectState &otherObjectState)
	{
		// If we're currently subdividing a BVH let's compute a quantized AABB.  Note that we can't just check if GetCurCullingBound()->GetType() == phBound::BVH 
		//   because that won't work when BVH structures are used in composite bounds.
		if(GetStateType() == OBJECTSTATETYPE_SUBTREE || GetStateType() == OBJECTSTATETYPE_SUBTREEHEADERS)
		{
			// Calculate culling box info relative to our object's bound-to-world matrix.
#if !MIDPHASE_USES_QUATS
			Mat34V localCullingBoxMatrix;
			UnTransformOrtho(localCullingBoxMatrix, m_CurBoundToWorldMatrix, otherObjectState.GetCurBoundingBoxMatrix());

			// Turn the OBB for the other object into a quantized AABB.  This will allow us to quickly accept/reject BVH nodes.
			const Vec3V aabbHalfWidth = geomBoxes::ComputeAABBExtentsFromOBB(localCullingBoxMatrix.GetMat33ConstRef(), otherObjectState.GetCurBoundingBoxHalfSize());
			const Vec3V localCullBoxCenter = localCullingBoxMatrix.GetCol3();
#else	// MIDPHASE_USES_QUATS
			QuatV localCullBoxQuat;
			Vec3V localCullBoxCenter;
			UnTransform(localCullBoxQuat, localCullBoxCenter, GetCurBoundToWorldOrientation(), GetCurBoundToWorldOffset(), otherObjectState.GetCurBoundToWorldOrientation(), otherObjectState.GetCurBoundingBoxCenter());

			// Turn the OBB for the other object into a quantized AABB.  This will allow us to quickly accept/reject BVH nodes.
			const Vec3V aabbHalfWidth = geomBoxes::ComputeAABBExtentsFromOBB(localCullBoxQuat, otherObjectState.GetCurBoundingBoxHalfSize());
#endif	// MIDPHASE_USES_QUATS
			const Vec3V aabbMin = localCullBoxCenter - aabbHalfWidth;
			const Vec3V aabbMax = localCullBoxCenter + aabbHalfWidth;

			const phOptimizedBvh *bvhStructure = GetBvhStructure();
			bvhStructure->QuantizeMin(m_AABBMinQuant, RCC_VECTOR3(aabbMin));
			bvhStructure->QuantizeMax(m_AABBMaxQuant, RCC_VECTOR3(aabbMax));
			Assert(m_AABBMinQuant[0] <= m_AABBMaxQuant[0]);
			Assert(m_AABBMinQuant[1] <= m_AABBMaxQuant[1]);
			Assert(m_AABBMinQuant[2] <= m_AABBMaxQuant[2]);
		}
	}

	// For aggregate bounds (like a grid or a composite) this may point to one of the sub-bounds.
	const phBound *GetCurCullingBound() const
	{
#if __64BIT
		return m_CurCullingBound;
#else
		return (const phBound *)(m_CurBoundingBoxCenter_WSXYZCurBoundW.GetWi());
#endif
	}

	// Not generally to be used, but ProcessLeafCollision() uses object states as its input so we need to be able to swap in a triangle shape.
	void SetCurCullingBound(const phBound *cullingBound)
	{
#if __64BIT
		m_CurCullingBound = cullingBound;
#else
		m_CurBoundingBoxCenter_WSXYZCurBoundW.SetWi((u32)(cullingBound));
#endif
	}

#if MIDPHASE_USES_QUATS
	// Get the current bound to world orientation.  For many bounds this will just be the orientation of the instance.  For bounds that are part of a
	//   composite this will be the component orientation concatenated with the instance orientation.
	QuatV_Out GetCurBoundToWorldOrientation() const
	{
		return m_CurBoundToWorldTransform.GetRotation();
	}

	// Get the current bound to world offset.  For many bounds this will just be the offset of the instance.  For bounds that are part of a composite this
	//   will be the component offset concatenated with the instance orientation and offset.
	Vec3V_Out GetCurBoundToWorldOffset() const
	{
		return m_CurBoundToWorldTransform.GetPosition();
	}

	// Get the last bound to world orientation.  For many bounds this will just be the last orientation of the instance.  For bounds that are part of a
	//   composite this will be the component last orientation concatenated with the instance last orientation.
	QuatV_Out GetLastBoundToWorldOrientation() const
	{
		return m_LastBoundToWorldTransform.GetRotation();
	}

	// Get the last bound to world offset.  For many bounds this will just be the last offset of the instance.  For bounds that are part of a composite this
	//   will be the component last offset concatenated with the instance last orientation and last offset.
	Vec3V_Out GetLastBoundToWorldOffset() const
	{
		return m_LastBoundToWorldTransform.GetPosition();
	}
#else	// MIDPHASE_USES_QUATS
	// Get the current bound to world matrix.  For many bounds this will just be the instance matrix.  For bounds that are part of a composite this will be
	//   the component matrix concatenated with the instance matrix.
	Mat34V_ConstRef GetCurBoundToWorldMatrix() const
	{
		return m_CurBoundToWorldMatrix;
	}

	// Get the last bound to world matrix.  For many bounds this will just be the last instance matrix.  For bounds that are part of a composite this will be
	//   the last component matrix concatenated with the last instance matrix.
	Mat34V_ConstRef GetLastBoundToWorldMatrix() const
	{
		return m_LastBoundToWorldMatrix;
	}
#endif	// MIDPHASE_USES_QUATS

	Vec3V_Out GetCurBoundingBoxHalfSize() const
	{
		return m_CurBoundingBoxHalfSizeXYZVolumeW.GetXYZ();
	}

	ScalarV_Out GetCurBoundingBoxVolume() const
	{
		return m_CurBoundingBoxHalfSizeXYZVolumeW.GetW();
	}

	int GetCurBoundingBoxVolumeAsInt() const
	{
		return m_CurBoundingBoxHalfSizeXYZVolumeW.GetWi();
	}

	Vec3V_Out GetCurBoundingBoxCenter() const
	{
		return m_CurBoundingBoxCenter_WSXYZCurBoundW.GetXYZ();
	}

#if !MIDPHASE_USES_QUATS
	Mat34V_Out GetCurBoundingBoxMatrix() const
	{
		Mat34V mtxOut = GetCurBoundToWorldMatrix();
		mtxOut.SetCol3(GetCurBoundingBoxCenter());
		return mtxOut;
	}
#endif	// MIDPHASE_USES_QUATS

	int GetStartIndex() const
	{
		return m_StartIndex;
	}

	int GetEndIndex() const
	{
		return m_EndIndex;
	}

	bool IsLeaf() const
	{
		return(GetStateType() == OBJECTSTATETYPE_LEAF);
	}

	// Find out whether the object to which this ObjectState corresponds was object 0 or object 1 in the root manifold.
	int GetCollisionBoundIndex() const
	{
		return (m_Flags & OBJECTSTATEFLAG_COLLISIONBOUNDINDEX);
	}

	// If we have descended *through* a composite bound this will tell us through what component we descended.  It will be zero if we have never descended
	//   *through* a composite.
	u16 GetComponentIndex() const
	{
		return m_ComponentIndex;
	}

#if OBJECTSTATE_ASSIGNMENT_OPERATOR_TYPE != 0
	__forceinline ObjectState &operator =(const ObjectState &otherObjectState)	// Should this be __forceinline'd or not?  I'm not sure.
	{
#if OBJECTSTATE_ASSIGNMENT_OPERATOR_TYPE == 1
		sysMemCpy(this, &otherObjectState, sizeof(ObjectState));
#else
#if !MIDPHASE_USES_QUATS
		CompileTimeAssert(sizeof(ObjectState) == 192);
		Vec3V * RESTRICT thisAsVec3V = reinterpret_cast<Vec3V *>(this);
		const Vec3V * RESTRICT otherAsVec3V = reinterpret_cast<const Vec3V *>(&otherObjectState);
		thisAsVec3V[0] = otherAsVec3V[0];
		thisAsVec3V[1] = otherAsVec3V[1];
		thisAsVec3V[2] = otherAsVec3V[2];
		thisAsVec3V[3] = otherAsVec3V[3];
		thisAsVec3V[4] = otherAsVec3V[4];
		thisAsVec3V[5] = otherAsVec3V[5];
		thisAsVec3V[6] = otherAsVec3V[6];
		thisAsVec3V[7] = otherAsVec3V[7];
		thisAsVec3V[8] = otherAsVec3V[8];
		thisAsVec3V[9] = otherAsVec3V[9];
		thisAsVec3V[10] = otherAsVec3V[10];
		thisAsVec3V[11] = otherAsVec3V[11];
#else	// MIDPHASE_USES_QUATS
		CompileTimeAssert(sizeof(ObjectState) == 128);
		Vec3V * RESTRICT thisAsVec3V = reinterpret_cast<Vec3V *>(this);
		const Vec3V * RESTRICT otherAsVec3V = reinterpret_cast<const Vec3V *>(&otherObjectState);
		thisAsVec3V[0] = otherAsVec3V[0];
		thisAsVec3V[1] = otherAsVec3V[1];
		thisAsVec3V[2] = otherAsVec3V[2];
		thisAsVec3V[3] = otherAsVec3V[3];
		thisAsVec3V[4] = otherAsVec3V[4];
		thisAsVec3V[5] = otherAsVec3V[5];
		thisAsVec3V[6] = otherAsVec3V[6];
		thisAsVec3V[7] = otherAsVec3V[7];
#endif	// MIDPHASE_USES_QUATS
#endif
		return *this;
	}
#endif

	__forceinline void SwapWith(ObjectState &otherObjectState)
	{
#if OBJECTSTATE_USE_NEW_SWAP
		Vec3V * RESTRICT thisAsVec3V = reinterpret_cast<Vec3V *>(this);
		Vec3V * RESTRICT otherAsVec3V = reinterpret_cast<Vec3V *>(&otherObjectState);
		SwapEm(thisAsVec3V[0], otherAsVec3V[0]);
		SwapEm(thisAsVec3V[1], otherAsVec3V[1]);
		SwapEm(thisAsVec3V[2], otherAsVec3V[2]);
		SwapEm(thisAsVec3V[3], otherAsVec3V[3]);
		SwapEm(thisAsVec3V[4], otherAsVec3V[4]);
		SwapEm(thisAsVec3V[5], otherAsVec3V[5]);
		SwapEm(thisAsVec3V[6], otherAsVec3V[6]);
		SwapEm(thisAsVec3V[7], otherAsVec3V[7]);
		SwapEm(thisAsVec3V[8], otherAsVec3V[8]);
		SwapEm(thisAsVec3V[9], otherAsVec3V[9]);
		SwapEm(thisAsVec3V[10], otherAsVec3V[10]);
		SwapEm(thisAsVec3V[11], otherAsVec3V[11]);
		SwapEm(thisAsVec3V[12], otherAsVec3V[12]);
		SwapEm(thisAsVec3V[13], otherAsVec3V[13]);
		SwapEm(thisAsVec3V[14], otherAsVec3V[14]);
		SwapEm(thisAsVec3V[15], otherAsVec3V[15]);
#else
		SwapEm(*this, otherObjectState);
#endif
	}


	// Not a const function any more because TestOverlap() can result in a state change in the ObjectState (for example changing from a 'component' to a 'leaf') once
	//   we've passed the requisite tests.  Also this function should probably get renamed now as its current name implies that it's just a spatial test.
	bool TestOverlap(const ObjectState &otherObjectState)
	{
		PF_FUNC_2(TestOverlap);
		PF_INCREMENT_2(TestOverlapCount);
		switch(GetStateType())
		{
			case OBJECTSTATETYPE_SUBTREEHEADERS:
			case OBJECTSTATETYPE_LEAF:
			{
				FastAssert(GetStateType() != OBJECTSTATETYPE_SUBTREEHEADERS || GetStartIndex() == 0);
				FastAssert(GetStateType() != OBJECTSTATETYPE_SUBTREEHEADERS || GetEndIndex() == GetBvhStructure()->GetNumUsedSubtreeHeaders());

				// NOTE: The geomBoxes::TestBoxXXX() functions seem to want 0 in the space of 1, which contradicts how I interpret the documentation.
#if !MIDPHASE_USES_QUATS
				const ScalarV vsZero(V_ZERO);
				const Vec4V boxHalfExtents0 = Vec4V(GetCurBoundingBoxHalfSize(), vsZero);
				const Vec4V boxHalfExtents1 = Vec4V(otherObjectState.GetCurBoundingBoxHalfSize(), vsZero);

				Mat34V relativeMatrix;
				UnTransformOrtho(relativeMatrix, otherObjectState.GetCurBoundingBoxMatrix(), GetCurBoundingBoxMatrix());

				const bool retVal = COT_TestBoxToBoxOBBFaces(boxHalfExtents0.GetXYZ(), boxHalfExtents1.GetXYZ(), relativeMatrix);
#else	// !MIDPHASE_USES_QUATS
#if 1
				QuatV relativeOrientation;
				Vec3V relativePosition;
				UnTransform(relativeOrientation, relativePosition, GetCurBoundToWorldOrientation(), GetCurBoundingBoxCenter(), otherObjectState.GetCurBoundToWorldOrientation(), otherObjectState.GetCurBoundingBoxCenter());

				//const bool retVal = geomBoxes::TestBoxToBoxOBBFaces(GetCurBoundingBoxHalfSize(), otherObjectState.GetCurBoundingBoxHalfSize(), relativeOrientation, relativePosition).Getb();
				const bool retVal = COT_TestBoxToBoxOBBFaces(GetCurBoundingBoxHalfSize(), otherObjectState.GetCurBoundingBoxHalfSize(), relativeOrientation, relativePosition);
#else
				QuatV relativeOrientation2;
				Vec3V relativePosition2;
				UnTransform(relativeOrientation2, relativePosition2, otherObjectState.GetCurBoundToWorldOrientation(), otherObjectState.GetCurBoundingBoxCenter(), GetCurBoundToWorldOrientation(), GetCurBoundingBoxCenter());
				Mat34V relativeMatrix;
				Mat34VFromQuatV(relativeMatrix, relativeOrientation2, relativePosition2);
				const ScalarV vsZero(V_ZERO);
				const Vec4V boxHalfExtents0 = Vec4V(GetCurBoundingBoxHalfSize(), vsZero);
				const Vec4V boxHalfExtents1 = Vec4V(otherObjectState.GetCurBoundingBoxHalfSize(), vsZero);
				const bool retVal = COT_TestBoxToBoxOBBFaces(boxHalfExtents0.GetXYZ(), boxHalfExtents1.GetXYZ(), relativeMatrix);
#endif
#endif	// !MIDPHASE_USES_QUATS
				return retVal;
			}

			case OBJECTSTATETYPE_SUBTREE:
			{
				// We've already quantized the bounds of the other object state so let's check if they overlap with our current node.
				PPC_STAT_COUNTER_INC(NodeOverlapTestCounter,1);
				const int curNodeIndex = GetStartIndex();
				const phOptimizedBvhNode &curNode = GetBvhNode(curNodeIndex);
				const bool nodeOverlaps = curNode.DoesQuantizedAABBOverlap(m_AABBMinQuant, m_AABBMaxQuant);
				return nodeOverlaps;
			}

			case OBJECTSTATETYPE_COMPONENTS:
			{
				// TODO: Really this should be collapsed into the OBJECTSTATETYPE_LEAF section above because now we *can* perform culling here.  We should
				//   never get into TestOverlap() with OBJECTSTATETYPE_COMPONENTS except on the first time through the loop.
				ASSERT_ONLY(const phBound *curCullingBound = GetCurCullingBound());
				FastAssert(curCullingBound->GetType() == phBound::COMPOSITE);
				ASSERT_ONLY(const phBoundComposite *compositeBound = static_cast<const phBoundComposite *>(curCullingBound));
				ASSERT_ONLY(const int numComponents = compositeBound->GetNumBounds());
				FastAssert(compositeBound->GetBound(GetStartIndex()) != NULL);
				FastAssert(GetEndIndex() == numComponents);

				return true;
			}

			case OBJECTSTATETYPE_COMPONENT:
			{
				return TestOverlapComponent(otherObjectState);
			}

			default:
			{
				Assert(GetStateType() == OBJECTSTATETYPE_SUBTREEHEADER);
				PPC_STAT_COUNTER_INC(NodeOverlapTestCounter,1);
				// We've already quantized the bounds of the other object state so let's check if they overlap with our current subtree.
				FastAssert(GetEndIndex() - GetStartIndex() == 1);
				const int curSubtreeHeaderIndex = GetStartIndex();
				const phOptimizedBvh::phBVHSubtreeInfo &curSubtree = GetSubtreeHeader(curSubtreeHeaderIndex);
				if(!curSubtree.DoesQuantizedAABBOverlap(m_AABBMinQuant, m_AABBMaxQuant))
				{
					return false;
				}

				const int startNodeIndex = SPU_ONLY(0) NON_SPU_ONLY(curSubtree.m_RootNodeIndex);
				const int endNodeIndex = SPU_ONLY(curSubtree.m_LastIndex - curSubtree.m_RootNodeIndex) NON_SPU_ONLY(curSubtree.m_LastIndex);

				// Return right now if we have no nodes
				if(Unlikely(startNodeIndex == endNodeIndex))
				{
					return false;
				}

				SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(DMATAG_BVHNODES)));	// Make sure that the DMA of the nodes has finished.
				InitSubtree(startNodeIndex, endNodeIndex, otherObjectState);
				// InitSubtree could have left us as a component, which isn't something that we want to pass to Iterate().  If we find that that has happened,
				//   let's go ahead and test that component now and (potentially) advance ourself into that component.
				// TODO: Maybe we should just leave ourself as a component and have Iterate() just ignore that type when it comes through?
				if(GetStateType() == OBJECTSTATETYPE_COMPONENT)
				{
					return TestOverlapComponent(otherObjectState);
				}
				return true;
			}
		}
	}

	// Evaluate whether it would be better 
	bool ShouldSubdivideOtherObjectState(const ObjectState &otherObjectState) const
	{
		if(otherObjectState.GetStateType() == OBJECTSTATETYPE_LEAF)
		{
			// The other object can't be subdivided, so obviously the answer is no.
			return false;
		}

		if(GetStateType() == OBJECTSTATETYPE_LEAF)
		{
			// We can't be subdivided but the other object can, so obviously the answer is yes.
			return true;
		}

		// If we got here both objects are divisible and so we have a choice as to which one we should subdivide.  We'll choose the one with the larger bounding box
		//   volume.  There might be better heuristics for this though.  Perhaps something like picking the object whose bounding box has the largest maximal dimension
		//   would work better.
		// NOTE: Simply comparing these as integers should be perfectly valid (and faster) as both volumes are positive values.
	// This fastAssert was found to be failing very rarely. Presumably that could only occur if otherVolume is NaN, or if either is negative
	// - We're not sure how such values could occur but we're placing validation earlier and would prefer not to crash here, so removing the fastAssert
	//	FastAssert((IsGreaterThanAll(otherObjectState.GetCurBoundingBoxVolume(), GetCurBoundingBoxVolume()) != 0) == 
	//			(otherObjectState.GetCurBoundingBoxVolumeAsInt() > GetCurBoundingBoxVolumeAsInt()));
		if(otherObjectState.GetCurBoundingBoxVolumeAsInt() > GetCurBoundingBoxVolumeAsInt())
		{
			return true;
		}

		return false;
	}

	// Iterate() is the fundamental operation in this midphase algorithm.  It takes an ObjectState that is ready to be subdivided subdivides it into two
	//   portions - one portion upon which work should continue (which remains in the calling object) and the work which should be resumed later (which is
	//   put into newObjectState).
	void Iterate(phCollisionWorkStack <COLLISION_WORK_STACK_DEPTH> *collisionWorkStack, ObjectState &otherObjectState);

	// Pop() is both for handling clean-up work (such as freeing memory) that should occur when an object state is no longer needed but also for determining
	//   what should actually be the state of the collision work stack.
	void Pop(phCollisionWorkStack <COLLISION_WORK_STACK_DEPTH> *collisionWorkStack, ObjectState &otherObjectState);

private:
	// When on SPU, this also handles the DMAs of 'sub' pieces of bounds (vertices, etc).
	// Return value indicates whether or not anything was found to collide with.  (Degenerate bounds such as 'flat' composites with no non-NULL bounds will be
	//   rejected at this stage.)
	bool InitState(const phBound *curBound)
	{
		FastAssert(curBound != NULL);
		SPU_ONLY(phSpuCollisionBuffers *collisionBuffers = GetSpuCollisionBuffers());

		SetCurCullingBound(curBound);
		// Init the object state type, start index and end index and perhaps BVH structure pointer according to the specifics of the bound that we're given.
		switch(curBound->GetType())
		{
		case phBound::COMPOSITE:
			{
				const phBoundComposite *compositeBound = static_cast<const phBoundComposite *>(curBound);
				const int numComponents = compositeBound->GetNumBounds();

				const phOptimizedBvh *compositeBVHStructure = compositeBound->GetBVHStructure();
				if(compositeBVHStructure != NULL)
				{
					// On SPU, this is where the DMA for the core part of the BVH structure gets kicked off.  We want to start that as soon as possible.
					InitBVHStructure(compositeBVHStructure);
				}

#if __SPU
				phBoundComposite *nonConstCompositeBound = const_cast<phBoundComposite *>(compositeBound);

				// The bound array was already DMA'ed in collisiontask.cpp
				//phBound **spuComponentPointers = reinterpret_cast<phBound **>(collisionBuffers->GetBlock(sizeof(phBound *) * numComponents));
				//sysDmaLargeGet(spuComponentPointers, (uint64_t)compositeBound->GetBoundArray(), sizeof(phBound *) * numComponents, DMA_TAG(DMATAG_COMPONENTPOINTERS));

				const bool hasTypeAndIncludeFlags = compositeBound->GetTypeAndIncludeFlags() != NULL;
				u32 *spuTypeAndIncludeFlags = NULL;
				if(hasTypeAndIncludeFlags)
				{
					spuTypeAndIncludeFlags = reinterpret_cast<u32 *>(collisionBuffers->GetBlock(sizeof(u32) * 2 * numComponents));
					sysDmaLargeGet(spuTypeAndIncludeFlags, (uint64_t)compositeBound->GetTypeAndIncludeFlags(), sizeof(u32) * 2 * numComponents, DMA_TAG(DMATAG_PERCOMPONENTDATA));
				}

				// Composite bounds with BVH structures can tend to have a lot of components, so we don't DMA over all of the large composite matrices and box
				//   extents here, instead we DMA over only what we need and when we need it (currently this is in InitSubtree()).
				// We only do this for the current and last matrices and the local box extents because a) they're by far the majority of the data and b) they're
				//   only used in one place so just grabbing what we need and using it then is much simpler than what we'd have to do to extend this idea to
				//   the bound pointers and type and include flags.
				if(compositeBVHStructure == NULL)
				{
					Mat34V *spuCurrentMatrices = reinterpret_cast<Mat34V *>(collisionBuffers->GetBlock(sizeof(Mat34V) * numComponents));
					sysDmaLargeGet(spuCurrentMatrices, (uint64_t)compositeBound->GetCurrentMatrices(), sizeof(Mat34V) * numComponents, DMA_TAG(DMATAG_PERCOMPONENTDATA));

					Mat34V *spuLastMatrices = reinterpret_cast<Mat34V *>(collisionBuffers->GetBlock(sizeof(Mat34V) * numComponents));
					sysDmaLargeGet(spuLastMatrices, (uint64_t)compositeBound->GetLastMatrices(), sizeof(Mat34V) * numComponents, DMA_TAG(DMATAG_PERCOMPONENTDATA));

					Vec3V *spuLocalBoxMinMaxs = reinterpret_cast<Vec3V *>(collisionBuffers->GetBlock(sizeof(Vec3V) * 2 * numComponents));
					sysDmaLargeGet(spuLocalBoxMinMaxs, (uint64_t)compositeBound->GetLocalBoxMinMaxsArray(), sizeof(Vec3V) * 2 * numComponents, DMA_TAG(DMATAG_PERCOMPONENTDATA));

					nonConstCompositeBound->m_CurrentMatrices = spuCurrentMatrices;
					nonConstCompositeBound->m_LastMatrices = spuLastMatrices;
					nonConstCompositeBound->m_LocalBoxMinMaxs = spuLocalBoxMinMaxs;
				}
				else
				{
					InitBVHStructureSpuExtra();
				}

				//nonConstCompositeBound->SetBoundArray(spuComponentPointers);
				nonConstCompositeBound->m_TypeAndIncludeFlags = spuTypeAndIncludeFlags;

				sysDmaWaitTagStatusAll(DMA_MASK(DMATAG_COMPONENTPOINTERS));
#endif
				// Q: Why do we do this only if there's no BVH structure?
				// A: Because, in that case, this is essentially free.  We'll be iterating over the components of a 'flat' composite anyway so getting
				//   a head start on that here (this work isn't wasted, we pick up later where we leave off here) really doesn't impose any extra
				//   work.  There are a number of reason not to do this for BVH-wrapped composites.  For one thing, they tend to be larger than 'flat'
				//   composites so this loop could end up iterating over more components.  Second, we expect it to be uncommon, at worst, for an
				//   empty composite to be in existence so this loop, regardless of how little work it does, would just be extra work.  And finally,
				//   we have to be able to handle finding NULL components in a BVH-wrapped composite anyway and catching empty composites here doesn't
				//   relieve us of having to handle that later.
				if(compositeBVHStructure == NULL)
				{
					// Set the start index to correspond to the first component that is non-NULL.
					int componentIndex = 0;
					while(Likely(componentIndex < numComponents) && Unlikely(compositeBound->GetBound(componentIndex) == NULL))
					{
						++componentIndex;
					}

					// Return false if we found an all-NULL composite bound.
					if(Unlikely(componentIndex == numComponents))
					{
						return false;
					}

					SetStartIndex(componentIndex);
					SetEndIndex(numComponents);
					SetStateType(OBJECTSTATETYPE_COMPONENTS);
				}

				SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(DMATAG_PERCOMPONENTDATA)));

				break;
			}
		case phBound::BVH:
			{
				const phBoundBVH *bvhBound = static_cast<const phBoundBVH *>(curBound);
				const phOptimizedBvh *bvhStructure = bvhBound->GetBVH();
				InitBVHStructure(bvhStructure);	// If on SPU, kick off the DMA of the core BVH structure as soon as possible.

#if __SPU
				phBoundBVH *nonConstBvhBound = const_cast<phBoundBVH *>(bvhBound);
				const int numMaterialIds = bvhBound->GetNumMaterials();
				phMaterialMgr::Id *spuMaterialIdBuffer = reinterpret_cast<phMaterialMgr::Id *>(collisionBuffers->GetBlock(sizeof(phMaterial) * numMaterialIds));
				sysDmaLargeGet(spuMaterialIdBuffer, (uint64_t)bvhBound->m_MaterialIds, numMaterialIds * sizeof(phMaterialMgr::Id), DMA_TAG(2));
				nonConstBvhBound->m_MaterialIds = spuMaterialIdBuffer;
#endif	// __SPU

				SPU_ONLY(InitBVHStructureSpuExtra());
				SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(2)));	// Could wait on this a LOT later.
				break;
			}
#if USE_GRIDS
		case phBound::GRID:
			{
				// Not supported yet.
				Assert(false);
				break;
			}
#endif
#if USE_GEOMETRY_CURVED
		case phBound::GEOMETRY_CURVED:
			{
#if __SPU
				phBoundCurvedGeometry *curvedGeomBound = const_cast<phBoundCurvedGeometry *>(static_cast<const phBoundCurvedGeometry *>(curBound));

				const int numCurvedEdges = curvedGeomBound->GetNumCurvedEdges();
				if(Likely(numCurvedEdges > 0))
				{
					phCurvedEdge *spuCurvedEdgeBuffer = reinterpret_cast<phCurvedEdge *>(GetSpuCollisionBuffers()->GetBlock(sizeof(phCurvedEdge) * numCurvedEdges));
					sysDmaLargeGet(spuCurvedEdgeBuffer, (uint64_t)&curvedGeomBound->GetCurvedEdge(0), numCurvedEdges * sizeof(phCurvedEdge), DMA_TAG(1));
					curvedGeomBound->SetCurvedEdges(spuCurvedEdgeBuffer);
				}

				const int numCurvedFaces = curvedGeomBound->GetNumCurvedFaces();
				if(Likely(numCurvedFaces > 0))
				{
					phCurvedFace *curvedFaceBuffer = reinterpret_cast<phCurvedFace *>(GetSpuCollisionBuffers()->GetBlock(sizeof(phCurvedFace) * numCurvedFaces));
					sysDmaLargeGet(curvedFaceBuffer, (uint64_t)&curvedGeomBound->GetCurvedFace(0), numCurvedFaces * sizeof(phCurvedFace), DMA_TAG(1));
					curvedGeomBound->SetCurvedFaces(curvedFaceBuffer);
				}
#endif	// __SPU
				// Intentional fall-through to phBound::GEOMETRY case.
			}
#endif // USE_GEOMETRY_CURVED
		case phBound::GEOMETRY:
			{
#if __SPU
				phBoundGeometry *geomBound = const_cast<phBoundGeometry *>(static_cast<const phBoundGeometry *>(curBound));
				DmaBoundGeometry(geomBound,collisionBuffers);
				DmaBoundGeometryWait();
#endif	// __SPU
				SetStateType(OBJECTSTATETYPE_LEAF);
			}
		default:
			{
				SetStateType(OBJECTSTATETYPE_LEAF);
				break;
			}
		}

		return true;
	}

	void InitBVHStructure(const phOptimizedBvh *bvhStructure)
	{
#if __SPU
		phSpuCollisionBuffers &collisionBuffers = *GetSpuCollisionBuffers();
		phOptimizedBvh *spuBvhStructure = reinterpret_cast<phOptimizedBvh *>(collisionBuffers.GetBlock(sizeof(phOptimizedBvh)));
		sysDmaLargeGet(spuBvhStructure, (uint64_t)bvhStructure, sizeof(phOptimizedBvh), DMA_TAG(DMATAG_BVHSTRUCTURE));
		SetStateTypeAndBVHStructure(OBJECTSTATETYPE_SUBTREEHEADERS, spuBvhStructure);
		SetStartIndex(0);
#else	// __SPU
		// TODO: Perhaps we should be more like the SPU and do a prefetch here and then not dereference the BVH structure until the second InitBVHStructureXXX() call?
		SetStateTypeAndBVHStructure(OBJECTSTATETYPE_SUBTREEHEADERS, bvhStructure);
		SetBvhNodes(&bvhStructure->GetNode(0));
		SetStartIndex(0);
		SetEndIndex(bvhStructure->GetNumUsedSubtreeHeaders());
#endif	// __SPU
	}

#if __SPU
	void InitBVHStructureSpuExtra()
	{
		phSpuCollisionBuffers &collisionBuffers = *GetSpuCollisionBuffers();
		phOptimizedBvh *spuBvhStructure = GetBvhStructure();

		sysDmaWaitTagStatusAll(DMA_MASK(DMATAG_BVHSTRUCTURE));

		const int numSubtreeHeaders = spuBvhStructure->GetNumUsedSubtreeHeaders();
		phOptimizedBvh::phBVHSubtreeInfo *spuSubtreeHeaders = reinterpret_cast<phOptimizedBvh::phBVHSubtreeInfo *>(collisionBuffers.GetBlock(sizeof(phOptimizedBvh::phBVHSubtreeInfo) * numSubtreeHeaders));
		sysDmaLargeGet(spuSubtreeHeaders, (uint64_t)spuBvhStructure->GetSubtreeHeaders(), sizeof(phOptimizedBvh::phBVHSubtreeInfo) * numSubtreeHeaders, DMA_TAG(DMATAG_SUBTREEHEADERS));
		spuBvhStructure->SetSubtreeHeadersPtr(spuSubtreeHeaders);
		SetEndIndex(numSubtreeHeaders);
	}
#endif	// __SPU

#if __SPU
	__forceinline void InitDmaBvhNodesFromSubtreeHeader(const phOptimizedBvh::phBVHSubtreeInfo &subtreeHeader)
	{
		sysDmaWaitTagStatusAll(DMA_MASK(DMATAG_SUBTREEHEADERS));

		const int subtreeRootNodeIndex = subtreeHeader.m_RootNodeIndex;
		const int subtreeEndNodeIndex = subtreeHeader.m_LastIndex;

		const phOptimizedBvh *bvhStructure = GetBvhStructure();
		const int numNodesInSubtree = subtreeEndNodeIndex - subtreeRootNodeIndex;
		phOptimizedBvhNode *spuBvhNodes = reinterpret_cast<phOptimizedBvhNode *>(GetSpuCollisionBuffers()->GetBlock(sizeof(phOptimizedBvhNode) * numNodesInSubtree));
		sysDmaLargeGet(spuBvhNodes, (uint64_t)&bvhStructure->GetNode(subtreeRootNodeIndex), sizeof(phOptimizedBvhNode) * numNodesInSubtree, DMA_TAG(DMATAG_BVHNODES));
		SetBvhNodes(spuBvhNodes);
	}
#endif	// __SPU

	__forceinline void InitSubtreeHeader()
	{
		SetStateType(OBJECTSTATETYPE_SUBTREEHEADER);
	}

	// Depending on nature of the subtree, this could leave us with different object state types.  If the subtree contains more than one node the object state
	//   type will become 'subtree'.  If the subtree contains only one node then the object state type will become either 'component' or 'leaf' depending on
	//   whether or not the BVH structure came from a composite.
	// On SPU, InitSubtree() doesn't/shouldn't get called until the nodes themselves are available (DMA'd in).
	void InitSubtree(int subtreeRootNodeIndex, int subtreeEndNodeIndex, const ObjectState &otherObjectState)
	{
		const phBound *curCullingBound = GetCurCullingBound();
		const bool bvhFromComposite = (curCullingBound->GetType() == phBound::COMPOSITE);
		const bool subtreeTreeIsLeaf = (subtreeEndNodeIndex - subtreeRootNodeIndex == 1);
		const phOptimizedBvhNode &rootNode = GetBvhNode(subtreeRootNodeIndex);
		Assert(rootNode.IsLeafNode() == (subtreeEndNodeIndex - subtreeRootNodeIndex == 1));
		if(subtreeTreeIsLeaf && bvhFromComposite)
		{
			// This is a BVH structure that came out of a composite.
			const int componentIndex = rootNode.GetPolygonStartIndex();
			const phBoundComposite *compositeBound = static_cast<const phBoundComposite *>(curCullingBound);
			//	Removed assert here that used to check that the component is non-NULL.  We now handle that (even though we'd prefer not to waste time just to end up with a NULL component).
#if __SPU
			// For BVH structures from composites we have to grab these matrices/vectors now since we don't just grab the entire set at the beginning.
			Mat34V componentCurrentMatrix, componentLastMatrix;
			Vec3V componentBoxLocalMin, componentBoxLocalMax;
			sysDmaLargeGet(&componentCurrentMatrix, (uint64_t)&compositeBound->GetCurrentMatrices()[componentIndex], 64, DMA_TAG(1));
			sysDmaLargeGet(&componentLastMatrix, (uint64_t)&compositeBound->GetLastMatrices()[componentIndex], 64, DMA_TAG(1));
			sysDmaLargeGet(&componentBoxLocalMin, (uint64_t)&compositeBound->GetLocalBoxMinMaxsArray()[componentIndex * 2], 16, DMA_TAG(1));
			sysDmaLargeGet(&componentBoxLocalMax, (uint64_t)&compositeBound->GetLocalBoxMinMaxsArray()[componentIndex * 2 + 1], 16, DMA_TAG(1));
#else	// __SPU
			Mat34V_ConstRef componentCurrentMatrix = compositeBound->GetCurrentMatrix(componentIndex);
			Mat34V_ConstRef componentLastMatrix = compositeBound->GetLastMatrix(componentIndex);
			const Vec3V componentBoxLocalMin = compositeBound->GetLocalBoxMins(componentIndex);
			const Vec3V componentBoxLocalMax = compositeBound->GetLocalBoxMaxs(componentIndex);
#endif	// __SPU
			SetStartIndex(componentIndex);
			SetEndIndex(componentIndex + 1);	// Doesn't really matter what we set this to but, technically, this is correct at this moment.

			SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(1)));
#if !MIDPHASE_USES_QUATS
			InitCompositeBoundingBox(GetCurBoundToWorldMatrix(), GetLastBoundToWorldMatrix(), componentCurrentMatrix, componentLastMatrix, componentBoxLocalMin, componentBoxLocalMax);
#else	// !MIDPHASE_USES_QUATS
			InitCompositeBoundingBox(GetCurBoundToWorldOrientation(), GetCurBoundToWorldOffset(), GetLastBoundToWorldOrientation(), GetLastBoundToWorldOffset(), componentCurrentMatrix, componentLastMatrix, componentBoxLocalMin, componentBoxLocalMax);
#endif	// !MIDPHASE_USES_QUATS
			SetStateType(OBJECTSTATETYPE_COMPONENT);

			// We've changed our 'bound to world' matrix so we need to re-determine the culling box information relative to this new matrix.
			InitCullingBoxInfo(otherObjectState);
		}
		else
		{
			// Calculate the bounding box here by unquantizing the node.  This is needed since we're switching from a BVH subtree to a single node with primitives in 
			//   it (which is a 'leaf' and hence needs a bounding box) OR we're just descending into a new subtree and need to re-compute the volume so that we can
			//   decide whether we need to start sub-dividing the other bound or not.
			const phOptimizedBvh *bvhStructure = GetBvhStructure();
#if !MIDPHASE_USES_QUATS
			InitBoundingBoxInfoFromBVHNode(bvhStructure, rootNode, GetCurBoundToWorldMatrix(), GetLastBoundToWorldMatrix());
#else	// !MIDPHASE_USES_QUATS
			InitBoundingBoxInfoFromBVHNode(bvhStructure, rootNode, GetCurBoundToWorldOrientation(), GetCurBoundToWorldOffset(), GetLastBoundToWorldOrientation(), GetLastBoundToWorldOffset());
#endif	// !MIDPHASE_USES_QUATS
			SetStartIndex(subtreeRootNodeIndex);
			SetEndIndex(subtreeEndNodeIndex);
			if(Likely(!subtreeTreeIsLeaf))
			{
				SetStateType(OBJECTSTATETYPE_SUBTREE);
			}
			else
			{
				// The subtree is just a primitive-containing leaf node.
				SetStateType(OBJECTSTATETYPE_LEAF);
			}
		}
	}

	// Nothing in here accesses the sub-bound itself which helps us by allowing us to defer DMA waiting/avoid touching that memory.
#if !MIDPHASE_USES_QUATS
	void InitCompositeBoundingBox(const phBoundComposite *compositeBound, Mat34V_In curBoundToWorldMatrix, Mat34V_In lastBoundToWorldMatrix, int componentIndex)
#else	// !MIDPHASE_USES_QUATS
	void InitCompositeBoundingBox(const phBoundComposite *compositeBound, QuatV_In curBoundToWorldOrientation, Vec3V_In curBoundToWorldOffset, QuatV_In lastBoundToWorldOrientation, Vec3V_In lastBoundToWorldOffset, int componentIndex)
#endif	// !MIDPHASE_USES_QUATS
	{
		// NOTE: This would be a good place to initiate the DMA of the sub-bound when on SPU.
		// Figure out the new, current and last, bound to world matrices.
		SPU_ONLY(Assert(GetSpuCollisionBuffers()->IsValidPointer(compositeBound->GetCurrentMatrices())));
		SPU_ONLY(Assert(GetSpuCollisionBuffers()->IsValidPointer(compositeBound->GetLastMatrices())));
		Mat34V_ConstRef componentCurMatrix = compositeBound->GetCurrentMatrix(componentIndex);
		Mat34V_ConstRef componentLastMatrix = compositeBound->GetLastMatrix(componentIndex);

		const Vec3V vBoundingBoxMin = compositeBound->GetLocalBoxMins(componentIndex);
		const Vec3V vBoundingBoxMax = compositeBound->GetLocalBoxMaxs(componentIndex);

#if !MIDPHASE_USES_QUATS
		InitCompositeBoundingBox(curBoundToWorldMatrix, lastBoundToWorldMatrix, componentCurMatrix, componentLastMatrix, vBoundingBoxMin, vBoundingBoxMax);
#else	// !MIDPHASE_USES_QUATS
		InitCompositeBoundingBox(curBoundToWorldOrientation, curBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset, componentCurMatrix, componentLastMatrix, vBoundingBoxMin, vBoundingBoxMax);
#endif	// !MIDPHASE_USES_QUATS
	}

	// Nothing in here accesses the sub-bound itself which helps us by allowing us to defer DMA waiting/avoid touching that memory.
	// This is a version that doesn't look into the arrays of cached data in the phBoundComposite either, for situations (like on SPU) where those pointers
	//   have not been set up.
#if !MIDPHASE_USES_QUATS
	void InitCompositeBoundingBox(Mat34V_In curBoundToWorldMatrix, Mat34V_In lastBoundToWorldMatrix, Mat34V_In componentCurMatrix, Mat34V_In componentLastMatrix, Vec3V_In localBoxMin, Vec3V_In localBoxMax)
#else	// !MIDPHASE_USES_QUATS
	void InitCompositeBoundingBox(QuatV_In curBoundToWorldOrientation, Vec3V_In curBoundToWorldOffset, QuatV_In lastBoundToWorldOrientation, Vec3V_In lastBoundToWorldOffset, Mat34V_In componentCurMatrix, Mat34V_In componentLastMatrix, Vec3V_In localBoxMin, Vec3V_In localBoxMax)
#endif	// !MIDPHASE_USES_QUATS
	{
#if !MIDPHASE_USES_QUATS
		Mat34V newCurBoundToWorldMatrix, newLastBoundToWorldMatrix;
		Transform(newCurBoundToWorldMatrix, curBoundToWorldMatrix, componentCurMatrix);
		Transform(newLastBoundToWorldMatrix, lastBoundToWorldMatrix, componentLastMatrix);
		SetBoundToWorldMatrices(newCurBoundToWorldMatrix, newLastBoundToWorldMatrix);
#else	// !MIDPHASE_USES_QUATS
		QuatV newCurBoundToWorldOrientation, newLastBoundToWorldOrientation;
		Vec3V newCurBoundToWorldOffset, newLastBoundToWorldOffset;
		Transform(newCurBoundToWorldOrientation, newCurBoundToWorldOffset, curBoundToWorldOrientation, curBoundToWorldOffset, QuatVFromMat34V(componentCurMatrix), componentCurMatrix.GetCol3());
		Transform(newLastBoundToWorldOrientation, newLastBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset, QuatVFromMat34V(componentLastMatrix), componentLastMatrix.GetCol3());
		SetBoundToWorldMatrices(newCurBoundToWorldOrientation, newCurBoundToWorldOffset, newLastBoundToWorldOrientation, newLastBoundToWorldOffset);
#endif	// !MIDPHASE_USES_QUATS

		// Initialize ourselves to represent this particular component.  This call will handle setting m_ObjectStateType too.
#if !MIDPHASE_USES_QUATS
		InitBoundingBoxInfo(localBoxMin, localBoxMax, newCurBoundToWorldMatrix, newLastBoundToWorldMatrix);
#else	// !MIDPHASE_USES_QUATS
		InitBoundingBoxInfo(localBoxMin, localBoxMax, newCurBoundToWorldOrientation, newCurBoundToWorldOffset, newLastBoundToWorldOrientation, newLastBoundToWorldOffset);
#endif	// !MIDPHASE_USES_QUATS
	}

	void InitCompositeComponent(const phBound *newCullingBound, int componentIndex)
	{
		SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(DMATAG_COMPOSITECOMPONENT)));
		Assert(newCullingBound->GetType() != phBound::COMPOSITE);
		InitState(newCullingBound);
		m_ComponentIndex = (u16)(componentIndex);				
	}

	bool TestOverlapComponent(const ObjectState &otherObjectState)
	{
		FastAssert(GetEndIndex() - GetStartIndex() == 1);
		const int componentIndex = GetStartIndex();
		const phBound *curCullingBound = GetCurCullingBound();
		FastAssert(curCullingBound->GetType() == phBound::COMPOSITE);
		const phBoundComposite *boundComposite = static_cast<const phBoundComposite *>(curCullingBound);

		if(Unlikely(boundComposite->GetBound(componentIndex) == NULL))
		{
			FastAssert(GetBvhStructure() != NULL);
			// For 'flat' (non-BVH-wrapped) composites this should be impossible as we skip to non-NULL components during iteration.  For BVH-wrapped
			//   composites we don't know what the next component will be until shortly before this point, and this is our first opportunity handle it.
			return false;
		}
#if __SPU
		phBound *spuBoundBuffer = reinterpret_cast<phBound *>(GetSpuCollisionBuffers()->GetBlock(phBound::MAX_BOUND_SIZE));
		sysDmaLargeGet(spuBoundBuffer, (uint64_t)boundComposite->GetBound(componentIndex), phBound::MAX_BOUND_SIZE, DMA_TAG(DMATAG_COMPOSITECOMPONENT));
#endif	// __SPU
		if(boundComposite->GetTypeAndIncludeFlags() != NULL)
		{
			NOTFINAL_ONLY(SPU_ONLY(GetSpuCollisionBuffers()->IsValidPointer(boundComposite->GetTypeAndIncludeFlags())));
			const u32 curTypeFlags = boundComposite->GetTypeFlags(componentIndex);
			const u32 curIncludeFlags = boundComposite->GetIncludeFlags(componentIndex);
			if((curTypeFlags & otherObjectState.GetIncludeFlags()) == 0 || (curIncludeFlags & otherObjectState.GetTypeFlags()) == 0)
			{
				// Type and include flags don't match up.  Let's bail.
				return false;
			}
			SetTypeAndIncludeFlags(curTypeFlags, curIncludeFlags);
		}

		// TODO: This is duplicated from above - that sucks.
		// We've passed the type and include flags test.  Now let's check the box-vs-box test.
#if !MIDPHASE_USES_QUATS
		const ScalarV vsZero(V_ZERO);
		const Vec4V boxHalfExtents0 = Vec4V(GetCurBoundingBoxHalfSize(), vsZero);
		const Vec4V boxHalfExtents1 = Vec4V(otherObjectState.GetCurBoundingBoxHalfSize(), vsZero);

		// NOTE: The geomBoxes::TestBoxXXX() functions seem to want 0 in the space of 1, which contradicts how I interpret the documentation.
		Mat34V relativeMatrix;
		UnTransformOrtho(relativeMatrix, otherObjectState.GetCurBoundingBoxMatrix(), GetCurBoundingBoxMatrix());

		if(!COT_TestBoxToBoxOBBFaces(boxHalfExtents0.GetXYZ(), boxHalfExtents1.GetXYZ(), relativeMatrix))
		{
			return false;
		}
#else	// !MIDPHASE_USES_QUATS
		QuatV relativeOrientation;
		Vec3V relativePosition;
		UnTransform(relativeOrientation, relativePosition, GetCurBoundToWorldOrientation(), GetCurBoundingBoxCenter(), otherObjectState.GetCurBoundToWorldOrientation(), otherObjectState.GetCurBoundingBoxCenter());

		//if(!geomBoxes::TestBoxToBoxOBBFaces(GetCurBoundingBoxHalfSize(), otherObjectState.GetCurBoundingBoxHalfSize(), relativeOrientation, relativePosition).Getb())
		if(!COT_TestBoxToBoxOBBFaces(GetCurBoundingBoxHalfSize(), otherObjectState.GetCurBoundingBoxHalfSize(), relativeOrientation, relativePosition))
		{
			return false;
		}
#endif	// !MIDPHASE_USES_QUATS

		// We've passed the type and include test and the box-vs-box test, now we can feel free to go ahead and touch the bound component's memory since
		//   we know we're going to be performing collision detection.
#if !__SPU
		InitCompositeComponent(boundComposite->GetBound(componentIndex), componentIndex);
#else
		InitCompositeComponent(spuBoundBuffer, componentIndex);
#endif
		InitCullingBoxInfo(otherObjectState);

		return true;
	}

	enum ObjectStateType
	{
		OBJECTSTATETYPE_COMPONENTS	=	0,		// Representing a range of components out of a composite.
		OBJECTSTATETYPE_COMPONENT,				// Representing a single component of a composite.
		OBJECTSTATETYPE_SUBTREEHEADERS,			// Representing a range of BVH subtree headers.
		OBJECTSTATETYPE_SUBTREEHEADER,			// Representing a single BVH subtree header (ie, before it's been descended into).
		OBJECTSTATETYPE_SUBTREE,				// Representing a single subtree of a BVH.
		OBJECTSTATETYPE_LEAF,					// In a terminal state, there is no more subdividing to do here.  Typically this is a single BVH node or a single bound.
	};

	__forceinline ObjectStateType GetStateType() const
	{
		return (ObjectStateType)(m_PackedBVHStructureAndStateType & 15);
	}

	__forceinline void SetCollisionBoundIndex(int collisionBoundIndex)
	{
		FastAssert(collisionBoundIndex == 0 || collisionBoundIndex == 1);
		m_Flags = (u16)(collisionBoundIndex);	// For now we're not using any more flags so we can just set this without any masking.
	}

#if !MIDPHASE_USES_QUATS
	__forceinline void SetBoundToWorldMatrices(Mat34V_In curBoundToWorldMatrix, Mat34V_In lastBoundToWorldMatrix)
#else	// !MIDPHASE_USES_QUATS
	__forceinline void SetBoundToWorldMatrices(QuatV_In curBoundToWorldOrientation, Vec3V_In curBoundToWorldOffset, QuatV_In lastBoundToWorldOrientation, Vec3V_In lastBoundToWorldOffset)
#endif	// !MIDPHASE_USES_QUATS
	{
#if !MIDPHASE_USES_QUATS
		// We store a pointer in the w component of m_CurBoundToWorldMatrix but it should never be the case that that will have been set to anything at this point.
		m_CurBoundToWorldMatrix = curBoundToWorldMatrix;
		m_LastBoundToWorldMatrix = lastBoundToWorldMatrix;
#else	// !MIDPHASE_USES_QUATS
		// We store a pointer in the w component of m_CurBoundToWorldTransform.m_pos but it should never be the case that that will have been set to
		//   anything at this point so we don't have to worry about stomping it.
		m_CurBoundToWorldTransform.SetRotation(curBoundToWorldOrientation);
		m_CurBoundToWorldTransform.SetPosition(curBoundToWorldOffset);
		m_LastBoundToWorldTransform.SetRotation(lastBoundToWorldOrientation);
		m_LastBoundToWorldTransform.SetPosition(lastBoundToWorldOffset);
#endif	// !MIDPHASE_USES_QUATS
	}

	__forceinline void SetStateType(ObjectStateType newObjectState)
	{
		FastAssert(newObjectState < 16);
		m_PackedBVHStructureAndStateType = (u32)(newObjectState) | (m_PackedBVHStructureAndStateType & ~15);
	}

	__forceinline void SetBVHStructure(const phOptimizedBvh *bvhStructure)
	{
		FastAssert(((size_t)(bvhStructure) & 15) == 0);
		m_PackedBVHStructureAndStateType = (size_t)(bvhStructure) | (m_PackedBVHStructureAndStateType & 15);
	}

	// This is more efficient than calling SetStateType() and SetBVHStructure() separately.
	__forceinline void SetStateTypeAndBVHStructure(ObjectStateType newObjectState, const phOptimizedBvh *bvhStructure)
	{
		FastAssert(newObjectState < 16);
		FastAssert(((size_t)(bvhStructure) & 15) == 0);
		m_PackedBVHStructureAndStateType = (size_t)(bvhStructure) | (newObjectState);
	}

	__forceinline void SetBvhNodes(const phOptimizedBvhNode *bvhNodes)
	{
#if __64BIT
		m_BvhNodes = bvhNodes;
#else
#if !MIDPHASE_USES_QUATS
		m_CurBoundToWorldMatrix.GetCol0Ref().SetWi(reinterpret_cast<u32>(bvhNodes));
#else	// !MIDPHASE_USES_QUATS
		m_CurBoundToWorldTransform.GetPositionRef().SetWi(reinterpret_cast<u32>(bvhNodes));
#endif	// !MIDPHASE_USES_QUATS
#endif	// __64BIT
	}

	__forceinline void SetStartIndex(const int startIndex)
	{
		FastAssert(startIndex < 65536);
		m_StartIndex = (u16)(startIndex);
	}

	__forceinline void SetEndIndex(const int endIndex)
	{
		FastAssert(endIndex < 65536);
		m_EndIndex = (u16)(endIndex);
	}

#if __SPU
	static phSpuCollisionBuffers *s_SpuCollisionBuffers;
#endif	// __SPU

	// When sub-dividing we might need to have a shape against which to do this.  These refer to the 'other' object and are stored in the local space of the bound 
	//   being sub-divided (us).  It turns out that we only really store this for when culling a BVH.
	s16 m_AABBMinQuant[3], m_AABBMaxQuant[3];

	enum ObjectStateFlags
	{
		OBJECTSTATEFLAG_COLLISIONBOUNDINDEX	=	1 << 0,		// To which object do we correspond in the root manifold?
	};

	u16 m_Flags;
	// Record where in a composite bound we might be.  Once we descend into a component in a composite bound this will be set to record into what component
	//   we descended.  It should be zero if we have never descended *through* a composite.
	u16 m_ComponentIndex;

	// These 
	Vec4V m_CurBoundingBoxHalfSizeXYZVolumeW;
	Vec4V m_CurBoundingBoxCenter_WSXYZCurBoundW;

	u32 m_TypeFlags;
	u32 m_IncludeFlags;
	size_t m_PackedBVHStructureAndStateType;	// The BVHStructure part of this is used when the state type is OBJECTSTATETYPE_SUBTREEHEADERS or OBJECTSTATETYPE_SUBTREE.
	u16 m_StartIndex;
	u16 m_EndIndex;

#if __64BIT
	const phOptimizedBvhNode *m_BvhNodes;
	const phBound *m_CurCullingBound;
#endif

#if MIDPHASE_USES_QUATS
	TransformV m_CurBoundToWorldTransform;
	TransformV m_LastBoundToWorldTransform;
#else	// MIDPHASE_USES_QUATS
	Mat34V m_CurBoundToWorldMatrix;
	Mat34V m_LastBoundToWorldMatrix;
#endif	// MIDPHASE_USES_QUATS
};

#if __SPU
phSpuCollisionBuffers *ObjectState::s_SpuCollisionBuffers = NULL;
#endif	// __SPU

template <int Depth> class phCollisionWorkStack
{
public:
	phCollisionWorkStack() :
	  m_ObjectStates(Depth * 2)
	{
		m_NextObjectStateIndex = 0;
	}

	// For efficiency (to avoid extra copies) the Push() method doesn't take two ObjectState objects and then copy them into its array (which would require
	//   the caller to create temporaries).  Instead, it gives back pointers to the two object states that are now at the top of the stack and allows the caller
	//   to fill them in directly.
	void Push(ObjectState *&newObjectState0, ObjectState *&newObjectState1)
	{
		const int nextObjectStateIndex = m_NextObjectStateIndex;
		newObjectState0 = &m_ObjectStates[nextObjectStateIndex];
		newObjectState1 = &m_ObjectStates[nextObjectStateIndex + 1];
		m_NextObjectStateIndex = nextObjectStateIndex + 2;
#if !__FINAL
		if(Unlikely(m_NextObjectStateIndex > Depth * 2))
		{
			Quitf("Overflowed collision work stack with depth %d.", Depth);
		}
#endif
	}

	void Pop(ObjectState &destObjectState0, ObjectState &destObjectState1)
	{
		FastAssert(!IsEmpty());
		const int nextObjectStateIndex = m_NextObjectStateIndex;
		destObjectState0 = m_ObjectStates[nextObjectStateIndex - 2];
		destObjectState1 = m_ObjectStates[nextObjectStateIndex - 1];
		m_NextObjectStateIndex = nextObjectStateIndex - 2;
	}

	bool IsEmpty() const
	{
		return (m_NextObjectStateIndex == 0);
	}

private:
	atFixedArray<ObjectState, Depth * 2> m_ObjectStates;
	int m_NextObjectStateIndex;
};

#endif // !USE_NEW_MID_PHASE || !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP


// phManifoldData is a class that wraps a manifold pointer and a pair of component indices into a neatly sortable object.  phCollisionCompositeManifoldMgr uses
//   this class to record the (PPU) pointers and component pairs of manifolds that we've obtained from the manifold pool so that we can conveniently access them
//   later (without DMAs or cache misses).
class phManifoldData
{
public:
	__forceinline void Set(phManifold *manifold, u8 componentA, u8 componentB)
	{
		FastAssert(manifold != NULL);
		m_Manifold = manifold;
		m_ComponentPair = (componentA << 8) + componentB;
	}
	__forceinline void Set(phManifold *manifold, u16 componentPair)
	{
		FastAssert(manifold != NULL);
		m_Manifold = manifold;
		m_ComponentPair = componentPair;
	}

	__forceinline phManifold *GetManifold() const
	{
		return m_Manifold;
	}

	__forceinline u8 GetComponentA() const
	{
		return (m_ComponentPair >> 8);
	}

	__forceinline u8 GetComponentB() const
	{
		return (m_ComponentPair & 0xFF);
	}

	__forceinline u16 GetComponentPair() const
	{
		return m_ComponentPair;
	}

	static int CompareFn(const void *p0, const void *p1)
	{
		const phManifoldData *manifoldData0 = reinterpret_cast<const phManifoldData *>(p0);
		const phManifoldData *manifoldData1 = reinterpret_cast<const phManifoldData *>(p1);

		const u32 componentPair0 = manifoldData0->GetComponentPair();
		const u32 componentPair1 = manifoldData1->GetComponentPair();

		FastAssert(componentPair0 != componentPair1);	// Should never have duplicates.
		return componentPair1 > componentPair0 ? -1 : 1;
	}

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	__forceinline void SetTouched(bool touched)
	{
		m_Touched = touched;
	}

	__forceinline bool GetTouched() const
	{
		return m_Touched;
	}
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

private:
	phManifold *m_Manifold;
	u16 m_ComponentPair;
#if CHECK_FOR_DUPLICATE_MANIFOLDS
	bool m_Touched : 1;
#endif
};


// phCollisionCompositeManifoldMgr handles the different things that must be done to manage the list of composite manifolds when collision occurs with a composite
//   object.  This includes determining what manifold to use for a given collision, keeping track of where that manifold came from (could be an existing manifold
//   or a freshly allocated one), minimizing hits on the manifold pool (which incur synchronization costs) and updating the composite manifold list when collision
//   detection for this object pair is complete.
template <int Size> class phCollisionCompositeManifoldMgr
{
public:
	phCollisionCompositeManifoldMgr(phManifold *rootManifold, phPool<phManifold> *manifoldPool)
		: m_RootManifold(rootManifold)
		, m_ManifoldPool(manifoldPool)
#if !__SPU
		, m_ManifoldInUse(NULL)
#endif	// !__SPU
		, m_NumManifoldsInList(0)
		, m_CompositeManifoldData(Size)
	{
	}

	phManifold *PreLeafCollisionFindManifold(int componentA, int componentB, const phBound *boundA, const phBound *boundB, bool useHighPriorityManifolds)
	{
		PF_FUNC_2(ManifoldManagement0);
		PF_START_MM(ManifoldManagement0a);
		NON_SPU_ONLY(FastAssert(m_ManifoldInUse == NULL));
		const phManifold * RESTRICT rootManifold = m_RootManifold;

		bool wasAllocatedNew = false;

		// Check to see if the root manifold already has a sub-manifold for this component pair.
		phManifold * NON_SPU_ONLY(RESTRICT) manifoldToUse = rootManifold->FindManifoldByComponentIndices(componentA, componentB);

		NON_SPU_ONLY(FastAssert(manifoldToUse == NULL || (manifoldToUse->GetComponentA() == componentA && manifoldToUse->GetComponentB() == componentB)));	// should be guaranteed by the code that found this manifold

#if __SPU
		if(manifoldToUse != NULL)
		{
			m_ManifoldWrapper.InitiateDmaGetStageOne(manifoldToUse, DMA_TAG(1));
		}
#endif	// __SPU

		PF_STOP_MM(ManifoldManagement0a);
		PF_START_MM(ManifoldManagement0b);

		if(manifoldToUse == NULL)
		{
			// We didn't find any appropriate manifold in the pre-existing set.  Let's look through the set of manifolds that are new for this frame.
			atFixedArray<phManifoldData, Size> &manifoldDataPtr = m_CompositeManifoldData;
			const int numNewManifolds = m_NumManifoldsInList;
			const int targetComponentPair = (componentA << 8) + componentB;
			for(int newManifoldIndex = 0; newManifoldIndex < numNewManifolds; ++newManifoldIndex)
			{
				const phManifoldData &curManifoldData = manifoldDataPtr[newManifoldIndex];
				Assert(__SPU || curManifoldData.GetComponentA() == curManifoldData.GetComponentA());
				Assert(__SPU || curManifoldData.GetComponentB() == curManifoldData.GetComponentB());
				// TODO: Should we branch hint unlikely here?
				if(curManifoldData.GetComponentPair() == targetComponentPair)
				{
					manifoldToUse = curManifoldData.GetManifold();
					SPU_ONLY(m_ManifoldWrapper.InitiateDmaGetStageOne(manifoldToUse, DMA_TAG(1)));
					break;
				}
			}
		}

		PF_STOP_MM(ManifoldManagement0b);
		PF_START_MM(ManifoldManagement0c);

		// If we've found one by this point, then we're updating an existing manifold.
		if(manifoldToUse != NULL)
		{
#if __SPU
			manifoldToUse = m_ManifoldWrapper.GetSpuManifold();
			sysDmaWaitTagStatusAll(DMA_MASK(1));
			m_ManifoldWrapper.InitiateDmaGetStageTwo(DMA_TAG(1));
#endif	// __SPU
			FastAssert(!manifoldToUse->CompositeManifoldsEnabled());
			FastAssert(manifoldToUse->GetComponentA() == componentA);	// should be guaranteed by the code that found this manifold
			FastAssert(manifoldToUse->GetComponentB() == componentB);	// should be guaranteed by the code that found this manifold
//			Assert(__SPU || manifoldToUse->GetBoundA() == boundA);
//			Assert(__SPU || manifoldToUse->GetBoundB() == boundB);
		}
		else
		{
			{
				const int numNewCompositeManifolds = m_NumManifoldsInList;
				const int numExistingCompositeManifolds = rootManifold->GetNumCompositeManifolds();
				Assert(numNewCompositeManifolds + numExistingCompositeManifolds <= phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS);
				if(Unlikely(numNewCompositeManifolds + numExistingCompositeManifolds == phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS))
				{
					// We wouldn't have room for this manifold so let's not bother to try and fetch it.
					Warningf("Too many composite component pairs colliding!");
					PF_STOP_MM(ManifoldManagement0c);
					return NULL;
				}
				// Allocate a manifold.  Who knows where it's been before this, so set up the manifold.
				manifoldToUse = m_ManifoldPool->Allocate(useHighPriorityManifolds);
				if(Unlikely(manifoldToUse == NULL))
				{
					// Failed to get a manifold!  Abandon ship!
					PF_STOP_MM(ManifoldManagement0c);
					return NULL;
				}
#if __SPU
				m_ManifoldWrapper.ClearManifold(manifoldToUse);
				manifoldToUse = m_ManifoldWrapper.GetSpuManifold();
#endif	// __SPU

				manifoldToUse->SetInstanceA(rootManifold->GetInstanceA());
				manifoldToUse->SetInstanceB(rootManifold->GetInstanceB());
				manifoldToUse->SetColliderA(rootManifold->GetColliderA());
				manifoldToUse->SetColliderB(rootManifold->GetColliderB());
				manifoldToUse->SetLevelIndexA(rootManifold->GetLevelIndexA(), rootManifold->GetGenerationIdA());
				manifoldToUse->SetLevelIndexB(rootManifold->GetLevelIndexB(), rootManifold->GetGenerationIdB());
			}

			// Regardless of which branch we took above these should be true.
			Assert(!manifoldToUse->CompositeManifoldsEnabled());
			Assert(manifoldToUse->GetNumContacts() == 0);
			wasAllocatedNew = true;

			// This is a fresh manifold so let's initialize it!  We expect that the instance, collider and level index values are correct by this point.
			FastAssert(manifoldToUse->GetInstanceA() == rootManifold->GetInstanceA() && manifoldToUse->GetInstanceB() == rootManifold->GetInstanceB());
			FastAssert(manifoldToUse->GetColliderA() == rootManifold->GetColliderA() && manifoldToUse->GetColliderB() == rootManifold->GetColliderB());
			FastAssert(manifoldToUse->GetLevelIndexA() == rootManifold->GetLevelIndexA() && manifoldToUse->GetLevelIndexB() == rootManifold->GetLevelIndexB());
			manifoldToUse->SetBoundA(boundA);
			manifoldToUse->SetBoundB(boundB);
			manifoldToUse->SetComponentA(componentA);
			manifoldToUse->SetComponentB(componentB);
		}
		FastAssert(manifoldToUse->GetComponentA() == componentA);
		FastAssert(manifoldToUse->GetComponentB() == componentB);

		m_WasAllocatedNew = wasAllocatedNew;
		NON_SPU_ONLY(m_ManifoldInUse = manifoldToUse);

		PF_STOP_MM(ManifoldManagement0c);
		return manifoldToUse;
	}

	void PostLeafCollisionProcessManifold()
	{
		PF_FUNC_2(ManifoldManagement1);
		NON_SPU_ONLY(phManifold *manifoldToUse = m_ManifoldInUse);
		SPU_ONLY(phManifold *manifoldToUse = m_ManifoldWrapper.GetSpuManifold());

		Assert(manifoldToUse->GetNumContacts() > 0);
		Assert(!manifoldToUse->ShouldRelease());
#if __SPU
		// Whether it was a new manifold (one allocated by us) or a pre-existing one, we need to send it back to the PPU.
		// TODO: Maybe we should only do this if there are *new* contact points?  
		m_ManifoldWrapper.InitiateDmaPut(DMA_TAG(2));
#endif

		if(m_WasAllocatedNew)
		{  
			NON_SPU_ONLY(phManifold *manifoldPtrToSave = manifoldToUse);
			SPU_ONLY(phManifold *manifoldPtrToSave = m_ManifoldWrapper.GetPpuManifold());
			// This was a new manifold that had contacts put into it.  Let's add it to the new manifolds list.
			const int numManifoldsInList = m_NumManifoldsInList;
			Assert(numManifoldsInList < Size);
			m_CompositeManifoldData[numManifoldsInList].Set(manifoldPtrToSave, static_cast<u8>(manifoldToUse->GetComponentA()), static_cast<u8>(manifoldToUse->GetComponentB()));
			m_NumManifoldsInList = numManifoldsInList + 1;
		}

#if __SPU
		// Really we only need to wait here if we initiated the DMA above, but it's probably faster to just wait all the time, even if we don't need to.
		sysDmaWaitTagStatusAll(DMA_MASK(2));
#endif	// __SPU

		NON_SPU_ONLY(m_ManifoldInUse = NULL);
	}

	void PostObjectCollisionFinalize()
	{
		PF_FUNC_2(ManifoldManagement2);
		const int numNewCompositeManifolds = m_NumManifoldsInList;
		const atFixedArray<phManifoldData, Size> &compositeManifoldData = m_CompositeManifoldData;
		// Should this be branch-hinted likely?
		if(numNewCompositeManifolds > 0)
		{
			phManifold * RESTRICT rootManifold = m_RootManifold;

#if CHECK_FOR_DUPLICATE_MANIFOLDS
			for (int i = 0 ; i < numNewCompositeManifolds ; i++)
				m_CompositeManifoldData[i].SetTouched(false);
			for (int i = 0 ; i < numNewCompositeManifolds ; i++)
			{
				if (Unlikely(m_CompositeManifoldData[i].GetTouched()))
				{
					const u16 mp0 = m_CompositeManifoldData[i].GetComponentPair();
					int j = 0;
					for ( ; j < i ; j++)
						if (mp0 == m_CompositeManifoldData[j].GetComponentPair())
							break;
					FastAssert(j < i);
					const u16 mp1 = m_CompositeManifoldData[j].GetComponentPair();
					(void)mp1;
					Displayf("Duplicate manifold pairs! %d / %d, %d / %d", i, i, mp0, mp1);
					__debugbreak();
				}
				m_CompositeManifoldData[i].SetTouched(true);
			}
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

			// Sort the list of new manifolds.
			qsort(&m_CompositeManifoldData[0], numNewCompositeManifolds, sizeof(phManifoldData), phManifoldData::CompareFn);

			// Now we have two sorted lists, one in m_CompositeManifoldData and the other in the root manifold, and we want to merge them together into a single,
			//   sorted list in the root manifold.  In order to do this without using any additional memory or requiring any extra memory copies we start from the
			//   end of list in the root manifold and work our way backward.
			const int numExistingCompositeManifolds = rootManifold->GetNumCompositeManifolds();
			const int numFinalManifolds = numNewCompositeManifolds + numExistingCompositeManifolds;
			Assert(numFinalManifolds <= phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS);

			int newManifoldIndex = numNewCompositeManifolds - 1;
			int newComponentPair = compositeManifoldData[newManifoldIndex].GetComponentPair();

			int existingManifoldIndex = numExistingCompositeManifolds - 1;
			int existingComponentPair = numExistingCompositeManifolds != 0 ? (rootManifold->GetCompositePairComponentA(existingManifoldIndex) << 8) + rootManifold->GetCompositePairComponentB(existingManifoldIndex) : -1;

			for(int finalManifoldIndex = numFinalManifolds - 1; finalManifoldIndex >= 0 && finalManifoldIndex != existingManifoldIndex; --finalManifoldIndex)
			{
				phManifold *finalManifold;
				const int finalComponentPair = Max(existingComponentPair, newComponentPair);
				if(existingComponentPair > newComponentPair)
				{
					finalManifold = rootManifold->GetCompositeManifold(existingManifoldIndex);
					existingComponentPair = existingManifoldIndex != 0 ? (rootManifold->GetCompositePairComponentA(existingManifoldIndex - 1) << 8) + rootManifold->GetCompositePairComponentB(existingManifoldIndex - 1) : -1;
					--existingManifoldIndex;
				}
				else
				{
					finalManifold = compositeManifoldData[newManifoldIndex].GetManifold();
					newComponentPair = newManifoldIndex != 0 ? compositeManifoldData[newManifoldIndex - 1].GetComponentPair() : -1;
					--newManifoldIndex;
				}
				u8 finalComponentA = static_cast<u8>(finalComponentPair >> 8);
				u8 finalComponentB = static_cast<u8>(finalComponentPair & 0xFF);
				rootManifold->SetManifoldAndPairComponents(finalManifoldIndex, finalManifold, finalComponentA, finalComponentB);
			}

			// TODO: Add an interface for this.
			rootManifold->m_NumCompositeManifolds = static_cast<u8>(numFinalManifolds);
		}
	}

private:
	phManifold *m_RootManifold;
	phPool<phManifold> *m_ManifoldPool;

	bool m_WasAllocatedNew;

	int m_NumManifoldsInList;
	atFixedArray<phManifoldData, Size> m_CompositeManifoldData;

#if !__SPU
	phManifold *m_ManifoldInUse;
#else	// !__SPU
	phSpuManifoldWrapper m_ManifoldWrapper;
#endif	// !__SPU
};

#if !USE_NEW_MID_PHASE || !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP
void ObjectState::Iterate(phCollisionWorkStack <COLLISION_WORK_STACK_DEPTH> *collisionWorkStack, ObjectState &otherObjectState)
{
	// TODO: This code is doing also the exact same thing as what it done in ObjectState::Pop().  Perhaps it would be possible to avoid that duplication.
	PF_FUNC_2(IterateCulling);
	switch(GetStateType())
	{
	case OBJECTSTATETYPE_COMPONENTS:
		{
			const phBound *curCullingBound = GetCurCullingBound();
			Assert(curCullingBound->GetType() == phBound::COMPOSITE);
			const phBoundComposite *compositeBound = static_cast<const phBoundComposite *>(curCullingBound);

			// Save this off because we're going to change the end index shortly.
			const int numComponents = GetEndIndex();
			FastAssert(numComponents > 0);

#if !MIDPHASE_USES_QUATS
			const Mat34V curBoundToWorldMatrix = GetCurBoundToWorldMatrix();
			const Mat34V lastBoundToWorldMatrix = GetLastBoundToWorldMatrix();
#else	// !MIDPHASE_USES_QUATS
			const QuatV curBoundToWorldOrientation = GetCurBoundToWorldOrientation();
			const Vec3V curBoundToWorldOffset = GetCurBoundToWorldOffset();
			const QuatV lastBoundToWorldOrientation = GetLastBoundToWorldOrientation();
			const Vec3V lastBoundToWorldOffset = GetLastBoundToWorldOffset();
#endif	// !MIDPHASE_USES_QUATS
			const int firstComponentIndex = GetStartIndex();
			FastAssert(compositeBound->GetBound(firstComponentIndex) != NULL);
			FastAssert(GetEndIndex() == compositeBound->GetNumBounds());
			SetStartIndex(firstComponentIndex);		// I don't think this is actually necessary.
			SetEndIndex(firstComponentIndex + 1);	// I don't think this is actually necessary.
#if !MIDPHASE_USES_QUATS
			InitCompositeBoundingBox(compositeBound, curBoundToWorldMatrix, lastBoundToWorldMatrix, firstComponentIndex);
#else	// !MIDPHASE_USES_QUATS
			InitCompositeBoundingBox(compositeBound, curBoundToWorldOrientation, curBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset, firstComponentIndex);
#endif	// !MIDPHASE_USES_QUATS
			// We've changed our 'bound to world' matrix so we need to re-determine the culling box information relative to this new matrix.
			InitCullingBoxInfo(otherObjectState);
			SetStateType(OBJECTSTATETYPE_COMPONENT);

			// Create an object state representing the rest of the composite (starting with the first component that is non-NULL).
			int nextNonNullComponentIndex = firstComponentIndex + 1;
			for(; nextNonNullComponentIndex < numComponents; ++nextNonNullComponentIndex)
			{
				const phBound *curComponent = compositeBound->GetBound(nextNonNullComponentIndex);
				if(Likely(curComponent != NULL))
				{
					ObjectState *objectState0, *objectState1;
					SPU_ONLY(GetSpuCollisionBuffers()->SetMarker());
					collisionWorkStack->Push(objectState0, objectState1);

					// Set up a few relevant members.  We don't need to calculate any bounding or cull boxes because the object state that we're setting up will
					//   only get used to create 'component' object states.
					objectState0->SetStartIndex(nextNonNullComponentIndex);
					objectState0->SetEndIndex(numComponents);
					objectState0->SetCurCullingBound(compositeBound);
					objectState0->SetCollisionBoundIndex(GetCollisionBoundIndex());
					objectState0->SetTypeAndIncludeFlags(GetTypeFlags(), GetIncludeFlags());
					objectState0->SetStateType(OBJECTSTATETYPE_COMPONENTS);
#if !MIDPHASE_USES_QUATS
					objectState0->SetBoundToWorldMatrices(curBoundToWorldMatrix, lastBoundToWorldMatrix);
#else	// !MIDPHASE_USES_QUATS
					objectState0->SetBoundToWorldMatrices(curBoundToWorldOrientation, curBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset);
#endif	// !MIDPHASE_USES_QUATS

					*objectState1 = otherObjectState;
					break;
				}
			}
			break;
		}
	case OBJECTSTATETYPE_SUBTREEHEADERS:
		{
			// Init newObjectState to correspond to the rest of the top-level subtrees (if any are left) and configure ourself to correspond only to the
			//   current subtree (by changing our type to OBJECTSTATETYPE_SUBTREE and setting the indices to bound ourself to only this subtree).
			const int subtreeHeaderStartIndex = GetStartIndex();
			const int subtreeHeaderEndIndex = GetEndIndex();
			// If Iterate() it called with an object state representing a range of subtree headers it had better be representing all of them.
			FastAssert(GetStartIndex() == 0);
			FastAssert(GetEndIndex() == GetBvhStructure()->GetNumUsedSubtreeHeaders());

			const bool haveWorkLeftToPush = (subtreeHeaderStartIndex + 1) < subtreeHeaderEndIndex;
			if(Likely(haveWorkLeftToPush))
			{
				ObjectState *objectState0, *objectState1;
				SPU_ONLY(GetSpuCollisionBuffers()->SetMarker());
				collisionWorkStack->Push(objectState0, objectState1);
				*objectState0 = *this;
				objectState0->SetStartIndex(subtreeHeaderStartIndex + 1);
				objectState0->SetEndIndex(subtreeHeaderEndIndex);
				Assert(objectState0->GetStateType() == OBJECTSTATETYPE_SUBTREEHEADERS);

				*objectState1 = otherObjectState;
			}

			SetEndIndex(1);
			SPU_ONLY(const phOptimizedBvh::phBVHSubtreeInfo &curSubtree = GetSubtreeHeader(subtreeHeaderStartIndex));
			SPU_ONLY(InitDmaBvhNodesFromSubtreeHeader(curSubtree));
			InitSubtreeHeader();

			break;
		}
	case OBJECTSTATETYPE_SUBTREE:
		{
			// Init newObjectState to correspond to the subtree rooted by the right child node (the index of its root will be the escape index of our
			//   left child node) and configure ourself to correspond only to the subtree rooted by the left child node.  Depending on whether our
			//   subtree's root is a leaf or not we may change our type to leaf or do nothing at all.
			const int curTreeStartIndex = GetStartIndex();
			const int curTreeEndIndex = GetEndIndex();

			ASSERT_ONLY(const phOptimizedBvhNode &curTreeRootNode = GetBvhNode(curTreeStartIndex));
			Assert(!curTreeRootNode.IsLeafNode());

			const int leftTreeStartIndex = curTreeStartIndex + 1;
			const phOptimizedBvhNode &leftTreeRootNode = GetBvhNode(leftTreeStartIndex);
			const int rightTreeStartIndex = leftTreeStartIndex + leftTreeRootNode.GetEscapeIndex();
			Assert(rightTreeStartIndex > leftTreeStartIndex);

			// Push the right subtree onto the stack to be processed later.
			ObjectState *objectState0, *objectState1;
			SPU_ONLY(GetSpuCollisionBuffers()->SetMarker());
			collisionWorkStack->Push(objectState0, objectState1);
			*objectState0 = *this;	// TODO: Might there be something cheaper that we can do instead of a wholesale copy like this?
			*objectState1 = otherObjectState;
			objectState0->InitSubtree(rightTreeStartIndex, curTreeEndIndex, otherObjectState);

			// Initialize ourself to correspond to the left subtree.
			InitSubtree(leftTreeStartIndex, rightTreeStartIndex, otherObjectState);
			break;
		}
	default:
		{
			// These are the only other state types.
			FastAssert(GetStateType() == OBJECTSTATETYPE_LEAF || GetStateType() == OBJECTSTATETYPE_SUBTREEHEADER);
			// We should never be asked to iterate something that we've reported is a leaf or a single header.
			FastAssert(false);
			break;
		}
	}
}


__forceinline void ObjectState::Pop(phCollisionWorkStack <COLLISION_WORK_STACK_DEPTH> *collisionWorkStack, ObjectState &otherObjectState)
{
	collisionWorkStack->Pop(*this, otherObjectState);
	// Before we release any blocks of memory we'd better make sure that any DMAs that might be pending into them are completed first.  You might think
	//   this wouldn't be a problem but sometimes we're actually fast enough to initiate a DMA, decide to abort the collision before we needed the data
	//   that we were DMAing, and then re-allocate that block to be used again, all before the DMA has completed.
	SPU_ONLY(sysDmaWaitTagStatusAll(SYS_DMA_MASK_ALL));
	SPU_ONLY(GetSpuCollisionBuffers()->ReleaseToLastMarker());
	switch(GetStateType())
	{
	case OBJECTSTATETYPE_COMPONENTS:
		{
			// We just popped off a range of components.  We don't really want that, so let's peel off the first component and keep that and then push the
			//   rest (if there are any) back onto the stack.
			// SPU TODO: This would probably be an even better place to kick off the DMA for component bound.
			const phBound *curCullingBound = GetCurCullingBound();
			Assert(curCullingBound->GetType() == phBound::COMPOSITE);
			const phBoundComposite *compositeBound = static_cast<const phBoundComposite *>(curCullingBound);
			SPU_ONLY(Assert(compositeBound->GetTypeAndIncludeFlags() == NULL || GetSpuCollisionBuffers()->IsValidPointer(compositeBound->GetTypeAndIncludeFlags())));
			FastAssert(GetEndIndex() == compositeBound->GetNumBounds());
			const int numComponents = GetEndIndex();
			FastAssert(numComponents > 0);
#if !MIDPHASE_USES_QUATS
			const Mat34V curBoundToWorldMatrix = GetCurBoundToWorldMatrix();
			const Mat34V lastBoundToWorldMatrix = GetLastBoundToWorldMatrix();
#else	// !MIDPHASE_USES_QUATS
			const QuatV curBoundToWorldOrientation = GetCurBoundToWorldOrientation();
			const Vec3V curBoundToWorldOffset = GetCurBoundToWorldOffset();
			const QuatV lastBoundToWorldOrientation = GetLastBoundToWorldOrientation();
			const Vec3V lastBoundToWorldOffset = GetLastBoundToWorldOffset();
#endif	// !MIDPHASE_USES_QUATS

			int componentIndex = GetStartIndex();
			SetEndIndex(componentIndex + 1);	// I don't think this is actually necessary.
#if !MIDPHASE_USES_QUATS
			InitCompositeBoundingBox(compositeBound, curBoundToWorldMatrix, lastBoundToWorldMatrix, componentIndex);
#else	// !MIDPHASE_USES_QUATS
			InitCompositeBoundingBox(compositeBound, curBoundToWorldOrientation, curBoundToWorldOffset, lastBoundToWorldOrientation, lastBoundToWorldOffset, componentIndex);
#endif	// !MIDPHASE_USES_QUATS
			// We've changed our 'bound to world' matrix so we need to re-determine the culling box information relative to this new matrix.
			InitCullingBoxInfo(otherObjectState);
			SetStateType(OBJECTSTATETYPE_COMPONENT);

			for(++componentIndex; componentIndex < numComponents; ++componentIndex)
			{
				const phBound *curComponent = compositeBound->GetBound(componentIndex);
				if(Likely(curComponent != NULL))
				{
					SPU_ONLY(GetSpuCollisionBuffers()->SetMarker());
					ObjectState *objectState0, *objectState1;
					collisionWorkStack->Push(objectState0, objectState1);

					// Set up a few relevant members.  We don't need to calculate any bounding or cull boxes because the object state that we're setting up will
					//   only get used to create 'component' object states.
					objectState0->SetStartIndex(componentIndex);
					Assert(objectState0->GetEndIndex() == numComponents);
					Assert(objectState0->GetCurCullingBound() == compositeBound);
					Assert(objectState0->GetCollisionBoundIndex() == GetCollisionBoundIndex());
					Assert(objectState0->GetTypeFlags() == GetTypeFlags());
					Assert(objectState0->GetIncludeFlags() == GetIncludeFlags());
					Assert(objectState0->GetStateType() == OBJECTSTATETYPE_COMPONENTS);

					// objectState1 should already be set up properly.
					break;
				}
			}

			break;
		}
	case OBJECTSTATETYPE_SUBTREEHEADERS:
		{
			// We just popped off a range of subtree headers.  We don't really want that, so let's peel off the first header and keep that and then push the
			//   rest (if there are any) back onto the stack.
			const int subtreeHeaderStartIndex = GetStartIndex();
			const int subtreeHeaderEndIndex = GetEndIndex();
			FastAssert(subtreeHeaderStartIndex > 0);
			FastAssert(subtreeHeaderEndIndex == GetBvhStructure()->GetNumUsedSubtreeHeaders());

			const bool haveWorkLeftToPush = (subtreeHeaderStartIndex + 1) < subtreeHeaderEndIndex;
			if(Likely(haveWorkLeftToPush))
			{
				SPU_ONLY(GetSpuCollisionBuffers()->SetMarker());
				ObjectState *objectState0, *objectState1;
				collisionWorkStack->Push(objectState0, objectState1);

				// Set up a few relevant members.  We don't need to calculate any bounding or cull boxes because the object state that we're setting up will
				//   only be used to create 'subtree header' object states.
				objectState0->SetStartIndex(subtreeHeaderStartIndex + 1);
				Assert(objectState0->GetEndIndex() == subtreeHeaderEndIndex);
				Assert(objectState0->GetStateType() == OBJECTSTATETYPE_SUBTREEHEADERS);

				// objectState1 should already be set up properly.
			}

			SetEndIndex(subtreeHeaderStartIndex + 1);
			SPU_ONLY(const phOptimizedBvh::phBVHSubtreeInfo &curSubtree = GetSubtreeHeader(subtreeHeaderStartIndex));
			SPU_ONLY(InitDmaBvhNodesFromSubtreeHeader(curSubtree));
			InitSubtreeHeader();

			break;
		}
	default:
		{
			// Nothing special to do for these cases right now.
			break;
		}
	}
}
#endif // !USE_NEW_MID_PHASE || !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP

__forceinline u32 GetPointOnEdgeFlags(Vec3V_In point0, Vec3V_In point1, Vec3V_In point2, Vec3V_In testPoint, ScalarV_In squaredMargin)
{
#if 1
	const Vec3V edge01 = Subtract(point1, point0);
	const Vec3V edge12 = Subtract(point2, point1);
	const Vec3V edge20 = Subtract(point0, point2);

	const Vec3V point0ToTestPoint = Subtract(testPoint, point0);
	const Vec3V point1ToTestPoint = Subtract(testPoint, point1);
	const Vec3V point2ToTestPoint = Subtract(testPoint, point2);

	const ScalarV fractionAlongEdge0 = Scale(Dot(edge01, point0ToTestPoint),InvMagSquared(edge01));
	const ScalarV fractionAlongEdge1 = Scale(Dot(edge12, point1ToTestPoint),InvMagSquared(edge12));
	const ScalarV fractionAlongEdge2 = Scale(Dot(edge20, point2ToTestPoint),InvMagSquared(edge20));

	const Vec3V point0Projected = AddScaled(point0,edge01,fractionAlongEdge0);
	const Vec3V point1Projected = AddScaled(point1,edge12,fractionAlongEdge1);
	const Vec3V point2Projected = AddScaled(point2,edge20,fractionAlongEdge2);

	const Vec3V distSquaredToEdge = Vec3V(MagSquared(Subtract(testPoint, point0Projected)),
										  MagSquared(Subtract(testPoint, point1Projected)),
										  MagSquared(Subtract(testPoint, point2Projected)));

	const VecBoolV result = IsLessThanOrEqual(distSquaredToEdge, Vec3V(squaredMargin));
	u32 resultFlags;
	ResultToIndexZYX(resultFlags, result);
	return resultFlags;
#else
	// This version is probably a bit faster, but has proven to have a lot of error near zero distances...which are
	// exactly the distances we're concerned with here. Basically, we're trying to get the length of the adjacent side
	// when the hypotenuse and opposite side are very nearly the same length. See B*187058
	const Vec3V edgeDir01 = Normalize(Subtract(point1, point0));
	const Vec3V edgeDir12 = Normalize(Subtract(point2, point1));
	const Vec3V edgeDir20 = Normalize(Subtract(point0, point2));

	const Vec3V point0ToTestPoint = Subtract(point0, testPoint);
	const Vec3V point1ToTestPoint = Subtract(point1, testPoint);
	const Vec3V point2ToTestPoint = Subtract(point2, testPoint);

	const Vec3V v3DistSquaredToPoints = Vec3V(MagSquared(point0ToTestPoint), MagSquared(point1ToTestPoint), MagSquared(point2ToTestPoint));
	const Vec3V v3DistAlongEdge = Vec3V(Dot(point0ToTestPoint, edgeDir01), Dot(point1ToTestPoint, edgeDir12), Dot(point2ToTestPoint, edgeDir20));
	const Vec3V v3DistSquaredAlongEdge = Scale(v3DistAlongEdge, v3DistAlongEdge);
	const Vec3V v3DistSquaredToEdge = Subtract(v3DistSquaredToPoints, v3DistSquaredAlongEdge);

	const VecBoolV vbResult = IsLessThanOrEqual(v3DistSquaredToEdge, Vec3V(squaredMargin));
	u32 resultFlags;
	ResultToIndexZYX(resultFlags, vbResult);
	return resultFlags;
#endif
}

// PRESERVE_SAMPLE_CONTACTS_SLIDING_BEHAVIOR -- The current code breaks the sample_contacts sliding page
//  - We use a broader tolerance in the full game in order to reduce missed contacts (And therefore complete fall-throughs for small/fast objects)
// Sample is stable with: kVecKindOfSmallFloat = ScalarV(V_FLT_SMALL_6); // ~ 0.000057 degrees
static ScalarV kVecKindOfSmallFloat = ScalarV(V_FLT_SMALL_2); // This comes out to about 0.57 degrees
__forceinline int phPairwiseCollisionProcessor::CheckContactAgainstEdge(Vec3V_In localizedVertex,
							 Vec3V_In localizedVertexNext,
							 Vec3V_In neighborNormal,
							 Vec3V_In faceNormal,
							 Vec3V_In curContactNormal)
{
	// The triangle has a neighboring triangle connected to this edge, so see if this collision should instead go on the neighboring triangle.
	// It may not quite be obvious what's going on here so let me explain.
	// Basically, we're trying to determine whether the collision normal is pointed 'away' from the polygon normal more than the edge normal is.
	// If that is the case, then we'll reject the collision because it's happening on an edge in a way that actually means that it shouldn't be
	//   happening because it would be protected by the neighboring polygon.
	// I could also dot the edge normal with the face normal - would that be better/faster?  (it might already be in a register)
	const ScalarV kEdgeDotNeighbor = Dot(faceNormal, neighborNormal);

	// Ensure that there's at least a little bit of slop allowed.  This is redundant with the tolerance in the comparison below and they
	//   should really be combined into one value.
	ScalarV kMinDot = kEdgeDotNeighbor;

	const Vec3V curEdge = Subtract(localizedVertexNext, localizedVertex);
	const ScalarV contactNormalProjectionAlongEdge = Scale(Dot(curContactNormal, curEdge),InvMagSquared(curEdge));
	Vec3V curContactNormalPerpToEdge = curContactNormal;

	// Generally speaking, contactNormalProjectionAlongEdge is going to be zero because the contact normal is going to be perpendicular
	//   to the edge, the exception being contacts that are at vertices. And yet there always seems to be a little to shave off, so we just do this all the time.
	curContactNormalPerpToEdge = SubtractScaled(curContactNormalPerpToEdge, curEdge, contactNormalProjectionAlongEdge);
	curContactNormalPerpToEdge = Normalize(curContactNormalPerpToEdge);

	// TRUE(non-zero value) means the collision normal is leaning far enough away from this polygon's normal that the collision should be with the neighboring polygon.
	int result = IsLessThanAll(Dot(curContactNormalPerpToEdge, faceNormal), kMinDot-kVecKindOfSmallFloat);
	return result;
}

// A 2% tolerance what was used for this check in the old midphase, but it might be more generous than necessary.  I tried a 1% tolerance that seemed to catch all
//   of the cases in the test case I was trying (friction page in sample_contacts).
static ScalarV s_PointOnEdgeToleranceFactorSquared(ScalarVFromF32(1.02f * 1.02f));

#if USE_CHECK_TRIANGLE
void CheckTriangle(Vec3V_In faceNormal, Vec3V_In vertices0, Vec3V_In edgeNormals0, Vec3V_In edgeNormals1, Vec3V_In edgeNormals2, Mat34V_In current, const phPolygon& triPrim, const phArchetype* triParentArch, int partIndex)
{
	// Meant to catch polys with bad neighbors - Unless some new bug has been introduced higher in the callstack, this should always point to an error in the art setup
	// The tolerance allowed is very large because that's pretty much what it takes to cause TriPenDepthSolver to start failing (V_TWO_OVER_PI because I was hoping to invite a friend over for PI)
	bool edgeNormIsOk0 = IsLessThanOrEqualAll( Abs( Subtract( Mag(edgeNormals0), ScalarV(V_ONE)) ), ScalarV(V_TWO_OVER_PI) ) != 0;
	bool edgeNormIsOk1 = IsLessThanOrEqualAll( Abs( Subtract( Mag(edgeNormals1), ScalarV(V_ONE)) ), ScalarV(V_TWO_OVER_PI) ) != 0;
	bool edgeNormIsOk2 = IsLessThanOrEqualAll( Abs( Subtract( Mag(edgeNormals2), ScalarV(V_ONE)) ), ScalarV(V_TWO_OVER_PI) ) != 0;
	if((!edgeNormIsOk0) || (!edgeNormIsOk1) || (!edgeNormIsOk2))
	{
		Vec3V triPrimPos = Transform(current, vertices0);
#if __SPU
		char archetypeNameBuffer[256] = {'u', 'n', 'i', 'n', 'i', 't', 0};
		const char* triParentName = archetypeNameBuffer;
		u8 archetypeBuffer[sizeof(phArchetype)] ;
		if(triParentArch != NULL)
		{
			const phArchetype *spuArchetype = reinterpret_cast<const phArchetype *>(&archetypeBuffer[0]);
			sysDmaLargeGet(spuArchetype, (uint64_t)triParentArch, sizeof(phArchetype), DMA_TAG(13));
			sysDmaWaitTagStatusAll(DMA_MASK(13));
			const char* ppuFileName = spuArchetype->GetFilename();
			if(ppuFileName)
			{
				u32 alignmentOffset = (u32)ppuFileName & 0xF;
				const char* alignedPpuFileName = ppuFileName - alignmentOffset;
				sysDmaGetAndWait(archetypeNameBuffer, (uint64_t)alignedPpuFileName, 256, DMA_TAG(13));
				triParentName += alignmentOffset;
			}
		}
#else // __SPU
		const char* triParentName = "No name!";
		if(triParentArch != NULL)
		{
			triParentName = triParentArch->GetFilename();
		}
#endif // __SPU

		Assertf(edgeNormIsOk0, "Bad Tri edge normal0: <%f %f %f> - '%s' -- Prim:%d - Pos<< %f, %f, %f >> - TriArea:%f - TriNorm<< %f, %f, %f >>", Vec3V(edgeNormals0).GetXf(), Vec3V(edgeNormals0).GetYf(), Vec3V(edgeNormals0).GetZf(), triParentName, partIndex, triPrimPos.GetXf(), triPrimPos.GetYf(), triPrimPos.GetZf(), triPrim.GetArea(), faceNormal.GetXf(), faceNormal.GetYf(), faceNormal.GetZf());
		Assertf(edgeNormIsOk1, "Bad Tri edge normal1: <%f %f %f> - '%s' -- Prim:%d - Pos<< %f, %f, %f >> - TriArea:%f - TriNorm<< %f, %f, %f >>", Vec3V(edgeNormals1).GetXf(), Vec3V(edgeNormals1).GetYf(), Vec3V(edgeNormals1).GetZf(), triParentName, partIndex, triPrimPos.GetXf(), triPrimPos.GetYf(), triPrimPos.GetZf(), triPrim.GetArea(), faceNormal.GetXf(), faceNormal.GetYf(), faceNormal.GetZf());
		Assertf(edgeNormIsOk2, "Bad Tri edge normal2: <%f %f %f> - '%s' -- Prim:%d - Pos<< %f, %f, %f >> - TriArea:%f - triNorm<< %f, %f, %f >>", Vec3V(edgeNormals2).GetXf(), Vec3V(edgeNormals2).GetYf(), Vec3V(edgeNormals2).GetZf(), triParentName, partIndex, triPrimPos.GetXf(), triPrimPos.GetYf(), triPrimPos.GetZf(), triPrim.GetArea(), faceNormal.GetXf(), faceNormal.GetYf(), faceNormal.GetZf());
	}
}
#endif // USE_CHECK_TRIANGLE

NON_SPU_ONLY(ASSERT_ONLY(extern bool g_UseBackfaceCulling;))
NON_SPU_ONLY(ASSERT_ONLY(extern bool g_UseNormalFiltering;))
NON_SPU_ONLY(ASSERT_ONLY(extern bool g_UseNormalFilteringAlways;))
#define ENABLE_NORMAL_FILTERING_WIDGET 0
NON_SPU_ONLY(ASSERT_ONLY(extern bool g_UseNormalClamping;))

void phPairwiseCollisionProcessor::ProcessPairwiseCollision(DiscreteCollisionDetectorInterface::SimpleResult &result, const NewCollisionInput &collisionInput)
{
	PF_FUNC_2(ProcessPairwiseCollision);
	PPC_STAT_TIMER_SCOPED(ProcessPairwiseCollisionTimer);
	PPC_STAT_COUNTER_INC(ProcessPairwiseCollisionCounter,1);

#if __DEV && !__SPU
	ALIGNAS(16) u8 boundBuffer0[phBound::MAX_BOUND_SIZE];
	ALIGNAS(16) u8 boundBuffer1[phBound::MAX_BOUND_SIZE];
	memcpy(boundBuffer0,collisionInput.object0,phBound::MAX_BOUND_SIZE);
	memcpy(boundBuffer1,collisionInput.object0,phBound::MAX_BOUND_SIZE);
#endif 

#if !DISABLE_SPECIAL_CASE_COLLISION
	// Grab the relevant information from the collision input.
	const phBound *boundA = collisionInput.object0->m_bound;
	const phBound *boundB = collisionInput.object1->m_bound;

	const bool useSpecialCaseCollision = UseSpecialCaseCollision(boundA,boundB);

	if (useSpecialCaseCollision)
		ContinuousConvexCollision::ComputeTimeOfImpact(collisionInput, result);
	else
#endif // !DISABLE_SPECIAL_CASE_COLLISION
		ContinuousConvexCollision::ComputeTimeOfImpactGJK(&collisionInput, &result);

#if USE_PHYSICS_PROFILE_CAPTURE
	if (result.GetHasResult())
		PPC_STAT_COUNTER_INC(ContactPointCounter,1);
#endif
}


#if !defined(__SPURS_JOB__)

__forceinline Vec3V_Out ComputeNeighborNormal (const rage::phPolygon& neighborPoly, int polyVertIndexA,
											 int polyVertIndexB, const rage::phBoundBVH * RESTRICT spuBVHBound, Vec3V_In polyVertA, Vec3V_In polyVertB)

{
	int neighborsOtherVertexIndex = neighborPoly.GetOtherVertexIndex((phPolygon::Index)(polyVertIndexA), (phPolygon::Index)(polyVertIndexB));
#if !__SPU
	const Vec3V neighborsOtherVertex = spuBVHBound->GetVertex(neighborsOtherVertexIndex);
#else
#if COMPRESSED_VERTEX_METHOD == 0
	small_cache_read(&neighborsOtherVertex,(uint64_t)(&spuBVHBound->GetVertexPointer()[neighborsOtherVertexIndex]),sizeof(rage::Vector3));
#else
	const CompressedVertexType *compressedVertexPointer = &spuBVHBound->GetCompressedVertexPointer()[3 * neighborsOtherVertexIndex];
	// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the address.
	const CompressedVertexType *ppuDMAAddress = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer) & ~15);
	u8 vertexBuffer[32];
	cellDmaLargeGet(vertexBuffer, (uint64_t)(ppuDMAAddress), 32, DMA_TAG(16), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(16));
	// TODO: Perhaps use small_cache_read functions here?
	//small_cache_read(&vertexBuffer[0], (uint64_t)(ppuDMAAddress), 16);
	//small_cache_read(&vertexBuffer[16], (uint64_t)(ppuDMAAddress) + 16, 16);

	const int bufferOffset = (int)(compressedVertexPointer) & 15;
	const CompressedVertexType *spuCompressedVector = reinterpret_cast<const CompressedVertexType *>(&vertexBuffer[bufferOffset]);

	const Vec3V neighborsOtherVertex = spuBVHBound->DecompressVertex(spuCompressedVector);
#endif	// COMPRESSED_VERTEX_METHOD == 0
#endif	// !__SPU
	return neighborPoly.ComputeUnitNormal(polyVertB, polyVertA, neighborsOtherVertex);
}


#if !__SPU
// When not on SPU, the user passes in references to pointers which we fill in and set to polygons that we have prefetched.
__forceinline int PrefetchNeighborsAndComputeNeighborFlags(const phPolygon *&neighbor0, const phPolygon *&neighbor1, const phPolygon *&neighbor2, const phPolygon &curPolygon, const s32 curPolygonIndex, const phBoundBVH * RESTRICT bvhBound)
#else	// !__SPU
// When on SPU, the user passes in destination buffers for us to put the neighbor polygons.
__forceinline int PrefetchNeighborsAndComputeNeighborFlags(phPolygon *neighbor0, phPolygon *neighbor1, phPolygon *neighbor2, const phPolygon &curPolygon, const s32 curPolygonIndex, const phBoundBVH * RESTRICT bvhBound)
#endif	// !__SPU
{
	// TODO: It might be possible to get this masking/selecting done faster on the SPU.
	const s32 realNeighborPolygonIndex0 = curPolygon.GetNeighboringPolyNum(0);
	const u32 hasNeighborMask0 = GenerateMaskNE(realNeighborPolygonIndex0, (s32)((phPolygon::Index)(-1)));
	const s32 neighborPolygonIndexToUse0 = ISelectI(hasNeighborMask0, curPolygonIndex, realNeighborPolygonIndex0);
	const phPolygon *neighborToUse0 = &bvhBound->GetPolygon(neighborPolygonIndexToUse0);
#if !__SPU
	PrefetchObject(neighborToUse0);
	neighbor0 = neighborToUse0;
#else	// !__SPU
	sysDmaWaitTagStatusAll(DMA_MASK(16));
	sysDmaLargeGet(neighbor0, (uint64_t)neighborToUse0, sizeof(phPolygon), DMA_TAG(16));
#endif	// !__SPU

	const s32 realNeighborPolygonIndex1 = curPolygon.GetNeighboringPolyNum(1);
	const u32 hasNeighborMask1 = GenerateMaskNE(realNeighborPolygonIndex1, (s32)((phPolygon::Index)(-1)));
	const s32 neighborPolygonIndexToUse1 = ISelectI(hasNeighborMask1, curPolygonIndex, realNeighborPolygonIndex1);
	const phPolygon *neighborToUse1 = &bvhBound->GetPolygon(neighborPolygonIndexToUse1);
#if !__SPU
	PrefetchObject(neighborToUse1);
	neighbor1 = neighborToUse1;
#else	// !__SPU
	sysDmaWaitTagStatusAll(DMA_MASK(17));
	sysDmaLargeGet(neighbor1, (uint64_t)neighborToUse1, sizeof(phPolygon), DMA_TAG(17));
#endif	// !__SPU

	const s32 realNeighborPolygonIndex2 = curPolygon.GetNeighboringPolyNum(2);
	const u32 hasNeighborMask2 = GenerateMaskNE(realNeighborPolygonIndex2, (s32)((phPolygon::Index)(-1)));
	const s32 neighborPolygonIndexToUse2 = ISelectI(hasNeighborMask2, curPolygonIndex, realNeighborPolygonIndex2);
	const phPolygon *neighborToUse2 = &bvhBound->GetPolygon(neighborPolygonIndexToUse2);
#if !__SPU
	PrefetchObject(neighborToUse2);
	neighbor2 = neighborToUse2;
#else	// !__SPU
	sysDmaWaitTagStatusAll(DMA_MASK(18));
	sysDmaLargeGet(neighbor2, (uint64_t)neighborToUse2, sizeof(phPolygon), DMA_TAG(18));
#endif	// !__SPU

	return (hasNeighborMask0 & 1) | (hasNeighborMask1 & 2) | (hasNeighborMask2 & 4);
}


// Call this function *only* if you have computed the 'true' normals of the neighbor polygons.
// Use vector selection to set edge and neighbor normals based on whether they're connected via a convex edge or not.
__forceinline void ComputeEdgeAndNeighborNormals(Vec3V_InOut edgeNormal0, Vec3V_InOut edgeNormal1, Vec3V_InOut edgeNormal2, Vec3V_InOut neighborNormal0, Vec3V_InOut neighborNormal1, Vec3V_InOut neighborNormal2, Vec3V_In curPolyUnitNormal, Vec3V_In polyVert0, Vec3V_In polyVert1, Vec3V_In polyVert2)
{
	// First determine if the neighbor forms a convex edge with this poly (Note that this is dependent on the winding order of our polys)
	const Vec3V edge0 = Subtract(polyVert1, polyVert0);
	const Vec3V neighborNormal0CrossEdge0 = Cross(neighborNormal0, edge0);
	const BoolV convexEdgeSelector0 = IsGreaterThan(Dot(neighborNormal0CrossEdge0, curPolyUnitNormal), ScalarV(V_ZERO));
	// Then determine if the two normals are safe to average (i.e. Not almost exactly opposing)
	const Vec3V normalSum0 = Add(curPolyUnitNormal, neighborNormal0);
	const ScalarV sumMag0 = Mag(normalSum0);
	const BoolV sumMagIsValid0 = IsGreaterThanOrEqual(sumMag0, ScalarV(V_FLT_SMALL_6));
	// If the neighbor forms a concave edge or has a normal almost exactly opposite ours then we set the edge normal to our own face normal, otherwise it is the average of the two face normals
	// Note that the edge normal as a function is discontinuous here and should mathematically become the cross between our face normal and the edge direction
	//  However, we've decided that such a double sided setup is an incorrect art asset and so this is purely to be tolerant to such a setup and avoid the NAN that would otherwise result
	edgeNormal0 = SelectFT(And(convexEdgeSelector0, sumMagIsValid0), curPolyUnitNormal, InvScale(normalSum0, sumMag0));
	FatalAssert(IsFiniteAll(edgeNormal0));
	// We pretend the neighbor's normal is the same as our own for concave edges
	neighborNormal0 = SelectFT(convexEdgeSelector0, curPolyUnitNormal, neighborNormal0);

	// Do it again for the other two edges:
	const Vec3V edge1 = Subtract(polyVert2, polyVert1);
	const Vec3V neighborNormal1CrossEdge1 = Cross(neighborNormal1, edge1);
	const BoolV convexEdgeSelector1 = IsGreaterThan(Dot(neighborNormal1CrossEdge1, curPolyUnitNormal), ScalarV(V_ZERO));
	const Vec3V normalSum1 = Add(curPolyUnitNormal, neighborNormal1);
	const ScalarV sumMag1 = Mag(normalSum1);
	const BoolV sumMagIsValid1 = IsGreaterThanOrEqual(sumMag1, ScalarV(V_FLT_SMALL_6));
	edgeNormal1 = SelectFT(And(convexEdgeSelector1, sumMagIsValid1), curPolyUnitNormal, InvScale(normalSum1, sumMag1));
	FatalAssert(IsFiniteAll(edgeNormal1));
	neighborNormal1 = SelectFT(convexEdgeSelector1, curPolyUnitNormal, neighborNormal1);

	//
	const Vec3V edge2 = Subtract(polyVert0, polyVert2);
	const Vec3V neighborNormal2CrossEdge2 = Cross(neighborNormal2, edge2);
	const BoolV convexEdgeSelector2 = IsGreaterThan(Dot(neighborNormal2CrossEdge2, curPolyUnitNormal), ScalarV(V_ZERO));
	const Vec3V normalSum2 = Add(curPolyUnitNormal, neighborNormal2);
	const ScalarV sumMag2 = Mag(normalSum2);
	const BoolV sumMagIsValid2 = IsGreaterThanOrEqual(sumMag2, ScalarV(V_FLT_SMALL_6));
	edgeNormal2 = SelectFT(And(convexEdgeSelector2, sumMagIsValid2), curPolyUnitNormal, InvScale(normalSum2, sumMag2));
	FatalAssert(IsFiniteAll(edgeNormal2));
	neighborNormal2 = SelectFT(convexEdgeSelector2, curPolyUnitNormal, neighborNormal2);
}


// Call this function if you have not already computed the 'true' normals of the neighbor polygons.
/*__forceinline */void ComputeEdgeAndNeighborNormals(
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	Vec3V_InOut edgeNormal0, Vec3V_InOut edgeNormal1, Vec3V_InOut edgeNormal2, 
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	Vec3V_InOut neighborNormal0, Vec3V_InOut neighborNormal1, Vec3V_InOut neighborNormal2, Vec3V_In curPolyUnitNormal, Vec3V_In polyVert0, Vec3V_In polyVert1, Vec3V_In polyVert2, const phPolygon &neighborPoly0, const phPolygon &neighborPoly1, const phPolygon &neighborPoly2, u32 polyVertIndex0, u32 polyVertIndex1, u32 polyVertIndex2, const phBoundBVH *bvhBound, const int hasNeighbor)
{
	FastAssert(bvhBound);
	if( !PARAM_disableFix7459244.Get() && !bvhBound)
		return;

	const VecBoolV hasNeighborSelectors[8] = {VecBoolV(V_F_F_F_F), VecBoolV(V_T_F_F_F), VecBoolV(V_F_T_F_F), VecBoolV(V_T_T_F_F), 
		VecBoolV(V_F_F_T_F), VecBoolV(V_T_F_T_F), VecBoolV(V_F_T_T_F), VecBoolV(V_T_T_T_F), };
	const VecBoolV hasNeighborV = hasNeighborSelectors[hasNeighbor];

	SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(16)));	// Ensure that the polygon has arrived.
	const u32 otherNeighborVertIndex0 = neighborPoly0.GetOtherVertexIndex(polyVertIndex0, polyVertIndex1);
#if __SPU
	u8 otherNeighborVert0Buffer[32] ;
#if COMPRESSED_VERTEX_METHOD == 0
	sysDmaLargeGet(otherNeighborVert0Buffer, (uint64_t)(bvhBound->GetVertexPointer()[otherNeighborVertIndex0]), sizeof(Vec3V));
#else	// COMPRESSED_VERTEX_METHOD == 0
	const CompressedVertexType *compressedVertexPointer0 = &bvhBound->GetCompressedVertexPointer()[3 * otherNeighborVertIndex0];
	// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the address.
	const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer0) & ~15);
	sysDmaLargeGet(otherNeighborVert0Buffer, (uint64_t)ppuDMAAddress0, 32, DMA_TAG(16));
#endif	// COMPRESSED_VERTEX_METHOD == 0
#else	// __SPU
	const Vec3V otherNeighborVert0 = bvhBound->GetVertex(otherNeighborVertIndex0);
	neighborNormal0 = SelectFT(hasNeighborV.GetX(), curPolyUnitNormal, neighborPoly0.ComputeUnitNormal(polyVert1, polyVert0, otherNeighborVert0));
#endif	// __SPU

	SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(17)));	// Ensure that the polygon has arrived.
	const u32 otherNeighborVertIndex1 = neighborPoly1.GetOtherVertexIndex(polyVertIndex1, polyVertIndex2);
#if __SPU
	u8 otherNeighborVert1Buffer[32] ;
#if COMPRESSED_VERTEX_METHOD == 0
	sysDmaLargeGet(otherNeighborVert1Buffer, (uint64_t)(bvhBound->GetVertexPointer()[otherNeighborVertIndex1]), sizeof(Vec3V));
#else	// COMPRESSED_VERTEX_METHOD == 0
	const CompressedVertexType *compressedVertexPointer1 = &bvhBound->GetCompressedVertexPointer()[3 * otherNeighborVertIndex1];
	// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the address.
	const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer1) & ~15);
	sysDmaLargeGet(otherNeighborVert1Buffer, (uint64_t)ppuDMAAddress1, 32, DMA_TAG(17));
#endif	// COMPRESSED_VERTEX_METHOD == 0
#else	// __SPU
	const Vec3V otherNeighborVert1 = bvhBound->GetVertex(otherNeighborVertIndex1);
	neighborNormal1 = SelectFT(hasNeighborV.GetY(), curPolyUnitNormal, neighborPoly1.ComputeUnitNormal(polyVert2, polyVert1, otherNeighborVert1));
#endif	// __SPU

	SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(18)));	// Ensure that the polygon has arrived.
	const u32 otherNeighborVertIndex2 = neighborPoly2.GetOtherVertexIndex(polyVertIndex2, polyVertIndex0);
#if __SPU
	u8 otherNeighborVert2Buffer[32] ;
#if COMPRESSED_VERTEX_METHOD == 0
	sysDmaLargeGet(otherNeighborVert2Buffer, (uint64_t)(bvhBound->GetVertexPointer()[otherNeighborVertIndex2]), sizeof(Vec3V));
#else	// COMPRESSED_VERTEX_METHOD == 0
	const CompressedVertexType *compressedVertexPointer2 = &bvhBound->GetCompressedVertexPointer()[3 * otherNeighborVertIndex2];
	// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the address.
	const CompressedVertexType *ppuDMAAddress2 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertexPointer2) & ~15);
	sysDmaLargeGet(otherNeighborVert2Buffer, (uint64_t)ppuDMAAddress2, 32, DMA_TAG(18));
#endif	// COMPRESSED_VERTEX_METHOD == 0
#else	// __SPU
	const Vec3V otherNeighborVert2 = bvhBound->GetVertex(otherNeighborVertIndex2);
	neighborNormal2 = SelectFT(hasNeighborV.GetZ(), curPolyUnitNormal, neighborPoly2.ComputeUnitNormal(polyVert0, polyVert2, otherNeighborVert2));
#endif	// __SPU

#if __SPU
	// Now let's wait for the DMAs to complete and decompress the vertices.
#if COMPRESSED_VERTEX_METHOD == 0
	sysDmaWaitTagStatusAll(DMA_MASK(16));
	const Vec3V otherNeighborVert0 = reinterpret_cast<const Vec3V*>(otherNeighborVert0Buffer);
	neighborNormal0 = SelectFT(hasNeighborV.GetX(), curPolyUnitNormal, neighborPoly0.ComputeUnitNormal(polyVert1, polyVert0, otherNeighborVert0));

	sysDmaWaitTagStatusAll(DMA_MASK(17));
	const Vec3V otherNeighborVert1 = reinterpret_cast<const Vec3V*>(otherNeighborVert1Buffer);
	neighborNormal1 = SelectFT(hasNeighborV.GetY(), curPolyUnitNormal, neighborPoly1.ComputeUnitNormal(polyVert2, polyVert1, otherNeighborVert1));

	sysDmaWaitTagStatusAll(DMA_MASK(18));
	const Vec3V otherNeighborVert2 = reinterpret_cast<const Vec3V*>(otherNeighborVert2Buffer);
	neighborNormal2 = SelectFT(hasNeighborV.GetZ(), curPolyUnitNormal, neighborPoly2.ComputeUnitNormal(polyVert0, polyVert2, otherNeighborVert2));
#else	// COMPRESSED_VERTEX_METHOD == 0
	const int bufferOffset0 = (int)(compressedVertexPointer0) & 15;
	const int bufferOffset1 = (int)(compressedVertexPointer1) & 15;
	const int bufferOffset2 = (int)(compressedVertexPointer2) & 15;

	const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&otherNeighborVert0Buffer[bufferOffset0]);
	sysDmaWaitTagStatusAll(DMA_MASK(16));
	const Vec3V otherNeighborVert0 = bvhBound->DecompressVertex(spuCompressedVector0);
	neighborNormal0 = SelectFT(hasNeighborV.GetX(), curPolyUnitNormal, neighborPoly0.ComputeUnitNormal(polyVert1, polyVert0, otherNeighborVert0));

	const CompressedVertexType *spuCompressedVector1 = reinterpret_cast<const CompressedVertexType *>(&otherNeighborVert1Buffer[bufferOffset1]);
	sysDmaWaitTagStatusAll(DMA_MASK(17));
	const Vec3V otherNeighborVert1 = bvhBound->DecompressVertex(spuCompressedVector1);
	neighborNormal1 = SelectFT(hasNeighborV.GetY(), curPolyUnitNormal, neighborPoly1.ComputeUnitNormal(polyVert2, polyVert1, otherNeighborVert1));

	const CompressedVertexType *spuCompressedVector2 = reinterpret_cast<const CompressedVertexType *>(&otherNeighborVert2Buffer[bufferOffset2]);
	sysDmaWaitTagStatusAll(DMA_MASK(18));
	const Vec3V otherNeighborVert2 = bvhBound->DecompressVertex(spuCompressedVector2);
	neighborNormal2 = SelectFT(hasNeighborV.GetZ(), curPolyUnitNormal, neighborPoly2.ComputeUnitNormal(polyVert0, polyVert2, otherNeighborVert2));
#endif	// COMPRESSED_VERTEX_METHOD == 0
#endif	// __SPU

#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	ComputeEdgeAndNeighborNormals(edgeNormal0, edgeNormal1, edgeNormal2, neighborNormal0, neighborNormal1, neighborNormal2, curPolyUnitNormal, polyVert0, polyVert1, polyVert2);
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
}


// Utility functions for random access fetching of vertices.
__forceinline void FetchVertices1(const phBoundBVH *bvhBound, int vertIndex0, Vec3V_InOut v0)
{
	PF_FUNC_2(GetCompressedVertex);
#if !__SPU
	v0 = bvhBound->GetBVHVertex(vertIndex0);
#else	// !__SPU
#if COMPRESSED_VERTEX_METHOD == 0
	sysDmaLargeGet(&v0, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex0], sizeof(Vec3V), DMA_TAG(1));
	sysDmaWaitTagStatusAll(DMA_MASK(1));
#else	// COMPRESSED_VERTEX_METHOD == 0
	u8 compressedVertexBuffer[32 * 1];
	const CompressedVertexType *ppuCompressedVertex0 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex0];
	// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the PPU addresses.
	const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex0) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[0], (uint64_t)ppuDMAAddress0, 32, DMA_TAG(1));

	const u32 bufferOffset0 = (u32)(ppuCompressedVertex0) & 15;
	const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset0]);

	sysDmaWaitTagStatusAll(DMA_MASK(1));

	v0 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector0));
#endif	// COMPRESSED_VERTEX_METHOD == 0
#endif	// !__SPU
}


__forceinline void FetchVertices2(const phBoundBVH *bvhBound, int vertIndex0, int vertIndex1, Vec3V_InOut v0, Vec3V_InOut v1)
{
	PF_FUNC_2(GetCompressedVertex);
#if !__SPU
	v0 = bvhBound->GetBVHVertex(vertIndex0);
	v1 = bvhBound->GetBVHVertex(vertIndex1);
#else	// !__SPU
#if COMPRESSED_VERTEX_METHOD == 0
	sysDmaLargeGet(&v0, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex0], sizeof(Vec3V), DMA_TAG(1));
	sysDmaLargeGet(&v1, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex1], sizeof(Vec3V), DMA_TAG(1));
	sysDmaWaitTagStatusAll(DMA_MASK(1));
#else	// COMPRESSED_VERTEX_METHOD == 0
	u8 compressedVertexBuffer[32 * 2];
	const CompressedVertexType *ppuCompressedVertex0 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex0];
	const CompressedVertexType *ppuCompressedVertex1 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex1];
	// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the PPU addresses.
	const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex0) & ~15);
	const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex1) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[0], (uint64_t)ppuDMAAddress0, 32, DMA_TAG(1));
	sysDmaLargeGet(&compressedVertexBuffer[32], (uint64_t)ppuDMAAddress1, 32, DMA_TAG(1));

	const u32 bufferOffset0 = (u32)(ppuCompressedVertex0) & 15;
	const u32 bufferOffset1 = (u32)(ppuCompressedVertex1) & 15;
	const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset0]);
	const CompressedVertexType *spuCompressedVector1 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset1 + 32]);

	sysDmaWaitTagStatusAll(DMA_MASK(1));

	v0 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector0));
	v1 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector1));
#endif	// COMPRESSED_VERTEX_METHOD == 0
#endif	// !__SPU
}

#if !PREFETCH_VERTICES
__forceinline void FetchVertices3(const phBoundBVH *bvhBound, int vertIndex0, int vertIndex1, int vertIndex2, Vec3V_InOut v0, Vec3V_InOut v1
	, Vec3V_InOut v2)
{
	PF_FUNC_2(GetCompressedVertex);
#if !__SPU
	v0 = bvhBound->GetBVHVertex(vertIndex0);
	v1 = bvhBound->GetBVHVertex(vertIndex1);
	v2 = bvhBound->GetBVHVertex(vertIndex2);
#else	// !__SPU
#if COMPRESSED_VERTEX_METHOD == 0
	sysDmaLargeGet(&v0, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex0], sizeof(Vec3V), DMA_TAG(1));
	sysDmaLargeGet(&v1, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex1], sizeof(Vec3V), DMA_TAG(1));
	sysDmaLargeGet(&v2, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex2], sizeof(Vec3V), DMA_TAG(1));
	sysDmaWaitTagStatusAll(DMA_MASK(1));
#else	// COMPRESSED_VERTEX_METHOD == 0
	u8 compressedVertexBuffer[32 * 3];

	const CompressedVertexType *ppuCompressedVertex0 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex0];
	const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex0) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[0], (uint64_t)ppuDMAAddress0, 32, DMA_TAG(1));

	const CompressedVertexType *ppuCompressedVertex1 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex1];
	const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex1) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[32], (uint64_t)ppuDMAAddress1, 32, DMA_TAG(2));

	const CompressedVertexType *ppuCompressedVertex2 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex2];
	const CompressedVertexType *ppuDMAAddress2 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex2) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[64], (uint64_t)ppuDMAAddress2, 32, DMA_TAG(3));

	const u32 bufferOffset0 = (u32)(ppuCompressedVertex0) & 15;
	const u32 bufferOffset1 = (u32)(ppuCompressedVertex1) & 15;
	const u32 bufferOffset2 = (u32)(ppuCompressedVertex2) & 15;
	const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset0]);
	const CompressedVertexType *spuCompressedVector1 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset1 + 32]);
	const CompressedVertexType *spuCompressedVector2 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset2 + 64]);

	sysDmaWaitTagStatusAll(DMA_MASK(1));
	v0 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector0));

	sysDmaWaitTagStatusAll(DMA_MASK(2));
	v1 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector1));

	sysDmaWaitTagStatusAll(DMA_MASK(3));
	v2 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector2));
#endif	// COMPRESSED_VERTEX_METHOD == 0
#endif	// !__SPU
}
#else	// !PREFETCH_VERTICES
// finalCompressedVertexX tells you where the compressed vertex data ended up.  On SPU this will point into compressedVertexBuffer and on PPU this will just point
//   into the vertex list of bvhBound.
__forceinline void PrefetchVertices3(const CompressedVertexType *&finalCompressedVertex0, const CompressedVertexType *&finalCompressedVertex1, const CompressedVertexType *&finalCompressedVertex2, const phBoundBVH *bvhBound, int vertIndex0, int vertIndex1, int vertIndex2 SPU_PARAM(u8 *compressedVertexBuffer))
{
	PF_FUNC_2(GetCompressedVertex);

	const CompressedVertexType *compressedVertex0 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex0];
	const CompressedVertexType *compressedVertex1 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex1];
	const CompressedVertexType *compressedVertex2 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex2];

#if !__SPU
	PrefetchDC(compressedVertex0);
	PrefetchDC(compressedVertex1);
	PrefetchDC(compressedVertex2);

	finalCompressedVertex0 = compressedVertex0;
	finalCompressedVertex1 = compressedVertex1;
	finalCompressedVertex2 = compressedVertex2;
#else	// !__SPU
	const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertex0) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[0], (uint64_t)ppuDMAAddress0, 32, DMA_TAG(1));

	const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertex1) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[32], (uint64_t)ppuDMAAddress1, 32, DMA_TAG(2));

	const CompressedVertexType *ppuDMAAddress2 = reinterpret_cast<const CompressedVertexType *>((int)(compressedVertex2) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[64], (uint64_t)ppuDMAAddress2, 32, DMA_TAG(3));

	const u32 bufferOffset0 = (u32)(compressedVertex0) & 15;
	const u32 bufferOffset1 = (u32)(compressedVertex1) & 15;
	const u32 bufferOffset2 = (u32)(compressedVertex2) & 15;
	finalCompressedVertex0 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset0]);
	finalCompressedVertex1 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset1 + 32]);
	finalCompressedVertex2 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset2 + 64]);
#endif	// !__SPU
}


__forceinline void DecompressVertices3(const phBoundBVH *bvhBound, const CompressedVertexType *finalCompressedVertex0, const CompressedVertexType *finalCompressedVertex1, const CompressedVertexType *finalCompressedVertex2, Vec3V_InOut v0, Vec3V_InOut v1, Vec3V_InOut v2)
{
	SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(1)));
	v0 = Vec3V(bvhBound->DecompressVertex(finalCompressedVertex0));

	SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(2)));
	v1 = Vec3V(bvhBound->DecompressVertex(finalCompressedVertex1));

	SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(3)));
	v2 = Vec3V(bvhBound->DecompressVertex(finalCompressedVertex2));
}
#endif	// !PREFETCH_VERTICES


__forceinline void FetchVertices4(const phBoundBVH *bvhBound, int vertIndex0, int vertIndex1, int vertIndex2, int vertIndex3, Vec3V_InOut v0
								  , Vec3V_InOut v1, Vec3V_InOut v2, Vec3V_InOut v3)
{
	PF_FUNC_2(GetCompressedVertex);
#if !__SPU
	v0 = bvhBound->GetBVHVertex(vertIndex0);
	v1 = bvhBound->GetBVHVertex(vertIndex1);
	v2 = bvhBound->GetBVHVertex(vertIndex2);
	v3 = bvhBound->GetBVHVertex(vertIndex3);
#else	// !__SPU
#if COMPRESSED_VERTEX_METHOD == 0
	sysDmaLargeGet(&v0, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex0], sizeof(Vec3V), DMA_TAG(1));
	sysDmaLargeGet(&v1, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex1], sizeof(Vec3V), DMA_TAG(1));
	sysDmaLargeGet(&v2, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex2], sizeof(Vec3V), DMA_TAG(1));
	sysDmaLargeGet(&v3, (uint64_t)&bvhBound->GetVertexPointer()[vertIndex3], sizeof(Vec3V), DMA_TAG(1));
	sysDmaWaitTagStatusAll(DMA_MASK(1));
#else	// COMPRESSED_VERTEX_METHOD == 0
	u8 compressedVertexBuffer[32 * 4];
	const CompressedVertexType *ppuCompressedVertex0 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex0];
	const CompressedVertexType *ppuCompressedVertex1 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex1];
	const CompressedVertexType *ppuCompressedVertex2 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex2];
	const CompressedVertexType *ppuCompressedVertex3 = &bvhBound->GetCompressedVertexPointer()[3 * vertIndex3];
	// We need to DMA in from a 16-byte aligned address, so let's chop off the bottom portion of the PPU addresses.
	const CompressedVertexType *ppuDMAAddress0 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex0) & ~15);
	const CompressedVertexType *ppuDMAAddress1 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex1) & ~15);
	const CompressedVertexType *ppuDMAAddress2 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex2) & ~15);
	const CompressedVertexType *ppuDMAAddress3 = reinterpret_cast<const CompressedVertexType *>((int)(ppuCompressedVertex3) & ~15);
	sysDmaLargeGet(&compressedVertexBuffer[0], (uint64_t)ppuDMAAddress0, 32, DMA_TAG(1));
	sysDmaLargeGet(&compressedVertexBuffer[32], (uint64_t)ppuDMAAddress1, 32, DMA_TAG(1));
	sysDmaLargeGet(&compressedVertexBuffer[64], (uint64_t)ppuDMAAddress2, 32, DMA_TAG(1));
	sysDmaLargeGet(&compressedVertexBuffer[96], (uint64_t)ppuDMAAddress3, 32, DMA_TAG(1));

	const u32 bufferOffset0 = (u32)(ppuCompressedVertex0) & 15;
	const u32 bufferOffset1 = (u32)(ppuCompressedVertex1) & 15;
	const u32 bufferOffset2 = (u32)(ppuCompressedVertex2) & 15;
	const u32 bufferOffset3 = (u32)(ppuCompressedVertex3) & 15;
	const CompressedVertexType *spuCompressedVector0 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset0]);
	const CompressedVertexType *spuCompressedVector1 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset1 + 32]);
	const CompressedVertexType *spuCompressedVector2 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset2 + 64]);
	const CompressedVertexType *spuCompressedVector3 = reinterpret_cast<const CompressedVertexType *>(&compressedVertexBuffer[bufferOffset3 + 96]);

	sysDmaWaitTagStatusAll(DMA_MASK(1));

	v0 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector0));
	v1 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector1));
	v2 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector2));
	v3 = Vec3V(bvhBound->DecompressVertex(spuCompressedVector3));
#endif	// COMPRESSED_VERTEX_METHOD == 0
#endif	// !__SPU
}

class ProcessLeafCollisionManifold
{
	phManifold * m_rootManifold;
	phManifold * m_manifold;
	phCollisionCompositeManifoldMgr<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS> * m_newCompositeManifolds;
	int m_componentA;
	int m_componentB;
	bool m_highPriorityManifolds;
	bool m_outOfMemory;
public:

	__forceinline ProcessLeafCollisionManifold(phCollisionCompositeManifoldMgr<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS> * const newCompositeManifolds, const bool highPriorityManifolds) : 
											   m_newCompositeManifolds(newCompositeManifolds), m_highPriorityManifolds(highPriorityManifolds) 
	{
	}

	__forceinline void SetManifold(phManifold * const rootManifold)
	{
		Assert(rootManifold);
		m_rootManifold = rootManifold;
		m_manifold = rootManifold;
		m_outOfMemory = false;
	}

	__forceinline void SetComponents(phManifold * const rootManifold, const int componentA, const int componentB)
	{
		m_rootManifold = rootManifold;
		m_manifold = NULL;
		m_componentA = componentA;
		m_componentB = componentB;
		m_outOfMemory = false;
	}

	__forceinline phManifold * GetManifold(const phBound * const boundA, const phBound * const boundB)
	{
		if (!m_manifold)
		{
			Assert(!m_outOfMemory);
			m_manifold = m_newCompositeManifolds->PreLeafCollisionFindManifold(m_componentA, m_componentB, boundA, boundB, m_highPriorityManifolds);
			m_outOfMemory = (m_manifold == NULL);
		}
		return m_manifold;
	}

	__forceinline phManifold * GetRootManifold()
	{
		return m_rootManifold;
	}

	__forceinline bool HasManifold() const
	{
		return (m_manifold != NULL);
	}

	__forceinline bool OutOfMemory() const
	{
		return m_outOfMemory;
	}
};

#if PRIM_CACHE_RENDER

EXT_PFD_DECLARE_ITEM(BvhNodeCollisions);

struct PrimitiveRenderCache
{
	typedef PrimitiveRenderCache* NID;

	struct key_t
	{
		u32 a1;
		u32 a2;
	};

	static __forceinline key_t MakeKey(const u32 levelIndex, const u16 component, const int partIndex)
	{
		FastAssert(levelIndex <= 0xFFFF);
		FastAssert(component <= 0xFF);
		FastAssert(partIndex <= 0xFFFF && partIndex >= 0);
		key_t k;
		k.a1 = (((u32)levelIndex) << 8) | (u32)component;
		k.a2 = (u32)partIndex;
		return k;
	}

	static __forceinline int KeyCmp(const key_t & k1, const key_t & k2)
	{
		return (k1.a1 == k2.a1) ? (k1.a2 < k2.a2) : (k1.a1 < k2.a1);
	}

	static __forceinline int KeyEqu(const key_t & k1, const key_t & k2)
	{
		return (k1.a1 == k2.a1 && k1.a2 == k2.a2);
	}

	key_t m_key;
	NID m_left;
	NID m_right;
	char m_balance;
};

struct PrimitiveRender
{
	enum
	{
		MAX_CACHE_COUNT = 1024 * 4,
	};

	struct PrimitiveRenderCacheAvlTreeAccessor
	{
		typedef PrimitiveRenderCache::NID NID;		// Node ID type. Could be an index, pointer, etc.
		typedef PrimitiveRenderCache::key_t KT;		// Key type.
		typedef char BT;							// Balance type.

		static __forceinline NID null() { return NULL; }
		static __forceinline NID & get_left(NID & nid) { return nid->m_left; }
		static __forceinline NID & get_right(NID & nid) { return nid->m_right; }
		static __forceinline BT & get_bal(NID & nid) { return nid->m_balance; }
		static __forceinline const KT & get_key(const NID & nid) { return nid->m_key; }
		static __forceinline void set_key(NID & /*nid*/, const KT & /*key*/) { /* key already set */ }//{ nid->key = key; }
		static __forceinline int cmp(const KT & k1, const KT & k2) { return PrimitiveRenderCache::KeyCmp(k1,k2); }
		static __forceinline int equ(const KT & k1, const KT & k2) { return PrimitiveRenderCache::KeyEqu(k1,k2); }
		static __forceinline void prefetch_find(NID & nid) { PrefetchObject<PrimitiveRenderCache>(nid); }
		static __forceinline void prefetch_insert(NID & /*nid*/) { /*PrefetchObject<PrimitiveLeaf>(nid);*/ }
		static __forceinline void prefetch_remove(NID & /*nid*/) { /*PrefetchObject<PrimitiveLeaf>(nid);*/ }
	};

	typedef avl_tree<PrimitiveRenderCacheAvlTreeAccessor> map_t;

	PrimitiveRenderCache m_list[MAX_CACHE_COUNT];
	int m_listCount;
	PrimitiveRenderCacheAvlTreeAccessor::NID m_root;

	sysCriticalSectionToken m_token;
	
	bool m_render;

	PrimitiveRender()
	{
		m_render = true;
		Reset();
	}

	void Reset()
	{
		m_listCount = 0;
		m_root = PrimitiveRenderCacheAvlTreeAccessor::null();
	}

	int ShouldRender(const u32 levelIndex, const u16 component, const int partIndex)
	{
		int retv = 0;
		if (m_render)
		{
			const PrimitiveRenderCache::key_t key = PrimitiveRenderCache::MakeKey(levelIndex,component,partIndex);
			map_t cacheMap(m_root);
			m_token.Lock();
			if (cacheMap.find(key) == PrimitiveRenderCacheAvlTreeAccessor::null())
			{
				if (m_listCount < MAX_CACHE_COUNT)
				{
					PrimitiveRenderCache * prc = m_list + m_listCount;
					m_listCount++;
					prc->m_key = key;
					cacheMap.insert(prc,key);
					m_root = cacheMap.get_root();
					retv = 1;
				}
			}
			m_token.Unlock();
		}		
		return retv;
	}

	void RenderTriangle(Vec3V_In v0_, Vec3V_In v1_, Vec3V_In v2_, Vec3V_In normal_, Mat34V_In xform_, const Color32 & color_)
	{
		if (PFD_BvhNodeCollisions.Begin())
		{
			static float normal_offset = .02f;
			const Vec3V offset = ScalarV(normal_offset) * normal_;
			const Vec3V v0 = v0_ + offset;
			const Vec3V v1 = v1_ + offset;
			const Vec3V v2 = v2_ + offset;

			grcWorldMtx(xform_);

			grcColor(color_);
			grcBegin(drawTris,3);
			grcNormal3f(normal_);
			grcVertex3f(v0);
			grcVertex3f(v1);
			grcVertex3f(v2);
			grcEnd();

			grcColor(Color_white);
			grcBegin(drawLineStrip,4);
			grcVertex3f(v0);
			grcVertex3f(v1);
			grcVertex3f(v2);
			grcVertex3f(v0);
			grcEnd();

			PFD_BvhNodeCollisions.End();
		}
	}

	void RenderBox(Vec3V_In center, Vec3V_In halfDims, Mat34V_In xform_, const Color32 & color_)
	{
		Mat34V mat = xform_;
		mat.SetCol3(center);
		const Vec3V size = ScalarV(V_TWO) * halfDims;
		if (PFD_BvhNodeCollisions.Begin())
		{
			grcDrawBox(RCC_VECTOR3(size),RCC_MATRIX34(mat),color_);
			PFD_BvhNodeCollisions.End();
		}
	}

	void RenderNodeBox(PrimitiveLeaf * leaf, Mat34V_In xform_, const Color32 & color_)
	{ 
		Vec3V nodeAABBMin;
		Vec3V nodeAABBMax;
		const phOptimizedBvh * bvh = leaf->GetBoundBVH()->GetBVH();
		const phOptimizedBvhNode * node = leaf->m_curNode;
		bvh->UnQuantize(RC_VECTOR3(nodeAABBMin), node->m_AABBMin);
		bvh->UnQuantize(RC_VECTOR3(nodeAABBMax), node->m_AABBMax);
		const ScalarV v_half(V_HALF);
		const Vec3V halfDims = v_half * (nodeAABBMax - nodeAABBMin);
		const Vec3V center = v_half * (nodeAABBMin + nodeAABBMax);
		RenderBox(center,halfDims,xform_,color_);
	}

	void RenderCylinderBox(Vec3V_In endCenter0, Vec3V_In endCenter1, ScalarV_In cylinderRadius, Mat34V_In xform_, const Color32 & color_)
	{
		const Vec3V unitCylinderShaft = Normalize(Subtract(endCenter1, endCenter0));
		const Vec3V sineRatios = Sqrt(Max(Vec3V(V_ONE) - unitCylinderShaft * unitCylinderShaft,Vec3V(V_ZERO)));
		const Vec3V discHalfDims = cylinderRadius * sineRatios;
		const Vec3V AABBMin = Min(endCenter0,endCenter1) - discHalfDims;
		const Vec3V AABBMax = Max(endCenter0,endCenter1) + discHalfDims;
		const ScalarV v_half(V_HALF);
		const Vec3V halfDims = v_half * (AABBMax - AABBMin);
		const Vec3V center = v_half * (AABBMin + AABBMax);
		RenderBox(center,halfDims,xform_,color_);
	}

	void RenderCylinderBox(const PrimitiveCylinder * pm, Mat34V_In xform_, const Color32 color_)
	{
		RenderCylinderBox(pm->cylinderEnd0,pm->cylinderEnd1,ScalarV(pm->m_radius),xform_,color_);
	}

	void Render(PrimitiveLeaf * leaf, Mat34V_In xform_, const u32 levelIndex, const u16 component, const int partIndex)
	{
		static Vec3V sizeOffset(-.1f,-.1f,-.1f);
		static Vec3V minSize(.1f,.1f,.1f);
		static Vec3V positionOffset(0.0f,0.0f,.25f);
		static const Color32 colors[6] = {Color_yellow,Color_red,Color_blue,Color_green,Color_purple,Color_LightBlue};
		int color_i = 0;

		color_i = leaf->m_curNode - leaf->GetBoundBVH()->GetBVH()->GetRootNode();
		const int saveRand = rand();
		srand(color_i);
		color_i = rand() % 6;
		srand(saveRand);
		if (ShouldRender(levelIndex,component,partIndex))
		{
			for (PrimitiveBase * prim = leaf->m_first ; prim ; prim = prim->m_next)
			{
				const TriangleShape * tm = prim->GetTriangleShape();
				if (tm)
				{
					Vec3V v0,v1,v2;
					prim->GetTriangleVertices(&v0,&v1,&v2);
					RenderTriangle(v0,v1,v2,tm->m_PolygonNormal,xform_,colors[color_i]);
				}
				//const PrimitiveCylinder * pc = prim->GetPrimitiveCylinder();
				//if (pc)
				//	RenderCylinderBox(pc,xform_,Color_red);
			}
			//RenderNodeBox(leaf,xform_,Color_yellow);
		}
	}
};

PrimitiveRender g_PrimitiveRender;

void PrimitiveRenderReset()
{
	g_PrimitiveRender.Reset();
}

#endif // PRIM_CACHE_RENDER

// PURPOSE: Branch-less function to generate size_t condition mask.
// PARAMS:	condition
// RETURN:  0xFFFFFFFF if condition != 0, 0 otherwise
__forceinline size_t GenerateMaskCondition(const s32 condition)
{
	const size_t output = ((ptrdiff_t)(condition | -condition)) >> (sizeof(size_t)*8-1);
	return output;
}

template <class Type> __forceinline const Type *ISelectP(size_t selector, const Type *input0, const Type *input1)
{
	const Type* output = (const Type *)((selector & (size_t)input1) | (~selector & (size_t)input0));
	return output;
}

void PrimitiveTriangle::Init(const phPrimitive & bvhPrimitive, const int curPrimIndex)
{
	curPrimitive = bvhPrimitive.GetPolygon();

	const phBoundBVH * boundBVH = m_leaf->GetBoundBVH();

	const int triVertIndex0 = curPrimitive.GetVertexIndex(0);
	const int triVertIndex1 = curPrimitive.GetVertexIndex(1);
	const int triVertIndex2 = curPrimitive.GetVertexIndex(2);
	FastAssert(triVertIndex0 < boundBVH->GetNumVertices());
	FastAssert(triVertIndex1 < boundBVH->GetNumVertices());
	FastAssert(triVertIndex2 < boundBVH->GetNumVertices());

	Vec3V vertex0, vertex1, vertex2;

#if PREFETCH_VERTICES

#if __SPU
	u8 triVertBuffer[32 * 3] ;
#endif	

	const CompressedVertexType *finalCompressedVertex0, *finalCompressedVertex1, *finalCompressedVertex2;
	PrefetchVertices3(finalCompressedVertex0, finalCompressedVertex1, finalCompressedVertex2, boundBVH, triVertIndex0, triVertIndex1, triVertIndex2 SPU_PARAM(triVertBuffer));
	DecompressVertices3(boundBVH, finalCompressedVertex0, finalCompressedVertex1, finalCompressedVertex2, vertex0, vertex1, vertex2);

#else // PREFETCH_VERTICES

	FetchVertices3(boundBVH, triVertIndex0, triVertIndex1, triVertIndex2, vertex0, vertex1, vertex2);

#endif // PREFETCH_VERTICES

	// This is the minimal setup needed to do the box rejection test.
	localTrianglePosition = Vec3V(V_ZERO);
	// TODO: Look into creating a simpler default constructor. The current one initializes a bunch of things that aren't needed in the primitive cache.
	TriangleShape * tm = PRIM_NEW(TriangleShape);
	tm->m_PolygonNormal = curPrimitive.ComputeUnitNormal(vertex0, vertex1, vertex2);
	tm->m_vertices1[0] = vertex0;
	tm->m_vertices1[1] = vertex1;
	tm->m_vertices1[2] = vertex2;
	tm->SetIndexInBound(curPrimIndex);
}

PHYSICS_FORCE_INLINE void PrimitiveTriangle::Setup(const NewCollisionInput & collisionInput)
{
	FastAssert(IsSetup() == false);
	
	const int triVertIndex0 = curPrimitive.GetVertexIndex(0);
	const int triVertIndex1 = curPrimitive.GetVertexIndex(1);
	const int triVertIndex2 = curPrimitive.GetVertexIndex(2);

	const phBoundBVH * boundBVH = m_leaf->GetBoundBVH();
	FastAssert(boundBVH);
	if( !PARAM_disableFix7459244.Get() && !boundBVH)
		return;

	TriangleShape * tm = GetTriangle();
	FastAssert(tm);
	const int partIndex = tm->GetIndexFromBound();

#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	Vec3V & edgeNormal0 = tm->m_EdgeNormals[0];
	Vec3V & edgeNormal1 = tm->m_EdgeNormals[1];
	Vec3V & edgeNormal2 = tm->m_EdgeNormals[2];
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	const Vec3V triVert0 = tm->m_vertices1[0];
	const Vec3V triVert1 = tm->m_vertices1[1];
	const Vec3V triVert2 = tm->m_vertices1[2];
	const Vec3V polyNormal = tm->m_PolygonNormal;
	localTrianglePosition = triVert0;

#if __SPU
	u8 neighborPolygonBuffer[sizeof(phPolygon) * 3] ;
	phPolygon *neighbor0, *neighbor1, *neighbor2;
#else
	const phPolygon *neighbor0, *neighbor1, *neighbor2;	// These will get set in PrefetchNeighborsAndComputeNeighborFlags().
#endif	// __SPU


#if __SPU
	neighbor0 = reinterpret_cast<phPolygon *>(&neighborPolygonBuffer[sizeof(phPolygon) * 0]);
	neighbor1 = reinterpret_cast<phPolygon *>(&neighborPolygonBuffer[sizeof(phPolygon) * 1]);
	neighbor2 = reinterpret_cast<phPolygon *>(&neighborPolygonBuffer[sizeof(phPolygon) * 2]);
#endif // __SPU
	hasNeighbor = PrefetchNeighborsAndComputeNeighborFlags(neighbor0, neighbor1, neighbor2, curPrimitive, partIndex, boundBVH);
	ComputeEdgeAndNeighborNormals(
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
		edgeNormal0, edgeNormal1, edgeNormal2, 
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
		neighborNormals0, neighborNormals1, neighborNormals2, polyNormal, triVert0, triVert1, triVert2, *neighbor0, *neighbor1, *neighbor2, triVertIndex0, triVertIndex1, triVertIndex2, boundBVH, hasNeighbor);

#if USE_CHECK_TRIANGLE
	const phInst * pInstOfBvh = m_leaf->GetInstOfBvh();
#if __SPU
	const phArchetype* rootBVHinstArch = NULL;
	u8 instanceBuffer[sizeof(phInst)] ;
	if(pInstOfBvh != NULL)
	{
		const phInst *spuInst = reinterpret_cast<const phInst *>(&instanceBuffer[0]);
		sysDmaLargeGet(spuInst, (uint64_t)pInstOfBvh, sizeof(phInst), DMA_TAG(13));
		sysDmaWaitTagStatusAll(DMA_MASK(13));
		rootBVHinstArch = spuInst->GetArchetype();
	}
#else // __SPU
	const phArchetype* rootBVHinstArch = pInstOfBvh != NULL ? pInstOfBvh->GetArchetype() : NULL;
#endif // __SPU
	CheckTriangle(polyNormal,localTrianglePosition,edgeNormal0,edgeNormal1,edgeNormal2,collisionInput.m_bvhInput->m_current,curPrimitive,rootBVHinstArch,partIndex);
#endif // USE_CHECK_TRIANGLE

	tm->m_vertices1[0] = Vec3V(V_ZERO);
	tm->m_vertices1[1] -= localTrianglePosition;
	tm->m_vertices1[2] -= localTrianglePosition;
#if COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	tm->m_VertexNormalCodes[0] = curPrimitive.GetVertexNormalCode(0);
	tm->m_VertexNormalCodes[1] = curPrimitive.GetVertexNormalCode(1);
	tm->m_VertexNormalCodes[2] = curPrimitive.GetVertexNormalCode(2);
#endif // COLLISION_MAY_USE_TRIANGLE_PD_SOLVER
	tm->SetMargin(boundBVH->GetMarginV());

	const BVHInput * bvhInput_ = collisionInput.m_bvhInput;
	FastAssert(bvhInput_);
	const Mat34V currentMatrix = bvhInput_->GetCurrentMatrix();
	const Mat34V lastMatrix = bvhInput_->GetLastMatrix();
	const int component = bvhInput_->GetComponent();

	collisionObject.m_current.Set3x3(currentMatrix);
	collisionObject.m_current.SetCol3(Transform(currentMatrix,localTrianglePosition));
#if USE_NEWER_COLLISION_OBJECT
	// These translation calculations have been optimized. In particular, they don't need to explicitly compute collisionObject.m_last. Also, cg offset is zero.
	// Original formula: trans = ComputeLinearMotion_(collisionObject->m_last,collisionObject->m_current,bound.GetCGOffset());
#if BVH_PRIMS_HAVE_ZERO_CG_OFFSET
	FastAssert(IsEqualAll(tm->GetCGOffset(),Vec3V(V_ZERO)));
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,localTrianglePosition);
#else
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,localTrianglePosition+tm->GetCGOffset());
#endif
#else
	collisionObject.m_last.Set3x3(lastMatrix);
	collisionObject.m_last.SetCol3(Transform(lastMatrix,localTrianglePosition));
#endif
	collisionObject.set(tm,component);
}

#if USE_PROJECTION_EDGE_FILTERING
// Returns 0 to thow out contact, 1 to keep contact normal as is, 2 keep contact normal that was projected.
enum ProjectContactNormal_e
{
	ProjectContactNormal_THROW_OUT = 0,
	ProjectContactNormal_KEEP = 1,
	ProjectContactNormal_KEEP_PROJECTED = 2,
};

int ProjectContactNormal(Vec3V_InOut contactNormalInOut, Vec3V_In faceNormal, Vec3V_In neighborNormal0, Vec3V_In neighborNormal1, Vec3V_In neighborNormal2, Vec3V_In v0, Vec3V_In v1, Vec3V_In v2, const int edgeValid)
{
	Vec3V planeNormal[3];
	Vec3V neighborFaceNormal[3];
	int planeNormalCount = 0;
	const ScalarV v_divideEps(V_FLT_SMALL_6);
	const ScalarV v_zero(V_ZERO);
	const ScalarV v_one(V_ONE);

	const Vec3V contactNormal = contactNormalInOut;
	
	if (edgeValid & 1)
	{
		const Vec3V edge = v1 - v0;
		const Vec3V outwardNormal = Cross(edge,faceNormal);
		const ScalarV outwardDist = Dot(contactNormal,outwardNormal);
		if (IsGreaterThanAll(outwardDist,v_zero))
		{
			const Vec3V normal = Cross(neighborNormal0,edge);
			const ScalarV normalLength = Mag(normal);
			if (IsGreaterThanOrEqualAll(normalLength,v_divideEps))
			{
				planeNormal[0] = normal / normalLength;
				neighborFaceNormal[0] = neighborNormal0;
				planeNormalCount = 1;
			}
		}
	}
	if (edgeValid & 2)
	{
		const Vec3V edge = v2 - v1;
		const Vec3V outwardNormal = Cross(edge,faceNormal);
		const ScalarV outwardDist = Dot(contactNormal,outwardNormal);
		if (IsGreaterThanAll(outwardDist,v_zero))
		{
			const Vec3V normal = Cross(neighborNormal1,edge);
			const ScalarV normalLength = Mag(normal);
			if (IsGreaterThanOrEqualAll(normalLength,v_divideEps))
			{
				planeNormal[planeNormalCount] = normal / normalLength;
				neighborFaceNormal[planeNormalCount] = neighborNormal1;
				planeNormalCount++;
			}
		}
	}
	if (edgeValid & 4)
	{
		const Vec3V edge = v0 - v2;
		const Vec3V outwardNormal = Cross(edge,faceNormal);
		const ScalarV outwardDist = Dot(contactNormal,outwardNormal);
		if (IsGreaterThanAll(outwardDist,v_zero))
		{
			const Vec3V normal = Cross(neighborNormal2,edge);
			const ScalarV normalLength = Mag(normal);
			if (IsGreaterThanOrEqualAll(normalLength,v_divideEps))
			{
				planeNormal[planeNormalCount] = normal / normalLength;
				neighborFaceNormal[planeNormalCount] = neighborNormal2;
				planeNormalCount++;
			}
		}		
	}

	if (planeNormalCount == 0)
	{
		return ProjectContactNormal_KEEP;
	}

	const ScalarV distEps(0.02f);	// sin(angularTolerance), a little over 1 degree of tolerance.

	Vec3V contactNormalProjected = contactNormal;
	bool insideValidSpace = true;
	bool gotValidProjection = false;

	if (planeNormalCount == 1)
	{
		// Project contact normal onto 1 plane.
		const int i = 0;
		const ScalarV lambda = -Dot(contactNormal,planeNormal[i]);
		if (IsGreaterThanOrEqualAll(lambda-distEps,v_zero))
		{
			insideValidSpace = false;
			const Vec3V contactNormalProjectedCur = contactNormal + lambda * planeNormal[i];
			// Only keep this if were not on all backsides.
			const ScalarV faceDot0 = Dot(contactNormalProjectedCur,faceNormal);
			const ScalarV faceDot1 = Dot(contactNormalProjectedCur,neighborFaceNormal[i]);
			if (IsGreaterThanAll(faceDot0,v_zero) || IsGreaterThanAll(faceDot1,v_zero))
			{
				contactNormalProjected = contactNormalProjectedCur;
				gotValidProjection = true;
			}
		}
	}
	else if (planeNormalCount == 2)
	{
		// Project contact normal onto 2 planes.
		const int i = 0;
		const int j = 1;
		const Vec3V planeNormal_i = planeNormal[i];
		const Vec3V planeNormal_j = planeNormal[j];
		const ScalarV dotij = Dot(planeNormal_i,planeNormal_j);
		const ScalarV det_ = v_one - dotij * dotij;
		if (IsGreaterThanAll(det_,v_divideEps))
		{
			ScalarV rs[2];
			rs[i] = -Dot(contactNormal,planeNormal[i]);
			rs[j] = -Dot(contactNormal,planeNormal[j]);
			// | 1     dotij | = rs[i] - distEps
			// | dotij 1     | = rs[j] - distEps
			// numer_i + eps = (rs[i] - de) * 1 - (rs[j] - de) * dotij
			// numer_j + eps = 1 * (rs[j] - de) - dotij * (rs[i] - ed)
			const ScalarV numer_i = rs[i] - rs[j] * dotij;
			const ScalarV numer_j = rs[j] - rs[i] * dotij;
			const ScalarV eps = distEps * (dotij - v_one);
			if (IsGreaterThanAll(numer_i + eps,v_zero) && IsGreaterThanAll(numer_j + eps,v_zero))
			{
				insideValidSpace = false;
				const Vec3V contactNormalProjectedCur = contactNormal + (numer_i * planeNormal_i + numer_j * planeNormal_j) / det_;
				// Only keep this if were not on all backsides.
				const ScalarV faceDot0 = Dot(contactNormalProjectedCur,faceNormal);
				const ScalarV faceDot1 = Dot(contactNormalProjectedCur,neighborFaceNormal[i]);
				const ScalarV faceDot2 = Dot(contactNormalProjectedCur,neighborFaceNormal[j]);
				if (IsGreaterThanAll(faceDot0,v_zero) || IsGreaterThanAll(faceDot1,v_zero) || IsGreaterThanAll(faceDot2,v_zero))
				{
					contactNormalProjected = contactNormalProjectedCur;
					gotValidProjection = true;
				}
			}
		}
		else
		{
			// This shouldn't be possible. The two planes are parallel. Most likely a degenerate triangle.
			Assertf(0,"Degenerte projections parallel planes.");
			insideValidSpace =  false;
			gotValidProjection = false;
		}
	}
	else
	{
		Assert(planeNormalCount == 3);
		// This shouldn't be possible. Most likely a degenerate triangle.
		Assertf(0,"Degenerate projection 3 planes.");
		insideValidSpace =  false;
		gotValidProjection = false;
	}
	
	if (insideValidSpace)
	{
		return ProjectContactNormal_KEEP;
	}

	if (gotValidProjection == false)
	{
		// The projection was far outside of the valid range. It's possible to project these cases into valid range but we'll just throw them out.
		return ProjectContactNormal_THROW_OUT;
	}

	const ScalarV contactNormalProjectedLength = Mag(contactNormalProjected);
	if (IsLessThanOrEqualAll(contactNormalProjectedLength,v_divideEps))
	{
		// Degenerate projection. Throw out.
		return ProjectContactNormal_THROW_OUT;
	}

	contactNormalProjected /= contactNormalProjectedLength;

	// Everything should be good at this point.
	contactNormalInOut = contactNormalProjected;
	return ProjectContactNormal_KEEP_PROJECTED;
}
#endif // USE_PROJECTION_EDGE_FILTERING

PHYSICS_FORCE_INLINE void PrimitiveTriangle::ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	FastAssert(pointCollector.GetHasResult());

	{
#if USE_NEW_TRIANGLE_BACK_FACE_CULL

		const Mat34V curTransform1 = collisionInput.object1->m_current;

#else // USE_NEW_TRIANGLE_BACK_FACE_CULL

		const Mat34V curTransform0 = collisionInput.object0->m_current;
		const Mat34V lastTransform0 = collisionInput.object0->m_last;

		const Mat34V curTransform1 = collisionInput.object1->m_current;
		const Mat34V lastTransform1 = collisionInput.object1->m_last;

#endif // USE_NEW_TRIANGLE_BACK_FACE_CULL

		TriangleShape * tm = GetTriangle();

		const Vec3V localizedVertices0 = tm->m_vertices1[0];
		const Vec3V localizedVertices1 = tm->m_vertices1[1];
		const Vec3V localizedVertices2 = tm->m_vertices1[2];

		const Vec3V faceNormal = tm->m_PolygonNormal;

		const ScalarV margin = m_leaf->GetBoundBVH()->GetMarginV();

		// Our goal here is the get contact B in the space of relocated (near the origin) triangle that we defined above.
		const Vec3V localPoint = pointCollector.GetPointOnBInLocal();

		// Determine the tolerance to use for the point-on-edge calculations.  Note that we are only using the margin from object B in this case (the concave one).
		//   This is because the point we are testing is coming from the contact point for object B, which will be on (or near) the surface of the margin-expanded
		//   concave bound.
		const ScalarV vsPointOnEdgeTolerance = (margin * margin) * s_PointOnEdgeToleranceFactorSquared;
		const int pointOnEdgeFlags = GetPointOnEdgeFlags(localizedVertices0, localizedVertices1, localizedVertices2, localPoint, vsPointOnEdgeTolerance);

		NON_SPU_ONLY(ASSERT_ONLY(if (g_UseNormalFiltering || g_UseBackfaceCulling)))
		{
			// NOTE: All of the vectors used here in this contact filtering are in the local space of the triangle bound.
			Vec3V curContactNormal = pointCollector.GetNormalOnBInWorld();
			curContactNormal = UnTransform3x3Ortho(curTransform1, curContactNormal);

#if !__SPU && __ASSERT || EARLY_FORCE_SOLVE
			NON_SPU_ONLY(ASSERT_ONLY(if (g_UseBackfaceCulling)))
			{
				// Check to see if the contact point was moving into the polygon or away from it.  If it's away, we probably don't want it as a contact point.
				// TODO: It seems like this is doing a lot of transforms ... maybe something can be reduced here?
				// Find the motion of the contact point, in the local space of the concave bound.

#if USE_NEW_TRIANGLE_BACK_FACE_CULL

				if (IsLessThanAll(Dot(curContactNormal, faceNormal),ScalarV(V_ZERO)))
				{
					// This contact point is on the backside of the triangle so skip it.
					pointCollector.SetHasResult(false);
					return;
				}

#else // USE_NEW_TRIANGLE_BACK_FACE_CULL

#if USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
				if (Unlikely(collisionInput.object0->m_bound->GetUseNewBackFaceCull()))
				{
					if (IsLessThanAll(Dot(curContactNormal, faceNormal),ScalarV(V_ZERO)))
					{
						// This contact point is on the backside of the triangle so skip it.
						pointCollector.SetHasResult(false);
						return;
					}
				}
				else
#endif // USE_NEW_TRIANGLE_BACK_FACE_CULL_OPTIONAL
				{
					const Vec3V localPointA = pointCollector.GetPointOnAInLocal();
					Mat34V deltaMatrixA(curTransform0);
					deltaMatrixA -= lastTransform0;
					const Vec3V localMotionA_WS = Transform( deltaMatrixA, localPointA );

					const Vec3V localPointB = pointCollector.GetPointOnBInLocal();
					Mat34V deltaMatrixB(curTransform1);
					deltaMatrixB -= lastTransform1;
					const Vec3V localMotionB_WS = Transform( deltaMatrixB, localPointB );

					// Get the final motion, at the point of contact, in the local space of bound 1.
					const Vec3V finalLocalMotion = UnTransform3x3Ortho( curTransform1, localMotionA_WS - localMotionB_WS );

					const ScalarV v_fltsmall2 = ScalarV(V_FLT_SMALL_6);
					if( IsGreaterThanAll( Dot(finalLocalMotion, faceNormal), v_fltsmall2 ) != 0 )
					{
						// This contact point is moving away from the triangle by more than a little, so skip it.
						pointCollector.SetHasResult(false);
						return;
					}
				}
#endif // USE_NEW_TRIANGLE_BACK_FACE_CULL
			}
#endif // !__SPU && __ASSERT

			NON_SPU_ONLY(ASSERT_ONLY(if (g_UseNormalFiltering)))
			{
#if USE_PROJECTION_EDGE_FILTERING
				const u32 useProjectionEdgeFiltering = collisionInput.object0->m_bound->GetUseProjectionEdgeFiltering();
				if (useProjectionEdgeFiltering)
				{
					const int result = ProjectContactNormal(curContactNormal,faceNormal,neighborNormals0,neighborNormals1,neighborNormals2,localizedVertices0,localizedVertices1,localizedVertices2,(hasNeighbor & pointOnEdgeFlags));
					if (result == ProjectContactNormal_THROW_OUT)
					{
						pointCollector.SetHasResult(false);
						return;
					}
					else if (result == ProjectContactNormal_KEEP_PROJECTED)
					{
						curContactNormal = Transform3x3(curTransform1,curContactNormal);
						pointCollector.SetNormalOnBInWorld(curContactNormal);
					}
					else
					{
						Assert(result == ProjectContactNormal_KEEP);
					}
				}
				else
#endif // USE_PROJECTION_EDGE_FILTERING
				{
					const int edgeResult0 = GenerateMaskGZ(phPairwiseCollisionProcessor::CheckContactAgainstEdge(localizedVertices0, localizedVertices1, neighborNormals0, faceNormal, curContactNormal));
					const int edgeResult1 = GenerateMaskGZ(phPairwiseCollisionProcessor::CheckContactAgainstEdge(localizedVertices1, localizedVertices2, neighborNormals1, faceNormal, curContactNormal));
					const int edgeResult2 = GenerateMaskGZ(phPairwiseCollisionProcessor::CheckContactAgainstEdge(localizedVertices2, localizedVertices0, neighborNormals2, faceNormal, curContactNormal));

					// Assert(1 & 1 | 1 & 0); // & gets priority over |, so this works out just like the logical operators
					// Check this contact point against each edge.
#if ENABLE_NORMAL_FILTERING_WIDGET
						if((( NON_SPU_ONLY(ASSERT_ONLY((g_UseNormalFilteringAlways ? 0xFFFFFFFF : hasNeighbor) & ))
							pointOnEdgeFlags ) & ((1 & edgeResult0) | (2 & edgeResult1) | (4 & edgeResult2))) != 0)
#else
						if(( (hasNeighbor & pointOnEdgeFlags) & ((1 & edgeResult0) | (2 & edgeResult1) | (4 & edgeResult2)) ) != 0)
#endif
					{
						pointCollector.SetHasResult(false);
						return;
					}
				}
			}
		}

		NON_SPU_ONLY(BANK_ONLY(if(g_UseNormalClamping)))
		{
			// Check to see if the contact point is close to an edge that doesn't have a neighbors.  Note that this will never check the same edge
			//   that we checked during filtering because there we only want edges that have neighbors.
			// When normal clamping is enabled, we don't want contacts with exposed edges to have normals pointing out into 'the abyss' (the area next to the 
			//   exposed edge that doesn't have a triangle present).  When we find ourselves near an exposed edge we simply set our contact normal to the polygon
			//   normal, which is the correct result 99.999% of the time.  If our contact point is on a vertex shared by two exposed edges then it's possible that
			//   that's not quite the result we want but it's cheap and easy and good enough.
			if((~hasNeighbor & pointOnEdgeFlags) != 0)
			{
				// The contact point is on an edge with no neighboring triangle.  Clamp the normal.
				pointCollector.SetNormalOnBInWorld(Transform3x3(curTransform1, faceNormal));
			}
		}

		// The local point needs to move too (moved by local space offset)
		pointCollector.SetPointOnBInLocal(pointCollector.GetPointOnBInLocal() + localTrianglePosition);

		// The contact constructor that runs when adding an actual new contact currently normalizes the contact normal (Occurs within phManifoldResult::AddContactPoint)
		// As long as this goes on this assert is going to fire spuriously - Therefore it is disabled, but we should turn it back on if that contact normalizing ever stops
		//Assertf( IsLessThanOrEqualAll( Abs( Subtract( Mag(pointCollector.GetNormalOnBInWorld()), ScalarV(V_ONE)) ), ScalarV(V_FLT_SMALL_2) ) , "Contact normal is not normalized: <%f %f %f> -- Triangle Area: %f  -  Norm: << %f, %f, %f >>", Vec3V(pointCollector.GetNormalOnBInWorld()).GetXf(), Vec3V(pointCollector.GetNormalOnBInWorld()).GetYf(), Vec3V(pointCollector.GetNormalOnBInWorld()).GetZf(), triPrim.GetArea(), faceNormal.GetXf(), faceNormal.GetYf(), faceNormal.GetZf());
	}
}

#if PRIM_CACHE_TESTOVERLAP_VIRTUAL
	#define TESTOVERLAP_INLINE 
#else
	#define TESTOVERLAP_INLINE PHYSICS_FORCE_INLINE
#endif

#if PRIM_CACHE_COLLIDE_VIRTUAL
	#define COLLIDE_INLINE
#else
	#define COLLIDE_INLINE PHYSICS_FORCE_INLINE
#endif 

TESTOVERLAP_INLINE int PrimitiveTriangle::TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents)
{
	TriangleShape * tm = GetTriangle();
	const Vec3V boxCenter = boxOrientation.GetCol3() - localTrianglePosition;
	const Vec3V expandedBoxHalfExtents = boxHalfExtents + Vec3V(m_leaf->GetBoundBVH()->GetMarginV());
	//return geomBoxes::TestPolygonToOrientedBoxFP(RCC_VECTOR3(tm->m_vertices1[0]),RCC_VECTOR3(tm->m_vertices1[1]),RCC_VECTOR3(tm->m_vertices1[2]),RCC_VECTOR3(tm->m_PolygonNormal),RCC_VECTOR3(boxCenter),RCC_MATRIX34(boxOrientation),RCC_VECTOR3(expandedBoxHalfExtents));
	return COT_TestPolygonToOrientedBoxFP(tm->m_vertices1[0],tm->m_vertices1[1],tm->m_vertices1[2],tm->m_PolygonNormal,boxCenter,boxOrientation,expandedBoxHalfExtents);
}

COLLIDE_INLINE void PrimitiveTriangle::Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult &pointCollector)
{
	CollideTemplate<PrimitiveTriangle>(this,boxOrientation,boxHalfExtents,collisionInput,/*phConvexIntersector::PDSOLVERTYPE_TRIANGLE,*/pointCollector);
}

void PrimitiveSphere::Init(const phPrimitive & bvhPrimitive, const int curPrimIndex)
{
	const phBoundBVH * boundBVH = m_leaf->GetBoundBVH();
	const phPrimSphere & spherePrim = bvhPrimitive.GetSphere();
	const int sphereCenterIndex = spherePrim.GetCenterIndex();
	FetchVertices1(boundBVH, sphereCenterIndex, localSpherePosition);
	m_radius = spherePrim.GetRadius();
	phBoundSphere * boundSphere = PRIM_NEW(phBoundSphere)(m_radius);
	boundSphere->SetIndexInBound(curPrimIndex);
}

PHYSICS_FORCE_INLINE void PrimitiveSphere::Setup(const NewCollisionInput & collisionInput)
{
	FastAssert(IsSetup() == false);

	phBoundSphere * boundSphere = GetSphere();

	const BVHInput * bvhInput_ = collisionInput.m_bvhInput;
	const Mat34V currentMatrix = bvhInput_->GetCurrentMatrix();
	const Mat34V lastMatrix = bvhInput_->GetLastMatrix();
	const int component = bvhInput_->GetComponent();

	collisionObject.m_current.Set3x3(currentMatrix);
	collisionObject.m_current.SetCol3(Transform(currentMatrix,localSpherePosition));
#if USE_NEWER_COLLISION_OBJECT
	// These translation calculations have been optimized. In particular, they don't need to explicitly compute collisionObject.m_last. Also, cg offset is zero.
	// Original formula: trans = ComputeLinearMotion_(collisionObject->m_last,collisionObject->m_current,bound.GetCGOffset());
#if BVH_PRIMS_HAVE_ZERO_CG_OFFSET
	FastAssert(IsEqualAll(boundSphere->GetCGOffset(),Vec3V(V_ZERO)));
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,localSpherePosition);
#else
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,localSpherePosition+boundSphere->GetCGOffset());
#endif
#else
	collisionObject.m_last.Set3x3(lastMatrix);
	collisionObject.m_last.SetCol3(Transform(lastMatrix,localSpherePosition));
#endif
	collisionObject.set(boundSphere,component);
}

PHYSICS_FORCE_INLINE void PrimitiveSphere::ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	(void)collisionInput;
	FastAssert(pointCollector.GetHasResult());
	// The local point needs to move too (moved by local space offset)
	pointCollector.SetPointOnBInLocal(pointCollector.GetPointOnBInLocal() + localSpherePosition);
}

TESTOVERLAP_INLINE int PrimitiveSphere::TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents)
{
	const Vec3V sphereCenter_BS = UnTransformOrtho(boxOrientation, localSpherePosition);
	const ScalarV radiusV(m_radius);
	return COT_TestSphereToAABB(sphereCenter_BS, radiusV, Negate(boxHalfExtents), boxHalfExtents);
}

COLLIDE_INLINE void PrimitiveSphere::Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	CollideTemplate<PrimitiveSphere>(this,boxOrientation,boxHalfExtents,collisionInput,/*phConvexIntersector::PDSOLVERTYPE_MINKOWSKI,*/pointCollector);
}

void PrimitiveCapsule::Init(const phPrimitive & bvhPrimitive, const int curPrimIndex)
{
	const phBoundBVH * boundBVH = m_leaf->GetBoundBVH();
	const phPrimCapsule & capsulePrim = bvhPrimitive.GetCapsule();
	const int capsuleEndIndex0 = capsulePrim.GetEndIndex0();
	const int capsuleEndIndex1 = capsulePrim.GetEndIndex1();
	FetchVertices2(boundBVH, capsuleEndIndex0, capsuleEndIndex1, capsuleEnd0, capsuleEnd1);
	m_radius = capsulePrim.GetRadius();

	phBoundCapsule * boundCapsule = PRIM_NEW(phBoundCapsule);
	boundCapsule->SetIndexInBound(curPrimIndex);
}

PHYSICS_FORCE_INLINE void PrimitiveCapsule::Setup(const NewCollisionInput & collisionInput)
{
	FastAssert(IsSetup() == false);

	const Vec3V capsuleShaft(capsuleEnd1 - capsuleEnd0);
	const ScalarV capsuleLength = MagFast(capsuleShaft);
	const ScalarV capsuleRadius(m_radius);

	phBoundCapsule * boundCapsule = GetCapsule();
	boundCapsule->SetCapsuleSize(capsuleRadius, capsuleLength);

	// Create a matrix to orient the capsule in the local space of the bound.
	const VecBoolV maskY = VecBoolV(V_F_T_F_F);
	const Vec3V result = SelectFT(maskY, capsuleShaft, Vec3V(V_ZERO));
	if(!IsZeroAll(result))
	{
		// The shaft axis is not parallel to the y-axis so let's create a matrix whose y-axis is aligned with the shaft.
		localCapsuleMatrix.SetCol0(Normalize(Cross(capsuleShaft, Vec3V(V_Y_AXIS_WZERO))));
		localCapsuleMatrix.SetCol1(Normalize(capsuleShaft));
		localCapsuleMatrix.SetCol2(Cross(localCapsuleMatrix.GetCol0(), localCapsuleMatrix.GetCol1()));
	}
	else
		localCapsuleMatrix.Set3x3(Mat33V(V_IDENTITY));
	localCapsuleMatrix.SetCol3(Average(capsuleEnd0, capsuleEnd1));

	const BVHInput * bvhInput_ = collisionInput.m_bvhInput;
	const Mat34V currentMatrix = bvhInput_->GetCurrentMatrix();
	const Mat34V lastMatrix = bvhInput_->GetLastMatrix();
	const int component = bvhInput_->GetComponent();

	Transform(collisionObject.m_current,currentMatrix,localCapsuleMatrix);
#if USE_NEWER_COLLISION_OBJECT
	// These translation calculations have been optimized. In particular, they don't need to explicitly compute collisionObject.m_last. Also, cg offset is zero.
	// Original formula: trans = ComputeLinearMotion_(collisionObject->m_last,collisionObject->m_current,bound.GetCGOffset());
#if BVH_PRIMS_HAVE_ZERO_CG_OFFSET
	FastAssert(IsEqualAll(boundCapsule->GetCGOffset(),Vec3V(V_ZERO)));
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,localCapsuleMatrix.GetCol3());
#else
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,Transform(localCapsuleMatrix,boundCapsule->GetCGOffset()));
#endif
#else
	Transform(collisionObject.m_last,lastMatrix,localCapsuleMatrix);
#endif
	collisionObject.set(boundCapsule,component);
}

PHYSICS_FORCE_INLINE void PrimitiveCapsule::ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	(void)collisionInput;
	FastAssert(pointCollector.GetHasResult());
	// The local point needs to move too (moved by local space matrix we relocated by earlier)
	pointCollector.SetPointOnBInLocal(Transform(localCapsuleMatrix, pointCollector.GetPointOnBInLocal()));
}

TESTOVERLAP_INLINE int PrimitiveCapsule::TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents)
{
	const Vec3V capsuleEnd0_BS = UnTransformOrtho(boxOrientation, capsuleEnd0);
	const Vec3V capsuleEnd1_BS = UnTransformOrtho(boxOrientation, capsuleEnd1);
	const ScalarV capsuleRadius(m_radius);
	//return geomBoxes::TestCapsuleToAlignedBoxFP(capsuleEnd0_BS.GetIntrin128(), capsuleEnd1_BS.GetIntrin128(), capsuleRadius.GetIntrin128(), Negate(boxHalfExtents).GetIntrin128(), boxHalfExtents.GetIntrin128());
	return COT_TestCapsuleToAlignedBoxFP(capsuleEnd0_BS, capsuleEnd1_BS, capsuleRadius, Negate(boxHalfExtents), boxHalfExtents);
}

COLLIDE_INLINE void PrimitiveCapsule::Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	CollideTemplate<PrimitiveCapsule>(this,boxOrientation,boxHalfExtents,collisionInput,/*phConvexIntersector::PDSOLVERTYPE_MINKOWSKI,*/pointCollector);
}

void PrimitiveBox::Init(const phPrimitive & bvhPrimitive, const int curPrimIndex)
{
	const phBoundBVH * boundBVH = m_leaf->GetBoundBVH();

	const phPrimBox &boxPrim = bvhPrimitive.GetBox();
	const int boxVertIndex0 = boxPrim.GetVertexIndex(0);
	const int boxVertIndex1 = boxPrim.GetVertexIndex(1);
	const int boxVertIndex2 = boxPrim.GetVertexIndex(2);
	const int boxVertIndex3 = boxPrim.GetVertexIndex(3);

	Vec3V boxVert0, boxVert1, boxVert2, boxVert3;
	FetchVertices4(boundBVH, boxVertIndex0, boxVertIndex1, boxVertIndex2, boxVertIndex3, boxVert0, boxVert1, boxVert2, boxVert3);

	// Create a matrix to orient the box in the local space of the bound.
	ScalarV maxMargin;
	geomBoxes::ComputeBoxDataFromOppositeDiagonals(boxVert0, boxVert1, boxVert2, boxVert3, localBoxMatrix, boxSize, maxMargin);

	const ScalarV margin = boundBVH->GetMarginV();
	const ScalarV finalMargin = Min(maxMargin, margin);

	phBoundBox * boxBound = PRIM_NEW(phBoundBox)(RCC_VECTOR3(boxSize));
	boxBound->SetMargin(finalMargin);
	boxBound->SetIndexInBound(curPrimIndex);
}

PHYSICS_FORCE_INLINE void PrimitiveBox::Setup(const NewCollisionInput & collisionInput)
{
	phBoundBox * boxBound = GetBox();

	const BVHInput * bvhInput_ = collisionInput.m_bvhInput;
	const Mat34V currentMatrix = bvhInput_->GetCurrentMatrix();
	const Mat34V lastMatrix = bvhInput_->GetLastMatrix();
	const int component = bvhInput_->GetComponent();

	Transform(collisionObject.m_current,currentMatrix,localBoxMatrix);
#if USE_NEWER_COLLISION_OBJECT
	// These translation calculations have been optimized. In particular, they don't need to explicitly compute collisionObject.m_last. Also, cg offset is zero.
	// Original formula: trans = ComputeLinearMotion_(collisionObject->m_last,collisionObject->m_current,bound.GetCGOffset());
#if BVH_PRIMS_HAVE_ZERO_CG_OFFSET
	FastAssert(IsEqualAll(boxBound->GetCGOffset(),Vec3V(V_ZERO)));
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,localBoxMatrix.GetCol3());
#else
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,Transform(localBoxMatrix,boxBound->GetCGOffset()));
#endif
#else
	Transform(collisionObject.m_last,lastMatrix,localBoxMatrix);
#endif
	collisionObject.set(boxBound,component);
}

PHYSICS_FORCE_INLINE void PrimitiveBox::ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	(void)collisionInput;
	FastAssert(pointCollector.GetHasResult());
	// The local point needs to move too (moved by local space matrix we relocated by earlier)
	pointCollector.SetPointOnBInLocal(Transform(localBoxMatrix, pointCollector.GetPointOnBInLocal()));
}

TESTOVERLAP_INLINE int PrimitiveBox::TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents)
{
	Mat34V relativeMatrix;
	UnTransformOrtho(relativeMatrix, localBoxMatrix, boxOrientation);
	// It's lame to have to have to make the w-component zero like this.
	const Vec4V boxHalfSize(Scale(ScalarV(V_HALF), boxSize), ScalarV(V_ZERO));
	return COT_TestBoxToBoxOBBFaces(boxHalfExtents, boxHalfSize.GetXYZ(), relativeMatrix);
}

COLLIDE_INLINE void PrimitiveBox::Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	CollideTemplate<PrimitiveBox>(this,boxOrientation,boxHalfExtents,collisionInput,/*phConvexIntersector::PDSOLVERTYPE_MINKOWSKI,*/pointCollector);
}

void PrimitiveCylinder::Init(const phPrimitive & bvhPrimitive, const int curPrimIndex)
{
	const phBoundBVH * boundBVH = m_leaf->GetBoundBVH();

	const phPrimCylinder & cylinderPrim = bvhPrimitive.GetCylinder();
	
	const int cylinderEndIndex0 = cylinderPrim.GetEndIndex0();
	const int cylinderEndIndex1 = cylinderPrim.GetEndIndex1();
	FetchVertices2(boundBVH, cylinderEndIndex0, cylinderEndIndex1, cylinderEnd0, cylinderEnd1);
	m_radius = cylinderPrim.GetRadius();

	phBoundCylinder * cylinderBound = PRIM_NEW(phBoundCylinder);
	cylinderBound->SetIndexInBound(curPrimIndex);
}

PHYSICS_FORCE_INLINE void PrimitiveCylinder::Setup(const NewCollisionInput & collisionInput)
{
	const Vec3V cylinderShaft(cylinderEnd1 - cylinderEnd0);
	const ScalarV cylinderLength = MagFast(cylinderShaft);
	const ScalarV cylinderRadius(m_radius);
	const ScalarV margin = m_leaf->GetBoundBVH()->GetMarginV();

	phBoundCylinder * cylinderBound = GetCylinder();

	cylinderBound->SetMargin(margin);
	const ScalarV vsHalf(V_HALF);
	cylinderBound->SetCylinderRadiusAndHalfHeight(cylinderRadius, vsHalf * cylinderLength);

	// Create a matrix to orient the cylinder in the local space of the bound.
	const VecBoolV maskY = VecBoolV(V_F_T_F_F);
	const Vec3V result = SelectFT(maskY, cylinderShaft, Vec3V(V_ZERO));
	if(!IsZeroAll(result))
	{
		// The shaft axis is not parallel to the y-axis so let's create a matrix whose y-axis is aligned with the shaft.
		localCylinderMatrix.SetCol0(Normalize(Cross(cylinderShaft, Vec3V(V_Y_AXIS_WZERO))));
		localCylinderMatrix.SetCol1(Normalize(cylinderShaft));
		localCylinderMatrix.SetCol2(Cross(localCylinderMatrix.GetCol0(), localCylinderMatrix.GetCol1()));
	}
	else
		localCylinderMatrix.Set3x3(Mat33V(V_IDENTITY));
	localCylinderMatrix.SetCol3(Average(cylinderEnd0, cylinderEnd1));

	const BVHInput * bvhInput_ = collisionInput.m_bvhInput;
	const Mat34V currentMatrix = bvhInput_->GetCurrentMatrix();
	const Mat34V lastMatrix = bvhInput_->GetLastMatrix();
	const int component = bvhInput_->GetComponent();

	Transform(collisionObject.m_current,currentMatrix,localCylinderMatrix);
#if USE_NEWER_COLLISION_OBJECT
	// These translation calculations have been optimized. In particular, they don't need to explicitly compute collisionObject.m_last. Also, cg offset is zero.
	// Original formula: trans = ComputeLinearMotion_(collisionObject->m_last,collisionObject->m_current,bound.GetCGOffset());
#if BVH_PRIMS_HAVE_ZERO_CG_OFFSET
	FastAssert(IsEqualAll(cylinderBound->GetCGOffset(),Vec3V(V_ZERO)));
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,localCylinderMatrix.GetCol3());
#else
	collisionObject.m_trans = ComputeLinearMotion_(lastMatrix,currentMatrix,Transform(localCylinderMatrix,cylinderBound->GetCGOffset()));
#endif
#else
	Transform(collisionObject.m_last,lastMatrix,localCylinderMatrix);
#endif
	collisionObject.set(cylinderBound,component);
}

PHYSICS_FORCE_INLINE void PrimitiveCylinder::ProcessResult(const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	(void)collisionInput;
	FastAssert(pointCollector.GetHasResult());
	// The local point needs to move too (moved by local space matrix we relocated by earlier)
	pointCollector.SetPointOnBInLocal(Transform(localCylinderMatrix, pointCollector.GetPointOnBInLocal()));
}

TESTOVERLAP_INLINE int PrimitiveCylinder::TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents)
{
	const Vec3V capsuleEnd0_BS = UnTransformOrtho(boxOrientation, cylinderEnd0);
	const Vec3V capsuleEnd1_BS = UnTransformOrtho(boxOrientation, cylinderEnd1);
	const ScalarV capsuleRadius(m_radius);
	//return geomBoxes::TestCapsuleToAlignedBoxFP(capsuleEnd0_BS.GetIntrin128(), capsuleEnd1_BS.GetIntrin128(), capsuleRadius.GetIntrin128(), Negate(boxHalfExtents).GetIntrin128(), boxHalfExtents.GetIntrin128());
	return COT_TestCapsuleToAlignedBoxFP(capsuleEnd0_BS, capsuleEnd1_BS, capsuleRadius, Negate(boxHalfExtents), boxHalfExtents);
}

COLLIDE_INLINE void PrimitiveCylinder::Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	CollideTemplate<PrimitiveCylinder>(this,boxOrientation,boxHalfExtents,collisionInput,/*phConvexIntersector::PDSOLVERTYPE_MINKOWSKI,*/pointCollector);
}

#if !PRIM_CACHE_TESTOVERLAP_VIRTUAL && USE_PRIMITIVE_CULLING
int PrimitiveBase::TestOverlap(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents)
{
	switch(m_primType)
	{
		#undef PRIMITIVE_TYPE_INC
		#define PRIMITIVE_TYPE_INC(className,enumLabel) case enumLabel: \
			return ((className*)this)->TestOverlap(boxOrientation,boxHalfExtents); 
		#include "PrimitiveTypes.inc"
		#undef PRIMITIVE_TYPE_INC

		default:
			FastAssert(0);
			return 0;
	}
}
#endif // !PRIM_CACHE_TESTOVERLAP_VIRTUAL

#if !PRIM_CACHE_COLLIDE_VIRTUAL
void PrimitiveBase::Collide(Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult & pointCollector)
{
	switch(m_primType)
	{
		#undef PRIMITIVE_TYPE_INC
		#define PRIMITIVE_TYPE_INC(className,enumLabel) case enumLabel: \
			((className*)this)->Collide(boxOrientation,boxHalfExtents,collisionInput,pointCollector); \
			break;
		#include "PrimitiveTypes.inc"
		#undef PRIMITIVE_TYPE_INC

		default:
			FastAssert(0);
	}
}
#endif // !PRIM_CACHE_COLLIDE_VIRTUAL

void CollideLeaf(PrimitiveLeaf * leaf, Mat34V_In boxOrientation, Vec3V_In boxHalfExtents, const NewCollisionInput & collisionInput, DiscreteCollisionDetectorInterface::SimpleResult &pointCollector)
{
	for (PrimitiveBase * prim = leaf->m_first ; prim ; prim = prim->m_next)
		prim->Collide(boxOrientation,boxHalfExtents,collisionInput,pointCollector);
}

#define USE_RAWRESULT_LIST 1

#if !USE_NEW_MID_PHASE || ALLOW_MID_PHASE_SWAP
__forceinline void InitCollisionObject(NewCollisionObject * co, Mat34V_In last, Mat34V_In current, const phBound * bound, const int component)
{
	co->m_current = current;
#if USE_NEWER_COLLISION_OBJECT
	co->m_trans = ComputeLinearMotion_(last,current,bound);
#else
	co->m_last = last;
#endif
	co->set(bound,component);
}

void InitCollisionObject(NewCollisionObject * co, const ObjectState * objectState)
{
	const phBound * bound = objectState->GetCurCullingBound();
	const int component = objectState->GetComponentIndex();
#if !MIDPHASE_USES_QUATS
	InitCollisionObject(co,objectState->GetLastBoundToWorldMatrix(),objectState->GetCurBoundToWorldMatrix(),bound,component);
#else	// !MIDPHASE_USES_QUATS
	Mat34V current;
	Mat34V last;
	Mat34VFromQuatV(current, objectState->GetCurBoundToWorldOrientation(), objectState->GetCurBoundToWorldOffset());
	Mat34VFromQuatV(last, objectState->GetLastBoundToWorldOrientation(), objectState->GetLastBoundToWorldOffset());
	InitCollisionObject(co,last,current,bound,component);
#endif	// !MIDPHASE_USES_QUATS
}

void ProcessResult(const ObjectState &manifoldObjectState0, const ObjectState &manifoldObjectState1, 
				   DiscreteCollisionDetectorInterface::SimpleResult & rawResult, phManifoldResult & manifoldResult)
{
#if !MIDPHASE_USES_QUATS
	manifoldResult.ProcessResult(manifoldObjectState0.GetCurBoundToWorldMatrix(), manifoldObjectState1.GetCurBoundToWorldMatrix(), rawResult);
#else	// !MIDPHASE_USES_QUATS
	manifoldResult.ProcessResult(manifoldObjectState0.GetCurBoundToWorldOrientation(), manifoldObjectState0.GetCurBoundToWorldOffset(), manifoldObjectState1.GetCurBoundToWorldOrientation(), manifoldObjectState1.GetCurBoundToWorldOffset(), rawResult);
#endif	// !MIDPHASE_USES_QUATS
}

// Once we're done with all our fancy culling we're left with two 'leaves'.  A 'leaf' is something that is indivisible from perspective of the midphase, but may
//   not be directly collidable.  Right now the only kinds of leaves are actual bounds and BVH nodes.  ProcessLeafCollision() is where we handle what to do with
//   those.
void phMidphase::ProcessLeafCollision(const ObjectState &manifoldObjectState0, const ObjectState &manifoldObjectState1, ProcessLeafCollisionManifold *virtualManifold, GJKCacheQueryInput * gjk_cache_info, phCollisionMemory * collisionMemory) const
{
	PF_FUNC_2(ProcessLeafCollision);
	PF_INCREMENT_2(ProcessLeafCollisionCount);

	const phBound *manifoldBound0 = manifoldObjectState0.GetCurCullingBound();
	const phBound *manifoldBound1 = manifoldObjectState1.GetCurCullingBound();
	const size_t isSwapped = ~GenerateMaskCondition(manifoldBound0->GetType() - phBound::BVH);
	const ObjectState * RESTRICT objectState0 = ISelectP(isSwapped,&manifoldObjectState0,&manifoldObjectState1);
	const ObjectState * RESTRICT objectState1 = ISelectP(isSwapped,&manifoldObjectState1,&manifoldObjectState0);

	const phBound *curCullingBound0 = objectState0->GetCurCullingBound();
	const phBound *curCullingBound1 = objectState1->GetCurCullingBound();

	NewCollisionObject collisionObject0;
	InitCollisionObject(&collisionObject0,objectState0);
	NewCollisionObject collisionObject1;
	InitCollisionObject(&collisionObject1,objectState1);

#if USE_RAWRESULT_LIST

	const int MAX_RAWRESULT_COUNT = 4;
	DiscreteCollisionDetectorInterface::SimpleResult rawResultList[MAX_RAWRESULT_COUNT];
	int rawResultListCount = 0;

#else // USE_RAWRESULT_LIST

	phManifoldResult manifoldResult(manifoldBound0, manifoldBound1, NULL);
	manifoldResult.SetReverseInManifold((u32)isSwapped);		// This is used as a select mask, so it really should be a u32 and not a size_t
	DiscreteCollisionDetectorInterface::SimpleResult rawResult;

#endif // USE_RAWRESULT_LIST

	const int boundType0 = curCullingBound0->GetType();
	const int boundType1 = curCullingBound1->GetType();
	FastAssert(boundType0 != phBound::BVH || boundType1 == phBound::BVH);
	FastAssert(boundType0 != phBound::COMPOSITE && boundType1 != phBound::COMPOSITE);

	// Create a new collisionInput.
	NewCollisionInput newInput;
	newInput.set(phManifold::GetManifoldMarginV(), &collisionObject0, NULL, gjk_cache_info, NULL);

	if(boundType1 != phBound::BVH)
	{
		newInput.object1 = &collisionObject1;

		// When we just have two 'bounds' then this is pretty simple.
		gjk_cache_info->SetPartIndex(0);

#if USE_RAWRESULT_LIST
		
		FastAssert(rawResultListCount == 0);
		phPairwiseCollisionProcessor::ProcessPairwiseCollision(rawResultList[0],newInput);
		if (rawResultList[0].GetHasResult())
			rawResultListCount = 1;

#else // USE_RAWRESULT_LIST

		collisionProcessor.ProcessBoundVsBound(rawResult, newInput);
		// This is waiting on existing manifold contacts - And we don't care if there is no result, we still want to let them finish here for the next iteration to not get stomped
		SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(1)));
		if(rawResult.GetHasResult())
		{
			manifoldResult.SetManifold(virtualManifold->GetManifold(manifoldBound0,manifoldBound1));
			if (Unlikely(virtualManifold->OutOfMemory()))
				return;
			Assert(manifoldResult.GetManifold());
			ProcessResult(manifoldObjectState0,manifoldObjectState1,rawResult,manifoldResult);
		}

#endif // USE_RAWRESULT_LIST
	}
	else
	{
		if(Verifyf(boundType0 != phBound::BVH, "Trying to collide a phBoundBVH against a phBoundBVH - this is not allowed."))
		{
			// BVH node vs something else.
			const phBoundBVH *boundBVH = static_cast<const phBoundBVH *>(curCullingBound1);
			Assert(objectState1->GetStartIndex() == objectState1->GetEndIndex() - 1);
			const int curNodeIndex = objectState1->GetStartIndex();
			const phOptimizedBvhNode &curNode = objectState1->GetBvhNode(curNodeIndex);

			newInput.m_bvhInput = &collisionObject1;

			// These get used for each primitive type below and don't change per primitive.
			Mat34V boxOrientation;
#if !MIDPHASE_USES_QUATS
			UnTransformOrtho(boxOrientation, objectState1->GetCurBoundToWorldMatrix(), objectState0->GetCurBoundingBoxMatrix());
#else	// !MIDPHASE_USES_QUATS
			QuatV boxRelativeOrientation;
			Vec3V boxRelativePosition;
			UnTransform(boxRelativeOrientation, boxRelativePosition, objectState1->GetCurBoundToWorldOrientation(), objectState1->GetCurBoundToWorldOffset(), objectState0->GetCurBoundToWorldOrientation(), objectState0->GetCurBoundingBoxCenter());

			Mat34VFromQuatV(boxOrientation, boxRelativeOrientation, boxRelativePosition);
#endif	// !MIDPHASE_USES_QUATS

			Vec3V boxHalfExtents = objectState0->GetCurBoundingBoxHalfSize();
			boxHalfExtents.SetWZero();	// It's lame to have to do this.

			PF_INCREMENT_2(BvhLeafCount);
			PPC_STAT_COUNTER_INC(BvhNodeLeafCounter,1);

			phManifold * rootManifold = virtualManifold->GetRootManifold();
#if __ASSERT
			phInst* pInstOfBvh = rootManifold->GetInstance(manifoldBound0->GetType()==phBound::BVH?0:1);
#endif // __ASSERT

			u32 levelIndex = ISelectI((u32)isSwapped,rootManifold->GetLevelIndexB(),rootManifold->GetLevelIndexA());
			u16 component = objectState1->GetComponentIndex();
			PrimitiveLeaf * leaf = collisionMemory->m_primCache.GetLeaf(boundBVH,&curNode,levelIndex,component CHECK_TRI_ONLY(pInstOfBvh));

#if PRIM_CACHE_RENDER
			g_PrimitiveRender.Render(leaf,objectState1->GetCurBoundToWorldMatrix(),levelIndex,component,curNode.GetPolygonStartIndex());
#endif
			// gjk_cache_info->PartIndex is set inside of prim->Collide.
			for (PrimitiveBase * prim = leaf->m_first ; prim ; prim = prim->m_next)
			{
#if USE_RAWRESULT_LIST

				FastAssert(rawResultListCount < MAX_RAWRESULT_COUNT);
				DiscreteCollisionDetectorInterface::SimpleResult & rawResult = rawResultList[rawResultListCount];
				
#endif // USE_RAWRESULT_LIST

				prim->Collide(boxOrientation,boxHalfExtents,newInput,rawResult);

#if USE_RAWRESULT_LIST

				if (rawResult.GetHasResult())
					rawResultListCount++;

#else // USE_RAWRESULT_LIST

				// This is waiting on existing manifold contacts - And we don't care if there is no result, we still want to let them finish here for the next iteration to not get stomped
				SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(1)));
				if(rawResult.GetHasResult())
				{
					manifoldResult.SetManifold(virtualManifold->GetManifold(manifoldBound0,manifoldBound1));
					if (Unlikely(virtualManifold->OutOfMemory()))
						return;
					Assert(manifoldResult.GetManifold());
					ProcessResult(manifoldObjectState0,manifoldObjectState1,rawResult,manifoldResult);
					rawResult.SetHasResult(false);
				}

#endif // USE_RAWRESULT_LIST
			}
		}
	}

#if USE_RAWRESULT_LIST

	if (rawResultListCount > 0)
	{
		phManifold * manifold = virtualManifold->GetManifold(manifoldBound0,manifoldBound1);
		if (Unlikely(virtualManifold->OutOfMemory()))
			return;
		FastAssert(manifold);
		phManifoldResult manifoldResult(manifoldBound0, manifoldBound1, manifold);
		manifoldResult.SetReverseInManifold((u32)isSwapped);		// This is used as a select mask, so it really should be a u32 and not a size_t

		// This is waiting on existing manifold contacts - And we don't care if there is no result, we still want to let them finish here for the next iteration to not get stomped
		// This probably isn't needed.
		SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(1)));

		for (int i = 0 ; i < rawResultListCount ; i++)
		{
			FastAssert(rawResultList[i].GetHasResult());
			ProcessResult(manifoldObjectState0,manifoldObjectState1,rawResultList[i],manifoldResult);
		}
		Assert(manifold->GetNumContacts() > 0);
	}

#endif // USE_RAWRESULT_LIST
}
#endif // !USE_NEW_MID_PHASE || ALLOW_MID_PHASE_SWAP

#if USE_NEW_MID_PHASE || USE_NEW_SELF_COLLISION

#define DONT_USE_COMPONENT_CACHE 0
#define USE_ALT_INTX 1

#if __SPU
	#define CI_SPU_ONLY(x) ,x
#else // __SPU
	#define CI_SPU_ONLY(x) 
#endif // __SPU

#if USE_NEWER_COLLISION_OBJECT

void ComputeBoundingVolume(Mat34V_In current, Mat34V_In last, const phBound * bound, Vec3V_InOut bbHalfWidth_Out, Vec3V_InOut bbCenterAbs_Out, Vec3V_InOut bbTrans_Out)
{
	const ScalarV v_half(V_HALF);

	const Vec3V AABBMinLoc = bound->GetBoundingBoxMin();
	const Vec3V AABBMaxLoc = bound->GetBoundingBoxMax();

	Vec3V bbHalfWidth = v_half * (AABBMaxLoc - AABBMinLoc);
	Vec3V bbCenter = v_half * (AABBMaxLoc + AABBMinLoc);
	Vec3V bbTrans;

	// Move and expand the matrix for the composite bound part's bounding box to include both this and the previous frame.
	if (COT_IsNonSimpleBoundType(bound->GetType()))
	{
		bbTrans = Vec3V(V_ZERO);
		COT_ExpandNonSimpleBoundOBBFromMotion(current,last,bbHalfWidth,bbCenter,bbHalfWidth,bbCenter);
	}
	else
	{
		bbTrans = ComputeLinearMotion_(last,current,bound);
		COT_ExpandSimpleBoundOBBFromMotion(current,bbTrans,bbHalfWidth,bbCenter);
	}

	// W has to be zero for TestBoxToBoxOBB.
	bbHalfWidth.SetWZero();

	// Calc the global coordinate position of the bounding box center.
	const Vec3V bbCenterAbs = Transform(current,bbCenter);

	bbHalfWidth_Out = bbHalfWidth;
	bbCenterAbs_Out = bbCenterAbs;
	bbTrans_Out = bbTrans;
}

#else // USE_NEWER_COLLISION_OBJECT

void ComputeBoundingVolume(Mat34V_In current, Mat34V_In last, const phBound * bound, Vec3V_InOut bbHalfWidth_Out, Vec3V_InOut bbCenterAbs_Out)
{
	const ScalarV v_half(V_HALF);

	const Vec3V AABBMinLoc = bound->GetBoundingBoxMin();
	const Vec3V AABBMaxLoc = bound->GetBoundingBoxMax();

	Vec3V bbHalfWidth = v_half * (AABBMaxLoc - AABBMinLoc);
	Vec3V bbCenter = v_half * (AABBMaxLoc + AABBMinLoc);

	// Move and expand the matrix for the composite bound part's bounding box to include both this and the previous frame.
	COT_ExpandBoundOBBFromMotion(current,last,bound,bbHalfWidth,bbCenter);

	// W has to be zero for TestBoxToBoxOBB.
	bbHalfWidth.SetWZero();

	// Calc the global coordinate position of the bounding box center.
	const Vec3V bbCenterAbs = Transform(current,bbCenter);

	bbHalfWidth_Out = bbHalfWidth;
	bbCenterAbs_Out = bbCenterAbs;
}

#endif // USE_NEWER_COLLISION_OBJECT

class ComponentInfo
{
public:

#if __SPU
	typedef phSpuCollisionBuffers ComponentAllocator_t;
#else
	typedef FrameAllocator<1> ComponentAllocator_t;
#endif

	bool Init(const int component, Mat34V_In currentInstanceMatrix, Mat34V_In lastInstanceMatrix, const phBoundComposite * boundComposite CI_SPU_ONLY(ComponentAllocator_t * componentAllocator) CI_SPU_ONLY(const bool DMAStuff) CI_SPU_ONLY(u8 * boundBuffer));

	__forceinline bool IsBVH() const { return (m_object.m_bound->GetType() == phBound::BVH); }
	__forceinline bool IsComposite() const { return (m_object.m_bound->GetType() == phBound::COMPOSITE); }
	__forceinline bool IsGeometry() const { return (m_object.m_bound->GetType() == phBound::GEOMETRY); }
	__forceinline bool IsSimpleBound() const { return (COT_IsNonSimpleBoundType(m_object.m_bound->GetType()) == 0); }

	__forceinline const phBoundBVH * GetBoundBVH() const { FastAssert(IsBVH()); return reinterpret_cast<const phBoundBVH *>(m_object.m_bound); }
	__forceinline const phBoundComposite * GetBoundComposite() const { FastAssert(IsComposite()); return reinterpret_cast<const phBoundComposite *>(m_object.m_bound); }
	__forceinline const phBoundGeometry * GetBoundGeometry() const { FastAssert(IsGeometry()); return reinterpret_cast<const phBoundGeometry *>(m_object.m_bound); }
	__forceinline const phBound * GetBound() const { return m_object.m_bound; }

	__forceinline int GetComponent() const { return m_object.m_component; }
	__forceinline Mat34V_Out GetCurrentMatrix() const { return m_object.m_current; }
	__forceinline Vec3V_Out GetBBCenterAbs() const { return m_bbCenterAbs; }
	__forceinline Vec3V_Out GetBBHalfWidthLoc() const { return m_bbHalfWidth; }

	NewCollisionObject m_object;
	Vec3V m_bbHalfWidth;
	Vec3V m_bbCenterAbs;
};

bool ComponentInfo::Init(const int component, Mat34V_In currentInstanceMatrix, Mat34V_In lastInstanceMatrix, const phBoundComposite * boundComposite CI_SPU_ONLY(ComponentAllocator_t * componentAllocator) CI_SPU_ONLY(const bool DMAStuff) CI_SPU_ONLY(u8 * boundBuffer))
{
#if __SPU
	const int BOUND_TAG_ID = 1;
	sysDmaLargeGet(boundBuffer, (uint64_t)(boundComposite->GetBound(component)), phBound::MAX_BOUND_SIZE, DMA_TAG(BOUND_TAG_ID));
	const phBound * bound = reinterpret_cast<const phBound *>(boundBuffer);
#else // __SPU
	const phBound * bound = boundComposite->GetBound(component);
#endif // __SPU

	m_object.set(bound,component);

	Mat34V currentComponentMatrix;
	Mat34V lastComponentMatrix;

#if __SPU
	if (DMAStuff)
	{
		const int STUFF_TAG_ID = 2;
		sysDmaGet(&currentComponentMatrix,(uint64_t)(boundComposite->GetCurrentMatrices() + component),sizeof(Mat34V),DMA_TAG(STUFF_TAG_ID));
		sysDmaGet(&lastComponentMatrix,(uint64_t)(boundComposite->GetLastMatrices() + component),sizeof(Mat34V),DMA_TAG(STUFF_TAG_ID));
		sysDmaWait(DMA_MASK(STUFF_TAG_ID));
	}
	else
#endif // __SPU
	{
		currentComponentMatrix = boundComposite->GetCurrentMatrix(component);
		lastComponentMatrix = boundComposite->GetLastMatrix(component);
	}

	Mat34V current;	
	Mat34V last;
	Transform(current,currentInstanceMatrix,currentComponentMatrix);

	if (m_object.m_bound->GetUseCurrentInstsanceMatrixOnly())
	{
		Transform(last,currentInstanceMatrix,lastComponentMatrix);
	}
	else
	{
		Transform(last,lastInstanceMatrix,lastComponentMatrix);
	}

#if __SPU
	sysDmaWaitTagStatusAll(DMA_MASK(BOUND_TAG_ID));
	if (m_object.m_bound->GetType() == phBound::GEOMETRY)
	{
		phBoundGeometry * geomBound = const_cast<phBoundGeometry *>(static_cast<const phBoundGeometry *>(bound));
		const bool success = DmaBoundGeometry(geomBound,componentAllocator);
		DmaBoundGeometryWait();
		if (Unlikely(success == false))
			return false;
	}
#endif // __SPU

	m_object.m_current = current;
#if USE_NEWER_COLLISION_OBJECT
	ComputeBoundingVolume(current,last,bound,m_bbHalfWidth,m_bbCenterAbs,m_object.m_trans);
#else // USE_NEWER_COLLISION_OBJECT
	m_object.m_last = last;
	ComputeBoundingVolume(current,last,bound,m_bbHalfWidth,m_bbCenterAbs);
#endif // USE_NEWER_COLLISION_OBJECT

	return true;
}

struct ComponentInfoWithBoundBuffer : public ComponentInfo
{
#if __SPU
	ALIGNAS(16) u8 m_boundBuffer[phBound::MAX_BOUND_SIZE] ;	// This is for the bound itself.
#endif // __SPU
};

struct ComponentInfoManager
{
#if __SPU
	#if !__ASSERT
		enum { COMPONENT_BUFFER_SIZE = 1024 * 50 };
	#else 
		enum { COMPONENT_BUFFER_SIZE = 1024 * 22 };
	#endif
#else // __SPU
	enum { COMPONENT_BUFFER_SIZE = 1024 * 22 };
#endif // __SPU

	ALIGNAS(16) u8 m_componentBuffer[COMPONENT_BUFFER_SIZE] ;	// This is for the bound itself.
	
	ComponentInfo::ComponentAllocator_t m_componentAllocator;

	enum { MAX_NUM_COMPONENTS = 256 };
	ComponentInfo * m_componentInfoTable[MAX_NUM_COMPONENTS];
	int m_componentInfoTableSize;

	ComponentInfoManager() : m_componentAllocator(m_componentBuffer,COMPONENT_BUFFER_SIZE)
	{
		m_componentInfoTableSize = 0;
	}

	__forceinline ComponentInfo * GetComponentInfo(const int component)
	{
		FastAssert(component >= 0 && component < m_componentInfoTableSize);
		return m_componentInfoTable[component];
	}

	__forceinline void SetComponentInfo(const int component, ComponentInfo * ci)
	{
		FastAssert(component >= 0 && component < m_componentInfoTableSize);
		m_componentInfoTable[component] = ci;
	}

	void InitComponentInfoMap(const int componentInfoTableSize)
	{
		FastAssert(componentInfoTableSize >= 0 && componentInfoTableSize <= MAX_NUM_COMPONENTS);
		m_componentInfoTableSize = componentInfoTableSize;
		memset(m_componentInfoTable,0,sizeof(ComponentInfo*) * m_componentInfoTableSize);
	}

	ComponentInfo * CreateComponentInfo(const int component, Mat34V_In curInstanceMatrix, Mat34V_In lastInstanceMatrix, const phBoundComposite * boundComposite CI_SPU_ONLY(const bool DMAStuff));

	void ResetBuffer(const int componentInfoTableSize = -1);
};

void ComponentInfoManager::ResetBuffer(const int componentInfoTableSize)
{
	PPC_STAT_COUNTER_INC(ResetBufferCounter,1);
	//Displayf("Reseting the ComponentInfo Cache.");
	m_componentAllocator.ReleaseToLastMarker();
	m_componentAllocator.SetMarker();
	if (Unlikely(componentInfoTableSize != -1))
	{
		FastAssert(componentInfoTableSize >= 0 && componentInfoTableSize <= MAX_NUM_COMPONENTS);
		m_componentInfoTableSize = componentInfoTableSize;
	}
	FastAssert(m_componentInfoTableSize >= 0 && m_componentInfoTableSize <= MAX_NUM_COMPONENTS);
	memset(m_componentInfoTable,0,sizeof(ComponentInfo*) * m_componentInfoTableSize);
}

ComponentInfo * ComponentInfoManager::CreateComponentInfo(const int component, Mat34V_In curInstanceMatrix, Mat34V_In lastInstanceMatrix, const phBoundComposite * boundComposite CI_SPU_ONLY(const bool DMAStuff))
{
	void * ptr = (void*)m_componentAllocator.GetBlockSafe(sizeof(ComponentInfoWithBoundBuffer));
	if (Unlikely(ptr == NULL))
		return NULL;
	ComponentInfoWithBoundBuffer * ci = new(ptr) ComponentInfoWithBoundBuffer;
	if (Unlikely(ci->Init(component,curInstanceMatrix,lastInstanceMatrix,boundComposite CI_SPU_ONLY(&m_componentAllocator) CI_SPU_ONLY(DMAStuff) CI_SPU_ONLY(ci->m_boundBuffer)) == false))
		return NULL;
	return ci;
}

bool OBBTest(const ComponentInfo & object0, const ComponentInfo & object1)
{
	Mat34V aTobMat;
	UnTransform3x3Ortho(aTobMat,object1.GetCurrentMatrix(),object0.GetCurrentMatrix());
	aTobMat.SetCol3(UnTransform3x3Ortho(object1.GetCurrentMatrix(),object0.GetBBCenterAbs() - object1.GetBBCenterAbs()));
	return COT_TestBoxToBoxOBBFaces(object0.GetBBHalfWidthLoc(),object1.GetBBHalfWidthLoc(),aTobMat);
}

#endif // #if USE_NEW_MID_PHASE || USE_NEW_SELF_COLLISION

#if USE_NEW_MID_PHASE

class MidPhaseIntersector
{
public:

#if __SPU
	struct BvhMemory
	{
		enum { MAX_MATERIAL_IDS = 256 };
		ALIGNAS(16) u8 m_bvhBuffer[sizeof(phOptimizedBvh)] ;
		ALIGNAS(16) u8 m_materialIdBuffer[sizeof(phMaterialMgr::Id) * MAX_MATERIAL_IDS] ;
	};
#endif // __SPU

#define BVH_PARAMS_ERROR_CHECK 0
#if BVH_PARAMS_ERROR_CHECK
	#define BVH_PARAMS_ERROR_CHECK_ONLY(x) x
#else
	#define BVH_PARAMS_ERROR_CHECK_ONLY(x)
#endif 

	struct BvhParams
	{
		Mat34V m_aTobMatrix;
		Vec3V m_aTobPosition;
		Vec3V m_boundAABBMinLoc;
		Vec3V m_boundAABBMaxLoc;
		u32 m_isSwapped;
		u32 m_isSwappedMask;
		BvhParams(u32 isSwapped)
		{
			FastAssert(isSwapped == 0 || isSwapped == 1);
			m_isSwapped = isSwapped;
			m_isSwappedMask = (u32)(-(int)isSwapped);
			FastAssert(m_isSwappedMask == 0 || m_isSwappedMask == 0xFFFFFFFF);
		}
#if USE_CHECK_TRIANGLE
		phInst * m_pInstOfBvh;
#endif // USE_CHECK_TRIANGLE
#if BVH_PARAMS_ERROR_CHECK
		bool m_obbValid;
		bool m_aabbValid;
		bool m_instValid;
		BvhParams()
		{
			m_obbValid = false;
			m_aabbValid = false;
			m_instValid =  false;
		}
#endif // BVH_PARAMS_ERROR_CHECK
	};

	struct BoundInfo : public ComponentInfo
	{
#if USE_NEWER_COLLISION_OBJECT

		Mat34V m_lastMatrix;
#if __ASSERT
		bool m_isLastMatrixValid;
#endif // __ASSERT

#if __ASSERT
		BoundInfo()
		{
			m_isLastMatrixValid = false;
		}
#endif // __ASSERT

		void SetLastMatrix(Mat34V_In lastMatrix)
		{
			m_lastMatrix = lastMatrix;
#if __ASSERT
			m_isLastMatrixValid = true;
#endif // __ASSERT
		}

		void SetLastMatrix(const int component, Mat34V_In lastInstanceMatrix, const phBoundComposite * boundComposite CI_SPU_ONLY(const bool DMAStuff))
		{
			Mat34V lastComponentMatrix;
#if __SPU
			if (DMAStuff)
			{
				const int STUFF_TAG_ID = 2;
				sysDmaGet(&lastComponentMatrix,(uint64_t)(boundComposite->GetLastMatrices() + component),sizeof(Mat34V),DMA_TAG(STUFF_TAG_ID));
				sysDmaWait(DMA_MASK(STUFF_TAG_ID));
			}
			else
#endif // __SPU
			{
				lastComponentMatrix = boundComposite->GetLastMatrix(component);
			}
			Transform( m_lastMatrix, lastInstanceMatrix, lastComponentMatrix );
#if __ASSERT
			m_isLastMatrixValid = true;
#endif // __ASSERT
		}

		Mat34V_Out GetLastMatrix() const
		{
#if __ASSERT
			FastAssert(m_isLastMatrixValid);
#endif // __ASSERT
			return m_lastMatrix;
		}

#else  // USE_NEWER_COLLISION_OBJECT

		Mat34V_Out GetLastMatrix() const
		{
			return m_object.m_last;
		}

#endif // USE_NEWER_COLLISION_OBJECT

		u32 m_typeFlags;
		u32 m_includeFlags;
		u32 m_levelIndex;

		u8 * m_componentList;
		int m_componentListCount;
	};

	__forceinline void InitBvhParamsOBB(BvhParams * CCD_RESTRICT bvhParams, const ComponentInfo * CCD_RESTRICT ci0, const BoundInfo * CCD_RESTRICT bi1)
	{
		BVH_PARAMS_ERROR_CHECK_ONLY(bvhParams->m_obbValid = true);
		FastAssert(bi1->IsBVH());
		UnTransform3x3Ortho(bvhParams->m_aTobMatrix,bi1->m_object.m_current,ci0->m_object.m_current);
		bvhParams->m_aTobPosition = UnTransform3x3Ortho(bi1->m_object.m_current,ci0->GetBBCenterAbs() - bi1->m_object.m_current.GetCol3());
	}

	__forceinline void InitBvhParamsAABB(BvhParams * CCD_RESTRICT bvhParams, const ComponentInfo * CCD_RESTRICT ci0, const BoundInfo * CCD_RESTRICT bi1)
	{
		// Make sure to call InitBvhParamsOBB first
		BVH_PARAMS_ERROR_CHECK_ONLY(FastAssert(bvhParams->m_obbValid));
		BVH_PARAMS_ERROR_CHECK_ONLY(bvhParams->m_aabbValid = true);
		FastAssert(bi1->IsBVH());
		(void)bi1;
		Vec3V halfWidthLoc = geomBoxes::ComputeAABBExtentsFromOBB(bvhParams->m_aTobMatrix.GetMat33ConstRef(),ci0->GetBBHalfWidthLoc());
		bvhParams->m_boundAABBMinLoc = bvhParams->m_aTobPosition - halfWidthLoc;
		bvhParams->m_boundAABBMaxLoc = bvhParams->m_aTobPosition + halfWidthLoc;
	}

#if USE_CHECK_TRIANGLE 
	void InitBvhParamsInst(BvhParams * CCD_RESTRICT bvhParams)
	{
		BVH_PARAMS_ERROR_CHECK_ONLY(bvhParams->m_instValid = true);
		if (bvhParams->m_isSwapped)
			bvhParams->m_pInstOfBvh = m_rootManifold->GetInstanceA();
		else
			bvhParams->m_pInstOfBvh = m_rootManifold->GetInstanceB();
	}
#endif // USE_CHECK_TRIANGLE

	void InitBvhParams(BvhParams * CCD_RESTRICT bvhParams, const ComponentInfo * CCD_RESTRICT ci0, const BoundInfo * CCD_RESTRICT bi1)
	{
		FastAssert(bi1->IsBVH());
		InitBvhParamsOBB(bvhParams,ci0,bi1);
		InitBvhParamsAABB(bvhParams,ci0,bi1);
#if USE_CHECK_TRIANGLE
		InitBvhParamsInst(bvhParams);
#endif // USE_CHECK_TRIANGLE
	}

	void SetBound(BoundInfo * CCD_RESTRICT bi, Mat34V_In boundLastMatrix, Mat34V_In boundCurrentMatrix, const phBound * CCD_RESTRICT bound, const u32 boundTypeFlags, const u32 boundIncludeFlags, const u32 boundLevelIndex, const u16 boundComponent)
	{

#if USE_NEWER_COLLISION_OBJECT
		bi->SetLastMatrix(boundLastMatrix);
#else // USE_NEWER_COLLISION_OBJECT
		bi->m_object.m_last = boundLastMatrix;
#endif // USE_NEWER_COLLISION_OBJECT
		bi->m_object.m_current = boundCurrentMatrix;
		bi->m_object.set(bound,boundComponent);

#if USE_NEWER_COLLISION_OBJECT
		ComputeBoundingVolume(boundCurrentMatrix,boundLastMatrix,bound,bi->m_bbHalfWidth,bi->m_bbCenterAbs,bi->m_object.m_trans);
#else // USE_NEWER_COLLISION_OBJECT
		ComputeBoundingVolume(boundCurrentMatrix,boundLastMatrix,bound,bi->m_bbHalfWidth,bi->m_bbCenterAbs);
#endif // USE_NEWER_COLLISION_OBJECT

		bi->m_typeFlags = boundTypeFlags;
		bi->m_includeFlags = boundIncludeFlags;

		bi->m_levelIndex = boundLevelIndex;

		bi->m_componentList = NULL;
		bi->m_componentListCount = 0;
	}

#if __SPU

	void SpuInitBoundComposite(BoundInfo * bi)
	{
		FastAssert(bi->IsComposite());

		phBoundComposite * boundComposite = const_cast<phBoundComposite*>(bi->GetBoundComposite());

		const int numComponents = boundComposite->GetNumBounds();

		// Allocate the component list array.
		bi->m_componentList = componentManager.m_componentAllocator.GetBlockSafe(sizeof(u8)*numComponents);
		FastAssert(bi->m_componentList);
		bi->m_componentListCount = 0;

		const int COMPOSITE_DMA_TAG = 1;

		// The bound array was already DMA'ed in collisiontask.cpp

		// Get the type and include flags
		if (boundComposite->GetTypeAndIncludeFlags() != NULL)
		{
			const int bufferSize = sizeof(u32) * 2 * numComponents;
			u32 * TypeAndIncludeFlagsBuffer = (u32*)componentManager.m_componentAllocator.GetBlockSafe(bufferSize);
			FastAssert(TypeAndIncludeFlagsBuffer);
			sysDmaLargeGet(TypeAndIncludeFlagsBuffer, (uint64_t)boundComposite->GetTypeAndIncludeFlags(), bufferSize, DMA_TAG(COMPOSITE_DMA_TAG));
			boundComposite->SetTypeAndIncludeFlags_(TypeAndIncludeFlagsBuffer);
		}
		
		sysDmaWaitTagStatusAll(DMA_MASK(COMPOSITE_DMA_TAG));
	}

	void SpuInitBoundBVH(BoundInfo * bi)
	{
		FastAssert(bi->IsBVH());
		phBoundBVH * boundBVH = const_cast<phBoundBVH *>(bi->GetBoundBVH());
		const int BVH_DMA_TAG = 1;
		
		// Get the phOptimizedBvh structure.
		sysDmaLargeGet(m_bvhMemory.m_bvhBuffer, (uint64_t)boundBVH->GetBVH(), sizeof(phOptimizedBvh), DMA_TAG(BVH_DMA_TAG));
		boundBVH->SetBVH(reinterpret_cast<phOptimizedBvh*>(m_bvhMemory.m_bvhBuffer));
		
		// Get the materials.
		const int numMaterialIds = boundBVH->GetNumMaterials();
		Assert(numMaterialIds < BvhMemory::MAX_MATERIAL_IDS);
		//phMaterialMgr::Id *spuMaterialIdBuffer = reinterpret_cast<phMaterialMgr::Id *>(collisionBuffers->GetBlock(sizeof(phMaterial) * numMaterialIds));
		sysDmaLargeGet(m_bvhMemory.m_materialIdBuffer, (uint64_t)boundBVH->m_MaterialIds, numMaterialIds * sizeof(phMaterialMgr::Id), DMA_TAG(BVH_DMA_TAG));
		boundBVH->m_MaterialIds = reinterpret_cast<phMaterialMgr::Id*>(m_bvhMemory.m_materialIdBuffer);
		
		sysDmaWaitTagStatusAll(DMA_MASK(BVH_DMA_TAG));
	}

	void SpuInitBoundGeometry(BoundInfo * bi)
	{
		FastAssert(bi->IsGeometry());
		phBoundGeometry * boundGeometry = const_cast<phBoundGeometry *>(bi->GetBoundGeometry());
		Verifyf(DmaBoundGeometry(boundGeometry,&componentManager.m_componentAllocator),"Collision memory allocation failure.");
		DmaBoundGeometryWait();
	}

	void InitBoundInfo(BoundInfo * bi)
	{
		if (bi->IsComposite())
		{
			SpuInitBoundComposite(bi);
		}
		else if (bi->IsGeometry())
		{
			SpuInitBoundGeometry(bi);
		}
	}

#else //__SPU

	void InitBoundInfo(BoundInfo * bi)
	{
		if (bi->IsComposite())
		{
			// Allocate component list array.
			const phBoundComposite * boundComposite = bi->GetBoundComposite();
			const int numComponents = boundComposite->GetNumBounds(); 
			bi->m_componentList = componentManager.m_componentAllocator.GetBlockSafe(sizeof(u8)*numComponents);
			FastAssert(bi->m_componentList);
			bi->m_componentListCount = 0;
		}
	}

#endif // __SPU

	static __forceinline u32 ShouldCollide(const u32 typeFlagsA, const u32 includeFlagsA, const u32 typeFlagsB, const u32 includeFlagsB)
	{
		const u32 retv = (typeFlagsA & includeFlagsB) && (typeFlagsB & includeFlagsA);
#if USE_PHYSICS_PROFILE_CAPTURE
		//if (retv)
		//	PPC_STAT_COUNTER_INC(ShouldCollideCounter,1);
#endif
		return retv;
	}

	void InitComponentList(BoundInfo * bi, const u32 otherTypeFlags, const u32 otherIncludeFlags)
	{
		FastAssert(bi->IsComposite());
		bi->m_componentListCount = 0;
		const phBoundComposite * boundComposite = bi->GetBoundComposite();
		u32 typeFlags = bi->m_typeFlags;
		u32 includeFlags = bi->m_includeFlags;
		for (int index = 0 ; index < boundComposite->GetNumBounds() ; index++)
		{
			if (boundComposite->GetBound(index))
			{
				if (boundComposite->GetTypeAndIncludeFlags())
				{
					typeFlags = boundComposite->GetTypeFlags(index);
					includeFlags = boundComposite->GetIncludeFlags(index);
				}
				if (ShouldCollide(typeFlags,includeFlags,otherTypeFlags,otherIncludeFlags))
				{
					Assert(index <= 0xFF);
					bi->m_componentList[bi->m_componentListCount] = (u8)index;
					bi->m_componentListCount++;
				}
			}
		}
	}
	
	bool RootOBBTest()
	{
		FastAssert(ShouldCollide(m_boundInfo[0].m_typeFlags,m_boundInfo[0].m_includeFlags,m_boundInfo[1].m_typeFlags,m_boundInfo[1].m_includeFlags));
		return OBBTest(m_boundInfo[0],m_boundInfo[1]);
	}

	struct NodeInfo
	{
		Vec3V m_nodeAABBHalfWidth;
		Vec3V m_nodeAABBCenter;
		Vec3V m_AABBMin;
		Vec3V m_AABBMax;
	};

	void InitNodeInfo(const phOptimizedBvh * CCD_RESTRICT bvh, const phOptimizedBvhNode * CCD_RESTRICT node, NodeInfo * CCD_RESTRICT ni)
	{
		//PPC_STAT_TIMER_SCOPED(BvhNodeTimer);
		//PPC_STAT_COUNTER_INC(BvhNodeCounter,1);
		Vec3V nodeAABBMin;
		Vec3V nodeAABBMax;
		bvh->UnQuantize(RC_VECTOR3(nodeAABBMin), node->m_AABBMin);
		bvh->UnQuantize(RC_VECTOR3(nodeAABBMax), node->m_AABBMax);
		const ScalarV half(V_HALF);
		Vec3V nodeAABBHalfWidth = half * (nodeAABBMax - nodeAABBMin);
		Vec3V nodeAABBCenter = half * (nodeAABBMin + nodeAABBMax);

#if 0		
		// Move and expand the composite bound part's bounding box to include both this and the previous frame.
		// TODO: The original BVH intersection doesn't expand the BV from motion. We should probably do this but for now we'll leave it as is.
		COT_ExpandOBBFromMotion(m_bvhInfo.m_bvhInput.m_current,m_bvhInfo.m_bvhInput.m_last,nodeAABBHalfWidth,nodeAABBCenter);
		ni->m_AABBMin = nodeAABBCenter - nodeAABBHalfWidth;
		ni->m_AABBMax = nodeAABBCenter + nodeAABBHalfWidth;
#else
		ni->m_AABBMin = nodeAABBMin;
		ni->m_AABBMax = nodeAABBMax;
#endif 

		// TestBoxToBoxOBB requires the w component to be zero.
		nodeAABBHalfWidth.SetWZero();
		ni->m_nodeAABBHalfWidth = nodeAABBHalfWidth;
		ni->m_nodeAABBCenter = nodeAABBCenter;
	}

	__forceinline bool IntersectAABB(const BvhParams & bvhParams, const NodeInfo & ni)
	{
		//PPC_STAT_TIMER_SCOPED(BvhNodeIsectTimer);
		PPC_STAT_COUNTER_INC(NodeOverlapTestCounter,1);
		BVH_PARAMS_ERROR_CHECK_ONLY(FastAssert(bvhParams.m_aabbValid));
		return IsGreaterThanOrEqualAll(ni.m_AABBMax,bvhParams.m_boundAABBMinLoc) && IsGreaterThanOrEqualAll(bvhParams.m_boundAABBMaxLoc,ni.m_AABBMin);
	}

	__forceinline bool IntersectOBB(BvhParams * CCD_RESTRICT bvhParams, const ComponentInfo * CCD_RESTRICT ci0, const NodeInfo * CCD_RESTRICT ni)
	{
		//PPC_STAT_TIMER_SCOPED(BvhNodeIsectTimer);
		BVH_PARAMS_ERROR_CHECK_ONLY(FastAssert(bvhParams->m_obbValid));
		bvhParams->m_aTobMatrix.SetCol3(bvhParams->m_aTobPosition - ni->m_nodeAABBCenter);
		return COT_TestBoxToBoxOBBFaces(ci0->GetBBHalfWidthLoc(), ni->m_nodeAABBHalfWidth, bvhParams->m_aTobMatrix);
	}

	MidPhaseIntersector(phManifold *rootManifold, phPool<phManifold> *manifoldPool) : m_newCompositeManifolds(rootManifold,manifoldPool) 
	{ 
	}

	ComponentInfoManager componentManager;
	BoundInfo m_boundInfo[2];
#if __SPU
	BvhMemory m_bvhMemory;
#endif // __SPU
	NewCollisionInput m_collisionInput;

	enum { MAX_RAWRESULT_COUNT = 4 };
	DiscreteCollisionDetectorInterface::SimpleResult m_rawResultList[MAX_RAWRESULT_COUNT];

	phCollisionCompositeManifoldMgr<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS> m_newCompositeManifolds;

	GJKCacheQueryInput m_gjkCacheInfo;
	phManifold * m_rootManifold;
	phCollisionMemory * m_collisionMemory;
	bool m_highPriorityManifolds;
	bool m_isComposite;

	void SetMisc(phManifold * rootManifold, const phCollisionInput & unswappedInput)
	{
		m_rootManifold = rootManifold;
		m_highPriorityManifolds = unswappedInput.highPriority;
		m_collisionMemory = unswappedInput.collisionMemory;
		m_gjkCacheInfo.SetCacheDatabase(unswappedInput.useGjkCache,GJKCACHESYSTEM,rootManifold);
		m_collisionInput.set(phManifold::GetManifoldMarginV(),NULL,NULL,&m_gjkCacheInfo,NULL);
		m_isComposite = m_boundInfo[0].IsComposite() || m_boundInfo[1].IsComposite();

		// Add a marker.
		componentManager.m_componentAllocator.SetMarker();
	}

	ComponentInfo * GetComponentInfo(BoundInfo * bi, const int component, const phBoundComposite * boundComposite)
	{
		ComponentInfo * ci = componentManager.GetComponentInfo(component);
		if (Unlikely(ci == NULL))
		{
			PPC_STAT_COUNTER_INC(MidPhaseMisses,1);
			ci = componentManager.CreateComponentInfo(component,bi->m_object.m_current,bi->GetLastMatrix(),boundComposite CI_SPU_ONLY(true));
			if (Unlikely(ci == NULL))
			{
				componentManager.ResetBuffer();
				ci = componentManager.CreateComponentInfo(component,bi->m_object.m_current,bi->GetLastMatrix(),boundComposite CI_SPU_ONLY(true));
				FastAssert(ci);
			}
			componentManager.SetComponentInfo(component,ci);
		}
		else
		{
			PPC_STAT_COUNTER_INC(MidPhaseHits,1);
		}
		return ci;
	}

#define MID_PHASE_EXIT_ON_FAILURE 1
#if MID_PHASE_EXIT_ON_FAILURE
#define CHECK_SUCCESS_RETURN(x) if (Unlikely((x) == false)) return
#define CHECK_SUCCESS_RETURN_RESULT(x) if (Unlikely((x) == false)) return false
#define SUCCESS_RETURN(b) return b
#define SUCCESS_RETURN_TYPE() bool
#else // MID_PHASE_EXIT_ON_FAILURE
#define CHECK_SUCCESS_RETURN(x) (x)
#define CHECK_SUCCESS_RETURN_RESULT(x) (x)
#define SUCCESS_RETURN(b) return
#define SUCCESS_RETURN_TYPE() void
#endif // MID_PHASE_EXIT_ON_FAILURE

	// ProcessResultComposite: returns true if we should continue processing, false otherwise.
	bool ProcessResultComposite(const NewCollisionObject * CCD_RESTRICT object0, const NewCollisionObject * CCD_RESTRICT object1, const u32 isSwappedMask, const int rawResultListCount)
	{
		FastAssert(rawResultListCount > 0 && rawResultListCount <= MAX_RAWRESULT_COUNT);

		phManifold * manifold;
		if (m_isComposite)
		{
			if (Unlikely(m_rootManifold->CompositeManifoldsEnabled() == false))
			{
				m_rootManifold->EnableCompositeManifolds();
				if (Unlikely(m_rootManifold->CompositeManifoldsEnabled() == false))
				{
					Displayf("Ran out of composite manifolds.");
					return false;
				}
			}
			manifold = m_newCompositeManifolds.PreLeafCollisionFindManifold(object0->m_component, object1->m_component, object0->m_bound, object1->m_bound, m_highPriorityManifolds);
			if (Unlikely(manifold == NULL))
			{
				Displayf("Composite manifolds couldn't create a manifold.");
				return false;
			}
		}
		else
			manifold = m_rootManifold;

		FastAssert(manifold);
		phManifoldResult manifoldResult(object0->m_bound, object1->m_bound, manifold);
		manifoldResult.SetReverseInManifold(isSwappedMask);

		// This is waiting on existing manifold contacts - And we don't care if there is no result, we still want to let them finish here for the next iteration to not get stomped
		// This probably isn't needed.
		SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(1)));

		for (int i = 0 ; i < rawResultListCount ; i++)
		{
			FastAssert(m_rawResultList[i].GetHasResult());
			manifoldResult.ProcessResult(object0->m_current,object1->m_current,m_rawResultList[i]);
			m_rawResultList[i].SetHasResult(false);
		}
		Assert(manifold->GetNumContacts() > 0);
		if (m_isComposite)
			m_newCompositeManifolds.PostLeafCollisionProcessManifold();
		return true;
	}

/*
	bool ProcessResultCompositeSwapped(const NewCollisionObject * CCD_RESTRICT object0__, const NewCollisionObject * CCD_RESTRICT object1__, const int rawResultListCount)
	{
		FastAssert(rawResultListCount > 0 && rawResultListCount <= MAX_RAWRESULT_COUNT);

		// TODO: Try using selects here.
		const NewCollisionObject * object0;// = ISelectP((size_t)m_isSwappedMask,object0__,object1__);
		const NewCollisionObject * object1;// = ISelectP((size_t)m_isSwappedMask,object1__,object0__);

		if (m_isSwappedMask)
		{
			object0 = object1__;
			object1 = object0__;
		}
		else
		{
			object0 = object0__;
			object1 = object1__;
		}
		return ProcessResultComposite(object0,object1,m_isSwappedMask,rawResultListCount);
	}
*/

	enum
	{
		CT_BOUND_BOUND,
		CT_BOUND_COMPOSITE,
		CT_BOUND_BVH,
		CT_COMPOSITE_COMPOSITE,
		CT_COMPOSITE_BVH,
		CT_NONE,
		CT_NUM_TYPES,
	};

	static int GetCollisionType(const phBound * bound0, const phBound * bound1)
	{
		if (bound0->GetType() == phBound::BVH)
		{
			if (Unlikely(bound1->GetType() == phBound::BVH))
				return CT_NONE;
			else if (bound1->GetType() == phBound::COMPOSITE)
				return CT_COMPOSITE_BVH;
			else
				return CT_BOUND_BVH;
		}
		else if (bound0->GetType() == phBound::COMPOSITE)
		{
			if (bound1->GetType() == phBound::BVH)
				return CT_COMPOSITE_BVH;
			else if (bound1->GetType() == phBound::COMPOSITE)
				return CT_COMPOSITE_COMPOSITE;
			else
				return CT_BOUND_COMPOSITE;
		}
		else
		{
			if (bound1->GetType() == phBound::BVH)
				return CT_BOUND_BVH;
			else if (bound1->GetType() == phBound::COMPOSITE)
				return CT_BOUND_COMPOSITE;
			else
				return CT_BOUND_BOUND;
		}
	}
	
	void BoundToBound()
	{
		FastAssert(m_boundInfo[0].IsSimpleBound());
		FastAssert(m_boundInfo[1].IsSimpleBound());

		BoundInfo * bi0;
		BoundInfo * bi1;

		bi0 = m_boundInfo + 0;
		bi1 = m_boundInfo + 1;

		// There's no need to do a flags check or a BV check since these were already done during the initialization of the root bounds.
		m_collisionInput.object0 = &bi0->m_object;
		m_collisionInput.object1 = &bi1->m_object;

		m_gjkCacheInfo.SetComponentIndex((u32)bi0->GetComponent(),(u32)bi1->GetComponent());
		m_gjkCacheInfo.SetPartIndex(0);
		phPairwiseCollisionProcessor::ProcessPairwiseCollision(m_rawResultList[0],m_collisionInput);
		if (m_rawResultList[0].GetHasResult())
			Verifyf(ProcessResultComposite(&bi0->m_object,&bi1->m_object,0,1),"This shouldn't allocate anything.");
	}

	__forceinline void InitCachelessComponent(ComponentInfoWithBoundBuffer * ci, BoundInfo * bi, const int component, const phBoundComposite * boundComposite)
	{
#if __SPU
		if (Unlikely(ci->Init(component,bi->m_object.m_current,bi->GetLastMatrix(),boundComposite,&componentManager.m_componentAllocator,true,ci->m_boundBuffer) == false))
		{
			// Currently the only bound type that allocates extra memory is GEOMETRY.
			FastAssert(ci->m_object.m_bound->GetType() == phBound::GEOMETRY);
			componentManager.ResetBuffer();
			// It might be more efficient to just re-dma the bound and not setup everything again.
			Verifyf(ci->Init(component,bi->m_object.m_current,bi->GetLastMatrix(),boundComposite,&componentManager.m_componentAllocator,true,ci->m_boundBuffer),"SPU collision memory allocation failure!");
		}
#else // __SPU
		// This shouldn't do any memory allocations.
		Verifyf(ci->Init(component,bi->m_object.m_current,bi->GetLastMatrix(),boundComposite),"This shouldn't do any memory allocations!");
#endif // __SPU
	}

#if __SPU
	__forceinline void ReInitCachelessComponent(ComponentInfoWithBoundBuffer * ci, BoundInfo * bi, const int component, const phBoundComposite * boundComposite)
	{
		// Currently the only bound type that allocates extra memory is GEOMETRY.
		if (ci->m_object.m_bound->GetType() == phBound::GEOMETRY)
		{
			// It might be more efficient to just re-dma the bound and not setup everything again.
			Verifyf(ci->Init(component,bi->m_object.m_current,bi->GetLastMatrix(),boundComposite,&componentManager.m_componentAllocator,true,ci->m_boundBuffer),"SPU collision memory allocation failure!");
		}
	}
#endif // SPU

	void BoundToComposite()
	{
		BoundInfo * bi0;
		BoundInfo * bi1;

		ComponentInfoWithBoundBuffer ci1Buffer;

		ComponentInfo * unSwappedCi[2];
		u32 isSwapped;
		// Swap the bound infos so that the second one is the composite.
		if (m_boundInfo[0].IsComposite())
		{
			FastAssert(m_boundInfo[1].IsSimpleBound());
			bi0 = m_boundInfo + 1;
			bi1 = m_boundInfo + 0;
			isSwapped = 1;
			unSwappedCi[0] = &ci1Buffer;
			unSwappedCi[1] = m_boundInfo + 1;
		}
		else
		{
			FastAssert(m_boundInfo[0].IsSimpleBound());
			FastAssert(m_boundInfo[1].IsComposite());
			bi0 = m_boundInfo + 0;
			bi1 = m_boundInfo + 1;
			isSwapped = 0;
			unSwappedCi[0] = m_boundInfo + 0;
			unSwappedCi[1] = &ci1Buffer;
		}

		const phBoundComposite * boundComposite1 = bi1->GetBoundComposite();
		u32 boundTypeFlags1 = bi1->m_typeFlags;
		u32 boundIncludeFlags1 = bi1->m_includeFlags;

		m_collisionInput.object0 = &unSwappedCi[0]->m_object;
		m_collisionInput.object1 = &unSwappedCi[1]->m_object;

		m_gjkCacheInfo.SetPartIndex(0);

		for (int component1 = 0 ; component1 < boundComposite1->GetNumBounds() ; component1++)
		{
			const phBound * bound1 = boundComposite1->GetBound(component1);
			
			if (bound1 == NULL)
				continue;

			// Get the component1 flags.
			if (boundComposite1->GetTypeAndIncludeFlags())
			{
				boundTypeFlags1 = boundComposite1->GetTypeFlags(component1);
				boundIncludeFlags1 = boundComposite1->GetIncludeFlags(component1);
			}

			// Do a flags check.
			if (ShouldCollide(bi0->m_typeFlags,bi0->m_includeFlags,boundTypeFlags1,boundIncludeFlags1))
			{
				// Get the component info.
				ComponentInfo * ci1 = &ci1Buffer;
				InitCachelessComponent(&ci1Buffer,bi1,component1,boundComposite1);

				if (OBBTest(*bi0,*ci1))
				{
					if (Unlikely(ci1->IsBVH()))
					{
						BoundInfo bi1_Temp;
						// These are the only parameters that are needed.
						bi1_Temp.m_object = ci1->m_object;
						bi1_Temp.m_levelIndex = bi1->m_levelIndex;
#if USE_NEWER_COLLISION_OBJECT
						// TODO: This is already computed in ComponentInfo::Init.
						bi1_Temp.SetLastMatrix(component1,bi1->GetLastMatrix(),boundComposite1 CI_SPU_ONLY(true));
#endif // USE_NEWER_COLLISION_OBJECT
						if (Unlikely(IntersectBVH(bi0,&bi1_Temp,isSwapped) == false))
							return;
						m_collisionInput.m_bvhInput = NULL;
						m_collisionInput.object0 = &unSwappedCi[0]->m_object;
						m_collisionInput.object1 = &unSwappedCi[1]->m_object;
						m_gjkCacheInfo.SetPartIndex(0);
					}
					else
					{
						m_gjkCacheInfo.SetComponentIndex(unSwappedCi[0]->GetComponent(),unSwappedCi[1]->GetComponent());
						FastAssert(m_gjkCacheInfo.m_PartIndex == 0);
						phPairwiseCollisionProcessor::ProcessPairwiseCollision(m_rawResultList[0],m_collisionInput);
						if (m_rawResultList[0].GetHasResult())
						{
							if (Unlikely(ProcessResultComposite(&unSwappedCi[0]->m_object,&unSwappedCi[1]->m_object,0,1) == false))
								return;
						}
					}
				}
			}
		}
	}

	void CompositeToComposite()
	{
		FastAssert(m_boundInfo[0].IsComposite());
		FastAssert(m_boundInfo[1].IsComposite());

		const int MAX_BVH_COMPONENTS = 32;
		u8 BVHComponentList0[MAX_BVH_COMPONENTS];
		int BVHComponentList0Count = 0;
		u8 BVHComponentList1[MAX_BVH_COMPONENTS];
		int BVHComponentList1Count = 0;

		// Initialize the component lists for both composites. This strips out NULL bounds and bounds that don't collide based on flags.
		InitComponentList(m_boundInfo+0,m_boundInfo[1].m_typeFlags,m_boundInfo[1].m_includeFlags);
		if (Unlikely(m_boundInfo[0].m_componentListCount == 0))
			return;
		InitComponentList(m_boundInfo+1,m_boundInfo[0].m_typeFlags,m_boundInfo[0].m_includeFlags);
		if (Unlikely(m_boundInfo[1].m_componentListCount == 0))
			return;
		
		// Swap the bound infos so that the first one (outer loop) has the smaller number of components.
		BoundInfo * bi0;
		BoundInfo * bi1;
		ComponentInfoWithBoundBuffer ci0Buffer;
		ComponentInfo * unSwappedCi[2];
		int isSwapped;

		if (m_boundInfo[0].m_componentListCount <= m_boundInfo[1].m_componentListCount)
		{
			bi0 = m_boundInfo + 0;
			bi1 = m_boundInfo + 1;
			isSwapped = 0;
			unSwappedCi[0] = &ci0Buffer;
		}
		else
		{
			bi0 = m_boundInfo + 1;
			bi1 = m_boundInfo + 0;
			isSwapped = 1;
			unSwappedCi[1] = &ci0Buffer;
		}

		// Miscellaneous stuff.
		u32 typeFlags0 = bi0->m_typeFlags;
		u32 includeFlags0 = bi0->m_includeFlags;
		u32 typeFlags1 = bi1->m_typeFlags;
		u32 includeFlags1 = bi1->m_includeFlags;
		const phBoundComposite * boundComposite0 = bi0->GetBoundComposite();
		const phBoundComposite * boundComposite1 = bi1->GetBoundComposite();

		componentManager.InitComponentInfoMap(boundComposite1->GetNumBounds());

		m_gjkCacheInfo.SetPartIndex(0);

#define USE_SECOND_COMPONENT_TEST 1
#if USE_SECOND_COMPONENT_TEST
		FastAssert(boundComposite1->GetNumBounds() <= 256);
		u32 bi1_Tested[8] = {0,0,0,0,0,0,0,0};
#endif // USE_SECOND_COMPONENT_TEST
		
		int write_index0 = 0; // Used for collapsing the bi0 component list when we remove components.
		for (int index0 = 0 ; index0 < bi0->m_componentListCount ; index0++)
		{
			const u8 component0 = bi0->m_componentList[index0];

			// Write the component.
			bi0->m_componentList[write_index0] = component0;
			write_index0++;

			// Get the first component info.
			ComponentInfo * ci0 = &ci0Buffer;
			InitCachelessComponent(&ci0Buffer,bi0,component0,boundComposite0);

			// Check the first component's BV against the second composite's BV.
			if (!OBBTest(*ci0,*bi1))
			{
				write_index0--; // Remove this component from the list.
				continue;
			}

			if (Unlikely(ci0->IsBVH()))
			{
				FastAssert(BVHComponentList0Count < MAX_BVH_COMPONENTS);
				BVHComponentList0[BVHComponentList0Count] = component0;
				BVHComponentList0Count++;
				write_index0--;	// Remove this component from the list.
				continue;
			}

			// Get the first component's flags.
			if (boundComposite0->GetTypeAndIncludeFlags())
			{
				typeFlags0 = boundComposite0->GetTypeFlags(component0);
				includeFlags0 = boundComposite0->GetIncludeFlags(component0);
			}

			int write_index1 = 0; // Used for collapsing the bi1 component list when we remove components.
			for (int index1 = 0 ; index1 < bi1->m_componentListCount ; index1++)
			{
				const u8 component1 = bi1->m_componentList[index1];
				
				// Write the component.
				bi1->m_componentList[write_index1] = component1;
				write_index1++;

				// Get the second component's flags.
				if (boundComposite1->GetTypeAndIncludeFlags())
				{
					typeFlags1 = boundComposite1->GetTypeFlags(component1);
					includeFlags1 = boundComposite1->GetIncludeFlags(component1);
				}

				// Do a flags check before proceeding.
				if (ShouldCollide(typeFlags0,includeFlags0,typeFlags1,includeFlags1))
				{
					PPC_STAT_COUNTER_INC(ComponentComponentCounter,1);
					// Get the second component info.
#if __SPU
					ComponentInfo * ci1 = componentManager.GetComponentInfo(component1);
					if (Unlikely(ci1 == NULL))
					{
						PPC_STAT_COUNTER_INC(MidPhaseMisses,1);
						ci1 = componentManager.CreateComponentInfo(component1,bi1->m_object.m_current,bi1->GetLastMatrix(),boundComposite1 CI_SPU_ONLY(true));
						if (Unlikely(ci1 == NULL))
						{
							componentManager.ResetBuffer();
							ReInitCachelessComponent(&ci0Buffer,bi0,component0,boundComposite0);
							ci1 = componentManager.CreateComponentInfo(component1,bi1->m_object.m_current,bi1->GetLastMatrix(),boundComposite1 CI_SPU_ONLY(true));
							FastAssert(ci1);
						}
						componentManager.SetComponentInfo(component1,ci1);
					}
					else
					{
						PPC_STAT_COUNTER_INC(MidPhaseHits,1);
					}
#else // __SPU
					ComponentInfo * ci1 = GetComponentInfo(bi1,component1,boundComposite1);
#endif // __SPU
					FastAssert(ci1);

#if USE_SECOND_COMPONENT_TEST

					const u32 bi1_TestedArrayIndex = component1 >> 5;//component1 / 32;
					const u32 bi1_TestedBitIndex = component1 & 31;//component1 % 32;
					const u32 bi1_TestedBitMask = 1 << bi1_TestedBitIndex;
					const u32 bi1_TestedChunk = bi1_Tested[bi1_TestedArrayIndex];
					const u32 bi1_TestedBit = bi1_TestedChunk & bi1_TestedBitMask;
					if (bi1_TestedBit == 0)
					{
						bi1_Tested[bi1_TestedArrayIndex] = bi1_TestedChunk | bi1_TestedBitMask;
						// Do an overlap test against the first composite's bounding volume.
						// Only do this test on the first pass. This excludes components in all subsequent passes for better performance.
						// I would like to do this test within InitComponentList but the SPU makes this difficult.
						if (!OBBTest(*bi0,*ci1))
						{
							write_index1--; // Remove this component from the list.
							continue;
						}
						else if (Unlikely(ci1->IsBVH()))
						{
							FastAssert(BVHComponentList1Count < MAX_BVH_COMPONENTS);
							BVHComponentList1[BVHComponentList1Count] = component1;
							BVHComponentList1Count++;
							write_index1--;	// Remove this component from the list.
							continue;
						}
					}

#else // USE_SECOND_COMPONENT_TEST

					if (Unlikely(ci1->IsBVH()))
					{
						if (OBBTest(*bi0,*ci1))
						{
							FastAssert(BVHComponentList1Count < MAX_BVH_COMPONENTS);
							BVHComponentList1[BVHComponentList1Count] = component1;
							BVHComponentList1Count++;
						}
						write_index1--;		// Remove this component from the list.
						continue;
					}

#endif // USE_SECOND_COMPONENT_TEST

					// Check the first component's BV against the second component's BV.
					if (OBBTest(*ci0,*ci1))
					{
						unSwappedCi[1-isSwapped] = ci1;
						m_gjkCacheInfo.SetComponentIndex(unSwappedCi[0]->GetComponent(),unSwappedCi[1]->GetComponent());
						FastAssert(m_gjkCacheInfo.m_PartIndex == 0);
						m_collisionInput.object0 = &unSwappedCi[0]->m_object;
						m_collisionInput.object1 = &unSwappedCi[1]->m_object;
						phPairwiseCollisionProcessor::ProcessPairwiseCollision(m_rawResultList[0],m_collisionInput);
						if (m_rawResultList[0].GetHasResult())
						{
							if (Unlikely(ProcessResultComposite(&unSwappedCi[0]->m_object,&unSwappedCi[1]->m_object,0,1) == false))
								return;
						}
					}
				}
			}
			bi1->m_componentListCount = write_index1;
		}
		bi0->m_componentListCount = write_index0;

		if (Unlikely(BVHComponentList0Count > 0))
		{
			if (Unlikely(ComponentBVHIntersect(BVHComponentList0,BVHComponentList0Count,bi0,bi1) == false))
				return;
		}
		if (Unlikely(BVHComponentList1Count > 0))
		{
			// The cache was only used for the second composite. Now it'll be used for the first composite, so it must be reset.
			componentManager.ResetBuffer(boundComposite0->GetNumBounds());
			if (Unlikely(ComponentBVHIntersect(BVHComponentList1,BVHComponentList1Count,bi1,bi0) == false))
				return;
		}
	}

	bool ComponentBVHIntersect(const u8 * BVHComponentList0, const int BVHComponentList0Count, BoundInfo * bi0, BoundInfo * bi1)
	{
		FastAssert(BVHComponentList0Count > 0);
		FastAssert(bi0->IsComposite());
		FastAssert(bi1->IsComposite());

		// Get some miscellaneous stuff.
		u32 typeFlags0 = bi0->m_typeFlags;
		u32 includeFlags0 = bi0->m_includeFlags;
		u32 typeFlags1 = bi1->m_typeFlags;
		u32 includeFlags1 = bi1->m_includeFlags;
		const phBoundComposite * boundComposite0 = bi0->GetBoundComposite();
		const phBoundComposite * boundComposite1 = bi1->GetBoundComposite();

		BoundInfo bi0_Temp;
#if __SPU
		ALIGNAS(16) u8 boundBuffer[phBound::MAX_BOUND_SIZE] ;
#endif // __SPU
		bi0_Temp.m_levelIndex = bi0->m_levelIndex;

		// Save the current component list. This might not always be necessary if nothing wants these after this function.
		u8 * componentList1Save = bi1->m_componentList;
		int componentList1SaveCount = bi1->m_componentListCount;

		// Allocate the temporary component list.
		u8 * componentList1Temp = Alloca(u8,bi1->m_componentListCount);
		int componentList1TempCount;
		bi1->m_componentList = componentList1Temp;

		// Set the swapped mask.
		u32 isSwapped;
		if (bi1 == m_boundInfo + 0)
			isSwapped = 0;
		else
			isSwapped = 1;

		for (int bvh_i = 0 ; bvh_i < BVHComponentList0Count ; bvh_i++)
		{
			const u8 component0 = BVHComponentList0[bvh_i];
			
			// Get the bvh component flags.
			if (boundComposite0->GetTypeAndIncludeFlags())
			{
				typeFlags0 = boundComposite0->GetTypeFlags(component0);
				includeFlags0 = boundComposite0->GetIncludeFlags(component0);
			}

			// Construct the list of components.
			componentList1TempCount = 0;
			for (int index1 = 0 ; index1 < componentList1SaveCount ; index1++)
			{
				const u8 component1 = componentList1Save[index1];
				if (boundComposite1->GetTypeAndIncludeFlags())
				{
					typeFlags1 = boundComposite1->GetTypeFlags(component1);
					includeFlags1 = boundComposite1->GetIncludeFlags(component1);
				}
				if (ShouldCollide(typeFlags0,includeFlags0,typeFlags1,includeFlags1))
				{
					componentList1Temp[componentList1TempCount] = component1;
					componentList1TempCount++;
				}
			}

			if (Likely(componentList1TempCount > 0))
			{
				bi1->m_componentListCount = componentList1TempCount;
				Verifyf(bi0_Temp.Init(component0,bi0->m_object.m_current,bi0->GetLastMatrix(),boundComposite0 CI_SPU_ONLY(&componentManager.m_componentAllocator) CI_SPU_ONLY(true) CI_SPU_ONLY(boundBuffer)),"Shouldn't allocate memory.");
#if USE_NEWER_COLLISION_OBJECT
				// TODO: This is already computed in ComponentInfo::Init.
				bi0_Temp.SetLastMatrix(component0,bi0->GetLastMatrix(),boundComposite0 CI_SPU_ONLY(true));
#endif // USE_NEWER_COLLISION_OBJECT
				FastAssert(bi0_Temp.IsBVH());
				if (Unlikely(IntersectBVH(bi1,&bi0_Temp,isSwapped) == false))
					return false;
			}
		}

		// Restore the saved component list.
		bi1->m_componentList = componentList1Save;
		bi1->m_componentListCount = componentList1SaveCount;

		return true;
	}

#if !USE_ALT_INTX
	// ProcessLeafNodeComposite: returns true if collision processing should continue, false other wise.
	bool ProcessLeafNodeComposite(BvhParams * CCD_RESTRICT bvhParams, BoundInfo * CCD_RESTRICT bi0, const BoundInfo * CCD_RESTRICT bi1, const phOptimizedBvhNode * CCD_RESTRICT node, const NodeInfo * CCD_RESTRICT ni)
	{
		(void)bvhParams;
		FastAssert(bi0->IsComposite());
		FastAssert(bi1->IsBVH());
		FastAssert(node->IsLeafNode());

		if (bi0->m_componentListCount == 0)
		{
			// Initialize the component list for the composite. This strips out NULL bounds and bounds that don't collide based on flags.
			InitComponentList(bi0,bi1->m_typeFlags,bi1->m_includeFlags);
			if (Unlikely(bi0->m_componentListCount == 0))
				return false;
			componentManager.InitComponentInfoMap(bi0->GetBoundComposite()->GetNumBounds());
		}

		PrimitiveLeaf * leaf = NULL;

#if USE_PRIMITIVE_CULLING
		const int MAX_NUM_PRIMS = 4;
		PrimitiveBase * primList[MAX_NUM_PRIMS];
		int primListCount = 0;
#endif

		const phBoundComposite * boundComposite = bi0->GetBoundComposite();

#if DONT_USE_COMPONENT_CACHE
		ComponentInfoWithBoundBuffer ci0Buffer;
#endif
		for (int index = 0 ; index < bi0->m_componentListCount ; index++)
		{
			const int component = bi0->m_componentList[index];

			// Get the component info.
#if DONT_USE_COMPONENT_CACHE
			ComponentInfo * ci = &ci0Buffer;
			ci->Init(component,bi0->m_object.m_current,bi0->m_object.m_last,boundComposite,&componentManager.m_componentAllocator CI_SPU_ONLY(true) CI_SPU_ONLY(ci0Buffer.m_boundBuffer));
#else
			ComponentInfo * ci = GetComponentInfo(bi0,component,boundComposite);
#endif
			FastAssert(ci);
			if (Unlikely(ci->IsBVH()))
			{
				// We don't support BVH-BVH collision.
				continue;
			}

			// Do a BV check of the component against the leaf.
			Mat34V aTobMat;
			UnTransform3x3Ortho(aTobMat,bi1->m_object.m_current,ci->m_object.m_current);
			const Vec3V aTobPos = UnTransform3x3Ortho(bi1->m_object.m_current,ci->m_bbCenterAbs-bi1->m_object.m_current.GetCol3());
			aTobMat.SetCol3(aTobPos-ni->m_nodeAABBCenter);
			if (COT_TestBoxToBoxOBBFaces(ci->m_bbHalfWidth, ni->m_nodeAABBHalfWidth, aTobMat))
			{
				PPC_STAT_COUNTER_INC(BvhNodeLeafCounter,1);
				if (Unlikely(leaf == NULL))
				{
					BVH_PARAMS_ERROR_CHECK_ONLY(ASSERT_ONLY(FastAssert(bvhParams->m_instValid)));
					leaf = m_collisionMemory->m_primCache.GetLeaf(bi1->GetBoundBVH(),node,bi1->m_levelIndex,(u16)bi1->GetComponent() CHECK_TRI_ONLY(bvhParams->m_pInstOfBvh));
#if USE_PRIMITIVE_CULLING
					Assert(primListCount == 0);
					bvhParams->m_aTobMatrix.SetCol3(bvhParams->m_aTobPosition);
					for (PrimitiveBase * prim = leaf->m_first ; prim ; prim = prim->m_next)
					{
						if (prim->TestOverlap(bvhParams->m_aTobMatrix,bi0->m_bbHalfWidth))
						{
							Assert(primListCount < MAX_NUM_PRIMS);
							primList[primListCount] = prim;
							primListCount++;
						}
					}
					if (primListCount == 0)
						return true;
#endif
				}
				FastAssert(leaf);
				//PPC_STAT_TIMER_STOP(PrimitiveGetLeafTimer);

				aTobMat.SetCol3(aTobPos);
				m_collisionInput.object0 = &ci->m_object;

				//PPC_STAT_TIMER_START(PrimitiveCollideTimer);
				int rawResultListCount = 0;
#if USE_PRIMITIVE_CULLING
				for (int prim_i = 0 ; prim_i < primListCount ; prim_i++)
				{
					PrimitiveBase * prim = primList[prim_i];
#else
				for (PrimitiveBase * prim = leaf->m_first ; prim ; prim = prim->m_next)
				{
#endif
					//PPC_STAT_COUNTER_INC(PrimitiveCollideCounter,1);
					FastAssert(rawResultListCount < MAX_RAWRESULT_COUNT);
					DiscreteCollisionDetectorInterface::SimpleResult & rawResult = m_rawResultList[rawResultListCount];

					prim->Collide(aTobMat,ci->m_bbHalfWidth,m_collisionInput,rawResult);

					if (rawResult.GetHasResult())
						rawResultListCount++;
				}
				//PPC_STAT_TIMER_STOP(PrimitiveCollideTimer);

				//PPC_STAT_TIMER_START(ProcessResultTimer);
				if (rawResultListCount > 0)
				{
					if (Unlikely(ProcessResultCompositeSwapped(&ci->m_object,&bi1->m_object,rawResultListCount) == false))
						return false;
				}
				//PPC_STAT_TIMER_STOP(ProcessResultTimer);
			}
		}
		return true;
	}
#endif // !USE_ALT_INTX

	bool ProcessLeafNode(BvhParams * CCD_RESTRICT bvhParams, const ComponentInfo * CCD_RESTRICT ci0, const BoundInfo * CCD_RESTRICT bi1, const phOptimizedBvhNode * CCD_RESTRICT node)
	{
		PPC_STAT_COUNTER_INC(BvhNodeLeafCounter,1);

		FastAssert(ci0->IsSimpleBound());
		FastAssert(bi1->IsBVH());
		FastAssert(node->IsLeafNode());

		BVH_PARAMS_ERROR_CHECK_ONLY(ASSERT_ONLY(FastAssert(bvhParams->m_instValid)));
		PrimitiveLeaf * leaf = m_collisionMemory->m_primCache.GetLeaf(bi1->GetBoundBVH(),node,bi1->m_levelIndex,(u16)bi1->GetComponent() CHECK_TRI_ONLY(bvhParams->m_pInstOfBvh));

#if PRIM_CACHE_RENDER
		g_PrimitiveRender.Render(leaf,bi1->m_object.m_current,bi1->m_levelIndex,(u16)bi1->GetComponent(),node->GetPolygonStartIndex());
#endif

		m_collisionInput.object0 = &ci0->m_object;
		
		const ComponentInfo * unSwappedCi[2];
		unSwappedCi[bvhParams->m_isSwapped] = ci0;
		unSwappedCi[1-bvhParams->m_isSwapped] = bi1;

		m_gjkCacheInfo.SetComponentIndex(unSwappedCi[0]->GetComponent(),unSwappedCi[1]->GetComponent());
		// gjkCacheInfo.PartIndex is set inside of prim->Collide;

		int rawResultListCount = 0;

		bvhParams->m_aTobMatrix.SetCol3(bvhParams->m_aTobPosition);
		for (PrimitiveBase * prim = leaf->m_first ; prim ; prim = prim->m_next)
		{
			FastAssert(rawResultListCount < MAX_RAWRESULT_COUNT);
			DiscreteCollisionDetectorInterface::SimpleResult & rawResult = m_rawResultList[rawResultListCount];

			prim->Collide(bvhParams->m_aTobMatrix,ci0->GetBBHalfWidthLoc(),m_collisionInput,rawResult);

			if (rawResult.GetHasResult())
				rawResultListCount++;
		}
		if (rawResultListCount > 0)
			return ProcessResultComposite(&unSwappedCi[0]->m_object,&unSwappedCi[1]->m_object,bvhParams->m_isSwappedMask,rawResultListCount);
		else
			return true;
	}

	// IntersectBVH1: Intersects a single component against a BVH using OBB tests. Used for subtrees near the bottom of the BVH.
	bool IntersectBVH1(BvhParams * CCD_RESTRICT bvhParams, const ComponentInfo * CCD_RESTRICT ci0, const BoundInfo * CCD_RESTRICT bi1, const phOptimizedBvhNode * CCD_RESTRICT leftChild, const phOptimizedBvhNode * CCD_RESTRICT rightChild)
	{
		FastAssert(ci0->IsSimpleBound());
		FastAssert(bi1->IsBVH());

		const int MAX_STACK_SIZE = 64;
		const phOptimizedBvhNode * stack[MAX_STACK_SIZE];
		int stackSize = 2;
		stack[0] = rightChild;
		stack[1] = leftChild;

		NodeInfo ni;

#if __SPU
		ALIGNAS(16) u8 nodeBuffer[sizeof(phOptimizedBvhNode)*2] ;
		const phOptimizedBvhNode * node_LS = reinterpret_cast<phOptimizedBvhNode*>(nodeBuffer);
		const phOptimizedBvhNode * nodeLeftChild_LS = reinterpret_cast<phOptimizedBvhNode*>(nodeBuffer) + 1;
		const int NODE_TAG = 1;
#endif // __SPU

		const phOptimizedBvh * bvh = bi1->GetBoundBVH()->GetBVH();
		do 
		{
			FastAssert(stackSize > 0);
			stackSize--;
#if __SPU
			const phOptimizedBvhNode * node_EA = stack[stackSize];
			sysDmaGet(node_LS, (uint64_t)node_EA, sizeof(phOptimizedBvhNode)*2, DMA_TAG(NODE_TAG));
			sysDmaWait(DMA_MASK(NODE_TAG));
			const phOptimizedBvhNode * node = node_LS;
#else // __SPU
			const phOptimizedBvhNode * node = stack[stackSize];
#endif // __SPU
			InitNodeInfo(bvh,node,&ni);
			if (IntersectOBB(bvhParams,ci0,&ni))
			{
				if (node->IsLeafNode())
				{
					if (Unlikely(ProcessLeafNode(bvhParams,ci0,bi1,node) == false))
						return false;
				}
				else
				{
					FastAssert(stackSize + 2 <= MAX_STACK_SIZE);
#if __SPU
					const phOptimizedBvhNode * leftChild = node_EA + 1;
					const phOptimizedBvhNode * rightChild = leftChild + nodeLeftChild_LS->GetEscapeIndex();
#else // __SPU
					const phOptimizedBvhNode * leftChild = node + 1;
					const phOptimizedBvhNode * rightChild = leftChild + leftChild->GetEscapeIndex();
#endif // __SPU
					stack[stackSize] = rightChild;
					stackSize++;
					stack[stackSize] = leftChild;
					stackSize++;
				}
			}
		} while (stackSize > 0);
		return true;
	}

	bool IntersectBVH_()
	{
		BoundInfo * bi0;
		BoundInfo * bi1;
		u32 isSwapped;
		if (m_boundInfo[0].IsBVH())
		{
			FastAssert(!m_boundInfo[1].IsBVH());
			bi0 = m_boundInfo + 1;
			bi1 = m_boundInfo + 0;
			isSwapped = 1;
		}
		else
		{
			FastAssert(m_boundInfo[1].IsBVH());
			bi0 = m_boundInfo + 0;
			bi1 = m_boundInfo + 1;
			isSwapped = 0;
		}
		return IntersectBVH(bi0,bi1,isSwapped);
	}

	// Intersects a composite or a single bound against a BVH. It starts off using AABB tests, then transitions to OBB tests when the size of the subtree is 'small enough'.
	bool IntersectBVH(BoundInfo * bi0, BoundInfo * bi1, const u32 isSwapped)
	{
		FastAssert(!bi0->IsBVH());
		FastAssert(bi1->IsBVH());
		FastAssert(isSwapped == 0 || isSwapped == 1);

		m_collisionInput.object1 = NULL;

#if __SPU
		SpuInitBoundBVH(bi1);
#endif // __SPU

		const phOptimizedBvh * bvh = bi1->GetBoundBVH()->GetBVH();
		phOptimizedBvhNode * root = bvh->GetRootNode();
		const int MAX_STACK_SIZE = 256;
		const phOptimizedBvhNode * stack[MAX_STACK_SIZE];
		int stackSize = 1;
		stack[0] = root;
		
		NodeInfo ni;
		
		BvhParams bvhParams(isSwapped);
		InitBvhParams(&bvhParams,bi0,bi1);
		
#if USE_NEWER_COLLISION_OBJECT
		BVHInput bvhInput;
		bvhInput.m_current = bi1->m_object.m_current;
		bvhInput.m_last = bi1->GetLastMatrix();
		bvhInput.m_component = bi1->m_object.m_component;
		m_collisionInput.m_bvhInput = &bvhInput;
#else // USE_NEWER_COLLISION_OBJECT
		m_collisionInput.m_bvhInput = &bi1->m_object;
#endif // USE_NEWER_COLLISION_OBJECT

#if USE_ALT_INTX 
		const float factorF = 1.0f;
		const ScalarV factor(factorF);
		const ScalarV size0_sq = MagSquared(factor * bi0->GetBBHalfWidthLoc());
#endif // USE_ALT_INTX
		
#if __SPU
		// TODO: The dma'ing here sucks. It's redundant. Update: Profiling reveals that the performance is actually pretty good. Probably not worth doing anything more complicated.
		ALIGNAS(16) u8 nodeBuffer[sizeof(phOptimizedBvhNode)*2] ;
		const phOptimizedBvhNode * node_LS = reinterpret_cast<phOptimizedBvhNode*>(nodeBuffer);
		const phOptimizedBvhNode * nodeLeftChild_LS = reinterpret_cast<phOptimizedBvhNode*>(nodeBuffer) + 1;
		const int NODE_TAG = 1;
#endif // __SPU

		do 
		{
			FastAssert(stackSize > 0);
			stackSize--;
#if __SPU
			const phOptimizedBvhNode * node_EA = stack[stackSize];
			sysDmaGet(node_LS, (uint64_t)node_EA, sizeof(phOptimizedBvhNode)*2, DMA_TAG(NODE_TAG));
			sysDmaWait(DMA_MASK(NODE_TAG));
			const phOptimizedBvhNode * node = node_LS;
#else // __SPU
			const phOptimizedBvhNode * node = stack[stackSize];
#endif // __SPU
			InitNodeInfo(bvh,node,&ni);
			if (IntersectAABB(bvhParams,ni))
			{
				if (node->IsLeafNode())
				{
					if (IntersectOBB(&bvhParams,bi0,&ni))
					{
						if (bi0->IsComposite())
						{
#if USE_ALT_INTX
							if (Unlikely(bi0->m_componentListCount == 0))
							{
								InitComponentList(bi0,bi1->m_typeFlags,bi1->m_includeFlags);
								if (Unlikely(bi0->m_componentListCount == 0))
									return false;
								componentManager.InitComponentInfoMap(bi0->GetBoundComposite()->GetNumBounds());
							}

							BvhParams bvhParams1(isSwapped);
#if USE_CHECK_TRIANGLE
							InitBvhParamsInst(&bvhParams1);
#endif // USE_CHECK_TRIANGLE
							const phBoundComposite * boundComposite0 = bi0->GetBoundComposite();
							for (int index = 0 ; index < bi0->m_componentListCount ; index++)
							{
								const int component0 = bi0->m_componentList[index];
								ComponentInfo * ci0 = GetComponentInfo(bi0,component0,boundComposite0);
								if (Likely(ci0->IsBVH() == false))
								{
									InitBvhParamsOBB(&bvhParams1,ci0,bi1);
									if (IntersectOBB(&bvhParams1,ci0,&ni))
									{
										if (Unlikely(ProcessLeafNode(&bvhParams1,ci0,bi1,node) == false))
											return false;
									}
								}
							}
#else // USE_ALT_INTX
							if (Unlikely(ProcessLeafNodeComposite(&bvhParams,bi0,bi1,node,&ni) == false))
								return false;
#endif // USE_ALT_INTX
						}
						else
						{
							if (Unlikely(ProcessLeafNode(&bvhParams,bi0,bi1,node) == false))
								return false;
						}
					}
				}
				else
				{
#if __SPU
					const phOptimizedBvhNode * leftChild = node_EA + 1;
					const phOptimizedBvhNode * rightChild = leftChild + nodeLeftChild_LS->GetEscapeIndex();
#else // __SPU
					const phOptimizedBvhNode * leftChild = node + 1;
					const phOptimizedBvhNode * rightChild = leftChild + leftChild->GetEscapeIndex();
#endif // __SPU
					
#if USE_ALT_INTX		
					// TODO: This code is mostly the same as above. It might be better to factor it out.
					// Process subtrees that are 'small enough' differently.
					if (IsGreaterThanAll(size0_sq,MagSquared(ni.m_nodeAABBHalfWidth)))
					{
						if (IntersectOBB(&bvhParams,bi0,&ni))
						{
							if (bi0->IsComposite())
							{
								if (Unlikely(bi0->m_componentListCount == 0))
								{
									InitComponentList(bi0,bi1->m_typeFlags,bi1->m_includeFlags);
									if (Unlikely(bi0->m_componentListCount == 0))
										return false;
									componentManager.InitComponentInfoMap(bi0->GetBoundComposite()->GetNumBounds());
								}

								BvhParams bvhParams1(isSwapped);
#if USE_CHECK_TRIANGLE
								InitBvhParamsInst(&bvhParams1);
#endif // USE_CHECK_TRIANGLE
								// Pass each component down the subtree separately.
								const phBoundComposite * boundComposite0 = bi0->GetBoundComposite();
								for (int index = 0 ; index < bi0->m_componentListCount ; index++)
								{
									const int component0 = bi0->m_componentList[index];
									ComponentInfo * ci0 = GetComponentInfo(bi0,component0,boundComposite0);
									if (Likely(ci0->IsBVH() == false))
									{
										InitBvhParamsOBB(&bvhParams1,ci0,bi1);
										if (IntersectOBB(&bvhParams1,ci0,&ni))
										{
											if (Unlikely(IntersectBVH1(&bvhParams1,ci0,bi1,leftChild,rightChild) == false))
												return false;
										}
									}
								}
							}
							else
							{
								if (Unlikely(IntersectBVH1(&bvhParams,bi0,bi1,leftChild,rightChild) ==  false))
									return false;
							}
						}
					}
					else
#endif // USE_ALT_INTX
					{
						FastAssert(stackSize + 2 <= MAX_STACK_SIZE);
						stack[stackSize] = rightChild;
						stackSize++;
						stack[stackSize] = leftChild;
						stackSize++;
					}
				}
			}
		} while (stackSize > 0);
		return true;
	}

	void PostCollisionProcess()
	{
		if (m_isComposite && Likely(m_rootManifold->CompositeManifoldsEnabled()))
			m_newCompositeManifolds.PostObjectCollisionFinalize();
	}
};

#if ALLOW_MID_PHASE_SWAP
bool g_UseNewMidPhaseCollision = true;
#endif

#endif // USE_NEW_MID_PHASE

void phMidphase::ProcessCollision(const phCollisionInput &unswappedInput) const
{
	PF_FUNC(ComputeCollision);
	PF_INCREMENT(ComputeCollisionCount);
	PPC_STAT_TIMER_SCOPED(ProcessCollisionTimer);
	PPC_STAT_COUNTER_INC(ProcessCollisionCounter,1);

#if USE_NEW_MID_PHASE

#if ALLOW_MID_PHASE_SWAP
	const phBound *rootBound0 = unswappedInput.boundA;
	const phBound *rootBound1 = unswappedInput.boundB;
	const int collisionType = MidPhaseIntersector::GetCollisionType(rootBound0,rootBound1);
	if (collisionType == MidPhaseIntersector::CT_NONE)
		return;
	static bool useAlt[MidPhaseIntersector::CT_NUM_TYPES];
	static bool useAltInited = false;
	if (useAltInited == false)
	{
		useAltInited = true;
		for (int i = 0 ; i < MidPhaseIntersector::CT_NUM_TYPES ; i++)
			useAlt[i] = true;
		//useAlt[MidPhaseIntersector::CT_BOUND_BOUND] = true;
		//useAlt[MidPhaseIntersector::CT_BOUND_BVH] = true;
		//useAlt[MidPhaseIntersector::CT_COMPOSITE_BVH] = true;
	}
	if (useAlt[collisionType] && g_UseNewMidPhaseCollision)
		ProcessCollisionNew(unswappedInput);
	else
		ProcessCollisionOriginal(unswappedInput);
#else // ALLOW_MID_PHASE_SWAP
	ProcessCollisionNew(unswappedInput);
#endif // ALLOW_MID_PHASE_SWAP

#else // USE_NEW_MID_PHASE

	ProcessCollisionOriginal(unswappedInput);

#endif // USE_NEW_MID_PHASE
}

#if USE_NEW_MID_PHASE
void phMidphase::ProcessCollisionNew(const phCollisionInput &unswappedInput) const
{
	// Test the root bound flags.
	if (Unlikely(MidPhaseIntersector::ShouldCollide(unswappedInput.typeFlagsA,unswappedInput.includeFlagsA,unswappedInput.typeFlagsB,unswappedInput.includeFlagsB) == 0))
		return;

	//PPC_STAT_COUNTER_INC(ProcessCollision0Counter,1);

	phManifold *rootManifold = unswappedInput.rootManifold;
	FastAssert(rootManifold);	// Pairs should always already have a manifold assigned by this point.
	const phBound *rootBound0 = unswappedInput.boundA;
	const phBound *rootBound1 = unswappedInput.boundB;

	//PPC_STAT_TIMER_SCOPED(ProcessCollisionNewTimer);
	//PPC_STAT_COUNTER_INC(ProcessCollisionNewCounter,1);
	phPool<phManifold>* manifoldPool = PHMANIFOLD;
	MidPhaseIntersector intx(rootManifold,manifoldPool);

	// Initialize the root bound objects.
	intx.SetBound(intx.m_boundInfo+0,unswappedInput.lastA,unswappedInput.currentA,rootBound0,unswappedInput.typeFlagsA,unswappedInput.includeFlagsA,rootManifold->GetLevelIndexA(),(u16)rootManifold->GetComponentA());
	intx.SetBound(intx.m_boundInfo+1,unswappedInput.lastB,unswappedInput.currentB,rootBound1,unswappedInput.typeFlagsB,unswappedInput.includeFlagsB,rootManifold->GetLevelIndexB(),(u16)rootManifold->GetComponentB());

	// Test the root bounds for overlap.
	if (intx.RootOBBTest() == false)
	{
#if USE_GJK_CACHE && USE_FRAME_PERSISTENT_GJK_CACHE
		GJKCacheDB * gjkCacheDB = rootManifold->GetGJKCacheDB();
		if (gjkCacheDB)
		{
#if __SPU
			cellDmaPutUint8(true,(uint64_t)&gjkCacheDB->m_delete,DMA_TAG(17),0,0);
#else // __SPU
			gjkCacheDB->m_delete = true;
#endif // __SPU
			rootManifold->SetGJKCacheDB(NULL);
		}
#endif // USE_GJK_CACHE && USE_FRAME_PERSISTENT_GJK_CACHE
		return;
	}

	//PPC_STAT_COUNTER_INC(ProcessCollision1Counter,1);

	intx.InitBoundInfo(intx.m_boundInfo+0);
	intx.InitBoundInfo(intx.m_boundInfo+1);

	// Setup miscellaneous stuff.
	intx.SetMisc(rootManifold,unswappedInput);

#if !__SPU	
	// TODO: This is not good. The proper place to set these pointers is in ProcessOverlaps. Any possible cache miss will likely not show up in a profile capture. On the other hand, 
	// not knowing when these pointers are valid or where they get set obfuscates the code and increases the chances of programming errors.

	// Where exactly is a good place to do this?  I chose to do it here because we have the bound pointer handy and it's in the general vicinity of where we'll be
	//   accessing the manifold (to avoid bad cache behavior) but this isn't the most logical place to do so.  That would be in phSimulator::ProcessOverlaps()
	//   where we fill in similar data in the manifold (but don't have the bound pointer handy and probably can't get it without a cache miss).
	rootManifold->SetBoundA(rootBound0);
	rootManifold->SetBoundB(rootBound1);
#endif	// !__SPU

	const int collisionType = MidPhaseIntersector::GetCollisionType(rootBound0,rootBound1);

	switch(collisionType)
	{
		case MidPhaseIntersector::CT_BOUND_BOUND:
			intx.BoundToBound();
			break;
		case MidPhaseIntersector::CT_BOUND_COMPOSITE:
			intx.BoundToComposite();
			break;
		case MidPhaseIntersector::CT_COMPOSITE_COMPOSITE:
			intx.CompositeToComposite();
			break;
		case MidPhaseIntersector::CT_BOUND_BVH:
		case MidPhaseIntersector::CT_COMPOSITE_BVH:
			intx.IntersectBVH_();
			break;
		default:
			Assert(0);
	};
	intx.PostCollisionProcess();

	intx.componentManager.m_componentAllocator.ReleaseAllMarkers();
}

#endif // USE_NEW_MID_PHASE

#if !USE_NEW_MID_PHASE || ALLOW_MID_PHASE_SWAP
void phMidphase::ProcessCollisionOriginal(const phCollisionInput &unswappedInput) const
{
	phManifold *rootManifold = unswappedInput.rootManifold;
	FastAssert(rootManifold);	// Pairs should always already have a manifold assigned by this point.
	const phBound *rootBound0 = unswappedInput.boundA;
	const phBound *rootBound1 = unswappedInput.boundB;

	// Shouldn't be getting any self-collisions through this code path.
	Assert(rootManifold->GetInstanceA() != rootManifold->GetInstanceB() || rootManifold->GetInstanceA() == NULL);
	Assert(rootManifold->GetLevelIndexA() != rootManifold->GetLevelIndexB() || rootManifold->GetLevelIndexA() == phInst::INVALID_INDEX);

#if __SPU
	u8 collisionBuffer[23 * 1024] ;
	phSpuCollisionBuffers spuCollisionBuffers(collisionBuffer, sizeof(collisionBuffer));
	ObjectState::SetSpuCollisionBuffers(&spuCollisionBuffers);
#endif	// __SPU

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create two ObjectState's to keep track of what part of the objects we're currently processing.
	ObjectState objectStates[2];
	if(!objectStates[0].Init(rootBound0, unswappedInput.currentA, unswappedInput.lastA, 0))
	{
		SPU_ONLY(sysDmaWaitTagStatusAll(SYS_DMA_MASK_ALL));
		SPU_ONLY(ObjectState::GetSpuCollisionBuffers()->ReleaseAllMarkers());
		return;
	}
	objectStates[0].SetTypeAndIncludeFlags(unswappedInput.typeFlagsA, unswappedInput.includeFlagsA);
	if(!objectStates[1].Init(rootBound1, unswappedInput.currentB, unswappedInput.lastB, 1))
	{
		SPU_ONLY(sysDmaWaitTagStatusAll(SYS_DMA_MASK_ALL));
		SPU_ONLY(ObjectState::GetSpuCollisionBuffers()->ReleaseAllMarkers());
		return;
	}
	objectStates[1].SetTypeAndIncludeFlags(unswappedInput.typeFlagsB, unswappedInput.includeFlagsB);
	objectStates[0].InitCullingBoxInfo(objectStates[1]);
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////

	// This is used to keep track of collision work that we need to come back to later.
	phCollisionWorkStack<COLLISION_WORK_STACK_DEPTH> collisionWorkStack;

	phPool<phManifold>* manifoldPool = PHMANIFOLD;
	bool highPriorityManifolds = unswappedInput.highPriority;
	phCollisionCompositeManifoldMgr<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS> newCompositeManifolds(rootManifold, manifoldPool);
	ProcessLeafCollisionManifold virtualManifold(&newCompositeManifolds,highPriorityManifolds);

	// NOTE: This wouldn't work for composites embedded in grids, although I'm not really sure that we really even support that (yeah, we don't - shouldn't they 
	//   use composite manifolds too though?).
	const bool rootBound0IsComposite = (rootBound0->GetType() == phBound::COMPOSITE);
	const bool rootBound1IsComposite = (rootBound1->GetType() == phBound::COMPOSITE);
	const bool useCompositeManifolds = rootBound0IsComposite || rootBound1IsComposite;
	FastAssert(useCompositeManifolds || !rootManifold->CompositeManifoldsEnabled());
	if (useCompositeManifolds)
	{
		// At least one of the two objects is a composite.
		rootManifold->EnableCompositeManifolds();
		if(rootManifold->m_CompositePointers == NULL)
		{
			// We need composite manifolds but we're not going to be able to have any way of keeping track of them.  Just abort the collision.
			SPU_ONLY(ObjectState::GetSpuCollisionBuffers()->ReleaseAllMarkers());
			return;
		}
	}
#if !__SPU
	// Where exactly is a good place to do this?  I chose to do it here because we have the bound pointer handy and it's in the general vicinity of where we'll be
	//   accessing the manifold (to avoid bad cache behavior) but this isn't the most logical place to do so.  That would be in phSimulator::ProcessOverlaps()
	//   where we fill in similar data in the manifold (but don't have the bound pointer handy and probably can't get it without a cache miss).
	rootManifold->SetBoundA(rootBound0);
	rootManifold->SetBoundB(rootBound1);
#endif	// !__SPU

	GJKCacheQueryInput gjk_cache_info;
	gjk_cache_info.SetCacheDatabase(unswappedInput.useGjkCache,GJKCACHESYSTEM,rootManifold);
	phCollisionMemory * collisionMemory = unswappedInput.collisionMemory;

	PF_START_2(MidphaseLoop);
	while(true)
	{
		// ShouldSubdivideOtherObjectState was crashing with what must have been either negative or NaN values in one of the objectState's volumes
		//  - so we're tossing some validation here trying to catch invalid volumes earlier and with a bit more context
		ASSERT_ONLY(const u32 float32NonRealExponent = 0x7f800000;)
		Assertf(objectStates[0].IsLeaf() || (objectStates[0].GetCurBoundingBoxVolume().Getf() > 0.0f && (objectStates[0].GetCurBoundingBoxVolumeAsInt() & float32NonRealExponent) != float32NonRealExponent),
				"ObjectState0 has bad volume: %.5f, 0x%x, %d", objectStates[0].GetCurBoundingBoxVolume().Getf(), objectStates[0].GetCurBoundingBoxVolumeAsInt(), objectStates[0].GetCurBoundingBoxVolumeAsInt()
				/*objectStates[0].*/);
		Assertf(objectStates[1].IsLeaf() || (objectStates[1].GetCurBoundingBoxVolume().Getf() > 0.0f && (objectStates[1].GetCurBoundingBoxVolumeAsInt() & float32NonRealExponent) != float32NonRealExponent),
				"ObjectState1 has bad volume: %.5f, 0x%x, %d", objectStates[1].GetCurBoundingBoxVolume().Getf(), objectStates[1].GetCurBoundingBoxVolumeAsInt(), objectStates[1].GetCurBoundingBoxVolumeAsInt());

		// TODO: We might not need to bother with this overlap test on the first iteration through this loop as ProcessOverlaps() performs a similar computation.
		//   On the other hand, ProcessOverlap() only performs that test in the case where the pair doesn't have a manifold, so it doesn't do it all the time
		//   (maybe that's okay though?)
		if(objectStates[0].TestOverlap(objectStates[1]))
		{
			if(objectStates[0].IsLeaf() && objectStates[1].IsLeaf())
			{
				// The object states need to be passed into ProcessLeafCollision() in the same order as they appear in the manifold.
				const ObjectState &objectState0ForCollision = objectStates[objectStates[0].GetCollisionBoundIndex()];
				const ObjectState &objectState1ForCollision = objectStates[objectStates[1].GetCollisionBoundIndex()];

				const int component0 = objectState0ForCollision.GetComponentIndex();
				const int component1 = objectState1ForCollision.GetComponentIndex();
				if (useCompositeManifolds)
					virtualManifold.SetComponents(rootManifold,component0,component1);
				else
					virtualManifold.SetManifold(rootManifold);
				gjk_cache_info.SetComponentIndex((u32)component0,(u32)component1);
				
				ProcessLeafCollision(objectState0ForCollision, objectState1ForCollision, &virtualManifold, &gjk_cache_info, collisionMemory);

				if (Unlikely(virtualManifold.OutOfMemory()))
				{
					newCompositeManifolds.PostObjectCollisionFinalize();
					SPU_ONLY(ObjectState::GetSpuCollisionBuffers()->ReleaseAllMarkers());
					PF_STOP_2(MidphaseLoop);
					return;
				}

				if(useCompositeManifolds && virtualManifold.HasManifold())
				{
					newCompositeManifolds.PostLeafCollisionProcessManifold();
				}
			}
			else
			{
				const bool shouldSwapObjects = objectStates[0].ShouldSubdivideOtherObjectState(objectStates[1]);
				if(shouldSwapObjects)
				{
					// Swap object states.
					objectStates[0].SwapWith(objectStates[1]);
					objectStates[0].InitCullingBoxInfo(objectStates[1]);
				}

				objectStates[0].Iterate(&collisionWorkStack, objectStates[1]);
				continue;
			}
		}

		if(!collisionWorkStack.IsEmpty())
		{
			// Pop new object states off of stack and into objectStates[0] and objectStates[1].
			objectStates[0].Pop(&collisionWorkStack, objectStates[1]);
		}
		else
		{
			// Collision detection for these two objects is done.
			if(useCompositeManifolds)
			{
				newCompositeManifolds.PostObjectCollisionFinalize();
			}

			// This wait corresponds to DMAs that were issued as a result of the ObjectState::Init() calls at the top of this function (the SPU
			//   collision buffers are going out of scope now and we need to make sure that we don't have any pending DMAs into that memory).
			SPU_ONLY(sysDmaWaitTagStatusAll(SYS_DMA_MASK_ALL));
			PF_STOP_2(MidphaseLoop);
			return;
		}
	}
}
#endif // !USE_NEW_MID_PHASE || ALLOW_MID_PHASE_SWAP

void phMidphase::ProcessSelfCollision(const phCollisionInput &unswappedInput, u8 *selfCollisionPairsA, u8 *selfCollisionPairsB, int numSelfCollisionPairs) const
{
	Assert(unswappedInput.boundA != NULL);
	Assert(unswappedInput.boundA->GetType() == phBound::COMPOSITE);
	const phBoundComposite *boundComposite = static_cast<const phBoundComposite *>(unswappedInput.boundA);
#if USE_NEW_SELF_COLLISION

#if ALLOW_MID_PHASE_SWAP
	if (g_UseNewMidPhaseCollision)
		ProcessSelfCollisionNew(*boundComposite, unswappedInput.currentA, unswappedInput.lastA, unswappedInput.rootManifold, selfCollisionPairsA, selfCollisionPairsB, numSelfCollisionPairs, unswappedInput.highPriority);
	else
		ProcessSelfCollisionOriginal(*boundComposite, unswappedInput.currentA, unswappedInput.lastA, unswappedInput.rootManifold, selfCollisionPairsA, selfCollisionPairsB, numSelfCollisionPairs, unswappedInput.highPriority);
#else // ALLOW_MID_PHASE_SWAP
	ProcessSelfCollisionNew(*boundComposite, unswappedInput.currentA, unswappedInput.lastA, unswappedInput.rootManifold, selfCollisionPairsA, selfCollisionPairsB, numSelfCollisionPairs, unswappedInput.highPriority);
#endif // ALLOW_MID_PHASE_SWAP

#else // USE_NEW_SELF_COLLISION
	
	ProcessSelfCollisionOriginal(*boundComposite, unswappedInput.currentA, unswappedInput.lastA, unswappedInput.rootManifold, selfCollisionPairsA, selfCollisionPairsB, numSelfCollisionPairs, unswappedInput.highPriority);

#endif // USE_NEW_SELF_COLLISION
}

#if USE_NEW_SELF_COLLISION
void phMidphase::ProcessSelfCollisionNew(const phBoundComposite &boundComposite, Mat34V_In curInstanceMatrix, Mat34V_In lastInstanceMatrix, phManifold *rootManifold,
									  u8 *selfCollisionPairsA, u8 *selfCollisionPairsB, int numSelfCollisionPairs, bool highPriorityManifolds) const
{
	rootManifold->EnableCompositeManifolds();
	if(Unlikely(rootManifold->m_CompositePointers == NULL))
	{
		// We need composite manifolds but we're not going to be able to have any way of keeping track of them.  Just abort the collision.
		return;
	}

	ComponentInfoManager componentManager;
	componentManager.InitComponentInfoMap(boundComposite.GetNumBounds());
	componentManager.m_componentAllocator.SetMarker();

	GJKCacheQueryInput gjkCacheQI;
	gjkCacheQI.SetCacheDatabase(true,GJKCACHESYSTEM,rootManifold);
	gjkCacheQI.SetPartIndex(0);

	NewCollisionInput collisionInput;
	collisionInput.set(phManifold::GetManifoldMarginV(),NULL,NULL,&gjkCacheQI,NULL);

	DiscreteCollisionDetectorInterface::SimpleResult result;

	phPool<phManifold>* manifoldPool = PHMANIFOLD;
	phCollisionCompositeManifoldMgr<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS> newCompositeManifolds(rootManifold, manifoldPool);
#if DONT_USE_COMPONENT_CACHE
	ComponentInfoWithBoundBuffer ciABuffer;
	ComponentInfoWithBoundBuffer ciBBuffer;
#endif

	for(int pairIndex = 0; Likely(pairIndex < numSelfCollisionPairs); pairIndex++)
	{
		const int curComponentA = selfCollisionPairsA[pairIndex];
		const int curComponentB = selfCollisionPairsB[pairIndex];

		PPC_STAT_COUNTER_INC(SelfCollisionIters,1);
#if DONT_USE_COMPONENT_CACHE
		ComponentInfo * componentInfoA = &ciABuffer;
		ComponentInfo * componentInfoB = &ciBBuffer;
		componentInfoA->Init(curComponentA,curInstanceMatrix,lastInstanceMatrix,&boundComposite,&componentManager.m_componentAllocator CI_SPU_ONLY(false) CI_SPU_ONLY(ciABuffer.m_boundBuffer);
		componentInfoB->Init(curComponentB,curInstanceMatrix,lastInstanceMatrix,&boundComposite,&componentManager.m_componentAllocator CI_SPU_ONLY(false) CI_SPU_ONLY(ciBBuffer.m_boundBuffer));
#else

		ComponentInfo * componentInfoA = componentManager.GetComponentInfo(curComponentA);
		if (Unlikely(componentInfoA == NULL))
		{
			PPC_STAT_COUNTER_INC(SelfCollisionMisses,1);
			componentInfoA = componentManager.CreateComponentInfo(curComponentA,curInstanceMatrix,lastInstanceMatrix,&boundComposite CI_SPU_ONLY(false));
			componentManager.SetComponentInfo(curComponentA,componentInfoA);
		}
		else
		{
			PPC_STAT_COUNTER_INC(SelfCollisionHits,1);
		}
		ComponentInfo * componentInfoB = componentManager.GetComponentInfo(curComponentB);
		if (Unlikely(componentInfoB == NULL))
		{
			PPC_STAT_COUNTER_INC(SelfCollisionMisses,1);
			componentInfoB = componentManager.CreateComponentInfo(curComponentB,curInstanceMatrix,lastInstanceMatrix,&boundComposite CI_SPU_ONLY(false));
			componentManager.SetComponentInfo(curComponentB,componentInfoB);
		}
		else
		{
			PPC_STAT_COUNTER_INC(SelfCollisionHits,1);
		}
		if (Unlikely(componentInfoA == NULL || componentInfoB == NULL))
		{
			componentManager.ResetBuffer();
			componentInfoA = componentManager.CreateComponentInfo(curComponentA,curInstanceMatrix,lastInstanceMatrix,&boundComposite CI_SPU_ONLY(false));
			componentInfoB = componentManager.CreateComponentInfo(curComponentB,curInstanceMatrix,lastInstanceMatrix,&boundComposite CI_SPU_ONLY(false));
			FastAssert(componentInfoA);
			FastAssert(componentInfoB);
			componentManager.SetComponentInfo(curComponentA,componentInfoA);
			componentManager.SetComponentInfo(curComponentB,componentInfoB);
		}
#endif // DONT_USE_COMPONENT_CACHE
		FastAssert(componentInfoA);
		FastAssert(componentInfoB);

		const NewCollisionObject * objectA = &componentInfoA->m_object;
		const NewCollisionObject * objectB = &componentInfoB->m_object;

		if (OBBTest(*componentInfoA,*componentInfoB))
		{
			gjkCacheQI.SetComponentIndex((u32)curComponentA,(u32)curComponentB);
			FastAssert(gjkCacheQI.m_PartIndex == 0);
			collisionInput.object0 = objectA;
			collisionInput.object1 = objectB;

			phPairwiseCollisionProcessor::ProcessPairwiseCollision(result,collisionInput);

			if(result.GetHasResult())
			{
				phManifold * compositeManifold = newCompositeManifolds.PreLeafCollisionFindManifold(curComponentA, curComponentB, objectA->m_bound, objectB->m_bound, highPriorityManifolds);
				if (Likely(compositeManifold))
				{
					phManifoldResult manifoldResult(objectA->m_bound, objectB->m_bound, compositeManifold);
					manifoldResult.ProcessResult_NoSwap(objectA->m_current, objectB->m_current, result);
					newCompositeManifolds.PostLeafCollisionProcessManifold();
				}
				else
				{
					break;
				}
				result.SetHasResult(false);
			}
		}
	}

	newCompositeManifolds.PostObjectCollisionFinalize();
	componentManager.m_componentAllocator.ReleaseAllMarkers();
}
#endif // USE_NEW_SELF_COLLISION

#if !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP
void phMidphase::ProcessSelfCollisionOriginal(const phBoundComposite &boundComposite, Mat34V_In curInstanceMatrix, Mat34V_In lastInstanceMatrix, phManifold *rootManifold,
						  u8 *selfCollisionPairsA, u8 *selfCollisionPairsB, int numSelfCollisionPairs, bool highPriorityManifolds) const
{
#if __SPU
	u8 collisionBuffer[23 * 1024];
	phSpuCollisionBuffers spuCollisionBuffers(collisionBuffer, sizeof(collisionBuffer));
	ObjectState::SetSpuCollisionBuffers(&spuCollisionBuffers);
	spuCollisionBuffers.SetMarker();
	spuCollisionBuffers.SetMarker();

	u8 boundBufferA[phBound::MAX_BOUND_SIZE];
	u8 boundBufferB[phBound::MAX_BOUND_SIZE];
	const phBound *boundA = reinterpret_cast<const phBound *>(boundBufferA);
	const phBound *boundB = reinterpret_cast<const phBound *>(boundBufferB);
#endif	// __SPU

	GJKCacheQueryInput gjkCacheQI;
	gjkCacheQI.SetCacheDatabase(true,GJKCACHESYSTEM,rootManifold);

	rootManifold->EnableCompositeManifolds();
	if(Unlikely(rootManifold->m_CompositePointers == NULL))
	{
		// We need composite manifolds but we're not going to be able to have any way of keeping track of them.  Just abort the collision.
		SPU_ONLY(ObjectState::GetSpuCollisionBuffers()->ReleaseAllMarkers());
		return;
	}

	phPool<phManifold>* manifoldPool = PHMANIFOLD;
	phCollisionCompositeManifoldMgr<phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS> newCompositeManifolds(rootManifold, manifoldPool);
	ObjectState objectStateA, objectStateB;
	ProcessLeafCollisionManifold virtualManifold(&newCompositeManifolds,highPriorityManifolds);

	for(int pairIndex = 0; Likely(pairIndex < numSelfCollisionPairs); pairIndex++)
	{
		u8 curComponentA = selfCollisionPairsA[pairIndex];
		u8 curComponentB = selfCollisionPairsB[pairIndex];

#if __SPU
		sysDmaLargeGet(boundBufferA, (uint64_t)(boundComposite.GetBound(curComponentA)), phBound::MAX_BOUND_SIZE, DMA_TAG(1));
		sysDmaLargeGet(boundBufferB, (uint64_t)(boundComposite.GetBound(curComponentB)), phBound::MAX_BOUND_SIZE, DMA_TAG(1));

		spuCollisionBuffers.ReleaseToLastMarker();	// Release the memory allocated for bound B.
#else	// __SPU
		const phBound *boundA = boundComposite.GetBound(curComponentA);
		const phBound *boundB = boundComposite.GetBound(curComponentB);
#endif	// __SPU

		Mat34V currentBoundToWorldMatrixA, lastBoundToWorldMatrixA, currentBoundToWorldMatrixB, lastBoundToWorldMatrixB;
		Transform(currentBoundToWorldMatrixA, curInstanceMatrix, boundComposite.GetCurrentMatrix(curComponentA));
		Transform(lastBoundToWorldMatrixA, lastInstanceMatrix, boundComposite.GetLastMatrix(curComponentA));
		Transform(currentBoundToWorldMatrixB, curInstanceMatrix, boundComposite.GetCurrentMatrix(curComponentB));
		Transform(lastBoundToWorldMatrixB, lastInstanceMatrix, boundComposite.GetLastMatrix(curComponentB));

		SPU_ONLY(sysDmaWaitTagStatusAll(DMA_MASK(1)));

#if __SPU
		spuCollisionBuffers.ReleaseToLastMarker();	// Release the memory allocated for bound A.
		spuCollisionBuffers.SetMarker();			// Mark where we are before allocate new memory for bound A.
#endif	// __SPU

		objectStateA.Init(boundA, currentBoundToWorldMatrixA, lastBoundToWorldMatrixA, 0);
		objectStateA.SetTypeAndIncludeFlags(TYPE_FLAGS_ALL, INCLUDE_FLAGS_ALL);

		SPU_ONLY(spuCollisionBuffers.SetMarker());	// Mark where we are before allocate new memory for bound B.
		objectStateB.Init(boundB, currentBoundToWorldMatrixB, lastBoundToWorldMatrixB, 1);
		objectStateB.SetTypeAndIncludeFlags(TYPE_FLAGS_ALL, INCLUDE_FLAGS_ALL);
		objectStateA.InitCullingBoxInfo(objectStateB);

		FastAssert(objectStateA.IsLeaf() && objectStateB.IsLeaf());
		if(objectStateA.TestOverlap(objectStateB))
		{
			virtualManifold.SetComponents(rootManifold,curComponentA,curComponentB);
			gjkCacheQI.SetComponentIndex((u32)curComponentA,(u32)curComponentB);

			// Self collision doesn't need a collisionMemory structure since it doesn't involve primitive BVHs.
			ProcessLeafCollision(objectStateA, objectStateB, &virtualManifold, &gjkCacheQI, NULL);
			if (Unlikely(virtualManifold.OutOfMemory()))
				break;
			
			if (virtualManifold.HasManifold())
				newCompositeManifolds.PostLeafCollisionProcessManifold();
		}
	}

#if __SPU
	spuCollisionBuffers.ReleaseToLastMarker();	// Release the memory allocated for bound B.
	spuCollisionBuffers.ReleaseToLastMarker();	// Release the memory allocated for bound A.
#endif	// __SPU

	newCompositeManifolds.PostObjectCollisionFinalize();
}
#endif // !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP

#endif	// !defined(__SPURS_JOB__)

} // namespace rage
