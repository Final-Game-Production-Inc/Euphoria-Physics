// 
// system/sharedheap.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#if __SPU
#include <cell/dma.h>
#endif

#include "sharedheap.h"

namespace rage {

void* sysSharedPtrBase::Lock(void *&origAddr,int &size,bool
#if __SPU || __ASSERT
							 read
#endif
							 ,bool write) {
#if __SPU
	if (!m_LocalAddr) {
		const int TAG = 0;

		// Transfer the header into a local buffer.  Currently only necessary
		// to get the size, so in theory we could just store it in the SharedPtr
		// itself, but I expect it to get more complex in the future.
		static u8 buf[128] __attribute__((aligned(128)));
		sysSharedHeap *hdr = (sysSharedHeap*) buf;
		cellDmaGetllar(buf,m_RemoteAddr,0,0);
		AssertVerify(cellDmaWaitAtomicStatus()==0);

		// Allocate local copy of the heap now that we know its size
		m_LocalAddr = (sysSharedHeap*) rage_new char[hdr->Size];

		// Copy the heap from PPU if desired
		if (read) {
			cellDmaGet(m_LocalAddr,m_RemoteAddr,hdr->Size,TAG,0,0);
			cellDmaWaitTagStatusAll(1<<TAG);
		}
		else	// Otherwise just make sure the header is still properly initialized.
			*m_LocalAddr = *hdr;
		m_LocalAddr->Read = read;
		m_LocalAddr->Write = write;
	}
#endif
	origAddr = m_LocalAddr->Addr;
	size = m_LocalAddr->Size;
	++(m_LocalAddr->LockCount);
	
	// Make sure original and current lock types are compatible
	// Turn a read-only lock into a read/write lock if necessary.
	Assert(m_LocalAddr->Read==read);
	m_LocalAddr->Write |= write;

	// Caller doesn't see the header.
	return (m_LocalAddr+1);
}


void sysSharedPtrBase::Unlock() {
	Assert(m_LocalAddr);
	Assert(m_LocalAddr->LockCount);

	m_LocalAddr->LockCount--;
#if __SPU
	// DMA back to PPU if fully unlocked and requested by any locker.
	if (m_LocalAddr->LockCount == 0) {
		const int TAG = 0;
		if (m_LocalAddr->Write) {
			cellDmaPut(m_LocalAddr,m_RemoteAddr,m_LocalAddr->Size,TAG,0,0);
			cellDmaWaitTagStatusAll(1<<TAG);
		}
		// Always reclaim the local storage and mark object as nonresident.
		delete[] (char*) m_LocalAddr;
		m_LocalAddr = NULL;
	}
#endif
}

}
