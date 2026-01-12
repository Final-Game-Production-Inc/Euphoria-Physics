// 
// physics/contacttask.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_CONTACTTASK_H 
#define PHYSICS_CONTACTTASK_H 

// Tasks should only depend on taskheader.h, not task.h (which is the dispatching system)
#include "../system/taskheader.h"

#if __PS3
#include <cell/spurs/task.h>
class CellSpursQueue;
#endif

namespace rage {

struct phTaskCollisionPair;

DECLARE_TASK_INTERFACE(UpdateContactsTask);
void UpdateContactsTask( ::rage::sysTaskParameters & );

} // namespace rage

#endif // PHYSICS_CONTACTTASK_H 
