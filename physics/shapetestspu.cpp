//
// physics/shapetestspu.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#if __SPU

#include "vector/vector3_consts_spu.cpp"

#include "shapetestspu.h"

#include "shapetest.h"

#include "cullshape.cpp"
#include "iterator.cpp"
#include "levelbase.cpp"
#include "levelnew.cpp"
#include "shapetest.cpp"

#include "math/polynomial.cpp"
#include "phbound/bound.cpp"
#include "phbound/boundbox.cpp"
#include "phbound/boundbvh.cpp"
#include "phbound/boundcapsule.cpp"
#include "phbound/boundcurvedgeom.cpp"
#include "phbound/boundcylinder.cpp"
#include "phbound/boundgrid.cpp"
#include "phbound/primitives.cpp"
#include "phCore/constants.h"
#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
#include "phBound/boundGeomSecondsurface.cpp"
#endif
#include "phbound/boundpolyhedron.cpp"
#include "phbound/boundsphere.cpp"
#include "phbound/boundtaperedcapsule.cpp"
#include "phbullet/IterativeCast.cpp"
#include "phbullet/VoronoiDegenerateSimplexSolver.cpp"
#include "phbullet/VoronoiSingleSimplexSolver.cpp"
#include "phcore/frameallocator.h"
#include "vector/geometry.cpp"
#include "vector/matrix34.cpp"

// If the file that included this file didn't explicitly support a shape type, disable that shape's code

#ifndef SHAPETESTSPU_SUPPORT_PROBES
#define SHAPETESTSPU_SUPPORT_PROBES 0
#endif // SHAPETESTSPU_SUPPORT_PROBES

#ifndef SHAPETESTSPU_SUPPORT_EDGES
#define SHAPETESTSPU_SUPPORT_EDGES 0
#endif // SHAPETESTSPU_SUPPORT_EDGES

#ifndef SHAPETESTSPU_SUPPORT_SPHERES
#define SHAPETESTSPU_SUPPORT_SPHERES 0
#endif // SHAPETESTSPU_SUPPORT_SPHERES

#ifndef SHAPETESTSPU_SUPPORT_CAPSULES
#define SHAPETESTSPU_SUPPORT_CAPSULES 0
#endif // SHAPETESTSPU_SUPPORT_CAPSULES

#ifndef SHAPETESTSPU_SUPPORT_SWEPTSPHERES
#define SHAPETESTSPU_SUPPORT_SWEPTSPHERES 0
#endif // SHAPETESTSPU_SUPPORT_SWEPTSPHERES

#ifndef SHAPETESTSPU_SUPPORT_TAPEREDSWEPTSPHERES
#define SHAPETESTSPU_SUPPORT_TAPEREDSWEPTSPHERES 0
#endif // SHAPETESTSPU_SUPPORT_TAPEREDSWEPTSPHERES

#ifndef SHAPETESTSPU_SUPPORT_BOXES
#define SHAPETESTSPU_SUPPORT_BOXES 0
#endif // SHAPETESTSPU_SUPPORT_BOXES

#ifndef SHAPETESTSPU_SUPPORT_OBJECTS
#define SHAPETESTSPU_SUPPORT_OBJECTS 0
#endif // SHAPETESTSPU_SUPPORT_OBJECTS

#ifndef SHAPETESTSPU_SUPPORT_BATCHES
#define SHAPETESTSPU_SUPPORT_BATCHES 0
#endif // SHAPETESTSPU_SUPPORT_BATCHES

// At least one shape type should be enabled
CompileTimeAssert(SHAPETESTSPU_SUPPORT_PROBES || 
				  SHAPETESTSPU_SUPPORT_EDGES || 
				  SHAPETESTSPU_SUPPORT_SPHERES || 
				  SHAPETESTSPU_SUPPORT_CAPSULES || 
				  SHAPETESTSPU_SUPPORT_SWEPTSPHERES || 
				  SHAPETESTSPU_SUPPORT_TAPEREDSWEPTSPHERES || 
				  SHAPETESTSPU_SUPPORT_BOXES || 
				  SHAPETESTSPU_SUPPORT_OBJECTS || 
				  SHAPETESTSPU_SUPPORT_BATCHES);

// These shapetests aren't supported yet on SPU
CompileTimeAssert(SHAPETESTSPU_SUPPORT_BOXES == 0);		//  Box shapetests are not implemented yet
CompileTimeAssert(SHAPETESTSPU_SUPPORT_OBJECTS == 0);	//  Object and batch shapetests are more complex to DMA over
CompileTimeAssert(SHAPETESTSPU_SUPPORT_BATCHES == 0);

#if SHAPETESTSPU_SUPPORT_CAPSULES || SHAPETESTSPU_SUPPORT_SPHERES || SHAPETESTSPU_SUPPORT_SWEPTSPHERES
#include "phbullet/ConvexIntersector.cpp"
#include "phbullet/GjkPairDetector.cpp"
#include "phbullet/TrianglePenetrationDepthSolver.cpp"
#include "phbullet/MinkowskiPenetrationDepthSolver.cpp"
#include "phbullet/VoronoiSimplexSolver.cpp"
#include "vector/vector3.cpp"
#endif // SHAPETESTSPU_SUPPORT_CAPSULES || SHAPETESTSPU_SUPPORT_SPHERES

namespace rage {

bool ArrayAssertFailed(int line) { return false; }
bool GetGJKCacheSeparatingDirection(const DiscreteCollisionDetectorInterface::ClosestPointInput & input, Vec3V_InOut sepAxis, ScalarV_InOut sepDist) 
{
	// ShapeTests don't use the GJK cache. This function is only here to prevent a linker error.
	(void)input;
	(void)sepAxis;
	(void)sepDist;
	return false;
}

} // namespace rage

using namespace rage;

template <class ShapeType>
void ShapeTestSpuHelper(phShapeTest<ShapeType> *ppuShapeTests, int numShapeTests, FrameAllocator<16> &scratchAllocator, phLevelNew *physicsLevel, phIterator& levelIterator)
{
	if (numShapeTests > 0)
	{
		scratchAllocator.SetMarker();
		phShapeTest<ShapeType> *spuCurShapeTest = reinterpret_cast<phShapeTest<ShapeType> *>(scratchAllocator.GetBlock(sizeof(phShapeTest<ShapeType>)));
		for (int shapeTestIndex=0; shapeTestIndex<numShapeTests; shapeTestIndex++)
		{
			levelIterator.ResetCull();
			// We need to get a hold of each of the phShapeTest's and we'll do that by DMAing them into our scratch memory.
			cellDmaLargeGet(spuCurShapeTest, (uint64_t)(&ppuShapeTests[shapeTestIndex]), sizeof(phShapeTest<ShapeType>), DMA_TAG(16), 0, 0);
			// This wait may also be waiting on the physics level DMA above.
			cellDmaWaitTagStatusAll(DMA_MASK(16));

			// There is a pointer in the phShapeTest that we also need to DMA over.  The *catch*, however, is that we have to save off the PPU address
			//   so that we can DMA it back once we're done.
			const int kNumIsects = spuCurShapeTest->GetShape().m_MaxNumIsects != -1 ? spuCurShapeTest->GetShape().m_MaxNumIsects : 1;
			phIntersection *ppuIsect = spuCurShapeTest->GetShape().m_Intersection;
			phIntersection *spuIsect = NULL;
			scratchAllocator.SetMarker();
			if(kNumIsects > 0)
			{
				spuIsect = reinterpret_cast<phIntersection *>(scratchAllocator.GetBlock(sizeof(phIntersection) * kNumIsects));
				spuCurShapeTest->GetShape().m_Intersection = spuIsect;
			}

			const phInst * pExcludeInst = spuCurShapeTest->GetExcludeInstance();
			const u32 iIncludeFlags = spuCurShapeTest->GetIncludeFlags();
			const u32 iTypeFlags = spuCurShapeTest->GetTypeFlags();
			const u32 iStateIncludeFlags = spuCurShapeTest->GetStateIncludeFlags();
			const u32 iTypeExcludeFlags = spuCurShapeTest->GetTypeExcludeFlags();
#if ENABLE_PHYSICS_LOCK
			spuCurShapeTest->TestInLevel(pExcludeInst, levelIterator, iIncludeFlags, iTypeFlags, iStateIncludeFlags, iTypeExcludeFlags, physicsLevel);
#else	// ENABLE_PHYSICS_LOCK
			spuCurShapeTest->TestInLevel(pExcludeInst, iIncludeFlags, iTypeFlags, iStateIncludeFlags, iTypeExcludeFlags, physicsLevel);
#endif	// ENABLE_PHYSICS_LOCK

			// DMA back any intersection results.  Maybe we don't need to bother doing this is we didn't find any hits?
			if(kNumIsects)
			{
				cellDmaLargePut(spuIsect, (uint64_t)(ppuIsect), sizeof(phIntersection) * kNumIsects, DMA_TAG(16), 0, 0);
			}

			spuCurShapeTest->GetShape().m_Intersection = ppuIsect;
			// We only DMA back phShapeBase because the only thing that's really going to have changed are the members in there (is that right?)
			cellDmaLargePut(spuCurShapeTest, (uint64_t)(&ppuShapeTests[shapeTestIndex]), sizeof(phShapeBase), DMA_TAG(16), 0, 0);
#if PROFILE_INDIVIDUAL_SHAPETESTS
			float* spuCompletionTime = &spuCurShapeTest->m_CompletionTime;
			float* ppuCompletionTime = &ppuShapeTests[shapeTestIndex].m_CompletionTime;
			cellDmaPut((float*)((uint64_t)spuCompletionTime & 0xFFFFFFF0), (uint64_t)ppuCompletionTime & 0xFFFFFFF0, 16, DMA_TAG(16), 0, 0);
#endif // PROFILE_INDIVIDUAL_SHAPETESTS
			cellDmaWaitTagStatusAll(DMA_MASK(16));

			scratchAllocator.ReleaseToLastMarker();
		}
		scratchAllocator.ReleaseToLastMarker();
	}

}

void shapetestspu (::rage::sysTaskParameters& taskParams)
{
//	spu_printf("\n*** shapetestspu!!! ***\n");
	phShapeTestTaskData& taskData = *static_cast<phShapeTestTaskData*>(taskParams.Input.Data);

#if ENABLE_PHYSICS_LOCK
	phMultiReaderLockToken spuPhysicsLock;
	RageCellSyncMutex *ppuModifyReaderCountMutex = (RageCellSyncMutex *)taskParams.UserData[phShapeTestTaskData::ParamIndex_ModifyReaderCountMutex].asPtr;
	spuPhysicsLock.SetPPUModifyReaderCountMutexPtr(ppuModifyReaderCountMutex);
	u32 *ppuReaderCount = (u32 *)taskParams.UserData[phShapeTestTaskData::ParamIndex_GlobalReaderCount].asPtr;
	spuPhysicsLock.SetPPUReaderCountPtr(ppuReaderCount);
	RageCellSyncMutex *ppuPhysicsLock = (RageCellSyncMutex *)taskParams.UserData[phShapeTestTaskData::ParamIndex_PhysicsMutex].asPtr;
	spuPhysicsLock.SetPPUPhysicsMutexPtr(ppuPhysicsLock);
#		if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	RageCellSyncMutex *ppuAllowNewReaderLock = (RageCellSyncMutex *)taskParams.UserData[phShapeTestTaskData::ParamIndex_AllowNewReaderMutex].asPtr;
	spuPhysicsLock.SetPPUAllowNewReaderMutexPtr(ppuAllowNewReaderLock);
#		endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
#endif

//	spu_printf("Scratch location is %u.\n", (int)(taskParams.Scratch.Data));
	FrameAllocator<16> scratchAllocator((u8 *)(taskParams.Scratch.Data), taskParams.Scratch.Size);
	scratchAllocator.SetMarker();

	// Grab the level iterator before DMAing the level, some of the information on that class changes when it's being written to.
	phIterator levelIterator(phIterator::PHITERATORLOCKTYPE_READLOCK,&spuPhysicsLock);

	phLevelNew *physicsLevel = reinterpret_cast<phLevelNew *>(scratchAllocator.GetBlock(sizeof(phLevelNew)));
	// We do this DMA on the same tag as the one for shape tests themselves so that we can just use that same wait (we don't need it before then).
	cellDmaLargeGet(physicsLevel, (uint64_t)(taskParams.UserData[phShapeTestTaskData::ParamIndex_PhysicsLevel].asPtr), sizeof(phLevelNew), DMA_TAG(16), 0, 0);

#if SHAPETESTSPU_SUPPORT_PROBES
	Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_ProbeShapeTest,"Multithreaded probe shape test needs more task data. Current %i, Required %i", (int)taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_ProbeShapeTest + 1);
	phShapeTest<phShapeProbe>* probeShapeTests = reinterpret_cast<phShapeTest<phShapeProbe> *>(taskParams.UserData[phShapeTestTaskData::ParamIndex_ProbeShapeTest].asPtr);
	ShapeTestSpuHelper(probeShapeTests, taskData.m_NumProbes, scratchAllocator, physicsLevel, levelIterator);
#else // SHAPETESTSPU_SUPPORT_PROBES
	Assertf(taskData.m_NumProbes == 0, "Shapetest task that doesn't support probes has probes on the task data.");
#endif // SHAPETESTSPU_SUPPORT_PROBES

#if SHAPETESTSPU_SUPPORT_EDGES
	Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_EdgeShapeTest,"Multithreaded edge shape test needs more task data. Current %i, Required %i", (int)taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_EdgeShapeTest + 1);
	phShapeTest<phShapeEdge>* edgeShapeTests = reinterpret_cast<phShapeTest<phShapeEdge> *>(taskParams.UserData[phShapeTestTaskData::ParamIndex_EdgeShapeTest].asPtr);
	ShapeTestSpuHelper(edgeShapeTests, taskData.m_NumEdges, scratchAllocator, physicsLevel, levelIterator);
#else // SHAPETESTSPU_SUPPORT_EDGES
	Assertf(taskData.m_NumEdges == 0, "Shapetest task that doesn't support edges has edges on the task data.");
#endif // SHAPETESTSPU_SUPPORT_EDGES

#if SHAPETESTSPU_SUPPORT_SPHERES
	Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_SphereShapeTest,"Multithreaded sphere shape test needs more task data. Current %i, Required %i", (int)taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_SphereShapeTest + 1);
	phShapeTest<phShapeSphere>* sphereShapeTests = reinterpret_cast<phShapeTest<phShapeSphere> *>(taskParams.UserData[phShapeTestTaskData::ParamIndex_SphereShapeTest].asPtr);
	ShapeTestSpuHelper(sphereShapeTests, taskData.m_NumSpheres, scratchAllocator, physicsLevel, levelIterator);
#else // SHAPETESTSPU_SUPPORT_SPHERES
	Assertf(taskData.m_NumSpheres == 0, "Shapetest task that doesn't support spheres has spheres on the task data.");
#endif // SHAPETESTSPU_SUPPORT_SPHERES

#if SHAPETESTSPU_SUPPORT_CAPSULES
	Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_CapsuleShapeTest,"Multithreaded capsule shape test needs more task data. Current %i, Required %i", (int)taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_CapsuleShapeTest + 1);
	phShapeTest<phShapeCapsule>* capsuleShapeTests = reinterpret_cast<phShapeTest<phShapeCapsule> *>(taskParams.UserData[phShapeTestTaskData::ParamIndex_CapsuleShapeTest].asPtr);
	ShapeTestSpuHelper(capsuleShapeTests, taskData.m_NumCapsules, scratchAllocator, physicsLevel, levelIterator);
#else // SHAPETESTSPU_SUPPORT_CAPSULES
	Assertf(taskData.m_NumCapsules == 0, "Shapetest task that doesn't support capsules has capsules on the task data.");
#endif // SHAPETESTSPU_SUPPORT_CAPSULES

#if SHAPETESTSPU_SUPPORT_SWEPTSPHERES
	Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_SweptSphereShapeTest,"Multithreaded swept sphere shape test needs more task data. Current %i, Required %i", (int)taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_SweptSphereShapeTest + 1);
	phShapeTest<phShapeSweptSphere>* sweptSphereShapeTests = reinterpret_cast<phShapeTest<phShapeSweptSphere> *>(taskParams.UserData[phShapeTestTaskData::ParamIndex_SweptSphereShapeTest].asPtr);
	ShapeTestSpuHelper(sweptSphereShapeTests, taskData.m_NumSweptSpheres, scratchAllocator, physicsLevel, levelIterator);
#else // SHAPETESTSPU_SUPPORT_SWEPTSPHERES
	Assertf(taskData.m_NumSweptSpheres == 0, "Shapetest task that doesn't support swept spheres has swept spheres on the task data.");
#endif // SHAPETESTSPU_SUPPORT_SWEPTSPHERES

#if SHAPETESTSPU_SUPPORT_TAPEREDSWEPTSPHERES
	Assertf(taskParams.UserDataCount>phShapeTestTaskData::ParamIndex_TaperedSweptSphereShapeTest,"Multithreaded tapered swept sphere shape test needs more task data. Current %i, Required %i", (int)taskParams.UserDataCount, phShapeTestTaskData::ParamIndex_TaperedSweptSphereShapeTest + 1);
	phShapeTest<phShapeTaperedSweptSphere>* taperedSweptSphereShapeTests = reinterpret_cast<phShapeTest<phShapeTaperedSweptSphere> *>(taskParams.UserData[phShapeTestTaskData::ParamIndex_TaperedSweptSphereShapeTest].asPtr);
	ShapeTestSpuHelper(taperedSweptSphereShapeTests, taskData.m_NumTaperedSweptSpheres, scratchAllocator, physicsLevel, levelIterator);
#else // SHAPETESTSPU_SUPPORT_TAPEREDSWEPTSPHERES
	Assertf(taskData.m_NumTaperedSweptSpheres == 0, "Shapetest task that doesn't support tapered swept spheres has tapered swept spheres on the task data.");
#endif // SHAPETESTSPU_SUPPORT_TAPEREDSWEPTSPHERES

	// This is really unnecessary if we're just going to return at this point.
	scratchAllocator.ReleaseToLastMarker();

	//	spu_printf("Done with task!\n");
}

#endif //__SPU
