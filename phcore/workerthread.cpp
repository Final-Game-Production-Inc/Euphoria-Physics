//
// phcore/workerthread.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#include "workerthread.h"

#include "config.h"
#include "sputaskset.h"

#include "profile/cellspurstrace.h"
#include "system/criticalsection.h"
#include "system/new.h"
#include "system/taskheader.h"

#if __PS3
#include <cell/spurs/task.h>
#else
#include "system/task.h"
#endif // __PS3

namespace rage {

#if __PS3
phWorkerThread::phWorkerThread(const char* name, void* elf, bool context, u32 stackSize)
	: m_Name(name)
	, m_NumActive(0)
	, m_Elf(elf)
	, m_Contexts(NULL)
	, m_ContextSize(0)
{
	if (context)
	{
		int numWorkerThreads = phConfig::GetMaxNumWorkerThreads();

		// See "Reduce context switching cost" in "libspurs Overview" for explanation
		CellSpursTaskLsPattern loadablePattern; 
		cellSpursTaskGetLoadableSegmentPattern(&loadablePattern, elf);

		CellSpursTaskLsPattern roPattern; 
		cellSpursTaskGetReadOnlyAreaPattern(&roPattern, elf);

		CellSpursTaskLsPattern stackPattern; 
		cellSpursTaskGenerateLsPattern(&stackPattern,
			256*1024 - CELL_SPURS_TASK_CEIL_2048(stackSize),
			CELL_SPURS_TASK_CEIL_2048(stackSize));

		m_ContextPattern.u64[0] = (loadablePattern.u64[0] & ~roPattern.u64[0]) | stackPattern.u64[0];
		m_ContextPattern.u64[1] = (loadablePattern.u64[1] & ~roPattern.u64[1]) | stackPattern.u64[1];

		cellSpursTaskGetContextSaveAreaSize(&m_ContextSize, &m_ContextPattern);

		m_Contexts = rage_new char*[numWorkerThreads];
		for (int thread = 0; thread < numWorkerThreads; ++thread)
		{
			m_Contexts[thread] = rage_aligned_new(128) char[m_ContextSize];
		}
	}
}
#else // __PS3
phWorkerThread::phWorkerThread(const char* name, ThreadFunc func, int scheduler)
	: m_Name(name)
	, m_NumActive(0)
	, m_TaskHandles(rage_new sysTaskHandle[phConfig::GetMaxNumWorkerThreads()])
	, m_ThreadFunc(func)
	, m_Scheduler(scheduler)
{
}
#endif // __PS3

phWorkerThread::~phWorkerThread()
{
#if __PS3
	if (m_Contexts)
	{
		int numWorkerThreads = phConfig::GetMaxNumWorkerThreads();
		for (int thread = 0; thread < numWorkerThreads; ++thread)
		{
			delete m_Contexts[thread];
		}
		delete [] m_Contexts;
		m_Contexts = NULL;
	}
#else
	delete [] m_TaskHandles;
#endif
}

#if RAGETRACE && __PS3
extern uint32_t (*g_pIdFromName)(const char*);
#endif // RAGETRACE && __PS3

void phWorkerThread::Initiate(const sysTaskParameters& taskParams, int numWorkerThreads)
{
	m_NumActive = numWorkerThreads;

#if __PS3
	Assertf((u32(&taskParams) & 15) == 0, "Task params must be 16 byte aligned");

	phWorkerThreadSpuParams params;
	params.completionQueue = phSpuTaskSet::GetSpursCompletionQueue();
	params.taskParams = &taskParams;
#if RAGETRACE
	params.spuTrace = (u32)g_pfSpuTrace;
	params.traceId = g_pIdFromName(m_Name);
#endif // RAGETRACE	

	const CellSpursTaskLsPattern* contextPattern = m_ContextSize ? &m_ContextPattern : 0;

	CellSpursTaskId taskId;
	for (int taskIndex = 0; taskIndex < numWorkerThreads; ++taskIndex)
	{
		cellSpursCreateTask(phSpuTaskSet::GetSpursTaskSet() /*taskset*/,
			&taskId /*id*/,
			m_Elf /*eaElf*/,
			m_ContextSize ? m_Contexts[taskIndex] : 0 /*eaContext*/,
			m_ContextSize /*sizeContext*/,
			contextPattern /*pattern*/,
			&params.spursArgument /*argTask*/);
	}
#else // __PS3
	for (int taskIndex = 0; taskIndex < numWorkerThreads; ++taskIndex)
	{
		m_TaskHandles[taskIndex] = sysTaskManager::Create(m_Name, m_ThreadFunc, taskParams, m_Scheduler);
	}
#endif // __PS3
}

#if !__PS3
extern sysCriticalSectionToken s_TaskToken;
#endif // __PS3

void phWorkerThread::Initiate(sysTaskParameters* const* taskParams, int numWorkerThreads)
{
	m_NumActive = numWorkerThreads;

#if __PS3
	phWorkerThreadSpuParams params;
	params.completionQueue = phSpuTaskSet::GetSpursCompletionQueue();
#if RAGETRACE
	params.spuTrace = (u32)g_pfSpuTrace;
	params.traceId = g_pIdFromName(m_Name);
#endif // RAGETRACE	

	const CellSpursTaskLsPattern* contextPattern = m_ContextSize ? &m_ContextPattern : 0;

	CellSpursTaskId taskId;
	for (int taskIndex = 0; taskIndex < numWorkerThreads; ++taskIndex)
	{
		Assertf((u32(taskParams[taskIndex]) & 15) == 0, "Task params must be 16 byte aligned");
		params.taskParams = taskParams[taskIndex];
		cellSpursCreateTask(phSpuTaskSet::GetSpursTaskSet() /*taskset*/,
			&taskId /*id*/,
			m_Elf /*eaElf*/,
			m_ContextSize ? m_Contexts[taskIndex] : 0 /*eaContext*/,
			m_ContextSize /*sizeContext*/,
			contextPattern /*pattern*/,
			&params.spursArgument /*argTask*/);
	}
#else // __PS3
	s_TaskToken.Lock();

	for (int taskIndex = 0; taskIndex < numWorkerThreads; ++taskIndex)
	{
		m_TaskHandles[taskIndex] = sysTaskManager::Create(m_Name, m_ThreadFunc, *taskParams[taskIndex], m_Scheduler);
	}

	s_TaskToken.Unlock();
#endif // __PS3
}

void phWorkerThread::Wait()
{
#if __PS3
	for (int taskIndex = 0; taskIndex < m_NumActive; ++taskIndex)
	{
		sysTaskParameters params ;
		cellSpursQueuePop(phSpuTaskSet::GetSpursCompletionQueue(), &params);
	}
#else // __PS3
	if (m_NumActive > 0)
	{
		sysTaskManager::WaitMultiple(m_NumActive, m_TaskHandles);
	}
#endif
}

} // namespace rage

