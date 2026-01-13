// 
// system/dependencyscheduler.cpp 
// 
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
// 

#include "dependencyscheduler.h"

#include "alloca.h"
#include "dependency.h"
#include "dependency_config.h"
#include "ipc.h"
#include "lockfreering.h"
#include "pix.h"
#include "task.h"
#include "param.h"

#include "math/amath.h"
#include "profile/element.h"
#include "string/string.h"
#include "system/threadtype.h"

PARAM(singleThreadedTaskManager, "Task schedudler/manager will dispatch tasks on the local thread");

// In Bank builds, alloc scratch space from the heap if the requested size is larger than the thread's pre-allocated
// scratch space.  (In Release/Final builds, we expect the stack scratch to always be large enough, and we will Quitf
// if it's not)
#define DO_HEAP_ALLOC_FOR_LARGE_REQUESTS			(__BANK)

namespace rage
{

////////////////////////////////////////////////////////////////////////////////

class Chain
{
public:
	// PURPOSE: Constructor
	Chain();

	// PURPOSE: Pop (highest priority) dependency task from head of chain
	sysDependency* Pop();

	// PURPOSE: Push dependency task into chain
	// RETURN: incremented number of pending
	u32 Push(u32 priority, sysDependency* dependency);

	// PURPOSE: Get number of pending
	u32 GetNumPending() const { return m_NumPending; }

private:
	static const u32 sm_NumRings = sysDependency::kPriorityNum;
	sysLockFreeRing<sysDependency, 2048> m_Rings[sm_NumRings];

	u32 m_NumPending;
};

////////////////////////////////////////////////////////////////////////////////

Chain::Chain()
: m_NumPending(0)
{
}

////////////////////////////////////////////////////////////////////////////////

sysDependency* Chain::Pop()
{
	for(int i=(sm_NumRings-1); i>=0; --i)
	{
		if(!m_Rings[i].IsEmpty())
		{
			sysDependency* dep = m_Rings[i].Pop();
			if(dep)
			{
				sysInterlockedDecrement(&m_NumPending);
				return dep;
			}
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

u32 Chain::Push(u32 priority, sysDependency* dependency)
{
	FastAssert(priority < Chain::sm_NumRings);
	while(!m_Rings[priority].Push(dependency));
	return sysInterlockedIncrement(&m_NumPending);
}

////////////////////////////////////////////////////////////////////////////////

#if SYS_DEPENDENCY_PROFILE
class pfDepTimer : public pfTimer
{
public:
	pfDepTimer(const char* name, pfGroup& group)
		: pfTimer(name, group, true) 
	{
		UElapsedFrameTime = 0;
	}

	utimer_t* GetCounter()
	{ 
		return &UElapsedFrameTime; 
	}
};
#endif

////////////////////////////////////////////////////////////////////////////////

struct WorkerContext
{
	sysIpcThreadId m_Thread;
	sysIpcEvent m_Event;
	u32 m_Active;
};

////////////////////////////////////////////////////////////////////////////////

struct DependencyContext
{
	WorkerContext m_Workers[SYS_DEPENDENCY_TASKMAXNUMWORKERS];
	Chain m_Chain;
	u32 m_Alive;

#if SYS_DEPENDENCY_PROFILE
	static const u32 sm_MaxNumTags = 256;
	pfDepTimer* m_Timers[sm_MaxNumTags];
	utimer_t* m_Counters[sm_MaxNumTags];
#endif
};

////////////////////////////////////////////////////////////////////////////////

DependencyContext s_DependencyContext;

////////////////////////////////////////////////////////////////////////////////

void sysDependencyScheduler::InitClass()
{
	DependencyContext& context = s_DependencyContext;
	context.m_Alive = true;

	// create worker threads
	char threadName[32];
	u32 numWorkers = SYS_DEPENDENCY_TASKACTIVENUMWORKERS;
	for(u32 i=0; i < numWorkers; i++)
	{
		formatf(threadName, "[RAGE] Dependency %d", i);
		WorkerContext& worker = context.m_Workers[i];
		worker.m_Event = sysIpcCreateEvent();
#if RSG_DURANGO || RSG_ORBIS
		static const u32 affinities[SYS_DEPENDENCY_TASKACTIVENUMWORKERS] = { 1, 2, 3, 5 };
		int processorAffinity = affinities[i];
#else
		int processorAffinity = ( i % 5 ) + SYS_DEPENDENCY_TASKRESERVEDWORKERS;
#endif
		worker.m_Active = true;
		worker.m_Thread = sysIpcCreateThread(DependencyWorkerThread, (void*)(size_t)i, SYS_DEPENDENCY_WORKERSTACKSIZE, SYS_DEPENDENCY_WORKERPRIORITY, threadName, processorAffinity);
	}

#if SYS_DEPENDENCY_PROFILE
	memset(context.m_Timers, 0, sizeof(context.m_Timers));
	memset(context.m_Counters, 0, sizeof(context.m_Counters));
#endif
}

////////////////////////////////////////////////////////////////////////////////

void sysDependencyScheduler::ShutdownClass()
{
	// exit workers
	DependencyContext& context = s_DependencyContext;
	context.m_Alive = false;

	for(int i=0; i < SYS_DEPENDENCY_TASKACTIVENUMWORKERS; i++)
	{
		WorkerContext& worker = context.m_Workers[i];
		sysIpcSetEvent(worker.m_Event);
		sysIpcWaitThreadExit(worker.m_Thread);
		sysIpcDeleteEvent(worker.m_Event);
		worker.m_Thread = sysIpcThreadIdInvalid;
	}

#if SYS_DEPENDENCY_PROFILE
	for(int i=0; i < DependencyContext::sm_MaxNumTags; i++)
	{
		delete context.m_Timers[i];
		context.m_Timers[i] = NULL;
		context.m_Counters[i] = NULL;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////

void sysDependencyScheduler::Insert(sysDependency* dep)
{
	Assertf(!dep->m_NumPending, "Only dependency without children should be inserted");
	InsertInternal(dep);
}

////////////////////////////////////////////////////////////////////////////////

void sysDependencyScheduler::InsertInternal(sysDependency* dep)
{
	Assertf( ((dep->m_Flags & sysDepFlag::ALLOC0) == 0 || dep->m_DataSizes[0] <= SYS_DEPENDENCY_WORKERSCRATCH),
			"Scratch size (%u) too large for %u - either increase SYS_DEPENDENCY_WORKERSCRATCH or reduce the scratch size requested by this dependency.  This will crash in non-BANK builds",
			dep->m_DataSizes[0], SYS_DEPENDENCY_WORKERSCRATCH);
#if !DO_HEAP_ALLOC_FOR_LARGE_REQUESTS && !__FINAL
	if((dep->m_Flags & sysDepFlag::ALLOC0) && dep->m_DataSizes[0] > SYS_DEPENDENCY_WORKERSCRATCH)
	{
		Quitf("Scratch size (%u) too large - either increase SYS_DEPENDENCY_WORKERSCRATCH or reduce the scratch size requested by this dependency", dep->m_DataSizes[0]);
	}
#endif

	DependencyContext& context = s_DependencyContext;
	Chain& chain = context.m_Chain;

	// insert into chain
	u32 priority = dep->m_Priority;
	u32 numPending = chain.Push(priority, dep);

	if(PARAM_singleThreadedTaskManager.Get())
	{
		// Run it locally first
		DependencyLocalThread();

		if(chain.GetNumPending() == 0)
		{
			return;
		}
	}

	// check if we need more workers
	const u32 maxWorkers = SYS_DEPENDENCY_TASKACTIVENUMWORKERS;
	const u32 workerPendingRatio = SYS_DEPENDENCY_TASKWORKERPENDINGRATIO;
	const u32 extraWorkerFromCriticalPriority = (priority == sysDependency::kPriorityCritical) ? 1 : 0;
	const u32 numDesiredWorkers = rage::Min(((numPending + workerPendingRatio-1) / workerPendingRatio) + extraWorkerFromCriticalPriority, maxWorkers);

	for(u32 i = 0; i < numDesiredWorkers; i++)
	{
		WorkerContext& worker = context.m_Workers[i];
		if(!worker.m_Active)
		{
			worker.m_Active = true;
			sysIpcSetEvent(worker.m_Event);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

#if !__FINAL
void sysDependencyScheduler::OutputDebug()
{
	u32 numWorkers = SYS_DEPENDENCY_TASKACTIVENUMWORKERS;
	DependencyContext& context = s_DependencyContext;
	Printf("****************************** sysDependencyScheduler ***********************************\n");
	Printf("Chain : pending %d\n", context.m_Chain.GetNumPending());
	for(u32 i = 0; i < numWorkers; i++)
	{
		Printf("Worker[%d] : active %d\n", i, context.m_Workers[i].m_Active);
	}
}
#endif // !__FINAL

////////////////////////////////////////////////////////////////////////////////

#if SYS_DEPENDENCY_PROFILE
void sysDependencyScheduler::RegisterTag(pfGroup& group, u32 tag, const char* name)
{
	DependencyContext& context = s_DependencyContext;
	Assert(tag < DependencyContext::sm_MaxNumTags);
	Assert(!context.m_Timers[tag]);
	pfDepTimer* timer = rage_new pfDepTimer(name, group);
	timer->Init();
	context.m_Timers[tag] = timer;
	context.m_Counters[tag] = timer->GetCounter();
}
#endif // SYS_DEPENDENCY_PROFILE

////////////////////////////////////////////////////////////////////////////////

void sysDependencyScheduler::DependencyWorkerThread(void* param)
{
	sysThreadType::AddCurrentThreadType(sysThreadType::THREAD_TYPE_DEPENDENCY);

	sysDependency* parents[sysDependency::sm_MaxNumParents];
	DependencyContext& context = s_DependencyContext;
	Chain& chain = context.m_Chain;
	u32 workerIdx = (u32)(size_t)param;
	WorkerContext& worker = context.m_Workers[workerIdx];
	u8* stackScratch = Alloca(u8, SYS_DEPENDENCY_WORKERSCRATCH);
	sysDependency* dep = chain.Pop();

	PF_PUSH_MARKERCN(0xff00ffff, "Dependency:%d",workerIdx);

	while(1)
	{
		// retrieve dependency
		while(!dep)
		{
			// must be false before the last pop
			sysInterlockedExchange(&worker.m_Active, false);
			dep = chain.Pop();
			if(dep)
			{
				worker.m_Active = true;
				break;
			}

			PF_POP_MARKER();
			sysIpcWaitEvent(worker.m_Event);
			PF_PUSH_MARKERCN(0xff00ffff, "Dependency:%d",workerIdx);

			if(!context.m_Alive)
				return;
		}
		TELEMETRY_START_ZONE(PZONE_NORMAL, __FILE__,__LINE__, "Dependency Task");

		// copy parents before execution
		for(u32 i=0; i < sysDependency::sm_MaxNumParents; i++)
		{
			parents[i] = dep->m_Parents[i];
		}
		u32 priority = dep->m_Priority;

#if DO_HEAP_ALLOC_FOR_LARGE_REQUESTS
		// allocate temporary space if required
		u8* heapScratch = NULL;
#endif
		if(dep->m_Flags & sysDepFlag::ALLOC0)
		{
#if DO_HEAP_ALLOC_FOR_LARGE_REQUESTS
			u32 scratchSize = dep->m_DataSizes[0];
			if (scratchSize <= SYS_DEPENDENCY_WORKERSCRATCH)
			{
				dep->m_Params[0].m_AsPtr = stackScratch;
			}
			else
			{
				heapScratch = rage_aligned_new(16) u8[scratchSize];
				dep->m_Params[0].m_AsPtr = heapScratch;
			}
#else
			FatalAssert(dep->m_DataSizes[0] <= SYS_DEPENDENCY_WORKERSCRATCH);
			dep->m_Params[0].m_AsPtr = stackScratch;
#endif
		}

#if SYS_DEPENDENCY_PROFILE
		pfDepTimer* timer = NULL;
		if(dep->m_Id)
		{
			timer = context.m_Timers[dep->m_Id];
			timer->Start();
		}
#endif // SYS_DEPENDENCY_PROFILE

		// execute dependency callbacks
		bool result = dep->m_Callback(*dep);

#if SYS_DEPENDENCY_PROFILE
		if(timer)
		{
			timer->Stop();
		}
#endif // SYS_DEPENDENCY_PROFILE

#if DO_HEAP_ALLOC_FOR_LARGE_REQUESTS
		// deallocate temporary space
		if (heapScratch)
		{
			delete[] heapScratch;
		}
#endif

		// collect next dependency
		if(result)
		{
			dep = NULL;

			// try to steal next a parent
			for(u32 i=0; i < sysDependency::sm_MaxNumParents; i++)
			{
				sysDependency* parent = parents[i];
				if(parent && !sysInterlockedDecrement(&parent->m_NumPending))
				{
					if(!dep && parent->m_Priority >= priority)
					{
						dep = parent;
					}
					else
					{
						InsertInternal(parent);
					}
				}
			}

			if(!dep)
			{
				dep = chain.Pop();
			}
		}
		else
		{
			// reduce the priority if we re-insert the dependency
			u32 priority = dep->m_Priority;
			u32 reducePriority = priority != 0 ? priority-1 : priority;
			chain.Push(reducePriority, dep);
			dep = chain.Pop();
		}
		TELEMETRY_END_ZONE(__FILE__,__LINE__);
	}
}

void sysDependencyScheduler::DependencyLocalThread()
{
	sysDependency* parents[sysDependency::sm_MaxNumParents];
	DependencyContext& context = s_DependencyContext;
	Chain& chain = context.m_Chain;
	u8* stackScratch = Alloca(u8, SYS_DEPENDENCY_WORKERSCRATCH);
	sysDependency* dep = chain.Pop();

	while(dep || chain.GetNumPending() > 0)
	{
		// retrieve dependency
		while(!dep)
		{
			dep = chain.Pop();
			if(dep)
			{
				break;
			}

			if(chain.GetNumPending() == 0)
			{
				return;
			}
		}

		// copy parents before execution
		for(u32 i=0; i < sysDependency::sm_MaxNumParents; i++)
		{
			parents[i] = dep->m_Parents[i];
		}
		u32 priority = dep->m_Priority;

#if DO_HEAP_ALLOC_FOR_LARGE_REQUESTS
		// allocate temporary space if required
		u8* heapScratch = NULL;
#endif
		if(dep->m_Flags & sysDepFlag::ALLOC0)
		{
#if DO_HEAP_ALLOC_FOR_LARGE_REQUESTS
			u32 scratchSize = dep->m_DataSizes[0];
			Assertf(scratchSize <= SYS_DEPENDENCY_WORKERSCRATCH,
				"Scratch size (%u) too large - either increase SYS_DEPENDENCY_WORKERSCRATCH or reduce the scratch size requested by this dependency.  This will crash in non-BANK builds",
				dep->m_DataSizes[0]);
			if (scratchSize <= SYS_DEPENDENCY_WORKERSCRATCH)
			{
				dep->m_Params[0].m_AsPtr = stackScratch;
			}
			else
			{
				heapScratch = rage_aligned_new(16) u8[scratchSize];
				dep->m_Params[0].m_AsPtr = heapScratch;
			}
#else
			FatalAssert(dep->m_DataSizes[0] <= SYS_DEPENDENCY_WORKERSCRATCH)
			dep->m_Params[0].m_AsPtr = stackScratch;
#endif
		}

#if SYS_DEPENDENCY_PROFILE
		// Wrap the markers stating which dependency
		DURANGO_ONLY(PF_PUSH_MARKERCN(0xff00ffff, "Dependency: local"));

		pfDepTimer* timer = NULL;
		if(dep->m_Id)
		{
			timer = context.m_Timers[dep->m_Id];
			timer->Start();
		}
#endif // SYS_DEPENDENCY_PROFILE

		// execute dependency callbacks
		bool result = dep->m_Callback(*dep);

#if SYS_DEPENDENCY_PROFILE
		if(timer)
		{
			timer->Stop();
		}

		DURANGO_ONLY(PF_POP_MARKER());
#endif // SYS_DEPENDENCY_PROFILE

#if DO_HEAP_ALLOC_FOR_LARGE_REQUESTS
		// deallocate temporary space
		if (heapScratch)
		{
			delete[] heapScratch;
		}
#endif

		// collect next dependency
		if(result)
		{
			dep = NULL;

			// try to steal next a parent
			for(u32 i=0; i < sysDependency::sm_MaxNumParents; i++)
			{
				sysDependency* parent = parents[i];
				if(parent && !sysInterlockedDecrement(&parent->m_NumPending))
				{
					if(!dep && parent->m_Priority >= priority)
					{
						dep = parent;
					}
					else
					{
						InsertInternal(parent);
					}
				}
			}

			if(!dep)
			{
				dep = chain.Pop();
			}
		}
		else
		{
			// reduce the priority if we re-insert the dependency
			u32 priority = dep->m_Priority;
			u32 reducePriority = priority != 0 ? priority-1 : priority;
			chain.Push(reducePriority, dep);
			dep = chain.Pop();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

} // namespace rage
