//
// system/exception.cpp
//
// Copyright (C) 2014-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_EXCEPTION_H
#define SYSTEM_EXCEPTION_H

#define EXCEPTION_HANDLING	(!__RESOURCECOMPILER && !__TOOL && !__GAMETOOL && !__MASTER)

#define CRASHPAD_ENABLED 0
#define BREAKPAD_ENABLED (!__RESOURCECOMPILER && !__TOOL && !__GAMETOOL && RSG_PC && __MASTER)
#define BACKTRACE_DUMP_AND_UPLOAD_ENABLED (RSG_DURANGO)
#define BACKTRACE_TEST_CRASHES_ENABLED (!__MASTER)

#define BACKTRACE_ENABLED (CRASHPAD_ENABLED || BREAKPAD_ENABLED || BACKTRACE_DUMP_AND_UPLOAD_ENABLED)

#if EXCEPTION_HANDLING
#	define EXCEPTION_HANDLING_ONLY(...)     __VA_ARGS__
#else
#	define EXCEPTION_HANDLING_ONLY(...)
#endif

#if EXCEPTION_HANDLING

#include "tmcommands.h"

#include <stddef.h>

namespace rage
{

namespace sysException
{

void Init();

bool IsEnabled();

// PURPOSE: Returns true if an exception has been thrown.  Useful for preventing spurious additional exceptions/timeouts.
bool HasBeenThrown();

void Crash();

// PURPOSE: Formatting print function that will work even when output is
// disabled.  Publicly available for GameExceptionDisplay etc callbacks.
// NOTES: The reason for being a function pointer rather than a function is an
// implementation detail, the pointer is const so that it cannot be changed.
// Higher level code should treat as though it was a regular function.
extern void (*const Print)(const char *fmt, ...) PRINTF_LIKE_N(1);

// PURPOSE: Allow the application to perform additional work (such as changing DisplayLine) just prior
// to the code in ExceptionFilter() that displays the stack trace.
extern void (*PreExceptionDisplay)(size_t exceptionAddress);

// PURPOSE: Allow the application to perform additional work (such as restoring DisplayLine) just after
// to the code in ExceptionFilter() that displays the stack trace.
extern void (*PostExceptionDisplay)();

// PURPOSE: Allow the application to perform additional work (such as restoring DisplayLine) just after
// to the code in ExceptionFilter() that displays the stack trace.
extern void (*GameExceptionDisplay)();

// PURPOSE:	Function pointer for default stack line display as used by exception handler
extern void (*ExceptionDisplayStackLine)(size_t addr, const char *sym, size_t displacement);

#if RSG_DURANGO
	// PURPOSE:	Create a dump file and allow the game to continue.
	bool GenerateCrashDump();
#endif // RSG_DURANGO

#if SYSTMCMD_ENABLE
	void SetBugstarConfig(const void *config);
#endif

#if RSG_ORBIS
	extern void (*GpuExceptionCallback)();
#endif

}
// namespace sysException

}
// namespace rage

#endif // EXCEPTION_HANDLING

#if BACKTRACE_ENABLED

#define BACKTRACE_ONLY(...) __VA_ARGS__

#include "atl\map.h"
#include "atl\string.h"

struct _EXCEPTION_POINTERS;
typedef _EXCEPTION_POINTERS EXCEPTION_POINTERS;

namespace rage
{

namespace sysException
{

// PURPOSE: Helper class to set the token used with Crashpad / Breakpad during static initialization.
// Declare a static instance of this class at game-level to override the default token.
class BacktraceConfig
{
public:
	BacktraceConfig(const char* token, const char* submissionUrl);

	virtual void OnSetup() = 0;
	virtual bool ShouldSubmitDump(const wchar_t* logFilename) = 0;
	virtual bool UploadDump(const wchar_t* dumpFilename, const wchar_t* logFilename, const atMap<atString, atString>* annotations, const char* url) = 0;

	const char* GetToken() { return m_token; }
	const char* GetUrl() { return m_url; }

protected:
	const char* m_token;
	const char* m_url;

#if RSG_DURANGO
	static const atMap<atString, atString>* GetAnnotations();
#endif
};

// PURPOSE: Helper class to cache running script data, attach to backtrace
// attributes after the game crashes
struct BacktraceScriptData
{
	u32 m_LastLaunchedScriptProgramId;
	u32 m_TimeSinceLastLaunchedScript;
};

// PURPOSE: Set an annotation (a simple key / value pair) that will be visible in Backtrace when a dump is uploaded.
// Annotations can be overwritten by passing in the same key and a different value.
// The encoding for both key and value is unspecified but is almost certainly UTF-8.
// This function is safe to call on any thread.
void SetAnnotation(const char* key, const char* value, size_t bufferSize = 64);
void SetAnnotation(const char* key, int value);
void SetAnnotation(const char* key, u32 value);

// PURPOSE: Disable crash handling for Backtrace (Crashpad / Breakpad) if set up
void DisableBacktrace();

// PURPOSE: Notify backtrace about a script has launched, with relevant data.
// This data will be reported upon game crash
void BacktraceNotifyScriptLaunched(u32 scriptProgramId, u32 currentTime);
// PURPOSE: Get the last launched script's data
BacktraceScriptData BacktraceGetLastLaunchedScriptData();

#if BREAKPAD_ENABLED

// PURPOSE: Breakpad initial setup, should be called once either from sysException::Init (if exception handling is enabled)
// or directly from CommonMain_Prologue instead.
bool SetupBreakpad();

#endif // BREAKPAD_ENABLED

#if BACKTRACE_DUMP_AND_UPLOAD_ENABLED

bool SetupBacktraceDumpAndUpload();

#endif // BACKTRACE_DUMP_AND_UPLOAD_ENABLED

#if BACKTRACE_TEST_CRASHES_ENABLED

// PURPOSE: For testing random crashes.  Sets up the crash counter to a random value.
void SetupForTestCrashes();

// PURPOSE: For testing random crashes.  Returns true after a random number of calls (picked randomly in SetupForTestCrashes between 0 and the max value).
// Pass in -crashpadTestCrashMax to set the maximum number of calls before crashing, defaults to 1000.
bool ShouldCrashForBacktraceTest();

// PURPOSE: For testing random crashes.  Scatter this macro around the code and run with -crashpadTestCrashEnable to have the game crash at one of the locations.
// Works best in places that are called once per frame, to avoid skewing in favour of any particular location.
#define BACKTRACE_TEST_CRASH if (sysException::ShouldCrashForBacktraceTest()) { *((int*)0) = 0; }
#else
#define BACKTRACE_TEST_CRASH

#endif // BREAKPAD_TEST_CRASHES_ENABLED

} // namespace sysException

} // namespace rage

#else // BACKTRACE_ENABLED

#define BACKTRACE_ONLY(...)

#endif // BACKTRACE_ENABLED

#endif // SYSTEM_EXCEPTION_H
