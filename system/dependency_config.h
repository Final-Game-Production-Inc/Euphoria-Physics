// 
// system/dependency_config.h 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_DEPENDENCY_CONFIG_H
#define SYSTEM_DEPENDENCY_CONFIG_H

// PURPOSE: Enable SPU dependencies (creates separate chain for SPU tasks, kicks SPU workers)
#define SYS_USE_SPU_DEPENDENCY (__PS3)

// PURPOSE: Ideal number of pending dependencies per task worker (will spin up more tasks if above this)
#define SYS_DEPENDENCY_TASKWORKERPENDINGRATIO (3)

// PURPOSE: Maximum number of task worker
#if RSG_PC
	#define SYS_DEPENDENCY_TASKMAXNUMWORKERS (64)
	#define SYS_DEPENDENCY_TASKRESERVEDWORKERS 2
	#define SYS_DEPENDENCY_TASKACTIVENUMWORKERS (Max(sysTaskManager::GetNumThreads() - SYS_DEPENDENCY_TASKRESERVEDWORKERS, 1))
#else
	#define SYS_DEPENDENCY_TASKMAXNUMWORKERS (4)
	#define SYS_DEPENDENCY_TASKRESERVEDWORKERS 0
	#define SYS_DEPENDENCY_TASKACTIVENUMWORKERS SYS_DEPENDENCY_TASKMAXNUMWORKERS
#endif

// PURPOSE: Worker thread stack size
#if !__OPTIMIZED
#define SYS_DEPENDENCY_WORKERSTACKSIZE (300*1024)
#elif __64BIT
#define SYS_DEPENDENCY_WORKERSTACKSIZE (144*1024)
#else
#define SYS_DEPENDENCY_WORKERSTACKSIZE (64*1024)
#endif

// PURPOSE: Worker thread priority
#define SYS_DEPENDENCY_WORKERPRIORITY (PRIO_HIGHEST)

// PURPOSE: Worker scratch space
#if __64BIT
#define SYS_DEPENDENCY_WORKERSCRATCH (96*1024)
#else
#define SYS_DEPENDENCY_WORKERSCRATCH (24*1024)
#endif

#endif  // SYSTEM_DEPENDENCY_CONFIG_H
