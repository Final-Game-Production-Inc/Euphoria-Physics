#ifndef SYSTEM_REPLAY_H
#define SYSTEM_REPLAY_H

#if __PPU

#include "grcore/wrapper_gcm.h"

#if GCM_REPLAY
extern bool g_grcIsCurrentlyCapturing;
#define IS_CAPTURING_REPLAY_DECL
#define IS_CAPTURING_REPLAY g_grcIsCurrentlyCapturing
#else
#define IS_CAPTURING_REPLAY_DECL
#define IS_CAPTURING_REPLAY			false
#endif

#endif // __PPU

#endif // SYSTEM_REPLAY_H
