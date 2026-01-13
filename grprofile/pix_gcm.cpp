//
// system/pix_gcm.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
//

#include "grprofile/pix.h"

#if __GCM && ENABLE_PIX_TAGGING

#include "grcore/wrapper_gcm.h"
#include "system/param.h"
#include "system/replay.h"

#if GCM_HUD
#define IS_CAPTURING_HUD	grcCellGcmHudIsEnabled
#else
#define IS_CAPTURING_HUD	false
#endif

bool g_CachedPIXIsGPUCapturing = false;

#include <sn/libsntuner.h>

#pragma comment(lib, "sntuner")

PARAM(pixsntuner,"Enable PIX captures while SN Tuner is running (normally only GPAD)");

#define IS_CAPTURING		(IS_CAPTURING_REPLAY || IS_CAPTURING_HUD || (PARAM_pixsntuner.Get() && snIsTunerRunning()))

using namespace rage;

static __THREAD u32 s_FilterStack;

void PIXBeginC(unsigned int filter, unsigned int /*c*/, const char *event)
{
	s_FilterStack <<= 1;
	IS_CAPTURING_REPLAY_DECL
	if (GCM_CONTEXT && (g_EnablePixAnnotation & (1 << filter)) && IS_CAPTURING)
	{
		s_FilterStack |= 1;

		cellGcmSetPerfMonPushMarker(GCM_CONTEXT,event);
	}
}

void PIXBegin(unsigned int filter, const char *event)
{
	s_FilterStack <<= 1;
	IS_CAPTURING_REPLAY_DECL
	if (GCM_CONTEXT && (g_EnablePixAnnotation & (1 << filter)) && IS_CAPTURING)
	{
		s_FilterStack |= 1;

		cellGcmSetPerfMonPushMarker(GCM_CONTEXT,event);
	}
}

void PIXBeginCN(unsigned int filter, unsigned int /*c*/, const char *event, ...)
{
	s_FilterStack <<= 1;
	IS_CAPTURING_REPLAY_DECL
	if (GCM_CONTEXT && (g_EnablePixAnnotation & (1 << filter)) && IS_CAPTURING)
	{
		s_FilterStack |= 1;

		char buffer[256];
		va_list	args;
		va_start(args, event);
		vsprintf(buffer, event, args);
		cellGcmSetPerfMonPushMarker(GCM_CONTEXT,buffer);
	}
}

void PIXBeginN(unsigned int filter, const char *event, ...)
{
	s_FilterStack <<= 1;
	IS_CAPTURING_REPLAY_DECL
	if (GCM_CONTEXT && (g_EnablePixAnnotation & (1 << filter)) && IS_CAPTURING)
	{
		s_FilterStack |= 1;

		char buffer[256];
		va_list	args;
		va_start(args, event);
		vsprintf(buffer, event, args);
		
		cellGcmSetPerfMonPushMarker(GCM_CONTEXT,buffer);
	}
}

void PIXEnd()
{
	IS_CAPTURING_REPLAY_DECL
	if (GCM_CONTEXT && (s_FilterStack & 1) && IS_CAPTURING)
	{
		cellGcmSetPerfMonPopMarker(GCM_CONTEXT);
	}
	s_FilterStack >>= 1;
}

void PIXEndE(const char * /*event*/)
{
	PIXEnd();
}

bool PIXSaveGPUCapture(char* /*filename*/)
{
	return true;
}

bool PIXIsGPUCapturing()
{
	IS_CAPTURING_REPLAY_DECL; 
	return IS_CAPTURING_REPLAY; 
}

#endif // __GCM && ENABLE_PIX_TAGGING
