// 
// grcore/edgeringbuffer.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EDGERINGBUFFER_H
#define GRCORE_EDGERINGBUFFER_H

#if __SPU
#if __ASSERT
#define EDGE_GEOM_DEBUG
#endif // __ASSERT
#include "edge/geom/edgegeom.cpp"

namespace rage
{

bool AllocateEdgeRingBuffer(u32 outputBufferInfoEa, u8 inputDmaTag, u8 outputDmaTag, u32 allocSize, EdgeGeomAllocationInfo *info, u32 gcmControlEa = 0)
{
	EdgeGeomSpuContext ctx;
	ctx.inputDmaTag = inputDmaTag;
	ctx.outputDmaTag = outputDmaTag;
	ctx.gcmControlEa = gcmControlEa;

	// This will be set to true if this info is passed to OutputIndexes or OutputVertexes
	info->isVertexData = false;
	info->wrapped = false;

	// round up to a multiple of 128 bytes.  This is ensures that all
	// output DMAs start on a 128 byte boundary, which is important
	// both for performance (DMA bandwidth is halved if the source and
	// destination addresses aren't at identical mod 128) and
	// correctness (the RSX reads data in 128-byte blocks, and padding
	// the allocations forces the RSX to fetch the data for each draw
	// call separately, avoiding several nasty cache issues).
	allocSize = (allocSize + 0x7F) & ~0x7F; 

	bool interruptsEnabled = spu_readch(SPU_RdMachStat) & 1;
	spu_idisable();

	// Load the ring buffer info
	static uint8_t ringInfoBuffer[0x80] __attribute__((__aligned__(128)));
	EdgeGeomRingBufferInfo volatile *ringInfo = (EdgeGeomRingBufferInfo volatile *)ringInfoBuffer;
	uint32_t spuId = cellSpursGetCurrentSpuId();
	const uint32_t ringInfoEa = outputBufferInfoEa + sizeof(EdgeGeomSharedBufferInfo)
		+  spuId * sizeof(EdgeGeomRingBufferInfo);

	// Atomic-get the ring buffer info
	// This is an abuse of the atomic DMA as we're interested in SL1, not atomic access
	spu_writech(MFC_LSA, (u32)ringInfo);
	spu_writech(MFC_EAL, ringInfoEa);
	spu_writech(MFC_Cmd, MFC_GETLLAR_CMD);
	spu_readch(MFC_RdAtomicStat);

	// has ring buffer?
	Assertf(ringInfo->endEa - ringInfo->startEa > 0,"bad ring buffer %u - %u (%d)",ringInfo->startEa,ringInfo->endEa,cellSpursGetCurrentSpuId()-2);

	// Attempt to allocate from the ring buffer once.  waitForGpu
	// is set to false, so if the allocation wouldn't succeed
	// we'll return immediately instead of blocking.
	bool success = allocateOutputSpaceFromRingBuffer(&ctx, (EdgeGeomRingBufferInfo*)ringInfo, allocSize, false, info);
	if (!success)
	{
		success = allocateOutputSpaceFromRingBuffer(&ctx, (EdgeGeomRingBufferInfo*)ringInfo, allocSize, true, info);
	}

	// Store the ring buffer info back to main memory
	// Unconditional put from SL1 (this cache line is private - no need for real atomic op)
	spu_writech(MFC_LSA, (u32)ringInfo);
	spu_writech(MFC_EAL, ringInfoEa);		
	spu_writech(MFC_Cmd, MFC_PUTLLUC_CMD);
	spu_readch(MFC_RdAtomicStat);

	if (interruptsEnabled)
	{
		spu_ienable();
	}

	// the allocation failed?
	if (!success)
	{
		return false; 
	}

	// Set some info if the allocation succeeded and is
	// from a ring buffer.
	info->rsxLabelEa = ringInfo->rsxLabelEa;
	info->endEa = info->ea + allocSize;

	return true;
}

} // namespace rage
#endif // __SPU

#endif // GRCORE_EDGERINGBUFFER_H