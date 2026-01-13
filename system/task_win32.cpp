// 
// system/task_win32.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "task.h"

#include "file/asset.h"

#if __WIN32 || __PSP2 || RSG_ORBIS

#if __WIN32
#include "system/xtl.h"
#endif

#include "atl/array.h"
#include "atl/map.h"
#include "bank/group.h"
#include "math/simplemath.h"
#include "profile/element.h"
#include "system/criticalsection.h"
#include "system/messagequeue.h"
#include "system/param.h"
#include "system/lockfreering.h"
#include "system/ipc.h"
#include "system/timer.h"
#include "system/param.h"
#include "system/pix.h"

#if __PSP2
#include <kernel.h>
#endif

#if RSG_PC && !__RESOURCECOMPILER
NOSTRIP_PC_PARAM(disableHyperthreading, "Don't count hyperthreaded cores as real ones to create tasks on");
PARAM(numCores, "Tasks will only be allocated to the first 'n' cores");
XPARAM(useSystemThreadManagement);
#endif // RSG_PC

namespace rage {

sysTaskParameters::sysTaskParameters() { memset(this,0,sizeof(*this)); }

sysCriticalSectionToken s_TaskToken;

struct __sysTaskHandle
{
	sysTaskHeader Header;
	sysIpcEvent Completed;
};

#if __WIN32
static ULONG_PTR s_ProcessMask = 63;
#elif __PSP2
static u32 s_ProcessMask = 7;
#elif RSG_ORBIS
static u32 s_ProcessMask = 63;		// I think...
#endif

#if HACK_GTA4 && !__PSP2
static const int MaxTasks = 512;
static const int MaxSchedulers = 32;
#else
static const int MaxTasks = 256;
static const int MaxSchedulers = 8;
#endif

#if __WIN32PC
int sysTaskManager::sm_iProcessorPackages = -1;
int sysTaskManager::sm_iProcessorCores = -1;
int sysTaskManager::sm_iLogicalCores = -1;
#endif // __WIN32PC

NOSTRIP_PC_PARAM(maxthreads, "[grcore] Maximum amount of threads to use in game");

typedef u16 TypeFreeListIdx;
CompileTimeAssert(sizeof(TypeFreeListIdx)==sizeof(u16) || MaxTasks < 255);

// EJ: Use a stack for O(1) runtime
static __sysTaskHandle s_Handles[MaxTasks];
static atFixedArray<TypeFreeListIdx, MaxTasks> s_FreeList;

class sysTaskScheduler;

struct sysTaskInfo
{
	sysTaskInfo();
	~sysTaskInfo();
	void Init(sysTaskScheduler &sched,const char* name,int cpu,int stackSize,int scratchSize,sysIpcPriority priority);

	char CpuName[32];
	sysIpcThreadId ThreadId;		// On the main CPU
	sysIpcEvent Completed;
	sysTaskBuffer Scratch;
	sysTaskHeader Header;
	sysTaskScheduler* Sched;

#if __BANK
	bool Disabled;
#endif // __BANK
};


class sysTaskScheduler
{
public:
	sysTaskScheduler(const char* name,int stackSize,int scratchSize,sysIpcPriority priority,u32 coreMask,int schedulerCpu);
	~sysTaskScheduler();
	static DECLARE_THREAD_FUNC(TaskWrapper);
	void AddTask(__sysTaskHandle& h) { m_Queue.Push(&h); };
	void AddTask(__sysTaskHandle* h) { m_Queue.Push(h); };
	void CheckParamCompatibility(const sysTaskParameters& p);

	atArray<sysTaskInfo> &GetThreads()			{ return m_Threads; }

private:
	atArray<sysTaskInfo> m_Threads;
	sysMessageQueue<__sysTaskHandle*,MaxTasks> m_Queue;
};

void sysTaskScheduler::CheckParamCompatibility(const sysTaskParameters& ASSERT_ONLY(p))
{
	for (int i=0; i<m_Threads.GetCount(); i++)
	{
		Assert(p.Scratch.Size <= m_Threads[i].Scratch.Size);
	}
}

sysTaskInfo::sysTaskInfo()
{
	memset(this,0,sizeof(*this));
}

void sysTaskInfo::Init(sysTaskScheduler &sched,const char* name,int cpu,int stackSize,int scratchSize,sysIpcPriority priority)
{
	scratchSize <<= __64BIT;		// double scratch sizes for now since data structures are typically bigger.
	stackSize <<= (__64BIT + (RSG_ORBIS && !__OPTIMIZED));			// stack too, for same reason -- and make it even bigger for unoptimized code, but only on Orbis for now for max safety.
	safecpy(CpuName,name,sizeof(CpuName));
	Scratch.Size = scratchSize;
	Scratch.Data = rage_new char[scratchSize];
	Completed = NULL;
	Sched = &sched;
	ThreadId = sysIpcCreateThread(sysTaskScheduler::TaskWrapper,(void*)this,stackSize,priority,CpuName,cpu);
}

sysTaskInfo::~sysTaskInfo()
{
	delete [] (char*) Scratch.Data;
}

sysTaskScheduler::sysTaskScheduler(const char* name,int stackSize,int scratchSize,sysIpcPriority priority,u32 coreMask,int /*schedulerCpu*/)
{
	int numHardwareThreads = 0;
	int coreMap[32];
	for (int i=0; i<32; i++)
		if (coreMask & (1<<i))
			coreMap[numHardwareThreads++] = i;
			
	m_Threads.Resize(numHardwareThreads);

	for (int i=0; i<numHardwareThreads; i++) {
		char buf[32];
		formatf(buf,sizeof(buf),"%s:%d",name,coreMap[i]);
		m_Threads[i].Init(*this,buf,coreMap[i],stackSize,scratchSize,priority);
	}
}

sysTaskScheduler::~sysTaskScheduler()
{
	// Send down termination signals.
	for (int i=0; i<m_Threads.GetCount(); i++)
		while (!m_Queue.Push(NULL));	// keep trying until the requests make it in
	// Wait for all threads to exit.  Do it here instead of sysTaskInfo dtor because the
	// message queue may have been destroyed first otherwise!
	for (int i=0; i<m_Threads.GetCount(); i++)
		sysIpcWaitThreadExit(m_Threads[i].ThreadId);
}

#if  TASK_USE_CONTINUATION_TASK

 __THREAD	sysTaskHandle s_continuationHandle;
bool sysTaskManager::SetContinuationTask( sysTaskHandle handle)
{
	if ( s_continuationHandle == 0)
	{
		s_continuationHandle= handle;
		return true;
	}
	return false;
}
#endif

DECLARE_THREAD_FUNC(sysTaskScheduler::TaskWrapper)
{
	sysTaskInfo &taskInfo = *(sysTaskInfo*)ptr;

#if __XENON && __DEV
	PIXNameThread(taskInfo.CpuName);
#endif

	while (__sysTaskHandle* nextTask = taskInfo.Sched->m_Queue.Pop())
	{
		DURANGO_ONLY(PF_PUSH_MARKERC(0xff00ff00, taskInfo.CpuName));

#if  TASK_USE_CONTINUATION_TASK
		do 
		{		
			s_continuationHandle=0;
#endif
			taskInfo.Completed = nextTask->Completed;
			sysTaskHeader &hdr = taskInfo.Header;
			hdr = nextTask->Header;
			hdr.Parameters.Scratch.Data = taskInfo.Scratch.Data;
			Assert(hdr.Parameters.Scratch.Size <= taskInfo.Scratch.Size);
			Assert( taskInfo.Header.Func );

			DURANGO_ONLY(PF_PUSH_MARKERC(0xff00ff00, taskInfo.Header.Name));
			TELEMETRY_START_ZONE(PZONE_NORMAL, __FILE__, __LINE__, taskInfo.Header.Name);

			// Call the thread function.
			taskInfo.Header.Func(taskInfo.Header.Parameters);

			TELEMETRY_END_ZONE(__FILE__,__LINE__);

			DURANGO_ONLY(PF_POP_MARKER());

			// Tell the client that their task is done
			sysIpcSetEvent(taskInfo.Completed);

#if  TASK_USE_CONTINUATION_TASK
			nextTask =  s_continuationHandle;
		}
		while ( nextTask);
#endif

			DURANGO_ONLY(PF_POP_MARKER());

#if __BANK
		// If we're disabled just hang around and wait
		volatile bool* disabledPtr = &taskInfo.Disabled;

		while (*disabledPtr)
		{
			sysIpcSleep(150);
		}
#endif // __BANK
	}
}

#if __WIN32PC && !__TOOL
typedef BOOL (WINAPI *LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, 
    PDWORD);

// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
    DWORD LSHIFT = sizeof(ULONG_PTR)*8 - 1;
    DWORD bitSetCount = 0;
    ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;    
    DWORD i;
    
    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest)?1:0);
        bitTest/=2;
    }

    return bitSetCount;
}

int sysTaskManager::GetProcessorPackages()
{
	Assert(sm_iProcessorPackages > 0);
	return sm_iProcessorPackages;
}

int sysTaskManager::GetProcessorCores()
{
	Assert(sm_iProcessorCores > 0);
	return sm_iProcessorCores;
}

int sysTaskManager::GetLogicalCores()
{
	Assert(sm_iLogicalCores > 0);
	return sm_iLogicalCores;
}

bool sysTaskManager::GetStandardProcessorInfo(int &iProcessorPackages, int &iProcessorCores, int &iLogicalCores)
{
	ULONG_PTR systemMask=0;
	GetProcessAffinityMask(GetCurrentProcess(),&s_ProcessMask,&systemMask);
	iProcessorCores = iLogicalCores = CountOnBits((u32)s_ProcessMask);
	iProcessorPackages = 1;
	return true;
}

bool sysTaskManager::GetProcessorInfo(int &iProcessorPackages, int &iProcessorCores, int &iLogicalCores)
{
	GetStandardProcessorInfo( iProcessorPackages, iProcessorCores, iLogicalCores);

	LPFN_GLPI glpi;
	BOOL done = FALSE;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	DWORD returnLength = 0;
	DWORD logicalProcessorCount = 0;
	DWORD numaNodeCount = 0;
	DWORD processorCoreCount = 0;
	DWORD processorL1CacheCount = 0;
	DWORD processorL2CacheCount = 0;
	DWORD processorL3CacheCount = 0;
	DWORD processorPackageCount = 0;
	DWORD byteOffset = 0;
	PCACHE_DESCRIPTOR Cache;

	glpi = (LPFN_GLPI) GetProcAddress(
							GetModuleHandle(TEXT("kernel32")),
							"GetLogicalProcessorInformation");
	if (NULL == glpi) 
	{
		Warningf("\nGetLogicalProcessorInformation is not supported.\n");
		return false;
	}

	while (!done)
	{
		DWORD rc = glpi(buffer, &returnLength);

		if (FALSE == rc) 
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
			{
				if (buffer) 
					delete [] buffer;

				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)rage_new char[returnLength];

				if (NULL == buffer) 
				{
					Errorf("\nError: Allocation failure\n");
					return (false);
				}
			} 
			else 
			{
				Errorf("\nError %d\n", GetLastError());
				return (false);
			}
		} 
		else
		{
			done = TRUE;
		}
	}

	ptr = buffer;

	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) 
	{
		switch (ptr->Relationship) 
		{
		case RelationNumaNode:
			// Non-NUMA systems report a single record of this type.
			numaNodeCount++;
			break;

		case RelationProcessorCore:
			processorCoreCount++;

			// A hyperthreaded core supplies more than one logical processor.
			logicalProcessorCount += CountSetBits(ptr->ProcessorMask);
			break;

		case RelationCache:
			// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
			Cache = &ptr->Cache;
			if (Cache->Level == 1)
			{
				processorL1CacheCount++;
			}
			else if (Cache->Level == 2)
			{
				processorL2CacheCount++;
			}
			else if (Cache->Level == 3)
			{
				processorL3CacheCount++;
			}
			break;

		case RelationProcessorPackage:
			// Logical processors share a physical package.
			processorPackageCount++;
			break;

		default:
			Errorf("\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n");
			break;
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}
	delete []buffer;

	iProcessorPackages = processorPackageCount;
	iProcessorCores = processorCoreCount;
	iLogicalCores = logicalProcessorCount;

#if RSG_PC && !__RESOURCECOMPILER
	int numCores=0;
	if (PARAM_numCores.Get(numCores))
		iProcessorCores = iLogicalCores = numCores;
#endif

	return true;
}
#endif // __WIN32PC

void sysTaskManager::InitClass() 
{
#if __ASSERT
	static bool inited;
	Assert(!inited);
	inited = true;
#endif

#if __WIN32PC 
#if __TOOL
	ULONG_PTR systemMask=0;
	GetProcessAffinityMask(GetCurrentProcess(),&s_ProcessMask,&systemMask);
#else
	AssertVerify(GetProcessorInfo(sm_iProcessorPackages, sm_iProcessorCores, sm_iLogicalCores));
 #if RSG_PC && !__RESOURCECOMPILER
	if (!PARAM_disableHyperthreading.Get())
 #endif
		sm_iProcessorCores = sm_iLogicalCores; // Allow Hyperthreads
#endif // __TOOL

#if RSG_PC && !__RESOURCECOMPILER && !__TOOL && !__FINAL
	if (PARAM_numCores.Get())
	{
		int numCores=1;
		PARAM_numCores.Get(numCores);
		s_ProcessMask = ((u64)1 << (u64)numCores) - 1;
	}
	/*
	else if (getenv("XoreaxBuildContext"))
	{
		int numCores = CountOnBits((u32)s_ProcessMask);
		if (numCores < 4)
			numCores = 1;
		else
			numCores >>= 2;
		s_ProcessMask = (1 << numCores) - 1;
		PARAM_useSystemThreadManagement.Set("");
		// Displayf("XGE detected, limiting number of cores to %d (%x)",numCores,(u32)s_ProcessMask);
	}
	*/
#endif // RSG_PC

	PARAM_maxthreads.Get(sm_iProcessorCores);

#endif // __WIN32PC


	for (int i=0; i<MaxTasks; i++)
	{
		TypeFreeListIdx id = static_cast<TypeFreeListIdx>(i);
		s_FreeList.Push(id);
		s_Handles[i].Completed = sysIpcCreateEvent();
	}
}


static atFixedArray<sysTaskScheduler*, MaxSchedulers> s_Schedulers;

int sysTaskManager::AddScheduler(const char* name,int stackSize,int scratchSize,sysIpcPriority priority,int coreMask,int schedulerCpu)
{
	int result = s_Schedulers.GetCount();
	AssertMsg(coreMask & s_ProcessMask,"sysTaskManager::AddScheduler - No cores available in specified mask");
	s_Schedulers.Append() = rage_new sysTaskScheduler(name,stackSize,scratchSize,priority,coreMask & s_ProcessMask,schedulerCpu);
	return result;
}



void sysTaskManager::ShutdownClass()
{
	for (int i=0; i<MaxTasks; i++)
	{
		if (s_Handles[i].Completed)
		{
			sysIpcDeleteEvent(s_Handles[i].Completed);
			s_Handles[i].Completed = 0;
		}
	}

	while (s_Schedulers.GetCount())
		delete s_Schedulers.Pop();

	s_FreeList.Reset();
}

__sysTaskHandle* sysTaskManager::AllocateTaskHandle()
{
	sysCriticalSection cs(s_TaskToken);

	if (s_FreeList.IsEmpty())
	{
#if __ASSERT
		PrintJobs();
		AssertMsg(false, "Out of jobs.  Calling code might not handle this well.");
#endif
		return NULL;
	}

	const int index = s_FreeList.Pop();
	return &s_Handles[index];
}

void sysTaskManager::FreeTaskHandle(sysTaskHandle task)
{
	Assertf(task, "NULL task handle!");

	if (task)
	{		
		const int index = static_cast<int>(task - s_Handles);
		TypeFreeListIdx id = static_cast<TypeFreeListIdx>(index);

		sysCriticalSection cs(s_TaskToken);
		task->Header.Func = NULL;
		s_FreeList.Push(id);
	}	
}

sysTaskHandle sysTaskManager::Setup(TASK_INTERFACE_PARAMS,const sysTaskParameters &p,int ASSERT_ONLY(schedulerIndex))
{
	sysCriticalSection cs(s_TaskToken);

	if (!s_Schedulers.GetCount())
	{
		Warningf("sysTaskManager::Create - no schedulers created yet, adding a default scheduler.");
		AddScheduler("defSched",65535,32768,PRIO_LOWEST,(int) s_ProcessMask,0);
	}

#if __ASSERT
	s_Schedulers[schedulerIndex]->CheckParamCompatibility(p);
#endif // __ASSERT

	// EJ: Use a stack for O(1) runtime
	__sysTaskHandle* handle = AllocateTaskHandle();
	Assertf(handle, "Unable to allocate memory for job task handle!");

	if (handle)
	{
		handle->Header.Name = taskName;
		handle->Header.Func = taskFunc;
		handle->Header.Parameters = p;
	}
	else
	{
		Errorf("Unable to allocate memory for job task handle!");
	}

	return handle;
}

sysTaskHandle sysTaskManager::Create(TASK_INTERFACE_PARAMS,const sysTaskParameters &p,int schedulerIndex)
{
	sysCriticalSection cs(s_TaskToken);

	sysTaskHandle handle = Setup(TASK_INTERFACE_PARAMS_PASS,p,schedulerIndex );
	if (handle)
	{
		Assert(handle->Header.Func != NULL);
		s_Schedulers[schedulerIndex]->AddTask(handle);
	}

	return handle;
}

sysPreparedTaskHandle sysTaskManager::Prepare(TASK_INTERFACE_PARAMS,const sysTaskParameters &p,int schedulerIndex)
{
	sysCriticalSection cs(s_TaskToken);

	sysPreparedTaskHandle pHdle;
	pHdle.hdle = Setup(TASK_INTERFACE_PARAMS_PASS,p,schedulerIndex);
	return pHdle;
}

sysTaskHandle sysTaskManager::Dispatch(const sysPreparedTaskHandle& handle, int schedulerIndex)
{	
	if (handle.hdle)
	{
		sysCriticalSection cs(s_TaskToken);
		s_Schedulers[schedulerIndex]->AddTask(*handle.hdle);		
	}
	
	return handle.hdle;
}

void sysTaskManager::DispatchLocally(const sysPreparedTaskHandle& prepHdle)
{
	const int ScratchSize=32*1024;
	char scratchBuffer[ScratchSize];
	sysTaskBuffer Scratch;
	Scratch.Size = ScratchSize;
	Scratch.Data = scratchBuffer;

	prepHdle.hdle->Header.Parameters.Scratch=Scratch;
	prepHdle.hdle->Header.Func(prepHdle.hdle->Header.Parameters);
	sysIpcSetEvent(prepHdle.hdle->Completed);
}

bool sysTaskManager::Poll(sysTaskHandle task)
{
	Assert(task);
	if (sysIpcPollEvent(task->Completed))
	{
		FreeTaskHandle(task);
		return true;
	}
	else
		return false;
}

__THREAD s64 g_TaskWaitTicks;
__THREAD u32 g_TaskTimeout;

void sysTaskManager::WaitMultiple(int taskCount,const sysTaskHandle* tasks)
{
	for (int i=0; i<taskCount; i++)
		if (tasks[i])
			Wait(tasks[i]);
}


void sysTaskManager::Wait(sysTaskHandle task)
{
	Assert(task);
	sysIpcWaitEvent(task->Completed);
	FreeTaskHandle(task);
}


int sysTaskManager::GetMaxTasks()
{
	return MaxTasks;
}

int sysTaskManager::GetNumThreads()
{
#if __WIN32PC
	#if __TOOL
		return CountOnBits((u32)s_ProcessMask);
	#else
		return GetProcessorCores(); 
	#endif // __TOOL
#elif __PSP2
	return 3;
#elif __XENON || RSG_DURANGO
	return 6;
#elif RSG_ORBIS
	return 6;
#else
	AssertMsg(0, "Not Implemented - sysTaskManager::GetNumThreads()")
#endif
}

#if !__FINAL
const char* sysTaskManager::GetTaskName(sysTaskHandle handle)
{
	__sysTaskHandle* h = handle;
	return h->Header.Name;
}
#endif

#if __BANK
void sysTaskManager::PrintJobs()
{
	Displayf("***********************************************************************");
	Displayf("Job Information");
	Displayf("***********************************************************************");

	sysCriticalSection cs(s_TaskToken);

	atMap<const char*, u32> jobs;
	for(int i = s_FreeList.GetCount(); i < s_FreeList.GetMaxCount(); ++i)
	{
		const char* name = sysTaskManager::GetTaskName(&s_Handles[i]);
		++jobs[name];
	}

	Displayf("Total: %d, Used: %d, Free: %d", s_FreeList.GetMaxCount(), s_FreeList.GetAvailable(), s_FreeList.GetCount());
	for(atMap<const char*, u32>::Iterator it = jobs.CreateIterator(); it; ++it)
		Displayf(" %s - %i pending jobs", it.GetKey(), it.GetData());

	Displayf("***********************************************************************");
}

void sysTaskManager::AddWidgets(bkGroup &bank)
{
	bkGroup* masterGroup = bank.AddGroup("Task Manager");

	masterGroup->AddButton("Print Jobs", &sysTaskManager::PrintJobs);

	for (int i=0; i<s_Schedulers.GetCount(); i++) {
		sysTaskScheduler* scheduler = s_Schedulers[i];

		if (scheduler) {
			char title[16];
			formatf(title, "Scheduler %d", i);
			bkGroup* schedulerGroup = masterGroup->AddGroup(title);

			atArray<sysTaskInfo> &threads = scheduler->GetThreads();

			for (int x=0; x<threads.GetCount(); x++) {
				sysTaskInfo &taskInfo = threads[x];

				char title[32];
				formatf(title, "Disable %s", taskInfo.CpuName);

				schedulerGroup->AddToggle(title, &taskInfo.Disabled);
			}
		}
	}

}
#endif // __BANK


}	// namespace rage

#endif	// __WIN32
