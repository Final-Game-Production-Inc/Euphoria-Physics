//
// system/pix_pc.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved. 
//

#include "grprofile/pix.h"

#include <stdio.h>

#if	(__WIN32PC || __XENON || RSG_DURANGO) && ENABLE_PIX_TAGGING

#if __WIN32PC
	#include "system/xtl.h"
	#include "system/d3d9.h"
	#define __UNICODE 1
	#include "string/unicode.h"
	#define UNICODE_USES_CONVERSION USES_CONVERSION
	#include "grcore/device.h"
#elif RSG_DURANGO
	#include "system/xtl.h"
	#include <xdk.h>
	#include "grcore/wrapper_d3d.h"
	#include "system/d3d11.h"
	#define PROFILE_BUILD 1
	#define DBG	1			// yuck
	#include <pix.h>
	#pragma comment(lib, "PIXEvt.lib")
	#include "string/unicode.h"
	#define UNICODE_USES_CONVERSION USES_CONVERSION
	#include "grcore/device.h"
	#include "grcore/gfxcontext_durango.h"
#else
	#define UNICODE_USES_CONVERSION
	#include "system/xtl.h"
	# if !defined(USE_PIX)
	# error "PIX is not properly enabled for some reason."
	# endif
#endif // __WIN32PC

#if __WIN32PC
#include "grcore/d3dwrapper.h"
#endif

#include "system/tls.h"

using namespace rage;

bool g_CachedPIXIsGPUCapturing = false;

static char g_StringBuffer[256];
extern unsigned g_EnablePixAnnotation;

static __THREAD u32 s_FilterStack;

#if RSG_DURANGO

static __THREAD ID3DUserDefinedAnnotation* s_pAnnotations = NULL;

ID3DUserDefinedAnnotation * GetAnnotation()
{
	if (s_pAnnotations == NULL)
	{
#if !__D3D11_MONO_DRIVER
		CHECK_HRESULT( g_grcCurrentContext->QueryInterface< ID3DUserDefinedAnnotation >( &s_pAnnotations ) );
#else
		CHECK_HRESULT( g_grcCurrentContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&s_pAnnotations) );
#endif
		Assert(s_pAnnotations);
	}

	return s_pAnnotations;
}

void PIXBeginDurango(unsigned int c, const char *event)
{
	if (!GRCDEVICE.CheckThreadOwnership())
		return;

	(void) c;
	UNICODE_USES_CONVERSION;
	wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(event));

	GetAnnotation()->BeginEvent( string );

//	pAnnotations->Release();
}

void PIXEndDurango()
{
	if (!GRCDEVICE.CheckThreadOwnership())
		return;

	GetAnnotation()->EndEvent();

//	pAnnotations->Release();
}

#endif	// RSG_DURANGO

void PIXBeginC(unsigned int filter, unsigned int c, const char *event)
{
	if (!event || !*event) 
		event = "[no label]";
	s_FilterStack <<= 1;
	if (g_EnablePixAnnotation & (1 << filter))
	{
		s_FilterStack |= 1;
#if __WIN32PC
		if (GRCDEVICE.CheckThreadOwnership())
		{
			UNICODE_USES_CONVERSION;
			wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(event));
			if (g_D3DPERF_BeginEvent)
				g_D3DPERF_BeginEvent(c, string);
		}
#elif __XENON
		PIXBeginNamedEvent(c, event);
#elif RSG_DURANGO
		UNICODE_USES_CONVERSION;
		wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(event));
		PIXBeginEvent(c | 0xff000000, string);
		PIXBeginDurango(c, event);
#endif
	}
}

void PIXBegin(unsigned int filter, const char *event)
{
	if (!event || !*event) 
		event = "[no label]";
	s_FilterStack <<= 1;
	if (g_EnablePixAnnotation & (1 << filter))
	{
		s_FilterStack |= 1;
#if __WIN32PC
		if (GRCDEVICE.CheckThreadOwnership())
		{
			UNICODE_USES_CONVERSION
				wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(event));
			if (g_D3DPERF_BeginEvent)
				g_D3DPERF_BeginEvent(0x00FF00, string);
		}
#elif __XENON
		PIXBeginNamedEvent(0x00FF00, event);
#elif RSG_DURANGO
		UNICODE_USES_CONVERSION
			wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(event));
		PIXBeginEvent(0xFF00FF00, string);
		PIXBeginDurango(0x00FF00, event);
#endif
	}
}

void PIXBeginCN(unsigned int filter, unsigned int c, const char *event, ...)
{
	s_FilterStack <<= 1;
	if (g_EnablePixAnnotation & (1 << filter))
	{
		s_FilterStack |= 1;

		va_list	args;
		va_start(args, event);
		vsprintf(g_StringBuffer, event, args);

#if __WIN32PC
		UNICODE_USES_CONVERSION;
		if (GRCDEVICE.CheckThreadOwnership())
		{
			wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(g_StringBuffer));
			if (g_D3DPERF_BeginEvent)
				g_D3DPERF_BeginEvent(c, string);
		}
#elif __XENON
		PIXBeginNamedEvent(c, g_StringBuffer);
#elif RSG_DURANGO
		UNICODE_USES_CONVERSION
		wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(g_StringBuffer));
		PIXBeginEvent(c | 0xff000000, string);
		PIXBeginDurango(c, g_StringBuffer);
#endif
	}
}

void PIXBeginN(unsigned int filter, const char *event, ...)
{
	s_FilterStack <<= 1;
	if (g_EnablePixAnnotation & (1 << filter))
	{
		s_FilterStack |= 1;
		va_list	args;
		va_start(args, event);
		vsprintf(g_StringBuffer, event, args);

#if __WIN32PC
		UNICODE_USES_CONVERSION

		if (GRCDEVICE.CheckThreadOwnership())
		{
			wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(g_StringBuffer));
			if (g_D3DPERF_BeginEvent)
				g_D3DPERF_BeginEvent(0x00FF00, string);
		}
#elif __XENON
		PIXBeginNamedEvent(0x00FF00, g_StringBuffer);
#elif RSG_DURANGO
		UNICODE_USES_CONVERSION
		wchar_t	*string = reinterpret_cast<wchar_t*>(A2W(g_StringBuffer));
		PIXBeginEvent(0xFF00FF00, string);
		PIXBeginDurango(0x00FF00, g_StringBuffer);
#endif
	}
}

void PIXEnd()
{
	if (g_EnablePixAnnotation && s_FilterStack & 1)
	{
#if __WIN32PC
		if (GRCDEVICE.CheckThreadOwnership())
		{
			if (g_D3DPERF_EndEvent)
				g_D3DPERF_EndEvent();
		}
#elif __XENON
		PIXEndNamedEvent();
#elif RSG_DURANGO
		PIXEndEvent();
		PIXEndDurango();
#endif
	}
	s_FilterStack >>= 1;
}

bool PIXSaveGPUCapture(char* XENON_ONLY(filename))
{ 
#if __XENON
	return (S_OK == PIXCaptureGpuFrame(filename));
#else
	return true;
#endif
}

bool PIXIsGPUCapturing() 
{ 
#if __XENON
	return PIXGetCaptureState() ==  PIX_CAPTURE_GPU; 
#elif RSG_DURANGO
	return PIXGetCaptureState() ==  PIX_CAPTURE_GPU;
#elif RSG_PC && __D3D11
	return g_D3DPERF_GetStatus && g_D3DPERF_GetStatus();
#else
	return false;
#endif
}


#endif // (__WIN32PC || __XENON) && ENABLE_PIX_TAGGING
