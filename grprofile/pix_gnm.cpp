//
// system/pix_gcm.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
//

#include "grprofile/pix.h"

#if RSG_ORBIS && ENABLE_PIX_TAGGING

#include "grcore/gnmx.h"
#include "grcore/device.h"
#include <stdio.h>

#include "system/tls.h"
#include "grcore/wrapper_gnm.h"
#include "system/param.h"
#include "grcore/gfxcontext_gnm.h"
#include "grcore/gputrace_gnm.h"

#include <gnm/platform.h>

static __THREAD rage::u32 s_FilterStack;

PARAM(rgpu,"Insert Razor GPU labels (slow, but what you want when debugging graphics)");

#define IS_CAPTURING (PARAM_rgpu.Get() || grcGpuTraceGNM::IsCapturing() || (sce::Gnm::isCaptureInProgress()))

unsigned int g_EnablePixAnnotation = ~0;
bool g_CachedPIXIsGPUCapturing = true;

using namespace rage;

void PIXBeginC(unsigned int filter, unsigned int /*c*/, const char *event)
{
	s_FilterStack <<= 1;
	if (&gfxc && (g_EnablePixAnnotation & (1 << filter)) && IS_CAPTURING)
	{
		s_FilterStack |= 1;

		gfxc.pushMarker(event);
#if SCE_ORBIS_SDK_VERSION < 0x01600051u
		if( grcGpuTraceGNM::IsCapturing() )
			sceRazorGpuThreadTracePushMarker(&gfxc.m_dcb, event);
#endif
	}
}

void PIXBegin(unsigned int filter, const char *event)
{
	s_FilterStack <<= 1;
	if (&gfxc && (g_EnablePixAnnotation & (1 << filter)) && IS_CAPTURING)
	{
		s_FilterStack |= 1;

		gfxc.pushMarker(event);
#if SCE_ORBIS_SDK_VERSION < 0x01600051u
		if( grcGpuTraceGNM::IsCapturing() )
			sceRazorGpuThreadTracePushMarker(&gfxc.m_dcb, event);
#endif
	}
}

void PIXBeginCN(unsigned int filter, unsigned int /*c*/, const char *event, ...)
{
	s_FilterStack <<= 1;
	if (&gfxc && (g_EnablePixAnnotation & (1 << filter)) && IS_CAPTURING)
	{
		s_FilterStack |= 1;

		char buffer[256];
		va_list	args;
		va_start(args, event);
		vsprintf(buffer, event, args);
		gfxc.pushMarker(buffer);
#if SCE_ORBIS_SDK_VERSION < 0x01600051u
		if( grcGpuTraceGNM::IsCapturing() )
			sceRazorGpuThreadTracePushMarker(&gfxc.m_dcb, buffer);
#endif
	}
}

void PIXBeginN(unsigned int filter, const char *event, ...)
{
	s_FilterStack <<= 1;
	if (&gfxc && (g_EnablePixAnnotation & (1 << filter)) && IS_CAPTURING)
	{
		s_FilterStack |= 1;

		char buffer[256];
		va_list	args;
		va_start(args, event);
		vsprintf(buffer, event, args);
		
		gfxc.pushMarker(buffer);
#if SCE_ORBIS_SDK_VERSION < 0x01600051u
		if( grcGpuTraceGNM::IsCapturing() )
			sceRazorGpuThreadTracePushMarker(&gfxc.m_dcb, buffer);
#endif
	}
}

void PIXEnd()
{
	if (&gfxc && (s_FilterStack & 1) && IS_CAPTURING)
	{
		gfxc.popMarker();
#if SCE_ORBIS_SDK_VERSION < 0x01600051u
		if( grcGpuTraceGNM::IsCapturing() )
			sceRazorGpuThreadTracePopMarker(&gfxc.m_dcb);
#endif
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
	return IS_CAPTURING; 
}

#endif // RSG_ORBIS && ENABLE_PIX_TAGGING
