// 
// physics/forcesolvertask.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef FORCE_SOLVER_TASK_H
#define FORCE_SOLVER_TASK_H

// Tasks should only depend on taskheader.h, not task.h (which is the dispatching system)
#include "../system/taskheader.h"

DECLARE_TASK_INTERFACE(forcesolver);
void forcesolver( ::rage::sysTaskParameters & );

#endif // FORCE_SOLVER_TASK_H
