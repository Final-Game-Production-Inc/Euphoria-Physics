//
// system/cellsyncmutex.cpp
//
// Copyright (C) 2013-2013 Rockstar Games.  All Rights Reserved.
//

#include "cellsyncmutex.h"

#if __PS3

#if __PPU
#	include <ppu_intrinsics.h>
#	include <sys/timer.h>
#elif __SPU
#	include <spu_intrinsics.h>
#	include <spu_mfcio.h>
#endif


namespace rage
{
	// The code below is compatable with cellSyncMutexLock().  Was initially
	// writen for the extra error checking to ensure that current is never
	// incremented past ticket (else the game will get stuck).
	//
	// But our custom implementation is actually better, since it saves an extra
	// memory access in the common case of not needing to wait after getting a
	// ticket.  Also (but way less important) we don't do pointless writes to
	// MFC_EAL, MFC_SIZE or MFC_TagID, and we don't have pointless dsync
	// instructions.
	//
#	if 0
		void rageCellSyncMutexLock(PPU_ONLY(RageCellSyncMutex*) SPU_ONLY(u32) mutex PPU_ONLY(,unsigned UNUSED_PARAM(busySpinCount)))
		{
			ASSERT_ONLY(const int ret =) cellSyncMutexLock(mutex);
			Assert(ret == CELL_OK);
		}

#	elif __PPU
		void rageCellSyncMutexLock(RageCellSyncMutex *mutex, unsigned busySpinCount)
		{
			// The CellSyncMutex structure is two half words
			//      [0..1]  currently running/most recently run ticket
			//      [2..3]  next available ticket
			u32 mut;
			u16 current, ticket;

			// Get a ticket value
			do
			{
				mut = __lwarx((u32*)mutex);
				current = (u16)(mut>>16);
				ticket  = (u16)mut;

				// If the ticket we just got has already expired, then there is
				// a serious error, and we will spin for ever waiting for the
				// ticket.  A likely cause of this would be unlocking the mutex
				// more times than it was locked.
				NOTFINAL_ONLY(if(Unlikely((s16)(ticket-current)<0)) __debugbreak();)

				mut = ((u32)current<<16) | (u32)(u16)(ticket+1);
			}
			while(Unlikely(!__stwcx((u32*)mutex,mut)));

			// Wait till the ticket value is able to be run
			while(Unlikely(current != ticket))
			{
				// This bit is different from Sony's code, cellSyncMutexLock
				// always busy spins (and it doesn't have a delay to be nice to
				// the other hw thread either).
				//
				// While it is a bug to use cellSyncMutexLock in a way where
				// priority inversion can cause a live lock, this is very
				// difficult to protect against (and we have seen it happen at
				// least once, GTAV B*1189938).  So for rageCellSyncMutexLock we
				// are allowing this by going to sleep once the busySpinCount
				// reaches zero.
				//
				// The reason this is branch hinted as likely always, is that if
				// we are going to sleep, then we really don't care about a
				// branch misprediction.  Not that this really matters for SNC
				// though since it is not creating the hinted branch
				// instructions.
				//
				if(Likely(busySpinCount))
				{
					--busySpinCount;
					__db16cyc();
				}
				else
				{
					sys_timer_usleep(50);
				}

				current = *(volatile u16*)mutex;
				NOTFINAL_ONLY(if(Unlikely((s16)(ticket-current)<0)) __debugbreak();)
			}
			__lwsync();
		}

#	elif __SPU
		void rageCellSyncMutexLock(u32 mutexEa)
		{
			char buf[128] ALIGNED(128);
			const u32 ea = mutexEa;
			const u32 ea128 = ea & ~127;
			u16 *const mut = (u16*)(buf + (ea&127));
			u16 current, ticket;
			do
			{
				spu_writech(MFC_LSA, (u32)buf);
				spu_writech(MFC_EAL, ea128);
				spu_writech(MFC_Cmd, MFC_GETLLAR_CMD);
				spu_readch(MFC_RdAtomicStat);

				current = mut[0];
				ticket  = mut[1];
				NOTFINAL_ONLY(if(Unlikely((s16)(ticket-current)<0)) __debugbreak();)

				mut[1] = ticket+1;
				spu_writech(MFC_LSA, (u32)buf);
				spu_writech(MFC_EAL, ea128);
				spu_writech(MFC_Cmd, MFC_PUTLLC_CMD);
			}
			while(Unlikely(spu_readch(MFC_RdAtomicStat)));

			if(Unlikely(current != ticket))
			{
				do
				{
					spu_writech(MFC_LSA, (u32)buf);
					spu_writech(MFC_EAL, ea128);
					spu_writech(MFC_Cmd, MFC_GETLLAR_CMD);
					spu_readch(MFC_RdAtomicStat);
					current = mut[0];
					NOTFINAL_ONLY(if(Unlikely((s16)(ticket-current)<0)) __debugbreak();)
				}
				while(Unlikely(current != ticket));

				// Clear line reservation, don't care if write actually occurs or not.
				// Don't really know how this helps, but the original Sony code did this.
				spu_writech(MFC_LSA, (u32)buf);
				spu_writech(MFC_EAL, ea128);
				spu_writech(MFC_Cmd, MFC_PUTLLC_CMD);
				spu_readch(MFC_RdAtomicStat);
			}
		}
#	endif


#	if 0
		void rageCellSyncMutexUnlock(PPU_ONLY(RageCellSyncMutex*) SPU_ONLY(u32) mutex)
		{
			ASSERT_ONLY(const int ret =) cellSyncMutexUnlock(mutex);
			Assert(ret == CELL_OK);
		}

#	elif __PPU
		void rageCellSyncMutexUnlock(RageCellSyncMutex *mutex)
		{
			__lwsync();
			u32 mut;
			u16 current, ticket;
			do
			{
				mut = __lwarx((u32*)mutex);
				current = (u16)(mut>>16)+1;
				ticket  = (u16)mut;

				// Check that we are not going past the next available ticket.
				// Otherwise this will cause the next locker to spin
				// indefinitely.  Most likely cause for this is unlocking a
				// mutex that has not been locked.
				NOTFINAL_ONLY(if(Unlikely((ticket-current)&0x8000)) __debugbreak();)

				mut = ((u32)current<<16) | (u32)(ticket);
			}
			while(Unlikely(!__stwcx((u32*)mutex, mut)));
		}

#	elif __SPU
		void rageCellSyncMutexUnlock(u32 mutexEa)
		{
			char buf[128] ALIGNED(128);
			const u32 ea = mutexEa;
			const u32 ea128 = ea & ~127;
			u16 *const mut = (u16*)(buf + (ea&127));
			u16 current;
			NOTFINAL_ONLY(u16 ticket;)
			do
			{
				spu_writech(MFC_LSA, (u32)buf);
				spu_writech(MFC_EAL, ea128);
				spu_writech(MFC_Cmd, MFC_GETLLAR_CMD);
				spu_readch(MFC_RdAtomicStat);

				current = mut[0]+1;

#				if !__FINAL
					ticket = mut[1];
					if(Unlikely((ticket-current)&0x8000)) __debugbreak();
#				endif

				mut[0] = current;
				spu_writech(MFC_LSA, (u32)buf);
				spu_writech(MFC_EAL, ea128);
				spu_writech(MFC_Cmd, MFC_PUTLLC_CMD);
			}
			while(Unlikely(spu_readch(MFC_RdAtomicStat)));
		}
#	endif

}
// namespace rage

#endif // __PS3
