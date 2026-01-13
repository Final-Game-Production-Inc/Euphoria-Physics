//
// system/debugmemoryfill.h
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_MEMORYFILL_H
#define SYSTEM_MEMORYFILL_H

#define SUPPORT_DEBUG_MEMORY_FILL	 (!__FINAL && !__PROFILE)

#if SUPPORT_DEBUG_MEMORY_FILL
#define DMF_DEFRAG_FREE		0x01
#define DMF_GAMEHEAP_FREE	0x02		// includes smallocator
#define DMF_GAMEHEAP_ALLOC	0x04		// includes smallocator
#define DMF_FRAGCACHE		0x08		// fragcache alloc and free
#define DMF_AUDIOPOOL		0x10		// audio pool alloc and free
#define DMF_GAMEPOOL		0x20		// game pool alloc and free
#define DMF_RESOURCE		0x40		// resource heap alloc (initial only on non-PS3) and free
#define DMF_ASYNCPOOL		0x80		// async pool alloc and free

// Defaults to true on __DEV, false otherwise.  Cannot be in rage namespace due to limitations on PPU symbol binding
extern unsigned char g_EnableDebugMemoryFill;
#define IF_DEBUG_MEMORY_FILL_N(x,n)	do { if (g_EnableDebugMemoryFill & (n)) { x; } } while(0)
#else
#define IF_DEBUG_MEMORY_FILL_N(x,n)	
#endif

#endif	// SYSTEM_MEMORYFILL_H
