/**
 * Lightweight Constant Update Engine (LCUE)
 * Version 0.9.5 - Apr/26/2013
 *
 * Provides a lightweight implementation of the Constant Update Engine (CUE), which is more limited but expected
 * to perform better on the CPU side and be easier to understand and extend.
 *
 *
 * GLOSSARY/DEFINITIONS:
 * -------------------------------------------------------------------------------------------------------------------------
 * - User Data (UD): 16 SGPRs s[0:15] in a shader core, which are pre-fetched by the SPI before execution starts.
 *   - UD is set through PM4 packets, thus, you don't need to maintain copies of those resource descriptors in memory.
 * - User Extended Data (UED): up to 48 DWORDs (SW limitation been lifted soon), four available tables EUD1-4.
 *   - Don't have a specific type (e.g. V#/T#/S# are supported), and is stored/kept in memory.
 *   If the UED is used, the UD will store a pointer to the start of each UED.
 * - Flat-table (or array): up to [N] DWORDs (SW limitation) for a specific resource type, five tables available.
 *   - Can be seen as a contiguous zero-based array that may (or may not) have gaps, and is stored/kept in memory.
 *   - Flat-tables: resource (textures/buffers), rw-resource (textures/buffers), sampler, constant buffer and vertex buffer.
 * - Memory based resources: UED or flat-table based resources.
 *
 *
 * GENERAL IMPROVEMENTS:
 * -------------------------------------------------------------------------------------------------------------------------
 * The LCUE tries to improve over the CUE in the following aspects:
 * - Keep memory-based resources stored as tightly packed as possible, reducing memory storage and bandwidth.
 *   - It only reserves the amount of memory a shader needs, and this is the only memory that gets flushed.
 * - Avoid copying resources over multiple different locations in memory.
 *   - Data is set directly to a scratch buffer that gets flushed once to its final position.
 *   - An older version of the LCUE didn't have this single flush but it was even more restrictive.
 * - Allow the user to reduce the maximum number of resources, if desired.
 *   - In the LCUE, this will reduce the size of the "ShaderResourceOffsets" structure.
 *
 *
 * TESSELLATION IMPROVEMENTS:
 * -------------------------------------------------------------------------------------------------------------------------
 * Using tessellation can be error prone due to the necessity of configuring shader resource registers and the GPU's context 
 * registers, according to the combination of Local shader, Hull shader and the desired patch count per thread group. To
 * use tessellation the user has to configure: local shader LDS size, tessellation internal constant buffer which is used 
 * by Hull and Domain stages (goes on API-slot 19), and a few context registers. Again, those will change if Local shader, 
 * Hull shader or patch count changes.
 *
 * To solve this issue, LCUE provides a mode that automatically handles all the "internal" resources and configurations
 * required by Tessellation. It also validates the provided "desired patch count per thread group", and if it's not valid,
 * it will use the closest valid value. You can enable/disable this mode through the 
 * LCUE_TESSELLATION_AUTO_CONFIGURE_TG_PATCH_COUNT_ENABLED define.
 *
 * The LCUE also provides the "computeTessellationTgPatchCountAndLdsSize" method to calculate the thread group patch count,
 * and LDS size, if you prefer to configure the tessellation pipeline manually.
 *
 *
 * EXTENDED USER DATA IMPROVEMENTS:
 * -------------------------------------------------------------------------------------------------------------------------
 * The PSSL Compiler has a switch that allows the extended user data (EUD) size to be increased from it's original 48DW.
 * The EUD table has a high priority, meaning that the compiler backend will move most of the resources types to there, if 
 * there's enough size available. The good this about the EUD, is that all resources are tightly packed there. The LCUE
 * currently supports EUD tables with any size, allowing you to take advantage of this switch.
 *
 *
 * COMPUTECONTEXT AND GRAPHICSCONTEXT
 * -------------------------------------------------------------------------------------------------------------------------
 * 
 *
 * PROFILING AND VALIDATION:
 * -------------------------------------------------------------------------------------------------------------------------
 * - Profile the number of set V#/T#/S#/PTRs calls, draw/dispatch calls and total amount of memory-based resources flushed.
 * - Check and validate if all resources required by the last bound shader were set.
 * - Check and validate if resources that are not expected/required by the current shader are set.
 *
 *
 * LIMITATIONS:
 * -------------------------------------------------------------------------------------------------------------------------
 * - Supports: LS, HS, VS, PS and CS.	(There's no Geometry Shader support yet)
 * - All resources used/required by a shader need to be set after the shader itself is bound, every time. 
 *   - Thus, resources descriptors are not maintained/shared between shaders.
 * - A few resource descriptor types are not handled yet: stream-out (for GS), immediate float/bool and GDS types.
 * - Constant Command Buffer (CCB) or CPRAM is not used, this has advantages and disadvantages.
 *
 * 
 * MORE DETAILS:
 * -------------------------------------------------------------------------------------------------------------------------
 * The LCUE uses the "ShaderResourceOffsets" struct to accelerate resource binding, when shader resources are set by the 
 * user they're either: set in the UD using PM4 packets, or copied to a scratch buffer.
 *
 * The scratch buffer is where all memory-based resources set by the user are directly put, and its used memory area
 * gets flushed later to its final position in a double-buffer scheme, which is where shaders will ultimately read from.
 *
 * Currently, only the Draw Command Buffer (DCB) is being used. This is different from the original CUE that also uses the 
 * Constant Command Buffer (CCB). When the CCB is used, it copies memory chunks to a special memory called CPRAM, and then
 * dumps that memory back to RAM again. The advantage of doing this is that write back operations to memory go through the
 * shader core L2 cache, pre-fetching it. However, this also requires sync instructions between DCB and CCB, and more memory
 * slots in the ring buffer to allow the CCB to be N units in front of the DCB.
 *
 *
 * FUTURE PLANS:
 * -------------------------------------------------------------------------------------------------------------------------
 * - Cache VS-PS export/import semantic tables.
 * - Cache LS-HS context registers and internal tessellation constant buffer.
 * - Create a write buffer for descriptors set through PM4 packets, that should speedup code that allocates memory from Draw/DispatchCommandBuffer.
 *
 */
#pragma once

#include <gnm.h>
#include <gnmx.h>
#include <stdint.h>

/**
 * @brief If defined, profiles draw/dispatch count, V#/T#/S# resource set calls and the total memory copied to the resource buffer and touched by GPU shader cores.
 */
// #define LCUE_PROFILE_ENABLED

/**
 * @brief If defined, handles tessellation shader stages and resources. This is not defined by default to increase performance.
 */
//#define LCUE_TESSELLATION_ENABLED

/**
 * @brief If defined, all "internal" tessellation parameters and resources are automatically generated, validated and set.
 * The following steps are done if enabled:
 * 1. Calculates patch count per thread-group and make sure the one specified by the user is valid, and if not, uses the closest valid one.
 * 2. Updates Local shader LDS size (shared by all tessellation thread groups).
 * 3. Generates and sets the TessellationDataConstantBuffer (internal constant buffer at API-slot 19).
 * 4. Sets TessellationRegisters configuration (context register).
 * 5. Sets patches per-VGT (context register).
 */
#define LCUE_TESSELLATION_TG_PATCH_COUNT_AUTO_CONFIGURE_ENABLED

/**
 * @brief If defined, works around the PSSL compiler bug that defaults max-tessellation to 0 when it's not declared in the shader code.
 */
#define LCUE_TESSELLATION_PSSLC_BUG_WORKAROUND_ENABLED

/**
 * @brief If defined, the Immediate Constant Buffer (ICB), known as embedded constant buffer in Gnm, descriptor is automatically generated and bound.
 * An ICB is generated when global static arrays are accessed through dynamic variables, preventing the compiler from embedding its immediate values in the shader code.
 * The ICB is stored after the end of the shader code area, and is expected on API-slot 15.
 */
#define LCUE_IMMEDIATE_CB_AUTO_HANDLE_ENABLED

/** 
 * @brief If defined, asserts if a resource that is not used by the current shader is bound. Cannot be used with the LCUE_VALIDATE_RESOURCE_BINDING_AND_SKIP_ENABLED define.
 */
// #define LCUE_VALIDATE_RESOURCE_BINDING_AND_ASSERT_ENABLED

/** 
 * @brief If defined, skips the binding of resources that are not used by the current shader. Cannot be used with the LCUE_VALIDATE_RESOURCE_BINDING_AND_ASSERT_ENABLED define.
 * If an array of resources is bound, each item in the array is checked and only valid/expected API-slots are bound. This is not recommended for optimal performance.
 */
#define LCUE_VALIDATE_RESOURCE_BINDING_AND_SKIP_ENABLED

/** 
 * @brief If defined, check if all resources expected by the current bound shaders are bound when a "draw" or "dispatch" is issued. Also checks if a shader expects more input
 * resources than the LCUE was configure to handle when "generateSROTable" is called.
 */
// #define LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED


namespace sce
{

namespace LCUE
{
	// Defines how many resources of each available type would be stored and handled by the LCUE, this also defines the maximum valid API-slot.
	// Note: resource types are: textures/buffers, rw_textures/rw_buffers, samplers, vertex_buffers and constant_buffers.
	const uint32_t kMaxResourceCount = 16;
	const uint32_t kMaxRwResourceCount = 16;
	const uint32_t kMaxSamplerCount = 16;
	const uint32_t kMaxVertexBufferCount = 16;
	const uint32_t kMaxConstantBufferCount = 20;

	const uint32_t kResourceInUserDataSgpr = 0x8000;
	const uint32_t kResourceIsVSharp = 0x4000;
	const uint32_t kResourceValueMask = 0x3FFF;
	
	const uint32_t kGpuStageBufferSizeInDwords = (6 * 1024) / 4; // 6KB is enough to store anything you can bing to a GPU shader stage
	const uint32_t kComputeScratchBufferSizeInDwords = kGpuStageBufferSizeInDwords;
	const uint32_t kGraphicsScratchBufferSizeInDwords = sce::Gnm::kShaderStageCount * kGpuStageBufferSizeInDwords;
	
	const uint32_t kMaxResourceBufferCount = 4;
	const uint32_t kGpuStageAverageBufferSizeInDwords = 1024 / 4; // Estimated size of memory based resources for one GPU stage (can hold 16T + 16VB + 4RW_T + 4S + 4CB)
	const uint32_t kMinResourceBufferSizeInDwords = 1000 * kGpuStageAverageBufferSizeInDwords; // Require a minimum buffer size that can store at least 1000 GPU stages

	const int32_t kGlobalInternalTableSizeInDwords = sce::Gnm::kShaderGlobalResourceCount * sizeof(sce::Gnm::Buffer) / 4;
	
	// Internal constant buffers that are spected at specific API-slots
	const int32_t kConstantBufferInternalApiSlotForTessellation = 19; // Required if Tessellation is enabled
	const int32_t kConstantBufferInternalApiSlotForEmbeddedData = 15; // 

	const uint64_t kShaderBinaryInfoSignatureMask = 0x00ffffffffffffffLL;
	const uint64_t kShaderBinaryInfoSignatureU64 =  0x007264685362724fLL;

	/**
	 * @brief Accelerates the binding of shader resources (170B per item).
	 */
	struct ShaderResourceOffsets
	{
		// How much memory needs to be reserved to store all memory-based shader resources (things not set through PM4)
		uint16_t requiredBufferSizeInDwords;

		// Shader stage
		uint8_t shaderStage;

		// For each available shader-resource-ptr, store the starting SGPR s[0:63] where it should be set (0xFF means it's not used)
		// Note: pointers take 2 SGPRs and must be 2DW aligned
		uint8_t fetchShaderPtrSgpr; // Currently, does not need to be stored as it's always s[0:1] (if it exists)
		uint8_t userExtendedData1PtrSgpr;
		uint8_t resourcePtrSgpr;
		uint8_t rwResourcePtrSgpr;
		uint8_t samplerPtrSgpr;
		uint8_t constBufferPtrSgpr;
		uint8_t vertexBufferPtrSgpr; // Currently, does not need to be stored as it's always s[2:3] (if it exists)
		uint8_t globalInternalPtrSgpr;

		// For each available shader-resource-flat-table (or array), store the memory offset (from the start of the buffer) to the beginning of its flat-table (0xFF means it's not used)
		// Note: required because arrays are 0 indexed but may have gaps, allowing you to use any index inside the valid range. 
		//       Thus, this accelerates setting the pointer to the beginning of flat-tables.
		uint16_t resourceArrayOffset;
		uint16_t rwResourceArrayOffset;
		uint16_t samplerArrayOffset;
		uint16_t constBufferArrayOffset;
		uint16_t vertexBufferArrayOffset;

		// For each logical shader API slot, store either: an offset to a memory location, or a User Data (UD) SGPR where the resource should be set.
		// Note: if (item[i]&kResourceInUserDataSgpr) it's set directly into s[0:15] using PM4 packets, otherwise it's copied into the scratch buffer using the offset.
		uint16_t resourceOffset[kMaxResourceCount];
		uint16_t rwResourceOffset[kMaxRwResourceCount];
		uint16_t samplerOffset[kMaxSamplerCount];
		uint16_t vertexBufferOffset[kMaxVertexBufferCount];
		uint16_t constBufferOffset[kMaxConstantBufferCount];

		// TODO: Missing/not-handled resource types:
		// - Stream-out immediate and table ptr
		// - All indirect table ptrs:
		//   - kShaderInputUsageImmAluFloatConst		// Immediate float const (scalar or vector).
		//   - kShaderInputUsageImmAluBool32Const		// 32 immediate Booleans packed into one UINT.
		//   - kShaderInputUsageImmGdsCounterRange		// Immediate UINT with GDS address range for counters (used for append/consume buffers).
		//   - kShaderInputUsageImmGdsMemoryRange		// Immediate UINT with GDS address range for storage.
		//   - kShaderInputUsageImmGwsBase				// Immediate UINT with GWS resource base offset.
	};


	/**
	 * @brief TODO
	 */
	struct ShaderResourceBindingProfilingInfo
	{
		uint32_t shaderResourceTotalUploadSizeInBytes;
		uint32_t setVsharpCount;
		uint32_t setSsharpCount;
		uint32_t setTsharpCount;
		uint32_t setPtrCount;
		uint32_t drawCount;
		uint32_t dispatchCount;
	};


	/**
	 * @brief TODO
	 */
	struct ShaderResourceBindingValidation
	{
		bool resourceOffsetIsBound[kMaxResourceCount];
		bool rwResourceOffsetIsBound[kMaxRwResourceCount];
		bool samplerOffsetIsBound[kMaxSamplerCount];
		bool vertexBufferOffsetIsBound[kMaxVertexBufferCount];
		bool constBufferOffsetIsBound[kMaxConstantBufferCount];
	};


	/**
	 * @brief Computes the maximum number of patches per thread-group for a LsShader/HsShader pair and returns the valid patch count closest to "optionalDesiredTgPatchCount",
	 * and the required thread-group LDS size for that patch count.
	 */
	inline void computeTessellationTgPatchCountAndLdsSize(int32_t* outTgPatchCount, int32_t* outTgLdsRequiredSizeInBytes, const sce::Gnmx::LsShader* localShader, const sce::Gnmx::HsShader* hullShader,
		int32_t optionalDesiredTgPatchCount = 16);

	/**
	 * @brief Generates a "ShaderResourceOffsets" from a GNMX shader type (Ls/Hs/Vs/Ps/Cs Shader).
	 * The shader code pointer and size (required by this method) are get from the GNMX shader type. If the shader code is in Garlic, the CPU access performance will not be optimal.
	 */
	int32_t generateSROTable(ShaderResourceOffsets* outTable, sce::Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct);

	/**
	 * @brief Generates a "ShaderResourceOffsets" from a GNMX shader type (Ls/Hs/Vs/Ps/Cs Shader).
	 * If the shader code is in Garlic, the CPU access performance will not be optimal.
	 */
	int32_t generateSROTable(ShaderResourceOffsets* outTable, sce::Gnm::ShaderStage shaderStage, const void* gnmxShaderStruct, const void* shaderCode, int32_t shaderCodeSize);


	/**
	 * @brief TODO
	 */
	struct BaseCUE
	{
	public:
		void setGlobalInternalResource(sce::Gnm::ShaderGlobalResourceType resourceType, restrict const sce::Gnm::Buffer* buffer);

	protected:
		void init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, 
			uint32_t* globalInternalResourceTableInGarlic = NULL, int32_t globalInternalResourceTableSizeInDwords = 0);
		void swapBuffers();
		inline uint32_t* allocateSpace(sce::Gnm::CommandBuffer *cb,uint32_t dwordCount);

	private:
		int32_t m_bufferIndex;								// Index of the write buffer being used (N-buffer scheme)
		int32_t m_bufferCount;								// Number of resource buffers in the N-buffer scheme

		uint32_t* m_bufferBegin[kMaxResourceBufferCount];	// Beginning of the buffer used to store shader resource data
		uint32_t* m_bufferEnd[kMaxResourceBufferCount];		// End of the buffer
		uint32_t* m_bufferCurrent;							// Current pointer inside the buffer, start of new allocations

	protected:
		sce::Gnm::Buffer* m_globalInternalResourceTable;

#if defined LCUE_PROFILE_ENABLED
		mutable ShaderResourceBindingProfilingInfo m_profiling;
#endif
#if defined LCUE_PROFILE_ENABLED
	public:
		mutable uint32_t m_setVsharpCount, m_setSsharpCount, m_setTsharpCount, m_setPtrCount,
			m_psUsageCount, m_drawCount, m_dispatchCount, m_shaderResourceTotalUploadSizeInBytes;
#endif
	};

	
	/**
	 * @brief TODO
	 */
	struct ComputeCUE : public BaseCUE
	{
	private:
		inline uint32_t* flushScratchBuffer();
		inline void updateCommonPtrsInUserDataSgprs(const uint32_t* resourceBufferFlushedAddress);
		inline void updateImmediateCb(const sce::Gnmx::ShaderCommonData* shaderCommon);
		void preDispatch();

	public:
		void init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, 
			uint32_t* globalInternalResourceTableInGarlic = NULL, int32_t globalInternalResourceTableSizeInDwords = 0);
		void swapBuffers();
		inline void setDcb(sce::Gnm::DispatchCommandBuffer* dcb) { m_dcb = dcb; };

		void setCsShader(const sce::Gnmx::CsShader* shader, const ShaderResourceOffsets* table);

		void setConstantBuffers(int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Buffer* buffer);
		void setBuffers(int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Buffer* buffer);
		void setTextures(int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Texture* texture);
		void setRwBuffers(int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Buffer* buffer);
		void setRwTextures(int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Texture* texture);
		void setSamplers(int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Sampler* sampler);
		void setVertexBuffers(int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Buffer* vertexBuffer);

		inline void dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) {
			preDispatch();
			m_dcb->dispatch(threadGroupX, threadGroupY, threadGroupZ);
		}

		inline void dispatchIndirect(uint32_t dataOffsetInBytes) {
			preDispatch();
			m_dcb->dispatchIndirect(dataOffsetInBytes);
		}

	protected:
		uint32_t m_scratchBuffer[kComputeScratchBufferSizeInDwords];
		sce::Gnm::DispatchCommandBuffer* m_dcb;

		const void* m_boundShader;
		const ShaderResourceOffsets* m_boundShaderResourceOffsets;
		bool m_dirtyShader;
		bool m_dirtyShaderResources;

#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
		mutable ShaderResourceBindingValidation m_boundShaderResourcesValidation;
#endif
	};


	/**
	 * @brief TODO
	 */
	struct GraphicsCUE : public BaseCUE
	{
	private:
		inline uint32_t* flushScratchBuffer(sce::Gnm::ShaderStage shaderStage);
		inline void updateLsEsVsPtrsInUserDataSgprs(sce::Gnm::ShaderStage shaderStage, const uint32_t* resourceBufferFlushedAddress);
		inline void updateCommonPtrsInUserDataSgprs(sce::Gnm::ShaderStage shaderStage, const uint32_t* resourceBufferFlushedAddress);
		inline void updateImmediateCb(sce::Gnm::ShaderStage shaderStage, const sce::Gnmx::ShaderCommonData* shaderCommon);
		void preDraw();
		void preDispatch();

	public:
		void init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, 
			uint32_t* globalInternalResourceTableInGarlic = NULL, int32_t globalInternalResourceTableSizeInDwords = 0);
		void swapBuffers();
		void setDcb(sce::Gnm::DrawCommandBuffer* dcb) { m_dcb = dcb; }

#if defined LCUE_TESSELLATION_ENABLED
		void setLsShader(const sce::Gnmx::LsShader* shader, const void* fetchShader, const ShaderResourceOffsets* table);
		void setHsShader(const sce::Gnmx::HsShader* shader, const ShaderResourceOffsets* table, int32_t optionalDesiredTgPatchCount = 16);
#endif

		void setVsShader(const sce::Gnmx::VsShader* shader, uint32_t shaderModifier, const void* fetchShader, const ShaderResourceOffsets* table);
		void setVsShader(const sce::Gnmx::VsShader* shader, const ShaderResourceOffsets* table);
		void setVsFetchShader(uint32_t shaderModifier, const void *fetchShader) { m_boundShaderModifier[Gnm::kShaderStageVs] = shaderModifier; m_boundFetchShader[Gnm::kShaderStageVs] = fetchShader; }

		void setPsShader(const sce::Gnmx::PsShader* shader, const ShaderResourceOffsets* table);
		void setCsShader(const sce::Gnmx::CsShader* shader, const ShaderResourceOffsets* table);

		void setConstantBuffers(sce::Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Buffer* buffer);
		inline bool isConstantBufferUsed(sce::Gnm::ShaderStage shaderStage,int32_t startApiSlot) {
			return (m_boundShaderResourceOffsets[shaderStage] && m_boundShaderResourceOffsets[shaderStage]->constBufferOffset[startApiSlot] != 0xFFFF);
		}
		void setBuffers(sce::Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Buffer* buffer);
		void setTextures(sce::Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Texture* texture);
		void setRwBuffers(sce::Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Buffer* buffer);
		void setRwTextures(sce::Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Texture* texture);
		void setSamplers(sce::Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Sampler* sampler);
		void setVertexBuffers(sce::Gnm::ShaderStage shaderStage, int32_t startApiSlot, int32_t apiSlotCount, restrict const sce::Gnm::Buffer* vertexBuffer);

		inline void drawIndexAuto(uint32_t indexCount) {
			preDraw();
			m_dcb->drawIndexAuto(indexCount);
		}

		inline void drawIndexInline(uint32_t indexCount, const void *indices, uint32_t indicesSizeInBytes) {
			preDraw();
			m_dcb->drawIndexInline(indexCount, indices, indicesSizeInBytes);
		}

		inline void drawIndex(uint32_t indexCount, const void *indexAddr) {
			preDraw();
			m_dcb->drawIndex(indexCount, indexAddr);
		}

		inline void drawIndexOffset(uint32_t indexOffset, uint32_t indexCount) {
			preDraw();
			m_dcb->drawIndexOffset(indexOffset, indexCount);
		}

		inline void drawIndirect(uint32_t dataOffsetInBytes) {
			preDraw();
			m_dcb->drawIndirect(dataOffsetInBytes);
		}

		inline void drawIndexIndirect(uint32_t dataOffsetInBytes) {
			preDraw();
			m_dcb->drawIndexIndirect(dataOffsetInBytes);
		}

		inline void dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) {
			preDispatch();
			m_dcb->dispatch(threadGroupX, threadGroupY, threadGroupZ);
		}

		inline void dispatchIndirect(uint32_t dataOffsetInBytes) {
			preDispatch();
			m_dcb->dispatchIndirect(dataOffsetInBytes);
		}

	protected:
		uint32_t m_scratchBuffer[kGraphicsScratchBufferSizeInDwords];
		sce::Gnm::DrawCommandBuffer* m_dcb;

		// Each active shader stage has a pointer to inside the buffer where it should place its resources according to the offset table
		const void* m_boundShader[sce::Gnm::kShaderStageCount];
		const void* m_boundFetchShader[sce::Gnm::kShaderStageCount]; // LS, ES or VS
		uint32_t m_boundShaderModifier[sce::Gnm::kShaderStageCount]; // LS, ES or VS
		const ShaderResourceOffsets* m_boundShaderResourceOffsets[sce::Gnm::kShaderStageCount];
		bool m_dirtyShader[sce::Gnm::kShaderStageCount];
		bool m_dirtyShaderResources[sce::Gnm::kShaderStageCount];

		// Tessellation
		int32_t m_tessellationDesiredTgPatchCount;

#if defined LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
		mutable ShaderResourceBindingValidation m_boundShaderResourcesValidation[sce::Gnm::kShaderStageCount];
#endif
	};


	/**
	 * @brief TODO
	 */
	struct ComputeContext : public ComputeCUE
	{
		void init(uint32_t* dcbBuffer, int32_t dcbBufferSizeInDwords,
			uint32_t* resourceBufferInGarlic, int32_t resourceBufferSizeInDwords, 
			uint32_t* globalInternalResourceTableInGarlic = NULL, int32_t globalInternalResourceTableSizeInDwords = 0,
			sce::Gnm::CommandCallbackFunc callbackFunc = NULL, void *callbackUserData = NULL);

//
#include "lcue_computecontext_methods.h"

	public:
		sce::Gnm::DispatchCommandBuffer m_dcb;
	};


	/**
	 * @brief TODO
	 */
	struct GraphicsContext : public GraphicsCUE
	{
		void init(uint32_t* dcbBuffer, int32_t dcbBufferSizeInDwords,
			uint32_t* resourceBufferInGarlic, int32_t resourceBufferSizeInDwords, 
			uint32_t* globalInternalResourceTableInGarlic = NULL, int32_t globalInternalResourceTableSizeInDwords = 0,
			sce::Gnm::CommandCallbackFunc callbackFunc = NULL, void *callbackUserData = NULL);

// 
#include "lcue_gfxcontext_methods.h"

		inline void setupScreenViewport(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset)
		{
			return Gnmx::setupScreenViewport(&m_dcb, left, top, right, bottom, zScale, zOffset);
		}

	public:
		sce::Gnm::DrawCommandBuffer m_dcb;
	};

}
}