//
// system/llsc_xbox360.cpp
//
// Copyright (C) 2012-2012 Rockstar Games.  All Rights Reserved.
//

//
// Got permission from GameDS (CAS-37188-ZYBX53) to use the Load Linked/Store
// Conditional CPU instructions.  There is a hardware bug in the 360 that
// requires interrupts to be disabled before the L[WD]ARX and re-enabled after
// the corresponding ST[WD]CX.  Microsoft recommended following the same
// approach as used by their intrinsics like _InterlockedCompareExchange.  That
// is, to store the previous MSR value somewhere, load r13 to MSR.EE/MSR.RI, do
// the atomic operations, then restore the previous MSR.EE/MSR.RI values.  Note
// that the MTMSREE instruction seems to be a Microsoft specific pneumonic for
// the MTMSRD L=1 instruction.
//

#if __XENON

// NOTE: We purposely do NOT include "llsc.h" here.  These functions do return a
// value, but due to the lame way __declspec(naked) works, we need to pretend
// that they don't here.

#include <PPCIntrinsics.h>

extern "C" __declspec(naked noinline) void/*rage::u32*/ sys_360_lwarx(const volatile rage::u32 * /*mem*/, rage::u64 * /*msr*/)
{
	__emit(0x7ca000a6);     // mfmsr    r5          011111 00101 00000 00000  0001010011 0
	__emit(0x7ca0212a);     // stdx     r5, r0, r4  011111 00101 00000 00100  0010010101 0
	__emit(0x7da10164);     // mtmsree  r13         011111 01101 00001 0000 0 0010110010 0
	__emit(0x7c601828);     // lwarx    r3, r0, r3  011111 00011 00000 00011  0000010100 0
	__emit(0x4e800020);     // blr                  010011 10100 00000 000 00 0000010000 0
}

extern "C" __declspec(naked noinline) void/*rage::u64*/ sys_360_ldarx(const volatile rage::u64 * /*mem*/, rage::u64 * /*msr*/)
{
	__emit(0x7ca000a6);     // mfmsr    r5          011111 00101 00000 00000  0001010011 0
	__emit(0x7ca0212a);     // stdx     r5, r0, r4  011111 00101 00000 00100  0010010101 0
	__emit(0x7da10164);     // mtmsree  r13         011111 01101 00001 0000 0 0010110010 0
	__emit(0x7c6018a8);     // ldarx    r3, r0, r3  011111 00011 00000 00011  0001010100 0
	__emit(0x4e800020);     // blr                  010011 10100 00000 000 00 0000010000 0
}

extern "C" __declspec(naked noinline) void/*bool*/ sys_360_stwcx(rage::u32 /*val*/, volatile rage::u32 * /*mem*/, rage::u64 /*msr*/)
{
	__emit(0x7c80192d);     // stwcx.   r4, r0, r3  011111 00100 00000 00011  0010010110 1
	__emit(0x7ca10164);     // mtmsree  r5          011111 00101 00001 0000 0 0010110010 0
	__emit(0x38600001);     // li       r3, 1       001110 00011 00000 0000000000000001
	__emit(0x4de20020);     // beqlr+               010011 01111 00010 000 00 0000010000 0
	__emit(0x38600000);     // li       r3, 0       001110 00011 00000 0000000000000000
	__emit(0x4e800020);     // blr                  010011 10100 00000 000 00 0000010000 0
}

extern "C" __declspec(naked noinline) void/*bool*/ sys_360_stdcx(rage::u64 /*val*/, volatile rage::u64 * /*mem*/, rage::u64 /*msr*/)
{
	__emit(0x7c8019ad);     // stdcx.   r4, r0, r3  011111 00100 00000 00011  0011010110 1
	__emit(0x7ca10164);     // mtmsree  r5          011111 00101 00001 0000 0 0010110010 0
	__emit(0x38600001);     // li       r3, 1       001110 00011 00000 0000000000000001
	__emit(0x4de20020);     // beqlr+               010011 01111 00010 000 00 0000010000 0
	__emit(0x38600000);     // li       r3, 0       001110 00011 00000 0000000000000000
	__emit(0x4e800020);     // blr                  010011 10100 00000 000 00 0000010000 0
}

#endif // __XENON
