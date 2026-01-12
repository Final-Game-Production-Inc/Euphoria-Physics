//
// physics/shapetestspu.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_SHAPE_TEST_SPU_H
#define PHYSICS_SHAPE_TEST_SPU_H

// Tasks should only depend on taskheader.h, not task.h (which is the dispatching system)
#include "../system/taskheader.h"



DECLARE_TASK_INTERFACE(shapetestspu);

void shapetestspu (::rage::sysTaskParameters& taskParams);



#endif
