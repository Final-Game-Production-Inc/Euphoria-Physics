// 
// system/defaultresponsefile.cpp 
// 
// Copyright (C) 1999-2007 Rockstar Games.  All Rights Reserved. 
// 

namespace rage {

#if __XENON
	const char *g_DefaultResponseFile = "game:\\commandline.txt";
#elif __PPU
	const char *g_DefaultResponseFile = "commandline.txt";
#elif RSG_DURANGO
#ifdef __FINAL_LOGGING
	const char *g_DefaultResponseFile = "g:\\commandline.txt";
#else
	const char *g_DefaultResponseFile = "xg:\\commandline.txt";
#endif
#elif RSG_ORBIS
	const char *g_DefaultResponseFile = "commandline.txt";
#elif RSG_PC && __FINAL
	// PC final will always try to load commandline.txt
	// For now, non-final PC builds will continue with their original behavior. (no load; see below)
	// This will eventually change to be more consistent across all builds/platforms.
	const char *g_DefaultResponseFile = "commandline.txt";
#else
	// all other platforms and configs will not load any response files by default (must be specified on the command line via @<responsefile>)
	const char *g_DefaultResponseFile;
#endif

} // namespace rage
