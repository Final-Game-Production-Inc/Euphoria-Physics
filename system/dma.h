//
// system/dma.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_DMA_H
#define SYSTEM_DMA_H

#include "diag/trap.h"

/*	Configuration options for this file, via the preprocessor:

	SYS_DMA_AGGRESSIVE_INLINING
		Force inline most dma functions.
		On by default, except when __ASSERT.

	SYS_DMA_BLOCKING
		Make all dma commands block.

	SYS_DMA_BOOKMARKS
		Set bookmarks that can be detected by tuner.

	SYS_DMA_BREAKPOINTS
		Allows EA and LS DMA breakpoints.

	SYS_DMA_TIMING
		Sums all the time spent waiting for DMA commands. 

	SYS_DMA_DETAILED_TIMING
		Collates the time spend waiting for DMA commands by file and line. 

	SYS_DMA_VALIDATION
		DMA command validation.

		1 -	General checks on valid alignment, size and DMA source and 
			destination addresses for all DMA commands. 
		2 - Tags LS areas as writable and readonly. DMA commands are checked to 
			be within writable areas. At job or task start and termination the 
			writable areas are checksummed. 

			For LS validation the job or task has to call sysLocalStoreInit, and 
			sysLocalStoreSetWritable. For checksum, call sysLocalStoreChecksum
			to initialize, and sysLocalStoreChecksum(false) to test.

	SYS_DMA_VERBOSE
		Adds file and line output to DMA asserts.
	
	SYS_DMA_DELAYED
		DMA commands are not started until a matching wait is called

*/

#ifdef SYS_DMA_EVERYTHING
#define SYS_DMA_BOOKMARKS		1	 
#define SYS_DMA_BREAKPOINTS		1	
#define SYS_DMA_TIMING			1
#define SYS_DMA_DETAILED_TIMING	1
#define SYS_DMA_VALIDATION		1	
#define SYS_DMA_VERBOSE			1
#define SYS_DMA_DELAYED			0
#define SYS_DMA_DELAYED_SPEW	0
#endif

#if __FINAL || defined(SYS_DMA_NOTHING)
#define SYS_DMA_BLOCKING		0
#define SYS_DMA_BOOKMARKS		0
#define SYS_DMA_BREAKPOINTS		0
#define SYS_DMA_TIMING			0
#define SYS_DMA_DETAILED_TIMING	0
#define SYS_DMA_VALIDATION		0
#define SYS_DMA_VERBOSE			0
#define SYS_DMA_DELAYED			0
#define SYS_DMA_DELAYED_SPEW	0
#endif

// Calling code can replace these:
#ifndef SYS_DMA_AGGRESSIVE_INLINING
#define SYS_DMA_AGGRESSIVE_INLINING     (__ASSERT? 0 : 1)
#endif

#ifndef SYS_DMA_BOOKMARKS
#define SYS_DMA_BOOKMARKS		        (__ASSERT && !__OPTIMIZED)
#endif

#ifndef SYS_DMA_BREAKPOINTS
#define SYS_DMA_BREAKPOINTS		        (__ASSERT && !__OPTIMIZED)
#endif

#ifndef SYS_DMA_VALIDATION
#define SYS_DMA_VALIDATION		        (__ASSERT? 1 : 0)
#endif

#ifndef SYS_DMA_VERBOSE
#define SYS_DMA_VERBOSE			        (0)
#endif

#ifndef SYS_DMA_TIMING			        // This is about 1-2% overhead because reading the timer forces a wait.  
#define SYS_DMA_TIMING			        (0)
#endif

#ifndef SYS_DMA_DETAILED_TIMING	        // This is about 1-2% overhead because reading the timer forces a wait. 
#define SYS_DMA_DETAILED_TIMING	        (0)
#endif

#ifndef SYS_DMA_BLOCKING
#define SYS_DMA_BLOCKING		        (0)
#endif

#ifndef SYS_DMA_TASK
#define SYS_DMA_TASK			        (0)
#endif

#ifndef SYS_DMA_DELAYED			        // This queues up DMAs and only issues them once a matching wait is found - Serious hit to performance but playable in most situations
#define SYS_DMA_DELAYED			        (0)
#endif

#ifndef SYS_DMA_DELAYED_SPEW	        // Absurd, unplayable framerate due to spew -- Use this to see every delayed dma and wait call
#define SYS_DMA_DELAYED_SPEW	        (0)
#endif

#if SYS_DMA_AGGRESSIVE_INLINING
#define SYS_DMA_INLINE      __forceinline
#else
#define SYS_DMA_INLINE      __attribute__((__noinline__))
#endif

#if SYS_DMA_DELAYED_SPEW
#define DELAY_SPEW_ONLY(x) (x)
#else
#define DELAY_SPEW_ONLY(x)
#endif


#if __SPU

#ifdef __CELL_DMA_H__
#error "Please include system/dma.h before <cell/dma.h>"
#endif

#define NO_CELL_DMA_ASSERT
#include <cell/dma.h>

#if SYS_DMA_DETAILED_TIMING
#include <algorithm>
#endif // SYS_DMA_DETAILED_TIMING

#if SYS_DMA_DELAYED
#include <string.h>
#endif

enum eSysDmaCmd
{
	SYS_DMA_WAIT = 0,
	SYS_DMA_GET = MFC_GET_CMD,
	SYS_DMA_PUT = MFC_PUT_CMD,
	SYS_DMA_GETF = MFC_GETF_CMD,
	SYS_DMA_PUTF = MFC_PUTF_CMD,
	SYS_DMA_GETB = MFC_GETB_CMD,
	SYS_DMA_PUTB = MFC_PUTB_CMD,
	SYS_DMA_GETL = MFC_GETL_CMD,
	SYS_DMA_PUTL = MFC_PUTL_CMD,
	SYS_DMA_GETLF = MFC_GETLF_CMD,
	SYS_DMA_PUTLF = MFC_PUTLF_CMD,
	SYS_DMA_GETLB = MFC_GETLB_CMD,
	SYS_DMA_PUTLB = MFC_PUTLB_CMD
};

// libspurs reserves the DMA tag ID 31. no effect on tasks, but affects
// jobs.
// https://ps3.scedev.net/technotes/view/450
#if SYS_DMA_TASK
#define SYS_DMA_MAX_TAG				32U
#define DONTUSE_DMA_SPURS_MASK		0x00000000
#else
#define SYS_DMA_MAX_TAG				31U
#define DONTUSE_DMA_SPURS_MASK		u32(1<<SYS_DMA_MAX_TAG)
#endif

#define SYS_DMA_MASK_ALL			(0xFFFFFFFF & ~DONTUSE_DMA_SPURS_MASK)

extern "C" char _etext[];
extern "C" char _end[];

namespace rage {

// Shared file/line params for passing more useful call info through wrappers
#if SYS_DMA_VERBOSE || SYS_DMA_DELAYED || SYS_DMA_DETAILED_TIMING
#define SYS_DMA_FILELINE_PARAMS , __FILE__, __LINE__
#define SYS_DMA_FILELINE_PARAMS_SOLO __FILE__, __LINE__
#define SYS_DMA_FILELINE_ARGS , const char* file, int line
#define SYS_DMA_FILELINE_ARGS_SOLO const char* file, int line
#define SYS_DMA_FILELINE_PASS , file, line
#else
#define SYS_DMA_FILELINE_PARAMS
#define SYS_DMA_FILELINE_PARAMS_SOLO
#define SYS_DMA_FILELINE_ARGS
#define SYS_DMA_FILELINE_ARGS_SOLO
#define SYS_DMA_FILELINE_PASS
#endif

//////////////////////////////////////////////////////////////////////////
// Timing
#if SYS_DMA_DETAILED_TIMING
extern u32 g_DmaWaitTime;

struct sysDmaTimeRecord
{
	u32 time;
	const char* func;
	const char* file;
	int line;
	eSysDmaCmd cmd;

	bool operator<(const sysDmaTimeRecord& other)
	{
		return file < other.file ||
			file == other.file && line < other.line ||
			file == other.file && line == other.line && cmd < other.cmd;
	}

	bool operator==(const sysDmaTimeRecord& other)
	{
		return file == other.file && line == other.line && cmd == other.cmd;
	}
};

bool SortByTimePredicate(const sysDmaTimeRecord& left, const sysDmaTimeRecord& right)
{
	return left.time > right.time;
}

static const int MAX_NUM_TIMED_WAITS = 64;
static sysDmaTimeRecord g_TimeRecords[MAX_NUM_TIMED_WAITS];
static int g_NumRecords;

void sysDmaPrintWaitTimes()
{
	Displayf("[DMA wait times]");

	u32 totalTime = 0;

	std::sort(g_TimeRecords, g_TimeRecords + g_NumRecords, &SortByTimePredicate);

	for (int recordIndex = 0; recordIndex < g_NumRecords; ++recordIndex)
	{
		sysDmaTimeRecord& record = g_TimeRecords[recordIndex];
		const char* cmdName;
		switch (record.cmd)
		{
			case SYS_DMA_GET:	cmdName = "GET";	break;
			case SYS_DMA_PUT:	cmdName = "PUT";	break;
			case SYS_DMA_GETF:	cmdName = "GETF";	break;
			case SYS_DMA_PUTF:	cmdName = "PUTF";	break;
			case SYS_DMA_GETB:	cmdName = "GETB";	break;
			case SYS_DMA_PUTB:	cmdName = "PUTB";	break;
			case SYS_DMA_GETL:	cmdName = "GETL";	break;
			case SYS_DMA_PUTL:	cmdName = "PUTL";	break;
			case SYS_DMA_GETLF:	cmdName = "GETLF";	break;
			case SYS_DMA_PUTLF:	cmdName = "PUTLF";	break;
			case SYS_DMA_GETLB:	cmdName = "GETLB";	break;
			case SYS_DMA_PUTLB:	cmdName = "PUTLB";	break;
			default:			cmdName = "WAIT";	break;
		}
		Displayf("%s:%d: %d (%s %s)", record.file, record.line, record.time, record.func, cmdName);
		totalTime += record.time;
	}

	Displayf("[Total DMA wait time: %d]", totalTime);
}

SYS_DMA_INLINE void _sysDmaAddWaitTime(u32 time, const char* func, const char* file, int line, eSysDmaCmd cmd)
{
	sysDmaTimeRecord searchFor;
	searchFor.time = time;
	searchFor.func = func;
	searchFor.file = file;
	searchFor.line = line;
	searchFor.cmd = cmd;

	sysDmaTimeRecord* found = std::lower_bound(g_TimeRecords, g_TimeRecords + g_NumRecords, searchFor);

	if (*found == searchFor)
	{
		found->time += time;
	}
	else
	{
		if (Verifyf(g_NumRecords < MAX_NUM_TIMED_WAITS, "You need to increase MAX_NUM_TIMED_WAITS (currently %d)", MAX_NUM_TIMED_WAITS))
		{
			for (sysDmaTimeRecord* last = g_TimeRecords + g_NumRecords; last >= found; last--)
			{
				*last = *(last - 1);
			}
			*found = searchFor;
			g_NumRecords++;
		}
	}
}

#define SYS_DMA_TIMING_BEGIN u32 start = ~spu_readch(SPU_RdDec)
#define SYS_DMA_TIMING_END u32 time = ~spu_readch(SPU_RdDec) - start; g_DmaWaitTime += time; _sysDmaAddWaitTime(time, __FUNCTION__, file, line, cmd)
#define SYS_DMA_TIMING_END_WAIT u32 time = ~spu_readch(SPU_RdDec) - start; g_DmaWaitTime += time; _sysDmaAddWaitTime(time, __FUNCTION__, file, line, SYS_DMA_WAIT)

#else // SYS_DMA_DETAILED_TIMING

#if SYS_DMA_TIMING
extern u32 g_DmaWaitTime;
#define SYS_DMA_TIMING_BEGIN		uint32_t now = ~spu_readch(SPU_RdDec)
#define SYS_DMA_TIMING_END			g_DmaWaitTime += ~spu_readch(SPU_RdDec) - now
#define SYS_DMA_TIMING_END_WAIT		g_DmaWaitTime += ~spu_readch(SPU_RdDec) - now
#else
#define SYS_DMA_TIMING_BEGIN
#define SYS_DMA_TIMING_END
#define SYS_DMA_TIMING_END_WAIT
#endif

#define sysDmaPrintWaitTimes()

#endif // SYS_DMA_DETAILED_TIMING

//////////////////////////////////////////////////////////////////////////
// Bookmarks
#if SYS_DMA_BOOKMARKS
#define SPU_BOOKMARK(value)			__asm__ volatile ("wrch $69,%0;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;nop" :: "r" (value));
#else
#define SPU_BOOKMARK(value)
#endif

//////////////////////////////////////////////////////////////////////////
// Verbose
#if SYS_DMA_VERBOSE // Forgive me lord, for I have sinned...
# define SYS_DMA_VERBOSE_PRINTF				"%s(%d) :"
#else
# define SYS_DMA_VERBOSE_PRINTF
#endif 

//////////////////////////////////////////////////////////////////////////
// Validation
#if SYS_DMA_VALIDATION 
extern const char *sysDmaContext;
extern u32 sysLastCommandBuffer;
#define sysDmaPushContext(x)	const char *prev = ::rage::sysDmaContext; sysDmaContext = x;		// NOT MULTI-STMT SAFE!!!
#define sysDmaSetContext(x)		::rage::sysDmaContext = x;
#define sysDmaPopContext()		::rage::sysDmaContext = prev

#if SYS_DMA_VERBOSE
bool sysDmaAssertFailVerbose(const char* file,u32 line,const char *tag,u32 value);
#define sysDmaAssert(var, cond)	if (Unlikely(!(cond))) { sysDmaAssertFailVerbose(file,line,#var,(u32)var); }
#else // SYS_DMA_VERBOSE
bool sysDmaAssertFail(const char *tag,u32 value);
#define sysDmaAssert(var, cond)	if (Unlikely(!(cond))) { sysDmaAssertFail(#var,(u32)var); }
#endif 

#else
#define sysDmaAssert(var,cond)
#define sysDmaPushContext(x)
#define sysDmaSetContext(x)
#define sysDmaPopContext()
#endif // __ASSERT


#if SYS_DMA_VALIDATION==2
extern u32 g_sysLsNumWritableBlocks;
extern u32 g_sysLsWritableBlocks[28];
extern u16 g_sysLsChecksums[192];


void sysLocalStoreSetWritable(const void* block, u32 size)
{
	if (size)
	{
		u32 begin = ((u32)block) & ~15;
		u32 end = ((u32)block + size + 15) & ~15;
		int i;
		for(i = g_sysLsNumWritableBlocks - 1; i >= 0 && g_sysLsWritableBlocks[i] > begin; --i)
			g_sysLsWritableBlocks[i+2] = g_sysLsWritableBlocks[i];
		g_sysLsWritableBlocks[i+1] = begin;
		g_sysLsWritableBlocks[i+2] = end;
		g_sysLsNumWritableBlocks += 2;
	}

	Assert(g_sysLsNumWritableBlocks < 28);
}

void sysLocalStoreInit(u32 stacksize=0)
{
	g_sysLsNumWritableBlocks = 0;
	vec_uint4 sp;
	asm volatile("lr %0, $1" : "=r" (sp)); 
	u32 stacktop = spu_extract(sp,0); 
	while (*(u32*)stacktop > stacktop)  
		stacktop = *(u32*)stacktop;
	stacktop += 16;
	sysLocalStoreSetWritable((void*)(stacktop - stacksize), stacksize);
	sysLocalStoreSetWritable(_etext, (stacksize ? (u32)_end : stacktop) - (u32)_etext);
}


bool sysLocalStoreChecksum(bool init=false)
{
	mfc_write_tag_mask(SYS_DMA_MASK_ALL);
	mfc_read_tag_status_all();

	u32 block = 0;
	u32 addr = 0;
	bool result = true;
	u32 csnum = 0;
	u32 endaddr = g_sysLsWritableBlocks[0];
	vec_ushort8 cs = (vec_ushort8)(0);
	do
	{
		cs = spu_add(cs, *(vec_ushort8*)addr);
		addr += 16;
		if (Unlikely(!(addr & 2047)) || Unlikely(addr == endaddr))
		{
			cs = spu_add(cs, spu_rlqwbyte(cs, 8));
			cs = spu_add(cs, spu_rlqwbyte(cs, 4));
			cs = spu_add(cs, spu_rlqwbyte(cs, 2));
			u16 sum = spu_extract(cs, 0);
			if (__builtin_expect(!init, 1) && __builtin_expect(sum != g_sysLsChecksums[csnum], 0))
			{
				// Also see JOB_USES_EVENT_FLAGS - changes can occur inside the spurs kernel when event flags are used (0x0080-0x0180)
				Displayf("Block ending at %x corrupted (%x != %x)!", addr, sum, g_sysLsChecksums[csnum]);
				if (addr == 0x800)
					Displayf("(if you're using event flags, this might be a false alarm)");
				result = false;
			}
			Assert(csnum < 192);
			g_sysLsChecksums[csnum++] = sum;
			cs = (vec_ushort8)(0);
		}
		while (Unlikely(addr == endaddr))
		{
			block += 2;
			addr = g_sysLsWritableBlocks[block-1];
			if (Likely(block < g_sysLsNumWritableBlocks))
				endaddr = g_sysLsWritableBlocks[block];
		}
	} while (Likely(addr < 256 * 1024));
	if (Unlikely(!result))
		for(u32 i=0; i<g_sysLsNumWritableBlocks; i+=2)
			Displayf("Writable : %x - %x", g_sysLsWritableBlocks[i], g_sysLsWritableBlocks[i+1]);

	return result;
}

static bool sysLocalStoreValidateRange(const void* ls, u32 size SYS_DMA_FILELINE_ARGS)
{
	if (g_sysLsNumWritableBlocks == 0)
		return true;

	for(u32 i=1; i<g_sysLsNumWritableBlocks; i+=2)
		if ((u32)ls >= g_sysLsWritableBlocks[i-1] && (u32)ls + size <= g_sysLsWritableBlocks[i])
			return true;
	Printf("SPU LS pointer validation failed "SYS_DMA_VERBOSE_PRINTF" : LS = 0x%p Size = 0x%x\nWriteable blocks-\n" SYS_DMA_FILELINE_PASS, ls, size);
	for(u32 i=1; i<g_sysLsNumWritableBlocks; i+=2)
		Printf("\tstart=0x%x, end=0x%x\n", g_sysLsWritableBlocks[i-1], g_sysLsWritableBlocks[i]);

	return false;
}

#else // SYS_DMA_VALIDATION==2
__forceinline void sysLocalStoreInit(u32 UNUSED_PARAM(stacksize) = 0) {}
__forceinline void sysLocalStoreSetWritable(const void*, u32 ) {}
__forceinline bool sysLocalStoreChecksum(bool UNUSED_PARAM(init)=false) {return true;}
__forceinline bool sysLocalStoreValidateRange(const void* ls, u32 size SYS_DMA_FILELINE_ARGS) {return true;}
#endif // SYS_DMA_VALIDATION==2

#if SYS_DMA_VALIDATION

void sysValidateDmaRange(const void *ls,uint64_t ppu,uint32_t size,uint32_t tag SYS_DMA_FILELINE_ARGS)
{
	if (!size) return;

	// Catch attempts to DMA over spurs kernel or beyond end of storage
	sysDmaAssert(ls,(uint32_t)ls >= 8192 && (uint32_t)ls + size <= 256*1024);
	// Make sure ppu address is valid too.  Could be more exhaustive here.
	sysDmaAssert(ppu,ppu >= 256*1024);
	// Validate tag
	TrapGE(tag,SYS_DMA_MAX_TAG);

	sysDmaAssert(ls,sysLocalStoreValidateRange(ls,size SYS_DMA_FILELINE_PASS));
}

void sysValidateDmaTypedRange(uint64_t ppu,uint32_t tag SYS_DMA_FILELINE_ARGS)
{
	// Make sure ppu address is valid too.  Could be more exhaustive here.
	sysDmaAssert(ppu,ppu >= 256*1024);
	// Validate tag
	TrapGE(tag,SYS_DMA_MAX_TAG);
}

void sysValidateDma(const void *ls,uint64_t ppu,uint32_t size,uint32_t tag SYS_DMA_FILELINE_ARGS)
{
	sysDmaAssert(ls,((uint32_t)ls & 15) == 0);
	sysDmaAssert(ppu,((uint32_t)ppu & 15) == 0);
	// For some reason code generation is worse if these two tests are combined:
	sysDmaAssert(size,(size & 15)==0);
	sysDmaAssert(size,size<=16384);
	sysValidateDmaRange(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
}

void sysValidateDmaList(const void *ls,const CellDmaListElement* elements,uint32_t sizeElements,uint32_t tag SYS_DMA_FILELINE_ARGS)
{
	sysDmaAssert(sizeElements,(sizeElements & 7) == 0);
	const uint8_t* ls8 = reinterpret_cast<const uint8_t*>(ls);
	while (sizeElements)
	{
		sysValidateDma(ls8,elements->eal,elements->size,tag SYS_DMA_FILELINE_PASS);
		ls8 += (elements->size + 15) & ~15;
		sizeElements -= sizeof(CellDmaListElement);
		++elements;
	}
}

void sysValidateDmaTyped(uint64_t lsbMask,uint64_t ppu,uint32_t tag SYS_DMA_FILELINE_ARGS)
{
	sysDmaAssert(ppu,(ppu & lsbMask) == 0);
	sysValidateDmaTypedRange(ppu,tag SYS_DMA_FILELINE_PASS);
}

void sysValidateDmaLarge(const void *ls,uint64_t ppu,uint32_t size,uint32_t tag SYS_DMA_FILELINE_ARGS)
{
	sysDmaAssert(ls,((uint32_t)ls & 15) == 0);
	sysDmaAssert(ppu,((uint32_t)ppu & 15) == 0);
	sysValidateDmaRange(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
}

void sysValidateDmaSmall(const void *ls,uint64_t ppu,uint32_t size,uint32_t tag SYS_DMA_FILELINE_ARGS)
{
	sysDmaAssert(ls,((uint32_t)ls & 15) == ((uint32_t)ppu & 15));
	sysDmaAssert(ls,((uint32_t)ls & (size-1)) == 0);
	sysDmaAssert(size,size == 1 || size == 2 || size == 4 || size == 8);
	sysValidateDmaRange(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
}

void sysValidateDmaMask(u32 mask SYS_DMA_FILELINE_ARGS)
{
	// should we catch mask==0 as an error?
	sysDmaAssert(mask,((uint32_t)mask & DONTUSE_DMA_SPURS_MASK) == 0);
}

#else	// !SYS_DMA_VALIDATION
__forceinline void sysValidateDmaRange(const void*,uint64_t,uint32_t,uint32_t SYS_DMA_FILELINE_ARGS){}
__forceinline void sysValidateDmaTypedRange(uint64_t,uint32_t SYS_DMA_FILELINE_ARGS){}
__forceinline void sysValidateDma(const void *,uint64_t,uint32_t,uint32_t SYS_DMA_FILELINE_ARGS){}
__forceinline void sysValidateDmaList(const void*,const CellDmaListElement*,uint32_t,uint32_t SYS_DMA_FILELINE_ARGS){}
__forceinline void sysValidateDmaTyped(uint64_t,uint64_t,uint32_t SYS_DMA_FILELINE_ARGS){}
__forceinline void sysValidateDmaLarge(const void*,uint64_t,uint32_t,uint32_t SYS_DMA_FILELINE_ARGS){}
__forceinline void sysValidateDmaSmall(const void*,uint64_t,uint32_t,uint32_t SYS_DMA_FILELINE_ARGS){}
__forceinline void sysValidateDmaMask(u32 mask SYS_DMA_FILELINE_ARGS){}
#endif	// SYS_DMA_VALIDATION


//////////////////////////////////////////////////////////////////////////
// Breakpoints
#if SYS_DMA_BREAKPOINTS

extern const u8* g_DataBreakpoint;
extern u32 g_DataBreakpointSize;
extern const u8* g_DataEABreakpoint;
extern u32 g_DataEABreakpointSize;

SYS_DMA_INLINE void sysCheckDmaDataBreakpoint(const void *ls,uint32_t size)
{
	if (Unlikely((u32)g_DataBreakpoint))
	{
		const u8* bpBegin = g_DataBreakpoint;
		const u8* bpEnd = g_DataBreakpoint + g_DataBreakpointSize;
		const u8* dmaBegin = reinterpret_cast<const u8*>(ls);
		const u8* dmaEnd = reinterpret_cast<const u8*>(ls) + size;

		if ( Unlikely(bpBegin < dmaEnd && bpEnd > dmaBegin) )
		{
			Printf("SPU Data Breakpoint: DMA from address 0x%x to 0x%x would overwrite part of 0x%x to 0x%x\n", (u32)dmaBegin, (u32)dmaEnd, (u32)bpBegin, (u32)bpEnd);
			__debugbreak();
		}
	}
}

SYS_DMA_INLINE void sysCheckDmaEADataBreakpoint(uint32_t ea,uint32_t size)
{
	if (Unlikely((u32)g_DataEABreakpoint))
	{
		const u8* bpBegin = g_DataEABreakpoint;
		const u8* bpEnd = g_DataEABreakpoint + g_DataEABreakpointSize;
		const u8* dmaBegin = reinterpret_cast<const u8*>(ea);
		const u8* dmaEnd = reinterpret_cast<const u8*>(ea) + size;

		if (Unlikely(bpBegin < dmaEnd && bpEnd > dmaBegin))
		{
			Printf("SPU Data Breakpoint: DMA from address 0x%x to 0x%x would overwrite part of EA 0x%x to 0x%x\n", (u32)dmaBegin, (u32)dmaEnd, (u32)bpBegin, (u32)bpEnd);
			__debugbreak();
		}
	}
}

SYS_DMA_INLINE void sysCheckDmaEADataBreakpoint(const mfc_list_element_t* list,uint32_t size)
{
	// TODO: not currently supported
}

__forceinline void sysSetDmaDataBreakpoint(const void* ls, uint32_t size)
{
	g_DataBreakpoint = reinterpret_cast<const u8*>(ls);
	g_DataBreakpointSize = size;
}

__forceinline void sysClearDmaDataBreakpoint()
{
	g_DataBreakpoint = NULL;
	g_DataBreakpointSize = 0;
}

__forceinline void sysSetDmaEADataBreakpoint(uint64_t ea, uint32_t size)
{
	g_DataEABreakpoint = reinterpret_cast<const u8*>(ea);
	g_DataEABreakpointSize = size;
}

__forceinline void sysClearEADmaDataBreakpoint()
{
	g_DataEABreakpoint = NULL;
	g_DataEABreakpointSize = 0;
}
#define sysCheckDmaBreakPoints(cmd,ls,ppu_or_list,size,tag) ((cmd&MFC_GET_CMD)?::rage::sysCheckDmaDataBreakpoint(ls,size): ::rage::sysCheckDmaEADataBreakpoint(ppu_or_list,size))
#else	// !SYS_DMA_BREAKPOINTS
__forceinline void sysClearDmaDataBreakpoint(){}
__forceinline void sysSetDmaDataBreakpoint(const void*, uint32_t){}
__forceinline void sysClearDmaEADataBreakpoint(){}
__forceinline void sysSetDmaEADataBreakpoint(uint64_t, uint32_t){}
#define sysCheckDmaBreakPoints(cmd,ls,ppu,size,tag)		(void)0
#endif	// !SYS_DMA_BREAKPOINTS

//////////////////////////////////////////////////////////////////////////
// Data and helpers for DMA delaying
#if !SYS_DMA_DELAYED

#define SYS_DMA_DSPEW_INTERFACE_PARAMS 
#define SYS_DMA_DSPEW_PARAMS 
#define SYS_DMA_DSPEW_ARGS 
#define SYS_DMA_INTERFACE_PARAMS 
#define SYS_DMA_PARAMS 

#else // SYS_DMA_DELAYED is on

// The extra duplication here feels stupid, but we really want to have the caller info for asserts even when not spewing
#define SYS_DMA_INTERFACE_PARAMS const char* file, u32 line, 
#define SYS_DMA_PARAMS file, line, 
#define SYS_DMA_ARGS __FILE__, __LINE__, 

#if SYS_DMA_DELAYED_SPEW // Same stuff as the DMA verbose setting, we just don't want to be stepping on each other's toes
#define SYS_DMA_DSPEW_INTERFACE_PARAMS const char* file, u32 line, 
#define SYS_DMA_DSPEW_PARAMS file, line, 
#define SYS_DMA_DSPEW_ARGS __FILE__, __LINE__, 
#else
#define SYS_DMA_DSPEW_INTERFACE_PARAMS 
#define SYS_DMA_DSPEW_PARAMS 
#define SYS_DMA_DSPEW_ARGS 
#endif

enum eDelayedDmaCmd
{
	SYSDMA = 0,
	SYSDMASMALL = 1,
	SYSDMALARGE = 2,
	SYSDMALIST = 3
};

struct sysDmaDelayedEntry
{
	eSysDmaCmd cmd;
	const void* ls;
	rage::u32 ppu;
	rage::u32 size;
	eDelayedDmaCmd entryType;
	rage::u32 tag;
};

static const rage::u32 MAX_NUM_DMAS_DELAYED = 48;
static sysDmaDelayedEntry g_DelayedDMAs[MAX_NUM_DMAS_DELAYED];
static rage::u32 g_NumDelayedDMAs = 0;

SYS_DMA_INLINE void InitiateDelayedDMA(sysDmaDelayedEntry curEntry)
{
	switch(curEntry.entryType)
	{
	case SYSDMA:
		{
			spu_mfcdma32((volatile void*)curEntry.ls, curEntry.ppu, curEntry.size, curEntry.tag, curEntry.cmd);
			break;
		}
	case SYSDMASMALL:
		{
			spu_mfcdma32((volatile void*)curEntry.ls, curEntry.ppu, curEntry.size, curEntry.tag, curEntry.cmd);
			break;
		}
	case SYSDMALARGE:
		{
			cellDmaLargeCmd((uintptr_t)curEntry.ls, (uint64_t)curEntry.ppu, (uint32_t)curEntry.size, (uint32_t)curEntry.tag, MFC_CMD_WORD(0,0,(uint32_t)curEntry.cmd));
			break;
		}
	case SYSDMALIST:
		{
			spu_mfcdma32((volatile void*)curEntry.ls, curEntry.ppu, curEntry.size, curEntry.tag, curEntry.cmd);
			break;
		}
	default:
		{
			break;
		}
	};
}

// We avoid forceinlining all the front end functions to save SPU space -- Performance is just a luxury when we're trying to verify our DMA/Wait correctness
/*SYS_DMA_INLINE*/ void AddDelayedDMA(SYS_DMA_INTERFACE_PARAMS eSysDmaCmd cmd, const void* ls, rage::u32 ppu, rage::u32 size, rage::u32 tag, eDelayedDmaCmd entryType)
{
	sysDmaDelayedEntry newEntry;
	newEntry.cmd = cmd;
	newEntry.ls = ls;
	newEntry.ppu = ppu;
	newEntry.size = size;
	newEntry.entryType = entryType;
	newEntry.tag = tag;

	// Classify this new entry so we can perform appropriate checks on the input
	// - f.ex. We probably want to do more with lists at some point (Or could do less work keeping buffer order if there are no fences or barriers present)
	bool isGet = false, isList = false, isFence = false, isBarrier = false;
	switch(newEntry.cmd)
	{
		case SYS_DMA_GET: { isGet = true; break; }
		case SYS_DMA_GETF: { isGet = true; isFence = true; break; }
		case SYS_DMA_GETB: { isGet = true; isBarrier = true; break; }
		case SYS_DMA_GETL: { isGet = true; isList = true; break; }
		case SYS_DMA_GETLF: { isGet = true; isList = true; isFence = true; break; }
		case SYS_DMA_GETLB: { isGet = true; isList = true; isBarrier = true; break; }

		case SYS_DMA_PUT: { isGet = false; break; }
		case SYS_DMA_PUTF: { isGet = false; isFence = true; break; }
		case SYS_DMA_PUTB: { isGet = false; isBarrier = true; break; }
		case SYS_DMA_PUTL: { isGet = false; isList = true; break; }
		case SYS_DMA_PUTLF: { isGet = false; isList = true; isFence = true; break; }
		case SYS_DMA_PUTLB: { isGet = false; isList = true; isBarrier = true; break; }

		default: { break; } // Shouldn't ever default
	}

	// We're going to check all existing DMAs to make sure they aren't sharing memory improperly
	// - I'm not convinced that PPU overlap really indicates a definite problem, but it would be suspicious
	const u8* curStart = isGet ? reinterpret_cast<const u8*>(newEntry.ls) : reinterpret_cast<const u8*>(newEntry.ppu);
	const u8* curEnd = isGet ? reinterpret_cast<const u8*>(newEntry.ls) + newEntry.size : reinterpret_cast<const u8*>(newEntry.ppu) + newEntry.size;
	rage::u32 ignoreTag = rage::u32(-1);
	for(rage::u32 index = g_NumDelayedDMAs; index > 0; index--)
	{
		// If we're a fence or barrier, we're guaranteed to finish after everything before us on the same tag
		if((isFence || isBarrier) && (g_DelayedDMAs[index-1].tag == newEntry.tag))
		{
			continue;
		}

		// If the other pending DMA is a barrier then it and everything before it on the same tag is guaranteed to finish before this new entry
		if((g_DelayedDMAs[index-1].tag == newEntry.tag) && (g_DelayedDMAs[index-1].cmd == SYS_DMA_GETB || g_DelayedDMAs[index-1].cmd == SYS_DMA_GETLB || g_DelayedDMAs[index-1].cmd == SYS_DMA_PUTB || g_DelayedDMAs[index-1].cmd == SYS_DMA_PUTLB))
		{
			ignoreTag = newEntry.tag;
		}
		//
		if(g_DelayedDMAs[index-1].tag == ignoreTag)
		{
			continue;
		}

		const u8* prevStart = isGet ? reinterpret_cast<const u8*>(g_DelayedDMAs[index-1].ls) : reinterpret_cast<const u8*>(g_DelayedDMAs[index-1].ppu);
		const u8* prevEnd = isGet ? reinterpret_cast<const u8*>(g_DelayedDMAs[index-1].ls) + g_DelayedDMAs[index-1].size : reinterpret_cast<const u8*>(g_DelayedDMAs[index-1].ppu) + g_DelayedDMAs[index-1].size;

		// This is redundant with the assert (In that the assert condition is now a truism) -- We're just avoiding a compile warning
		if( Unlikely(curStart < prevEnd && curEnd > prevStart) )
		{
			// IMO this is entirely assert worthy
			// But if you'd prefer spew, flip this define off
#define NO_ASSERT_ON_MEM_OVERLAP 0
#if __ASSERT && !NO_ASSERT_ON_MEM_OVERLAP
			Assertf(curStart >= prevEnd || curEnd <= prevStart, 
				"%s (%d): DMA%c(tag:%d) from address 0x%x to 0x%x would overwrite part of 0x%x to 0x%x from a previous (pending) DMA(tag:%d)", 
				SYS_DMA_PARAMS (isGet?'G':'P'), newEntry.tag-1, (u32)curStart, (u32)curEnd, (u32)prevStart, (u32)prevEnd, g_DelayedDMAs[index-1].tag-1);
#else
			Displayf("%s (%d): DMA%c(tag:%d) from address 0x%x to 0x%x would overwrite part of 0x%x to 0x%x from a previous (pending) DMA(tag:%d)", SYS_DMA_PARAMS (isGet?'G':'P'), newEntry.tag-1, (u32)curStart, (u32)curEnd, (u32)prevStart, (u32)prevEnd, g_DelayedDMAs[index-1].tag-1);
#endif
		}
	}

	// TODO -- Do something with puts and/or lists?
	// Fill LS memory with specific garbage value to make it more obvious when someone uses it before waiting
	if(isGet && !isList)
	{
		memset(const_cast<void*>(newEntry.ls), 0xBDBDBDBD, newEntry.size);
	}

	// Under the assumption that the oldest DMAs being held for longer will make missing DMA/Wait pairs obvious
	//  - We'd really like to issue the incoming new one rather than send off anything in the current buffer when we overflow
	// But we cannot do this due to fences and barriers - Those are supposed to guarantee relative completion order so we need to preserve input order of the DMAs
	if(g_NumDelayedDMAs < MAX_NUM_DMAS_DELAYED)
	{
		DELAY_SPEW_ONLY(Displayf("%s (%d): AddDMA %d %d", SYS_DMA_PARAMS (1<<tag), tag-1));
		g_DelayedDMAs[g_NumDelayedDMAs] = newEntry;
		g_NumDelayedDMAs++;
	}
	else
	{
		DELAY_SPEW_ONLY(Displayf("%s (%d): AddDMA_Overflow %d %d", SYS_DMA_PARAMS (1<<tag), tag-1));
		// TODO - Retain older desired behavior (From comment above) for cases where there is no relevant barrier/fence?
		//InitiateDelayedDMA(newEntry);

		// In practice we actually don't ever get very close to the buffer size (speaking from test runs with the physics jobs - others may use more concurrent DMAs)
		// So we'll go with the simple solution of flushing out the oldest of the new tag to make space (Noting that the oldest of the new tag may in fact be this new entry)
		sysDmaDelayedEntry entryToFlush = newEntry;
		for(rage::u32 index = 0; index < g_NumDelayedDMAs; index++)
		{
			if(g_DelayedDMAs[index].tag == newEntry.tag)
			{
				// We found an existing entry of the same tag, so that will become the entry to remove
				entryToFlush = g_DelayedDMAs[index];
				// Then shift the rest of the list down to preserve relative order
				for(rage::u32 swapI = index+1; swapI < g_NumDelayedDMAs; swapI++)
				{
					g_DelayedDMAs[swapI-1] = g_DelayedDMAs[swapI];
				}
				// And tack the new entry on the back (There is no change in list size - we're still maxed out)
				g_DelayedDMAs[g_NumDelayedDMAs-1] = newEntry;
				// Need to leave the outer loop otherwise we'd just leak entries
				break;
			}
		}
		InitiateDelayedDMA(entryToFlush);
	}
}

/*SYS_DMA_INLINE*/ void FlushDelayedDMAs(rage::u32 tag)
{
	//DELAY_SPEW_ONLY(Displayf("FLUSH DMAs %d %d", (1<<tag), tag-1));
	for(rage::u32 index = g_NumDelayedDMAs; index > 0; index--)
	{
		if(g_DelayedDMAs[index-1].tag == tag)
		{
			InitiateDelayedDMA(g_DelayedDMAs[index-1]);

			// Fences and barriers mean we need to preserve order, so we'll shift the rest of the list down
			for(rage::u32 swapI = index; swapI < g_NumDelayedDMAs; swapI++)
			{
				g_DelayedDMAs[swapI-1] = g_DelayedDMAs[swapI];
			}

			g_NumDelayedDMAs--;
		}
	}
}

// SUPER HACK -- Just made to work for our very specific use since GCC doesn't appear to have atoi by default
SYS_DMA_INLINE const char* ItoAhack(rage::u32 val, char* result)
{
	const char* const numbers = "0123456789";
	rage::u32 digit1 = val / 10;
	rage::u32 digit2 = val - (digit1 * 10);

	result[0] = ' ';
	result[2] = 0;
	result[3] = 0;
	if(digit1 == 0)
	{
		result[1] = numbers[digit2];
	}
	else
	{
		result[1] = numbers[digit1];
		result[2] = numbers[digit2];
	}


	return result;
}

/*SYS_DMA_INLINE*/ void FlushDelayedDMAsMask(SYS_DMA_DSPEW_INTERFACE_PARAMS rage::u32 mask)
{
#if SYS_DMA_DELAYED_SPEW
	const int sizeofBuffer = 90; // Max length of the characters is 86 if all bits are set
	char tagBuffer[sizeofBuffer] = {0};
	for(rage::u32 tag = 1; tag < 32; tag++)
	{
		if((rage::u32(1<<tag) & mask) != 0)
		{
			// No idea why, but all printf variants seem to overrun a bunch of SPU memory and break stuff
			// Which is how we end up with the janky code below
			//	snprintf(tagBuffer, sizeofBuffer, " %d", tag-1);
			char buf[5];
			strcat(tagBuffer, ItoAhack(tag-1, buf));
		}
	}
#endif
	DELAY_SPEW_ONLY(Displayf("FDbMASK count before %d", g_NumDelayedDMAs));
	DELAY_SPEW_ONLY(Displayf("%s (%d): FLUSH DMAs by MASK %d%s", SYS_DMA_DSPEW_PARAMS mask, tagBuffer));
	for(rage::u32 tag = 0; tag < 32; tag++)
	{
		if((rage::u32(1<<tag) & mask) != 0)
		{
			FlushDelayedDMAs(tag);
		}
	}
	DELAY_SPEW_ONLY(Displayf("FDbMASK count after --- %d", g_NumDelayedDMAs));
}

/*SYS_DMA_INLINE*/ void AddDelayedDMAAndFlush(SYS_DMA_INTERFACE_PARAMS eSysDmaCmd cmd, const void* ls, rage::u32 ppu, rage::u32 size, rage::u32 tag, eDelayedDmaCmd entryType)
{
	DELAY_SPEW_ONLY(Displayf("%s (%d): Add Flush and Wait %d %d", SYS_DMA_PARAMS (1<<tag), tag-1));
	AddDelayedDMA(SYS_DMA_PARAMS cmd, ls, ppu, size, tag, entryType);
	FlushDelayedDMAs(tag);
	DELAY_SPEW_ONLY(Displayf("ADDaFwait after %d", g_NumDelayedDMAs));
	mfc_write_tag_mask(1<<tag);
	mfc_read_tag_status_all();
}
#endif // SYS_DMA_DELAYED
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Dma support

SYS_DMA_INLINE void _sysDma(eSysDmaCmd cmd, const void* ls, rage::u32 ppu, rage::u32 size, rage::u32 tag SYS_DMA_FILELINE_ARGS)
{
	sysValidateDma(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
	sysCheckDmaBreakPoints(cmd,ls,ppu,size,tag);
	SYS_DMA_TIMING_BEGIN;
	spu_mfcdma32((volatile void*)ls,ppu,size,tag,cmd);
	SYS_DMA_TIMING_END;
}

SYS_DMA_INLINE void _sysDmaAndWait(eSysDmaCmd cmd, const void* ls, rage::u32 ppu, rage::u32 size, rage::u32 tag SYS_DMA_FILELINE_ARGS)
{
	sysValidateDma(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
	sysCheckDmaBreakPoints(cmd,ls,ppu,size,tag);
	SYS_DMA_TIMING_BEGIN;
	spu_mfcdma32((volatile void*)ls,ppu,size,tag,cmd);
	mfc_write_tag_mask(1<<tag);
	mfc_read_tag_status_all();
	SYS_DMA_TIMING_END;
}

SYS_DMA_INLINE void _sysDmaSmall(eSysDmaCmd cmd, const void* ls, rage::u32 ppu, rage::u32 size, rage::u32 tag SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaSmall(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
	sysCheckDmaBreakPoints(cmd,ls,ppu,size,tag);
	SYS_DMA_TIMING_BEGIN;
	spu_mfcdma32((volatile void*)ls,ppu,size,tag,cmd);
	SYS_DMA_TIMING_END;
}

SYS_DMA_INLINE void _sysDmaSmallAndWait(eSysDmaCmd cmd, const void* ls, rage::u32 ppu, rage::u32 size, rage::u32 tag SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaSmall(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
	sysCheckDmaBreakPoints(cmd,ls,ppu,size,tag);
	SYS_DMA_TIMING_BEGIN;
	spu_mfcdma32((volatile void*)ls,ppu,size,tag,cmd);
	mfc_write_tag_mask(1<<tag);
	mfc_read_tag_status_all();
	SYS_DMA_TIMING_END;
}

SYS_DMA_INLINE void _sysDmaLarge(eSysDmaCmd cmd, const void* ls, rage::u32 ppu, rage::u32 size, rage::u32 tag SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaLarge(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
	sysCheckDmaBreakPoints(cmd,ls,ppu,size,tag);
	SYS_DMA_TIMING_BEGIN;
	cellDmaLargeCmd((uintptr_t)ls, (uint64_t)ppu, (uint32_t)size, (uint32_t)tag, MFC_CMD_WORD(0,0,(uint32_t)cmd));
	SYS_DMA_TIMING_END;
}

SYS_DMA_INLINE void _sysDmaLargeAndWait(eSysDmaCmd cmd, const void* ls, rage::u32 ppu, rage::u32 size, rage::u32 tag SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaLarge(ls,ppu,size,tag SYS_DMA_FILELINE_PASS);
	sysCheckDmaBreakPoints(cmd,ls,ppu,size,tag);
	SYS_DMA_TIMING_BEGIN;
	cellDmaLargeCmd((uintptr_t)ls, (uint64_t)ppu, (uint32_t)size, (uint32_t)tag, MFC_CMD_WORD(0,0,(uint32_t)cmd));
	mfc_write_tag_mask(1<<tag);
	mfc_read_tag_status_all();
	SYS_DMA_TIMING_END;
}

SYS_DMA_INLINE void _sysDmaList(eSysDmaCmd cmd, const void* ls, const mfc_list_element_t* list, rage::u32 size, rage::u32 tag SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaList(ls,list,size,tag SYS_DMA_FILELINE_PASS);
	sysCheckDmaBreakPoints(cmd,ls,list,size,tag);
	SYS_DMA_TIMING_BEGIN;
	spu_mfcdma32((volatile void*)ls,(rage::u32)list,size,tag,cmd);
	SYS_DMA_TIMING_END;
}

SYS_DMA_INLINE void _sysDmaListAndWait(eSysDmaCmd cmd, const void* ls, const mfc_list_element_t* list, rage::u32 size, rage::u32 tag SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaList(ls,list,size,tag SYS_DMA_FILELINE_PASS);
	sysCheckDmaBreakPoints(cmd,ls,list,size,tag);
	SYS_DMA_TIMING_BEGIN;
	spu_mfcdma32((volatile void*)ls,(rage::u32)list,size,tag,cmd);
	mfc_write_tag_mask(1<<tag);
	mfc_read_tag_status_all();
	SYS_DMA_TIMING_END;
}

__forceinline void* sysDmaEa2Ls(u32 ea, const void* ls)
{
	return (void*)(((u32)ls) + (ea & 15));
}

SYS_DMA_INLINE u32 _sysDmaWaitTagStatusAll(SYS_DMA_DSPEW_INTERFACE_PARAMS u32 mask SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaMask(mask SYS_DMA_FILELINE_PASS);

#if SYS_DMA_DELAYED
	FlushDelayedDMAsMask(SYS_DMA_DSPEW_PARAMS mask);
#endif

#if SYS_DMA_TIMING || SYS_DMA_DETAILED_TIMING
	SYS_DMA_TIMING_BEGIN;
	mfc_write_tag_mask(mask);
	uint32_t result = mfc_read_tag_status_all();
	SYS_DMA_TIMING_END_WAIT;
	return result;
#else
	mfc_write_tag_mask(mask);
	return mfc_read_tag_status_all();
#endif
}

SYS_DMA_INLINE u32 _sysDmaWaitTagStatusAny(SYS_DMA_DSPEW_INTERFACE_PARAMS u32 mask SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaMask(mask SYS_DMA_FILELINE_PASS);

#if SYS_DMA_DELAYED
	FlushDelayedDMAsMask(SYS_DMA_DSPEW_PARAMS mask);
#endif

#if SYS_DMA_TIMING || SYS_DMA_DETAILED_TIMING
	SYS_DMA_TIMING_BEGIN;
	mfc_write_tag_mask(mask);
	uint32_t result = mfc_read_tag_status_any();
	SYS_DMA_TIMING_END_WAIT;
	return result;
#else
	mfc_write_tag_mask(mask);
	return mfc_read_tag_status_any();
#endif
}

SYS_DMA_INLINE u32 _sysDmaWaitTagStatusImmediate(SYS_DMA_DSPEW_INTERFACE_PARAMS u32 mask SYS_DMA_FILELINE_ARGS)
{
	sysValidateDmaMask(mask SYS_DMA_FILELINE_PASS);

#if SYS_DMA_DELAYED
	FlushDelayedDMAsMask(SYS_DMA_DSPEW_PARAMS mask);
#endif

#if SYS_DMA_TIMING || SYS_DMA_DETAILED_TIMING
	SYS_DMA_TIMING_BEGIN;
	mfc_write_tag_mask(mask);
	uint32_t result = mfc_read_tag_status_immediate();
	SYS_DMA_TIMING_END_WAIT;
	return result;
#else
	mfc_write_tag_mask(mask);
	return mfc_read_tag_status_immediate();
#endif
}

#define sysDma(cmd,...)							((void)(SYS_DMA_BLOCKING?::rage::_sysDmaAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS): ::rage::_sysDma(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaFileLine(cmd,...)					((void)(SYS_DMA_BLOCKING?::rage::_sysDmaAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS): ::rage::_sysDma(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaSmall(cmd,...)					((void)(SYS_DMA_BLOCKING?::rage::_sysDmaSmallAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS): ::rage::_sysDmaSmall(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaSmallFileLine(cmd,...)			((void)(SYS_DMA_BLOCKING?::rage::_sysDmaSmallAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS): ::rage::_sysDmaSmall(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaLarge(cmd,...)					((void)(SYS_DMA_BLOCKING?::rage::_sysDmaLargeAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS): ::rage::_sysDmaLarge(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaLargeFileLine(cmd,...)			((void)(SYS_DMA_BLOCKING?::rage::_sysDmaLargeAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS): ::rage::_sysDmaLarge(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaList(cmd,...)						((void)(SYS_DMA_BLOCKING?::rage::_sysDmaListAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS): ::rage::_sysDmaList(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaListFileLine(cmd,...)				((void)(SYS_DMA_BLOCKING?::rage::_sysDmaListAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS): ::rage::_sysDmaList(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaAndWait(cmd, ...)					((void)(::rage::_sysDmaAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaAndWaitFileLine(cmd, ...)			((void)(::rage::_sysDmaAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaSmallAndWait(cmd, ...)			((void)(::rage::_sysDmaSmallAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaSmallAndWaitFileLine(cmd, ...)	((void)(::rage::_sysDmaSmallAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaLargeAndWait(cmd, ...)			((void)(::rage::_sysDmaLargeAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaLargeAndWaitFileLine(cmd, ...)	((void)(::rage::_sysDmaLargeAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaListAndWait(cmd, ...)				((void)(::rage::_sysDmaListAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))
#define sysDmaListAndWaitFileLine(cmd, ...)		((void)(::rage::_sysDmaListAndWait(cmd,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)))

#define sysDmaWaitTagStatusAll(mask)			((void)(::rage::_sysDmaWaitTagStatusAll(SYS_DMA_DSPEW_ARGS   mask SYS_DMA_FILELINE_PARAMS)))
#define sysDmaWaitTagStatusAllFileLine(mask)	((void)(::rage::_sysDmaWaitTagStatusAll(SYS_DMA_DSPEW_PARAMS mask SYS_DMA_FILELINE_PASS)))
#define sysDmaWait(mask)						((void)(::rage::_sysDmaWaitTagStatusAll(SYS_DMA_DSPEW_ARGS   mask SYS_DMA_FILELINE_PARAMS)))
#define sysDmaWaitFileLine(mask)				((void)(::rage::_sysDmaWaitTagStatusAll(SYS_DMA_DSPEW_PARAMS mask SYS_DMA_FILELINE_PASS)))
#define sysDmaWaitTagStatusAny(mask)			((void)(::rage::_sysDmaWaitTagStatusAny(SYS_DMA_DSPEW_ARGS   mask SYS_DMA_FILELINE_PARAMS)))
#define sysDmaWaitTagStatusAnyFileLine(mask)	((void)(::rage::_sysDmaWaitTagStatusAny(SYS_DMA_DSPEW_PARAMS mask SYS_DMA_FILELINE_PASS)))
#define sysDmaPoll(mask)						((void)(::rage::_sysDmaWaitTagStatusAny(SYS_DMA_DSPEW_ARGS   mask SYS_DMA_FILELINE_PARAMS)))
#define sysDmaPollFileLine(mask)				((void)(::rage::_sysDmaWaitTagStatusAny(SYS_DMA_DSPEW_PARAMS mask SYS_DMA_FILELINE_PASS)))

template<class T>
__forceinline T sysDmaGetTypeT(u32 ppu, u32 tag SYS_DMA_FILELINE_ARGS)
{
	qword buf;
	T* ls = (T*)sysDmaEa2Ls(ppu, &buf);
	::rage::_sysDmaSmallAndWait(SYS_DMA_GET, ls, ppu, sizeof(T), tag SYS_DMA_FILELINE_PASS);
	return *ls;
}
template<class T>
__forceinline void sysDmaPutTypeT(T value, u32 ppu, u32 tag SYS_DMA_FILELINE_ARGS)
{
	qword buf = (qword)spu_splats(value);
	::rage::_sysDmaSmallAndWait(SYS_DMA_PUT, sysDmaEa2Ls(ppu, &buf), ppu, sizeof(T), tag SYS_DMA_FILELINE_PASS);
}
#define sysDmaGetType(type,mask, ...)		((void)(sysValidateDmaTyped(mask,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)), \
												::rage::sysDmaGetTypeT<type>(__VA_ARGS__ SYS_DMA_FILELINE_PARAMS))
#define sysDmaGetTypeFileLine(type,mask, ...)		((void)(sysValidateDmaTyped(mask,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)), \
												::rage::sysDmaGetTypeT<type>(__VA_ARGS__ SYS_DMA_FILELINE_PARAMS))
#define sysDmaPutType(type,mask,val,...)	((void)(sysValidateDmaTyped(mask,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)), \
												::rage::sysDmaPutTypeT<type>(val, __VA_ARGS__ SYS_DMA_FILELINE_PARAMS))
#define sysDmaPutTypeFileLine(type,mask,val,...)	((void)(sysValidateDmaTyped(mask,__VA_ARGS__ SYS_DMA_FILELINE_PARAMS)), \
												::rage::sysDmaPutTypeT<type>(val, __VA_ARGS__ SYS_DMA_FILELINE_PARAMS))

#define sysDmaWaitTagStatusImmediate		((void)(::rage::_sysDmaWaitTagStatusImmediate(SYS_DMA_DSPEW_ARGS ::rage::sysValidateDmaMask(mask SYS_DMA_FILELINE_PARAMS))))

//////////////////////////////////////////////////////////////////////////
// DMA Commands

// void sysDmaGet(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaGet(ls,ppu,size,tag)          (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaGetFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaGet(ls,ppu,size,tag)          sysDma(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#define sysDmaGetFileLine(ls,ppu,size,tag)	sysDmaFileLine(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#endif
// void sysDmaPut(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaPut(ls,ppu,size,tag)          (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaPutFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaPut(ls,ppu,size,tag)          sysDma(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#define sysDmaPutFileLine(ls,ppu,size,tag)	sysDmaFileLine(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#endif
// void sysDmaGetf(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaGetf(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaGetfFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaGetf(ls,ppu,size,tag)         sysDma(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#define sysDmaGetfFileLine(ls,ppu,size,tag)	sysDmaFileLine(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaPutf(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaPutf(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaPutfFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaPutf(ls,ppu,size,tag)         sysDma(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#define sysDmaPutfFileLine(ls,ppu,size,tag) sysDmaFileLine(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaGetb(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaGetb(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETB, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaGetbFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETB, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaGetb(ls,ppu,size,tag)         sysDma(SYS_DMA_GETB,(ls),(ppu),(size),(tag))
#define sysDmaGetbFileLine(ls,ppu,size,tag) sysDmaFileLine(SYS_DMA_GETB,(ls),(ppu),(size),(tag))
#endif
// void sysDmaPutb(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaPutb(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTB, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaPutbFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTB, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaPutb(ls,ppu,size,tag)         sysDma(SYS_DMA_PUTB,(ls),(ppu),(size),(tag))
#define sysDmaPutbFileLine(ls,ppu,size,tag)	sysDmaFileLine(SYS_DMA_PUTB,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallGet(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallGet(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallGetFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallGet(ls,ppu,size,tag)     sysDmaSmall(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#define sysDmaSmallGetFileLine(ls,ppu,size,tag)     sysDmaSmallFileLine(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallPut(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallPut(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallPutFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallPut(ls,ppu,size,tag)     sysDmaSmall(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#define sysDmaSmallPutFileLine(ls,ppu,size,tag)     sysDmaSmallFileLine(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallGetf(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallGetf(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallGetfFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallGetf(ls,ppu,size,tag)    sysDmaSmall(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#define sysDmaSmallGetfFileLine(ls,ppu,size,tag)    sysDmaSmallFileLine(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallPutf(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallPutf(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallPutfFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallPutf(ls,ppu,size,tag)    sysDmaSmall(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#define sysDmaSmallPutfFileLine(ls,ppu,size,tag)    sysDmaSmallFileLine(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallGetb(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallGetb(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETB, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallGetbFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETB, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallGetb(ls,ppu,size,tag)    sysDmaSmall(SYS_DMA_GETB,(ls),(ppu),(size),(tag))
#define sysDmaSmallGetbFileLine(ls,ppu,size,tag)    sysDmaSmallFileLine(SYS_DMA_GETB,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallPutb(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallPutb(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTB, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallPutbFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTB, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallPutb(ls,ppu,size,tag)    sysDmaSmall(SYS_DMA_PUTB,(ls),(ppu),(size),(tag))
#define sysDmaSmallPutbFileLine(ls,ppu,size,tag)    sysDmaSmallFileLine(SYS_DMA_PUTB,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargeGet(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargeGet(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargeGetFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargeGet(ls,ppu,size,tag)     sysDmaLarge(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#define sysDmaLargeGetFileLine(ls,ppu,size,tag)     sysDmaLargeFileLine(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargePut(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargePut(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargePutFileLine(ls,ppu,size,tag)	(::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargePut(ls,ppu,size,tag)     sysDmaLarge(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#define sysDmaLargePutFileLine(ls,ppu,size,tag)     sysDmaLargeFileLine(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargeGetf(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargeGetf(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargeGetfFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargeGetf(ls,ppu,size,tag)    sysDmaLarge(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#define sysDmaLargeGetfFileLine(ls,ppu,size,tag)    sysDmaLargeFileLine(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargeputf(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargePutf(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargePutfFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargePutf(ls,ppu,size,tag)    sysDmaLarge(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#define sysDmaLargePutfFileLine(ls,ppu,size,tag)    sysDmaLargeFileLine(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargeGetb(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargeGetb(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETB, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargeGetbFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETB, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargeGetb(ls,ppu,size,tag)    sysDmaLarge(SYS_DMA_GETB,(ls),(ppu),(size),(tag))
#define sysDmaLargeGetbFileLine(ls,ppu,size,tag)    sysDmaLargeFileLine(SYS_DMA_GETB,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargePutb(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargePutb(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTB, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargePutbFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTB, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargePutb(ls,ppu,size,tag)    sysDmaLarge(SYS_DMA_PUTB,(ls),(ppu),(size),(tag))
#define sysDmaLargePutbFileLine(ls,ppu,size,tag)    sysDmaLargeFileLine(SYS_DMA_PUTB,(ls),(ppu),(size),(tag))
#endif
// void sysDmaListGet(const void* ls, const mfc_list_element_t* list, u32 listsize, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListGet(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETL, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListGetFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETL, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListGet(ls,list,listsize,tag)      sysDmaList(SYS_DMA_GETL,(ls),(list),(listsize),(tag))
#define sysDmaListGetFileLine(ls,list,listsize,tag)      sysDmaListFileLine(SYS_DMA_GETL,(ls),(list),(listsize),(tag))
#endif
// void sysDmaListPut(const void* ls, const mfc_list_element_t* list, u32 listsize, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListPut(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTL, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListPutFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTL, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListPut(ls,list,listsize,tag)      sysDmaList(SYS_DMA_PUTL,(ls),(list),(listsize),(tag))
#define sysDmaListPutFileLine(ls,list,listsize,tag)      sysDmaListFileLine(SYS_DMA_PUTL,(ls),(list),(listsize),(tag))
#endif
// void sysDmaListGetf(const void* ls, const mfc_list_element_t* list, u32 listsize, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListGetf(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETLF, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListGetfFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETLF, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListGetf(ls,list,listsize,tag)     sysDmaList(SYS_DMA_GETLF,(ls),(list),(listsize),(tag))
#define sysDmaListGetfFileLine(ls,list,listsize,tag)     sysDmaListFileLine(SYS_DMA_GETLF,(ls),(list),(listsize),(tag))
#endif
// void sysDmaListPutf(const void* ls, const mfc_list_element_t* list, u32 listsize, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListPutf(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTLF, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListPutfFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTLF, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListPutf(ls,list,listsize,tag)     sysDmaList(SYS_DMA_PUTLF,(ls),(list),(listsize),(tag))
#define sysDmaListPutfFileLine(ls,list,listsize,tag)     sysDmaListFileLine(SYS_DMA_PUTLF,(ls),(list),(listsize),(tag))
#endif
// void sysDmaListGetb(const void* ls, const mfc_list_element_t* list, u32 listsize, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListGetb(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_GETLB, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListGetbFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_GETLB, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListGetb(ls,list,listsize,tag)     sysDmaList(SYS_DMA_GETLB,(ls),(list),(listsize),(tag))
#define sysDmaListGetbFileLine(ls,list,listsize,tag)     sysDmaListFileLine(SYS_DMA_GETLB,(ls),(list),(listsize),(tag))
#endif
// void sysDmaListPutb(const void* ls, const mfc_list_element_t* list, u32 listsize, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListPutb(ls,ppu,size,tag)         (::rage::AddDelayedDMA(SYS_DMA_ARGS SYS_DMA_PUTLB, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListPutbFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMA(SYS_DMA_PARAMS SYS_DMA_PUTLB, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListPutb(ls,list,listsize,tag)     sysDmaList(SYS_DMA_PUTLB,(ls),(list),(listsize),(tag))
#define sysDmaListPutbFileLine(ls,list,listsize,tag)     sysDmaListFileLine(SYS_DMA_PUTLB,(ls),(list),(listsize),(tag))
#endif


// u8 sysDmaGetUInt8(u32 ppu, u32 tag)
#define sysDmaGetUInt8(ppu,tag)			sysDmaGetType(u8,0,(ppu),(tag))
#define sysDmaGetUInt8FileLine(ppu,tag)			sysDmaGetTypeFileLine(u8,0,(ppu),(tag))
// u16 sysDmaGetUInt16(u32 ppu, u32 tag)
#define sysDmaGetUInt16(ppu,tag)		sysDmaGetType(u16,1,(ppu),(tag))
#define sysDmaGetUInt16FileLine(ppu,tag)		sysDmaGetTypeFileLine(u16,1,(ppu),(tag))
// u32 sysDmaGetUInt32(u32 ppu, u32 tag)
#define sysDmaGetUInt32(ppu,tag)		sysDmaGetType(u32,3,(ppu),(tag))
#define sysDmaGetUInt32FileLine(ppu,tag)		sysDmaGetTypeFileLine(u32,3,(ppu),(tag))
// u64 sysDmaGetUInt64(u32 ppu, u32 tag)
#define sysDmaGetUInt64(ppu,tag)		sysDmaGetType(u64,7,(ppu),(tag))
#define sysDmaGetUInt64FileLine(ppu,tag)		sysDmaGetTypeFileLine(u64,7,(ppu),(tag))
// void sysDmaPutUInt8(u8 val, u32 ppu, u32 tag)
#define sysDmaPutUInt8(val,ppu,tag)			sysDmaPutType(u8,0,(val),(ppu),(tag))
#define sysDmaPutUInt8FileLine(val,ppu,tag)			sysDmaPutTypeFileLine(u8,0,(val),(ppu),(tag))
// void sysDmaPutUInt16(u16 val, u32 ppu, u32 tag)
#define sysDmaPutUInt16(val,ppu,tag)		sysDmaPutType(u16,1,(val),(ppu),(tag))
#define sysDmaPutUInt16FileLine(val,ppu,tag)		sysDmaPutTypeFileLine(u16,1,(val),(ppu),(tag))
// void sysDmaPutUInt32(u32 val, u32 ppu, u32 tag)
#define sysDmaPutUInt32(val,ppu,tag)		sysDmaPutType(u32,3,(val),(ppu),(tag))
#define sysDmaPutUInt32FileLine(val,ppu,tag)		sysDmaPutTypeFileLine(u32,3,(val),(ppu),(tag))
// void sysDmaPutUInt64(u64 val, u32 ppu, u32 tag)
#define sysDmaPutUInt64(val,ppu,tag)		sysDmaPutType(u64,7,(val),(ppu),(tag))
#define sysDmaPutUInt64FileLine(val,ppu,tag)		sysDmaPutTypeFileLine(u64,7,(val),(ppu),(tag))


// void sysDmaGetAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaGetAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaGetAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaGetAndWait(ls,ppu,size,tag)		sysDmaAndWait(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#define sysDmaGetAndWaitFileLine(ls,ppu,size,tag)		sysDmaAndWaitFileLine(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallGet(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallGetAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallGetAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallGetAndWait(ls,ppu,size,tag)  sysDmaSmallAndWait(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#define sysDmaSmallGetAndWaitFileLine(ls,ppu,size,tag)  sysDmaSmallAndWaitFileLine(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargeGetAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargeGetAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargeGetAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_GET, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargeGetAndWait(ls,ppu,size,tag)	sysDmaLargeAndWait(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#define sysDmaLargeGetAndWaitFileLine(ls,ppu,size,tag)	sysDmaLargeAndWaitFileLine(SYS_DMA_GET,(ls),(ppu),(size),(tag))
#endif
// void sysDmaListGetAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListGetAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_GETL, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListGetAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_GETL, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListGetAndWait(ls,list,listsize,tag)	sysDmaListAndWait(SYS_DMA_GETL,(ls),(list),(listsize),(tag))
#define sysDmaListGetAndWaitFileLine(ls,list,listsize,tag)	sysDmaListAndWaitFileLine(SYS_DMA_GETL,(ls),(list),(listsize),(tag))
#endif
// void sysDmaPutAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaPutAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaPutAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaPutAndWait(ls,ppu,size,tag)		sysDmaAndWait(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#define sysDmaPutAndWaitFileLine(ls,ppu,size,tag)		sysDmaAndWaitFileLine(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallPutAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallPutAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallPutAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallPutAndWait(ls,ppu,size,tag)  sysDmaSmallAndWait(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#define sysDmaSmallPutAndWaitFileLine(ls,ppu,size,tag)  sysDmaSmallAndWaitFileLine(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargePutAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargePutAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargePutAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_PUT, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargePutAndWait(ls,ppu,size,tag)	sysDmaLargeAndWait(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#define sysDmaLargePutAndWaitFileLine(ls,ppu,size,tag)	sysDmaLargeAndWaitFileLine(SYS_DMA_PUT,(ls),(ppu),(size),(tag))
#endif
// void sysDmaListPutAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListPutAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_PUTL, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListPutAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_PUTL, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListPutAndWait(ls,list,listsize,tag)	sysDmaListAndWait(SYS_DMA_PUTL,(ls),(list),(listsize),(tag))
#define sysDmaListPutAndWaitFileLine(ls,list,listsize,tag)	sysDmaListAndWaitFileLine(SYS_DMA_PUTL,(ls),(list),(listsize),(tag))
#endif

// void sysDmaGetfAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaGetfAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaGetfAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaGetfAndWait(ls,ppu,size,tag)		sysDmaAndWait(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#define sysDmaGetfAndWaitFileLine(ls,ppu,size,tag)		sysDmaAndWaitFileLine(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallGetfAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallGetfAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallGetfAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallGetfAndWait(ls,ppu,size,tag)  sysDmaSmallAndWait(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#define sysDmaSmallGetfAndWaitFileLine(ls,ppu,size,tag)  sysDmaSmallAndWaitFileLine(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargeGetfAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargeGetfAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargeGetfAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_GETF, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargeGetfAndWait(ls,ppu,size,tag)	sysDmaLargeAndWait(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#define sysDmaLargeGetfAndWaitFileLine(ls,ppu,size,tag)	sysDmaLargeAndWaitFileLine(SYS_DMA_GETF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaListGetfAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListGetfAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_GETLF, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListGetfAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_GETLF, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListGetfAndWait(ls,list,listsize,tag)	sysDmaListAndWait(SYS_DMA_GETLF,(ls),(list),(listsize),(tag))
#define sysDmaListGetfAndWaitFileLine(ls,list,listsize,tag)	sysDmaListAndWaitFileLine(SYS_DMA_GETLF,(ls),(list),(listsize),(tag))
#endif
// void sysDmaPutfAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaPutfAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#define sysDmaPutfAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMA))
#else
#define sysDmaPutfAndWait(ls,ppu,size,tag)		sysDmaAndWait(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#define sysDmaPutfAndWaitFileLine(ls,ppu,size,tag)		sysDmaAndWaitFileLine(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaSmallPutfAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaSmallPutfAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#define sysDmaSmallPutfAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMASMALL))
#else
#define sysDmaSmallPutfAndWait(ls,ppu,size,tag)  sysDmaSmallAndWait(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#define sysDmaSmallPutfAndWaitFileLine(ls,ppu,size,tag)  sysDmaSmallAndWaitFileLine(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaLargePutfAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaLargePutfAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#define sysDmaLargePutfAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_PUTF, (ls), (ppu), (size), (tag), ::rage::SYSDMALARGE))
#else
#define sysDmaLargePutfAndWait(ls,ppu,size,tag)	sysDmaLargeAndWait(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#define sysDmaLargePutfAndWaitFileLine(ls,ppu,size,tag)	sysDmaLargeAndWaitFileLine(SYS_DMA_PUTF,(ls),(ppu),(size),(tag))
#endif
// void sysDmaListPutfAndWait(const void* ls, u32 ppu, u32 size, u32 tag)
#if SYS_DMA_DELAYED
#define sysDmaListPutfAndWait(ls,ppu,size,tag)         (::rage::AddDelayedDMAAndFlush(SYS_DMA_ARGS SYS_DMA_PUTLF, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#define sysDmaListPutfAndWaitFileLine(ls,ppu,size,tag) (::rage::AddDelayedDMAAndFlush(SYS_DMA_PARAMS SYS_DMA_PUTLF, (ls), (rage::u32)(ppu), (size), (tag), ::rage::SYSDMALIST))
#else
#define sysDmaListPutfAndWait(ls,list,listsize,tag)	sysDmaListAndWait(SYS_DMA_PUTLF,(ls),(list),(listsize),(tag))
#define sysDmaListPutfAndWaitFileLine(ls,list,listsize,tag)	sysDmaListAndWaitFileLine(SYS_DMA_PUTLF,(ls),(list),(listsize),(tag))
#endif

// If you are wondering where the barrier versions are for the sysDma*AndWait
// macros, they are unnecessary.  Because we are waiting on completion, there is
// not going to be any other dmas on the same tag (unless we do something crazy
// with interrupts, but seems unlikely they would ever use the same dma tags). 

} // namespace rage 

#endif		// __SPU

namespace rage {

#if __PS3

inline u64 sysDmaListMakeLargeEntry(const void *address,u32 size)
{
	Assertf(size<2 || (size==2 && !((u32)address&1)) || (size==4 && !((u32)address&3)) || 
		(size==8 && !((u32)address&7)) || (size>=16 && !(size&15) && !((u32)address&15)),"Invalid address %p",address);
	return (u64)address | ((u64)size << 32);	
}

inline u64 sysDmaListMakeEntry(const void *address,u32 size)
{
	Assertf(size <= 16384, "Invalid size %x",size);
	return sysDmaListMakeLargeEntry(address,size);
}

inline u64 sysDmaListComputeSize(const u64 *elements,u32 sizeElements)
{
	u32 size = 0;
	while (sizeElements) 
	{
		size += (u32)((*elements++ >> 32) + 15) & ~15;
		sizeElements -= 8;
	}
	return size;
}

#endif // __PS3

#define DMA_TAG(xfer) (xfer + 1)
#define DMA_MASK(xfer) (1 << DMA_TAG(xfer))

}	// namespace rage

// This is terrible, but guarantees that spu_library is always seeing the same settings.
#include "system/spu_library.cpp"

#endif // SYSTEM_DMA_H
