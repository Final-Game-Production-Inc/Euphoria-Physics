//
// system/main.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_MAIN_H
#define SYSTEM_MAIN_H

/*
	Configuration options for this file, via the preprocessor:

		If __WINMAIN is nonzero on Win32/Xbox builds, we declare WinMain instead of main,
		effectively removing the console window.

		Unless we're in a __TOOL build or USE_STOCK_ALLOCATOR is defined, SIMPLE_HEAP_SIZE
		will default to 128M if it's not already defined to something else.

		If SIMPLE_HEAP_SIZE is defined, we set up a heap of that size (in kilobytes)
		prior to entering Main.  If SIMPLE_PHYSICAL_SIZE is not set, it defaults to the
		same value as SIMPLE_HEAP_SIZE.  On PS3, all virtual memory not given to
		SIMPLE_HEAP_SIZE is taken by the virtual resource heap manager.  SIMPLE_PHYSICAL_SIZE
		defines the size of the unified resource heap on Xenon.  It defines the size of the
		physical resource heap on PS3.  Any physical (VRAM) memory not consumed by the physical
		resource heap on PS3 is left for the game resource heap (what physical_new uses, and
		are what all render targets, non-resourced graphics objects, etc, draw from).

		If SIMPLE_OVERHEAD_SIZE is defined (on __XENON builds only for now) we don't use
		SIMPLE_PHYSICAL_SIZE directly; instead we get the actual amount of available memory,
		subtract off the game heap size, and the desired overhead size, and return the
		remainder to the physical heap.

		On PS3, GCM_BUFFER_SIZE defines the initial GCM command buffer size, in kilobytes.
		If not specified, it defaults to 1024k.  We do this to speed up memory translation for GCM and to allow
		any normal game heap object to be usable as a GPU resource; it avoids requiring
		yet another separate heap.

		On PS3, we also define system module loading here.  We load all of the modules we currently link 
		with, and it can be extended by #defining CELL_EXTRA_SYSMODULES before including main.h.

		If NO_XMEM_WRAPPERS is defined on Xenon builds, we don't declare wrapper functions
		which thunk down to rage wrappers of the real xenon functions; currently the rage
		versions just facilitate better memory tracking.  If you need to explicitly replace
		XTL memory management yourself, define this symbol and then make sure you declare
		replacement versions of XMemAlloc, etc in an extern "C" block in your top level .cpp file.

		If NO_INIT_GAME_HEAP is defined, 'main.h' doesn't define the InitGameHeap() function,
		allowing the user to define a custom one instead, in case the standard configuration options
		are insufficient.

		If PGKNOWNREFPOOLSIZE is defined, it's used to override the size of pgBaseKnownReferencePool.

		You trigger a response file by including the first parameter on the command line:
		testgame @c:\whatever.txt will parse c:\whatever.txt for parameters; quotes are properly 
		stripped as necessary from parameters.  Any additional parameters on the original command
		line are patched in at the end as well; this is useful for flags like -localhost.  File 
		reading is done well before any asset manager is set up, so paths must be fully-qualified.
		On Xbox this happens after rfs is installed so the argfile can be on your local pc.
		The response file itself may contain other @c:\otherfile.txt references (recursively,
		if necessary) so that complex response files can be built up out of simpler pieces.

		If you run any application with no command line at all on Xenon, it will look for
		d:\commandline.txt; if that file exists, it will be used as an implicit response file.
		We only look on the "real" d: drive, not RFS, so it may be either burned onto the
		disc or in the emulation hard drive root.  On all platforms, you can also place the
		macro SET_DEFAULT_RESPONSE_FILE in your top level code to use a different default response
		file.  This replaces d:\commandline.txt on Xenon builds.

		You can disable RFS entirely (not just on Final builds) by using #define DISABLE_RFS before
		including this file.

		Also note that any command line parameter can be prefixed with a three-letter platform
		code -- pc: (for PC), xe: (for Xenon), p3: (for PlayStation3), du: (for Durango), or or: (for Orbis).
		If the current platform doesn't match this prefix, then the parameter is ignored.
		You can also use ! here to negate the meaning of the test:
			-pc:whatever			-whatever on PC, nothing on all other builds
			-!xe:whatever			Nothing on Xenon, -whatever on all other builds.
		You can also specify a particular configuration via -de: (Debug), -be: (Beta), -re: (Release), -x8: (!64-bit), and -x6: (64-bit).
		More than one of these tests can be strung together, and all must pass:
			-pc:be:whatever			-whatever on PC beta build, nothing on all other builds
			-!xe:de:whatever		-whatever on all non-Xenon debug builds, nothing on others.
		If you need OR functionality, simply specify the parameter on the command line a second time
		with the additional checks, but that's starting to get a little crazy.

        Use ;, #, or // to begin a one-line comment.  All characters to the end of the
        line are ignored.

		Current known memory overhead on PS3: (Release or Final builds) (these numbers are several years out of date)
			1024k for default PRX's (and USBD)
			1328k for primary SPURS group (5 SPU's)
			 304k for secondary SPURS group (ghetto SPU)
			  32k for spu_printf_service
			=====
			2686k total overhead; this is all allocated before our memory manager is set up
				(with the possible exception of the GCM debug PRX)
			192k for exception handling (64k for dummy thread stack, 128l for lv2 PRX)
			Everything else seems to be thread stacks allocated directly or indirectly by the app.

		To reserve memory for this on PS3 builds, set EXTERNAL_HEAP_SIZE to the size (in kilobytes)
		of memory to reserve and be doled out by sysMemPhysicalAllocate (which is what SimpleAllocator
		uses to back its memory.  This amount is expressed in kilobytes, but is rounded up to megabytes
		when it's used in the current implementation.

		The amount of memory to leave to the OS is dictated by THREAD_STACK_SIZE, also in kilobytes.
		This amount should be enough for all thread stacks, and the TLS for each thread (which is managed
		by the OS in 64k blocks; the amount of TLS state per thread ought to be minimal though).

		The main game heap and the external heap pool are both allocated with 1M granularity, which means that 
		relatively small changes in static code and data size may result in gaining or losing a megabyte of space.
*/

#include "system/applicationview_durango.winrt.h"
#include "system/bootmgr.h"
#include "system/debugmemoryfill.h"
#include "system/exception.h"
#include "system/rageroot.h"
#include "system/memory.h"
#include "system/memmanager.h"
#include "system/nelem.h"
#include "system/new.h"
#include "system/ipc.h"
#include "system/stack.h"
#include "system/xtl.h"
#include "data/struct.h"
#include "data/marker.h"
#include "diag/tracker.h"
#include "data/resourceheader.h"
#include "paging/base.h"

#if RSG_ORBIS
#include <kernel.h>
#include <sdk_version.h>
#endif

#if RSG_DURANGO
namespace rage
{
	extern size_t CalculateGraphicsMemBufferAlignment(size_t buffersize);
	//extern void RegisterGraphicsMemBuffer(void *pAddr, size_t buffersize);
	extern void SetPhysicalMemoryArea(void *pAddr, size_t buffersize);
	extern void SetAsUsableForInPlaceSwizzling(void *pAddr, size_t buffersize);
}
#endif

#ifdef WANT_PORTCULLIS
#include "portcullis/portcullis.h"
#else
#define PORTCULLIS_ENABLE 0
#endif

#ifdef RAGE_TRACKING
#include "file/tcpip.h"
#include "diag/tracker_remote2.h"
#include "system/memvisualize.h"
#endif

#if MEMORY_TRACKER
#include "system/systemallocator_system.h"
#endif

#define SET_DEFAULT_RESPONSE_FILE(x) namespace rage { const char *g_DefaultResponseFile = x; }

#if __FINAL
#define FINAL_ONLY(x) x
#else
#define FINAL_ONLY(x)
#endif

#if !__TOOL && !defined(USE_STOCK_ALLOCATOR) && !defined(SIMPLE_HEAP_SIZE)
# if RSG_ORBIS && !__OPTIMIZED
# define SIMPLE_HEAP_SIZE	(512*1024)
# else
# define SIMPLE_HEAP_SIZE	(128*1024)
# endif
#endif

#if !__TOOL && !defined(FLEX_HEAP_SIZE)
#if RSG_ORBIS
#define FLEX_HEAP_SIZE (0*1024)
#endif
#endif

#if !__TOOL && !defined(HEMLIS_HEAP_SIZE)
#define HEMLIS_HEAP_SIZE (0*1024)
#endif

#if !__TOOL && !defined(RECORDER_HEAP_SIZE)
#define RECORDER_HEAP_SIZE (0 * 1024)
#endif

#if __XENON
#if !defined(SIMPLE_COMBINED_SIZE)
#define SIMPLE_COMBINED_SIZE (SIMPLE_HEAP_SIZE + SIMPLE_PHYSICAL_SIZE)
#endif

#if RESOURCE_HEADER
#if !defined(HEADER_HEAP_SIZE)
#define HEADER_HEAP_SIZE (0)
#endif
#endif

#if !defined(SIMPLE_POOL_SIZE)
#define SIMPLE_POOL_SIZE (16*1024)
#endif
#endif

#if defined (SIMPLE_OVERHEAD_SIZE) && __XENON
#define SIMPLE_PHYSICAL_SIZE SimplePhysicalSize()
#include "system/xtl.h"
static DWORD SimplePhysicalSize()
{
	static DWORD result;
	if (!result)
	{
		MEMORYSTATUS ms;
		GlobalMemoryStatus(&ms);
		// Note that the game heap has already been allocated and constructed by now.
		result = (ms.dwAvailPhys>>10) - (SIMPLE_OVERHEAD_SIZE);
		rage::Displayf("%uk avail after game heap setup, after overhead physical heap is %uk", ms.dwAvailPhys>>10,result);
	}
	return result;
}
#endif

#if defined (SIMPLE_HEAP_SIZE) && !defined (SIMPLE_PHYSICAL_SIZE)
 #if SIMPLE_HEAP_SIZE > 248*1024
 #define SIMPLE_PHYSICAL_SIZE (248*1024)
 #else
 #define SIMPLE_PHYSICAL_SIZE SIMPLE_HEAP_SIZE
 #endif
#endif

#if __PS3
 // VRAM_GAME_RESERVED should be specified for forcing some non-vertex data to
 // the last addresses of RSX local.
 //
 // Some PS3s have faulty memory that corrupts bits.  If an index gets
 // corrupted, then it can cause a vertex access outside of mapped memory.  By
 // placing non-vertex data (eg. render targets) at the end of vram, this
 // problem can be partially avoided (vertex buffers on main could still fail
 // though).  See GTAV B*1409529 for an example.
 //
 // By specifying atleast 8160KiB, a single bit corruption of an index is
 // prevented from causing a graphics error 258 on an RSX local vertex fetch.
 // This value is based on the maximum vertex stride of 255 supported by the
 // RSX.
 //
 #if !defined (VRAM_GAME_RESERVED)
 #define VRAM_GAME_RESERVED 0
 #endif
#endif

#if !defined(DEBUG_HEAP_SIZE)
#	if RSG_PC
#		define DEBUG_HEAP_SIZE 64*1024
#	elif (RSG_DURANGO || RSG_ORBIS)
#		define DEBUG_HEAP_SIZE 32*1024
#	else
#		define DEBUG_HEAP_SIZE 10*1024
#	endif
#endif // !defined(DEBUG_HEAP_SIZE)

#if !defined(REPLAY_HEAP_SIZE)
#	if RSG_DURANGO || RSG_ORBIS || RSG_PC
#		define REPLAY_HEAP_SIZE (0)
#	endif
#endif // !defined(DEBUG_HEAP_SIZE)

#ifndef SIMPLE_PHYSICAL_NODES
#define SIMPLE_PHYSICAL_NODES 8192
#endif

#ifdef SIMPLE_HEAP_SIZE
#include "system/simpleallocator.h"
#include "system/externalallocator.h"
#if ENABLE_BUDDY_ALLOCATOR
#include "system/buddyallocator.h"
#else
#include "system/virtualallocator.h"
#endif
#include "system/multiallocator.h"
#include "system/sparseallocator.h"
#else
#include "system/stockallocator.h"
#endif

#if RSG_ORBIS
#include <gnm.h>
// https://orbis.scedev.net/technotes/view/15/1
size_t sceLibcHeapSize = SCE_LIBC_HEAP_SIZE_EXTENDED_ALLOC_NO_LIMIT;
unsigned int sceLibcHeapExtendedAlloc = 1; 
#endif

#if API_HOOKER
#include "system/minhook.h"

typedef PVOID(WINAPI* VirtualAllocHooker)(void*, size_t, unsigned long, unsigned long);
extern VirtualAllocHooker g_virtualAllocHooker;

typedef BOOL(WINAPI* VirtualFreeHooker)(void*, size_t, unsigned long);
extern VirtualFreeHooker g_virtualFreeHooker;

typedef PVOID(WINAPI* HeapAllocHooker)(void*, unsigned long, size_t);
extern HeapAllocHooker g_heapAllocHooker;

typedef PVOID(WINAPI* HeapReAllocHooker)(void*, unsigned long, void*, size_t);
extern HeapReAllocHooker g_heapReAllocHooker;

typedef BOOL(WINAPI* HeapFreeHooker)(void*, unsigned long, void*);
extern HeapFreeHooker g_heapFreeHooker;

typedef PVOID(WINAPI* LocalAllocHooker)(unsigned int, size_t);
extern LocalAllocHooker g_localAllocHooker;

typedef HLOCAL(WINAPI* LocalReAllocHooker)(void*, size_t, unsigned int);
extern LocalReAllocHooker g_localReAllocHooker;

typedef HLOCAL(WINAPI* LocalFreeHooker)(void*);
extern LocalFreeHooker g_localFreeHooker;

typedef HLOCAL(WINAPI* GlobalFreeHooker)(void*);
extern GlobalFreeHooker g_globalFreeHooker;

void InitHooker()
{
	// Initialize MinHook.
	MH_STATUS status = MH_Initialize();
	if (status == MH_OK)
	{
		// VirtualAlloc
		status = MH_CreateHookEx(&VirtualAlloc, &VirtualAllocHooked, &g_virtualAllocHooker);
		if (status == MH_OK)
		{
			status = MH_EnableHook(&VirtualAlloc);
			if (status != MH_OK)
				Errorf("MH_EnableHook(&VirtualAlloc) failed: 0x%X", status);
		}
		else
			Errorf("MH_CreateHookEx(&VirtualAlloc, &VirtualAllocHooked, &g_virtualAllocHooker) failed: 0x%X", status);

		// VirtualFree
		status = MH_CreateHookEx(&VirtualFree, &VirtualFreeHooked, &g_virtualFreeHooker);
		if (status == MH_OK)
		{
			status = MH_EnableHook(&VirtualFree);
			if (status != MH_OK)
				Errorf("MH_EnableHook(&VirtualFree) failed: 0x%X", status);
		}
		else
			Errorf("MH_CreateHookEx(&VirtualFree, &VirtualFreeHooked, &g_virtualFreeHooker) failed: 0x%X", status);

		// HeapAlloc
		status = MH_CreateHookEx(&HeapAlloc, &HeapAllocHooked, &g_heapAllocHooker);
		if (status == MH_OK)
		{
			status = MH_EnableHook(&HeapAlloc);
			if (status != MH_OK)
				Errorf("MH_EnableHook(&HeapAlloc) failed: 0x%X", status);
		}
		else
			Errorf("MH_CreateHookEx(&HeapAlloc, &HeapAllocHooked, &g_heapAllocHooker) failed: 0x%X", status);

		// HeapReAlloc
		status = MH_CreateHookEx(&HeapReAlloc, &HeapReAllocHooked, &g_heapReAllocHooker);
		if (status == MH_OK)
		{
			status = MH_EnableHook(&HeapReAlloc);
			if (status != MH_OK)
				Errorf("MH_EnableHook(&HeapReAlloc) failed: 0x%X", status);
		}
		else
			Errorf("MH_CreateHookEx(&HeapAlloc, &HeapAllocHooked, &g_heapAllocHooker) failed: 0x%X", status);

		// HeapFree
		status = MH_CreateHookEx(&HeapFree, &HeapFreeHooked, &g_heapFreeHooker);
		if (status == MH_OK)
		{
			status = MH_EnableHook(&HeapFree);
			if (status != MH_OK)
				Errorf("MH_EnableHook(&HeapFree) failed: 0x%X", status);
		}
		else
			Errorf("MH_CreateHookEx(&HeapFree, &HeapFreeHooked, &g_heapFreeHooker) failed: 0x%X", status);

		// LocalFree
		status = MH_CreateHookEx(&LocalFree, &LocalFreeHooked, &g_localFreeHooker);
		if (status == MH_OK)
		{
			status = MH_EnableHook(&LocalFree);
			if (status != MH_OK)
				Errorf("MH_EnableHook(&LocalFree) failed: 0x%X", status);
		}
		else
			Errorf("MH_CreateHookEx(&LocalFree, &LocalFreeHooked, &g_localFreeHooker) failed: 0x%X", status);

		// GlobalFree
		status = MH_CreateHookEx(&GlobalFree, &GlobalFreeHooked, &g_globalFreeHooker);
		if (status == MH_OK)
		{
			status = MH_EnableHook(&GlobalFree);
			if (status != MH_OK)
				Errorf("MH_EnableHook(&GlobalFree) failed: 0x%X", status);
		}
		else
			Errorf("MH_CreateHookEx(&GlobalFree, &GlobalFreeHooked, &g_globalFreeHooker) failed: 0x%X", status);
	}
	else
		Errorf("MH_Initialize failed: 0x%X", status);
}
#endif

#if !__TOOL

#if __PPU
#include <stdlib.h>
#include <string.h>
#include <sys/prx.h>
#include <sys/memory.h>
#include <cell/sysmodule.h>
#include <unistd.h>
#include <sys/vm.h>
#include <sys/timer.h>
#include "system/splitallocator.h"
#include "system/tinyheap.h"
#include "system/virtualmemory_psn.h"

#if !__FINAL
#include <sys/dbg.h>
extern "C" int snIsDebuggerRunning (void);
extern "C" int snIsTunerRunning (void);
#endif
#include <sysutil/sysutil_sysparam.h>
#include "grprofile/pix.h"			// for GCM_HUD #define

# ifndef GCM_BUFFER_SIZE
# define GCM_BUFFER_SIZE	1024
# endif

# ifdef MEMORY_CONTAINER_SIZE
# error "MEMORY_CONTAINER_SIZE is no longer used; use EXTERNAL_HEAP_SIZE and THREAD_STACK_SIZE instead."
# endif

namespace rage
{
	size_t g_GcmInitBufferSize;
	size_t g_GcmMappingSize;
	void* g_GcmHeap;
	void* g_MainHeap;

	sysMemAllocator* g_pResidualAllocator;	
	u32 g_bDontUseResidualAllocator;

	// Spurs setup; it allocates system memory, so grab it first before we do our heap measurements.
	extern void sysTaskManager__SysInitClass();
	// Second portion contains things that do not allocate system memory
	// - And in fact would like the general memory allocators set up for their own use
	extern void sysTaskManager__SysInitClass2();
	extern void sysMemPhysicalSetup(size_t);
}

#endif	// __PPU

#if __WIN32 
namespace rage
{
	extern size_t* g_DebugData;
}
#endif

namespace rage
{
	extern bool (*g_pProjectPrologue)(void);
	extern bool (*g_pProjectMainOrDoOneLoop)(void);
	extern void (*g_pProjectEpilogue)(void);
}

#define SET_APP_ENTRY_POINTS(pro, loop, epi) \
static struct app_entry_point_setter_t \
{ \
	app_entry_point_setter_t() \
	{ \
		g_pProjectPrologue = pro; \
		g_pProjectMainOrDoOneLoop = loop; \
		g_pProjectEpilogue = epi; \
} \
} app_entry_point_setter_instance; \


#if __XENON
namespace rage 
{
	extern sysMemAllocator* g_pXenonPoolAllocator;
	NOTFINAL_ONLY(size_t g_sysExtraMemory;)
}
#endif // __XENON

#if defined(__MFC) || RSG_ORBIS || RSG_DURANGO || BACKTRACE_ENABLED
namespace rage { sysMemAllocator* s_pTheAllocator; }
#endif

#ifndef EXTERNAL_HEAP_SIZE
#define EXTERNAL_HEAP_SIZE	1024
#endif
#ifndef THREAD_STACK_SIZE
#define THREAD_STACK_SIZE	768
#endif
#ifndef VIRTUAL_HEAP_SIZE
#define VIRTUAL_HEAP_SIZE	(128*1024)
#endif

#if __CONSOLE
#if __FINAL
// FINAL builds always restrict to testkit mem size.  It is 'final' after all.
#if ENABLE_DEBUG_HEAP
#define IsActualMem() false
#else
#define IsActualMem() true
#endif // ENABLE_DEBUG_HEAP
#else
#include "system/param.h"
// Command-line flags for manipulating the heaps include:
//	-actualmem -> simulate actual memory config, even on devkit
inline bool IsActualMem()
{
	return rage::sysParam::FindCommandLineArg("-actualmem");
};
#endif // __FINAL

#if __XENON && !__FINAL
#include <xbdm.h>
#pragma comment(lib,"xbdm.lib")
#endif

#if ENABLE_DEBUG_HEAP
namespace rage 
{
	extern size_t g_sysExtraStreamingMemory;
}

size_t GetExtraStreamingMemory()
{
#if __PS3
	int argc = getargc();
	char** argv = getargv();
	for (int i = 1; i < argc; i++)
	{
		const char* pszParam = argv[i];
		if (NULL != strstr(pszParam, "@extrastreamingmemory"))
			return (32 << 10);
	}
#elif __XENON
	LPSTR cmd = GetCommandLineA();
	const char* pszParam = strstr(cmd, "@extrastreamingmemory");
	if (NULL != pszParam)
		return (32 << 10);
#endif

	return 0;
}
#endif

size_t GetTotalOSMemory()
{
	return (size_t)(PS3_ONLY(43) XENON_ONLY(32) DURANGO_ONLY(3*1024) ORBIS_ONLY(static_cast<size_t>(3584)) << 20);
}

size_t GetTotalRetailMemory()
{
	return (size_t)(PS3_ONLY(256) XENON_ONLY(512) DURANGO_ONLY(5*1024) ORBIS_ONLY(static_cast<size_t>(4608)) << 20);
}

size_t GetTotalDevkitMemory()
{
	return (size_t)(PS3_ONLY(450) XENON_ONLY(1024) DURANGO_ONLY(5*1024) ORBIS_ONLY(static_cast<size_t>(5376)) << 20);
}

size_t GetTotalUserMemory()
{
#if __PS3
	sys_memory_info_t mem_info;
	sys_memory_get_user_memory_size( &mem_info );	
	return mem_info.total_user_memory;
#elif __XENON
	MEMORYSTATUS mem_status;
	GlobalMemoryStatus(&mem_status);
	return mem_status.dwTotalPhys;
#elif RSG_DURANGO
	TITLEMEMORYSTATUS status;
	status.dwLength =  sizeof(TITLEMEMORYSTATUS);
	TitleMemoryStatus(&status);
	return status.ullTotalMem;
#elif RSG_ORBIS
	return sceKernelGetDirectMemorySize();
#endif
}

size_t GetUsedUserMemory()
{
#if __PS3
	sys_memory_info_t mem_info;
	sys_memory_get_user_memory_size(&mem_info);	
	return mem_info.total_user_memory - mem_info.available_user_memory;
#elif __XENON
	MEMORYSTATUS mem_status;
	GlobalMemoryStatus(&mem_status);
	return mem_status.dwTotalPhys - mem_status.dwAvailPhys;
#elif RSG_ORBIS
	/* https://ps4.scedev.net/forums/thread/7291/
	   https://ps4.scedev.net/forums/thread/21183/
	   You can use sceKernelVirtualQuery to get information about all mapped memory. You have to call it for each area, but if you 
	   call it with SCE_KERNEL_VQ_FIND_NEXT specified then you can just pass the returned info->end address in to the next call until 
	   it returns SCE_KERNEL_ERROR_EACCES to iterate over every block.
	 */
	size_t total = 0;
	int32_t result = 0;
	void* pAddress = NULL;
	SceKernelVirtualQueryInfo info;

	while ((result = sceKernelVirtualQuery(pAddress, SCE_KERNEL_VQ_FIND_NEXT, &info, sizeof(SceKernelVirtualQueryInfo))) != SCE_KERNEL_ERROR_EACCES)
	{
		pAddress = info.end;
		const size_t size = reinterpret_cast<size_t>(info.end) - reinterpret_cast<size_t>(info.start);				
		total += size;
	}

	return total;
#elif RSG_DURANGO
	TITLEMEMORYSTATUS status;
	status.dwLength =  sizeof(TITLEMEMORYSTATUS);
	TitleMemoryStatus(&status);
	return status.ullTotalMem - status.ullAvailMem;
#endif
}

size_t GetAvailableUserMemory()
{
#if __PS3
	sys_memory_info_t mem_info;
	sys_memory_get_user_memory_size(&mem_info);	
	return mem_info.available_user_memory;
#elif __XENON
	MEMORYSTATUS mem_status;
	GlobalMemoryStatus(&mem_status);
	return mem_status.dwAvailPhys;
#elif RSG_DURANGO
	TITLEMEMORYSTATUS status;
	status.dwLength =  sizeof(TITLEMEMORYSTATUS);
	TitleMemoryStatus(&status);
	return status.ullAvailMem;
#elif RSG_ORBIS
	return GetTotalUserMemory() - GetUsedUserMemory();
#endif
}

size_t GetTotalExtraMemory()
{
#if __PS3
	return IsActualMem() ? 0 : ((GetTotalUserMemory() - GetTotalRetailMemory()) + GetTotalOSMemory());
#elif __XENON && !__FINAL
	DWORD dwDebugMemorySize;
	DmGetDebugMemorySize(&dwDebugMemorySize);
	return (dwDebugMemorySize << 20);
#elif RSG_DURANGO && !__FINAL
	// This is horrable as the old xdk can't DebugMemGetRegion always returns zero on the newer system updates 
	// ToolingMemoryStatus is what should be used but is not avaliable on the older xdk's :(
	TITLEMEMORYSTATUS status;
	status.dwLength =  sizeof(TITLEMEMORYSTATUS);
	TitleMemoryStatus(&status);
	return status.ullTotalMem - 5167382528; // Default Title memory avaliable is 4928 MB
#elif RSG_ORBIS
	return GetTotalUserMemory() - GetTotalRetailMemory();
#else
	return 0;
#endif
}

bool IsDevkit()
{
	return GetTotalExtraMemory() > 0;
}

bool HasExtraMemory()
{
	return IsDevkit() && !IsActualMem();
}
#endif

#if __FINAL && __PPU
bool ShouldWeCrashTheGameIfQAHasMisconfiguredATestKitWhenRunningTheGameInPS3Final()
{	
	const size_t userOSMemoryTotal = GetTotalUserMemory() + GetTotalOSMemory();
	Memoryf("[STARTUP] PS3 Available + OS Memory = %d / %d KB\n", userOSMemoryTotal >> 10);

	if (userOSMemoryTotal < (255 << 20))
	{
		int argc = getargc();
		char **argv = getargv();
		for (int i=1; i<argc; i++)
		{
			Memoryf("argv[%d] = %s\n", i, argv[i]);

			if (NULL != strstr(argv[i], "@nocrash"))
			{
				Memoryf("ShouldWeCrashTheGameIfQAHasMisconfiguredATestKitWhenRunningTheGameInPS3Final = %d\n", false);
				return false;
			}
		}

		Memoryf("ShouldWeCrashTheGameIfQAHasMisconfiguredATestKitWhenRunningTheGameInPS3Final = %d\n", true);
		return true;
	}

	Memoryf("ShouldWeCrashTheGameIfQAHasMisconfiguredATestKitWhenRunningTheGameInPS3Final = %d (2)\n", false);
	return false;
}
#endif

#if __PS3 && !__NO_OUTPUT
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// [DEBUG]
// Functions to check if we're running on a devkit, etc.
//
bool IsGcmDebugEnabled()
{
	const int maxModules = 32;
	rage::u8 buffer[sizeof(sys_prx_id_t) * maxModules];
	sys_prx_get_module_list_t moduleList;
	moduleList.size = sizeof(sys_prx_get_module_list_t);
	moduleList.idlist = (sys_prx_id_t*)buffer;
	moduleList.max = maxModules;

	int ret = sys_prx_get_module_list(0, &moduleList);
	if( ret == CELL_OK )
	{
		for( int i = 0; i < moduleList.count; i++ )
		{
			sys_prx_module_info_t info;
			info.size = sizeof(sys_prx_module_info_t);
			info.segments = NULL;
			info.segments_num = 0;

			rage::u8 filenameBuffer[64];
			info.filename = (char*)filenameBuffer;
			info.filename_size = 64;

			ret = sys_prx_get_module_info(moduleList.idlist[i], 0, &info);
			if( ret == CELL_OK )
			{
				//Startupf("%s -- %s\n", info.name, info.filename);
				if(strstr(info.filename, "libgcm_sys_deh.sprx"))
				{
					// this is the gcm debug prx
					return true;
				}
				else if (strstr(info.filename, "libgcm_sys.sprx"))
				{
					// this is the non-debug prx
					return false;
				}
			}
		}
	}
	return false;
}
#endif // !__FINAL && __PS3

namespace rage
{
	// This gets used by 'system/main.cpp' as the size of pgBaseKnownReferencePool.
#ifdef PGKNOWNREFPOOLSIZE
	int g_KnownRefPoolSize = PGKNOWNREFPOOLSIZE;
#else
	int g_KnownRefPoolSize = -1;
#endif
}

#if __PPU
#if EXCEPTION_HANDLING
#pragma comment(lib, "lv2dbg_stub")
#endif
#pragma comment(lib,"sysmodule_stub")
#endif

#if !defined(NO_INIT_GAME_HEAP)

bool g_MemVizEnabled;

static void InitGameHeap()
{
	using namespace rage;

#if __XENON && !__FINAL
	// Send custom message to QAxbWatson to tell it to start logging again.
	// Generally this is unnecessary, since QA Menu will restart QAxbWatson, and
	// logging is enabled by default.  But for debugging purposes where
	// QAxbWatson was run manually, then logging stopped after an exception and
	// reboot, this can be useful.
	DmSendNotificationString("R*!LOG_START");
#endif

#if RSG_ORBIS || (defined(__RGSC_DLL) && __RGSC_DLL)
	// std allocates before our heap is allocated in release builds
	static bool inited = false;
	if(!inited)
	{
		inited = true;
	}
	else
	{
		return;
	}
#endif


#ifdef EARLY_INIT_CALLBACK
	EARLY_INIT_CALLBACK();
#endif

#if ENABLE_DEBUG_HEAP
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [DEBUG]
	//
#if RSG_PC
	size_t debugMemorySize = (DEBUG_HEAP_SIZE) << 10;

	const char* debugMemOverrideCmd = "@debugMemory";
	const char* cmdPtr = NULL;
	
	LPSTR cmd = GetCommandLine();
	cmdPtr = strstr(cmd, debugMemOverrideCmd);

	if (cmdPtr && cmdPtr[12] == '=')
		debugMemorySize = ((size_t)atoi(cmdPtr + 13)) << 20;

	if (debugMemorySize >= ((size_t)2 << 30))
		Quitf("Debug memory size must be less than 2048 MB, attempted to use @debugMemory=%d", (int)(debugMemorySize >> 20));

	Memoryf("[STARTUP] @debugMemory     = %u MB\n", (debugMemorySize >> 20));
#else
	const size_t debugMemorySize = (DEBUG_HEAP_SIZE) << 10;
	CompileTimeAssert(0 == (debugMemorySize & 0xFFFFF)); // Make sure we are 1MB aligned
#endif

#endif

#ifdef SIMPLE_HEAP_SIZE
#if __PPU
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [INIT]
	//
	#define VRAM_SIZE	(249*1024)
	static char stdout_buffer[__FINAL? 128 : 512];
	setvbuf(stdout, stdout_buffer, _IOLBF, sizeof(stdout_buffer));
	setvbuf(stderr, NULL, _IOLBF, 0);

#if EXCEPTION_HANDLING
	bool disabletmloadoptionscheck = false;
	bool rockstartargetmanager = false;
#endif

#if EXCEPTION_HANDLING || SUPPORT_DEBUG_MEMORY_FILL
	int argc = getargc();
	char **argv = getargv();
	for (int i=1; i<argc; i++)
	{
#		if EXCEPTION_HANDLING
			if (!strcmp(argv[i],"@disabletmloadoptionscheck"))
			{
				Memoryf("[STARTUP] Disabling PS3 TM load options check!\n");
				disabletmloadoptionscheck = true;
			}

			else if (!strcmp(argv[i],"-rockstartargetmanager"))
			{
				Memoryf("[STARTUP] -rockstartargetmanager\n");
				rockstartargetmanager = true;
			}
#		endif

#		if SUPPORT_DEBUG_MEMORY_FILL
			if (!strcmp(argv[i],"@nodebugmemoryfill"))
			{
				if (g_EnableDebugMemoryFill)
					Memoryf("[STARTUP] Disabling debug memory fill!\n");

				g_EnableDebugMemoryFill = false;
			}

			else if (!strncmp(argv[i],"@debugmemoryfill",16))
			{
				if (argv[i][16]=='=')
					g_EnableDebugMemoryFill = (u8)atoi(argv[i]+17);
				else
					g_EnableDebugMemoryFill = 255;
				Memoryf("[STARTUP] Enabling debug memory fill mask %d!\n",g_EnableDebugMemoryFill);
			}
#		endif
	}
#endif
	int result;
	size_t userMemorySize = GetAvailableUserMemory();

#if RAGE_MEMORY_DEBUG
	const size_t userMemoryTotal = GetTotalUserMemory();
	const size_t exeMemoryTotal = userMemoryTotal - userMemorySize;
	const size_t extraMemoryTotal = GetTotalExtraMemory();	
	size_t extraMemorySize = extraMemoryTotal;
#endif // RAGE_MEMORY_DEBUG

	Memoryf("[STARTUP] PS3 Actual Memory         = %s\n", IsActualMem() ? "YES" : "NO");
	Memoryf("[STARTUP] PS3 Kit Type              = %s\n", IsDevkit() ? "DEVKIT" : "TESTKIT");
	Memoryf("[STARTUP] PS3 Memory Total          = %d KB\n", ((userMemoryTotal + GetTotalOSMemory()) >> 10));
	Memoryf("[STARTUP] PS3 OS Memory             = %d KB\n", (GetTotalOSMemory() >> 10));
	Memoryf("[STARTUP] PS3 Self Memory           = %d KB\n", (exeMemoryTotal >> 10));
	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

#if __FINAL
	if (ShouldWeCrashTheGameIfQAHasMisconfiguredATestKitWhenRunningTheGameInPS3Final())
		sysMemManager::CrashTheGameIfQAHasMisconfiguredATestKitWhenRunningTheGameInPS3Final(GetTotalUserMemory() + GetTotalOSMemory());
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [DEBUG]
	//
#if RAGE_MEMORY_DEBUG && ENABLE_DEBUG_HEAP
	const size_t toolMemorySize = (5 << 20);
	CompileTimeAssert(0 == (toolMemorySize & 0xFFFFF)); // Make sure we are 1MB aligned

	// EJ: There is only extra memory on a devkit!
	if (IsDevkit())
	{
		extraMemorySize -= (debugMemorySize + toolMemorySize);
		Memoryf("[STARTUP] PS3 Debug Memory          = %d KB\n", (debugMemorySize >> 10));
		Memoryf("[STARTUP] PS3 Tool Memory           = %d KB\n", (toolMemorySize >> 10));
		Memoryf("[STARTUP] PS3 Extra Memory          = %d / %d KB\n", (extraMemorySize  >> 10), (extraMemoryTotal >> 10));				
	}
#endif // RAGE_MEMORY_DEBUG && ENABLE_DEBUG_HEAP

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [IGNORED]
	//
#if RAGE_MEMORY_DEBUG
	// EJ: Pre-allocate extra memory so it's never available
	if (IsDevkit())
	{
		if (IsActualMem())
		{
			Memoryf("[STARTUP] PS3 Ignored Memory        = %d / %d KB\n", ((extraMemorySize & ~0xfffff) >> 10), (extraMemorySize >> 10));

			sys_addr_t addr = NULL;
			extraMemorySize &= ~0xfffff;

			// EJ: Pre-allocate extra memory so it's never available
			if ((result = sys_memory_allocate(extraMemorySize, SYS_MEMORY_PAGE_SIZE_1M, &addr)) != CELL_OK)
				Quitf("[STARTUP] ERROR: Ignored Memory Allocation Failed! Code %x\n", result);

			userMemorySize = GetAvailableUserMemory();
			Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));
		}
	}
#endif // RAGE_MEMORY_DEBUG
	
	Memoryf("[STARTUP] PS3 VRAM Memory           = %d KB\n", VRAM_SIZE);
	Memoryf("[STARTUP] PS3 VRAM Render Targets   = %d / %d KB\n", VRAM_GAME_RESERVED, VRAM_SIZE);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [MODULES]
	//
	// Load modules first before we steal all remaining memory for ourselves.
	// This seems to burn about a megabyte
	/*
		CELL_SYSMODULE_SYSUTIL, CELL_SYSMODULE_GCM_SYS, CELL_SYSMODULE_AUDIO, CELL_SYSMODULE_IO, CELL_SYSMODULE_SYNC, 
		CELL_SYSMODULE_SPURS, CELL_SYSMODULE_OVIS, CELL_SYSMODULE_SHEAP, CELL_SYSMODULE_DAISY, CELL_SYSMODULE_FS, and 
		CELL_SYSMODULE_SYSUTIL_NP_TROPHY are automatically loaded when libsysmodule starts up. Therefore, the application 
		does not need to load them explicitly. Also, these PRX modules will not be unloaded with cellSysmoduleUnloadModule().
	*/
	static u16 modules[] = 
	{
		CELL_SYSMODULE_NET, CELL_SYSMODULE_VOICE, CELL_SYSMODULE_RTC, CELL_SYSMODULE_SYSUTIL_NP_TUS, CELL_SYSMODULE_SYSUTIL_GAME

#if !__FINAL
		,CELL_SYSMODULE_LV2DBG
#endif
	};

#if RAGE_MEMORY_DEBUG
	char* modules_name[] = {"Network", "Voice  ", "Time   ", "TUS    ", "Game   ", "Debug  "};
#endif // RAGE_MEMORY_DEBUG

	for (int i = 0; i < (sizeof(modules) / sizeof(u16)); ++i)
	{
		if (CELL_OK == cellSysmoduleIsLoaded(modules[i]))
		{
			Memoryf("[STARTUP] Module %x already resident?\n", modules[i]);
		}
		else
		{
#if RAGE_MEMORY_DEBUG
			sys_memory_info_t mem_info_pre;
			sys_memory_get_user_memory_size(&mem_info_pre);
#endif // RAGE_MEMORY_DEBUG
			if (CELL_OK != (result = cellSysmoduleLoadModule(modules[i])))
			{
				Memoryf("[STARTUP] Fatal error, cannot load module 0x%x (see sysmodule.h, code=%x)\n", modules[i], result);
				*(int*)0 = 0;
			}
#if RAGE_MEMORY_DEBUG
			sys_memory_info_t mem_info_post;
			sys_memory_get_user_memory_size(&mem_info_post);
			Memoryf("[STARTUP] Mod %s (0x%02X)        = %u KB\n", modules_name[i], modules[i],(mem_info_pre.available_user_memory - mem_info_post.available_user_memory) >> 10);
#endif // RAGE_MEMORY_DEBUG
		}
	}

	// EJ: Get the module size
	const size_t moduleMemorySize = userMemorySize - GetAvailableUserMemory();
	userMemorySize -= moduleMemorySize;
	Memoryf("[STARTUP] PS3 Module Memory         = %d KB\n", (moduleMemorySize >> 10));
	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [SYSTASK]
	//
	rage::sysTaskManager__SysInitClass();

	// NOTE: ejanderson - Get the module size
	const size_t sysTaskMemorySize = userMemorySize - GetAvailableUserMemory();
	userMemorySize -= sysTaskMemorySize;
	Memoryf("[STARTUP] PS3 SysTask Memory        = %d KB\n", (sysTaskMemorySize >> 10));
	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [EXCEPTION HANDLING]
	//
#if EXCEPTION_HANDLING
// Tuner 430 is forcably disabling exception handling, so we need to disable all these checks for now.
// https://ps3.scedev.net/support/issue/120928
if (!snIsTunerRunning())
{
	bool failBecauseOfBadTmOptions = false;

	// When the Rockstar Target Manager is running, it does the exception
	// handling.  But regardless of whether or not R*TM or a debugger is
	// running, still initialize (not register) exception handling.  This is
	// required for other sys_dbg_* functions to work.  Also works as a test the
	// the TM load options are correct.
	//
	// Default stack size for exception handler is 16KB, increased to 20KB after B*1032300 occurred.
	if ((result = sys_dbg_set_stacksize_ppu_exception_handler(20*1024)) != CELL_OK)
	{
		if (!disabletmloadoptionscheck)
		{
			Memoryf("################################\n");
			Memoryf("Failed to set exception handler stack size. Please make sure that exception handling it is enabled.\n");
			Memoryf("To enable. in the XMB, Goto Settings > Debug Setting > Exception Handler, and make sure it is set to on.\n");
			Memoryf("For more detailed instructions (with pictures) go to https://devstar.rockstargames.com/wiki/index.php/PS3_Exception_Handler_Setup\n");
			Memoryf("EXCEPTION HANDLING: sys_dbg_set_stacksize_ppu_exception_handler failed (%x)\n",result);
			Memoryf("################################\n");
			failBecauseOfBadTmOptions = true;
		}
	}
	else if ((result = sys_dbg_initialize_ppu_exception_handler(0x3e9)) != CELL_OK)
	{
		if (!disabletmloadoptionscheck)
		{
			Memoryf("################################\n");
			Memoryf("Failed to install exception handler. Please make sure that it is enabled.\n");
			Memoryf("To enable. in the XMB, Goto Settings > Debug Setting > Exception Handler, and make sure it is set to on.\n");
			Memoryf("For more detailed instructions (with pictures) go to https://devstar.rockstargames.com/wiki/index.php/PS3_Exception_Handler_Setup\n");
			Memoryf("EXCEPTION HANDLING: sys_dbg_initialize_ppu_exception_handler failed (%x)\n",result);
			Memoryf("################################\n");
			failBecauseOfBadTmOptions = true;
		}
	}
	else if (snIsDebuggerRunning() || snIsTunerRunning() || rockstartargetmanager)
	{
		Memoryf("EXCEPTION HANDLING: debugger, tuner or R*TM detected, not installing.\n");
		g_EnableExceptionHandling = rockstartargetmanager;
	}
	else if ((result = sys_dbg_register_ppu_exception_handler(sysStack::ExceptionCallback,SYS_DBG_PPU_THREAD_STOP | SYS_DBG_SPU_THREAD_GROUP_STOP)) == CELL_OK)
	{
		g_EnableExceptionHandling = true;
	}
	else if (!disabletmloadoptionscheck)
	{
		Memoryf("EXCEPTION HANDLING: register_ppu_exception_handler failed (%x)",result);
		failBecauseOfBadTmOptions = true;
	}

	if (!disabletmloadoptionscheck)
	{
		// Make sure coredumps are enabled.
		sys_dbg_coredump_parameter_t coredumpParams;
		if (sys_dbg_get_coredump_params(&coredumpParams) != CELL_OK || coredumpParams == SYS_DBG_COREDUMP_OFF)
		{
			Memoryf("################################\n");
			Memoryf("Coredump not enabled.  Must be enabled in the Target Manager, under\n");
			Memoryf("Properties > Target > Load Options > Core Dump > ...\n");
			Memoryf("See https://devstar.rockstargames.com/wiki/index.php/PS3_core_dump for more information.\n");
			Memoryf("################################\n");
			failBecauseOfBadTmOptions = true;
		}

		// Make sure GCM debug is enabled.  "RSX profiling tool" and "HUD" can still be disabled.
		if (!IsGcmDebugEnabled())
		{
			// When booting from the XMB, GCM debug cannot be enabled, so we
			// don't want to generate an error.  Note that using argv[0] like
			// this is a TRC violation; do not copy this to shipping code.
			char **argv = getargv();
			if (!strstr(argv[0], "EBOOT.BIN"))
			{
				Memoryf("################################\n");
				Memoryf("GCM debug not enabled.  Must be enabled in the Target Manager, under\n");
				Memoryf("Properties > Target > Load Options > GCM Debug > Enable GCM debug\n");
				Memoryf("See https://devstar.rockstargames.com/wiki/index.php/PS3_core_dump for more information.\n");
				Memoryf("################################\n");
				failBecauseOfBadTmOptions = true;
			}
		}

		// Make sure MAT is disabled.  We've had performance bugs raised because
		// people have accidentally turned this on :/ If you do actually want to
		// use MAT, then add @disabletmloadoptionscheck to the commandline.
		sys_addr_t matTestTmpMem;
		if (sys_memory_allocate(0x100000, SYS_MEMORY_PAGE_SIZE_1M, &matTestTmpMem) != CELL_OK)
		{
			Quitf("Failed to allocate temporary memory for MAT test");
		}
		if (sys_dbg_mat_set_condition(matTestTmpMem, SYS_DBG_MAT_WRITE) == CELL_OK)
		{
			Memoryf("################################\n");
			Memoryf("MAT enabled.  Must be disabled in the Target Manager, under\n");
			Memoryf("Properties > Target > Load Options > Enable MAT\n");
			Memoryf("See https://devstar.rockstargames.com/wiki/index.php/PS3_core_dump for more information.\n");
			Memoryf("################################\n");
			failBecauseOfBadTmOptions = true;
		}
		if (sys_memory_free(matTestTmpMem) != CELL_OK)
		{
			Quitf("Failed to free temporary memory for MAT test");
		}
	}

	if (failBecauseOfBadTmOptions)
	{
		for (;;) sys_timer_sleep(1);
	}

}
#endif // EXCEPTION_HANDLING

	const size_t exceptionMemorySize = userMemorySize - GetAvailableUserMemory();
	userMemorySize -= exceptionMemorySize;
	Memoryf("[STARTUP] PS3 Exception Memory      = %d KB\n", (exceptionMemorySize >> 10));
	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [THREAD STACK]
	//
	FINAL_ONLY(const) size_t threadMemorySize = (THREAD_STACK_SIZE << 10);

#if !__FINAL
	// EJ: Debug station needs 192kb extra
	if (IsDevkit())
		threadMemorySize += (192 << 10);
#endif // !__FINAL
	
	userMemorySize -= threadMemorySize;
	Memoryf("[STARTUP] PS3 Thread Memory         = %d KB\n", (threadMemorySize >> 10));
	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [EXTERNAL HEAP]
	//
	const size_t externalMemorySize = (EXTERNAL_HEAP_SIZE << 10);
	userMemorySize -= externalMemorySize;
	Memoryf("[STARTUP] PS3 External Memory       = %d KB\n", (externalMemorySize >> 10));
	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [RESIDUAL MEMORY]
	//
	const size_t residualMemorySize = userMemorySize & 0xfffff;
	userMemorySize -= residualMemorySize;
	Memoryf("[STARTUP] PS3 Residual Memory       = %d KB\n", (residualMemorySize >> 10));	
	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [WASTED MEMORY]
	//	
	const size_t wastedMemorySize = (residualMemorySize & 0xffff);
	Memoryf("[STARTUP] PS3 Wasted Memory         = %d / %d KB\n", (wastedMemorySize >> 10), (residualMemorySize >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [COMMERCE MEMORY]
	//	
	const size_t commerceMemoryTotal = COMMERCE_HEAP_SIZE + (residualMemorySize - wastedMemorySize);
	userMemorySize -= COMMERCE_HEAP_SIZE;
	Memoryf("[STARTUP] PS3 Commerce Memory       = %d / %d KB\n", (commerceMemoryTotal >> 10), (COMMERCE_HEAP_SIZE >> 10));
	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

	// EJ: Map the container memory to virtual hard drive space
	static VirtualMemHeap virtualMemHeap;	
	if (!virtualMemHeap.AllocateMemory(COMMERCE_IO_SIZE, commerceMemoryTotal))
		Quitf("[STARTUP] ERROR: Unable to create commerce memory map!");

	sysMemManager::GetInstance().SetVirtualMemHeap(virtualMemHeap);
	char* commerceMemory = static_cast<char*>(virtualMemHeap.GetHeapPtr());
	size_t commerceMemorySize = commerceMemoryTotal;

	// Network
	sysMemManager::GetInstance().SetNetworkMemory(commerceMemory);
	commerceMemory += NETWORK_HEAP_SIZE;
	commerceMemorySize -= NETWORK_HEAP_SIZE;

	// Frag
	sysMemManager::GetInstance().SetFragMemory(commerceMemory);
	commerceMemory += FRAG_HEAP_SIZE;
	commerceMemorySize -= FRAG_HEAP_SIZE;

	// MovePed
	sysMemManager::GetInstance().SetMovePedMemory(commerceMemory);
	commerceMemory += MOVEPED_HEAP_SIZE;
	commerceMemorySize -= MOVEPED_HEAP_SIZE;

	// Flex
	static sysMemSimpleAllocator flexAllocator(commerceMemory, commerceMemorySize, sysMemSimpleAllocator::HEAP_FLEX, false);
	flexAllocator.SetQuitOnFail(false);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [FINAL MEMORY]
	//
	size_t finalMemorySize = userMemorySize & ~0xfffff;

#if ENABLE_DEBUG_HEAP
	// EJ: There is only extra memory on a devkit!
	if (IsDevkit())
	{		
		finalMemorySize -= debugMemorySize;
		RAGE_MEMORY_DEBUG_ONLY(finalMemorySize -= toolMemorySize;)
	}	
#endif // ENABLE_DEBUG_HEAP

	// Restrict to some sane maximum
	const size_t desiredMemoryMax = (208 << 20);
	if (finalMemorySize > desiredMemoryMax)
	{
		Memoryf("[STARTUP] PS3 Desired Max Memory    = %d / %d KB\n", (desiredMemoryMax >> 10), (finalMemorySize >> 10));
		finalMemorySize = desiredMemoryMax;
	}	
	
	FINAL_ONLY(const) size_t desiredMemorySize = (((SIMPLE_HEAP_SIZE) + (VIRTUAL_HEAP_SIZE) + (HEADER_HEAP_SIZE)) << 10) & ~0xfffff;

#if ENABLE_DEBUG_HEAP
	g_sysExtraStreamingMemory = (GetExtraStreamingMemory() << 10);
	Memoryf("[STARTUP] PS3 Extra Streaming       = %d KB\n", (g_sysExtraStreamingMemory >> 10));
	desiredMemorySize += g_sysExtraStreamingMemory;
#endif

	Memoryf("[STARTUP] PS3 Desired Memory        = %d / %d KB\n", (desiredMemorySize >> 10), (finalMemorySize >> 10));		

	if (finalMemorySize > desiredMemorySize)
	{
#if RAGE_MEMORY_DEBUG
		const size_t overageMemorySize = (finalMemorySize - desiredMemorySize);
		Memoryf("[STARTUP] PS3 Overage Memory        = %d KB\n", (overageMemorySize >> 10));

		if (IsActualMem() && !IsDevkit())
			Memoryf("[STARTUP] PS3 TestKit has extra memory! Safe to increase SIMPLE_HEAP_SIZE or VIRTUAL_HEAP_SIZE by %d Mb\n", (overageMemorySize >> 20));
#endif
#if !__FINAL
		finalMemorySize = desiredMemorySize;
#endif
	}
	else if (finalMemorySize < desiredMemorySize)
	{
		Memoryf("[STARTUP] Requested heap was %dM+%dM (%dM) but only %dM is available.\n", (SIMPLE_HEAP_SIZE >> 10), (VIRTUAL_HEAP_SIZE >> 10), (desiredMemorySize >> 20), (finalMemorySize >> 20));
		Memoryf("[STARTUP] The game will not likely run!\n");
	}

	Memoryf("[STARTUP] PS3 Final Memory          = %d / %d KB\n", (finalMemorySize >> 10), (userMemorySize >> 10));	
	userMemorySize -= finalMemorySize;

	Memoryf("[STARTUP] PS3 Available Memory      = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));

	sys_addr_t addr = 0;
	if ((result = sys_memory_allocate(finalMemorySize, SYS_MEMORY_PAGE_SIZE_1M, &addr)) != CELL_OK)
		Quitf("[STARTUP] ERROR: Unable to allocate main memory! Code %x\n", result);

	// Quitf and Memoryf do not reference the var args in final no output
	// builds, so need to disable the warning about the result variable being
	// set but not used.
	(void) result;

#if RAGE_MEMORY_DEBUG
	size_t remainingMemorySize = GetAvailableUserMemory() - threadMemorySize - wastedMemorySize;

#if ENABLE_DEBUG_HEAP
	// EJ: There is only extra memory on a devkit!
	if (IsDevkit())
	{
		remainingMemorySize -= (debugMemorySize + toolMemorySize);
		Memoryf("[STARTUP] PS3 Remaining user memory: %ukb; USED=%uM DEBUG=%ukb TOOL=%ukb TSS=%ukb EHS=%ukb, EXTRA=%ukb (WASTED)\n", (remainingMemorySize >> 10), (finalMemorySize >> 20), (debugMemorySize >> 10), (toolMemorySize >> 10), (threadMemorySize >> 10), (externalMemorySize >> 10), (wastedMemorySize >> 10));
	}
	else
#endif // ENABLE_DEBUG_HEAP
	{
		Memoryf("[STARTUP] PS3 Remaining user memory: %ukb; USED=%uM TSS=%ukb EHS=%ukb, EXTRA=%ukb (WASTED)\n", (remainingMemorySize >> 10), (finalMemorySize >> 20), (threadMemorySize >> 10), (externalMemorySize >> 10), (wastedMemorySize >> 10));
	}	
#endif // RAGE_MEMORY_DEBUG

	char* gcmHeap = (char*) (size_t) addr;
	const size_t gcmSize = ((GCM_BUFFER_SIZE) << 10);
	char* gameHeap = gcmHeap + gcmSize;
	
	const size_t resVirtHeapSize = (VIRTUAL_HEAP_SIZE << 10) - gcmSize ENABLE_DEBUG_HEAP_ONLY(+ g_sysExtraStreamingMemory);
	FastAssert(resVirtHeapSize > 0);
	FastAssert(resVirtHeapSize <= (256 << 20));

	// Since we require 1M granularity, give anything extra left over after the GCM buffer back to the game heap	
	const size_t headerHeapSize = (HEADER_HEAP_SIZE << 10);
	const size_t gameVirtHeapSize = finalMemorySize - resVirtHeapSize - headerHeapSize - gcmSize;
	char* hdrHeapMemory = gameHeap + gameVirtHeapSize;
	char* resVirtHeap = hdrHeapMemory + headerHeapSize;

	g_GcmInitBufferSize = (GCM_BUFFER_SIZE) << 10;
	g_GcmMappingSize = finalMemorySize;
	g_GcmHeap = (GCM_BUFFER_SIZE)? gcmHeap : NULL;
	g_MainHeap = gameHeap;

	CHECKPOINT_SYSTEM_MEMORY;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [GAME VIRTUAL]
	//
	static sysMemSimpleAllocator gameVirtualAllocator(gameHeap, gameVirtHeapSize, sysMemSimpleAllocator::HEAP_MAIN);
	Memoryf("[STARTUP] PS3 Game Virt Heap        = %d / %d / %d KB\n", (gameVirtHeapSize >> 10), ((gameVirtHeapSize + (residualMemorySize - wastedMemorySize)) >> 10), SIMPLE_HEAP_SIZE);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [FLEX ALLOCATOR]
	//
	static sysMemSplitAllocator splitAllocator(flexAllocator, gameVirtualAllocator);
	sysMemManager::GetInstance().SetFlexAllocator(splitAllocator);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [HEADER VIRTUAL]
	//	
	static sysMemSimpleAllocator headerVirtualAllocator(hdrHeapMemory, headerHeapSize, sysMemSimpleAllocator::HEAP_HEADER, false);
	headerVirtualAllocator.SetQuitOnFail(false);
	Memoryf("[STARTUP] PS3 Header Virt Heap      = %d KB\n", (headerHeapSize >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [RESOURCE VIRTUAL]
	//
	size_t resVirtWorkSize = COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(resVirtHeapSize / g_rscVirtualLeafSize);

	Memoryf("[STARTUP] PS3 Res Virt Work         = %d KB\n", (resVirtWorkSize >> 10));
	void* const resVirtWork = gameVirtualAllocator.Allocate(resVirtWorkSize, 16);

	static sysMemBuddyAllocator resourceVirtualAllocator(resVirtHeap, g_rscVirtualLeafSize, (resVirtHeapSize / g_rscVirtualLeafSize), resVirtWork);	
	Memoryf("[STARTUP] PS3 Res Virt Heap         = %d KB\n", (resVirtHeapSize >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [GAME PHYSICAL]
	//
	const size_t gamePhysWorkSize = COMPUTE_WORKSPACE_SIZE((SIMPLE_PHYSICAL_NODES)/2);
	static u8 gamePhysWork[gamePhysWorkSize];
	Memoryf("[STARTUP] PS3 Game Phys Work        = %d KB\n", (gamePhysWorkSize >> 10));

	// Physical size is in kilobytes; page size is 4k.
	CompileTimeAssert(SIMPLE_PHYSICAL_SIZE <= (248*1024)); // revise

	const size_t gamePhysHeapSize = (VRAM_SIZE - (SIMPLE_PHYSICAL_SIZE) - (VRAM_GAME_RESERVED)) << 10;
	static sysMemExternalAllocator gamePhysicalAllocator(gamePhysHeapSize,(void*)-1 /*config.localAddress*/,((SIMPLE_PHYSICAL_NODES) / 2), gamePhysWork);	
	Memoryf("[STARTUP] PS3 Game Phys Heap        = %d KB\n", (gamePhysHeapSize >> 10));
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [RESOURCE PHYSICAL]
	//	
	const size_t resPhysWorkSize = COMPUTE_BUDDYHEAP_WORKSPACE_SIZE((SIMPLE_PHYSICAL_SIZE<<10) / g_rscPhysicalLeafSize);
	static u8 resourcePhysicalWorkspace[resPhysWorkSize];
	Memoryf("[STARTUP] PS3 Res Phys Work         = %d KB\n", (resPhysWorkSize >> 10));

	const size_t resPhysHeapSize = (SIMPLE_PHYSICAL_SIZE << 10);
	static sysMemBuddyAllocator resourcePhysicalAllocator((void*)-1, g_rscPhysicalLeafSize, (resPhysHeapSize / g_rscPhysicalLeafSize), resourcePhysicalWorkspace);	
	Memoryf("[STARTUP] PS3 Res Phys Heap         = %d KB\n", (resPhysHeapSize >> 10));

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [MULTI ALLOCATOR]
	//
	static sysMemMultiAllocator theAllocator;
	theAllocator.AddAllocator(gameVirtualAllocator);
	theAllocator.AddAllocator(resourceVirtualAllocator);
	theAllocator.AddAllocator(gamePhysicalAllocator);
	theAllocator.AddAllocator(resourcePhysicalAllocator);
	theAllocator.AddAllocator(headerVirtualAllocator);

	CHECKPOINT_SYSTEM_MEMORY;

#elif RSG_ORBIS
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [PS4 - MEMORY INFO]
	//	
#if RAGE_MEMORY_DEBUG
	Memoryf("[STARTUP] PS4 - %-24s = %s\n", "Mode", IsDevkit() ? "DEVELOPMENT" : "RETAIL");	

	const size_t userMemoryTotal = GetTotalUserMemory();
	Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Total Memory", userMemoryTotal >> 20);	

	const size_t exeMemoryTotal = GetUsedUserMemory();
	Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Executable Memory", exeMemoryTotal >> 20);
	NOTFINAL_ONLY(MEMMANAGER.SetExeSize(exeMemoryTotal);)
	
	const size_t extraMemoryTotal = GetTotalExtraMemory();
	Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Extra Memory", extraMemoryTotal >> 20);
#endif

	size_t userMemorySize = GetAvailableUserMemory();
	NOTFINAL_ONLY(Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);)

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBONE - DEBUG]
	//
#if ENABLE_DEBUG_HEAP
	Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Debug Memory", debugMemorySize >> 20);
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [PS4 - HEMLIS]
	//
#if !__FINAL
	const size_t hemlisHeapSize = (HEMLIS_HEAP_SIZE << 10);
	static sysMemSimpleAllocator hemlisAllocator(sysMemVirtualAllocate(hemlisHeapSize), hemlisHeapSize, sysMemSimpleAllocator::HEAP_MAIN);
	Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Hemlis Memory", hemlisHeapSize >> 20);	
	MEMMANAGER.SetHemlisAllocator(hemlisAllocator);	
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [PS4 - FLEX]
	//
	const size_t flexHeapSize = (static_cast<size_t>(FLEX_HEAP_SIZE) << static_cast<size_t>(10));		
	static sysMemSimpleAllocator flexAllocator(sysMemFlexAllocate(flexHeapSize), flexHeapSize, sysMemSimpleAllocator::HEAP_FLEX);
	NOTFINAL_ONLY(Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Flex Heap", (size_t) flexHeapSize >> 20);)
	sysMemManager::GetInstance().SetFlexAllocator(flexAllocator);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [PS4 - GAME]
	//
	const size_t gameVirtHeapSize = (static_cast<size_t>(SIMPLE_HEAP_SIZE) << static_cast<size_t>(10));
	static sysMemSimpleAllocator gameVirtualAllocator(sysMemVirtualAllocate(gameVirtHeapSize), gameVirtHeapSize, sysMemSimpleAllocator::HEAP_MAIN);	
	NOTFINAL_ONLY(Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Game Heap", gameVirtHeapSize >> 20);)

	userMemorySize = GetAvailableUserMemory();
	NOTFINAL_ONLY(Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);)

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [PS4 - CHUNKY]
	//
	size_t chunkyHeapSize = 13 << 20; // Decrease hemlis by 13mb
	Memoryf("[CHUNKY] PS4 Chunky Allocator taking %d from Hemlis\n", (int)chunkyHeapSize);
	static sysMemSimpleAllocator chunkyAllocator(sysMemVirtualAllocate(chunkyHeapSize), chunkyHeapSize, sysMemSimpleAllocator::HEAP_CHUNKY, false);
	sysMemManager::GetInstance().SetChunkyAllocator(&chunkyAllocator);

	//////////////////////////////////////////////////////////////////////////
	// [ PS4 - POOL]
	const size_t poolHeapSize = (static_cast<size_t>(POOL_HEAP_SIZE) << static_cast<size_t>(10));

	static sysMemSimpleAllocator poolAllocator(sysMemVirtualAllocate(poolHeapSize), poolHeapSize, sysMemSimpleAllocator::HEAP_MAIN, false);
	sysMemManager::GetInstance().SetPoolAllocator(&poolAllocator);
	NOTFINAL_ONLY(Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Pool Heap", poolHeapSize >> 20);)

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [PS4 - RESOURCE]
	//
	size_t resVirtHeapSize = ((size_t)SIMPLE_PHYSICAL_SIZE) << (size_t)10;
#if ENABLE_BUDDY_ALLOCATOR
	size_t resVirtWorkSize = COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(resVirtHeapSize / g_rscVirtualLeafSize);	
	resVirtWorkSize = (resVirtWorkSize + 32767) & ~32767;

	char *resVirtHeap = (char*) sysMemVirtualAllocate(resVirtHeapSize + resVirtWorkSize);
	Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Resource Heap", resVirtHeapSize >> 20);
	Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u KB\n", "Resource Work", resVirtWorkSize >> 10);

	char* resVirtWork = resVirtHeap;
	resVirtHeap += resVirtWorkSize;

	static sysMemBuddyAllocator resourceVirtualAllocator(resVirtHeap, g_rscVirtualLeafSize, (resVirtHeapSize / g_rscVirtualLeafSize), resVirtWork);
#else
	static sysMemVirtualAllocator resourceVirtualAllocator(resVirtHeapSize);
#endif

	userMemorySize = GetAvailableUserMemory();
	NOTFINAL_ONLY(Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);)

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [PS4 - FRAGMENT]
	//
	void* pFragMemory = sysMemVirtualAllocate(FRAG_HEAP_SIZE);
	Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u MB\n", "Fragment Heap", static_cast<size_t>(FRAG_HEAP_SIZE >> 20));
	sysMemManager::GetInstance().SetFragMemory(pFragMemory);

	userMemorySize = GetAvailableUserMemory();
	NOTFINAL_ONLY(Memoryf("[STARTUP] PS4 - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);)

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [PS4 - MULTI ALLOCATOR]
	//
	static sysMemMultiAllocator theAllocator;
	theAllocator.AddAllocator(gameVirtualAllocator);
	theAllocator.AddAllocator(resourceVirtualAllocator);
	theAllocator.AddAllocator(resourceVirtualAllocator);
	theAllocator.AddAllocator(resourceVirtualAllocator);

#elif RSG_DURANGO
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBone - MEMORY INFO]
	//

	XMemSetAllocationHooks(sysMemXMemAlloc, sysMemXMemFree);

#if RAGE_MEMORY_DEBUG
	Memoryf("[STARTUP] XBONE - %-24s = %s\n", "Mode", IsDevkit() ? "DEVELOPMENT" : "RETAIL");	

	const size_t userMemoryTotal = GetTotalUserMemory();
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Total Memory", userMemoryTotal >> 20);	

	const size_t exeMemoryTotal = GetUsedUserMemory();
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Executable Memory", exeMemoryTotal >> 20);
	NOTFINAL_ONLY(MEMMANAGER.SetExeSize(exeMemoryTotal);)

	const size_t extraMemoryTotal = GetTotalExtraMemory();
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Extra Memory", extraMemoryTotal >> 20);
#endif

	size_t userMemorySize = GetAvailableUserMemory();
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBONE - DEBUG]
	//
#if ENABLE_DEBUG_HEAP
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Debug Memory", debugMemorySize >> 20);
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBONE - HEMLIS]
	//
#if !__FINAL
	const size_t hemlisHeapSize = (HEMLIS_HEAP_SIZE << 10);
	static sysMemSimpleAllocator hemlisAllocator(sysMemVirtualAllocate(hemlisHeapSize), hemlisHeapSize, sysMemSimpleAllocator::HEAP_MAIN);		
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Hemlis Memory", hemlisHeapSize >> 20);	
	MEMMANAGER.SetHemlisAllocator(hemlisAllocator);
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBONE - GAME]
	//
	// Normal game heap memory.  (Make it GPU-visible, ONION)
	const size_t gameVirtHeapSize = (((size_t)SIMPLE_HEAP_SIZE) << (size_t)10);
	void* gameVirtHeap = VirtualAllocTracked(NULL, gameVirtHeapSize, MEM_COMMIT | MEM_RESERVE | MEM_GRAPHICS | MEM_4MB_PAGES, PAGE_READWRITE | PAGE_GPU_COHERENT);

	static sysMemSimpleAllocator gameVirtualAllocator(gameVirtHeap, gameVirtHeapSize, sysMemSimpleAllocator::HEAP_MAIN);
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Game Heap", gameVirtHeapSize >> 20);
	Memoryf("[STARTUP]         (%p to %p)\n", gameVirtHeap, (char*)gameVirtHeap + gameVirtHeapSize);
	
	userMemorySize = GetAvailableUserMemory();
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBONE - CHUNKY]
	size_t chunkyHeapSize = 13 << 20; // Decrease hemlis by 13mb
	Memoryf("[CHUNKY] XBONE Chunky Allocator taking %d from Hemlis\n", (int)chunkyHeapSize);
	static sysMemSimpleAllocator chunkyAllocator(sysMemVirtualAllocate(chunkyHeapSize), chunkyHeapSize, sysMemSimpleAllocator::HEAP_CHUNKY, false);
	sysMemManager::GetInstance().SetChunkyAllocator(&chunkyAllocator);

	//////////////////////////////////////////////////////////////////////////
	// [ XBONE - POOL]
	const size_t poolHeapSize = (static_cast<size_t>(POOL_HEAP_SIZE) << static_cast<size_t>(10));

	static sysMemSimpleAllocator poolAllocator(sysMemVirtualAllocate(poolHeapSize), poolHeapSize, sysMemSimpleAllocator::HEAP_MAIN, false);
	sysMemManager::GetInstance().SetPoolAllocator(&poolAllocator);
	NOTFINAL_ONLY(Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Pool Heap", poolHeapSize >> 20);)

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBONE - RESOURCE]
	//
	// Heap
	const size_t resPhysHeapSize = ((size_t)SIMPLE_PHYSICAL_SIZE) << (size_t)10;
#if ENABLE_BUDDY_ALLOCATOR
	char* resPhysHeap = (char*) sysMemPhysicalAllocate(resPhysHeapSize, CalculateGraphicsMemBufferAlignment(resPhysHeapSize), PhysicalMemoryType::PHYS_MEM_GARLIC_WRITEBACK);
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Resource Heap", resPhysHeapSize >> 20);
	Memoryf("[STARTUP]         (%p to %p)\n", resPhysHeap, resPhysHeap + resPhysHeapSize);

	userMemorySize = GetAvailableUserMemory();
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);

	// Workspace
	size_t resPhysWorkSize = COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(resPhysHeapSize / g_rscPhysicalLeafSize);	
	resPhysWorkSize = (resPhysWorkSize + 32767) & ~32767;

	char* resPhysWork = (char*) sysMemVirtualAllocate(resPhysWorkSize);
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u KB\n", "Resource Work", resPhysWorkSize >> 10);
	Memoryf("[STARTUP]         (%p to %p)\n", resPhysWork, resPhysWork + resPhysWorkSize);

	userMemorySize = GetAvailableUserMemory();
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);

	// Buddy Allocator
	static sysMemBuddyAllocator resPhysAllocator(resPhysHeap, g_rscPhysicalLeafSize, (resPhysHeapSize / g_rscPhysicalLeafSize), resPhysWork);

	// Register it as a graphics memory area.
	rage::SetPhysicalMemoryArea(resPhysHeap, resPhysHeapSize);
#else
	static sysMemVirtualAllocator resPhysAllocator(resPhysHeapSize);
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBONE - FRAGMENT]
	//
	void* pFragMemory = sysMemVirtualAllocate(FRAG_HEAP_SIZE);
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u MB\n", "Fragment Heap", static_cast<size_t>(FRAG_HEAP_SIZE >> 20));
	Memoryf("[STARTUP]         (%p to %p)\n", pFragMemory, (char*)pFragMemory + FRAG_HEAP_SIZE);
	sysMemManager::GetInstance().SetFragMemory(pFragMemory);

	userMemorySize = GetAvailableUserMemory();
	Memoryf("[STARTUP] XBONE - %-24s = %" SIZETFMT "u / %" SIZETFMT "u MB\n", "Available Memory", userMemorySize >> 20, userMemoryTotal >> 20);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [XBONE - MULTI ALLOCATOR]
	//
	static sysMemMultiAllocator theAllocator;
	theAllocator.AddAllocator(gameVirtualAllocator); // MEMTYPE_GAME_VIRTUAL
	theAllocator.AddAllocator(resPhysAllocator); // MEMTYPE_RESOURCE_VIRTUAL
	theAllocator.AddAllocator(resPhysAllocator); // MEMTYPE_GAME_PHYSICAL
	theAllocator.AddAllocator(resPhysAllocator); // MEMTYPE_RESOURCE_PHYSICAL

#else	// !__PPU && !RSG_ORBIS && !RSG_DURANGO

#if __XENON
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [INIT]
	// 
#if RAGE_MEMORY_DEBUG
	const size_t userMemoryTotal = GetTotalUserMemory();
	const size_t userMemorySize = GetAvailableUserMemory();
	const size_t exeMemoryTotal = userMemoryTotal - userMemorySize - GetTotalOSMemory();
#endif

	Memoryf("[STARTUP] XBOX Actual Memory    = %s\n", IsActualMem() ? "YES" : "NO");
	Memoryf("[STARTUP] XBOX Kit Type         = %s\n", IsDevkit() ? "DEVKIT" : "TESTKIT");
	Memoryf("[STARTUP] XBOX Memory Total     = %d KB\n", (userMemoryTotal >> 10));
	Memoryf("[STARTUP] XBOX OS Memory        = %d KB\n", (GetTotalOSMemory() >> 10));
	Memoryf("[STARTUP] XBOX XEX Memory       = %d KB\n", (exeMemoryTotal >> 10));	

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [DEBUG]
	//
	g_pXenonPoolAllocator = NULL;
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [POOL]
	//
	// To account for the difference between debug and final executables on 360, and to
	// have a more representative size streaming buffer during development, we've chosen
	// to use a certain amount of debug memory on the 1GB kits to shift off something easily
	// moved (in this case, pools storage).
	const size_t poolMemorySize = (SIMPLE_POOL_SIZE) << 10;

	if (poolMemorySize)
	{
#if ENABLE_DEBUG_POOL
		void* const poolMemory = sysMemDebugAllocate(poolMemorySize);

		// Debug
		const size_t extraMemoryTotal = GetTotalExtraMemory();
		const size_t extraMemorySize = extraMemoryTotal - debugMemorySize - poolMemorySize;
		Memoryf("[STARTUP] XBOX Extra Memory     = %d / %d KB\n", (extraMemorySize  >> 10), (extraMemoryTotal >> 10));		
		Memoryf("[STARTUP] XBOX Debug Memory     = %d KB\n", (debugMemorySize >> 10));
#else
		void* const poolMemory = sysMemPhysicalAllocate(poolMemorySize);
#endif
		if (poolMemory)
		{
			static sysMemSimpleAllocator xenonPoolAllocator(poolMemory, poolMemorySize, sysMemSimpleAllocator::HEAP_FLEX, false);
			xenonPoolAllocator.SetQuitOnFail(false);
			g_pXenonPoolAllocator = &xenonPoolAllocator;
		}
	}

	Memoryf("[STARTUP] XBOX Pool Memory      = %d KB\n", (poolMemorySize >> 10));
	Memoryf("[STARTUP] XBOX Available Memory = %d / %d KB\n", (userMemorySize >> 10), (userMemoryTotal >> 10));
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [IGNORED]
	//
#if !__FINAL
	g_sysExtraMemory = 0;

	if (sysBootManager::IsDebuggerPresent())
	{
		// Take off extra development memory
		LPSTR cmd = GetCommandLine();
		char* extraCmd = strstr(cmd, "@extramemory=");
		if (extraCmd)
		{
			g_sysExtraMemory = (atoi(extraCmd + 13) << 10);
			Memoryf("[STARTUP] XBOX @extramemory     = %d KB\n", (g_sysExtraMemory >> 10));
		}

#if SUPPORT_DEBUG_MEMORY_FILL
		if (strstr(cmd,"@nodebugmemoryfill")) 
		{
			if (g_EnableDebugMemoryFill)
				Memoryf("[STARTUP] Disabling debug memory fill\n");
			g_EnableDebugMemoryFill = false;
		}
		if (const char *dm = strstr(cmd,"@debugmemoryfill")) 
		{
			if (dm[16]=='=')
				g_EnableDebugMemoryFill = (u8)atoi(dm+17);
			else
				g_EnableDebugMemoryFill = 0xFF;
			
			Memoryf("[STARTUP] Enabling debug memory fill mask %d!\n",g_EnableDebugMemoryFill);
			g_EnableDebugMemoryFill = true;
		}
#endif
	}
#endif // !__FINAL

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [MERGED]
	//
	size_t combinedSize = static_cast<size_t>(SIMPLE_COMBINED_SIZE) << 10;
	NOTFINAL_ONLY(combinedSize -= g_sysExtraMemory);

#if ENABLE_DEBUG_HEAP
	g_sysExtraStreamingMemory = (GetExtraStreamingMemory() << 10);
	Memoryf("[STARTUP] XBOX Extra Streaming  = %d KB\n", (g_sysExtraStreamingMemory >> 10));
#endif

#if ENABLE_DEBUG_POOL
	combinedSize -= FRAG_HEAP_SIZE;
#endif

	char* combinedMemory = (char*) sysMemPhysicalAllocate(combinedSize);
#endif // __XENON

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [GAME VIRTUAL]
#if RSG_XENON
	FINAL_ONLY(const) size_t headerHeapSize = (HEADER_HEAP_SIZE << 10);
	const size_t gameVirtHeapSize = (static_cast<size_t>(SIMPLE_HEAP_SIZE) << 10) - FRAG_HEAP_SIZE - headerHeapSize;
#else
	const size_t gameVirtHeapSize = (static_cast<size_t>(SIMPLE_HEAP_SIZE) << 10) - FRAG_HEAP_SIZE;
#endif

#if API_HOOKER
	InitHooker();
#endif

	sysMemAllocator *theGameAllocator;

#if (!RSG_FINAL && !__FINAL) && RSG_PC
	bool sparseGameHeap = sysParam::FindCommandLineArg("@sparsegameheap");
	if (sparseGameHeap) 
	{
		static sysMemSparseAllocator gameVirtualAllocatorSparse(gameVirtHeapSize, sysMemSimpleAllocator::HEAP_MAIN);
		theGameAllocator = &gameVirtualAllocatorSparse;
	}
	else 
#endif
	{
		static sysMemSimpleAllocator gameVirtualAllocatorSimple(gameVirtHeapSize, sysMemSimpleAllocator::HEAP_MAIN);
		theGameAllocator = &gameVirtualAllocatorSimple;
	}
	sysMemAllocator &gameVirtualAllocator = *theGameAllocator;
#if RSG_XENON
	Memoryf("[STARTUP] XBOX Game Virt Heap   = %d KB\n", (gameVirtHeapSize >> 10));	
	static sysMemSimpleAllocator gameVirtualAllocator(combinedMemory, gameVirtHeapSize, sysMemSimpleAllocator::HEAP_MAIN);
	combinedMemory += gameVirtHeapSize;
	combinedSize -= gameVirtHeapSize;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [FRAG CACHE]
	//
	Memoryf("[STARTUP] XBOX Frag Cache Heap  = %d KB\n", (FRAG_HEAP_SIZE >> 10));

#if ENABLE_DEBUG_POOL
	void* pFragMemory = sysMemDebugAllocate(FRAG_HEAP_SIZE);
	sysMemManager::GetInstance().SetFragMemory(pFragMemory);
#else
	sysMemManager::GetInstance().SetFragMemory(combinedMemory);
	combinedMemory += FRAG_HEAP_SIZE;
	combinedSize -= FRAG_HEAP_SIZE;
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [HEADER VIRTUAL]
	//
#if ENABLE_DEBUG_HEAP
	char* prevCombinedMemory = NULL;
	size_t prevHeaderHeapSize = 0;

	if (g_sysExtraStreamingMemory > 0)
	{
		prevHeaderHeapSize = headerHeapSize;
		headerHeapSize = g_sysExtraStreamingMemory;

		prevCombinedMemory = combinedMemory;		
		combinedMemory = (char*) sysMemDebugAllocate(headerHeapSize);
	}
#endif

	Memoryf("[STARTUP] XBOX Header Virt Heap = %d KB\n", (headerHeapSize >> 10));
	static sysMemSimpleAllocator headerVirtualAllocator(combinedMemory, headerHeapSize, sysMemSimpleAllocator::HEAP_HEADER, false);		
	headerVirtualAllocator.SetQuitOnFail(false);

#if ENABLE_DEBUG_HEAP
	if (g_sysExtraStreamingMemory > 0)
	{
		combinedMemory = prevCombinedMemory;
		headerHeapSize = prevHeaderHeapSize;
	}
#endif
	combinedMemory += headerHeapSize;
	combinedSize -= headerHeapSize;

#else

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [FRAG CACHE]
	//
	void* pFragMemory = sysMemPhysicalAllocate(FRAG_HEAP_SIZE);
    //@@: location MAIN_INITGAMEHEAP_SETFRAGMEMORY
	sysMemManager::GetInstance().SetFragMemory(pFragMemory);
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [RESOURCE VIRTUAL]
	//
	// Physical size is in kilobytes; page size is 8k.
	size_t resVirtHeapSize = static_cast<size_t>(SIMPLE_PHYSICAL_SIZE) << 10;

#if RSG_PC
	sysMemAllocator *thePhysicalAllocator;
	if (ENABLE_BUDDY_ALLOCATOR && USE_SPARSE_MEMORY NOTFINAL_ONLY( && sparseGameHeap))
	{
		size_t resVirtWorkSize = COMPUTE_SPARSEBUDDYHEAP_WORKSPACE_SIZE(resVirtHeapSize / g_rscPhysicalLeafSize);
		resVirtWorkSize = (resVirtWorkSize + 1023) & ~1023;
		void* const resVirtWork = sysMemVirtualAllocate(resVirtWorkSize);
		
		static sysMemSparseBuddyAllocator physicalAllocatorSparse(resVirtHeapSize, g_rscPhysicalLeafSize, (resVirtHeapSize / g_rscPhysicalLeafSize), resVirtWork);
		thePhysicalAllocator = &physicalAllocatorSparse;
	}
	else 
	{
		static sysMemGrowBuddyAllocator physicalAllocatorSimple(g_rscPhysicalLeafSize, resVirtHeapSize RAGE_TRACKING_ONLY(, "Resource Physical"));
		thePhysicalAllocator = &physicalAllocatorSimple;
	}
	sysMemAllocator &physicalAllocator = *thePhysicalAllocator;

#elif RSG_XENON
	NOTFINAL_ONLY(resVirtHeapSize -= g_sysExtraMemory;)
	size_t resVirtWorkSize = COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(resVirtHeapSize / g_rscPhysicalLeafSize);	
	void* const resVirtWork = gameVirtualAllocator.Allocate(resVirtWorkSize, 16);	
	static sysMemBuddyAllocator physicalAllocator(combinedMemory, g_rscPhysicalLeafSize, (resVirtHeapSize / g_rscPhysicalLeafSize), resVirtWork);
	Memoryf("[STARTUP] XBOX Res Virt Work    = %d KB\n", (resVirtWorkSize >> 10));
	Memoryf("[STARTUP] XBOX Res Virt Heap    = %d KB\n", (resVirtHeapSize >> 10));
	Memoryf("[STARTUP] XBOX System Heap      = %d KB\n", (GetAvailableUserMemory() >> 10));

#else
	size_t resVirtWorkSize = COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(resVirtHeapSize / g_rscPhysicalLeafSize);	
	void* const resVirtWork = gameVirtualAllocator.Allocate(resVirtWorkSize, 16);
	void* const resVirtHeap = sysMemPhysicalAllocate(resVirtHeapSize);
	// Register it as a graphics memory area.
	rage::SetPhysicalMemoryArea(resVirtHeap, resVirtHeapSize);

	static sysMemBuddyAllocator physicalAllocator(resVirtHeap, g_rscPhysicalLeafSize, (resVirtHeapSize / g_rscPhysicalLeafSize), resVirtWork);
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [MULTI ALLOCATOR]
	//	
	//@@: location MAIN_INITGAMEHEAP_ADDALLOCATOR
	static sysMemMultiAllocator theAllocator;
	theAllocator.AddAllocator(gameVirtualAllocator);

#if RSG_PC && defined(SIMPLE_VIRTUAL_SIZE) && (SIMPLE_VIRTUAL_SIZE > 0)
	if (ENABLE_BUDDY_ALLOCATOR && USE_SPARSE_MEMORY NOTFINAL_ONLY( && sparseGameHeap))
	{
		// Really virtual but physical names used virtual above - Lets add to the confusion
		size_t resPhysHeapSize = static_cast<size_t>(SIMPLE_VIRTUAL_SIZE) << 10;
		size_t resPhysWorkSize = COMPUTE_SPARSEBUDDYHEAP_WORKSPACE_SIZE( resPhysHeapSize / g_rscVirtualLeafSize);
		resPhysWorkSize = (resPhysWorkSize + 1023) & ~1023;
		void* const resPhysWork = sysMemVirtualAllocate(resPhysWorkSize);
		static sysMemSparseBuddyAllocator resourceVirtualAllocator(resPhysHeapSize, g_rscVirtualLeafSize, (resPhysHeapSize / g_rscVirtualLeafSize), resPhysWork);
		theAllocator.AddAllocator(resourceVirtualAllocator);
	}
	else 
	{
		static sysMemGrowBuddyAllocator resourceVirtualAllocator(g_rscVirtualLeafSize, (SIMPLE_VIRTUAL_SIZE) << 10 RAGE_TRACKING_ONLY(, "Resource Virtual"));
		theAllocator.AddAllocator(resourceVirtualAllocator);
	}
#else // !RSG_PC
	theAllocator.AddAllocator(physicalAllocator);
#endif // !RSG_PC
	
	theAllocator.AddAllocator(physicalAllocator);
	theAllocator.AddAllocator(physicalAllocator);
	XENON_ONLY(theAllocator.AddAllocator(headerVirtualAllocator);)
#endif // __PPU

#if ENABLE_DEFRAGMENTATION
	pgBase::SetTrackedHeap(*theAllocator.GetAllocator(MEMTYPE_RESOURCE_VIRTUAL));
#endif

#if RSG_DURANGO || RSG_ORBIS || RSG_PC
	//////////////////////////////////////////////////////////////////////////
	// [REPLAY ALLOCATOR]
	//	
#if REPLAY_HEAP_SIZE > 0
	u32 ReplayHeapSize = REPLAY_HEAP_SIZE;
#if !__FINAL
	if (rage::sysParam::FindCommandLineArg("@useDecreasedReplayMemory"))
	{
		ReplayHeapSize -= 24 << 20;
	}
#endif // !__FINAL
#if RSG_ORBIS
	// EJA: Moved this to flex memory because we were OOM on PS4
	void* replayHeap = sysMemFlexAllocate(ReplayHeapSize); //MEMMANAGER.GetFlexAllocator()->Allocate(REPLAY_HEAP_SIZE, 16);
#else
	void* replayHeap = sysMemVirtualAllocate(ReplayHeapSize);
#endif
	static sysMemSimpleAllocator replayAllocator(replayHeap, ReplayHeapSize, sysMemSimpleAllocator::HEAP_REPLAY, false);
	replayAllocator.SetQuitOnFail(false);
	sysMemManager::GetInstance().SetReplayAllocator(&replayAllocator);
#endif
#endif	//RSG_DURANGO || RSG_ORBIS || RSG_PC

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// [DEBUG]
	//
#if ENABLE_DEBUG_HEAP
#if RSG_ORBIS
	void* debugHeapMemory = sysMemVirtualAllocate(debugMemorySize);
#elif RSG_DURANGO

#if RSG_DEV || RSG_BANK
	size_t extraMemory = GetTotalExtraMemory();
	Memoryf("[STARTUP] XBOX - ExtraMemory     = %d KB\n", (extraMemory >> 10));
	Memoryf("[STARTUP] XBOX - DebugMemorySize = %d KB\n", (debugMemorySize >> 10));
	if (extraMemory<=debugMemorySize)
	{
		Quitf("\"Profiling mode\" needs to be enabled in the GDK Xbox Manager - Console Settings -> Debug or \"Debug Memory Mode\" Xbox One Manager - Settings > Other Settings - to \"Pix and Title\".");
	}
#endif

	void* debugHeapMemory = sysMemDebugAllocate(debugMemorySize);
#else
	void* debugHeapMemory = sysMemDebugAllocate(debugMemorySize);
#endif
	
#if RSG_ORBIS || RSG_DURANGO
	++g_sysDirectVirtualEnabled;
	++g_sysDirectDebugEnabled;
#endif

	if (debugHeapMemory)
	{
		CONSOLE_ONLY(Memoryf("+++ EXTRA MEMORY DETECTED!!!+++ - will be used for debug heap\n"));
		CONSOLE_ONLY(Memoryf("[STARTUP] Allocating %" SIZETFMT "uk debug heap...\n", (debugMemorySize >> 10)));
		Memoryf("[STARTUP]         (%p to %p)\n", debugHeapMemory, (char*)debugHeapMemory + debugMemorySize);

		static sysMemSimpleAllocator debugAllocator(debugHeapMemory, (int)debugMemorySize, sysMemSimpleAllocator::HEAP_DEBUG);
		theAllocator.AddAllocator(debugAllocator);
		g_sysHasDebugHeap = true;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// [RECORDER]
		//
#if (RSG_DURANGO || RSG_ORBIS || RSG_PC) && !__TOOL && !__RESOURCECOMPILER && (!defined(__RGSC_DLL) || !__RGSC_DLL)
		const size_t recorderHeapSize = (RECORDER_HEAP_SIZE << 10);
		void* recorderMemory = debugAllocator.Allocate(recorderHeapSize, 16);
		static sysMemSimpleAllocator recorderAllocator(recorderMemory, recorderHeapSize, sysMemSimpleAllocator::HEAP_MAIN);
		Memoryf("[STARTUP] %-24s = %" SIZETFMT "u MB\n", "Recorder Memory", recorderHeapSize >> 20);	
		MEMMANAGER.SetRecorderAllocator(recorderAllocator);	
#endif
	}
	else
	{
		CONSOLE_ONLY(Memoryf("+++ NO EXTRA MEMORY!!!!+++ - debug allocations will come from game memory!!!\n"));
		// If there's no debug heap, just point at the game allocator.
#if __BANK
#if __XENON
		Quitf("Xbox kits configured for 512Mb cannot run this configuration. Only RELEASE build can run on these machines");
#endif //__XENON
#endif //__BANK

		theAllocator.AddAllocator(gameVirtualAllocator);
	}
#else // ENABLE_DEBUG_HEAP
		// If there's no debug heap, just point at the game allocator.
		theAllocator.AddAllocator(gameVirtualAllocator);
#endif	// ENABLE_DEBUG_HEAP

#else	// SIMPLE_HEAP_SIZE
	// This bit of terror prevents the stock allocator from ever being destroyed
	// on shutdown, so we are immune to object cleanup order.
	static char theAllocatorBuffer[sizeof(stockAllocator)];
	sysMemAllocator& theAllocator = *(::new (theAllocatorBuffer) stockAllocator);
#endif	// SIMPLE_HEAP_SIZE

	//@@: location MAIN_INITGAMEHEAP_SET_CURRENT_ALLOCATOR
	sysMemAllocator::SetCurrent(theAllocator);
	sysMemAllocator::SetMaster(theAllocator);
	sysMemAllocator::SetContainer(theAllocator);

#if MEMORY_TRACKER
	sysMemStartDebug();
	static sysMemSystemTracker systemTracker;	
	sysMemManager::GetInstance().SetSystemTracker(systemTracker);
	sysMemEndDebug();
#endif

#if RESOURCE_HEADER
	sysMemManager::GetInstance().InitHeaders(8000);
#endif

#if defined(__MFC) || RSG_ORBIS || RSG_DURANGO || BACKTRACE_ENABLED
	s_pTheAllocator = &theAllocator;
#endif

#if __WIN32PC && !__TOOL
	
	//@@: location MAIN_INITGAMEHEAP_DEBUG_ALLOCATE
	static char stdout_buffer[512];
	setvbuf(stdout, stdout_buffer, _IOLBF, sizeof(stdout_buffer));
#endif

	CHECKPOINT_SYSTEM_MEMORY;

#if __PPU
	sysTaskManager__SysInitClass2();
#endif
}

#endif	// !defined(NO_INIT_GAME_HEAP)

#if !RSG_ORBIS	|| (SCE_ORBIS_SDK_VERSION >= 0x00920020u)
#if __WIN32
#pragma warning(disable: 4074) //  warning C4074: initializers put in compiler reserved initialization area
#pragma init_seg(compiler)
#endif
static struct InitGameHeap_t 
{
	InitGameHeap_t()
	{ 
		using namespace rage;
#if RAGE_USE_DEJA && __XENON
		DEJA_FLUSH(true);
#endif
		InitGameHeap();
#if RAGE_USE_DEJA
		DEJA_BOOKMARK("Main","InitGameHeap");
		DEJA_FLUSH(true);
#endif
		sysMemAllocator::GetCurrent().BeginLayer();
	}
	~InitGameHeap_t()
	{
#if RAGE_USE_DEJA
		DEJA_TERMINATE();
#endif
		// Don't do leak dumps if byte swapping, there are too many things we can't clean up properly.
		using namespace rage;
#if !__RESOURCECOMPILER
		sysMemAllocator::GetCurrent().EndLayer("global",NULL);
#endif
	}
} InitGameHeapInstance PPU_ONLY(__attribute__((init_priority(101)))) ORBIS_ONLY(__attribute__((init_priority(101))));
#endif	// !RSG_ORBIS

#if __PAGING
#include "paging/rscbuilder.h"
#endif

void *operator_new(size_t size,size_t align,const char *file,int line) {
	using namespace rage;
#if defined(__MFC) || RSG_ORBIS	|| RSG_DURANGO	// Razor starts its own threads that allocate memory.
	if (!sysMemAllocator_sm_Current) sysMemAllocator_sm_Current = s_pTheAllocator;
	if (!sysMemAllocator_sm_Master) sysMemAllocator_sm_Master = s_pTheAllocator;
	if (!sysMemAllocator_sm_Container) sysMemAllocator_sm_Container = s_pTheAllocator;
#endif
#if RSG_ORBIS && SCE_ORBIS_SDK_VERSION < 0x00920020u
	// temporary until init_priority works
	static bool init; if (!init) { init = true; InitGameHeap(); }
#endif

#if defined(__RGSC_DLL) && __RGSC_DLL
	// std allocates before our heap is allocated in release builds
	static bool init; if (!init) { init = true; InitGameHeap(); }
#endif

	void *result = sysMemAllocator::GetCurrent().LoggedAllocate(size,align,0,file,line);

#if !__FINAL
	if (result == sysMemAllocator::sm_BreakOnAddr)
		__debugbreak();
#endif

#if HACK_GTA4 && !__NO_OUTPUT
	if (!result && size > 0)
	{
		using namespace rage;
		sysMemAllocator *current = sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_GAME_VIRTUAL);
		Warningf("operator_new(%u,%d) failed.(%d/%d).",(rage::u32)size,(rage::s32)align,(rage::u32)current->GetLargestAvailableBlock(),(rage::u32)current->GetHeapSize());
	}
#endif // HACK_GTA4

	return result;
}

void *operator_new(size_t size,size_t align) {
	return operator_new(size, align, NULL, 0);
}

extern __THREAD int RAGE_LOG_DISABLE;

void* operator new(size_t size) {
#if RAGE_ENABLE_RAGE_NEW
	using namespace rage;
	if (!RAGE_LOG_DISABLE)
	{
		Assertf(false, "Call rage_new, not new, for better memory tracking.");
	}
#endif
	return operator_new(size, rage::sysMemAllocator::DEFAULT_ALIGNMENT);
}

void* operator new[](size_t size) {
#if RAGE_ENABLE_RAGE_NEW
	using namespace rage;
	if (!RAGE_LOG_DISABLE)
	{
		Assertf(false, "Call rage_new, not new, for better memory tracking.");
	}
#endif
	return operator_new(size, rage::sysMemAllocator::DEFAULT_ALIGNMENT);
}

void* operator new(size_t size,size_t align) {
#if RAGE_ENABLE_RAGE_NEW
	using namespace rage;
	if (!RAGE_LOG_DISABLE)
	{
		Assertf(false, "Call rage_aligned_new, not new, for better memory tracking.");
	}
#endif
	return operator_new((size > align) ? size : align, align);
}

void* operator new[](size_t size,size_t align) {
#if RAGE_ENABLE_RAGE_NEW
	using namespace rage;
	if (!RAGE_LOG_DISABLE)
	{
		Assertf(false, "Call rage_aligned_new, not new, for better memory tracking.");
	}
#endif
	return operator_new((size > align) ? size : align, align);
}

#if RAGE_ENABLE_RAGE_NEW

void* operator new(size_t size,const char* file,int line) {
	return operator_new(size, rage::sysMemAllocator::DEFAULT_ALIGNMENT, file, line);
}

void* operator new[](size_t size,const char* file,int line) {
	return operator_new(size, rage::sysMemAllocator::DEFAULT_ALIGNMENT, file, line);
}

void* operator new(size_t size,size_t align,const char* file,int line) {
	return operator_new(size > align ? size : align, align, file, line);
}

void* operator new[](size_t size,size_t align,const char* file,int line) {
	return operator_new((size > align) ? size : align, align, file, line);
}

#if __PS3 // special version for gcc when the rage_aligned_new is called on a class that is already ALIGNED
void* operator new(size_t size,size_t alignType,size_t alignParam,const char* file,int line) {
	size_t maxAlign = alignType > alignParam ? alignType : alignParam;
	return operator_new((size > maxAlign) ? size : maxAlign, maxAlign, file, line);
}

void* operator new[](size_t size,size_t alignType,size_t alignParam,const char* file,int line) {
	size_t maxAlign = alignType > alignParam ? alignType : alignParam;
	return operator_new((size > maxAlign) ? size : maxAlign, maxAlign, file, line);
}
#endif
#else
#if __PS3 // special version for gcc when the rage_aligned_new is called on a class that is already ALIGNED
void* operator new(size_t size,size_t alignType,size_t alignParam) {
	size_t maxAlign = alignType > alignParam ? alignType : alignParam;
	return operator_new((size > maxAlign) ? size : maxAlign, maxAlign);
}

void* operator new[](size_t size,size_t alignType,size_t alignParam) {
	size_t maxAlign = alignType > alignParam ? alignType : alignParam;
	return operator_new((size > maxAlign) ? size : maxAlign, maxAlign);
}
#endif
#endif

void operator_delete(void *ptr) {
	using namespace rage;
	if (ptr) {
#if defined(__MFC) || (RSG_ORBIS && SCE_ORBIS_SDK_VERSION < 0x00920020u) || (RSG_DURANGO)
		if (!sysMemAllocator_sm_Current) sysMemAllocator_sm_Current = s_pTheAllocator;
		if (!sysMemAllocator_sm_Master) sysMemAllocator_sm_Master = s_pTheAllocator;
		if (!sysMemAllocator_sm_Container) sysMemAllocator_sm_Container = s_pTheAllocator;
#endif

#if !__FINAL
		if (ptr == sysMemAllocator::sm_BreakOnAddr)
			__debugbreak();
#endif

		sysMemAllocator::GetCurrent().Free(ptr);
	}
}


void operator delete(void *ptr) ORBIS_ONLY(_THROW0()) {
	operator_delete(ptr);
}

void operator delete[](void *ptr) ORBIS_ONLY(_THROW0()) {
	operator_delete(ptr);
}


#if __PPU
// See https://ps3.scedev.net/forums/thread/777?pg=1#n2633 for why we need these:

void *operator new(size_t size,const std::nothrow_t&) {
	return operator_new(size, rage::sysMemAllocator::DEFAULT_ALIGNMENT);
}

void *operator new[](size_t size,const std::nothrow_t&) {
	return operator_new(size, rage::sysMemAllocator::DEFAULT_ALIGNMENT);
}

void operator delete(void *ptr,const std::nothrow_t&) {
	operator_delete(ptr);
}

void operator delete[](void *ptr,const std::nothrow_t&) {
	operator_delete(ptr);
}

void* operator new( size_t size , size_t align, const std::nothrow_t &) {
	return operator_new(size, align);
}

void* operator new[]( size_t size, size_t align, const std::nothrow_t &) {
	return operator_new(size, align);
}

#endif	// __PPU
						 
#else

#endif	// !__TOOL || RSC

#if __XENON && !defined(NO_XMEM_WRAPPERS)

namespace rage {
extern void *XenonMemAlloc(size_t,u32);
extern void XenonMemFree(void*,u32);
extern size_t XenonMemSize(void*,u32);
};

extern "C" {

void * XMemAlloc(unsigned long dwSize,unsigned long dwAllocAttributes) {
	return rage::XenonMemAlloc(dwSize,dwAllocAttributes);
}

void XMemFree(void *pAddress,unsigned long dwAllocAttributes) {
	rage::XenonMemFree(pAddress,dwAllocAttributes);
}

unsigned long XMemSize(void *pAddress,unsigned long dwAllocAttributes) {
	return rage::XenonMemSize(pAddress,dwAllocAttributes);
}

}	// extern "C"

#endif

// Pure virtual trapping
#if __PPU || __PSP2
extern "C" {
	int __cxa_pure_virtual() {
		Quitf("Pure virtual function called.");
		return 0;
	}
}
#elif !__TOOL && !BACKTRACE_ENABLED
int __cdecl _purecall(void) {
	Quitf(rage::ERR_SYS_PURE,"Pure virtual function called.");
	return 0; 
}
#endif

// Catch attempts to call malloc (causing compile error on malloc_stats in SNC currently)
#if __PPU && !defined(NO_DISABLE_MALLOC)
extern "C" {
	void   _malloc_init(void) { }
	void   _malloc_finalize(void) { }
	void*  malloc(size_t size) { return rage_new char[size]; }
	void   free(void *ptr) { delete[] (char*) ptr; }
	void*  calloc(size_t nelem, size_t size) { Quitf("Attempted to calloc %ux%u",nelem,size); return NULL; }
	//void*  realloc(void *ptr, size_t size) { Quitf("Attempted to realloc(%p,%u)",ptr,size); return NULL;	}

	void*  realloc(void *ptr, size_t size) 
	{ 
		char* newptr = rage_new char[size];
		memcpy(newptr,(char*)ptr,size);
		delete[] (char*) ptr;
		return newptr;
		//Quitf("Attempted to realloc(%p,%u)",ptr,size); return NULL;	
	}

	void*  memalign(size_t boundary, size_t size) { return rage_aligned_new(boundary) char[size]; }
	void*  reallocalign(void *ptr, size_t size, size_t boundary) { Quitf("Attempted to reallocalign(%p,%u,%u)",ptr,size,boundary); return NULL; }
	// the std:: shouldn't be necessary, SNC bug
	int    std::malloc_stats(malloc_managed_size *mmsize) { mmsize->current_inuse_size = mmsize->current_system_size = mmsize->max_system_size = 0; return 0; }
	size_t malloc_usable_size(void * /*ptr*/) { __debugbreak(); return 0; }
}
#endif


namespace rage {
	// PURPOSE:	Common toplevel entry point.  main and WinMain both immediately
	//			thunk down to this function.
	// PARAMS:	argc - Arg count as per main
	//			argv - Arg value array as per main
	extern int CommonMain(int argc,char **argv);
#if __PPU
	extern int WrapCommonMain(int argc,char **argv);
#endif

	extern bool g_EnableRfs;
} // namsespace rage

#if __WIN32PC && defined(__WINMAIN) && __WINMAIN
int __stdcall WinMain(struct HINSTANCE__*,struct HINSTANCE__*,char *,int) {
	extern int __argc;
	extern char **__argv;
	int result = ::rage::CommonMain(__argc,__argv);
	return result;
}
#elif RSG_DURANGO
// Application entry point
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	rage::sysApplicationViewSource^ applicationViewSource = ref new rage::sysApplicationViewSource();
	Windows::ApplicationModel::Core::CoreApplication::Run(applicationViewSource);
	return 0;
}
#else
int main(int argc,char **argv) {
#ifdef DISABLE_RFS
	rage::g_EnableRfs = false;
#endif

#if __PPU
	int result = ::rage::WrapCommonMain(argc,argv);
#else
	int result = ::rage::CommonMain(argc,argv);
#endif

#if PORTCULLIS_ENABLE
	rage::portcullis::Shutdown();
#endif // PORTCULLIS_ENABLE

	return result;
}
#endif	// __WINMAIN

#endif // SYSTEM_MAIN_H
