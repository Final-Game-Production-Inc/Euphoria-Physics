// 
// grcore/gcmringbuffer.h 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_GCMRINGBUFFER_H 
#define GRCORE_GCMRINGBUFFER_H 

#include "system/dma.h"

// Wait ~96,000 cycles to avoid flooding the GPU with label
// reads.  Each read of the SPU decrementer takes 40
// cycles.  Code stolen from Edge.  Uses new count recommended by sony technote.
// https://ps3.scedev.net/technotes/view/749
// This is about 30 microseconds each.
#if __SPU
#define RSX_SPIN_WAIT		for(u32 __i=0; __i<2400; ++__i) spu_readch(SPU_RdDec)
#else
#include <sys/timer.h>
#define RSX_SPIN_WAIT		sys_timer_usleep(30)
#endif // __SPU
#define RSX_DEADMAN_TIMEOUT	100000		// about 3 seconds if you called RSX_SPIN_WAIT this many times.

#include <stdint.h>
#include "grcore/channel.h"

namespace rage {

#if __ASSERT
#define ringAssert(x)				do { if (!(x)) grcErrorf("ringAssert failed, name \"%s\" line %d",m_Name,__LINE__); } while (0)
#define ringAssertf(x,f,args...)	do { if (!(x)) grcErrorf("ringAssertf failed, name \"%s\" line %d " f,m_Name,__LINE__,##args); } while (0)
#define ringErrorf(f,args...)		do { grcErrorf("ringErrorf, name \"%s\" line %d " f,m_Name,__LINE__,##args); } while (0)
#else
#define ringAssert(x)
#define ringAssertf(x,f,...)
#define ringErrorf(f,...)
#endif

/*	
	Class implements a ring buffer shared between PPU/SPU and the RSX.
	Buffer must be at least twice as large as the largest single allocation
	or else it runs the risk of wedging due to wraparound issues.  In particular,
	imagine the case where Head==Tail in the middle of the buffer, and you
	try to allocate (Size/2)+1 bytes.  Note that we could deal with this
	by resetting Head=Tail=0 any time the queue goes empty, although that
	would violate the current design where only one of the two is modified
	by each of the sender and receiver.

	Note that there are no particular restrictions on the size of the FIFO.

	If Head==Tail, the ring buffer is empty.  Otherwise, Head and Tail
	are always within the half-open interval [0, Size).  The buffer can
	hold at most Size-1 bytes so that we can tell empty and full apart.
*/
class gcmRingBuffer {
public:
	// PURPOSE:	Allocates space in the ring buffer
	void *Allocate(size_t size) {
		ringAssert(size);
		ringAssert(2*size <= GetRingSize());

		// Add validation for ring buffer free cache since it's a new feature
		ringAssertf( ((m_CachedFree&&((m_CachedFree<m_StartEa)||(m_CachedFree>m_EndEa))) == false),
			"Invalid m_CachedFree (must be either NULL or within the ring buffer - 0x%08x ring buffer is 0x%08x - 0x%08x)\n",
			m_CachedFree, m_StartEa, m_EndEa );

		// Grab the start and end pointers from the mutex.
		u32 start			= m_StartEa;
		u32 end				= m_EndEa;
		u32 current			= m_CurrentEa;
		u32 currentPlusSize	= current+size;
		u32 startPlusSize	= start+size;
		bool getLabel = m_CachedFree? false: true;


#if __SPU
		u32 labelEa			= (u32)m_RsxLabelAddr;
		uint8_t labelBuffer[0x80] __attribute__((__aligned__(128)));
		u32 *pLabelValue = (u32 *) ((uintptr_t) labelBuffer + (0x7FU & (uintptr_t) labelEa));
#endif // __SPU

#if !__FINAL
		u32 deadman = RSX_DEADMAN_TIMEOUT;
#endif
		for (;;) {
			if (getLabel) {
#if __SPU
				sysDmaSmallGetAndWait(pLabelValue, labelEa, sizeof(*pLabelValue), m_DmaTag);
				m_CachedFree = *pLabelValue;
#else
				m_CachedFree = *m_RsxLabelAddr;
#endif // __SPU
			}

			// Free
			u32 freeEnd = m_CachedFree;

			// Test to see if the allocation will fit.
			if (__builtin_expect((((freeEnd <= current) || (currentPlusSize < freeEnd)) &&
				((currentPlusSize <= end) || (startPlusSize < freeEnd))), true))
			{
				// Check whether we need to flip back to the beginning of the ring buffer
				if (currentPlusSize > end)
				{
					current = start;
					currentPlusSize = startPlusSize;
				}

				m_CurrentEa = currentPlusSize;
				return (void*)current;
			}
			// Now we must read the label to know where the RSX really is
			else if (!getLabel)
			{
				getLabel = true;
			}
			// May want to wait for a bit if this fails...
			else {
#if !__FINAL
				if (!--deadman) {
					Errorf("head=%x tail=%x size=%x/%x hung (label %d)",m_StartEa,m_CurrentEa,(u32)size,GetRingSize(),m_RsxLabelId);

					deadman = RSX_DEADMAN_TIMEOUT;
#ifdef RINGBUFFER_STALL_CALLBACK
					RINGBUFFER_STALL_CALLBACK();
#endif
				}
#endif

				// Wait ~96,000 cycles to avoid flooding the GPU with label
				// reads.  Each read of the SPU decrementer takes 40
				// cycles
				RSX_SPIN_WAIT;
			}
		}
	}

	void *TryAllocate(size_t size) {
		ringAssert(size);
		ringAssert(2*size <= GetRingSize());

		// Add validation for ring buffer free cache since it's a new feature
		ringAssertf( ((m_CachedFree&&((m_CachedFree<m_StartEa)||(m_CachedFree>m_EndEa))) == false),
			"Invalid m_CachedFree (must be either NULL or within the ring buffer - 0x%08x ring buffer is 0x%08x - 0x%08x)\n",
			m_CachedFree, m_StartEa, m_EndEa );

		// Grab the start and end pointers from the mutex.
		u32 start			= m_StartEa;
		u32 end				= m_EndEa;
		u32 current			= m_CurrentEa;
		u32 currentPlusSize	= current+size;
		u32 startPlusSize	= start+size;
		bool getLabel = m_CachedFree? false: true;

		if (getLabel)
		{
#if __SPU
			u32 labelEa			= (u32)m_RsxLabelAddr;
			uint8_t labelBuffer[0x80] __attribute__((__aligned__(128)));
			u32 *pLabelValue = (u32 *) ((uintptr_t) labelBuffer + (0x7FU & (uintptr_t) labelEa));

			sysDmaSmallGetAndWait(pLabelValue, labelEa, sizeof(*pLabelValue), m_DmaTag);
			m_CachedFree = *pLabelValue;
#else
			m_CachedFree = * m_RsxLabelAddr;
#endif // __SPU
		}

		// Free
		u32 freeEnd = m_CachedFree;

		// Test to see if the allocation will fit.
		if (__builtin_expect((((freeEnd <= current) || (currentPlusSize < freeEnd)) &&
			((currentPlusSize <= end) || (startPlusSize < freeEnd))), true))
		{
			// Check whether we need to flip back to the beginning of the ring buffer
			if (currentPlusSize > end)
			{
				current = start;
				currentPlusSize = startPlusSize;
			}

			m_CurrentEa = currentPlusSize;
			return (void*)current;
		}

		return NULL;
	}

	u32 GetOffset(void *ptr) {
		ringAssert((u32)ptr >= m_StartEa && (u32)ptr < m_EndEa);
		return ((u32)ptr - m_StartEa) + m_StartOffset;
	}

	void *GetAddressUnsafe(u32 offset) {
		return (offset >= m_StartOffset && offset < m_StartOffset + GetRingSize())
			? (void*)(m_StartEa + (offset - m_StartOffset)) : NULL;
	}

	void *GetAddress(u32 offset) {
		void *result = GetAddressUnsafe(offset);
		ringAssert(result);
		return result;
	}

	// PURPOSE: Send a command to the GPU indicating the given allocation
	//			is now done.
	void Free(void *firstFree,CellGcmContextData *ctxt) {
 		ringAssert((u32)firstFree >= m_StartEa && (u32)firstFree <= m_EndEa);
		cellGcmSetWriteTextureLabel(ctxt, m_RsxLabelId, (u32)firstFree);
	}


#if __PPU
	void Init(const char* name, void *ringBase,size_t ringSize,u32 labelId,int dmaTag,u32 initAlloc = 0) {
		m_StartEa = (u32)ringBase;
		m_EndEa = m_StartEa + ringSize;
		m_CurrentEa = m_StartEa + initAlloc;

		m_RsxLabelId = labelId;
		m_RsxLabelAddr = cellGcmGetLabelAddress(labelId);
		*m_RsxLabelAddr = 0;

		m_CachedFree = 0;
		m_DmaTag = dmaTag;

		cellGcmAddressToOffset(ringBase,&m_StartOffset);
		safecpy(m_Name, name);
		grcDisplayf("RingBuffer Init: name \"%s\", label %d at %p, buffer at %x, size %x",m_Name,m_RsxLabelId,m_RsxLabelAddr,m_StartEa,GetRingSize());
	}
#endif	// __PPU

	u32 GetRingBeginOffset() const { return m_StartOffset; }

	u32 GetLabelId() const { return m_RsxLabelId; }

	u8* GetRingBegin() const { return (u8*)m_StartEa; }

	u32 GetRingSize() const { return m_EndEa - m_StartEa; }

private:
	u32 m_StartEa;              // address of buffer start
	u32 m_StartOffset;          // RSX offset of buffer start
	u32 m_EndEa;                // address of buffer end (start + size)
	u32 m_CurrentEa;            // address of beginning of buffer free area
	u32 m_RsxLabelId;           // RSX label which is used to track how much data has been read
	u32* m_RsxLabelAddr;        // Address of RSX label used for synchronization
	u32 m_CachedFree;           // Cached free pointer (must be initialized to 0)
	u32 m_DmaTag;

	char m_Name[16];
} ;

#undef ringAssert
#undef ringAssertf

} // namespace rage

#endif // GRCORE_GCMRINGBUFFER_H 
