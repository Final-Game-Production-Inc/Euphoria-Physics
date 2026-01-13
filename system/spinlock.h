// 
// system/spinlock.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_SPINLOCK_H
#define SYSTEM_SPINLOCK_H

#include "interlocked.h"

#if __SPU
#include "system/dma.h"
#endif

#if __XENON
// These are taken from ppcintrinsics.h and are obviously never going to change on this platform.
extern "C" {
	void __emit(unsigned int opcode);
}
#define sys_lwsync()		__emit(0x7C2004AC)
#define sys_sync()			__emit(0x7C0004AC)
#elif __PPU
#include <ppu_intrinsics.h>
#define sys_lwsync			__lwsync
#define sys_sync			__sync
#elif RSG_PC || RSG_DURANGO 
extern "C" void _ReadWriteBarrier();
#pragma intrinsic(_ReadWriteBarrier)
#define sys_lwsync			_ReadWriteBarrier
#define sys_sync			_ReadWriteBarrier
#elif RSG_ORBIS
#define sys_lwsync			__builtin_ia32_mfence
#define sys_sync			__builtin_ia32_mfence
#else
static inline void sys_lwsync() {}
static inline void sys_sync()	{}
#endif

namespace rage {

typedef volatile unsigned sysSpinLockToken;

enum
{
    SYS_SPINLOCK_UNLOCKED,
    SYS_SPINLOCK_LOCKED
};

inline void sysSpinLockLock(sysSpinLockToken& token)
{
#if __SPU
	Assert(cellDmaGetUint32((uint64_t)&token, 31, 0, 0) <= SYS_SPINLOCK_LOCKED);
#else
	Assert(token <= SYS_SPINLOCK_LOCKED);		// unsigned type; catch uninitialized variables
#endif
	u32 stopper = 8;
	while (sysInterlockedCompareExchange(&token,SYS_SPINLOCK_LOCKED,SYS_SPINLOCK_UNLOCKED) == SYS_SPINLOCK_LOCKED) {
		volatile u32 spinner = 0;
		while (spinner < stopper) {
			++spinner;
			PPU_ONLY(__db16cyc());
			SPU_ONLY(spu_readch(SPU_RdDec));
		}
		if (stopper < 1024)
			stopper <<= 1;
	}
	/* From Bruce Dawson post on xboxds:
	In the lock case you are supposed to have an import barrier--an lwsync after 
	you acquire the lock. This ensures your reads will get a consistent view of 
	memory. I'm not sure exactly when omitting this will cause failures, but 
	that is the one missing piece I can see. I suspect that the __lwsync for an 
	import barrier is so that after you acquire the lock you are guaranteed that 
	any subsequent reads will be coming from L2, if needed, rather than using 
	stale data in L1. */
	sys_lwsync();
}

inline void sysSpinLockUnlock(sysSpinLockToken& token)
{
	/* From Bruce Dawson post on xboxds:
	Simple rule of thumb: if you want a series of writes to be visible to 
	another hardware thread, do an lwsync before you set the flag that says that 
	data is ready. A critical section does this for you, but the Interlocked* 
	functions do not. This rule applies whether the two threads in question are 
	on the same core or different cores. Failing to do so could cause bugs that 
	will occur at random and rare times, or could make your title less likely to 
	make the back-compat list for the *next* Xbox. */
	sys_lwsync();
#if __SPU
	Assert(cellDmaGetUint32((uint64_t)&token, 31, 0, 0) <= SYS_SPINLOCK_LOCKED);
	cellDmaPutUint32(SYS_SPINLOCK_UNLOCKED, (uint64_t)&token, 31, 0, 0);
#else
	Assertf(token == SYS_SPINLOCK_LOCKED, "Spinlock not locked! State = %d", token);		// make sure it's still locked
	token = SYS_SPINLOCK_UNLOCKED;
#endif
}

inline bool sysSpinLockTryLock(sysSpinLockToken& token)
{
#if __SPU
	Assert(cellDmaGetUint32((uint64_t)&token, 31, 0, 0) <= SYS_SPINLOCK_LOCKED);
#else
	Assert(token <= SYS_SPINLOCK_LOCKED);		// unsigned type; catch uninitialized variables
#endif
	const bool gotLock =
		( sysInterlockedCompareExchange(&token,SYS_SPINLOCK_LOCKED,SYS_SPINLOCK_UNLOCKED) == SYS_SPINLOCK_UNLOCKED );

	if( gotLock )
	{
		sys_lwsync();
	}

	return gotLock;
}

/*
	The sysSpinLock class implements a spinlock, which is a very low overhead mutex.
	Note that it only makes sense to use this when you expect contention to be low, because it will hammer
	the bus trying to get the lock.  It does not handle recursion or anything fancy like that.
*/
class sysSpinLock {
public:
	sysSpinLock(sysSpinLockToken& token) : m_Token(&token) {
		sysSpinLockLock(*m_Token);
	}
	~sysSpinLock() {
		sysSpinLockUnlock(*m_Token);
	}
private:
	sysSpinLockToken *m_Token;
};

}	// namespace rage

//PURPOSE
//  Helper macros used to implement structured locks.
//NOTES
//  The following demonstrates these macros:
//
//  SYS_SPINLOCK_ENTER( lock )
//  {
//      Do some stuff...
//  }
//  SYS_SPINLOCK_EXIT( lock )
//
#define SYS_SPINLOCK_ENTER( lock )\
    { ::rage::sysSpinLockLock( lock ); ASSERT_ONLY( const sysSpinLockToken* __cur_sl_token__ = &lock; )

#define SYS_SPINLOCK_EXIT( lock )\
    FastAssert( &lock == __cur_sl_token__ ); ::rage::sysSpinLockUnlock( lock ); }

#endif	// SYSTEM_SPINLOCK_H
