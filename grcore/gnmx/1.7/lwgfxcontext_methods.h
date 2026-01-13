/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#if !defined(_SCE_GNMX_LCUE_GFXCONTEXT_METHODS_H)
#define _SCE_GNMX_LCUE_GFXCONTEXT_METHODS_H

#if !defined(DOXYGEN_IGNORE)

#undef _SCE_GNMX_GFXCONTEXT_METHODS_H
#include "grcore/gnmx/gfxcontext_methods.h"
#undef _SCE_GNMX_GFXCONTEXT_METHODS_H

#endif


/** @brief Resets the Gnm::DrawCommandBuffer, and swaps buffers in the LCUE for a new frame.
	Call this at the beginning of every frame.
	The Gnm::DrawCommandBuffer will be reset to empty (<c>m_cmdptr = m_beginptr</c>).
*/
void reset()
{
	m_dcb.resetBuffer();
	swapBuffers();
}

#if !defined(DOXYGEN_IGNORE)

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
	<li><c>setScanModeControl(kScanModeControlAaDisable, kScanModeControlViewportScissorDisable, kScanModeControlLineStippleDisable);</c></li>
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
	m_dcb.initializeDefaultHardwareState();
}


/** @brief Sets the index size (16 or 32 bits). 
	All future draw calls that reference index buffers will use this index size to interpret their contents.
	This function will roll the hardware context.
	@param indexSize Index size to set.
	@cmdsize 2
*/
void setIndexSize(Gnm::IndexSize indexSize)
{
	return m_dcb.setIndexSize(indexSize);
}


/** @brief Sets the base address where the indices are located for functions that do not specify their own indices.
	This function never rolls the hardware context.
	@param indexAddr Address of the index buffer. Must be 2-byte aligned.
	@see drawIndexIndirect, drawIndexOffset
	@cmdsize 3
*/
void setIndexBuffer(const void * indexAddr)
{
	return m_dcb.setIndexBuffer(indexAddr);
}


/** @brief Sets the render target for the specified RenderTarget slot.

	This wrapper automatically works around a rare hardware quirk involving CMASK cache
	corruption with multiple render targets whose FMASK pointers are identical (for example, if they are all NULL).

	This function will roll the hardware context.
	@param rtSlot RenderTarget slot index to which this render target is set to (0-7).
	@param target  Pointer to a Gnm::RenderTarget structure. If NULL is passed, the color buffer for this slot is disabled.
	@sa Gnm::RenderTarget::disableFmaskCompressionForMrtWithCmask
*/
void setRenderTarget(uint32_t rtSlot, sce::Gnm::RenderTarget const *target)
{
	if (target == NULL)
		return m_dcb.setRenderTarget(rtSlot, NULL);

	if (target->getCmaskFastClearEnable() && target->getCmaskAddress256ByteBlocks() != 0 && 
		target->getFmaskAddress256ByteBlocks() == 0)
	{
		sce::Gnm::RenderTarget rtCopy = *target;
		rtCopy.disableFmaskCompressionForMrtWithCmask();
		target = &rtCopy;
	}
	m_dcb.setRenderTarget(rtSlot, target);
}


/** @brief Specifies information for multiple vertex geometry tessellator (VGT) configurations.
	This function will roll the hardware context.
	@param primGroupSize	Number of primitives sent to one VGT block before switching to the next block. It has an implied +1 value. That is, 0 = 1 primitive/group, and 255 = 256 primitives/group.
									For tessellation, <c><i>primGroupSize</i></c> should be set to the number of patches per thread group minus 1.
	@param partialVsWaveMode	If enabled, then the VGT will issue a VS-stage wavefront as soon as a primitive group is finished. Otherwise the VGT will continue a VS-stage wavefront from one primitive group to next
			                         		primitive group within a draw call. This must be enabled for streamout.
	@param switchOnEopMode	If enabled, the IA will switch between VGTs at packet boundaries. If disabled, it will switch on <c><i>primGroupSize</i></c>.
	@note This setting is not used by dispatchDraw, which overrides the setting to (29, kVgtPartialVsWaveEnable, kVgtSwitchOnEopEnable).  The last set value is restored by the first draw command after a dispatchDraw.
	@cmdsize 3
*/
void setVgtControl(uint8_t primGroupSize, Gnm::VgtPartialVsWaveMode partialVsWaveMode, Gnm::VgtSwitchOnEopMode switchOnEopMode)
{
	m_dcb.setVgtControl(primGroupSize, partialVsWaveMode, switchOnEopMode);
}


/** @brief Writes the specified 64-bit value to the given location in memory when this command reaches the end of the processing pipe (EOP).

	This function never rolls the hardware context.
	@param eventType   Determines when <c><i>immValue</i></c> will be written to the specified address.
	@param dstGpuAddr     GPU relative address to which the given value will be written. Must be 8-byte aligned.
	@param immValue       Value that will be written to <c><i>dstGpuAddr</i></c>.
	@param cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
	@sa Gnm::DrawCommandBuffer::writeAtEndOfPipe
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
	@param eventType   Determines when <c><i>immValue</i></c> will be written to the specified address.
	@param dstGpuAddr     GPU relative address to which the given value will be written. Must be 8-byte aligned.
	@param immValue       Value that will be written to <c><i>dstGpuAddr</i></c>.
	@param cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
	@sa Gnm::DrawCommandBuffer::writeAtEndOfPipe
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
	@param eventType   Determines when the counter value will be written to the specified address.
	@param dstGpuAddr     GPU relative address to which the counter value will be written. Must be 8-byte aligned.
	@param cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
	@sa Gnm::DrawCommandBuffer::writeAtEndOfPipe
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
	@param eventType   Determines when <c><i>immValue</i></c> will be written to the specified address.
	@param dstGpuAddr     GPU relative address to which the given value will be written. Must be 8-byte aligned.
	@param immValue       Value that will be written to <c><i>dstGpuAddr</i></c>.
	@param cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
	@sa Gnm::DrawCommandBuffer::writeAtEndOfPipeWithInterrupt
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
	@param eventType   Determines when the counter value will be written to the specified address.
	@param dstGpuAddr     GPU relative address to which the counter value will be written. Must be 8-byte aligned.
	@param cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
	@sa Gnm::DrawCommandBuffer::writeAtEndOfPipeWithInterrupt
	@cmdsize 6
*/
void writeTimestampAtEndOfPipeWithInterrupt(Gnm::EndOfPipeEventType eventType, void *dstGpuAddr, Gnm::CacheAction cacheAction)
{
	return m_dcb.writeAtEndOfPipeWithInterrupt(eventType,
													Gnm::kEventWriteDestMemory, dstGpuAddr,
													Gnm::kEventWriteSourceGpuCoreClockCounter, 0,
													cacheAction, Gnm::kCachePolicyLru);
}


/** @brief Submits the DrawCommandBuffer and ConstantCommandBuffer.
	@note This function is not available in offline mode.
			To enable auto-validation on submit(), please enable the "GPU Validation" option in the Target Manager Settings.
	@return A code indicating the submission status.
*/
int32_t submit()
{
	void* commandBuffer1GpuAddress = m_dcb.m_beginptr;
	uint32_t commandBuffer1Size = (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t);
	return Gnm::submitCommandBuffers(1, &commandBuffer1GpuAddress, &commandBuffer1Size, 0, 0);
}


/** @brief Submits the DrawCommandBuffer and ConstantCommandBuffer and immediately requests a flip.
	@note This function is not available in offline mode.
	@param videoOutHandle  Video out handle.
	@param rtIndex         RenderTarget index to flip to.
	@param flipMode        Flip mode.
	@param flipArg         Flip argument.
	@return A code indicating the error status.
	@note To enable auto-validation on submit(), please enable the "GPU Validation" option in the Target Manager Settings.
*/
int32_t submitAndFlip(uint32_t videoOutHandler, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg)
{
	m_dcb.prepareFlip();

	void* commandBuffer1GpuAddress = m_dcb.m_beginptr;
	uint32_t commandBuffer1Size = (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t);
	return Gnm::submitAndFlipCommandBuffers(1, &commandBuffer1GpuAddress, &commandBuffer1Size, 0, 0, videoOutHandler, rtIndex, flipMode, flipArg);
}


/** @brief Runs validation on the DrawCommandBuffer and ConstantCommandBuffer without submitting them.
	@note This function is not available in offline mode.
	@return A code indicating the validation status.
*/
int32_t validate()
{
	void* commandBuffer1GpuAddress = m_dcb.m_beginptr;
	uint32_t commandBuffer1Size = (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t);
	return Gnm::validateCommandBuffers(1, &commandBuffer1GpuAddress, &commandBuffer1Size, 0, 0);
}


// ----------------------------------------------------------------------------------------------------------------------------------
// Gnmx Helpers
// ----------------------------------------------------------------------------------------------------------------------------------

/** @brief Wrapper around <c>dmaData()</c> to clear the values of one or more append or consume buffer counters to the specified value.

	This function never rolls the hardware context.
	@param destRangeByteOffset Byte offset in GDS to the beginning of the counter range to clear. Must be a multiple of 4.
	@param startApiSlot Index of the first <c>RW_Buffer</c> API slot whose counter should be updated. Valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
	@param numApiSlots Number of consecutive slots to update. <c><i>startApiSlot</i> + <i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
	@param clearValue The value to set the specified counters to.
	@see Gnm::DispatchCommandBuffer::dmaData, Gnm::DrawCommandBuffer::dmaData
	@note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
*/
void clearAppendConsumeCounters(uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, uint32_t clearValue)
{
	return Gnmx::clearAppendConsumeCounters(&m_dcb, destRangeByteOffset, startApiSlot, numApiSlots, clearValue);
}


/** @brief Uses the CP DMA to transfer data from a source address to a destination address.
	This function never rolls the hardware context.
	@param dstGpuAddr          Destination address to write the data to.
	@param srcGpuAddr          Source address to read the data from.
	@param numBytes            Number of bytes to transfer over.
	@param isBlocking          If true, the CP waits for the DMA to be complete before performing any more processing.
*/
void copyData(void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	return Gnmx::copyData(&m_dcb, dstGpuAddr, srcGpuAddr, numBytes, isBlocking);
}


/** @brief Inserts user data directly inside the command buffer returning a locator for later reference.
	This function never rolls the hardware context.
	@param dataStream  Pointer to the data.
	@param sizeInDword Size of the data in stride of 4. Note that the maximum size of a single command packet is 2^16 bytes,
						and the effective maximum value of <c><i>sizeInDword</i></c> will be slightly less than that due to packet headers
						and padding.
	@param alignment   Alignment of the embedded copy in the command buffer.
	@return            Returns a pointer to the allocated buffer.
*/
void* embedData(const void *dataStream, uint32_t sizeInDword, Gnm::EmbeddedDataAlignment alignment)
{
	return Gnmx::embedData(&m_dcb, dataStream, sizeInDword, alignment);
}


/** @brief Uses the CP DMA to clear a buffer to specified value (such as a GPU memset).
	This function never rolls the hardware context.
	@param dstGpuAddr    Destination address to write the data to.
	@param srcData       The value to fill the destination buffer with.
	@param numBytes      Size of the destination buffer.  Must be a multiple of 4.
	@param isBlocking    If true, the CP will block while the transfer is active.
*/
void fillData(void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	return Gnmx::fillData(&m_dcb, dstGpuAddr, srcData, numBytes, isBlocking);
}


/** @brief Wrapper around <c>dmaData()</c> to retrieve the values of one or more append or consume buffer counters and store them in a GPU-visible address.

	This function never rolls the hardware context.
	@param destGpuAddr GPU-visible address to write the counter values to.
	@param srcRangeByteOffset Byte offset in GDS to the beginning of the counter range to read. Must be a multiple of 4.
	@param startApiSlot Index of the first RW_Buffer API slot whose counter should be read. Valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
	@param numApiSlots Number of consecutive slots to read. <c><i>startApiSlot</i> + <i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
	@see Gnm::DispatchCommandBuffer::dmaData, Gnm::DrawCommandBuffer::dmaData
	@note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
*/
void readAppendConsumeCounters(void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots)
{
	return Gnmx::readAppendConsumeCounters(&m_dcb, destGpuAddr, srcRangeByteOffset, startApiSlot, numApiSlots);
}


/** @brief Sets the multisampling sample locations to default values.
	This function will roll hardware context.
	@param numSamples Number of samples used while multisampling.
*/
void setAaDefaultSampleLocations(Gnm::NumSamples numSamples)
{
	return Gnmx::setAaDefaultSampleLocations(&m_dcb, numSamples);
}


/** @brief Utility function that configures the viewport, scissor, and guard band for the provided viewport dimensions.

	If more control is required, users can call the underlying functions manually.
	This function will roll hardware context.
	@param left The X coordinate of the left edge of the rendering surface, in pixels.
	@param top The Y coordinate of the top edge of the rendering surface, in pixels.
	@param right The X coordinate of the right edge of the rendering surface, in pixels.
	@param bottom The Y coordinate of the bottom edge of the rendering surface, in pixels.
	@param zScale Scale value for the Z transform from clip-space to screen-space. The correct value depends on which
				    convention you're following in your projection matrix. For OpenGL-style matrices, use <c><i>zScale</i></c>=0.5. For Direct3D-style
					matrices, use <c><i>zScale</i></c>=1.0.
	@param zOffset Offset value for the Z transform from clip-space to screen-space. The correct value depends on which
					convention you're following in your projection matrix. For OpenGL-style matrices, use <c><i>zOffset</i></c>=0.5. For Direct3D-style
					matrices, use <c><i>zOffset</i></c>=0.0.
*/
void setupScreenViewport(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset)
{
	return Gnmx::setupScreenViewport(&m_dcb, left, top, right, bottom, zScale, zOffset);
}


/** @brief Wrapper around <c>dmaData()</c> to update the values of one or more append or consume buffer counters, using values source from the provided GPU-visible address.

	This function never rolls the hardware context.
	@param destRangeByteOffset Byte offset in GDS to the beginning of the counter range to update. Must be a multiple of 4.
	@param startApiSlot Index of the first <c>RW_Buffer</c> API slot whose counter should be updated. Valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
	@param numApiSlots Number of consecutive slots to update. <c><i>startApiSlot</i> + <i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
	@param srcGpuAddr GPU-visible address to read the new counter values from.
	@see Gnm::DispatchCommandBuffer::dmaData, Gnm::DrawCommandBuffer::dmaData
	@note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
*/
inline void writeAppendConsumeCounters(uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, const void *srcGpuAddr)
{
	return Gnmx::writeAppendConsumeCounters(&m_dcb, destRangeByteOffset, startApiSlot, numApiSlots, srcGpuAddr);
}

#endif // !defined(DOXYGEN_IGNORE)
#endif // _SCE_GNMX_LCUE_GFXCONTEXT_METHODS_H
