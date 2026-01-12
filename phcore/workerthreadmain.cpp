//
// phcore/workerthreadmain.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#if __SPU

#include "workerthread.h"

#include "profile/cellspurstrace.h"
#include "system/dma.h"
#include "system/taskheader.h"

#include <cell/spurs/queue.h>

namespace rage {

void phWorkerThreadMain(sysTaskParameters& params);

RAGETRACE_ONLY(pfSpuTrace* g_pfSpuTrace = 0;)

} // namespace rage

using namespace rage;

int cellSpursTaskMain(qword argTaskQword, uint64_t argTaskset)
{
	//Need to set all memory on the spu to be writable.
	//If we don't do this then we'll get an error in sysValidatePointer when we do a dma.
	//This isn't quite a proper fix because we really want only to set inputs/outputs memory
	//and scratchpad to be writable.  Ideally, we'd implement something like in cellSpursJobMain2.
	sysLocalStoreSetWritable(0,(256*1024));

	phWorkerThreadSpuParams params = { uiQWord : (vec_uint4)argTaskQword };

#if RAGETRACE
	pfSpuTrace trace(params.spuTrace);
	if (params.spuTrace)
	{
		g_pfSpuTrace = &trace;
		trace.Push((pfTraceCounterId*)params.traceId);
	}
#endif

	const int tag = 1;
	sysTaskParameters taskParams ;
	sysDmaGetAndWait(&taskParams, uint64_t(params.taskParams), (sizeof(sysTaskParameters) + 15) & ~15, tag);

	phWorkerThreadMain(taskParams);

	// Push one element onto the queue indicating that we're done
	int result;
	do
	{
		result=cellSpursQueueTryPushBegin((uint64_t)params.completionQueue, &params, tag);
	}
	while (result == CELL_SPURS_TASK_ERROR_AGAIN || result == CELL_SPURS_TASK_ERROR_BUSY);

	Assert((result == CELL_OK) && "Error writing to SPURS queue.");

	cellSpursQueuePushEnd((uint64_t)params.completionQueue, tag);

#if RAGETRACE
	if (params.spuTrace)
	{
		trace.Pop();
		trace.Finish(tag);
	}
#endif

	return 0;
}

#endif // __SPU
