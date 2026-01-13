//
// system/platform.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_PLATFORM_H
#define SYSTEM_PLATFORM_H

namespace rage {

namespace platform {

const char WIN32PC = 'w';
const char XENON = 'x';
const char PS3 = 'c';			// 'Cell'
const char PSP2 = 'v';			// 'VITA' is the code name for the platform
const char WIN64PC = 'y';
const char UNKNOWN = 'u';
const char ORBIS = 'o';
const char DURANGO = 'd';

}	// platform


// Current platform
#if __XENON
#define RSG_PLATFORM	"Xbox 360"
#define RSG_PLATFORM_ID	"xbox360"
#define RSG_PLATFORM_CODE platform::XENON
#elif __PS3
#define RSG_PLATFORM	"PS3"
#define RSG_PLATFORM_ID	"ps3"
#define RSG_PLATFORM_CODE platform::PS3
#elif __PSP2
#define RSG_PLATFORM	"Vita"
#define RSG_PLATFORM_ID	"vita"
#define RSG_PLATFORM_CODE platform::PSP2
#elif RSG_ORBIS
#define RSG_PLATFORM	"PS4"
#define RSG_PLATFORM_ID	"ps4"
#define RSG_PLATFORM_CODE platform::ORBIS
#elif RSG_DURANGO
#define RSG_PLATFORM	"Durango"
#define RSG_PLATFORM_ID	"xboxone"
#define RSG_PLATFORM_CODE platform::DURANGO
#elif __64BIT
#define RSG_PLATFORM	"x64"
#define RSG_PLATFORM_ID	"x64"
#define RSG_PLATFORM_CODE platform::WIN64PC
#else
	#if __TOOL
		#define RSG_PLATFORM	"Tool"
		#define RSG_PLATFORM_ID	"tool"
		#define RSG_PLATFORM_CODE platform::UNKNOWN
	#elif __RESOURCECOMPILER
		#define RSG_PLATFORM	"Rsc"
		#define RSG_PLATFORM_ID	"rsc"
		#define RSG_PLATFORM_CODE platform::UNKNOWN
	#else
		#define RSG_PLATFORM	"Win32"
		#define RSG_PLATFORM_ID	"pc"
		#define RSG_PLATFORM_CODE platform::WIN32PC
	#endif
#endif

// Current build configuration
#if !__OPTIMIZED
	#if __DEV
		#define RSG_CONFIGURATION           "Debug"
		#define RSG_CONFIGURATION_LOWERCASE	"debug"
		#define RSG_CONFIGURATION_CODE      "d"
	#else // __DEV
		#define RSG_CONFIGURATION           "PreRelease"
		#define RSG_CONFIGURATION_LOWERCASE "prerelease"
		#define RSG_CONFIGURATION_CODE      "pr"
	#endif // _DEV
#elif __DEV
	#define RSG_CONFIGURATION           "Beta"
	#define RSG_CONFIGURATION_LOWERCASE "beta"
	#define RSG_CONFIGURATION_CODE      "b"
#elif __BANK
	#define RSG_CONFIGURATION           "BankRelease"
	#define RSG_CONFIGURATION_LOWERCASE "bankrelease"
	#define RSG_CONFIGURATION_CODE      "br"
#elif __PROFILE
	#define RSG_CONFIGURATION           "Profile"
	#define RSG_CONFIGURATION_LOWERCASE "profile"
	#define RSG_CONFIGURATION_CODE      "p"
#elif __FINAL
	#define RSG_CONFIGURATION           "Final"
	#define RSG_CONFIGURATION_LOWERCASE "final"
	#define RSG_CONFIGURATION_CODE      "f"
#else // __FINAL
	#define RSG_CONFIGURATION           "Release"
	#define RSG_CONFIGURATION_LOWERCASE "release"
	#define RSG_CONFIGURATION_CODE      "r"
#endif // __FINAL




const char g_sysHostPlatform = RSG_PC ? (__64BIT ? platform::WIN64PC : platform::WIN32PC) : RSG_DURANGO ? platform::DURANGO : RSG_ORBIS ? platform::ORBIS : __XENON ? platform::XENON : __PPU? platform::PS3 : __PSP2? platform::PSP2 : platform::UNKNOWN;

#if __WIN32PC
extern char g_sysPlatform;	// platform::WIN32PC, XENON, or PS3; initialized to the current platform.
#else
const char g_sysPlatform = g_sysHostPlatform;
#endif

/*	PURPOSE:	Translates a string into a rage::platform identifier
	PARAMS:		string - Platform name to translate
	RETURNS:	plaform::WIN32PC, XENON, PS3, PSP2, or UNKNOWN.
	NOTES:		Returns current (host) platform if string is NULL, same thing as g_sysHostPlatform.
*/
extern char sysGetPlatform(const char *string = NULL);

/*	PURPOSE:	Translates a platform identifier into a "needs byte swap" flag,
				based on the current target platform and the current actual platform.
	PARAMS:		platformId - Value of g_sysPlatform or return from sysGetPlatform.
	RETURNS:	True if we need a byte swap, else false.
*/
extern bool sysGetByteSwap(char platformId);

/*	RETURNS:	True if the platform uses big-endian byte order */
extern bool sysIsBigEndian(char platformId);

/*	RETURNS:	True if this platform is a 64-bit platform */
extern bool	sysIs64Bit(char platformId);

}	// rage

#endif
