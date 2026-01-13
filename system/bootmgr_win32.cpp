// 
// system/bootmgr_win32.cpp 
// 
// Copyright (C) 1999-2015 Rockstar Games.  All Rights Reserved. 
// 
#include "bootmgr.h"

#if RSG_PC || RSG_DURANGO

#include "string/string.h"
#include "system/membarrier.h"
#include "system/param.h"
#include "system/tmcommands.h"
#include "system/xtl.h"		// IsDebuggerPresent is available in SDK now

namespace rage {

XPARAM(forcebootcd);

#if RSG_DURANGO
PARAM(devkit, "let the game know it is running on a devkit");
static bool s_IsDevkit/*=false*/;
static bool s_IsDevkitDetectionDone/*=false*/;
#endif

u32 s_attached = 0;

bool sysBootManager::IsBootedFromDisc() {
#if !RSG_PC
	return PARAM_forcebootcd.Get() || !strncmp(sysParam::GetProgramName(),"D:",2);
#else
	return PARAM_forcebootcd.Get();
#endif
}

#if !__FINAL

bool sysBootManager::IsDebuggerPresent()
{
#if RSG_PC && SYSTMCMD_ENABLE
	if (PARAM_rockstartargetmanager.Get()) return false;
#endif
	return ::IsDebuggerPresent()?true:false;
}

bool sysBootManager::IsDevkit() {
#if RSG_DURANGO
	// There is a threading race condition here, but it is safe, all that can
	// happen is two threads run the devkit detection concurrently.
	if (!s_IsDevkitDetectionDone)
	{
		// 360 had a nice simple DmGetConsoleType function, but there doesn't seem
		// to be anything like that on XB1.  Have asked on the XB1 forms
		// (https://forums.xboxlive.com/AnswerPage.aspx?qid=49ec94b3-ae88-4931-a9c9-33ea8cac0f08&tgt=1)
		// if there is any better way to do this.
		//
		// If there is a debugger connected, then this must be a devkit.
		if (::IsDebuggerPresent())
			s_IsDevkit = true;
		// If the game was launched via a batchfile or R*TM, then that can do the
		// testkit/devkit detection and specify -devkit on the commandline if a
		// devkit was found.
		else if (PARAM_devkit.Get())
			s_IsDevkit = true;
		// Else we really have no idea, so have to assume it is a testkit.
		else
			s_IsDevkit = false;

		// Ensure s_IsDevkit is writen before s_IsDevkitDetectionDone
		sysMemWriteBarrier();

		s_IsDevkitDetectionDone = true;
	}
	return s_IsDevkit;
#else
	return false;
#endif
}

#endif

void sysBootManager::InitClass() {
	StartDebuggerSupport();
}


void sysBootManager::ShutdownClass() {
}

} // namespace rage


#endif
