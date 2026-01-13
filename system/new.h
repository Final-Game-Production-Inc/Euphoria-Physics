// 
// system/new.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_NEW_H
#define SYSTEM_NEW_H

#include <new>

#include "system/new_config.h"

// Note: Be careful of using new CMyClass[n] with supposedly 16-byte-aligned classes on __WIN32PC, you will get a misaligned
// SSE vector move exception (if you're lucky enough to be using SSE and have it catch the misalignment for you at runtime)!
//
// For a temporary workaround, use either _mm_malloc() or _aligned_malloc() (same things) along with placement new.

void* operator new(size_t size,size_t align);
void* operator new[](size_t size,size_t align);

#if RAGE_ENABLE_RAGE_NEW

void* operator new(size_t size,const char *file,int line);
void* operator new[](size_t size,const char *file,int line);
void* operator new(size_t size,size_t align,const char *file,int line);
void* operator new[](size_t size,size_t align,const char *file,int line);

#if __PS3 // special version for gcc when the rage_aligned_new is called on a class that is already ALIGNED
void* operator new(size_t size,size_t alignType,size_t alignParam,const char *file,int line);
void* operator new[](size_t size,size_t alignType,size_t alignParam,const char *file,int line);
#endif

// Search for the following string: \bnew{[ \t]+[:_a-zA-Z]} with regex's enabled.
// Replace it with rage_new\1 to fix code to call rage_new.
#define rage_new					new(__FILE__,__LINE__)
#define rage_aligned_new(a)			new((size_t)(a),__FILE__,__LINE__)
#define rage_alignof_new(a)			new(__alignof(a),__FILE__,__LINE__)
#define rage_heap_new(h)			new((h),__FILE__,__LINE__)
#define rage_heap_aligned_new(h,a)	new((h),(size_t)(a),__FILE__,__LINE__)



#else

#if __PS3 // special version for gcc when the rage_aligned_new is called on a class that is already ALIGNED
void* operator new(size_t size,size_t alignType,size_t alignParam);
void* operator new[](size_t size,size_t alignType,size_t alignParam);
#endif

#define rage_new					new
#define rage_aligned_new(a)			new((size_t)(a))
#define rage_alignof_new(a)			new(__alignof(a))
#define rage_heap_new(h)			new(h)
#define rage_heap_aligned_new(h,a)	new((h),(size_t)(a))

#endif // !defined(__MFC) && __DEV && !__TOOL && !__SPU

#define rage_placement_new(p)		::new(p)

#endif
