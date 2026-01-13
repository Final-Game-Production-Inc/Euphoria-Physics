// 
// system/codefrag_spu.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_CODEFRAG_SPU_H 
#define SYSTEM_CODEFRAG_SPU_H 

#if __SPU
#include "ppu_symbol.h"
#endif

// The size of the .SpuGUID section.
#define SPUGUID_SIZE_BYTES	16
#define SPUGUID_SIZE_WORDS	4

namespace rage
{

// Declare the symbols that are needed to access the spu code fragment from the PPU .elf.

#if __SPU
#define DECLARE_FRAG_INTERFACE(name)	DECLARE_PPU_SYMBOL(char, _binary_spu_##name##_frag_bin_start); DECLARE_PPU_SYMBOL(char, _binary_spu_##name##_frag_bin_size); DECLARE_PPU_SYMBOL(char, _binary_spu_##name##_frag_bin_end);
#define FRAG_INTERFACE_START(name)		PPU_SYMBOL(_binary_spu_##name##_frag_bin_start)
#define FRAG_INTERFACE_SIZE(name)		PPU_SYMBOL(_binary_spu_##name##_frag_bin_size)
#define FRAG_INTERFACE_END(name)		PPU_SYMBOL(_binary_spu_##name##_frag_bin_end)
#else
#define DECLARE_FRAG_INTERFACE(name)	extern char _binary_spu_##name##_frag_bin_start[], _binary_spu_##name##_frag_bin_size[], _binary_spu_##name##_frag_bin_end[];
#define FRAG_INTERFACE_START(name)		(_binary_spu_##name##_frag_bin_start)
#define FRAG_INTERFACE_SIZE(name)		(_binary_spu_##name##_frag_bin_size)
#define FRAG_INTERFACE_END(name)		(_binary_spu_##name##_frag_bin_end)
#endif

// Declare a code fragment:
//
// SPUFRAG_DECL(int, int, int);
//
// ==
//
// "int SpuShaderMain( int, int )" // (need a fixed function name for entry point)
//
#if __SPU
#define SPUFRAG_DECL(returntype, name, ...) extern "C" returntype SpuShaderMain( __VA_ARGS__ ) __attribute__((used)) __attribute__((noinline)) __attribute__((section(".before_text")));
#else
#define SPUFRAG_DECL(returntype, name, ...) returntype name(__VA_ARGS__);
#endif

// Implement a code fragment:
//
// SPUFRAG_IMPL(int, int a, int b)
// {
//		return a + b;
// }
//
// ==
//
// "int SpuShaderMain( int a, int b ) // (need a fixed function name for entry point)
//	{
//		return a + b;
//	}"
//
#if __SPU
#define SPUFRAG_IMPL(returntype, name, ...) extern "C" returntype SpuShaderMain( __VA_ARGS__ )
#else
#define SPUFRAG_IMPL(returntype, name, ...) returntype name(__VA_ARGS__)
#endif

// A structure for PPU that encompasses:
//	1) The PPU address of the code fragment.
//	2) The size of the code fragment for DMA purposes.
//
// The former should be a multiple of 16 bytes. Assert for it.
// If the latter is not a multiple of 16 bytes, be sure to rage::RoundUp<16>(size) before DMA'ing.
struct spuCodeFragment
{
	spuCodeFragment() {}
	spuCodeFragment(u32* a_pCodeFrag, u32 a_codeFragSize) : pCodeFrag(a_pCodeFrag), codeFragSize(a_codeFragSize) {}
	u32* pCodeFrag;
	u32 codeFragSize;
	// Note that you should jump into pCodeFrag+4 to jump over the 4 words that make up the SPU GUID...
	// though it doesn't actually hurt to execute those 4 instructions, as they turn out to be harmless.
};

#if __PS3
#define FRAG_SPU_CODE(name) spuCodeFragment((u32*)FRAG_INTERFACE_START(name), (u32)FRAG_INTERFACE_SIZE(name))
#else
#define FRAG_SPU_CODE(name) spuCodeFragment((u32*)name, 0)
#endif

} // namespace rage

#endif // SYSTEM_CODEFRAG_SPU_H
