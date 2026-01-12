//
// physics/simulator.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#if __PS3
#include <cell/spurs/queue.h>
#endif

#include "simulator.h"

#include "btAxisSweep3.h"
#include "broadphase.h"
#include "colliderdispatch.h"
#include "collidertask.h"
#include "collision.h"
#include "collisionoverlaptest.h"
#include "debugcontext.h"
#include "debugphysics.h"
#include "instbehavior.h"
#include "instbreakable.h"
#include "iterator.h"
#include "levelnew.h"
#include "manifold.h"
#include "manifoldresult.h"
#include "overlappingpairarray.h"
#include "sleep.h"
#include "sleepmgr.h"
#include "spuevents.h"

#include "bank/bank.h"
#include "phbound/bound.h"
#include "phbound/boundbvh.h"
#include "phbound/boundcomposite.h"
#include "phbound/boundpolyhedron.h"
#include "phbound/OptimizedBvh.h"
#include "phbullet/boxBoxDistance.h"
#if (__WIN32 || ENABLE_UNUSED_PHYSICS_CODE)
#include "phbullet/btStackAlloc.h"
#endif // __WIN32 || ENABLE_UNUSED_PHYSICS_CODE
#include "phbullet/CollisionWorkUnit.h"
#include "phcore/avl_tree.h"
#include "phcore/constants.h"
#include "phcore/LinearMemoryAllocator.h"
#include "phcore/phmath.h"
#include "phcore/pool.h"
#include "phcore/workerthread.h"
#include "phcore/sputaskset.h"
#include "phsolver/contactmgr.h"
#include "grprofile/drawmanager.h"
#include "profile/profiler.h"
#include "profile/timebars.h"
#include "system/param.h"
#include "grprofile/pix.h"
#include "system/memmanager.h"
#include "system/task.h"
#include "vector/colors.h"
#include "vector/colorvector3.h"
#include "vector/geometry.h"
#include "vector/matrix34.h"
#include "vector/matrix44.h"
#include "vector/matrix33.h"
#include "vectormath/classes.h"
#include "CollisionMemory.h"
#include "phcore/bitvector.h"
#include "physics/physicsprofilecapture.h"

#if RSG_ORBIS
#include <kernel.h>
#endif

#define INCLUDE_DETAIL_PHYS_SIM_TIMEBARS 0

#if RAGE_TIMEBARS
#if INCLUDE_DETAIL_PHYS_SIM_TIMEBARS || INCLUDE_DETAIL_TIMEBARS
#define PF_PUSH_TIMEBAR_PHYS_SIM_DETAIL(name)				PF_PUSH_TIMEBAR_DETAIL(name)
#define PF_START_TIMEBAR_PHYS_SIM_DETAIL(name)				PF_START_TIMEBAR_DETAIL(name)
#define PF_POP_TIMEBAR_PHYS_SIM_DETAIL()					PF_POP_TIMEBAR_DETAIL(1)
#define PF_AUTO_PUSH_PHYS_SIM_DETAIL(name)					::rage::pfAutoPushTimer localPushTimer(name,true)
#else // INCLUDE_DETAIL_PHYS_SIM_TIMEBARS
#define PF_PUSH_TIMEBAR_PHYS_SIM_DETAIL(name)
#define PF_START_TIMEBAR_PHYS_SIM_DETAIL(name)
#define PF_POP_TIMEBAR_PHYS_SIM_DETAIL()
#define PF_AUTO_PUSH_PHYS_SIM_DETAIL(name)
#endif // INCLUDE_DETAIL_PHYS_SIM_TIMEBARS
#else // RAGE_TIMEBARS
#define PF_PUSH_TIMEBAR_PHYS_SIM_DETAIL(name)
#define PF_START_TIMEBAR_PHYS_SIM_DETAIL(name)
#define PF_POP_TIMEBAR_PHYS_SIM_DETAIL()
#define PF_AUTO_PUSH_PHYS_SIM_DETAIL(name)
#endif // RAGE_TIMEBARS

GJK_COLLISION_OPTIMIZE_OFF()

#define COLLISION_SWEEP_DRAW (__BANK && EARLY_FORCE_SOLVE)

#if COLLISION_SWEEP_DRAW
#define COLLISION_SWEEP_DRAW_ONLY(X) X

#include "phbullet/SimdTransformUtil.h"
#include "system/timemgr.h"
#else
#define COLLISION_SWEEP_DRAW_ONLY(X)
#endif

#if __BANK
#define NON_BANK_ONLY(X)
#else	// __BANK
#define NON_BANK_ONLY(X)	X
#endif	// __BANK

#if __PPU
#include <sys/spu_initialize.h>
#endif // __PPU

#define PHCOLLISION_USE_TASKS	(!__PS3 && !TEST_REPLAY)

// Set this to 1 to enable the new OBB-vs-OBB test in ProcessOverlaps.
#define PHSIMULATOR_USE_OBB_TEST_IN_PROCESSOVERLAPS	1

// Set this to 1 to make the OBB-vs-OBB test not actually reject pairs but instead record whether or not it would have rejected the pair and then
//   check that after collision detection against whether or not any contacts were found.  It stores this information in pad[0] of the phTaskCollisionPair.
#define PHSIMULATOR_TEST_NEW_OBB_TEST				(0 && PHSIMULATOR_USE_OBB_TEST_IN_PROCESSOVERLAPS && __ASSERT)

#define TRACE_COLLIDE 0

#if TRACE_COLLIDE && __XENON && !__FINAL
#include "system/timemgr.h"
#include "system/xtl.h"
#include <tracerecording.h>
#pragma comment (lib, "tracerecording.lib")
#endif

PHYSICS_OPTIMIZATIONS

#define DELAYED_SAP_MAX 512

#if __PPU
bool g_BulletSpinWait = false;
#endif // __PPU


#if PHCOLLISION_USE_TASKS

#include "system/spinlock.h"
#include "system/task.h"
#define PHCOLLISION_USE_TASKS_ONLY(X) X
#else
#define PHCOLLISION_USE_TASKS_ONLY(X)
#endif

bool g_debugBool1 = false;
bool g_debugBool2 = false;
bool g_debugBool3 = false;
bool g_debugBool4 = false;
bool g_debugBool5 = false;
bool g_debugBool6 = true;

RELOAD_TASK_DECLARE(shapetestspu);
RELOAD_TASK_DECLARE(shapetestcapsulespu);
RELOAD_TASK_DECLARE(shapetestspherespu);
RELOAD_TASK_DECLARE(shapetestsweptspherespu);
RELOAD_TASK_DECLARE(shapetesttaperedsweptspherespu);

#if __PS3
extern char SPURS_TASK_START(collision)[];
extern char SPURS_TASK_START(updatecolliders)[];
#if USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
extern char SPURS_TASK_START(commitdeferredoctreeupdates)[];
#endif	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
#endif

namespace rage {

PARAM(nomtphys, "[physics] Don't use multithreaded physics simulation.");
PARAM(mtphys, "[physics] Use multithreaded physics simulation, even though this build is not optimized.");

PARAM(maxmanagedcolliders, "[physics] Override the maximum number of managed colliders.");
PARAM(scratchpadsizekb, "[physics] Override the scratchpad size of physics.");
PARAM(maxmanifolds, "[physics] Override the maximum number of manifolds.");
PARAM(maxhighprioritymanifolds, "[physics] Override the maximum number of high-priority manifolds.");
PARAM(maxcompositemanifolds, "[physics] Override the maximum number of composite manifolds.");
PARAM(maxcontacts, "[physics] Override the maximum number of contacts."); 
PARAM(maxinstbehaviors, "[physics] Override the maximum number of inst behaviors.");
PARAM(maxexternvelmanifolds, "[physics] Override the maximum number of extern velocity manifolds.");
PARAM(maxconstraints, "[physics] Override the maximum number of constraints.");
PARAM(maxbigconstraints, "[physics] Override the maximum number of big constraints.");
PARAM(maxlittleconstraints, "[physics] Override the maximmum number of little constraints.");
PARAM(sleepbuffersizekb, "[physics] Override the sleep buffer size.");
PARAM(maxsleepislands, "[physics] Override the maximum number of sleep islands.");

int g_NumAutoCCDCollisions = 0;

#if __WIN32
__THREAD btStackAlloc* g_PerThreadStackAlloc = NULL;
ASSERT_ONLY(static bool s_PerThreadStackAllocCreated = false;)
static int s_PerThreadStackAllocCount = 0;
static btStackAlloc** s_PerThreadStackAllocTable;
#endif // WIN32

static sysCriticalSectionToken s_SortInstanceBehaviorsToken;
static sysCriticalSectionToken s_DelayedSAPAddRemoveToken;

static const int DEFAULT_NUM_COLLISIONS_PER_TASK = 1;
#if __BANK
static const int MAX_NUM_COLLISIONS_PER_TASK = 64;
#endif

#if PHCOLLISION_USE_TASKS
DECLARE_TASK_INTERFACE(ProcessPairListTask);
#endif

EXT_PFD_DECLARE_ITEM(BroadphasePairs);
EXT_PFD_DECLARE_ITEM(NoContact);
EXT_PFD_DECLARE_ITEM(PrePush);
EXT_PFD_DECLARE_GROUP(Manifolds);
EXT_PFD_DECLARE_ITEM(Islands);

// Set the frame rates to use for testing whether a force can break an object. Object breaking is done through
// impulses, so breaking with forces would depend on the frame rate if the actual frame time was used.
#define DEFAULT_FRAME_RATE	30.0f
#define DEFAULT_FRAME_TIME	1.0f/DEFAULT_FRAME_RATE


////////////////////////////////////////////////////////////////
// profiling variables

namespace SimulatorStats
{
	PF_PAGE(SimPage,"ph Simulator");

	PF_GROUP(Simulator);
	PF_LINK(SimPage,Simulator);
	PF_TIMER(SimUpdate,Simulator);
	PF_TIMER(CoreUpdate, Simulator);
	PF_TIMER(CollideActive, Simulator);
	PF_TIMER(CollideActiveDOU, Simulator);
	PF_TIMER(PreCollide,Simulator);
	PF_TIMER_OFF(ColliderUpdateThread,Simulator);
	PF_TIMER(Collide,Simulator);
	PF_TIMER_OFF(CollideThread,Simulator);
	PF_TIMER(CollideJobs,Simulator);
	PF_TIMER(ProcessOverlap,Simulator);
	PF_TIMER(SortPairsByCost,Simulator);
	PF_TIMER(ExtraWork,Simulator);
	PF_TIMER(IslandBuilding,Simulator);
	PF_TIMER(PostCollide,Simulator);
	PF_TIMER_OFF(PostCollideThread,Simulator);
	PF_TIMER(ShouldFindImpacts, Simulator);
	PF_TIMER(PreComputeImpacts,Simulator);
	PF_TIMER(SortInstanceBehaviors,Simulator);
	PF_TIMER(SplitPairs,Simulator);
	PF_TIMER(ContactMgrUpdateContacts,Simulator);
#if __PS3
	PF_TIMER(ManifoldPlans,Simulator);
#endif
	PF_TIMER(SecondBallisticUpdate,Simulator);
	PF_TIMER_OFF(RefreshContactsThread,Simulator);
	PF_TIMER(CollisionActivations,Simulator);

	PF_GROUP(PreCollide);
	PF_LINK(SimPage,PreCollide);
	PF_TIMER(UpdateColliders,PreCollide);
	PF_TIMER(UpdateSleep,PreCollide);

	PF_GROUP(Collide);
	PF_LINK(SimPage,Collide);
	PF_TIMER(Cull,Collide);
	PF_TIMER(TestboundActive,Collide);
	PF_TIMER(TestboundInactive,Collide);
	PF_TIMER(TestboundFixed,Collide);
	PF_TIMER_OFF(Cull_MI,Collide);
	PF_TIMER_OFF(TestboundActive_MI,Collide);
	PF_TIMER_OFF(TestboundInactive_MI,Collide);
	PF_TIMER_OFF(Cull_AI,Collide);
	PF_TIMER_OFF(TestboundActive_AI,Collide);
	PF_TIMER_OFF(TestboundInactive_AI,Collide);
	PF_TIMER_OFF(TestboundFixed_AI,Collide);

	PF_GROUP(PostCollide);
	PF_LINK(SimPage,PostCollide);
	PF_TIMER(ForceSolver,PostCollide);
	PF_TIMER(UpdateCollidersPost,PostCollide);
	PF_TIMER(PostCollideActives,PostCollide);

	PF_GROUP(AddAndDeleteObjects);
	PF_LINK(SimPage,AddAndDeleteObjects);
	PF_TIMER(AddActiveObject,AddAndDeleteObjects);
	PF_TIMER(AddInactiveObject,AddAndDeleteObjects);
	PF_TIMER(AddFixedObject,AddAndDeleteObjects);
	PF_TIMER(DeleteObject,AddAndDeleteObjects);
	PF_TIMER(DeleteObjectSearch,AddAndDeleteObjects);
	PF_TIMER(CommitDelayedObjectsSAP,AddAndDeleteObjects);

#if EARLY_FORCE_SOLVE
	PF_GROUP(EarlyForceSolve);
	PF_LINK(SimPage, EarlyForceSolve);
	PF_TIMER(RefreshInstAndColliderPointers, EarlyForceSolve);
	PF_TIMER(IntegrateVelocities,EarlyForceSolve);
	PF_TIMER(VelocitySolve,EarlyForceSolve);
	PF_TIMER(IntegratePositions,EarlyForceSolve);
	PF_TIMER(IntegratePositionsCallbacks,EarlyForceSolve);
	PF_TIMER(PushSolve,EarlyForceSolve);
	PF_TIMER(PushCollision,EarlyForceSolve);
	PF_TIMER(ApplyPushes,EarlyForceSolve);
	PF_TIMER(ApplyPushesJobs,EarlyForceSolve);
	PF_TIMER(ApplyPushesCallbacks,EarlyForceSolve);
	PF_TIMER(ApplyPushesCommit,EarlyForceSolve);
	PF_TIMER(FindPushPairs,EarlyForceSolve);
#endif // EARLY_FORCE_SOLVE

	PF_PAGE(SimCountersPage,"ph Simulator Counters");

	PF_GROUP(SimCounters);
	PF_LINK(SimCountersPage, SimCounters);
	PF_COUNTER(BroadphasePairs, SimCounters);
	PF_COUNTER(OPAPairs, SimCounters);
	PF_COUNTER(BPRejectArrayFull, SimCounters);
	PF_COUNTER(BPRejectNonExist, SimCounters);
	PF_COUNTER(BPRejectState, SimCounters);
	PF_COUNTER(BPRejectFlags, SimCounters);
	PF_COUNTER(BPRejectBoxTest, SimCounters);
	PF_COUNTER(BPRejectSFImpacts, SimCounters);
	PF_COUNTER(BPRejectCollideObjects, SimCounters);
	PF_COUNTER(BPAccept, SimCounters);
};

using namespace SimulatorStats;

namespace phCollisionStats
{
    EXT_PF_TIMER(SimCollide);
    EXT_PF_TIMER(ManifoldMaintenance);
    EXT_PF_TIMER(TriangleConvex);
    EXT_PF_TIMER(ContactMaintenance);
    EXT_PF_TIMER(MidNarrowPhase);
    EXT_PF_TIMER(OctreeMidphase);
    EXT_PF_TIMER(PairwiseBoxTests);
    EXT_PF_TIMER(PairwiseCollisions);
    EXT_PF_TIMER(ConcaveConvex);
    EXT_PF_TIMER(IslandMaintenance);
};

using namespace phCollisionStats;

namespace BroadphaseCollisionStats
{
	EXT_PF_TIMER(PruneBroadphase);
};

using namespace BroadphaseCollisionStats;

namespace phPoolStats
{
    PF_PAGE(PHPools,"ph Pools");

    PF_GROUP(Pools);
    PF_LINK(PHPools,Pools);

	PF_VALUE_INT(Manifolds,Pools);
	PF_VALUE_INT(Contacts,Pools);
	PF_VALUE_INT(CompositePointers,Pools);
};

using namespace phPoolStats;

namespace ObjectStats
{
	PF_PAGE(ObjPage,"ph Objects");

	PF_GROUP(Objects);
	PF_LINK(ObjPage,Objects);
	PF_VALUE_INT_OFF(TotalObjects,Objects);
	PF_VALUE_INT(ActiveObjects,Objects);
	PF_VALUE_INT_OFF(InactiveObjects,Objects);
	PF_VALUE_INT_OFF(FixedObjects,Objects);
	PF_VALUE_INT(ActiveArticulated,Objects);
	PF_VALUE_INT_OFF(InstBehaviors,Objects);
	PF_VALUE_INT_OFF(MaxLevelIndex,Objects);
};

using namespace ObjectStats;

namespace phCCDStats
{
	EXT_PF_VALUE_INT(TotalCCD_Calls);
	EXT_PF_VALUE_INT(TotalCCD_Iters);
	EXT_PF_VALUE_FLOAT(AvgCCD_Iters);
	EXT_PF_VALUE_INT(PeakCCD_Iters);
	EXT_PF_VALUE_INT(MinCCD_Iters);
};
using namespace phCCDStats;

#if EARLY_FORCE_SOLVE
namespace PushCollisionTimersStats
{
	PF_PAGE(PushCollisionTimersPage,"ph Push Collision Timers");

	PF_GROUP(PushCollisionTimersGroup);
	PF_LINK(PushCollisionTimersPage,PushCollisionTimersGroup);
	PF_TIMER(PCCollisions,PushCollisionTimersGroup);
	PF_TIMER(PCPushSolve,PushCollisionTimersGroup);
	PF_TIMER(PCApplyPushes,PushCollisionTimersGroup);
	PF_TIMER(PCFindPairs,PushCollisionTimersGroup);
};

using namespace PushCollisionTimersStats;
#endif // EARLY_FORCE_SOLVE_ONLY

////////////////////////////////////////////////////////////////

#define MAX_MOVING_INSTANCES 120
#define MAX_ACTIVE_INSTANCES 120

// statics
phSimulator * phSimulator::sm_ActiveInstance;
#if __DEV
const int phSimulator::sm_DebugLevel = 0;
#endif

EARLY_FORCE_SOLVE_ONLY(static bool s_PushCollision;)

////////////////////////////////////////////////////////////////
// useful macros

#if __DEV
#define VALIDATE_PHLEVEL(x)											if (DebugLevel>x) {m_Level->Validate();}
#else
#define VALIDATE_PHLEVEL(x)
#endif

////////////////////////////////////////////////////////////////

Vector3 phSimulator::sm_Gravity = Vector3(0.0f,GRAVITY,0.0f);
#if USER_JBN
bool phSimulator::sm_SleepEnabled = false;
#else
bool phSimulator::sm_SleepEnabled = true;
#endif
bool phSimulator::sm_EnablePushes = true;
bool phSimulator::sm_AlwaysPush = false;
bool phSimulator::sm_ValidateArchetypes = false;
bool g_UseBackfaceCulling = EARLY_FORCE_SOLVE;
bool g_UseNormalFiltering = true;
bool g_UseNormalFilteringAlways = true;
bool g_UseNormalClamping = true;
bool g_MaintainLooseOctree = true;
extern int g_MaxContinuousIterations;
#if __BANK
#if USE_BOX_BOX_DISTANCE
extern bool g_UseBoxBoxDistance;
#endif	// USE_BOX_BOX_DISTANCE
#if CAPSULE_TO_CAPSULE_DISTANCE
extern bool g_CapsuleCapsuleDistance;
#endif	// CAPSULE_TO_CAPSULE_DISTANCE
#if CAPSULE_TO_TRIANGLE_DISTANCE
extern bool g_CapsuleTriangleDistance;
#endif	// CAPSULE_TO_TRIANGLE_DISTANCE
#if DISC_TO_TRIANGLE_DISTANCE
extern bool g_DiscTriangleDistance;
#endif	// DISC_TO_TRIANGLE_DISTANCE
#if BOX_TO_TRIANGLE_DISTANCE
extern bool g_BoxTriangleDistance;
#endif	// BOX_TO_TRIANGLE_DISTANCE
#if USE_GJK_CACHE
extern bool g_UseFramePersistentGJKCache;
extern bool g_UseGJKCache;
#endif
#if USE_NEW_MID_PHASE && ALLOW_MID_PHASE_SWAP
extern bool g_UseNewMidPhaseCollision;
#endif
#endif	// __BANK
int phSimulator::sm_ColliderUpdateIndex = phInst::INVALID_INDEX;
bool phSimulator::sm_ColliderUpdateEnabled = true;
bool phSimulator::sm_ComputeForces = true;
#if __BANK
bool phSimulator::sm_UseOctreeUpdateTask = true;
#endif	// __BANK
bool phSimulator::sm_ShouldFindImpacts = true;
bool phSimulator::sm_PreComputeImpacts = true;
bool phSimulator::sm_ReportMovedBySim = true;
bool phSimulator::sm_CheckAllPairs = false;
bool phSimulator::sm_ManifoldBoxTest = true;
#if USE_DETERMINISTIC_ORDERING
bool phSimulator::sm_SortPairsByCost = false;
#else
bool phSimulator::sm_SortPairsByCost = true;
#endif
bool phSimulator::sm_delaySAPAddRemove = false;
bool g_SweepFromSafe = true;

#if PROPHYLACTIC_SWAPS
bool g_ProphylacticSwaps = true;
#endif

Vector3 phSimulator::sm_MinConcaveThickness = Vector3(0.1f, 0.1f, 0.1f);
bool phSimulator::sm_SelfCollisionsEnabled = true;
bool phSimulator::sm_InitClassCalled = false;

#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
#if __BANK
bool phSimulator::sm_GetInstBehaviorFromArray = true;
#endif	// __BANK
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY

int phSimulator::sm_NumCollisionsPerTask = DEFAULT_NUM_COLLISIONS_PER_TASK;

#if __BANK
int phSimulator::sm_MinManifoldPointLifetime = PHSIM_DEFAULT_MIN_POINT_LIFETIME;
#endif
bool phSimulator::sm_CompositePartSphereTest = false;
bool phSimulator::sm_CompositePartOBBTest = false;
bool phSimulator::sm_CompositePartAABBTest = true;
int phSimulator::sm_ConcavePenetration = phSimulator::Penetration_Triangle;
int phSimulator::sm_ConvexPenetration = phSimulator::Penetration_Minkowski;
float phSimulator::sm_AllowedPenetration = 0.007f;
float phSimulator::s_AllowedAnglePenetration = 0.03f;
bool phSimulator::sm_SOLIDPenetrationLicenceWarning = false;
#if __BANK
extern bank_u8 s_MaxPenetratingFrames;
#endif // __BANK

#ifdef __SNC__
// suppress warning 552: variable "rage::s_SimInstance" was set but never used
#pragma diag_suppress 552
#endif

static phSimulator* s_SimInstance;

void phSimulator::SetActiveInstance (phSimulator* instance)
{
	sm_ActiveInstance = instance;
	s_SimInstance = instance;
}


phSimulator::phSimulator()
	: m_Level(NULL)
	, m_ContactMgr(NULL)
	, m_ManifoldPool(NULL)
	, m_ContactPool(NULL)
	, m_CompositePointerPool(NULL)
	, m_ConstraintMgr(NULL)
	, m_SleepMgr(NULL)
#if EARLY_FORCE_SOLVE
	, m_PushPairsA(NULL)
	, m_PushPairsB(NULL)
#endif // EARLY_FORCE_SOLVE
	, m_IsSafeToModifyManifolds(true)
	, m_NumActivatingPairs(0)
	, m_UpdateRunning(false)
	, m_SafeToDelete(true)
	, m_LastUpdateThisFrame(true)
	, m_MaxManagedColliders(0)
	, m_NumUsedManagedColliders(0)
    , m_ManagedColliders(NULL)
	, m_ManagedColliderFlags(NULL)
	, m_ManagedSleeps(NULL)
	, m_AvailableColliderIndices(NULL)
	, m_InstBehaviors(NULL)
#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
	, m_OptInstBehaviorLevelIndices(NULL)
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX
	, m_OptInstBehaviorsNeedSort(false)
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX
	, m_NumInstBehaviors(0)
	, m_MaxInstBehaviors(0)
#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
	, m_InstBehaviorByLevelIndex(0)
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
	, m_AllCollisionsDoneCallback(MakeFunctor(phSimulator::DefaultAllCollisionsDoneCallback))
	, m_PreComputeAllImpactsFunc(MakeFunctor(phSimulator::DefaultPreComputeAllImpacts))
	, m_PreApplyAllImpactsFunc(MakeFunctor(phSimulator::DefaultPreApplyAllImpacts))
	, m_AllowBreakingCallback(MakeFunctorRet(phSimulator::DefaultAllowBreakingCallback))
#if __PS3
	, m_CollisionTasks(rage_new phWorkerThread("Collision", SPURS_TASK_START(collision)))
	, m_UpdateCollidersTasks(rage_new phWorkerThread("UpdateColliders", SPURS_TASK_START(updatecolliders)))
#else // __PS3
	, m_CollisionTasks(rage_new phWorkerThread(TASK_INTERFACE(ProcessPairListTask), phConfig::GetCollisionTaskScheduler()))
	, m_UpdateCollidersTasks(rage_new phWorkerThread(TASK_INTERFACE(UpdateCollidersTask), phConfig::GetColliderTaskScheduler()))
#endif // __PS3
#if USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
#if __PS3
	, m_CommitDeferredOctreeUpdatesTask(rage_new phWorkerThread("CommitOctreeUpdates", SPURS_TASK_START(commitdeferredoctreeupdates)))
#else	// __PS3
	, m_CommitDeferredOctreeUpdatesTask(rage_new phWorkerThread(TASK_INTERFACE(CommitDeferredOctreeUpdatesTask), 0))
#endif	// __PS3
#endif	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
	, m_gjkCacheSystem(NULL)
{
}

phSimulator::~phSimulator ()
{
#if USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
	delete m_CommitDeferredOctreeUpdatesTask;
#endif	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK

	delete m_UpdateCollidersTasks;
	delete m_CollisionTasks;

#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
	delete [] m_InstBehaviorByLevelIndex;
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
	delete [] m_OptInstBehaviorLevelIndices;
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
	delete [] m_InstBehaviors;
	delete m_ConstraintMgr;
	delete m_ManifoldPool;
	delete m_ContactPool;
	delete m_CompositePointerPool;
    delete m_OverlappingPairArray;
	delete m_ContactMgr;
#if EARLY_FORCE_SOLVE
	delete m_PushPairsA;
	delete m_PushPairsB;
#endif // EARLY_FORCE_SOLVE
	delete m_SleepMgr;
	delete [] m_AvailableColliderIndices;
	delete [] m_ManagedSleeps;
	delete [] m_ManagedColliderFlags;
	delete [] m_ManagedColliders;

	DestroyGJKCacheSystem(m_gjkCacheSystem);
}

#if EARLY_FORCE_SOLVE
const int DEFAULT_MAX_PUSH_PAIRS = 256;
#endif


void phSimulator::Init (phLevelNew *pLevelNew, const InitParams& userParams)
{
	Assertf(sm_InitClassCalled, "You must call phSimulator::InitClass before calling phSimulator::Init");

	Assert(m_gjkCacheSystem == NULL);
	m_gjkCacheSystem = CreateGJKCacheSystem();

	// Set the physics level for this simulator.
	Assert(pLevelNew);
	m_Level = pLevelNew;
#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	m_Level->SetEnableDeferredOctreeUpdate(true);
#endif

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	m_Level->SetEnableDeferredCompositeBvhUpdate(true);
#endif

	InitParams params = userParams;

	// If command line parameters were specified, use them instead
	PARAM_maxmanagedcolliders.Get(params.maxManagedColliders);
	if(PARAM_scratchpadsizekb.Get())
	{
		int scratchPadSizeKB = 0;
		PARAM_scratchpadsizekb.Get(scratchPadSizeKB);
		params.scratchpadSize = scratchPadSizeKB*1024;
	}
	PARAM_maxmanifolds.Get(params.maxManifolds);
	PARAM_maxhighprioritymanifolds.Get(params.highPriorityManifolds);
	PARAM_maxcompositemanifolds.Get(params.maxCompositeManifolds);
	PARAM_maxcontacts.Get(params.maxContacts);
	PARAM_maxexternvelmanifolds.Get(params.maxExternVelManifolds);
	if(PARAM_maxinstbehaviors.Get())
	{
		int iMaxInstBehaviors = 0;
		PARAM_maxinstbehaviors.Get(iMaxInstBehaviors);
		params.maxInstBehaviors = (u16)iMaxInstBehaviors;
	}
	PARAM_maxconstraints.Get(params.maxConstraints);
	PARAM_maxbigconstraints.Get(params.maxBigConstraints);
	PARAM_maxlittleconstraints.Get(params.maxLittleConstraints);
	if(PARAM_sleepbuffersizekb.Get())
	{
		int sleepBufferSizeKB = 0;
		PARAM_sleepbuffersizekb.Get(sleepBufferSizeKB);
		params.sleepBufferSize = sleepBufferSizeKB*1024;
	}
	PARAM_maxsleepislands.Get(params.maxSleepIslands);

	int maxManagedColliders = params.maxManagedColliders;
	int scratchpadSize = params.scratchpadSize;
	int maxManifolds = params.maxManifolds;
	int maxExternVelManifolds = params.maxExternVelManifolds;

	if (params.maxInstBehaviors > 0)
	{
		SetMaxInstBehaviors(params.maxInstBehaviors);
	}

	// Set the maximum number of managed colliders, and make sure it's not greater than the physics level's maximum number of active objects.
	// It can be less because the physics level can have active objects with colliders that are not managed by the simulator.
	m_MaxManagedColliders = maxManagedColliders;
	Assertf(m_MaxManagedColliders<=m_Level->GetMaxActive(),"phSimulator::Init has %i managed colliders but the physics level has a maximum of %i.",m_MaxManagedColliders,m_Level->GetMaxActive());

#if __PS3
	m_ManifoldDmaPlanPool = rage_new phManifold::DmaPlan[maxManifolds];
	m_ColliderDmaPlanPool = rage_new phCollider::DmaPlan[m_MaxManagedColliders];
#endif

	m_ManagedColliders = rage_new phCollider[m_MaxManagedColliders];
	m_ManagedColliderFlags = rage_new u8[m_MaxManagedColliders];
	m_ManagedSleeps = rage_new phSleep[m_MaxManagedColliders];
	m_AvailableColliderIndices = rage_new u16[m_MaxManagedColliders];

	for(int nAvailableColliderIndex = 0; nAvailableColliderIndex < m_MaxManagedColliders; ++nAvailableColliderIndex)
	{
		m_AvailableColliderIndices[nAvailableColliderIndex] = (u16)(nAvailableColliderIndex);
		m_ManagedColliders[nAvailableColliderIndex].Init(NULL, &m_ManagedSleeps[nAvailableColliderIndex]);
		m_ManagedColliderFlags[nAvailableColliderIndex] = 0;
		m_ManagedSleeps[nAvailableColliderIndex].Init(&m_ManagedColliders[nAvailableColliderIndex]);
#if __PS3
		m_ColliderDmaPlanPool[nAvailableColliderIndex].Initialize();
		m_ManagedColliders[nAvailableColliderIndex].GenerateDmaPlan(m_ColliderDmaPlanPool[nAvailableColliderIndex]);
#endif
	}
	m_NumUsedManagedColliders = 0;

	m_AddObjectDelayedSAPInst.Reset();
	m_AddObjectDelayedSAPLevelIndex.Reset();

	m_AddObjectDelayedSAPInst.Reserve( DELAYED_SAP_MAX );
	m_AddObjectDelayedSAPLevelIndex.Reserve( DELAYED_SAP_MAX );
	m_RemoveObjectDelayedSAPLevelIndex.Reserve( DELAYED_SAP_MAX );

    AssertMsg(maxManifolds < 65535, "No more than 65534 manifolds can be created.");

	// Create the manifold manager
	m_ManifoldPool = rage_new phPool<phManifold>(maxManifolds);
	m_ManifoldPool->SetHighPriorityCount(params.highPriorityManifolds);

	AssertMsg(params.maxContacts < 65535, "No more than 65534 contacts can be created.");
	m_ContactPool = rage_new phPool<phContact>(Min(65534, params.maxContacts));

	AssertMsg(params.maxCompositeManifolds < 65535, "No more than 65534 composite manifolds can be created.");
	m_CompositePointerPool = rage_new phPool<phCompositePointers>(params.maxCompositeManifolds);

    //m_OverlappingPairArray = rage_new phOverlappingPairArray(m_Level->GetMaxCollisionPairs(), m_Level->GetMaxObjects(), m_Level->GetMaxObjects());
	// We never need more pairs than the max number of manifolds.
	m_OverlappingPairArray = rage_new phOverlappingPairArray(maxManifolds, m_Level->GetMaxObjects(), m_Level->GetMaxObjects());


#if EARLY_FORCE_SOLVE
	m_PushPairsA = rage_new phOverlappingPairArray(DEFAULT_MAX_PUSH_PAIRS, 0, m_Level->GetMaxObjects());
	m_PushPairsB = rage_new phOverlappingPairArray(DEFAULT_MAX_PUSH_PAIRS, 0, m_Level->GetMaxObjects());
#endif // EARLY_FORCE_SOLVE

	// Create the contact manager.
	m_ContactMgr = rage_new phContactMgr(scratchpadSize, maxExternVelManifolds, m_OverlappingPairArray);

#if SCRATCH_ALLOCATOR
	static sysMemStack scratchAllocator(m_ContactMgr->GetScratchpad(), scratchpadSize);
	sysMemManager::GetInstance().SetScratchAllocator(scratchAllocator);
#endif

	m_ConstraintMgr = rage_new phConstraintMgr;
	m_ConstraintMgr->CreateConstraints(params.maxConstraints, params.maxBigConstraints, params.maxLittleConstraints);

	m_SleepMgr = rage_new phSleepMgr(params.sleepBufferSize, params.maxSleepIslands);

	int maxObjects = m_Level->GetMaxObjects();

#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
	m_InstBehaviorByLevelIndex = rage_new phInstBehavior*[maxObjects];
	for (int objectIndex=0; objectIndex<maxObjects; objectIndex++)
	{
		m_InstBehaviorByLevelIndex[objectIndex] = NULL;
	}
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
#if __PFDRAW
	// Set the bounds drawing color choice
	m_Level->sm_ColorChoiceFunc = phLevelNew::ColorChoiceFunc().ResetC<phSimulator, &phSimulator::SimColorChoice>(this);
#endif
}


void phSimulator::Clear ()
{
	m_NumUsedManagedColliders = 0;

	for(int nAvailableColliderIndex = 0; nAvailableColliderIndex < m_MaxManagedColliders; ++nAvailableColliderIndex)
	{
		m_ManagedColliders[nAvailableColliderIndex].SetInstanceAndReset(NULL);

		m_AvailableColliderIndices[nAvailableColliderIndex] = (u16)(nAvailableColliderIndex);
	}
}

void phSimulator::ValidateArchetypes ()
{
#if __ASSERT
#if ENABLE_PHYSICS_LOCK
	phIterator iterator(phIterator::PHITERATORLOCKTYPE_WRITELOCK);
#else	// ENABLE_PHYSICS_LOCK
	phIterator iterator;
#endif	// ENABLE_PHYSICS_LOCK
	iterator.InitCull_All();

	u16 levelIndex = iterator.GetFirstLevelIndex(m_Level);
	while(levelIndex != (u16)(-1))
	{
		phInst* inst = m_Level->GetInstance(levelIndex);
		Assert(inst->GetArchetype());
		Assert(inst->GetArchetype()->GetBound());
		Assertf(inst->GetArchetype()->GetBound()->GetType() >= 0 && inst->GetArchetype()->GetBound()->GetType() < phBound::NUM_BOUND_TYPES,
			"Validation found unknown bound type %p", inst->GetArchetype()->GetBound());

		levelIndex = iterator.GetNextLevelIndex(m_Level);
	}
#endif
}

void phSimulator::SortUnusedManagedColliderIndices ()
{
	std::sort(m_AvailableColliderIndices+m_NumUsedManagedColliders,m_AvailableColliderIndices+m_MaxManagedColliders);
}


void phSimulator::InitClass()
{
	Assertf(sm_InitClassCalled == false, "You called phSimulator::InitClass twice.");
	sm_InitClassCalled = true;

	int numWorkerThreads = phConfig::GetMaxNumWorkerThreads();

	if (numWorkerThreads == -1)
	{
		// The user has not specified the number of worker threads, so try to figure it out
		numWorkerThreads = sysTaskManager::GetNumThreads();
		phConfig::SetMaxNumWorkerThreads(numWorkerThreads, true);

		const char* nomtphys;
		const char* mtphys;
		if (PARAM_nomtphys.Get(nomtphys))
		{
			if (!stricmp(nomtphys, "forcesolver"))
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
				phConfig::SetForceSolverWorkerThreadCount(0);
			}
			else if (!stricmp(nomtphys, "contact"))
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
				phConfig::SetContactWorkerThreadCount(0);
			}
#if EARLY_FORCE_SOLVE
			else if (!stricmp(nomtphys, "integratevelocities"))
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
				phConfig::SetIntegrateVelocitiesWorkerThreadCount(0);
			}
			else if (!stricmp(nomtphys, "integratepositions"))
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
				phConfig::SetIntegratePositionsWorkerThreadCount(0);
			}
			else if (!stricmp(nomtphys, "applypushes"))
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
				phConfig::SetApplyPushesWorkerThreadCount(0);
			}
#else // EARLY_FORCE_SOLVE
			else if (!stricmp(nomtphys, "colliderpre"))
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
				phConfig::SetColliderPreWorkerThreadCount(0);
			}
			else if (!stricmp(nomtphys, "colliderpost"))
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
				phConfig::SetColliderPostWorkerThreadCount(0);
			}
#endif
			else if (!stricmp(nomtphys, "collision"))
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
				phConfig::SetCollisionWorkerThreadCount(0);
			}
			else
			{
				phConfig::SetWorkerThreadCount(0);
			}
		}
		else if (PARAM_mtphys.Get(mtphys)) // Disable worker threads in debug builds by default
		{
			if (!stricmp(mtphys, "forcesolver"))
			{
				phConfig::SetWorkerThreadCount(0);
				phConfig::SetForceSolverWorkerThreadCount(numWorkerThreads);
			}
			else if (!stricmp(mtphys, "contact"))
			{
				phConfig::SetWorkerThreadCount(0);
				phConfig::SetContactWorkerThreadCount(numWorkerThreads);
			}
#if EARLY_FORCE_SOLVE
			else if (!stricmp(mtphys, "integratevelocities"))
			{
				phConfig::SetWorkerThreadCount(0);
				phConfig::SetIntegrateVelocitiesWorkerThreadCount(numWorkerThreads);
			}
			else if (!stricmp(mtphys, "integratepositions"))
			{
				phConfig::SetWorkerThreadCount(0);
				phConfig::SetIntegratePositionsWorkerThreadCount(numWorkerThreads);
			}
			else if (!stricmp(mtphys, "applypushes"))
			{
				phConfig::SetWorkerThreadCount(0);
				phConfig::SetApplyPushesWorkerThreadCount(numWorkerThreads);
			}
#else // EARLY_FORCE_SOLVE
			else if (!stricmp(mtphys, "colliderpre"))
			{
				phConfig::SetWorkerThreadCount(0);
				phConfig::SetColliderPreWorkerThreadCount(numWorkerThreads);
			}
			else if (!stricmp(mtphys, "colliderpost"))
			{
				phConfig::SetWorkerThreadCount(0);
				phConfig::SetColliderPostWorkerThreadCount(numWorkerThreads);
			}
#endif // EARLY_FORCE_SOLVE
			else if (!stricmp(mtphys, "collision"))
			{
				phConfig::SetWorkerThreadCount(0);
				phConfig::SetCollisionWorkerThreadCount(numWorkerThreads);
			}
			else
			{
				phConfig::SetWorkerThreadCount(numWorkerThreads);
			}
		}
		else
		{
#if __OPTIMIZED
			phConfig::SetWorkerThreadCount(numWorkerThreads);
#else
			phConfig::SetWorkerThreadCount(0);
#endif
		}
	}

#if __WIN32
	int numCullers = phConfig::GetTotalMaxNumWorkerThreads();
	ASSERT_ONLY(s_PerThreadStackAllocCreated = true;)
	s_PerThreadStackAllocTable = rage_new btStackAlloc*[numCullers];
	for (int alloc = 0; alloc < numCullers; ++alloc)
	{
		s_PerThreadStackAllocTable[alloc] = rage_new btStackAlloc(64 * 1024);
	}
#endif // __WIN32

	PPU_ONLY(phSpuTaskSet::InitClass();)
	phArticulatedCollider::InitClass();

	// Setup SPU->PPU connection for verbose non-normal assertions
#if __PPU && __ASSERT
	sysTaskManager::SetSpuEventHandler(EVENTCMD_ASSERT_CONTACT_NORM, phManifold::SpuAssertNormalEventHandler);
#endif

#if USER_JBN
#if __PS3 && 0
	phConfig::SetWorkerThreadCount(0);
	phConfig::SetCollisionMainThreadAlsoWorks(false);
	phConfig::SetCollisionWorkerThreadCount(1);
	phConfig::SetCollisionMainThreadAlsoWorks(false);
#else
	phConfig::SetWorkerThreadCount(0);
#endif
#endif // USER_JBN
}


void phSimulator::ShutdownClass ()
{
	Assertf(sm_InitClassCalled, "You called phSimulator::ShutdownClass without calling phSimulator::InitClass.");

	phArticulatedCollider::ShutdownClass();

#if __PPU
	phSpuTaskSet::ShutdownClass();
#endif

#if __WIN32
	ASSERT_ONLY(s_PerThreadStackAllocCreated = false;)
	for (int stackAlloc = 0; stackAlloc < phConfig::GetTotalMaxNumWorkerThreads(); ++stackAlloc)
	{
		delete s_PerThreadStackAllocTable[stackAlloc];
		s_PerThreadStackAllocTable[stackAlloc] = NULL;
	}
	delete [] s_PerThreadStackAllocTable;
#endif

	sm_InitClassCalled = false;
}

void phSimulator::SetMaxInstBehaviors(u16 maxInstBehaviors)
{
	Assert(m_InstBehaviors == NULL);
	m_MaxInstBehaviors = maxInstBehaviors;
	AllocateInstBehaviors();
}

void phSimulator::AllocateInstBehaviors()
{
	Assert(m_InstBehaviors==NULL);
	if (m_MaxInstBehaviors>0)
	{
		m_InstBehaviors = rage_new phInstBehavior *[m_MaxInstBehaviors];

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
		Assert(m_OptInstBehaviorLevelIndices == NULL);
		m_OptInstBehaviorLevelIndices = rage_new u16[m_MaxInstBehaviors];
#endif
	}
}

void phSimulator::Reset ()
{
	// Reset the contact manager, the collision manifolds and the collision islands.
	m_ConstraintMgr->Reset();
    //m_ManifoldPool->Reset();
	m_ContactMgr->Reset();

	// Release all the manifolds (resetting manifolds above only releases composite manifolds).
	phBroadPhase* broadPhase = m_Level->GetBroadPhase();
	btBroadphasePair* prunedPairs = broadPhase->getPrunedPairs();
	const int numPrunedPairs = broadPhase->getPrunedPairCount();
	for (int pairIndex=0; pairIndex<numPrunedPairs; pairIndex++)
	{
		btBroadphasePair* pair = prunedPairs + pairIndex;
		DESTRUCT_BP_PAIR(pair);
	}

	m_OverlappingPairArray->pairs.ResetCount();

	// Reset the sweep-and-prune arrays.
	m_AddObjectDelayedSAPInst.Resize(0);
	m_AddObjectDelayedSAPLevelIndex.Resize(0);
	m_RemoveObjectDelayedSAPLevelIndex.Resize(0);
}


void phSimulator::ResetInstanceBehaviors()
{
	//////////////////////////////////////////////////////////////////////////////////////
	// Reset any instance behaviors.
	for(int InstBehaviorIndex = 0; InstBehaviorIndex < m_NumInstBehaviors; ++InstBehaviorIndex)
	{
		m_InstBehaviors[InstBehaviorIndex]->Reset();
	}
}

void phSimulator::SetAllCollisionsDoneCallback(AllCollisionsDoneCallback func)
{
	m_AllCollisionsDoneCallback = func;
}

void phSimulator::SetPreComputeAllImpactsFunc(PreComputeAllImpactsFunc func)
{
	m_PreComputeAllImpactsFunc = func;
}

void phSimulator::SetPreApplyAllImpactsFunc(PreApplyAllImpactsFunc func)
{
	m_PreApplyAllImpactsFunc = func;
}

void phSimulator::SetAllowBreakingCallback(AllowBreakingCallback func)
{
	m_AllowBreakingCallback = func;
}

void phSimulator::DefaultAllCollisionsDoneCallback ()
{
}

void phSimulator::DefaultPreComputeAllImpacts (phContact** UNUSED_PARAM(contacts), int UNUSED_PARAM(numContacts))
{
}

void phSimulator::DefaultPreApplyAllImpacts (phContact** UNUSED_PARAM(contacts), int UNUSED_PARAM(numContacts))
{
}

bool phSimulator::DefaultAllowBreakingCallback()
{
	return true;
}

void phSimulator::SetMaintainLooseOctree(bool enabled)     
{
	g_MaintainLooseOctree = enabled;
}

bool phSimulator::GetMaintainLooseOctree()
{
	return g_MaintainLooseOctree;
}


bool phSimulator::AddInstBehavior (phInstBehavior& instBehavior)
{
	AssertMsg(m_Level->LegitLevelIndex(instBehavior.GetInstance()->GetLevelIndex()), "Adding inst behavior to an instance that is not in the level");
	AssertMsg(m_Level->IsInLevel(instBehavior.GetInstance()->GetLevelIndex()), "Adding inst behavior to an instance that is not in the level");
	Assert(m_NumInstBehaviors < m_MaxInstBehaviors);

	if(m_NumInstBehaviors < m_MaxInstBehaviors)
	{
		m_InstBehaviors[m_NumInstBehaviors] = &instBehavior;
		++m_NumInstBehaviors;

#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
		m_InstBehaviorByLevelIndex[instBehavior.GetInstance()->GetLevelIndex()] = &instBehavior;
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY

		// Note: In the PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE case,
		// we could add the level index to the end of m_OptInstBehaviorLevelIndices,
		// to keep the arrays parallel with matching data. However, the values in that
		// array should currently not be used by anything except for when we are sorted.
		// so it shouldn't be necessary. /FF

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX
		// We added something to the end, so the array probably needs to be sorted.
		// TODO: We may want to consider checking if it's already in order where it's
		// at, if it's likely that they get inserted in ascending level index
		// order, or consider doing an ordered insertion instead of re-sorting,
		// if games rarely add more than one per update. /FF
		m_OptInstBehaviorsNeedSort = true;
#endif

		return true;
	}

	return false;
}


void phSimulator::RemoveInstanceAndBehavior (phInstBehavior& instBehavior, bool forceImmediateDelete)
{
	const phInst* instance = instBehavior.GetInstance();
	RemoveInstBehavior(instBehavior);
	if (instance)
	{
		DeleteObject(instance->GetLevelIndex(),forceImmediateDelete);
	}
}


void phSimulator::RemoveInstBehavior (phInstBehavior& instBehavior)
{
	// Note: If the instance behavior array is sorted by level index, we could
	// potentially do a binary search (call FindInstanceBehaviorInternal()) here.
	// However (unless we move all entries after the removed one), the array would
	// then be unsorted, so it wouldn't really help if more than one instance behavior
	// is removed (which is probably a somewhat common case). For simplicity, and because
	// removing instance behaviors probably doesn't happen that much, I kept the old
	// linear search version here for now. /FF

	Assert(m_NumInstBehaviors > 0);
	for(int InstBehaviorIndex = 0; InstBehaviorIndex < m_NumInstBehaviors; ++InstBehaviorIndex)
	{
		if(m_InstBehaviors[InstBehaviorIndex] == &instBehavior)
		{
			--m_NumInstBehaviors;
			m_InstBehaviors[InstBehaviorIndex] = m_InstBehaviors[m_NumInstBehaviors];

#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
			m_InstBehaviorByLevelIndex[instBehavior.GetInstance()->GetLevelIndex()] = NULL;
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX
			// Since we have messed with the array's order, remember that we need to
			// sort it again later. /FF
			m_OptInstBehaviorsNeedSort = true;
#endif
			return;
		}
	}
	Assertf(false, "phInstBehavior 0x%p is not in the simulator list", &instBehavior);
}

phInstBehavior * phSimulator::GetInstBehaviorBySearch(int levelIndex, bool needsLock) const
{
#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX
	if (needsLock)
	{
		s_SortInstanceBehaviorsToken.Lock();
	}

	int instBehaviorIndex;
	if(!m_OptInstBehaviorsNeedSort)
	{
		if (needsLock)
		{
			s_SortInstanceBehaviorsToken.Unlock();
		}

		// The array is sorted, use binary search. /FF
		instBehaviorIndex = FindInstanceBehaviorInternal(levelIndex);
	}
	else
	{
		// The array is currently not sorted, so sort it to we can use
		// binary searching. /FF
		SortInstanceBehaviors();

		if (needsLock)
		{
			s_SortInstanceBehaviorsToken.Unlock();
		}

		instBehaviorIndex = FindInstanceBehaviorInternal(levelIndex);
	}

	// Check if we found anything. /FF
	if(instBehaviorIndex >= 0)
	{
		Assert(instBehaviorIndex < m_NumInstBehaviors);
		return m_InstBehaviors[instBehaviorIndex];
	}
#else
	// Unsorted array, do linear seach. /FF
	for(int InstBehaviorIndex = 0; InstBehaviorIndex < m_NumInstBehaviors; ++InstBehaviorIndex)
	{
		if(m_InstBehaviors[InstBehaviorIndex]->GetInstance()->GetLevelIndex() == levelIndex)
		{
			return m_InstBehaviors[InstBehaviorIndex];
		}
	}
#endif

	return NULL;
}


#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SANITY_CHECK

void phSimulator::SanityCheckInstanceBehaviorArrays() const
{
	if(m_OptInstBehaviorsNeedSort)
	{
		return;
	}

	const int num = m_NumInstBehaviors;
	Assert(num <= m_MaxInstBehaviors);
	int prev = -1;
	for(int i = 0; i < num; i++)
	{
		const phInstBehavior *instBehavior = m_InstBehaviors[i];
		Assert(instBehavior);
		const phInst *inst = instBehavior->GetInstance();
		Assert(inst);
		const int levelIndex = inst->GetLevelIndex();
		Assert(levelIndex > prev);

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
		Assert(levelIndex == m_OptInstBehaviorLevelIndices[i]);
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
		prev = levelIndex;
	}
}

#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SANITY_CHECK

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX

// PURPOSE:	Helper class needed to use STL's binary search.
class phInstBehaviorLevelIndexHelper
{
public:
	phInstBehaviorLevelIndexHelper(int levelIndex)
			: m_LevelIndex(levelIndex)
	{
	}

	phInstBehaviorLevelIndexHelper(phInstBehavior *a)
			: m_LevelIndex(a->GetInstance()->GetLevelIndex())
	{
	}

	int	m_LevelIndex;
};

// PURPOSE:	Comparison functor for sorting and binary searching
//			in arrays of phInstBehavior pointers.
class CompareInstBehaviorByLevelIndex : public std::binary_function<phInstBehaviorLevelIndexHelper, phInstBehaviorLevelIndexHelper, bool>
{
public:
	bool operator()(const phInstBehaviorLevelIndexHelper& left, const phInstBehaviorLevelIndexHelper& right) const
	{
		return left.m_LevelIndex < right.m_LevelIndex;
	}
};

int phSimulator::FindInstanceBehaviorInternal(int levelIndex) const
{
	Assert(!m_OptInstBehaviorsNeedSort);	// Caller should have checked. /FF

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
	Assert(levelIndex >= 0 && levelIndex <= 0xffff);

	// We have a separate array with level indices, use STL binary search.
	// The advantage of this over the binary search directly in m_InstBehaviors[]
	// is that we avoid a bunch of pointer dereferences and cache misses,
	// and the compiler appears to do a better job optimizing the search
	// than if going through the phInstBehaviorLevelIndexHelper helper
	// (I spotted a bctr to sCompareInstBehaviorByLevelIndex, it wasn't inlined). /FF
	const u16 *start = m_OptInstBehaviorLevelIndices;
	const u16 *end = start + m_NumInstBehaviors;
	const u16 *d = std::lower_bound(start, end, (u16)levelIndex);
	if(d >= start && d < end)
	{
		// We still need to check if we found it, because lower_bound() basically
		// just tells us where it would be if it's there. /FF
		if(*d == levelIndex)
		{
			return (int)(d - start);
		}
	}
#else	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE

	phInstBehavior **end = m_InstBehaviors + m_NumInstBehaviors;

	// Create the key. Note that we could probably avoid the whole
	// phInstBehaviorLevelIndexHelper business if instead we somehow created
	// (or pretended to have) a phInstBehavior that points to a phInst with
	// the level index we are looking for, but that would be messy. /FF
	phInstBehaviorLevelIndexHelper key(levelIndex);

	// Do the binary search. /FF
	phInstBehavior **d = std::lower_bound(m_InstBehaviors, end, key, CompareInstBehaviorByLevelIndex());
	if(d >= m_InstBehaviors && d < end)
	{
		// We still need to check if we found it, because lower_bound() basically
		// just tells us where it would be if it's there. /FF
		if((*d)->GetInstance()->GetLevelIndex() == levelIndex)
		{
			return d - m_InstBehaviors;
		}
	}
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE

	return -1;
}


void phSimulator::SortInstanceBehaviors() const
{
	PF_FUNC(SortInstanceBehaviors);

	Assert(m_OptInstBehaviorsNeedSort);	// Caller should have checked. /FF

	const int num = m_NumInstBehaviors;
	phInstBehavior **end = m_InstBehaviors + num;
	std::sort(m_InstBehaviors, end, CompareInstBehaviorByLevelIndex());

#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE
	// In this case, we want to have a separate array that just contains the indices.
	// The simplest and possibly fastest way to do this seems to be to just do an
	// extra pass over the array and copy the level indices. /FF
	for(int i = 0; i < num; i++)
	{
		m_OptInstBehaviorLevelIndices[i] = m_InstBehaviors[i]->GetInstance()->GetLevelIndex();
	}
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SEPARATE

	m_OptInstBehaviorsNeedSort = false;
}

#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX

void phSimulator::AddSelfCollision(phInst* instance)
{
    m_Level->GetBroadPhase()->addOverlappingPair(instance->GetLevelIndex(), instance->GetLevelIndex());
}


void phSimulator::PreUpdateInstanceBehaviors (float timeStep)
{
	///////////////////////////////////////////////////////////////////////////////////////////////
	// PreUpdate any instance behaviors. This is used for cloth to update positions from velocities
	// before colliding, and after drawing, to avoid drawing penetrating vertices. The same sequence
	// is used for physically simulated objects.
	for (int InstBehaviorIndex = 0; InstBehaviorIndex < m_NumInstBehaviors; ++InstBehaviorIndex)
	{
		m_InstBehaviors[InstBehaviorIndex]->PreUpdate(timeStep);
	}
}


void phSimulator::UpdateInstanceBehaviors (float timeStep)
{
#if PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SANITY_CHECK
	SanityCheckInstanceBehaviorArrays();
#endif	// PHSIM_OPT_INSTBEHAVIOR_SORT_BY_LEVELINDEX_SANITY_CHECK

	//////////////////////////////////////////////////////////////////////////////////////
	// Update any instance behaviors.
	for(int InstBehaviorIndex = 0; InstBehaviorIndex < m_NumInstBehaviors; ++InstBehaviorIndex)
	{
		m_InstBehaviors[InstBehaviorIndex]->Update(timeStep);
	}
}


void phSimulator::UpdateOneCollider (phCollider* collider, float timeStep, bool doCollisions) const
{
	Assert(collider);
	Assert(collider->GetInstance());
	phSleep* sleep = collider->GetSleep();
	if (sleep && sleep->IsAsleep())
	{
		// This collider is sleeping, so see if it should be awakened.
		if (!sleep->CheckNearlyStill(timeStep, 0.0f))
		{
			// External impulses or applied motion since the last simulator update caused enough motion to wake up this sleeping collider.
			// CheckNearlyStill includes impulses, pushes and velocities, but does not include forces, so that external forces can be
			// balanced by simulator impetuses (gravity or collisions) without awakening the object.
			sleep->WakeUp();
		}
	}
	
	// Update its velocity and position.
	Scalar vTimeStep;
	vTimeStep.Set(timeStep);
	collider->Update(vTimeStep, sm_Gravity);
	int levelIndex = collider->GetInstance()->GetLevelIndex();
	m_Level->UpdateObjectLocation(levelIndex,&RCC_MATRIX34(collider->GetLastInstanceMatrix()));

	Assert(!doCollisions);	// not finished yet
	if (doCollisions)
	{
	//	Collide();
	//	m_ContactMgr->Update(timeStep);
	}
	
    if (sleep && !ColliderIsPermanentlyActive(collider))
    {
	    // The collider has a sleep and it is not permanently active, so update the sleep. This done after gravity and collisions so that objects
	    // that are kept in equilibrium by external impetuses balancing simulator impetuses can go to sleep. If sleep were updated before collision
	    // tests, a car resting on its wheel forces, for example, would be kept awake by the wheel forces before they could be balanced by gravity.
	    collider->UpdateSleep(timeStep);

		// Deactivation would occur here, if the collider's sleep is finished sleeping, but this method does not deactivate colliders.
	}

	// Update this collider's velocity from impulses and forces.
	collider->UpdateVelocity(vTimeStep);

	if (doCollisions)
	{
	//	m_ContactMgr->ComputeAndApplyPushes(timeStep);
	}
	
	// Update the collider's position from pushes.
	collider->Move(vTimeStep, m_ContactMgr->GetUsePushes());

	// Inform the physics level that this collider may have moved.
	m_Level->UpdateObjectLocation(levelIndex, &RCC_MATRIX34(collider->GetLastInstanceMatrix()));
	
	if (doCollisions)
	{
		// Deactivate all the contacts.
	//	m_ContactMgr->PostUpdate();

	//	m_Level->PostCollideActives();
	}
}

static bool s_NeedsCollision;

#if __BANK && EARLY_FORCE_SOLVE
bool g_PushCollisionTTY;
#endif // __BANK

#if __PS3
extern float g_JointLimitElasticity;
#endif // __PS3

EARLY_FORCE_SOLVE_ONLY(extern float g_PushCollisionTolerance;)
EARLY_FORCE_SOLVE_ONLY(extern float g_TurnCollisionTolerance;)

void phSimulator::FillInColliderUpdateParams(sysTaskParameters& params, int updateType, float timeStep)
{
	static u32 s_UpdateCollidersAtom;
	s_UpdateCollidersAtom = 0;

	params.UserData[0].asInt = updateType;
	params.UserData[1].asPtr = &s_UpdateCollidersAtom;
	params.UserData[2].asInt = m_Level->GetNumActive();
	params.UserData[3].asPtr = m_Level->GetActiveObjectArray();
	params.UserData[4].asPtr = m_Level->GetActiveObjectUserDataArray();
	params.UserData[5].asFloat = timeStep;
	params.UserData[6].asFloat = sm_Gravity.x;
	params.UserData[7].asFloat = sm_Gravity.y;
	params.UserData[8].asFloat = sm_Gravity.z;
	params.UserData[9].asBool = sm_SleepEnabled;
#if EARLY_FORCE_SOLVE
	params.UserData[11].asPtr = &s_NeedsCollision;
#if __BANK
	params.UserData[12].asBool = g_PushCollisionTTY;
#endif
#else
	params.UserData[11].asBool = phContactMgr::GetUsePushes();
#endif

#if __PS3
	params.UserData[13].asPtr = phArticulatedCollider::GetActiveArray();
	params.UserData[14].asPtr = phArticulatedCollider::GetActiveCountPtr();
	params.UserData[15].asFloat = g_JointLimitElasticity;
#endif

	params.UserData[16].asFloat = s_AllowedAnglePenetration;

#if __PS3
	params.UserData[17].asFloat = g_PushCollisionTolerance;
	params.UserData[18].asFloat = g_TurnCollisionTolerance;
#endif
}

#if !EARLY_FORCE_SOLVE

///////////////////////////////////////////////////////////////////////////////////////////////////////
// PreCollide() should be called before the main loop for testing collisions.
void phSimulator::PreCollide (float timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::PreCollide");
	PF_FUNC(PreCollide);

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR
	// Commit any changes that are queued up from client code moving instances.  The main reason why this matters is so that activations that occur due
	//   to collisions can prime the overlapping pair array properly (we perform a level cull to find the initial set of overlapping objects).
	InitiateCommitDeferredOctreeUpdatesTask();
	FlushCommitDeferredOctreeUpdatesTask();
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR

	ScalarV vTimeStep(ScalarVFromF32(timeStep));

	// Prevent the physics level from adding or deleting active objects while the simulator does collisions.
	m_Level->FreezeActiveState();

	if (sm_ColliderUpdateEnabled)
    {
		//////////////////////////////////////////////////////////////////////////////////////
		// See if any sleeping active objects should be awakened.
		PF_START_TIMEBAR_PHYS_SIM_DETAIL("Update Sleep");
		PF_START(UpdateSleep);
		int iActiveIndex;
		for( iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
		{
			int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 
			if(sm_ColliderUpdateIndex != phInst::INVALID_INDEX && sm_ColliderUpdateIndex != levelIndex)
				continue;

			phCollider* collider = GetActiveCollider(levelIndex);
			Assert(collider);
			Assert(collider->GetInstance());
			phSleep* sleep = collider->GetSleep();
			if (sleep && sleep->IsAsleep())
			{
				// This collider is sleeping, so see if it should be awakened.
				if (!sleep->CheckNearlyStill(timeStep, 0.0f))
				{
					// External impulses or applied motion since the last simulator update caused enough motion to wake up this sleeping collider.
					// CheckNearlyStill includes impulses, pushes and velocities, but does not include forces, so that external forces can be
					// balanced by simulator impetuses (gravity or collisions) without awakening the object.
					sleep->WakeUp();
				}
			}
		}
        PF_STOP(UpdateSleep);

		//////////////////////////////////////////////////////////////////////////////////////
		// Update active objects.
		PF_START_TIMEBAR_PHYS_SIM_DETAIL("Update Colliders Pre");
		PF_START(UpdateColliders); 

		if (phConfig::GetColliderPreWorkerThreadCount()>0)
		{
			static sysTaskParameters params ;
			FillInColliderUpdateParams(params, PHCOLLIDER_UPDATE, timeStep);

			m_UpdateCollidersTasks->Initiate(params, phConfig::GetColliderPreWorkerThreadCount());

			if (phConfig::GetColliderPreMainThreadAlsoWorks())
			{
				UpdateCollidersTask(params);
			}

			m_UpdateCollidersTasks->Wait();

			// With PHYSLEVEL_OPT_CACHE on, we shouldn't be performing *any* level operations during the collider update, so we should be able to
			//   avoid calling this until now.
			// TODO: Is this really even necessary now?  I'm not sure.
			m_Level->BeginMultipleUpdates();

			PF_START_TIMEBAR_PHYS_SIM_DETAIL("Update Object Locations");

			const bool isIncrementalBroadPhase = m_Level->GetBroadPhase()->IsIncremental();

			for( iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
			{
				int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 
				phCollider* collider = GetActiveCollider(levelIndex);

				if (collider->IsArticulated() && !isIncrementalBroadPhase)
				{
					m_Level->GetBroadPhase()->addOverlappingPair((u16)levelIndex, (u16)levelIndex);
				}

				m_Level->UpdateObjectLocationAndRadius(levelIndex, &RCC_MATRIX34(collider->GetLastInstanceMatrix()));
			}

			m_Level->EndMultipleUpdates();
		}
		else
		{
			const bool isIncrementalBroadPhase = m_Level->GetBroadPhase()->IsIncremental();

			for( iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
			{
				int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 
				phCollider* collider = GetActiveCollider(levelIndex);
				Assert(collider);
				// Update its velocity and position.
				collider->Update(vTimeStep.GetIntrin128(), sm_Gravity);

				if (collider->IsArticulated() && !isIncrementalBroadPhase)
				{
					m_Level->GetBroadPhase()->addOverlappingPair((u16)levelIndex, (u16)levelIndex);
				}

				m_Level->UpdateObjectLocationAndRadius(levelIndex, &RCC_MATRIX34(collider->GetLastInstanceMatrix()));
			}
		}

		PF_STOP(UpdateColliders);
	}
}

void phSimulator::Collide (float timeStep)
{
    PF_FUNC(SimCollide);
	PF_FUNC(Collide);
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::Collide");

#if TRACE_COLLIDE && __XENON && !__FINAL
	const int TRACE_FRAME = 100;
	if (TIME.GetFrameCount() == TRACE_FRAME)
	{
		XTraceStartRecording("devkit:\\SimCollide.pix2");
	}
#endif

	// Clear the pair array for the incoming pairs, so that we can add pairs during ProcessOverlaps
	m_OverlappingPairArray->pairs.Resize(0);
	m_OverlappingPairArray->numCollisionPairs = 0;

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Broadphase prune");
	PF_START(PruneBroadphase);
	// Have the broadphase check for and remove pairs that should no longer be there.
	m_Level->GetBroadPhase()->pruneActiveOverlappingPairs();
	PF_STOP(PruneBroadphase);
    CollideActive(m_OverlappingPairArray, 0, timeStep);

#if TRACE_COLLIDE && __XENON && !__FINAL
	if (TIME.GetFrameCount() == TRACE_FRAME)
	{
		XTraceStopRecording();
	}
#endif

}

#endif // !EARLY_FORCE_SOLVE

#define FAST_BOXES 0

#if FAST_BOXES
bool g_FastBoxes = false;
#endif // FAST_BOXES

#define FAST_CAPSULES 0

#if FAST_CAPSULES
bool g_FastCapsules = false;
#endif // FAST_CAPSULES

#if COLLISION_SWEEP_DRAW
struct phCollisionSweepRecord
{
	int levelIndex;
	int generationId;
	int component;
	int iteration;
	Mat34V lastMatrix;
	Mat34V currentMatrix;
};

const int MAX_COLLISION_SWEEP_RECORDS = 128;

phCollisionSweepRecord g_CollisionSweepRecords[MAX_COLLISION_SWEEP_RECORDS];
u32 g_NumCollisionSweepRecords = 0;

static int s_CurrentCollisionIteration = 0;
#endif

__forceinline Mat34V_ConstRef GetLastMatrix(const phInst * pInst, const phCollider * collider, const phLevelNew * level, const phCollider * other_Collider, const bool other_InactiveCollidesAgainstInactive)
{
	if (collider)
	{
		if (other_Collider || other_InactiveCollidesAgainstInactive BANK_ONLY(|| !g_SweepFromSafe))
		{
			return collider->GetLastInstanceMatrix();
		}
		else
		{
			return collider->GetLastSafeInstanceMatrix();
		}
	}
	else
	{
		return level->GetLastInstanceMatrix(pInst);
	}
}

void phSimulator::CollisionIntersection (phManifold* rootManifold, phCollisionMemory * collisionMemory)
{
    PF_FUNC(MidNarrowPhase);

	phInst*     pInst1       = rootManifold->GetInstanceA();
	phInst*     pInst2       = rootManifold->GetInstanceB();

    if (pInst1 == pInst2)
    {
        CollisionSelfIntersection(pInst1, rootManifold);

		return;
    }
    else
    {
		const phLevelNew *level = m_Level;
		Assert(level == PHLEVEL);
		if (level->LegitLevelIndex(pInst1->GetLevelIndex()) && level->LegitLevelIndex(pInst2->GetLevelIndex()))
		{
			const phArchetype* archetype1 = pInst1->GetArchetype();
			Assert(archetype1);
			const phArchetype* archetype2 = pInst2->GetArchetype();
			Assert(archetype2);
			// TODO: Perhaps a prefetch would be appropriate here?

#if FAST_BOXES || FAST_CAPSULES
			const phBound* boundA = archetype1->GetBound();
			const phBound* boundB = archetype2->GetBound();
			int boundTypeA = boundA->GetType();
			int boundTypeB = boundB->GetType();
#endif // FAST_BOXES || FAST_CAPSULES

#if FAST_BOXES
			if(g_FastBoxes && boundTypeA == phBound::BOX && boundTypeB == phBound::BOX)
			{
				const phBoundBox* boxA = static_cast<const phBoundBox*>(boundA);
				Vec3V halfA, centerA;
				boxA->GetBoundingBoxHalfWidthAndCenter(halfA, centerA);
				const Mat34V& transA = pInst1->GetMatrix();
				Mat34V boxMatA;
				boxMatA.Set3x3(transA.GetMat33());
				boxMatA.SetCol3(Transform(transA, centerA));

				const phBoundBox* boxB = static_cast<const phBoundBox*>(boundB);
				Vec3V halfB, centerB;
				boxB->GetBoundingBoxHalfWidthAndCenter(halfB, centerB);
				const Mat34V& transB = pInst2->GetMatrix();
				Mat34V boxMatB;
				boxMatB.Set3x3(transB.GetMat33());
				boxMatB.SetCol3(Transform(transB, centerB));

				Vec3V normal, pointA, pointB;
				ScalarV dist = boxBoxDistance(normal, pointA, pointB, halfA, boxMatA, halfB, boxMatB);

				Vec3V worldPointOnA = Transform(boxMatA, pointA);
				Vec3V worldPointOnB = Transform(boxMatB, pointB);

				rootManifold->SetBoundA(boundA);
				rootManifold->SetBoundB(boundB);

				phManifoldResult manifoldResult;
				manifoldResult.Set(&RCC_MATRIX34(transA),boundA,&RCC_MATRIX34(transB),boundB,rootManifold);
				manifoldResult.AddContactPoint((-normal).GetIntrin128(), pointA.GetIntrin128(), pointB.GetIntrin128(), worldPointOnA.GetIntrin128(), worldPointOnB.GetIntrin128(), dist.GetIntrin128(), 0, 0 TRACK_COLLISION_TIME_PARAM(0.0f));

				return;
			}
#endif // FAST_BOXES

#if FAST_CAPSULES
			if(g_FastCapsules && boundTypeA == phBound::CAPSULE && boundTypeB == phBound::CAPSULE)
			{
				const phBoundCapsule* capsuleA = static_cast<const phBoundCapsule*>(boundA);
				const phBoundCapsule* capsuleB = static_cast<const phBoundCapsule*>(boundB);

				ScalarV marginA = capsuleA->GetMarginV();
				ScalarV marginB = capsuleB->GetMarginV();

				Vec3V pointA2 = pInst1->GetMatrix().GetCol1() * capsuleA->GetLengthV(marginA);
				Vec3V offset = pInst1->GetMatrix().GetCol3() - pointA2 * ScalarV(V_HALF);
				Vec3V halfLengthB = pInst2->GetMatrix().GetCol1() * capsuleB->GetHalfLengthV(marginB);
				Vec3V directionB = pInst2->GetMatrix().GetCol3();
				Vec3V pointB1 = directionB - halfLengthB - offset;
				Vec3V pointB2 = directionB + halfLengthB - offset;

				Vec3V pointA, pointB;
				geomPoints::FindClosestSegSegToSeg(pointA.GetIntrin128Ref(), pointB.GetIntrin128Ref(), pointA2.GetIntrin128(), pointB1.GetIntrin128(), pointB2.GetIntrin128());

				Vec3V delta = pointB - pointA;
				ScalarV dist = Mag(delta);
				ScalarV invDist = InvertSafe(dist);
				Vec3V normal = delta * invDist;

				Vec3V worldPointOnA = pointA + normal * marginA + offset;
				Vec3V worldPointOnB = pointB - normal * marginB + offset;

				ScalarV marginDist = dist - marginA - marginB;

				rootManifold->SetBoundA(boundA);
				rootManifold->SetBoundB(boundB);

				phManifoldResult manifoldResult;
				manifoldResult.Set(&RCC_MATRIX34(pInst1->GetMatrix()),boundA,&RCC_MATRIX34(pInst2->GetMatrix()),boundB,rootManifold);
				manifoldResult.AddContactPoint((-normal).GetIntrin128(), worldPointOnA.GetIntrin128(), worldPointOnB.GetIntrin128(), marginDist.GetIntrin128(), 0, 0);

				return;
			}
#endif // FAST_CAPSULES

			phCollisionInput input(collisionMemory,true);
			input.rootManifold = rootManifold;
			input.currentA = pInst1->GetMatrix();
			input.currentB = pInst2->GetMatrix();

			// Set up current and last instance matrices.
			const int levelIndex1 = pInst1->GetLevelIndex();
			const int levelIndex2 = pInst2->GetLevelIndex();
			phCollider* collider1 = GetCollider(levelIndex1);
			phCollider* collider2 = GetCollider(levelIndex2);
			const u32 typeFlags1 = level->GetInstanceTypeFlags(levelIndex1);
			const u32 includeFlags1 = level->GetInstanceIncludeFlags(levelIndex1);
			const u32 typeFlags2 = level->GetInstanceTypeFlags(levelIndex2);
			const u32 includeFlags2 = level->GetInstanceIncludeFlags(levelIndex2);

			input.lastA = GetLastMatrix(pInst1,collider1,level,collider2,rootManifold->m_InactiveCollidesAgainstInactiveB);
			input.lastB = GetLastMatrix(pInst2,collider2,level,collider1,rootManifold->m_InactiveCollidesAgainstInactiveA);
#if EARLY_FORCE_SOLVE
			// When we're performing push collisions, no pairs without collider should make it to here.
			if ((collider1 != NULL || collider2 != NULL) && (!collider1 || !collider1->GetNeedsCollision()) &&
				(!collider2 || !collider2->GetNeedsCollision()))
			{
				return;
			}
#endif // EARLY_FORCE_SOLVE_ONLY

#if COLLISION_SWEEP_DRAW
			phBound* bound1 = archetype1->GetBound();
			if (bound1->GetType() == phBound::COMPOSITE)
			{
				phBoundComposite* composite = static_cast<phBoundComposite*>(bound1);
				int numParts = composite->GetNumBounds();
				for (int i = 0; i < numParts; ++i)
				{
					const u32 nextRecord = sysInterlockedIncrement(&g_NumCollisionSweepRecords);
					if (nextRecord <= MAX_COLLISION_SWEEP_RECORDS)
					{
						phCollisionSweepRecord& record = g_CollisionSweepRecords[nextRecord - 1];
						record.levelIndex = levelIndex1;
						record.generationId = m_Level->GetGenerationID(levelIndex1);
						record.component = i;
						record.iteration = s_CurrentCollisionIteration;
						Transform(record.lastMatrix, input.lastA, composite->GetLastMatrix(i));
						Transform(record.currentMatrix, input.currentA, composite->GetCurrentMatrix(i));
					}
					else
					{
						g_NumCollisionSweepRecords = MAX_COLLISION_SWEEP_RECORDS;
					}
				}
			}
			else
			{
				const u32 nextRecord = sysInterlockedIncrement(&g_NumCollisionSweepRecords);
				if (nextRecord <= MAX_COLLISION_SWEEP_RECORDS)
				{
					phCollisionSweepRecord& record = g_CollisionSweepRecords[nextRecord - 1];
					record.levelIndex = levelIndex1;
					record.generationId = m_Level->GetGenerationID(levelIndex1);
					record.component = -1;
					record.iteration = s_CurrentCollisionIteration;
					record.lastMatrix = input.lastA;
					record.currentMatrix = input.currentA;
				}
				else
				{
					g_NumCollisionSweepRecords = MAX_COLLISION_SWEEP_RECORDS;
				}
			}

			phBound* bound2 = archetype2->GetBound();
			if (bound2->GetType() == phBound::COMPOSITE)
			{
				phBoundComposite* composite = static_cast<phBoundComposite*>(bound2);
				int numParts = composite->GetNumBounds();
				for (int i = 0; i < numParts; ++i)
				{
					const u32 nextRecord = sysInterlockedIncrement(&g_NumCollisionSweepRecords);
					if (nextRecord <= MAX_COLLISION_SWEEP_RECORDS)
					{
						phCollisionSweepRecord& record = g_CollisionSweepRecords[nextRecord - 1];
						record.levelIndex = levelIndex2;
						record.generationId = m_Level->GetGenerationID(levelIndex2);
						record.component = i;
						record.iteration = s_CurrentCollisionIteration;
						Transform(record.lastMatrix, input.lastB, composite->GetLastMatrix(i));
						Transform(record.currentMatrix, input.currentB, composite->GetCurrentMatrix(i));
					}
					else
					{
						g_NumCollisionSweepRecords = MAX_COLLISION_SWEEP_RECORDS;
					}
				}
			}
			else
			{
				const u32 nextRecord = sysInterlockedIncrement(&g_NumCollisionSweepRecords);
				if (nextRecord <= MAX_COLLISION_SWEEP_RECORDS)
				{
					phCollisionSweepRecord& record = g_CollisionSweepRecords[nextRecord - 1];
					record.levelIndex = levelIndex2;
					record.generationId = m_Level->GetGenerationID(levelIndex2);
					record.iteration = s_CurrentCollisionIteration;
					record.lastMatrix = input.lastB;
					record.currentMatrix = input.currentB;
				}
				else
				{
					g_NumCollisionSweepRecords = MAX_COLLISION_SWEEP_RECORDS;
				}
			}
#endif // COLLISION_SWEEP_DRAW

			input.boundA = archetype1->GetBound();
			input.typeFlagsA = typeFlags1;
			input.includeFlagsA = includeFlags1;
			input.boundB = archetype2->GetBound();
			input.typeFlagsB = typeFlags2;
			input.includeFlagsB = includeFlags2;

			input.highPriority = (pInst1->GetInstFlag(phInst::FLAG_HIGH_PRIORITY) || pInst2->GetInstFlag(phInst::FLAG_HIGH_PRIORITY));

#if HACK_GTA4_BOUND_GEOM_SECOND_SURFACE
			input.secondSurfaceInterpA=(pInst1 ? pInst1->GetBoundGeomIntersectionSecondSurfaceInterpolation() : 0);
			input.secondSurfaceInterpB=(pInst2 ? pInst2->GetBoundGeomIntersectionSecondSurfaceInterpolation() : 0);
#endif

			phMidphase midphaseProcessor;
			midphaseProcessor.ProcessCollision(input);

#if PHSIMULATOR_TEST_NEW_OBB_TEST
			int totalContactCount = 0;
			if(rootManifold->CompositeManifoldsEnabled())
			{
				// Search the sub-manifolds for contact points.
				for(int compositeManifoldIndex = 0; compositeManifoldIndex < rootManifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
				{
					const phManifold *manifoldToCheck = rootManifold->GetCompositeManifold(compositeManifoldIndex);
					for(int contactIndex = 0; contactIndex < manifoldToCheck->GetNumContacts(); ++contactIndex)
					{
						if(manifoldToCheck->GetContactPoint(contactIndex).GetDepth() > 0.0f)
						{
							++totalContactCount;
						}
					}
				}
			}
			else
			{
				// Search the root manifold for contact points.
				const phManifold *manifoldToCheck = rootManifold;
				for(int contactIndex = 0; contactIndex < manifoldToCheck->GetNumContacts(); ++contactIndex)
				{
					if(manifoldToCheck->GetContactPoint(contactIndex).GetDepth() > 0.0f)
					{
						++totalContactCount;
					}
				}
			}
			Assert(totalContactCount == 0 || pair->pad[0] == 0);
#endif

			return;
		}
    }

	rootManifold->RemoveAllContacts();
}

void phSimulator::CollisionSelfIntersection(phInst* inst, phManifold* manifold)
{
	PPC_STAT_TIMER_SCOPED(SelfCollisionTimer);
#if USE_NEW_SELF_COLLISION
	Assert(inst);
	phCollider * collider = GetActiveCollider(inst->GetLevelIndex());
	Assert(collider);
	if (collider->GetType() != phCollider::TYPE_RIGID_BODY)
	{
		phArticulatedCollider * articulatedCollider = static_cast<phArticulatedCollider *>(collider);

		// See if there's any reason not to process self collision pairs for this object.
		const bool anyPartsCanCollide = articulatedCollider->GetAnyPartsCanCollide();
		const bool selfCollisionEnabled = PHSIM->GetSelfCollisionsEnabled();
		const int disableSelfCollisionFramesLeft  = articulatedCollider->GetDisableSelfCollisionFramesLeft();
		const int numSelfCollisionPairs = articulatedCollider->GetNumSelfCollisionPairs();

		// Exit if self collisions aren't enabled for this articulated object.
		// Exit if self collisions are disabled globally.
		// The user may have requested to disable self collision for a few frames.
		// Exit if there are no collision pairs.
		if (Unlikely( (anyPartsCanCollide == false) || (selfCollisionEnabled == false) || (disableSelfCollisionFramesLeft > 0) || (numSelfCollisionPairs == 0) ))
			return;

		Assert(inst->GetArchetype() && inst->GetArchetype()->GetBound() && inst->GetArchetype()->GetBound()->GetType()==phBound::COMPOSITE);

		const phArchetype * archetype = inst->GetArchetype();
		const phBoundComposite * boundComposite = static_cast<const phBoundComposite *>(archetype->GetBound());
		Mat34V_ConstRef curInstanceMatrix = inst->GetMatrix();
		Mat34V_ConstRef lastInstanceMatrix = collider->GetLastInstanceMatrix();
		bool highPriorityManifolds = inst->GetInstFlag(phInst::FLAG_HIGH_PRIORITY) != 0;
		u8 * selfCollisionPairsA = articulatedCollider->GetSelfCollisionPairsA();
		u8 * selfCollisionPairsB = articulatedCollider->GetSelfCollisionPairsB();

		phMidphase midphaseProcessor;
#if ALLOW_MID_PHASE_SWAP
		if (g_UseNewMidPhaseCollision)
			midphaseProcessor.ProcessSelfCollisionNew(*boundComposite,curInstanceMatrix,lastInstanceMatrix,manifold,selfCollisionPairsA,selfCollisionPairsB,numSelfCollisionPairs,highPriorityManifolds);
		else
			midphaseProcessor.ProcessSelfCollisionOriginal(*boundComposite,curInstanceMatrix,lastInstanceMatrix,manifold,selfCollisionPairsA,selfCollisionPairsB,numSelfCollisionPairs,highPriorityManifolds);
#else
		midphaseProcessor.ProcessSelfCollisionNew(*boundComposite,curInstanceMatrix,lastInstanceMatrix,manifold,selfCollisionPairsA,selfCollisionPairsB,numSelfCollisionPairs,highPriorityManifolds);
#endif
	}
#else
	phCollider* collider = GetActiveCollider(inst->GetLevelIndex());
	collider->SelfCollision(manifold);
#endif
}

sysCriticalSectionToken g_PairConsumeToken;
int g_PairConsumeIndex;

void phSimulator::ProcessPairListTask(sysTaskParameters& p)
{
	PF_FUNC(CollideThread);
#if !__STATS
	PIXBegin(0, "CollisionTask");
#endif

	phCollisionMemory collisionMemory;

	phSimulator* sim = static_cast<phSimulator*>(p.UserData[0].asPtr);
	phOverlappingPairArray* pairArray = static_cast<phOverlappingPairArray*>(p.UserData[1].asPtr);
	ScalarV timeStep = ScalarV(p.UserData[2].asFloat);
	int pairConsumeIndex = 0;
	int pairsLeft = 0;
	bool morePairsToProcess = true;

	sysTimer timer;

	while (morePairsToProcess || pairsLeft > 0)
	{
		for (int pairIndex = 0; pairIndex < pairsLeft; ++pairIndex)
		{
			phTaskCollisionPair* currentPair = &pairArray->pairs[pairConsumeIndex + pairIndex];

#if EARLY_FORCE_SOLVE
			// With EARLY_FORCE_SOLVE, it's unavoidable to get constraint pairs in the collision system when doing push collisions. This is
			// because we're collecting pairs based on island membership. We need those pairs to go into the solver, so we need to just skip
			// over them when collision gets to them.
			if (currentPair->manifold->IsConstraint())
			{
				continue;
			}
#endif

			timer.Reset();

			sim->CollisionIntersection(currentPair->manifold,&collisionMemory);

			if (currentPair->manifold->CompositeManifoldsEnabled())
			{
				const int numCompositeManifolds = currentPair->manifold->GetNumCompositeManifolds();
				for (int manifoldIndex = 0; manifoldIndex < numCompositeManifolds; ++manifoldIndex)
				{
					phManifold* compositeManifold = currentPair->manifold->GetCompositeManifold(manifoldIndex);

					if (!s_PushCollision)
					{
						compositeManifold->IncrementContactLifetimes();
					}

					compositeManifold->RefreshContactPoints(GetMinManifoldPointLifetime(), timeStep);
				}
			}
			else
			{
				if (!s_PushCollision)
				{
					currentPair->manifold->IncrementContactLifetimes();
				}

				currentPair->manifold->RefreshContactPoints(GetMinManifoldPointLifetime(), timeStep);
			}

			currentPair->manifold->SetCollisionTime(timer.GetTickTime());
		}

		g_PairConsumeToken.Lock();
		pairConsumeIndex = g_PairConsumeIndex;
		pairsLeft = Min(pairArray->numCollisionPairs - pairConsumeIndex, sm_NumCollisionsPerTask);
		g_PairConsumeIndex += pairsLeft;
		g_PairConsumeToken.Unlock();

		morePairsToProcess = !pairArray->allPairsReady;
	}

#if !__STATS
	PIXEnd();
#endif
}

#if USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
void phSimulator::CommitDeferredOctreeUpdatesTask(sysTaskParameters &p)
{
	phLevelNew *physLevel = reinterpret_cast<phLevelNew *>(p.UserData[0].asPtr);
	physLevel->CommitDeferredOctreeUpdates();
}
#endif	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK

#define USE_SIMULATOR_STATS1 (0 && __STATS)

#if USE_SIMULATOR_STATS1
namespace SimulatorStats1
{
	PF_PAGE(SimPage1,"ph Simulator Stats1");

	PF_GROUP(SimulatorStats1);
	PF_LINK(SimPage1,SimulatorStats1);

	PF_COUNTER(numSimpleColliders,SimulatorStats1);
	PF_COUNTER(numArticulatedColliders,SimulatorStats1);
	PF_COUNTER(numRigidBodies,SimulatorStats1);
	
	PF_COUNTER(numBroadPhasePairs,SimulatorStats1);
	
	PF_COUNTER(numRootManifolds,SimulatorStats1);
	PF_COUNTER(numActiveRootManifolds,SimulatorStats1);
	PF_COUNTER(numCompositeManifolds,SimulatorStats1);
	//PF_COUNTER(numWastedManifolds,SimulatorStats1);
	//PF_COUNTER(numWastedBvhManifolds,SimulatorStats1);
	PF_COUNTER(numOtherManifolds,SimulatorStats1);
	
	PF_COUNTER(numComposites,SimulatorStats1);
	
	PF_COUNTER(numActiveContacts,SimulatorStats1);
	PF_COUNTER(numInactiveContacts,SimulatorStats1);
	PF_COUNTER(numOtherContacts,SimulatorStats1);

	PF_COUNTER(numRigidRigidPairs,SimulatorStats1);
	PF_COUNTER(numRigidInactivePairs,SimulatorStats1);
	PF_COUNTER(numRigidStaticPairs,SimulatorStats1);
	PF_COUNTER(numBvhPairs,SimulatorStats1);

	PF_COUNTER(collisionTimeMCS,SimulatorStats1);
	PF_COUNTER(collisionPairTimeMCSMax0,SimulatorStats1);
	PF_COUNTER(collisionPairTimeMCSMax1,SimulatorStats1);
	PF_COUNTER(RigidRigidPairTimeMCS,SimulatorStats1);
	PF_COUNTER(RigidInactivePairTimeMCS,SimulatorStats1);
	PF_COUNTER(RigidStaticPairTimeMCS,SimulatorStats1);

	PF_COUNTER(numCacheEntries,SimulatorStats1);
	PF_COUNTER(numRejectedStaticBvh,SimulatorStats1);
	PF_COUNTER(StaticBvhRejectionTimeMCS,SimulatorStats1);
	PF_COUNTER(CalcClosestLeafDistanceTimeMCS,SimulatorStats1);
	PF_COUNTER(numTestedStaticBvh,SimulatorStats1);
	PF_COUNTER(numCachedStaticBvh,SimulatorStats1);
	PF_COUNTER(numCalculatedStaticBvh,SimulatorStats1);
	PF_COUNTER(PruneNewPairsTimeMCS,SimulatorStats1);
};

using namespace SimulatorStats1;

#define STAT1_MCS_TIMER_START(name) sysTimer name##timer;
#define	STAT1_MCS_TIMER_STOP(name) PF_INCREMENTBY(name,u32(name##timer.GetUsTime()))
#define STAT1_INCREMENTBY(name,count) PF_INCREMENTBY(name,count)

void GetNumActiveContacts(phManifold * manifold, int * numActiveContactsAccum, int * numInactveContactsAccum)
{
	Assert(manifold->CompositeManifoldsEnabled() == false);
	int numActiveContacts = 0;
	int numInactiveContacts = 0;
	for (int contact_i = 0 ; contact_i < manifold->GetNumContacts() ; contact_i++)
	{
		const phContact & contact = manifold->GetContactPoint(contact_i);
		if (contact.IsContactActive())
			numActiveContacts++;
		else
			numInactiveContacts++;
	}
	*numActiveContactsAccum += numActiveContacts;
	*numInactveContactsAccum += numInactiveContacts;
}

u32 g_WastedManifoldCount = 20;

void GatherStatistics()
{
	int numSimpleColliders = 0;
	int numArticulatedColliders = 0;
	int numRigidBodies = 0;
	for (int activeIndex=PHLEVEL->GetFirstActiveIndex(); activeIndex!=phLevelBase::INVALID_STATE_INDEX; activeIndex=PHLEVEL->GetNextActiveIndex())
	{
		phCollider * collider = PHSIM->GetActiveCollider(activeIndex);
		if (collider->IsArticulated())
		{
			phArticulatedCollider * articulatedCollider = reinterpret_cast<phArticulatedCollider*>(collider);
			numArticulatedColliders++;
			numRigidBodies += articulatedCollider->GetBody()->GetNumBodyParts();
		}
		else
		{
			numSimpleColliders++;
			numRigidBodies++;
		}
	}

	int numRigidRigidPairs = 0;
	int numRigidInactivePairs = 0;
	int numRigidStaticPairs = 0;
	int numBvhPairs = 0;

	int numBroadPhasePairs = 0;
	
	int numRootManifolds = 0;
	int numActiveRootManifolds = 0;
	int numCompositeManifolds = 0;
	//int numWastedManifolds = 0;
	//int numWastedBvhManifolds = 0;

	int numComposites = 0;

	int numActiveContacts = 0;
	int numInactiveContacts = 0;
	
	u32 collisionTicks = 0;
	u32 collisionPairTicksMax0 = 0;
	u32 collisionPairTicksMax1 = 0;

	u32 RigidRigidPairTicks = 0;
	u32 RigidInactivePairTicks = 0;
	u32 RigidStaticPairTicks = 0;

	{
		phLevelNew* level = PHLEVEL;
		phBroadPhase* broadPhase = level->GetBroadPhase();
		btAxisSweep3 * broadPhase1 = reinterpret_cast<btAxisSweep3*>(broadPhase);
		const bool bOnlyMainThreadIsUpdatingPhysics = PHLEVEL->GetOnlyMainThreadIsUpdatingPhysics();
		if(!bOnlyMainThreadIsUpdatingPhysics)
		{
			//broadPhase1->m_CriticalSectionToken.Lock();
			broadPhase1->CriticalSectionLock();
		}
		else
		{
			//Assertf(sysIpcGetCurrentThreadId() == broadPhase1->m_threadId, "Only the main update thread should be updating the physics broadphase now!");
			Assertf(sysIpcGetCurrentThreadId() == broadPhase1->GetCurThreadId(), "Only the main update thread should be updating the physics broadphase now!");
		}


		btBroadphasePair *prunedPairs = broadPhase->getPrunedPairs();
		const int numPrunedPairs = broadPhase->getPrunedPairCount();

		numBroadPhasePairs = numPrunedPairs;
		for (int pair_i = 0 ; pair_i < numPrunedPairs ; pair_i++)
		{
			btBroadphasePair * pair = prunedPairs + pair_i;
			phManifold * manifold = pair->GetManifold();
			if (manifold)
			{
				numRootManifolds++;
				
				phBound * boundA = manifold->GetInstanceA()->GetArchetype()->GetBound();
				phBound * boundB = manifold->GetInstanceB()->GetArchetype()->GetBound();
				const bool hasBvh = (boundA->GetType() == phBound::BVH || boundB->GetType() == phBound::BVH);
				if (hasBvh)
					numBvhPairs++;

				// Count the number of active contacts in this manifold.
				int numManifoldActiveContacts = 0;
				int numManifoldInactiveContacts = 0;
				if (manifold->CompositeManifoldsEnabled())
				{
					numComposites++;
					numCompositeManifolds += manifold->GetNumCompositeManifolds();
					for (int compositeManifold_i = 0 ; compositeManifold_i < manifold->GetNumCompositeManifolds() ; compositeManifold_i++)
					{
						phManifold * compositeManifold = manifold->GetCompositeManifold(compositeManifold_i);
						GetNumActiveContacts(compositeManifold,&numManifoldActiveContacts,&numManifoldInactiveContacts);
					}
				}
				else
				{
					GetNumActiveContacts(manifold,&numManifoldActiveContacts,&numManifoldInactiveContacts);
				}
				numActiveContacts += numManifoldActiveContacts;
				numInactiveContacts += numManifoldInactiveContacts;
				if (numManifoldActiveContacts > 0)
				{
					numActiveRootManifolds++;
				}
				if (numManifoldActiveContacts + numManifoldInactiveContacts > 0)
				{
					manifold->m_NoCollisionCounter = 0;
				}
				else
				{
					manifold->m_NoCollisionCounter++;
				}
				
/*
				if (manifold->m_NoCollisionCounter >= g_WastedManifoldCount)
				{
					numWastedManifolds++;
					if (hasBvh)
					{
						numWastedBvhManifolds++;
					}
				}
*/

				// Get the collision time
				const u32 ticks = manifold->GetCollisionTime();
				collisionTicks += ticks;
				if (ticks > collisionPairTicksMax1)
				{
					if (ticks > collisionPairTicksMax0)
					{
						collisionPairTicksMax1 = collisionPairTicksMax0;
						collisionPairTicksMax0 = ticks;
					}
					else
					{
						collisionPairTicksMax1 = ticks;
					}
				}

				// Get the type of collision pair
				const u32 levelIndexA = manifold->GetLevelIndexA();
				const u32 levelIndexB = manifold->GetLevelIndexB();
				const bool fixedA = PHLEVEL->IsFixed(levelIndexA);
				const bool fixedB = PHLEVEL->IsFixed(levelIndexB);
				if (fixedA == false && fixedB == false)
				{
					if (manifold->GetColliderA() == NULL || manifold->GetColliderB() == NULL)
					{
						numRigidInactivePairs++;
						RigidInactivePairTicks += ticks;
					}
					else
					{
						numRigidRigidPairs++;
						RigidRigidPairTicks += ticks;
					}
				}
				else 
				{
					numRigidStaticPairs++;
					RigidStaticPairTicks += ticks;
				}
			}
		}

		if(!bOnlyMainThreadIsUpdatingPhysics)
		{
			//broadPhase1->m_CriticalSectionToken.Unlock();
			broadPhase1->CriticalSectionUnlock();
		}
	}
	const u32 collisionTimeMCS = u32(float(collisionTicks) * sysTimerConsts::TicksToMicroseconds);
	const u32 collisionPairTimeMCSMax0 = u32(float(collisionPairTicksMax0) * sysTimerConsts::TicksToMicroseconds);
	const u32 collisionPairTimeMCSMax1 = u32(float(collisionPairTicksMax1) * sysTimerConsts::TicksToMicroseconds);

	const u32 RigidRigidPairTimeMCS = u32(float(RigidRigidPairTicks) * sysTimerConsts::TicksToMicroseconds);
	const u32 RigidInactivePairTimeMCS = u32(float(RigidInactivePairTicks) * sysTimerConsts::TicksToMicroseconds);
	const u32 RigidStaticPairTimeMCS = u32(float(RigidStaticPairTicks) * sysTimerConsts::TicksToMicroseconds);

	const int numOtherManifolds = PHMANIFOLD->GetInUseCount() - (numRootManifolds + numCompositeManifolds);
	const int numOtherContacts = PHCONTACT->GetInUseCount() - (numActiveContacts + numInactiveContacts);

	PF_INCREMENTBY(numSimpleColliders,numSimpleColliders);
	PF_INCREMENTBY(numArticulatedColliders,numArticulatedColliders);
	PF_INCREMENTBY(numRigidBodies,numRigidBodies);
	
	PF_INCREMENTBY(numBroadPhasePairs,numBroadPhasePairs);

	PF_INCREMENTBY(numRootManifolds,numRootManifolds);
	PF_INCREMENTBY(numActiveRootManifolds,numActiveRootManifolds);
	PF_INCREMENTBY(numCompositeManifolds,numCompositeManifolds);
	//PF_INCREMENTBY(numWastedManifolds,numWastedManifolds);
	//PF_INCREMENTBY(numWastedBvhManifolds,numWastedBvhManifolds);
	PF_INCREMENTBY(numOtherManifolds,numOtherManifolds);

	PF_INCREMENTBY(numComposites,numComposites);

	PF_INCREMENTBY(numActiveContacts,numActiveContacts);
	PF_INCREMENTBY(numInactiveContacts,numInactiveContacts);
	PF_INCREMENTBY(numOtherContacts,numOtherContacts);

	PF_INCREMENTBY(numRigidRigidPairs,numRigidRigidPairs);
	PF_INCREMENTBY(numRigidInactivePairs,numRigidInactivePairs);
	PF_INCREMENTBY(numRigidStaticPairs,numRigidStaticPairs);
	PF_INCREMENTBY(numBvhPairs,numBvhPairs);

	PF_INCREMENTBY(collisionTimeMCS,collisionTimeMCS);
	PF_INCREMENTBY(collisionPairTimeMCSMax0,collisionPairTimeMCSMax0);
	PF_INCREMENTBY(collisionPairTimeMCSMax1,collisionPairTimeMCSMax1);

	PF_INCREMENTBY(RigidRigidPairTimeMCS,RigidRigidPairTimeMCS);
	PF_INCREMENTBY(RigidInactivePairTimeMCS,RigidInactivePairTimeMCS);
	PF_INCREMENTBY(RigidStaticPairTimeMCS,RigidStaticPairTimeMCS);
}

void IncrementNumCacheEntries(const int count)
{
	PF_INCREMENTBY(numCacheEntries,count);
}

#else // USE_SIMULATOR_STATS1 

#define STAT1_MCS_TIMER_START(name)
#define	STAT1_MCS_TIMER_STOP(name)
#define STAT1_INCREMENTBY(name,count)

#endif // USE_SIMULATOR_STATS1

#if USE_STATIC_BVH_REJECTION

#if USE_STATIC_BVH_REJECTION_SWAP
static bool g_UseStaticBvhRejection = true;
#endif // USE_STATIC_BVH_REJECTION_SWAP

__forceinline bool HasContacts(const phManifold * manifold)
{
	if (manifold)
	{
		if (manifold->CompositeManifoldsEnabled())
			return (manifold->GetNumCompositeManifolds() > 0);
		else
			return (manifold->GetNumContacts() > 0);
	}
	return false;
}

ScalarV_Out CalcDistanceSq(const phOptimizedBvh * bvh, const phOptimizedBvhNode * node, Vec3V_In pos)
{
	Vec3V nodeAABBMin;
	Vec3V nodeAABBMax;
	bvh->UnQuantize(RC_VECTOR3(nodeAABBMin), node->m_AABBMin);
	bvh->UnQuantize(RC_VECTOR3(nodeAABBMax), node->m_AABBMax);
	const Vec3V proj = Clamp(pos,nodeAABBMin,nodeAABBMax);
	return MagSquared(pos - proj);
}

#if 0
ScalarV CalcClosestLeafDistance(Vec3V_In pos, const phOptimizedBvh * bvh)
{
	const phOptimizedBvhNode * root = bvh->GetRootNode();
	const ScalarV rootDistSq = CalcDistanceSq(bvh,root,pos);

	if (root->IsLeafNode())
		return rootDistSq;

	const int MAX_STACK_SIZE = 256;
	const phOptimizedBvhNode * stack[MAX_STACK_SIZE];
	ScalarV distSqStack[MAX_STACK_SIZE];
	int stackSize = 1;
	stack[0] = root;
	distSqStack[0] = rootDistSq;
	
	ScalarV closestDistSq(V_FLT_MAX);

	do
	{
		FastAssert(stackSize > 0);
		stackSize--;
		const phOptimizedBvhNode * node = stack[stackSize];
		FastAssert(!node->IsLeafNode());
		
		if (IsGreaterThanAll(closestDistSq,distSqStack[stackSize]))
		{
			const phOptimizedBvhNode * leftChild = node + 1;
			const phOptimizedBvhNode * rightChild = leftChild + leftChild->GetEscapeIndex();

			const ScalarV leftDistSq = CalcDistanceSq(bvh,leftChild,pos);
			const ScalarV rightDistSq = CalcDistanceSq(bvh,rightChild,pos);

			// Swap the nodes so that we process the closest one first.
			const phOptimizedBvhNode * nodeList[2];
			ScalarV distSqList[2];
			if (IsLessThanOrEqualAll(leftDistSq,rightDistSq))
			{
				nodeList[0] = leftChild;
				nodeList[1] = rightChild;
				distSqList[0] = leftDistSq;
				distSqList[1] = rightDistSq;
			}
			else
			{
				nodeList[1] = leftChild;
				nodeList[0] = rightChild;
				distSqList[1] = leftDistSq;
				distSqList[0] = rightDistSq;
			}

			// We only need to recurse if the distance to the closer node is less than our current closest distance.
			if (IsGreaterThanAll(closestDistSq,distSqList[0]))
			{
				if (nodeList[0]->IsLeafNode())
				{
					// We don't need to examine the other branch since its closest distance must be greater than or equal to this leaf node's distance.
					closestDistSq = distSqList[0];
				}
				else
				{
					// Check if we need to examine the farther node.
					if (IsGreaterThanAll(closestDistSq,distSqList[1]))
					{
						if (nodeList[1]->IsLeafNode())
							closestDistSq = distSqList[1];
						else
						{
							FastAssert(stackSize < MAX_STACK_SIZE);
							stack[stackSize] = nodeList[1];
							distSqStack[stackSize] = distSqList[1];
							stackSize++;
						}
					}
					FastAssert(stackSize < MAX_STACK_SIZE);
					stack[stackSize] = nodeList[0];
					distSqStack[stackSize] = distSqList[0];
					stackSize++;
				}
			}
		}
	} while (stackSize > 0);
	return closestDistSq;
}

#else // #if 0

ScalarV CalcClosestLeafDistance(Vec3V_In pos, const phOptimizedBvh * bvh)
{
	const phOptimizedBvhNode * root = bvh->GetRootNode();
	const ScalarV rootDistSq = CalcDistanceSq(bvh,root,pos);

	if (root->IsLeafNode())
		return rootDistSq;

	const int MAX_STACK_SIZE = 256;
	const phOptimizedBvhNode * stack[MAX_STACK_SIZE];
	ScalarV distSqStack[MAX_STACK_SIZE];
	int stackSize = 1;
	stack[0] = root;
	distSqStack[0] = rootDistSq;

	ScalarV closestDistSq(V_FLT_MAX);

	do
	{
		FastAssert(stackSize > 0);
		stackSize--;
		const phOptimizedBvhNode * node = stack[stackSize];
		FastAssert(!node->IsLeafNode());

		// The stack is sorted by distance. Were done if we pop a node whose distance is greater than the current closest distance.
		if (IsLessThanOrEqualAll(closestDistSq,distSqStack[stackSize]))
			return closestDistSq;

		const phOptimizedBvhNode * leftChild = node + 1;
		const phOptimizedBvhNode * rightChild = leftChild + leftChild->GetEscapeIndex();

		const ScalarV leftDistSq = CalcDistanceSq(bvh,leftChild,pos);
		const ScalarV rightDistSq = CalcDistanceSq(bvh,rightChild,pos);

		if (IsLessThanAll(leftDistSq,closestDistSq))
		{
			if (leftChild->IsLeafNode())
			{
				closestDistSq = leftDistSq;
			}
			else
			{
				// Insert the node into the stack. Keep the stack sorted by distance so we process closer nodes first.
				FastAssert(stackSize < MAX_STACK_SIZE);
				int i = stackSize;
				while (i > 0 && IsGreaterThanAll(leftDistSq,distSqStack[i-1]))
				{
					distSqStack[i] = distSqStack[i-1];
					stack[i] = stack[i-1];
					i--;
				}
				distSqStack[i] = leftDistSq;
				stack[i] = leftChild;
				stackSize++;
			}
		}

		if (IsLessThanAll(rightDistSq,closestDistSq))
		{
			if (rightChild->IsLeafNode())
			{
				closestDistSq = rightDistSq;
			}
			else
			{
				// Insert the node into the stack. Keep the stack sorted by distance so we process closer nodes first.
				FastAssert(stackSize < MAX_STACK_SIZE);
				int i = stackSize;
				while (i > 0 && IsGreaterThanAll(rightDistSq,distSqStack[i-1]))
				{
					distSqStack[i] = distSqStack[i-1];
					stack[i] = stack[i-1];
					i--;
				}
				distSqStack[i] = rightDistSq;
				stack[i] = rightChild;
				stackSize++;
			}
		}
	} while (stackSize > 0);
	return closestDistSq;
}
#endif // #if 0

namespace BVCache
{
	struct BVInfo
	{
		Vec3V m_AABBHalfWidth;
		Vec3V m_AABBCenterAbs;

		Vec3V m_AABBHalfWidth_Safe;
		Vec3V m_AABBCenterAbs_Safe;

		Vec3V m_AABBHalfWidth_Total;
		Vec3V m_AABBCenterAbs_Total;

		const phBound * m_bound;

		typedef const phInst* key_t;

		key_t m_key;
		BVInfo * m_left;
		BVInfo * m_right;
		s8 m_balance;

		bool m_isStaticBvh;
		//bool m_inactiveCollidesAgainstInactive;

		__forceinline Mat34V_Out GetCurrentMatrix() const
		{
			return reinterpret_cast<const phInst *>(m_key)->GetMatrix();
		}
	};

	struct AvlTreeAccessor
	{
		typedef BVInfo* NID;			// Node ID type. Could be an index, pointer, etc.
		typedef BVInfo::key_t KT;		// Key type.
		typedef s8 BT;					// Balance type.

		static __forceinline NID null() { return NULL; }
		static __forceinline NID & get_left(NID & nid) { return nid->m_left; }
		static __forceinline NID & get_right(NID & nid) { return nid->m_right; }
		static __forceinline BT & get_bal(NID & nid) { return nid->m_balance; }
		static __forceinline const KT & get_key(const NID & nid) { return nid->m_key; }
		static __forceinline void set_key(NID & /*nid*/, const KT & /*key*/) { /* key already set */ }//{ nid->key = key; }
		static __forceinline int cmp(const KT & k1, const KT & k2) { return (k1 < k2); }
		static __forceinline int equ(const KT & k1, const KT & k2) { return (k1 == k2); }
		static __forceinline void prefetch_find(NID & /*nid*/) { /*PrefetchObject<PrimitiveLeaf>(nid);*/ }
		static __forceinline void prefetch_insert(NID & /*nid*/) { /*PrefetchObject<PrimitiveLeaf>(nid);*/ }
		static __forceinline void prefetch_remove(NID & /*nid*/) { /*PrefetchObject<PrimitiveLeaf>(nid);*/ }
	};

	typedef avl_tree<AvlTreeAccessor> BVCacheMap_t;

	LinearMemoryAllocator g_BVCacheAllocator;
	BVCacheMap_t g_BVCacheMap;

	BVInfo * Find(phInst * key)
	{
		return g_BVCacheMap.find(key);
	}

	BVInfo * Allocate(const phInst * key)
	{
		void * ptr = g_BVCacheAllocator.Alloc(sizeof(BVInfo),__alignof(BVInfo));
		FastAssert(ptr);
		BVInfo * cache = new(ptr) BVInfo;
		cache->m_key = key;
		g_BVCacheMap.insert(cache,key);
		return cache;
	}

	BVInfo * CreateCache(const phInst * inst, const phCollider * collider, const int levelIndex)
	{
		const Mat34V current = inst->GetMatrix();

		const phBound * bound = inst->GetArchetype()->GetBound();
		Vec3V boundHalfWidth, boundCenter;
		bound->GetBoundingBoxHalfWidthAndCenter(boundHalfWidth, boundCenter);

		Vec3V AABBHalfWidth;
		Vec3V AABBCenterAbs;

		Vec3V AABBHalfWidth_Safe;
		Vec3V AABBCenterAbs_Safe;

		Vec3V AABBHalfWidth_Total;
		Vec3V AABBCenterAbs_Total;

		if (collider)
		{
			const Mat34V last = collider->GetLastInstanceMatrix();
			AABBHalfWidth = boundHalfWidth;
			AABBCenterAbs = boundCenter;
			COT_ExpandOBBFromMotion(current, last, AABBHalfWidth, AABBCenterAbs);

			const Mat34V lastSafe = collider->GetLastSafeInstanceMatrix();
			AABBHalfWidth_Safe = boundHalfWidth;
			AABBCenterAbs_Safe = boundCenter;
			COT_ExpandOBBFromMotion(current, lastSafe, AABBHalfWidth_Safe, AABBCenterAbs_Safe);

			const Vec3V AABBMin_Total = Min(AABBCenterAbs - AABBHalfWidth, AABBCenterAbs_Safe - AABBHalfWidth_Safe);
			const Vec3V AABBMax_Total = Max(AABBCenterAbs + AABBHalfWidth, AABBCenterAbs_Safe + AABBHalfWidth_Safe);
			const ScalarV v_half(V_HALF);
			AABBHalfWidth_Total = v_half * (AABBMax_Total - AABBMin_Total);
			AABBCenterAbs_Total = v_half * (AABBMin_Total + AABBMax_Total);

			AABBCenterAbs = Transform(current,AABBCenterAbs);
			AABBCenterAbs_Safe = Transform(current,AABBCenterAbs_Safe);
			AABBCenterAbs_Total = Transform(current,AABBCenterAbs_Total);

			// Set the w components to zero for the box rejection tests.
			AABBHalfWidth.SetWZero();
			AABBHalfWidth_Safe.SetWZero();
			AABBHalfWidth_Total.SetWZero();
		}
		else
		{
			const Mat34V last = PHLEVEL->GetLastInstanceMatrix(inst);
			
			AABBHalfWidth = boundHalfWidth;
			AABBCenterAbs = boundCenter;
			COT_ExpandOBBFromMotion(current, last, AABBHalfWidth, AABBCenterAbs);
			AABBCenterAbs = Transform(current,AABBCenterAbs);
			
			// Set the w component to zero for the box rejection tests.
			AABBHalfWidth.SetWZero();

			AABBHalfWidth_Safe = AABBHalfWidth;
			AABBCenterAbs_Safe = AABBCenterAbs;

			AABBHalfWidth_Total = AABBHalfWidth;
			AABBCenterAbs_Total = AABBCenterAbs;
		}

		const bool isFixed = PHLEVEL->IsFixed(levelIndex);
		const bool isStaticBvh = (bound->GetType() == phBound::BVH) && isFixed;
		//const bool inactiveCollidesAgainstInactive = PHLEVEL->GetInactiveCollidesAgainstInactive(levelIndex);

		BVInfo * cache = Allocate(inst);
		cache->m_AABBHalfWidth = AABBHalfWidth;
		cache->m_AABBCenterAbs = AABBCenterAbs;
		cache->m_AABBHalfWidth_Safe = AABBHalfWidth_Safe;
		cache->m_AABBCenterAbs_Safe = AABBCenterAbs_Safe;
		cache->m_AABBHalfWidth_Total = AABBHalfWidth_Total;
		cache->m_AABBCenterAbs_Total = AABBCenterAbs_Total;
		cache->m_bound = bound;
		cache->m_isStaticBvh = isStaticBvh;
		//cache->m_inactiveCollidesAgainstInactive = inactiveCollidesAgainstInactive;

		return cache;
	}

	struct StaticBvhCache
	{
		enum
		{
#if __DEV
			CACHE_SIZE = 1024 * 1,
#else // __DEV
			CACHE_SIZE = 1024 * 2,
#endif // __DEV
			INVALID_INDEX = (u16)-1
		};

		struct CacheInfo
		{
			Vec4V m_sphere;	// x,y,z are position, w is the radius squared.

#if __DEV
			u16 m_levelIndexA;
			u16 m_genIdA;
			u16 m_levelIndexB;
			u16 m_genIdB;
#endif // __DEV

#if USE_STATIC_BVH_REJECTION_DEBUG
			u16 m_bpPairIndex;
			bool m_allocated;
#endif // USE_STATIC_BVH_REJECTION_DEBUG

			void Invalidate()
			{
				// Setting the radiusSq to anything less than one guarantees that the sphere test will fail.
				// This shouldn't be needed if the cache management logic is correct. We do it for extra safety. Also, it's not expensive.
				m_sphere = Vec4V(V_NEGONE);	
#if __DEV
				m_levelIndexA = INVALID_INDEX;
				m_genIdA = INVALID_INDEX;
				m_levelIndexB = INVALID_INDEX;
				m_genIdB = INVALID_INDEX;
#endif // __DEV
			}

#if __DEV
			bool Matches(const btBroadphasePair * pair) const
			{
				return (m_levelIndexA == pair->GetObject0()) && (m_genIdA == pair->GetGenId0()) && 
					   (m_levelIndexB == pair->GetObject1()) && (m_genIdB == pair->GetGenId1());
			}
#endif // __DEV
			
			Vec3V_Out GetCenter() const
			{
				return m_sphere.GetXYZ();
			}

			ScalarV_Out GetRadiusSq() const
			{
				return m_sphere.GetW();
			}

			void SetCenter(Vec3V_In center)
			{
				m_sphere.SetXYZ(center);
			}

			void SetRadiusSq(ScalarV_In radiusSq)
			{
				m_sphere.SetW(radiusSq);
			}
		};

		CacheInfo m_cache[CACHE_SIZE];
		u16 m_freeList[CACHE_SIZE];
		int m_allocCount;
		//int m_resetCount;
		StaticBvhCache()
		{
			//m_resetCount = 0;
			ResetCache();
		}

		void ResetCache()
		{
			//m_resetCount++;
			for (int i = 0 ; i < CACHE_SIZE ; i++)
			{
				CacheInfo * cache = m_cache + i;
				cache->Invalidate();
				m_freeList[i] = (u16)i;
			}
			m_allocCount = 0;
		}

#if USE_STATIC_BVH_REJECTION_DEBUG
		bool IsFree(const u16 cacheIndex) const
		{
			for (int i = m_allocCount ; i < CACHE_SIZE ;i++)
			{
				if (m_freeList[i] == cacheIndex)
					return true;
			}
			return false;
		}

		void ValidateCache()
		{
			phLevelNew* level = PHLEVEL;
			phBroadPhase* broadPhase = level->GetBroadPhase();
			btAxisSweep3 * broadPhase1 = reinterpret_cast<btAxisSweep3*>(broadPhase);
			const bool bOnlyMainThreadIsUpdatingPhysics = PHLEVEL->GetOnlyMainThreadIsUpdatingPhysics();
			if(!bOnlyMainThreadIsUpdatingPhysics)
			{
				broadPhase1->CriticalSectionLock();
			}
			else
			{
				Assertf(sysIpcGetCurrentThreadId() == broadPhase1->GetCurThreadId(), "Only the main update thread should be updating the physics broadphase now!");
			}

			for (int cache_i = 0 ; cache_i < CACHE_SIZE ; cache_i++)
			{
				CacheInfo * cache = m_cache + cache_i;
				cache->m_bpPairIndex = INVALID_INDEX;
				cache->m_allocated = true;
			}

			for (int ac_i = m_allocCount ; ac_i < CACHE_SIZE ; ac_i++)
			{
				CacheInfo * cache = m_cache + m_freeList[ac_i];
				cache->m_allocated = false;
			}

			btBroadphasePair *prunedPairs = broadPhase->getPrunedPairs();
			//const int numPrunedPairs = broadPhase1->m_NumSorted;
			const int numPrunedPairs = broadPhase->getPrunedPairCount();

			for (int pair_i = 0 ; pair_i < numPrunedPairs ; pair_i++)
			{
				btBroadphasePair * pair = prunedPairs + pair_i;
				const u16 cacheIndex = pair->GetCacheIndex();
				if (cacheIndex != INVALID_INDEX)
				{
					FastAssert(cacheIndex < CACHE_SIZE);
					CacheInfo * cache = m_cache + cacheIndex;
					FastAssert(cache->Matches(pair));
					FastAssert(cache->m_allocated);
					cache->m_bpPairIndex = (u16)pair_i;
				}
			}

			for (int cache_i = 0 ; cache_i < CACHE_SIZE ; cache_i++)
			{
				CacheInfo * cache = m_cache + cache_i;
				if (cache->m_bpPairIndex == INVALID_INDEX)
				{
					FastAssert(cache->m_allocated == false);
					FastAssert(cache->m_levelIndexA == INVALID_INDEX);
					FastAssert(cache->m_genIdA == INVALID_INDEX);
					FastAssert(cache->m_levelIndexB == INVALID_INDEX);
					FastAssert(cache->m_genIdB == INVALID_INDEX);
				}
			}

			if(!bOnlyMainThreadIsUpdatingPhysics)
			{
				broadPhase1->CriticalSectionUnlock();
			}
		}
#endif // USE_STATIC_BVH_REJECTION_DEBUG

		CacheInfo * AllocCache(btBroadphasePair * pair)
		{
			if (m_allocCount == CACHE_SIZE)
			{
				return NULL;
			}
			FastAssert(m_allocCount >= 0 && m_allocCount < CACHE_SIZE);
			const u16 cacheIndex = m_freeList[m_allocCount];
			m_allocCount++;
			pair->SetCacheIndex(cacheIndex);
			CacheInfo * cache = m_cache + cacheIndex;
#if __DEV
			cache->m_levelIndexA = pair->GetObject0();
			cache->m_genIdA = pair->GetGenId0();
			cache->m_levelIndexB = pair->GetObject1();
			cache->m_genIdB = pair->GetGenId1();
#endif // __DEV
			return cache;
		}

		bool IsFull() const
		{
			return (m_allocCount == CACHE_SIZE);
		}

		void DeleteCache(const u16 cacheIndex)
		{
			FastAssert(cacheIndex < CACHE_SIZE);
#if USE_STATIC_BVH_REJECTION_DEBUG
			FastAssert(IsFree(cacheIndex) == false);
#endif // USE_STATIC_BVH_REJECTION_DEBUG
			FastAssert(m_allocCount > 0 && m_allocCount <= CACHE_SIZE);
			m_cache[cacheIndex].Invalidate();
			m_allocCount--;
			m_freeList[m_allocCount] = cacheIndex;
		}

		CacheInfo * GetCache(btBroadphasePair * pair)
		{
			const u16 cacheIndex = pair->GetCacheIndex();
			if (cacheIndex != INVALID_INDEX)
			{
				FastAssert(cacheIndex < CACHE_SIZE);
				CacheInfo * cache = m_cache + cacheIndex;
#if __DEV
				FastAssert(cache->Matches(pair));
#endif // __DEV
				return cache;
			}
			return NULL;
		}
	};
	StaticBvhCache g_StaticBvhCache;

	void CacheProlog()	
	{
		g_BVCacheAllocator.SetBuffer(PHSIM->GetContactMgr()->GetScratchpad(),PHSIM->GetContactMgr()->GetScratchpadSize());
		g_BVCacheMap.remove_all();
#if USE_STATIC_BVH_REJECTION_DEBUG
		g_StaticBvhCache.ValidateCache();
#endif // USE_STATIC_BVH_REJECTION_DEBUG
	}

	void CacheEpilog()
	{
		PHSIM->GetContactMgr()->MarkScratchpadUsed(g_BVCacheAllocator.GetUsed());
		g_BVCacheMap.remove_all();
		g_BVCacheAllocator.NullBuffer();
#if USE_SIMULATOR_STATS1
		IncrementNumCacheEntries(g_StaticBvhCache.m_allocCount);
#endif // USE_SIMULATOR_STATS1
#if USE_STATIC_BVH_REJECTION_DEBUG
		g_StaticBvhCache.ValidateCache();
#endif // USE_STATIC_BVH_REJECTION_DEBUG
	}
};

void DeleteStaticBvhCacheEntry(const u16 cacheIndex)
{
	BVCache::g_StaticBvhCache.DeleteCache(cacheIndex);
}

#if USE_STATIC_BVH_REJECTION_DEBUG
void ValidateStaticCache()
{
	BVCache::g_StaticBvhCache.ValidateCache();
}
#endif // USE_STATIC_BVH_REJECTION_DEBUG

#endif // USE_STATIC_BVH_REJECTION

/// PHCOLLISION_USE_TASKS_ONLY(static sysSpinLockToken token;)

void phSimulator::ProcessOverlaps(phOverlappingPairArray* pairArray, btBroadphasePair* prunedPairs, u16* sortList, int numPrunedPairs, Vec::V3Param128 timeStep)
{
	NOT_EARLY_FORCE_SOLVE_ONLY(phContactMgr* contactMgr = m_ContactMgr;)
	phLevelNew* level = m_Level;
	Assert(PHLEVEL == m_Level);
	phPool<phManifold>* manifoldPool = PHMANIFOLD;

#if USE_STATIC_BVH_REJECTION
	BVCache::CacheProlog();
#endif // USE_STATIC_BVH_REJECTION

	u32 next = 0;

	for( int pairIndex = 0; pairIndex < numPrunedPairs; pairIndex++ )
	{
		next = *sortList++;
		btBroadphasePair& pair = prunedPairs[next];
		
		// Skip already-removed guys
		if( pair.GetObject1() != (u16)-1 )
		{
			if(pairArray->pairs.GetCount() >= pairArray->pairs.GetCapacity())
			{
				PF_INCREMENT(BPRejectArrayFull);
				DELETE_BP_MANIFOLD((&pair));
				continue;
			}

			phManifold *manifold = pair.GetManifold();
			u16 levelIndexA = pair.GetObject0();
			u16 levelIndexB = pair.GetObject1();

			phCollider*     colliderA     = GetCollider(levelIndexA);
			phCollider*     colliderB     = GetCollider(levelIndexB);

			if (Likely(manifold))
			{
				PrefetchObject(manifold);
			}
			if (colliderA)
			{
				PrefetchObject(colliderA);
			}
			if (colliderB)
			{
				PrefetchObject(colliderB);
			}

			// Fetch all of this information now because it all touches the same memory.
			phLevelBase::eObjectState stateA = level->GetState(levelIndexA);
			phInst* instA = level->GetInstance(levelIndexA);
			Assert(instA);
			const u32 typeFlagsA = level->GetInstanceTypeFlags(levelIndexA);
			const u32 includeFlagsA = level->GetInstanceIncludeFlags(levelIndexA);

			phLevelBase::eObjectState stateB = level->GetState(levelIndexB);
			phInst* instB = level->GetInstance(levelIndexB);
			Assert(instB);
			const u32 typeFlagsB = level->GetInstanceTypeFlags(levelIndexB);
			const u32 includeFlagsB = level->GetInstanceIncludeFlags(levelIndexB);

			Assertf(level->IsLevelIndexGenerationIDCurrent(levelIndexA,pair.GetGenId0()) && level->IsLevelIndexGenerationIDCurrent(levelIndexB,pair.GetGenId1()), "Manifold with invalid instance found in phSimulator::ProcessOverlaps. Push Iteration: %s.", (pairArray==m_OverlappingPairArray) ? "false" : "true");
			FastAssert(stateA != phLevelBase::OBJECTSTATE_NONEXISTENT && stateB != phLevelBase::OBJECTSTATE_NONEXISTENT);
/*
			if(Unlikely(stateA == phLevelBase::OBJECTSTATE_NONEXISTENT || stateB == phLevelBase::OBJECTSTATE_NONEXISTENT))
			{
				PF_INCREMENT(BPRejectNonExist);
				DELETE_BP_MANIFOLD((&pair));
				continue;
			}
*/
			FastAssert(level->ShouldCollideByState(levelIndexA,levelIndexB));
/*
			if (Unlikely(!level->ShouldCollideByState(levelIndexA,levelIndexB)))
			{
				PF_INCREMENT(BPRejectState);
				DELETE_BP_MANIFOLD((&pair));
				continue;
			}
*/
#if PROPHYLACTIC_SWAPS
			if (Likely(g_ProphylacticSwaps))
			{
				// The first condition below, (colliderB && !colliderA), is at least necessary due to 
				// a bug in code related to phTaskCollisionPair::Exchange().  If this is removed, a ragdoll 
				// colliding with the ground plane(with levelIndex 0), does not behave correctly (ragdoll 
				// gets accelerated through the ground). Exchange() seems to behave correctly when 
				// it is run on the ppu, which would point to something not getting back mm, but it 
				// all looks correct.
				if (colliderB && !colliderA ||
					(colliderA && colliderB && colliderB->IsArticulated() && !colliderA->IsArticulated()))
				{
					// Note that, strictly speaking, we should also be swapping the type and include flags of the objects here too (since we fetched them
					//   already).  However, the only place where those are used is where we match the flags below and that check is symmetric so it doesn't
					//   really matter.
					SwapEm(levelIndexA, levelIndexB);
					SwapEm(instA, instB);
					SwapEm(colliderA, colliderB);
				}
			}
#endif // PROPHYLACTIC_SWAPS

			// Match archetype type and include flags
			if(!phLevelNew::MatchFlags(typeFlagsA, includeFlagsA, typeFlagsB, includeFlagsB))
			{
				PF_INCREMENT(BPRejectFlags);
				DELETE_BP_MANIFOLD((&pair));
				continue;
			}

#if PHSIMULATOR_TEST_NEW_OBB_TEST
			char wouldFailBoxTests = 0;
#endif

#if USE_STATIC_BVH_REJECTION
#if USE_STATIC_BVH_REJECTION_SWAP
			if (g_UseStaticBvhRejection)
#endif // USE_STATIC_BVH_REJECTION_SWAP
			{
				if ((HasContacts(manifold) == false) && (instA != instB) && phSimulator::GetManifoldBoxTestEnabled())
				{
					const BVCache::BVInfo * cacheA = BVCache::Find(instA);
					if (cacheA == NULL)
					{
						cacheA = BVCache::CreateCache(instA,colliderA,levelIndexA);
					}

					const BVCache::BVInfo * cacheB = BVCache::Find(instB);
					if (cacheB == NULL)
					{
						cacheB = BVCache::CreateCache(instB,colliderB,levelIndexB);
					}

					// Swap the caches so that A is the static bvh.
					if (cacheB->m_isStaticBvh)
					{
						Assert(!cacheA->m_isStaticBvh);
						const BVCache::BVInfo * temp = cacheA;
						cacheA = cacheB;
						cacheB = temp;
					}

#if 1
					const Vec3V halfWidthA = cacheA->m_AABBHalfWidth_Total;
					const Vec3V centerA = cacheA->m_AABBCenterAbs_Total;

					const Vec3V halfWidthB = cacheB->m_AABBHalfWidth_Total;
					const Vec3V centerB = cacheB->m_AABBCenterAbs_Total;
#else
					Vec3V halfWidthA, centerA;
					if (colliderB || cacheB->m_inactiveCollidesAgainstInactive BANK_ONLY(|| !g_SweepFromSafe))
					{
						halfWidthA = cacheA->m_AABBHalfWidth;
						centerA = cacheA->m_AABBCenterAbs;
					}
					else
					{
						halfWidthA = cacheA->m_AABBHalfWidth_Safe;
						centerA = cacheA->m_AABBCenterAbs_Safe;
					}

					Vec3V halfWidthB, centerB;
					if (colliderA || cacheA->m_inactiveCollidesAgainstInactive BANK_ONLY(|| !g_SweepFromSafe))
					{
						halfWidthB = cacheB->m_AABBHalfWidth;
						centerB = cacheB->m_AABBCenterAbs;
					}
					else
					{
						halfWidthB = cacheB->m_AABBHalfWidth_Safe;
						centerB = cacheB->m_AABBCenterAbs_Safe;
					}
#endif 
					// Grab the current matrices.
					Mat34V currentA = cacheA->GetCurrentMatrix();
					Mat34V currentB = cacheB->GetCurrentMatrix();

					bool removeManifold = false;

					if (cacheA->m_isStaticBvh)
					{
						STAT1_MCS_TIMER_START(StaticBvhRejectionTimeMCS);
						STAT1_INCREMENTBY(numTestedStaticBvh,1);

						const Vec3V boxHalfWidth = COT_ACE_ExpandBoxHalfWidthForTest(halfWidthB);
						bool reCalculateSphere = false;
						BVCache::StaticBvhCache::CacheInfo * sphereCache = BVCache::g_StaticBvhCache.GetCache(&pair);
						if (sphereCache)
						{
							STAT1_INCREMENTBY(numCachedStaticBvh,1);

							const Vec3V cachedCenter = sphereCache->GetCenter();
							const ScalarV cachedRadiusSq = sphereCache->GetRadiusSq();
							
							// Determine if the box is within the sphere
							const Vec3V centerDif = UnTransform3x3Ortho(currentB,centerB - Transform(currentA,cachedCenter));
							const VecBoolV sel = IsGreaterThanOrEqual(centerDif,Vec3V(V_ZERO));
							const Vec3V extremeCorner = SelectFT(sel,-boxHalfWidth,boxHalfWidth);
							const ScalarV distSq = MagSquared(extremeCorner + centerDif);
							if (IsLessThanAll(distSq,cachedRadiusSq))
							{
								STAT1_INCREMENTBY(numRejectedStaticBvh,1);
								removeManifold = true;
							}
							else
							{
								// Only recalc if our distance from the cached center is large enough.
								const float reCalculateThresh = 2.0f;
								const float reCalculateThreshSq = reCalculateThresh * reCalculateThresh;
								if (IsGreaterThanAll(MagSquared(centerDif),ScalarVFromF32(reCalculateThreshSq)))
								{
									reCalculateSphere = true;
								}
							}
						}
						else
						{
							reCalculateSphere = (BVCache::g_StaticBvhCache.IsFull() == false);
						}
						
						if (reCalculateSphere)
						{
							STAT1_INCREMENTBY(numCalculatedStaticBvh,1);

							// Recalculate the sphere.
							const phBoundBVH * boundBvh = reinterpret_cast<const phBoundBVH *>(cacheA->m_bound);
							const Vec3V boxCenterLoc = UnTransformOrtho(currentA,centerB);
							
							STAT1_MCS_TIMER_START(CalcClosestLeafDistanceTimeMCS);
							const ScalarV sphereRadiusSq = CalcClosestLeafDistance(boxCenterLoc,boundBvh->GetBVH());
							STAT1_MCS_TIMER_STOP(CalcClosestLeafDistanceTimeMCS);

							if (!sphereCache)
							{
								sphereCache = BVCache::g_StaticBvhCache.AllocCache(&pair);
								FastAssert(sphereCache);
							}
							sphereCache->SetCenter(boxCenterLoc);
							sphereCache->SetRadiusSq(sphereRadiusSq);

							// Check if we're within the sphere.
							const ScalarV halfWidthLengthSq = MagSquared(boxHalfWidth);
							if (IsLessThanAll(halfWidthLengthSq,sphereRadiusSq))
							{
								STAT1_INCREMENTBY(numRejectedStaticBvh,1);
								removeManifold = true;
							}
						}
						STAT1_MCS_TIMER_STOP(StaticBvhRejectionTimeMCS);
					}
					else
					{
						FastAssert(IsEqualAll(halfWidthA.GetW(),ScalarV(V_ZERO)));
						FastAssert(IsEqualAll(halfWidthB.GetW(),ScalarV(V_ZERO)));
						//halfWidthA.SetWZero();	// Needed for TestBoxToBoxXXX functions.
						//halfWidthB.SetWZero();	// Needed for TestBoxToBoxXXX functions.

						currentA.SetCol3(centerA);
						currentB.SetCol3(centerB);

						// Construct a matrix which takes something in the space of A and puts it into the space of B.
						Mat34V relativeMatrix;
						UnTransformOrtho(relativeMatrix, currentB, currentA);

						if(!COT_TestBoxToBoxOBBFaces(halfWidthA, halfWidthB, relativeMatrix))
						{
							PF_INCREMENT(BPRejectBoxTest);
							removeManifold = true;
						}
					}
					if (removeManifold)
					{
						DELETE_BP_MANIFOLD((&pair));
						continue;
					}
				}
			}
#if USE_STATIC_BVH_REJECTION_SWAP
			else // if (g_UseStaticBvhRejection)
#endif // USE_STATIC_BVH_REJECTION_SWAP
#endif // USE_STATIC_BVH_REJECTION
#if !USE_STATIC_BVH_REJECTION || USE_STATIC_BVH_REJECTION_SWAP
			{
				if (manifold == NULL && instA != instB && phSimulator::GetManifoldBoxTestEnabled())
				{
#if PHSIMULATOR_USE_OBB_TEST_IN_PROCESSOVERLAPS

				// Grab the current matrices.
				Mat34V currentA = instA->GetMatrix();
				Mat34V currentB = instB->GetMatrix();

				// Grab the last matrices.
#if USE_CORRECT_EXPANDED_BOUNDING_VOLUME
				const bool InactiveCollidesAgainstInactiveA = level->GetInactiveCollidesAgainstInactive(levelIndexA);
				const bool InactiveCollidesAgainstInactiveB = level->GetInactiveCollidesAgainstInactive(levelIndexB);
				const Mat34V lastA = GetLastMatrix(instA,colliderA,level,colliderB,InactiveCollidesAgainstInactiveB);
				const Mat34V lastB = GetLastMatrix(instB,colliderB,level,colliderA,InactiveCollidesAgainstInactiveA);
#else // USE_CORRECT_EXPANDED_BOUNDING_VOLUME
				Mat34V lastA = colliderA ? colliderA->GetLastInstanceMatrix() : level->GetLastInstanceMatrix(instA);
				Mat34V lastB = colliderB ? colliderB->GetLastInstanceMatrix() : level->GetLastInstanceMatrix(instB);
#endif // USE_CORRECT_EXPANDED_BOUNDING_VOLUME

				// Compute box sizes and centers.
				// Grab half widths and centers.
				Vec3V halfWidthA, centerA;
				instA->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(halfWidthA, centerA);
				Vec3V halfWidthB, centerB;
				instB->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(halfWidthB, centerB);

				// Expand both bounding boxes for their motion and update the matrices to reflect the new centers.
				COT_ExpandOBBFromMotion(currentA, lastA, halfWidthA, centerA);
				COT_ExpandOBBFromMotion(currentB, lastB, halfWidthB, centerB);

				halfWidthA.SetWZero();	// Needed for TestBoxToBoxXXX functions.
				halfWidthB.SetWZero();	// Needed for TestBoxToBoxXXX functions.

				currentA.SetCol3(Transform(currentA, centerA));
				currentB.SetCol3(Transform(currentB, centerB));

				// Construct a matrix which takes something in the space of A and puts it into the space of B.
				Mat34V relativeMatrix;
				UnTransformOrtho(relativeMatrix, currentB, currentA);

				//if(!geomBoxes::TestBoxToBoxOBBFaces(RCC_VECTOR3(halfWidthA), RCC_VECTOR3(halfWidthB), RCC_MATRIX34(relativeMatrix)))
				if(!COT_TestBoxToBoxOBBFaces(halfWidthA, halfWidthB, relativeMatrix))
				{
#if !PHSIMULATOR_TEST_NEW_OBB_TEST
					PF_INCREMENT(BPRejectBoxTest);
					DELETE_BP_MANIFOLD((&pair));
					continue;
#else
					wouldFailBoxTests = 1;
#endif
				}

#else // PHSIMULATOR_USE_OBB_TEST_IN_PROCESSOVERLAPS
				// compute world space extents of A
				Mat34V currentA = instA->GetMatrix();
				Vec3V halfWidthA, centerA;
				instA->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(halfWidthA, centerA);
				if (colliderA)
				{
					// TODO: This isn't taking into account the safe last matrix.
					COT_ExpandOBBFromMotion(RCC_MATRIX34(currentA),RCC_MATRIX34(colliderA->GetLastInstanceMatrix()),RC_VECTOR3(halfWidthA),RC_VECTOR3(centerA));
				}
				geomBoxes::TransformAABB(RCC_MATRIX34(currentA),
					RC_VECTOR3(centerA),
					RC_VECTOR3(halfWidthA));
				Vec3V minA(centerA - halfWidthA);
				Vec3V maxA(centerA + halfWidthA);

				// compute world space extents of B
				Mat34V currentB = instB->GetMatrix();
				Vec3V halfWidthB, centerB;
				instB->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(halfWidthB, centerB);
				if (colliderB)
				{
					// TODO: This isn't taking into account the safe last matrix.
					COT_ExpandOBBFromMotion(RCC_MATRIX34(currentB),RCC_MATRIX34(colliderB->GetLastInstanceMatrix()),RC_VECTOR3(halfWidthB),RC_VECTOR3(centerB));
				}
				geomBoxes::TransformAABB(RCC_MATRIX34(currentB),
					RC_VECTOR3(centerB),
					RC_VECTOR3(halfWidthB));

				Vec3V minB(centerB - halfWidthB);
				Vec3V maxB(centerB + halfWidthB);

				// AABB test
				if (IsGreaterThanAll(minA, maxB) || IsGreaterThanAll(minB, maxA))
				{
					PF_INCREMENT(BPRejectBoxTest);

					if (pair.GetManifold())
					{
						manifoldPool->Release(pair.GetManifold());
					}

					pair.SetManifold(NULL);

					continue;
				}
#endif // PHSIMULATOR_USE_OBB_TEST_IN_PROCESSOVERLAPS
				}
			} // if (g_UseStaticBvhRejection)
#endif // !USE_STATIC_BVH_REJECTION || USE_STATIC_BVH_REJECTION_SWAP

			bool shouldFindImpacts = true;
			if (Unlikely(instA == instB))
			{
				// If we get a self collision pair that is not articulated, skip this pair. This can happen if there is a stale
				// pair from the previous frame which used to belong to an articulated body, but now belongs to a rigid one.
				shouldFindImpacts = (colliderA != NULL && colliderA->IsArticulated());
			}
			else
			{
				if (Likely(phSimulator::GetShouldFindImpactsEnabled()))
				{
					PF_START(ShouldFindImpacts);

					shouldFindImpacts = instA->ShouldFindImpacts(instB) && instB->ShouldFindImpacts(instA);

					PF_STOP(ShouldFindImpacts);
				}
			}

			if(Unlikely(!shouldFindImpacts))
			{
				// ShouldFindImpacts() said to skip the collision.
				PF_INCREMENT(BPRejectSFImpacts);
				DELETE_BP_MANIFOLD((&pair));
				continue;
			}

#if 0
			// This is some alternate code that checks for the flag before calling GetInstBehavior().
			const bool aHasInstBehavior = ((instA->GetInstFlags() & phInst::FLAG_HAS_INST_BEHAVIOR) != 0);
			const bool bHasInstBehavior = ((instB->GetInstFlags() & phInst::FLAG_HAS_INST_BEHAVIOR) != 0);
			phInstBehavior* instBehaviorA = aHasInstBehavior ? GetInstBehavior(levelIndexA, false) : NULL;
			phInstBehavior* instBehaviorB = bHasInstBehavior ? GetInstBehavior(levelIndexB, false) : NULL;
#else
			phInstBehavior* instBehaviorA = GetInstBehavior(levelIndexA, false);
			phInstBehavior* instBehaviorB = GetInstBehavior(levelIndexB, false);
			const bool aHasInstBehavior = (instBehaviorA != NULL);
			const bool bHasInstBehavior = (instBehaviorB != NULL);
#endif

			if(Unlikely((aHasInstBehavior && !instBehaviorA->CollideObjects(timeStep, instA, colliderA, instB, colliderB, instBehaviorB)) 
				|| (bHasInstBehavior && !instBehaviorB->CollideObjects(timeStep, instB, colliderB, instA, colliderA, instBehaviorA))
				))
			{
				// One of the instance behaviors handled the collision, so we don't want to go through with the normal collision process.
				PF_INCREMENT(BPRejectCollideObjects);
				DELETE_BP_MANIFOLD((&pair));
				continue;
			}

			u16 generationIdA = PHLEVEL->GetGenerationID(levelIndexA);
			u16 generationIdB = PHLEVEL->GetGenerationID(levelIndexB);

			u32 instFlagsA = instA->GetInstFlags();
			u32 instFlagsB = instB->GetInstFlags();
			u32 instFlagsCombinedOr = (instFlagsA | instFlagsB);
			FastAssert(manifold == pair.GetManifold());
			if (Unlikely(manifold == NULL))
			{
				bool highPriorityManifolds = ((instFlagsCombinedOr & phInst::FLAG_HIGH_PRIORITY) != 0);
				manifold = manifoldPool->Allocate(highPriorityManifolds);
				pair.SetManifold(manifold);

				if (Likely(manifold))
				{
					// With the new collision code we like to only initialize things once so do it here.
					manifold->SetLevelIndexA(levelIndexA, generationIdA);
					manifold->SetLevelIndexB(levelIndexB, generationIdB);
					manifold->SetColliderA(colliderA);
					manifold->SetColliderB(colliderB);
					manifold->SetInstanceA(instA);
					manifold->SetInstanceB(instB);
					manifold->SetComponentA(0);
					manifold->SetComponentB(0);
				}
			}

			if (Likely(manifold != NULL))
			{
#if !EARLY_FORCE_SOLVE
				// Save manifold if one of the bodies wants its externally controlled 
				// velocity queried, so it can be called later.
				if(Unlikely((instFlagsCombinedOr & phInst::FLAG_QUERY_EXTERN_VEL) != 0))
				{
					atArray<phManifold*>& externVels = contactMgr->GetManifoldsWithExternVelocity();
					if (Verifyf(externVels.GetCount() < externVels.GetCapacity(), "Extern velocity manifolds array is full (%d items)", externVels.GetCapacity()))
					{
						externVels.Push(manifold);
					}
				}
#endif // !EARLY_FORCE_SOLVE

				// I'm not entirely clear on how this can happen, but apparently it can.  And even if it can happen, I'm not entirely clear that all four of the
				//   conditions in "instancesMatch" are really necessary, I believe that just the instance pointer checks matter (I think the level indices *have*
				//   to match if we're here).
				const bool instancesMatch = (manifold->GetLevelIndexA() == levelIndexA) & (manifold->GetInstanceA() == instA) & (manifold->GetLevelIndexB() == levelIndexB) & (manifold->GetInstanceB() == instB);
				if(!Verifyf(instancesMatch, "Manifold instances (%i, %i) don't match broadphase pair instances (%i, %i), removing contacts.",manifold->GetLevelIndexA(),manifold->GetLevelIndexB(),levelIndexA,levelIndexB))
				{
					manifold->RemoveAllContacts();
				}

				// It's perfectly legitimate for the colliders to not match.  This can happen if an object is deactivated and then re-activated immediately.  Any
				//   pairs that the object was part of will stay 'valid' and therefore never get cleared and any manifold that the pair owned (and the contacts or
				//   sub-manifolds that it contains) will persist but the collider pointer(s) might have changed.  If we find that to be the case we'll fix it here.
				const bool collidersMatch = (manifold->GetColliderA() == colliderA) & (manifold->GetColliderB() == colliderB);

				// To avoid having two loops over the same memory, if we find that either data is out of date we'll just set it all.
				if(Unlikely(!collidersMatch || !instancesMatch))
				{
					if(manifold->CompositeManifoldsEnabled())
					{
						const int numCompositeManifolds = manifold->GetNumCompositeManifolds();
						for (int manifoldIndex = 0; manifoldIndex < numCompositeManifolds; ++manifoldIndex)
						{
							phManifold* pCompositeCollidingManifold = manifold->GetCompositeManifold(manifoldIndex);
							Assert(pCompositeCollidingManifold != NULL);
							pCompositeCollidingManifold->SetColliderA(colliderA);
							pCompositeCollidingManifold->SetColliderB(colliderB);
							pCompositeCollidingManifold->SetInstanceA(instA);
							pCompositeCollidingManifold->SetInstanceB(instB);
							pCompositeCollidingManifold->SetLevelIndexA(levelIndexA, generationIdA);
							pCompositeCollidingManifold->SetLevelIndexB(levelIndexB, generationIdB);
						}
					}

					manifold->SetColliderA(colliderA);
					manifold->SetColliderB(colliderB);
					manifold->SetInstanceA(instA);
					manifold->SetInstanceB(instB);
					manifold->SetLevelIndexA(levelIndexA, generationIdA);
					manifold->SetLevelIndexB(levelIndexB, generationIdB);
				}

#if __ASSERT
				
				// If any of these fail then something has slipped through the cracks above.
#define VALIDATE_MANIFOLD_1(manifold_) FastAssert(manifold_); \
				if ((manifold_->GetLevelIndexA() != levelIndexA)	 || (manifold_->GetLevelIndexB() != levelIndexB) || \
					(manifold_->GetInstanceA() != instA)			 || (manifold_->GetInstanceB() != instB) || \
					(manifold_->GetColliderA() != colliderA)		 || (manifold_->GetColliderB() != colliderB)) \
				{ \
					Displayf("Invalid manifold in broad phase list! 0x%p %d",manifold_,*(sortList-1)); \
					__debugbreak(); \
				} 
				// Make checking generation id skipable for now.
#define VALIDATE_MANIFOLD_2(manifold_) Assert((manifold_->GetGenerationIdA() == generationIdA) && (manifold_->GetGenerationIdB() == generationIdB))
				
				VALIDATE_MANIFOLD_1(manifold);
				VALIDATE_MANIFOLD_2(manifold);
				if (manifold->CompositeManifoldsEnabled())
				{
					const int numCompositeManifolds = manifold->GetNumCompositeManifolds();
					for (int manifoldIndex = 0; manifoldIndex < numCompositeManifolds; ++manifoldIndex)
					{
						phManifold* compositeManifold = manifold->GetCompositeManifold(manifoldIndex);
						VALIDATE_MANIFOLD_1(compositeManifold);
						VALIDATE_MANIFOLD_2(compositeManifold);
					}
				}

#endif // __ASSERT

				manifold->m_InactiveCollidesAgainstInactiveA = level->GetInactiveCollidesAgainstInactive(levelIndexA);
				manifold->m_InactiveCollidesAgainstInactiveB = level->GetInactiveCollidesAgainstInactive(levelIndexB);

				//CollisionIntersection(inst1, inst2);
				Assertf((instA->GetArchetype()->GetBound()->GetType() != phBound::BVH) || (instB->GetArchetype()->GetBound()->GetType() != phBound::BVH), "%p (%s) %p (%s) - BVH to BVH collision not supported", instA, instA->GetArchetype()->GetFilename(), instB, instB->GetArchetype()->GetFilename());

				TrapGE(pairArray->pairs.GetCount(), pairArray->pairs.GetCapacity());
				phTaskCollisionPair& overlappingPair = pairArray->pairs.Append();
#if PHSIMULATOR_TEST_NEW_OBB_TEST
				overlappingPair.pad[0] = wouldFailBoxTests;
#endif
				overlappingPair.levelIndex1 = levelIndexA;
				overlappingPair.generationId1 = generationIdA;
				overlappingPair.levelIndex2 = levelIndexB;
				overlappingPair.generationId2 = generationIdB;
				overlappingPair.manifold = manifold;
				sys_lwsync();
				pairArray->numCollisionPairs++;
#if PH_MANIFOLD_TRACK_IS_IN_OPA
				manifold->SetIsInOPA(true);
#endif // PH_MANIFOLD_TRACK_IS_IN_OPA

				// If we add a new pair to a push pair OPA make sure it's in the simulator's OPA
				// Pushes can generate contacts between two objects that didn't have any before. If we
				//   don't have that manifold in the simulator OPA then we won't be able to find the 
				//   manifold when the user wants to transfer/delete contacts with an instance. 
				// NOTE: Since the broadphase can get duplicates during activations it's possible for this to add the same
				//         pair to the simulator OPA. 
				if(pairArray != m_OverlappingPairArray)
				{
					if(Verifyf(m_OverlappingPairArray->pairs.GetCount() < m_OverlappingPairArray->pairs.GetCapacity(),"Ran out of collision pairs in the phSimulator overlapping pair array. Max %i.",m_OverlappingPairArray->pairs.GetCapacity()))
					{
						m_OverlappingPairArray->pairs.Append() = overlappingPair;
						sys_lwsync();
						m_OverlappingPairArray->numCollisionPairs++;
					}
				}

				// TODO: do we need to insert articulated bodies into the pair array here? We do that for the old solver...
				bool atLeastOneInactive = stateA == phLevelBase::OBJECTSTATE_INACTIVE || stateB == phLevelBase::OBJECTSTATE_INACTIVE;
				if (atLeastOneInactive)
				{
					AddActivatingPair( &overlappingPair );
				}

#if __ASSERT
				if (Unlikely(instA == instB))
				{
					phCollider* collider = GetCollider(levelIndexA);

					Assert(!collider || collider->IsArticulated());
				}
#endif

				//else 
				//	return true;	// this removes us from the pair list

				//	Warningf("%d and %d overlap\n",instA->GetLevelIndex(), instB->GetLevelIndex() );
				//	((TPhysicsHandle*)(pair.m_pProxy1->m_clientObject))->GetClient()->getName() )

			}

			PF_INCREMENT(BPAccept);
		}
		else
		{
			Assert(pair.GetManifold() == NULL);
		}
	}

#if USE_STATIC_BVH_REJECTION
	BVCache::CacheEpilog();
#endif // USE_STATIC_BVH_REJECTION
}

class PairCostPredicate : public std::binary_function<u16, u16, bool>
{
	btBroadphasePair* m_Pairs;

public:
	PairCostPredicate(btBroadphasePair* pairs)
		: m_Pairs(pairs)
	{ }

	bool operator()(u16 left, u16 right) const
	{
		const btBroadphasePair& leftPair = m_Pairs[left];
		phManifold* leftManifold = leftPair.GetManifold();
		if (leftManifold == NULL)
		{
			return false;
		}

		const btBroadphasePair& rightPair = m_Pairs[right];
		phManifold* rightManifold = rightPair.GetManifold();
		if (rightManifold == NULL)
		{
			return true;
		}

		return leftManifold->GetCollisionTime() > rightManifold->GetCollisionTime();
	}
};

#if CHECK_FOR_DUPLICATE_MANIFOLDS

PARAM(nomanifoldcheck, "[physics] Don't do manifold error checking.");

typedef BitVectorFixedSize<1024*30> BigBitVector;

template <class T> void VerifyMemoryPool(phPool<T> * memoryPool, BigBitVector * bitVector)
{
	FastAssert(memoryPool->GetAvailCount() + memoryPool->GetInUseCount() <= (int)bitVector->GetBitVectorSize());
	const int availCount = memoryPool->GetAvailCount();
	T ** pool = memoryPool->GetPool();
	//PrefetchBuffer<sizeof(T*)*2200>(pool);
	bitVector->SetAll(false);
	for (int i = 0 ; i < availCount ; i++)
	{
		T * object = pool[i];
		const u32 index = memoryPool->GetIndex(object);
		const u32 prevBit = bitVector->SetBit(index,true);
		if (Unlikely(prevBit != 0))
		{
			Displayf("Duplicate slot found in pool list! 0x%p %d",object,i);
			__debugbreak();
		}
		//memoryPool->VerifyObject(object);
#if PHPOOL_EXTRA_VERIFICATION
		const u32 allocated = memoryPool->GetAllocated(object);
		if (Unlikely(allocated))
		{
			Displayf("Unallocated slot marked as allocated! 0x%p %d",object,i);
			__debugbreak();
		}
#endif // PHPOOL_EXTRA_VERIFICATION
	}
}

void VerifyPools(BigBitVector * bitVector)
{
	VerifyMemoryPool(s_SimInstance->GetManifoldPool(),bitVector);
	//VerifyMemoryPool(s_SimInstance->GetCompositePointerPool(),bitVector);
	//VerifyMemoryPool(s_SimInstance->GetContactPool(),bitVector);
}

__forceinline u32 CheckManifold(const phManifold * manifold, BigBitVector * bitVector)
{
	phPool<phManifold> * pool = s_SimInstance->GetManifoldPool();
	//pool->VerifyObject(manifold);
#if PHPOOL_EXTRA_VERIFICATION
	const u32 allocated = pool->GetAllocated(manifold);
	if (Unlikely(!allocated))
	{
		Displayf("Unallocated manifold slot still being used! 0x%p",manifold);
		__debugbreak();
	}
#endif // PHPOOL_EXTRA_VERIFICATION
	const u32 index = pool->GetIndex(manifold);
	return bitVector->SetBit(index,true);
}

__forceinline u32 CheckCompositePointers(const phCompositePointers * CompositePointers, BigBitVector * bitVector)
{
	phPool<phCompositePointers> * pool = s_SimInstance->GetCompositePointerPool();
	//pool->VerifyObject(manifold);
#if PHPOOL_EXTRA_VERIFICATION
	const u32 allocated = pool->GetAllocated(CompositePointers);
	if (Unlikely(!allocated))
	{
		Displayf("Unallocated composite pointer slot still being used! 0x%p",CompositePointers);
		__debugbreak();
	}
#endif // PHPOOL_EXTRA_VERIFICATION
	const u32 index = pool->GetIndex(CompositePointers);
	return bitVector->SetBit(index,true);
}

bool FindManifold(phManifold * manifoldToFind, phOverlappingPairArray * pairArray, int * arrIndex, int * compIndex)
{
	FastAssert(arrIndex && compIndex && manifoldToFind);
	for (int pair_i = 0 ; pair_i < pairArray->pairs.GetCount() ; pair_i++)
	{
		phTaskCollisionPair * pair = &pairArray->pairs[pair_i];
		phManifold * manifold = pair->manifold;
		if (manifold)
		{
			if (manifold == manifoldToFind)
			{
				*arrIndex = pair_i;
				*compIndex = -1;
				return true;
			}
			if (manifold->CompositeManifoldsEnabled())
			{
				for (int comp_i = 0 ; comp_i < manifold->GetNumCompositeManifolds() ; comp_i++)
				{
					phManifold * compositeManifold = manifold->GetCompositeManifold(comp_i);
					if (compositeManifold == manifoldToFind)
					{
						*arrIndex = pair_i;
						*compIndex = comp_i;
						return true;
					}
				}
			}
		}
	}
	*arrIndex = -1;
	*compIndex = -1;
	return false;
}

bool FindManifold(phManifold * manifoldToFind, btBroadphasePair * pairArray, const int pairArrayCount, int * arrIndex, int * compIndex)
{
	FastAssert(arrIndex && compIndex && manifoldToFind);
	for (int pair_i = 0 ; pair_i < pairArrayCount ; pair_i++)
	{
		phManifold * manifold = pairArray[pair_i].GetManifold();
		if (manifold)
		{
			if (manifold == manifoldToFind)
			{
				*arrIndex = pair_i;
				*compIndex = -1;
				return true;
			}
			if (manifold->CompositeManifoldsEnabled())
			{
				for (int comp_i = 0 ; comp_i < manifold->GetNumCompositeManifolds() ; comp_i++)
				{
					phManifold * compositeManifold = manifold->GetCompositeManifold(comp_i);
					if (compositeManifold == manifoldToFind)
					{
						*arrIndex = pair_i;
						*compIndex = comp_i;
						return true;
					}
				}
			}
		}
	}
	*arrIndex = -1;
	*compIndex = -1;
	return false;
}

void VerifyCompositeManifold(phManifold * rootManifold, phManifold * compositeManifold)
{
	(void)rootManifold;
	(void)compositeManifold;
	Assert(rootManifold->GetGenerationIdA() == compositeManifold->GetGenerationIdA());
	Assert(rootManifold->GetGenerationIdB() == compositeManifold->GetGenerationIdB());
	FastAssert(rootManifold->GetLevelIndexA() == compositeManifold->GetLevelIndexA());
	FastAssert(rootManifold->GetLevelIndexB() == compositeManifold->GetLevelIndexB());
}

enum DuplicationType_e
{
	DUPLICATION_TYPE_NONE = 0,
	DUPLICATION_TYPE_ROOT_MANIFOLD = 1,
	DUPLICATION_TYPE_COMPOSITE_MANIFOLD = 2,
	DUPLICATION_TYPE_COMPOSITE_POINTERS = 3,
};

void DuplicateManifoldCheckProcess(phBroadPhase * broadPhase, phManifold * manifold, BigBitVector * manifoldBitVector, BigBitVector * compositeBitVector)
{
	int arrIndex;
	int compIndex;
	bool findResult;
	if (Unlikely(CheckManifold(manifold,manifoldBitVector) != 0))
	{
		findResult = FindManifold(manifold,broadPhase->getPrunedPairs(),broadPhase->getPrunedPairCount(),&arrIndex,&compIndex);
		Displayf("BP Duplicate root manifolds found! 0x%p %d %d",manifold,arrIndex,compIndex);
		if (findResult)
		{
			Displayf("For real!");
			__debugbreak();
		}
	}
	if (manifold->GetCompositePointers())
	{
		if (Unlikely(CheckCompositePointers(manifold->GetCompositePointers(),compositeBitVector) != 0))
		{
			Displayf("BP Duplicate composite pointers found! 0x%p",manifold);
			__debugbreak();
		}
		for (int i = 0 ; i < manifold->GetNumCompositeManifolds() ; i++)
		{
			phManifold * compositeManifold = manifold->GetCompositeManifold(i);
			VerifyCompositeManifold(manifold,compositeManifold);
			if (Unlikely(CheckManifold(compositeManifold,manifoldBitVector) != 0))
			{
				findResult = FindManifold(manifold,broadPhase->getPrunedPairs(),broadPhase->getPrunedPairCount(),&arrIndex,&compIndex);
				Displayf("BP Duplicate composite manifolds found! 0x%p %d %d",compositeManifold,arrIndex,compIndex);
				if (findResult)
				{
					Displayf("For real!");
					__debugbreak();
				}
			}
		}
	}
}

void DuplicateManifoldCheckProcess(phOverlappingPairArray * pairArray, phManifold * manifold, BigBitVector * manifoldBitVector, BigBitVector * compositeBitVector)
{
	int arrIndex;
	int compIndex;
	bool findResult;
	if (Unlikely(CheckManifold(manifold,manifoldBitVector) != 0))
	{
		findResult = FindManifold(manifold,pairArray,&arrIndex,&compIndex);
		Displayf("Duplicate root manifolds found! 0x%p %d %d",manifold,arrIndex,compIndex);
		if (findResult)
		{
			Displayf("For real!");
			__debugbreak();
		}
	}
	if (manifold->GetCompositePointers())
	{
		if (Unlikely(CheckCompositePointers(manifold->GetCompositePointers(),compositeBitVector) != 0))
		{
			Displayf("Duplicate composite pointers found! 0x%p",manifold);
			__debugbreak();
		}
		for (int i = 0 ; i < manifold->GetNumCompositeManifolds() ; i++)
		{
			phManifold * compositeManifold = manifold->GetCompositeManifold(i);
			VerifyCompositeManifold(manifold,compositeManifold);
			if (Unlikely(CheckManifold(compositeManifold,manifoldBitVector) != 0))
			{
				findResult = FindManifold(manifold,pairArray,&arrIndex,&compIndex);
				Displayf("Duplicate composite manifolds found! 0x%p %d %d",compositeManifold,arrIndex,compIndex);
				if (findResult)
				{
					Displayf("For real!");
					__debugbreak();
				}
			}
		}
	}
}

void CheckForDuplicateManifolds(phBroadPhase * broadPhase)
{
	if (!PARAM_nomanifoldcheck.Get())
	{
		btBroadphasePair * prunedPairs = broadPhase->getPrunedPairs();
		const int numPrunedPairs = broadPhase->getPrunedPairCount();

		BigBitVector manifoldBitVector;
		BigBitVector compositeBitVector;
		//PrefetchBuffer<sizeof(BigBitVector)>(&bitVector);
		//VerifyPools(&bitVector);
		manifoldBitVector.SetAll(false);
		compositeBitVector.SetAll(false);
		for (int i = 0 ; i < numPrunedPairs ; i++)
		{
			phManifold * manifold = prunedPairs[i].GetManifold();
			if (manifold)
				DuplicateManifoldCheckProcess(broadPhase,manifold,&manifoldBitVector,&compositeBitVector);
		}
	}
}

void CheckForDuplicateManifolds(phOverlappingPairArray * pairArray)
{
	if (!PARAM_nomanifoldcheck.Get())
	{
		BigBitVector manifoldBitVector;
		BigBitVector compositeBitVector;
		//PrefetchBuffer<sizeof(BigBitVector)>(&bitVector);
		//VerifyPools(&bitVector);
		manifoldBitVector.SetAll(false);
		compositeBitVector.SetAll(false);
		for (int i = 0 ; i < pairArray->pairs.GetCount() ; i++)
		{
			phTaskCollisionPair * pair = &pairArray->pairs[i];
			phManifold * manifold = pair->manifold;
			if (manifold)
				DuplicateManifoldCheckProcess(pairArray,manifold,&manifoldBitVector,&compositeBitVector);
		}
	}
}

#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

#if __DEV
void ManifoldReferenceCheck(const phManifold* pManifold, const phInst* pInst, bool allowEmptyManifolds, bool allowDeactivatedConstraints)
{
	bool foundReference = false;
	if(pManifold && pInst)
	{
		if(pManifold->GetInstanceA() == pInst || pManifold->GetInstanceB() == pInst)
		{
			if(allowEmptyManifolds)
			{
				if(pManifold->CompositeManifoldsEnabled())
				{
					// Look for a sub-manifold with contacts
					for(int compositeManifoldIndex = 0; compositeManifoldIndex < pManifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
					{
						if(phManifold* pCompositeManifold = pManifold->GetCompositeManifold(compositeManifoldIndex))
						{
							if(pCompositeManifold->GetNumContacts() > 0)
							{
								foundReference = true;
							}
						}
					}
				}
				else if(pManifold->GetNumContacts() > 0)
				{
					if(pManifold->IsConstraint())
					{
						// Constraints get turned off by deactivation
						Assert(pManifold->GetNumContacts() == 1);
						if(pManifold->GetContactPoint(0).IsContactActive() || !allowDeactivatedConstraints)
						{
							foundReference = true;
						}
					}
					else
					{
						foundReference = true;
					}
				}
			}
			else
			{
				foundReference = true;
			}

			if(foundReference)
			{
				const phInst* pOtherInst = (pManifold->GetInstanceA() == pInst) ? pManifold->GetInstanceB() : pManifold->GetInstanceA();
				Assertf(!foundReference,	"Found a manifold referencing a phInst that it shouldn't:"
											"\n\tpInst: '%s'"
											"\n\tpOtherInst: '%s'"
											"\n\tManifold Type: '%s'",
											pInst->GetArchetype()->GetFilename(),
											pOtherInst ? pOtherInst->GetArchetype()->GetFilename() : "(NULL)",
											pManifold->CompositeManifoldsEnabled() ? "Composite" : (pManifold->IsConstraint() ? "Constraint" : "Basic"));
			}
		}
	}
}

void phSimulator::CheckForManifoldReferencesToInst(const phInst* pInst, bool allowEmptyManifolds, bool allowDeactivatedConstraints)
{
	phBroadPhase* pBroadPhase = m_Level->GetBroadPhase();
	for (int i = 0 ; i < pBroadPhase->getPrunedPairCount() ; i++)
	{
		ManifoldReferenceCheck(pBroadPhase->getPrunedPairs()[i].GetManifold(),pInst,allowEmptyManifolds,allowDeactivatedConstraints);
	}
}

void DumpManifoldHelper(const phManifold* pManifold, int myObjectIndex, int otherObjectIndex, bool isArticulatedFixedCollision)
{
	Displayf("\t\tComponent: %i, %i",pManifold->GetComponent(myObjectIndex), pManifold->GetComponent(otherObjectIndex));
	Displayf("\t\tMass Inv Scale: %f, %f",pManifold->GetMassInvScale(myObjectIndex), pManifold->GetMassInvScale(otherObjectIndex));
	for(int contactIndex = 0; contactIndex < pManifold->GetNumContacts(); ++contactIndex)
	{
		Displayf("\t\tContact %i:",contactIndex);
		const phContact& contact = pManifold->GetContactPoint(contactIndex);
		Displayf("\t\t\tNormal: <%f, %f, %f>", VEC3V_ARGS(contact.GetWorldNormal()));
		Displayf("\t\t\tPos: <%f, %f, %f>, <%f, %f, %f>", VEC3V_ARGS(contact.GetWorldPos(myObjectIndex)), VEC3V_ARGS(contact.GetWorldPos(otherObjectIndex)));
		Displayf("\t\t\tTarget Rel Vel: <%f, %f, %f>", VEC3V_ARGS(contact.GetTargetRelVelocity()));
		const Vec3V impulse = isArticulatedFixedCollision ? contact.ComputeTotalArtFixImpulse() : contact.ComputeTotalImpulse();
		Displayf("\t\t\tImpulse: <%f, %f, %f>", VEC3V_ARGS(impulse));
		Displayf("\t\t\tDepth: %f",contact.GetDepth());
		Displayf("\t\t\tFriction: %f", contact.GetFriction());
		Displayf("\t\t\tElasticity: %f", contact.GetElasticity());
		Displayf("\t\t\tActive: %s", contact.IsContactActive() ? "yes" : "no");
		Displayf("\t\t\tLifetime: %i",contact.GetLifetime());
	}
}

void phSimulator::DumpManifoldsWithInstance(const phInst* pInst)
{
	Displayf("*** Dumping manifolds with instance '%s' 0x%p (0x%p) ***",pInst->GetArchetype()->GetFilename(), pInst, pInst->GetUserData());
	phBroadPhase* pBroadPhase = m_Level->GetBroadPhase();
	for (int i = 0 ; i < pBroadPhase->getPrunedPairCount() ; i++)
	{
		if(const phManifold* pManifold = pBroadPhase->getPrunedPairs()[i].GetManifold())
		{
			if(pManifold->ObjectsInContact())
			{
				if(pManifold->GetInstanceA() == pInst || pManifold->GetInstanceB() == pInst)
				{
					int myObjectIndex = (pManifold->GetInstanceA() == pInst) ? 0 : 1;
					int otherObjectIndex = 1 - myObjectIndex;
					const phInst* pOtherInstance = pManifold->GetInstance(otherObjectIndex);
					const phCollider* pOtherCollider = pManifold->GetCollider(otherObjectIndex);
					bool isArticulatedFixedCollision = pManifold->IsArticulatedFixedCollision();
					Displayf("Manifold with '%s' 0x%p (0x%p)",pOtherInstance ? pOtherInstance->GetArchetype()->GetFilename() : NULL, pOtherInstance, pOtherInstance ? pOtherInstance->GetUserData() : NULL);
					Displayf("\tOther Collider: 0x%p",pOtherCollider);
					if(pOtherCollider)
					{
						Displayf("\t\tVelocity: <%f, %f, %f>",VEC3V_ARGS(pOtherCollider->GetVelocity()));
						Displayf("\t\tAng Velocity: <%f, %f, %f>",VEC3V_ARGS(pOtherCollider->GetAngVelocity()));
						Displayf("\t\tMass: %f",pOtherCollider->GetMass());
						Displayf("\t\tAng Inertia: <%f, %f, %f>",VEC3V_ARGS(pOtherCollider->GetAngInertia()));
						Displayf("\t\tArticulated: %s",pOtherCollider->IsArticulated() ? "yes" : "no");
					}
					if(pManifold->CompositeManifoldsEnabled())
					{
						for(int compositeManifoldIndex = 0; compositeManifoldIndex < pManifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
						{
							Displayf("\tComposite Manifold %i:",compositeManifoldIndex);
							DumpManifoldHelper(pManifold->GetCompositeManifold(compositeManifoldIndex),myObjectIndex,otherObjectIndex,isArticulatedFixedCollision);
						}
					}
					else
					{
						DumpManifoldHelper(pManifold,myObjectIndex,otherObjectIndex,isArticulatedFixedCollision);
					}
				}
			}
		}
	}
}
#endif // __DEV

#if __BANK
static phLevelNew::BroadPhaseType sm_BroadPhaseType = phLevelNew::Broadphase_Count;
static bool s_DumpOverlappingPairs = false;
#endif

#if USER_JBN
	#define CAPTURE_COLLISION_FLUSH 1
#else
	#define CAPTURE_COLLISION_FLUSH 0
#endif

static u32 s_LastFrameMostExpensivePair;

static const u32 BUCKET_BITS = 3;
static const u16 NUM_PAIR_SORT_BUCKETS = 1 << BUCKET_BITS;

// How many pairs in each bucket?
static u16 s_PairsBuckets[2][NUM_PAIR_SORT_BUCKETS];

// Alternates between zero and one to select from the two s_PairsBuckets arrays
static int s_CurrentFrame = 0;

void SortPairsByCost(u16* sortList, btBroadphasePair* prunedPairs, const int numPrunedPairs)
{
	if (numPrunedPairs == 0)
	{
		return;
	}

	// Select which pair array we're using this frame
	int lastFrame = s_CurrentFrame;
	int currentFrame = 1 - s_CurrentFrame;
	s_CurrentFrame = currentFrame;

	u16* currentPairsBuckets = s_PairsBuckets[currentFrame];
	u16* lastPairsBuckets = s_PairsBuckets[lastFrame];

	// Compute the starting index for each bucket
	u16 bucketOffset = 0;
	u16 bucketStarts[NUM_PAIR_SORT_BUCKETS];
	for (int bucketIndex = 0; bucketIndex < NUM_PAIR_SORT_BUCKETS; ++bucketIndex)
	{
		currentPairsBuckets[bucketIndex] = 0;
		u16 nextBucketCount = lastPairsBuckets[bucketIndex];
		bucketStarts[bucketIndex] = bucketOffset;
		bucketOffset += nextBucketCount;
	}

	// The total of all the lastBucketPairs has to be equal to numPrunedPairs, so adjust the last few buckets so the totals match.
	// If we were to recompute bucketOffset at this point, we would come up with exactly numPrunedPairs. This is critical so that once
	// we have handed out all the indices, all the spaces are filled with no gaps left.
	if (bucketOffset < numPrunedPairs)
	{
		lastPairsBuckets[NUM_PAIR_SORT_BUCKETS - 1] += u16(numPrunedPairs - bucketOffset);
	}
	else
	{
		int endBucket = NUM_PAIR_SORT_BUCKETS - 1;
		while (bucketOffset - numPrunedPairs > lastPairsBuckets[endBucket])
		{
			TrapLE(endBucket, 0);
			TrapGE(endBucket, NUM_PAIR_SORT_BUCKETS);
			bucketOffset -= lastPairsBuckets[endBucket];
			lastPairsBuckets[endBucket--] = 0;
		}

		TrapLT(endBucket, 0);
		TrapGE(endBucket, NUM_PAIR_SORT_BUCKETS);
		lastPairsBuckets[endBucket] -= u16(bucketOffset - numPrunedPairs);
	}

	// For computing the most expensive pair for the next time through
	u32 newMostExpensivePair = 0;

	// To avoid having to search each time a bucket comes up empty, remember where we found a full one for each bucket
	u32 bucketReassignment[NUM_PAIR_SORT_BUCKETS] = { 0, 1, 2, 3, 4, 5, 6, 7 };

	for (u16 pairIndex = 0; pairIndex < numPrunedPairs; ++pairIndex)
	{
		if (Likely(pairIndex + 1 < numPrunedPairs))
		{
			PrefetchDC(&prunedPairs[pairIndex + 1].GetManifold()->GetCollisionTimeRef());
		}
		
		// Compute the bucket where we should go
		u16 bucket = NUM_PAIR_SORT_BUCKETS - 1;
		if (phManifold* manifold = prunedPairs[pairIndex].GetManifold())
		{
			const u32 pairTime = manifold->GetCollisionTime();
			u32 normalizedTime = (pairTime << BUCKET_BITS) / (s_LastFrameMostExpensivePair + 1);
			normalizedTime = Min(normalizedTime, u32(NUM_PAIR_SORT_BUCKETS - 1));
			bucket = u16(NUM_PAIR_SORT_BUCKETS - 1 - normalizedTime);

			newMostExpensivePair = Max(newMostExpensivePair, pairTime);
		}

		// Remember for next frame where we would have liked to go
		TrapGE(bucket, NUM_PAIR_SORT_BUCKETS);
		currentPairsBuckets[bucket]++;

		// Try to go where we prefer, but if we don't have room, find a different bucket and remember it
		u32 actualBucket = bucketReassignment[bucket];
		TrapGE(actualBucket, u32(NUM_PAIR_SORT_BUCKETS));
		while (lastPairsBuckets[actualBucket] == 0)
		{
			actualBucket = (actualBucket + 1) & (NUM_PAIR_SORT_BUCKETS - 1);
		}
		bucketReassignment[bucket] = actualBucket;

		// Subtract ourselves from the bucket that we went in to
		TrapGE(actualBucket, u32(NUM_PAIR_SORT_BUCKETS));
		TrapLE(lastPairsBuckets[actualBucket], 0);
		lastPairsBuckets[actualBucket]--;

		// Finally, put ourselves in the sorted list
		sortList[bucketStarts[actualBucket]++] = pairIndex;
	}

	s_LastFrameMostExpensivePair = newMostExpensivePair;
}


int phSimulator::CollideActive (phOverlappingPairArray* pairArray, u32 startingBroadphasePairIndex, float timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::CollideActive");
	PF_FUNC(CollideActive);

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	PF_START(CollideActiveDOU);
	// Commit any changes that are queued up from client code moving instances.  The main reason why this matters is so that activations that occur due
	//   to collisions can prime the overlapping pair array properly (we perform a level cull to find the initial set of overlapping objects).
	// This one is not optional (controlled by a #define) because we need to ensure that the physics level is up-to-date each and every time we perform
	//   collision activations (which can happen after each collision detection pass).
	InitiateCommitDeferredOctreeUpdatesTask();
	PF_STOP(CollideActiveDOU);
#endif // LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

    pairArray->allPairsReady = false;

	NOT_EARLY_FORCE_SOLVE_ONLY(m_NumActivatingPairs = 0);

	phLevelNew* level = m_Level;
	phBroadPhase* broadPhase = level->GetBroadPhase();
#if USE_PRUNE_NEW_PAIRS
	STAT1_MCS_TIMER_START(PruneNewPairsTimeMCS);
	broadPhase->PruneNewOverlappingPairs(startingBroadphasePairIndex);
	STAT1_MCS_TIMER_STOP(PruneNewPairsTimeMCS);
#endif // USE_PRUNE_NEW_PAIRS
	Assert((int)startingBroadphasePairIndex <= broadPhase->getPrunedPairCount());
	btBroadphasePair *prunedPairs = broadPhase->getPrunedPairs() + startingBroadphasePairIndex;
	const int numPrunedPairs = broadPhase->getPrunedPairCount() - startingBroadphasePairIndex;
	// Return the next broad phase pair index that needs to be processed. This needs to be here because the octree update can create new pairs.
	const int nextStartingBroadphasePair = broadPhase->getPrunedPairCount();
	Assert(numPrunedPairs <= (int)broadPhase->m_pairCache->GetMaxPairs());
	u16* sortList = Alloca(u16, numPrunedPairs);

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(broadPhase);
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Sort pairs by cost");
	PF_START(SortPairsByCost);

	if (sm_SortPairsByCost && startingBroadphasePairIndex == 0)
	{
		SortPairsByCost(sortList, prunedPairs, numPrunedPairs);
	}
	else
	{
		for (u16 index = 0; index < numPrunedPairs; ++index)
		{
			sortList[index] = index;
		}
	}

	PF_STOP(SortPairsByCost);

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Process overlaps");
	PF_START(ProcessOverlap);

	s_SortInstanceBehaviorsToken.Lock();

	PF_INCREMENTBY(BroadphasePairs, numPrunedPairs);

#if __BANK
	if (s_DumpOverlappingPairs)
	{
		s_DumpOverlappingPairs = false;

		Displayf("Physics: Overlapping pairs (%d)", numPrunedPairs);

		for( int iPrune = 0; iPrune < numPrunedPairs; iPrune++ )
		{
			btBroadphasePair *pair = prunedPairs + sortList[iPrune];

			int object0 = pair->GetObject0();
			const char* name0 = "(invalid)";
			if (PHLEVEL->LegitLevelIndex(object0))
			{
				if (phInst* inst = PHLEVEL->GetInstance(object0))
				{
					if (phArchetype* arch = inst->GetArchetype())
					{
						name0 = arch->GetFilename();
					}
				}
			}

			int object1 = pair->GetObject1();
			const char* name1 = "(invalid)";
			if (PHLEVEL->LegitLevelIndex(object1))
			{
				if (phInst* inst = PHLEVEL->GetInstance(object1))
				{
					if (phArchetype* arch = inst->GetArchetype())
					{
						name1 = arch->GetFilename();
					}
				}
			}

			if (phManifold* manifold = pair->GetManifold())
			{
				Displayf("%d: %s:%d, %s:%d <%.3f>", iPrune, name0, object0, name1, object1, float(manifold->GetCollisionTime()) * sysTimerConsts::TicksToMilliseconds);
			}
			else
			{
				Displayf("%d: %s:%d, %s:%d (no manifold)", iPrune, name0, object0, name1, object1);
			}
		}
	}
#endif // __BANK

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE
	FlushCommitDeferredOctreeUpdatesTask();
#endif // LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR

	PF_START(CollideJobs);

	// Can't delete stuff from callbacks while the collision jobs are running
	SetSafeToDelete(false);
	SetSafeToModifyManifolds(false);

#if USE_PPU_FRAME_PERSISTENT_GJK_CACHE
	GJKCacheCollisionProlog(GJKCACHESYSTEM);
#endif // USE_PPU_FRAME_PERSISTENT_GJK_CACHE

#if !CAPTURE_COLLISION_FLUSH
	InitiateCollisionJobs(*pairArray, timeStep);
#endif // CAPTURE_COLLISION_FLUSH

	ProcessOverlaps(pairArray, prunedPairs, sortList, numPrunedPairs, ScalarV(timeStep).GetIntrin128ConstRef());

	s_SortInstanceBehaviorsToken.Unlock();

	PF_STOP(ProcessOverlap);

	PF_START(ExtraWork);

    // Broadphase is over, set this flag to true
    pairArray->allPairsReady = true;

	// This work gets done while the other threads/SPUs are busy performing collisions
	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Add constraints");

#if EARLY_FORCE_SOLVE
	bool addToSolver = !s_PushCollision;
#else
	bool addToSolver = true;
#endif

	s_SortInstanceBehaviorsToken.Lock();
	m_ConstraintMgr->UpdateConstraintContacts(1.0f / timeStep, addToSolver);
	s_SortInstanceBehaviorsToken.Unlock();

	NOT_EARLY_FORCE_SOLVE_ONLY(m_ContactMgr->InitNextHalfScratchpad();)

	PF_STOP(ExtraWork);

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Flush collision jobs");
#if !CAPTURE_COLLISION_FLUSH
    FlushCollisionJobs(pairArray, timeStep);
#else // !CAPTURE_COLLISION_FLUSH
	PPC_STAT_TIMER_START(CollisionFlushTimer);
	InitiateCollisionJobs(*pairArray, timeStep);
	FlushCollisionJobs(pairArray, timeStep);
	PPC_STAT_TIMER_STOP(CollisionFlushTimer);
#endif // !CAPTURE_COLLISION_FLUSH

#if USE_PPU_FRAME_PERSISTENT_GJK_CACHE
	GJKCacheCollisionEpilog(GJKCACHESYSTEM);
#endif // USE_PPU_FRAME_PERSISTENT_GJK_CACHE

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(m_OverlappingPairArray);
	if (pairArray != m_OverlappingPairArray)
		CheckForDuplicateManifolds(pairArray);
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

	// OK, now we can delete stuff again
	SetSafeToDelete(true);
	SetSafeToModifyManifolds(true);

    PF_STOP(CollideJobs);

	// This feels a little weird putting PreComputeImpacts here, but it needs to be:
	//    * before FindPushPairs, which is the next thing after CollideActive, so that disabled contacts don't cause pushes
	//    * before CollisionActivations, so that disabled contacts don't activate stuff
	//
	// Also note that now that we're updating contact positions in the collision threads, we may as well call it here. In fact it would be super
	// awesome if we could start calling PreComputeImpacts for manifolds just as they started being completed by worker threads, but there's no
	// mechanism for that right now.
	PF_START_TIMEBAR_PHYS_SIM_DETAIL("PreCompteImpacts callbacks");
	if (GetPreComputeImpactsEnabled())
	{
		m_ContactMgr->DoPreComputeImpacts(pairArray);
	}

	// Removing these manifolds from the OPA is causing ragdolls to not always activate correctly. 
	// There must be some code that is looking for manifolds without checking contacts.
	//pairArray->RemoveEmptyManifolds();

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(m_OverlappingPairArray);
	if (pairArray != m_OverlappingPairArray)
		CheckForDuplicateManifolds(pairArray);
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

	CollisionActivations();

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(m_OverlappingPairArray);
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

	return nextStartingBroadphasePair;
}

void phSimulator::CollisionActivations()
{
	PF_FUNC(CollisionActivations);
	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Collion Activations");

	phLevelNew* level = m_Level;

	for (int pairIndex = 0; pairIndex < m_NumActivatingPairs; ++pairIndex)
	{
		phTaskCollisionPair* pair = m_ActivatingPairs[pairIndex];
		phManifold* manifold = pair->manifold;

		if (!manifold)
		{
			continue;
		}

		if (manifold->CompositeManifoldsEnabled() || manifold->GetNumContacts() > 0)
		{
			//TMS: Scaling back to just instances PH_DEBUG_CONTEXT(manifold);, for now.
			bool activeCollision = false;
			Vec3V contactPointA;
			Vec3V contactPointB;
			phContact *contact = NULL;

			if (manifold->IsConstraint())
			{
				activeCollision = true;
			}
			else if (manifold->CompositeManifoldsEnabled())
			{
				for (int compositeIndex = 0; !activeCollision && compositeIndex < manifold->GetNumCompositeManifolds(); ++compositeIndex)
				{
					phManifold* compositeManifold = manifold->GetCompositeManifold(compositeIndex);
					for (int pointIndex = compositeManifold->GetNumContacts() - 1; pointIndex >= 0; --pointIndex)
					{
						contact = &compositeManifold->GetContactPoint(pointIndex);
						if (contact->IsContactActive())
						{
							contactPointA = contact->GetWorldPosA();
							contactPointB = contact->GetWorldPosB();
							activeCollision = true;
							break;
						}
					}
				}
			}
			else
			{
				for (int pointIndex = manifold->GetNumContacts() - 1; pointIndex >= 0; --pointIndex)
				{
					contact = &manifold->GetContactPoint(pointIndex);
					if (contact->IsContactActive())
					{
						contactPointA = contact->GetWorldPosA();
						contactPointB = contact->GetWorldPosB();
						activeCollision = true;
						break;
					}
				}
			}

			if (activeCollision)
			{
				int levelIndexA = manifold->GetLevelIndexA();
				phLevelBase::eObjectState stateA = phLevelBase::OBJECTSTATE_NONEXISTENT;

				if (PHLEVEL->LegitLevelIndex(levelIndexA))
				{
					if( !PHLEVEL->IsInLevel(levelIndexA) )
						continue;

					if( level->GetInstance(levelIndexA) == NULL )
						continue;

					stateA = level->GetState(levelIndexA);
				}

				int levelIndexB = manifold->GetLevelIndexB();
				phLevelBase::eObjectState stateB = phLevelBase::OBJECTSTATE_NONEXISTENT;

				if (PHLEVEL->LegitLevelIndex(levelIndexB))
				{
					if( !PHLEVEL->IsInLevel(levelIndexB) )
						continue;

					if( level->GetInstance(levelIndexB) == NULL )
						continue;

					stateB = level->GetState(levelIndexB);
				}

				bool bHasRelativeVelocity;
				if (manifold->IsConstraint())
				{
					bHasRelativeVelocity = true;
				}
				else
				{
					Vec3V velA, velB;
					GetObjectLocalVelocity(velA, *level->GetInstance(levelIndexA), contactPointA, manifold->GetComponentA());
					GetObjectLocalVelocity(velB, *level->GetInstance(levelIndexB), contactPointB, manifold->GetComponentB());

					static ScalarV scTolerance = ScalarV(V_FLT_SMALL_3);
					bHasRelativeVelocity = (IsCloseAll(velA, velB, scTolerance) == 0);
				}

				phCollider* colliderA;
				if(stateA == phLevelBase::OBJECTSTATE_INACTIVE && PHMANIFOLD->GetAvailCount() > 0 && 
					bHasRelativeVelocity)
				{
					colliderA = ActivateObject(levelIndexA, NULL, manifold->GetInstanceB());

					if(colliderA && colliderA->IsArticulated())
					{
						static_cast<phArticulatedCollider*>(colliderA)->AddToActiveArray();
					}
				}
				else
				{
					colliderA = GetCollider(levelIndexA);
				}
				manifold->SetColliderA(colliderA);
				if (manifold->CompositeManifoldsEnabled())
				{
					for(int compositeManifoldIndex = 0; compositeManifoldIndex < manifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
					{
						phManifold *compositeManifold = manifold->GetCompositeManifold(compositeManifoldIndex);
						compositeManifold->SetColliderA(colliderA);
					}
				}

				phCollider* colliderB;
				if (stateB == phLevelBase::OBJECTSTATE_INACTIVE && PHMANIFOLD->GetAvailCount() > 0 &&
					bHasRelativeVelocity)
				{
					colliderB = ActivateObject(levelIndexB, NULL, manifold->GetInstanceA());

					if(colliderB && colliderB->IsArticulated())
					{
						static_cast<phArticulatedCollider*>(colliderB)->AddToActiveArray();
					}
				}
				else
				{
					colliderB = GetCollider(levelIndexB);
				}
				manifold->SetColliderB(colliderB);
				if (manifold->CompositeManifoldsEnabled())
				{
					for(int compositeManifoldIndex = 0; compositeManifoldIndex < manifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
					{
						phManifold *compositeManifold = manifold->GetCompositeManifold(compositeManifoldIndex);
						compositeManifold->SetColliderB(colliderB);
					}
				}

#if TRACK_COLLISION_TIME
				// If we have just activated an object then set both collision times to 1.0
				// This ensures that the just activated collision object does not move through collision straight away.
				// We want to make sure an object cannot move without any collision detection.
				// If one object didn't wake up make sure we don't alter the other's collision time... so only set either if BOTH are non-null
				if(colliderA && colliderB)
				{
					colliderA->SetCollisionTime(1.0f);
					colliderB->SetCollisionTime(1.0f);
				}
#endif // TRACK_COLLISION_TIME

#if PROPHYLACTIC_SWAPS
				if (Likely(g_ProphylacticSwaps))
				{
					if (colliderB && !colliderA ||
						(colliderA && colliderB && colliderB->IsArticulated() && !colliderA->IsArticulated()))
					{
						pair->Exchange();
					}
				}
#endif // PROPHYLACTIC_SWAPS
			}
		}
	}

	m_NumActivatingPairs = 0;
}

WIN32_ONLY(extern sysCriticalSectionToken s_TaskToken;)

void phSimulator::InitiateCollisionJobs(phOverlappingPairArray& pairArray, float timeStep)
{
	WIN32_ONLY(sysCriticalSection taskCriticalSection(s_TaskToken);)

    g_PairConsumeIndex = 0;

#if __PPU
	static PairListWorkUnitInput wuInput ;

	wuInput.m_minimumConcaveThickness = GetMinimumConcaveThickness();
	wuInput.m_allowedPenetration = phSimulator::GetAllowedPenetrationV();

	wuInput.m_pairListPtr = pairArray.pairs.GetElements();
	wuInput.m_numPairsMM = &pairArray.numCollisionPairs;
	diagDebugLog(diagDebugLogPhysics, 'pPnP', &pairArray.numCollisionPairs);
	wuInput.m_allPairsReadyMM = &pairArray.allPairsReady;
	wuInput.m_numPairsPerMutexLock = PHSIM->GetNumCollisionsPerTask();

	wuInput.m_selfCollisionsEnabled = phSimulator::GetSelfCollisionsEnabled();

#if __BANK
#if OCTANT_MAP_SUPPORT_ACCEL
	wuInput.m_useOctantMap = phBoundPolyhedron::GetUseOctantMapRef();
#endif // OCTANT_MAP_SUPPORT_ACCEL
#if USE_BOX_BOX_DISTANCE
	wuInput.m_useBoxBoxDistance = g_UseBoxBoxDistance;
#endif // USE_BOX_BOX_DISTANCE
#if CAPSULE_TO_CAPSULE_DISTANCE
	wuInput.m_fastCapsuleToCapsule = g_CapsuleCapsuleDistance;
#endif // CAPSULE_TO_CAPSULE_DISTANCE
#if CAPSULE_TO_TRIANGLE_DISTANCE
	wuInput.m_fastCapsuleToTriangle = g_CapsuleTriangleDistance;
#endif // CAPSULE_TO_TRIANGLE_DISTANCE
#if DISC_TO_TRIANGLE_DISTANCE
	wuInput.m_fastDiscToTriangle = g_DiscTriangleDistance;
#endif // DISC_TO_TRIANGLE_DISTANCE
#if BOX_TO_TRIANGLE_DISTANCE
	wuInput.m_fastBoxToTriangle = g_BoxTriangleDistance;
#endif // BOX_TO_TRIANGLE_DISTANCE
	wuInput.m_sweepFromSafe = g_SweepFromSafe;
#endif // __BANK

	EARLY_FORCE_SOLVE_ONLY(wuInput.m_pushCollision = s_PushCollision;)

	wuInput.m_debugBool1 = g_debugBool1;
	wuInput.m_debugBool2 = g_debugBool2;
	wuInput.m_debugBool3 = g_debugBool3;
	wuInput.m_debugBool4 = g_debugBool4;
	wuInput.m_debugBool5 = g_debugBool5;
	wuInput.m_debugBool6 = g_debugBool6;

	wuInput.m_pairConsumeToken = &rage::g_PairConsumeToken;
	wuInput.m_pairConsumeIndex = &rage::g_PairConsumeIndex;
	wuInput.m_InstLastMatrices = PHLEVEL->GetLastInstanceMatricesBaseAddr();
	wuInput.m_LevelIdxToLastMatrixMap = PHLEVEL->GetLastInstanceMatrixIndexMapBaseAddr();
	wuInput.m_LevelObjectDataArray = PHLEVEL->GetObjectDataArray();

	wuInput.m_TimeStep = timeStep;
	wuInput.m_MinManifoldPointLifetime = GetMinManifoldPointLifetime();

#if TRACK_COLLISION_TYPE_PAIRS
	wuInput.m_typePairTable = g_CollisionTypePairTable;
#endif

	MATERIALMGR.FillInSpuWorkUnit(wuInput);
	PHMANIFOLD->FillInSpuInitParams(wuInput.m_manifoldPoolInitParams);
	PHCONTACT->FillInSpuInitParams(wuInput.m_contactPoolInitParams);
	PHCOMPOSITEPOINTERS->FillInSpuInitParams(wuInput.m_compositePointersPoolInitParams);

#if ALLOW_MID_PHASE_SWAP
	wuInput.m_UseNewMidPhaseCollision = g_UseNewMidPhaseCollision;
#endif 

#if USE_PS3_FRAME_PERSISTENT_GJK_CACHE
	wuInput.m_gjkCacheSystem_EA = GJKCACHESYSTEM;
#endif // USE_PS3_FRAME_PERSISTENT_GJK_CACHE

#if __BANK && __PS3
	wuInput.m_UseGJKCache = g_UseGJKCache;
	wuInput.m_UseFramePersistentGJKCache = g_UseFramePersistentGJKCache;
#endif // __BANK && __PS3

	static sysTaskParameters params ;
	params.Input.Data = &wuInput;
#else
	diagDebugLogSetDisabled(true);

	sysTaskParameters params;
	sysMemZeroBytes<sizeof(sysTaskParameters)>(&params);

	params.UserDataCount = 2;
	params.UserData[0].asPtr = this;
	params.UserData[1].asPtr = &pairArray;
	params.UserData[2].asFloat = timeStep;
#endif

	// Workaround fix for physics hang, see B* 2078547 - lock the pair consume token before 
	// creating the worker threads, so they immediately block and wait, and then unlock it 
	// when we're done creating the worker threads.
	g_PairConsumeToken.Lock();
	m_CollisionTasks->Initiate(params, phConfig::GetCollisionWorkerThreadCount());
	g_PairConsumeToken.Unlock();
}


void phSimulator::FlushCollisionJobs(phOverlappingPairArray* pairArray, float timeStep)
{
    // If the main application thread is helping with collisions, start it up here
	if (phConfig::GetCollisionMainThreadAlsoWorks())
    {
        sysTaskParameters p;
		sysMemZeroBytes<sizeof(sysTaskParameters)>(&p);

        p.UserDataCount = 2;
        p.UserData[0].asPtr = this;
		p.UserData[1].asPtr = pairArray;
		p.UserData[2].asFloat = timeStep;

        ProcessPairListTask(p);
    }

    // Wait for worker threads to complete
	m_CollisionTasks->Wait();
}


__forceinline void phSimulator::InitiateCommitDeferredOctreeUpdatesTask()
{
#if USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
#if __BANK
	if(sm_UseOctreeUpdateTask)
#endif	// __BANK
	{
		static sysTaskParameters params ;

		params.UserDataCount = 5;
		params.UserData[0].asPtr = m_Level;

#if ENABLE_PHYSICS_LOCK
#if __PPU
	params.UserData[1].asPtr = g_GlobalPhysicsLock.GetGlobalReaderCountPtr();
	params.UserData[2].asPtr = g_GlobalPhysicsLock.GetPhysicsMutexPtr();
#if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	params.UserData[3].asPtr = g_GlobalPhysicsLock.GetAllowNewReaderMutex();
#else	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	params.UserData[3].asPtr = NULL;
#endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	params.UserData[4].asPtr = g_GlobalPhysicsLock.GetModifyReaderCountMutexPtr();
#else	// __PPU
	// Don't need to pass these via parameters on 360/PC.  We probably don't really need to set any of these.
	CompileTimeAssert(__WIN32);
	params.UserData[1].asPtr = NULL;
	params.UserData[2].asPtr = NULL;
	params.UserData[3].asPtr = NULL;
	params.UserData[4].asPtr = NULL;
#endif	// __PPU
#endif	// ENABLE_PHYSICS_LOCK

#if !__PPU
		diagDebugLogSetDisabled(true);
#endif	// !__PPU

		m_CommitDeferredOctreeUpdatesTask->Initiate(params, 1);
	}
#if __BANK
	else
	{
		m_Level->CommitDeferredOctreeUpdates();
	}
#endif	// __BANK
#else	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
	m_Level->CommitDeferredOctreeUpdates();
#endif	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
}


__forceinline void phSimulator::FlushCommitDeferredOctreeUpdatesTask()
{
#if USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
	// Wait for worker threads to complete
	BANK_ONLY(if(sm_UseOctreeUpdateTask)) m_CommitDeferredOctreeUpdatesTask->Wait();
#else	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
	// If we're not using a task to run it on a separate thread then we don't do anything here.
#endif	// USE_COMMIT_DEFERRED_OCTREE_UPDATES_TASK
}



void phSimulator::CheckBroadphaseOverlaps(phInst* inst)
{
	// First, get all the insts we overlap with according to the octree
	Vec3V obbExtents, obbCenter;
	inst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(obbExtents, obbCenter);
	Vec3V boxHalfWidth = geomBoxes::ComputeAABBExtentsFromOBB(inst->GetMatrix().GetMat33ConstRef(), obbExtents);
	Vec3V boxCenter = Transform(inst->GetMatrix(), obbCenter);

#if ENABLE_PHYSICS_LOCK
	phIterator it(phIterator::PHITERATORLOCKTYPE_READLOCK);
#else	// ENABLE_PHYSICS_LOCK
	phIterator it;
#endif	// ENABLE_PHYSICS_LOCK
	it.InitCull_AABB(boxCenter,boxHalfWidth);
	it.SetCullAgainstInstanceAABBs(true);

	const int MAX_OVERLAPPING_INSTS = 1024;
	u16 octreeOverlappingInsts[MAX_OVERLAPPING_INSTS];
	int numOctreeOverlappingInsts = 0;
	u16 overlappingInst = m_Level->GetFirstCulledObject(it);
	u16 levelIndex = inst->GetLevelIndex();
	while(overlappingInst != phInst::INVALID_INDEX && numOctreeOverlappingInsts < MAX_OVERLAPPING_INSTS)
	{
		if (overlappingInst != levelIndex)
		{
			octreeOverlappingInsts[numOctreeOverlappingInsts++] = overlappingInst;
		}

		overlappingInst = m_Level->GetNextCulledObject(it);
	}

	// Give up if we overlap too many things
	if (numOctreeOverlappingInsts == MAX_OVERLAPPING_INSTS)
	{
		return;
	}

	// Then get the insts we overlap according to the SAP
	btBroadphasePair* prunedPairs = m_Level->GetBroadPhase()->getPrunedPairs();
	const int nPrunedPair = m_Level->GetBroadPhase()->getPrunedPairCount();

	u16 sapOverlappingInsts[MAX_OVERLAPPING_INSTS];
	int numSapOverlappingInsts = 0;

	for( int iPrune = 0; iPrune < nPrunedPair && numSapOverlappingInsts < MAX_OVERLAPPING_INSTS; iPrune++ )
	{
		btBroadphasePair *pair = prunedPairs + iPrune;
		// skip already moved guys
		if( pair->GetObject0() == levelIndex )
		{
			if (sapOverlappingInsts[numSapOverlappingInsts] != levelIndex)
			{
				sapOverlappingInsts[numSapOverlappingInsts++] = pair->GetObject1();
			}
		}
		else if( pair->GetObject1() == levelIndex )
		{
			if (sapOverlappingInsts[numSapOverlappingInsts] != levelIndex)
			{
				sapOverlappingInsts[numSapOverlappingInsts++] = pair->GetObject0();
			}
		}
	}

	// Give up if we overlap too many things
	if (numSapOverlappingInsts == MAX_OVERLAPPING_INSTS)
	{
		return;
	}

	std::sort(octreeOverlappingInsts, octreeOverlappingInsts + numOctreeOverlappingInsts);
	std::sort(sapOverlappingInsts, sapOverlappingInsts + numSapOverlappingInsts);

	int octreeIndex = 0;
	for (int sapIndex = 0; sapIndex < numSapOverlappingInsts; ++sapIndex, ++octreeIndex)
	{
		// The SAP is allowed to contain more stuff due to quantization
		while (octreeOverlappingInsts[octreeIndex] > sapOverlappingInsts[sapIndex])
		{
			sapIndex++;
		}
		Assert(octreeOverlappingInsts[octreeIndex] == sapOverlappingInsts[sapIndex]);
	}
}


void phSimulator::CheckAllBroadphaseOverlaps()
{
#if ENABLE_PHYSICS_LOCK
	phIterator it(phIterator::PHITERATORLOCKTYPE_READLOCK);
#else	// ENABLE_PHYSICS_LOCK
	phIterator it;
#endif	// ENABLE_PHYSICS_LOCK
	it.InitCull_All();
	it.SetStateIncludeFlags(phLevelBase::STATE_FLAG_ACTIVE);

	u16 inst = m_Level->GetFirstCulledObject(it);
	while(inst != phInst::INVALID_INDEX)
	{
		CheckBroadphaseOverlaps(PHLEVEL->GetInstance(inst));

		inst = m_Level->GetNextCulledObject(it);
	}
}

void phSimulator::BreakObject (phInstBreakable* pInstance, Vec::V3Param128 timeStep, 
	Vec3V_In worldPosition,	Vec3V_In impulseV, const int iElement, const int iComponent, 
	const bool isForce, bool scaleForceByMassRatio, float breakScale) const
{
	Assert(pInstance);
	Assert(pInstance->GetArchetype());
	// Create the list of broken parts, for use in recalculating collisions.
	phInst* brokenInstList[phInstBreakable::MAX_NUM_BROKEN_INSTS];
	int numBrokenInsts = 0;

	// Create a blank class for use by breakable objects to record and recall how they will break, another one to
	// test each breakable object to find the weakest, and pointers to swap between them to keep track of the weakest.
	phBreakData breakData0,breakData1;
	phBreakData* breakData = &breakData0;
	phBreakData* testBreakData = &breakData1;

	// Cache off the original mass so we know how to distribute the forces on the broken pieces
	float originalMass = pInstance->GetArchetype()->GetMass();
	Assert(originalMass > 0.0f);

	ScalarV v_breakScale = ScalarVFromF32(breakScale);
	Vec3V preBreakImpulse;
	Vec3V preScaleImpulse(V_ZERO);
	float breakStrength;
	const int maxNumRecurses = 32;
	int numRecurses = 0;
	int index;
	bool wasForce = false;
	Vec3V impulse = impulseV;
	bool bIsForce = isForce;
	if (bIsForce)
	{
		// Scale the force by the default frame time to make the impulse that is used to test whether
		// the object will break. Scaling it by the real frame time would make breaking depend on frame
		// rate. The real force will be used to break the object.
		wasForce = true;
		impulse = Scale( impulse, ScalarVFromF32(DEFAULT_FRAME_TIME) );
		bIsForce = false;
	}

	phInst* pInstanceA = pInstance;
	phInst* pInstanceB = NULL;
	phInstBreakable* breakInstList[phInstBreakable::MAX_NUM_BREAKABLE_INSTS] = {pInstance};
	int numBreakInsts = 1;
	while (numBreakInsts>0)
	{
		// scale up the impulse that's used to FindWeakestInst
		preScaleImpulse = impulse;
		impulse = Scale( preScaleImpulse, v_breakScale );

		// Find the weakest breakable instance and the scale factor to get the bre-breaking impetuses from
		// the non-breaking impetuses (energyScale).

		phInstBreakable* weakestInst = FindWeakestInst(breakInstList,numBreakInsts,&breakStrength,
			worldPosition, Vec3V(V_ZERO), impulse, pInstance, iComponent, NULL, 0,
			&breakData,&testBreakData);

		// restore impulse to original value before it gets applied to anything (including for the preBreakImpulse calculation)
		impulse = preScaleImpulse;

		if (weakestInst)
		{
			// Apply the pre-breaking impetuses in all the impact list impacts involving the weakest instance,
			// or from the single applied impetus. The pre-breaking impetuses are the non-breaking impetuses
			// scaled by the breaking strength.

			// Change the impetus back to a force so that it gets properly scaled by the current frame rate
			if (wasForce)
			{
				impulse = Scale( impulse, ScalarVFromF32(DEFAULT_FRAME_TIME));
				bIsForce = false;
			}

			// Apply the pre-breaking impulse to the weakest object, changing its motion instantly, and
			// set the impulse to be the post-breaking impulse.
			Assert(!bIsForce);
			Assert(breakStrength>=0.0f && breakStrength<=1.0f);

			// scale back to original impulse when calculating impulse to apply
			preBreakImpulse = Scale(impulse, Vec3VFromF32(breakStrength));
			phCollider *collider = GetCollider(weakestInst);

			// We already tried activating the instance, if it didn't create a collider then don't apply a pre-breaking impulse, otherwise it might break the object here.
			if (collider)
			{
				// Apply the pre-breaking impulse to the unbroken collider.
				float impulseScale = scaleForceByMassRatio ? (weakestInst->GetArchetype()->GetMass() / originalMass) : 1.0f;
				collider->ApplyImpulseChangeMotion(VEC3V_TO_VECTOR3(preBreakImpulse)*impulseScale,VEC3V_TO_VECTOR3(worldPosition),iComponent);
			}

			if (wasForce)
			{
				impulse = Scale( impulse, ScalarVFromF32(DEFAULT_FRAME_RATE) );
				bIsForce = true;
			}
			// Break the weakest breakable instance.
			numBrokenInsts = weakestInst->BreakApart(breakInstList,&numBreakInsts,brokenInstList,
				iComponent, 0, pInstanceA, pInstanceB, *breakData);
			Assert(numBrokenInsts<=phInstBreakable::MAX_NUM_BROKEN_INSTS);
			Assert(numBreakInsts<=phInstBreakable::MAX_NUM_BREAKABLE_INSTS);

			// Find the post-breaking impetuses.
			// Convert the total impulse to the post-breaking impulse.
			Vec3V impetus = impulse;
			// scale back to original impetus
			impetus -= preBreakImpulse;
			impulse = impetus;
		}
		else
		{
			// None of the colliding objects will break in these collisions.
			break;
		}

		// Increment and check the number of times this has been done. This is only for protection in case a derived
		// instance class messes up BreakApart by forever adding breakable instances, which could freeze here.
		if (++numRecurses>maxNumRecurses)
		{
			Warningf("Too many recurses breaking objects in phSimulator::DoCollision()");
			break;
		}
	}

	if (wasForce)
	{
		// Change the impetus back to a force so that it gets properly scaled by the current frame rate
		impulse = Scale( impulse, ScalarVFromF32(DEFAULT_FRAME_RATE) );
		bIsForce = true;
	}


	// Apply the post-breaking part of the applied impulse to the specified broken parts.
	//Assert(numBrokenInsts==0 || numBreakInsts==0);
	for (index=0; index<numBrokenInsts; index++)
	{
		ScalarV impulseScale = scaleForceByMassRatio ? ScalarVFromF32(brokenInstList[index]->GetArchetype()->GetMass() / originalMass) : ScalarV(V_ONE);
		phCollider *collider = GetCollider(brokenInstList[index]);
		if (collider)
		{
			collider->ApplyImpetus( worldPosition, Scale(impulse,impulseScale), iElement, iComponent, bIsForce, timeStep, breakScale);
		}
		else
		{
			brokenInstList[index]->ApplyImpetus( worldPosition, Scale(impulse,impulseScale), iElement, iComponent, bIsForce, breakScale);
		}
	}

	// Apply the post-breaking part of the applied impulse to any unbroken breakable parts.
	for (index=0; index<numBreakInsts; index++)
	{
		ScalarV impulseScale = scaleForceByMassRatio ? ScalarVFromF32(breakInstList[index]->GetArchetype()->GetMass() / originalMass) : ScalarV(V_ONE);
		phCollider *collider = GetCollider(breakInstList[index]);
		if (collider)
		{
			collider->ApplyImpetus( worldPosition, Scale(impulse,impulseScale), iElement, iComponent, bIsForce, timeStep, breakScale);
		}
		else
		{
			breakInstList[index]->ApplyImpetus( worldPosition, Scale(impulse,impulseScale), iElement, iComponent, bIsForce, breakScale);
		}
	}
}


phInstBreakable* phSimulator::FindWeakestInst (phInstBreakable** breakInstList, int numBreakInsts,float* breakStrength, 
	Vec3V_In worldPosA, Vec3V_In worldPosB, Vec3V_In impulseA,const phInst* pInstanceA, const int iComponentA,const phInst* pInstanceB, const int iComponentB,
	phBreakData** breakData, phBreakData** testBreakData) const
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
		// Get the breaking strength for this instance in these collisions (0=breaks easily, 1=doesn't break).
		testInst = breakInstList[instIndex];
		Vector3 breakingImpulses[phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS];
		Vector4 breakingPositions[phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS];
		int numComponents = 1;
		breakingImpulses[0].Zero();
		breakingPositions[0].Zero();
		if (testInst->GetArchetype()->GetBound()->GetType() == phBound::COMPOSITE)
		{
			numComponents = static_cast<phBoundComposite*>(testInst->GetArchetype()->GetBound())->GetNumBounds();

			// Check that number of bounds is less than the maximum number of breakable instances
			//Assert(numComponents < phInstBreakable::MAX_NUM_BREAKABLE_INSTS && "phInstBreakable::MAX_NUM_BREAKABLE_INSTS exceeded");
			if(numComponents > phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS)
			{
				Warningf("phSimulator::FindWeakestInst(): Maximum number of breakable components %d exceeded: %d", phInstBreakable::MAX_NUM_BREAKABLE_INSTS, numComponents);
				numComponents = phInstBreakable::MAX_NUM_BREAKABLE_COMPONENTS;
			}

			for (int component = 1; component < numComponents; ++component)
			{
				breakingImpulses[component].Zero();
				breakingPositions[component].Zero();
			}
		}

		if (testInst == pInstanceA)
		{
			if(iComponentA >= 0 && iComponentA < numComponents)
			{
				Vec3V impulse = impulseA;
				breakingImpulses[iComponentA].Add(Vector3(impulse.GetIntrin128()));
				ScalarV mag = Mag(impulse);
				Vec4V position;
				position.SetXYZ(worldPosA * mag);
				position.SetW(mag);
				breakingPositions[iComponentA].Add(Vector4(position.GetIntrin128()));
			}
		}
		else if (testInst == pInstanceB)
		{
			if(iComponentB >= 0 && iComponentB < numComponents)
			{
				Vec3V impulse = impulseA;
				breakingImpulses[iComponentB].Add(Vector3(impulse.GetIntrin128()));
				ScalarV mag = Mag(impulse);
				Vec4V position;
				position.SetXYZ(worldPosB * mag);
				position.SetW(mag);
				breakingPositions[iComponentB].Add(Vector4(position.GetIntrin128()));
			}
		}

		if (testInst->FindBreakStrength(breakingImpulses,breakingPositions,&testBreakStrength,*testBreakData) &&
			testBreakStrength<(*breakStrength))
		{
			// This is the most easily breakable instance found so far.
			(*breakStrength) = testBreakStrength;
			SwapEm(*breakData,*testBreakData);
			weakestInst = testInst;
		}
	}

	// Return the most easily breakable instance in these collisions, or NULL if these collisions
	// will not cause any of the instances to break.
	return weakestInst;
}

phCollider* phSimulator::TestPromoteInstance (phInst* instance)
{
	Assert(instance);
	phCollider *collider = NULL;
	if(m_Level->IsInactive(instance->GetLevelIndex()))
	{
		collider = ActivateObject(instance->GetLevelIndex());
	}
	else
	{
		collider = GetCollider(instance->GetLevelIndex());
	}
	return collider;
}


void phSimulator::GetInvMassMatrix (const phInst* instance, const phCollider* collider, const Vector3& sourcePos, Matrix33& outMtx,
									const Vector3* responsePos, Vector3* localVelocity, Vector3* angVelocity)
{
	if (collider)
	{
		// A collider was found, so get the inverse mass matrix for the collider.
		collider->GetInvMassMatrix(RC_MAT33V(outMtx),sourcePos,&RCC_VEC3V(*responsePos));
		if (localVelocity)
		{
			*localVelocity = VEC3V_TO_VECTOR3(collider->GetLocalVelocity(sourcePos.xyzw));
		}
		if (angVelocity)
		{
			angVelocity->Set(RCC_VECTOR3(collider->GetAngVelocity()));
		}
	}
	else
	{
		// No collider was found (the object is not active), so get the inverse mass matrix for the instance.
		Assert(instance);
		instance->GetInvMassMatrix(outMtx,sourcePos,responsePos);
		if (localVelocity)
		{
			*localVelocity = VEC3V_TO_VECTOR3(instance->GetExternallyControlledLocalVelocity(sourcePos));
		}
		if (angVelocity)
		{
			*angVelocity = VEC3V_TO_VECTOR3(instance->GetExternallyControlledAngVelocity());
		}
	}
}


void phSimulator::GetCompositeInvMassMatrix (const phInst* instanceA, const phCollider* colliderA, const phInst* instanceB,
												const phCollider* colliderB, const Vector3& sourcePosA, const Vector3& sourcePosB,
												Matrix33& mtx, const Vector3* responsePosA, const Vector3* responsePosB,
												Vector3* localDelVel)
{
	// Get the inverse mass matrix of object A.
	GetInvMassMatrix(instanceA,colliderA,sourcePosA,mtx,responsePosA,localDelVel);

	// Get the inverse mass matrix of object B.
	Matrix33 immB;
	Vector3 localVelB;
	Vector3* localVelBPtr = localDelVel ? &localVelB : NULL;
	GetInvMassMatrix(instanceB,colliderB,sourcePosB,immB,responsePosB,localVelBPtr);

	if (localDelVel)
	{
		// Get the difference between the velocities at the source position.
		localDelVel->Subtract(localVelB);
	}

	// Add the inverse mass matrices of the two objects.
	mtx.Add(immB);
}


#if HACK_GTA4_FRAG_BREAKANDDAMAGE
bool phSimulator::ApplyImpetusAndBreakingImpetus(Vec::V3Param128 timeStep, int levelIndex, const Vector3& impetus, const Vector3& position, int component, int element, float breakingImpetus, bool scaleImpulseByMassRatio)
{
	//Compute a breakscale that satisfies
	//|impetus|*breakScale=breakingImpetus.
	const float impulseMag=impetus.Mag();
	float breakScale;
	if(0.0f==impulseMag)
	{
		breakScale=0.0f;
	}
	else
	{
		breakScale=breakingImpetus/impulseMag;
	}
	return ApplyImpetus(timeStep,levelIndex,impetus,position,component,element,true,scaleImpulseByMassRatio,breakScale);
}

bool phSimulator::ApplyForceAndBreakingForce(Vec::V3Param128 timeStep, int levelIndex, const Vector3& force, const Vector3& position, int component, int element, float breakingForce, bool scaleForceByMassRatio)
{
	//Compute a breakscale that satisfies
	//|force|*breakScale=breakingForce.
	const float forceMag=force.Mag();
	float breakScale;
	if(0.0f==forceMag)
	{
		breakScale=0.0f;
	}
	else
	{
			breakScale=breakingForce/forceMag;
	}
	return ApplyImpetus(timeStep,levelIndex,force,position,component,element,false,scaleForceByMassRatio,breakScale);
}

#endif//HACK_GTA4_FRAG_BREAKANDDAMAGE

bool phSimulator::ApplyImpetus (Vec::V3Param128 timeStep, int levelIndex, const Vector3& impetus, const Vector3& position, int component, int element, bool isImpulse, bool scaleForceByMassRatio, float breakScale)
{
	bool isActive = false;

	// Get the instance pointer.
	phInst* instance = m_Level->GetInstance(levelIndex);

	// Get the collider, making the object active if it isn't already and should be when hit.
	phCollider* collider = TestPromoteInstance(instance);
	isActive = (collider!=NULL);

	// HACK: Check for invalid components after potentially activating the instance. This is a simple band-aid fix 
	//         for ragdolls switching LOD during activation. We still aren't solving the problem of the component index referencing
	//         the wrong component
	const phBound* bound = instance->GetArchetype()->GetBound();
	if(bound->GetType() == phBound::COMPOSITE && component >= static_cast<const phBoundComposite*>(bound)->GetNumBounds())
	{
		return isActive;
	}

	if (instance->IsBreakable(NULL))
	{
		// only allow breaking of objects if we have enough managed colliders to give to the breaking object's children later on
		// as the broken off children will need to have managed colliders assigned to them to properly activate.
		if(m_NumUsedManagedColliders<m_MaxManagedColliders)
		{
			// Try to break the object into pieces, recursing to try to break any breakable broken pieces.
			BreakObject(static_cast<phInstBreakable*>(instance),timeStep,RCC_VEC3V(position),RCC_VEC3V(impetus),element,component,!isImpulse,scaleForceByMassRatio,breakScale);
		}
	}
	else
	{
		// The instance is not breakable.
		if (collider)
		{
			collider->ApplyImpetus(RCC_VEC3V(position), RCC_VEC3V(impetus), element, component, !isImpulse, timeStep, breakScale);
		}
		else
		{
			instance->ApplyImpetus(RCC_VEC3V(position), RCC_VEC3V(impetus), element, component, !isImpulse, breakScale);
		}
	}

	return isActive;
}


bool phSimulator::ApplyImpulse (int levelIndex, const Vector3& impulse, const Vector3& position, int component, int partIndex, float breakScale)
{
	return ApplyImpetus(ScalarV(V_ZERO).GetIntrin128ConstRef(),levelIndex,impulse,position,component,partIndex,true,false,breakScale);
}


bool phSimulator::ApplyBulletImpulse (int levelIndex, ScalarV_In bulletMass, ScalarV_In bulletSpeed, Vec3V_In bulletWorldUnitDir, Vec3V_In worldPosition, int component, int partIndex, ScalarV_In breakScale)
{
	bool isActive = false;
	if (m_Level->IsActive(levelIndex) || m_Level->IsInactive(levelIndex))
	{
		// The target object isn't fixed.  Get the instance pointer.
		phInst* instance = m_Level->GetInstance(levelIndex);

		// Get the collider, making the object active if it isn't already and should be when hit.
		phCollider* collider = TestPromoteInstance(instance);
		Vec3V bulletVelocity = Scale(bulletWorldUnitDir,bulletSpeed);
		Vec3V impulse = Scale(bulletMass,bulletVelocity);
		if (collider)
		{
			isActive = true;
			Matrix33 invMass;
			collider->GetInvMassMatrix(RC_MAT33V(invMass),worldPosition.GetIntrin128(),NULL,component,component);
			float inverseBulletMass = InvertSafe(bulletMass.Getf());
			invMass.a.x += inverseBulletMass;
			invMass.b.y += inverseBulletMass;
			invMass.c.z += inverseBulletMass;
			Vector3 reducedImpulse = invMass.SolveSVD(RCC_VECTOR3(bulletVelocity));
			impulse = RCC_VEC3V(reducedImpulse);
		}

		// HACK: Check for invalid components after potentially activating the instance. This is a simple band-aid fix 
		//         for ragdolls switching LOD during activation. We still aren't solving the problem of the component index referencing
		//         the wrong component
		const phBound* bound = instance->GetArchetype()->GetBound();
		if(bound->GetType() == phBound::COMPOSITE && component >= static_cast<const phBoundComposite*>(bound)->GetNumBounds())
		{
			return isActive;
		}

		if (instance->IsBreakable(NULL))
		{
			// Try to break the object into pieces, recursing to try to break any breakable broken pieces.
			BreakObject(static_cast<phInstBreakable*>(instance),ScalarV(V_ZERO).GetIntrin128ConstRef(), worldPosition, impulse, partIndex, component, false, false , breakScale.Getf());
		}
		else
		{
			// The instance is not breakable.
			if (collider)
			{
				collider->ApplyImpetus(worldPosition, impulse, partIndex, component, false, ScalarV(V_ZERO).GetIntrin128ConstRef(), breakScale.Getf());
			}
			else
			{
				instance->ApplyImpetus(worldPosition, impulse, partIndex, component, false, breakScale.Getf());
			}
		}
	}

	return isActive;
}


bool phSimulator::ApplyBulletImpulse (int levelIndex, Vec3V_In impulse, ScalarV_In bulletSpeed, Vec3V_In worldPosition, int component, int element, ScalarV_In breakScale)
{
	// Compute the world unit direction and the mass from the given speed and impulse.
	Vec3V bulletDirection = impulse;
	ScalarV bulletMass = Mag(bulletDirection);
	bulletDirection = InvScale(bulletDirection,bulletMass);
	bulletMass = Scale(bulletMass,Invert(bulletSpeed));

	// Apply the bullet impulse with the mass, speed and direction.
	return ApplyBulletImpulse(levelIndex,bulletMass,bulletSpeed,bulletDirection,worldPosition,component,element,breakScale);
}


bool phSimulator::ApplyForce (Vec::V3Param128 timeStep, int levelIndex, const Vector3& force, const Vector3& position, int component, int element, float breakScale)
{
	return ApplyImpetus(timeStep,levelIndex,force,position,component,element,false,false,breakScale);
}

bool phSimulator::ApplyForceScaled (Vec::V3Param128 timeStep, int levelIndex, Vector3& force, const Vector3& position, float mass, int component)
{
	// Get the physics instance that will get the force.
	phInst *instance = m_Level->GetInstance(levelIndex);
	if(instance && !m_Level->IsFixed(levelIndex))
	{
		// Scale the force down to account for the reaction of the object applying the force (this is used by wheels
		// applying a force from a car to an object touched by the wheel).
		Assert(instance->GetArchetype());
		float instanceMass = instance->GetArchetype()->GetMass();
		force.Scale(instanceMass/(mass+instanceMass));
		return ApplyForce(timeStep,levelIndex,force,position,component);
	}

	// There's no instance in the physics level with the given level index.
	return false;
}


bool phSimulator::ApplyImpetus (Vec::V3Param128 timeStep, int levelIndex, const Vector3& impetus, bool isImpulse)
{
	// Get the physics instance that will get the impetus.
	phInst *instance = m_Level->GetInstance(levelIndex);
	if (instance)
	{
		// Get the object's center of mass.
		Vector3 centerOfMass = VEC3V_TO_VECTOR3(instance->GetCenterOfMass());
		const int component = 0;
		const int element = 0;
		return ApplyImpetus(timeStep,levelIndex,impetus,centerOfMass,component,element,isImpulse);
	}

	// There's no instance in the physics level with the given level index.
	return false;
}


bool phSimulator::ApplyAngImpulse(int levelIndex, const Vector3& angImpulse)
{
	if(m_Level->IsActive(levelIndex) || m_Level->IsInactive(levelIndex))
	{
		// The target object isn't fixed.  Get the instance pointer.
		phInst *instance = m_Level->GetInstance(levelIndex);

		// Get the collider, making the object active if it isn't already and should be when hit.
		phCollider* collider = TestPromoteInstance(instance);

		// The instance is not breakable.
		if (collider)
		{
			collider->ApplyAngImpulse(angImpulse);
			return true;
		}
		else
		{
			instance->ApplyAngImpulse(angImpulse);
			return false;
		}
	}

	return false;
}


bool phSimulator::ApplyTorque(Vec::V3Param128 timeStep, int levelIndex, Vec::V3Param128 torque)
{
	if(m_Level->IsActive(levelIndex) || m_Level->IsInactive(levelIndex))
	{
		// The target object isn't fixed.  Get the instance pointer.
		phInst *instance = m_Level->GetInstance(levelIndex);

		// Get the collider, making the object active if it isn't already and should be when hit.
		phCollider* collider = TestPromoteInstance(instance);

		// The instance is not breakable.
		if (collider)
		{
			collider->ApplyTorque(torque, timeStep);
			return true;
		}
		else
		{
			instance->ApplyTorque(torque);
			return false;
		}
	}

	return false;
}


bool phSimulator::ApplyForceCenterOfMass (int levelIndex, Vec::V3Param128 force, Vec::V3Param128 timestep, float breakScale)
{
	bool isActive = false;

	// Get the instance pointer.
	phInst* instance = m_Level->GetInstance(levelIndex);

	// Get the collider, making the object active if it isn't already and should be when hit.
	phCollider* collider = TestPromoteInstance(instance);
	isActive = (collider!=NULL);

	// Compute center of mass, only used for breaking and inst notification
	Vector3 centerOfMass = VEC3V_TO_VECTOR3(instance->GetCenterOfMass());

	if (instance->IsBreakable(NULL))
	{
		// Try to break the object into pieces, recursing to try to break any breakable broken pieces.
		BreakObject(static_cast<phInstBreakable*>(instance),timestep,RCC_VEC3V(centerOfMass),Vec3V(force),0,0,true,false,breakScale);
	}
	else
	{
		// The instance is not breakable.
		if (collider)
		{
			collider->ApplyForceCenterOfMass(force, timestep);
		}

		instance->ApplyForce(force, centerOfMass, 0, 0, breakScale);
	}

	return isActive;
}


Mat34V_ConstRef phSimulator::GetLastInstanceMatrix (const phInst* instance) const
{
	const phCollider* collider = GetCollider(instance);
	if (collider)
	{
		return collider->GetLastInstanceMatrix();
	}

	return m_Level->GetLastInstanceMatrix(instance);
}

void phSimulator::SetLastInstanceMatrix(const phInst* inst, Mat34V_In lastMtx)
{
	phCollider* collider = GetCollider(inst);
	if (collider)
	{
		collider->SetLastInstanceMatrix(lastMtx);
	}
	else if (inst->HasLastMatrix())
	{
		m_Level->SetLastInstanceMatrix(inst, lastMtx);
	}
}

bool phSimulator::ColliderIsPermanentlyActive (const phCollider* collider) const
{
	if (!ColliderIsManagedBySimulator(collider))
	{
		// The given collider is not managed by the simulator, so it does not have a permanently active flag.
		// It can still be made permanently active by making its sleep NULL.
		return false;
	}

	// The given collider is managed by the simulator, so check its permanently active flag.
	size_t colliderIndex = collider - &m_ManagedColliders[0];
	return (m_ManagedColliderFlags[colliderIndex] & COLLIDERFLAG_PERMANENTLYACTIVE);
}


bool phSimulator::ColliderIsManagedBySimulator (const phCollider* collider) const
{
	size_t colliderIndex = collider - &m_ManagedColliders[0];
	return colliderIndex<(size_t)m_MaxManagedColliders;
}

#if !EARLY_FORCE_SOLVE
void phSimulator::PostCollide (float timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::PostCollide");
	PF_FUNC(PostCollide);

#if __DEBUGLOG
	for( int iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
	{
		int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 
		phCollider* collider = GetActiveCollider(levelIndex);
		collider->DebugReplay();
	}
#endif

	ScalarV vTimeStep(ScalarVFromF32(timeStep));

	if (sm_ComputeForces)
	{
		Assert(m_ContactMgr);

		PF_START(ForceSolver);

		m_ContactMgr->Update(m_OverlappingPairArray, timeStep);

		PF_STOP(ForceSolver);
	}

	// Turn off UOL asserts, because pushing causes articulated bodies to call MoveColliderWithRootLink, which no longer calls UOL.
	// That should be fine because we're going to call it in the loop below, so just inform the level to turn the other way.
	m_Level->BeginMultipleUpdates();

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Update colliders post");
	PF_START(UpdateCollidersPost);

	if (phConfig::GetColliderPostWorkerThreadCount()>0)
	{
		static sysTaskParameters params ;
		FillInColliderUpdateParams(params, PHCOLLIDER_UPDATE_POST, timeStep);

		m_UpdateCollidersTasks->Initiate(params, phConfig::GetColliderPostWorkerThreadCount());

		if (phConfig::GetColliderPostMainThreadAlsoWorks())
		{
			UpdateCollidersTask(params);
		}

		m_UpdateCollidersTasks->Wait();

		for( int iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
		{
			int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 
			phCollider* collider = GetActiveCollider(levelIndex);
			Assert(collider);

			phInst* inst = collider->GetInstance();
			// Inform the instance that it may have moved.
			if (sm_ReportMovedBySim)
			{
				inst->ReportMovedBySim();
			}

			m_Level->UpdateObjectLocationAndRadius(levelIndex, &RCC_MATRIX34(collider->GetLastInstanceMatrix()));
		}
	}
    else
	if (sm_ColliderUpdateEnabled)
    {
		for( int iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
		{
			int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 

			// Get the collider and see if it is sleeping.
			phCollider* collider = GetActiveCollider(levelIndex);
			Assert(collider);

			phInst* inst = collider->GetInstance();

			collider->UpdateVelocity(vTimeStep.GetIntrin128());

		    // Update the collider's position from pushes.
		    collider->Move(vTimeStep.GetIntrin128(), phContactMgr::GetUsePushes());

			phSleep * sleep = (sm_SleepEnabled ? collider->GetSleep() : NULL);
			if (sleep && !ColliderIsPermanentlyActive(collider))
			{
				collider->UpdateSleep(timeStep);
			}

			m_Level->UpdateObjectLocationAndRadius(levelIndex, &RCC_MATRIX34(collider->GetLastInstanceMatrix()));

			if (sm_ReportMovedBySim)
			{
				// Inform the instance that it may have moved.
				inst->ReportMovedBySim();
			}
	    }
    }

	m_Level->EndMultipleUpdates();

	PF_STOP(UpdateCollidersPost);

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR
	// Synchronize the octree again  Given that we also did this in PreCollide(), the instances in the deferred update list at this point should be pretty much exactly
	//   the physically active objects.  That's not quite true as objects can activate/deactivate and such and the clients might insert code that could change things too.
	InitiateCommitDeferredOctreeUpdatesTask();
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR

	if (sm_SleepEnabled)
	{
		m_ContactMgr->UpdateSleep();
	}

	// Deactivate all the contacts.
	m_ConstraintMgr->PostUpdate();

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR
	FlushCommitDeferredOctreeUpdatesTask();
#endif	// LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR

	PF_START(PostCollideActives);
	m_Level->PostCollideActives();
	PF_STOP(PostCollideActives);
}
#endif // !EARLY_FORCE_SOLVE

int phSimulator::AddActiveObject(phInst *pInst, bool bAlwaysActive)
{
	PF_FUNC(AddActiveObject);
	Assert(pInst);
	Assert(pInst->GetArchetype());
	Assert(pInst->GetArchetype()->GetMass() > 0.0f);
	Assert(pInst->GetArchetype()->GetBound());
	Assert(pInst->GetArchetype()->GetBound()->CanBecomeActive());

	phCollider* colliderToActivate = NULL;
	phInst* instToActivate = pInst->PrepareForActivation(&colliderToActivate, NULL, NULL);

	int nLevelIndex;
	phLevelNew *level = m_Level;
	if(instToActivate != NULL)
	{
		// The instance has not rejected activation.
		bool usingManagedCollider = false;

		if(colliderToActivate == NULL)
		{
			// The instance did not specify a collider to use so we'll need to grab one from our managed pool of phColliders.
			usingManagedCollider = true;
			if (m_NumUsedManagedColliders<m_MaxManagedColliders)
			{
				u32 uColliderIndex = m_AvailableColliderIndices[m_NumUsedManagedColliders];
				colliderToActivate = &m_ManagedColliders[uColliderIndex];
				Assert(colliderToActivate->GetSleep());
				m_ManagedColliderFlags[uColliderIndex] = bAlwaysActive ? (u8)(COLLIDERFLAG_PERMANENTLYACTIVE) : (u8)(0);
				Assert(colliderToActivate->GetInstance() == NULL);
				colliderToActivate->SetInstanceAndReset(instToActivate);
			}
			else
			{
#if __ASSERT
				for(int managedColliderIndex = 0; managedColliderIndex < m_MaxManagedColliders; ++managedColliderIndex)
				{
					const phCollider &curManagedCollider = m_ManagedColliders[managedColliderIndex];
					const phInst *curColliderInstance = curManagedCollider.GetInstance();
					if(curColliderInstance == NULL)
					{
						continue;
					}
					const phArchetype *curColliderArchetype = curColliderInstance->GetArchetype();
					if(curColliderArchetype == NULL)
					{
						continue;
					}
					Displayf("%3d: %5d %s", managedColliderIndex, curColliderInstance->GetLevelIndex(), curColliderArchetype->GetFilename());
				}
				Assertf(false, "Ignorable - the simulator was initialized with %i managed colliders and it's not enough.",m_MaxManagedColliders);
#endif

				return phInst::INVALID_INDEX;
			}
		}

		// One way or another, the collider that we're going to use should be hooked up to the instance that's going to get activated.
		Assert(colliderToActivate->GetInstance() == instToActivate);
		colliderToActivate->SetLastInstanceMatrix(level->GetLastInstanceMatrix(instToActivate));

		nLevelIndex = level->AddActiveObjectFromSimulator(instToActivate, colliderToActivate, true, sm_delaySAPAddRemove);

		// If we were successfully added to the level and we're using a managed collider, let's record that.
		if(nLevelIndex != phInst::INVALID_INDEX)
		{
			if (usingManagedCollider)
			{
				++m_NumUsedManagedColliders;
			}

			if (colliderToActivate->IsArticulated())
			{
				// This collider is articulated, so add it as a self overlapping pair the physics level's broadphase culler.
				level->GetBroadPhase()->addOverlappingPair((u16)nLevelIndex, (u16)nLevelIndex);
			}

			instToActivate->OnActivate(NULL);
		}
	}
	else
	{
		// The instance has rejected activation so just add the instance as an inactive object.
		Assert(colliderToActivate == NULL);
		nLevelIndex = level->AddInactiveObject(pInst, sm_delaySAPAddRemove);
	}


	if( sm_delaySAPAddRemove )
	{
		s_DelayedSAPAddRemoveToken.Lock();
		Assert( m_AddObjectDelayedSAPInst.GetCount() <= DELAYED_SAP_MAX-1 );
		if( m_AddObjectDelayedSAPInst.GetCount() == DELAYED_SAP_MAX-1 )
		{
			CommitDelayedObjectsSAP();
		}

		m_AddObjectDelayedSAPInst.Push( pInst );
		m_AddObjectDelayedSAPLevelIndex.Push( nLevelIndex );
		s_DelayedSAPAddRemoveToken.Unlock();
	}

	return nLevelIndex;
}


void phSimulator::AddActiveObjects(phInst **apInst, int nInst, int *levelIndex, bool bAlwaysActive)
{
	PF_FUNC(AddActiveObject);

	int nActive = 0;
	int nInactive = 0;
	phInst **active = Alloca( phInst *, nInst );
	phInst **inactive = Alloca( phInst *, nInst );
	phCollider **aColliderToActivate = Alloca( phCollider *, nInst );
	bool	*aUsingManagedCollider = Alloca( bool, nInst );
	sysMemSet( aUsingManagedCollider, 0, sizeof( bool )*nInst );
	sysMemSet( aColliderToActivate, 0, sizeof( phCollider * )*nInst );

	int iInst;
	for( iInst = 0; iInst < nInst; iInst++ )
	{
		phInst *pInst = apInst[iInst];

		Assert(pInst);
		Assert(pInst->GetArchetype());
		Assert(pInst->GetArchetype()->GetMass() > 0.0f);
		Assert(pInst->GetArchetype()->GetBound());
		Assert(pInst->GetArchetype()->GetBound()->CanBecomeActive());

		phCollider* colliderToActivate = NULL;
		phInst* instToActivate = pInst->PrepareForActivation(&colliderToActivate, NULL, NULL);

		if(instToActivate != NULL)
		{

			if(colliderToActivate == NULL)
			{
				// The instance did not specify a collider to use so we'll need to grab one from our managed pool of phColliders.
				aUsingManagedCollider[iInst] = true;
				Assert(m_NumUsedManagedColliders < m_MaxManagedColliders);
				u32 uColliderIndex = m_AvailableColliderIndices[m_NumUsedManagedColliders];
				colliderToActivate = &m_ManagedColliders[uColliderIndex];
				Assert(colliderToActivate->GetSleep());
				m_ManagedColliderFlags[uColliderIndex] = bAlwaysActive ? (u8)(COLLIDERFLAG_PERMANENTLYACTIVE) : (u8)(0);
				Assert(colliderToActivate->GetInstance() == NULL);
				colliderToActivate->SetInstanceAndReset(instToActivate);

				++m_NumUsedManagedColliders;
			}

			// One way or another, the collider that we're going to use should be hooked up to the instance that's going to get activated.
			Assert(colliderToActivate->GetInstance() == instToActivate);
			
			colliderToActivate->SetLastInstanceMatrix(PHLEVEL->GetLastInstanceMatrix(instToActivate));

			aColliderToActivate[nActive] = colliderToActivate;
			active[nActive] = instToActivate;
			nActive++;
		}
		else
		{
			inactive[nInactive] = pInst;
			nInactive++;
		}

	}

	if( levelIndex == NULL )
	{
		levelIndex = Alloca( int, nActive + nInactive );
	}

	// add the objects
	m_Level->AddObjects( active, nActive, levelIndex, phLevelBase::OBJECTSTATE_ACTIVE, (void **)aColliderToActivate );
	m_Level->AddObjects( inactive, nInactive, levelIndex+nActive, phLevelBase::OBJECTSTATE_INACTIVE );

	// clean up any rejects
	for( iInst = 0; iInst < nInst; iInst++ )
	{
		if( levelIndex[iInst] == phInst::INVALID_INDEX )
		{
			if( aUsingManagedCollider[iInst] )
			{
				size_t colliderIndex = aColliderToActivate[iInst] - &m_ManagedColliders[0];
				if (colliderIndex < (size_t)m_MaxManagedColliders)
				{
					// The object getting deactivated is one of the managed colliders, so return it to the pool.
					Assert(m_NumUsedManagedColliders > 0);
					--m_NumUsedManagedColliders;
					m_AvailableColliderIndices[m_NumUsedManagedColliders] = (u16)(colliderIndex);
					ASSERT_ONLY(aColliderToActivate[iInst]->SetInstanceAndReset(NULL));
				}
			}
		}
		else if (aColliderToActivate[iInst] && aColliderToActivate[iInst]->IsArticulated())
		{
			u16 levelIdx = aColliderToActivate[iInst]->GetInstance()->GetLevelIndex();
			Assert(levelIdx == levelIndex[iInst]);
			// This collider is articulated, so add it as a self overlapping pair the physics level's broadphase culler.
			m_Level->GetBroadPhase()->addOverlappingPair(levelIdx, levelIdx);
		}
	}

}

void phSimulator::AddFixedObjects( phInst **pInst, int nInst, int *levelIndex )
{
	if( levelIndex == NULL )
	{
		levelIndex = Alloca( int, nInst);
	}

	m_Level->AddObjects( pInst, nInst, levelIndex, phLevelBase::OBJECTSTATE_FIXED );
}

void phSimulator::AddInactiveObjects( phInst **pInst, int nInst, int *levelIndex )
{
	PF_FUNC(AddInactiveObject);
	if( levelIndex == NULL )
	{
		levelIndex = Alloca( int, nInst);
	}

	m_Level->AddObjects( pInst, nInst, levelIndex, phLevelBase::OBJECTSTATE_INACTIVE );
}

void phSimulator::CommitDelayedObjectsSAP()
{
	PF_FUNC(CommitDelayedObjectsSAP);

	if( m_RemoveObjectDelayedSAPLevelIndex.GetCount() > 0 )
	{
		m_Level->CommitDelayedDeleteObjects( &m_RemoveObjectDelayedSAPLevelIndex[0], m_RemoveObjectDelayedSAPLevelIndex.GetCount() );
	}

	m_RemoveObjectDelayedSAPLevelIndex.Resize(0);

	if( m_AddObjectDelayedSAPInst.GetCount() > 0 )
	{
		m_Level->AddToBroadphase( &m_AddObjectDelayedSAPInst[0], m_AddObjectDelayedSAPInst.GetCount(), &m_AddObjectDelayedSAPLevelIndex[0] );
	}
	
	m_AddObjectDelayedSAPInst.Resize(0);
	m_AddObjectDelayedSAPLevelIndex.Resize(0);

}

#if __PS3
phManifold::DmaPlan* phSimulator::GetDmaPlan(phManifold* manifold)
{
	u32 index = m_ManifoldPool->GetIndex(manifold);
	return &m_ManifoldDmaPlanPool[index];
}

phManifold* phSimulator::GetManifold(phManifold::DmaPlan* dmaPlan)
{
	u32 index = dmaPlan - m_ManifoldDmaPlanPool;
	return m_ManifoldPool->GetObject(index);
}

phCollider::DmaPlan* phSimulator::GetDmaPlan(phCollider* collider)
{
	u32 index = m_ManagedColliders - collider;
	return &m_ColliderDmaPlanPool[index];
}

phCollider* phSimulator::GetManifold(phCollider::DmaPlan* dmaPlan)
{
	u32 index = dmaPlan - m_ColliderDmaPlanPool;
	return &m_ManagedColliders[index];
}
#endif // __PS3

int phSimulator::AddActiveObject(phCollider* colliderPassedIn)
{
	PF_FUNC(AddActiveObject);
	Assert(colliderPassedIn);
	Assert(colliderPassedIn->GetInstance());
	Assert(colliderPassedIn->GetInstance()->GetArchetype());
	Assert(colliderPassedIn->GetInstance()->GetArchetype()->GetMass() > 0.0f);
	Assert(colliderPassedIn->GetInstance()->GetArchetype()->GetBound());
	Assert(colliderPassedIn->GetInstance()->GetArchetype()->GetBound()->CanBecomeActive());

	phCollider* colliderToActivate = NULL;
	phInst* instToActivate = colliderPassedIn->GetInstance()->PrepareForActivation(&colliderToActivate, NULL, NULL);
	int nLevelIndex;
	phInst* pInst = instToActivate;

	if (instToActivate)
	{
		// Make sure that, if we were passed in a collider to use AND the instance gave us a collider to use, the two colliders match.
		Assertf(!colliderToActivate || colliderPassedIn==colliderToActivate,"Call AddActiveObject(phInst*) instead because PrepareForActivation provides a collider for this instance.");

		// One way or another, the collider that we're going to use should be hooked up to the instance that's going to get activated.
		Assert(colliderPassedIn->GetInstance() == instToActivate);
		colliderPassedIn->SetLastInstanceMatrix(PHLEVEL->GetLastInstanceMatrix(instToActivate));

		// The instance has not rejected activation so go ahead and add the instance and its collider as an active object.
		nLevelIndex = m_Level->AddActiveObjectFromSimulator(instToActivate, colliderPassedIn, true,sm_delaySAPAddRemove);

		if (nLevelIndex != phInst::INVALID_INDEX)
		{
			if(colliderPassedIn->IsArticulated())
			{	
				// This collider is articulated, so add it as a self overlapping pair the physics level's broadphase culler.
				m_Level->GetBroadPhase()->addOverlappingPair((u16)nLevelIndex,(u16)nLevelIndex);
			}
			instToActivate->OnActivate(NULL);
		}
	}
	else
	{
		pInst = colliderPassedIn->GetInstance();
		// The instance has rejected activation so just add the instance as an inactive object.
		nLevelIndex = m_Level->AddInactiveObject(pInst,sm_delaySAPAddRemove);
	}

	if( sm_delaySAPAddRemove )
	{
		s_DelayedSAPAddRemoveToken.Lock();
		Assert( m_AddObjectDelayedSAPInst.GetCount() <= DELAYED_SAP_MAX-1 );
		if( m_AddObjectDelayedSAPInst.GetCount() == DELAYED_SAP_MAX-1 )
		{
			CommitDelayedObjectsSAP();
		}

		m_AddObjectDelayedSAPInst.Push( pInst );
		m_AddObjectDelayedSAPLevelIndex.Push( nLevelIndex );
		s_DelayedSAPAddRemoveToken.Unlock();
	}

	return nLevelIndex;
}


int phSimulator::AddInactiveObject(phInst *pInst)
{
	PF_FUNC(AddInactiveObject);
	Assert(pInst);
	Assert(pInst->GetArchetype());
	Assert(pInst->GetArchetype()->GetBound());
	Assert(pInst->GetArchetype()->GetBound()->CanBecomeActive() || pInst->GetInstFlag(phInst::FLAG_NEVER_ACTIVATE));

	int levelIndex = m_Level->AddInactiveObject(pInst, sm_delaySAPAddRemove);

	if( sm_delaySAPAddRemove )
	{
		s_DelayedSAPAddRemoveToken.Lock();
		Assert( m_AddObjectDelayedSAPInst.GetCount() <= DELAYED_SAP_MAX-1 );
		if( m_AddObjectDelayedSAPInst.GetCount() == DELAYED_SAP_MAX-1 )
		{
			CommitDelayedObjectsSAP();
		}

		m_AddObjectDelayedSAPInst.Push( pInst );
		m_AddObjectDelayedSAPLevelIndex.Push( levelIndex );
		s_DelayedSAPAddRemoveToken.Unlock();
	}

	return levelIndex;
}


int phSimulator::AddFixedObject(phInst *pInst)
{
	PF_FUNC(AddFixedObject);

	int levelIndex = m_Level->AddFixedObject(pInst, sm_delaySAPAddRemove);

	if( sm_delaySAPAddRemove )
	{
		s_DelayedSAPAddRemoveToken.Lock();
		Assert( m_AddObjectDelayedSAPInst.GetCount() <= DELAYED_SAP_MAX-1 );
		if( m_AddObjectDelayedSAPInst.GetCount() == DELAYED_SAP_MAX-1 )
		{
			CommitDelayedObjectsSAP();
		}

		m_AddObjectDelayedSAPInst.Push( pInst );
		m_AddObjectDelayedSAPLevelIndex.Push( levelIndex );
		s_DelayedSAPAddRemoveToken.Unlock();
	}

	return levelIndex;
}


bool phSimulator::GetObjectVelocity (const phInst& instance, Vector3& velocity) const
{
	// See if the given instance is in the physics level.
	if (instance.IsInLevel())
	{
		// The instance is in the physics level, so see if it has a collider.
		if (phCollider* collider = GetCollider(&instance))
		{
			// The instance is physically active in the level, so get its collider's velocity.
			velocity.Set(RCC_VECTOR3(collider->GetVelocity()));
		}
		else
		{
			// The instance is not physically active, so get any controlling velocity it has.
			velocity = VEC3V_TO_VECTOR3(instance.GetExternallyControlledVelocity());
		}

		// Return true to indicate that the instance is in the physics level.
		return true;
	}
	
	// The instance not in the physics level. Get any controlling velocity it has.
	velocity = VEC3V_TO_VECTOR3(instance.GetExternallyControlledVelocity());

	// Return false to indicate that the instance is not in the physics level.
	return false;
}


bool phSimulator::GetObjectAngVelocity (const phInst& instance, Vector3& angVelocity) const
{
	// See if the given instance is in the physics level.
	if (instance.IsInLevel())
	{
		// The instance is in the physics level, so see if it has a collider.
		if (phCollider* collider = GetCollider(&instance))
		{
			// The instance is physically active in the level, so get its collider's velocity.
			angVelocity.Set(RCC_VECTOR3(collider->GetAngVelocity()));
		}
		else
		{
			// The instance is not physically active, so get any controlling velocity it has.
			angVelocity = VEC3V_TO_VECTOR3(instance.GetExternallyControlledAngVelocity());
		}

		// Return true to indicate that the instance is in the physics level.
		return true;
	}
	
	// The instance not in the physics level. Get any controlling velocity it has.
	angVelocity = VEC3V_TO_VECTOR3(instance.GetExternallyControlledAngVelocity());

	// Return false to indicate that the instance is not in the physics level.
	return false;
}


bool phSimulator::GetObjectLocalVelocity(Vec3V_InOut localVelocity, const phInst& instance, Vec3V_In position, int component) const
{
	// See if the given instance is in the physics level.
	if (instance.IsInLevel())
	{
		// The instance is in the physics level, so see if it has a collider.
		if (phCollider* collider = GetCollider(&instance))
		{
			// The instance is physically active in the level, so get its collider's velocity.
			localVelocity = collider->GetLocalVelocity(position.GetIntrin128(), component);
		}
		else
		{
			// The instance is not physically active, so get any controlling velocity it has.
			localVelocity = instance.GetExternallyControlledLocalVelocity(position.GetIntrin128(), component);
		}

		// Return true to indicate that the instance is in the physics level.
		return true;
	}

	// The instance not in the physics level. Get any controlling velocity it has.
	localVelocity = instance.GetExternallyControlledLocalVelocity(position.GetIntrin128(), component);

	return false;
}

bool phSimulator::GetObjectLocalVelocity(Vector3& localVelocity, const phInst& instance, const Vector3& position, int component) const
{
	return GetObjectLocalVelocity(RC_VEC3V(localVelocity), instance, VECTOR3_TO_VEC3V(position), component);
}


bool phSimulator::TeleportObject (phInst& instance, const Matrix34& newMatrix, const Matrix34* newLastMatrix)
{
	PH_DEBUG_CONTEXT(&instance);

	// Get the last matrix, using the new matrix if it is not specified.
	const Matrix34* lastMatrix = (newLastMatrix ? newLastMatrix : &newMatrix);
	Assert(lastMatrix);

	// Go through all the overlapping pairs and clear all the manifolds that refer to this instance, since the
	// points they contain will surely not be useful where we are taking it. Worse, they are likely to produce
	// extremely deep contact points that could move us to the wrong place altogether.
	if(newLastMatrix != NULL)
	{
		for (int pairIndex = 0; pairIndex < m_OverlappingPairArray->pairs.GetCount(); ++pairIndex)
		{
			if (phManifold* manifold = m_OverlappingPairArray->pairs[pairIndex].manifold)
			{
				if (manifold->GetLevelIndexA()==instance.GetLevelIndex() || manifold->GetLevelIndexB()==instance.GetLevelIndex())
				{
					manifold->RemoveAllContacts();
				}
			}
		}
	}

	return MoveObject(instance, RCC_MAT34V(newMatrix), RCC_MAT34V(*lastMatrix));
}


bool phSimulator::MoveObject (phInst& instance, Mat34V_In newMatrix, Mat34V_In newLastMatrix)
{
	PH_DEBUG_CONTEXT(&instance);

	// We need the set the last matrix before calling SetMatrix() 
	// in case newLastMatrix is a reference to phInst::m_Matrix.
	if(instance.IsInLevel() && instance.HasLastMatrix())
	{
		PHLEVEL->SetLastInstanceMatrix(&instance, newLastMatrix);
	}

	instance.SetMatrix( newMatrix );

	if (instance.IsInLevel())
	{
		// Get the instance's collider, if it has one.
		if (phCollider* collider = GetCollider(&instance))
		{
			// The instance is active in the physics level, so set the colliders new current and last matrices.
			collider->SetColliderMatrixFromInstance();
			collider->SetLastInstanceMatrix(newLastMatrix);
			collider->ClearNonPenetratingAfterTeleport();
		}

		// Inform the physics level and the instance that the object moved.
		m_Level->UpdateObjectLocation(instance.GetLevelIndex(), &RCC_MATRIX34(newLastMatrix));
		instance.ReportMovedBySim();

		// Return true to indicate that the instance is in the physics level.
		return true;
	}

	// Return false to indicate that the instance is not in the physics level.
	return false;
}


void phSimulator::DeleteObjectHelper(int levelIndex)
{
	// Get the instance with this level index.
	PH_DEBUG_CONTEXT_ONLY(phInst* instance = m_Level->GetInstance(levelIndex));
	PH_DEBUG_CONTEXT(instance);

	AssertMsg(GetInstBehavior(levelIndex) == NULL, "Deleting object that still has an inst behavior");

	// Forcibly deactivate the collider before it gets removed from the level
	if(m_Level->IsActive(levelIndex))
	{
		DeactivateObject(levelIndex,true);
	}
}

void phSimulator::DeleteObject(phInst* pInst, bool forceImmediateDelete)
{
	DeleteObject(pInst->GetLevelIndex(),forceImmediateDelete);
}
void phSimulator::DeleteObject(int levelIndex, bool forceImmediateDelete)
{
	PF_FUNC(DeleteObject);
	PH_DEBUG_CONTEXT(levelIndex);

	Assert(IsSafeToDelete());

	// If the user has an instance with a seemingly valid level index that doesn't exist in the level it probably means there is memory corruption going on.
	if(!Verifyf(m_Level->LegitLevelIndex(levelIndex) && m_Level->IsInLevel(levelIndex) && m_Level->GetInstance(levelIndex),"phSimulator::DeleteObject - Passed invalid level index"))
	{
		NOTFINAL_ONLY(Quitf("phSimulator::DeleteObject - Passed invalid level index"));
		return;
	}

	DeleteObjectHelper( levelIndex );
	// Delete the object from the physics level.
	bool delay = !forceImmediateDelete && sm_delaySAPAddRemove;
	if (m_Level->GetBroadPhase()->isHandleAdded((u16)levelIndex))
	{
		m_Level->DeleteObject(levelIndex, delay);
		if( delay )
		{
			s_DelayedSAPAddRemoveToken.Lock();
			Assert( m_RemoveObjectDelayedSAPLevelIndex.GetCount() <= DELAYED_SAP_MAX-1 );
			if( m_RemoveObjectDelayedSAPLevelIndex.GetCount() == DELAYED_SAP_MAX-1 )
			{
				CommitDelayedObjectsSAP();
			}

			m_RemoveObjectDelayedSAPLevelIndex.Push(levelIndex);
			s_DelayedSAPAddRemoveToken.Unlock();
		}
	}	
	else
	{
		PF_START(DeleteObjectSearch);
		s_DelayedSAPAddRemoveToken.Lock();
		Assert( m_AddObjectDelayedSAPLevelIndex.GetCount() <= DELAYED_SAP_MAX-1 );
		int delayed = m_AddObjectDelayedSAPLevelIndex.Find( levelIndex );
		PF_STOP(DeleteObjectSearch);

		Assert(delayed >= 0);
		m_Level->DeleteObject1( levelIndex );
		m_AddObjectDelayedSAPLevelIndex.DeleteFast( delayed );
		m_AddObjectDelayedSAPInst.DeleteFast( delayed );
		s_DelayedSAPAddRemoveToken.Unlock();
		m_Level->DeleteObject2( levelIndex );
		m_Level->ReturnObjectIndexToPool((u16)(levelIndex));
	}
}

void phSimulator::DeleteObjects(int *levelIndex, int nLevelIndex)
{
	PF_FUNC(DeleteObject);

	int iLevel;
	for( iLevel = 0; iLevel < nLevelIndex; iLevel++ )
	{
		DeleteObjectHelper( levelIndex[iLevel] );
	}
	// Delete the object from the physics level.
	m_Level->DeleteObjects(levelIndex, nLevelIndex);
}

phCollider* phSimulator::ActivateObject(int levelIndex, phInst* instToActivate, phCollider* colliderToActivate, phInst* otherInst, const phConstraintBase * constraint)
{
	PHLOCK_SCOPEDWRITELOCK;

	PH_DEBUG_CONTEXT(levelIndex);
	if (!m_Level->IsInactive(levelIndex))
	{
		// This object can't be activated because it's not inactive.
		return NULL;
	}

	if (!m_Level->CheckAddActiveObjects(1))
	{
		// The level can't take any more active objects.
		return NULL;
	}

	phInstBehavior* instBehavior = GetInstBehavior(levelIndex);
	if (instBehavior && !instBehavior->ActivateWhenHit())
	{
		// The instance behavior won't let this object activate
		return NULL;
	}

	ASSERT_ONLY(const phCollider *colliderPassedIn = colliderToActivate);

	// If phInst::PrepareForActivation somehow calls ActivateInstance we will end up with duplicated level indices in the active list which will eventually crash when only one of the indices is removed
	//   and the other references a NULL collider. I'm sure there are other issues with duplicated active instances that we just don't know about. Preventing double activation here seems like the safest/simplest 
	//   solution. 
	static phInst* s_PrepareForActivationInst = NULL;
	if(!Verifyf(s_PrepareForActivationInst != instToActivate,"Calling phSimulator::ActivateObject '%s' from inside phInst::PrepareForActivation, bailing out.",instToActivate->GetArchetype()->GetFilename()))
	{
		return NULL;
	}
#if __DEV
	if(s_PrepareForActivationInst)
	{
		// It shouldn't be fatal to activate a different instance than the one that's currently being activated but it's not ideal. 
		sysStack::PrintStackTrace();
		Warningf("Calling phSimulator::ActivateObject('%s',0x%p) from inside phInst::PrepareForActivation('%s',0x%p).",instToActivate->GetArchetype()->GetFilename(),instToActivate,s_PrepareForActivationInst->GetArchetype()->GetFilename(),s_PrepareForActivationInst);
	}
#endif 
	s_PrepareForActivationInst = instToActivate;
	instToActivate = instToActivate->PrepareForActivation(&colliderToActivate, otherInst, constraint);
	s_PrepareForActivationInst = NULL;

	// Make sure that, if we were passed in a collider to use AND the instance gave us a collider to use, the two colliders match.
	Assert(colliderPassedIn == NULL || colliderToActivate == NULL || colliderPassedIn == colliderToActivate);

	if (instToActivate == NULL)
	{
		// This instance doesn't want to be activated.  That's cool.
		return NULL;
	}

	// If this fails, then the above PrepareForActivation added objects and didn't leave room for the primary object to be activated.
	// We can't just abort the activation because the instance might be left in an inappropriate state so the offending PrepareForActivation needs to be fixed
	//   to ensure that we can continue here.
	Assert(m_Level->CheckAddActiveObjects(1));

	Assertf(!instToActivate->GetInstFlag(phInst::FLAG_NEVER_ACTIVATE), "ActivateObject was called on an inst %s with FLAG_NEVER_ACTIVATE, PrepareForActivation should return NULL", instToActivate->GetArchetype() ? instToActivate->GetArchetype()->GetFilename(): "NONAME" );

	AssertMsg(colliderToActivate == NULL || colliderToActivate->GetSleep() == NULL || colliderToActivate->GetSleep()->IsAwake(), "instToActivate->PrepareForActivation returned a custom collider which is sleeping and may not properly activate");

	if (colliderToActivate == NULL)
	{
		// The instance didn't give us a specific collider that it wanted to use, so we'll need to give it one of our own.
		if (m_NumUsedManagedColliders < m_MaxManagedColliders)
		{
			colliderToActivate = &m_ManagedColliders[m_AvailableColliderIndices[m_NumUsedManagedColliders]];
			Assert(colliderToActivate->GetSleep());
			Assert(colliderToActivate->GetInstance() == NULL);
			// Should we just call an Init on the collider here?
			colliderToActivate->SetInstanceAndReset(instToActivate);
			++m_NumUsedManagedColliders;
		}
		else
		{
			Assertf(0,"Ignorable - the simulator was initialized with %i colliders and it's not enough.",m_MaxManagedColliders);
			return NULL;
		}
	}

	// One way or another, the collider that we're going to use should be hooked up to the instance that's going to get activated.
	Assert(colliderToActivate->GetInstance() == instToActivate);

	colliderToActivate->SetLastInstanceMatrix(PHLEVEL->GetLastInstanceMatrix(instToActivate));

	Assert(colliderToActivate != NULL);

	ASSERT_ONLY(bool instanceWasActivated =)
	m_Level->ActivateObject(levelIndex, colliderToActivate, false);
	Assert(instanceWasActivated);
	
	//Notify the instance that it has been activated.
	instToActivate->OnActivate(otherInst);

	if (colliderToActivate->IsArticulated())
	{
		// This collider is articulated, so add it as a self overlapping pair the physics level's broadphase culler.
		// The physics level's ActivateObject above adds broadphase overlapping pairs, but not self-overlapping pairs for articualted bodies.
		m_Level->GetBroadPhase()->addOverlappingPair((u16)levelIndex,(u16)levelIndex);
	}

	if (m_ContactMgr->GetUseSleepIslands())
	{
		if (phSleepIsland* sleepIsland = PHSLEEP->GetSleepIsland((u16)levelIndex))
		{
			sleepIsland->WakeObjects();
		}
	}

	ASSERT_ONLY(colliderToActivate->ValidateInstanceMatrixAlignedWithCollider();)

	TRACK_PUSH_COLLIDERS_ONLY(Assert(!colliderToActivate->GetIsInPushPair());)

	return colliderToActivate;
}

phCollider* phSimulator::ActivateObject (int levelIndex, phCollider* colliderToActivate, phInst* otherInst, const phConstraintBase * constraint)
{
	// Get the instance to deactivate.
	phInst* pInst = m_Level->GetInstance(levelIndex);
	return ActivateObject(levelIndex, pInst, colliderToActivate, otherInst, constraint);
}

phCollider* phSimulator::ActivateObject (phInst* instToActivate, phCollider* colliderToActivate, phInst* otherInst, const phConstraintBase * constraint)
{
	// Confirm the level's phInst for this index matches the inst's level index.
	int knLevelIndex = instToActivate->GetLevelIndex();
	Assert(instToActivate ==  m_Level->GetInstance(knLevelIndex));
	return ActivateObject(knLevelIndex, instToActivate, colliderToActivate, otherInst, constraint);
}


void phSimulator::DeactivateObject(const int knLevelIndex, phInst* pInst, bool forceDeactivate)
{
	PHLOCK_SCOPEDWRITELOCK;

	// Let's ensure that the object actually is active.
	Assert(m_Level->IsActive(knLevelIndex));

	// Get the collider, and see if it is managed by the simulator.
	phCollider* collider = (phCollider*)(m_Level->GetUserData(knLevelIndex));
	bool managedBySim = ColliderIsManagedBySimulator(collider);

	// Notify the instance that it is being deactivated and see if that's okay with it.
	if(pInst->PrepareForDeactivation(managedBySim,forceDeactivate) || forceDeactivate)
	{
		// collider might have changed (fragInstNM switches to a managed collider) so make sure we've got the right one
		collider = (phCollider*)(m_Level->GetUserData(knLevelIndex));
		managedBySim = ColliderIsManagedBySimulator(collider);
		if (managedBySim)
		{
			// The object getting deactivated is one of our managed colliders.  Let's return it to our pool.
			Assert(m_NumUsedManagedColliders > 0);
			--m_NumUsedManagedColliders;
			size_t colliderIndex = collider - &m_ManagedColliders[0];
			m_AvailableColliderIndices[m_NumUsedManagedColliders] = (u16)(colliderIndex);

			// In assert builds, mark the collider as having been recycled
			ASSERT_ONLY(collider->SetInstanceAndReset(NULL));
		}

		m_Level->DeactivateObject(knLevelIndex);
		
		//Notify the instance that it has been deactivated.
		pInst->OnDeactivate();
	}
}

void phSimulator::DeactivateObject(const int knLevelIndex, bool forceDeactivate)
{
	// Get the instance to deactivate.
	phInst* pInst = m_Level->GetInstance(knLevelIndex);
	DeactivateObject(knLevelIndex, pInst, forceDeactivate);
}

void phSimulator::DeactivateObject(phInst* pInst, bool forceDeactivate)
{
	// Confirm the level's phInst for this index matches the inst's level index.
	int knLevelIndex = pInst->GetLevelIndex();
	Assert(pInst ==  m_Level->GetInstance(knLevelIndex));
	DeactivateObject(knLevelIndex, pInst, forceDeactivate);
}

void phSimulator::ActivateAll()
{
#if ENABLE_PHYSICS_LOCK
	phIterator iterator(phIterator::PHITERATORLOCKTYPE_WRITELOCK);
#else	// ENABLE_PHYSICS_LOCK
	phIterator iterator;
#endif	// ENABLE_PHYSICS_LOCK
	iterator.InitCull_All();
	iterator.SetStateIncludeFlags(phLevelBase::STATE_FLAG_INACTIVE);

	u16 levelIndex = iterator.GetFirstLevelIndex(m_Level);
	while(levelIndex != (u16)(-1))
	{
		ActivateObject(levelIndex);

		levelIndex = iterator.GetNextLevelIndex(m_Level);
	}
}

#if __PFDRAW
float g_LastPhysicsTimeStep = 0.0f;
#endif

#if EARLY_FORCE_SOLVE
void phSimulator::IntegrateVelocities(ScalarV_In timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::IntegrateVelocities");
	PF_FUNC(IntegrateVelocities);

	static sysTaskParameters params ;
	FillInColliderUpdateParams(params, PHCOLLIDER_INTEGRATE_VELOCITIES, timeStep.Getf());

	m_UpdateCollidersTasks->Initiate(params, phConfig::GetIntegrateVelocitiesWorkerThreadCount());

	if (phConfig::GetIntegrateVelocitiesMainThreadAlsoWorks())
	{
		UpdateCollidersTask(params);
	}

	m_UpdateCollidersTasks->Wait();
}

#define REFRESH_INST_AND_COLLIDER_POINTERS_BY_OVERLAPPING_PAIRS 1

#if REFRESH_INST_AND_COLLIDER_POINTERS_BY_OVERLAPPING_PAIRS
void phSimulator::RefreshInstAndColliderPointers(float /*timeStep*/)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::RefreshInstAndColliderPointers");
	PF_FUNC(RefreshInstAndColliderPointers);

	phOverlappingPairArray::PairArray& pairArray = GetOverlappingPairArray()->pairs;
	const int numPairs = pairArray.GetCount();

	atArray<phManifold*>& externVels = m_ContactMgr->GetManifoldsWithExternVelocity();

	if (Likely(numPairs > 1))
	{
		PrefetchObject(&pairArray[1]);
	}

	for (int pairIndex = 0; pairIndex < numPairs; ++pairIndex)
	{
		int nextPairIndex = pairIndex + 1;
		if (Likely(nextPairIndex < numPairs))
		{
			PrefetchObject(pairArray[nextPairIndex].manifold);

			int nextNextPairIndex = pairIndex + 2;
			if (Likely(nextNextPairIndex < numPairs))
			{
				PrefetchObject(&pairArray[nextNextPairIndex]);
			}
		}

		phTaskCollisionPair& pair = pairArray[pairIndex];

		if (phManifold* manifold = pair.manifold)
		{
			int levelIndexA = manifold->GetLevelIndexA();
			int generationIdA = manifold->GetGenerationIdA();
			phInst* instA = NULL;
			phCollider* colliderA = NULL;
			u32 instFlagsA = 0;
			if (levelIndexA != phInst::INVALID_INDEX)
			{
				if (!PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndexA, generationIdA))
				{
					pair.manifold = NULL;
					continue;
				}

				instA = PHLEVEL->GetInstance(levelIndexA);
				colliderA = GetCollider(levelIndexA);
				instFlagsA = instA->GetInstFlags();
			}

			int levelIndexB = manifold->GetLevelIndexB();
			int generationIdB = manifold->GetGenerationIdB();

			phInst* instB = NULL;
			phCollider* colliderB = NULL;
			u32 instFlagsB = 0;
			if (levelIndexB != phInst::INVALID_INDEX)
			{
				if (!PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndexB, generationIdB))
				{
					pair.manifold = NULL;
					continue;
				}

				instB = PHLEVEL->GetInstance(levelIndexB);
				colliderB = GetCollider(levelIndexB);
				instFlagsB = instB->GetInstFlags();
			}

			u32 instFlagsCombinedOr = (instFlagsA | instFlagsB);

			// Save manifold if one of the bodies wants its externally controlled 
			// velocity queried, so it can be called later.
			if(Unlikely((instFlagsCombinedOr & phInst::FLAG_QUERY_EXTERN_VEL) != 0))
			{
				if (Verifyf(externVels.GetCount() < externVels.GetCapacity(), "Extern velocity manifolds array is full (%d items)", externVels.GetCapacity()))
				{
					externVels.Push(manifold);
				}
			}

#if PROPHYLACTIC_SWAPS
			if (Likely(g_ProphylacticSwaps))
			{
				// The first condition below, (colliderB && !colliderA), is at least necessary due to 
				// a bug in code related to phTaskCollisionPair::Exchange().  If this is removed, a ragdoll 
				// colliding with the ground plane(with levelIndex 0), does not behave correctly (ragdoll 
				// gets accelerated through the ground). Exchange() seems to behave correctly when 
				// it is run on the ppu, which would point to something not getting back mm, but it 
				// all looks correct.
				if (colliderB && !colliderA ||
					(colliderA && colliderB && colliderB->IsArticulated() && !colliderA->IsArticulated()))
				{
					manifold->Exchange();

					SwapEm(levelIndexA, levelIndexB);
					SwapEm(instA, instB);
					SwapEm(colliderA, colliderB);
				}
			}
#endif // PROPHYLACTIC_SWAPS

			manifold->SetColliderA(colliderA);
			manifold->SetColliderB(colliderB);

			if(manifold->CompositeManifoldsEnabled())
			{
				// Search the sub-manifolds for contact points.
				for(int compositeManifoldIndex = 0; compositeManifoldIndex < manifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
				{
					phManifold* compositeManifold = manifold->GetCompositeManifold(compositeManifoldIndex);
					compositeManifold->SetColliderA(colliderA);
					compositeManifold->SetColliderB(colliderB);
				}
			}
		}
	}
}
#else // REFRESH_INST_AND_COLLIDER_POINTERS_BY_OVERLAPPING_PAIRS
void phSimulator::RefreshInstAndColliderPointers(float timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::RefreshInstAndColliderPointers");
	PF_FUNC(RefreshInstAndColliderPointers);

	phBroadPhase* broadPhase = m_Level->GetBroadPhase();
	btBroadphasePair* broadPhasePairs = broadPhase->getPrunedPairs();
	const int numPrunedPairs = broadPhase->getPrunedPairCount();
	phOverlappingPairArray* pairArray = GetOverlappingPairArray();

	pairArray->pairs.Resize(0);
	pairArray->numCollisionPairs = 0;

	for (int pairIndex = 0; pairIndex < numPrunedPairs; ++pairIndex)
	{
		btBroadphasePair& pair = broadPhasePairs[pairIndex];

		int levelIndex0 = pair.GetObject0();
		int generationId0 = pair.GetGenId0();

		int levelIndex1 = pair.GetObject1();
		int generationId1 = pair.GetGenId1();

		if (levelIndex0 != phInst::INVALID_INDEX && !PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndex0, generationId0) ||
			levelIndex1 != phInst::INVALID_INDEX && !PHLEVEL->IsLevelIndexGenerationIDCurrent(levelIndex1, generationId1))
		{
			static_cast<btAxisSweep3*>(m_Level->GetBroadPhase())->removeOverlappingPair(&pair);
			continue;
		}
		else if (phManifold* manifold = pair.GetManifold())
		{
			u16 levelIndexA = (u16)manifold->GetLevelIndexA();
			u16 levelIndexB = (u16)manifold->GetLevelIndexB();

			phInst* instA = NULL;
			phCollider* colliderA = NULL;
			if (levelIndexA != phInst::INVALID_INDEX)
			{
				instA = PHLEVEL->GetInstance(levelIndexA);
				colliderA = PHSIM->GetCollider(levelIndexA);
			}

			phInst* instB = NULL;
			phCollider* colliderB = NULL;
			if (levelIndexB != phInst::INVALID_INDEX)
			{
				instB = PHLEVEL->GetInstance(levelIndexB);
				colliderB = PHSIM->GetCollider(levelIndexB);
			}

#if PROPHYLACTIC_SWAPS
			if (Likely(g_ProphylacticSwaps))
			{
				// The first condition below, (colliderB && !colliderA), is at least necessary due to 
				// a bug in code related to phTaskCollisionPair::Exchange().  If this is removed, a ragdoll 
				// colliding with the ground plane(with levelIndex 0), does not behave correctly (ragdoll 
				// gets accelerated through the ground). Exchange() seems to behave correctly when 
				// it is run on the ppu, which would point to something not getting back mm, but it 
				// all looks correct.
				if (colliderB && !colliderA ||
					(colliderA && colliderB && colliderB->IsArticulated() && !colliderA->IsArticulated()))
				{
					manifold->Exchange();

					SwapEm(levelIndexA, levelIndexB);
					SwapEm(instA, instB);
					SwapEm(colliderA, colliderB);
				}
			}
#endif // PROPHYLACTIC_SWAPS

			manifold->SetColliderA(colliderA);
			manifold->SetColliderB(colliderB);

			if(manifold->CompositeManifoldsEnabled())
			{
				// Search the sub-manifolds for contact points.
				for(int compositeManifoldIndex = 0; compositeManifoldIndex < manifold->GetNumCompositeManifolds(); ++compositeManifoldIndex)
				{
					phManifold* compositeManifold = manifold->GetCompositeManifold(compositeManifoldIndex);
					compositeManifold->SetColliderA(colliderA);
					compositeManifold->SetColliderB(colliderB);
				}
			}

			phTaskCollisionPair& pair = pairArray->pairs.Append();
			pair.inst1 = instA;
			pair.levelIndex1 = levelIndexA;
			pair.collider1 = colliderA;
			pair.inst2 = instB;
			pair.levelIndex2 = levelIndexB;
			pair.collider2 = colliderB;
			pair.manifold = manifold;
			pair.wasExchanged = false;
			sys_lwsync();
			pairArray->numCollisionPairs++;
		}
	}

	m_ConstraintMgr->UpdateConstraintContacts(1.0f / timeStep, true);
}
#endif // REFRESH_INST_AND_COLLIDER_POINTERS_BY_OVERLAPPING_PAIRS

void phSimulator::VelocitySolve(ScalarV_In timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::VelocitySolve");
	PF_FUNC(VelocitySolve);
	PPC_STAT_TIMER_SCOPED(VelocitySolveTimer);

	m_ContactMgr->SetUsePushes(false);
	m_ContactMgr->Update(m_OverlappingPairArray, timeStep.Getf());
	m_ContactMgr->SetUsePushes(true);
}

void phSimulator::IntegratePositions(ScalarV_In timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::IntegratePositions");
	PF_FUNC(IntegratePositions);

	static sysTaskParameters params ;
	FillInColliderUpdateParams(params, PHCOLLIDER_INTEGRATE_POSITIONS, timeStep.Getf());

	m_UpdateCollidersTasks->Initiate(params, phConfig::GetIntegratePositionsWorkerThreadCount());

	if (phConfig::GetIntegratePositionsMainThreadAlsoWorks())
	{
		UpdateCollidersTask(params);
	}

	m_UpdateCollidersTasks->Wait();

	PF_START(IntegratePositionsCallbacks);

	for( int iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
	{
		int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 
		phCollider* collider = GetActiveCollider(levelIndex);
		Assert(collider);

		phInst* inst = collider->GetInstance();

		m_Level->UpdateObjectLocationAndRadius(levelIndex, &RCC_MATRIX34(collider->GetLastInstanceMatrix()));

		if (sm_ReportMovedBySim)
		{
			// Inform the instance that it may have moved.
			inst->ReportMovedBySim();
		}
	}

	PF_STOP(IntegratePositionsCallbacks);
}

void phSimulator::PushSolve(ScalarV_In timeStep, phOverlappingPairArray* pairArray)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::PushSolve");
	PF_FUNC(PushSolve);
	PPC_STAT_TIMER_SCOPED(PushSolveTimer);
	PPC_STAT_COUNTER_INC(PushSolveCounter,1);

#if TRACK_PUSH_COLLIDERS
	// Loop over the push pairs and mark the colliders as being in push pairs so we now which ones to update later
	for(int pushPairIndex = 0; pushPairIndex < pairArray->numCollisionPairs; ++pushPairIndex)
	{
		if(phManifold* pManifold = pairArray->pairs[pushPairIndex].manifold)
		{
			if(phCollider* pColliderA = pManifold->GetColliderA())
			{
				pColliderA->SetIsInPushPair(true);
			}
			if(phCollider* pColliderB = pManifold->GetColliderB())
			{
				pColliderB->SetIsInPushPair(true);
			}
		}
	}
#endif // TRACK_PUSH_COLLIDERS
	
	phArticulatedCollider::SaveAndZeroAllVelocitiesForPushes();

	m_ContactMgr->Update(pairArray, timeStep.Getf());
}

#if __PFDRAW
int g_PushCollisionCount = 0;
int g_TurnCollisionCount = 0;
#endif

void phSimulator::ApplyPushes(ScalarV_In timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::ApplyPushes");
	PF_FUNC(ApplyPushes);
	PPC_STAT_TIMER_SCOPED(ApplyPushesTimer);

	PF_START(ApplyPushesJobs);

	PF_DRAW_ONLY(g_PushCollisionCount = 0;)
	PF_DRAW_ONLY(g_TurnCollisionCount = 0;)

	static sysTaskParameters params ;
	FillInColliderUpdateParams(params, PHCOLLIDER_APPLY_PUSHES, timeStep.Getf());

	m_UpdateCollidersTasks->Initiate(params, phConfig::GetApplyPushesWorkerThreadCount());

	if (phConfig::GetApplyPushesMainThreadAlsoWorks())
	{
		UpdateCollidersTask(params);
	}

	m_UpdateCollidersTasks->Wait();

	PF_STOP(ApplyPushesJobs);
	PF_START(ApplyPushesCallbacks);

	for( int iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
	{
		int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 
		phCollider* collider = GetActiveCollider(levelIndex);
		Assert(collider);
#if TRACK_PUSH_COLLIDERS
		if(collider->GetIsInPushPair())
#endif // TRACK_PUSH_COLLIDERS
		{
			phInst* inst = collider->GetInstance();

			m_Level->UpdateObjectLocationAndRadius(levelIndex, &RCC_MATRIX34(collider->GetLastInstanceMatrix()));

			if (sm_ReportMovedBySim)
			{
				// Inform the instance that it may have moved.
				inst->ReportMovedBySim();
			}

			// Reset the flag for the next push solve
			// NOTE: This must be done after the ApplyPushes UpdateColliderTask which reads this flag
			TRACK_PUSH_COLLIDERS_ONLY(collider->SetIsInPushPair(false);)
		}
	}

	PF_STOP(ApplyPushesCallbacks);
	PF_START(ApplyPushesCommit);

	PF_STOP(ApplyPushesCommit);
}

#if __BANK
static int s_MaxPushPairs = DEFAULT_MAX_PUSH_PAIRS;
static int s_AllocatedPushPairs = DEFAULT_MAX_PUSH_PAIRS;
#endif // __BANK

extern float g_PushRadiasBias;

bool g_EnablePreventsPushCollisions = true;

class FindPushPairsFunctor
{
	phOverlappingPairArray* m_PushPairs;
	mutable int m_CurrentIsland;
	mutable int m_NumFoundPairs;
	mutable bool m_FoundPushPair;
	bool m_DoingPushCollision;

public:
	FindPushPairsFunctor(phOverlappingPairArray* pushPairs, bool doingPushCollision)
		: m_PushPairs(pushPairs)
		, m_CurrentIsland(0)
		, m_NumFoundPairs(0)
		, m_FoundPushPair(false)
		, m_DoingPushCollision(doingPushCollision)
	{
	}

	bool Process(phTaskCollisionPair& pair) const
	{
		// The + 1 is because if we allow the pair array to become completely full, we get crashes
		// TODO: figure out why it crashes in that case
		if (m_PushPairs->pairs.GetCount() + 1 < m_PushPairs->pairs.GetCapacity())
		{
			m_PushPairs->pairs.Append() = pair;

			phManifold* manifold = pair.manifold;

			// If we haven't found a push pair yet in this island, check these colliders
			if (!m_FoundPushPair || g_EnablePreventsPushCollisions)
			{
				if (m_DoingPushCollision)
				{
					if (phCollider* collider = manifold->GetColliderA())
					{
						if (collider->GetPreventsPushCollisions())
						{
							m_FoundPushPair = false;

							return true;
						}

						if (collider->GetNeedsCollision())
						{
#if __BANK
							if (g_PushCollisionTTY)
							{
								Displayf("Push collision caused by %s needing collision", collider->GetInstance()->GetArchetype()->GetFilename());
							}
#endif // __BANK

							m_FoundPushPair = true;

							return false;
						}
					}

					if (phCollider* collider = manifold->GetColliderB())
					{
						if (collider->GetPreventsPushCollisions())
						{
							m_FoundPushPair = false;

							return true;
						}

						if (collider->GetNeedsCollision())
						{
							m_FoundPushPair = true;

#if __BANK
							if (g_PushCollisionTTY)
							{
								Displayf("Push collision caused by %s needing collision", collider->GetInstance()->GetArchetype()->GetFilename());
							}
#endif // __BANK

							return false;
						}
					}
				}

				if (manifold->IsSelfCollision())
				{
					phCollider* collider = manifold->GetColliderA();
					if (collider->IsArticulated())
					{
						phArticulatedCollider* artCollider = static_cast<phArticulatedCollider*>(collider);
						if (artCollider->GetNumDeepJointLimitDofs() > 0)
						{
#if __BANK
							if (g_PushCollisionTTY && m_DoingPushCollision)
							{
								Displayf("Push collision caused by %d deep joint limits on %s", artCollider->GetNumDeepJointLimitDofs(), collider->GetInstance()->GetArchetype()->GetFilename());
							}
#endif // __BANK

							m_FoundPushPair = true;
						}
					}
				}
				else
				{
					float allowedPenetration = PHSIM->GetAllowedPenetration();
					phCollider* colliderA = manifold->GetColliderA();
					phCollider* colliderB = manifold->GetColliderB();
					if (colliderA)
					{
						if (m_DoingPushCollision)
						{
							allowedPenetration += colliderA->GetApproximateRadius() * g_PushRadiasBias;
						}
						if (!colliderB)
						{
							allowedPenetration += colliderA->GetExtraAllowedPenetration();
						}
					}
					if (colliderB)
					{
						if (m_DoingPushCollision)
						{
							allowedPenetration += colliderB->GetApproximateRadius() * g_PushRadiasBias;
						}
						if (!colliderA)
						{
							allowedPenetration += colliderB->GetExtraAllowedPenetration();
						}
					}
					if (manifold->HasDeepContacts(allowedPenetration, BANK_ONLY(g_PushCollisionTTY &&) m_DoingPushCollision))
					{
						m_FoundPushPair = true;
					}
				}
			}

			return false;
		}
		else
		{
			// We overflowed, so throw out this island
			m_FoundPushPair = false;

			return true;
		}
	}

	bool NextIsland() const
	{
		if (m_FoundPushPair)
		{
			// Commit the island to live in the pair array
			m_NumFoundPairs = m_PushPairs->pairs.GetCount();
		}
		else
		{
			// Erase the pairs that we collected from this island
			m_PushPairs->pairs.Resize(m_NumFoundPairs);
		}

		m_FoundPushPair = false;

		++m_CurrentIsland;

		return false;
	}
};


phOverlappingPairArray* phSimulator::FindPushPairs(phOverlappingPairArray* pairArray, bool doingPushCollision)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::FindPushPairs");
	PF_FUNC(FindPushPairs);

	if (pairArray->pairs.GetCount() == 0)
	{
		return NULL;
	}

#if __BANK
	if (Unlikely(sm_AlwaysPush && !doingPushCollision))
	{
		return pairArray;
	}

	if (Unlikely(s_AllocatedPushPairs < s_MaxPushPairs))
	{
		delete m_PushPairsA;
		delete m_PushPairsB;
		m_PushPairsA = rage_new phOverlappingPairArray(s_MaxPushPairs, s_MaxPushPairs * 2, m_Level->GetMaxObjects());
		m_PushPairsB = rage_new phOverlappingPairArray(s_MaxPushPairs, s_MaxPushPairs * 2, m_Level->GetMaxObjects());
		s_AllocatedPushPairs = s_MaxPushPairs;
	}
#endif // BANK

	phOverlappingPairArray* pushPairs = pairArray == m_PushPairsA ? m_PushPairsB : m_PushPairsA;
	pushPairs->Reset();

	pairArray->IterateIslandPairs(FindPushPairsFunctor(pushPairs, doingPushCollision));

	if (pushPairs->pairs.GetCount() > 0)
	{
#if __BANK
		if (g_PushCollisionTTY && doingPushCollision)
		{
			Displayf("Push collision %d: %d pairs (out of %d last iteration)", s_CurrentCollisionIteration, pushPairs->pairs.GetCount(), pairArray->pairs.GetCount());
		}
#endif // __BANK

		pushPairs->numCollisionPairs = pushPairs->pairs.GetCount();

		return pushPairs;
	}
	else
	{
		return NULL;
	}
}

// temporarily disable push collisions, so we can separate out the bugs due to EARLY_FORCE_SOLVE alone
int g_MaxPushCollisionIterations = 2;

void phSimulator::CoreUpdate(float timeStep, bool finalUpdate)
{
	PF_AUTO_PUSH_TIMEBAR("phSimulator::CoreUpdate");
	PF_FUNC(CoreUpdate);

	Assert(m_NumActivatingPairs == 0);

	ScalarV timeStepV(timeStep);

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(m_Level->GetBroadPhase());
	CheckForDuplicateManifolds(m_OverlappingPairArray);
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

	IntegrateVelocities(timeStepV);
	RefreshInstAndColliderPointers(timeStep);
	VelocitySolve(timeStepV);
	// We need to add objects to the broadphase after the velocity solve so broken
	//   objects don't get pruned instantly.
	if (sm_delaySAPAddRemove)
	{
		s_DelayedSAPAddRemoveToken.Lock();
		Assert( m_AddObjectDelayedSAPInst.GetCount() <= DELAYED_SAP_MAX-1 );
		Assert( m_RemoveObjectDelayedSAPLevelIndex.GetCount() <= DELAYED_SAP_MAX-1 );
		CommitDelayedObjectsSAP();
		s_DelayedSAPAddRemoveToken.Unlock();
	}

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(m_Level->GetBroadPhase());
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

	// But we also need to make sure that we add these objects before integrate positions
	//  Otherwise the SAP AABB is missing the first frame's motion expansion
	IntegratePositions(timeStepV);

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	// update/rebuild composite BVHs to reflect game side changes. Do this as late as possible since it requires a write lock and we want 
	//   current readers to finish (we prevent new readers above the core update)
	PHLEVEL->ProcessDeferredCompositeBvhUpdates();
#endif

	s_PushCollision = false;

	// Clear the pair array for the incoming pairs, so that we can add pairs during ProcessOverlaps
	phOverlappingPairArray* pairArray = m_OverlappingPairArray;

#if PH_MANIFOLD_TRACK_IS_IN_OPA
	for (int i = 0; i < m_ManifoldPool->GetInUseCount() + m_ManifoldPool->GetAvailCount(); ++i)
	{
		m_ManifoldPool->GetObject(i)->SetIsInOPA(false);
	}
#endif // PH_MANIFOLD_TRACK_IS_IN_OPA
	pairArray->pairs.Resize(0);
	pairArray->numCollisionPairs = 0;

#if __STATS
#define PUSH_COLLISION_STATS_ONLY(X) if (iteration > 0) X
#else
#define PUSH_COLLISION_STATS_ONLY(X)
#endif

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Broadphase prune");
	PF_START(PruneBroadphase);
	// Have the broadphase check for and remove pairs that should no longer be there.
	m_Level->GetBroadPhase()->pruneActiveOverlappingPairs();
	PF_STOP(PruneBroadphase);

	// This gets set to true inside of ApplyPushes if there are any colliders that need collision. Set it to true for the
	// first iteration since that's the only way to get into the loop.
	s_NeedsCollision = true;

	u32 nextStartingBroadphasePair = 0;
	for (int iteration = 0; s_NeedsCollision && iteration < g_MaxPushCollisionIterations; ++iteration)
	{
		// Set to false here so that each time we won't do another iteration unless we find we need to in ApplyPushes
		s_NeedsCollision = false;
		Assert(pairArray);

#if COLLISION_SWEEP_DRAW
		s_CurrentCollisionIteration = iteration;
#endif

		PUSH_COLLISION_STATS_ONLY(PF_START(PCCollisions));
		nextStartingBroadphasePair = CollideActive(pairArray, nextStartingBroadphasePair, timeStep);
		PUSH_COLLISION_STATS_ONLY(PF_STOP(PCCollisions));

		if (!sm_EnablePushes || !finalUpdate)
		{
			break;
		}

		pairArray->ComputeIslands(!s_PushCollision, !s_PushCollision);

		PUSH_COLLISION_STATS_ONLY(PF_START(PCFindPairs));
		pairArray = FindPushPairs(pairArray, iteration > 0);
		PUSH_COLLISION_STATS_ONLY(PF_STOP(PCFindPairs));

		if (pairArray)
		{
			PUSH_COLLISION_STATS_ONLY(PF_START(PCPushSolve));
			PushSolve(timeStepV, pairArray);
			PUSH_COLLISION_STATS_ONLY(PF_STOP(PCPushSolve));

			PUSH_COLLISION_STATS_ONLY(PF_START(PCApplyPushes));
			ApplyPushes(timeStepV);
			PUSH_COLLISION_STATS_ONLY(PF_STOP(PCApplyPushes));

#if CHECK_FOR_DUPLICATE_MANIFOLDS
			CheckForDuplicateManifolds(m_OverlappingPairArray);
			if (pairArray != m_OverlappingPairArray)
				CheckForDuplicateManifolds(pairArray);
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS
		}

		s_PushCollision = true;

		STATS_ONLY(if (iteration == 0) { PF_START(PushCollision); })
	}

	if (sm_EnablePushes && finalUpdate)
	{
		PF_STOP(PushCollision);
	}

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("UpdateLastMatricesFromCurrent");
	for( int iActiveIndex = m_Level->GetNumActive() - 1; iActiveIndex >= 0; --iActiveIndex )
	{
		int levelIndex = m_Level->GetActiveLevelIndex(iActiveIndex); 
		phCollider* collider = GetActiveCollider(levelIndex);
		Assert(collider);

		collider->UpdateLastMatrixFromCurrent(finalUpdate);
	}

	// Reset BreakingPairs list
	m_ContactMgr->ResetBreakingPairs(PHSIM->GetOverlappingPairArray());

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("UpdateSleep");
	if (sm_SleepEnabled && finalUpdate)
	{
		m_ContactMgr->UpdateSleep();
	}

#if USE_SIMULATOR_STATS1
	GatherStatistics();
#endif // USE_SIMULATOR_STATS1

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(m_OverlappingPairArray);
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS
}
#else // EARLY_FORCE_SOLVE
void phSimulator::CoreUpdate(float timeStep)
{
	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::CoreUpdate");
	PF_FUNC(CoreUpdate);

	if (sm_delaySAPAddRemove)
	{
		s_DelayedSAPAddRemoveToken.Lock();
		Assert( m_AddObjectDelayedSAPInst.GetCount() <= DELAYED_SAP_MAX-1 );
		Assert( m_RemoveObjectDelayedSAPLevelIndex.GetCount() <= DELAYED_SAP_MAX-1 );
		CommitDelayedObjectsSAP();
		s_DelayedSAPAddRemoveToken.Unlock();
	}

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	// update/rebuild composite BVHs to reflect game side changes. Do this as late as possible since it requires a write lock and we want 
	//   current readers to finish (we prevent new readers above the core update)
	PHLEVEL->ProcessDeferredCompositeBvhUpdates();
#endif

	PreCollide(timeStep);

#if __PFDRAW
	if (PFD_PrePush.WillDraw())
	{
		m_Level->ProfileDraw();
	}
#endif // __PFDRAW

	Collide(timeStep);

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Collisions done callback");
	m_AllCollisionsDoneCallback();

	PostCollide(timeStep);
}
#endif // EARLY_FORCE_SOLVE

#if PRIM_CACHE_RENDER
void PrimitiveRenderReset();
#endif

#if USER_JBN
int g_TotalUpdateCount = 0;
float g_TotalUpdateTime = 0;
#endif // USER_JBN

#if USE_PHYSICS_PROFILE_CAPTURE
bool g_StartPhysicsProfileCapture = false;
#endif // USE_PHYSICS_PROFILE_CAPTURE

#if USE_NEGATIVE_DEPTH_TUNABLE
bool g_UseNegativeDepth = true;
bool g_UseNegativeDepthForceCancel = false;
bool g_UseNegativeDepthForceCancel1 = false;
bool g_UseRemoveContact = true;
float g_NegativeDepthBias0 = 0.0f;
float g_NegativeDepthBias1 = 1.0f;
float g_NegativeDepthPenetrationBias = 1.0f;
float g_PositiveDepthBias = 1;
float g_RemoveContactDistance = REMOVE_SEPARATED_CONTACTS_VERTICAL_DIST;//phManifold::GetManifoldMargin();
float g_AllowedPenetrationOffset = 0;
#endif // USE_NEGATIVE_DEPTH_TUNABLE

void phSimulator::Update (float timeStep, bool finalUpdate)
{
#if USER_JBN
	g_TotalUpdateCount++;
	g_TotalUpdateTime += timeStep;
#endif // USER_JBN

#if USE_PHYSICS_PROFILE_CAPTURE
	if (g_StartPhysicsProfileCapture)
	{
		PhysicsCaptureStart();
		g_StartPhysicsProfileCapture = false;
	}
	if (PhysicsCaptureIsRunning())
		timeStep = 1.0f / 60.0f;	// Lock the delta time.
	PhysicsCaptureFrameStart();
#endif // USE_PHYSICS_PROFILE_CAPTURE

#if PRIM_CACHE_RENDER
	PrimitiveRenderReset();
#endif

	m_UpdateRunning = true;

	COLLISION_SWEEP_DRAW_ONLY(g_NumCollisionSweepRecords = 0;)

#if __PFDRAW
	g_LastPhysicsTimeStep = timeStep;
#endif
#if __STATS
	PF_VALUE_VAR(TotalCCD_Calls).Set(0);
	PF_VALUE_VAR(TotalCCD_Iters).Set(0);
	PF_VALUE_VAR(AvgCCD_Iters).Set(0.0f);
	PF_VALUE_VAR(PeakCCD_Iters).Set(0);
	PF_VALUE_VAR(MinCCD_Iters).Set(0);
#endif

	if (sm_ValidateArchetypes)
	{
		// Check all the data inserted into the level to make sure it's in a valid state
		ValidateArchetypes();
	}

	phArticulatedCollider::ClearActive();

	// The user has asked us to update the simulator but without using any CPUs. Until we get the physics running
	// on GPUs we need to at least use some kind of processing power.
	if (phConfig::GetForceSolverMainThreadAlsoWorks() == false && phConfig::GetForceSolverWorkerThreadCount() == 0)
	{
		phConfig::SetForceSolverMainThreadAlsoWorks(true);
	}

	if (phConfig::GetContactMainThreadAlsoWorks() == false && phConfig::GetContactWorkerThreadCount() == 0)
	{
		phConfig::SetContactMainThreadAlsoWorks(true);
	}

#if EARLY_FORCE_SOLVE
	if (phConfig::GetIntegrateVelocitiesMainThreadAlsoWorks() == false && phConfig::GetIntegrateVelocitiesWorkerThreadCount() == 0)
	{
		phConfig::SetIntegrateVelocitiesMainThreadAlsoWorks(true);
	}

	if (phConfig::GetIntegratePositionsMainThreadAlsoWorks() == false && phConfig::GetIntegratePositionsWorkerThreadCount() == 0)
	{
		phConfig::SetIntegratePositionsMainThreadAlsoWorks(true);
	}

	if (phConfig::GetApplyPushesMainThreadAlsoWorks() == false && phConfig::GetApplyPushesWorkerThreadCount() == 0)
	{
		phConfig::SetApplyPushesMainThreadAlsoWorks(true);
	}
#else // EARLY_FORCE_SOLVE
	if (phConfig::GetColliderPreMainThreadAlsoWorks() == false && phConfig::GetColliderPreWorkerThreadCount() == 0)
	{
		phConfig::SetColliderPreMainThreadAlsoWorks(true);
	}

	if (phConfig::GetColliderPostMainThreadAlsoWorks() == false && phConfig::GetColliderPostWorkerThreadCount() == 0)
	{
		phConfig::SetColliderPostMainThreadAlsoWorks(true);
	}
#endif // EARLY_FORCE_SOLVE

	if (phConfig::GetCollisionMainThreadAlsoWorks() == false && phConfig::GetCollisionWorkerThreadCount() == 0)
	{
		phConfig::SetCollisionMainThreadAlsoWorks(true);
	}

#if __BANK
	UpdateTestForces(timeStep);
#endif

#if 1 && __BANK

	if( sm_BroadPhaseType == phLevelNew::Broadphase_Count )
	{
		sm_BroadPhaseType = m_Level->m_BroadPhaseType;
	}
	else if( sm_BroadPhaseType != m_Level->m_BroadPhaseType )
	{
		m_Level->SetBroadPhaseType( sm_BroadPhaseType );
		m_Level->InitBroadphase( m_Level->m_BroadPhase->m_pairCache, true );
	}

#endif

	PF_AUTO_PUSH_PHYS_SIM_DETAIL("phSimulator::Update");

	PF_START(SimUpdate);
	PPC_STAT_TIMER_START(SimUpdateTimer);

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	// until the end of the function, it is safe to update composite BVHs instantly
	PHLEVEL->SetEnableDeferredCompositeBvhUpdate(false);
#endif

	CoreUpdate(timeStep, finalUpdate);

#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR
	// Synchronize the octree again  Given that we also did this in CollideActive(), the instances in the deferred update list at this point should be
	//   pretty much exactly the physically active objects.  That's not quite true as objects can activate/deactivate and such and the clients might
	//   insert code that could change things too.
	InitiateCommitDeferredOctreeUpdatesTask();
#endif	// PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR && LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

	ComputeMaxStackAlloc();

	PF_SET(TotalObjects,PHLEVEL->GetNumObjects());
	PF_SET(ActiveObjects,PHLEVEL->GetNumActive());
	PF_SET(InactiveObjects,PHLEVEL->GetNumInactive());
	PF_SET(FixedObjects,PHLEVEL->GetNumFixed());
	PF_SET(ActiveArticulated,*phArticulatedCollider::GetActiveCountPtr());
	PF_SET(InstBehaviors,m_NumInstBehaviors);
	PF_SET(MaxLevelIndex,PHLEVEL->GetCurrentMaxLevelIndex());

	PF_SET(Manifolds,PHMANIFOLD->GetAvailCount());
	PF_SET(Contacts,PHCONTACT->GetAvailCount());
	PF_SET(CompositePointers,PHCOMPOSITEPOINTERS->GetAvailCount());

	if (sm_CheckAllPairs)
	{
		CheckAllBroadphaseOverlaps();
	}

#if CHECK_FOR_DUPLICATE_MANIFOLDS
	CheckForDuplicateManifolds(m_Level->GetBroadPhase());
	CheckForDuplicateManifolds(m_OverlappingPairArray);
#endif // CHECK_FOR_DUPLICATE_MANIFOLDS

#if !__NO_OUTPUT
	if (m_ManifoldPool->RanOutOfObjects() ||
		m_ContactPool->RanOutOfObjects() ||
		m_CompositePointerPool->RanOutOfObjects())
	{
		m_ManifoldPool->DumpObjects();
	}

	m_ManifoldPool->AssertIfOutOfObjects();
	m_ContactPool->AssertIfOutOfObjects();
	m_CompositePointerPool->AssertIfOutOfObjects();
#endif

	// Note, should already be set to false by the force solver by now, but just in case...
	m_UpdateRunning = false;

	PF_START_TIMEBAR_PHYS_SIM_DETAIL("Collisions done callback");
	m_AllCollisionsDoneCallback();

#if LEVELNEW_ENABLE_DEFERRED_COMPOSITE_BVH_UPDATE
	// after this point, composite BVH updates need to be deferred
	PHLEVEL->SetEnableDeferredCompositeBvhUpdate(true);
#endif
#if LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE && PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR
	FlushCommitDeferredOctreeUpdatesTask();
#endif	// PHSIM_COMMIT_OCTREE_UPDATE_IN_SIMULATOR && LEVELNEW_ENABLE_DEFERRED_OCTREE_UPDATE

#if USE_GJK_CACHE
	//GJKCacheSystemPostCollisionUpdate(GJKCACHESYSTEM);
#endif

	// Nobody should be reading from this array after the simulator update. Clearing the list lets us remove
	//   entries from it much faster
	phArticulatedCollider::ClearActive();

	PPC_STAT_TIMER_STOP(SimUpdateTimer);
	PF_STOP(SimUpdate);

#if USE_PHYSICS_PROFILE_CAPTURE
	PhysicsCaptureFrameEnd();
#endif // USE_PHYSICS_PROFILE_CAPTURE

#if __STATS
	if(finalUpdate)
	{
		PF_INCREMENTBY(OPAPairs,m_OverlappingPairArray->pairs.GetCount());
	}
#endif // __STATS
}

#if __PFDRAW
class DrawIslands
{
	mutable Vec3V m_IslandMin;
	mutable Vec3V m_IslandMax;

public:
	DrawIslands()
	{
		ResetMinMax();
	}

	void ResetMinMax() const
	{
		m_IslandMin = Vec3V(V_FLT_MAX);
		m_IslandMax = -Vec3V(V_FLT_MAX);
	}

	__forceinline bool Process(int levelIndex, int UNUSED_PARAM(objectIndex)) const
	{
		if (PHLEVEL->LegitLevelIndex(levelIndex) && PHLEVEL->IsInLevel(levelIndex) && PHLEVEL->IsActive(levelIndex))
		{
			phInst* inst = PHLEVEL->GetInstance(levelIndex);

			Vec3V extents, boxCenter;
			inst->GetArchetype()->GetBound()->GetBoundingBoxHalfWidthAndCenter(extents, boxCenter);
			Mat34V_ConstRef current = inst->GetMatrix();
			extents = geomBoxes::ComputeAABBExtentsFromOBB(current.GetMat33ConstRef(), extents);
			boxCenter = Transform(current, boxCenter);

			Vec3V mins, maxs;
			maxs = boxCenter + extents;
			mins = boxCenter - extents;

			m_IslandMax = Max(m_IslandMax, maxs);
			m_IslandMin = Min(m_IslandMin, mins);
		}

		// Return false means, don't skip to the next island after this inst
		return false;
	}

	__forceinline bool NextIsland(int UNUSED_PARAM(numObjects)) const
	{
		grcDrawBox(RCC_VECTOR3(m_IslandMin), RCC_VECTOR3(m_IslandMax), PFD_Islands.GetBaseColor());

		ResetMinMax();

		// Return false means, don't repeat this island again, proceed to the next one
		return false;
	}
};

#if COLLISION_SWEEP_DRAW
EXT_PFD_DECLARE_ITEM(CollisionSweep);
EXT_PFD_DECLARE_ITEM_SLIDER_INT(CollisionSweepIteration);
EXT_PFD_DECLARE_ITEM_SLIDER(CollisionSweepTime);

float g_CollisionTimePhase = 0.0f;
#endif // COLLISION_SWEEP_DRAW

bool g_RenderBroadPhasePairs_ManifoldOnly = 0;
void phSimulator::ProfileDraw() const
{
	//m_Level->ProfileDraw();

	m_Level->ProfileDraw(m_Level->sm_ColorChoiceFunc);
	m_ConstraintMgr->ProfileDraw();
	m_SleepMgr->ProfileDraw();

#if COLLISION_SWEEP_DRAW
	if (g_CollisionTimePhase == 1.0f)
	{
		g_CollisionTimePhase = 0.0f;
	}
	else
	{
		float phaseDelta = TIME.GetSeconds() / Max(PFD_CollisionSweepTime.GetValue(), 0.1f);
		g_CollisionTimePhase += phaseDelta;
		if (g_CollisionTimePhase >= 1.0f)
		{
			g_CollisionTimePhase = 1.0f;
		}
	}

	if (PFD_CollisionSweep.WillDraw())
	{
		for (u32 index = 0; index < g_NumCollisionSweepRecords; ++index)
		{
			phCollisionSweepRecord& record = g_CollisionSweepRecords[index];

			if (record.iteration == PFD_CollisionSweepIteration.GetValue() && record.levelIndex != phInst::INVALID_INDEX)
			{
				if (PHLEVEL->IsLevelIndexGenerationIDCurrent(record.levelIndex, record.generationId))
				{
					phInst* instance = PHLEVEL->GetInstance(record.levelIndex);

					phBound* bound = instance->GetArchetype()->GetBound();
					if (bound && bound->GetType() == phBound::COMPOSITE)
					{
						phBoundComposite* boundComposite = static_cast<phBoundComposite*>(bound);
						bound = boundComposite->GetBound(record.component);
					}

					if (bound)
					{
						Vec3V linVel = SimdTransformUtil::CalculateLinearVelocity(record.lastMatrix.GetCol3().GetIntrin128(), record.currentMatrix.GetCol3().GetIntrin128());

						QuatV quat0, quatDelta;
						SimdTransformUtil::CalculateQuaternionDelta(MAT33V_ARG(record.lastMatrix.GetMat33()), MAT33V_ARG(record.currentMatrix.GetMat33()), quat0.GetIntrin128Ref(), quatDelta.GetIntrin128Ref());

						Mat34V mtx;
						SimdTransformUtil::IntegrateTransform(MAT34V_ARG(record.lastMatrix),linVel.GetIntrin128(),quat0.GetIntrin128(),quatDelta.GetIntrin128(),ScalarV(g_CollisionTimePhase).GetIntrin128(),mtx);

						bound->Draw(mtx, false, false);
					}
				}
			}
		}
	}
#endif // COLLISION_SWEEP_DRAW

	if (PFDGROUP_Manifolds.WillDraw() || PFD_BroadphasePairs.WillDraw())
	{
		btBroadphasePair *prunedPairs = m_Level->GetBroadPhase()->getPrunedPairs();
		const int prunedPairCount = m_Level->GetBroadPhase()->getPrunedPairCount();

		for( int pairIndex = 0; pairIndex < prunedPairCount; ++pairIndex )
		{
			btBroadphasePair *pair = prunedPairs + pairIndex;

			if (!g_RenderBroadPhasePairs_ManifoldOnly || pair->GetManifold())
            if (PFD_BroadphasePairs.Begin())
            {
				phInst* inst1 = m_Level->GetInstanceSafe(pair->GetObject0());
                phInst* inst2 = m_Level->GetInstanceSafe(pair->GetObject1());

                if (inst1 && inst1->GetArchetype() && inst1->GetArchetype()->GetBound() &&
                    inst2 && inst2->GetArchetype() && inst2->GetArchetype()->GetBound())
                {
                    phBound* bound1 = inst1->GetArchetype()->GetBound();

                    Vector3 boxCenter1, boxSize1;
                    bound1->GetBoundingBoxHalfWidthAndCenter(RC_VEC3V(boxSize1), RC_VEC3V(boxCenter1));
					Mat34V_ConstRef mat1 = inst1->GetMatrix();
					boxSize1 = VEC3V_TO_VECTOR3(geomBoxes::ComputeAABBExtentsFromOBB(mat1.GetMat33ConstRef(), RCC_VEC3V(boxSize1)));
					boxCenter1 = VEC3V_TO_VECTOR3(Transform(mat1, RCC_VEC3V(boxCenter1)));

                    Vector3 a1(-boxSize1.x,-boxSize1.y,-boxSize1.z); a1.Add(boxCenter1);
                    Vector3 b1(+boxSize1.x,-boxSize1.y,-boxSize1.z); b1.Add(boxCenter1);
                    Vector3 c1(+boxSize1.x,+boxSize1.y,-boxSize1.z); c1.Add(boxCenter1);
                    Vector3 d1(-boxSize1.x,+boxSize1.y,-boxSize1.z); d1.Add(boxCenter1);
                    Vector3 e1(-boxSize1.x,-boxSize1.y,+boxSize1.z); e1.Add(boxCenter1);
                    Vector3 f1(+boxSize1.x,-boxSize1.y,+boxSize1.z); f1.Add(boxCenter1);
                    Vector3 g1(+boxSize1.x,+boxSize1.y,+boxSize1.z); g1.Add(boxCenter1);
                    Vector3 h1(-boxSize1.x,+boxSize1.y,+boxSize1.z); h1.Add(boxCenter1);

                    phBound* bound2 = inst2->GetArchetype()->GetBound();

                    Vector3 boxCenter2, boxSize2;
                    bound2->GetBoundingBoxHalfWidthAndCenter(RC_VEC3V(boxSize2), RC_VEC3V(boxCenter2));
					Mat34V_ConstRef mat2 = inst2->GetMatrix();
					boxSize2 = VEC3V_TO_VECTOR3(geomBoxes::ComputeAABBExtentsFromOBB(mat2.GetMat33ConstRef(), RCC_VEC3V(boxSize2)));
					boxCenter2 = VEC3V_TO_VECTOR3(Transform(mat2, RCC_VEC3V(boxCenter2)));

                    Vector3 a2(-boxSize2.x,-boxSize2.y,-boxSize2.z); a2.Add(boxCenter2);
                    Vector3 b2(+boxSize2.x,-boxSize2.y,-boxSize2.z); b2.Add(boxCenter2);
                    Vector3 c2(+boxSize2.x,+boxSize2.y,-boxSize2.z); c2.Add(boxCenter2);
                    Vector3 d2(-boxSize2.x,+boxSize2.y,-boxSize2.z); d2.Add(boxCenter2);
                    Vector3 e2(-boxSize2.x,-boxSize2.y,+boxSize2.z); e2.Add(boxCenter2);
                    Vector3 f2(+boxSize2.x,-boxSize2.y,+boxSize2.z); f2.Add(boxCenter2);
                    Vector3 g2(+boxSize2.x,+boxSize2.y,+boxSize2.z); g2.Add(boxCenter2);
                    Vector3 h2(-boxSize2.x,+boxSize2.y,+boxSize2.z); h2.Add(boxCenter2);

                    grcBegin(drawLines,64);
					grcColor(Color_yellow);

                    // Draw box 1
                    grcVertex3f(a1);	grcVertex3f(b1);
                    grcVertex3f(b1);	grcVertex3f(c1);
                    grcVertex3f(c1);	grcVertex3f(d1);
                    grcVertex3f(d1);	grcVertex3f(a1);

                    grcVertex3f(a1);	grcVertex3f(e1);
                    grcVertex3f(b1);	grcVertex3f(f1);
                    grcVertex3f(c1);	grcVertex3f(g1);
                    grcVertex3f(d1);	grcVertex3f(h1);

                    grcVertex3f(e1);	grcVertex3f(f1);
                    grcVertex3f(f1);	grcVertex3f(g1);
                    grcVertex3f(g1);	grcVertex3f(h1);
                    grcVertex3f(h1);	grcVertex3f(e1);

                    // Draw box 2
                    grcVertex3f(a2);	grcVertex3f(b2);
                    grcVertex3f(b2);	grcVertex3f(c2);
                    grcVertex3f(c2);	grcVertex3f(d2);
                    grcVertex3f(d2);	grcVertex3f(a2);

                    grcVertex3f(a2);	grcVertex3f(e2);
                    grcVertex3f(b2);	grcVertex3f(f2);
                    grcVertex3f(c2);	grcVertex3f(g2);
                    grcVertex3f(d2);	grcVertex3f(h2);

                    grcVertex3f(e2);	grcVertex3f(f2);
                    grcVertex3f(f2);	grcVertex3f(g2);
                    grcVertex3f(g2);	grcVertex3f(h2);
                    grcVertex3f(h2);	grcVertex3f(e2);

					grcColor(PFD_BroadphasePairs.GetBaseColor());

                    // Draw lines between
                    grcVertex3f(a1);    grcVertex3f(a2);
                    grcVertex3f(b1);    grcVertex3f(b2);
                    grcVertex3f(c1);    grcVertex3f(c2);
                    grcVertex3f(d1);    grcVertex3f(d2);
                    grcVertex3f(e1);    grcVertex3f(e2);
                    grcVertex3f(f1);    grcVertex3f(f2);
                    grcVertex3f(g1);    grcVertex3f(g2);
                    grcVertex3f(h1);    grcVertex3f(h2);

                    grcEnd();
                }

                PFD_BroadphasePairs.End();
            }

			if (pair->GetManifold())
			{
				pair->GetManifold()->ProfileDraw();
			}
		}
	}

#if ENABLE_PHYSICS_LOCK
	phIterator activeIterator(phIterator::PHITERATORLOCKTYPE_READLOCK);
#else	// ENABLE_PHYSICS_LOCK
	phIterator activeIterator;
#endif	// ENABLE_PHYSICS_LOCK
	activeIterator.SetStateIncludeFlags(phLevelBase::STATE_FLAG_ACTIVE);
	activeIterator.InitCull_All();
//	for (int activeIndex=m_Level->GetFirstActiveIndex(); activeIndex!=phLevelBase::INVALID_STATE_INDEX; activeIndex=m_Level->GetNextActiveIndex())
	int activeIndex = activeIterator.GetFirstLevelIndex(m_Level);
	while(activeIndex != phInst::INVALID_INDEX)
	{
		phCollider* collider = GetActiveCollider(activeIndex);
		if( collider )
			collider->ProfileDraw();

		activeIndex = activeIterator.GetNextLevelIndex(m_Level);
	}

	for(int InstBehaviorIndex = 0; InstBehaviorIndex < m_NumInstBehaviors; ++InstBehaviorIndex)
	{
		m_InstBehaviors[InstBehaviorIndex]->ProfileDraw();
	}

	if (PFD_Islands.Begin())
	{
		bool oldLighting = grcLighting(false);

		m_OverlappingPairArray->IterateIslandInsts(DrawIslands());

		grcLighting(oldLighting);

		PFD_Islands.End();
	}
}

void phSimulator::SimColorChoice(const phInst* inst) const
{
	switch(m_Level->GetState(inst->GetLevelIndex()))
	{
	case phLevelBase::OBJECTSTATE_ACTIVE:
		{
			phCollider* collider = GetActiveCollider(inst->GetLevelIndex());

			phSleep* sleep = ColliderIsPermanentlyActive(collider) ? NULL : collider->GetSleep();
			float sleepPercent = sleep ? Min(1.0f, sleep->GetPercentTotal()) : 0.0f;

			if (collider->IsArticulated())
			{
				grcColor(Lerp(sleepPercent, Color_blue, Color_yellow));
			}
			else if (collider->CanBeArticulated())
			{
				grcColor(Lerp(sleepPercent, Color_purple, Color_yellow));
			}
			else
			{
				grcColor(Lerp(sleepPercent, Color_red, Color_yellow));
			}

			break;
		}
	case phLevelBase::OBJECTSTATE_INACTIVE:
		{
			grcColor(Color_green);
			break;
		}
	default:
		{
			grcColor(Color_grey);
			break;
		}
	}
}
#endif // __PFDRAW

// Air density should be in mks units (kg/m^3).
void phSimulator::ApplyAirResistance(Vec::V3Param128 timeStep, float airDensity, const Vector3& windVelocity)
{
	for (int activeIndex=PHLEVEL->GetFirstActiveIndex(); activeIndex!=phLevelBase::INVALID_STATE_INDEX; activeIndex=PHLEVEL->GetNextActiveIndex())
	{
		GetActiveCollider(activeIndex)->ApplyAirResistance(timeStep,airDensity, windVelocity);
	}
}


void phSimulator::SetGravity (Vec3V_In gravity)
{
	sm_Gravity.Set(VEC3V_TO_VECTOR3(gravity));
}


void phSimulator::SetGravity (const Vector3& gravity)
{
	sm_Gravity.Set(gravity);
}


Vec3V_Out phSimulator::GetGravityV ()
{
	return VECTOR3_TO_VEC3V(sm_Gravity);
}


const Vector3& phSimulator::GetGravity ()
{
	return sm_Gravity;
}


void phSimulator::SetSleepEnabled (bool enabled)
{
	sm_SleepEnabled = enabled;
}

void phSimulator::SetDelaySAPAddRemoveEnabled(bool enabled)
{
	sm_delaySAPAddRemove = enabled;
}


bool phSimulator::GetDelaySAPAddRemoveEnabled()
{
	return sm_delaySAPAddRemove;
}


bool phSimulator::GetSleepEnabled ()
{
	return sm_SleepEnabled;
}

void phSimulator::SetMinimumConcaveThickness(float minThickness)
{
	sm_MinConcaveThickness.Set(minThickness);
}

Vector3 phSimulator::GetMinimumConcaveThickness()
{
	return sm_MinConcaveThickness;
}

void phSimulator::UpdateMinConcaveThickness()
{
	sm_MinConcaveThickness.SplatX(sm_MinConcaveThickness);
}

int phSimulator::GetNumCollisionsPerTask()
{
    return sm_NumCollisionsPerTask;
}

#if (__WIN32 || ENABLE_UNUSED_PHYSICS_CODE)
btStackAlloc* phSimulator::GetPerThreadStackAlloc ()
{
#if __WIN32
	// See if a stack alloc has already been assigned to this thread (g_PerThreadStackAlloc is thread-specific).
	AssertMsg(s_PerThreadStackAllocCreated,"Call phSimulator::InitClass on startup.");
	if (g_PerThreadStackAlloc == NULL)
	{
		// No culler has been assigned to this thread. Lock while one is assigned.
		static sysSpinLockToken token;
		sysSpinLock sl(token);

		// Assign the next available culler to this thread.
		Assertf(s_PerThreadStackAllocCount<phConfig::GetTotalMaxNumWorkerThreads(),"s_PerThreadStackAllocCount is %i but GetTotalMaxNumWorkerThreads is %i",s_PerThreadStackAllocCount,phConfig::GetTotalMaxNumWorkerThreads());
		g_PerThreadStackAlloc = s_PerThreadStackAllocTable[s_PerThreadStackAllocCount++];
		AssertMsg(g_PerThreadStackAlloc,"GetPerThreadStackAlloc failed - call phSimulator::InitClass on startup.");
	}

	return g_PerThreadStackAlloc;
#else
	static btStackAlloc s_StaticStackAlloc(64 * 1024);
	return &s_StaticStackAlloc;
#endif
}
#endif // __WIN32

int s_MaxEPAStack = 0;

void phSimulator::ComputeMaxStackAlloc()
{
#if __WIN32
	s_MaxEPAStack = 0;
	for (int sa = 0; sa < s_PerThreadStackAllocCount; ++sa)
	{
		if (s_PerThreadStackAllocTable[sa])
		{
			s_MaxEPAStack = Max<unsigned int>(s_PerThreadStackAllocTable[sa]->maxusedsize, s_MaxEPAStack);
		}
	}
#endif
}

#if __BANK
bool phSimulator::sm_ApplyTestForce = false;
int phSimulator::sm_TestForceLevelIndex = 0;
int phSimulator::sm_TestForceComponent = 0;
float phSimulator::sm_TestForceMassMultiplier = 1.0f;
bool phSimulator::sm_ApplyTestForceToCenter = true;
bool phSimulator::sm_IsTestForceModelSpace = false;
float phSimulator::sm_fTestForceRange = 0.0f;
Vector3 phSimulator::sm_TestForceScale(0.0f, 0.0f, 0.0f);
Vector3 phSimulator::sm_TestForceLoc(0.0f, 0.0f, 0.0f);
extern int sm_spatialHashLevel;
extern int sm_spatialHashStartGridResolution;
extern bool g_ExtraCCDCollision;
extern bool g_NewCCDRotation;
extern bool g_NewCCDRotationExtraTests;
#if !__PS3
EARLY_FORCE_SOLVE_ONLY(bool g_AlwaysPushCollisions = false;)
#endif // !__PS3

static const char* s_PenetrationSolverNames[] =
{
    "Minkowski",
    "GJK EPA",
    "Triangle"
};

static const char* s_BroadphaseNames[] =
{
	"3Axis SAP",
	"1Axis SAP",
	"Loose Octree",
	"Spatial Hashing",
	"NxN",
	"---------------"
};

int g_NumContactsPerFrame;
extern int g_NumDeepPenetrationChecks;

void phSimulator::ReloadShapeTestELF()
{
	RELOAD_TASK_BINARY(shapetestspu);
	RELOAD_TASK_BINARY(shapetestcapsulespu);
	RELOAD_TASK_BINARY(shapetestspherespu);
	RELOAD_TASK_BINARY(shapetestsweptspherespu);
	RELOAD_TASK_BINARY(shapetesttaperedsweptspherespu);
}

void DumpOverlappingPairs()
{
	s_DumpOverlappingPairs = true;
}

#if TRACK_COLLISION_TYPE_PAIRS
void PrintCollisionTypePairTable();
#endif // TRACK_COLLISION_TYPE_PAIRS

#if STIFFNESS_MODULUS_ENABLED
PARAM(stiffnessmodulus, "[physics] Modulating factor on stiffness");
#endif

void MaxThreads()
{
	phConfig::SetWorkerThreadCount(phConfig::GetMaxNumWorkerThreads());
	phConfig::SetMainThreadAlsoWorks(false);
}

void MinThreads()
{
	phConfig::SetWorkerThreadCount(0);
	phConfig::SetMainThreadAlsoWorks(true);
}

#if MATCH_FLAGS_FOR_PAIRS
bool g_MatchFlagsForPairs = true;
#endif // MATCH_FLAGS_FOR_PAIRS

void phSimulator::AddWidgets (bkBank& bank)
{
#if STIFFNESS_MODULUS_ENABLED
	PARAM_stiffnessmodulus.Get(phJoint::sm_StiffnessModulus);
	bank.AddSlider("Stiffness modulus", &phJoint::sm_StiffnessModulus, 0.0f, 0.999f, 0.01f);
#endif

	bank.AddSlider("Gravity",&sm_Gravity,-100.0f,100.0f,0.1f);
	bank.AddToggle("Effector driving enabled", &phArticulatedBody::sm_EffectorDrivingEnabled);
	bank.AddToggle("Enable Sleep",&sm_SleepEnabled);
	bank.AddToggle("Enable Pushes", &sm_EnablePushes);
	bank.AddToggle("Always Push", &sm_AlwaysPush);
#if MATCH_FLAGS_FOR_PAIRS
	bank.AddToggle("Match Flags For Pairs", &g_MatchFlagsForPairs);
#endif // MATCH_FLAGS_FOR_PAIRS
#if EARLY_FORCE_SOLVE
	bank.AddSlider("Max Push Collision Iterations", &g_MaxPushCollisionIterations, 1, 100, 1);
	bank.AddSlider("Max Push Pairs", &s_MaxPushPairs, 0, 10000, 1);
#if !__PS3
	bank.AddToggle("Always use push collisions", &g_AlwaysPushCollisions);
#endif // !__PS3
	bank.AddSlider("Safe Push Limit", &g_PushCollisionTolerance, 0.0f, 1.0f, 0.001f);
	bank.AddSlider("Safe Turn Limit", &g_TurnCollisionTolerance, 0.0f, 1.0f, 0.001f);
	bank.AddToggle("Push Collision TTY", &g_PushCollisionTTY);
#endif // EARLY_FORCE_SOLVE
	bank.AddSlider("Allowed Penetration", &sm_AllowedPenetration, 0.0f, 1.0f, 0.001f);
	bank.AddSlider("Allowed Angle Penetration", &s_AllowedAnglePenetration, 0.0f, 1.0f, 0.001f);
	bank.AddSlider("Last Safe Matrix Max Penetration Frames",&s_MaxPenetratingFrames, 0, 255, 1);
	bank.AddToggle("Validate Archetypes",&sm_ValidateArchetypes);
    bank.AddToggle("Collider Update",&sm_ColliderUpdateEnabled);
	bank.AddToggle("Compute Forces",&sm_ComputeForces);
	bank.AddToggle("Maintain Loose Octree", &g_MaintainLooseOctree);
#if PROPHYLACTIC_SWAPS
	bank.AddToggle("Prophylactic Swaps", &g_ProphylacticSwaps);
#endif // PROPHYLACTIC_SWAPS
	bank.AddToggle("Use Deferred Octree Update Task", &sm_UseOctreeUpdateTask);

#if PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY
	bank.AddToggle("Inst Beh Array", &sm_GetInstBehaviorFromArray);
#endif // PHSIM_OPT_INSTBEHAVIOR_LEVELINDEX_ARRAY

	bank.AddButton("Dump Overlapping Pairs", datCallback(DumpOverlappingPairs));
	bank.AddButton("Dump Constraints", datCallback(CFA(phConstraintMgr::DumpConstraints)));

#if TRACK_COLLISION_TYPE_PAIRS
	bank.AddButton("Dump Collision Stats", datCallback(CFA(PrintCollisionTypePairTable)));
#endif // TRACK_COLLISION_TYPE_PAIRS

	bank.PushGroup("Ragdoll tuning", false);
	bank.AddSlider("Ragdoll Max Linear Speed",&phArticulatedBody::sm_RagdollMaxLinearSpeed,0.0f,500.0f,1.0f);
	bank.AddSlider("Ragdoll Max Angular Speed",&phArticulatedBody::sm_RagdollMaxAngularSpeed,0.0f,100.0f,0.1f);
	bank.AddSlider("Ragdoll Damping Lin C",&phArticulatedBody::sm_RagdollDampingLinC,0.0f,0.5f,0.001f);
	bank.AddSlider("Ragdoll Damping Lin V",&phArticulatedBody::sm_RagdollDampingLinV,0.0f,0.5f,0.001f);
	bank.AddSlider("Ragdoll Damping Lin V squared",&phArticulatedBody::sm_RagdollDampingLinV2,0.0f,0.5f,0.001f);
	bank.AddSlider("Ragdoll Damping Ang C",&phArticulatedBody::sm_RagdollDampingAngC,0.0f,0.5f,0.001f);
	bank.AddSlider("Ragdoll Damping Ang V",&phArticulatedBody::sm_RagdollDampingAngV,0.0f,0.5f,0.001f);
	bank.AddSlider("Ragdoll Damping Ang V squared",&phArticulatedBody::sm_RagdollDampingAngV2,0.0f,0.5f,0.001f);
	bank.AddSlider("Emergency min stiffness", &phArticulatedBody::sm_EmergencyMinStiffness, 0.0f, 0.999f, 0.01f);
	bank.PopGroup();

	{ // Multithreading
		Assertf(phConfig::GetMaxNumWorkerThreads() > -1, "You must call phSimulator::InitClass before phSimulator::AddWidgets");
		bank.PushGroup("Multithreading", false);
		bank.AddButton("Max threads", datCallback(CFA(MaxThreads)));
		bank.AddButton("Min threads", datCallback(CFA(MinThreads)));
		bank.PushGroup("Force Solver", false);
		bank.AddSlider("Worker threads", &phConfig::GetForceSolverWorkerThreadCountRef(), 0, phConfig::GetMaxNumWorkerThreads(), 1 );
		bank.AddToggle("Main thread helps simulator", &phConfig::GetForceSolverMainThreadAlsoWorksRef() );
		bank.AddSlider("Max phases", &phConfig::GetForceSolverMaxPhasesRef(), 0, 32768, 1);
		bank.AddSlider("OoO Tolerance", &phConfig::GetForceSolverOutOfOrderToleranceRef(), 0, 32, 1);
		bank.PopGroup();
		bank.PushGroup("Contact", false);
		bank.AddSlider("Worker threads", &phConfig::GetContactWorkerThreadCountRef(), 0, phConfig::GetMaxNumWorkerThreads(), 1 );
		bank.AddToggle("Main thread helps simulator", &phConfig::GetContactMainThreadAlsoWorksRef() );
		bank.PopGroup();
#if EARLY_FORCE_SOLVE
		bank.PushGroup("IntegrateVelocities", false);
		bank.AddSlider("Worker threads", &phConfig::GetIntegrateVelocitiesWorkerThreadCountRef(), 0, phConfig::GetMaxNumWorkerThreads(), 1 );
		bank.AddToggle("Main thread helps simulator", &phConfig::GetIntegrateVelocitiesMainThreadAlsoWorksRef() );
		bank.PopGroup();
		bank.PushGroup("IntegratePositions", false);
		bank.AddSlider("Worker threads", &phConfig::GetIntegratePositionsWorkerThreadCountRef(), 0, phConfig::GetMaxNumWorkerThreads(), 1 );
		bank.AddToggle("Main thread helps simulator", &phConfig::GetIntegratePositionsMainThreadAlsoWorksRef() );
		bank.PopGroup();
		bank.PushGroup("ApplyPushes", false);
		bank.AddSlider("Worker threads", &phConfig::GetApplyPushesWorkerThreadCountRef(), 0, phConfig::GetMaxNumWorkerThreads(), 1 );
		bank.AddToggle("Main thread helps simulator", &phConfig::GetApplyPushesMainThreadAlsoWorksRef() );
		bank.PopGroup();
#else // EARLY_FORCE_SOLVE
		bank.PushGroup("ColliderPre", false);
		bank.AddSlider("Worker threads", &phConfig::GetColliderPreWorkerThreadCountRef(), 0, phConfig::GetMaxNumWorkerThreads(), 1 );
		bank.AddToggle("Main thread helps simulator", &phConfig::GetColliderPreMainThreadAlsoWorksRef() );
		bank.PopGroup();
		bank.PushGroup("ColliderPost", false);
		bank.AddSlider("Worker threads", &phConfig::GetColliderPostWorkerThreadCountRef(), 0, phConfig::GetMaxNumWorkerThreads(), 1 );
		bank.AddToggle("Main thread helps simulator", &phConfig::GetColliderPostMainThreadAlsoWorksRef() );
		bank.PopGroup();
#endif // EARLY_FORCE_SOLVE
		bank.PushGroup("Collision", false);
		bank.AddSlider("Worker threads", &phConfig::GetCollisionWorkerThreadCountRef(), 0, phConfig::GetMaxNumWorkerThreads(), 1 );
		bank.AddToggle("Main thread helps simulator", &phConfig::GetCollisionMainThreadAlsoWorksRef() );
		bank.PopGroup();
		bank.AddSlider("Collisions Per Task", &sm_NumCollisionsPerTask, 1, MAX_NUM_COLLISIONS_PER_TASK, 1);

#if __PPU
		bank.AddButton("Reload Shape Test ELF", datCallback(CFA(phSimulator::ReloadShapeTestELF)));
		bank.AddToggle("Bullet Spin Wait", &g_BulletSpinWait);
#endif

		bank.PopGroup();
	}

	{ // Callbacks
		bank.PushGroup("Callbacks", false);
		bank.AddToggle("ShouldFindImpacts",&sm_ShouldFindImpacts);
		bank.AddToggle("PreComputeImpacts",&sm_PreComputeImpacts);
		bank.AddToggle("Report Moved By Sim", &sm_ReportMovedBySim);
		bank.PopGroup();
	}

	{ // Broad phase
		bank.PushGroup("Broad phase", false);
		bank.AddCombo("Broadphase", (int*)(&sm_BroadPhaseType), int(phLevelNew::Broadphase_Count), s_BroadphaseNames);
		bank.AddSlider("Spatial Hash Levels ( requires switch to different broadphase, then back )", &sm_spatialHashLevel, 0, INT_MAX, 1);
		bank.AddSlider("Spatial Hash Initial Resolution ( requires switch to different broadphase, then back )", &sm_spatialHashStartGridResolution, 0, INT_MAX, 1);
#if !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
		bank.AddToggle("Use secondary broadphase for batch add", &(phLevelNew::sm_useSecondaryBroadphaseForBatchAdd));
		bank.AddSlider("Threshold for secondary batch add", &(phLevelNew::sm_batchAddThresholdForSecondaryBroadphase), 0, INT_MAX, 1);
#endif // !PHLEVELNEW_DISABLE_SECONDARY_BROADPHASE
		bank.AddToggle("Delay add/remove from broadphase for batching", &sm_delaySAPAddRemove);
		bank.AddToggle("Check All Pairs", &sm_CheckAllPairs);
		bank.AddToggle("Manifold box test", &sm_ManifoldBoxTest);
		bank.AddToggle("Sort by cost", &sm_SortPairsByCost);
#if USE_STATIC_BVH_REJECTION_SWAP
		bank.AddToggle("Use Static Bvh Rejection",&g_UseStaticBvhRejection);
#endif // USE_STATIC_BVH_REJECTION_SWAP
		bank.PopGroup();
	}

#if USE_NEW_MID_PHASE && ALLOW_MID_PHASE_SWAP
	{ // Mid Phase
		bank.PushGroup("Mid phase", true);
		bank.AddToggle("Use New Mid Phase Collision",&g_UseNewMidPhaseCollision);
		bank.PopGroup();
	}
#endif // USE_NEW_MID_PHASE && ALLOW_MID_PHASE_SWAP

#if USE_PHYSICS_PROFILE_CAPTURE
	{ // Profile Capture
		bank.PushGroup("Profile Capture", true);
		bank.AddToggle("Start Physics Profile Capture",&g_StartPhysicsProfileCapture);
		bank.PopGroup();
	}
#endif // USE_PHYSICS_PROFILE_CAPTURE

#if USE_NEGATIVE_DEPTH_TUNABLE
	{ // Negative Depth
		bank.PushGroup("Negative Depth", true);
		bank.AddToggle("Use Negative Depth",&g_UseNegativeDepth);
		bank.AddToggle("Use Negative Depth Force Cancel",&g_UseNegativeDepthForceCancel);
		bank.AddToggle("Use Negative Depth Force Cancel1",&g_UseNegativeDepthForceCancel1);
		bank.AddToggle("Use Remove Contact",&g_UseRemoveContact);
		bank.AddSlider("Negative Depth Bias0", &g_NegativeDepthBias0, 0.0f, 1.0f, 0.01f);
		bank.AddSlider("Negative Depth Bias1", &g_NegativeDepthBias1, 0.0f, 1.0f, 0.01f);
		bank.AddSlider("Negative Depth Penetration Bias", &g_NegativeDepthPenetrationBias, 0.0f, 1.0f, 0.01f);
		bank.AddSlider("Positive Depth Bias", &g_PositiveDepthBias, 0.0f, 1.0f, 0.01f);
		bank.AddSlider("Remove Contact Distance", &g_RemoveContactDistance, phManifold::GetManifoldMargin(), .2f, 0.01f);
		bank.AddSlider("Allowed Penetration Offset", &g_AllowedPenetrationOffset, -.2f, +.2f, .001f);
		bank.PopGroup();
	}
#endif // USE_NEGATIVE_DEPTH_TUNABLE

	{ // Narrow phase
		bank.PushGroup("Narrow phase", false);

		{ // Narrow phase first test
			bank.PushGroup("Narrow phase first test", false);
			bank.AddToggle("Composite Part Sphere Test", &sm_CompositePartSphereTest);
			bank.AddToggle("Composite Part OBB Test", &sm_CompositePartOBBTest);
			bank.AddToggle("Composite Part AABB Test", &sm_CompositePartAABBTest);
			bank.PopGroup();
		}

		{ // CCD
			bank.PushGroup("CCD", false);
			bank.AddToggle("Extra CCD Collision", &g_ExtraCCDCollision);
			bank.AddSlider("Max CA Iterations", &g_MaxContinuousIterations, 0, 1000, 1);
			bank.AddToggle("Sweep From Safe Spot", &g_SweepFromSafe);
			bank.PopGroup();
		}

		bank.AddSlider("Min point lifetime", &sm_MinManifoldPointLifetime, 0, 10, 1);
#if 0
		phManifold::AddWidgets(bank);
#endif
#if __ASSERT
		bank.AddToggle("Backface Culling", &g_UseBackfaceCulling);
		bank.AddToggle("Normal Filtering", &g_UseNormalFiltering);
		bank.AddToggle("Normal Filter Regardless of Neighbors", &g_UseNormalFilteringAlways);
		bank.AddToggle("Normal Clamping", &g_UseNormalClamping);
#endif // __ASSERT
		bank.AddCombo("Concave Penetration", &sm_ConcavePenetration, Penetration_Count, s_PenetrationSolverNames);
		bank.AddCombo("Convex Penetration", &sm_ConvexPenetration, Penetration_Count - 1, s_PenetrationSolverNames);
		bank.AddSlider("Min Concave Thickness", &sm_MinConcaveThickness.x, 0.0f, 10.0f, 0.001f, datCallback(CFA(phSimulator::UpdateMinConcaveThickness)));
		bank.AddToggle("Self Collisions", &sm_SelfCollisionsEnabled);
#if OCTANT_MAP_SUPPORT_ACCEL
		bank.AddToggle("Use Octant Map", &phBoundPolyhedron::GetUseOctantMapRef());
#endif
#if USE_BOX_BOX_DISTANCE
		bank.AddToggle("Use Box/Box Distance", &g_UseBoxBoxDistance);
#endif
#if FAST_BOXES
		bank.AddToggle("Fast Boxes", &g_FastBoxes);
#endif // FAST_BOXES
#if CAPSULE_TO_CAPSULE_DISTANCE
		bank.AddToggle("Use Capsule/Capsule", &g_CapsuleCapsuleDistance);
#endif
#if CAPSULE_TO_TRIANGLE_DISTANCE
		bank.AddToggle("Use Capsule/Triangle", &g_CapsuleTriangleDistance);
#endif
#if DISC_TO_TRIANGLE_DISTANCE
		bank.AddToggle("Use Disc/Triangle", &g_DiscTriangleDistance);
#endif
#if BOX_TO_TRIANGLE_DISTANCE
		bank.AddToggle("Use Box/Triangle", &g_BoxTriangleDistance);
#endif
#if FAST_CAPSULES
		bank.AddToggle("Fast Capsules", &g_FastCapsules);
#endif // FAST_CAPSULES
#if USE_GJK_CACHE
		bank.AddToggle("Use Frame Persistent GJK Cache",&g_UseFramePersistentGJKCache);
		bank.AddToggle("Use GJK Cache",&g_UseGJKCache);
#endif
		bank.AddToggle("Use bbox for poly finding", &g_UseBBoxForPolyFinding);
		bank.PopGroup();
	}

	{ // Debug bools
		bank.PushGroup("Debug Bools", false);
		bank.AddToggle("Debug Bool 1", &g_debugBool1);
		bank.AddToggle("Debug Bool 2", &g_debugBool2);
		bank.AddToggle("Debug Bool 3", &g_debugBool3);
		bank.AddToggle("Debug Bool 4", &g_debugBool4);
		bank.AddToggle("Debug Bool 5", &g_debugBool5);
		bank.AddToggle("Debug Bool 6", &g_debugBool6);
		bank.PopGroup(); //"Debug Bools"
	}

	{ // Collision stats
		bank.PushGroup("Collision stats", false);
		bank.AddSlider("Num Contacts", &g_NumContactsPerFrame, 0, INT_MAX, 0);
		bank.AddSlider("Num Deep Penetrations", &g_NumDeepPenetrationChecks, 0, INT_MAX, 0);
		bank.AddSlider("Auto CCD Collisions", &g_NumAutoCCDCollisions, 0, INT_MAX, 0);
		bank.AddSlider("Max EPA Stack", &s_MaxEPAStack, 0, INT_MAX, 0);
		bank.PopGroup(); //"Collision stats"
	}

	{ // Debug force testing
		bank.PushGroup("Debug Force Testing", false, "Provides controls for adding forces to arbitrary objects");
		bank.AddToggle("Apply Test Force", (bool*) &sm_ApplyTestForce, NullCB, "Enable this to apply the force defined below to the object specified by it's level index.");
		bank.AddSlider("Level Index", &sm_TestForceLevelIndex, 0, 10000, 1, NullCB, "The index of the object to apply the force upon.");
		bank.AddSlider("Component Index", &sm_TestForceComponent, 0, 1000, 1, NullCB, "The component within the specified object to apply the force upon. Note: Requires 'Apply to center of mass' to be disabled.");
		bank.AddSlider("Mass force multiplier", &sm_TestForceMassMultiplier, -5.0f, 5.0f, 0.1f, NullCB, "Scale the force by the mass of the object.");
		bank.AddToggle("Apply to center of mass", &sm_ApplyTestForceToCenter, NullCB, "When unchecked, apply to the specified location relative to the matrix & possibly transform to local space.");
		bank.AddToggle("Apply in local space", &sm_IsTestForceModelSpace, NullCB, "Requires 'Apply to center of mass' to be unchecked to be useful.");
		bank.AddSlider("Force magnitude range", &sm_fTestForceRange, -10000.0f, 10000.0f, 1, NullCB, "The maximum magnitude of the force attenuated by scale vector and mass force multiplier.");
		bank.AddSlider("Force Scale", &sm_TestForceScale, -1.0f, 1.0f, 0.01f, NullCB, "Multiplier for the force magnitude on each axis.");
		bank.AddSlider("Force Location", &sm_TestForceLoc, -100.0f, 100.0f, 1.0f, NullCB, "Offset from the center of mass that the force is applied. Requires 'Apply to center of mass' to be unchecked.");
		bank.PopGroup();
	}

	phContactMgr::AddWidgets( bank );
	bank.PushGroup("Manifold Pool");
	phPool<phManifold>::AddWidgets( bank );
	bank.PopGroup();
	bank.PushGroup("Composite Pointers Pool");
	phPool<phCompositePointers>::AddWidgets( bank );
	bank.PopGroup();
	bank.PushGroup("Contact Pool");
	phPool<phContact>::AddWidgets( bank );
	bank.PopGroup();
	phCollider::AddWidgets( bank );
	phInst::AddWidgets( bank );
}

void phSimulator::UpdateTestForces(float timeStep)
{
	if(!sm_ApplyTestForce)
	{
		return;
	}

	if(!m_Level->LegitLevelIndex(sm_TestForceLevelIndex))
	{
		return;
	}

	phCollider* collider;
	if(m_Level->IsInactive(sm_TestForceLevelIndex))
	{
		collider = ActivateObject(sm_TestForceLevelIndex);
	}
	else
	{
		collider = GetActiveCollider(sm_TestForceLevelIndex);
	}
	if(!collider)
	{
		return;
	}
	const Matrix34& m = RCC_MATRIX34(collider->GetMatrix());

	Vector3 force;
	Vector3 loc;
	static const Vector3 unitForce(1.0f,1.0f,1.0f);
	if(sm_IsTestForceModelSpace)
	{
		m.Transform3x3(unitForce, force);
		m.Transform(sm_TestForceLoc, loc);
	}
	else
	{
		force = unitForce;
		loc = m.d + sm_TestForceLoc;
	}

	force.Multiply(sm_TestForceScale);
	force.Scale(sm_fTestForceRange);

	force.Scale(timeStep);
	force.Scale(sm_TestForceMassMultiplier * collider->GetMass());

	if(sm_ApplyTestForceToCenter)
	{
		ApplyForce(ScalarV(timeStep).GetIntrin128ConstRef(), sm_TestForceLevelIndex, force);
	}
	else
	{
		ApplyForce(ScalarV(timeStep).GetIntrin128ConstRef(), sm_TestForceLevelIndex, force, loc, sm_TestForceComponent);
	}
}

#endif

#if __BANK
void phSimulator::DumpActivatingPairs()
{
	Displayf("phSimulator::DumpActivatingPairs (%d)", m_NumActivatingPairs);

	for( int i = 0; i < m_NumActivatingPairs; i++ )
	{
		phTaskCollisionPair *pair = m_ActivatingPairs[ i ];

		const char* name0 = "(invalid)";
		if (pair->levelIndex1)
		{
			if (phInst* inst = PHLEVEL->GetInstance(pair->levelIndex1))
			{
				if (phArchetype* arch = inst->GetArchetype())
				{
					name0 = arch->GetFilename();
				}
			}
		}

		const char* name1 = "(invalid)";
		if (pair->levelIndex2)
		{
			if (phInst* inst = PHLEVEL->GetInstance(pair->levelIndex2))
			{
				if (phArchetype* arch = inst->GetArchetype())
				{
					name1 = arch->GetFilename();
				}
			}
		}

		if (phManifold* manifold = pair->manifold)
		{
			Displayf("%d: %s:%d, %s:%d <%.3f>", i, name0, pair->levelIndex1, name1, pair->levelIndex2, float(manifold->GetCollisionTime()) * sysTimerConsts::TicksToMilliseconds);
		}
		else
		{
			Displayf("%d: %s:%d, %s:%d (no manifold)", i, name0, pair->levelIndex1, name1, pair->levelIndex2);
		}
	}
}
#endif // #if __BANK


}	// namespace rage
