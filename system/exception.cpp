//
// system/exception.cpp
//
// Copyright (C) 2014-2015 Rockstar Games.  All Rights Reserved.
//

//
// *****************************************************************************
// *  WARNING:                                                                 *
// *                                                                           *
// *  If you need to ask Sony any questions about the exception handler code,  *
// *  do NOT use their public forums.  Only use private support.               *
// *                                                                           *
// *  PS4 exception handling is for some retarded reason not part of the       *
// *  regular SDK, and is a closely guarded secret that was a pain in the      *
// *  arse to get access to.                                                   *
// *                                                                           *
// *  See https://ps4.scedev.net/support/issue/33723.                          *
// *                                                                           *
// *****************************************************************************
//

#include "exception.h"

#include "system/param.h"

// Note that some gametools require PARAM_unattended, so it needs to be declared
// outside of the #if EXCEPTION_HANDLING block.
namespace rage
{
PARAM(unattended, "[startup] Don't pop up blocking screens in the game (signin, display calibration, controller disconnected, etc)");
}

#if BACKTRACE_TEST_CRASHES_ENABLED
namespace rage
{
	NOSTRIP_PARAM(backtraceTestCrashMin, "[startup] Minimum value for crash test counter (higher values result in later crashes, default is 1)");
	NOSTRIP_PARAM(backtraceTestCrashMax, "[startup] Maximum value for crash test counter (higher values result in later crashes, default is 1000)");
	NOSTRIP_PARAM(backtraceTestCrashEnable, "[startup] Causes a random crash during startup.");
}
#endif

#if (EXCEPTION_HANDLING || BACKTRACE_DUMP_AND_UPLOAD_ENABLED)

#include "diag/channel.h"
#include "diag/output.h"
#include "file/default_paths.h"
#include "string/string.h"
#include "system/bootmgr.h"
#include "system/criticalsection.h"
#include "system/exec.h"
#include "system/hangdetect.h"
#include "system/ipc.h"
#include "system/memory.h"
#include "system/nelem.h"
#include "system/stack.h"
#include "system/tmcommands.h"
#include "system/xtl.h"
#include <stdio.h>
#include <string.h>

#if __WIN32
#	include <dbghelp.h>
#elif RSG_ORBIS
#	include <kernel.h>
#	include <libdbg.h>
#	include <sdk_version.h>
#	if SCE_ORBIS_SDK_VERSION > 0x02000000u
#		error "check if there is new exception.h and libSceDbgException_stub_weak.a available"
		// These files are not part of the regular SDK release, and are downloaded via
		// "SDK / Development Tools" > "SDK" > "Private Releases for the limited Users" > "Exception Handler library - X.XXX.XXX".
		// Then copy "target/lib/libSceDbgException_stub_weak.a" and "target/include/exception.h" (renaming to scedbgexception.h to avoid conflicts).
		// Once that is done, bump the version number check above to remind us next SDK update.  Thanks.
#	endif
#	include "scedbgexception.h"
#	pragma comment(lib, "libSceDbgException_stub_weak.a")
#	pragma comment(lib, "libSceDbg_stub_weak.a")
#endif

#if RSG_ORBIS
#	include <coredump.h>
#	pragma comment(lib,"SceCoredump_stub_weak")
#endif

// There probably isn't any real need for exceptions to dump out register values
// anymore.  With coredumps being generated, the register spew is basically
// redundant.
#define EXCEPTION_DISPLAY_REGISTERS         (1)

// Change these to modify the prefix before all exception handler printouts, if you want it to stand out.
#define PREFIX	TPurple
#define SUFFIX	TNorm


#define STACK_SYMBOL_SUPPORT    (!__FINAL && !__NO_OUTPUT)

#if RSG_DURANGO
typedef BOOL (__cdecl MiniDumpWriteDump_FnType)(
	HANDLE hProcess,
	DWORD ProcessId,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	/*PMINIDUMP_CALLBACK_INFORMATION*/PVOID CallbackParam);
static MiniDumpWriteDump_FnType *MiniDumpWriteDump_FnPtr;
#endif // RSG_DURANGO

#endif // (EXCEPTION_HANDLING || BACKTRACE_DUMP_AND_UPLOAD_ENABLED)

#if EXCEPTION_HANDLING


namespace rage
{

PARAM(forceexceptions, "[startup] Force exceptions even when we think a debugger is present");
PARAM(nominidump, "Do not run a mini dump.");

namespace sysException
{

#if SYSTMCMD_ENABLE
	static const void *s_BugstarConfig/*=NULL*/;
	void SetBugstarConfig(const void *config)
	{
		s_BugstarConfig = config;
	}
#endif // SYSTMCMD_ENABLE

static bool IsForceExceptions()
{
#	if !__FINAL && !__NO_OUTPUT
		return PARAM_forceexceptions.Get();
#	else
		return false;
#	endif
}

static void FlushLogFile()
{
#	if !__NO_OUTPUT
		diagChannel::FlushLogFile();
#	endif
}

#if __NO_OUTPUT
	static void NativePrint(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
#		if __WIN32
			char buffer[CHANNEL_MESSAGE_SIZE];
			vformatf(buffer, sizeof(buffer), fmt, args);
			OutputDebugString(buffer);
#		else
			vprintf(fmt, args);
#		endif
		va_end(args);
	}
	void (*const Print)(const char *fmt, ...) = NativePrint;
#else
	void (*const Print)(const char *fmt, ...) = diagLoggedPrintf;
#endif

static void DefaultExceptionDisplayStackLine(size_t addr, const char *sym, size_t offset)
{
	Print(PREFIX "0x%016" SIZETFMT "x - %s+0x%" SIZETFMT "x" SUFFIX "\n", addr, sym, offset);
}

void (*PreExceptionDisplay)(size_t) = NULL;
void (*PostExceptionDisplay)() = NULL;
void (*GameExceptionDisplay)() = NULL;
void (*ExceptionDisplayStackLine)(size_t, const char*, size_t) = DefaultExceptionDisplayStackLine;

#if RSG_ORBIS
	void (*GpuExceptionCallback)() = NULL;
#endif


// This critical section object is required because it is possible for threads
// to collide within the exception handler.
static sysCriticalSectionToken s_Mutex;


static volatile bool s_HasBeenThrown = false;
static bool s_IsEnabled = false;

bool IsEnabled()
{
	return s_IsEnabled;
}

bool HasBeenThrown()
{
	return s_HasBeenThrown;
}

void Crash()
{
#	if __FINAL
		*(volatile int*)0 = 0;
#	else
		__debugbreak();
#	endif
}

void TargetCrashCallback(unsigned pass)
{
	switch (pass)
	{
		// pass = 0: R*TM has begun logging a crash (Or intentional dump)
		case 0:
		{
			HANG_DETECT_SAVEZONE_ENTER();
			s_HasBeenThrown = true;
			FlushLogFile();
			break;
		}

		// pass = 1: R*TM has finished logging a crash
		case 1:
		{
			FlushLogFile();

			Print(PREFIX "");
			if (GameExceptionDisplay)
			{
				GameExceptionDisplay();
			}

			FlushLogFile();

			Print(PREFIX "");
			if (PostExceptionDisplay)
			{
				PostExceptionDisplay();
			}

			// Disable output since any further spam is useless and just confuses
			// things by hiding the exception handler output.
			OUTPUT_ONLY(diagChannel::SetOutput(false);)

			FlushLogFile();
			break;
		}

		// pass = 2: R*TM has finished logging an intentional dump (Currently PS4 only)
		case 2:
		{
			FlushLogFile();
			s_HasBeenThrown = false;
			HANG_DETECT_SAVEZONE_EXIT(NULL);
			break;
		}
	}
}

#if RSG_ORBIS

#	define EXCEPTION_TYPES(FUNC)                                               \
		FUNC(SCE_DBG_EXCEPTION_ILL),                                           \
		FUNC(SCE_DBG_EXCEPTION_FPE),                                           \
		FUNC(SCE_DBG_EXCEPTION_BUS),                                           \
		FUNC(SCE_DBG_EXCEPTION_SEGV),                                          \
		FUNC(SCE_DBG_EXCEPTION_GPU),
#	define EXCEPTION_TYPES_VALUE(NAME)     NAME
#	define EXCEPTION_TYPES_STRING(NAME)    #NAME
	static const int s_ExceptionTypeValues[] =
	{
		EXCEPTION_TYPES(EXCEPTION_TYPES_VALUE)
	};
	static const char *const s_ExceptionTypeStrings[] =
	{
		EXCEPTION_TYPES(EXCEPTION_TYPES_STRING)
	};
#	undef EXCEPTION_TYPES_STRING
#	undef EXCEPTION_TYPES_VALUE
#	undef EXCEPTION_TYPES

	static SceKernelEqueue s_GpuEventQueue;

	static void ExceptionHandler(int type, SceDbgUcontext *context)
	{
		// Lock the critical section token because it's possible for another
		// thread to have an exception while the first exception is still being
		// handled.
		s_Mutex.Lock();

		// Don't spam up the output with hang detection countdown warnings.
		HANG_DETECT_SAVEZONE_ENTER();

		s_HasBeenThrown = true;

		// Make sure output is enabled so that we actually print the exception
		// handler info.
		OUTPUT_ONLY(diagChannel::SetOutput(true);)
		FlushLogFile();

#		if SYSTMCMD_ENABLE
			if (!sysBootManager::IsDevkit() && PARAM_rockstartargetmanager.Get())
			{
				sysTmCmdExceptionHandlerBegin();
			}
#		endif

		// Emergency attempt to get a coredump if we notice we have somehow
		// gone recursive.  For example, a crash inside some gameplay
		// exception handling callback.
		static bool recursive;
		if (recursive)
		{
			// Use printf as we are in emergency cleanup right now, and
			// that is less likely to have issues than diagLoggedPrintf et al.
			printf("ERROR: Recursive exception handler detected\n");
			const int ret = sceDbgRemoveExceptionHandler(type);
			if (ret != SCE_OK)
			{
				printf("ERROR: Unable to remove exception handler, error 0x%08x\n", ret);
			}
			return;
		}
		recursive = true;

		mcontext_t *const mc = &(context->uc_mcontext);

#		if STACK_SYMBOL_SUPPORT
			sysStack::OpenSymbolFile();
			char symName[256] = "unknown";
			u32 disp = sysStack::ParseMapFileSymbol(symName, sizeof(symName), mc->mc_rip);
#		else
			const char *const symName = "nosymbols";
			const u32 disp = 0;
#		endif

		Print(PREFIX "");
		if (PreExceptionDisplay)
		{
			PreExceptionDisplay(mc->mc_rip);
		}

		const char *exceptionStr = NULL;
		for (unsigned i=0; i<NELEM(s_ExceptionTypeValues); ++i)
		{
			if (s_ExceptionTypeValues[i] == type)
			{
				exceptionStr = s_ExceptionTypeStrings[i];
				break;
			}
		}
		Print(PREFIX "*** EXCEPTION %s CAUGHT at address 0x%016lx" SUFFIX "\n", exceptionStr, mc->mc_rip);

#		if EXCEPTION_DISPLAY_REGISTERS
			ExceptionDisplayStackLine(mc->mc_rip, symName, disp);
			Print("\n" PREFIX "RAX %016lx   R8  %016lx" SUFFIX "\n",   mc->mc_rax, mc->mc_r8 );
			Print(     PREFIX "RBX %016lx   R9  %016lx" SUFFIX "\n",   mc->mc_rbx, mc->mc_r9 );
			Print(     PREFIX "RCX %016lx   R10 %016lx" SUFFIX "\n",   mc->mc_rcx, mc->mc_r10);
			Print(     PREFIX "RDX %016lx   R11 %016lx" SUFFIX "\n",   mc->mc_rdx, mc->mc_r11);
			Print(     PREFIX "RSP %016lx   R12 %016lx" SUFFIX "\n",   mc->mc_rsp, mc->mc_r12);
			Print(     PREFIX "RBP %016lx   R13 %016lx" SUFFIX "\n",   mc->mc_rbp, mc->mc_r13);
			Print(     PREFIX "RSI %016lx   R14 %016lx" SUFFIX "\n",   mc->mc_rsi, mc->mc_r14);
			Print(     PREFIX "RDI %016lx   R15 %016lx" SUFFIX "\n\n", mc->mc_rdi, mc->mc_r15);
#		endif

		// Display the callstack
		ExceptionDisplayStackLine(mc->mc_rip, symName, disp);
		u64 rbp = mc->mc_rbp;
		u64 stackStart, stackEnd;
		bool stackCorrupted = (sceKernelIsStack((void*)rbp, (void**)&stackStart, (void**)&stackEnd) != SCE_OK);
		if (!stackCorrupted)
		{
			for (;;)
			{
				// If the qwords at rbp and rbp+8 are not within the stack,
				// bail out and flag stack corruption.  Notice that both rbp
				// and rbp+16 are tested against stack end to ensure a
				// corrupted rbp that wraps when 16 is added to it, is
				// flagged as corrupted.
				if (rbp<stackStart || rbp>stackEnd || rbp+16>stackEnd)
				{
					stackCorrupted = true;
					break;
				}

				// Break out if end of stack reached.
				const u64 rip = *(u64*)(rbp+8);
				if (!rip)
				{
					break;
				}

#				if STACK_SYMBOL_SUPPORT
					disp = sysStack::ParseMapFileSymbol(symName, sizeof(symName), rip);
#				endif
				ExceptionDisplayStackLine(rip, symName, disp);

				rbp = *(u64*)rbp;
			}
		}
		if (stackCorrupted)
		{
			Print(PREFIX "stack corrupted" SUFFIX "\n");
		}

		FlushLogFile();

		Print(PREFIX "\n");
		if (GameExceptionDisplay)
		{
			GameExceptionDisplay();
		}

		FlushLogFile();

		Print(PREFIX "");
		if (PostExceptionDisplay)
		{
			PostExceptionDisplay();
		}

		Print(PREFIX "\n");

		// Disable output since any further spam is useless and just confuses things
		// by hiding the exception handler output.
		OUTPUT_ONLY(diagChannel::SetOutput(false);)

		FlushLogFile();

#		if STACK_SYMBOL_SUPPORT
			sysStack::CloseSymbolFile();
#		endif

		const int ret = sceDbgRemoveExceptionHandler(type);
		if (ret != SCE_OK)
		{
			Print(PREFIX "ERROR: Failed to remove exception handler, error 0x%08x." SUFFIX "\n", ret);
		}

#	    if SYSTMCMD_ENABLE
			if (!sysBootManager::IsDevkit() && PARAM_rockstartargetmanager.Get())
			{
				sysTmCmdExceptionHandlerEnd("", s_BugstarConfig);
			}
#		endif

		FlushLogFile();
	}

	static void GpuEventHandlerThread(void*)
	{
		for (;;)
		{
			int ret;

			SceKernelEvent event;
			int num = 0;
			ret = sceKernelWaitEqueue(s_GpuEventQueue, &event, 1, &num, NULL);
			if (ret != SCE_OK)
			{
				Print(PREFIX "WARNING: GpuEventHandlerThread, unexpected result waiting on event queue, error 0x%08x." SUFFIX "\n", ret);
			}
			if (num == 0)
			{
				continue;
			}

			// Make sure output is enabled so that we actually print the
			// exception handler info.
			OUTPUT_ONLY(diagChannel::SetOutput(true);)
			FlushLogFile();

#			if SYSTMCMD_ENABLE
				if (!sysBootManager::IsDevkit() && PARAM_rockstartargetmanager.Get())
				{
					sysTmCmdExceptionHandlerBegin();
				}
#			endif

			// The GPU exception type appears to be stored in the least
			// significant 32-bits of the event data.
			const u32 type = (u32)sceKernelGetEventData(&event);

			// Some types are defined in Sony's exception.h file
			// ("system/scedbgexception.h" in our source tree).  But the TM API
			// Reference doc seems to have a more complete list (see
			// eStopNotificationReason).
			struct TypeStrPair
			{
				u32         type;
				const char *str;
			};
			static const TypeStrPair typeStrPairs[] =
			{
				{ 0xa0d0c001, "SCE_DBG_EXCEPTION_GPU_FAULT_ASYNC"                               },
				{ 0xa0d04002, "SCE_DBG_EXCEPTION_GPU_HP3D_TIMEOUT_ASYNC"                        },
				{ 0xa0d04003, "SCE_DBG_EXCEPTION_GPU_SUBMITDONE_TIMEOUT_ASYNC"                  },
				{ 0xa0d0c004, "SCE_DBG_EXCEPTION_GPU_BREAK_ASYNC"                               },
				{ 0xa0d0c005, "SCE_DBG_EXCEPTION_GPU_FAULT_PAGE_FAULT_ASYNC"                    },
				{ 0xa0d0c006, "SCE_DBG_EXCEPTION_GPU_FAULT_BAD_OP_CODE_ASYNC"                   },
				{ 0xa0d0c007, "SCE_DBG_EXCEPTION_GPU_FAULT_SUBMITDONE_TIMEOUT_IN_RUN_ASYNC"     },
				{ 0xa0d0c008, "SCE_DBG_EXCEPTION_GPU_FAULT_SUBMITDONE_TIMEOUT_IN_SUSPEND_ASYNC" },
				{ 0xa0d0c009, "SCE_DBG_EXCEPTION_CPU_FAULT_SUBMITDONE_TIMEOUT_IN_RUN_ASYNC"     },
				{ 0xa0d0c00a, "SCE_DBG_EXCEPTION_CPU_FAULT_SUBMITDONE_TIMEOUT_IN_SUSPEND_ASYNC" },
				{ 0xa0d0c00b, "SCE_DBG_EXCEPTION_GPU_FAULT_UNKNOWN_ASYNC"                       },
			};
			const char *str = "???";
			for (unsigned i=0; i<NELEM(typeStrPairs); ++i)
			{
				if (type == typeStrPairs[i].type)
				{
					str = typeStrPairs[i].str;
					break;
				}
			}
			Print(PREFIX "*** GPU EVENT %s CAUGHT" SUFFIX "\n", str);

			// Disable all registerred exception handlers so that the when the
			// GPU error handling code kicks in, it will trigger a coredump.
			for (unsigned i=0; i<NELEM(s_ExceptionTypeValues); ++i)
			{
				ret = sceDbgRemoveExceptionHandler(s_ExceptionTypeValues[i]);
				if (ret != SCE_OK)
				{
					diagLoggedPrintf(PREFIX "WARNING: Failed to remove exception handler for %s, error 0x%08x." SUFFIX "\n", s_ExceptionTypeStrings[i], ret);
				}
			}

			if (GpuExceptionCallback)
			{
				GpuExceptionCallback();
			}

			// Disable output since any further spam is useless and just
			// confuses things by hiding the exception handler output.  Output
			// will be forcably ebabled again in ExceptionHandler once the GPU
			// timeout occurs.
			OUTPUT_ONLY(diagChannel::SetOutput(false);)

#	        if SYSTMCMD_ENABLE
				if (!sysBootManager::IsDevkit() && PARAM_rockstartargetmanager.Get())
				{
					sysTmCmdExceptionHandlerEnd("", s_BugstarConfig);
				}
#			endif

			FlushLogFile();
		}
	}

	void Init()
	{
		const bool isDevkit = sysBootManager::IsDevkit();

		// TODO: Decide whether or not this should be enabled with R*TM on a
		// devkit.  Could possibly simplify code by having all the handling done
		// target side.  Though things like hang detection callstacks could be a
		// problem.  Obviously if it is possible, then that would have been the
		// best way to implement things all along, but took a year of hassling
		// Sony to finally get access to the exception handler :(
		//
#		if SYSTMCMD_ENABLE
			if (isDevkit && PARAM_rockstartargetmanager.Get())
			{
				s_IsEnabled = true;
				return;
			}
#		endif

		int ret;

		if (!sysBootManager::IsDebuggerPresent() || IsForceExceptions())
		{
			for (unsigned i=0; i<NELEM(s_ExceptionTypeValues); ++i)
			{
				ret = sceDbgInstallExceptionHandler(s_ExceptionTypeValues[i], ExceptionHandler);
				if (ret != SCE_OK)
				{
					Quitf("Failed installing exception handler for %s, error 0x%08x", s_ExceptionTypeStrings[i], ret);
				}
			}
			s_IsEnabled = true;
		}
		else
		{
			Memoryf("EXCEPTION HANDLING: debugger detected, not installing.\n");
			Assert(!s_IsEnabled);
		}

		// The condition for enabling GPU exception handling is a bit different.
		// We DO want to install it when running under a debugger.  There is
		// pretty much nothing than can be quickly seen looking in the debugger,
		// instead the automated command buffer validation, etc, is way more
		// useful.
		//
		// GPU exception handling looks to have first been added in firmware
		// 1.75.  Note that this is not tied to the SDK the code is compiled
		// with.  So detect the firmware version number, and initialize if
		// available.  Though annoyingly, we can't query the version number
		// on a testkit.
		//
		bool gpuExceptionHandling;
		if (isDevkit)
		{
			// Version number string format is apparently " x.xxx.xxx", though
			// haven't seen that documented anywhere.
			char verStr[SCE_SYSTEM_SOFTWARE_VERSION_LEN+1];
			ret = sceDbgGetSystemSwVersion(verStr, NELEM(verStr));
			if (ret != SCE_OK)
			{
				Quitf("Failed to get system software version, error 0x%08x", ret);
			}
			Assert(verStr[0]==' ' && verStr[2]=='.' && verStr[6]=='.' && verStr[10]=='\0');
			const u32 ver = ((verStr[1]-'0')<<24) | ((verStr[3]-'0')<<20) | ((verStr[4]-'0')<<16) | ((verStr[5]-'0')<<12)
				| ((verStr[7]-'0')<<8) | ((verStr[8]-'0')<<4) | (verStr[9]-'0');
			gpuExceptionHandling = (ver >= 0x1750000);
			if (!gpuExceptionHandling)
			{
				Errorf("PS4 firmware version \"%s\" older than 1.75, not installing gpu exception handler", verStr);
			}
		}
		else
		{
			gpuExceptionHandling = true;
		}
		if (gpuExceptionHandling)
		{
			ret = sceKernelCreateEqueue(&s_GpuEventQueue, "GPU event queue");
			if (ret != SCE_OK)
			{
				Quitf("Failed to create gpu event queue, error 0x%08x", ret);
			}

			if (sysIpcCreateThread(GpuEventHandlerThread, NULL, 0x10000, PRIO_HIGHEST, "[RAGE] GPU event handler") == sysIpcThreadIdInvalid)
			{
				Quitf("Failed to create gpu event handler thread");
			}

			ret = sceDbgAddGpuExceptionEvent(s_GpuEventQueue, NULL);
			if (ret != SCE_OK)
			{
				if (isDevkit)
				{
					Quitf("Failed to register gpu event queue, error 0x%08x", ret);
				}
				else
				{
					Errorf("PS4 testkit failed to register gpu event queue, is installed firmware older than 1.75 ?");
				}
			}
		}
	}


#elif __WIN32

	static void SanitizePath(TCHAR *szPath)
	{
		// Convert '/' to '\\'
		TCHAR *p = szPath;
		TCHAR c = *p;
		do
		{
			if (c == TEXT('/'))
			{
				*p = TEXT('\\');
			}
		}
		while ((c = *++p) != TEXT('\0'));

		// Make sure path does not end in '\\'
		if (p[-1] == TEXT('\\'))
		{
			p[-1] = TEXT('\0');
		}
	}

	// Based on example from http://msdn.microsoft.com/en-gb/library/windows/desktop/ee416349(v=vs.85).aspx
	// NB: Returned pointer is to static storage - will change on subsequent calls to this function!
	static const TCHAR* GenerateDump(EXCEPTION_POINTERS* pExceptionPointers, bool miniDump)
	{
		BOOL bMiniDumpSuccessful;
		static TCHAR szFileName[MAX_PATH+1];
		TCHAR* szAppName;
		const char* const argv0 = sysParam::GetProgramName();
		const char* const lastBackslash = strrchr(argv0, '\\');
		const char* const lastDot = strrchr(argv0, '.');
		if (lastBackslash==NULL || lastDot<=lastBackslash)
		{
			szAppName = TEXT("game");
		}
		else
		{
			const ptrdiff_t numCharsIncludingNull = lastDot-lastBackslash;
			szAppName = (TCHAR*)alloca(sizeof(TCHAR)*numCharsIncludingNull);
			if (sizeof(TCHAR) == 1)
			{
				memcpy(szAppName, lastBackslash+1, numCharsIncludingNull-1);
				szAppName[numCharsIncludingNull-1] = '\0';
			}
			else
			{
				memset(szAppName, 0, sizeof(TCHAR)*numCharsIncludingNull);
				for (int i=0; i<numCharsIncludingNull-1; ++i)
				{
					szAppName[i] = lastBackslash[i+1];
				}
			}
		}

		HANDLE hDumpFile;
		SYSTEMTIME stLocalTime;
		MINIDUMP_EXCEPTION_INFORMATION ExpParam;

#		if !__FINAL && !__NO_OUTPUT
			if (PARAM_nominidump.Get())
			{
				return TEXT("");
			}
#		endif

#		if __WIN32PC

			TCHAR szPathBuf[MAX_PATH+1];
			const TCHAR *szPath;

			// Look for RAGE_CRASHDUMP_DIR environment variable.  Note the -1's and
			// an unsigned comparison take care of the case of
			// GetEnvironmentVariable returning zero (ie, failing).
			if (GetEnvironmentVariable(TEXT("RAGE_CRASHDUMP_DIR"), szPathBuf, NELEM(szPathBuf))-1 < NELEM(szPathBuf)-1)
			{
				SanitizePath(szPathBuf);
				szPath = szPathBuf;
			}
			else
			{
				// Failed.  Default to "x:\dumps" instead.
				// This is the same default as used by R*TM.
				szPath = TEXT("x:\\dumps");
			}

		    if (!CreateDirectory(szPath, NULL))
			{
				DWORD err = GetLastError();
				if (err != ERROR_ALREADY_EXISTS)
				{
					diagLoggedPrintf(PREFIX "Failed to create directory \"%s\" (error=0x%08x)" SUFFIX "\n", /*WIN32PC_ONLY(T2A)*/(szPath), err);
					return NULL;
				}
			}

#		else

			const TCHAR szPath[] = "d:";

#		endif

		GetLocalTime(&stLocalTime);

		// WARNING: Do not rename the dmp file on XB1 without changing the
		// appropriate code in R*TM.  R*TM relies on this filename being
		// formatted a particular way.
#		define Lwhich_snprintf_     _snwprintf
#		define which_snprintf_      _snprintf
#		define which_snprintf       TEXT(which_snprintf_)
		which_snprintf(szFileName, MAX_PATH, TEXT("%s\\%s-%04d%02d%02d-%02d%02d%02d-%s%ld.dmp"),
			szPath, szAppName,
			stLocalTime.wYear, stLocalTime.wMonth,  stLocalTime.wDay,
			stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
#			if RSG_DURANGO
				"",
#			else
				miniDump ? "mini-" : "full-",
#			endif
			GetCurrentProcessId());
#		undef which_snprintf
#		undef which_snprintf_
#		undef Lwhich_snprintf_

		hDumpFile = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
		if (hDumpFile == INVALID_HANDLE_VALUE)
		{
			const u32 hresult = GetLastError();
			struct HresultStringPair
			{
				u32         m_hresult;
				const char *m_string;
			};
#			define HRESULT_STRING_PAIR(CODE)    { CODE, #CODE }
			static const HresultStringPair s_hrStrPairs[] =
			{
				HRESULT_STRING_PAIR(ERROR_DISK_FULL),
			};
#			undef HRESULT_STRING_PAIR
			const char *str = "?";
			for (unsigned i=0; i<NELEM(s_hrStrPairs); ++i)
			{
				if (s_hrStrPairs[i].m_hresult == hresult)
				{
					str = s_hrStrPairs[i].m_string;
					break;
				}
			}
			Print(PREFIX "Failed to open dump file \"%s\" (HRESULT=0x%08x %s)" SUFFIX "\n", /*WIN32PC_ONLY(T2A)*/(szFileName), hresult, str);
			return NULL;
		}

		ExpParam.ThreadId = GetCurrentThreadId();
		ExpParam.ExceptionPointers = pExceptionPointers;
		ExpParam.ClientPointers = TRUE;

		Print(
			PREFIX "" SUFFIX "\n"
			PREFIX "********************************************************************************" SUFFIX "\n"
			PREFIX "" SUFFIX "\n"
			PREFIX "Generating dump file \"" DURANGO_ONLY("x") "%s\"..." SUFFIX "\n"
#			if RSG_DURANGO
				PREFIX "" SUFFIX "\n"
				PREFIX "This step takes approximately 10 minutes (with no progress updates!)." SUFFIX "\n"
				PREFIX "" SUFFIX "\n"
				PREFIX "IF THIS CRASH IS A VALID BUG, PLEASE DO NOT REBOOT CONSOLE DURING THIS TIME." SUFFIX "\n"
#			endif
			PREFIX "" SUFFIX "\n"
			PREFIX "********************************************************************************" SUFFIX "\n"
			PREFIX "" SUFFIX "\n"
			, /*WIN32PC_ONLY(T2A)*/(szFileName));

			MINIDUMP_TYPE dumpType;
			if(miniDump)
			{
				dumpType = (MINIDUMP_TYPE)(
					MiniDumpNormal
				);
			}
			else
			{
				dumpType = (MINIDUMP_TYPE)(
					  MiniDumpWithFullMemory
					| MiniDumpWithHandleData
					| MiniDumpWithProcessThreadData
					| MiniDumpWithFullMemoryInfo
					| MiniDumpWithThreadInfo
#				  if RSG_DURANGO
					| MiniDumpIgnoreInaccessibleMemory
					| MiniDumpWithModuleHeaders
#				  endif
#				  if RSG_PC
					| MiniDumpWithUnloadedModules
#				  endif
				);
			}
#		if RSG_DURANGO
#			define MiniDumpWriteDump MiniDumpWriteDump_FnPtr
#		endif
		bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hDumpFile, dumpType, pExceptionPointers ? &ExpParam : NULL, NULL, NULL);
#		if RSG_DURANGO
#			undef MiniDumpWriteDump
#		endif
		const DWORD dmpError = GetLastError();
		CloseHandle(hDumpFile);
#		if RSG_PC && !__FINAL && !__NO_OUTPUT
			if (!PARAM_unattended.Get())
			{
				system("pause"); // stop so we can see the tty
			}
#		endif
		if (bMiniDumpSuccessful)
		{
			// When R*TM is running, signal that to copy the dumpfile since
			// SysTrayRFS will often disconnect by now, and cause a second
			// crash.
#			if SYSTMCMD_ENABLE
				if (PARAM_rockstartargetmanager.Get())
				{
					sysTmCmdExceptionHandlerEnd(szFileName, s_BugstarConfig);
					return szFileName;
				}
#			endif
#			if RSG_DURANGO

				Print(PREFIX "Dump created on target XBox One" SUFFIX "\n");

				char szHostPcPath[MAX_PATH+1];
				char exec[2*MAX_PATH+64];

				// Check that the RS_TOOLSROOT environment variable is set for
				// SysTrayRFS running on the host PC.  Without it, we won't be
				// able to copy the coredump.
				if (!sysGetEnv("RS_TOOLSROOT", szHostPcPath, NELEM(szHostPcPath)))
				{
					Print(PREFIX "Failed.  RS_TOOLSROOT environment variable not set.  Please manually copy the coredump off the console." SUFFIX "\n");
					return NULL;
				}

				// Next check the that the batch file to perform the copy is
				// available.
				if (sysExec("if exist %RS_TOOLSROOT%\\script\\util\\durango_copy_coredump.bat ( exit 0 ) else ( exit 1 )") != 0)
				{
					Print(PREFIX "Failed.  \"%%RS_TOOLSROOT%%\\script\\util\\durango_copy_coredump.bat\" does not exist (RS_TOOLSROOT=\"%s\").  Please manually copy the coredump off the console." SUFFIX "\n", szHostPcPath);
					return NULL;
				}

				// Look for RAGE_CRASHDUMP_DIR environment variable.
				if (sysGetEnv("RAGE_CRASHDUMP_DIR", szHostPcPath, NELEM(szHostPcPath)))
				{
					SanitizePath(szHostPcPath);
				}
				else
				{
					// Failed.  Use "x:\\dumps" as the default.
					strcpy(szHostPcPath, "x:\\dumps");
				}

				snprintf(exec, NELEM(exec)-1, "%%RS_TOOLSROOT%%\\script\\util\\durango_copy_coredump.bat x%s %s", szFileName, szHostPcPath);
				Print(PREFIX "Copying dump to host PC \"%s\\%s\"" SUFFIX "\n", szHostPcPath, szFileName+3);
				if (sysExec(exec) != 0)
				{
					Print(PREFIX "Failed.  Please manually copy the coredump off the console." SUFFIX "\n");
					return NULL;
				}

				Print(PREFIX "Successful" SUFFIX "\n");
				return szFileName;

#			else // __WIN32PC

				Print(PREFIX "Successful" SUFFIX "\n");
				return szFileName;

#			endif
		}
		else
		{
			Print(
				PREFIX "" SUFFIX "\n"
				PREFIX "MiniDumpWriteDump failed (HRESULT=0x%08x)" SUFFIX "\n"
				PREFIX "" SUFFIX "\n"
				, dmpError);
#			if RSG_DURANGO
				if (dmpError == (DWORD)HRESULT_FROM_WIN32(ERROR_WRITE_PROTECT))
				{
					Print(
						PREFIX "" SUFFIX "\n"
						PREFIX "Did you reboot the console during coredump generation ?" SUFFIX "\n"
						PREFIX "" SUFFIX "\n");
				}
				else if (dmpError == (DWORD)HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY))
				{
					// Show free memory
					MEMORYSTATUSEX ms;
					ms.dwLength = sizeof(MEMORYSTATUSEX);
					GlobalMemoryStatusEx(&ms);
					Print(PREFIX "Free memory: (%u KB) %u bytes physical, (%u KB) %u bytes virtual, load: %d" SUFFIX "\n",
						(u32)(ms.ullAvailPhys / 1024), (u32)(ms.ullAvailPhys), (u32)(ms.ullAvailVirtual/ 1024), (u32)(ms.ullAvailVirtual),
						ms.dwMemoryLoad);
				}
#			endif
#			if SYSTMCMD_ENABLE
				if (PARAM_rockstartargetmanager.Get())
				{
					sysTmCmdExceptionHandlerEnd(NULL, s_BugstarConfig);
				}
#			endif
			return NULL;
		}
	}

	static long __stdcall ExceptionHandler(EXCEPTION_POINTERS *ep)
	{
#		if RSG_PC
			OutputDebugString("ExceptionHandler invoked!\n");
#		endif

		// Lock the critical section token because it's possible for another thread to have an exception while
		// the first exception is still being handled.
		s_Mutex.Lock();

		// Don't spam up the output with hang detection countdown warnings.
		HANG_DETECT_SAVEZONE_ENTER();

		// Make sure output is enabled so that we actually print the exception
		// handler info.
		OUTPUT_ONLY(diagChannel::SetOutput(true);)
		FlushLogFile();

#		if STACK_SYMBOL_SUPPORT
			sysStack::OpenSymbolFile();
			char symName[256] = "unknown";
			u32 disp = sysStack::ParseMapFileSymbol(symName, sizeof(symName), (size_t)ep->ExceptionRecord->ExceptionAddress);
#		else
			const char *const symName = "nosymbols";
			const u32 disp = 0;
#		endif

#		if SYSTMCMD_ENABLE
			if (PARAM_rockstartargetmanager.Get())
			{
				sysTmCmdExceptionHandlerBegin();
			}
#		endif

		Print(PREFIX "");
		if (PreExceptionDisplay)
		{
			PreExceptionDisplay((size_t)ep->ExceptionRecord->ExceptionAddress);
		}

		Print(PREFIX "*** EXCEPTION %x CAUGHT at address %p" SUFFIX "\n", ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress);

		ExceptionDisplayStackLine((size_t)ep->ExceptionRecord->ExceptionAddress, symName, disp);

#		if EXCEPTION_DISPLAY_REGISTERS
			CONTEXT *c = ep->ContextRecord;
			CONTEXT cCopy = *c;
#			if !__64BIT
				Print("\n" PREFIX "EAX %08x   EBX %08x" SUFFIX "\n",   cCopy.Eax,cCopy.Ebx);
				Print(     PREFIX "ECX %08x   EDX %08x" SUFFIX "\n",   cCopy.Ecx,cCopy.Edx);
				Print(     PREFIX "ESI %08x   EDI %08x" SUFFIX "\n",   cCopy.Esi,cCopy.Edi);
				Print(     PREFIX "ESP %08x   EBP %08x" SUFFIX "\n\n", cCopy.Esp,cCopy.Ebp);
#			else
				Print("\n" PREFIX "RAX %08x%08x   R8  %08x%08x" SUFFIX "\n",   (u32)(cCopy.Rax>>32), (u32)cCopy.Rax, (u32)(cCopy.R8 >>32), (u32)cCopy.R8 );
				Print(     PREFIX "RBX %08x%08x   R9  %08x%08x" SUFFIX "\n",   (u32)(cCopy.Rbx>>32), (u32)cCopy.Rbx, (u32)(cCopy.R9 >>32), (u32)cCopy.R9 );
				Print(     PREFIX "RCX %08x%08x   R10 %08x%08x" SUFFIX "\n",   (u32)(cCopy.Rcx>>32), (u32)cCopy.Rcx, (u32)(cCopy.R10>>32), (u32)cCopy.R10);
				Print(     PREFIX "RDX %08x%08x   R11 %08x%08x" SUFFIX "\n",   (u32)(cCopy.Rdx>>32), (u32)cCopy.Rdx, (u32)(cCopy.R11>>32), (u32)cCopy.R11);
				Print(     PREFIX "RSP %08x%08x   R12 %08x%08x" SUFFIX "\n",   (u32)(cCopy.Rsp>>32), (u32)cCopy.Rsp, (u32)(cCopy.R12>>32), (u32)cCopy.R12);
				Print(     PREFIX "RBP %08x%08x   R13 %08x%08x" SUFFIX "\n",   (u32)(cCopy.Rbp>>32), (u32)cCopy.Rbp, (u32)(cCopy.R13>>32), (u32)cCopy.R13);
				Print(     PREFIX "RSI %08x%08x   R14 %08x%08x" SUFFIX "\n",   (u32)(cCopy.Rsi>>32), (u32)cCopy.Rsi, (u32)(cCopy.R14>>32), (u32)cCopy.R14);
				Print(     PREFIX "RDI %08x%08x   R15 %08x%08x" SUFFIX "\n\n", (u32)(cCopy.Rdi>>32), (u32)cCopy.Rdi, (u32)(cCopy.R15>>32), (u32)cCopy.R15);
#			endif
#		endif

		PVOID trace[32] = {0};
#		if RSG_DURANGO
			const int count = RtlCaptureStackBackTrace(0, 32, trace, NULL);
#		else
			const int count = CaptureStackBackTrace(0, 32, trace, NULL);
#		endif
		for (int i=0; i<count; i++)
		{
#			if STACK_SYMBOL_SUPPORT
				disp = sysStack::ParseMapFileSymbol(symName, sizeof(symName), (size_t)trace[i]);
#			endif
			ExceptionDisplayStackLine((size_t)trace[i], symName, disp);
		}

		FlushLogFile();

		Print(PREFIX "\n");
		if (GameExceptionDisplay)
		{
			GameExceptionDisplay();
		}

		FlushLogFile();

		Print(PREFIX "");
		if (PostExceptionDisplay)
		{
			PostExceptionDisplay();
		}

		Print(PREFIX "\n");
#		if RSG_PC
#			define Lwhich_snprintf_     _snwprintf
#			define which_snprintf_      _snprintf
#			define which_snprintf       TEXT(which_snprintf_)

			// Create a mini-dump first
			static TCHAR minidumpFilename[MAX_PATH];
			const TCHAR* dumpPath = GenerateDump(ep, true);
			if(dumpPath)
			{
				which_snprintf(minidumpFilename, NELEM(minidumpFilename), TEXT("%s"), dumpPath);
			}
			else
			{
				minidumpFilename[0] = 0;
			}
			dumpPath =
#endif
				GenerateDump(ep, false);

		// Disable output since any further spam is useless and just confuses things
		// by hiding the exception handler output.
		OUTPUT_ONLY(diagChannel::SetOutput(false);)

		FlushLogFile();

#		if STACK_SYMBOL_SUPPORT
			sysStack::CloseSymbolFile();
#		endif

#		if RSG_DURANGO
			for (;;) {}
#		else
			// PC crash dumps without R*TM: Display a message box for QA
#			if RSG_PC
				static TCHAR buff[MAX_PATH+256];
#				if SYSTMCMD_ENABLE
					if (PARAM_rockstartargetmanager.Get())
					{
						// R*TM attached, it'll handle everything for us
					}
				else
#				endif
				{
					// R*TM not attached, display a message
					if(dumpPath && minidumpFilename[0])
					{
						// Generated both dumps
						which_snprintf(buff, NELEM(buff), TEXT("The game has crashed and a crash dump has been produced.\nPlease attach %s and %s to the bug."),
							minidumpFilename, dumpPath);
					}
					else if(minidumpFilename[0])
					{
						// Only mini-dump
						which_snprintf(buff, NELEM(buff), TEXT("The game has crashed and a crash dump has been produced.\nPlease attach %s to the bug."),
							minidumpFilename);
					}
					else if(dumpPath)
					{
						// Only full dump
						which_snprintf(buff, NELEM(buff), TEXT("The game has crashed and a crash dump has been produced.\nPlease attach %s to the bug."),
							dumpPath);
					}
					else
					{
						// No dumps written!
						which_snprintf(buff, NELEM(buff), TEXT("The game has crashed though the dump failed to write. Please run dbgview for more information."));
					}
					MessageBox(NULL, buff, TEXT("Crashed!"), MB_OK | MB_ICONEXCLAMATION);
				}
#				undef which_snprintf
#				undef which_snprintf_
#				undef Lwhich_snprintf_
#			endif
			diagTerminate();
			return EXCEPTION_CONTINUE_SEARCH;
#		endif
	}

	void Init()
	{
#		if RSG_DURANGO
			// MiniDumpWriteDump now needs to be loaded from "toolhelpx",
			//  https://forums.xboxlive.com/AnswerPage.aspx?qid=685c102e-3166-4f58-8a14-9ee6051246c2&tgt=1
			//
			// Only unicode LoadLibrary variant can be used,
			//  https://forums.xboxlive.com/AnswerPage.aspx?qid=9481116b-057f-4860-9875-b07ea596cc79&tgt=1
			//
			HINSTANCE dll = LoadLibraryW(L"toolhelpx");
			if (!dll)
			{
				Quitf("Unable to load toolhelpx");
			}
			MiniDumpWriteDump_FnPtr = (MiniDumpWriteDump_FnType*)GetProcAddress(dll, "MiniDumpWriteDump");
			if (!MiniDumpWriteDump_FnPtr)
			{
				Quitf("Unable to find MiniDumpWriteDump entry point in toolhelpx");
			}

			// Ensure there is enough hdd space available so that we will be able to
			// create a dump if the game crashes.
			ULARGE_INTEGER freeBytesAvailable;
			PULARGE_INTEGER lpTotalNumberOfBytes = NULL;
			PULARGE_INTEGER lpTotalNumberOfFreeBytes = NULL;
			if (!GetDiskFreeSpaceExW(L"d:\\", &freeBytesAvailable, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes))
			{
				Quitf("GetDiskFreeSpaceEx failed, hresult 0x%08x", GetLastError());
			}
			const u64 requiredSpaceGB = 8;  // GB, not GiB, since dealing with HDD space
			const u64 requiredSpaceBytes = requiredSpaceGB*1000000000uLL;
			if (freeBytesAvailable.QuadPart < requiredSpaceBytes)
			{
				Quitf("XB1 HDD full, at least %u GB space must be freed up before game will run", requiredSpaceGB);
			}

#		endif

		// if debugger is present, don't handle
		// exceptions.  This gets around the annoying
		// problem where people forget to set certain
		// exceptions to trigger when they're running
		// in the debugger:
		if (!sysBootManager::IsDebuggerPresent() || IsForceExceptions())
		{
			// TODO: There is currently no Durango API to do the exception handling
			// work in the Rockstar Target Manager.  Remove the Durango check here
			// when it is supported.
			//
			// For 360, if the console is not a devkit, then R*TM is much more
			// limited.  Need to do exception handling on the console in this case.
			//
			if(!BREAKPAD_ENABLED && (RSG_DURANGO || (1 SYSTMCMD_ENABLE_ONLY(&& !PARAM_rockstartargetmanager.Get()))))
			{
				SetUnhandledExceptionFilter(ExceptionHandler);
				Displayf("Unhandled exception filter installed.");
			}
			s_IsEnabled = true;
		}
		else
		{
			Memoryf("EXCEPTION HANDLING: debugger detected, not installing.\n");
			Displayf("Unhandled exception filter NOT installed!");
		}
	}

#	if RSG_DURANGO

		bool GenerateCrashDump()
		{
			// Generate a dummy exception context. While this isn't really required, it gives us information
			// about this thread's context, and more information is usually a good thing...
			CONTEXT ctx;
			ctx.ContextFlags = CONTEXT_ALL;
			RtlCaptureContext(&ctx);

			EXCEPTION_RECORD er;
			er.ExceptionCode = EXCEPTION_SINGLE_STEP;
			er.ExceptionFlags = 0;
			er.ExceptionRecord = NULL;
			er.ExceptionAddress = (void*)ctx.Rip;
			er.NumberParameters = 0;

			EXCEPTION_POINTERS ep;
			ep.ExceptionRecord = &er;
			ep.ContextRecord = &ctx;

			diagLoggedPrintf(PREFIX "Generating crash dump..." SUFFIX "\n");
			FlushLogFile();
			HANG_DETECT_SAVEZONE_ENTER();
			bool ret = GenerateDump(&ep, false) != NULL;
			HANG_DETECT_SAVEZONE_EXIT(NULL);
			diagLoggedPrintf(PREFIX "Crash dump complete." SUFFIX "\n");
			FlushLogFile();
			return ret;
		}

#	endif // RSG_DURANGO

#endif // __WIN32

}
// namespace sysException

}
// namespace rage

#endif // EXCEPTION_HANDLING

#if BACKTRACE_ENABLED

#include <time.h>
#include "diag/channel.h"
#include "system/memory.h"
#include "system/nelem.h"

// Set up channel for sysException class
RAGE_DEFINE_CHANNEL(exception)
#define exceptionAssert(cond)                     RAGE_ASSERT(exception,cond)
#define exceptionAssertf(cond,fmt,...)            RAGE_ASSERTF(exception,cond,fmt,##__VA_ARGS__)
#define exceptionVerify(cond)                     RAGE_VERIFY(exception,cond)
#define exceptionVerifyf(cond,fmt,...)            RAGE_VERIFYF(exception,cond,fmt,##__VA_ARGS__)
//#define exceptionFatalf(fmt,...)                  RAGE_FATALF(exception,fmt,##__VA_ARGS__)
#define exceptionErrorf(fmt,...)                  RAGE_ERRORF(exception,fmt,##__VA_ARGS__)
#define exceptionWarningf(fmt,...)                RAGE_WARNINGF(exception,fmt,##__VA_ARGS__)
#define exceptionDisplayf(fmt,...)                RAGE_DISPLAYF(exception,fmt,##__VA_ARGS__)
#define exceptionDebugf1(fmt,...)                 RAGE_DEBUGF1(exception,fmt,##__VA_ARGS__)
#define exceptionDebugf2(fmt,...)                 RAGE_DEBUGF2(exception,fmt,##__VA_ARGS__)
#define exceptionDebugf3(fmt,...)                 RAGE_DEBUGF3(exception,fmt,##__VA_ARGS__)

namespace rage
{
	extern sysMemAllocator* s_pTheAllocator;
}

#if BREAKPAD_ENABLED
#	pragma push_macro("max")
#	undef max
#	include "client/windows/handler/exception_handler.h"
#	include "common/windows/http_upload.h"
#	if RSG_PC
#		include <KnownFolders.h>
#		include <shlobj.h>
#		if __OPTIMIZED
#			pragma comment(lib, "breakpad/x64/release/common.lib")
#			pragma comment(lib, "breakpad/x64/release/crash_generation_client.lib")
#			pragma comment(lib, "breakpad/x64/release/crash_report_sender.lib")
#			pragma comment(lib, "breakpad/x64/release/exception_handler.lib")
//#			pragma comment(lib, "breakpad/x64/release/processor_bits.lib")
#			pragma comment(lib, "advapi32.lib")
#		else
#			pragma comment(lib, "breakpad/x64/debug/common.lib")
#			pragma comment(lib, "breakpad/x64/debug/crash_generation_client.lib")
#			pragma comment(lib, "breakpad/x64/debug/crash_report_sender.lib")
#			pragma comment(lib, "breakpad/x64/debug/exception_handler.lib")
//#			pragma comment(lib, "breakpad/x64/debug/processor_bits.lib")
#			pragma comment(lib, "advapi32.lib")
#		endif
#	pragma comment(lib, "wininet.lib")
#	endif

namespace rage
{
	NOSTRIP_PC_PARAM(enableCrashpad, "[startup] Enable Breakpad (defaults to disabled on Master builds, use this to turn it on)");
	PARAM(testBreakpadOnStartup, "[startup] Always trigger a Breakpad dump on startup");
	PARAM(breakpadUrl, "[startup] Override URL to upload Breakpad dumps to");

#if BACKTRACE_TEST_CRASHES_ENABLED
	NOSTRIP_PARAM(breakpadMessagebox, "[breakpad] Show a messagebox when in the Breakpad crash handler, so a debugger can be attached.");
#else
	PARAM(breakpadMessagebox, "[breakpad] Show a messagebox when in the Breakpad crash handler, so a debugger can be attached.");
#endif
}

extern __THREAD int RAGE_LOG_DISABLE;

class ScopedRageLogDisabler
{
public:
	ScopedRageLogDisabler() { ++RAGE_LOG_DISABLE; }
	~ScopedRageLogDisabler() { --RAGE_LOG_DISABLE; }
};

#define DISABLE_RAGE_LOG ScopedRageLogDisabler rageLogDisabler_;

#endif // BREAKPAD_ENABLED

#if BACKTRACE_DUMP_AND_UPLOAD_ENABLED
#include "atl\map.h"
#include "atl\string.h"
#endif

#if BACKTRACE_TEST_CRASHES_ENABLED
#include "math/random.h"
#endif

namespace rage
{

namespace sysException
{
	static const char* s_backtraceSubmissionUrl = nullptr;

	static BacktraceConfig* s_backtraceConfig = nullptr;

	BacktraceConfig::BacktraceConfig(const char* token, const char* submissionUrl)
		: m_token(token), m_url(submissionUrl)
	{
		s_backtraceConfig = this;
		s_backtraceSubmissionUrl = submissionUrl;
	}

#if BACKTRACE_TEST_CRASHES_ENABLED
	static volatile u32 s_backtraceCrashCounter = 0;
#endif

#if BREAKPAD_ENABLED
	std::map<std::wstring, std::wstring>* g_pBreakpadAnnotations;
	std::map<std::wstring, std::wstring>* g_pBreakpadFiles;
	std::wstring* g_breakpadDumpPath = nullptr, *g_breakpadLogPath = nullptr;

	bool UploadBreakpadMinidump()
	{
		if (sysParam::IsInitialized() && PARAM_breakpadUrl.Get(s_backtraceSubmissionUrl))
		{
			exceptionDisplayf("[Breakpad] Overriding backtrace URL to %s\n", s_backtraceSubmissionUrl);
		}

		if (s_backtraceSubmissionUrl == nullptr)
		{
			exceptionDisplayf("[Breakpad] Unable to upload dump, submission URL is null\n");
			return false;
		}

		if (!g_pBreakpadAnnotations || !g_pBreakpadFiles)
		{
			exceptionDisplayf("[Breakpad] Unable to upload dump, annotation and file maps are null\n");
			return false;
		}

		DISABLE_RAGE_LOG;

		USES_CONVERSION;
		const char16* urlW = UTF8_TO_WIDE(s_backtraceSubmissionUrl);

		exceptionDisplayf("[Breakpad] Attempting to upload Breakpad dump\n");

		int responseCode;
		std::wstring* pResponseBody = NULL;

#if !RSG_FINAL
		std::wstring responseBody;
		pResponseBody = &responseBody;
#endif

		bool success = google_breakpad::HTTPUpload::SendMultipartPostRequest(reinterpret_cast<const wchar_t*>(urlW), *g_pBreakpadAnnotations, *g_pBreakpadFiles, NULL, pResponseBody, &responseCode);

		exceptionDisplayf("[Breakpad] Breakpad dump upload success: %s response code: %d" NOTFINAL_ONLY(" response body:\n%ls") "\n", success ? "true" : "false", responseCode NOTFINAL_ONLY(, responseBody.c_str()));

		return success;
	}

#if 0
	bool SaveBreakpadCrashContextLog()
	{
		if (!g_breakpadLogPath)
		{
			exceptionDisplayf("[Breakpad] No string allocated for log path!");
			return false;
		}

		// Handle exceptions being thrown by threads that have no current allocator set - for example, Windows worker threads
		if (!sysMemAllocator::IsCurrentSet())
		{
			sysMemAllocator::SetCurrent(sysMemAllocator::GetMaster());
		}

		// Get filename and write to the globally-allocated path wstring
		char fileName[RAGE_MAX_PATH] = { 0 };
		GetDumpBaseFilename(fileName, RAGE_MAX_PATH);

		wchar_t wPath[RAGE_MAX_PATH] = { 0 };
		formatf_wchar_t(wPath, L"%ls%hs.crash.log", g_TempLogFolder, fileName);

		// Copy to the global path
		*g_breakpadLogPath = wPath;

		// Get the data to write
		g_coredumpManager.EnsureGatherDump();
		const u8* data = g_coredumpManager.GetDumpStream();
		size_t size = g_coredumpManager.GetDumpStreamSize();

		// Open and write the file
		if (FILE* hFile = _wfopen(wPath, L"w"))
		{
			exceptionDisplayf("Breakpad - Saving %ls - %" I64FMT "u bytes\n", wPath, size);

			fwrite(data, 1, size, hFile);
			fflush(hFile);
			fclose(hFile);

			return true;
		}
		else
		{
			exceptionDisplayf("Breakpad - Failed to save %ls\n", wPath);
			return false;
		}
	}
#endif

	bool __cdecl BreakpadCrashCallback(const wchar_t* dump_path, const wchar_t* minidump_id, void* UNUSED_PARAM(context), EXCEPTION_POINTERS* UNUSED_PARAM(exinfo), MDRawAssertionInfo* UNUSED_PARAM(assertion), bool UNUSED_PARAM(succeeded))
	{
		if (PARAM_breakpadMessagebox.Get())
		{
			MessageBoxW(NULL, L"In BreakpadCrashCallback", L"Crashed", MB_OK);
		}

		// Handle exceptions being thrown by threads that have no current allocator set - for example, Windows worker threads
		if (!sysMemAllocator::IsCurrentSet())
		{
			sysMemAllocator::SetCurrent(*s_pTheAllocator);
		}

		exceptionDisplayf("[Breakpad] In Breakpad crash callback\n");

		exceptionDisplayf("[Breakpad] Dump path: %ls\n", dump_path);
		exceptionDisplayf("[Breakpad] Dump name: %ls\n", minidump_id);

		// Check the init has run
		if (!g_breakpadDumpPath || !g_breakpadLogPath)
		{
			exceptionDisplayf("[Breakpad] No strings allocated for file paths!");
			return false;
		}

		if (s_backtraceConfig)
		{
			if (s_backtraceConfig->ShouldSubmitDump(g_breakpadLogPath->c_str()))
			{
				exceptionDisplayf("[Breakpad] ShouldSubmit callback returned true, continuing.\n");
			}
			else
			{
				exceptionDisplayf("[Breakpad] Skipping Breakpad upload due to ShouldSubmit callback returning false.\n");
				return false;
			}
		}
		else
		{
			exceptionDisplayf("[Breakpad] No ShouldSubmit callback set, continuing.\n");

			// Create an empty file rather than altering the file map now, as it may not be safe to do so
			HANDLE file = CreateFileW(g_breakpadLogPath->c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (file != INVALID_HANDLE_VALUE)
			{
				const char* empty = "(empty)";
				WriteFile(file, &empty, (DWORD)strlen(empty), NULL, NULL);
				CloseHandle(file);
			}
			else
			{
				exceptionDisplayf("[Breakpad] Unable to create empty crash context log.\n");
			}
		}

		*g_breakpadDumpPath = dump_path;
		*g_breakpadDumpPath += minidump_id;
		*g_breakpadDumpPath += L".dmp";

		// Write out the crash context log
		//SaveBreakpadCrashContextLog();

		// Upload the dump, with the log (if saved successfully)
		UploadBreakpadMinidump();

		return true;
	}

	google_breakpad::ExceptionHandler* g_breakpadHandler = nullptr;

	bool SetupBreakpad()
	{
		// Re-use the Crashpad enable flag, as RGL already supports this
		if (!PARAM_enableCrashpad.Get())
		{
			exceptionDebugf1("[Breakpad] Not setting up Breakpad due to missing command-line arg\n");
			return false;
		}

		exceptionDebugf1("[Breakpad] Setting up Breakpad\n");

		DISABLE_RAGE_LOG;

		google_breakpad::ExceptionHandler::MinidumpCallback callback = &BreakpadCrashCallback;

#if RSG_DURANGO
		wchar_t* dumpPath = L"T:\\";
#elif RSG_PC
		wchar_t dumpPath[RAGE_MAX_PATH];
		PWSTR pwszPath = NULL;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pwszPath)))
		{
			formatf(dumpPath, L"%ls\\", pwszPath);
			CoTaskMemFree(pwszPath);

			wchar_t* path[] = { L"Rockstar Games", L"GTAV", L"CrashLogs" };
			for (int i = 0; i < NELEM(path); ++i)
			{
				// Append path and try to create directory if it doesn't exist
				safecatf(dumpPath, L"%ls\\", path[i]);
				CreateDirectoryW(dumpPath, NULL);
			}
		}
#endif
		g_breakpadHandler = rage_new google_breakpad::ExceptionHandler(dumpPath, nullptr, callback, nullptr, google_breakpad::ExceptionHandler::HANDLER_ALL);
		g_breakpadHandler->set_handle_debug_exceptions(true);

		g_pBreakpadAnnotations = rage_new std::map<std::wstring, std::wstring>();
		g_pBreakpadFiles = rage_new std::map<std::wstring, std::wstring>();

		// Set up the files map in advance to avoid memory allocation during crash handling
		g_breakpadDumpPath = &((*g_pBreakpadFiles)[L"upload_file_minidump"]);
		g_breakpadLogPath = &((*g_pBreakpadFiles)[L"crashcontext.log"]);
		g_breakpadDumpPath->reserve(RAGE_MAX_PATH);
		g_breakpadLogPath->reserve(RAGE_MAX_PATH);

		// Set up the crash log path in advance
		*g_breakpadLogPath = dumpPath;
		*g_breakpadLogPath += L"crashcontext.log";

		exceptionDebugf1("[Breakpad] Set up Breakpad\n");

		if (PARAM_testBreakpadOnStartup.Get())
		{
			g_breakpadHandler->WriteMinidump();
		}

#if BACKTRACE_TEST_CRASHES_ENABLED
		SetupForTestCrashes();
#endif


//#if RSG_SCARLETT
	//	SetAnnotation("platform", "scarlett");
#if RSG_PC
		SetAnnotation("platform", "PC");
#elif RSG_DURANGO
		SetAnnotation("platform", "durango");
#endif

		if (s_backtraceConfig)
		{
			s_backtraceConfig->OnSetup();
		}

		return true;
	}

	void DisableBacktrace()
	{
		if (g_breakpadHandler)
		{
			// Shutdown logic is in the exception handler destructor, so delete the object
			delete g_breakpadHandler;
			g_breakpadHandler = nullptr;
		}
	}

	void SetAnnotation(const char* key, const char* value, size_t UNUSED_PARAM(bufferSize))
	{
		if (!g_pBreakpadAnnotations)
			return;

		USES_CONVERSION;
		const char16* keyW = UTF8_TO_WIDE(key);
		const char16* valueW = UTF8_TO_WIDE(value);
		(*g_pBreakpadAnnotations)[std::wstring(reinterpret_cast<const wchar_t*>(keyW))] = std::wstring(reinterpret_cast<const wchar_t*>(valueW));
	}

	void SetAnnotation(const char* key, int value)
	{
		char buffer[12];
		formatf(buffer, "%d", value);
		SetAnnotation(key, buffer, sizeof(buffer));
	}

	void SetAnnotation(const char* key, u32 value)
	{
		char buffer[12];
		formatf(buffer, "%u", value);
		SetAnnotation(key, buffer, sizeof(buffer));
	}

#endif // BREAKPAD_ENABLED
	static BacktraceScriptData s_btScriptData;

	void BacktraceNotifyScriptLaunched(u32 scriptProgramId, u32 currentTime)
	{
		s_btScriptData.m_LastLaunchedScriptProgramId = scriptProgramId;
		s_btScriptData.m_TimeSinceLastLaunchedScript = currentTime;
	}

	BacktraceScriptData BacktraceGetLastLaunchedScriptData()
	{
		return s_btScriptData;
	}

#if BACKTRACE_DUMP_AND_UPLOAD_ENABLED

	static atMap<atString, atString>* g_backtraceAnnotations;
	static sysCriticalSectionToken g_backtraceCs;
	static bool g_backtraceInCrashHandler = false;

#if RSG_DURANGO
	const rage::atMap<rage::atString, rage::atString>* BacktraceConfig::GetAnnotations()
	{
		return g_backtraceAnnotations;
	}
#endif

	void SetAnnotation(const char* key, const char* value, size_t UNUSED_PARAM(bufferSize))
	{
		if (!g_backtraceAnnotations)
			return;

		atString keyString = atString(key);
		atString* existingValue = g_backtraceAnnotations->Access(keyString);
		if (existingValue)
		{
			*existingValue = value;
		}
		else
		{
			g_backtraceAnnotations->Insert(keyString, atString(value));
		}
	}

	void SetAnnotation(const char* key, int value)
	{
		char buffer[12];
		formatf(buffer, "%d", value);
		SetAnnotation(key, buffer, sizeof(buffer));
	}

	void SetAnnotation(const char* key, u32 value)
	{
		char buffer[12];
		formatf(buffer, "%u", value);
		SetAnnotation(key, buffer, sizeof(buffer));
	}

	// Helper class to ensure that we only ever have one thread in the crash handler, and it's non-recursive
	// (i.e. if a thread is handling a crash, but then crashes again, we early-out of handling the second crash)
	class CrashDumpLockHelper
	{
	public:
		CrashDumpLockHelper() : m_acquiredLock(false)
		{
			SYS_CS_SYNC(g_backtraceCs);
			if (g_backtraceInCrashHandler)
			{
				m_acquiredLock = false;
			}
			else
			{
				m_acquiredLock = true;
				g_backtraceInCrashHandler = true;
			}
		}

		bool AcquiredLock()
		{
			return m_acquiredLock;
		}

		~CrashDumpLockHelper()
		{
			if (m_acquiredLock)
			{
				SYS_CS_SYNC(g_backtraceCs);
				g_backtraceInCrashHandler = false;
			}
		}

	private:
		bool m_acquiredLock;
	};

	static bool IsDebugException(EXCEPTION_POINTERS *ep)
	{
		// Exception code for C++ exceptions.  Ignore these, as they are sometimes thrown by Windows libraries (e.g. url:bugstar:6923040)
		// and probably by third-party code too.
	#ifndef EXCEPTION_CPP
		const DWORD EXCEPTION_CPP = 0xE06D7363UL;
	#endif
	#ifndef EXCEPTION_SET_THREAD_NAME
		const DWORD EXCEPTION_SET_THREAD_NAME = 0x406D1388UL;
	#endif
	#ifndef DBG_PRINTEXCEPTION_WIDE_C
		const DWORD DBG_PRINTEXCEPTION_WIDE_C = 0x4001000AUL;
	#endif

		// Borrowed from https://github.com/google/breakpad/blob/master/src/client/windows/handler/exception_handler.cc
		switch (ep->ExceptionRecord->ExceptionCode)
		{
			/*case EXCEPTION_BREAKPOINT:*/ // Breakpoints are technically debug exceptions, but we want to catch __debugbreak() here
			case EXCEPTION_SINGLE_STEP:
			case EXCEPTION_SET_THREAD_NAME:
			case EXCEPTION_CPP:
			case DBG_PRINTEXCEPTION_C:
			case DBG_PRINTEXCEPTION_WIDE_C:
				return true;
		}
		return false;
	}

	static long __stdcall BacktraceDumpAndUploadExceptionHandler(EXCEPTION_POINTERS *ep)
	{
		exceptionDisplayf("[Backtrace] In BacktraceDumpAndUploadExceptionHandler");

	#if RSG_DURANGO && !__MASTER
		FlushLogFile();
	#endif

		// Return value to indicate that the next exception handler should be called after ours.
		// We currently always return this, no matter what happens.
		const long retVal = EXCEPTION_CONTINUE_SEARCH;

		// Skip debug exceptions
		if (IsDebugException(ep))
		{
			exceptionErrorf("[Backtrace] Skipping debug exception");
			return retVal;
		}

		// Critical section to ensure we only handle one crash at once
		CrashDumpLockHelper lockHelper;
		if (lockHelper.AcquiredLock())
		{
			exceptionDisplayf("[Backtrace] Acquired crash lock");
		}
		else
		{
			exceptionDisplayf("[Backtrace] Couldn't acquire crash lock (this or another thread is already handling an exception)");
			return retVal;
		}

		if (!s_backtraceConfig)
		{
			exceptionErrorf("[Backtrace] No Backtrace config object");
			return retVal;
		}

		// Setup the dump path and filename
		wchar_t* dumpPath = L"T:\\";
		wchar_t dumpFilename[MAX_PATH];
		wcscpy_s(dumpFilename, dumpPath);
		wcscat_s(dumpFilename, L"crash.dmp");

		wchar_t logFilename[MAX_PATH];
		wcscpy_s(logFilename, dumpPath);
		wcscat_s(logFilename, L"crashcontext.log");

		bool includeLog = false;

		if (s_backtraceConfig->ShouldSubmitDump(logFilename))
		{
			exceptionDisplayf("[Backtrace] ShouldSubmit callback returned true, continuing.\n");
			includeLog = true;
		}
		else
		{
			exceptionDisplayf("[Backtrace] Skipping Backtrace upload due to ShouldSubmit callback returning false.\n");
			return retVal;
		}

		// Adapted from GenerateCrashDump above, which is in turn based on example from http://msdn.microsoft.com/en-gb/library/windows/desktop/ee416349(v=vs.85).aspx
		HANDLE dumpFile = CreateFileW(dumpFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
		if (dumpFile == INVALID_HANDLE_VALUE)
		{
			exceptionErrorf("[Backtrace] Unable to open dump file to write to!");
			return retVal;
		}

		MINIDUMP_EXCEPTION_INFORMATION ExpParam;
		ExpParam.ThreadId = GetCurrentThreadId();
		ExpParam.ExceptionPointers = ep;
		ExpParam.ClientPointers = TRUE;

		// For now, let's stick to mini-minidumps
		bool miniDump = true;

		MINIDUMP_TYPE dumpType;
		if(miniDump)
		{
			dumpType = (MINIDUMP_TYPE)(
				MiniDumpNormal
				);
		}
		else
		{
			dumpType = (MINIDUMP_TYPE)(
				MiniDumpWithFullMemory
				| MiniDumpWithHandleData
				| MiniDumpWithProcessThreadData
				| MiniDumpWithFullMemoryInfo
				| MiniDumpWithThreadInfo
#if RSG_DURANGO
				| MiniDumpIgnoreInaccessibleMemory
				| MiniDumpWithModuleHeaders
#endif
#if RSG_PC
				| MiniDumpWithUnloadedModules
#endif
				);
		}

		bool success = (MiniDumpWriteDump_FnPtr(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, dumpType, ep ? &ExpParam : NULL, NULL, NULL) == TRUE);

		CloseHandle(dumpFile);

		if (success)
		{
			exceptionDisplayf("[Backtrace] Wrote out dump.");
			if (s_backtraceConfig->UploadDump(dumpFilename, logFilename, g_backtraceAnnotations, s_backtraceConfig->GetUrl()))
			{
				exceptionDisplayf("[Backtrace] Uploaded dump.");
			}
			else
			{
				exceptionDisplayf("[Backtrace] Failed to upload dump.");
			}
		}
		else
		{
			exceptionDisplayf("[Backtrace] Failed to write out dump.");
		}

	#if RSG_DURANGO && !__MASTER
		FlushLogFile();
	#endif

		return retVal;
	}

	void TestBacktraceDumpAndUpload()
	{
		struct Test
		{
			static void Run(void*)
			{
				// Stolen from GenerateCrashDump above...
				// Generate a dummy exception context. While this isn't really required, it gives us information
				// about this thread's context, and more information is usually a good thing...
				CONTEXT ctx;
				ctx.ContextFlags = CONTEXT_ALL;
				RtlCaptureContext(&ctx);

				EXCEPTION_RECORD er;
				er.ExceptionCode = EXCEPTION_BREAKPOINT;
				er.ExceptionFlags = 0;
				er.ExceptionRecord = NULL;
				er.ExceptionAddress = (void*)ctx.Rip;
				er.NumberParameters = 0;

				EXCEPTION_POINTERS ep;
				ep.ExceptionRecord = &er;
				ep.ContextRecord = &ctx;

				BacktraceDumpAndUploadExceptionHandler(&ep);
			}
		};
		sysIpcCreateThread(&Test::Run, (void*)0, sysIpcMinThreadStackSize, PRIO_NORMAL, "Testing Backtrace Dump And Upload", 0, "BacktraceUpload");
	}

	bool SetupBacktraceDumpAndUpload()
	{
		g_backtraceAnnotations = rage_new atMap<atString, atString>();

		exceptionDebugf1("[Backtrace] Setting up Backtrace dump-and-upload\n");

		// Based on sysException::Init(), above.
		// MiniDumpWriteDump now needs to be loaded from "toolhelpx",
		//  https://forums.xboxlive.com/AnswerPage.aspx?qid=685c102e-3166-4f58-8a14-9ee6051246c2&tgt=1
		//
		// Only unicode LoadLibrary variant can be used,
		//  https://forums.xboxlive.com/AnswerPage.aspx?qid=9481116b-057f-4860-9875-b07ea596cc79&tgt=1
		//
		HINSTANCE dll = LoadLibraryW(L"toolhelpx");
		if (!dll)
		{
			exceptionErrorf("[Backtrace] Unable to load toolhelpx\n");
			return false;
		}

		MiniDumpWriteDump_FnPtr = (MiniDumpWriteDump_FnType*)GetProcAddress(dll, "MiniDumpWriteDump");
		if (!MiniDumpWriteDump_FnPtr)
		{
			exceptionErrorf("[Backtrace] Unable to find MiniDumpWriteDump entry point in toolhelpx");
			return false;
		}

		SetUnhandledExceptionFilter(BacktraceDumpAndUploadExceptionHandler);

#if BACKTRACE_TEST_CRASHES_ENABLED
		SetupForTestCrashes();
#endif

#if RSG_DURANGO
		SetAnnotation("platform", "durango");
#else
		#error Unsupported platform for Backtrace dump-and-upload
#endif

		if (s_backtraceConfig)
		{
			s_backtraceConfig->OnSetup();
		}

		return true;
	}

	void DisableBacktrace()
	{
		SetUnhandledExceptionFilter(nullptr);

	}

#endif // BACKTRACE_DUMP_AND_UPLOAD_ENABLED

#if BACKTRACE_TEST_CRASHES_ENABLED

	void SetupForTestCrashes()
	{
		// Set up the automatic test crash counter.  Place the CRASHPAD_TEST_CRASH macro at various locations throughout the code
		// and run with -crashpadTestCrashEnable, to have the game randomly crash at one of the locations.  This allows testing
		// of Crashpad to generate different callstacks.
		if (PARAM_backtraceTestCrashEnable.Get())
		{
			static bool s_firstTime = true;
			if (s_firstTime)
			{
				s_firstTime = false;

				mthRandom rnd((int)time(NULL));
				for (int i = 0; i < 100; i++)
					rnd.GetInt();

				int crashCounterMin = 0, crashCounterMax = 1000;
				PARAM_backtraceTestCrashMin.Get(crashCounterMin);
				PARAM_backtraceTestCrashMax.Get(crashCounterMax);
				s_backtraceCrashCounter = rnd.GetRanged(crashCounterMin, crashCounterMax);
			}
		}
	}

	bool ShouldCrashForBacktraceTest()
	{
		if (!PARAM_backtraceTestCrashEnable.Get())
			return false;

		if (s_backtraceCrashCounter > 0)
		{
			u32 val = sysInterlockedDecrement(&s_backtraceCrashCounter);

			if (val == 0)
			{
// Use this for testing the crash callback function
#if BREAKPAD_ENABLED && 0
				if (g_breakpadHandler)
				{
					g_breakpadHandler->WriteMinidump();
					return true;
				}
#elif BACKTRACE_DUMP_AND_UPLOAD_ENABLED && 0
				TestBacktraceDumpAndUpload();
				return false;
#else
				return true;
#endif
			}
		}
		return false;
	}
#endif // BACKTRACE_TEST_CRASHES_ENABLED

} // namespace sysException

} // namespace rage

#if BREAKPAD_ENABLED
#pragma pop_macro("max")
#endif

#endif // BACKTRACE_ENABLED