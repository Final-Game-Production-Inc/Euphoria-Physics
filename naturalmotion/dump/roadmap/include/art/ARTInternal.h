#ifndef NM_ART_INTERNAL_H
#define NM_ART_INTERNAL_H

#include "nmutils/NMTypes.h"

#if defined(NM_PLATFORM_CELL_PPU) || defined(NM_PLATFORM_WII)
# include <stdio.h>
# include <string.h> // for memcpy
# define _snprintf snprintf
#endif // NM_PLATFORM_CELL_PPU || PLATFORM_WII


#include "art/ARTBaseDefs.h"
#include "nmutils/MemoryStream.h"

#include "art/ARTMemory.h"
#include "art/ART.h"

#endif // NM_ART_INTERNAL_H
