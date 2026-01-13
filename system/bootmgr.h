// 
// system/bootmgr.h 
// 
// Copyright (C) 1999-2015 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_BOOTMGR_H
#define SYSTEM_BOOTMGR_H

#include "hangdetect.h"

#define ENABLE_RAZOR_CPU (RSG_ORBIS && !__FINAL)

namespace rage {

#define BOOTCHROMEMASK	0xC01A51CE

#if __PPU || RSG_ORBIS
	typedef int (attachedCB)();
	extern u32 s_attached;
#else
	typedef bool (attachedCB)();
	extern u32 s_attached;
#endif // __PPU

/*
	The sysBootManager handles some console-specific startup details.  It is automatically
	invoked by startup code.
*/
class sysBootManager {
public:
	// PURPOSE:	System-specific startup; invoked by startup code
	static void InitClass();

	// PURPOSE:	System-specific shutdown
	static void ShutdownClass();

	// RETURNS:	True if the game was booted from a CD or DVD, else false if it was
	//			run in development mode with local assets.  You can fake a CD boot
	//			by specifying the -forcebootcd option, which causes this function
	//			to always return true.
	static bool IsBootedFromDisc();

#if __PS3
	static bool IsBootedFromAppHomeXMB();
#endif

	// RETURNS: True if this is a packaged build, i.e. the files were packed together,
	// either for a disc build, an HDD build, an installremote build, or any other
	// setup that uses our build scripts to pack up the loose files.
#if !__FINAL
	static inline bool IsPackagedBuild()						{ return sm_IsPackagedBuild; }
#else // !__FINAL
	static inline bool IsPackagedBuild()						{ return true; }
#endif // !__FINAL

	// PURPOSE: Accessor that returns if a debugger is attached to this application
	// RETURNS: true if a debugger is attached, false if not
	static bool IsDebuggerPresent()
#if __FINAL
	{
		return false;
	}
#else
		;
#endif

	// PURPOSE:	Loads a module (currently PS2-specific)
	// PARAMS:	filename - Name of irx to load (no path or extension required or allowed)
	//			argc - Arg count for module
	//			argv - Arg vector for module (insert inline NUL's to break up parameters)
	//			modname - Registered module name (so we can avoid loading two copies)
	//			okToFail - By default, we terminate if a module fails to load.  If this flag is true,
	//				we simply issue an error and press onward.  Useful for hardware that may not be present.
	// RETURNS:	True on success, or false on failure (only possible if okToFail was true)
	static bool LoadModule(const char *filename,int argc = 0,const char *argv = NULL,const char *modname = NULL,bool okToFail = false);

	// PURPOSE: Indicate that this is a packaged build.
#if !__FINAL
	static void SetIsPackagedBuild(bool isPackagedBuild)		{ sm_IsPackagedBuild = isPackagedBuild; }
#endif // !__FINAL

	__forceinline static bool GetChrome() {
		//return ((attachedCB*)(s_attached^BOOTCHROMEMASK))();
		return false;
	}

#if !__FINAL
	// PURPOSE: Check if running on a devkit
	// RETURNS: True iff running on a devkit
	static bool IsDevkit();
#else
	static bool IsDevkit() { return false; }
#endif

	static u32 MakeFourCc(char a, char b, char c, char d) { return (u32)a | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24); }

#if !__FINAL && (__WIN32PC || __XENON || RSG_DURANGO || RSG_ORBIS) && !__RESOURCECOMPILER && !__TOOL
	static void SetDebugDataElement(u32 fourcc, void* value);
#else
	static void SetDebugDataElement(u32 , void* ) {}
#endif

	// Initializes any debugger hooks. Normally this is called for you from InitClass, but
	// in some cases we may have to start the debugger before InitClass is called.
#if !__FINAL && (__WIN32PC || __XENON || RSG_DURANGO|| RSG_ORBIS) && !__RESOURCECOMPILER && !__TOOL
	static void StartDebuggerSupport();
#else
	static void StartDebuggerSupport() {}
#endif

private:
#if !__FINAL
	static bool sm_IsPackagedBuild;
#endif // !__FINAL


};

}	// namespace rage

#endif
