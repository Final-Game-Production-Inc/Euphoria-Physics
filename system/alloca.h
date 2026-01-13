//
// system/alloca.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_ALLOCA_H
#define SYSTEM_ALLOCA_H

#if __WIN32
#include <malloc.h>
#define RageAlloca alloca
#elif __PS3 || __PSP2
#include <alloca.h>
#include <stddef.h>		// for size_t
	#ifdef __SNC__
	extern "C" {
	extern void* alloca(unsigned);
	}
	#endif
	#define RageAlloca alloca
#else
#include <stdlib.h>
#define RageAlloca alloca
#endif

// This is just a helper for the Alloca*() macros.
template <typename _Type>
_Type* AllocaAlignAssert(_Type* addr)
{
	// Let's make sure we were aligned even before our manual adjustment of "+ ((__align)-1)) & ~((__align)-1))" in AllocaAligned(), as a check.
	AssertMsg( ((((size_t)addr) & (__alignof(_Type)-1)) == 0), "alloca() did not align to the alignment requirement of the type!" );

	return addr;
}

// PURPOSE
//   Allocate memory on the stack
// NOTES
//   1. The memory returned is automatically reclaimed when the current function returns
//   2. If the stack is overrun (too much is allocated), the program can malfuntion without
//      warning, so use at your own risk! Sometimes you'll get a chkesp error at function
//      termination when that happens, but sometimes you'll just get invalid results with
//      no warning.
//	 3. We always allocate a multiple of 16 bytes from the stack, so that we don't end up
//		with an unaligned stack pointer for the rest of the function
//   4. We actually allocate "__align" + the alignment we need. So if we need 128 byte alignment
//		we allocate an extra 128 bytes, so that we know there's an aligned address in there 
//		somewhere that we can start on without overwriting the stack on the end
#define Alloca(__type,__num)				(__type*)AllocaAlignAssert( (__type*)RageAlloca((((__num)*sizeof(__type)) + 15) & ~15) )
#define AllocaAligned(__type,__num,__align)	(__type*)																		\
											(																				\
												(																			\
													(size_t)RageAlloca(														\
														((__num)*sizeof(__type) + __align + 15) & ~15						\
													)																		\
													+																		\
													((__align)-1)															\
												)																			\
												&																			\
												~((__align)-1)																\
											)

#endif // SYSTEM_ALLOCA_H

