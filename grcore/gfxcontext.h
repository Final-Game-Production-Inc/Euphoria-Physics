//
// grcore/gfxcontext.h
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_GFXCONTEXT_H
#define GRCORE_GFXCONTEXT_H

#include "allocscope.h"

#if RSG_DURANGO
#	include "gfxcontext_durango.h"
#elif RSG_ORBIS
#	include "gfxcontext_gnm.h"
#endif

#endif // GRCORE_GFXCONTEXT_H
