//
// system/tmcommands.cpp
//
// Copyright (C) 2013-2015 Rockstar Games.  All Rights Reserved.
//

#include "tmcommands.h"

#if SYSTMCMD_ENABLE

#include "bootmgr.h"
#include "ipc.h"
#include "nelem.h"
#include "param.h"
#include "platform.h"
#include "xtl.h"
#include "diag/output.h"
#include "diag/channel.h"
#include "diag/trap.h"
#include "system/alloca.h"
#include "system/exception.h"
#include "system/hangdetect.h"
#include "system/namedpipe.h"
#include "system/stack.h"
#include "system/timer.h"
#include "file/tcpip.h"
#include <stdio.h>
#include <string.h>

#if RSG_XENON
#	include "xbdm.h"
#endif

#if !REDIRECT_TTY
#error R*TM now requires REDIRECT_TTY be defined.
#endif

namespace rage
{
PARAM(rockstartargetmanager,"[startup] Indicates RTM is being used.  Additional configuration parameters passed through in string.  Format is semicolon separated key value pairs.  Supported key value pairs: pdb=path;tty=ipaddr:port");

#define SYSTMINIT_WAIT_MSG                                                     \
	TRed "\n"                                                                  \
	"################################################################\n"       \
	"######## Waiting for Rockstar Target Manager connection ########\n"       \
	"################################################################\n"       \
	"\n"                                                                       \
	TNorm "\n"

#define SYSTMINIT_CONNECTED_MSG "R*TM connected.\n"
#define SYSTMINIT_TIMEOUT_MSG "R*TM connection timed out!\n"

#if RSG_DURANGO
	static char s_Pdb[256] = "xg:\\game_durango_" RSG_CONFIGURATION_LOWERCASE ".pdb";
	static char s_FullPackageName[256];
	bool g_rockstartargetmanagerConnected = false; // Used during thread construction
#endif

static char s_TtyRedirectAddr[256];
static const int RTM_TTY_PORT = 1895;

static void parseRockstarTargetManagerArgs()
{
	// Parse the additional R*TM args passed with -rockstartargetmanager
	const char *args;
	if (PARAM_rockstartargetmanager.Get(args))
	{
		Assert(args);
		while (*args != '\0')
		{
			// Copy the next ';' delimited arg
			char arg[256];
			char *p = arg;
			for (;;)
			{
				char c = *args;
				if (c == '\0')
				{
					break;
				}
				++args;
				if (c == ';')
				{
					break;
				}
				*p++ = c;
				// Non-final code, so trap is good enough here (ie,
				// don't need to protect against malicous buffer
				// overflows).
 				TrapGE((size_t)(p - arg), (size_t)NELEM(arg));
			}
			*p = '\0';

			if (memcmp(arg, "tty=", 4) == 0)
			{
				strncpy(s_TtyRedirectAddr, arg+4, sizeof(s_TtyRedirectAddr));
				s_TtyRedirectAddr[sizeof(s_TtyRedirectAddr)-1] = '\0';
			}
#if RSG_DURANGO
			else if (memcmp(arg, "pdb=", 4) == 0)
			{
				strncpy(s_Pdb, arg+4, sizeof(s_Pdb));
				s_Pdb[sizeof(s_Pdb)-1] = '\0';
			}
			else if (memcmp(arg, "fullpackagename=", 16) == 0)
			{
				strncpy(s_FullPackageName, arg+16, sizeof(s_FullPackageName));
				s_FullPackageName[sizeof(s_FullPackageName)-1] = '\0';
			}
#endif // RSG_DURANGO
		}

		// If no TTY IP was specified, default to localhost
		if (s_TtyRedirectAddr[0] == '\0')
		{
			formatf(s_TtyRedirectAddr, "%s:%d", fiDeviceTcpIp::GetLocalHost(), RTM_TTY_PORT);
		}
	}
}

void sysTmInit(bool waitConnect, u32 timeoutMs)
{
	parseRockstarTargetManagerArgs();
	if (waitConnect)
	{
		diagLoggedPrintf("Attempting to connect to R*TM on host PC at %s...\n", s_TtyRedirectAddr);
		diagLoggedPrintf(SYSTMINIT_WAIT_MSG);
		sysTimer rtmConnectTimer;
		bool connected = diagChannel::RedirectOutput(s_TtyRedirectAddr, timeoutMs);
		float connectTime = rtmConnectTimer.GetTime();

		if (connected)
		{
			diagLoggedPrintf(SYSTMINIT_CONNECTED_MSG);
			diagLoggedPrintf("Took %f seconds to connect to R*TM\n", connectTime);
#			if RSG_DURANGO
				g_rockstartargetmanagerConnected = true;
#			elif RSG_PC
				diagLoggedPrintf(">>R*TM ATTACH_PID:%u\n", ::GetCurrentProcessId());
				// Use FlushLogFile to synchronize with R*TM and ensure it has attached to our process.
				const u32 infiniteTimeoutMs = 0;
				diagChannel::FlushLogFile(infiniteTimeoutMs);
#			endif
			sysTmCmdNop();

#			if RSG_DURANGO
				// XB1 devkit hang detection relies on telling R*TM about each
				// thread.  Since this current thread has already started
				// (obviously), report it now.
				sysTmCmdThreadBegin();
#			endif
		}
		else
		{
			// Timed out, clear -rockstartargetmanager arg
			PARAM_rockstartargetmanager.Set(NULL);
			diagLoggedPrintf(SYSTMINIT_TIMEOUT_MSG);
			DURANGO_ONLY(g_rockstartargetmanagerConnected = false;)
		}

		// Flush log file so that we know this completed if we get logs!
		diagChannel::FlushLogFile();
	}
}

#if RSG_DURANGO

	extern "C" void sysTmCmdNop_Internal();
	extern "C" void sysTmCmdPrintAllThreadStacks_Internal();
	extern "C" void sysTmCmdStopNoErrorReport_Internal();
	extern "C" void sysTmCmdQuitf_Internal(const char *msg);
	extern "C" void sysTmCmdGpuHang_Internal(const wchar_t *xhitFilename);
	extern "C" void sysTmCmdCpuHang_Internal();
	extern "C" void sysTmCmdConfig_Internal(const char *config);
	extern "C" void sysTmCmdExceptionHandlerBegin_Internal();
	extern "C" void sysTmCmdExceptionHandlerEnd_Internal(const char *coredump, const void *bugstarConfig);
	extern "C" void sysTmCmdThreadBegin_Internal();
	extern "C" void sysTmCmdThreadEnd_Internal();
	extern "C" void sysTmCmdReportGpuHangHack_Internal();
	extern "C" bool sysTmCmdGetGpuCommandBufferAccess_Internal(GpuCmdBufAccess *access);

	void sysTmCmdNop()
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdNop_Internal();
		else
			diagLoggedPrintf(">>R*TM NOP\n");
	}

	void sysTmCmdPrintAllThreadStacks()
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdPrintAllThreadStacks_Internal();
		else
			diagLoggedPrintf(">>R*TM PRINT_ALL_THREAD_STACKS\n");
	}

	void sysTmCmdStopNoErrorReport()
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdStopNoErrorReport_Internal();
		else
		{
			HANG_DETECT_SAVEZONE_ENTER();
			sysStack::PrintStackTrace();
			diagLoggedPrintf(">>R*TM STOP_NO_ERROR_REPORT\n");
			for (;;) sysIpcSleep(1);
		}
	}

	void sysTmCmdQuitf(const char *msg)
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdQuitf_Internal(msg);
		else
			diagLoggedPrintf(">>R*TM QUITF:%s\n", msg);

		// Second debug break to cause a crash and run the standard exception
		// handling.  Note that the __debugbreak() can't be inside of the "array
		// of bytes as a function" hackery in tmcommands_ms.cpp, since that
		// seems to break RtlCaptureStackBackTrace.
		__debugbreak();
	}

	void sysTmCmdGpuHang(const wchar_t *xhitFilename)
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdGpuHang_Internal(xhitFilename);
		else
			diagLoggedPrintf(">>R*TM GPU_HANG:%s\n", xhitFilename);
		__debugbreak();
	}

	void sysTmCmdCpuHang()
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdCpuHang_Internal();
		else
			diagLoggedPrintf(">>R*TM CPU_HANG\n");
		__debugbreak();
	}

	void sysTmCmdConfig(const char *config)
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdConfig_Internal(config);
		else
			diagLoggedPrintf(">>R*TM CONFIG:%s\n", config);
	}

	void sysTmCmdCreateDump()
	{
#		if EXCEPTION_HANDLING
			sysException::GenerateCrashDump();
#		endif
	}

	void sysTmCmdExceptionHandlerBegin()
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdExceptionHandlerBegin_Internal();
		else
			diagLoggedPrintf(">>R*TM EXCEPTION_HANDLER_BEGIN\n");
	}

	void sysTmCmdExceptionHandlerEnd(const char *coredump, const void *bugstarConfig)
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdExceptionHandlerEnd_Internal(coredump, bugstarConfig);
		else
			diagLoggedPrintf(">>R*TM EXCEPTION_HANDLER_END:%s;%s\n", coredump, bugstarConfig?bugstarConfig:"");
	}

	void sysTmCmdThreadBegin()
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdThreadBegin_Internal();
		// It is pointless to attempt anything for this on a testkit.
	}

	void sysTmCmdThreadEnd()
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdThreadEnd_Internal();
		// It is pointless to attempt anything for this on a testkit.
	}

	void sysTmCmdReportGpuHangHack()
	{
		if (sysBootManager::IsDevkit())
			sysTmCmdReportGpuHangHack_Internal();
		// It is pointless to attempt anything for this on a testkit.
	}

	bool sysTmCmdGetGpuCommandBufferAccess(GpuCmdBufAccess *access)
	{
		if (sysBootManager::IsDevkit())
			return sysTmCmdGetGpuCommandBufferAccess_Internal(access);
		// It is pointless to attempt anything for this on a testkit.
		return false;
	}

	const char *sysTmGetPdb()
	{
		return s_Pdb;
	}

	const char *sysTmGetFullPackageName()
	{
		return s_FullPackageName;
	}

#elif RSG_ORBIS

#	define TMCMD_HEADER()                                                      \
		__asm__ __volatile__                                                   \
		(                                                                      \
			"int    $0x41\n\t"                                                 \
			"jmp    0f\n\t"                                                    \
			".ascii \"R*TM\"\n\t"                                              \
			/* .byte is currently getting ignored, so we need to encode the */ \
			/* opcode and any data using octal character constants in a     */ \
			/* .ascii string.                                               */ \
			".ascii \""

	// Constraints can be passed in as varags to ensure the compiler puts
	// command args in the correct registers.
#	define TMCMD_FOOTER(...)                                                   \
					   "\"\n\t"                                                \
			"0:\n\t"                                                           \
			/* Some single byte nops (0x90) to help disassembly start       */ \
			/* correctly again on an instruction boundary                   */ \
			".ascii \"\\220\\220\\220\\220\"\n\t"                              \
			".ascii \"\\220\\220\\220\\220\"\n\t"                              \
			".ascii \"\\220\\220\\220\\220\"\n\t"                              \
			".ascii \"\\220\\220\\220\\220\""                                  \
			__VA_ARGS__                                                        \
		)

#	define TMCMD_OPCODE(THREE_DIGIT_OCTAL_OPCODE)                              \
		TMCMD_HEADER()                                                         \
		"\\"#THREE_DIGIT_OCTAL_OPCODE                                          \
		TMCMD_FOOTER()

#	define TMCMD_OPCODE_1ARG(THREE_DIGIT_OCTAL_OPCODE, ARG)                    \
		TMCMD_HEADER()                                                         \
		"\\"#THREE_DIGIT_OCTAL_OPCODE                                          \
		TMCMD_FOOTER(::"D"(ARG))

	__attribute__((__noinline__)) void sysTmCmdNop()
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE(000);
		else
			diagLoggedPrintf(">>R*TM NOP\n");
	}

	__attribute__((__noinline__)) void sysTmCmdPrintAllThreadStacks()
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE(001);
		else
			diagLoggedPrintf(">>R*TM PRINT_ALL_THREAD_STACKS\n");
	}

	__attribute__((__noinline__)) void sysTmCmdStopNoErrorReport()
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE(002);
		else
		{
			HANG_DETECT_SAVEZONE_ENTER();
			sysStack::PrintStackTrace();
			diagLoggedPrintf(">>R*TM STOP_NO_ERROR_REPORT\n");
			for (;;) sysIpcSleep(1);
		}
	}

	__attribute__((__noinline__)) void sysTmCmdQuitf(const char *msg)
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE_1ARG(003, msg);
		else
		{
			diagLoggedPrintf(">>R*TM QUITF:%s\n", msg);
			__debugbreak();
		}
	}

	__attribute__((__noinline__)) void sysTmCmdGpuHang()
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE(004);
		else
			diagLoggedPrintf(">>R*TM GPU_HANG\n");
	}

	__attribute__((__noinline__)) void sysTmCmdCpuHang()
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE(005);
		else
		{
			diagLoggedPrintf(">>R*TM CPU_HANG\n");
			__debugbreak();
		}
	}

	__attribute__((__noinline__)) void sysTmCmdConfig(const char *config)
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE_1ARG(006, config);
		else
			diagLoggedPrintf(">>R*TM CONFIG:%s\n", config);
	}

	__attribute__((__noinline__)) void sysTmCmdCreateDump()
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE(007);
		else
			diagLoggedPrintf(">>R*TM CREATE_DUMP\n");
	}

	__attribute__((__noinline__)) void sysTmCmdExceptionHandlerBegin()
	{
		Assert(!sysBootManager::IsDevkit());    // currently not expecting this to be called from a devkit
// 		if (sysBootManager::IsDevkit())
// 			TMCMD_OPCODE(010);
// 		else
			diagLoggedPrintf(">>R*TM EXCEPTION_HANDLER_BEGIN\n");
	}

	__attribute__((__noinline__)) void sysTmCmdExceptionHandlerEnd(const char *coredump, const void *bugstarConfig)
	{
		Assert(!sysBootManager::IsDevkit());    // currently not expecting this to be called from a devkit
// 		if (sysBootManager::IsDevkit())
// 			TMCMD_OPCODE(011);
// 		else
			diagLoggedPrintf(">>R*TM EXCEPTION_HANDLER_END:%s;%s\n", coredump, bugstarConfig?bugstarConfig:"");
	}

	__attribute__((__noinline__)) void sysTmCmdSubmitDoneExceptionEnabled(bool enabled)
	{
		if (sysBootManager::IsDevkit())
			TMCMD_OPCODE_1ARG(200, enabled);
		else
			diagLoggedPrintf(">>R*TM ORBIS_SUBMIT_DONE_EXCEPTION_ENABLED:%u\n", enabled);
	}

#	undef TMCMD_OPCODE_1ARG
#	undef TMCMD_OPCODE
#	undef TMCMD_FOOTER
#	undef TMCMD_HEADER


#elif RSG_PC

	extern "C" void sysTmCmdNop_Internal();
	extern "C" void sysTmCmdPrintAllThreadStacks_Internal();
	extern "C" void sysTmCmdStopNoErrorReport_Internal();
	extern "C" void sysTmCmdQuitf_Internal(const char *msg);
	extern "C" void sysTmCmdGpuHang_Internal();
	extern "C" void sysTmCmdCpuHang_Internal();
	extern "C" void sysTmCmdConfig_Internal(const char *config);
	extern "C" void sysTmCmdExceptionHandlerBegin_Internal();
	extern "C" void sysTmCmdExceptionHandlerEnd_Internal(const char *coredump, const void *bugstarConfig);

	void sysTmCmdNop()
	{
		sysTmCmdNop_Internal();
	}

	void sysTmCmdPrintAllThreadStacks()
	{
		sysTmCmdPrintAllThreadStacks_Internal();
	}

	void sysTmCmdStopNoErrorReport()
	{
		sysTmCmdStopNoErrorReport_Internal();
	}

	void sysTmCmdQuitf(const char *msg)
	{
		sysTmCmdQuitf_Internal(msg);
	}

	void sysTmCmdGpuHang()
	{
		sysTmCmdGpuHang_Internal();
	}

	void sysTmCmdCpuHang()
	{
		sysTmCmdCpuHang_Internal();
	}

	void sysTmCmdConfig(const char *config)
	{
		sysTmCmdConfig_Internal(config);
	}

	void sysTmCmdCreateDump()
	{
// #		if EXCEPTION_HANDLING
// 			sysException::GenerateCrashDump();
// #		endif
	}

	void sysTmCmdExceptionHandlerBegin()
	{
		sysTmCmdExceptionHandlerBegin_Internal();
	}

	void sysTmCmdExceptionHandlerEnd(const char *coredump, const void *bugstarConfig)
	{
		sysTmCmdExceptionHandlerEnd_Internal(coredump, bugstarConfig);
	}


#elif RSG_PS3

	// Non-inline to force specific registers
	// r3 should contain "R*TM"
	// r4 should contain the command number
	// r5 optional additional data
	// r6 optional additional data
	__attribute__((__noinline__)) float sysTmCmd(u32 r3, u64 /*r4*/, u64 /*r5*/=0, u64 /*r6*/=0)
	{
		// Perform a float load since "R*TM" is non-aligned and will raise an exception
		return *(volatile float*)r3;
	}

	void sysTmCmdNop()
	{
		sysTmCmd(0x522a544d, 0);
	}

	void sysTmCmdPrintAllThreadStacks()
	{
		sysTmCmd(0x522a544d, 1);
	}

	void sysTmCmdStopNoErrorReport()
	{
		sysTmCmd(0x522a544d, 2);
	}

	void sysTmCmdQuitf(const char *msg)
	{
		sysTmCmd(0x522a544d, 3, (u32)msg);
	}

	void sysTmCmdGpuHang()
	{
		sysTmCmd(0x522a544d, 4);
	}

	void sysTmCmdCpuHang()
	{
		sysTmCmd(0x522a544d, 5);
	}

	void sysTmCmdConfig(const char *config)
	{
		sysTmCmd(0x522a544d, 6, (u32)config);
	}

	void sysTmCmdCreateDump()
	{
		sysTmCmd(0x522a544d, 7);
	}

	void sysTmCmdExceptionHandlerBegin()
	{
		sysTmCmd(0x522a544d, 8);
	}

	void sysTmCmdExceptionHandlerEnd(const char *coredump, const void *bugstarConfig)
	{
		sysTmCmd(0x522a544d, 9, (u32)coredump, (u32)bugstarConfig);
	}

	void sysTmInit(bool waitConnect, u32 UNUSED_PARAM(timeoutMs))
	{
		if (waitConnect)
		{
			diagLoggedPrintf(SYSTMINIT_WAIT_MSG);

			// Because target side exception handling is disabled with
			// -rockstartargetmanager, if the R*TM is not yet running, then this
			// crash just hangs until it starts.
			sysTmCmdNop();
			diagLoggedPrintf(SYSTMINIT_CONNECTED_MSG);
		}
	}

#elif RSG_XENON

#	define TMCMD_HEADER(DATA_SIZE)                                                                                     \
		__emit(0x0fe00016);                         /* __debugbreak()                                               */ \
		                                                                                                               \
		/* note +4, not +3, since not just rounding up,  but also taking into account the opcode byte               */ \
		__emit(0x48000000+(((DATA_SIZE)+4)&~3)+8);  /* b        .+?         010010 000000000000000000000000 00      */ \
		__emit(0x522a544d);                         /* .ascii   "R*TM"                                              */

#	define TMCMD_FOOTER()                                                                                              \
		__emit(0x4e800020);                         /* blr                  010011 10100 00000 000 00 0000010000 0  */

	__declspec(naked noinline) void sysTmCmdNop()
	{
		TMCMD_HEADER(0)
		__emit(0x00000000);
		TMCMD_FOOTER()
	}

	__declspec(naked noinline) void sysTmCmdPrintAllThreadStacks()
	{
		TMCMD_HEADER(0)
		__emit(0x01000000);
		TMCMD_FOOTER()
	}

	__declspec(naked noinline) void sysTmCmdStopNoErrorReport()
	{
		TMCMD_HEADER(0)
		__emit(0x02000000);
		TMCMD_FOOTER()
	}

	static __declspec(naked noinline) void sysTmCmdQuitf_Internal(const char *UNUSED_PARAM(msg))
	{
		TMCMD_HEADER(0)
		__emit(0x03000000);
		TMCMD_FOOTER()
	}

	void sysTmCmdQuitf(const char *msg)
	{
		if (!sysBootManager::IsDevkit())
		{
			diagLoggedPrintf(">>R*TM QUITF:%s\n", msg);

			// The __debugbreak currently used in the sysTmCmdXXX functions does
			// not stop a testkit.  Need to crash another way.  There is no
			// guaranteed invalid instruction like UD2 on x86, so just pick a
			// random invalid instruction 0 (there are no zero opcode
			// instructions).
			__emit(0);
		}
		else
		{
			sysTmCmdQuitf_Internal(msg);
		}
	}

	__declspec(naked noinline) void sysTmCmdGpuHang()
	{
		TMCMD_HEADER(0)
		__emit(0x04000000);
		TMCMD_FOOTER()
	}

	__declspec(naked noinline) void sysTmCmdCpuHang()
	{
		TMCMD_HEADER(0)
		__emit(0x05000000);
		TMCMD_FOOTER()
	}

	__declspec(naked noinline) void sysTmCmdConfig_Internal(const char *config)
	{
		TMCMD_HEADER(0)
		__emit(0x05000000);
		TMCMD_FOOTER()
	}

	__declspec(naked noinline) void sysTmCmdCreateDump()
	{
		TMCMD_HEADER(0)
		__emit(0x07000000);
		TMCMD_FOOTER()
	}

	__declspec(naked noinline) void sysTmCmdExceptionHandlerBegin()
	{
		TMCMD_HEADER(0)
		__emit(0x08000000);
		TMCMD_FOOTER()
	}

	__declspec(naked noinline) void sysTmCmdExceptionHandlerEnd(const char *UNUSED_PARAM(coredump), const void *UNUSED_PARAM(bugstarConfig))
	{
		TMCMD_HEADER(0)
		__emit(0x09000000);
		TMCMD_FOOTER()
	}

	void sysTmCmdCpuHang(const char *config)
	{
		if (!sysBootManager::IsDevkit())
		{
			diagLoggedPrintf(">>R*TM CONFIG:%s\n", config);
			__emit(0);
		}
		else
		{
			sysTmCmdConfig_Internal(config);
		}
	}


#	undef TMCMD_FOOTER
#	undef TMCMD_HEADER

	// This is a bit ridiculous :(
	//
	// There doesn't seem to be any debug API to "stop" a thread from the PC,
	// only to "suspend" it.  But in order to modify registers (and hence call
	// callbacks, etc), the thread must be stopped.  So here we have a dedicated
	// thread for R*TM comms.
	//
	__declspec(naked noinline) static void TmThread(void *UNUSED_PARAM(connectedSema))
	{
		// 0    spin waiting for R*TM
		// 1    set by R*TM to signal initial connection, does not trigger a break
		// 2    set by R*TM to cause a break
		static volatile u8 externalBreak = false;
		__asm
		{
			mflr    r12
			stw     r12, -8(r1)
#			undef   std // STLPort defines std to something stupid
		  //std     r28, -0x28(r1)
			std     r29, -0x20(r1)
			std     r30, -0x18(r1)
			std     r31, -0x10(r1)
			stwu    r1,  -0x80(r1)

			// r31 set to signature {"R*TM",r1[32:63]} so that the R*TM can
			// check that we have made it up to this main function (else there
			// would be a race condition on thread launch.  The reason for
			// copying the stack pointer into r31 as well is to make sure we
			// don't get a false positive match inside any functions we call
			// from here (since it may preserve r31, but trash r30).
			addis   r31, r0,  0x522a
			ori     r31, r31, 0x544d
			rldicr  r31, r31, 32, 31
			rldimi  r31, r1, 0, 32

			// r30 is read by the R*TM, so the register assignment must not be
			// changed unless the PC side too is also changed.
			lau     r30, externalBreak
			lal     r30, r30, externalBreak

			// If connectedSema!=null, then increment semaphore when first
			// broken into by the R*TM.
			cmpwi   r3, 0
			ori     r29, r3, 0
			beq     wait_break_loop

			// Spin in a loop until the R*TM sets externalBreak to non-zero
		wait_connect_loop:
			li      r3, 10
			bl      sysIpcSleep
			lbz     r3, 0(r30)
			cmpwi   r3, 0
			beq     wait_connect_loop

			// Tell the main thread that we have connected and it can now
			// continue.
			ori     r3, r29, 0
			li      r4, 1
			bl      sysIpcSignalSema

			// Clear externalBreak
			li      r3, 0
			stb     r3, 0(r30)

			// Now spin for ever.  If we ever detect externalBreak set again,
			// then hit a hardcoded breakpoint, transferring control back to the
			// target manager.
		wait_break_loop:
			li      r3, 30
			bl      sysIpcSleep
			lbz     r3, 0(r30)
			cmplwi  r3, 2
			blt     wait_break_loop     // spin while 0 or 1, as R*TM may try to reconnect

			twi     31, r0, 22          // __debugbreak()

			// Clear externalBreak
			li      r3, 0
			stb     r3, 0(r30)

			b       wait_break_loop
		}
	}

	void sysTmInit(bool waitConnect, u32 UNUSED_PARAM(timeoutMs))
	{
		if (!sysBootManager::IsDevkit())
		{
			if (waitConnect)
			{
				Errorf("360 console is not a dev kit.  Only minimal Rockstar Target Manager support can be enabled.");
			}
			return;
		}

		sysIpcSema connectedSema = NULL;

		if (waitConnect)
		{
			diagLoggedPrintf(SYSTMINIT_WAIT_MSG);

			connectedSema = sysIpcCreateSema(!waitConnect);
		}

		// The thread name "R*TM" is searched for by the PC side R* target
		// manager, so do not change this, unless the PC side tool is also
		// changed.
		//
		// Also note that this thread has an extremely high priority set on
		// purpose.  It will be sleeping virtually all the time, so should not
		// impact performance, but when it does wake up, it needs to be able to
		// preempt other threads in the case of a live lock.
		//
		sysIpcCreateThread(TmThread, connectedSema, sysIpcMinThreadStackSize, PRIO_HIGHEST, "R*TM");

		if (waitConnect)
		{
			sysIpcWaitSema(connectedSema);
			sysIpcDeleteSema(connectedSema);
			diagLoggedPrintf(SYSTMINIT_CONNECTED_MSG);
		}
	}


#else
	void sysTmCmdNop()
	{
	}

	void sysTmCmdPrintAllThreadStacks()
	{
	}

	void sysTmCmdStopNoErrorReport()
	{
		__debugbreak();
	}

	void sysTmCmdQuitf(const char *UNUSED_PARAM(msg))
	{
		diagTerminate();
	}

	void sysTmCmdGpuHang()
	{
		__debugbreak();
	}

	void sysTmCmdCpuHang()
	{
		__debugbreak();
	}

	void sysTmCmdConfig(const char *UNUSED_PARAM(config))
	{
	}

	void sysTmCmdCreateDump()
	{
	}

	void sysTmCmdExceptionHandlerBegin()
	{
	}

	void sysTmCmdExceptionHandlerEnd(const char *UNUSED_PARAM(coredump), const void *UNUSED_PARAM(bugstarConfig))
	{
	}

	void sysTmInit(bool UNUSED_PARAM(waitConnect), u32 UNUSED_PARAM(timeoutMs))
	{
	}

#endif


#undef SYSTMINIT_WAIT_MSG

}
// namespace rage

#endif // SYSTMCMD_ENABLE
