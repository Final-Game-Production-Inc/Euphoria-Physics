/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
// This file was auto-generated from dispatchcommandbuffer.h -- do not edit by hand!
// This file should NOT be included directly by any non-Gnmx source files!

#if !defined(_SCE_GNMX_COMPUTECONTEXT_METHODS_H)
#define _SCE_GNMX_COMPUTECONTEXT_METHODS_H

/** @brief Sets up a default hardware state for the compute ring.
				       This function calls flushShaderCachesAndWait() on all caches.
				@cmdsize 256
*/
void initializeDefaultHardwareState()
{
	return m_dcb.initializeDefaultHardwareState();
}

/** @brief Sets a per dispatch mask that determines which compute units are active in the specified shader engine.
			All masks are logical masks, indexed from 0 to Gnm::kNumCusPerSe <c>-1</c> regardless of which physical compute units are working and enabled by the driver.
			@note Only applies to PlayStation®4 targets.
			@param[in] engine Specifies which shader engine should be configured.
			@param[in] mask Mask enabling compute units for the CS shader stage.
			@see Gnm::DrawCommandBuffer::setGraphicsResourceManagement(), Gnmx::GfxContext::setGraphicsResourceManagement()
			@cmdsize 3
*/
void setComputeResourceManagement(Gnm::ShaderEngine engine, uint16_t mask)
{
	return m_dcb.setComputeResourceManagement(engine, mask);
}

/** @brief Sets the shader code to be used for the CS shader stage.
			  @param[in] computeData   Pointer to structure containing memory address (256-byte aligned) of the shader code and additional
			                                   registers to set as determined by the shader compiler.
			  @cmdsize 25
*/
void setCsShader(const Gnm::CsStageRegisters * computeData)
{
	return m_dcb.setCsShader(computeData);
}

/** @brief Sets the shader code to be used for the CS shader stage as a bulky shader.
				Threadgroups with bulky shaders are scheduled to separate CUs.
				Specify a shader to be bulky to prevent a situation when only few threadgroups can fit into single CU because each uses a significant number of registers and/or LDS.
			  @param[in] computeData   Pointer to structure containing the memory address (256-byte aligned) of the shader code and additional
			                                   registers to set as determined by the shader compiler.
			  @cmdsize 25
*/
void setBulkyCsShader(const Gnm::CsStageRegisters * computeData)
{
	return m_dcb.setBulkyCsShader(computeData);
}

/**
* @brief Reads data from global data store (GDS).
* @param[in] eventType Specifies the event used to trigger the GDS read.
* @param[out] dstGpuAddr The destination address where the GDS data should be written. This pointer must be 4-byte aligned and must not be set to NULL.
* @param[in] gdsOffsetInDwords The dword offset into GDS to read from.
* @param[in] gdsSizeInDwords The number of dwords to read.
* @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
* @cmdsize 5
*/
void readDataFromGds(Gnm::EndOfShaderEventType eventType, void * dstGpuAddr, uint32_t gdsOffsetInDwords, uint32_t gdsSizeInDwords)
{
	return m_dcb.readDataFromGds(eventType, dstGpuAddr, gdsOffsetInDwords, gdsSizeInDwords);
}

/** @brief Allocates space for user data directly inside the command buffer, and returns a CPU pointer to the space.
			  @param[in] sizeInBytes          Size of the data in bytes. Note that the maximum size of a single command packet is 2^16 bytes,
			                      and the effective maximum value of <c><i>sizeInBytes</i></c> will be slightly less than that due to packet headers
								  and padding.
			  @param[in] alignment    Alignment from the start of the command buffer.
			  @return Returns a pointer to the memory just allocated. If the requested size is larger than the maximum packet size (64KB),
			          the function returns 0.
			  @cmdsize 2 + (sizeInBytes + sizeof(uint32_t) - 1)/sizeof(uint32_t) + (uint32_t)(1<<alignment)/sizeof(uint32_t)
*/
void * allocateFromCommandBuffer(uint32_t sizeInBytes, Gnm::EmbeddedDataAlignment alignment)
{
	return m_dcb.allocateFromCommandBuffer(sizeInBytes, alignment);
}

/** @brief Sets a buffer resource (<c>V#</c>) in the appropriate shader user data registers.
				@param[in] startUserDataSlot The first user data slot to write to. There are 16 dword-sized user data slots available per shader stage.
				                         A <c>V#</c> occupies four consecutive slots, so the valid range for <c><i>startUserDataSlot</i></c> is <c>[0..12]</c>.
				@param[in] buffer Pointer to a Buffer resource definition.
				@cmdsize 8
*/
void setVsharpInUserData(uint32_t startUserDataSlot, const Gnm::Buffer * buffer)
{
	return m_dcb.setVsharpInUserData(startUserDataSlot, buffer);
}

/** @brief Sets a texture resource (<c>T#</c>) in the appropriate shader user data registers.
				@param[in] startUserDataSlot Starting user data slot. There are 16 dword-sized user data slots available per shader stage.
				                         A <c>T#</c> occupies eight consecutive slots, so the valid range for <c><i>startUserDataSlot</i></c> is <c>[0..8]</c>.
				@param[in] tex Pointer to a Texture resource definition.
				@cmdsize 12
*/
void setTsharpInUserData(uint32_t startUserDataSlot, const Gnm::Texture * tex)
{
	return m_dcb.setTsharpInUserData(startUserDataSlot, tex);
}

/** @brief Sets a sampler resource (<c>S#</c>) in the appropriate shader user data registers.
				@param[in] startUserDataSlot Starting user data slot. There are 16 dword-sized user data slots available per shader stage.
				                         An S# occupies four consecutive slots, so the valid range for <c><i>startUserDataSlot</i></c> is <c>[0..12]</c>.
				@param[in] sampler Pointer to a Sampler resource definition.
				@cmdsize 8
*/
void setSsharpInUserData(uint32_t startUserDataSlot, const Gnm::Sampler * sampler)
{
	return m_dcb.setSsharpInUserData(startUserDataSlot, sampler);
}

/** @brief Sets a GPU pointer in the appropriate shader user data registers.
				@param[in] startUserDataSlot Starting user data slot. There are 16 dword-sized user data slots available per shader stage.
				                         A GPU address occupies two consecutive slots, so the valid range for <c><i>startUserDataSlot</i></c> is <c><i>[0..14]</i></c>.
				@param[in] gpuAddr GPU address to write to the specified slot.
				@cmdsize 4
*/
void setPointerInUserData(uint32_t startUserDataSlot, void * gpuAddr)
{
	return m_dcb.setPointerInUserData(startUserDataSlot, gpuAddr);
}

/** @brief Sets an arbitrary 32-bit integer constant in a shader user data register.
				@param[in] userDataSlot Destination user data slot. There are 16 dword-sized user data slots available per shader stage.
				                    This function sets a single slot, so the valid range for <c><i>userDataSlot</i></c> is <c>[0..15]</c>.
				@param[in] data Data.
				@cmdsize 3
*/
void setUserData(uint32_t userDataSlot, uint32_t data)
{
	return m_dcb.setUserData(userDataSlot, data);
}

/** @brief Copies several contiguous dwords into the CS shader stage's user data registers.
					@param startUserDataSlot	The first slot to write to. The valid range is <c>[0..15]</c>.
					@param userData				The source data to copy into the user data registers. If <c><i>numDwords</i></c> is greater than 0, this pointer must not be NULL.
					@param numDwords			The number of dwords to copy into the user data registers. The sum of <c><i>startUserDataSlot</i></c> and <c><i>numDwords</i></c> must not exceed 16.
					@cmdsize 4+numDwords
*/
void setUserDataRegion(uint32_t startUserDataSlot, const uint32_t * userData, uint32_t numDwords)
{
	return m_dcb.setUserDataRegion(startUserDataSlot, userData, numDwords);
}

/** @brief Specifies how the scratch buffer (compute only) should be subdivided between the executing wavefronts.
				Basically, <c><i>maxNumWaves</i> * <i>num1KByteChunksPerWave</i> * 1024</c> must be less than or equal to the total size of the scratch buffer.
*  @param[in] maxNumWaves Maximum number of wavefronts that could be using the scratch buffer simultaneously.
*                     This should less or equal to 640.
*  @param[in] num1KByteChunksPerWave The amount of scratch buffer space for use by each wavefront. Specified in units of 1024-byte chunks.
*  @cmdsize 3
*/
void setScratchSize(uint32_t maxNumWaves, uint32_t num1KByteChunksPerWave)
{
	return m_dcb.setScratchSize(maxNumWaves, num1KByteChunksPerWave);
}

/** @brief Configures a GDS ordered append unit internal counter to enable special ring buffer counter handling of ds_ordered_count operations targeting a specific GDS address.
			    The GDS ordered append unit supports 16 special allocation counters, which, like GDS, are a global resource that must
				be managed by the application.
				This configures the GDS OA counter <c><i>oaCounterIndex</i></c> ([0:15]) to detect ds_ordered_count operations targeting
				an allocation position counter at GDS address <c><i>gdsDwOffsetOfCounter</i></c>. The options are modified in order
				to prevent ring buffer data overwrites in addition to the wavefront ordering enforced by ds_ordered_count in general.
				The ring buffer total size in arbitrary allocation units <c><i>spaceInAllocationUnits</i></c> is stored in two internal
				GDS registers associated with the GDS OA counter, the constant <c>WRAP_SIZE</c> and the initial value of 
				<c>SPACE_AVAILABLE</c>.
				A given shader wavefront may issue up to 4 ds_ordered_count operations. The GDS ordered append unit queues these
				operations in 4 independent internal queues. This ensures that the operations in each queue are executed in wavefront 
				launch order. The first ds_ordered_count operation issued by each shader wavefront in a dispatch is guaranteed to
				execute in wavefront launch order vs. the first ds_ordered_count operation issued by every other shader wavefront.
				The same guarantee applies to the second, third, and fourth operation issued by each shader wavefront. Arbitrarily
				many shader wavefronts may execute the first operation before any execute the second, or the second before any 
				execute the third, and so on.
				Wavefront launch order for compute dispatches is straight forward. Dispatches create wavefronts ordered by 
				thread group ID and iterate over X in the inner loop, then Y, then Z. Compute shaders must also be dispatched
				with dispatchWithOrderedAppend() in order to enable hardware generation of an ordered wavefront index.
				Any ds_ordered_count operation whose GDS base dword address (M0 bits [18:31]) matches gdsDwOffsetOfCounter will
				be intercepted for special processing by the GDS ordered append unit. If the operation is the 
				<c><i>oaOpIndex</i></c>'th ds_ordered_count operation issued by the wavefront and the wavefront originated from
				the same asynchronous compute pipe_id that enabled the GDS OA counter, the operation is converted into an
				allocation operation. If the operation matches the address but not <c><i>oaOpIndex</i></c> or the compute pipe_id,
				it is converted to a deallocation operation.  Operations matching the address but originating from graphics pipe
				shader wavefronts are also converted to deallocation operations.
				Any ds_ordered_count operation which is converted to either an allocation or deallocation discards the 
				ds_ordered_count immediate offset (OFFSET0 instruction modifier) and treats it as 0.
				Any ds_ordered_count operation converted to an allocation operation will first wait until the internal
				<c>SPACE_AVAILABLE</c> register value is larger than the requested allocation size before decrementing the
				internal register value. In addition, some specific ds_ordered_count operations, such as WRAP and CONDXCHG32,
				receive some additional pre-processing of the allocation count to produce source operands to the internal
				atomic operations. See the ISA documentation for ds_ordered_count for more details.
				Any ds_ordered_count operation converted to a deallocation operation will increment the internal 
				<c>SPACE_AVAILABLE</c> register value and discard the GDS atomic operation entirely.
				Together, these allocation and deallocation operations allow a shader running in one compute pipe or graphics stage
				to write data into a ring buffer. This data is consumed by another shader running in a different compute pipe or graphics 
				stage. Allocations and writes are performed in wavefront launch order for the source pipe or stage, and reads and 
				deallocations are performed in wavefront launch order for the destination pipe or stage. Typically, an additional 
				source pipe wavefront launch ordered GDS counter is required to indicate the completion of writes, which requires
				another ds_ordered_count but not another OA counter.
				Dispatch draw requires one counter to be configured for the index ring buffer and, if enabled, a second counter
				to be configured for the vertex ring buffer.
				
				@param[in] oaCounterIndex			The index of one of the 16 available GDS ordered append unit internal counters that should be configured; range [0:15].
				@param[in] gdsDwOffsetOfCounter		The dword offset of the allocation offset counter in GDS which will be matched against ds_ordered_count M0[18:31] base addresses.
				@param[in] oaOpIndex				The ds_ordered_count operation index which will be interpreted as an allocation operation if also issued from this compute pipe_id.
				@param[in] spaceInAllocationUnits	The size of the ring buffer in arbitrary units (elements).
				@cmdsize 5
*/
void enableOrderedAppendAllocationCounter(uint32_t oaCounterIndex, uint32_t gdsDwOffsetOfCounter, uint32_t oaOpIndex, uint32_t spaceInAllocationUnits)
{
	return m_dcb.enableOrderedAppendAllocationCounter(oaCounterIndex, gdsDwOffsetOfCounter, oaOpIndex, spaceInAllocationUnits);
}

/** @brief Disables a GDS ordered append unit internal counter.
				@param[in] oaCounterIndex		The index of the GDS ordered append unit internal counter to disable; range [0:15].
				@cmdsize 5
*/
void disableOrderedAppendAllocationCounter(uint32_t oaCounterIndex)
{
	return m_dcb.disableOrderedAppendAllocationCounter(oaCounterIndex);
}

/** @brief Copies data inline into the command buffer and uses the command processor to transfer it to a destination GPU address.
			@param[out] dstGpuAddr      Destination address to write the data to. This pointer must not be NULL.
			@param[in] data            Pointer to data to be copied inline.
			@param[in] sizeInDwords    Number of dwords of data to copy.
			@param[in] writeConfirm    Enables/disables write confirmation for this memory write.
			@cmdsize 4+sizeInDwords
*/
void writeDataInline(void * dstGpuAddr, const void * data, uint32_t sizeInDwords, Gnm::WriteDataConfirmMode writeConfirm)
{
	return m_dcb.writeDataInline(dstGpuAddr, data, sizeInDwords, writeConfirm);
}

/** @brief Copies data inline into the command buffer and uses the command processor to transfer it to a destination GPU address through the GPU L2 cache.
			@param[out] dstGpuAddr      Destination address to write the data to. This pointer must not be NULL.
			@param[in] data            Pointer to data to be copied inline.
			@param[in] sizeInDwords    Number of dwords of data to copy.
			@param[in] cachePolicy	   Specifies the cache policy to use, if the data is written to the GPU's L2 cache.
			@param[in] writeConfirm    Enables or disables write confirmation for this memory write.
			@cmdsize 4+sizeInDwords
*/
void writeDataInlineThroughL2(void * dstGpuAddr, const void * data, uint32_t sizeInDwords, Gnm::CachePolicy cachePolicy, Gnm::WriteDataConfirmMode writeConfirm)
{
	return m_dcb.writeDataInlineThroughL2(dstGpuAddr, data, sizeInDwords, cachePolicy, writeConfirm);
}

/** @brief Triggers an event on the GPU.
			The standard cache flushing and output flushing events do not roll the hardware context. It is possible but not likely that other events roll the hardware context.
			@param[in] eventType Type of the event the command processor should wait for.
			@cmdsize 2
*/
void triggerEvent(Gnm::EventType eventType)
{
	return m_dcb.triggerEvent(eventType);
}

/** @brief Requests the GPU to trigger an interrupt upon release memory event.
			This function never rolls the hardware context.
			@param[in] eventType   Determines when interrupt will be triggered.
			@param[in] cacheAction Cache action to perform.
			@cmdsize 7
*/
void triggerReleaseMemEventInterrupt(Gnm::ReleaseMemEventType eventType, Gnm::CacheAction cacheAction)
{
	return m_dcb.triggerReleaseMemEventInterrupt(eventType, cacheAction);
}

/** @brief Writes the specified value to the given location in memory and triggers an interrupt upon release memory event.
			@param[in] eventType   Determines the type of shader to wait for before writing <c><i>immValue</i></c> to <c>*<i>dstGpuAddr</i></c>.
			@param[in] dstSelector Specifies which levels of the memory hierarchy to write to.
			@param[out] dstGpuAddr     GPU address to which <c><i>immValue</i></c> will be written. Must be 4-byte aligned. This pointer must not be NULL.
			@param[in] srcSelector Specifies the type of data to write -- either the provided <c><i>immValue</i></c>, or an internal GPU counter.
			@param[in] immValue       Value that will be written to <c><i>dstGpuAddr</i></c>. If <c><i>srcSelect</i></c> specifies a GPU counter, this argument
			                      will be ignored.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@param[in] writePolicy		Specifies the cache policy to use, if the data is written to the GPU's L2 cache.
			@cmdsize 7
*/
void writeReleaseMemEventWithInterrupt(Gnm::ReleaseMemEventType eventType, Gnm::EventWriteDest dstSelector, void * dstGpuAddr, Gnm::EventWriteSource srcSelector, uint64_t immValue, Gnm::CacheAction cacheAction, Gnm::CachePolicy writePolicy)
{
	return m_dcb.writeReleaseMemEventWithInterrupt(eventType, dstSelector, dstGpuAddr, srcSelector, immValue, cacheAction, writePolicy);
}

/** @brief Writes the specified value to the given location in memory.
			@param[in] eventType   Determines the type of shader to wait for before writing <c><i>immValue</i></c> to <c>*<i>dstGpuAddr</i></c>.
			@param[in] dstSelector Specifies which levels of the memory hierarchy to write to.
			@param[out] dstGpuAddr     GPU address to which <c><i>immValue</i></c> will be written. Must be 4-byte aligned. This pointer must not be NULL.
			@param[in] srcSelector Specifies the type of data to write -- either the provided <c><i>immValue</i></c>, or an internal GPU counter.
			@param[in] immValue       Value that will be written to <c><i>dstGpuAddr</i></c>. If <c><i>srcSelect</i></c> specifies a GPU counter, this argument
			                      will be ignored.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@param[in] writePolicy		Specifies the cache policy to use, if the data is written to the GPU's L2 cache.
			@param[in] writeConfirm Specifies whether or not to confirm the write. Regardless of this value, CP doesn't stall the command buffer.
			@cmdsize 7
*/
void writeReleaseMemEvent(Gnm::ReleaseMemEventType eventType, Gnm::EventWriteDest dstSelector, void * dstGpuAddr, Gnm::EventWriteSource srcSelector, uint64_t immValue, Gnm::CacheAction cacheAction, Gnm::CachePolicy writePolicy)
{
	return m_dcb.writeReleaseMemEvent(eventType, dstSelector, dstGpuAddr, srcSelector, immValue, cacheAction, writePolicy);
}
SCE_GNM_API_CHANGED
void writeReleaseMemEvent(Gnm::ReleaseMemEventType eventType, Gnm::EventWriteDest dstSelector, void * dstGpuAddr, Gnm::EventWriteSource srcSelector, uint64_t immValue, Gnm::CacheAction cacheAction, Gnm::CachePolicy writePolicy, Gnm::WriteDataConfirmMode writeConfirm)
{
	SCE_GNM_UNUSED(writeConfirm);
	return m_dcb.writeReleaseMemEvent(eventType, dstSelector, dstGpuAddr, srcSelector, immValue, cacheAction, writePolicy);
}

/** @brief Blocks frontend processing until indicated test passes.
			The 32-bit value at the specified GPU address is tested against the
			reference value with the test qualified by the specified function and mask.
			Basically, this function tells the GPU to stall until <c><i>compareFunc</i>((*<i>gpuAddr</i> and <i>mask</i>), <i>refValue</i>) == true</c>.
			@param[in] gpuAddr  Address to poll. Must be 4-byte aligned.
			@param[in] mask     Mask to be applied to <c>*<i>gpuAddr</i></c> before comparing to <c><i>refValue</i></c>.
			@param[in] compareFunc Specifies the type of comparison to be done between (<c>*<i>gpuAddr</i></c> and <c><i>mask</i></c>) and the <c><i>refValue</i></c>.
			@param[in] refValue    Expected value of <c>*<i>gpuAddr</i></c>.
			@cmdsize 7
*/
void waitOnAddress(void * gpuAddr, uint32_t mask, Gnm::WaitCompareFunc compareFunc, uint32_t refValue)
{
	return m_dcb.waitOnAddress(gpuAddr, mask, compareFunc, refValue);
}

/** @brief Waits for all PS shader output to one or more targets to complete.
			One can specify the various render target slots
			(color and/or depth,) to be checked within the provided base address and size: all active contexts associated with
			those target can then be waited for. The caller may also optionally specify that certain caches be flushed.
			The kExtendedCacheActionFlushAndInvalidateCbCache and kExtendedCacheActionFlushAndInvalidateDbCache are not allowed in the compute.
			This function may roll the hardware context.
			@note This command will only wait on output written by graphics shaders -- not compute shaders!
			@param[in] baseAddr256     Starting base address (256-byte aligned) of the surface to be synchronized to (high 32 bits of a 40-bit
								   virtual GPU address).
			@param[in] sizeIn256ByteBlocks        Size of the surface. Has a granularity of 256 bytes.
			@param[in] targetMask		Configures which of the source and destination caches should be enabled for coherency. This field is
											composed of individual flags from the #WaitTargetSlot enum.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified writes are complete.
			@param[in] extendedCacheMask Specifies additional caches to flush and invalidate. This field is composed of individual flags from Gnm::ExtendedCacheAction.
			@see Gnm::WaitTargetSlot, Gnm::ExtendedCacheAction, flushShaderCachesAndWait()
			@cmdsize 7
*/
void waitForGraphicsWrites(uint32_t baseAddr256, uint32_t sizeIn256ByteBlocks, uint32_t targetMask, Gnm::CacheAction cacheAction, uint32_t extendedCacheMask)
{
	return m_dcb.waitForGraphicsWrites(baseAddr256, sizeIn256ByteBlocks, targetMask, cacheAction, extendedCacheMask);
}

/** @brief Requests a flush of the specified data cache(s), and waits for the flush operation(s) to complete.
				The kExtendedCacheActionFlushAndInvalidateCbCache and kExtendedCacheActionFlushAndInvalidateDbCache are not allowed in the compute.
				This function may roll the hardware context.
				@note This function is equivalent to calling <c>waitForGraphicsWrites(0,0,0,<i>cacheAction</i>,<i>extendedCacheMask</i>)</c>.
				@param[in] cacheAction      Specifies which caches to flush and invalidate.
				@param[in] extendedCacheMask Specifies additional caches to flush and invalidate. This field is composed of individual flags from Gnm::ExtendedCacheAction.
				@see Gnm::ExtendedCacheAction, waitForGraphicsWrites()
				@cmdsize 7
*/
void flushShaderCachesAndWait(Gnm::CacheAction cacheAction, uint32_t extendedCacheMask)
{
	return m_dcb.flushShaderCachesAndWait(cacheAction, extendedCacheMask);
}

/** @brief Inserts the specified number of dwords in the command buffer as a NOP packet.
			@param[in] numDwords   Number of dwords to insert. The entire packet (including the PM4 header) will be <c><i>numDwords</i></c>.
			                   Valid range is [0..16384].
			@cmdsize numDwords
*/
void insertNop(uint32_t numDwords)
{
	return m_dcb.insertNop(numDwords);
}

/** @brief Sets a marker command in the command buffer that will be used by performance analysis and debug tools.
			The marker command created by this function will be handled as a standalone marker. For a scoped marker block, use pushMarker() and popMarker().
			
			@param[in] debugString   String to be embedded into the command buffer.
			
			@see pushMarker(), popMarker()
			
			@cmdsize 2 + (uint32_t)(strlen(debugString)+1+3)/sizeof(uint32_t)
*/
void setMarker(const char * debugString)
{
	return m_dcb.setMarker(debugString);
}

/** @brief Sets a colored marker command in the command buffer that will be used by the PA/Debug tools.
			The marker command created by this function will be handled as a standalone marker. For a scoped marker block,
			use pushMarker() and popMarker().
			This function never rolls the hardware context.
			
			@param[in]	debugString		The string to be embedded into the command buffer.
			@param[in]	argbColor		The color of the marker.
			@see pushMarker(), popMarker()
			
			@cmdsize 2 + (uint32_t)(strlen(debugString)+1+3)/sizeof(uint32_t)
*/
void setMarker(const char * debugString, uint32_t argbColor)
{
	return m_dcb.setMarker(debugString, argbColor);
}

/** @brief Sets a marker command in the command buffer that will be used by the Performance Analysis and Debug tools.
			The marker command created by this function is handled as the beginning of a scoped marker block.
			Close the block with a matching call to popMarker(). Marker blocks can be nested.
			
			@param[in] debugString   String to be embedded into the command buffer.
			
			@see popMarker()
			
			@cmdsize 2 + (uint32_t)(strlen(debugString)+1+3)/sizeof(uint32_t)
*/
void pushMarker(const char * debugString)
{
	return m_dcb.pushMarker(debugString);
}

/** @brief Sets a colored marker command in the command buffer that will be used by the Performance Analysis and Debug tools.
			The marker command created by this function is handled as the beginning of a scoped marker block.
			Close the block with a matching call to popMarker(). Marker blocks can be nested.
			This function never rolls the hardware context.
			
			@param[in]	debugString		The string to be embedded into the command buffer.
			@param[in]	argbColor		The color of the marker.
			
			@see popMarker()
			@cmdsize 2 + (uint32_t)(strlen(debugString)+1+3)/sizeof(uint32_t)
*/
void pushMarker(const char * debugString, uint32_t argbColor)
{
	return m_dcb.pushMarker(debugString, argbColor);
}

/** @brief Closes the marker block opened by the most recent pushMarker() command.
				@see pushMarker()
				@cmdsize 2
*/
void popMarker()
{
	return m_dcb.popMarker();
}

/** @brief Prefetches data into L2$
	
				@param dataAddr      The Gpu address of the buffer to be preloaded in L2$.
				@param sizeInBytes   The size of the buffer in bytes.
					
				@note  Do not use this for buffers intended to be written to.
*/
void prefetchIntoL2(void * dataAddr, uint32_t sizeInBytes)
{
	return m_dcb.prefetchIntoL2(dataAddr, sizeInBytes);
}

/** @brief Signals a semaphore.
				@param[in] semAddr Address of the semaphore's mailbox (must be 8-byte aligned). This pointer must not be NULL.
				@param[in] behavior Selects between incrementing the mailbox value and setting the mailbox value to 1.
				@param[in] updateConfirm If enabled, the packet waits for the mailbox to be written.
				@cmdsize 3
*/
void signalSemaphore(uint64_t * semAddr, Gnm::SemaphoreSignalBehavior behavior, Gnm::SemaphoreUpdateConfirmMode updateConfirm)
{
	return m_dcb.signalSemaphore(semAddr, behavior, updateConfirm);
}

/** @brief Waits on a semaphore.
				Waits until the value in the mailbox is not 0.
				While waiting on a semaphore, the queue is idle and can be preempted for other queues.
				@param[in] semAddr Address of the semaphore's mailbox (must be 8 bytes aligned). This pointer must not be NULL.
				@param[in] behavior Selects the action to perform when the semaphore opens (mailbox becomes non-zero): either decrement or do nothing.
				@cmdsize 3
*/
void waitSemaphore(uint64_t * semAddr, Gnm::SemaphoreWaitBehavior behavior)
{
	return m_dcb.waitSemaphore(semAddr, behavior);
}

/** @brief Sets queue priority.
				Queue priority can be from 0 through 15 with 15 being the highest priority and 0 the lowest.
				@param[in] queueId The ID of the queue returned by Gnm::mapComputeQueue().
				@param[in] priority The queue priority.
				@cmdsize 3
*/
void setQueuePriority(uint32_t queueId, uint32_t priority)
{
	return m_dcb.setQueuePriority(queueId, priority);
}

/** @brief Enables a queue's quantum timer.
				The quantum timer starts counting down whenever the queue is activated by the scheduler. After it reaches 0, the queue yields to other queues with the same priority.
				The next time the queue becomes active, its quantum timer will be restarted with the same initial value.
				This is useful to fairly distribute compute time between queues with the same priority.
				@param[in] queueId The ID of the queue returned by Gnm::mapComputeQueue().
				@param[in] quantumScale The units of time used.
				@param[in] quantumDuration The initial timer value. Valid range is 0-63.
				@cmdsize 3
*/
void enableQueueQuantumTimer(uint32_t queueId, Gnm::QuantumScale quantumScale, uint32_t quantumDuration)
{
	return m_dcb.enableQueueQuantumTimer(queueId, quantumScale, quantumDuration);
}

/** @brief Disables a queue's quantum timer.
				@param[in] queueId The ID of the queue returned by Gnm::mapComputeQueue().
				@cmdsize 3
*/
void disableQueueQuantumTimer(uint32_t queueId)
{
	return m_dcb.disableQueueQuantumTimer(queueId);
}

/** @brief Inserts a rewind packet and reserves a specified number of dwords after.
				
				@param[in] reservedDWs      The number of dwords to reserve.
				@return The dword offset from the start of the current command buffer.
				
				@cmdsize 2 + reservedDWs
*/
uint64_t pause(uint32_t reservedDWs)
{
	return m_dcb.pause(reservedDWs);
}

/** @brief Releases the rewind.
				@cmdsize 0
*/
void resume(uint64_t offset)
{
	return m_dcb.resume(offset);
}

/** @brief Fills the allocated hole by copying the <c><i>commandStream</i></c> into it and then releases the rewind packet.
				
				@param[in]	offset			The dword offset of the hole.
				@param[in]	commandStream	The command stream to fill the hole with. 
				@param[in]	sizeInDW		The size of the hole in dwords. 
				
				@cmdsize 0
*/
void fillAndResume(uint64_t offset, void * commandStream, uint32_t sizeInDW)
{
	return m_dcb.fillAndResume(offset, commandStream, sizeInDW);
}

/** @brief Inserts a 'chain' to another command buffer into a previously allocated hole then release the rewind.
				@note The size of the allocated hole must be 4 dwords
				@cmdsize 0
*/
void chainCommandBufferAndResume(uint64_t offset, void * nextIbBaseAddr, uint64_t nextIbSizeInDW)
{
	return m_dcb.chainCommandBufferAndResume(offset, nextIbBaseAddr, nextIbSizeInDW);
}

/** @brief Inserts a 'call' to command buffer.
				@param[in] cbBaseAddr Address of the dcb.
				@param[in] cbSizeInDW Size of the dcb in dwords.
				@note  Calls are only allowed off the primary ring,
				@cmdsize 4
*/
void callCommandBuffer(void * cbBaseAddr, uint64_t cbSizeInDW)
{
	return m_dcb.callCommandBuffer(cbBaseAddr, cbSizeInDW);
}

/** @brief Chain to another command buffer.
				@param[in] cbBaseAddr Address of the dcb
				@param[in] cbSizeInDW Size of the dcb in dwords
				@cmdsize 4
*/
void chainCommandBuffer(void * cbBaseAddr, uint64_t cbSizeInDW)
{
	return m_dcb.chainCommandBuffer(cbBaseAddr, cbSizeInDW);
}

#endif // !defined(_SCE_GNMX_COMPUTECONTEXT_METHODS_H)
