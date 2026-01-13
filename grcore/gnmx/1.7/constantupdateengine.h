/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_CONSTANTUPDATEENGINE_H)
#define _SCE_GNMX_CONSTANTUPDATEENGINE_H

#include "common.h" // must be first, to define uint32_t and friends!
#ifdef SCE_GNMX_ENABLE_CUE_V2
#include "cue.h"
#else
#include <gnm/constants.h>
#include <gnm/tessellation.h>
#include <gnm/streamout.h>
#include "shaderbinary.h"

namespace sce
{
	// Forward Declarations
	namespace Gnm
	{
		class Buffer;
		class Sampler;
		class Texture;
		class DrawCommandBuffer;
		class DispatchCommandBuffer;
		struct InputUsageData;
	}

    /** @brief Higher-level layer on top of the %Gnm library.
		In the %Gnmx library, higher level functions for driving the GPU are defined using the Gnm interface. Many of
		the Gnmx API calls are transferred directly to corresponding low-level Gnm calls. However, methods for
		generating command buffers and managing shader resources, etc. are implemented so that they can be called
		from a GfxContext class instance, which contains state variables. There are differences between the Gnmx layer
		and the Gnm layer in that, for example, an instance of a class in one Gnmx namespace is not guaranteed to be
		usable from another Gnmx namespace, and a method in one Gnmx instance cannot, in general, be called
		from another Gnmx namespace.

		The two main entry points to Gnmx functions are the GfxContext class and the ComputeContext class. To realize
		a comprehensible rendering API suitable for users who already have knowledge of OpenGL, Direct3D or PlayStation®3,
		the GfxContext class uses a command buffer pair together with a ConstantUpdateEngine instance, which manages shader
		resources across multiple draw calls. However, one should bear in mind that these implementations are offered
		primarily as a usable reference and that, as a whole, the %Gnmx API layer is still similar to the Gnm interface in
		the sense that results are not necessarily guaranteed. Problems might occur if API calls are re-ordered, and care
		must still be taken with: hardware resources such as caches, memory access ordering when programming.
		
		The %Gnmx library should be used as the starting point for initial development on PlayStation®4. Note that the
		Gnmx source code has been written to provide comprehensible working examples of how to call sequences of Gnm
		functions and is not necessarily optimized. In many cases, it will be necessary to modify the source code of
		the %Gnmx library for optimization purposes. An optimized library may be provided by SCE in the future together
		with source code.
     */
	namespace Gnmx
	{
		/** @brief The ConstantUpdateEngine (or CUE) manages the dynamic mapping between shader resource descriptions in memory and shader resource API slots.
		*/
		class SCE_GNMX_EXPORT ConstantUpdateEngine
		{
		public:
			/** @brief Default constructor */
			ConstantUpdateEngine(void);
			/** @brief Default destructor */
			~ConstantUpdateEngine();

			/**
			 * @brief Determines how large a buffer must be passed to init() function's <c><i>heap</i></c> parameter for a given engine type and ring buffer size.
			 * @param[in] numRingEntries Specifies how large each resource chunk's ring buffer should be. The larger this value, the
			 *                       more space the Constant Update Engine has to minimizing DMA stalls by preparing data for upcoming draw/dispatch calls
			 *                       in parallel. 16 entries is a reasonable default value for now. Must be the same value passed to init().
			 * @return The minimum size of the required buffer, in bytes.
			 * @see ConstantUpdateEngine::init()
			 */
			static uint32_t computeHeapSize(uint32_t numRingEntries);

			/**
			 * @brief Determines how large a buffer must be passed to init() function's <c><i>cpramShadowBuffer</i></c> parameter for a given engine type.
			 * @return The minimum size of the required buffer, in bytes.
			 * @see ConstantUpdateEngine::init()
			 */
			static uint32_t computeCpRamShadowSize();

			/**
			 * @brief Performs one-time initialization of a Constant Update Engine object.
			 * @param[out] cpramShadowBuffer Buffer used internally by the Constant Update Engine to mirror the contents of CPRAM. This buffer is read and written by the CPU only.
			 *                          Use computeCpRamShadowSize() to determine its minimum size. Its minimum alignment is <c>Gnm::kMinimumBufferAlignmentInBytes</c>. This pointer must not be NULL.
			 * @param[out] heapAddr Buffer used internally by the Constant Update Engine to store per-resource-chunk ring buffers. This buffer is only used by the GPU.
			 *                 Use computeHeapSize() to determine its minimum size. Its minimum alignment is <c>Gnm::kMinimumBufferAlignmentInBytes</c>. This pointer must not be NULL.
			 * @param[in] numRingEntries Specifies how large each resource chunk's ring buffer should be. See computeHeapSize() for more information.
			 * @see computeHeapSize(), computeCpRamShadowSize()
			 */
			void init(void *cpramShadowBuffer, void *heapAddr, uint32_t numRingEntries);

			/**
			 * @brief Sets the address of the system's global resource table -- a collection of <c>V#</c>s which point to global buffers for various shader tasks.
			 * @param[out] addr The GPU-visible address of the global resource table. The minimum size of this buffer is given by
			 *             <c>Gnm::SCE_GNM_SHADER_GLOBAL_TABLE_SIZE</c>, and its minimum alignment is <c>Gnm::kMinimumBufferAlignmentInBytes</c>. This buffer is accessed by both the CPU and GPU.
			 *             This pointer must not be NULL.
			 */
			void setGlobalResourceTableAddr(void *addr);

			/**
			 * @brief Sets an entry in the global resource table.
			 * @note This function modifies the global resource table. It is not safe to modify the global resource table unless the GPU is idle.
			 * @param[in] resType The global resource type to bind a buffer for. Each global resource type has its own entry in the global resource table.
			 * @param[in] res The buffer to bind to the specified entry in the global resource table. The size of the buffer is global-resource-type-specific. Must not be NULL.
			 */
			void setGlobalDescriptor(Gnm::ShaderGlobalResourceType resType, const Gnm::Buffer *res);

			/** @brief Imposes a new upper bound on the number of read-only texture/buffer resources for a given shader stage.

			           Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
					   but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountResource.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveResourceSlotCount(Gnm::ShaderStage stage, uint32_t count);

			/** @brief Imposes a new upper bound on the number of read/write texture/buffer resources for a given shader stage.

			           Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountRwResource.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveRwResourceSlotCount(Gnm::ShaderStage stage, uint32_t count);

			/** @brief Imposes a new upper bound on the number of samplers for a given shader stage.

					   Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountSampler.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveSamplerSlotCount(Gnm::ShaderStage stage, uint32_t count);

			/** @brief Imposes a new upper bound on the number of vertex buffers for a given shader stage.

					   Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountVertexBuffer.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveVertexBufferSlotCount(Gnm::ShaderStage stage, uint32_t count);

			/** @brief Imposes a new upper bound on the number of constant buffers for a given shader stage.

				       Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountConstantBuffer.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveConstantBufferSlotCount(Gnm::ShaderStage stage, uint32_t count);

			/** @brief Imposes a new upper bound on the number of VS streamout buffers.

			           Decreasing this value reduces the number of unique resources that can be bound simultaneously,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] count The new slot count for the VS shader stage. Must be less than or equal to Gnm::kSlotCountStreamoutBuffer.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveStreamoutBufferSlotCount(uint32_t count);

			///////////////////////
			// Functions to bind shader resources
			///////////////////////

			/**
			 * @brief Binds one or more read-only texture objects to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountResource-1].
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] textures The Texture objects to bind to the specified slots. <c>textures[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c>textures[1]</c> to <c><i>startApiSlot</i> +1</c>, and so on.
			 *                 The contents of these Texture objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                 be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 * @note Buffers and Textures share the same pool of API slots.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setTextures(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Texture *textures);

			/**
			 * @brief Binds one or more read-only buffer objects to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountResource-1].
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] buffers The #Buffer objects to bind to the specified slots. <c>buffers[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c>buffers[1]</c> to <c><i>startApiSlot</i> +1</c>, and so on.
			 *                The contents of these #Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 * @note Buffers and Textures share the same pool of API slots.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers);

			/**
			 * @brief Binds one or more read/write texture objects to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountRwResource-1].
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] rwTextures The Texture objects to bind to the specified slots. <c><i>rwTextures</i>[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c><i>rwTextures</i>[1]</c> to <c><i>startApiSlot</i> +1</c>, etc.
			 *                   The contents of these Texture objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                   be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 * @note <c><i>rwBuffers</i></c> and <c><i>rwTextures</i></c> share the same pool of API slots.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setRwTextures(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Texture *rwTextures);

			/**
			 * @brief Binds one or more read/write buffer objects to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountRwResource-1].
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] rwBuffers The Buffer objects to bind to the specified slots. <c><i>rwBuffers</i>[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c><i>rwBuffers</i>[1]</c> to <c><i>startApiSlot</i> +1</c>, and so on.
			 *                  The contents of these Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                  be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 * @note <c><i>rwBuffers</i></c> and <c><i>rwTextures</i></c> share the same pool of API slots.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setRwBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *rwBuffers);

			/**
			 * @brief Binds one or more sampler objects to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountSampler-1].
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] samplers The Sampler objects to bind to the specified slots. <c>samplers[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c>samplers[1]</c> to <c><i>startApiSlot</i>+1</c>, and so on.
			 *                 The contents of these Sampler objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                 be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setSamplers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Sampler *samplers);

			/**
			 * @brief Binds one or more constant buffer objects to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountConstantBuffer-1].
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] buffers The constant buffer objects to bind to the specified slots. <c>buffers[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c>buffers[1]</c> to <c><i>startApiSlot</i> +1</c>, and so on.
			 *                The contents of these Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setConstantBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers);

			/**
			 * @brief Binds one or more vertex buffer objects to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountVertexBuffer-1].
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] buffers The vertex buffer objects to bind to the specified slots. <c>buffers[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c>buffers[1]</c> to <c><i>startApiSlot</i>+1</c>, and so on.
			 *                The contents of these Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setVertexBuffers(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers);

			/**
			 * @brief Binds one or more Boolean constants to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountBoolConstant-1]</c>.
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] bits The Boolean constants to bind to the specified slots. Each slot will contain 32 1-bit Boolean values packed together in a singled dword.
			 *             <c><i>bits</i>[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c>bits[1]</c> to <c><i>startApiSlot</i> + 1</c>, and so on.
			 *             The contents of the bits array are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *             be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setBoolConstants(Gnm::ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const uint32_t *bits);

			/**
			 * @brief Binds one or more floating-point constants to the specified shader stage.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountResource-1]</c>.
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] floats The constants to bind to the specified slots. <c>floats[0]</c> will be bound to <c><i>startApiSlot</i></c>, <c>floats[1]</c> to <c><i>startApiSlot</i>+1</c>, and so on.
			 *               The contents of these Texture objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *               be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
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
			 * @param[in] stage The shader stage to bind this counter range to.
			 * @param[in] rangeGdsOffsetInBytes The byte offset to the start of the counters in GDS. Must be a multiple of 4. 
			 * @param[in] rangeSizeInBytes The size of the counter range in bytes. Must be a multiple of 4.
			 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
			 */
			void setAppendConsumeCounterRange(Gnm::ShaderStage stage, uint32_t rangeGdsOffsetInBytes, uint32_t rangeSizeInBytes);

            /**
			 * @brief Binds one or more streamout buffer objects to the specified shader stage.
			 * @param[in] startApiSlot The first API slot to bind to. Valid slots are [0..Gnm::kSlotCountStreamoutBuffer-1].
			 * @param[in] numApiSlots The number of consecutive API slots to bind.
			 * @param[in] buffers The streamout buffer objects to bind to the specified slots. buffers[0] will be bound to <c><i>startSlot</i></c>, buffers[1] to <c><i>startSlot</i> + 1</c>, and so on.
			 *                The contents of these Buffer objects are cached locally inside the Constant Update Engine. If this parameter is NULL, the specified slots will
			 *                be unbound; this is not necessary for regular operation, but may be handy for debugging purposes.
			 */
			void setStreamoutBuffers(uint32_t startApiSlot, uint32_t numApiSlots, const Gnm::Buffer *buffers);

			/**
			 * @brief Specifies a range of the Global Data Store to be used by shaders.
			 *
			 *  This function never rolls the hardware context.
			 *
			 * @param[in] stage The shader stage to bind this range to.
			 * @param[in] rangeGdsOffsetInBytes The byte offset to the start of the range in GDS. Must be a multiple of 4. 
			 * @param[in] rangeSizeInBytes The size of the counter range in bytes. Must be a multiple of 4.
			 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes. It is an error to specify a range outside these bounds.
			 */
			void setGdsMemoryRange(Gnm::ShaderStage stage, uint32_t rangeGdsOffsetInBytes, uint32_t rangeSizeInBytes);

			//////////////////////
			// Functions to bind shaders
			//////////////////////
			/**
			 * @brief Sets the active shader stages in the graphics pipeline for preDraw calls, 
			 *        which should be kept in sync with calls to DrawCommandBuffer::setActiveShaderStages().
			 * 
			 * @param[in] activeStages Indicates which shader stages should be considered activated for following preDraw calls.
			 */
			void setActiveShaderStages(Gnm::ActiveShaderStages activeStages)
			{
				m_activeShaderStages = activeStages;
			}
			/** @brief Gets the last value set using setActiveShaderStages()
			 * 
			 * @return The shader stages that will be considered activated by preDraw calls.
			 */			
			Gnm::ActiveShaderStages getActiveShaderStages() const
			{
				return m_activeShaderStages;
			}

			/**
			 * @brief Binds a shader to the VS stage.
			 * @param[in] vsb Pointer to the shader to bind to the VS stage.
			 * @param[in] shaderModifier Shader Modifier value generated by generateVsFetchShaderBuildState(), use 0 if no fetch shader.
			 * @param[in] fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>vsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setVsShader(const Gnmx::VsShader *vsb, uint32_t shaderModifier, void *fetchShaderAddr);


			/**
			 * @brief Binds a shader pair to the asynchronous compute and VS stages for use in a Gnm::DrawCommandBuffer::dispatchDraw() call.
			 * @param[in] csvsb Pointer to the shaders to bind to the asynchronous compute and VS stage.
			 * @param[in] shaderModifierVs If the VS shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param[in] fetchShaderAddrVs If the VS shader requires a fetch shader, pass its GPU address here; otherwise pass 0.
			 * @param[in] shaderModifierCs If the CS shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param[in] fetchShaderAddrCs If the CS shader requires a fetch shader, pass its GPU address here; otherwise, pass 0.
			 * @note  Setting the asynchronous compute shader does not affect the currently set graphics CS stage shader, which is set by setCsShader().
			 *		  Asynchronous compute shaders share the user data constants and CUE ring elements set up for Gnm::kShaderStageVs,
			 *		  while graphics CS stage shaders use the user data constants and CUE ring elements set up for Gnm::kShaderStageCs.
			 * @note  Only the pointers <c>csvsb->getVertexShader()</c> and <c>csvsb->getComputeShader()</c> are cached inside the ConstantUpdateEngine; the location 
			 *		  and contents of these pointers should not be changed before all calls to Gnm::DrawCommandBuffer::dispatchDraw() which use this shader have been made!
			 * @note This binding will not take effect on the GPU until preDispatchDraw() or preDraw() is called.
			 */
			void setCsVsShaders(const Gnmx::CsVsShader *csvsb, uint32_t shaderModifierVs, void *fetchShaderAddrVs, uint32_t shaderModifierCs, void *fetchShaderAddrCs);

			/**
			 * @brief Binds a shader to the asynchronous compute stage, for use in a Gnm::DrawCommandBuffer::dispatchDraw() call.
			 * Generally, compute shaders for dispatch draw are packed into a CsVsShader and so will instead be set by calling ConstantUpdateEngine::setCsVsShaders().
			 * This function never rolls the hardware context.
			 * @param[in] csb Pointer to the shader to bind to the asynchronous compute stage.
			 * @param[in] shaderModifierCs If the compute shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param[in] fetchShaderAddrCs If the compute shader requires a fetch shader, pass its GPU address here; otherwise pass 0.
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
			 * @param[in] psb Pointer to the shader to bind to the PS stage.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>psb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setPsShader(const Gnmx::PsShader *psb);

			/**
			 * @brief Sets the PS Input Table
			 * @param[in] psInput PS Input table.
			 * @note This function should be called after calls to setPsShader() and setVsShader().
			 * Calls to setPsShader() or setVsShader() will invalidate this table.
			 */
			void setPsInputUsageTable(uint32_t *psInput)
			{
				m_psInputs = psInput;
			}

			/**
			 * @brief Binds a shader to the CS stage.
			 * @param[in] csb Pointer to the shader to bind to the CS stage.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>csb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setCsShader(const Gnmx::CsShader *csb);

			/**
			 * @brief Binds a shader to the LS stage.
			 * @param[in] lsb Pointer to the shader to bind to the LS stage.
			 * @param[in] shaderModifier Shader Modifier value generated by generateLsFetchShaderBuildState(), use 0 if no fetch shader.
			 * @param[in] fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>lsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setLsShader(const Gnmx::LsShader *lsb, uint32_t shaderModifier, void *fetchShaderAddr);

			/**
			 * @brief Binds a shader to the LS stage.
			 * @param[in] lsb Pointer to the shader to bind to the LS stage.
			 * @param[in] shaderModifier Shader Modifier value generated by generateLsFetchShaderBuildState(), use 0 if no fetch shader.
			 * @param[in] fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @param[in] lsRegs LS stage registers to be copied into CUE. The register contents will be cached inside the Constant Update Engine.
			 * @note  RAGE specific, thread safe version.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>lsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setLsShader(const Gnmx::LsShader *lsb, uint32_t shaderModifier, void *fetchShaderAddr, const Gnm::LsStageRegisters *lsRegs);

			/**
			 * @brief Binds a shader to the HS stage.
			 * @param[in] hsb Pointer to the shader to bind to the HS stage.
			 * @param[in] tessRegs Tessellation register settings for this HS shader. The register contents will be cached inside the Constant Update Engine.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>hsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setHsShader(const Gnmx::HsShader *hsb, const Gnm::TessellationRegisters *tessRegs);

			/**
			 * @brief Binds a shader to the ES stage.
			 * @param[in] esb Pointer to the shader to bind to the ES stage.
			 * @param[in] shaderModifier Shader Modifier value generated by generateEsFetchShaderBuildState(), use 0 if no fetch shader.
			 * @param[in] fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>esb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setEsShader(const Gnmx::EsShader *esb, uint32_t shaderModifier, void *fetchShaderAddr);

			/**
			 * @brief Binds a shader to the ES stage.
			 * @param[in] esb Pointer to the shader to bind to the ES stage.
			 * @param[in] ldsSizeIn512Bytes The LDS allocation size in 512-byte granularity allocation units. Internally, this value will be passed to
			 *                              EsStageRegisters::updateLdsSize() before the EsShader is bound. If this parameter is 0, the function behaves
			 *                              identically to ConstantUpdateEngine::setEsShader().
			 * @param[in] shaderModifier Shader Modifier value generated by generateEsFetchShaderBuildState(), use 0 if no fetch shader.
			 * @param[in] fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>esb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setOnChipEsShader(const Gnmx::EsShader *esb, uint32_t ldsSizeIn512Bytes, uint32_t shaderModifier, void *fetchShaderAddr);


			/**
			 * @brief Binds a shader to the GS and VS stages.
			 * @param[in] gsb Pointer to the shader to bind to the GS/VS stages.
			 * @note  Only the pointer is cached inside the Constant Update Engine; the location and contents of <c>*<i>gsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @note This binding will not take effect on the GPU until preDraw() or preDispatch() is called.
			 */
			void setGsVsShaders(const GsShader *gsb);

			////////////////////////////////
			// Shader Resource Table (SRT) functions
			////////////////////////////////

			/**
			 * @brief Binds an internal SRT buffer. 
			 
			   This buffer must be allocated by the caller, but its contents are filled in automatically by the constant update engine.
			 * @param[in] stage The shader stage to bind this buffer to.
			 * @param[in] buf A pointer to the buffer. In the future, a function will written to obtain the size of the buffer.
			 * @sa setUserSrtBuffer()
			 */
			void setInternalSrtBuffer(Gnm::ShaderStage stage, void *buf);

			/**
			 * @brief Binds a user SRT buffer. 
			 
				This buffer is filled in by the developer.
			 * @param[in] stage The shader stage to bind this buffer to.
			 * @param[in] buf Pointer to the buffer. If NULL, <c><i>bufSizeInDwords</i></c> must be 0.
			 * @param[in] bufSizeInDwords Size of the data pointed to by <c><i>buf</i></c> in dwords. Valid range is [1..8] if <c><i>buf</i></c> is non-NULL.
			 * @sa setInternalSrtBuffer()
			 */
			void setUserSrtBuffer(Gnm::ShaderStage stage, const void *buf, uint32_t bufSizeInDwords);

			////////////////////////////////
			// Draw/Dispatch related functions
			////////////////////////////////

			/**
			 * @brief Executes all previous enqueued resource and shader bindings in preparation for a draw call.
			 *
			 * This may involve waiting on a previous draw to complete before kicking off a DMA to copy
			 * dirty resource chunks to/from CPRAM.
			 * @note When using the Constant Update Engine to manage shaders and shader resources, this function must be called immediately before every draw call,
			 * and postDraw() must be called immediately afterwards.
			 * @param[in,out] dcb The draw command buffer to write commands to.
			 * @param[in,out] ccb The constant command buffer to write commands to.
			 * @see postDraw()
			 */
			void preDraw(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb);

			/**
			 * @brief Inserts GPU commands to indicate that the resources used by the most recent draw call can be safely overwritten in the Constant Update Engine ring buffers.
			 *
			 * @note When using the Constant Update Engine to manage shaders and shader resources, preDraw() must be called immediately before every draw call,
			 * and this function must be called immediately afterwards.
			 * @param[in,out] dcb The draw command buffer to write commands to.
			 * @param[in,out] ccb The constant command buffer to write commands to.
			 * @see preDraw()
			 */
			void postDraw(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb);

			/**
			 * @brief Executes all previous enqueued resource and shader bindings in preparation for a dispatch call.
			 *
			 * This may involve waiting on a previous draw to complete before kicking off a DMA to copy
			 * dirty resource chunks to/from CPRAM.
			 * @note When using the ConstantUpdateEngine to manage shaders and shader resources, this function must be called immediately before every dispatch call,
			 * and postDispatch() must be called immediately afterwards.
			 * @param[in,out] dcb The draw or dispatch command buffer to write commands to.
			 * @param[in,out] ccb The constant command buffer to write commands to.
			 * @see postDispatch()
			 */
			void preDispatch(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb);

			/**
			 * @brief Inserts GPU commands to indicate that the resources used by the most recent dispatch call can be safely overwritten in the ConstantUpdateEngine ring buffers.
			 *
			 * @note When using the ConstantUpdateEngine to manage shaders and shader resources, preDispatch() must be called immediately before every dispatch call,
			 * and this function must be called immediately afterwards.
			 * @param[in,out] dcb The draw or dispatch command buffer to write commands to.
			 * @param[in,out] ccb The constant command buffer to write commands to.
			 * @see preDispatch()
			 */
			void postDispatch(Gnm::DrawCommandBuffer *dcb, Gnm::ConstantCommandBuffer *ccb);
			/**
			 * @brief Executes all previous enqueued resource and shader bindings in preparation for a Gnm::DrawCommandBuffer::dispatchDraw() call.
			 *
			 * This may involve waiting on a previous draw to complete before kicking off a DMA to copy
			 * dirty resource chunks to/from CPRAM.
			 *
			 * @note When using the constant update engine to manage shaders and shader resources, this function must be called immediately before every Gnm::DrawCommandBuffer::dispatchDraw() call,
			 * and postDraw() must be called immediately afterwards.
			 * 
			 * @param[in,out]	dcb										The draw command buffer to write commands to.
			 * @param[in,out]	acb										The asynchronous compute dispatch command buffer to write commands to.
			 * @param[in,out]	ccb										The constant command buffer to write commands to.
			 * @param[out]		pOutOrderedAppendMode					Receives the current asynchronous compute CsShader::m_orderedAppendMode to be passed to DispatchCommandBuffer::dispatchDraw().
			 * @param[out]		pOutDispatchDrawIndexDeallocationMask	Receives the dispatch draw index mask value specified by the current asynchronous compute CsShader to be passed to DrawCommandBuffer::setDispatchDrawIndexDeallocationMask().
			 * @param[out]		pOutSgprKrbLoc							Receives the user sgpr of type kShaderInputUsageImmGdsKickRingBufferOffset in the asynchronous compute CsShader::getInputUsageSlotTable() to be passed to DispatchCommandBuffer::dispatchDraw().
			 * @param[out]		pOutSgprInstanceCs						Receives the user sgpr of type kShaderInputUsageImmDispatchDrawInstances in asynchronous compute CsShader::getInputUsageSlotTable(); if set to NULL, this does not happen.
			 * @param[out]		pOutDispatchDrawMode					Receives the current DispatchDrawMode implied by the current VsShader::getInputUsageSlotTable() to be passed to DrawCommandBuffer::dispatchDraw().
			 * @param[out]		pOutSgprVrbLoc							Receives the user sgpr of type kShaderInputUsageImmVertexRingBufferOffset in VsShader::getInputUsageSlotTable() to be passed to DrawCommandBuffer::dispatchDraw().
			 * @param[out]		pOutSgprInstanceVs						Receives the user sgpr of type kShaderInputUsageImmDispatchDrawInstances in VsShader::getInputUsageSlotTable(); if set to NULL, this does not happen.
			 *
			 * @see postDispatchDraw(), Gnm::DispatchCommandBuffer::dispatchDraw(), Gnm::DrawCommandBuffer::dispatchDraw()
			 */
			void preDispatchDraw(Gnm::DrawCommandBuffer *dcb, Gnm::DispatchCommandBuffer *acb, Gnm::ConstantCommandBuffer *ccb,
				Gnm::DispatchOrderedAppendMode *pOutOrderedAppendMode, uint32_t *pOutDispatchDrawIndexDeallocationMask, uint32_t *pOutSgprKrbLoc, uint32_t *pOutSgprInstanceCs, 
				Gnm::DispatchDrawMode *pOutDispatchDrawMode, uint32_t *pOutSgprVrbLoc, uint32_t *pOutSgprInstanceVs);
			
			/**
			 * @brief Inserts GPU commands to indicate that the resources used by the most recent Gnm::DrawCommandBuffer::dispatchDraw() call can be safely overwritten in the constant update engine ring buffers. 
			 *
			 * @note When using the constant update engine to manage shaders and shader resources, preDispatchDraw() must be called immediately before every dispatchDraw() call
			 * and this function must be called immediately afterwards.
			 *
			 * @param[in,out] dcb		The draw command buffer to write commands to.
			 * @param[in,out] acb		The asynchronous compute dispatch command buffer to write commands to.
			 * @param[in,out] ccb		The constant command buffer to write commands to.
			 *
			 * @see preDispatchDraw(), Gnm::DrawCommandBuffer::dispatchDraw()
			 */
			void postDispatchDraw(Gnm::DrawCommandBuffer *dcb, Gnm::DispatchCommandBuffer *acb, Gnm::ConstantCommandBuffer *ccb);

			/** @brief Sets the vertex and instance offset for the current shader configuration.
			 *
			 * The vertex and instance offsets work only when enabled in the vertex shader.
			 *
			 * @param dcb				The draw command buffer.	 
			 * @param vertexOffset		The offset added to each vertex index.
			 * @param instanceOffset	The offset added to instance index.
			 */
			void setVertexAndInstanceOffset(Gnm::DrawCommandBuffer *dcb, uint32_t vertexOffset, uint32_t instanceOffset);

			 /** @brief Checks if the current shader configuration is expecting a vertex or instance offset.
			  */
			 bool isVertexOrInstanceOffsetEnabled() const;

			/**
			 * @brief Call this function at the beginning of every frame.
			 */
			void advanceFrame(void);

			/**
			 * @brief Invalidate all current shader and resource bindings.
			 */
			void invalidateAllBindings(void);

			/** @brief Returns the size (in bytes) of the ConstantUpdateEngine's heap buffer, as passed to ConstantUpdateEngine::init().
				@return The size (in bytes) of the ConstantUpdateEngine's heap buffer.
				*/
			uint32_t getHeapSize(void) const
			{
				return m_heapSize;
			}

			void setOnChipEsVertsPerSubGroup(uint16_t onChipEsVertsPerSubGroup)
			{
				m_onChipEsVertsPerSubGroup = onChipEsVertsPerSubGroup;
			}

			void setOnChipEsExportVertexSizeInDword(uint16_t onChipEsExportVertexSizeInDword)
			{
				m_onChipEsExportVertexSizeInDword = onChipEsExportVertexSizeInDword;
			}

			////////////////////////////////
			// DispatchDraw related functions
			////////////////////////////////

			void setDispatchDrawData(void const* pDispatchDrawData, uint32_t sizeofDispatchDrawData)
			{
				m_pDispatchDrawData = pDispatchDrawData;
				m_sizeofDispatchDrawData = sizeofDispatchDrawData;
			}
			
		private:
#if !defined(DOXYGEN_IGNORE)
			uint32_t m_heapSize; // Size of the heap buffer passed to init(), in bytes

			// Private ring buffer struct
			class CueRingBuffer
			{
			public:
				uint64_t m_headElemIndex;
				void *m_elementsAddr; // GPU address of the array of elements.
				uint32_t m_elemSizeDwords; // Size of each element, in dwords.
				uint32_t m_elemCount; // Capacity of the buffer, in elements.
				uint64_t m_wrappedIndex;
				uint64_t m_halfPointIndex;

				// Use to determine size of buffer argument for init().
				static uint32_t computeSpaceRequirements(uint32_t elemSizeDwords, uint32_t elemCount);

				void init(void *bufferAddr, uint32_t bufferBytes, uint32_t elemSizeDwords, uint32_t elemCount);

				// Gets a pointer to the current head element.
				void *getCurrentHead(void) const;

				// Advances the head pointer (if the buffer isn't stalled) and returns a pointer to the new head element.
				bool advanceHead(void);

				void submitted();
			};

			// Offset into each type's range of data within a shader stage, in DWORDs.
			static const uint32_t kDwordOffsetResource	       = 0;
			static const uint32_t kDwordOffsetRwResource       = kDwordOffsetResource                     + Gnm::kSlotCountResource                 * Gnm::kDwordSizeResource;
			static const uint32_t kDwordOffsetSampler		   = kDwordOffsetRwResource                   + Gnm::kSlotCountRwResource               * Gnm::kDwordSizeRwResource;
			static const uint32_t kDwordOffsetVertexBuffer     = kDwordOffsetSampler                      + Gnm::kSlotCountSampler                  * Gnm::kDwordSizeSampler;
			static const uint32_t kDwordOffsetConstantBuffer   = kDwordOffsetVertexBuffer                 + Gnm::kSlotCountVertexBuffer             * Gnm::kDwordSizeVertexBuffer;
			static const uint32_t kDwordOffsetBoolConstant     = kDwordOffsetConstantBuffer               + Gnm::kSlotCountConstantBuffer           * Gnm::kDwordSizeConstantBuffer;
			static const uint32_t kDwordOffsetFloatConstant	   = kDwordOffsetBoolConstant                 + Gnm::kSlotCountBoolConstant             * Gnm::kDwordSizeBoolConstant;
			static const uint32_t kDwordOffsetAppendConsumeGdsCounterRange = kDwordOffsetFloatConstant    + Gnm::kSlotCountFloatConstant            * Gnm::kDwordSizeFloatConstant;
			static const uint32_t kDwordOffsetStreamoutBuffer  = kDwordOffsetAppendConsumeGdsCounterRange + Gnm::kSlotCountAppendConsumeCounterRange* Gnm::kDwordSizeAppendConsumeCounterRange;
			static const uint32_t kDwordOffsetExtendedUserData = kDwordOffsetStreamoutBuffer              + Gnm::kSlotCountStreamoutBuffer          * Gnm::kDwordSizeStreamoutBuffer;
			static const uint32_t kDwordOffsetGdsMemoryRange   = kDwordOffsetExtendedUserData             + Gnm::kSlotCountExtendedUserData         * Gnm::kDwordSizeExtendedUserData;
			static const uint32_t kPerStageDwordSize           = kDwordOffsetGdsMemoryRange               + Gnm::kSlotCountGdsMemoryRange           * Gnm::kDwordSizeGdsMemoryRange;
			// Size of each chunk type, in bytes.
			static const uint32_t kChunkBytesResource		   = Gnm::kChunkSlotCountResource         * Gnm::kDwordSizeResource         * sizeof(uint32_t);
			static const uint32_t kChunkBytesRwResource	       = Gnm::kChunkSlotCountRwResource       * Gnm::kDwordSizeRwResource       * sizeof(uint32_t);
			static const uint32_t kChunkBytesSampler		   = Gnm::kChunkSlotCountSampler          * Gnm::kDwordSizeSampler          * sizeof(uint32_t);
			static const uint32_t kChunkBytesVertexBuffer	   = Gnm::kChunkSlotCountVertexBuffer     * Gnm::kDwordSizeVertexBuffer     * sizeof(uint32_t);
			static const uint32_t kChunkBytesConstantBuffer    = Gnm::kChunkSlotCountConstantBuffer   * Gnm::kDwordSizeConstantBuffer   * sizeof(uint32_t);
			static const uint32_t kChunkBytesStreamoutBuffer   = Gnm::kChunkSlotCountStreamoutBuffer  * Gnm::kDwordSizeStreamoutBuffer  * sizeof(uint32_t);
			static const uint32_t kChunkBytesExtendedUserData  = Gnm::kChunkSlotCountExtendedUserData * Gnm::kDwordSizeExtendedUserData * sizeof(uint32_t);
			// Number of chunks of each type, for each shader stage.
			static const uint32_t kChunkCountResource		   = Gnm::kSlotCountResource         / Gnm::kChunkSlotCountResource;
			static const uint32_t kChunkCountRwResource	       = Gnm::kSlotCountRwResource       / Gnm::kChunkSlotCountRwResource;
			static const uint32_t kChunkCountSampler		   = Gnm::kSlotCountSampler          / Gnm::kChunkSlotCountSampler;
			static const uint32_t kChunkCountVertexBuffer	   = Gnm::kSlotCountVertexBuffer     / Gnm::kChunkSlotCountVertexBuffer;
			static const uint32_t kChunkCountConstantBuffer    = Gnm::kSlotCountConstantBuffer   / Gnm::kChunkSlotCountConstantBuffer;
			static const uint32_t kChunkCountStreamoutBuffer   = Gnm::kSlotCountStreamoutBuffer  / Gnm::kChunkSlotCountStreamoutBuffer;
			static const uint32_t kChunkCountExtendedUserData  = Gnm::kSlotCountExtendedUserData / Gnm::kChunkSlotCountExtendedUserData;
			// Ring buffer offsets for each resource type.
			static const uint32_t kRingBuffersIndexResource         = 0;
			static const uint32_t kRingBuffersIndexRwResource       = kRingBuffersIndexResource        + kChunkCountResource;
			static const uint32_t kRingBuffersIndexSampler          = kRingBuffersIndexRwResource      + kChunkCountRwResource;
			static const uint32_t kRingBuffersIndexVertexBuffer     = kRingBuffersIndexSampler         + kChunkCountSampler;
			static const uint32_t kRingBuffersIndexConstantBuffer   = kRingBuffersIndexVertexBuffer    + kChunkCountVertexBuffer;
			static const uint32_t kRingBuffersIndexStreamoutBuffer  = kRingBuffersIndexConstantBuffer  + kChunkCountConstantBuffer;
			static const uint32_t kRingBuffersIndexExtendedUserData = kRingBuffersIndexStreamoutBuffer + kChunkCountStreamoutBuffer;
			static const uint32_t kNumRingBuffersPerStage           = 
				kChunkCountResource +
				kChunkCountRwResource +
				kChunkCountSampler +
				kChunkCountVertexBuffer +
				kChunkCountConstantBuffer +
				kChunkCountStreamoutBuffer +
				kChunkCountExtendedUserData +
				0;

			void updateCpRam(Gnm::ConstantCommandBuffer *ccb, Gnm::ShaderStage stage, const Gnm::InputUsageSlot *usageSlots, const uint32_t numInputSlots, void *outUsageChunkAddrs[16]);
			void applyConstantsForDraw(Gnm::DrawCommandBuffer *dcb, Gnm::ShaderStage stage, const Gnm::InputUsageSlot *usageSlots, const uint32_t numInputSlots, void *usageChunkAddrs[16]);
			void applyConstantsForDispatchDrawAcb(Gnm::DispatchCommandBuffer *acb, const Gnm::InputUsageSlot *usageSlots, const uint32_t numInputSlots, void *usageChunkAddrs[16]);

			/** @brief Writes CCB commands to dump a dirty chunk from the CPRAM shadow buffer into CPRAM, and from there into its corresponding ring buffer.
			@param[in,out] ccb The writeToCpRam and cpRamDump packets will be written to this command buffer.
			@param[in] chunkByteOffset Byte offset of the chunk within this Constant Update Engine's view of CPRAM.
			@param[in] chunkBytes Size of the chunk to dump, in bytes.
			@param[in,out] ringBuffer The ring buffer to which this chunk should eventually be dumped.
			*/
			void dumpDirtyChunkToRingBuffer(Gnm::ConstantCommandBuffer *ccb, uint16_t chunkByteOffset, uint32_t chunkBytes, CueRingBuffer &ringBuffer);

			// Shadows the cpram for future userdata and extended userdata 'constants'.
			uint32_t *m_cpRamShadowDwords;

			// Per-chunk dirty bits. "Dirty" means "differs from what's in the current ring-buffer head, and should be written from CPRAM to a new ring buffer entry".
			uint8_t m_dirtyResourceChunkBits[        Gnm::kShaderStageCount][(kChunkCountResource         + 7) / 8];
			uint8_t m_dirtyRwResourceChunkBits[      Gnm::kShaderStageCount][(kChunkCountRwResource       + 7) / 8];
			uint8_t m_dirtySamplerChunkBits[         Gnm::kShaderStageCount][(kChunkCountSampler          + 7) / 8];
			uint8_t m_dirtyVertexBufferChunkBits[    Gnm::kShaderStageCount][(kChunkCountVertexBuffer     + 7) / 8];
			uint8_t m_dirtyConstantBufferChunkBits[  Gnm::kShaderStageCount][(kChunkCountConstantBuffer   + 7) / 8];
			uint8_t m_dirtyStreamoutBufferChunkBits[ Gnm::kShaderStageCount][(kChunkCountStreamoutBuffer  + 7) / 8];
			uint8_t m_dirtyExtendedUserDataChunkBits[Gnm::kShaderStageCount][(kChunkCountExtendedUserData + 7) / 8];

			// Per-API-slot dirty bits. One bit per API slot, rounded up to the next byte. "Dirty" means "the user bound a new value to this API slot since the last time
			// a shader requested this resource".
			uint8_t m_dirtyResourceSlotBits[        Gnm::kShaderStageCount][(Gnm::kSlotCountResource         + 7) / 8];
			uint8_t m_dirtyRwResourceSlotBits[      Gnm::kShaderStageCount][(Gnm::kSlotCountRwResource       + 7) / 8];
			uint8_t m_dirtySamplerSlotBits[         Gnm::kShaderStageCount][(Gnm::kSlotCountSampler          + 7) / 8];
			uint8_t m_dirtyVertexBufferSlotBits[    Gnm::kShaderStageCount][(Gnm::kSlotCountVertexBuffer     + 7) / 8];
			uint8_t m_dirtyConstantBufferSlotBits[  Gnm::kShaderStageCount][(Gnm::kSlotCountConstantBuffer   + 7) / 8];
			uint8_t m_dirtyStreamoutBufferSlotBits[ Gnm::kShaderStageCount][(Gnm::kSlotCountStreamoutBuffer  + 7) / 8];
			uint8_t m_dirtyExtendedUserDataSlotBits[Gnm::kShaderStageCount][(Gnm::kSlotCountExtendedUserData + 7) / 8]; // this field refers to the EUD as a whole, not individual slots within ExtendedUserData.

			// Artificial limits on the number of shader resource slots per shader stage. Lowering these counts from the default values can improve the performance
			// of the ConstantUpdateEngine.
			uint8_t m_activeResourceSlotCounts[        Gnm::kShaderStageCount]; ///< Must be less than or equal to Gnm::kSlotCountResource.
			uint8_t m_activeRwResourceSlotCounts[      Gnm::kShaderStageCount]; ///< Must be less than or equal to Gnm::kSlotCountRwResource.
			uint8_t m_activeSamplerSlotCounts[         Gnm::kShaderStageCount]; ///< Must be less than or equal to Gnm::kSlotCountSampler.
			uint8_t m_activeVertexBufferSlotCounts[    Gnm::kShaderStageCount]; ///< Must be less than or equal to Gnm::kSlotCountVertexBuffer.
			uint8_t m_activeConstantBufferSlotCounts[  Gnm::kShaderStageCount]; ///< Must be less than or equal to Gnm::kSlotCountConstantBuffer.
			uint8_t m_activeStreamoutBufferSlotCount;                           ///< Must be less than or equal to Gnm::kSlotCountStreamoutBuffer.

			// Internal SRT buffers
			void *m_internalSrtBuffers[Gnm::kShaderStageCount];
			uint32_t m_userSrtData[Gnm::kShaderStageCount][8];
			uint32_t m_userSrtDataSizeInDwords[Gnm::kShaderStageCount];

			// Stores the range of Extended User Data slots that are actually dirty (and must therefore be DMA'd to/from CPRAM)
			class { public: uint8_t m_firstSlot, m_lastSlotPlusOne; } m_dirtyEudSlotRanges[Gnm::kShaderStageCount];
			// Stores the range of Dispatch Draw Data dwords that are actually dirty (and must therefore be DMA'd to/from CPRAM)
			class { public: uint8_t m_firstDword, m_lastDwordPlusOne; } m_dirtyDddDwordRanges;

			// Cached pointers to the most recently bound shaders.
			void *m_currentVSFetchShaderAddr;
			void *m_currentLSFetchShaderAddr;
			void *m_currentESFetchShaderAddr;
			void *m_currentAcbCSFetchShaderAddr;
			uint32_t m_currentVsShaderModifier;
			uint32_t m_currentLsShaderModifier;
			uint32_t m_currentEsShaderModifier;
			uint32_t m_currentAcbCsShaderModifier;

			public:
			const Gnmx::VsShader *m_currentVSB;
			const Gnmx::PsShader *m_currentPSB;
			const Gnmx::CsShader *m_currentCSB;
			const Gnmx::LsShader *m_currentLSB;
			const Gnmx::HsShader *m_currentHSB;
			const Gnmx::EsShader *m_currentESB;
			const Gnmx::GsShader *m_currentGSB;
			const Gnmx::CsShader *m_currentAcbCSB;
			private:
			Gnm::ActiveShaderStages m_activeShaderStages;
			bool m_dirtyShaders[Gnm::kShaderStageCount+1];
			bool m_isSrtShader[Gnm::kShaderStageCount+1];
			bool m_anyWrapped;
			uint32_t m_ldsSizeIn512Bytes;

			uint32_t *m_psInputs;

			Gnm::TessellationRegisters m_currentTessRegs;
			Gnm::LsStageRegisters m_currentLsRegs;

			void const* m_pDispatchDrawData;
			uint32_t m_sizeofDispatchDrawData;

			void *m_globalTableAddr;
			uint32_t m_globalTablePtr[SCE_GNM_SHADER_GLOBAL_TABLE_SIZE / sizeof(uint32_t)];
			bool     m_globalTableNeedsUpdate;

			uint16_t getResourceDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getRwResourceDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getSamplerDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getConstantBufferDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getVertexBufferDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getBoolConstantDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getFloatConstantDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getAppendConsumeCounterRangeDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getStreamoutDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getExtendedUserDataDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;
			uint16_t getGdsMemoryRangeDwordOffset(Gnm::ShaderStage stage, uint32_t slot) const;

			CueRingBuffer m_ringBuffers[Gnm::kShaderStageCount][kNumRingBuffersPerStage];
			uint32_t m_chunksPerRingBuffer;

			uint16_t m_onChipEsVertsPerSubGroup;
			uint16_t m_onChipEsExportVertexSizeInDword;

			bool m_betweenPreAndPostDraw;

			void writeDirtyRangesToCpRam(Gnm::ConstantCommandBuffer *ccb, uint8_t *dirtyBits, int32_t firstBitIndex, int32_t bitCount, uint16_t baseByteOffset, uint32_t recordBytes);

			void setTsharpInExtendedUserData(Gnm::ShaderStage stage, uint8_t userDataSlot, const Gnm::Texture *tex);
			void setSsharpInExtendedUserData(Gnm::ShaderStage stage, uint8_t userDataSlot, const Gnm::Sampler *sampler);
			void setVsharpInExtendedUserData(Gnm::ShaderStage stage, uint8_t userDataSlot, const Gnm::Buffer *buffer);
			void setPointerInExtendedUserData(Gnm::ShaderStage stage, uint8_t userDataSlot, void *gpuAddr);
			void setDwordInExtendedUserData(Gnm::ShaderStage stage, uint8_t userDataSlot, uint32_t data);
#endif // !defined(DOXYGEN_IGNORE)
		};
	}
}

#endif // !SCE_GNMX_ENABLE_CUE_V2
#endif
