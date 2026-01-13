//
// system/tmcommands_ms.cpp
//
// Copyright (C) 2013-2015 Rockstar Games.  All Rights Reserved.
//

// This is a ridiculously evil hack, all because our build system makes it near
// impossible to add asm files that are assembled only on a single platform.
// Yes, thats right, we are forcing arrays of bytes into the code segment, then
// using extern "C" so we can fool the linker into calling them :(
//
// If at some point in the future this stops working (for example, maybe the
// extern "C" no longer fools the linker?), then PG3 will need to be beat into
// submission so that we can add assembler files, and have them platform
// specific.  The .asm code should be something along the lines of..
//
//------------------------------------------------------------------------------
// _text segment
//
// TMCMD_HEADER macro name:req
// ?&name&@rage@@YAXXZ proc
// 		int     3
// 		jmp     short @F
// 		db      'R','*','T','M'
// endm
//
// TMCMD_FOOTER macro name:req
// 	@@:
//		; Some single byte nops (90h) to help disassembly start
// 		; correctly again on an instruction boundary
// 		db      10h dup(90h)
// 		ret
// ?&name&@rage@@YAXXZ endp
// endm
//
// ; void __cdecl rage::sysTmCmdNop(void)
// TMCMD_HEADER sysTmCmdNop
// 		db      0       ; command opcode
// TMCMD_FOOTER sysTmCmdNop
//
// ; ...
//
// _text ends
//
// end
//------------------------------------------------------------------------------
//

#if RSG_DURANGO || RSG_PC

// Kludge to get the SYSTMCMD_ENABLE define out of the tmcommands.h header
// without getting anything else.
#define JUST_DEFINE_SYSTMCMD_ENABLE
#include "tmcommands.h"
#undef JUST_DEFINE_SYSTMCMD_ENABLE

#if SYSTMCMD_ENABLE

#pragma code_seg(".text.systmcmd")

using namespace rage;

// Helpers used for determining the offset for jmp near in the sysTmCmd*
// function.  Use sizeof(CountBytes(...)) to count the number of arguments.
struct{u8 b[1];}  CountBytes(u8);
struct{u8 b[2];}  CountBytes(u8,u8);
struct{u8 b[3];}  CountBytes(u8,u8,u8);
struct{u8 b[4];}  CountBytes(u8,u8,u8,u8);
struct{u8 b[5];}  CountBytes(u8,u8,u8,u8,u8);
struct{u8 b[6];}  CountBytes(u8,u8,u8,u8,u8,u8);
struct{u8 b[7];}  CountBytes(u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[8];}  CountBytes(u8,u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[9];}  CountBytes(u8,u8,u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[10];} CountBytes(u8,u8,u8,u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[11];} CountBytes(u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[12];} CountBytes(u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[13];} CountBytes(u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[14];} CountBytes(u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[15];} CountBytes(u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8);
struct{u8 b[16];} CountBytes(u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8,u8);

// Macro for creating a sysTmCmd* function.
// NAME - function name
// CMD  - command opcode
// ...  - optional additional data to be embedded after the opcode
#define TMCMD(NAME, CMD, ...)                                                  \
	                                                                           \
	/* Ensure that the offset can be encoded in a signed 8-bit value for    */ \
	/* the jmp short.  Note that the CompileTimeAssert macro does not work  */ \
	/* in this context, so roll our own here.                               */ \
	typedef u8 NAME##_CompileTimeAssert[                                       \
	                 (4+sizeof(CountBytes(CMD,__VA_ARGS__)) <= 127) ? 1 : -1]; \
	                                                                           \
	/* "Function" is really an array of bytes in the .text segment          */ \
	extern "C" __declspec(allocate(".text.systmcmd"))                          \
	const u8 NAME[] =                                                          \
	{                                                                          \
		0xcc,                                           /*     int 3        */ \
		0xeb, 4+sizeof(CountBytes(CMD,__VA_ARGS__)),    /*     jmp short @F */ \
		'R','*','T','M',                                /*     ; signagure  */ \
		CMD,                                            /*     ; cmd opcode */ \
		__VA_ARGS__,                                    /*     ; cmd data   */ \
		                                                /* @F:              */ \
		0xc3                                            /*     retn         */ \
	}

TMCMD(sysTmCmdNop_Internal,                         0x00);
TMCMD(sysTmCmdPrintAllThreadStacks_Internal,        0x01);
TMCMD(sysTmCmdStopNoErrorReport_Internal,           0x02);
TMCMD(sysTmCmdQuitf_Internal,                       0x03);
TMCMD(sysTmCmdGpuHang_Internal,                     0x04);
TMCMD(sysTmCmdCpuHang_Internal,                     0x05);
TMCMD(sysTmCmdConfig_Internal,                      0x06);
// 0x07 is sysTmCmdCreateDump
TMCMD(sysTmCmdExceptionHandlerBegin_Internal,       0x08);
TMCMD(sysTmCmdExceptionHandlerEnd_Internal,         0x09);

#if RSG_DURANGO
	// 0x80 and 0x81 are deprecated values for sysTmCmdExceptionHandlerBegin and sysTmCmdExceptionHandlerEnd
	TMCMD(sysTmCmdThreadBegin_Internal,                 0x82);
	TMCMD(sysTmCmdThreadEnd_Internal,                   0x83);
	TMCMD(sysTmCmdReportGpuHangHack_Internal,           0x84);
	TMCMD(sysTmCmdGetGpuCommandBufferAccess_Internal,   0x85);
#endif

#undef TMCMD

#endif // SYSTMCMD_ENABLE

#endif // RSG_PC || RSG_DURANGO
