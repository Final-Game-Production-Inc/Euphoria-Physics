// 
// system/taskserver_spu.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "taskserver.h"

#include <spu_intrinsics.h>
#include "system/dma.h"

namespace rage {

u32 sysTaskGetNextTask(sysTaskInfo &taskInfo,sysTaskHeader &hdr)
{
	// Let PU know we're ready (the interrupt will wake up the PU interrupt thread)
	// We pass along our taskInfo address so that PU can fill it before it wakes us up
	spu_write_interrupt(0x100);

	Displayf("wait for PU");
	u32 taskAddr = spu_wait_inbox();
	Displayf("task addr at %x",taskAddr);
	sysDmaGet(&taskInfo,(u64)taskAddr,sizeof(taskInfo),0);
	sysDmaWaitTagStatusAll(1<<0);


	static char staticInputBuffer[16384] ALIGNED(128), staticOutputBuffer[16834] ALIGNED(128);

	Displayf("i am [%s] command %x",taskInfo.CpuName,taskInfo.Command);
	hdr.Command = taskInfo.Command;
	Assert(taskInfo.PuInputBufferSize < sizeof(staticInputBuffer));
	Assert(taskInfo.PuOutputBufferSize < sizeof(staticOutputBuffer));
	hdr.InputBuffer = staticInputBuffer;
	hdr.InputBufferSize = taskInfo.PuInputBufferSize;
	hdr.OutputBuffer = staticOutputBuffer;
	hdr.OutputBufferSize = taskInfo.PuOutputBufferSize;
	sysDmaGet((void*)hdr.InputBuffer,taskInfo.PuInputBuffer,hdr.InputBufferSize,0);
	sysDmaWaitTagStatusAll(1<<0);

	return hdr.Command;
}

void sysTaskCompleteTask(sysTaskInfo &taskInfo,sysTaskHeader &hdr)
{
	if (hdr.OutputBufferSize) {
		sysDmaPut(hdr.OutputBuffer,taskInfo.PuOutputBuffer,hdr.OutputBufferSize,0);
		sysDmaWaitTagStatusAll(1<<0);
	}

	// Let PU-side scheduler know the task is complete.
	spu_write_interrupt(0);
}

}	// namespace rage
