// 
// physics/constraintmgr.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_CONSTRAINTMGR_H 
#define PHYSICS_CONSTRAINTMGR_H 

#include "constraintbase.h"
#include "constrainthandle.h"

#include "atl/pool.h"
#include "grprofile/drawcore.h"

#define PH_CONSTRAINT_ASSERT_DEPTH 15.0f
#define USE_CONSTRAINT_MGR_RELEASE_MANIFOLD 0

namespace rage {

class phConstraintBase;
class phConstraintDistance;
class phConstraintFixed;
class phConstraintFixedRotation;
class phConstraintHinge;
class phConstraintSpherical;
class phConstraintAttachment;
class phConstraintPrismatic;
class phForceSolver;
class phInst;
template <class T> class phPool;
class Vector3;

class phConstraintMgr
{
public:
	phConstraintMgr();
	~phConstraintMgr();

	void Reset();

#if __PFDRAW
	void ProfileDraw() const;
	void ProfileDraw(const phInst* inst) const;
#endif

	void UpdateConstraintContacts(float invTimeStep, bool addToSolver);

	void AddExtraIterManifold(phManifold* manifold);
	void PerformExtraConstraintIterations(float invTimeStep, bool usePushes, int iterations, phForceSolver& forceSolver);

	void CreateConstraints (int numManagedConstraints, int numBigConstraints, int numLittleConstraints, int maxNumConstraints=-1);

#if !__FINAL
	void SetOwner(const char* file, int line)
	{
		sm_OwnerFile = file;
		sm_OwnerLine = line;
	}
#endif // !__FINAL
	//////////////////////////////////////////////////////////////////////////
	// Virtual constraint interface

	// PURPOSE: Allocate a constraint and insert it into the simulation
	// PARAMS:
	//	params - One of the phConstraint*::Params structures
	// RETURN: The handle associated with the constraint that was allocated, or an invalid handle if the allocation failed
	phConstraintHandle Insert(const phConstraintBase::Params& params);
	phConstraintHandle InsertAndReturnTemporaryPointer(const phConstraintBase::Params& params, phConstraintBase*& newConstraint);

	// PURPOSE: Remove a constraint from the simulation and recycle the memory associated with it
	// PARAMS:
	//	handle - the handle of the constraint to remove, returned by Insert
	// NOTES:  - Constraints will automatically remove themselves when either of the instances they are attached to leaves the simulation.
	//         - If the constraint was already removed, this function has no effect.
	void Remove(phConstraintHandle handle);

	// PURPOSE: Retrieve a pointer directly to a virtual constraint, for modifying it after creation
	// PARAMS:
	//	handle - the handle of the constraint to retrieve, returned by Insert
	// RETURN:	a pointer to the constraint, or NULL if the constraint has already been Removed
	// NOTE:  This pointer should not be kept while phSimulator::Update runs. Obtain a new pointer for the next frame instead.
	phConstraintBase* GetTemporaryPointer(phConstraintHandle handle);

	// PURPOSE: Loop through the stored constraints searching for ones involving the specified instance(s)
	bool AreConstrained(const phInst* instance1, const phInst* instance2) const;
	bool IsConstrained(const phInst* instance) const;
	bool IsConstrainedToWorld(const phInst* instance) const;
	
	// PURPOSE: Remove all the active constraints on the given instance.
	// PARAMS:
	//	instance - the instance on which to remove active constraints
	void RemoveActiveConstraints (const phInst* instance);

	// PURPOSE: Modify inverse inertia and mass to strictly enforce no rotation/translation on bodies constrained
	//          to the world.
	void EnforceWorldConstraints();

	// PURPOSE: Try to break the user constraints
	void BreakConstraints();

	// Hardcoding to 128 because phContactMgr::MAX_NUM_BREAKABLE_COMPONENTS would require me to pull in phsolver/contactmgr.h.  If it ever changes there we'll
	//   get a compile error anyway.
	void ReplaceInstanceComponents(int originalLevelIndex, const atFixedBitSet<128>& componentBits, int newLevelIndex, int newGenerationId);

	// Check for a valid component.  Assert and set the component to 0 if invalid.
	bool IsValidComponent(const phInst *pInst, u16 component);

#if !__FINAL
	static void DumpConstraints();
#endif

	void AddWorldConstraint(phConstraintHandle constraint)
	{
		m_WorldConstraints.PushAndGrow(constraint);
	}

	bool HasWorldConstraints()
	{
		return m_WorldConstraints.GetCount() > 0;
	}

private:
	void Remove(u16 index);
	bool IsInFreeList(const phConstraintBase *constraint) const;
	phConstraintBase* AllocateAndConstruct(const phConstraintBase::Params& params);
#if USE_CONSTRAINT_MGR_RELEASE_MANIFOLD
	void ReleaseManifold(phManifold* manifold);
#endif // USE_CONSTRAINT_MGR_RELEASE_MANIFOLD
	void DestructAndFree(phConstraintBase* constraint);

	int m_MaxNumActiveConstraints;
#if USE_CONSTRAINT_MGR_RELEASE_MANIFOLD
	atArray<phManifold*> m_ReleasedManifolds;
#endif // USE_CONSTRAINT_MGR_RELEASE_MANIFOLD

	// Virtual constraints
	struct ConstraintRecord
	{
		phConstraintBase* constraint;
		u16 generation;
		u16 activeIndex;

		ConstraintRecord() : generation(0) { }
	};

	ConstraintRecord* m_VirtualConstraints;
	u16* m_ActiveIndices;
	u16* m_AvailableIndices;
	u16 m_NumVirtualConstraints;

	atArray<phManifold*> m_ExtraIterManifolds;
	atArray<phConstraintHandle> m_WorldConstraints;

	atPoolBase* m_BigConstraints;
	atPoolBase* m_LittleConstraints;

#if !__FINAL
	const char* sm_OwnerFile;
	int sm_OwnerLine;
#endif
	friend class phConstraintAttachment;
	friend class phConstraintPrismatic;
};

} // namespace rage

#endif // PHYSICS_CONSTRAINTMGR_H 
