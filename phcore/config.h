//
// phcore/config.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_CONFIG_H
#define PHCORE_CONFIG_H

#include "constants.h" // for EARLY_FORCE_SOLVE

namespace rage {

#if __SPU

#define VALIDATE_PHYSICS(x)	{x}
#define VALIDATE_PHYSICS_ASSERT(x)
#define VALIDATE_PHYSICS_ASSERT_MSG(x,y)
#define VALIDATE_PHYSICS_ASSERTF(x,y,...)
#define VALIDATE_PHYSICS_FAST_ASSERT(x)

#else // __SPU

//=============================================================================
//
// PURPOSE
//   Contains configuration settings that are shared in the 
//   physics library.  These should be settings that are 
//   modifiable on a per-project basis.
// <FLAG Component>
//
class phConfig
{
public:

	// PURPOSE: Determine how many contexts/etc spaces to set up to allow simultaneous tasks on multiple SPUs/threads/
	// NOTE: This must be called before phSimulator::InitClass. If you don't call it before initializing the simulator,
	//       the system will try to determine an optimal setting.
	static void SetMaxNumWorkerThreads(int numTasks, bool mainThreadWorks);
	static int GetMaxNumWorkerThreads();
	static int GetTotalMaxNumWorkerThreads();

	// PURPOSE: Returns true if reference counting is turned on.
	static bool IsRefCountingEnabled();

	// PURPOSE: Turns on or off reference counting.
	// NOTES: This function will assert if the reference counting state is frozen.
	static void EnableRefCounting(bool enabled=true);

	// PURPOSE: Prevents reference counting from being turned on or off.
	static void FreezeRefCounting ();

	// PURPOSE: Tell the simulator how many simultaneous tasks to use for all stages.
	// NOTES: If this is higher than the simulator actually can get from the scheduler, 
	//        it can hang in the solver during Sync.
	//        These funcs simply call the more specific versions below.
	static void SetWorkerThreadCount(int count);
	static void SetMainThreadAlsoWorks(bool enable);
	
	// ForceSolver
	// PURPOSE: Tell the simulator what scheduler to use for ForceSolver Update
	static void SetForceSolverTaskScheduler(int index);
	static int GetForceSolverTaskScheduler();

	static void SetForceSolverWorkerThreadCount(int count);
	static int GetForceSolverWorkerThreadCount();
	static int& GetForceSolverWorkerThreadCountRef();
	static int GetForceSolverWorkerThreadTotalCount();

	static void SetForceSolverMainThreadAlsoWorks(bool enable);
	static bool GetForceSolverMainThreadAlsoWorks();
	static bool& GetForceSolverMainThreadAlsoWorksRef();

	static void SetForceSolverMaxPhases(int maxPhases);
	static int GetForceSolverMaxPhases();
	static int& GetForceSolverMaxPhasesRef();

	static void SetForceSolverOutOfOrderTolerance(u32 tolerance);
	static u32 GetForceSolverOutOfOrderTolerance();
	static u32& GetForceSolverOutOfOrderToleranceRef();

	// Contact update 
	// PURPOSE: Tell the simulator what scheduler to use for Contact Update
	static void SetContactTaskScheduler(int index);
	static int GetContactTaskScheduler();

	static void SetContactWorkerThreadCount(int count);
	static int GetContactWorkerThreadCount();
	static int& GetContactWorkerThreadCountRef();
	static int GetContactWorkerThreadTotalCount();

	static void SetContactMainThreadAlsoWorks(bool enable);
	static bool GetContactMainThreadAlsoWorks();
	static bool& GetContactMainThreadAlsoWorksRef();

	// PURPOSE: Tell the simulator what scheduler to use for collider update
	static void SetColliderTaskScheduler(int index);
	static int GetColliderTaskScheduler();

#if EARLY_FORCE_SOLVE
	// Collider PreCollide Update
	static void SetIntegrateVelocitiesWorkerThreadCount(int count);
	static int GetIntegrateVelocitiesWorkerThreadCount();
	static int& GetIntegrateVelocitiesWorkerThreadCountRef();
	static int GetIntegrateVelocitiesWorkerThreadTotalCount();

	static void SetIntegrateVelocitiesMainThreadAlsoWorks(bool enable);
	static bool GetIntegrateVelocitiesMainThreadAlsoWorks();
	static bool& GetIntegrateVelocitiesMainThreadAlsoWorksRef();

	// Collider IntegrateVelocities Update
	static void SetIntegratePositionsWorkerThreadCount(int count);
	static int GetIntegratePositionsWorkerThreadCount();
	static int& GetIntegratePositionsWorkerThreadCountRef();
	static int GetIntegratePositionsWorkerThreadTotalCount();

	static void SetIntegratePositionsMainThreadAlsoWorks(bool enable);
	static bool GetIntegratePositionsMainThreadAlsoWorks();
	static bool& GetIntegratePositionsMainThreadAlsoWorksRef();

	// Collider PreCollide Update
	// PURPOSE: Tell the simulator what scheduler to use for PreCollide Update collider
	static void SetApplyPushesWorkerThreadCount(int count);
	static int GetApplyPushesWorkerThreadCount();
	static int& GetApplyPushesWorkerThreadCountRef();
	static int GetApplyPushesWorkerThreadTotalCount();

	static void SetApplyPushesMainThreadAlsoWorks(bool enable);
	static bool GetApplyPushesMainThreadAlsoWorks();
	static bool& GetApplyPushesMainThreadAlsoWorksRef();
#else // EARLY_FORCE_SOLVE
	// Collider PreCollide Update
	static void SetColliderPreWorkerThreadCount(int count);
	static int GetColliderPreWorkerThreadCount();
	static int& GetColliderPreWorkerThreadCountRef();
	static int GetColliderPreWorkerThreadTotalCount();

	static void SetColliderPreMainThreadAlsoWorks(bool enable);
	static bool GetColliderPreMainThreadAlsoWorks();
	static bool& GetColliderPreMainThreadAlsoWorksRef();

	// Collider PostCollde Update
	static void SetColliderPostWorkerThreadCount(int count);
	static int GetColliderPostWorkerThreadCount();
	static int& GetColliderPostWorkerThreadCountRef();
	static int GetColliderPostWorkerThreadTotalCount();

	static void SetColliderPostMainThreadAlsoWorks(bool enable);
	static bool GetColliderPostMainThreadAlsoWorks();
	static bool& GetColliderPostMainThreadAlsoWorksRef();
#endif // EARLY_FORCE_SOLVE

	// Collision Detection
	// PURPOSE: Tell the simulator what scheduler to use for Collision detection
	static void SetCollisionTaskScheduler(int index);
	static int GetCollisionTaskScheduler();

	static void SetCollisionWorkerThreadCount(int count);
	static int GetCollisionWorkerThreadCount();
	static int& GetCollisionWorkerThreadCountRef();
	static int GetCollisionWorkerThreadTotalCount();

	static void SetCollisionMainThreadAlsoWorks(bool enable);
	static bool GetCollisionMainThreadAlsoWorks();
	static bool& GetCollisionMainThreadAlsoWorksRef();

#if __DEV
	static bool sm_ValidatePhysics;
#endif

private:
	static bool sm_RefCountingFrozen;
	static bool sm_RefCounting;

	static int sm_MaxNumWorkerThreads;
	static bool sm_MainThreadWorks;

	static int sm_ForceSolverTaskScheduler;
	static int sm_ForceSolverWorkerThreadCount;
	static bool sm_ForceSolverMainThreadAlsoWorks;

	static int sm_ForceSolverMaxPhases;
	static u32 sm_ForceSolverOutOfOrderTolerance;

	static int sm_ContactTaskScheduler;
	static int sm_ContactWorkerThreadCount;
	static bool sm_ContactMainThreadAlsoWorks;

	static int sm_ColliderTaskScheduler;

#if EARLY_FORCE_SOLVE
	static int sm_IntegratePositionsWorkerThreadCount;
	static bool sm_IntegratePositionsMainThreadAlsoWorks;

	static int sm_IntegrateVelocitiesWorkerThreadCount;
	static bool sm_IntegrateVelocitiesMainThreadAlsoWorks;

	static int sm_ApplyPushesWorkerThreadCount;
	static bool sm_ApplyPushesMainThreadAlsoWorks;
#else // EARLY_FORCE_SOLVE
	static int sm_ColliderPreWorkerThreadCount;
	static bool sm_ColliderPreMainThreadAlsoWorks;

	static int sm_ColliderFirstWorkerThreadCount;
	static bool sm_ColliderFirstMainThreadAlsoWorks;

	static int sm_ColliderPostWorkerThreadCount;
	static bool sm_ColliderPostMainThreadAlsoWorks;
#endif // EARLY_FORCE_SOLVE

	static int sm_CollisionTaskScheduler;
	static int sm_CollisionWorkerThreadCount;
	static bool sm_CollisionMainThreadAlsoWorks;
};

#if __ASSERT && !RSG_ORBIS
#define VALIDATE_PHYSICS(x)	if(phConfig::sm_ValidatePhysics) {x}
#define VALIDATE_PHYSICS_ASSERT(x) Verifyf(!phConfig::sm_ValidatePhysics || x, "sm_ValidatePhysics is on")
#define VALIDATE_PHYSICS_ASSERT_MSG(x,y) Verifyf(!phConfig::sm_ValidatePhysics || x, "%s sm_ValidatePhysics is on", y)
#define VALIDATE_PHYSICS_ASSERTF(x,fmt,...) Verifyf(!phConfig::sm_ValidatePhysics || x, fmt, ##__VA_ARGS__)
#define VALIDATE_PHYSICS_FAST_ASSERT(x) AssertMsg(!phConfig::sm_ValidatePhysics || (x),"sm_ValidatePhysics is on.")
#else
#define VALIDATE_PHYSICS(x)
#define VALIDATE_PHYSICS_ASSERT(x)
#define VALIDATE_PHYSICS_ASSERT_MSG(x,y)
#define VALIDATE_PHYSICS_ASSERTF(x,fmt,...)
#define VALIDATE_PHYSICS_FAST_ASSERT(x)
#endif

//=============================================================================
// Implementations


inline void phConfig::SetMaxNumWorkerThreads(int numTasks, bool mainThreadWorks)
{
	Assert(sm_MaxNumWorkerThreads == -1);
	sm_MaxNumWorkerThreads = numTasks;

	if (numTasks > 0)
	{
		sm_MainThreadWorks = mainThreadWorks;
	}
	else
	{
		sm_MainThreadWorks = true;
	}
}

inline int phConfig::GetMaxNumWorkerThreads()
{
	return sm_MaxNumWorkerThreads;
}

inline int phConfig::GetTotalMaxNumWorkerThreads()
{
	return sm_MaxNumWorkerThreads + (sm_MainThreadWorks ? 1 : 0);
}

inline bool phConfig::IsRefCountingEnabled ()
{
	return sm_RefCounting;
}

inline void phConfig::FreezeRefCounting()
{
	sm_RefCountingFrozen = true;
}

inline void phConfig::SetWorkerThreadCount(int count)
{
	SetForceSolverWorkerThreadCount(count);
	SetForceSolverOutOfOrderTolerance(count / 2);
	SetContactWorkerThreadCount(count);
#if EARLY_FORCE_SOLVE
	SetIntegrateVelocitiesWorkerThreadCount(count);
	SetIntegratePositionsWorkerThreadCount(count);
	SetApplyPushesWorkerThreadCount(count);
#else // EARLY_FORCE_SOLVE
	SetColliderPreWorkerThreadCount(count);
	SetColliderPostWorkerThreadCount(count);
#endif // EARLY_FORCE_SOLVE
	SetCollisionWorkerThreadCount(count);
}

inline void phConfig::SetMainThreadAlsoWorks(bool enable)
{
	SetForceSolverMainThreadAlsoWorks(enable);
	SetContactMainThreadAlsoWorks(enable);
#if EARLY_FORCE_SOLVE
	SetIntegrateVelocitiesMainThreadAlsoWorks(enable);
	SetIntegratePositionsMainThreadAlsoWorks(enable);
	SetApplyPushesMainThreadAlsoWorks(enable);
#else // EARLY_FORCE_SOLVE
	SetColliderPreMainThreadAlsoWorks(enable);
	SetColliderPostMainThreadAlsoWorks(enable);
#endif // EARLY_FORCE_SOLVE
	SetCollisionMainThreadAlsoWorks(enable);
}

inline void phConfig::SetForceSolverTaskScheduler(int index)
{
	sm_ForceSolverTaskScheduler = index;
}

inline int phConfig::GetForceSolverTaskScheduler()
{
	return sm_ForceSolverTaskScheduler;
}

inline void phConfig::SetForceSolverWorkerThreadCount(int count)
{
	Assert(count >= 0);
	sm_ForceSolverWorkerThreadCount = count;
}

inline int phConfig::GetForceSolverWorkerThreadCount()
{
	return sm_ForceSolverWorkerThreadCount;
}

inline int& phConfig::GetForceSolverWorkerThreadCountRef()
{
	return sm_ForceSolverWorkerThreadCount;
}

inline int phConfig::GetForceSolverWorkerThreadTotalCount()
{
	return sm_ForceSolverWorkerThreadCount + (sm_ForceSolverMainThreadAlsoWorks ? 1 : 0);
}

inline void phConfig::SetForceSolverMainThreadAlsoWorks(bool enable)
{
	sm_ForceSolverMainThreadAlsoWorks = enable;
}

inline bool phConfig::GetForceSolverMainThreadAlsoWorks()
{
	return sm_ForceSolverMainThreadAlsoWorks;
}

inline bool& phConfig::GetForceSolverMainThreadAlsoWorksRef()
{
	return sm_ForceSolverMainThreadAlsoWorks;
}

inline void phConfig::SetForceSolverMaxPhases(int maxPhases)
{
	sm_ForceSolverMaxPhases = maxPhases;
}

inline int phConfig::GetForceSolverMaxPhases()
{
	return sm_ForceSolverMaxPhases;
}

inline int& phConfig::GetForceSolverMaxPhasesRef()
{
	return sm_ForceSolverMaxPhases;
}

inline void phConfig::SetForceSolverOutOfOrderTolerance(u32 tolerance)
{
	sm_ForceSolverOutOfOrderTolerance = tolerance;
}

inline u32 phConfig::GetForceSolverOutOfOrderTolerance()
{
	return sm_ForceSolverOutOfOrderTolerance;
}

inline u32& phConfig::GetForceSolverOutOfOrderToleranceRef()
{
	return sm_ForceSolverOutOfOrderTolerance;
}

inline void phConfig::SetContactTaskScheduler(int index)
{
	sm_ContactTaskScheduler = index;
}

inline int phConfig::GetContactTaskScheduler()
{
	return sm_ContactTaskScheduler;
}

inline void phConfig::SetContactWorkerThreadCount(int count)
{
	Assert(count >= 0);
	sm_ContactWorkerThreadCount = count;
}

inline int phConfig::GetContactWorkerThreadCount()
{
	return sm_ContactWorkerThreadCount;
}

inline int& phConfig::GetContactWorkerThreadCountRef()
{
	return sm_ContactWorkerThreadCount;
}

inline int phConfig::GetContactWorkerThreadTotalCount()
{
	return sm_ContactWorkerThreadCount + (sm_ContactMainThreadAlsoWorks ? 1 : 0);
}

inline void phConfig::SetContactMainThreadAlsoWorks(bool enable)
{
	sm_ContactMainThreadAlsoWorks = enable;
}

inline bool phConfig::GetContactMainThreadAlsoWorks()
{
	return sm_ContactMainThreadAlsoWorks;
}

inline bool& phConfig::GetContactMainThreadAlsoWorksRef()
{
	return sm_ContactMainThreadAlsoWorks;
}

inline void phConfig::SetColliderTaskScheduler(int index)
{
	sm_ColliderTaskScheduler = index;
}

inline int phConfig::GetColliderTaskScheduler()
{
	return sm_ColliderTaskScheduler;
}

#if EARLY_FORCE_SOLVE
inline void phConfig::SetIntegrateVelocitiesWorkerThreadCount(int count)
{
	Assert(count >= 0);
	sm_IntegrateVelocitiesWorkerThreadCount = count;
}

inline int phConfig::GetIntegrateVelocitiesWorkerThreadCount()
{
	return sm_IntegrateVelocitiesWorkerThreadCount;
}

inline int& phConfig::GetIntegrateVelocitiesWorkerThreadCountRef()
{
	return sm_IntegrateVelocitiesWorkerThreadCount;
}

inline int phConfig::GetIntegrateVelocitiesWorkerThreadTotalCount()
{
	return sm_IntegrateVelocitiesWorkerThreadCount + (sm_IntegrateVelocitiesMainThreadAlsoWorks ? 1 : 0);
}

inline void phConfig::SetIntegrateVelocitiesMainThreadAlsoWorks(bool enable)
{
	sm_IntegrateVelocitiesMainThreadAlsoWorks = enable;
}

inline bool phConfig::GetIntegrateVelocitiesMainThreadAlsoWorks()
{
	return sm_IntegrateVelocitiesMainThreadAlsoWorks;
}

inline bool& phConfig::GetIntegrateVelocitiesMainThreadAlsoWorksRef()
{
	return sm_IntegrateVelocitiesMainThreadAlsoWorks;
}

inline void phConfig::SetIntegratePositionsWorkerThreadCount(int count)
{
	Assert(count >= 0);
	sm_IntegratePositionsWorkerThreadCount = count;
}

inline int phConfig::GetIntegratePositionsWorkerThreadCount()
{
	return sm_IntegratePositionsWorkerThreadCount;
}

inline int& phConfig::GetIntegratePositionsWorkerThreadCountRef()
{
	return sm_IntegratePositionsWorkerThreadCount;
}

inline int phConfig::GetIntegratePositionsWorkerThreadTotalCount()
{
	return sm_IntegratePositionsWorkerThreadCount + (sm_IntegratePositionsMainThreadAlsoWorks ? 1 : 0);
}

inline void phConfig::SetIntegratePositionsMainThreadAlsoWorks(bool enable)
{
	sm_IntegratePositionsMainThreadAlsoWorks = enable;
}

inline bool phConfig::GetIntegratePositionsMainThreadAlsoWorks()
{
	return sm_IntegratePositionsMainThreadAlsoWorks;
}

inline bool& phConfig::GetIntegratePositionsMainThreadAlsoWorksRef()
{
	return sm_IntegratePositionsMainThreadAlsoWorks;
}

inline void phConfig::SetApplyPushesWorkerThreadCount(int count)
{
	Assert(count >= 0);
	sm_ApplyPushesWorkerThreadCount = count;
}

inline int phConfig::GetApplyPushesWorkerThreadCount()
{
	return sm_ApplyPushesWorkerThreadCount;
}

inline int& phConfig::GetApplyPushesWorkerThreadCountRef()
{
	return sm_ApplyPushesWorkerThreadCount;
}

inline int phConfig::GetApplyPushesWorkerThreadTotalCount()
{
	return sm_ApplyPushesWorkerThreadCount + (sm_ApplyPushesMainThreadAlsoWorks ? 1 : 0);
}

inline void phConfig::SetApplyPushesMainThreadAlsoWorks(bool enable)
{
	sm_ApplyPushesMainThreadAlsoWorks = enable;
}

inline bool phConfig::GetApplyPushesMainThreadAlsoWorks()
{
	return sm_ApplyPushesMainThreadAlsoWorks;
}

inline bool& phConfig::GetApplyPushesMainThreadAlsoWorksRef()
{
	return sm_ApplyPushesMainThreadAlsoWorks;
}
#else // EARLY_FORCE_SOLVE
inline void phConfig::SetColliderPreWorkerThreadCount(int count)
{
	Assert(count >= 0);
	sm_ColliderPreWorkerThreadCount = count;
}

inline int phConfig::GetColliderPreWorkerThreadCount()
{
	return sm_ColliderPreWorkerThreadCount;
}

inline int& phConfig::GetColliderPreWorkerThreadCountRef()
{
	return sm_ColliderPreWorkerThreadCount;
}

inline int phConfig::GetColliderPreWorkerThreadTotalCount()
{
	return sm_ColliderPreWorkerThreadCount + (sm_ColliderPreMainThreadAlsoWorks ? 1 : 0);
}

inline void phConfig::SetColliderPreMainThreadAlsoWorks(bool enable)
{
	sm_ColliderPreMainThreadAlsoWorks = enable;
}

inline bool phConfig::GetColliderPreMainThreadAlsoWorks()
{
	return sm_ColliderPreMainThreadAlsoWorks;
}

inline bool& phConfig::GetColliderPreMainThreadAlsoWorksRef()
{
	return sm_ColliderPreMainThreadAlsoWorks;
}

inline void phConfig::SetColliderPostWorkerThreadCount(int count)
{
	Assert(count >= 0);
	sm_ColliderPostWorkerThreadCount = count;
}

inline int phConfig::GetColliderPostWorkerThreadCount()
{
	return sm_ColliderPostWorkerThreadCount;
}

inline int& phConfig::GetColliderPostWorkerThreadCountRef()
{
	return sm_ColliderPostWorkerThreadCount;
}

inline int phConfig::GetColliderPostWorkerThreadTotalCount()
{
	return sm_ColliderPostWorkerThreadCount + (sm_ColliderPostMainThreadAlsoWorks ? 1 : 0);
}

inline void phConfig::SetColliderPostMainThreadAlsoWorks(bool enable)
{
	sm_ColliderPostMainThreadAlsoWorks = enable;
}

inline bool phConfig::GetColliderPostMainThreadAlsoWorks()
{
	return sm_ColliderPostMainThreadAlsoWorks;
}

inline bool& phConfig::GetColliderPostMainThreadAlsoWorksRef()
{
	return sm_ColliderPostMainThreadAlsoWorks;
}
#endif // EARLY_FORCE_SOLVE

inline void phConfig::SetCollisionTaskScheduler(int index)
{
	sm_CollisionTaskScheduler = index;
}

inline int phConfig::GetCollisionTaskScheduler()
{
	return sm_CollisionTaskScheduler;
}

inline void phConfig::SetCollisionWorkerThreadCount(int count)
{
	Assert(count >= 0);
	sm_CollisionWorkerThreadCount = count;
}

inline int phConfig::GetCollisionWorkerThreadCount()
{
	return sm_CollisionWorkerThreadCount;
}

inline int& phConfig::GetCollisionWorkerThreadCountRef()
{
	return sm_CollisionWorkerThreadCount;
}

inline int phConfig::GetCollisionWorkerThreadTotalCount()
{
	return sm_CollisionWorkerThreadCount + (sm_CollisionMainThreadAlsoWorks ? 1 : 0);
}

inline void phConfig::SetCollisionMainThreadAlsoWorks(bool enable)
{
	sm_CollisionMainThreadAlsoWorks = enable;
}

inline bool phConfig::GetCollisionMainThreadAlsoWorks()
{
	return sm_CollisionMainThreadAlsoWorks;
}

inline bool& phConfig::GetCollisionMainThreadAlsoWorksRef()
{
	return sm_CollisionMainThreadAlsoWorks;
}

#endif // __SPU

} // namespace rage

#endif // PHCORE_CONFIG_H
