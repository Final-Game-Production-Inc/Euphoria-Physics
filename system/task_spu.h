// 
// system/task_spu.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#if __SPU

#include "system/taskheader.h"
#include "profile/cellspurstrace.h"
#include "system/dma.h"
#include "system/task_spu_config.h"
#include "system/tasklog.h"
#include "system/ppu_symbol.h"
#include "system/debugmemoryfill.h"

#include <cell/spurs/job_chain.h>
#include <cell/spurs/common.h>
#include <cell/sync2/semaphore.h>
#include <sys/event_flag.h>
#include <cell/dma.h>
#include <spu_printf.h>

#define TASK_SPU_DEBUG_SPEW (0 && __DEV)

#if DEBUG_TASK_COMPLETION
DECLARE_PPU_SYMBOL(rage::sysTaskLog,s_TaskLog);
#endif

#ifndef PASS_SPURS_ARGS
#define PASS_SPURS_ARGS (0)
#endif // PASS_SPURS_ARGS

// #define ENTRYPOINT to the name of the function before including this file.
// cellSpursJobMain must be at offset zero, so include this file first.
#ifdef TASK_SPU_2
#if PASS_SPURS_ARGS
void ENTRYPOINT(::rage::sysTaskContext &p, CellSpursJobContext2* jobContext, CellSpursJob256 *job);
#else
void ENTRYPOINT(::rage::sysTaskContext& c);
#endif // PASS_SPURS_ARGS
#else
#if PASS_SPURS_ARGS
void ENTRYPOINT(::rage::sysTaskParameters &p, CellSpursJobContext2* jobContext, CellSpursJob256 *job);
#else
void ENTRYPOINT(::rage::sysTaskParameters &p);
#endif // PASS_SPURS_ARGS
#endif // TASK_SPU_2

namespace rage {
RAGETRACE_ONLY(pfSpuTrace* g_pfSpuTrace = 0;)
} // namespace rage

#if !__FINAL
#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)

extern "C" const qword __SPU_GUID[1];

inline void checkBufferEndMarker(vector signed char* bufferEnd, const vector signed char kMarker, const char* bufName, const char* tag)
{
    if (__builtin_expect(si_to_uint(si_gbb(si_ceqb(*bufferEnd,kMarker)))!=0xFFFF, false)) {
		rage::u64 guid;
		int guidResult = cellSpursGetSpuGuid(__SPU_GUID, &guid);
		if (guidResult != 0)
		{
			guid = (rage::u64)-1;
		}
		rage::u32 *pGuid = (rage::u32*)&guid;

		spu_printf("%s: At %s, %s buffer corrupted! guid %.8x%.8x\n", QUOTE(ENTRYPOINT), tag, bufName, pGuid[0], pGuid[1]); 
        __debugbreak();  
    }
}
#else //__FINAL
inline void checkBufferEndMarker(vector signed char* , const vector signed char , const char*)
{	
}
#endif//__FINAL


#if TASK_SPU_EXTRA_CHECKS
vector signed char g_savedLs0 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
const vector signed char kMarker = {0xDE, 0xAD, 0xC0, 0xDE, 0xFF, 0xFF, 0xFF, 0xFF, 0xDE, 0xAD, 0xC0, 0xDE, 0xFF, 0xFF, 0xFF, 0xFF};	
vector signed char* sBufferEnd = 0;
vector signed char* ioBufferEnd = 0;
vector signed char* oBufferEnd = 0;

void checkSpursMarkers(char* tag="")
{
	// Check markers 
	// - if they're gone, the job wrote past the end of its buffers
	if (sBufferEnd) {
		checkBufferEndMarker(sBufferEnd, kMarker, "scratch", tag);
	}
	if (ioBufferEnd) {
		checkBufferEndMarker(ioBufferEnd, kMarker, "input/output", tag);
	}
	if (oBufferEnd) {
		checkBufferEndMarker(oBufferEnd, kMarker, "output", tag);
	}

	// Checks that nobody modified the contents of address 0
	// - if an assertion trips here, you're likely to have deferenced a NULL pointer
	// - there are legitimate uses of LS0 (interrupts) but you're probably not using this
	//   and even in that case you're meant to restore it!

	const vector signed char curLs0 = *((vector signed char*) 0);
	if (__builtin_expect(si_to_uint(si_gbb(si_ceqb(curLs0,g_savedLs0)))!=0xFFFF, false)) {
		rage::u64 guid;
		int guidResult = cellSpursGetSpuGuid(__SPU_GUID, &guid);
		if (guidResult != 0)
		{
			guid = (rage::u64)-1;
		}
		rage::u32 *pGuid = (rage::u32*)&guid;
		spu_printf("%s, At %s, LS0 corrupted! guid %.8x%.8x\n", QUOTE(ENTRYPOINT), tag, pGuid[0], pGuid[1]); 
		__debugbreak();  
	}
}
#else
inline void checkSpursMarkers(char*)
{}
#endif

#if SUPPORT_DEBUG_MEMORY_FILL
DECLARE_PPU_SYMBOL(unsigned char,g_EnableDebugMemoryFill);
unsigned char g_EnableDebugMemoryFill = 0xFF;
#endif

extern "C"
void cellSpursJobMain2(CellSpursJobContext2* jobContext, CellSpursJob256 *job)
{
#if SYS_DMA_TIMING
	rage::g_DmaWaitTime = 0;
#endif

#if SUPPORT_DEBUG_MEMORY_FILL
	// We could extend this to 16 bytes of "shared" debug variables pretty easily.
	// Cannot use rage version outside of namespace rage

	// If we are using reloaded spu jobs they won't have the PPU_SYMBOLS resolved.
	if ((uint64_t)PPU_SYMBOL(g_EnableDebugMemoryFill))
	{
		g_EnableDebugMemoryFill = cellDmaGetUint8((uint64_t)PPU_SYMBOL(g_EnableDebugMemoryFill),jobContext->dmaTag,0,0);
	}
#endif

#if DEBUG_TASK_COMPLETION // If we are using reloaded spu jobs they won't have the PPU_SYMBOLS resolved.
	if (PPU_SYMBOL(s_TaskLog))
	{
		DEBUG_TASK_COMPLETION_ONLY(PPU_SYMBOL(s_TaskLog)->Log('STRT',(uint32_t)job->workArea.userData[25],0,0));
	}
#endif
#if !__FINAL
	// Under 270+ at least, there is some sort of stub at address zero that has
	// a branch-to-self at the end.  Let's prefer a faulting instruction (stopd)
	// instead so it's obvious when not attached via a debugger.
	*(int*)0 = 0x3DCD;
#endif	

    rage::sysLocalStoreInit(job->header.sizeStack<<4);
    rage::sysLocalStoreSetWritable(jobContext->ioBuffer, job->header.sizeInOrInOut);
    rage::sysLocalStoreSetWritable(jobContext->oBuffer, job->header.useInOutBuffer ? 0 : job->header.sizeOut);
    rage::sysLocalStoreSetWritable(jobContext->sBuffer, job->header.sizeScratch<<4);
    rage::sysLocalStoreSetWritable(job, sizeof(CellSpursJob256));
#ifdef JOB_USES_EVENT_FLAGS
    rage::sysLocalStoreSetWritable((void*)0x80, 0x100); // spurs scratch area
#endif
    rage::sysLocalStoreChecksum(true);

#if RAGETRACE
	uint64_t traceData = job->workArea.userData[24];
	rage::pfSpuTrace trace(traceData>>32);
	if (traceData)
	{
		rage::g_pfSpuTrace = &trace;
		trace.Push((rage::pfTraceCounterId*)((uint32_t)job->workArea.userData[24]));
	}
#endif

	// We've reserved 16 bytes for this at the end of our output buffer
	// (see task_psn.cpp) as it must have the right lifetime
	void** outBuf = job->header.useInOutBuffer ? &jobContext->ioBuffer : &jobContext->oBuffer;
	uint32_t* outBufSz = job->header.useInOutBuffer ? &job->header.sizeInOrInOut : &job->header.sizeOut;
	*outBufSz -= 16;
	uint64_t* dones = (uint64_t*)(*outBufSz + (uintptr_t)*outBuf);

#if TASK_SPU_EXTRA_CHECKS
    // Save the contents of address 0 
	g_savedLs0 = *((vector signed char*) 0);
    // We also add markers at the end of scratch and io buffers. 
    // Note that we can only do this because the PPU side reserved the extra memory
    unsigned int sizeMarker = 16;
	ioBufferEnd = 0;
	oBufferEnd = 0;
	sBufferEnd = 0;
    if (job->header.sizeInOrInOut) 
    {
        job->header.sizeInOrInOut -= sizeMarker;
        ioBufferEnd = (vector signed char* )(job->header.sizeInOrInOut + (uintptr_t)jobContext->ioBuffer);
        *ioBufferEnd = kMarker;
    }
    if (job->header.sizeOut && !job->header.useInOutBuffer) 
    {
        job->header.sizeOut -= sizeMarker;
        oBufferEnd = (vector signed char* )(job->header.sizeOut + (uintptr_t)jobContext->oBuffer);
        *oBufferEnd = kMarker;	
    }
    if (job->header.sizeScratch) 
    {
        job->header.sizeScratch -= sizeMarker>>4;
        sBufferEnd = (vector signed char*)((job->header.sizeScratch<<4) + (uintptr_t)jobContext->sBuffer);
        *sBufferEnd = kMarker;	
    }
#endif//TASK_SPU_EXTRA_CHECKS

#if TASK_SPU_DEBUG_SPEW
	{
	Displayf("JOB@spu%i: numIoBuffer = %d, numCacheBuffer = %d, %sBuffer = %p(%04x), oBuffer = %p(%04x), sBuffer = %p(%04x)",
		cellSpursGetCurrentSpuId(),
		jobContext->numIoBuffer, jobContext->numCacheBuffer, 
		job->header.useInOutBuffer? "io": "i",
		jobContext->ioBuffer, job->header.sizeInOrInOut, 
		jobContext->oBuffer, job->header.sizeOut,
		jobContext->sBuffer, job->header.sizeScratch<<4);
	for (uint32_t i=0; i<jobContext->numIoBuffer; i++)
		Displayf("input[%d]@spu%i = %016llx",i,cellSpursGetCurrentSpuId(),job->workArea.userData[i]);
	for (uint32_t i=0; i<jobContext->numCacheBuffer; i++)
		Displayf("readonly[%d]@spu%i = %p",i,cellSpursGetCurrentSpuId(),jobContext->cacheBuffer[i]);
	uint32_t *userData = (uint32_t*) &job->workArea.userData[jobContext->numIoBuffer + jobContext->numCacheBuffer + (jobContext->oBuffer? 1 : 0)];
	for (uint32_t i=0; i<*userData; i++)
		Displayf("userdata[%d]@spu%i = %x",i,cellSpursGetCurrentSpuId(),userData[i+1]);
	}
#endif

#ifdef TASK_SPU_2
    ::rage::sysTaskContext p(jobContext, job);
#if PASS_SPURS_ARGS
	ENTRYPOINT(p, jobContext, job);
#else
	ENTRYPOINT(p);
#endif // PASS_SPURS_ARGS
#else
#if TASK_SPU_DEBUG_SPEW
	{
    uint32_t *userData = (uint32_t*) &job->workArea.userData[jobContext->numIoBuffer + jobContext->numCacheBuffer + (jobContext->oBuffer? 1 : 0)];
    for (uint32_t i=0; i<*userData; i++)
        Displayf("userdata[%d]@spu%i = %x",i,cellSpursGetCurrentSpuId(),userData[i+1]);
	}
#endif
    ::rage::sysTaskParameters p;

    // Retrieve IO buffers
    void *ptrList[26];
    cellSpursJobGetPointerList(ptrList, &job->header, jobContext);

    // Locate the input buffer in LS and its size (from the original input DMA)
    // Warning: because we add an extra qword for synchronization if in inout mode
    // we must subtract it here
    p.Input.Size = job->header.sizeInOrInOut;
    p.Input.Data = (void *) ptrList[0];
    int nextData = jobContext->numIoBuffer;

    // Fill in the read-only data
    p.ReadOnlyCount = jobContext->numCacheBuffer;
    for (int i=0; i<jobContext->numCacheBuffer; i++) {
        p.ReadOnly[i].Data = jobContext->cacheBuffer[i];
        p.ReadOnly[i].Size = (size_t)(job->workArea.dmaList[i+nextData] >> 32);
    }

    // Locate the output buffer in LS and its size (from the original user data)
    nextData += jobContext->numCacheBuffer;
    size_t dmaDestAddr, dmaSize;
    void *dmaLocalAddr;
    if (jobContext->oBuffer) {
        dmaDestAddr = (size_t)(job->workArea.dmaList[nextData]);
        p.Output.Data = dmaLocalAddr = jobContext->oBuffer;
        p.Output.Size = dmaSize = (size_t)(job->workArea.dmaList[nextData++] >> 32);
    }
	else if(job->header.useInOutBuffer) {
		dmaDestAddr = (size_t)job->workArea.userData[0];
		dmaSize = p.Input.Size;
		dmaLocalAddr = p.Input.Data;
		p.Output.Data = 0;
		p.Output.Size = 0;
	}
	else {
		dmaDestAddr = 0;
		dmaSize = 0;
		dmaLocalAddr = 0;
		p.Output.Data = 0;
		p.Output.Size = 0;
	}

    // Finally copy in any user data
    int *userData = (int*) (void*) &job->workArea.dmaList[nextData];
    p.UserDataCount = *userData;
    for (int i=0; i<*userData; i++)
        p.UserData[i].asInt = userData[i+1];

    // Fill in scratch information
    p.Scratch.Data = jobContext->sBuffer;
    p.Scratch.Size = (job->header.sizeScratch << 4);	// input is in quadwords, fix it

#if TASK_SPU_DEBUG_SPEW
	Displayf("BEGIN Task inputSize=%lu, outputSize=%lu",p.Input.Size,p.Output.Size);
#endif

#if PASS_SPURS_ARGS
	ENTRYPOINT(p, jobContext, job);
#else
	ENTRYPOINT(p);
#endif // PASS_SPURS_ARGS

	/// Write the output.  
#if TASK_SPU_DEBUG_SPEW
	Displayf("END Dma from %p to %lx, %ld bytes",dmaLocalAddr,dmaDestAddr,dmaSize);
#endif
		
	sysDmaLargePut(dmaLocalAddr, dmaDestAddr, dmaSize, jobContext->dmaTag);

#endif // TASK_SPU_2

#if RAGETRACE
	if (traceData)
	{
		trace.Pop();
		trace.Finish(jobContext->dmaTag, false);
	}
#endif

	uint32_t eventFlag = job->workArea.userData[25] >> 32;
	if (Likely(eventFlag)) {
		uint32_t eventBit = job->workArea.userData[25] & 63;
		// We no longer depend on the exact order these happen in; we always wait for the handle and the memory flag.
		// We check the memory flag first when doing a Poll, then do the sys_event_flag_wait if-and-only-if the memory flag 
		// is ready to save a syscall in that case.  We wait on the event flag first when doing a Wait because we don't
		// want to waste cycles polling if it's going to be a while, but then we poll on the memory flag afterward to
		// make sure the output DMA is really finished.
		sys_event_flag_set_bit_impatient(eventFlag, eventBit);
		uint64_t doneAddr = jobContext->eaJobDescriptor + 248;
		dones[1] = 0;
		sysDmaSmallPutf(&dones[1], doneAddr, 8, jobContext->dmaTag);

#if DEBUG_TASK_COMPLETION // If we are using reloaded spu jobs they won't have the PPU_SYMBOLS resolved.
		if (PPU_SYMBOL(s_TaskLog))
		{
			DEBUG_TASK_COMPLETION_ONLY(PPU_SYMBOL(s_TaskLog)->Log('ZERO',(uint32_t)job->workArea.userData[25],0,0));
		}
#endif
	}

    checkSpursMarkers("exit");

    if (!rage::sysLocalStoreChecksum())
        __debugbreak();
}

#endif
