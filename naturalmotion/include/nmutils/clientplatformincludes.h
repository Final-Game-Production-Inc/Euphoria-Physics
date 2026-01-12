
#ifndef NM_CLIENTPLATFORM_INCLUDES_H
#define NM_CLIENTPLATFORM_INCLUDES_H


#ifdef RAGE_RELEASE
#if HACK_GTA4 // Please don't include xtl.h in global headers
	#include "system/memops.h"
#else
#include "system/xtl.h"
#endif
#else // RAGE_RELEASE
#ifdef WIN32
# include <windows.h>
#endif
#ifdef NM_PLATFORM_X360
# include "system/xtl.h"
#endif
#endif // RAGE_RELEASE

#if __BANK   
#define ART_ENABLE_BSPY 0
#else
#define ART_ENABLE_BSPY 0
#endif

//So that the gameLibs can be compiled with ART_ENABLE_BSPY 1 
// and still link with NM code if ART_ENABLE_BSPY 0 (e.g. for profiling)
// Bspy functions called by the gamelibs are defined here but do nothing
#define NM_EMPTY_BSPYSERVER 0//NB: This should always be checked in as 0


#endif // NM_CLIENTPLATFORM_INCLUDES_H


