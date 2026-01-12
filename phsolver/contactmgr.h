//
// physics/contactmgr.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_CONTACTMGR_H
#define PHYSICS_CONTACTMGR_H

#include "forcesolver.h"

#include "atl/bitset.h"
#include "atl/functor.h"
#include "physics/contactiterator.h"

#if __PS3
#include <cell/spurs/task.h>
class CellSpursTaskset;
class CellSpursQueue;
#endif

namespace rage {

template <class _Type> class atPool;
class phBreakData;
class phContact;
class phInstBreakable;
class phSleepIsland;
class phWorkerThread;
struct sysTaskParameters;
class sysBarrier;

// PURPOSE
//   The contact manager uses all the contacts in a single frame to find collision impulses
//   simultaneously for each set of interacting objects.
// NOTES
//   1.	The contact manager is optional. When it is not used, the simulator calculates collision impulses
//      using only pairwise collisions.
// <FLAG Component>
class phContactMgr : datBase
{
public:

	phContactMgr(int scratchpadSize, int maxExternVelManifolds, phOverlappingPairArray* pairArray);
	~phContactMgr();

#if __BANK
	static void AddWidgets (bkBank& bank);
#endif

	u8* GetScratchpad() { return m_Scratchpad; }
	unsigned int GetScratchpadSize() { return m_ScratchPadSize; }
	unsigned int GetScratchpadMaxUsed() { return m_ScratchPadMaxUsed; }
	void MarkScratchpadUsed(u32 used);

	void Reset();

	// PURPOSE: Set the number of iterations used in the collision solver.
	// PARAMS:
	//	numIterations - the number of iterations to be used in the collision solver
	// NOTES:	This can be lowered from its default value for faster and less accurate collisions, or raised for slower and more accurate collisions.
	static void SetLCPSolverIterations (int numIterations);

	// PURPOSE: Set the number of iterations used in the collision solver during the final phase.
	// PARAMS:
	//	numIterations - the number of iterations to be used in the collision solver in the final phase.
	// NOTES:	This can be raised to increase the stability of constraints attached to the world.
	static void SetLCPSolverIterationsFinal (int numIterations);

	// PURPOSE: Get the number of iterations used in the collision solver.
	// RETURN:	the number of iterations to be used in the collision solver
	static int GetLCPSolverIterations ();

	// PURPOSE: Get the number of iterations used in the collision solver in the final phase.
	// RETURN:	the number of iterations to be used in the collision solver in the final phase
	static int GetLCPSolverIterationsFinal ();

	static int GetNumLCPSolverIterations ();

	// PURPOSE: Set the separation bias used for Baumgarte forces.
	// PARAMS: bias - The new bias to use
	// NOTES: Baumgarte forces are used to correct position error when pushes are off.
	static void SetSeparationBias (float bias) { sm_SeparationBias = bias; }
	static float GetSeparationBias () { return sm_SeparationBias; }

	static void SetUsePushes(bool enable) { sm_UsePushes = enable; }
	static bool GetUsePushes() { return sm_UsePushes; }

	static bool GetUseSleepIslands() { return sm_UseSleepIslands; }

	static void SetBreakingEnabled(bool enabled) { sm_BreakingEnabled = enabled; }

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	// **** New contact mgr update ***

	// PURPOSE: Needs to be called in order for UpdateNew to work
	void InitNew(int maxManifolds, int maxExternVelManifolds, phOverlappingPairArray* pairArray);

	// PURPOSE: Should be called if InitNew was called during startup
	void ShutdownNew();

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	void Update(phOverlappingPairArray* pairArray, float timeStep);

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	// PURPOSE: Assign the pairs from the pair arrays to buckets which can be solved simultaneously
	// PARAMS:
	//     pairArray - The input, the array of pairs that needs to be solved
	// NOTES: This is called while collisions are still in progress, since the results of collision are not needed
	void SplitPairs(phOverlappingPairArray* pairArray);

	// PURPOSE: Diagnostic function, outputs the results to SplitPairs to the TTY
	void PrintPairs();

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	// PURPOSE: Performs per-manifold work that needs to be done before the solver can run
	// PARAMS:
	//     pairArray - The array of pairs that contains the contacts that need to be updated
	void UpdateContacts(phOverlappingPairArray* pairArray, float timeStep, bool bJustRunConstraintFunctions, bool computeBounceAndTangent);

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	// PURPOSE: Iterates over all the contact points in the manifolds that have at least one inst that has an externally 
	//          controlled velocity to report, and calculates the target relative velocity for that contact point.
	void UpdateContactsExternVelocity();

	// PURPOSE: Accessor that allows one to get the array containing manifolds with at least 
	//          one inst with externally controlled velocity.
	atArray<phManifold*>& GetManifoldsWithExternVelocity() { return m_ManifoldsWithExternVel; }

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	// PURPOSE: The top level function for constraint solving, this function also handles breaking
	// PARAMS:
	//     pairArray - The array of pairs, indexed by paraGroup into buckets
	//     timeSlice - The time interval this solve is stretching over
	//     paraGroup - The buckets that the constraints have been divided into, output by SplitPairs
	//     numPara - The number of parallel groups, also output by SplitPairs
	void SolveConstraints(phOverlappingPairArray* pairArray, float timeStep);

	// PURPOSE:  Compute and apply impulses for a given set of constraints and their modified inverse masses and inertias.
	// PARAMS:
	//     pairArray - The array of pairs, indexed by paraGroup into buckets
	//     timeSlice - The time interval this solve is stretching over
	//     paraGroup - The buckets that the constraints have been divided into, output by SplitPairs
	//     numPara - The number of parallel groups, also output by SplitPairs
	//     numIterations - The number of iterations before the solver terminates
	//	   warmStart - during preresponse apply the previous solution to the velocity
	void SolveImpulses(phOverlappingPairArray* pairArray, float timeStep, u32 numIterations, bool applyWarmStart, bool clearWarmStart, bool preResponse);

	// PURPOSE: Call SolveImpulses() twice, changing inverse mass/inertia with EnforceWorldConstraints in-between.         
	// PARAMS:
	//     pairArray - The array of pairs, indexed by paraGroup into buckets
	//     timeSlice - The time interval this solve is stretching over
	//     paraGroup - The buckets that the constraints have been divided into, output by SplitPairs
	//     numPara - The number of parallel groups, also output by SplitPairs
	void SolveImpulsesPhase1And2(phOverlappingPairArray* pairArray, float timeStep, bool applyWarmStart, bool clearWarmStart);

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	// PURPOSE: Called by CollectImpulsesFromPair to detect breaking.
	// PARAMS: manifold - The manifold to process
	void CollectImpulsesFromManifold(phManifold* manifold, bool collectForA, bool collectForB);

	// PURPOSE: Called by BreakObjects to detect breaking. Calls CollectImpulsesFromManifold on each manifold in the pair.
	// PARAMS: pair - The pair to process
	void CollectImpulsesFromPair(phTaskCollisionPair& pair);

	// PURPOSE: Called for each island to see if anything in the island is breaking.
	void BreakObjectsInIsland();

	// PURPOSE: See if any of the objects will break, and break them
	// PARAMS: pairArray - Both input and output. On input, the array of pairs that we want to try to break. If some of them
	//					   broke, on output contains all the pairs that are in islands that contained breaking insts, that
	//                     need to be re-solved.
	// RETURNS: true if some object broke, false otherwise
	bool BreakObjects(phOverlappingPairArray*& pairArray);

	// Reset the BreakingPairs list to the overlapping pairs array
	void ResetBreakingPairs(phOverlappingPairArray* pairs);

	// PURPOSE: Gather all the pairs that are to receive optional iterations either according to the user-set FLAG_OPTIONAL_ITERATIONS in phInst 
	// PARAMS: pairArray - Both input and output. On input, the array of pairs that we are checking for the optional iterations flag. 
	//					   Output contains all the pairs that either have the flag or shared an island with a pair that had the flag.
	// RETURNS: true if some objects need optional iterations, false otherwise
	bool GatherOptionalIterPairs(phOverlappingPairArray*& pairArray);

	// PURPOSE: Called by GatherOptionalIterPairs to check for FLAG_OPTIONAL_ITERATIONS in each phInst in the pair. 
	// PARAMS: pair - The pair to process
	void CheckPairForOptionalItersFlag(phTaskCollisionPair& pair);

	// PURPOSE: Called for each island to see if the island pairs need optional iterations.
	void IslandNeedsOptionalIters();

	phInstBreakable* FindWeakestInst (u16* breakInstList,
									  Vector4* breakingImpulses,
									  Vector4* breakingPositions,
									  int numBreakInsts,
									  float* breakStrength,
									  phBreakData** breakData,
									  phBreakData** testBreakData) const;

	static const int MAX_NUM_BREAKABLE_COMPONENTS = 128;
	// Just like it sounds
	void RemoveAllContactsWithInstance (phInst* instance);
	void RemoveAllContactsWithInstance (phInst* instance, atFixedBitSet<MAX_NUM_BREAKABLE_COMPONENTS>& componentBits);

	// Finds all contacts that involve the given instance and zeroes out their warm-start (previous solution) information so that the solver starts from scratch.
	// This is useful if you're changing something about an instance *but not changing it's bounds*.  (You might, for example, change from articulated to rigid
	//   or vice-versa, or change the mass or angular inertia of the object).  Most likely the geometric data in the existing contacts is still valuable but the
	//   previous computed solutions are not going to be reliable.
	void ResetWarmStartAllContactsWithInstance (const phInst* instance);

	void UpdateColliderInManifoldsWithInstance (const phInst* instance, phCollider* collider);

	void ReplaceInstanceComponents (phInst* instance, atFixedBitSet<MAX_NUM_BREAKABLE_COMPONENTS>& componentBits, phInst* newInstance);

	// PURPOSE: Apply a feedback impulse to each manifold that contributed to breaking the given instance
	// PARAMS:
	//   newlyBrokenInstance - the instance that just broke
	//   impulseScale - the fraction of (required breaking force)/(applied force). This should be between 0 and 1.
	void ApplyPostBreakingImpulses (phInst& newlyBrokenInstance, bool wasOriginalInstanceFixed,ScalarV_In impulseScale);

	// PURPOSE: Throw away the result of impulse solving, because some objects in the island broke
	// PARAMS:
	//     objects - An array of level indices
	//     numObjects - The number of objects in the objects array.
	void RevertImpulses(u16* objects, int numObjects);

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	// PURPOSE: Try to put objects to sleep
	void UpdateSleep();

	// PURPOSE: This function has two modes: either asking the inst whether to vote for sleep, or telling the inst to sleep
	// PARAMS: levelIndex - The inst to either put to sleep or ask for its sleeping vote
	bool SleepInst(int levelIndex, int objectIndex);

	// PURPOSE: Tally the votes for the island to sleep, and then put it to sleep if the vote was unanimous
	bool NextSleepIsland(int numObjects);

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	void DoPreComputeImpacts(phOverlappingPairArray* pairArray);

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

	// PURPOSE: Prepare the other half scratchpad for allocating via StartHalfScratchpad/EndHalfScratchpad
	size_t GetHalfScratchpadSize();

	// PURPOSE: Prepare the other half scratchpad for allocating via StartHalfScratchpad/EndHalfScratchpad
	void InitNextHalfScratchpad();

	// PURPOSE: Redirects all allocations into a miniheap within the current half of the scratchpad
	void StartHalfScratchpad();

	// PURPOSE: Ends the miniheap, so allocation return back to normal.
	void EndHalfScratchpad();

	//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

private:

	void ApplyPostBreakingImpulsesOnManifold(phManifold& breakingManifold, int otherObjectSlot, bool isArticulatedFixedCollision, phCollider& otherCollider, ScalarV_In cgOffsetScale, ScalarV_In signedImpulseScale);

	void FillInForceSolverMembers(phForceSolver& forceSolver, u32 numIterations, float timeStep, bool applyWarmStart, bool clearWarmStart, bool preResponse, phOverlappingPairArray* pairArray);

	// PURPOSE: Initialize a new contact group by setting its head contact to NULL and its number of contacts to zero, and incrementing the number of groups.
	// RETURN:	the index number of the new contact group
	//	int AddContactGroup ();

	// begin - phForceSolver related -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

	// PURPOSE: The maximum number of phases to split up the pairs in SplitPairs
	static const u32 MAX_PHASES = 1024;

#if !__SPU
	phForceSolver* m_ForceSolvers;
#endif // !__SPU

	sysTaskParameters** m_ForceSolverParams;
	phWorkerThread* m_ForceSolverTasks;
	phWorkerThread* m_ForceSolverPushTasks;
	phWorkerThread* m_UpdateContactsTasks;

#if __PS3
	u32 m_MaxUpdateContactsFragSize;
#endif // __PPU

	u32 m_PhaseAllocateAtom;
	u64 m_PhaseReleaseAtom;
	sysIpcSema m_PhaseReleaseSema;

#define TRACK_PHASE_RELEASE_SEMA_COUNT 0

	u32 m_CurrentWindowSize;

	phForceSolver::ParallelGroup* m_ParaGroup;
	u32 m_NumPara;
	u32 m_InitialPhasesToRelease;

	bool m_IslandWantsToSleep;
	bool m_SleepingCurrentIsland;
	int m_ObjectsToSleep;
	phSleepIsland* m_SleepIsland;

	int m_ImpulsesArraySize;
	Vec4V* m_ImpulsesByComponent;
	Vec4V* m_PositionsByComponent;

	int m_CurrentIsland;
	int m_FirstPairInIsland;
	bool m_AnyBroken;
	bool m_ContainedInstNeedsOptionalIters;
	phOverlappingPairArray* m_BreakingPairs;
	phOverlappingPairArray* m_PreviousBreakingPairs;
	phOverlappingPairArray* m_OptionalIterPairs;

	int m_WhichScratchpadHalf;
	u8* m_ScratchpadCurrentTop;
	u8* m_ScratchpadCurrentBottom;

	atArray<phManifold*> m_ManifoldsWithExternVel;

	u8* m_Scratchpad;
	u32 m_ScratchPadSize;
	u32 m_ScratchPadMaxUsedThisFrame;
	u32 m_ScratchPadMaxUsed;

	static int sm_LCPSolverIterations;
	static int sm_LCPSolverIterationsFinal;
	static int sm_LCPSolverIterationsExtra;
	static int sm_LCPSolverIterationsOptional;
	static float sm_SeparationBias;
	static float sm_MinBounce;
	static bool sm_BaumgarteJointLimitTerm;
	static bool sm_UsePushes;
#if TURN_CLAMPING
	static float sm_MaxTurn;
#endif
	static bool sm_BreakingEnabled;
	static float sm_BreakImpulseScale;
	static float sm_BreakImpulseCGOffset;
	static bool sm_OptionalItersEnabled;
	static bool sm_DontCombineArticulated;
	static int sm_MaxPairsPerPhase;
	static int sm_MaxArticulatedPerPhase;
	static bool sm_UseSleepIslands;


#if __PS3 && __BANK
	static int sm_PrintPairsAlsoPrintsDmaPlans;
#endif
};

inline int phContactMgr::GetNumLCPSolverIterations ()
{
	return sm_LCPSolverIterations;
}


} // namespace rage

void forcesolver(rage::sysTaskParameters& p);

#endif	// end of #ifndef PHCOLLIDE_CONTACTMGR_H
