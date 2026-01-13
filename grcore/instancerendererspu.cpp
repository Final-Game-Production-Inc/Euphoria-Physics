// 
// grcore/instancerendererspu.cpp
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/instancerendererspu.h"

#define INSTANCE_RENDERER_DEBUG_SPEW (0)
#if INSTANCE_RENDERER_DEBUG_SPEW
#if __SPU
#define dprintf(f, args...) Printf("[SPU%d,%d,%d] " f "\n", cellSpursGetCurrentSpuId(), spuContext->FrameCount, __LINE__, ##args)
#else
#define dprintf(f, args...) Printf("[PPU,%d,%d] " f "\n", GRCDEVICE.GetFrameCounter(), __LINE__, ##args)
#endif // __SPU
#else
#define dprintf(...)
#endif // 0

#if __SPU
#include "grcore/edgeringbuffer.h"
#include "grcore/grcorespu.h"
#include "grcore/vertexdecl.h"
#include "math/amath.h"
#include "system/dma.h"
#include "system/spuintrinsics.h"
#include "system/memops.h"

#if __SPU
int32_t gcmCallback(CellGcmContextData *data,uint32_t /*amt*/) { __debugbreak(); return 0; }
#endif

namespace rage
{

typedef bool (InstanceShader)(grcInstanceRenderer::InstanceContext* context);

bool DownloadBatch(grcInstanceRenderer::SpuContext* spuContext, void* outputVertexDataLs, u32 numInstances, CellGcmContextData* gcmCtx)
{
	const u32 numOutputVertexes = numInstances * spuContext->NumVertexesPerInstance;
	const u32 batchSize = (numOutputVertexes * spuContext->VertexDecl->Stream0Size + 0x7f) & ~0x7f;
	dprintf("numOutputVertexes=%d batchSize=%d", numOutputVertexes, batchSize);

	dprintf("begin allocate EDGE fifo");
	EdgeGeomAllocationInfo allocInfo;
	AssertVerify(AllocateEdgeRingBuffer(spuContext->OutputBufferInfoEa, spuContext->InputDmaTag, spuContext->OutputDmaTag, batchSize, &allocInfo, spuContext->GcmControlEa));
	dprintf("end allocate EDGE fifo");

	Assert(allocInfo.location == CELL_GCM_LOCATION_LOCAL);
	Assert(allocInfo.rsxLabelEa);

	dprintf("dma vertex data");
	sysDmaLargePut(outputVertexDataLs, allocInfo.ea, batchSize, spuContext->OutputDmaTag);

	// If we've written output data to video memory, we need to make sure
	// that those output DMAs are finished before the command buffer hole
	// is filled, otherwise the RSX might try to access the data before
	// it's been committed to video memory.
	dprintf("sync vertex data");
	u32 syncData ALIGNED(128);
	sysDmaSmallGetf(&syncData, allocInfo.ea, sizeof(syncData), spuContext->InputDmaTag);

	dprintf("download command buffer hole");
	cellGcmSetInvalidateVertexCacheUnsafeInline(gcmCtx);

	u32 offsets[]  = { allocInfo.offset, 0, 0, 0 };
	ForceBindVertexDeclaration(gcmCtx, spuContext->VertexDecl, offsets, spuContext->VertexShaderInputs);

	static const u8 kDrawModeLUT[] =
	{
		CELL_GCM_PRIMITIVE_POINTS,
		CELL_GCM_PRIMITIVE_LINES,
		CELL_GCM_PRIMITIVE_LINE_STRIP,
		CELL_GCM_PRIMITIVE_TRIANGLES,
		CELL_GCM_PRIMITIVE_TRIANGLE_STRIP,
		CELL_GCM_PRIMITIVE_TRIANGLE_FAN,
		CELL_GCM_PRIMITIVE_QUADS
	};
	const u8 drawMode = kDrawModeLUT[spuContext->DrawMode];
	if (spuContext->IndexBufferOffset != 0)
	{
		cellGcmSetDrawIndexArrayUnsafeInline(gcmCtx, drawMode, numInstances * spuContext->NumIndexesPerInstance, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, spuContext->IndexBufferOffset >> 31, spuContext->IndexBufferOffset);
	}
	else
	{
		cellGcmSetDrawArraysUnsafeInline(gcmCtx, drawMode, 0, numOutputVertexes);
	}

	// If using ring buffers for output, then generate the command 
	// buffer commands that will update the RSX label value once 
	// the data created by this job is completely consumed.
	cellGcmSetWriteTextureLabelUnsafeInline(gcmCtx, (allocInfo.rsxLabelEa & 0x0FF0) >> 4, allocInfo.endEa);

	// 16 byte aligned
	while (reinterpret_cast<u32>(gcmCtx->current) & 15)
	{
		cellGcmSetNopCommandUnsafeInline(gcmCtx, 1);
	}

	u32 commandBufferHoleSize = (gcmCtx->current - gcmCtx->begin) << 2;

#if 0
	gcm::DumpFifo(gcmCtx->begin, gcmCtx->current);
#endif // 0

	sysDmaPutf(gcmCtx->begin, spuContext->CommandBufferHoleEa, commandBufferHoleSize, spuContext->OutputDmaTag);

	spuContext->CommandBufferHoleEa += commandBufferHoleSize;
	gcmCtx->current = gcmCtx->begin;

	sysDmaWaitTagStatusAll(1 << spuContext->OutputDmaTag);

	return true;
}

void instancerendererspu(sysTaskParameters& taskParams, CellSpursJobContext2* jobContext, CellSpursJob256* /*job*/)
{
	// Sync instruction stream, since we're DMA'ing in a code frag
	spu_sync_c();

#ifdef INSTANCE_SHADER_CALLBACK
	Assert(taskParams.Input.Size == sizeof(u128));
	*(InstanceShader**)(&taskParams.Input.Data) = &INSTANCE_SHADER_CALLBACK;
#endif // INSTANCE_SHADER_CALLBACK

	// We copy the SPU config structure inline into the job's user data
	grcInstanceRenderer::SpuContext _spuContext;
	grcInstanceRenderer::SpuContext* spuContext = &_spuContext;
	sysMemCpy(&_spuContext, (const void*)taskParams.UserData, sizeof(grcInstanceRenderer::SpuContext));
	spuContext->InputDmaTag = jobContext->dmaTag;
	spuContext->OutputDmaTag = jobContext->dmaTag;

	dprintf("begin");
	// DMA in vertex declaration
	u8 vertexDeclBuffer[(sizeof(spuVertexDeclaration) + 15) & ~15] ;
	const u32 vertexDeclEa = reinterpret_cast<u32>(spuContext->VertexDecl);
	spuContext->VertexDecl = reinterpret_cast<spuVertexDeclaration*>(vertexDeclBuffer);
	dprintf("get vertex decl");
	sysDmaGet(vertexDeclBuffer, vertexDeclEa, sizeof(vertexDeclBuffer), spuContext->InputDmaTag);

	const u32 jtsEa = spuContext->CommandBufferHoleEa;
	spuContext->CommandBufferHoleEa += 16; // JTS->JTN + 3 nops

	// Set up scratch memory
	u8* scratchPtr = reinterpret_cast<u8*>(taskParams.Scratch.Data);
	
	// Allocate space for type data
	void* const typeDataLs = spuContext->TypeDataSize > 0 ? reinterpret_cast<void* const>(scratchPtr) : NULL;
	if (typeDataLs)
	{
		// No need to wait just yet
		dprintf("get type data");
		sysDmaGet(typeDataLs, reinterpret_cast<u32>(spuContext->TypeData), spuContext->TypeDataSize, spuContext->InputDmaTag);
		scratchPtr += spuContext->TypeDataSize;
	}

	dprintf("get dma list");

	const u32 dmaListSize = (sizeof(grcInstanceRenderer::DmaListItem) * spuContext->DmaListCount + 15) & ~15;
	grcInstanceRenderer::DmaListItem* dmaList = reinterpret_cast<grcInstanceRenderer::DmaListItem*>(scratchPtr);
	sysDmaGet(dmaList, spuContext->DmaListEa, dmaListSize, spuContext->InputDmaTag);
	scratchPtr += dmaListSize;

	dprintf("setup finished");

	grcInstanceRenderer::InstanceContext instanceContext;
	instanceContext.InstanceIdx = 0;
	instanceContext.NumVertexesPerInstance = spuContext->NumVertexesPerInstance;
	instanceContext.TypeData = typeDataLs;
	instanceContext.InputDmaTag = spuContext->InputDmaTag;
	instanceContext.OutputDmaTag = spuContext->OutputDmaTag;

	// Wait for vertex decl to be DMA'ed in
	sysDmaWaitTagStatusAll(1 << spuContext->InputDmaTag);

	instanceContext.OutputVertexStride = spuContext->VertexDecl->Stream0Size;

	for (u32 i = 0; i < spuContext->DmaListCount; ++i)
	{
		grcInstanceRenderer::DmaListItem& dmaListItem = dmaList[i];
		instanceContext.InputInstanceSize = dmaListItem.ElementSize;

		u8* currScratchPtr = scratchPtr;
		// Allocate space for GCM context
		CellGcmContextData gcmCtx;
		gcmCtx.begin = gcmCtx.current = reinterpret_cast<u32*>(currScratchPtr);
		gcmCtx.end = gcmCtx.begin + dmaListItem.CommandBufferHoleScratchSize; // Size in words
		gcmCtx.callback = NULL;
		currScratchPtr += dmaListItem.CommandBufferHoleScratchSize << 2;

		// Allocate space for per instance data
		u8* const inputInstanceDataLs = currScratchPtr;
		currScratchPtr += dmaListItem.InputInstanceScratchSize;
		instanceContext.InputInstanceData = inputInstanceDataLs;

		// Allocate space for output vertex data
		u8* const outputVertexDataLs = currScratchPtr;
		currScratchPtr += dmaListItem.OutputVertexScratchSize;
		instanceContext.OutputVertexData = outputVertexDataLs;

		Assertf(currScratchPtr <= scratchPtr + taskParams.Scratch.Size, "overran scratch memory");

		u32 numInstancesPerBatch = 0;
		for (u32 j = 0; j < dmaListItem.Count; ++j)
		{
			const u32 batchIdx = j % dmaListItem.BatchCount;
			// Should we start a new batch?
			if (Unlikely(batchIdx == 0))
			{
				dprintf("get input instance data");
				instanceContext.InputInstanceData = inputInstanceDataLs + (dmaListItem.Ea & 15);
				const u32 batchSize = dmaListItem.ElementSize * dmaListItem.BatchCount;
				cellDmaUnalignedGet(instanceContext.InputInstanceData, dmaListItem.Ea, batchSize, spuContext->InputDmaTag, 0, 0);
				dmaListItem.Ea += batchSize;
				sysDmaWaitTagStatusAll(1 << spuContext->InputDmaTag);
			}

#if INSTANCE_RENDERER_DEBUG_SPEW
			checkSpursMarkers("begin code frag");
#endif // INSTANCE_RENDERER_DEBUG_SPEW

			const bool shouldOutput = (*(InstanceShader*)taskParams.Input.Data)(&instanceContext);

#if INSTANCE_RENDERER_DEBUG_SPEW
			checkSpursMarkers("end code frag");
#endif // INSTANCE_RENDERER_DEBUG_SPEW

			if (shouldOutput)
			{
				++numInstancesPerBatch;
				instanceContext.OutputVertexData += instanceContext.OutputVertexStride * spuContext->NumVertexesPerInstance;
				Assertf(instanceContext.OutputVertexData <= outputVertexDataLs + dmaListItem.OutputVertexScratchSize, "%p [%p, %p]", instanceContext.OutputVertexData, outputVertexDataLs, outputVertexDataLs + dmaListItem.OutputVertexScratchSize);

				// Have we finished this batch?
				if (Unlikely(numInstancesPerBatch == dmaListItem.BatchCount))
				{
					dprintf("download batch");
					AssertVerify(DownloadBatch(spuContext, outputVertexDataLs, numInstancesPerBatch, &gcmCtx));
					instanceContext.OutputVertexData = outputVertexDataLs;
					numInstancesPerBatch = 0;
				}
			}

			// Advance pointers and such
			++instanceContext.InstanceIdx;
			instanceContext.InputInstanceData += dmaListItem.ElementSize;
			Assertf(instanceContext.InputInstanceData <= inputInstanceDataLs + dmaListItem.InputInstanceScratchSize, "%p [%p, %p]", instanceContext.InputInstanceData, inputInstanceDataLs, inputInstanceDataLs + dmaListItem.InputInstanceScratchSize);
		}

		// Is there a left over batch?
		if (numInstancesPerBatch > 0)
		{
			dprintf("download leftover batch");
			AssertVerify(DownloadBatch(spuContext, outputVertexDataLs, numInstancesPerBatch, &gcmCtx));
		}
	}

#if __ASSERT
	// Make sure that what we're replacing really is a jump!
	uint32_t wasJump = sysDmaGetUInt32(jtsEa, spuContext->OutputDmaTag);
	Assert(wasJump & CELL_GCM_METHOD_FLAG_JUMP);
	Assertf((wasJump & 4095) == (jtsEa & 4095),"%x != %x",wasJump,jtsEa);
#endif
	sysDmaPutUInt32(CELL_GCM_METHOD_FLAG_JUMP | spuContext->JumpToNextOffset, jtsEa, spuContext->OutputDmaTag);

	dprintf("end");
}

} // namespace rage

#elif __PPU

#include "grcore/device.h"
#include "grcore/edge_jobs.h"
#include "grcore/indexbuffer.h"
#include "grcore/gcmringbuffer.h"
#include "grcore/wrapper_gcm.h"
#include "system/task.h"

DECLARE_TASK_INTERFACE(instancerendererspu);

namespace rage
{

// from effect_gcm.cpp
extern u16 g_VertexShaderInputs;

// from device_gcm.cpp
extern const grcVertexDeclaration* grcCurrentVertexDeclaration;

static gcmRingBuffer s_RingBuffer;
static bool s_Initialized = false;
static u32 s_RingBufferSize = 0;
static u32 s_ReferenceCount = 0;

static u32 GetCommandBufferHolePerBatchSize(const grcInstanceRenderer::PpuContext* ppuContext, u32 numInstancesPerBatch)
{
	CellGcmContextData measureContext;
	measureContext.begin = NULL;
	measureContext.end = NULL;
	measureContext.current = measureContext.begin;
	measureContext.callback = NULL;

	cellGcmSetInvalidateVertexCacheMeasure(&measureContext);

	measureContext.current += 44; // BindVertexDeclaration

	static const u8 kDrawModeLUT[] =
	{
		CELL_GCM_PRIMITIVE_POINTS,
		CELL_GCM_PRIMITIVE_LINES,
		CELL_GCM_PRIMITIVE_LINE_STRIP,
		CELL_GCM_PRIMITIVE_TRIANGLES,
		CELL_GCM_PRIMITIVE_TRIANGLE_STRIP,
		CELL_GCM_PRIMITIVE_TRIANGLE_FAN,
		CELL_GCM_PRIMITIVE_QUADS
	};

	if (ppuContext->IndexBuffer != NULL)
	{
		// The final parameter of cellGcmSetDrawIndexArray is 126 (=0x7e).
		// This is the function's worst-case alignment, and generates another
		// 8 bytes worth of commands. 
		const u32 numIndexes = numInstancesPerBatch * ppuContext->NumIndexesPerInstance;
		const u32 offset = 0x7e;
		cellGcmSetDrawIndexArrayMeasure(&measureContext, kDrawModeLUT[ppuContext->DrawMode], numIndexes, CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16, offset >> 31, offset);
	}
	else
	{
		const u32 numVertexes = numInstancesPerBatch * ppuContext->NumVertexesPerInstance;
		cellGcmSetDrawArraysMeasure(&measureContext, kDrawModeLUT[ppuContext->DrawMode], 0, numVertexes);
	}

	// Sync for EDGE ring buffer
	cellGcmSetWriteTextureLabelMeasure(&measureContext, 0, 0);

	// 16 byte aligned
	return (measureContext.current - measureContext.begin + 3) & ~3;
}

static u32 GetCommandBufferHoleSize(grcInstanceRenderer::PpuContext* ppuContext, u32 scratchSize)
{
	const u32 outputVertexSizePerInstance = ppuContext->VertexDecl->Stream0Size * ppuContext->NumVertexesPerInstance;

	u32 totalContextSize = 0;
	for (u32 i = 0; i < ppuContext->DmaListCount; ++i)
	{
		grcInstanceRenderer::DmaListItem& dmaListItem = ppuContext->DmaList[i];
		dmaListItem.BatchCount = dmaListItem.Count;
		
		u32 batchSize;
		u32 lastBatchCount;
		do
		{
			FastAssert(dmaListItem.BatchCount > 0);

			// 16 bytes padding for unaligned DMA's
			dmaListItem.InputInstanceScratchSize = ((dmaListItem.ElementSize * dmaListItem.BatchCount + 15) & ~15) + 16;
			// Align to 128 bytes for Edge DMA's
			dmaListItem.OutputVertexScratchSize = (outputVertexSizePerInstance * dmaListItem.BatchCount + 127) & ~127;

			// FIFO overhead per batch
			dmaListItem.CommandBufferHoleScratchSize = GetCommandBufferHolePerBatchSize(ppuContext, dmaListItem.BatchCount);
			batchSize = dmaListItem.InputInstanceScratchSize + dmaListItem.OutputVertexScratchSize + (dmaListItem.CommandBufferHoleScratchSize << 2);

			lastBatchCount = dmaListItem.BatchCount;
			dmaListItem.BatchCount /= 2;
		}
		while (batchSize > scratchSize);

		dmaListItem.BatchCount = lastBatchCount;

		const u32 numFullBatches = dmaListItem.Count / dmaListItem.BatchCount;
		u32 contextSize = dmaListItem.CommandBufferHoleScratchSize * numFullBatches;

		const u32 leftOverInstanceCount = dmaListItem.Count % dmaListItem.BatchCount;
		if (leftOverInstanceCount > 0)
		{
			contextSize += GetCommandBufferHolePerBatchSize(ppuContext, leftOverInstanceCount);
		}

		totalContextSize += contextSize;
	}

	return totalContextSize;
}

bool grcInstanceRenderer::InitializeRingBuffer()
{
	if (Unlikely(!s_Initialized))
	{
		if (Unlikely(s_ReferenceCount == 0))
		{
			return false;
		}

		u32* ringBase = GRCDEVICE.CreateDisplayList(s_RingBufferSize);
		Assert(ringBase);
		const u32 labelId = gcm::RsxSemaphoreRegistrar::Allocate(1);
		Assert(gcm::RsxSemaphoreRegistrar::IsValid(labelId));
		// DMA tag doesn't matter when ring buffer is only used on the PPU
		const int dmaTag = 0;
		s_RingBuffer.Init("SPU instance renderer FIFO", ringBase, s_RingBufferSize << 2, labelId, dmaTag);

		s_Initialized = true;
	}

	return true;
}

void grcInstanceRenderer::Init(u32 ringBufferSize)
{
	FastAssert(!s_Initialized);
	FastAssert(ringBufferSize > 0);

	s_ReferenceCount++;
	s_RingBufferSize = Max(ringBufferSize >> 2, s_RingBufferSize);
}

void grcInstanceRenderer::Shutdown()
{
	FastAssert(s_ReferenceCount > 0);
	s_ReferenceCount--;

	if (!s_Initialized || s_ReferenceCount > 0)
	{
		return;
	}

	GRCDEVICE.DeleteDisplayList(s_RingBuffer.GetRingBeginOffset());
	gcm::RsxSemaphoreRegistrar::Free(s_RingBuffer.GetLabelId());

	s_Initialized = false;
	s_RingBufferSize = 0;
}

extern u32 grcCurrentVertexOffsets[];

void grcInstanceRenderer::Create(grcInstanceRenderer::PpuContext* ppuContext, const char* codeName, void* codeStart, u32 codeSize, bool isCodeFrag /*= true*/)
{
	if (Unlikely(!InitializeRingBuffer()))
		return;

	// Lots of validation
	FastAssert(s_ReferenceCount > 0);
	FastAssert(ppuContext != NULL);
	FastAssert(codeStart != NULL);
	FastAssert(codeSize > 0);
	FastAssert(ppuContext->DmaList);
	FastAssert(ppuContext->DmaListCount >= 1);
	FastAssert(ppuContext->DmaList->Ea != 0);
	FastAssert(ppuContext->DmaList->Count > 0);
	FastAssert(ppuContext->DmaList->ElementSize > 0);
	FastAssert(!ppuContext->IndexBuffer || ppuContext->NumIndexesPerInstance > 0);

	// Make sure that we have *both* a type data size and type data pointer
	FastAssert(ppuContext->TypeDataSize == 0 || ppuContext->TypeData != NULL);
	FastAssert(ppuContext->TypeData == NULL || ppuContext->TypeDataSize > 0);

	// Make sure that we have a vertex declaration
	FastAssert(ppuContext->VertexDecl != NULL);

	// Make sure that all data is suitably aligned for DMA
	FastAssert((reinterpret_cast<u32>(ppuContext->VertexDecl) & 15) == 0);
	FastAssert((reinterpret_cast<u32>(codeStart) & 15) == 0);
	FastAssert(ppuContext->CacheTypeData || ((reinterpret_cast<u32>(ppuContext->TypeData) & 15) == 0));

	// Make sure we don't try and DMA anything from VRAM (because it's really slow)
	FastAssert(!gcm::IsLocalPtr(reinterpret_cast<void*>(ppuContext->DmaList->Ea)));
	FastAssert(!gcm::IsLocalPtr(ppuContext->VertexDecl));
	FastAssert(!gcm::IsLocalPtr(codeStart));
	FastAssert(!ppuContext->TypeData || !gcm::IsLocalPtr(ppuContext->TypeData));

	grcJobParameters taskParams;
	taskParams.SpuStackSize = 8 * 1024; // 8k is the default
	
	codeSize = (codeSize + 15) & ~15;
	if (isCodeFrag)
	{
		taskParams.InputData = codeStart;
		taskParams.InputSize = codeSize;
		// Size of wrapper code
		codeSize += (reinterpret_cast<u32>(TASK_INTERFACE_SIZE(instancerendererspu)) + 15) & ~15;
	}
	else
	{
		static u128 dontcare;
		taskParams.InputData = &dontcare;
		taskParams.InputSize = sizeof(dontcare);
		// Size of dummy input data
		codeSize += sizeof(dontcare);
	}
	taskParams.UserDataCount = sizeof(grcInstanceRenderer::SpuContext) / sizeof(taskParams.UserData[0]);
	const u32 typeDataSize = (ppuContext->TypeDataSize + 15) & ~15;
	const u32 dmaListSize = (sizeof(grcInstanceRenderer::DmaListItem) * ppuContext->DmaListCount + 15) & ~15;
	taskParams.ScratchSize = (
		kMaxSPUJobSize -
		codeSize -
		taskParams.SpuStackSize -
		sizeof(taskParams.UserData) -
		typeDataSize -
		dmaListSize
	+ 15) & ~15;

	grcInstanceRenderer::SpuContext* spuContext = reinterpret_cast<grcInstanceRenderer::SpuContext*>(taskParams.UserData);
	spuContext->NumVertexesPerInstance = ppuContext->NumVertexesPerInstance;
	spuContext->NumIndexesPerInstance = ppuContext->NumIndexesPerInstance;
	spuContext->OutputBufferInfoEa = GEOMETRY_JOBS.GetOutputBufferInfoEa();
	FastAssert(!(spuContext->OutputBufferInfoEa & 0x00000002));
	spuContext->DrawMode = ppuContext->DrawMode;
	CompileTimeAssert(sizeof(spuVertexDeclaration) == sizeof(grcVertexDeclaration));
	spuContext->VertexDecl = reinterpret_cast<spuVertexDeclaration*>(ppuContext->VertexDecl);
	spuContext->VertexShaderInputs = g_VertexShaderInputs;
	spuContext->TypeData = ppuContext->TypeData;
	spuContext->TypeDataSize = typeDataSize;
	spuContext->FrameCount = GRCDEVICE.GetFrameCounter();
	spuContext->IndexBufferOffset = ppuContext->IndexBuffer ? static_cast<grcIndexBufferGCM*>(ppuContext->IndexBuffer)->GetGCMOffset() : 0;
	spuContext->GcmControlEa = reinterpret_cast<u32>(cellGcmGetControlRegister());
	spuContext->NonZeroStreamOffsets[0] = grcCurrentVertexOffsets[1];
	spuContext->NonZeroStreamOffsets[1] = grcCurrentVertexOffsets[2];
	spuContext->NonZeroStreamOffsets[2] = grcCurrentVertexOffsets[3];
	
	// Size in words
	dprintf("GetCommandBufferHoleSize");
	spuContext->CommandBufferHoleSize = GetCommandBufferHoleSize(ppuContext, taskParams.ScratchSize);
	// +4 for JTS and alignment padding, +4 for jump back and 16 byte alignment
	const u32 gcmCtxSizeWords = spuContext->CommandBufferHoleSize + 4 + 4;
	const u32 typeDataSizeWords = ppuContext->CacheTypeData ? typeDataSize >> 2 : 0;
	const u32 dmaListSizeWords = dmaListSize >> 2;
	const u32 ringSizeWords = gcmCtxSizeWords + typeDataSizeWords + dmaListSizeWords;
	dprintf("allocate ring buffer");

	u32 vertexScratchSize = 0;
	for (u32 i = 0; i < ppuContext->DmaListCount; ++i)
	{
		vertexScratchSize += ppuContext->DmaList[i].OutputVertexScratchSize;
	}
	bool forceKick = GEOMETRY_JOBS.IsRingBufferFull(vertexScratchSize);
	u32* ringMem = AllocateRingBuffer(ringSizeWords, forceKick);
	Assert(ringMem);

	u32* currRingMem = ringMem;

	CellGcmContextData gcmCtx;
	gcmCtx.begin = gcmCtx.current = currRingMem;
	currRingMem += gcmCtxSizeWords;
	gcmCtx.end = gcmCtx.begin + gcmCtxSizeWords;
	gcmCtx.callback = NULL;

	// Where we start DMA'ing GPU commands to
	spuContext->CommandBufferHoleEa = reinterpret_cast<u32>(gcmCtx.begin);

	// The GPU memory offset of the beginning of our segment
	const u32 selfOffset = gcm::GetOffset(gcmCtx.begin);

	// Jump to next when we finish the SPU job
	spuContext->JumpToNextOffset = selfOffset + 4;

	// Jump to self until we finish the SPU job
	cellGcmSetJumpCommandUnsafeInline(&gcmCtx, selfOffset);

	cellGcmSetNopCommand(&gcmCtx, gcmCtx.end - gcmCtx.current);

	if (ppuContext->CacheTypeData)
	{
		dprintf("cache type data");
		spuContext->TypeData = currRingMem;
		currRingMem += typeDataSizeWords;
		sysMemCpy(spuContext->TypeData, ppuContext->TypeData, ppuContext->TypeDataSize);
	}

	dprintf("copy dma list");
	sysMemCpy(currRingMem, ppuContext->DmaList, dmaListSizeWords << 2);
	spuContext->DmaListEa = reinterpret_cast<u32>(currRingMem);
	currRingMem += dmaListSizeWords;
	spuContext->DmaListCount = ppuContext->DmaListCount;

	dprintf("insert jump spu command");

	// Jump from the virtual FIFO to our segment
	SPU_COMMAND(grcDevice__Jump, 0);
	cmd->jumpLocalOffset = selfOffset;
	cmd->jumpBack = gcmCtx.end - 1;

	// This must happen after the jump back in the main context
	dprintf("free ring buffer");
	FreeRingBuffer(ringMem, ringSizeWords, GCM_CONTEXT);

	// The GPU will hang if we don't tell the renderer that we're
	// changing the current vertex declaration before any subsequent
	// immediate mode rendering
	grcCurrentVertexDeclaration = ppuContext->VertexDecl;

	dprintf("kick job");
	if (isCodeFrag)
		grcCreateGraphicsJob(TASK_INTERFACE(instancerendererspu), taskParams);
	else
		grcCreateGraphicsJob(codeName, reinterpret_cast<char*>(codeStart), reinterpret_cast<char*>(codeSize), taskParams);

	InvalidateSpuGcmState(CachedStates.VertexFormats, ~0);
}

u32* grcInstanceRenderer::AllocateRingBuffer(u32 wordCount, bool forceKick /*= false*/)
{
	if (Unlikely(!InitializeRingBuffer()))
	{
		return NULL;
	}

	// round up allocations to 16 bytes for DMA's
	u32 ringSize = ((wordCount + 3) & ~3) << 2;
	void* success;

	if (Unlikely(forceKick))
	{
		GRCDEVICE.KickOffGpu();
		success = s_RingBuffer.Allocate(ringSize);
	}
	else
	{
		success = s_RingBuffer.TryAllocate(ringSize);
		if (Unlikely(!success))
		{
			GRCDEVICE.KickOffGpu();
			success = s_RingBuffer.Allocate(ringSize);
		}
	}

	return reinterpret_cast<u32*>(success);
}

void grcInstanceRenderer::FreeRingBuffer(void* ptr, u32 wordCount, CellGcmContextData* gcmCtx)
{
	if (Unlikely(!InitializeRingBuffer()))
	{
		return;
	}

	u32 ringSize = ((wordCount + 3) & ~3) << 2;
	return s_RingBuffer.Free((char*)ptr+ringSize, gcmCtx); // round up allocations to 16 bytes for DMA's
}

} // namespace rage

#endif
