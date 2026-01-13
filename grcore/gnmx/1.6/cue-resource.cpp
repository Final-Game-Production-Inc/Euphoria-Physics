/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.071
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifdef __ORBIS__
#include <x86intrin.h>
#endif // __ORBIS__
#include <gnm.h>

#include "grcore/gnmx/common.h"

#ifdef CUE_V2
#include "grcore/gnmx/cue.h"
#include "grcore/gnmx/cue-helper.h"

using namespace sce::Gnm;
using namespace sce::Gnmx;


void ConstantUpdateEngine::setTextures(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Texture *textures)
{
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountResource, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountResource);
	if ( numApiSlots == 0 )
		return;

#if SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
	if ( !textures ) return;
#endif // SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL

	if ( numApiSlots == 1 )
	{
		//
		// Here the user set only 1 T#
		//

		const uint16_t	curChunk = startApiSlot / kResourceChunkSize;
		const uint16_t	slot	 = (startApiSlot & ConstantUpdateEngineHelper::kResourceChunkSizeMask);
		const uint16_t	slotMask = 1 << (16-slot-1);

		StageChunkState *chunkState = m_stageInfo[stage].resourceStage;
		__int128_t *curData = (__int128_t*)textures;

		//

		// Check for a resource conflict between previous draw and current draw:
		// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
		if ( !chunkState[curChunk].usedChunk &&
			 (chunkState[curChunk].curSlots & slotMask) )
		{
			// Conflict
			chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
			chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
			chunkState[curChunk].curChunk  = 0;
		}

		// No chunk allocated for the current draw (due to conflict, or because it was never used before)
		if ( !chunkState[curChunk].curChunk )
		{
			chunkState[curChunk].curSlots = 0;
			chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetResource, curChunk, ConstantUpdateEngineHelper::kResourceChunkSizeInDWord);
		}

		if ( curData )
		{
			SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kResourceSizeInDqWord == 2);
			chunkState[curChunk].curChunk[2*slot]   = *(curData+0);
			chunkState[curChunk].curChunk[2*slot+1] = *(curData+1);
				//
			chunkState[curChunk].curSlots |= slotMask;
		}
		else
		{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			chunkState[curChunk].curChunk[2*slot]   = 0;
			chunkState[curChunk].curChunk[2*slot+1] = 0;
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

			chunkState[curChunk].curSlots  &= ~slotMask;
			chunkState[curChunk].usedSlots &= ~slotMask; // to avoid keeping older version of this resource
		}

		__int128_t apiMask;

		if ( startApiSlot < 64 )
		{
			uint64_t halfMask = ((uint64_t)1)<<(startApiSlot);
			apiMask = halfMask;
		}
		else
		{
			uint64_t halfMask = (((uint64_t)1)<<(startApiSlot-64));
			apiMask = (__int128_t)halfMask << 64;
		}

		// Check if we need to dirty the EUD:
		if ( m_stageInfo[stage].eudResourceSet & apiMask )
			m_shaderDirtyEud |= (1 << stage);

		// Keep track of used APIs:
		if ( curData )
		{
			m_stageInfo[stage].activeResource |= apiMask;

			// Mark the resource ptr dirty:
			m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexResource);
		}
		else
		{
			m_stageInfo[stage].activeResource &= ~apiMask;
		}
	}
	else
	{
		//
		// The user may set N T# in a row; which could overlap multiple chunk,
		// compute start and end for
		//

		const uint16_t startChunk = startApiSlot / kResourceChunkSize;
		const uint16_t endChunk   = (startApiSlot+numApiSlots-1) / kResourceChunkSize;

		const uint16_t initialSlot = (startApiSlot & ConstantUpdateEngineHelper::kResourceChunkSizeMask);
		const uint16_t finalSlot   = ((startApiSlot+numApiSlots-1) & ConstantUpdateEngineHelper::kResourceChunkSizeMask);

		StageChunkState *chunkState = m_stageInfo[stage].resourceStage;

		uint32_t  curChunk = startChunk;
		__int128_t *curData = (__int128_t*)textures;

		while ( curChunk <= endChunk )
		{
			const uint16_t startSlot = curChunk == startChunk ? initialSlot : 0;
			const uint16_t endSlot   = curChunk == endChunk   ? finalSlot   : ConstantUpdateEngineHelper::kResourceChunkSizeMask; // aka: kResourceChunkSize-1
			const uint16_t range     = ((1 << (16-endSlot-1))-1) ^ ((1 << (16-startSlot))-1);
			//const uint16_t range     = ((1 << (endSlot+1))-1) ^ ((1 << startSlot)-1);

			//

			// Check for a resource conflict between previous draw and current draw:
			// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
			if ( !chunkState[curChunk].usedChunk &&
				 (chunkState[curChunk].curSlots & range) )
			{
				// Conflict
				chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
				chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
				chunkState[curChunk].curChunk  = 0;
			}

			// No chunk allocated for the current draw (due to conflict, or because it was never used before)
			if ( !chunkState[curChunk].curChunk )
			{
				chunkState[curChunk].curSlots = 0;
				chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetResource, curChunk, ConstantUpdateEngineHelper::kResourceChunkSizeInDWord);
			}

			if ( curData )
			{
				// Copy the user resources in the current chunk:
				// TODO: Check is unrolling the loop would help.
				for (uint32_t iResource = startSlot; iResource <= endSlot; ++iResource)
				{
					SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kResourceSizeInDqWord == 2);

					chunkState[curChunk].curChunk[2*iResource]   = *(curData+0);
					chunkState[curChunk].curChunk[2*iResource+1] = *(curData+1);
					curData += 2;
				}
				//
				chunkState[curChunk].curSlots |= range;
			}
			else
			{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				for (uint32_t iResource = startSlot; iResource <= endSlot; ++iResource)
				{
					SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kResourceSizeInDqWord == 2);
					chunkState[curChunk].curChunk[2*iResource]   = 0;
					chunkState[curChunk].curChunk[2*iResource+1] = 0;
				}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

				chunkState[curChunk].curSlots  &= ~range;
				chunkState[curChunk].usedSlots &= ~range; // to avoid keeping older version of this resource
			}

			// Next chunk:
			curChunk++;
		}

		__int128_t apiMask;

		if ( startApiSlot+numApiSlots < 64 )
		{
			uint64_t halfMask = ((((uint64_t)1)<<(startApiSlot+numApiSlots))-1) ^ ((((uint64_t)1)<<startApiSlot)-1);
			apiMask = halfMask;
		}
		else if ( startApiSlot >= 64 )
		{
			uint64_t halfMask = ((((uint64_t)1)<<(startApiSlot+numApiSlots-64))-1) ^ ((((uint64_t)1)<<(startApiSlot-64))-1);
			apiMask = (__int128_t)halfMask << 64;
		}
		else
		{
			apiMask = ((((__int128_t)1)<<(startApiSlot+numApiSlots))-1) ^ ((((__int128_t)1)<<startApiSlot)-1);
		}


		// Check if we need to dirty the EUD:
		if ( m_stageInfo[stage].eudResourceSet & apiMask )
			m_shaderDirtyEud |= (1 << stage);

		// Keep track of used APIs:
		if ( curData )
		{
			m_stageInfo[stage].activeResource |= apiMask;

			// Mark the resource ptr dirty:
			m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexResource);
		}
		else
		{
			m_stageInfo[stage].activeResource &= ~apiMask;
		}
	}
}

void ConstantUpdateEngine::setBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers)
{
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountResource, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountResource);
	if ( numApiSlots == 0 )
		return;

#if SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
	if ( !buffers ) return;
#endif // SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL

	if ( numApiSlots == 1 )
	{
		//
		// Here the user set only 1 T#
		//

		const uint16_t	curChunk = startApiSlot / kResourceChunkSize;
		const uint16_t	slot	 = (startApiSlot & ConstantUpdateEngineHelper::kResourceChunkSizeMask);
		const uint16_t	slotMask = 1 << (16-slot-1);

		StageChunkState *chunkState = m_stageInfo[stage].resourceStage;
		__int128_t *curData = (__int128_t*)buffers;

		//

		// Check for a resource conflict between previous draw and current draw:
		// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
		if ( !chunkState[curChunk].usedChunk &&
			 (chunkState[curChunk].curSlots & slotMask) )
		{
			// Conflict
			chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
			chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
			chunkState[curChunk].curChunk  = 0;
		}

		// No chunk allocated for the current draw (due to conflict, or because it was never used before)
		if ( !chunkState[curChunk].curChunk )
		{
			chunkState[curChunk].curSlots = 0;
			chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetResource, curChunk, ConstantUpdateEngineHelper::kResourceChunkSizeInDWord);
		}

		if ( curData )
		{
			SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kResourceSizeInDqWord == 2);
			chunkState[curChunk].curChunk[2*slot]   = *(curData+0);
			chunkState[curChunk].curChunk[2*slot+1] = 0;
				//
			chunkState[curChunk].curSlots |= slotMask;
		}
		else
		{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			chunkState[curChunk].curChunk[2*slot]   = 0;
			chunkState[curChunk].curChunk[2*slot+1] = 0;
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

			chunkState[curChunk].curSlots  &= ~slotMask;
			chunkState[curChunk].usedSlots &= ~slotMask; // to avoid keeping older version of this resource
		}

		__int128_t apiMask;

		if ( startApiSlot+numApiSlots < 64 )
		{
			uint64_t halfMask = ((((uint64_t)1)<<(startApiSlot+numApiSlots))-1) ^ ((((uint64_t)1)<<startApiSlot)-1);
			apiMask = halfMask;
		}
		else if ( startApiSlot >= 64 )
		{
			uint64_t halfMask = ((((uint64_t)1)<<(startApiSlot+numApiSlots-64))-1) ^ ((((uint64_t)1)<<(startApiSlot-64))-1);
			apiMask = (__int128_t)halfMask << 64;
		}
		else
		{
			apiMask = ((((__int128_t)1)<<(startApiSlot+numApiSlots))-1) ^ ((((__int128_t)1)<<startApiSlot)-1);
		}

		// Check if we need to dirty the EUD:
		if ( m_stageInfo[stage].eudResourceSet & apiMask )
			m_shaderDirtyEud |= (1 << stage);

		// Keep track of used APIs:
		if ( curData )
		{
			m_stageInfo[stage].activeResource |= apiMask;

			// Mark the resource ptr dirty:
			m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexResource);
		}
		else
		{
			m_stageInfo[stage].activeResource &= ~apiMask;
		}
	}
	else
	{
		//
		// The user may set N V# in a row; which could overlap multiple chunk,
		// compute start and end for
		//

		const uint16_t startChunk = startApiSlot / kResourceChunkSize;
		const uint16_t endChunk   = (startApiSlot+numApiSlots-1) / kResourceChunkSize;

		const uint16_t initialSlot = (startApiSlot & ConstantUpdateEngineHelper::kResourceChunkSizeMask);
		const uint16_t finalSlot   = ((startApiSlot+numApiSlots-1) & ConstantUpdateEngineHelper::kResourceChunkSizeMask);

		StageChunkState *chunkState = m_stageInfo[stage].resourceStage;

		uint32_t  curChunk = startChunk;
		__int128_t *curData = (__int128_t*)buffers;

		while ( curChunk <= endChunk )
		{
			const uint16_t startSlot = curChunk == startChunk ? initialSlot : 0;
			const uint16_t endSlot   = curChunk == endChunk   ? finalSlot   : ConstantUpdateEngineHelper::kResourceChunkSizeMask; // aka: kResourceChunkSize-1
			const uint16_t range     = ((1 << (16-endSlot-1))-1) ^ ((1 << (16-startSlot))-1);
			//const uint16_t range     = ((1 << (endSlot+1))-1) ^ ((1 << startSlot)-1);

			//

			// Check for a resource conflict between previous draw and current draw:
			// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
			if ( !chunkState[curChunk].usedChunk &&
				 (chunkState[curChunk].curSlots & range) )
			{
				// Conflict
				chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
				chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
				chunkState[curChunk].curChunk  = 0;
			}

			// No chunk allocated for the current draw (due to conflict, or because it was never used before)
			if ( !chunkState[curChunk].curChunk )
			{
				chunkState[curChunk].curSlots = 0;
				chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetResource, curChunk, ConstantUpdateEngineHelper::kResourceChunkSizeInDWord);
			}

			if ( curData )
			{
				// Copy the user resources in the current chunk:
				// TODO: Check is unrolling the loop would help.
				for (uint32_t iResource = startSlot; iResource <= endSlot; ++iResource)
				{
					SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kResourceSizeInDqWord == 2);

					chunkState[curChunk].curChunk[2*iResource]   = *curData;
					chunkState[curChunk].curChunk[2*iResource+1] = 0;
					curData += 1;
				}
				//
				chunkState[curChunk].curSlots |= range;
			}
			else
			{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
				for (uint32_t iResource = startSlot; iResource <= endSlot; ++iResource)
				{
					SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kResourceSizeInDqWord == 2);
					chunkState[curChunk].curChunk[2*iResource]   = 0;
					chunkState[curChunk].curChunk[2*iResource+1] = 0;
				}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

				chunkState[curChunk].curSlots  &= ~range;
				chunkState[curChunk].usedSlots &= ~range; // to avoid keeping older version of this resource
			}

			// Next chunk:
			curChunk++;
		}

		__int128_t apiMask;

		if ( startApiSlot+numApiSlots < 64 )
		{
			uint64_t halfMask = ((((uint64_t)1)<<(startApiSlot+numApiSlots))-1) ^ ((((uint64_t)1)<<startApiSlot)-1);
			apiMask = halfMask;
		}
		else if ( startApiSlot >= 64 )
		{
			uint64_t halfMask = ((((uint64_t)1)<<(startApiSlot+numApiSlots-64))-1) ^ ((((uint64_t)1)<<(startApiSlot-64))-1);
			apiMask = (__int128_t)halfMask << 64;
		}
		else
		{
			apiMask = ((((__int128_t)1)<<(startApiSlot+numApiSlots))-1) ^ ((((__int128_t)1)<<startApiSlot)-1);
		}

		// Check if we need to dirty the EUD:
		if ( m_stageInfo[stage].eudResourceSet & apiMask )
			m_shaderDirtyEud |= (1 << stage);

		// Keep track of used APIs:
		if ( curData )
		{
			m_stageInfo[stage].activeResource |= apiMask;

			// Mark the resource ptr dirty:
			m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexResource);
		}
		else
		{
			m_stageInfo[stage].activeResource &= ~apiMask;
		}
	}
}

void ConstantUpdateEngine::setSamplers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Sampler *samplers)
{
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountSampler, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountSampler);
	if ( numApiSlots == 0 )
		return;

#if SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
	if ( !samplers ) return;
#endif // SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL

	//
	// The user may set N T# in a row; which could overlap multiple chunk,
	// compute start and end for
	//

	const uint16_t startChunk = startApiSlot / kSamplerChunkSize;
	const uint16_t endChunk   = (startApiSlot+numApiSlots-1) / kSamplerChunkSize;

	const uint16_t initialSlot = (startApiSlot & ConstantUpdateEngineHelper::kSamplerChunkSizeMask);
	const uint16_t finalSlot   = ((startApiSlot+numApiSlots-1) & ConstantUpdateEngineHelper::kSamplerChunkSizeMask);

	StageChunkState *chunkState = m_stageInfo[stage].samplerStage;

	uint32_t  curChunk = startChunk;
	__int128_t *curData = (__int128_t*)samplers;

	while ( curChunk <= endChunk )
	{
		const uint16_t startSlot = curChunk == startChunk ? initialSlot : 0;
		const uint16_t endSlot   = curChunk == endChunk   ? finalSlot   : ConstantUpdateEngineHelper::kSamplerChunkSizeMask; // aka: kSamplerChunkSize-1
		const uint16_t range     = ((1 << (16-endSlot-1))-1) ^ ((1 << (16-startSlot))-1);
		//const uint16_t range     = ((1 << (endSlot+1))-1) ^ ((1 << startSlot)-1);

		//

		// Check for a resource conflict between previous draw and current draw:
		// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
		if ( !chunkState[curChunk].usedChunk &&
			 (chunkState[curChunk].curSlots & range) )
		{
			// Conflict
			chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
			chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
			chunkState[curChunk].curChunk  = 0;
		}

		// No chunk allocated for the current draw (due to conflict, or because it was never used before)
		if ( !chunkState[curChunk].curChunk )
		{
			chunkState[curChunk].curSlots = 0;
			chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetSampler, curChunk, ConstantUpdateEngineHelper::kSamplerChunkSizeInDWord);
		}

		if ( curData )
		{
			// Copy the user resources in the current chunk:
			// TODO: Check is unrolling the loop would help.
			for (uint32_t iSampler = startSlot; iSampler <= endSlot; ++iSampler)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kSamplerSizeInDqWord == 1);

				chunkState[curChunk].curChunk[iSampler]   = *(curData+0);
				curData += 1;
			}
			//
			chunkState[curChunk].curSlots |= range;
		}
		else
		{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			for (uint32_t iSampler = startSlot; iSampler <= endSlot; ++iSampler)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kSamplerSizeInDqWord == 1);
				chunkState[curChunk].curChunk[iSampler]   = 0;
			}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

			chunkState[curChunk].curSlots  &= ~range;
			chunkState[curChunk].usedSlots &= ~range; // to avoid keeping older version of this resource
		}

		// Next chunk:
		curChunk++;
	}

	const uint16_t apiMask = ((1<<(startApiSlot+numApiSlots))-1) ^ ((1<<startApiSlot)-1);

	// Check if we need to dirty the EUD:
	if ( m_stageInfo[stage].eudSamplerSet & apiMask )
		m_shaderDirtyEud |= (1 << stage);

	// Keep track of used APIs:
	if ( curData )
	{
		m_stageInfo[stage].activeSampler |= apiMask;

		// Mark the resource ptr dirty:
		m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexSampler);
	}
	else
	{
		m_stageInfo[stage].activeSampler &= ~apiMask;
	}
}

void ConstantUpdateEngine::setConstantBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers)
{
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountConstantBuffer, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountConstantBuffer);
	if ( numApiSlots == 0 )
		return;

#if SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
	if ( !buffers ) return;
#endif // SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL

	//
	// The user may set N T# in a row; which could overlap multiple chunk,
	// compute start and end for
	//

	const uint16_t startChunk = startApiSlot / kConstantBufferChunkSize;
	const uint16_t endChunk   = (startApiSlot+numApiSlots-1) / kConstantBufferChunkSize;

	const uint16_t initialSlot = (startApiSlot & ConstantUpdateEngineHelper::kConstantBufferChunkSizeMask);
	const uint16_t finalSlot   = ((startApiSlot+numApiSlots-1) & ConstantUpdateEngineHelper::kConstantBufferChunkSizeMask);

	StageChunkState *chunkState = m_stageInfo[stage].constantBufferStage;

	uint32_t  curChunk = startChunk;
	__int128_t *curData = (__int128_t*)buffers;

	while ( curChunk <= endChunk )
	{
		const uint16_t startSlot = curChunk == startChunk ? initialSlot : 0;
		const uint16_t endSlot   = curChunk == endChunk   ? finalSlot   : ConstantUpdateEngineHelper::kConstantBufferChunkSizeMask; // aka: kConstantBufferChunkSize-1
		const uint16_t range     = ((1 << (16-endSlot-1))-1) ^ ((1 << (16-startSlot))-1);
		//const uint16_t range     = ((1 << (endSlot+1))-1) ^ ((1 << startSlot)-1);

		//

		// Check for a resource conflict between previous draw and current draw:
		// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
		if ( !chunkState[curChunk].usedChunk &&
			 (chunkState[curChunk].curSlots & range) )
		{
			// Conflict
			chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
			chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
			chunkState[curChunk].curChunk  = 0;
		}

		// No chunk allocated for the current draw (due to conflict, or because it was never used before)
		if ( !chunkState[curChunk].curChunk )
		{
			chunkState[curChunk].curSlots = 0;
			chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetConstantBuffer, curChunk, ConstantUpdateEngineHelper::kConstantBufferChunkSizeInDWord);
		}

		if ( curData )
		{
			// Copy the user resources in the current chunk:
			// TODO: Check is unrolling the loop would help.
			for (uint32_t iConstant = startSlot; iConstant <= endSlot; ++iConstant)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kConstantBufferSizeInDqWord == 1);

				chunkState[curChunk].curChunk[iConstant]   = *(curData+0);
				curData += 1;
			}
			//
			chunkState[curChunk].curSlots |= range;
		}
		else
		{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			for (uint32_t iConstant = startSlot; iConstant <= endSlot; ++iConstant)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kConstantBufferSizeInDqWord == 1);
				chunkState[curChunk].curChunk[iConstant]   = 0;
			}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

			chunkState[curChunk].curSlots  &= ~range;
			chunkState[curChunk].usedSlots &= ~range; // to avoid keeping older version of this resource
		}

		// Next chunk:
		curChunk++;
	}

	const uint32_t apiMask = ((1<<(startApiSlot+numApiSlots))-1) ^ ((1<<startApiSlot)-1);

	// Check if we need to dirty the EUD:
	if ( m_stageInfo[stage].eudConstantBufferSet & apiMask )
		m_shaderDirtyEud |= (1 << stage);

	// Keep track of used APIs:
	if ( curData )
	{
		m_stageInfo[stage].activeConstantBuffer |= apiMask;

		// Mark the resource ptr dirty:
		m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexConstantBuffer);
	}
	else
	{
		m_stageInfo[stage].activeConstantBuffer &= ~apiMask;
	}
}

void ConstantUpdateEngine::setRwTextures(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Texture *rwTextures)
{
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountRwResource, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountRwResource);
	if ( numApiSlots == 0 )
		return;

#if SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
	if ( !rwTextures ) return;
#endif // SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL

	//
	// The user may set N T# in a row; which could overlap multiple chunk,
	// compute start and end for
	//

	const uint16_t startChunk = startApiSlot / kRwResourceChunkSize;
	const uint16_t endChunk   = (startApiSlot+numApiSlots-1) / kRwResourceChunkSize;

	const uint16_t initialSlot = (startApiSlot & ConstantUpdateEngineHelper::kRwResourceChunkSizeMask);
	const uint16_t finalSlot   = ((startApiSlot+numApiSlots-1) & ConstantUpdateEngineHelper::kRwResourceChunkSizeMask);

	StageChunkState *chunkState = m_stageInfo[stage].rwResourceStage;

	uint32_t  curChunk = startChunk;
	__int128_t *curData = (__int128_t*)rwTextures;

	while ( curChunk <= endChunk )
	{
		const uint16_t startSlot = curChunk == startChunk ? initialSlot : 0;
		const uint16_t endSlot   = curChunk == endChunk   ? finalSlot   : ConstantUpdateEngineHelper::kRwResourceChunkSizeMask; // aka: kRwResourceChunkSize-1
		const uint16_t range     = ((1 << (16-endSlot-1))-1) ^ ((1 << (16-startSlot))-1);
		//const uint16_t range     = ((1 << (endSlot+1))-1) ^ ((1 << startSlot)-1);

		//

		// Check for a resource conflict between previous draw and current draw:
		// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
		if ( !chunkState[curChunk].usedChunk &&
			 (chunkState[curChunk].curSlots & range) )
		{
			// Conflict
			chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
			chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
			chunkState[curChunk].curChunk  = 0;
		}

		// No chunk allocated for the current draw (due to conflict, or because it was never used before)
		if ( !chunkState[curChunk].curChunk )
		{
			chunkState[curChunk].curSlots = 0;
			chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetRwResource, curChunk, ConstantUpdateEngineHelper::kRwResourceChunkSizeInDWord);
		}

		if ( curData )
		{
			// Copy the user resources in the current chunk:
			// TODO: Check is unrolling the loop would help.
			for (uint32_t iRwResource = startSlot; iRwResource <= endSlot; ++iRwResource)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kRwResourceSizeInDqWord == 2);

				chunkState[curChunk].curChunk[2*iRwResource]   = *(curData+0);
				chunkState[curChunk].curChunk[2*iRwResource+1] = *(curData+1);
				curData += 2;
			}
			//
			chunkState[curChunk].curSlots |= range;
		}
		else
		{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			for (uint32_t iRwResource = startSlot; iRwResource <= endSlot; ++iRwResource)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kRwResourceSizeInDqWord == 2);
				chunkState[curChunk].curChunk[2*iRwResource]   = 0;
				chunkState[curChunk].curChunk[2*iRwResource+1] = 0;
			}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

			chunkState[curChunk].curSlots  &= ~range;
			chunkState[curChunk].usedSlots &= ~range; // to avoid keeping older version of this resource
		}

		// Next chunk:
		curChunk++;
	}

	const uint16_t apiMask = ((1<<(startApiSlot+numApiSlots))-1) ^ ((1<<startApiSlot)-1);

	// Check if we need to dirty the EUD:
	if ( m_stageInfo[stage].eudRwResourceSet & apiMask )
		m_shaderDirtyEud |= (1 << stage);

	// Keep track of used APIs:
	if ( curData )
	{
		m_stageInfo[stage].activeRwResource |= apiMask;

		// Mark the resource ptr dirty:
		m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexRwResource);
	}
	else
	{
		m_stageInfo[stage].activeRwResource &= ~apiMask;
	}
}

void ConstantUpdateEngine::setRwBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *rwBuffers)
{
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountRwResource, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountRwResource);
	if ( numApiSlots == 0 )
		return;

#if SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
	if ( !rwBuffers ) return;
#endif // SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL

	//
	// The user may set N T# in a row; which could overlap multiple chunk,
	// compute start and end for
	//

	const uint16_t startChunk = startApiSlot / kRwResourceChunkSize;
	const uint16_t endChunk   = (startApiSlot+numApiSlots-1) / kRwResourceChunkSize;

	const uint16_t initialSlot = (startApiSlot & ConstantUpdateEngineHelper::kRwResourceChunkSizeMask);
	const uint16_t finalSlot   = ((startApiSlot+numApiSlots-1) & ConstantUpdateEngineHelper::kRwResourceChunkSizeMask);

	StageChunkState *chunkState = m_stageInfo[stage].rwResourceStage;

	uint32_t  curChunk = startChunk;
	__int128_t *curData = (__int128_t*)rwBuffers;

	while ( curChunk <= endChunk )
	{
		const uint16_t startSlot = curChunk == startChunk ? initialSlot : 0;
		const uint16_t endSlot   = curChunk == endChunk   ? finalSlot   : ConstantUpdateEngineHelper::kRwResourceChunkSizeMask; // aka: kRwResourceChunkSize-1
		const uint16_t range     = ((1 << (16-endSlot-1))-1) ^ ((1 << (16-startSlot))-1);
		//const uint16_t range     = ((1 << (endSlot+1))-1) ^ ((1 << startSlot)-1);

		//

		// Check for a resource conflict between previous draw and current draw:
		// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
		if ( !chunkState[curChunk].usedChunk &&
			 (chunkState[curChunk].curSlots & range) )
		{
			// Conflict
			chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
			chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
			chunkState[curChunk].curChunk  = 0;
		}

		// No chunk allocated for the current draw (due to conflict, or because it was never used before)
		if ( !chunkState[curChunk].curChunk )
		{
			chunkState[curChunk].curSlots = 0;
			chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetRwResource, curChunk, ConstantUpdateEngineHelper::kRwResourceChunkSizeInDWord);
		}

		if ( curData )
		{
			// Copy the user resources in the current chunk:
			// TODO: Check is unrolling the loop would help.
			for (uint32_t iRwResource = startSlot; iRwResource <= endSlot; ++iRwResource)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kRwResourceSizeInDqWord == 2);

				chunkState[curChunk].curChunk[2*iRwResource]   = *(curData+0);
				chunkState[curChunk].curChunk[2*iRwResource+1] = 0;
				curData += 1;
			}
			//
			chunkState[curChunk].curSlots |= range;
		}
		else
		{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			for (uint32_t iRwResource = startSlot; iRwResource <= endSlot; ++iRwResource)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kRwResourceSizeInDqWord == 2);
				chunkState[curChunk].curChunk[2*iRwResource]   = 0;
				chunkState[curChunk].curChunk[2*iRwResource+1] = 0;
			}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

			chunkState[curChunk].curSlots  &= ~range;
			chunkState[curChunk].usedSlots &= ~range; // to avoid keeping older version of this resource
		}

		// Next chunk:
		curChunk++;
	}

	const uint16_t apiMask = ((1<<(startApiSlot+numApiSlots))-1) ^ ((1<<startApiSlot)-1);

	// Check if we need to dirty the EUD:
	if ( m_stageInfo[stage].eudRwResourceSet & apiMask )
		m_shaderDirtyEud |= (1 << stage);

	// Keep track of used APIs:
	if ( curData )
	{
		m_stageInfo[stage].activeRwResource |= apiMask;

		// Mark the resource ptr dirty:
		m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexRwResource);
	}
	else
	{
		m_stageInfo[stage].activeRwResource &= ~apiMask;
	}
}

void ConstantUpdateEngine::setVertexBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers)
{
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountVertexBuffer, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountVertexBuffer);
	if ( numApiSlots == 0 )
		return;

#if SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
	if ( !buffers ) return;
#endif // SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL

	//
	// The user may set N T# in a row; which could overlap multiple chunk,
	// compute start and end for
	//

	const uint16_t startChunk = startApiSlot / kVertexBufferChunkSize;
	const uint16_t endChunk   = (startApiSlot+numApiSlots-1) / kVertexBufferChunkSize;

	const uint16_t initialSlot = (startApiSlot & ConstantUpdateEngineHelper::kVertexBufferChunkSizeMask);
	const uint16_t finalSlot   = ((startApiSlot+numApiSlots-1) & ConstantUpdateEngineHelper::kVertexBufferChunkSizeMask);

	StageChunkState *chunkState = m_stageInfo[stage].vertexBufferStage;

	uint32_t  curChunk = startChunk;
	__int128_t *curData = (__int128_t*)buffers;

	while ( curChunk <= endChunk )
	{
		const uint16_t startSlot = curChunk == startChunk ? initialSlot : 0;
		const uint16_t endSlot   = curChunk == endChunk   ? finalSlot   : ConstantUpdateEngineHelper::kVertexBufferChunkSizeMask; // aka: kVertexBufferChunkSize-1
		const uint16_t range     = ((1 << (16-endSlot-1))-1) ^ ((1 << (16-startSlot))-1);
		//const uint16_t range     = ((1 << (endSlot+1))-1) ^ ((1 << startSlot)-1);

		//

		// Check for a resource conflict between previous draw and current draw:
		// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
		if ( !chunkState[curChunk].usedChunk &&
			 (chunkState[curChunk].curSlots & range) )
		{
			// Conflict
			chunkState[curChunk].usedSlots = chunkState[curChunk].curSlots;
			chunkState[curChunk].usedChunk = chunkState[curChunk].curChunk;
			chunkState[curChunk].curChunk  = 0;
		}

		// No chunk allocated for the current draw (due to conflict, or because it was never used before)
		if ( !chunkState[curChunk].curChunk )
		{
			chunkState[curChunk].curSlots = 0;
			chunkState[curChunk].curChunk = ConstantUpdateEngineHelper::allocateRegionToCopyToCpRam(m_ccb, stage, ConstantUpdateEngineHelper::kDwordOffsetVertexBuffer, curChunk, ConstantUpdateEngineHelper::kVertexBufferChunkSizeInDWord);
		}

		if ( curData )
		{
			// Copy the user resources in the current chunk:
			// TODO: Check is unrolling the loop would help.
			for (uint32_t iVertexBuffer = startSlot; iVertexBuffer <= endSlot; ++iVertexBuffer)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kVertexBufferSizeInDqWord == 1);

				chunkState[curChunk].curChunk[iVertexBuffer] = *(curData+0);
				curData += 1;
			}
			//
			chunkState[curChunk].curSlots |= range;
		}
		else
		{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
			for (uint32_t iVertexBuffer = startSlot; iVertexBuffer <= endSlot; ++iVertexBuffer)
			{
				SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kVertexBufferSizeInDqWord == 1);
				chunkState[curChunk].curChunk[iVertexBuffer] = 0;
			}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

			chunkState[curChunk].curSlots  &= ~range;
			chunkState[curChunk].usedSlots &= ~range; // to avoid keeping older version of this resource
		}

		// Next chunk:
		curChunk++;
	}

	const uint32_t apiMask = ((1<<(startApiSlot+numApiSlots))-1) ^ ((1<<startApiSlot)-1);

	// TODO: Dirty the EUD if the vertex buffer ptr is set in the EUD

	// Keep track of used APIs:
	if ( curData )
	{
		m_stageInfo[stage].activeVertexBuffer |= apiMask;

		// Mark the resource ptr dirty:
		m_stageInfo[stage].dirtyRing |= 1<<(31-kRingBuffersIndexVertexBuffer);
	}
	else
	{
		m_stageInfo[stage].activeVertexBuffer &= ~apiMask;
	}
}

void ConstantUpdateEngine::setBoolConstants(Gnm::ShaderStage stage, uint32_t maskAnd, uint32_t maskOr)
{
	m_stageInfo[stage].boolValue = (m_stageInfo[stage].boolValue & maskAnd) | maskOr;
}

void ConstantUpdateEngine::setFloatConstants(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const float *floats)
{
	SCE_GNM_ERROR("Not yet implemented in CUE2!");
	SCE_GNM_STATIC_ASSERT(!CUE2_SHOW_UNIMPLEMENTED);
}

void ConstantUpdateEngine::setAppendConsumeCounterRange(Gnm::ShaderStage stage, uint32_t rangeGdsOffsetInBytes, uint32_t rangeSizeInBytes)
{
	m_stageInfo[stage].appendConsumeDword = (rangeGdsOffsetInBytes << 16) | rangeSizeInBytes;
}

void ConstantUpdateEngine::setStreamoutBuffers(uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers)
{
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountStreamoutBuffer, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountConstantBuffer);
	if ( numApiSlots == 0 )
		return;

#if SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
	if ( !buffers ) return;
#endif // SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL

	//
	// Only 4 streamout buffers.
	// Only one chunk
	//

	const uint32_t	stage	  = Gnm::kShaderStageVs;
	const uint16_t	startSlot = (startApiSlot);
	const uint16_t	endSlot   = (startApiSlot+numApiSlots-1);
	const uint16_t	range     = ((1 << (16-endSlot-1))-1) ^ ((1 << (16-startSlot))-1);
	//const uint16_t range     = ((1 << (endSlot+1))-1) ^ ((1 << startSlot)-1);

	StageChunkState *chunkState = &m_streamoutBufferStage;

	__int128_t *curData = (__int128_t*)buffers;

	// Check for a resource conflict between previous draw and current draw:
	// -> a conflict may happen if a texture used in the previous draw is set again for the current one.
	if ( !chunkState->usedChunk &&
		 (chunkState->curSlots & range) )
	{
		// Conflict
		chunkState->usedSlots = chunkState->curSlots;
		chunkState->usedChunk = chunkState->curChunk;
		chunkState->curChunk  = 0;
	}

	// No chunk allocated for the current draw (due to conflict, or because it was never used before)
	if ( !chunkState->curChunk )
	{
		chunkState->curSlots = 0;
		chunkState->curChunk = (__int128_t*)m_dcb->allocateFromCommandBuffer(ConstantUpdateEngineHelper::kStreamoutChunkSizeInBytes, Gnm::kEmbeddedDataAlignment16);

		// Check if we need to dirty the EUD:
		if ( m_eudReferencesStreamoutBuffers )
			m_shaderDirtyEud |= (1 << stage);
	}

	if ( curData )
	{
		// Copy the user resources in the current chunk:
		// TODO: Check is unrolling the loop would help.
		for (uint32_t iSoBuffer = startSlot; iSoBuffer <= endSlot; ++iSoBuffer)
		{
			SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kStreamoutBufferSizeInDqWord == 1);
			chunkState->curChunk[iSoBuffer] = *(curData+0);
			curData += 1;
		}
		//
		chunkState->curSlots |= range;
	}
	else
	{
#if SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
		for (uint32_t iSoBuffer = startSlot; iSoBuffer <= endSlot; ++iSoBuffer)
		{
			SCE_GNM_STATIC_ASSERT(ConstantUpdateEngineHelper::kStreamoutBufferSizeInDqWord == 1);
			chunkState->curChunk[iSoBuffer]   = 0;
		}
#endif // SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

		chunkState->curSlots  &= ~range;
		chunkState->usedSlots &= ~range; // to avoid keeping older version of this resource
	}
}

void ConstantUpdateEngine::setInternalSrtBuffer(ShaderStage stage, void *buf)
{
	m_stageInfo[stage].internalSrtBuffer = buf;
}

void ConstantUpdateEngine::setUserSrtBuffer(ShaderStage stage, const void *buf, uint32_t bufSizeInDwords)
{
	const uint32_t		*bufDwords	 = (const uint32_t *)buf;
	uint32_t			*stageDwords = m_stageInfo[stage].userSrtBuffer;

	switch(bufSizeInDwords)
	{
	 case 8: stageDwords[7] = bufDwords[7];
	 case 7: stageDwords[6] = bufDwords[6];
	 case 6: stageDwords[5] = bufDwords[5];
	 case 5: stageDwords[4] = bufDwords[4];
	 case 4: stageDwords[3] = bufDwords[3];
	 case 3: stageDwords[2] = bufDwords[2];
	 case 2: stageDwords[1] = bufDwords[1];
	 case 1: stageDwords[0] = bufDwords[0];
	}

	m_stageInfo[stage].userSrtBufferSizeInDwords = bufSizeInDwords;
}



//------


// Copy un-overwritten resource from the used by a previous draw to the current chunk.
// e.g: updateChunkState(m_resourceStage[stage], kTextureNumChunks)
void ConstantUpdateEngine::updateChunkState128(StageChunkState *chunkState, uint32_t numChunks)
{
	// This should only be done if the bound shaders request it.

	// TODO: Check is unrolling the loop would help.
	for (uint32_t iChunk = 0; iChunk < numChunks ; ++iChunk)
	{
		// Check which resources needs to be copy from chunk of the previous draw to the current one.
		uint16_t maskToUpdate = chunkState[iChunk].usedSlots & (~chunkState[iChunk].curSlots);

		while ( maskToUpdate )
		{
			const uint16_t ndx = __lzcnt16(maskToUpdate); // 0 represent the high bit (0x8000)
			const uint16_t bit = 1 << (15 - ndx);
			const uint16_t clr = ~bit;

			// Copy the current:
			chunkState[iChunk].curChunk[ndx]   = chunkState[iChunk].usedChunk[ndx];
			maskToUpdate &= clr;
		}

		if ( chunkState[iChunk].curChunk )
			chunkState[iChunk].usedChunk = 0;
		chunkState[iChunk].curSlots |= chunkState[iChunk].usedSlots;
	}
}

void ConstantUpdateEngine::updateChunkState256(StageChunkState *chunkState, uint32_t numChunks)
{
	// This should only be done if the bound shaders request it.

	// TODO: Check is unrolling the loop would help.
    for (uint32_t iChunk = 0; iChunk < numChunks ; ++iChunk)
	{
	    // Check which resources needs to be copy from chunk of the previous draw to the current one.
		uint16_t maskToUpdate = chunkState[iChunk].usedSlots & (~chunkState[iChunk].curSlots);

		while ( maskToUpdate )
		{
			const uint16_t ndx = __lzcnt16(maskToUpdate); // 0 represent the high bit (0x8000)
			const uint16_t bit = 1 << (15 - ndx);
			const uint16_t clr = ~bit;

			// Copy the current:
			chunkState[iChunk].curChunk[2*ndx]   = chunkState[iChunk].usedChunk[2*ndx];
			chunkState[iChunk].curChunk[2*ndx+1] = chunkState[iChunk].usedChunk[2*ndx+1];
			maskToUpdate &= clr;
		}

		if ( chunkState[iChunk].curChunk )
			chunkState[iChunk].usedChunk = 0;
		chunkState[iChunk].curSlots |= chunkState[iChunk].usedSlots;
	}
}


#endif // CUE_V2
