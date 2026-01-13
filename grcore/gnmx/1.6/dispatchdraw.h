/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef _SCE_GNMX_DISPATCHDRAW_H
#define _SCE_GNMX_DISPATCHDRAW_H

#include <string.h>
#include <gnm/common.h>
#include <gnm/buffer.h>
#include <gnm/constants.h>
#include "grcore/gnmx/error_gen.h"

namespace sce
{
	namespace Gnmx
	{
		const uint32_t kDispatchDrawClipCullFlagClipSpaceDX =	0x00000000;	///< Clip space is  0 < Z < W.
		const uint32_t kDispatchDrawClipCullFlagClipSpaceOGL =	0x00000001;	///< Clip space is -W < Z < W.
		const uint32_t kDispatchDrawClipCullFlagCullCW =		0x00000002;	///< Cull clockwise triangles.
		const uint32_t kDispatchDrawClipCullFlagCullCCW =		0x00000004;	///< Cull counter-clockwise triangles.

		/** @brief Describes the input data for a dispatch draw triangle culling shader.
		*/
		class DispatchDrawTriangleCullData
		{
		public:
			Gnm::Buffer		m_bufferIrb;			///< Index ring buffer. Must be Gnm::Buffer::initAsDataBuffer(pIrb, kDataFormatR16Uint, m_sizeofIrbInIndices).
			Gnm::Buffer		m_bufferInputIndexData;	///< Input index data. Must be Gnm::Buffer::initAsByteBuffer(pInputData, sizeofInputData).
			uint16_t		m_numIndexDataBlocks;	///< Number of index data blocks in <c><i>m_bufferInputIndexData</i></c>.
			uint16_t		m_gdsOffsetOfIrbWptr;	///< Offset in GDS of index ring buffer write pointer counter. Must match the value passed to Gnm::DispatchCommandBuffer::setupDispatchDrawIndexRingBuffer()
			uint32_t		m_sizeofIrbInIndices;	///< Size of index ring buffer in indices. Must match the value passed to Gnm::DispatchCommandBuffer::setupDispatchDrawIndexRingBuffer().

			uint32_t m_clipCullSettings;	///< Union of <c>kDispatchDrawClipCullFlag*</c> variable values.
			uint32_t m_reserved;		// Unused.
			/** Calculating area = <c>(x1/w1 - x2/w2)*y0/w0 + (x2/w2 - x0/w0)*y1/w1 + (x0/w0 - x1/w1)*y2/w2</c>,
				requires calculating the error that will be introduced by <c>*1/w</c> and quantization to hardware screenspace coordinates
				and only cull triangles with area less than <c>-max_area_error (CCW)</c> or area greater than <c>max_area_error (CW)</c>. */
			float m_quantErrorScreenX;
			float m_quantErrorScreenY; ///< See description for #m_quantErrorScreenX.
			/** Triangles which are clipped disable area culling to be safe, as clipping might add difficult to predict errors.
			    Clip if X is greater than <c><i>m_gbHorizClipAdjust</i> * W</c> or X less than <c>-<i>m_gbHorizClipAdjust</i> * W</c>. 
				Clip if Y is greater than <c><i>m_gbVertClipAdjust</i> * W</c> or <c>Y < -<i>m_gbVertClipAdjust</i> * W</c>. 
				Clip if Z is greater than <c>W</c> or Z less than <c>-(clip_z_gl ? 1.0 : 0.0) * W</c>.*/
			float m_gbHorizClipAdjust;
			float m_gbVertClipAdjust; ///< See description for #m_gbHorizClipAdjust.

			//FIXME: need to implement support for instancing
			//FIXME: need to implement support for partial draws (block-wise only, at least, perhaps also masked at primitive level?)
		};

		/** @brief Status for creating input data for DispatchDraw rendering.
		*/
		typedef enum DispatchDrawStatus
		{
			kDispatchDrawOk = 0,																						///< Input data buffer created successfully.
			kDispatchDrawErrorInvalidArguments =			SCE_GNMX_ERROR_DISPATCH_DRAW_INVALID_ARGUMENTS,				///< Buffer creation failed because one or more of the input arguments were not valid.
			kDispatchDrawErrorOutOfSpaceForIndexData =		SCE_GNMX_ERROR_DISPATCH_DRAW_OUT_OF_SPACE_FOR_INDEX_DATA,	///< The size of the output buffer is not large enough for the indexes; call getSizeofDispatchDrawInputData() to determine the size required.
			kDispatchDrawErrorOutOfSpaceForBlockOffset =	SCE_GNMX_ERROR_DISPATCH_DRAW_OUT_OF_SPACE_FOR_BLOCK_OFFSET,	///< The block offset table is not large enough; call getSizeofDispatchDrawInputData() to determine the number of blocks required.
			kDispatchDrawErrorUnrepresentableOffset =		SCE_GNMX_ERROR_DISPATCH_DRAW_UNREPRESENTABLE_OFFSET, 
		} DispatchDrawStatus;

		/** @brief Calculates the required size of input data buffer and number of blocks for DispatchDrawTriangleCullData constructed from input indices in kPrimitiveTypeTriList or kPrimitiveTypeTriStrip format.
		 * @param primType The primitive type of <c><i>pIndicesIn</i></c>, either #Gnm::kPrimitiveTypeTriList or #Gnm::kPrimitiveTypeTriStrip.
		 * @param pIndicesIn The 16-bit input index list.
		 * @param numIndicesIn The number of indices in the 16-bit index list, which should be a multiple of three for #Gnm::kPrimitiveTypeTriList.
		 * @param pNumBlocksOut The required size of the block offset table to write at the start of <c><i>pIndexDataOut</i></c> is returned in this pointer.
		 * @return the size required on success, or DispatchDrawStatus value on errors.
		 */
		size_t getSizeofDispatchDrawInputData(Gnm::PrimitiveType primType, uint16_t const* pIndicesIn, uint32_t numIndicesIn, uint32_t* pNumBlocksOut);

		/** @brief Constructs input data buffer for DispatchDrawTriangleCullData from input indices in kPrimitiveTypeTriList or kPrimitiveTypeTriStrip format.
		 * @param primType The primitive type of <c><i>pIndicesIn</i></c>, either #Gnm::kPrimitiveTypeTriList or #Gnm::kPrimitiveTypeTriStrip.
		 * @param pIndicesIn The 16-bit input index list.
		 * @param numIndicesIn The number of indices in the 16-bit index list. Should be a multiple of three for #Gnm::kPrimitiveTypeTriList.
		 * @param pIndexDataOut A pointer to an output buffer to write data to, which should have buffer (4-byte) alignment; getSizeofDispatchDrawInputData() should be called to determine the size required.

		 * @param sizeofIndexDataOut The size of the output buffer <c><i>pIndexDataOut</i></c>. Call getSizeofDispatchDrawInputData() to determine the size required.
		 * @param numBlocksMax The size of the block offset table to write at the start of <c><i>pIndexDataOut</i></c>. Call getSizeofDispatchDrawInputData() to determine the number of blocks required.
		 * @return #kDispatchDrawOk (0) or other #DispatchDrawStatus value on error.
		 */
		int32_t createDispatchDrawInputData(Gnm::PrimitiveType primType, uint16_t const*const pIndicesIn, uint32_t const numIndicesIn, void*const pIndexDataOut, size_t const sizeofIndexDataOut, uint32_t const numBlocksMax);
	}
}

#endif /* _SCE_GNMX_DISPATCHDRAW_H */

