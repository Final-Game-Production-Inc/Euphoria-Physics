//
// system/tasklog.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef SYSTEM_TASKLOG_H
#define SYSTEM_TASKLOG_H

#if __PS3
#include <cell/atomic.h>
# if __SPU
# include "system/dma.h"
# endif
#else
#include "system/spinlock.h"
#endif

#define __TASKLOG	(!__FINAL && !__TOOL && !__RESOURCECOMPILER)

#if __TASKLOG
#define TASKLOG_ONLY(x)		x
#define TASKLOG(fmt,...)	s_TaskLog.Log(fmt,##__VA_ARGS__)
#else
#define TASKLOG_ONLY(x)
#define TASKLOG(fmt,...)	do { } while (0)
#endif

#if __TASKLOG && __ASSERT
#define TASKLOG_ASSERT(x)			(void)(Likely(x) || (s_TaskLog.Dump(),::rage::diagAssertHelper(__FILE__,__LINE__,"%s",#x)) || (__debugbreak(),0))
#define TASKLOG_ASSERTF(x,fmt,...)	(void)(Likely(x) || (s_TaskLog.Dump(),::rage::diagAssertHelper(__FILE__,__LINE__,"%s: " fmt,#x,##__VA_ARGS__)) || (__debugbreak(),0))
#else
#define TASKLOG_ASSERT(x)
#define TASKLOG_ASSERTF(x,fmt,...)
#endif

namespace rage {

class sysTaskLog {
public:
#if !__SPU
	// PURPOSE:	Creates log buffer.  Must be at multiple of 16 bytes and size (32 bytes for 64-bit platforms)
	sysTaskLog(void *buffer,size_t bufferSize);

	void Dump();
#endif

	// PURPOSE:	Log a message
	// PARAMS:	arg0, arg1, arg2, arg3 - Values for the message.
	// NOTES:	All messages are 16 bytes to keep buffer management as fast
	//			and simple as possible (32 bytes for 64-bit platforms).
	//			On SPU, "this" is actually the ea in main memory of the buffer.
#if __SPU
	inline void Log(u16 arg0,size_t arg1 = 0,size_t arg2 = 0,size_t arg3 = 0);
#else
	inline void Log(const char *arg0,size_t arg1 = 0,size_t arg2 = 0,size_t arg3 = 0);
#endif
private:
	size_t		*m_Buffer;
	u32			m_BufferCount;		// in units of 4*size_t
	u32			m_BufferLast;		// pointer to most-recently-added message} ALIGNED(128);
	ATTR_UNUSED u32	padding[__64BIT ? 28 : 29];
#if !__PS3
	static		sysSpinLockToken sm_Token;
#endif
} ALIGNED(128);

#if __PPU
inline void sysTaskLog::Log(const char *arg0,size_t arg1,size_t arg2,size_t arg3)
{
	u32 newBufferLast;
	do {
		newBufferLast = cellAtomicLockLine32(&m_BufferLast);

		if (newBufferLast == 0)
			newBufferLast = m_BufferCount;
		--newBufferLast;

	} while (cellAtomicStoreConditional32(&m_BufferLast, newBufferLast));
	Assert(m_BufferCount > 0 && newBufferLast < m_BufferCount);
	u32 *tmp = (u32*) (m_Buffer + newBufferLast);
	tmp[0] = (u32)arg0;
	tmp[1] = arg1;
	tmp[2] = arg2;
	tmp[3] = arg3;
}
#elif __SPU
inline void sysTaskLog::Log(u16 arg0,size_t arg1,size_t arg2,size_t arg3)
{
	sysTaskLog _this;
	do {
		// Grab entire cache line
		cellDmaGetllar(&_this,(u64)(u32)this,0,0);
		cellDmaWaitAtomicStatus();

		if (_this.m_BufferLast == 0)
			_this.m_BufferLast = _this.m_BufferCount;
		--_this.m_BufferLast;

		// Attempt to write it back out again
		cellDmaPutllc(&_this,(u64)(u32)this,0,0);
		// ...and retry if it failed
	} while (cellDmaWaitAtomicStatus());

	Assert(_this.m_BufferCount > 0 && _this.m_BufferLast < _this.m_BufferCount);

	u32 eaMsg = (u32) _this.m_Buffer + (_this.m_BufferLast<<4);

	u32 msgBuf[4] ;
	msgBuf[0] = arg0;
	msgBuf[1] = arg1;
	msgBuf[2] = arg2;
	msgBuf[3] = arg3;
	sysDmaPutfAndWait(msgBuf, eaMsg, 16, 9 /* FIFOTAG */);
}
#else
inline void sysTaskLog::Log(const char *arg0,size_t arg1,size_t arg2,size_t arg3)
{
	// Normally spinlocks can be bad news, but this is a perfect
	// application for them -- impossible to block, and guaranteed
	// to finish quickly.
	sysSpinLockLock(sm_Token);
	if (m_BufferLast == 0)
		m_BufferLast = m_BufferCount;
	--m_BufferLast;
	size_t *tmp = m_Buffer + (m_BufferLast<<2);
	sysSpinLockUnlock(sm_Token);
	tmp[0] = (size_t)arg0;
	tmp[1] = arg1;
	tmp[2] = arg2;
	tmp[3] = arg3;
}
#endif

} // namespace rage

// Has to be outside namespace to avoid SPU symbol resolution issues.
extern rage::sysTaskLog s_TaskLog;

#endif // SYSTEM_TASKLOG_H
