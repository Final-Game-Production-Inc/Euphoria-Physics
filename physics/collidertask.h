// 
// physics/collidertask.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_COLLIDERTASK_H 
#define PHYSICS_COLLIDERTASK_H 

// Tasks should only depend on taskheader.h, not task.h (which is the dispatching system)
#include "../system/taskheader.h"

#include "phcore/constants.h" // for EARLY_FORCE_SOLVE

#if __PS3
#include <cell/spurs/task.h>
class CellSpursQueue;
#endif

namespace rage {

enum
{
#if EARLY_FORCE_SOLVE
	PHCOLLIDER_INTEGRATE_VELOCITIES = 0
	, PHCOLLIDER_INTEGRATE_POSITIONS
	, PHCOLLIDER_APPLY_PUSHES
#else // EARLY_FORCE_SOLVE
	PHCOLLIDER_UPDATE = 0
	, PHCOLLIDER_UPDATE_POST
#endif // EARLY_FORCE_SOLVE
};

DECLARE_TASK_INTERFACE(UpdateCollidersTask);
void UpdateCollidersTask( ::rage::sysTaskParameters & );

} // namespace rage

#endif // PHYSICS_COLLIDERTASK_H 
