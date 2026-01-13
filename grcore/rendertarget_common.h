//
// grcore/rendertarget_common.h
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_RENDERTARGET_COMMON_H
#define GRCORE_RENDERTARGET_COMMON_H


#include "grcore/config.h"
#include "grcore/device.h"
#include "grcore/effect_mrt_config.h"


// Note that the tracking of MSAA/EQAA resolves won't worth with
// multi-threaded-rendering (short of some nasty GPU command buffer hacks),
// since a render target may be resolved in one subrender thread, then used in
// another.  From the GPU's point of view, the order may be correct, but the
// order the subrender threads execute on the CPU may not be the same.
#define DEBUG_TRACK_MSAA_RESOLVES       (1 && __ASSERT && NUMBER_OF_RENDER_THREADS==1 && DEVICE_MSAA)


#endif // GRCORE_RENDERTARGET_COMMON_H
