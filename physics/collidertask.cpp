// 
// physics/collidertask.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "collidertask.h"

#include "broadphase.h"
#include "colliderdispatch.h"
#include "sleep.h"
#include "simulator.h"

#include "phcore/constants.h"

#include "profile/element.h"
#include "system/spinlock.h"
#include "system/interlocked.h"
#include "grprofile/pix.h"

#if __SPU
#include "pharticulated/spudma.h"
#include <cell/spurs/queue.h>
#endif

#include "profile/cellspurstrace.h"

namespace rage {

namespace SimulatorStats
{
	EXT_PF_TIMER(ColliderUpdateThread);
}

using namespace SimulatorStats;

#if __SPU
// 69k below is to support horse with 29 links.
static const int FULL_ARTICULATED_SCRATCHPAD_SIZE = 1024 * 69;
static u8 s_ArticulatedScratchpad[FULL_ARTICULATED_SCRATCHPAD_SIZE] ;
static ArticulatedDmaFull articulatedDma;

float g_JointLimitElasticity = 0.0f;
#if __BANK
bool g_PushCollisionTTY;
#endif // __BANK

extern float g_PushCollisionTolerance;
extern float g_TurnCollisionTolerance;
#endif // __SPU

void UpdateCollidersTask(sysTaskParameters& params)
{
	PF_FUNC(ColliderUpdateThread);
#if !__STATS
	PIXBegin(0, "UpdateCollidersTask");
#endif

	int updateType				=				params.UserData[0].asInt;
	u32* atom					= (u32*)		params.UserData[1].asPtr;
	u32 numColliders			=				params.UserData[2].asInt;
	u16* levelIndices			= (u16*)		params.UserData[3].asPtr;
	phCollider** colliders		= (phCollider**)params.UserData[4].asPtr;
	ScalarV timeStepV			=				ScalarV(params.UserData[5].asFloat);
	float timeStep				=				params.UserData[5].asFloat;

	Vec3V gravity(params.UserData[6].asFloat, params.UserData[7].asFloat, params.UserData[8].asFloat);

	bool sleepEnabled			=				params.UserData[9].asBool;
#if EARLY_FORCE_SOLVE
	bool* needsCollision		= (bool*)		params.UserData[11].asPtr;
#if __SPU
#if __BANK
	g_PushCollisionTTY			=				params.UserData[12].asBool;
#endif // __BANK
#endif // __SPU
#else // EARLY_FORCE_SOLVE
	bool usePushes				=				params.UserData[11].asBool;
#endif // EARLY_FORCE_SOLVE

#if __SPU
	bool sentNeedsCollision = false;
	phArticulatedCollider** colliderArray = (phArticulatedCollider**)params.UserData[13].asPtr;
	u32* colliderCount = (u32*)params.UserData[14].asPtr;
	phArticulatedCollider::SetActiveArrayInfo(colliderArray, colliderCount);
	g_JointLimitElasticity = params.UserData[15].asFloat;
#endif

	float allowedAnglePenetration = params.UserData[16].asFloat;

#if __SPU
	g_PushCollisionTolerance = params.UserData[17].asFloat;
	g_TurnCollisionTolerance = params.UserData[18].asFloat;
#endif // __SPU

	u32 colliderIndex;
	while ((colliderIndex = sysInterlockedIncrement(atom) - 1) < numColliders)
	{
#if __SPU
		static const int tag = 1;
		u16 levelIndex = sysDmaGetUInt16(uint64_t(levelIndices + colliderIndex), tag);
		phCollider* colliderMm = (phCollider*)sysDmaGetUInt32(uint64_t(colliders + levelIndex), tag);
		u8 colliderBuffer[sizeof(phArticulatedCollider)] ALIGNED(128);
		sysDmaGet(colliderBuffer, uint64_t(colliderMm), sizeof(phArticulatedCollider), tag);
		phCollider* collider = (phCollider*)colliderBuffer;
		sysDmaWaitTagStatusAll(1 << tag);

		bool artAllocOk = true;

		if (collider->IsArticulated())
		{
			articulatedDma.Init(s_ArticulatedScratchpad, FULL_ARTICULATED_SCRATCHPAD_SIZE);
			artAllocOk = articulatedDma.FetchToLs(static_cast<phArticulatedCollider*>(collider));
		}

		if (!artAllocOk)
		{
			Errorf("UpdateCollidersPreTask -- Not enough SPU memory for articulated body.");
			continue;
		}

		phInst* instMm = collider->GetInstance();
		u8 instBuffer[sizeof(phInst)] ALIGNED(128);
		sysDmaGet(instBuffer, uint64_t(instMm), sizeof(phInst), tag);
		sysDmaWaitTagStatusAll(1 << tag);
		phInst* inst = (phInst*)instBuffer;
		collider->SetLSInstance(inst);

		phArchetype* archetypeMm = inst->GetArchetype();
		u8 archetypeBuffer[sizeof(phArchetypeDamp)] ALIGNED(128);
		sysDmaGet(archetypeBuffer, uint64_t(archetypeMm), sizeof(phArchetypeDamp), tag);
		sysDmaWaitTagStatusAll(1 << tag);
		phArchetypeDamp* archetype = (phArchetypeDamp*)archetypeBuffer;
		inst->SetLSArchetype(archetype);
#else
		u16 levelIndex = levelIndices[colliderIndex];
		phCollider* collider = colliders[levelIndex];
#endif // __SPU

		// BEGIN COMMON SECTION

		switch (updateType)
		{
#if EARLY_FORCE_SOLVE
			case PHCOLLIDER_INTEGRATE_VELOCITIES:
			{
				if (collider->IsArticulated())
				{
#if __SPU
					static_cast<phArticulatedCollider*>(colliderMm)->AddToActiveArray();
#else
					static_cast<phArticulatedCollider*>(collider)->AddToActiveArray();
#endif
				}

				// Update velocity from all sources.
				collider->ApplyGravity(gravity.GetIntrin128(), timeStepV.GetIntrin128());
				collider->ApplyInternalForces(timeStepV.GetIntrin128());
				collider->DampMotion(timeStepV.GetIntrin128());
				collider->UpdateVelocity(timeStepV.GetIntrin128(), true);
				collider->UpdateLastVelocities();

				if (collider->IsArticulated())
				{
					phArticulatedCollider* artCollider = static_cast<phArticulatedCollider*>(collider);
					artCollider->FindJointLimitDofs(allowedAnglePenetration);
				}

				//Reset solver mass and inertia tensor
				collider->ResetSolverInvMass();
				collider->ResetSolverInvAngInertia();
				collider->ResetRotationConstraintInvMass();
				collider->ResetRotationConstraintInvAngInertia();
				collider->ResetTranslationConstraintInvMass();
				collider->ResetTranslationConstraintInvAngInertia();

				collider->SetClearNextWarmStart(collider->GetNeedsUpdateBeforeFinalIterations());
				collider->SetNeedsUpdateBeforeFinalIterations(false);

				break;
			}
			case PHCOLLIDER_INTEGRATE_POSITIONS:
			{
				// Update velocity from all sources.
				if (collider->IsDoubleDampingEnabled())
				{
					collider->DampMotion(timeStepV.GetIntrin128());
				}
				collider->UpdateVelocityFromExternal(timeStepV.GetIntrin128());  // articulated functionality has moved to ApplyInternalForces.  TODO - investigate whether regular colliders need this call here.
				collider->UpdatePositionFromVelocity(timeStepV.GetIntrin128());

				if (collider->IsArticulated())
				{
					phArticulatedCollider* artCollider = static_cast<phArticulatedCollider*>(collider);
					artCollider->FindJointLimitDofs(allowedAnglePenetration);
				}

				//Reset solver mass and inertia tensor again for the upcoming push solve
				collider->ResetSolverInvMass();
				collider->ResetSolverInvAngInertia();
				collider->ResetRotationConstraintInvMass();
				collider->ResetRotationConstraintInvAngInertia();
				collider->ResetTranslationConstraintInvMass();
				collider->ResetTranslationConstraintInvAngInertia();

				collider->SetClearNextWarmStart(collider->GetNeedsUpdateBeforeFinalIterations());
				collider->SetNeedsUpdateBeforeFinalIterations(false);

				if(collider->IncrementAndCheckRejuvenation())
				{	
					collider->Rejuvenate();
				}

				if (sleepEnabled)
				{
#if __SPU
					if (phSleep* sleepMm = collider->GetSleep())
					{
						u8 sleepBuffer[sizeof(phSleep)] ;
						phSleep* sleep = (phSleep*)sleepBuffer;
						sysDmaGetAndWait(sleepBuffer, uint64_t(sleepMm), sizeof(phSleep), tag);

						collider->SetSleep(sleep);
						sleep->SetColliderPtr(collider);

						sleep->Update(timeStep);

						sleep->SetColliderPtr(colliderMm);
						collider->SetSleep(sleepMm);

						sysDmaPutAndWait(sleepBuffer, uint64_t(sleepMm), sizeof(phSleep), tag);
					}
#else
					if (phSleep* sleep = collider->GetSleep())
					{
						sleep->Update(timeStep);
					}
#endif
				}

				break;
			}
			case PHCOLLIDER_APPLY_PUSHES:
			{
#if TRACK_PUSH_COLLIDERS
				if(collider->GetIsInPushPair())
#endif // TRACK_PUSH_COLLIDERS
				{
					// Update the collider's position from pushes.
					collider->Move(timeStepV.GetIntrin128(), true);

					if (collider->IsArticulated())
					{
						phArticulatedCollider* artCollider = static_cast<phArticulatedCollider*>(collider);
						phArticulatedBody* body = artCollider->GetBody();

						body->CalculateInertias();
						artCollider->FindJointLimitDofs(allowedAnglePenetration);
					}

#if __SPU
					if (collider->GetNeedsCollision())
					{
						if (!sentNeedsCollision)
						{
							sysDmaPutUInt8(true, (uint64_t)needsCollision, 1);

							sentNeedsCollision = true;
						}
					}
#else // __SPU
					if (collider->GetNeedsCollision())
					{
						*needsCollision = true;
					}
#endif // __SPU
				}
#if TRACK_PUSH_COLLIDERS
				else
				{
					collider->UpdateApplyZeroPushes();
				}
#endif // TRACK_PUSH_COLLIDERS

				break;
			}
#else // EARLY_FORCE_SOLVE
			case PHCOLLIDER_UPDATE:
			{
#if __SPU
				if (collider->IsArticulated())
				{
					static_cast<phArticulatedCollider*>(colliderMm)->AddToActiveArray();
				}
#endif
				collider->Update(timeStepV.GetIntrin128(), gravity.GetIntrin128());
				break;
			}

			case PHCOLLIDER_UPDATE_POST:
			{
				phSleep* sleep = NULL;

#if __SPU
				u8 sleepObjBuffer[sizeof(phSleep)] ;
				phSleep* sleepObjAddrMM = collider->GetSleep();
				if (sleepObjAddrMM && sleepEnabled)
				{
					sysDmaGet(sleepObjBuffer, uint64_t(sleepObjAddrMM), sizeof(phSleep), tag);
					collider->SetSleep(reinterpret_cast<phSleep*>(sleepObjBuffer));
					sysDmaWaitTagStatusAll(1 << tag);
					collider->GetSleep()->SetColliderPtr(collider);
					sleep = (phSleep*)sleepObjBuffer;
				}
#else
				if (sleepEnabled)
				{
					sleep = collider->GetSleep();
				}
#endif

				collider->UpdateVelocity(timeStepV.GetIntrin128());

				collider->Move(timeStepV.GetIntrin128(), usePushes);

				if (sleep)
				{
					sleep->Update(timeStep);
				}

				if (collider->IsArticulated())
				{
					phArticulatedCollider* artCollider = static_cast<phArticulatedCollider*>(collider);
					phArticulatedBody* body = artCollider->GetBody();

					body->CalculateInertias();
				}

#if __SPU
				if (sleep)
				{
					phSleep* sleepLS = collider->GetSleep();
					sleepLS->SetColliderPtr(colliderMm);
					sysDmaPut(sleepObjBuffer, uint64_t(sleepObjAddrMM), sizeof(phSleep), tag);
					collider->SetSleep(sleepObjAddrMM);
				}
#endif

				break;
			}
#endif // EARLY_FORCE_SOLVE
		}

		// END COMMON SECTION

#if __SPU
		inst->SetLSArchetype(archetypeMm);
		collider->SetLSInstance(instMm);
		sysDmaPut(instBuffer, uint64_t(instMm), sizeof(phInst), tag);

		int sizeofCollider = sizeof(phCollider);
		switch(collider->GetType())
		{
		case phCollider::TYPE_ARTICULATED_BODY:
		case phCollider::TYPE_ARTICULATED_LARGE_ROOT:
			sizeofCollider = sizeof(phArticulatedCollider);
			articulatedDma.WriteFromLs();
			break;
		}

		sysDmaPut(colliderBuffer, uint64_t(colliderMm), sizeofCollider, tag);
		sysDmaWaitTagStatusAll(1 << tag);
#endif
	}

#if !__STATS
	PIXEnd();
#endif
}

#if __SPU
void phWorkerThreadMain(sysTaskParameters& params)
{
	UpdateCollidersTask(params);
}
#endif // __SPU

} // namespace rage
