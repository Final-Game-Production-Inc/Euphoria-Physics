//
// physics/contactmgr.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "contactmgr.h"

#define PHCONTACT_USE_TASKS			(1 && !TEST_REPLAY)

#include "forcesolvertask.h"
#include "forcesolvertables.h"

#include "atl/pool.h"
#include "bank/bank.h"
#include "data/callback.h"
#include "diag/tracker.h"
#include "math/simplemath.h"
#include "pharticulated/jointdispatch.h"
#include "phbound/boundcomposite.h"
#include "phcore/constants.h"
#include "phcore/material.h"
#include "phcore/pool.h"
#include "phcore/workerthread.h"
#include "phcore/sputaskset.h" 
#include "physics/broadphase.h"
#include "physics/colliderdispatch.h"
#include "physics/contacttask.h"
#include "physics/debugcontacts.h"
#include "physics/instbehavior.h"
#include "physics/instbreakable.h"
#include "physics/levelnew.h"
#include "physics/overlappingpairarray.h"
#include "physics/simulator.h"
#include "physics/sleep.h"
#include "physics/sleepmgr.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "profile/timebars.h"
#include "system/ipc.h"
#include "system/memory.h"
#include "system/miniheap.h"
#include "system/param.h"
#include "system/performancetimer.h"
#include "system/task.h"
#include "system/cache.h"
#include "vectormath/classes.h"
#include "vectormath/legacyconvert.h"
#include "physics/physicsprofilecapture.h"

#define INCLUDE_DETAIL_PHYS_CMGR_TIMEBARS 0

#if RAGE_TIMEBARS
#if INCLUDE_DETAIL_PHYS_CMGR_TIMEBARS || INCLUDE_DETAIL_TIMEBARS
#define PF_PUSH_TIMEBAR_PHYS_CMGR_DETAIL(name)				PF_PUSH_TIMEBAR_DETAIL(name)
#define PF_START_TIMEBAR_PHYS_CMGR_DETAIL(name)				PF_START_TIMEBAR_DETAIL(name)
#define PF_POP_TIMEBAR_PHYS_CMGR_DETAIL()					PF_POP_TIMEBAR_DETAIL()
#define PF_AUTO_PUSH_PHYS_CMGR_DETAIL(name)					::rage::pfAutoPushTimer localPushTimer(name,true)
#else // INCLUDE_DETAIL_PHYS_CMGR_TIMEBARS
#define PF_PUSH_TIMEBAR_PHYS_CMGR_DETAIL(name)
#define PF_START_TIMEBAR_PHYS_CMGR_DETAIL(name)
#define PF_POP_TIMEBAR_PHYS_CMGR_DETAIL()
#define PF_AUTO_PUSH_PHYS_CMGR_DETAIL(name)
#endif // INCLUDE_DETAIL_PHYS_CMGR_TIMEBARS
#else // RAGE_TIMEBARS
#define PF_PUSH_TIMEBAR_PHYS_CMGR_DETAIL(name)
#define PF_START_TIMEBAR_PHYS_CMGR_DETAIL(name)
#define PF_POP_TIMEBAR_PHYS_CMGR_DETAIL()
#define PF_AUTO_PUSH_PHYS_CMGR_DETAIL(name)
#endif // RAGE_TIMEBARS

#define CONTACTMGR_CHECK_FORCE_SOLVER_DMA	0
#if CONTACTMGR_CHECK_FORCE_SOLVER_DMA
#include "fragment/manager.h"
#endif	// CONTACTMGR_CHECK_FORCE_SOLVER_DMA

#include <stdlib.h>

#include <algorithm>
#include <functional>

#define CALL_ACTIVE_OBJECT_GETEXTERNVELOCITY 1

SOLVER_OPTIMISATIONS()

namespace rage {

float phContactMgr::sm_SeparationBias = 0.2f;
float phContactMgr::sm_MinBounce = 1.0f;
bool phContactMgr::sm_BaumgarteJointLimitTerm = false;
bool phContactMgr::sm_UsePushes = true;
#if TURN_CLAMPING
float phContactMgr::sm_MaxTurn = EARLY_FORCE_SOLVE_ONLY(0.1f) NOT_EARLY_FORCE_SOLVE_ONLY(0.25 * PI);
#endif
bool phContactMgr::sm_BreakingEnabled = true;
float phContactMgr::sm_BreakImpulseScale = 0.4f;
float phContactMgr::sm_BreakImpulseCGOffset = 0.0f;
bool phContactMgr::sm_OptionalItersEnabled = true;
bool phContactMgr::sm_DontCombineArticulated = false;
int phContactMgr::sm_MaxPairsPerPhase = phForceSolver::MAX_BATCHES_NUM;
int phContactMgr::sm_MaxArticulatedPerPhase = 2;
bool phContactMgr::sm_UseSleepIslands = true;
int phContactMgr::sm_LCPSolverIterations = 4;
int phContactMgr::sm_LCPSolverIterationsFinal = 4;
int phContactMgr::sm_LCPSolverIterationsExtra = 6;
int phContactMgr::sm_LCPSolverIterationsOptional = 4;

PARAM(postbreakimpulsescale,"[physics] the scale of the impulse applied back to objects when they break other objects");
PARAM(postbreakimpulsecgoffset,"[physics] how far off from the CG post break impulses are applied");

namespace phContactStats
{
	PF_PAGE(PHContacts,"ph Contacts");

	PF_GROUP(Update);
	PF_LINK(PHContacts,Update);
	PF_TIMER(UpdateContacts,Update);
	PF_TIMER(UpdateContactsExternVelocity,Update);
	PF_TIMER(SortContacts,Update);
	PF_TIMER(InitArticulated,Update);
	PF_TIMER(Solve,Update);
	PF_TIMER(ExtraConstraintIterations,Update);
	PF_TIMER(BreakObjects,Update);
	PF_TIMER(BreakConstraints,Update);
	PF_TIMER(BreakSplitPairs,Update);
	PF_TIMER(BreakSolve,Update);
	PF_TIMER(BreakUpdateContacts,Update);
	PF_TIMER(BreakTotal,Update);
	PF_TIMER(OptionalIters,Update);
	PF_TIMER(OptionalItersUpdateContacts,Update);
	PF_TIMER(OptionalItersSolve,Update);
	PF_TIMER(GatherOptionalIterPairs,Update);
	PF_TIMER(PreResponse,Update);
	PF_TIMER(PreResponseTnread,Update);
	PF_TIMER(PreResponsePhase,Update);
	PF_TIMER(ApplyImpulseThread,Update);
	PF_TIMER(ApplyImpulsePhase,Update);
	PF_TIMER(EnsureAllArtVelocitiesFullyPropagated,Update);
	PF_TIMER(phConstraintCylindricalUpdate,Update);
	PF_TIMER(phConstraintRotationUpdate,Update);

	PF_PAGE(PHMemory,"ph Memory");

	PF_GROUP(Memory);
	PF_LINK(PHMemory,Memory);

	PF_VALUE_INT(MaxMemoryUsed,Memory);
	PF_VALUE_INT(MemoryUsed,Memory);
};

using namespace phContactStats;

namespace SimulatorStats
{
	EXT_PF_TIMER(PreComputeImpacts);
	EXT_PF_TIMER(SplitPairs);
	EXT_PF_TIMER(ContactMgrUpdateContacts);
};

using namespace SimulatorStats;

void phContactMgr::Reset()
{
}

void phContactMgr::SetLCPSolverIterations (int numIterations)
{
	sm_LCPSolverIterations = numIterations;
}

void phContactMgr::SetLCPSolverIterationsFinal (int numIterations)
{
	sm_LCPSolverIterationsFinal = numIterations;
}

int phContactMgr::GetLCPSolverIterations ()
{
	return sm_LCPSolverIterations;
}

int phContactMgr::GetLCPSolverIterationsFinal ()
{
	return sm_LCPSolverIterationsFinal;
}

#if __PPU
u32 ComputeMaxFragSize(phForceSolver::SpuConstraintTable table)
{
	u32 maxFragSize = 0;

	for (int column = 0; column < FORCE_SOLVER_TABLE_COLS; ++column)
	{
		for (int row = 0; row < FORCE_SOLVER_TABLE_ROWS; ++row)
		{
			maxFragSize = Max(maxFragSize, (u32)table[row][column].codeFragSize);
		}
	}

	return maxFragSize;
}
#endif

phContactMgr::phContactMgr(int scratchpadSize, int maxExternVelManifolds, phOverlappingPairArray* pairArray)
	: m_ScratchPadSize(scratchpadSize)
{
	RAGE_TRACK(Physics);

	// Initialize the LCP solver and reset the contact manager.
	m_Scratchpad = rage_new u8[m_ScratchPadSize];
	m_ScratchpadCurrentTop = m_Scratchpad;
	m_ScratchpadCurrentBottom = m_Scratchpad + (m_ScratchPadSize >> 1);
	m_WhichScratchpadHalf = 0;

	m_ParaGroup = rage_new phForceSolver::ParallelGroup[phContactMgr::MAX_PHASES];

	m_ManifoldsWithExternVel.Reserve(maxExternVelManifolds);

#if !__PPU
	m_PhaseReleaseSema = sysIpcCreateSema(0);
#endif // !__PPU

	m_CurrentWindowSize = 0;

	ResetBreakingPairs(pairArray);

	m_ScratchPadMaxUsed = 0;

	int numWorkerThreads = phConfig::GetTotalMaxNumWorkerThreads();
	m_ForceSolvers = rage_new phForceSolver[numWorkerThreads];


	m_ForceSolverParams = rage_new sysTaskParameters*[numWorkerThreads];
	for (int threadIndex = 0; threadIndex < numWorkerThreads; ++threadIndex)
	{
		m_ForceSolverParams[threadIndex] = rage_new sysTaskParameters;
	}

	m_ForceSolverTasks = rage_new phWorkerThread(TASK_INTERFACE(SolveConstraintTask), phConfig::GetForceSolverTaskScheduler());
	m_ForceSolverPushTasks = rage_new phWorkerThread(TASK_INTERFACE(SolveConstraintTask), phConfig::GetForceSolverTaskScheduler());
	m_UpdateContactsTasks = rage_new phWorkerThread(TASK_INTERFACE(UpdateContactsTask), phConfig::GetContactTaskScheduler());

	PARAM_postbreakimpulsescale.Get(sm_BreakImpulseScale);
	PARAM_postbreakimpulsecgoffset.Get(sm_BreakImpulseCGOffset);
}


phContactMgr::~phContactMgr()
{
	delete [] m_ParaGroup;

#if !__PPU
	sysIpcDeleteSema(m_PhaseReleaseSema);
#endif

	int numWorkerThreads = phConfig::GetTotalMaxNumWorkerThreads();
	for(int threadIndex = 0; threadIndex < numWorkerThreads; threadIndex++)
	{
		delete m_ForceSolverParams[threadIndex];
	}
	delete m_UpdateContactsTasks;
	delete m_ForceSolverPushTasks;
	delete m_ForceSolverTasks;
	delete [] m_ForceSolverParams;

	delete [] m_ForceSolvers;

	delete[] m_Scratchpad;
}



bool g_UsePushesForJoints = true;
float g_JointLimitElasticity = 0.0f;
bool g_EnableBaumgarteJoints;

#if __BANK
bool g_DisableJoints = false;

void PrintPairsCallback()
{
	PHSIM->GetContactMgr()->PrintPairs();
}

#if POSITIVE_DEPTH_ONLY
static const char* POSITIVE_DEPTH_MODES[] =
{
	"Disabled",
	"Enabled",
	"Velocity"
};
#endif

void phContactMgr::AddWidgets (bkBank& bank)
{
	bank.PushGroup( "Force solver" );

	bank.AddSlider( "Solver Iterations", &sm_LCPSolverIterations, 0, 4096, 1 );
	bank.AddSlider( "Solver Iterations Final", &sm_LCPSolverIterationsFinal, 0, 4096, 1 );
	bank.AddSlider( "Solver Iterations Extra", &sm_LCPSolverIterationsExtra, 0, 4096, 1 );
	bank.AddSlider( "Solver Iterations Optional", &sm_LCPSolverIterationsOptional, 0, 4096, 1 );
	bank.AddToggle( "Solver Iterations Optional Enabled", &sm_OptionalItersEnabled );
	bank.AddSlider( "Separation Bias", &sm_SeparationBias, 0.0f, 3.0f, 0.02f );
	bank.AddSlider( "Min Bounce", &sm_MinBounce, 0.0f, 10.0f, 0.02f );
#if !EARLY_FORCE_SOLVE
	bank.AddToggle( "Use Pushes", &sm_UsePushes );
#endif // !EARLY_FORCE_SOLVE
#if TURN_CLAMPING
	bank.AddSlider( "Max Turn", &sm_MaxTurn, 0.0f, 2 * PI, 0.1f );
#endif
	bank.AddToggle( "Use Sleep Islands", &sm_UseSleepIslands );
	bank.AddToggle( "Disable Joints", &g_DisableJoints );
	bank.AddToggle( "Use Pushes For Joints", &g_UsePushesForJoints );
	bank.AddToggle( "Baumgarte For Joints", &sm_BaumgarteJointLimitTerm );
	bank.AddSlider( "Joint Limit Elasticity", &g_JointLimitElasticity, 0.0f, 1.0f, 0.02f );
#if POSITIVE_DEPTH_ONLY
	bank.AddCombo( "Positive Depth Only", &phManifold::sm_PositiveDepthMode, 3, POSITIVE_DEPTH_MODES );
#endif
	bank.AddToggle( "Breaking Enabled", &sm_BreakingEnabled );
	bank.AddSlider( "Break Impulse Scale", &sm_BreakImpulseScale, 0.0f, 1.0f, 0.01f);
	bank.AddSlider( "Break Impulse CG Offset", &sm_BreakImpulseCGOffset, 0.0f, 1.0f, 0.01f);
	bank.AddToggle( "Don't Combine Articulated", &sm_DontCombineArticulated);
	bank.AddSlider( "Max Pairs Per Phase", &sm_MaxPairsPerPhase, 1, phForceSolver::MAX_BATCHES_NUM, 1 );
	bank.AddSlider( "Max Articulated Per Phase", &sm_MaxArticulatedPerPhase, 1, phForceSolver::MAX_BATCHES_NUM, 1 );
	bank.AddButton( "Dump Ph Force Solver Pairs", datCallback(CFA(PrintPairsCallback)));

	bank.PopGroup();
}
#endif

#if CHECK_FOR_DUPLICATE_MANIFOLDS
void CheckForDuplicateManifolds(phOverlappingPairArray * pairArray);
#endif

void phContactMgr::Update (phOverlappingPairArray* pairArray, float timeStep)
{
#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(pairArray);
#endif

	m_ScratchPadMaxUsedThisFrame = 0;

	// Initialize some force solver globals (allowedPenetration, gravity, timestep)
	ScalarV invTimeStep = Invert(ScalarV(timeStep));
	phForceSolver::sm_Globals.invTimeStep = invTimeStep;
	phForceSolver::sm_Globals.allowedPenetration = ScalarVFromF32(m_ForceSolvers->m_AllowedPenetration);
	phForceSolver::sm_Globals.halfAllowedPenetration = ScalarVFromF32(m_ForceSolvers->m_AllowedPenetration) * ScalarV(V_HALF);

	PF_AUTO_PUSH_PHYS_CMGR_DETAIL("phContactMgr::UpdateNew");

	PF_START_TIMEBAR_PHYS_CMGR_DETAIL("Update contacts extern velocity");

	EARLY_FORCE_SOLVE_ONLY(if (!sm_UsePushes))
	{
		UpdateContactsExternVelocity();
	}

	PF_START_TIMEBAR_PHYS_CMGR_DETAIL("Update contacts");

	EARLY_FORCE_SOLVE_ONLY(InitNextHalfScratchpad();)

	PF_START(UpdateContacts);
	PF_START(ContactMgrUpdateContacts);
	PPC_STAT_TIMER_START(UpdateContactsTimer);
	UpdateContacts(pairArray, timeStep, false, true);
	PPC_STAT_TIMER_STOP(UpdateContactsTimer);
	PF_STOP(ContactMgrUpdateContacts);
	PF_STOP(UpdateContacts);

	PF_START_TIMEBAR_PHYS_CMGR_DETAIL("Solve Constraints");

	SolveConstraints(pairArray, timeStep);

//It isn't true that the simulator update has finished if we are doing EARLY_FORCE_SOLVE
//Also, GTA doesn't use PostSolveCallbacks anyway, so there would be no benefit to this
#if !EARLY_FORCE_SOLVE
	PHSIM->ForceSolverDone();
#endif

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(pairArray);
#endif
}

#if __ASSERT
__forceinline void  RagdollSizeAbort(const phManifold* manifold)
{
	bool isArticulatedA = manifold->GetColliderA() && manifold->GetColliderA()->IsArticulated();
	bool isArticulatedB = manifold->GetColliderB() && manifold->GetColliderB()->IsArticulated();

	const char* nameA = isArticulatedA ? manifold->GetInstanceA()->GetArchetype()->GetFilename() : "";
	const char* nameB = isArticulatedB ? manifold->GetInstanceB()->GetArchetype()->GetFilename() : "";

	if (isArticulatedA && isArticulatedB)
	{
		Assertf(false, "Manifold won't fit in a phase even all by itself...articulated bodies '%s' (0x%p) and '%s' (0x%p) are too big", nameA, manifold->GetColliderA(), nameB, manifold->GetColliderB());
	}
	else if (isArticulatedA)
	{
		Assertf(false, "Manifold won't fit in a phase even all by itself...articulated body '%s' (0x%p) is too big", nameA, manifold->GetColliderA());
	}
	else if (isArticulatedB)
	{
		Assertf(false, "Manifold won't fit in a phase even all by itself...articulated body '%s' (0x%p) is too big", nameB, manifold->GetColliderB());
	}
	else
	{
		Assertf(false, "Somehow this manifold is huge even though it doesn't have an articulated body...this probably shouldn't ever happen...!");
	}
}
#else
__forceinline void  RagdollSizeAbort(const phManifold* UNUSED_PARAM(manifold))
{
}
#endif

static const u32 SHIFT_TABLE_32[] = 
{
	1U <<  0, 1U <<  1, 1U <<  2, 1U <<  3, 1U <<  4, 1U <<  5, 1U <<  6, 1U <<  7,
	1U <<  8, 1U <<  9, 1U << 10, 1U << 11, 1U << 12, 1U << 13, 1U << 14, 1U << 15,
	1U << 16, 1U << 17, 1U << 18, 1U << 19, 1U << 20, 1U << 21, 1U << 22, 1U << 23,
	1U << 24, 1U << 25, 1U << 26, 1U << 27, 1U << 28, 1U << 29, 1U << 30, 1U << 31
};

struct ManifoldFindRecord
{
	phManifold* manifold;
	u16 levelIndexA;
	u16 levelIndexB;
};

struct ManifoldIndexSortPredicateA : public std::binary_function<const ManifoldFindRecord&, const ManifoldFindRecord&, bool>
{
	ManifoldFindRecord* m_Manifolds;

	ManifoldIndexSortPredicateA(ManifoldFindRecord* findRecords)
		: m_Manifolds(findRecords)
	{
	}

	bool operator()(const u16 left, const u16 right) const
	{
		return m_Manifolds[left].levelIndexA < m_Manifolds[right].levelIndexA;
	}
};

struct ManifoldIndexSortPredicateB : public std::binary_function<const ManifoldFindRecord&, const ManifoldFindRecord&, bool>
{
	ManifoldFindRecord* m_Manifolds;

	ManifoldIndexSortPredicateB(ManifoldFindRecord* findRecords)
		: m_Manifolds(findRecords)
	{
	}

	bool operator()(const u16 left, const u16 right) const
	{
		return m_Manifolds[left].levelIndexB < m_Manifolds[right].levelIndexB;
	}
};

#define VERIFY_MANIFOLD_FINDER 0

#if VERIFY_MANIFOLD_FINDER
#define VERIFY_MANIFOLD_FINDER_ONLY(X) X
#else
#define VERIFY_MANIFOLD_FINDER_ONLY(X)
#endif

#define TRACK_SEARCH_ALTERNATES 0

#if TRACK_SEARCH_ALTERNATES
namespace phSplitPairsStats
{
	PF_PAGE(PHSplitPairs,"ph SplitPairs");

	PF_GROUP(Stats);
	PF_LINK(PHSplitPairs,Stats);
	PF_COUNTER(Alternate0,Stats);
	PF_COUNTER(Alternate1,Stats);
	PF_COUNTER(Alternate2,Stats);
	PF_COUNTER(Alternate3,Stats);
	PF_COUNTER(Alternate4,Stats);
	PF_COUNTER(Alternate5,Stats);
	PF_COUNTER(Alternate6,Stats);
	PF_COUNTER(Alternate7,Stats);
	PF_COUNTER(Alternate8,Stats);
	PF_COUNTER(Alternate9,Stats);
};

using namespace phSplitPairsStats;
#endif

struct ManifoldFinder
{
	ManifoldFinder(phOverlappingPairArray::PairArray& pairs, u32 numInstances, u32* activeObjectTable)
	{
		TRAP_ONLY(m_NumInstances = numInstances;)
		m_UnassignedRandomizer = 0;
		m_ActiveObjectTable = activeObjectTable;

		u32 numPairs = pairs.GetCount();
		u16* sortedManifoldsByA = rage_new u16[numPairs];
		m_SortedManifoldsByA = sortedManifoldsByA;
		u16* sortedManifoldsByB = rage_new u16[numPairs];
		m_SortedManifoldsByB = sortedManifoldsByB;
		ManifoldFindRecord* unassignedManifolds = rage_new ManifoldFindRecord[numPairs];
		m_UnassignedManifolds = unassignedManifolds;

		u32 numUnassignedManifolds = 0;
		for (u16 pairIndex = 0; pairIndex < numPairs; ++pairIndex)
		{
			s32 nextPairIndex = IIncrementSaturateAndWrap(pairIndex, numPairs);
			phTaskCollisionPair& nextPair = pairs[nextPairIndex];
			if (phManifold* nextManifold = nextPair.manifold)
			{
				PrefetchObject(nextManifold);
			}

			s32 nextNextPairIndex = IIncrementSaturateAndWrap(nextPairIndex, numPairs);
			PrefetchObject(&pairs[nextNextPairIndex]);

			phTaskCollisionPair& pair = pairs[pairIndex];
			u16 levelIndexA = pair.levelIndex1;
			u16 levelIndexB = pair.levelIndex2;
			if (phManifold* manifold = pair.manifold)
			{
				// The manifold can be in a swapped state when it gets here.
				//FastAssert(manifold->GetGenerationIdA() == pair.generationId1);
				//FastAssert(manifold->GetGenerationIdB() == pair.generationId2);
				if ((!PHLEVEL->LegitLevelIndex(levelIndexA) || PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndexA, pair.generationId1)) &&
					(!PHLEVEL->LegitLevelIndex(levelIndexB) || PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndexB, pair.generationId2)))
				{
					// No use putting fixed/fixed collisions in the solver
					if (ObjectIsActive(levelIndexA) || ObjectIsActive(levelIndexB))
					{
						bool selfCollision = levelIndexA == levelIndexB;
						if (selfCollision || manifold->ObjectsInContact())
						{
							TrapGE(numUnassignedManifolds, numPairs);
							sortedManifoldsByA[numUnassignedManifolds] = (u16)numUnassignedManifolds;
							sortedManifoldsByB[numUnassignedManifolds] = (u16)numUnassignedManifolds;
							unassignedManifolds[numUnassignedManifolds].manifold = manifold;
							unassignedManifolds[numUnassignedManifolds].levelIndexA = levelIndexA;
							unassignedManifolds[numUnassignedManifolds].levelIndexB = levelIndexB;
							numUnassignedManifolds++;
						}
					}
				}
			}
		}

		m_NumUnassignedManifolds = numUnassignedManifolds;
		TRAP_ONLY(m_OriginalNumUnassignedManifolds = numUnassignedManifolds;)

		if (numUnassignedManifolds == 0)
		{
			return;
		}

		std::sort(sortedManifoldsByA, sortedManifoldsByA + numUnassignedManifolds, ManifoldIndexSortPredicateA(unassignedManifolds));
		std::sort(sortedManifoldsByB, sortedManifoldsByB + numUnassignedManifolds, ManifoldIndexSortPredicateB(unassignedManifolds));

		u16* firstManifoldsByA = rage_new u16[numInstances];
		m_FirstManifoldsByA = firstManifoldsByA;
		u16* firstManifoldsByB = rage_new u16[numInstances];
		m_FirstManifoldsByB = firstManifoldsByB;

		u16* numManifoldsByA = rage_new u16[numInstances];
		m_NumManifoldsByA = numManifoldsByA;
		u16* numManifoldsByB = rage_new u16[numInstances];
		m_NumManifoldsByB = numManifoldsByB;

		VERIFY_MANIFOLD_FINDER_ONLY(sysMemSet(numManifoldsByA, 0, sizeof(u16) * numInstances));
		VERIFY_MANIFOLD_FINDER_ONLY(sysMemSet(numManifoldsByB, 0, sizeof(u16) * numInstances));

		u16* whereInSortedA = rage_new u16[numUnassignedManifolds];
		m_WhereInSortedA = whereInSortedA;
		u16* whereInSortedB = rage_new u16[numUnassignedManifolds];
		m_WhereInSortedB = whereInSortedB;

		u32 levelIndexA = unassignedManifolds[sortedManifoldsByA[0]].levelIndexA;
		u32 levelIndexB = unassignedManifolds[sortedManifoldsByB[0]].levelIndexB;
		if (ObjectIsActive(levelIndexA))
		{
			TrapGE(levelIndexA, numInstances);
			firstManifoldsByA[levelIndexA] = 0;
		}
		if (ObjectIsActive(levelIndexB))
		{
			TrapGE(levelIndexB, numInstances);
			firstManifoldsByB[levelIndexB] = 0;
		}

		// Go through the manifolds in sorted order, and compute whereInSorted, numManifoldsBy, and firstManifoldsBy
		for (u32 sortedIndex = 0; sortedIndex < numUnassignedManifolds; ++sortedIndex)
		{
			// Where is the manifold in the unassigned list
			u32 unassignedIndexA = sortedManifoldsByA[sortedIndex];
			TrapGE(unassignedIndexA, numUnassignedManifolds);

			// What is the level index A for the Nth manifold sorted by A?
			u32 newLevelIndexA = unassignedManifolds[unassignedIndexA].levelIndexA;

			// Record where this manifold is in the sorted list so we can find it in there later for removal
			whereInSortedA[unassignedIndexA] = (u16)sortedIndex;

			if (levelIndexA != newLevelIndexA)
			{
				// We've found the full set of manifolds with this ievel index A

				// This is how many manifolds with this ievel index A there are, record that
				if (ObjectIsActive(levelIndexA))
				{
					TrapGE(levelIndexA, numInstances);
					numManifoldsByA[levelIndexA] = u16(sortedIndex - firstManifoldsByA[levelIndexA]);
				}

				// Record where the block of manifolds begins, in the sorted list, which have this level index A
				if (ObjectIsActive(newLevelIndexA))
				{
					TrapGE(newLevelIndexA, numInstances);
					firstManifoldsByA[newLevelIndexA] = (u16)sortedIndex;
				}
				levelIndexA = newLevelIndexA;
			}

			// Where is the manifold in the unassigned list
			u32 unassignedIndexB = sortedManifoldsByB[sortedIndex];
			TrapGE(unassignedIndexB, numUnassignedManifolds);

			// What is the level index B for the Nth manifold sorted by B?
			u32 newLevelIndexB = unassignedManifolds[unassignedIndexB].levelIndexB;

			// Record where this manifold is in the sorted list so we can find it in there later for removal
			whereInSortedB[unassignedIndexB] = (u16)sortedIndex;
			if (levelIndexB != newLevelIndexB)
			{
				// We've found the full set of manifolds with this ievel index B

				// This is how many manifolds with this ievel index B there are, record that
				if (ObjectIsActive(levelIndexB))
				{
					TrapGE(levelIndexB, numInstances);
					numManifoldsByB[levelIndexB] = u16(sortedIndex - firstManifoldsByB[levelIndexB]);
				}

				// Record where the block of manifolds begins, in the sorted list, which have this level index B
				if (ObjectIsActive(newLevelIndexB))
				{
					TrapGE(newLevelIndexB, numInstances);
					firstManifoldsByB[newLevelIndexB] = (u16)sortedIndex;
				}
				levelIndexB = newLevelIndexB;
			}
		}

		// Close out the last blocks by recording how many manifolds are in the lists
		if (ObjectIsActive(levelIndexA))
		{
			TrapGE(levelIndexA, numInstances);
			numManifoldsByA[levelIndexA] = u16(numUnassignedManifolds - firstManifoldsByA[levelIndexA]);
		}
		if (ObjectIsActive(levelIndexB))
		{
			TrapGE(levelIndexB, numInstances);
			numManifoldsByB[levelIndexB] = u16(numUnassignedManifolds - firstManifoldsByB[levelIndexB]);
		}

		m_PreferB = false;

		VERIFY_MANIFOLD_FINDER_ONLY(Verify());
	}

	u32 Find(u32 levelIndexA, u32 levelIndexB, u32 alternateManifold)
	{
		VERIFY_MANIFOLD_FINDER_ONLY(Verify());

#if TRACK_SEARCH_ALTERNATES
		switch (alternateManifold)
		{
		case 0:
			PF_INCREMENT(Alternate0);
			break;

		case 1:
			PF_INCREMENT(Alternate1);
			break;

		case 2:
			PF_INCREMENT(Alternate2);
			break;

		case 3:
			PF_INCREMENT(Alternate3);
			break;

		case 4:
			PF_INCREMENT(Alternate4);
			break;

		case 5:
			PF_INCREMENT(Alternate5);
			break;

		case 6:
			PF_INCREMENT(Alternate6);
			break;

		case 7:
			PF_INCREMENT(Alternate7);
			break;

		case 8:
			PF_INCREMENT(Alternate8);
			break;

		default:
			PF_INCREMENT(Alternate9);
			break;
		}
#endif

		if (m_PreferB)
		{
			if (ObjectIsActive(levelIndexB))
			{
				TrapGE(levelIndexB, m_NumInstances);
				u32 numManifoldsB = m_NumManifoldsByB[levelIndexB];
				if (alternateManifold < m_NumManifoldsByB[levelIndexB])
				{
					u32 sortedIndexB = m_FirstManifoldsByB[levelIndexB] + alternateManifold;
					TrapGE(sortedIndexB, m_OriginalNumUnassignedManifolds);
					u32 unassignedIndexB = m_SortedManifoldsByB[sortedIndexB];
					TrapGE(unassignedIndexB, m_NumUnassignedManifolds);
					return unassignedIndexB;
				}
				else
				{
					// Cut down the alternate index, so we can start searching the full unassigned list
					alternateManifold -= numManifoldsB;
				}
			}
		}

		// Is there a manifold with the same level index A?
		if (ObjectIsActive(levelIndexA))
		{
			TrapGE(levelIndexA, m_NumInstances);
			u32 numManifoldsA = m_NumManifoldsByA[levelIndexA];
			if (alternateManifold < numManifoldsA)
			{
				m_PreferB = false;

				u32 sortedIndexA = m_FirstManifoldsByA[levelIndexA] + alternateManifold;
				TrapGE(sortedIndexA, m_OriginalNumUnassignedManifolds);
				u32 unassignedIndexA = m_SortedManifoldsByA[sortedIndexA];
				TrapGE(unassignedIndexA, m_NumUnassignedManifolds);
				return unassignedIndexA;
			}
			else
			{
				// Cut down the alternate number, so we can start searching by B slot
				alternateManifold -= numManifoldsA;
			}
		}

		// How about B?
		if (!m_PreferB)
		{
			if (ObjectIsActive(levelIndexB))
			{
				TrapGE(levelIndexB, m_NumInstances);
				u32 numManifoldsB = m_NumManifoldsByB[levelIndexB];
				if (alternateManifold < m_NumManifoldsByB[levelIndexB])
				{
					m_PreferB = true;

					u32 sortedIndexB = m_FirstManifoldsByB[levelIndexB] + alternateManifold;
					TrapGE(sortedIndexB, m_OriginalNumUnassignedManifolds);
					u32 unassignedIndexB = m_SortedManifoldsByB[sortedIndexB];
					TrapGE(unassignedIndexB, m_NumUnassignedManifolds);
					return unassignedIndexB;
				}
				else
				{
					// Cut down the alternate index, so we can start searching the full unassigned list
					alternateManifold -= numManifoldsB;
				}
			}
		}

		// If not, then grab a random manifold if we've got one
		if (alternateManifold < m_NumUnassignedManifolds)
		{
			m_UnassignedRandomizer = (m_UnassignedRandomizer + alternateManifold) % m_NumUnassignedManifolds;
			return m_UnassignedRandomizer;
		}
		
		return phInst::INVALID_INDEX;
	}

	phManifold* GetManifold(u32 foundIndex)
	{
		TrapGE(foundIndex, m_NumUnassignedManifolds);
		return m_UnassignedManifolds[foundIndex].manifold;
	}

	u16 GetLevelIndexA(u32 foundIndex)
	{
		TrapGE(foundIndex, m_NumUnassignedManifolds);
		return m_UnassignedManifolds[foundIndex].levelIndexA;
	}

	u16 GetLevelIndexB(u32 foundIndex)
	{
		TrapGE(foundIndex, m_NumUnassignedManifolds);
		return m_UnassignedManifolds[foundIndex].levelIndexB;
	}

	void RemoveManifold(u32 foundIndex)
	{
		VERIFY_MANIFOLD_FINDER_ONLY(Verify());

		TrapGE(foundIndex, m_NumUnassignedManifolds);
		ManifoldFindRecord& removedManifold = m_UnassignedManifolds[foundIndex];

		// ***
		// *** Handle removing the manifold from the sorted list by A
		// ***

		u32 levelIndexA = removedManifold.levelIndexA;

		if (ObjectIsActive(levelIndexA))
		{
			TrapGE(levelIndexA, m_NumInstances);

			// Find out the last manifold in the lists sorted by A so we can swap it into place
			u32 lastByA = m_FirstManifoldsByA[levelIndexA] + m_NumManifoldsByA[levelIndexA] - 1;
			TrapGE(lastByA, m_OriginalNumUnassignedManifolds);

			// There's now one less manifold with this level index in slot A
			--(m_NumManifoldsByA[levelIndexA]);

			// We'll be swapping this manifold into place in the sorted arrays here
			u32 unassignedMovedManifoldIndexA = m_SortedManifoldsByA[lastByA];
			TrapGE(unassignedMovedManifoldIndexA, m_NumUnassignedManifolds);

			// This is where the removed manifold was...
			u32 sortedMovedToA = m_WhereInSortedA[foundIndex];
			TrapGE(sortedMovedToA, m_OriginalNumUnassignedManifolds);

			// ...so swap the last manifold on top of it...
			m_SortedManifoldsByA[sortedMovedToA] = (u16)unassignedMovedManifoldIndexA;
			ASSERT_ONLY(m_SortedManifoldsByA[lastByA] = 0xffff;)

			// ...making sure to update our index so it's still correct
			m_WhereInSortedA[unassignedMovedManifoldIndexA] = (u16)sortedMovedToA;
			Assert(m_SortedManifoldsByA[sortedMovedToA] == unassignedMovedManifoldIndexA || m_SortedManifoldsByA[sortedMovedToA] == 0xffff);
		}

		// ***
		// *** Handle removing the manifold from the sorted list by B
		// ***

		u32 levelIndexB = removedManifold.levelIndexB;

		if (ObjectIsActive(levelIndexB))
		{
			TrapGE(levelIndexB, m_NumInstances);

			// Find out the last manifold in the lists sorted by B so we can swap it into place
			u32 lastByB = m_FirstManifoldsByB[levelIndexB] + m_NumManifoldsByB[levelIndexB] - 1;
			TrapGE(lastByB, m_OriginalNumUnassignedManifolds);

			// There's now one less manifold with this level index in slot B
			--(m_NumManifoldsByB[levelIndexB]);

			// We'll be swapping this manifold into place in the sorted arrays here
			u32 unassignedMovedManifoldIndexB = m_SortedManifoldsByB[lastByB];
			TrapGE(unassignedMovedManifoldIndexB, m_NumUnassignedManifolds);

			// This is where the removed manifold was...
			u32 sortedMovedToB = m_WhereInSortedB[foundIndex];
			TrapGE(sortedMovedToB, m_OriginalNumUnassignedManifolds);

			// ...so swap the last manifold on top of it...
			m_SortedManifoldsByB[sortedMovedToB] = (u16)unassignedMovedManifoldIndexB;
			ASSERT_ONLY(m_SortedManifoldsByB[lastByB] = 0xffff;)

			// ...making sure to update our index so it's still correct
			m_WhereInSortedB[unassignedMovedManifoldIndexB] = (u16)sortedMovedToB;
			Assert(m_SortedManifoldsByB[sortedMovedToB] == unassignedMovedManifoldIndexB || m_SortedManifoldsByB[sortedMovedToB] == 0xffff);
		}

		// ***
		// *** Handle removing the manifold from the overall unsorted list
		// ***

		// Move the last manifold in the list on top of our removed one
		u32 lastManifold = --m_NumUnassignedManifolds;
		if (foundIndex != lastManifold)
		{
			m_UnassignedManifolds[foundIndex] = m_UnassignedManifolds[lastManifold];
			ASSERT_ONLY(m_UnassignedManifolds[lastManifold].manifold = NULL;)

			// Update the where in sorted indices
			u32 whereInSortedA = m_WhereInSortedA[lastManifold];
			m_WhereInSortedA[foundIndex] = (u16)whereInSortedA;
			ASSERT_ONLY(m_WhereInSortedA[lastManifold] = 0xffff);
			TrapGE(whereInSortedA, m_OriginalNumUnassignedManifolds);
			u32 whereInSortedB = m_WhereInSortedB[lastManifold];
			m_WhereInSortedB[foundIndex] = (u16)whereInSortedB;
			ASSERT_ONLY(m_WhereInSortedB[lastManifold] = 0xffff);
			TrapGE(whereInSortedB, m_OriginalNumUnassignedManifolds);

			// Update the sorted list indices
			m_SortedManifoldsByA[whereInSortedA] = (u16)foundIndex;
			m_SortedManifoldsByB[whereInSortedB] = (u16)foundIndex;
		}
		
		VERIFY_MANIFOLD_FINDER_ONLY(Verify());
	}

	u32 NumberLeft()
	{
		return m_NumUnassignedManifolds;
	}

	static size_t ComputeNeededSize(u32 numPairs, u32 numInstances)
	{
		size_t neededSpace = (numPairs * sizeof(u16) + 16) * 2; // sortedManifolds
		neededSpace += (numPairs * sizeof(ManifoldFindRecord) + 16) * 3; // unassignedManifolds, numManifolds
		neededSpace += (numInstances * sizeof(u16) + 16) * 4; // firstManifolds, whereInSorted

		return neededSpace;
	}

private:

	bool ObjectIsActive(u32 levelIndex)
	{
		if (levelIndex == phInst::INVALID_INDEX)
		{
			return false;
		}

		TrapGE(levelIndex, m_NumInstances);
		u32 idxA = levelIndex >> 5;
		u32 maskA = SHIFT_TABLE_32[levelIndex & 31];
		return (m_ActiveObjectTable[idxA] & maskA) != 0;
	}

#if VERIFY_MANIFOLD_FINDER
	void Verify()
	{
		for (u32 i = 0; i < m_NumUnassignedManifolds; ++i)
		{
			Assert(m_UnassignedManifolds[i].manifold);
			u32 levelIndexA = m_UnassignedManifolds[i].levelIndexA;
			u32 levelIndexB = m_UnassignedManifolds[i].levelIndexB;
			Assert(m_NumManifoldsByA[levelIndexA] > 0 || !ObjectIsActive(levelIndexA));
			Assert(m_NumManifoldsByB[levelIndexB] > 0 || !ObjectIsActive(levelIndexB));
			Assert(m_SortedManifoldsByA[m_WhereInSortedA[i]] == i);
			Assert(m_SortedManifoldsByB[m_WhereInSortedB[i]] == i);
		}

		for (u32 i = 0; i < m_NumInstances; ++i)
		{
			for (int a = 0; a < m_NumManifoldsByA[i]; ++a)
			{
				Assert(m_UnassignedManifolds[m_SortedManifoldsByA[m_FirstManifoldsByA[i] + a]].levelIndexA == i);
			}

			for (int b = 0; b < m_NumManifoldsByB[i]; ++b)
			{
				Assert(m_UnassignedManifolds[m_SortedManifoldsByB[m_FirstManifoldsByB[i] + b]].levelIndexB == i);
			}
		}
	}
#endif

	// The manifolds that still need to be assigned, in random order
	ManifoldFindRecord* m_UnassignedManifolds;

	// The number of manifolds still in m_UnassignedManifolds
	u32 m_NumUnassignedManifolds;

	// Rotating manifold index, 
	u32 m_UnassignedRandomizer;

	// The original number of unassigned manifolds, only used for array bounds checking on m_SortedManifoldsByX
	TRAP_ONLY(u32 m_OriginalNumUnassignedManifolds;)

	// The number of instances, only used for array bounds checking on m_NumManifoldsByX and m_FirstManifoldsByX
	TRAP_ONLY(u32 m_NumInstances;)

	// The table of bits of active objects by level index, computed in SplitPairs and passed in
	u32* m_ActiveObjectTable;

	// Index lists (indexing into m_UnassignedManifolds) that are sorted by level index A/B
	u16* m_SortedManifoldsByA;
	u16* m_SortedManifoldsByB;

	// Index into sorted lists by level index, showing where the first manifold is by level index
	u16* m_FirstManifoldsByA;
	u16* m_FirstManifoldsByB;

	// Number of manifolds in the sorted lists by level index. The manifolds from m_SortedManifoldsByX[m_FirstManifoldsByX[levelIndex]]
	// to m_SortedManifoldsByX[m_FirstManifoldsByX[levelIndex] + m_NumManifoldsByX[levelIndex] - 1] will all have levelIndex in slot X
	u16* m_NumManifoldsByA;
	u16* m_NumManifoldsByB;

	// Map from unassigned manifold index to where the manifold is in the sorted lists. Necessary so that we can fix up indices in the sorted
	// lists when we move manifolds around in m_UnassignedManifolds.
	u16* m_WhereInSortedA;
	u16* m_WhereInSortedB;

	// This bool controls whether we'll try to find a match by level index A or B first. That way we'll get "runs" of the same level index
	// in each slot, hopefully getting all the manifolds with a particular inst in one phase.
	bool m_PreferB;
};

static size_t ComputeSplitPairSize(u32 numInstances, u32 phaseWindowSize, u32 numPairs)
{
	const u32 activeObjectTableSize = (numInstances + 31) / 32;
	const u32 stateTableSize = (numInstances + 31) / 32;

	size_t neededSpace = (activeObjectTableSize * sizeof(u32) + 16) * 2; // activeObjectTable, articulatedObjectTable
	neededSpace += stateTableSize * sizeof(u32) + 32; // stateTable
	neededSpace += phaseWindowSize * sizeof(u16*) + 16; // phaseObjectArray
	neededSpace += phaseWindowSize * phForceSolver::MAX_BATCHES_NUM * 2 * sizeof(u16) + 16; // phaseObjectArray[phaseIndex]
	neededSpace += phaseWindowSize * sizeof(u32) + 16; // phaseObjectCount
	neededSpace += ManifoldFinder::ComputeNeededSize(numPairs, numInstances); // unassignedPairs

	return neededSpace;
}

void AllocateObjects(u32 phase, u16** phaseObjectArray, u32* phaseObjectCount, u32* stateTable, u32 TRAP_ONLY(stateTableSize))
{
	u16* fullPhaseObjectArray = phaseObjectArray[phase];
	u32 fullPhaseObjectCount = phaseObjectCount[phase];
	for (u32 objectIndex = 0; objectIndex < fullPhaseObjectCount; ++objectIndex)
	{
		u32 levelIndex = fullPhaseObjectArray[objectIndex];
		u32 idx = levelIndex >> 5;
		u32 mask = SHIFT_TABLE_32[levelIndex & 31];
		TrapGE(idx, stateTableSize);
		stateTable[idx] |= mask;
	}
}

void ReleaseObjects(u32 dependencyPhase, u16** phaseObjectArray, u32* phaseObjectCount, u32* stateTable, u32 TRAP_ONLY(stateTableSize))
{
	// There are no more pairs that will fit in this phase, so wait for an older phase to release before continuing
	u16* dependencyPhaseObjectArray = phaseObjectArray[dependencyPhase];
	u32 dependencyPhaseObjectCount = phaseObjectCount[dependencyPhase];

	// Go through the object array for the dependency phase, and release any objects that are currently reserved
	for (u32 objectIndex = 0; objectIndex < dependencyPhaseObjectCount; ++objectIndex)
	{
		u32 levelIndex = dependencyPhaseObjectArray[objectIndex];
		u32 idx = levelIndex >> 5;
		u32 mask = SHIFT_TABLE_32[levelIndex & 31];
		TrapGE(idx, stateTableSize);
		stateTable[idx] &= ~mask;
	}

	// We have released all the objects from this phase, so clear the object count array in case we wrap around
	phaseObjectCount[dependencyPhase] = 0;
}

void phContactMgr::SplitPairs(phOverlappingPairArray* pairArray)
{
	PF_FUNC(SplitPairs);

	// If there are no pairs, we're done
	if (pairArray->pairs.GetCount() == 0)
	{
		m_NumPara = 0;
		return;
	}

	// Phase window maximum size is now fixed
	const u32 phaseWindowSize = 32;

	// Compute the sizes of some tables early so we can do a memory check
	const u32 numInstances = PHLEVEL->GetCurrentMaxLevelIndex() + 1;
	const u32 activeObjectTableSize = (numInstances + 31) / 32;
	const u32 stateTableSize = (numInstances + 31) / 32;
	const u32 numPairs = pairArray->pairs.GetCount();

	size_t neededSpace = ComputeSplitPairSize(numInstances, phaseWindowSize, numPairs);

	// If we don't have enough space, reduce the max phases until we do, hopefully we don't have more objects than this anyway
	u32 maxPhases = phConfig::GetForceSolverMaxPhases();
	while (neededSpace + maxPhases * sizeof(phForceSolver::ParallelGroup) > GetHalfScratchpadSize())
	{
		u32 oldMaxPhases = maxPhases;
		maxPhases = u32(oldMaxPhases * 0.9f);
		phConfig::SetForceSolverMaxPhases(maxPhases);

		Warningf("Physics force solver max phases too large, reducing from %d to %d due to scratch pad size limitations.", oldMaxPhases, maxPhases);
	}

	// This is what we are computing, the groups that we can process in parallel
	phForceSolver::ParallelGroup* paraGroup = m_ParaGroup;

	// Init the first phase...we do this to each phase as we move along
	paraGroup[0].numManifolds = 0;
	paraGroup[0].phasesToRelease = 0;

	// This tracks the current number of phases we've currently filled out
	u32 numPara = 0;

	// Allocate all the tables out of the scratchpad, rather than main memory
	StartHalfScratchpad();

	// Look up the whether each instance is active in advance, to improve cache performance later
	u32* activeObjectTable = rage_new u32[activeObjectTableSize];
	u32* articulatedObjectTable = rage_new u32[activeObjectTableSize];
	sysMemSet(activeObjectTable, 0x00, activeObjectTableSize * sizeof(u32));
	sysMemSet(articulatedObjectTable, 0x00, activeObjectTableSize * sizeof(u32));

	bool dontCombineArticulated = sm_DontCombineArticulated;
	const phLevelNew *physicsLevel = PHLEVEL;
	for(int activeIndex = physicsLevel->GetNumActive() - 1; activeIndex >= 0; --activeIndex)
	{
		const int levelIndex = physicsLevel->GetActiveLevelIndex(activeIndex);
		const u32 wordIndex = (levelIndex >> 5);
		const u32 bitOffset = (levelIndex & 31);

		// I tried it out both ways with a large number of active objects and using a shift table seemed to be a very *slight* improvement.
		// I expected that it would be a bigger win given that variable shifts are supposed to be so costly.  I haven't checked to see if I'm incurring
		//   a cache miss on the first access of SHIFT_TABLE_32 - maybe a carefully-placed pre-fetch would help there?  I took a guess at a good location
		//   for one but it didn't seem to make any difference at all.
#if 1
		const u32 mask = SHIFT_TABLE_32[bitOffset];
#else
		const u32 mask = (1 << bitOffset);
#endif
		TrapGE(wordIndex, activeObjectTableSize);
		activeObjectTable[wordIndex] |= mask;

		if (dontCombineArticulated || sm_MaxArticulatedPerPhase < phForceSolver::MAX_BATCHES_NUM)
		{
			phCollider* collider = PHSIM->GetCollider(levelIndex);
			Assert(collider);
			if (collider->IsArticulated())
			{
				articulatedObjectTable[wordIndex] |= mask;
			}
		}
	}

	// The state table shows which objects are "in use" at the current "time"
	// NOTE: should start all zeroes, and end all zeroes
	u32* stateTable;
	stateTable = rage_new u32[stateTableSize];
	sysMemSet(stateTable, 0, sizeof(u32) * (stateTableSize));

	phOverlappingPairArray::PairArray& pairs = pairArray->pairs;

	// The phase object array tells which objects are reserved for each phase in the active window
	u16** phaseObjectArray = rage_new u16*[phaseWindowSize];
	for (u32 phaseIndex = 0; phaseIndex < phaseWindowSize; ++phaseIndex)
	{
		phaseObjectArray[phaseIndex] = rage_new u16[phForceSolver::MAX_BATCHES_NUM * 2];
	}
	u32* phaseObjectCount = rage_new u32[phaseWindowSize];
	sysMemSet(phaseObjectCount, 0, sizeof(u32) * phaseWindowSize);

	ManifoldFinder finder(pairs, numInstances, activeObjectTable);

	// Done with allocations
	EndHalfScratchpad();

	// The target count is our guess as to how many constraints should be handled in each phase to maximize parallelism
	u32 targetCount = sm_MaxPairsPerPhase;

	// The current window phase shows which phase within the window we are looking at. Increases modulo the window size.
	u32 curWindowPhase = 0;

	// This indicates the oldest thread that we might have dependencies on (it might still be running on another worker thread)
	u32 dependencyPhase = 0;

	u16 initialPhasesToRelease = 0;
	u16 totalReleases = 0;
	u16* currentPhasesToRelease = &initialPhasesToRelease;

	u32 lastLevelIndexA = phInst::INVALID_INDEX;
	u32 lastLevelIndexB = phInst::INVALID_INDEX;

	u32 alternateManifold = 0; 

	int numArticulated = 0;
	bool lastPhaseHadArticulated = false;

	while (finder.NumberLeft() > 0 && numPara < maxPhases) 
	{
		u32 foundIndex = finder.Find(lastLevelIndexA, lastLevelIndexB, alternateManifold);
		
		// If the last phase had articulated bodies, consider ending the phase now
		bool phaseFull = false;
		if (lastPhaseHadArticulated)
		{
			if (dontCombineArticulated)
			{
				phaseFull = true;
			}
			else
			{
				numArticulated++;
				if (numArticulated >= sm_MaxArticulatedPerPhase)
				{
					phaseFull = true;
				}
			}
		}

		// End the phase if it's full
		if (paraGroup[numPara].numManifolds == targetCount)
		{
			phaseFull = true;
		}

		bool addDependency = foundIndex == phInst::INVALID_INDEX;

		// If we have to add a dependency and we already have stuff in our current phase, finish this phase
		// off so that it doesn't need to depend on the phase we're releasing
		if (phaseFull || (addDependency && m_ParaGroup[numPara].numManifolds > 0))
		{
			// We can't put anything else in the current phase, finish it off
			numPara++;
			paraGroup[numPara].numManifolds = 0;
			paraGroup[numPara].phasesToRelease = 0;
			numArticulated = 0;

			// Increase the phases to release of the current dependency phase
			totalReleases++;
			(*currentPhasesToRelease)++;
			FastAssert(initialPhasesToRelease <= numPara + 1);
			lastPhaseHadArticulated = false;

			// We only have so much space in our phase object arrays, so we have to release a dependency phase
			if (numPara - dependencyPhase + 1 >= phaseWindowSize)
			{
				addDependency = true;
			}

			// Update state table with changes...couldn't do this earlier because it would prevent objects from being used twice in the same phase
			AllocateObjects(curWindowPhase, phaseObjectArray, phaseObjectCount, stateTable, stateTableSize);

			// Proceed to the next phase in the window, pushing the previous phase in this slot out of the window.
			curWindowPhase = (curWindowPhase + 1) % phaseWindowSize;

			// We've got a problem if the object count of the new phase we're on is non-zero
			Assert(phaseObjectCount[curWindowPhase] == 0);
		}

		if (addDependency)
		{
			// We've decided we can't go on without releasing the objects in the dependency phase, so do that
			ReleaseObjects(dependencyPhase % phaseWindowSize, phaseObjectArray, phaseObjectCount, stateTable, stateTableSize);

			// Go back to manifold zero since we just released dependencies it might be allocatable now
			alternateManifold = 0;

			// Releasing a dependency means the dependency phase has moved
			currentPhasesToRelease = &m_ParaGroup[dependencyPhase].phasesToRelease;
			dependencyPhase++;

			// Look for another manifold; hopefully there will be one since we released a phase
			continue;
		}

		phManifold* manifold = finder.GetManifold(foundIndex);

		// In case we can't allocate this manifold, this makes sure we get a different one on the next time through
		alternateManifold++;
		
		u32 levelIndexA = finder.GetLevelIndexA(foundIndex);
		bool legitA = PHLEVEL->LegitLevelIndex(levelIndexA);
		u32 idxA = levelIndexA >> 5;
		u32 maskA = SHIFT_TABLE_32[levelIndexA & 31];

		// Check to see if either of these objects are already in use by a phase in the window
		if (legitA)
		{
			TrapGE(idxA, stateTableSize);
			if(stateTable[idxA] & maskA)
			{
				// If so, this manifold will have to wait until later.
				continue;
			}
		}

		u32 levelIndexB = finder.GetLevelIndexB(foundIndex);
		bool legitB = PHLEVEL->LegitLevelIndex(levelIndexB);
		u32 idxB = levelIndexB >> 5;
		u32 maskB = SHIFT_TABLE_32[levelIndexB & 31];

		if (legitB)
		{
			TrapGE(idxB, stateTableSize);
			if(stateTable[idxB] & maskB)
			{
				// If so, this manifold will have to wait until later.
				continue;
			}
		}

		bool pairContainsArticulated = (legitA && (articulatedObjectTable[idxA] & maskA)) || (legitB && (articulatedObjectTable[idxB] & maskB));

		phForceSolver::ParallelGroup& curParaGroup = paraGroup[numPara];
		if (dontCombineArticulated && pairContainsArticulated && curParaGroup.numManifolds > 0)
		{
			// This manifold contains something articulated, but the current phase already has rigid body manifolds in it, so save this manifold until later
			continue;
		}

		// We're confident at this point that the manifold will be assigned to this phase
		alternateManifold = 0;
		lastPhaseHadArticulated = pairContainsArticulated;
		finder.RemoveManifold(foundIndex);

		// Mark these instances as being used, if they are active
		u16* activePhaseObjectArray = phaseObjectArray[curWindowPhase];
		u32 activePhaseObjectCount = phaseObjectCount[curWindowPhase];

		if (legitA)
		{
			TrapGE(idxA, activeObjectTableSize);
			if (activeObjectTable[idxA] & maskA)
			{
				TrapGE(activePhaseObjectCount, phForceSolver::MAX_BATCHES_NUM * 2);
				activePhaseObjectArray[activePhaseObjectCount++] = (u16)levelIndexA;
				lastLevelIndexA = levelIndexA;
			}
			else
			{
				lastLevelIndexA = phInst::INVALID_INDEX;
			}
		}
		else
		{
			lastLevelIndexA = phInst::INVALID_INDEX;
		}
		if (legitB)
		{
			TrapGE(idxB, activeObjectTableSize);
			if (activeObjectTable[idxB] & maskB)
			{
				TrapGE(activePhaseObjectCount, phForceSolver::MAX_BATCHES_NUM * 2);
				activePhaseObjectArray[activePhaseObjectCount++] = (u16)levelIndexB;
				lastLevelIndexB = levelIndexB;
			}
			else
			{
				lastLevelIndexB = phInst::INVALID_INDEX;
			}
		}
		else
		{
			lastLevelIndexB = phInst::INVALID_INDEX;
		}

		// Put the manifold into the parallel group for this phase
		curParaGroup.manifolds[curParaGroup.numManifolds++] = manifold;
		phaseObjectCount[curWindowPhase] = activePhaseObjectCount;
	}

	Assertf(finder.NumberLeft() == 0, "Max phases (%d) is exhausted with %d manifolds not assigned to a phase, some objects will not obey constraints.", maxPhases, finder.NumberLeft());

	// Release the last phase
	(*currentPhasesToRelease)++;
	FastAssert(initialPhasesToRelease <= numPara + 1);
	totalReleases++;

	AllocateObjects(curWindowPhase, phaseObjectArray, phaseObjectCount, stateTable, stateTableSize);

	for (u32 wrapGroupIndex = 0; dependencyPhase < numPara &&
								 wrapGroupIndex < numPara &&
								 totalReleases < numPara + 1; ++wrapGroupIndex)
	{
		bool addDependency = false;
		phForceSolver::ParallelGroup& wrapGroup = paraGroup[wrapGroupIndex];
		if (numPara - dependencyPhase + 1 >= phaseWindowSize)
		{
			addDependency = true;
		}
		else
		{
			for (int pairIndex = 0; pairIndex < wrapGroup.numManifolds; ++pairIndex)
			{
				phManifold& manifold = *wrapGroup.manifolds[pairIndex];
				u32 levelIndexA = manifold.GetLevelIndexA();
				u32 idxA = levelIndexA >> 5;
				u32 maskA = SHIFT_TABLE_32[levelIndexA & 31];
				bool legitA = PHLEVEL->LegitLevelIndex(levelIndexA);

				// Check to see if either of these objects are already in use by someone other than the active phase
				if (legitA)
				{
					TrapGE(idxA, stateTableSize);
					if(stateTable[idxA] & maskA)
					{
						addDependency = true;
						break;
					}
				}

				u32 levelIndexB = manifold.GetLevelIndexB();
				u32 idxB = levelIndexB >> 5;
				u32 maskB = SHIFT_TABLE_32[levelIndexB & 31];
				bool legitB = PHLEVEL->LegitLevelIndex(levelIndexB);

				if (legitB)
				{
					TrapGE(idxB, stateTableSize);
					if(stateTable[idxB] & maskB)
					{
						addDependency = true;
						break;
					}
				}
			}
		}

		if (addDependency)
		{
			// There are no more pairs that will fit in this phase, so wait for an older phase to release before continuing
			ReleaseObjects(dependencyPhase % phaseWindowSize, phaseObjectArray, phaseObjectCount, stateTable, stateTableSize);

			// Releasing a dependency means the dependency phase has moved
			currentPhasesToRelease = &m_ParaGroup[dependencyPhase].phasesToRelease;
			dependencyPhase++;
		}

		(*currentPhasesToRelease)++;
		FastAssert(initialPhasesToRelease <= numPara + 1);

		totalReleases++;
	}

	// Don't delete, this memory is just taken from the scratchpad
	//delete [] stateTable;

	// Somehow we're getting initialPhasesToRelease greater than numPara, which is bad, but I can't figure out how it could happen
	FastAssert(initialPhasesToRelease <= numPara + 1);
	initialPhasesToRelease = u16(initialPhasesToRelease <= numPara + 1 ? initialPhasesToRelease : numPara + 1);

	m_NumPara = numPara + 1;
	m_InitialPhasesToRelease = initialPhasesToRelease;

	Assert(numPara + 1 >= u32(totalReleases - initialPhasesToRelease));
	m_ParaGroup[numPara].phasesToRelease = u16(numPara + 1 + initialPhasesToRelease - totalReleases);

	//PrintPairs();
}

void phContactMgr::PrintPairs()
{
#if __BANK
	Displayf("=-=-=-= Dump force solver pairs =-=-=-=");
	Displayf("    Initial phases to release: %d", m_InitialPhasesToRelease);
	Displayf("  (* marks articulated objects)");
	for(u32 l=0;l<m_NumPara;l++)
	{
		phForceSolver::ParallelGroup& paraGroup = m_ParaGroup[l];
#if PER_MANIFOLD_SOLVER_METRICS
		Displayf("*** ParallelGroup Phase %d, phasesToRelease: %d, wait: %d, solve: %d", l, paraGroup.phasesToRelease, paraGroup.waitTime, paraGroup.solveTime);
#else
		Displayf("*** ParallelGroup Phase %d, phasesToRelease: %d",l,paraGroup.phasesToRelease);
#endif
		for(int j=0;j<paraGroup.numManifolds;j++)
		{
			phManifold& manifold = *paraGroup.manifolds[j];
			const char* articulated1 = "";
			if (manifold.GetLevelIndexA() != phInst::INVALID_INDEX)
			{
				if (phCollider* collider = PHSIM->GetCollider(manifold.GetLevelIndexA()))
				{
					if (collider->IsArticulated())
					{
						articulated1 = "*";
					}
				}
			}
			const char* articulated2 = "";
			if (manifold.GetLevelIndexB() != phInst::INVALID_INDEX)
			{
				if (phCollider* collider = PHSIM->GetCollider(manifold.GetLevelIndexB()))
				{
					if (collider->IsArticulated())
					{
						articulated2 = "*";
					}
				}
			}
#if PER_MANIFOLD_SOLVER_METRICS
			Displayf(">>> %4d :: %s%d vs %s%d :: uc: %d v: %d p: %d",
				j,
				articulated1,
				manifold.GetLevelIndexA(),
				articulated2,
				manifold.GetLevelIndexB(),
				manifold.GetUpdateContactsTime(),
				manifold.GetVelocitySolveTime(),
				manifold.GetPushSolveTime());
#else
			Displayf(">>> %4d :: %s%d vs %s%d" ,j,articulated1,manifold.GetLevelIndexA(),articulated2,manifold.GetLevelIndexB());
#endif
		}
	}
#endif // !__FINAL
}

static u32 s_UpdateContactsAtom;

void phContactMgr::UpdateContacts(phOverlappingPairArray* pairArray, float timeStep, bool bJustRunConstraintFunctions, bool
#if ENSURE_ART_VELOCITY_PROP_IN_UPDATE
	computeBounceAndTangent
#endif // ENSURE_ART_VELOCITY_PROP_IN_UPDATE
	)
{
	s_UpdateContactsAtom = 0;

	static sysTaskParameters s_UpdateContactsTaskParams ;

	s_UpdateContactsTaskParams.UserData[0].asBool = bJustRunConstraintFunctions;
	s_UpdateContactsTaskParams.UserData[1].asFloat = phSimulator::GetAllowedPenetration();
#if ENSURE_ART_VELOCITY_PROP_IN_UPDATE
	s_UpdateContactsTaskParams.UserData[2].asBool = computeBounceAndTangent;
#else // ENSURE_ART_VELOCITY_PROP_IN_UPDATE
	s_UpdateContactsTaskParams.UserData[2].asBool = !bJustRunConstraintFunctions;
#endif // ENSURE_ART_VELOCITY_PROP_IN_UPDATE

	s_UpdateContactsTaskParams.UserData[3].asPtr = &s_UpdateContactsAtom;
	s_UpdateContactsTaskParams.UserData[4].asInt = pairArray->pairs.GetCount();
	s_UpdateContactsTaskParams.UserData[5].asPtr = pairArray->pairs.GetElements();
	s_UpdateContactsTaskParams.UserData[6].asInt = phSimulator::GetMinManifoldPointLifetime();
	
	bool incrementContactLifetimes = EARLY_FORCE_SOLVE_ONLY(!sm_UsePushes) NOT_EARLY_FORCE_SOLVE_ONLY(true);
	s_UpdateContactsTaskParams.UserData[8].asBool = incrementContactLifetimes;

	s_UpdateContactsTaskParams.UserData[9].asFloat = sm_MinBounce;

	// from phContactMgr::FillInForceSolverMembers
	// phForceSolver::m_SperateBias = sm_SeparationBias
	// phForceSolver::m_TimeStep = timeStep;
	// from phForceSolver::InitGlobals.
	// phForceSolver::sm_Globals.separateBias = ScalarVFromF32(m_SeparateBias) * ScalarVFromF32(m_TimeStep * MAGIC_SEPARATION_BIAS_SYNC_NUMBER);
	const float separateBiasMultiplier = sm_SeparationBias * timeStep * MAGIC_SEPARATION_BIAS_SYNC_NUMBER;
	//const float separateBiasMultiplier = globals.separateBias;
	//const float separateBiasMultiplier = 1.0f;
	s_UpdateContactsTaskParams.UserData[10].asFloat = separateBiasMultiplier;

	s_UpdateContactsTaskParams.UserData[18].asFloat = timeStep;

#if POSITIVE_DEPTH_ONLY
	s_UpdateContactsTaskParams.UserData[19].asInt = phManifold::sm_PositiveDepthMode;
#endif

#if !ENSURE_ART_VELOCITY_PROP_IN_UPDATE
 	PF_START(EnsureAllArtVelocitiesFullyPropagated);
 	phArticulatedCollider::EnsureAllArtVelocitiesFullyPropagated();
 	PF_STOP(EnsureAllArtVelocitiesFullyPropagated);
#endif // !ENSURE_ART_VELOCITY_PROP_IN_UPDATE

	bool mainThreadHelps = phConfig::GetContactMainThreadAlsoWorks();

	m_UpdateContactsTasks->Initiate(s_UpdateContactsTaskParams, phConfig::GetContactWorkerThreadCount());

	if (!bJustRunConstraintFunctions)
	{
		PF_START_TIMEBAR_PHYS_CMGR_DETAIL("Split Pairs");
		SplitPairs(pairArray);

		PF_START_TIMEBAR_PHYS_CMGR_DETAIL("Compute Islands");
		// We only want to add non-touching active objects to the islands if we're not doing push collisions
		bool mainPairArray = (pairArray == PHSIM->GetOverlappingPairArray());
		bool breakingPairs = (pairArray == m_BreakingPairs);
		pairArray->ComputeIslands(mainPairArray || breakingPairs, mainPairArray);
	}

	PF_START_TIMEBAR_PHYS_CMGR_DETAIL("Flush contact jobs");

	if (mainThreadHelps)
	{
		UpdateContactsTask(s_UpdateContactsTaskParams);
	}

	m_UpdateContactsTasks->Wait();
}

static void UpdateContactsInManifold(phManifold* manifold)
{
	phInst*     instA    = manifold->GetInstanceA();
	phInst*     instB    = manifold->GetInstanceB();
	if (instA && instB)
	{
		int         callA    = instA->GetInstFlag(phInst::FLAG_QUERY_EXTERN_VEL);
		int         callB    = instB->GetInstFlag(phInst::FLAG_QUERY_EXTERN_VEL);
#if !CALL_ACTIVE_OBJECT_GETEXTERNVELOCITY
		callA                = callA && !PHLEVEL->IsActive(instA->GetLevelIndex());
		callB                = callB && !PHLEVEL->IsActive(instB->GetLevelIndex());
#endif
		for (int contactIdx = 0; contactIdx < manifold->GetNumContacts(); ++contactIdx)
		{
			phContact& cp = manifold->GetContactPoint(contactIdx);
			Vector3 v(Vector3::ZeroType);

			// Here we subtract a from b (b-a), but it should be thought of as -(a-b) since 
			// we are negating (a-b) since a imparts its externally controlled velocity on b, 
			// and vice versa. 
			if (callB)
			{
				Vec3V pos     = cp.GetWorldPosB();
				int   compIdx = manifold->GetComponentB();
				v = VEC3V_TO_VECTOR3(instB->GetExternallyControlledLocalVelocity(pos.GetIntrin128(), compIdx));
			}

			if (callA)
			{
				Vec3V   pos     = cp.GetWorldPosA();
				int     compIdx = manifold->GetComponentA();
				Vector3 va = VEC3V_TO_VECTOR3(instA->GetExternallyControlledLocalVelocity(pos.GetIntrin128(), compIdx));
				v.Subtract(va);
			}

			cp.SetTargetRelVelocity(cp.GetTargetRelVelocity() + RCC_VEC3V(v));
			// Assert that we're not getting huge from accumulating
			ASSERT_ONLY(float targetRelVelocityMag = Mag(cp.GetTargetRelVelocity()).Getf();)
			Assertf(targetRelVelocityMag <= 10.0f*DEFAULT_MAX_SPEED, "Large target relative velocity value detected! We're probably failing to reset to zero anywhere in the frame. Mag: %f", Mag(cp.GetTargetRelVelocity()).Getf());
		}
	}
}

void phContactMgr::UpdateContactsExternVelocity()
{
	PF_FUNC(UpdateContactsExternVelocity);

	for (int mIdx = 0; mIdx < m_ManifoldsWithExternVel.GetCount(); ++mIdx)
	{
		phManifold* rootManifold = m_ManifoldsWithExternVel[mIdx];

		if(rootManifold->CompositeManifoldsEnabled())
		{
			for(int cmIdx = 0; cmIdx < rootManifold->GetNumCompositeManifolds(); ++cmIdx)
			{
				UpdateContactsInManifold(rootManifold->GetCompositeManifold(cmIdx));
			}
		}
		else
		{
			UpdateContactsInManifold(rootManifold);
		}
	}

	m_ManifoldsWithExternVel.ResetCount();
}

void phContactMgr::SolveConstraints(phOverlappingPairArray* pairArray, float timeStep)
{
	PF_START(Solve);
	SolveImpulsesPhase1And2(pairArray, timeStep,
#if EARLY_FORCE_SOLVE
		sm_UsePushes == false
#else
		true
#endif
		, false);

	PF_STOP(Solve);

	PF_START(BreakTotal);

	if (sm_BreakingEnabled EARLY_FORCE_SOLVE_ONLY(&& sm_UsePushes == false))
	{
		phOverlappingPairArray* breakPairs = pairArray;

		const int MAX_NUM_BREAKS = 8;
		int numBreaks = 0;
		while (numBreaks < MAX_NUM_BREAKS && PHSIM->AllowBreaking() && BreakObjects(breakPairs))
		{
			++numBreaks;

			PF_START(BreakUpdateContacts);
			UpdateContacts(breakPairs, timeStep, false, false);
			PF_STOP(BreakUpdateContacts);

			PF_START(BreakSolve);
			SolveImpulsesPhase1And2(breakPairs, timeStep, false, true);
			PF_STOP(BreakSolve);
		}

		if (numBreaks >= MAX_NUM_BREAKS)
		{
			Warningf("Breaking aborted due to exceeding max break iterations, %d", MAX_NUM_BREAKS);
		}

		// Since we are not currently in the middle of a breaking operation, set the breaking pairs to the whole list,
		// so ReplaceInstanceComponent will search that instead.
		m_BreakingPairs = pairArray;
		m_FirstPairInIsland = 0;

		PHCONSTRAINT->BreakConstraints();
	}

	PF_STOP(BreakTotal);

	PF_START(OptionalIters);

	// Check if extra iterations are needed for ragdolls
	if (sm_OptionalItersEnabled EARLY_FORCE_SOLVE_ONLY(&& sm_UsePushes == false))
	{
		phOverlappingPairArray* optionalIterPairs = pairArray;

		// Gather list of pairs that belonged to islands which contained a pair with the 'extra ragdoll iterations' flag set
		if (GatherOptionalIterPairs(optionalIterPairs))
		{
			// Run the extra iterations
			PF_START(OptionalItersUpdateContacts);
			SplitPairs(optionalIterPairs);
			PF_STOP(OptionalItersUpdateContacts);

			PF_START(OptionalItersSolve);
			SolveImpulses(optionalIterPairs, timeStep, sm_LCPSolverIterationsOptional, false, false, false);
			PF_STOP(OptionalItersSolve);
		}

		m_FirstPairInIsland = 0;
	}

	PF_STOP(OptionalIters);
}

void phContactMgr::SolveImpulsesPhase1And2(phOverlappingPairArray* pairArray, float timeStep, bool applyWarmStart, bool clearWarmStart)
{
	PF_START(InitArticulated);
	phArticulatedCollider::InitAllAccumJointImpulse();
	PF_STOP(InitArticulated);

	if (PHCONSTRAINT->HasWorldConstraints() && sm_LCPSolverIterationsFinal > 0)
	{
		SolveImpulses(pairArray,timeStep,sm_LCPSolverIterations,true && applyWarmStart,clearWarmStart, applyWarmStart || clearWarmStart);
		PHCONSTRAINT->EnforceWorldConstraints();
		UpdateContacts(pairArray, timeStep, true, false);
		SolveImpulses(pairArray,timeStep,sm_LCPSolverIterationsFinal,false,false,false);
	}
	else
	{
		SolveImpulses(pairArray,
			timeStep,
			sm_LCPSolverIterations + sm_LCPSolverIterationsFinal,
			true && applyWarmStart,
			clearWarmStart,
			applyWarmStart || clearWarmStart);
	}

	PF_START(ExtraConstraintIterations);
	PHCONSTRAINT->PerformExtraConstraintIterations(1.0f/timeStep, sm_UsePushes, sm_LCPSolverIterationsExtra, m_ForceSolvers[0]);
	PF_STOP(ExtraConstraintIterations);
}

void phContactMgr::FillInForceSolverMembers(phForceSolver& forceSolver, u32 numIterations, float timeStep, bool applyWarmStart, bool clearWarmStart, bool preResponse, phOverlappingPairArray* pairArray)
{
	forceSolver.m_NumPhases = m_NumPara;
	forceSolver.m_NumIterations = numIterations;
	forceSolver.m_ExtraReleasesAtEnd = Max(0, phConfig::GetForceSolverWorkerThreadTotalCount() - int(m_InitialPhasesToRelease));
	forceSolver.m_TimeStep = timeStep;
	forceSolver.m_SeparateBias = sm_SeparationBias;
	forceSolver.m_ComputePushesAndTurns = sm_UsePushes;
	forceSolver.m_AllowedPenetration = phSimulator::GetAllowedPenetration();
	forceSolver.m_AllowedAnglePenetration = phSimulator::GetAllowedAnglePenetration();
	forceSolver.m_MinBounce = sm_MinBounce;
	forceSolver.m_ApplyWarmStart = applyWarmStart;
	forceSolver.m_ClearWarmStart = clearWarmStart;
#if TURN_CLAMPING
	forceSolver.m_MaxTurn = sm_MaxTurn;;
#endif
	forceSolver.m_BaumgarteJointLimitTerm = sm_BaumgarteJointLimitTerm;
	forceSolver.m_PhaseAllocateAtom = &m_PhaseAllocateAtom;
	forceSolver.m_PhaseReleaseAtom = &m_PhaseReleaseAtom;
	forceSolver.m_PhaseReleaseSema = m_PhaseReleaseSema;
	forceSolver.m_PreResponse = preResponse;
	PS3_ONLY(forceSolver.m_DmaPlanAsList = g_DmaPlanAsList;)

	forceSolver.m_ParallelGroups = m_ParaGroup;
	forceSolver.m_OverlappingPairs = pairArray;
}

void phContactMgr::SolveImpulses(phOverlappingPairArray* pairArray, float timeStep, u32 numIterations, bool applyWarmStart, bool clearWarmStart, bool preResponse)
{
	Assert(!clearWarmStart || !applyWarmStart);

	if (m_NumPara == 0 || 0==numIterations)
	{
		return;
	}

	int numWorkerThreads = phConfig::GetForceSolverWorkerThreadCount();
	bool mainThreadHelps = phConfig::GetForceSolverMainThreadAlsoWorks();

	if (mainThreadHelps)
	{
		if (numWorkerThreads == 0)
		{
			// Enforce all constraints on the main CPU single-threaded
			phForceSolver& forceSolver = m_ForceSolvers[0];

			FillInForceSolverMembers(forceSolver, numIterations, timeStep, applyWarmStart, clearWarmStart, preResponse, pairArray);

			forceSolver.SolveSingleThreaded();

			return;
		}
	}

	// Restart the atoms so they are ready to record our march through the phases
	m_PhaseAllocateAtom = 0;
	m_PhaseReleaseAtom = 0;

	// Signal the sema once for every phase which could be started right away, so that the thread will pause if even one more
	//     is requested before the results from the first one are committed
	u32 windowSize = numWorkerThreads + phConfig::GetForceSolverOutOfOrderTolerance();
	windowSize = Min(32U, windowSize);

	u32 currentWindowSize = m_CurrentWindowSize;

	sysIpcSignalSema(m_PhaseReleaseSema, m_InitialPhasesToRelease);

#if TRACK_PHASE_RELEASE_SEMA_COUNT
	Displayf("initial release: sema %d", phIpcQuerySema(m_PhaseReleaseSema));
#endif // TRACK_PHASE_RELEASE_SEMA_COUNT

	m_CurrentWindowSize = currentWindowSize;

	for(int i=0;i<numWorkerThreads;i++)
	{
		phForceSolver& forceSolver = m_ForceSolvers[i];

		FillInForceSolverMembers(forceSolver, numIterations, timeStep, applyWarmStart, clearWarmStart, preResponse, pairArray);

		sysTaskParameters& params = *m_ForceSolverParams[i];

		sysMemZeroWords<sizeof(sysTaskParameters)/4>(&params);

		params.Input.Data = (void *)(&forceSolver);
		params.Input.Size = sizeof(phForceSolver);

		params.Output.Data = (void *)(&forceSolver);
		params.Output.Size = sizeof(phForceSolver);
	}

	if (sm_UsePushes)
	{
		m_ForceSolverPushTasks->Initiate(m_ForceSolverParams, phConfig::GetForceSolverWorkerThreadCount());
	}
	else
	{
		m_ForceSolverTasks->Initiate(m_ForceSolverParams, phConfig::GetForceSolverWorkerThreadCount());
	}

#if !__PPU
	if (mainThreadHelps)
	{
		Assert(numWorkerThreads < phConfig::GetTotalMaxNumWorkerThreads());
		phForceSolver& forceSolver = m_ForceSolvers[numWorkerThreads];

		FillInForceSolverMembers(forceSolver, numIterations, timeStep, applyWarmStart, clearWarmStart, preResponse, pairArray);

		forceSolver.Solve();
	}
#endif

	if (sm_UsePushes)
	{
		m_ForceSolverPushTasks->Wait();
	}
	else
	{
		m_ForceSolverTasks->Wait();
	}

	// The variable size window introduces a complication for managing the semaphore. The way the threads
	// know to terminate is that they allocate a phase index which is greater than the total number of phases,
	// which requires the semaphore to be signaled an extra time. But the variable size phase window might
	// be too big, so that the worker threads don't consume all the extra semaphore signals. In this case we
	// wait until they all complete and correct the semaphore on the main thread afterwards.
	//
	// See also: the similar comment in phForceSolver::Solve about this
	if (m_InitialPhasesToRelease > u32(phConfig::GetForceSolverWorkerThreadTotalCount()))
	{
		u32 releasesAtEnd = m_InitialPhasesToRelease - phConfig::GetForceSolverWorkerThreadTotalCount();
		sysIpcWaitSema(m_PhaseReleaseSema, releasesAtEnd);

#if TRACK_PHASE_RELEASE_SEMA_COUNT
		Displayf("final alloc: sema %d", phIpcQuerySema(m_PhaseReleaseSema));
#endif // TRACK_PHASE_RELEASE_SEMA_COUNT
	}

	Assert(phIpcQuerySema(m_PhaseReleaseSema) == 0);
}

class BreakObjectsFunctor
{
	phContactMgr* m_ContactMgr;

public:
	BreakObjectsFunctor(phContactMgr* contactMgr)
		: m_ContactMgr(contactMgr)
	{
	}

	__forceinline bool Process(phTaskCollisionPair& pair) const
	{
		m_ContactMgr->CollectImpulsesFromPair(pair);

		return false;
	}

	__forceinline bool NextIsland() const
	{
		m_ContactMgr->BreakObjectsInIsland();

		return false;
	}
};

class OptionalIterFunctor
{
	phContactMgr* m_ContactMgr;

public:
	OptionalIterFunctor(phContactMgr* contactMgr)
		: m_ContactMgr(contactMgr)
	{
	}

	__forceinline bool Process(phTaskCollisionPair& pair) const
	{
		m_ContactMgr->CheckPairForOptionalItersFlag(pair);

		return false;
	}

	__forceinline bool NextIsland() const
	{
		m_ContactMgr->IslandNeedsOptionalIters();

		return false;
	}
};


void phContactMgr::CollectImpulsesFromManifold(phManifold* manifold, bool collectForA, bool collectForB)
{
	Assert(collectForA || collectForB);

	bool nonZeroImpulse[phManifold::MANIFOLD_CACHE_SIZE];
	Vec3V contactImpulses[phManifold::MANIFOLD_CACHE_SIZE];

	bool isArticulatedFixedCollision = manifold->IsArticulatedFixedCollision();

	// Many contacts have an extremely small impulse and could force us to run breaking when we
	//   know there will be none. If an instance had a zero breaking strength and was bumped with
	//   an impulse under this threshold it won't break, but if the impulse is under the threshold
	//   you wouldn't expect anything to happen anyways. 
	const Vec3V minImpulseForBreaking(V_FLT_SMALL_3);

	// First Loop
	//   -Find the impulse of each contact
	//   -Count the number of non-zero impulse contacts (required for second loop)
	const int numContacts = manifold->GetNumContacts();
	int numContactsWithImpulse = 0;
	for(int contactIndex = 0; contactIndex < numContacts; ++contactIndex)
	{
		const phContact& contact = manifold->GetContactPoint(contactIndex);
		if(contact.IsContactActive())
		{
			Vec3V impulse;
			if (isArticulatedFixedCollision)
			{
#if __ASSERT
				if( IsGreaterThanAll(Abs(Subtract(Mag(contact.GetWorldNormal()), ScalarV(V_ONE))), ScalarVFromF32(POLYGONMAXNORMALERROR)) != 0)
				{
					manifold->AssertContactNormalVerbose(contact, __FILE__, __LINE__);
				}
#endif
				impulse = contact.ComputeTotalArtFixImpulse();
			}
			else
			{
				impulse = contact.ComputeTotalImpulse();
			}

			if(IsLessThanAll(Abs(impulse),minImpulseForBreaking) == 0)
			{
				++numContactsWithImpulse;
				nonZeroImpulse[contactIndex] = true;
				contactImpulses[contactIndex] = impulse;
			}
			else
			{
				nonZeroImpulse[contactIndex] = false;
			}
		}
		else
		{
			nonZeroImpulse[contactIndex] = false;
		}
	}

	if(numContactsWithImpulse > 0)
	{
		phOverlappingPairArray* pairArray = m_PreviousBreakingPairs;

#if HACK_GTA4_FRAG_BREAKANDDAMAGE
		phInst* pInstA = manifold->GetInstanceA();
		phInst* pInstB = manifold->GetInstanceB();
#endif

		// The W-component of the first component's impulse vector is used as a flag
		//   for whether or not any component on that instance has an impulse. This lets 
		//   us skip breaking completely for many objects. 
		const ScalarV svIntOne(V_INT_1);

		// Second Loop
		//   -Only process contacts with a non-zero impulse
		//   -Let the user modify the breaking impulse of the contact
		//   -Add the modified breaking impulse to the array
		for (int contactIndex = 0; contactIndex < numContacts; ++contactIndex)
		{
			if (nonZeroImpulse[contactIndex])
			{
				phContact& contact = manifold->GetContactPoint(contactIndex);
				const Vec3V impulse = contactImpulses[contactIndex];
				const ScalarV impulseMag = Mag(impulse);
				const ScalarV impulseMagSquared = Scale(impulseMag,impulseMag);
				const Vec3V impulseNormalized = InvScaleSafe(impulse,impulseMag,Vec3V(V_ZERO));
				if (collectForA)
				{
					int firstComponentIndex = pairArray->objectsByIndex[manifold->GetLevelIndexA()] * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS;
					int arrayIndex = firstComponentIndex + manifold->GetComponentA();
					Assert (arrayIndex >= 0 && arrayIndex < m_ImpulsesArraySize);
#if HACK_GTA4_FRAG_BREAKANDDAMAGE
					// use callback on phInst to decide on breaking impulse
					if(pInstA)
					{
						ScalarV breakingMag = pInstA->ModifyImpulseMag(manifold->GetComponentA(), manifold->GetComponentB(), numContactsWithImpulse, impulseMagSquared, pInstB);
						if(breakingMag.Getf() > -1.0f)
						{
							ScalarV oldW = m_ImpulsesByComponent[firstComponentIndex].GetW();
							m_ImpulsesByComponent[arrayIndex] += Vec4V(Scale(impulseNormalized,breakingMag),ScalarV(V_ZERO));
							m_ImpulsesByComponent[firstComponentIndex].SetW(SelectFT(IsZero(breakingMag),svIntOne,oldW));
						}
						else
						{
							// If instance returns -1.0 then just use contact impulse
							m_ImpulsesByComponent[arrayIndex] += Vec4V(impulse,ScalarV(V_ZERO));
							m_ImpulsesByComponent[firstComponentIndex].SetW(svIntOne);
						}
					}
#else
					m_ImpulsesByComponent[arrayIndex] += Vec4V(impulse,ScalarV(V_ZERO));
					m_ImpulsesByComponent[firstComponentIndex].SetW(svIntOne);
#endif
					m_PositionsByComponent[arrayIndex] += Vec4V(Scale(contact.GetWorldPosA(),impulseMag), impulseMag);
				}
				if (collectForB)
				{
					int firstComponentIndex = pairArray->objectsByIndex[manifold->GetLevelIndexB()] * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS;
					int arrayIndex = firstComponentIndex + manifold->GetComponentB();
					Assert (arrayIndex >= 0 && arrayIndex < m_ImpulsesArraySize);
#if HACK_GTA4_FRAG_BREAKANDDAMAGE
					// use callback on phInst to decide on breaking impulse
					if(pInstB)
					{
						ScalarV breakingMag = pInstB->ModifyImpulseMag(manifold->GetComponentB(), manifold->GetComponentA(), numContactsWithImpulse, impulseMagSquared, pInstA);
						if(breakingMag.Getf() > -1.0f)
						{
							ScalarV oldW = m_ImpulsesByComponent[firstComponentIndex].GetW();
							m_ImpulsesByComponent[arrayIndex] -= Vec4V(Scale(impulseNormalized,breakingMag),ScalarV(V_ZERO));
							m_ImpulsesByComponent[firstComponentIndex].SetW(SelectFT(IsZero(breakingMag),svIntOne,oldW));
						}
						else
						{
							// If instance returns -1.0 then just use contact impulse
							m_ImpulsesByComponent[arrayIndex] -= Vec4V(impulse,ScalarV(V_ZERO));
							m_ImpulsesByComponent[firstComponentIndex].SetW(svIntOne);
						}
					}
#else
					m_ImpulsesByComponent[arrayIndex] -= Vec4V(impulse,ScalarV(V_ZERO));
					m_ImpulsesByComponent[firstComponentIndex].SetW(svIntOne);

#endif
					m_PositionsByComponent[arrayIndex] += Vec4V(Scale(contact.GetWorldPosB(),impulseMag), impulseMag);
				}
			}
		}
	}
}

void phContactMgr::CollectImpulsesFromPair(phTaskCollisionPair& pair)
{
	phManifold* manifold = pair.manifold;

	bool collectForA = true;
	bool collectForB = true;
	// A null instance and invalid level index should be strictly tied, just being extra safe
	if(!manifold->GetInstanceA() || !manifold->GetInstanceA()->IsBreakable(manifold->GetInstanceB())
		|| manifold->GetLevelIndexA() == phInst::INVALID_INDEX)
	{
		collectForA = false;
	}
	if(!manifold->GetInstanceB() || !manifold->GetInstanceB()->IsBreakable(manifold->GetInstanceA())
		|| manifold->GetLevelIndexB() == phInst::INVALID_INDEX)
	{
		collectForB = false;
	}
	//
	if(!collectForA && !collectForB)
	{
		return;
	}

	if(m_BreakingPairs->pairs.GetCount() == m_BreakingPairs->pairs.GetCapacity())
	{
		// NOTE: This shouldn't be necessary but we need more scratch memory to guarantee it doesn't happen
		Warningf("Not enough room to add pair to breaking OPA.");
		return;
	}

	bool anyContactsOnThisPair = false;
	if (manifold->CompositeManifoldsEnabled())
	{
		for (int manifoldIndex = 0; manifoldIndex < manifold->GetNumCompositeManifolds(); ++manifoldIndex)
		{
			phManifold* compositeManifold = manifold->GetCompositeManifold(manifoldIndex);
			if(compositeManifold->GetNumContacts() > 0)
			{
				CollectImpulsesFromManifold(compositeManifold, collectForA, collectForB);
				anyContactsOnThisPair = true;
			}
		}
	}
	else if (manifold->GetNumContacts() > 0)
	{
		CollectImpulsesFromManifold(manifold, collectForA, collectForB);
		anyContactsOnThisPair = true;
	}

	if(anyContactsOnThisPair)
	{
		m_BreakingPairs->pairs.Append() = pair;
	}
}

void phContactMgr::BreakObjectsInIsland()
{
	phOverlappingPairArray* pairArray = m_PreviousBreakingPairs;

	if (!Verifyf((m_CurrentIsland) < pairArray->numIslands, "Skipping break island due to out-of-range island index."))
	{
		return;
	}

	int firstObject = 0;

	if (m_CurrentIsland > 0)
	{
		firstObject = pairArray->firstObjectInIsland[m_CurrentIsland - 1];
	}

	int numObjects = pairArray->firstObjectInIsland[m_CurrentIsland] - firstObject;

	float breakStrength;
	phBreakData breakData0,breakData1;
	phBreakData* breakData = &breakData0;
	phBreakData* testBreakData = &breakData1;

	if (phInstBreakable* weakestInst = FindWeakestInst(&pairArray->objectsByIsland[firstObject],
		(Vector4*)m_ImpulsesByComponent + (firstObject * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS),
		(Vector4*)m_PositionsByComponent + (firstObject * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS),
		numObjects,
		&breakStrength,
		&breakData,
		&testBreakData))
	{
		m_AnyBroken = true;

		//ApplyPreBreakingImpulses();
		bool wasWasWeakestInstanceFixed = !PHLEVEL->IsActive(weakestInst->GetLevelIndex());
		phInst* newInst = weakestInst->BreakApart(*breakData);

		// Restore the velocities back to where they were before we solved constraints
		RevertImpulses(&pairArray->objectsByIsland[firstObject], numObjects);

		// phInstBreakable::BreakApart probably shouldn't give us instances not in the level
		if(newInst && PHLEVEL->IsInLevel(newInst->GetLevelIndex()))
		{
			// Apply the post breaking impulse to the new instance. This could either be the same instance
			//   we broke if there was a root break, or a brand new instance. 
			ApplyPostBreakingImpulses(*newInst,wasWasWeakestInstanceFixed,ScalarVFromF32(breakStrength));
		}

		// Set m_FirstPairInIsland so it can be used for ReplaceInstanceComponent
		m_FirstPairInIsland = m_BreakingPairs->pairs.GetCount();
	}
	else
	{
		// Erase the pairs that we collected from this island, since it didn't break
		m_BreakingPairs->pairs.Resize(m_FirstPairInIsland);
	}

	++m_CurrentIsland;
}

void phContactMgr::ResetBreakingPairs(phOverlappingPairArray* pairs)
{
	m_BreakingPairs = pairs;
	m_FirstPairInIsland = 0;
}

bool phContactMgr::BreakObjects(phOverlappingPairArray*& pairArray)
{
	PF_FUNC(BreakObjects);

	phLevelNew *physLevel = PHLEVEL;
	phBroadPhase *broadPhase = physLevel->GetBroadPhase();
	broadPhase->SetPreBreakingPairMarker();

	if (pairArray->numObjects == 0)
	{
		return false;
	}

	// Each island can have one break, each of which can result in MAX_NUM_BREAKABLE_COMPONENTS objects being added if a break-on-delete
	// group has multiple children.
	int possibleNumObjectsAfterBreaking = pairArray->numObjects + pairArray->numIslands * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS;
	int numInstances = physLevel->GetMaxObjects();
	const u32 maxNumPairs = pairArray->pairs.GetCount() * 2;
	const u32 phaseWindowSize = 32;
	int maxLevelIndex = physLevel->GetMaxObjects();

	size_t neededSpace;
	neededSpace = pairArray->numObjects * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS * sizeof(Vec3V); // m_ImpulsesByComponent->m_Elements
	neededSpace += pairArray->numObjects * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS * sizeof(Vec4V); // m_PositionsByComponent->m_Elements
	neededSpace += phOverlappingPairArray::ComputeSizeForNewClass(maxNumPairs, possibleNumObjectsAfterBreaking, maxLevelIndex);
	neededSpace += ComputeSplitPairSize(numInstances, phaseWindowSize, maxNumPairs);

	neededSpace += pairArray->numObjects * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS * sizeof(float); // m_MinBreakingImpulsesByComponent
	neededSpace += pairArray->numIslands * sizeof(float); // m_MinBreakingImpulsesByIsland

	if (neededSpace > GetHalfScratchpadSize())
	{
		Warningf("Physics breaking operation aborted due to scratchpad running out of space.");

		MarkScratchpadUsed((u32)neededSpace);

		return false;
	}

	InitNextHalfScratchpad();

	StartHalfScratchpad();

	int impulsesArraySize = m_ImpulsesArraySize = pairArray->numObjects * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS;
	m_ImpulsesByComponent = rage_new Vec4V[impulsesArraySize];
	sysMemSet(m_ImpulsesByComponent, 0, sizeof(Vec4V) * impulsesArraySize);
	m_PositionsByComponent = rage_new Vec4V[impulsesArraySize];
	sysMemSet(m_PositionsByComponent, 0, sizeof(Vec4V) * impulsesArraySize);

	m_PreviousBreakingPairs = m_BreakingPairs;
	m_BreakingPairs = rage_new phOverlappingPairArray(maxNumPairs, possibleNumObjectsAfterBreaking, maxLevelIndex);

	EndHalfScratchpad();

	m_CurrentIsland = 0;
	m_FirstPairInIsland = 0;
	m_AnyBroken = false;

	pairArray->IterateIslandPairs(BreakObjectsFunctor(this));

	pairArray = m_BreakingPairs;

	return m_AnyBroken;
}

phInstBreakable* phContactMgr::FindWeakestInst (u16* breakInstList,
	Vector4* breakingImpulses,
	Vector4* breakingPositions,
	int numBreakInsts,
	float* breakStrength,
	phBreakData** breakData,
	phBreakData** testBreakData) const
{
	// Initialize the breaking strength at 1, which means that no breaking occurs because the pre-breaking impetuses are the
	// same as the non-breaking impetuses.
	(*breakStrength) = 1.0f;

	// Loop over all the breakable instances in these collisions to find the one that will break most easily.
	phInstBreakable* weakestInst = NULL;
	phInstBreakable* testInst;
	float testBreakStrength;
	for (int instIndex=0; instIndex<numBreakInsts; instIndex++)
	{
		u16 levelIndex = breakInstList[instIndex];
		if(PHLEVEL->IsInLevel(levelIndex))
		{
			int firstComponentIndex = instIndex * phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS;
			// If any components have impulse then the W-component of the first impulse will be marked
			if(RCC_VEC4V(breakingImpulses[firstComponentIndex]).GetWi() > 0)
			{
				if (phInst* inst = PHLEVEL->GetInstance(levelIndex))
				{
					if(inst->IsBreakable(NULL))
					{
						// Get the breaking strength for this instance in these collisions (0=breaks easily, 1=doesn't break).
						testInst = static_cast<phInstBreakable*>(inst);
						if (testInst->FindBreakStrength(
							(Vector3*)&breakingImpulses[firstComponentIndex],
							&breakingPositions[firstComponentIndex],
							&testBreakStrength,
							*testBreakData) &&
							testBreakStrength<(*breakStrength))
						{
							// This is the most easily breakable instance found so far.
							(*breakStrength) = testBreakStrength;
							SwapEm(*breakData,*testBreakData);
							weakestInst = testInst;
						}
					}
				}
			}
		}
	}

	// Return the most easily breakable instance in these collisions, or NULL if these collisions
	// will not cause any of the instances to break.
	return weakestInst;
}

void phContactMgr::RemoveAllContactsWithInstance (phInst* instance)
{
	PHLOCK_SCOPEDREADLOCK;

	if(!PHSIM->IsSafeToModifyManifolds())
	{
		NOTFINAL_ONLY(Quitf("Trying to modify manifolds in a callback while it isn't safe.");)
		return;
	}

	Assert(instance != NULL);
	for (int pairIndex = m_FirstPairInIsland; pairIndex < m_BreakingPairs->pairs.GetCount(); ++pairIndex)
	{
		phManifold* manifold = m_BreakingPairs->pairs[pairIndex].manifold;
		if (manifold->GetInstanceA()==instance || manifold->GetInstanceB()==instance)
		{
#if USE_FRAME_PERSISTENT_GJK_CACHE
			manifold->ResetGJKCache();
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
			manifold->RemoveAllContacts();
		}
	}
}


void phContactMgr::RemoveAllContactsWithInstance (phInst* instance, atFixedBitSet<MAX_NUM_BREAKABLE_COMPONENTS>& componentBits)
{
	PHLOCK_SCOPEDREADLOCK;

	if(!PHSIM->IsSafeToModifyManifolds())
	{
		NOTFINAL_ONLY(Quitf("Trying to modify manifolds in a callback while it isn't safe.");)
		return;
	}

	Assert(instance != NULL);
	Assertf(instance->GetArchetype()->GetBound()->GetType() == phBound::COMPOSITE, "Passed in an instance with non-composite bound - use the other RemoveAllContactsWithInstance() instead.");
	for (int pairIndex = m_FirstPairInIsland; pairIndex < m_BreakingPairs->pairs.GetCount(); ++pairIndex)
	{
		phManifold* rootManifold = m_BreakingPairs->pairs[pairIndex].manifold;
		if (rootManifold->GetInstanceA()==instance || rootManifold->GetInstanceB()==instance)
		{
#if USE_FRAME_PERSISTENT_GJK_CACHE
			rootManifold->ResetGJKCache();
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
			const int matchInstanceSlot = rootManifold->GetInstanceA()==instance ? 0 : 1;
			if(rootManifold->CompositeManifoldsEnabled())
			{
				const int numSubManifolds = rootManifold->GetNumCompositeManifolds();
				for(int subManifoldIndex = 0; subManifoldIndex < numSubManifolds; ++subManifoldIndex)
				{
					if(componentBits.IsSet(rootManifold->GetCompositePairComponent(matchInstanceSlot, subManifoldIndex)))
					{
						phManifold *subManifold = rootManifold->GetCompositeManifold(subManifoldIndex);
						subManifold->RemoveAllContacts();
					}
				}
			}
			else
			{
				if(rootManifold->IsConstraint())
				{
					if(componentBits.IsSet(rootManifold->GetComponent(matchInstanceSlot)))
					{
						rootManifold->RemoveAllContacts();
					}
				}
				// If it's not a composite manifold and it's not a constraint manifold we don't need to do anything to handle it and its component numbers
				//   don't hold any useful information so we don't want to look at them.
			}
		}
	}
}

void phContactMgr::ResetWarmStartAllContactsWithInstance (const phInst* instance)
{
	PHLOCK_SCOPEDREADLOCK;

	Assert(instance != NULL);
	for (int pairIndex = m_FirstPairInIsland; pairIndex < m_BreakingPairs->pairs.GetCount(); ++pairIndex)
	{
		phManifold* rootManifold = m_BreakingPairs->pairs[pairIndex].manifold;
		if (rootManifold->GetInstanceA()==instance || rootManifold->GetInstanceB()==instance)
		{
			rootManifold->ResetWarmStart();
		}
	}
}


void phContactMgr::UpdateColliderInManifoldsWithInstance (const phInst* instance, phCollider* collider)
{
	Assertf(instance != NULL, "Don't pass in NULL for the instance, how could that possibly make sense?");

	const ptrdiff_t originalLevelIndex = instance->GetLevelIndex(); // The level index of the object being replaced
	Assert(originalLevelIndex != phInst::INVALID_INDEX);

	phOverlappingPairArray *curOverlappingPairArray = m_BreakingPairs;
	atArray<phTaskCollisionPair, 0, u32> &taskCollisionPairs = curOverlappingPairArray->pairs;
	int numTCPairs = taskCollisionPairs.GetCount();

	for(int tcPairIndex = m_FirstPairInIsland; tcPairIndex < numTCPairs; ++tcPairIndex)
	{
		// Grab the current task collision pair.
		phTaskCollisionPair &curPair = taskCollisionPairs[tcPairIndex];
		const ptrdiff_t pairLevelIndexA = curPair.levelIndex1;
		const ptrdiff_t pairLevelIndexB = curPair.levelIndex2;

		// Check a bit for consistency.  We expect the ordering in the phTaskCollisionPair to match the ordering in the manifold.
		Assert(curPair.manifold != NULL);
		Assert((int)curPair.manifold->GetLevelIndexA() == pairLevelIndexA);
		Assert((int)curPair.manifold->GetLevelIndexB() == pairLevelIndexB);

		const size_t matchesOldLevelIndexAMask = GenerateMaskEq(pairLevelIndexA, originalLevelIndex);
		const size_t matchesOldLevelIndexBMask = GenerateMaskEq(pairLevelIndexB, originalLevelIndex);
		if((matchesOldLevelIndexAMask | matchesOldLevelIndexBMask) != 0)
		{
			phManifold *pairRootManifold = curPair.manifold;

			if( pairRootManifold->GetInstanceA() == instance || pairRootManifold->GetInstanceB() == instance )
			{
				Assert(pairRootManifold->GetInstanceA() == instance || pairRootManifold->GetInstanceB() == instance);

				// It is possible for a manifold to involve just a single inst. For example, an articulated body in contact with itself.
				// This is fine, we just need to make sure to update both colliders in the manifold if so.
				phCollider *previousColliderA = pairRootManifold->GetColliderA();
				phCollider *previousColliderB = pairRootManifold->GetColliderB();
				pairRootManifold->SetColliderA(ISelectP<phCollider>(matchesOldLevelIndexAMask, previousColliderA, collider));
				pairRootManifold->SetColliderB(ISelectP<phCollider>(matchesOldLevelIndexBMask, previousColliderB, collider));

				if(pairRootManifold->CompositeManifoldsEnabled())
				{
					const int numSubManifolds = pairRootManifold->GetNumCompositeManifolds();
					for(int subManifoldIndex = 0; subManifoldIndex < numSubManifolds; ++subManifoldIndex)
					{
						phManifold *subManifold = pairRootManifold->GetCompositeManifold(subManifoldIndex);
						subManifold->SetColliderA(ISelectP<phCollider>(matchesOldLevelIndexAMask, previousColliderA, collider));
						subManifold->SetColliderB(ISelectP<phCollider>(matchesOldLevelIndexBMask, previousColliderB, collider));
					}
				}
			}
		}
	}
}

void phContactMgr::RevertImpulses(u16* objects, int numObjects)
{
	for (int index = 0; index < numObjects; ++index)
	{
		phCollider* collider = PHSIM->GetCollider(objects[index]);
		if (collider)
		{
			collider->RevertImpulses();
		}
	}
}

class SleepFunctor
{
	phContactMgr* m_ContactMgr;

public:
	SleepFunctor(phContactMgr* contactMgr)
		: m_ContactMgr(contactMgr)
	{
	}

	__forceinline bool Process(int levelIndex, int objectIndex) const
	{
		return m_ContactMgr->SleepInst(levelIndex, objectIndex);
	}

	__forceinline bool NextIsland(int numObjects) const
	{
		return m_ContactMgr->NextSleepIsland(numObjects);
	}
};

void phContactMgr::UpdateSleep()
{
	phOverlappingPairArray* pairArray = PHSIM->GetOverlappingPairArray();

	m_SleepingCurrentIsland = false;
	m_IslandWantsToSleep = true;
	m_SleepIsland = NULL;
	m_ObjectsToSleep = 0;

	pairArray->IterateIslandInsts(SleepFunctor(this));
}

bool phContactMgr::SleepInst(int levelIndex, int objectIndex)
{
	// Object "levelIndex" was found in the island we are currently iterating over

	// Are we in "deactivate all the objects in this island mode"?
	if (m_SleepingCurrentIsland)
	{
		if (PHLEVEL->LegitLevelIndex(levelIndex) && !PHLEVEL->GetInstance(levelIndex)->GetInstFlag(phInst::FLAG_SLEEPS_ALONE))
		{
			if (m_SleepIsland)
			{
				// If we're storing this island in a sleep island, put the object info in there
				m_SleepIsland->SetObject(objectIndex, levelIndex);
			}
			else
			{
				// If we don't have a sleep island, just try to deactivate it
				if (PHLEVEL->IsActive(levelIndex))
				{
					PHSIM->DeactivateObject(levelIndex);
				}
			}
		}

		// Don't skip to the next island
		return false;
	}
	else
	{
		// If not, ask it whether it wants to vote for the current island to sleep
		if (phCollider* collider = PHSIM->GetCollider(levelIndex))
		{
			phSleep* sleep = collider->GetSleep();
			bool awake = !sleep || !sleep->IsDoneSleeping() || sleep->IsActiveSleeper();

			phInst* inst = collider->GetInstance();
			if (inst->GetInstFlag(phInst::FLAG_SLEEPS_ALONE))
			{
				if (!awake)
				{
					if (PHLEVEL->IsActive(levelIndex))
					{
						PHSIM->DeactivateObject(levelIndex);
					}
				}
			}
			else
			{
				++m_ObjectsToSleep;

				if (awake)
				{
					m_IslandWantsToSleep = false;
				}
			}
		}

		// If we don't want to sleep, return false to signal that we want to skip to the next island
		return false;
	}
}

bool phContactMgr::NextSleepIsland(int numObjects)
{
	// We just reached the end of an island

	// Find out if we were just looping over an island to sleep it or to take a vote on sleeping
	if (m_SleepingCurrentIsland == false)
	{
		// We were just taking a vote, act on the results
		if (m_IslandWantsToSleep)
		{
			// The whole island wants to sleep, put us in that mode and return true to loop over this island again
			m_SleepingCurrentIsland = true;

			// Only use a sleep island if there is more than one object in this island
			if (m_ObjectsToSleep > 1 && sm_UseSleepIslands)
			{
				m_SleepIsland = PHSLEEP->AllocateSleepIsland(numObjects);
			}

			return true;
		}
		else
		{
			// The island does not currently want to sleep, so get ready and go on to the next island
			m_IslandWantsToSleep = true;
			m_ObjectsToSleep = 0;
			return false;
		}
	}
	else
	{
		// We just completed putting an island to sleep, get ready and go on the to the next island
		if (m_SleepIsland)
		{
			m_SleepIsland->SleepObjects();
			m_SleepIsland = NULL;
		}

		m_SleepingCurrentIsland = false;
		m_IslandWantsToSleep = true;
		m_ObjectsToSleep = 0;
		return false;
	}
}

CompileTimeAssert(phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS == phContactMgr::MAX_NUM_BREAKABLE_COMPONENTS);

bool phContactMgr::GatherOptionalIterPairs(phOverlappingPairArray*& pairArray)
{
	PF_FUNC(GatherOptionalIterPairs);

	if (pairArray->numObjects == 0)
	{
		return false;
	}

	int numTasks = phConfig::GetForceSolverWorkerThreadCount();
	if (phConfig::GetForceSolverMainThreadAlsoWorks())
	{
		numTasks += 1;
	}

	int possibleNumObjectsAfterBreaking = pairArray->numObjects + pairArray->numIslands;
	const u32 maxNumPairs = pairArray->pairs.GetCount() * 2;
	int maxLevelIndex = PHLEVEL->GetMaxObjects();

	InitNextHalfScratchpad();
	StartHalfScratchpad();
	m_OptionalIterPairs = rage_new phOverlappingPairArray(maxNumPairs, possibleNumObjectsAfterBreaking, maxLevelIndex);
	EndHalfScratchpad();

	m_FirstPairInIsland = 0;
	m_ContainedInstNeedsOptionalIters = false;

	pairArray->IterateIslandPairs(OptionalIterFunctor(this));

	pairArray = m_OptionalIterPairs;

	return pairArray->pairs.GetCount() > 0;
}

void phContactMgr::CheckPairForOptionalItersFlag(phTaskCollisionPair& pair)
{
	phManifold* manifold = pair.manifold;

	m_OptionalIterPairs->pairs.Append() = pair;

	if (!m_ContainedInstNeedsOptionalIters &&
		((manifold->GetInstanceA() && manifold->GetInstanceA()->GetInstFlag(phInst::FLAG_OPTIONAL_ITERATIONS)) ||
		(manifold->GetInstanceB() && manifold->GetInstanceB()->GetInstFlag(phInst::FLAG_OPTIONAL_ITERATIONS))))
	{
		m_ContainedInstNeedsOptionalIters = true;
	}
}

void phContactMgr::IslandNeedsOptionalIters()
{
	if (m_ContainedInstNeedsOptionalIters)
	{
		// Set m_FirstPairInIsland
		m_FirstPairInIsland = m_OptionalIterPairs->pairs.GetCount();
	}
	else
	{
		// Erase the pairs that we collected from this island, since they don't need extra iterations
		m_OptionalIterPairs->pairs.Resize(m_FirstPairInIsland);
	}

	m_ContainedInstNeedsOptionalIters = false;
}

void phContactMgr::ReplaceInstanceComponents (phInst* instance, atFixedBitSet<phContactMgr::MAX_NUM_BREAKABLE_COMPONENTS>& componentBits, phInst* newInstance)
{
	Assert(sysThreadType::IsUpdateThread());
	if(!PHSIM->IsSafeToModifyManifolds())
	{
		NOTFINAL_ONLY(Quitf("Trying to modify manifolds in a callback while it isn't safe.");)
		return;
	}

	Assertf(instance != newInstance, "It doesn't make sense to pass in the same instance for the new and old instance, what are you trying to accomplish?");
	Assertf(instance != NULL, "Don't pass in NULL for the old instance, how could that possibly make sense?");
	Assertf(newInstance != NULL, "Don't pass in NULL for the new instance.  If you're trying to clear out contacts call ReplaceInstanceComponent() (no 's' at the end - and yes, that function should get renamed).");

	const u16 originalLevelIndex = instance->GetLevelIndex(); // The level index of the object being replaced
	Assert(originalLevelIndex != phInst::INVALID_INDEX);
	const u16 newLevelIndex = newInstance->GetLevelIndex(); // The level index of the new object replacing the old object
	Assert(newLevelIndex != phInst::INVALID_INDEX);
	Assert(originalLevelIndex != newLevelIndex);

	phLevelNew *physLevel = PHLEVEL;

	const phHandle originalHandle = phHandle(originalLevelIndex,physLevel->GetGenerationID(originalLevelIndex));
	const phHandle newHandle = phHandle(newLevelIndex,physLevel->GetGenerationID(newLevelIndex));

	phCollider *newCollider = PHSIM->GetCollider(newLevelIndex);

	phBroadPhase *broadPhase = physLevel->GetBroadPhase();
	btBroadphasePair *broadPhasePairs = broadPhase->getPrunedPairs();
	const int numPreBreakingPairs = (int)broadPhase->GetNumPreBreakingPairs();
	// 'initial' because we might add some during this process and we don't need to iterate through those.
	const int numInitialBPPairs = broadPhase->getPrunedPairCount();

	phOverlappingPairArray *curOverlappingPairArray = m_BreakingPairs;
	atArray<phTaskCollisionPair, 0, u32> &taskCollisionPairs = curOverlappingPairArray->pairs;
	int numTCPairs = taskCollisionPairs.GetCount();

	for(int tcPairIndex = 0; tcPairIndex < numTCPairs; ++tcPairIndex)
	{
		// Grab the current task collision pair.
		phTaskCollisionPair &curPair = taskCollisionPairs[tcPairIndex];

		FastAssert(curPair.manifold);
		FastAssert(curPair.manifold->GetHandleA() == phHandle(curPair.levelIndex1,curPair.generationId1));
		FastAssert(curPair.manifold->GetHandleB() == phHandle(curPair.levelIndex2,curPair.generationId2));

		const phHandle pairHandleA = phHandle(curPair.levelIndex1,curPair.generationId1);
		const phHandle pairHandleB = phHandle(curPair.levelIndex2,curPair.generationId2);

		// Check a bit for consistency.  For one thing, we shouldn't come across any phTaskCollisionPairs that already reference the new instance.  We also
		//   expect the ordering in the phTaskCollisionPair to match the ordering in the manifold.
		Assertf(pairHandleA != newHandle, "Found a manifold that references the new object (%i,%i). ReplaceInstanceComponents should only be called once.", newHandle.GetLevelIndex(), newHandle.GetGenerationId());
		Assertf(pairHandleB != newHandle, "Found a manifold that references the new object (%i,%i). ReplaceInstanceComponents should only be called once.", newHandle.GetLevelIndex(), newHandle.GetGenerationId());
		Assertf(curPair.manifold != NULL, "Task collision pair (%i,%i) (%i,%i) doesn't have a manifold.",pairHandleA.GetLevelIndex(),pairHandleA.GetGenerationId(),pairHandleB.GetLevelIndex(),pairHandleB.GetGenerationId());
		Assertf(curPair.manifold->GetHandleA() == pairHandleA, "Manifold handle (%i,%i) doesn't match collision pair handle (%i,%i).", curPair.manifold->GetLevelIndexA(), curPair.manifold->GetGenerationIdA(), pairHandleA.GetLevelIndex(), pairHandleA.GetGenerationId());
		Assertf(curPair.manifold->GetHandleB() == pairHandleB, "Manifold handle (%i,%i) doesn't match collision pair handle (%i,%i).", curPair.manifold->GetLevelIndexB(), curPair.manifold->GetGenerationIdB(), pairHandleB.GetLevelIndex(), pairHandleB.GetGenerationId());

		const size_t matchesOldHandleAMask = GenerateMaskEq(pairHandleA, originalHandle);
		const size_t matchesOldHandleBMask = GenerateMaskEq(pairHandleB, originalHandle);
		if((matchesOldHandleAMask | matchesOldHandleBMask) != 0)
		{
			const int matchInstanceSlot = (matchesOldHandleBMask & 1);
			const int otherInstanceSlot = 1 - matchInstanceSlot;

			phManifold *pairRootManifold = curPair.manifold;

			// Let's gather some information about the 'other' object in the collision.
			const phHandle otherHandle = pairRootManifold->GetHandle(otherInstanceSlot);
			Assert(otherHandle != originalHandle || pairHandleA == pairHandleB);
			phInst *otherInstance = pairRootManifold->GetInstance(otherInstanceSlot);
			phCollider *otherCollider = pairRootManifold->GetCollider(otherInstanceSlot);
			
			// Find the slots for the new and other instance on the new manifolds. The lesser level index should be first. 
			const int newInstanceNewSlot = GenerateMaskLT(otherHandle.GetLevelIndex(),newLevelIndex) & 1;
			const int otherInstanceNewSlot = 1 - newInstanceNewSlot; 
			const bool swappingOtherSlot = (otherInstanceSlot != otherInstanceNewSlot);

			phTaskCollisionPair* newTaskCollisionPair = NULL;

			if(pairRootManifold->CompositeManifoldsEnabled())
			{

				if(!physLevel->LegitLevelIndex(otherHandle.GetLevelIndex()) || !physLevel->IsLevelIndexGenerationIDCurrent(otherHandle.GetLevelIndex(),otherHandle.GetGenerationId()))
				{
#if __BANK
					sysStack::PrintStackTrace();
					Displayf("phContactMgr::ReplaceInstanceComponents - Found Invalid Other Instance (%i, %i), New inst (%i, %i), Original Inst (%i, %i)", otherHandle.GetLevelIndex(),otherHandle.GetGenerationId(),newHandle.GetLevelIndex(),newHandle.GetGenerationId(),originalHandle.GetLevelIndex(),originalHandle.GetGenerationId());
#endif // __BANK
					// Transferring manifolds with removed instances is acceptable except when the 'other' and 'new' instances share a level index. In that case we would end up
					//   creating a self collision pair for the new object. Since nobody needs a deleted manifold it should be safe to move on
					for(int compositeManifoldIndex = 0; compositeManifoldIndex < pairRootManifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
					{
						Displayf("\tphContactMgr::ReplaceInstanceComponents - Removing %i contacts from manifold %i",pairRootManifold->GetCompositeManifold(compositeManifoldIndex)->GetNumContacts(),compositeManifoldIndex);
						pairRootManifold->GetCompositeManifold(compositeManifoldIndex)->RemoveAllContacts();
					}
					continue;
				}
				Assert(otherHandle.GetLevelIndex() != newLevelIndex);
				Assert(otherHandle.GetLevelIndex() != originalLevelIndex);
				Assert(otherInstance->IsInLevel());
				Assert(otherInstance->GetLevelIndex() == otherHandle.GetLevelIndex());

				// Forcing the user to remove manifolds on instance removal is really expensive. We're better off just removing the out of date manifolds during the 
				//   simulator update. 
				//Assertf(otherHandle.GetLevelIndex() == phInst::INVALID_INDEX || physLevel->IsNonexistent(otherHandle.GetLevelIndex()) || physLevel->GetGenerationID(otherHandle.GetLevelIndex()) == otherHandle.GetGenerationId(),"Other instance (%i,%i) is not current in level.", otherHandle.GetLevelIndex(),otherHandle.GetGenerationId());
				
				Assert(!pairRootManifold->IsConstraint());	// Constraints aren't supposed to be using composite manifolds.

				// TODO: There's not really any good reason to build up a temporary array like this and then copy it over, it's just that that's the current interface.
				//   It would be better to just write the values to their final locations.
				int numSurvivingSubManifolds = 0;
				phManifold *survivingOriginalSubManifolds[phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS];
				u8 survivingOriginalComponentPairs[2 * phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS];
				int numNewSubManifolds = 0;
				phManifold *newSubManifolds[phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS];
				u8 newComponentPairs[2 * phCompositePointers::MAX_NUM_COLLIDING_COMPOSITE_PAIRS];

				phManifold *newRootManifold = NULL;
				const int numSubManifolds = pairRootManifold->GetNumCompositeManifolds();
				for(int subManifoldIndex = 0; subManifoldIndex < numSubManifolds; ++subManifoldIndex)
				{
					Assert(pairRootManifold->CompositeManifoldsEnabled());
					phManifold *curSubManifold = pairRootManifold->GetCompositeManifold(subManifoldIndex);
					const int transferringObjectComponent = pairRootManifold->GetCompositePairComponent(matchInstanceSlot, subManifoldIndex);
					const int otherComponent = pairRootManifold->GetCompositePairComponent(otherInstanceSlot, subManifoldIndex);
					if(componentBits.IsSet(transferringObjectComponent))
					{
						// If we have a self collision, we'd better not find a situation in which we have two components that are colliding with each other and
						//   both think that they are breaking off because that would mean that we have an articulated piece breaking off which can't happen (right
						//   now, at least).
						Assert(pairHandleA != pairHandleB|| componentBits.IsClear(otherComponent));

						Assertf(curSubManifold->GetNumContacts() == 0 || static_cast<const phBoundComposite*>(newInstance->GetArchetype()->GetBound())->GetBound(transferringObjectComponent) != NULL, 
							"Transferring non-empty manifold to NULL component %i on %s.",transferringObjectComponent,newInstance->GetArchetype()->GetFilename());

						if(newRootManifold == NULL)
						{
							if(taskCollisionPairs.GetCapacity() == taskCollisionPairs.GetCount())
							{
								// If we're unable to allocate a new task collision pair just abort
								// NOTE: This shouldn't be necessary for simulator breaks since phContactMgr::BreakObjects should guarantee
								//         enough memory. We would need more scratch memory to ensure we always have enough room though. 
								//       Enough breaking outside the sim update could cause this although at that point the OPA has a capacity 
								//         of max manifolds (phSimulator::Init) so it's much less likely. 
								Warningf("Wanted to transfer contacts to a new manifold but there's no room for a new task collision pair.");
								curSubManifold->RemoveAllContacts();
								PHMANIFOLD->Release(curSubManifold);
								continue;
							}

							// We haven't set up a new root manifold yet, so that means this is the first component we've found to transfer.  First let's find
							//   the broadphase pair that corresponds to the newly added object and the object with which the new instance should be colliding.
							//   If it's not there and we don't have room to add a pair for it then we can't allocate the manifold (the broadphase pair 'owns'
							//   the manifold) and thus we can't transfer the contacts.
							u32 isNewLevelIndexGreaterMask = GenerateMaskGZ(((int)newHandle.GetLevelIndex()) - ((int)otherHandle.GetLevelIndex()));
							phHandle lesserLevelIndexHandle = phHandle(ISelectI(isNewLevelIndexGreaterMask,newHandle.GetLevelIndexAndGenerationId(),otherHandle.GetLevelIndexAndGenerationId()));
							phHandle greaterLevelIndexHandle = phHandle(ISelectI(isNewLevelIndexGreaterMask,otherHandle.GetLevelIndexAndGenerationId(),newHandle.GetLevelIndexAndGenerationId()));
							btBroadphasePair *bpPair = NULL;
							for(int bpPairIndex = numPreBreakingPairs; bpPairIndex < numInitialBPPairs; ++bpPairIndex)
							{
								btBroadphasePair *curBPPair = &broadPhasePairs[bpPairIndex];
								Assert(curBPPair->GetObject1() >= curBPPair->GetObject0());
								if(phHandle(curBPPair->GetObject0(),curBPPair->GetGenId0()) == lesserLevelIndexHandle && phHandle(curBPPair->GetObject1(),curBPPair->GetGenId1()) == greaterLevelIndexHandle)
								{
									Assertf(curBPPair->GetManifold() == NULL,"There is already a manifold between an object and the new object. ReplaceInstanceComponents shouldn't need to be called more than once.");
									bpPair = curBPPair;
									break;
								}
							}

							if(bpPair == NULL)
							{
								// For one reason or another there wasn't a broadphase pair added for this pair.  That could be because the delayed SAP add and
								//   remove option is being used, for example.
								if(broadPhase->IsFull())
								{
									// If there's no broadphase pair then we don't have anywhere to put the manifold so there's really nothing that we can do for
									//   this pair.
									Warningf("Wanted to transfer contacts to a new manifold but there's no room for a new broadphase pair.");
									curSubManifold->RemoveAllContacts();
									PHMANIFOLD->Release(curSubManifold);
									continue;
								}

								bpPair = broadPhase->addOverlappingPair(newHandle.GetLevelIndex(), otherHandle.GetLevelIndex());
								// We made sure that we had room before we added it, so it should not be possible for us to not have a broadphase pair now.
								Assert(bpPair != NULL);
							}

							newRootManifold = PHMANIFOLD->Allocate();
							if(newRootManifold == NULL)
							{
								Warningf("Wanted to transfer contacts to a new manifold but we're out of manifolds.");
								curSubManifold->RemoveAllContacts();
								PHMANIFOLD->Release(curSubManifold);
								continue;
							}

							Assert(!newRootManifold->CompositeManifoldsEnabled());
							newRootManifold->EnableCompositeManifolds();
							if(!newRootManifold->CompositeManifoldsEnabled())
							{
								Warningf("Wanted to transfer contacts to a new manifold but we're out of composite pointers objects.");
								PHMANIFOLD->Release(newRootManifold);
								curSubManifold->RemoveAllContacts();
								PHMANIFOLD->Release(curSubManifold);
								newRootManifold = NULL;
								continue;
							}

							// We got a new root manifold, let's initialize it.
							newRootManifold->SetHandle(newInstanceNewSlot,newHandle);
							newRootManifold->SetHandle(otherInstanceNewSlot,otherHandle);
							newRootManifold->SetCollider(newInstanceNewSlot, newCollider);
							newRootManifold->SetCollider(otherInstanceNewSlot, otherCollider);
							newRootManifold->SetInstance(newInstanceNewSlot, newInstance);
							newRootManifold->SetInstance(otherInstanceNewSlot, otherInstance);
							newRootManifold->SetComponent(0, 0);
							newRootManifold->SetComponent(1, 0);
	#if PH_MANIFOLD_TRACK_IS_IN_OPA
							newRootManifold->SetIsInOPA(true);
	#endif // PH_MANIFOLD_TRACK_IS_IN_OPA

							// The broadphase pair has already been set up, let's just tell it about its manifold.  This is important because the broadphase pair
							//   'owns' the manifold that it points to, and also because broadphase pairs are what are persistent from frame to frame.  Without the
							//   the broadphase pair, even if we set up the manifold and the task collision pair properly, the fact that these two objects are in
							//   contact would be lost next physics update.
							bpPair->SetManifold(newRootManifold);

							// Set up a new task collision pair so that collision between these objects gets processed this frame.
							phTaskCollisionPair &newTCPair = taskCollisionPairs.Append();
	//#if PHSIMULATOR_TEST_NEW_OBB_TEST
	//						newTCPair.pad[0] = wouldFailBoxTests;
	//#endif
							newTaskCollisionPair = &newTCPair;
							newTCPair.manifold = newRootManifold;
							sys_lwsync();
							curOverlappingPairArray->numCollisionPairs++;
						}

						// If we've gotten this far then we have a manifold that we need to transfer and we have the means to do it, so let's make it happen.
						Assert(newRootManifold != NULL);	// Should anything fail above we should have executed a continue and skipped past this.
						
						// Replace all the information pertaining to the old instance with the new instance's information
						curSubManifold->SetHandle(matchInstanceSlot,newHandle);
						curSubManifold->SetCollider(matchInstanceSlot,newCollider);
						curSubManifold->SetInstance(matchInstanceSlot,newInstance);
						curSubManifold->ResetWarmStart();

						// We might need to swap this manifold so that LevelIndexA < LevelIndexB
						if(swappingOtherSlot)
						{
							curSubManifold->ExchangeThisManifoldOnly();
						}

						newSubManifolds[numNewSubManifolds] = curSubManifold;
						newComponentPairs[2 * numNewSubManifolds + newInstanceNewSlot] = (u8)transferringObjectComponent;
						newComponentPairs[2 * numNewSubManifolds + otherInstanceNewSlot] = (u8)otherComponent;
						++numNewSubManifolds;
					}
					else
					{
						// If we got here this is a sub-manifold that is not affected by the change in instance.  We will just transfer it over.
						// We need to add in the slots since we aren't changing the order of the manifold if it's not moving to the new object.
						survivingOriginalSubManifolds[numSurvivingSubManifolds] = curSubManifold;
						survivingOriginalComponentPairs[2 * numSurvivingSubManifolds + matchInstanceSlot] = (u8)transferringObjectComponent;
						survivingOriginalComponentPairs[2 * numSurvivingSubManifolds + otherInstanceSlot] = (u8)otherComponent;

						// This assert isn't necessarily bad, it just means that we're leaving manifolds to NULL components on the original instance. 
						// If the user is aware of this and plans to get rid of the contacts some other way the assert could be removed. For now though it should never fire. 
						Assertf(curSubManifold->GetNumContacts() == 0 || static_cast<const phBoundComposite*>(instance->GetArchetype()->GetBound())->GetBound(transferringObjectComponent) != NULL, 
							"Leaving non-empty manifold of NULL component %i on %s.",transferringObjectComponent,instance->GetArchetype()->GetFilename());

						++numSurvivingSubManifolds;
					}
				}

				Assertf(numSurvivingSubManifolds + numNewSubManifolds == numSubManifolds, "Manifold transfer did not complete successfully.  See warning above for more information.");
				pairRootManifold->CopyCompositeArrays(numSurvivingSubManifolds, survivingOriginalComponentPairs, survivingOriginalSubManifolds);
				Assert(numNewSubManifolds == 0 || newRootManifold != NULL);
				if(newRootManifold != NULL)
				{
					newRootManifold->CopyCompositeArrays(numNewSubManifolds, newComponentPairs, newSubManifolds);
				}
			}
			else
			{
				// It's not a composite manifold.  Since ReplaceInstanceComponents() is being called, that should mean that at least one of the two objects
				//   is a composite, and generally that would mean that the only reason we'd get here would be if we had a constraint manifold.  It is, however,
				//   possible for a root manifold to have been allocated to a pair that has at least one composite object and for either that root manifold
				//   not have had composite manifolds enabled yet (because collision detection decided not to proceed, usually because at least one of the
				//   objects was 'degenerate' in some way) or for somebody to have cleared the root manifold.
				if(pairRootManifold->IsConstraint())
				{
					if((pairHandleA == originalHandle && componentBits.IsSet(pairRootManifold->GetComponentA())) || 
						(pairHandleB == originalHandle && componentBits.IsSet(pairRootManifold->GetComponentB())))
					{
						// Replace the original instances information on the manifold
						pairRootManifold->SetHandle(matchInstanceSlot,newHandle);
						pairRootManifold->SetCollider(matchInstanceSlot,newCollider);
						pairRootManifold->SetInstance(matchInstanceSlot,newInstance);
						
						// At this point pairRootManifold is the correct manifold between the other instance and the new instance
						// It might need to be swapped though, to ensure that LevelIndexA < LevelIndexB
						if(swappingOtherSlot)
						{
							pairRootManifold->ExchangeThisManifoldOnly();
						}

						newTaskCollisionPair = &curPair;
					}
				}
				else
				{
					Assert(pairRootManifold->GetNumContacts() == 0);
				}
			}

			// Ensure the new task collision pair is ordered properly
			if(newTaskCollisionPair)
			{
				// This should really just be done using the slot indices
				if(newInstanceNewSlot == 1)
				{
					newTaskCollisionPair->levelIndex2 = newHandle.GetLevelIndex();
					newTaskCollisionPair->generationId2 = newHandle.GetGenerationId();
					newTaskCollisionPair->levelIndex1 = otherHandle.GetLevelIndex();
					newTaskCollisionPair->generationId1 = otherHandle.GetGenerationId();
				}
				else
				{
					newTaskCollisionPair->levelIndex2 = otherHandle.GetLevelIndex();
					newTaskCollisionPair->generationId2 = otherHandle.GetGenerationId();
					newTaskCollisionPair->levelIndex1 = newHandle.GetLevelIndex();
					newTaskCollisionPair->generationId1 = newHandle.GetGenerationId();
				}
			}
		}
	}

	PHCONSTRAINT->ReplaceInstanceComponents(originalHandle.GetLevelIndex(), componentBits, newHandle.GetLevelIndex(), newHandle.GetGenerationId());
}


void phContactMgr::ApplyPostBreakingImpulses (phInst& newlyBrokenInstance, bool wasOriginalInstanceFixed, ScalarV_In impulseScale)
{
	// Add in the global impulse scale
	const ScalarV globalImpulseScale = ScalarVFromF32(sm_BreakImpulseScale);
	if(IsEqualAll(globalImpulseScale,ScalarV(V_ZERO)))
	{
		return;
	}
	const ScalarV totalImpulseScale = Scale(impulseScale,globalImpulseScale);	
	Assertf(IsLessThanOrEqualAll(totalImpulseScale,ScalarV(V_ONE)) && IsGreaterThanOrEqualAll(totalImpulseScale,ScalarV(V_ZERO)),"Invalid post break impulse scale (%f).", totalImpulseScale.Getf());

	const ScalarV cgOffsetScale = ScalarVFromF32(sm_BreakImpulseCGOffset);

	const u16 newLevelIndex = newlyBrokenInstance.GetLevelIndex();
	Assert(newLevelIndex != phInst::INVALID_INDEX);
	const phHandle newHandle = phHandle(newLevelIndex,PHLEVEL->GetGenerationID(newLevelIndex));

	// Loop over all the breaking pairs
	atArray<phTaskCollisionPair, 0, u32> &taskCollisionPairs = m_BreakingPairs->pairs;
	int numTCPairs = taskCollisionPairs.GetCount();
	for(int tcPairIndex = m_FirstPairInIsland; tcPairIndex < numTCPairs; ++tcPairIndex)
	{
		phTaskCollisionPair &curPair = taskCollisionPairs[tcPairIndex];
		phManifold& manifold = *curPair.manifold;
		FastAssert(curPair.manifold);
		FastAssert(manifold.GetHandleA() == phHandle(curPair.levelIndex1,curPair.generationId1));
		FastAssert(manifold.GetHandleB() == phHandle(curPair.levelIndex2,curPair.generationId2));

		// Determine if this manifold contains the newly broken instance
		const size_t matchesNewHandleAMask = GenerateMaskEq(phHandle(curPair.levelIndex1,curPair.generationId1), newHandle);
		const size_t matchesNewHandleBMask = GenerateMaskEq(phHandle(curPair.levelIndex2,curPair.generationId2), newHandle);

		// No self collisions or constraints. We could probably support them but it isn't necessary at this point. 
		if((matchesNewHandleBMask ^ matchesNewHandleAMask) != 0 && !manifold.IsConstraint())
		{
			const int otherInstanceSlot = (matchesNewHandleAMask & 1);
			// If the other object doesn't have a collider then it shouldn't have contributed to the breaking of the new instance
			phCollider* otherCollider = manifold.GetCollider(otherInstanceSlot);
			if(otherCollider)
			{
				bool isArticulatedFixedCollision = wasOriginalInstanceFixed && otherCollider->IsArticulated();

				// Make sure the impulse points into the other object
				const ScalarV signedImpulseScale = (otherInstanceSlot==0) ? totalImpulseScale : Negate(totalImpulseScale);

				if(manifold.CompositeManifoldsEnabled())
				{
					int numCompositeManifolds = manifold.GetNumCompositeManifolds();
					for(int compositeManifoldIndex = 0; compositeManifoldIndex < numCompositeManifolds; ++compositeManifoldIndex)
					{
						ApplyPostBreakingImpulsesOnManifold(*manifold.GetCompositeManifold(compositeManifoldIndex),otherInstanceSlot,isArticulatedFixedCollision,*otherCollider,cgOffsetScale,signedImpulseScale);
					}
				}
				else
				{
					ApplyPostBreakingImpulsesOnManifold(manifold,otherInstanceSlot,isArticulatedFixedCollision,*otherCollider,cgOffsetScale,signedImpulseScale);
				}
			}
		}
	}
}

void phContactMgr::ApplyPostBreakingImpulsesOnManifold(phManifold& breakingManifold, int otherObjectSlot, bool isArticulatedFixedCollision, phCollider& otherCollider, ScalarV_In cgOffsetScale, ScalarV_In signedImpulseScale)
{
	Assert(breakingManifold.GetCollider(otherObjectSlot) == &otherCollider);
	const int otherComponent = breakingManifold.GetComponent(otherObjectSlot);
	const ScalarV otherMassInvScale = ScalarVFromF32(breakingManifold.GetMassInvScale(otherObjectSlot));

	// Find the CG of the other collider/articulated part on this manifold
	Vec3V otherCenterOfGravity = otherCollider.GetPosition();
	if(otherCollider.IsArticulated())
	{
		phArticulatedCollider& otherArticulatedCollider = static_cast<phArticulatedCollider&>(otherCollider);
		otherCenterOfGravity = Add(otherCenterOfGravity, otherArticulatedCollider.GetBody()->GetLink(otherArticulatedCollider.GetLinkFromComponent(otherComponent)).GetPositionV());
	}
	
	int numContacts = breakingManifold.GetNumContacts();
	for(int contactIndex = 0; contactIndex < numContacts; ++contactIndex)
	{
		const phContact& contact = breakingManifold.GetContactPoint(contactIndex);
		if(contact.IsContactActive() && contact.IsPositiveDepth())
		{

			// Find a point between the contact position and the CG to apply the impulse
			const Vec3V contactPosition = contact.GetWorldPos(otherObjectSlot);
			const Vec3V impulsePosition = Lerp(cgOffsetScale,otherCenterOfGravity,contactPosition);

			// Compute the impulse we're applying
			// The impulse is whatever the collider would have received had the object not broken, scaled by the fraction (minimum impulse to break)/(applied impulse)
			// The unsigned scale should be between 0 and 1. If it was greater than one it means the minimum impulse to break is  greater than the applied impulse so there
			//   shouldn't have been a break in the first place. 
			const Vec3V contactImpulse = isArticulatedFixedCollision ? contact.ComputeTotalArtFixImpulse() : contact.ComputeTotalImpulse();
			const Vec3V applicableContactImpulse = Scale(contactImpulse,otherMassInvScale);
			const Vec3V breakingImpulse = Scale(applicableContactImpulse,signedImpulseScale);

			otherCollider.ApplyImpulse(breakingImpulse.GetIntrin128(),impulsePosition.GetIntrin128(),otherComponent);
		}
	}
}

class ManifoldSortPredicateA : public std::binary_function<const phManifold*, const phManifold*, bool>
{
public:
	bool operator()(const phManifold* left, const phManifold* right) const
	{
		return left->GetLevelIndexA() < right->GetLevelIndexA();
	}
};

class ManifoldSortPredicateB : public std::binary_function<const phManifold*, const phManifold*, bool>
{
public:
	bool operator()(const phManifold* left, const phManifold* right) const
	{
		return left->GetLevelIndexB() < right->GetLevelIndexB();
	}
};

void phContactMgr::DoPreComputeImpacts(phOverlappingPairArray* pairArray)
{
	PF_FUNC(PreComputeImpacts);

	const phOverlappingPairArray::PairArray& pairs = pairArray->pairs;
	u16 numPairs = (u16)pairs.GetCount();

	// index lists are like linked list of indices, hooking up all the pairs with the same instance A/B
	u16* indexListA = Alloca(u16, numPairs);
	u16* indexListB = Alloca(u16, numPairs);

	// first manifold arrays point to the first pair in the index array for a particular level index, basically the head pointers
	u16 maxLevelIndex = (u16)PHLEVEL->GetCurrentMaxLevelIndex() + 1;
	u16* firstManifoldsByObjectA = Alloca(u16, maxLevelIndex);
	sysMemSet(firstManifoldsByObjectA, 0xff, maxLevelIndex * sizeof(u16));
	u16* firstManifoldsByObjectB = Alloca(u16, maxLevelIndex);
	sysMemSet(firstManifoldsByObjectB, 0xff, maxLevelIndex * sizeof(u16));

	u16* uniqueLevelIndices = Alloca(u16, maxLevelIndex);
	u16 numUniqueLevelIndices = 0;

	// First build up the index lists
	for (u16 pair = 0; pair < numPairs; ++pair)
	{
		if (Likely(pair + 1 < numPairs))
		{
			if (phManifold* nextManifold = pairs[pair + 1].manifold)
			{
				PrefetchObject(nextManifold);
			}
		}

		if (phManifold* manifold = pairs[pair].manifold)
		{
			if (manifold->ObjectsInContact())
			{
				u16 levelIndexA = (u16)manifold->GetLevelIndexA();
				if (levelIndexA != (u16)BAD_INDEX)
				{
					TrapGE(levelIndexA, maxLevelIndex);
					u16 previousFirstA = firstManifoldsByObjectA[levelIndexA];
					firstManifoldsByObjectA[levelIndexA] = (u16)pair;
					indexListA[pair] = (u16)previousFirstA;

					if (previousFirstA == 0xffff && firstManifoldsByObjectB[levelIndexA] == 0xffff)
					{
						// First time we encountered this object, put it into our list of unique indices
						TrapGE(numUniqueLevelIndices, maxLevelIndex);
						uniqueLevelIndices[numUniqueLevelIndices++] = levelIndexA;
					}
				}

				u16 levelIndexB = (u16)manifold->GetLevelIndexB();
				if (levelIndexA != levelIndexB && levelIndexB != (u16)BAD_INDEX)
				{
					TrapGE(levelIndexB, maxLevelIndex);
					u16 previousFirstB = firstManifoldsByObjectB[levelIndexB];
					firstManifoldsByObjectB[levelIndexB] = (u16)pair;
					indexListB[pair] = (u16)previousFirstB;

					if (previousFirstB == 0xffff && firstManifoldsByObjectA[levelIndexB] == 0xffff)
					{
						// First time we encountered this object, put it into our list of unique indices
						TrapGE(numUniqueLevelIndices, maxLevelIndex);
						uniqueLevelIndices[numUniqueLevelIndices++] = levelIndexB;
					}
				}
			}
		}
	}
		
	const u32 MAX_NUM_MANIFOLDS_PER_INSTANCE = 1024;
	phManifold** sortedManifoldsA = Alloca(phManifold*, MAX_NUM_MANIFOLDS_PER_INSTANCE);
	phManifold** sortedManifoldsB = Alloca(phManifold*, MAX_NUM_MANIFOLDS_PER_INSTANCE);

	// Then go through and collect the pairs for each instance and call the callback
	for (int uniqueIndex = 0; uniqueIndex < numUniqueLevelIndices; ++uniqueIndex)
	{
		u16 levelIndex = uniqueLevelIndices[uniqueIndex];
		phInst* inst = PHLEVEL->GetInstance(levelIndex);

		u16 pairA = firstManifoldsByObjectA[levelIndex];
		u32 numSortedA = 0;
		while (pairA != 0xffff)
		{
			if (Likely(numSortedA < MAX_NUM_MANIFOLDS_PER_INSTANCE))
			{
				sortedManifoldsA[numSortedA] = pairs[pairA].manifold;
			}
			numSortedA++;
			TrapGE(pairA, numPairs);
			pairA = indexListA[pairA];
		}

		Assertf(numSortedA < MAX_NUM_MANIFOLDS_PER_INSTANCE, "Exceeded MAX_NUM_MANIFOLDS_PER_INSTANCE (%d), the %s is involved in %d manifolds.",
			MAX_NUM_MANIFOLDS_PER_INSTANCE,
			inst->GetArchetype()->GetFilename(),
			numSortedA);

		numSortedA = Min(numSortedA, MAX_NUM_MANIFOLDS_PER_INSTANCE);

		u16 pairB = firstManifoldsByObjectB[levelIndex];
		u32 numSortedB = 0;
		while (pairB != 0xffff)
		{
			if (Likely(numSortedB < MAX_NUM_MANIFOLDS_PER_INSTANCE))
			{
				sortedManifoldsB[numSortedB++] = pairs[pairB].manifold;
			}
			TrapGE(pairB, numPairs);
			pairB = indexListB[pairB];
		}

		Assertf(numSortedB < MAX_NUM_MANIFOLDS_PER_INSTANCE, "Exceeded MAX_NUM_MANIFOLDS_PER_INSTANCE (%d), the %s is involved in %d manifolds.",
			MAX_NUM_MANIFOLDS_PER_INSTANCE,
			inst->GetArchetype()->GetFilename(),
			numSortedB);

		numSortedB = Min(numSortedB, MAX_NUM_MANIFOLDS_PER_INSTANCE);

		// Get this up here so we can do the other calculations while the likely cache miss resolves
		if (Verifyf(inst, "NULL inst in DoPreComputeImpacts") && numSortedA + numSortedB > 0)
		{
			phContactIterator it(sortedManifoldsA, numSortedA, sortedManifoldsB, numSortedB, inst);

			if (!it.AtEnd())
			{
				if(phInstBehavior* instBehavior = PHSIM->GetInstBehavior(levelIndex))
				{
					instBehavior->PreComputeImpacts(it);
				}

				inst->PreComputeImpacts(it);
			}
		}
	}
}


size_t phContactMgr::GetHalfScratchpadSize()
{
	return m_ScratchPadSize >> 1;
}

void phContactMgr::InitNextHalfScratchpad()
{
	m_WhichScratchpadHalf = 1 - m_WhichScratchpadHalf;

	int halfSize = m_ScratchPadSize >> 1;
	m_ScratchpadCurrentTop = m_Scratchpad + m_WhichScratchpadHalf * halfSize;
	m_ScratchpadCurrentBottom = m_ScratchpadCurrentTop + halfSize;
}

void phContactMgr::StartHalfScratchpad()
{
	sysMemStartMiniHeap(m_ScratchpadCurrentTop, m_ScratchpadCurrentBottom - m_ScratchpadCurrentTop);
}

void phContactMgr::EndHalfScratchpad()
{
	size_t scratchLeft = sysMemEndMiniHeap();
	m_ScratchpadCurrentTop = (u8*)((size_t(m_ScratchpadCurrentBottom - scratchLeft) + 15) & ~15);

	// Double since this is only half of the scratchpad
	u8* maxTop = m_Scratchpad + m_WhichScratchpadHalf * (m_ScratchPadSize >> 1);
	u32 scratchpadUsed = (u32)(2 * (m_ScratchpadCurrentTop - maxTop));

	MarkScratchpadUsed(scratchpadUsed);
}

void phContactMgr::MarkScratchpadUsed(u32 used)
{
	m_ScratchPadMaxUsed = Max(used, m_ScratchPadMaxUsed);
	m_ScratchPadMaxUsedThisFrame = Max(used, m_ScratchPadMaxUsedThisFrame);

	PF_SET(MaxMemoryUsed, m_ScratchPadMaxUsed);
	PF_SET(MemoryUsed, m_ScratchPadMaxUsedThisFrame);
}

}	// namespace rage
