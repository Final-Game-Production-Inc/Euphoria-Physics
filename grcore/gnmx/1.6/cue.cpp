/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.071
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/common.h"
#ifdef CUE_V2

#include <cstring>
#include <x86intrin.h>
#include "grcore/gnmx/cue.h"
#include "grcore/gnmx/gfxcontext.h"
#include "grcore/gnmx/cue-helper.h"

using namespace sce::Gnm;
using namespace sce::Gnmx;


//////////////////////////////////////

namespace
{
	const uint32_t kWaitOnDeCounterDiffSizeInDword = 2;
}

ConstantUpdateEngine::ConstantUpdateEngine(void)
{
}


ConstantUpdateEngine::~ConstantUpdateEngine(void)
{
}


void ConstantUpdateEngine::init(void *heapAddr, uint32_t numRingEntries)
{
	SCE_GNM_VALIDATE(heapAddr != NULL, "heapAddr must not be NULL.");
	SCE_GNM_VALIDATE((uintptr_t(this) & 0xf) == 0, "ConstantUpdateEngine object (0x%p) must be aligned to a 16-byte boundary.", this);
	SCE_GNM_VALIDATE((uintptr_t(heapAddr) & 0x3) == 0, "heapAddr (0x%p) must be aligned to a 4-byte boundary.", heapAddr);
	SCE_GNM_VALIDATE(numRingEntries>=4, "If hardware kcache invalidation is enabled, numRingEntries (%d) must be at least 4.", numRingEntries);

	m_dcb					= 0;
	m_ccb					= 0;
	m_activeShaderStages	= Gnm::kActiveShaderStagesVsPs;
	m_numRingEntries		= numRingEntries;
	m_shaderUsesSrt			= 0;
	m_anyWrapped			= true;
	m_dirtyVsOrPs			= false;
	m_psInputs				= NULL;

	m_currentVSB = NULL;
	m_currentPSB = NULL;
	m_currentLSB = NULL;
	m_currentESB = NULL;
	memset(m_stageInfo, 0, sizeof(m_stageInfo));

	m_streamoutBufferStage.usedChunk = 0;
	m_streamoutBufferStage.curChunk  = 0;
	m_streamoutBufferStage.curSlots  = 0;
	m_streamoutBufferStage.usedSlots = 0;
	m_eudReferencesStreamoutBuffers	 = false;

	// Divvy up the heap between the ring buffers for each shader stage.
	// This is a duplicate of the body of ConstantUpdateEngine::computeHeapSize(). We don't
	// just call computeHeapSize() here because init() needs the intermediate k*RingBufferBytes values.
	const uint32_t kResourceRingBufferBytes	        = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kChunkSlotCountResource        *Gnm::kDwordSizeResource,         numRingEntries);
	const uint32_t kRwResourceRingBufferBytes	    = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kChunkSlotCountRwResource      *Gnm::kDwordSizeRwResource,       numRingEntries);
	const uint32_t kSamplerRingBufferBytes		    = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kChunkSlotCountSampler         *Gnm::kDwordSizeSampler,          numRingEntries);
	const uint32_t kVertexBufferRingBufferBytes     = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kChunkSlotCountVertexBuffer    *Gnm::kDwordSizeVertexBuffer,     numRingEntries);
	const uint32_t kConstantBufferRingBufferBytes   = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kChunkSlotCountConstantBuffer  *Gnm::kDwordSizeConstantBuffer,   numRingEntries);
	const uint32_t kStreamoutBufferRingBufferBytes  = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kChunkSlotCountStreamoutBuffer *Gnm::kDwordSizeStreamoutBuffer,  numRingEntries);
	const uint32_t kDispatchDrawDataRingBufferBytes = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kChunkSlotCountDispatchDrawData*Gnm::kDwordSizeDispatchDrawData, numRingEntries);

	m_heapSize = computeHeapSize(numRingEntries);

	// Initialize ring buffers
	void *freeAddr = heapAddr;
	for(int iStage=0; iStage<Gnm::kShaderStageCount; ++iStage)
	{
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexResource],		 freeAddr, kResourceRingBufferBytes,		 Gnm::kChunkSlotCountResource*Gnm::kDwordSizeResource,				   m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexRwResource],		 freeAddr, kRwResourceRingBufferBytes,		 Gnm::kChunkSlotCountRwResource*Gnm::kDwordSizeRwResource,			   m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexSampler],			 freeAddr, kSamplerRingBufferBytes,			 Gnm::kChunkSlotCountSampler*Gnm::kDwordSizeSampler,				   m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexVertexBuffer],	 freeAddr, kVertexBufferRingBufferBytes,	 Gnm::kChunkSlotCountVertexBuffer*Gnm::kDwordSizeVertexBuffer,		   m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexConstantBuffer],	 freeAddr, kConstantBufferRingBufferBytes,	 Gnm::kChunkSlotCountConstantBuffer*Gnm::kDwordSizeConstantBuffer,	   m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexStreamoutBuffer],  freeAddr, kStreamoutBufferRingBufferBytes,  Gnm::kChunkSlotCountStreamoutBuffer*Gnm::kDwordSizeStreamoutBuffer,   m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexDispatchDrawData], freeAddr, kDispatchDrawDataRingBufferBytes, Gnm::kChunkSlotCountDispatchDrawData*Gnm::kDwordSizeDispatchDrawData, m_numRingEntries);
	}

	const uint32_t actualUsed = static_cast<uint32_t>(uintptr_t(freeAddr) - uintptr_t(heapAddr));
	SCE_GNM_ASSERT(actualUsed == m_heapSize); // sanity check
	SCE_GNM_UNUSED(actualUsed);

	memset(m_globalTablePtr, 0, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE);
	m_globalTableNeedsUpdate = false;
}


//------


void ConstantUpdateEngine::setGlobalResourceTableAddr(void *addr)
{
	m_globalTableAddr = addr;
}


void ConstantUpdateEngine::setGlobalDescriptor(Gnm::ShaderGlobalResourceType resType, const Gnm::Buffer *res)
{
	((__int128_t*)m_globalTablePtr)[resType] = *(__int128_t*)res;
	m_globalTableNeedsUpdate = true;
}


//------


void ConstantUpdateEngine::applyInputUsageData(Gnm::ShaderStage currentStage)
{
	const uint8_t			currentStageMask = 1 << currentStage;
	StageInfo * const		stageInfo		 = m_stageInfo+currentStage;

	const Gnm::InputUsageSlot		*inputUsageTable     = stageInfo->inputUsageTable;
	const uint32_t					 inputUsageTableSize = stageInfo->inputUsageTableSize;
	const uint32_t					 usesSrt			 = m_shaderUsesSrt & currentStageMask;

	uint32_t usedRingBuffer = 0;		// Bitfield representing which ring are actually used by the shaders.

	SCE_GNM_VALIDATE( inputUsageTableSize, "This function should not be called if inputUsageTableSize is 0.");

	// Do we need to allocate the EUD?
	uint32_t * newEud = 0;
	if ( stageInfo->eudSizeInDWord && (m_shaderDirtyEud & currentStageMask) )
	{
		stageInfo->eudAddr = (uint32_t*)m_dcb->allocateFromCommandBuffer(stageInfo->eudSizeInDWord*4, Gnm::kEmbeddedDataAlignment128);
		ConstantUpdateEngineHelper::cleanEud(this, currentStage);
		newEud = stageInfo->eudAddr;
	}

#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
	static const uint32_t nullData[8] = {};
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

	// Build the ring address tables:
	uint16_t ringBufferActiveSize[kNumRingBuffersPerStage];

	// Populate the USGPRs:
	uint32_t iUsageSlot = 0;
	while ( iUsageSlot < inputUsageTableSize )
	{
		const uint32_t destUsgpr  = inputUsageTable[iUsageSlot].m_startRegister;

		// EUD? Early exit.
		if ( destUsgpr > 15 )
			break;

		const uint32_t srcApiSlot = inputUsageTable[iUsageSlot].m_apiSlot;
		const uint32_t regCount   = (inputUsageTable[iUsageSlot].m_registerCount*4) + 4; // 1 -> 8 DW; 0 -> 4 DW

		switch ( inputUsageTable[iUsageSlot].m_usageType )
		{
		 case Gnm::kShaderInputUsageImmResource:
			 {
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				 if ( stageInfo->activeResource & (((__int128_t)1)<<srcApiSlot) )
				 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

					 const uint32_t			 chunkId   = srcApiSlot / kResourceChunkSize;
					 const uint32_t			 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kResourceChunkSizeMask);
					 const uint32_t			 usedCur   = stageInfo->resourceStage[chunkId].curSlots & (1 << (15-chunkSlot));
					 const __int128_t		*chunk	   = usedCur ? stageInfo->resourceStage[chunkId].curChunk : stageInfo->resourceStage[chunkId].usedChunk;
					 const uint32_t			*dataPtr   = (const uint32_t*)(chunk+chunkSlot*ConstantUpdateEngineHelper::kResourceSizeInDqWord);

					 SCE_GNM_VALIDATE(stageInfo->activeResource & (((__int128_t)1)<<srcApiSlot),
									  "The shader requires a resource in slot %i which hasn't been set.\n"
									  "If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
					 m_dcb->setUserDataRegion(currentStage, destUsgpr, dataPtr, regCount);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				 }
				 else
				 {
					 m_dcb->setUserDataRegion(currentStage, destUsgpr, nullData, regCount);
				 }
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			 }
			 break;
		 case Gnm::kShaderInputUsageImmSampler:
			 {
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				 if ( stageInfo->activeSampler & (1<<srcApiSlot) )
				 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 const uint32_t			 chunkId   = srcApiSlot / kSamplerChunkSize;
					 const uint32_t			 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kSamplerChunkSizeMask);
					 const uint32_t			 usedCur   = stageInfo->samplerStage[chunkId].curSlots & (1 << (15-chunkSlot));
					 const __int128_t		*chunk	   = usedCur ? stageInfo->samplerStage[chunkId].curChunk : stageInfo->samplerStage[chunkId].usedChunk;
					 const uint32_t			*dataPtr   = (const uint32_t*)(chunk+chunkSlot*ConstantUpdateEngineHelper::kSamplerSizeInDqWord);

					 SCE_GNM_VALIDATE(stageInfo->activeSampler & (1<<srcApiSlot),
									  "The shader requires a sampler in slot %i which hasn't been set.\n"
									  "If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
					 m_dcb->setSsharpInUserData(currentStage, destUsgpr, (Gnm::Sampler*)dataPtr);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				 }
				 else
				 {
					 m_dcb->setSsharpInUserData(currentStage, destUsgpr, (Gnm::Sampler*)nullData);
				 }
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			 }
			 break;
		 case Gnm::kShaderInputUsageImmConstBuffer:
			 {
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				 if ( stageInfo->activeConstantBuffer & (1<<srcApiSlot) )
				 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 const uint32_t			 chunkId   = srcApiSlot / kConstantBufferChunkSize;
					 const uint32_t			 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kConstantBufferChunkSizeMask);
					 const uint32_t			 usedCur   = stageInfo->constantBufferStage[chunkId].curSlots & (1 << (15-chunkSlot));
					 const __int128_t		*chunk	   = usedCur ? stageInfo->constantBufferStage[chunkId].curChunk : stageInfo->constantBufferStage[chunkId].usedChunk;
					 const uint32_t			*dataPtr   = (const uint32_t*)(chunk+chunkSlot*ConstantUpdateEngineHelper::kConstantBufferSizeInDqWord);

					 SCE_GNM_VALIDATE(stageInfo->activeConstantBuffer & (1<<srcApiSlot),
									  "The shader requires a constant buffer in slot %i which hasn't been set.\n"
									  "If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
					 m_dcb->setVsharpInUserData(currentStage, destUsgpr, (Gnm::Buffer*)dataPtr);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				 }
				 else
				 {
					 m_dcb->setVsharpInUserData(currentStage, destUsgpr, (Gnm::Buffer*)nullData);
				 }
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			 }
			 break;
		 case Gnm::kShaderInputUsageImmVertexBuffer:
			SCE_GNM_ERROR("Not yet implemented!\n");
			break;
		 case Gnm::kShaderInputUsageImmRwResource:
			 {
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				 if ( stageInfo->activeRwResource & (1<<srcApiSlot) )
				 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 const uint32_t			 chunkId   = srcApiSlot / kRwResourceChunkSize;
					 const uint32_t			 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kRwResourceChunkSizeMask);
					 const uint32_t			 usedCur   = stageInfo->rwResourceStage[chunkId].curSlots & (1 << (15-chunkSlot));
					 const __int128_t		*chunk	   = usedCur ? stageInfo->rwResourceStage[chunkId].curChunk : stageInfo->rwResourceStage[chunkId].usedChunk;
					 const uint32_t			*dataPtr   = (const uint32_t*)(chunk+chunkSlot*ConstantUpdateEngineHelper::kRwResourceSizeInDqWord);

					 SCE_GNM_VALIDATE(stageInfo->activeRwResource & (1<<srcApiSlot),
									  "The shader requires a rw resource in slot %i which hasn't been set.\n"
									  "If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
					 m_dcb->setUserDataRegion(currentStage, destUsgpr, dataPtr, regCount);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				 }
				 else
				 {
					 m_dcb->setUserDataRegion(currentStage, destUsgpr, nullData, regCount);
				 }
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			 }
			 break;
		 case Gnm::kShaderInputUsageImmAluFloatConst:
			SCE_GNM_ERROR("Not yet implemented!\n");
			break;
		 case Gnm::kShaderInputUsageImmAluBool32Const:
			m_dcb->setUserData(currentStage, destUsgpr, stageInfo->boolValue);
			break;
		 case Gnm::kShaderInputUsageImmGdsCounterRange:
			m_dcb->setUserData(currentStage, destUsgpr, stageInfo->appendConsumeDword);
			break;
		 case Gnm::kShaderInputUsageImmGdsMemoryRange:
		 case Gnm::kShaderInputUsageImmGwsBase:
			SCE_GNM_ERROR("Not yet implemented!\n");
			break;
		 case Gnm::kShaderInputUsageImmShaderResourceTable:
			 {
				 SCE_GNM_VALIDATE(usesSrt, "non-SRT shaders should not be looking for Shader Resource Tables!");
				 const uint32_t dwCount = inputUsageTable[iUsageSlot].m_srtSizeInDWordMinusOne+1;
				 SCE_GNM_VALIDATE(dwCount == stageInfo->userSrtBufferSizeInDwords, "SRT user data size mismatch: shader expected %d DWORDS, but caller provided %d.",
								  dwCount, stageInfo->userSrtBufferSizeInDwords);
				 m_dcb->setUserDataRegion(currentStage, destUsgpr, stageInfo->userSrtBuffer, dwCount);
			 }
			break;
		 case Gnm::kShaderInputUsageSubPtrFetchShader:
			SCE_GNM_VALIDATE(destUsgpr == 0, "Fetch shader are expected to be set in usgpr 0 -- Please check input data.");
			break;
		 case Gnm::kShaderInputUsagePtrResourceTable:
			 {
				 if ( stageInfo->dirtyRing & (1 << (31-kRingBuffersIndexResource)) )
					updateChunkState256(stageInfo->resourceStage, kResourceNumChunks);

				 void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexResource);
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, ringBufferAddr);
				 usedRingBuffer |= 1<< (31-kRingBuffersIndexResource);
				 ringBufferActiveSize[kRingBuffersIndexResource] = 128 - ConstantUpdateEngineHelper::lzcnt128(stageInfo->activeResource);
			 }
			 break;
		 case Gnm::kShaderInputUsagePtrInternalResourceTable:
			SCE_GNM_ERROR("Not yet implemented!\n");
			break;
		 case Gnm::kShaderInputUsagePtrSamplerTable:
			 {
				 if ( stageInfo->dirtyRing & (1 << (31-kRingBuffersIndexSampler)) )
					updateChunkState128(stageInfo->samplerStage, kSamplerNumChunks);

				 void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexSampler);
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, ringBufferAddr);
				 usedRingBuffer |= 1<< (31-kRingBuffersIndexSampler);
				 ringBufferActiveSize[kRingBuffersIndexSampler] = 16 - __lzcnt16(stageInfo->activeSampler);
			 }
			 break;
		 case Gnm::kShaderInputUsagePtrConstBufferTable:
			 {
				 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexConstantBuffer)) )
					 updateChunkState128(stageInfo->constantBufferStage	, kConstantBufferNumChunks);

				 void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexConstantBuffer);
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, ringBufferAddr);
				 usedRingBuffer |= 1<< (31-kRingBuffersIndexConstantBuffer);
				 ringBufferActiveSize[kRingBuffersIndexConstantBuffer] = 32 - __lzcnt32(stageInfo->activeConstantBuffer);
			 }
			 break;
		 case Gnm::kShaderInputUsagePtrVertexBufferTable:
			 {
				 ringBufferActiveSize[kRingBuffersIndexVertexBuffer] = 32 - __lzcnt32(stageInfo->activeVertexBuffer);

				 if ( ringBufferActiveSize[kRingBuffersIndexVertexBuffer] <= 8 )
				 {
					 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexVertexBuffer)) )
						 updateChunkState128(stageInfo->vertexBufferStage, 1);

					 SCE_GNM_VALIDATE(stageInfo->vertexBufferStage[0].curChunk, "Invalid pointer.\n");
					 m_dcb->setPointerInUserData(currentStage, destUsgpr, stageInfo->vertexBufferStage[0].curChunk);
				 }
				 else
				 {
					 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexVertexBuffer)) )
						 updateChunkState128(stageInfo->vertexBufferStage, kVertexBufferNumChunks);

					 void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexVertexBuffer);
					 m_dcb->setPointerInUserData(currentStage, destUsgpr, ringBufferAddr);
					 usedRingBuffer |= 1<< (31-kRingBuffersIndexVertexBuffer);
				 }
			 }
			 break;
		 case Gnm::kShaderInputUsagePtrSoBufferTable:
			 {
				 updateChunkState128(&m_streamoutBufferStage, 1);
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, m_streamoutBufferStage.curChunk);
			 }
			break;
		 case Gnm::kShaderInputUsagePtrRwResourceTable:
			 {
				 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexRwResource)) )
					 updateChunkState256(stageInfo->rwResourceStage, kRwResourceNumChunks);

				 void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexRwResource);
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, ringBufferAddr);
				 usedRingBuffer |= 1<< (31-kRingBuffersIndexRwResource);
				 ringBufferActiveSize[kRingBuffersIndexRwResource] = 16 - __lzcnt16(stageInfo->activeRwResource);
			 }
			 break;
		 case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			 {
				 SCE_GNM_VALIDATE(m_globalTableAddr, "Global table pointer not specified. Call setGlobalResourceTableAddr() first!");
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, m_globalTableAddr);
			 }
			 break;

		 case Gnm::kShaderInputUsagePtrExtendedUserData:
			 {
				 if ( usesSrt )
				 {
					 m_dcb->setPointerInUserData(currentStage, destUsgpr, stageInfo->internalSrtBuffer);
				 }
				 else if ( newEud )
				 {
					 SCE_GNM_VALIDATE(newEud, "Invalid pointer.\n");
					 m_dcb->setPointerInUserData(currentStage, destUsgpr, newEud);
				 }
			 }
			 break;

		 case Gnm::kShaderInputUsagePtrIndirectResourceTable:
		 case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
		 case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
		 case Gnm::kShaderInputUsagePtrDispatchDraw:
			SCE_GNM_ERROR("Not yet implemented!\n");
			break;
		 default:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
		}

		iUsageSlot++;
	}

	// EUD Population:
	if ( newEud )
	{
		while ( iUsageSlot < inputUsageTableSize)
		{
			const uint32_t destEudNdx = inputUsageTable[iUsageSlot].m_startRegister - 16;
			const uint32_t srcApiSlot = inputUsageTable[iUsageSlot].m_apiSlot;
			const uint32_t regCount   = (inputUsageTable[iUsageSlot].m_registerCount+1) * 4; // 1 -> 8 DW; 0 -> 4 DW

			switch ( inputUsageTable[iUsageSlot].m_usageType )
			{
			 case Gnm::kShaderInputUsageImmResource:
				 {
					 const __int128_t *dataPtr;
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 if ( stageInfo->activeResource & (((__int128_t)1)<<srcApiSlot) )
					 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
						 const uint32_t		 chunkId   = srcApiSlot / kResourceChunkSize;
						 const uint32_t		 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kResourceChunkSizeMask);
						 const uint32_t		 usedCur   = stageInfo->resourceStage[chunkId].curSlots & (1 << (15-chunkSlot));
						 const __int128_t	*chunk	   = usedCur ? stageInfo->resourceStage[chunkId].curChunk : stageInfo->resourceStage[chunkId].usedChunk;
						 dataPtr   = chunk+chunkSlot*ConstantUpdateEngineHelper::kResourceSizeInDqWord;
						 SCE_GNM_VALIDATE(stageInfo->activeResource & (((__int128_t)1)<<srcApiSlot),
										  "The shader requires a resource in slot %i which hasn't been set.\n"
										  "If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 }
					 else
					 {
						 dataPtr = (const __int128_t *)nullData;
					 }
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

					 __int128_t *eud128 = (__int128_t *)(newEud + destEudNdx);
					 eud128[0] = dataPtr[0];
					 if ( regCount == 8 )
						 eud128[1] = dataPtr[1];

					 stageInfo->eudResourceSet |= (((__int128)1) << srcApiSlot);
				 }
				 break;
			 case Gnm::kShaderInputUsageImmSampler:
				 {
					 const __int128_t *dataPtr;
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 if ( stageInfo->activeSampler & (1<<srcApiSlot) )
					 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
						 const uint32_t		 chunkId   = srcApiSlot / kSamplerChunkSize;
						 const uint32_t		 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kSamplerChunkSizeMask);
						 const uint32_t		 usedCur   = stageInfo->samplerStage[chunkId].curSlots & (1 << (15-chunkSlot));
						 const __int128_t	*chunk	   = usedCur ? stageInfo->samplerStage[chunkId].curChunk : stageInfo->samplerStage[chunkId].usedChunk;
						 dataPtr   = chunk+chunkSlot*ConstantUpdateEngineHelper::kSamplerSizeInDqWord;
						 SCE_GNM_VALIDATE(stageInfo->activeSampler & (1<<srcApiSlot),
										  "The shader requires a sampler in slot %i which hasn't been set.\n"
										  "If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 }
					 else
					 {
						 dataPtr = (const __int128_t *)nullData;
					 }
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

					 __int128_t *eud128 = (__int128_t *)(newEud + destEudNdx);
					 eud128[0] = dataPtr[0];

					 stageInfo->eudSamplerSet |= (1 << srcApiSlot);
				 }
				 break;
			 case Gnm::kShaderInputUsageImmConstBuffer:
				 {
					 const __int128_t *dataPtr;
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 if ( stageInfo->activeConstantBuffer & (1<<srcApiSlot) )
					 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
						 const uint32_t		 chunkId   = srcApiSlot / kConstantBufferChunkSize;
						 const uint32_t		 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kConstantBufferChunkSizeMask);
						 const uint32_t		 usedCur   = stageInfo->constantBufferStage[chunkId].curSlots & (1 << (15-chunkSlot));
						 const __int128_t	*chunk	   = usedCur ? stageInfo->constantBufferStage[chunkId].curChunk : stageInfo->constantBufferStage[chunkId].usedChunk;
						 dataPtr   = chunk+chunkSlot*ConstantUpdateEngineHelper::kConstantBufferSizeInDqWord;
						 SCE_GNM_VALIDATE(stageInfo->activeConstantBuffer & (1<<srcApiSlot),
										  "The shader requires a constant buffer in slot %i which hasn't been set.\n"
										  "If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 }
					 else
					 {
						 dataPtr = (const __int128_t *)nullData;
					 }
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

					 __int128_t *eud128 = (__int128_t *)(newEud + destEudNdx);
					 eud128[0] = dataPtr[0];

					 stageInfo->eudConstantBufferSet |= (1 << srcApiSlot);
				 }
				 break;
			 case Gnm::kShaderInputUsageImmVertexBuffer:
				SCE_GNM_ERROR("Not yet implemented!\n");
				break;
			 case Gnm::kShaderInputUsageImmRwResource:
				 {
					 const __int128_t *dataPtr;
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 if ( stageInfo->activeRwResource & (1<<srcApiSlot) )
					 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
						 const uint32_t		 chunkId   = srcApiSlot / kRwResourceChunkSize;
						 const uint32_t		 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kRwResourceChunkSizeMask);
						 const uint32_t		 usedCur   = stageInfo->rwResourceStage[chunkId].curSlots & (1 << (15-chunkSlot));
						 const __int128_t	*chunk	   = usedCur ? stageInfo->rwResourceStage[chunkId].curChunk : stageInfo->rwResourceStage[chunkId].usedChunk;
						 dataPtr   = chunk+chunkSlot*ConstantUpdateEngineHelper::kRwResourceSizeInDqWord;
						 SCE_GNM_VALIDATE(stageInfo->activeRwResource & (1<<srcApiSlot),
										  "The shader requires a rw resource in slot %i which hasn't been set.\n"
										  "If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					 }
					 else
					 {
						 dataPtr = (const __int128_t *)nullData;
					 }
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

					 __int128_t *eud128 = (__int128_t *)(newEud + destEudNdx);
					 eud128[0] = dataPtr[0];
					 if ( regCount == 8 )
						 eud128[1] = dataPtr[1];

					 stageInfo->eudRwResourceSet |= (1 << srcApiSlot);
				 }
				 break;
			 case Gnm::kShaderInputUsageImmAluFloatConst:
				SCE_GNM_ERROR("Not yet implemented!\n");
				break;
			 case Gnm::kShaderInputUsageImmAluBool32Const:
				newEud[destEudNdx] = stageInfo->boolValue;
				break;
			 case Gnm::kShaderInputUsageImmGdsCounterRange:
				{
					uint32_t *eud32 = (uint32_t *)(newEud + destEudNdx);
					*eud32 = stageInfo->appendConsumeDword;
				}
				break;
			 case Gnm::kShaderInputUsageImmGdsMemoryRange:
			 case Gnm::kShaderInputUsageImmGwsBase:
				SCE_GNM_ERROR("Not yet implemented!\n");
				break;
			 case Gnm::kShaderInputUsageImmShaderResourceTable:
				SCE_GNM_ERROR("non-SRT shaders should not be looking for Shader Resource Tables!");
				break;

			 case Gnm::kShaderInputUsageSubPtrFetchShader:
				SCE_GNM_ERROR("Fetch shader are expected to be set in usgpr 0 -- Please check input data.");
				break;

			 case Gnm::kShaderInputUsagePtrResourceTable:
				 {
					 if ( stageInfo->dirtyRing & (1 << (31-kRingBuffersIndexResource)) )
						 updateChunkState256(stageInfo->resourceStage, kResourceNumChunks);

					 // Need to mark the entire resource table as used.
					 stageInfo->eudResourceSet = stageInfo->activeResource;

					 void **eud64 = (void **)(newEud + destEudNdx);
					 (*eud64) = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexResource);

					 usedRingBuffer |= 1<< (31-kRingBuffersIndexResource);
					 ringBufferActiveSize[kRingBuffersIndexResource] = 128 - ConstantUpdateEngineHelper::lzcnt128(stageInfo->activeResource);
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrInternalResourceTable:
				SCE_GNM_ERROR("Not yet implemented!\n");
				break;
			 case Gnm::kShaderInputUsagePtrSamplerTable:
				 {
					 if ( stageInfo->dirtyRing & (1 << (31-kRingBuffersIndexSampler)) )
						 updateChunkState128(stageInfo->samplerStage, kSamplerNumChunks);

					 // Need to mark the entire resource table as used.
					 stageInfo->eudSamplerSet = stageInfo->activeSampler;

					 void **eud64 = (void **)(newEud + destEudNdx);
					 (*eud64) = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexSampler);

					 usedRingBuffer |= 1<< (31-kRingBuffersIndexSampler);
					 ringBufferActiveSize[kRingBuffersIndexSampler] = 16 - __lzcnt16(stageInfo->activeSampler);
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrConstBufferTable:
				 {
					 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexConstantBuffer)) )
						 updateChunkState128(stageInfo->constantBufferStage	, kConstantBufferNumChunks);

					 // Need to mark the entire resource table as used.
					 stageInfo->eudConstantBufferSet = stageInfo->activeConstantBuffer;

					 void **eud64 = (void **)(newEud + destEudNdx);
					 (*eud64) = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexConstantBuffer);

					 usedRingBuffer |= 1<< (31-kRingBuffersIndexConstantBuffer);
					 ringBufferActiveSize[kRingBuffersIndexConstantBuffer] = 32 - __lzcnt32(stageInfo->activeConstantBuffer);
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrVertexBufferTable:
				 {
					 void **eud64 = (void **)(newEud + destEudNdx);
					 ringBufferActiveSize[kRingBuffersIndexVertexBuffer] = 32 - __lzcnt32(stageInfo->activeVertexBuffer);

					 if ( ringBufferActiveSize[kRingBuffersIndexVertexBuffer] <= 8 )
					 {
						 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexVertexBuffer)) )
							 updateChunkState128(stageInfo->vertexBufferStage, 1);

						 SCE_GNM_VALIDATE(stageInfo->vertexBufferStage[0].curChunk, "Invalid pointer.\n");
						 (*eud64) = stageInfo->vertexBufferStage[0].curChunk;
					 }
					 else
					 {
						 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexVertexBuffer)) )
							 updateChunkState128(stageInfo->vertexBufferStage, kVertexBufferNumChunks);

						 (*eud64) = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexVertexBuffer);
						 usedRingBuffer |= 1<< (31-kRingBuffersIndexVertexBuffer);
					 }
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrSoBufferTable:
				 {
					 void **eud64 = (void **)(newEud + destEudNdx);
					 updateChunkState128(&m_streamoutBufferStage, 1);
					 (*eud64) = m_streamoutBufferStage.curChunk;
					 m_eudReferencesStreamoutBuffers = true;
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrRwResourceTable:
				 {
					 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexRwResource)) )
						 updateChunkState256(stageInfo->rwResourceStage, kRwResourceNumChunks);

					 // Need to mark the entire resource table as used.
					 stageInfo->eudRwResourceSet = stageInfo->activeRwResource;

					 void **eud64 = (void **)(newEud + destEudNdx);
					 (*eud64) = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexRwResource);

					 usedRingBuffer |= 1<< (31-kRingBuffersIndexRwResource);
					 ringBufferActiveSize[kRingBuffersIndexRwResource] = 16 - __lzcnt16(stageInfo->activeRwResource);
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrInternalGlobalTable:
				 {
					 SCE_GNM_VALIDATE(m_globalTableAddr, "Global table pointer not specified. Call setGlobalResourceTableAddr() first!");
					 void **eud64 = (void **)(newEud + destEudNdx);
					 (*eud64) = m_globalTableAddr;
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrExtendedUserData:
				SCE_GNM_ERROR("EUD withing EUD is not supported.");
				break;
			 case Gnm::kShaderInputUsagePtrIndirectResourceTable:
			 case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
			 case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
			 case Gnm::kShaderInputUsagePtrDispatchDraw:
				SCE_GNM_ERROR("Not yet implemented!\n");
				break;
			 default:
				SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
			}
			iUsageSlot++;
		}
	}

	// Update the ring buffer as needed:
	const uint32_t cpRamStageOffset = currentStage*ConstantUpdateEngineHelper::kPerStageDwordSize;
	uint32_t ringToUpdate = usedRingBuffer & stageInfo->dirtyRing;

	if ( ringToUpdate )
	{
		while ( ringToUpdate )
		{
			const uint32_t ndx = __lzcnt32(ringToUpdate); // 0 represent the high bit (0x8000 0000)
			const uint32_t bit = 1 << (31 - ndx);
			const uint32_t clr = ~bit;

			//

			const uint32_t numResourcesToCopy = ringBufferActiveSize[ndx];

			const uint16_t cpRamOffset = cpRamStageOffset + ConstantUpdateEngineHelper::ringBufferOffsetPerStageInCpRam[ndx];
			const uint16_t sizeInDw    = ConstantUpdateEngineHelper::slotSizeInDword[ndx] * numResourcesToCopy;
			void           *ringAddr   = ConstantUpdateEngineHelper::getRingBuffersNextHead(&stageInfo->ringBuffers[ndx]);

			m_ccb->cpRamDump(ringAddr, cpRamOffset*4, sizeInDw);
			const bool hasWrapped = ConstantUpdateEngineHelper::advanceRingBuffersHead(&stageInfo->ringBuffers[ndx]);
			m_anyWrapped = m_anyWrapped || hasWrapped;

			//

			stageInfo->dirtyRing &= clr;
			ringToUpdate &= clr;
		}
		m_usingCcb = true;
	}
}


void ConstantUpdateEngine::preDraw()
{
	const uint32_t esgs_en = (m_activeShaderStages>>5)&1;
	const uint32_t lshs_en = (m_activeShaderStages&1);
	SCE_GNM_VALIDATE(m_activeShaderStages != kActiveShaderStagesDispatchDrawVsPs, "setActiveShaderStages must be VsPs, EsGsVsPs, LsHsVsPs, or LsHsEsGsVsPs");

	// These sync is needed only when one of the ring buffers crosses a sync point (either 1/2 or last chunk), if this has not happened it's going to be replaced with a NOP

	m_usingCcb = false;

	m_ccb->waitOnDeCounterDiff(m_numRingEntries/4);
	ConstantCommandBuffer savedCcb = *m_ccb;
	savedCcb.m_cmdptr -= kWaitOnDeCounterDiffSizeInDword;

	// Update PS usage table, generating it from scratch if the user didn't pass one in.
	if (m_currentPSB != NULL && m_dirtyVsOrPs && m_currentPSB->m_numInputSemantics != 0)
	{
		if ( m_psInputs )
		{
			m_dcb->setPsShaderUsage(m_psInputs, m_currentPSB->m_numInputSemantics);
		}
		else
		{
			uint32_t psInputs[32];
			Gnm::generatePsShaderUsageTable(psInputs,
											m_currentVSB->getExportSemanticTable(), m_currentVSB->m_numExportSemantics,
											m_currentPSB->getPixelInputSemanticTable(), m_currentPSB->m_numInputSemantics);
			m_dcb->setPsShaderUsage(psInputs, m_currentPSB->m_numInputSemantics);
		}
		m_dirtyVsOrPs = false;
	}


	Gnm::ShaderStage currentStage = Gnm::kShaderStagePs;

	do
	{
		if ( m_stageInfo[currentStage].inputUsageTableSize )
		{
			m_dcb->prefetchIntoL2((void*)((uintptr_t)m_stageInfo[currentStage].shaderBaseAddr256<<8),
								  m_stageInfo[currentStage].shaderCodeSizeInBytes);
			applyInputUsageData(currentStage);
		}

		// Next active stage:
		currentStage++;
		if ( currentStage == Gnm::kShaderStageGs && !esgs_en )
			currentStage = Gnm::kShaderStageHs; // Skip Gs/Es
		if ( currentStage == Gnm::kShaderStageHs && !lshs_en )
			currentStage = Gnm::kShaderStageCount; // Skip Ls/Hs

	} while ( currentStage < kShaderStageCount );

	// Check if we need to update the global table:
	if ( m_globalTableNeedsUpdate )
	{
		SCE_GNM_VALIDATE(m_globalTableAddr, "Global table pointer not specified. Call setGlobalResourceTableAddr() first!");
		m_dcb->writeDataInlineThroughL2(m_globalTableAddr, m_globalTablePtr, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE/4, Gnm::kCachePolicyLru, Gnm::kWriteDataConfirmEnable);
		// Invalidate KCache:
		m_dcb->flushShaderCachesAndWait(kCacheActionNone,kExtendedCacheActionInvalidateKCache, kStallCommandBufferParserDisable);
		m_globalTableNeedsUpdate = false;
	}

	if ( m_usingCcb )
	{
		m_ccb->incrementCeCounter();
		m_dcb->waitOnCe();
	}

	if(!m_anyWrapped)
	{
		savedCcb.insertNop(kWaitOnDeCounterDiffSizeInDword);
	}
	m_anyWrapped = false;
}


void ConstantUpdateEngine::postDraw()
{
	if ( m_usingCcb )
	{
		// Inform the constant engine that this draw has ended so that the constant engine can reuse
		// the constant memory allocated to this draw call
		m_dcb->incrementDeCounter();
		m_usingCcb = false;
	}
}


void ConstantUpdateEngine::preDispatch()
{
	m_ccb->waitOnDeCounterDiff(m_numRingEntries/4);
	ConstantCommandBuffer savedCcb = *m_ccb;
	savedCcb.m_cmdptr -= kWaitOnDeCounterDiffSizeInDword;

	if ( m_stageInfo[kShaderStageCs].inputUsageTableSize )
	{
		m_dcb->prefetchIntoL2((void*)((uintptr_t)m_stageInfo[kShaderStageCs].shaderBaseAddr256<<8),
							  m_stageInfo[kShaderStageCs].shaderCodeSizeInBytes);

		applyInputUsageData(kShaderStageCs);
	}

	// Check if we need to update the global table:
	if ( m_globalTableNeedsUpdate )
	{
		SCE_GNM_VALIDATE(m_globalTableAddr, "Global table pointer not specified. Call setGlobalResourceTableAddr() first!");
		m_dcb->writeDataInlineThroughL2(m_globalTableAddr, m_globalTablePtr, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE/4, Gnm::kCachePolicyLru, Gnm::kWriteDataConfirmEnable);
		// Invalidate KCache:
		m_dcb->flushShaderCachesAndWait(kCacheActionNone,kExtendedCacheActionInvalidateKCache, kStallCommandBufferParserDisable);
		m_globalTableNeedsUpdate = false;
	}

	if ( m_usingCcb )
	{
		m_ccb->incrementCeCounter();
		m_dcb->waitOnCe();
	}

	if(!m_anyWrapped)
	{
		savedCcb.insertNop(kWaitOnDeCounterDiffSizeInDword);
	}

	m_anyWrapped = false;
}


void ConstantUpdateEngine::postDispatch()
{
	if ( m_usingCcb )
	{
		// Inform the constant engine that this dispatch has ended so that the constant engine can reuse
		// the constant memory allocated to this dispatch
		m_dcb->incrementDeCounter();
		m_usingCcb = false;
	}
}


void ConstantUpdateEngine::advanceFrame(void)
{
	m_anyWrapped = true;

	for (uint32_t iStage = 0; iStage < kShaderStageCount; ++iStage)
	{
		for (uint32_t iResource = 0; iResource < kResourceNumChunks; ++iResource)
		{
			m_stageInfo[iStage].resourceStage[iResource].usedChunk = 0;
			m_stageInfo[iStage].resourceStage[iResource].curChunk  = 0;
			m_stageInfo[iStage].resourceStage[iResource].curSlots  = 0;
			m_stageInfo[iStage].resourceStage[iResource].usedSlots = 0;
		}

		for (uint32_t iSampler = 0; iSampler < kSamplerNumChunks; ++iSampler)
		{
			m_stageInfo[iStage].samplerStage[iSampler].usedChunk = 0;
			m_stageInfo[iStage].samplerStage[iSampler].curChunk	 = 0;
			m_stageInfo[iStage].samplerStage[iSampler].curSlots	 = 0;
			m_stageInfo[iStage].samplerStage[iSampler].usedSlots = 0;
		}

		for (uint32_t iConstantBuffer = 0; iConstantBuffer < kConstantBufferNumChunks; ++iConstantBuffer)
		{
			m_stageInfo[iStage].constantBufferStage[iConstantBuffer].usedChunk = 0;
			m_stageInfo[iStage].constantBufferStage[iConstantBuffer].curChunk  = 0;
			m_stageInfo[iStage].constantBufferStage[iConstantBuffer].curSlots  = 0;
			m_stageInfo[iStage].constantBufferStage[iConstantBuffer].usedSlots = 0;
		}

		for (uint32_t iVertexBuffer = 0; iVertexBuffer < kVertexBufferNumChunks; ++iVertexBuffer)
		{
			m_stageInfo[iStage].vertexBufferStage[iVertexBuffer].usedChunk = 0;
			m_stageInfo[iStage].vertexBufferStage[iVertexBuffer].curChunk  = 0;
			m_stageInfo[iStage].vertexBufferStage[iVertexBuffer].curSlots  = 0;
			m_stageInfo[iStage].vertexBufferStage[iVertexBuffer].usedSlots = 0;
		}

		for (uint32_t iRwResource = 0; iRwResource < kRwResourceNumChunks; ++iRwResource)
		{
			m_stageInfo[iStage].rwResourceStage[iRwResource].usedChunk = 0;
			m_stageInfo[iStage].rwResourceStage[iRwResource].curChunk  = 0;
			m_stageInfo[iStage].rwResourceStage[iRwResource].curSlots  = 0;
			m_stageInfo[iStage].rwResourceStage[iRwResource].usedSlots = 0;
		}

		m_stageInfo[iStage].eudResourceSet		 = 0;
		m_stageInfo[iStage].eudSamplerSet		 = 0;
		m_stageInfo[iStage].eudConstantBufferSet = 0;
		m_stageInfo[iStage].eudRwResourceSet	 = 0;

		m_stageInfo[iStage].activeResource		 = 0;
		m_stageInfo[iStage].activeSampler		 = 0;
		m_stageInfo[iStage].activeConstantBuffer = 0;
		m_stageInfo[iStage].activeVertexBuffer	 = 0;
		m_stageInfo[iStage].activeRwResource	 = 0;

		m_stageInfo[iStage].internalSrtBuffer		  = 0;
		m_stageInfo[iStage].userSrtBufferSizeInDwords = 0;

		m_stageInfo[iStage].inputUsageTableSize = 0;
		m_stageInfo[iStage].eudSizeInDWord		= 0;
		m_stageInfo[iStage].dirtyRing			= 0;
		m_stageInfo[iStage].appendConsumeDword	= 0;

		for (uint32_t iRing = 0; iRing < kNumRingBuffersPerStage; ++iRing)
		{
			ConstantUpdateEngineHelper::updateRingBuffersStatePostSubmission(&m_stageInfo[iStage].ringBuffers[iRing]);
		}
	}

	m_streamoutBufferStage.usedChunk = 0;
	m_streamoutBufferStage.curChunk  = 0;
	m_streamoutBufferStage.curSlots  = 0;
	m_streamoutBufferStage.usedSlots = 0;
	m_eudReferencesStreamoutBuffers	 = false;

	m_activeShaderStages = Gnm::kActiveShaderStagesVsPs;
	m_shaderUsesSrt		 = 0;
	m_anyWrapped		 = true;
	m_dirtyVsOrPs		 = false;
	m_psInputs			 = NULL;
}


#endif // CUE_V2
