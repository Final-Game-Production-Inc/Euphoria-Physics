//
// physics/spucachearray.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_SPUCACHEARRAY_H
#define PHYSICS_SPUCACHEARRAY_H

#if __SPU
#include "system/dma.h"
#include <cell/dma.h>

#include "system/memops.h"

namespace rage {

class SpuCacheArrayBase
{
public:
	SpuCacheArrayBase(size_t sizeofT, int n, const uint32_t* dmaTags, const uint32_t* dmaMasks, void* cache, void** mmAddresses, int* cacheRefs);

	void LockEmptySlotKickDmaRead(int slot, void* mmAddress);

	// NOTES: NULL is explicitly handled as a valid input for mmAddress. The "-1" slot will be returned,
	//        and calls to WaitDmaRead will return NULL.
	int LockSlotKickDmaRead(void* mmAddress);

	void UnlockSlot(int slot);

	void AssertAllSlotsUnlocked();

protected:
	void* WaitDmaReadBase(int slot);
	void WaitAllDmaWriteBase();

	void UnlockSlotKickDmaWriteBase(int slot, size_t bytes);

private:
	static const int NOT_FOUND = -1;

	__forceinline uint32_t DmaTagR(int slot) { return m_DmaTags[slot * 2];	}
	__forceinline uint32_t DmaTagW(int slot) { return m_DmaTags[slot * 2 + 1]; }
	__forceinline uint32_t DmaMaskR(int slot) { return m_DmaMasks[slot * 2]; }
	__forceinline uint32_t DmaMaskW(int slot) { return m_DmaMasks[slot * 2 + 1]; }
	__forceinline uint32_t DmaMaskRW(int slot) { return DmaMaskR(slot) | DmaMaskW(slot); }

	int FindAddress(void* mmAddress);
	void FindEmptySlot(int& foundSlot);

	size_t m_SizeofT;
	int m_N;
	const uint32_t* m_DmaTags;
	const uint32_t* m_DmaMasks;
	u8* m_Cache;
	void** m_MmAddresses;
	int* m_CacheRefs;	
};

template <class T, int N>
class SpuCacheArray : public SpuCacheArrayBase
{
public:
	SpuCacheArray(const uint32_t* dmaTags, const uint32_t* dmaMasks)
		: SpuCacheArrayBase(sizeof(T), N, dmaTags, dmaMasks, m_Cache, (void**)m_MmAddresses, m_CacheRefs)
	{
	}

	T* WaitDmaRead(int slot)
	{
		return (T*)WaitDmaReadBase(slot);
	}

	void WaitAllDmaWrite()
	{
		WaitAllDmaWriteBase();
	}

	void UnlockSlotKickDmaWrite(int slot, size_t bytes = sizeof(T))
	{
		UnlockSlotKickDmaWriteBase(slot, bytes);
	}

	T* GetMMAddrForSlot(int slot)
	{
		Assert(slot < N);
		return m_MmAddresses[slot];
	}

private:
	// Align up to next highest 16-byte boundary.
	static const int ALIGNED_SIZE = (sizeof(T) + 15) & ~15;

	u8 m_Cache[N][ALIGNED_SIZE] ;
	T* m_MmAddresses[N] ; // Aligned so that we can load these addresses in a quadword and check four at a time.
	int m_CacheRefs[N];
};

__forceinline void SpuCacheArrayBase::UnlockSlot(int slot)
{
	if (slot >= 0)
	{
		FastAssert(slot < m_N);
		m_CacheRefs[slot]--;

		if (m_CacheRefs[slot] == 0)
		{
			// Write zero into the slot so we'll load from mm the next time this data is accessed
			m_MmAddresses[slot] = 0;
		}
	}
}

__forceinline void* SpuCacheArrayBase::WaitDmaReadBase(int slot)
{
	if (slot < 0)
	{
		return NULL;
	}

	FastAssert(slot < m_N);
	FastAssert(DmaTagR(slot)); // Can't read if a zero tag was provided

	// Wait for the oustanding DMA on tag corresponding to bit set in 'DmaMaskR(slot)' to complete.
	cellDmaWaitTagStatusAny(DmaMaskR(slot));

	// Return the cached data.
	return m_Cache + m_SizeofT * slot;
}

__forceinline void SpuCacheArrayBase::WaitAllDmaWriteBase()
{
	// Wait for all the oustanding DMA writes to complete
	for (int slot = 0; slot < m_N; ++slot)
	{
		cellDmaWaitTagStatusAny(DmaMaskW(slot));
	}
}

__forceinline void SpuCacheArrayBase::UnlockSlotKickDmaWriteBase(int slot, size_t bytes)
{
	if (slot >= 0)
	{
		FastAssert(slot < m_N);
		FastAssert(DmaTagW(slot)); // Can't write if a zero tag was provided

		// Decrease ref count.
		m_CacheRefs[slot]--;

		// Only write out to memory if the ref count is zero.
		if (m_CacheRefs[slot] == 0)
		{
			cellDmaLargePut(m_Cache + m_SizeofT * slot, (uint64_t)m_MmAddresses[slot], bytes, DmaTagW(slot), 0, 0);

			// Write zero into the slot so we'll load from mm the next time this data is accessed
			m_MmAddresses[slot] = 0;
		}
	}
}


} // namespace rage

#endif // __SPU

#endif	// PHYSICS_SPUCACHEARRAY_H
