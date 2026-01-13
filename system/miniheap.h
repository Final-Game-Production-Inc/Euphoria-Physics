// 
// system/miniheap.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_MINIHEAP_H
#define SYSTEM_MINIHEAP_H

namespace rage {

// PURPOSE
//   Begin redirecting all memory allocations to grow-only heap in a particular block of memory.
// PARAMS
//   buffer - a pointer to the start of the mini heap
//   size - the number of bytes in the mini heap
// NOTES
//   These calls will NOT nest.
extern void sysMemStartMiniHeap(void *buffer,size_t size);


// PURPOSE
//   Enable/disable the miniheap allocator. If enable is false, the default allocator is enabled 
//	 again, but the miniheap allocator is retained until sysMemEndMiniHeap.
extern void sysMemEnableMiniHeap(bool enable);


// PURPOSE
//   Restores memory allocator to previous location
// RETURNS
//   Returns amount of heap remaining.
extern size_t sysMemEndMiniHeap();

// PURPOSE
//	Determine the amount of memory used in the mini heap
// RETURNS
//	The amount of memory used in the mini heap
extern size_t sysMemGetUsedMiniHeap();

// PURPOSE
//	Get current top of the mini heap
// Returns
//	Top of the heap
extern void *sysMemGetTopMiniHeap( size_t align = 1 );

// PUPOSE
//	Get the amount of free memory left in the mini heap
// Returns
//	Available unallocated memory in mini heap
extern size_t sysMemGetFreeMiniHeap( size_t align = 1 );

// PURPOSE
// pop memory off the top of the heap
extern void sysMemPopMiniHeap( size_t popCount );

}

#endif // SYSTEM_MINIHEAP_H
