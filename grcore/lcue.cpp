
#include "lcue.h"

using namespace sce;
using namespace LCUE;

#if !defined _DEBUG
#define LCUE_FORCE_INLINE __inline__ __attribute__((always_inline)) 
#else
#define LCUE_FORCE_INLINE 
#endif

// Profiling
#if defined(LCUE_PROFILE_ENABLED)
#define LCUE_PROFILE_SET_VSHARP_CALL m_setVsharpCount++
#define LCUE_PROFILE_SET_SSHARP_CALL m_setSsharpCount++
#define LCUE_PROFILE_SET_TSHARP_CALL m_setTsharpCount++
#define LCUE_PROFILE_SET_PTR_CALL m_setPtrCount++
#define LCUE_PROFILE_PSSHADER_USAGE_CALL m_psUsageCount++
#define LCUE_PROFILE_DRAW_CALL m_drawCount++
#define LCUE_PROFILE_DISPATCH_CALL m_dispatchCount++
#define LCUE_PROFILE_REQUIRED_BUFFER_SIZE(TABLE) m_shaderResourceTotalUploadSizeInBytes += ((TABLE)->requiredBufferSizeInDwords)
#define LCUE_PROFILE_RESET																\
	m_setVsharpCount = m_setSsharpCount = m_setTsharpCount = m_setPtrCount = 0;			\
	m_psUsageCount = m_drawCount = m_dispatchCount = m_shaderResourceTotalUploadSizeInBytes = 0;
#else
#define LCUE_PROFILE_SET_VSHARP_CALL
#define LCUE_PROFILE_SET_SSHARP_CALL 
#define LCUE_PROFILE_SET_TSHARP_CALL 
#define LCUE_PROFILE_SET_PTR_CALL 
#define LCUE_PROFILE_PSSHADER_USAGE_CALL
#define LCUE_PROFILE_DRAW_CALL 
#define LCUE_PROFILE_DISPATCH_CALL 
#define LCUE_PROFILE_REQUIRED_BUFFER_SIZE(TABLE) 
#define LCUE_PROFILE_RESET
#endif

#if defined LCUE_IMMEDIATE_CB_AUTO_HANDLE_ENABLED
#define LCUE_IMMEDIATE_CB_UPDATE_ON_GRAPHICS(a, b) updateImmediateCb(a, (const Gnmx::ShaderCommonData*)(b))
#define LCUE_IMMEDIATE_CB_UPDATE_ON_COMPUTE(b) updateImmediateCb( (const Gnmx::ShaderCommonData*)(b) )
#else
#define LCUE_IMMEDIATE_CB_UPDATE_ON_GRAPHICS(a, b)
#define LCUE_IMMEDIATE_CB_UPDATE_ON_COMPUTE(b)
#endif

// Validation of resources that are not expected by the shader
#if defined LCUE_VALIDATE_RESOURCE_BINDING_AND_ASSERT_ENABLED
#define LCUE_ASSERT_OR_CONTINUE(a) assert(a)
#elif defined LCUE_VALIDATE_RESOURCE_BINDING_AND_SKIP_ENABLED
#define LCUE_ASSERT_OR_CONTINUE(a) if (!(a)) continue
#else
#define LCUE_ASSERT_OR_CONTINUE(a)
#endif

// Validation of complete resource binding
#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
#define LCUE_VALIDATE_RESOURCE_INIT_TABLE(a, b) initResourceBindingValidation(a, b)
#define LCUE_VALIDATE_RESOURCE_CHECK_TABLE(a) assert(isResourceBindingComplete(a))
#define LCUE_VALIDATE_RESOURCE_MARK_USED(a) (a) = true
#else
#define LCUE_VALIDATE_RESOURCE_INIT_TABLE(a, b)
#define LCUE_VALIDATE_RESOURCE_CHECK_TABLE(a)
#define LCUE_VALIDATE_RESOURCE_MARK_USED(a)
#endif


inline void setPersistentRegisterRange(Gnm::CommandBuffer* cb, Gnm::ShaderStage shaderStage, uint32_t startSgpr, const uint32_t* values, uint32_t valuesCount)
{
	assert( valuesCount <= (sizeof(Gnm::Texture)/sizeof(uint32_t)) );

	const uint32_t kGpuStageUserDataRegisterBases[Gnm::kShaderStageCount] = { 0x240, 0xC, 0x4C, 0x8C, 0xCC, 0x10C, 0x14C };
	Gnm::ShaderType shaderType = (shaderStage == Gnm::kShaderStageCs)? Gnm::kShaderTypeCompute : Gnm::kShaderTypeGraphics;

	uint32_t packetSizeInDw = 2 + valuesCount;

	if (cb->m_cmdptr + packetSizeInDw > cb->m_endptr)
		cb->m_callback.m_func(cb,packetSizeInDw,cb->m_callback.m_userData);

	uint32_t packet = 0xC0007600; // Set persistent register
	packet |= (shaderType << 1);
	packet |= (valuesCount << 16);

	cb->m_cmdptr[0] = packet;
	cb->m_cmdptr[1] = kGpuStageUserDataRegisterBases[(int32_t)shaderStage] + startSgpr;
	for (uint32_t i=0; i<valuesCount; i++)
		cb->m_cmdptr[i+2] = values[i];
	cb->m_cmdptr += packetSizeInDw;
}


LCUE_FORCE_INLINE 
	void setPtrInPersistentRegisterRange(Gnm::CommandBuffer* cb, Gnm::ShaderStage shaderStage, uint32_t startSgpr, const void* address)
{
	uint32_t values[2];
	values[0] = (uint32_t)( ((uintptr_t)address) & 0xFFFFFFFFULL );
	values[1] = (uint32_t)( ((uintptr_t)address) >> 32 );
	setPersistentRegisterRange(cb, shaderStage, startSgpr, values, 2);
}


LCUE_FORCE_INLINE 
	void setDataInUserDataSgprOrMemory(Gnm::CommandBuffer* dcb, uint32_t* scratchBuffer, Gnm::ShaderStage shaderStage, uint16_t shaderResourceOffset, restrict const void* data, int32_t dataSizeInBytes)
{
	assert(dcb != NULL);
	assert(data != NULL && dataSizeInBytes > 0 && (dataSizeInBytes%4) == 0);

	int32_t userDataRegisterOrMemoryOffset = (shaderResourceOffset & kResourceValueMask);
	if ((shaderResourceOffset & kResourceInUserDataSgpr) != 0)
	{
		setPersistentRegisterRange(dcb, shaderStage, userDataRegisterOrMemoryOffset, (uint32_t*)data, dataSizeInBytes/sizeof(uint32_t));
	}
	else
	{
		restrict uint32_t* scratchDestAddress = (uint32_t*)(scratchBuffer + ((int)shaderStage * kGpuStageBufferSizeInDwords) + userDataRegisterOrMemoryOffset);
		__builtin_memcpy(scratchDestAddress, data, dataSizeInBytes);
	}
}


#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
bool isResourceBindingComplete(const LCUE::ShaderResourceBindingValidation* table)
{
	bool isValid = true;
	if (table != NULL)
	{
		int32_t i=0; // Failed resource index
		while (isValid && i<kMaxResourceCount) { isValid &= table->resourceOffsetIsBound[i]; ++i; } --i;			assert(isValid); i=0; // Resource not bound
		while (isValid && i<kMaxRwResourceCount) { isValid &= table->rwResourceOffsetIsBound[i]; ++i; } --i;		assert(isValid); i=0; // RW-resource not bound
		while (isValid && i<kMaxSamplerCount) { isValid &= table->samplerOffsetIsBound[i]; ++i; } --i;				assert(isValid); i=0; // Sampler not bound
		while (isValid && i<kMaxVertexBufferCount) { isValid &= table->vertexBufferOffsetIsBound[i]; ++i; } --i;	assert(isValid); i=0; // Constant buffer not bound
		while (isValid && i<kMaxConstantBufferCount) { isValid &= table->constBufferOffsetIsBound[i]; ++i; } --i;	assert(isValid); i=0; // Vertex buffer not bound
	}
	return isValid;
}


void initResourceBindingValidation(LCUE::ShaderResourceBindingValidation* validationTable, const LCUE::ShaderResourceOffsets* table)
{
	if (table != NULL)
	{
		// Mark all expected resources slots (!= 0xFFFF) as not bound (false)
		for (int32_t i=0; i<kMaxResourceCount; ++i)			validationTable->resourceOffsetIsBound[i] = (table->resourceOffset[i] == 0xFFFF);
		for (int32_t i=0; i<kMaxRwResourceCount; ++i)		validationTable->rwResourceOffsetIsBound[i] = (table->rwResourceOffset[i] == 0xFFFF);
		for (int32_t i=0; i<kMaxSamplerCount; ++i)			validationTable->samplerOffsetIsBound[i] = (table->samplerOffset[i] == 0xFFFF);
		for (int32_t i=0; i<kMaxVertexBufferCount; ++i)		validationTable->vertexBufferOffsetIsBound[i] = (table->vertexBufferOffset[i] == 0xFFFF);
		for (int32_t i=0; i<kMaxConstantBufferCount; ++i)	validationTable->constBufferOffsetIsBound[i] = (table->constBufferOffset[i] == 0xFFFF);
	}
	else
	{
		// If there's no table, all resources are valid
		assert( sizeof(bool) == sizeof(unsigned char) );
		__builtin_memset(validationTable, true, sizeof(ShaderResourceBindingValidation));
	}
}
#endif


void BaseCUE::init(
	uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, 
	uint32_t* globalInternalResourceTableInGarlic, int32_t globalInternalResourceTableSizeInDwords)
{
	// assert(resourceBuffersInGarlic != NULL && resourceBufferCount >= 1 && resourceBufferCount <= kMaxResourceBufferCount && resourceBufferSizeInDwords >= kMinResourceBufferSizeInDwords);
	assert( (globalInternalResourceTableInGarlic == NULL && globalInternalResourceTableSizeInDwords == 0) ||
		globalInternalResourceTableInGarlic != NULL && globalInternalResourceTableSizeInDwords == kGlobalInternalTableSizeInDwords);

	m_bufferIndex = 0;
	if (resourceBufferCount && resourceBuffersInGarlic)
	{
		m_bufferCount = (resourceBufferCount < kMaxResourceBufferCount)? resourceBufferCount : kMaxResourceBufferCount;
		for (int32_t i=0; i<m_bufferCount; ++i)
		{
			m_bufferBegin[i] = resourceBuffersInGarlic[i];
			m_bufferEnd[i] = m_bufferBegin[i] + resourceBufferSizeInDwords;
		}
		m_bufferCurrent = m_bufferBegin[m_bufferIndex];
	}
	else
	{
		m_bufferCount = 0;
		m_bufferCurrent = NULL;
	}

	m_globalInternalResourceTable = (Gnm::Buffer*)globalInternalResourceTableInGarlic;

	LCUE_PROFILE_RESET;
}


void BaseCUE::swapBuffers()
{
	if (m_bufferCount)
	{
		m_bufferIndex = (m_bufferIndex+1) % m_bufferCount;
		m_bufferCurrent = m_bufferBegin[m_bufferIndex];
	}

	LCUE_PROFILE_RESET;
}


void BaseCUE::setGlobalInternalResource(Gnm::ShaderGlobalResourceType resourceType, restrict const Gnm::Buffer* buffer)
{
	assert(m_globalInternalResourceTable != NULL && buffer != NULL);
	assert(resourceType >= 0 && resourceType <= Gnm::kShaderGlobalResourceCount);
	assert(resourceType != Gnm::kShaderGlobalResourceTessFactorBuffer || ((uintptr_t)buffer->getBaseAddress()) % Gnm::kMinimumTessFactorBufferAlignmentInBytes == 0);

	__builtin_memcpy(&m_globalInternalResourceTable[(int32_t)resourceType], buffer, sizeof(Gnm::Buffer));
}


LCUE_FORCE_INLINE
	uint32_t* BaseCUE::allocateSpace(sce::Gnm::CommandBuffer *cb,uint32_t dwordCount)
{
	if (m_bufferCurrent)
	{
		assert( (m_bufferCurrent + dwordCount) < m_bufferEnd[m_bufferIndex] );
		uint32_t *result = m_bufferCurrent;
		m_bufferCurrent += dwordCount;
		return result;
	}
	else
	{
		if (cb->m_cmdptr + dwordCount > cb->m_endptr)
			cb->m_callback.m_func(cb,dwordCount,cb->m_callback.m_userData);
		return (cb->m_endptr -= dwordCount);
	}
}


void ComputeCUE::init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, 
	uint32_t* globalInternalResourceTableInGarlic, int32_t globalInternalResourceTableSizeInDwords)
{
	BaseCUE::init(resourceBuffersInGarlic, resourceBufferCount, resourceBufferSizeInDwords,
		globalInternalResourceTableInGarlic, globalInternalResourceTableSizeInDwords);

	__builtin_memset(m_scratchBuffer, 0, sizeof(uint32_t) * kComputeScratchBufferSizeInDwords);

	m_dirtyShaderResources = false;
	m_dirtyShader = false;
	m_boundShaderResourceOffsets = NULL;
	m_boundShader = NULL;

	m_dcb = NULL;

#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	__builtin_memset(&m_boundShaderResourcesValidation, true, sizeof(ShaderResourceBindingValidation));
#endif
}


void ComputeCUE::swapBuffers()
{
	BaseCUE::swapBuffers();
	m_dirtyShaderResources = false;
	m_dirtyShader = false;
	m_boundShaderResourceOffsets = NULL;
	m_boundShader = NULL;

#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	__builtin_memset(&m_boundShaderResourcesValidation, true, sizeof(ShaderResourceBindingValidation));
#endif
}


LCUE_FORCE_INLINE 
	uint32_t* ComputeCUE::flushScratchBuffer()
{
	assert( m_boundShader != NULL );

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;

	// Copy scratch data over the main buffer
	restrict uint32_t* destAddr = allocateSpace(m_dcb, table->requiredBufferSizeInDwords);
	restrict uint32_t* sourceAddr = m_scratchBuffer;
	__builtin_memcpy(destAddr, sourceAddr, table->requiredBufferSizeInDwords * sizeof(uint32_t));

	LCUE_PROFILE_REQUIRED_BUFFER_SIZE(table);

	return destAddr;
}


LCUE_FORCE_INLINE 
	void ComputeCUE::updateCommonPtrsInUserDataSgprs(const uint32_t* resourceBufferFlushedAddress)
{
	assert( m_boundShader != NULL );
	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;

	if (table->globalInternalPtrSgpr != 0xFF)
	{
		assert(m_globalInternalResourceTable != NULL);
		setPtrInPersistentRegisterRange(m_dcb, Gnm::kShaderStageCs, table->globalInternalPtrSgpr, m_globalInternalResourceTable );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->userExtendedData1PtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, Gnm::kShaderStageCs, table->userExtendedData1PtrSgpr, resourceBufferFlushedAddress );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->resourcePtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, Gnm::kShaderStageCs, table->resourcePtrSgpr, resourceBufferFlushedAddress + table->resourceArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->rwResourcePtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, Gnm::kShaderStageCs, table->rwResourcePtrSgpr, resourceBufferFlushedAddress + table->rwResourceArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->samplerPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, Gnm::kShaderStageCs, table->samplerPtrSgpr, resourceBufferFlushedAddress + table->samplerArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->constBufferPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, Gnm::kShaderStageCs, table->constBufferPtrSgpr, resourceBufferFlushedAddress + table->constBufferArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}
}


LCUE_FORCE_INLINE 
	void ComputeCUE::updateImmediateCb(const Gnmx::ShaderCommonData* shaderCommon)
{
	if (shaderCommon != NULL && shaderCommon->m_embeddedConstantBufferSizeInDQW > 0)
	{
		const uint32_t* shaderRegisters = (const uint32_t*)(shaderCommon + 1);
		const uint8_t* shaderCode = (uint8_t*)( ((uintptr_t)shaderRegisters[0] << 8ULL) | ((uintptr_t)shaderRegisters[1] << 40ULL) );

		Gnm::Buffer embeddedCb;
		embeddedCb.initAsConstantBuffer((void*)(shaderCode + shaderCommon->m_shaderSize), shaderCommon->m_embeddedConstantBufferSizeInDQW << 4);
		setConstantBuffers(LCUE::kConstantBufferInternalApiSlotForEmbeddedData, 1, &embeddedCb);
	}
}


void ComputeCUE::preDispatch()
{
	assert(m_dcb != NULL);
	LCUE_VALIDATE_RESOURCE_CHECK_TABLE( &m_boundShaderResourcesValidation );

	if (m_dirtyShader)
	{
		const Gnmx::CsShader* csShader = (const Gnmx::CsShader*)m_boundShader;
		m_dcb->setCsShader( &csShader->m_csStageRegisters );
		
		LCUE_IMMEDIATE_CB_UPDATE_ON_COMPUTE(csShader);
	}

	if (m_dirtyShaderResources)
		updateCommonPtrsInUserDataSgprs( flushScratchBuffer() );

	m_dirtyShaderResources = false;
	m_dirtyShader = false;
	LCUE_PROFILE_DISPATCH_CALL;
}


void ComputeCUE::setCsShader(const Gnmx::CsShader* shader, const ShaderResourceOffsets* table)
{
	assert( shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageCs );
	LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation, table );

	m_dirtyShaderResources = true;
	m_dirtyShader |= (m_boundShader != shader);
	m_boundShaderResourceOffsets = table;
	m_boundShader = shader;
}


void ComputeCUE::setConstantBuffers(int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Buffer* buffer)
{
	assert(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxConstantBufferCount);
	assert(m_boundShaderResourceOffsets != NULL && buffer != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets->constBufferOffset[currentApiSlot] != 0xFFFF);
		LCUE_VALIDATE_RESOURCE_MARK_USED( m_boundShaderResourcesValidation.constBufferOffsetIsBound[currentApiSlot] );
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], Gnm::kShaderStageCs, table->constBufferOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
	}

	m_dirtyShaderResources = true;
	LCUE_PROFILE_SET_VSHARP_CALL;
}


void ComputeCUE::setVertexBuffers(int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Buffer* buffer)
{
	assert(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxVertexBufferCount);
	assert(m_boundShaderResourceOffsets != NULL && buffer != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; ++i)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets->vertexBufferOffset[currentApiSlot] != 0xFFFF);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.vertexBufferOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], Gnm::kShaderStageCs, table->vertexBufferOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
	}

	m_dirtyShaderResources = true;
	LCUE_PROFILE_SET_VSHARP_CALL;
}


void ComputeCUE::setBuffers(int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Buffer* buffer)
{
	assert(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxResourceCount);
	assert(m_boundShaderResourceOffsets != NULL && buffer != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets->resourceOffset[currentApiSlot] != 0xFFFF &&
			(m_boundShaderResourceOffsets->resourceOffset[currentApiSlot] & kResourceIsVSharp) != 0);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.resourceOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], Gnm::kShaderStageCs, table->resourceOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
	}

	m_dirtyShaderResources = true;
	LCUE_PROFILE_SET_VSHARP_CALL;
}


void ComputeCUE::setRwBuffers(int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Buffer* buffer)
{
	assert(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxRwResourceCount);
	assert(m_boundShaderResourceOffsets != NULL && buffer != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets->rwResourceOffset[currentApiSlot] != 0xFFFF &&
			(m_boundShaderResourceOffsets->rwResourceOffset[currentApiSlot] & kResourceIsVSharp) != 0);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.rwResourceOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], Gnm::kShaderStageCs, table->rwResourceOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
	}

	m_dirtyShaderResources = true;
	LCUE_PROFILE_SET_VSHARP_CALL;
}


void ComputeCUE::setTextures(int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Texture* texture)
{
	assert(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxResourceCount);
	assert(m_boundShaderResourceOffsets != NULL && texture != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets->resourceOffset[currentApiSlot] != 0xFFFF &&
			(m_boundShaderResourceOffsets->resourceOffset[currentApiSlot] & kResourceIsVSharp) == 0);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.resourceOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], Gnm::kShaderStageCs, table->resourceOffset[currentApiSlot], texture+i, sizeof(Gnm::Texture));
	}

	m_dirtyShaderResources = true;
	LCUE_PROFILE_SET_TSHARP_CALL;
}


void ComputeCUE::setRwTextures(int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Texture* texture)
{
	assert(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxRwResourceCount);
	assert(m_boundShaderResourceOffsets != NULL && texture != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets->rwResourceOffset[currentApiSlot] != 0xFFFF &&
			(m_boundShaderResourceOffsets->rwResourceOffset[currentApiSlot] & kResourceIsVSharp) == 0);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.rwResourceOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], Gnm::kShaderStageCs, table->rwResourceOffset[currentApiSlot], texture+i, sizeof(Gnm::Texture));
	}

	m_dirtyShaderResources = true;
	LCUE_PROFILE_SET_TSHARP_CALL;
}


void ComputeCUE::setSamplers(int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Sampler* sampler)
{
	assert(startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxSamplerCount);
	assert(m_boundShaderResourceOffsets != NULL && sampler != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets;
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets->samplerOffset[currentApiSlot] != 0xFFFF);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation.samplerOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], Gnm::kShaderStageCs, table->samplerOffset[currentApiSlot], sampler+i, sizeof(Gnm::Sampler));
	}

	m_dirtyShaderResources = true;
	LCUE_PROFILE_SET_SSHARP_CALL;
}



void GraphicsCUE::init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, 
	uint32_t* globalInternalResourceTableInGarlic, int32_t globalInternalResourceTableSizeInDwords)
{
	BaseCUE::init(resourceBuffersInGarlic, resourceBufferCount, resourceBufferSizeInDwords, 
		globalInternalResourceTableInGarlic, globalInternalResourceTableSizeInDwords);

	__builtin_memset(m_scratchBuffer, 0, sizeof(uint32_t) * kGraphicsScratchBufferSizeInDwords);

	__builtin_memset(m_dirtyShaderResources, 0, sizeof(bool) * Gnm::kShaderStageCount);
	__builtin_memset(m_dirtyShader, 0, sizeof(bool) * Gnm::kShaderStageCount);
	__builtin_memset(m_boundShaderResourceOffsets, 0, sizeof(void*) * Gnm::kShaderStageCount);
	__builtin_memset(m_boundShader, 0, sizeof(void*) * Gnm::kShaderStageCount);

	m_dcb = NULL;
	m_tessellationDesiredTgPatchCount = 0;

#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	__builtin_memset(m_boundShaderResourcesValidation, true, sizeof(ShaderResourceBindingValidation) * Gnm::kShaderStageCount);
#endif
}


void GraphicsCUE::swapBuffers()
{
	BaseCUE::swapBuffers();
	__builtin_memset(m_dirtyShaderResources, 0, sizeof(bool) * Gnm::kShaderStageCount);
	__builtin_memset(m_dirtyShader, 0, sizeof(bool) * Gnm::kShaderStageCount);
	__builtin_memset(m_boundShaderResourceOffsets, 0, sizeof(void*) * Gnm::kShaderStageCount);
	__builtin_memset(m_boundShader, 0, sizeof(void*) * Gnm::kShaderStageCount);

#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	__builtin_memset(m_boundShaderResourcesValidation, true, sizeof(ShaderResourceBindingValidation) * Gnm::kShaderStageCount);
#endif
}


LCUE_FORCE_INLINE 
	uint32_t* GraphicsCUE::flushScratchBuffer(Gnm::ShaderStage shaderStage)
{
	assert( m_boundShader[(int32_t)shaderStage] != NULL );

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];

	// Copy scratch data over the main buffer
	restrict uint32_t* destAddr = allocateSpace(m_dcb, table->requiredBufferSizeInDwords);
	restrict uint32_t* sourceAddr = m_scratchBuffer + ((int32_t)shaderStage * kGpuStageBufferSizeInDwords);
	__builtin_memcpy(destAddr, sourceAddr, table->requiredBufferSizeInDwords * sizeof(uint32_t));

	LCUE_PROFILE_REQUIRED_BUFFER_SIZE(table);

	return destAddr;
}


LCUE_FORCE_INLINE 
	void GraphicsCUE::updateLsEsVsPtrsInUserDataSgprs(Gnm::ShaderStage shaderStage, const uint32_t* resourceBufferFlushedAddress)
{
	assert( m_boundShader[(int32_t)shaderStage] != NULL );
	assert( shaderStage == Gnm::kShaderStageLs || shaderStage == Gnm::kShaderStageEs || shaderStage == Gnm::kShaderStageVs );
	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];

	if (table->fetchShaderPtrSgpr != 0xFF)
	{
		assert( m_boundFetchShader[(int32_t)shaderStage] != NULL );
		setPtrInPersistentRegisterRange(m_dcb, shaderStage, table->fetchShaderPtrSgpr, m_boundFetchShader[(int32_t)shaderStage]);
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->vertexBufferPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, shaderStage, table->vertexBufferPtrSgpr, resourceBufferFlushedAddress + table->vertexBufferArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}

}


LCUE_FORCE_INLINE 
	void GraphicsCUE::updateCommonPtrsInUserDataSgprs(Gnm::ShaderStage shaderStage, const uint32_t* resourceBufferFlushedAddress)
{
	assert( m_boundShader[(int32_t)shaderStage] != NULL );
	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	
	if (table->globalInternalPtrSgpr != 0xFF)
	{
		assert(m_globalInternalResourceTable != NULL);
		setPtrInPersistentRegisterRange(m_dcb, shaderStage, table->globalInternalPtrSgpr, m_globalInternalResourceTable );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->userExtendedData1PtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, shaderStage, table->userExtendedData1PtrSgpr, resourceBufferFlushedAddress );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->resourcePtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, shaderStage, table->resourcePtrSgpr, resourceBufferFlushedAddress + table->resourceArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->rwResourcePtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, shaderStage, table->rwResourcePtrSgpr, resourceBufferFlushedAddress + table->rwResourceArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->samplerPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, shaderStage, table->samplerPtrSgpr, resourceBufferFlushedAddress + table->samplerArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}
	if (table->constBufferPtrSgpr != 0xFF)
	{
		setPtrInPersistentRegisterRange(m_dcb, shaderStage, table->constBufferPtrSgpr, resourceBufferFlushedAddress + table->constBufferArrayOffset );
		LCUE_PROFILE_SET_PTR_CALL;
	}
}


LCUE_FORCE_INLINE 
	void GraphicsCUE::updateImmediateCb(sce::Gnm::ShaderStage shaderStage, const Gnmx::ShaderCommonData* shaderCommon)
{
	if (shaderCommon != NULL && shaderCommon->m_embeddedConstantBufferSizeInDQW > 0)
	{
		const uint32_t* shaderRegisters = (const uint32_t*)(shaderCommon + 1);
		const uint8_t* shaderCode = (uint8_t*)( ((uintptr_t)shaderRegisters[0] << 8ULL) | ((uintptr_t)shaderRegisters[1] << 40ULL) );

		Gnm::Buffer embeddedCb;
		embeddedCb.initAsConstantBuffer((void*)(shaderCode + shaderCommon->m_shaderSize), shaderCommon->m_embeddedConstantBufferSizeInDQW << 4);
		setConstantBuffers(shaderStage, LCUE::kConstantBufferInternalApiSlotForEmbeddedData, 1, &embeddedCb);
	}
}


void GraphicsCUE::preDispatch()
{
	assert(m_dcb != NULL);
	LCUE_VALIDATE_RESOURCE_CHECK_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageCs] );

	if (m_dirtyShader[Gnm::kShaderStageCs])
		m_dcb->setCsShader( &((const Gnmx::CsShader*)m_boundShader[Gnm::kShaderStageCs])->m_csStageRegisters );

	if (m_dirtyShaderResources[Gnm::kShaderStageCs])
	{
		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageCs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageCs, flushAddress);
	}

	m_dirtyShaderResources[Gnm::kShaderStageCs] = false;
	m_dirtyShader[Gnm::kShaderStageCs] = false;
	LCUE_PROFILE_DISPATCH_CALL;
}


void GraphicsCUE::preDraw()
{
	assert( m_dcb != NULL );
	assert( m_boundShader[Gnm::kShaderStageVs] != NULL); // VS cannot be NULL for any pipeline configuration
	//assert( m_boundShader[Gnm::kShaderStagePs] != NULL); // Pixel Shader can be NULL

#if defined LCUE_TESSELLATION_ENABLED

#if defined LCUE_TESSELLATION_PSSLC_BUG_WORKAROUND_ENABLED
	if (m_dirtyShader[Gnm::kShaderStageHs])
	{
		assert(m_boundShader[Gnm::kShaderStageHs] != NULL);

		// Note that we are changing a const value here but this is workaround for a PSSL compiler bug
		Gnmx::HsShader* hsShader = (Gnmx::HsShader*)m_boundShader[Gnm::kShaderStageHs];
		if (hsShader->m_hsStageRegisters.m_vgtHosMaxTessLevel == 0)
			hsShader->m_hsStageRegisters.m_vgtHosMaxTessLevel = (6 + 127) << 23; // 64.0f
	}
#endif

#if defined LCUE_TESSELLATION_TG_PATCH_COUNT_AUTO_CONFIGURE_ENABLED
	if (m_dirtyShader[Gnm::kShaderStageLs] || m_dirtyShader[Gnm::kShaderStageHs])
	{
		assert(m_boundShader[Gnm::kShaderStageLs] != NULL);
		assert(m_boundShader[Gnm::kShaderStageHs] != NULL);

		const Gnmx::LsShader* lsShader = (const Gnmx::LsShader*)m_boundShader[Gnm::kShaderStageLs];
		const Gnmx::HsShader* hsShader = (const Gnmx::HsShader*)m_boundShader[Gnm::kShaderStageHs];
		int32_t tgPatchCount, tgLdsSizeInBytes;
		computeTessellationTgPatchCountAndLdsSize(&tgPatchCount, &tgLdsSizeInBytes, lsShader, hsShader, m_tessellationDesiredTgPatchCount);

		// Update required LDS size on LS resource register
		Gnm::LsStageRegisters lsStateRegistersCopy = ((const Gnmx::LsShader*)m_boundShader[Gnm::kShaderStageLs])->m_lsStageRegisters;
		lsStateRegistersCopy.m_spiShaderPgmRsrc2Ls |= (tgLdsSizeInBytes >> 2); // TODO Needs to zero bitfield first?

		// Generate internal TessellationDataConstantBuffer
		assert( sizeof(Gnm::TessellationDataConstantBuffer) % 4 == 0);
		Gnm::TessellationDataConstantBuffer* tessellationInternalConstantBuffer = (Gnm::TessellationDataConstantBuffer*)allocateSpace(m_dcb, sizeof(Gnm::TessellationDataConstantBuffer) / 4);
		tessellationInternalConstantBuffer->init(&hsShader->m_hullStateConstants, lsShader->m_lsStride, tgPatchCount); // TODO: I can manually inline this to speedup things

		// Update tessellation internal CB on API-slot 19
		Gnm::Buffer tessellationCb;
		tessellationCb.initAsConstantBuffer(tessellationInternalConstantBuffer, sizeof(Gnm::TessellationDataConstantBuffer));
		setConstantBuffers(Gnm::kShaderStageHs, kConstantBufferInternalApiSlotForTessellation, 1, &tessellationCb);
		setConstantBuffers(Gnm::kShaderStageVs, kConstantBufferInternalApiSlotForTessellation, 1, &tessellationCb);

		// Update VGT_LS_HS context registers
		m_dcb->setVgtControl(tgPatchCount-1, Gnm::kVgtPartialVsWaveEnable, Gnm::kVgtSwitchOnEopEnable);
		Gnm::TessellationRegisters tessellationVgtLsHsConfiguration;
		tessellationVgtLsHsConfiguration.init(&hsShader->m_hullStateConstants, tgPatchCount);

		m_dcb->setLsShader( &lsStateRegistersCopy );
		m_dcb->setHsShader( &hsShader->m_hsStageRegisters, &tessellationVgtLsHsConfiguration );

		LCUE_IMMEDIATE_CB_UPDATE_ON_GRAPHICS(Gnm::kShaderStageLs, lsShader);
		LCUE_IMMEDIATE_CB_UPDATE_ON_GRAPHICS(Gnm::kShaderStageHs, hsShader);
	}
#else
	if (m_dirtyShader[Gnm::kShaderStageLs])
	{
		const Gnmx::LsShader* lsShader = (const Gnmx::LsShader*)m_boundShader[Gnm::kShaderStageLs];
		const Gnm::LsStageRegisters* lsStateRegisters = &lsShader->m_lsStageRegisters;
		m_dcb->setLsShader( lsStateRegisters );
		
		LCUE_IMMEDIATE_CB_UPDATE_ON_GRAPHICS(Gnm::kShaderStageLs, lsShader);
	}
	if (m_dirtyShader[Gnm::kShaderStageHs])
	{
		const Gnmx::HsShader* hsShader = (const Gnmx::HsShader*)m_boundShader[Gnm::kShaderStageHs];
		Gnm::TessellationRegisters tessellationVgtLsHsConfiguration;
		tessellationVgtLsHsConfiguration.init( &hsShader->m_hullStateConstants, m_tessellationDesiredTgPatchCount);
		m_dcb->setHsShader( &hsShader->m_hsStageRegisters, &tessellationVgtLsHsConfiguration );
		
		LCUE_IMMEDIATE_CB_UPDATE_ON_GRAPHICS(Gnm::kShaderStageHs, hsShader);
	}
#endif

	if (m_dirtyShaderResources[Gnm::kShaderStageLs])
	{
		// Validate after handling immediate CBs (if necessary) and before flushing scratch buffer
		LCUE_VALIDATE_RESOURCE_CHECK_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageLs] );

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageLs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageLs, flushAddress);
		updateLsEsVsPtrsInUserDataSgprs(Gnm::kShaderStageLs, flushAddress);
	}
	if (m_dirtyShaderResources[Gnm::kShaderStageHs])
	{
		// Validate after handling immediate CBs (if necessary) and before flushing scratch buffer
		LCUE_VALIDATE_RESOURCE_CHECK_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageHs] );

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageHs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageHs, flushAddress);
	}
#endif

	const Gnmx::VsShader* vsShader = ((const Gnmx::VsShader*)m_boundShader[Gnm::kShaderStageVs]);
	const Gnmx::PsShader* psShader = ((const Gnmx::PsShader*)m_boundShader[Gnm::kShaderStagePs]);

	if (m_dirtyShader[Gnm::kShaderStageVs])
	{
		m_dcb->setVsShader( &vsShader->m_vsStageRegisters, m_boundShaderModifier[Gnm::kShaderStageVs] );
	}
	if (m_dirtyShader[Gnm::kShaderStageVs] || m_dirtyShaderResources[Gnm::kShaderStageVs])
	{
		LCUE_IMMEDIATE_CB_UPDATE_ON_GRAPHICS(Gnm::kShaderStageVs, vsShader);
	}
	if (m_dirtyShader[Gnm::kShaderStagePs])
	{
		m_dcb->setPsShader( (psShader != NULL)? &psShader->m_psStageRegisters : NULL );
	}
	if (m_dirtyShader[Gnm::kShaderStagePs] || m_dirtyShaderResources[Gnm::kShaderStagePs])
	{
		LCUE_IMMEDIATE_CB_UPDATE_ON_GRAPHICS(Gnm::kShaderStagePs, psShader);
	}

	if (m_dirtyShaderResources[Gnm::kShaderStageVs])
	{
		// Validate after handling immediate CBs (if necessary) and before flushing scratch buffer
		LCUE_VALIDATE_RESOURCE_CHECK_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageVs] );

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStageVs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStageVs, flushAddress);
		updateLsEsVsPtrsInUserDataSgprs(Gnm::kShaderStageVs, flushAddress);
	}
	if (m_dirtyShaderResources[Gnm::kShaderStagePs])
	{
		// Validate after handling immediate CBs (if necessary) and before flushing scratch buffer
		LCUE_VALIDATE_RESOURCE_CHECK_TABLE(	&m_boundShaderResourcesValidation[Gnm::kShaderStagePs] );

		const uint32_t* flushAddress = flushScratchBuffer(Gnm::kShaderStagePs);
		updateCommonPtrsInUserDataSgprs(Gnm::kShaderStagePs, flushAddress);
	}

// This can be enabled to avoid regenerating and resetting the pixel usage table everytime
#if !defined(LCUE_SKIP_VS_PS_SEMANTIC_TABLE)
	if (m_dirtyShader[Gnm::kShaderStageVs] || m_dirtyShader[Gnm::kShaderStagePs])
	{
		Gnmx::VsShader* vsShader = (Gnmx::VsShader*)m_boundShader[Gnm::kShaderStageVs];
		Gnmx::PsShader* psShader = (Gnmx::PsShader*)m_boundShader[Gnm::kShaderStagePs];

		uint32_t psInputs[32];
		if (psShader != NULL && psShader->m_numInputSemantics != 0)
		{
			Gnm::generatePsShaderUsageTable(psInputs,
				vsShader->getExportSemanticTable(), vsShader->m_numExportSemantics,
				psShader->getPixelInputSemanticTable(), psShader->m_numInputSemantics);
			m_dcb->setPsShaderUsage(psInputs, psShader->m_numInputSemantics);
			LCUE_PROFILE_PSSHADER_USAGE_CALL;
		}
	}
#endif

	__builtin_memset(&m_dirtyShader[Gnm::kShaderStagePs], 0, (Gnm::kShaderStageCount-1) * sizeof(bool));
	__builtin_memset(&m_dirtyShaderResources[Gnm::kShaderStagePs], 0, (Gnm::kShaderStageCount-1) * sizeof(bool));
	LCUE_PROFILE_DRAW_CALL;
}


#if defined LCUE_TESSELLATION_ENABLED
void GraphicsCUE::setLsShader(const Gnmx::LsShader* shader, const void* fetchShader, const ShaderResourceOffsets* table)
{
	assert( shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageLs);
	assert( fetchShader != NULL || table->fetchShaderPtrSgpr == 0xFF );
	LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageLs], table );

	// We always mark shaderResourceOffsets as dirty when the shader changes
	m_dirtyShaderResources[Gnm::kShaderStageLs] = true;
	m_dirtyShader[Gnm::kShaderStageLs] |= (m_boundShader[Gnm::kShaderStageLs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStageLs] = table;
	m_boundShader[Gnm::kShaderStageLs] = shader;
	m_boundFetchShader[Gnm::kShaderStageLs] = fetchShader;
}


void GraphicsCUE::setHsShader(const Gnmx::HsShader* shader, const ShaderResourceOffsets* table, int32_t optionalTgPatchCount)
{
	assert( shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageHs);
	LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageHs], table );

	// We always mark shaderResourceOffsets as dirty when the shader changes
	m_dirtyShaderResources[Gnm::kShaderStageHs] = true;
	m_dirtyShader[Gnm::kShaderStageHs] |= (m_boundShader[Gnm::kShaderStageHs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStageHs] = table;
	m_boundShader[Gnm::kShaderStageHs] = shader;
	m_tessellationDesiredTgPatchCount = optionalTgPatchCount;
}
#endif


void GraphicsCUE::setVsShader(const Gnmx::VsShader* shader, uint32_t shaderModifier, const void* fetchShader, const ShaderResourceOffsets* table)
{
	assert( shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageVs);
	assert( fetchShader != NULL || table->fetchShaderPtrSgpr == 0xFF );
	LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageVs], table );

	// We always mark shaderResourceOffsets as dirty when the shader changes
	m_dirtyShaderResources[Gnm::kShaderStageVs] = true;
	m_dirtyShader[Gnm::kShaderStageVs] |= (m_boundShader[Gnm::kShaderStageVs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStageVs] = table;
	m_boundShader[Gnm::kShaderStageVs] = shader;
	m_boundShaderModifier[Gnm::kShaderStageVs] = shaderModifier;
	m_boundFetchShader[Gnm::kShaderStageVs] = fetchShader;
}


void GraphicsCUE::setVsShader(const Gnmx::VsShader* shader, const ShaderResourceOffsets* table)
{
	assert( shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageVs);
	LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageVs], table );

	// We always mark shaderResourceOffsets as dirty when the shader changes
	m_dirtyShaderResources[Gnm::kShaderStageVs] = true;
	m_dirtyShader[Gnm::kShaderStageVs] |= (m_boundShader[Gnm::kShaderStageVs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStageVs] = table;
	m_boundShader[Gnm::kShaderStageVs] = shader;
}


void GraphicsCUE::setPsShader(const Gnmx::PsShader* shader, const ShaderResourceOffsets* table)
{
	assert( (shader == NULL && table == NULL) || (shader != NULL && table != NULL) );
	assert( table == NULL || table->shaderStage == Gnm::kShaderStagePs);
	LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStagePs], table );

	// Special case: if the Pixel Shader is NULL we don't mark shaderResourceOffsets as dirty to prevent flushing the scratch buffer
	m_dirtyShaderResources[Gnm::kShaderStagePs] = (shader != NULL); 
	m_dirtyShader[Gnm::kShaderStagePs] |= (m_boundShader[Gnm::kShaderStagePs] != shader);

	m_boundShaderResourceOffsets[Gnm::kShaderStagePs] = table;
	m_boundShader[Gnm::kShaderStagePs] = shader;
}


void GraphicsCUE::setCsShader(const Gnmx::CsShader* shader, const ShaderResourceOffsets* table)
{
	assert( shader != NULL && table != NULL && table->shaderStage == Gnm::kShaderStageCs );
	LCUE_VALIDATE_RESOURCE_INIT_TABLE( &m_boundShaderResourcesValidation[Gnm::kShaderStageCs], table );

	// We always mark shaderResourceOffsets as dirty when the shader changes
	m_dirtyShaderResources[Gnm::kShaderStageCs] = true;
	m_dirtyShader[Gnm::kShaderStageCs] |= (m_boundShader[Gnm::kShaderStageCs] != shader);
	
	m_boundShaderResourceOffsets[Gnm::kShaderStageCs] = table;
	m_boundShader[Gnm::kShaderStageCs] = shader;
}


void GraphicsCUE::setConstantBuffers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Buffer* buffer)
{
	assert(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxConstantBufferCount);
	assert(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets[(int32_t)shaderStage]->constBufferOffset[currentApiSlot] != 0xFFFF);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].constBufferOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], shaderStage, table->constBufferOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
	LCUE_PROFILE_SET_VSHARP_CALL;
}


void GraphicsCUE::setVertexBuffers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Buffer* buffer)
{
	assert(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxVertexBufferCount);
	assert(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; ++i)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets[(int32_t)shaderStage]->vertexBufferOffset[currentApiSlot] != 0xFFFF);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].vertexBufferOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], shaderStage, table->vertexBufferOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
	LCUE_PROFILE_SET_VSHARP_CALL;
}


void GraphicsCUE::setBuffers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Buffer* buffer)
{
	assert(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxResourceCount);
	assert(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceOffset[currentApiSlot] != 0xFFFF &&
			(m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceOffset[currentApiSlot] & kResourceIsVSharp) != 0);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].resourceOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], shaderStage, table->resourceOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
	LCUE_PROFILE_SET_VSHARP_CALL;
}


void GraphicsCUE::setRwBuffers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Buffer* buffer)
{
	assert(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxRwResourceCount);
	assert(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && buffer != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceOffset[currentApiSlot] != 0xFFFF &&
			(m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceOffset[currentApiSlot] & kResourceIsVSharp) != 0);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].rwResourceOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], shaderStage, table->rwResourceOffset[currentApiSlot], buffer+i, sizeof(Gnm::Buffer));
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
	LCUE_PROFILE_SET_VSHARP_CALL;
}


void GraphicsCUE::setTextures(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Texture* texture)
{
	assert(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxResourceCount);
	assert(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && texture != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceOffset[currentApiSlot] != 0xFFFF &&
			(m_boundShaderResourceOffsets[(int32_t)shaderStage]->resourceOffset[currentApiSlot] & kResourceIsVSharp) == 0);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].resourceOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], shaderStage, table->resourceOffset[currentApiSlot], texture+i, sizeof(Gnm::Texture));
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
	LCUE_PROFILE_SET_TSHARP_CALL;
}


void GraphicsCUE::setRwTextures(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Texture* texture)
{
	assert(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxRwResourceCount);
	assert(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && texture != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceOffset[currentApiSlot] != 0xFFFF &&
			(m_boundShaderResourceOffsets[(int32_t)shaderStage]->rwResourceOffset[currentApiSlot] & kResourceIsVSharp) == 0);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].rwResourceOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], shaderStage, table->rwResourceOffset[currentApiSlot], texture+i, sizeof(Gnm::Texture));
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
	LCUE_PROFILE_SET_TSHARP_CALL;
}


void GraphicsCUE::setSamplers(Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const Gnm::Sampler* sampler)
{
	assert(shaderStage < Gnm::kShaderStageCount && startApiSlot >= 0 && (startApiSlot+apiSlotCount) <= kMaxSamplerCount);
	assert(m_boundShaderResourceOffsets[(int32_t)shaderStage] != NULL && sampler != NULL);

	const ShaderResourceOffsets* table = m_boundShaderResourceOffsets[(int32_t)shaderStage];
	for (int32_t i=0; i<apiSlotCount; i++)
	{
		int32_t currentApiSlot = startApiSlot+i;
		LCUE_ASSERT_OR_CONTINUE(m_boundShaderResourceOffsets[(int32_t)shaderStage]->samplerOffset[currentApiSlot] != 0xFFFF);
		LCUE_VALIDATE_RESOURCE_MARK_USED(m_boundShaderResourcesValidation[shaderStage].samplerOffsetIsBound[currentApiSlot]);
		setDataInUserDataSgprOrMemory(m_dcb, &m_scratchBuffer[0], shaderStage, table->samplerOffset[currentApiSlot], sampler+i, sizeof(Gnm::Sampler));
	}

	m_dirtyShaderResources[(int32_t)shaderStage] = true;
	LCUE_PROFILE_SET_SSHARP_CALL;
}


void ComputeContext::init(uint32_t* dcbBuffer, int32_t dcbBufferSizeInDwords,
	uint32_t* resourceBufferInGarlic, int32_t resourceBufferSizeInDwords, 
	uint32_t* globalInternalResourceTableInGarlic, int32_t globalInternalResourceTableSizeInDwords,
	sce::Gnm::CommandCallbackFunc callbackFunc, void *callbackUserData)
{
	assert(dcbBuffer != NULL);
	assert(dcbBufferSizeInDwords > 0 && dcbBufferSizeInDwords <= Gnm::kIndirectBufferMaximumSizeInBytes);

	m_dcb.init(dcbBuffer, dcbBufferSizeInDwords, callbackFunc, callbackUserData);

	ComputeCUE::init(&resourceBufferInGarlic, 1, resourceBufferSizeInDwords, globalInternalResourceTableInGarlic, globalInternalResourceTableSizeInDwords);
	ComputeCUE::setDcb(&m_dcb);
}


void GraphicsContext::init(uint32_t* dcbBuffer, int32_t dcbBufferSizeInDwords, uint32_t* resourceBufferInGarlic, int32_t resourceBufferSizeInDwords, 
	uint32_t* globalInternalResourceTableInGarlic, int32_t globalInternalResourceTableSizeInDwords, Gnm::CommandCallbackFunc callbackFunc, void* callbackUserData)
{
	assert(dcbBuffer != NULL);
	assert(dcbBufferSizeInDwords > 0 && dcbBufferSizeInDwords <= Gnm::kIndirectBufferMaximumSizeInBytes);

	m_dcb.init(dcbBuffer, dcbBufferSizeInDwords, callbackFunc, callbackUserData);

	GraphicsCUE::init(&resourceBufferInGarlic, 1, resourceBufferSizeInDwords, globalInternalResourceTableInGarlic, globalInternalResourceTableSizeInDwords);
	GraphicsCUE::setDcb(&m_dcb);
}


inline int32_t lcue_min(int32_t a,int32_t b) { return a<b? a : b; }

LCUE_FORCE_INLINE 
	void LCUE::computeTessellationTgPatchCountAndLdsSize(int32_t* outTgPatchCount, int32_t* outTgLdsSizeInBytes, const Gnmx::LsShader* localShader, const Gnmx::HsShader* hullShader,
	int32_t optionalDesiredTgPatchCount)
{
	assert(outTgPatchCount != NULL);
	assert(outTgLdsSizeInBytes != NULL);
	assert(localShader != NULL);
	assert(hullShader != NULL);

	const Gnm::HullStateConstants* hsConstants = &hullShader->m_hullStateConstants;
	uint32_t lsStrideInBytes = localShader->m_lsStride;

	// Check if the patchCount provided is valid
	const int32_t kLdsSizeAlignmentInBytes = 512;
	const int32_t kSimdPerCuCount = 4;
	const int32_t kVgprsPerSimd = 256;
	const int32_t kCuLdsSizeInBytes = 32 * 1024; // TODO: It's 64KB but it appears a TG is still limited somewhere to 32KB
	const int32_t kMaxThreadsPerTg = 1024; // TODO: It should be 2048 but for some reason it's 1024

	int32_t localRequiredVgpr = (localShader->m_lsStageRegisters.m_spiShaderPgmRsrc1Ls & 0x3F) * 4 + 4;
	int32_t hullRequiredVgpr = (hullShader->m_hsStageRegisters.m_spiShaderPgmRsrc1Hs & 0x3F) * 4 + 4;
	assert(localRequiredVgpr%4==0 && hullRequiredVgpr%4==0);

	int32_t patchSizeInBytes = hsConstants->m_numInputCP * lsStrideInBytes + 
		hsConstants->m_numOutputCP * hsConstants->m_cpStride + 
		hsConstants->m_numPatchConst * 16;
	//hsConstants->m_tessFactorStride; // TODO: This is probably the correct way of doing it but I'm doing it the wrong way to match TessellationDataConstantBuffer::init

	int32_t localMaxSimdThreadsLimitedByVgpr = (kVgprsPerSimd / localRequiredVgpr);
	int32_t localMaxPatchesLimitedByVgpr = kSimdPerCuCount * (localMaxSimdThreadsLimitedByVgpr / hsConstants->m_numInputCP);
	int32_t hullMaxSimdThreadsLimitedByVgpr = (kVgprsPerSimd / hullRequiredVgpr);
	int32_t hullMaxPatchesLimitedByVgpr = kSimdPerCuCount * (hullMaxSimdThreadsLimitedByVgpr / hsConstants->m_numThreads);
	int32_t maxTgPatchesLimitedByVgpr = lcue_min(localMaxPatchesLimitedByVgpr, hullMaxPatchesLimitedByVgpr);
	int32_t localMaxPatchesLimitedByThreads = kMaxThreadsPerTg / hsConstants->m_numInputCP;
	int32_t hullMaxPatchesLimitedByThreads = kMaxThreadsPerTg / hsConstants->m_numThreads;
	int32_t maxTgPatchesLimitedByThreads = lcue_min(localMaxPatchesLimitedByThreads, hullMaxPatchesLimitedByThreads);
	int32_t maxTgPatchesLimitedByLds = kCuLdsSizeInBytes / patchSizeInBytes;
	int32_t maxTgPatches = lcue_min( lcue_min(maxTgPatchesLimitedByVgpr, maxTgPatchesLimitedByThreads), maxTgPatchesLimitedByLds);
	maxTgPatches = lcue_min(maxTgPatches, optionalDesiredTgPatchCount);

	// Update required LDS size
	int32_t ldsRequiredSizeInBytes = ((patchSizeInBytes * maxTgPatches) + kLdsSizeAlignmentInBytes - 1) & ~(kLdsSizeAlignmentInBytes-1);
	assert(ldsRequiredSizeInBytes <= kCuLdsSizeInBytes);

	*outTgPatchCount = maxTgPatches;
	*outTgLdsSizeInBytes = ldsRequiredSizeInBytes;
}


int32_t getUsedApiSlotsFromMask(int32_t* outUsedApiSlots, int32_t usedApiSlotsCount, uint32_t mask[4], int32_t maxResourceCount)
{
	int32_t resourceCount = 0;
	for (int32_t slot = 0; slot < usedApiSlotsCount; ++slot)
		if (mask[slot>>5] & (1<<(slot & 0x1F)))
			outUsedApiSlots[resourceCount++] = slot;

#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	// Check if the shader uses any API-slot over the maximum count configured to be handled by the LCUE
	for (int32_t slot = usedApiSlotsCount; slot < maxResourceCount; ++slot)
		assert( (mask[slot>>5] & (1<<(slot & 0x1F))) == 0);
#endif

	return resourceCount;
}


int32_t getUsedApiSlotsFromMask(int32_t* outUsedApiSlots, int32_t usedApiSlotsCount, uint32_t mask, int32_t maxResourceCount)
{
	int32_t resourceCount = 0;
	for (int32_t slot = 0; slot < usedApiSlotsCount; ++slot)
		if ( ((1<<slot) & mask))
			outUsedApiSlots[resourceCount++] = slot;

#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
	// Check if the shader uses any API-slot over the maximum count configured to be handled by the LCUE
	for (int32_t slot = usedApiSlotsCount; slot < maxResourceCount; ++slot)
		assert( ((1<<slot) & mask) == 0);
#endif

	return resourceCount;
}


int32_t LCUE::generateSROTable(ShaderResourceOffsets* outTable, Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct)
{
	// Get the shader pointer and its size from the GNMX shader type
	const uint32_t* shaderRegisters = NULL;
	if (shaderStage == Gnm::kShaderStageCs) { shaderRegisters = (uint32_t*)&((const Gnmx::CsShader*)gnmxShaderStruct)->m_csStageRegisters; }
	else if (shaderStage == Gnm::kShaderStageVs) { shaderRegisters = (uint32_t*)&((const Gnmx::VsShader*)gnmxShaderStruct)->m_vsStageRegisters; }
	else if (shaderStage == Gnm::kShaderStagePs) { shaderRegisters = (uint32_t*)&((const Gnmx::PsShader*)gnmxShaderStruct)->m_psStageRegisters; }
	else if (shaderStage == Gnm::kShaderStageHs) { shaderRegisters = (uint32_t*)&((const Gnmx::HsShader*)gnmxShaderStruct)->m_hsStageRegisters; }
	else if (shaderStage == Gnm::kShaderStageLs) { shaderRegisters = (uint32_t*)&((const Gnmx::LsShader*)gnmxShaderStruct)->m_lsStageRegisters; }

	const void* shaderCode = (void*)( ((uintptr_t)shaderRegisters[0] << 8ULL) | ((uintptr_t)shaderRegisters[1] << 40ULL) );
	int32_t shaderCodeSize = ((const Gnmx::ShaderCommonData*)gnmxShaderStruct)->m_shaderSize;
	assert(shaderCode != NULL && shaderCodeSize > 0);

	return generateSROTable(outTable, shaderStage, gnmxShaderStruct, shaderCode, shaderCodeSize);
}


int32_t LCUE::generateSROTable(ShaderResourceOffsets* outTable, Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct, const void* shaderCode, int32_t shaderCodeSize)
{
	assert(outTable != NULL);
	assert(shaderStage >= Gnm::kShaderStageCs && shaderStage <= Gnm::kShaderStageLs && shaderStage != Gnm::kShaderStageEs && shaderStage != Gnm::kShaderStageGs);

	const uint64_t kNibbleToCount = 0x4332322132212110ull;

	typedef struct
	{
		uint8_t m_signature[7];					// 'OrbShdr'
		uint8_t m_version;						// ShaderBinaryInfoVersion
		uint32_t m_pssl_or_cg : 1;				// 1 = PSSL / Cg, 0 = IL / shtb
		uint32_t m_cached : 1;					// 1 = when compile, debugging source was cached.  May only make sense for PSSL=1
		uint32_t m_type : 4;					// See enum ShaderBinaryType
		uint32_t m_source_type : 2;				// See enum ShaderSourceType
		uint32_t m_length : 24;					// Binary length not counting this structure (i.e. points to top of binary)
		uint8_t  m_chunkUsageBaseOffsetInDW;	// in DW
		uint8_t  m_numInputUsageSlots:6;		// Up to 16 usage slots + 48 extended user data. (note: max of 63 since the ptr to the ext user data takes 2 slots)
		uint8_t  m_reserved2 : 2;				// For future use
		uint8_t  m_reserved3[2];				// For future use
		uint32_t m_shaderHash0;					// Association hash first 4 bytes
		uint32_t m_shaderHash1;					// Association hash second 4 bytes
		uint32_t m_crc32;						// crc32 of shader + this struct, just up till this field
	} ShaderBinaryInfo;

	// Shader footer. There's a footer after the shader code which contains resource info used populate ShaderResourceOffsets
	ShaderBinaryInfo const *shaderBinaryInfo = (ShaderBinaryInfo const*)((uintptr_t)shaderCode + shaderCodeSize - sizeof(ShaderBinaryInfo));
	assert( (*((uint64_t const*)shaderBinaryInfo->m_signature) & kShaderBinaryInfoSignatureMask) == kShaderBinaryInfoSignatureU64);
	
	// Get usage masks and input usage slots
	uint32_t const* usageMasks = (unsigned int const*)((unsigned char const*)shaderBinaryInfo - shaderBinaryInfo->m_chunkUsageBaseOffsetInDW*4);
	int32_t inputUsageSlotsCount = shaderBinaryInfo->m_numInputUsageSlots;
	Gnm::InputUsageSlot const* inputUsageSlots = (Gnm::InputUsageSlot const*)usageMasks - inputUsageSlotsCount;
	 
	//
	__builtin_memset(outTable, 0xFFFF, sizeof(ShaderResourceOffsets));
	outTable->shaderStage = shaderStage;
	int32_t lastUserDataResourceSizeInDwords = 0;
	int32_t requiredMemorySizeInDwords = 0;

	// Here we handle all immediate resources s[1:16] plus s[16:48] (extended user data)
	// Resources that go into the extended user data also have "immediate" usage type, although they are stored in a table (not fetch by the SPI)
	for (uint32_t i=0; i<inputUsageSlotsCount; ++i)
	{
		int32_t apiSlot = inputUsageSlots[i].m_apiSlot;
		int32_t startRegister = inputUsageSlots[i].m_startRegister;
		bool isVSharp = (inputUsageSlots[i].m_resourceType == 0);
		uint16_t vsharpFlag = (isVSharp)? kResourceIsVSharp : 0;

		uint16_t extendedRegisterOffsetInDwords = (startRegister >= 16)? (startRegister-16) : 0;
		requiredMemorySizeInDwords = (requiredMemorySizeInDwords > extendedRegisterOffsetInDwords)?
			requiredMemorySizeInDwords : extendedRegisterOffsetInDwords;

		// Handle immediate resources
		switch (inputUsageSlots[i].m_usageType)
		{
		case Gnm::kShaderInputUsageImmResource:
			assert(apiSlot >= 0 && apiSlot < kMaxResourceCount);
			outTable->resourceOffset[apiSlot] = (startRegister < 16)?
				(kResourceInUserDataSgpr | vsharpFlag | startRegister) : (vsharpFlag | extendedRegisterOffsetInDwords);
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 8;
			break;

		case Gnm::kShaderInputUsageImmRwResource:
			assert(apiSlot >= 0 && apiSlot < kMaxRwResourceCount);
			outTable->rwResourceOffset[apiSlot] = (inputUsageSlots[i].m_startRegister < 16)?
				(kResourceInUserDataSgpr | vsharpFlag | startRegister) : (vsharpFlag | extendedRegisterOffsetInDwords);
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 8;
			break;

		case Gnm::kShaderInputUsageImmSampler:
			assert(apiSlot >= 0 && apiSlot < kMaxSamplerCount);
			outTable->samplerOffset[apiSlot] = (inputUsageSlots[i].m_startRegister < 16)?
				(kResourceInUserDataSgpr | startRegister) : extendedRegisterOffsetInDwords;
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 4;
			break;

		case Gnm::kShaderInputUsageImmConstBuffer:
			assert(apiSlot >= 0 && apiSlot < kMaxConstantBufferCount);
			outTable->constBufferOffset[apiSlot] = (inputUsageSlots[i].m_startRegister < 16)?
				(kResourceInUserDataSgpr | startRegister) : extendedRegisterOffsetInDwords;
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 4;
			break;

		case Gnm::kShaderInputUsageImmVertexBuffer:
			assert(apiSlot >= 0 && apiSlot < kMaxVertexBufferCount);
			outTable->vertexBufferOffset[apiSlot] = (inputUsageSlots[i].m_startRegister < 16)?
				(kResourceInUserDataSgpr | startRegister) : extendedRegisterOffsetInDwords;
			lastUserDataResourceSizeInDwords = (startRegister < 16)? 0 : 4;
			break;
		}
	}

	// Make sure we can fit a T# (if required) in the last userOffset
	requiredMemorySizeInDwords += lastUserDataResourceSizeInDwords;

	// Now handle pointers. Items handled below cannot be found more than once
	for (uint32_t i=0; i<inputUsageSlotsCount; ++i)
	{
		uint8_t maskChunks = inputUsageSlots[i].m_chunkMask;
		uint8_t chunksCount = (kNibbleToCount >> ((maskChunks & 0xF)*4)) & 0xF; (void)chunksCount;
		assert(usageMasks+chunksCount <= (uint32_t const*)shaderBinaryInfo);
		
		// Lets fill the resource indices first
		int32_t usedApiSlots[Gnm::kSlotCountResource]; // Use the size of the biggest resource table
		int32_t usedApiSlotCount;

		// This thing will break if there's more than 1 table for any resource type
		uint8_t startRegister = inputUsageSlots[i].m_startRegister;

		switch (inputUsageSlots[i].m_usageType)
		{
		case Gnm::kShaderInputUsageSubPtrFetchShader:
			assert(inputUsageSlots[i].m_apiSlot == 0);
			outTable->fetchShaderPtrSgpr = startRegister;
			break;

		case Gnm::kShaderInputUsagePtrExtendedUserData:
			assert(inputUsageSlots[i].m_apiSlot == 1); // TODO: Only one slot supported for now
			outTable->userExtendedData1PtrSgpr = startRegister;
			break;

		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			assert(inputUsageSlots[i].m_apiSlot == 0);
			outTable->globalInternalPtrSgpr = startRegister;
			break;

		case Gnm::kShaderInputUsagePtrResourceTable:
		{
			assert(inputUsageSlots[i].m_apiSlot == 0);
			outTable->resourcePtrSgpr = startRegister;
			outTable->resourceArrayOffset = requiredMemorySizeInDwords;

			assert(usageMasks < (uint32_t const*)shaderBinaryInfo);
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
				outTable->resourceOffset[currentApiSlot] = requiredMemorySizeInDwords + currentApiSlot * Gnm::kDwordSizeResource;
			}
			requiredMemorySizeInDwords += (lastUsedApiSlot+1) * Gnm::kDwordSizeResource;
		}
		break;

		case Gnm::kShaderInputUsagePtrRwResourceTable:
		{
			assert(inputUsageSlots[i].m_apiSlot == 0);
			outTable->rwResourcePtrSgpr = startRegister;
			outTable->rwResourceArrayOffset = requiredMemorySizeInDwords;

			assert(usageMasks < (uint32_t const*)shaderBinaryInfo);
			usedApiSlotCount = getUsedApiSlotsFromMask(&usedApiSlots[0], kMaxRwResourceCount, *usageMasks++, Gnm::kSlotCountRwResource);

			int32_t lastUsedApiSlot = usedApiSlots[usedApiSlotCount-1];
			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				int32_t currentApiSlot = usedApiSlots[j];
				outTable->rwResourceOffset[currentApiSlot] = requiredMemorySizeInDwords + currentApiSlot * Gnm::kDwordSizeRwResource;
			}
			requiredMemorySizeInDwords += (lastUsedApiSlot+1) * Gnm::kDwordSizeRwResource;
		}
		break;

		case Gnm::kShaderInputUsagePtrConstBufferTable:
		{
			assert(inputUsageSlots[i].m_apiSlot == 0); // TODO BE: Get rid of?
			outTable->constBufferPtrSgpr = startRegister;
			outTable->constBufferArrayOffset = requiredMemorySizeInDwords;
			
			assert(usageMasks < (uint32_t const*)shaderBinaryInfo);
			usedApiSlotCount = getUsedApiSlotsFromMask(&usedApiSlots[0], kMaxConstantBufferCount, *usageMasks++, Gnm::kSlotCountConstantBuffer);

			int32_t lastUsedApiSlot = usedApiSlots[usedApiSlotCount-1];
			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				int32_t currentApiSlot = usedApiSlots[j];
				outTable->constBufferOffset[currentApiSlot] = requiredMemorySizeInDwords + currentApiSlot * Gnm::kDwordSizeConstantBuffer;
			}
			requiredMemorySizeInDwords += (lastUsedApiSlot+1) * Gnm::kDwordSizeConstantBuffer;
		}
		break;

		case Gnm::kShaderInputUsagePtrSamplerTable:
		{
			assert(inputUsageSlots[i].m_apiSlot == 0);
			outTable->samplerPtrSgpr = startRegister;
			outTable->samplerArrayOffset = requiredMemorySizeInDwords;

			assert(usageMasks < (uint32_t const*)shaderBinaryInfo);
			usedApiSlotCount = getUsedApiSlotsFromMask(&usedApiSlots[0], kMaxSamplerCount, *usageMasks++, Gnm::kSlotCountSampler);

			int32_t lastUsedApiSlot = usedApiSlots[usedApiSlotCount-1];
			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				int32_t currentApiSlot = usedApiSlots[j];
				outTable->samplerOffset[currentApiSlot] = requiredMemorySizeInDwords + currentApiSlot * Gnm::kDwordSizeSampler;
			}
			requiredMemorySizeInDwords += (lastUsedApiSlot+1) * Gnm::kDwordSizeSampler;
		}
		break;

		case Gnm::kShaderInputUsagePtrVertexBufferTable:
		{
			assert(shaderStage == Gnm::kShaderStageLs || shaderStage == Gnm::kShaderStageEs || shaderStage == Gnm::kShaderStageVs);
			assert(inputUsageSlots[i].m_apiSlot == 0);
			outTable->vertexBufferPtrSgpr = startRegister;
			outTable->vertexBufferArrayOffset = requiredMemorySizeInDwords;
			
			assert(usageMasks <= (uint32_t const*)shaderBinaryInfo);
			usedApiSlotCount = 0;
			if (shaderStage == Gnm::kShaderStageVs)
				usedApiSlotCount = ((Gnmx::VsShader*)gnmxShaderStruct)->m_numInputSemantics;
			else if (shaderStage == Gnm::kShaderStageLs)
				usedApiSlotCount = ((Gnmx::LsShader*)gnmxShaderStruct)->m_numInputSemantics;
			else if (shaderStage == Gnm::kShaderStageEs)
				usedApiSlotCount = ((Gnmx::EsShader*)gnmxShaderStruct)->m_numInputSemantics;
			// Check if the shader uses any API-slot over the maximum count configured to be handled by the LCUE
			assert(usedApiSlotCount == 0 || usedApiSlotCount <= LCUE::kMaxVertexBufferCount);

			for (uint8_t j=0; j<usedApiSlotCount; j++)
			{
				outTable->vertexBufferOffset[j] = requiredMemorySizeInDwords + j * Gnm::kDwordSizeVertexBuffer;
			}
			requiredMemorySizeInDwords += usedApiSlotCount * Gnm::kDwordSizeVertexBuffer;
		}
		break;
		}
	}
	outTable->requiredBufferSizeInDwords = requiredMemorySizeInDwords;

	// Just checking for unhandled input data
	for (uint32_t i=0; i<inputUsageSlotsCount; ++i)
	{
		switch (inputUsageSlots[i].m_usageType)
		{
		case Gnm::kShaderInputUsageImmResource:
		case Gnm::kShaderInputUsageImmRwResource:
		case Gnm::kShaderInputUsageImmSampler:
		case Gnm::kShaderInputUsageImmConstBuffer:
		case Gnm::kShaderInputUsageImmVertexBuffer:
		case Gnm::kShaderInputUsageSubPtrFetchShader:
		case Gnm::kShaderInputUsagePtrExtendedUserData:
		case Gnm::kShaderInputUsagePtrResourceTable:
		case Gnm::kShaderInputUsagePtrRwResourceTable:
		case Gnm::kShaderInputUsagePtrConstBufferTable:
		case Gnm::kShaderInputUsagePtrVertexBufferTable:
		case Gnm::kShaderInputUsagePtrSamplerTable:
		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			// Handled
			break;

		default:
			// Not handled yet
			assert(false);
			break;
		}
	}
	
	return 0;
}
