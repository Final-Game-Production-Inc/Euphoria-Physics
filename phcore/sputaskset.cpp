//
// phcore/sputaskset.cpp
//
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved.
//

#if __PS3

#include "sputaskset.h"

#include <cell/spurs/queue.h>
#include <cell/spurs/task.h>
#include <stdlib.h>

#define CELL_CHECK(x)	{ int result = (x); if (result) Quitf("%s failed, code %x",#x,result); }

namespace rage {

extern CellSpurs* g_Spurs;

CellSpursTaskset* phSpuTaskSet::sm_SpursTaskSet;
CellSpursQueue* phSpuTaskSet::sm_SpursCompletionQueue;
void* phSpuTaskSet::sm_SpursCompletionQueueBuffer;

void phSpuTaskSet::InitClass()
{
	Assert(sm_SpursTaskSet == NULL);
	Assert(sm_SpursCompletionQueue == NULL);
	Assert(sm_SpursCompletionQueueBuffer == NULL);

	sm_SpursTaskSet = (CellSpursTaskset*)memalign(CELL_SPURS_TASKSET_ALIGN, CELL_SPURS_TASKSET_CLASS1_SIZE);

	uint8_t prios[8] = {1, 1, 1, 1, 1, 1, 1, 1};
	CellSpursTasksetAttribute attributeTaskset;
	CELL_CHECK(cellSpursTasksetAttributeInitialize(&attributeTaskset, (uint64_t)0, prios, 6));
	static const char* name = "Physics";
	CELL_CHECK(cellSpursTasksetAttributeSetName(&attributeTaskset, name));
	CELL_CHECK(cellSpursTasksetAttributeSetTasksetSize(&attributeTaskset, CELL_SPURS_TASKSET_CLASS1_SIZE));

#if __ASSERT // Only enable LS clear on debug and beta builds
	CELL_CHECK(cellSpursTasksetAttributeEnableClearLS(&attributeTaskset, 1));
#endif

	CELL_CHECK(cellSpursCreateTasksetWithAttribute(g_Spurs, sm_SpursTaskSet, &attributeTaskset));

#define QUEUE_SIZE 128

	sm_SpursCompletionQueue = (CellSpursQueue*)memalign(CELL_SPURS_QUEUE_ALIGN, CELL_SPURS_QUEUE_SIZE);
	sm_SpursCompletionQueueBuffer = memalign(128, sizeof(CellSpursTaskArgument) * QUEUE_SIZE);
	CELL_CHECK(cellSpursQueueInitialize(sm_SpursTaskSet,
		sm_SpursCompletionQueue, sm_SpursCompletionQueueBuffer, sizeof(CellSpursTaskArgument),
		QUEUE_SIZE, CELL_SPURS_QUEUE_SPU2PPU));

	CELL_CHECK(cellSpursQueueAttachLv2EventQueue(sm_SpursCompletionQueue));
}

void phSpuTaskSet::ShutdownClass()
{
	CELL_CHECK(cellSpursQueueDetachLv2EventQueue(sm_SpursCompletionQueue));
	CELL_CHECK(cellSpursShutdownTaskset(sm_SpursTaskSet));
	CELL_CHECK(cellSpursJoinTaskset(sm_SpursTaskSet));

	delete sm_SpursCompletionQueueBuffer;
	sm_SpursCompletionQueueBuffer = NULL;
	delete sm_SpursCompletionQueue;
	sm_SpursCompletionQueue = NULL;
	delete sm_SpursTaskSet;
	sm_SpursTaskSet = NULL;
}

} // namespace rage

#endif // __PS3
