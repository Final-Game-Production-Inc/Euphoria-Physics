/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/common.h"
#ifdef CUE_V2

#include "grcore/gnmx/cue.h"
#include "grcore/gnmx/cue-helper.h"

using namespace sce::Gnm;
using namespace sce::Gnmx;

uint32_t ConstantUpdateEngine::computeHeapSize(uint32_t numRingEntries)
{
	// Note: this code is duplicated in the beginning of ConstantUpdateEngine::init()
	const uint32_t kResourceRingBufferBytes	        = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kSlotCountResource        *Gnm::kDwordSizeResource,         numRingEntries);
	const uint32_t kRwResourceRingBufferBytes	    = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kSlotCountRwResource      *Gnm::kDwordSizeRwResource,       numRingEntries);
	const uint32_t kSamplerRingBufferBytes		    = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kSlotCountSampler         *Gnm::kDwordSizeSampler,          numRingEntries);
	const uint32_t kVertexBufferRingBufferBytes     = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kSlotCountVertexBuffer    *Gnm::kDwordSizeVertexBuffer,     numRingEntries);
	const uint32_t kConstantBufferRingBufferBytes   = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kSlotCountConstantBuffer  *Gnm::kDwordSizeConstantBuffer,   numRingEntries);
	const uint32_t kStreamoutBufferRingBufferBytes  = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kSlotCountStreamoutBuffer *Gnm::kDwordSizeStreamoutBuffer,  numRingEntries);
	const uint32_t kDispatchDrawDataRingBufferBytes = ConstantUpdateEngineHelper::computeShaderResourceRingSize(Gnm::kSlotCountDispatchDrawData*Gnm::kDwordSizeDispatchDrawData, numRingEntries);
	const uint32_t numStages = Gnm::kShaderStageCount;
	const uint32_t totalRingBufferRequiredSize = numStages * (
		kResourceRingBufferBytes +
		kRwResourceRingBufferBytes +
		kSamplerRingBufferBytes +
		kVertexBufferRingBufferBytes +
		kConstantBufferRingBufferBytes +
		kStreamoutBufferRingBufferBytes +
		kDispatchDrawDataRingBufferBytes) +
		0;

	return totalRingBufferRequiredSize;
}


uint32_t ConstantUpdateEngine::computeCpRamShadowSize(void)
{
	return 0;
}


#endif // CUE_V2
