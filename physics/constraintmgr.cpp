// 
// physics/constraintmgr.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "constraintmgr.h"

#include "collider.h"
#include "constraintattachment.h"
#include "constraints.h"
#include "inst.h"
#include "overlappingpairarray.h"
#include "simulator.h"

#include "phbound/boundcomposite.h"
#include "phcore/pool.h"
#include "phsolver/forcesolvertables.h"

#include "grprofile/drawmanager.h"
#include "profile/element.h"
#include "profile/profiler.h"

#if USE_CONSTRAINT_UPDATE_CHECK
namespace SmokeTests
{
	XPARAM(smoketest);
};

bool IsRunningSmokeTest()
{
	const char* pSmokeTestName = NULL;
	SmokeTests::PARAM_smoketest.Get(pSmokeTestName);
	return (pSmokeTestName != NULL);
}
#endif // USE_CONSTRAINT_UPDATE_CHECK

namespace rage {


EXT_PFD_DECLARE_ITEM(Constraints);

namespace phContactStats
{
	EXT_PF_TIMER(BreakConstraints);
}

using namespace phContactStats;

namespace ConstraintMgrStats
{
	PF_PAGE(ConstraintMgrPage,"ph ConstraintMgr");

	PF_GROUP(ConstraintMgr);
	PF_LINK(ConstraintMgrPage,ConstraintMgr);
	PF_VALUE_INT(MaxActiveConstraints,ConstraintMgr);
	PF_VALUE_INT(ActiveConstraints,ConstraintMgr);
	PF_VALUE_INT(ExtraIterConstraints,ConstraintMgr);
	PF_VALUE_INT(ConstraintUID,ConstraintMgr);
};

using namespace ConstraintMgrStats;

phConstraintMgr::phConstraintMgr()
	: m_MaxNumActiveConstraints(0)
	, m_NumVirtualConstraints(0)
{
#if USE_CONSTRAINT_MGR_RELEASE_MANIFOLD
	m_ReleasedManifolds.Reserve(256);
#endif // USE_CONSTRAINT_MGR_RELEASE_MANIFOLD

	Reset();
}

phConstraintMgr::~phConstraintMgr()
{
	while (m_NumVirtualConstraints > 0)
	{
		Remove(m_ActiveIndices[0]);
	}

	delete m_VirtualConstraints;
	delete m_BigConstraints;
	delete m_LittleConstraints;
	delete [] m_AvailableIndices;
	delete [] m_ActiveIndices;
}

void phConstraintMgr::Reset ()
{
	// Remove all virtual constraints
	while (m_NumVirtualConstraints > 0)
	{
		Remove(m_ActiveIndices[0]);
	}
}

static sysCriticalSectionToken s_AddAndRemoveCritSec;

#if __PFDRAW
void phConstraintMgr::ProfileDraw() const
{
	if (PFD_Constraints.Begin())
	{
		sysCriticalSection critSec(s_AddAndRemoveCritSec);

		grcWorldIdentity();

		int numVirtualConstraints = m_NumVirtualConstraints;
		for (int index = 0; index < numVirtualConstraints; ++index)
		{
			phConstraintBase* constraint = m_VirtualConstraints[m_ActiveIndices[index]].constraint;

			constraint->ProfileDraw();
		}

		PFD_Constraints.End();
	}
}

void phConstraintMgr::ProfileDraw(const phInst* inst) const
{
	if (PFD_Constraints.Begin())
	{
		sysCriticalSection critSec(s_AddAndRemoveCritSec);

		grcWorldIdentity();

		int numVirtualConstraints = m_NumVirtualConstraints;
		for (int index = 0; index < numVirtualConstraints; ++index)
		{
			phConstraintBase* constraint = m_VirtualConstraints[m_ActiveIndices[index]].constraint;

			if(constraint->GetInstanceA() == inst || constraint->GetInstanceB() == inst)
			{
				constraint->ProfileDraw();
			}
		}

		PFD_Constraints.End();
	}
}

#endif

#if PROPHYLACTIC_SWAPS
extern bool g_ProphylacticSwaps;
#endif

void phConstraintMgr::UpdateConstraintContacts(float invTimeStep, bool addToSolver)
{
#if USE_CONSTRAINT_MGR_RELEASE_MANIFOLD
	while (m_ReleasedManifolds.GetCount() > 0)
	{
		PHMANIFOLD->Release(m_ReleasedManifolds.Pop());
	}
#endif // USE_CONSTRAINT_MGR_RELEASE_MANIFOLD

	PF_SET(MaxActiveConstraints,m_MaxNumActiveConstraints);

	m_ExtraIterManifolds.Resize(0);
	m_WorldConstraints.Resize(0);

	// Obtain the critical section outside of constraint update, so that we'll have it already for each removal
	s_AddAndRemoveCritSec.Lock();

	// Also process the virtual constraints	
	ScalarV invTimeStepV = ScalarVFromF32(invTimeStep);
	for (u16 index = 0; index < m_NumVirtualConstraints; ++index)
	{
		const ConstraintRecord& constraintRecord = m_VirtualConstraints[m_ActiveIndices[index]];
		phConstraintBase * constraint = constraintRecord.constraint;

		Assert(!IsInFreeList(constraint));

#if USE_CONSTRAINT_UPDATE_CHECK
		phOverlappingPairArray* pairArray = PHSIM->GetOverlappingPairArray();
		const int pairArrayCount0 = pairArray->pairs.GetCount();
#endif // USE_CONSTRAINT_UPDATE_CHECK
		bool removeMe = constraint->Update(invTimeStepV.GetIntrin128(), addToSolver);

		// Don't delete constraints after the first round of collision, they can add pairs to the OPA during the first round that wont get cleaned up.
		if (removeMe && addToSolver)
		{
#if USE_CONSTRAINT_UPDATE_CHECK
			// Make sure no pairs were added to the OPA.
			const int pairArrayCount1 = pairArray->pairs.GetCount();
			if (pairArrayCount0 != pairArrayCount1)
			{
				Displayf("Deleting a constraint that added pairs(%d,%s,%d,%d,%p) This will likely lead to a crash.\n",constraint->GetType(),constraint->GetTypeName(),index,m_ActiveIndices[index],constraint);
				if (IsRunningSmokeTest())
					FastAssert(0);	// Halt in a smoketest.
				else
					Assert(0);		// Make skippable per Eugene's request.
			}
#endif // USE_CONSTRAINT_UPDATE_CHECK
			Remove(m_ActiveIndices[index]);

			// Decrement index because we probably need to still process the constraint that was just swapped into place
			index--;
		}
	}

	s_AddAndRemoveCritSec.Unlock();

	PF_SET(ActiveConstraints,m_NumVirtualConstraints);
	PF_SET(ExtraIterConstraints,m_ExtraIterManifolds.GetCount());
}

void phConstraintMgr::AddExtraIterManifold(phManifold* manifold)
{
	m_ExtraIterManifolds.PushAndGrow(manifold);
}

void phConstraintMgr::PerformExtraConstraintIterations(float /*invTimeStep*/, bool usePushes, int iterations, phForceSolver& forceSolver)
{
	phForceSolver::ConstraintTable* constraintTable = usePushes ? &g_ApplyImpulseAndPushConstraintTable : &g_ApplyImpulseConstraintTable;
	/// ScalarV invTimeStepV = ScalarVFromF32(invTimeStep);

	int numManifolds = m_ExtraIterManifolds.GetCount();
	for (int iter = 0; iter < iterations; ++iter)
	{
		// Update constraints, and convert constraints into contacts for use in the collision response solver.
		for (int manifoldIndex = 0; manifoldIndex < numManifolds; ++manifoldIndex)
		{
			// Update this constraint.
			phManifold* extrIterManifold = m_ExtraIterManifolds[manifoldIndex];

			forceSolver.IterateOneConstraint(*extrIterManifold, *constraintTable);
		}
	}
}

void phConstraintMgr::CreateConstraints (int numManagedConstraints, int numBigConstraints, int numLittleConstraints, int maxNumConstraints)
{
	Assert(numManagedConstraints < 65535);
	Assert(numBigConstraints < 65535);
	Assert(numLittleConstraints < 65535);
	Assert(maxNumConstraints==BAD_INDEX || maxNumConstraints<65535);
	int maxNumActiveConstraints =  maxNumConstraints>=0 ? maxNumConstraints : numManagedConstraints;
	m_MaxNumActiveConstraints = maxNumActiveConstraints;

	m_VirtualConstraints = rage_new ConstraintRecord[maxNumActiveConstraints];
	m_AvailableIndices = rage_new u16[maxNumActiveConstraints];
	m_ActiveIndices = rage_new u16[maxNumActiveConstraints];

	m_ExtraIterManifolds.Reserve(32);

	// Create the constraint pools, if the user requests them (otherwise, allocation is from the main game heap)
	if (numBigConstraints > 0)
	{
		size_t bigConstraintSize = sizeof(rage::phConstraintHinge);
		bigConstraintSize = Max(sizeof(rage::phConstraintPrismatic), bigConstraintSize);
		m_BigConstraints = rage_new atPoolBase(bigConstraintSize, (u16)numBigConstraints);
	}
	else
	{
		m_BigConstraints = NULL;
	}
	if (numLittleConstraints > 0)
	{
		size_t littleConstraintSize = sizeof(rage::phConstraintSpherical);
		littleConstraintSize = Max(sizeof(rage::phConstraintFixed), littleConstraintSize);
		littleConstraintSize = Max(sizeof(rage::phConstraintFixedRotation), littleConstraintSize);
		littleConstraintSize = Max(sizeof(rage::phConstraintAttachment), littleConstraintSize);
		littleConstraintSize = Max(sizeof(rage::phConstraintHalfSpace), littleConstraintSize);
		littleConstraintSize = Max(sizeof(rage::phConstraintDistance), littleConstraintSize);
		littleConstraintSize = Max(sizeof(rage::phConstraintRotation), littleConstraintSize);
		littleConstraintSize = Max(sizeof(rage::phConstraintCylindrical), littleConstraintSize);
		m_LittleConstraints = rage_new atPoolBase(littleConstraintSize, (u16)numLittleConstraints);
	}
	else
	{
		m_LittleConstraints = NULL;
	}

	for (u16 index = 0; index < maxNumActiveConstraints; ++index)
	{
		m_AvailableIndices[index] = index;
	}

	m_NumVirtualConstraints = 0;
}


phConstraintHandle phConstraintMgr::Insert(const phConstraintBase::Params& params)
{
	phConstraintBase* newConstraint;

	return InsertAndReturnTemporaryPointer(params, newConstraint);
}

phConstraintBase* phConstraintMgr::AllocateAndConstruct(const phConstraintBase::Params& params)
{
	phConstraintBase* newConstraint = NULL;

	sysMemStartTemp();

	// Create the constraint of the correct type based on the type in the params
	switch (params.type)
	{
	case phConstraintBase::HALFSPACE:
		if (m_LittleConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_LittleConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintHalfSpace(static_cast<const phConstraintHalfSpace::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintHalfSpace(static_cast<const phConstraintHalfSpace::Params&>(params));
		}
		break;

	case phConstraintBase::DISTANCE:
		if (m_LittleConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_LittleConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintDistance(static_cast<const phConstraintDistance::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintDistance(static_cast<const phConstraintDistance::Params&>(params));
		}
		break;

	case phConstraintBase::FIXED:
		if (m_LittleConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_LittleConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintFixed(static_cast<const phConstraintFixed::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintFixed(static_cast<const phConstraintFixed::Params&>(params));
		}
		break;

	case phConstraintBase::HINGE:
		if (m_BigConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_BigConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintHinge(static_cast<const phConstraintHinge::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintHinge(static_cast<const phConstraintHinge::Params&>(params));
		}
		break;

	case phConstraintBase::SPHERICAL:
		if (m_LittleConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_LittleConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintSpherical(static_cast<const phConstraintSpherical::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintSpherical(static_cast<const phConstraintSpherical::Params&>(params));
		}
		break;

	case phConstraintBase::ROTATION:
		if (m_LittleConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_LittleConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintRotation(static_cast<const phConstraintRotation::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintRotation(static_cast<const phConstraintRotation::Params&>(params));
		}
		break;

	case phConstraintBase::FIXEDROTATION:
		if (m_LittleConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_LittleConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintFixedRotation(static_cast<const phConstraintFixedRotation::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintFixedRotation(static_cast<const phConstraintFixedRotation::Params&>(params));
		}
		break;

	case phConstraintBase::ATTACHMENT:
		if (m_LittleConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_LittleConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintAttachment(static_cast<const phConstraintAttachment::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintAttachment(static_cast<const phConstraintAttachment::Params&>(params));
		}
		break;

	case phConstraintBase::PRISMATIC:
		if (m_BigConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_BigConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintPrismatic(static_cast<const phConstraintPrismatic::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintPrismatic(static_cast<const phConstraintPrismatic::Params&>(params));
		}
		break;

	case phConstraintBase::CYLINDRICAL:
		if (m_LittleConstraints)
		{
			newConstraint = static_cast<phConstraintBase*>(m_LittleConstraints->New());
			if (newConstraint)
			{
				::new ((void*)newConstraint) phConstraintCylindrical(static_cast<const phConstraintCylindrical::Params&>(params));
			}
		}
		else
		{
			newConstraint = rage_new phConstraintCylindrical(static_cast<const phConstraintCylindrical::Params&>(params));
		}
		break;
	}
	
	sysMemEndTemp();

	//Check whether construction failed
	if( newConstraint != NULL 
		&& newConstraint->IsFlaggedForDestruction() )
	{
		DestructAndFree(newConstraint);
		newConstraint = NULL;
	}

	return newConstraint;
}

void phConstraintMgr::DestructAndFree(phConstraintBase* constraint)
{
	if(constraint == NULL)
	{
		return;
	}

	switch (constraint->GetType())
	{
	case phConstraintBase::HALFSPACE:
	case phConstraintBase::DISTANCE:
	case phConstraintBase::FIXED:
	case phConstraintBase::SPHERICAL:
	case phConstraintBase::ROTATION:
	case phConstraintBase::ATTACHMENT:
	case phConstraintBase::FIXEDROTATION:
	case phConstraintBase::CYLINDRICAL:
		if (m_LittleConstraints)
		{
			constraint->~phConstraintBase();
			Assert(!m_LittleConstraints->IsInFreeList(constraint));
			m_LittleConstraints->Delete(constraint);
		}
		else
		{
			delete constraint;
		}
		break;

	case phConstraintBase::HINGE:
	case phConstraintBase::PRISMATIC:
		if (m_BigConstraints)
		{
			constraint->~phConstraintBase();
			Assert(!m_BigConstraints->IsInFreeList(constraint));
			m_BigConstraints->Delete(constraint);
		}
		else
		{
			delete constraint;
		}
		break;

	default:
		Assert(false);
	}
}

bool phConstraintMgr::IsValidComponent(const phInst *pInst, u16 component)
{
	bool validComponent = component == 0;
	if (component != 0)
	{
		if (pInst && pInst->GetArchetype() && pInst->GetArchetype()->GetBound())
		{
			phBound *bound = pInst->GetArchetype()->GetBound();
			if (bound->IsTypeComposite(bound->GetType()))
			{
				if( Verifyf( static_cast<phBoundComposite*>(bound)->GetBound( component ), "phConstraintMgr::IsValidComponent - Invalid component supplied.  Component is %u is not valid.", 
					component ) )
				{
					validComponent = true;
				}
			}
		}
	}

	return validComponent;
}

phConstraintHandle phConstraintMgr::InsertAndReturnTemporaryPointer(const phConstraintBase::Params& params, phConstraintBase*& newConstraint)
{
	phConstraintHandle handle;

	if (!Verifyf(IsValidComponent(params.instanceA, params.componentA) && IsValidComponent(params.instanceB, params.componentB), 
		"phConstraintMgr::InsertAndReturnTemporaryPointer - Invalid component supplied.  Constraint will not be created."))
	{
		return handle;
	}

	sysCriticalSection critSec(s_AddAndRemoveCritSec);

	// Fail if we have filled up the active constraint array
	if (m_NumVirtualConstraints < m_MaxNumActiveConstraints)
	{
		newConstraint = AllocateAndConstruct(params);

		if (newConstraint)
		{

			// Record the location in the game code this constraint came from
			NOTFINAL_ONLY(newConstraint->SetOwner(sm_OwnerFile, sm_OwnerLine);)

			// Get the active index for the constraint
			u16 constraintActiveIndex = m_NumVirtualConstraints++;

			// Pop an index off the available index stack
			u16 constraintIndex = m_AvailableIndices[constraintActiveIndex];

			// Record the index in the active index array
			m_ActiveIndices[constraintActiveIndex] = constraintIndex;

			// Fetch the constraint record
			ConstraintRecord& record = m_VirtualConstraints[constraintIndex];

			// Remember the constraint we allocated
			record.constraint = newConstraint;

			// Also remember where we are in the active array for instant removal
			record.activeIndex = constraintActiveIndex;

			// And, remember what our index is into the constraint record array so we can retrieve the record
			handle.index = constraintIndex;

			// Ensure a legit generation is never zero
			u16 generation = record.generation;
			if (generation == 0)
			{
				record.generation = generation = 1;
			}
			handle.generation = generation;

			newConstraint->SetHandle(handle);
		}
	}

#if __BANK
	if(!newConstraint)
	{
		static bool bOnce = true;
		if (bOnce)
		{
			Assertf(m_NumVirtualConstraints < m_MaxNumActiveConstraints, "phConstraintMgr ran out of constraints - dumping list");
			Displayf("phConstraintMgr ran out of constraints! %d out of %d", m_NumVirtualConstraints, m_MaxNumActiveConstraints);
			phConstraintMgr::DumpConstraints();

			bOnce = false;
		}
	}
#endif

	return handle;
}

void phConstraintMgr::Remove(phConstraintHandle handle)
{
	// Remove a constraint given its handle
	if (handle.generation > 0)
	{
		// First, verify the generation id
		ConstraintRecord& record = m_VirtualConstraints[handle.index];
		if (record.generation == handle.generation)
		{
			record.constraint->FlagForDestruction();
		}
	}
}

void phConstraintMgr::Remove(u16 index)
{
	s_AddAndRemoveCritSec.Lock();

	// Decrement the number of virtual constraints
	--m_NumVirtualConstraints;

	// Put the index back into the available stack
	m_AvailableIndices[m_NumVirtualConstraints] = index;

	ConstraintRecord& record = m_VirtualConstraints[index];

	// Increment the generation id of this constraint to make sure any further references to this record fail
	u16 generation = record.generation + 1;

	// Ensure a legit generation is never zero
	if (generation == 0)
	{
		generation = 1;
	}

	record.generation = generation;

	// Find where this constraint is in the active indices
	u16 activeIndex = record.activeIndex;

	// Find out the index of the constraint we'll be swapping into place over the removed one
	u16 movingIndex = m_ActiveIndices[m_NumVirtualConstraints];

	// Tell the constraint that we're moving what its new active index is
	m_VirtualConstraints[movingIndex].activeIndex = activeIndex;

	// Move the new index on top of the one we're removing
	m_ActiveIndices[activeIndex] = movingIndex;

	s_AddAndRemoveCritSec.Unlock();

	// Recycle the constraints and give them a chance to clean up after themselves
	phConstraintBase* constraint = record.constraint;

	DestructAndFree(constraint);
}

phConstraintBase* phConstraintMgr::GetTemporaryPointer(phConstraintHandle handle)
{
	// Check for validity
	if (handle.IsValid())
	{
		// Check that the generation id is current
		ConstraintRecord& record = m_VirtualConstraints[handle.index];
		if (record.generation == handle.generation)
		{
			phConstraintBase* constraint = record.constraint;

			if (constraint->IsInstAValid() && constraint->IsInstBValid())
			{
				return record.constraint;
			}
		}
	}

	return NULL;
}

#if !__FINAL
void phConstraintMgr::DumpConstraints()
{
	phConstraintMgr* cm = PHCONSTRAINT;

	Displayf("Virtual Constraints:");
	Displayf("Index\tAddress\tType\tInstA\tUserDataA\tArchA\tNameA\tInstB\tUserDataB\tArchB\tNameB\tFile Line");
	int numVirtualConstraints = cm->m_NumVirtualConstraints;
	for (int index = 0; index < numVirtualConstraints; ++index)
	{
		phConstraintBase* constraint = cm->m_VirtualConstraints[cm->m_ActiveIndices[index]].constraint;

		const char* typeName = constraint->GetTypeName();
		phInst* instA = constraint->GetInstanceA();
		phInst* instB = constraint->GetInstanceB();
		void* userDataA = instA ? instA->GetUserData() : NULL;
		void* userDataB = instB ? instB->GetUserData() : NULL;
		phArchetype* archA = instA ? instA->GetArchetype() : NULL;
		phArchetype* archB = instB ? instB->GetArchetype() : NULL;
		const char* nameA = archA ? archA->GetFilename() : NULL;
		const char* nameB = archB ? archB->GetFilename() : NULL;
		const char* file = constraint->GetOwnerFile();
		int line = constraint->GetOwnerLine();

		Displayf("%d:\t0x%p\t%s\t0x%p\t0x%p\t0x%p\t%s\t0x%p\t0x%p\t0x%p\t%s\t%s %d", index, constraint, typeName, instA, userDataA, archA, nameA, instB, userDataB, archB, nameB, file, line);
	}

	Displayf("End of constraints.");
}
#endif


#if __ASSERT
bool phConstraintMgr::IsInFreeList(const phConstraintBase *constraint) const
{
	if(m_LittleConstraints != NULL)
	{
		if(m_LittleConstraints->IsInFreeList(constraint))
		{
			return true;
		}
	}

	if(m_BigConstraints != NULL)
	{
		if(m_BigConstraints->IsInFreeList(constraint))
		{
			return true;
		}
	}

	return false;
}
#endif	// __ASSERT


void phConstraintMgr::RemoveActiveConstraints (const phInst* instance)
{
	PHLOCK_SCOPEDREADLOCK;

	// Find this instance everywhere it occurs in the list of constraints.
	for (u16 index = 0; index < m_NumVirtualConstraints; ++index)
	{
		phConstraintBase* constraint = m_VirtualConstraints[m_ActiveIndices[index]].constraint;

		if (constraint->GetInstanceA() == instance || constraint->GetInstanceB() == instance)
		{
			constraint->FlagForDestruction();
		}
	}
}

#if USE_CONSTRAINT_MGR_RELEASE_MANIFOLD
void phConstraintMgr::ReleaseManifold(phManifold* manifold)
{
	if (manifold->GetNumContacts() > 0)
	{
		Assert(manifold->GetNumContacts() == 1);
		manifold->GetContactPoint(0).ActivateContact(false);
	}

	m_ReleasedManifolds.PushAndGrow(manifold);
}
#endif // USE_CONSTRAINT_MGR_RELEASE_MANIFOLD

void phConstraintMgr::EnforceWorldConstraints()
{
	u32 count = m_WorldConstraints.GetCount();
	for (u16 index = 0; index < count; ++index)
	{
		if (phConstraintBase* constraint = GetTemporaryPointer(m_WorldConstraints[index]))
		{
			constraint->EnforceWorldConstraints();
		}
	}
}

void phConstraintMgr::BreakConstraints()
{
	PF_FUNC(BreakConstraints);

	for (u16 index = 0; index < m_NumVirtualConstraints; ++index)
	{
		phConstraintBase* constraint = m_VirtualConstraints[m_ActiveIndices[index]].constraint;
		
		if( constraint->IsAtLimit() )
		{
			constraint->UpdateBreaking();
		}
	}
}

void phConstraintMgr::ReplaceInstanceComponents(int originalLevelIndex, const atFixedBitSet<128>& componentBits, int newLevelIndex, int newGenerationId)
{
	for (int index = 0; index < m_NumVirtualConstraints; ++index)
	{
		phConstraintBase* constraint = m_VirtualConstraints[m_ActiveIndices[index]].constraint;
		constraint->ReplaceInstanceComponents(originalLevelIndex, componentBits, newLevelIndex, newGenerationId);
	}
}

bool phConstraintMgr::IsConstrainedToWorld(const phInst* instance) const
{
	for (u16 index = 0; index < m_NumVirtualConstraints; ++index)
	{
		phConstraintBase& constraint = *m_VirtualConstraints[m_ActiveIndices[index]].constraint;
		if (   (constraint.GetInstanceA()==instance && constraint.GetInstanceB()==NULL)
			|| (constraint.GetInstanceA()==NULL     && constraint.GetInstanceB()==instance)	)
		{ return true; }
	}

	return false;
}

bool phConstraintMgr::AreConstrained(const phInst* instance1, const phInst* instance2) const
{
	for (u16 index = 0; index < m_NumVirtualConstraints; ++index)
	{
		phConstraintBase& constraint = *(m_VirtualConstraints[m_ActiveIndices[index]].constraint);
		if (   (constraint.GetInstanceA()==instance1 && constraint.GetInstanceB()==instance2)
			|| (constraint.GetInstanceA()==instance2 && constraint.GetInstanceB()==instance1)	)
		{ return true; }
	}

	return false;
}

bool phConstraintMgr::IsConstrained(const phInst* instance) const
{
	for (u16 index = 0; index < m_NumVirtualConstraints; ++index)
	{
		if (m_VirtualConstraints[m_ActiveIndices[index]].constraint->GetInstanceA() == instance ||
			m_VirtualConstraints[m_ActiveIndices[index]].constraint->GetInstanceB() == instance)
        {
            // Verify that this constraint hasn't been flagged for deletion.
            ConstraintRecord& record = m_VirtualConstraints[m_ActiveIndices[index]];
            if (record.constraint->IsFlaggedForDestruction())
            {
                continue;
            }

			return true;
        }
	}

	return false;
}

} // namespace rage
