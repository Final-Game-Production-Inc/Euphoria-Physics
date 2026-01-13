// 
// system/task.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_TASK_H
#define SYSTEM_TASK_H

#include <stddef.h>		// for size_t

#include "data/base.h"
#include "system/ipc.h"

#include "taskheader.h"

#if __PS3 
#include <sys/sys_types.h>
#include <cell/spurs/task_types.h>
#include <cell/spurs/job_chain_types.h>
struct sys_event;
#endif // __PS3 


#define TASK_ALLOW_SPU_ADD_JOB	1
#define TASK_USE_CONTINUATION_TASK 0

namespace rage {
enum
{
	// NOTE: ejanderson - CELL_SPURS_MAX_SIZE_JOB_MEMORY is defined as 234 KB.
	// c:\usr\local\cell\SDK_doc\en\chm\PS3_SDKDoc_e.chm  says we have 234k user space
	// Alastair reports he's had problems with going over 231k, so let's round it off.
	kMaxSPUJobSize = (230*1024),
};


#if __PS3

enum
{
	SYSTASKMANAGER_EVENT_PORT = 1,

	EVENTCMD_PRINTF = 0,
	NUM_SYSTEM_EVENTCMDS,

	MAX_NUM_SUPPORTED_EVENTCMDS = 4
};

// Notice we use sys_event not sys_event_t here in the header file.  sys_event_t
// is a typedef of struct sys_event (since the SPURS headers are C not C++).
// That means we cannot forward declare sys_event_t.
typedef void SpuEventHandler(const sys_event *event);

// Helper function use in event handlers.
extern int SpuThreadReadLs(void* ea, sys_spu_thread_t spuThread, u32 ls, u32 size);

#endif // __PS3


class bkGroup;

/*
	This class allows you to create and submit arbitrary tasks for asynchronous completion.

	Once you create a task, you must eventually block on its completion, at which time you
	know the outputs are valid.
*/
class sysTaskManager : public datBase {
public:
	// PURPOSE:	Initialize the task framework
	// NOTES:	On Win32 platform, no scheduler objects are created yet.  Instead, the system
	//			will create a default scheduler using all available cores if no scheduler
	//			had been created before the first task is created.
	static void InitClass();

	// PURPOSE:	Shutdown the task framework
	static void ShutdownClass();

#if __PS3
#if HACK_GTA4
	enum 
	{
		SCHEDULER_DEFAULT_SHORT,		// Primary:		high prio on SPU0 SPU1, lower prio on SPU2 SPU3, lowest on SPU4
		SCHEDULER_DEFAULT_LONG,			// Primary:		normal prio SPU2 SPU3 only (can't load balance well because too long)
		SCHEDULER_SHAPE_TEST,			// Primary:		normal prio on SPU0 SPU1, lower prio on SPU2 SPU3, lowest on SPU4 (non-BANK only)
		SCHEDULER_AUDIO_ENGINE,			// Primary:		high prio on SPU2 SPU3, lowest on SPU4
		SCHEDULER_GRAPHICS_MAIN,		// Primary:		high prio on SPU5 only (drawablespu)
		SCHEDULER_GRAPHICS_OTHER,		// Primary:		SPU0 only to avoid deadlocks with kJobChainMainGraphics - higher priority than default jobs
		SCHEDULER_SECONDARY,			// Secondary:	SPURS instance (shared with DDLENC)

		NUM_SCHEDULES,
	};
#else
	enum
	{
		SCHEDULE_DEFAULT,		// Can run on any SPU, prefer SPU0 and really try to avoid SPU4, not to disturb the patcher
		SCHEDULER_GRAPHICS_MAIN,		// Edge currently runs on SPU2-SPU3 and we want to avoid deadlocks)
		SCHEDULE_HIGH_PRI_RSX,	// High priority jobs that avoid RSX deadlocks with patcher & edge
		SCHEDULE_SECONDARY,		// Secondary instance - shared with dolby
		SCHEDULE_LOW_PRIORITY_1ON2,	// Low priority jobs, these can run on one of 2 SPUs, but only 1 job at a time
		SCHEDULE_LOW_PRIORITY_2ON4,	// Low priority jobs, these can run on one of 4 SPUs, but only 2 jobs at a time
		SCHEDULE_LOW_PRIORITY_4ON4,	// Low priority jobs, these can run on one of 4 SPUs.
		SCHEDULE_SEQUENTIAL_A,
		SCHEDULE_SEQUENTIAL_B,
		SCHEDULE_5ON5,				// 5 jobs at a time, on any SPU 0 to 4. This can interfere with the patcher.
		
		NUM_SCHEDULES
	};
#endif // HACK_GTA
#endif

	// PURPOSE:	Gets a task handle pointer
	static __sysTaskHandle* AllocateTaskHandle();

#if __WIN32 || __PSP2 || RSG_ORBIS
	static void FreeTaskHandle(sysTaskHandle task);
#endif

	// PURPOSE:	Creates a task and schedules it for completion.
	// PARAMS:	task - task object (maps to a function address or embedded code address)
	//			params - Task parameter structure
	//			schedulerIndex - Which scheduler to assign the job to.  Win32 only.
	//				Each scheduler maintains its own work queue.
	// RETURNS:	Nonzero handle on success, else zero.
	// NOTES:	If input and output point to the same place, they must also have the
	//			same size.  On Win32 builds, if no schedulers have been created yet,
	//			a default one is created before creating the task.
	static sysTaskHandle Create(TASK_INTERFACE_PARAMS,const sysTaskParameters &params,int schedulerIndex = 0);

	// PURPOSE:	Creates a task but doesn't schedule it for execution.
	// PARAMS:	task - task object (maps to a function address or embedded code address)
	//			params - Task parameter structure
	//			schedulerIndex - Which scheduler to assign the job to.  Win32 only.
	//				Each scheduler maintains its own work queue.
	// RETURNS:	Nonzero handle on success, else zero.
	// NOTES:	If input and output point to the same place, they must also have the
	//			same size.  On Win32 builds, if no schedulers have been created yet,
	//			a default one is created before creating the task.
	static sysPreparedTaskHandle Prepare(TASK_INTERFACE_PARAMS,const sysTaskParameters &params,int schedulerIndex = 0);

	// PURPOSE : Executes a prepared task
	static sysTaskHandle Dispatch( const sysPreparedTaskHandle& handle, int schedulerIndex = 0 );
	
	static void DispatchLocally( const sysPreparedTaskHandle& prepHdle);

#if __WIN32 || __PSP2 || RSG_ORBIS
	// PURPOSE:	Create a new scheduler object and add it to the system.
	// PARAMS:	name - Name for the scheduler (For debugger thread view) (up to 13 characters)
	//			stackSize - Thread stack size of each worker thread
	//			scratchSize - Amount of scratch space allocated for each worker thread
	//			priority - Priority to run worker threads at.  If numerically lower than 
	//				sysIpcBasePriority, threads will preempt the main thread; this should 
	//				be used for short-term jobs that the main thread will block on.
	//			cpuMask - Bitmask of which processors to use.  On Xenon, hardware threads
	//				zero and five are generally pretty full, so long-term tasks should
	//				avoid running on those.
	//			schedulerCpu - Which cpu number to run the scheduler on.
	// NOTES:	We assume that applications create their schedulers once at startup
	//			since we don't provide a mechanism for destroying them.
	static int AddScheduler(const char *name,int stackSize,int scratchSize,sysIpcPriority priority,int cpuMask,int schedulerCpu);
#endif

	// PURPOSE:	Polls for task completion
	// PARAMS:	handle - handle of task to check
	// RETURNS:	True if task is complete, else false
	// NOTES:	Polling should be minimized because some threads may not get scheduled until you
	//			block on their completion.  If Poll returns true, the task handle is freed
	//			exactly as if you'd successfully Wait'd on it.
	static bool Poll(sysTaskHandle handle);

	// PURPOSE:	Blocks for task completion
	// PARAMS:	handle - handle of task to check
	// RETURNS:	True if task is complete, else false
	static void Wait(sysTaskHandle handle);

	// PURPOSE:	Blocks on all tasks being complete; simply a shorthand for calling Wait on each handle in turn.
	// PARAMS:	count - Number of tasks to check.
	//			tasks - Pointer to array of task handles.  NULL pointers are safely ignored.
	// NOTES:	Task handles are freed once this function completes
	static void WaitMultiple(int count,const sysTaskHandle *tasks);

	// PURPOSE:	Return name of task for debugging purposes
	// PARAMS:  handle - handle of task to get name for 
	// RETURNS: name
#if !__FINAL
	static const char* GetTaskName(sysTaskHandle handle);
#endif

	// PURPOSE: Determine the maximum number of tasks the system supports
	static int GetMaxTasks();

	// PURPOSE: Determine the number of simultaneous threads the system supports
	static int GetNumThreads();

#if __WIN32PC && !__TOOL
	static int GetProcessorPackages();
	static int GetProcessorCores();
	static int GetLogicalCores();
	
	static bool GetProcessorInfo(int &iProcessorPackages, int &iProcessorCores, int &iLogicalCores);
	static bool GetStandardProcessorInfo(int &iProcessorPackages, int &iProcessorCores, int &iLogicalCores);
#endif // __WIN32PC

#if __WIN32 && TASK_USE_CONTINUATION_TASK
	// PURPOSE: Sets the next task to be executed on the same thread.
	// can be useful to chain tasks together.
	static bool SetContinuationTask( sysTaskHandle handle);
#endif

#if __PS3 
	static void CreateTaskset(CellSpursTaskset* taskset, uint64_t argTaskset, const uint8_t priority[8], unsigned int max_contention, const char *staticName);
	static void SetExceptionHandler(CellSpursJobChain* jobChain);
	static void SetSpuEventHandler(unsigned id, SpuEventHandler *handler);
#endif

#if __BANK
	static void AddWidgets(bkGroup &bank);
	static void PrintJobs();
	
#if __PS3
	static void UpdatePriorities(void *schedulerIndex);
#endif // __PS3
#endif // __BANK

	static DECLARE_THREAD_FUNC(TaskWrapper);

private:
	static DECLARE_THREAD_FUNC(Scheduler);
	static sysTaskHandle Setup(TASK_INTERFACE_PARAMS,const sysTaskParameters &p,int schedulerIndex);

#if __WIN32PC
	static int sm_iProcessorPackages;
	static int sm_iProcessorCores;
	static int sm_iLogicalCores;
#endif // __WIN32PC
};


#if __PPU

class AutoRegisterSpuEventHandler
{
public:

	AutoRegisterSpuEventHandler(unsigned id, SpuEventHandler *handler)
	{
		sysTaskManager::SetSpuEventHandler(id, handler);
	}

	// No need to worry about a dtor, just a waste of code space
};

#define REGISTER_SPU_EVENT_HANDLER(EVENT_ID, HANDLER) \
	static const AutoRegisterSpuEventHandler s_AutoRegisterSpuEventHandler_##HANDLER((EVENT_ID), HANDLER)

#endif // __PPU

}	// namespace rage

#endif
