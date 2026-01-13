//
// grprofile/pix_common.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
//

#include "pix.h"

#include "system/pix.h"

using namespace rage;

void PIXInit() {
#if ENABLE_PIX_TAGGING
	// Sets up all the callback functions so that the macros from system\pix.h actually do something
	pfPixMarkerFuncs::g_PixBeginCb = PIXBegin;
	pfPixMarkerFuncs::g_PixBeginCCb = PIXBeginC;
	pfPixMarkerFuncs::g_PixBeginNCb = PIXBeginN;
	pfPixMarkerFuncs::g_PixBeginCNCb = PIXBeginCN;
	pfPixMarkerFuncs::g_PixEndCb = PIXEnd;
	pfPixMarkerFuncs::g_PixEndECb = PIXEndE;
#endif
}
