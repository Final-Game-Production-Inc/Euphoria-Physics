// 
// physics/levelnew.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "levelnew.h"

#include "btAxisSweep1.h"
#include "btAxisSweep3.h"
#include "btSpatialHash.h"
#include "btNxN.h"
#include "collisionoverlaptest.h"
#include "debugEvents.h"
#include "inst.h"
#include "intersection.h"
#include "iterator.h"
#include "levelbroadphase.h"
#include "shapetest.h"

#include "phbound/boundcomposite.h"
#include "vectormath/classes.h"

#if !__SPU
#include "simulator.h"
#endif // !__SPU

#include "bank/bank.h"

#if __PFDRAW
#include "physics/collider.h"
#endif // __PFDRAW

#include "diag/art_channel.h"
#include "phbullet/SimdTransformUtil.h"
#include "phcore/segment.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "system/cache.h"
#include "system/param.h"
#include "system/timemgr.h"		// Had to add this when integrating a changeset from RDR2.  This really shouldn't be a dependency here though.

#if __PFDRAW
#include "vector/colors.h"
#endif // __PFDRAW

#if __NMDRAW
#include "nmext/NMRenderBuffer.h"
#endif

#if __SPU
#include "system/dma.h"
#include <cell/dma.h>
#endif

#if DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
#include "system/stack.h"
#endif

#define FAST_LEVELNEW			1

#define PHLEVELNEW_MAX_DEFERRED_BVH_UPDATE_HANDLES 300

#define PHLEVELNEW_EVERYWHERE_NODE_INDEX 8191

namespace rage {

CompileTimeAssertSize(phLooseQuadtreeNode, 32, 32);
CompileTimeAssertSize(phLooseOctreeNode, 48, 48);

#if __ASSERT
#ifdef GTA_REPLAY_RAGE
bool	noPlacementAssert = false;
void	SetNoPlacementAssert(bool var) { noPlacementAssert = var; }
#endif	//GTA_REPLAY_RAGE
#endif	//__ASSERT

int sm_spatialHashLevel = 7;
//!me this number should really depend on the size of the cell you want in the first level...
int sm_spatialHashStartGridResolution = 256;

#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
template <class __NodeType> bool phLevelNodeTree<__NodeType>::sm_useSecondaryBroadphaseForBatchAdd = false;
template <class __NodeType> int phLevelNodeTree<__NodeType>::sm_batchAddThresholdForSecondaryBroadphase = 7;
#endif // !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE

#if __BANK
template <class __NodeType> int phLevelNodeTree<__NodeType>::sm_MaxOctreeNodesEverUsed = 0;
template <class __NodeType> int phLevelNodeTree<__NodeType>::sm_MaxActiveObjectsEver = 0;
template <class __NodeType> int phLevelNodeTree<__NodeType>::sm_MaxObjectsEver = 0;
template <class __NodeType> int phLevelNodeTree<__NodeType>::sm_TotalObjectsAdded = 0;
template <class __NodeType> int phLevelNodeTree<__NodeType>::sm_TotalObjectsDeleted = 0;
template <class __NodeType> bool phLevelNodeTree<__NodeType>::sm_DisableInactiveCollidesAgainstInactive = false;
template <class __NodeType> bool phLevelNodeTree<__NodeType>::sm_DisableInactiveCollidesAgainstFixed = false;
#endif
#if !__SPU && __PFDRAW
template <class __NodeType> phLevelNew::ColorChoiceFunc phLevelNodeTree<__NodeType>::sm_ColorChoiceFunc;
#endif

// Convenience function for wrapping DMA/DMA emulation.
#if __SPU || EMULATE_SPU
__forceinline void TransferData(void *localMemory, const void *mainMemory, u32 size)
{
#if __SPU
//	if((((int)(localMemory)) & 15) != 0)
//	{
//		SPU_PRINTF("!!! DMA destination %u is not 16-byte aligned.\n", (int)(localMemory));
//	}
//	if((((int)(mainMemory)) & 15) != 0)
//	{
//		SPU_PRINTF("!!! DMA source %u is not 16-byte aligned.\n", (int)(mainMemory));
//	}
//	if((size & 15) != 0)
//	{
//		SPU_PRINTF("!!! DMA size %u is not a multiple of 16!\n", size);
//	}

//	SPU_PRINTF("About to DMA %u bytes from %p to %p.\n", size, mainMemory, localMemory);
	// Gives more info when it goes wrong - want to move this change across to rage/dev
	sysDmaLargeGet(localMemory, (u32)(mainMemory), size, DMA_TAG(16));
	sysDmaWaitTagStatusAll(DMA_MASK(16));
#else
	sysMemCpy(localMemory, mainMemory, size);
#endif
}
#endif


#if __SPU
class phSpuLevelContext
{
public:
	__forceinline void FetchInstanceArchetypeAndBound(const phObjectData &spuObjectData)
	{
		const phInst *ppuInst = spuObjectData.GetInstance();
		sysDmaLargeGet(&m_InstanceBuffer[0], (uint64_t)ppuInst, sizeof(phInst), DMA_TAG(2));
		const phInst *spuInst = reinterpret_cast<const phInst *>(&m_InstanceBuffer[0]);
		sysDmaWaitTagStatusAll(DMA_MASK(2));

		const phArchetype *ppuArchetype = spuInst->GetArchetype();
		sysDmaLargeGet(&m_ArchetypeBuffer[0], (uint64_t)ppuArchetype, sizeof(phArchetype), DMA_TAG(2));
		const phArchetype *spuArchetype = reinterpret_cast<const phArchetype *>(&m_ArchetypeBuffer[0]);
		sysDmaWaitTagStatusAll(DMA_MASK(2));

		const phBound *ppuBound = spuArchetype->GetBound();
		sysDmaLargeGet(&m_BoundBuffer[0], (uint64_t)ppuBound, sizeof(phBound), DMA_TAG(2));
		sysDmaWaitTagStatusAll(DMA_MASK(2));
	}

	__forceinline const phInst *GetSpuInstance() const
	{
		return reinterpret_cast<const phInst *>(&m_InstanceBuffer[0]);
	}

	__forceinline const phArchetype *GetSpuArchetype() const
	{
		return reinterpret_cast<const phArchetype *>(&m_ArchetypeBuffer[0]);
	}

	__forceinline const phBound *GetSpuBound() const
	{
		return reinterpret_cast<const phBound *>(&m_BoundBuffer[0]);
	}

private:
	u8 m_InstanceBuffer[sizeof(phInst)] ;
	u8 m_ArchetypeBuffer[sizeof(phArchetype)] ;
	u8 m_BoundBuffer[sizeof(phBound)] ;
};
#endif	// __SPU

#if __WIN32
#pragma warning( disable:4244 )
#endif

#if !__SPU
template <class __NodeType> phLevelNodeTree<__NodeType> *phLevelNodeTree<__NodeType>::sm_ActiveInstance = NULL;
#endif


//////////////////////////////////////////////////////////////////////////
/// PHLEVEL_DEBUG_CONSISTENCY_CHECKS is used to enable internal consistency
/// checks at strategic points in the phLevelNodeTree class.
/// NOTE: There are some additional DO_CONSISTENCY_CHECK calls that are commented out
/// and you may find it useful to enable them if you encounter problems with this class.
#define PHLEVEL_DEBUG_CONSISTENCY_CHECKS (0 && __DEV)

#if PHLEVEL_DEBUG_CONSISTENCY_CHECKS
#define DO_CONSISTENCY_CHECK(x,y) DoConsistencyChecks(x,y)
#else
#define DO_CONSISTENCY_CHECK(x,y)
#endif

//////////////////////////////////////////////////////////////////////////
/// Extra consistency checks include checking for duplicate nodes and
/// duplicate objects.
///
#define LEVELNEW_EXTRACONSISTENCYCHECKS	(0 && __DEV)


// This #define controls an extra set of Asserts that ensure that some of the internals of the level are working correctly.
// If you are ever making any changes to the level you should turn this back to and make sure that none of the asserts that
//   it controls are failing.  None of the asserts that this controls should ever be able to be made to fail by users of
//   phLevelNodeTree.
#define LEVELNEW_INTERNALCONSISTENCY_ASSERTS 0
#if LEVELNEW_INTERNALCONSISTENCY_ASSERTS
#define PHLEVELNEW_ASSERT(x)	Assert(x)
#else
#define PHLEVELNEW_ASSERT(x)
#endif


#if FAST_LEVELNEW
// TODO: I'm not really sure how to make this work with templates.  It seems like we would need a different CompileTimeAssert() for each 'type' of
//   phLevelNodeTree.  If that's the case, then this should really be in leveldefs.h
//	CompileTimeAssert( (phLevelNodeTree::knMaxOctreeDepth == 18) );	// if this fails    kfLeafNodeWidthFactor has to be recomputed to the correct value
	const float kfLeafNodeWidthFactor = 7.62939453125E-06f; //1.0f / (float)(1 << 17);
#else
	const float kfLeafNodeWidthFactor = 1.0f / (float)(1 << (phLevelNodeTree::knMaxOctreeDepth - 1));
#endif

#if !__SPU

EXT_PFD_DECLARE_ITEM(Centroid);
EXT_PFD_DECLARE_ITEM(CenterOfGravity);
EXT_PFD_DECLARE_ITEM(AngularInertia);
EXT_PFD_DECLARE_ITEM(ColliderAngularInertia);
EXT_PFD_DECLARE_ITEM(InvertAngularInertia);
EXT_PFD_DECLARE_ITEM_SLIDER(AngularInertiaScale);
EXT_PFD_DECLARE_ITEM(CullSpheres);
EXT_PFD_DECLARE_ITEM(CullBoxes);
EXT_PFD_DECLARE_ITEM(CullSolid);
EXT_PFD_DECLARE_ITEM_SLIDER_INT(CullOpacity);
EXT_PFD_DECLARE_ITEM(LevelIndices);
EXT_PFD_DECLARE_ITEM(InactiveCollidesVsInactive);
EXT_PFD_DECLARE_ITEM(InactiveCollidesVsFixed);
EXT_PFD_DECLARE_ITEM(InstMatrices);

EXT_PFD_DECLARE_GROUP(Bounds);
EXT_PFD_DECLARE_ITEM(Solid);
EXT_PFD_DECLARE_ITEM(SolidBoundLighting);
EXT_PFD_DECLARE_ITEM(Wireframe);
EXT_PFD_DECLARE_ITEM(Active);
EXT_PFD_DECLARE_ITEM(Inactive);
EXT_PFD_DECLARE_ITEM(Fixed);
EXT_PFD_DECLARE_ITEM(SupportPoints);
EXT_PFD_DECLARE_ITEM(AnimateFromLast);
EXT_PFD_DECLARE_ITEM_SLIDER(AnimateTime);
EXT_PFD_DECLARE_ITEM(DrawBoundMaterials);
EXT_PFD_DECLARE_ITEM(BoundNames);
EXT_PFD_DECLARE_ITEM(BoundTypes);
EXT_PFD_DECLARE_ITEM_SLIDER_INT(HighlightFlags);
EXT_PFD_DECLARE_ITEM_SLIDER(BoundDrawDistance);
EXT_PFD_DECLARE_ITEM(ThinPolys);
EXT_PFD_DECLARE_ITEM(BadNormalPolys);
EXT_PFD_DECLARE_ITEM(BadNeighborPolys);
EXT_PFD_DECLARE_ITEM(BigPrimitives);
EXT_PFD_DECLARE_ITEM_SLIDER(AttentionSphere);

EXT_PFD_DECLARE_GROUP(TypeFlagFilter);
EXT_PFD_DECLARE_ITEM(TypeFlag0);
EXT_PFD_DECLARE_ITEM(TypeFlag1);
EXT_PFD_DECLARE_ITEM(TypeFlag2);
EXT_PFD_DECLARE_ITEM(TypeFlag3);
EXT_PFD_DECLARE_ITEM(TypeFlag4);
EXT_PFD_DECLARE_ITEM(TypeFlag5);
EXT_PFD_DECLARE_ITEM(TypeFlag6);
EXT_PFD_DECLARE_ITEM(TypeFlag7);
EXT_PFD_DECLARE_ITEM(TypeFlag8);
EXT_PFD_DECLARE_ITEM(TypeFlag9);
EXT_PFD_DECLARE_ITEM(TypeFlag10);
EXT_PFD_DECLARE_ITEM(TypeFlag11);
EXT_PFD_DECLARE_ITEM(TypeFlag12);
EXT_PFD_DECLARE_ITEM(TypeFlag13);
EXT_PFD_DECLARE_ITEM(TypeFlag14);
EXT_PFD_DECLARE_ITEM(TypeFlag15);
EXT_PFD_DECLARE_ITEM(TypeFlag16);
EXT_PFD_DECLARE_ITEM(TypeFlag17);
EXT_PFD_DECLARE_ITEM(TypeFlag18);
EXT_PFD_DECLARE_ITEM(TypeFlag19);
EXT_PFD_DECLARE_ITEM(TypeFlag20);
EXT_PFD_DECLARE_ITEM(TypeFlag21);
EXT_PFD_DECLARE_ITEM(TypeFlag22);
EXT_PFD_DECLARE_ITEM(TypeFlag23);
EXT_PFD_DECLARE_ITEM(TypeFlag24);
EXT_PFD_DECLARE_ITEM(TypeFlag25);
EXT_PFD_DECLARE_ITEM(TypeFlag26);
EXT_PFD_DECLARE_ITEM(TypeFlag27);
EXT_PFD_DECLARE_ITEM(TypeFlag28);
EXT_PFD_DECLARE_ITEM(TypeFlag29);
EXT_PFD_DECLARE_ITEM(TypeFlag30);
EXT_PFD_DECLARE_ITEM(TypeFlag31);

EXT_PFD_DECLARE_GROUP(IncludeFlagFilter);
EXT_PFD_DECLARE_ITEM(IncludeFlag0);
EXT_PFD_DECLARE_ITEM(IncludeFlag1);
EXT_PFD_DECLARE_ITEM(IncludeFlag2);
EXT_PFD_DECLARE_ITEM(IncludeFlag3);
EXT_PFD_DECLARE_ITEM(IncludeFlag4);
EXT_PFD_DECLARE_ITEM(IncludeFlag5);
EXT_PFD_DECLARE_ITEM(IncludeFlag6);
EXT_PFD_DECLARE_ITEM(IncludeFlag7);
EXT_PFD_DECLARE_ITEM(IncludeFlag8);
EXT_PFD_DECLARE_ITEM(IncludeFlag9);
EXT_PFD_DECLARE_ITEM(IncludeFlag10);
EXT_PFD_DECLARE_ITEM(IncludeFlag11);
EXT_PFD_DECLARE_ITEM(IncludeFlag12);
EXT_PFD_DECLARE_ITEM(IncludeFlag13);
EXT_PFD_DECLARE_ITEM(IncludeFlag14);
EXT_PFD_DECLARE_ITEM(IncludeFlag15);
EXT_PFD_DECLARE_ITEM(IncludeFlag16);
EXT_PFD_DECLARE_ITEM(IncludeFlag17);
EXT_PFD_DECLARE_ITEM(IncludeFlag18);
EXT_PFD_DECLARE_ITEM(IncludeFlag19);
EXT_PFD_DECLARE_ITEM(IncludeFlag20);
EXT_PFD_DECLARE_ITEM(IncludeFlag21);
EXT_PFD_DECLARE_ITEM(IncludeFlag22);
EXT_PFD_DECLARE_ITEM(IncludeFlag23);
EXT_PFD_DECLARE_ITEM(IncludeFlag24);
EXT_PFD_DECLARE_ITEM(IncludeFlag25);
EXT_PFD_DECLARE_ITEM(IncludeFlag26);
EXT_PFD_DECLARE_ITEM(IncludeFlag27);
EXT_PFD_DECLARE_ITEM(IncludeFlag28);
EXT_PFD_DECLARE_ITEM(IncludeFlag29);
EXT_PFD_DECLARE_ITEM(IncludeFlag30);
EXT_PFD_DECLARE_ITEM(IncludeFlag31);

EXT_PFD_DECLARE_ITEM_SLIDER(NormalLength);
EXT_PFD_DECLARE_ITEM(Face);
EXT_PFD_DECLARE_ITEM(Edge);

EXT_PFD_DECLARE_ITEM(ProbeSegments);
EXT_PFD_DECLARE_ITEM(ProbeIsects);
EXT_PFD_DECLARE_ITEM(ProbeNormals);
EXT_PFD_DECLARE_ITEM(EdgeSegments);
EXT_PFD_DECLARE_ITEM(EdgeIsects);
EXT_PFD_DECLARE_ITEM(EdgeNormals);
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
EXT_PFD_DECLARE_ITEM(SphereSegments);
EXT_PFD_DECLARE_ITEM(SphereIsects);
EXT_PFD_DECLARE_ITEM(SphereNormals);


namespace PhysicsLevelStats
{
	PF_PAGE(LevelPage,"ph Level");

	PF_GROUP(AddAndDeleteObjects);
	PF_LINK(LevelPage,AddAndDeleteObjects);
	PF_TIMER(AddObject,AddAndDeleteObjects);
	PF_TIMER(DeleteObject,AddAndDeleteObjects);
	PF_TIMER(CommitDeferredOctreeUpdates,AddAndDeleteObjects);
};

using namespace PhysicsLevelStats;

namespace BroadphaseCollisionStats
{
	PF_PAGE(BroadphasePage, "ph Broadphase Collision" );

	PF_GROUP(Broadphase);
	PF_LINK(BroadphasePage, Broadphase);
	PF_TIMER(AddObjectsBroadPhase, Broadphase );
	PF_TIMER(AddObjectsSecondaryBroadPhase, Broadphase );
	PF_TIMER(AddObjectsLevel, Broadphase );
	PF_TIMER(DeleteObjectsBroadPhase, Broadphase );
	PF_TIMER(DeleteObjectsSecondaryBroadPhase, Broadphase);
	PF_TIMER(PruneBroadphase, Broadphase );
	PF_TIMER(SortPairs, Broadphase );
	PF_TIMER(TestOverlaps, Broadphase );
};

using namespace BroadphaseCollisionStats;

phLooseOctreeNode::phLooseOctreeNode()
{
	Clear();
}


phLooseOctreeNode::~phLooseOctreeNode()
{
}

void phLooseOctreeNode::Clear()
{
	m_ChildNodeIndices[0] = (u16)(-1);
	m_ChildNodeIndices[1] = (u16)(-1);
	m_ChildNodeIndices[2] = (u16)(-1);
	m_ChildNodeIndices[3] = (u16)(-1);
	m_ChildNodeIndices[4] = (u16)(-1);
	m_ChildNodeIndices[5] = (u16)(-1);
	m_ChildNodeIndices[6] = (u16)(-1);
	m_ChildNodeIndices[7] = (u16)(-1);

	m_ParentNodeIndex = (u16)(-1);
	m_ChildNodeCount = 0;

	m_ContainedObjectCount = 0;

	m_ContainedObjectStartIdx = phInst::INVALID_INDEX;
	m_CenterXYZHalfWidthW = Vec4V(V_ZERO);

	m_VisitingObjectCount = 0;
}
#endif	// !__SPU

void phLooseOctreeNode::InitExtentsAsChild(const phLooseOctreeNode * XENON_ONLY(RESTRICT) const kpkParentNode, const int knChildIndex)
{
#if 0
	// Possibly better implementation by keeping everything in the float pipeline ... this function still could use some improvement though.
	Vec3f Center;
	LoadAsScalar(Center, kpkParentNode->m_CenterXYZHalfWidthW);
	const float kfHalfHalfWidth = 0.5f * kpkParentNode->m_CenterXYZHalfWidthW.GetWf();
	const float kfNegHalfHalfWidth = -0.5f * kpkParentNode->m_CenterXYZHalfWidthW.GetWf();
	Center.SetX(Center.GetX() + ((knChildIndex & 1) == 0 ? kfNegHalfHalfWidth : kfHalfHalfWidth));
	Center.SetY(Center.GetY() + ((knChildIndex & 2) == 0 ? kfNegHalfHalfWidth : kfHalfHalfWidth));
	Center.SetZ(Center.GetZ() + ((knChildIndex & 4) == 0 ? kfNegHalfHalfWidth : kfHalfHalfWidth));
	StoreAsScalar(m_Center, Center);
#else
	m_CenterXYZHalfWidthW = kpkParentNode->m_CenterXYZHalfWidthW;
	RC_VECTOR3(m_CenterXYZHalfWidthW).x += (knChildIndex & 1) == 0 ? -0.5f * kpkParentNode->m_CenterXYZHalfWidthW.GetWf() : 0.5f * kpkParentNode->m_CenterXYZHalfWidthW.GetWf();
	RC_VECTOR3(m_CenterXYZHalfWidthW).y += (knChildIndex & 2) == 0 ? -0.5f * kpkParentNode->m_CenterXYZHalfWidthW.GetWf() : 0.5f * kpkParentNode->m_CenterXYZHalfWidthW.GetWf();
	RC_VECTOR3(m_CenterXYZHalfWidthW).z += (knChildIndex & 4) == 0 ? -0.5f * kpkParentNode->m_CenterXYZHalfWidthW.GetWf() : 0.5f * kpkParentNode->m_CenterXYZHalfWidthW.GetWf();
#endif

	m_CenterXYZHalfWidthW.SetW(ScalarV(V_HALF)*kpkParentNode->m_CenterXYZHalfWidthW.GetW());
}

void phLooseOctreeNode::ConnectParent(const int knParentNodeIndex )
{
	PHLEVELNEW_ASSERT(m_ParentNodeIndex == (u16)(-1));
	m_ParentNodeIndex = (u16)(knParentNodeIndex);
}

void phLooseOctreeNode::ConnectChild(const int knChildIndex, const int knChildNodeIndex)
{
	PHLEVELNEW_ASSERT(knChildIndex >= 0);
	PHLEVELNEW_ASSERT(knChildIndex < 8);
	PHLEVELNEW_ASSERT(m_ChildNodeCount < 8);
	PHLEVELNEW_ASSERT(m_ChildNodeIndices[knChildIndex] == (u16)(-1));

	m_ChildNodeIndices[knChildIndex] = (u16)(knChildNodeIndex);
	FastAssert(m_ChildNodeCount < 8);
	++m_ChildNodeCount;
}

void phLooseOctreeNode::DisconnectParent()
{
	PHLEVELNEW_ASSERT(m_ParentNodeIndex != (u16)(-1));
	m_ParentNodeIndex = (u16)(-1);
}

void phLooseOctreeNode::DisconnectChild(const int knChildIndex)
{
	PHLEVELNEW_ASSERT(knChildIndex >= 0);
	PHLEVELNEW_ASSERT(knChildIndex < 8);
	PHLEVELNEW_ASSERT(m_ChildNodeCount > 0);
	PHLEVELNEW_ASSERT(m_ChildNodeIndices[knChildIndex] != (u16)(-1));

	m_ChildNodeIndices[knChildIndex] = (u16)(-1);
	--m_ChildNodeCount;
}


#if !__SPU
phLooseQuadtreeNode::phLooseQuadtreeNode()
{
	Clear();
}


phLooseQuadtreeNode::~phLooseQuadtreeNode()
{
}

void phLooseQuadtreeNode::Clear()
{
	m_ChildNodeIndices[0] = (u16)(-1);
	m_ChildNodeIndices[1] = (u16)(-1);
	m_ChildNodeIndices[2] = (u16)(-1);
	m_ChildNodeIndices[3] = (u16)(-1);

	m_ParentNodeIndex = (u16)(-1);
	m_ChildNodeCount = 0;

	m_ContainedObjectCount = 0;

	m_ContainedObjectStartIdx = phInst::INVALID_INDEX;
	m_CenterXYZHalfWidthW = Vec4V(V_ZERO);

	m_VisitingObjectCount = 0;
}
#endif	// !__SPU

void phLooseQuadtreeNode::InitExtentsAsChild(const phLooseQuadtreeNode * XENON_ONLY(RESTRICT) const kpkParentNode, const int knChildIndex)
{
	Assert(knChildIndex < 4);

	// The parent node should be in memory so this should keep everything in the float pipeline.  Unfortunately there's still going to be some branching below.
	const float kfParentHalfWidth = kpkParentNode->m_CenterXYZHalfWidthW.GetWf();
	const float kfHalfWidth = 0.5f * kfParentHalfWidth;
	const float kfNegHalfWidth = -0.5f * kfParentHalfWidth;
	m_CenterXYZHalfWidthW.SetXf(kpkParentNode->m_CenterXYZHalfWidthW.GetXf() + ((knChildIndex & 1) == 0 ? kfNegHalfWidth : kfHalfWidth));
	m_CenterXYZHalfWidthW.SetYf(kpkParentNode->m_CenterXYZHalfWidthW.GetYf() + ((knChildIndex & 2) == 0 ? kfNegHalfWidth : kfHalfWidth));
	m_CenterXYZHalfWidthW.SetWf(kfHalfWidth);
}

void phLooseQuadtreeNode::ConnectParent(const int knParentNodeIndex )
{
	PHLEVELNEW_ASSERT(m_ParentNodeIndex == (u16)(-1));
	m_ParentNodeIndex = (u16)(knParentNodeIndex);
}

void phLooseQuadtreeNode::ConnectChild(const int knChildIndex, const int knChildNodeIndex)
{
	PHLEVELNEW_ASSERT(knChildIndex >= 0);
	PHLEVELNEW_ASSERT(knChildIndex < 4);
	PHLEVELNEW_ASSERT(m_ChildNodeCount < 4);
	PHLEVELNEW_ASSERT(m_ChildNodeIndices[knChildIndex] == (u16)(-1));

	m_ChildNodeIndices[knChildIndex] = (u16)(knChildNodeIndex);
	FastAssert(m_ChildNodeCount < 4);
	++m_ChildNodeCount;
}

void phLooseQuadtreeNode::DisconnectParent()
{
	PHLEVELNEW_ASSERT(m_ParentNodeIndex != (u16)(-1));
	m_ParentNodeIndex = (u16)(-1);
}

void phLooseQuadtreeNode::DisconnectChild(const int knChildIndex)
{
	PHLEVELNEW_ASSERT(knChildIndex >= 0);
	PHLEVELNEW_ASSERT(knChildIndex < 4);
	PHLEVELNEW_ASSERT(m_ChildNodeCount > 0);
	PHLEVELNEW_ASSERT(m_ChildNodeIndices[knChildIndex] != (u16)(-1));

	m_ChildNodeIndices[knChildIndex] = (u16)(-1);
	--m_ChildNodeCount;
}


#if !__SPU
PARAM(createchild,"[physics] phLevelNodeTree::m_nMinObjectsInNodeToCreateChild");
PARAM(collapsechild,"[physics] phLevelNodeTree::m_nMaxObjectsInNodeToCollapseChild");
PARAM(maxcollisionpairs, "[physics] Override the maximum number of broadphase pairs.");

template <class __NodeType> phLevelNodeTree<__NodeType>::phLevelNodeTree() 
	: phLevelBase()
	, m_InstOutOfWorldCallback(InstOutOfWorldCallback(DefaultNotifyOutOfWorld))
	, m_MultipleUpdates(false)
	, m_CurrentMaxLevelIndex(-1)
	, m_bOnlyMainThreadIsUpdatingPhysics(false)
#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	, m_pauDeferredUpdateLevelIndices(NULL)
	, m_uNumDeferredUpdateLevelIndices(0)
	, m_uMaxDeferredUpdateLevelIndices(1)
	, m_DeferredOctreeUpdateEnabled(false)
#endif
#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	, m_DeferredCompositeBvhUpdateEnabled(false)
#endif
#if __BANK
	, m_DebugPOICenter(V_ZERO)
#endif
{

#if __ASSERT
	m_threadId = sysIpcGetCurrentThreadId();
#endif

	m_nActiveIteratorIndex = -1;						// No active iteration.

	m_LevelCenter = Vec3V(V_ZERO);
	m_fLevelHalfWidth = 0.0f;

    m_MaxCollisionPairs = 10240;
	PARAM_maxcollisionpairs.Get(m_MaxCollisionPairs);

//	m_nMinObjectsInNodeToCreateChild = 6;
//	m_nMaxObjectsInNodeToCollapseChild = 5;
	int minObjectsInNodeToCreateChild;
	int maxObjectsInNodeToCollapseChild;
	if (PARAM_createchild.Get(minObjectsInNodeToCreateChild) == false)
	{
		minObjectsInNodeToCreateChild = 5;
	}
	if (PARAM_collapsechild.Get(maxObjectsInNodeToCollapseChild) == false)
	{
		maxObjectsInNodeToCollapseChild = 3;
	}
	SetOctreeNodeCreationParameters(minObjectsInNodeToCreateChild, maxObjectsInNodeToCollapseChild);

	m_paObjectData = NULL;
#if LEVELNEW_GENERATION_IDS
	m_pauGenerationIDs = NULL;
#endif
	m_pauSleepIslands = NULL;
	m_pauObjectDataIndexList = NULL;
	m_pauObjectUserData = NULL;

	m_pauActiveObjectLevelIndex = NULL;

	m_uTotalOctreeNodeCount = 0;
	m_uOctreeNodesInUse = 0;
	m_paOctreeNodes = NULL;
#if LEVELNEW_USE_EVERYWHERE_NODE
	m_uFirstObjectInEverywhereNodeIndex = (u16)(-1);
#endif
	m_pauOctreeNodeIndexList = NULL;
	m_pInstLastMatrixIndices = NULL;
	m_MaxInstLastMatrices = 0;
	m_pInstLastMatrices = NULL;
	m_BroadPhaseType = AXISSWEEP3;
	m_BroadPhase = NULL;
}

template <class __NodeType> phLevelNodeTree<__NodeType>::~phLevelNodeTree()
{
	Assert(m_paObjectData == NULL);
#if LEVELNEW_GENERATION_IDS
	Assert(m_pauGenerationIDs == NULL);
#endif
	Assert(m_pauSleepIslands == NULL);
	Assert(m_pauObjectDataIndexList == NULL);
	Assert(m_pauObjectUserData == NULL);
	Assert(m_pauActiveObjectLevelIndex == NULL);

	Assert(m_uTotalOctreeNodeCount == 0);
	Assert(m_uOctreeNodesInUse == 0);
	Assert(m_paOctreeNodes == NULL);
	Assert(m_pauOctreeNodeIndexList == NULL);
	Assert(m_pInstLastMatrixIndices == NULL);
	Assert(m_MaxInstLastMatrices == 0);
	Assert(m_pInstLastMatrices == NULL);
	Assert(m_FreeInstLastMatrices.GetCapacity() == 0);
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::InitBroadphase( btOverlappingPairCache *existingPairCache, bool transferHandles )
{
	Vec3V levelHalfWidthV = Vec3VFromF32(m_fLevelHalfWidth);

	Vector3 *aabbMin = NULL;
	Vector3 *aabbMax = NULL;
	int *pOwner = NULL;
	u16 nHandles = 0;

	if( m_BroadPhase != NULL )
	{
		// don't delete cache if we are transfering from another BP
		if( existingPairCache == m_BroadPhase->m_pairCache )
		{
			if( transferHandles == true )
			{
				m_BroadPhase->prepareCacheForTransfer();
				nHandles = m_BroadPhase->getNumHandles();
				aabbMax = Alloca( Vector3, nHandles );
				aabbMin = Alloca( Vector3, nHandles );
				pOwner = Alloca( int, nHandles );
				m_BroadPhase->getHandles( &aabbMin, &aabbMax, &pOwner, nHandles );
			}

			m_BroadPhase->m_pairCache = NULL;
		}

		delete m_BroadPhase;
	}

#if ENABLE_UNUSED_PHYSICS_CODE
	switch ( m_BroadPhaseType )
	{
	case AXISSWEEP3:
		m_BroadPhase = rage_new btAxisSweep3(this, m_MaxObjects, m_MaxCollisionPairs, Vector3((m_LevelCenter - levelHalfWidthV).GetIntrin128()), Vector3((m_LevelCenter + levelHalfWidthV).GetIntrin128()), existingPairCache);
		break;
	case AXISSWEEP1:
		m_BroadPhase = rage_new btAxisSweep1(m_MaxObjects, m_MaxCollisionPairs, existingPairCache);
		break;
	case LEVEL:
		m_BroadPhase = rage_new phLevelBroadPhase(this, m_MaxObjects, m_MaxCollisionPairs, existingPairCache);
		break;
	case SPATIALHASH:
		m_BroadPhase = rage_new btSpatialHash(m_MaxObjects, m_MaxCollisionPairs, Vector3((m_LevelCenter - levelHalfWidthV).GetIntrin128()), Vector3((m_LevelCenter + levelHalfWidthV).GetIntrin128()), existingPairCache, sm_spatialHashLevel, sm_spatialHashStartGridResolution );
		break;
	case NXN:
		m_BroadPhase = rage_new btNxN(m_MaxObjects, m_MaxCollisionPairs, existingPairCache);
		break;
	default:
		break;
	}
#else // ENABLE_UNUSED_PHYSICS_CODE
	// AXISSWEEP3
	m_BroadPhase = rage_new btAxisSweep3(this, m_MaxObjects, m_MaxCollisionPairs, Vector3((m_LevelCenter - levelHalfWidthV).GetIntrin128()), Vector3((m_LevelCenter + levelHalfWidthV).GetIntrin128()), existingPairCache);
#endif // ENABLE_UNUSED_PHYSICS_CODE

	if( transferHandles == true )
	{
		m_BroadPhase->addHandlesNoNewPairs( aabbMin, aabbMax, pOwner, nHandles );
	}
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::Init ()
{
	phLevelBase::Init();

	m_paObjectData = rage_new phObjectData[m_MaxObjects];
#if LEVELNEW_GENERATION_IDS
	m_pauGenerationIDs = rage_new u16[m_MaxObjects];
#endif
	m_pauSleepIslands = rage_new phSleepIsland::Id[m_MaxObjects];
 	m_pauObjectDataIndexList = rage_new u16[m_MaxObjects];
	m_pauObjectUserData = rage_new void *[m_MaxObjects];
	sysMemSet(m_pauObjectUserData, 0, sizeof(void*) * m_MaxObjects);

	for (int nObjectIndex = 0; nObjectIndex < m_MaxObjects; ++nObjectIndex)
	{
		m_paObjectData[nObjectIndex].SetState(phLevelBase::OBJECTSTATE_NONEXISTENT);
#if LEVELNEW_GENERATION_IDS
		m_pauGenerationIDs[nObjectIndex] = 0;
#endif
		m_pauSleepIslands[nObjectIndex].index = 0xffff;
		m_pauSleepIslands[nObjectIndex].generationId = 0xffff;
		m_pauObjectDataIndexList[nObjectIndex] = (u16)(nObjectIndex);
	}

	m_pauActiveObjectLevelIndex = rage_new u16[m_MaxActive];

	m_paOctreeNodes = rage_new __NodeType[m_uTotalOctreeNodeCount];
	m_pauOctreeNodeIndexList = rage_new u16[m_uTotalOctreeNodeCount];

	int nOctreeNodeIndex;
	for(nOctreeNodeIndex = 0; nOctreeNodeIndex < m_uTotalOctreeNodeCount; ++nOctreeNodeIndex)
	{
		m_pauOctreeNodeIndexList[nOctreeNodeIndex] = (u16)(nOctreeNodeIndex);
	}

	Assert(m_MaxInstLastMatrices < INVALID_INST_LAST_MATRIX_INDEX);
#if (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	m_pInstLastMatrixIndices = rage_new u16[m_MaxObjects];
	memset(m_pInstLastMatrixIndices, 0xFF, m_MaxObjects * sizeof(u16));
#else
	m_pInstLastMatrixIndices = rage_new u8[m_MaxObjects];
	memset(m_pInstLastMatrixIndices, 0xFF, m_MaxObjects);
#endif // (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	m_pInstLastMatrices      = rage_new Mat34V[m_MaxInstLastMatrices];

	m_FreeInstLastMatrices.Reserve(m_MaxInstLastMatrices);

	for(int i = 0; i < m_MaxInstLastMatrices; ++i)
	{
		m_FreeInstLastMatrices.Push(i);
	}

	InitBroadphase();
#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	m_secondaryBroadphase = rage_new phLevelBroadPhase(this, m_MaxObjects, m_MaxCollisionPairs);
#endif	// !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE

	// Let's get a root octree node.
	u16 uRootOctreeNodeIndex = GetOctreeNodeIndexFromPool();
	Assert(uRootOctreeNodeIndex == 0);
	__NodeType *pRootOctreeNode = GetOctreeNode(uRootOctreeNodeIndex);
	pRootOctreeNode->m_CenterXYZHalfWidthW = Vec4V(m_LevelCenter, ScalarVFromF32(m_fLevelHalfWidth));
	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	Assert(m_pauDeferredUpdateLevelIndices == NULL);
	Assert(m_uNumDeferredUpdateLevelIndices == 0);
	Assert(m_uMaxDeferredUpdateLevelIndices > 0);
	m_pauDeferredUpdateLevelIndices = rage_new u16[m_uMaxDeferredUpdateLevelIndices];
	m_DeferredOctreeUpdateLevelIndices.Init(m_MaxObjects);
#endif

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	m_UpdateCompositeBvhLevelIndices.Init(m_MaxObjects);
	m_FullRebuildCompositeBvhLevelIndices.Init(m_MaxObjects);
	m_DeferredBvhUpdateCompositeLevelIndices.Reserve(PHLEVELNEW_MAX_DEFERRED_BVH_UPDATE_HANDLES);
#endif // LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE

	NON_SPU_ONLY(phShapeTestTaskData::InitDefault();)
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::Clear()
{
	Assert(m_nActiveIteratorIndex == -1);						// No active iteration.

	phLevelBase::Clear();
	Assert(m_NumObjects == 0);

	// This is slightly different from phLevelNodeTree::Reinit() because this also resets the states in the object data.
	// I don't know of any good reason not to do that in Reinit(), but I didn't want to change existing behavior.
	for(int nObjectIndex = 0; nObjectIndex < m_MaxObjects; ++nObjectIndex)
	{
		m_paObjectData[nObjectIndex].SetState(phLevelBase::OBJECTSTATE_NONEXISTENT);
#if LEVELNEW_GENERATION_IDS
		m_pauGenerationIDs[nObjectIndex] = 0;
#endif
		m_pauSleepIslands[nObjectIndex].index = 0xffff;
		m_pauSleepIslands[nObjectIndex].generationId = 0xffff;
		m_pauObjectDataIndexList[nObjectIndex] = (u16)(nObjectIndex);
	}

	// Return all of the octree nodes to the available pool.
	for(int nOctreeNodeIndex = 0; nOctreeNodeIndex < m_uTotalOctreeNodeCount; ++nOctreeNodeIndex)
	{
		m_pauOctreeNodeIndexList[nOctreeNodeIndex] = (u16)(nOctreeNodeIndex);
	}

	for(int nOctreeNodeIndex = m_uTotalOctreeNodeCount - 1; nOctreeNodeIndex >= 0; --nOctreeNodeIndex)
	{
		GetOctreeNode(nOctreeNodeIndex)->Clear();
	}
	m_uOctreeNodesInUse = 0;


	u16 uRootOctreeNodeIndex = GetOctreeNodeIndexFromPool();
	Assert(uRootOctreeNodeIndex == 0);
	__NodeType *pRootOctreeNode = GetOctreeNode(uRootOctreeNodeIndex);
	pRootOctreeNode->m_CenterXYZHalfWidthW = Vec4V(m_LevelCenter, ScalarVFromF32(m_fLevelHalfWidth));

#if (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	memset(m_pInstLastMatrixIndices, 0xFF, m_MaxObjects * sizeof(u16));
#else
	memset(m_pInstLastMatrixIndices, 0xFF, m_MaxObjects);
#endif // (RSG_PC || RSG_DURANGO || RSG_ORBIS)
	m_FreeInstLastMatrices.ResetCount();
	for(int i = 0; i < m_MaxInstLastMatrices; ++i)
	{
		m_FreeInstLastMatrices.Push(i);
	}

	m_BroadPhase->clear();
#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	m_secondaryBroadphase->clear();
#endif	// !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	m_DeferredOctreeUpdateLevelIndices.Reset();
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
}


#if !__SPU
template <class __NodeType> void phLevelNodeTree<__NodeType>::Shutdown()
{
#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	m_DeferredOctreeUpdateLevelIndices.Kill();
	delete [] m_pauDeferredUpdateLevelIndices;
	m_pauDeferredUpdateLevelIndices = NULL;
	m_uMaxDeferredUpdateLevelIndices = 0;
	m_uNumDeferredUpdateLevelIndices = 0;
#endif

	delete m_BroadPhase;

#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	delete m_secondaryBroadphase;
#endif	// !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE

	delete [] m_pInstLastMatrices;
	m_pInstLastMatrices = NULL;
	delete [] m_pInstLastMatrixIndices;
	m_pInstLastMatrixIndices = NULL;
	m_MaxInstLastMatrices = 0;
	m_FreeInstLastMatrices.Reset();

	delete [] m_pauOctreeNodeIndexList;
	m_pauOctreeNodeIndexList = NULL;
	delete [] m_paOctreeNodes;
	m_paOctreeNodes = NULL;

	m_uOctreeNodesInUse = 0;
	m_uTotalOctreeNodeCount = 0;

	delete [] m_pauActiveObjectLevelIndex;
	m_pauActiveObjectLevelIndex = NULL;

	delete [] m_pauObjectUserData;
	m_pauObjectUserData = NULL;
	delete [] m_pauObjectDataIndexList;
	m_pauObjectDataIndexList = NULL;
#if LEVELNEW_GENERATION_IDS
	delete [] m_pauGenerationIDs;
	m_pauGenerationIDs = NULL;
#endif
	delete [] m_pauSleepIslands;
	m_pauSleepIslands = NULL;
	delete [] m_paObjectData;
	m_paObjectData = NULL;

	phLevelBase::Shutdown();

	// clear out the static ActiveInstance pointer if we are the active level (PHLEVEL)
	if (GetActiveInstance() == this)
	{
		SetActiveInstance(NULL);
	}
}
#endif


template <class __NodeType> void phLevelNodeTree<__NodeType>::SetMaxObjects(const int knMaxObjectCount)
{
//    Assert(!IsInitialized());
	m_MaxObjects = (u16)(knMaxObjectCount);
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::SetExtents (Vec3V_In levelMin, Vec3V_In levelMax)
{
//    Assert(!IsInitialized());

	// Find the center from the given extents.
	m_LevelCenter = Average( levelMin, levelMax );

	// Find the half-width from the given extents.
	m_fLevelHalfWidth = 0.5f * Max(levelMax.GetXf()-levelMin.GetXf(), levelMax.GetYf()-levelMin.GetYf(), levelMax.GetZf()-levelMin.GetZf());
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::SetExtents (const Vector3& levelCenter, float levelHalfWidth)
{
	RC_VECTOR3(m_LevelCenter) = levelCenter;
	m_fLevelHalfWidth = levelHalfWidth;
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::SetNumOctreeNodes(const int knOctreeNodeCount)
{
//    Assert(!IsInitialized()); 
	Assert(knOctreeNodeCount < PHLEVELNEW_EVERYWHERE_NODE_INDEX);
	m_uTotalOctreeNodeCount = (u16)(knOctreeNodeCount);
}


#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
template <class __NodeType> void phLevelNodeTree<__NodeType>::SetMaxDeferredInstanceCount(int maxDeferredInstanceCount)
{
//	Assert(!IsInitialized()); 
	Assert(maxDeferredInstanceCount <= 65535);
	m_uMaxDeferredUpdateLevelIndices = maxDeferredInstanceCount;
}
#endif


template <class __NodeType> void phLevelNodeTree<__NodeType>::AddToBroadphase( phInst *pInst, int uLevelIndex )
{
	PF_FUNC(AddObjectsBroadPhase);
	
	Vec3V extents, boxCenter;
	pInst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(extents, boxCenter);
	
	Mat34V_ConstRef mtx = pInst->GetMatrix();
	extents = geomBoxes::ComputeAABBExtentsFromOBB(mtx.GetMat33ConstRef(), extents);
	boxCenter = Transform(mtx, boxCenter);

	const Vec3V maxs = boxCenter + extents;
	const Vec3V mins = boxCenter - extents;

	m_BroadPhase->addHandle( RCC_VECTOR3(mins), RCC_VECTOR3(maxs), uLevelIndex );
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::AddToBroadphase( phInst **pInst, int nCount, int *levelIndex )
{
	PF_FUNC(AddObjectsBroadPhase);
	// No physics level locking is needed here because the broadphase is responsible for its own synchronization.

	Vector3 *mins = Alloca( Vector3,nCount );
	Vector3 *maxs = Alloca( Vector3,nCount );

//	ASSERT_ONLY(Vector3 levelHalfWidth(m_fLevelHalfWidth,m_fLevelHalfWidth,m_fLevelHalfWidth));
//	ASSERT_ONLY(const Vector3 worldMin(RCC_VECTOR3(m_LevelCenter) - levelHalfWidth));
//	ASSERT_ONLY(const Vector3 worldMax(RCC_VECTOR3(m_LevelCenter) + levelHalfWidth));
	int iInst;
	for( iInst = 0; iInst < nCount; iInst++ )
	{
		Vector3 extents, boxCenter;
		pInst[iInst]->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(RC_VEC3V(extents), RC_VEC3V(boxCenter));
		Mat34V_ConstRef mtx = pInst[iInst]->GetMatrix();
		extents = VEC3V_TO_VECTOR3(geomBoxes::ComputeAABBExtentsFromOBB(mtx.GetMat33ConstRef(), RCC_VEC3V(extents)));
		boxCenter = VEC3V_TO_VECTOR3(Transform(mtx, RCC_VEC3V(boxCenter)));

		maxs[iInst].Add(boxCenter, extents);
		mins[iInst].Subtract(boxCenter, extents);

		// We've had the occasional assert in btAxisSweep3 where these asserts are failing, but there was no information about the inst at that 
		// point so the asserts have been mostly duplicated here so when they fail we can see what it is related to.

		Assertf(!mins[iInst].IsGreaterThan(maxs[iInst]), 
			"Invalid bounds extents - %s min:%f %f %f max:%f %f %f", 
			pInst[iInst]->GetArchetype()->GetFilename(), 
			mins[iInst].x, mins[iInst].y, mins[iInst].z, 
			maxs[iInst].x, maxs[iInst].y, maxs[iInst].z);

		// Disabling these asserts because some clients allow instances to be out of the world in certain ways.  In the future we should probably
		//   have a #define that allows clients to specify on which axes they want objects to be checked.
//		Assertf(mins[iInst].IsGreaterThan(worldMin) && mins[iInst].IsLessThan(worldMax), 
//			"Invalid bound minimum - %s min:%f %f %f m_worldAabbMin:%f %f %f m_worldAabbMax:%f %f %f", 
//			pInst[iInst]->GetArchetype()->GetFilename(), 
//			mins[iInst].x, mins[iInst].y, mins[iInst].z,
//			worldMin.x, worldMin.y, worldMin.z,
//			worldMax.x, worldMax.y, worldMax.z
//			);

//		Assertf(maxs[iInst].IsGreaterThan(worldMin) && maxs[iInst].IsLessThan(worldMax), 
//			"Invalid bound maximum - %s max:%f %f %f m_worldAabbMin:%f %f %f m_worldAabbMax:%f %f %f", 
//			pInst[iInst]->GetArchetype()->GetFilename(),
//			maxs[iInst].x, maxs[iInst].y, maxs[iInst].z,
//			worldMin.x, worldMin.y, worldMin.z,
//			worldMax.x, worldMax.y, worldMax.z
//			);
	}

#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	if(sm_useSecondaryBroadphaseForBatchAdd && nCount > sm_batchAddThresholdForSecondaryBroadphase)
	{
		btOverlappingPairCache *swapCache = m_secondaryBroadphase->m_pairCache;
		m_secondaryBroadphase->m_pairCache = m_BroadPhase->m_pairCache;
		m_secondaryBroadphase->pruneActiveOverlappingPairs();
	//	m_BroadPhase->m_pairCache = m_secondaryBroadphase->m_pairCache;
		m_secondaryBroadphase->m_pairCache = swapCache;
		m_BroadPhase->addHandlesNoNewPairs( mins, maxs, levelIndex, nCount );
	}
	else
#endif	// !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	{
		m_BroadPhase->addHandles( mins, maxs, levelIndex, nCount );
	}
}

template <class __NodeType> int phLevelNodeTree<__NodeType>::AddObjectHelper(phInst* pInst, eObjectState type, void* userData )
{
	PF_FUNC(AddObjectsLevel);

#if LEVELNEW_EXTRACONSISTENCYCHECKS
	CheckForDuplicateObject(pInst);
#endif
	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);

#if !__FINAL
	if (pInst->GetLevelIndex()!=phInst::INVALID_INDEX)
	{
		// In non-assert builds, quit if the object is already in the level, because otherwise this can cause mysterious crashes.
		char buffer[512];
		Quitf("Inserting an object [%s] that is already in the level is a fatal error.",  GetInstDescription(*pInst, buffer, sizeof(buffer)));
		// This won't get reached of course. 
		return phInst::INVALID_INDEX;
	}
#endif

	Assert(pInst->GetArchetype() != NULL);
	Assertf(((const Matrix34*)(&pInst->GetMatrix()))->IsOrthonormal(), "Object '%s' is being inserted with a non-orthonormal matrix!", pInst->GetArchetype()->GetFilename());

	if (m_NumObjects >= m_MaxObjects)
	{
#if __BANK
		DumpObjects();
#endif
		Assertf(m_NumObjects < m_MaxObjects, "There are %d physics objects, but the maximum allowed is only %d.", m_NumObjects, m_MaxObjects);
		return phInst::INVALID_INDEX;
	}

	u16 uLevelIndex = GetObjectIndexFromPool();
	phObjectData *pObjectData = &(m_paObjectData[uLevelIndex]);

	pInst->SetLevelIndex(uLevelIndex);
	pObjectData->SetInstance(pInst);
	SetState(uLevelIndex, type);
	// Clear the instance states in case they were left set by the user
	pObjectData->ClearCollidesWithInactive();
	pObjectData->ClearCollidesWithFixed();

	Assert(pInst->GetArchetype()->GetBound() != NULL);
	const Vec3V center = pInst->GetArchetype()->GetBound()->GetWorldCentroid( pInst->GetMatrix() );
	float fRadius = pInst->GetArchetype()->GetBound()->GetRadiusAroundCentroid();
	Assertf(fRadius > 0.0f, "Object with zero radius being inserted into the physics level: %s (%p / %p / %p) at <%f, %f, %f>", pInst->GetArchetype()->GetFilename(), pInst, pInst->GetArchetype(), pInst->GetArchetype()->GetBound(), center.GetXf(), center.GetYf(), center.GetZf());

	// Initialize the object data with the current instance information (bounding sphere center and radius and type and include flags).
	// TODO: We calculate/fetch the center and radius above and then also do so inside of this function.  Do something more efficient instead.
	InitObjectData(*pObjectData);

	switch ( type )
	{
	case OBJECTSTATE_ACTIVE:	
		Assert(m_NumActive < m_MaxActive);
//		Assert(m_nActiveIteratorIndex == -1);	// It's probably okay to increase the number of active objects during an iteration.
		AddActiveObjectUserData(uLevelIndex, userData);
		++m_NumActive;
		break;
	case OBJECTSTATE_INACTIVE:
		++m_NumInactive;
		break;
	case OBJECTSTATE_FIXED:
		++m_NumFixed;
		break;
	default:
		break;
	}

	if(GetConstOctreeNode(0)->IsPointWithinNode(center))
	{
#if LEVELNEW_INTERNALCONSISTENCY_ASSERTS
		ASSERT_ONLY(u16 uOctreeNodeIndex = )
#endif
		AddObjectToOctree(uLevelIndex, *pObjectData, center, fRadius);
//		PHLEVELNEW_ASSERT(IsObjectContained(uOctreeNodeIndex, uLevelIndex));
	}
	else
	{
		pObjectData->SetOctreeNodeIndex(PHLEVELNEW_EVERYWHERE_NODE_INDEX);
#if LEVELNEW_USE_EVERYWHERE_NODE
		// Add it to the 'everywhere node'.
		AddObjectToEverywhereNode(uLevelIndex, *pObjectData);
#endif
		if(m_InstOutOfWorldCallback.IsBound())
		{
			m_InstOutOfWorldCallback(pInst);
		}
	}

	if(pInst->HasLastMatrix())
	{
		ReserveInstLastMatrixInternal(pInst);
	}

	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);

	return uLevelIndex;
}


template <class __NodeType> int phLevelNodeTree<__NodeType>::AddObject(phInst* pInst, eObjectState type, void* userData, bool delaySAPAdd )
{
	PF_FUNC(AddObject);
#if ENABLE_PHYSICS_LOCK
	PHLOCK_SCOPEDWRITELOCK;
#endif	// ENABLE_PHYSICS_LOCK

	int uLevelIndex = AddObjectHelper( pInst, type, userData );

	if( !delaySAPAdd )
	{
		AddToBroadphase( pInst, uLevelIndex );
	}

	PF_START(AddObjectsSecondaryBroadPhase);
#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	m_secondaryBroadphase->addHandle( VEC3_ZERO, VEC3_ZERO, uLevelIndex );
#endif	// !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	PF_STOP(AddObjectsSecondaryBroadPhase);

	return uLevelIndex;
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::AddObjects(phInst** pInst, int nCount, int *levelIndexOut, eObjectState type, void** userData )
{
	PF_FUNC(AddObject);
#if ENABLE_PHYSICS_LOCK
	PHLOCK_SCOPEDWRITELOCK;
#endif	// ENABLE_PHYSICS_LOCK

	int iInst;
	for( iInst = 0; iInst < nCount; iInst++ )
	{
		void *ud = ( userData ) ? userData[iInst] : NULL;

		levelIndexOut[iInst] = AddObjectHelper( pInst[iInst], type, ud );
		PF_START(AddObjectsSecondaryBroadPhase);
#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
		m_secondaryBroadphase->addHandle( VEC3_ZERO, VEC3_ZERO, levelIndexOut[iInst] );
#endif	// !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
		PF_STOP(AddObjectsSecondaryBroadPhase);
	}

	AddToBroadphase( pInst, nCount, levelIndexOut );
}

template <class __NodeType> void* phLevelNodeTree<__NodeType>::GetUserData(int levelIndex) const
{
	// Currently, only active objects are allowed to have data.
	Assert(IsActive(levelIndex));
	return GetActiveObjectUserData(levelIndex);
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::DeleteObject(phInst *pInst, bool delaySAPRemove)
{
#if ENABLE_PHYSICS_LOCK
	PHLOCK_SCOPEDWRITELOCK;
#endif	// ENABLE_PHYSICS_LOCK

	Assert(pInst->GetArchetype() != NULL);
	Assert(pInst->GetArchetype()->GetBound() != NULL);
	DeleteObject(pInst->GetLevelIndex(), delaySAPRemove);
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::DeleteObject(int nLevelIndex, bool delaySAPRemove)
{
	PF_FUNC(DeleteObject);
#if ENABLE_PHYSICS_LOCK
	PHLOCK_SCOPEDWRITELOCK;
#endif	// ENABLE_PHYSICS_LOCK
	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, false);

	DeleteObject1( nLevelIndex );
	if( !delaySAPRemove )
	{
		PF_START( DeleteObjectsBroadPhase );
	m_BroadPhase->removeHandles( &nLevelIndex, 1 );
		PF_STOP( DeleteObjectsBroadPhase );
	}
	PF_START(DeleteObjectsSecondaryBroadPhase);
#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	m_secondaryBroadphase->removeHandle( nLevelIndex );
#endif	// !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
	PF_STOP(DeleteObjectsSecondaryBroadPhase);
	DeleteObject2( nLevelIndex );

	if( !delaySAPRemove )
	{
		ReturnObjectIndexToPool((u16)(nLevelIndex));
	}

	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, false);
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::CommitDelayedDeleteObjects(int *levelIndex, int nCount)
{
	PF_FUNC(DeleteObject);

#if ENABLE_PHYSICS_LOCK
	PHLOCK_SCOPEDWRITELOCK;
#endif	// ENABLE_PHYSICS_LOCK

	PF_START(DeleteObjectsBroadPhase);
	m_BroadPhase->removeHandles( levelIndex, nCount );	
	PF_STOP(DeleteObjectsBroadPhase);
	int i;
	for( i = 0; i < nCount; i++ )
	{
		ReturnObjectIndexToPool((u16)(levelIndex[i]));
	}
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::DeleteObject1(int levelIndex)
{
	Assert(GetInstance(levelIndex)->HasLastMatrix() || m_MaxInstLastMatrices == 0 || 
		m_pInstLastMatrixIndices[levelIndex] == INVALID_INST_LAST_MATRIX_INDEX);
	
	if(GetInstance(levelIndex)->HasLastMatrix())
	{
		ReleaseInstLastMatrixInternal(GetInstance(levelIndex));
	}

#if LEVELNEW_GENERATION_IDS
	++m_pauGenerationIDs[levelIndex];		// This is expected to wrap-around.
#endif

	switch(GetState(levelIndex))
	{
	case OBJECTSTATE_ACTIVE:
		{
			Assert(m_nActiveIteratorIndex == -1);
			RemoveActiveObjectUserData(levelIndex);
			Assert(m_NumActive > 0);
			--m_NumActive;
			break;
		}
	case OBJECTSTATE_INACTIVE:
		{
			Assert(m_NumInactive > 0);
			--m_NumInactive;
			break;
		}
	default:
		{
			Assert(IsFixed(levelIndex));
			Assert(m_NumFixed > 0);
			--m_NumFixed;
			break;
		}
	}

	phObjectData *pCurObjectData = &m_paObjectData[levelIndex];
	const int knOctreeNodeIndex = pCurObjectData->GetOctreeNodeIndex();

	if(knOctreeNodeIndex == PHLEVELNEW_EVERYWHERE_NODE_INDEX)
	{
		// The object was out of the world and hence wasn't in the octree.
#if LEVELNEW_USE_EVERYWHERE_NODE
		// Remove the object from the 'everywhere node'.
		RemoveObjectFromEverywhereNode(levelIndex, *pCurObjectData);
#endif
	}
	else
	{
		const float kfRadius = pCurObjectData->m_CachedCenterAndRadius.GetWf();
		__NodeType *initialOctreeNode = GetOctreeNode(pCurObjectData->GetOctreeNodeIndex());
		RemoveObjectFromOctree(initialOctreeNode, (u32)levelIndex, *pCurObjectData, kfRadius);
	}
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::DeleteObjects(int *levelIndex, int count)
{
	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, false);
	PF_FUNC(DeleteObject);

	int iLevelIndex;
	for( iLevelIndex = 0; iLevelIndex < count; iLevelIndex++ )
	{
		DeleteObject1( levelIndex[iLevelIndex] );
	}

	PF_START( DeleteObjectsBroadPhase );
    m_BroadPhase->removeHandles( levelIndex, count );
	PF_STOP( DeleteObjectsBroadPhase );

	for( iLevelIndex = 0; iLevelIndex < count; iLevelIndex++ )
	{
		PF_START(DeleteObjectsSecondaryBroadPhase);
#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
		m_secondaryBroadphase->removeHandle( levelIndex[iLevelIndex] );
#endif	// !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
		PF_STOP(DeleteObjectsSecondaryBroadPhase);
		DeleteObject2( levelIndex[iLevelIndex] );
		ReturnObjectIndexToPool((u16)(levelIndex[iLevelIndex]));
	}

	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, false);
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::DeleteObject2(int levelIndex)
{
	phObjectData *pCurObjectData = &m_paObjectData[levelIndex];
	SetState(levelIndex, OBJECTSTATE_NONEXISTENT);
	pCurObjectData->GetInstance()->SetLevelIndex(phInst::INVALID_INDEX);
	pCurObjectData->SetInstance(NULL);
}


#if 0
template <class __NodeType> void phLevelNodeTree<__NodeType>::ResetLocation(const int knLevelIndex, const Matrix34 &rmtxNew/* = M34_IDENTITY*/)
{
	phObjectData *pCurObjectData = &m_paObjectData[knLevelIndex];
	phInst *pInst = pCurObjectData->m_pInst;
	pInst->SetMatrix(rmtxNew);
	UpdateObjectLocation(knLevelIndex);
}
#endif


template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateObjectArchetype(int levelIndex)
{
	Assert(LegitLevelIndex(levelIndex));
	phObjectData &d = m_paObjectData[levelIndex];
	const phInst *inst = d.GetInstance();
	Assert(inst);

	const phArchetype &arch = *inst->GetArchetype();
	d.m_CachedArchIncludeFlags = arch.GetIncludeFlags();
	d.m_CachedArchTypeFlags = arch.GetTypeFlags();
}


extern bool g_MaintainLooseOctree;
#endif	// !__SPU

template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateObjectPositionInOctree(phObjectData &objectDataToUpdate, int nLevelIndex, Vec3V_In center, float fRadius)
{
	phObjectData *pObjectData = &objectDataToUpdate;
	Assert(pObjectData->GetState() != OBJECTSTATE_NONEXISTENT);

	// Check that the cached radius is the same as the 'live' radius at this point (ensuring that the client didn't call UpdateObjectLocation()
	//   when they should have instead called UpdateObjectLocationAndRadius()).
	ASSERT_ONLY(float cachedRadius = pObjectData->m_CachedCenterAndRadius.GetWf();)
	Assert(fRadius == cachedRadius);
	Assert(FPIsFinite(fRadius));

#if ENABLE_PHYSICS_LOCK
	// This is only called from code paths in which the write lock has already been acquired so there's no need to acquire it again here.
	// TODO: Get rid of this lock.
//	NON_SPU_ONLY(Assert(g_GlobalPhysicsLock.GetPerThreadWriteLockCount() > 0));
	NON_SPU_ONLY(PHLOCK_SCOPEDWRITELOCK);
#endif	// ENABLE_PHYSICS_LOCK

	const u32 initialOctreeNodeIndex = pObjectData->GetOctreeNodeIndex();
	pObjectData->m_CachedCenterAndRadius.SetXYZ(center);

	SPU_ONLY(phSpuObjectBuffer<__NodeType> octreeNodeBuffer);

	SPU_ONLY(phSpuObjectBuffer<__NodeType> rootOctreeNodeBuffer);
	const __NodeType *rootOctreeNode = NON_SPU_ONLY(GetOctreeNode(0)) SPU_ONLY(rootOctreeNodeBuffer.FetchObject(GetOctreeNode(0)));
	if(!rootOctreeNode->IsPointWithinNode(center))
	{
		if(initialOctreeNodeIndex != PHLEVELNEW_EVERYWHERE_NODE_INDEX)
		{
			// The object was within the bounds of the level the last frame, but is no longer this frame.
			OUTPUT_ONLY(char buffer[1024]={0});
			phInst *pInst = pObjectData->GetInstance();
			Warningf("Object '%s' [%d] is out of the bounds of the physics level <%f,%f,%f>!", 
					GetInstDescription(*pInst, buffer, sizeof(buffer)), nLevelIndex, 
					pInst->GetMatrix().GetCol3().GetXf(), 
					pInst->GetMatrix().GetCol3().GetYf(), 
					pInst->GetMatrix().GetCol3().GetZf()
				);
			__NodeType *pInitialOctreeNode = NON_SPU_ONLY(GetOctreeNode(initialOctreeNodeIndex)) SPU_ONLY(octreeNodeBuffer.FetchObject(GetOctreeNode(initialOctreeNodeIndex)));
			RemoveObjectFromOctree(pInitialOctreeNode, (u32)nLevelIndex, *pObjectData, fRadius);
			SPU_ONLY(octreeNodeBuffer.ReturnObject());
			pObjectData->SetOctreeNodeIndex(PHLEVELNEW_EVERYWHERE_NODE_INDEX);
#if LEVELNEW_USE_EVERYWHERE_NODE
			// Add it to the 'everywhere node'.
			AddObjectToEverywhereNode(nLevelIndex, *pObjectData);
#endif
			if(m_InstOutOfWorldCallback.IsBound())
			{
				m_InstOutOfWorldCallback(pInst);
			}
		}
		DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);
		return;
	}

	if(initialOctreeNodeIndex == PHLEVELNEW_EVERYWHERE_NODE_INDEX)
	{
		// This object was outside of the bounds of the level before, but is now in the bounds.  Let's put it back into the octree.
#if LEVELNEW_USE_EVERYWHERE_NODE
		// Remove it from the 'everywhere node'.
		RemoveObjectFromEverywhereNode(nLevelIndex, objectDataToUpdate);
#endif
		AddObjectToOctree((u16)(nLevelIndex), objectDataToUpdate, center, fRadius);
		DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);
		return;
	}

	__NodeType *pInitialOctreeNode = NON_SPU_ONLY(GetOctreeNode(initialOctreeNodeIndex)) SPU_ONLY(octreeNodeBuffer.FetchObject(GetOctreeNode(initialOctreeNodeIndex)));
	PHLEVELNEW_ASSERT(IsObjectContained(pInitialOctreeNode, nLevelIndex SPU_PARAM(objectDataToUpdate)));
	PHLEVELNEW_ASSERT(pInitialOctreeNode->m_ContainedObjectCount > m_nMaxObjectsInNodeToCollapseChild || initialOctreeNodeIndex == 0 || pInitialOctreeNode->m_ChildNodeCount > 0);

	// bOldNodeIsValid will be true iff the object isn't too big to stay in its node AND if the center of the object is still in that node (
	// Basically, if bOldNodeIsValid is true, then we don't need to walk up the octree at all to locate the correct node in which to place this object.
	const bool bOldNodeIsValid = pInitialOctreeNode->IsPointWithinNode(center) && (fRadius <= pInitialOctreeNode->m_CenterXYZHalfWidthW.GetWf() || initialOctreeNodeIndex == 0);
	const bool bIsVisitingObject = fRadius <= 0.5f * pInitialOctreeNode->m_CenterXYZHalfWidthW.GetWf();
	const int knInitialVisitingObjectChildIndex = pObjectData->GetVisitingObjectChildIndex();
	int nFinalVisitingObjectChildIndex;

	// JTODO: These two branches have a lot of repetition and they could be combined.
	if(bOldNodeIsValid)
	{
		// If it's not visiting, or if it is visiting and the child node index has not changed and there is no child node to go into, 
		// then the octree does not need to be updated.
		if(!bIsVisitingObject || ((nFinalVisitingObjectChildIndex = pInitialOctreeNode->SubClassifyPoint(center)) == knInitialVisitingObjectChildIndex && pInitialOctreeNode->m_ChildNodeIndices[nFinalVisitingObjectChildIndex] == (u16)(-1) && pInitialOctreeNode->GetVisitingObjectCount(nFinalVisitingObjectChildIndex) < m_nMinObjectsInNodeToCreateChild))
		{
			// The old octree node is still valid and either the object isn't a visiting object or it hasn't moved over to another child.
			// DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);
			return;
		}

		// The object is still in the same octree node, but it is a visiting object and has moved over another child.
		//PHLEVELNEW_ASSERT(bIsVisitingObject);
		pInitialOctreeNode->DecrementVisitingObjectCount(knInitialVisitingObjectChildIndex);
		//PHLEVELNEW_ASSERT(nFinalVisitingObjectChildIndex != -1);
		if((pInitialOctreeNode->m_ChildNodeIndices[nFinalVisitingObjectChildIndex] != (u16)(-1)) || (pInitialOctreeNode->GetVisitingObjectCount(nFinalVisitingObjectChildIndex) + 1 >= m_nMinObjectsInNodeToCreateChild))
		{
			// The object is in fact able to get pushed farther down the tree.
//			PHLEVELNEW_ASSERT(IsObjectContained(pInitialOctreeNode, nLevelIndex SPU_PARAM(objectDataToUpdate)));
			RemoveObjectFromOctreeNode(pInitialOctreeNode, nLevelIndex, *pObjectData);
			PHLEVELNEW_ASSERT(!IsObjectContained(pInitialOctreeNode, nLevelIndex SPU_PARAM(objectDataToUpdate)));

			InsertObjectIntoSubtree(initialOctreeNodeIndex, pInitialOctreeNode, center, fRadius, nLevelIndex, objectDataToUpdate SPU_PARAM(octreeNodeBuffer));
		}
		else
		{
			// The visiting object just moved over a new child, it wouldn't get moved to a new node.
			// NOTE: We shouldn't have to worry about running out of visiting objects here because as long as m_nMinObjectsInNodeToCreateChild is less than
			//         the max number of visiting objects we'll try to create a child first. 
			pObjectData->SetVisitingObjectChildIndex(nFinalVisitingObjectChildIndex);
			pInitialOctreeNode->IncrementVisitingObjectCount(nFinalVisitingObjectChildIndex);
		}
	}
	else
	{
		if(bIsVisitingObject)
		{
			pInitialOctreeNode->DecrementVisitingObjectCount(knInitialVisitingObjectChildIndex);
		}

		PHLEVELNEW_ASSERT(pInitialOctreeNode->m_ContainedObjectCount > 0);
		PHLEVELNEW_ASSERT(IsObjectContained(pInitialOctreeNode, nLevelIndex SPU_PARAM(objectDataToUpdate)));
		RemoveObjectFromOctreeNode(pInitialOctreeNode, nLevelIndex, *pObjectData);
		PHLEVELNEW_ASSERT(!IsObjectContained(pInitialOctreeNode, nLevelIndex SPU_PARAM(objectDataToUpdate)));

		const u32 postPullOctreeNodeIndex = PullSphere(initialOctreeNodeIndex, pInitialOctreeNode, center, fRadius SPU_PARAM(octreeNodeBuffer));
		__NodeType *pPostPullOctreeNode = NON_SPU_ONLY(GetOctreeNode(postPullOctreeNodeIndex)) SPU_ONLY(octreeNodeBuffer.GetSpuObject());
		Assert(pPostPullOctreeNode->IsPointWithinNode(center));
		Assert(fRadius <= pPostPullOctreeNode->m_CenterXYZHalfWidthW.GetWf());
		InsertObjectIntoSubtree(postPullOctreeNodeIndex, pPostPullOctreeNode, center, fRadius, nLevelIndex, objectDataToUpdate SPU_PARAM(octreeNodeBuffer));
	}
	SPU_ONLY(octreeNodeBuffer.ReturnObject());
	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);
	// Don't call pInst->DebugReplay() here because it can fail, and there's no harm in calling UpdateObjectLocation() inconsistently.
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateObjectRadiusInOctree(phObjectData &objectDataToUpdate, int nLevelIndex, Vec3V_In center, float fRadius)
{
	DO_CONSISTENCY_CHECK((u16)(nLevelIndex), false);

	phObjectData *pObjectData = &objectDataToUpdate;
	Assert(pObjectData->GetState() != OBJECTSTATE_NONEXISTENT);
	const float kfOldRadius = pObjectData->m_CachedCenterAndRadius.GetWf();

	Assert(FPIsFinite(fRadius));

	// Update *only* the radius in the cache at this point.
	pObjectData->m_CachedCenterAndRadius.SetWf(fRadius);

#if !__FINAL && !__SPU
	if(fRadius > GetOctreeNode(0)->m_CenterXYZHalfWidthW.GetWf())
	{
		phInst *pInst = pObjectData->GetInstance();
		Warningf("Object '%s' [%d] has gotten too big for the physics level! (Radius is %f)", pInst->GetArchetype()->GetFilename(), nLevelIndex, fRadius);
		Warningf("The physics level will still work properly, but performance may be very poor and this is probably an error!");
	}
#endif	// !__FINAL

	const int knInitialOctreeNodeIndex = pObjectData->GetOctreeNodeIndex();
	// If knInitialOctreeNodeIndex == 8191 then the object was out of the bounds of the physics level on the previous frame and therefore,
	//   although it was 'in' the level and had a level index, it *wasn't* located in the octree anywhere and therefore we don't need to
	//   do any of the fixing up of anything in the octree that we do below (we couldn't anyway).
	// Re-adding the object to the octree, if necessary, is handled in UpdateObjectLocation() (called below) based on the object's location.
	if(knInitialOctreeNodeIndex != PHLEVELNEW_EVERYWHERE_NODE_INDEX)
	{
#if !__SPU
		__NodeType *pInitialOctreeNode = GetOctreeNode(knInitialOctreeNodeIndex);
#else	// !__SPU
		phSpuObjectBuffer<__NodeType> octreeNodeBuffer;
		__NodeType *pInitialOctreeNode = octreeNodeBuffer.FetchObject(GetOctreeNode(knInitialOctreeNodeIndex));
#endif	// !__SPU
		PHLEVELNEW_ASSERT(IsObjectContained(pInitialOctreeNode, nLevelIndex SPU_PARAM(objectDataToUpdate)));
		const float C = 0.5f * pInitialOctreeNode->m_CenterXYZHalfWidthW.GetWf();

		// We need to check if we need to adjust any visiting object counts due to the change in size.
		if(fRadius > kfOldRadius)
		{
			// The object grew in size - let's check to see if we need to decrement any visiting object counts.
			if(kfOldRadius <= C && fRadius > C )
			{
				pInitialOctreeNode->DecrementVisitingObjectCount(pObjectData->GetVisitingObjectChildIndex());
				SPU_ONLY(octreeNodeBuffer.ReturnObject());
			}
		}
		else if(fRadius < kfOldRadius)	// <- strictly speaking the code in this block works fine if fRadius <= kfOldRadius
		{
			// The object got smaller - let's check to see if we need to increment any visiting object counts.
			if(kfOldRadius > C && fRadius <= C )
			{
				const int knNewChildIndex = pInitialOctreeNode->SubClassifyPoint(center);
				if(pInitialOctreeNode->RoomForAdditionalVisitingObject(knNewChildIndex))
				{
					pInitialOctreeNode->IncrementVisitingObjectCount(knNewChildIndex);
					pObjectData->SetVisitingObjectChildIndex(knNewChildIndex);
				}
				else
				{
					NON_SPU_ONLY(ASSERT_ONLY(DumpNode(*pInitialOctreeNode)));
					Assertf(false,"Ran out of visiting objects in this node. Check log for details.");
					// We don't have enough room for another visiting object, just send it to the everywhere node
					RemoveObjectFromOctreeNode(pInitialOctreeNode, nLevelIndex, *pObjectData);
					pObjectData->SetOctreeNodeIndex(PHLEVELNEW_EVERYWHERE_NODE_INDEX);
					AddObjectToEverywhereNode(nLevelIndex, *pObjectData);
				}
				SPU_ONLY(octreeNodeBuffer.ReturnObject());
			}
		}
	}
}

#if !__SPU
template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateObjectPositionInBroadphase(int nLevelIndex, const Matrix34* lastInstMatrix)
{
	const phObjectData *pObjectData = &m_paObjectData[nLevelIndex];
	const phInst *pInst = pObjectData->GetInstance();
	Assert(pInst->GetArchetype() != NULL);
	Assert(pInst->GetArchetype()->GetBound() != NULL);

	Assertf(RCC_MATRIX34(pInst->GetMatrix()).IsOrthonormal(REJUVENATE_ERROR), "phLevelNodeTree::UpdateObjectPositionInBroadphase(): inst matrix is not orthonormal"
		"\nFilename: %s"
		"\nNon-orthornormality: %f"
		"\n[%5.3f\t %5.3f\t %5.3f\t %5.3f]"
		"\n[%5.3f\t %5.3f\t %5.3f\t %5.3f]"
		"\n[%5.3f\t %5.3f\t %5.3f\t %5.3f]\n",
		pInst->GetArchetype()->GetFilename(),
		RCC_MATRIX34(pInst->GetMatrix()).MeasureNonOrthonormality(REJUVENATE_ERROR),
		pInst->GetMatrix().GetCol0().GetXf(), pInst->GetMatrix().GetCol1().GetXf(), pInst->GetMatrix().GetCol2().GetXf(), pInst->GetMatrix().GetCol3().GetXf(),
		pInst->GetMatrix().GetCol0().GetYf(), pInst->GetMatrix().GetCol1().GetYf(), pInst->GetMatrix().GetCol2().GetYf(), pInst->GetMatrix().GetCol3().GetYf(),
		pInst->GetMatrix().GetCol0().GetZf(), pInst->GetMatrix().GetCol1().GetZf(), pInst->GetMatrix().GetCol2().GetZf(), pInst->GetMatrix().GetCol3().GetZf());

	Vector3 extents, boxCenter;
	pInst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(RC_VEC3V(extents), RC_VEC3V(boxCenter));
	Mat34V_ConstRef current = pInst->GetMatrix();
	if (lastInstMatrix)
	{
#if USE_SAFE_LAST_MATRIX_IN_BOUNDING_VOLUME
		phCollider * collider = static_cast<phCollider*>(GetActiveObjectUserData(nLevelIndex));
		if (collider)
		{
			// Take into account the safe last matrix.
			COT_ExpandOBBFromMotion(current, RCC_MAT34V(*lastInstMatrix), collider->GetLastSafeInstanceMatrix(), RC_VEC3V(extents), RC_VEC3V(boxCenter));
		}
		else
		{
			COT_ExpandOBBFromMotion(current, RCC_MAT34V(*lastInstMatrix), RC_VEC3V(extents), RC_VEC3V(boxCenter));
		}
#else // USE_SAFE_LAST_MATRIX_IN_BOUNDING_VOLUME
		COT_ExpandOBBFromMotion(current, RCC_MAT34V(*lastInstMatrix), RC_VEC3V(extents), RC_VEC3V(boxCenter));
#endif // USE_SAFE_LAST_MATRIX_IN_BOUNDING_VOLUME
		COT_ACE_HalfExpandBoxHalfWidthForSAP(RC_VEC3V(extents));
	}
	extents = VEC3V_TO_VECTOR3(geomBoxes::ComputeAABBExtentsFromOBB(current.GetMat33ConstRef(), RCC_VEC3V(extents)));
	boxCenter = VEC3V_TO_VECTOR3(Transform(current, RCC_VEC3V(boxCenter)));

	Vector3 mins, maxs;
	maxs.Add(boxCenter, extents);
	mins.Subtract(boxCenter, extents);

	// This is somewhat redundant as we also do it inside of m_BroadPhase->updateHandle() but I'm not going to mess with that for now.
	if( m_BroadPhase->isHandleAdded( nLevelIndex ) )
	{
		m_BroadPhase->updateHandle( (u16)nLevelIndex, mins, maxs);
	}
}
#endif	// !__SPU

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
#if !__SPU
#if __DEV
#if RSG_ORBIS
#pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"
#endif
static int s_CurDeferredUpdateAdditions = 0;
static int s_CurUniqueDeferredUpdateAdditions = 0;
static int s_MaxDeferredUpdateAdditions = 0;
static int s_MaxUniqueDeferredUpdateAdditions = 0;
#endif	// __DEV

template <class __NodeType> void phLevelNodeTree<__NodeType>::AddLevelIndexToDeferredOctreeUpdateList(int nLevelIndex)
{
	const bool bOnlyMainThreadIsUpdatingPhysics = this->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_DeferredOctreeUpdateCriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics level now!");
	}

#if __DEV
	++s_CurDeferredUpdateAdditions;
#endif	// __DEV

#if __ASSERT
	Assert(LegitLevelIndex(nLevelIndex));
	Assert(!IsNonexistent(nLevelIndex));
	phArchetype* archetype = m_paObjectData[nLevelIndex].GetInstance()->GetArchetype();
	Assertf(archetype->GetBound()->GetRadiusAroundCentroid() < GetOctreeNode(0)->m_CenterXYZHalfWidthW.GetWf(), "Object '%s' [%d] has gotten too big for the physics level! (Radius is %f)", archetype->GetFilename(), nLevelIndex, archetype->GetBound()->GetRadiusAroundCentroid());
#endif // __ASSERT

	if(!m_DeferredOctreeUpdateLevelIndices.GetAndSet(nLevelIndex))
	{
#if __DEV
		++s_CurUniqueDeferredUpdateAdditions;
#endif	// __DEV
		int numDeferredUpdateLevelIndex = m_uNumDeferredUpdateLevelIndices;
		Assert(numDeferredUpdateLevelIndex <= m_uMaxDeferredUpdateLevelIndices);
		if(numDeferredUpdateLevelIndex == m_uMaxDeferredUpdateLevelIndices)
		{
			// Very unfortunate to try and do this now, who knows what else is going on that might get stalled big time.
			CommitDeferredOctreeUpdates();
			Assert(m_uNumDeferredUpdateLevelIndices == 0);
			numDeferredUpdateLevelIndex = 0;
		}
		m_pauDeferredUpdateLevelIndices[numDeferredUpdateLevelIndex] = nLevelIndex;
		m_uNumDeferredUpdateLevelIndices = (u16)(numDeferredUpdateLevelIndex + 1);
	}

	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_DeferredOctreeUpdateCriticalSectionToken.Unlock();
	}
}
#endif	// !__SPU

template <class __NodeType> void phLevelNodeTree<__NodeType>::ClearDeferredOctreeUpdateList()
{
#if !__SPU
	const bool bOnlyMainThreadIsUpdatingPhysics = this->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_DeferredOctreeUpdateCriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics level now!");
	}
#endif	// !__SPU

#if __DEV && !__SPU
	s_MaxDeferredUpdateAdditions = Max(s_MaxDeferredUpdateAdditions, s_CurDeferredUpdateAdditions);
	s_MaxUniqueDeferredUpdateAdditions = Max(s_MaxUniqueDeferredUpdateAdditions, s_CurUniqueDeferredUpdateAdditions);

	s_CurDeferredUpdateAdditions = 0;
	s_CurUniqueDeferredUpdateAdditions = 0;
#endif	// __DEV && !__SPU

	m_uNumDeferredUpdateLevelIndices = 0;
#if !__SPU
	m_DeferredOctreeUpdateLevelIndices.Reset();
#else	// !__SPU
	// On SPU we set all of the bits in the bit set to zero.  There is probably a faster way to do this but this should be fine.
	const int numU32sToZero = m_DeferredOctreeUpdateLevelIndices.GetSizeInWords();
	u32 *bitSetPtr = m_DeferredOctreeUpdateLevelIndices.GetBitsPtr();
	for(int bitSetIndex = 0; bitSetIndex < numU32sToZero; ++bitSetIndex)
	{
		sysDmaPutUInt32(0, (uint64_t)&bitSetPtr[bitSetIndex], DMA_TAG(1));
	}
#endif	// !__SPU

#if !__SPU
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_DeferredOctreeUpdateCriticalSectionToken.Unlock();
	}
#endif
}
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

#if !__SPU
template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateObjectLocation(int nLevelIndex, const Matrix34* lastInstMatrix)
{
#if __PS3
	PDR_ONLY(debugPlayback::RecordUpdateLocation(nLevelIndex, &RCC_MAT34V(*lastInstMatrix)));
#endif

	// This is saying that it is false that the radius is good, but that is only partially true.  It is good as far as the octree is concerned, but bad as far as the object is concerned.
	DO_CONSISTENCY_CHECK((u16)(nLevelIndex), false);

	Assert(!IsNonexistent(nLevelIndex));

	UpdateObjectPositionInBroadphase(nLevelIndex, lastInstMatrix);

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	if(!m_DeferredOctreeUpdateEnabled)
	{
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
		if (g_MaintainLooseOctree)
		{
#if ENABLE_PHYSICS_LOCK
			g_GlobalPhysicsLock.WaitAsWriter();
#endif	// ENABLE_PHYSICS_LOCK

			phObjectData &objectData = m_paObjectData[nLevelIndex];
			const phInst *inst = objectData.GetInstance();
			const phArchetype *archetype = inst->GetArchetype();
			const phBound *bound = archetype->GetBound();
			const Vec3V newCenter = bound->GetWorldCentroid(inst->GetMatrix());
			const float newRadius = bound->GetRadiusAroundCentroid();
			UpdateObjectPositionInOctree(objectData, nLevelIndex, newCenter, newRadius);

#if ENABLE_PHYSICS_LOCK
			g_GlobalPhysicsLock.ReleaseAsWriter();
#endif	// ENABLE_PHYSICS_LOCK
		}
#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	}
	else
	{
		AddLevelIndexToDeferredOctreeUpdateList(nLevelIndex);
	}
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateObjectLocationAndRadius(int knLevelIndex, const Matrix34* lastInstMatrix)
{
#if __PS3
	PDR_ONLY(debugPlayback::RecordUpdateLocation(knLevelIndex, &RCC_MAT34V(*lastInstMatrix)));
#endif

	Assert(!IsNonexistent(knLevelIndex));
	UpdateObjectPositionInBroadphase(knLevelIndex, lastInstMatrix);

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	if(!m_DeferredOctreeUpdateEnabled)
	{
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
		if (g_MaintainLooseOctree)
		{
#if ENABLE_PHYSICS_LOCK
			g_GlobalPhysicsLock.WaitAsWriter();
#endif	// ENABLE_PHYSICS_LOCK

			phObjectData &objectData = m_paObjectData[knLevelIndex];
			const phInst *inst = objectData.GetInstance();
			const phArchetype *archetype = inst->GetArchetype();
			const phBound *bound = archetype->GetBound();
			const Vec3V newCenter = bound->GetWorldCentroid(inst->GetMatrix());
			const float newRadius = bound->GetRadiusAroundCentroid();

			UpdateObjectRadiusInOctree(objectData, knLevelIndex, newCenter, newRadius);
			UpdateObjectPositionInOctree(objectData, knLevelIndex, newCenter, newRadius);

#if ENABLE_PHYSICS_LOCK
			g_GlobalPhysicsLock.ReleaseAsWriter();
#endif	// ENABLE_PHYSICS_LOCK
		}
#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	}
	else
	{
		AddLevelIndexToDeferredOctreeUpdateList(knLevelIndex);
	}
#endif
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateObjectLocationAndRadius(int levelIndex, const Mat34V_Ptr lastInstMatrix)
{
	UpdateObjectLocationAndRadius(levelIndex, (const Matrix34*)lastInstMatrix);
}
#endif	// !__SPU

template <class __NodeType> void phLevelNodeTree<__NodeType>::RebuildCompositeBvh(int levelIndex)
{
	if(Verifyf(levelIndex != phInst::INVALID_INDEX, "Invalid level index passed to phLevelNodeTree::RebuildCompositeBvh, if the bound isn't in the level use phBoundComposite::UpdateBvh(true)."))
	{	
#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
		if(m_DeferredCompositeBvhUpdateEnabled)
		{
			sysCriticalSection criticalSection(m_DeferredBvhUpdateCriticalSectionToken);
			if(Verifyf(m_DeferredBvhUpdateCompositeLevelIndices.GetCount() < m_DeferredBvhUpdateCompositeLevelIndices.GetCapacity(), "Need to allocate more deferred BVH rebuild composite handles, max is currently %i.", m_DeferredBvhUpdateCompositeLevelIndices.GetCapacity()))
			{
				// Force the rebuild bit to 1 since rebuilds take priority over just a normal update
				m_FullRebuildCompositeBvhLevelIndices.Set(levelIndex);
				if(!m_UpdateCompositeBvhLevelIndices.GetAndSet(levelIndex))
				{
					// If this level index doesn't have a deferred update lined up already, add it
					m_DeferredBvhUpdateCompositeLevelIndices.Push((u16)levelIndex);
				}
			}
		}
		else
		{
			UpdateCompositeBvhFromLevelIndex(levelIndex, true);
		}
#else // LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
		UpdateCompositeBvhFromLevelIndex(levelIndex, true);
#endif // LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	}
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateCompositeBvh(int levelIndex)
{
	if(Verifyf(levelIndex != phInst::INVALID_INDEX, "Invalid level index passed to phLevelNodeTree::UpdateCompositeBvh, if the bound isn't in the level use phBoundComposite::UpdateBvh(false)."))
	{
#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
		if(m_DeferredCompositeBvhUpdateEnabled)
		{
			// Ensure that there is room in the handle array
			sysCriticalSection criticalSection(m_DeferredBvhUpdateCriticalSectionToken);
			if(Verifyf(m_DeferredBvhUpdateCompositeLevelIndices.GetCount() < m_DeferredBvhUpdateCompositeLevelIndices.GetCapacity(), "Need to allocate more deferred BVH update composite handles, max is currently %i.", m_DeferredBvhUpdateCompositeLevelIndices.GetCapacity()))
			{
				if(!m_UpdateCompositeBvhLevelIndices.GetAndSet(levelIndex))
				{
					// If this level index doesn't have a deferred update lined up already, add it
					m_DeferredBvhUpdateCompositeLevelIndices.Push((u16)levelIndex);
				}
			}
		}
		else
		{
			UpdateCompositeBvhFromLevelIndex(levelIndex, false);
		}
#else // LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
		UpdateCompositeBvhFromLevelIndex(levelIndex, false);
#endif // LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	}
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::UpdateCompositeBvhFromLevelIndex(int levelIndex, bool fullRebuild)
{
	phBound *bound = GetInstance(levelIndex)->GetArchetype()->GetBound();
	if(Verifyf(bound->GetType() == phBound::COMPOSITE, "Trying to update the BVH of a non-composite bound."))
	{
		static_cast<phBoundComposite*>(bound)->UpdateBvh(fullRebuild);
	}
}

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
template <class __NodeType> void phLevelNodeTree<__NodeType>::ProcessDeferredCompositeBvhUpdates()
{
	sysCriticalSection criticalSection(m_DeferredBvhUpdateCriticalSectionToken);

	if(!m_DeferredBvhUpdateCompositeLevelIndices.empty())
	{
#if ENABLE_PHYSICS_LOCK && !__SPU
		g_GlobalPhysicsLock.WaitAsWriter();
#endif	// ENABLE_PHYSICS_LOCK && !__SPU

		for(int deferredHandleIndex = 0; deferredHandleIndex < m_DeferredBvhUpdateCompositeLevelIndices.GetCount(); ++deferredHandleIndex)
		{
			u16 levelIndex = m_DeferredBvhUpdateCompositeLevelIndices[deferredHandleIndex];

			// Check if the instance has been removed since the deferred BVH update was set up
			if(GetState(levelIndex) != OBJECTSTATE_NONEXISTENT)
			{
				phInst* inst = GetInstance(levelIndex);
				// Check if the bound is still a composite
				// It is possible this is an entirely new instance with a new bound. Rebuilding or updating the BVH shouldn't hurt.
				phBound *bound = inst->GetArchetype()->GetBound();
				if(bound->GetType() == phBound::COMPOSITE)
				{
					static_cast<phBoundComposite*>(bound)->UpdateBvh(m_FullRebuildCompositeBvhLevelIndices.IsSet(levelIndex));
				}
			}
		}

		// Clear out all of the containers used by the deferred update
		m_UpdateCompositeBvhLevelIndices.Reset();
		m_FullRebuildCompositeBvhLevelIndices.Reset();
		m_DeferredBvhUpdateCompositeLevelIndices.clear();

#if ENABLE_PHYSICS_LOCK && !__SPU
		g_GlobalPhysicsLock.ReleaseAsWriter();
#endif	// ENABLE_PHYSICS_LOCK && !__SPU
	}
}
#endif // LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE

#if !__SPU
template <class __NodeType> void phLevelNodeTree<__NodeType>::CompositeBoundSetBoundThreadSafe(phBoundComposite& compositeBound, int componentIndex, phBound* pBound)
{
	bool hasWriteLock = false;

	if(compositeBound.GetBound(componentIndex) && compositeBound.GetBound(componentIndex)->GetRefCount() == 1)
	{
		// We're about to delete a bound so grab the write lock
		g_GlobalPhysicsLock.WaitAsWriter();
		hasWriteLock = true;
	}

	compositeBound.SetBound(componentIndex,pBound);

	if(hasWriteLock)
	{
		// Release the write lock if we grabbed it
		g_GlobalPhysicsLock.ReleaseAsWriter();
	}
}
template <class __NodeType> void phLevelNodeTree<__NodeType>::CompositeBoundSetBoundsThreadSafe(phBoundComposite& compositeBound, const phBoundComposite& compositeBoundToCopy)
{
	bool hasWriteLock = false;

	int numBoundsToSet = Min(compositeBound.GetNumBounds(),compositeBoundToCopy.GetNumBounds());
	for(int componentIndex = 0; componentIndex < numBoundsToSet; ++componentIndex)
	{
		if(!hasWriteLock && compositeBound.GetBound(componentIndex) && compositeBound.GetBound(componentIndex)->GetRefCount() == 1)
		{
			// We're about to delete a bound so grab the write lock
			g_GlobalPhysicsLock.WaitAsWriter();
			hasWriteLock = true;
		}
		compositeBound.SetBound(componentIndex, compositeBoundToCopy.GetBound(componentIndex));
	}

	if(hasWriteLock)
	{
		// Release the write lock if we grabbed it
		g_GlobalPhysicsLock.ReleaseAsWriter();
	}
}
template <class __NodeType> void phLevelNodeTree<__NodeType>::CompositeBoundSetActiveBoundsThreadSafe(phBoundComposite& compositeBound, const phBoundComposite& compositeBoundToCopy)
{
	bool hasWriteLock = false;

	int numBoundsToSet = Min(compositeBound.GetNumBounds(),compositeBoundToCopy.GetNumBounds());
	for(int componentIndex = 0; componentIndex < numBoundsToSet; ++componentIndex)
	{
		if(compositeBound.GetBound(componentIndex))
		{
			if(!hasWriteLock && compositeBound.GetBound(componentIndex)->GetRefCount() == 1)
			{
				// We're about to delete a bound so grab the write lock
				g_GlobalPhysicsLock.WaitAsWriter();
				hasWriteLock = true;
			}
			compositeBound.SetBound(componentIndex, compositeBoundToCopy.GetBound(componentIndex));
		}
	}

	if(hasWriteLock)
	{
		// Release the write lock if we grabbed it
		g_GlobalPhysicsLock.ReleaseAsWriter();
	}
}
template <class __NodeType> void phLevelNodeTree<__NodeType>::CompositeBoundRemoveBoundThreadSafe (phBoundComposite& compositeBound, int componentIndex)
{
	CompositeBoundSetBoundThreadSafe(compositeBound,componentIndex,NULL);
}
template <class __NodeType> void phLevelNodeTree<__NodeType>::CompositeBoundRemoveBoundsThreadSafe (phBoundComposite& compositeBound)
{
	bool hasWriteLock = false;

	int numBounds = compositeBound.GetNumBounds();
	for(int componentIndex = 0; componentIndex < numBounds; ++componentIndex)
	{
		if(compositeBound.GetBound(componentIndex))
		{
			if(!hasWriteLock && compositeBound.GetBound(componentIndex)->GetRefCount() == 1)
			{
				// We're about to delete a bound so grab the write lock
				g_GlobalPhysicsLock.WaitAsWriter();
				hasWriteLock = true;
			}
			compositeBound.RemoveBound(componentIndex);
		}
	}

	if(hasWriteLock)
	{
		// Release the write lock if we grabbed it
		g_GlobalPhysicsLock.ReleaseAsWriter();
	}
}
#endif // !__SPU

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
template <class __NodeType> void phLevelNodeTree<__NodeType>::CommitDeferredOctreeUpdates()
{
	PF_FUNC(CommitDeferredOctreeUpdates);
#if !__SPU
	// sysCriticalSection doesn't seem to have any implementation for SPU.  I'll have to look into that later.
	const bool bOnlyMainThreadIsUpdatingPhysics = this->GetOnlyMainThreadIsUpdatingPhysics();
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_DeferredOctreeUpdateCriticalSectionToken.Lock();
	}
	else
	{
		Assertf(sysIpcGetCurrentThreadId() == m_threadId, "Only the main update thread should be updating the physics level now!");
	}
#endif	// !__SPU

#if ENABLE_PHYSICS_LOCK && !__SPU
	// On SPU you have to wait for the write lock outside of this function.  I don't like that inconsistency but it's simpler that way.
	g_GlobalPhysicsLock.WaitAsWriter();
#endif	// ENABLE_PHYSICS_LOCK && !__SPU
	SPU_ONLY(phSpuLevelContext levelContext);
	SPU_ONLY(phSpuObjectBuffer<phObjectData> objectDataBuffer);

	for(int deferredIndex = m_uNumDeferredUpdateLevelIndices - 1; deferredIndex >= 0; --deferredIndex)
	{
		const int deferredLevelIndex = m_pauDeferredUpdateLevelIndices[deferredIndex];
		// NOTE: If generation IDs are enabled we could use them to ensure that the level index is actually still being used by the same instance it was
		//   when we added it to the list.  I'm not doing that because a) it doesn't hurt anything if we update an instance that we didn't have to (shouldn't
		//   cost much), b) it's pretty unlikely to happen anyway, c) I'd prefer not to add a branch to handle the case given a) and b) above and d) I'd now
		//   have to store 32 bits per instance in the list rather than the 16 that I am now.
		phObjectData &objectData = NON_SPU_ONLY(m_paObjectData[deferredLevelIndex]) SPU_ONLY(*objectDataBuffer.FetchObject(&m_paObjectData[deferredLevelIndex]));
		if(objectData.GetState() != OBJECTSTATE_NONEXISTENT)
		{
			FastAssert(objectData.GetState() < OBJECTSTATE_NONEXISTENT);
#if !__SPU
			const phInst *inst = objectData.GetInstance();
			const phArchetype *archetype = inst->GetArchetype();
			const phBound *bound = archetype->GetBound();
#else	// !__SPU
			levelContext.FetchInstanceArchetypeAndBound(objectData);
			const phInst *inst = levelContext.GetSpuInstance();
			const phBound *bound = levelContext.GetSpuBound();
#endif	// !__SPU
			const Vec3V newCenter = bound->GetWorldCentroid(inst->GetMatrix());
			const float newRadius = bound->GetRadiusAroundCentroid();

			UpdateObjectRadiusInOctree(objectData, deferredLevelIndex, newCenter, newRadius);
			UpdateObjectPositionInOctree(objectData, deferredLevelIndex, newCenter, newRadius);
			SPU_ONLY(objectDataBuffer.ReturnObject());
		}
	}

#if ENABLE_PHYSICS_LOCK && !__SPU
	g_GlobalPhysicsLock.ReleaseAsWriter();
#endif	// ENABLE_PHYSICS_LOCK && !__SPU

	ClearDeferredOctreeUpdateList();

#if !__SPU
	if(!bOnlyMainThreadIsUpdatingPhysics)
	{
		m_DeferredOctreeUpdateCriticalSectionToken.Unlock();
	}
#endif
}
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

#if !__SPU
template <class __NodeType> void phLevelNodeTree<__NodeType>::SetNotifyOutOfWorldCallback(InstOutOfWorldCallback callback)	
{ 
	m_InstOutOfWorldCallback = callback; 
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::DefaultNotifyOutOfWorld(phInst* inst)
{
	inst->NotifyOutOfWorld();
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::SetMaxInstLastMatrices(int maxInstLastMatrices) 
{
	Assert(maxInstLastMatrices <= MAX_INST_LAST_MATRICES);
	m_MaxInstLastMatrices = maxInstLastMatrices; 
}

template <class __NodeType> bool phLevelNodeTree<__NodeType>::CanBecomeActive (int levelIndex) const
{
	return !(IsFixed(levelIndex) || (GetInstance(levelIndex)->GetInstFlag(phInst::FLAG_NEVER_ACTIVATE) != 0));
}

template <class __NodeType> phLevelBase::eObjectState phLevelNodeTree<__NodeType>::GetStateSafe (const phInst* instance) const
{
	if (instance)
	{
		int levelIndex = instance->GetLevelIndex();
		if (IsInLevel(levelIndex))
		{
			return GetState(levelIndex);
		}
	}

	return OBJECTSTATE_NONEXISTENT;
}

template <class __NodeType> bool phLevelNodeTree<__NodeType>::ActivateObject(const int knLevelIndex, const void* kuUserData, bool UNUSED_PARAM(bPermanentlyActive)/* = false*/)
{
#if ENABLE_PHYSICS_LOCK
	PHLOCK_SCOPEDWRITELOCK;
#endif	// ENABLE_PHYSICS_LOCK
	//DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);

	Assert(m_NumActive < m_MaxActive);
	Assert(LegitLevelIndex(knLevelIndex));
	Assert(IsInactive(knLevelIndex));
	Assert(m_NumInactive > 0);

#if __BANK
	sm_MaxActiveObjectsEver = Max(sm_MaxActiveObjectsEver, (int)m_NumActive);
#endif

	SetState(knLevelIndex, OBJECTSTATE_ACTIVE);
//	Assert(m_nActiveIteratorIndex == -1);	// It's probably okay to increase the number of active objects during an iteration.
	AddActiveObjectUserData(knLevelIndex, kuUserData);

	--m_NumInactive;
	++m_NumActive;

	if(m_BroadPhase->IsIncremental())
	{
		FindAndAddOverlappingPairs(knLevelIndex);
	}

	return true;
}


template <class __NodeType> bool phLevelNodeTree<__NodeType>::DeactivateObject(const int knLevelIndex)
{
#if ENABLE_PHYSICS_LOCK
	PHLOCK_SCOPEDWRITELOCK;
#endif	// ENABLE_PHYSICS_LOCK
	//DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);

	Assert(LegitLevelIndex(knLevelIndex));
	Assert(IsActive(knLevelIndex));
	Assert(m_NumActive > 0);

	SetState(knLevelIndex, OBJECTSTATE_INACTIVE);
	Assert(m_nActiveIteratorIndex == -1);
	RemoveActiveObjectUserData(knLevelIndex);

	--m_NumActive;
	++m_NumInactive;
	//DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);

	return true;
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::FindAndAddOverlappingPairs (int levelIndex)
{
	// Find all the objects in the world that overlap with this object, and add them to the overlapping pair array

	phInst* inst = GetInstance(levelIndex);
	Vec3V boxHalfWidth;	
	Vec3V boxCenter;
	inst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(boxHalfWidth, boxCenter);
	Mat34V_ConstRef mtx = inst->GetMatrix();
	boxHalfWidth = geomBoxes::ComputeAABBExtentsFromOBB(mtx.GetMat33ConstRef(), boxHalfWidth);
	boxCenter = Transform(mtx, boxCenter);

	// Add in the broadphase inverse quantum, so we don't miss things that we only overlap in the SAP due to quantization
	// We actually need to add in two quantum units to account for the fact that both AABBs are getting quantized (and min's round down, max's round up).
	// And we need a third because of the quirky way that the 3-axis SAP keeps all min's at even quantum units and max's at odd quantum units.
	const Vec3V quantumUnit = VECTOR3_TO_VEC3V(m_BroadPhase->GetInvQuantize());
	const Vec3V thriceQuantumUnit = quantumUnit + quantumUnit + quantumUnit;
	boxHalfWidth = Add(boxHalfWidth, thriceQuantumUnit);

#if ENABLE_PHYSICS_LOCK
	// A read lock should be okay here because we don't modify the level while the iterator is in existence.
	phIterator it(phIterator::PHITERATORLOCKTYPE_READLOCK);
#else
	phIterator it;
#endif
	it.InitCull_AABB(boxCenter,boxHalfWidth);
	it.SetStateIncludeFlags(phLevelBase::STATE_FLAG_INACTIVE | phLevelBase::STATE_FLAG_FIXED);
	it.SetSkipTypeIncludeFlagsTests(true);
	it.SetCullAgainstInstanceAABBs(true);

	u16 uCurObjectLevelIndex = GetFirstCulledObject(it);
	while(uCurObjectLevelIndex != phInst::INVALID_INDEX)
	{
		if (uCurObjectLevelIndex != levelIndex)
		{
			// This culled object is not the same as this object, so add a new overlapping pair. Articulated objects are added as self-pairs in the simulator.
			m_BroadPhase->addOverlappingPair(levelIndex, uCurObjectLevelIndex);
		}

		uCurObjectLevelIndex = GetNextCulledObject(it);
	}

	//DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);

}


template <class __NodeType> int phLevelNodeTree<__NodeType>::GetFirstActiveIndex()
{
	Assert(m_nActiveIteratorIndex == -1);
	m_nActiveIteratorIndex = 0;
	return GetNextActiveIndex();
}


template <class __NodeType> int phLevelNodeTree<__NodeType>::GetNextActiveIndex()
{
	Assert(m_nActiveIteratorIndex <= m_NumActive);
	AssertMsg(m_nActiveIteratorIndex != -1, "phLevelNodeTree::GetNextActiveIndex() : Trying to get next active index with no active iteration in progress.");
	if(m_nActiveIteratorIndex >= m_NumActive)
	{
		// -1 signifies that the iteration has ended.
		m_nActiveIteratorIndex = -1;
		return phInst::INVALID_INDEX;
	}

	int nNextLevelIndex = GetActiveLevelIndex(m_nActiveIteratorIndex);
	++m_nActiveIteratorIndex;
	return nNextLevelIndex;
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::ReserveInstLastMatrix(phInst* inst)
{
	Assert(m_FreeInstLastMatrices.GetCount() > 0);
	Assert(!inst->HasLastMatrix());

	inst->SetInstFlag(phInst::FLAG_INTERNAL_USE_ONLY_LAST_MTX, true);

	if(IsInLevel(inst->GetLevelIndex()))
	{
		ReserveInstLastMatrixInternal(inst);
	}
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::ReserveInstLastMatrixInternal(phInst* inst)
{
	Assert(m_FreeInstLastMatrices.GetCount() > 0);

	if(m_FreeInstLastMatrices.GetCount() > 0)
	{
		int levelIndex = inst->GetLevelIndex();

		if(m_pInstLastMatrixIndices[levelIndex] == INVALID_INST_LAST_MATRIX_INDEX)
		{
			int mtxIdx = m_FreeInstLastMatrices.Pop();
			m_pInstLastMatrixIndices[levelIndex] = mtxIdx;
			SetLastInstanceMatrix(inst, inst->GetMatrix());
		}
	}
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::ReleaseInstLastMatrix(phInst* inst)
{
	Assert(inst != NULL);
	Assert(inst->HasLastMatrix());

	if(inst->HasLastMatrix() && IsInLevel(inst->GetLevelIndex()))
	{
		ReleaseInstLastMatrixInternal(inst);
	}	

	inst->SetInstFlag(phInst::FLAG_INTERNAL_USE_ONLY_LAST_MTX, false);
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::ReleaseInstLastMatrixInternal(phInst* inst)
{
	if(m_MaxInstLastMatrices > 0)
	{
		int levelIndex = inst->GetLevelIndex();
		int mtxIdx     = m_pInstLastMatrixIndices[levelIndex];
		Assert(mtxIdx >= 0 && mtxIdx < m_MaxInstLastMatrices);
		Assert(mtxIdx != INVALID_INST_LAST_MATRIX_INDEX);
		// If mtxIdx is not valid we must have run out of last matrices when it was added to the level.
		if(mtxIdx != INVALID_INST_LAST_MATRIX_INDEX)
		{
			m_FreeInstLastMatrices.Push(mtxIdx);
			m_pInstLastMatrixIndices[levelIndex] = INVALID_INST_LAST_MATRIX_INDEX;
		}
	}
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::SetLastInstanceMatrix(const phInst* inst, Mat34V_In lastMtx)
{
	Assert(inst != NULL);
	Assert(inst->HasLastMatrix());
	Assert(m_MaxInstLastMatrices > 0);
	Assert(IsInLevel(inst->GetLevelIndex()));
	Assert(m_pInstLastMatrixIndices[inst->GetLevelIndex()] != INVALID_INST_LAST_MATRIX_INDEX);

	u16 levelIndex = inst->GetLevelIndex();

	if(IsInLevel(levelIndex) && m_pInstLastMatrixIndices[levelIndex] != INVALID_INST_LAST_MATRIX_INDEX)
	{
		m_pInstLastMatrices[m_pInstLastMatrixIndices[levelIndex]] = lastMtx;
	}
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::PostCollideActives()
{
	// Thaw the active state - convert pending active objects to active objects and update active object locations.
	m_FrozenActiveState = false;
	DO_CONSISTENCY_CHECK(phInst::INVALID_INDEX, true);
}


#if !__SPU
#if __PFDRAW
extern float g_AnimateFromLastPhase;
extern float g_AttentionSphereRadius;
#endif

template <class __NodeType> void phLevelNodeTree<__NodeType>::ProfileDraw(ColorChoiceFunc PF_DRAW_ONLY(chooseColor)) const
{
#if ENABLE_PHYSICS_LOCK
	PHLOCK_SCOPEDREADLOCK;
#endif	// ENABLE_PHYSICS_LOCK

#if __PFDRAW
	if (PFD_AnimateFromLast.WillDraw())
	{
		if (g_AnimateFromLastPhase == 1.0f)
		{
			g_AnimateFromLastPhase = 0.0f;
		}
		else
		{
			float phaseDelta = TIME.GetSeconds() / Max(PFD_AnimateTime.GetValue(), 0.1f);
			g_AnimateFromLastPhase += phaseDelta;
			if (g_AnimateFromLastPhase >= 1.0f)
			{
				g_AnimateFromLastPhase = 1.0f;
			}
		}
	}

	// HACK ALERT! I'm using this rather silly normal, because at the moment, it allows
	// wireframe rendering to draw full bright on the PC. In the future, lighting mode will
	// probably need to be cached in the grc buffer.
	Vector3 defaultNormal(1e30f, 1e30f, 1e30f); 
	grcNormal3f(defaultNormal);
	grcColor(Color_white);

	g_AttentionSphereRadius -= TIME.GetSeconds();

	float attentionSphereSlider = PFD_AttentionSphere.GetValue();
	if (attentionSphereSlider > 0.01f)
	{
		while (g_AttentionSphereRadius < 0.0f )
		{
			g_AttentionSphereRadius += attentionSphereSlider;
		}
	}
	else
	{
		g_AttentionSphereRadius = 0.0f;
	}

#define GET_TYPE_FLAG_IF_WIDGET_ENABLED(bit) (PFD_TypeFlag##bit.WillDraw() ? 1 << bit : 0)
#define GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(bit) (PFD_IncludeFlag##bit.WillDraw() ? 1 << bit : 0)

	unsigned typeFlags = 0xffffffff;
	unsigned includeFlags = 0xffffffff;

	if( PFDGROUP_TypeFlagFilter.WillDraw() )
	{
		typeFlags = 
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(0) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(1) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(2) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(3) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(4) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(5) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(6) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(7) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(8) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(9) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(10) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(11) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(12) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(13) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(14) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(15) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(16) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(17) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(18) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(19) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(20) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(21) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(22) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(23) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(24) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(25) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(26) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(27) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(28) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(29) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(30) |
			GET_TYPE_FLAG_IF_WIDGET_ENABLED(31);
	}
	if( PFDGROUP_IncludeFlagFilter.WillDraw() )
	{
		includeFlags =
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(0) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(1) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(2) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(3) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(4) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(5) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(6) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(7) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(8) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(9) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(10) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(11) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(12) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(13) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(14) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(15) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(16) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(17) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(18) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(19) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(20) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(21) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(22) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(23) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(24) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(25) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(26) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(27) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(28) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(29) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(30) |
			GET_INCLUDE_FLAG_IF_WIDGET_ENABLED(31);
	}

	if (PFDGROUP_Bounds.Begin(false))
	{
		Vector3 InstCenter;
		grcViewport *CurViewport = grcViewport::GetCurrent();

		for(int nObjectLevelIndex = 0; nObjectLevelIndex < m_MaxObjects; ++nObjectLevelIndex)
		{
			if(IsNonexistent(nObjectLevelIndex))
			{
				continue;
			}

			const phInst *inst = GetInstance(nObjectLevelIndex);
			Assert(inst->GetArchetype() != NULL);
			Assert(inst->GetArchetype()->GetBound() != NULL);

			// Check the type flags to see if this type of bound is one of the bounds that should be drawn at all.
			// If all type flags are on, we draw all objects (even those that don't have any type flags set).
			if( PFDGROUP_TypeFlagFilter.GetEnabled() && typeFlags != 0xffffffff)
			{
				if(!(GetInstanceTypeFlags(nObjectLevelIndex) & typeFlags))
				{
					continue;
				}
			}

			// Check the include flags to see if this type of bound is one of the bounds that should be drawn at all.
			// If all include are on, we draw all objects (even those that don't have any include flags set).
			if( PFDGROUP_IncludeFlagFilter.GetEnabled() && includeFlags != 0xffffffff)
			{
				if(!(GetInstanceIncludeFlags(nObjectLevelIndex) & includeFlags))
				{
					continue;
				}
			}

			// Let's check the bounding sphere of the instance versus the viewport and don't bother trying to draw it if it won't be visible anyway.
			InstCenter = VEC3V_TO_VECTOR3(inst->GetArchetype()->GetBound()->GetWorldCentroid(inst->GetMatrix()));
			if(CurViewport && CurViewport->IsSphereVisible(InstCenter.x, InstCenter.y, InstCenter.z, inst->GetArchetype()->GetBound()->GetRadiusAroundCentroid()) == cullOutside)
			{
				continue;
			}

			if(CurViewport && VEC3V_TO_VECTOR3(CurViewport->GetCameraPosition()).Dist2(InstCenter) >
			   square(PFD_BoundDrawDistance.GetValue() + inst->GetArchetype()->GetBound()->GetRadiusAroundCentroid()))
			{
				continue;
			}

			eObjectState state = GetState(nObjectLevelIndex);
			if((state == OBJECTSTATE_ACTIVE && PFD_Active.WillDraw()) ||
			   (state == OBJECTSTATE_INACTIVE && PFD_Inactive.WillDraw()) ||
			   (state == OBJECTSTATE_FIXED && PFD_Fixed.WillDraw()))
			{
				chooseColor(inst);
				DrawBound(inst,typeFlags,includeFlags);

				bool oldLighting = grcLighting(false);

				if (PFD_Face.WillDraw())
				{
					DrawNormals(inst, phBound::FACE_NORMALS, PFD_NormalLength.GetValue(), typeFlags, includeFlags);
				}
				if (PFD_Edge.WillDraw())
				{
					DrawNormals(inst, phBound::EDGE_NORMALS, PFD_NormalLength.GetValue(), typeFlags, includeFlags);
				}

                bool drawIndices = PFD_LevelIndices.WillDraw();
                bool drawNames = PFD_BoundNames.WillDraw();
				bool drawTypes = PFD_BoundTypes.WillDraw();
                if (drawIndices || drawNames || drawTypes)
				{
                    const int OBJECT_TEXT_SIZE = 1024;
					char objectText[OBJECT_TEXT_SIZE];
					objectText[OBJECT_TEXT_SIZE - 1] = '\0';
                    if (drawIndices && drawNames)
                    {
                        formatf(objectText,OBJECT_TEXT_SIZE,"%i\n%s",inst->GetLevelIndex(),inst->GetArchetype()->GetFilename());
                    }
                    else if (drawIndices)
                    {
					    formatf(objectText,OBJECT_TEXT_SIZE,"%i",inst->GetLevelIndex());
                    }
                    else if (drawNames)
                    {
                        formatf(objectText,OBJECT_TEXT_SIZE,"%s",inst->GetArchetype()->GetFilename());
                    }

					if(drawTypes)
					{
						phBound const * bound = inst->GetArchetype()->GetBound();

						if(bound)
						{
							if(bound->GetType() == phBound::COMPOSITE)
							{
								char subBoundText[OBJECT_TEXT_SIZE];
									 subBoundText[OBJECT_TEXT_SIZE - 1] = '\0';

								phBoundComposite const * boundComposite = static_cast<phBoundComposite const*>(bound);
								for(int i=0; i< boundComposite->GetNumBounds(); i++)
								{
									phBound const* subBound = boundComposite->GetBound(i);

									if(subBound)
									{
										formatf(subBoundText, OBJECT_TEXT_SIZE, "%d:%s", i, subBound->GetTypeString());

										Mat34V componentSpaceToWorldSpace;
										Transform(componentSpaceToWorldSpace, inst->GetMatrix(), boundComposite->GetCurrentMatrix(i));

										Vector3 center = VEC3V_TO_VECTOR3(subBound->GetCenterOfMass(componentSpaceToWorldSpace));
										grcColor(Color_white);
										grcDrawLabelf(center, subBoundText);
									}
								}
							}
							else
							{
								safecatf(objectText,OBJECT_TEXT_SIZE,drawIndices||drawNames ? "\n%s" : "%s", bound->GetTypeString());
							}
						}
					}

					Vector3 center = VEC3V_TO_VECTOR3(inst->GetArchetype()->GetBound()->GetCenterOfMass(inst->GetMatrix()));
					grcColor(Color_white);
					grcDrawLabelf(center, objectText);
				}

				if (PFD_InstMatrices.WillDraw())
				{
					Vector3 boxSize = VEC3V_TO_VECTOR3(inst->GetArchetype()->GetBound()->GetBoundingBoxSize());
					float size = Max(boxSize.x, boxSize.y, boxSize.z) * 0.1f;
					size = Min(size, 1.0f);
					grcDrawAxis(size, RCC_MATRIX34(inst->GetMatrix()));
				}

				grcLighting(oldLighting);
			}
		}

		PFDGROUP_Bounds.End();
	}
#endif // __PFDRAW

#if __NMDRAW
	bool thingsWorthDrawing = false;
	for(int nObjectLevelIndex = 0; nObjectLevelIndex < m_MaxObjects; ++nObjectLevelIndex)
	{
		if(IsNonexistent(nObjectLevelIndex))
		{
			continue;
		}

		const phInst *inst = GetInstance(nObjectLevelIndex);
		Assert(inst->GetArchetype() != NULL);
		Assert(inst->GetArchetype()->GetBound() != NULL);

		DrawBound(inst,typeFlags,includeFlags);
		thingsWorthDrawing = true;
	}

	// draw level spatial extents
	if (thingsWorthDrawing)
	{
		Matrix34 octMtx;
		octMtx.Identity();
		for (int i=0; i<m_uOctreeNodesInUse; i++)
		{
			__NodeType *node = &m_paOctreeNodes[i];
			octMtx.d = node->m_Center;    
			NMRenderBuffer::getInstance()->addBox(NMDRAW_SPATIAL, octMtx, Vector3(node->m_HalfWidth, node->m_HalfWidth, node->m_HalfWidth), Vector3(0, 1, 1));
		}
	}
#endif
}

#if __NMDRAW
template <class __NodeType> void phLevelNodeTree<__NodeType>::DrawBound(const phInst* inst, u32 typeFlagFilter, u32 includeFlagFilter) const
#else
template <class __NodeType> void phLevelNodeTree<__NodeType>::DrawBound(const phInst* PF_DRAW_ONLY(inst), u32 PF_DRAW_ONLY(typeFlagFilter), u32 PF_DRAW_ONLY(includeFlagFilter)) const
#endif // __NMDRAW
{
#if __PFDRAW
	phBound* bound = inst->GetArchetype()->GetBound();
	Assert(bound);

	bool oldLighting = false;
	bool supportDrawing = PFD_SupportPoints.WillDraw() && (bound->IsConvex() || bound->GetType() == phBound::COMPOSITE);

	static const u32 BLINK_SPEED = 10;

	if (PFD_InactiveCollidesVsInactive.WillDraw() &&
		PFD_InactiveCollidesVsFixed.WillDraw() &&
		GetInactiveCollidesAgainstInactive(inst->GetLevelIndex()) &&
		GetInactiveCollidesAgainstFixed(inst->GetLevelIndex()))
	{
		u32 blink = (TIME.GetFrameCount() / BLINK_SPEED) % 3;
		if (blink == 1)
		{
			grcColor(Color_red);
		}
		else if (blink == 2)
		{
			grcColor(Color_yellow);
		}
	}
	else if (PFD_InactiveCollidesVsInactive.WillDraw() &&
		GetInactiveCollidesAgainstInactive(inst->GetLevelIndex()))
	{
		u32 blink = (TIME.GetFrameCount() / BLINK_SPEED) % 2;
		if (blink == 1)
		{
			grcColor(Color_red);
		}
	}
	else if (PFD_InactiveCollidesVsFixed.WillDraw() &&
		GetInactiveCollidesAgainstFixed(inst->GetLevelIndex()))
	{
		u32 blink = (TIME.GetFrameCount() / BLINK_SPEED) % 2;
		if (blink == 1)
		{
			grcColor(Color_yellow);
		}
	}

	Mat34V drawMatrix = inst->GetMatrix();

	if (PFD_AnimateFromLast.WillDraw())
	{
		Mat34V lastMatrix = PHSIM->GetLastInstanceMatrix(inst);
		Mat34V interpolatedMatrix;

		Vec3V linVel = Subtract(inst->GetArchetype()->GetBound()->GetCenterOfMass(drawMatrix), inst->GetArchetype()->GetBound()->GetCenterOfMass(lastMatrix));
		interpolatedMatrix.Set3x3(drawMatrix);
		interpolatedMatrix.SetCol3( SubtractScaled(drawMatrix.GetCol3(), linVel, ScalarV(V_ONE) - ScalarV(g_AnimateFromLastPhase)) );

		drawMatrix = interpolatedMatrix;
	}

	int whichPolys = phBound::ALL_POLYS;

	if (PFD_ThinPolys.WillDraw())
	{
		whichPolys |= phBound::RENDER_THIN_POLYS;
	}
	
	if (PFD_BadNormalPolys.WillDraw())
	{
		whichPolys |= phBound::RENDER_BAD_NORMAL_POLYS;
	}
	
	if (PFD_BadNeighborPolys.WillDraw())
	{
		whichPolys |= phBound::RENDER_BAD_NEIGHBOR_POLYS;
	}

	if (PFD_BigPrimitives.WillDraw())
	{
		whichPolys |= phBound::RENDER_BIG_PRIMITIVES;
	}

	if (PFD_Solid.WillDraw())
	{
		oldLighting = grcLighting(PFD_SolidBoundLighting.GetEnabled());

		bound->Draw(drawMatrix, PFD_DrawBoundMaterials.WillDraw(), true, whichPolys, PFD_HighlightFlags.GetValue(), typeFlagFilter, includeFlagFilter, inst->GetArchetype()->GetTypeFlags(), inst->GetArchetype()->GetIncludeFlags());

		grcLighting(oldLighting);
	}

	if (PFD_Centroid.WillDraw())
	{
		bool oldLighting = grcLighting(false);
		bound->DrawCentroid(drawMatrix);
		grcLighting(oldLighting);
	}

	if (PFD_CenterOfGravity.WillDraw())
	{
		bool oldLighting = grcLighting(false);
		bound->DrawCenterOfGravity(drawMatrix);
		grcLighting(oldLighting);
	}

	if (PFD_AngularInertia.WillDraw())
	{
		bool oldLighting = grcLighting(false);
		Color32 oldColor(grcCurrentColor);
		Color32 angInertiaColor(160,10,240);

		Mat34V parentMatrix = inst->GetMatrix();
		Vec3V cgOffset = bound->GetCGOffset();
		cgOffset = Transform(parentMatrix, cgOffset);
		Mat34V cgOffMatrix = parentMatrix;
		cgOffMatrix.SetCol3(cgOffset);

		bound->DrawAngularInertia(cgOffMatrix, PFD_AngularInertiaScale.GetValue(), PFD_InvertAngularInertia.WillDraw(), angInertiaColor);
		if(bound->GetType() == phBound::COMPOSITE)
		{
			const phBoundComposite* boundComp = static_cast<const phBoundComposite*>(bound);
			for (int child = 0; child < boundComp->GetNumBounds(); ++child)
			{
				if(boundComp->GetBound(child))
				{
					Mat34V childMtx = boundComp->GetCurrentMatrix(child);
					Transform(childMtx, parentMatrix, childMtx);

					boundComp->GetBound(child)->DrawAngularInertia(childMtx, PFD_AngularInertiaScale.GetValue(), PFD_InvertAngularInertia.WillDraw(), angInertiaColor);
				}
			}
		}

		grcColor(oldColor);
		grcLighting(oldLighting);
	}

	if (PFD_Wireframe.WillDraw())
	{
		oldLighting = grcLighting(false);

		if (PFD_Solid.WillDraw())
		{
			grcColor(Color_black);
		}

		if (!supportDrawing)
		{
			bound->Draw(drawMatrix, PFD_DrawBoundMaterials.WillDraw(), false, whichPolys, PFD_HighlightFlags.GetValue(), typeFlagFilter, includeFlagFilter);
		}
		else
		{
			bound->DrawSupport(drawMatrix, typeFlagFilter, includeFlagFilter);
		}

		grcLighting(oldLighting);
	}

	if (PFD_CullSpheres.WillDraw())
	{
		bool oldLighting = grcLighting(false);

		Color32 currentColor(grcCurrentColor);
		grcColor(Color32(currentColor.GetRed() / 2, currentColor.GetGreen() / 2, currentColor.GetBlue() / 2, PFD_CullOpacity.GetValue()));
		Vector3 cullCenter;

		cullCenter = VEC3V_TO_VECTOR3(Transform(drawMatrix, bound->GetCentroidOffset()));

		grcDrawSphere(bound->GetRadiusAroundCentroid(), cullCenter, 32, true, PFD_CullSolid.GetEnabled());

		grcLighting(oldLighting);
	}

    if (PFD_CullBoxes.WillDraw())
    {
		bool oldLighting = grcLighting(false);

		Color32 currentColor(grcCurrentColor);
        Color32 color(currentColor.GetRed() / 2, currentColor.GetGreen() / 2, currentColor.GetBlue() / 2, PFD_CullOpacity.GetValue());
        Vector3 cullCenter;

		Vector3 boxCenter;
		boxCenter.Average(VEC3V_TO_VECTOR3(bound->GetBoundingBoxMin()), VEC3V_TO_VECTOR3(bound->GetBoundingBoxMax()));
		(*(const Matrix34*)(&drawMatrix)).Transform(boxCenter);
        Matrix34 boxMtx = (*(const Matrix34*)(&drawMatrix));
		boxMtx.d = boxCenter;

        Vector3 boxSize = VEC3V_TO_VECTOR3(bound->GetBoundingBoxSize());

		if(PFD_CullSolid.GetEnabled())
		{
			grcDrawSolidBox(boxSize, boxMtx, color);
		}
		else
		{
			grcDrawBox(boxSize, boxMtx, color);
		}

		const phBound* bound = inst->GetArchetype()->GetBound();
		if (bound->GetType() == phBound::COMPOSITE)
		{
			const phBoundComposite* composite = static_cast<const phBoundComposite*>(bound);

			for (int part = 0; part < composite->GetNumBounds(); ++ part)
			{
				if (composite->GetBound(part))
				{
					Vector3 partMins = RCC_VECTOR3(composite->GetLocalBoxMins(part));
					Vector3 partMaxs = RCC_VECTOR3(composite->GetLocalBoxMaxs(part));
					Vector3 partCenter;
					partCenter.Average(partMins, partMaxs);
					Vector3 partSize;
					partSize.Subtract(partMaxs, partMins);

					Matrix34 partMtx;
					partMtx.Dot(RCC_MATRIX34(composite->GetCurrentMatrix(part)), (*(const Matrix34*)(&drawMatrix)));

					partMtx.Transform(partCenter);
					partMtx.d = partCenter;

					grcDrawBox(partSize, partMtx, color);
				}
			}
		}

		grcLighting(oldLighting);
    }
#endif // __PFDRAW

#if __NMDRAW
	phBound* bound = inst->GetArchetype()->GetBound();
	Assert(bound);

	bound->NMRender((*(const Matrix34*)(&drawMatrix)));
#endif // __NMDRAW
}

#if __PFDRAW
template <class __NodeType> void phLevelNodeTree<__NodeType>::DrawNormals(const phInst* inst, int whichNormals, float length, unsigned int typeFilter, unsigned int includeFilter) const
{
	int whichPolys = phBound::ALL_POLYS;

	if (PFD_ThinPolys.WillDraw())
	{
		whichPolys |= phBound::RENDER_THIN_POLYS;
	}

	if (PFD_BadNormalPolys.WillDraw())
	{
		whichPolys |= phBound::RENDER_BAD_NORMAL_POLYS;
	}

	if (PFD_BadNeighborPolys.WillDraw())
	{
		whichPolys |= phBound::RENDER_BAD_NEIGHBOR_POLYS;
	}

	inst->GetArchetype()->GetBound()->DrawNormals(inst->GetMatrix(), whichNormals, whichPolys, length, typeFilter, includeFilter);
}


#if __PFDRAW
template <class __NodeType> void phLevelNodeTree<__NodeType>::LevelColorChoice(const phInst* inst) const
{
	switch(GetState(inst->GetLevelIndex()))
	{
	case OBJECTSTATE_ACTIVE:
		{
			grcColor(Color_red);
			break;
		}
	case OBJECTSTATE_INACTIVE:
		{
			grcColor(Color_green);
			break;
		}
	default:
		{
			grcColor(Color_grey);
			break;
		}
	}
}
#endif
#endif // __PFDRAW
#endif
#endif	// !__SPU

template <class __NodeType> __forceinline u16 phLevelNodeTree<__NodeType>::GetOctreeNodeIndexFromPool()
{
	FastAssert(m_uOctreeNodesInUse < m_uTotalOctreeNodeCount);
	u16 uOctreeNodeIndex = NON_SPU_ONLY(m_pauOctreeNodeIndexList[m_uOctreeNodesInUse]) SPU_ONLY((sysDmaGetUInt16((uint64_t)&m_pauOctreeNodeIndexList[m_uOctreeNodesInUse], DMA_TAG(1))));
	++m_uOctreeNodesInUse;

#if __DEV
	if (m_uOctreeNodesInUse>=m_uTotalOctreeNodeCount)
	{
		Quitf("The physics level used up all its octree nodes.  This is fatal but it should be impossible for it to happen.");
	}
#if __BANK && !__SPU
	sm_MaxOctreeNodesEverUsed = Max((int)m_uOctreeNodesInUse, sm_MaxOctreeNodesEverUsed);
#endif
#endif

	return uOctreeNodeIndex;
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::ReturnOctreeNodeIndexToPool(u16 uOctreeNodeIndex)
{
	PHLEVELNEW_ASSERT(m_uOctreeNodesInUse > 0);

	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeCount == 0));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeIndices[0] == (u16)(-1)));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeIndices[1] == (u16)(-1)));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeIndices[2] == (u16)(-1)));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeIndices[3] == (u16)(-1)));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeIndices[4] == (u16)(-1)));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeIndices[5] == (u16)(-1)));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeIndices[6] == (u16)(-1)));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ChildNodeIndices[7] == (u16)(-1)));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ParentNodeIndex == (u16)(-1)));

	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ContainedObjectCount == 0));
	NON_SPU_ONLY(PHLEVELNEW_ASSERT(m_paOctreeNodes[uOctreeNodeIndex].m_ContainedObjectStartIdx == phInst::INVALID_INDEX));

	NON_SPU_ONLY(PHLEVELNEW_ASSERT(GetConstOctreeNode(uOctreeNodeIndex)->m_VisitingObjectCount == 0));

	--m_uOctreeNodesInUse;
	NON_SPU_ONLY(m_pauOctreeNodeIndexList[m_uOctreeNodesInUse] = uOctreeNodeIndex);
	SPU_ONLY(sysDmaPutUInt16(uOctreeNodeIndex, (uint64_t)&m_pauOctreeNodeIndexList[m_uOctreeNodesInUse], DMA_TAG(1)));
}

#if !__SPU
template <class __NodeType> u16 phLevelNodeTree<__NodeType>::GetObjectIndexFromPool()
{
	PHLEVELNEW_ASSERT(m_NumObjects < m_MaxObjects);

	u16 levelIndex = m_pauObjectDataIndexList[m_NumObjects];
	++m_NumObjects;

	m_CurrentMaxLevelIndex = Max((int)levelIndex, m_CurrentMaxLevelIndex);
#if __BANK
	sm_MaxObjectsEver = Max(sm_MaxObjectsEver,(int)m_NumObjects);
	sm_TotalObjectsAdded++;
#endif
	return levelIndex;
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::ReturnObjectIndexToPool(u16 uObjectIndex)
{
	PHLEVELNEW_ASSERT(m_NumObjects > 0);

#if LEVELNEW_ACTIVE_LEVELINDEX_DUPE_CHECKING
	// We just blindly tack the returned index on to our pool which means a double release could cause us to issue
	//  the same level index to two separate insts later
	// Lets check for that now
	int nObjectIndex;
	for(nObjectIndex = m_MaxObjects-1; nObjectIndex >= m_NumObjects; --nObjectIndex)
	{
		if( Unlikely(uObjectIndex == m_pauObjectDataIndexList[nObjectIndex]) )
		{
			// TODO: Switch to FastAssert whenever that or some equivalent resumes working in Bankrelease
			__debugbreak();
		}
	}
#endif

	--m_NumObjects;
#if USE_DETERMINISTIC_ORDERING
	// Keep the indices sorted.
	int i = m_NumObjects;
	while ((i < m_MaxObjects - 1) && (uObjectIndex > m_pauObjectDataIndexList[i+1]))
	{
		m_pauObjectDataIndexList[i] = m_pauObjectDataIndexList[i+1];
		i++;
	}
	m_pauObjectDataIndexList[i] = uObjectIndex;
#else // USE_DETERMINISTIC_ORDERING
	m_pauObjectDataIndexList[m_NumObjects] = uObjectIndex;
#endif // USE_DETERMINISTIC_ORDERING

	if (m_CurrentMaxLevelIndex == uObjectIndex)
	{
		int levelIndex = m_CurrentMaxLevelIndex - 1;
		while (!IsInLevel(levelIndex) && levelIndex > 0)
		{
			--levelIndex;
		}
		m_CurrentMaxLevelIndex = levelIndex;
	}

#if __BANK
	sm_TotalObjectsDeleted++;
#endif
}
#endif	// !__SPU

template <class __NodeType> const char* phLevelNodeTree<__NodeType>::GetInstDescription(const phInst& rInst, char *pBuffer, int iBufferSize) const
{
#if !__SPU
	formatf(pBuffer, iBufferSize, "0x%p", &rInst);
#else	// !__SPU
	// On SPU we need to do some DMAing in order to get the object name.
	const phArchetype *ppuArchetype = reinterpret_cast<const phArchetype *>(sysDmaGetUInt32((uint64_t)rInst.GetArchetypePtr(), DMA_TAG(1)));

	u8 archetypeBuffer[sizeof(phArchetype)] ;
	sysDmaLargeGet(&archetypeBuffer[0], (uint64_t)ppuArchetype, sizeof(phArchetype), DMA_TAG(1));
	const phArchetype *spuArchetype = reinterpret_cast<const phArchetype *>(&archetypeBuffer[0]);
	sysDmaWaitTagStatusAll(DMA_MASK(1));

	sysDmaGet(pBuffer, (uint64_t)spuArchetype->GetFilename(), iBufferSize, DMA_TAG(1));
	sysDmaWaitTagStatusAll(DMA_MASK(1));
#endif	// !__SPU
	return pBuffer;
}

// This is similar to AddXXXObject, except it is to be used when you already have an instance that has a level index.
template <class __NodeType> u16 phLevelNodeTree<__NodeType>::AddObjectToOctree(u16 uLevelIndex, phObjectData &objectDataToAdd, Vec3V_In krvecCenter, const float kfRadius)
{
	NON_SPU_ONLY(Assert(&m_paObjectData[uLevelIndex] == &objectDataToAdd));	// Ensure that the information that they passed in is consistent.

	SPU_ONLY(phSpuObjectBuffer<__NodeType> octreeNodeBuffer);
	__NodeType *rootOctreeNode = NON_SPU_ONLY(GetOctreeNode(0)) SPU_ONLY(octreeNodeBuffer.FetchObject(GetOctreeNode(0)));
	Assert(rootOctreeNode->IsPointWithinNode(krvecCenter));

	const int nOctreeNodeIndex = InsertObjectIntoSubtree(0, rootOctreeNode, krvecCenter, kfRadius, uLevelIndex, objectDataToAdd SPU_PARAM(octreeNodeBuffer));

#if __ASSERT && !__SPU
	if(nOctreeNodeIndex != PHLEVELNEW_EVERYWHERE_NODE_INDEX
#ifdef GTA_REPLAY_RAGE
		&& noPlacementAssert == false
#endif //GTA_REPLAY_RAGE
		)
	{
		// TODO: Maybe add this to the SPU code?  We'll see if we have enough room.
		// Search through this node to find any with identical matrices and archetypes. This is a common game-level placement problem
		// that is difficult to detect by visual inspection.
		int objectIndex = GetOctreeNode(nOctreeNodeIndex)->m_ContainedObjectStartIdx;
		phInst* inst = objectDataToAdd.GetInstance();
		phArchetype* archetype = inst->GetArchetype();
		while(objectIndex != phInst::INVALID_INDEX)
		{
			phObjectData& otherObjectData = m_paObjectData[objectIndex];
			if(objectIndex != uLevelIndex)
			{
				phInst* otherInst = otherObjectData.GetInstance();
				if (otherInst->GetArchetype() == archetype)
				{
					if(IsEqualAll(inst->GetMatrix(),otherInst->GetMatrix()))
					{
						char tmpBufferA[256], tmpBufferB[256];
						GetInstDescription(*inst, tmpBufferA, sizeof(tmpBufferA));
						GetInstDescription(*otherInst, tmpBufferB, sizeof(tmpBufferB));
						artAssertf(0, "Multiple objects with the same archetype '%s' LI:%d,%d found at the location <%f, %f, %f>. This is probably a placement error. (phInsts %s, %s)", archetype->GetFilename(), inst->GetLevelIndex(), otherInst->GetLevelIndex(), (*(const Matrix34*)(&inst->GetMatrix())).d.x, (*(const Matrix34*)(&inst->GetMatrix())).d.y, (*(const Matrix34*)(&inst->GetMatrix())).d.z, tmpBufferA, tmpBufferB);
						break;
					}
				}
			}
			objectIndex = otherObjectData.m_uNextObjectInNodeIndex;
		}
	}
#endif
	SPU_ONLY(octreeNodeBuffer.ReturnObject());

	return(u16)(nOctreeNodeIndex);
}


// This function removes an object from the octree, but leaves its object data (and level index) intact.
// This is similar to DeleteObject except that it doesn't try to recycle the level index (it lets the object keep it).
// This is used when an object is removed from the physics level (duh) and also when an object has gone out of the bounds of
//   the physics level.
template <class __NodeType> void phLevelNodeTree<__NodeType>::RemoveObjectFromOctree(__NodeType *pInitialOctreeNode, const u32 uLevelIndex, phObjectData &objectDataToRemove, const float kfRadius)
{
	phObjectData *pCurObjectData = &objectDataToRemove;
	NON_SPU_ONLY(FastAssert((pInitialOctreeNode - &m_paOctreeNodes[0]) == objectDataToRemove.GetOctreeNodeIndex()));	// Ensure that the data passed in is consistent.

	RemoveObjectFromOctreeNode(pInitialOctreeNode, uLevelIndex, objectDataToRemove);

	if(kfRadius <= 0.5f * pInitialOctreeNode->m_CenterXYZHalfWidthW.GetWf())
	{
		// The object is a visiting object in this node.
		pInitialOctreeNode->DecrementVisitingObjectCount(pCurObjectData->GetVisitingObjectChildIndex());
	}
	PruneObject(pInitialOctreeNode, objectDataToRemove.GetOctreeNodeIndex());
}


// This function finds the lowest ancestor to knStartingOctreeNodeIndex that could possibly contain the specified object.  It assumes that the
//   object was previously located in octree node knStartingOctreeNodeIndex, and adjusts the data in the octree nodes (such as collapsing nodes, 
//   reducing maximum child radii, and removing state flags) from knStartingOctreeNodeIndex up to the final node to reflect the fact that the given
//   object is no longer located in its subtree.
// This is useful for when an object has changed locations (and hence should still be in the octree somewhere) as it comprises the first half
//   of what is necessary to properly relocate an object.  Prior to this, the object needs to have been removed from knStartingOctreeNodeIndex.
// Note: This method will not make an any adjustments to the final node to which the object is pulled (this, of course, is logical, because there 
//   would be no changes there to make there).
// Returns: The index of the node to which the object was pulled.
template <class __NodeType> u16 phLevelNodeTree<__NodeType>::PullSphere(const int knStartingOctreeNodeIndex, __NodeType *pInitialOctreeNode, Vec3V_In krvecCenter, const float kfRadius SPU_PARAM(phSpuObjectBuffer<__NodeType> &octreeNodeBuffer))
{
	NON_SPU_ONLY(Assert(pInitialOctreeNode == GetOctreeNode(knStartingOctreeNodeIndex)));
	SPU_ONLY(Assert(pInitialOctreeNode == octreeNodeBuffer.GetSpuObject()));
	__NodeType *pCurOctreeNode = pInitialOctreeNode;
	PHLEVELNEW_ASSERT(!pCurOctreeNode->IsPointWithinNode(krvecCenter) || kfRadius > pCurOctreeNode->m_CenterXYZHalfWidthW.GetWf());
	Assert(pInitialOctreeNode->m_ContainedObjectCount > 0 || pInitialOctreeNode->m_ChildNodeCount > 0);

	// TODO: This could be improved because we know that no CollapseChild() calls will come after we've had an iteration without a CollapseChild()
	//   call.  So we could have one loop moving up the tree looking for children to collapse and then a second, much simpler loop, that picks up
	//   where the first left off, that just moves up the tree looking for where to stop.
	u32 uCurOctreeNodeIndex = (u32)knStartingOctreeNodeIndex;
	while(true)
	{
		// Since we might kill this node soon, we'll just cache off its parent node index.
		const u32 uNextOctreeNodeIndex = pCurOctreeNode->m_ParentNodeIndex;

		if(pCurOctreeNode->m_ChildNodeCount == 0 && pCurOctreeNode->m_ContainedObjectCount <= m_nMaxObjectsInNodeToCollapseChild && uCurOctreeNodeIndex != 0)
		{
			// This node doesn't have enough objects in it and it doesn't have any children, so it should get collapsed into its parent.
			CollapseChild(pCurOctreeNode, uCurOctreeNodeIndex);
		}

		// This is somewhat of overkill as most of the time we probably haven't changed pCurOctreeNode if it hasn't been removed from use.
		SPU_ONLY(octreeNodeBuffer.ReturnObject());

		if(Unlikely(uNextOctreeNodeIndex == (u16)(-1)))
		{
			// We're already at the top of the octree.  This can't actually happen unless an object doesn't fit into the root node for some reason (probably
			//   can only happen if it's too big?)
			break;
		}
		pCurOctreeNode = NON_SPU_ONLY(GetOctreeNode(uNextOctreeNodeIndex)) SPU_ONLY(octreeNodeBuffer.FetchObject(GetOctreeNode(uNextOctreeNodeIndex)));
		uCurOctreeNodeIndex = uNextOctreeNodeIndex;

		if(pCurOctreeNode->IsPointWithinNode(krvecCenter) && kfRadius <= pCurOctreeNode->m_CenterXYZHalfWidthW.GetWf())
		{
			// The object we're pulling fits within this node so we're done.
			break;
		}
	}
	
	return uCurOctreeNodeIndex;
}


// This function finds the lowest descendant of knStartingOctreeNodeIndex that could possibly contain the specified object and inserts it there.  It assumes that the
//   object was previously located in octree node knStartingOctreeNodeIndex, and adjusts the data in the octree nodes (such as creating child nodes, 
//   increasing maximum child radii, and adding state flags) from knStartingOctreeNodeIndex down to the final node to reflect the fact that the given
//   object is now located in its subtree.
// This is useful for when an object has changed locations (and hence should still be in the octree somewhere) as it comprises the second half
//   of what is necessary to properly relocate the object.  After this, the object should be added to the final octree node.
// Note: This method will not make an any adjustments to the starting node from which the object is pushed (this, of course, is logical, because 
//   there would be no changes there to make there).
// Note: knStartingOctreeNodeIndex must be a valid starting octree node (this is checked by the Assert).
// Note: The octree structure itself should be intact and consistent, with the exception that the object to which this sphere corresponds should
//   *not* be accounted for in the tree at all (it should not be in any node's object list, it should not be counted as a visiting object in any
//   node, etc).
// Note: On SPU, this function sends back all octree nodes that it visits, possibly including the initial octree node, *except* for the final node
//   upon which is leaves octreeNodeBuffer.  Therefore, clients should not need to keep track of the initial octree node so as to send it back to the
//   PPU but rather need only worry about the final node (to which they're likely to want to make changes).
// Returns: This index of the node to which the object was pushed.
template <class __NodeType> u16 phLevelNodeTree<__NodeType>::InsertObjectIntoSubtree(const int knStartingOctreeNodeIndex, __NodeType *pInitialOctreeNode, Vec3V_In krvecCenter, const float kfRadius, u16 uLevelIndex, phObjectData &objectDataToAdd SPU_PARAM(phSpuObjectBuffer<__NodeType> &octreeNodeBuffer))
{
	int nCurOctreeNodeIndex = knStartingOctreeNodeIndex;
	__NodeType *pCurOctreeNode = pInitialOctreeNode;
	NON_SPU_ONLY(Assert(pInitialOctreeNode == GetOctreeNode(knStartingOctreeNodeIndex)));
	SPU_ONLY(Assert(pInitialOctreeNode == octreeNodeBuffer.GetSpuObject()));

	PHLEVELNEW_ASSERT(pCurOctreeNode->IsPointWithinNode(krvecCenter));
	PHLEVELNEW_ASSERT(pCurOctreeNode->m_CenterXYZHalfWidthW.GetWf() >= kfRadius || knStartingOctreeNodeIndex == 0);

	// Keep trying to go deeper down the octree as long as the object will fit.
	while(kfRadius <= 0.5f * pCurOctreeNode->m_CenterXYZHalfWidthW.GetWf())
	{
		// We're not at the right depth yet.  Let's find out through which child we should descend.
		int nOctreeNodeChildIndex = pCurOctreeNode->SubClassifyPoint(krvecCenter);
		int nNewOctreeNodeIndex = pCurOctreeNode->m_ChildNodeIndices[nOctreeNodeChildIndex];

		if(nNewOctreeNodeIndex == (u16)(-1))
		{
			Assert(pCurOctreeNode->GetVisitingObjectCount(nOctreeNodeChildIndex) <= pCurOctreeNode->m_ContainedObjectCount);
			// There's no child at that location, let's see if we should create one or not.
			if(pCurOctreeNode->GetVisitingObjectCount(nOctreeNodeChildIndex) + 1 < m_nMinObjectsInNodeToCreateChild || (pCurOctreeNode->m_CenterXYZHalfWidthW.GetWf() <= kfLeafNodeWidthFactor * m_fLevelHalfWidth) || (m_uOctreeNodesInUse >= m_uTotalOctreeNodeCount))
			{
				if(pCurOctreeNode->RoomForAdditionalVisitingObject(nOctreeNodeChildIndex))
				{
					// Either there aren't enough visiting objects to warrant a new node, or we've bottomed out and don't want to make the octree any deeper, or there aren't any octree nodes left.
					// Either way we'll just stop here and mark it as a visiting object.
					pCurOctreeNode->IncrementVisitingObjectCount(nOctreeNodeChildIndex);
					objectDataToAdd.SetVisitingObjectChildIndex(nOctreeNodeChildIndex);
					break;
				}
				else
				{
					// We don't want to or can't make a new node, add the object to the everywhere node and let the user know we're about to get slower.
					NON_SPU_ONLY(ASSERT_ONLY(DumpNode(*pCurOctreeNode)));
					Assertf(false,"Ran out of visiting objects in this node. Check log for details.");
					objectDataToAdd.SetOctreeNodeIndex(PHLEVELNEW_EVERYWHERE_NODE_INDEX);
					AddObjectToEverywhereNode(uLevelIndex, objectDataToAdd);
					return PHLEVELNEW_EVERYWHERE_NODE_INDEX;
				}
			}

			// We meet all the conditions to create a new child node
			nNewOctreeNodeIndex = CreateChild(nCurOctreeNodeIndex, pCurOctreeNode, nOctreeNodeChildIndex);
			SPU_ONLY(octreeNodeBuffer.ReturnObject());
			pCurOctreeNode = NON_SPU_ONLY(GetOctreeNode(nNewOctreeNodeIndex)) SPU_ONLY(octreeNodeBuffer.FetchObject(GetOctreeNode(nNewOctreeNodeIndex)));
			PHLEVELNEW_ASSERT(pCurOctreeNode->IsPointWithinNode(pCurOctreeNode->m_CenterXYZHalfWidthW.GetXYZ()));
			PHLEVELNEW_ASSERT(pCurOctreeNode->IsPointWithinNode(krvecCenter));
		}
		else
		{
			SPU_ONLY(if(pCurOctreeNode == pInitialOctreeNode) { octreeNodeBuffer.ReturnObject(); });
			pCurOctreeNode = NON_SPU_ONLY(GetOctreeNode(nNewOctreeNodeIndex)) SPU_ONLY(octreeNodeBuffer.FetchObject(GetOctreeNode(nNewOctreeNodeIndex)));
		}

		// Advance down the tree.
		PHLEVELNEW_ASSERT(nNewOctreeNodeIndex < m_uTotalOctreeNodeCount);
		nCurOctreeNodeIndex = nNewOctreeNodeIndex;
		PHLEVELNEW_ASSERT(pCurOctreeNode->IsPointWithinNode(krvecCenter));
	}

	// We found a valid node, add the object to it
	__NodeType *pFinalOctreeNode = NON_SPU_ONLY(GetOctreeNode(nCurOctreeNodeIndex)) SPU_ONLY(octreeNodeBuffer.GetSpuObject());
	AddObjectToOctreeNode(pFinalOctreeNode, uLevelIndex, objectDataToAdd);
	objectDataToAdd.SetOctreeNodeIndex((u16)(nCurOctreeNodeIndex));

	PHLEVELNEW_ASSERT(pFinalOctreeNode->IsPointWithinNode(center));
	PHLEVELNEW_ASSERT(IsObjectContained(pFinalOctreeNode, nLevelIndex SPU_PARAM(objectDataToUpdate)));

	return (u16)nCurOctreeNodeIndex;
}


// This function adjusts the octree to compensate for the removal of an object with a given radius and with a given object state.
template <class __NodeType> void phLevelNodeTree<__NodeType>::PruneObject(__NodeType *pStartingOctreeNode, int startingOctreeNodeIndex)
{
	__NodeType *pCurOctreeNode = pStartingOctreeNode;
	int uCurOctreeNodeIndex = startingOctreeNodeIndex;
	SPU_ONLY(phSpuObjectBuffer<__NodeType> octreeNodeBuffer);
	while(true)
	{
		// Since we might kill this node soon, we'll just cache off its parent node index.
		u32 uNextOctreeNodeIndex = pCurOctreeNode->m_ParentNodeIndex;

		const int numObjectsInNode = pCurOctreeNode->m_ContainedObjectCount;
		if(pCurOctreeNode->m_ChildNodeCount == 0 && numObjectsInNode <= m_nMaxObjectsInNodeToCollapseChild && uNextOctreeNodeIndex != (u16)(-1))
		{
			// This node doesn't have enough objects in it and it doesn't have any children, so it should get collapsed into its parent.
			CollapseChild(pCurOctreeNode, uCurOctreeNodeIndex);
		}

#if __SPU
		// This is somewhat of overkill as most of the time we probably haven't changed pCurOctreeNode.
		if(pCurOctreeNode != pStartingOctreeNode)
		{
			octreeNodeBuffer.ReturnObject();
		}
#endif	// __SPU

		if(Unlikely(uNextOctreeNodeIndex == (u16)(-1)))
		{
			// We're already at the top of the octree.  This can't actually happen unless an object doesn't fit into the root node for some reason (probably
			//   can only happen if it's too big?)
			break;
		}
		uCurOctreeNodeIndex = uNextOctreeNodeIndex;
		pCurOctreeNode = NON_SPU_ONLY(GetOctreeNode(uCurOctreeNodeIndex)) SPU_ONLY(octreeNodeBuffer.FetchObject(GetOctreeNode(uCurOctreeNodeIndex)));
	}
}


// This method adds an object to the linked list *only*.
template <class __NodeType> void phLevelNodeTree<__NodeType>::AddObjectToOctreeNode(__NodeType *pInitialOctreeNode, const int knLevelIndex, phObjectData &objectDataToAdd)
{
	PHLEVELNEW_ASSERT(knLevelIndex < m_MaxObjects);
	NON_SPU_ONLY(Assert(&m_paObjectData[knLevelIndex] == &objectDataToAdd));	// Ensure that the information that they passed in is consistent.

	__NodeType *pCurOctreeNode = pInitialOctreeNode;
	PHLEVELNEW_ASSERT(pCurOctreeNode->m_ContainedObjectCount < 255);

	objectDataToAdd.m_uNextObjectInNodeIndex = pCurOctreeNode->m_ContainedObjectStartIdx;
	pCurOctreeNode->m_ContainedObjectStartIdx = (u16)(knLevelIndex);

#if __ASSERT
	// SPU TODO: Make this work on SPU.
	if(pCurOctreeNode->m_ContainedObjectCount >= 255)
	{
#if !__SPU
		Displayf("Objects in node %p", pCurOctreeNode);
		int nextObjectIndex = pCurOctreeNode->m_ContainedObjectStartIdx;
		while(nextObjectIndex != phInst::INVALID_INDEX)
		{
			const phObjectData &objectData = m_paObjectData[nextObjectIndex];
			const phInst *inst = objectData.GetInstance();
			const phArchetype *arch = inst->GetArchetype();
			Displayf("%d: %s", nextObjectIndex, arch->GetFilename());
			nextObjectIndex = m_paObjectData[nextObjectIndex].m_uNextObjectInNodeIndex;
		}
#endif	// !__SPU
		Quitf("Too many objects in node.  This is fatal.");
	}
#endif
	++pCurOctreeNode->m_ContainedObjectCount;
}


// This method removes the first object from the 'contained object' linked list and reflects that change in the node's object count.
template <class __NodeType> void phLevelNodeTree<__NodeType>::RemoveFirstObjectFromOctreeNode(__NodeType *pCurOctreeNode, const phObjectData &objectDataToRemove)
{
	NON_SPU_ONLY(Assert((&objectDataToRemove - &m_paObjectData[0]) == pCurOctreeNode->m_ContainedObjectStartIdx));
	pCurOctreeNode->m_ContainedObjectStartIdx = objectDataToRemove.m_uNextObjectInNodeIndex;
	--pCurOctreeNode->m_ContainedObjectCount;
}

// This method removes the object from the 'contained object' linked list and reflects that change in the node's object count.  By virtual of the
//   fact that this works by changing the 'next' object index of the previous 'object data' object, this function can only be used for objects that
//   are *not* the first object in the list (whose index is stored in the octree node).
// This is much faster than the other version of RemoveObjectFromOctreeNode() but it requires the user to pass in the previous object data.  As such,
//   you would typically only use this function in situation when you are already iterating over the objects in the node.
template <class __NodeType> void phLevelNodeTree<__NodeType>::RemoveObjectFromOctreeNode(__NodeType *pCurOctreeNode, const phObjectData &objectDataToRemove, phObjectData &prevObjectData)
{
	NON_SPU_ONLY(Assert((&objectDataToRemove - &m_paObjectData[0]) == prevObjectData.m_uNextObjectInNodeIndex));
	prevObjectData.m_uNextObjectInNodeIndex = objectDataToRemove.m_uNextObjectInNodeIndex;
	--pCurOctreeNode->m_ContainedObjectCount;
}

// This method removes the object from the linked list and reflects that change in the node's object count.
// If you are iterating through an octree node when you want to delete, you should use the other RemoveObjectFromOctreeNode() to avoid doing
//   another unnecessary iteration through the objects in the node.
// On PPU, the level index could be easily calculated from the address of the object data but on the SPU this is not the case, thus we just pass
//   both in both cases.
template <class __NodeType> void phLevelNodeTree<__NodeType>::RemoveObjectFromOctreeNode(__NodeType *pCurOctreeNode, const u32 knLevelIndex, const phObjectData &objectDataToRemove)
{
	FastAssert(pCurOctreeNode->m_ContainedObjectCount > 0);
	NON_SPU_ONLY(Assert((&objectDataToRemove - &m_paObjectData[0]) == (int)knLevelIndex));	// Ensure that the information that they passed in is consistent.
	PHLEVELNEW_ASSERT(IsObjectContained(pCurOctreeNode, knLevelIndex SPU_PARAM(objectDataToRemove)));

	const u32 firstObjectInNodeLevelIndex = pCurOctreeNode->m_ContainedObjectStartIdx;
	if(pCurOctreeNode->m_ContainedObjectStartIdx == knLevelIndex)
	{
		// The object that we're removing is the first object in the linked list.
		pCurOctreeNode->m_ContainedObjectStartIdx = objectDataToRemove.m_uNextObjectInNodeIndex;
	}
	else
	{
		// The object that we're removing is not the first object in the linked list.  Let's find the object data that comes before it in the list.
		SPU_ONLY(phSpuObjectBuffer<phObjectData> objectDataBuffer);
		phObjectData *nextObjectData = NON_SPU_ONLY(&m_paObjectData[firstObjectInNodeLevelIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&m_paObjectData[firstObjectInNodeLevelIndex]));
		u32 nextObjectInNodeLevelIndex = nextObjectData->m_uNextObjectInNodeIndex;
		FastAssert(nextObjectInNodeLevelIndex != phInst::INVALID_INDEX);
		while(nextObjectInNodeLevelIndex != knLevelIndex)
		{
			nextObjectData = NON_SPU_ONLY(&m_paObjectData[nextObjectInNodeLevelIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&m_paObjectData[nextObjectInNodeLevelIndex]));
			nextObjectInNodeLevelIndex = nextObjectData->m_uNextObjectInNodeIndex;
			FastAssert(nextObjectInNodeLevelIndex != phInst::INVALID_INDEX);
		}

		// Found the object, now let's tell it who its new next object is.
		nextObjectData->m_uNextObjectInNodeIndex = objectDataToRemove.m_uNextObjectInNodeIndex;
		SPU_ONLY(objectDataBuffer.ReturnObject());
	}

	--pCurOctreeNode->m_ContainedObjectCount;
	PHLEVELNEW_ASSERT(!IsObjectContained(pCurOctreeNode, knLevelIndex SPU_PARAM(objectDataToRemove)));
}


#if LEVELNEW_USE_EVERYWHERE_NODE
template <class __NodeType> void phLevelNodeTree<__NodeType>::AddObjectToEverywhereNode(const int knLevelIndex, phObjectData &objectData)
{
	NON_SPU_ONLY(Assert((&objectData - &m_paObjectData[0]) == knLevelIndex));
	objectData.m_uNextObjectInNodeIndex = m_uFirstObjectInEverywhereNodeIndex;
	m_uFirstObjectInEverywhereNodeIndex = (u16)(knLevelIndex);
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::RemoveObjectFromEverywhereNode(const int knLevelIndex, phObjectData &objectData)
{
	if(m_uFirstObjectInEverywhereNodeIndex == knLevelIndex)
	{
		// This is the first object in the everywhere node.  Just change the head pointer so that it doesn't point to that object any more.
		m_uFirstObjectInEverywhereNodeIndex = objectData.m_uNextObjectInNodeIndex;
	}
	else
	{
		// The object that we're removing is not the first object in the linked list.  Let's find the object data that comes before it in the list.
		SPU_ONLY(phSpuObjectBuffer<phObjectData> objectDataBuffer);
		phObjectData *nextObjectData = NON_SPU_ONLY(&m_paObjectData[m_uFirstObjectInEverywhereNodeIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&m_paObjectData[m_uFirstObjectInEverywhereNodeIndex]));
		int nextObjectInNodeLevelIndex = nextObjectData->m_uNextObjectInNodeIndex;
		FastAssert(nextObjectInNodeLevelIndex != phInst::INVALID_INDEX);
		while(nextObjectInNodeLevelIndex != knLevelIndex)
		{
			nextObjectData = NON_SPU_ONLY(&m_paObjectData[nextObjectInNodeLevelIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&m_paObjectData[nextObjectInNodeLevelIndex]));
			nextObjectInNodeLevelIndex = nextObjectData->m_uNextObjectInNodeIndex;
			FastAssert(nextObjectInNodeLevelIndex != phInst::INVALID_INDEX);
		}

		// Found the object, now let's tell it who its new next object is.
		nextObjectData->m_uNextObjectInNodeIndex = objectData.m_uNextObjectInNodeIndex;
		SPU_ONLY(objectDataBuffer.ReturnObject());
	}
}
#endif


template <class __NodeType> int phLevelNodeTree<__NodeType>::CreateChild(const int knOctreeNodeIndex, __NodeType *pParentOctreeNode, const int knChildIndex)
{
	NON_SPU_ONLY(FastAssert(GetConstOctreeNode(knOctreeNodeIndex) == pParentOctreeNode));
	// First of all, let's get a new octree node.  We don't need to check for failure here because that was the responsibility of the calling code.
	int nChildOctreeNodeIndex = GetOctreeNodeIndexFromPool();
	SPU_ONLY(phSpuObjectBuffer<__NodeType> childOctreeNodeBuffer);
	__NodeType *pChildOctreeNode = NON_SPU_ONLY(GetOctreeNode(nChildOctreeNodeIndex)) SPU_ONLY(childOctreeNodeBuffer.FetchObject(GetOctreeNode(nChildOctreeNodeIndex)));

	// Let's get the new node attached to the octree properly.
	pParentOctreeNode->ConnectChild(knChildIndex, nChildOctreeNodeIndex);
	pChildOctreeNode->ConnectParent(knOctreeNodeIndex);

	// Let's set up the extents of the new octree node.
	pChildOctreeNode->InitExtentsAsChild(pParentOctreeNode, knChildIndex);

	// It should always be the case that we're creating a child when the parent node contains one fewer object than the count at which we go to create nodes.
	// If this fails, either we're creating a node before we should be, or we didn't create a child node when we should have.
	Assert(pParentOctreeNode->GetVisitingObjectCount(knChildIndex) + 1 == m_nMinObjectsInNodeToCreateChild);

	// Now let's move all of the objects out of the parent that should actually be in the child.
	phObjectData *paObjectData = m_paObjectData;
	FastAssert(pParentOctreeNode->m_ContainedObjectCount + 1 >= m_nMinObjectsInNodeToCreateChild);

	SPU_ONLY(phSpuObjectBuffer<phObjectData> objectDataBuffer);
	u32 curLevelIndex = pParentOctreeNode->m_ContainedObjectStartIdx;
	do
	{
		phObjectData *pCurObjectData = NON_SPU_ONLY(&paObjectData[curLevelIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&paObjectData[curLevelIndex]));
		const u32 nextLevelIndex = pCurObjectData->m_uNextObjectInNodeIndex;
		float fObjectRadius = pCurObjectData->m_CachedCenterAndRadius.GetWf();
		if(fObjectRadius <= 0.5f * pParentOctreeNode->m_CenterXYZHalfWidthW.GetWf())
		{
#if __ASSERT && !__SPU
			// Make sure the object's center is in the same child node that the object data thinks it is in.
			if (!CanOctreeBeOutOfSync())
			{
				const phInst *pCurInst = pCurObjectData->GetInstance();
				Vec3V objectCenter = pCurInst->GetArchetype()->GetBound()->GetWorldCentroid(pCurInst->GetMatrix());
				int objectCenterChildIndex = pParentOctreeNode->SubClassifyPoint(objectCenter);
				int visitingObjectChildIndex = pCurObjectData->GetVisitingObjectChildIndex();
				if (objectCenterChildIndex != visitingObjectChildIndex)
				{
					// If this fails, then the object's center was moved across octree cell boundaries without calling
					// PHLEVEL->UpdateObjectLocation(levelIndex).
					// To fix this, look at pCurInst->m_Archetype->FileName to see what the object is, and then look for calls
					// to SetMatrix() in game code that aren't followed by calls to PHLEVEL->UpdateObjectLocation(levelIndex).
#if DEBUG_PHINST_TRACK_SETMATRIX_CALLSTACK
					sysStack::PrintCapturedStackTrace(pCurInst->GetLastSetMatrixCallstack(),32);
#endif
					Assertf(0,"%s was moved without notifying the physics level with PHLEVEL->UpdateObjectLocation().",pCurInst->GetArchetype()->GetFilename());
					Warningf("An object '%s' [%d] was moved without notifying the physics level (use PHLEVEL->UpdateObjectLocation(levelIndex).", pCurInst->GetArchetype()->GetFilename(), pCurInst->GetLevelIndex());
				}
			}
#endif
			// TODO: Add assert here to verify that the visiting object child index is correct.
			if(pCurObjectData->GetVisitingObjectChildIndex() == knChildIndex)
			{
				// This object should be pulled down into our new node.
				// TODO: Use the other RemoveObjectFromOctreeNode() to prevent unnecessary iteration here.
				PHLEVELNEW_ASSERT(IsObjectContained(pParentOctreeNode, curLevelIndex SPU_PARAM(*pCurObjectData)));
				RemoveObjectFromOctreeNode(pParentOctreeNode, curLevelIndex, *pCurObjectData);
				PHLEVELNEW_ASSERT(!IsObjectContained(pParentOctreeNode, curLevelIndex SPU_PARAM(*pCurObjectData)));

				if(fObjectRadius <= 0.5f * pChildOctreeNode->m_CenterXYZHalfWidthW.GetWf())
				{
					// This object still a visiting object in the child node, so let's mark it as such.
					const Vec3V objectCenter = pCurObjectData->m_CachedCenterAndRadius.GetXYZ();
					const int knNewChildIndex = pChildOctreeNode->SubClassifyPoint(objectCenter);
					if(pChildOctreeNode->RoomForAdditionalVisitingObject(knNewChildIndex))
					{
						AddObjectToOctreeNode(pChildOctreeNode, curLevelIndex, *pCurObjectData);
						PHLEVELNEW_ASSERT(IsObjectContained(pChildOctreeNode, curLevelIndex SPU_PARAM(*pCurObjectData)));
						pChildOctreeNode->IncrementVisitingObjectCount(knNewChildIndex);
						//Assert(pChildOctreeNode->GetVisitingObjectCount(knNewChildIndex) < m_nMinObjectsInNodeToCreateChild);	// Should be a valid assert but probably not needed.
						pCurObjectData->SetVisitingObjectChildIndex(knNewChildIndex);
					}
					else
					{
						// We don't have enough room to add a visiting object so we need to move this to the everywhere node
						// NOTE: 
						//   This is only possible because the objects might not be in their correct place within the node yet. 
						//   We could have close to the limit of visiting objects in child we're creating but there could be
						//   visiting objects in other children that belong in the new node. When we add those we'll go over the limit.
						NON_SPU_ONLY(ASSERT_ONLY(DumpNode(*pChildOctreeNode)));
						Assertf(false,"Ran out of visiting objects in this node. Check log for details.");
						pCurObjectData->SetOctreeNodeIndex(PHLEVELNEW_EVERYWHERE_NODE_INDEX);
						AddObjectToEverywhereNode(curLevelIndex, *pCurObjectData);
					}
				}
				else
				{
					// This object belongs in this node
					AddObjectToOctreeNode(pChildOctreeNode, curLevelIndex, *pCurObjectData);
					PHLEVELNEW_ASSERT(IsObjectContained(pChildOctreeNode, curLevelIndex SPU_PARAM(*pCurObjectData)));
				}

				pCurObjectData->SetOctreeNodeIndex((u16)(nChildOctreeNodeIndex));
				SPU_ONLY(objectDataBuffer.ReturnObject());
			}
		}

		curLevelIndex = nextLevelIndex;
	}
	while(curLevelIndex != phInst::INVALID_INDEX);
	pParentOctreeNode->ClearVisitingObjectCount(knChildIndex);

	SPU_ONLY(childOctreeNodeBuffer.ReturnObject());

	// TODO: Shouldn't we be checking to see if pChildOctreeNode itself should have any children created?
	return nChildOctreeNodeIndex;
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::CollapseChild(__NodeType *octreeNodeToCollapse, const int childOctreeNodeIndex)
{
	__NodeType *pChildOctreeNode = octreeNodeToCollapse;
	PHLEVELNEW_ASSERT(IsObjectCountCorrect(pChildOctreeNode, -1, NULL, 0));
	Assert(pChildOctreeNode->m_ChildNodeCount == 0);
	const int knParentOctreeNodeIndex = pChildOctreeNode->m_ParentNodeIndex;
	SPU_ONLY(phSpuObjectBuffer<__NodeType> parentOctreeNodeBuffer);
	__NodeType *pParentOctreeNode = NON_SPU_ONLY(GetOctreeNode(knParentOctreeNodeIndex)) SPU_ONLY(parentOctreeNodeBuffer.FetchObject(GetOctreeNode(knParentOctreeNodeIndex)));
	PHLEVELNEW_ASSERT(IsObjectCountCorrect(pParentOctreeNode, -1 ,NULL, 1));

	const int knChildIndex = pParentOctreeNode->SubClassifyPoint( pChildOctreeNode->m_CenterXYZHalfWidthW.GetXYZ() );
	PHLEVELNEW_ASSERT(pParentOctreeNode->m_ChildNodeIndices[knChildIndex] == (u16)(childOctreeNodeIndex));
	FastAssert(pParentOctreeNode->GetVisitingObjectCount(knChildIndex) == 0);

	// Let's move all of the objects out of the child node and into the parent node.
	const int numObjectsInChildNode = pChildOctreeNode->m_ContainedObjectCount;
	FastAssert(numObjectsInChildNode > 0);	// If this can fail then we should wrap this whole thing in an if.
	const int firstChildNodeObjectIndex = pChildOctreeNode->m_ContainedObjectStartIdx;
	FastAssert(firstChildNodeObjectIndex != phInst::INVALID_INDEX);

	SPU_ONLY(phSpuObjectBuffer<phObjectData> objectDataBuffer);

	// NOTE: We shouldn't have to worry about overrunning the visiting count as long as m_nMaxObjectsInNodeToCollapseChild is less than the max number of visiting objects
	int childNodeObjectIndex = firstChildNodeObjectIndex;
	while(true)
	{
		pParentOctreeNode->IncrementVisitingObjectCount(knChildIndex);
		phObjectData *curObjectData = NON_SPU_ONLY(&m_paObjectData[childNodeObjectIndex]) SPU_ONLY(objectDataBuffer.FetchObject(&m_paObjectData[childNodeObjectIndex]));
		curObjectData->SetOctreeNodeIndex((u16)(knParentOctreeNodeIndex));
		curObjectData->SetVisitingObjectChildIndex(knChildIndex);

		const int nextChildNodeObjectIndex = curObjectData->m_uNextObjectInNodeIndex;
		const bool isLastObjectInNode = (nextChildNodeObjectIndex == phInst::INVALID_INDEX);
		if(isLastObjectInNode)
		{
			curObjectData->m_uNextObjectInNodeIndex = pParentOctreeNode->m_ContainedObjectStartIdx;
			SPU_ONLY(objectDataBuffer.ReturnObject());
			break;
		}

		SPU_ONLY(objectDataBuffer.ReturnObject());
		childNodeObjectIndex = nextChildNodeObjectIndex;
	}

	// Append the parent node's object list to the object list of the node that got collapsed and make that the new parent node object list.
	pParentOctreeNode->m_ContainedObjectStartIdx = firstChildNodeObjectIndex;
	pParentOctreeNode->m_ContainedObjectCount = (u8)((int)pParentOctreeNode->m_ContainedObjectCount + numObjectsInChildNode);

	pParentOctreeNode->DisconnectChild(knChildIndex);
	SPU_ONLY(parentOctreeNodeBuffer.ReturnObject());

	pChildOctreeNode->DisconnectParent();
	pChildOctreeNode->ClearAllVisitingObjectCounts();
	pChildOctreeNode->m_ContainedObjectCount = 0;
	pChildOctreeNode->m_ContainedObjectStartIdx = phInst::INVALID_INDEX;

	PHLEVELNEW_ASSERT(IsObjectCountCorrect(pParentOctreeNode, -1, NULL, 2));
	PHLEVELNEW_ASSERT(IsObjectCountCorrect(pChildOctreeNode, -1, NULL, 3));

	ReturnOctreeNodeIndexToPool((u16)(childOctreeNodeIndex));
}

#if !__SPU
// m_NumActive should be updated *after* this function is called.
template <class __NodeType> void phLevelNodeTree<__NodeType>::RemoveActiveObjectUserData(const int knLevelIndex)
{
	m_pauObjectUserData[knLevelIndex] = NULL;
	int nActiveObjectIndex;
	for(nActiveObjectIndex = m_NumActive-1; nActiveObjectIndex >= 0 && m_pauActiveObjectLevelIndex[nActiveObjectIndex] != knLevelIndex; --nActiveObjectIndex);

	// I do not think the 'nActiveObjectIndex >= 0' should ever be a terminating condition
	// If it were, nActiveObjectIndex would be -1 and we would stomp the memory just before the array
	if( Unlikely(nActiveObjectIndex < 0) )
	{
		// TODO: Switch to FastAssert whenever that or some equivalent resumes working in Bankrelease
		__debugbreak();
	}

	m_pauActiveObjectLevelIndex[nActiveObjectIndex] = m_pauActiveObjectLevelIndex[m_NumActive-1];
}
#endif // !__SPU


#if !LEVELNEW_SPU_CODE
template <class __NodeType> /*inline */u16 phLevelNodeTree<__NodeType>::GetFirstCulledNode(phIterator& it) const
#else
template <class __NodeType> /*inline */u16 phLevelNodeTree<__NodeType>::GetFirstCulledNode(phIterator& it, __NodeType &outSpuNode) const
#endif
{
#if !LEVELNEW_SPU_CODE
	const __NodeType *rootNode = GetConstOctreeNode(0);
#else
	__NodeType *rootNode = &outSpuNode;
	TransferData(rootNode, GetConstOctreeNode(0), sizeof(__NodeType));
#endif
	if(CheckNodeAgainstCullObject(*rootNode, it))
	{
		it.m_CurDepth = 0;
		it.m_NodeIndices[0] = 0;

#if !LEVELNEW_SPU_CODE
		return (rootNode->m_ContainedObjectCount != 0) ? 0 : GetNextCulledNode(it);
#else
		return (rootNode->m_ContainedObjectCount != 0) ? 0 : GetNextCulledNode(it, outSpuNode);
#endif
	}

#if LEVELNEW_USE_EVERYWHERE_NODE
	// If there's an 'everywhere node' and there are objects in it, we need to look through that node before we're done.
	if(m_uFirstObjectInEverywhereNodeIndex != (u16)(-1))
	{
		it.m_CurDepth = 0;
		it.m_NodeIndices[0] = PHLEVELNEW_EVERYWHERE_NODE_INDEX;
		return PHLEVELNEW_EVERYWHERE_NODE_INDEX;
	}
#endif

	it.m_CurDepth = -1;
	return (u16)(-1);
}


#if !LEVELNEW_SPU_CODE
template <class __NodeType> /*inline */u16 phLevelNodeTree<__NodeType>::GetNextCulledNode(phIterator& it) const
#else
template <class __NodeType> /*inline */u16 phLevelNodeTree<__NodeType>::GetNextCulledNode(phIterator& it, __NodeType &outSpuNode) const
#endif
{
#if LEVELNEW_USE_EVERYWHERE_NODE
	if(it.m_NodeIndices[it.m_CurDepth] == PHLEVELNEW_EVERYWHERE_NODE_INDEX)
	{
		Assert(it.m_CurDepth == 0);
		return (u16)(-1);
	}
#endif

#if 1
	u16 uCurOctreeNodeIndex = it.m_NodeIndices[it.m_CurDepth];
#if !LEVELNEW_SPU_CODE
	const __NodeType *pCurOctreeNode = GetConstOctreeNode(uCurOctreeNodeIndex);
#else
	__NodeType *pCurOctreeNode = reinterpret_cast<__NodeType *>(&outSpuNode);
	TransferData(pCurOctreeNode, GetConstOctreeNode(uCurOctreeNodeIndex), sizeof(__NodeType));
//	SPU_PRINTF("GetNextCulledNode(): DMA'd last node %d, has %d objects.\n", uCurOctreeNodeIndex, pCurOctreeNode->m_ContainedObjectCount);
#endif

	int nChildIndex = 0;
	while(true)
	{
		// The top of the stack currently has the last valid node found.  Now we're going to try to walk down the tree (through valid nodes)
		//   until we either find a valid node with objects or we reach a dead-end.
		if(pCurOctreeNode->m_ChildNodeCount > 0)
		{
StartNodeWithChildren:
			while(nChildIndex < __NodeType::GetBranchingFactor())
			{
				PHLEVELNEW_ASSERT(pCurOctreeNode->m_ChildNodeCount > 0);
				u16 uNextOctreeNodeIndex = pCurOctreeNode->m_ChildNodeIndices[nChildIndex];
				if(uNextOctreeNodeIndex == (u16)(-1))
				{
					++nChildIndex;
					continue;
				}

#if !__SPU && !EMULATE_SPU
				const __NodeType *pNextOctreeNode = GetConstOctreeNode(uNextOctreeNodeIndex);
#else
				u8 nextNodeBuffer[sizeof(__NodeType)];
				__NodeType *pNextOctreeNode = reinterpret_cast<__NodeType *>(nextNodeBuffer);
				TransferData(pNextOctreeNode, GetConstOctreeNode(uNextOctreeNodeIndex), sizeof(__NodeType));
//				SPU_PRINTF("GetNextCulledNode():Found child node %d: has %d objects.\n", uNextOctreeNodeIndex, pNextOctreeNode->m_ContainedObjectCount);
#endif
				if(!CheckNodeAgainstCullObject(*pNextOctreeNode, it))
				{
#if __SPU || EMULATE_SPU
//					SPU_PRINTF("GetNextCulledNode(): Node %u failed cull test..\n", uNextOctreeNodeIndex);
#endif
					++nChildIndex;
					continue;
				}

#if __SPU || EMULATE_SPU
//				SPU_PRINTF("GetNextCulledNode(): Node %u passed cull test.  It has %u objects in it.\n", uNextOctreeNodeIndex, pNextOctreeNode->m_ContainedObjectCount);
#endif

				it.m_ChildIndices[it.m_CurDepth] = (u8)(nChildIndex);
				++it.m_CurDepth;
				it.m_NodeIndices[it.m_CurDepth] = (u16)(uNextOctreeNodeIndex);

#if __SPU
				// If we're on the SPU we can't just copy pointers, we actually need to do a deep copy of the data.
				sysMemCpy(pCurOctreeNode, pNextOctreeNode, sizeof(__NodeType));
#endif

				if(pNextOctreeNode->m_ContainedObjectCount == 0)
				{
					// This is a valid octree node, but it doesn't have any objects in it, so let's not return it.
					nChildIndex = 0;
#if !__SPU && !EMULATE_SPU
					pCurOctreeNode = pNextOctreeNode;
#endif
					goto StartNodeWithChildren;
				}

				return uNextOctreeNodeIndex;
			}
		}

		// If we get here, we've reached a dead-end.  Let's crawl up the stack one spot and try again from there.
/*		--rnDepth;
		if(rnDepth != -1)
		{
			uCurOctreeNodeIndex = it.m_NodeIndices[rnDepth];
			pCurOctreeNode = (const __NodeType *)(&m_paOctreeNodes[uCurOctreeNodeIndex]);
			nChildIndex = pauChildIndex[rnDepth] + 1;
		}*/
		do
		{
			--it.m_CurDepth;
			if(it.m_CurDepth == -1)
			{
#if LEVELNEW_USE_EVERYWHERE_NODE
				if(m_uFirstObjectInEverywhereNodeIndex == (u16)(-1))
				{
					return m_uFirstObjectInEverywhereNodeIndex;
					//return (u16)(-1);
				}

				it.m_CurDepth = 0;
				it.m_NodeIndices[0] = PHLEVELNEW_EVERYWHERE_NODE_INDEX;
				return PHLEVELNEW_EVERYWHERE_NODE_INDEX;
#else
				return (u16)(-1);
#endif
			}
			uCurOctreeNodeIndex = it.m_NodeIndices[it.m_CurDepth];
			nChildIndex = it.m_ChildIndices[it.m_CurDepth] + 1;
		}
		while(nChildIndex == __NodeType::GetBranchingFactor());
#if !__SPU && !EMULATE_SPU
		pCurOctreeNode = (const __NodeType *)(&m_paOctreeNodes[uCurOctreeNodeIndex]);
#else
		TransferData(pCurOctreeNode, GetConstOctreeNode(uCurOctreeNodeIndex), sizeof(__NodeType));
//		SPU_PRINTF("GetNextCulledNode(): Node %d: %d objects.\n", uCurOctreeNodeIndex, pCurOctreeNode->m_ContainedObjectCount);
#endif
	}

//	Assert(rnDepth == -1);
//	return (u16)(-1);
#else
	u16 uCurOctreeNodeIndex = it.m_NodeIndices[it.m_CurDepth];
	const __NodeType *pCurOctreeNode = GetConstOctreeNode(uCurOctreeNodeIndex);

	if(pCurOctreeNode->m_uChildNodeCount > 0)
	{
StartNodeWithChildren:
		// He's got a child, so let's find and check his children.
		int nChildIndex;
		for(nChildIndex = 0; nChildIndex < __NodeType::GetBranchingFactor(); ++nChildIndex)
		{
			u16 uNextOctreeNodeIndex = pCurOctreeNode->m_auChildNodeIndices[nChildIndex];
			if(uNextOctreeNodeIndex == (u16)(-1))
			{
				continue;
			}

			const __NodeType *pNextOctreeNode = GetConstOctreeNode(uNextOctreeNodeIndex);
			if(!it.CheckNode(pNextOctreeNode))
			{
				continue;
			}

			pauChildIndex[it.m_CurDepth] = (u8)(nChildIndex);
			++it.m_CurDepth;
			it.m_NodeIndices[it.m_CurDepth] = (u16)(uNextOctreeNodeIndex);

			if(pNextOctreeNode->m_ContainedObjectCount == 0)
			{
				// This is a valid octree node, but it doesn't have any objects in it, so let's not return it.
				pCurOctreeNode = pNextOctreeNode;
				goto StartNodeWithChildren;
			}

			return uNextOctreeNodeIndex;
		}
	}

	// He doesn't have any (valid) children, so let's unwind the stack, trying to find a node with a child that we haven't yet visited.
	--it.m_CurDepth;
	while(it.m_CurDepth >= 0)
	{
		pCurOctreeNode = GetConstOctreeNode(it.m_NodeIndices[it.m_CurDepth]);

		// Let's pick up at the child index where we left off when we were here before.
		int nChildIndex = pauChildIndex[it.m_CurDepth] + 1;
		while(nChildIndex < __NodeType::GetBranchingFactor())
		{
			u16 uNextOctreeNodeIndex = pCurOctreeNode->m_auChildNodeIndices[nChildIndex];
			if(uNextOctreeNodeIndex == (u16)(-1))
			{
				++nChildIndex;
				continue;
			}

			const __NodeType *pNextOctreeNode = GetConstOctreeNode(uNextOctreeNodeIndex);
			if(!it.CheckNode(pNextOctreeNode))
			{
				++nChildIndex;
				continue;
			}

			pauChildIndex[it.m_CurDepth] = (u8)(nChildIndex);
			++it.m_CurDepth;
			it.m_NodeIndices[it.m_CurDepth] = (u16)(uNextOctreeNodeIndex);

			if(pNextOctreeNode->m_ContainedObjectCount == 0)
			{
				// This is a valid octree node, but it doesn't have any objects in it, so let's not return it.
				nChildIndex = 0;
				pCurOctreeNode = pNextOctreeNode;
				continue;
			}

			return uNextOctreeNodeIndex;
		}
		--it.m_CurDepth;
	}

	Assert(it.m_CurDepth == -1);
	return (u16)(-1);
#endif
}


template <class __NodeType> bool phLevelNodeTree<__NodeType>::CachedObjectDataCheckInstance(const phIterator &iter, int levelIndex) const
{
	// This whole function was adapted from Iterator::CheckInstance(). This version
	// function doesn't use phInst, phArchetype, or phBound. This is very
	// much intentional, as cache performance greatly increases by not having to
	// access data in multiple places. /FF

    Assert(iter.GetCullType() != phCullShape::PHCULLTYPE_UNSPECIFIED);

	FastAssert(LegitLevelIndex(levelIndex));

#if !__SPU
	const phObjectData &d = m_paObjectData[levelIndex];
#else
	const phObjectData &d = *iter.GetCurObjectData();
#endif
	const u32 stateFlag = GetBitFromExistentState(d.GetState());

	// For proper operation, these asserts should not fail if turned on.
	// If they do, the user may need to call UpdateObjectArchetype().
	// However, please be very careful about enabling these asserts,
	// as there will probably be a big performance penalty due to accessing
	// the archetype's memory. /FF
	//ASSERT_ONLY(const phInst *pCurInst = GetInstance(levelIndex));
	//Assert(pCurInst->GetArchetype()->GetTypeFlags() == d.m_CachedArchTypeFlags);
	//Assert(pCurInst->GetArchetype()->GetIncludeFlags() == d.m_CachedArchIncludeFlags);

	if((stateFlag & iter.m_StateIncludeFlags) == 0
			|| !MatchFlags(iter.m_TypeFlags, iter.m_IncludeFlags, d.m_CachedArchTypeFlags, d.m_CachedArchIncludeFlags)
			|| (d.m_CachedArchTypeFlags & iter.m_TypeExcludeFlags) != 0
			)
	{
		// Note: I have seen some load-hit-stores coming from here, at least in
		// the original Iterator::CheckInstance() function. I think this happens
		// from registers being saved on the stack when entering the function, then
		// when quickly exiting it again if the flags didn't match, it reads the
		// data back. It might be possible to improve this by rearranging the
		// code a little. /FF

		return false;
	}

	// Instead of computing the instance's bounding sphere here:
	//	const Vector3 sphereCenter = inst->GetArchetype()->GetBound()->GetCenter((*(const Matrix34*)(&inst->GetMatrix())));
	//	const Vector3 sphereRadius = inst->GetArchetype()->GetBound()->GetRadiusAroundCentroidV();
	// we use the precomputed data. Note that this is primarily done to reduce
	// L2 cache misses, the reduced math is a bonus. /FF
	Vec4V sphereCenter = d.m_CachedCenterAndRadius;
	ScalarV sphereRadius = SplatW( sphereCenter );

	// TODO: I think we should consider refactoring to get this switch out of here.
	// It would be very nice to be able to loop over a number of objects and
	// not have to re-read the same data from the iterator on each iteration,
	// the values (sphere center or whatever) would just be kept in registers. /FF

	return iter.m_CullShape.CheckSphere(sphereCenter.GetXYZ(), sphereRadius);
}


template <class __NodeType> inline u16 phLevelNodeTree<__NodeType>::GetNextCulledObjectFromNode(phIterator& it) const
{
	phObjectData * paObjectData = m_paObjectData;

	Assert(LegitLevelIndex(it.m_LastObjectIndex));
#if !__SPU && !EMULATE_SPU
	const phObjectData *pCurObjectData = &paObjectData[it.m_LastObjectIndex];
	const phInst *pCurInst = pCurObjectData->GetInstance();
#else
	// Grab the phObjectData that corresponds to the last instance we looked at.
	const phObjectData *pCurObjectData = it.GetCurObjectData();
//	SPU_PRINTF("GetNextCulledObjectFromNode(): DMA'd object data %u.\n", it.m_LastObjectIndex);
//	SPU_PRINTF("GetNextCulledObjectFromNode(): Node is %u, Next object is %u.\n", pCurObjectData->GetOctreeNodeIndex(), pCurObjectData->m_uNextObjectInNodeIndex);
#endif
	u16 uNextObjectLevelIndex = pCurObjectData->m_uNextObjectInNodeIndex;
	while(uNextObjectLevelIndex != phInst::INVALID_INDEX)
	{
#if !__SPU && !EMULATE_SPU
		pCurObjectData = &paObjectData[uNextObjectLevelIndex];
		pCurInst = pCurObjectData->GetInstance();

		u16 uNextNextObjectLevelIndex = pCurObjectData->m_uNextObjectInNodeIndex;
		if (uNextNextObjectLevelIndex != phInst::INVALID_INDEX)
		{
			PrefetchDC(&paObjectData[uNextNextObjectLevelIndex]);
		}

#else // !__SPU && !EMULATE_SPU
		// Make our already-queued-up object data the current one.  It's probably already finished DMAing so that's good.
		it.PushNextObjectDataToCurrent();
		pCurObjectData = it.GetCurObjectData();

		// Start the DMA on the next object data.
		const u16 nextObjectInNodeIndex = pCurObjectData->m_uNextObjectInNodeIndex;
		if(nextObjectInNodeIndex != (u16)(-1))
		{
			it.FetchNextObjectData(&paObjectData[nextObjectInNodeIndex]);
		}
//		SPU_PRINTF("GetNextCulledObjectFromNode(): DMA'd object data %u.\n", uNextObjectLevelIndex);
//		SPU_PRINTF("GetNextCulledObjectFromNode(): Node is %u, Next object is %u.\n", pCurObjectData->GetOctreeNodeIndex(), pCurObjectData->m_uNextObjectInNodeIndex);

		// Now that we've gotten the phObjectData, let's kick off the DMA for the phInst that goes with it.
		it.InitiateInstanceDMA(pCurObjectData->GetInstance());
#endif // !__SPU && !EMULATE_SPU

		bool instCheckPassed;

		if (it.GetCullAgainstInstanceAABBs())
		{
#if __SPU || EMULATE_SPU
			// In this case we have get all of this data now in order to even proceed.
			const phInst *pCurInst = reinterpret_cast<const phInst *>(it.GetInstanceBuffer());
			it.WaitCompleteInstanceDMA();
			//		SPU_PRINTF("GetNextCulledObjectFromNode(): DMA'd instance with level index %u.\n", pCurInst->GetLevelIndex());

			// Now that we've gotten the phInst, we need to get it's archetype.
			it.InitiateArchetypeDMA(pCurInst->GetArchetype());
			const phArchetypeDamp *pArchetype = reinterpret_cast<const phArchetypeDamp *>(it.GetArchetypeBuffer());

			it.WaitCompleteArchetypeDMA();
			//		SPU_PRINTF("GetNextCulledObjectFromNode(): DMA'd archetype with name %s.\n", pArchetype->GetFilename());

			it.InitiateBoundDMA(pArchetype->GetBound());
			const phBound *pBound = reinterpret_cast<const phBound *>(it.GetBoundBuffer());
			it.WaitCompleteBoundDMA();
			//		SPU_PRINTF("GetNextCulledObjectFromNode(): DMA'd bound with radius %f.\n", pBound->GetRadiusAroundCentroid());

			const Vector3 sphereCenter(VEC3V_TO_VECTOR3(pBound->GetWorldCentroid(pCurInst->GetMatrix())));
			const Vector3 sphereRadius = SCALARV_TO_VECTOR3(pBound->GetRadiusAroundCentroidV());
#endif // __SPU || EMULATE_SPU

			if(pCurObjectData->m_uNextObjectInNodeIndex != phInst::INVALID_INDEX)
			{
				PrefetchDC(paObjectData[pCurObjectData->m_uNextObjectInNodeIndex].GetInstance());
			}

#if !__SPU && !EMULATE_SPU
			instCheckPassed = it.CheckInstance(pCurInst, pCurObjectData->m_CachedArchTypeFlags, pCurObjectData->m_CachedArchIncludeFlags, GetBitFromExistentState(GetState(uNextObjectLevelIndex)));
#else
			instCheckPassed = it.CheckInstance(sphereCenter, sphereRadius, pCurObjectData->m_CachedArchTypeFlags, pCurObjectData->m_CachedArchIncludeFlags, GetBitFromExistentState(pCurObjectData->GetState()));
#endif
		}
		else
		{
			instCheckPassed = CachedObjectDataCheckInstance(it, uNextObjectLevelIndex);
		}

		if (instCheckPassed)
		{
#if __SPU || EMULATE_SPU
//			Displayf("GetNextCulledObjectFromNode(): Instance %u was accepted.", uNextObjectLevelIndex);
#endif
			return uNextObjectLevelIndex;
		}

#if __SPU || EMULATE_SPU
//		Displayf("GetNextCulledObjectFromNode(): Instance %u was rejected.", uNextObjectLevelIndex);
		uNextObjectLevelIndex = pCurObjectData->m_uNextObjectInNodeIndex;
#else
		uNextObjectLevelIndex = uNextNextObjectLevelIndex;
#endif
	}

	return phInst::INVALID_INDEX;
}


#if !__SPU && !EMULATE_SPU
template <class __NodeType> inline u16 phLevelNodeTree<__NodeType>::GetFirstCulledObjectFromNode(phIterator& it, const int knCurOctreeNodeIndex) const
#else
template <class __NodeType> inline u16 phLevelNodeTree<__NodeType>::GetFirstCulledObjectFromNode(phIterator& it, const int knCurOctreeNodeIndex, const __NodeType *pCurOctreeNode) const
#endif
{
#if LEVELNEW_USE_EVERYWHERE_NODE
	if(knCurOctreeNodeIndex != PHLEVELNEW_EVERYWHERE_NODE_INDEX)
	{
#endif

#if !__SPU && !EMULATE_SPU
		const __NodeType *pCurOctreeNode = GetConstOctreeNode(knCurOctreeNodeIndex);
#endif

		// This assert is what allows us to avoid having to check if m_ContainedObjectStartIdx is phInst::INVALID_INDEX.  This function should never be called
		//   on an octree node that doesn't have any objects in it.
		PHLEVELNEW_ASSERT(pCurOctreeNode->m_ContainedObjectCount > 0);

		it.m_LastObjectIndex = pCurOctreeNode->m_ContainedObjectStartIdx;
		PHLEVELNEW_ASSERT(pCurOctreeNode->m_ContainedObjectStartIdx < m_MaxObjects);

#if LEVELNEW_USE_EVERYWHERE_NODE
	}
	else
	{
		// TODO: Change this to a PHLEVELNEW_ASSERT.
		Assert(m_uFirstObjectInEverywhereNodeIndex != (u16)(-1));
		it.m_LastObjectIndex = m_uFirstObjectInEverywhereNodeIndex;
	}
#endif

	// Get the phObjectData.
#if !__SPU && !EMULATE_SPU
	const phObjectData *pCurObjectData = &m_paObjectData[it.m_LastObjectIndex];
#else
	// Grab the phObjectData that corresponds to this instance.
	it.FetchCurObjectData(&m_paObjectData[it.m_LastObjectIndex]);
	const phObjectData *pCurObjectData = it.GetCurObjectData();
//	SPU_PRINTF("GetFirstCulledObjectFromNode(): DMA'd object data %u.\n", it.m_LastObjectIndex);
//	SPU_PRINTF("GetFirstCulledObjectFromNode(): Node is %u, Next object is %u.\n", pCurObjectData->GetOctreeNodeIndex(), pCurObjectData->m_uNextObjectInNodeIndex);

	// Start the DMA on the next object data.
	const int nextObjectInNodeIndex = pCurObjectData->m_uNextObjectInNodeIndex;
	if(nextObjectInNodeIndex != (u16)(-1))
	{
		it.FetchNextObjectData(&m_paObjectData[nextObjectInNodeIndex]);
	}

	// Now that we've gotten the phObjectData, let's kick off the DMA for the phInst that goes with it.
	it.InitiateInstanceDMA(pCurObjectData->GetInstance());
#endif // !__SPU && !EMULATE_SPU

	// Prefetch the next phObjectData. The phInst isn't accessed in the probably relatively common case of it being
	// rejected, so we don't want to bring it into the cache too early. /FF
	if(pCurObjectData->m_uNextObjectInNodeIndex != phInst::INVALID_INDEX)
	{
		PrefetchDC(&m_paObjectData[pCurObjectData->m_uNextObjectInNodeIndex]);
	}

	bool instCheckPassed;

	if (it.GetCullAgainstInstanceAABBs())
	{
#if !__SPU && !EMULATE_SPU
		const phInst *pCurInst = pCurObjectData->GetInstance();

		if(pCurObjectData->m_uNextObjectInNodeIndex != phInst::INVALID_INDEX)
		{
			PrefetchDC(m_paObjectData[pCurObjectData->m_uNextObjectInNodeIndex].GetInstance());
		}
		instCheckPassed = it.CheckInstance(pCurInst, pCurObjectData->m_CachedArchTypeFlags, pCurObjectData->m_CachedArchIncludeFlags, GetBitFromExistentState(GetState(it.m_LastObjectIndex)));
#else
		// In this case (culling against AABBs) we have get all of this data now in order to even proceed.
		const phInst *pCurInst = reinterpret_cast<const phInst *>(it.GetInstanceBuffer());
		it.WaitCompleteInstanceDMA();
		//	SPU_PRINTF("GetFirstCulledObjectFromNode(): DMA'd instance with level index %u.\n", pCurInst->GetLevelIndex());

		// Now that we've gotten the phInst, we need to get it's archetype.
		it.InitiateArchetypeDMA(pCurInst->GetArchetype());
		const phArchetypeDamp *pArchetype = reinterpret_cast<const phArchetypeDamp *>(it.GetArchetypeBuffer());
		it.WaitCompleteArchetypeDMA();
		//	SPU_PRINTF("GetFirstCulledObjectFromNode(): DMA'd archetype with name %s.\n", pArchetype->GetFilename());
		it.InitiateBoundDMA(pArchetype->GetBound());
		const phBound *pBound = reinterpret_cast<const phBound *>(it.GetBoundBuffer());
		it.WaitCompleteBoundDMA();
		//	SPU_PRINTF("GetFirstCulledObjectFromNode(): DMA'd bound with radius %f.\n", pBound->GetRadiusAroundCentroid());

		// TODO: It looks like this code has suffered some rot.  This isn't even the right CheckInstance() call to be making for this case and the sphere
		//   center and radius are calculated here and that's completely unnecessary - the correct CheckInstance() function doesn't want them and even *if*
		//   it did, we could just have fetched them from the object data.
#if 1
		// Code that I think is incorrect.
		const Vector3 sphereCenter(VEC3V_TO_VECTOR3(pBound->GetWorldCentroid(pCurInst->GetMatrix())));
		const Vector3 sphereRadius = SCALARV_TO_VECTOR3(pBound->GetRadiusAroundCentroidV());

		instCheckPassed = it.CheckInstance(sphereCenter, sphereRadius, pCurObjectData->m_CachedArchTypeFlags, pCurObjectData->m_CachedArchIncludeFlags, GetBitFromExistentState(pCurObjectData->GetState()));
#else
		// What I think it should be (note that this is the same as above so they should probably just be combined outside of the #if/#else/#endif block):
		instCheckPassed = it.CheckInstance(pCurInst, pCurObjectData->m_CachedArchTypeFlags, pCurObjectData->m_CachedArchIncludeFlags, GetBitFromExistentState(GetState(it.m_LastObjectIndex)));
#endif
#endif
	}
	else
	{
		instCheckPassed = CachedObjectDataCheckInstance(it, it.m_LastObjectIndex);
	}

	if (instCheckPassed)
	{
#if __SPU || EMULATE_SPU
//		Displayf("GetFirstCulledObjectFromNode(): Instance %u was accepted.", it.m_LastObjectIndex);
#endif

		return it.m_LastObjectIndex;
	}

#if __SPU || EMULATE_SPU
//	Displayf("GetFirstCulledObjectFromNode(): Instance %u was rejected.", it.m_LastObjectIndex);
#endif

	return GetNextCulledObjectFromNode(it);
}


template <class __NodeType> /*inline */u16 phLevelNodeTree<__NodeType>::GetFirstCulledObject(phIterator& it) const
{
#if !__SPU && !EMULATE_SPU
	u16 uCurOctreeNodeIndex = GetFirstCulledNode(it);
#else
	u8 nodeBuffer[sizeof(__NodeType)];
	__NodeType *curOctreeNode = reinterpret_cast<__NodeType *>(nodeBuffer);
	u16 uCurOctreeNodeIndex = GetFirstCulledNode(it, *curOctreeNode);
#endif
	while(uCurOctreeNodeIndex != (u16)(-1))
	{
#if !__SPU && !EMULATE_SPU
		PHLEVELNEW_ASSERT((LEVELNEW_USE_EVERYWHERE_NODE && uCurOctreeNodeIndex == PHLEVELNEW_EVERYWHERE_NODE_INDEX) || GetConstOctreeNode(uCurOctreeNodeIndex)->m_ContainedObjectCount > 0);
		u16 uCurObjectLevelIndex = GetFirstCulledObjectFromNode(it, uCurOctreeNodeIndex);
#else
		PHLEVELNEW_ASSERT((LEVELNEW_USE_EVERYWHERE_NODE && uCurOctreeNodeIndex == PHLEVELNEW_EVERYWHERE_NODE_INDEX) || curOctreeNode->m_ContainedObjectCount > 0);
		u16 uCurObjectLevelIndex = GetFirstCulledObjectFromNode(it, uCurOctreeNodeIndex, curOctreeNode);
#endif
		if(uCurObjectLevelIndex != phInst::INVALID_INDEX)
		{
			it.m_LastObjectIndex = uCurObjectLevelIndex;
			return uCurObjectLevelIndex;
		}

#if !__SPU && !EMULATE_SPU
		uCurOctreeNodeIndex = GetNextCulledNode(it);
#else
		uCurOctreeNodeIndex = GetNextCulledNode(it, *curOctreeNode);
#endif
	}

	return phInst::INVALID_INDEX;
}


template <class __NodeType> /*inline */u16 phLevelNodeTree<__NodeType>::GetNextCulledObject(phIterator& it) const
{
	// Let's try getting another object from the last node that we found.
	u16 uCurObjectLevelIndex = GetNextCulledObjectFromNode(it);
	if(uCurObjectLevelIndex != phInst::INVALID_INDEX)
	{
		it.m_LastObjectIndex = uCurObjectLevelIndex;
		return uCurObjectLevelIndex;
	}

	// There were no more valid objects in that last node, so let's look for a new node.
#if !__SPU && !EMULATE_SPU
	u16 uCurOctreeNodeIndex = GetNextCulledNode(it);
#else
	u8 curNodeBuffer[sizeof(__NodeType)];
	__NodeType *pCurOctreeNode = reinterpret_cast<__NodeType *>(curNodeBuffer);
///	const int uInitialOctreeNodeIndex = it.m_NodeIndices[it.m_CurDepth];
///	TransferData(pCurOctreeNode, GetConstOctreeNode(uInitialOctreeNodeIndex), sizeof(__NodeType));
//	SPU_PRINTF("phLevelNodeTree::GetNextCulledObject() : Looking node %d, found %d objects.\n", uInitialOctreeNodeIndex, pCurOctreeNode->m_ContainedObjectCount);

	u16 uCurOctreeNodeIndex = GetNextCulledNode(it, *pCurOctreeNode);
#endif
	while(uCurOctreeNodeIndex != (u16)(-1))
	{
#if !__SPU && !EMULATE_SPU
		PHLEVELNEW_ASSERT((LEVELNEW_USE_EVERYWHERE_NODE && uCurOctreeNodeIndex == PHLEVELNEW_EVERYWHERE_NODE_INDEX) || GetConstOctreeNode(uCurOctreeNodeIndex)->m_ContainedObjectCount > 0);
		uCurObjectLevelIndex = GetFirstCulledObjectFromNode(it, uCurOctreeNodeIndex);
#else
		PHLEVELNEW_ASSERT((LEVELNEW_USE_EVERYWHERE_NODE && uCurOctreeNodeIndex == PHLEVELNEW_EVERYWHERE_NODE_INDEX) || pCurOctreeNode->m_ContainedObjectCount > 0);
		uCurObjectLevelIndex = GetFirstCulledObjectFromNode(it, uCurOctreeNodeIndex, pCurOctreeNode);
#endif
		if(uCurObjectLevelIndex != phInst::INVALID_INDEX)
		{
			it.m_LastObjectIndex = uCurObjectLevelIndex;
			return uCurObjectLevelIndex;
		}

		// That node didn't have any valid objects, so let's look for a new node.
#if !__SPU && !EMULATE_SPU
		uCurOctreeNodeIndex = GetNextCulledNode(it);
#else
		uCurOctreeNodeIndex = GetNextCulledNode(it, *pCurOctreeNode);
#endif
	}

	// No more objects are left.
	return phInst::INVALID_INDEX;
}


template <class __NodeType> phInst* phLevelNodeTree<__NodeType>::GetFirstCulledInstance (phIterator& iterator) const
{
	u16 levelIndex = GetFirstCulledObject(iterator);
	return levelIndex != phInst::INVALID_INDEX ? m_paObjectData[levelIndex].GetInstance() : NULL;
}


template <class __NodeType> phInst* phLevelNodeTree<__NodeType>::GetNextCulledInstance (phIterator& iterator) const
{
	u16 levelIndex = GetNextCulledObject(iterator);
	return levelIndex != phInst::INVALID_INDEX ? m_paObjectData[levelIndex].GetInstance() : NULL;
}

// TODO - Would kind of like to suck this inside the phCullShape class like we were able to do with CheckSphere and CheckAABB /RP
template <class __NodeType> bool phLevelNodeTree<__NodeType>::CheckNodeAgainstCullObject(const __NodeType& octreeNode, const phIterator& it) const
{
	Assert(it.GetCullType() != phCullShape::PHCULLTYPE_UNSPECIFIED);

	switch(it.GetCullType())
	{
	case phCullShape::PHCULLTYPE_SPHERE:
		{
			const phCullShape::phCullData_Sphere* data = &it.m_CullShape.GetSphereData();
			//				return octreeNode->DoesSphereIntersect(data->m_Center, data->m_fRadius);
			return octreeNode.DoesSphereIntersectSpecial(data->m_Center, data->m_Radius);
		}
	case phCullShape::PHCULLTYPE_LINESEGMENT:
		{
			const phCullShape::phCullData_LineSegment* data = &it.m_CullShape.GetLineSegmentData();
			bool bRetVal = octreeNode.DoesLineSegmentIntersect(data->m_P0, data->m_P1);
#if __SPU || EMULATE_SPU
			if(!bRetVal)
			{
//				SPU_PRINTF("CheckNodeAgainstCullObject(): Failed (2).\n");
			}
#endif
			return bRetVal;
		}
	case phCullShape::PHCULLTYPE_XZCIRCLE:
		{
			const phCullShape::phCullData_XZCircle* data = &it.m_CullShape.GetXZCircleData();
			return octreeNode.DoesXZCircleIntersect(data->m_Center, data->m_Radius.Getf());
		}
	case phCullShape::PHCULLTYPE_CAPSULE:
		{
			const phCullShape::phCullData_Capsule* data = &it.m_CullShape.GetCapsuleData();
			return octreeNode.DoesCapsuleIntersect(data->m_P0, data->m_ShaftAxis, data->m_ShaftLength, data->m_Radius);
		}
	case phCullShape::PHCULLTYPE_BOX:
		{
			const phCullShape::phCullData_Box* data = &it.m_CullShape.GetBoxData();
			return octreeNode.DoesBoxIntersect(data->m_BoxAxes, data->m_BoxHalfSize);
		}
	case phCullShape::PHCULLTYPE_POINT:
		{
			const phCullShape::phCullData_Point* data = &it.m_CullShape.GetPointData();
			return octreeNode.IsPointWithinNode(data->m_vecPoint);
		}
	case phCullShape::PHCULLTYPE_AABB:
		{
			const phCullShape::phCullData_AABB* data = &it.m_CullShape.GetAABBData();
			return octreeNode.DoesAABBIntersect(data->m_BoxCenter, data->m_BoxHalfSize);
		}
	default:
		{
			Assert(it.GetCullType() == phCullShape::PHCULLTYPE_ALL);
			return true;
		}
	}
}


template <class __NodeType> u16 phLevelNodeTree<__NodeType>::CullObjects(phIterator& it, u16 *paLevelIndices) const
{
	Assert(paLevelIndices != NULL);
	Assert(it.GetCullType() != phCullShape::PHCULLTYPE_UNSPECIFIED);
	u16 uNumCulled = 0;

	u16 uCurObjectLevelIndex = GetFirstCulledObject(it);
	while(uCurObjectLevelIndex != phInst::INVALID_INDEX)
	{
		paLevelIndices[uNumCulled] = uCurObjectLevelIndex;
		++uNumCulled;
		uCurObjectLevelIndex = GetNextCulledObject(it);
	}
	return uNumCulled;
}


#if !__SPU

template <class __NodeType> int phLevelNodeTree<__NodeType>::TestProbe (const phSegment& krsegProbe, phIntersection* pIsect, const phInst* pExcludeInstance, u32 uIncludeFlags, u32 uTypeFlags,
							u8 uStateIncludeFlags, int numIntersections, u32 typeExcludeFlags/*=TYPE_FLAGS_NONE*/) const
{
	Assert(numIntersections > 0);
	phShapeTest<phShapeProbe> probeTester;
	probeTester.InitProbe(krsegProbe,pIsect,numIntersections);
	return probeTester.TestInLevel(pExcludeInstance,uIncludeFlags,uTypeFlags,uStateIncludeFlags,typeExcludeFlags,this);
}


template <class __NodeType> int phLevelNodeTree<__NodeType>::TestEdge (const phSegment& segment, phIntersection* isect0, phIntersection* UNUSED_PARAM(isect1), const phInst* excludeInstance,
							u32 includeFlags, u32 typeFlags, u8 stateIncludeFlags, int numIsectPairs, u32 typeExcludeFlags/*=TYPE_FLAGS_NONE*/) const
{
	// Make local arrays to so that the bound culler in the shape test doesn't allocate them.
	phShapeTest<phShapeEdge> edgeTester;
	edgeTester.InitEdge(segment,isect0,numIsectPairs);
	return edgeTester.TestInLevel(excludeInstance,includeFlags,typeFlags,stateIncludeFlags,typeExcludeFlags,this);
}


template <class __NodeType> bool phLevelNodeTree<__NodeType>::TestProbeBatch (int nNumSegs, const phSegment** seg, const Vector3 &vecSegsCenter, float fSegsRadius, const Matrix34 &mtxSegsBox, phIntersection** isect,
									const phInst *pInstExclude, u32 uIncludeFlags, u32 uTypeFlags, u8 uStateIncludeFlags, u32 typeExcludeFlags/*=TYPE_FLAGS_NONE*/) const
{
	// just to compile
	fSegsRadius += 0.0f*mtxSegsBox.a.x+vecSegsCenter.x;

	// Set a maximum of 128 batched probes.
	ASSERT_ONLY(const int maxBatchedProbes = 128;)
	AssertMsg(nNumSegs<=maxBatchedProbes, "Increase maxBatchedProbes, it can go up to 1024 (MAX_BATCHED_SHAPES)");

	// Create a shape tester with batched tests. The spaces inside the phShapeTest declaration are necessary for PS3 builds (maybe it's confused by >>).
	phShapeTest<phShapeBatch> batchTester;

	//int memoryReq = nNumSegs * sizeof(phShapeProbe);
	//if (memoryReq < 100 * 1024)	// Don't allocate too much on the stack.
	{
		if (nNumSegs > 0)
		{
			phShapeProbe *probes = Alloca(phShapeProbe, nNumSegs);
			for (int x=0; x<nNumSegs; x++)
			{
				::new(&probes[x]) phShapeProbe();
			}

			batchTester.GetShape().SetProbes(probes, nNumSegs);

			// NOTE: We'll never call the destructor, but currently phShapeProbe objects
			// have an empty destructor (so we'd spend a lot of cycles calling that
			// function every frame, it's not inlined). If that ever changes, we should call the
			// destructor after the test.
		}
	}
	/*else
	{
		batchTester.GetShape().AllocateProbes(nNumSegs);
	}*/

	for (int probeIndex=0; probeIndex<nNumSegs; probeIndex++)
	{
		batchTester.InitProbe(*seg[probeIndex],isect[probeIndex]);
	}

	return (batchTester.TestInLevel(pInstExclude,uIncludeFlags,uTypeFlags,uStateIncludeFlags,typeExcludeFlags,this)>0);
}


template <class __NodeType> int phLevelNodeTree<__NodeType>::TestCapsule (Vector3::Vector3Param krvecP0, Vector3::Vector3Param krvecP1, float kfRadius, phIntersection* pIsect,
								const phInst* pInstExclude, u32 uIncludeFlags, u32 uTypeFlags, u8 uStateIncludeFlags, bool bDirected, int numIntersections,
								u32 typeExcludeFlags/*=TYPE_FLAGS_NONE*/) const
{
	phSegment segment;
	segment.Set(krvecP0,krvecP1);
	if (bDirected)
	{
		phShapeTest<phShapeSweptSphere> sweptSphereTester;
		sweptSphereTester.InitSweptSphere(segment,kfRadius,pIsect,numIntersections);
		return sweptSphereTester.TestInLevel(pInstExclude,uIncludeFlags,uTypeFlags,uStateIncludeFlags,typeExcludeFlags,this);
	}
	else
	{
		phShapeTest<phShapeCapsule> capsuleTester;
		capsuleTester.InitCapsule(segment,kfRadius,pIsect,numIntersections);
		return capsuleTester.TestInLevel(pInstExclude,uIncludeFlags,uTypeFlags,uStateIncludeFlags,typeExcludeFlags,this);
	}
}


template <class __NodeType> int phLevelNodeTree<__NodeType>::TestCapsule (Vector3::Vector3Param axisStart, Vector3::Vector3Param axis, float ASSERT_ONLY(shaftLength), float radius, phIntersection* pIsect,
								const phInst *pExcludeInstance, u32 uIncludeFlags, u32 uTypeFlags, u8 uStateIncludeFlags, bool bDirected, int numIntersections,
								u32 typeExcludeFlags/*=TYPE_FLAGS_NONE*/) const
{
	// This cast is necessary only because, on optimized Xbox 360 builds, Vector3::Vector3Param is actually a __vector4, and doesn't define a Mag2() function.
	Assert(Vector3(axis).Mag2() >= square(0.999f * shaftLength));
	Assert(Vector3(axis).Mag2() <= square(1.001f * shaftLength));
	phSegment segment;
	Vector3 segmentEnd(axisStart);
	segmentEnd.Add(axis);
	segment.Set(axisStart,segmentEnd);
	if (bDirected)
	{
		phShapeTest<phShapeSweptSphere> sweptSphereTester;
		sweptSphereTester.InitSweptSphere(segment,radius,pIsect,numIntersections);
		return sweptSphereTester.TestInLevel(pExcludeInstance,uIncludeFlags,uTypeFlags,uStateIncludeFlags,typeExcludeFlags,this);
	}
	else
	{
		phShapeTest<phShapeCapsule> capsuleTester;
		capsuleTester.InitCapsule(segment,radius,pIsect,numIntersections);
		return capsuleTester.TestInLevel(pExcludeInstance,uIncludeFlags,uTypeFlags,uStateIncludeFlags,typeExcludeFlags,this);
	}

}


template <class __NodeType> int phLevelNodeTree<__NodeType>::TestSweptSphere (const Vector3& endA, const Vector3& endB, float radius, phIntersection* isect, const phInst* excludeInstance,
									u32 includeFlags, u32 typeFlags, u8 includeStateFlags, int numIntersections, u32 excludeTypeFlags/*=TYPE_FLAGS_NONE*/) const
{
	return TestCapsule(endA,endB,radius,isect,excludeInstance,includeFlags,typeFlags,includeStateFlags,true,numIntersections, excludeTypeFlags);
}


template <class __NodeType> int phLevelNodeTree<__NodeType>::TestPoint (const Vector3& krvecPoint, phIntersection* pIsect, const phInst* pInstExclude, u32 uIncludeFlags, u32 uTypeFlags, u8 uStateIncludeFlags,
							int numIntersections, u32 typeExcludeFlags/*=TYPE_FLAGS_NONE*/) const
{
	phShapeTest<phShapeSphere> pointTester;
	const float radius = 0.0f;
	pointTester.InitSphere(krvecPoint,radius,pIsect,numIntersections);
	return pointTester.TestInLevel(pInstExclude,uIncludeFlags,uTypeFlags,uStateIncludeFlags,typeExcludeFlags,this);
}


template <class __NodeType> int phLevelNodeTree<__NodeType>::TestSphere (const Vector3& krvecCenter, const float kfRadius, phIntersection* pIsect, const phInst* pExcludeInstance, u32 uIncludeFlags, u32 uTypeFlags,
								u8 uStateIncludeFlags, int numIntersections, u32 typeExcludeFlags/*=TYPE_FLAGS_NONE*/) const
{
	phShapeTest<phShapeSphere> sphereTester;
	sphereTester.InitSphere(krvecCenter,kfRadius,pIsect,numIntersections);
	return sphereTester.TestInLevel(pExcludeInstance,uIncludeFlags,uTypeFlags,uStateIncludeFlags,typeExcludeFlags,this);
}


#if !__SPU
template <class __NodeType> int phLevelNodeTree<__NodeType>::TestObject (phInst* instance, phIntersection* intersection, const phInst* excludeInstance, u32 includeFlags, u32 typeFlags, u8 stateIncludeFlags,
								int numIntersections, u32 typeExcludeFlags) const
{
	phShapeTest<phShapeObject> objectTester;
	objectTester.InitObject(*instance->GetArchetype()->GetBound(),(*(const Matrix34*)(&instance->GetMatrix())),intersection,numIntersections);
	//objectTester.SetExcludeInstanceList (phInst** instanceList, int numInstances);
	return objectTester.TestInLevel(excludeInstance,includeFlags,typeFlags,stateIncludeFlags,typeExcludeFlags,this);
}
#endif // !__SPU


#if PHLEVEL_DEBUG_CONSISTENCY_CHECKS != 0
template <class __NodeType> void phLevelNodeTree<__NodeType>::DoConsistencyChecks(u16 uExcludeIndex, bool bRadiusIsGood) const
{
	float fMaxSubRadius; int nNodesVisited; u32 uStateFlags; int nObjectsFound;

	CheckNode(0, -1, fMaxSubRadius, nNodesVisited, uStateFlags, nObjectsFound, uExcludeIndex, bRadiusIsGood);
	Assert(nNodesVisited == m_uOctreeNodesInUse);
	Assert(nObjectsFound == m_NumObjects);
}
#else
template <class __NodeType> void phLevelNodeTree<__NodeType>::DoConsistencyChecks(u16 UNUSED_PARAM(uExcludeIndex), bool UNUSED_PARAM(bRadiusIsGood)) const
{
}
#endif

#if LEVELNEW_EXTRACONSISTENCYCHECKS || (PHLEVEL_DEBUG_CONSISTENCY_CHECKS != 0)
template <class __NodeType> void phLevelNodeTree<__NodeType>::CheckNode(const int knStartingOctreeNodeIndex, const int knParentOctreeNodeIndex, float &rfMaxSubRadius, int &rnNodesVisited, u32 &ruStateFlags, int &rnObjectsFound, u16 uExcludeIndex, bool bRadiusIsGood) const
{
	rfMaxSubRadius = 0.0f;
	rnNodesVisited = 1;
	ruStateFlags = 0;
	rnObjectsFound = 0;

	const __NodeType *pCurOctreeNode = GetConstOctreeNode(knStartingOctreeNodeIndex);
	Assert(pCurOctreeNode->m_ParentNodeIndex == (u16)(knParentOctreeNodeIndex));

	if((u16)(knParentOctreeNodeIndex) != (u16)(-1))
	{
		Assert(GetConstOctreeNode(knParentOctreeNodeIndex)->IsPointWithinNode(pCurOctreeNode->m_Center.GetIntrin128()));
		Assert(pCurOctreeNode->m_HalfWidth == 0.5f * GetConstOctreeNode(knParentOctreeNodeIndex)->m_HalfWidth);
	}

	// First, we check all of the objects in this node, and
	// 1) ensure that they all are properly in this node,
	// 2) find the maximum radius across all of the objects,
	int anVisitingObjectChildIndices[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	for(u16 uLevelIndex = pCurOctreeNode->m_ContainedObjectStartIdx; uLevelIndex != phInst::INVALID_INDEX; uLevelIndex = m_paObjectData[uLevelIndex].m_uNextObjectInNodeIndex)
	{
		const phObjectData *pCurObjectData = &m_paObjectData[uLevelIndex];
		Assert(pCurObjectData->GetOctreeNodeIndex() == knStartingOctreeNodeIndex);
		const phInst *pInst = pCurObjectData->GetInstance();
#if __ASSERT
		Vector3 vecCenter;
		pInst->GetArchetype()->GetBound()->GetCenter((*(const Matrix34*)(&pInst->GetMatrix())), RC_VEC3V(vecCenter));
#endif // __ASSERT
		float fRadius(pInst->GetArchetype()->GetBound()->GetRadiusAroundCentroid());

		Assert(pCurObjectData->GetInstance()->GetLevelIndex() == uLevelIndex);

		Assert((uLevelIndex == uExcludeIndex && !bRadiusIsGood) || fRadius <= pCurOctreeNode->m_HalfWidth || knStartingOctreeNodeIndex == 0);
		Assert(uLevelIndex == uExcludeIndex || pCurOctreeNode->IsPointWithinNode(vecCenter));
		ASSERT_ONLY(int nChildIndex = pCurOctreeNode->SubClassifyPoint(RC_VEC3V(vecCenter)));
		Assert(uLevelIndex == uExcludeIndex || (fRadius > 0.5f * pCurOctreeNode->m_HalfWidth) || (pCurObjectData->GetVisitingObjectChildIndex() == nChildIndex));
		Assert(uLevelIndex == uExcludeIndex || (fRadius > 0.5f * pCurOctreeNode->m_HalfWidth) || (pCurOctreeNode->m_ChildNodeIndices[nChildIndex] == (u16)(-1)));

		Assert((pCurOctreeNode->m_SubNodeStateFlags & GetBitFromExistentState(GetState(uLevelIndex))) != 0);

		if(uExcludeIndex == phInst::INVALID_INDEX || bRadiusIsGood)
		{
			if(fRadius <= 0.5f * pCurOctreeNode->m_HalfWidth)
			{
				const int knVisitingObjectChildIndex = pCurObjectData->GetVisitingObjectChildIndex();
				++anVisitingObjectChildIndices[knVisitingObjectChildIndex];
				Assert(anVisitingObjectChildIndices[knVisitingObjectChildIndex] <= pCurOctreeNode->GetVisitingObjectCount(knVisitingObjectChildIndex));
			}

			rfMaxSubRadius = Max(rfMaxSubRadius, fRadius);
		}

		ruStateFlags |= GetBitFromExistentState(GetState(uLevelIndex));

		++rnObjectsFound;
	}

	Assert(pCurOctreeNode->m_ContainedObjectCount == rnObjectsFound);

	if(uExcludeIndex == phInst::INVALID_INDEX)
	{
		// This means that the visiting object data that we calculated above should be exactly correct.
		for(int nChildIndex = 0; nChildIndex < __NodeType::GetBranchingFactor(); ++nChildIndex)
		{
			Assert(pCurOctreeNode->GetVisitingObjectCount(nChildIndex) == anVisitingObjectChildIndices[nChildIndex]);
		}
	}
	else if(bRadiusIsGood)
	{
		// The objects may not be in the right places, but their sizes are correct.  So while the distribution of visiting objects among the children may be different
		//   than we computed, the totals should be correct.
		Assert(pCurOctreeNode->GetVisitingObjectCount(0) + pCurOctreeNode->GetVisitingObjectCount(1) + pCurOctreeNode->GetVisitingObjectCount(2) + pCurOctreeNode->GetVisitingObjectCount(3)
		 + pCurOctreeNode->GetVisitingObjectCount(4) +  + pCurOctreeNode->GetVisitingObjectCount(5)  + pCurOctreeNode->GetVisitingObjectCount(6) +  + pCurOctreeNode->GetVisitingObjectCount(7) == 
		 anVisitingObjectChildIndices[0] + anVisitingObjectChildIndices[1] + anVisitingObjectChildIndices[2] + anVisitingObjectChildIndices[3] + anVisitingObjectChildIndices[4]
		 + anVisitingObjectChildIndices[5] + anVisitingObjectChildIndices[6] + anVisitingObjectChildIndices[7]);
	}

	for(int nChildIndex = 0; nChildIndex < __NodeType::GetBranchingFactor(); ++nChildIndex)
	{
		int nChildOctreeNodeIndex = pCurOctreeNode->m_ChildNodeIndices[nChildIndex];
		if(nChildOctreeNodeIndex == (u16)(-1))
		{
			continue;
		}

		float fTempMaxSubRadius; int nTempNodesVisited;	u32 uStateFlags; int nObjectsFound;
		CheckNode(nChildOctreeNodeIndex, knStartingOctreeNodeIndex ,fTempMaxSubRadius, nTempNodesVisited, uStateFlags, nObjectsFound, uExcludeIndex, bRadiusIsGood);
		rnObjectsFound += nObjectsFound;

		if(uExcludeIndex == phInst::INVALID_INDEX || bRadiusIsGood)
		{
			rfMaxSubRadius = Max(rfMaxSubRadius, fTempMaxSubRadius);
		}
		rnNodesVisited += nTempNodesVisited;
		ruStateFlags |= uStateFlags;
	}

	Assert(pCurOctreeNode->m_SubNodeStateFlags == ruStateFlags);
}


template <class __NodeType> void phLevelNodeTree<__NodeType>::CheckForDuplicateObject(const phInst *pInst) const
{
	u16 uObjectIndex;
	for(uObjectIndex = 0; uObjectIndex < m_MaxObjects; ++uObjectIndex)
	{
		Assert(IsNonexistent(uObjectIndex) || m_paObjectData[uObjectIndex].GetInstance() != pInst);
	}
}
#else
template <class __NodeType> void phLevelNodeTree<__NodeType>::CheckNode(const int UNUSED_PARAM(knStartingOctreeNodeIndex), const int UNUSED_PARAM(knParentOctreeNodeIndex), float &UNUSED_PARAM(rfMaxSubRadius), int &UNUSED_PARAM(rnNodesVisited), u32 &UNUSED_PARAM(ruStateFlags), int &UNUSED_PARAM(rnObjectsFound), u16 UNUSED_PARAM(uExcludeIndex), bool UNUSED_PARAM(bRadiusIsGood)) const
{
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::CheckForDuplicateObject(const phInst *UNUSED_PARAM(pInst)) const
{
}
#endif


#if __BANK

template <class __NodeType> void phLevelNodeTree<__NodeType>::AddWidgets(bkBank &bank)
{
	if (GetActiveInstance())
	{
		GetActiveInstance()->AddLevelWidgets(bank);
	}
	else
	{
		phLevelBase::AddWidgets(bank);
	}
}

#if !__SPU
template <class __NodeType> void phLevelNodeTree<__NodeType>::AddLevelWidgets(bkBank &bank)
{
	phLevelBase::AddWidgets(bank);
	bank.AddButton("Dump Objects", datCallback(MFA(phLevelNodeTree::DumpObjects), this));
	bank.AddSlider("Max Octree Nodes Ever Used", &sm_MaxOctreeNodesEverUsed, 0, 1000000, 0);
	bank.AddSlider("Max Active Objects Ever", &sm_MaxActiveObjectsEver, 0, 1000000, 0);
	bank.AddSlider("Max Objects Ever", &sm_MaxObjectsEver, 0, 1000000, 0);
	bank.AddSlider("Total objects added", &sm_TotalObjectsAdded, 0, 1000000, 0);
	bank.AddSlider("Total objects removed", &sm_TotalObjectsDeleted, 0, 1000000, 0);
	bank.AddToggle("Disable inactive vs inactive", &sm_DisableInactiveCollidesAgainstInactive);
	bank.AddToggle("Disable inactive vs fixed", &sm_DisableInactiveCollidesAgainstFixed);
}

template <class __NodeType> void phLevelNodeTree<__NodeType>::DumpObjects()
{
	Displayf("==== Begin phLevel object dump =====");
	Displayf("phLevel POI Center: %f %f %f", m_DebugPOICenter.GetXf(), m_DebugPOICenter.GetYf(),  m_DebugPOICenter.GetZf());
	Displayf("Level Index,Radius,Archetype,X,Y,Z");
	for(int levelIndex = 0; levelIndex < m_MaxObjects; ++levelIndex)
	{
		if(IsNonexistent(levelIndex))
		{
			continue;
		}
		const phInst *curInst = GetInstance(levelIndex);
		Assert(curInst != NULL);
		const phArchetype *curArchetype = curInst->GetArchetype();
		Assert(curArchetype != NULL);
		Assert(curArchetype->GetBound());
		Vec3V loc = curInst->GetPosition();
		Vec3V deltaV = loc - m_DebugPOICenter;
		ScalarV distV = Mag(deltaV);
		float dist = distV.Getf();
		Displayf("%4d,%.2f,%s,%.2f,%.2f,%.2f,%.2f", levelIndex, curArchetype->GetBound()->GetRadiusAroundCentroid(), curArchetype->GetFilename(), loc.GetXf(), loc.GetYf(), loc.GetZf(), dist);
	}
	Displayf("==== End phLevel object dump =====");
}
template <class __NodeType> void phLevelNodeTree<__NodeType>::DumpNode(__NodeType& node) const
{
	Displayf("Dumping Level Node:");
	Displayf("\tPosition: %5.2f, %5.2f, %5.2f",VEC3V_ARGS(node.m_CenterXYZHalfWidthW.GetXYZ()));
	Displayf("\tRadius: %f",node.m_CenterXYZHalfWidthW.GetZf());
	Displayf("\tNum Active Child Nodes: %i",node.m_ChildNodeCount);
	int maxNumChildNodes = sizeof(node.m_ChildNodeIndices)/sizeof(node.m_ChildNodeIndices[0]);
	for(int childNodeIndex = 0; childNodeIndex < maxNumChildNodes; ++childNodeIndex)
	{
		if(node.m_ChildNodeIndices[childNodeIndex] != (u16)-1)
		{
			Displayf("\t\tActive Child Node - %i objects in child node",GetConstOctreeNode(node.m_ChildNodeIndices[childNodeIndex])->m_ContainedObjectCount);
		}
		else
		{
			Displayf("\t\tInactive Child Node - %i visiting objects",node.GetVisitingObjectCount(childNodeIndex));
		}
	}

	Displayf("\tNum Objects: %i",node.m_ContainedObjectCount);
	int nextObjectIndex = node.m_ContainedObjectStartIdx;
	while(nextObjectIndex != phInst::INVALID_INDEX)
	{
		const phObjectData &objectData = m_paObjectData[nextObjectIndex];
		const phInst* instance = objectData.GetInstance();
		const phArchetype* archtype = instance->GetArchetype();
		const phBound* bound = archtype->GetBound();
		Displayf("\t\tName: '%s'",archtype->GetFilename());
		Displayf("\t\t\tLevel Index: %i",instance->GetLevelIndex());
		switch(objectData.GetState())
		{
		case OBJECTSTATE_ACTIVE: Displayf("\t\t\tState: Active"); break;
		case OBJECTSTATE_INACTIVE: Displayf("\t\t\tState: Inactive"); break;
		case OBJECTSTATE_NONEXISTENT: Displayf("\t\t\tState: Fixed"); break;
		default: Displayf("\t\t\tState: ???"); break;
		}
		Displayf("\t\t\tVisiting Child Index: %i",objectData.GetVisitingObjectChildIndex());
		Displayf("\t\t\tPosition: Actual(%5.2f, %5.2f, %5.2f) Cached(%5.2f, %5.2f, %5.2f)", VEC3V_ARGS(instance->GetWorldCentroid()), VEC3V_ARGS(objectData.GetCenterAndRadius().GetXYZ()));
		Displayf("\t\t\tRadius: Actual(%f) Cached(%f)", bound->GetRadiusAroundCentroid(), objectData.GetCenterAndRadius().GetWf());
		nextObjectIndex = m_paObjectData[nextObjectIndex].m_uNextObjectInNodeIndex;
	}
}
#endif

#endif // __BANK

template <class __NodeType> void phLevelNodeTree<__NodeType>::InitObjectData(phObjectData &objectDataToInit)
{
	const phInst *inst = objectDataToInit.GetInstance();
	Assert(inst);

	const phArchetype &arch = *inst->GetArchetype();
	const phBound *bnd = arch.GetBound();
	const Vec3V center = bnd->GetWorldCentroid(inst->GetMatrix());
	const ScalarV fRadius = bnd->GetRadiusAroundCentroidV();

	VecBoolV v_maskw		= VecBoolV(V_F_F_F_T);
	Vec4V centerAndRadius	= SelectFT( v_maskw, Vec4V(center), Vec4V(fRadius) );

	objectDataToInit.m_CachedCenterAndRadius = centerAndRadius;
	objectDataToInit.m_CachedArchIncludeFlags = arch.GetIncludeFlags();
	objectDataToInit.m_CachedArchTypeFlags = arch.GetTypeFlags();
}

#endif // !__SPU

// This needs to be instantiated after all member functions have been defined for RSG_ORBIS, since it doesn't defer template instantiations
// by default.
template class phLevelNodeTree<phLooseOctreeNode>;

} // namespace rage 
