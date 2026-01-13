// 
// system/platform.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "platform.h"

#include "string/string.h"
#include "system/endian.h"


namespace rage {

#if __WIN32PC
char g_sysPlatform = g_sysHostPlatform;
#endif

// PC knows about all platforms, Xenon and PS3 will only ever return their platform or UNKNOWN
char sysGetPlatform(const char *string) {
	if (!string)
		return g_sysHostPlatform;
#if RSG_PC
	else if (!stricmp(string,"win32pc") || !stricmp(string,"x86") || !stricmp(string,"pc") || !stricmp(string,"win32"))
		return platform::WIN32PC;
	else if (!stricmp(string,"win64pc") || !stricmp(string,"x64") || !stricmp(string,"win64"))
		return platform::WIN64PC;
	else if (!stricmp(string,"psp2") || !stricmp(string,"vita"))
		return platform::PSP2;
	else if (!stricmp(string,"durango") || !stricmp(string,"xboxone") )
		return platform::DURANGO;
	else if (!stricmp(string,"orbis") || !stricmp(string,"ps4") )
		return platform::ORBIS;
#endif
#if RSG_XENON || RSG_PC
	else if (!stricmp(string,"xenon") || !strcmp(string,"xbox360"))
		return platform::XENON;
#endif
#if RSG_PS3 || RSG_PC
	else if (!stricmp(string,"ps3") || !stricmp(string,"psn"))
		return platform::PS3;
#endif
#if RSG_DURANGO
	else if (!stricmp(string,"durango") || !stricmp(string,"xboxone"))
		return platform::DURANGO;
#endif
#if RSG_ORBIS
	else if (!stricmp(string,"orbis") || !stricmp(string,"ps4"))
		return platform::ORBIS;
#endif
	else
		return platform::UNKNOWN;
}

bool sysIsBigEndian(char platformId)
{
	using namespace platform;
	switch(platformId)
	{
	case XENON:
	case PS3:
		return true;
	default:
		return false;
	}
}

bool sysIs64Bit(char platformId)
{
	using namespace platform;
	switch(platformId)
	{
	case WIN64PC:
	case ORBIS:
	case DURANGO:
		return true;
	default:
		return false;
	}
}

bool sysGetByteSwap(char platformId) {
	return sysEndian::IsBig() != sysIsBigEndian(platformId);
}

}	// namespace rage


