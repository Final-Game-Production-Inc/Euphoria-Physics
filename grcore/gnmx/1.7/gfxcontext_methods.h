/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
// This file was auto-generated from drawcommandbuffer.h -- do not edit by hand!
// This file should NOT be included directly by any non-Gnmx source files!

#if !defined(_SCE_GNMX_GFXCONTEXT_METHODS_H)
#define _SCE_GNMX_GFXCONTEXT_METHODS_H

/** @brief Sets the PM4 packet type to Compute or Graphics shader type.
			  @param[in] shaderType Specifies either Compute or Graphics.
			  @cmdsize 0
*/
void setShaderType(Gnm::ShaderType shaderType)
{
	return m_dcb.setShaderType(shaderType);
}

/** @brief  Sets up a default context state for the graphics ring.
				@cmdsize 10
*/
void initializeToDefaultContextState()
{
	return m_dcb.initializeToDefaultContextState();
}

/** @brief Flushes the streamout pipeline.
				This function should be called before changing or querying streamout state with writeStreamoutBufferUpdate().
				This function will flush the entire GPU pipeline and wait for it to be idle. Use extremely sparingly!
				@cmdsize 21
*/
void flushStreamout()
{
	return m_dcb.flushStreamout();
}

/** @brief Sets the streamout buffer's parameters.
				The VGT (vertex geometry tessellator) needs to know the size and stride of the streamout buffers to correctly update buffer offset.
				This function will roll the hardware context.
				@param[in] bufferId The streamout buffer to update.
				@param[in] bufferSizeInDW The size of the buffer.
				@param[in] bufferStrideInDW The size of an element in the buffer.
				@cmdsize 4
*/
void setStreamoutBufferDimensions(Gnm::StreamoutBufferId bufferId, uint32_t bufferSizeInDW, uint32_t bufferStrideInDW)
{
	return m_dcb.setStreamoutBufferDimensions(bufferId, bufferSizeInDW, bufferStrideInDW);
}

/** @brief Sets the mapping from GS streams to the VS streams in streamout.
				This function will roll the hardware context.
				@param[in] mapping A four-element bitfield where each 4-bit element controls four VS streams for the particular GS stream.
				@cmdsize 3
*/
void setStreamoutMapping(const Gnm::StreamoutBufferMapping * mapping)
{
	return m_dcb.setStreamoutMapping(mapping);
}

/** @brief Write a streamout update packet.
				This packet must be preceded by flushStreamout().
				@param[in] buffer The ID of the streamout buffer to update.
				@param[in] sourceSelect The operation to perform on the buffer offset register.
				@param[in] updateMemory The operation to perform with the current buffer filled size.
				@param[out] dstAddr If Gnm::kStreamoutBufferUpdateSaveFilledSize is specified then <c><i>dstAddr</i></c> provides the memory offset for the saved value.
				@param[in] srcAddrOrImm Specifies the parameter for the <c><i>sourceSelect</i></c> operation.
				@cmdsize 6
*/
void writeStreamoutBufferUpdate(Gnm::StreamoutBufferId buffer, Gnm::StreamoutBufferUpdateWrite sourceSelect, Gnm::StreamoutBufferUpdateSaveFilledSize updateMemory, void * dstAddr, uint64_t srcAddrOrImm)
{
	return m_dcb.writeStreamoutBufferUpdate(buffer, sourceSelect, updateMemory, dstAddr, srcAddrOrImm);
}

/** @brief Writes the immediate offset into a buffer.
				This function is a convenience wrapper around writeStreamoutBufferUpdate().
				@param[in] buffer The ID of the streamout buffer to update.
				@param[in] offset The offset to write into the buffer offset register.
				@cmdsize 6
*/
void writeStreamoutBufferOffset(Gnm::StreamoutBufferId buffer, uint32_t offset)
{
	return m_dcb.writeStreamoutBufferOffset(buffer, offset);
}

/** @brief Enables streamout from a VS shader.
				This allows streamout without a GS shader. After enabling, the streamout has to be set up the same way as the streamout from a GS shader.
				The VS shader has to be compiled with the streamout for this to have any effect. 
				The enabled state might be disabled by setting a GS shader and will be disabled with <c>setGsMode(kGsModeDisable)</c>.
				This function will roll the hardware context.
				
				@param[in] enable		A flag that specifies whether to enable streamout from VS shaders.
				@cmdsize 3
				@see setGsShader(), setGsMode()
*/
void setVsShaderStreamoutEnable(bool enable)
{
	return m_dcb.setVsShaderStreamoutEnable(enable);
}

/** @brief Sets up the buffer parameters for the opaque draw.
				
				This function sets up parameters needed to compute the number of elements in the drawOpaque. The formula for the number of elements is (BUFFER SIZE - OFFSET)/ STRIDE.
				Since the size is read from a memory location it's useful when the number of elements to draw is not known at the time of command buffer's creation such
				as when drawing elements from a streamout buffer.
				This function will roll the hardware context.

				@param sizeLocation The pointer to a dword that contains the opaque buffer size in bytes.
				@param stride The size of a buffer's element in bytes.
				@param offset The value in bytes to be subtracted from the buffer's size.
				@see drawOpaqueAuto()
				@cmdsize 12
*/
void setupDrawOpaqueParameters(void * sizeLocation, uint32_t stride, uint32_t offset)
{
	return m_dcb.setupDrawOpaqueParameters(sizeLocation, stride, offset);
}

/** @brief Sets per dispatch limits that control how many compute wavefronts will be allowed to run simultaneously in the GPU.
			This function never rolls the hardware context.
			@param[in] wavesPerSh        Wavefront limit per shader engine. Range is [1:1023]. 0 to disable the limit.
			@param[in] threadgroupsPerCu  Threadgroup limit per compute unit. Range is [1:15]. 0 to disable the limit.
			@param[in] lockThreshold     Per-shader-engine low threshold for locking. Granularity is 4; 0 to disable the locking.
			@cmdsize 3
*/
void setComputeShaderControl(uint32_t wavesPerSh, uint32_t threadgroupsPerCu, uint32_t lockThreshold)
{
	return m_dcb.setComputeShaderControl(wavesPerSh, threadgroupsPerCu, lockThreshold);
}

/** @brief Sets the mask that determines which compute units (CUs) are active in each shader engine (SE).
				This function never rolls the hardware context.
				@param[in] stage             Shader stage for which the control should be set. Any stage besides CS is valid.
				@param[in] cuMask            Per-shader-engine compute unit execution mask. Specify 0 to use the default mask.
										 Compute units 0, 1, and 2 (bits 0, 1, and 2) cannot be reconfigured from the default enabled setting.
										 For the PS, VS, and GS stages, compute units 0, 1, and 2 must be enabled.
										 For the ES stage, compute unit 0 must be disabled and 1 and 2 must be enabled.
										 For the LS stage, compute units 0 and 1 must be disabled and 2 must be enabled.
										 For the HS stage, <c><i>cuMask</i></c> must be 0; the mask is shared with the LS stage instead.
				@param[in] waveLimit         Per-shader-engine wavefront limits in units of 16. Range is [1:63]. Set to 0 to disable the limit.
				@param[in] lockThreshold     Per-shader-engine low threshold for locking. Granularity is 4. Range is [1:15]. Set to 0 to disable locking.
			
				@note  For CS stage, use setComputeShaderControl().
				@note  For HS stage, <c><i>cuMask</i></c> must be zero; HS stage shares a <c><i>cuMask</i></c> with LS stage.
				@note  Changes to the shader control will remain until the next call to this function.
			@cmdsize (stage == kShaderStagePs) ? 6 : 3
*/
void setGraphicsShaderControl(Gnm::ShaderStage stage, uint16_t cuMask, uint32_t waveLimit, uint32_t lockThreshold)
{
	return m_dcb.setGraphicsShaderControl(stage, cuMask, waveLimit, lockThreshold);
}

/** @brief Sets a per dispatch mask that determines which compute units are active in the specified shader engine.
			All masks are logical masks, indexed from 0 to Gnm::kNumCusPerSe -1, regardless of which physical compute units are working and enabled by the driver.
			This function never rolls the hardware context.
			@note Only applies to PlayStation®4 targets.
			@param[in] engine Specifies which shader engine should be configured.
			@param[in] mask Mask enabling compute units for the CS shader stage.
			@see setGraphicsResourceManagement(), Gnmx::GfxContext::setGraphicsResourceManagement()
			@cmdsize 3
*/
void setComputeResourceManagement(Gnm::ShaderEngine engine, uint16_t mask)
{
	return m_dcb.setComputeResourceManagement(engine, mask);
}

/** @brief Sets masks that determine which compute units are active in the specified shader engine / shader unit during the graphics shader stages.
			All masks are logical masks, indexed from 0 to Gnm::kNumCusPerSe -1, regardless of which physical compute units are working and enabled by the driver.
			@note Only applies to PlayStation®4 targets.
			@param[in] engine Specifies which shader engine should be configured.
			@param[in] maskPs Mask enabling compute units for the PS shader stage.
			@param[in] maskVs Mask enabling compute units for the VS shader stage.
			@param[in] maskGs Mask enabling compute units for the GS shader stage.
			@param[in] maskEs Mask enabling compute units for the ES shader stage.
			@param[in] maskHsLs Mask enabling compute units for the HS/LS shader stages.
			@see setComputeResourceManagement(), Gnmx::GfxContext::setComputeResourceManagement()
			@cmdsize 19
*/
void setGraphicsResourceManagement(Gnm::ShaderEngine engine, uint16_t maskPs, uint16_t maskVs, uint16_t maskGs, uint16_t maskEs, uint16_t maskHsLs)
{
	return m_dcb.setGraphicsResourceManagement(engine, maskPs, maskVs, maskGs, maskEs, maskHsLs);
}

/** @brief Specifies how the scratch buffer (graphics only) should be subdivided between the executing wavefronts for graphics shaders (all stages except CS).
* Basically, <c><i>maxNumWaves</i> * <i>num1KByteChunksPerWave</i> * 1024</c> must be less than or equal to the total size of the scratch buffer.
* This function will roll the hardware context.
*  @param[in] maxNumWaves Maximum number of wavefronts that could be using the scratch buffer simultaneously.
*                     This number should less than or equal to 32 times the number of compute units (maximum recommended: <c>32 * 18 = 576</c>).
*  @param[in] num1KByteChunksPerWave The amount of scratch buffer space for use by each wavefront. Specified in units of 1024-byte chunks.
*  @cmdsize 3
*/
void setGraphicsScratchSize(uint32_t maxNumWaves, uint32_t num1KByteChunksPerWave)
{
	return m_dcb.setGraphicsScratchSize(maxNumWaves, num1KByteChunksPerWave);
}

/** @brief Specifies how the scratch buffer (compute only) should be subdivided between the executing wavefronts for compute (CS stage) shaders.
*
* Basically, <c><i>maxNumWaves</i>*<i>num1KByteChunksPerWave</i>*1024</c> must be less than or equal to the total size of the scratch buffer.
* This function will roll the hardware context.
*  @param[in] maxNumWaves Maximum number of wavefronts that could be using the scratch buffer simultaneously.
*                     This number should less than or equal to 32 times the number of compute units (maximum recommended: <c>32*18 = 576</c>).
*  @param[in] num1KByteChunksPerWave The amount of scratch buffer space for use by each wavefront. Specified in units of 1024-byte chunks.
*  @cmdsize 3
*/
void setComputeScratchSize(uint32_t maxNumWaves, uint32_t num1KByteChunksPerWave)
{
	return m_dcb.setComputeScratchSize(maxNumWaves, num1KByteChunksPerWave);
}

/** @brief Sets the parameters for the Viewport Transform Engine.
				This function will roll the hardware context.
				@param[in] vportControl Register contents.  See ViewportTransformControl structure definition for details.
				@see ViewportTransformControl
				@cmdsize 3
*/
void setViewportTransformControl(Gnm::ViewportTransformControl vportControl)
{
	return m_dcb.setViewportTransformControl(vportControl);
}

/** @brief Sets the parameters for controlling the polygon clipper.
				This function will roll the hardware context.
			  
				@param[in] reg Value to write to the clip control register.
			  
				@see Gnm::ClipControl
				@cmdsize 3
*/
void setClipControl(Gnm::ClipControl reg)
{
	return m_dcb.setClipControl(reg);
}

/** @brief Sets the parameters of one of the eight user clip planes.
				Any vertex that lies on the negative half space of the plane are determined to be outside the clip plane.
				This function will roll the hardware context.
			  @param[in] clipPlane  Indicates which clip plane to set the values for. Range is [0-5].
			  @param[in] x          X component of the plane equation.
			  @param[in] y          Y component of the plane equation.
			  @param[in] z          Z component of the plane equation.
			  @param[in] w          W component of the plane equation.
			  @cmdsize 6
*/
void setUserClipPlane(uint32_t clipPlane, float x, float y, float z, float w)
{
	return m_dcb.setUserClipPlane(clipPlane, x, y, z, w);
}

/** @brief Specifies the top-left and bottom-right coordinates of the four clip rectangles.
			This function will roll the hardware context.
			@param[in] rectId    Index of the clip rectangle to modify (0..3).
			@param[in] left      Left x value of clip rectangle.  15-bit unsigned.  Valid range 0-16383.
			@param[in] top       Top y value of clip rectangle.  15-bit unsigned.  Valid range 0-16383.
			@param[in] right     Right x value of clip rectangle.  15-bit unsigned.  Valid range 0-16384.
			@param[in] bottom    Bottom y value of clip rectangle.  15-bit unsigned.  Valid range 0-16384.
			@cmdsize 4
*/
void setClipRectangle(uint32_t rectId, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	return m_dcb.setClipRectangle(rectId, left, top, right, bottom);
}

/**	@brief Sets the clip rule to use for the OpenGL Clip Boolean function.
			This function will roll the hardware context.
			@param[in] clipRule		The inside flags for each of the four clip rectangles form a 4-bit binary number arranged as 3210.
										These 4 bits are taken as a 4-bit index and the corresponding bit in the 16-bit <c><i>clipRule</i></c> specifies whether the pixel is visible.
										Common values include:
										\li <c>0xFFFE</c> ("Inside any of the four rectangles -> visible").
										\li <c>0x8000</c> ("Outside any of the four rectangles -> not visible").
										\li <c>0xFFFF</c> ("Always visible; ignore clip rectangles").
										\li <c>0x0000</c> ("Always invisible" -- not very useful).
			@cmdsize 3
*/
void setClipRectangleRule(uint16_t clipRule)
{
	return m_dcb.setClipRectangleRule(clipRule);
}

/** @brief Configures the Primitive Setup register, which provides control for facedness, culling, polygon offset, window offset, provoking vertex, and so on.
				This function will roll the hardware context.
			  @param[in] reg    Value to write to the Primitive Setup register.
			  @see Gnm::PrimitiveSetup
			  @cmdsize 3
*/
void setPrimitiveSetup(Gnm::PrimitiveSetup reg)
{
	return m_dcb.setPrimitiveSetup(reg);
}

/** @brief Enables/disables the use of the primitive reset index, used with strip-type primitives to begin a new strip in the middle of a draw call.
				The reset index is specified with setPrimitiveResetIndex().
				This function will roll the hardware context.
			  @param[in] enable If true, primitive reset index functionality is enabled.  If false, it is disabled; the reset index is treated just like any other index buffer entry.
			  @see setPrimitiveResetIndex()
			  @cmdsize 3
*/
void setPrimitiveResetIndexEnable(bool enable)
{
	return m_dcb.setPrimitiveResetIndexEnable(enable);
}

/** @brief Sets the reset index: the value that starts a new primitive (strip/fan/polygon) when it is encountered in the index buffer.
				For this function to work, the reset index feature must be enabled with setPrimitiveResetIndexEnable().
				This function will roll the hardware context.
				@param[in] resetIndex The new restart index.
				@see setPrimitiveResetIndexEnable()
				@cmdsize 3
*/
void setPrimitiveResetIndex(uint32_t resetIndex)
{
	return m_dcb.setPrimitiveResetIndex(resetIndex);
}

/** @brief Sets the vertex quantization behavior, which describes how floating-point X,Y vertex coordinates are converted to fixed-point values.
				This function will roll the hardware context.
				@param[in] quantizeMode Controls the precision of the destination fixed-point vertex coordinate values.
				@param[in] roundMode Controls the rounding behavior when converting to fixed-point.
				@param[in] centerMode Controls the location of the pixel center: 0,0 (Direct3D-style) or 0.5,0.5 (OpenGL-style).
				@cmdsize 3
*/
void setVertexQuantization(Gnm::VertexQuantizationMode quantizeMode, Gnm::VertexQuantizationRoundMode roundMode, Gnm::VertexQuantizationCenterMode centerMode)
{
	return m_dcb.setVertexQuantization(quantizeMode, roundMode, centerMode);
}

/** @brief Specifies the offset from screen coordinates to window coordinates.
				Vertices will be offset by these values if <c><i>windowOffsetEnable</i></c> is true in
				setPrimitiveSetup(). The window scissor and generic scissor will be offset by these values if the <c><i>windowOffsetDisable</i></c> is false in
				setWindowScissor() / setGenericScissor().
				This function will roll the hardware context.
			  @param[in] offsetX  Offset in x-direction from screen to window coordinates. 16-bit signed.
			  @param[in] offsetY  Offset in y-direction from screen to window coordinates. 16-bit signed.
			  @see Gnm::PrimitiveSetup::setVertexWindowOffsetEnable()
			  @cmdsize 3
*/
void setWindowOffset(int16_t offsetX, int16_t offsetY)
{
	return m_dcb.setWindowOffset(offsetX, offsetY);
}

/** @brief Specifies the screen scissor rectangle parameters.
			This is the basic, global, always-enabled, scissor rectangle that applies to every render call.
			This scissor is not affected by the window offset specified by setWindowOffset(); it is specified in absolute coordinates.
				Will roll the hardware context.
			  @param[in] left                 Left hand edge of scissor rectangle.  Valid range -32768-16383.
			  @param[in] top                  Upper edge of scissor rectangle.  Valid range -32768-16383.
			  @param[in] right                Right hand edge of scissor rectangle. Valid range -32768-16384.
			  @param[in] bottom               Lower edge of scissor rectangle.  Valid range -32768-16384.
			  @cmdsize 4
*/
void setScreenScissor(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	return m_dcb.setScreenScissor(left, top, right, bottom);
}

/** @brief Specifies the window scissor rectangle parameters.
				The window scissor is a global, auxiliary scissor rectangle that can be
				specified in either absolute or window-relative coordinates.
				This function will roll the hardware context.
			  @param[in] left                 Left hand edge of scissor rectangle.  Valid range 0-16383.
			  @param[in] top                  Upper edge of scissor rectangle.  Valid range 0-16383.
			  @param[in] right                Right hand edge of scissor rectangle. Valid range 0-16384.
			  @param[in] bottom               Lower edge of scissor rectangle.  Valid range 0-16384.
			  @param[in] windowOffsetEnable   Enables/disables the extra window offset provided by setWindowOffset(). If disabled, the first four arguments are interpreted as
			                              absolute coordinates.
			  @see setWindowOffset()
			  @cmdsize 4
*/
void setWindowScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, Gnm::WindowOffsetMode windowOffsetEnable)
{
	return m_dcb.setWindowScissor(left, top, right, bottom, windowOffsetEnable);
}

/** @brief Specifies the generic scissor rectangle parameters.
				The generic scissor is an additional global, auxiliary scissor rectangle that can be
				specified in either absolute or window-relative coordinates.
				This function will roll the hardware context.
			  @param[in] left                 Left hand edge of scissor rectangle.  Valid range 0-16383.
			  @param[in] top                  Upper edge of scissor rectangle.  Valid range 0-16383.
			  @param[in] right                Right hand edge of scissor rectangle. Valid range 0-16384.
			  @param[in] bottom               Lower edge of scissor rectangle.  Valid range 0-16384.
			  @param[in] windowOffsetEnable   Enables/disables the extra window offset provided by setWindowOffset(). If disabled, the first four arguments are interpreted as
			                              absolute coordinates.
			  @see setWindowOffset()
			  @cmdsize 4
*/
void setGenericScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, Gnm::WindowOffsetMode windowOffsetEnable)
{
	return m_dcb.setGenericScissor(left, top, right, bottom, windowOffsetEnable);
}

/** @brief Specifies the viewport scissor rectangle parameters associated with a viewport ID.
				Geometry shaders can select any of the 16
				viewports; all other shaders can only select viewport 0.
				Note that the viewport scissor will be ignored unless the feature is enabled with setScanModeControl().
				This function will roll the hardware context.
			  @param[in] viewportId           ID of the viewport whose scissor should be updated (ranges from 0 through 15).
			  @param[in] left                 Left hand edge of scissor rectangle.  Valid range 0-32383.
			  @param[in] top                  Upper edge of scissor rectangle.  Valid range 0-32383.
			  @param[in] right                Right hand edge of scissor rectangle. Valid range 0-32384.
			  @param[in] bottom               Lower edge of scissor rectangle.  Valid range 0-32384.
			  @param[in] windowOffsetEnable   Enables/disables the extra window offset provided by setWindowOffset(). If disabled, the first four arguments are interpreted as
			                              absolute coordinates.
			  @see setWindowOffset(), setScanModeControl()
			  @cmdsize 4
*/
void setViewportScissor(uint32_t viewportId, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, Gnm::WindowOffsetMode windowOffsetEnable)
{
	return m_dcb.setViewportScissor(viewportId, left, top, right, bottom, windowOffsetEnable);
}

/** @brief Sets the viewport parameters associated with a viewport ID.
				This function will roll the hardware context.
			  @param[in] viewportId           ID of the viewport (ranges from 0 through 15).
			  @param[in] dmin                 Minimum Z Value from Viewport Transform.  Z values will be clamped by the DB to this value.
			  @param[in] dmax                 Maximum Z Value from Viewport Transform.  Z values will be clamped by the DB to this value.
			  @param[in] scale                Array containing the x, y and z scales.
			  @param[in] offset               Array containing the x, y and z offsets.
			  @cmdsize 12
*/
void setViewport(uint32_t viewportId, float dmin, float dmax, const float scale[3], const float offset[3])
{
	return m_dcb.setViewport(viewportId, dmin, dmax, scale, offset);
}

/** @brief Enables the MSAA and viewport scissoring settings in the Scan Mode control register.
				This function will roll the hardware context.
			  @param[in] msaa             Enables multisampling anti-aliasing.
			  @param[in] viewportScissor  Enables scissors for viewports.
			  @cmdsize 3
*/
void setScanModeControl(Gnm::ScanModeControlAa msaa, Gnm::ScanModeControlViewportScissor viewportScissor)
{
	return m_dcb.setScanModeControl(msaa, viewportScissor);
}
SCE_GNM_API_CHANGED
void setScanModeControl(Gnm::ScanModeControlAa msaa, Gnm::ScanModeControlViewportScissor viewportScissor, Gnm::ScanModeControlLineStipple lineStipple)
{
	(void)lineStipple;
	return m_dcb.setScanModeControl(msaa, viewportScissor);
}

/** @brief Controls Multisampling aliasing.
				This function will roll the hardware context.
			  @param[in] logNumSamples Specified the number of samples to use for MSAA.
			  @cmdsize 3
*/
void setAaSampleCount(Gnm::NumSamples logNumSamples)
{
	return m_dcb.setAaSampleCount(logNumSamples);
}

/** @brief Specifies how often the PS shader is run: once per pixel, or once per sample.
				This function will roll the hardware context.
				@param[in] rate The PS shader execution rate.
				@sa Gnm::DepthEqaaControl::setPsSampleIterationCount()
				@cmdsize 3
*/
void setPsShaderRate(Gnm::PsShaderRate rate)
{
	return m_dcb.setPsShaderRate(rate);
}

/** @brief Sets the render override control.
				@param[in] renderOverrideControl The render override control state.
				@cmdsize 3
*/
void setRenderOverrideControl(Gnm::RenderOverrideControl renderOverrideControl)
{
	return m_dcb.setRenderOverrideControl(renderOverrideControl);
}

/** @brief Sets the multisample AA mask.
				This function will roll the hardware context.
			  @param[in] mask 	Mask is a 64-bit quantity that is treated as 4 16-bit masks. LSB is Sample0, MSB is Sample15.
			  						The 4 masks are applied to each 2x2 screen-aligned pixels as follows:
									- Upper Left Corner   15:0
									- Upper Right Corner 31:16
									- Lower Left Corner  47:32
									- Lower Right Corner 63:48
			  @cmdsize 6
*/
void setAaSampleMask(uint64_t mask)
{
	return m_dcb.setAaSampleMask(mask);
}

/** @brief Sets the Multisampling sample programmable locations per quad (2x2 pixels).
			This function will roll the hardware context.
			Each <c><i>samples</i></c> DWORD gives four 4-bit x/y pairs. Each 4-bit quantity is a signed offset from the pixel center with the range [-8/16, 7/16]:
			<table>
			<tr><td>	x/y pair									</td><td>	Position									</td><td>	Sample Locations		</td></td>
			<tr><td><c>samples[ 0]: x0y0_0		</c>	</td><td>	Upper-left pixel, samples  0- 3.	</td><td>	[ S3_Y  S3_X  S2_Y  S2_X  S1_Y  S1_X  S0_Y  S0_X]	</td></tr>
			<tr><td><c>samples[ 1]: x0y0_1		</c>	</td><td>	Upper-left pixel, samples  4- 7.	</td><td>	 [ S7_Y  S7_X  S6_Y  S6_X  S5_Y  S5_X  S4_Y  S4_X]	</td></tr>
			<tr><td><c>samples[ 2]: x0y0_2		</c>	</td><td>	Upper-left pixel, samples  8-11.	</td><td>	 [S11_Y S11_X S10_Y S10_X  S9_Y  S9_X  S8_Y  S8_X]	</td></tr>
			<tr><td><c>samples[ 3]: x0y0_3		</c>	</td><td>	Upper-left pixel, samples 12-15.	</td><td>	 [S15_Y S15_X S14_Y S14_X S13_Y S13_X S12_Y S12_X]	</td></tr>
			<tr><td><c>samples[ 4]: x1y0_0		</c>	</td><td>	Upper-right pixel, samples  0- 3.	</td><td>	 [ S3_Y  S3_X  S2_Y  S2_X  S1_Y  S1_X  S0_Y  S0_X]	</td></tr>
			<tr><td><c>samples[ 5]: x1y0_1		</c>	</td><td>	Upper-right pixel, samples  4- 7.	</td><td>	 [ S7_Y  S7_X  S6_Y  S6_X  S5_Y  S5_X  S4_Y  S4_X]	</td></tr>
			<tr><td><c>samples[ 6]: x1y0_2		</c>	</td><td>	Upper-right pixel, samples  8-11.	</td><td>	 [S11_Y S11_X S10_Y S10_X  S9_Y  S9_X  S8_Y  S8_X]	</td></tr>
			<tr><td><c>samples[ 7]: x1y0_3 	</c>	</td><td>	Upper-right pixel, samples 12-15.	</td><td>	 [S15_Y S15_X S14_Y S14_X S13_Y S13_X S12_Y S12_X]	</td></tr>
			<tr><td><c>samples[ 8]: x0y1_0		</c>	</td><td>	Lower-left pixel, samples  0- 3.	</td><td>	 [ S3_Y  S3_X  S2_Y  S2_X  S1_Y  S1_X  S0_Y  S0_X]	</td></tr>
			<tr><td><c>samples[ 9]: x0y1_1		</c>	</td><td>	Lower-left pixel, samples  4- 7.	</td><td>	 [ S7_Y  S7_X  S6_Y  S6_X  S5_Y  S5_X  S4_Y  S4_X]	</td></tr>
			<tr><td><c>samples[10]: x0y1_2	</c>	</td><td>	Lower-left pixel, samples  8-11.	</td><td>	 [S11_Y S11_X S10_Y S10_X  S9_Y  S9_X  S8_Y  S8_X]	</td></tr>
			<tr><td><c>samples[11]: x0y1_3	</c>	</td><td>	Lower-left pixel, samples 12-15.	</td><td>	 [S15_Y S15_X S14_Y S14_X S13_Y S13_X S12_Y S12_X]	</td></tr>
			<tr><td><c>samples[12]: x1y1_0	</c>	</td><td>	Lower-right pixel, samples  0- 3.	</td><td>	 [ S3_Y  S3_X  S2_Y  S2_X  S1_Y  S1_X  S0_Y  S0_X]	</td></tr>
			<tr><td><c>samples[13]: x1y1_1	</c>	</td><td>	Lower-right pixel, samples  4- 7.	</td><td>	 [ S7_Y  S7_X  S6_Y  S6_X  S5_Y  S5_X  S4_Y  S4_X]	</td></tr>
			<tr><td><c>samples[14]: x1y1_2	</c>	</td><td>	Lower-right pixel, samples  8-11.	</td><td>	 [S11_Y S11_X S10_Y S10_X  S9_Y  S9_X  S8_Y  S8_X]	</td></tr>
			<tr><td><c>samples[15]: x1y1_3	</c>	</td><td>	Lower-right pixel, samples 12-15.	</td><td>	 [S15_Y S15_X S14_Y S14_X S13_Y S13_X S12_Y S12_X]	</td></tr>
			</table>

			  @param[in] samples Array of 16 32-bit values describing the sample locations for a 2x2 pixel quad. See Description for details.
			  @cmdsize 18
*/
void setAaSampleLocations(uint32_t samples[16])
{
	return m_dcb.setAaSampleLocations(samples);
}

/** @brief Sets the centroid priorities for a sorted list of sample locations.
				Caller must sort sample location distances from closest to furthest and put
				closest sample location number in <c>DISTANCE_0</c>, next in <c>DISTANCE_1</c>, and so on.
				This function will roll the hardware context.
				@param[in] priority		64-bit mask that represents the 16 4-bit values that specify the
											centroid priority. Each of the 4-bit values specifies the nth closest
											sample location to the center. The 4-bit values at offset 0
											is the first closest sample location to center.
				@cmdsize 4
*/
void setCentroidPriority(uint64_t priority)
{
	return m_dcb.setCentroidPriority(priority);
}

/** @brief Specifies the width of the line.
			This function will roll the hardware context.
			@param[in] widthIn8ths    The width of the line in 1/8ths of a pixel.
			@cmdsize 3
*/
void setLineWidth(uint16_t widthIn8ths)
{
	return m_dcb.setLineWidth(widthIn8ths);
}

/** @brief Specifies the dimensions of the point primitives.
			This function will roll the hardware context.
			@param[in] halfWidth    Half width (horizontal radius) of point; fixed point (12.4), 12 bits integer, 4 bits fractional pixels.
			@param[in] halfHeight   Half height (vertical radius) of point; fixed point (12.4), 12 bits integer, 4 bits fractional pixels.
			@cmdsize 3
*/
void setPointSize(uint16_t halfWidth, uint16_t halfHeight)
{
	return m_dcb.setPointSize(halfWidth, halfHeight);
}

/** @brief Specifies the minimum and maximum radius of point primitives and point sprites.
				This function will roll the hardware context.
				@param[in] minRadius Minimum radius; fixed point (12.4), 12 bits integer, 4 bits fractional pixels.
				@param[in] maxRadius Maximum radius; fixed point (12.4), 12 bits integer, 4 bits fractional pixels.
				@cmdsize 3
*/
void setPointMinMax(uint16_t minRadius, uint16_t maxRadius)
{
	return m_dcb.setPointMinMax(minRadius, maxRadius);
}

/** @brief Sets the clamp value for polygon offset.
				This function will roll the hardware context.
				@param[in] clamp Specifies the maximum (if <c><i>clamp</i></c> is positive) or minimum (if <c><i>clamp</i></c> is negative) value clamp for the polygon offset result.
				@note Polygon offset is <c>max(|dzdx|,|dzdy|) * scale + offset * 2^(exponent(max_z_in_primitive) - mantissa_bits_in_z_format)</c>, with clamp applied.
				@see setPolygonOffsetZFormat(), setPolygonOffsetFront(), setPolygonOffsetBack(), Gnm::PrimitiveSetup::setPolygonOffsetEnable()
				@cmdsize 3
*/
void setPolygonOffsetClamp(float clamp)
{
	return m_dcb.setPolygonOffsetClamp(clamp);
}

/** @brief Sets information about the Z-buffer format needed for polygon offset.
				This function will roll the hardware context.
				@param[in] format Z-buffer format.
				@see setPolygonOffsetClamp(), setPolygonOffsetFront(), setPolygonOffsetBack(), Gnm::PrimitiveSetup::setPolygonOffsetEnable()
				@cmdsize 3
*/
void setPolygonOffsetZFormat(Gnm::ZFormat format)
{
	return m_dcb.setPolygonOffsetZFormat(format);
}

/** @brief Sets the front face polygon scale and offset.
				This function will roll the hardware context.
				@param[in] scale   Specifies polygon-offset scale for front-facing polygons; 32-bit IEEE float format. Scale must be specified in 1/16th units (e.g. pass 16.0f for a scale of 1.0).
				@param[in] offset  Specifies polygon-offset offset for front-facing polygons; 32-bit IEEE float format.
				@note Polygon offset is <c>max(|dzdx|,|dzdy|) * scale + offset * 2^(exponent(max_z_in_primitive) - mantissa_bits_in_z_format)</c>, with clamp applied.
				@see setPolygonOffsetClamp(), setPolygonOffsetZFormat(), setPolygonOffsetBack(), Gnm::PrimitiveSetup::setPolygonOffsetEnable()
				@cmdsize 4
*/
void setPolygonOffsetFront(float scale, float offset)
{
	return m_dcb.setPolygonOffsetFront(scale, offset);
}

/** @brief Sets the back face polygon scale and offset.
				This function will roll the hardware context.
				@param[in] scale   Specifies polygon-offset scale for back-facing polygons; 32-bit IEEE float format. Scale must be specified in 1/16th units (e.g. pass 16.0f for a scale of 1.0).
				@param[in] offset  Specifies polygon-offset offset for back-facing polygons; 32-bit IEEE float format.
				@note Polygon offset is <c>max(|dzdx|,|dzdy|) * scale + offset * 2^(exponent(max_z_in_primitive) - mantissa_bits_in_z_format)</c>, with clamp applied.
				@see setPolygonOffsetClamp(), setPolygonOffsetZFormat(), setPolygonOffsetFront(), Gnm::PrimitiveSetup::setPolygonOffsetEnable()
				@cmdsize 4
*/
void setPolygonOffsetBack(float scale, float offset)
{
	return m_dcb.setPolygonOffsetBack(scale, offset);
}

/** @brief Sets the hardware screen offset to center guard band.
				This function will roll the hardware context.
				@param[in] offsetX   Hardware screen offset in X from 0 to 8192 in units of 16 pixels. Range is [0..511].
				@param[in] offsetY   Hardware screen offset in Y from 0 to 8192 in units of 16 pixels. Range is [0..511].
				@sa setGuardBandClip(), setGuardBandDiscard()
				@cmdsize 3
*/
void setHardwareScreenOffset(uint32_t offsetX, uint32_t offsetY)
{
	return m_dcb.setHardwareScreenOffset(offsetX, offsetY);
}

/** @brief Sets the horizontal and vertical clip guard bands.
				This function will roll the hardware context.
				@param[in] vertClip Adjusts the vertical clipping guard band. 32-bit floating point value greater than or equal to 1.0f.
				@param[in] horzClip Adjusts the horizontal clipping guard band. 32-bit floating point value greater than or equal to 1.0f.
				@note These values are a multiplier on the w term of the vertex. Set to 1.0f for no guard band.
				@sa setGuardBandDiscard(), setHardwareScreenOffset()
				@cmdsize 6
*/
void setGuardBandClip(float horzClip, float vertClip)
{
	return m_dcb.setGuardBandClip(horzClip, vertClip);
}

/** @brief Sets the horizontal and vertical discard guard bands.
				This function will roll the hardware context.
				@param[in] vertDisc Adjusts the vertical discard guard band. 32-bit floating point value >= 1.0f.
				@param[in] horzDisc Adjusts the horizontal discard guard band. 32-bit floating point value >= 1.0f.
				@note These values are a multiplier on the w term of the vertex. Set to 1.0f for no guard band.
				@sa setGuardBandClip(), setHardwareScreenOffset()
				@cmdsize 6
*/
void setGuardBandDiscard(float horzDisc, float vertDisc)
{
	return m_dcb.setGuardBandDiscard(horzDisc, vertDisc);
}

/** @brief Sets the two instance step rates that the hardware uses to divide the instance ID by.
				The instance is divided by these two step rates and the result is provided in registers that can be used in the fetch shader.
				This function will roll the hardware context.
				@param[in] step0  Instance step rate 0.
				@param[in] step1  Instance step rate 1.
				@cmdsize 4
*/
void setInstanceStepRate(uint32_t step0, uint32_t step1)
{
	return m_dcb.setInstanceStepRate(step0, step1);
}

/** @brief Sets Pixel Shader interpolator settings for each of the parameters.
				This function will roll the hardware context if different from current state.
				@param[in] inputTable    Array containing the various interpolator settings. This pointer must not be NULL.
				@param[in] numItems      Number of items in the table. Must be less than or equal to 32.
				@note This is set automatically by Gnmx::ConstantUpdateEngine::setPsShader.
				@cmdsize 2+numItems
*/
void setPsShaderUsage(const uint32_t * inputTable, uint32_t numItems)
{
	return m_dcb.setPsShaderUsage(inputTable, numItems);
}

/** @brief Informs the hardware of the maximum number of ES and GS threads that can be launched per on-chip GS sub-group.
				An on-chip GS sub-group consists of a set of ES, GS, and VS wavefronts that share a single LDS allocation.
				
				Use this function when on-chip GS mode is enabled with setGsMode().
			    @param[in] esVerticesPerSubGroup The number of ES vertices (i.e. threads) needed to create the GS primitives specified in <c><i>gsOutputPrimitivesPerSubGroup</i></c> based on the input primitive type.
				@param[in] gsPrimitivesPerSubGroup The number of GS primitives (i.e. threads) that can fit in the LDS based on the LDS usage for input vertices plus all output streams.
				@see setGsMode()
				@cmdsize 3
*/
void setGsOnChipControl(uint32_t esVerticesPerSubGroup, uint32_t gsOutputPrimitivesPerSubGroup)
{
	return m_dcb.setGsOnChipControl(esVerticesPerSubGroup, gsOutputPrimitivesPerSubGroup);
}

/** @brief Sets the address of the border color table.
				This table is unique for each graphics context. It should contain no more than 4096 entries,
				each of which is a 16-byte float4 in RGBA channel order. The total maximum size of the table is thus 64 KB. It is recommended that the entire
				64 KB be allocated as a precaution even if unused, as any reference by a sampler to an out-of-bounds table entry will most likely crash the GPU.
				This function will roll the hardware context.
*  @param[in] tableAddr 256-byte-aligned address of the table of border colors.
*  @cmdsize 3
*/
void setBorderColorTableAddr(void * tableAddr)
{
	return m_dcb.setBorderColorTableAddr(tableAddr);
}

/**
* @brief Reads data from global data store (GDS).
			 	This function never rolls the hardware context.
			 
				@param[in]  eventType			Specifies the event used to trigger the GDS read.
				@param[out] dstGpuAddr			The destination address where the GDS data should be written.
				@param[in]  gdsOffsetInDwords	The dword offset into GDS to read from.
				@param[in]  gdsSizeInDwords		The number of dwords to read.
				
				@note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
				
				@cmdsize 5
*/
void readDataFromGds(Gnm::EndOfShaderEventType eventType, void * dstGpuAddr, uint32_t gdsOffsetInDwords, uint32_t gdsSizeInDwords)
{
	return m_dcb.readDataFromGds(eventType, dstGpuAddr, gdsOffsetInDwords, gdsSizeInDwords);
}

/** @brief Allocates space for user data directly inside the command buffer, and returns a CPU pointer to the space.
				This function never rolls the hardware context.
			  @param[in] sizeInBytes  Size of the data in bytes. Note that the maximum size of a single command packet is 2^16 bytes,
			                      and the effective maximum value of <c><i>sizeInBytes</i></c> will be slightly less than that due to packet headers
								  and padding.
			  @param[in] alignment    Alignment of the pointer from the start of memory, not from the start of the command buffer.
			  @return Returns a pointer to the memory just allocated. If the requested size is larger than the maximum packet size (64 KB),
			          the function will return 0.
			  @cmdsize 2 + (sizeInBytes + sizeof(uint32_t) - 1)/sizeof(uint32_t) + (uint32_t)(1<<alignment)/sizeof(uint32_t)
*/
void * allocateFromCommandBuffer(uint32_t sizeInBytes, Gnm::EmbeddedDataAlignment alignment)
{
	return m_dcb.allocateFromCommandBuffer(sizeInBytes, alignment);
}

/** @brief Copies several contiguous dwords into the specified shader stage's user data registers.
				@param stage The shader stage whose user data registers should be updated.
				@param startUserDataSlot The first slot to write to. The valid range is <c>[0..15]</c>.
				@param userData The source data to copy into the user data registers. If <c><i>numDwords</i></c> is greater than 0, this pointer must not be NULL.
				@param numDwords The number of dwords to copy into the user data registers. The sum of <c><i>startUserDataSlot</i></c> and <c><i>numDwords</i></c> must not exceed 16.
				@cmdsize 4+numDwords
*/
void setUserDataRegion(Gnm::ShaderStage stage, uint32_t startUserDataSlot, const uint32_t * userData, uint32_t numDwords)
{
	return m_dcb.setUserDataRegion(stage, startUserDataSlot, userData, numDwords);
}

/** @brief Sets the depth render target.
			This function will roll the hardware context.
			@param[in] depthTarget  Pointer to a Gnm::DepthRenderTarget struct. If NULL is passed, the depth buffer is disabled.
			@cmdsize depthTarget ? 24 : 6
*/
void setDepthRenderTarget(Gnm::DepthRenderTarget const * depthTarget)
{
	return m_dcb.setDepthRenderTarget(depthTarget);
}

/** @brief Sets the depth clear value.
				This is the depth value used when an HTILE buffer entry's <c>ZMASK</c> field is 0, which indicates that the tile has been cleared to the background depth.
				This function will roll the hardware context.
				@param[in] clearValue	Sets the depth clear value using a 32-bit floating point value which must be in the range of 0.0 to 1.0.
				@see Gnm::DbRenderControl::setDepthClearEnable()
				@cmdsize 3
*/
void setDepthClearValue(float clearValue)
{
	return m_dcb.setDepthClearValue(clearValue);
}

/** @brief Sets the stencil clear value.
				Stencil value used when an HTILE buffer entry's <c>SMEM</c> field is 0, which specifies that the tile is cleared to background stencil values.
				This function will roll the hardware context.
				@param[in] clearValue	Sets the 8-bit stencil clear value.
				@see Gnm::DbRenderControl::setStencilClearEnable()
				@cmdsize 3
*/
void setStencilClearValue(uint8_t clearValue)
{
	return m_dcb.setStencilClearValue(clearValue);
}

/** @brief Sets the fast clear color for the corresponding render target slot.
				This function will roll the hardware context.
				@param[in] rtSlot   Slot index of the Gnm::RenderTarget for which the fast clear color is to be set <c>(0..7)</c>.
				@param[in] clearColor Contains the fast clear color. See the description for how the two <c>uint32_t</c> values are interpreted.
				<table>
				<tr><td>	  Pixel size       </td><td>          Relevant bits                               </td></tr>
				<tr><td>	    8 bpp          </td><td>          clearColor[0] bits [7:0], BGR          </td></tr>
				<tr><td>	   16 bpp         </td><td>          clearColor[0] bits [15:0], ABGR       </td></tr>
				<tr><td>	   32 bpp         </td><td>          clearColor[0] bits [31:0], ABGR       </td></tr>
				<tr><td>	   64 bpp         </td><td>          clearColor[0] and clearColor[1] bits [31:0], ABGR       </td></tr>
				<tr><td>	  128 bpp         </td><td>          Unsupported       </td></tr>
				</table>
				@cmdsize 4
*/
void setCmaskClearColor(uint32_t rtSlot, const uint32_t clearColor[2])
{
	return m_dcb.setCmaskClearColor(rtSlot, clearColor);
}

/** @brief Controls which of the color channels are written into the color surface for the eight Gnm::RenderTarget slots.
				This function will roll the hardware context.
				@param[in] mask        Contains color channel mask fields for writing to Gnm::RenderTarget slots. <c><i>mask</i></c> is treated as a set of eight 4-bit values:
										one for each corresponding render target slot starting from the low bits. Red, green, blue, and alpha are channels 0, 1, 2, and 3 in the pixel shader and are
										enabled by bits 0, 1, 2, and 3 in each field. The low order bit corresponds to the red channel. A zero bit disables writing
										to that channel and a one bit enables writing to that channel.
				@note The channels may be in a different order in the frame buffer, depending on the Gnm::RenderTargetChannelOrder
				field; the bits in <c><i>mask</i></c> correspond to the order of channels after blending and before Gnm::RenderTargetChannelOrder is applied.
				@see Gnm::RenderTargetChannelOrder
				@cmdsize 3
*/
void setRenderTargetMask(uint32_t mask)
{
	return m_dcb.setRenderTargetMask(mask);
}

/** @brief Sets the blending settings for the specified render target slot.
				This function will roll the hardware context.
				@param[in] rtSlot Slot index of the render target to which the new blend settings should be applied <c>[0..7]</c>.
				@param[in] blendControl The new blend settings to apply.
				@cmdsize 3
*/
void setBlendControl(uint32_t rtSlot, Gnm::BlendControl blendControl)
{
	return m_dcb.setBlendControl(rtSlot, blendControl);
}

/** @brief Sets the channels of the constant blend color.
				Certain Gnm::BlendMultiplier values refer to a "constant color" or "constant alpha"; this function specifies these constants.
				This function will roll the hardware context.
				@param[in] red        Red channel of constant blend color as a float.
				@param[in] green      Green channel of constant blend color as a float.
				@param[in] blue       Blue channel of constant blend color as a float.
				@param[in] alpha      Alpha channel of constant blend color as a float.
				@see Gnm::BlendMultiplier
				@cmdsize 6
*/
void setBlendColor(float red, float green, float blue, float alpha)
{
	return m_dcb.setBlendColor(red, green, blue, alpha);
}

/** @brief Sets the stencil test value, stencil mask, stencil write mask, and stencil op value for front- and back-facing primitives together.
				This function will roll the hardware context.
				@param[in] stencilControl Structure in which to specify a mask for stencil buffer values on read and on write and values for stencil test and stencil operation.
				@see Gnm::DepthStencilControl::setStencilFunction(), Gnm::StencilOpControl::setStencilOps()
				@cmdsize 4
*/
void setStencil(Gnm::StencilControl stencilControl)
{
	return m_dcb.setStencil(stencilControl);
}

/** @brief Sets the stencil reference value, stencil mask, stencil write mask, and stencil operation value for front- and back-facing primitives separately.
				This function will roll the hardware context.
				@param[in] front	Specifies the stencil test and operation values and read and write masks for front-facing primitives.
				@param[in] back     Specifies the stencil test and operation values and read and write masks for back-facing primitives.
				@see Gnm::DepthStencilControl::setStencilFunctionBack(), Gnm::DepthStencilControl::setSeparateStencilEnable(), Gnm::StencilOpControl::setStencilOpsBack(),
				@cmdsize 4
*/
void setStencilSeparate(Gnm::StencilControl front, Gnm::StencilControl back)
{
	return m_dcb.setStencilSeparate(front, back);
}

/** @brief Controls alpha-to-mask and sets the dither values if desired.
				
				This function will roll the hardware context.
				
				@param[in] alphaToMaskControl The alpha-to-mask control register.
				@cmdsize 3
*/
void setAlphaToMaskControl(Gnm::AlphaToMaskControl alphaToMaskControl)
{
	return m_dcb.setAlphaToMaskControl(alphaToMaskControl);
}
SCE_GNM_API_DEPRECATED(setAlphaToMaskControl)
void setAlphaToMask(Gnm::AlphaToMaskMode enable, uint32_t pixelOff00, uint32_t pixelOff01, uint32_t pixelOff10, uint32_t pixelOff11, uint32_t round)
{
	Gnm::AlphaToMaskControl control;
	control.init();
	control.setEnabled(enable);
	control.setPixelDitherThresholds((Gnm::AlphaToMaskDitherThreshold)pixelOff00, (Gnm::AlphaToMaskDitherThreshold)pixelOff01,
		(Gnm::AlphaToMaskDitherThreshold)pixelOff10, (Gnm::AlphaToMaskDitherThreshold)pixelOff11);
	control.setRoundMode((Gnm::AlphaToMaskRoundMode)round);
	return m_dcb.setAlphaToMaskControl(control);
}

/** @brief Controls HTILE stencil 0.
				HTILE stencil (known as Hi-S) is like a mini stencil buffer, stored as a "may pass" and "may fail" bit in each HTILE buffer entry.
				Stencil buffer writes for which the "may pass" or "may fail" results can be determined for an entire tile will cause HTILE stencil to be updated.
				Subsequent stencil buffer tests for which results are logically deducible from HTILE stencil can be tile-wise trivially accepted or rejected.
				The application workflow is as follows:
				-# Set HTILE stencil and clear stencil buffer. Do not change HTILE stencil until before next clear.
				-# Write to the stencil buffer in ways that maximize HTILE stencil's ability to tile-wise accelerate later stencil tests.
				-# Render with stencil testing in ways that maximize HTILE stencil's ability to trivially accept or reject tiles.
				@param[in] htileStencilControl Specifies an HTILE stencil for both front- and back-facing primitives.
				@cmdsize 3
*/
void setHtileStencil0(Gnm::HtileStencilControl htileStencilControl)
{
	return m_dcb.setHtileStencil0(htileStencilControl);
}

/** @brief Controls HTILE stencil 1.
				HTILE stencil (known as Hi-S) is like a mini stencil buffer, stored as a "may pass" and "may fail" bit in each HTILE buffer entry.
				Stencil buffer writes for which the "may pass" or "may fail" results can be determined for an entire tile will cause HTILE stencil to be updated.
				Subsequent stencil buffer tests for which results are logically deducible from HTILE stencil can be tile-wise trivially accepted or rejected.
				The application workflow is as follows:
				-# Set HTILE stencil and clear stencil buffer. Do not change HTILE stencil until before next clear.
				-# Write to the stencil buffer in ways that maximize HTILE stencil's ability to tile-wise accelerate later stencil tests.
				-# Render with stencil testing in ways that maximize HTILE stencil's ability to trivially accept or reject tiles.
				@param[in] htileStencilControl Specifies an HTILE stencil for both front- and back-facing primitives.
				@cmdsize 3
*/
void setHtileStencil1(Gnm::HtileStencilControl htileStencilControl)
{
	return m_dcb.setHtileStencil1(htileStencilControl);
}

/** @brief Controls general CB behavior across all render targets.
				This function will roll the hardware context.
				@param[in] mode This field selects standard color processing or one of several major operation modes.
				@param[in] op This field specifies the Boolean operation to apply to source (shader output) and destination
			            (the color buffer). For now, this value must be <c>0xCC</c> (Gnm::kRasterOpSrcCopy), which disables the ROP function and copies the source to the destination.
				@note Gnm::RasterOp must be set to <c>kRasterOpSrcCopy</c> if any render target enables blending.
				@see Gnm::RasterOp
				@cmdsize 3
*/
void setCbControl(Gnm::CbMode mode, Gnm::RasterOp op)
{
	return m_dcb.setCbControl(mode, op);
}

/** @brief Writes the Gnm::DepthStencilControl, which controls depth and stencil tests.
				This function will roll the hardware context.
				@param[in] depthControl   Value to write to the Gnm::DepthStencilControl register.
				@see setDepthStencilDisable()
				@cmdsize 3
*/
void setDepthStencilControl(Gnm::DepthStencilControl depthControl)
{
	return m_dcb.setDepthStencilControl(depthControl);
}

/** @brief Convenient alternative to setDepthStencilControl(), which disables depth/stencil writes and depth/stencil tests.
				This function will roll the hardware context.
				@see setDepthStencilControl()
				@cmdsize 3
*/
void setDepthStencilDisable()
{
	return m_dcb.setDepthStencilDisable();
}

/** @brief Sets the minimum and maximum values used by the depth bounds test.
				This test must be enabled using Gnm::DepthStencilControl::setDepthBoundsEnable().
				If the test is disabled, or if no depth render target is bound, these values are ignored.
				This function will roll the hardware context.
				@param[in] depthBoundsMin The minimum depth that will pass the depth bounds test. Must be less than or equal to <c><i>depthBoundsMax</i></c>. [range: 0..1].
				@param[in] depthBoundsMax The maximum depth that will pass the depth bounds test. Must be greater than <c><i>depthBoundsMin</i></c>. [range: 0..1].
				@see Gnm::DepthStencilControl::setDepthBoundsEnable()
				@cmdsize 4
*/
void setDepthBoundsRange(float depthBoundsMin, float depthBoundsMax)
{
	return m_dcb.setDepthBoundsRange(depthBoundsMin, depthBoundsMax);
}

/** @brief Writes the Gnm::StencilOpControl object, which controls stencil operations.
				This function will roll the hardware context.
				@param[in] stencilControl   Value to write to the Gnm::StencilOpControl register.
				@cmdsize 3
*/
void setStencilOpControl(Gnm::StencilOpControl stencilControl)
{
	return m_dcb.setStencilOpControl(stencilControl);
}

/** @brief Controls enabling depth and stencil clear and configuring various parameters for depth and stencil copy.
				This function will roll the hardware context.
				@param[in] reg      Value to write to the Gnm::DbRenderControl register.
				@cmdsize 3
*/
void setDbRenderControl(Gnm::DbRenderControl reg)
{
	return m_dcb.setDbRenderControl(reg);
}

/** @brief Configures the ZPass count behavior.
				This function will roll the hardware context.
				@param[in] zPassIncrement     If enabled, the internal ZPass counter will be incremented for every fragment that passes the Z test.
				@param[in] perfectZPassCounts If enabled, ZPass counts are forced to be accurate (by disabling no-op culling optimizations which could otherwise lead incorrect counts).
										This is usually enabled when issuing an occlusion query.
				@param[in] log2SampleRate     Sets how many samples are counter for ZPass counts. Normally set to the number of anti-aliased samples.
				@cmdsize 3
*/
void setDbCountControl(Gnm::DbCountControlZPassIncrement zPassIncrement, Gnm::DbCountControlPerfectZPassCounts perfectZPassCounts, uint32_t log2SampleRate)
{
	return m_dcb.setDbCountControl(zPassIncrement, perfectZPassCounts, log2SampleRate);
}

/** @brief Sets the depth EQAA parameters.
			This function will roll the hardware context.
			@param[in] depthEqaa  Value to write to the DB EQAA register.
			@cmdsize 3
*/
void setDepthEqaaControl(Gnm::DepthEqaaControl depthEqaa)
{
	return m_dcb.setDepthEqaaControl(depthEqaa);
}

/** @brief Enables primitive ID generation which is incremented for each new primitive.
				This function will roll the hardware context.
				@param[in] enable Enables or disables primitive ID generation.
				@cmdsize 3
*/
void setPrimitiveIdEnable(bool enable)
{
	return m_dcb.setPrimitiveIdEnable(enable);
}

/** @brief Controls vertex reuse in the VGT (vertex geometry tessellator).
				Reuse is turned off for streamout and viewports.
				This function will roll the hardware context.
				@param[in] enable If true, VGT vertex reuse is enabled.
				@cmdsize 3
*/
void setVertexReuseEnable(bool enable)
{
	return m_dcb.setVertexReuseEnable(enable);
}

/** @brief Sets the VGT (vertex geometry tessellator) primitive type.
				All future draw calls will use this primitive type.
				This function will roll the hardware context.
				@param[in] primType    Primitive type to set.
				@cmdsize 3
*/
void setPrimitiveType(Gnm::PrimitiveType primType)
{
	return m_dcb.setPrimitiveType(primType);
}

/** @brief Sets the number of instances for subsequent draw commands.
				This function never rolls the hardware context.
				@param[in] numInstances The number of instances to render for subsequent draw commands.
								  The minimum value is 1; if 0 is passed, it will be treated as 1.
				@cmdsize 2
*/
void setNumInstances(uint32_t numInstances)
{
	return m_dcb.setNumInstances(numInstances);
}

/** @brief Sets the index offset.
				This offset is added to every index rendered, including those generated by drawIndexAuto(). The default value is 0.
				This function never rolls the hardware context.
				@param[in] offset The offset to set.
				@cmdsize 3
*/
void setIndexOffset(uint32_t offset)
{
	return m_dcb.setIndexOffset(offset);
}

/** @brief Inserts a draw call using auto generated indices and the previously defined primitive type.
				
				The number of indexes is derived from the values set with setupDrawOpaqueParameters() as NUM INDICES  = (BUFFER SIZE - OFFSET)/STRIDE
				This is useful to draw vertices from a streamout buffer when the number of vertices is only known after streamout has finished. Then it's possible
				to write the streamout buffer size into a memory location and use that location as a parameter to the setupDrawOpaqueParameters().
			
				Draw commands never roll the hardware context, but use the current context such that the next command that sets context state will roll context.
				@see setPrimitiveType(), setupDrawOpaqueParameters()
				@cmdsize 5
*/
void drawOpaqueAuto()
{
	return m_dcb.drawOpaqueAuto();
}

/** @brief Sets the number of elements in the index buffer.
				This function never rolls the hardware context.
				@param[in] indexCount Count of indices in the index buffer.
				@cmdsize 2
*/
void setIndexCount(uint32_t indexCount)
{
	return m_dcb.setIndexCount(indexCount);
}

/** @brief Sets the buffer that contains the arguments for the indirect calls: drawIndexIndirect(), drawIndirect() and dispatchIndirect().
				This function never rolls the hardware context.
				@param[in] indirectBaseAddr Address of the buffer containing arguments for use by the indirect draw/dispatch. Must be 8-byte aligned.
				@see drawIndexIndirect(), drawIndirect(), drawIndexIndirectMulti(), drawIndirectMulti(), dispatchIndirect()
				@cmdsize 4
*/
void setBaseIndirectArgs(void * indirectBaseAddr)
{
	return m_dcb.setBaseIndirectArgs(indirectBaseAddr);
}

/** @brief Configures a GDS ordered append unit internal counter to enable special ring buffer counter handling of ds_ordered_count operations targeting a specific GDS address.
			    The GDS ordered append unit supports 16 special allocation counters, which, like GDS, are a global resource that must
				be managed by the application.
				This configures the GDS OA counter <c><i>oaCounterIndex</i></c> ([0:15]) to detect ds_ordered_count operations targeting
				an allocation position counter at GDS address <c><i>gdsDwOffsetOfCounter</i></c>. The operations are modified in order
				to prevent ring buffer data overwrites in addition to the wavefront ordering enforced by ds_ordered_count in general.
				The ring buffer total size in arbitrary allocation units (<c><i>spaceInAllocationUnits</i></c>) is stored in two internal
				GDS registers associated with the GDS OA counter: the constant <c>WRAP_SIZE</c> and the initial value of 
				<c>SPACE_AVAILABLE</c>.
				A given shader wavefront may issue up to 4 ds_ordered_count operations. The GDS ordered append unit queues these
				operations in 4 independent internal queues. This ensures that the operations in each queue are executed in wavefront 
				launch order. The first ds_ordered_count operation issued by each shader wavefront in a dispatch is guaranteed to
				execute in wavefront launch order vs. the first ds_ordered_count operation issued by every other shader wavefront.
				The same guarantee applies to the second, third, and fourth operation issued by each shader wavefront. Arbitrarily,
				many shader wavefronts may execute the first operation before any execute the second, or the second before any 
				execute the third, and so on.
				Wavefront launch order for graphics pipe CS dispatches is straight forward. Dispatches create wavefronts ordered by 
				thread group ID and iterate over X in the inner loop, then Y, then Z. VS wavefront launch order is simply in vertex
				order (post-reuse determination). PS wavefronts are launched as two independent streams on PlayStation®4 hardware with
				one from each shader engine. As a result, ds_ordered_count operations for PS always implicitly split atomic operations
				between 2 sequential counters based on the source shader engine index. Compute shaders must also be dispatched
				with dispatchWithOrderedAppend() in order to enable hardware generation of an ordered wavefront index. VS stage 
				generation of an ordered wavefront index is enabled only by dispatchDraw().
				Any ds_ordered_count operation whose GDS base dword address (M0 bits [18:31]) matches <c><i>gdsDwOffsetOfCounter</i></c>
				will be intercepted for special processing by the GDS ordered append unit. If the operation is the 
				<c><i>oaOpIndex</i></c>'th ds_ordered_count operation issued by the wavefront and the wavefront originated from
				the graphics shader stage <c><i>stage</i></c>, the operation is converted into an allocation operation. If the 
				operation matches the address but not <c><i>oaOpIndex</i></c> or <c><i>stage</i></c>, it is converted to a 
				deallocation operation.  Operations matching the address but originating from asynchronous compute pipes are also
				converted into deallocation operations.
				Any ds_ordered_count operation which is converted to either an allocation or deallocation discards the 
				ds_ordered_count immediate offset (OFFSET0 instruction modifier) and treats it as 0.
				Any ds_ordered_count operation converted to an allocation operation will first wait until the internal
				<c>SPACE_AVAILABLE</c> register value is larger than the requested allocation size before decrementing the
				internal register value.  In addition, some specific ds_ordered_count operations such as WRAP and CONDXCHG32
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
				@param[in] stage					The graphic pipe shader stage will issue allocation operations. Only kShaderStageCs, kShaderStagePs, and kShaderStageVs are supported.
				@param[in] oaOpIndex				The ds_ordered_count operation index which will be interpreted as an allocation operation if also issued from the matching graphic pipe shader stage.
				@param[in] spaceInAllocationUnits	The size of the ring buffer in arbitrary units (elements).
				
				@cmdsize 5
*/
void enableOrderedAppendAllocationCounter(uint32_t oaCounterIndex, uint32_t gdsDwOffsetOfCounter, Gnm::ShaderStage stage, uint32_t oaOpIndex, uint32_t spaceInAllocationUnits)
{
	return m_dcb.enableOrderedAppendAllocationCounter(oaCounterIndex, gdsDwOffsetOfCounter, stage, oaOpIndex, spaceInAllocationUnits);
}

/** @brief Disables a GDS ordered append unit internal counter.
				@param[in] oaCounterIndex			The index of the GDS ordered append unit internal counter to disable; range [0:15].
				
				@cmdsize 5
*/
void disableOrderedAppendAllocationCounter(uint32_t oaCounterIndex)
{
	return m_dcb.disableOrderedAppendAllocationCounter(oaCounterIndex);
}

/** @brief Inserts an occlusion query to count the number of pixels which have passed the depth test.
				Generally, two queries are necessary: one at the beginning and one at the end of the command sequence to measure.
				This function never rolls the hardware context.
				
				@param[in] queryOp			Specifies whether this query marks the beginning of a measurement or the end.
				@param[out] queryResults	Receives the ZPass counts.  If <c><i>queryOp</i></c> is kOcclusionQueryOpBegin or kOcclusionQueryOpClearAndBegin,
											the results are automatically reset to zero with a GPU DMA before the query is written. The contents of this address are described by the
											Gnm::OcclusionQueryResults structure. The contents of this object will be written by the GPU, so the object itself must be
											in GPU-visible memory. This pointer must not be NULL.
				
				@note If the CPU is waiting on query results, and if results objects are being reused within the same command buffer,
						using kOcclusionQueryOpBegin or kOcclusionQueryOpClearAndBegin can lead to a race condition. It is strongly recommended that query results be cleared to zero
						before submission, and that a separate results object is used for each query.
				
				@see setZPassPredicationEnable(), Gnm::OcclusionQueryResults, setDbCountControl()
				
				@cmdsize queryOp == sce::Gnm::kOcclusionQueryOpClearAndBegin ? 22 : 8
*/
void writeOcclusionQuery(Gnm::OcclusionQueryOp queryOp, Gnm::OcclusionQueryResults * queryResults)
{
	return m_dcb.writeOcclusionQuery(queryOp, queryResults);
}

/** @brief Enables and configures conditional rendering based on ZPass results.
				When ZPass predication is active, certain packets (such as draws) will be skipped based on the results of an occlusion / ZPass query.
				This function never rolls the hardware context.
				@param[in] queryResults The results of a previously-issued occlusion query. It is not necessary for the
										application to wait for the results to be ready before enabling predication.
										This pointer must not be NULL.
				@param[in] hint Indicates how to handle draw packets that occur before the appropriate query results are ready. The GPU can
							either wait until the results are ready, or just execute the draw packet as if it were unpredicated.
				@param[in] action Specifies the relation between the query results and whether the predicated draw packets are skipped or executed.
				@see writeOcclusionQuery(), setZPassPredicationDisable(), Gnm::OcclusionQueryResults()
				@cmdsize 3
*/
void setZPassPredicationEnable(Gnm::OcclusionQueryResults * queryResults, Gnm::PredicationZPassHint hint, Gnm::PredicationZPassAction action)
{
	return m_dcb.setZPassPredicationEnable(queryResults, hint, action);
}

/** @brief Disables conditional rendering. Draw call packets will proceed regardless of ZPass results.
				This function never rolls the hardware context.
				@see setZPassPredicationEnable()
				@cmdsize 3
*/
void setZPassPredicationDisable()
{
	return m_dcb.setZPassPredicationDisable();
}

/** @brief Copies data inline into the command buffer and uses the command processor to transfer it to a destination GPU address.
			This function never rolls the hardware context.
			@param[out] dstGpuAddr      Destination address to write the data to.
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
			@param[out] dstGpuAddr      Destination address to write the data to.
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
			This function never rolls the hardware context. (?)
			@param[in] eventType Type of the event the command processor should wait for.
			@cmdsize 2
*/
void triggerEvent(Gnm::EventType eventType)
{
	return m_dcb.triggerEvent(eventType);
}

/** @brief Writes the specified 64-bit value to the given location in memory when this command reaches the end of the processing pipe (EOP).
			This function never rolls the hardware context.
			@param[in] eventType   Determines when <c><i>immValue</i></c> will be written to the specified address.
			@param[in] dstSelector Specifies which levels of the memory hierarchy to write to.
			@param[out] dstGpuAddr     GPU relative address to which the given value will be written. This pointer must be 8-byte aligned and must not be NULL.
			@param[in] srcSelector Specifies the type of data to write -- either the provided <c><i>immValue</i></c>, or an internal GPU counter.
			@param[in] immValue       Value that will be written to <c><i>dstGpuAddr</i></c>. If <c><i>srcSelect</i></c> specifies a GPU counter, this argument
			                      will be ignored.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified write is complete.
			@param[in] cachePolicy		Specifies the cache policy to use, if the data is written to the GPU's L2 cache. This is enabled only when <c><i>dstSelector</i></c> has been set to anything other than <c>kEventWriteDestMemory</c>.
			
			@note This command will not wait for any previously issued draw/dispatch commands to complete. If this behavior is required, use writeAtEndOfShader().
			
			@see writeAtEndOfShader()
			@cmdsize 6
*/
void writeAtEndOfPipe(Gnm::EndOfPipeEventType eventType, Gnm::EventWriteDest dstSelector, void * dstGpuAddr, Gnm::EventWriteSource srcSelector, uint64_t immValue, Gnm::CacheAction cacheAction, Gnm::CachePolicy cachePolicy)
{
	return m_dcb.writeAtEndOfPipe(eventType, dstSelector, dstGpuAddr, srcSelector, immValue, cacheAction, cachePolicy);
}

/** @brief Writes the specified 64-bit value to the given location in memory and triggers an interrupt when this command reaches the end of the processing pipe (EOP).
			This function never rolls the hardware context.
			
			@param[in] eventType   Determines when <c><i>immValue</i></c> will be written to the specified address.
			@param[in] dstSelector Specifies which levels of the memory hierarchy to write to.
			@param[out] dstGpuAddr     GPU address to which <c><i>immValue</i></c> will be written. Must be 8-byte aligned. Must not be NULL.
			@param[in] srcSelector Specifies the type of data to write -- either the provided <c><i>immValue</i></c>, or an internal GPU counter.
			@param[in] immValue       Value that will be written to <c><i>dstGpuAddr</i></c>. If <c><i>srcSelect</i></c> specifies a GPU counter, this argument
			                          will be ignored.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified data is written and the interrupt triggers.
			@param[in] cachePolicy		Specifies the cache policy to use if the data is written to the GPU's L2 cache. This is enabled only when <c><i>dstSelector</i></c> has been set to anything other than <c>kEventWriteDestMemory</c>.
			
			
			@note Applications can use SceKernelEqueue and Gnm::addEqEvent() to handle interrupts.
			
			@note This command will not wait for any previously issued draw/dispatch commands to complete. If this behavior is required, use writeAtEndOfShader().
			
			@see writeAtEndOfShader()
			@cmdsize 6
*/
void writeAtEndOfPipeWithInterrupt(Gnm::EndOfPipeEventType eventType, Gnm::EventWriteDest dstSelector, void * dstGpuAddr, Gnm::EventWriteSource srcSelector, uint64_t immValue, Gnm::CacheAction cacheAction, Gnm::CachePolicy cachePolicy)
{
	return m_dcb.writeAtEndOfPipeWithInterrupt(eventType, dstSelector, dstGpuAddr, srcSelector, immValue, cacheAction, cachePolicy);
}

/** @brief Requests the GPU to trigger an interrupt upon EOP event.
			This function never rolls the hardware context.
			@param[in] eventType   Determines when interrupt will be triggered.
			@param[in] cacheAction Cache action to perform.
			@note Applications can use <c>SceKernelEqueue</c> and sce::Gnm::addEqEvent() to handle interrupts.
			@cmdsize 6
*/
void triggerEndOfPipeInterrupt(Gnm::EndOfPipeEventType eventType, Gnm::CacheAction cacheAction)
{
	return m_dcb.triggerEndOfPipeInterrupt(eventType, cacheAction);
}

/** @brief Writes the specified value to the given location in memory when the specified shader stage becomes idle.
			This function never rolls the hardware context.
			@param[in] eventType   Determines the type of shader to wait for before writing <c><i>immValue</i></c> to <c>*dstGpuAddr</c> (PS or CS).
								   If kEosPsDone is passed, this command will not wait for previous dispatches to complete.
								   If kEosCsDone is passed, this command will not wait for previous draws to complete.
			@param[out] dstGpuAddr     GPU address to which <c><i>immValue</i></c> will be written. Must be 4-byte aligned. Must not be NULL.
			@param[in] immValue       Value that will be written to <c><i>dstGpuAddr</i></c>.
			
			@note When <c><i>eventType</i></c> is set to <c>kEosCsDone</c>, this function should be called right after the call to 'dispatch'.
			
			@cmdsize 5
*/
void writeAtEndOfShader(Gnm::EndOfShaderEventType eventType, void * dstGpuAddr, uint32_t immValue)
{
	return m_dcb.writeAtEndOfShader(eventType, dstGpuAddr, immValue);
}

/** @brief Blocks frontend processing until indicated test passes.
			The 32-bit value at the specified GPU address is tested against the
			reference value with the test qualified by the specified function and mask.
			Basically: tell the GPU to stall until <c><i>compareFunc</i>((*<i>gpuAddr</i> and <i>mask</i>), <i>refValue</i>) == true</c>.
			This function never rolls the hardware context.
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

/** @brief Stalls parsing of the command buffer until all previous commands have started execution.
			@note	Commands may not necessarily be finished executing. For example, draw commands
					may have been launched but not necessarily finished and committed to memory.
			@note	In addition to fetching command buffer data, the CP prefetches the index data referenced by subsequent draw calls.
					Stalling the command buffer parsing will also prevent this index prefetching, which may be necessary if the index data is
					dynamically generated or uploaded just-in-time by a previous command.
			@cmdsize 2
			@sa waitOnAddressAndStallCommandBufferParser()
*/
void stallCommandBufferParser()
{
	return m_dcb.stallCommandBufferParser();
}

/** @brief Blocks frontend processing until indicated test passes and until all previous commands have started execution.
			The 32-bit value at the specified GPU address is tested against the
			reference value with the test qualified by the specified function and mask.
			This function tells the GPU to stall until:
			<c><i>compareFunc</i>((*<i>gpuAddr</i> and <i>mask</i>), <i>refValue</i>) == true</c>
			
			Unlike waitOnAddress(), this variant only supports a compare function of Gnm::kWaitCompareFuncGreaterEqual (which is hard-coded).
			This function never rolls the hardware context.
			
			@param[in] gpuAddr		Address to poll. Must be 4-byte aligned.
			@param[in] mask			Mask to be applied to <c>*<i>gpuAddr</i></c> before comparing to <c><i>refValue</i></c>.
			@param[in] refValue		Expected value of <c>*<i>gpuAddr</i></c>.
			@note	Commands may not necessarily be finished executing. For example, draw commands
					may have been launched but not necessarily finished and committed to memory.
			@note	In addition to fetching command buffer data, the CP prefetches the index data referenced by subsequent draw calls.
					Stalling the command buffer parsing will also prevent this index prefetching, which may be necessary if the index data is
					dynamically generated or uploaded just-in-time by a previous command.
			@cmdsize 7
*/
void waitOnAddressAndStallCommandBufferParser(void * gpuAddr, uint32_t mask, uint32_t refValue)
{
	return m_dcb.waitOnAddressAndStallCommandBufferParser(gpuAddr, mask, refValue);
}

/** @brief Blocks frontend processing until indicated test passes.
			The 32-bit value at the specified GPU register is tested against the
			reference value with the test qualified by the specified function and mask.
			Basically: tell the GPU to stall until <c><i>compareFunc</i>((*<i>gpuAddr</i> and <i>mask</i>), <i>refValue</i>) == true</c>.
			This function never rolls the hardware context.
			@param[in] gpuReg  Register offset to poll.
			@param[in] mask     Mask to be applied to <c>*<i>gpuAddr</i></c> before comparing to <c><i>refValue</i></c>.
			@param[in] compareFunc Specifies the type of comparison to be done between (<c>*<i>gpuAddr</i></c> and <c><i>mask</i></c>) and the <c><i>refValue</i></c>.
			@param[in] refValue    Expected value of <c>*<i>gpuAddr</i></c>.
			@cmdsize 7
*/
void waitOnRegister(uint16_t gpuReg, uint32_t mask, Gnm::WaitCompareFunc compareFunc, uint32_t refValue)
{
	return m_dcb.waitOnRegister(gpuReg, mask, compareFunc, refValue);
}

/** @brief Waits for all PS shader output to one or more targets to complete.
			One can specify the various render target slots
			(color and/or depth,) to be checked within the provided base address and size: all active contexts associated with
			those target can then be waited for. The caller may also optionally specify that certain caches be flushed.
			This function may roll the hardware context.
			@note This command will only wait on output written by graphics shaders, not compute shaders!
			@param[in] baseAddr256     Starting base address (256-byte aligned) of the surface to be synchronized to (high 32 bits of a 40-bit
								   virtual GPU address).
			@param[in] sizeIn256ByteBlocks        Size of the surface. Has a granularity of 256 bytes.
			@param[in] targetMask		Configures which of the source and destination caches should be enabled for coherency. This field is
											composed of individual flags from the #Gnm::WaitTargetSlot enum.
			@param[in] cacheAction      Specifies which caches to flush and invalidate after the specified writes are complete.
			@param[in] extendedCacheMask Specifies additional caches to flush and invalidate. This field is composed of individual flags from the #Gnm::ExtendedCacheAction enum.
			@param[in] commandBufferStallMode Specifies whether to stall further parsing of the command buffer until the wait condition is complete.
			@see Gnm::WaitTargetSlot, Gnm::ExtendedCacheAction, flushShaderCachesAndWait()
			@cmdsize 7
*/
void waitForGraphicsWrites(uint32_t baseAddr256, uint32_t sizeIn256ByteBlocks, uint32_t targetMask, Gnm::CacheAction cacheAction, uint32_t extendedCacheMask, Gnm::StallCommandBufferParserMode commandBufferStallMode)
{
	return m_dcb.waitForGraphicsWrites(baseAddr256, sizeIn256ByteBlocks, targetMask, cacheAction, extendedCacheMask, commandBufferStallMode);
}

/** @brief Requests a flush of the specified data cache(s), and waits for the flush operation(s) to complete.
				This function may roll the hardware context.
				@note This function is equivalent to calling <c>waitForGraphicsWrites(0,0,0,<i>cacheAction</i>,<i>extendedCacheMask</i>)</c>.
				@param[in] cacheAction      Specifies which caches to flush and invalidate.
				@param[in] extendedCacheMask Specifies additional caches to flush and invalidate. This field is composed of individual flags from the #Gnm::ExtendedCacheAction enum.
				@param[in] commandBufferStallMode Specifies whether to stall further parsing of the command buffer until the wait condition is complete.
				@see waitForGraphicsWrites(), Gnm::ExtendedCacheAction
				@cmdsize 7
*/
void flushShaderCachesAndWait(Gnm::CacheAction cacheAction, uint32_t extendedCacheMask, Gnm::StallCommandBufferParserMode commandBufferStallMode)
{
	return m_dcb.flushShaderCachesAndWait(cacheAction, extendedCacheMask, commandBufferStallMode);
}

/** @brief Signals a semaphore.
				@param[out] semAddr Address of the semaphore's mailbox (must be 8-byte aligned). This pointer must not be NULL.
				@param[in] behavior Selects between incrementing the mailbox value and setting the mailbox value to 1.
				@param[in] updateConfirm If enabled, the packet waits for the mailbox to be written.
				@cmdsize 3
*/
void signalSemaphore(uint64_t * semAddr, Gnm::SemaphoreSignalBehavior behavior, Gnm::SemaphoreUpdateConfirmMode updateConfirm)
{
	return m_dcb.signalSemaphore(semAddr, behavior, updateConfirm);
}

/** @brief Waits on a semaphore.
				This function waits until the value in the mailbox is not 0.
				@param[out] semAddr Address of the semaphore's mailbox (must be 8-byte aligned). This pointer must not be NULL.
				@param[in] behavior Selects the action to perform when the semaphore opens (mailbox becomes non-zero): either decrement or do nothing.
				@cmdsize 3
*/
void waitSemaphore(uint64_t * semAddr, Gnm::SemaphoreWaitBehavior behavior)
{
	return m_dcb.waitSemaphore(semAddr, behavior);
}

/** @brief Writes out the statistics for the specified event to memory.
				This function never rolls the hardware context.
				@param[in] eventStats  	Type of event to get the statistics for.
				@param[out] dstGpuAddr	GPU-relative address where the stats will be written to. This pointer must not be NULL.
				@cmdsize 4
*/
void writeEventStats(Gnm::EventStats eventStats, void * dstGpuAddr)
{
	return m_dcb.writeEventStats(eventStats, dstGpuAddr);
}

/** @brief Inserts the specified number of dwords in the command buffer as a NOP packet.
				This function never rolls the hardware context.
				@param[in] numDwords   Number of dwords to insert. The entire packet (including the PM4 header) will be <c><i>numDwords</i></c>. The valid range is [0..16384].
				@cmdsize numDwords
*/
void insertNop(uint32_t numDwords)
{
	return m_dcb.insertNop(numDwords);
}

/** @brief Sets a marker command in the command buffer that will be used by the PA/Debug tools.
			The marker command created by this function will be handled as a standalone marker. For a scoped marker block,
			use pushMarker() and popMarker().
			This function never rolls the hardware context.
			@param[in] debugString   The string to be embedded into the command buffer. This pointer must not be NULL.
			
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
			
			@param debugString		The string to be embedded into the command buffer. This pointer must not be NULL.
			@param argbColor		The color of the marker.
			
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
			This function never rolls the hardware context.
			
			@param[in] debugString   The string to be embedded into the command buffer. This pointer must not be NULL.
			
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
			@param debugString  The string to be embedded into the command buffer. This pointer must not be NULL.
			@param argbColor	The color of the marker.
			
			@see popMarker()
			@cmdsize 2 + (uint32_t)(strlen(debugString)+1+3)/sizeof(uint32_t)
*/
void pushMarker(const char * debugString, uint32_t argbColor)
{
	return m_dcb.pushMarker(debugString, argbColor);
}

/** @brief Closes the marker block opened by the most recent pushMarker() command.
				This function never rolls the hardware context.
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

#ifdef __ORBIS__
/** @brief Waits for the specified render target to stop being displayed.
			@param[in] videoOutHandle The video output handle obtained from <c>sceVideoOutOpen()</c>.
			@param[in] flipIndex Render target to wait for.
			@cmdsize 7
*/
void waitUntilSafeForRendering(uint32_t videoOutHandle, uint32_t flipIndex)
{
	return m_dcb.waitUntilSafeForRendering(videoOutHandle, flipIndex);
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
				
				@param[in]	offset			The dword offset.
				
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

/** @brief Insert a 'chain' to another command buffer into a previously allocated hole then release the rewind.
				@note The size of the allocated hole must be 4 DW
				@cmdsize 0
*/
void chainCommandBufferAndResume(uint64_t offset, void * nextIbBaseAddr, uint64_t nextIbSizeInDW)
{
	return m_dcb.chainCommandBufferAndResume(offset, nextIbBaseAddr, nextIbSizeInDW);
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

#endif // __ORBIS__

#endif // !defined(_SCE_GNMX_GFXCONTEXT_METHODS_H)
