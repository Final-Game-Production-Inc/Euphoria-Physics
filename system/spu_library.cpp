#if __SPU
#ifndef SYSTEM_SPU_LIBRARY_CPP
#define SYSTEM_SPU_LIBRARY_CPP
// #include "system/dma.h"
#include <cell/spurs/common.h>

#include "system/spu_printf_helper.h"

namespace rage {

class datResource;
datResource* datResource_sm_Current=NULL;

#if !__FINAL		// make sure this is all gone in final builds.

// used by SYS_DMA_BREAKPOINTS
const u8* g_DataBreakpoint = NULL;
u32 g_DataBreakpointSize = 0;
const u8* g_DataEABreakpoint = NULL;
u32 g_DataEABreakpointSize = 0;

// used by SYS_DMA_TIMING
u32 g_DmaWaitTime=0;

// used by SYS_DMA_VALIDATION>0
const char *sysDmaContext=NULL;

// used by SYS_DMA_VALIDATION==2
u32 g_sysLsNumWritableBlocks=0;
u32 g_sysLsWritableBlocks[28]={0};
u16 g_sysLsChecksums[192]={0};

// NOTE!  PPU-side server already adds the spu id before every printout now.
// Also, if the string starts with HALT: we'll echo it in a popup message box.

void sysDmaFail(const char *tag,u32 value) {
	register u32 stack_sp __asm__("1");
	RAGE_SPU_PRINTF("HALT: $%05x$ sysDmaAssert: bad value for %s: 0x%x during %s\n",stack_sp,tag,value,sysDmaContext); 
	__debugbreak(); 
	while(1);
}

bool sysDmaAssertFail(const char *tag,u32 value)
{
	register u32 stack_sp __asm__("1");
	RAGE_SPU_PRINTF("HALT: $%05x$ SPU DMA Assertion: bad %s: 0x%x during %s\n",	stack_sp, tag, value, sysDmaContext);
	__debugbreak(); 
	return false;
}

bool sysDmaAssertFailVerbose(const char* file,u32 line,const char *tag,u32 value)
{
	register u32 stack_sp __asm__("1");
	RAGE_SPU_PRINTF("HALT: $%05x$ SPU DMA Assertion: %s(%d) : bad %s: 0x%x during %s\n", stack_sp, file, line, tag, value, sysDmaContext);
	__debugbreak(); 
	return false;
}

int AssertFailed(const char *msg,const char *file,int line) {
	register u32 stack_sp __asm__("1");
	int result = RAGE_SPU_PRINTF("ASSERT: $%05x$ Assertion failed %s(%d): %s\n",stack_sp,file,line,msg);
	if (result != 1) {
		__debugbreak();
		while(1);
	}
	else
		return 0;
}

int DebugAssertFailed(void) {
	return false;
}

#endif	// !__FINAL

}	// namespace rage

#else
//	#error "spu_library.cpp has already been included."
#endif	// SYSTEM_SPU_LIBRARY_CPP...
#endif	// __SPU

