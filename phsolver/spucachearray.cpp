//
// physics/spucachearray.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#if __SPU
#include "spucachearray.h"

namespace rage {

SpuCacheArrayBase::SpuCacheArrayBase(size_t sizeofT, int n, const uint32_t* dmaTags, const uint32_t* dmaMasks, void* cache, void** mmAddresses, int* cacheRefs)
	: m_SizeofT((sizeofT + 15) & ~15)
	, m_N(n)
	, m_DmaTags(dmaTags)
	, m_DmaMasks(dmaMasks)
	, m_Cache((u8*)cache)
	, m_MmAddresses(mmAddresses)
	, m_CacheRefs(cacheRefs)
{
	sysMemSet(m_CacheRefs, 0, m_N * sizeof(int));
}

void SpuCacheArrayBase::LockEmptySlotKickDmaRead(int slot, void* mmAddress)
{
	Assert(m_CacheRefs[slot] == 0);

	// DMA from mem into ls.
	// *(m_Cache+m_SizeofT*foundSlot) = *(mmAddress);
	cellDmaWaitTagStatusAll(DmaMaskW(slot));
	cellDmaLargeGet(m_Cache + m_SizeofT * slot, (uint64_t)mmAddress, m_SizeofT, DmaTagR(slot), 0, 0);
	m_MmAddresses[slot] = mmAddress;

	m_CacheRefs[slot]++;
}

int SpuCacheArrayBase::LockSlotKickDmaRead(void* mmAddress)
{
	// Return "slot not found" if ppu address passed is NULL.
	if (Unlikely(mmAddress == NULL))
	{	
		return NOT_FOUND;
	}

	// Look for the address in the cache.
	int foundSlot = FindAddress(mmAddress);

	// Most of the time, we'll assume that the content at a specific address is already in the cache.
	// So, it's unlikely that a slot with the address is not found.
	if (Unlikely(foundSlot == NOT_FOUND))
	{
#if __ASSERT && 0
		// If all the slots are locked when this is called, that's a user error
		bool someSlotUnlocked = false;
		for (int slot = 0; slot < m_N; ++slot)
		{
			if (Unlikely(m_CacheRefs[slot] == 0))
			{
				someSlotUnlocked = true;
				break;
			}
		}
		Assert(someSlotUnlocked);
#endif

		// Find a slot with:
		// 1) Zero cache refs, and
		// 2) Zero outstanding DMA reads or writes.
		FindEmptySlot(foundSlot);

		//Assert(foundSlot >= 0 && foundSlot < m_N);
		//Assert(DmaTagR(foundSlot)); // Can't read if a zero tag was provided
		//Displayf("DMA: ls 0x%p ea 0x%p size 0x%x tag %d", m_Cache + m_SizeofT * foundSlot, mmAddress, (uint32_t)m_SizeofT, DmaTagR(foundSlot));
		LockEmptySlotKickDmaRead(foundSlot, mmAddress);
	}
	else
	{
		// This slot is now being used, and needs either a:
		// 1) UnlockSlot(), or a
		// 2) UnlockSlotKickDmaWrite().
		m_CacheRefs[foundSlot]++;
	}

	return foundSlot;
}


void SpuCacheArrayBase::AssertAllSlotsUnlocked()
{
#if __ASSERT
	for (int slot = 0; slot < m_N; ++slot)
	{
		Assert(m_CacheRefs[slot] == 0);
	}
#endif
}

int SpuCacheArrayBase::FindAddress(void* mmAddress)
{
	const u32 CONST_N = (u32)m_N;

	if( Likely( mmAddress != NULL ) )
	{
		vec_uint4 addrToCompare = spu_splats( (u32)mmAddress );
		for (u32 vecPos = 0; vecPos < (CONST_N+3)/4; ++vecPos)
		{
			// Get four addresses at a time.
			vec_uint4 fourAddrs = ((vec_uint4*)(m_MmAddresses))[vecPos];

			// Compare the four addresses to the one we are trying to lookup.
			vec_uint4 cmpResult = spu_cmpeq( fourAddrs, addrToCompare );

			// Gather the result into the four LSb's of 'gatheredResult'.
			vec_uint4 gatheredResult = spu_gather( cmpResult );

			// Count the number of leading zeros. This will be anywhere from 28 to 32 (32 if no match).
			vec_uint4 numLeadingZeros = spu_cntlz( gatheredResult );

			// # of leading zeros.
			u32 scalarNumLeadingZeros = spu_extract( numLeadingZeros, 0 );

			// [28-32] minus 28 gives us the range [0 - 4] (4 if no match).
			u32 scalarSlotNum = scalarNumLeadingZeros - 28;

			// We want to continue iterating if:
			//
			// We found nothing this loop (scalarSlotNum==4)
			if( Likely( scalarSlotNum == 4 ) )
			{
				continue;
			}

			// Find the global slot location we've matched (not just the slot within this 4-tuple).
			u32 scalarGlobalSlotNum = 4*vecPos + scalarSlotNum;

			// If we did find something:
			//
			// It's either a false match ( scalarGlobalSlotNum >= m_N ), or
			// we should return it.
			if( Likely( scalarGlobalSlotNum < CONST_N ) ) // valid
			{
				return scalarGlobalSlotNum;
			}
			else // false match (this can only happen if m_N is not a multiple of 4)
			{
				return NOT_FOUND;
			}
		}
	}

	return NOT_FOUND;
}


void SpuCacheArrayBase::FindEmptySlot(int& foundSlot)
{
	// TODO: some kind of LRU algorithm?

#if 0 // Disable this version, the other one seems to be working better
	// A mask, the union the DMA tags of the currently unlocked cache slots
	uint32_t idleMask = 0;
	for (int slot = 0; slot < m_N; ++slot)
	{
		if (m_CacheRefs[slot] == 0)
		{
			const uint32_t dmaMask = DmaMaskRW(slot);

			idleMask |= dmaMask;
		}
	}

	// See if any of the slots are open and done with their DMAs
	uint32_t allDmasDone = 0;
	do
	{
		// Wait until one or more of the currently idle slots is done with its DMA
		uint32_t dmaDoneMask = cellDmaWaitTagStatusAny(idleMask);
		allDmasDone |= dmaDoneMask;

		// If we don't find a R/W pair that are both idle, we'll have to wait again

		// Once we've checked for R/W pairs after a particular channel completes, don't wait on it next round
		idleMask &= ~dmaDoneMask;

		for (int slot = 0; slot < m_N; ++slot)
		{
			if (m_CacheRefs[slot] == 0)
			{
				const uint32_t dmaMask = DmaMaskRW(slot);
				if (allDmasDone & dmaMask)
				{
					foundSlot = slot;
					break;
				}
			}
		}

	}
	while (foundSlot == NOT_FOUND);
#else

#define DEBUGGA 0

	// Loop until a slot is open and done with its DMA.
	do
	{
		// Try every slot.
		for (int slot = 0; slot < m_N; ++slot)
		{
#if DEBUGGA
			static int loopIterAvg = 0;
			static int loopIters = 0;
#endif
			// Is the slot open?
			if (m_CacheRefs[slot] == 0)
			{
				// Are all outstanding DMA reads and writes done?
				const uint32_t dmaMask = DmaMaskRW(slot);
				if (cellDmaWaitTagStatusImmediate(dmaMask) == dmaMask)
				{
					// Let's use this slot then.
					foundSlot = slot;

#if DEBUGGA
					loopIterAvg += slot;
					loopIters++;
					if( loopIters == 1000 )
					{
						float avg = 1.0f*loopIterAvg/1000;
						Printf( "Avg # of loop iters to find empty slot: %f\n", avg );
						loopIterAvg = loopIters = 0;
					}
					if( foundSlot >= 5 )
					{
						Printf( "Warning: foundSlot is gt five... = %i...\n", foundSlot );
					}
#endif
					break;
				}
			}
		}
	}
	while (foundSlot == NOT_FOUND);
#endif
}


} // namespace rage

#endif // __SPU
