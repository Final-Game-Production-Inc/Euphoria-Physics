// 
// system/buddyallocator_config.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_BUDDYALLOCATOR_CONFIG_H
#define SYSTEM_BUDDYALLOCATOR_CONFIG_H

#define RESOURCE_POOLS 0		// was PS3 || XENON

#define ENABLE_BUDDY_ALLOCATOR RSG_PC

#if defined(__RGSC_DLL) && __RGSC_DLL
	// the sparse allocator breaks the social club DLL
	#define USE_SPARSE_MEMORY (0)
	#define FREE_PHYSICAL_RESOURCES (1)
#else // defined(__RGSC_DLL) && __RGSC_DLL
	#define USE_SPARSE_MEMORY (1 && !__RESOURCECOMPILER && !__TOOL && RSG_PC && __64BIT && __DEV)
	#define FREE_PHYSICAL_RESOURCES (1 && /*!USE_SPARSE_MEMORY &&*/ !__RESOURCECOMPILER && !__TOOL && RSG_PC)
#endif // defined(__RGSC_DLL) && __RGSC_DLL

// Here there be dragons...
// The following code will not discard non-ref memory blocks from defrag consideration
// Basically, defragging such blocks is a waste of time
#define ENABLE_DEFRAG_CALLBACK (0)
#define ENABLE_DEFRAG_DEBUG (0)

#if ENABLE_DEFRAG_CALLBACK
#define ENABLE_DEFRAG_CALLBACK_ONLY(x) x
typedef bool (*IsObjectReadyToDelete)(void* ptr);
#else
#define ENABLE_DEFRAG_CALLBACK_ONLY(x)
#endif

#endif
