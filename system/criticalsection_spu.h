// 
// system/criticalsection_spu.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_CRITICALSECTION_SPU_H
#define SYSTEM_CRITICALSECTION_SPU_H

#include "system/criticalsection.h"

#if __PPU
#include <sys/synchronization.h>
#endif
#if __PS3
#include "system/cellsyncmutex.h"
#endif

namespace rage {

#if __PS3

/*
	To reference this object from SPU code, do something like this:
		sysCriticalSectionTokenSpu *pCritSec = (sysCriticalSectionTokenSpu *)CELL_SPURS_PPU_SYM(s_KnownRefToken);
		pCritSec->Lock();
		: : :
		pCritSec->Unlock();
	In particular, pCritSec is really a PPU-side address, NOT an address in local storage.

	Note that these critical section objects nest properly on all platforms except SPU.  That could be fixed if necessary.

	Make sure that you declare a sysCriticalSectionTokenSpu on the PPU side if you need SPU interoperability; in particular,
	this class is not compatible with the more generic sysCriticalSectionToken class on the PPU.  The easiest way to deal
	with this is to use the SYS_CS_SYNC_SPU_DECLARE and SYS_CS_SYNC_SPU macros at the bottom of this file, which expand
	to sysCriticalSection[Token] on non-PS3 platforms, and sysCriticalSection[Token]Spu on PS3 (both PPU and SPU).
*/
struct sysCriticalSectionTokenSpu 
{
#if __SPU
	uint64_t padding[3];
#else
	sys_lwmutex_t critsec;
#endif
	RageCellSyncMutex spinlock;
	unsigned nest_count;

#if __PPU
	sysCriticalSectionTokenSpu()
	{
		sys_lwmutex_attribute_t attr = { SYS_SYNC_FIFO, SYS_SYNC_RECURSIVE, "cs_spu" };
		if (sys_lwmutex_create(&critsec,&attr) != CELL_OK)
			Quitf("Unable to create critsec lwmutex");
		// Tokens are generally at global scope (and zeroed), but let's be pedantic just in case.
		rageCellSyncMutexInitialize(&spinlock);
		nest_count = 0;
	}
	~sysCriticalSectionTokenSpu()
	{
		sys_lwmutex_destroy(&critsec);
	}
#endif
	void Lock() 
	{
#if __PPU
		sys_lwmutex_lock(&critsec,0);
		if (++nest_count == 1)
			rageCellSyncMutexLock(&spinlock);
#else
		rageCellSyncMutexLock((u32)&spinlock);
#endif
	}
	void Unlock() 
	{
#if __PPU
		if (--nest_count == 0)
			rageCellSyncMutexUnlock(&spinlock);
		sys_lwmutex_unlock(&critsec);
#else
		rageCellSyncMutexUnlock((u32)&spinlock);
#endif
	}
};

struct sysCriticalSectionSpu
{
	sysCriticalSectionSpu(sysCriticalSectionTokenSpu &token) : Token(token) 
	{
		Token.Lock();
	}
	~sysCriticalSectionSpu() 
	{
		Token.Unlock();
	}

	sysCriticalSectionTokenSpu &Token;
};
// On PPU, we take the critsec first and then a spinlock, so that we don't busy wait for other ppu threads.
// The SPU only takes the spinlock.
#define SYS_CS_SYNC_SPU_DECLARE(cs)		rage::sysCriticalSectionTokenSpu cs
#define SYS_CS_SYNC_SPU(cs)				rage::sysCriticalSectionSpu __cssync(cs)

#else		// !__PS3

#define SYS_CS_SYNC_SPU_DECLARE(tok)	rage::sysCriticalSectionToken tok
#define SYS_CS_SYNC_SPU(cs)				rage::sysCriticalSection __cssync(cs)

#endif

}	// namespace rage

#endif
