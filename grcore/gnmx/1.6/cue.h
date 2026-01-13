/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.071
* Copyright (C) 2014 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_CUE2_H)
#define _SCE_GNMX_CUE2_H

#include "grcore/gnmx/common.h"
#ifdef CUE_V2

#include <gnm/common.h> // must be first, to define uint32_t and friends!
#include <gnm/constants.h>
#include <gnm/tessellation.h>
#include <gnm/streamout.h>
#include "grcore/gnmx/shaderbinary.h"

// If nonzero, binding a NULL pointer as a shader resource will be ignored.
// This matches the default behavior of the original ConstantUpdateEngine, and may be required for compatibility.
// If zero, binding a NULL pointer to a resource tells the CUE it can stop tracking the resource slot,
// which can significantly improve performance. It is highly recommended that this value remains undefined, and that unused resources be unbound (set to NULL).
//
#ifndef SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL
#define SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL	0
#endif //!SCE_GNM_CUE2_IGNORE_SHADER_RESOURCES_SET_TO_NULL


// If nonzero, all shader resource definitions are initialized to zero when the CUE is created.
// This matches the default behavior of the original ConstantUpdateEngine, and may be required for compatibility.
// If zero (default), the initial contents of shader resource definitions are undefined; attempts to use an unbound resource will trigger an assert.
// It is highly recommended that this value remainds undefined, and that all shader resources are properly initialized and bound prior to their first use.
//
#ifndef SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES
#define SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES	0
#endif //!SCE_GNM_CUE2_SUPPORT_UNINITIALIZE_SHADER_RESOURCES

// If nonzero, the contents of a shader's input usage table will be fully parsed to compute the required size of its Extended User Data (EUD) table.
// This may be necessary when using older versions of PSSLC, which do not guarantee a sensible ordering of elements within the input usage table.
// If zero (default), the CUE assumes that the shader's input usage table is sorted by slot index, and can quickly compute the EUD table size by inspecting the final
// table element.
// It is highly recommended that this value remain undefined, unless an assert triggers that tells you otherwise.
//
#ifndef SCE_GNM_CUE2_PARSE_INPUTS_TO_COMPUTE_EUD_SIZE
#define SCE_GNM_CUE2_PARSE_INPUTS_TO_COMPUTE_EUD_SIZE		0
#endif //!SCE_GNM_CUE2_PARSE_INPUTS_TO_COMPUTE_EUD_SIZE


namespace sce
{
	// Forward Declarations
	namespace Gnm
	{
		class Buffer;
		class Sampler;
		class Texture;
		class ConstantCommandBuffer;
		class DrawCommandBuffer;
		class DispatchCommandBuffer;
		struct InputUsageData;
	}

	namespace Gnmx
	{
		class CsShader;
		class EsShader;
		class GsShader;
		class HsShader;
		class LsShader;
		class PsShader;
		class VsShader;
		class CsVsShader;

		//

		class SCE_GNMX_EXPORT ConstantUpdateEngine
		{
		public:

			static uint32_t computeHeapSize(uint32_t numRingEntries);
			static uint32_t computeCpRamShadowSize();

		public:

			/** @brief Default constructor */
			ConstantUpdateEngine(void);
			/** @brief Default destructor */
			~ConstantUpdateEngine(void);

			/** @brief Associates a DrawCommandBuffer/ConstantCommandBuffer pair with this CUE. Any CUE functions
			 *         that need to write GPU commands will use these buffers.
			 *  @param dcb The DrawCommandBuffer where draw commands will be written.
			 *  @param ccb The ConstantCommandBuffer where shader resource definition management commands will be written.
			 */
			void bindCommandBuffers(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb)
			{
				m_dcb = dcb;
				m_ccb = ccb;
			}

			void init(void *heapAddr, uint32_t numRingEntries);

			/**
			 * @brief Sets the address of the system's global resource table -- a collection of <c>V#</c>s which point to global buffers for various shader tasks.
			 * @param addr The GPU-visible address of the global resource table. The minimum size of this buffer is given by
			 *             Gnm::SCE_GNM_SHADER_GLOBAL_TABLE_SIZE, and its minimum alignment is Gnm::kMinimumBufferAlignmentInBytes. This buffer is accessed by both the CPU and GPU.
			 */

			void setGlobalResourceTableAddr(void *addr);

			/**
			 * @brief Sets an entry in the global resource table.
			 * @note This function modifies the global resource table. It is not safe to modify the global resource table unless the GPU is idle.
			 * @param resType The global resource type to bind a buffer for. Each global resource type has its own entry in the global resource table.
			 * @param res The buffer to bind to the specified entry in the global resource table. The size of the buffer is global-resource-type-specific.
			 */
			void setGlobalDescriptor(Gnm::ShaderGlobalResourceType resType, const Gnm::Buffer *res);

			/**
			 * @brief Sets the active shader stages in the graphics pipeline.
			 *
			 * Note that the compute-only CS stage is always active.
			 * This function will roll the hardware context.
			 * @param activeStages Indicates which shader stages should be activated.
			 */
			void setActiveShaderStages(Gnm::ActiveShaderStages activeStages)
			{
				m_activeShaderStages = activeStages;
			}

			/** @brief Gets the last value set using setActiveShaderStages()
			 *
			 * @return The shader stages that will be considered activated by preDraw calls.
			 */

			Gnm::ActiveShaderStages getActiveShaderStages(void) const
			{
				return m_activeShaderStages;
			}


			///////////////////////
			// Functions to bind shader resources
			///////////////////////

			/**
			 * @brief Binds one or more read-only texture objects to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountResource-1].
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param textures The Texture objects to bind to the specified slots. textures[0] will be bound to <c><i>startApiSlot</i></c>, textures[1] to <c><i>startApiSlot</i> +1</c>, and so on.
			 *                 The contents of these Texture objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                 be unbound.
			 * @note Buffers and Textures share the same pool of API slots.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setTextures(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Texture *textures);

			/**
			 * @brief Binds one or more read-only buffer objects to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountResource-1].
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param buffers The #Buffer objects to bind to the specified slots. <c>buffers[0]</c> will be bound to <c><i>startApiSlot</i></c>, buffers[1] to <c><i>startApiSlot</i> +1</c>, and so on.
			 *                The contents of these #Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                be unbound.
			 * @note Buffers and Textures share the same pool of API slots.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers);

			/**
			 * @brief Binds one or more read/write texture objects to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountRwResource-1].
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param rwTextures The Texture objects to bind to the specified slots. <c><i>rwTextures</i>[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c><i>rwTextures</i>[1]</c> to <c><i>startApiSlot</i> +1</c>, etc.
			 *                   The contents of these Texture objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                   be unbound.
			 * @note <c><i>rwBuffers</i></c> and <c><i>rwTextures</i></c> share the same pool of API slots.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setRwTextures(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Texture *rwTextures);

			/**
			 * @brief Binds one or more read/write buffer objects to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountRwResource-1].
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param rwBuffers The Buffer objects to bind to the specified slots. <c><i>rwBuffers</i>[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c><i>rwBuffers</i>[1]</c> to <c><i>startApiSlot</i> +1</c>, and so on.
			 *                  The contents of these Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                  be unbound.
			 * @note <c><i>rwBuffers</i></c> and <c><i>rwTextures</i></c> share the same pool of API slots.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setRwBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *rwBuffers);

			/**
			 * @brief Binds one or more sampler objects to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountSampler-1].
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param samplers The Sampler objects to bind to the specified slots. samplers[0] will be bound to <c><i>startApiSlot</i></c>, samplers[1] to <c><i>startApiSlot</i>+1</c>, and so on.
			 *                 The contents of these Sampler objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                 be unbound.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setSamplers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Sampler *samplers);

			/**
			 * @brief Binds one or more constant buffer objects to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountConstantBuffer-1].
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param buffers The constant buffer objects to bind to the specified slots. buffers[0] will be bound to <c><i>startApiSlot</i></c>, buffers[1] to <c><i>startApiSlot</i> +1</c>, and so on.
			 *                The contents of these Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                be unbound.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setConstantBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers);

			/**
			 * @brief Binds one or more vertex buffer objects to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountVertexBuffer-1].
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param buffers The vertex buffer objects to bind to the specified slots. buffers[0] will be bound to startApiSlot, buffers[1] to <c><i>startApiSlot</i>+1</c>, and so on.
			 *                The contents of these Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                be unbound.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setVertexBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers);

			/**
			 * @brief Binds one or more Boolean constants to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountBoolConstant-1]</c>.
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param bits The Boolean constants to bind to the specified slots. Each slot will contain 32 1-bit Boolean values packed together in a singled dword.
			 *             <c><i>bits</i>[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c><i>bits</i>[1]</c> to <c><i>startApiSlot</i> + 1</c>, and so on.
			 *             The contents of the bits array are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *             be unbound.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setBoolConstants(Gnm::ShaderStage stage, uint32_t maskAnd, uint32_t maskOr);

			/**
			 * @brief Binds one or more floating-point constants to the specified shader stage.
			 * @param stage The resource(s) will be bound to this shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountResource-1]</c>.
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param floats The constants to bind to the specified slots. <c><i>floats</i>[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c><i>floats</i>[1]</c> to <c><i>startApiSlot</i>+1</c>, and so on.
			 *               The contents of these Texture objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *               be unbound.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called
			 */
			void setFloatConstants(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const float *floats);

			/**
			 * @brief Specifies a range of the Global Data Store to be used by shaders for atomic global counters such as those
			 *        used to implement PSSL <c>AppendRegularBuffer</c> and <c>ConsumeRegularBuffer</c> objects.
			 *
			 *  Each counter is a 32-bit integer. The counters for each shader stage may have a different offset in GDS. For example:
			 *  @code
			 *     setAppendConsumeCounterRange(kShaderStageVs, 0x0100, 12) // Set up 3 counters for the VS stage starting at offset 0x100.
			 *     setAppendConsumeCounterRange(kShaderStageCs, 0x0400, 4)  // Set up 1 counter for the CS stage at offset 0x400.
			 *	@endcode
			 *
			 *  The index is defined by the chosen slot in the PSSL shader. For example:
			 *  @code
			 *     AppendRegularBuffer<uint> appendBuf : register(u3) // Will access the 4th counter starting at the base offset provided to this function.
			 *  @endcode
			 *
			 *  This function never rolls the hardware context.
			 *
			 * @param stage The shader stage to bind this counter range to.
			 * @param rangeGdsOffsetInBytes The byte offset to the start of the counters in GDS. Must be a multiple of 4.
			 * @param rangeSizeInBytes The size of the counter range in bytes. Must be a multiple of 4.
			 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
			 */
			void setAppendConsumeCounterRange(Gnm::ShaderStage stage, uint32_t rangeGdsOffsetInBytes, uint32_t rangeSizeInBytes);

            /**
			 * @brief Binds one or more streamout buffer objects to the specified shader stage.
			 * @param startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountStreamoutBuffer-1].
			 * @param numApiSlots The number of consecutive API slots to bind.
			 * @param buffers The streamout buffer objects to bind to the specified slots. buffers[0] will be bound to <c><i>startSlot</i></c>, buffers[1] to <c><i>startSlot</i> + 1</c>, and so on.
			 *                The contents of these Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                be unbound.
			 */
			void setStreamoutBuffers(uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers);

			//////////////////////
			// Functions to bind shaders
			//////////////////////

			/**
			 * @brief Binds a shader to the VS stage.
			 * @param vsb Pointer to the shader to bind to the VS stage.
			 * @param shaderModifier Shader Modifier value generated by generateVsFetchShaderBuildState(), use 0 if no fetch shader.
			 * @param fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>vsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setVsShader(const Gnmx::VsShader *vsb, uint32_t shaderModifier, void *fetchShaderAddr);


			/**
			 * @brief Binds a shader pair to the asynchronous compute and VS stages for use in a dispatchDraw call.
			 * @param csvsb Pointer to the shaders to bind to the asynchronous compute and VS stage.
			 * @param shaderModifierVs If the VS shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param fetchShaderAddrVs If the VS shader requires a fetch shader, pass its GPU address here; otherwise pass 0.
			 * @param shaderModifierCs If the CS shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param fetchShaderAddrCs If the CS shader requires a fetch shader, pass its GPU address here; otherwise, pass 0.
			 * @note  Setting the asynchronous compute shader does not affect the currently set graphics CS stage shader, which is set by setCsShader().
			 *		  Asynchronous compute shaders share the user data constants and CUE ring elements set up for Gnm::kShaderStageVs,
			 *		  while graphics CS stage shaders use the user data constants and CUE ring elements set up for Gnm::kShaderStageCs.
			 * @note  Only the pointers csvsb->getVertexShader() and csvsb->getComputeShader() are cached inside the Gnm::ConstantUpdateEngine; the location
			 *		  and contents of these pointers should not be changed before all calls to dispatchDraw() which use this shader have been made!
			 * @note This binding will not take effect on the GPU until preDispatchDraw() or preDraw() is called.
			 */
			void setCsVsShaders(const Gnmx::CsVsShader *csvsb, uint32_t shaderModifierVs, void *fetchShaderAddrVs, uint32_t shaderModifierCs, void *fetchShaderAddrCs);

			/**
			 * @brief Binds a shader to the asynchronous compute stage, for use in a dispatchDraw() call.
			 * Generally, compute shaders for dispatch draw are packed into a CsVsShader and so will instead be set by calling setCsVsShaders.
			 * This function never rolls the hardware context.
			 * @param csb Pointer to the shader to bind to the asynchronous compute stage.
			 * @param shaderModifierCs If the compute shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param fetchShaderAddrCs If the compute shader requires a fetch shader, pass its GPU address here; otherwise pass 0.
			 * @note  Setting the asynchronous compute shader does not affect the currently set graphics CS stage shader, which is set by setCsShader().
			 *		  Asynchronous compute shaders share the user data constants and CUE ring elements set up for Gnm::kShaderStageVs,
			 *		  while graphics CS stage shaders use the user data constants and CUE ring elements set up for Gnm::kShaderStageCs.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c><i>*csb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDispatchDraw() is called.
			 */
			void setAsynchronousComputeShader(const Gnmx::CsShader *csb, uint32_t shaderModifierCs, void *fetchShaderAddrCs);

			/**
			 * @brief Binds a shader to the PS stage.
			 * @param psb Pointer to the shader to bind to the PS stage.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>psb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setPsShader(const Gnmx::PsShader *psb);

			/**
			 * @brief Binds a shader to the CS stage.
			 * @param csb Pointer to the shader to bind to the CS stage.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>csb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setCsShader(const Gnmx::CsShader *csb);

			/**
			 * @brief Binds a shader to the LS stage.
			 * @param lsb Pointer to the shader to bind to the LS stage.
			 * @param shaderModifier Shader Modifier value generated by generateLsFetchShaderBuildState(), use 0 if no fetch shader.
			 * @param fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>lsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setLsShader(const Gnmx::LsShader *lsb, uint32_t shaderModifier, void *fetchShaderAddr);

			/**
			 * @brief Binds a shader to the HS stage.
			 * @param hsb Pointer to the shader to bind to the HS stage.
			 * @param tessRegs Tessellation register settings for this HS shader. The register contents will be cached inside the Constant Update Engine.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>hsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setHsShader(const Gnmx::HsShader *hsb, const Gnm::TessellationRegisters *tessRegs);

			/**
			 * @brief Binds a shader to the ES stage.
			 * @param esb Pointer to the shader to bind to the ES stage.
			 * @param shaderModifier Shader Modifier value generated by generateEsFetchShaderBuildState(), use 0 if no fetch shader.
			 * @param fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>esb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setEsShader(const Gnmx::EsShader *esb, uint32_t shaderModifier, void *fetchShaderAddr);

			/**
			 * @brief Binds a shader to the GS and VS stages.
			 * @param gsb Pointer to the shader to bind to the GS/VS stages.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>gsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setGsVsShaders(const GsShader *gsb);

			/**
			 * @brief Sets the PS Input Table
			 * @param[in] psInput PS Input table.
			 * @note This function should be called after calls to setPsShader() and setVsShader().
			 *       Calls to setPsShader() or setVsShader() will invalidate this table.
			 *       If not specified, an appropriate table will be auto-generated in preDraw().
			 */
			void setPsInputUsageTable(uint32_t *psInput)
			{
				m_psInputs = psInput;
			}

			/**
			 * @brief Sets the vertex and instance offset for the current shader configuration.
			 * Vertex and instance offsets work only when enabled in the vertex shader.
			 * @param vertexOffset Offset added to each vertex index.
			 * @param instanceOffset Offset added to instance index.
			 */
			void setVertexAndInstanceOffset(uint32_t vertexOffset, uint32_t instanceOffset);

			 /**
			  * @brief Check if the current shader configuration is expecting vertex or instance offset.
			  */
			bool isVertexOrInstanceOffsetEnabled() const;

			////////////////////////////////
			// Draw/Dispatch related functions
			////////////////////////////////
			void preDraw();
			void postDraw();

			void preDispatch();
			void postDispatch();

			//

			/**
			 * @brief Call this function at the beginning of every frame.
			 */
			void advanceFrame(void);

			uint32_t getHeapSize(void) const
			{
				return m_heapSize;
			}

		public:

			//--------------------------------------------------------------------------------
			// Adjustable Constants:
			//--------------------------------------------------------------------------------

			static const uint32_t		kResourceChunkSize		 =  8;
			static const uint32_t		kResourceNumChunks		 = 16;
			static const uint32_t		kSamplerChunkSize		 =  8;
			static const uint32_t		kSamplerNumChunks		 =  2;
			static const uint32_t		kConstantBufferChunkSize =  8;
			static const uint32_t		kConstantBufferNumChunks =  3;
			static const uint32_t		kVertexBufferChunkSize	 =  8;
			static const uint32_t		kVertexBufferNumChunks	 =  4;
			static const uint32_t		kRwResourceChunkSize	 =  8;
			static const uint32_t		kRwResourceNumChunks	 =  2;

#ifndef DOXYGEN_IGNORE
			//--------------------------------------------------------------------------------
			// Internal Constants:
			//--------------------------------------------------------------------------------

			// Ring buffer indices for each chunked resource type.
			enum
			{
				kRingBuffersIndexResource,
				kRingBuffersIndexRwResource,
				kRingBuffersIndexSampler,
				kRingBuffersIndexVertexBuffer,
				kRingBuffersIndexConstantBuffer,
				kRingBuffersIndexStreamoutBuffer,
				kRingBuffersIndexDispatchDrawData,

				kNumRingBuffersPerStage
			};


			//--------------------------------------------------------------------------------
			// Internal Objects:
			//--------------------------------------------------------------------------------
			typedef struct StageChunkState
			{
				uint16_t  curSlots;    // Bitfield representing the resource which have been set for the upcoming draw.
				uint16_t  usedSlots;   // Bitfield representing the resource set for the previous draw.
				uint32_t  reserved;

				__int128_t *curChunk;  // Pointer to the current constructed chunk of resource for the next draw.
				__int128_t *usedChunk; // Pointer to the previously use chunk of resource from the last draw.
			} StageChunkState;

			// Different states for StageChunkState:
			// - Init: nothing allocated:
			//   . m_newResources  = 0
			//   . m_usedResources = 0
			//   . curChunk      = 0
			//   . m_prevChunk     = 0
			// - Just after draw:
			//   . m_newResources  = resource "used" by a previous draw
			//   . m_usedResources = 0
			//   . curChunk      = current resource set
			//   . m_prevChunk     = 0
			// - Between draws (no conflict):
			//   . m_newResources  = resource "used" by a previous draws and incoming draw
			//   . m_usedResources = 0
			//   . curChunk      = current resource set
			//   . m_prevChunk     = 0
			// - Between draws (w/ conflicts):
			//   . m_newResources  = resource "used" for incoming draw
			//   . m_usedResources = resource "used" by a previous draws and incoming draw
			//   . curChunk      = new set of resource
			//   . m_prevChunk     = previous resource set

			struct ShaderResourceRingBuffer
			{
				uint64_t headElemIndex;
				void    *elementsAddr;   // GPU address of the array of elements.
				uint32_t elemSizeDwords; // Size of each element, in dwords.
				uint32_t elemCount;	   // Capacity of the buffer, in elements.
				uint64_t wrappedIndex;
				uint64_t halfPointIndex;
			};


			typedef struct StageInfo
			{
				StageChunkState resourceStage[kResourceNumChunks];
				StageChunkState samplerStage[kSamplerNumChunks];
				StageChunkState constantBufferStage[kConstantBufferNumChunks];
				StageChunkState vertexBufferStage[kVertexBufferNumChunks];
				StageChunkState rwResourceStage[kRwResourceNumChunks];

				ShaderResourceRingBuffer ringBuffers[kNumRingBuffersPerStage];

				// Bitfield tracking which the resources have been set and
				// Bitfields tracking which resources have been set within the EUD:
				__int128_t activeResource;
				__int128_t eudResourceSet;

				uint16_t   activeSampler;
				uint16_t   eudSamplerSet;

				uint32_t   activeConstantBuffer;
				uint32_t   eudConstantBufferSet;

				uint16_t   activeRwResource;
				uint16_t   eudRwResourceSet;

				uint32_t   activeVertexBuffer;

				//

				uint32_t   boolValue;

				void      *internalSrtBuffer;
				uint32_t   userSrtBuffer[8];
				uint32_t   userSrtBufferSizeInDwords;

				const Gnm::InputUsageSlot	*inputUsageTable;
				uint32_t				 	*eudAddr;
				uint32_t				 	 shaderBaseAddr256;
				uint32_t				 	 shaderCodeSizeInBytes;
				uint16_t					 inputUsageTableSize;
				uint16_t                     eudSizeInDWord; // 0 means: no EUD.
				uint32_t                     dirtyRing; // Bitfield of input to upload to the ring.
				uint32_t                     appendConsumeDword;
			} StageInfo;

			////////////////////////////////
			// Compatibility API added. These functions are either no-ops, or wrappers around
			// other CUE functions.
			////////////////////////////////
			void preDraw(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb);
			void postDraw(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb);
			void preDispatch(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb);
			void postDispatch(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb);
			void setVertexAndInstanceOffset(Gnm::DrawCommandBuffer *dcb, uint32_t vertexOffset, uint32_t instanceOffset);


			void init(void *cpramShadowBuffer, void *heapAddr, uint32_t numRingEntries);

			void setActiveResourceSlotCount(Gnm::ShaderStage stage, uint32_t count);
			void setActiveRwResourceSlotCount(Gnm::ShaderStage stage, uint32_t count);
			void setActiveSamplerSlotCount(Gnm::ShaderStage stage, uint32_t count);
			void setActiveVertexBufferSlotCount(Gnm::ShaderStage stage, uint32_t count);
			void setActiveConstantBufferSlotCount(Gnm::ShaderStage stage, uint32_t count);
			void setActiveStreamoutBufferSlotCount(uint32_t count);

			void setBoolConstants(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const uint32_t *bits)
			{
				if ( numApiSlots == 0 )
					return;
				const uint32_t andMask = m_stageInfo[stage].boolValue;
				uint32_t orMask  = 0;
				for(uint32_t iSlot=0; iSlot<numApiSlots; ++iSlot)
					orMask |= (bits[iSlot] ? 1 : 0) << iSlot;
				orMask = orMask << startApiSlot;
				setBoolConstants(stage, andMask, orMask);
			}

			// Placeholder stuff to be implemented later
			void setDwordMaskedInDispatchDrawData(uint8_t dispatchDrawDword, uint32_t data, uint32_t mask);
			void setDwordInDispatchDrawData(uint8_t dispatchDrawDword, uint32_t data);
			void setTsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Gnm::Texture *tex);
			void setVsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Gnm::Buffer *buffer);
			void setSsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Gnm::Sampler *sampler);
			void setFloatInDispatchDrawData(uint8_t dispatchDrawDword, float fData)
			{
				union
				{
					float asFloat;
					uint32_t asU32;
				} value;
				value.asFloat = fData;
				setDwordInDispatchDrawData(dispatchDrawDword, value.asU32);
			}
			void preDispatchDraw(Gnm::DrawCommandBuffer *dcb, Gnm::DispatchCommandBuffer *acb, Gnm::ConstantCommandBuffer *ccb,
				Gnm::DispatchOrderedAppendMode *pOut_orderedAppendMode, uint16_t *pOut_dispatchDrawIndexDeallocationMask, uint32_t *pOut_sgprKrbLoc, Gnm::DispatchDrawMode *pOut_dispatchDrawMode, uint32_t *pOut_sgprVrbLoc);
			//~			void preDispatchDraw(Gnm::DrawCommandBuffer *dcb, Gnm::DispatchCommandBuffer *acb, Gnm::ConstantCommandBuffer *ccb,
			//~								 Gnm::DispatchOrderedAppendMode *pOutOrderedAppendMode, uint16_t *pOutDispatchDrawIndexDeallocationMask, uint32_t *pOutSgprKrbLoc, uint32_t *pOutSgprInstanceCs,
			//~								 Gnm::DispatchDrawMode *pOutDispatchDrawMode, uint32_t *pOutSgprVrbLoc, uint32_t *pOutSgprInstanceVs);
			void postDispatchDraw(Gnm::DrawCommandBuffer *dcb, Gnm::DispatchCommandBuffer *acb, Gnm::ConstantCommandBuffer *ccb);
			void setInternalSrtBuffer(Gnm::ShaderStage stage, void *buf);
			void setUserSrtBuffer(Gnm::ShaderStage stage, const void *buf, uint32_t bufSizeInDwords);

			//--------------------------------------------------------------------------------
			// Internal API:
			//--------------------------------------------------------------------------------
			void updateChunkState128(StageChunkState *chunkState, uint32_t numChunks);
			void updateChunkState256(StageChunkState *chunkState, uint32_t numChunks);
			void applyInputUsageData(Gnm::ShaderStage currentStage);

			//--------------------------------------------------------------------------------
			// Internal Variables:
			//--------------------------------------------------------------------------------
			uint32_t m_heapSize;
			uint32_t m_numRingEntries;

			Gnm::DrawCommandBuffer     *m_dcb;
			Gnm::ConstantCommandBuffer *m_ccb;

			Gnm::ActiveShaderStages m_activeShaderStages;

			StageInfo m_stageInfo[Gnm::kShaderStageCount];
			StageChunkState m_streamoutBufferStage;


			//////////////////////
			// Cached shader state
			//////////////////////
			const Gnmx::VsShader * m_currentVSB; // TODO: must currently be cached for compatibility with old CUE's drawIndirect()
			const Gnmx::PsShader * m_currentPSB; // TODO: must currently be cached to allow the PS inputs to be calculated before every draw, once we know what the VSB is.
			const Gnmx::LsShader * m_currentLSB; // TODO: must currently be cached for compatibility with old CUE's drawIndirect()
			const Gnmx::EsShader * m_currentESB; // TODO: must currently be cached for compatibility with old CUE's drawIndirect()
			const uint32_t *m_psInputs;
			bool m_dirtyVsOrPs;

			bool m_anyWrapped;
			bool m_usingCcb;
			bool m_eudReferencesStreamoutBuffers;

			// Bitfields indexed by shader stage
			uint8_t m_shaderUsesSrt;	 // One bit per shader stage. If a stage's bit is set, the bound shader uses Shader Resource Tables.
			uint8_t m_shaderDirtyEud;	 // One bit per shader stage. If a stage's bit is set, its EUD needs to be allocated and setup.

			void     *m_globalTableAddr;
			uint32_t m_globalTablePtr[SCE_GNM_SHADER_GLOBAL_TABLE_SIZE / sizeof(uint32_t)];
			bool     m_globalTableNeedsUpdate;
#endif // DOXYGEN_IGNORE
		};
	}
}
#endif // CUE_V2

#endif
