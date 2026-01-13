//
// system/threadtrace.h
//
// Copyright (C) 2013-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_THREADTRACE_H
#define SYSTEM_THREADTRACE_H


#define SYSTEM_THREADTRACE_ENABLE       (__PS3 && __BANK)

#if SYSTEM_THREADTRACE_ENABLE
#	define SYSTEM_THREADTRACE_ENABLE_ONLY(...)  __VA_ARGS__
#else
#	define SYSTEM_THREADTRACE_ENABLE_ONLY(...)
#endif


namespace rage
{
#	if SYSTEM_THREADTRACE_ENABLE

		extern void sysThreadTraceInit();
		extern void sysThreadTraceShutdown();
		extern void sysThreadTraceBeginFrame();
		extern void sysThreadTracePrintThreadStats();

#	else

		__forceinline void sysThreadTraceInit() {}
		__forceinline void sysThreadTraceShutdown() {}
		__forceinline void sysThreadTraceBeginFrame() {}
		__forceinline void sysThreadTracePrintThreadStats() {}

#	endif // SYSTEM_THREADTRACE_ENABLE
}
// namespace rage


#endif // SYSTEM_THREADTRACE_H
