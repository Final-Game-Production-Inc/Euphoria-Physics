//
// phcore/workerthread.h
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_WORKERTHREAD_H
#define PHCORE_WORKERTHREAD_H

#if __PS3
#include <cell/spurs/task_types.h>

class CellSpursQueue;
#endif

namespace rage {

struct sysTaskParameters;

#if __PS3
union phWorkerThreadSpuParams
{
	struct {
		CellSpursQueue* completionQueue;
		const sysTaskParameters* taskParams;
		u32 spuTrace;
		u32 traceId;
	};

#if __PPU
	CellSpursTaskArgument spursArgument;
#elif __SPU
	vec_uint4 uiQWord;
#endif
} __attribute__((aligned(16)));
#else // __PS3
struct __sysTaskHandle;
typedef struct __sysTaskHandle *sysTaskHandle;
#endif // __PS3

class phWorkerThread
{
public:
#if __PS3
	phWorkerThread(const char* name, void* elf, bool context = false, u32 stackSize = 0);
#else
	typedef void (*ThreadFunc)(sysTaskParameters&);
	phWorkerThread(const char* name, ThreadFunc func, int scheduler);
#endif

	~phWorkerThread();

	void Initiate(const sysTaskParameters& taskParams, int numWorkerThreads);
	void Initiate(sysTaskParameters* const* taskParams, int numWorkerThreads);
	void Wait();

private:
	const char* m_Name;
	int m_NumActive;

#if __PS3
	void* m_Elf;
	u32 m_ContextSize;
	CellSpursTaskLsPattern m_ContextPattern;
	char** m_Contexts;
#else
	sysTaskHandle* m_TaskHandles;
	ThreadFunc m_ThreadFunc;
	int m_Scheduler;
#endif // __PS3
};

} // namespace rage


#endif // PHCORE_WORKERTHREAD_H
