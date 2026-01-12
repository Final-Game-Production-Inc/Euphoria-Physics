//
// phcore/scratchallocator.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef PHCORE_FRAMEALLOCATOR_H
#define PHCORE_FRAMEALLOCATOR_H

namespace rage {


// FrameAllocator is a very simple frame-based allocator.  Its advantages are that it is very simple, has low overhead, and provides a clean, streamlined interface.
// It is a good choice to use when your allocations have the property that an allocation never has a lifetime greater than the lifetime of the allocation that
//   preceded it.  A simple heap (like sysTinyHeap) works well in this situation too but there you have the added complexity of having to remember what allocations
//   you made so that you can then free them all individually.
// In the context of FrameAllocator, a 'frame' is defined to be a set of allocations that all share the same lifetime.  Frames are defined by 'markers' that you
//   set at various points via SetMarker().  
// A frame allocator is particularly convenient when you have a variable number of allocations to make, perhaps in response to information coming from data that has
//   been DMA'd from the PPU to the SPU.  As a concrete example, if you were attempting to get an entire phBound object onto the SPU you would first DMA over
//   the 'core' part of the phBound.  Then, depending on the type that you find the bound to be (and what you need to do with it), you might still need to DMA over
//   a vertex array, an array of subtree headers, an array of composite bound pointers, or nothing at all.
// With FrameAllocator you simply call SetMarker() at the beginning of your variable number of allocations that share a lifetime and then call ReleaseToLastMarker()
//   afterward when you're done with them.  You don't have to worry about what allocations you may or may not have made between the two points, they will all be
//   released.
// NOTE: As currently written, FrameAllocator does not provide any way for a client to get feedback about an allocation failing due to running out of space in the
//   buffer.  Currently there is just an Assert() that will fail if this happens.  This behavior should probably be changed so that GetBlock() returns NULL in that
//   case.
template <int NumFrames> class FrameAllocator
{
public:
	FrameAllocator(u8 *scratchStart, u32 scratchSize)
	{
		Init(scratchStart, scratchSize);
	}

	void Init(u8 *scratchStart, u32 scratchSize)
	{
		Assert((scratchSize & 15) == 0);
		Assert(((size_t)scratchStart & 15) == 0);
		Assert(scratchStart != NULL);
		Assert(scratchSize > 0);
		m_ScratchStart = scratchStart;
		m_ScratchSize = scratchSize;
		m_ScratchUsed = 0;
		m_MarkersSet = 0;
	}

	~FrameAllocator()
	{
		// If you see this assert you didn't release all of the markers that you set.  This isn't going to cause the FrameAllocator to fail but it's likely
		//   that there's a bug in the client code that's using this class.
		Assert(m_MarkersSet == 0);
	}

	void SetMarker()
	{
		Assert(m_MarkersSet < NumFrames);
		m_Markers[m_MarkersSet] = m_ScratchUsed;
		++m_MarkersSet;
	}

	void ReleaseToLastMarker()
	{
		Assert(m_MarkersSet > 0);
		--m_MarkersSet;
		m_ScratchUsed = m_Markers[m_MarkersSet];
		//		spu_printf("FrameAllocator::ReleaseToLastMarker(): Total used is now %u.\n", m_ScratchUsed);
	}

	void ReleaseAllMarkers()
	{
		m_MarkersSet = 0;
		m_ScratchUsed = 0;
	}

	u8 *GetBlock(u32 blockSize)
	{
		//Assert((blockSize & 15) == 0);
		blockSize = (blockSize + 15) & ~15;
		u8 *retVal = m_ScratchStart + m_ScratchUsed;
		m_ScratchUsed += blockSize;
		//		spu_printf("FrameAllocator::GetBlock(): Requesting %u. Total used is now %u.\n", blockSize, m_ScratchUsed);
		Assertf(m_ScratchUsed <= m_ScratchSize, "m_ScratchUsed = %d  m_ScratchSize = %d  blockSize = %d", m_ScratchUsed, m_ScratchSize, blockSize);
		return retVal;
	}

	u8 *GetBlockSafe(u32 blockSize)
	{
		//Assert((blockSize & 15) == 0);
		blockSize = (blockSize + 15) & ~15;
		u8 *retVal = m_ScratchStart + m_ScratchUsed;
		m_ScratchUsed += blockSize;
		//		spu_printf("FrameAllocator::GetBlock(): Requesting %u. Total used is now %u.\n", blockSize, m_ScratchUsed);
		if (Likely(m_ScratchUsed <= m_ScratchSize))
			return retVal;
		else
			return NULL;
	}

	// !__FINAL so that you're not tempted to 'work around' bugs related to bad pointers instead of fixing them.
#if !__FINAL
	// Use this when you have a pointer that you believe should point into the currently allocated portion of the buffer.
	// This function is deprecated.
	__forceinline void ValidatePointer(const void *ASSERT_ONLY(pointer)) const
	{
		Assert(IsValidPointer(pointer));
	}

	// Use this to check if a pointer is taken from the currently allocated portion of the buffer.
	__forceinline bool IsValidPointer(const void *pointer) const
	{
		return (pointer >= m_ScratchStart) && (pointer < (m_ScratchStart + m_ScratchUsed));
	}
#endif	// !__FINAL

private:
	u8	*m_ScratchStart;
	u32	m_ScratchSize;

	u32	m_ScratchUsed;

	u32	m_Markers[NumFrames];
	u8	m_MarkersSet;
};


} // namespace rage

#endif // PHCORE_FRAMEALLOCATOR_H