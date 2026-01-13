/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2012 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#if !defined(_SCE_GNMX_HELPERS_H)
#define _SCE_GNMX_HELPERS_H

#include <gnm/common.h>
#include <gnm/constants.h>
#include <gnm/error.h>

namespace sce
{
	namespace Gnm
	{
		class DispatchCommandBuffer;
		class MeasureDispatchCommandBuffer;
		class DrawCommandBuffer;
		class MeasureDrawCommandBuffer;
	}
	namespace Gnmx
	{
		class EsShader; // see gnmx/shaderbinary.h
		class GsShader; // see gnmx/shaderbinary.h
		class LsShader; // see gnmx/shaderbinary.h
		class HsShader; // see gnmx/shaderbinary.h

		/**
		 * @brief Converts a single-precision IEEE float value to an unsigned 4.8 fixed-point value.
		 * @param[in] f The float to convert. Must be in the range <c>[0..15]</c> or the function will assert.
		 * @return The nearest U4.8 value to <c><i>f</i></c>.
		 */
		static inline uint32_t convertF32ToU4_8(float f)
		{
			SCE_GNM_VALIDATE(f >= 0.0f && f < 16.f, "f (%f) must be in the range [0..16)", f);
			uint32_t retVal = static_cast<uint32_t>(f*256.f);
			return f < 0.0f ? 0x000 : (retVal > 0xFFF ? 0xFFF : retVal);
		}

		/**
		 * @brief Converts a single-precision IEEE float value to an unsigned 12.4 fixed-point value.
		 * @param[in] f The float to convert. Must be in the range <c>[0..4095]</c>, or the function will assert.
		 * @return The nearest U12.4 value to <c><i>f</i></c>.
		 */
		static inline uint16_t convertF32ToU12_4(float f)
		{
			SCE_GNM_VALIDATE(f >= 0.0f && f < 4096.f, "f (%f) must be in the range [0..4096)", f);
			uint16_t retVal = static_cast<uint16_t>(f*16.f);
			return f < 0.0f ? 0x0000 : retVal;
		}

		/**
		 * @brief Converts a single-precision IEEE float value to a signed 6.8 fixed-point value.
		 * @param[in] f The float to convert. Must be in the range <c>(-32..32)</c>, or the function will assert.
		 * @return The nearest S6.8 value to <c><i>f</i></c>.
		 */
		static inline int32_t convertF32ToS6_8(float f)
		{
			SCE_GNM_VALIDATE(f > -32.0f && f < 32.f, "f (%f) must be in the range (-32..32)", f);
			int32_t retVal = static_cast<int32_t>(f*256.f);
			retVal = retVal < -32*256 ? (-32*256) : (retVal >= 32*256 ? (32*256-1) : retVal);
			return retVal & 0x3FFF; // mask off high bits for negative values
		}

		/**
		 * @brief Converts a single-precision IEEE float value to a signed 2.4 fixed-point value.
		 * @param[in] f The float to convert. Must be in the range <c>(-2..2)</c>, or the function will assert.
		 * @return The nearest S2.4 value to <c><i>f</i></c>.
		 */
		static inline int32_t convertF32ToS2_4(float f)
		{
			SCE_GNM_VALIDATE(f > -2.0f && f <= 2.f, "f (%f) must be in the range (-2..2)", f);
			int32_t retVal = static_cast<int32_t>(f*16.f);
			retVal = retVal < -2*16 ? (-2*16) : (retVal >= 2*16 ? (2*16-1) : retVal);
			return retVal & 0x003F; // mask off high bits for negative values
		}

		/**
		 * @brief Converts a single-precision IEEE float value to the nearest half-precision IEEE float.
		 * @param[in] f32 The float to convert.
		 * @return The nearest F16 value to <c><i>f32</i></c>.
		 */
		static inline uint16_t convertF32ToF16(const float f32)
		{
			union F32toU32
			{
				uint32_t u;
				float    f;
			} val;
			val.f = f32;

			uint32_t uRes32 = val.u;

			const int16_t  iExp      = (const int16_t)(((const int32_t) ((uRes32>>23)&0xff))-127+15);
			const int16_t  iExpClamp = iExp < 0 ? 0 : (iExp>31 ? 31 : iExp);

			const uint16_t sign = (const uint16_t) ((uRes32>>16)&0x8000);
			const uint16_t mant = (const uint16_t) ((uRes32>>13)&0x3ff);
			const uint16_t exp  = iExpClamp << 10;

			return sign | exp | mant;
		}

		/**
		* @brief Converts a half-precision IEEE float value to the nearest single-precision IEEE float.
		 * @param[in] f16 The float to convert.
		 * @return The nearest F32 value to <c><i>f16</i></c>.
		 */
		static inline float convertF16ToF32(const uint16_t f16)
		{
			union U32toF32
			{
				uint32_t u;
				float    f;
			} val;

			const uint32_t sign = (((const uint32_t) f16)&0x8000)<<16;
			const uint32_t mant = (f16&0x3ff)<<13; // 13 bits are zeros
			const uint32_t exp  = (f16&0x7fff)==0 ? 0 : (((((const uint32_t) f16)>>10)&0x1f)-15+127)<<23;

			val.u = sign | exp | mant;
			return val.f;
		}

		/**
		 * @brief Calculates the number of bits in an FMASK element.
		 * @param[in] numSamples The number of samples per pixel. Must be >= <c><i>numFragments</i></c>.
		 * @param[in] numFragments The number of fragments per pixel. Must be <= <c><i>numSamples</i></c>.
		 * @return The number of bits in an FMASK element.
		 */
		uint32_t getFmaskShiftBits(Gnm::NumSamples numSamples, Gnm::NumFragments numFragments);

		/** @brief Determines the recommended number of patches and the number of primitives per VGT, used as a parameter to Gnm::DrawCommandBuffer::setVgtControl().
		 *
		 * These values are relevant when using hardware tessellation. Ideally, there should be no more than 256 vertices worth of patches per VGT, but you also do not want any partially filled HS thread groups.
		 * @param[out] outVgtPrimCount The VGT primitive count will be written here. Note that you must subtract 1 from this value before passing it to setVgtControl(). This pointer must not be NULL.
		 * @param[out] outPatchCount The number of patches per HS thread group will be written here. This value is used as an input to Gnm::TessellationDataConstantBuffer::init(), Gnm::TessellationRegisters::init(),
		 *                      and GfxContext::setLsHsShaders(). This pointer must not be NULL.
		 * @param[in] maxHsVgprCount The maximum number of HS-stage general-purpose registers to allocate to patches for each threadgroup.
		 * @param[in] maxHsLdsBytes The maximum amount of HS-stage local data store to allocate for patches for each threadgroup. This must also include space for the outputs of the LS stage.
		 * @param[in] lsb The LsShader which will feed into hsb. This pointer must not be NULL.
		 * @param[in] hsb The HsShader to generate patch counts for. This pointer must not be NULL.
		 * @see Gnm::DrawCommandBuffer::setVgtControl(), Gnm::TessellationDataConstantBuffer::init(), Gnm::TessellationRegisters::init(), GfxContext::setLsHsShaders()
		 */
		void computeVgtPrimitiveAndPatchCounts(uint32_t *outVgtPrimCount, uint32_t *outPatchCount, uint32_t maxHsVgprCount, uint32_t maxHsLdsBytes, const LsShader *lsb, const HsShader *hsb);

		/** @brief Computes the number of primitives per VGT.
		 *
		 * Output from this function is used as an argument to pass to Gnm::DrawCommandBuffer::setVgtControl() and may adjust the requested number of patches.
		 * These values are relevant when using hardware tessellation. Ideally, there should be no more than 256 vertices worth of patches per VGT, but you also
		 * do not want any partially filled HS thread groups either.
		 *
		 * @param outVgtPrimCount	Receives the VGT primitive count. Note that 1 must be subtracted from this value before passing it to setVgtControl().
		 * @param inoutPatchCount	An input/output parameter. The input should contain the requested number of patches, and this value can be calculated using sce::Gnm::computeNumPatches().
		 *							The value supplied must be greater than 0.	It may be adjusted by this function and is used as an argument to pass to 
		 *							Gnm::TessellationDataConstantBuffer::init(), Gnm::TessellationRegisters::init() and GfxContext::setLsHsShaders().
		 * @param hsb				The HsShader to generate patch counts for.
		 *
		 * @see Gnm::DrawCommandBuffer::setVgtControl(), Gnm::TessellationDataConstantBuffer::init(), Gnm::TessellationRegisters::init(), GfxContext::setLsHsShaders()
		 */
		void computeVgtPrimitiveCountAndAdjustNumPatches(uint32_t *outVgtPrimCount, uint32_t *inoutPatchCount, const HsShader *hsb);

		/** @brief Computes the LDS allocation size and number of GS threads per LDS allocation for on-chip GS.
		 *
		 * These values may be passed to GfxContext::setOnChipEsShader() and GfxContext::setOnChipGsVsShaders(), respectively.
		 * They are optimized to maximize GS and ES thread utilization within the constraints placed by running in no more than maxLdsUsage bytes of LDS.
		 *
		 * @param[out] outLdsSizeIn512Bytes The ldsSize value in 512 bytes will be written here.
		 * @param[out] outGsPrimsPerSubGroup The gsPrimsPerSubGroup value to be passed to GfxContext::setOnChipGsVsShaders() will be written here.
		 * @param[in] esb The on-chip-GS EsShader binary which will be passed to GfxContext::setOnChipEsShader() must be supplied here.
		 * @param[in] gsb The on-chip GsShader binary which will be passed to GfxContext::setOnChipGsVsShaders() must be supplied here.
		 * @param[in] maxLdsUsage The maximum size in bytes which should be returned in outLdsSizeIn512Bytes.  64*1024 is the maximum size, but smaller values can be used to insist that more than 1 on-chip GS sub-group should be able to run per compute unit.
		 * @return true if at least one GS thread (primitive) can be run in maxLdsUsage bytes of LDS, or false if not.
		 */
		bool computeOnChipGsConfiguration(uint32_t *outLdsSizeIn512Bytes, uint32_t *outGsPrimsPerSubGroup, const EsShader *esb, const GsShader *gsb, uint32_t maxLdsUsage);

		/////////////// DrawCommandBuffer helper functions

		/** @brief Uses the CP DMA to clear a buffer to specified value (like a GPU memset) using a draw command buffer.

			@param[in,out] dcb				The draw command buffer to write GPU commands to.
			@param[out] dstGpuAddr			The destination address to write the data to.
			@param[in] srcData				The value to fill the destination buffer with.
			@param[in] numBytes				The size of the destination buffer. This must be a multiple of 4.
			@param[in] isBlocking    		A flag that specifies whether the CP will block while the transfer is active.
		*/
		void fillData(Gnm::DrawCommandBuffer *dcb, void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking);

		/** @brief Uses the CP DMA to clear a buffer to specified value (like a GPU memset) using a measure draw command buffer.

			
			@param[in,out] dcb				The measure draw command buffer to write GPU commands to.
			@param[out] dstGpuAddr			The destination address to write the data to.
			@param[in] srcData				The value to fill the destination buffer with.
			@param[in] numBytes				The size of the destination buffer. This must be a multiple of 4.
			@param[in] isBlocking			A flag that specifies whether the CP will block while the transfer is active.
		*/
		void fillData(Gnm::MeasureDrawCommandBuffer *dcb, void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking);
		
		/** @brief Uses the CP DMA to transfer data from a source address to a destination address in a draw command buffer.
			
			@param[in,out] dcb				The draw command buffer to write GPU commands to.
			@param[out] dstGpuAddr			The destination address to write the data to.
			@param[in] srcGpuAddr			The source address to read the data from.
			@param[in] numBytes				The number of bytes to transfer over.
			@param[in] isBlocking			A flag that specifies whether the CP will block while the transfer is active.
		*/
		void copyData(Gnm::DrawCommandBuffer *dcb, void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking);
		
		/** @brief Uses the CP DMA to transfer data from a source address to a destination address in a measure draw command buffer.
			
			@param[in,out] dcb				The measure draw command buffer to write GPU commands to.
			@param[out] dstGpuAddr			The destination address to write the data to.
			@param[in] srcGpuAddr			The source address to read the data from.
			@param[in] numBytes				The number of bytes to transfer over.
			@param[in] isBlocking			A flag that specifies whether the CP will block while the transfer is active.
		*/
		void copyData(Gnm::MeasureDrawCommandBuffer *dcb, void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking);
		
		/** @brief Inserts user data directly inside the draw command buffer returning a locator for later reference.
			
			@param[in,out] dcb		The draw command buffer to write GPU commands to.
			@param[in] dataStream	A pointer to the data to embed.
			@param[in] sizeInDword	The size of the data in a stride of 4. Note that the maximum size of a single command packet is 2^16 bytes,
									and the effective maximum value of <c><i>sizeInDword</i></c> will be slightly less than that due to packet headers
									and padding.
			@param[in] alignment	The alignment of the embedded copy in the CB.
			
			@return					A pointer to the allocated buffer.
		*/
		void* embedData(Gnm::DrawCommandBuffer *dcb, const void *dataStream, uint32_t sizeInDword, Gnm::EmbeddedDataAlignment alignment);
		
		/** @brief Inserts user data directly inside the measure command buffer returning a locator for later reference.

			@param[in,out] dcb		The measure draw command buffer to write GPU commands to.
			@param[in] dataStream	A pointer to the data to embed.
			@param[in] sizeInDword	The size of the data in a stride of 4. Note that the maximum size of a single command packet is 2^16 bytes,
									and the effective maximum value of <c><i>sizeInDword</i></c> will be slightly less than that due to packet headers
									and padding.
			@param[in] alignment	The alignment of the embedded copy in the CB.
			
			@return					A pointer to the allocated buffer.
		*/		
		void* embedData(Gnm::MeasureDrawCommandBuffer *dcb, const void *dataStream, uint32_t sizeInDword, Gnm::EmbeddedDataAlignment alignment);

		/** @brief Sets the multisampling sample locations to default values in a draw command buffer. 

			@param[in,out] dcb			The Gnm::DrawCommandBuffer to write commands to.
			@param[in] numAASamples		The number of samples used while multisampling.
		*/
		void setAaDefaultSampleLocations(Gnm::DrawCommandBuffer *dcb, Gnm::NumSamples numAASamples);

		/** @brief Sets the multisampling sample locations to default values in a measure draw command buffer.

			@param[in,out] dcb			The Gnm::MeasureDrawCommandBuffer to write commands to.
			@param[in] numAASamples		The number of samples used while multisampling.
		*/
		void setAaDefaultSampleLocations(Gnm::MeasureDrawCommandBuffer *dcb, Gnm::NumSamples numAASamples);

		/** @brief A utility function that configures (for draw command buffers) the viewport, scissor, and guard band for the provided viewport dimensions.
			
			If more control is required, users can call the underlying functions manually.
			
			@param[in,out] dcb		The draw command buffer to write GPU commands to.
			@param[in] left			The X coordinate of the left edge of the rendering surface in pixels.
			@param[in] top			The Y coordinate of the top edge of the rendering surface in pixels.
			@param[in] right		The X coordinate of the right edge of the rendering surface in pixels.
			@param[in] bottom		The Y coordinate of the bottom edge of the rendering surface in pixels.
			@param[in] zScale		The scale value for the Z transform from clip-space to screen-space. The correct value depends on which
								convention you are following in your projection matrix. For OpenGL-style matrices, use <c><i>zScale</i></c> = 0.5. For Direct3D-style
								matrices, use <c><i>zScale</i></c> = 1.0.
			@param[in] zOffset		The offset value for the Z transform from clip-space to screen-space. The correct value depends on which
								convention you are following in your projection matrix. For OpenGL-style matrices, use <c><i>zOffset</i></c> = 0.5. For Direct3D-style
								matrices, use <c><i>zOffset</i></c> = 0.0.
		*/
		void setupScreenViewport(Gnm::DrawCommandBuffer *dcb, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset);

		/** @brief Utility function that configures (for measure draw command buffers) the viewport, scissor, and guard band for the provided viewport dimensions.
			
			If more control is required, users can call the underlying functions manually.
			
			@param[in,out] dcb      The measure draw command buffer to write GPU commands to.
			@param[in] left			The X coordinate of the left edge of the rendering surface in pixels.
			@param[in] top			The Y coordinate of the top edge of the rendering surface in pixels.
			@param[in] right		The X coordinate of the right edge of the rendering surface in pixels.
			@param[in] bottom		The Y coordinate of the bottom edge of the rendering surface in pixels.
			@param[in] zScale		The scale value for the Z transform from clip-space to screen-space. The correct value depends on which
								convention you are following in your projection matrix. For OpenGL-style matrices, use <c><i>zScale</i></c> = 0.5. For Direct3D-style
								matrices, use <c><i>zScale</i></c> = 1.0.
			@param[in] zOffset		The offset value for the Z transform from clip-space to screen-space. The correct value depends on which
								convention you are following in your projection matrix. For OpenGL-style matrices, use <c><i>zOffset</i></c> = 0.5. For Direct3D-style
								matrices, use <c><i>zOffset</i></c> = 0.0.
		*/
		void setupScreenViewport(Gnm::MeasureDrawCommandBuffer *dcb, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset);

		/** @brief A wrapper around <c>dmaData()</c> to clear the values of one or more append/consume buffer counters (draw command) to the specified value.
		 *
		 * @param[in,out] dcb					The draw command buffer to write GPU commands to.
		 * @param[in] destRangeByteOffset		The byte offset in GDS to the beginning of the counter range to clear. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first <c>RW_Buffer</c> API slot whose counter should be updated. The valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to update. <c><i>startApiSlot</i></c> + <c><i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
		 * @param[in] clearValue				The value to set the specified counters to.
		 *
		 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()
		 *
		 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
		*/
		void clearAppendConsumeCounters(Gnm::DrawCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, uint32_t clearValue);

		/** @brief A wrapper around <c>dmaData()</c> to clear the values of one or more append/consume buffer counters (measure draw command) to the specified value.
		 *
		 * @param[in,out] dcb					The measure draw command buffer to write GPU commands to.
		 * @param[in] destRangeByteOffset		The byte offset in GDS to the beginning of the counter range to clear. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first <c>RW_Buffer</c> API slot whose counter should be updated. The valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to update. <c><i>startApiSlot</i></c> + <c><i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
		 * @param[in] clearValue				The value to set the specified counters to.
		 *
		 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()
 		 *
		 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
		*/
		void clearAppendConsumeCounters(Gnm::MeasureDrawCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, uint32_t clearValue);

		/** @brief A wrapper around <c>dmaData()</c> to update the values of one or more append/consume buffer counters (draw command) using values sourced from the provided GPU-visible address.
		 *
		 * @param[in,out] dcb					The draw command buffer to write GPU commands to.
		 * @param[in] destRangeByteOffset		The byte offset in GDS to the beginning of the counter range to update. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first <c>RW_Buffer</c> API slot whose counter should be updated. The valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>
		 * @param[in] numApiSlots				The number of consecutive slots to update. <c><i>startApiSlot</i></c> + <c><i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
		 * @param[in] srcGpuAddr				The GPU-visible address to read the new counter values from.
		 *
		 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()
		 *
		 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
		 */
		void writeAppendConsumeCounters(Gnm::DrawCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, const void *srcGpuAddr);
		
		/** @brief A wrapper around <c>dmaData()</c> to update the values of one or more append/consume buffer counters (measure draw command) using values sourced from the provided GPU-visible address.
		 *
		 * @param[in,out] dcb					The measure draw command buffer to write GPU commands to.
		 * @param[in] destRangeByteOffset		The byte offset in GDS to the beginning of the counter range to update. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first <c>RW_Buffer</c> API slot whose counter should be updated. The valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>
		 * @param[in] numApiSlots				The number of consecutive slots to update. <c><i>startApiSlot</i></c> + <c><i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
		 * @param[in] srcGpuAddr				The GPU-visible address to read the new counter values from.
		 *
		 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()
		 *
		 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
		 */
		void writeAppendConsumeCounters(Gnm::MeasureDrawCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, const void *srcGpuAddr);

		/** @brief A wrapper around <c>dmaData()</c> to retrieve the values of one or more append/consume buffer counters (draw command) and store them in a GPU-visible address.
		 *
		 * @param[in,out] dcb					The draw command buffer to write GPU commands to.
		 * @param[out] destGpuAddr				The GPU-visible address to write the counter values to.
		 * @param[in] srcRangeByteOffset		The byte offset in GDS to the beginning of the counter range to read. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first <c>RW_Buffer</c> API slot whose counter should be read. The valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to read. <c><i>startApiSlot</i></c> + <c><i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
		 *
		 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()
		 *
		 * @note  GDS accessible size is provided by Gnm::kGdsAccessibleMemorySizeInBytes.
		 */
		void readAppendConsumeCounters(Gnm::DrawCommandBuffer *dcb, void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots);
		
		/** @brief Wrapper around <c>dmaData()</c> to retrieve the values of one or more append/consume buffer counters (measure draw command) and store them in a GPU-visible address.
		 *
		 * @param[in,out] dcb					The measure command buffer to write GPU commands to.
		 * @param[out] destGpuAddr				The GPU-visible address to write the counter values to.
		 * @param[in] srcRangeByteOffset		The byte offset in GDS to the beginning of the counter range to read. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first <c>RW_Buffer</c> API slot whose counter should be read. The valid range is <c>[0..Gnm::kSlotCountRwResource -1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to read. <c><i>startApiSlot</i></c> + <c><i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
		 *
		 * @see Gnm::DispatchCommandBuffer::dmaData(), Gnm::DrawCommandBuffer::dmaData()		 
		 *
		 * @note  GDS accessible size is provided by Gnm::kGdsAccessibleMemorySizeInBytes.
		 */		
		void readAppendConsumeCounters(Gnm::MeasureDrawCommandBuffer *dcb, void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots);

		////////////// DispatchCommandBuffer helper functions

		/** @brief Uses the CP DMA to clear a buffer to specified value (like a GPU memset) using a dispatch command buffer.

			@param[in,out] dcb			The dispatch command buffer to write GPU commands to.
			@param[out] dstGpuAddr		The destination address to write the data to.
			@param[in] srcData			The value to fill the destination buffer with.
			@param[in] numBytes			The size of the destination buffer. This must be a multiple of 4.
			@param[in] isBlocking		A flag that specifies whether the CP will block while the transfer is active.
		*/
		void fillData(Gnm::DispatchCommandBuffer *dcb, void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking);
		
		/** @brief Uses the CP DMA to clear a buffer to specified value (like a GPU memset) using a measure dispatch command buffer.

			@param[in,out] dcb			The measure dispatch command buffer to write GPU commands to.
			@param[out] dstGpuAddr		The destination address to write the data to.
			@param[in] srcData			The value to fill the destination buffer with.
			@param[in] numBytes			The size of the destination buffer. This must be a multiple of 4.
			@param[in] isBlocking		A flag that specifies whether the CP will block while the transfer is active.
		*/
		void fillData(Gnm::MeasureDispatchCommandBuffer *dcb, void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking);

		/** @brief Uses the CP DMA to transfer data from a source address to a destination address in a dispatch command buffer.

			@param[in,out] dcb			The dispatch command buffer to write GPU commands to.
			@param[out] dstGpuAddr		The destination address to write the data to.
			@param[in] srcGpuAddr		The source address to read the data from.
			@param[in] numBytes         The number of bytes to transfer over.
			@param[in] isBlocking       A flag that specifies whether the CP will block while the transfer is active.
		*/
		void copyData(Gnm::DispatchCommandBuffer *dcb, void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking);
		
		/** @brief Uses the CP DMA to transfer data from a source address to a destination address in a measure dispatch command buffer.
			
			@param[in,out] dcb			The measure dispatch command buffer to write GPU commands to.
			@param[out] dstGpuAddr		The destination address to write the data to.
			@param[in] srcGpuAddr		The source address to read the data from.
			@param[in] numBytes			The number of bytes to transfer over.
			@param[in] isBlocking		A flag that specifies whether the CP will block while the transfer is active.
		*/
		void copyData(Gnm::MeasureDispatchCommandBuffer *dcb, void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking);
		
		/** @brief Inserts user data directly inside the dispatch command buffer returning a locator for later reference.
			
			@param[in,out] dcb			The dispatch command buffer to write GPU commands to.
			@param[in] dataStream		A pointer to the data to embed.
			@param[in] sizeInDword		The size of the data in a stride of 4. Note that the maximum size of a single command packet is 2^16 bytes,
										and the effective maximum value of <c><i>sizeInDword</i></c> will be slightly less than that due to packet headers
										and padding.
			@param[in] alignment		The alignment of the embedded copy in the CB.

			@return						A pointer to the allocated buffer. 
		*/
		void* embedData(Gnm::DispatchCommandBuffer *dcb, const void *dataStream, uint32_t sizeInDword, Gnm::EmbeddedDataAlignment alignment);

		/** @brief Inserts user data directly inside the measure dispatch command buffer returning a locator for later reference.

			@param[in,out] dcb			The measure dispatch command buffer to write GPU commands to.
			@para[in]m dataStream		A pointer to the data to embed.
			@param[in] sizeInDword		The size of the data in a stride of 4. Note that the maximum size of a single command packet is 2^16 bytes,
										and the effective maximum value of <c><i>sizeInDword</i></c> will be slightly less than that due to packet headers
										and padding.
			@param[in] alignment		The alignment of the embedded copy in the CB.

			@return						A pointer to the allocated buffer. 
		*/		
		void* embedData(Gnm::MeasureDispatchCommandBuffer *dcb, const void *dataStream, uint32_t sizeInDword, Gnm::EmbeddedDataAlignment alignment);

		/** @brief A wrapper around <c>dmaData()</c> to clear the values of one or more append/consume buffer counters (dispatch command) to the specified value.
		 *
		 * @param[in,out] dcb					The dispatch command buffer to write GPU commands to.
		 * @param[in] destRangeByteOffset		The byte offset in GDS to the beginning of the counter range to clear. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first RW_Buffer API slot whose counter should be updated. The valid range is <c>[0..kSlotCountRwResource-1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to update. <c><i>startApiSlot</i></c> + <c><i>numApiSlots</i></c> must be <= kSlotCountRwResource.
		 * @param[in] clearValue				The value to set the specified counters to.
 		 *
		 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
		 */
		void clearAppendConsumeCounters(Gnm::DispatchCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, uint32_t clearValue);
		
		/** @brief A wrapper around <c>dmaData()</c> to clear the values of one or more append/consume buffer counters (measure dispatch command) to the specified value.
		 *
		 * @param[in,out] dcb					The measure dispatch command buffer to write GPU commands to.
		 * @param[in] destRangeByteOffset		The byte offset in GDS to the beginning of the counter range to clear. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first RW_Buffer API slot whose counter should be updated. The valid range is <c>[0..kSlotCountRwResource-1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to update. <c><i>startApiSlot</i></c> + <c><i>numApiSlots</i></c> must be <= kSlotCountRwResource.
		 * @param[in] clearValue				The value to set the specified counters to.
 		 *
		 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
		*/		
		void clearAppendConsumeCounters(Gnm::MeasureDispatchCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, uint32_t clearValue);

		/** @brief A wrapper around <c>dmaData()</c> to update the values of one or more append/consume buffer counters (dispatch command) using values source from the provided GPU-visible address.
		 *
		 * @param[in,out] dcb					The dispatch command buffer to write GPU commands to.
		 * @param[in] destRangeByteOffset		The byte offset in GDS to the beginning of the counter range to update. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first RW_Buffer API slot whose counter should be updated. The valid range is <c>[0..kSlotCountRwResource-1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to update. <c><i>startApiSlot</i></c>+<c><i>numApiSlots</i></c> must be <= kSlotCountRwResource.
		 * @param[in] srcGpuAddr				The GPU-visible address to read the new counter values from.
		 */
		void writeAppendConsumeCounters(Gnm::DispatchCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, const void *srcGpuAddr);
		
		/** @brief A wrapper around <c>dmaData()</c> to update the values of one or more append/consume buffer counters (measure dispatch command), using values source from the provided GPU-visible address.
		 *
		 * @param[in,out] dcb					The measure dispatch command buffer to write GPU commands to.
		 * @param[in] destRangeByteOffset		The byte offset in GDS to the beginning of the counter range to update. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first RW_Buffer API slot whose counter should be updated. The valid range is <c>[0..kSlotCountRwResource-1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to update. <c><i>startApiSlot</i></c>+<c><i>numApiSlots</i></c> must be <= kSlotCountRwResource.
		 * @param[in] srcGpuAddr				The GPU-visible address to read the new counter values from.
		 */
		void writeAppendConsumeCounters(Gnm::MeasureDispatchCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, const void *srcGpuAddr);

		/** @brief A wrapper around <c>dmaData()</c> to retrieve the values of one or more append/consume buffer counters (dispatch command) and store them in a GPU-visible address.
		 *
		 * @param[in,out] dcb						The dispatch command buffer to write GPU commands to.
		 * @param[out] destGpuAddr				The GPU-visible address to write the counter values to.
		 * @param[in] srcRangeByteOffset		The byte offset in GDS to the beginning of the counter range to read. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first <c>RW_Buffer</c> API slot whose counter should be read. The valid range is <c>[0..kSlotCountRwResource-1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to read. <c><i>startApiSlot</i></c>+<c><i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
		 *
		 * @note  GDS accessible size is provided by Gnm::kGdsAccessibleMemorySizeInBytes.
		 */
		void readAppendConsumeCounters(Gnm::DispatchCommandBuffer *dcb, void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots);

		/** @brief A wrapper around <c>dmaData()</c> to retrieve the values of one or more append/consume buffer counters (measure dispatch command) and store them in a GPU-visible address.
		 *
		 * @param[in,out] dcb						The measure dispatch command buffer to write GPU commands to.
		 * @param[out] destGpuAddr				The GPU-visible address to write the counter values to.
		 * @param[in] srcRangeByteOffset		The byte offset in GDS to the beginning of the counter range to read. This must be a multiple of 4.
		 * @param[in] startApiSlot				The index of the first <c>RW_Buffer</c> API slot whose counter should be read. The valid range is <c>[0..kSlotCountRwResource-1]</c>.
		 * @param[in] numApiSlots				The number of consecutive slots to read. <c><i>startApiSlot</i></c>+<c><i>numApiSlots</i></c> must be less than or equal to Gnm::kSlotCountRwResource.
		 *
		 * @note  GDS accessible size is provided by sce::Gnm::kGdsAccessibleMemorySizeInBytes.
		 */		
		void readAppendConsumeCounters(Gnm::MeasureDispatchCommandBuffer *dcb, void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots);

	}
}

#endif // !defined(_SCE_GNMX_HELPERS_H)
