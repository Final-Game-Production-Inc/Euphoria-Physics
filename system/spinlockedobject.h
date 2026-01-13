#ifndef _INC_SPINLOCKEDOBJECT_H_
#define _INC_SPINLOCKEDOBJECT_H_

//////////////
// Includes //
//////////////

#if !__SPU
#include "system/interlocked.h"
#endif

#if __SPU
#include "system/dma.h"
#include <cell/atomic.h>
#include <cell/dma.h>
#endif //__SPU

namespace rage {

/////////////
// Classes //
/////////////

/*
PURPOSE:

This class is useful to implement spin waiting. The time spent by the Spin() method increases at every call
(up to a fixed amount and unless you call Start() again).
*/

class sysSpinner
{
private:

	u32		m_counter;

public:

	void Start() {
		m_counter = 8;
	}
	
	void Spin()
	{
		volatile u32	spinner = 0;

		while ( spinner < m_counter ) {
			++spinner;
			PPU_ONLY(__db16cyc());
			SPU_ONLY(spu_readch(SPU_RdDec));
		}

		if ( m_counter < 1024 )
			m_counter <<= 1;
	}

	void End() {
	}
};

/*
PURPOSE:

This class represents a spin lock with an object "embedded" in it. There's a limit of 124 bytes on the size of
the object (i.e. any instance of this class must fit in a 128-bytes cache line).

On traditional CPUs (PPU, Xenon, PC...) using this class is not different than simply having the object in
memory and controlling access to it using a normal spinlock (implemented with interlocked functions).

The advantage of this class comes from using it in SPU code. On SPU, interlocked functions work by locking
and unlocking entire cache lines (even if you just want to atomically modify or read a single word):
- When you acquire the lock, a copy of the cache line is stored into the LS.
- When you release the lock, you must post the new content for the entire cache line to main memory.
By embedding the actual object that you want to access in the cache line, the data is
fetched from / posted to main memory by the interlocked functions theirselves without the need of
an additional DMA.
*/

template <typename Type>
class sysSpinLockedObject
{
private:

	enum
	{
		SPINLOCK_UNLOCKED,
		SPINLOCK_LOCKED
	};

	union
	{
		u32		m_locked;
		u32		m_ea;
	};

	Type		m_object;
	u8			m_padding[ 128 - sizeof(u32) - sizeof(Type) ];

public:

	Type* GetObject() {
		return &m_object;
	}

#if !__SPU
	void Initialize() {
		m_locked = SPINLOCK_UNLOCKED;
	}
	
	u32 GetEa() const {
		return (u32) this;
	}

	void Lock()
	{
		sysSpinner	spinner;

		spinner.Start();
		while ( sysInterlockedExchange( &m_locked, SPINLOCK_LOCKED ) != SPINLOCK_UNLOCKED )
			spinner.Spin();
		spinner.End();
	}

	void Unlock() {
		sysInterlockedExchange( &m_locked, SPINLOCK_UNLOCKED );
	}
#endif // !__SPU

#if __SPU
	void Initialize(const u32 ea) {
		m_ea = ea;
	}
	
	u32 GetEa() const {
		return m_ea;
	}

	void Lock()
	{
		const u32	ea = m_ea;
		sysSpinner	spinner;

		spinner.Start();
		while ( cellAtomicStore32( (u32*) this, ea, SPINLOCK_LOCKED ) != SPINLOCK_UNLOCKED )
			spinner.Spin();
		spinner.End();

		m_ea = ea;
	}

	void Unlock()
	{
		const u32	ea = m_ea;
		sysSpinner	spinner;

		m_locked = SPINLOCK_UNLOCKED;
		cellDmaPutlluc( (u32*) this, ea, 0, 0 );

		spinner.Start();
		while ( cellDmaWaitAtomicStatus() != 0x2 )
			spinner.Spin();
		spinner.End();

		m_ea = ea;
	}
#endif // __SPU
};

} // namespace rage

#endif // !defined _INC_SPINLOCKEDOBJECT_H_
