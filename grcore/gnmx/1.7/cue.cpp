/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/common.h"
#ifdef SCE_GNMX_ENABLE_CUE_V2

#include <cstring>
#ifdef __ORBIS__
#include <x86intrin.h>
#else //__ORBIS__
#include <intrin.h>
#endif //__ORBIS__
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

SCE_GNM_STATIC_ASSERT((sizeof(ConstantUpdateEngine) & 0xf) == 0);

ConstantUpdateEngine::ConstantUpdateEngine(void)
{
}


ConstantUpdateEngine::~ConstantUpdateEngine(void)
{
}


void ConstantUpdateEngine::init(void *heapAddr, uint32_t numRingEntries)
{
	RingSetup ringSetup;
	ringSetup.numResourceSlots	   = Gnm::kSlotCountResource;
	ringSetup.numRwResourceSlots   = Gnm::kSlotCountRwResource;
	ringSetup.numSampleSlots	   = Gnm::kSlotCountSampler;
	ringSetup.numVertexBufferSlots = Gnm::kSlotCountVertexBuffer;

	init(heapAddr, numRingEntries, ringSetup);
}


void ConstantUpdateEngine::init(void *heapAddr, uint32_t numRingEntries, RingSetup ringSetup)
{
	SCE_GNM_VALIDATE(heapAddr != NULL, "heapAddr must not be NULL.");
	SCE_GNM_VALIDATE((uintptr_t(heapAddr) & 0x3) == 0, "heapAddr (0x%p) must be aligned to a 4-byte boundary.", heapAddr);
	SCE_GNM_VALIDATE(numRingEntries>=4, "If hardware kcache invalidation is enabled, numRingEntries (%d) must be at least 4.", numRingEntries);

	m_ringSetup = ringSetup;
	ConstantUpdateEngineHelper::validateRingSetup(&m_ringSetup);

	m_dcb					= 0;
	m_ccb					= 0;
	m_activeShaderStages	= Gnm::kActiveShaderStagesVsPs;
	m_numRingEntries		= numRingEntries;
	m_shaderUsesSrt			= 0;
	m_anyWrapped			= true;
	m_dirtyVsOrPs			= false;
	m_psInputs				= NULL;
	m_dirtyStage            = 0;

	m_currentVSB = NULL;
	m_currentPSB = NULL;
	m_currentLSB = NULL;
	m_currentESB = NULL;
	m_currentAcbCSB = NULL;

	m_prefetchShaderCode = false; // The L2 prefetch is currently disabled -- No need to insert unnecessary nops.

	memset(m_stageInfo, 0, sizeof(m_stageInfo));

	m_streamoutBufferStage.usedChunk = 0;
	m_streamoutBufferStage.curChunk  = 0;
	m_streamoutBufferStage.curSlots  = 0;
	m_streamoutBufferStage.usedSlots = 0;
	m_eudReferencesStreamoutBuffers	 = false;

	m_onChipEsVertsPerSubGroup        = 0;
	m_onChipEsExportVertexSizeInDword = 0;

	// Divvy up the heap between the ring buffers for each shader stage.
	// This is a duplicate of the body of ConstantUpdateEngine::computeHeapSize(). We don't
	// just call computeHeapSize() here because init() needs the intermediate k*RingBufferBytes values.
	const uint32_t kResourceRingBufferBytes	        = ConstantUpdateEngineHelper::computeShaderResourceRingSize(m_ringSetup.numResourceSlots        * Gnm::kDwordSizeResource,         numRingEntries);
	const uint32_t kRwResourceRingBufferBytes	    = ConstantUpdateEngineHelper::computeShaderResourceRingSize(m_ringSetup.numRwResourceSlots      * Gnm::kDwordSizeRwResource,       numRingEntries);
	const uint32_t kSamplerRingBufferBytes		    = ConstantUpdateEngineHelper::computeShaderResourceRingSize(m_ringSetup.numSampleSlots          * Gnm::kDwordSizeSampler,          numRingEntries);
	const uint32_t kVertexBufferRingBufferBytes     = ConstantUpdateEngineHelper::computeShaderResourceRingSize(m_ringSetup.numVertexBufferSlots    * Gnm::kDwordSizeVertexBuffer,     numRingEntries);
	const uint32_t kConstantBufferRingBufferBytes   = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kSlotCountConstantBuffer       * Gnm::kDwordSizeConstantBuffer,   numRingEntries);

	m_heapSize = computeHeapSize(numRingEntries, m_ringSetup);

	// Initialize ring buffers
	void *freeAddr = heapAddr;
	for(int iStage=0; iStage<Gnm::kShaderStageCount; ++iStage)
	{
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexResource],		 freeAddr, kResourceRingBufferBytes,		 m_ringSetup.numResourceSlots        * Gnm::kDwordSizeResource,         m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexRwResource],		 freeAddr, kRwResourceRingBufferBytes,		 m_ringSetup.numRwResourceSlots      * Gnm::kDwordSizeRwResource,       m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexSampler],			 freeAddr, kSamplerRingBufferBytes,			 m_ringSetup.numSampleSlots          * Gnm::kDwordSizeSampler,          m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexVertexBuffer],	 freeAddr, kVertexBufferRingBufferBytes,	 m_ringSetup.numVertexBufferSlots    * Gnm::kDwordSizeVertexBuffer,     m_numRingEntries);
		freeAddr = ConstantUpdateEngineHelper::initializeRingBuffer(&m_stageInfo[iStage].ringBuffers[kRingBuffersIndexConstantBuffer],	 freeAddr, kConstantBufferRingBufferBytes,	 Gnm::kSlotCountConstantBuffer       * Gnm::kDwordSizeConstantBuffer,   m_numRingEntries);
	}

	const uint32_t actualUsed = static_cast<uint32_t>(uintptr_t(freeAddr) - uintptr_t(heapAddr));
	SCE_GNM_ASSERT(actualUsed == m_heapSize); // sanity check
	SCE_GNM_UNUSED(actualUsed);

	memset(m_globalTablePtr, 0, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE);
	m_globalTableNeedsUpdate = false;

	m_dispatchDrawIndexDeallocNumBits = 0;
	m_dispatchDrawOrderedAppendMode = (Gnm::DispatchOrderedAppendMode)0;
}

//------


void ConstantUpdateEngine::setGlobalResourceTableAddr(void *addr)
{
	m_globalTableAddr = addr;
}


void ConstantUpdateEngine::setGlobalDescriptor(sce::Gnm::ShaderGlobalResourceType resType, const sce::Gnm::Buffer *res)
{
	((__int128_t*)m_globalTablePtr)[resType] = *(__int128_t*)res;
	m_globalTableNeedsUpdate = true;
}


//------


void ConstantUpdateEngine::applyInputUsageData(sce::Gnm::ShaderStage currentStage)
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
				 if ( ConstantUpdateEngineHelper::isBitSet(stageInfo->activeResource, srcApiSlot) )
				 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

					 const uint32_t			 chunkId   = srcApiSlot / kResourceChunkSize;
					 const uint32_t			 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kResourceChunkSizeMask);
					 const uint32_t			 usedCur   = stageInfo->resourceStage[chunkId].curSlots & (1 << (15-chunkSlot));
					 const __int128_t		*chunk	   = usedCur ? stageInfo->resourceStage[chunkId].curChunk : stageInfo->resourceStage[chunkId].usedChunk;
					 const uint32_t			*dataPtr   = (const uint32_t*)(chunk+chunkSlot*ConstantUpdateEngineHelper::kResourceSizeInDqWord);

					 SCE_GNM_VALIDATE(ConstantUpdateEngineHelper::isBitSet(stageInfo->activeResource, srcApiSlot),
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
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
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
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
			break;
		 case Gnm::kShaderInputUsageImmAluBool32Const:
			m_dcb->setUserData(currentStage, destUsgpr, stageInfo->boolValue);
			break;
		 case Gnm::kShaderInputUsageImmGdsCounterRange:
			m_dcb->setUserData(currentStage, destUsgpr, stageInfo->appendConsumeDword);
			break;
		 case Gnm::kShaderInputUsageImmGdsMemoryRange:
			m_dcb->setUserData(currentStage, destUsgpr, stageInfo->gdsMemoryRangeDword);
			break;
		 case Gnm::kShaderInputUsageImmGwsBase:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
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
		 case Gnm::kShaderInputUsageImmLdsEsGsSize:
			m_dcb->setUserData(currentStage, destUsgpr, m_onChipEsVertsPerSubGroup * m_onChipEsExportVertexSizeInDword);
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
				 ringBufferActiveSize[kRingBuffersIndexResource] = (uint16_t)(128 - ConstantUpdateEngineHelper::lzcnt128(stageInfo->activeResource));
			 }
			 break;
		 case Gnm::kShaderInputUsagePtrInternalResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
			break;
		 case Gnm::kShaderInputUsagePtrSamplerTable:
			 {
				 if ( stageInfo->dirtyRing & (1 << (31-kRingBuffersIndexSampler)) )
					updateChunkState128(stageInfo->samplerStage, kSamplerNumChunks);

				 void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexSampler);
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, ringBufferAddr);
				 usedRingBuffer |= 1<< (31-kRingBuffersIndexSampler);
				 ringBufferActiveSize[kRingBuffersIndexSampler] = (uint16_t)(16 - __lzcnt16(stageInfo->activeSampler));
			 }
			 break;
		 case Gnm::kShaderInputUsagePtrConstBufferTable:
			 {
				 if ( stageInfo->dirtyRing & (1<< (31-kRingBuffersIndexConstantBuffer)) )
					 updateChunkState128(stageInfo->constantBufferStage	, kConstantBufferNumChunks);

				 void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexConstantBuffer);
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, ringBufferAddr);
				 usedRingBuffer |= 1<< (31-kRingBuffersIndexConstantBuffer);
				 ringBufferActiveSize[kRingBuffersIndexConstantBuffer] = (uint16_t)(32 - __lzcnt32(stageInfo->activeConstantBuffer));
			 }
			 break;
		 case Gnm::kShaderInputUsagePtrVertexBufferTable:
			 {
				 ringBufferActiveSize[kRingBuffersIndexVertexBuffer] = (uint16_t)(32 - __lzcnt32(stageInfo->activeVertexBuffer));

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

		 case Gnm::kShaderInputUsageImmGdsKickRingBufferOffset:
		 case Gnm::kShaderInputUsageImmVertexRingBufferOffset:
			 //nothing to do
			 break;
		 case Gnm::kShaderInputUsagePtrDispatchDraw:
			 {
				 SCE_GNM_VALIDATE(m_pDispatchDrawData, "DispatchDrawData pointer not specified. Call setDispatchDrawData() first!");
				 m_dcb->setPointerInUserData(currentStage, destUsgpr, (void*)m_pDispatchDrawData);
			 }
			 break;
		 case Gnm::kShaderInputUsageImmDispatchDrawInstances:
			 //nothing to do
			 break;

		 case Gnm::kShaderInputUsagePtrIndirectResourceTable:
		 case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
		 case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
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
					 if ( ConstantUpdateEngineHelper::isBitSet(stageInfo->activeResource, srcApiSlot) )
					 {
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
						 const uint32_t		 chunkId   = srcApiSlot / kResourceChunkSize;
						 const uint32_t		 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kResourceChunkSizeMask);
						 const uint32_t		 usedCur   = stageInfo->resourceStage[chunkId].curSlots & (1 << (15-chunkSlot));
						 const __int128_t	*chunk	   = usedCur ? stageInfo->resourceStage[chunkId].curChunk : stageInfo->resourceStage[chunkId].usedChunk;
						 dataPtr   = chunk+chunkSlot*ConstantUpdateEngineHelper::kResourceSizeInDqWord;
						 SCE_GNM_VALIDATE(ConstantUpdateEngineHelper::isBitSet(stageInfo->activeResource, srcApiSlot),
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

					 stageInfo->eudResourceSet[srcApiSlot>>6] |= (((uint64_t)1) << (srcApiSlot & 63));
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
				SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
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
				SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
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
				{
					uint32_t *eud32 = (uint32_t *)(newEud + destEudNdx);
					*eud32 = stageInfo->gdsMemoryRangeDword;
				}
				break;
			 case Gnm::kShaderInputUsageImmGwsBase:
				SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
				break;
			 case Gnm::kShaderInputUsageImmShaderResourceTable:
				SCE_GNM_ERROR("non-SRT shaders should not be looking for Shader Resource Tables!");
				break;

			 case Gnm::kShaderInputUsageImmLdsEsGsSize:
				{
					uint32_t *eud32 = (uint32_t *)(newEud + destEudNdx);
					*eud32 = m_onChipEsVertsPerSubGroup * m_onChipEsExportVertexSizeInDword;
				}
				break;

			 case Gnm::kShaderInputUsageSubPtrFetchShader:
				SCE_GNM_ERROR("Fetch shader are expected to be set in usgpr 0 -- Please check input data.");
				break;

			 case Gnm::kShaderInputUsagePtrResourceTable:
				 {
					 if ( stageInfo->dirtyRing & (1 << (31-kRingBuffersIndexResource)) )
						 updateChunkState256(stageInfo->resourceStage, kResourceNumChunks);

					 // Need to mark the entire resource table as used.
					 stageInfo->eudResourceSet[0] = stageInfo->activeResource[0];
					 stageInfo->eudResourceSet[1] = stageInfo->activeResource[1];

					 void **eud64 = (void **)(newEud + destEudNdx);
					 (*eud64) = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexResource);

					 usedRingBuffer |= 1<< (31-kRingBuffersIndexResource);
					 ringBufferActiveSize[kRingBuffersIndexResource] = (uint16_t)(128 - ConstantUpdateEngineHelper::lzcnt128(stageInfo->activeResource));
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrInternalResourceTable:
				SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
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
					 ringBufferActiveSize[kRingBuffersIndexSampler] = (uint16_t)(16 - __lzcnt16(stageInfo->activeSampler));
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
					 ringBufferActiveSize[kRingBuffersIndexConstantBuffer] = (uint16_t)(32 - __lzcnt32(stageInfo->activeConstantBuffer));
				 }
				 break;
			 case Gnm::kShaderInputUsagePtrVertexBufferTable:
				 {
					 void **eud64 = (void **)(newEud + destEudNdx);
					 ringBufferActiveSize[kRingBuffersIndexVertexBuffer] = (uint16_t)(32 - __lzcnt32(stageInfo->activeVertexBuffer));

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
					 ringBufferActiveSize[kRingBuffersIndexRwResource] = (uint16_t)(16 - __lzcnt16(stageInfo->activeRwResource));
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
			 case Gnm::kShaderInputUsagePtrDispatchDraw:
				SCE_GNM_ERROR("PtrDispatchDraw withing EUD is not supported.");
				break;
			 case Gnm::kShaderInputUsagePtrIndirectResourceTable:
			 case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
			 case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
				SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
				break;
			 default:
				SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
			}
			iUsageSlot++;
		}
	}

	// Update the ring buffer as needed:
	const uint16_t cpRamStageOffset = (uint16_t)(currentStage*ConstantUpdateEngineHelper::kPerStageDwordSize);
	uint32_t ringToUpdate = usedRingBuffer & stageInfo->dirtyRing;

	if ( ringToUpdate )
	{
		while ( ringToUpdate )
		{
			const uint32_t ndx = __lzcnt32(ringToUpdate); // 0 represent the high bit (0x8000 0000)
			const uint32_t bit = 1 << (31 - ndx);
			const uint32_t clr = ~bit;

			//

			const uint16_t numResourcesToCopy = ringBufferActiveSize[ndx];

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

void ConstantUpdateEngine::applyInputUsageDataForDispatchDrawCompute()
{
	// dispatch draw asynchronous compute always shares data in rings or other memory with graphics VS stage:
	Gnm::ShaderStage currentStage = Gnm::kShaderStageVs;
	StageInfo * const		stageInfo		 = m_stageInfo+currentStage;
	const uint8_t			currentStageMask = 1 << currentStage;
	const uint32_t			usesSrtVs = m_shaderUsesSrt & currentStageMask;

	StageInfo * const		stageInfoCs		 = m_stageInfo+kShaderStageAsynchronousCompute;
	const Gnm::InputUsageSlot		*inputUsageTable     = stageInfoCs->inputUsageTable;
	const uint32_t					 inputUsageTableSize = stageInfoCs->inputUsageTableSize;
	const uint32_t					 usesSrt			 = m_shaderUsesSrt & (1 << kShaderStageAsynchronousCompute);

	SCE_GNM_VALIDATE( inputUsageTableSize, "This function should not be called if inputUsageTableSize is 0.");
	SCE_GNM_VALIDATE( stageInfoCs->eudSizeInDWord <= stageInfo->eudSizeInDWord, "Mismatch between dispatch draw asynchronous compute shader and VS shader EUD size" );
	void *newEud = stageInfo->eudAddr;

#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
	static const uint32_t nullData[8] = {};
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

	// Build the ring address tables:
//	uint16_t ringBufferActiveSize[kNumRingBuffersPerStage];

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
				if ( ConstantUpdateEngineHelper::isBitSet(stageInfo->activeResource, srcApiSlot) )
				{
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
					const uint32_t			 chunkId   = srcApiSlot / kResourceChunkSize;
					const uint32_t			 chunkSlot = srcApiSlot & (ConstantUpdateEngineHelper::kResourceChunkSizeMask);
					const uint32_t			 usedCur   = stageInfo->resourceStage[chunkId].curSlots & (1 << (15-chunkSlot));
					const __int128_t		*chunk	   = usedCur ? stageInfo->resourceStage[chunkId].curChunk : stageInfo->resourceStage[chunkId].usedChunk;
					const uint32_t			*dataPtr   = (const uint32_t*)(chunk+chunkSlot*ConstantUpdateEngineHelper::kResourceSizeInDqWord);

					SCE_GNM_VALIDATE(ConstantUpdateEngineHelper::isBitSet(stageInfo->activeResource, srcApiSlot),
						"The shader requires a resource in VS slot %i which hasn't been set.\n"
						"If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
					m_acb->setUserDataRegion(destUsgpr, dataPtr, regCount);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				}
				else
				{
					m_acb->setUserDataRegion(destUsgpr, nullData, regCount);
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
						"The shader requires a sampler in VS slot %i which hasn't been set.\n"
						"If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
					m_acb->setSsharpInUserData(destUsgpr, (Gnm::Sampler*)dataPtr);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				}
				else
				{
					m_acb->setSsharpInUserData(destUsgpr, (Gnm::Sampler*)nullData);
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
						"The shader requires a constant buffer in VS slot %i which hasn't been set.\n"
						"If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
					m_acb->setVsharpInUserData(destUsgpr, (Gnm::Buffer*)dataPtr);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				}
				else
				{
					m_acb->setVsharpInUserData(destUsgpr, (Gnm::Buffer*)nullData);
				}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			}
			break;
		case Gnm::kShaderInputUsageImmVertexBuffer:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
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
						"The shader requires a rw resource in VS slot %i which hasn't been set.\n"
						"If this is an expected behavior, please define: SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES.", srcApiSlot);
					m_acb->setUserDataRegion(destUsgpr, dataPtr, regCount);
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				}
				else
				{
					m_acb->setUserDataRegion(destUsgpr, nullData, regCount);
				}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			}
			break;
		case Gnm::kShaderInputUsageImmAluFloatConst:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
			break;
		case Gnm::kShaderInputUsageImmAluBool32Const:
			m_acb->setUserData(destUsgpr, stageInfo->boolValue);
			break;
		case Gnm::kShaderInputUsageImmGdsCounterRange:
			m_acb->setUserData(destUsgpr, stageInfo->appendConsumeDword);
			break;
		case Gnm::kShaderInputUsageImmGdsMemoryRange:
			m_acb->setUserData(destUsgpr, stageInfo->gdsMemoryRangeDword);
			break;
		case Gnm::kShaderInputUsageImmGwsBase:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
			break;
		case Gnm::kShaderInputUsageImmShaderResourceTable:
			{
				SCE_GNM_VALIDATE(usesSrt, "non-SRT shaders should not be looking for Shader Resource Tables!");
				const uint32_t dwCount = inputUsageTable[iUsageSlot].m_srtSizeInDWordMinusOne+1;
				SCE_GNM_VALIDATE(!usesSrt || usesSrtVs, "Mismatch between dispatch draw asynchronous compute shader and VS shader InputUsage table SRT usage");
				SCE_GNM_VALIDATE(dwCount <= stageInfo->userSrtBufferSizeInDwords, "SRT user data size mismatch: dispatch draw CS shader expected %d DWORDS, but caller provided %d to VS stage.", dwCount, stageInfoCs->userSrtBufferSizeInDwords);
				m_acb->setUserDataRegion(destUsgpr, stageInfo->userSrtBuffer, dwCount);
			}
			break;
		case Gnm::kShaderInputUsageImmLdsEsGsSize:
			SCE_GNM_ERROR("OnChip Gs is not supported by DispatchDraw!\n");
			break;
		case Gnm::kShaderInputUsageSubPtrFetchShader:
			SCE_GNM_VALIDATE(destUsgpr == 0, "Fetch shader are expected to be set in usgpr 0 -- Please check input data.");
			break;
		case Gnm::kShaderInputUsagePtrResourceTable:
			{
				//FIXME: should validate that CS doesn't use any chunks that VS does not?
				void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexResource);
				m_acb->setPointerInUserData(destUsgpr, ringBufferAddr);
			}
			break;
		case Gnm::kShaderInputUsagePtrInternalResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
			break;
		case Gnm::kShaderInputUsagePtrSamplerTable:
			{
				//FIXME: should validate that CS doesn't use any chunks that VS does not?
				void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexSampler);
				m_acb->setPointerInUserData(destUsgpr, ringBufferAddr);
			}
			break;
		case Gnm::kShaderInputUsagePtrConstBufferTable:
			{
				//FIXME: should validate that CS doesn't use any chunks that VS does not?
				void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexConstantBuffer);
				m_acb->setPointerInUserData(destUsgpr, ringBufferAddr);
			}
			break;
		case Gnm::kShaderInputUsagePtrVertexBufferTable:
			{
				//FIXME: should validate that stageInfo doesn't use any chunks that stageInfoVs does not?
				uint32_t ringBufferActiveSize_VertexBuffer = 32 - __lzcnt32(stageInfo->activeVertexBuffer);

				if ( ringBufferActiveSize_VertexBuffer <= 8 )
				{
					SCE_GNM_VALIDATE(stageInfo->vertexBufferStage[0].curChunk, "Invalid pointer.\n");
					m_acb->setPointerInUserData(destUsgpr, stageInfo->vertexBufferStage[0].curChunk);
				}
				else
				{
					void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexVertexBuffer);
					m_acb->setPointerInUserData(destUsgpr, ringBufferAddr);
				}
			}
			break;
		case Gnm::kShaderInputUsagePtrSoBufferTable:
			{
				//FIXME: should validate that CS doesn't use any chunks that VS does not?
				m_acb->setPointerInUserData(destUsgpr, m_streamoutBufferStage.curChunk);
			}
			break;
		case Gnm::kShaderInputUsagePtrRwResourceTable:
			{
				//FIXME: should validate that CS doesn't use any chunks that VS does not?
				void * const ringBufferAddr = ConstantUpdateEngineHelper::getRingAddress(stageInfo, kRingBuffersIndexRwResource);
				m_acb->setPointerInUserData(destUsgpr, ringBufferAddr);
			}
			break;
		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			{
				SCE_GNM_VALIDATE(m_globalTableAddr, "Global table pointer not specified. Call setGlobalResourceTableAddr() first!");
				m_acb->setPointerInUserData(destUsgpr, m_globalTableAddr);
			}
			break;

		case Gnm::kShaderInputUsagePtrExtendedUserData:
			{
				if ( usesSrt )
				{
					m_acb->setPointerInUserData(destUsgpr, stageInfo->internalSrtBuffer);
				}
				else if ( newEud )
				{
					SCE_GNM_VALIDATE(newEud, "Invalid pointer.\n");
					m_acb->setPointerInUserData(destUsgpr, newEud);
				}
			}
			break;

		case Gnm::kShaderInputUsageImmGdsKickRingBufferOffset:
		case Gnm::kShaderInputUsageImmVertexRingBufferOffset:
			//nothing to do
			break;
		case Gnm::kShaderInputUsagePtrDispatchDraw:
			{
				SCE_GNM_VALIDATE(m_pDispatchDrawData, "DispatchDrawData pointer not specified. Call setDispatchDrawData() first!");
				m_acb->setPointerInUserData(destUsgpr, (void*)m_pDispatchDrawData);
			}
			break;
		case Gnm::kShaderInputUsageImmDispatchDrawInstances:
			//nothing to do
			break;

		case Gnm::kShaderInputUsagePtrIndirectResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
			break;
		default:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", inputUsageTable[iUsageSlot].m_usageType); // not supported yet
		}

		iUsageSlot++;
	}

#if SCE_GNM_CUE2_VALIDATE_DISPATCH_DRAW_INPUT_USAGE_TABLE
	// EUD Validation:
	if ( newEud )
	{
		const Gnm::InputUsageSlot		*inputUsageTableVs     = stageInfo->inputUsageTable;
		const uint32_t					 inputUsageTableSizeVs = stageInfo->inputUsageTableSize;
		uint32_t iUsageSlotVs = 0;
		while ( iUsageSlotVs < inputUsageTableSizeVs && inputUsageTableVs[iUsageSlotVs].m_startRegister < 16 )
			++iUsageSlotVs;

		while ( iUsageSlot < inputUsageTableSize)
		{
			const uint32_t destEudNdx = inputUsageTable[iUsageSlot].m_startRegister - 16;
			const uint32_t srcApiSlot = inputUsageTable[iUsageSlot].m_apiSlot;
			const uint32_t regCount   = (inputUsageTable[iUsageSlot].m_registerCount+1) * 4; // 1 -> 8 DW; 0 -> 4 DW

			while ( iUsageSlotVs < inputUsageTableSizeVs && inputUsageTableVs[iUsageSlotVs].m_startRegister < 16 + destEudNdx )
				++iUsageSlotVs;
			SCE_GNM_VALIDATE( iUsageSlotVs < inputUsageTableSizeVs 
				&& inputUsageTable[iUsageSlot].m_startRegister == inputUsageTableVs[iUsageSlotVs].m_startRegister
				&& inputUsageTable[iUsageSlot].m_usageType == inputUsageTableVs[iUsageSlotVs].m_usageType
				&& inputUsageTable[iUsageSlot].m_apiSlot == inputUsageTableVs[iUsageSlotVs].m_apiSlot 
				&& inputUsageTable[iUsageSlot].m_registerCount == inputUsageTableVs[iUsageSlotVs].m_registerCount );
			iUsageSlot++;
		}
	}
#endif //#if SCE_GNM_CUE2_VALIDATE_DISPATCH_DRAW_INPUT_USAGE_TABLE
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
	uint32_t currentStageMask = (1<<currentStage);

	do
	{
		if ( (m_dirtyStage & currentStageMask) && m_stageInfo[currentStage].inputUsageTableSize )
		{
			if ( m_prefetchShaderCode )
			{
				m_dcb->prefetchIntoL2((void*)((uintptr_t)m_stageInfo[currentStage].shaderBaseAddr256<<8),
									  m_stageInfo[currentStage].shaderCodeSizeInBytes);
			}
			applyInputUsageData(currentStage);
		}

		m_dirtyStage = m_dirtyStage & ~currentStageMask;

		// Next active stage:
#ifdef __ORBIS__
		currentStage++;
#else // __ORBIS__
		currentStage = (Gnm::ShaderStage)(currentStage + 1);
#endif // __ORBIS__
		currentStageMask <<= 1;

		if ( currentStage == Gnm::kShaderStageGs && !esgs_en )
		{
			currentStage = Gnm::kShaderStageHs; // Skip Gs/Es
			currentStageMask = (1<<currentStage);
		}
		if ( currentStage == Gnm::kShaderStageHs && !lshs_en )
		{
			currentStage = Gnm::kShaderStageCount; // Skip Ls/Hs
			currentStageMask = (1<<currentStage);
		}

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

	if ( (m_dirtyStage & (1<<kShaderStageCs)) &&  m_stageInfo[kShaderStageCs].inputUsageTableSize )
	{
		if ( m_prefetchShaderCode )
		{
			m_dcb->prefetchIntoL2((void*)((uintptr_t)m_stageInfo[kShaderStageCs].shaderBaseAddr256<<8),
								  m_stageInfo[kShaderStageCs].shaderCodeSizeInBytes);
		}

		applyInputUsageData(kShaderStageCs);
	}

	m_dirtyStage = m_dirtyStage & ~(1<<sce::Gnm::kShaderStageCs);

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
		for (uint32_t iRing = 0; iRing < kNumRingBuffersPerStage; ++iRing)
		{
			ConstantUpdateEngineHelper::updateRingBuffersStatePostSubmission(&m_stageInfo[iStage].ringBuffers[iRing]);
		}
	}

	invalidateAllBindings();
}

void ConstantUpdateEngine::invalidateAllBindings(void)
{
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

		m_stageInfo[iStage].eudResourceSet[0]	 = 0;
		m_stageInfo[iStage].eudResourceSet[1]	 = 0;
		m_stageInfo[iStage].eudSamplerSet		 = 0;
		m_stageInfo[iStage].eudConstantBufferSet = 0;
		m_stageInfo[iStage].eudRwResourceSet	 = 0;

		m_stageInfo[iStage].activeResource[0]	 = 0;
		m_stageInfo[iStage].activeResource[1]	 = 0;
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
	}

	m_streamoutBufferStage.usedChunk = 0;
	m_streamoutBufferStage.curChunk  = 0;
	m_streamoutBufferStage.curSlots  = 0;
	m_streamoutBufferStage.usedSlots = 0;
	m_eudReferencesStreamoutBuffers	 = false;

	m_onChipEsVertsPerSubGroup        = 0;
	m_onChipEsExportVertexSizeInDword = 0;

	m_activeShaderStages = Gnm::kActiveShaderStagesVsPs;
	m_shaderUsesSrt		 = 0;
	m_anyWrapped		 = true;
	m_dirtyVsOrPs		 = false;
	m_psInputs			 = NULL;
	m_dirtyStage         = 0;
	m_dispatchDrawIndexDeallocNumBits = 0;
	m_dispatchDrawOrderedAppendMode = (Gnm::DispatchOrderedAppendMode)0;
}



#endif // SCE_GNMX_ENABLE_CUE_V2
