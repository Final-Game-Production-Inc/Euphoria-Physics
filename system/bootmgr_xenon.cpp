// 
// system/bootmgr_xenon.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#include "bootmgr.h"

#if __XENON

#include "system/param.h"
#include "string/string.h"
#include "system/tmcommands.h"

#if !__FINAL
#include "file/remote.h"
#endif

#include "system/xtl.h"
#include <xbdm.h>

namespace rage {

XPARAM(forcebootcd);

u32 s_attached = NULL;

bool sysBootManager::IsBootedFromDisc() {
	bool result = PARAM_forcebootcd.Get() || !strncmp(sysParam::GetProgramName(),"A:",2);
#if !__FINAL
	if (!result && !fiRemoteServerIsRunning) {
		static bool warned;
		if (!warned) {
			Warningf("RFS not detected, assuming we booted from launcher or a disc");
			warned = true;
		}
		result = true;
	}
#endif
	return result;
}

#if !__FINAL
bool sysBootManager::IsDebuggerPresent()
{
	return DmIsDebuggerPresent() SYSTMCMD_ENABLE_ONLY(&& !PARAM_rockstartargetmanager.Get());
}

bool sysBootManager::IsDevkit()
{
	static bool sChecked/*=false*/;
	static bool sDevkit/*=false*/;
	if (!sChecked)
	{
		DWORD consoleType;
		HRESULT res = DmGetConsoleType(&consoleType);
		if (res < 0)
		{
			Errorf("Failed to determine 360 console type, HRESULT 0x%08x", res);
			// Leave default of not devkit
		}
		else
		{
			sDevkit = (consoleType == DMCT_DEVELOPMENT_KIT);
		}
		sChecked = true;
	}
	return sDevkit;
}
#endif

void sysBootManager::InitClass() {
	StartDebuggerSupport();
}


void sysBootManager::ShutdownClass() {
}

}	// namespace rage

#endif
