#include "grcore/config.h"

#if ENABLE_LCUE
#include "grcore/lcue.h"
#else

// SCE_GNMX_ENABLE_CUE_V2 was called CUE_V2 prior to SDK1.7
#include <sdk_version.h>
#if SCE_ORBIS_SDK_VERSION < 0x01700000u
#ifdef CUE_V2
#defined SCE_GNMX_ENABLE_CUE_V2
#endif
#endif

#include "grcore/gnmx/gnmx.h"
#endif
