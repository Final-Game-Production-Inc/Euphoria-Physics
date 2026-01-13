// 
// system/wndproc.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "wndproc.h"

#if __WIN32PC
#include "system/xtl.h"
rage::rageLRESULT (__stdcall *rage::g_WindowProc)(struct HWND__ *,rage::rageUINT,rage::rageWPARAM,rage::rageLPARAM) = DefWindowProcW;
#endif

// These are currently used by the __WIN32PC mouse code to properly normalize
// the mouse cursor to the backbuffer size regardless of the actual client window size.
// Don't get tempted to use them for something else.
int rage::g_WindowWidth, rage::g_WindowHeight;
float rage::g_InvWindowWidth, rage::g_InvWindowHeight;

/* Main application window.  If this is not set before calling InitClass, grcDevice
	will create a window for you. */
struct HWND__ *rage::g_hwndMain = NULL;

/* Parent of main application window.  Leave this as NULL if you want a normal
	toplevel application window */
struct HWND__ *rage::g_hwndParent = NULL;

/* Optional override for Present call; leave NULL to default to g_hwndMain */
struct HWND__ *rage::g_hwndOverride = NULL;

struct HINSTANCE__ *rage::g_hInstance = NULL;

/* Main application window class. */
unsigned rage::g_winClass;

/* Flag to indicate whether we are currently running in a window or fullscreen */
bool rage::g_inWindow;

/* Flag to indicate whether this window should be on top of all the other windows only used in window mode */
bool rage::g_isTopMost;
