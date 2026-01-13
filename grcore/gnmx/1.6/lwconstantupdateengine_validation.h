/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_LCUE_VALIDATION_H)
#define _SCE_GNMX_LCUE_VALIDATION_H

#include <stdint.h>
#include <gnm.h>

namespace sce
{
	namespace Gnmx
	{
		namespace LightweightConstantUpdateEngine
		{
			/** @brief Error codes returned when resource bindings fail */
			typedef enum ResourceBindingError
			{
				kErrorOk							= 0x00000000,	///< No error found

				// Validation method parameters
				kErrorInvalidParameters				= 0x00000001,	///< Parameters passed to the validation function are not valid

				// Memory errors
				kErrorMemoryNotMapped				= 0x00000100,	///< Used memory is not mapped
				kErrorMemoryProtectionMismatch		= 0x00000200,	///< Used memory is mapped but with different protection flags from the specified ones
				kErrorResourceMemoryTypeMismatch	= 0x00000400,	///< Used memory type in resource is not compatible with resource type (e.g. using RO memory type with a RWTexture)
		
				// V# errors
				kErrorVSharpDataFormatIsNotValid	= 0x00001000,	///< V# has an invalid data format
				kErrorVSharpStrideIsNotValid		= 0x00002000,	///< V# has an invalid stride
				kErrorVSharpNumElementsIsNotValid	= 0x00004000,	///< V# has an invalid number of elements
				kErrorVSharpInvalidBufferType		= 0x00010000,	///< V# used as either Byte/Data/RegularBuffer but invalid for all of those types

				// # not valid
				kErrorVSharpIsNotValid				= 0x01000000,	///< V# is not valid (likely not initialized)
				kErrorTSharpIsNotValid				= 0x02000000,	///< T# is not valid (likely not initialized)
				kErrorSSharpIsNotValid				= 0x04000000,	///< S# is not valid (likely not initialized)
			} ResourceBindingError;

#if !defined(DOXYGEN_IGNORE)
			/**
			 * @brief Check memory area used by the resource is mapped and has the correct protection flags
			 * @param buffer Buffer (V#) object to validate
			 * @param memoryProtectionFlag Memory protection flag of the resource
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateResourceMemoryArea(const sce::Gnm::Buffer* buffer, int32_t memoryProtectionFlag);

			/**
			 * @brief Check memory area used by the resource is mapped and has the correct protection flags
			 * @param texture Texture (T#) object to validate
			 * @param memoryProtectionFlag Memory protection flag of the resource
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateResourceMemoryArea(const sce::Gnm::Texture* texture, int32_t memoryProtectionFlag);

			/**
			 * @brief Check that the memory area used by the resource is mapped and has the correct protection flags.
			 * @param resourceBeginAddr Base address of the resource
			 * @param resourceSizeInBytes Size of the resource in bytes
			 * @param memoryProtectionFlag Memory protection flag of the resource
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateResourceMemoryArea(const void* resourceBeginAddr, uint64_t resourceSizeInBytes, int32_t memoryProtectionFlag);

			/**
			 * @brief Check that a valid constant buffer is bound
			 * @param buffer Constant buffer object to be checked
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateConstantBuffer(const sce::Gnm::Buffer* buffer);

			/**
			 * @brief check that a valid vertex buffer is bound
			 * @param buffer Vertex buffer object to be checked
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateVertexBuffer(const sce::Gnm::Buffer* buffer);

			/**
			 * @brief Check that a valid buffer is bound
			 * @param buffer Buffer object to be checked
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateBuffer(const sce::Gnm::Buffer* buffer);

			/**
 			 * @brief Check that a valid rwbuffer is bound
			 * @param buffer Rwbuffer object to be checked
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateRwBuffer(const sce::Gnm::Buffer* buffer);

			/**
			 * @brief Check that a valid texture is bound
			 * @param texture Texture object to be checked
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateTexture(const sce::Gnm::Texture* texture);

			/**
			 * @brief Check that a valid rwtexture is bound
			 * @param texture RwTexture buffer object to be checked
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateRwTexture(const sce::Gnm::Texture* texture);

			/**
			 * @brief Check that a valid sampler is bound
			 * @param sampler Sampler object to be checked
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateSampler(const sce::Gnm::Sampler* sampler);

			/**
			 * @brief Check for valid parameters specific to all V#'s
			 * @param buffer Generic V# to be checked
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateGenericVsharp(const sce::Gnm::Buffer* buffer);

			/**
			 * @brief Check a single entry in the Global Internal Descriptor Table.
			 * @param resourceType The global resource type
			 * @param globalInternalTableMemory Pointer to the global internal table
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateGlobalInternalTableResource(sce::Gnm::ShaderGlobalResourceType resourceType, const void* globalInternalTableMemory);

			/**
			 * @brief Check all items in the Global Internal Table that are used by the specified shader.
			 * @param globalInternalTableMemory Pointer to the global internal table
			 * @param gnmxShaderStruct Pointer to the gnmx shader structure
			 * @return Returns kErrorOk (=0) if no error is found, otherwise returns a combination of errors defined by LcueValidationErrors
			 */
			ResourceBindingError validateGlobalInternalResourceTableForShader(const void* globalInternalTableMemory, const void* gnmxShaderStruct);
#endif // !defined(DOXYGEN_IGNORE) 
		} // LCUE
	} // Gnmx
} // sce

#endif // _SCE_GNMX_LCUE_VALIDATION_H