#if __SPU

#include "SpuDoubleBuffer.h"

#include "phbullet/CollisionWorkUnit.h"
#include "physics/collision.h"
#include "physics/manifoldresult.h"
#include "physics/overlappingpairarray.h"
#include "system/spinlock.h"
#include "phbullet/GjkSimplexCache.h"

#if __BANK
#define NON_BANK_ONLY(X)
#else	// __BANK
#define NON_BANK_ONLY(X)	X
#endif	// __BANK


namespace rage {

// TODO: Perhaps phSPUPairFetcher should really be template'd.
const int PAIRS_PER_BUFFER = 2;
class phSPUPairFetcher
{
public:
	phSPUPairFetcher(sysSpinLockToken *ppuPairConsumeToken, int *ppuPairConsumeIndex, int *ppuNumPairs, bool *ppuAllPairsReady, phTaskCollisionPair *ppuPairList, int numPairsPerMutexLock);

	phTaskCollisionPair *GetNextPair();

private:
	//////////////////////////////////////////////////
	// These members are constant for the life of phSPUPairFetcher and must be set in the constructor.
	// PPU pointers
	sysSpinLockToken* m_PPUPairConsumeToken;
	int *m_PPUPairConsumeIndex;
	int *m_PPUNumPairs;
	bool *m_PPUAllPairsReady;
	phTaskCollisionPair *m_PPUPairList;

	// Perhaps this should be a template parameter?
	int m_NumPairsPerMutexLock;
	//
	//////////////////////////////////////////////////

	//////////////////////////////////////////////////
	// These members represent the current state of the phSPUPairFetcher.
	int m_PairsLeft;
	bool m_AllPairsConsumed;
	int m_PairConsumeIndex;

	phTaskCollisionPair *m_DMAInPtr;

	phTaskCollisionPair *m_PairBuffer;
	int m_PairIndexInBuffer;
	int m_PairsInCurrentBuffer;
	int m_PairsInNextBuffer;

	DoubleBuffer<phTaskCollisionPair, PAIRS_PER_BUFFER> m_PairArrayBuffers;
	//
	//////////////////////////////////////////////////
};


phSPUPairFetcher::phSPUPairFetcher(sysSpinLockToken *ppuPairConsumeToken, int *ppuPairConsumeIndex, int *ppuNumPairs, bool *ppuAllPairsReady, phTaskCollisionPair *ppuPairList, int numPairsPerMutexLock) :
	m_PPUPairConsumeToken(ppuPairConsumeToken)
,	m_PPUPairConsumeIndex(ppuPairConsumeIndex)
,	m_PPUNumPairs(ppuNumPairs)
,	m_PPUAllPairsReady(ppuAllPairsReady)
,	m_PPUPairList(ppuPairList)
,	m_NumPairsPerMutexLock(numPairsPerMutexLock)
{
	m_PairsLeft = 0;
	m_AllPairsConsumed = false;
	m_PairIndexInBuffer = 0;
	m_PairsInCurrentBuffer = 0;
}


phTaskCollisionPair *phSPUPairFetcher::GetNextPair()
{
	while(!m_AllPairsConsumed)
	{
		// If we have used all the ones we had, we will have to consume some overlapping pairs from the main memory array
		if(m_PairIndexInBuffer == m_PairsInCurrentBuffer && m_PairsLeft == 0)
		{
			// Consume some pairs, under mutex control
			sysSpinLockLock(*m_PPUPairConsumeToken);

			// Find out what pairs are still available based on the main memory index.
			int numPairsPerAllocation = (m_NumPairsPerMutexLock + 3) & ~3;
			m_PairConsumeIndex = cellDmaGetUint32((uint64_t)m_PPUPairConsumeIndex, DMA_TAG(17), 0, 0);
			int numPairs = cellDmaGetUint32((uint64_t)m_PPUNumPairs, DMA_TAG(17), 0, 0);
			bool allPairsReady = cellDmaGetUint8((uint64_t)m_PPUAllPairsReady, DMA_TAG(17), 0, 0) != 0;
			m_PairsLeft = Min(numPairs - m_PairConsumeIndex, numPairsPerAllocation);
			if(m_PairsLeft <= 0 && allPairsReady)
			{
				m_AllPairsConsumed = true;
			}

			if(m_PairsLeft > 0)
			{
				// Figure out where in main memory the next pairs reside
				m_DMAInPtr = m_PPUPairList + m_PairConsumeIndex;
				m_PairConsumeIndex += m_PairsLeft;

				// Write back where the next SPU task should pick up on the pair list
				cellDmaPutUint32(m_PairConsumeIndex, (uint64_t)m_PPUPairConsumeIndex, DMA_TAG(17), 0, 0);

				m_PairsInNextBuffer = (m_PairsLeft > PAIRS_PER_BUFFER) ? PAIRS_PER_BUFFER : m_PairsLeft;
				m_PairArrayBuffers.backBufferDmaGet((uint64_t)m_DMAInPtr, m_PairsInNextBuffer * sizeof(phTaskCollisionPair), DMA_TAG(17));
				m_PairsInCurrentBuffer = m_PairIndexInBuffer = 0;
				m_DMAInPtr += PAIRS_PER_BUFFER;
			}

			sysSpinLockUnlock(*m_PPUPairConsumeToken);
		}

		// See if the current pair buffer is empty, and we need to DMA some more
		if(m_PairsLeft > 0 && m_PairIndexInBuffer == m_PairsInCurrentBuffer)
		{
			// wait for back buffer dma and swap buffers
			m_PairBuffer = m_PairArrayBuffers.swapBuffers();
			m_PairsInCurrentBuffer = m_PairsInNextBuffer;
			m_PairsLeft -= m_PairsInNextBuffer;
			m_PairIndexInBuffer = 0;

			// prefetch next set of inputs
			if(m_PairsLeft > 0)
			{
				m_PairsInNextBuffer = (m_PairsLeft > PAIRS_PER_BUFFER) ? PAIRS_PER_BUFFER : m_PairsLeft;
				m_PairArrayBuffers.backBufferDmaGet((uint64_t)m_DMAInPtr, m_PairsInNextBuffer * sizeof(phTaskCollisionPair), DMA_TAG(17));
				m_DMAInPtr += PAIRS_PER_BUFFER;
			}
		}

		// If the current buffer has pairs in it, grab one of those
		if(m_PairIndexInBuffer < m_PairsInCurrentBuffer)
		{
			return m_PairBuffer + m_PairIndexInBuffer++;
		}
	}

	return NULL;
}

#if __ASSERT // The same assert on the PPU is in contact.cpp
void AssertOnLargeDepths(phManifold* manifold, const phInst* instA, const phArchetype* archA, const phInst* instB, const phArchetype* archB, Mat34V_In lastA, Mat34V_In lastB, float* previousDepths)
{
	int numContacts = manifold->GetNumContacts();
	for (int contactIndex = 0; contactIndex < numContacts; ++contactIndex)
	{
		phContact& manifoldPoint = manifold->GetContactPoint(contactIndex);

		if (Unlikely(manifoldPoint.GetDepth() >= CONTACT_DEPTH_ASSERT_THRESHOLD && manifoldPoint.IsContactActive()))
		{
			if(manifold->GetColliderA() || manifold->GetColliderB())
			{
				char archNameBuffA[256];
				const char* nameA = NULL;
				if(archA)
				{
					const char* ppuFileName = archA->GetFilename();
					if(ppuFileName)
					{
						u32 alignmentOffset = (u32)ppuFileName & 0xF;
						const char* alignedPpuFileName = ppuFileName - alignmentOffset;
						sysDmaGetAndWait(archNameBuffA, (uint64_t)alignedPpuFileName, 256, DMA_TAG(13));
						nameA = archNameBuffA + alignmentOffset;
					}
				}

				char archNameBuffB[256];
				const char* nameB = NULL;
				if(archB)
				{
					const char* ppuFileName = archB->GetFilename();
					if(ppuFileName)
					{
						u32 alignmentOffset = (u32)ppuFileName & 0xF;
						const char* alignedPpuFileName = ppuFileName - alignmentOffset;
						sysDmaGetAndWait(archNameBuffB, (uint64_t)alignedPpuFileName, 256, DMA_TAG(13));
						nameB = archNameBuffB + alignmentOffset;
					}
				}

				Vec3V posA = instA ? instA->GetPosition() : Vec3V(V_ZERO);
				Vec3V posB = instB ? instB->GetPosition() : Vec3V(V_ZERO);
				Vec3V prevPosA = lastA.GetCol3();
				Vec3V prevPosB = lastB.GetCol3();

				// Errorf here so we get the actual data in the logs, since Assertf will drop it on the SPU at the moment
				Errorf(	"Contact depth %f exceeded allowed threshold %f"
					"\n\tPrevious Depth: %f"
					"\n\tObject A - '%s'"
					"\n\t\tPosition - <%5.2f, %5.2f, %5.2f>"
					"\n\t\tPrev Position - <%5.2f, %5.2f, %5.2f>"
					"\n\tObject B - '%s'"
					"\n\t\tPosition - <%5.2f, %5.2f, %5.2f>"
					"\n\t\tPrev Position - <%5.2f, %5.2f, %5.2f>",
					manifoldPoint.GetDepth(),
					CONTACT_DEPTH_ASSERT_THRESHOLD,
					previousDepths[contactIndex],
					nameA,
					VEC3V_ARGS(posA),
					VEC3V_ARGS(prevPosA),
					nameB,
					VEC3V_ARGS(posB),
					VEC3V_ARGS(prevPosB));

				Assertf(	manifoldPoint.GetDepth() < CONTACT_DEPTH_ASSERT_THRESHOLD,
					"Contact depth %f exceeded allowed threshold %f"
					"\n\tPrevious Depth: %f"
					"\n\tObject A - '%s'"
					"\n\t\tPosition - <%5.2f, %5.2f, %5.2f>"
					"\n\t\tPrev Position - <%5.2f, %5.2f, %5.2f>"
					"\n\tObject B - '%s'"
					"\n\t\tPosition - <%5.2f, %5.2f, %5.2f>"
					"\n\t\tPrev Position - <%5.2f, %5.2f, %5.2f>",
					manifoldPoint.GetDepth(),
					CONTACT_DEPTH_ASSERT_THRESHOLD,
					previousDepths[contactIndex],
					nameA,
					VEC3V_ARGS(posA),
					VEC3V_ARGS(prevPosA),
					nameB,
					VEC3V_ARGS(posB),
					VEC3V_ARGS(prevPosB));
			}
		}
	}
}
#endif // __ASSERT

// Returns a mask based on whether or not we issued a DMA put from the rootManifold
u32 ProcessPair(phTaskCollisionPair *spuPair, const PairListWorkUnitInput &wui, phSpuManifoldWrapper &rootManifoldWrapper, bool BANK_ONLY(selfCollisionsEnabled), int rootManifoldDmaTag, bool BANK_ONLY(sweepFromSafe), phCollisionMemory * collisionMemory)
{
	sysTimer collisionTimer;

	u8 instanceABuffer[sizeof(phInst)] ;
	u8 instanceBBuffer[sizeof(phInst)] ;
	u8 archetypeABuffer[sizeof(phArchetype)] ;
	u8 archetypeBBuffer[sizeof(phArchetype)] ;
	u8 colliderABuffer[sizeof(phArticulatedCollider)] ;
	u8 colliderBBuffer[sizeof(phArticulatedCollider)] ;
	u8 boundABuffer[phBound::MAX_BOUND_SIZE] ;
	u8 boundBBuffer[phBound::MAX_BOUND_SIZE] ;
	u8 objectDataBufferA[sizeof(phObjectData)] ;
	u8 objectDataBufferB[sizeof(phObjectData)] ;

	const phInst *spuInstA = reinterpret_cast<const phInst *>(&instanceABuffer[0]);
	const phInst *spuInstB = reinterpret_cast<const phInst *>(&instanceBBuffer[0]);
	const phArchetype *spuArchetypeA = reinterpret_cast<const phArchetype *>(&archetypeABuffer[0]);
	const phArchetype *spuArchetypeB = reinterpret_cast<const phArchetype *>(&archetypeBBuffer[0]);
	const phArticulatedCollider *spuColliderA = reinterpret_cast<const phArticulatedCollider *>(&colliderABuffer[0]);
	const phArticulatedCollider *spuColliderB = reinterpret_cast<const phArticulatedCollider *>(&colliderBBuffer[0]);
	const phBound *spuBoundA = reinterpret_cast<const phBound *>(&boundABuffer[0]);
	const phBound *spuBoundB = reinterpret_cast<const phBound *>(&boundBBuffer[0]);
	const phObjectData *spuObjectDataA = reinterpret_cast<const phObjectData *>(&objectDataBufferA[0]);
	const phObjectData *spuObjectDataB = reinterpret_cast<const phObjectData *>(&objectDataBufferB[0]);

	const Mat34V *ppuInstLastMatricesBaseAddrMM = wui.m_InstLastMatrices;
	const u8 *ppuLevelIdxToInstLastMatrixMapMM = wui.m_LevelIdxToLastMatrixMap;

	int numSelfCollisionPairs = 0;	// Initialized just so that the compiler doesn't complain.
	u8 *selfCollisionPairsA = NULL;
	u8 *selfCollisionPairsB = NULL;

	phManifold *spuManifold = rootManifoldWrapper.GetSpuManifold();

	sysDmaWaitTagStatusAll(DMA_MASK(rootManifoldDmaTag));
	rootManifoldWrapper.InitiateDmaGetStageOne(spuPair->manifold, DMA_TAG(rootManifoldDmaTag));
	sysDmaWaitTagStatusAll(DMA_MASK(rootManifoldDmaTag));

	if (spuManifold->GetInstanceA() == NULL || spuManifold->GetInstanceB() == NULL)
	{
		return 0x00000000;
	}

	const int INST_TAG = 1;
	sysDmaLargeGet(spuInstA, (uint64_t)spuManifold->GetInstanceA(), sizeof(phInst), DMA_TAG(INST_TAG));
	sysDmaLargeGet(spuInstB, (uint64_t)spuManifold->GetInstanceB(), sizeof(phInst), DMA_TAG(INST_TAG));

	phCollisionInput collisionInput(collisionMemory,true);
	collisionInput.rootManifold = spuManifold;
	spuPair->manifold = spuManifold;

	sysDmaWaitTagStatusAll(DMA_MASK(INST_TAG));
	// spuInstA and spuInstB are now available.

	// Kick these off as soon as the instances are available.
	const int ARCH_TAG = 1;
	sysDmaLargeGet(spuArchetypeA, (uint64_t)spuInstA->GetArchetype(), sizeof(phArchetype), DMA_TAG(ARCH_TAG));
	sysDmaLargeGet(spuArchetypeB, (uint64_t)spuInstB->GetArchetype(), sizeof(phArchetype), DMA_TAG(ARCH_TAG));

	FastAssert(spuInstA->GetLevelIndex() == spuPair->levelIndex1);
	FastAssert(spuInstB->GetLevelIndex() == spuPair->levelIndex2);
	const int levelIndex1 = spuManifold->GetLevelIndexA();
	const int levelIndex2 = spuManifold->GetLevelIndexB();
	FastAssert(levelIndex1 == spuPair->levelIndex1);
	FastAssert(levelIndex2 == spuPair->levelIndex2);

#if EARLY_FORCE_SOLVE
	// With EARLY_FORCE_SOLVE, it's unavoidable to get constraint pairs in the collision system when doing push collisions. This is
	// because we're collecting pairs based on island membership. We need those pairs to go into the solver, so we need to just skip
	// over them when collision gets to them.
	if (spuManifold->IsConstraint())
	{
		// Make sure to wait on any DMAs that we kicked off that are going into locations on the stack.  We wouldn't want those to complete after this
		//   function has terminated.
		//sysDmaWaitTagStatusAll(DMA_MASK(1));
		sysDmaWaitTagStatusAll(DMA_MASK(ARCH_TAG));
		return 0x00000000;
	}
#endif

	rootManifoldWrapper.InitiateDmaGetStageTwo(DMA_TAG(rootManifoldDmaTag));

	const int COLLIDER_TAG = 3;
	const phCollider *ppuColliderA = spuManifold->GetColliderA();
	if(ppuColliderA != NULL)
	{
		sysDmaLargeGet(&colliderABuffer[0], (uint64_t)ppuColliderA, sizeof(phArticulatedCollider), DMA_TAG(COLLIDER_TAG));
	}
	else if(spuInstA->HasLastMatrix())
	{
		const u8 *mmAddr = ppuLevelIdxToInstLastMatrixMapMM + levelIndex1;
		mmAddr = (u8*)((u32)mmAddr & 0xFFFFFFF0);
		// Here we are using collisionInput.lastA just as a dma buffer for the index since it is not used until we have the index, and it should be 16 byte aligned.
		sysDmaGet(&collisionInput.lastA, (uint64_t)mmAddr, 16, DMA_TAG(COLLIDER_TAG));
	}
	const phCollider *ppuColliderB = spuManifold->GetColliderB();
	if(ppuColliderB != NULL)
	{
		sysDmaLargeGet(&colliderBBuffer[0], (uint64_t)ppuColliderB, sizeof(phArticulatedCollider), DMA_TAG(COLLIDER_TAG));
	}
	else if(spuInstB->HasLastMatrix())
	{
		const u8 *mmAddr = ppuLevelIdxToInstLastMatrixMapMM + levelIndex2;
		mmAddr = (u8*)((u32)mmAddr & 0xFFFFFFF0);
		// Here we are using collisionInput.lastB just as a dma buffer for the index since it is not used until we have the index, and it should be 16 byte aligned.
		sysDmaGet(&collisionInput.lastB, (uint64_t)mmAddr, 16, DMA_TAG(COLLIDER_TAG));
	}

	const int OBJECTDATA_TAG = 4;
	const phObjectData *ppuObjectDataArrayBase = (const phObjectData *)wui.m_LevelObjectDataArray;
	sysDmaGet(spuObjectDataA, (uint64_t)(&ppuObjectDataArrayBase[levelIndex1]), sizeof(phObjectData), DMA_TAG(OBJECTDATA_TAG));
	sysDmaGet(spuObjectDataB, (uint64_t)(&ppuObjectDataArrayBase[levelIndex2]), sizeof(phObjectData), DMA_TAG(OBJECTDATA_TAG));

	collisionInput.currentA = spuInstA->GetMatrix();
	collisionInput.currentB = spuInstB->GetMatrix();

	// Wait on the archetypes.
	//sysDmaWaitTagStatusAll(DMA_MASK(1));
	sysDmaWaitTagStatusAll(DMA_MASK(ARCH_TAG));

	const int BOUND_TAG = 1;
	sysDmaLargeGet(spuBoundA, (uint64_t)spuArchetypeA->GetBound(), phBound::MAX_BOUND_SIZE, DMA_TAG(BOUND_TAG));
	sysDmaLargeGet(spuBoundB, (uint64_t)spuArchetypeB->GetBound(), phBound::MAX_BOUND_SIZE, DMA_TAG(BOUND_TAG));

	// Wait on the colliders/last matrices.
	//sysDmaWaitTagStatusAll(DMA_MASK(3));
	sysDmaWaitTagStatusAll(DMA_MASK(COLLIDER_TAG));

	bool doCollision;
	bool doSelfCollision;

	const bool isSelfCollision = (spuManifold->GetInstanceA() == spuManifold->GetInstanceB());

	// Pairs with no colliders shouldn't have gotten this far unless we're on the first (non-push) collision detection pass.
#if EARLY_FORCE_SOLVE
	if ((ppuColliderA != NULL || ppuColliderB != NULL) && (!ppuColliderA || !spuColliderA->GetNeedsCollision()) &&
		(!ppuColliderB || !spuColliderB->GetNeedsCollision()))
	{
		doCollision = false;
		doSelfCollision = false;
	}
	else
#endif
	if(Unlikely(isSelfCollision))
	{
		doCollision = false;
		doSelfCollision = false;

#if __BANK
		if (Likely(selfCollisionsEnabled))
#endif // __BANK
		{
			Assert(ppuColliderA == ppuColliderB);
			Assert(spuColliderA->GetType() == phCollider::TYPE_ARTICULATED_BODY || spuColliderA->GetType() == phCollider::TYPE_ARTICULATED_LARGE_ROOT);
			const phArticulatedCollider *articulatedCollider = static_cast<const phArticulatedCollider *>(spuColliderA);
			numSelfCollisionPairs = articulatedCollider->m_PartCanCollideA.GetCount();

			// See if there's any reason not to process self collision pairs for this object.
			const bool anyPartsCanCollide = articulatedCollider->m_AnyPartsCanCollide;
			const int disableSelfCollisionFramesLeft  = articulatedCollider->m_DisableSelfCollisionFramesLeft;
			if (Likely(!(!anyPartsCanCollide || disableSelfCollisionFramesLeft > 0 || numSelfCollisionPairs == 0)))
			{
				selfCollisionPairsA = Alloca(u8, numSelfCollisionPairs);
				selfCollisionPairsB = Alloca(u8, numSelfCollisionPairs);
				sysDmaLargeGet(selfCollisionPairsA, (uint64_t)(&articulatedCollider->m_PartCanCollideA[0]), sizeof(u8) * numSelfCollisionPairs, DMA_TAG(3));
				sysDmaLargeGet(selfCollisionPairsB, (uint64_t)(&articulatedCollider->m_PartCanCollideB[0]), sizeof(u8) * numSelfCollisionPairs, DMA_TAG(3));

				doSelfCollision = true;
			}
		}
	}
	else
	{
		doCollision = true;
		doSelfCollision = false;
	}

	Mat34V instLastA;
	if(ppuColliderA != NULL)
	{
		instLastA = spuColliderA->GetLastInstanceMatrix();

		if (ppuColliderB || spuManifold->GetInactiveCollidesAgainstInactiveB() BANK_ONLY(|| !sweepFromSafe))
		{
			collisionInput.lastA = spuColliderA->GetLastInstanceMatrix();
		}
		else
		{
			collisionInput.lastA = spuColliderA->GetLastSafeInstanceMatrix();
		}
	}
	else if(spuInstA->HasLastMatrix())
	{
		u32 offset16 = (u32)(ppuLevelIdxToInstLastMatrixMapMM + spuInstA->GetLevelIndex()) & 0xF;
		const u32 instLastMatrixIdxA = *((const u8 *)&collisionInput.lastA + offset16);

		const Mat34V *mtxAddrMM = ppuInstLastMatricesBaseAddrMM + instLastMatrixIdxA;
		sysDmaGet(&collisionInput.lastA, (uint64_t)mtxAddrMM, sizeof(Mat34V), DMA_TAG(5));
		sysDmaGet(&instLastA, (uint64_t)mtxAddrMM, sizeof(Mat34V), DMA_TAG(5));
	}
	else
	{
		collisionInput.lastA = spuInstA->GetMatrix();
		instLastA = spuInstA->GetMatrix();
	}

	Mat34V instLastB;
	if(ppuColliderB != NULL)
	{
		instLastB = spuColliderB->GetLastInstanceMatrix();

		if (ppuColliderA || spuManifold->GetInactiveCollidesAgainstInactiveA() BANK_ONLY(|| !sweepFromSafe))
		{
			collisionInput.lastB = spuColliderB->GetLastInstanceMatrix();
		}
		else
		{
			collisionInput.lastB = spuColliderB->GetLastSafeInstanceMatrix();
		}
	}
	else if(spuInstB->HasLastMatrix())
	{
		u32 offset16 = (u32)(ppuLevelIdxToInstLastMatrixMapMM + spuInstB->GetLevelIndex()) & 0xF;
		const u32 instLastMatrixIdxB = *((const u8 *)&collisionInput.lastB + offset16);

		const Mat34V *mtxAddrMM = ppuInstLastMatricesBaseAddrMM + instLastMatrixIdxB;
		sysDmaGet(&collisionInput.lastB, (uint64_t)mtxAddrMM, sizeof(Mat34V), DMA_TAG(5));
		sysDmaGet(&instLastB, (uint64_t)mtxAddrMM, sizeof(Mat34V), DMA_TAG(5));
	}
	else
	{
		collisionInput.lastB = spuInstB->GetMatrix();
		instLastB = spuInstB->GetMatrix();
	}

	collisionInput.boundA = spuBoundA;
	collisionInput.boundB = spuBoundB;

	// Wait on the object data.
	//sysDmaWaitTagStatusAll(DMA_MASK(4));
	sysDmaWaitTagStatusAll(DMA_MASK(OBJECTDATA_TAG));

	collisionInput.typeFlagsA = spuObjectDataA->m_CachedArchTypeFlags;
	collisionInput.includeFlagsA = spuObjectDataA->m_CachedArchIncludeFlags;
	collisionInput.typeFlagsB = spuObjectDataB->m_CachedArchTypeFlags;
	collisionInput.includeFlagsB = spuObjectDataB->m_CachedArchIncludeFlags;

	// Wait on the bounds.
	//sysDmaWaitTagStatusAll(DMA_MASK(1));
	sysDmaWaitTagStatusAll(DMA_MASK(BOUND_TAG));

	const int boundsArraysDmaTag = 15;

	// DMA the composite manifold bound arrays.
	Mat34V* ppuCompositeCurrentMatsA = NULL;
	Mat34V* ppuCompositeLastMatsA = NULL;
	int numBoundsA = 0;
	phBound** spuCompositeBoundArrayA = NULL;
	if(spuBoundA->GetType() == phBound::COMPOSITE)
	{
		const phBoundComposite * boundCompositeA = reinterpret_cast<const phBoundComposite*>(spuBoundA);
		ppuCompositeCurrentMatsA = boundCompositeA->m_CurrentMatrices;
		ppuCompositeLastMatsA = boundCompositeA->m_LastMatrices;
		numBoundsA = boundCompositeA->GetNumBounds();
		spuCompositeBoundArrayA = Alloca(phBound*, numBoundsA);
		sysDmaLargeGet(spuCompositeBoundArrayA, (uint64_t)boundCompositeA->GetBoundArray(), sizeof(phBound*) * numBoundsA, DMA_TAG(boundsArraysDmaTag));
		const_cast<phBoundComposite*>(boundCompositeA)->SetBoundArray(spuCompositeBoundArrayA);
	}

	Mat34V* ppuCompositeCurrentMatsB = NULL;
	Mat34V* ppuCompositeLastMatsB = NULL;
	phBound** spuCompositeBoundArrayB = NULL;
	int numBoundsB = 0;
	if(spuBoundB->GetType() == phBound::COMPOSITE)
	{
		if (isSelfCollision)
		{
			// Don't double DMA the bounds.
			ppuCompositeCurrentMatsB = ppuCompositeCurrentMatsA;
			ppuCompositeLastMatsB = ppuCompositeLastMatsA;
			numBoundsB = numBoundsA;
			spuCompositeBoundArrayB = spuCompositeBoundArrayA;
		}
		else
		{
			const phBoundComposite * boundCompositeB = reinterpret_cast<const phBoundComposite*>(spuBoundB);
			ppuCompositeCurrentMatsB = boundCompositeB->m_CurrentMatrices;
			ppuCompositeLastMatsB = boundCompositeB->m_LastMatrices;
			numBoundsB = boundCompositeB->GetNumBounds();
			spuCompositeBoundArrayB = Alloca(phBound*, numBoundsB);
			sysDmaLargeGet(spuCompositeBoundArrayB, (uint64_t)boundCompositeB->GetBoundArray(), sizeof(phBound*) * numBoundsB, DMA_TAG(boundsArraysDmaTag));
			const_cast<phBoundComposite*>(boundCompositeB)->SetBoundArray(spuCompositeBoundArrayB);
		}
	}

#if !__FINAL
	// Catch composites that are too big and don't have a BVH structure on them.
	if(spuBoundA->GetType() == phBound::COMPOSITE)
	{
		const phBoundComposite *compositeBoundA = static_cast<const phBoundComposite *>(spuBoundA);
		const int numComponentsA = compositeBoundA->GetNumBounds();
		if(Unlikely(compositeBoundA->GetBVHStructure() == NULL && numComponentsA > 50))
		{
			char archetypeNameBuffer[128] = {0};
			const char* ppuFileName = spuArchetypeA->GetFilename();
			Displayf("ppuFilename: 0x%X",(u32)ppuFileName);
			const char* spuFileName = archetypeNameBuffer;
			if(ppuFileName)
			{
				u32 alignmentOffset = (u32)ppuFileName & 0xF;
				const char* alignedPpuFileName = ppuFileName - alignmentOffset;
				Displayf("alignmentOffset: 0x%X",(u32)alignmentOffset);
				Displayf("alignedPpuFileName: 0x%X",(u32)alignedPpuFileName);
				sysDmaGetAndWait(archetypeNameBuffer, (uint64_t)alignedPpuFileName, 128, DMA_TAG(13));
				spuFileName += alignmentOffset;
			}
			phBoundComposite *nonConstCompositeBoundA = const_cast<phBoundComposite *>(compositeBoundA);
			nonConstCompositeBoundA->SetNumBounds(50);
			Warningf("Found composite bound in archetype '%s' with %d components but no BVH structure - pretending like those extra bounds don't exist", spuFileName, numComponentsA);
		}
	}

	if(spuBoundB->GetType() == phBound::COMPOSITE)
	{
		const phBoundComposite *compositeBoundB = static_cast<const phBoundComposite *>(spuBoundB);
		const int numComponentsB = compositeBoundB->GetNumBounds();
		if(Unlikely(compositeBoundB->GetBVHStructure() == NULL && numComponentsB > 50))
		{
			char archetypeNameBuffer[128] = {0};
			const char* ppuFileName = spuArchetypeB->GetFilename();
			Displayf("ppuFilename: 0x%X",(u32)ppuFileName);
			const char* spuFileName = archetypeNameBuffer;
			if(ppuFileName)
			{
				u32 alignmentOffset = (u32)ppuFileName & 0xF;;
				const char* alignedPpuFileName = ppuFileName - alignmentOffset;
				Displayf("alignmentOffset: 0x%X",(u32)alignmentOffset);
				Displayf("alignedPpuFileName: 0x%X",(u32)alignedPpuFileName);
				sysDmaGetAndWait(archetypeNameBuffer, (uint64_t)alignedPpuFileName, 128, DMA_TAG(13));
				spuFileName += alignmentOffset;
			}
			phBoundComposite *nonConstCompositeBoundB = const_cast<phBoundComposite *>(compositeBoundB);
			nonConstCompositeBoundB->SetNumBounds(50);
			Warningf("Found composite bound in archetype '%s' with %d components but no BVH structure - pretending like those extra bounds don't exist", spuFileName, numComponentsB);
		}
	}
#endif	// !__FINAL

	// Wait for the manifold composite pointers OR root manifold contacts (StageTwoDMA)
	sysDmaWaitTagStatusAll(DMA_MASK(rootManifoldDmaTag));

	// Wait on the last matrices for instances.
	sysDmaWaitTagStatusAll(DMA_MASK(5));
	
	// Wait on the composite manifold bound arrays.
	sysDmaWaitTagStatusAll(DMA_MASK(boundsArraysDmaTag));

	phMidphase midphaseProcessor;
	if(Likely(doCollision))
	{
		Assert(!isSelfCollision);
		Assert(!doSelfCollision);

		midphaseProcessor.ProcessCollision(collisionInput);
	}
	else if (Likely(doSelfCollision))
	{
		Assert(isSelfCollision);
		Assert(!doCollision);

		// Get the relevant stuff for the composite bound.
		const phBoundComposite *compositeBound = static_cast<const phBoundComposite *>(spuBoundA);
		phBoundComposite *nonConstCompositeBound = const_cast<phBoundComposite *>(compositeBound);
		const int numComponents = compositeBound->GetNumBounds();

		// The bound array was DMA'ed above.
		//phBound **spuComponentPointers = Alloca(phBound *, numComponents);
		//sysDmaLargeGet(spuComponentPointers, (uint64_t)compositeBound->GetBoundArray(), sizeof(phBound *) * numComponents, DMA_TAG(1));

		Mat34V *spuCurrentMatrices = Alloca(Mat34V, numComponents);
		sysDmaLargeGet(spuCurrentMatrices, (uint64_t)compositeBound->GetCurrentMatrices(), sizeof(Mat34V) * numComponents, DMA_TAG(1));

		Mat34V *spuLastMatrices = Alloca(Mat34V, numComponents);
		sysDmaLargeGet(spuLastMatrices, (uint64_t)compositeBound->GetLastMatrices(), sizeof(Mat34V) * numComponents, DMA_TAG(1));

#if !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP
		Vec3V *spuLocalBoxMinMaxs = Alloca(Vec3V, 2 * numComponents);
		sysDmaLargeGet(spuLocalBoxMinMaxs, (uint64_t)nonConstCompositeBound->GetLocalBoxMinMaxsArray(), sizeof(Vec3V) * 2 * numComponents, DMA_TAG(1));
#endif // !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP

		//nonConstCompositeBound->SetBoundArray(spuComponentPointers);
		nonConstCompositeBound->m_CurrentMatrices = spuCurrentMatrices;
		nonConstCompositeBound->m_LastMatrices = spuLastMatrices;
#if !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP
		nonConstCompositeBound->m_LocalBoxMinMaxs = spuLocalBoxMinMaxs;
#endif // !USE_NEW_SELF_COLLISION || ALLOW_MID_PHASE_SWAP

		// Wait for the stuff we just kicked off and the last instance matrices for instances.
		sysDmaWaitTagStatusAll(DMA_MASK(1));
		// Wait for the self collision pairs.
		sysDmaWaitTagStatusAll(DMA_MASK(3));

		midphaseProcessor.ProcessSelfCollision(collisionInput, selfCollisionPairsA, selfCollisionPairsB, numSelfCollisionPairs);
	}

	// Presumably I should be setting the bound pointers here too, but nobody actually seems to care about that anyway.
	//	spuManifold->SetBoundA(spuArchetypeA->GetBound());
	//	spuManifold->SetBoundB(spuArchetypeB->GetBound());

	// Refresh the contacts' world positions. We handle this here even for contacts which were cached from previous frames.
	
	if (spuManifold->CompositeManifoldsEnabled())
	{
		const int numCompositeManifolds = spuManifold->GetNumCompositeManifolds();
		for (int manifoldIndex = 0; manifoldIndex < numCompositeManifolds; ++manifoldIndex)
		{
			phManifold* ppuCompositeManifold = spuManifold->GetCompositeManifold(manifoldIndex);

			// Get the sub-manifold
			u8 manifoldBuffer[sizeof(phManifold)] ALIGNED(128);
			static const int compositeManifoldDmaTag = 9;
			sysDmaGet(manifoldBuffer, uint64_t(ppuCompositeManifold), sizeof(phManifold), DMA_TAG(compositeManifoldDmaTag));
			sysDmaWaitTagStatusAll(DMA_MASK(compositeManifoldDmaTag));

			phManifold* spuCompositeManifold = (phManifold*)manifoldBuffer;

			phContact contacts[phManifold::MANIFOLD_CACHE_SIZE];
			phContact* contactsLs[phManifold::MANIFOLD_CACHE_SIZE] = { &contacts[0], &contacts[1], &contacts[2], &contacts[3], &contacts[4], &contacts[5] };
			phCompositePointers compositePointersLs;
			Assert(!spuCompositeManifold->CompositeManifoldsEnabled());

			// Get the contacts
			spuCompositeManifold->SetContactsLs(contactsLs);
			static const int compositeContactsDmaTag = 10;
			spuCompositeManifold->GatherContactPointsFromMm(DMA_TAG(compositeContactsDmaTag));

			bool abort = false;

			// Compute the last and current matrices for A
			Mat34V currentA;
			Mat34V lastA;
			if (spuBoundA->GetType() == phBound::COMPOSITE)
			{
				int componentA = spuCompositeManifold->GetComponentA();
				if (componentA < 0 || componentA >= numBoundsA)
				{
					abort = true;
				}
				else if (!spuCompositeBoundArrayA[componentA])
				{
					abort = true;
				}

				if (Likely(!abort))
				{
					Mat34V currentPartA;
					sysDmaGet(&currentPartA, uint64_t(ppuCompositeCurrentMatsA + componentA), sizeof(Mat34V), DMA_TAG(compositeManifoldDmaTag));
					Mat34V lastPartA;
					sysDmaGet(&lastPartA, uint64_t(ppuCompositeLastMatsA + componentA), sizeof(Mat34V), DMA_TAG(compositeManifoldDmaTag));
					sysDmaWaitTagStatusAll(DMA_MASK(compositeManifoldDmaTag));

					Transform(currentA, collisionInput.currentA, currentPartA);
					Transform(lastA, instLastA, lastPartA);
				}
			}
			else
			{
				currentA = collisionInput.currentA;
				lastA = instLastA;
			}

			// Compute the last and current matrices for B
			Mat34V currentB;
			Mat34V lastB;
			if (spuBoundB->GetType() == phBound::COMPOSITE)
			{
				int componentB = spuCompositeManifold->GetComponentB();
				if (componentB < 0 || componentB >= numBoundsB)
				{
					abort = true;
				}
				else if (!spuCompositeBoundArrayB[componentB])
				{
					abort = true;
				}

				if (Likely(!abort))
				{
					Mat34V currentPartB;
					sysDmaGet(&currentPartB, uint64_t(ppuCompositeCurrentMatsB + componentB), sizeof(Mat34V), DMA_TAG(compositeManifoldDmaTag));
					Mat34V lastPartB;
					sysDmaGet(&lastPartB, uint64_t(ppuCompositeLastMatsB + componentB), sizeof(Mat34V), DMA_TAG(compositeManifoldDmaTag));
					sysDmaWaitTagStatusAll(DMA_MASK(compositeManifoldDmaTag));

					Transform(currentB, collisionInput.currentB, currentPartB);
					Transform(lastB, instLastB, lastPartB);
				}
			}
			else
			{
				currentB = collisionInput.currentB;
				lastB = instLastB;
			}

			// We need the contacts before we can start refresh
			sysDmaWaitTagStatusAll(DMA_MASK(compositeContactsDmaTag));

			if (Likely(!abort))
			{
				if (!wui.m_pushCollision)
				{
					spuCompositeManifold->IncrementContactLifetimes();
				}

#if	__ASSERT
				float previousDepths[phManifold::MANIFOLD_CACHE_SIZE];
				for(int contactIndex = 0; contactIndex < spuCompositeManifold->GetNumContacts(); ++contactIndex)
				{
					previousDepths[contactIndex] = spuCompositeManifold->GetContactPoint(contactIndex).GetDepth();
				}
#endif // __ASSERT

				spuCompositeManifold->RefreshContactPoints(wui.m_MinManifoldPointLifetime, currentA, lastA, currentB, lastB, ScalarV(wui.m_TimeStep));

				ASSERT_ONLY(AssertOnLargeDepths(spuCompositeManifold, spuInstA, spuArchetypeA, spuInstB, spuArchetypeB, lastA, lastB, previousDepths);)

				// DMA results back to main memory.
				spuCompositeManifold->ScatterContactPointsToMm(DMA_TAG(compositeManifoldDmaTag));
			}
			else
			{
				spuCompositeManifold->RemoveAllContacts();
			}

			sysDmaPut(manifoldBuffer, uint64_t(ppuCompositeManifold), sizeof(phManifold), DMA_TAG(compositeManifoldDmaTag));
			sysDmaWaitTagStatusAll(DMA_MASK(compositeManifoldDmaTag));
		}
	}
	else
	{
		if (!wui.m_pushCollision)
		{
			spuManifold->IncrementContactLifetimes();
		}

#if	__ASSERT
		float previousDepths[phManifold::MANIFOLD_CACHE_SIZE];
		for(int contactIndex = 0; contactIndex < spuManifold->GetNumContacts(); ++contactIndex)
		{
			previousDepths[contactIndex] = spuManifold->GetContactPoint(contactIndex).GetDepth();
		}
#endif // __ASSERT

		spuManifold->RefreshContactPoints(wui.m_MinManifoldPointLifetime,
			collisionInput.currentA,
			instLastA,
			collisionInput.currentB,
			instLastB,
			ScalarV(wui.m_TimeStep));

		ASSERT_ONLY(AssertOnLargeDepths(spuManifold, spuInstA, spuArchetypeA, spuInstB, spuArchetypeB, instLastA, instLastB, previousDepths);)
	}

	spuManifold->SetCollisionTime(collisionTimer.GetTickTime());

	rootManifoldWrapper.InitiateDmaPut(DMA_TAG(rootManifoldDmaTag));
	return 0xffffffff;
}

#if ALLOW_MID_PHASE_SWAP
extern bool g_UseNewMidPhaseCollision;
#endif

void phWorkerThreadMain(sysTaskParameters& params)
{
	// DMA over the first set of inputs.
	PairListWorkUnitInput pairListWorkUnitInput ;
	cellDmaLargeGet(&pairListWorkUnitInput, (uint64_t)params.Input.Data, sizeof(pairListWorkUnitInput), DMA_TAG(1), 0, 0);
	cellDmaWaitTagStatusAll(DMA_MASK(1));

	bool selfCollisionsEnabled = pairListWorkUnitInput.m_selfCollisionsEnabled;

#if __BANK
#if OCTANT_MAP_SUPPORT_ACCEL
	phBoundPolyhedron::GetUseOctantMapRef() = pairListWorkUnitInput.m_useOctantMap;
#endif // OCTANT_MAP_SUPPORT_ACCEL
#if USE_BOX_BOX_DISTANCE
	g_UseBoxBoxDistance = pairListWorkUnitInput.m_useBoxBoxDistance;
#endif // USE_BOX_BOX_DISTANCE
#if CAPSULE_TO_CAPSULE_DISTANCE
	g_CapsuleCapsuleDistance = pairListWorkUnitInput.m_fastCapsuleToCapsule;
#endif // CAPSULE_TO_CAPSULE_DISTANCE
#if CAPSULE_TO_TRIANGLE_DISTANCE
	g_CapsuleTriangleDistance = pairListWorkUnitInput.m_fastCapsuleToTriangle;
#endif // CAPSULE_TO_TRIANGLE_DISTANCE
#if DISC_TO_TRIANGLE_DISTANCE
	g_DiscTriangleDistance = pairListWorkUnitInput.m_fastDiscToTriangle;
#endif // DISC_TO_TRIANGLE_DISTANCE
#if BOX_TO_TRIANGLE_DISTANCE
	g_BoxTriangleDistance = pairListWorkUnitInput.m_fastBoxToTriangle;
#endif // BOX_TO_TRIANGLE_DISTANCE
#endif // __BANK

#if ALLOW_MID_PHASE_SWAP
	g_UseNewMidPhaseCollision = pairListWorkUnitInput.m_UseNewMidPhaseCollision;
#endif

	phPool<phManifold>::InitSpu(pairListWorkUnitInput.m_manifoldPoolInitParams);
	phPool<phContact>::InitSpu(pairListWorkUnitInput.m_contactPoolInitParams);
	phPool<phCompositePointers>::InitSpu(pairListWorkUnitInput.m_compositePointersPoolInitParams);

	const PairListWorkUnitInput &wui = pairListWorkUnitInput;
	phSPUPairFetcher pairFetcher(wui.m_pairConsumeToken, wui.m_pairConsumeIndex, wui.m_numPairsMM, wui.m_allPairsReadyMM, wui.m_pairListPtr, wui.m_numPairsPerMutexLock);

	u32 numMaterialOverridePairs = wui.m_NumMaterialOverridePairs;
	const phMaterialPair* spuMaterialOverridePairs = Alloca(phMaterialPair, numMaterialOverridePairs);
	cellDmaLargeGet(spuMaterialOverridePairs, (uint64_t)wui.m_MaterialOverridePairs, sizeof(phMaterialPair) * numMaterialOverridePairs, DMA_TAG(1), 0, 0);
	phManifoldResult::SetMaterialData(wui.m_materialArray, wui.m_numMaterials, wui.m_materialStride, wui.m_materialMask, spuMaterialOverridePairs, numMaterialOverridePairs);

#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
	SpuInitGjkCacheSystem(wui);
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE

#if __BANK && __PS3
	g_UseGJKCache = wui.m_UseGJKCache;
	g_UseFramePersistentGJKCache = wui.m_UseFramePersistentGJKCache;
#endif // __BANK && __PS3

	phSpuManifoldWrapper spuManifoldWrapper0;
	phSpuManifoldWrapper spuManifoldWrapper1;

	phTaskCollisionPair *spuPair = pairFetcher.GetNextPair();
	phSpuManifoldWrapper *curSpuManifoldWrapper = &spuManifoldWrapper0;
	const int ManifoldBufferTag0 = 7;
	const int ManifoldBufferTag1 = 8;
	int curManifoldBufTag = ManifoldBufferTag0;

	bool sweepFromSafe = true;
	BANK_ONLY(sweepFromSafe = pairListWorkUnitInput.m_sweepFromSafe);
	cellDmaWaitTagStatusAll(DMA_MASK(1)); // Wait for the material override pairs to DMA over
	phCollisionMemory collisionMemory;
	while(spuPair != NULL)
	{
		//Displayf("Pair: %d, %d", pair->levelIndex1, pair->levelIndex2);
		u32 swapMask = ProcessPair(spuPair, pairListWorkUnitInput, *curSpuManifoldWrapper, selfCollisionsEnabled, curManifoldBufTag, sweepFromSafe, &collisionMemory);

		// Swap which manifold wrapped we pass in so that they get double-buffered.
		curSpuManifoldWrapper = (phSpuManifoldWrapper *)(ISelectI(swapMask, (int)curSpuManifoldWrapper, (int)curSpuManifoldWrapper ^ (int)&spuManifoldWrapper0 ^ (int)&spuManifoldWrapper1));
		curManifoldBufTag = ISelectI(swapMask, curManifoldBufTag, curManifoldBufTag ^ ManifoldBufferTag0 ^ ManifoldBufferTag1);

		spuPair = pairFetcher.GetNextPair();
	}

	sysDmaWaitTagStatusAll(DMA_MASK(ManifoldBufferTag0) | DMA_MASK(ManifoldBufferTag1));

	phPool<phCompositePointers>::ShutdownSpu();
	phPool<phContact>::ShutdownSpu();
	phPool<phManifold>::ShutdownSpu();

#if TRACK_COLLISION_TYPE_PAIRS
	SendTypePairTableToMainMemory(wui.m_typePairTable);
#endif
}

}	// namespace rage

#endif // __SPU
