// 
// system/main_spu.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "main_spu.h"

#include "taskserver.h"

extern void TaskMain(rage::sysTaskInfo &taskInfo);

int main(void)
{
	// Declare our task info object
	rage::sysTaskInfo taskInfo ;

	rage::Displayf("spu - HELLO WORLD!");

	// Invoke the task server loop
	TaskMain(taskInfo);

	return 0;
}
