// 
// physics/contacttask.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "contacttask.h"
#include "system/dma.h"

#include "collider.h"
#include "colliderdispatch.h"
#include "manifold.h"
#include "overlappingpairarray.h"

#include "pharticulated/articulatedcollider.h"
#include "phcore/phmath.h"
#include "phcore/pool.h"
#include "phsolver/forcesolverartjoints.h"
#include "phsolver/forcesolvertables.h"
#include "physics/levelnew.h"
#include "physics/simulator.h"
#include "profile/element.h"
#include "system/interlocked.h"
#include "grprofile/pix.h"

#if __SPU
#include <cell/spurs/queue.h>
#include "pharticulated/spudma.h"
#endif

namespace rage {

namespace SimulatorStats
{
	EXT_PF_TIMER(RefreshContactsThread);
}

using namespace SimulatorStats;

#if __SPU
static const int ARTICULATED_SCRATCHPAD_SIZE = 1024 * 65;
static u8 s_ArticulatedScratchpad[2][ARTICULATED_SCRATCHPAD_SIZE] ;
static ArticulatedDmaFull articulatedDma[2];
extern Mat34V* g_LastMatricesBaseAddrMM;
extern u8* g_LevelIdxToLastMatrixMapMM;
#endif

void CallConstraintFunction(void* fragBuffer, phManifold* manifold, const phForceSolverGlobals& globals)
{
	((phForceSolver::ConstraintFunc)fragBuffer)(*manifold, globals);
}

#define BUILD_DMA_PLAN 1

// The job of UpdateContactsTask() is to call RefreshContacts on each manifold that it comes across.
void UpdateContactsTask(sysTaskParameters& params)
{
	PF_FUNC(RefreshContactsThread);
#if !__STATS
	PIXBegin(0, "UpdateContactsTask");
#endif

	bool justRunConstraintFunctions		=						params.UserData[0].asBool;
	ScalarV allowedPenetration   		=						ScalarV(params.UserData[1].asFloat);
	bool computeBounceAndTangent		=						params.UserData[2].asBool;

	u32* atom							= (u32*)                params.UserData[3].asPtr;
	u32 numTasks						=                       params.UserData[4].asInt;
	phTaskCollisionPair* pairs			= (phTaskCollisionPair*)params.UserData[5].asPtr;
	int minManifoldPointLifeTime		=						params.UserData[6].asInt;
	float minBounceF					=						params.UserData[9].asFloat;

	const ScalarV separateBiasMultiplier =						ScalarV(params.UserData[10].asFloat);

#if __SPU
	size_t maxFragSize					=						params.UserData[12].asInt;
	u8* fragBuffer = Alloca(u8, maxFragSize);

#if BUILD_DMA_PLAN
	phManifold* manifoldArrayStart		= (phManifold*)			params.UserData[13].asPtr;
	phManifold::DmaPlan* planArrayStart	= (phManifold::DmaPlan*)params.UserData[14].asPtr;
#endif

	g_LastMatricesBaseAddrMM			= (Mat34V*)				params.UserData[16].asPtr;
	g_LevelIdxToLastMatrixMapMM			= (u8*)					params.UserData[17].asPtr;
#endif

	ScalarV timeStep					=				ScalarV(params.UserData[18].asFloat);

#if POSITIVE_DEPTH_ONLY && __SPU
	phManifold::sm_PositiveDepthMode	=						params.UserData[19].asInt;
#endif

	phForceSolverGlobals globals;
	globals.calculateBounceAndTangent = BoolV(computeBounceAndTangent);
	globals.minBounce = ScalarV(minBounceF);
	const ScalarV invTimeStep = Invert(ScalarV(timeStep));
	globals.invTimeStep = invTimeStep;
	globals.allowedPenetration = allowedPenetration;
	globals.halfAllowedPenetration = allowedPenetration * ScalarV(V_HALF);
	const ScalarV halfPenetration = globals.halfAllowedPenetration;
	globals.GetInverseInertiaMatrix = &phMathInertia::GetInverseInertiaMatrix;
	globals.Exchange = &phManifold::Exchange;
	globals.GetInverseMassMatrix = &phArticulatedCollider::GetInverseMassMatrix;
	globals.GetInverseMassMatrixSelf = &phArticulatedCollider::GetInverseMassMatrixSelf;
	globals.GetLocalVelocityNoProp = &phArticulatedBody::GetLocalVelocityNoProp;

	// Since we purge composite sub-manifolds with no contact points we need to keep track of which ones should be kept.  When on SPU the pointers tracked here
	//   are PPU pointers.  These values are only used for composite manifolds.
	// TODO: We should get rid of this because I don't think we really need to have an array at all any more, since we can/should just write the contents
	//   of this array directly into the composite pointers object.  We already do that on SPU.
	int numNonEmptyCompositeManifolds;	// Initialized in the loop below.

	phPool<phManifold>* manifoldPool = PHMANIFOLD;

	u32 pairIndex;
	while ((pairIndex = sysInterlockedIncrement(atom) - 1) < numTasks)
	{
		PER_MANIFOLD_SOLVER_METRICS_ONLY(sysTimer timer;)

#if __SPU
		static const int tag = 1;
		phTaskCollisionPair pair;
		sysDmaGet(&pair, uint64_t(&pairs[pairIndex]), sizeof(phTaskCollisionPair), DMA_TAG(tag));
		sysDmaWaitTagStatusAll(DMA_MASK(tag));

		// This can be NULL with EARLY_FORCE_SOLVE because RefreshInstAndColliderPointers might be protecting us from a deleted object
		if (phManifold* manifoldMm = pair.manifold)
		{
			u8 manifoldBuffer[sizeof(phManifold)] ALIGNED(128);
			sysDmaGet(manifoldBuffer, uint64_t(manifoldMm), sizeof(phManifold), DMA_TAG(tag));
			sysDmaWaitTagStatusAll(DMA_MASK(tag));
			phManifold* manifold = (phManifold*)manifoldBuffer;
			pair.manifold = manifold;

			phCollider* collider1Mm = manifold->GetColliderA();
			phCollider* collider2Mm = manifold->GetColliderB();

			phCollider* collider1Ls = NULL;
			static u8 s_ColliderBufferA[sizeof(phArticulatedCollider)];
			if (collider1Mm)
			{
				collider1Ls = (phArticulatedCollider*)s_ColliderBufferA;
				sysDmaGet(collider1Ls, (uint64_t)collider1Mm, sizeof(phArticulatedCollider), DMA_TAG(tag));
			}

			phCollider* collider2Ls = NULL;
			static u8 s_ColliderBufferB[sizeof(phArticulatedCollider)];
			if (collider2Mm)
			{
				if (collider1Mm == collider2Mm)
				{
					collider2Ls = collider1Ls;
					manifold->SetColliderB(collider1Ls);
				}
				else
				{
					collider2Ls = (phArticulatedCollider*)s_ColliderBufferB;
					sysDmaGet(collider2Ls, (uint64_t)collider2Mm, sizeof(phArticulatedCollider), DMA_TAG(tag));
				}
			}

			sysDmaWaitTagStatusAll(DMA_MASK(tag));

#if BUILD_DMA_PLAN
			// Get our main DMA plan going
			phManifold::DmaPlan* dmaPlanMm = (manifoldMm - manifoldArrayStart) + planArrayStart;
			phManifold::DmaPlan dmaPlan;
			dmaPlan.Initialize(dmaPlanMm);
#endif // BUILD_DMA_PLAN

			manifold->SetColliderA(collider1Ls);
			manifold->SetColliderB(collider2Ls);

			phContact contacts[phManifold::MANIFOLD_CACHE_SIZE];
			phContact* contactsLs[phManifold::MANIFOLD_CACHE_SIZE] = { &contacts[0], &contacts[1], &contacts[2], &contacts[3], &contacts[4], &contacts[5] };
			phCompositePointers compositePointersLs;
			if (manifold->CompositeManifoldsEnabled())
			{
				manifold->SetCompositePointersLs(&compositePointersLs);
				manifold->GetCompositePointersFromMm(DMA_TAG(tag));
			}
			else
			{
				manifold->SetContactsLs(contactsLs);
				manifold->GatherContactPointsFromMm(DMA_TAG(tag));
			}

			Mat33V constraintMatrixLs;
			manifold->SetConstraintMatrixLs(&constraintMatrixLs);
			manifold->GetConstraintMatrixFromMm(DMA_TAG(tag));

			sysDmaWaitTagStatusAll(DMA_MASK(tag));

			bool artAllocOK = true;

			if (collider1Ls && collider1Ls->IsArticulated())
			{
				articulatedDma[0].Init(s_ArticulatedScratchpad[0], ARTICULATED_SCRATCHPAD_SIZE);
				artAllocOK = articulatedDma[0].FetchToLs(static_cast<phArticulatedCollider*>(collider1Ls));
			}

			if (artAllocOK && collider2Ls && collider2Ls->IsArticulated())
			{
				articulatedDma[1].Init(s_ArticulatedScratchpad[1], ARTICULATED_SCRATCHPAD_SIZE);

				// if the articulated body types are the same, only fetch the type data once
				phArticulatedCollider *copyCollider = NULL;
				if (collider1Ls && collider1Ls->IsArticulated() &&
					((phArticulatedCollider *) collider1Ls)->GetARTAssetID() >= 0 &&
					((phArticulatedCollider *) collider1Ls)->GetARTAssetID() == ((phArticulatedCollider *) collider2Ls)->GetARTAssetID() &&
					((phArticulatedCollider *) collider1Ls)->GetLOD() == ((phArticulatedCollider *) collider2Ls)->GetLOD())
				{
					copyCollider = static_cast<phArticulatedCollider*>(collider1Ls);
				}

				artAllocOK = articulatedDma[1].FetchToLs(static_cast<phArticulatedCollider*>(collider2Ls), copyCollider);
			}

			if (artAllocOK)
			{
				int tableRow    = GetColliderTypeIndex(collider1Ls) * phManifold::NUM_CONSTRAINT_TYPES;
				int tableColumn = GetColliderTypeIndex(collider2Ls);			

				if (manifold->CompositeManifoldsEnabled())
				{
					Assert(!manifold->IsConstraint());
					Assert(manifold->m_ConstraintMatrix == NULL);
#if 0 // Not currently anything in UpdateContactsArtJoint
					if (collider1Ls == collider2Ls)
					{
						// Self collision, process joints
						UpdateContactsArtJoint(*manifold, globals);
					}
#endif

					numNonEmptyCompositeManifolds = 0;

					u8 compositeManifoldBuffer[sizeof(phManifold)] ALIGNED(128);

					const int numCompositeManifolds = manifold->GetNumCompositeManifolds();

#if BUILD_DMA_PLAN
					// Go ahead and generate the DMA plan for the parent, since it doesn't have any contacts anyway
					manifold->GenerateDmaPlan(dmaPlan, manifoldMm);

					// Note that the composite pointers are not yet computed, but their address will not change so writing them into the DMA plan is fine here
					if (manifold->m_CompositePointers && numCompositeManifolds > 0)
					{
						dmaPlan.AddObject(manifold->m_CompositePointers, true);
						dmaPlan.AddFixup((void**)&(manifold->m_CompositePointers), (void**)&(manifoldMm->m_CompositePointers));
					}
#endif

					for (int manifoldIndex = 0; manifoldIndex < numCompositeManifolds; ++manifoldIndex)
					{					
						phManifold* compositeManifoldMm = manifold->GetCompositeManifold(manifoldIndex);

						sysDmaGet(compositeManifoldBuffer, uint64_t(compositeManifoldMm), sizeof(phManifold), DMA_TAG(tag));
						sysDmaWaitTagStatusAll(DMA_MASK(tag));

						phManifold* compositeManifoldLs = (phManifold*)compositeManifoldBuffer;
						Assert(!compositeManifoldLs->IsConstraint());
						Assert(compositeManifoldLs->m_ConstraintMatrix == NULL);
						compositeManifoldLs->SetColliderA(collider1Ls);
						compositeManifoldLs->SetColliderB(collider2Ls);

						compositeManifoldLs->SetContactsLs(contactsLs);
						compositeManifoldLs->GatherContactPointsFromMm(DMA_TAG(tag));

						sysDmaWaitTagStatusAll(DMA_MASK(tag));

						spuCodeFragment& frag = g_UpdateContactsSpuConstraintTable[tableRow][tableColumn];
						Assert(maxFragSize >= frag.codeFragSize);
						sysDmaLargeGet(fragBuffer, (u64)frag.pCodeFrag, RoundUp<16>(frag.codeFragSize), DMA_TAG(0));
						sysDmaWait(DMA_MASK(0));
						spu_sync_c();
#if USE_PRECOMPUTE_SEPARATEBIAS
						compositeManifoldLs->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTimeStep);
#endif // USE_PRECOMPUTE_SEPARATEBIAS
						CallConstraintFunction(fragBuffer, compositeManifoldLs, globals);

						compositeManifoldLs->SetColliderA(collider1Mm);
						compositeManifoldLs->SetColliderB(collider2Mm);

						// Build up the list of the manifolds that still have contacts in them.  If there are no contacts left in this manifold, release it.
						if(compositeManifoldLs->GetNumContacts() > 0)
						{
							compositePointersLs.SetManifoldAndPairComponents(numNonEmptyCompositeManifolds, compositeManifoldMm, static_cast<u8>(compositeManifoldLs->GetComponentA()), static_cast<u8>(compositeManifoldLs->GetComponentB()));
							++numNonEmptyCompositeManifolds;
						}
						else
						{
							// We need to DMA the contents of the manifold back to the PPU or else the manifold will look like it still has contacts in it
							//   and they will get 'freed' again by phManifold::Reset() when the manifold gets allocated from the manifold pool again.
							sysDmaPut(compositeManifoldBuffer, uint64_t(compositeManifoldMm), sizeof(phManifold), DMA_TAG(tag));
							sysDmaWaitTagStatusAll(DMA_MASK(tag));

							manifoldPool->Release(compositeManifoldMm);

							// This used to be the end of the loop over the composite manifolds but it's not any more.  We need to make sure that we don't
							//   try and do any of the DMA plan stuff below for this manifold.
							continue;
						}

#if BUILD_DMA_PLAN
						const int finalManifoldIndex = numNonEmptyCompositeManifolds - 1;

						// This has to be out here because the dtor can't run until the DMA wait in the same block below
						phManifold::DmaPlan compositeDmaPlan;

						if (finalManifoldIndex == 0)
						{
							// The first composite manifold we just include directly in the parent manifold DMA plan
							compositeManifoldLs->GenerateDmaPlan(dmaPlan, compositeManifoldMm);
							dmaPlan.AddFixup((void**)&(compositePointersLs.GetManifoldRef(0)), (void**)&manifold->m_CompositePointers->GetManifoldRef(0));
						}
						else
						{
							// The second composite manifold gets its DMA plan written into the parent manifold DMA plan
							sysDmaPlan* compositeDmaPlanMm = (compositeManifoldMm - manifoldArrayStart) + planArrayStart;

							if (finalManifoldIndex == 1)
							{
								// If a second manifold exists, includes its DMA plan in the DMA plan for the parent manifold
								compositePointersLs.SetSecondManifoldDmaPlan(compositeDmaPlanMm);
								dmaPlan.AddObject((phManifold::DmaPlan*&)compositeDmaPlanMm, true);
								dmaPlan.AddFixup((void**)&(compositePointersLs.GetSecondManifoldDmaPlanRef()), (void**)&(manifold->m_CompositePointers->GetSecondManifoldDmaPlanRef()));
							}

							// Generate the composite DMA plan back and write it back to main memory
							// Note that the parent manifold DMA plan must be finished before we call Initialize here, because they share the same m_ObjectInfos array
							compositeDmaPlan.Initialize(compositeDmaPlanMm);
							compositeManifoldLs->GenerateDmaPlan(compositeDmaPlan, compositeManifoldMm);
							compositeDmaPlan.RelocateToMm();
							sysDmaPut(&compositeDmaPlan, (uint64_t)compositeDmaPlanMm, sizeof(phManifold::DmaPlan), DMA_TAG(tag));
						}
#endif

						compositeManifoldLs->ScatterContactPointsToMm(DMA_TAG(tag));

						sysDmaPut(compositeManifoldBuffer, uint64_t(compositeManifoldMm), sizeof(phManifold), DMA_TAG(tag));
						sysDmaWaitTagStatusAll(DMA_MASK(tag));
					}

					// If the list of non-empty manifolds has changed, update the root manifold to reflect that.
					FastAssert(numNonEmptyCompositeManifolds <= numCompositeManifolds);
					FastAssert(numCompositeManifolds <= phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS);

					PER_MANIFOLD_SOLVER_METRICS_ONLY(utimer_t time = timer.GetTickTime();)

#if BUILD_DMA_PLAN
					// We have to send the composite pointers object back to main memory since we will have filled in m_SecondManifoldDmaPlan
					manifold->PutCompositePointersToMm(DMA_TAG(tag));

					if(Likely(numNonEmptyCompositeManifolds != numCompositeManifolds))
					{
						manifold->SetNumCompositeManifolds(numNonEmptyCompositeManifolds);
						manifold->SetColliderA(collider1Mm);
						manifold->SetColliderB(collider2Mm);
						PER_MANIFOLD_SOLVER_METRICS_ONLY(manifold->SetUpdateContactsTime(time);)
						sysDmaPut(manifoldBuffer, uint64_t(manifoldMm), sizeof(phManifold), DMA_TAG(tag));
					}
#if PER_MANIFOLD_SOLVER_METRICS
					else
					{
						manifoldMm->DmaPutContactsTime(time, DMA_TAG(tag + 1));
					}
#endif // PER_MANIFOLD_SOLVER_METRICS

					sysDmaWaitTagStatusAll(DMA_MASK(tag));
#else // BUILD_DMA_PLAN
					if(Likely(numNonEmptyCompositeManifolds != numCompositeManifolds))
					{
						manifold->SetNumCompositeManifolds(numNonEmptyCompositeManifolds);
						manifold->PutCompositePointersToMm(DMA_TAG(tag));

						manifold->SetColliderA(collider1Mm);
						manifold->SetColliderB(collider2Mm);
						PER_MANIFOLD_SOLVER_METRICS_ONLY(manifold->SetUpdateContactsTime(time);)
						sysDmaPut(manifoldBuffer, uint64_t(manifoldMm), sizeof(phManifold), DMA_TAG(tag));
						sysDmaWaitTagStatusAll(DMA_MASK(tag));
					}
#if PER_MANIFOLD_SOLVER_METRICS
					else
					{
						manifoldMm->DmaPutContactsTime(time, DMA_TAG(tag + 1));
					}
#endif // PER_MANIFOLD_SOLVER_METRICS
#endif // BUILD_DMA_PLAN
				}
				else
				{
					if (manifold->IsConstraint() && !justRunConstraintFunctions)
					{
						manifold->RefreshContactPoints(minManifoldPointLifeTime, timeStep);
					}

					spuCodeFragment& frag = g_UpdateContactsSpuConstraintTable[tableRow + manifold->GetConstraintType()][tableColumn];
					Assert(maxFragSize >= frag.codeFragSize);
					sysDmaLargeGet(fragBuffer, (u64)frag.pCodeFrag, RoundUp<16>(frag.codeFragSize), DMA_TAG(0));
					sysDmaWait(DMA_MASK(0));
					spu_sync_c();
#if USE_PRECOMPUTE_SEPARATEBIAS
					manifold->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTimeStep);
#endif // USE_PRECOMPUTE_SEPARATEBIAS
					CallConstraintFunction(fragBuffer, manifold, globals);

					manifold->SetColliderA(collider1Mm);
					manifold->SetColliderB(collider2Mm);

#if BUILD_DMA_PLAN
					// We can generate the DMA plan now for the contact points, since we've done manifold reduction already
					manifold->GenerateDmaPlan(dmaPlan, manifoldMm);
#endif // BUILD_DMA_PLAN

					manifold->PutConstraintMatrixToMm(DMA_TAG(tag));
					manifold->ScatterContactPointsToMm(DMA_TAG(tag));

					PER_MANIFOLD_SOLVER_METRICS_ONLY(manifold->SetUpdateContactsTime(timer.GetTickTime());)

					sysDmaPut(manifoldBuffer, uint64_t(manifoldMm), sizeof(phManifold), DMA_TAG(tag));
					sysDmaWaitTagStatusAll(DMA_MASK(tag));
				}
			}
			else
			{
				Errorf("UpdateContactsTask -- Not enough SPU memory for articulated body.");
			}

#if BUILD_DMA_PLAN
			// Write the main DMA plan back to main memory
			dmaPlan.RelocateToMm();

			sysDmaPutAndWait(&dmaPlan, (uint64_t)dmaPlanMm, sizeof(phManifold::DmaPlan), DMA_TAG(tag));
#endif
		}
#else // !__SPU

		phTaskCollisionPair& pair = pairs[pairIndex];

		// This can be NULL with EARLY_FORCE_SOLVE because RefreshInstAndColliderPointers might be protecting us from a deleted object
		if (phManifold* manifold = pair.manifold)
		{
#if !HACK_GTA4
#if __DEV
			// This code is just a safety check to protect from a crash that has occurred in the past
			// when a manifold had pointers to phInst that had been removed from the level.
			if (PHLEVEL->LegitLevelIndex(manifold->GetLevelIndexA()) && !PHLEVEL->IsNonexistent(manifold->GetLevelIndexA()))
			{
				phInst* instance = PHLEVEL->GetInstance(manifold->GetLevelIndexA());
				Assert(instance == manifold->GetInstanceA());
				manifold->SetInstanceA(instance);
			}
			Assert(manifold->GetInstanceA() == NULL || manifold->GetInstanceA()->GetLevelIndex() == manifold->GetLevelIndexA());

			if (PHLEVEL->LegitLevelIndex(manifold->GetLevelIndexB()) && !PHLEVEL->IsNonexistent(manifold->GetLevelIndexB()))
			{
				phInst* instance = PHLEVEL->GetInstance(manifold->GetLevelIndexB());
				Assert(instance == manifold->GetInstanceB());
				manifold->SetInstanceB(instance);
			}
			Assert(manifold->GetInstanceB() == NULL || manifold->GetInstanceB()->GetLevelIndex() == manifold->GetLevelIndexB());
#endif
#endif

			phCollider* colliderA = manifold->GetColliderA();
			phCollider* colliderB = manifold->GetColliderB();

			if (justRunConstraintFunctions &&
				(!colliderA || !colliderA->GetNeedsUpdateBeforeFinalIterations()) &&
				(!colliderB || !colliderB->GetNeedsUpdateBeforeFinalIterations()))
			{
				continue;
			}

			int tableRow    = GetColliderTypeIndex(colliderA) * phManifold::NUM_CONSTRAINT_TYPES;
			int tableColumn = GetColliderTypeIndex(colliderB);		

			if (manifold->CompositeManifoldsEnabled())
			{
#if 0 // Not currently anything in UpdateContactsArtJoint
				if (pair.collider1 == pair.collider2)
				{
					// Self collision, process joints
					UpdateContactsArtJoint(*manifold, globals);
				}
#endif

				numNonEmptyCompositeManifolds = 0;

				const int numCompositeManifolds = manifold->GetNumCompositeManifolds();
				for (int manifoldIndex = 0; manifoldIndex < numCompositeManifolds; ++manifoldIndex)
				{
					phManifold* compositeManifold = manifold->GetCompositeManifold(manifoldIndex);

#if USE_PRECOMPUTE_SEPARATEBIAS
					compositeManifold->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTimeStep);
#endif // USE_PRECOMPUTE_SEPARATEBIAS
					g_UpdateContactsConstraintTable[tableRow][tableColumn](*compositeManifold, globals);

					if (!justRunConstraintFunctions)
					{
						// Build up the list of the manifolds that still have contacts in them.  If there are no contacts left in this manifold, release it.
						if (!compositeManifold->ShouldRelease())
						{
							manifold->SetManifoldAndPairComponents(numNonEmptyCompositeManifolds, compositeManifold, static_cast<u8>(compositeManifold->GetComponentA()), static_cast<u8>(compositeManifold->GetComponentB()));
							++numNonEmptyCompositeManifolds;
						}
						else
						{
							manifoldPool->Release(compositeManifold);
						}
					}
				}

				// If the list of non-empty manifolds has changed, update the root manifold to reflect that.
				if(!justRunConstraintFunctions && numNonEmptyCompositeManifolds != numCompositeManifolds)
				{
					manifold->SetNumCompositeManifolds(numNonEmptyCompositeManifolds);
				}
			}
			else
			{
				if (manifold->IsConstraint() && !justRunConstraintFunctions)
				{
					manifold->RefreshContactPoints(minManifoldPointLifeTime, timeStep);
				}

#if USE_PRECOMPUTE_SEPARATEBIAS
				manifold->UpdateSeparateBias(separateBiasMultiplier,halfPenetration,invTimeStep);
#endif // USE_PRECOMPUTE_SEPARATEBIAS
				g_UpdateContactsConstraintTable[tableRow + manifold->GetConstraintType()][tableColumn](*manifold, globals);
			}

#if __PPU && BUILD_DMA_PLAN
			phManifold::DmaPlan* dmaPlan = PHSIM->GetDmaPlan(manifold);
			dmaPlan->Initialize();
			manifold->GenerateDmaPlan(*dmaPlan);

			// Generate DMA plans for the second and further manifolds (manifold zero is DMAed along with the parent manifold)
			if (manifold->CompositeManifoldsEnabled())
			{
				int numComposite = manifold->GetNumCompositeManifolds();

				for (int index = 1; index < numComposite; ++index)
				{
					phManifold* compositeManifold = manifold->GetCompositeManifold(index);
					sysDmaPlan* compositeDmaPlan = PHSIM->GetDmaPlan(compositeManifold);
					compositeDmaPlan->Initialize();
					compositeManifold->GenerateDmaPlan(*compositeDmaPlan);
				}
			}
#endif // __PPU && BUILD_DMA_PLAN

			PER_MANIFOLD_SOLVER_METRICS_ONLY(manifold->SetUpdateContactsTime(timer.GetTickTime());)
		}
#endif // !__SPU
	}

#if !__STATS
	PIXEnd();
#endif
}

#if __SPU
void phWorkerThreadMain(sysTaskParameters& params)
{
	static const int tag = 1;

	const int paddedManifoldInitParamsBufferSize = RoundUp<16>(sizeof(phPool<phManifold>::SpuInitParams));
	u8 manifoldInitParamsBuffer[paddedManifoldInitParamsBufferSize] ;
	phPool<phManifold>::SpuInitParams *manifoldPoolInitParams = reinterpret_cast<phPool<phManifold>::SpuInitParams *>(&manifoldInitParamsBuffer);
	sysDmaGet(manifoldPoolInitParams, uint64_t(params.UserData[15].asPtr), paddedManifoldInitParamsBufferSize, DMA_TAG(tag));

	const int paddedContactInitParamsBufferSize = RoundUp<16>(sizeof(phPool<phContact>::SpuInitParams));
	u8 contactInitParamsBuffer[paddedContactInitParamsBufferSize] ;
	phPool<phContact>::SpuInitParams *contactPoolInitParams = reinterpret_cast<phPool<phContact>::SpuInitParams *>(&contactInitParamsBuffer);
	sysDmaGet(contactPoolInitParams, uint64_t(params.UserData[7].asPtr), paddedContactInitParamsBufferSize, DMA_TAG(tag));

	sysDmaWaitTagStatusAll(DMA_MASK(tag));	// Wait for contact and manifoldPoolInitParams to finish coming in.

	PHCONTACT->InitSpu(*contactPoolInitParams);
	PHMANIFOLD->InitSpu(*manifoldPoolInitParams);

	UpdateContactsTask(params);

	PHMANIFOLD->ShutdownSpu();
	PHCONTACT->ShutdownSpu();
}
#endif // __SPU

} // namespace rage
