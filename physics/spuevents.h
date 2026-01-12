//
// physics/spuevents.h
//
// Copyright (C) 2012-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_SPUEVENTS_H
#define PHYSICS_SPUEVENTS_H

#if __PS3

#include "rmcore/spuevents.h"

namespace rage
{

enum
{
	EVENTCMD_ASSERT_CONTACT_NORM = NUM_RMCORE_EVENTCMDS,
	NUM_PHYSICS_EVENTCMDS
};

}
// namespace rage

#endif // __PS3

#endif // PHYSICS_SPUEVENTS_H
