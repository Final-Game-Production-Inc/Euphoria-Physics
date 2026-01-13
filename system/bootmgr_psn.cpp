// 
// system/bootmgr_psn.cpp 
// 
// Copyright (C) 1999-2015 Rockstar Games.  All Rights Reserved. 
// 
#include "bootmgr.h"

#include "system/param.h"
#include "system/tmcommands.h"
#include "system/nelem.h"
#include "string/string.h"

#if __PPU

#include <sys/paths.h>

extern "C" int snIsDebuggerRunning (void);
#pragma comment(lib, "sn")

namespace rage {

XPARAM(forcebootcd);
XPARAM(forceboothdd);

u32 s_attached = u32(&snIsDebuggerRunning) ^ BOOTCHROMEMASK;

bool sysBootManager::IsBootedFromAppHomeXMB() {
	return !strcmp(sysParam::GetProgramName(),"/app_home/PS3_GAME/USRDIR/EBOOT.BIN");
}

bool sysBootManager::IsBootedFromDisc() {
#if __FINAL && __NO_OUTPUT
	// Master always boots from disc
	return true;
#else // __NO_OUTPUT
	return (PARAM_forcebootcd.Get() || PARAM_forceboothdd.Get() ||
		!strncmp(sysParam::GetProgramName(),"/dev_bdvd/", 10) || 
		!strncmp(sysParam::GetProgramName(),"/dev_hdd", 8)) ||
		!strcmp(sysParam::GetProgramName(),"/app_home/PS3_GAME/USRDIR/EBOOT.BIN");
#endif // __NO_OUTPUT
}

#if !__FINAL
bool sysBootManager::IsDebuggerPresent()
{
	return snIsDebuggerRunning() != 0 SYSTMCMD_ENABLE_ONLY(&& !PARAM_rockstartargetmanager.Get());
}
#endif

void sysBootManager::InitClass() {
	StartDebuggerSupport();
}


void sysBootManager::ShutdownClass() {
}

}	// namespace rage

#elif __PSP2

namespace rage {

void sysBootManager::InitClass() {
	StartDebuggerSupport();
}


void sysBootManager::ShutdownClass() {
}


bool sysBootManager::IsDebuggerPresent()
{
	return false;
}

}

#elif RSG_ORBIS

#include <sdk_version.h>

#if !__FINAL
#	include <libdbg.h>
#	pragma comment(lib, "SceDbg_stub_weak")
#endif

#if ENABLE_RAZOR_CPU
#	include <perf.h>
#	pragma comment(lib,"ScePerf_stub_weak")
#endif

namespace rage {

static bool s_IsDevkit = false;

void sysBootManager::InitClass() {
	// Initialise s_IsDevkit
#if !__FINAL
	IsDebuggerPresent();
#endif

	StartDebuggerSupport();

#if ENABLE_RAZOR_CPU && SCE_ORBIS_SDK_VERSION < 0x01700000u
	static uint64_t traceBuffer[4 * 1024 * 1024];	// 32M buffer, copied from samples
	sceRazorCpuInit(traceBuffer,sizeof(traceBuffer));
#endif //ENABLE_RAZOR_CPU
}

void sysBootManager::ShutdownClass() {
}

#if !__FINAL
bool sysBootManager::IsDebuggerPresent() {
	static bool first = true;
	if(first)
	{
		// See if this is a testkit, since all debug calls crash on PS4 test kits. Hooray!
		// There's no "nice" way to see if we're running on a test kit, so see if the libPerf module
		// is present. See https://ps4.scedev.net/forums/thread/38579/.
		SceKernelModule modules[256];
		bool haveLibPerf = false;
		size_t numLoaded = 0;
		sceKernelGetModuleList(modules, NELEM(modules), &numLoaded);
		for(size_t i=0; i<numLoaded; ++i)
		{
			SceKernelModuleInfo info;
			info.size = sizeof(info);
			if(sceKernelGetModuleInfo(modules[i], &info) == SCE_OK)
			{
				if(strncmp(info.name, "libScePerf", 10) == 0)
				{
					haveLibPerf = true;
					break;
				}
			}
		}
		if(!haveLibPerf)
		{
			Displayf("libScePerf is not present: Assuming this is a PS4 test kit.");
			s_IsDevkit = false;
		}
		else
		{
			s_IsDevkit = true;
		}
		first = false;
	}

	return s_IsDevkit && sceDbgIsDebuggerAttached() != 0 SYSTMCMD_ENABLE_ONLY(&& !PARAM_rockstartargetmanager.Get());
}

bool sysBootManager::IsDevkit() {
	return s_IsDevkit;
}

#endif // !__FINAL

XPARAM(forcebootcd);
XPARAM(forceboothdd);

bool sysBootManager::IsBootedFromDisc() {
#if __FINAL && __NO_OUTPUT
	// Master always boots from disc
	return true;
#else // __NO_OUTPUT
	return PARAM_forcebootcd.Get();
#endif // __NO_OUTPUT
}

}

#endif
