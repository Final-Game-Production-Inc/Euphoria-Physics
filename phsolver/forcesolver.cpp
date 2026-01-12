// 
// physics/forcesolver.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "system/dma.h"

#include "forcesolver.h"

#include "contactmgr.h"
#include "forcesolverartjoints.h"
#include "forcesolvertables.h"
#include "spucachearray.h"

#include "phcore/phmath.h"
#include "physics/colliderdispatch.h"
#include "physics/overlappingpairarray.h"
#include "profile/element.h"
#include "system/barrier.h"
#include "grprofile/pix.h"
#include "vectormath/classes.h"

#if __SPU
#include "pharticulated/spudma.h"
#include "system/dma.h"
#include "system/ppu_symbol.h"
#include <cell/spurs/queue.h>
#include <cell/spurs/semaphore.h>
#endif

#if RSG_ORBIS && ORBIS_USE_POSIX_SEMAPHORES
#include <semaphore.h> // for phIpcQuerySema (used only in asserts and debug code)
#endif

#include "pharticulated/articulatedcollider.h"

SOLVER_OPTIMISATIONS()

namespace rage {

#if __ASSERT
const size_t phForceSolver::COLLIDER_HEAP_SIZE = 120 * 1024;
#else
const size_t phForceSolver::COLLIDER_HEAP_SIZE = 140 * 1024;
#endif

phForceSolverGlobals phForceSolver::sm_Globals;

#if HACK_GTA4_ADD_FORCER_SOLVER_DEBUG_RECORD
phForceSolverDebugRecord phForceSolver::sm_aForceSolverRecords[MAX_NUM_FORCE_SOLVER_DEBUG_RECORDS];
int phForceSolver::sm_iNumForceSolverRecords=0;
bool phForceSolver::sm_bActivateDebugRecords=false;
void phForceSolver::AddForceSolverDebugRecord(const phForceSolverDebugRecord& record)
{
	if(sm_bActivateDebugRecords)
	{
		//Assertf(sm_iNumForceSolverRecords<MAX_NUM_FORCE_SOLVER_DEBUG_RECORDS, "Out of bounds");
		if(sm_iNumForceSolverRecords<MAX_NUM_FORCE_SOLVER_DEBUG_RECORDS)
		{
			sm_aForceSolverRecords[sm_iNumForceSolverRecords]=record;
			sm_iNumForceSolverRecords++;
		}
	}
}

#endif


namespace phContactStats
{
	EXT_PF_TIMER(PreResponse);
	EXT_PF_TIMER(PreResponseTnread);
	EXT_PF_TIMER(PreResponsePhase);
	EXT_PF_TIMER(ApplyImpulse);
	EXT_PF_TIMER(ApplyImpulseThread);
	EXT_PF_TIMER(ApplyImpulsePhase);
	EXT_PF_TIMER(Sync);
}

using namespace phContactStats;

#if !__SPU
void phForceSolver::IterateOneConstraint(phManifold& manifold, ConstraintTable functionTable)
{
	int tableRow    = GetColliderTypeIndex(manifold.GetColliderA()) * phManifold::NUM_CONSTRAINT_TYPES;
	int tableColumn = GetColliderTypeIndex(manifold.GetColliderB());

	if (manifold.GetColliderA() && manifold.GetInstanceA() && manifold.GetInstanceA() == manifold.GetInstanceB() && functionTable != g_PreResponseConstraintTable)
	{
		// This manifold is a self-collision, so put the articulated body joint limits in the solver.
		if (m_ComputePushesAndTurns)
		{
			ApplyImpulseAndPushArtJoint(manifold, sm_Globals);
		}
		else
		{
			ApplyImpulseArtJoint(manifold, sm_Globals);
		}
	}

	if (manifold.CompositeManifoldsEnabled())
	{
		// This manifold is a composite, containing a collection of collision manifolds.
		// Put all of the manifolds in this composite manifold in the solver.
		for (int manifoldIndex = 0; manifoldIndex < manifold.GetNumCompositeManifolds(); ++manifoldIndex)
		{
			if (manifoldIndex + 1 < manifold.GetNumCompositeManifolds())
			{
				PrefetchObject(manifold.GetCompositeManifold(manifoldIndex + 1));
			}

			// Assume that this component manifold is a collision (not a constraint).
			phManifold* componentManifold = manifold.GetCompositeManifold(manifoldIndex);
			functionTable[tableRow][tableColumn](*componentManifold, sm_Globals);
		}
	}
	else
	{
		// Put this manifold in the solver, assuming that all of its contacts have the same constraint type
		// (if it's a constraint instead of a collision, there should be only one contact).
		functionTable[tableRow + manifold.GetConstraintType()][tableColumn](manifold, sm_Globals);
	}
}

#if __WIN32 || RSG_ORBIS // Can't do this on PPU because the constraint table is full of SPU pointers. Instead, the PPU uses SolveSingleThreaded
void phForceSolver::IterateConstraints(u32 phase, bool preResponse)
{
	ConstraintTable* functionTable = preResponse ? &g_PreResponseConstraintTable : (m_ComputePushesAndTurns ? &g_ApplyImpulseAndPushConstraintTable : &g_ApplyImpulseConstraintTable);
	ParallelGroup& paraGroup = m_ParallelGroups[phase % m_NumPhases];

	for (int pairIndex = 0; pairIndex < paraGroup.numManifolds; ++pairIndex)
	{
		PER_MANIFOLD_SOLVER_METRICS_ONLY(sysTimer timer;)

		// Prefetch, saves about 10% of execution time in the 1000 objects sample_bench test case
		if (pairIndex + 2 < paraGroup.numManifolds)
		{
			phManifold* manifold = paraGroup.manifolds[pairIndex + 2];
			PrefetchObject(manifold);
		}

		if (pairIndex + 1 < paraGroup.numManifolds)
		{
			phManifold* manifold = paraGroup.manifolds[pairIndex + 1];
			if (phCollider* colliderA = manifold->GetColliderA())
			{
				PrefetchObject(colliderA);
			}
			if (phCollider* colliderB = manifold->GetColliderB())
			{
				PrefetchObject(colliderB);
			}
			manifold->PrefetchAllContactPoints();
		}

		phManifold* manifold = paraGroup.manifolds[pairIndex];

		IterateOneConstraint(*manifold, *functionTable);

#if PER_MANIFOLD_SOLVER_METRICS
		if (m_ComputePushesAndTurns)
		{
			manifold->SetPushSolveTime(timer.GetTickTime());
		}
		else
		{
			manifold->SetVelocitySolveTime(timer.GetTickTime());
		}
#endif // PER_MANIFOLD_SOLVER_METRICS
	}
}
#endif // __WIN32
#else // __SPU

class phManifoldDmaBufferExample
{
	phManifold m_Manifold;
	phManifold m_CompositeManifold;
	phManifold::DmaPlan m_SecondManifoldDmaPlan;
	phCompositePointers m_CompositePointers;
	phContact m_Contacts[phManifold::MANIFOLD_CACHE_SIZE];
	Mat33V m_ConstraintMatrix;
};

class phCompositeManifoldDmaBufferExample
{
	phManifold m_Manifold;
	phContact m_Contacts[phManifold::MANIFOLD_CACHE_SIZE];
};

#define TRACK_SOLVE_TIME 0

#if TRACK_SOLVE_TIME
u32 g_ConstraintFuncTime;
#endif

// Why is this in a separate function? To avoid an ICE...
void CallConstraintFunction(void* fragBuffer, phManifold* manifold, const phForceSolverGlobals& globals)
{
#if TRACK_SOLVE_TIME
	u32 now = ~spu_readch(SPU_RdDec);
#endif
	((phForceSolver::ConstraintFunc)fragBuffer)(*manifold, globals);
#if TRACK_SOLVE_TIME
	g_ConstraintFuncTime += ~spu_readch(SPU_RdDec) - now;
#endif
}

phManifold::DmaPlan* phForceSolver::GetPlanForManifold(phManifold* manifold)
{
	u32 index = manifold - m_ManifoldArrayStart;
	return &m_DmaPlanArrayStart[index];
}

void phForceSolver::IterateConstraints(u32 phase, bool preResponse)
{
#if TRACK_SOLVE_TIME
	u32 setupNow = ~spu_readch(SPU_RdDec);
#endif

	ParallelGroup paraGroup;
	sysDmaGet(&paraGroup, uint64_t(m_ParallelGroups + (phase % m_NumPhases)), sizeof(ParallelGroup),DMA_TAG(1));

	// Storage for the DMA plans
	u8 planStorage[4][sizeof(phManifold::DmaPlan)] ;
	u8 objectStorage[4][sizeof(phManifoldDmaBufferExample)] ;
	sysDmaPlanQuadBuffer manifoldPlans(QUAD_ARRAY_PARAM((sysDmaPlan*)planStorage), QUAD_ARRAY_PARAM(objectStorage), DMA_TAG(15));

	u8 colliderPlanStorageA[4][sizeof(phArticulatedCollider::DmaPlan)] ;
	sysDmaPlanQuadBuffer colliderPlansA(QUAD_ARRAY_PARAM((sysDmaPlan*)colliderPlanStorageA), DMA_TAG(19));

	u8 colliderPlanStorageB[4][sizeof(phArticulatedCollider::DmaPlan)] ;
	sysDmaPlanQuadBuffer colliderPlansB(QUAD_ARRAY_PARAM((sysDmaPlan*)colliderPlanStorageB), DMA_TAG(23));

	// Heap for the colliders
	u8 colliderHeap[COLLIDER_HEAP_SIZE] ALIGNED(128);

	// Storage and management for the fragment cache buffers
	u8 fragBufferStorage[3][m_MaxFragSize] ALIGNED(128);
	u8* fragBuffers[3] = { fragBufferStorage[0], fragBufferStorage[1], fragBufferStorage[2] };
	u32 fragTags[3] = { DMA_TAG(10), DMA_TAG(11), DMA_TAG(12) };
	u32 fragMasks[3] = { DMA_MASK(10), DMA_MASK(11), DMA_MASK(12) };
	int currentFrag = 0;
	bool reuseLastFrag = false;

	// Initialization was fine, but we can't proceed until we have the parallel groups
	sysDmaWaitTagStatusAll(DMA_MASK(1));

	if(Unlikely(paraGroup.numManifolds == 0)) {
		return;
	}

	// Start the fetch of the first plans
	manifoldPlans.KickPlanGet(0, paraGroup.manifoldDmaPlans[0], sizeof(phManifold::DmaPlan));
	colliderPlansA.KickPlanGet(0, paraGroup.colliderADmaPlans[0], paraGroup.colliderADmaPlanSizes[0]);
	colliderPlansB.KickPlanGet(0, paraGroup.colliderBDmaPlans[0], paraGroup.colliderBDmaPlanSizes[0]);

	// ...and code frag
	u32** codeFrags;
	u32* codeFragSizes;
	if (Unlikely(preResponse))
	{
		codeFrags = paraGroup.preResponseFrag;
		codeFragSizes = paraGroup.preResponseFragSize;
	}
	else
	{
		codeFrags = paraGroup.applyImpulseFrag;
		codeFragSizes = paraGroup.applyImpulseFragSize;
	}
	u32* codeFrag = codeFrags[0];
	u32 codeFragSize = codeFragSizes[0];
	Assert(m_MaxFragSize >= codeFragSize);
	sysDmaLargeGet(fragBuffers[0], (u64)codeFrag, RoundUp<16>(codeFragSize), fragTags[0]);

	if (Likely(paraGroup.numManifolds > 1))
	{	
		// Start the fetch of the second plans
		manifoldPlans.KickPlanGet(1, paraGroup.manifoldDmaPlans[1], sizeof(phManifold::DmaPlan));
		colliderPlansA.KickPlanGet(1, paraGroup.colliderADmaPlans[1], paraGroup.colliderADmaPlanSizes[1]);
		colliderPlansB.KickPlanGet(1, paraGroup.colliderBDmaPlans[1], paraGroup.colliderBDmaPlanSizes[1]);

		// ...and code frag
		u32* codeFrag = codeFrags[1];
		if (Unlikely(codeFrag))
		{
			u32 codeFragSize = codeFragSizes[1];
			Assert(m_MaxFragSize >= codeFragSize);
			sysDmaLargeGet(fragBuffers[1], (u64)codeFrag, RoundUp<16>(codeFragSize), fragTags[1]);
		}
	}

	// Start the get of the first plans' objects
	phManifold* nextManifold = (phManifold*)manifoldPlans.KickObjectGet(0);

	phCollider* nextColliderA = NULL;
	if (Likely(paraGroup.colliderADmaPlans[0]))
	{
		Assert(paraGroup.colliderAOffsets[0] + sizeof(phCollider) < COLLIDER_HEAP_SIZE);
		nextColliderA = (phCollider*)(paraGroup.colliderAOffsets[0] + colliderHeap);
		colliderPlansA.KickObjectGetBuffer(0, nextColliderA);
	}

	phCollider* nextColliderB = NULL;
	if (Likely(paraGroup.colliderBDmaPlans[0]))
	{
		Assert(paraGroup.colliderBOffsets[0] + sizeof(phCollider) < COLLIDER_HEAP_SIZE);
		nextColliderB = (phCollider*)(paraGroup.colliderBOffsets[0] + colliderHeap);
		colliderPlansB.KickObjectGetBuffer(0, nextColliderB);
	}
	
#if TRACK_SOLVE_TIME
	u32 setupTime = ~spu_readch(SPU_RdDec) - setupNow;
	Displayf("Setup time: %d", setupTime);

	u32 iterateNow = ~spu_readch(SPU_RdDec);
#endif

	for (int pairIndex = 0; pairIndex < paraGroup.numManifolds; ++pairIndex)
	{
		PER_MANIFOLD_SOLVER_METRICS_ONLY(sysTimer timer;)

		manifoldPlans.WaitForObjectPrevPut();
		colliderPlansA.WaitForObjectPrevPut();
		colliderPlansB.WaitForObjectPrevPut();

		int nextNextIndex = pairIndex + 2;
		if (Likely(nextNextIndex < paraGroup.numManifolds))
		{	
			// Fetch plans two pairs in advance
			manifoldPlans.KickPlanGet(2, paraGroup.manifoldDmaPlans[nextNextIndex], sizeof(phManifold::DmaPlan));
			colliderPlansA.KickPlanGet(2, paraGroup.colliderADmaPlans[nextNextIndex], paraGroup.colliderADmaPlanSizes[nextNextIndex]);
			colliderPlansB.KickPlanGet(2, paraGroup.colliderBDmaPlans[nextNextIndex], paraGroup.colliderBDmaPlanSizes[nextNextIndex]);

			// ...and code frag
			u32* codeFrag = codeFrags[nextNextIndex];
			if (Unlikely(codeFrag))
			{
				u32 codeFragSize = codeFragSizes[nextNextIndex];
				Assert(m_MaxFragSize >= codeFragSize);
				int fragIndex = (currentFrag + 2) % 3;
				// May need to wait on this tag from a previous iteration (EXTREMELY unlikely that this will actually wait, but it is not strictly impossible)
				sysDmaWaitTagStatusAll(fragMasks[fragIndex]);
				sysDmaLargeGet(fragBuffers[fragIndex], (u64)codeFrag, RoundUp<16>(codeFragSize), fragTags[fragIndex]);
			}
		}

		phCollider* colliderA = nextColliderA;
		phCollider* colliderB = nextColliderB;
		phManifold* manifold = nextManifold;

		int nextIndex = pairIndex + 1;
		bool anyPairsLeft = nextIndex < paraGroup.numManifolds;
		if (Likely(anyPairsLeft))
		{	
			// Kick gets for the objects for the next pair
			nextManifold = (phManifold*)manifoldPlans.KickObjectGet(1);

			if (Unlikely(paraGroup.colliderADmaPlans[nextIndex]))
			{
				nextColliderA = (phCollider*)(paraGroup.colliderAOffsets[nextIndex] + colliderHeap);
				colliderPlansA.KickObjectGetBuffer(1, nextColliderA);
			}
			else
			{
				nextColliderA = NULL;
			}

			if (Unlikely(paraGroup.colliderBDmaPlans[nextIndex]))
			{
				nextColliderB = (phCollider*)(paraGroup.colliderBOffsets[nextIndex] + colliderHeap);
				colliderPlansB.KickObjectGetBuffer(1, nextColliderB);
			}
			else
			{
				nextColliderB = NULL;
			}
		}

		// Wait until the manifold is fully in memory
		manifoldPlans.WaitForObjectGet();
		manifold->SetLsPointersFromMmPointers();

		u32 colliderADmaPlanSize = paraGroup.colliderADmaPlanSizes[pairIndex];
		if (Unlikely(colliderADmaPlanSize && colliderADmaPlanSize != REUSE_COLLIDER))
		{	
			colliderPlansA.WaitForObjectGetBuffer(colliderA); // Wait until colliderA is fully in memory
		}

		u32 colliderBDmaPlanSize = paraGroup.colliderBDmaPlanSizes[pairIndex];
		if (Unlikely(colliderBDmaPlanSize && colliderBDmaPlanSize != REUSE_COLLIDER))
		{	
			colliderPlansB.WaitForObjectGetBuffer(colliderB); // Wait until colliderB is fully in memory
		}

		// Save the mm pointers so we can put them back later
		phCollider* colliderAmm = manifold->GetColliderA();
		phCollider* colliderBmm = manifold->GetColliderB();
		phCollider* setColliderB = colliderAmm == colliderBmm ? colliderA : colliderB;

		if ((colliderA != NULL || colliderB != NULL)
			// Abort if we had a collider during SplitPairs, but it has been removed during PreComputeImpacts
			&& (!colliderA || colliderAmm)
			&& (!colliderB || colliderBmm))
		{
			// Push the local store colliders into the manifold
			manifold->SetColliderA(colliderA);
			manifold->SetColliderB(setColliderB);

			if (Unlikely(colliderAmm == colliderBmm && !preResponse))
			{
				// Self collision, process joints
#if SPU_FORCE_SOLVER_USE_PUSH
				ApplyImpulseAndPushArtJoint(*manifold, sm_Globals);
#else
				ApplyImpulseArtJoint(*manifold, sm_Globals);
#endif
			}

			if (Likely(manifold->CompositeManifoldsEnabled()))
			{
				int numCompositeManifolds = manifold->GetNumCompositeManifolds();

				if (Likely(numCompositeManifolds > 0))
				{
					// Like magic, the first composite manifold is already in memory (see phManifold::GenerateDmaPlan)
					phManifold* compositeManifoldLS = manifold->GetCompositeManifold(0);

					u8 compositePlanStorage[4][sizeof(phManifold::DmaPlan)] ;
					u8 objectStorage[4][sizeof(phCompositeManifoldDmaBufferExample)] ;
					sysDmaPlanQuadBuffer compositeManifoldPlans(QUAD_ARRAY_PARAM((sysDmaPlan*)compositePlanStorage), QUAD_ARRAY_PARAM(objectStorage),  DMA_TAG(11));

					phManifold* secondManifold = NULL;
					sysDmaPlan* secondManifoldDmaPlan = NULL;
					if (Likely(numCompositeManifolds > 1))
					{
						// Start the DMA of the second manifold
						secondManifoldDmaPlan = manifold->GetSecondManifoldDmaPlan();
						secondManifold = (phManifold*)compositeManifoldPlans.GetPreviousObject();
						secondManifoldDmaPlan->RelocateToLs();
						secondManifoldDmaPlan->KickDMAGet(secondManifold, DMA_TAG(10));

						if (Likely(numCompositeManifolds > 2))
						{
							// Start the DMA of the plan for the third manifold
							compositeManifoldPlans.KickPlanGet(0, GetPlanForManifold(manifold->GetCompositeManifold(2)), sizeof(phManifold::DmaPlan));
						}
					}

					compositeManifoldLS->SetLsPointersFromMmPointers();
					compositeManifoldLS->SetColliderA(colliderA);
					compositeManifoldLS->SetColliderB(setColliderB);

					// Once we make sure we have the code frag, let's process that constraint
					if (Unlikely(!reuseLastFrag))
					{
						sysDmaWaitTagStatusAll(fragMasks[currentFrag]);
						spu_sync_c();
						reuseLastFrag = true;
					}
					CallConstraintFunction(fragBuffers[currentFrag], compositeManifoldLS, sm_Globals);

					compositeManifoldLS->SetColliderA(colliderAmm);
					compositeManifoldLS->SetColliderB(colliderBmm);

					// Note that we don't have to DMA the first manifold back, since it will be carried back with its parent by the same plan that brought it here

					if (Likely(numCompositeManifolds > 1))
					{
						phManifold* nextCompositeManifold = NULL;
						if (Likely(numCompositeManifolds > 2))
						{
							if (Likely(numCompositeManifolds > 3))
							{
								// Start the DMA of the plan for the fourth manifold
								compositeManifoldPlans.KickPlanGet(1, GetPlanForManifold(manifold->GetCompositeManifold(3)), sizeof(phManifold::DmaPlan));
							}

							// Start the DMA of the third manifold
							nextCompositeManifold = (phManifold*)compositeManifoldPlans.KickObjectGet(0);
						}

						// Make sure the second manifold is fully in memory
						sysDmaWaitTagStatusAll(DMA_MASK(10));
						secondManifoldDmaPlan->FixupPointersToLs(secondManifold);

						secondManifold->SetLsPointersFromMmPointers();
						secondManifold->SetColliderA(colliderA);
						secondManifold->SetColliderB(setColliderB);

						// The process that constraint!
						CallConstraintFunction(fragBuffers[currentFrag], secondManifold, sm_Globals);

						secondManifold->SetColliderA(colliderAmm);
						secondManifold->SetColliderB(colliderBmm);

						// And send that manifold back to main memory
						secondManifoldDmaPlan->FixupPointersToMm(secondManifold);
						secondManifoldDmaPlan->KickDMAPut(secondManifold, DMA_TAG(14));

						for (int manifoldIndex = 2; manifoldIndex < numCompositeManifolds; ++manifoldIndex)
						{
							// Finally we are in the inner loop!
							compositeManifoldLS = nextCompositeManifold;

							if (Likely(manifoldIndex + 1 < numCompositeManifolds))
							{
								int nextNextCompositeIndex = manifoldIndex + 2;
								if (Likely(nextNextCompositeIndex < numCompositeManifolds))
								{
									// Get the plan for the object after next
									compositeManifoldPlans.KickPlanGet(2, GetPlanForManifold(manifold->GetCompositeManifold(nextNextCompositeIndex)), sizeof(phManifold::DmaPlan));
								}

								// Otherwise just start the DMA of the next object
								nextCompositeManifold = (phManifold*)compositeManifoldPlans.KickObjectGet(1);
							}

							// Wait for the current object
							compositeManifoldPlans.WaitForObjectGet();
							compositeManifoldLS->SetColliderA(colliderA);
							compositeManifoldLS->SetColliderB(setColliderB);

							compositeManifoldLS->SetLsPointersFromMmPointers();

							// Process the constraint!
							CallConstraintFunction(fragBuffers[currentFrag], compositeManifoldLS, sm_Globals);

							compositeManifoldLS->SetColliderA(colliderAmm);
							compositeManifoldLS->SetColliderB(colliderBmm);

							// And send the composite manifold back to main memory
							compositeManifoldPlans.KickObjectPut();
							compositeManifoldPlans.RotateBuffersObject();
						}

						if (Unlikely(secondManifoldDmaPlan))
						{
							// If the second manifold was being sent to main memory, wait for it to complete here so we can prepare the DMA plan to be DMAed back
							// (of course we didn't change the DMA plan, but it's in the DMA plan for the parent manifold, so it will get send back until we get read-only objects build into the DMA plan)
							sysDmaWaitTagStatusAll(DMA_MASK(14));
							secondManifoldDmaPlan->RelocateToMm();
						}
					}
				}
			}
			else
			{
				// Non-composite manifolds are nice and simple
				if (Unlikely(!reuseLastFrag))
				{
					sysDmaWaitTagStatusAll(fragMasks[currentFrag]);
					spu_sync_c();
					reuseLastFrag = true;
				}
				CallConstraintFunction(fragBuffers[currentFrag], manifold, sm_Globals);
			}

			manifold->SetColliderA(colliderAmm);
			manifold->SetColliderB(colliderBmm);
		}

#if PER_MANIFOLD_SOLVER_METRICS
#if SPU_FORCE_SOLVER_USE_PUSH
		manifold->SetPushSolveTime(timer.GetTickTime());
#else // SPU_FORCE_SOLVER_USE_PUSH
		manifold->SetVelocitySolveTime(timer.GetTickTime());
#endif // SPU_FORCE_SOLVER_USE_PUSH
#endif // PER_MANIFOLD_SOLVER_METRICS

		// DMA the manifold back
		manifoldPlans.KickObjectPut();

		if (Unlikely(anyPairsLeft && codeFragSizes[nextIndex]))
		{
			currentFrag = (currentFrag + 1) % 3;
			reuseLastFrag = false;
		}
		else
		{
			// Rotate the frag buffers
			u32 nextIndex = (currentFrag + 1) % 3;
			u32 nextNextIndex = (currentFrag + 2) % 3;
			u8* tempBuffer = fragBuffers[nextIndex];
			fragBuffers[nextIndex] = fragBuffers[nextNextIndex];
			fragBuffers[nextNextIndex] = tempBuffer;

			u32 tempTag = fragTags[nextIndex];
			fragTags[nextIndex] = fragTags[nextNextIndex];
			fragTags[nextNextIndex] = tempTag;

			u32 tempMask = fragMasks[nextIndex];
			fragMasks[nextIndex] = fragMasks[nextNextIndex];
			fragMasks[nextNextIndex] = tempMask;
		}

		// Rotate the parent manifold buffers
		manifoldPlans.RotateBuffersObject();

		// If the collider is being repeated to the next pair, only partially rotate the buffers
		if (Likely(anyPairsLeft && paraGroup.colliderADmaPlanSizes[nextIndex] == REUSE_COLLIDER))
		{
			colliderPlansA.RotateBuffersKeepCurrent();

			Assert(!nextColliderA);
			nextColliderA = colliderA;
		}
		else
		{
			// If we had a collider before, send it back to main memory now; we're done with it
			if (Likely(colliderA))
			{
				colliderPlansA.KickObjectPutBuffer(colliderA);
			}

			// Rotate the collider buffers
			colliderPlansA.RotateBuffers();
		}

		// If the collider is being repeated to the next pair, only partially rotate the buffers
		if (anyPairsLeft && paraGroup.colliderBDmaPlanSizes[nextIndex] == REUSE_COLLIDER)
		{
			colliderPlansB.RotateBuffersKeepCurrent();

			Assert(!nextColliderB);
			nextColliderB = colliderB;
		}
		else
		{
			// If we had a collider before, send it back to main memory now; we're done with it
			if (Unlikely(colliderB))
			{
				colliderPlansB.KickObjectPutBuffer(colliderB);
			}

			// Rotate the collider buffers
			colliderPlansB.RotateBuffers();
		}
	}

	// Wait for a few more things to finish writing back before our phase is complete
	manifoldPlans.WaitForObjectPut();
	colliderPlansA.WaitForObjectPut();
	colliderPlansB.WaitForObjectPut();

	sysDmaWaitTagStatusAll(SYS_DMA_MASK_ALL);

#if TRACK_SOLVE_TIME
	u32 iterateTime = ~spu_readch(SPU_RdDec) - iterateNow;
	Displayf("iterate time: %d", iterateTime);
#endif
}
#endif

#if __SPU
u32 GetStack()
{
	vec_uint4 sp;
	asm volatile("lr %0, $1" : "=r" (sp)); 
	return spu_extract(sp,0);
}
#endif

int phIpcQuerySema(sysIpcSema sema)
{
#if RSG_ORBIS && ORBIS_USE_POSIX_SEMAPHORES
	int value;
	sem_getvalue((sem_t*)sema, &value);
	return value;
#else
	(void)sema;
	return 0;
#endif
}

int phForceSolver::AllocatePhase()
{
	// The sema will be signalled once every time a new safe phase opens up. It will be kicked off by the main processor as
	// much as dependencies allow, as determined by SplitPairs, so that that many threads can begin working, and be incremented
	// every time the oldest phase is retired.
	sysIpcWaitSema(m_PhaseReleaseSema);

	u32 phase = sysInterlockedIncrement(m_PhaseAllocateAtom) - 1;

#if TRACK_PHASE_RELEASE_SEMA_COUNT
	Displayf("alloc: phase %d sema %d", phase, phIpcQuerySema(m_PhaseReleaseSema));
#endif // TRACK_PHASE_RELEASE_SEMA_COUNT

	Assert(phase < 65535);

	//Displayf("Allocating phase %d", phase);

	return phase;
}

const int UNKNOWN_PHASE = -1;

int phForceSolver::ReleasePhase(u32 phase)
{
	//Displayf("Releasing phase %d/%d", phase, totalPhases);

	// This boolean will become true if we successfully write our change back atomically, so we can get on to the next phase
	bool success = false;
	u32 stopper = 8;

	// If we would otherwise release the semaphore just once, we'll grab a new phase to process next instead. That way we can get
	// started on it without waiting on a semaphore. This collapses the signal with the next wait. When that happens this variable
	// will return carrying the next phase index to the main phase loop.
	int nextPhase = UNKNOWN_PHASE;

	while (!success)
	{
		// Make a copy of what we think the phase release atom is, so that if someone else modifies it in the mean time
		// we'll know we have to start the whole process again.
		u64 phaseReleaseAtom = sysInterlockedRead(m_PhaseReleaseAtom);

		// The current index is stored in the bottom 32 bits of the atom
		u64 phaseReleaseIndex = phaseReleaseAtom & 0xffffffffUL;

		// See if we are releasing the index that we need to be able to move the semaphore
		if (phaseReleaseIndex == phase)
		{
			// The out of order buffer is the upper 32 bits of the atom. It is a bit array telling which of the next 31 phases
			// were actually already completed out of order.
			u64 outOfOrderBuffer = phaseReleaseAtom >> 32;

			// Even if the ooo buffer has nothing for us we'll kick the sema for ourselves
			u32 numToRelease = m_ParallelGroups[(phaseReleaseIndex++) % m_NumPhases].phasesToRelease;

			// For each bit set starting at the LSB until there is a zero, we  will kick the sema.
			u32 releasedPhases = 0;
			while (outOfOrderBuffer & 1)
			{
				numToRelease += m_ParallelGroups[(phaseReleaseIndex++) % m_NumPhases].phasesToRelease;
				++releasedPhases;

				outOfOrderBuffer >>= 1;
			}

			// Put the atom back together and hope we finished this before another thread modified the atom
			u64 newPhaseReleaseAtom = phaseReleaseIndex | (outOfOrderBuffer << 31);

			// If the exchage fails, we'll go back to the beginning of the function and try again
			if (sysInterlockedCompareExchange(m_PhaseReleaseAtom, newPhaseReleaseAtom, phaseReleaseAtom) == phaseReleaseAtom)
			{
				sys_lwsync();

				//Displayf("need to signal %d times", numToRelease);

				// If the exchange succeeds, kick the sema once for ourselves and once for every bit we erased
				if (numToRelease > 0)
				{
					// Allocate the next phase immediately, if we're releasing any phases
					nextPhase = sysInterlockedIncrement(m_PhaseAllocateAtom) - 1;

					// Only signal numToRelease - 1 times, because we are keeping one for ourselves
					if (numToRelease > 1)
					{
						sysIpcSignalSema(m_PhaseReleaseSema, numToRelease - 1);
					}

#if TRACK_PHASE_RELEASE_SEMA_COUNT
					Displayf("release: phase (%d-%d) sema %d", phase, phase + releasedPhases, phIpcQuerySema(m_PhaseReleaseSema));
#endif // TRACK_PHASE_RELEASE_SEMA_COUNT
				}

				// Our work here is done, go on to the next phase
				success = true;
			}
		}
		else
		{
			// We are being released out of order, put ourselves into the ooo buffer
			Assert(phase - phaseReleaseIndex > 0);

			u64 oooMask = u64(1) << (31 + phase - phaseReleaseIndex);

			// Compute a new phase release atom with our bit set
			u64 newPhaseReleaseAtom = phaseReleaseAtom | oooMask;

			// If the exchage fails, we'll go back to the beginning of the function and try again
			if (sysInterlockedCompareExchange(m_PhaseReleaseAtom, newPhaseReleaseAtom, phaseReleaseAtom) == phaseReleaseAtom)
			{
				sys_lwsync();

				// Our work here is done, go on to the next phase
				success = true;
			}
		}

		if (!success)
		{
			volatile u32 spinner = 0;
			while (spinner < stopper)
				++spinner;
			if (stopper < 1024)
				stopper <<= 1;
		}
	}

	return nextPhase;
}

#ifndef SPU_FORCE_SOLVER_USE_PUSH
#define SPU_FORCE_SOLVER_USE_PUSH 0
#endif

void phForceSolver::InitGlobals()
{
	ScalarV invTimeStep = Invert(ScalarVFromF32(m_TimeStep));
	sm_Globals.separateBias = ScalarVFromF32(m_SeparateBias) * ScalarVFromF32(m_TimeStep * MAGIC_SEPARATION_BIAS_SYNC_NUMBER);
	sm_Globals.invTimeStep = invTimeStep;
	sm_Globals.allowedPenetration = ScalarVFromF32(m_AllowedPenetration);
	sm_Globals.halfAllowedPenetration = ScalarVFromF32(m_AllowedPenetration) * ScalarV(V_HALF);
	sm_Globals.allowedAnglePenetration = ScalarVFromF32(m_AllowedAnglePenetration);
	sm_Globals.minBounce = -ScalarVFromF32(m_MinBounce);
#if TURN_CLAMPING
	sm_Globals.maxTurn = ScalarV(m_MaxTurn);
#endif
	Assert(!m_ClearWarmStart || !m_ApplyWarmStart);
	sm_Globals.applyWarmStart = m_ApplyWarmStart;
	sm_Globals.clearWarmStart = m_ClearWarmStart;
	sm_Globals.baumgarteJointLimitTerm = m_BaumgarteJointLimitTerm;

	sm_Globals.GetInverseMassMatrix = &phArticulatedCollider::GetInverseMassMatrix;
	sm_Globals.GetInverseMassMatrixSelf = &phArticulatedCollider::GetInverseMassMatrixSelf;
	sm_Globals.GetInverseInertiaMatrix = &phMathInertia::GetInverseInertiaMatrix;
	sm_Globals.Exchange = &phManifold::Exchange;

	sm_Globals.GetAngularVelocity = &phArticulatedBody::GetAngularVelocity;
	sm_Globals.GetLocalVelocity = &phArticulatedBody::GetLocalVelocity;
	sm_Globals.GetLocalVelocityNoProp = &phArticulatedBody::GetLocalVelocityNoProp;
	sm_Globals.ApplyImpulse = &phArticulatedBody::ApplyImpulse;
	sm_Globals.ApplyAngImpulse = &phArticulatedBody::ApplyAngImpulse;

	PS3_ONLY(g_DmaPlanAsList = m_DmaPlanAsList;)
}

#if !__PPU
void phForceSolver::Solve()
{
	InitGlobals();

	u32 totalPhases = m_PreResponse ? m_NumPhases * (1 + m_NumIterations) : 
										m_NumPhases * m_NumIterations;

	Assert(m_NumPhases > 0);
	PF_START(PreResponseTnread);

	PER_MANIFOLD_SOLVER_METRICS_ONLY(sysTimer timer;)

	u32 phaseIndex = AllocatePhase();

	if (m_PreResponse)
	{
		while (phaseIndex < m_NumPhases)
		{
#if PER_MANIFOLD_SOLVER_METRICS
			m_ParallelGroups[phaseIndex].AddWaitTime(timer.GetTickTime());
			timer.Reset();
#endif // PER_MANIFOLD_SOLVER_METRICS

			PF_START(PreResponse);
			IterateConstraints(phaseIndex, true);
			PF_STOP(PreResponse);

#if PER_MANIFOLD_SOLVER_METRICS
			m_ParallelGroups[phaseIndex].AddSolveTime(timer.GetTickTime());
			timer.Reset();
#endif // PER_MANIFOLD_SOLVER_METRICS

			phaseIndex = ReleasePhase(phaseIndex);

			if (phaseIndex == UNKNOWN_PHASE)
			{
				phaseIndex = AllocatePhase();
			}
		}
	}

	PF_STOP(PreResponseTnread);

	PF_START(ApplyImpulseThread);

	while (phaseIndex < totalPhases)
	{
#if PER_MANIFOLD_SOLVER_METRICS
		m_ParallelGroups[phaseIndex].AddWaitTime(timer.GetTickTime());
		timer.Reset();
#endif // PER_MANIFOLD_SOLVER_METRICS

		PF_START(ApplyImpulsePhase);
		IterateConstraints(phaseIndex, false);
		PF_STOP(ApplyImpulsePhase);

#if PER_MANIFOLD_SOLVER_METRICS
		m_ParallelGroups[phaseIndex].AddSolveTime(timer.GetTickTime());
		timer.Reset();
#endif // PER_MANIFOLD_SOLVER_METRICS

		phaseIndex = ReleasePhase(phaseIndex);

		if (phaseIndex == UNKNOWN_PHASE)
		{
			phaseIndex = AllocatePhase();
		}
	}

	// The variable size window introduces a complication for managing the semaphore. The way the threads
	// know to terminate is that they allocate a phase index which is greater than the total number of phases,
	// which requires the semaphore to be signaled an extra time. But the variable size phase window might
	// not be big enough to have an extra signal for each worker thread. So, we sometimes have to signal
	// the sema a few extra times to allow the worker threads to terminate. We can't do this too early (or we'd
	// violate dependencies) so the responsibility falls on the first worker thread to get a phase greater
	// than the total number of phases.
	//
	// See also: the similar comment in phContactMgr::SolveImpulses about this

	//Displayf("reached phase number %d past end", phaseIndex);
	if (phaseIndex == totalPhases && m_ExtraReleasesAtEnd > 0)
	{
		//Displayf("releasing %d extra phases", m_ExtraReleasesAtEnd);

		sysIpcSignalSema(m_PhaseReleaseSema, m_ExtraReleasesAtEnd);

#if TRACK_PHASE_RELEASE_SEMA_COUNT
		Displayf("finish: phase %d sema %d", phaseIndex, phIpcQuerySema(m_PhaseReleaseSema));
#endif // TRACK_PHASE_RELEASE_SEMA_COUNT
	}

#if PER_MANIFOLD_SOLVER_METRICS
	m_ParallelGroups[phaseIndex].AddWaitTime(timer.GetTickTime());
	timer.Reset();
#endif // PER_MANIFOLD_SOLVER_METRICS

	PF_STOP(ApplyImpulseThread);
}
#endif

void phForceSolver::SolveSingleThreaded()
{
	InitGlobals();

	if (m_PreResponse)
	{
		for (int pairArrayIndex = 0; pairArrayIndex < m_OverlappingPairs->pairs.GetCount(); ++pairArrayIndex)
		{
			phTaskCollisionPair& taskPair = m_OverlappingPairs->pairs[pairArrayIndex];
			if (phManifold* manifold = taskPair.manifold)
			{
				// The manifold without any valid contacts can possibly slip to this stage as phContactMgr::SplitPairs doesn't remove them from pair array for single threaded physics
				if(manifold->ObjectsInContact() || manifold->IsSelfCollision())
				{
					IterateOneConstraint(*manifold, g_PreResponseConstraintTable);
				}
			}
		}
	}

	phForceSolver::ConstraintTable* applyTable = m_ComputePushesAndTurns ? &g_ApplyImpulseAndPushConstraintTable : &g_ApplyImpulseConstraintTable;

	for (u32 iteration = 0; iteration < m_NumIterations; ++iteration)
	{
		for (int pairArrayIndex = 0; pairArrayIndex < m_OverlappingPairs->pairs.GetCount(); ++pairArrayIndex)
		{
			phTaskCollisionPair& taskPair = m_OverlappingPairs->pairs[pairArrayIndex];
			if (phManifold* manifold = taskPair.manifold)
			{
				if(manifold->ObjectsInContact() || manifold->IsSelfCollision())
				{
					IterateOneConstraint(*manifold, *applyTable);
				}
			}
		}
	}
}

#if __WIN32 || RSG_ORBIS
void SolveConstraintTask(sysTaskParameters& params)
{
#if !__STATS
	PIXBegin(0, "SolveConstraintsTask");
#endif

	phForceSolver* forceSolver = (phForceSolver*)params.Input.Data;

	forceSolver->Solve();

#if !__STATS
	PIXEnd();
#endif
}
#endif

#if __SPU
void phWorkerThreadMain(sysTaskParameters& params)
{
#if TRACK_SOLVE_TIME
	u32 now = ~spu_readch(SPU_RdDec);

	g_ConstraintFuncTime = 0;
#endif 

	static const int tag = 1;

	phForceSolver forceSolver;
	sysDmaLargeGetAndWait(&forceSolver, (uint64_t)params.Input.Data, sizeof(phForceSolver), DMA_TAG(tag));

	u8 pairArrayBuffer[sizeof(phOverlappingPairArray)] ;
	phOverlappingPairArray* pairArray = (phOverlappingPairArray*)pairArrayBuffer;

	sysDmaGetAndWait(pairArrayBuffer, uint64_t(forceSolver.m_OverlappingPairs), sizeof(phOverlappingPairArray), DMA_TAG(tag));

	forceSolver.m_OverlappingPairs = pairArray;

#if SYS_DMA_DETAILED_TIMING || SYS_DMA_TIMING
	u32 start = ~spu_readch(SPU_RdDec);
#endif

	forceSolver.Solve();

#if SYS_DMA_DETAILED_TIMING || SYS_DMA_TIMING
	u32 time = ~spu_readch(SPU_RdDec) - start;

	if (cellSpursGetCurrentSpuId() == 0)
	{
		Displayf("solve time %d, dma time %d", time, g_DmaWaitTime);

		sysDmaPrintWaitTimes();
	}
#endif

#if TRACK_SOLVE_TIME
	u32 time = ~spu_readch(SPU_RdDec) - now;

	Displayf("total time: %d, constraint %d", time, g_ConstraintFuncTime);
#endif
}
#endif // __SPU

} // namespace rage
