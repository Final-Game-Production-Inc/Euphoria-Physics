//
// system/tasklog.cpp
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#include "tasklog.h"
#include "memory.h"
#include "system/new.h"

#if __TASKLOG

#if __64BIT
#	define  TASKLOG_BUFFER_ALIGN    32
#else
#	define  TASKLOG_BUFFER_ALIGN    16
#endif

ALIGNAS(TASKLOG_BUFFER_ALIGN) size_t s_TaskLogBuffer[512*4];
rage::sysTaskLog s_TaskLog(s_TaskLogBuffer,sizeof(s_TaskLogBuffer));

namespace rage {

#if !__SPU

#if RSG_PS3
static const char *DecodeSpuString(u32 index) {
	// Use a switch statement instead of an array so it's easier to maintain.
	switch (index) {
		case 0:	// shouldn't ever happen, but avoids compile warning on SNC
		default: return "UNKNOWN %x %x %x";
	}
}
#endif

sysTaskLog::sysTaskLog(void *buffer,size_t bufferSize)
{
#if ENABLE_DEBUG_HEAP
	
	// If a debug heap allocation works, use a much bigger buffer.
	size_t debugTrySize = __64BIT ? 128*1024 : 64*1024;
	void* debugTry;

	{
		// Debug Heap
		sysMemAutoUseDebugMemory debug;
		debugTry = rage_new char[debugTrySize];
	}

	if (debugTry) 
	{
		void *alignedAlloc = (void*)(((size_t)debugTry+TASKLOG_BUFFER_ALIGN-1)&(size_t)-TASKLOG_BUFFER_ALIGN);
		debugTrySize -= (size_t)alignedAlloc - (size_t)debugTry;
		debugTrySize &= (size_t)-TASKLOG_BUFFER_ALIGN;
		debugTry = alignedAlloc;
		buffer = debugTry;
		bufferSize = debugTrySize;
	}
#endif

	Assert(((size_t)buffer & (4*sizeof(size_t)-1)) == 0);
	Assert((bufferSize     & (4*sizeof(size_t)-1)) == 0);

	m_Buffer = (size_t*) buffer;
	m_BufferCount = (u32)(bufferSize >> (__64BIT?5:4));
	m_BufferLast = m_BufferCount - 1;
}

void sysTaskLog::Dump()
{
	// m_BufferLast contains the most recently added message.
	// m_BufferLast+1 is the next-most-recently-added message, and so on.
	// So the messages are displayed with more recently added messages listed first.
	Displayf("*** BEGIN TASKLOG DUMP (newest message) ***");
	int toPrint = m_BufferCount;
	u32 offset = m_BufferLast;
	while (toPrint--) {
		size_t *cur = (size_t*)((size_t)m_Buffer + (offset<<(__64BIT?5:4)));
		if (++offset == m_BufferCount)
			offset = 0;
		if (cur[0])
			Displayf(PS3_ONLY(cur[0]<=65535?DecodeSpuString(cur[0]):)(const char*)cur[0],
				cur[1],cur[2],cur[3]);
	}
	Displayf("*** END TASKLOG DUMP (oldest message) ***");
}

#endif	// !__SPU

#if !__PS3
sysSpinLockToken sysTaskLog::sm_Token = SYS_SPINLOCK_UNLOCKED;
#endif	// !__PS3


} // namespace rage

#undef TASKLOG_BUFFER_ALIGN

#endif		// __TASKLOG
