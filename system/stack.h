//
// system/stack.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_STACK_H 
#define SYSTEM_STACK_H

#include <stddef.h>

#if __WIN32
#include <stdio.h>
struct _EXCEPTION_POINTERS;
extern "C" void *        __cdecl _exception_info(void);
#endif

#if RSG_ORBIS
#define SIZETFMT	"l"
#define PTRDIFFTFMT "l"
#elif __64BIT
#define SIZETFMT	"I"
#define PTRDIFFTFMT "I"
#elif __PS3
#define SIZETFMT	"z"
#define PTRDIFFTFMT "t"
#else
#define SIZETFMT	""
#define PTRDIFFTFMT ""
#endif

#if __PPU
#include <sys/ppu_thread.h>
#endif
#include "system/tmcommands.h"

namespace rage {

/*
PURPOSE
This class provides access to the Win32 call stack. The member functions are all static functions. (<c>__WIN32</c> only.)
<FLAG Component>
*/
class sysStack
{
public:
#if !__FINAL && !__NO_OUTPUT

	// PURPOSE: Called by Rockstar Target Manager when the console crashes.
	// NOTE:    Deprecated, only being kept around temprarily for backwards compatability.
	//          Use sysException::TargetCrashCallback instead.
	static void TargetCrashCallback(unsigned pass);

	// PURPOSE:	Default stack line display function
	static void	 DefaultDisplayLine(size_t addr,const char* sym,size_t displacement);

    // PURPOSE: Allow the application to perform additional work just prior
    // to displaying the stack track in PrintStackTrace().
    static void (*PrePrintStackTrace)();

    // PURPOSE: Allow the application to perform additional work just after
    // displaying the stack track in PrintStackTrace().
    static void (*PostPrintStackTrace)();

	// PURPOSE:	Called by our setup code.
	static void InitClass(const char *argv0);

	// PURPOSE:	Called by our teardown code.
	static void	ShutdownClass();

	// PURPOSE: Stores the symbol name containing the given address into the dest buffer.
	// PARAMS:	dest - Destination buffer
	//			destSize - sizeof(dest)
	//			lookupAddr - Address of symbol to resolve
	//			runningProcess - true if this is a lookup for the running executable (false for 'cmpmaptool').
	// RETURNS: Offset from symbol name to lookup address.
	static u32 ParseMapFileSymbol(char *dest,int destSize,size_t lookupAddr,bool runningProcess = true);

	// PURPOSE: Capture the current call stack into a buffer
	// PARAMS:	pDest - Destination buffer, guaranteed be zero-terminated
	//			destCount - Number of elements in pDest
	//			ignoreLevels - Number of stack levels to skip (including this function itself)
	static void CaptureStackTrace(size_t* pDest, u32 destCount, u32 ignoreLevels = 1);

	// PURPOSE:	 Call CaptureStackTrace, then display it with PrintCapturedStackTrace.
	static void	PrintStackTrace(void (*DisplayFn)(size_t,const char*,size_t) = DefaultDisplayLine, u32 ignoreLevels = 2);

#if __XENON
	// PURPOSE: Capture the current call stack for the given thread into a buffer.
	// PARAMS:	pDest - Destination buffer, guaranteed be zero-terminated
	//			destCount - Number of elements in pDest
	//			ignoreLevels - Number of stack levels to skip (including this function itself)
    // NOTES:   The thread should be stopped prior to calling this function.
	static void CaptureStackTrace(const unsigned threadId,
                                    u32* pDest,
                                    const unsigned destCount,
                                    const unsigned ignoreLevels);
#endif // __XENON

#if __XENON || __WIN32PC
	// PURPOSE:	 For each thread print the stack trace with PrintCapturedStackTrace.
	static void	PrintStackTraceAllThreads(void (*DisplayFn)(size_t,const char*,size_t) = DefaultDisplayLine);
#endif // __XENON || __WIN32PC

	// PURPOSE: Print a stack trace captured with CaptureStackTrace
	static void	PrintCapturedStackTrace(const size_t* pSrc, int entries, void (*DisplayFn)(size_t,const char*,size_t) = DefaultDisplayLine);

	// PURPOSE: Returns true if within the scope of a PrintStackTrace, useful for determining if the stack being traced is the
	// result of a real exception or just the result of an informative stack trace display.
	static bool	IsPrintingStackTrace();

	// PURPOSE:	Call this if you're externally calling ParseMapFileSymbol many times.  Call CloseSymbolFile when done.
	//			Calls nest properly.  ParseMapFileSymbol calls it internally if necessary though.
	static void OpenSymbolFile();

	static void CloseSymbolFile();

	static const char* GetSymbolFilename();

#if !__FINAL
	// Associates the current backtrace with a small numeric handle.  Useful when there are a small number of
	// backtraces you need to remember and don't want to spend the memory.
	static u16 RegisterBacktrace(int ignoreLevels=2);

	// Prints a registered backtrace.
	static void PrintRegisteredBacktrace(u16, void (*DisplayFn)(size_t,const char*,size_t) = DefaultDisplayLine);
#endif

#else
	static void PrintStackTrace(void (* /*DisplayFn*/)(size_t,const char*,size_t) = NULL, u32 /*ignoreLevels*/ = 2) {}
#endif // !__FINAL && !__NO_OUTPUT
};

#if __SPU
// PURPOSE: Print the stack pointer and remaining stack space left on SPU
__forceinline void PrintStackInfo(const char* file, int line)
{
	void* stack;
	asm volatile("lr %0, $1" : "=r" (stack));
	void* stackSize;
	asm volatile("rotqbyi  %0, $1, 0x04" : "=r" (stackSize));
	Displayf("stack: 0x%08x size: 0x%08x line: %s %d", (u32)stack, (u32)stackSize, file, line);
}

#define PRINT_STACK_INFO ::rage::PrintStackInfo(__FILE__, __LINE__)
#else
#define PRINT_STACK_INFO
#endif

} // namespace rage

#endif
