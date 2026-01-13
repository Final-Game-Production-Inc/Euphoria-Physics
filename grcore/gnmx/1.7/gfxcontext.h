/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_GFXCONTEXT_H)
#define _SCE_GNMX_GFXCONTEXT_H

#include <gnm/constantcommandbuffer.h>
#include <gnm/drawcommandbuffer.h>
#include <gnm/offline.h>
#include "grcore/gnmx/constantupdateengine.h"
#include "grcore/gnmx/helpers.h"
#include "grcore/gnmx/computequeue.h"
#include "grcore/gnmx/dispatchdraw.h"
#include "shaderbinary.h"

// TEMPORARY FOR DEPRECATE REASONS:
#include <gnm/platform.h>

//#define SCE_GNMX_RECORD_LAST_COMPLETION
//#define SCE_GNMX_ENABLE_GFXCONTEXT_CALLCOMMANDBUFFERS

namespace sce
{
	namespace Gnmx
	{

		/** @brief Encapsulates a Gnm::DrawCommandBuffer and Gnm::ConstantCommandBuffer pair and a ConstantUpdateEngine and wraps them in a single higher-level interface.
		 *
		 * This should be a new developer's main entry point into the PlayStation®4 rendering API.
		 * The GfxContext object submits to the graphics ring, and supports both graphics and compute operations; for compute-only tasks, use the ComputeContext class.
		 * @see ComputeContext
		 */
		class SCE_GNMX_EXPORT GfxContext
		{
		public:
			/** @brief Default constructor. */
			GfxContext(void);
			/** @brief Default destructor. */
			~GfxContext(void);

			/** @brief Initializes a GfxContext with application-provided memory buffers.
				@param[out] cueCpRamShadowBuffer A buffer used by the Constant Update Engine to shadow the contents of CPRAM. Use
											ConstantUpdateEngine::computeCpRamShadowSize() to determine the correct buffer size.
											This buffer should NOT be in shared / GPU-visible memory, as the CPU will be reading
											from it very heavily!
				@param[out] cueHeapAddr A buffer in VRAM for use by the Constant Update Engine. Use ConstantUpdateEngine::computeHeapSize()
								   to determine the correct buffer size.
				@param[in] numRingEntries The number of entries in each internal ConstantUpdateEngine ring buffer. For now, pass 16.
				@param[out] dcbBuffer A buffer for use by <c>m_dcb</c>.
				@param[in] dcbSizeInBytes The size of <c><i>dcbBuffer</i></c> in bytes.
				@param[out] ccbBuffer A buffer for use by <c>m_ccb</c>.
				@param[in] ccbSizeInBytes The size of <c><i>ccbBuffer</i></c> in bytes.
				*/
			void init(void *cueCpRamShadowBuffer, void *cueHeapAddr, uint32_t numRingEntries,
					  void *dcbBuffer, uint32_t dcbSizeInBytes, void *ccbBuffer, uint32_t ccbSizeInBytes);

#ifdef SCE_GNMX_ENABLE_CUE_V2
			/** @brief Initializes a GfxContext with application-provided memory buffers.

				@param[out] cueHeapAddr		A buffer in VRAM for use by the Constant Update Engine. Use ConstantUpdateEngine::computeHeapSize()
											to determine the correct buffer size.
				@param[in] numRingEntries	The number of entries in each internal ConstantUpdateEngine ring buffer. For now a value of 16 should be passed.
				@param[in] ringSetup		The ring configuration for the constant update engine.
				@param[out] dcbBuffer		A buffer for use by <c>m_dcb</c>.
				@param[in] dcbSizeInBytes	The size of <c><i>dcbBuffer</i></c> in bytes.
				@param[out] ccbBuffer		A buffer for use by <c>m_ccb</c>.
				@param[in] ccbSizeInBytes	The size of <c><i>ccbBuffer</i></c> in bytes.
				*/
			void init(void *cueHeapAddr, uint32_t numRingEntries, ConstantUpdateEngine::RingSetup ringSetup,
					  void *dcbBuffer, uint32_t dcbSizeInBytes, void *ccbBuffer, uint32_t ccbSizeInBytes);
#endif // SCE_GNMX_ENABLE_CUE_V2

			/** @brief Initializes a GfxContext with application-provided memory buffers to enable dispatch draw.

				@note initDispatchDrawCommandBuffer must only be called after init()
				@param[out] acbBuffer A buffer for use by the Dispatch Command Buffer.
				@param[in] acbSizeInBytes The size of <c><i>acbBuffer</i></c>, in bytes.
				*/
			void initDispatchDrawCommandBuffer(void *acbBuffer, uint32_t acbSizeInBytes);

			/** @brief Sets the compute queue the GfxContext should use for dispatch draw asynchronous compute dispatches.
				@param[in,out]  pQueue   ComputeQueue to use for dispatch draw asynchronous compute dispatches.
				*/
			void setDispatchDrawComputeQueue(ComputeQueue *pQueue)
			{
				m_pQueue = pQueue;
			}
			
			/** @brief Sets up a default hardware state for the graphics ring.
				This function will wait for the GPU to be idle, and then roll the hardware context.
				Some of the render states set by this function are (in no particular order):
				<ul>
				<li><c>setVertexQuantization(kVertexQuantizationMode16_8, kVertexQuantizationRoundModeRoundToEven, kVertexQuantizationCenterAtHalf);</c></li>
				<li><c>setLineWidth(8);</c></li>
				<li><c>setPointSize(0x0008, 0x0008);</c></li>
				<li><c>setPointMinMax(0x0000, 0xFFFF);</c></li>
				<li><c>setClipControl( (ClipControl)0 );</c></li>
				<li><c>setViewportTransformControl( (ViewportTransformControl)0 );</c></li>
				<li><c>setClipRectangleRule(0xFFFF);</c></li>
				<li><c>setGuardBandClip(1.0f, 1.0f);</c></li>
				<li><c>setGuardBandDiscard(1.0f, 1.0f);</c></li>
				<li><c>setCbControl(Gnm::kCbModeNormal, Gnm::kRasterOpSrcCopy);</c></li>
				<li><c>setAaSampleMask((uint64_t)(-1LL));</c></li>
				<li><c>setNumInstances(1);</c></li>
				<li><c>setScanModeControl(kScanModeControlAaDisable, kScanModeControlViewportScissorDisable);</c></li>
				<li><c>setPsSampleIterationEnable(false);</c></li>
				<li><c>setAaSampleCount(kNumSamples1);</c></li>
				<li><c>setPolygonOffsetZFormat(kZFormat32Float);</c></li>
				<li><c>setGraphicsShaderControl(kShaderStagePs, 0xFFFF, 0, 0);</c></li>
				<li><c>setGraphicsShaderControl(kShaderStageVs, 0xFFFF, 0, 0);</c></li>
				<li><c>setGraphicsShaderControl(kShaderStageGs, 0xFFFF, 0, 0);</c></li>
				<li><c>setGraphicsShaderControl(kShaderStageEs, 0xFFFE, 0, 0);</c></li>
				<li><c>setGraphicsShaderControl(kShaderStageHs, 0x0000, 0, 0);</c></li>
				<li><c>setGraphicsShaderControl(kShaderStageLs, 0xFFFC, 0, 0);</c></li>
				</ul>
				@cmdsize 256
			*/
			void initializeDefaultHardwareState()
			{
				if (m_acb.m_beginptr != NULL)
					m_acb.initializeDefaultHardwareState();
				m_dcb.initializeDefaultHardwareState();
				m_dispatchDrawNumInstancesMinus1 = 0;	//setNumInstances(1)
				m_dispatchDrawIndexDeallocMask = 0;
			}

			/** @brief Gets the required size in bytes of a GDS area used by the GPU to track outstanding dispatchDraw() calls.

			    The GDS area consists of a kick ring buffer with elements that are 3 dwords in size, and a number of dword counters required to track index and vertex ring buffer allocations.

				@param[in] numKickRingBufferElems			The size of the GDS kick ring buffer in elements, which must be at least 2 elements in size. This determines how many dispatchDraw() calls can be simultaneously generating index data.
			*/
			uint32_t getRequiredSizeOfGdsDispatchDrawArea(uint32_t numKickRingBufferElems);

			/** @brief Configures and zeroes the dispatch draw GDS kick ring buffer and configures the index ring buffer.
				
				The kick ring buffer is currently always allocated at GDS offset 0 and is 768 dwords in size. It is followed by 8 dword counters for a total of 776 dwords.				
				
				@param[out] pIndexRingBuffer					A pointer to memory to use for the index ring buffer, which must be 256 byte aligned and should generally reside in GARLIC memory.
				@param[in] sizeofIndexRingBufferAlign256B	The size of the index ring buffer, which must be a multiple of 256 bytes.
				@param[in] numKickRingBufferElems			The size of the GDS kick ring buffer in elements, which must be at least 2 elements in size. This determines how many dispatchDraw() calls can be simultaneously generating index data.
				@param[in] gdsOffsetDispatchDrawArea		The offset of the GDS area to use for tracking dispatchDraw() calls, which must be dword aligned and at least <c>getRequiredSizeOfGdsDispatchDrawArea(numKickRingBufferElems)</c> in size.
				@param[in] gdsOaCounterForDispatchDraw		The index of a GDS ordered append unit internal counter to use for dispatch draw (range [0:15]).
			*/
			void setupDispatchDrawRingBuffers(void *pIndexRingBuffer, uint32_t sizeofIndexRingBufferAlign256B, uint32_t numKickRingBufferElems, uint32_t gdsOffsetDispatchDrawArea, uint32_t gdsOaCounterForDispatchDraw);

			/** @brief Resets the ConstantUpdateEngine, Gnm::DrawCommandBuffer, and Gnm::ConstantCommandBuffer for a new frame.
			
				Call this at the beginning of every frame.

				The Gnm::DrawCommandBuffer and Gnm::ConstantCommandBuffer will be reset to empty (<c>m_cmdptr = m_beginptr</c>)
				All shader pointers currently cached in the Constant Update Engine will be set to NULL.
			*/
			void reset(void);

#if defined(SCE_GNM_OFFLINE_MODE)
			/** @brief Computes the required size of the two output buffers generated when this structure is serialized.
				Two buffers are generated during serialization: a "temp" buffer that can be discarded after initialization and a "persistent" buffer
				that contains the actual GPU command buffers.
				@note This function is only available in offline mode.
				@param[out] outTempBufferSize The size of the "temp" buffer, in bytes.
				@param[out] outPersistentBufferSize The size of the "persistent" buffer, in bytes.
				@sa serializeIntoBuffers()
				*/
			void getSerializedSizes(size_t *outTempBufferSize, size_t *outPersistentBufferSize) const;

			/** @brief Serializes the contents of this object in a format that can be loaded and submitted at runtime by the GfxContextSubmitOnly class.
				Two buffers are generated during serialization: a "temp" buffer that can be discarded after initialization and a "persistent" buffer
				that contains the actual GPU command buffers.
				@note This function is only available in offline mode.
				@param[out] destTempBuffer The contents of the "temp" buffer will be written here. This buffer must be at least as large as <c><i>tempBufferSize</i></c>.
				@param[in] tempBufferSize The size of <c><i>destTempBuffer</i></c>, in bytes. Calculated by getSerializedSizes().
				@param[out] destPersistentBuffer The contents of the persistent buffer will be written here. This buffer must be at least as large as <c><i>persistentBufferSize</i></c>.
				@param[in] persistentBufferSize The size of the <c><i>destPersistentBuffer</i></c>, in bytes. Calculated by getSerializedSizes().
				@sa getSerializedSizes()
				*/
			void serializeIntoBuffers(void *destTempBuffer, size_t tempBufferSize, void *destPersistentBuffer, size_t persistentBufferSize) const;
#endif // defined(SCE_GNM_OFFLINE_MODE

#if !defined(SCE_GNM_OFFLINE_MODE)
			/** @brief Submits the DrawCommandBuffer and ConstantCommandBuffer.
				@note This function is not available in offline mode.
					  To enable auto-validation on submit(), please enable the "GPU Validation" option in the Target Manager Settings.
				@return A code indicating the submission status.
				*/
			int32_t submit(void);


			/** @brief Runs validation on the DrawCommandBuffer and ConstantCommandBuffer without submitting them.
				@note This function is not available in offline mode.
				@return A code indicating the validation status.
				*/
			int32_t validate(void);

			/** @brief Submits the DrawCommandBuffer and ConstantCommandBuffer and immediately requests a flip.
				@note This function is not available in offline mode.
				@param[in] videoOutHandle  Video out handle.
				@param[in] rtIndex         RenderTarget index to flip to.
				@param[in] flipMode        Flip mode.
				@param[in] flipArg         Flip argument.
				@return A code indicating the error status.
				@note To enable auto-validation on submit(), please enable the "GPU Validation" option in the Target Manager Settings.
				*/
			int32_t submitAndFlip(uint32_t videoOutHandle, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg);

			/** @brief Submits the DrawCommandBuffer and ConstantCommandBuffer and immediately requests a flip.
				
				@note This function is not available in offline mode.
				
				@param[in] videoOutHandle  Video out handle.
				@param[in] rtIndex         RenderTarget index to flip to.
				@param[in] flipMode        Flip mode.
				@param[in] flipArg         Flip argument.
				@param[out] labelAddr       GPU address to be updated when the command buffer has been processed.
				@param[in] value           Value to write to 'labelAddr'.
				
				@return A code indicating the error status.
				@note To enable auto-validation on submit(), please enable the "GPU Validation" option in the Target Manager Settings.
				*/
			int32_t submitAndFlip(uint32_t videoOutHandle, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg,
								  void *labelAddr, uint32_t value);
#endif // !defined(SCE_GNM_OFFLINE_MODE

			//////////// Constant Update Engine commands

			/** @brief Imposes a new upper bound on the number of read-only texture/buffer resources for a given shader stage.

			           Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
					   but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountResource.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveResourceSlotCount(Gnm::ShaderStage stage, uint32_t count)
			{
				return m_cue.setActiveResourceSlotCount(stage,count);
			}

			/** @brief Imposes a new upper bound on the number of read/write texture/buffer resources for a given shader stage.

			           Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountRwResource.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveRwResourceSlotCount(Gnm::ShaderStage stage, uint32_t count)
			{
				return m_cue.setActiveRwResourceSlotCount(stage,count);
			}

			/** @brief Imposes a new upper bound on the number of samplers for a given shader stage.

					   Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountSampler.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveSamplerSlotCount(Gnm::ShaderStage stage, uint32_t count)
			{
				return m_cue.setActiveSamplerSlotCount(stage,count);
			}

			/** @brief Imposes a new upper bound on the number of vertex buffers for a given shader stage.

					   Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountVertexBuffer.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveVertexBufferSlotCount(Gnm::ShaderStage stage, uint32_t count)
			{
				return m_cue.setActiveVertexBufferSlotCount(stage,count);
			}

			/** @brief Imposes a new upper bound on the number of constant buffers for a given shader stage.

				       Decreasing this value reduces the number of unique resources that can be bound simultaneously per shader stage,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] stage The shader stage whose active slot count should be modified.
				@param[in] count The new slot count for this shader stage. Must be less than or equal to Gnm::kSlotCountConstantBuffer.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveConstantBufferSlotCount(Gnm::ShaderStage stage, uint32_t count)
			{
				return m_cue.setActiveConstantBufferSlotCount(stage,count);
			}

			/** @brief Imposes a new upper bound on the number of VS streamout buffers.

			           Decreasing this value reduces the number of unique resources that can be bound simultaneously,
			           but may improve the performance of the ConstantUpdateEngine. By default, all slots are active.
				@param[in] count The new slot count for the VS shader stage. Must be less than or equal to Gnm::kSlotCountStreamoutBuffer.
				@note It is not safe to call this function while the ConstantUpdateEngine is in use. It's safest to set all active slot counts
				      at initialization time.
			*/
			void setActiveStreamoutBufferSlotCount(uint32_t count)
			{
				return m_cue.setActiveStreamoutBufferSlotCount(count);
			}


			/**
			 * @brief Binds one or more read-only texture objects to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountResource -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] textures The Gnm::Texture objects to bind to the specified slots. <c>textures[0]</c> will be bound to startSlot, <c>textures[1]</c> to <c><i>startSlot</i> + 1</c>, and so on.
			 *                 The contents of these Texture objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *                 be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 * @note Buffers and Textures share the same pool of API slots.
			 */
			void setTextures(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const Gnm::Texture *textures)
			{
				return m_cue.setTextures(stage, startSlot, numSlots, textures);
			}

			/**
			 * @brief Binds one or more read-only buffer objects to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountResource -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] buffers The Gnm::Buffer objects to bind to the specified slots. <c>buffers[0]</c> will be bound to <c><i>startSlot</i></c>, <c>buffers[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *                The contents of these Gnm::Buffer objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *                be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 * @note Gnm::Buffer and Gnm::Texture objects share the same pool of API slots.
			 */
			void setBuffers(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const Gnm::Buffer *buffers)
			{
				return m_cue.setBuffers(stage, startSlot, numSlots, buffers);
			}

			/**
			 * @brief Binds one or more read/write texture objects to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountRwResource -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] rwTextures The Gnm::Texture objects to bind to the specified slots. <c>rwTextures[0]</c> will be bound to <c><i>startSlot</i></c>, <c>rwTextures[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *                   The contents of these Texture objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *                   be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 * @note <c><i>rwBuffers</i></c> and <c><i>rwTextures</i></c> objects share the same pool of API slots.
			 * @see setRwBuffers()
			 */
			void setRwTextures(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const Gnm::Texture *rwTextures)
			{
				return m_cue.setRwTextures(stage, startSlot, numSlots, rwTextures);
			}

			/**
			 * @brief Binds one or more read/write buffer objects to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountRwResource -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] rwBuffers The Gnm::Buffer objects to bind to the specified slots. <c>rwBuffers[0]</c> will be bound to <c><i>startSlot</i></c>, <c>rwBuffers[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *                  The contents of these Buffer objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *                  be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 * @note <c><i>rwBuffers</i></c> and <c><i>rwTextures</i></c> objects share the same pool of API slots.
			 * @see setRwTextures()
			 */
			void setRwBuffers(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const Gnm::Buffer *rwBuffers)
			{
				return m_cue.setRwBuffers(stage, startSlot, numSlots, rwBuffers);
			}

			/**
			 * @brief Binds one or more sampler objects to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountSampler -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] samplers The Gnm::Sampler objects to bind to the specified slots. <c>samplers[0]</c> will be bound to <c><i>startSlot</i></c>, <c>samplers[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *                 The contents of these Sampler objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *                 be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 */
			void setSamplers(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const Gnm::Sampler *samplers)
			{
				return m_cue.setSamplers(stage, startSlot, numSlots, samplers);
			}

			/**
			 * @brief Binds one or more constant buffer objects to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountConstantBuffer -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] buffers The constant buffer objects to bind to the specified slots. <c>buffers[0]</c> will be bound to <c><i>startSlot</i></c>, <c>buffers[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *                The contents of these Gnm::Buffer objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *                be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 */
			void setConstantBuffers(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const Gnm::Buffer *buffers)
			{
				return m_cue.setConstantBuffers(stage, startSlot, numSlots, buffers);
			}

			/**
			 * @brief Binds one or more vertex buffer objects to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountVertexBuffer -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] buffers The vertex buffer objects to bind to the specified slots. <c>buffers[0]</c> will be bound to <c><i>startSlot</i></c>, <c>buffers[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *                The contents of these Gnm::Buffer objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *                be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 */
			void setVertexBuffers(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const Gnm::Buffer *buffers)
			{
				return m_cue.setVertexBuffers(stage, startSlot, numSlots, buffers);
			}

            /**
			 * @brief Binds one or more streamout buffer objects to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountStreamoutBuffer -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] buffers The streamout buffer objects to bind to the specified slots. <c>buffers[0]</c> will be bound to <c><i>startSlot</i></c>, <c>buffers[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *                The contents of these Gnm::Buffer objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *                be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 */
			void setStreamoutBuffers(uint32_t startSlot, uint32_t numSlots, const Gnm::Buffer *buffers)
			{
				return m_cue.setStreamoutBuffers(startSlot, numSlots, buffers);
			}

			/**
			 * @brief Binds one or more Boolean constants to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountBoolConstant -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] bits The Boolean constants to bind to the specified slots. Each slot will contain 32 1-bit bools packed together in a singled dword.
			 *             <c>bits[0]</c> will be bound to <c><i>startSlot</i></c>, <c>bits[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *             The contents of the bits array are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *             be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 */
			void setBoolConstants(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const uint32_t *bits)
			{
				return m_cue.setBoolConstants(stage, startSlot, numSlots, bits);
			}

			/**
			 * @brief Binds one or more floating-point constants to the specified shader stage.
			 * This function never rolls the hardware context.
			 * @param[in] stage The resource(s) will be bound to this shader stage.
			 * @param[in] startSlot The first slot to bind to. Valid slots are <c>[0..Gnm::kSlotCountResource -1]</c>.
			 * @param[in] numSlots The number of consecutive slots to bind.
			 * @param[in] floats The constants to bind to the specified slots. <c>floats[0]</c> will be bound to <c><i>startSlot</i></c>, <c>floats[1]</c> to <c><i>startSlot</i> +1</c>, and so on.
			 *               The contents of these Gnm::Texture objects are cached locally inside the ConstantUpdateEngine. If this parameter is NULL, the specified slots will
			 *               be unbound; this is not necessary for regular operation, but may be useful for debugging purposes.
			 */
			void setFloatConstants(Gnm::ShaderStage stage, uint32_t startSlot, uint32_t numSlots, const float *floats)
			{
				return m_cue.setFloatConstants(stage, startSlot, numSlots, floats);
			}


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
			 * @param[in] baseOffsetInBytes The byte offset to the start of the counters in GDS. Must be a multiple of 4. 
			 * @param[in] rangeSizeInBytes The size of the counter range, in bytes. Must be a multiple of 4.
			 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
			 */
			void setAppendConsumeCounterRange(Gnm::ShaderStage stage, uint32_t baseOffsetInBytes, uint32_t rangeSizeInBytes)
			{
				return m_cue.setAppendConsumeCounterRange(stage, baseOffsetInBytes, rangeSizeInBytes);
			}

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
			void setGdsMemoryRange(Gnm::ShaderStage stage, uint32_t rangeGdsOffsetInBytes, uint32_t rangeSizeInBytes)
			{
				return m_cue.setGdsMemoryRange(stage, rangeGdsOffsetInBytes, rangeSizeInBytes);
			}

			/**
			 * @brief Specifies a buffer to hold tessellation constants.
			 * This function never rolls the hardware context.
			 * @param[in] tcbAddr		The address of the buffer.
			 * @param[in] domainStage	The stage in which the domain shader will execute.
			 */
			void setTessellationDataConstantBuffer(void *tcbAddr, Gnm::ShaderStage domainStage);

			/**
			 * @brief Specifies a buffer to receive off-chip tessellation factors.
			 * This function never rolls the hardware context.
			 * @note This function modifies the global resource table. It is not safe to modify the global resource table unless the GPU graphics pipe is idle.
			 * @param[out] tessFactorMemoryBaseAddr	The address of the buffer. This address must already have been mapped correctly. Please refer to <c>Gnmx::Toolkit::allocateTessellationFactorRingBuffer()</c>.
			 */
			void setTessellationFactorBuffer(void *tessFactorMemoryBaseAddr);
			
			/**	@brief Sets the active shader stages in the graphics pipeline. 
				Note that the compute-only CS stage is always active for dispatch commands.
				This function will roll the hardware context.
				@param[in] activeStages Indicates which shader stages should be activated.
				@see Gnm::DrawCommandBuffer::setGsMode()
				@cmdsize 3
			 */
			void setActiveShaderStages(Gnm::ActiveShaderStages activeStages)
			{
				m_cue.setActiveShaderStages(activeStages);
				return m_dcb.setActiveShaderStages(activeStages);
			}

			/** @brief Sets the index size (16 or 32 bits). 
				All future draw calls that reference index buffers will use this index size to interpret their contents.
				This function will roll the hardware context.
				@param[in] indexSize Index size to set.
				@note This setting is not used by dispatchDraw(), which overrides the setting to Gnm::kIndexSize16ForDispatchDraw.  The last set value is restored by the first draw command after a dispatchDraw() call .
				@cmdsize 2
			*/
			void setIndexSize(Gnm::IndexSize indexSize)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "setIndexSize can't be called between beginDispatchDraw and endDispatchDraw");
				SCE_GNM_VALIDATE((indexSize & kIndexSizeFlagDispatchDraw) == 0, "GfxContext::setIndexSize should only be called to set non-dispatchDraw IndexSize values");
				return m_dcb.setIndexSize(indexSize);
			}

			/** @brief Sets the base address where the indices are located for functions that do not specify their own indices.
				This function never rolls the hardware context.
				@param[in] indexAddr Address of the index buffer. Must be 2-byte aligned.
				@note This setting is not used by dispatchDraw, which overrides the setting to point to the index ring buffer passed to setupDispatchDrawRingBuffers.  The last set value is restored by the first draw command after a dispatchDraw.
				@see drawIndexIndirect(), drawIndexOffset()
				@cmdsize 3
			*/
			void setIndexBuffer(const void * indexAddr)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "setIndexBuffer can't be called between beginDispatchDraw and endDispatchDraw");
				return m_dcb.setIndexBuffer(indexAddr);
			}

			/** @brief Specifies information for multiple vertex geometry tessellator (VGT) configurations.
			This function will roll the hardware context.
			@param[in] primGroupSize	Number of primitives sent to one VGT block before switching to the next block. It has an implied +1 value. That is, 0 = 1 primitive/group, and 255 = 256 primitives/group.
			                        		For tessellation, <c><i>primGroupSize</i></c> should be set to the number of patches per thread group minus 1.
			@param[in] partialVsWaveMode	If enabled, then the VGT will issue a VS-stage wavefront as soon as a primitive group is finished. Otherwise the VGT will continue a VS-stage wavefront from one primitive group to next
			                         				primitive group within a draw call. This must be enabled for streamout.
			@param[in] switchOnEopMode	If enabled, the IA will switch between VGTs at packet boundaries. If disabled, it will switch on <c><i>primGroupSize</i></c>.
			@note This setting is not used by dispatchDraw(), which overrides the setting to <c>(29, kVgtPartialVsWaveEnable, kVgtSwitchOnEopEnable)</c>.  The last set value is restored by the first draw command after a dispatchDraw() call.
			@cmdsize 3
			*/
			void setVgtControl(uint8_t primGroupSize, Gnm::VgtPartialVsWaveMode partialVsWaveMode, Gnm::VgtSwitchOnEopMode switchOnEopMode)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "setVgtControl can't be called between beginDispatchDraw and endDispatchDraw");
				return m_dcb.setVgtControl(primGroupSize, partialVsWaveMode, switchOnEopMode);
			}

			/**
			 * @brief Binds a shader to the VS stage.
			 * This function will roll hardware context if any of the following Gnm::VsStageRegisters set for <c><i>vsb</i></c> are different from current state:
			 - <c>m_spiVsOutConfig</c>
			 - <c>m_spiShaderPosFormat</c>
			 - <c>m_paClVsOutCntl</c>

			 The check is deferred until next draw call.
			 * @param[in] vsb Pointer to the shader to bind to the VS stage.
			 * @param[in] shaderModifier If the shader requires a fetch shader, pass the associated shader modifier value here. Otherwise, pass 0.
			 * @param[in] fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>vsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 */
			void setVsShader(const Gnmx::VsShader *vsb, uint32_t shaderModifier, void *fetchShaderAddr)
			{
				return m_cue.setVsShader(vsb, shaderModifier, fetchShaderAddr);
			}


			/**
			 * @brief Binds a shader pair to the asynchronous compute and VS stages for use in a dispatchDraw() call.
			 * This function will roll hardware context if any of the following Gnm::VsStageRegisters set for <c><i>csvsb</i></c> are different from current state:
			 *
			 * - <c>m_spiVsOutConfig</c>
			 * - <c>m_spiShaderPosFormat</c>
			 * - <c>m_paClVsOutCntl</c>
			 *
			 * The check is deferred until next draw call.
			 * @param[in] csvsb Pointer to the shaders to bind to the asynchronous compute and VS stages.
			 * @param[in] shaderModifierVs If the VS shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param[in] fetchShaderAddrVs If the VS shader requires a fetch shader, pass its GPU address here; otherwise pass 0.
			 * @param[in] shaderModifierCs If the compute shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param[in] fetchShaderAddrCs If the compute shader requires a fetch shader, pass its GPU address here; otherwise pass 0.
			 * @note  Setting the asynchronous compute shader does not affect the currently set graphics CS stage shader, which is set by setCsShader().
			 *		  Asynchronous compute shaders share the user data constants and CUE ring elements set up for Gnm::kShaderStageVs,
			 *		  while graphics CS stage shaders use the user data constants and CUE ring elements set up for Gnm::kShaderStageCs.
			 * @note  Only the pointers <c>csvsb->getVertexShader()</c> and <c>csvsb->getComputeShader()</c> are cached inside the Gnm::ConstantUpdateEngine; the location 
			 *		  and contents of these pointers should not be changed before all calls to dispatchDraw() which use this shader have been made!
			 */
			void setCsVsShaders(const Gnmx::CsVsShader *csvsb, uint32_t shaderModifierVs, void *fetchShaderAddrVs, uint32_t shaderModifierCs, void *fetchShaderAddrCs)
			{
				return m_cue.setCsVsShaders(csvsb, shaderModifierVs, fetchShaderAddrVs, shaderModifierCs, fetchShaderAddrCs);
			}

			/**
			 * @brief Binds a shader to the asynchronous compute stage, for use in a dispatchDraw call.
			 * Generally, compute shaders for dispatch draw are packed into a CsVsShader, and so will be set by calling setCsVsShaders(), instead.
			 * This function never rolls the hardware context.
			 * @param[in] csb Pointer to the shader to bind to the asynchronous compute stage.
			 * @param[in] shaderModifierCs If the compute shader requires a fetch shader, pass the associated shader modifier value here; otherwise pass 0.
			 * @param[in] fetchShaderAddrCs If the compute shader requires a fetch shader, pass its GPU address here; otherwise pass 0.
			 * @note  Setting the asynchronous compute shader does not affect the currently set graphics CS stage shader, which is set by setCsShader().
			 *		  Asynchronous compute shaders share the user data constants and CUE ring elements set up for Gnm::kShaderStageVs,
			 *		  while graphics CS stage shaders use the user data constants and CUE ring elements set up for Gnm::kShaderStageCs.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c><i>*csb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 */
			void setAsynchronousComputeShader(const Gnmx::CsShader *csb, uint32_t shaderModifierCs, void *fetchShaderAddrCs)
			{
				return m_cue.setAsynchronousComputeShader(csb, shaderModifierCs, fetchShaderAddrCs);
			}

			/**
			 * @brief Binds a shader to the PS stage.
			 * This function will roll hardware context if any of the following Gnm::PsStageRegisters set for the shader specified by <c><i>psb</i></c> are different from current state:
			 - <c>m_spiShaderZFormat</c>
			 - <c>m_spiShaderColFormat</c>
			 - <c>m_spiPsInputEna</c>
			 - <c>m_spiPsInputAddr</c>
			 - <c>m_spiPsInControl</c>
			 - <c>m_spiBarycCntl</c>
			 - <c>m_dbShaderControl</c>
			 - <c>m_cbShaderMask</c>

			 The check is deferred until next draw call.
			 * @param[in] psb Pointer to the shader to bind to the PS stage.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>psb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 */
			void setPsShader(const Gnmx::PsShader *psb)
			{
				return m_cue.setPsShader(psb);
			}

			/**
			 * @brief Binds a shader to the CS stage.
			 * This function never rolls the hardware context.
			 * @param[in] csb Pointer to the shader to bind to the CS stage.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>csb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 */
			void setCsShader(const Gnmx::CsShader *csb)
			{
				return m_cue.setCsShader(csb);
			}

			/**
			 * @brief Binds a shader to the ES stage.
			 * This function never rolls the hardware context.
			 * @param[in] esb Pointer to the shader to bind to the ES stage.
			 * @param[in] shaderModifier If the shader requires a fetch shader, pass the associated shader modifier value here. Otherwise, pass 0.
			 * @param[in] fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>esb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 */
			void setEsShader(const Gnmx::EsShader *esb, uint32_t shaderModifier, void *fetchShaderAddr)
			{
				return m_cue.setEsShader(esb, shaderModifier, fetchShaderAddr);
			}

			/** @brief Writes the specified 64-bit value to the given location in memory when this command reaches the end of the processing pipe (EOP).

			This function never rolls the hardware context.
			@param[in] eventType   Determines when <c><i>immValue</i></c> will be written to the specified address.
			@param[out] dstGpuAddr     GPU address to which the given value will be written. This address must be 8-byte aligned and must not be set to NULL.
			@param[in] immValue       Value that will be written to <c><i>dstGpuAddr</i></c>.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@sa Gnm::DrawCommandBuffer::writeAtEndOfPipe()
			@cmdsize 6
			*/
			void writeImmediateAtEndOfPipe(Gnm::EndOfPipeEventType eventType, void *dstGpuAddr, uint64_t immValue, Gnm::CacheAction cacheAction)
			{
				return m_dcb.writeAtEndOfPipe(eventType,
												 Gnm::kEventWriteDestMemory, dstGpuAddr,
												 Gnm::kEventWriteSource64BitsImmediate, immValue,
												 cacheAction, Gnm::kCachePolicyLru);
			}

			/** @brief Writes the specified 32-bit value to the given location in memory when this command reaches the end of the processing pipe (EOP).

			This function never rolls the hardware context.
			@param[in] eventType   Determines when <c><i>immValue</i></c> will be written to the specified address.
			@param[out] dstGpuAddr     GPU address to which the given value will be written. This address must be 8-byte aligned and must not be set to NULL.
			@param[in] immValue       Value that will be written to <c><i>dstGpuAddr</i></c>.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@sa Gnm::DrawCommandBuffer::writeAtEndOfPipe()
			@cmdsize 6
			*/
			void writeImmediateDwordAtEndOfPipe(Gnm::EndOfPipeEventType eventType, void *dstGpuAddr, uint64_t immValue, Gnm::CacheAction cacheAction)
			{
				return m_dcb.writeAtEndOfPipe(eventType,
												 Gnm::kEventWriteDestMemory, dstGpuAddr,
												 Gnm::kEventWriteSource32BitsImmediate, immValue,
												 cacheAction, Gnm::kCachePolicyLru);
			}

			/** @brief Writes the GPU core clock counter to the given location in memory when this command reaches the end of the processing pipe (EOP).

			This function never rolls the hardware context.
			@param[in] eventType   Determines when the counter value will be written to the specified address.
			@param[out] dstGpuAddr     GPU address to which the counter value will be written. This address must be 8-byte aligned and must not be set to NULL.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@sa Gnm::DrawCommandBuffer::writeAtEndOfPipe()
			@cmdsize 6
			*/
			void writeTimestampAtEndOfPipe(Gnm::EndOfPipeEventType eventType, void *dstGpuAddr, Gnm::CacheAction cacheAction)
			{
				return m_dcb.writeAtEndOfPipe(eventType,
											  Gnm::kEventWriteDestMemory, dstGpuAddr,
											  Gnm::kEventWriteSourceGpuCoreClockCounter, 0,
											  cacheAction, Gnm::kCachePolicyLru);
			}

			/** @brief Writes the specified 64-bit value to the given location in memory and triggers an interrupt when this command reaches the end of the processing pipe (EOP).

			This function never rolls the hardware context.
			@param[in] eventType   Determines when <c><i>immValue</i></c> will be written to the specified address.
			@param[out] dstGpuAddr     GPU address to which the given value will be written. This address must be 8-byte aligned and must not be set to NULL.
			@param[in] immValue       Value that will be written to <c><i>dstGpuAddr</i></c>.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@sa Gnm::DrawCommandBuffer::writeAtEndOfPipeWithInterrupt()
			@cmdsize 6
			*/
			void writeImmediateAtEndOfPipeWithInterrupt(Gnm::EndOfPipeEventType eventType, void *dstGpuAddr, uint64_t immValue, Gnm::CacheAction cacheAction)
			{
				return m_dcb.writeAtEndOfPipeWithInterrupt(eventType,
															  Gnm::kEventWriteDestMemory, dstGpuAddr,
															  Gnm::kEventWriteSource64BitsImmediate, immValue,
															  cacheAction, Gnm::kCachePolicyLru);
			}

			/** @brief Writes the GPU core clock counter to the given location in memory and triggers an interrupt when this command reaches the end of the processing pipe (EOP).

			This function never rolls the hardware context.
			@param[in] eventType   Determines when the counter value will be written to the specified address.
			@param[out] dstGpuAddr     GPU address to which the counter value will be written. This address must be 8-byte aligned and must not be set to NULL.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@sa Gnm::DrawCommandBuffer::writeAtEndOfPipeWithInterrupt()
			@cmdsize 6
			*/
			void writeTimestampAtEndOfPipeWithInterrupt(Gnm::EndOfPipeEventType eventType, void *dstGpuAddr, Gnm::CacheAction cacheAction)
			{
				return m_dcb.writeAtEndOfPipeWithInterrupt(eventType,
															  Gnm::kEventWriteDestMemory, dstGpuAddr,
															  Gnm::kEventWriteSourceGpuCoreClockCounter, 0,
															  cacheAction, Gnm::kCachePolicyLru);
			}

			/**
			 * @brief Binds shaders to the LS and HS stages, and updates the LS shader's LDS size based on the HS shader's needs.
			 * This function will roll hardware context if any of the Gnm::HsStageRegisters or Gnm::HullStateConstants set in <c><i>hsb</i></c> or the value of <c><i>numPatches</i></c> are different from current state:

			 Gnm::HsStageRegisters
			 - <c>m_vgtTfParam</c>
			 - <c>m_vgtHosMaxTessLevel</c>
			 - <c>m_vgtHosMinTessLevel</c>

			 Gnm::HullStateConstants
			 - <c>m_numThreads</c>
			 - <c>m_numInputCP</c>

			 The check is deferred until next draw call.
			 * @param[in] lsb Pointer to the shader to bind to the LS stage. This must not be set to NULL if <c><i>hsb</i></c> is non-NULL.
			 * @param[in] shaderModifier If the shader requires a fetch shader, pass the associated shader modifier value here. Otherwise, pass 0.
			 * @param[in] fetchShaderAddr If the LS shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @param[in] hsb Pointer to the shader to bind to the HS stage. This must not be set to NULL if <c><i>lsb</i></c> is non-NULL.
			 * @param[in] numPatches Number of patches in the HS shader.
			 * @note Only the shader pointers are cached inside the ConstantUpdateEngine; the location and contents of <c><i>lsb</i></c> and <c><i>hsb</i></c> must not change
			 *       until different shaders are bound to these stages!
			 */
			void setLsHsShaders(Gnmx::LsShader *lsb, uint32_t shaderModifier, void *fetchShaderAddr, const Gnmx::HsShader *hsb, uint32_t numPatches);


			/**
			 * @brief Binds a shader to the GS and VS stages.
			 * This function will roll hardware context if any of Gnm::GsStageRegisters entries set for the GsShader specified in <c><i>gsb</i></c>
			 or the Gnm::VsStageRegisters entries set for the copy shader specified in by GsShader::getCopyShader() call to the shader specified in <c><i>gsb</i></c> are different from current state:

			 Gnm::GsStageRegisters
			 - <c>m_vgtStrmoutConfig</c>
			 - <c>m_vgtGsOutPrimType</c>
			 - <c>m_vgtGsInstanceCnt</c>

			 Gnm::VsStageRegisters
			 - <c>m_spiVsOutConfig</c>
			 - <c>m_spiShaderPosFormat</c>
			 - <c>m_paClVsOutCntl</c>

			 The check is deferred until next draw call.
			 * @param[in] gsb Pointer to the shader to bind to the GS/VS stages.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>gsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 */
			void setGsVsShaders(const Gnmx::GsShader *gsb);

			/**
			 * @brief Binds an on-chip-GS vertex shader to the ES stage.
			 * This function never rolls the hardware context.
			 * @param[in] esb Pointer to the shader to bind to the ES stage.
			 * @param[in] ldsSizeIn512Bytes Pass the size of the LDS allocation required for on-chip GS here.  The size unit is 512 bytes and must be compatible with the GsShader and gsPrimsPerSubGroup value passed to setOnChipGsVsShaders.
			 * @param[in] shaderModifier If the shader requires a fetch shader, pass the associated shader modifier value here. Otherwise, pass 0.
			 * @param[in] fetchShaderAddr If the shader requires a fetch shader, pass its GPU address here. Otherwise, pass 0.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>esb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @see computeOnChipGsConfiguration
			 */
			void setOnChipEsShader(const Gnmx::EsShader *esb, uint32_t ldsSizeIn512Bytes, uint32_t shaderModifier, void *fetchShaderAddr)
			{
				m_cue.setOnChipEsShader(esb, ldsSizeIn512Bytes, shaderModifier, fetchShaderAddr);
			}

			/**
			 * @brief Binds an on-chip GS shader to the GS and VS stages and sets up the on-chip GS sub-group size controls.
			 * This function will roll hardware context if any of Gnm::GsStageRegisters entries set for the GsShader specified in <c><i>gsb</i></c>
			 or the Gnm::VsStageRegisters entries set for the copy shader specified in by GsShader::getCopyShader() call to the shader specified in 
			 <c><i>gsb</i></c> are different from current state:

			 Gnm::GsStageRegisters
			 - <c>m_vgtStrmoutConfig</c>
			 - <c>m_vgtGsOutPrimType</c>
			 - <c>m_vgtGsInstanceCnt</c>

			 Gnm::VsStageRegisters
			 - <c>m_spiVsOutConfig</c>
			 - <c>m_spiShaderPosFormat</c>
			 - <c>m_paClVsOutCntl</c>

			 * This function will also roll context if the value of gsPrimsPerSubGroup or gsb->m_inputVertexCount changes.

			 The check is deferred until next draw call.
			 * @param[in] gsb Pointer to the shader to bind to the GS/VS stages.
			 * @param[in] gsPrimsPerSubGroup The number of GS threads which will be launched per on-chip-GS LDS allocation, which must be compatible with the size of the LDS allocation passed to setOnChipEsShaders.
			 * @note  Only the pointer is cached inside the ConstantUpdateEngine; the location and contents of <c>*<i>gsb</i></c> must not change until
			 *        a different shader has been bound to this stage!
			 * @see computeOnChipGsConfiguration
			 */
			void setOnChipGsVsShaders(const Gnmx::GsShader *gsb, uint32_t gsPrimsPerSubGroup);

			/**
			 * @brief Sets the ring buffer where data will flow from the ES to the GS stages when geometry shading is enabled.
			 * This function will roll hardware context.
			 * @note This function modifies the global resource table. It is not safe to modify the global resource table unless the GPU graphics pipe is idle.
			 * @note This function overwrites the register state set by setOnChipEsGsLayout and vice versa.
			 * @param[out] baseAddr						The address of the buffer.
			 * @param[in] ringSize						The size of the buffer.
			 * @param[in] maxExportVertexSizeInDword	The maximum size of a vertex export in dwords.
			 */
			void setEsGsRingBuffer(void *baseAddr, uint32_t ringSize, uint32_t maxExportVertexSizeInDword);

			/**
			 * @brief Sets the layout of the LDS area where data will flow from the ES to the GS stages when on-chip geometry shading is enabled.
			 * This sets the same context register state as setEsGsRingBuffer(NULL, 0, maxExportVertexSizeInDword), but does not modify the global resource table.
			 * Due to LDS bank conflicts at a stride of 32 dwords, it is possible that setting this value sli
			 *
			 * This function will roll hardware context.
			 * @param[in] maxExportVertexSizeInDword	The stride of an ES-GS vertex in dwords, which must match EsShader::m_maxExportVertexSizeInDword.
			 */
			void setOnChipEsGsLdsLayout(uint32_t maxExportVertexSizeInDword);

			/**
			 * @brief Sets the ring buffers where data will flow from the GS to the VS stages when geometry shading is enabled.
			 * This function will roll hardware context.
			 * @note This function modifies the global resource table. It is not safe to modify the global resource table unless the GPU graphics pipe is idle.
			 * @param[out] baseAddr						The address of the buffer. This address must not be set to NULL if <c><i>ringSize</i></c> is non-zero.
			 * @param[in] ringSize						The size of the buffer.
			 * @param[in] vtxSizePerStreamInDword		The vertex size for each of four streams in dwords. This pointer must not be set to NULL.
			 * @param[in] maxOutputVtxCount				The maximum number of vertices output from the GS stage.
			 */
			void setGsVsRingBuffers(void *baseAddr, uint32_t ringSize,
									const uint32_t vtxSizePerStreamInDword[4], uint32_t maxOutputVtxCount);

			/**
			 * @brief Sets the layout of the LDS area where data will flow from the GS to the VS stages when on-chip geometry shading is enabled.
			 * This sets the same context register state as setGsVsRingBuffers(NULL, 0, vtxSizePerStreamInDword, maxOutputVtxCount), but does not modify the global resource table.
			 *
			 * This function will roll hardware context.
			 * @param[in] vtxSizePerStreamInDword		The stride of GS-VS vertices for each of four streams in dwords, which must match GsShader::m_memExportVertexSizeInDWord[]. This pointer must not be NULL.
			 * @param[in] maxOutputVtxCount				The maximum number of vertices output from the GS stage, which must match GsShader::m_maxOutputVertexCount.
			 */
			void setOnChipGsVsLdsLayout(const uint32_t vtxSizePerStreamInDword[4], uint32_t maxOutputVtxCount);

			/**
			 * @brief Sets the address of the system's global resource table: a collection of <c>V#</c>s which point to global buffers for various shader tasks.
			 * This function never rolls the hardware context.
			 * @param[out] addr The GPU-visible address of the global resource table. The minimum size of this buffer is given by
			 *             Gnm::SCE_GNM_SHADER_GLOBAL_TABLE_SIZE, and its minimum alignment is Gnm::kMinimumBufferAlignmentInBytes.
			 */
			void setGlobalResourceTableAddr(void *addr)
			{
				return m_cue.setGlobalResourceTableAddr(addr);
			}

			/**
			 * @brief Sets an entry in the global resource table.
			 * This function never rolls the hardware context.
			 * @note This function modifies the global resource table. It is not safe to modify the global resource table unless the GPU is idle.
			 * @param[in] resType The global resource type to bind a buffer for. Each global resource type has its own entry in the global resource table.
			 * @param[in] res The buffer to bind to the specified entry in the global resource table. The size of the buffer is global-resource-type-specific.
			 */
			void setGlobalDescriptor(Gnm::ShaderGlobalResourceType resType, const Gnm::Buffer *res)
			{
				return m_cue.setGlobalDescriptor(resType, res);
			}


			/** @brief Turns off the Gs Mode.
			          Will roll hardware context if different from current state. (?)
			   @note  This function has to be called prior moving back to a non-GS pipeline in addition to setShaderStages. */
			void setGsModeOff()
			{
				return m_dcb.setGsMode(Gnm::kGsModeDisable, Gnm::kGsMaxOutputVertexCount1024);
			}

			//////////// Draw commands

#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
			/** @brief Begins the instrumentation process for a draw or dispatch.

				This function calculates the byte offset of a subsequent draw or dispatch, so you can instrument it later.
				@return The byte offset from the beginning of the draw command buffer to the draw or dispatch to be instrumented.
			*/
			uint32_t beginRecordLastCompletion() const
			{
				return static_cast<uint32_t>( (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t) );
			}

			/** @brief Ends the instrumentation process for a draw or dispatch.
				
				       This function adds commands to write the byte offset of a draw or dispatch in the draw command buffer to a space near the start of the draw command buffer.
				       This function may stall the GPU until this value is written.
				@param[in] offset The byte offset from the beginning of the draw command buffer to the draw or dispatch to be instrumented.
			*/
			void endRecordLastCompletion(uint32_t offset)
			{
				if(m_recordLastCompletionMode != kRecordLastCompletionDisabled)
				{
					writeImmediateDwordAtEndOfPipe(Gnm::kEopFlushCbDbCaches, m_addressOfOffsetOfLastCompletion, offset, Gnm::kCacheActionNone);
					if(m_recordLastCompletionMode == kRecordLastCompletionSynchronous)
					{
						waitOnAddress(m_addressOfOffsetOfLastCompletion, 0xFFFFFFFF, Gnm::kWaitCompareFuncEqual, offset);
					}
				}
			}
#endif

			/** @brief Inserts a draw call using auto generated indices.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.

				@param[in] indexCount  The index up to which to auto generate the indices.
				
				@see Gnm::DrawCommandBuffer::setPrimitiveType()
			  */
			void drawIndexAuto(uint32_t indexCount)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndexAuto can't be called between beginDispatchDraw and endDispatchDraw");
				SCE_GNM_VALIDATE(!m_cue.isVertexOrInstanceOffsetEnabled(), "Using a shader that is expecting a vertex and/or instance offset without specifing them");
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawIndexAuto(indexCount);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}
			
			/** @brief Inserts a draw call using auto generated indices while adding an offset to the vertex and instance indexes.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.

				@param indexCount		The index up to which to auto generate the indices.
				@param vertexOffset		The offset added to each vertex index.
				@param instanceOffset	The offset added to each instance index.
				
				@see Gnm::DrawCommandBuffer::setPrimitiveType()
			  */
			void drawIndexAuto(uint32_t indexCount, uint32_t vertexOffset, uint32_t instanceOffset)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndexAuto can't be called between beginDispatchDraw and endDispatchDraw");
				m_cue.setVertexAndInstanceOffset(&m_dcb, vertexOffset,instanceOffset);
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawIndexAuto(indexCount);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			
			/** @brief Inserts a draw call using streamout output.

				Draw commands never roll the hardware context, but use the current context such that the next command that sets context state will roll context.

				@see Gnm::DrawCommandBuffer::setPrimitiveType()
			  */
			void drawOpaque()
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawOpaque can't be called between beginDispatchDraw and endDispatchDraw");
				SCE_GNM_VALIDATE(!m_cue.isVertexOrInstanceOffsetEnabled(), "Using a shader that is expecting a vertex and/or instance offset without specifing them");
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawOpaqueAuto();
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}
			
			/** @brief Inserts a draw call using streamout output while adding an offset to the vertex and instance indexes.

				Draw commands never roll the hardware context, but use the current context such that the next command that sets context state will roll context.

				@param vertexOffset		The offset added to each vertex index.
				@param instanceOffset	The offset added to each instance index.
				
				@see Gnm::DrawCommandBuffer::setPrimitiveType()
			  */
			void drawOpaque(uint32_t vertexOffset, uint32_t instanceOffset)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawOpaque can't be called between beginDispatchDraw and endDispatchDraw");
				m_cue.setVertexAndInstanceOffset(&m_dcb, vertexOffset,instanceOffset);
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawOpaqueAuto();
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Inserts a draw call using provided indices, which are inserted into the command buffer.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.

				@param[in] indexCount			The number of indices to insert.
				@param[in] indices				A pointer to the first index in the buffer containing <c><i>indexCount</i></c> indices. The pointer should be 4-byte aligned.
				@param[in] indicesSizeInBytes	The size of the buffer pointed to by <c><i>indices</i></c> in bytes. To specify the size of individual indices, use setIndexSize().
			  
				@see Gnm::DrawCommandBuffer::setPrimitiveType(), Gnm::DrawCommandBuffer::setIndexSize()
			  */
			void drawIndexInline(uint32_t indexCount, const void *indices, uint32_t indicesSizeInBytes)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndexInline can't be called between beginDispatchDraw and endDispatchDraw");
				SCE_GNM_VALIDATE(!m_cue.isVertexOrInstanceOffsetEnabled(), "Using a shader that is expecting a vertex and/or instance offset without specifing them");
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawIndexInline(indexCount, indices, indicesSizeInBytes);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Inserts a draw call using provided indices, which are inserted into the command buffer, while adding an offset to the vertex and instance indexes.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.
			  
				@param indexCount			The number of indices to insert.
				@param indices				A pointer to the first index in the buffer containing <c><i>indexCount</i></c> indices. The pointer should be 4-byte aligned.
				@param indicesSizeInBytes	The size of the buffer pointed to by <c><i>indices</i></c> in bytes. To specify the size of individual indices, use setIndexSize().
				@param vertexOffset			The offset added to each vertex index.
				@param instanceOffset		The offset added to each instance index.
				
				@see Gnm::DrawCommandBuffer::setPrimitiveType(), Gnm::DrawCommandBuffer::setIndexSize()
			  */
			void drawIndexInline(uint32_t indexCount, const void *indices, uint32_t indicesSizeInBytes, uint32_t vertexOffset, uint32_t instanceOffset)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndexInline can't be called between beginDispatchDraw and endDispatchDraw");
				m_cue.setVertexAndInstanceOffset(&m_dcb, vertexOffset,instanceOffset);
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawIndexInline(indexCount, indices, indicesSizeInBytes);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Inserts a draw call using indices which are located in memory.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.
			  
				@param[in] indexCount			The number of indices to insert.
				@param[in] indexAddr			The GPU address of the index buffer.
				
				@see Gnm::DrawCommandBuffer::setPrimitiveType(), Gnm::DrawCommandBuffer::setIndexSize()
			  */
			void drawIndex(uint32_t indexCount, const void *indexAddr)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndex can't be called between beginDispatchDraw and endDispatchDraw");
				SCE_GNM_VALIDATE(!m_cue.isVertexOrInstanceOffsetEnabled(), "Using a shader that is expecting a vertex and/or instance offset without specifing them");
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawIndex(indexCount, indexAddr);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Inserts a draw call, using indices which are located in memory, while adding an offset to the vertex and instance indexes.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.
			  
				@param indexCount			The number of indices to insert.
				@param indexAddr			The GPU address of the index buffer.
				@param vertexOffset			The offset added to each vertex index.
				@param instanceOffset		The offset added to each instance index.
				
				@see Gnm::DrawCommandBuffer::setPrimitiveType(), Gnm::DrawCommandBuffer::setIndexSize()
			  */
			void drawIndex(uint32_t indexCount, const void *indexAddr, uint32_t vertexOffset, uint32_t instanceOffset)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndex can't be called between beginDispatchDraw and endDispatchDraw");
				m_cue.setVertexAndInstanceOffset(&m_dcb, vertexOffset,instanceOffset);
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawIndex(indexCount, indexAddr);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Issues an indirect draw call, which reads its parameters from a specified address in GPU memory.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.
				
				@param[in] dataOffsetInBytes		The offset (in bytes) into the indirect arguments buffer at which to read the parameters from.
													The data at this offset should be a Gnm::DrawIndirectArgs structure.
				
				@note The buffer containing the indirect arguments should already have been set using setBaseIndirectArgs().
				
				@see Gnm::DrawIndirectArgs, setBaseIndirectArgs()
			*/
			void drawIndirect(uint32_t dataOffsetInBytes)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndirect can't be called between beginDispatchDraw and endDispatchDraw");
				// no need to set vertex and instance offsets here, they are coming from the draw indirect structure.
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				switch( m_cue.getActiveShaderStages())
				{
				case Gnm::kActiveShaderStagesVsPs:
					{
						const Gnmx::VsShader *pvs = m_cue.m_currentVSB;
						m_dcb.drawIndirect(dataOffsetInBytes,Gnm::kShaderStageVs,(uint8_t)pvs->getVertexOffsetUserRegister(),(uint8_t)pvs->getInstanceOffsetUserRegister());
					}
					break;
				case Gnm::kActiveShaderStagesEsGsVsPs:
					{
						const Gnmx::EsShader *pes = m_cue.m_currentESB;
						m_dcb.drawIndirect(dataOffsetInBytes,Gnm::kShaderStageEs,(uint8_t)pes->getVertexOffsetUserRegister(),(uint8_t)pes->getInstanceOffsetUserRegister());
					}
					break;
				case Gnm::kActiveShaderStagesLsHsVsPs:
				case Gnm::kActiveShaderStagesLsHsEsGsVsPs:
					{
						const Gnmx::LsShader *pls = m_cue.m_currentLSB;
						m_dcb.drawIndirect(dataOffsetInBytes,Gnm::kShaderStageLs,(uint8_t)pls->getVertexOffsetUserRegister(),(uint8_t)pls->getInstanceOffsetUserRegister());
					}
					break;
				case Gnm::kActiveShaderStagesDispatchDrawVsPs:
					SCE_GNM_ERROR("drawIndirect does not support DispatchDaw");
				}
				
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Issues an indirect multi-draw call, which reads its parameters from a specified address in GPU memory.

			    This is similar to calling drawIndirect() several times, with multiple DrawIndirectArgs objects tightly packed in memory.
				Draw commands never roll the hardware context, but use the current context such that the next command that sets context state will roll context.
				@param dataOffsetInBytes Offset (in bytes) into the buffer that contains the first draw call's indirect arguments, set using setBaseIndirectArgs().
								         The data at this offset should be a Gnm::DrawIndirectArgs structure.
				@param drawCount The number of draw calls to launch. Each draw reads a new DrawIndirectArgs object from the indirect arguments buffer.
				@note The buffer containing the indirect arguments should already have been set using setBaseIndirectArgs().
				@see Gnm::DrawIndexIndirectArgs, setBaseIndirectArgs()
			*/
			void drawIndirectMulti(uint32_t dataOffsetInBytes, uint32_t drawCount)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndirectMulti can't be called between beginDispatchDraw and endDispatchDraw");
				// no need to set vertex and instance offsets here, they are coming from the draw indirect structure.
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				switch( m_cue.getActiveShaderStages())
				{
				case Gnm::kActiveShaderStagesVsPs:
					{
						const Gnmx::VsShader *pvs = m_cue.m_currentVSB;
						m_dcb.drawIndirectMulti(dataOffsetInBytes,drawCount,Gnm::kShaderStageVs,(uint8_t)pvs->getVertexOffsetUserRegister(),(uint8_t)pvs->getInstanceOffsetUserRegister());
					}
					break;
				case Gnm::kActiveShaderStagesEsGsVsPs:
					{
						const Gnmx::EsShader *pes = m_cue.m_currentESB;
						m_dcb.drawIndirectMulti(dataOffsetInBytes,drawCount,Gnm::kShaderStageEs,(uint8_t)pes->getVertexOffsetUserRegister(),(uint8_t)pes->getInstanceOffsetUserRegister());
					}
					break;
				case Gnm::kActiveShaderStagesLsHsVsPs:
				case Gnm::kActiveShaderStagesLsHsEsGsVsPs:
					{
						const Gnmx::LsShader *pls = m_cue.m_currentLSB;
						m_dcb.drawIndirectMulti(dataOffsetInBytes,drawCount,Gnm::kShaderStageLs,(uint8_t)pls->getVertexOffsetUserRegister(),(uint8_t)pls->getInstanceOffsetUserRegister());
					}
					break;
				case Gnm::kActiveShaderStagesDispatchDrawVsPs:
					SCE_GNM_ERROR("drawIndirectMulti does not support DispatchDaw");
				}
				
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Issues an indirect drawIndex call, which reads its parameters from a specified address in the GPU memory.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.
				
				@param[in] dataOffsetInBytes	The offset (in bytes) into the buffer that contains the indirect arguments, which is set using setBaseIndirectArgs().
												The data at this offset should be a Gnm::DrawIndexIndirectArgs structure.
				
				@note	The index buffer should already have been set up using setIndexBuffer() and setIndexCount(),
						and the buffer containing the indirect arguments should have been set using setBaseIndirectArgs().
						The index count in args->m_indexCountPerInstance will be clamped to the value passed to setIndexCount().
				
				@see Gnm::DrawIndexIndirectArgs, setBaseIndirectArgs(), setIndexBuffer(), setIndexCount()
			*/
			void drawIndexIndirect(uint32_t dataOffsetInBytes)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndexIndirect can't be called between beginDispatchDraw and endDispatchDraw");
				// no need to set vertex and instance offsets here, they are coming from the draw indirect structure.
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				switch( m_cue.getActiveShaderStages())
				{
				case Gnm::kActiveShaderStagesVsPs:
					{
						const Gnmx::VsShader *pvs = m_cue.m_currentVSB;
						m_dcb.drawIndexIndirect(dataOffsetInBytes,Gnm::kShaderStageVs,pvs->m_fetchControl&0xf,(pvs->m_fetchControl>>4)&0xf);
					}
					break;
				case Gnm::kActiveShaderStagesEsGsVsPs:
					{
						const Gnmx::EsShader *pes = m_cue.m_currentESB;
						m_dcb.drawIndexIndirect(dataOffsetInBytes,Gnm::kShaderStageEs,pes->m_fetchControl&0xf,(pes->m_fetchControl>>4)&0xf);
					}
					break;
				case Gnm::kActiveShaderStagesLsHsVsPs:
				case Gnm::kActiveShaderStagesLsHsEsGsVsPs:
					{
						const Gnmx::LsShader *pls = m_cue.m_currentLSB;
						m_dcb.drawIndexIndirect(dataOffsetInBytes,Gnm::kShaderStageLs,pls->m_fetchControl&0xf,(pls->m_fetchControl>>4)&0xf);
					}
					break;
				case Gnm::kActiveShaderStagesDispatchDrawVsPs:
					SCE_GNM_ERROR("drawIndexIndirect does not support DispatchDaw");
				}
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Issues an indirect multi-draw call, which reads its parameters from a specified address in GPU memory.

				This is similar to calling drawIndirect() several times, with multiple DrawIndirectArgs objects tightly packed in memory.
				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.
				
				@param dataOffsetInBytes	The offset (in bytes) into the buffer that contains the indirect arguments, which is set using setBaseIndirectArgs().
											The data at this offset should be a Gnm::DrawIndexIndirectArgs structure.
				@param drawCount			The number of draw calls to launch. Each draw reads a new DrawIndexIndirectArgs object from the indirect arguments buffer.
				
				@note The index buffer and the buffer containing the indirect arguments should already have been set up using setIndexBuffer() and setIndexCount().
				      The index count in args->m_indexCountPerInstance will be clamped to the value passed to setIndexCount().
				
				@see Gnm::DrawIndexIndirectArgs, setBaseIndirectArgs(), setIndexBuffer(), setIndexCount()
			*/
			void drawIndexIndirectMulti(uint32_t dataOffsetInBytes, uint32_t drawCount)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndexIndirectMulti can't be called between beginDispatchDraw and endDispatchDraw");
				// no need to set vertex and instance offsets here, they are coming from the draw indirect structure.
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				switch( m_cue.getActiveShaderStages())
				{
				case Gnm::kActiveShaderStagesVsPs:
					{
						const Gnmx::VsShader *pvs = m_cue.m_currentVSB;
						m_dcb.drawIndexIndirectMulti(dataOffsetInBytes,drawCount,Gnm::kShaderStageVs,pvs->m_fetchControl&0xf,(pvs->m_fetchControl>>4)&0xf);
					}
					break;
				case Gnm::kActiveShaderStagesEsGsVsPs:
					{
						const Gnmx::EsShader *pes = m_cue.m_currentESB;
						m_dcb.drawIndexIndirectMulti(dataOffsetInBytes,drawCount,Gnm::kShaderStageEs,pes->m_fetchControl&0xf,(pes->m_fetchControl>>4)&0xf);
					}
					break;
				case Gnm::kActiveShaderStagesLsHsVsPs:
				case Gnm::kActiveShaderStagesLsHsEsGsVsPs:
					{
						const Gnmx::LsShader *pls = m_cue.m_currentLSB;
						m_dcb.drawIndexIndirectMulti(dataOffsetInBytes,drawCount,Gnm::kShaderStageLs,pls->m_fetchControl&0xf,(pls->m_fetchControl>>4)&0xf);
					}
					break;
				case Gnm::kActiveShaderStagesDispatchDrawVsPs:
					SCE_GNM_ERROR("drawIndexIndirectMulti does not support DispatchDaw");
				}
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			/** @brief Inserts a draw call using indices which are located in memory and whose base, size, and element size were set previously.

				Draw commands never roll the hardware context but instead use the current context such that the next command that sets context state will roll the context.
				
				@param[in] indexOffset			The starting index number in the index buffer.
				@param[in] indexCount			The number of indices to insert.
				
				@see Gnm::DrawCommandBuffer::setIndexBuffer(), Gnm::DrawCommandBuffer::setIndexCount(), Gnm::DrawCommandBuffer::setIndexSize()
			*/
			void drawIndexOffset(uint32_t indexOffset, uint32_t indexCount)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndexOffset can't be called between beginDispatchDraw and endDispatchDraw");
				SCE_GNM_VALIDATE(!m_cue.isVertexOrInstanceOffsetEnabled(), "Using a shader that is expecting a vertex and/or instance offset without specifing them");
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawIndexOffset(indexOffset, indexCount);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}
			/** @brief Inserts a draw call, using indices which are located in memory and whose base, size, and element size were set previously,
						while adding an offset to the vertex and instance indexes.

				Draw commands never roll the hardware context, but use the current context such that the next command that sets context state will roll the context.
				
				@param indexOffset			The starting index number in the index buffer.
				@param indexCount			The number of indices to insert.
				@param vertexOffset			The offset added to each vertex index.
				@param instanceOffset		The offset added to each instance index.
				
				@see Gnm::DrawCommandBuffer::setIndexBuffer(), Gnm::DrawCommandBuffer::setIndexCount(), Gnm::DrawCommandBuffer::setIndexSize()
			*/
			void drawIndexOffset(uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, uint32_t instanceOffset)
			{
				SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "drawIndexOffset can't be called between beginDispatchDraw and endDispatchDraw");
				m_cue.setVertexAndInstanceOffset(&m_dcb, vertexOffset,instanceOffset);
				m_cue.preDraw(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.drawIndexOffset(indexOffset, indexCount);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDraw(&m_dcb, &m_ccb);
			}

			////////////// Dispatch commands

			/** @brief Inserts a compute shader dispatch with the indicated number of thread groups.
				This function never rolls the hardware context.
				@param[in] threadGroupX Number of thread groups dispatched along the X dimension.
				@param[in] threadGroupY Number of thread groups dispatched along the Y dimension.
				@param[in] threadGroupZ Number of thread groups dispatched along the Z dimension.
			*/
			void dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
			{
				m_cue.preDispatch(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.dispatch(threadGroupX, threadGroupY, threadGroupZ);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDispatch(&m_dcb, &m_ccb);
			}

			/** @brief Inserts an indirect compute shader dispatch, whose parameters are read from GPU memory.
			This function never rolls the hardware context.
			@param[in] dataOffsetInBytes Offset (in bytes) into the buffer that contains the indirect arguments (set using <c>setBaseIndirectArgs()</c>).
			                  The data at this offset should be a Gnm::DispatchIndirectArgs structure.
			@note The buffer containing the indirect arguments should already have been set using <c>setBaseIndirectArgs()</c>.
			@see Gnm::DispatchCommandBuffer::setBaseIndirectArgs()
			*/
			void dispatchIndirect(uint32_t dataOffsetInBytes)
			{
				m_cue.preDispatch(&m_dcb, &m_ccb);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				const uint32_t offset = beginRecordLastCompletion();
#endif
				m_dcb.dispatchIndirect(dataOffsetInBytes);
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
				endRecordLastCompletion(offset);
#endif
				m_cue.postDispatch(&m_dcb, &m_ccb);
			}

			/** @brief Sets up context state and synchronization for dispatch draw and must be called before issuing a block of dispatchDraw() calls.

				This function inserts a CP label write into the DrawCommandBuffer and a corresponding wait into the asynchronous compute
				DispatchCommandBuffer. This is done to prevent dispatchDraw asynchronous compute wavefronts from being launched long before the DrawCommandBuffer
				launches the vertex wavefronts to consume their output data.  If the index ring buffer has insufficient space for all of the output from
				all launched compute wavefronts, any wavefronts which do not find available space will stall and tie up shading resources until 
				the consumer wavefronts launch.

				This function will roll context and modifies context state associated with setIndexSize(), setIndexBuffer(), and setVgtControl().
				setIndexSize(), setIndexBuffer(), setVgtControl(), or any non-dispatchDraw draw functions may not be called between beginDispatchDraw()
				and endDispatchDraw().

				@note m_acb.setComputeResourceManagement() should also be used to ensure that there is at least one compute unit for Vshell shaders
				      to use in the event that Vshell interrupts in the middle of a dispatchDraw() call.  Dispatch draw compute shader wavefronts are 
					  not guaranteed to complete once launched. This is because they may stall until the consumer vertex shader wavefronts have launched
					  to free up space if the index ring buffer is full. Limiting dispatchDraw() computes to run on 8 of the 9 compute units in 
					  each SE is sufficient to prevent deadlocks against Vshell rendering.
				@note Unless the compute dispatches are guaranteed not to fill the ring buffers (and halt) or are limited to no more than 1 thread group per CU,
				      tessellation or on-chip GS draw commands cannot be overlapped by DispatchCommandBuffer::dispatchDraw() compute dispatches.
				      A GPU hang can result if a tessellation or on-chip GS thread group launches on a CU that is already hosting more than 1 thread group
				      of compute, which is waiting for space in the dispatch draw ring buffers. In general, if any dispatchDraw commands will follow, it is safer
					  to prevent all overlap of dispatch draw compute with tessellation or on-chip GS draw commands by calling
					  GfxContext::dispatchDrawComputeWaitForEndOfPipe() after the last tessellation or on-chip GS draw command.
			 */
			void beginDispatchDraw();

			/** @brief Restores the context state and must be called after issuing a block of dispatchDraw() calls.

				This function will roll context and modifies the context state associated with setIndexSize(), setIndexBuffer(), and setVgtControl().

				@param[in] indexSize				The Gnm::IndexSize value to restore.
				@param[in] indexAddr				The index buffer pointer to restore.
				@param[in] primGroupSizeMinus1		The setVgtControl() value to restore; the default setting for normal rendering is 255.
				@param[in] partialVsWaveMode		The setVgtControl() value to restore; the default setting for normal rendering is kVgtPartialVsWaveDisable.
				@param[in] switchOnEopMode			The setVgtControl() value to restore; the default setting for normal rendering is kVgtSwitchOnEopDisable.
			 */
			void endDispatchDraw(Gnm::IndexSize indexSize, const void *indexAddr, uint8_t primGroupSizeMinus1, Gnm::VgtPartialVsWaveMode partialVsWaveMode, Gnm::VgtSwitchOnEopMode switchOnEopMode);

			/** @brief Restores context state with default setVgtControl() settings and must be called after issuing a block of dispatchDraw() to restore context state.

				This function will roll context and modifies the context state associated with setIndexSize(), setIndexBuffer(), and setVgtControl(). It restores state to
				setIndexSize(indexSize), setIndexBuffer(indexAddr) and setVgtControl(255, Gnm::kVgtPartialVsWaveDisable, Gnm::kVgtSwitchOnEopDisable).

				@param[in] indexSize				The Gnm::IndexSize value to restore.
				@param[in] indexAddr				The index buffer pointer to restore.

				
			 */
			inline void endDispatchDraw(Gnm::IndexSize indexSize, const void *indexAddr)
			{
				endDispatchDraw(indexSize, indexAddr, 255, Gnm::kVgtPartialVsWaveDisable, Gnm::kVgtSwitchOnEopDisable);
			}

			/** @brief Inserts an asynchronous compute shader Gnm::DispatchCommandBuffer::dispatchDraw() and a paired vertex shader Gnm::DrawCommandBuffer::dispatchDraw().

				Draw commands never roll the hardware context but use the current context such that the next command that sets context state will roll the context.

				@note dispatchDraw() also has the following state requirements to function properly in addition to those set directly by beginDispatchDraw() and dispatchDraw():
				      setActiveShaderStages(Gnm::kActiveShaderStagesDispatchDrawVsPs) : Always required by dispatch draw hardware.
				      setNumInstances(1) : Dispatch draw is not compatible with hardware instancing. Instancing is implemented in software via setDispatchDrawNumInstances().
				
				@note Unless the compute dispatches are guaranteed not to fill the ring buffers (and halt) or are limited to no more than 1 thread group per CU,
				      tessellation or on-chip GS draw commands cannot be overlapped by DispatchCommandBuffer::dispatchDraw() compute dispatches.
				      A GPU hang can result if a tessellation or on-chip GS thread group launches on a CU that is already hosting more than 1 thread group
				      of compute, which is waiting for space in the dispatch draw ring buffers. In general, if any dispatchDraw commands will follow, it is safer
					  to prevent all overlap of dispatch draw compute with tessellation or on-chip GS draw commands by calling
					  GfxContext::dispatchDrawComputeWaitForEndOfPipe() after the last tessellation or on-chip GS draw command.			   

				@param[in] pDispatchDrawIndexData	The input data to a dispatch draw asynchronous compute shader, which should be generated by Gnmx::createDispatchDrawInputData().
			   */
			void dispatchDraw(DispatchDrawTriangleCullIndexData const *pDispatchDrawIndexData);

			/** @brief Inserts an EOP label write in the DrawCommandBuffer and a corresponding DispatchCommandBuffer::waitOnAddress() in the asynchronous compute buffer.
						This function never rolls the hardware context.
			 
				@note Unless the compute dispatches are guaranteed not to fill the ring buffers (and halt) or are limited to no more than 1 thread group per CU,
				      tessellation or on-chip GS draw commands cannot be overlapped by DispatchCommandBuffer::dispatchDraw() compute dispatches.
				      A GPU hang can result if a tessellation or on-chip GS thread group launches on a CU that is already hosting more than 1 thread group
				      of compute, which is waiting for space in the dispatch draw ring buffers. In general, if any dispatchDraw commands will follow, it is safer
					  to prevent all overlap of dispatch draw compute with tessellation or on-chip GS draw commands by calling
					  GfxContext::dispatchDrawComputeWaitForEndOfPipe() after the last tessellation or on-chip GS draw command.			   
				
				@see dispatchDraw()
			 */
			void dispatchDrawComputeWaitForEndOfPipe();

			//////////// Wrappers around Gnmx helper functions

			/** @brief Sets the render target for the specified RenderTarget slot.

				This wrapper automatically works around a rare hardware quirk involving CMASK cache
				corruption with multiple render targets whose FMASK pointers are identical (for example, if they are all NULL).

				This function will roll the hardware context.
				@param[in] rtSlot RenderTarget slot index to which this render target is set to (0-7).
				@param[in] target  Pointer to a Gnm::RenderTarget structure. If NULL is passed, the color buffer for this slot is disabled.
				@sa Gnm::RenderTarget::disableFmaskCompressionForMrtWithCmask()
			*/
			void setRenderTarget(uint32_t rtSlot, Gnm::RenderTarget const *target);

			/** @brief Uses the CP DMA to clear a buffer to specified value (such as a GPU memset).
				This function never rolls the hardware context.
				@param[out] dstGpuAddr    Destination address to write the data to.
				@param[in] srcData       The value to fill the destination buffer with.
				@param[in] numBytes      Size of the destination buffer.  Must be a multiple of 4.
				@param[in] isBlocking    If true, the CP will block while the transfer is active.
			*/
			void fillData(void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
			{
				return Gnmx::fillData(&m_dcb, dstGpuAddr, srcData, numBytes, isBlocking);
			}

			/** @brief Uses the CP DMA to transfer data from a source address to a destination address.
				This function never rolls the hardware context.
				@param[out] dstGpuAddr          Destination address to write the data to.
				@param[in] srcGpuAddr          Source address to read the data from.
				@param[in] numBytes            Number of bytes to transfer over.
				@param[in] isBlocking          If true, the CP waits for the DMA to be complete before performing any more processing.
			*/
			void copyData(void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
			{
				return Gnmx::copyData(&m_dcb, dstGpuAddr, srcGpuAddr, numBytes, isBlocking);
			}

			/** @brief Inserts user data directly inside the command buffer returning a locator for later reference.
				This function never rolls the hardware context.
				@param[in] dataStream  Pointer to the data.
				@param[in] sizeInDword Size of the data in stride of 4. Note that the maximum size of a single command packet is 2^16 bytes,
								   and the effective maximum value of <c><i>sizeInDword</i></c> will be slightly less than that due to packet headers
								   and padding.
				@param[in] alignment   Alignment of the embedded copy in the command buffer.
				@return            Returns a pointer to the allocated buffer.
			*/
			void* embedData(const void *dataStream, uint32_t sizeInDword, Gnm::EmbeddedDataAlignment alignment)
			{
				return Gnmx::embedData(&m_dcb, dataStream, sizeInDword, alignment);
			}

			/** @brief Sets the multisampling sample locations to default values.
				This function will roll hardware context.
				@param[in] numSamples Number of samples used while multisampling.
			*/
			void setAaDefaultSampleLocations(Gnm::NumSamples numSamples)
			{
				return Gnmx::setAaDefaultSampleLocations(&m_dcb, numSamples);
			}

			/** @brief Utility function that configures the viewport, scissor, and guard band for the provided viewport dimensions.

				If more control is required, users can call the underlying functions manually.
				This function will roll hardware context.
				@param[in] left The X coordinate of the left edge of the rendering surface, in pixels.
				@param[in] top The Y coordinate of the top edge of the rendering surface, in pixels.
				@param[in] right The X coordinate of the right edge of the rendering surface, in pixels.
				@param[in] bottom The Y coordinate of the bottom edge of the rendering surface, in pixels.
				@param[in] zScale Scale value for the Z transform from clip-space to screen-space. The correct value depends on which
				              convention you're following in your projection matrix. For OpenGL-style matrices, use <c><i>zScale</i></c>=0.5. For Direct3D-style
							  matrices, use <c><i>zScale</i></c>=1.0.
				@param[in] zOffset Offset value for the Z transform from clip-space to screen-space. The correct value depends on which
							  convention you're following in your projection matrix. For OpenGL-style matrices, use <c><i>zOffset</i></c>=0.5. For Direct3D-style
							  matrices, use <c><i>zOffset</i></c>=0.0.
			*/
			void setupScreenViewport(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset)
			{
				return Gnmx::setupScreenViewport(&m_dcb, left, top, right, bottom, zScale, zOffset);
			}

			/** @brief Utility function that configures dispatch draw shaders for the given viewport dimensions and guard band.

				This function will not hardware context.
				@param[in] left The X coordinate of the left edge of the rendering surface, in pixels.
				@param[in] top The Y coordinate of the top edge of the rendering surface, in pixels.
				@param[in] right The X coordinate of the right edge of the rendering surface, in pixels.
				@param[in] bottom The Y coordinate of the bottom edge of the rendering surface, in pixels.
			*/
			void setupDispatchDrawScreenViewport(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

			/** @brief Helper function that configures DispatchDraw shaders for the given clip and cull settings.

				This function will not roll the hardware context.

				@param[in] primitiveSetup The value currently set to setPrimitiveSetup().
				@param[in] clipControl The value currently set to setClipControl().
			*/
			void setupDispatchDrawClipCullSettings(Gnm::PrimitiveSetup primitiveSetup, Gnm::ClipControl clipControl);

			/** Helper function that configures DispatchDraw shaders for the given clip and cull settings.

				This function will not roll the hardware context.
				@param[in] dispatchDrawClipCullFlags A union of <c>Gnm::kDispatchDrawClipCullFlag*</c> flags controlling DispatchDraw shader clipping and culling. 
			*/
			void setupDispatchDrawClipCullSettings(uint32_t dispatchDrawClipCullFlags)
			{
				m_pDispatchDrawData->m_clipCullSettings = (uint16_t)((m_pDispatchDrawData->m_clipCullSettings &~kDispatchDrawClipCullMask) | (dispatchDrawClipCullFlags & kDispatchDrawClipCullMask));
			}

			/** Sets the number of instances for subsequent dispatchDraw calls and is analogous to setNumInstances() for standard draws.

				Vertex input attributes using index kFetchShaderUseInstanceId will read their vertex buffers using the instance ID rather than the
				vertex index. The instance ID is an alternate index for vertex buffer reads that increments from 0 to <c>numInstances-1</c> with all
				vertices in each instance passed the same instance ID.

				Dispatch draw is limited to 2^16 instances per GfxContext::dispatchDraw() call. GfxContext::dispatchDraw() may issue many pairs of 
				DrawCommandBuffer::dispatchDraw() and DispatchCommandBuffer::dispatchDraw() calls in order to render the total requested number of
				instances, but it will only update the constant update engine (CUE) once for all of the calls.

				This function will not roll the hardware context.
				
				@param[in] numInstances		The number of instances to render in the range [0:65536] with 0 interpreted as 1.
			*/
			void setDispatchDrawNumInstances(uint32_t numInstances)
			{
				SCE_GNM_VALIDATE(numInstances <= 0x10000, "setDispatchDrawNumInstances: numInstances must be in the range [0:1<<16] (0:65536)");
				if (numInstances == 0 || numInstances > 0x10000)
					numInstances = 1;
				m_dispatchDrawNumInstancesMinus1 = (uint16_t)(numInstances-1);
			}

			/** Sets the instance step rates for subsequent dispatchDraw calls and is analogous to setInstanceStepRate() for standard draws.

				Vertex input attributes using index kFetchShaderUseInstanceIdOverStepRate0 or kFetchShaderUseInstanceIdOverStepRate1
				will read their vertex buffers using the instance ID divided by <c><i>stepRate0</i></c> or <c><i>stepRate1</i></c> rather than the
				vertex index. The instance ID is an alternate index for vertex buffer reads that increments from 0 to <c>numInstances-1</c> with all
				vertices in each instance passed the same instance ID.

				This function will not roll the hardware context.

				@param[in] stepRate0		The step rate for vertex input attributes using index kFetchShaderUseInstanceIdOverStepRate0 in the range [0:65536] with 0 interpreted as 1.
				@param[in] stepRate1		The step rate for vertex input attributes using index kFetchShaderUseInstanceIdOverStepRate1 in the range [0:65536] with 0 interpreted as 1.
			*/
			void setDispatchDrawInstanceStepRate(uint32_t stepRate0, uint32_t stepRate1)
			{
				SCE_GNM_VALIDATE(stepRate0 <= 0x10000 && stepRate1 <= 0x10000, "setDispatchDrawInstanceStepRate: stepRate0 and stepRate1 must be in the range [0:1<<16] (0:65536)");
				if (stepRate0 == 0 || stepRate0 > 0x10000)
					stepRate0 = 1;
				if (stepRate1 == 0 || stepRate1 > 0x10000)
					stepRate1 = 1;
				m_pDispatchDrawData->m_instanceStepRate0Minus1 = (uint16_t)(stepRate0-1);
				m_pDispatchDrawData->m_instanceStepRate1Minus1 = (uint16_t)(stepRate1-1);
			}

			/**
			 * @brief Wrapper around <c>dmaData()</c> to clear the values of one or more append or consume buffer counters to the specified value.
			 *
			 *	This function never rolls the hardware context.
			 *
			 * @param[in] destRangeByteOffset Byte offset in GDS to the beginning of the counter range to clear. Must be a multiple of 4.
			 * @param[in] startApiSlot Index of the first <c>RW_Buffer</c> API slot whose counter should be updated. Valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
			 * @param[in] numApiSlots Number of consecutive slots to update. <c><i>startApiSlot</i> + <i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
			 * @param[in] clearValue The value to set the specified counters to.
			 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()
			 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
			 */
			void clearAppendConsumeCounters(uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, uint32_t clearValue)
			{
				return Gnmx::clearAppendConsumeCounters(&m_dcb, destRangeByteOffset, startApiSlot, numApiSlots, clearValue);
			}

			/**
			 * @brief Wrapper around <c>dmaData()</c> to update the values of one or more append or consume buffer counters, using values source from the provided GPU-visible address.
			 *
			 * This function never rolls the hardware context.
			 *
			 * @param[in] destRangeByteOffset Byte offset in GDS to the beginning of the counter range to update. Must be a multiple of 4.
			 * @param[in] startApiSlot Index of the first <c>RW_Buffer</c> API slot whose counter should be updated. Valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
			 * @param[in] numApiSlots Number of consecutive slots to update. <c><i>startApiSlot</i> + <i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
			 * @param[in] srcGpuAddr GPU-visible address to read the new counter values from.
			 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()
			 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
			 */
			void writeAppendConsumeCounters(uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, const void *srcGpuAddr)
			{
				return Gnmx::writeAppendConsumeCounters(&m_dcb, destRangeByteOffset, startApiSlot, numApiSlots, srcGpuAddr);
			}

			/**
			 * @brief Wrapper around <c>dmaData()</c> to retrieve the values of one or more append or consume buffer counters and store them in a GPU-visible address.
			 *
			 * This function never rolls the hardware context.
			 *
			 * @param[out] destGpuAddr GPU-visible address to write the counter values to.
			 * @param[in] srcRangeByteOffset Byte offset in GDS to the beginning of the counter range to read. Must be a multiple of 4.
			 * @param[in] startApiSlot Index of the first RW_Buffer API slot whose counter should be read. Valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
			 * @param[in] numApiSlots Number of consecutive slots to read. <c><i>startApiSlot</i> + <i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
			 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()
			 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
			 */
			void readAppendConsumeCounters(void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots)
			{
				return Gnmx::readAppendConsumeCounters(&m_dcb, destGpuAddr, srcRangeByteOffset, startApiSlot, numApiSlots);
			}

			/** @brief Splits the command buffers.
					   
				This function will cause the GfxContext to stop adding commands to the active command buffers and create new ones.
				This is most often necessary when chainCommandBuffer(), which must be the last packet in its DCB.
			*/
			bool splitCommandBuffers(void);

#ifdef SCE_GNMX_ENABLE_GFXCONTEXT_CALLCOMMANDBUFFERS // Disabled by default until a future release
			/** @brief Call another command buffer pair. When the called command buffers end, execution will continue
			           in the current command buffer.
					   
				@param dcbBaseAddr Address of the destination dcb. This pointer must not be NULL if dcbSizeInDwords is non-zero.
				@param dcbSizeInDwords Size of the destination dcb in DWORDs. May be 0 if no dcb is required.
				@param ccbBaseAddr Address of the destination ccb. This pointer must not be NULL if ccbSizeInDwords is non-zero.
				@param ccbSizeInDwords Size of the destination ccb in DWORDs. May be 0 if no ccb is required.
				@note After this function is called, all previous resource/shader bindings and render state are undefined.
			*/
			void callCommandBuffers(void *dcbAddr, uint32_t dcbSizeInDwords, void *ccbAddr, uint32_t ccbSizeInDwords);
#endif

			// auto-generated method forwarding to m_dcb
			#include "gfxcontext_methods.h"

		public:
			Gnm::DrawCommandBuffer     m_dcb; ///< Draw command buffer. Access directly at your own risk!
			Gnm::DispatchCommandBuffer m_acb; ///< Asynchronous compute dispatch command buffer. Access directly at your own risk!
			Gnm::ConstantCommandBuffer m_ccb; ///< Constant command buffer. Access directly at your own risk!
			ConstantUpdateEngine       m_cue; ///< Constant update engine. Access directly at your own risk!
			Gnmx::ComputeQueue     *m_pQueue; ///< Compute queue for <c>m_acb</c>.  Access directly at your own risk!

			Gnmx::DispatchDrawTriangleCullData *m_pDispatchDrawData;	///< A pointer to the current working DispatchDrawTriangleCullData, which is always allocated from <c>m_acb</c>.
			uint32_t m_dispatchDrawIndexDeallocMask;					///< The last dispatch draw compute shader index deallocation mask value.
			uint16_t m_dispatchDrawNumInstancesMinus1;					///< The setDispatchDrawNumIndices() value.
			uint16_t m_dispatchDrawFlags;								///< A union of kDispatchDrawFlag*.

			static const uint16_t kDispatchDrawFlagInDispatchDraw	= 0x0001;	///< between beginDispatchDraw and endDispatchDraw
			static const uint16_t kDispatchDrawFlagIrbValid			= 0x0002;	///< A flag that indicates that setupDispatchDrawRingBuffers() was called with valid inputs.

			static const uint32_t kIndexSizeFlagDispatchDraw				= 0x00000010;	///< bit 4 of Gnm::IndexSize indicates dispatch draw is enabled.

#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
			/** @brief Describes whether, and how, to record which Draw or Dispatch completed last for debug purposes.
				@sa initializeRecordLastCompletion()
				*/
			typedef enum RecordLastCompletionMode
			{
				kRecordLastCompletionDisabled,     ///< Do not record the offset of the last Draw or Dispatch that completed.
				kRecordLastCompletionAsynchronous, ///< Write command buffer offset of draw or dispatch at EOP to known offset in the DrawCommandBuffer.
				kRecordLastCompletionSynchronous,  ///< Write command buffer offset of draw or dispatch at EOP to known offset in the DrawCommandBuffer, and wait for this value to be written before proceeding to the next draw or dispatch.
			} RecordLastCompletionMode;
			RecordLastCompletionMode m_recordLastCompletionMode; ///< Whether, and how, to record which Draw or Dispatch completed last.

			/** @brief Specifies whether, and how, to record which Draw or Dispatch completed last.
			           If level is not kRecordLastCompletionDisabled, must be called after reset() and before the first draw or dispatch.
					   This feature is used to help narrow down the cause of a GPU crash. If the instrumentation is not <c>kRecordLastCompletionDisabled</c>,
					   each draw or dispatch call is instrumented to write its own command buffer offset to a label upon completion, and optionally stall the CP until
					   the label has been updated. The default instrumentation level is <c>kRecordLastCompletionDisabled</c>.
				@param[in] level The record-lastc-completion mode to set.
			*/
			void initializeRecordLastCompletion(RecordLastCompletionMode level);

			/** @brief The address where the command buffer offset of the most recently completed draw/dispatch in the DCB is stored when that draw/dispatch completes.
			
				       Helpful for determining which call caused the GPU to crash.
				@sa RecordLastCompletionMode()
			*/
			uint32_t *m_addressOfOffsetOfLastCompletion;
#endif

#if !defined(DOXYGEN_IGNORE)
			// The following code/data is used to work around the hardware's 4 MB limit on individual command buffers. We use the m_callback
			// fields of m_dcb and m_ccb to detect when either buffer crosses a 4 MB boundary, and save off the start/size of both buffers
			// into the m_submissionRanges array. When submit() is called, the m_submissionRanges array is used to submit each <4MB chunk individually.
			//
			// In order for this code to function properly, users of this class must not modify m_dcb.m_callback or m_ccb.m_callback!
			// To register a callback that triggers when m_dcb/m_ccb run out of space, use m_bufferFullCallback.
			static const uint32_t kMaxNumStoredSubmissions = 16; // Maximum number of <4MB submissions that can be recorded. Make this larger if you want more; it just means GfxContext objects get larger.
			const uint32_t *m_currentDcbSubmissionStart; // Beginning of the submit currently being constructed in the DCB
			const uint32_t *m_currentAcbSubmissionStart; // Beginning of the submit currently being constructed in the ACB
			const uint32_t *m_currentCcbSubmissionStart; // Beginning of the submit currently being constructed in the CCB
			const uint32_t *m_actualDcbEnd; // Actual end of the DCB's data buffer (i.e. dcbBuffer+dcbSizeInBytes/4)
			const uint32_t *m_actualAcbEnd; // Actual end of the ACB's data buffer (i.e. acbBuffer+acbSizeInBytes/4)
			const uint32_t *m_actualCcbEnd; // Actual end of the CCB's data buffer (i.e. ccbBuffer+ccbSizeInBytes/4)
			class SubmissionRange
			{
			public:
				uint32_t m_dcbStartDwordOffset, m_dcbSizeInDwords;
				uint32_t m_acbStartDwordOffset, m_acbSizeInDwords;
				uint32_t m_ccbStartDwordOffset, m_ccbSizeInDwords;
			};
			SubmissionRange m_submissionRanges[kMaxNumStoredSubmissions]; // Stores the range of each previously-constructed submission (not including the one currently under construction).
			uint32_t m_submissionCount; // The current number of stored submissions in m_submissionRanges (again, not including the one currently under construction).

			/** @brief Defines a type of function that is called when one of the GfxContext command buffers is out of space. 

				@param[in,out] gfxc					A pointer to the GfxContext object whose command buffer is out of space.
				@param[in,out] cb					A pointer to the CommandBuffer object that is out of space. This will be one of gfxc's CommandBuffer members.
				@param[in] sizeInDwords				The size of the unfulfilled CommandBuffer request in dwords.
				@param[in] userData					The user data passed to the function.
				
				@return A value of true is returned if the requested space is available in <c><i>cb</i></c> when the function returns; otherwise false is returned.
			 */
			typedef bool (*BufferFullCallbackFunc)(GfxContext *gfxc, Gnm::CommandBuffer *cb, uint32_t sizeInDwords, void *userData);

			/** @brief Bundles a callback function pointer and the data that will be passed to it. */
			class BufferFullCallback
			{
			public:
				BufferFullCallbackFunc m_func; ///< Function to call when the command buffer is out of space.
				void *m_userData; ///< User data passed to <c><i>m_func</i></c>.
			};

			BufferFullCallback m_cbFullCallback; // Invoked when m_dcb or m_ccb actually runs out of space (as opposed to crossing a 4 MB boundary)
			SCE_GNM_API_DEPRECATED_VAR_MSG("Use m_cbFullCallback instead")
			Gnm::CommandCallback m_bufferFullCallback; // Invoked when m_dcb or m_ccb actually runs out of space (as opposed to crossing a 4 MB boundary)

#endif // !defined(DOXYGEN_IGNORE)
		};

#if !defined(SCE_GNM_OFFLINE_MODE)
		/** @brief Stripped-down variant of GfxContext, used to load a serialized GfxContext created offline, patch its pointers, and submit it.

		      The rough sequence of events when using this class is:

				<b>Offline</b>
				<ol>
				<li> Create a GfxContext, passing 0 as the ConstantUpdateEngine heap address.
				<li> Use the GfxContext functions to build GPU command buffers, as usual. Any VRAM address that is passed into the API must be
						"disguised" using Gnm::CommandBufferPatchTableBuilder::disguiseDwordOffsetAsAddress() or Gnm::CommandBufferPatchTableBuilder::disguiseUniqueIdAsAddress().
						These disguised offsets/IDs will be used to build a patch table that must be applied to the command buffer at runtime, once the final VRAM
						addresses are known.
				<li> Use GfxContext::getSerializedSizes() to determine the size of the two serialization output buffers.
				<li> Allocate space for the output buffers, and use GfxContext::serializeIntoBuffers() to populate them with data.
				<li> Write the serialized data buffers to disk.
				</ol>
				<b>Runtime</b>
				<ol>
				<li> Load the temporary buffer (generated offline by GfxContext::serializeIntoBuffers()) into system private memory.
				<li> Load the persistent buffer (generated offline by GfxContext::serializeIntoBuffers()) into shared VRAM.
				<li> Extract the required ConstantUpdateEngine heap size from the temp buffer using GfxContextSubmitOnly::getRequiredCueHeapSize().
				<li> Allocate the ConstantUpdateEngine heap in GPU private memory.
				<li> Initialize the submit-only context with GfxContextSubmitOnly::init()
				<li> Set the final addresses for any unique-ID patches in m_dcbPatchTable and m_ccbPatchTable using Gnm::CommandBufferPatchTable::setAddressForPatchId()
				<li> Apply the finished patch tables to the command buffers with GfxContextSubmitOnly::patchCommandBuffers(). After the command buffers are patched,
				   the temporary buffer can be safely freed (but this will prevent the command buffer from being re-patched later).
				<li> It is now safe to call GfxContextSubmitOnly::submit() and GfxContextSubmitOnly::validate().
				</ol>
			@note This class is not available in offline mode.
			*/
		class GfxContextSubmitOnly
		{
		public:
			/** @brief Default constructor. */
			GfxContextSubmitOnly(void);
			/** @brief Default destructor. */
			~GfxContextSubmitOnly(void);

			/** @brief Calculates the required size for the ConstantUpdateEngine heap buffer passed to GfxContextSubmitOnly::init().
				@param[in] srcTempBuffer Pointer to the "temp" buffer generated by GfxContext::serializeIntoBuffers(). The heap size is extracted from
				                     this buffer.
				@return The size (in bytes) of the ConstantUpdateEngine heap buffer. Use this to allocate the ConstantUpdateEngine heap.
				@sa GfxContextSubmitOnly::init()
				*/
			static size_t getRequiredCueHeapSize(const void *srcTempBuffer);

			/** @brief Initializes the submit-only context, using data from the two buffers generated offline by GfxContext::serializeIntoBuffers().
				@param[out] cueHeapAddr Address of the buffer used to simulate the ConstantUpdateEngine heap (even though this object does not contain a ConstantUpdateEngine). Use GfxContextSubmitOnly::getRequiredCueHeapSize() to
				                   determine the size of this buffer.
				@param[in] srcTempBuffer The temp buffer generated by GfxContext::serializeIntoBuffers(). This buffer should be in system private memory, and must be aligned to a 4-byte boundary.
				                     It can be freed by the caller after GfxContextSubmitOnly::patchCommandBuffers() is called.
				@param[in] srcTempBufferSize The size of <c><i>srcTempBuffer</i></c>, in bytes.
				@param[in] srcPersistentBuffer The persistent buffer generated by GfxContext::serializeIntoBuffers(). This buffer should be in CPU/GPU shared memory, and must be aligned to a 256-byte boundary.
				                           It is the caller's responsibility to free this memory when this context is no longer needed.
				@param[in] srcPersistentBufferSize The size of the persistent buffer specified by <c><i>srcPersistentBuffer</i></c>, in bytes.
				*/
			void init(void *cueHeapAddr, void *srcTempBuffer, uint32_t srcTempBufferSize, void *srcPersistentBuffer, uint32_t srcPersistentBufferSize);

			/** @brief Sets the compute queue the GfxContext should use for dispatch draw dispatches.
				@param[in,out]  pQueue   ComputeQueue to use for dispatch draw dispatches.
				*/
			void setDispatchDrawComputeQueue(ComputeQueue *pQueue)
			{
				m_pQueue = pQueue;
			}

			/** @brief Assigns a final VRAM address to the command buffer patch with the specified unique ID.

				This function sets the address in <c><i>m_dcbPatchTable</i></c> first, or in <c><i>m_ccbPatchTable</i></c> if not found in <c><i>m_dcbPatchTable</i></c>.
				@param[in] id The unique ID of the patch to associate with <c><i>finalAddr</i></c>.
				@param[in] finalAddr The address in VRAM to associate with the specified unique ID when patching the command buffer.
				@return Returns true if the specified ID was located in either <c><i>m_dcbPatchTable</i></c> or <c><i>m_ccbPatchTable</i></c>, or false if the ID was not found in
				        either patch table.
				@sa GfxContextSubmitOnly::patchCommandBuffers(), Gnm::CommandBufferPatchTableBuilder::disguiseUniqueIdAsAddress()
				*/
			bool setAddressForPatchId(uint32_t id, void *finalAddr);

			/** @brief Applies the patches in Draw and Constant command buffers to their respective command buffers (<c>m_dcb</c> and <c>m_ccb</c>).
			    @note Before this function can be called, all patches with unique IDs must be associated with their final VRAM addresses
					   using GfxContextSubmitOnly::setAddressForPatchId().
				@note This function must be called before GfxContextSubmitOnly::submit() or GfxContextSubmitOnly::validate() can be used.

				@note After this function is called, it is safe for the caller to free the temp buffer passed to GfxContextSubmitOnly::init().
				@sa GfxContextSubmitOnly::setAddressForPatchId()
				*/
			void patchCommandBuffers(void);

			/** @brief Submits the Gnm::DrawCommandBuffer and Gnm::ConstantCommandBuffer.
				@note It is an error to call this function before calling GfxContextSubmitOnly::patchCommandBuffers().
				@return A code indicating the submission status.
				*/
			int32_t submit(void);


			/** @brief Runs validation on the Gnm::DrawCommandBuffer and Gnm::ConstantCommandBuffer without submitting them.
				@note It is an error to call this function before calling GfxContextSubmitOnly::patchCommandBuffers().
				@return A code indicating the validation status.
				*/
			int32_t validate(void);

			/** @brief Submits the DrawCommandBuffer and ConstantCommandBuffer and immediately requests a flip.
				@note This function is not available in offline mode.
				@param[in] videoOutHandle Video out handle.
				@param[in] rtIndex         Index to the render target to flip to.
				@param[in] flipMode        Flip mode.
				@param[in] flipArg         Flip argument.
				@return A code indicating the error status.
				@note To enable auto-validation on submit(), please enable the "GPU Validation" option in the Target Manager Settings.
				*/
			int32_t submitAndFlip(uint32_t videoOutHandle, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg);

		public:
			Gnm::DrawCommandBuffer     m_dcb;				///< A draw command buffer to which this context's commands will be written. Access directly at your own risk!
			Gnm::DispatchCommandBuffer m_acb;				///< An asynchronous compute dispatch command buffer to which this context's DispatchDraw() commands will be written. Access directly at your own risk!
			Gnm::ConstantCommandBuffer m_ccb;				///< A constant command buffer. Access directly at your own risk!
			void *m_cueHeapAddr;							///< The buffer in which to store the data written by the CCB. It acts like the ConstantUpdateEngine's ring buffers even though no actual ConstantUpdateEngine is required.
			Gnm::CommandBufferPatchTable m_dcbPatchTable;	///< The patch table for <c>m_dcb</c>.
			Gnm::CommandBufferPatchTable m_acbPatchTable;	///< The patch table for <c>m_acb</c>.
			Gnm::CommandBufferPatchTable m_ccbPatchTable;	///< The patch table for <c>m_ccb</c>.
			Gnmx::ComputeQueue     *m_pQueue;				///< The compute queue for <c>m_acb</c>. Access directly at your own risk!

#if !defined(DOXYGEN_IGNORE)
			// The following code/data is used to work around the hardware's 4 MB limit on individual command buffer submissions.
			static const uint32_t kMaxNumStoredSubmissions = 16;						// Maximum number of <4MB submissions that can be recorded.
			GfxContext::SubmissionRange m_submissionRanges[kMaxNumStoredSubmissions];	// Stores the range of each previously-constructed submit.
			uint32_t m_submissionCount;													// The current number of stored submission in m_submissionRanges.
		#endif // !defined(DOXYGEN_IGNORE)
		};
#endif // !defined(SCE_GNM_OFFLINE_MODE)
	}
}
#endif
