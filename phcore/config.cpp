//
// phcore/config.cpp
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#include "config.h"

using namespace rage;

//=============================================================================
// phConfig

bool phConfig::sm_RefCounting = false;
bool phConfig::sm_RefCountingFrozen = false;

int phConfig::sm_MaxNumWorkerThreads = -1;
bool phConfig::sm_MainThreadWorks = false;

int phConfig::sm_ForceSolverTaskScheduler = 0;
int phConfig::sm_ForceSolverWorkerThreadCount = 0;
bool phConfig::sm_ForceSolverMainThreadAlsoWorks = false;

int phConfig::sm_ForceSolverMaxPhases = 2048;
u32 phConfig::sm_ForceSolverOutOfOrderTolerance = 0;

int phConfig::sm_ContactTaskScheduler = 0;
int phConfig::sm_ContactWorkerThreadCount = 0;
bool phConfig::sm_ContactMainThreadAlsoWorks = false;

int phConfig::sm_ColliderTaskScheduler = 0;

#if EARLY_FORCE_SOLVE
int phConfig::sm_IntegrateVelocitiesWorkerThreadCount = 0;
bool phConfig::sm_IntegrateVelocitiesMainThreadAlsoWorks = false;

int phConfig::sm_IntegratePositionsWorkerThreadCount = 0;
bool phConfig::sm_IntegratePositionsMainThreadAlsoWorks = false;

int phConfig::sm_ApplyPushesWorkerThreadCount = 0;
bool phConfig::sm_ApplyPushesMainThreadAlsoWorks = false;
#else // EARLY_FORCE_SOLVE
int phConfig::sm_ColliderPreWorkerThreadCount = 0;
bool phConfig::sm_ColliderPreMainThreadAlsoWorks = false;

int phConfig::sm_ColliderPostWorkerThreadCount = 0;
bool phConfig::sm_ColliderPostMainThreadAlsoWorks = false;
#endif // EARLY_FORCE_SOLVE

int phConfig::sm_CollisionTaskScheduler = 0;
int phConfig::sm_CollisionWorkerThreadCount = 0;
bool phConfig::sm_CollisionMainThreadAlsoWorks = false;

#if __DEV
bool phConfig::sm_ValidatePhysics = false;
#endif

void phConfig::EnableRefCounting(bool enable)
{
	Assert(!sm_RefCountingFrozen);
	sm_RefCounting = enable;
}

