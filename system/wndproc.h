// 
// system/wndproc.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_WNDPROC_H
#define SYSTEM_WNDPROC_H

struct HWND__;
struct HINSTANCE__;

namespace rage {

#if __WIN32PC
typedef unsigned rageUINT;
# if __64BIT
typedef unsigned __int64 rageWPARAM;
typedef signed __int64 rageLPARAM;
typedef signed __int64 rageLRESULT;
typedef signed __int64 rageINT_PTR;
# else
typedef unsigned int rageWPARAM;
typedef long rageLPARAM;
typedef long rageLRESULT;
typedef signed int rageINT_PTR;
# endif
// PURPOSE: The windows procedure function to use to hanlde Windows messages
extern rageLRESULT (__stdcall *g_WindowProc)(struct HWND__ *,rageUINT,rageWPARAM,rageLPARAM);
#endif

// The width of the window the game is running in
extern int g_WindowWidth;

// The height of the window the game is running in
extern int g_WindowHeight;

// One over the width of the window the game is running in
extern float g_InvWindowWidth;

// One over the height of the window the game is running in
extern float g_InvWindowHeight;

extern struct HWND__ *g_hwndMain, *g_hwndParent, *g_hwndOverride;

extern HINSTANCE__ *g_hInstance;

extern unsigned g_winClass;

extern bool g_inWindow;

extern bool g_isTopMost;

}

#endif
