/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_COMPUTECONTEXT_H)
#define _SCE_GNMX_COMPUTECONTEXT_H

#include <gnm/constantcommandbuffer.h>
#include <gnm/dispatchcommandbuffer.h>
#include <gnm/offline.h>
#include "grcore/gnmx/constantupdateengine.h"
#include "grcore/gnmx/helpers.h"
#include "grcore/gnmx/lwcomputeconstantupdateengine.h"

namespace sce
{
	namespace Gnmx
	{

		/** @brief Encapsulates a Gnm::DispatchCommandBuffer in a higher-level interface.
		 *
		 * This should be a new developer's main entry point into the PlayStation®4 rendering API.
		 * The ComputeContext object submits to the specified compute ring and only supports compute operations; for graphics tasks, use the GfxContext class.
		 * @see GfxContext
		 */
		class SCE_GNMX_EXPORT ComputeContext 
		{
		public:
			/** @brief Default constructor. */
			ComputeContext(void);
			/** @brief Default destructor. */
			~ComputeContext(void);

			/** @brief Initializes a ComputeContext with application-provided memory buffers.
				@param[out] dcbBuffer A buffer for use by the Gnm::DispatchCommandBuffer.
				@param[in] dcbSizeInBytes The size of <c><i>dcbBuffer</i></c>, in bytes.
				*/
			void init(void *dcbBuffer, uint32_t dcbSizeInBytes);


			/** @brief Initializes a ComputeContext with application-provided memory buffers including LCUE buffers.
			 *
			 *  @param[in] dcbBuffer						A buffer for use by the Gnm::DispatchCommandBuffer.
			 *  @param[in] dcbBufferSizeInDwords			The size of <c><i>dcbBuffer</i></c> in DWORDS.
			 *  @param[in] resourceBufferInGarlic			A resource buffer for use by the LCUE.
			 *  @param[in] resourceBufferSizeInDwords		The size of <c><i>resourceBufferInGarlic</i></c> in DWORDS.
			 *  @param[in] globalInternalResourceTableAddr	A pointer to the global resource table in memory.
			 */
#if !defined(SCE_GNM_OFFLINE_MODE)
			void init(uint32_t* dcbBuffer, uint32_t dcbBufferSizeInDwords, uint32_t* resourceBufferInGarlic, int32_t resourceBufferSizeInDwords, uint32_t* globalInternalResourceTableAddr); 
#endif // !defined(SCE_GNM_OFFLINE_MODE)

			/** @brief Resets the Gnm::DispatchCommandBuffer for a new frame. 
			
			Call this at the beginning of every frame. The Gnm::DispatchCommandBuffer will be reset to empty (<c>m_cmdptr = m_beginptr</c>).
			*/
			void reset(void);

#if !defined(SCE_GNM_OFFLINE_MODE)

			/** @brief Binds a CS shader to the CS stage.
			 *
			 *  @param[in] shader			The pointer to the CS shader.
			 *
			 *  @note Only the pointer is cached inside the LCUE; the location and contents of <c>*shader</c> must not change until the GPU has completed the dispatch!
			 *	@note This binding will not take effect on the GPU until preDispatch() is called.
			 *  @note This function must be called first before any resource bindings calls. If setCsShader() is called again, all resource bindings for the stage will need to be rebound.
			 *  @note This function will regenerate the InputResourceOffsets table on every binding. It is not recommended to call this function regularly; instead cache the table and call setCsShader() with 
			 *  an InputResourceOffsets table explicitly.
			 */
			void setCsShader(const sce::Gnmx::CsShader* shader)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				LightweightConstantUpdateEngine::generateInputResourceOffsetTable(&m_boundInputResourceOffsets, Gnm::kShaderStageCs, shader);
				setCsShader(shader, &m_boundInputResourceOffsets);
			}

			/** @brief Binds a CS shader to the CS stage using an explicitly specified InputResourceOffsets table.
			 *
			 *  @param[in] shader			The pointer to the CS shader.
			 *	@param[in] table Matching	The ShaderResourceOffsets table created by generateShaderResourceOffsetTable().

			 *  @note Only the pointer is cached inside the LCUE; the location and contents of <c>*shader</c> must not change until the GPU has completed the dispatch!
			 *	@note This binding will not take effect on the GPU until preDispatch() is called.
			 *  @note This function must be called first before any resource bindings calls. If setCsShader() is called again, all resource bindings for the stage will need to be rebound.
			 */
			void setCsShader(const sce::Gnmx::CsShader* shader, const LightweightConstantUpdateEngine::InputResourceOffsets* table)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setCsShader(shader, table);
			}

			/** @brief Specifies a range of the Global Data Store to be used by shaders for atomic global counters such as those
			 *  used to implement PSSL <c>AppendRegularBuffer</c> and <c>ConsumeRegularBuffer</c> objects.
			 *
			 *  Each counter is a 32-bit integer. The counters for this CS shader stage may have a different offset in GDS. For example:
			 *
			 *  @code
			 *     setAppendConsumeCounterRange(0x0400, 4)  // Set up 1 counter for the CS stage at offset 0x400.
			 *	@endcode
			 *
			 *  The index is defined by the chosen slot in the PSSL shader. For example:
			 *
			 *  @code
			 *     AppendRegularBuffer<uint> appendBuf : register(u3) // Will access the 4th counter starting at the base offset provided to this function.
			 *  @endcode
			 *
			 *  This function never rolls the hardware context.
			 *
			 *  @param[in] gdsMemoryBaseInBytes		The byte offset to the start of the counters in GDS. This must be a multiple of 4.
			 *  @param[in] countersSizeInBytes		The size of the counter range in bytes. This must be a multiple of 4.
			 *
			 *  @note GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes(). 
			 */
			void setAppendConsumeCounterRange(uint32_t gdsMemoryBaseInBytes, uint32_t countersSizeInBytes)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setAppendConsumeCounterRange(gdsMemoryBaseInBytes, countersSizeInBytes);
			}

			/** @brief Binds one or more constant buffer objects to the CS Shader stage.
			 *
			 *  @param startApiSlot		The first API slot to bind to. Valid slots are [0..LCUE::kMaxConstantBufferCount-1].
			 *  @param apiSlotCount		The number of consecutive API slots to bind.
			 *  @param buffer			The constant buffer objects to bind to the specified slots. <c>buffer[0]</c> will be bound to <c><i>startApiSlot</i></c>,
			 *							<c>buffer[1]</c> to <c><i>startApiSlot+1</i></c> and so on. The contents of these Buffer objects are cached locally inside the LCUE's scratch buffer.
			 */
			void setConstantBuffers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Buffer* buffer)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setConstantBuffers(startApiSlot, apiSlotCount, buffer);
			}

			/** @brief Binds one or more read-only buffer objects to the CS Shader stage.
			 *
			 *  @param[in] startApiSlot		The first API slot to bind to. Valid slots are [0..LCUE::kMaxResourceCount-1].
			 *  @param[in] apiSlotCount		The number of consecutive API slots to bind.
			 *  @param[in] buffer			The read-only buffer objects to bind to the specified slots. <c>buffer[0]</c> will be bound to <c><i>startApiSlot</i></c>,
			 *							<c>buffer[1]</c> to <c><i>startApiSlot+1</i></c> and so on. The contents of these Buffer objects are cached locally inside the LCUE's scratch buffer.
			 *
			 *  @note Buffers and Textures share the same pool of API slots.
			 */
			void setBuffers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Buffer* buffer)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setBuffers(startApiSlot, apiSlotCount, buffer);
			}
				
			/** @brief Binds one or more read/write buffer objects to the CS Shader stage.
			 *
			 *  @param[in] startApiSlot			The first API slot to bind to. Valid slots are [0..LCUE::kMaxRwResourceCount-1].
			 *  @param[in] apiSlotCount			The number of consecutive API slots to bind.
			 *  @param[in] buffer				The read/write buffer objects to bind to the specified slots. <c>buffer[0]</c> will be bound to <c><i>startApiSlot</i></c>,
			 *								<c>buffer[1]</c> to <c><i>startApiSlot+1</i></c> and so on. The contents of these Buffer objects are cached locally inside the LCUE's scratch buffer.
			 *
			 *  @note Buffers and Textures share the same pool of API slots.
			 */
			void setRwBuffers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Buffer* buffer)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setRwBuffers(startApiSlot, apiSlotCount, buffer);
			}

			/** @brief Binds one or more read-only texture objects to the CS Shader stage.
			 *
			 *  @param[in] startApiSlot			The first API slot to bind to. Valid slots are [0..LCUE::kMaxResourceCount-1].
			 *  @param[in] apiSlotCount			The number of consecutive API slots to bind.
			 *  @param[in] texture				The read-only texture objects to bind to the specified slots. <c>texture[0]</c> will be bound to <c><i>startApiSlot</i></c>,
			 *								<c>texture[1]</c> to <c><i>startApiSlot+1</i></c> and so on. The contents of these texture objects are cached locally inside the LCUE's scratch buffer.
			 *
			 *  @note Buffers and Textures share the same pool of API slots.
			 */
			void setTextures(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Texture* texture)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setTextures(startApiSlot, apiSlotCount, texture);
			}

			/** @brief Binds one or more read/write texture objects to the CS Shader stage.
			 *
			 *  @param[in] startApiSlot			The first API slot to bind to. Valid slots are [0..LCUE::kMaxRwResourceCount-1].     
			 *  @param[in] apiSlotCount			The number of consecutive API slots to bind.
			 *  @param[in] texture				The read/write texture objects to bind to the specified slots. <c>texture[0]</c> will be bound to <c><i>startApiSlot</i></c>,
			 *								<c>texture[1]</c> to <c><i>startApiSlot+1</i></c> and so on. The contents of these texture objects are cached locally inside the LCUE's scratch buffer.
			 *
			 *  @note Buffers and Textures share the same pool of API slots.
			 */
			void setRwTextures(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Texture* texture)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setRwTextures(startApiSlot, apiSlotCount, texture);
			}

			/** @brief Binds one or more sampler objects to the CS Shader stage.
			 *
			 *  @param[in] startApiSlot			The first API slot to bind to. Valid slots are [0..LCUE::kMaxSamplerCount-1].
			 *  @param[in] apiSlotCount			The number of consecutive API slots to bind.
			 *  @param[in] sampler				The sampler objects to bind to the specified slots. <c>sampler[0]</c> will be bound to <c><i>startApiSlot</i></c>,
			 *								<c>sampler[1]</c> to <c><i>startApiSlot+1</i></c> and so on. The contents of these Sampler objects are cached locally inside the LCUE's scratch buffer.
			 */	
			void setSamplers(int32_t startApiSlot, int32_t apiSlotCount, const sce::Gnm::Sampler* sampler)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setSamplers(startApiSlot, apiSlotCount, sampler);
			}

			/** @brief Binds a user SRT buffer to the CS Shader stage.
			 *
			 *  @param[in] buffer				The pointer to the buffer. If this is set to NULL, <c><i>sizeInDwords</i></c> must be 0.
			 *  @param[in] sizeInDwords			The size of the data pointed to by <c><i>buffer</i></c> in DWORDS.
			 *								The valid range is [1..kMaxSrtUserDataCount] if <c><i>buffer</i></c> is non-NULL.
			 */
			void setUserSrtBuffer(const void* buffer, uint32_t sizeInDwords)
			{
				SCE_GNM_VALIDATE(m_lwcue.m_bufferBegin[0] != NULL, "In order to use this method, the new init function must be called with a non NULL resourceBufferInGarlic.");
				m_lwcue.setUserSrtBuffer(buffer, sizeInDwords);
			}
#endif // !defined(SCE_GNM_OFFLINE_MODE)
			
			/** @brief Inserts a compute shader dispatch with the indicated number of thread groups.
			    
				@param[in] threadGroupX			The number of thread groups dispatched along the X dimension.
			    @param[in] threadGroupY			The number of thread groups dispatched along the Y dimension.
			    @param[in] threadGroupZ			The number of thread groups dispatched along the Z dimension.
			    
				@cmdsize 7
			*/
			void dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
			{
				#if !defined(SCE_GNM_OFFLINE_MODE)
				if (m_UsingLightweightConstantUpdateEngine)
				{
					m_lwcue.preDispatch();
				}
				#endif // !defined(SCE_GNM_OFFLINE_MODE)

				m_dcb.dispatch(threadGroupX, threadGroupY, threadGroupZ);
			}

#if defined(SCE_GNM_OFFLINE_MODE)
			/** @brief Computes the required size of the two output buffers generated when this structure is serialized.
				@note This function is only available in offline mode.
				@param[out] outTempBufferSize The size (in bytes) of the "temp" buffer (containing data used at runtime during initialization) will be written here.
				@param[out] outPersistentBufferSize The size (in bytes) of the "persistent" buffer (containing the command buffer data) will be written here.
				@sa ComputeContext::serializeIntoBuffers()
				*/
			void getSerializedSizes(size_t *outTempBufferSize, size_t *outPersistentBufferSize) const;

			/** @brief Serializes the contents of this object in a format that can be loaded and submitted at runtime by the ComputeContextSubmitOnly class.
				Two buffers are generated during serialization: a "temp" buffer that can be discarded after initialization and a "persistent" buffer
				that contains the actual GPU command buffers.
				@note This function is only available in offline mode.
				@param[out] destTempBuffer The contents of the "temp" buffer will be written here. This buffer must be at least as large as <c><i>tempBufferSize</i></c>.
				@param[in] tempBufferSize The size (in bytes) of <c><i>destTempBuffer</i></c>. Calculated by ComputeContext::getSerializedSizes().
				@param[out] destPersistentBuffer The contents of the persistent buffer will be written here. This buffer must be at least as large as <c><i>persistentBufferSize</i></c>.
				@param[in] persistentBufferSize The size (in bytes) of the <c><i>destPersistentBuffer</i></c>. Calculated by ComputeContext::getSerializedSizes().
				@sa ComputeContext::getSerializedSizes()
				*/
			void serializeIntoBuffers(void *destTempBuffer, size_t tempBufferSize, void *destPersistentBuffer, size_t persistentBufferSize) const;
#endif

			//////////// Dispatch commands

			/** @brief Uses the CP DMA to clear a buffer to specified value (such as a GPU memset).
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
						  @param[in] dataStream    Pointer to the data.
						  @param[in] sizeInDword   Size of the data in stride of 4. Note that the maximum size of a single command packet is 2^16 bytes,
											   and the effective maximum value of <c><i>sizeInDword</i></c> will be slightly less than that due to packet headers
											   and padding.
						  @param[in] alignment     Alignment of the embedded copy in the command buffer.
						  @return Returns a pointer to the allocated buffer.
			*/
			void* embedData(const void *dataStream, uint32_t sizeInDword, Gnm::EmbeddedDataAlignment alignment)
			{
				return Gnmx::embedData(&m_dcb, dataStream, sizeInDword, alignment);
			}

			/**
			* @brief Wrapper around <c>dmaData()</c> to clear the values of one or more append or consume buffer counters to the specified value.
			 * @param[in] destRangeByteOffset Byte offset in GDS to the beginning of the counter range to update. Must be a multiple of 4.
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
			 * @brief Wrapper around <c>dmaData()</c> to update the values of one or more append or consume buffer counters, using values sourced from the provided GPU-visible address.
			 * @param[in] destRangeByteOffset Byte offset in GDS to the beginning of the counter range to update. Must be a multiple of 4.
			 * @param[in] startApiSlot Index of the first <c>RW_Buffer</c> API slot whose counter should be updated. Valid range is <c>[0..Gnm::kSlotCountRwResource-1]</c>.
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

			// auto-generated method forwarding to m_dcb
			#include "computecontext_methods.h"

		public:
			Gnm::DispatchCommandBuffer m_dcb; ///< Dispatch command buffer. Access directly at your own risk!
			LightweightConstantUpdateEngine::ComputeConstantUpdateEngine m_lwcue; ///< Compute based Lightweight Constant Update Engine. Access directly at your own risk!
			LightweightConstantUpdateEngine::InputResourceOffsets m_boundInputResourceOffsets;
			bool m_UsingLightweightConstantUpdateEngine;
#if !defined(DOXYGEN_IGNORE)
			// The following code/data is used to work around the hardware's 4 MB limit on individual command buffers. We use the m_callback
			// fields of m_dcb to detect when either buffer crosses a 4 MB boundary, and save off the start/size of both buffers
			// into the m_submissionRanges array. When submit() is called, the m_submissionRanges array is used to submit each <4MB chunk individually.
			//
			// In order for this code to function properly, users of this class must not modify m_dcb.m_callback!
			// To register a callback that triggers when m_dcb run out of space, use m_bufferFullCallback.
			static const uint32_t kMaxNumStoredSubmissions = 16; // Maximum number of <4MB submissions that can be recorded. Make this larger if you want more; it just means ComputeContext objects get larger.
			const uint32_t *m_currentDcbSubmissionStart; // Beginning of the submission currently being constructed in the DCB
			const uint32_t *m_actualDcbEnd; // Actual end of the m_dcb's data buffer (i.e. dcbBuffer+dcbSizeInBytes/4)
			class SubmissionRange
			{
			public:
				uint32_t m_dcbStartDwordOffset, m_dcbSizeInDwords;
			};
			SubmissionRange m_submissionRanges[kMaxNumStoredSubmissions]; // Stores the range of each previously-constructed submission (not including the one currently under construction)
			uint32_t m_submissionCount; // The current number of stored submissions in m_submissionRanges (again, not including the one currently under construction)

			/** @brief Defines a callback function that is called when the ComputeContext command buffer is out of space. 

				@param[in,out] gfxc			A pointer to the ComputeContext object whose command buffer is out of space.
				@param[in,out] cb			A pointer to the CommandBuffer object that is out of space. This will be <c>cmpc.m_dcb</c>.
				@param[in] sizeInDwords		The size of the unfulfilled CommandBuffer request in dwords.
				@param[in] userData			The user data.
				
				@return A value of true if the requested space is available in cb when the function returns; otherwise false is returned.
			 */
			typedef bool (*BufferFullCallbackFunc)(ComputeContext *gfxc, Gnm::CommandBuffer *cb, uint32_t sizeInDwords, void *userData);

			/** @brief Represents a callback function, which is called when the command buffer is out of space,
					and the data that will be passed to it. */
			class BufferFullCallback
			{
			public:
				BufferFullCallbackFunc m_func;	///< A pointer to the function to call when the command buffer is out of space.
				void *m_userData;				///< The user data to pass to the fucntion referenced by <c><i>m_func</i></c>.
			};

			BufferFullCallback m_cbFullCallback; ///< A callback function (plus user data to pass to it), which is invoked when <c>m_dcb</c>
												 ///< actually runs out of space (as opposed to crossing a 4 MB boundary).
			SCE_GNM_API_DEPRECATED_VAR_MSG("Use m_cbFullCallback instead")
			Gnm::CommandCallback m_bufferFullCallback; // Invoked when m_dcbactually runs out of space (as opposed to crossing a 4 MB boundary)

#endif // !defined(DOXYGEN_IGNORE)
		};

#if !defined(SCE_GNM_OFFLINE_MODE)
		/** @brief Loads a serialized ComputeContext created offline, patches its pointers, and submits it.

		     The ComputeContextSubmitOnly class is a stripped-down variant of ComputeContext. Generally, the sequence of events when using this class is:
		     
		     <b>Offline</b>
		     <ol>
		     <li> Create a ComputeContext, passing 0 as the ConstantUpdateEngine heap address.
		     <li> Use the ComputeContext functions to build GPU command buffers, as usual. Any VRAM address that is passed into the API must be
				      "disguised" using Gnm::CommandBufferPatchTableBuilder::disguiseDwordOffsetAsAddress() or Gnm::CommandBufferPatchTableBuilder::disguiseUniqueIdAsAddress().
					  These disguised offsets or IDs are used to build a patch table that must be applied to the command buffer at runtime, after the final VRAM
					  addresses are known.
			<li> Use ComputeContext::getSerializedSizes() to determine the size of the two serialization output buffers.
			<li> Allocate space for the output buffers, and use ComputeContext::serializeIntoBuffers() to populate them with data.
			<li> Write the serialized data buffers to disk.
			</ol>
			<b>Runtime</b>
			<ol>
			<li> Load the temporary buffer (generated offline by ComputeContext::serializeIntoBuffers()) into system private memory.
			<li> Load the persistent buffer (generated offline by ComputeContext::serializeIntoBuffers()) into shared VRAM.
			<li>  Initialize the submit-only context with ComputeContextSubmitOnly::init().
			<li>  Set the final addresses for any unique-ID patches in <c>m_dcbPatchTable</c> using Gnm::CommandBufferPatchTable::setAddressForPatchId().
			<li>  Apply the finished patch tables to the command buffers with ComputeContextSubmitOnly::patchCommandBuffers(). After the command buffers are patched,
				      the temporary buffer can be safely freed (Note that this will prevent the command buffer from being re-patched later).
			<li>  It is now safe to submit the ComputeContext's command buffer.
			</ol>
			@note This class is not available in offline mode.
			*/
		class ComputeContextSubmitOnly
		{
		public:
			/** @brief Default constructor. */
			ComputeContextSubmitOnly(void);
			/** @brief Default destructor. */
			~ComputeContextSubmitOnly(void);
	
			/** @brief Initializes the submit-only context, using data from the two buffers generated offline by ComputeContext::serializeIntoBuffers().
				@param[in] srcTempBuffer The temporary buffer generated by ComputeContext::serializeIntoBuffers(). This buffer should be in system private memory. It must be aligned to a 4-byte boundary.
				                     It can be freed by the caller after ComputeContextSubmitOnly::patchCommandBuffers() is called.
				@param[in] srcTempBufferSize The size of <c><i>srcTempBuffer</i></c>, in bytes.
				@param[in] srcPersistentBuffer The persistent buffer generated by ComputeContext::serializeIntoBuffers(). This buffer should be in CPU/GPU shared memory. It must be aligned to a 256-byte boundary.
				                           It is the caller's responsibility to free this memory when this context is no longer needed.
				@param[in] srcPersistentBufferSize The size of <c><i>srcPersistentBuffer</i></c>, in bytes.
				*/
			void init(void *srcTempBuffer, uint32_t srcTempBufferSize, void *srcPersistentBuffer, uint32_t srcPersistentBufferSize);

			/** @brief Assigns a final VRAM address to the command buffer patch with the specified unique ID.
				@param[in] id The unique ID of the patch to associate with <c><i>finalAddr</i></c>.
				@param[in] finalAddr The address in VRAM to associate with the specified unique ID when patching the command buffer.
				@return Returns true if the specified ID was located in either <c>m_dcbPatchTable</c>, or false if the ID was not found in
				        either patch table.
				@sa ComputeContextSubmitOnly::patchCommandBuffers(), Gnm::CommandBufferPatchTableBuilder::disguiseUniqueIdAsAddress()
				*/
			bool setAddressForPatchId(uint32_t id, void *finalAddr);

			/** @brief Applies the patches in <c>m_dcbPatchTable</c> to the command buffer (<c>m_dcb</c>).
			    Before this function can be called, all patches with unique IDs must be associated with their final VRAM addresses
					   using ComputeContextSubmitOnly::setAddressForPatchId().
				@note This function must be called before ComputeQueue::submit() can be used.
				@note After this function is called, it is safe for the caller to free the temp buffer passed to ComputeContextSubmitOnly::init().
				@sa ComputeContextSubmitOnly::setAddressForPatchId()
				*/
			void patchCommandBuffers(void);
		public:
			Gnm::DispatchCommandBuffer     m_dcb; ///< Dispatch command buffer. Access directly at your own risk!
			Gnm::CommandBufferPatchTable m_dcbPatchTable; ///< Patch table for <c>m_dcb</c>.
#if !defined(DOXYGEN_IGNORE)
			// The following code/data is used to work around the hardware's 4 MB limit on individual command buffers.
			static const uint32_t kMaxNumStoredSubmissions = 16; // Maximum number of <4MB submissions that can be recorded.
			ComputeContext::SubmissionRange m_submissionRanges[kMaxNumStoredSubmissions]; // Stores the range of each previously-constructed submission.
			uint32_t m_submissionCount; // The current number of stored submissions in m_submissionRanges.
		#endif // !defined(DOXYGEN_IGNORE)
		};
#endif // !defined(SCE_GNM_OFFLINE_MODE)

	}
}

#endif
