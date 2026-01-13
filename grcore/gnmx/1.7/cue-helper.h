/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_CUE2_HELPER_H)
#define _SCE_GNMX_CUE2_HELPER_H

#ifdef __ORBIS__
#include <x86intrin.h>
#else //__ORBIS__
#include <intrin.h>
#endif //__ORBIS__
#include <gnm.h>
#include "grcore/gnmx/cue.h"


#define CUE2_SHOW_UNIMPLEMENTED 0

#ifndef __ORBIS__
#define __builtin_memset memset
#endif // __ORBIS__

namespace ConstantUpdateEngineHelper
{

	SCE_GNM_STATIC_ASSERT((sce::Gnmx::ConstantUpdateEngine::kResourceChunkSize & (sce::Gnmx::ConstantUpdateEngine::kResourceChunkSize-1)) == 0); // needs to be a power of 2
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kResourceChunkSize <= 16);
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kResourceChunkSize*sce::Gnmx::ConstantUpdateEngine::kResourceNumChunks == sce::Gnm::kSlotCountResource);
	SCE_GNM_STATIC_ASSERT((sce::Gnmx::ConstantUpdateEngine::kSamplerChunkSize & (sce::Gnmx::ConstantUpdateEngine::kSamplerChunkSize-1)) == 0); // needs to be a power of 2
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kSamplerChunkSize <= 16);
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kSamplerChunkSize*sce::Gnmx::ConstantUpdateEngine::kSamplerNumChunks == sce::Gnm::kSlotCountSampler);
	SCE_GNM_STATIC_ASSERT((sce::Gnmx::ConstantUpdateEngine::kConstantBufferChunkSize & (sce::Gnmx::ConstantUpdateEngine::kConstantBufferChunkSize-1)) == 0); // needs to be a power of 2
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kConstantBufferChunkSize <= 16);
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kConstantBufferChunkSize*sce::Gnmx::ConstantUpdateEngine::kConstantBufferNumChunks == (sce::Gnm::kSlotCountConstantBuffer+4)); // special case: count = 20.
	SCE_GNM_STATIC_ASSERT((sce::Gnmx::ConstantUpdateEngine::kVertexBufferChunkSize & (sce::Gnmx::ConstantUpdateEngine::kVertexBufferChunkSize-1)) == 0); // needs to be a power of 2
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kVertexBufferChunkSize <= 16);
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kVertexBufferChunkSize*sce::Gnmx::ConstantUpdateEngine::kVertexBufferNumChunks == (sce::Gnm::kSlotCountVertexBuffer));
	SCE_GNM_STATIC_ASSERT((sce::Gnmx::ConstantUpdateEngine::kRwResourceChunkSize & (sce::Gnmx::ConstantUpdateEngine::kRwResourceChunkSize-1)) == 0); // needs to be a power of 2
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kRwResourceChunkSize <= 16);
	SCE_GNM_STATIC_ASSERT(sce::Gnmx::ConstantUpdateEngine::kRwResourceChunkSize*sce::Gnmx::ConstantUpdateEngine::kRwResourceNumChunks == sce::Gnm::kSlotCountRwResource);
	SCE_GNM_STATIC_ASSERT(sce::Gnm::kSlotCountStreamoutBuffer <= 8);

	static const uint32_t		kResourceChunkSizeMask			= sce::Gnmx::ConstantUpdateEngine::kResourceChunkSize-1;
	static const uint32_t		kResourceChunkSizeInDWord		= sce::Gnm::kDwordSizeResource * sce::Gnmx::ConstantUpdateEngine::kResourceChunkSize;
	static const uint32_t		kResourceSizeInDqWord			= sce::Gnm::kDwordSizeResource / 4;
	static const uint32_t		kSamplerChunkSizeMask			= sce::Gnmx::ConstantUpdateEngine::kSamplerChunkSize-1;
	static const uint32_t		kSamplerChunkSizeInDWord		= sce::Gnm::kDwordSizeSampler * sce::Gnmx::ConstantUpdateEngine::kSamplerChunkSize;
	static const uint32_t		kSamplerSizeInDqWord			= sce::Gnm::kDwordSizeSampler / 4;
	static const uint32_t		kConstantBufferChunkSizeMask	= sce::Gnmx::ConstantUpdateEngine::kConstantBufferChunkSize-1;
	static const uint32_t		kConstantBufferChunkSizeInDWord = sce::Gnm::kDwordSizeConstantBuffer * sce::Gnmx::ConstantUpdateEngine::kConstantBufferChunkSize;
	static const uint32_t		kConstantBufferSizeInDqWord		= sce::Gnm::kDwordSizeConstantBuffer / 4;
	static const uint32_t		kVertexBufferChunkSizeMask		= sce::Gnmx::ConstantUpdateEngine::kVertexBufferChunkSize-1;
	static const uint32_t		kVertexBufferChunkSizeInDWord	= sce::Gnm::kDwordSizeVertexBuffer * sce::Gnmx::ConstantUpdateEngine::kVertexBufferChunkSize;
	static const uint32_t		kVertexBufferSizeInDqWord		= sce::Gnm::kDwordSizeVertexBuffer / 4;
	static const uint32_t		kRwResourceChunkSizeMask		= sce::Gnmx::ConstantUpdateEngine::kRwResourceChunkSize-1;
	static const uint32_t		kRwResourceChunkSizeInDWord		= sce::Gnm::kDwordSizeRwResource * sce::Gnmx::ConstantUpdateEngine::kRwResourceChunkSize;
	static const uint32_t		kRwResourceSizeInDqWord			= sce::Gnm::kDwordSizeRwResource / 4;

	static const uint32_t		kStreamoutChunkSizeInBytes		= sce::Gnm::kDwordSizeStreamoutBuffer * sce::Gnm::kSlotCountStreamoutBuffer * 4;
	static const uint32_t		kStreamoutBufferSizeInDqWord	= sce::Gnm::kDwordSizeStreamoutBuffer / 4;


	// Offset into each type's range of data within a shader stage, in DWORDs.
	static const uint32_t kDwordOffsetResource	       = 0;
	static const uint32_t kDwordOffsetRwResource       = kDwordOffsetResource                     + sce::Gnm::kSlotCountResource                  * sce::Gnm::kDwordSizeResource;
	static const uint32_t kDwordOffsetSampler		   = kDwordOffsetRwResource                   + sce::Gnm::kSlotCountRwResource                * sce::Gnm::kDwordSizeRwResource;
	static const uint32_t kDwordOffsetVertexBuffer     = kDwordOffsetSampler                      + sce::Gnm::kSlotCountSampler                   * sce::Gnm::kDwordSizeSampler;
	static const uint32_t kDwordOffsetConstantBuffer   = kDwordOffsetVertexBuffer                 + sce::Gnm::kSlotCountVertexBuffer              * sce::Gnm::kDwordSizeVertexBuffer;
	static const uint32_t kPerStageDwordSize		   = kDwordOffsetConstantBuffer               + sce::Gnm::kSlotCountConstantBuffer            * sce::Gnm::kDwordSizeConstantBuffer;

//~	static const uint32_t kDwordOffsetBoolConstant     = kDwordOffsetConstantBuffer               + sce::Gnm::kSlotCountConstantBuffer            * sce::Gnm::kDwordSizeConstantBuffer;
//~	static const uint32_t kDwordOffsetFloatConstant	   = kDwordOffsetBoolConstant                 + sce::Gnm::kSlotCountBoolConstant              * sce::Gnm::kDwordSizeBoolConstant;
//~	static const uint32_t kDwordOffsetAppendConsumeGdsCounterRange = kDwordOffsetFloatConstant    + sce::Gnm::kSlotCountFloatConstant             * sce::Gnm::kDwordSizeFloatConstant;
//~	static const uint32_t kDwordOffsetStreamoutBuffer  = kDwordOffsetAppendConsumeGdsCounterRange + sce::Gnm::kSlotCountAppendConsumeCounterRange * sce::Gnm::kDwordSizeAppendConsumeCounterRange;
//~	static const uint32_t kDwordOffsetExtendedUserData = kDwordOffsetStreamoutBuffer              + sce::Gnm::kSlotCountStreamoutBuffer           * sce::Gnm::kDwordSizeStreamoutBuffer;
//~	static const uint32_t kDwordOffsetDispatchDrawData = kDwordOffsetExtendedUserData             + sce::Gnm::kSlotCountExtendedUserData          * sce::Gnm::kDwordSizeExtendedUserData;
//~	static const uint32_t kPerStageDwordSize		   = kDwordOffsetDispatchDrawData             + sce::Gnm::kSlotCountDispatchDrawData          * sce::Gnm::kDwordSizeDispatchDrawData;


	static constexpr const uint8_t inputSizeInDWords[] =
	{
		8, // kShaderInputUsageImmResource
		4, // kShaderInputUsageImmSampler
		4, // kShaderInputUsageImmConstBuffer
		4, // kShaderInputUsageImmVertexBuffer
		8, // kShaderInputUsageImmRwResource
		1, // kShaderInputUsageImmAluFloatConst
		1, // kShaderInputUsageImmAluBool32Const
		1, // kShaderInputUsageImmGdsCounterRange
		1, // kShaderInputUsageImmGdsMemoryRange
		1, // kShaderInputUsageImmGwsBase
		2, // kShaderInputUsageImmShaderResourceTable
		0, //
		0, //
		1, // kShaderInputUsageImmLdsEsGsSize
		0, //
		0, //
		0, //
		0, //
		2, // kShaderInputUsageSubPtrFetchShader
		2, // kShaderInputUsagePtrResourceTable
		2, // kShaderInputUsagePtrInternalResourceTable
		2, // kShaderInputUsagePtrSamplerTable
		2, // kShaderInputUsagePtrConstBufferTable
		2, // kShaderInputUsagePtrVertexBufferTable
		2, // kShaderInputUsagePtrSoBufferTable
		2, // kShaderInputUsagePtrRwResourceTable
		2, // kShaderInputUsagePtrInternalGlobalTable
		2, // kShaderInputUsagePtrExtendedUserData
		2, // kShaderInputUsagePtrIndirectResourceTable
		2, // kShaderInputUsagePtrIndirectInternalResourceTable
		2, // kShaderInputUsagePtrIndirectRwResourceTable
		0, //
		0, //
		0, //
		1, // kShaderInputUsageImmGdsKickRingBufferOffset
		1, // kShaderInputUsageImmVertexRingBufferOffset
		2, // kShaderInputUsagePtrDispatchDraw
		1, // kShaderInputUsageImmDispatchDrawInstances
	};

	static constexpr const uint16_t ringBufferOffsetPerStageInCpRam[] =
	{
		kDwordOffsetResource,
		kDwordOffsetRwResource,
		kDwordOffsetSampler,
		kDwordOffsetVertexBuffer,
		kDwordOffsetConstantBuffer,
	};

	static constexpr const uint16_t slotSizeInDword[] =
	{
		sce::Gnm::kDwordSizeResource,
		sce::Gnm::kDwordSizeRwResource,
		sce::Gnm::kDwordSizeSampler,
		sce::Gnm::kDwordSizeVertexBuffer,
		sce::Gnm::kDwordSizeConstantBuffer,
	};

#ifdef __ORBIS__
	SCE_GNM_STATIC_ASSERT(ringBufferOffsetPerStageInCpRam[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexResource]			== kDwordOffsetResource);
	SCE_GNM_STATIC_ASSERT(ringBufferOffsetPerStageInCpRam[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexRwResource]			== kDwordOffsetRwResource);
	SCE_GNM_STATIC_ASSERT(ringBufferOffsetPerStageInCpRam[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexSampler]				== kDwordOffsetSampler);
	SCE_GNM_STATIC_ASSERT(ringBufferOffsetPerStageInCpRam[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexVertexBuffer]		== kDwordOffsetVertexBuffer);
	SCE_GNM_STATIC_ASSERT(ringBufferOffsetPerStageInCpRam[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexConstantBuffer]		== kDwordOffsetConstantBuffer);

	SCE_GNM_STATIC_ASSERT(slotSizeInDword[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexResource]			== sce::Gnm::kDwordSizeResource);
	SCE_GNM_STATIC_ASSERT(slotSizeInDword[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexRwResource]			== sce::Gnm::kDwordSizeRwResource);
	SCE_GNM_STATIC_ASSERT(slotSizeInDword[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexSampler]				== sce::Gnm::kDwordSizeSampler);
	SCE_GNM_STATIC_ASSERT(slotSizeInDword[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexVertexBuffer]		== sce::Gnm::kDwordSizeVertexBuffer);
	SCE_GNM_STATIC_ASSERT(slotSizeInDword[sce::Gnmx::ConstantUpdateEngine::kRingBuffersIndexConstantBuffer]		== sce::Gnm::kDwordSizeConstantBuffer);
#endif // __ORBIS__

	//--------------------------------------------------------------------------------//



	static inline __int128_t *allocateRegionToCopyToCpRam(sce::Gnm::ConstantCommandBuffer *ccb,
														  uint32_t stage, uint32_t baseResourceOffset,
														  uint32_t chunk, uint32_t chunkSizeInDW)
	{
		const uint32_t stageOffset      = stage * kPerStageDwordSize;
		const uint32_t baseOffsetInCp   = baseResourceOffset + stageOffset;
		const uint32_t allocationOffset = baseOffsetInCp + chunk * chunkSizeInDW;

		__int128_t * const allocatedRegion = (__int128_t*)ccb->allocateRegionToCopyToCpRam((uint16_t)(allocationOffset*4), chunkSizeInDW);
		SCE_GNM_VALIDATE(allocatedRegion, "Constant Update Engine Error: Couldn't allocate memory from the cpRam");

#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
		__builtin_memset((uint32_t*)allocatedRegion, 0, chunkSizeInDW*4);
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
		return allocatedRegion;
	}

	static inline __int128_t *allocateRegionToCopyToCpRamForConstantBuffer(sce::Gnm::ConstantCommandBuffer *ccb,
																		   uint32_t stage, uint32_t chunk, uint32_t chunkSizeInDW,
																		   uint32_t allocationSizeInDW)
	{
		const uint32_t stageOffset      = stage * kPerStageDwordSize;
		const uint32_t baseOffsetInCp   = ConstantUpdateEngineHelper::kDwordOffsetConstantBuffer+ stageOffset;
		const uint32_t allocationOffset = baseOffsetInCp + chunk * chunkSizeInDW;

		__int128_t * const allocatedRegion = (__int128_t*)ccb->allocateRegionToCopyToCpRam((uint16_t)(allocationOffset*4), allocationSizeInDW);
		SCE_GNM_VALIDATE(allocatedRegion, "Constant Update Engine Error: Couldn't allocate memory from the cpRam");

#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
		__builtin_memset((uint32_t*)allocatedRegion, 0, allocationSizeInDW*4);
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
		return allocatedRegion;
	}

	static inline void validateRingSetup(sce::Gnmx::ConstantUpdateEngine::RingSetup *ringSetup)
	{
		SCE_GNM_UNUSED(ringSetup);

		SCE_GNM_VALIDATE(ringSetup->numResourceSlots	 <= sce::Gnm::kSlotCountResource,	  "numResourceSlots must be less or equal than Gnm::kSlotCountResource.");
		SCE_GNM_VALIDATE(ringSetup->numRwResourceSlots	 <= sce::Gnm::kSlotCountRwResource,	  "numRwResourceSlots must be less or equal than Gnm::kSlotCountRwResource.");
		SCE_GNM_VALIDATE(ringSetup->numSampleSlots		 <= sce::Gnm::kSlotCountSampler,	  "numSampleSlots must be less or equal than Gnm::kSlotCountSampler.");
		SCE_GNM_VALIDATE(ringSetup->numVertexBufferSlots <= sce::Gnm::kSlotCountVertexBuffer, "numVertexBufferSlots must be less or equal than Gnm::kSlotCountVertexBuffer.");

		SCE_GNM_VALIDATE((ringSetup->numResourceSlots	  & kResourceChunkSizeMask)		== 0, "numResourceSlots must be a multiple of ConstantUpdateEngine::kResourceChunkSize.");
		SCE_GNM_VALIDATE((ringSetup->numRwResourceSlots	  & kRwResourceChunkSizeMask)	== 0, "numRwResourceSlots must be a multiple of ConstantUpdateEngine::kRwResourceChunkSize.");
		SCE_GNM_VALIDATE((ringSetup->numSampleSlots		  & kSamplerChunkSizeMask)		== 0, "numSampleSlots must must be a multiple of ConstantUpdateEngine::kSamplerChunkSize.");
		SCE_GNM_VALIDATE((ringSetup->numVertexBufferSlots & kVertexBufferChunkSizeMask)	== 0, "numVertexBufferSlots must be a multiple of ConstantUpdateEngine::kVertexBufferChunkSize.");

		ringSetup->numResourceSlots		= (ringSetup->numResourceSlots	   + kResourceChunkSizeMask)	 & ~kResourceChunkSizeMask;
		ringSetup->numRwResourceSlots	= (ringSetup->numRwResourceSlots   + kRwResourceChunkSizeMask)	 & ~kRwResourceChunkSizeMask;
		ringSetup->numSampleSlots		= (ringSetup->numSampleSlots	   + kSamplerChunkSizeMask)		 & ~kSamplerChunkSizeMask;
		ringSetup->numVertexBufferSlots = (ringSetup->numVertexBufferSlots + kVertexBufferChunkSizeMask) & ~kVertexBufferChunkSizeMask;
	}

	static inline void cleanEud(sce::Gnmx::ConstantUpdateEngine *cue, sce::Gnm::ShaderStage stage)
	{
		sce::Gnmx::ConstantUpdateEngine::StageInfo *stageInfo = cue->m_stageInfo+stage;

		// Clean EUD:
		cue->m_shaderDirtyEud &= ~(1 << stage);

		// Clear the EUD resource usage:
		stageInfo->eudResourceSet[0]	= 0;
		stageInfo->eudResourceSet[1]	= 0;
		stageInfo->eudSamplerSet		= 0;
		stageInfo->eudConstantBufferSet = 0;
		stageInfo->eudRwResourceSet 	= 0;

		cue->m_eudReferencesStreamoutBuffers = false;
	}

	static inline uint16_t calculateEudSizeInDWord(const sce::Gnm::InputUsageSlot *inputUsageTable, uint32_t inputUsageTableSize)
	{
		if ( inputUsageTableSize == 0)
			return 0;

#if SCE_GNM_CUE2_PARSE_INPUTS_TO_COMPUTE_EUD_SIZE || defined(SCE_GNM_DEBUG)
		uint32_t lastIndex = inputUsageTableSize-1;
		for (uint32_t iInputSlot = 0; iInputSlot < inputUsageTableSize; ++iInputSlot)
		{
			if ( inputUsageTable[iInputSlot].m_startRegister > inputUsageTable[lastIndex].m_startRegister )
				lastIndex = iInputSlot;
		}

#if !SCE_GNM_CUE2_PARSE_INPUTS_TO_COMPUTE_EUD_SIZE && defined(SCE_GNM_DEBUG)
		SCE_GNM_VALIDATE(lastIndex == inputUsageTableSize-1,
							"The input usage slot in the ExtentedUserData buffer are out of order\n"
							"Please update PSSLC or enable the #define: SCE_GNM_CUE2_PARSE_INPUTS_TO_COMPUTE_EUD_SIZE in cue-helper.h");
#endif // !defined(SCE_GNM_CUE2_PARSE_INPUTS_TO_COMPUTE_EUD_SIZE) && defined(SCE_GNM_DEBUG)

		inputUsageTableSize = lastIndex + 1;
#endif // (SCE_GNM_CUE2_PARSE_INPUTS_TO_COMPUTE_EUD_SIZE) || defined(SCE_GNM_DEBUG)
		const sce::Gnm::InputUsageSlot lastInput = inputUsageTable[inputUsageTableSize-1];
		return lastInput.m_startRegister > 15 ? lastInput.m_startRegister + inputSizeInDWords[lastInput.m_usageType] - 16 : 0;
	}

	static inline uint32_t lzcnt128(const uint64_t value[2])
	{
		const uint64_t lo = value[0];
		const uint64_t hi = value[1];

		const uint32_t hicnt = (uint32_t)__lzcnt64(hi);
		const uint32_t locnt = (uint32_t)__lzcnt64(lo);

		return hicnt == 64 ? 64 + locnt : hicnt;
	}

	static inline uint32_t isBitSet(const uint64_t value[2], uint32_t bit)
	{
		return value[bit>>6] & (((uint64_t)1) << (bit & 63)) ? 1 : 0;
	}

	//--------------------------------------------------------------------------------//

	void *getRingAddress(const sce::Gnmx::ConstantUpdateEngine::StageInfo *stageInfo, uint32_t ringIndex);

	static inline uint32_t computeShaderResourceRingSize(uint32_t elemSizeDwords, uint32_t elemCount)
	{
		return (elemSizeDwords)*elemCount*sizeof(uint32_t);
	}

	static inline void* initializeRingBuffer(sce::Gnmx::ConstantUpdateEngine::ShaderResourceRingBuffer *ringBuffer, void *bufferAddr, uint32_t bufferBytes, uint32_t elemSizeDwords, uint32_t elemCount)
	{
		SCE_GNM_VALIDATE(bufferBytes >= computeShaderResourceRingSize(elemSizeDwords, elemCount), "bufferBytes (%d) is too small; use computeSpaceRequirements() to determine the minimum size.", bufferBytes);
		SCE_GNM_VALIDATE(elemCount > 1, "elemCount must be at least 2"); // Need at least two elements
		ringBuffer->headElemIndex	 = elemCount-1;
		ringBuffer->elementsAddr	 = bufferAddr;
		ringBuffer->elemSizeDwords = elemSizeDwords;
		ringBuffer->elemCount		 = elemCount;
		ringBuffer->wrappedIndex	 = 0;
		ringBuffer->halfPointIndex = elemCount/2;

		return (uint8_t*)bufferAddr + bufferBytes;
	}

	static inline void *getRingBuffersNextHead(const sce::Gnmx::ConstantUpdateEngine::ShaderResourceRingBuffer *ringBuffer)
	{
		const uint32_t nextElemIndex = (ringBuffer->headElemIndex+1) % ringBuffer->elemCount;
		return (uint8_t*)ringBuffer->elementsAddr + (nextElemIndex * ringBuffer->elemSizeDwords * sizeof(uint32_t));
	}

	static inline bool advanceRingBuffersHead(sce::Gnmx::ConstantUpdateEngine::ShaderResourceRingBuffer *ringBuffer)
	{
		ringBuffer->headElemIndex = (ringBuffer->headElemIndex+1) % ringBuffer->elemCount;
		return ringBuffer->headElemIndex == ringBuffer->wrappedIndex || ringBuffer->headElemIndex == ringBuffer->halfPointIndex;
	}

	static inline void updateRingBuffersStatePostSubmission(sce::Gnmx::ConstantUpdateEngine::ShaderResourceRingBuffer *ringBuffer)
	{
		ringBuffer->wrappedIndex = ringBuffer->headElemIndex;
		ringBuffer->halfPointIndex = (ringBuffer->headElemIndex + ringBuffer->elemCount/2)%ringBuffer->elemCount;
	}
}


#endif // _SCE_GNMX_CUE2_HELPER_H
