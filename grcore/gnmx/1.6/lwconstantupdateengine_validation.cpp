/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(SCE_GNM_OFFLINE_MODE) // LCUE isn't supported off line

#include <sys/dmem.h>
#include <sys/errno.h>
#include <sceerror.h>

#include "grcore/gnmx/lwconstantupdateengine_validation.h"

using namespace sce;
using namespace Gnmx;
using namespace Gnmx::LightweightConstantUpdateEngine;

ResourceBindingError LightweightConstantUpdateEngine::validateResourceMemoryArea(const Gnm::Buffer* buffer, int32_t memoryProtectionFlag)
{
	return validateResourceMemoryArea(buffer->getBaseAddress(), buffer->getSize(), memoryProtectionFlag);
}


ResourceBindingError LightweightConstantUpdateEngine::validateResourceMemoryArea(const Gnm::Texture* texture, int32_t memoryProtectionFlag)
{
	uint64_t textureSizeInBytes;
	Gnm::AlignmentType textureAlign;
	int32_t status = GpuAddress::computeTotalTiledTextureSize(&textureSizeInBytes, &textureAlign, texture);
	SCE_GNM_UNUSED(status);
	SCE_GNM_ASSERT(status == GpuAddress::kStatusSuccess);				// If fails: Texture is invalid.

	return validateResourceMemoryArea(texture->getBaseAddress(), textureSizeInBytes, memoryProtectionFlag);
}


ResourceBindingError LightweightConstantUpdateEngine::validateResourceMemoryArea(const void* resourceBeginAddr, uint64_t resourceSizeInBytes, int32_t memoryProtectionFlag)
{
	SCE_GNM_ASSERT(resourceBeginAddr != NULL);
//	SCE_GNM_ASSERT(resourceSizeInBytes > 0);	// zero sized resources can be valid, if a user wants to bind a NULL buffer (only valid if the GPU will not read the resource)
	SCE_GNM_ASSERT(memoryProtectionFlag != 0);
	if (resourceBeginAddr == NULL || resourceSizeInBytes <= 0 || memoryProtectionFlag == 0)
		return kErrorInvalidParameters;

	void* resourceEndAddr = (void*)((uint64_t)resourceBeginAddr + resourceSizeInBytes);
	do 
	{
		void* blockStart;
		void* blockEnd;
		int32_t blockProtection;
		int32_t status = sceKernelQueryMemoryProtection((void*)resourceBeginAddr, &blockStart, &blockEnd, &blockProtection);

		SCE_GNM_ASSERT(status == SCE_OK);														// If fails: Memory is not mapped.
		SCE_GNM_ASSERT( (blockProtection & memoryProtectionFlag) == memoryProtectionFlag );	// If fails: Memory protection doesn't match the desired one.
		if (status != SCE_OK)
			return kErrorMemoryNotMapped;
		if ((blockProtection & memoryProtectionFlag) != memoryProtectionFlag)
			return kErrorMemoryProtectionMismatch;

		// Go to the next block
		resourceBeginAddr = (void*)( (uint64_t)blockEnd + 1 );

	} while (resourceBeginAddr < resourceEndAddr);

	return kErrorOk;
}


ResourceBindingError LightweightConstantUpdateEngine::validateGlobalInternalTableResource(Gnm::ShaderGlobalResourceType resourceType, const void* globalInternalTableMemory)
{
	// TODO
	(void)resourceType; (void)globalInternalTableMemory;
	SCE_GNM_ASSERT(false);

	return LightweightConstantUpdateEngine::kErrorOk;
}


ResourceBindingError LightweightConstantUpdateEngine::validateGlobalInternalResourceTableForShader(const void* globalInternalTableMemory, const void* gnmxShaderStruct)
{
	// TODO
	(void)globalInternalTableMemory; (void)gnmxShaderStruct;
	SCE_GNM_ASSERT(false);

	return LightweightConstantUpdateEngine::kErrorOk;
}


ResourceBindingError LightweightConstantUpdateEngine::validateGenericVsharp(const sce::Gnm::Buffer* buffer)
{
	uint32_t err = kErrorOk;
	
	if (!buffer->isBuffer())
		err |= kErrorVSharpIsNotValid;

	const Gnm::DataFormat dataFormat = buffer->getDataFormat();
	const uint32_t stride = buffer->getStride();
	const uint32_t numElements = buffer->getNumElements();
	const uint32_t bufferSize = buffer->getSize();

	// Check if data format is valid
	if (!dataFormat.supportsBuffer())
		err |= kErrorVSharpDataFormatIsNotValid;

	bool isStrideValid = (stride == 0) || (stride >= dataFormat.getBytesPerElement());
	if (!isStrideValid)
		err |= kErrorVSharpStrideIsNotValid;

	// If stride is zero, numElements == bufferSizeInBytes
	bool isNumberElementsValid = ( numElements > 0 && (stride > 0 || numElements == bufferSize));
	if (!isNumberElementsValid)
		err |= kErrorVSharpNumElementsIsNotValid;

	return (ResourceBindingError)err;
}


ResourceBindingError LightweightConstantUpdateEngine::validateConstantBuffer(const sce::Gnm::Buffer* buffer)
{
	uint32_t err = validateGenericVsharp(buffer);
	err |= validateResourceMemoryArea(buffer, SCE_KERNEL_PROT_GPU_READ);
	// Can use any resource memory type AFAIK

	// Stride and format are fixed for constant buffers
	if (buffer->getStride() != 16)
		err |= kErrorVSharpStrideIsNotValid;
	if (buffer->getDataFormat().m_asInt != Gnm::kDataFormatR32G32B32A32Float.m_asInt)
		err |= kErrorVSharpDataFormatIsNotValid;

	return (ResourceBindingError)err;
}


ResourceBindingError LightweightConstantUpdateEngine::validateVertexBuffer(const sce::Gnm::Buffer* buffer)
{
	uint32_t err = validateGenericVsharp(buffer);
	err |= validateResourceMemoryArea(buffer, SCE_KERNEL_PROT_GPU_READ);
	// Can use any resource memory type AFAIK

	return (ResourceBindingError)err;
}


ResourceBindingError LightweightConstantUpdateEngine::validateBuffer(const sce::Gnm::Buffer* buffer)
{
	uint32_t err = validateGenericVsharp(buffer);
	err |= validateResourceMemoryArea(buffer, SCE_KERNEL_PROT_GPU_READ);
	// Can use any resource memory type AFAIK

	// DataBuffer, RegularBuffer, ByteBuffer
	const Gnm::DataFormat dataFormat = buffer->getDataFormat();
	bool isValidByteBuffer = (buffer->getStride() == 0 && dataFormat.m_asInt == Gnm::kDataFormatR8Uint.m_asInt);
	bool isValidDataBuffer = (buffer->getStride() > 0);
	bool isValidRegularBuffer = (dataFormat.m_asInt == Gnm::kDataFormatR32Float.m_asInt);
	if (!isValidByteBuffer && !isValidDataBuffer && !isValidRegularBuffer)
		err |= kErrorVSharpInvalidBufferType;

	return (ResourceBindingError)err;
}


ResourceBindingError LightweightConstantUpdateEngine::validateRwBuffer(const sce::Gnm::Buffer* buffer)
{
	uint32_t err = validateGenericVsharp(buffer);
	err |= validateResourceMemoryArea(buffer, SCE_KERNEL_PROT_GPU_READ|SCE_KERNEL_PROT_GPU_WRITE);
	
	if (buffer->getResourceMemoryType() == Gnm::kResourceMemoryTypeRO)
		err |=  kErrorResourceMemoryTypeMismatch;

	// ByteBuffer, DataBuffer, RegularBuffer
	const Gnm::DataFormat dataFormat = buffer->getDataFormat();
	bool isValidByteBuffer = (buffer->getStride() == 0 && dataFormat.m_asInt == Gnm::kDataFormatR8Uint.m_asInt);
	bool isValidDataBuffer = (buffer->getStride() > 0);
	bool isValidRegularBuffer = (dataFormat.m_asInt == Gnm::kDataFormatR32Float.m_asInt);
	if (!isValidByteBuffer && !isValidDataBuffer && !isValidRegularBuffer)
		err |= kErrorVSharpInvalidBufferType;

	return (ResourceBindingError)err;
}


ResourceBindingError LightweightConstantUpdateEngine::validateTexture(const sce::Gnm::Texture* texture)
{
	uint32_t err = kErrorOk;

	if (!texture->isTexture())
		err |= kErrorTSharpIsNotValid;

	err |= validateResourceMemoryArea(texture, SCE_KERNEL_PROT_GPU_READ);
	// Can use any resource memory type AFAIK

	return (ResourceBindingError)err;
}


ResourceBindingError LightweightConstantUpdateEngine::validateRwTexture(const sce::Gnm::Texture* texture)
{
	uint32_t err = kErrorOk;

	if (!texture->isTexture())
		err |= kErrorTSharpIsNotValid;

	err |= validateResourceMemoryArea(texture, SCE_KERNEL_PROT_GPU_READ|SCE_KERNEL_PROT_GPU_WRITE);
	if (texture->getResourceMemoryType() == Gnm::kResourceMemoryTypeRO)
		err |= kErrorResourceMemoryTypeMismatch;

	return (ResourceBindingError)err;
}


ResourceBindingError LightweightConstantUpdateEngine::validateSampler(const sce::Gnm::Sampler* sampler)
{
	// TODO
	(void)sampler;
	uint32_t err = kErrorOk;

	return (ResourceBindingError)err;
}

#endif // !defined(SCE_GNM_OFFLINE_MODE)