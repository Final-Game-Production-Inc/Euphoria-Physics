// 
// system/sharedheap.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_SHAREDHEAP_H
#define SYSTEM_SHAREDHEAP_H

#include "data/resource.h"

namespace rage {

/*
	The shared heap and shared pointer classes are intended primarily for PS3 SPU
	compatibility, although they force you to think about things like cache
	locality even on other platforms.

	The goal is to abstract the memory barrier between the PPU and SPU.
	A shared heap is always created on the PPU side; then a handle to this
	heap (a shared pointer) is passed around on both the PPU and SPU.

	On the SPU side, given a shared pointer, you can lock the heap for read-only,
	read-write, or write-only access.  Write-only is a bit of a misnomer, because
	you are allowed to read data that you already wrote; this is merely an
	optimization to avoid the initial copy from the PPU side if you don't care
	about the current contents.

	Any given heap can only be locked by one client at a time, but it may be
	locked more than once by that client to simplify nesting code.  Since the
	heap ultimately lives on the PPU, locking on the PPU side is only to guarantee
	that the SPU's see a coherent version of the data.  When the heap is first
	locked on the SPU side (unless it is locked write-only because it is strictly
	an output buffer) the system initiates a DMA operation.  If you choose,
	you can use an alternative begin/end API that will schedule the DMA without
	blocking, which is useful for requesting your next input ahead of time.
	Local memory for the heap is allocated dynamically.

	Once the heap is locked and operated upon, it can be unlocked; once the lock
	count drops to zero, unless the heap was locked for read-only access, the
	data is copied back to the PPU.

	In the future we may support locking and unlocking portions of a shared heap
	to minimize local memory use and DMA bandwidth, but my hope is that we can
	instead create larger numbers of smaller shared heaps instead in order to
	keep the API as simple as possible.

	Since we can lock the same heap multiple times from the same client and we
	have three different locking methods, certain combinations don't make
	sense.  In particular, once a heap is first locked for write-only access all
	further locks must also be write-only (when I say "first locked" I mean any
	time it is locked when it was not currently already locked, not merely the
	first time the heap is ever locked, period).  Obviously a heap that is first
	locked read-write can be subsequently locked read-only.  We also allow a
	heap that is locked read-only to subsequently be locked read-write since
	there's no technical reason preventing this.

	The sysSharedHeap object exists only on the PPU; it must be 128-byte aligned
	to avoid cache coherency issues.

	TODO: Right now it's not safe to Lock a sysSharedPtr after it's been locked
	so we might just not allow multiple locks.  Otherwise we need another
	level of indirection so that the local buffer is only allocated once.
*/


class sysSharedHeap {
	friend class sysSharedPtrBase;
public:
private:
	// 128-byte-aligned to avoid cache line issues.
	void* Addr;		// Address this heap has been relocated to
	int Size;		// Size of the heap (including this header)
	u32 LockCount;	// Number of outstanding locks
	bool Read, Write;
	ATTR_UNUSED bool pad0, pad1;
	// More to come here
};


class sysSharedPtrBase {
public:
	void Unlock();

protected:
	void* Lock(void *&origAddr,int &size,bool rd,bool wr);

#if __SPU
	u32				m_RemoteAddr;	// PPU-side address of the sysSharedHeap.
	sysSharedHeap	*m_LocalAddr;	// Local copy, or zero if not locked.
#else
	sysSharedHeap	*m_LocalAddr;	// Local address
	u32				pad0;			// Keep object size consistent
#endif
};

template <class _T> class sysSharedPtr: public sysSharedPtrBase {
public:
	_T* Lock(bool rd,bool wr) {
		/*	Note that we live in the address space of whoever locked us last.
			So if the SPU locks a heap and it gets written back to PPU memory,
			it will still be in the SPU's address space.  If and when the heap
			is locked on the PPU side, it will be relocated into the PPU address space.
			Arguably this transform should be done on the SPU side but our resource
			constructors are written assuming you're translating from an invalid to
			a valid address space.  We could fix this with some significant API
			changes or by logging all fixups in a stack that get applied in
			reverse order. */
		void *origAddr;	int size;
		_T* result = (_T*) sysSharedPtrBase::Lock(origAddr,size,rd,wr);
		if (origAddr != result) {
			datResource rsc(result,origAddr,size);
			_T::Place(rsc);
		}
		return result;
	}
};

}	// namespace rage

#endif	// SYSTEM_SHAREDHEAP_H
