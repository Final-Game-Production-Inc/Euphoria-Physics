/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_LWCOMPUTECONSTANTUPDATEENGINE_H)
#define _SCE_GNMX_LWCOMPUTECONSTANTUPDATEENGINE_H

#include "lwbaseconstantupdateengine.h"

namespace sce
{
	namespace Gnmx
	{
		namespace LightweightConstantUpdateEngine
		{

			/** @brief Lightweight ConstantUpdateEngine for async-compute context */
			class SCE_GNMX_EXPORT ComputeConstantUpdateEngine : public BaseConstantUpdateEngine
			{
			public:

				
				/** @brief Initializes the resource areas for the Computes constant updates.
				 *  @param resourceBuffersInGarlic Array of resource buffers to be used by the ComputeCUE.
				 *  @param resourceBufferCount Number of resource buffers created.
				 *  @param resourceBufferSizeInDwords Size of each resource buffer.
				 *  @param globalInternalResourceTableAddr A pointer to the global resource table in memory.
				 */
				void init(uint32_t** resourceBuffersInGarlic, int32_t resourceBufferCount, int32_t resourceBufferSizeInDwords, void* globalInternalResourceTableAddr);

				
				/** @brief Swap LCUE's ComputeCUE buffers for the next frame. */
				void swapBuffers();

				
				/** @brief Sets the pointer to the dispatch command buffer.
				 *  @param dcb Pointer to the dispatch command buffer.
				 */
				SCE_GNM_FORCE_INLINE void setDispatchCommandBuffer(sce::Gnm::DispatchCommandBuffer* dcb) { m_dcb = dcb; };

				
				/** @brief Binds a CS shader to the CS stage.
				 *  @param shader Pointer to the CS shader.
				 *  @param table Matching ShaderResourceOffsets table created by generateShaderResourceOffsetTable.
				 *  @note Only the pointer is cached inside the LCUE; the location and contents of <c><i>*shader</i></c> must not change until the GPU has completed the dispatch!
				 *	@note This binding will not take effect on the GPU until preDispatch() is called.
				 *  @note This function must be called first before any resource bindings calls. If setCsShader() is called again, all resource bindings for the stage will need to be re-bound.
				 */
				void setCsShader(const sce::Gnmx::CsShader* shader, const InputResourceOffsets* table);

	
				/** @brief Specifies a range of the Global Data Store to be used by shaders for atomic global counters such as those
				 *  used to implement PSSL <c>AppendRegularBuffer</c> and <c>ConsumeRegularBuffer</c> objects.
				 *
				 *  Each counter is a 32-bit integer. The counters for this CS shader stage may have a different offset in GDS. For example:
				 *  @code
				 *     setAppendConsumeCounterRange(0x0400, 4)  // Set up 1 counter for the CS stage at offset 0x400.
				 *	@endcode
				 *
				 *  The index is defined by the chosen slot in the PSSL shader. For example:
				 *  @code
				 *     AppendRegularBuffer<uint> appendBuf : register(u3) // Will access the 4th counter starting at the base offset provided to this function.
				 *  @endcode
				 *
				 *  This function never rolls the hardware context.
				 *
				 *  @param gdsMemoryBaseInBytes The byte offset to the start of the counters in GDS. Must be a multiple of 4.
				 *  @param countersSizeInBytes The size of the counter range in bytes. Must be a multiple of 4.
				 *  @note GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes. 
				 */
				void setAppendConsumeCounterRange(uint32_t gdsMemoryBaseInBytes, uint32_t countersSizeInBytes);


				/** @brief Binds one or more constant buffer objects to the CS Shader stage.
				 *  @param startApiSlot The first API slot to bind to. Valid slots are [0..LCUE::kMaxConstantBufferCount-1].
				 *  @param apiSlotCount The number of consecutive API slots to bind.
				 *  @param buffer The constant buffer objects to bind to the specified slots.
				 *	buffer[0] will be bound to <c><i>startApiSlot</i></c>, buffer[1] to <c><i>startApiSlot+1</i></c>, and so on.
				 *	The contents of these Buffer objects are cached locally inside the LCUE's scratch buffer.
				 *  @note This binding will not take effect on the GPU until preDispatch() is called.
				 */
				void setConstantBuffers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Buffer* buffer);

				
				/** @brief Binds one or more read-only buffer objects to the CS Shader stage. 
				 *  @param startApiSlot The first API slot to bind to. Valid slots are [0..LCUE::kMaxResourceCount-1].
				 *  @param apiSlotCount The number of consecutive API slots to bind.
				 *  @param buffer The buffer objects to bind to the specified slots.
				 *	buffer[0] will be bound to <c><i>startApiSlot</i></c>, buffer[1] to <c><i>startApiSlot+1</i></c>, and so on. 
				 *	The contents of these Buffer objects are cached locally inside the LCUE's scratch buffer.
				 *  @note Buffers and Textures share the same pool of API slots.
				 *  @note This binding will not take effect on the GPU until preDispatch() is called.
				 */
				void setBuffers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Buffer* buffer);

				
				/** @brief Binds one or more read/write buffer objects to the CS Shader stage.
				 *  @param startApiSlot The first API slot to bind to. Valid slots are [0..LCUE::kMaxRwResourceCount-1].
				 *  @param apiSlotCount The number of consecutive API slots to bind.
				 *  @param buffer The rwbuffer objects to bind to the specified slots. 
				 *	buffer[0] will be bound to <c><i>startApiSlot</i></c>, buffer[1] to <c><i>startApiSlot+1</i></c>, and so on. 
				 *	The contents of these Buffer objects are cached locally inside the LCUE's scratch buffer.
				 *  @note rwBuffers and rwTextures share the same pool of API slots.
				 *  @note This binding will not take effect on the GPU until preDispatch() is called.
				 */
				void setRwBuffers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Buffer* buffer);

				
				/** @brief Binds one or more read-only texture objects to the CS Shader stage. 
				 *  @param startApiSlot The first API slot to bind to. Valid slots are [0..LCUE::kMaxResourceCount-1].
				 *  @param apiSlotCount The number of consecutive API slots to bind.
				 *  @param texture The texture objects to bind to the specified slots. 
				 *  texture[0] will be bound to <c><i>startApiSlot</i></c>, texture[1] to <c><i>startApiSlot+1</i></c>, and so on. 
				 *  The contents of these texture objects are cached locally inside the LCUE's scratch buffer.
				 *  @note Buffers and Textures share the same pool of API slots.
				 *  @note This binding will not take effect on the GPU until preDispatch() is called.
				 */
				void setTextures(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Texture* texture);


				/** @brief Binds one or more read/write texture objects to the CS Shader stage. 
				 *  @param startApiSlot The first API slot to bind to. Valid slots are [0..LCUE::kMaxRwResourceCount-1].     
				 *  @param apiSlotCount The number of consecutive API slots to bind.
				 *  @param texture The rwtexture objects to bind to the specified slots. 
				 *  texture[0] will be bound to <c><i>startApiSlot</i></c>, texture[1] to <c><i>startApiSlot+1</i></c>, and so on. 
				 *  The contents of these texture objects are cached locally inside the LCUE's scratch buffer.
				 *  @note rwBuffers and rwTextures share the same pool of API slots.
				 *  @note This binding will not take effect on the GPU until preDispatch() is called.
				 */
				void setRwTextures(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Texture* texture);

				
				/** @brief Binds one or more sampler objects to the CS Shader stage. 
				 *  @param startApiSlot The first API slot to bind to. Valid slots are [0..LCUE::kMaxSamplerCount-1].
				 *  @param apiSlotCount The number of consecutive API slots to bind.
				 *  @param sampler The sampler objects to bind to the specified slots. 
				 *  sampler[0] will be bound to <c><i>startApiSlot</i></c>, sampler[1] to <c><i>startApiSlot+1</i></c>, and so on. 
				 *  The contents of these Sampler objects are cached locally inside the LCUE's scratch buffer.
				 *  @note This binding will not take effect on the GPU until preDispatch() is called.
				 */
				void setSamplers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Sampler* sampler);


				/** @brief Binds a user SRT buffer to the CS Shader stage.
				 *  @param buffer Pointer to the buffer. If NULL, <c><i>bufSizeInDwords</i></c> must be 0.
				 *  @param sizeInDwords Size of the data pointed to by <c><i>buffer</i></c> in dwords. Valid range is [1..kMaxSrtUserDataCount] if <c><i>buffer</i></c> is non-NULL.
				 *  @note This binding will not take effect on the GPU until preDispatch() is called.
				 */
				void setUserSrtBuffer(const void* buffer, uint32_t sizeInDwords);

				
				////////////// Dispatch commands
				
				/** @brief Inserts a compute shader dispatch with the indicated number of thread groups.
				 * 	This function never rolls the hardware context.
				 *	@param threadGroupX Number of thread groups dispatched along the X dimension.
				 *	@param threadGroupY Number of thread groups dispatched along the Y dimension.
				 *	@param threadGroupZ Number of thread groups dispatched along the Z dimension.
				 */
				SCE_GNM_FORCE_INLINE void dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
				{
					#if !defined(SCE_GNM_OFFLINE_MODE)
					preDispatch();
					#endif // !defined(SCE_GNM_OFFLINE_MODE)
					m_dcb->dispatch(threadGroupX, threadGroupY, threadGroupZ);
				}

				
				/** @brief Executes all previous enqueued resource and shader bindings in preparation for a dispatch call.
				 *  Dirty resource bindings will be flushed from the internal scratch buffer, and committed to the resource buffer
				 *  @note When using the Lightweight Constant Update Engine to manage shaders and shader resources, this function must be called 
				 *  immediately before every dispatch call
				 */
				void preDispatch();
			
	#if !defined(DOXYGEN_IGNORE)

				SCE_GNM_LCUE_NOT_SUPPORTED
				void setInternalSrtBuffer(const void* buffer){SCE_GNM_UNUSED(buffer);};
				
				SCE_GNM_API_REMOVED("dispatchIndirect API has been removed, see technote https://ps4.scedev.net/technotes/view/175/1")
				void dispatchIndirect(uint32_t dataOffsetInBytes){SCE_GNM_UNUSED(dataOffsetInBytes);};

			protected:

				uint32_t							m_scratchBuffer[kComputeScratchBufferSizeInDwords];
				sce::Gnm::DispatchCommandBuffer*	m_dcb;

				const void*		m_boundShader;
				uint32_t		m_boundShaderAppendConsumeCounterRange;
				const InputResourceOffsets* m_boundShaderResourceOffsets;
				
				bool m_dirtyShader;
				bool m_dirtyShaderResources;

		#if defined SCE_GNM_LCUE_VALIDATE_COMPLETE_RESOURCE_BINDING_ENABLED
				mutable ShaderResourceBindingValidation m_boundShaderResourcesValidation;
		#endif

			private:

				SCE_GNM_FORCE_INLINE uint32_t* flushScratchBuffer();
				SCE_GNM_FORCE_INLINE void updateCommonPtrsInUserDataSgprs(const uint32_t* resourceBufferFlushedAddress);
				SCE_GNM_FORCE_INLINE void updateEmbeddedCb(const sce::Gnmx::ShaderCommonData* shaderCommon);

				void setUserData(int32_t startSgpr, int32_t sgprCount, const uint32_t* data);
				void setPtrInUserData(int32_t startSgpr, const void* gpuAddress);
	#endif // !defined(DOXYGEN_IGNORE)
			};
		} // LightweightConstantUpdateEngine
	} // Gnmx
} // sce

#endif // _SCE_GNMX_LWCOMPUTECONSTANTUPDATEENGINE_H