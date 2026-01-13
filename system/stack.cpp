//
// system/stack.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "stack.h"

#include "criticalsection.h"
#include "file/device.h"
#include "file/stream.h"
#include "math/amath.h"
#include "system/bootmgr.h"
#include "system/exec.h"
#include "system/memory.h"
#include "system/nelem.h"
#include "system/param.h"
#include "system/tmcommands.h"
#include "system/threadregistry.h"

#include "diag/channel.h"
#include "diag/output.h"		// for colorization
#include "diag/diagerrorcodes.h"

#include <algorithm>

// Change these to modify the prefix before all stack trace printouts, if you want it to stand out.
// The bizarre bit below allows it to show up in the error logs of CI build machine runs.
// But we only want it for real rage code, not the projects
#if HACK_MC4 | HACK_RDR2 | HACK_MP3	| HACK_JIMMY | HACK_BULLY2 | HACK_MC5
#define PREFIX	TPurple "\t"
#elif HACK_GTA4 // This doesn't agree with the GTA/Jimmy asset tester machine
#define PREFIX	TPurple
#else
#define PREFIX	TPurple "stack.cpp(64): Error : "
#endif
#define SUFFIX	TNorm

#define INCLUDE_LINE_NUMBERS_IN_CALLSTACK		(__BANK)

#if __WIN32PC
#include "file/asset.h"
#endif

#if __XENON
#	include "xtl.h"
#	include <xbdm.h>
#elif __WIN32PC
#	include "xtl.h"
#	include <DbgHelp.h>
#	pragma comment(lib,"dbghelp")
#	include <TlHelp32.h>
#	include "system/param.h"
#	if !__NO_OUTPUT
#		include <psapi.h>	// For EnumProcessModules(), etc.
#	endif
#elif __PS3
#	include <cell/cell_fs.h>
#	include <cell/dbg.h>
#	include <sys/dbg.h>
#	include <sys/process.h>
#	include <sys/timer.h>
#	pragma comment(lib, "lv2dbg_stub")
#elif __PSP2
#	include <kernel.h>
#	include <kernel/backtrace.h>
#elif RSG_ORBIS
#	include "system/platform.h"
#	include <sdk_version.h>
#elif RSG_DURANGO
#	include "xtl.h"
#	include <dbghelp.h>
#endif

using namespace rage;

#if !__FINAL && !__NO_OUTPUT
NOSTRIP_PARAM(symfilepath, "Tell the game where to find the symbol file");
PARAM(nostacktrace, "Stack trace kills PC performance when abused");
#endif


#if !__FINAL && !__NO_OUTPUT

// Depracated wrapper.
#include "exception.h"  // the dependency on sysException here is bad, should be removed as soon as we can release a new R*TM
namespace rage { namespace sysException { void TargetCrashCallback(unsigned); } }
void sysStack::TargetCrashCallback(unsigned EXCEPTION_HANDLING_ONLY(pass))
{
#	if EXCEPTION_HANDLING
		sysException::TargetCrashCallback(pass);
#	endif
}


void sysStack::DefaultDisplayLine(size_t OUTPUT_ONLY(addr),const char *OUTPUT_ONLY(sym),size_t OUTPUT_ONLY(offset)) {
#	if __64BIT
		diagLoggedPrintf(PREFIX "0x%016" SIZETFMT "x - %s+0x%" SIZETFMT "x" SUFFIX "\n",addr,sym,offset);
#	else
		diagLoggedPrintf(PREFIX "0x%08" SIZETFMT "x - %s+0x%" SIZETFMT "x" SUFFIX "\n",addr,sym,offset);
#	endif
}


void (*sysStack::PrePrintStackTrace)() = NULL;

static bool s_IsPrintingStackTrace = false;

bool sysStack::IsPrintingStackTrace() { 
	return s_IsPrintingStackTrace; 
}

struct Symbol {
	size_t Addr;
	size_t Name;
};

static ptrdiff_t s_SymbolCount;
static ptrdiff_t s_SymbolPreferredLoadAddress;	// Default load address, from the compressed map file.
#if RSG_DURANGO || RSG_ORBIS
static ptrdiff_t s_UnrelocatedMainAddress;      // Address of main in map file.
#elif __WIN32PC
static size_t s_SymbolActualLoadAddress;		// Actual load address as reported by Windows.
static size_t s_SymbolExecutableSize;			// Size of the executable image, as reported by Windows.
#endif
static char s_SymbolFileName[256];				// File name (on disk or in memory) of symbol file
static __THREAD fiStream *s_SymbolFile = NULL;	// Per-thread handle to symbol file
static __THREAD int s_SymbolFileRefCount;		// Per-thread reference count to symbol file
#if ENABLE_DEBUG_HEAP
static bool s_TriedToAllocateCache;				// Have we already tried to load the symbol file into memory?
#endif
static bool s_IsMem;							// True if s_SymbolFileName is a memory file

static sysCriticalSectionToken	s_symbolFileCritSec;

void sysStack::OpenSymbolFile() {

	s_symbolFileCritSec.Lock();

	// Inc (per-thread) refcount
	++s_SymbolFileRefCount;

	if (!s_SymbolFile && s_SymbolCount)		// don't waste time opening it if it was never there.
	{
		s_SymbolFile = fiStream::Open(s_SymbolFileName,s_IsMem? fiDeviceMemory::GetInstance():fiDeviceLocal::GetInstance());
#if ENABLE_DEBUG_HEAP
		if (s_SymbolFile && !s_TriedToAllocateCache && g_sysHasDebugHeap)
		{
			// Try to load the symbol file into (debug) memory for faster access
			s_TriedToAllocateCache = true;
			int symSize = s_SymbolFile->Size();
			char* symBuffer;

			{
				// Debug Heap
				sysMemAutoUseDebugMemory debug;
				symBuffer = rage_new char[symSize];
			}

			if (symBuffer)
			{
				// Read symbol file into memory and then re-open the file as a memory file
				s_SymbolFile->Read(symBuffer,symSize);
				fiDevice::MakeMemoryFileName(s_SymbolFileName,sizeof(s_SymbolFileName),symBuffer,symSize,false,"stack cache file");
				s_SymbolFile->Close();
				s_SymbolFile = fiStream::Open(s_SymbolFileName,fiDeviceMemory::GetInstance());
				s_IsMem = true;
			}
			else
			{
				Errorf("sysStack::OpenSymbolFile - Could not allocate memory to read symbol file '%s'.", s_SymbolFileName);
			}
		}
		else if(!s_IsMem)
		{
			Errorf("sysStack::OpenSymbolFile - Could not open symbol file '%s'.", s_SymbolFileName);
		}
#endif
	}
}


void sysStack::CloseSymbolFile() {
	if (--s_SymbolFileRefCount == 0 && s_SymbolFile) {
		s_SymbolFile->Close();
		s_SymbolFile = NULL;
	}

	s_symbolFileCritSec.Unlock();
}


#if RSG_DURANGO
extern "C" int main();
#elif RSG_ORBIS
extern int main(int argc,char **argv);
#endif

void sysStack::InitClass(const char *argv0) {
	Assert(!s_SymbolFile);

	// Nonsense just to ensure that TargetCrashCallback is never dead stripped.
	// When the depracted sysStack::TargetCrashCallback wrapper is removed, this code should be moved into exception.cpp.
	if ((void(*)(unsigned))argv0 == TargetCrashCallback)
	{
		TargetCrashCallback(0);
	}

	const char *pCustomSymFilePath = 0;
	if (!PARAM_symfilepath.Get(pCustomSymFilePath))
	{
		if (sysBootManager::IsBootedFromDisc())
		{
			//Assume loading from disk
			//should probably use s_SymbolFileName
			if (strrchr(argv0,':'))
			{
#if __XENON
				if (argv0[0] == 'E')	// local hard drive - so don't change this.
					strcpy(s_SymbolFileName,argv0);
				else
#endif
					formatf(s_SymbolFileName,"game:%s", strrchr(argv0,':')+1);
			}
			else
			{
#if __PS3
				if (strncmp(argv0, "/dev", 4) == 0) // Local device - don't change.
					strcpy(s_SymbolFileName,argv0);
				else
#endif // __PS3
					formatf(s_SymbolFileName,"game:%s", argv0);
			}
		}
		else
		{
			strcpy(s_SymbolFileName,argv0);
		}

		if (strrchr(s_SymbolFileName,'.'))
			strcpy(strrchr(s_SymbolFileName,'.'),".cmp");
		else
			strcat(s_SymbolFileName,".cmp");
	}
	else
	{
		strcpy(s_SymbolFileName, pCustomSymFilePath);
	}

	// TODO: If there's enough external memory, we could cache the symbol table for great justice.
	s_SymbolFile = fiStream::Open(s_SymbolFileName,fiDeviceLocal::GetInstance());
	if (s_SymbolFile) {
#if __64BIT
		s_SymbolFile->Read(&s_SymbolCount, sizeof(ptrdiff_t));
		s_SymbolFile->Read(&s_SymbolPreferredLoadAddress, sizeof(ptrdiff_t));
#else
		s_SymbolFile->ReadInt(&s_SymbolCount, 1);
		s_SymbolFile->ReadInt(&s_SymbolPreferredLoadAddress, 1);
#endif
#if RSG_DURANGO || RSG_ORBIS
		s_SymbolFile->Read(&s_UnrelocatedMainAddress, sizeof(ptrdiff_t));
#endif
		s_SymbolFile->Close();
		s_SymbolFile = NULL;
#if __XENON		// Detect and ignore placeholder file.
		if (s_SymbolCount == 'calP')
			s_SymbolCount = 0;
#endif
	}
	else
	{
#if RSG_ORBIS && SCE_ORBIS_SDK_VERSION >= (0x00930020u)
		Warningf("Could not load symbol file from %s. Make sure your working directory (/app0/) is set correctly.", s_SymbolFileName);
#endif
	}

	// Pre-cache the symbol file if possible
	OpenSymbolFile();
	CloseSymbolFile();

#if __WIN32PC
	// In case the code below fails, these values should work if we loaded
	// at our preferred address.
	s_SymbolActualLoadAddress = 0;
	s_SymbolExecutableSize = ~0U;

	// From the current process, populate an array of module identifiers.
	// According to some documentation, the first should be the executable itself,
	// which is the only thing we're interested in, so we only need an array with one slot.
	HMODULE hMods[1];
	DWORD cbNeeded = 0;
	HANDLE process = GetCurrentProcess();
	if(EnumProcessModules(process, hMods, sizeof(hMods), &cbNeeded) && cbNeeded >= 1)
	{
		// Determine the actual base address and the size of the executable.
		MODULEINFO info;
		if(GetModuleInformation(process, hMods[0], &info, sizeof(info)))
		{
			s_SymbolActualLoadAddress = (size_t)info.lpBaseOfDll;
			s_SymbolExecutableSize = (size_t)info.SizeOfImage;
		}
	}
	SymSetOptions(SYMOPT_LOAD_LINES);
	SymInitialize(process, NULL, TRUE);
#endif
}

const char* sysStack::GetSymbolFilename()
{
	return s_SymbolFileName;
}

void sysStack::ShutdownClass() {
	Assert(s_SymbolFileRefCount == 0);
}

u32 sysStack::ParseMapFileSymbol(char *dest,int destSize,size_t lookupAddr,bool WIN32PC_ONLY(runningProcess)) {
	OpenSymbolFile();

	if (s_SymbolFile) {

		bool gotAddr = true;

		// On PC, we hopefully know the range of addresses where the executable loaded.
		// If within that range, here we remap the lookup address to an address in the
		// map file.
#if __WIN32PC
		if(runningProcess)
		{
			if(lookupAddr >= s_SymbolActualLoadAddress && lookupAddr < s_SymbolActualLoadAddress + s_SymbolExecutableSize)
			{
				lookupAddr = lookupAddr - s_SymbolActualLoadAddress + s_SymbolPreferredLoadAddress;
			}
			else
			{
				gotAddr = false;
			}
		}
#elif RSG_DURANGO || RSG_ORBIS
		const size_t mainOffset = (size_t) main ;
		lookupAddr = lookupAddr - mainOffset + s_UnrelocatedMainAddress;
#endif

		if(gotAddr)
		{
			ptrdiff_t low = 0, high = s_SymbolCount - 2;
			while (low<high) {
				ptrdiff_t mid = (low + high) >> 1;
				Symbol symbols[2];
				s_SymbolFile->Seek64(sizeof(s_SymbolCount) + sizeof(s_SymbolPreferredLoadAddress)
#if RSG_DURANGO || RSG_ORBIS
					+ sizeof(s_UnrelocatedMainAddress)
#endif
					+ mid * sizeof(Symbol));
#if __64BIT
				s_SymbolFile->Read(symbols, sizeof(symbols));
#else
				s_SymbolFile->ReadInt((int*)(symbols), 4);
#endif
				if (lookupAddr >= symbols[0].Addr && lookupAddr < symbols[1].Addr) {
					s_SymbolFile->Seek64(symbols[0].Name);
					s_SymbolFile->Read(dest,(128 < destSize ? 128 : destSize));
					CloseSymbolFile();
					return ptrdiff_t_to_int(lookupAddr - symbols[0].Addr);
				}
				else if (lookupAddr > symbols[0].Addr)
					low = mid+1;
				else
					high = mid;
			}
		}
		safecpy(dest,"notfound",destSize);
	}
	else
		safecpy(dest,"nosymbols",destSize);

	CloseSymbolFile();
	return 0;
}


void sysStack::PrintStackTrace(void (*displayFn)(size_t,const char*,size_t), u32 ignoreLevels) {

	if (PrePrintStackTrace)
	{
		PrePrintStackTrace();
	}

	size_t trace[32];
	CaptureStackTrace(trace,32,ignoreLevels);
	PrintCapturedStackTrace(trace, 32, displayFn);
}

void sysStack::PrintCapturedStackTrace(const size_t *trace, int entries, void (*displayFn)(size_t,const char*,size_t)) 
{
	if (PARAM_nostacktrace.Get())
		return;

	if (s_IsPrintingStackTrace)
	{
		diagLoggedPrintf(PREFIX "Request to print callstack received while printing a callstack." SUFFIX "\n");
		return;
	}

	s_IsPrintingStackTrace = true;

#if __WIN32PC
	// Use the dbghelp library for symbol lookup on PC, since it's able to resolve symbols in DLLs
	HANDLE process = GetCurrentProcess();
	static char symbolBuff[sizeof(SYMBOL_INFO) + 256]; // SYMBOL_INFO + symbol name
#if INCLUDE_LINE_NUMBERS_IN_CALLSTACK
	static char lineBuff[64]; // line number
#endif
	static char moduleName[64]; // module name
	static char buff[256+64]; // module_name!symbol
	SYMBOL_INFO* pSymbol = (SYMBOL_INFO*)symbolBuff;
	for (const size_t *tp=trace; *tp && entries--; tp++) {
		DWORD64 disp;
		pSymbol->MaxNameLen = sizeof(symbolBuff) - sizeof(SYMBOL_INFO);
		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		if (!SymFromAddr(process, (DWORD64)*tp, &disp, pSymbol)) {
			formatf(pSymbol->Name, pSymbol->MaxNameLen, "unknown");
			disp = 0;
		}

		// Find the module containing this function
		HMODULE module;
		if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)*tp, &module))
		{
			formatf(moduleName, "unknown");
			disp = 0;
		}
		else {
			GetModuleBaseName(process, module, moduleName, NELEM(moduleName));
			if (disp == 0) {
				disp = (DWORD64)*tp - (DWORD64)module;
			}
		}

#if INCLUDE_LINE_NUMBERS_IN_CALLSTACK
		// Find the line number
		IMAGEHLP_LINE64 line;
		DWORD dispLine;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		if (!SymGetLineFromAddr64(process, (DWORD64)*tp, &dispLine, &line)) {
			lineBuff[0] = '\0';
		}
		else {
			formatf(lineBuff, "(Line %d)", line.LineNumber);
		}

		formatf(buff, "%s!%s%s", moduleName, pSymbol->Name, lineBuff);
#else
		formatf(buff, "%s!%s", moduleName, pSymbol->Name);
#endif
		displayFn(*tp,buff,(size_t)disp);
	}
#else
	OpenSymbolFile();

	for (const size_t *tp=trace; *tp && entries--; tp++) {
		char symname[256] = "unknown";
		u32 disp = ParseMapFileSymbol(symname,sizeof(symname),*tp);
		displayFn(*tp,symname,disp);
	}

	CloseSymbolFile();
#endif // __WIN32PC

	s_IsPrintingStackTrace = false;
}

void sysStack::CaptureStackTrace(size_t* pDest, u32 destCount, u32 ignoreLevels /* = 1 */) {

#if __XENON
	DWORD* trace = Alloca(DWORD, destCount + ignoreLevels);
	DmCaptureStackBackTrace(destCount + ignoreLevels - 1,(VOID**)trace);
	trace[destCount + ignoreLevels - 1] = 0;
	XMemCpy(pDest, trace + ignoreLevels, destCount * sizeof(u32));
#elif __PS3
	memset(pDest, 0, destCount * sizeof(u32));
	cellDbgPpuThreadGetStackBackTrace(ignoreLevels,destCount-1,pDest,NULL);
#elif __PSP2
	unsigned numReturn = 0;
	SceKernelCallFrame *trace = Alloca(SceKernelCallFrame, destCount + ignoreLevels);
	SceSize numBytes = sizeof(SceKernelCallFrame) * (destCount + ignoreLevels);
	memset(trace,0,numBytes);
	sceKernelBacktrace(SCE_KERNEL_BACKTRACE_CONTEXT_CURRENT, trace, numBytes, &numReturn, SCE_KERNEL_BACKTRACE_MODE_USER);
	for (u32 i=0; i<destCount; i++)
		pDest[i] = trace[i + ignoreLevels].pc;
#elif __WIN32PC
	memset(pDest, 0, destCount * sizeof(size_t));
	CaptureStackBackTrace(ignoreLevels, destCount, (PVOID*)pDest, NULL);
#elif RSG_DURANGO
	memset(pDest, 0, destCount * sizeof(size_t));
	RtlCaptureStackBackTrace(ignoreLevels, destCount, (PVOID*)pDest, NULL);
#elif RSG_ORBIS
	size_t temp[12];
	memset(temp,0,sizeof(temp));
	// Each one of these generates 3+N instructions in a debug build; it's better when optimized
#define STACK_LEVEL(n) temp[n] = (size_t)__builtin_return_address(n); if (temp[n] && temp[n] == (u32)temp[n])
	STACK_LEVEL(0) {
		STACK_LEVEL(1) {
			STACK_LEVEL(2) {
				STACK_LEVEL(3) {
					STACK_LEVEL(4) {
						STACK_LEVEL(5) {
							STACK_LEVEL(6) {
								STACK_LEVEL(7) {
									STACK_LEVEL(8) {
										STACK_LEVEL(9) {
											STACK_LEVEL(10) {
												temp[11] = (size_t) __builtin_return_address(11);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
#undef STACK_LEVEL
	for (u32 i=0; i<destCount; i++)
		pDest[i] = i + ignoreLevels < 12? temp[i + ignoreLevels] : 0;
#endif
}

#if __WIN32PC
void sysStack::PrintStackTraceAllThreads(void (*DisplayFn)(size_t,const char*,size_t))
{
	// Get a handle to each thread in this process apart from this one
	DWORD currProcessId = GetCurrentProcessId();
	DWORD currThreadId = GetCurrentThreadId();
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, currProcessId);
	static HANDLE hThreads[160];
	static char errorBuff[256];
	u32 numThreads = 0;
	if(hSnapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if(Thread32First(hSnapshot, &te))
		{
			do {
				// This will return all the threads on the system, not just in our process!
				if(te.th32OwnerProcessID == currProcessId && te.th32ThreadID != currThreadId)
				{
					HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
					if(hThread)
					{
						hThreads[numThreads++] = hThread;
						if(numThreads == NELEM(hThreads))
						{
							diagLoggedPrintf(PREFIX "Warning: Found too many threads - increase size of hThreads[] array!");
							break;
						}
					}
					else
					{
#if THREAD_REGISTRY
						diagLoggedPrintf(PREFIX "Warning: OpenThread() failed for '%s' (thread ID 0x%x). Error code %d (%s)\n",
							sysThreadRegistry::GetThreadName((sysIpcThreadId)te.th32ThreadID), te.th32ThreadID, GetLastError(),
							diagErrorCodes::Win32ErrorToString(GetLastError(), errorBuff, NELEM(errorBuff)));
#else
						diagLoggedPrintf(PREFIX "Warning: OpenThread() failed for thread ID 0x%x. Error code %d (%s)\n",
							te.th32ThreadID, GetLastError(),
							diagErrorCodes::Win32ErrorToString(GetLastError(), errorBuff, NELEM(errorBuff)));
#endif // THREAD_REGISTRY
					}
				}
				te.dwSize = sizeof(te);
			} while(Thread32Next(hSnapshot, &te));
		}
		else
		{
			diagLoggedPrintf(PREFIX "Warning: Thread32First() failed with error %d (%s)\n", GetLastError(),
				diagErrorCodes::Win32ErrorToString(GetLastError(), errorBuff, NELEM(errorBuff)));
		}
		CloseHandle(hSnapshot);
	}
	else
	{
		diagLoggedPrintf(PREFIX "Warning: CreateToolhelp32Snapshot() failed with error %d (%s)\n", GetLastError(),
			diagErrorCodes::Win32ErrorToString(GetLastError(), errorBuff, NELEM(errorBuff)));
	}

	// Gather stacks for all the threads
	for(u32 t=0; t<numThreads; ++t)
	{
		// Thread must be suspended for GetThreadContext() to succeed
		SuspendThread(hThreads[t]);

		// Walk this thread's stack
		CONTEXT ctx;
		sysMemSet(&ctx, 0, sizeof(ctx));
		ctx.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
		if(GetThreadContext(hThreads[t], &ctx))
		{
			STACKFRAME64 frame;
			sysMemSet(&frame, 0, sizeof(frame));
			#if __64BIT
				frame.AddrPC.Offset    = ctx.Rip;
				frame.AddrStack.Offset = ctx.Rsp;
				frame.AddrFrame.Offset = ctx.Rbp;
			#else
				frame.AddrPC.Offset    = ctx.Eip;
				frame.AddrStack.Offset = ctx.Esp;
				frame.AddrFrame.Offset = ctx.Ebp;
			#endif
			frame.AddrPC.Mode      = AddrModeFlat;
			frame.AddrStack.Mode   = AddrModeFlat;
			frame.AddrFrame.Mode   = AddrModeFlat;
			size_t trace[32];
			trace[0] = (size_t)frame.AddrPC.Offset;
			u32 count = 1;
			for(int i=1; i<NELEM(trace); ++i)
			{
				if(!StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), hThreads[t], &frame, &ctx,
					NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
				{
					count = i;
					break;
				}
				trace[i] = (size_t)frame.AddrPC.Offset;
			}

			// Resume the thread as soon as we're done with it's context
			ResumeThread(hThreads[t]);

			// Spit out the thread name (if available, else ID) and stack
#if THREAD_REGISTRY
			diagLoggedPrintf(PREFIX "\nStack for '%s' (thread ID 0x%x):\n",
				sysThreadRegistry::GetThreadName((sysIpcThreadId)GetThreadId(hThreads[t])), GetThreadId(hThreads[t]));
#elif !__RGSC_DLL_X86
			diagLoggedPrintf(PREFIX "\nStack for thread ID 0x%x:\n", GetThreadId(hThreads[t]));
#endif // THREAD_REGISTRY
			PrintCapturedStackTrace(trace, count, DisplayFn);
		}
		else
		{
#if !__RGSC_DLL_X86
			int err = GetLastError();
#endif
			ResumeThread(hThreads[t]); // NB: Resume before printing, in case the thread has the log critsec

#if !__RGSC_DLL_X86
			diagLoggedPrintf(PREFIX "Warning: GetThreadContext() for thread ID 0x%x failed with error %d (%s)\n",
				GetThreadId(hThreads[t]), diagErrorCodes::Win32ErrorToString(err, errorBuff, NELEM(errorBuff)));
#endif
		}

		CloseHandle(hThreads[t]);
	}
}

#endif // __WIN32PC

#if !__FINAL
// We don't want to be allocating memory in this code, since the memory allocators may be calling it!
const int REGISTERED_CAPTURE_DEPTH = 8;
const int MAX_REGISTERED_BACKTRACES = 3000;
const int REGISTERED_BACKTRACE_HASH_SIZE = 13297;		// prime; see http://www.addedbytes.com/blog/prime-numbers/ for more.

// Strictly speaking it only needs to be at least one larger, but that could lead to really long clumps.
CompileTimeAssert(REGISTERED_BACKTRACE_HASH_SIZE > MAX_REGISTERED_BACKTRACES * 4);

struct BacktraceState {
	size_t BacktraceHeap[MAX_REGISTERED_BACKTRACES][REGISTERED_CAPTURE_DEPTH];
	u16 RegisteredBacktraceHash[REGISTERED_BACKTRACE_HASH_SIZE];
	u16 BacktraceCount;
} *g_BacktraceState;
// CompileTimeAssertSize(BacktraceState, 0, 0);

u16 sysStack::RegisterBacktrace(int ignoreLevels)
{
	if (!g_BacktraceState) 
	{
#if ENABLE_DEBUG_HEAP
		{
			// Debug Heap
			sysMemAutoUseDebugMemory debug;

			// Call the OS-level allocator directly since this is very low-level code.
			g_BacktraceState = rage_new BacktraceState;
		}		
#endif
		// Deal with failure since the debug heap may not exist.
		if (!g_BacktraceState)
			return 0;

		memset(g_BacktraceState, 0, sizeof(BacktraceState));
	}
	BacktraceState *bts = g_BacktraceState;

	size_t temp[REGISTERED_CAPTURE_DEPTH];
	CaptureStackTrace(temp,REGISTERED_CAPTURE_DEPTH,ignoreLevels);
	size_t hash = 0;
	for (int i=0; i<REGISTERED_CAPTURE_DEPTH; i++)
		hash ^= temp[i];
	// LSB's are always zero
	hash >>= 2;
	hash %= REGISTERED_BACKTRACE_HASH_SIZE;

	// Find an empty slot that doesn't match our trace already
	while (bts->RegisteredBacktraceHash[hash] && memcmp(temp, bts->BacktraceHeap[bts->RegisteredBacktraceHash[hash]-1], REGISTERED_CAPTURE_DEPTH * sizeof(u32))) {
		if (++hash == REGISTERED_BACKTRACE_HASH_SIZE)
			hash = 0;
	}
	if (!bts->RegisteredBacktraceHash[hash]) {
		if (Verifyf(bts->BacktraceCount < MAX_REGISTERED_BACKTRACES,"Too many unique callstacks (%d), new ones will not be remembered.",bts->BacktraceCount)) {
			sysMemCpy(bts->BacktraceHeap[bts->BacktraceCount], temp, sizeof(bts->BacktraceHeap[bts->BacktraceCount]));
			bts->RegisteredBacktraceHash[hash] = ++(bts->BacktraceCount);
			// Displayf("************* %d unique callstacks so far",BacktraceCount);
		}
	}
	// This may be zero if we ran out of buffer slots.
	return bts->RegisteredBacktraceHash[hash];
}

void sysStack::PrintRegisteredBacktrace(u16 bt, void (*DisplayFn)(size_t,const char*,size_t))
{
	if (bt && g_BacktraceState)
		PrintCapturedStackTrace(g_BacktraceState->BacktraceHeap[bt-1],REGISTERED_CAPTURE_DEPTH, DisplayFn);
	else
		Warningf("[Null backtrace index, no callstack available]");
}
#endif		// !__FINAL

#endif		// __NO_OUTPUT
