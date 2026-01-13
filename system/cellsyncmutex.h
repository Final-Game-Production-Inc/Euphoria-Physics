//
// system/cellsyncmutex.h
//
// Copyright (C) 2013-2013 Rockstar Games.  All Rights Reserved.
//
// Drop in replacement for CellSyncMutex.
// Based heavily on the disassembly of Sony's code.
//

#ifndef SYSTEM_CELLSYNCMUTEX_H
#define SYSTEM_CELLSYNCMUTEX_H

#if __PS3

namespace rage
{

struct RageCellSyncMutex
{
	u32 m_Mutex;
};

inline void rageCellSyncMutexInitialize(RageCellSyncMutex *mutex) { mutex->m_Mutex = 0; }

#if __PPU
	extern void rageCellSyncMutexLock(RageCellSyncMutex *mutex, unsigned busySpinCount=1024);
	extern void rageCellSyncMutexUnlock(RageCellSyncMutex *mutex);
#else
	extern void rageCellSyncMutexLock(u32 mutexEa);
	extern void rageCellSyncMutexUnlock(u32 mutexEa);
#endif

}
// namespace rage

#endif // __PS3

#endif // SYSTEM_CELLSYNCMUTEX_H
