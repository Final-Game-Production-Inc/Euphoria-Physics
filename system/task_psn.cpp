// 
// system/task_psn.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
// This code is heavily based upon samples/raw_spr_intr in the CELL SDK.
//

#define TASK_PSN_DEBUG_CAPTURE_HANDLE_STACKS    0

#include "atl/array.h"
#include "bank/group.h"
#include "diag/channel.h"
#include "diag/output.h"
#include "file/remote.h"
#include "math/intrinsics.h"
#include "string/stringhash.h"
#include "system/bootmgr.h"
#include "system/cellsyncmutex.h"
#include "system/memory.h"
#include "system/nelem.h"
#include "system/param.h"
#include "system/replay.h"
#include "system/spurscheck.h"
#include "system/stack.h"
#include "system/tasklog.h"
#include "system/task_spu_config.h"
#include "system/spurs_gcm_config.h"

namespace rage {
XPARAM(nopopups);
}

#if __PPU

#include "task.h"

#include "atl/queue.h"
#if __ASSERT
#include "atl/map.h"
#endif
#if HACK_GTA4
#include "file/stream.h"
#include "file/device.h"
#include <cell/gcm.h>
#endif // HACK_GTA4
#include "system/bootmgr.h"
#include "system/param.h"
#include "system/stack.h"
#include "system/timer.h"
#include "system/dma.h"
#include "system/criticalsection.h"
#include "profile/cellspurstrace.h"

#include <cell/spurs.h>
#include <stdio.h>
#include <string.h>		// for memset

#include <sys/synchronization.h>

#if TASK_ALLOW_SPU_ADD_JOB
#include <cell/sync/mutex.h>
#endif

#pragma comment(lib,"dbg")
#pragma comment(lib,"spurs_stub")

////// BEGIN SONY SAMPLE CODE //////

/* Lv2 OS headers */
#include <sys/event.h>
#include <sys/spu_thread.h>
#include <sys/timer.h>
#include <sys/ppu_thread.h>
#include <ppu_intrinsics.h>
#include <cell/dbg.h>
#include <sys/dbg.h>

/* for spu_printf */
#include <spu_printf.h>
#include <cell/spurs/lv2_event_queue.h>

DECLARE_TASK_INTERFACE(SignalJobComplete);

#define	TERMINATING_PORT_NAME	0xFEE1DEAD

#define	STACK_SIZE	(1024 * 16)

struct SpursPrintf
{
	sys_event_queue_t	m_equeue;
	sys_ppu_thread_t	m_spu_printf_handler;
	sys_event_port_t	m_terminating_port;
	rage::sysMemAllocator*	m_memAllocator;

	int spurs_printf_service_initialize (CellSpurs *spurs,int prio,const char *desc);
	int spurs_printf_service_finalize (CellSpurs *spurs);
} s_spursPrintf
#if !SYSTEM_WORKLOAD
	,s_spursPrintf2
#endif
;

extern const char *diagAssertTitle;

#if !__FINAL

namespace rage
{

#define EA_GUID_MAP_SIZE    0x120
#define LS_GUID_MAP_SIZE    0x10

static bool IsSpuGuid(u64 addr07, u64 addr8f)
{
	// Is this four ILA instructions with register r2 as the destination, and
	// the least significant two bits of each constant, 0, 1, 2 and 3.
 	// See https://ps3.scedev.net/forums/thread/35449/.
	//   bits (big endian numbering)
	//    0.. 6  ILA opcode (0x21)
	//    7..24  18-bit immediate value
	//   25..31  destination register
	return ((addr07 & 0xFE0001FFFE0001FFull) == 0x4200000242000082ull &&
			(addr8f & 0xFE0001FFFE0001FFull) == 0x4200010242000182ull);
}

static u64 ExtractSpuGuid(u64 addr07, u64 addr8f)
{
	return ((addr07<< 7)&0xffff000000000000uLL)
		 | ((addr07<<23)&0x0000ffff00000000uLL)
		 | ((addr8f>>25)&0x00000000ffff0000uLL)
		 | ((addr8f>> 9)&0x000000000000ffffuLL);
}

struct LsGuidMap
{
	u64 guid;
	u32 ls;
	u32 ea;
};

struct EaGuidMap
{
	u32 guidInsns[4];
	u32 ea;
};

#if !__FINAL

static unsigned ScanSpuGuids(LsGuidMap *out, unsigned maxOut, const EaGuidMap *eaMap, unsigned eaCount, sys_spu_thread_t spuThread)
{
	unsigned num = 0;
	// Think 128-bytes is safe, but this is not performance critical, so may as well check a smaller alignment
	for (u32 ls=0; ls<0x40000; ls+=16)
	{
		u64 insns[2];
		if (sys_spu_thread_read_ls(spuThread, ls+0, insns+0, 8) == CELL_OK && 
			sys_spu_thread_read_ls(spuThread, ls+8, insns+1, 8) == CELL_OK)
		{
			if (IsSpuGuid(insns[0], insns[1]))
			{
				out[num].guid = ExtractSpuGuid(insns[0], insns[1]);
				out[num].ls   = ls;
				out[num].ea   = 0;

				for (unsigned i=0; i<eaCount; ++i)
				{
					if (memcmp(eaMap[i].guidInsns, insns, 16) == 0)
					{
						out[num].ea = eaMap[i].ea;
						break;
					}
				}

				if (++num >= maxOut)
				{
					break;
				}
			}
		}
	}
	return num;
}

#endif

// // Useful debugging function, please do not delete
// static void DisplayEaGuidMap(const EaGuidMap *eaGuidMap, unsigned eaGuidMapSize)
// {
// 	diagLoggedPrintf(TPurple "\n*** ALL SPU GUIDS ***" TNorm "\n");
// 	for (unsigned i=0; i<eaGuidMapSize; ++i)
// 	{
// 		const u64 guid = ExtractSpuGuid(
// 			(u64)eaGuidMap[i].guidInsns[0]<<32 | eaGuidMap[i].guidInsns[1],
// 			(u64)eaGuidMap[i].guidInsns[2]<<32 | eaGuidMap[i].guidInsns[3]);
// 		char g[8] ;
// 		sysMemCpy(g, &guid, 8);
// 		char symName[256]="";
// 		sysStack::ParseMapFileSymbol(symName, 256, eaGuidMap[i].ea);
// 		diagLoggedPrintf(TPurple " %08x : GUID = %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx \"%s\"" TNorm "\n",
// 			eaGuidMap[i].ea, g[0], g[1], g[2], g[3], g[4], g[5], g[6], g[7], symName);
// 	}
// }

static void DisplayLsGuidMap(const LsGuidMap *lsGuidMap, unsigned lsGuidMapSize)
{
	diagLoggedPrintf(TPurple "\n*** SPU GUIDS ***" TNorm "\n");
	for (unsigned i=0; i<lsGuidMapSize; ++i)
	{
		char g[8] ;
		sysMemCpy(g, &lsGuidMap[i].guid, 8);
		char symName[256]="";
		sysStack::ParseMapFileSymbol(symName, 256, lsGuidMap[i].ea);
		diagLoggedPrintf(TPurple " %06x : GUID = %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx \"%s\" (ea 0x%08x)" TNorm "\n",
			lsGuidMap[i].ls, g[0], g[1], g[2], g[3], g[4], g[5], g[6], g[7], symName, lsGuidMap[i].ea);
	}
}

static unsigned LoadEaGuidMap(EaGuidMap *out, unsigned maxOut)
{
	char fileName[256];
	strcpy(fileName, sysParam::GetProgramName());
	if (strrchr(fileName,'.'))
		strcpy(strrchr(fileName,'.'),".cmp");
	else
		strcat(fileName,".cmp");

	unsigned numguids = 0;

	fiStream *S = fiStream::Open(fileName, fiDeviceLocal::GetInstance());
	if (S)
	{
		// read spu guids from compressed map file
		unsigned size = S->Size();
		S->Seek(size-4);
		S->Read(&numguids, 4);
		numguids = (numguids>>24) | ((numguids>>8)&0xff00) | ((numguids<<8)&0xff0000) | (numguids<<24);
		if (numguids > maxOut)
		{
			Assertf(0, "Out of space to load all spu guids (0x%08x > 0x%08x)", numguids, maxOut);
			numguids = maxOut;
		}
		S->Seek(size-4-numguids*sizeof(*out));
		S->Read(out, numguids*sizeof(*out));
		S->Close();

		// eas need endian swapping
		for (unsigned i=0; i<numguids; ++i)
		{
			const u32 ea = out[i].ea;
			out[i].ea = (ea>>24) | ((ea>>8)&0xff00) | ((ea<<8)&0xff0000) | (ea<<24);
		}
	}
	else
	{
		Errorf("Couldn't open map file %s", fileName);
	}

// 	DisplayEaGuidMap(out, numguids);

	return numguids;
}

static const LsGuidMap *FindLsGuidMapEntry(const LsGuidMap *lsGuidMap, unsigned lsGuidMapSize, u32 ls)
{
	if (!lsGuidMapSize || ls<lsGuidMap[0].ls)
	{
		static const LsGuidMap notFound = {0, 0, 0};
		return &notFound;
	}

	// Find the corresponding guid
	unsigned i;
	for (i=1; i<lsGuidMapSize; ++i)
	{
		if (ls<lsGuidMap[i].ls)
		{
			break;
		}
	}
	return lsGuidMap+i-1;
}

static u32 ParseMapFileSymbol(char *funcName, unsigned maxOut, u32 ls, const LsGuidMap *lsGuidMapEntry)
{
	if (!lsGuidMapEntry->ea)
	{
		safecpy(funcName, "?", maxOut);
		return ls;
	}
	return sysStack::ParseMapFileSymbol(funcName, maxOut, ls - lsGuidMapEntry->ls + lsGuidMapEntry->ea);
}

static void DefaultDisplayStackLine(u32 frameNumber, u32 lsAddr, u32 lsOffset, const char *sym, u32 symOffset)
{
	diagLoggedPrintf(TPurple "%2d - 0x%06x (0x%06x) - %s+%x" TNorm "\n", frameNumber, lsAddr, lsOffset, sym, symOffset);
}

static void SpuCallstack(sys_spu_thread_t thread, u32 pc, u32 sp, const LsGuidMap *lsGuidMap, unsigned lsGuidMapSize, unsigned frameNumber, void (*printFn)(u32,u32,u32,const char*,u32)=DefaultDisplayStackLine)
{
	for(unsigned i=frameNumber;; ++i)
	{
		const LsGuidMap *lsGuidMapEntry = FindLsGuidMapEntry(lsGuidMap, lsGuidMapSize, pc);
		char symName[256];
		u32 offset = ParseMapFileSymbol(symName, NELEM(symName), pc, lsGuidMapEntry);
		printFn(i, pc, pc - lsGuidMapEntry->ls, symName, offset);
		u64 nextsp, nextpc;
		if (sys_spu_thread_read_ls(thread, (uint32_t)sp, &nextsp, 4) != CELL_OK || nextsp-1 >= 256*1024-1 ||
			sys_spu_thread_read_ls(thread, (uint32_t)nextsp + 16, &nextpc, 4) != CELL_OK || nextpc >= 256*1024)
			break;
		sp = (u32)nextsp;
		pc = (u32)nextpc;
	}
}

static void SpuCallstack(sys_spu_thread_t thread, u32 pc, u32 lr, u32 sp, const LsGuidMap *lsGuidMap, unsigned lsGuidMapSize)
{
	DisplayLsGuidMap(lsGuidMap, lsGuidMapSize);

#if 0	/// this probably isn't too useful to most people and really clutters the output
	diagLoggedPrintf(TPurple "\n*** SPU ASM ***" TNorm "\n");
	for(u32 addr = pc - 32; addr < pc + 32; addr += 4)
	{
		u64 instr;
		if (sys_spu_thread_read_ls(thread, addr, &instr, 4) == CELL_OK)
			diagLoggedPrintf(TPurple "%s0x%06x: %08x" TNorm "\n",addr==pc?">":" ",addr,(u32)instr);
	}
#endif

	diagLoggedPrintf(TPurple "\n*** SPU CALLSTACK ***" TNorm "\n");
	// Special case to start stack walk.  If we are in a leaf function, then the
	// first stackframe (caller's frame) will not have lr saved into it.  Detect
	// a leaf function by pc and lr not being inside the same function.  This is
	// as close to correct as possible(?), but may miss one level of the stack
	// if a recursive function overflows the stack.
	char symName[256];
	const LsGuidMap *lsGuidMapEntry = FindLsGuidMapEntry(lsGuidMap, lsGuidMapSize, pc);
	u32 pcFuncOffs = ParseMapFileSymbol(symName, 256, pc, lsGuidMapEntry);
	diagLoggedPrintf(TPurple " 0 - 0x%06x (0x%06x) - %s+%x " TNorm "\n", pc, pc - lsGuidMapEntry->ls, symName, pcFuncOffs);
	u32 pcFunc = pc - pcFuncOffs;
	u32 lrFuncOffs = ParseMapFileSymbol(symName, 256, lr, FindLsGuidMapEntry(lsGuidMap, lsGuidMapSize, pc));
	u32 lrFunc = lr - lrFuncOffs;
	// Initialize nextpc here (though not really necissary), just incase a
	// future compiler version detects it as "potentially used when
	// uninitialized".
	u64 nextsp, nextpc=0;
	// Notice that we check the validity of nextsp-1 (not nextsp), this is
	// because the ABI specifies that the back chain is terminated with 0.
	bool invalid = (sys_spu_thread_read_ls(thread, (uint32_t)sp, &nextsp, 4) != CELL_OK || nextsp-1 >= 256*1024-1 ||
			        sys_spu_thread_read_ls(thread, (uint32_t)nextsp + 16, &nextpc, 4) != CELL_OK || nextpc >= 256*1024);
	if(pcFunc == lrFunc)
	{
		// Not a leaf function, so read both sp and lr from the stackframe.
		sp = (u32)nextsp;
		pc = (u32)nextpc;
	}
	else
	{
		// Possibly a leaf function (or no child function yet been called).
		// Don't read lr from stack frame, use the register instead.
		pc = lr;

		// We don't want to unwind an additional stackframe here if we are a
		// leaf function, but we do wan't to in the case of a non-leaf function
		// that just hasn't called anything yet (so lr not modified).  Detect
		// the non-leaf case by checking if the lr stored in the caller's frame
		// is the same as the register.
		//
		// This is not bulletproof.  In the case of a leaf function, the lr slot
		// in the caller's stackframe will be uninitialized, so there is a small
		// chance of an erroneous match.
		//
		sp = (lr == (u32)nextpc) ? (u32)nextsp : sp;
	}
	if(!invalid)
	{
		SpuCallstack(thread, pc, sp, lsGuidMap, lsGuidMapSize, 1);
	}
	diagLoggedPrintf("\n");
}

static void SpuCallstack(sys_spu_thread_t thread, const sys_dbg_spu_thread_context2_t *ctx, const LsGuidMap *lsGuidMap, unsigned lsGuidMapSize)
{
	const u32 pc = ctx->npc;
	const u32 lr = ctx->gpr[0].word[0];
	const u32 sp = ctx->gpr[1].word[0];
	SpuCallstack(thread, pc, lr, sp, lsGuidMap, lsGuidMapSize);
}

} // namespace rage

#endif // !__NO_OUTPUT

XPARAM(bugAsserts);

#if !__FINAL
int spuAssertCallbackDefault(int , const char* , const char* assertText, int defaultIfNoServer)
{
	return rage::fiRemoteShowMessageBox(assertText,diagAssertTitle? diagAssertTitle : rage::sysParam::GetProgramName(),MB_ABORTRETRYIGNORE | MB_ICONQUESTION | MB_TOPMOST | MB_DEFBUTTON3,defaultIfNoServer);
}

// Callback for displaying assert dialog
int (*spuAssertCallback)(int spu, const char* summary, const char* assertText, int defaultIfNoServer) = spuAssertCallbackDefault;
#endif

#if !__FINAL

void SpuCallstackFromAssertString(sys_spu_thread_t thread, const char *text, void (*printFn)(rage::u32,rage::u32,rage::u32,const char*,rage::u32)=rage::DefaultDisplayStackLine)
{
	using namespace rage;

	EaGuidMap eaGuidMap[EA_GUID_MAP_SIZE];
	const unsigned eaGuidMapSize = LoadEaGuidMap(eaGuidMap, NELEM(eaGuidMap));
	LsGuidMap lsGuidMap[LS_GUID_MAP_SIZE];
	const unsigned lsGuidMapSize = ScanSpuGuids(lsGuidMap, NELEM(lsGuidMap), eaGuidMap, eaGuidMapSize, thread);

	// ASSERT: $hhhhh$
	const char *const delim = strchr(text,'$');
	if (!delim || delim[6]!='$')
		return;

	/* Typical SPU callstack: (the value in printBuffer is 26490)
		026490  000264C0 000014C0 000198F0 FFFFF4F0
		0264A0  00004C00 FFFFCE68 FFFFCE68 FFFFCE68
		0264B0  00004C00 FFFFF968 FFFFF968 FFFFF968
		0264C0  00026530 00001530 00019960 FFFFF560
		0264D0  000052D8 00000000 00000000 00000000
		0264E0  00013830 000135F0 000136B0 00013770
		0264F0  00004C00 FFFFCE68 FFFFCE68 FFFFCE68
		026500  00010203 00010203 00010203 00010203
		026510  000156F0 00000800 00000800 00018800
		026520  00026C22 00001C22 0001A052 FFFFFC52
		026530  00026860 00001860 00019C90 FFFFF890
		026540  00009188 00000000 00000000 00000000 */
	u32 sp = strtoul(delim+1,NULL,16);
	u32 pc;

	u64 nextsp, nextpc;
	if (sys_spu_thread_read_ls(thread, (uint32_t)sp, &nextsp, 4) != CELL_OK || nextsp-1 >= 256*1024-1 ||
		sys_spu_thread_read_ls(thread, (uint32_t)nextsp + 16, &nextpc, 4) != CELL_OK || nextpc >= 256*1024)
	{
		diagLoggedPrintf(TPurple "error reading from ls stack" TNorm "\n");
		return;
	}

	sp = (u32)nextsp;
	pc = (u32)nextpc;

	const unsigned frameNumber = 0;
	SpuCallstack(thread, pc, sp, lsGuidMap, lsGuidMapSize, frameNumber, printFn);
}

#endif // !__FINAL

static void SpuPrintfEventHandler(const sys_event_t *event)
{
	using namespace rage;

	const sys_spu_thread_t spu = event->data1;
#if __FINAL
	sys_spu_thread_write_spu_mb(spu, 0);
#else
	// diagLoggedPrintf("%d:%9.4f: ",spu>>24,rage::sysTimer::GetTicks() * rage::sysTimer::GetTicksToMilliseconds());
	char printBuffer[512];
	const int sret = spu_thread_snprintf(printBuffer,sizeof(printBuffer), spu, (uint32_t) event->data3);
	if (strstr(printBuffer,"ASSERT:")) {
		unsigned hashCode = atStringHash(printBuffer);
		const unsigned maxIgnore = 32;
		static unsigned ignores[maxIgnore], ignoreCount;
		int answer = 0;
		for (unsigned i=0; i<ignoreCount; i++)
			if (hashCode == ignores[i]) {
				answer = IDIGNORE;
				break;
			}
		if (!answer) {
			char printBuffer2[512];
			diagLoggedPrintf("{SPU%d} %s",spu>>24,printBuffer);
			SpuCallstackFromAssertString(spu, printBuffer);
			formatf(printBuffer2, sizeof(printBuffer2), "%s%s", printBuffer, "\nSPU ASSERT detected -- see TTY for details.\nPress Abort to stop the program.\nPress Retry to continue onward but halt on this message if it happens again.\nPress Ignore to continue onward and ignore this in the future.");
//			safecat(printBuffer,"\nSPU ASSERT detected -- see TTY for details.\nPress Abort to stop the program.\nPress Retry to continue onward but halt on this message if it happens again.\nPress Ignore to continue onward and ignore this in the future.");
			int defaultIfNoServer = IDIGNORE;
			const char *nopopups;
			if (PARAM_nopopups.Get(nopopups))
			{
				if (!stricmp(nopopups,"abort"))
					defaultIfNoServer = IDABORT;
				else if (!stricmp(nopopups,"retry"))
					defaultIfNoServer = IDRETRY;
			}
			answer = spuAssertCallback(spu, printBuffer, printBuffer2, defaultIfNoServer);
/*			if(PARAM_bugAsserts.Get())
			{
				answer = fiRemoteShowAssertMessageBox(printBuffer2,diagAssertTitle? diagAssertTitle : sysParam::GetProgramName(),MB_ABORTRETRYIGNORE | MB_ICONQUESTION | MB_TOPMOST | MB_DEFBUTTON3,defaultIfNoServer);

				// Call the callback that is used for logging/updated the associated assert bug
				// (ID_CUSTOM4 indicates that they wish to view the bug)
				spuLogBugCallback(spu, printBuffer, printBuffer, hashCode, (answer == IDCUSTOM4));

				// Modify the result to something appropriate if IDCUSTOM4 was returned
				if (answer == IDCUSTOM4)
				{
					answer = fiRemoteShowMessageBox(printBuffer2,diagAssertTitle?diagAssertTitle:sysParam::GetProgramName(),MB_ABORTRETRYIGNORE | MB_ICONQUESTION | MB_TOPMOST | MB_DEFBUTTON3,defaultIfNoServer);
				}
				answer = IDIGNORE;
			}
			else
					answer = fiRemoteShowMessageBox(printBuffer2,diagAssertTitle? diagAssertTitle : sysParam::GetProgramName(),MB_ABORTRETRYIGNORE | MB_ICONQUESTION | MB_TOPMOST | MB_DEFBUTTON3,defaultIfNoServer);*/
			if (answer == IDIGNORE && ignoreCount < maxIgnore)
				ignores[ignoreCount++] = hashCode;
		}
		sys_spu_thread_write_spu_mb(spu, answer != IDABORT);
	}
	else {
		int ret = sys_spu_thread_write_spu_mb(spu, sret);
		if (ret) {
			diagLoggedPrintf("sys_spu_thread_write_spu_mb failed (%d)\n", ret);
			sys_ppu_thread_exit(~0U);
		}
		diagLoggedPrintf("{SPU%d} %s",spu>>24,printBuffer);
		if (strstr(printBuffer,"HALT:") || strstr(printBuffer,"(halt)")) {
			SpuCallstackFromAssertString(spu, printBuffer);
			safecat(printBuffer,"\nSPU halt detected -- see TTY for details.");
			fiRemoteShowMessageBox(printBuffer,diagAssertTitle? diagAssertTitle : rage::sysParam::GetProgramName(),MB_OK|MB_ICONERROR|MB_TOPMOST,IDOK);
		}
	}
#endif // __FINAL
}

static rage::SpuEventHandler *s_spuEventHandlers[rage::MAX_NUM_SUPPORTED_EVENTCMDS] =
{
	SpuPrintfEventHandler,
};
CompileTimeAssert(rage::EVENTCMD_PRINTF == 0);

/*static*/ void rage::sysTaskManager::SetSpuEventHandler(unsigned id, rage::SpuEventHandler *handler)
{
	Assert(id < NELEM(s_spuEventHandlers));
	s_spuEventHandlers[id] = handler;
}

static void SpuEventHandlerThread(uint64_t arg)
{
	SpursPrintf *that = (SpursPrintf*)(size_t) arg;

	// Because this thread is created directly, sysIpcThreadWrapper is not
	// called.  Therefore we need to manually setup the memory allocator here.
	rage::sysMemAllocator::SetMaster(*(that->m_memAllocator));
	rage::sysMemAllocator::SetCurrent(*(that->m_memAllocator));
	rage::sysMemAllocator::SetContainer(*(that->m_memAllocator));

	for (;;) {
		sys_event_t	event;
		NOTFINAL_ONLY(int ret =) sys_event_queue_receive(that->m_equeue, &event, 0);

		if (event.source == TERMINATING_PORT_NAME) {
			break;
		}

		const uint32_t eventId = (uint32_t)(event.data2 & 0x00ffffff);
		rage::SpuEventHandler *const func = s_spuEventHandlers[eventId];
		func(&event);
	}
	OUTPUT_ONLY(rage::diagLoggedPrintf("Finalizing the spu event handler\n");)
	sys_ppu_thread_exit(0);
}

int SpursPrintf::spurs_printf_service_initialize (CellSpurs *spurs,int prio,const char *desc)
{
	int	ret;
	/* create event_queue for printf */
	sys_event_queue_attribute_t	attr;
	sys_event_queue_attribute_initialize (attr);

	/* queue depth must be equal or greater than max_spu_threads */
	ret = sys_event_queue_create (&m_equeue, &attr, SYS_EVENT_QUEUE_LOCAL, 8);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("sys_event_queue_create failed (%d)\n", ret);)
		return ret;
	}

	/* Add the master memory allocator for use by this thread */
	m_memAllocator = &(rage::sysMemAllocator::GetMaster());

	/* 		create ppu_thread for printf handling */
	ret = sys_ppu_thread_create (&m_spu_printf_handler, SpuEventHandlerThread, (uint64_t)this, prio, STACK_SIZE, SYS_PPU_THREAD_CREATE_JOINABLE, desc);

	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("sys_ppu_thread_create failed (%d)\n", ret);)
		return ret;
	}

	/*
	* Create the terminating port. This port is used only in 
	* spu_printf_service_finalize().
	*/
	ret = sys_event_port_create(&m_terminating_port, SYS_EVENT_PORT_LOCAL, TERMINATING_PORT_NAME);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("spu_printf_server_initialize: sys_event_port_create failed %d\n", ret);)
		return ret;
	}

	ret = sys_event_port_connect_local(m_terminating_port, m_equeue);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("spu_printf_server_initialize: sys_event_connect failed %d\n", ret);)
		return ret;
	}

	/* connect to SPURS */
	uint8_t	port = rage::SYSTASKMANAGER_EVENT_PORT;
	ret = cellSpursAttachLv2EventQueue (spurs, m_equeue, &port, 0);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("spu_printf_server_initialize: cellSpursAttachLv2EventQueue failed %d\n", ret);)
		return ret;
	}

	return CELL_OK;
}

int SpursPrintf::spurs_printf_service_finalize (CellSpurs *spurs)
{
	int ret;
	uint64_t	exit_code;

	/*
	*
	*/
	ret = cellSpursDetachLv2EventQueue (spurs, rage::SYSTASKMANAGER_EVENT_PORT);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("cellSpursDetachLv2EventQueue failed %d\n", ret);)
		return ret;
	}

	/*
	* send event for temination.
	*/
	ret = sys_event_port_send (m_terminating_port, 0, 0, 0);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("sys_event_port_send failed %d\n", ret);)
		return ret;
	}
	/*	wait for termination of the handler thread */
	ret = sys_ppu_thread_join (m_spu_printf_handler, &exit_code);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("sys_ppu_thread_join failed %d\n", ret);)
		return ret;
	}

	/* Disconnect and destroy the terminating port */
	ret = sys_event_port_disconnect(m_terminating_port);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("sys_event_disconnect failed %d\n", ret);)
		return ret;
	}
	ret = sys_event_port_destroy(m_terminating_port);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("sys_event_port_destroy failed %d\n", ret);)
		return ret;
	}	

	/*	clean event_queue for spu_printf */
	ret = sys_event_queue_destroy (m_equeue, 0);
	if (ret) {
		OUTPUT_ONLY(rage::diagLoggedPrintf("sys_event_queue_destroy failed %d\n", ret);)
		return ret;
	}

	return CELL_OK;
}

////// END SONY SAMPLE CODE //////


namespace rage {

int SpuThreadReadLs(void *ea, sys_spu_thread_t spuThread, u32 ls, u32 size)
{
	u32 curLs = ls;
	u8* curEa = (u8*)(ea);
	u32 sizeRemaining = size;

	while (sizeRemaining&~7)
	{
		const int ret = sys_spu_thread_read_ls(spuThread, curLs, (u64*)curEa, 8);
		if (Unlikely(ret != CELL_OK))
		{
			return ret;
		}
		curLs += 8;
		curEa += 8;
		sizeRemaining -= 8;
	}
	if (sizeRemaining&4)
	{
		u64 tmp;
		const int ret = sys_spu_thread_read_ls(spuThread, curLs, &tmp, 4);
		if (Unlikely(ret != CELL_OK))
		{
			return ret;
		}
		*(u32*)curEa = (u32)tmp;
		curLs += 4;
		curEa += 4;
		sizeRemaining -= 4;
	}
	if (sizeRemaining&2)
	{
		u64 tmp;
		const int ret = sys_spu_thread_read_ls(spuThread, curLs, &tmp, 2);
		if (Unlikely(ret != CELL_OK))
		{
			return ret;
		}
		*(u16*)curEa = (u16)tmp;
		curLs += 2;
		curEa += 2;
		sizeRemaining -= 2;
	}
	if (sizeRemaining&1)
	{
		u64 tmp;
		const int ret = sys_spu_thread_read_ls(spuThread, curLs, &tmp, 1);
		if (Unlikely(ret != CELL_OK))
		{
			return ret;
		}
		*(u8*)curEa = (u8)tmp;
	}

	return CELL_OK;
}

#if RAGETRACE
pfSpuTrace* g_pfSpuTrace[6] ;
#endif

sysTaskParameters::sysTaskParameters() { memset(this,0,sizeof(*this)); }

void *g_SpursReportArea=NULL;
CellSpurs *g_Spurs = NULL;
CellSpursAttribute g_SpursAttributes;
const char* g_SpursPrefix = "Primary";		// In SN Tuner, specify PrimaryCellSpursKernel0-4
#if !SYSTEM_WORKLOAD
CellSpurs *g_Spurs2 = NULL;
CellSpursAttribute g_Spurs2Attributes;
const char* g_Spurs2Prefix = "Secondary";	// In SN Tuner, specify SecondaryCellSpursKernel0
#endif

struct __sysTaskHandle {
	CellSpursJob256 MainJob;
};

static const int MaxTasks = TASK_MAX_TASKS_SPU;

typedef u8 TypeFreeListIdx;
CompileTimeAssert(MaxTasks < 256);

template <class T,size_t size> class sysLockFreeStack {
public:
	enum { INVALID = (T)~0 };
	sysLockFreeStack()
	{
		CompileTimeAssert((1uLL<<(sizeof(T)*8)) > size);
		for (unsigned i=0; i<size-1; ++i) {
			m_Storage[i] = (T)(i+1);
		}
		m_Storage[size-1] = INVALID;
		m_Head = 0;
	}
	void Push(T elem)
	{
		const u32 newHead = elem;
		u32 oldHead;
		do {
			oldHead = __lwarx(&m_Head);
			m_Storage[newHead] = (T)oldHead;
			__lwsync();
		} while(Unlikely(!__stwcx(&m_Head, newHead)));
	}
	T Pop()
	{
		u32 oldHead, newHead;
		do {
			oldHead = __lwarx(&m_Head);
			if (Unlikely(oldHead == INVALID)) {
				// clear reservation
				unsigned scratch;
				__stwcx(&scratch, 0);
				return INVALID;
			}
			newHead = m_Storage[oldHead];
		} while (Unlikely(!__stwcx(&m_Head, newHead)));
		return (T)oldHead;
	}
private:
	T m_Storage[size];
	volatile u32 m_Head;
};

typedef sysLockFreeStack<TypeFreeListIdx,MaxTasks> FreeListType;
FreeListType s_FreeList;

// If you see assertions related to userSize, change MainJob to be of type CellSpursJob256
static const int userSize = sizeof(__sysTaskHandle) == 256? 26 : 10;
#define Semaphore MainJob.workArea.userData[userSize-1]

struct JobChain 
{
	static const int MaxCommands = MaxTasks + 8;

	void Init(CellSpurs *spurs,const uint8_t *jobChainPriorities,int maxContention,const char *name);

#if !__FINAL
	void UpdatePriorities(CellSpurs *spurs,const uint8_t *jobChainPriorities);
#endif // !__FINAL

	void Shutdown();
	void AddJob(CellSpursJob256 *job);

	CellSpursJobChain m_Chain;
	uint64_t m_CommandList[MaxCommands];
	int m_CommandIndex;

#if TASK_ALLOW_SPU_ADD_JOB
	// job chain mutex so SPUs and PPUs can add to same job chain.
	RageCellSyncMutex m_JobChainSyncMutex;
#endif
	sysCriticalSectionToken m_JobChainCritSec;
};

void JobChain::Init(CellSpurs *spurs,const uint8_t *jobChainPriorities,int maxContention,const char *name)
{
	const int maxJobsToFetchAtOnce = 1;
	CellSpursJobChainAttribute jobChainAttributes;
	
	uint32_t readyCount = 0; // autoReadyCount - let spurs decide
	SPURS_CHECK(cellSpursJobChainAttributeInitialize(&jobChainAttributes, &m_CommandList[0], 
		sizeof(__sysTaskHandle), maxJobsToFetchAtOnce, jobChainPriorities, 
		maxContention, readyCount? false: true/*autoReadyCount*/, 2/*tag0*/, 3/*tag1*/, 
		false /*isFixedMemAlloc*/, sizeof(__sysTaskHandle), readyCount/*readyCount*/));
	SPURS_CHECK(cellSpursJobChainAttributeSetName(&jobChainAttributes, name));
	SPURS_CHECK(cellSpursJobChainAttributeSetHaltOnError(&jobChainAttributes));
	SPURS_CHECK(cellSpursCreateJobChainWithAttribute(spurs, &m_Chain, &jobChainAttributes));

	sysTaskManager::SetExceptionHandler(&m_Chain);

	m_CommandIndex = 0;
	m_CommandList[MaxCommands-1] = CELL_SPURS_JOB_COMMAND_NEXT(&m_CommandList[0]);

#if TASK_ALLOW_SPU_ADD_JOB
	// for job chain synchronization.
	rageCellSyncMutexInitialize(&m_JobChainSyncMutex);
#endif
}

#if !__FINAL
void JobChain::UpdatePriorities(CellSpurs *spurs,const uint8_t *jobChainPriorities)
{
	CellSpursWorkloadId workloadId;

	SPURS_CHECK(cellSpursGetJobChainId(&m_Chain, &workloadId));

	char debugOutput[256];

	formatf(debugOutput, "Updating priorities for workload %x: ", workloadId);

	for (int x=0; x<CELL_SPURS_MAX_SPU; x++)
	{
		char numberStr[16];
		formatf(numberStr, "%d", jobChainPriorities[x]);

		safecat(debugOutput, numberStr);

		if (x != CELL_SPURS_MAX_SPU - 1)
		{
			safecat(debugOutput, ",");
		}
	}

	Displayf(debugOutput);

	SPURS_CHECK(cellSpursSetPriorities(spurs, workloadId, jobChainPriorities));
}
#endif // !__FINAL

#if !__FINAL
struct spuException
{
	CellSpurs* spurs;
	CellSpursJobChain* jobChain;
	CellSpursExceptionInfo exception;
	const void* eaJobBinary;
	uint32_t lsAddrJobBinary;
};
static spuException g_spuException = {0};

void JobChainExceptionHandler(CellSpurs* spurs,
								CellSpursJobChain* jobChain,
								const CellSpursExceptionInfo* exception,
								const void* eaJobBinary,
								uint32_t lsAddrJobBinary,
								void* /*arg*/)
{
	// Sleep to make sure any lv2 os output has finished, so that it won't get
	// interleaved with ours
	const unsigned ms=500;
	sysIpcSleep(ms);

	g_spuException.spurs = spurs;
	g_spuException.jobChain = jobChain;
	g_spuException.exception = *exception;
	g_spuException.eaJobBinary = eaJobBinary;
	g_spuException.lsAddrJobBinary = lsAddrJobBinary;
	
	// Wake up the PPU exception handler thread.  This is not the same as
	// triggering an exception on the PPU, so saves getting all the lv2
	// exception handler output as well.
	const uint64_t flags=0;
	while(sys_dbg_signal_to_ppu_exception_handler(flags) != CELL_OK) sysIpcSleep(ms);

	// Can't safetly continue
	for (;;) sysIpcSleep(ms);
}


void TaskSetExceptionHandler(CellSpurs* spurs,
							 CellSpursTaskset* taskSet,
							 CellSpursTaskId taskId,
							 const CellSpursExceptionInfo* exception,
							 void* /*arg*/)
{
	// Sleep to make sure any lv2 os output has finished, so that it won't get
	// interleaved with ours
	const unsigned ms=500;
	sysIpcSleep(ms);

	CellSpursTasksetInfo info;
	cellSpursGetTasksetInfo(taskSet, &info);    
	g_spuException.spurs = spurs;
	g_spuException.jobChain = 0;
	g_spuException.exception = *exception;
	g_spuException.eaJobBinary = info.taskInfo[taskId].eaElf;
	g_spuException.lsAddrJobBinary = 0;

	// Wake up the PPU exception handler thread.  This is not the same as
	// triggering an exception on the PPU, so saves getting all the lv2
	// exception handler output as well.
	const uint64_t flags=0;
	while(sys_dbg_signal_to_ppu_exception_handler(flags) != CELL_OK) sysIpcSleep(ms);

	// Can't safetly continue
	for (;;) sysIpcSleep(ms);
}

void GlobalExceptionHandler(CellSpurs* spurs,
							const CellSpursExceptionInfo *exception,
							CellSpursWorkloadId /*id*/,
							void* /*arg*/)
{    
	// Sleep to make sure any lv2 os output has finished, so that it won't get
	// interleaved with ours
	const unsigned ms=500;
	sysIpcSleep(ms);

	g_spuException.spurs = spurs;
	g_spuException.jobChain = 0;
	g_spuException.exception = *exception;
	g_spuException.eaJobBinary = 0;
	g_spuException.lsAddrJobBinary = 0;

	// Wake up the PPU exception handler thread.  This is not the same as
	// triggering an exception on the PPU, so saves getting all the lv2
	// exception handler output as well.
	const uint64_t flags=0;
	while(sys_dbg_signal_to_ppu_exception_handler(flags) != CELL_OK) sysIpcSleep(ms);

	// Can't safetly continue
	for (;;) sysIpcSleep(ms);
}

static void DisplaySpuRegisters(const sys_dbg_spu_thread_context2_t *ctx);

#if HANG_DETECT_THREAD
void SpuHangHandler()
{
	EaGuidMap eaGuidMap[EA_GUID_MAP_SIZE];
	const unsigned eaGuidMapSize = LoadEaGuidMap(eaGuidMap, NELEM(eaGuidMap));

	diagLoggedPrintf(TPurple "*** SPU Threads" TNorm "\n");
	sys_spu_thread_group_t spuThreadGroups[32];
	u64 numSpuThreadGroups = 32;
	u64 totalNumSpuThreadGroups;
	sys_dbg_get_spu_thread_group_ids(spuThreadGroups, &numSpuThreadGroups, &totalNumSpuThreadGroups);
	for(u64 i=0; i<numSpuThreadGroups; ++i)
	{
		sys_spu_thread_t threads[32];
		u64 numThreads = 32;
		u64 totalNumThreads;
		sys_dbg_get_spu_thread_ids(spuThreadGroups[i], threads, &numThreads, &totalNumThreads);
		char threadGroupName[256];
		sys_dbg_get_spu_thread_group_name(spuThreadGroups[i], threadGroupName);
		for(u64 j=0; j<numThreads; ++j)
		{
			sys_dbg_spu_thread_context2_t context;
			if (sys_dbg_read_spu_thread_context2(threads[j], &context) == CELL_OK)
			{
				char threadName[256];            
				sys_dbg_get_spu_thread_name(threads[j], threadName);
				diagLoggedPrintf(TPurple "### thread [%s:%s]\n NPC = %x spu_status = %x" TNorm "\n", threadGroupName, threadName, context.npc, context.spu_status);
				LsGuidMap lsGuidMap[LS_GUID_MAP_SIZE];
				const unsigned lsGuidMapSize = ScanSpuGuids(lsGuidMap, NELEM(lsGuidMap), eaGuidMap, eaGuidMapSize, threads[j]);
				SpuCallstack(threads[j], &context, lsGuidMap, lsGuidMapSize);
				diagLoggedPrintf(TPurple "## thread end [%s:%s]\n" TNorm "\n", threadGroupName, threadName);
			}
		}
	}
}
#endif // HANG_DETECT_THREAD


bool SpuExceptionHandler()
{
	if (!g_spuException.exception.spu_thread)
		return false;

	sys_spu_thread_t thread = g_spuException.exception.spu_thread;
	u32 pc = g_spuException.exception.spu_npc;
	u32 cause = g_spuException.exception.cause;
	u32 base = g_spuException.lsAddrJobBinary;
	char name[256];
	sys_dbg_get_spu_thread_name(thread, name);
	diagLoggedPrintf(TPurple "*** SPURS EXCEPTION CAUGHT in thread [%s] cause %x ( ",name,cause);
	if (cause & SYS_SPU_EXCEPTION_DMA_ALIGNMENT)    diagLoggedPrintf("SYS_SPU_EXCEPTION_DMA_ALIGNMENT ");
	if (cause & SYS_SPU_EXCEPTION_DMA_COMMAND)      diagLoggedPrintf("SYS_SPU_EXCEPTION_DMA_COMMAND ");
	if (cause & SYS_SPU_EXCEPTION_SPU_ERROR)        diagLoggedPrintf("SYS_SPU_EXCEPTION_SPU_ERROR ");
	if (cause & SYS_SPU_EXCEPTION_MFC_FIR)          diagLoggedPrintf("SYS_SPU_EXCEPTION_MFC_FIR ");
	if (cause & SYS_SPU_EXCEPTION_MFC_SEGMENT)      diagLoggedPrintf("SYS_SPU_EXCEPTION_MFC_SEGMENT ");
	if (cause & SYS_SPU_EXCEPTION_MFC_STORAGE)      diagLoggedPrintf("SYS_SPU_EXCEPTION_MFC_STORAGE ");
	if (cause & SYS_SPU_EXCEPTION_STOP_CALL)        diagLoggedPrintf("SYS_SPU_EXCEPTION_STOP_CALL ");
	if (cause & SYS_SPU_EXCEPTION_STOP_BREAK)       diagLoggedPrintf("SYS_SPU_EXCEPTION_STOP_BREAK ");
	if (cause & SYS_SPU_EXCEPTION_HALT)             diagLoggedPrintf("SYS_SPU_EXCEPTION_HALT ");
	if (cause & SYS_SPU_EXCEPTION_UNKNOWN_SIGNAL)   diagLoggedPrintf("SYS_SPU_EXCEPTION_UNKNOWN_SIGNAL ");
	diagLoggedPrintf(") at address %x (%x)" TNorm "\n", pc, pc - base);

	sys_dbg_spu_thread_context2_t context;
	if (sys_dbg_read_spu_thread_context2(thread, &context) == CELL_OK)
	{
		DisplaySpuRegisters(&context);
		EaGuidMap eaGuidMap[EA_GUID_MAP_SIZE];
		const unsigned eaGuidMapSize = LoadEaGuidMap(eaGuidMap, NELEM(eaGuidMap));
		LsGuidMap lsGuidMap[LS_GUID_MAP_SIZE];
		const unsigned lsGuidMapSize = ScanSpuGuids(lsGuidMap, NELEM(lsGuidMap), eaGuidMap, eaGuidMapSize, thread);
		SpuCallstack(thread, &context, lsGuidMap, lsGuidMapSize);

		if((cause&SYS_SPU_EXCEPTION_HALT) && ((s32)context.gpr[1].word[1]<0))
		{
			diagLoggedPrintf(TPurple "possible stack overflow\n" TNorm "\n");
		}
	}
	else
	{
		diagLoggedPrintf(TPurple "Error getting spu thread context" TNorm "\n");
	}

	return true;
}

static void DisplaySpuRegisters(const sys_dbg_spu_thread_context2_t *ctx)
{
	for (int i=0,j=64; i<64; ++i,++j)
		diagLoggedPrintf(TPurple "r%03d=[%08x|%08x|%08x|%08x]    r%03d=[%08x|%08x|%08x|%08x]" TNorm "\n",
			i,ctx->gpr[i].word[0],ctx->gpr[i].word[1],ctx->gpr[i].word[2],ctx->gpr[i].word[3],
			j,ctx->gpr[j].word[0],ctx->gpr[j].word[1],ctx->gpr[j].word[2],ctx->gpr[j].word[3]);
	diagLoggedPrintf("\n");

	// Note that we can probably remove this when we update SDK versions.
	// Sony's 3.70 firmware displays this information for us.
	for (unsigned i=0,i4=0; i<24; ++i,i4+=4 )
	{
		diagLoggedPrintf(TPurple "mfc_cq_sr[%02u]  %08x %08x %08x %08x %08x %08x %08x %08x" TNorm "\n",i,
			(u32)(ctx->mfc_cq_sr[i4+0]>>32), (u32)ctx->mfc_cq_sr[i4+0],
			(u32)(ctx->mfc_cq_sr[i4+1]>>32), (u32)ctx->mfc_cq_sr[i4+1],
			(u32)(ctx->mfc_cq_sr[i4+2]>>32), (u32)ctx->mfc_cq_sr[i4+2],
			(u32)(ctx->mfc_cq_sr[i4+3]>>32), (u32)ctx->mfc_cq_sr[i4+3] );
	}
}

#endif


void JobChain::Shutdown()
{
	SPURS_CHECK(cellSpursShutdownJobChain(&m_Chain));
	SPURS_CHECK(cellSpursJoinJobChain(&m_Chain));
}

extern bool g_IsExiting;

void JobChain::AddJob(CellSpursJob256 *job)
{
	SYS_CS_SYNC(m_JobChainCritSec);

#if TASK_ALLOW_SPU_ADD_JOB
	rageCellSyncMutexLock(&m_JobChainSyncMutex);
	// Note that rageCellSyncMutexLock does a lwsync for us
#endif

	// Check this now before we actually add the job, otherwise the assert could fire incorrectly
	// when the job really did manage to run to completion between the two lines of code!
	Assertf(job->workArea.userData[25],"ReadyHeader failed at %p?",&job->workArea.userData[25]);

	// Have we filled up a job chain command list yet?
	int nextCommand = m_CommandIndex+1;
	if (nextCommand == MaxCommands-1)
		nextCommand = 0;
	// Store the next END command first in case the chain is already running
	m_CommandList[nextCommand] = CELL_SPURS_JOB_COMMAND_END;
	sys_lwsync();
	m_CommandList[m_CommandIndex] = CELL_SPURS_JOB_COMMAND_JOB(job);
	sys_lwsync();
	m_CommandIndex = nextCommand;
	
	while (g_IsExiting)		// If we're shutting down, SPURS might be unavailable and this has a small chance of crashing.
		sysIpcSleep(1000);

#if TASK_ALLOW_SPU_ADD_JOB
	rageCellSyncMutexUnlock(&m_JobChainSyncMutex);
#endif

	// DEBUG_TASK_COMPLETION_ONLY(s_TaskLog.Log('KICK',u32(&job->workArea.userData[25]),0,0));

	SPURS_CHECK(cellSpursRunJobChain(&m_Chain));
}

struct JobChainData
{
	const char* m_Name;
	int m_SpursInstance;
	int m_MaxContention;
	uint8_t m_SpuPriorities[CELL_SPURS_MAX_SPU];
};

#if HACK_GTA4
JobChainData jobData[sysTaskManager::NUM_SCHEDULES] = 
{
	// Name						Inst		Contention		Priorities (SYSTEM_WORKLOAD can change entry[5] to nonzero)
	{"DefaultShort",			0,			8,				{1,  1,  1,  1,  15, 0,  0,  0}},
	{"DefaultLong",				0,			8,				{8,  8,	 8,  8,  0,  0,  0,  0}},
	{"ShapeTests",				0,			8,				{4,  4,  4,  4,  0,  0,  0,  0}},
	{"AudioEngine",				0,			8,				{4,  4,  4,  4,  0,  0,  0,  0}},
	{"GraphicsMain",			0,			1,				{0,  0,  0,  0,  1,  0,  0,  0}},
	{"GraphicsOther",			0,			1,				{6,  0,  0,  0,  0,  0,  0,  0}},
	{"Secondary",				1,			1,				{SYSTEM_WORKLOAD?0:4,  0,  0,  0,  0,  SYSTEM_WORKLOAD?4:0,  0,  0}},
};
#else
JobChainData jobData[sysTaskManager::NUM_SCHEDULES] = 
{
	// Name						Inst		Contention		Priorities
	{"DefaultJobs",				0,			5,				{ 8,  8,  8,  8,  0,  0,  0,  0}},
	{"FragmentPatchJobs",		0,			1,				{ 0,  0,  0,  0,  2,  0,  0,  0}},
	{"OtherRsxJobs",			0,			1,				{ 6,  6,  0,  0,  0,  0,  0,  0}},
	{"SecondaryGroupJobs",		1,			1,				{ 8,  0,  0,  0,  0,  0,  0,  0}},
	{"LowPriorityJobs_1on2",	0,			1,				{ 0, 10,  0, 10,  0,  0,  0,  0}},
	{"LowPriorityJobs_2on4",	0,			2,				{10, 10, 10, 10,  0,  0,  0,  0}},
	{"LowPriorityJobs_4on4",	0,			4,				{10, 10, 10, 10,  0,  0,  0,  0}},
	{"Sequential_A",			0,			1,				{ 8,  8,  8,  8,  0,  0,  0,  0}},
	{"Sequential_B",			0,			1,				{ 8,  8,  8,  8,  0,  0,  0,  0}},
	{"DefaultJobs_5on5",		0,			5,				{ 8,  8,  8,  8,  8,  0,  0,  0}},
};
#endif // HACK_GTA4

JobChain s_JobChains[sysTaskManager::NUM_SCHEDULES];

const int NUM_SPURS_THREADS_PRIMARY = 5 + SYSTEM_WORKLOAD;

static __sysTaskHandle s_Handles[MaxTasks] ALIGNED(256);		// salt code expects lower eight bits to be zero
static atRangeArray<sys_event_flag_t,(TASK_MAX_TASKS_SPU+63)/64> s_CompletionFlags;

#if TASK_PSN_DEBUG_CAPTURE_HANDLE_STACKS
#define TASK_PSN_DEBUG_CAPTURE_HANDLE_STACKS_ONLY(...)  __VA_ARGS__
static size_t s_HandleStacks[MaxTasks][16];
#else
#define TASK_PSN_DEBUG_CAPTURE_HANDLE_STACKS_ONLY(...)
#endif

#if !__FINAL
static atRangeArray<u8,MaxTasks> s_HandleSalt;

static __sysTaskHandle* AddSalt(__sysTaskHandle* h)
{
	Assert(((size_t)h & 255) == 0);
	ptrdiff_t i = h - s_Handles;
	return (__sysTaskHandle*)(size_t(h) | ++s_HandleSalt[i]);
}

static void RemoveSalt(__sysTaskHandle* &h)
{
	size_t salt = size_t(h) & 255;
	h = (__sysTaskHandle*)(size_t(h) & ~255);
	// Trap if the stored salt doesn't match what we actually got
	ptrdiff_t i = h - s_Handles;
	if (s_HandleSalt[i] != salt) {
		Errorf("Handle %p was passed in with salt %d but we expected %d",h,salt,s_HandleSalt[i]);
		__debugbreak();
	}
	// We could bump the salt again now as additional protection on double waits but then Poll would need a separate code path
}

#else
#define AddSalt(x) x
#define RemoveSalt(x)
#endif


void sysTaskManager__SysInitClass()
{
	CHECKPOINT_SYSTEM_MEMORY;

	// Spurs handler must have higher priority than the main thread 
	// (and the completion wait thread)
	sys_ppu_thread_t idMainPpuThread;
	int prioMainPpuThread;
	SPURS_CHECK(sys_ppu_thread_get_id(&idMainPpuThread));
	SPURS_CHECK(sys_ppu_thread_get_priority(idMainPpuThread, &prioMainPpuThread));
	int spursHandlerPPUPriority = prioMainPpuThread - 32;

	CHECKPOINT_SYSTEM_MEMORY;

	sys_addr_t addr;
	SPURS_CHECK(sys_memory_allocate(MAPPED_GCM_AREA_SIZE, SYS_MEMORY_PAGE_SIZE_1M, &addr));
	g_SpursReportArea = (void*)addr;	
	CompileTimeAssert((GCM_REPORT_AREA_SIZE & 127) == 0);
	g_Spurs =  (CellSpurs*)((char*)g_SpursReportArea + GCM_REPORT_AREA_SIZE);
#if !SYSTEM_WORKLOAD
	g_Spurs2 = (CellSpurs*)((char*)g_Spurs + sizeof(*g_Spurs));
#endif

	// Primary group - using 5 SPUs - should never get preempted
	SPURS_CHECK(cellSpursAttributeInitialize(&g_SpursAttributes, NUM_SPURS_THREADS_PRIMARY, 200, spursHandlerPPUPriority, false));
#if !SYSTEM_WORKLOAD
	SPURS_CHECK(cellSpursAttributeSetSpuThreadGroupType(&g_SpursAttributes, SYS_SPU_THREAD_GROUP_TYPE_EXCLUSIVE_NON_CONTEXT));
#else
	const uint8_t system_workload_prio[8] = { 0, 0, 0, 0, 0, 1, 0, 0 };
	const bool is_preemptible[8] = { false, false, false, false, false, true, false, false };
	SPURS_CHECK(cellSpursAttributeEnableSystemWorkload(&g_SpursAttributes, system_workload_prio, 6, is_preemptible));
#endif
	SPURS_CHECK(cellSpursAttributeSetNamePrefix(&g_SpursAttributes, g_SpursPrefix, strlen(g_SpursPrefix)));
	SPURS_CHECK(cellSpursInitializeWithAttribute(g_Spurs, &g_SpursAttributes));

	CHECKPOINT_SYSTEM_MEMORY;

#if !SYSTEM_WORKLOAD
	// Secondary group - lower priority will share the 6th SPU with DDLEnc and other OS stuff
	SPURS_CHECK(cellSpursAttributeInitialize(&g_Spurs2Attributes, 1, 201, spursHandlerPPUPriority, false));
	SPURS_CHECK(cellSpursAttributeSetNamePrefix(&g_Spurs2Attributes, g_Spurs2Prefix, strlen(g_Spurs2Prefix))); 
	SPURS_CHECK(cellSpursInitializeWithAttribute(g_Spurs2, &g_Spurs2Attributes));
#endif

	CHECKPOINT_SYSTEM_MEMORY;

#if !__FINAL
	// Exception Handling
	SPURS_CHECK(cellSpursEnableExceptionEventHandler(g_Spurs,true));
	SPURS_CHECK(cellSpursSetGlobalExceptionEventHandler(g_Spurs,GlobalExceptionHandler,NULL));
	
#if !SYSTEM_WORKLOAD
	SPURS_CHECK(cellSpursEnableExceptionEventHandler(g_Spurs2,true));
	SPURS_CHECK(cellSpursSetGlobalExceptionEventHandler(g_Spurs2,GlobalExceptionHandler,NULL));
#endif
#endif

	sys_event_flag_attribute_t attr;
	sys_event_flag_attribute_initialize(attr);
	for (int i=0; i<(TASK_MAX_TASKS_SPU+63)/64; i++)
		SPURS_CHECK(sys_event_flag_create(&s_CompletionFlags[i],&attr,0));
	CHECKPOINT_SYSTEM_MEMORY;
}

void sysTaskManager__SysInitClass2()
{
	// Spurs handler must have higher priority than the main thread 
	// (and the completion wait thread)
	sys_ppu_thread_t idMainPpuThread;
	int prioMainPpuThread;
	SPURS_CHECK(sys_ppu_thread_get_id(&idMainPpuThread));
	SPURS_CHECK(sys_ppu_thread_get_priority(idMainPpuThread, &prioMainPpuThread));

#if !__FINAL
	int spursHandlerPPUPriority = prioMainPpuThread - 32;

	// Printf server 
	SPURS_CHECK(s_spursPrintf.spurs_printf_service_initialize(g_Spurs, spursHandlerPPUPriority, "[RAGE] Primary SPU Printf"));
#if !SYSTEM_WORKLOAD
	SPURS_CHECK(s_spursPrintf2.spurs_printf_service_initialize(g_Spurs2, spursHandlerPPUPriority, "[RAGE] Secondary SPU Printf"));
#endif
#endif
}

void sysTaskManager::InitClass()
{
#if __ASSERT
	static bool inited;
	Assert(!inited);
	inited = true;
#endif
	CHECKPOINT_SYSTEM_MEMORY;

	for(int i = 0; i < sysTaskManager::NUM_SCHEDULES; i++)
	{
#if !SYSTEM_WORKLOAD
		CellSpurs* spursInst = jobData[i].m_SpursInstance == 0 ? g_Spurs : g_Spurs2;
#else
		CellSpurs* spursInst = g_Spurs;
#endif
		s_JobChains[i].Init(spursInst, jobData[i].m_SpuPriorities, jobData[i].m_MaxContention, jobData[i].m_Name);
	}
}

void sysTaskManager::ShutdownClass()
{
	for(int i = sysTaskManager::NUM_SCHEDULES-1; i >= 0; i--)
		s_JobChains[i].Shutdown();

#if !__FINAL
#if !SYSTEM_WORKLOAD
	SPURS_CHECK(s_spursPrintf2.spurs_printf_service_finalize(g_Spurs2));
#endif
	SPURS_CHECK(s_spursPrintf.spurs_printf_service_finalize(g_Spurs));
#endif

#if !SYSTEM_WORKLOAD
	SPURS_CHECK(cellSpursFinalize(g_Spurs2));
	g_Spurs2 = NULL;
#endif
	SPURS_CHECK(cellSpursFinalize(g_Spurs));
	g_Spurs = NULL;
}

#if RAGETRACE
static uint32_t default_IdFromName(const char * /*name*/)
{
	return 0;
}

uint32_t (*g_pIdFromName)(const char*) = default_IdFromName;
#endif

__sysTaskHandle* sysTaskManager::AllocateTaskHandle()
{
	u32 index = s_FreeList.Pop();
	if (Unlikely(index == FreeListType::INVALID))
	{
#if __ASSERT
		PrintJobs();
		AssertMsg(false,"Out of jobs.  Calling code might not handle this well.");
#endif
		return NULL;
	}
 
	__sysTaskHandle *h = &s_Handles[index];
	__dcbz(h); // fast 128 byte aligned block clear
	__dcbz(((u8*)h)+128);
	// DEBUG_TASK_COMPLETION_ONLY(s_TaskLog.Log('ALOC',index,(u32)h->Semaphore,0));
	return h;
}

#if DEBUG_TASK_COMPLETION
static void LogBits(u32 id,u32 index,u64 bits)
{
	u64 accum = 0;
	u32 count = 0;
	u32 base = index & ~63U;
	for (u32 i=0; i<64; i++) {
		if ((1ULL << i) & bits) {
			accum |= u64(base + i) << count;
			count += 16;
			if (count == 64) {
				s_TaskLog.Log(id,index,u32(accum>>32),u32(accum));
				accum = 0;
				count = 0;
			}
		}
	}
	if (count)
		s_TaskLog.Log(id,index,u32(accum>>32),u32(accum));
}
#endif


static inline void ReadyHeader(__sysTaskHandle *handle)
{
	uint32_t index = handle - s_Handles;
	uint32_t eventIndex = index >> 6;
	sys_event_flag_t completion = s_CompletionFlags[eventIndex];
	handle->Semaphore = ((uint64_t)completion << 32) | index;
	DEBUG_TASK_COMPLETION_ONLY(s_TaskLog.Log('REDY',index,0,0));
}

sysTaskHandle sysTaskManager::Setup(TASK_INTERFACE_PARAMS,const sysTaskParameters &p,int ASSERT_ONLY(schedulerIndex))
{
	AssertMsg(g_Spurs, "You forgot to call sysTaskManager::InitClass");

	AssertMsg(p.Input.Data,"Input.Data cannot be NULL");
	Assert(p.ReadOnlyCount <= TASK_MAX_READ_ONLY);
	Assert(p.UserDataCount <= TASK_MAX_USER_DATA);

	AssertMsg((p.Input.Size & 15) == 0,"Input.Size not multiple of 16");
	AssertMsg((p.Output.Size & 15) == 0,"Output.Size not multiple of 16");
	AssertMsg((p.Scratch.Size & 15) == 0,"Scratch.Size not multiple of 16");

	AssertMsg((reinterpret_cast<unsigned int>(p.Input.Data) & 15) == 0,"Input.Data not aligned to 16byte boundary");
	AssertMsg((reinterpret_cast<unsigned int>(p.Output.Data) & 15) == 0,"Output.Data not aligned to 16byte boundary");

	AssertMsg(!p.Input.Size  || !sysIpcIsStackAddress(p.Input.Data),  "Cannot have input data on stack");
	AssertMsg(!p.Output.Size || !sysIpcIsStackAddress(p.Output.Data), "Cannot have output data on stack");

	size_t stackSize = p.SpuStackSize? p.SpuStackSize : 8192;
	size_t taskUsed = p.Input.Size + p.Scratch.Size + (size_t)taskSize + stackSize;

	for(u32 i=0; i<p.ReadOnlyCount; ++i)
	{
		taskUsed += p.ReadOnly[i].Size;
		AssertMsg((p.ReadOnly[i].Size & 15) == 0,"Readonly data size not multiple of 16");
		AssertMsg((reinterpret_cast<unsigned int>(p.ReadOnly[i].Data) & 15) == 0,"Readonly address not on 16byte boundary");
		AssertMsg(!sysIpcIsStackAddress(p.ReadOnly[i].Data), "Cannot have readonly data on stack");
	}

	if (p.Output.Data != p.Input.Data)
		taskUsed += p.Output.Size;

	// c:\usr\local\cell\SDK_doc\en\chm\PS3_SDKDoc_e.chm  says we have 234k user space
	// Alastair reports he's had problems with going over 231k, so let's round it off.

	if (taskUsed > kMaxSPUJobSize)
	{
		// fail like a complete bastard 
		Quitf("Task code and data too large (%uk total, limit is %uk)",taskUsed>>10,kMaxSPUJobSize/1024);
		return NULL; 
	}

	Assert((schedulerIndex>=0)&&(schedulerIndex<NUM_SCHEDULES));
	__sysTaskHandle *h = AllocateTaskHandle();
	if (!h)
		return h;

	ASSERT_ONLY(const int maxUserDataCount = static_cast<int>(sizeof(h->MainJob.workArea.userData) / sizeof(int)) - 2);

	//// Create job for the main job, setting up input and output DMA's
	CellSpursJobHeader &jh = h->MainJob.header;
	jh.eaBinary = (uintptr_t) taskStart;
	jh.sizeBinary = CELL_SPURS_GET_SIZE_BINARY(taskSize);
	
	bool inOut = p.Input.Data == p.Output.Data;
	Assert(!inOut || p.Input.Size == p.Output.Size);

	// Add extra space at the end of whatever is used as output, for the fenced "dones" DMA
	// Warning: must match task_psn.h
	unsigned int sizeDones = 16;
    unsigned int sizeMarker = TASK_SPU_EXTRA_CHECKS?16:0;	// overflow marker
	jh.useInOutBuffer = inOut;
	jh.sizeInOrInOut = p.Input.Size + sizeMarker + (inOut? sizeDones: 0);
	jh.sizeOut = inOut? 0 : (p.Output.Size + sizeDones + sizeMarker);
	jh.sizeScratch = (p.Scratch.Size + sizeMarker) >> 4;

	// Convert the default of zero ourselves to guarantee it never changes on us unexpectedly.
	jh.sizeStack = (stackSize >> 4);
	TASK_PSN_DEBUG_CAPTURE_HANDLE_STACKS_ONLY(sysStack::CaptureStackTrace(s_HandleStacks[h-s_Handles], NELEM(s_HandleStacks[0]), 1));

	// The first word is a DMA descriptor for the input
	// Input larger than 16k must be broken up!
	int inputIndex = 0;
	size_t remaining = p.Input.Size;
	void *inputData = p.Input.Data;
	Assert(inputIndex + static_cast<int>((p.Input.Size + 16384-1) / 16384) <= maxUserDataCount);
	while (remaining) {
		size_t thisChunk = remaining < 16384? remaining : 16384;
		h->MainJob.workArea.userData[inputIndex++] = sysDmaListMakeEntry(inputData,thisChunk);
		inputData = (void*)((char*)inputData + thisChunk);
		remaining -= thisChunk;
	}
	Assert(inputIndex < userSize);
	jh.sizeDmaList  = inputIndex * sizeof(uint64_t);	// Only the first word is part of a DMA list.

	// Next are any read-only DMA's.  These are done via software and therefore
	// are not subject to the normal 16k limit.
	int readonlyIndex = inputIndex;
	Assert(readonlyIndex + static_cast<int>(p.ReadOnlyCount) <= maxUserDataCount);
	for (size_t i=0; i<p.ReadOnlyCount; i++)
		h->MainJob.workArea.userData[readonlyIndex++] = sysDmaListMakeLargeEntry(p.ReadOnly[i].Data, p.ReadOnly[i].Size);
	jh.sizeCacheDmaList = (readonlyIndex - inputIndex) * sizeof(uint64_t);
	
	// Next is a DMA descriptor for the output (used by the SPU)
	// This doesn't have a 16k limit either because we initiate the DMA ourselves.
	int outputIndex = readonlyIndex;
	if (!inOut) {
		Assert(outputIndex + 1 <= maxUserDataCount);
		h->MainJob.workArea.userData[outputIndex++] = sysDmaListMakeLargeEntry(p.Output.Data, p.Output.Size);
		Assert(outputIndex < userSize);
	}

#if RAGETRACE
	// Store information for trace
	h->MainJob.workArea.userData[userSize-2] = 
			g_pIdFromName(taskName) |
		(((uint64_t)g_pfSpuTrace)<<32);
#else
	// Record PPU job start time.
	h->MainJob.workArea.userData[userSize-2] = ((uint64_t)sysTimer::GetTicks() << 32) | (uint32_t)taskName;
#endif
	Assert(outputIndex < userSize-1);	// use last byte as the job completion flag.
	ReadyHeader(h);

	// Finally any user data is copied in (32 bits per entry, so we use half of a userData slot for each one)
	int userDataIndex = outputIndex;
	Assert(userDataIndex + static_cast<int>(p.UserDataCount) <= maxUserDataCount);
	int *dataPtr = (int*) &h->MainJob.workArea.userData[userDataIndex];
	*dataPtr = p.UserDataCount;
	for (size_t i=0; i<p.UserDataCount; i++)
		dataPtr[i+1] = p.UserData[i].asInt;

	return h;
}

sysTaskHandle sysTaskManager::Create(TASK_INTERFACE_PARAMS,const sysTaskParameters &p,int schedulerIndex)
{
	sysTaskHandle h = Setup(TASK_INTERFACE_PARAMS_PASS,p,schedulerIndex);
	JobChain &jc = s_JobChains[schedulerIndex];
	if (h)
		jc.AddJob(&h->MainJob);
	return AddSalt(h);
}

sysPreparedTaskHandle sysTaskManager::Prepare(TASK_INTERFACE_PARAMS,const sysTaskParameters &p,int schedulerIndex)
{
	sysTaskHandle h = Setup(TASK_INTERFACE_PARAMS_PASS,p,schedulerIndex);
	
	// setup preparedTaskHandle so can be executed on a spu
	sysPreparedTaskHandle prepTaskHandle;
	JobChain &jc = s_JobChains[schedulerIndex];
	prepTaskHandle.JobChainCommandListEAddress = (void*)jc.m_CommandList;
#if TASK_ALLOW_SPU_ADD_JOB
	prepTaskHandle.JobChainCommandListLockEAddress = (void*)&jc.m_JobChainSyncMutex;
#endif
	prepTaskHandle.JobChainEAddress = (void*)&jc.m_Chain;;
	prepTaskHandle.JobChainCommandListIndexEAddress = (void*)&jc.m_CommandIndex;
	prepTaskHandle.jobToAdd = &h->MainJob;
	prepTaskHandle.hdle = AddSalt(h);

	return prepTaskHandle;
}

sysTaskHandle sysTaskManager::Dispatch( const sysPreparedTaskHandle& handle, int schedulerIndex)
{	
	JobChain &jc = s_JobChains[schedulerIndex];
	sysTaskHandle h = (sysTaskHandle)(size_t(handle.hdle) & ~255);		// Strip the salt off
	if ( h )
		jc.AddJob(&h->MainJob);
	return handle.hdle;									// Return the original handle again with salt still there
}
void sysTaskManager::CreateTaskset(CellSpursTaskset* taskset, uint64_t argTaskset, const uint8_t priority[8], unsigned int max_contention, const char *staticName)
{
	CellSpursTasksetAttribute attributeTaskset;
	SPURS_CHECK(cellSpursTasksetAttributeInitialize(&attributeTaskset, argTaskset, priority, max_contention));
#if __ASSERT && CELL_SDK_VERSION >= 0x250000 // Only enable LS clear on debug and beta builds
	SPURS_CHECK(cellSpursTasksetAttributeEnableClearLS(&attributeTaskset, 1));
#endif
	SPURS_CHECK(cellSpursTasksetAttributeSetName(&attributeTaskset, staticName));
	SPURS_CHECK(cellSpursCreateTasksetWithAttribute(
		g_Spurs, 
		taskset, 
		&attributeTaskset));

#if !__FINAL
	SPURS_CHECK(cellSpursTasksetSetExceptionEventHandler(taskset, TaskSetExceptionHandler, 0));
#endif
}

void sysTaskManager::SetExceptionHandler(CellSpursJobChain* NOTFINAL_ONLY(jobChain))
{
#if !__FINAL
	SPURS_CHECK(cellSpursJobChainSetExceptionEventHandler(jobChain, JobChainExceptionHandler, 0));
#endif
}

#if !__FINAL
const char* sysTaskManager::GetTaskName(sysTaskHandle handle)
{
#if RAGETRACE
    return ((pfTraceCounterId*)((u32)handle->MainJob.workArea.userData[userSize-2]))->m_Name;
#else
    return (const char*)(u32)handle->MainJob.workArea.userData[userSize-2];
#endif
}
#endif	// !__FINAL

#define Done MainJob.workArea.userData[userSize-1]
#define StartTime MainJob.workArea.userData[userSize-2]

PARAM(jobtimings,"[task] Spew job timings");


NOTFINAL_ONLY(static sysIpcCurrentThreadId s_LockThreads[MaxTasks]);

bool sysTaskManager::Poll(sysTaskHandle handle)
{
	Assert(handle);
	RemoveSalt(handle);
	NOTFINAL_ONLY(if (!handle->MainJob.header.sizeStack) Quitf("Waiting on a stale handle"));

	// Quick reject if semaphore hasn't been cleared by job code yet.
	if (handle->Semaphore)
		return false;

	uint32_t index = handle - s_Handles;
	uint32_t eventIndex = index >> 6;
	uint64_t bit = 1ULL << (index & 63);
#if !__FINAL
	if (Unlikely(s_LockThreads[index]))
	{
		// Rather than a Assertf, crash so we get exception handler output (and resulting coredump).
		// See B*792608.
		Errorf("Thread %x was already waiting on this handle when we tried to poll!", s_LockThreads[index]);
		__debugbreak();
	}
#endif

	sys_event_flag_t completion = s_CompletionFlags[eventIndex];
	SPURS_CHECK(sys_event_flag_wait(completion,bit,SYS_EVENT_FLAG_WAIT_AND | SYS_EVENT_FLAG_WAIT_CLEAR,NULL,SYS_NO_TIMEOUT));

	NOTFINAL_ONLY(handle->MainJob.header.sizeStack = 0);
	TASK_PSN_DEBUG_CAPTURE_HANDLE_STACKS_ONLY(sysStack::CaptureStackTrace(s_HandleStacks[index], NELEM(s_HandleStacks[0]), 1));

	s_FreeList.Push(index);
	return true;
}

__THREAD s64 g_TaskWaitTicks;
__THREAD u32 g_TaskTimeout;

void sysTaskManager::Wait(sysTaskHandle handle)
{
	utimer_t now = sysTimer::GetTicks();
	Assert(handle);
	RemoveSalt(handle);
	uint32_t index = handle - s_Handles;

#if !__FINAL
	if (Unlikely(!handle->MainJob.header.sizeStack))
	{
		// Rather than a Quitf, crash so we get exception handler output (and resulting coredump).
		// See B*792608.
		Errorf("Waiting on a stale handle (index=0x%08x).  Please attach coredump to bug.", index);
		__debugbreak();
	}
#endif

#if !__FINAL
	sysIpcCurrentThreadId prevLocker = (sysIpcCurrentThreadId)cellAtomicStore32((uint32_t*)&s_LockThreads[index],(uint32_t)g_CurrentThreadId);
	if (Unlikely(prevLocker))
	{
		// Rather than a Assertf, crash so we get exception handler output (and resulting coredump).
		// See B*792608.
		Errorf("Thread 0x%x was already waiting on this handle!  Please attach coredump to bug.", prevLocker);
		__debugbreak();
	}
#endif

	uint32_t eventIndex = index >> 6;
	sys_event_flag_t completion = s_CompletionFlags[eventIndex];
	uint64_t bit = 1ULL << (index & 63);
	SPURS_CHECK(sys_event_flag_wait(completion,bit,SYS_EVENT_FLAG_WAIT_AND | SYS_EVENT_FLAG_WAIT_CLEAR,NULL,SYS_NO_TIMEOUT));

	// We may get the event before the SPU triggered the write so wait for that too.
	while (handle->Semaphore) 
		__db16cyc();

	NOTFINAL_ONLY(s_LockThreads[index] = 0);
	NOTFINAL_ONLY(handle->MainJob.header.sizeStack = 0);
	TASK_PSN_DEBUG_CAPTURE_HANDLE_STACKS_ONLY(sysStack::CaptureStackTrace(s_HandleStacks[index], NELEM(s_HandleStacks[0]), 1));
	s_FreeList.Push(index);
	g_TaskWaitTicks += sysTimer::GetTicks() - now;
}


void sysTaskManager::WaitMultiple(int count,const sysTaskHandle *handles)
{
	for (int i=0; i<count; i++)
		if (handles[i])
			Wait(handles[i]);
}

int sysTaskManager::GetMaxTasks()
{
	return MaxTasks;
}


int sysTaskManager::GetNumThreads()
{
	return NUM_SPURS_THREADS_PRIMARY;
}

#if __BANK
void sysTaskManager::PrintJobs()
{
	Displayf("***********************************************************************");
	Displayf("Job Information");
	Displayf("***********************************************************************");

	atMap<const char*, u32> jobs;
	for(int i=0; i<MaxTasks; ++i)
	{ 
		const char* name = sysTaskManager::GetTaskName(&s_Handles[i]);
		// In __ASSERT builds, sizeStack is nonzero if the job hasn't been Wait'd or Poll'd on yet.
		// Don't bother remembering ones that we know finished properly.
		if (s_Handles[i].MainJob.header.sizeStack)
			++jobs[name];
	}

	for(atMap<const char*, u32>::Iterator it = jobs.CreateIterator(); it; ++it)
		Displayf(" %s - %i pending jobs", it.GetKey(), it.GetData());

	Displayf("***********************************************************************");
}

void sysTaskManager::AddWidgets(bkGroup &bank)
{
	bkGroup *masterGroup = bank.AddGroup("SPU Task Manager");

	masterGroup->AddButton("Print Jobs", &sysTaskManager::PrintJobs);

	for(int i = 0; i < NUM_SCHEDULES; i++)
	{
		JobChainData &chain = jobData[i];

		bkGroup *spuGroup = masterGroup->AddGroup(chain.m_Name);

		for (int x=0; x<6 /*CELL_SPURS_MAX_SPU*/; x++)
		{
			char title[32];
			formatf(title, "SPU%d Priority", x);

			spuGroup->AddSlider(title, &jobData[i].m_SpuPriorities[x], 0, 15, 1.0f, datCallback(CFA1(sysTaskManager::UpdatePriorities), (void *) i));
		}
	}
}

void sysTaskManager::UpdatePriorities(void *schedulerIndexPtr)
{
	int schedulerIndex = (int) schedulerIndexPtr;

	FastAssert((u32) schedulerIndex < (u32) NUM_SCHEDULES);

#if !SYSTEM_WORKLOAD
	CellSpurs* spursInst = jobData[schedulerIndex].m_SpursInstance == 0 ? g_Spurs : g_Spurs2;
#else
	CellSpurs* spursInst = g_Spurs;
#endif

	s_JobChains[schedulerIndex].UpdatePriorities(spursInst, jobData[schedulerIndex].m_SpuPriorities);
}

#endif // __BANK





// matts - new style task creation:

sysTaskContext::sysTaskContext(TASK_INTERFACE_PARAMS, u32 outputsize, u32 scratchsize, u32 stacksize)
{
	AssertMsg(g_Spurs, "You forgot to call sysTaskManager::InitClass");
	AssertMsg((outputsize & 15) == 0, "output size not multiple of 16");
	AssertMsg((scratchsize & 15) == 0, "scratch size not multiple of 16");

	Init();
	if (!IsValid())
		return;
	ASSERT_ONLY(m_UserData = false;)
	ASSERT_ONLY(m_AddedOutput = false;)
	ASSERT_ONLY(m_DontAssertOnErrors = false;)
	m_OutputIndex = 0;
	m_CacheableSize = 0;	

	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	jh.eaBinary = (uintptr_t) taskStart;
	jh.sizeBinary = CELL_SPURS_GET_SIZE_BINARY(taskSize);
	jh.sizeOut = outputsize;
	jh.sizeStack = stacksize >> 4;
	jh.sizeScratch = scratchsize >> 4;

#if RAGETRACE
	// Store information for trace
	m_Handle->MainJob.workArea.userData[userSize-2] = 
		g_pIdFromName(taskName) |
		(((uint64_t)g_pfSpuTrace)<<32);
#else
	// Record PPU job start time.
	m_Handle->MainJob.workArea.userData[userSize-2] = 
		((uint64_t)sysTimer::GetTicks() << 32) | (uint32_t)taskName;
#endif
	ReadyHeader(m_Handle);
}

sysTaskContext::~sysTaskContext()
{
	AssertMsg(!m_Handle || m_Started || m_DontAssertOnErrors, "Task was never started!");
}

void sysTaskContext::Init()
{
	AssertMsg(g_Spurs, "You forgot to call sysTaskManager::InitClass");
	m_Handle = sysTaskManager::AllocateTaskHandle();
	ASSERT_ONLY(m_Started = 0;)
}

__forceinline void copy128b(void* __restrict pDst, const void* __restrict pSrc)
{
#ifdef __SNC__
	sysMemCpy(pDst,pSrc,128);
#else
	__vector4 v0,v1,v2,v3,v4,v5,v6,v7;
	asm volatile(
		 "lvx  %0, %8,%12; lvx  %1, %8,%13; lvx  %2, %8,%14; lvx  %3, %8,%15; "
		 "lvx  %4, %9,%12; lvx  %5, %9,%13; lvx  %6, %9,%14; lvx  %7, %9,%15; "
		 "stvx %0,%10,%12; stvx %1,%10,%13; stvx %2,%10,%14; stvx %3,%10,%15; "
		 "stvx %4,%11,%12; stvx %5,%11,%13; stvx %6,%11,%14; stvx %7,%11,%15"
		: "=&v"(v0), "=&v"(v1), "=&v"(v2), "=&v"(v3), 
		  "=&v"(v4), "=&v"(v5), "=&v"(v6), "=&v"(v7)
		: "b" (pSrc), "b"((u8*)pSrc+64), "b" (pDst), "b"((u8*)pDst+64),
		  "r"(0), "r"(0x10), "r"(0x20), "r"(0x30)
		: "memory");
#endif
}

sysTaskContext::sysTaskContext(const sysTaskContext& rhs)
{
	Init();
	if (IsValid())
		*this = rhs;
}

__attribute__((noinline))
sysTaskContext& sysTaskContext::operator = (const sysTaskContext& rhs)
{
	Assert(IsValid());
	Assert(!m_Started);
 	copy128b(&m_Handle->MainJob, &rhs.m_Handle->MainJob);
 	copy128b(((u8*)&m_Handle->MainJob) + 128, ((u8*)&rhs.m_Handle->MainJob) + 128);
	m_CacheableSize = rhs.m_CacheableSize;
	m_OutputIndex = rhs.m_OutputIndex;
	ASSERT_ONLY(m_UserData = rhs.m_UserData;)
	ASSERT_ONLY(m_AddedOutput = rhs.m_AddedOutput;)
	return *this;
}

void sysTaskContext::SetInputOutput()
{
	Assert(IsValid());	
	Assert(!m_Started);
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	AssertMsg(jh.sizeOut == 0, "Cannot have an output size when using a combined input/output buffer");
	jh.useInOutBuffer = 1;
}

bool sysTaskContext::IsValid() const
{
	return m_Handle != 0;
}

void sysTaskContext::AddInput(const void* pSrc, size_t size)
{
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	Assert(IsValid());
	Assert(!m_Started);
	AssertMsg(jh.sizeCacheDmaList == 0 && !m_UserData, "add inputs before cache dma or user data");
	AssertMsg(!m_AddedOutput, "add inputs before outputs when using I/O buffer");
	AssertMsg((reinterpret_cast<u32>(pSrc) & 15) == 0, "input data not aligned to 16 byte boundary");
	AssertMsg((size & 15) == 0, "input data size not a multiple of 16 bytes");
	AssertMsg(!sysIpcIsStackAddress(pSrc), "cannot have input data on stack");
	jh.sizeInOrInOut += size;
	u64* dma = (u64*)((u8*)&m_Handle->MainJob.workArea + m_OutputIndex);
	while (size) {
		size_t thisChunk = size < 16384 ? size : 16384;
		*dma++ = sysDmaListMakeEntry(pSrc, thisChunk);
		pSrc = (void*)((char*)pSrc + thisChunk);
		size -= thisChunk;
		jh.sizeDmaList += 8;
		m_OutputIndex += 8;
	}
	AssertMsg(m_OutputIndex <= (userSize-2)*8, "dma list too big");
#if !__FINAL
	if(m_OutputIndex > (userSize-2)*8)
		Quitf("DMA list is too big, you're going to trash some memory so we'll just abort");
#endif // !__FINAL
}

void sysTaskContext::AddOutput(size_t size)
{
	Assert(IsValid());
	Assert(!m_Started);
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	if (jh.useInOutBuffer)
	{
		jh.sizeInOrInOut += size;
		ASSERT_ONLY(m_AddedOutput = true;)
	}
	else
	{
		jh.sizeOut += size;	
	}
}

void sysTaskContext::AddCacheable(void* pSrc, u32 size)
{
	Assert(IsValid());
	Assert(!m_Started);
	AssertMsg(!m_UserData, "add read only data before user data");
	AssertMsg((size & 15) == 0, "Cacheable data size not multiple of 16");
	AssertMsg((reinterpret_cast<unsigned int>(pSrc) & 15) == 0,"Cacheable address not on 16byte boundary");
	AssertMsg(!sysIpcIsStackAddress(pSrc), "cannot have cacheable data on stack");

	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	u64* dma = (u64*)((u8*)&m_Handle->MainJob.workArea + m_OutputIndex);
	*dma = sysDmaListMakeLargeEntry(pSrc, size);
	jh.sizeCacheDmaList += 8;
	m_OutputIndex += 8;
	m_CacheableSize += size;
	AssertMsg(m_OutputIndex <= (userSize-2)*8, "dma list too big");
}

void* sysTaskContext::AllocUserData(u32 bytes, u32 align)
{
	Assert(IsValid());
	Assert(!m_Started);
	u16 newOutputIndex = (m_OutputIndex + align - 1) & ~(align - 1);

	// make sure we have space
	if((newOutputIndex+bytes) <= (userSize-2)*8)
	{
		m_OutputIndex = newOutputIndex;
		void* pData = (u8*)&m_Handle->MainJob.workArea + m_OutputIndex;
		ASSERT_ONLY(m_UserData = true;)
		m_OutputIndex += bytes;
		return pData;
	}
	Assertf(m_DontAssertOnErrors, "Too much user data! Can't allocate any more space for this job. DMA list size (%d) + userdata size (%d) needs to be <= %d", newOutputIndex, bytes, (userSize-2)*8);
	return NULL;
}

sysTaskHandle sysTaskContext::TryStartInternal(int schedulerIndex, bool ASSERT_ONLY(canAssert))
{
	Assert(IsValid());
	Assert(!m_Started);

	u32 taskUsed = GetTotalSize();

	AddOutput(16); // reserve space for outputting dones marker 

# if TASK_SPU_EXTRA_CHECKS 
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	// add space for markers used by TASK_SPU_EXTRA_CHECKS
	if (jh.sizeInOrInOut)
		jh.sizeInOrInOut += 16;
	if (jh.sizeOut && !jh.useInOutBuffer)
		jh.sizeOut += 16;
	if (jh.sizeScratch)
		jh.sizeScratch += 1;
# endif // TASK_SPU_EXTRA_CHECKS

	if (taskUsed <= kMaxSPUJobSize)
	{
		ASSERT_ONLY(m_Started = true;)
		Assert(m_Handle->Semaphore);
		JobChain &jc = s_JobChains[schedulerIndex];
		jc.AddJob(&m_Handle->MainJob);
		return AddSalt(m_Handle);
	}
	else
	{
#		if __ASSERT
			if (canAssert)
			{
				Assertf(0, "Task code and data too large (%uk total, limit is %uk)",
					taskUsed/1024, kMaxSPUJobSize/1024);
			}
#		endif
		Cancel();
		return NULL;
	}
}

void sysTaskContext::Cancel()
{
	Assertf(!m_Started && m_Handle, "Task already started or cancelled");
	int index = m_Handle - s_Handles;
	m_Handle = 0;
	s_FreeList.Push(index);
}

int sysTaskContext::GetInputSize() const
{
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	return jh.sizeInOrInOut;
}

int sysTaskContext::GetOutputSize() const
{
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	return jh.useInOutBuffer ? jh.sizeInOrInOut : jh.sizeOut;
}

int sysTaskContext::GetScratchSize() const
{
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	return jh.sizeScratch << 4;
}

int sysTaskContext::GetCodeSize() const
{
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	return jh.sizeBinary << 4;	
}

int sysTaskContext::GetCacheableSize() const
{
	return m_CacheableSize;
}

int sysTaskContext::GetStackSize() const
{
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	return jh.sizeStack << 4;
}

int sysTaskContext::GetTotalSize() const
{
	CellSpursJobHeader& jh = m_Handle->MainJob.header;
	int size = m_CacheableSize + (jh.sizeBinary<<4) + jh.sizeInOrInOut + 
		jh.sizeOut + (jh.sizeScratch<<4) + (jh.sizeStack<<4) + 16;
# if TASK_SPU_EXTRA_CHECKS 
	if (jh.sizeInOrInOut)
		size += 16;
	if (jh.sizeOut && !jh.useInOutBuffer)
		size += 16;
	if (jh.sizeScratch)
		size += 16;
#endif
	return size;
}

int sysTaskContext::GetFreeSize() const
{
	return kMaxSPUJobSize - GetTotalSize();
}

bool sysTaskContext::FitsOnSpu() const
{
	return GetTotalSize() <= kMaxSPUJobSize;
}

}	// namespace rage

#endif	// __PPU
