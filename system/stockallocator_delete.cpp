// 
// system/stock_newdelete.cpp 
// 
// Copyright (C) 1999-2011 Rockstar Games.  All Rights Reserved. 
// 

#if __TOOL

#if __WIN32 && !__OPTIMIZED
#include <crtdbg.h>
#endif
#if __WIN32
#include <malloc.h>		// for non-standard _msize function
#elif __PPU
#include <stdlib.h>
extern "C" size_t malloc_usable_size(void*);
#else
#include <stdlib.h>
#endif

void operator delete(void *ptr) {
	if (ptr) _aligned_free(ptr);
}

#endif