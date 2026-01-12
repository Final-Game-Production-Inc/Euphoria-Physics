//
// physics/constraintbase.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "constraintbase.h"

#include "collider.h"
#include "levelnew.h"
#include "overlappingpairarray.h"
#include "simulator.h"

#include "phbound/boundcomposite.h"
#include "phbullet/DiscreteCollisionDetectorInterface.h"
#include "phcore/pool.h"

#if !__FINAL
#include "system/timemgr.h"
#endif

namespace rage {


phConstraintBase::phConstraintBase(const Params& params)
	: m_FlaggedForDestruction(false)
	, m_MassInvScaleA(params.massInvScaleA)
	, m_MassInvScaleB(params.massInvScaleB)
	, m_ComponentA((u16)params.componentA)
	, m_ComponentB((u16)params.componentB)
	, m_UsePushes(params.usePushes)
	, m_Swapped(false)
	, m_BreakingStrengthSqr(params.breakingStrength*params.breakingStrength)
	, m_Broken(false)
	, m_Breakable(params.breakable)
	, m_Type((u8)params.type)
	, m_SeparateBias(params.separateBias)
	, m_AllowForceActivation(true)
	, m_AtLimit(true)
	, m_NeedsExtraIterations(params.extraIterations)
	NOTFINAL_ONLY(, m_FrameCreated(TIME.GetFrameCount()))
{
	Assertf(params.instanceA || params.instanceB, "Constraint added with two NULL instances");

	bool componentGoodA = true;

	if (params.componentA > 0)
	{
		componentGoodA = false;

		phBoundComposite* bound = static_cast<phBoundComposite*>(params.instanceA->GetArchetype()->GetBound());
		if(Verifyf(bound->GetType() == phBound::COMPOSITE, "Constraint attached to a non-composite bound %s with component %d", params.instanceA->GetArchetype()->GetFilename(), params.componentA))
		{
			if (Verifyf(params.componentA < bound->GetMaxNumBounds(), "Constraint attached to component %d, but %s only has %d parts", params.componentA, params.instanceA->GetArchetype()->GetFilename(), bound->GetMaxNumBounds()))
			{
				componentGoodA = true;
			}
		}
	}

	if (params.instanceA && componentGoodA)
	{
		Assertf(params.instanceA->GetLevelIndex() != phInst::INVALID_INDEX, "Constraint's instanceA is not in the level");
		u16 levelIndexA = params.instanceA->GetLevelIndex();
		m_LevelIndexA = levelIndexA;
		m_GenerationIdA = PHLEVEL->GetGenerationID(levelIndexA);
	}
	else
	{
		m_LevelIndexA = phInst::INVALID_INDEX;
		m_GenerationIdA = phInst::INVALID_INDEX;
	}

	bool componentGoodB = true;

	if (params.componentB > 0)
	{
		componentGoodB = false;

		phBoundComposite* bound = static_cast<phBoundComposite*>(params.instanceB->GetArchetype()->GetBound());
		if(Verifyf(bound->GetType() == phBound::COMPOSITE, "Constraint attached to a non-composite bound %s with component %d", params.instanceB->GetArchetype()->GetFilename(), params.componentB))
		{
			if (Verifyf(params.componentB < bound->GetMaxNumBounds(), "Constraint attached to component %d, but %s only has %d parts", params.componentB, params.instanceB->GetArchetype()->GetFilename(), bound->GetMaxNumBounds()))
			{
				componentGoodB = true;
			}
		}
	}

	if (params.instanceB && componentGoodB)
	{
		Assertf(params.instanceB->GetLevelIndex() != phInst::INVALID_INDEX, "Constraint's instanceB is not in the level");
		u16 levelIndexB = params.instanceB->GetLevelIndex();
		m_LevelIndexB = levelIndexB;
		m_GenerationIdB = PHLEVEL->GetGenerationID(levelIndexB);
	}
	else
	{
		m_LevelIndexB = phInst::INVALID_INDEX;
		m_GenerationIdB = phInst::INVALID_INDEX;
	}
}

#if !__FINAL
void phConstraintBase::SetOwner(const char* file, int line)
{
	m_OwnerFile = file;
	m_OwnerLine = line;
}
#endif // !__FINAL

void phConstraintBase::SetBreakable(bool breakable, float breakingStrength)
{
	m_Breakable = breakable;
	m_BreakingStrengthSqr = breakingStrength * breakingStrength;
}

void phConstraintBase::SetBroken(bool broken)
{
	m_Broken = broken;
}

bool phConstraintBase::Update(Vec::V3Param128 invTimeStep128, bool addToSolver)
{
	if (IsFlaggedForDestruction())
	{
		// This signals to the constraint manager to destruct and free the constraint
		return true;
	}

	// Make sure the constrained objects are still in the physics level.
	phInst* instanceA = GetInstanceA();
	if (m_LevelIndexA != phInst::INVALID_INDEX && instanceA == NULL)
	{
		// This constraint attaches an object that's not in the level, so abort and disable ourselves
		FlagForDestruction();
		return true;
	}

	phInst* instanceB = GetInstanceB();

	if (m_LevelIndexB != phInst::INVALID_INDEX && instanceB == NULL)
	{
		// This constraint attaches an object that's not in the level, so abort and disable ourselves
		FlagForDestruction();
		return true;
	}

	if (m_Broken)
	{
		return false;
	}

	// Try to get a collider for object A, and see if it is awake.
	phCollider* colliderA = NULL;
	if (instanceA)
	{
		colliderA = PHSIM->GetCollider(instanceA->GetLevelIndex());
	}

	// Try to get a collider for object B, and see if it is awake, or if the world constraint moved.
	phCollider* colliderB = NULL;
	if (instanceB)
	{
		// This constraint has an instance for object B, so try to get a collider for object B.
		colliderB = PHSIM->GetCollider(instanceB->GetLevelIndex());
	}

	if (colliderA == NULL && colliderB == NULL)
		return false;

	// We have two active objects, or one active object attached to the world, apply the constraint.
	VirtualUpdate(invTimeStep128, instanceA, colliderA, instanceB, colliderB, addToSolver);

	return false;
}

void phConstraintBase::EnforceWorldConstraints()
{
	if (m_LevelIndexB == phInst::INVALID_INDEX)
	{
		phInst* instanceA = GetInstanceA();

		if (instanceA)
		{
			phCollider* colliderA = PHSIM->GetCollider(instanceA->GetLevelIndex());

			if (colliderA)
			{
				VirtualEnforceWorldConstraints(colliderA);
			}
		}
	}
	else if (m_LevelIndexA == phInst::INVALID_INDEX)
	{
		phInst* instanceB = GetInstanceB();

		if (instanceB)
		{
			phCollider* colliderB = PHSIM->GetCollider(instanceB->GetLevelIndex());

			if (colliderB)
			{
				VirtualEnforceWorldConstraints(colliderB);
			}
		}
	}
}

void phConstraintBase::VirtualEnforceWorldConstraints(phCollider* UNUSED_PARAM(collider))
{
}


void phConstraintBase::RequestWorldLimitEnforcement()
{
	if (m_LevelIndexB == phInst::INVALID_INDEX)
	{
		phInst* instanceA = GetInstanceA();

		if (instanceA)
		{
			phCollider* colliderA = PHSIM->GetCollider(instanceA->GetLevelIndex());

			if (colliderA)
			{
				PHCONSTRAINT->AddWorldConstraint(m_Handle);
			}
		}
	}
	else if (m_LevelIndexA == phInst::INVALID_INDEX)
	{
		phInst* instanceB = GetInstanceB();

		if (instanceB)
		{
			phCollider* colliderB = PHSIM->GetCollider(instanceB->GetLevelIndex());

			if (colliderB)
			{
				PHCONSTRAINT->AddWorldConstraint(m_Handle);
			}
		}
	}
}


void phConstraintBase::ReplaceInstanceComponents(int originalLevelIndex, const atFixedBitSet<128>& componentBits, int newLevelIndex, int newGenerationId)
{
	if(m_LevelIndexA == originalLevelIndex && componentBits.IsSet(m_ComponentA))
	{
		m_LevelIndexA = (u16)newLevelIndex;
		m_GenerationIdA = (u16)newGenerationId;
	}
	if(m_LevelIndexB == originalLevelIndex && componentBits.IsSet(m_ComponentB))
	{
		m_LevelIndexB = (u16)newLevelIndex;
		m_GenerationIdB = (u16)newGenerationId;
	}
}


phInst* phConstraintBase::GetInstanceA()
{
	phInst* instanceA = NULL;
	if (m_LevelIndexA != phInst::INVALID_INDEX)
	{
		if (PHLEVEL->IsLevelIndexGenerationIDCurrent(m_LevelIndexA, m_GenerationIdA))
		{
			instanceA = PHLEVEL->GetInstance(m_LevelIndexA);
		}
	}
	return instanceA;
}

phInst* phConstraintBase::GetInstanceB()
{
	phInst* instanceB = NULL;
	if (m_LevelIndexB != phInst::INVALID_INDEX)
	{
		if (PHLEVEL->IsLevelIndexGenerationIDCurrent(m_LevelIndexB, m_GenerationIdB))
		{
			instanceB = PHLEVEL->GetInstance(m_LevelIndexB);
		}
	}
	return instanceB;
}

bool phConstraintBase::IsInstAValid()
{
	return m_LevelIndexA == phInst::INVALID_INDEX || GetInstanceA() != NULL;
}

bool phConstraintBase::IsInstBValid()
{
	return m_LevelIndexB == phInst::INVALID_INDEX || GetInstanceB() != NULL;
}

phManifold* phConstraintBase::AllocateManifold(phInst* instanceA, phInst* instanceB, Mat33V* constraintMatrix)
{
	phManifold* manifold = PHMANIFOLD->Allocate(true);

	if (manifold)
	{
		manifold->Reset();

		manifold->SetInstanceA(instanceA);
		manifold->SetInstanceB(instanceB);
		manifold->SetColliderA(NULL);
		manifold->SetColliderB(NULL);
		manifold->SetLevelIndexA(m_LevelIndexA, m_GenerationIdA);
		manifold->SetLevelIndexB(m_LevelIndexB, m_GenerationIdB);
		manifold->SetComponentA(m_ComponentA);
		manifold->SetComponentB(m_ComponentB);
		manifold->SetConstraintMatrix(constraintMatrix);
		phContact constraintContact;
		manifold->AddManifoldPoint(constraintContact);
		manifold->SetIsConstraint();
		if (manifold->GetNumContacts() == 0)
		{
			PHMANIFOLD->Release(manifold);
			manifold = NULL;
		}
	}

	return manifold;
}

void phConstraintBase::GetWorldMatrix(const phInst* instance, int component, Mat34V_Ref matrix) const
{
	Mat34V worldMatrix = Mat34V(V_IDENTITY);
	if (instance && instance->GetArchetype() && instance->GetArchetype()->GetBound())
	{
		const phBound& bound = *instance->GetArchetype()->GetBound();
		if (bound.GetType()==phBound::COMPOSITE)
		{
			const phBoundComposite& compositeBound = *static_cast<const phBoundComposite*>(&bound);
			Mat34V tempMat = instance->GetMatrix();
			if (Verifyf(component < compositeBound.GetMaxNumBounds(), "Bad component number %d in constraint on object %s", component, instance->GetArchetype()->GetFilename()))
			{
				Transform( worldMatrix, tempMat, compositeBound.GetCurrentMatrix(component));
			}
			else
			{
				worldMatrix = tempMat;
			}
		}
		else
		{
			worldMatrix = instance->GetMatrix();
		}
	}

	matrix = worldMatrix;
}

QuatV_Out phConstraintBase::GetWorldQuaternion(const phInst* instance, int component) const
{
	Mat34V worldMatrix;
	GetWorldMatrix(instance, component, worldMatrix);
	return QuatVFromMat33V(worldMatrix.GetMat33());
}

#if PROPHYLACTIC_SWAPS
extern bool g_ProphylacticSwaps;
#endif

void phConstraintBase::AddManifoldToSolver(phManifold* manifold, phInst* instanceA, phCollider* colliderA, phInst* instanceB, phCollider* colliderB)
{
	if (!colliderA && !colliderB)
	{
		return;
	}

	if (m_NeedsExtraIterations)
	{
		PHCONSTRAINT->AddExtraIterManifold(manifold);
	}

	Assert(manifold);
	phOverlappingPairArray* pairArray = PHSIM->GetOverlappingPairArray();
	if (pairArray->pairs.GetCount() < pairArray->pairs.GetCapacity())
	{
		u16 levelIndexA = m_LevelIndexA;
		u16 levelIndexB = m_LevelIndexB;
		u16 generationIdA = m_GenerationIdA;
		u16 generationIdB = m_GenerationIdB;
		float massInvScaleA = m_MassInvScaleA;
		float massInvScaleB = m_MassInvScaleB;
		int componentA = m_ComponentA;
		int componentB = m_ComponentB;
		if (m_Swapped)
		{
			SwapEm(instanceA, instanceB);
			SwapEm(colliderA, colliderB);
			SwapEm(levelIndexA, levelIndexB);
			SwapEm(generationIdA, generationIdB);
			SwapEm(massInvScaleA, massInvScaleB);
			SwapEm(componentA, componentB);
		}
		manifold->SetInstanceA(instanceA);
		manifold->SetInstanceB(instanceB);
		manifold->SetColliderA(colliderA);
		manifold->SetColliderB(colliderB);
		manifold->SetLevelIndexA(levelIndexA, generationIdA);
		manifold->SetLevelIndexB(levelIndexB, generationIdB);
		manifold->SetComponentA(componentA);
		manifold->SetComponentB(componentB);
		manifold->SetMassInvScaleA(massInvScaleA);
		manifold->SetMassInvScaleB(massInvScaleB);

		bool usePushes = m_UsePushes;
		manifold->m_UsePushes = usePushes;

		TrapGE(pairArray->pairs.GetCount(), pairArray->pairs.GetCapacity());
		phTaskCollisionPair& pair = pairArray->pairs.GetElements()[pairArray->pairs.GetCount()];
		pair.levelIndex1 = levelIndexA;
		pair.generationId1 = generationIdA;
		pair.levelIndex2 = levelIndexB;
		pair.generationId2 = generationIdB;
		pair.manifold = manifold;

#if PROPHYLACTIC_SWAPS
		if (Likely(g_ProphylacticSwaps))
		{
			if (colliderB && !colliderA ||
				(colliderA && colliderB && colliderB->IsArticulated() && !colliderA->IsArticulated()))
			{
				pair.Exchange();

				m_Swapped = !m_Swapped;
			}
		}
#endif // PROPHYLACTIC_SWAPS

#if TRACK_COLLISION_TIME
		if (phCollider* collider = PHSIM->GetCollider(m_LevelIndexA))
		{
			collider->DecreaseCollisionTimeSafe(0.0f);
		}

		if (phCollider* collider = PHSIM->GetCollider(m_LevelIndexB))
		{
			collider->DecreaseCollisionTimeSafe(0.0f);
		}
#endif

		sys_lwsync();
		pairArray->pairs.Append();
#if PH_MANIFOLD_TRACK_IS_IN_OPA
		manifold->SetIsInOPA(true);
#endif // PH_MANIFOLD_TRACK_IS_IN_OPA

		if (m_AllowForceActivation && ((instanceA && !colliderA && colliderB) || (instanceB && !colliderB && colliderA)))
		{
			PHSIM->AddActivatingPair(&pair);
		}
	}
	else
	{
		AssertMsg(false, "Ran out of overlapping pairs when trying to insert a constraint pair.");
	}
}

phContact* phConstraintBase::GetContactPoint(phManifold* manifold)
{
	if (Verifyf(manifold->GetNumContacts() == 1, "Somehow the contact point was lost for a constraint manifold, disabling"))
	{
		return &manifold->GetContactPoint(0);
	}
	else
	{
		return NULL;
	}
}

bool phConstraintBase::BreakIfImpulseSufficient(phManifold* manifold)
{
	phContact& contact = manifold->GetContactPoint(0);

	Vec3V impulse;
	if (manifold->IsArticulatedFixedCollision())
	{
		impulse = contact.ComputeTotalArtFixImpulse();
	}
	else
	{
		impulse = contact.ComputeTotalImpulse();
	}

	if (MagSquared( impulse ).Getf() > m_BreakingStrengthSqr)
	{
		m_Broken = true;
		return true;
	}
	
	return false;
}

void phConstraintBase::DisableManifold(phManifold* manifold)
{
	if (manifold && manifold->GetNumContacts() > 0)
	{
		FastAssert(manifold->GetNumContacts() == 1);
		manifold->GetContactPoint(0).ActivateContact(false);
	}
}

#if __ASSERT
int phConstraintBase::ConstraintAssertFunc(const char* file, int line, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char msg[1024];
	vformatf(msg, sizeof(msg), fmt, args);
	va_end(args);

	const char* typeName = GetTypeName();
	phInst* instA = GetInstanceA();
	phInst* instB = GetInstanceB();
	phArchetype* archA = instA ? instA->GetArchetype() : NULL;
	phArchetype* archB = instB ? instB->GetArchetype() : NULL;
	const char* nameA = archA ? archA->GetFilename() : NULL;
	const char* nameB = archB ? archB->GetFilename() : NULL;
	const char* ownerFile = GetOwnerFile();
	int ownerLine = GetOwnerLine();

	static bool alreadyDumpedInstances = false;
	if(!alreadyDumpedInstances)
	{
		alreadyDumpedInstances = true;
		if(instA)
		{
			instA->InvalidStateDump();
		}
		if(instB)
		{
			instB->InvalidStateDump();
		}
	}

	return diagAssertHelper(file, line, """%s"", from %s constraint at 0x%p between %s and %s (constraint inserted at %s:%d)", msg, typeName, this, nameA, nameB, ownerFile, ownerLine);
}
#endif // __ASSERT

void phConstraintBase::FlagForDestruction()
{
	if (!IsFlaggedForDestruction())
	{
		m_FlaggedForDestruction = true;

		DisableManifolds();
	}
}

void phConstraintBase::ReconstructBaseParams(phConstraintBase::Params& inoutParams)
{
	  inoutParams.breakable = m_Breakable;
	  inoutParams.breakingStrength = Sqrtf(m_BreakingStrengthSqr);
	  inoutParams.componentA = m_ComponentA;
	  inoutParams.componentB = m_ComponentB;
	  inoutParams.instanceA = GetInstanceA();
	  inoutParams.instanceB = GetInstanceB();
	  inoutParams.massInvScaleA = m_MassInvScaleA;
	  inoutParams.massInvScaleB = m_MassInvScaleB;
	  inoutParams.separateBias = m_SeparateBias;
	  inoutParams.usePushes = m_UsePushes;
	  inoutParams.extraIterations = m_NeedsExtraIterations;
	  
	  //This doesn't get copied: inoutParams.type
}

} // namespace rage
