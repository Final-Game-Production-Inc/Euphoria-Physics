// 
// system/bootmgr_xenon.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#include "bootmgr.h"

#include "system/param.h"
#if RSG_ORBIS
#include <sdk_version.h>
#include <sceerror.h>
#include <sys/dmem.h>
#endif
#if RSG_WIN32
#include "system/xtl.h"		// IsDebuggerPresent is available in SDK now
#endif
#include <stdio.h>

namespace rage {

PARAM(forcebootcd,"[startup] Force booting from cd");
PARAM(forceboothdd,"[startup] Force booting from HDD");
PARAM(forceboothddloose,"[startup] Force booting from HDD with loose files");

#if !__FINAL
bool sysBootManager::sm_IsPackagedBuild;
#endif // !__FINAL


#if !__FINAL && (__XENON || __WIN32PC || RSG_DURANGO || RSG_ORBIS) && !__RESOURCECOMPILER && !__TOOL
size_t* g_DebugData;

void sysBootManager::SetDebugDataElement(u32 fourcc, void* value) { 

	if (!sysBootManager::IsDebuggerPresent())
		return;

	if(g_DebugData)
	{
		size_t& debugDataSize = g_DebugData[1];
		for (size_t i = 2; i < debugDataSize; i+=2)
		{
			if (g_DebugData[i] == fourcc)
			{
				g_DebugData[i+1] = (size_t)value;
				return;	
			}
		}

		Assert(debugDataSize < 1024);

		g_DebugData[debugDataSize++] = fourcc;
		g_DebugData[debugDataSize++] = (size_t)value;
	}
}

void sysBootManager::StartDebuggerSupport()
{
	if (sysBootManager::IsDebuggerPresent())
	{
		if (g_DebugData) 
		{ 
			return;
		}

		// To pass additional global data to the debugger, what we do is allocate page at a specific virtual address
		// and then the debugger can read memory at that address. We also store a 4cc at the beginning of the page 
		// so that the debugger knows its really reading debug data and not some other data that happened to be at that address already
		// Each platform might need to use a different address, depending on how address spaces are partitioned for that platform
		// The addresses here don't have any significance other than they are values that are in the address space that we can allocate
#if __XENON
		void * const DEBUG_DATA_PTR = (void *)0x00070000;
#elif __WIN32PC 
		void * const DEBUG_DATA_PTR = (void *)0x000007f000000000; // Near the top of addressable memory (see GetSystemInfo( SYSTEM_INFO& lpSystemInfo) and lpMaximumApplicationAddress)
#elif RSG_DURANGO
		void * const DEBUG_DATA_PTR = (void *)0x0000020700000000;
#elif RSG_ORBIS
		void * const DEBUG_DATA_PTR = (void *)0x70070000;
#endif

#if __XENON || __WIN32PC || RSG_DURANGO
		g_DebugData = (size_t*)VirtualAlloc(DEBUG_DATA_PTR,4096,MEM_RESERVE,PAGE_READWRITE);

		Assertf(g_DebugData, "Couldn't allocate space for debug data. 0x%x\n", GetLastError());

		if (g_DebugData)
		{
			AssertVerify(VirtualAlloc(g_DebugData,4096,MEM_COMMIT,PAGE_READWRITE) != NULL);
#if RSG_CPU_X64
			g_DebugData[0] = MakeFourCc('d', 'b', '6', '4');
#else
			g_DebugData[0] = MakeFourCc('d', '3', 'b', 'g');
#endif
			g_DebugData[1] = 2;
		}
#elif RSG_ORBIS
		size_t memLen = 16 * 1024; // Length: 16KB, minimum for sceKernelAllocateDirectMemory
		ptrdiff_t memStart;
		if (sceKernelAllocateDirectMemory(0, SCE_KERNEL_MAIN_DMEM_SIZE, memLen, 0, 
#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
			SCE_KERNEL_WC_GARLIC_NONVOLATILE,	// I really doubt you meant GPU memory here?
#else
			SCE_KERNEL_WB_ONION,
#endif
			&memStart) == SCE_OK)
		{
			void* start_addr = DEBUG_DATA_PTR;
			if ( sceKernelMapDirectMemory( &start_addr,  memLen, SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_CPU_WRITE, 0, memStart, 0 ) == SCE_OK)
			{
				g_DebugData = (size_t*)start_addr;
				g_DebugData[0] = MakeFourCc('d', 'b', '6', '4');
				g_DebugData[1] = 2;
			}
		}
#endif
	}
}
#endif


}	// namespace rage

