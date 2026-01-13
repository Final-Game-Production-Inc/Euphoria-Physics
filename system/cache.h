//
// system/cache.h
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_CACHE_H
#define SYSTEM_CACHE_H

#if __XENON
#include <ppcintrinsics.h>
#elif __SPU
#include <spu_intrinsics.h>
#elif __PPU
#include <ppu_intrinsics.h>
#elif __WIN32PC
#include <emmintrin.h>
#elif RSG_ORBIS
#include <emmintrin.h>
#include <xmmintrin.h>
#elif RSG_DURANGO
#include <emmintrin.h>
#include <xmmintrin.h>
#endif
#if __WIN32
#include <crtdefs.h>
#else
#include <stddef.h>
#endif

#if RSG_ORBIS || RSG_DURANGO || RSG_PC
// PURPOSE: Use in place of ALIGNAS to align a variable or class to a cache line
#define RSG_CACHE_LINE_SIZE 64
#elif RSG_PS3 || RSG_XENON
#define RSG_CACHE_LINE_SIZE 128
// Add new platforms as needed
#endif

#define CACHE_ALIGNED ALIGNAS(RSG_CACHE_LINE_SIZE)

namespace rage {

#if __WIN32PC
// Global - data cache line size in bytes.
extern u32 g_DataCacheLineSize;
#endif

#if __PPU || RSG_XENON
// PURPOSE:	Write back a range of memory to data cache.  Necessary when you expect
//			cached data to be immediately usable by a dma-based coprocessor.
// PARAMS:	base - base address of memory to flush
//			bytes - Number of bytes to flush
//			flush - true if the cache is to be cleared, false if the data remains in the cache
// NOTES:	Both the start and end addresses will be rounded outward to the platform-
//			specific cache boundary size.
static inline void WritebackDC(const void *base,size_t bytes) {
	u32 start = (u32)base & ~127;
	u32 end = ((u32)base + bytes - 1) & ~127;
	do {
		__dcbst(XENON_ONLY(0,) (void*)start);
		start += 128;
	} while (start <= end);
}
#elif RSG_DURANGO || RSG_ORBIS || RSG_PC
static inline void WritebackDC(const void *base,size_t bytes) {
#	if __WIN32PC
		const size_t cacheLineSize = g_DataCacheLineSize;
#	else
		const size_t cacheLineSize = 64;
#	endif
	size_t start = (size_t)base & (size_t)-(ptrdiff_t)cacheLineSize;
	size_t end = ((size_t)base + bytes - 1) & (size_t)-(ptrdiff_t)cacheLineSize;
	_mm_mfence();
	do {
		_mm_clflush((void*)start);
		start += cacheLineSize;
	} while (start <= end);
}
#else
static inline void WritebackDC(const void*,size_t) { }
#endif

// PURPOSE:	Zero a data cache line. Prevents a fetch from main memory before a write.
// PARAMS:	base - base address of memory
//			offset - optional offset
#if __PS3 || __XENON
__forceinline void ZeroDC(void* pAddr, u32 offset = 0)
#else 
__forceinline void ZeroDC(void*, u32)
#endif // __PPU || __XENON
{
#if __XENON
	FastAssert(!((((u32)pAddr)+offset)&127)); // ensure 128 byte aligned
	__dcbz128(offset, pAddr);
#elif __PPU
	FastAssert(!((((u32)pAddr)+offset)&127)); // ensure 128 byte aligned
	__dcbz(((u8*)pAddr) + offset);
#endif
}

inline void ZeroDCRange(void* pDest, void* pEnd)
{
	size_t destaddr = ((size_t)pDest)+127;
	size_t end = ((size_t)pEnd) & 127;
	for(; destaddr < end; destaddr += 128)
		ZeroDC((void*)destaddr, 0);
}

} // namespace rage

// Intentionally outside of RAGE namespace because it's a macro on some platforms and a function on others.
#if __XENON
#define PrefetchDC(x)	__dcbt(0,(void*)(x))
#elif __PPU
#define PrefetchDC(x)	__dcbt((void*)(x))
#elif __WIN32PC || RSG_ORBIS
inline void PrefetchDC(const void* p)
{
	// "Hint" reference:
	// http://www.sesp.cse.clrc.ac.uk/html/SoftwareTools/vtune/users_guide/mergedProjects/analyzer_ec/mergedProjects/reference_olh/mergedProjects/instructions/instruct32_hh/vc249.htm
	_mm_prefetch( (const char*)p, _MM_HINT_NTA );
}
#elif RSG_DURANGO
inline void PrefetchDC(const void* p)
{
	// I ran the GTAV performance tests multiple times using _MM_HINT_NTA and _MM_HINT_T0.  I got similar
	// performance for the 2 hints on Orbis but consistently better results using _MM_HINT_T0 on Durango.
	_mm_prefetch( (const char*)p, _MM_HINT_T0 );
}
#else
inline void PrefetchDC(const void*) { }
#endif

#if __XENON || __PPU || __WIN32PC || RSG_DURANGO || RSG_ORBIS
template<size_t bufferSize>
void PrefetchBuffer(const void* t)
{
#if __WIN32PC
	Assert( rage::g_DataCacheLineSize && "Data cache line size is invalid." );
	rage::u32 CacheLineSize = rage::g_DataCacheLineSize;
#elif RSG_DURANGO || RSG_ORBIS
	const rage::u32 CacheLineSize = 64; // RSG_ORBIS have 64-byte cache lines (CPU_Core-Overview_e.pdf)
#else
	const rage::u32 CacheLineSize = 128; // Both __XENON and __PPU have 128 byte cache lines
#endif

	for(rage::u32 i = 0; i < bufferSize; i += CacheLineSize)
	{
		PrefetchDC(((const char*)t) + i);
	}
}
#else
template<size_t bufferSize>
void PrefetchBuffer(const void* /*t*/)
{
}
#endif


#if __XENON || __PPU || __WIN32PC || RSG_DURANGO || RSG_ORBIS
template<typename _Type> void PrefetchObject(_Type* t)
{
	PrefetchBuffer<sizeof(_Type)>(t);
}
#else
template<typename _Type> void PrefetchObject(_Type*)
{
}
#endif

#if __XENON
#define PrefetchDC2(base,offset)	__dcbt(offset,(char*)(base))
#elif __PPU
#define PrefetchDC2(base,offset)	__dcbt((char*)(base)+offset)
#elif __WIN32PC || RSG_DURANGO || RSG_ORBIS
inline void PrefetchDC2(const void* base,size_t offset)
{
	// "Hint" reference:
	// http://www.sesp.cse.clrc.ac.uk/html/SoftwareTools/vtune/users_guide/mergedProjects/analyzer_ec/mergedProjects/reference_olh/mergedProjects/instructions/instruct32_hh/vc249.htm
	_mm_prefetch( (const char*)base+offset, _MM_HINT_NTA );
}
#else
inline void PrefetchDC2(const void*,size_t) { }
#endif

#if __XENON
#define FreeDC(base,offset)	__dcbf(offset,(char*)(base))
#elif __PPU
#define FreeDC(base,offset)	__dcbf((char*)(base)+offset)
#else
inline void FreeDC(const void*, size_t) { }
#endif

#endif // SYSTEM_CACHE_H

