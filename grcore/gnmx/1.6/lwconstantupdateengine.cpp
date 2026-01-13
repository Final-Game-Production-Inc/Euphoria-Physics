/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.071
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(SCE_GNM_OFFLINE_MODE) // LCUE isn't supported off line

#include "grcore/gnmx/lwbaseconstantupdateengine.h"
#include "grcore/gnmx/lwcomputeconstantupdateengine.h"
#include "grcore/gnmx/lwgfxconstantupdateengine.h"

using namespace sce;
using namespace Gnmx;
using namespace Gnmx::LightweightConstantUpdateEngine;

#if defined SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING_ENABLED
	#define SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(a) SCE_GNM_ASSERT(a)
#else
	#define SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(a)
#endif

// Validation of complete resource binding
#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	#define SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(a, b) initResourceBindingValidation(a, b)
	#define SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(a) SCE_GNM_ASSERT(isResourceBindingComplete(a))
	#define SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(a) (a) = true
#else
	#define SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(a, b)
	#define SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(a)
	#define SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(a)
#endif

SCE_GNM_FORCE_INLINE void setPersistentRegisterRange(Gnm::DrawCommandBuffer* dcb, Gnm::ShaderStage shaderStage, uint32_t startSgpr, const uint32_t* values, uint32_t valuesCount)
{
	const uint32_t kGpuStageUserDataRegisterBases[Gnm::kShaderStageCount] = { 0x240, 0xC, 0x4C, 0x8C, 0xCC, 0x10C, 0x14C };
	Gnm::ShaderType shaderType = (shaderStage == Gnm::kShaderStageCs)? Gnm::kShaderTypeCompute : Gnm::kShaderTypeGraphics;

	uint32_t regAddr = (kGpuStageUserDataRegisterBases[(int32_t)shaderStage] + 0x2C00) + startSgpr;
	dcb->setShaderType(shaderType);
	dcb->setPersistentRegisterRange(regAddr, values, valuesCount);
}


SCE_GNM_FORCE_INLINE void setPersistentRegisterRange(Gnm::DispatchCommandBuffer* dcb, Gnm::ShaderStage shaderStage, uint32_t startSgpr, const uint32_t* values, uint32_t valuesCount)
{
	const uint32_t kGpuStageUserDataRegisterBases[Gnm::kShaderStageCount] = { 0x240, 0xC, 0x4C, 0x8C, 0xCC, 0x10C, 0x14C };
	Gnm::ShaderType shaderType = (shaderStage == Gnm::kShaderStageCs)? Gnm::kShaderTypeCompute : Gnm::kShaderTypeGraphics;
	(void)shaderType;

	uint32_t regAddr = (kGpuStageUserDataRegisterBases[(int32_t)shaderStage] + 0x2C00) + startSgpr;
	dcb->setPersistentRegisterRange(regAddr, values, valuesCount);
}


SCE_GNM_FORCE_INLINE void setPtrInPersistentRegister(Gnm::DrawCommandBuffer* dcb, Gnm::ShaderStage shaderStage, uint32_t startSgpr, const void* address)
{
	void* gpuAddress = (void*)address;
	dcb->setPointerInUserData(shaderStage, startSgpr, gpuAddress);
}


SCE_GNM_FORCE_INLINE void setPtrInPersistentRegister(Gnm::DispatchCommandBuffer* dcb, Gnm::ShaderStage shaderStage, uint32_t startSgpr, const void* address)
{
	(void)shaderStage;
	void* gpuAddress = (void*)address;
	dcb->setPointerInUserData(startSgpr, gpuAddress);
}


SCE_GNM_FORCE_INLINE void setDataInUserDataSgprOrMemory(Gnm::DrawCommandBuffer* dcb, uint32_t* scratchBuffer, Gnm::ShaderStage shaderStage, uint16_t shaderResourceOffset, const void* restrict data, int32_t dataSizeInBytes)
{
	SCE_GNM_ASSERT(dcb != NULL);
	SCE_GNM_ASSERT(data != NULL && dataSizeInBytes > 0 && (dataSizeInBytes%4) == 0);

	int32_t userDataRegisterOrMemoryOffset = (shaderResourceOffset & LightweightConstantUpdateEngine::kResourceValueMask);
	if ((shaderResourceOffset & LightweightConstantUpdateEngine::kResourceInUserDataSgpr) != 0)
	{
		setPersistentRegisterRange(dcb, shaderStage, userDataRegisterOrMemoryOffset, (uint32_t*)data, dataSizeInBytes/sizeof(uint32_t));
	}
	else
	{
		uint32_t* restrict scratchDestAddress = (uint32_t*)(scratchBuffer + ((int)shaderStage * LightweightConstantUpdateEngine::kGpuStageBufferSizeInDwords) + userDataRegisterOrMemoryOffset);
		__builtin_memcpy(scratchDestAddress, data, dataSizeInBytes);
	}
}


SCE_GNM_FORCE_INLINE void setDataInUserDataSgprOrMemory(Gnm::DispatchCommandBuffer* dcb, uint32_t* scratchBuffer, Gnm::ShaderStage shaderStage, uint16_t shaderResourceOffset, const void* restrict data, int32_t dataSizeInBytes)
{
	SCE_GNM_ASSERT(dcb != NULL);
	SCE_GNM_ASSERT(data != NULL && dataSizeInBytes > 0 && (dataSizeInBytes%4) == 0);

	int32_t userDataRegisterOrMemoryOffset = (shaderResourceOffset & LightweightConstantUpdateEngine::kResourceValueMask);
	if ((shaderResourceOffset & LightweightConstantUpdateEngine::kResourceInUserDataSgpr) != 0)
	{
		setPersistentRegisterRange(dcb, shaderStage, userDataRegisterOrMemoryOffset, (uint32_t*)data, dataSizeInBytes/sizeof(uint32_t));
	}
	else
	{
		uint32_t* restrict scratchDestAddress = (uint32_t*)(scratchBuffer + ((int)shaderStage * LightweightConstantUpdateEngine::kGpuStageBufferSizeInDwords) + userDataRegisterOrMemoryOffset);
		__builtin_memcpy(scratchDestAddress, data, dataSizeInBytes);
	}
}


#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
bool isResourceBindingComplete(const LightweightConstantUpdateEngine::ShaderResourceBindingValidation* table)
{
	bool isValid = true;
	if (table != NULL)
	{
		int32_t i=0; // Failed resource index
		while (isValid && i<LightweightConstantUpdateEngine::kMaxConstantBufferCount) { isValid &= table->constBufferOffsetIsBound[i]; ++i; } --i;	SCE_GNM_ASSERT(isValid); i=0;	// Vertex buffer not bound
		while (isValid && i<LightweightConstantUpdateEngine::kMaxVertexBufferCount) { isValid &= table->vertexBufferOffsetIsBound[i]; ++i; } --i;	SCE_GNM_ASSERT(isValid); i=0;	// Constant buffer not bound
		while (isValid && i<LightweightConstantUpdateEngine::kMaxResourceCount) { isValid &= table->resourceOffsetIsBound[i]; ++i; } --i;			SCE_GNM_ASSERT(isValid); i=0;	// Resource not bound
		while (isValid && i<LightweightConstantUpdateEngine::kMaxRwResourceCount) { isValid &= table->rwResourceOffsetIsBound[i]; ++i; } --i;		SCE_GNM_ASSERT(isValid); i=0;	// RW-resource not bound
		while (isValid && i<LightweightConstantUpdateEngine::kMaxSamplerCount) { isValid &= table->samplerOffsetIsBound[i]; ++i; } --i;			SCE_GNM_ASSERT(isValid); i=0;	// Sampler not bound
		//while (isValid && i<LCUE::kMaxStreamOutBufferCount) { isValid &= table->streamOutOffsetIsBound[i]; ++i; } --i;	SCE_GNM_ASSERT(isValid); i=0;	// Stream-out not bound
		isValid &= table->appendConsumeCounterIsBound;																	SCE_GNM_ASSERT(isValid);		// AppendConsumeCounter not bound

		// Note: if failing on Constant-Buffer slot 15, see updateEmbeddedCb(), it might be related
	}
	return isValid;
}


void initResourceBindingValidation(LightweightConstantUpdateEngine::ShaderResourceBindingValidation* validationTable, const LightweightConstantUpdateEngine::InputResourceOffsets* table)
{
	if (table != NULL)
	{
		// Mark all expected resources slots (!= 0xFFFF) as not bound (false)
		for (int32_t i=0; i<LightweightConstantUpdateEngine::kMaxConstantBufferCount; ++i)		validationTable->constBufferOffsetIsBound[i] = (table->constBufferDwOffset[i] == 0xFFFF);
		for (int32_t i=0; i<LightweightConstantUpdateEngine::kMaxVertexBufferCount; ++i)		validationTable->vertexBufferOffsetIsBound[i] = (table->vertexBufferDwOffset[i] == 0xFFFF);
		for (int32_t i=0; i<LightweightConstantUpdateEngine::kMaxResourceCount; ++i)			validationTable->resourceOffsetIsBound[i] = (table->resourceDwOffset[i] == 0xFFFF);
		for (int32_t i=0; i<LightweightConstantUpdateEngine::kMaxRwResourceCount; ++i)			validationTable->rwResourceOffsetIsBound[i] = (table->rwResourceDwOffset[i] == 0xFFFF);
		for (int32_t i=0; i<LightweightConstantUpdateEngine::kMaxSamplerCount; ++i)			validationTable->samplerOffsetIsBound[i] = (table->samplerDwOffset[i] == 0xFFFF);
		//for (int32_t i=0; i<LCUE::kMaxStreamOutBufferCount; ++i)	validationTable->streamOutOffsetIsBound[i] = (table->streamOutDwOffset[i] == 0xFFFF);
		validationTable->appendConsumeCounterIsBound = (table->appendConsumeCounterSgpr == 0xFF);
	}
	else
	{
		// If there's no table, all resources are valid
		SCE_GNM_ASSERT( sizeof(bool) == sizeof(unsigned char) );
		__builtin_memset(validationTable, true, sizeof(LightweightConstantUpdateEngine::ShaderResourceBindingValidation));
	}
}
#endif	// SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED


void BaseConstantUpdateEngine::init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, void* globalInternalResourceTableAddr)
{
	SCE_GNM_ASSERT(resourceBuffersInGarlic != NULL && resourceBufferCount >= 1 && resourceBufferCount <= kMaxResourceBufferCount && resourceBufferSizeInDwords >= kMinResourceBufferSizeInDwords);

	m_bufferIndex = 0;
	m_bufferCount = (resourceBufferCount < kMaxResourceBufferCount)? resourceBufferCount : kMaxResourceBufferCount;
	for (int32_t i=0; i<m_bufferCount; ++i)
	{
		m_bufferBegin[i] = resourceBuffersInGarlic[i];
		m_bufferEnd[i] = m_bufferBegin[i] + resourceBufferSizeInDwords;
	}
	m_bufferCurrent = m_bufferBegin[m_bufferIndex];

	m_globalInternalResourceTableAddr = (Gnm::Buffer*)globalInternalResourceTableAddr;
}


void BaseConstantUpdateEngine::swapBuffers()
{
	m_bufferIndex = (m_bufferIndex+1) % m_bufferCount;
	m_bufferCurrent = m_bufferBegin[m_bufferIndex];
}


void BaseConstantUpdateEngine::setGlobalInternalResource(Gnm::ShaderGlobalResourceType resourceType, const Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(m_globalInternalResourceTableAddr != NULL && buffer != NULL);
	SCE_GNM_ASSERT(resourceType >= 0 && resourceType < Gnm::kShaderGlobalResourceCount);
	
	// Tessellation-factor-buffer - Checks address, alignment, size and memory type (apparently, must be Gnm::kResourceMemoryTypeGC)
	SCE_GNM_ASSERT(resourceType != Gnm::kShaderGlobalResourceTessFactorBuffer || 
				  (buffer->getBaseAddress() == sce::Gnm::getTheTessellationFactorRingBufferBaseAddress() &&
				  ((uintptr_t)buffer->getBaseAddress() % Gnm::kAlignmentOfTessFactorBufferInBytes) == 0 &&
				  buffer->getSize() == Gnm::kTfRingSizeInBytes) );

	__builtin_memcpy(&m_globalInternalResourceTableAddr[(int32_t)resourceType], buffer, sizeof(Gnm::Buffer));
}


void ComputeConstantUpdateEngine::init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, void* globalInternalResourceTableAddr)
{
	BaseConstantUpdateEngine::init(resourceBuffersInGarlic, resourceBufferCount, resourceBufferSizeInDwords, globalInternalResourceTableAddr);
	__builtin_memset(m_scratchBuffer, 0, sizeof(uint32_t) * kComputeScratchBufferSizeInDwords);

	m_dirtyShaderResources = false;
	m_dirtyShader = false;
	m_boundShaderResourceOffsets = NULL;
	m_boundShader = NULL;

	m_dcb = NULL;

#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	__builtin_memset(&m_boundShaderResourcesValidation, true, sizeof(ShaderResourceBindingValidation));
#endif
}


void ComputeConstantUpdateEngine::swapBuffers()
{
	BaseConstantUpdateEngine::swapBuffers();
	m_dirtyShaderResources = false;
	m_dirtyShader = false;
	m_boundShaderResourceOffsets = NULL;
	m_boundShader = NULL;

#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	__builtin_memset(&m_boundShaderResourcesValidation, true, sizeof(ShaderResourceBindingValidation));
#endif

#if defined (SCE_GNM_LCUE_CLEAR_HARDWARE_KCACHE)
	// To ensure no stale data will be fetched, first invalidate the KCache/L1/L2
	m_dcb->flushShaderCachesAndWait(Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionInvalidateKCache);
#endif
}


SCE_GNM_FORCE_INLINE uint32_t* ComputeConstantUpdateEngine::flushScratchBuffer()
{
	SCE_GNM_ASSERT(m_boundShader != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets;
//	SCE_GNM_ASSERT((m_bufferCurrent + table->requiredBufferSizeInDwords) < m_bufferEnd[m_bufferIndex]);
	if ((m_bufferCurrent + table->requiredBufferSizeInDwords) >= m_bufferEnd[m_bufferIndex])
	{
		SCE_GNM_ERROR("ComputeConstantUpdateEngine::flushScratchBuffer() failed due to out-of-memory, please increase the resource buffer size in ComputeConstantUpdateEngine::init() ");
	}

	// Copy scratch data over the main buffer
	uint32_t* restrict destAddr = m_bufferCurrent;
	uint32_t* restrict sourceAddr = m_scratchBuffer;
	m_bufferCurrent += table->requiredBufferSizeInDwords;
	__builtin_memcpy(destAddr, sourceAddr, table->requiredBufferSizeInDwords * sizeof(uint32_t));

	return destAddr;
}


SCE_GNM_FORCE_INLINE void ComputeConstantUpdateEngine::updateCommonPtrsInUserDataSgprs(const uint32_t* resourceBufferFlushedAddress)
{
	SCE_GNM_ASSERT(m_boundShader != NULL);
	const InputResourceOffsets* table = m_boundShaderResourceOffsets;

	if (table->userExtendedData1PtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, Gnm::kShaderStageCs, table->userExtendedData1PtrSgpr, resourceBufferFlushedAddress);
	}
	if (table->constBufferPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, Gnm::kShaderStageCs, table->constBufferPtrSgpr, resourceBufferFlushedAddress + table->constBufferArrayDwOffset);
	}
	if (table->resourcePtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, Gnm::kShaderStageCs, table->resourcePtrSgpr, resourceBufferFlushedAddress + table->resourceArrayDwOffset);
	}
	if (table->rwResourcePtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, Gnm::kShaderStageCs, table->rwResourcePtrSgpr, resourceBufferFlushedAddress + table->rwResourceArrayDwOffset);
	}
	if (table->samplerPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, Gnm::kShaderStageCs, table->samplerPtrSgpr, resourceBufferFlushedAddress + table->samplerArrayDwOffset);
	}
	if (table->globalInternalPtrSgpr != 0xFF)
	{
		SCE_GNM_ASSERT(m_globalInternalResourceTableAddr != NULL);
		setPtrInPersistentRegister(m_dcb, Gnm::kShaderStageCs, table->globalInternalPtrSgpr, m_globalInternalResourceTableAddr);
	}
	if (table->appendConsumeCounterSgpr != 0xFF)
	{
		setPersistentRegisterRange(m_dcb, Gnm::kShaderStageCs, table->appendConsumeCounterSgpr, &m_boundShaderAppendConsumeCounterRange, 1);
	}
}

/** @brief The Embedded Constant Buffer (ECB) descriptor is automatically generated and bound for shaders that have dependencies
 *	An ECB is generated when global static arrays are accessed through dynamic variables, preventing the compiler from embedding its immediate values in the shader code.
 *  The ECB is stored after the end of the shader code area, and is expected on API-slot 15.
 */
SCE_GNM_FORCE_INLINE void ComputeConstantUpdateEngine::updateEmbeddedCb(const Gnmx::ShaderCommonData* shaderCommon)
{
	if (shaderCommon != NULL && shaderCommon->m_embeddedConstantBufferSizeInDQW > 0)
	{
		const uint32_t* shaderRegisters = (const uint32_t*)(shaderCommon + 1);
		const uint8_t* shaderCode = (uint8_t*)( ((uintptr_t)shaderRegisters[0] << 8ULL) | ((uintptr_t)shaderRegisters[1] << 40ULL) );

		Gnm::Buffer embeddedCb;
		embeddedCb.initAsConstantBuffer((void*)(shaderCode + shaderCommon->m_shaderSize), shaderCommon->m_embeddedConstantBufferSizeInDQW << 4);
		setConstantBuffers(LightweightConstantUpdateEngine::kConstantBufferInternalApiSlotForEmbeddedData, 1, &embeddedCb);
	}
}


void ComputeConstantUpdateEngine::preDispatch()
{
	SCE_GNM_ASSERT(m_dcb != NULL);

	const Gnmx::CsShader* csShader = (const Gnmx::CsShader*)m_boundShader;
	if (m_dirtyShader)
		m_dcb->setCsShader( &csShader->m_csStageRegisters );

	// Handle Immediate Constant Buffer on CB slot 15
	if (m_dirtyShader || m_dirtyShaderResources)
		updateEmbeddedCb((const Gnmx::ShaderCommonData*)csShader);

	if (m_dirtyShaderResources)
	{
		SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE( &m_boundShaderResourcesValidation );
		updateCommonPtrsInUserDataSgprs( flushScratchBuffer() );
	}

	m_dirtyShaderResources = false;
	m_dirtyShader = false;
}


void ComputeConstantUpdateEngine::setCsShader(const Gnmx::CsShader* shader, const InputResourceOffsets* table)
{
	SCE_GNM_ASSERT(shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageCs);
	SCE_GNM_ASSERT((m_bufferCurrent + table->requiredBufferSizeInDwords) < m_bufferEnd[m_bufferIndex]);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(&m_boundShaderResourcesValidation, table);

	m_dirtyShaderResources = true;
	m_dirtyShader |= (m_boundShader != shader);
	m_boundShaderResourceOffsets = table;
	m_boundShader = shader;
}


void ComputeConstantUpdateEngine::setAppendConsumeCounterRange(uint32_t gdsMemoryBaseInBytes, uint32_t countersSizeInBytes)
{
	SCE_GNM_ASSERT((gdsMemoryBaseInBytes%4)==0 && gdsMemoryBaseInBytes < UINT16_MAX && (countersSizeInBytes%4)==0 && countersSizeInBytes < UINT16_MAX);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets != NULL);

	SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets->appendConsumeCounterSgpr != 0xFF);
	if (m_boundShaderResourceOffsets->appendConsumeCounterSgpr != 0xFF)
	{
		SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.appendConsumeCounterIsBound);
		m_boundShaderAppendConsumeCounterRange = (gdsMemoryBaseInBytes << 16) | countersSizeInBytes;
	}

	m_dirtyShaderResources = true;
}


void ComputeConstantUpdateEngine::setConstantBuffers(int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxConstantBufferCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets != NULL && buffer != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_CONSTANT_BUFFER(buffer+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets->constBufferDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets->constBufferDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED( m_boundShaderResourcesValidation.constBufferOffsetIsBound[currentApiSlot] );
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, Gnm::kShaderStageCs, table->constBufferDwOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
		}
	}

	m_dirtyShaderResources = true;
}


void ComputeConstantUpdateEngine::setBuffers(int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxResourceCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets != NULL && buffer != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_BUFFER(buffer+i);
		// Cannot currently be validated as this information is not stored for entries in the resource-flat-table (possible to retrieve from Shader::Binary)
		//SCE_GNM_ASSERT((m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceDwOffset[currentApiSlot] & kResourceIsVSharp) != 0);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets->resourceDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets->resourceDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.resourceOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, Gnm::kShaderStageCs, table->resourceDwOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
		}
	}

	m_dirtyShaderResources = true;
}


void ComputeConstantUpdateEngine::setRwBuffers(int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxRwResourceCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets != NULL && buffer != NULL);
	SCE_GNM_ASSERT(buffer->getResourceMemoryType() != Gnm::kResourceMemoryTypeRO);
	
	const InputResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_RWBUFFER(buffer+i);
		// Cannot currently be validated as this information is not stored for entries in the resource-flat-table (possible to retrieve from Shader::Binary)
		//SCE_GNM_ASSERT((m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceDwOffset[currentApiSlot] & kResourceIsVSharp) != 0);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets->rwResourceDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets->rwResourceDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.rwResourceOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, Gnm::kShaderStageCs, table->rwResourceDwOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
		}
	}

	m_dirtyShaderResources = true;
}


void ComputeConstantUpdateEngine::setTextures(int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Texture* texture)
{
	SCE_GNM_ASSERT(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxResourceCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets != NULL && texture != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_TEXTURE(texture+i);
		// Cannot currently be validated as this information is not stored for entries in the resource-flat-table (possible to retrieve from Shader::Binary)
		//SCE_GNM_ASSERT((m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceDwOffset[currentApiSlot] & kResourceIsVSharp) == 0);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets->resourceDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets->resourceDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.resourceOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, Gnm::kShaderStageCs, table->resourceDwOffset[currentApiSlot], texture+i, sizeof(Gnm::Texture));
		}
	}

	m_dirtyShaderResources = true;
}


void ComputeConstantUpdateEngine::setRwTextures(int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Texture* texture)
{
	SCE_GNM_ASSERT(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxRwResourceCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets != NULL && texture != NULL);
	SCE_GNM_ASSERT(texture->getResourceMemoryType() != Gnm::kResourceMemoryTypeRO);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_RWTEXTURE(texture+i);
		// Cannot currently be validated as this information is not stored for entries in the resource-flat-table (possible to retrieve from Shader::Binary)
		//SCE_GNM_ASSERT((m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceDwOffset[currentApiSlot] & kResourceIsVSharp) == 0);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets->rwResourceDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets->rwResourceDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.rwResourceOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, Gnm::kShaderStageCs, table->rwResourceDwOffset[currentApiSlot], texture+i, sizeof(Gnm::Texture));
		}
	}

	m_dirtyShaderResources = true;
}


void ComputeConstantUpdateEngine::setSamplers(int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Sampler* sampler)
{
	SCE_GNM_ASSERT(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxSamplerCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets != NULL && sampler != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_SAMPLER(sampler+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets->samplerDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets->samplerDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.samplerOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, Gnm::kShaderStageCs, table->samplerDwOffset[currentApiSlot], sampler+i, sizeof(Gnm::Sampler));
		}
	}

	m_dirtyShaderResources = true;
}


void ComputeConstantUpdateEngine::setUserData(int32_t startSgpr, int32_t sgprCount, const uint32_t* data)
{
	SCE_GNM_ASSERT(startSgpr >= 0 && (startSgpr+sgprCount) <= kMaxUserDataCount);
	setPersistentRegisterRange(m_dcb, Gnm::kShaderStageCs, startSgpr, data, sgprCount);
}


void ComputeConstantUpdateEngine::setPtrInUserData(int32_t startSgpr, const void* gpuAddress)
{
	SCE_GNM_ASSERT(startSgpr >= 0 && (startSgpr) <= kMaxUserDataCount);
	setPtrInPersistentRegister(m_dcb, Gnm::kShaderStageCs, startSgpr, gpuAddress);
}


void ComputeConstantUpdateEngine::setUserSrtBuffer(const void* buffer, uint32_t sizeInDwords)
{
	SCE_GNM_UNUSED(sizeInDwords);
	SCE_GNM_ASSERT(sizeInDwords > 0 && sizeInDwords <= kMaxSrtUserDataCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets != NULL && buffer != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets;
	SCE_GNM_ASSERT(sizeInDwords == table->userSrtDataCount);
	setUserData(table->userSrtDataSgpr, table->userSrtDataCount, (const uint32_t *)buffer);
}

void GraphicsConstantUpdateEngine::init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, void* globalInternalResourceTableAddr)
{
	BaseConstantUpdateEngine::init(resourceBuffersInGarlic, resourceBufferCount, resourceBufferSizeInDwords, globalInternalResourceTableAddr);
	__builtin_memset(m_scratchBuffer, 0, sizeof(uint32_t) * kGraphicsScratchBufferSizeInDwords);

	__builtin_memset(m_dirtyShaderResources, 0, sizeof(bool) * Gnm::kShaderStageCount);
	__builtin_memset(m_dirtyShader, 0, sizeof(bool) * Gnm::kShaderStageCount);
	__builtin_memset(m_boundShaderResourceOffsets, 0, sizeof(void*) * Gnm::kShaderStageCount);
	__builtin_memset(m_boundShader, 0, sizeof(void*) * Gnm::kShaderStageCount);
	m_dirtyShader[Gnm::kShaderStagePs] = true; // First Pixel Shader MUST be marked as dirty (handles issue when first PS is NULL)

	m_dcb = NULL;
	m_activeShaderStages = Gnm::kActiveShaderStagesVsPs;

	// Fixed offset table for the VS GPU stage when the Geometry pipeline is enabled (the copy shader)
	__builtin_memset(&m_fixedGsVsShaderResourceOffsets, 0xFF, sizeof(InputResourceOffsets));
	__builtin_memset(&m_fixedGsVsStreamOutShaderResourceOffsets, 0xFF, sizeof(InputResourceOffsets));
	m_fixedGsVsShaderResourceOffsets.requiredBufferSizeInDwords = 0;
	m_fixedGsVsShaderResourceOffsets.shaderStage = Gnm::kShaderStageVs;
	m_fixedGsVsShaderResourceOffsets.globalInternalPtrSgpr = kDefaultGlobalInternalTablePtrSgpr;
	m_fixedGsVsStreamOutShaderResourceOffsets.requiredBufferSizeInDwords = kMaxStreamOutBufferCount * sizeof(Gnm::Buffer); // 4 stream-out buffers
	m_fixedGsVsStreamOutShaderResourceOffsets.shaderStage = Gnm::kShaderStageVs;
	m_fixedGsVsStreamOutShaderResourceOffsets.globalInternalPtrSgpr = kDefaultGlobalInternalTablePtrSgpr;
	m_fixedGsVsStreamOutShaderResourceOffsets.streamOutPtrSgpr = kDefaultStreamOutTablePtrSgpr;
	m_fixedGsVsStreamOutShaderResourceOffsets.streamOutArrayDwOffset = 0;
	for (int32_t i=0; i<kMaxStreamOutBufferCount; i++)
		m_fixedGsVsStreamOutShaderResourceOffsets.streamOutDwOffset[i] = i * sizeof(Gnm::Buffer) / sizeof(uint32_t);

	m_gsMode = Gnm::kGsModeDisable;
	m_gsMaxOutput = Gnm::kGsMaxOutputVertexCount1024;

	m_tessellationCurrentTgPatchCount = 0;
	m_tessellationAutoManageReservedCb = true;

#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	__builtin_memset(m_boundShaderResourcesValidation, true, sizeof(ShaderResourceBindingValidation) * Gnm::kShaderStageCount);
#endif
}


void GraphicsConstantUpdateEngine::swapBuffers()
{
	BaseConstantUpdateEngine::swapBuffers();
	__builtin_memset(m_dirtyShaderResources, 0, sizeof(bool) * Gnm::kShaderStageCount);
	__builtin_memset(m_dirtyShader, 0, sizeof(bool) * Gnm::kShaderStageCount);
	__builtin_memset(m_boundShaderResourceOffsets, 0, sizeof(void*) * Gnm::kShaderStageCount);
	__builtin_memset(m_boundShader, 0, sizeof(void*) * Gnm::kShaderStageCount);
	m_dirtyShader[Gnm::kShaderStagePs] = true; // First Pixel Shader MUST be marked as dirty (handles issue when first PS is NULL)

#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	__builtin_memset(m_boundShaderResourcesValidation, true, sizeof(ShaderResourceBindingValidation) * Gnm::kShaderStageCount);
#endif

#if defined (SCE_GNM_LCUE_CLEAR_HARDWARE_KCACHE)
	// To ensure no stale data will be fetched, first invalidate the KCache/L1/L2
	// Note this will be the very first command in the dcb, even before WaitUntilSafeForRender()
	m_dcb->flushShaderCachesAndWait(Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionInvalidateKCache, Gnm::kStallCommandBufferParserDisable);
#endif
}


SCE_GNM_FORCE_INLINE uint32_t* GraphicsConstantUpdateEngine::flushScratchBuffer(Gnm::ShaderStage shaderStage)
{
	SCE_GNM_ASSERT(m_boundShader[(int32_t)shaderStage] != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
//	SCE_GNM_ASSERT((m_bufferCurrent + table->requiredBufferSizeInDwords) < m_bufferEnd[m_bufferIndex]);
	if ((m_bufferCurrent + table->requiredBufferSizeInDwords) >= m_bufferEnd[m_bufferIndex])
	{
		SCE_GNM_ERROR("GraphicsConstantUpdateEngine::flushScratchBuffer() failed due to out-of-memory, please increase the resource buffer size in GraphicsConstantUpdateEngine::init() ");
	}

	// Copy scratch data over the main buffer
	uint32_t* restrict destAddr = m_bufferCurrent;
	uint32_t* restrict sourceAddr = m_scratchBuffer + ((int32_t)shaderStage * kGpuStageBufferSizeInDwords);
	m_bufferCurrent += table->requiredBufferSizeInDwords;
	__builtin_memcpy(destAddr, sourceAddr, table->requiredBufferSizeInDwords * sizeof(uint32_t));

	return destAddr;
}


SCE_GNM_FORCE_INLINE void GraphicsConstantUpdateEngine::updateLsEsVsPtrsInUserDataSgprs(Gnm::ShaderStage shaderStage, const uint32_t* resourceBufferFlushedAddress)
{
	SCE_GNM_ASSERT(m_boundShader[(int32_t)shaderStage] != NULL);
	SCE_GNM_ASSERT(shaderStage == Gnm::kShaderStageLs || shaderStage == Gnm::kShaderStageEs || shaderStage == Gnm::kShaderStageVs);
	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];

	if (table->fetchShaderPtrSgpr != 0xFF)
	{
		SCE_GNM_ASSERT(m_boundFetchShader[(int32_t)shaderStage] != NULL);
		setPtrInPersistentRegister(m_dcb, shaderStage, table->fetchShaderPtrSgpr, m_boundFetchShader[(int32_t)shaderStage]);
	}
	if (table->vertexBufferPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, shaderStage, table->vertexBufferPtrSgpr, resourceBufferFlushedAddress + table->vertexBufferArrayDwOffset );
	}
	if (table->streamOutPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, shaderStage, table->streamOutPtrSgpr, resourceBufferFlushedAddress + table->streamOutArrayDwOffset);
	}
}


SCE_GNM_FORCE_INLINE void GraphicsConstantUpdateEngine::updateCommonPtrsInUserDataSgprs(Gnm::ShaderStage shaderStage, const uint32_t* resourceBufferFlushedAddress)
{
	SCE_GNM_ASSERT(m_boundShader[(int32_t)shaderStage] != NULL);
	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	
	if (table->userExtendedData1PtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, shaderStage, table->userExtendedData1PtrSgpr, resourceBufferFlushedAddress);
	}
	if (table->constBufferPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, shaderStage, table->constBufferPtrSgpr, resourceBufferFlushedAddress + table->constBufferArrayDwOffset);
	}
	if (table->resourcePtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, shaderStage, table->resourcePtrSgpr, resourceBufferFlushedAddress + table->resourceArrayDwOffset);
	}
	if (table->rwResourcePtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, shaderStage, table->rwResourcePtrSgpr, resourceBufferFlushedAddress + table->rwResourceArrayDwOffset);
	}
	if (table->samplerPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegister(m_dcb, shaderStage, table->samplerPtrSgpr, resourceBufferFlushedAddress + table->samplerArrayDwOffset);
	}
	if (table->globalInternalPtrSgpr != 0xFF)
	{
		SCE_GNM_ASSERT(m_globalInternalResourceTableAddr != NULL);
		setPtrInPersistentRegister(m_dcb, shaderStage, table->globalInternalPtrSgpr, m_globalInternalResourceTableAddr);
	}
	if (table->appendConsumeCounterSgpr != 0xFF)
	{
		setPersistentRegisterRange(m_dcb, shaderStage, table->appendConsumeCounterSgpr, &m_boundShaderAppendConsumeCounterRange[shaderStage], 1);
	}

}

/** @brief The Embedded Constant Buffer (ECB) descriptor is automatically generated and bound for shaders that have dependencies
 *	An ECB is generated when global static arrays are accessed through dynamic variables, preventing the compiler from embedding its immediate values in the shader code.
 *  The ECB is stored after the end of the shader code area, and is expected on API-slot 15.
 */
SCE_GNM_FORCE_INLINE void GraphicsConstantUpdateEngine::updateEmbeddedCb(sce::Gnm::ShaderStage shaderStage, const Gnmx::ShaderCommonData* shaderCommon)
{
	if (shaderCommon != NULL && shaderCommon->m_embeddedConstantBufferSizeInDQW > 0)
	{
		const uint32_t* shaderRegisters = (const uint32_t*)(shaderCommon + 1);
		const uint8_t* shaderCode = (uint8_t*)(((uintptr_t)shaderRegisters[0] << 8ULL) | ((uintptr_t)shaderRegisters[1] << 40ULL));

		Gnm::Buffer embeddedCb;
		embeddedCb.initAsConstantBuffer((void*)(shaderCode + shaderCommon->m_shaderSize), shaderCommon->m_embeddedConstantBufferSizeInDQW << 4);
		setConstantBuffers(shaderStage, LightweightConstantUpdateEngine::kConstantBufferInternalApiSlotForEmbeddedData, 1, &embeddedCb);
	}
}


void GraphicsConstantUpdateEngine::preDispatch()
{
	SCE_GNM_ASSERT(m_dcb != NULL);
	
	const Gnmx::CsShader* csShader = ((const Gnmx::CsShader*)m_boundShader[Gnm::kShaderStageCs]);
	if (m_dirtyShader[Gnm::kShaderStageCs])
		m_dcb->setCsShader( &csShader->m_csStageRegisters );

	// Handle embedded Constant Buffer on CB slot 15
	if (m_dirtyShader[Gnm::kShaderStageCs] || m_dirtyShaderResources[Gnm::kShaderStageCs])
		updateEmbeddedCb(Gnm::kShaderStageCs, (const Gnmx::ShaderCommonData*)csShader);

	if (m_dirtyShaderResources[Gnm::kShaderStageCs])
	{
		SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageCs]);

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageCs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageCs, flushAddress);
	}

	m_dirtyShaderResources[Gnm::kShaderStageCs] = false;
	m_dirtyShader[Gnm::kShaderStageCs] = false;
}


SCE_GNM_FORCE_INLINE void GraphicsConstantUpdateEngine::preDrawTessellation(bool geometryShaderEnabled)
{
	SCE_GNM_UNUSED(geometryShaderEnabled);

 	const Gnmx::LsShader* lsShader = (const Gnmx::LsShader*)m_boundShader[Gnm::kShaderStageLs];
 	const Gnmx::HsShader* hsShader = (const Gnmx::HsShader*)m_boundShader[Gnm::kShaderStageHs];
	SCE_GNM_ASSERT(lsShader != NULL);
	SCE_GNM_ASSERT(hsShader != NULL);

	// if this is 0, its a bug in the PSSL compiler, MAX_TESS_FACTOR was not defined!
	SCE_GNM_ASSERT(hsShader->m_hsStageRegisters.m_vgtHosMaxTessLevel != 0);

	// if enabled, LCUE will use space in the main buffer, and set the requires tessellation constant buffers itself.
	if (m_tessellationAutoManageReservedCb)	// Note: enabled by default in GNMX, if user chooses to manage and set the buffers themselves, make sure to disable
	{
		// LS or HS shader has been marked dirty, so reset the tessellation data constant buffers
		if (m_dirtyShader[Gnm::kShaderStageLs] || m_dirtyShader[Gnm::kShaderStageHs])
		{
			// Check that we have enough space in the main buffer
			SCE_GNM_ASSERT( sizeof(Gnm::TessellationDataConstantBuffer) % 4 == 0);
			SCE_GNM_ASSERT( (m_bufferCurrent + sizeof(Gnm::TessellationDataConstantBuffer) / 4) < m_bufferEnd[m_bufferIndex] );

			// Generate internal TessellationDataConstantBuffer using some space from the main buffer
			Gnm::TessellationDataConstantBuffer* tessellationInternalConstantBuffer = (Gnm::TessellationDataConstantBuffer*)m_bufferCurrent;
			m_bufferCurrent += sizeof(Gnm::TessellationDataConstantBuffer) / 4;
			tessellationInternalConstantBuffer->init(&hsShader->m_hullStateConstants, lsShader->m_lsStride, m_tessellationCurrentTgPatchCount);

			// Update tessellation internal CB on API-slot 19
			m_tessellationCurrentCb.initAsConstantBuffer(tessellationInternalConstantBuffer, sizeof(Gnm::TessellationDataConstantBuffer));
			m_tessellationCurrentCb.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // tessellation constant buffer, is read-only
		}

		// update hull shader and domain shader (domain runs on either ES or VS stage depending on if geometry shading is enabled)
		setConstantBuffers(Gnm::kShaderStageHs, kConstantBufferInternalApiSlotForTessellation, 1, &m_tessellationCurrentCb);
		setConstantBuffers((geometryShaderEnabled)? Gnm::kShaderStageEs : Gnm::kShaderStageVs, kConstantBufferInternalApiSlotForTessellation, 1, &m_tessellationCurrentCb);
	}

	if (m_dirtyShader[Gnm::kShaderStageLs])
		m_dcb->setLsShader(&lsShader->m_lsStageRegisters, m_boundShaderModifier[Gnm::kShaderStageLs]);

	if (m_dirtyShader[Gnm::kShaderStageHs])
	{
		Gnm::TessellationRegisters tessellationVgtLsHsConfiguration;
		tessellationVgtLsHsConfiguration.init( &hsShader->m_hullStateConstants, m_tessellationCurrentTgPatchCount);
		m_dcb->setHsShader(&hsShader->m_hsStageRegisters, &tessellationVgtLsHsConfiguration);
	}

	// Handle embedded Constant Buffer on CB slot 15
	if (m_dirtyShader[Gnm::kShaderStageLs] || m_dirtyShaderResources[Gnm::kShaderStageLs])
		updateEmbeddedCb(Gnm::kShaderStageLs, (const Gnmx::ShaderCommonData*)lsShader);
	if (m_dirtyShader[Gnm::kShaderStageHs] || m_dirtyShaderResources[Gnm::kShaderStageHs])
		updateEmbeddedCb(Gnm::kShaderStageHs, (const Gnmx::ShaderCommonData*)hsShader);

	if (m_dirtyShaderResources[Gnm::kShaderStageLs])
	{
		// Validate after handling embedded CBs (if necessary) and before flushing scratch buffer
		SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageLs]);

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageLs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageLs, flushAddress);
		updateLsEsVsPtrsInUserDataSgprs(Gnm::kShaderStageLs, flushAddress);
	}
	if (m_dirtyShaderResources[Gnm::kShaderStageHs])
	{
		// Validate after handling embedded CBs (if necessary) and before flushing scratch buffer
		SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageHs]);

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageHs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageHs, flushAddress);
	}
}


SCE_GNM_FORCE_INLINE void GraphicsConstantUpdateEngine::preDrawGeometryShader()
{
	const Gnmx::EsShader* esShader = (const Gnmx::EsShader*)m_boundShader[Gnm::kShaderStageEs];
	const Gnmx::GsShader* gsShader = (const Gnmx::GsShader*)m_boundShader[Gnm::kShaderStageGs];

	if (m_dirtyShader[Gnm::kShaderStageEs])
		m_dcb->setEsShader(&esShader->m_esStageRegisters, m_boundShaderModifier[Gnm::kShaderStageEs]);
	if (m_dirtyShader[Gnm::kShaderStageGs])
		m_dcb->setGsShader(&gsShader->m_gsStageRegisters);

	// Handle embedded Constant Buffer on CB slot 15
	if (m_dirtyShader[Gnm::kShaderStageEs] || m_dirtyShaderResources[Gnm::kShaderStageEs])
		updateEmbeddedCb(Gnm::kShaderStageEs, (const Gnmx::ShaderCommonData*)esShader);
	if (m_dirtyShader[Gnm::kShaderStageGs] || m_dirtyShaderResources[Gnm::kShaderStageGs])
		updateEmbeddedCb(Gnm::kShaderStageGs, (const Gnmx::ShaderCommonData*)gsShader);

	if (m_dirtyShaderResources[Gnm::kShaderStageEs])
	{
		// Validate after handling embedded CBs (if necessary) and before flushing scratch buffer
		SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageEs]);

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageEs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageEs, flushAddress);
		updateLsEsVsPtrsInUserDataSgprs(Gnm::kShaderStageEs, flushAddress);
	}
	if (m_dirtyShaderResources[Gnm::kShaderStageGs])
	{
		// Validate after handling embedded CBs (if necessary) and before flushing scratch buffer
		SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageGs]);

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageGs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageGs, flushAddress);
	}
}


void GraphicsConstantUpdateEngine::preDraw()
{
	SCE_GNM_ASSERT( m_dcb != NULL );
	SCE_GNM_ASSERT( m_boundShader[Gnm::kShaderStageVs] != NULL); // VS cannot be NULL for any pipeline configuration

	bool tessellationEnabled = false;
	bool geometryShaderEnabled = false;

	geometryShaderEnabled = (m_activeShaderStages == Gnm::kActiveShaderStagesEsGsVsPs) | (m_activeShaderStages == Gnm::kActiveShaderStagesLsHsEsGsVsPs);
	if (geometryShaderEnabled)
		preDrawGeometryShader();

	tessellationEnabled = (m_activeShaderStages == Gnm::kActiveShaderStagesLsHsVsPs) | (m_activeShaderStages == Gnm::kActiveShaderStagesLsHsEsGsVsPs);
	if (tessellationEnabled)
		preDrawTessellation(geometryShaderEnabled);
	
	const Gnmx::VsShader* vsShader = ((const Gnmx::VsShader*)m_boundShader[Gnm::kShaderStageVs]);
	const Gnmx::PsShader* psShader = ((const Gnmx::PsShader*)m_boundShader[Gnm::kShaderStagePs]);

	if (m_dirtyShader[Gnm::kShaderStageVs])
		m_dcb->setVsShader(&vsShader->m_vsStageRegisters, m_boundShaderModifier[Gnm::kShaderStageVs]);
	if (m_dirtyShader[Gnm::kShaderStagePs])
		m_dcb->setPsShader((psShader != NULL)? &psShader->m_psStageRegisters : NULL);
	
	// Handle embedded Constant Buffer on CB slot 15
	if (m_dirtyShader[Gnm::kShaderStageVs] || m_dirtyShaderResources[Gnm::kShaderStageVs])
		updateEmbeddedCb(Gnm::kShaderStageVs, (const Gnmx::ShaderCommonData*)vsShader);
	if (m_dirtyShader[Gnm::kShaderStagePs] || m_dirtyShaderResources[Gnm::kShaderStagePs])
		updateEmbeddedCb(Gnm::kShaderStagePs, (const Gnmx::ShaderCommonData*)psShader);

	if (m_dirtyShaderResources[Gnm::kShaderStageVs])
	{
		// Validate after handling embedded CBs (if necessary) and before flushing scratch buffer
		SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageVs]);

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageVs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageVs, flushAddress);
		updateLsEsVsPtrsInUserDataSgprs(Gnm::kShaderStageVs, flushAddress);
	}
	if (m_dirtyShaderResources[Gnm::kShaderStagePs])
	{
		// Validate after handling embedded CBs (if necessary) and before flushing scratch buffer
		SCE_GNM_LCUE_VALIDATE_RESOURCE_CHECK_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStagePs]);

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStagePs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStagePs, flushAddress);
	}

// This can be enabled to avoid regenerating and resetting the pixel usage table every time
#if !defined(SCE_GNM_LCUE_SKIP_VS_PS_SEMANTIC_TABLE)
	if (m_dirtyShader[Gnm::kShaderStageVs] || m_dirtyShader[Gnm::kShaderStagePs])
	{
		uint32_t psInputs[32];
		if (psShader != NULL && psShader->m_numInputSemantics != 0)
		{
			Gnm::generatePsShaderUsageTable(psInputs,
				vsShader->getExportSemanticTable(), vsShader->m_numExportSemantics,
				psShader->getPixelInputSemanticTable(), psShader->m_numInputSemantics);
			m_dcb->setPsShaderUsage(psInputs, psShader->m_numInputSemantics);
		}
	}
#endif

	__builtin_memset(&m_dirtyShader[Gnm::kShaderStagePs], 0, (Gnm::kShaderStageCount-1) * sizeof(bool));
	__builtin_memset(&m_dirtyShaderResources[Gnm::kShaderStagePs], 0, (Gnm::kShaderStageCount-1) * sizeof(bool));
}


void GraphicsConstantUpdateEngine::setEsShader(const sce::Gnmx::EsShader* shader, uint32_t shaderModifier, const void* fetchShader, const InputResourceOffsets* table)
{
	SCE_GNM_ASSERT( shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageEs);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageEs], table );

	m_dirtyShaderResources[Gnm::kShaderStageEs] = true;
	m_dirtyShader[Gnm::kShaderStageEs] |= (m_boundShader[Gnm::kShaderStageEs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStageEs] = table;
	m_boundShader[Gnm::kShaderStageEs] = shader;
	m_boundFetchShader[Gnm::kShaderStageEs] = fetchShader;
	m_boundShaderModifier[Gnm::kShaderStageEs] = shaderModifier;
}


void GraphicsConstantUpdateEngine::setEsFetchShader(uint32_t shaderModifier, const void* fetchShader)
{
	m_boundShaderModifier[Gnm::kShaderStageEs] = shaderModifier;
	m_boundFetchShader[Gnm::kShaderStageEs] = fetchShader;
}


void GraphicsConstantUpdateEngine::setGsVsShaders(const sce::Gnmx::GsShader* shader, const InputResourceOffsets* gsTable)
{
	SCE_GNM_ASSERT(shader != NULL && gsTable != NULL && gsTable->shaderStage == Gnm::kShaderStageGs);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageGs], gsTable);

	m_dirtyShaderResources[Gnm::kShaderStageGs] = true;
	m_dirtyShader[Gnm::kShaderStageGs] |= (m_boundShader[Gnm::kShaderStageGs] != shader);
	m_boundShaderResourceOffsets[Gnm::kShaderStageGs] = gsTable;
	m_boundShader[Gnm::kShaderStageGs] = shader;

	const sce::Gnmx::VsShader* vsShader = shader->getCopyShader();
	const InputResourceOffsets* vsShaderResourceOffsets = (vsShader->m_common.m_numInputUsageSlots == 1)? &m_fixedGsVsShaderResourceOffsets : &m_fixedGsVsStreamOutShaderResourceOffsets;
	setVsShader(vsShader, vsShaderResourceOffsets);

	// This rolls the hardware context, thus we should only set it if it's really necessary
	SCE_GNM_ASSERT((Gnm::GsMaxOutputVertexCount)shader->getCopyShader()->m_gsModeOrNumInputSemanticsCs <= Gnm::kGsMaxOutputVertexCount128);
	Gnm::GsMaxOutputVertexCount requiredGsMaxOutput = (Gnm::GsMaxOutputVertexCount)shader->getCopyShader()->m_gsModeOrNumInputSemanticsCs;
	if(m_gsMode != Gnm::kGsModeEnable || requiredGsMaxOutput != m_gsMaxOutput)
	{
		m_dcb->setGsMode(Gnm::kGsModeEnable, requiredGsMaxOutput);
	}

	m_gsMode = Gnm::kGsModeEnable;
	m_gsMaxOutput = requiredGsMaxOutput;
}


void GraphicsConstantUpdateEngine::setGsVsShadersOnChip(const sce::Gnmx::GsShader* shader, const InputResourceOffsets* gsTable)
{
	SCE_GNM_ASSERT(shader != NULL && gsTable != NULL && gsTable->shaderStage == Gnm::kShaderStageGs);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageGs], gsTable);

	m_dirtyShaderResources[Gnm::kShaderStageGs] = true;
	m_dirtyShader[Gnm::kShaderStageGs] |= (m_boundShader[Gnm::kShaderStageGs] != shader);
	m_boundShaderResourceOffsets[Gnm::kShaderStageGs] = gsTable;
	m_boundShader[Gnm::kShaderStageGs] = shader;

	const sce::Gnmx::VsShader* vsShader = shader->getCopyShader();
	const InputResourceOffsets* vsShaderResourceOffsets = (vsShader->m_common.m_numInputUsageSlots == 1)? &m_fixedGsVsShaderResourceOffsets : &m_fixedGsVsStreamOutShaderResourceOffsets;
	setVsShader(vsShader, vsShaderResourceOffsets);

	// This rolls the hardware context, thus we should only set it if it's really necessary
	SCE_GNM_ASSERT((Gnm::GsMaxOutputVertexCount)shader->getCopyShader()->m_gsModeOrNumInputSemanticsCs <= Gnm::kGsMaxOutputVertexCount128);
	Gnm::GsMaxOutputVertexCount requiredGsMaxOutput = (Gnm::GsMaxOutputVertexCount)shader->getCopyShader()->m_gsModeOrNumInputSemanticsCs;
	if(m_gsMode != Gnm::kGsModeEnableOnChip || requiredGsMaxOutput != m_gsMaxOutput)
	{
		m_dcb->setGsMode(Gnm::kGsModeEnableOnChip, requiredGsMaxOutput);
	}

	m_gsMode = Gnm::kGsModeEnableOnChip;
	m_gsMaxOutput = requiredGsMaxOutput;
}


void GraphicsConstantUpdateEngine::setStreamoutBuffers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxStreamOutBufferCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)Gnm::kShaderStageVs] != NULL && buffer != NULL);
	//SCE_GNM_LCUE_VALIDATE_RESOURCE_MEMORY_MAPPED(buffer, SCE_KERNEL_PROT_GPU_READ|SCE_KERNEL_PROT_GPU_WRITE);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)Gnm::kShaderStageVs];
	for (int32_t i=0; i<apiSlotCount; ++i)
	{
		SCE_GNM_ASSERT((buffer+i)->isBuffer());
		int32_t currentApiSlot = startApiSlot+i;

		SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)Gnm::kShaderStageVs]->streamOutPtrSgpr != 0xFF);
		if (m_boundShaderResourceOffsets[(int32_t)Gnm::kShaderStageVs]->streamOutPtrSgpr != 0xFF)
		{
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, Gnm::kShaderStageVs, table->streamOutDwOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
		}
	}

	m_dirtyShaderResources[(int32_t)Gnm::kShaderStageVs] = true;
}


void GraphicsConstantUpdateEngine::setLsShader(const Gnmx::LsShader* shader, uint32_t shaderModifier, const void* fetchShader, const InputResourceOffsets* table)
{
	SCE_GNM_ASSERT(shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageLs);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageLs], table);

	m_dirtyShaderResources[Gnm::kShaderStageLs] = true;
	m_dirtyShader[Gnm::kShaderStageLs] |= (m_boundShader[Gnm::kShaderStageLs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStageLs] = table;
	m_boundShader[Gnm::kShaderStageLs] = shader;
	m_boundFetchShader[Gnm::kShaderStageLs] = fetchShader;
	m_boundShaderModifier[Gnm::kShaderStageLs] = shaderModifier;
}


void GraphicsConstantUpdateEngine::setLsFetchShader(uint32_t shaderModifier, const void* fetchShader)
{
	m_boundShaderModifier[Gnm::kShaderStageLs] = shaderModifier;
	m_boundFetchShader[Gnm::kShaderStageLs] = fetchShader;
}


void GraphicsConstantUpdateEngine::setHsShader(const Gnmx::HsShader* shader, const InputResourceOffsets* table, uint32_t tgPatchCount)
{
	SCE_GNM_ASSERT(shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageHs);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageHs], table);

	m_dirtyShaderResources[Gnm::kShaderStageHs] = true;
	m_dirtyShader[Gnm::kShaderStageHs] |= (m_boundShader[Gnm::kShaderStageHs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStageHs] = table;
	m_boundShader[Gnm::kShaderStageHs] = shader;
	m_tessellationCurrentTgPatchCount = tgPatchCount;
}


void GraphicsConstantUpdateEngine::setVsShader(const Gnmx::VsShader* shader, uint32_t shaderModifier, const void* fetchShader, const InputResourceOffsets* table)
{
	SCE_GNM_ASSERT(shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageVs);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageVs], table);

	m_dirtyShaderResources[Gnm::kShaderStageVs] = true;
	m_dirtyShader[Gnm::kShaderStageVs] |= (m_boundShader[Gnm::kShaderStageVs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStageVs] = table;
	m_boundShader[Gnm::kShaderStageVs] = shader;
	m_boundFetchShader[Gnm::kShaderStageVs] = fetchShader;
	m_boundShaderModifier[Gnm::kShaderStageVs] = shaderModifier;
}


void GraphicsConstantUpdateEngine::setVsFetchShader(uint32_t shaderModifier, const void* fetchShader)
{
	m_boundShaderModifier[Gnm::kShaderStageVs] = shaderModifier;
	m_boundFetchShader[Gnm::kShaderStageVs] = fetchShader;
}


void GraphicsConstantUpdateEngine::setPsShader(const Gnmx::PsShader* shader, const InputResourceOffsets* table)
{
	SCE_GNM_ASSERT((shader == NULL && table == NULL) || (shader != NULL && table != NULL));
	SCE_GNM_ASSERT(table == NULL || table->shaderStage == Gnm::kShaderStagePs);
	SCE_GNM_ASSERT(table == NULL || (m_bufferCurrent + table->requiredBufferSizeInDwords) < m_bufferEnd[m_bufferIndex]);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStagePs], table);

	// Special case: if the Pixel Shader is NULL we don't mark shaderResourceOffsets as dirty to prevent flushing the scratch buffer
	m_dirtyShaderResources[Gnm::kShaderStagePs] = (shader != NULL); 
	m_dirtyShader[Gnm::kShaderStagePs] |= (m_boundShader[Gnm::kShaderStagePs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStagePs] = table;
	m_boundShader[Gnm::kShaderStagePs] = shader;
}


void GraphicsConstantUpdateEngine::setCsShader(const Gnmx::CsShader* shader, const InputResourceOffsets* table)
{
	SCE_GNM_ASSERT(shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageCs);
	SCE_GNM_ASSERT((m_bufferCurrent + table->requiredBufferSizeInDwords) < m_bufferEnd[m_bufferIndex]);
	SCE_GNM_LCUE_VALIDATE_RESOURCE_INIT_TABLE(&m_boundShaderResourcesValidation[Gnm::kShaderStageCs], table);

	m_dirtyShaderResources[Gnm::kShaderStageCs] = true;
	m_dirtyShader[Gnm::kShaderStageCs] |= (m_boundShader[Gnm::kShaderStageCs] != shader);
	
	m_boundShaderResourceOffsets[Gnm::kShaderStageCs] = table;
	m_boundShader[Gnm::kShaderStageCs] = shader;
}



void GraphicsConstantUpdateEngine::setAppendConsumeCounterRange(Gnm::ShaderStage shaderStage, uint32_t gdsMemoryBaseInBytes, uint32_t countersSizeInBytes)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL);
	SCE_GNM_ASSERT((gdsMemoryBaseInBytes%4)==0 && gdsMemoryBaseInBytes < UINT16_MAX && (countersSizeInBytes%4)==0 && countersSizeInBytes < UINT16_MAX);
	
	SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets[(int32_t)shaderStage]->appendConsumeCounterSgpr != 0xFF);
	if (m_boundShaderResourceOffsets[(int32_t)shaderStage]->appendConsumeCounterSgpr != 0xFF)
	{
		SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].appendConsumeCounterIsBound);
		m_boundShaderAppendConsumeCounterRange[shaderStage] = (gdsMemoryBaseInBytes << 16) | countersSizeInBytes;
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
}


void GraphicsConstantUpdateEngine::setConstantBuffers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxConstantBufferCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_CONSTANT_BUFFER(buffer+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets[(int32_t)shaderStage]->constBufferDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets[(int32_t)shaderStage]->constBufferDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].constBufferOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, shaderStage, table->constBufferDwOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
		}
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
}


void GraphicsConstantUpdateEngine::setVertexBuffers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxVertexBufferCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; ++i)
	{
		SCE_GNM_LCUE_VALIDATE_VERTEX_BUFFER(buffer+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets[(int32_t)shaderStage]->vertexBufferDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets[(int32_t)shaderStage]->vertexBufferDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].vertexBufferOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, shaderStage, table->vertexBufferDwOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
		}
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
}


void GraphicsConstantUpdateEngine::setBuffers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxResourceCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_BUFFER(buffer+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].resourceOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, shaderStage, table->resourceDwOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
		}
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
}


void GraphicsConstantUpdateEngine::setRwBuffers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Buffer* buffer)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxRwResourceCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);
	SCE_GNM_ASSERT(buffer->getResourceMemoryType() != Gnm::kResourceMemoryTypeRO);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_RWBUFFER(buffer+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].rwResourceOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, shaderStage, table->rwResourceDwOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
		}
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
}


void GraphicsConstantUpdateEngine::setTextures(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Texture* texture)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxResourceCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && texture != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_TEXTURE(texture+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].resourceOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, shaderStage, table->resourceDwOffset[currentApiSlot], texture+i, sizeof(Gnm::Texture));
		}
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
}


void GraphicsConstantUpdateEngine::setRwTextures(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Texture* texture)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxRwResourceCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && texture != NULL);
	SCE_GNM_ASSERT(texture->getResourceMemoryType() != Gnm::kResourceMemoryTypeRO);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_RWTEXTURE(texture+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].rwResourceOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, shaderStage, table->rwResourceDwOffset[currentApiSlot], texture+i, sizeof(Gnm::Texture));
		}
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
}


void GraphicsConstantUpdateEngine::setSamplers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, const Gnm::Sampler* sampler)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxSamplerCount);
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && sampler != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		SCE_GNM_LCUE_VALIDATE_SAMPLER(sampler+i);

		int32_t currentApiSlot = startApiSlot+i;
		SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING(m_boundShaderResourceOffsets[(int32_t)shaderStage]->samplerDwOffset[currentApiSlot] != 0xFFFF);
		if (m_boundShaderResourceOffsets[(int32_t)shaderStage]->samplerDwOffset[currentApiSlot] != 0xFFFF)
		{
			SCE_GNM_LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].samplerOffsetIsBound[currentApiSlot]);
			setDataInUserDataSgprOrMemory(m_dcb, m_scratchBuffer, shaderStage, table->samplerDwOffset[currentApiSlot], sampler+i, sizeof(Gnm::Sampler));
		}
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
}


void GraphicsConstantUpdateEngine::setUserData(sce::Gnm::ShaderStage shaderStage, int32_t startSgpr, int32_t sgprCount, const uint32_t* data)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startSgpr >= 0 && (startSgpr+sgprCount) <= kMaxUserDataCount);
	setPersistentRegisterRange(m_dcb, shaderStage, startSgpr, data, sgprCount);
}


void GraphicsConstantUpdateEngine::setPtrInUserData(sce::Gnm::ShaderStage shaderStage, int32_t startSgpr, const void* gpuAddress)
{
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && startSgpr >= 0 && (startSgpr) <= kMaxUserDataCount);
	setPtrInPersistentRegister(m_dcb, shaderStage, startSgpr, gpuAddress);
}


void GraphicsConstantUpdateEngine::setUserSrtBuffer(sce::Gnm::ShaderStage shaderStage, const void* buffer, uint32_t sizeInDwords)
{
	SCE_GNM_UNUSED(sizeInDwords);
	SCE_GNM_ASSERT(shaderStage < Gnm::kShaderStageCount && (sizeInDwords > 0 && sizeInDwords <= kMaxSrtUserDataCount));
	SCE_GNM_ASSERT(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);

	const InputResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	SCE_GNM_ASSERT(sizeInDwords == table->userSrtDataCount);
	setUserData(shaderStage, table->userSrtDataSgpr, table->userSrtDataCount, (const uint32_t *)buffer);
}


void GraphicsContext::init(uint32_t* dcbBuffer, int32_t dcbBufferSizeInDwords, 
								 uint32_t* resourceBufferInGarlic, int32_t resourceBufferSizeInDwords, 
								 uint32_t* globalInternalResourceTableAddr, 
								 Gnm::CommandCallbackFunc callbackFunc, void* callbackUserData)
{
	SCE_GNM_ASSERT(dcbBuffer != NULL);
	SCE_GNM_ASSERT(dcbBufferSizeInDwords > 0 && (dcbBufferSizeInDwords * sizeof(uint32_t)) <= Gnm::kIndirectBufferMaximumSizeInBytes);

	m_dcb.init(dcbBuffer, dcbBufferSizeInDwords * sizeof(uint32_t), callbackFunc, callbackUserData);

	GraphicsConstantUpdateEngine::init(&resourceBufferInGarlic, 1, resourceBufferSizeInDwords, globalInternalResourceTableAddr);
	GraphicsConstantUpdateEngine::setDrawCommandBuffer(&m_dcb);
	
#if defined (SCE_GNM_LCUE_CLEAR_HARDWARE_KCACHE)
	// first order of business, invalidate the KCache/L1/L2 to rid us of any stale data
	m_dcb.flushShaderCachesAndWait(Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionInvalidateKCache, Gnm::kStallCommandBufferParserDisable);
#endif
}


void GraphicsContext::setEsGsRingBuffer(void* ringBuffer, uint32_t ringSizeInBytes, uint32_t maxExportVertexSizeInDword)
{
	Gnm::Buffer ringReadDescriptor;
	Gnm::Buffer ringWriteDescriptor;
	ringReadDescriptor.initAsEsGsReadDescriptor(ringBuffer, ringSizeInBytes);
	ringWriteDescriptor.initAsEsGsWriteDescriptor(ringBuffer, ringSizeInBytes);

	setGlobalInternalResource(Gnm::kShaderGlobalResourceEsGsReadDescriptor, &ringReadDescriptor);
	setGlobalInternalResource(Gnm::kShaderGlobalResourceEsGsWriteDescriptor, &ringWriteDescriptor);

	m_dcb.setupEsGsRingRegisters(maxExportVertexSizeInDword);
}


void GraphicsContext::setGsVsRingBuffers(void* ringBuffer, uint32_t ringSizeInBytes, const uint32_t vertexSizePerStreamInDword[4], uint32_t maxOutputVertexCount)
{
	Gnm::Buffer ringReadDescriptor;
	Gnm::Buffer ringWriteDescriptor[4];

	ringReadDescriptor.initAsGsVsReadDescriptor(ringBuffer, ringSizeInBytes);
	ringWriteDescriptor[0].initAsGsVsWriteDescriptor(ringBuffer, 0, vertexSizePerStreamInDword, maxOutputVertexCount);
	ringWriteDescriptor[1].initAsGsVsWriteDescriptor(ringBuffer, 1, vertexSizePerStreamInDword, maxOutputVertexCount);
	ringWriteDescriptor[2].initAsGsVsWriteDescriptor(ringBuffer, 2, vertexSizePerStreamInDword, maxOutputVertexCount);
	ringWriteDescriptor[3].initAsGsVsWriteDescriptor(ringBuffer, 3, vertexSizePerStreamInDword, maxOutputVertexCount);

	setGlobalInternalResource(Gnm::kShaderGlobalResourceGsVsReadDescriptor, &ringReadDescriptor);
	setGlobalInternalResource(Gnm::kShaderGlobalResourceGsVsWriteDescriptor0, &ringWriteDescriptor[0]);
	setGlobalInternalResource(Gnm::kShaderGlobalResourceGsVsWriteDescriptor1, &ringWriteDescriptor[1]);
	setGlobalInternalResource(Gnm::kShaderGlobalResourceGsVsWriteDescriptor2, &ringWriteDescriptor[2]);
	setGlobalInternalResource(Gnm::kShaderGlobalResourceGsVsWriteDescriptor3, &ringWriteDescriptor[3]);

	m_dcb.setupGsVsRingRegisters(vertexSizePerStreamInDword, maxOutputVertexCount);
}


void LightweightConstantUpdateEngine::computeTessellationTgParameters(uint32_t *outVgtPrimitiveCount, uint32_t* outTgPatchCount, uint32_t desiredTgLdsSizeInBytes, uint32_t desiredTgPatchCount, const Gnmx::LsShader* localShader, const Gnmx::HsShader* hullShader)
{
	uint32_t vgtPrimitiveCount = 0;
	uint32_t tgPatchCount = 0; 
	uint32_t tgLdsSizeInBytes = 0; 

	const uint32_t kMaxTGLdsSizeInBytes = 32*1024;	// max allocation of LDS a thread group can make.
	SCE_GNM_ASSERT(desiredTgLdsSizeInBytes <= kMaxTGLdsSizeInBytes); SCE_GNM_UNUSED(kMaxTGLdsSizeInBytes);

	// calculate the minimum LDS required
	tgLdsSizeInBytes = computeLdsUsagePerPatchInBytesPerThreadGroup(&hullShader->m_hullStateConstants, localShader->m_lsStride);
	SCE_GNM_ASSERT(tgLdsSizeInBytes <= kMaxTGLdsSizeInBytes);
	tgLdsSizeInBytes = SCE_GNM_MAX(tgLdsSizeInBytes, desiredTgLdsSizeInBytes); // use desired value if the user decided to use more than the minimum

	// calculate the VGT primitive count and the minimum patch count for tessellation
	uint32_t maxVgprCount = hullShader->m_hsStageRegisters.getNumVgprs();
	SCE_GNM_ASSERT(maxVgprCount%4==0);

	Gnmx::computeVgtPrimitiveAndPatchCounts(&vgtPrimitiveCount, &tgPatchCount, maxVgprCount, tgLdsSizeInBytes, localShader, hullShader);
	tgPatchCount = SCE_GNM_MAX(tgPatchCount, desiredTgPatchCount); // use which ever patch count is the maximum

	// write out the VGT and Patch count
	*outVgtPrimitiveCount	= vgtPrimitiveCount;
	*outTgPatchCount		= tgPatchCount;
}


int32_t getUsedApiSlotsFromMask(int32_t* outUsedApiSlots, int32_t usedApiSlotsCount, uint32_t mask[4], int32_t maxResourceCount)
{
	SCE_GNM_UNUSED(maxResourceCount);

	int32_t resourceCount = 0;
	for (int32_t slot = 0; slot < usedApiSlotsCount; ++slot)
		if (mask[slot>>5] & (1<<(slot & 0x1F)))
			outUsedApiSlots[resourceCount++] = slot;

#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	// Check if the shader uses any API-slot over the maximum count configured to be handled by the LCUE
	for (int32_t slot = usedApiSlotsCount; slot < maxResourceCount; ++slot)
		SCE_GNM_ASSERT( (mask[slot>>5] & (1<<(slot & 0x1F))) == 0);
#endif

	return resourceCount;
}


int32_t getUsedApiSlotsFromMask(int32_t* outUsedApiSlots, int32_t usedApiSlotsCount, uint32_t mask, int32_t maxResourceCount)
{
	SCE_GNM_UNUSED(maxResourceCount);

	int32_t resourceCount = 0;
	for (int32_t slot = 0; slot < usedApiSlotsCount; ++slot)
		if ( ((1<<slot) & mask))
			outUsedApiSlots[resourceCount++] = slot;

#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	// Check if the shader uses any API-slot over the maximum count configured to be handled by the LCUE
	for (int32_t slot = usedApiSlotsCount; slot < maxResourceCount; ++slot)
		SCE_GNM_ASSERT( ((1<<slot) & mask) == 0);
#endif

	return resourceCount;
}


int32_t LightweightConstantUpdateEngine::patchInputResourceOffsetTableWithSemanticTable(InputResourceOffsets* outTable, const InputResourceOffsets* table, const int32_t* semanticRemapTable, int32_t semanticRemapTableSizeInItems)
{
	SCE_GNM_ASSERT(outTable != NULL && table != NULL);
	SCE_GNM_ASSERT(semanticRemapTable != NULL && semanticRemapTableSizeInItems >= 0 && semanticRemapTableSizeInItems < kMaxVertexBufferCount);

	// Validate remap table
#if defined(GNM_DEBUG)
	bool remapUsed[LightweightConstantUpdateEngine::kMaxVertexBufferCount];
	memset(remapUsed, 0, LightweightConstantUpdateEngine::kMaxVertexBufferCount * sizeof(bool));
	for (int32_t i=0; i<semanticRemapTableSizeInItems; ++i)
	{
		int32_t remapIndex = semanticRemapTable[i];
		if (remapIndex == -1)
			continue;

		SCE_GNM_ASSERT(!remapUsed[remapIndex]);
		remapUsed[remapIndex] = true;
	}
#endif

	InputResourceOffsets resultTable;
	memcpy(&resultTable, table, sizeof(InputResourceOffsets));

	// Patch
	int32_t remapCountDiff = 0;
	for (int32_t i=0; i<semanticRemapTableSizeInItems; ++i)
	{
		int32_t remapIndex = semanticRemapTable[i];
		if (remapIndex == -1)
		{
			// Decrease VB input count
			if (table->vertexBufferDwOffset[i] != 0xFFFF)
				remapCountDiff--;

			resultTable.vertexBufferDwOffset[i] = 0xFFFF;
		}
		else
		{
			// Increased VB input count
			if (table->vertexBufferDwOffset[i] == 0xFFFF)
				remapCountDiff++;

			// Make sure it remaps to a valid VB
			SCE_GNM_ASSERT(table->vertexBufferDwOffset[remapIndex] != 0xFFFF);
			resultTable.vertexBufferDwOffset[i] = table->vertexBufferDwOffset[remapIndex];
		}
	}

	// If it's not zero you increased the number of input-VBs
	SCE_GNM_ASSERT(remapCountDiff == 0);
	memcpy(outTable, &resultTable, sizeof(InputResourceOffsets));

	return remapCountDiff;
}


void LightweightConstantUpdateEngine::generateInputResourceOffsetTable(InputResourceOffsets* outTable, Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct)
{
	// Get the shader pointer and its size from the GNMX shader type
	const Gnmx::ShaderCommonData* shaderCommonData = (const Gnmx::ShaderCommonData*)gnmxShaderStruct;
	const uint32_t* shaderRegisters = (const uint32_t*)(shaderCommonData+1);

	const void* shaderCode = (void*)( ((uintptr_t)shaderRegisters[0] << 8ULL) | ((uintptr_t)shaderRegisters[1] << 40ULL) );
	int32_t shaderCodeSize = ((const Gnmx::ShaderCommonData*)gnmxShaderStruct)->m_shaderSize;
	SCE_GNM_ASSERT(shaderCode != NULL && shaderCodeSize > 0);

	generateInputResourceOffsetTable(outTable, shaderStage, gnmxShaderStruct, shaderCode, shaderCodeSize, shaderCommonData->m_shaderIsUsingSrt);
}


void LightweightConstantUpdateEngine::generateInputResourceOffsetTable(InputResourceOffsets* outTable, Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct, const void* shaderCode, int32_t shaderCodeSizeInBytes, bool isSrtUsed)
{
	typedef struct
	{
		uint8_t			m_signature[7];				// 'OrbShdr'
		uint8_t			m_version;					// ShaderBinaryInfoVersion
		unsigned int	m_pssl_or_cg	: 1;		// 1 = PSSL / Cg, 0 = IL / shtb
		unsigned int	m_cached		: 1;		// 1 = when compile, debugging source was cached.  May only make sense for PSSL=1
		uint32_t		m_type			: 4;		// See enum ShaderBinaryType
		uint32_t		m_source_type	: 2;		// See enum ShaderSourceType
		unsigned int	m_length		: 24;		// Binary length not counting this structure (i.e. points to top of binary)
		uint8_t			m_chunkUsageBaseOffsetInDW;	// in DW
		uint8_t			m_numInputUsageSlots;		// Up to 16 usage slots + 48 extended user data. (note: max of 63 since the ptr to the ext user data takes 2 slots)
		uint8_t			m_reserved3[2];				// For future use
		uint32_t		m_shaderHash0;				// Association hash first 4 bytes
		uint32_t		m_shaderHash1;				// Association hash second 4 bytes
		uint32_t		m_crc32;					// crc32 of shader + this struct, just up till this field
	} ShaderBinaryInfo;

	SCE_GNM_ASSERT(outTable != NULL);
	SCE_GNM_ASSERT(shaderStage <= Gnm::kShaderStageCount);

	// Get resource info to populate ShaderResourceOffsets
	ShaderBinaryInfo const *shaderBinaryInfo = (ShaderBinaryInfo const*)((uintptr_t)shaderCode + shaderCodeSizeInBytes - sizeof(ShaderBinaryInfo));
	SCE_GNM_ASSERT( (*((uint64_t const*)shaderBinaryInfo->m_signature) & kShaderBinaryInfoSignatureMask) == kShaderBinaryInfoSignatureU64);
	
	// Get usage masks and input usage slots
	uint32_t const* usageMasks = (unsigned int const*)((unsigned char const*)shaderBinaryInfo - shaderBinaryInfo->m_chunkUsageBaseOffsetInDW*4);
	int32_t inputUsageSlotsCount = shaderBinaryInfo->m_numInputUsageSlots;
	Gnm::InputUsageSlot const* inputUsageSlots = (Gnm::InputUsageSlot const*)usageMasks - inputUsageSlotsCount;
	 
	// Cache shader input information into the ShaderResource Offsets table
	__builtin_memset(outTable, 0xFF, sizeof(InputResourceOffsets));
	outTable->shaderStage = shaderStage;
	outTable->isSrtShader = isSrtUsed;
	int32_t lastUserDataResourceSizeInDwords = 0;
	int32_t requiredMemorySizeInDwords = 0;

	// Here we handle all immediate resources s[1:16] plus s[16:48] (extended user data)
	// Resources that go into the extended user data also have "immediate" usage type, although they are stored in a table (not fetch by the SPI)
	for (int32_t i=0; i<inputUsageSlotsCount; ++i)
	{
		int32_t apiSlot = inputUsageSlots[i].m_apiSlot;
		int32_t startRegister = inputUsageSlots[i].m_startRegister;
		bool isVSharp = (inputUsageSlots[i].m_resourceType == 0);
		uint16_t vsharpFlag = (isVSharp)? kResourceIsVSharp : 0;

		uint16_t extendedRegisterOffsetInDwords = (startRegister >= 16)? (startRegister-16) : 0;
		requiredMemorySizeInDwords = (requiredMemorySizeInDwords > extendedRegisterOffsetInDwords)?
			requiredMemorySizeInDwords : extendedRegisterOffsetInDwords;

		// Handle immediate resources, including some pointer types
		switch (inputUsageSlots[i].m_usageType)
		{
		case Gnm::kShaderInputUsageImmGdsCounterRange:
			outTable->appendConsumeCounterSgpr = startRegister;
			break;

		//case Gnm::kShaderInputUsagePtrSoBufferTable: // Only present in the VS copy-shader that doesn't have a footer
		//	outTable->streamOutPtrSgpr = startRegister;
		//	break;

		case Gnm::kShaderInputUsageSubPtrFetchShader:
			SCE_GNM_ASSERT(inputUsageSlots[i].m_apiSlot == 0);
			outTable->fetchShaderPtrSgpr = startRegister;
			break;

		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			SCE_GNM_ASSERT(inputUsageSlots[i].m_apiSlot == 0);
			outTable->globalInternalPtrSgpr = startRegister;
			break;

		case Gnm::kShaderInputUsagePtrExtendedUserData:
			SCE_GNM_ASSERT(inputUsageSlots[i].m_apiSlot == 1);
			outTable->userExtendedData1PtrSgpr = startRegister;
			break;

		// below resources can either be inside UserData or the EUD
		case Gnm::kShaderInputUsageImmResource:
			SCE_GNM_ASSERT(apiSlot >= 0 && apiSlot < kMaxResourceCount);
			outTable->resourceDwOffset[apiSlot] = (startRegister < 16)?
				(kResourceInUserDataSgpr | vsharpFlag | startRegister) : (vsharpFlag | extendedRegisterOffsetInDwords);
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 8;
			break;

		case Gnm::kShaderInputUsageImmRwResource:
			SCE_GNM_ASSERT(apiSlot >= 0 && apiSlot < kMaxRwResourceCount);
			outTable->rwResourceDwOffset[apiSlot] = (inputUsageSlots[i].m_startRegister < 16)?
				(kResourceInUserDataSgpr | vsharpFlag | startRegister) : (vsharpFlag | extendedRegisterOffsetInDwords);
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 8;
			break;

		case Gnm::kShaderInputUsageImmSampler:
			SCE_GNM_ASSERT(apiSlot >= 0 && apiSlot < kMaxSamplerCount);
			outTable->samplerDwOffset[apiSlot] = (inputUsageSlots[i].m_startRegister < 16)?
				(kResourceInUserDataSgpr | startRegister) : extendedRegisterOffsetInDwords;
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 4;
			break;

		case Gnm::kShaderInputUsageImmConstBuffer:
			SCE_GNM_ASSERT(apiSlot >= 0 && apiSlot < kMaxConstantBufferCount);
			outTable->constBufferDwOffset[apiSlot] = (inputUsageSlots[i].m_startRegister < 16)?
				(kResourceInUserDataSgpr | startRegister) : extendedRegisterOffsetInDwords;
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 4;
			break;

		case Gnm::kShaderInputUsageImmVertexBuffer:
			SCE_GNM_ASSERT(apiSlot >= 0 && apiSlot < kMaxVertexBufferCount);
			outTable->vertexBufferDwOffset[apiSlot] = (inputUsageSlots[i].m_startRegister < 16)?
				(kResourceInUserDataSgpr | startRegister) : extendedRegisterOffsetInDwords;
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 4;
			break;
		
		// SRTs will always reside inside the Imm UserData (dwords 0-15), as opposed to the 
		// above resources which can exist in the EUD
		case Gnm::kShaderInputUsageImmShaderResourceTable:
			SCE_GNM_ASSERT(apiSlot >= 0 && apiSlot < kMaxUserDataCount);
			outTable->userSrtDataSgpr = inputUsageSlots[i].m_startRegister;
			outTable->userSrtDataCount = inputUsageSlots[i].m_srtSizeInDWordMinusOne+1;
			break;
		}
	}

	// Make sure we can fit a T# (if required) in the last userOffset
	requiredMemorySizeInDwords += lastUserDataResourceSizeInDwords;

	// Now handle only pointers to resource-tables. Items handled below cannot be found more than once
	for (int32_t i=0; i<inputUsageSlotsCount; ++i)
	{
		uint8_t maskChunks = inputUsageSlots[i].m_chunkMask;
		
		const uint64_t kNibbleToCount = 0x4332322132212110ull;
		uint8_t chunksCount = (kNibbleToCount >> ((maskChunks & 0xF)*4)) & 0xF; (void)chunksCount;
		SCE_GNM_ASSERT(usageMasks+chunksCount <= (uint32_t const*)shaderBinaryInfo);
		
		// Lets fill the resource indices first
		int32_t usedApiSlots[Gnm::kSlotCountResource]; // Use the size of the biggest resource table
		int32_t usedApiSlotCount;

		// This thing will break if there's more than 1 table for any resource type
		uint8_t startRegister = inputUsageSlots[i].m_startRegister;

		switch (inputUsageSlots[i].m_usageType)
		{
		case Gnm::kShaderInputUsagePtrResourceTable:
		{
			SCE_GNM_ASSERT(inputUsageSlots[i].m_apiSlot == 0);
			outTable->resourcePtrSgpr = startRegister;
			outTable->resourceArrayDwOffset = requiredMemorySizeInDwords;

			SCE_GNM_ASSERT(usageMasks < (uint32_t const*)shaderBinaryInfo);
			uint32_t maskArray[4] = { 0, 0, 0, 0};
			if (maskChunks & 1) maskArray[0] = *usageMasks++;
			if (maskChunks & 2) maskArray[1] = *usageMasks++;
			if (maskChunks & 4) maskArray[2] = *usageMasks++;
			if (maskChunks & 8) maskArray[3] = *usageMasks++;
			usedApiSlotCount = getUsedApiSlotsFromMask(&usedApiSlots[0], kMaxResourceCount, maskArray, Gnm::kSlotCountResource);

			int32_t lastUsedApiSlot = usedApiSlots[usedApiSlotCount-1];
			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				int32_t currentApiSlot = usedApiSlots[j];
				outTable->resourceDwOffset[currentApiSlot] = requiredMemorySizeInDwords + currentApiSlot * Gnm::kDwordSizeResource;
			}
			requiredMemorySizeInDwords += (lastUsedApiSlot+1) * Gnm::kDwordSizeResource;
		}
		break;

		case Gnm::kShaderInputUsagePtrRwResourceTable:
		{
			SCE_GNM_ASSERT(inputUsageSlots[i].m_apiSlot == 0);
			outTable->rwResourcePtrSgpr = startRegister;
			outTable->rwResourceArrayDwOffset = requiredMemorySizeInDwords;

			SCE_GNM_ASSERT(usageMasks < (uint32_t const*)shaderBinaryInfo);
			usedApiSlotCount = getUsedApiSlotsFromMask(&usedApiSlots[0], kMaxRwResourceCount, *usageMasks++, Gnm::kSlotCountRwResource);

			int32_t lastUsedApiSlot = usedApiSlots[usedApiSlotCount-1];
			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				int32_t currentApiSlot = usedApiSlots[j];
				outTable->rwResourceDwOffset[currentApiSlot] = requiredMemorySizeInDwords + currentApiSlot * Gnm::kDwordSizeRwResource;
			}
			requiredMemorySizeInDwords += (lastUsedApiSlot+1) * Gnm::kDwordSizeRwResource;
		}
		break;

		case Gnm::kShaderInputUsagePtrConstBufferTable:
		{
			SCE_GNM_ASSERT(inputUsageSlots[i].m_apiSlot == 0);
			outTable->constBufferPtrSgpr = startRegister;
			outTable->constBufferArrayDwOffset = requiredMemorySizeInDwords;
			
			SCE_GNM_ASSERT(usageMasks < (uint32_t const*)shaderBinaryInfo);
			usedApiSlotCount = getUsedApiSlotsFromMask(&usedApiSlots[0], kMaxConstantBufferCount, *usageMasks++, Gnm::kSlotCountConstantBuffer);

			int32_t lastUsedApiSlot = usedApiSlots[usedApiSlotCount-1];
			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				int32_t currentApiSlot = usedApiSlots[j];
				outTable->constBufferDwOffset[currentApiSlot] = requiredMemorySizeInDwords + currentApiSlot * Gnm::kDwordSizeConstantBuffer;
			}
			requiredMemorySizeInDwords += (lastUsedApiSlot+1) * Gnm::kDwordSizeConstantBuffer;
		}
		break;

		case Gnm::kShaderInputUsagePtrSamplerTable:
		{
			SCE_GNM_ASSERT(inputUsageSlots[i].m_apiSlot == 0);
			outTable->samplerPtrSgpr = startRegister;
			outTable->samplerArrayDwOffset = requiredMemorySizeInDwords;

			SCE_GNM_ASSERT(usageMasks < (uint32_t const*)shaderBinaryInfo);
			usedApiSlotCount = getUsedApiSlotsFromMask(&usedApiSlots[0], kMaxSamplerCount, *usageMasks++, Gnm::kSlotCountSampler);

			int32_t lastUsedApiSlot = usedApiSlots[usedApiSlotCount-1];
			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				int32_t currentApiSlot = usedApiSlots[j];
				outTable->samplerDwOffset[currentApiSlot] = requiredMemorySizeInDwords + currentApiSlot * Gnm::kDwordSizeSampler;
			}
			requiredMemorySizeInDwords += (lastUsedApiSlot+1) * Gnm::kDwordSizeSampler;
		}
		break;

		case Gnm::kShaderInputUsagePtrVertexBufferTable:
		{
			SCE_GNM_ASSERT(shaderStage == Gnm::kShaderStageLs || shaderStage == Gnm::kShaderStageEs || shaderStage == Gnm::kShaderStageVs);
			SCE_GNM_ASSERT(inputUsageSlots[i].m_apiSlot == 0);
			outTable->vertexBufferPtrSgpr = startRegister;
			outTable->vertexBufferArrayDwOffset = requiredMemorySizeInDwords;
			
			const Gnm::VertexInputSemantic* semanticTable = NULL;
			SCE_GNM_ASSERT(usageMasks <= (uint32_t const*)shaderBinaryInfo);
			usedApiSlotCount = 0;
			if (shaderStage == Gnm::kShaderStageVs)
			{
				usedApiSlotCount = ((Gnmx::VsShader*)gnmxShaderStruct)->m_numInputSemantics;
				semanticTable = ((Gnmx::VsShader*)gnmxShaderStruct)->getInputSemanticTable();
			}
			else if (shaderStage == Gnm::kShaderStageLs)
			{
				usedApiSlotCount = ((Gnmx::LsShader*)gnmxShaderStruct)->m_numInputSemantics;
				semanticTable = ((Gnmx::LsShader*)gnmxShaderStruct)->getInputSemanticTable();
			}
			else if (shaderStage == Gnm::kShaderStageEs)
			{
				usedApiSlotCount = ((Gnmx::EsShader*)gnmxShaderStruct)->m_numInputSemantics;
				semanticTable = ((Gnmx::EsShader*)gnmxShaderStruct)->getInputSemanticTable();
			}
			// Check if the shader uses any API-slot over the maximum count configured to be handled by the LCUE
			SCE_GNM_ASSERT(usedApiSlotCount > 0 && usedApiSlotCount <= LightweightConstantUpdateEngine::kMaxVertexBufferCount);

			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				int32_t semanticIndex = semanticTable[j].m_semantic;
				SCE_GNM_ASSERT(semanticIndex >= 0 && semanticIndex < LightweightConstantUpdateEngine::kMaxVertexBufferCount);
				outTable->vertexBufferDwOffset[semanticIndex] = requiredMemorySizeInDwords + semanticIndex * Gnm::kDwordSizeVertexBuffer;
			}
			requiredMemorySizeInDwords += (semanticTable[usedApiSlotCount-1].m_semantic+1) * Gnm::kDwordSizeVertexBuffer;
			
		}
		break;
		}
	}
	outTable->requiredBufferSizeInDwords = requiredMemorySizeInDwords;

	// Checking for unhandled input data
	for (int32_t i=0; i<inputUsageSlotsCount; ++i)
	{
		switch (inputUsageSlots[i].m_usageType)
		{
		case Gnm::kShaderInputUsageImmResource:
		case Gnm::kShaderInputUsageImmRwResource:
		case Gnm::kShaderInputUsageImmSampler:
		case Gnm::kShaderInputUsageImmConstBuffer:
		case Gnm::kShaderInputUsageImmVertexBuffer:
		case Gnm::kShaderInputUsageImmShaderResourceTable:
		case Gnm::kShaderInputUsageSubPtrFetchShader:
		case Gnm::kShaderInputUsagePtrExtendedUserData:
		case Gnm::kShaderInputUsagePtrResourceTable:
		case Gnm::kShaderInputUsagePtrRwResourceTable:
		case Gnm::kShaderInputUsagePtrConstBufferTable:
		case Gnm::kShaderInputUsagePtrVertexBufferTable:
		case Gnm::kShaderInputUsagePtrSamplerTable:
		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
		case Gnm::kShaderInputUsageImmGdsCounterRange:
		//case Gnm::kShaderInputUsagePtrSoBufferTable:		// Only present in the VS copy-shader that doesn't have a footer
			// Handled
			break;

		default:
			// Not handled yet
			SCE_GNM_ASSERT(false);
			break;
		}
	}
}

#endif // !defined(SCE_GNM_OFFLINE_MODE)
