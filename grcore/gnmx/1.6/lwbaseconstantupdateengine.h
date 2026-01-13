/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_LWBASECONSTANTUPDATEENGINE_H)
#define _SCE_GNMX_LWBASECONSTANTUPDATEENGINE_H

#include <gnm.h>
#include "grcore/gnmx/shaderbinary.h"
#include "grcore/gnmx/helpers.h"
#include <stdint.h>

/** @brief If defined (enabled by default), clear the HW KCache/L1/L2 caches at init time and at the end of each LCUE buffer swap in swapBuffers(). 
 *  Note: This will result in the flushing and invalidation of the GPU KCache/L1/L2 caches, which may slightly affect performance of any asynchronous compute jobs.
 *  In this case you can disable this feature, but it's advised that you manually flush and invalidate the GPU KCache/L1/L2 caches before the pending LCUE buffer
 *  is consumed by the GPU.
 */
#define SCE_GNM_LCUE_CLEAR_HARDWARE_KCACHE

/** @brief If defined (disabled by default), and assert will be thrown if a resource that is not used by the current shader is bound.
 *  By default the LCUE will skip resources if the binding does not exist in the ShaderResourceOffsetTable.  
 *  Use this assert to catch unused bindings your engine makes to reduce LCUE call overhead.
 *  Note: to enable, you will need to recompile Gnmx library (Debug) with this flag enabled.
 */
//#define SCE_GNM_LCUE_ASSERT_ON_MISSING_RESOURCE_BINDING_ENABLED

#if !defined(DOXYGEN_IGNORE)

/** @brief Validate resource descriptor data (i.e. checks if V#/T# is valid) as they are bound.
 *  Note: while this is enabled, LCUE will run slower. Enabled in Gnmx debug
 */
#include "grcore/gnmx/lwconstantupdateengine_validation.h"
#define SCE_GNM_LCUE_VALIDATE_CONSTANT_BUFFER(a)	SCE_GNM_ASSERT( sce::Gnmx::LightweightConstantUpdateEngine::validateConstantBuffer(a) == LightweightConstantUpdateEngine::kErrorOk )
#define SCE_GNM_LCUE_VALIDATE_VERTEX_BUFFER(a)		SCE_GNM_ASSERT( sce::Gnmx::LightweightConstantUpdateEngine::validateVertexBuffer(a)	== LightweightConstantUpdateEngine::kErrorOk )
#define SCE_GNM_LCUE_VALIDATE_BUFFER(a)				SCE_GNM_ASSERT( sce::Gnmx::LightweightConstantUpdateEngine::validateBuffer(a) == LightweightConstantUpdateEngine::kErrorOk )
#define SCE_GNM_LCUE_VALIDATE_RWBUFFER(a)			SCE_GNM_ASSERT( sce::Gnmx::LightweightConstantUpdateEngine::validateRwBuffer(a) == LightweightConstantUpdateEngine::kErrorOk )
#define SCE_GNM_LCUE_VALIDATE_TEXTURE(a)			SCE_GNM_ASSERT( sce::Gnmx::LightweightConstantUpdateEngine::validateTexture(a) == LightweightConstantUpdateEngine::kErrorOk )
#define SCE_GNM_LCUE_VALIDATE_RWTEXTURE(a)			SCE_GNM_ASSERT( sce::Gnmx::LightweightConstantUpdateEngine::validateRwTexture(a) == LightweightConstantUpdateEngine::kErrorOk )
#define SCE_GNM_LCUE_VALIDATE_SAMPLER(a)			SCE_GNM_ASSERT( sce::Gnmx::LightweightConstantUpdateEngine::validateSampler(a) == LightweightConstantUpdateEngine::kErrorOk )

/** @brief If defined, check if all resources expected by the current bound shaders are bound when a "draw" or "dispatch" is issued. Also checks if a shader expects more input
 * resources than the LCUE was configure to handle when "generateShaderResourceOffsetTable" is called.
 *  Note: to enable, you will need to recompile Gnmx library with this flag enabled.
 */
// #define SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED

#ifdef __ORBIS__
#define SCE_GNM_LCUE_NOT_SUPPORTED __attribute__((unavailable("Method or attribute is not supported by the Lightweight ConstantUpdateEngine.")))
#else // __ORBIS__
#define SCE_GNM_LCUE_NOT_SUPPORTED 
#endif // __ORBIS__

#endif // !defined(DOXYGEN_IGNORE)

namespace sce
{
	namespace Gnmx
	{
		/** @brief The Lightweight ConstantUpdateEngine (LCUE), like the CUE, manages dynamic mappings of shader resource descriptors in memory, but does so in a more CPU efficient manner.
		 */
		namespace LightweightConstantUpdateEngine
		{
#if !defined(DOXYGEN_IGNORE)
			/*	GLOSSARY/DEFINITIONS:
				-------------------------------------------------------------------------------------------------------------------------
				User Data (UD): contains 16 SGPRs s[0:15] in a shader core, which are pre-fetched by the SPI before execution starts.
				UD is set through PM4 packets, thus, you don't need to maintain copies of those resource descriptors in memory.
			 
				User Extended Data (UED): contains up to 48 DWORDs (SW limitation, size can be adjusted see Gnm Overview for more details) 
				and does not have a specific type (e.g. V#/T#/S# are supported), and is stored in memory. If the UED is used, 
				the UD will store a pointer to the start of each UED.
			 
				Flat-table (or array): up to [N] DWORDs (SW limitation) for a specific resource type, five tables available. Can be seen 
				as a contiguous zero-based array that may (or may not) have gaps, and is stored in memory.
			 
				Flat-tables: resource (textures/buffers), rw-resource (textures/buffers), sampler, constant buffer and vertex buffer.
			 
				Memory based resources: UED or flat-table based resources.
			*/

			// Internal constants defined for the LCUE 

			// Defines how many resources of each available type will be stored and handled by the LCUE, this also defines the maximum valid API-slot
			// Note: resource types are textures/buffers, rw_textures/rw_buffers, samplers, vertex_buffers and constant_buffers
			const int32_t kMaxResourceCount			= 16;	///< Default value is 16 but can be increased, PSSL compiler limit is 128
			const int32_t kMaxRwResourceCount		= 16;	///< Default value is 16, PSSL compiler limit is 16
			const int32_t kMaxSamplerCount			= 16;	///< Default value is 16, PSSL compiler limit is 16
			const int32_t kMaxVertexBufferCount		= 16;	///< Default value is 16 but can be increased, PSSL compiler limit is 32
			const int32_t kMaxConstantBufferCount	= 20;	///< Default value is 20, PSSL compiler limit is 20, Note: Because API-slots 15-19 are all reserved this value should remain 20
			const int32_t kMaxStreamOutBufferCount	= 4;	///< Default value is  4, PSSL compiler limit is 4
			const int32_t kMaxUserDataCount			= 16;	///< Default value is 16, PSSL compiler limit is 16
			const int32_t kMaxSrtUserDataCount	    = 8;	///< Default value is  8, PSSL compiler limit is 8

			const int32_t kDefaultGlobalInternalTablePtrSgpr	= 0;	///< Default SGPR in PSSL, Note: it has lower priority than FetchPtr (sgpr would be s[4:5], after FetchPtr and VbPtr)
			const int32_t kDefaultFetchShaderPtrSgpr			= 0;	///< Default SGPR in PSSL
			const int32_t kDefaultStreamOutTablePtrSgpr			= 2;	///< Default SGPR in PSSL
			const int32_t kDefaultVertexBufferTablePtrSgpr		= 2;	///< Default SGPR in PSSL

			const int32_t kResourceInUserDataSgpr	= 0x8000;	///< In User data resource Mask
			const int32_t kResourceIsVSharp			= 0x4000;	///< VShapr resource Mask Note: only used/available for immediate resources
			const int32_t kResourceValueMask		= 0x3FFF;

			// 6KB is enough to store anything you can bind to a GPU shader stage, all counted in DWORDS
			const int32_t kGpuStageBufferSizeInDwords			= (6*1024) / sizeof(uint32_t);	///< size of Single buffer Stage
			const int32_t kComputeScratchBufferSizeInDwords		= kGpuStageBufferSizeInDwords;	///< Compute Scratch buffer size
			const int32_t kGraphicsScratchBufferSizeInDwords	= sce::Gnm::kShaderStageCount * kGpuStageBufferSizeInDwords; ///< Graphics Scratch buffer size (all graphics shader stages)
			const int32_t kGlobalInternalTableSizeInDwords		= sce::Gnm::kShaderGlobalResourceCount * sizeof(sce::Gnm::Buffer) / sizeof(uint32_t);

			// Internal constant buffers that are expected at fixed API-slots
			const int32_t kConstantBufferInternalApiSlotForTessellation = 19; ///< Tessellation constant buffer (with strides for LDS data) fixed API-slot (HS,VS/ES gpu stages).
			const int32_t kConstantBufferInternalApiSlotForEmbeddedData = 15; ///< Immediate/Embedded constant buffer fixed API-slot (any GPU stage).
	
			const int32_t kGpuStageAverageBufferSizeInDwords	= 1024 / sizeof(uint32_t);	///< Estimated size of memory based resources for one GPU stage (can hold 16T + 16VB + 4RW_T + 4S + 4CB).
			const int32_t kMinResourceBufferSizeInDwords		= 500 * kGpuStageAverageBufferSizeInDwords; ///< Require a minimum buffer size that can store at least 500 GPU stages.
			const int32_t kMaxResourceBufferCount				= 4;

			const uint64_t kShaderBinaryInfoSignatureMask		= 0x00ffffffffffffffLL;
			const uint64_t kShaderBinaryInfoSignatureU64		= 0x007264685362724fLL;
#endif	// !defined(DOXYGEN_IGNORE)

			
			/** @brief Used to accelerate the bindings of required shader resources.
			 *  By caching input information of a shader. 
			 */
			class InputResourceOffsets
			{
			public:
				uint16_t requiredBufferSizeInDwords;	///< How much memory needs to be reserved to store all memory-based resources (things not set through PM4).
				uint8_t	shaderStage;					///< Shader Stage (LS/HS/ES/GS/VS/PS) for the shader resources offsets.
				bool	isSrtShader;					///< If true, shader makes use of SRT's.

				// For each available shader-resource-ptr, store the starting SGPR s[0:254] where it'll be set (0xFF means not used). Pointers take 2 SGPRs (64b) and must be 2DW aligned
				uint8_t	fetchShaderPtrSgpr;				///< SGPR containing the fetch shader pointer. (If exists, s[0:1] is always used).
				uint8_t	vertexBufferPtrSgpr;			///< SGPR containing the vertex buffer table pointer. (If exists, s[2:3] is always used, only in vertex pipeline).
				uint8_t streamOutPtrSgpr;				///< SGPR containing the stream out buffer pointer. (If exists, s[2:3] is always used, only in Geometry pipeline).
				uint8_t userExtendedData1PtrSgpr;		///< SGPR containing the user extended data table pointer.
//				uint8_t userInternalSrtDataPtrSgpr;		///< *Note: Not supported for now*.
				uint8_t constBufferPtrSgpr;				///< SGPR containing the constant buffer table pointer.
				uint8_t resourcePtrSgpr;				///< SGPR containing the resource buffer table pointer.
				uint8_t rwResourcePtrSgpr;				///< SGPR containing the read/write resource buffer table pointer.
				uint8_t samplerPtrSgpr;					///< SGPR containing the sampler buffer table pointer.
				uint8_t globalInternalPtrSgpr;			///< SGPR containing the global internal pointer, either stored in s[0:1] or s[4:5].
				uint8_t appendConsumeCounterSgpr;		///< SGPR containing the 32bit value address and size used from GDS.
				uint8_t userSrtDataSgpr;				///< SGPR containing the start offset of the SRT Data Buffer.
				uint8_t userSrtDataCount;				///< Stores the number of DWORDS in use by the SRT Data Buffer (size will be between 1-8).

				// For each available shader-resource-flat-table (aka array), store the memory offset (from the start of the buffer) to the beginning of its flat-table (0xFFFF means it's not used).
				// Note: arrays are 0 indexed but the user can skip/set any index inside the range, allowing gaps at any place. This accelerates setting the pointer to the beginning of flat-tables.
				uint16_t constBufferArrayDwOffset;		///< Constant buffer table offset into the main buffer.
				uint16_t vertexBufferArrayDwOffset;		///< Vertex buffer table offset into the main buffer.
				uint16_t resourceArrayDwOffset;			///< Resource buffer table offset into the main buffer.
				uint16_t rwResourceArrayDwOffset;		///< Read/Write resource buffer table offset into the main buffer.
				uint16_t samplerArrayDwOffset;			///< Sampler buffer table offset into the main buffer.
				uint16_t streamOutArrayDwOffset;		///< Stream out buffer table offset into the main buffer, only for Geometry pipeline.

				// For each logical shader API slot, store either: an offset to a memory location, or a User Data (UD) SGPR where the resource should be set.
				// Note: if (item[i]&kResourceInUserDataSgpr) it's set directly into s[0:15] using PM4 packets, otherwise it's copied into the scratch buffer using the offset.
				uint16_t resourceDwOffset[kMaxResourceCount];			///< Start offset of a resource in the resource buffer table or user data.
				uint16_t rwResourceDwOffset[kMaxRwResourceCount];		///< Start offset of a resource in the read/write resource buffer table or user data.
				uint16_t samplerDwOffset[kMaxSamplerCount];				///< Start offset of a sampler in the sampler buffer table or user data.
				uint16_t constBufferDwOffset[kMaxConstantBufferCount];	///< Start offset of a constant buffer in the constant buffer table or user data.
				uint16_t vertexBufferDwOffset[kMaxVertexBufferCount];	///< Start offset of a vertex array in the vertex buffer table or user data.
				uint16_t streamOutDwOffset[kMaxVertexBufferCount];		///< Start offset of a stream out buffer in the stream out buffer table or user data. only for Geometry pipeline.

//				kShaderInputUsageImmAluFloatConst					// Immediate float const (scalar or vector). Not Supported
//				kShaderInputUsageImmAluBool32Const					// 32 immediate Booleans packed into one UINT. Not Supported
			};

#if !defined(DOXYGEN_IGNORE)
			/** @brief Used to validate shader resource bindings */
			struct ShaderResourceBindingValidation
			{
				bool constBufferOffsetIsBound[kMaxConstantBufferCount];
				bool vertexBufferOffsetIsBound[kMaxVertexBufferCount];
				bool resourceOffsetIsBound[kMaxResourceCount];
				bool rwResourceOffsetIsBound[kMaxRwResourceCount];
				bool samplerOffsetIsBound[kMaxSamplerCount];
				bool appendConsumeCounterIsBound;
//				bool streamOutOffsetIsBound[kMaxStreamOutBufferCount];		// No info available, stream-outs are in the VS but there's footer for the VS in a VsGs shader (only GS has a footer).
			};
#endif	// !defined(DOXYGEN_IGNORE)


			/** @brief Computes the maximum number of patches per thread-group for a LsShader/HsShader pair and returns a valid VGT and Patch count.
			 *	@param outVgtPrimitiveCount The VGT primitive count will be written here. Note that you must subtract 1 from this value before passing it to setVgtControl().
			 *	@param outTgPatchCount The number of computed patches per HS thread group will be written here. 
			 *	@param desiredTgLdsSizeInBytes A desired amount of HS-stage local data store to be allocated (max size is 32K). Desired LDS size will be used if higher than the minimum requirement of the HsShader.
			 *	@param desiredTgPatchCount A desired patch count to use. Desired patch count will be used if it is higher than the minimum requirement of the HsShader.
			 *	@param localShader The paired LsShader for the HsShader.
			 *	@param hullShader The HsShader to generate patch counts for.
			 */
			SCE_GNMX_EXPORT void computeTessellationTgParameters(uint32_t *outVgtPrimitiveCount, uint32_t* outTgPatchCount, uint32_t desiredTgLdsSizeInBytes, uint32_t desiredTgPatchCount, 
																 const Gnmx::LsShader* localShader, const Gnmx::HsShader* hullShader);

			
			/** @brief Fills the in/out "ShaderResourceOffsets" struct with data extracted from the GNMX shader struct (LS/HS/ES/VS/PS/CS Shader).
			 *	The shader code address and size are extracted from the GNMX shader. If shader code is in Garlic, the CPU performance will not be optimal, thus, 
			 *	we recommend calling this method before patching the shader code to be in GPU-friendly Garlic memory.
			 *	@param outTable The ShaderResourceOffsets table that will be written to. Note: outTable value must not be NULL.
			 *	@param shaderStage The shader stage that the ShaderResourceOffsets table is being generated for.
			 *	@param gnmxShaderStruct Gnmx shader class to be parsed, Note: shaderStage and gnmxShaderStruct shader type must match.
			 *  @note To maintain high performance of the LCUE, it is only necessary to build the SROTable once and can be cached alongside  the shader.
			 */
			SCE_GNMX_EXPORT void generateInputResourceOffsetTable(InputResourceOffsets* outTable, sce::Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct);

			
			/** @brief Fills the in/out "ShaderResourceOffsets" struct with data extracted from the GNMX shader struct (LS/HS/ES/VS/PS/CS Shader).
			 *  If the shader code is in Garlic, the CPU performance will not be optimal, thus we recommend calling this method before patching the shader code to be in GPU-friendly Garlic memory.
			 *  @param outTable The ShaderResourceOffsets table that will be written to. Note: outTable value must not be NULL.
			 *  @param shaderStage The shader stage that the ShaderResourceOffsets table is being generated for.
			 *  @param gnmxShaderStruct	Gnmx shader class to be parsed, Note: shaderStage and gnmxShaderStruct shader type must match.
			 *  @param shaderCode Pointer to the beginning of the shader microcode.
			 *  @param shaderCodeSizeInBytes Size of the shader microcode in bytes.
			 *  @param isSrtUsed If the shader uses SRTs, set to true, see Gnmx::ShaderCommonData.
			 *  @note To maintain high performance of the LCUE, it is only necessary to build the SROTable once and can be cached alongside  the shader.
			 */
			SCE_GNMX_EXPORT void generateInputResourceOffsetTable(InputResourceOffsets* outTable, sce::Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct, const void* shaderCode, int32_t shaderCodeSizeInBytes, bool isSrtUsed);

			
			/** @brief Fills the in/out "ShaderResourceOffsets" struct with data copied from another table, and applied the input semantic remap table.
			 *  @param outTable The patched ShaderResourceOffsets table will be written here. Note: outTable value must not be NULL.
			 *  @param inTable The original The ShaderResourceOffsets table that needs to be patched.
			 *  @param semanticRemapTable Table specifying how semantics should be remapped for the fetch shader.
			 *  @param semanticRemapTableSizeInItems Number of items in the semantic remap table.
			 *  @return Returns the difference between the number of VBs modified by semantic remap table. Note: the number of VB's needs to remain the same,
			 *  if non-zero value is returned, you incorrectly increased the number of input-VBs for the fetch shader.
			 *  @note To maintain high performance of the LCUE, it is only necessary to patch an SROTable if a fetch shader requires its semantics be remapped. SROTable can be cached once modified.
			 */
			SCE_GNMX_EXPORT int32_t patchInputResourceOffsetTableWithSemanticTable(InputResourceOffsets* outTable, const InputResourceOffsets* inTable, const int32_t* semanticRemapTable, int32_t semanticRemapTableSizeInItems);

		
			/** @brief Base class for Lightweight ConstantUpdateEngine */
			class SCE_GNMX_EXPORT BaseConstantUpdateEngine
			{
			public:

				
				/** @brief Sets the address to the global resource table -- a collection of V#'s which point to global buffers for various shader tasks
				 *  @param globalResourceTableAddr A pointer to the global resource table in memory.
				 *  @note The address specified in globalResourceTableAddr needs to be in GPU-visible memory and must be set before calling setGlobalInternalResource.
				 */
				SCE_GNM_FORCE_INLINE void setGlobalInternalResourceTable(void* globalResourceTableAddr) { m_globalInternalResourceTableAddr = (Gnm::Buffer*)globalResourceTableAddr; }

				
				/** @brief Sets an entry in the global resource table.
				 *  @param resourceType The global resource type to bind a buffer for. Each global resource type has its own entry in the global resource table.
				 *  @param buffer The buffer to bind to the specified entry in the global resource table. The size of the buffer is global-resource-type-specific.
				 *  @note This function modifies the global resource table. It is not safe to modify the global resource table unless the GPU is idle.
				 */
				void setGlobalInternalResource(sce::Gnm::ShaderGlobalResourceType resourceType, const sce::Gnm::Buffer* buffer);

#if !defined(DOXYGEN_IGNORE)

				void init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, void* globalInternalResourceTableAddr);

				void swapBuffers();

				int32_t m_bufferIndex;								// Index of the write buffer being used (N-buffer scheme)
				int32_t m_bufferCount;								// Number of resource buffers in the N-buffer scheme

				uint32_t* m_bufferBegin[kMaxResourceBufferCount];	// Beginning of the buffer used to store shader resource data
				uint32_t* m_bufferEnd[kMaxResourceBufferCount];		// End of the buffer
				uint32_t* m_bufferCurrent;							// Current pointer inside the buffer, start of new allocations

				sce::Gnm::Buffer* m_globalInternalResourceTableAddr;
#endif // !defined(DOXYGEN_IGNORE)
			};

		} // LightweightConstantUpdateEngine
	} // Gnmx
} // sce

#endif // _SCE_GNMX_LWBASECONSTANTUPDATEENGINE_H