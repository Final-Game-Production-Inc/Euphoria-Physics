#include "grcore/wrapper_gcm.h"
#include "rmcore/spuevents.h"
#include "system/dma.h"

#define DUMP_FIFO (0)

CellGcmContextData ctxt ;

using namespace rage;

static u32 gcmFlushOffset;

spuGcmState *pSpuGcmState;

CellGcmControl s_Control ;
qword vramDummy;

#ifdef RINGBUFFER_STALL_CALLBACK
static void gcmDisplayGetStatus()
{
	if( pSpuGcmState->isGPADCapturing ) // replay capture..
		return;

	const u32 data[] = {(u32)pSpuGcmState};
	si_dsync();
	sys_spu_thread_send_event(SYSTASKMANAGER_EVENT_PORT, EVENTCMD_RSX_STALL, (u32)data);
	spu_readch(SPU_RdInMbox);
}
#endif

void* gcmAllocateVram(uint32_t &outOffset,uint32_t size)
{
	// Round allocation size upward (to strictest alignment necessary)
	size = (size + 127) & ~127;
	spuGcmState *pSpuGcmState = ::pSpuGcmState;

#if !__PROFILE && !__FINAL
	uint32_t stallTime = ~spu_readch(SPU_RdDec);
#endif

	void *result = pSpuGcmState->FragmentFifo.Allocate(size);
	if (pSpuGcmState->LastFragment)
		pSpuGcmState->FragmentFifo.Free(pSpuGcmState->LastFragment, GCM_CONTEXT);
	outOffset = pSpuGcmState->FragmentFifo.GetOffset(result);
	pSpuGcmState->LastFragment = (u8*) result;

#if !__PROFILE && !__FINAL
	stallTime = ~spu_readch(SPU_RdDec) - stallTime;
	pSpuGcmState->FragmentStallTime += stallTime;
#endif
	return result;
}

void gcmPartialFlushInternal(CellGcmContextData *data,u32 jumpDest,bool doKick)
{
	u32 *const begin = data->begin;
	DEV_ONLY(u32 *const end = data->end;)
	u32 *current = data->current;
	u32 stopOffset = (u32)current - (u32)begin;
	spuGcmState *pSpuGcmState = ::pSpuGcmState;

	// If we're doing a kick, then we need to insert the jump to the next
	// segment, then nops up to the next qword boundary
	//     if (doKick) *current++ = CELL_GCM_JUMP(jumpDest);
	//     while ((u32)current & 15) *current++ = 0;
	//
	qword cmdQuad = si_lqd(si_from_ptr(current), 0);
	// Create 128-bit mask from doKick
	qword doKickMask = si_from_int(doKick);
	doKickMask = si_shufb(doKickMask, doKickMask, si_ilh(0x0303));
	doKickMask = si_clgti(doKickMask, 0);
	// Build shuffle control word for inserting jump command.  Even if
	// doKick is false, we still insert the jump.  This is safe because either
	// we are still inside the local store copy of the command buffer, or if we
	// are right at the end, then doKick must be true.
	qword cwd = si_cwd(si_from_ptr(current), 0);
	// Nop out the remainder of the qword, since we are going to dma out a 16-byte multiple.
	CompileTimeAssert(CELL_GCM_METHOD_NOP == 0);
	qword nopMask = si_rotqmby(si_il(-1), si_sfi(si_andi(si_from_ptr(current), 15), 0));
	cmdQuad = si_andc(cmdQuad, nopMask);
	// Insert either a CELL_GCM_METHOD_NOP or a jump
	CompileTimeAssert((CELL_GCM_METHOD_FLAG_JUMP & 0xffff) == 0);
	qword jmp = si_or(si_from_uint(jumpDest), si_ilhu(CELL_GCM_METHOD_FLAG_JUMP>>16));
	qword jmpOrNop = si_and(jmp, doKickMask);
	cmdQuad = si_shufb(jmpOrNop, cmdQuad, cwd);
	si_stqd(cmdQuad, si_from_ptr(current), 0);
	current = (u32*)si_to_ptr(si_a(si_from_ptr(current), si_andi(doKickMask, 4)));
	current = (u32*)si_to_ptr(si_andi(si_ai(si_from_ptr(current), 15), -16));

#if __DEV
	if (current > end + 1)
		Quitf("segment overrun stopoffset=%u begin=%p current=%p end=%p jumpDest=%x",stopOffset,begin,current,end,jumpDest);
#endif

	u32 thisOffset = (u32)current - (u32)begin;

	// grcDisplayf("DMA to %p (%d bytes)",(char*)pSpuGcmState->NextSegment + gcmFlushOffset,thisOffset - gcmFlushOffset);

	// This dma put of the rsx command buffer is on the same tag as the fenced
	// put of the Edge jobs to the job chain.  This ensures that the Edge job
	// that will overwrite the local stall holes we are flushing here, cannot
	// start until the command buffer is actually in xdr.  See
	// grcGeometryJobs::AddJob().
	sysDmaPut((char*)begin + gcmFlushOffset,(uint32_t)pSpuGcmState->NextSegment + gcmFlushOffset,thisOffset - gcmFlushOffset,FIFOTAG);
	gcmFlushOffset = thisOffset;
	data->current = current;

	if (doKick) {
		// Read VRAM back to enforce data ordering (must be to static var since we don't wait)
		if (pSpuGcmState->FragmentCacheLocation == CELL_GCM_LOCATION_LOCAL)
			sysDmaGetf(&vramDummy,(uint32_t)pSpuGcmState->FragmentFifo.GetRingBegin(),16,FIFOTAG);

		// Update put ptr with a fenced dma on the same tag
		// We run up to, but not including, the jump out of this segment so that we know any readahead will
		// have been cleared.
		s_Control.put = pSpuGcmState->NextSegmentOffset + stopOffset;
		sysDmaSmallPutf((void*)&s_Control.put,(uint32_t)&pSpuGcmState->Control->put,sizeof(uint32_t),FIFOTAG);
	}
}


int32_t gcmCallback(CellGcmContextData *data,uint32_t /*amt*/)
{
	using namespace rage;

	// Allocate the next segment now so that the segment we're finishing knows where to jump.
	void *nextSegment = pSpuGcmState->NextNextSegment;
	u32 nextSegmentOffset = pSpuGcmState->CommandFifo.GetOffset(nextSegment);
	spuGcmState *pSpuGcmState = ::pSpuGcmState;

	// Transmit any segment data that we didn't already send down before for EDGE synchronization
	gcmPartialFlushInternal(data,nextSegmentOffset,true);
//	TrapNE((int)pSpuGcmState, (int)::pSpuGcmState);
//	pSpuGcmState = ::pSpuGcmState;

#if DUMP_FIFO
	gcm::DumpFifo(data->begin,data->current);
#endif

	// Reset buffer.  Note that unlike on the host, we DO reserve space for the jump here.
	// Note that we do some allocation from the end of the FIFO segment downward (it's DMA'd out separately)
	// so we have to reset the end pointer now as well.
	gcmFlushOffset = 0;
	data->current = data->begin;
	data->end = (uint32_t*)((char*)data->begin + SPU_FIFO_SEGMENT_SIZE - sizeof(uint32_t));

	// Remember the PPU/DMA address of where the next segment will go, and its RSX offset.
	pSpuGcmState->NextSegment = nextSegment;
	pSpuGcmState->NextSegmentOffset = nextSegmentOffset;

#if !__PROFILE && !__FINAL
	uint32_t stallTime = ~spu_readch(SPU_RdDec);
#endif

	// Must wait for dma completion so that the local store doesn't get
	// overwriten.  Since both this mfc tag wait, and the CommandFifo.Allocate
	// below are blocking, may as well do this one first, to potentially reduce
	// RSX local store bandwidth usage, and general SPU power consumption.
	sysDmaWaitTagStatusAll(1<<FIFOTAG);

	// Allocate the next segment after this one is kicked to minimize chances of deadlock
	pSpuGcmState->NextNextSegment = pSpuGcmState->CommandFifo.Allocate(SPU_FIFO_SEGMENT_SIZE);

#if !__PROFILE && !__FINAL
	stallTime = ~spu_readch(SPU_RdDec) - stallTime;
	pSpuGcmState->FifoStallTime += stallTime;
#endif

	// Mark previous fifo segment as being free (at the beginning of the new
	// segment).  Note that this function writes to the RSX command buffer in
	// local store, so it must be after the mfc tag wait.
	pSpuGcmState->CommandFifo.Free(nextSegment,data);

	DRAWABLESPU_STATS_ADD(RingBufferUsed,SPU_FIFO_SEGMENT_SIZE >> 10);
	return CELL_OK;
}

void gcmPartialFlush(CellGcmContextData *data)
{
	// If we're pretty close to the end of the segment, just do a normal flush
	// Otherwise we might exactly fill the segment with the partial flush and
	// be unable to do the real callback later.
	if (data->current + 4 >= data->end)
		gcmCallback(data,0);
	else
		gcmPartialFlushInternal(data,pSpuGcmState->NextSegmentOffset + ((u32)data->current - (u32)data->begin) + 4,false);
}

