// 
// grcore/edge_jobs.cpp 
// 
// Copyright (C) 2007-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/effect_config.h"
#include "grcore/edge_jobs.h"
#include "grcore/channel.h"

#define EDGE_JOBS_MANAGEMENT_ON_SPU					(USE_EDGE && SPU_GCM_FIFO)			// whether edge_jobs is running as a SPU spurs task or as a PPU thread
#define	EDGE_JOBS_EVENT_POLLING_JOB_HACK			(1)									// cellSpursEventFlagWait can only work if called from a task

#if USE_EDGE
#include <cell/spurs.h>
#include <cell/atomic.h>

#if __PPU
#include <cell/gcm.h>
#include <sys/time_util.h>
#endif //__PPU

#include "edge/geom/edgegeom_structs.h"

#include "system/task.h"

#include "grcore/wrapper_gcm.h"
#include "grcore/grcorespu.h"
#if !__SPU
#include "grcore/viewport.h"
#endif
#include "grcore/edgegeomspu.h"
#include "grcore/matrix43.h"
#include "system/cache.h"
#include "system/nelem.h"
#include "system/spinlock.h"
#include "system/task.h"

using namespace rage;
using namespace rage::edgegeomspujob;

#include "grcore/edge_callbacks.cpp"

#if __PPU

DECLARE_TASK_INTERFACE(edgegeomspu);

#if HACK_GTA4
	DECLARE_TASK_INTERFACE(edgeExtractGeomSpu_);
	#include "grcore/edgeExtractgeomspu.h"
#endif

#endif //__PPU

#define CELL_CHECK(x)	{ int result = (x); if (result) Quitf("%s failed, code %x (file: %s line: %d)",#x,result,__FILE__,__LINE__); }

#if __SPU
#	define SPU_UINTPTR_CAST(a) (uintptr_t)((a))
#else //!__SPU
#	define SPU_UINTPTR_CAST(a) ((a))
#endif //__SPU

// Sanity checks - make sure main edge structures have the right size
CompileTimeAssert((sizeof(EdgeGeomPpuConfigInfo)&0x0F)==0x00);
CompileTimeAssert((sizeof(EdgeGeomSpuConfigInfo)&0x0F)==0x00);
CompileTimeAssert((sizeof(EdgeGeomSharedBufferInfo)&0x7F)==0x00);
CompileTimeAssert((sizeof(EdgeGeomRingBufferInfo)&0x7F)==0x00);
CompileTimeAssert((sizeof(EdgeGeomOutputBufferInfo)&0x7F)==0x00);

#if __SPU

/*! To prevent a race condition where cellSpursRunJobChain increases the
 *  workload ready count, before the dma put of the new job command at the end
 *  of this function completes, we first overwrite the END command with a JTS
 *  command.  Unlike END, a JTS will not reduce cause the workload ready count
 *  to be reduced to zero.
 *
 *  \param  eaJobChainEnd  Effective address of the most recent END command in the job chain.
 */
static __forceinline void JobChainJts(u64 *eaJobChainEnd)
{
	// This put needs to be fenced to ensure that it occurs after the put of the
	// END it is overwriting.
	static const u64 s_cmdJts[2]  = {CELL_SPURS_JOB_COMMAND_JTS, CELL_SPURS_JOB_COMMAND_JTS};
	sysDmaSmallPutf((char*)&s_cmdJts+((u32)(eaJobChainEnd)&15), (u32)(eaJobChainEnd), 8, FIFOTAG);
}

/*! Place a new job command at the end of the job chain.  Before calling this,
 *  there must be a call to JobChainJts, a wait on FIFOTAG completion.
 *
 *  \param  eaJobChain      Effective address of the job chain struct.
 *  \param  eaJobChainEnd   Effective address of the most recent JTS command in the job chain (writen by JobChainJts).
 *  \param  eaJobChainNext  Effective address of the next command in the job chain after the JTS.
 *  \param  eaJob           Effective address of the job to be added to the job chain.
 */
static __forceinline void JobChainAddJob(CellSpursJobChain *eaJobChain, u64 *eaJobChainJts, u64 *eaJobChainNext, u32 eaJob)
{
	static const u64 s_cmdEnd[2]  = {CELL_SPURS_JOB_COMMAND_END, CELL_SPURS_JOB_COMMAND_END};
	static qword s_cmdJob;
	static qword shuf0A0A = {
		0x80,0x80,0x80,0x80, 0x00,0x01,0x02,0x03, 0x80,0x80,0x80,0x80, 0x00,0x01,0x02,0x03
	};
	CompileTimeAssert(CELL_SPURS_JOB_COMMAND_JOB(0xfffffff8) == 0xfffffff8);
	Assert(CELL_SPURS_JOB_COMMAND_JOB(eaJob) == eaJob);

	// Code must have already waited on completion of FIFOTAG since the last
	// call to this function.
	s_cmdJob = si_shufb(si_from_uint(eaJob), si_from_uint(eaJob), shuf0A0A);

	// Write the next END command to the job chain before we write the new
	// job command.  This dma put does not need to be orderred with respect
	// to other dmas, since the command cannot be executed until the job
	// command is writen.
	sysDmaSmallPut ((char*) s_cmdEnd+((u32)(eaJobChainNext)&15), (u32)(eaJobChainNext), 8, FIFOTAG);

	// Fenced put of the job command.  The fence is for orderring to ensure
	// it is after the new job chain END command, and all the input data for
	// the job.
	sysDmaSmallPutf((char*)&s_cmdJob+((u32)(eaJobChainJts)&15),  (u32)(eaJobChainJts),  8, FIFOTAG);

	// Set ready count (always done to avoid hang since GuardNotify doesn't
	// guarantee the list is running).
	CELL_CHECK(cellSpursRunJobChain(SPU_UINTPTR_CAST((eaJobChain))));
}

#endif // __SPU

/*! Adds a 16-byte aligned local stall hole to the given GCM context and returns the address.
 *
 * 	\param	thisContext		
 *	\param	holeSize		The hole size in bytes.
 *
 *	Important: this function can't *currently* be called from SPU (EA vs LS align + use 
 *	of cellGcmAddressToOffset)
 */ 
void* grcGeometryJobs::SetLocalStallHole(CellGcmContextData *ctx, u32 holeSize, u32 extraHoleSize /*= 0*/, const spuGcmState* gcmState /*= NULL*/)
{
	(void)sizeof(gcmState); // potentially unused

	void* holeEa = NULL;

	// check the reserve
	const u32 wordsRequired = ((holeSize + extraHoleSize) >> 2) + 3;

	if(ctx->current + wordsRequired > ctx->end)
	{
		// TODO: some sort of verify here?
		if(!ctx->callback || (*ctx->callback)(ctx, wordsRequired) != CELL_OK) {
			Printf("cellGcmSetLocalStallHole failed!\n");
			return NULL;
		}
	}
	else	// Already guaranteed aligned if we had to invoke the callback.
		ALIGN_CONTEXT_TO_16(ctx);

	// record the start and end
	holeEa = ctx->current;
	u32 holeEnd = (u32)holeEa + holeSize;

	// fill with local stalls once at the start and at the start of every 128 byte boundary
	u32 jumpOffset;
#if __SPU && SPU_GCM_FIFO
	FastAssert(gcmState);
	jumpOffset = gcmState->NextSegmentOffset + ((u32)ctx->current - (u32)ctx->begin);
#else
	cellGcmAddressToOffset(ctx->current, &jumpOffset);
#endif
	cellGcmSetJumpCommandUnsafeInline(ctx, jumpOffset);
	u32 nextJ2S = ((u32)holeEa + 0x80) & ~0x7F;
	while(nextJ2S < holeEnd)
	{
#if !__SPU
		__dcbz( (void*)nextJ2S ); // prefetching here improves this loop's performance significantly
#endif
		ctx->current = (u32*)nextJ2S;
#if __SPU && SPU_GCM_FIFO
		jumpOffset = gcmState->NextSegmentOffset + ((u32)ctx->current - (u32)ctx->begin);
#else
		cellGcmAddressToOffset(ctx->current, &jumpOffset);
#endif
		cellGcmSetJumpCommandUnsafeInline(ctx, jumpOffset);
		nextJ2S = ((u32)ctx->current + 0x80) & ~0x7F;
	}
	ctx->current = (u32*)holeEnd;

	// return the start of the hole
#if __SPU && SPU_GCM_FIFO
	return (char*)gcmState->NextSegment + ((u32)holeEa - (u32)ctx->begin);
#else
	return holeEa;
#endif
}


#if __SPU && HACK_GTA4_MODELINFOIDX_ON_SPU
//
// assert if some dma lists look incorrect/suspicious
// better to assert here than have SPU halted in edgegeomspu, etc.
// allows to early detect problems with wrong EdgeGeom data, etc.
//
static
void VerifyDma(spuGcmState *pGcmState, Dma *dmaListElement, const char *dmaListName)
{
#if __ASSERT

	if(dmaListElement->Ea)
	{	// ea is not NULL:
		Assertf(dmaListElement->Ea > (256*1024),"edge_jobs: Bad dma list: %s (ea:0x%X, size:%d), GtaModelIndex: %d, GtaRenderPhaseID: %d. Possibly caused by corrupt EdgeGeom data.", dmaListName, dmaListElement->Ea, dmaListElement->Size, pGcmState->gSpuGta4DebugInfo.gta4ModelInfoIdx, pGcmState->gSpuGta4DebugInfo.gta4RenderPhaseID);
		if(dmaListElement->Ea <= (256*1024))
			spu_printf("edge_jobs: Bad dma list: %s (ea:0x%X, size:%d), GtaModelIndex: %d, GtaRenderPhaseID: %d. Possibly caused by corrupt EdgeGeom data.", dmaListName, dmaListElement->Ea, dmaListElement->Size, pGcmState->gSpuGta4DebugInfo.gta4ModelInfoIdx, pGcmState->gSpuGta4DebugInfo.gta4RenderPhaseID);
//		Assertf(dmaListElement->Size!=0, "edge_jobs: Bad dma list for %s (ea:0x%X, size:%d), GtaModelIndex: %d, GtaRenderPhaseID: %d.", dmaListName, dmaListElement->Ea, dmaListElement->Size, pGcmState->gSpuGta4DebugInfo.gta4ModelInfoIdx, pGcmState->gSpuGta4DebugInfo.gta4RenderPhaseID);
//		if(dmaListElement->Size==0)
//			spu_printf("edge_jobs: Bad dma list for %s (ea:0x%X, size:%d), GtaModelIndex: %d, GtaRenderPhaseID: %d.", dmaListName, dmaListElement->Ea, dmaListElement->Size, pGcmState->gSpuGta4DebugInfo.gta4ModelInfoIdx, pGcmState->gSpuGta4DebugInfo.gta4RenderPhaseID);
	}
	else
	{	// ea is NULL, so size should be 0:
		FatalAssertf(dmaListElement->Size==0,
			"edge_jobs: Bad dma list for %s (ea:0x%X, size:%d), GtaModelIndex: %d, GtaRenderPhaseID: %d. Possibly caused by corrupt EdgeGeom data.", dmaListName, dmaListElement->Ea, dmaListElement->Size, pGcmState->gSpuGta4DebugInfo.gta4ModelInfoIdx, pGcmState->gSpuGta4DebugInfo.gta4RenderPhaseID);
	}

#endif // __ASSERT
}

static
void VerifyJobDmaLists(CellSpursEdgeJob *job, spuGcmState *gcmState)
{
	VerifyDma(gcmState, &job->DmaList.SpuOutputStreamDesc,			"SpuOutputStreamDesc");
	VerifyDma(gcmState, &job->DmaList.RsxOnlyStreamDesc,			"RsxOnlyStreamDesc");

	VerifyDma(gcmState, &job->DmaList.Indexes0,						"Indexes0");
	VerifyDma(gcmState, &job->DmaList.Indexes1,						"Indexes1");

	VerifyDma(gcmState, &job->DmaList.SkinningMatrices0,			"SkinningMatrices0");
	VerifyDma(gcmState, &job->DmaList.SkinningMatrices1,			"SkinningMatrices1");
	VerifyDma(gcmState, &job->DmaList.SkinningIndexesAndWeights0,	"SkinningIndexesAndWeights0");
	VerifyDma(gcmState, &job->DmaList.SkinningIndexesAndWeights1,	"SkinningIndexesAndWeights1");


	VerifyDma(gcmState, &job->DmaList.PrimaryVertexes0,				"PrimaryVertexes0");
	VerifyDma(gcmState, &job->DmaList.PrimaryVertexes1,				"PrimaryVertexes1");
	VerifyDma(gcmState, &job->DmaList.PrimaryVertexes2,				"PrimaryVertexes2");
	VerifyDma(gcmState, &job->DmaList.SecondaryVertexes0,			"SecondaryVertexes0");
	VerifyDma(gcmState, &job->DmaList.SecondaryVertexes1,			"SecondaryVertexes1");
	VerifyDma(gcmState, &job->DmaList.SecondaryVertexes2,			"SecondaryVertexes2");
	VerifyDma(gcmState, &job->DmaList.PrimaryFixedPointOffsets,		"PrimaryFixedPointOffsets");
	// VerifyDma(gcmState, &job->DmaList.SecondaryFixedPointOffsets,	"SecondaryFixedPointOffsets");

	VerifyDma(gcmState, &job->DmaList.PrimarySpuInputStreamDesc,	"PrimarySpuInputStreamDesc");
	VerifyDma(gcmState, &job->DmaList.SecondarySpuInputStreamDesc,	"SecondarySpuInputStreamDesc");

}// VerifyJobDmaLists()...
#endif // __SPU && HACK_GTA4_MODELINFOIDX_ON_SPU...

CompileTimeAssert(sizeof(CellSpursEdgeJob)==512 || EDGE_STRUCT_SIZE_NO_ASSERT);

// #include "system/findsize.h"
// struct CellSpursEdgeJobNoPad : public CellSpursEdgeJob_Base<CellSpursEdgeJob_DmaList_Base,CellSpursEdgeJob_CachedDmaList_Base> { };
// FindSize(CellSpursEdgeJobNoPad);	// currently 560 out of 640.  Would need to save another 80 bytes to help at all due to 128-byte alignment
// FindSize(spuGcmState);		// currently 1376 bytes.

/*! Adds a pending edge geometry job to the live ring-buffered job chain.
 *
 *	This use a live job chain. The function will immediately kick the job if possible or stall if 
 *	job ring buffer is full.
 *
 *	Parameters:
 *
 *	\param	ctx						The GCM context to add the local stall hole for the draw commands.
 *	\param	lsPpuConfigInfo			The edge config info for this segment. 
 *									- On PPU this is a pointer to the EdgeGeomPpuConfigInfo structure.
 *									- On SPU this is a pointer local store copy of the 
 *									  EdgeGeomPpuConfigInfo structure.
 *	\param	eaSkinningMatrices		Effective address (=main memory) of skinning matrices. 
 *									Can be NULL if segment does not require skinning.
 *									These should be in packed form (i.e. 3 quadwords) per bone.								
 *	\param	lsLocalToWorldMatrix	The local to world matrix for this segment. Will be copied into the job.
 *									- On PPU this is a pointer to the EdgeGeomLocalToWorldMatrix structure.
 *									- On SPU this is a pointer local store copy of the 
 *									  EdgeGeomLocalToWorldMatrix structure.
 *	\param	edgeCullingMode			Edge culling mode (not rage)
 *	\param	vertexShaderInputMask	Vertex shader input mask (one bit per rsx stream), used to optimize
 *									output/input flavours and select smaller ones dynamically if possible.
 *									That's the mechanism used to lower shadow processing cost automagically.
 *									See g_VertexShaderInputs in grCore.
 *
 *	Note that the DMA-list assumes there is no embedded custom flavour code, no blend-shapes and no instancing.
 *
 *	\warning	This function is not thread safe.
 *	\warning	Only available on either PPU or SPU, but not both at the same time.
 *              - on PPU, if EDGE_JOBS_MANAGEMENT_ON_SPU is 0 
 *              - on SPU, if EDGE_JOBS_MANAGEMENT_ON_SPU is 1 
 */
#if !__PPU || !SPU_GCM_FIFO
CompileTimeAssertSize(CellSpursJobHeader,48,48);

void grcGeometryJobs::AddJob(
					  CellGcmContextData *ctx,
					  const EdgeGeomPpuConfigInfo *lsPpuConfigInfo, 
					  const void *eaSkinningMatrices, 
					  const Vector3& offset,
					  u32 vertexShaderInputMask,
					  spuGcmState& gcmState)
{
	// Yes it's possible, we might not expect a position
	if (Unlikely((vertexShaderInputMask & 1) == 0))
		return;

	// Replace previous job chain END with a JTS
	u32 nextJob = m_NextJob;
	u64 *const eaJobChainCmds = m_eaJobChainCommandArray;
	u64 *const eaJobChainJts = eaJobChainCmds + nextJob;
	JobChainJts(eaJobChainJts);

#if __SPU
	// Paranoid tests : In LS
	Assert(lsPpuConfigInfo && 256*1024>(uintptr_t)lsPpuConfigInfo);	
	// Paranoid tests : In main memory (test is bad)
	Assert((!eaSkinningMatrices) || (256*1024<=(uintptr_t)eaSkinningMatrices));
#endif //__SPU

	// Optimize input/output vertex format for shadows etc.
	const bool enableSecondaryStreamRsx = (vertexShaderInputMask != 1/*grcFvf::grcfcPositionMask*/);
	const bool enableSecondaryStream	= enableSecondaryStreamRsx || bool(gcmState.IsVehicleGeom);

#ifdef EDGE_JOBS_PRESTALLHOLE_CALLBACK
	EDGE_JOBS_PRESTALLHOLE_CALLBACK(ctx, vertexShaderInputMask, gcmState, lsPpuConfigInfo);
#endif // EDGE_JOBS_PRESTALLHOLE_CALLBACK

	u32 holeSize = lsPpuConfigInfo->spuConfigInfo.commandBufferHoleSize << 4; // quadwords->bytes
	void* holeEa = grcGeometryJobs::SetLocalStallHole(ctx, holeSize, sizeof(CellSpursEdgeJob), &gcmState);

	// Allocate job from the END of the current segment.  If the Edge clip
	// planes are disabled, then we don't bother allocating space for them,
	// since they are the last thing in the struct.
	//
	// Would be nice to be able to write a compile time assert to properly check
	// that no other fields have been added after EdgeClipPlanesTransposed, but
	// due to alignment padding, this is the best we can do...
	//
	CompileTimeAssert(sizeof(CellSpursEdgeJob) == ((OffsetOf(CellSpursEdgeJob,SpuGcmState.EdgeClipPlanesTransposed)+EDGE_NUM_TRIANGLE_CLIP_PLANES*16+__alignof(CellSpursEdgeJob)-1)&-(unsigned)__alignof(CellSpursEdgeJob)));
	u8 clipPlaneEnable = gcmState.EdgeClipPlaneEnable;
	u32 clipPlanesSize = (clipPlaneEnable==0 ? 0 : clipPlaneEnable==0xff ? EDGE_NUM_TRIANGLE_CLIP_PLANES*16 : 4*16);
	u32 realJobSize = OffsetOf(CellSpursEdgeJob,SpuGcmState.EdgeClipPlanesTransposed) + clipPlanesSize;
	// Notice that here we round the job size up to a 128-byte multiple.  It is
	// not really clear why the code is asserting about alignment, possibly it
	// is to ensure dma efficiency?  Code runs fine without the rounding, but
	// for the sake of minimal changes, do round up similar to before.
	u32 jobSize128 = (realJobSize + 127) & ~127;
	ctx->end = (uint32_t*)((char*)ctx->end - jobSize128);

	uint32_t eaJob = (uint32_t)pSpuGcmState->NextSegment + ((uint32_t)ctx->end - (uint32_t)ctx->begin + 4);
	Assert((eaJob & 127) == 0);

	// Get the job descriptor; note that the GCM context is guaranteed to be 128-byte aligned already,
	// and end points at the last full word in the buffer.  So one word past that will be aligned as well.
	CellSpursEdgeJob *job = (CellSpursEdgeJob*)(ctx->end + 1);

	Assert(((uint32_t)job & 127) == 0);

	// Do the list construction as quadwords to save unnecessary read-modify-writes (size, then ea)
	qword *dmas = (qword*)job;
	dmas[0] = dmas[1] = dmas[2] = si_il(0);
	uint32_t dmas_3_0 = 0, dmas_3_1 = 0, dmas_3_2 = 0, dmas_3_3 = 0;
	if (lsPpuConfigInfo->spuConfigInfo.outputVertexFormatId == 0xff)
	{
		dmas_3_0 = lsPpuConfigInfo->spuOutputStreamDescSize;
		dmas_3_1 = (u32)lsPpuConfigInfo->spuOutputStreamDesc;
	}
	if (enableSecondaryStreamRsx)
	{
		dmas_3_2 = lsPpuConfigInfo->rsxOnlyStreamDescSize;
		dmas_3_3 = (u32)lsPpuConfigInfo->rsxOnlyStreamDesc;
	}
	dmas[3] = MAKE_QWORD(dmas_3_0,dmas_3_1,dmas_3_2,dmas_3_3);

	dmas[4] = MAKE_QWORD(lsPpuConfigInfo->indexesSizes[0],(u32)lsPpuConfigInfo->indexes,lsPpuConfigInfo->indexesSizes[1],(u32)lsPpuConfigInfo->indexes + lsPpuConfigInfo->indexesSizes[0]);
	dmas[5] = MAKE_QWORD(lsPpuConfigInfo->skinMatricesSizes[0],(u32)eaSkinningMatrices + lsPpuConfigInfo->skinMatricesByteOffsets[0],lsPpuConfigInfo->skinMatricesSizes[1],(u32)eaSkinningMatrices + lsPpuConfigInfo->skinMatricesByteOffsets[1]);
	dmas[6] = MAKE_QWORD(lsPpuConfigInfo->skinIndexesAndWeightsSizes[0],(u32)lsPpuConfigInfo->skinIndexesAndWeights,lsPpuConfigInfo->skinIndexesAndWeightsSizes[1],(u32)lsPpuConfigInfo->skinIndexesAndWeights + lsPpuConfigInfo->skinIndexesAndWeightsSizes[0]);
	dmas[7] = MAKE_QWORD(lsPpuConfigInfo->spuVertexesSizes[0],(u32)lsPpuConfigInfo->spuVertexes[0],lsPpuConfigInfo->spuVertexesSizes[1],(u32)lsPpuConfigInfo->spuVertexes[0] + lsPpuConfigInfo->spuVertexesSizes[0]);

	uint32_t dmas_8_0 = lsPpuConfigInfo->spuVertexesSizes[2];
	uint32_t dmas_8_1 = (u32)lsPpuConfigInfo->spuVertexes[0] + lsPpuConfigInfo->spuVertexesSizes[0] + lsPpuConfigInfo->spuVertexesSizes[1];
	if (enableSecondaryStream)
	{
		dmas[8] = MAKE_QWORD(dmas_8_0,dmas_8_1,lsPpuConfigInfo->spuVertexesSizes[3],(u32)lsPpuConfigInfo->spuVertexes[1]);
		dmas[9] = MAKE_QWORD(lsPpuConfigInfo->spuVertexesSizes[4],(u32)lsPpuConfigInfo->spuVertexes[1] + lsPpuConfigInfo->spuVertexesSizes[3],lsPpuConfigInfo->spuVertexesSizes[5],(u32)lsPpuConfigInfo->spuVertexes[1] + lsPpuConfigInfo->spuVertexesSizes[3] + lsPpuConfigInfo->spuVertexesSizes[4]);
		Assert(!lsPpuConfigInfo->fixedOffsetsSize[1]);
	}
	else
	{
		dmas[8] = MAKE_QWORD_ZZ(dmas_8_0,dmas_8_1);
		dmas[9] = si_il(0);
	}

	uint32_t dmas_10_0 = lsPpuConfigInfo->fixedOffsetsSize[0];
	uint32_t dmas_10_1 = (u32)lsPpuConfigInfo->fixedOffsets[0];
	if (lsPpuConfigInfo->spuConfigInfo.inputVertexFormatId == 0xff)
		dmas[10] = MAKE_QWORD(dmas_10_0,dmas_10_1,lsPpuConfigInfo->spuInputStreamDescSizes[0],(u32)lsPpuConfigInfo->spuInputStreamDescs[0]);
	else
		dmas[10] = MAKE_QWORD_ZZ(dmas_10_0,dmas_10_1);

	if (enableSecondaryStream && lsPpuConfigInfo->spuConfigInfo.secondaryInputVertexFormatId == 0xff)
		dmas[11] = MAKE_QWORD_ZZ(lsPpuConfigInfo->spuInputStreamDescSizes[1],(u32)lsPpuConfigInfo->spuInputStreamDescs[1]);
	else
		dmas[11] = si_il(0);

	dmas[12] = *(qword*)&lsPpuConfigInfo->spuConfigInfo;

	uint32_t *offsetU = (uint32_t*)&offset;
	dmas[13] = MAKE_QWORD(offsetU[0],offsetU[1],offsetU[2],(u32)holeEa);
	dmas[14] = MAKE_QWORD((u32)m_eaOutputBufferInfo,(u32)lsPpuConfigInfo->blendShapes,(u32)lsPpuConfigInfo->rsxOnlyVertexes,(vertexShaderInputMask << 16) | lsPpuConfigInfo->numBlendShapes);

	CompileTimeAssert((OffsetOf(CellSpursEdgeJob, DmaList) & 15) == 0);
	// CompileTimeAssert((OffsetOf(CellSpursEdgeJob, CachedDmaList) & 15) == 0);
	CompileTimeAssert((OffsetOf(CellSpursEdgeJob, SpuConfigInfo) & 15) == 0);
	CompileTimeAssert((OffsetOf(spuGcmState, EdgeWorld) & 15) == 0);
	CompileTimeAssert((OffsetOf(spuGcmState, EdgeInfo) & 15) == 0);
	CompileTimeAssert((OffsetOf(CellSpursEdgeJob, Offset) & 15) == 0);
	// CompileTimeAssert((sizeof(job->DmaList) & 15) == 0);
	CompileTimeAssert((sizeof(CellSpursEdgeJob) & 127) == 0);

#ifdef EDGE_JOBS_POSTSTALLHOLE_CALLBACK
	EDGE_JOBS_POSTSTALLHOLE_CALLBACK(job, ctx, gcmState, lsPpuConfigInfo);
#endif // EDGE_JOBS_POSTSTALLHOLE_CALLBACK

	// Finish the job	
#if __SPU && SPU_GCM_FIFO
	job->Header.eaBinary = (uint64_t) gcmState._binary_edgegeomspu_job_elf_start;
	job->Header.sizeBinary = CELL_SPURS_GET_SIZE_BINARY(gcmState._binary_edgegeomspu_job_elf_size);
#else
	job->Header.eaBinary = (uint64_t)_binary_edgegeomspu_job_elf_start;
	job->Header.sizeBinary = CELL_SPURS_GET_SIZE_BINARY(_binary_edgegeomspu_job_elf_size);
#endif
	// DMA lists must have an even number of elements (actually I don't think this is true)
	job->Header.sizeDmaList = sizeof(job->DmaList);
	job->Header.eaHighInput = 0;
	job->Header.useInOutBuffer = 1;
#if HACK_GTA4
	//	job->Header.sizeInOrInOut = lsPpuConfigInfo->ioBufferSize + (sizeof(spuGcmState)-sizeof(EdgeGeomLocalToWorldMatrix)-sizeof(EdgeGeomSpuConfigInfo)-sizeof(EdgeGeomViewportInfo)) + 2048;
	job->Header.sizeInOrInOut = lsPpuConfigInfo->ioBufferSize;

	// allocate some extra temp space for 4xfloat positions for shadows:
	if(spuGcmShadowWarpEnabled(gcmState.shadowType))
	{
		job->Header.sizeInOrInOut += (lsPpuConfigInfo->spuConfigInfo.numVertexes/2)*4;
		job->Header.sizeInOrInOut += 128;
		
		if(lsPpuConfigInfo->spuConfigInfo.outputVertexFormatId==0xff && !lsPpuConfigInfo->spuOutputStreamDesc)
		{
			job->Header.sizeInOrInOut += (lsPpuConfigInfo->spuConfigInfo.numVertexes/2)*4;
			job->Header.sizeInOrInOut += (256-128);
		}
	}
#else
	job->Header.sizeInOrInOut = lsPpuConfigInfo->ioBufferSize + (sizeof(spuGcmState)-sizeof(EdgeGeomLocalToWorldMatrix)-sizeof(EdgeGeomSpuConfigInfo)-sizeof(EdgeGeomViewportInfo));
#endif

#if HACK_GTA4
	// do not decrease stack for EDGE tint:
	job->Header.sizeStack = ((job->Header.sizeCacheDmaList && (!gcmState.edgeTintPalettePtr))?4096:8192) >> 4;
#else
	job->Header.sizeStack = 4096 >> 4;
#endif
	// CellSpursJobHeader::sizeScratch is in qwords
	job->Header.sizeScratch = lsPpuConfigInfo->scratchSizeInQwords;

	// Add one qword of extra space at the end of scratch and IO 
	// buffers, for overflow checking
	job->Header.sizeInOrInOut += 16;	// in bytes
	job->Header.sizeScratch += 1;		// in qwords


#if HACK_GTA4 && __DEV
	// edgegeomspu: approx. max. 230KB of memory available:
	const u32 totalJobMemSize = u32(job->Header.sizeBinary*16)		+
								u32(job->Header.sizeInOrInOut)		+
								u32(job->Header.sizeOut)			+
								u32(job->Header.sizeStack*16)		+
								u32(job->Header.sizeScratch*16)		+
								u32(job->Header.sizeCacheDmaList? job->CachedDmaList.ExtraData.Size : 0);

	if(totalJobMemSize > kMaxSPUJobSize)	// CELL_SPURS_MAX_SIZE_JOB_MEMORY
	{
	#if HACK_GTA4_MODELINFOIDX_ON_SPU
		spu_printf("\n Edgegeomspu: Total current job size: %dbytes (%dKB) (max: %dKB), modelInfoIdx=%d, renderPhaseID=%d",
			totalJobMemSize, totalJobMemSize/1024, kMaxSPUJobSize/1024,
			gcmState.gSpuGta4DebugInfo.gta4ModelInfoIdx, gcmState.gSpuGta4DebugInfo.gta4RenderPhaseID);
	#endif
		Assertf(0,"Edgegeomspu: Total current job size: %dbytes (%dKB) (max: %dKB).", totalJobMemSize, totalJobMemSize/1024, kMaxSPUJobSize/1024);
	}
#endif //HACK_GTA4 && __DEV...

#if __SPU

#if HACK_GTA4_MODELINFOIDX_ON_SPU
	VerifyJobDmaLists(job, &gcmState);
#endif

	// Because we are going to reuse the local store buffers used to put to the
	// SPURS jobchain, we need to block on completion of the tag before
	// modifying them.  Do the block before we start putting other stuff on the
	// same tag.  This block also ensures that the JTS from JobChainJts has been
	// writen before cellSpursRunJobChain is called.
	sysDmaWaitTagStatusAll(1<<FIFOTAG);

	// Dma out the Edge job.  Must do a memcpy of gcmState here (not dma put
	// directly from the source structure), since we return before blocking on
	// FIFOTAG.  Also, we must use the same dma tag as the RSX command buffer,
	// since we need to make sure that the Edge job doesn't start until the
	// stall hole has been put to XDR.
	CompileTimeAssert((OffsetOf(CellSpursEdgeJob, SpuGcmState) & 15) == 0);
	memcpy(&job->SpuGcmState, &gcmState, realJobSize-OffsetOf(CellSpursEdgeJob,SpuGcmState));
	sysDmaPut(job, eaJob, realJobSize, FIFOTAG);

#if SPU_GCM_FIFO
	// Do a partial flush of the current segment to make sure the stall hole and SPU GCM state
	// is in place before the EDGE job actually starts.  Note that we don't kick, so RSX should
	// never see any of this data until the segment is closed out.
	gcmPartialFlush(ctx);
#endif // SPU_GCM_FIFO

#endif // __SPU

	// Now add the job command to the *live* job chain
	nextJob = nextJob + 1;
	nextJob &= -(nextJob < m_MaxJobs);
	JobChainAddJob(m_eaJobChain, eaJobChainJts, eaJobChainCmds+nextJob, eaJob);
	m_NextJob = nextJob;
}


void grcGeometryJobs::AddOtherJob(CellGcmContextData *ctx,char* binaryEa, char* binarySize, size_t InputSize, void *InputData, size_t ScratchSize, size_t SpuStackSize, size_t UserDataCount, int *UserData)
{
	// Replace previous job chain END with a JTS
	u32 nextJob = m_NextJob;
	u64 *const eaJobChainCmds = m_eaJobChainCommandArray;
	u64 *const eaJobChainJts = eaJobChainCmds + nextJob;
	JobChainJts(eaJobChainJts);

	// make sure there's enough room in this segment.
	// Note that we intentionally overallocate here, because we size the job list by the maximum number of jobs
	// that could fit in the GCM FIFO without stalling out, and if we used the actual (smaller) size here we'd
	// run the risk of overwriting old descriptors before they were executed.  We could also avoid the issue
	// by simply allocating more descriptors.  Note that while we allocate full space, we only zero out and copy
	// the parts we actually use.
	CompileTimeAssert(sizeof(CellSpursEdgeJob) >= sizeof(CellSpursJob256));
	if ((uint32_t)ctx->current + sizeof(CellSpursEdgeJob) > (uint32_t)ctx->end)
		gcmCallback(ctx,0);

	// Allocate job from the END of the current segment
	ctx->end = (uint32_t*)((char*)ctx->end - sizeof(CellSpursJob256));

	uint32_t eaJob = (uint32_t)pSpuGcmState->NextSegment + ((uint32_t)ctx->end - (uint32_t)ctx->begin + 4);
	Assert((eaJob & 127) == 0);

	// Get the job descriptor; note that the GCM context is guaranteed to be 128-byte aligned already,
	// and end points at the last full word in the buffer.  So one word past that will be aligned as well.
	// Note that the job chain actually uses 640-byte jobs, so the job manager will pick up bonus crap.
	CellSpursJob256 *job = (CellSpursJob256*)(ctx->end + 1);
	Assert(((uint32_t)job & 127) == 0);

	// Memset it to zero
	memset(job, 0, sizeof(*job));

	CellSpursJobHeader &jh = job->header;
	jh.eaBinary = (uintptr_t) binaryEa;
	jh.sizeBinary = CELL_SPURS_GET_SIZE_BINARY(binarySize);
	unsigned int sizeDones = 16;
	unsigned int sizeMarker = TASK_SPU_EXTRA_CHECKS?16:0;	// overflow marker
	const bool inOut = true;
	jh.useInOutBuffer = inOut;
	jh.sizeInOrInOut = InputSize + sizeMarker + (inOut? sizeDones: 0);
	jh.sizeScratch = (ScratchSize + sizeMarker) >> 4;

	// Convert the default of zero ourselves to guarantee it never changes on us unexpectedly.
	jh.sizeStack = (SpuStackSize >> 4);

	// The first word is a DMA descriptor for the input
	// Input larger than 16k must be broken up!
	int inputIndex = 0;
	size_t remaining = InputSize;
	void *inputData = InputData;
	while (remaining) {
		size_t thisChunk = remaining < 16384? remaining : 16384;
		job->workArea.userData[inputIndex++] = sysDmaListMakeEntry(inputData,thisChunk);
		inputData = (void*)((char*)inputData + thisChunk);
		remaining -= thisChunk;
	}
	// Assert(inputIndex < userSize);
	jh.sizeDmaList  = inputIndex * sizeof(uint64_t);	// Only the first word is part of a DMA list.
	jh.sizeCacheDmaList = 0;

	// Finally any user data is copied in (32 bits per entry, so we use half of a userData slot for each one)
	int userDataIndex = inputIndex;
	Assert(userDataIndex + static_cast<int>(UserDataCount) <= 26*2);
	int *dataPtr = (int*) &job->workArea.userData[userDataIndex];
	*dataPtr = UserDataCount;
	for (size_t i=0; i<UserDataCount; i++)
		dataPtr[i+1] = UserData[i];

	// Wait for job chain END to be replaced with a JTS.  See AddJob for more details.
	sysDmaWaitTagStatusAll(1<<FIFOTAG);

	// Dma out the job header
	sysDmaPut(job, (uintptr_t)eaJob, sizeof(*job), FIFOTAG);

	// Now add the job command to the *live* job chain
	nextJob = nextJob + 1;
	nextJob &= -(nextJob < m_MaxJobs);
	JobChainAddJob(m_eaJobChain, eaJobChainJts, eaJobChainCmds+nextJob, eaJob);
	m_NextJob = nextJob;

	// NOTFINAL_ONLY(pSpuGcmState->EdgeInfo.taskLog->Log('ADDO',0,0,commandIndex));
}

#endif


#if HACK_GTA4
#if USE_EDGE && __PPU

static
EdgeGeomAttributeBlock* GetAttributeBlockFromVertexStreamDesc(EdgeGeomVertexStreamDescription *vertStreamDesc, EdgeGeomAttributeId edgeAttrID)
{
	const u32 numBlocks = vertStreamDesc->numBlocks;

	EdgeGeomGenericBlock *nextAttribute = vertStreamDesc->blocks;

	for(u32 i=0; i<numBlocks; i++)
	{
		EdgeGeomAttributeBlock *block = &nextAttribute->attributeBlock;
		nextAttribute++;

		if(block->edgeAttributeId == edgeAttrID)
			return(block);
	}

	return(NULL);
}

//
//
//
//
bool grcGeometryJobs::AddExtractJob(CellGcmContextData*			ctx,
							 const EdgeGeomPpuConfigInfo*		ppuConfigInfo, 
							 const EdgeGeomLocalToWorldMatrix*	localToWorldMatrix,
							 const Vector4**					pClipPlanes,
							 int								ClipPlaneCount,
							 const u8*							boneRemapLut,
							 unsigned							boneRemapLutSize,
							 void*								pDestVertsEa,
							 Vector4*							EndOfOutputArray,
							 void*								pDestVertsPtrsEa,
							 u16*								pDestIndexesEa,
							 u32								vertexOffsetForBatch,
							 u32								numTotalVerts,
							 void*								damageTexture,
							 float								boundsRadius,
							 const Vector3*						pBoneNormals,
							 float								dotProdThreshold
							#if HACK_GTA4_MODELINFOIDX_ON_SPU
								,CGta4DbgSpuInfoStruct				*pGta4SpuInfoStruct
							#endif
							,Matrix43*							ms,
							u8									extractMask)
{
	sysTaskHandle handle = NULL;
	if(AddExtractJobAsync(ctx, (u32**)(&handle), ppuConfigInfo, localToWorldMatrix, pClipPlanes, ClipPlaneCount, boneRemapLut, boneRemapLutSize, pDestVertsEa, EndOfOutputArray,
		pDestVertsPtrsEa, pDestIndexesEa, vertexOffsetForBatch, numTotalVerts, damageTexture, boundsRadius, pBoneNormals, dotProdThreshold
#if HACK_GTA4_MODELINFOIDX_ON_SPU
		, pGta4SpuInfoStruct
#endif
		, ms, extractMask))
	{
		// ...and wait for results
		rage::sysTaskManager::Wait(handle);
		return true;
	}
	
	return false;
}// end of AddExtractJob()...

bool grcGeometryJobs::AddExtractJobAsync(CellGcmContextData*	/*ctx*/,
	u32**								handle,
	const EdgeGeomPpuConfigInfo*		ppuConfigInfo, 
	const EdgeGeomLocalToWorldMatrix*	/*localToWorldMatrix*/,
	const Vector4**						pClipPlanes,
	int									ClipPlaneCount,
	const u8*							boneRemapLut,
	unsigned							boneRemapLutSize,
	void*								pDestVertsEa,
	Vector4*							EndOfOutputArray,
	void*								pDestVertsPtrsEa,
	u16*								pDestIndexesEa,
	u32									vertexOffsetForBatch,
	u32									numTotalVerts,
	void*								damageTexture,
	float								boundsRadius,
	const Vector3*						pBoneNormals,
	float								dotProdThreshold
#if HACK_GTA4_MODELINFOIDX_ON_SPU
	,CGta4DbgSpuInfoStruct				*pGta4SpuInfoStruct
#endif		
	,Matrix43*						ms,
	u8									extractMask)
{
	//	sysSpinLock spinLock(s_EdgeJobCreationSpinLock);

	damageTexture = NULL;

	const bool bExtractJobProjected = ClipPlaneCount && pClipPlanes;
	const bool bIsSkinned			= false && ((ppuConfigInfo->spuConfigInfo.indexesFlavorAndSkinningFlavor&0xF) != EDGE_GEOM_SKIN_NONE);

	// Add one qword of extra space at the end of scratch and IO buffers, for overflow checking
	//
	// The scratch buffer needs to be bloated in the case of bExtractJobProjected,
	// as we are potentially also decompressing up to two additional uniform tables
	// from the RSX only stream.
	//
	EdgeGeomVertexStreamDescription *rsxOnlyStreamDesc = (EdgeGeomVertexStreamDescription*)ppuConfigInfo->rsxOnlyStreamDesc;
	u32 extraUniformTablesSize = 0;
	if (bExtractJobProjected && rsxOnlyStreamDesc)
	{
		// How many extra uniform tables are being decompressed
		u32 minUniformTables;
		minUniformTables  = !!GetAttributeBlockFromVertexStreamDesc(rsxOnlyStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_COLOR);
		minUniformTables += !!GetAttributeBlockFromVertexStreamDesc(rsxOnlyStreamDesc, EDGE_GEOM_ATTRIBUTE_ID_NORMAL);

		// If the uniform tables can then be copied back into the memory holding
		// the compressed rsx only streams, then there is no need for additional
		// uniform tables.
		const u32 spaceInRsxOnlyStream = rsxOnlyStreamDesc->stride >> 4;
		u32 extraUniformTables = minUniformTables>spaceInRsxOnlyStream ? minUniformTables-spaceInRsxOnlyStream : 0;

		// But... we still need the temporary space to decompress the vertex
		// streams, so must ensure that the uniform table count is large enough.
		const u32 origUniformTableCount = (ppuConfigInfo->spuConfigInfo.flagsAndUniformTableCount & 0x0f) + 1;
		u32 uniformTableCount = origUniformTableCount + extraUniformTables;
		uniformTableCount = Max(uniformTableCount, minUniformTables);

		const u32 numVertexes8 = (ppuConfigInfo->spuConfigInfo.numVertexes+7)&~7;
		extraUniformTablesSize = (uniformTableCount - origUniformTableCount) * numVertexes8 << 4;
	}
	const u32 scratchSize	= (ppuConfigInfo->scratchSizeInQwords<<4) + extraUniformTablesSize + 16;
	const u32 stackSize		= 6144 + (bExtractJobProjected? (ppuConfigInfo->spuConfigInfo.numVertexes*sizeof(u8)*8) : (0))
		+ (EXTRACTGEOM_pClipPlanesSPU_SIZE+EXTRACTGEOM_BoneNormals_SIZE+EXTRACTGEOM_ClipPlanes_SIZE);	// in bytes
	const u32 outputSize0	= 0;

	Assertf(!bExtractJobProjected || (ppuConfigInfo->spuConfigInfo.numVertexes <= EXTRACTGEOM_MAX_SPU_VERTS), "ERROR: num verts=%d. Too many verts for create projected textures on SPU!", ppuConfigInfo->spuConfigInfo.numVertexes);

	sysTaskContext context(TASK_INTERFACE(edgeExtractGeomSpu_), outputSize0, scratchSize, stackSize);

	// We handle errors properly here, so don't want code to assert
	context.DontAssertOnErrors();


	s32 InputSizes[CExtractGeomParams::idxMax]={0};

	InputSizes[CExtractGeomParams::idxSpuOutputStreamDescSize]	= ppuConfigInfo->spuOutputStreamDescSize;
	InputSizes[CExtractGeomParams::idxSpuInputStreamDescSizeA]	= ppuConfigInfo->spuInputStreamDescSizes[0];
	InputSizes[CExtractGeomParams::idxSpuInputStreamDescSizeB]	= ppuConfigInfo->spuInputStreamDescSizes[1];
	InputSizes[CExtractGeomParams::idxRsxOnlyStreamDescSize]	= ppuConfigInfo->rsxOnlyStreamDescSize;

	switch((ppuConfigInfo->spuConfigInfo.indexesFlavorAndSkinningFlavor >> 4) & 0xF)
	{
	case EDGE_GEOM_INDEXES_COMPRESSED_TRIANGLE_LIST_CW:
	case EDGE_GEOM_INDEXES_COMPRESSED_TRIANGLE_LIST_CCW:
		// size of index buffer AFTER decompression
		// TODO: it unnecessarily blows up input dma fetch size, but at least decompression buffer for indices has the right size:
		InputSizes[CExtractGeomParams::idxIndexSizes]			= (u32)((ppuConfigInfo->spuConfigInfo.numIndexes*sizeof(u16)+0x0f)&(~0xf));
		break;
	case EDGE_GEOM_INDEXES_U16_TRIANGLE_LIST_CW:
	case EDGE_GEOM_INDEXES_U16_TRIANGLE_LIST_CCW:
		InputSizes[CExtractGeomParams::idxIndexSizes]			= (u32)ppuConfigInfo->indexesSizes[0] + (u32)ppuConfigInfo->indexesSizes[1];
	default:
		break;
	}

	InputSizes[CExtractGeomParams::idxSkinIsAndWsSizes]			= (u32)ppuConfigInfo->skinIndexesAndWeightsSizes[0] + (u32)ppuConfigInfo->skinIndexesAndWeightsSizes[1];

	InputSizes[CExtractGeomParams::idxSpuVertexesSizesA]		= ppuConfigInfo->spuVertexesSizes[0] + ppuConfigInfo->spuVertexesSizes[1] + ppuConfigInfo->spuVertexesSizes[2];
	InputSizes[CExtractGeomParams::idxSpuVertexesSizesB]		= ppuConfigInfo->spuVertexesSizes[3] + ppuConfigInfo->spuVertexesSizes[4] + ppuConfigInfo->spuVertexesSizes[5];
	InputSizes[CExtractGeomParams::idxFixedOffsetsSizeA]		= ppuConfigInfo->fixedOffsetsSize[0];
	InputSizes[CExtractGeomParams::idxFixedOffsetsSizeB]		= ppuConfigInfo->fixedOffsetsSize[1];
	InputSizes[CExtractGeomParams::idxRsxOnlyVertexesSize]		= (ppuConfigInfo->rsxOnlyVertexesSize+15)&0xfffffff0;	// align size to be DMA'able
	InputSizes[CExtractGeomParams::idxBoneRemapLutSize]			= boneRemapLutSize;

	if(bExtractJobProjected)
	{
		// no rsxOnlyStreams when clipping stuff:
		// TODO: parse ppuConfigInfo->spuInputStreamDescs[0] & [1] for normalsID
		if(bIsSkinned)
			InputSizes[CExtractGeomParams::idxRsxOnlyVertexesSize] = 0;

		Assert(EndOfOutputArray != NULL);
	}

	bool bWantSkinned = (extractMask & CExtractGeomParams::extractSkin);
	if (!Verifyf(ms || !bWantSkinned, "Skinned vertes requested but no matrix set was given"))
	{
		extractMask &= ~CExtractGeomParams::extractSkin;
		bWantSkinned = false;
	}

	context.SetInputOutput();
	context.AddInput(ppuConfigInfo->spuOutputStreamDesc,	InputSizes[CExtractGeomParams::idxSpuOutputStreamDescSize]);
	context.AddInput(ppuConfigInfo->rsxOnlyStreamDesc,		InputSizes[CExtractGeomParams::idxRsxOnlyStreamDescSize]);

	// convert rsxOnly stream OFFSET (done in ctor of grmGeometryEdge) into PPU address:
	void *rsxOnlyVertexes = ppuConfigInfo->rsxOnlyVertexes;
	if(gcm::IsValidLocalOffset((u32)rsxOnlyVertexes))
		rsxOnlyVertexes = gcm::LocalPtr((u32)rsxOnlyVertexes);
	else
		rsxOnlyVertexes = gcm::MainPtr((u32)rsxOnlyVertexes);
	context.AddInput(rsxOnlyVertexes,						InputSizes[CExtractGeomParams::idxRsxOnlyVertexesSize]);

	context.AddInput(ppuConfigInfo->indexes,				InputSizes[CExtractGeomParams::idxIndexSizes]);
	context.AddInput(ppuConfigInfo->skinIndexesAndWeights,	InputSizes[CExtractGeomParams::idxSkinIsAndWsSizes]);
	if (bWantSkinned)
	{
		context.AddInput((Matrix43*)((u32)ms + ppuConfigInfo->skinMatricesByteOffsets[0]), ppuConfigInfo->skinMatricesSizes[0]);
		context.AddInput((Matrix43*)((u32)ms + ppuConfigInfo->skinMatricesByteOffsets[1]), ppuConfigInfo->skinMatricesSizes[1]);
	}
	context.AddInput(ppuConfigInfo->spuVertexes[0],			InputSizes[CExtractGeomParams::idxSpuVertexesSizesA]);
	context.AddInput(ppuConfigInfo->spuVertexes[1],			InputSizes[CExtractGeomParams::idxSpuVertexesSizesB]);

	context.AddInput(ppuConfigInfo->fixedOffsets[0],		InputSizes[CExtractGeomParams::idxFixedOffsetsSizeA]);
	context.AddInput(ppuConfigInfo->fixedOffsets[1],		InputSizes[CExtractGeomParams::idxFixedOffsetsSizeB]);

	context.AddInput(ppuConfigInfo->spuInputStreamDescs[0],	InputSizes[CExtractGeomParams::idxSpuInputStreamDescSizeA]);
	context.AddInput(ppuConfigInfo->spuInputStreamDescs[1],	InputSizes[CExtractGeomParams::idxSpuInputStreamDescSizeB]);

	context.AddInput(&ppuConfigInfo->spuConfigInfo,			sizeof(EdgeGeomSpuConfigInfo));

	context.AddInput(boneRemapLut,                          boneRemapLutSize);


	if(damageTexture)
	{
		Displayf("AddExtractJob: damage texture=%p, radius=%f, vertexsizes[2]=%d ", damageTexture, boundsRadius, ppuConfigInfo->spuVertexesSizes[2]);
		//	context.AddCacheable( damageTexture			, 128*128*3);
	}

	// setup job's header sizeInOrInOut size:
	s32 outputSize = ppuConfigInfo->ioBufferSize;
	for(u32 i=0; i<CExtractGeomParams::idxMax; i++)
		outputSize -= InputSizes[i];

	// following entries don't count for IO buffer size:
	outputSize += InputSizes[CExtractGeomParams::idxSpuOutputStreamDescSize];
	outputSize += InputSizes[CExtractGeomParams::idxRsxOnlyStreamDescSize];
	outputSize += InputSizes[CExtractGeomParams::idxRsxOnlyVertexesSize];

	if(bExtractJobProjected && (!bIsSkinned))
	{
		outputSize = outputSize>0? outputSize : 0;
	}

	Assert(outputSize >= 0);
	context.AddOutput(outputSize);


	CExtractGeomParams *info = context.AllocUserDataAs<CExtractGeomParams>();

	// AllocUserDataAs will report an error if it will return NULL, no need for
	// second error report here.  But we do need to handle it without crashing.
	if(Unlikely(!info))
	{
		return false;
	}

	for(u32 i=0; i<CExtractGeomParams::idxMax; i++)
	{
		Assert(InputSizes[i] < 65535);
		info->InputSizes[i] = InputSizes[i];
	}


	info->m_bIsHardSkinned			= bool((u32(ClipPlaneCount)>>16)&0x0001);
	info->ClipPlaneCount			= u32(ClipPlaneCount)&0xffff;
	Assert(info->ClipPlaneCount < EXTRACTGEOM_MAX_SPU_CLIPPLANES);
	info->ClipPlanesEa				= (u32)pClipPlanes;
	Assert16(pClipPlanes);
	info->outputBufferIndiciesEa 	= (u32)pDestIndexesEa;
	info->outputBufferVertsEa	    = (u32)pDestVertsEa;
	Assert16(pDestVertsEa);
	info->outputBufferVertsPtrsEa	= (u32)pDestVertsPtrsEa;
	Assert16(pDestVertsPtrsEa);
	info->skinMatricesByteOffsets0	= ppuConfigInfo->skinMatricesByteOffsets[0];
	info->skinMatricesByteOffsets1	= ppuConfigInfo->skinMatricesByteOffsets[1];
	info->skinMatricesSizes0		= ppuConfigInfo->skinMatricesSizes[0];
	info->m_skinMatricesNum			= bWantSkinned ? (u16)((ppuConfigInfo->skinMatricesSizes[0] + ppuConfigInfo->skinMatricesSizes[1]) / 48) : 0;
	info->numTotalVerts				= numTotalVerts;
	info->vertexOffsetForBatch		= vertexOffsetForBatch;
	info->damageTexture				= NULL;	//damageTexture;
	info->boundsRadius				= boundsRadius;
	info->EndOfOutputArray			= EndOfOutputArray;
	info->pBoneNormals				= pBoneNormals;
	Assert16(pBoneNormals);
	info->m_dotProdThreshold			= dotProdThreshold;
	info->m_extractMask				= extractMask;
#if HACK_GTA4_MODELINFOIDX_ON_SPU
	if(pGta4SpuInfoStruct)
		info->gta4SpuInfoStruct		= *pGta4SpuInfoStruct;
	else
		info->gta4SpuInfoStruct.Invalidate();
#endif		

	// extract!
	*handle = (u32*)context.TryStart(sysTaskManager::SCHEDULER_GRAPHICS_OTHER/*SCHEDULER_GRAPHICS_MAIN*/);
	if (Unlikely(!*handle))
	{
		return false;
	}

	return true;
}// end of AddExtractJobAsync()...

bool grcGeometryJobs::FinalizeExtractJobAsync(CellGcmContextData* /*ctx*/, u32* handle)
{
	if (Unlikely(!handle))
	{
		return false;
	}

	rage::sysTaskManager::Wait((sysTaskHandle)handle);
	
	return true;
}// end of AddExtractJobAsync()...
#endif // USE_EDGE && __PPU...
#endif // HACK_GTA4...



/*!	Initialization
 *	- Allocates data structures and output buffers
 *	- Sets up the segmented/live job ring buffer
 *
 *	\param	numJobSegments			Number of job segments (ring buffer management unit) 
 *									must be <= 16 because of cellSpursEventFlag restrictions.
 *	\param	numJobsPerSegment		Number of jobs per job segment
 *	\param	sizeOutputRingBuffer	Per active SPU / in bytes 
 *	\param	sizeOutputSharedBuffer	Global / in bytes
 *	\param	outputBufferLocation	Either CELL_GCM_LOCATION_MAIN or CELL_GCM_LOCATION_LOCAL (vram)
 *	\param	spuPriorities			Spurs priority array. Any SPU with a 0 value won't get a ring
 *									buffer allocated.
 *
 *	\warning	Only available on PPU
 */ 
#if !__SPU
int grcGeometryJobs::Initialize(CellSpurs* spursInstance,
							    unsigned numJobs,
							    unsigned sizeOutputRingBuffer, 
							    unsigned int sizeOutputSharedBuffer,
							    u8 outputBufferLocation,
							    u8 spuPriorities[8])
{
	m_OutputLocation = outputBufferLocation;
	m_FirstRsxLabelIndex = gcm::RsxSemaphoreRegistrar::Allocate(EDGE_MAX_SPU);
	m_MaxJobs = numJobs;
	m_NextJob = 0;

	// Allocate internal data
	m_eaOutputBufferInfo = rage_new EdgeGeomOutputBufferInfo;		
	Assert((0x7F&(uintptr_t)m_eaOutputBufferInfo)==0);

	m_eaJobChain = rage_new CellSpursJobChain;						
	Assert((0x7F&(uintptr_t)m_eaJobChain)==0);
	
	m_eaJobChainCommandArray = rage_aligned_new(128) uint64_t[numJobs + 1];
	Assert((0x7F&(uintptr_t)m_eaJobChainCommandArray)==0);

	// Count how many SPUs have a non-zero priority (those with 0 don't actually need a ring buffer)
	unsigned int edgeSpuCount = 0;
	for (unsigned int i=0; i<EDGE_MAX_SPU; i++) {
		if (spuPriorities[i]) {
			edgeSpuCount++;
		}
	}

	// Allocate output buffers
	uint32_t ringOutputTotalSize = edgeSpuCount * sizeOutputRingBuffer;
	if(m_OutputLocation == CELL_GCM_LOCATION_MAIN) {
		m_eaSharedOutputBuffer = rage_aligned_new(128) uint8_t[sizeOutputSharedBuffer];
		m_eaRingOutputBuffer = rage_aligned_new(128) uint8_t[ringOutputTotalSize];
	}
	else if (m_OutputLocation == CELL_GCM_LOCATION_LOCAL) {
		m_eaSharedOutputBuffer = (uint8_t*) physical_new(sizeOutputSharedBuffer, 128);
		m_eaRingOutputBuffer = (uint8_t*) physical_new(ringOutputTotalSize, 128);
	}
	else {
		Assert(false);
	}

	// Set up shared buffer info
	memset(m_eaOutputBufferInfo, 0, sizeof(*m_eaOutputBufferInfo));
	m_eaOutputBufferInfo->sharedInfo.startEa = (uint32_t)m_eaSharedOutputBuffer;
	m_eaOutputBufferInfo->sharedInfo.endEa = m_eaOutputBufferInfo->sharedInfo.startEa
		+ sizeOutputSharedBuffer;
	m_eaOutputBufferInfo->sharedInfo.currentEa = m_eaOutputBufferInfo->sharedInfo.startEa;
	m_eaOutputBufferInfo->sharedInfo.locationId = m_OutputLocation;
	m_eaOutputBufferInfo->sharedInfo.failedAllocSize = 0;
	cellGcmAddressToOffset((void*)m_eaOutputBufferInfo->sharedInfo.startEa,
		&m_eaOutputBufferInfo->sharedInfo.startOffset);

	// Set up ring buffer infos
	uint32_t curOutputBuffer = (uint32_t)m_eaRingOutputBuffer;
	for(uint32_t iSpu=0; iSpu<EDGE_MAX_SPU; ++iSpu) {	
		// labels are statically allocated - use SPU id [0..6] 
		// to keep things simple and avoid remapping we allocate label and
		// ring-buffer structs  (but not buffer space) even for SPUs that will never see edge
		u32 *outputLabel = cellGcmGetLabelAddress(m_FirstRsxLabelIndex + iSpu);

		// Each active SPU must have its own separate ring buffer, with no overlap.
		m_eaOutputBufferInfo->ringInfo[iSpu].startEa = curOutputBuffer;
		uint32_t sizeROutput = spuPriorities[iSpu]? sizeOutputRingBuffer: 0;
		curOutputBuffer += sizeROutput;
		m_eaOutputBufferInfo->ringInfo[iSpu].endEa = curOutputBuffer;		
		m_eaOutputBufferInfo->ringInfo[iSpu].currentEa = m_eaOutputBufferInfo->ringInfo[iSpu].startEa;
		m_eaOutputBufferInfo->ringInfo[iSpu].cachedFree = 0; // forces a reload
		m_eaOutputBufferInfo->ringInfo[iSpu].totalAlloc = 0; // stats
		m_eaOutputBufferInfo->ringInfo[iSpu].locationId = m_OutputLocation;
		m_eaOutputBufferInfo->ringInfo[iSpu].rsxLabelEa = sizeROutput? (uint32_t)outputLabel: 0L; // disable texture labels if no ring buffer
		cellGcmAddressToOffset((void*)m_eaOutputBufferInfo->ringInfo[iSpu].startEa,
			&m_eaOutputBufferInfo->ringInfo[iSpu].startOffset);

		// RSX label value must be initialized to the end of the buffer
		*outputLabel = m_eaOutputBufferInfo->ringInfo[iSpu].endEa;
	}

	// Sanity check
	Assert((curOutputBuffer-(uint32_t)m_eaRingOutputBuffer)==ringOutputTotalSize);

	// create our static job chain commands which loop forever
	for (unsigned i=0; i<m_MaxJobs; i++)
		m_eaJobChainCommandArray[i] = CELL_SPURS_JOB_COMMAND_END;
	m_eaJobChainCommandArray[m_MaxJobs] = CELL_SPURS_JOB_COMMAND_NEXT(&m_eaJobChainCommandArray[0]);

	// create the job chain	
	CellSpursJobChainAttribute attr;

	uint32_t readyCount = 0; // autoReadyCount - let spurs decide
	CELL_CHECK(cellSpursJobChainAttributeInitialize(&attr, &m_eaJobChainCommandArray[0], 
		sizeof(CellSpursEdgeJob), 4 /*maxgrab */, spuPriorities, 
		EDGE_MAX_SPU /*maxContention*/, readyCount? false: true /*autoReadyCount*/, 
		4 /*tag0*/, 5/*tag1*/, false /*isFixedMemAlloc*/, 
		sizeof(CellSpursEdgeJob), readyCount /*readyCount*/));
	CELL_CHECK(cellSpursJobChainAttributeSetName(&attr, "EDGE/GFX"));
	CELL_CHECK(cellSpursJobChainAttributeSetHaltOnError(&attr));
	CELL_CHECK(cellSpursCreateJobChainWithAttribute(
		spursInstance, 
		m_eaJobChain, 
		&attr));

	// kick the job list (will wait on the end, in autoreadycount mode)
	CELL_CHECK(cellSpursRunJobChain(m_eaJobChain));

	grcDisplayf("MaxJobs=%u",m_MaxJobs);
	grcDisplayf("sizeof(CellSpursEdgeJob)=%d",sizeof(CellSpursEdgeJob));
	grcDisplayf("sizeof(CellSpursEdgeJob_DmaList)=%d",sizeof(CellSpursEdgeJob_DmaList));
	grcDisplayf("sizeof(CellSpursEdgeJob_CachedDmaList)=%d",sizeof(CellSpursEdgeJob_CachedDmaList));
	grcDisplayf("sizeof(spuGcmStateBaseEdge)=%d",sizeof(spuGcmStateBaseEdge));

	m_Initialized = true;

	return 0 /*CELL_OK*/;
}
#endif // !__SPU

void grcGeometryJobs::SetOccluders(void* quads, u32 _quadCount)
{
	u8 quadCount;
	Assign(quadCount, _quadCount);

#if __SPU
	pSpuGcmState->EdgeOccluderQuads = quads;
	pSpuGcmState->EdgeOccluderQuadCount = quadCount;
#else
	SPU_COMMAND(grcGeometryJobs__SetOccluders, quadCount);
	cmd->occluderQuads = quads;
#endif // __SPU
}

/*! \warning	Only available on PPU
 */
#if !__SPU
int grcGeometryJobs::Shutdown(void)
{
	if (!m_Initialized)
	{
		return -1;
	}

	// Shutdown edge geom
#if (!!EDGE_JOBS_MANAGEMENT_ON_SPU) ^ (!!__PPU)
	// :TODO: fix in SPU mode.. must wait for the task, itself waiting on all job segments
	grcGeometryJobs::WaitForAllJobSegments();
#endif

	// Request job chain shutdown
	if (m_eaJobChain)
	{
		CELL_CHECK(cellSpursShutdownJobChain(m_eaJobChain));
	}

	// Wait for job segment shutdown
	if (m_eaJobChain)
	{
		CELL_CHECK(cellSpursJoinJobChain(m_eaJobChain));
	}

	// Detach event queue
#if EDGE_JOBS_MANAGEMENT_ON_SPU
	// do nothing
#endif

	// Free output buffers
	if (m_OutputLocation == CELL_GCM_LOCATION_MAIN)
	{
		delete[] m_eaRingOutputBuffer;	m_eaRingOutputBuffer = NULL;
		delete[] m_eaSharedOutputBuffer;	m_eaSharedOutputBuffer = NULL;
	}
	else if (m_OutputLocation == CELL_GCM_LOCATION_LOCAL)
	{
		physical_delete(m_eaRingOutputBuffer);	m_eaRingOutputBuffer = NULL;
		physical_delete(m_eaSharedOutputBuffer);	m_eaSharedOutputBuffer = NULL;
	}
	else
	{
		Assert(false);
	}

	delete[] m_eaJobChainCommandArray;		m_eaJobChainCommandArray = NULL;
	delete[] m_eaJobChain;					m_eaJobChain = NULL;
	delete[] m_eaOutputBufferInfo;			m_eaOutputBufferInfo = NULL;
	
	gcm::RsxSemaphoreRegistrar::Free(m_FirstRsxLabelIndex, EDGE_MAX_SPU);

	m_Initialized = false;

	return 0 /*CELL_OK*/;
}
#endif //!__SPU

/*! To be called at the beginning of each frame.
 *	
 *	\note		Currently only used for profiling / collecting stats, as everything else
 *				is handled by frame-independent ring buffers.
 *
 *	\warning	Only available on PPU
 */
#if !__SPU
void grcGeometryJobs::BeginFrame(void) 
{		
	// Ring buffer usage since last call to BeginFrame
	// notice that it may not actually be a real frame because it's asynchronous
	static uint32_t largestAlloc[EDGE_MAX_SPU]= {0, 0, 0, 0, 0};
	static uint64_t prevTotalAlloc[EDGE_MAX_SPU]= {0, 0, 0, 0, 0};
	for(uint32_t iSpu=0; iSpu<EDGE_MAX_SPU; ++iSpu) {	
		uint64_t curTotalAlloc = m_eaOutputBufferInfo->ringInfo[iSpu].totalAlloc;
		if (curTotalAlloc>prevTotalAlloc[iSpu]) {
			uint32_t alloc = (uint32_t) (curTotalAlloc - prevTotalAlloc[iSpu]);
			if (alloc>largestAlloc[iSpu]) {
				grcDebugf1("Edge largest ring buffer usage so far @ SPU%i = %uKB", iSpu, alloc >> 10);
				largestAlloc[iSpu] = alloc;
			}
		}
		prevTotalAlloc[iSpu] = curTotalAlloc;
	}
}

bool grcGeometryJobs::IsRingBufferFull(u32 size) const
{
	for(uint32_t iSpu=0; iSpu<EDGE_MAX_SPU; ++iSpu) {
		EdgeGeomRingBufferInfo* ringInfo = &m_eaOutputBufferInfo->ringInfo[iSpu];

		if (ringInfo->rsxLabelEa == 0)
			continue;

		u32 startPlusSize = ringInfo->startEa + size;
		u32 currentPlusSize = ringInfo->currentEa + size;
		u32 end = ringInfo->endEa;
		u32 current = ringInfo->currentEa;

		// Free
		uint32_t freeEnd = ringInfo->cachedFree ? ringInfo->cachedFree : *(u32*)ringInfo->rsxLabelEa;

		// Test to see if the allocation will not fit.
		if (!(((freeEnd <= current) || (currentPlusSize < freeEnd)) &&
			((currentPlusSize <= end) || (startPlusSize < freeEnd))))
		{
			return true;
		}
	}

	return false;
}
#endif //!__SPU


/*! To be called at the end of each frame.
 *
 *	\note		Currently only used for profiling / collecting stats, as everything else
 *				is handled by frame-independent ring buffers. 
 *
 *	\warning	Only available on PPU
 */
#if !__SPU
void grcGeometryJobs::EndFrame(void) 
{	
}
#endif //!__SPU

#else // !USE_EDGE

using namespace rage;

void* grcGeometryJobs::SetLocalStallHole(CellGcmContextData *, u32, u32, const spuGcmState*) {return NULL;}
void grcGeometryJobs::AddJob(CellGcmContextData *, 
	const EdgeGeomPpuConfigInfo *, const void *, 
	const Vector3&,
	u32, spuGcmState&) {}

#if HACK_GTA4
#if USE_EDGE && __PPU
void grcGeometryJobs::AddExtractJob(CellGcmContextData*, const EdgeGeomPpuConfigInfo*, 
	const EdgeGeomLocalToWorldMatrix*, const Vector4**, int, const u8*, unsigned, void*,
	Vector4*, void*, u16*, u32, u32, void*, float, const Vector3*, float
	#if HACK_GTA4_MODELINFOIDX_ON_SPU
	,CGta4DbgSpuInfoStruct*
	#endif		
	,Matrix43* ms,
	u8 extractMask) {}
#endif
#endif

int grcGeometryJobs::Initialize(CellSpurs*, unsigned int, unsigned int, unsigned int, u8, u8[8]) {return 0;}
int grcGeometryJobs::Shutdown(void) {return 0;}
void grcGeometryJobs::BeginFrame(void)	{}
void grcGeometryJobs::EndFrame(void) {}
bool grcGeometryJobs::IsRingBufferFull(u32) const { return false; }

#endif // USE_EDGE
