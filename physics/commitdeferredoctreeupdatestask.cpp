#if __SPU

#include "physics/levelnew.h"

namespace rage {

void phWorkerThreadMain(sysTaskParameters& params)
{
	phLevelNew *ppuPhysLevel = reinterpret_cast<phLevelNew *>(params.UserData[0].asPtr);
	u8 physLevelBuffer[sizeof(phLevelNew)] ;
	sysDmaLargeGet(&physLevelBuffer[0], (uint64_t)(ppuPhysLevel), sizeof(phLevelNew), DMA_TAG(1));
	phLevelNew *spuPhysLevel = reinterpret_cast<phLevelNew *>(&physLevelBuffer[0]);
	sysDmaWaitTagStatusAll(DMA_MASK(1));

	const int numDeferredUpdateLevelIndices = spuPhysLevel->m_uNumDeferredUpdateLevelIndices;
	u16 *deferredUpdateLevelIndices = Alloca(u16, numDeferredUpdateLevelIndices);
	u16 *ppuDeferredUpdateLevelIndices = spuPhysLevel->m_pauDeferredUpdateLevelIndices;
	sysDmaLargeGet(&deferredUpdateLevelIndices[0], (uint64_t)(ppuDeferredUpdateLevelIndices), sizeof(u16) * numDeferredUpdateLevelIndices, DMA_TAG(1));
	spuPhysLevel->m_pauDeferredUpdateLevelIndices = deferredUpdateLevelIndices;
	sysDmaWaitTagStatusAll(DMA_MASK(1));

#if ENABLE_PHYSICS_LOCK
	phMultiReaderLockToken spuPhysicsLock;
	RageCellSyncMutex *ppuModifyReaderCountMutex = (RageCellSyncMutex *)params.UserData[4].asPtr;
	spuPhysicsLock.SetPPUModifyReaderCountMutexPtr(ppuModifyReaderCountMutex);
	u32 *ppuReaderCount = (u32 *)params.UserData[1].asPtr;
	spuPhysicsLock.SetPPUReaderCountPtr(ppuReaderCount);
	RageCellSyncMutex *ppuPhysicsLock = (RageCellSyncMutex *)params.UserData[2].asPtr;
	spuPhysicsLock.SetPPUPhysicsMutexPtr(ppuPhysicsLock);
#if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	RageCellSyncMutex *ppuAllowNewReaderLock = (RageCellSyncMutex *)params.UserData[3].asPtr;
	spuPhysicsLock.SetPPUAllowNewReaderMutexPtr(ppuAllowNewReaderLock);
#endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS

	spuPhysicsLock.WaitAsWriter();
#endif	// ENABLE_PHYSICS_LOCK

//	Displayf("Before CommitDeferredOctreeUpdates()");
	spuPhysLevel->CommitDeferredOctreeUpdates();
//	Displayf("After CommitDeferredOctreeUpdates()");

	spuPhysLevel->m_pauDeferredUpdateLevelIndices = ppuDeferredUpdateLevelIndices;
	sysDmaLargePut(&physLevelBuffer[0], (uint64_t)(ppuPhysLevel), sizeof(phLevelNew), DMA_TAG(1));
	sysDmaWaitTagStatusAll(DMA_MASK(1));

#if ENABLE_PHYSICS_LOCK
	spuPhysicsLock.ReleaseAsWriter();
#endif	// ENABLE_PHYSICS_LOCK
}

}	// namespace rage

#endif // __SPU
