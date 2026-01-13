// 
// system/spuget.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_SPUGET_H 
#define SYSTEM_SPUGET_H 

#include "system/dma.h"

namespace rage {

#if __SPU

#define DEBUG_GET	0			// 0=none, 1=names, 2=names and hex data
#if DEBUG_GET
const char *spuGetName;
#endif

char *spuScratch, *spuScratchTop;

#ifndef spuGetTag
const int spuGetTag = 0;
const int spuGetTag2 = 1;
const int spuBindTag = 2;
#endif

#if !__NO_OUTPUT
void spuHexDump(void *ls,size_t size)
{
	for (size_t i=0; i<size; i+=16)
	{
		unsigned *j= (unsigned*) ((char*)ls + i);
		Displayf("%p: %8x %8x %8x %8x",j,j[0],j[1],j[2],j[3]);
	}
}
#endif

/* PURPOSE:	Given an address and a size, pull the data from the PPU and return
			the new address in local store.  If the address is already in local
			store, return immediately.
   PARAMS:	ppuAddress - ppu-side address of data
			ppuSize - amount of data to transfer.  If it's smaller than 16 bytes,
				it must not cross a 16-byte boundary.  If it's 16 bytes or larger,
				it must be aligned to a 16-byte boundary, and its transfer size
				is rounded up to next multiple of 16
   RETURNS:	Pointer to copy of data in local store.
   NOTES:	spuScratch must be initialized by the caller to a scratch area, and
			spuScratchTop is the first invalid byte.  We assert if we overrun
			the scratch area */
inline void* spuGetDataNoWait(void *ppuAddress,size_t ppuSize)
{
#if DEBUG_GET
	Displayf("%s: get from %p, %lu bytes to addr %p",spuGetName,ppuAddress,ppuSize,spuScratch);
	spuGetName = NULL;
#endif
	Assertf(((u32)ppuAddress & 15) == 0 || ppuSize < 16,"Bad ppu addr %p or size %lu",ppuAddress,ppuSize);

	// If it's already in LS (or NULL), return immediately without doing any additional work
	if ((u32)ppuAddress < 256*1024)
		return ppuAddress;

	// Round size up to next multiple of 16
	ppuSize = (ppuSize+15) & ~15;

	void *result = spuScratch;
	spuScratch = spuScratch + ppuSize;
	if (spuScratch > spuScratchTop)
#if SYS_DMA_VALIDATION
		Quitf("spuGetData - scratch overflow on %u byte allocation (during %s)",(u32)ppuSize,sysDmaContext);
#else
		Quitf("spuGetData - scratch overflow on %u byte allocation",(u32)ppuSize);
#endif

//	SPU_BOOKMARK(103);

	// Retrieve the data (ignoring any unaligned PPU address)
	sysDmaLargeGet(result, (uint64_t)ppuAddress & ~15, ppuSize, spuGetTag);

	// Copy the LSB's of the source address into the destination, in case it wasn't aligned
	result = (void*)((u32)result | ((u32)ppuAddress & 15));

//	SPU_BOOKMARK(0);

#if DEBUG_GET > 1
	sysDmaWaitTagStatusAll(1<<spuGetTag);
	spuHexDump(result,ppuSize);
#endif
	return result;
}

inline void* spuGetData(void *ppuAddress,size_t ppuSize)
{
	void *ret = spuGetDataNoWait(ppuAddress,ppuSize);
	sysDmaWaitTagStatusAll(1<<spuGetTag);
	return ret;
}

/* PURPOSE:	Type-safe version of spuGetData specialized for atArrays.  Will retrieve
			the element data based on the current count (so the basic array data must
			already be valid) */
template <class T> __forceinline T* spuGetArray_(atArray<T> &array)
{
	return (array.m_Elements = (T*) spuGetData(array.m_Elements, sizeof(T) * array.m_Count));
}

/* PURPOSE:	Type-safe version of spuGetData.  Can provide optional element count. */
template <class T> __forceinline T* spuGet_(T* &ptr,unsigned count = 1)
{
	return (ptr = (T*) spuGetData(ptr, sizeof(T) * count));
}

template <class T> __forceinline T* spuGetTemp_(T* ptr,unsigned count = 1)
{
	return (T*) spuGetData(ptr, sizeof(T) * count);
}

#if DEBUG_GET
#define spuGetArray(x)		((spuGetName = "array " #x), rage::spuGetArray_(x))
#define spuGet(x...)		((spuGetName = #x), rage::spuGet_(x))
#define spuGetTemp(x...)	((spuGetName = #x), rage::spuGetTemp_(x))
#else
#define spuGetArray(x)		rage::spuGetArray_(x)
#define spuGet(x...)		rage::spuGet_(x)
#define spuGetTemp(x...)	rage::spuGetTemp_(x)
#endif

#else	// !__SPU

#define spuGetArray(x)	(x)
#define spuGet(x)	(x)

#endif	// !__SPU

} // namespace rage

#endif // SYSTEM_SPUGET_H 
