/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
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
			@param engine Specifies which shader engine should be configured.
			@param mask Mask enabling compute units for the CS shader stage.
			@see Gnm::DrawCommandBuffer::setGraphicsResourceManagement(), Gnmx::GfxContext::setGraphicsResourceManagement()
			@cmdsize 3
*/
void setComputeResourceManagement(Gnm::ShaderEngine engine, uint16_t mask)
{
	return m_dcb.setComputeResourceManagement(engine, mask);
}

/** @brief Sets the shader code to be used for the CS shader stage.
			  @param computeData   Pointer to structure containing memory address (256-byte aligned) of the shader code and additional
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
			  @param computeData   Pointer to structure containing the memory address (256-byte aligned) of the shader code and additional
			                                   registers to set as determined by the shader compiler.
			  @cmdsize 25
*/
void setBulkyCsShader(const Gnm::CsStageRegisters * computeData)
{
	return m_dcb.setBulkyCsShader(computeData);
}

/**
* @brief Reads data from global data store (GDS).
* @param eventType Specifies the event used to trigger the GDS read.
* @param dstGpuAddr The destination address where the GDS data should be written.
* @param gdsOffsetInDwords dword offset into GDS to read from.
* @param gdsSizeInDwords Number of dwords to read.
* @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
* @cmdsize 5
*/
void readDataFromGds(Gnm::EndOfShaderEventType eventType, void * dstGpuAddr, uint32_t gdsOffsetInDwords, uint32_t gdsSizeInDwords)
{
	return m_dcb.readDataFromGds(eventType, dstGpuAddr, gdsOffsetInDwords, gdsSizeInDwords);
}

/** @brief Allocates space for user data directly inside the command buffer, and returns a CPU pointer to the space.
			  @param sizeInBytes          Size of the data in bytes. Note that the maximum size of a single command packet is 2^16 bytes,
			                      and the effective maximum value of <c><i>sizeInBytes</i></c> will be slightly less than that due to packet headers
								  and padding.
			  @param alignment    Alignment from the start of the command buffer.
			  @return Returns a pointer to the memory just allocated. If the requested size is larger than the maximum packet size (64KB),
			          the function returns 0.
			  @cmdsize 2 + (sizeInBytes + sizeof(uint32_t) - 1)/sizeof(uint32_t) + (uint32_t)(1<<alignment)/sizeof(uint32_t)
*/
void * allocateFromCommandBuffer(uint32_t sizeInBytes, Gnm::EmbeddedDataAlignment alignment)
{
	return m_dcb.allocateFromCommandBuffer(sizeInBytes, alignment);
}

/** @brief Sets a buffer resource (<c>V#</c>) in the appropriate shader user data registers.
				@param startUserDataSlot The first user data slot to write to. There are 16 dword-sized user data slots available per shader stage.
				                         A <c>V#</c> occupies four consecutive slots, so the valid range for <c><i>startUserDataSlot</i></c> is <c>[0..12]</c>.
				@param buffer Pointer to a Buffer resource definition.
				@cmdsize 8
*/
void setVsharpInUserData(uint32_t startUserDataSlot, const Gnm::Buffer * buffer)
{
	return m_dcb.setVsharpInUserData(startUserDataSlot, buffer);
}

/** @brief Sets a texture resource (<c>T#</c>) in the appropriate shader user data registers.
				@param startUserDataSlot Starting user data slot. There are 16 dword-sized user data slots available per shader stage.
				                         A <c>T#</c> occupies eight consecutive slots, so the valid range for <c><i>startUserDataSlot</i></c> is <c>[0..8]</c>.
				@param tex Pointer to a Texture resource definition.
				@cmdsize 12
*/
void setTsharpInUserData(uint32_t startUserDataSlot, const Gnm::Texture * tex)
{
	return m_dcb.setTsharpInUserData(startUserDataSlot, tex);
}

/** @brief Sets a sampler resource (<c>S#</c>) in the appropriate shader user data registers.
				@param startUserDataSlot Starting user data slot. There are 16 dword-sized user data slots available per shader stage.
				                         An S# occupies four consecutive slots, so the valid range for <c><i>startUserDataSlot</i></c> is <c>[0..12]</c>.
				@param sampler Pointer to a Sampler resource definition.
				@cmdsize 8
*/
void setSsharpInUserData(uint32_t startUserDataSlot, const Gnm::Sampler * sampler)
{
	return m_dcb.setSsharpInUserData(startUserDataSlot, sampler);
}

/** @brief Sets a GPU pointer in the appropriate shader user data registers.
				@param startUserDataSlot Starting user data slot. There are 16 dword-sized user data slots available per shader stage.
				                         A GPU address occupies two consecutive slots, so the valid range for <c><i>startUserDataSlot</i></c> is <c><i>[0..14]</i></c>.
				@param gpuAddr GPU address to write to the specified slot.
				@cmdsize 4
*/
void setPointerInUserData(uint32_t startUserDataSlot, void * gpuAddr)
{
	return m_dcb.setPointerInUserData(startUserDataSlot, gpuAddr);
}

/** @brief Sets an arbitrary 32-bit integer constant in a shader user data register.
				@param userDataSlot Destination user data slot. There are 16 dword-sized user data slots available per shader stage.
				                    This function sets a single slot, so the valid range for <c><i>userDataSlot</i></c> is <c>[0..15]</c>.
				@param data Data.
				@cmdsize 3
*/
void setUserData(uint32_t userDataSlot, uint32_t data)
{
	return m_dcb.setUserData(userDataSlot, data);
}

void setUserDataRegion(uint32_t startUserDataSlot, const uint32_t * userData, uint32_t numDwords)
{
	return m_dcb.setUserDataRegion(startUserDataSlot, userData, numDwords);
}

/** @brief Specifies how the scratch buffer (compute only) should be subdivided between the executing wavefronts.
				Basically, <c><i>maxNumWaves</i> * <i>num1KByteChunksPerWave</i> * 1024</c> must be less than or equal to the total size of the scratch buffer.
*  @param maxNumWaves Maximum number of wavefronts that could be using the scratch buffer simultaneously.
*                     This should less or equal to 32 times the number of compute units (maximum recommended: 32*18 = 576).
*  @param num1KByteChunksPerWave The amount of scratch buffer space for use by each wavefront. Specified in units of 1024-byte chunks.
*  @cmdsize 3
*/
void setScratchSize(uint32_t maxNumWaves, uint32_t num1KByteChunksPerWave)
{
	return m_dcb.setScratchSize(maxNumWaves, num1KByteChunksPerWave);
}

/** @brief Copies data inline into the command buffer and uses the command processor to transfer it to a destination GPU address.
			@param dstGpuAddr      Destination address to write the data to.
			@param data            Pointer to data to be copied inline.
			@param sizeInDwords    Number of dwords of data to copy.
			@param writeConfirm    Enables/disables write confirmation for this memory write.
			@cmdsize 4+sizeInDwords
*/
void writeDataInline(void * dstGpuAddr, const void * data, uint32_t sizeInDwords, Gnm::WriteDataConfirmMode writeConfirm)
{
	return m_dcb.writeDataInline(dstGpuAddr, data, sizeInDwords, writeConfirm);
}

/** @brief Copies data inline into the command buffer and uses the command processor to transfer it to a destination GPU address through the GPU L2 cache.
			@param dstGpuAddr      Destination address to write the data to.
			@param data            Pointer to data to be copied inline.
			@param sizeInDwords    Number of dwords of data to copy.
			@param cachePolicy	   Specifies the cache policy to use, if the data is written to the GPU's L2 cache.
			@param writeConfirm    Enables or disables write confirmation for this memory write.
			@cmdsize 4+sizeInDwords
*/
void writeDataInlineThroughL2(void * dstGpuAddr, const void * data, uint32_t sizeInDwords, Gnm::CachePolicy cachePolicy, Gnm::WriteDataConfirmMode writeConfirm)
{
	return m_dcb.writeDataInlineThroughL2(dstGpuAddr, data, sizeInDwords, cachePolicy, writeConfirm);
}

/** @brief Triggers an event on the GPU.
			The standard cache flushing and output flushing events do not roll the hardware context. It is possible but not likely that other events roll the hardware context.
			@param eventType Type of the event the command processor should wait for.
			@cmdsize 2
*/
void triggerEvent(Gnm::EventType eventType)
{
	return m_dcb.triggerEvent(eventType);
}

/** @brief Requests the GPU to trigger an interrupt upon release memory event.
			This function never rolls the hardware context.
			@param eventType   Determines when interrupt will be triggered.
			@param cacheAction Cache action to perform.
			@cmdsize 7
*/
void triggerReleaseMemEventInterrupt(Gnm::ReleaseMemEventType eventType, Gnm::CacheAction cacheAction)
{
	return m_dcb.triggerReleaseMemEventInterrupt(eventType, cacheAction);
}

/** @brief Writes the specified value to the given location in memory and triggers an interrupt upon release memory event.
			@param eventType   Determines the type of shader to wait for before writing <c><i>immValue</i></c> to <c>*<i>dstGpuAddr</i></c>.
			@param dstSelector Specifies which levels of the memory hierarchy to write to.
			@param dstGpuAddr     GPU address to which <c><i>immValue</i></c> will be written. Must be 4-byte aligned.
			@param srcSelector Specifies the type of data to write -- either the provided <c><i>immValue</i></c>, or an internal GPU counter.
			@param immValue       Value that will be written to <c><i>dstGpuAddr</i></c>. If <c><i>srcSelect</i></c> specifies a GPU counter, this argument
			                      will be ignored.
			@param cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@param writePolicy		Specifies the cache policy to use, if the data is written to the GPU's L2 cache.
			@cmdsize 7
*/
void writeReleaseMemEventWithInterrupt(Gnm::ReleaseMemEventType eventType, Gnm::EventWriteDest dstSelector, void * dstGpuAddr, Gnm::EventWriteSource srcSelector, uint64_t immValue, Gnm::CacheAction cacheAction, Gnm::CachePolicy writePolicy)
{
	return m_dcb.writeReleaseMemEventWithInterrupt(eventType, dstSelector, dstGpuAddr, srcSelector, immValue, cacheAction, writePolicy);
}

/** @brief Writes the specified value to the given location in memory.
			@param eventType   Determines the type of shader to wait for before writing <c><i>immValue</i></c> to <c>*<i>dstGpuAddr</i></c>.
			@param dstSelector Specifies which levels of the memory hierarchy to write to.
			@param dstGpuAddr     GPU address to which <c><i>immValue</i></c> will be written. Must be 4-byte aligned.
			@param srcSelector Specifies the type of data to write -- either the provided <c><i>immValue</i></c>, or an internal GPU counter.
			@param immValue       Value that will be written to <c><i>dstGpuAddr</i></c>. If <c><i>srcSelect</i></c> specifies a GPU counter, this argument
			                      will be ignored.
			@param cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@param writePolicy		Specifies the cache policy to use, if the data is written to the GPU's L2 cache.
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
			Basically, this function tells the GPU to stall until <c>compareFunc((*<i>gpuAddr</i> and <i>mask</i>), <i>refValue</i>) == true</c>.
			@param gpuAddr  Address to poll. Must be 4-byte aligned.
			@param mask     Mask to be applied to <c>*<i>gpuAddr</i></c> before comparing to <c><i>refValue</i></c>.
			@param compareFunc Specifies the type of comparison to be done between (<c>*<i>gpuAddr</i></c> and <c><i>mask</i></c>) and the <c><i>refValue</i></c>.
			@param refValue    Expected value of <c>*<i>gpuAddr</i></c>.
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
			This function may roll the hardware context.
			@note This command will only wait on output written by graphics shaders -- not compute shaders!
			@param baseAddr256     Starting base address (256-byte aligned) of the surface to be synchronized to (high 32 bits of a 40-bit
								   virtual GPU address).
			@param sizeIn256ByteBlocks        Size of the surface. Has a granularity of 256 bytes.
			@param targetMask		Configures which of the source and destination caches should be enabled for coherency. This field is
											composed of individual flags from the #WaitTargetSlot enum.
			@param cacheAction      Specifies which caches to flush and invalidate after the specified writes are complete.
			@param extendedCacheMask Specifies additional caches to flush and invalidate. This field is composed of individual flags from the #ExtendedCacheAction enum.
			@see Gnm::WaitTargetSlot, Gnm::ExtendedCacheAction, flushShaderCachesAndWait()
			@cmdsize 7
*/
void waitForGraphicsWrites(uint32_t baseAddr256, uint32_t sizeIn256ByteBlocks, uint32_t targetMask, Gnm::CacheAction cacheAction, uint32_t extendedCacheMask)
{
	return m_dcb.waitForGraphicsWrites(baseAddr256, sizeIn256ByteBlocks, targetMask, cacheAction, extendedCacheMask);
}

/** @brief Requests a flush of the specified data cache(s), and waits for the flush operation(s) to complete.
				This function may roll the hardware context.
				@note This function is equivalent to calling <c>waitForGraphicsWrites(0,0,0,<i>cacheAction</i>,<i>extendedCacheMask</i>)</c>.
				@param cacheAction      Specifies which caches to flush and invalidate.
				@param extendedCacheMask Specifies additional caches to flush and invalidate. This field is composed of individual flags from the #ExtendedCacheAction enum.
				@see Gnm::ExtendedCacheAction, waitForGraphicsWrites()
				@cmdsize 7
*/
void flushShaderCachesAndWait(Gnm::CacheAction cacheAction, uint32_t extendedCacheMask)
{
	return m_dcb.flushShaderCachesAndWait(cacheAction, extendedCacheMask);
}

/** @brief Inserts the specified number of dwords in the command buffer as a NOP packet.
			@param numDwords   Number of dwords to insert. The entire packet (including the PM4 header) will be <c><i>numDwords</i></c>.
			                   Valid range is [0..16384].
			@cmdsize numDwords
*/
void insertNop(uint32_t numDwords)
{
	return m_dcb.insertNop(numDwords);
}

/** @brief Sets a marker command in the command buffer that will be used by performance analysis and debug tools.
			The marker command created by this function will be handled as a standalone marker. For a scoped marker block, use pushMarker() and popMarker().
			@param debugString   String to be embedded into the command buffer.
			@see pushMarker(), popMarker()
			@cmdsize 2 + (uint32_t)(strlen(debugString)+1+3)/sizeof(uint32_t)
*/
void setMarker(const char * debugString)
{
	return m_dcb.setMarker(debugString);
}

/** @brief Sets a marker command in the command buffer that will be used by performance analysis and debug tools.
			The marker command created by this function will be handled as a standalone marker. For a scoped marker block, use pushMarker() and popMarker().
			@param debugString   String to be embedded into the command buffer.
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
			@param debugString   String to be embedded into the command buffer.
			@see popMarker()
			@cmdsize 2 + (uint32_t)(strlen(debugString)+1+3)/sizeof(uint32_t)
*/
void pushMarker(const char * debugString)
{
	return m_dcb.pushMarker(debugString);
}

/** @brief Sets a marker command in the command buffer that will be used by the Performance Analysis and Debug tools.
			The marker command created by this function is handled as the beginning of a scoped marker block.
			Close the block with a matching call to popMarker(). Marker blocks can be nested.
			@param debugString   String to be embedded into the command buffer.
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

/** @brief Signals a semaphore.
				@param semAddr Address of the semaphore's mailbox (must be 8-byte aligned).
				@param behavior Selects between incrementing the mailbox value and setting the mailbox value to 1.
				@param updateConfirm If enabled, the packet waits for the mailbox to be written.
				@cmdsize 3
*/
void signalSemaphore(uint64_t * semAddr, Gnm::SemaphoreSignalBehavior behavior, Gnm::SemaphoreUpdateConfirmMode updateConfirm)
{
	return m_dcb.signalSemaphore(semAddr, behavior, updateConfirm);
}

/** @brief Wait on a semaphore.
				Waits until the value in the mailbox is not 0.
				While waiting on a semaphore, the queue is idle and can be preempted for other queues.
				@param semAddr Address of the semaphore's mailbox (must be 8 bytes aligned).
				@param behavior Selects the action to perform when the semaphore opens (mailbox becomes non-zero): either decrement or do nothing.
				@cmdsize 3
*/
void waitSemaphore(uint64_t * semAddr, Gnm::SemaphoreWaitBehavior behavior)
{
	return m_dcb.waitSemaphore(semAddr, behavior);
}

/** @brief Sets queue priority.
				Queue priority can be from 0 through 15 with 15 being the highest priority and 0 the lowest.
				@param queueId The ID of the queue returned by Gnm::mapComputeQueue().
				@param priority The queue priority.
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
				@param queueId The ID of the queue returned by Gnm::mapComputeQueue().
				@param quantumScale The units of time used.
				@param quantumDuration The initial timer value. Valid range is 0-63.
				@cmdsize 3
*/
void enableQueueQuantumTimer(uint32_t queueId, Gnm::QuantumScale quantumScale, uint32_t quantumDuration)
{
	return m_dcb.enableQueueQuantumTimer(queueId, quantumScale, quantumDuration);
}

/** @brief Disables a queue's quantum timer.
				@param queueId The ID of the queue returned by Gnm::mapComputeQueue().
				@cmdsize 3
*/
void disableQueueQuantumTimer(uint32_t queueId)
{
	return m_dcb.disableQueueQuantumTimer(queueId);
}

/** @brief Insert a rewind packet and reserve N dws after.
				Returns a DWORD offset from the start of the current command buffer.
				@cmdsize 2 + reservedDWs
*/
uint64_t pause(uint32_t reservedDWs)
{
	return m_dcb.pause(reservedDWs);
}

/** @brief Release the rewind.
				@cmdsize 0
*/
void resume(uint64_t offset)
{
	return m_dcb.resume(offset);
}

/** @brief Fill the allocated hole copying the commandStream into it, then release the rewind packet.
				@cmdsize 0
*/
void fillAndResume(uint64_t offset, void * commandStream, uint32_t sizeInDW)
{
	return m_dcb.fillAndResume(offset, commandStream, sizeInDW);
}

/** @brief Insert a 'chain' to another command buffer into a previously allocated hole then release the rewind.
				@note The size of the allocated hole must be 4 DW
				@cmdsize 0
*/
void chainCommandBufferAndResume(uint64_t offset, void * nextIbBaseAddr, uint64_t nextIbSizeInDW)
{
	return m_dcb.chainCommandBufferAndResume(offset, nextIbBaseAddr, nextIbSizeInDW);
}

/** @brief Insert a 'call' to command buffer.
				@param cbBaseAddr Address of the dcb
				@param cbSizeInDW Size of the dcb in DW
				@note  Calls are only allowed off the primary ring,
				@cmdsize 4
*/
void callCommandBuffer(void * cbBaseAddr, uint64_t cbSizeInDW)
{
	return m_dcb.callCommandBuffer(cbBaseAddr, cbSizeInDW);
}

/** @brief Chain to another command buffer.
				@param cbBaseAddr Address of the dcb
				@param cbSizeInDW Size of the dcb in DW
				@cmdsize 4
*/
void chainCommandBuffer(void * cbBaseAddr, uint64_t cbSizeInDW)
{
	return m_dcb.chainCommandBuffer(cbBaseAddr, cbSizeInDW);
}

#endif // !defined(_SCE_GNMX_COMPUTECONTEXT_METHODS_H)
