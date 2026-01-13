/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
* Copyright (C) 2012 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/
#include "grcore/gnmx/helpers.h"

#ifndef SCE_GNM_HP3D
#include <gnm/dispatchcommandbuffer.h>
#include <gnm/measuredispatchcommandbuffer.h>
#endif // SCE_GNM_HP3D
#include <gnm/drawcommandbuffer.h>
#include <gnm/measuredrawcommandbuffer.h>
#include <gnm/constants.h>
#include "grcore/gnmx/shaderbinary.h"

#include <cmath>
#include <cstring>

using namespace sce::Gnm;
using namespace sce::Gnmx;

uint32_t sce::Gnmx::getFmaskShiftBits(NumSamples numSamples, NumFragments numFragments)
{
	SCE_GNM_VALIDATE((uint32_t)numFragments <= (uint32_t)numSamples, "numFragments (%d) must be <= numSamples (%d)", 1<<numFragments, 1<<numSamples);
	if ((uint32_t)numSamples == (uint32_t)numFragments)
	{
		// Normal MSAA
		uint32_t numBitsFromFragments[] = {1, 1, 2, 4};
		return numBitsFromFragments[numFragments];
	}
	else // numFragments < numSamples
	{
		// EQAA
		uint32_t numBitsFromFragments[] = {1, 2, 4, 4};
		return numBitsFromFragments[numFragments];
	}
}

void sce::Gnmx::computeVgtPrimitiveAndPatchCounts(uint32_t *outVgtPrimCount, uint32_t *outPatchCount, uint32_t maxHsVgprCount, uint32_t maxHsLdsBytes, const LsShader *lsb, const HsShader *hsb)
{
	SCE_GNM_VALIDATE(lsb != 0, "lsb must not be NULL.");
	SCE_GNM_VALIDATE(hsb != 0, "hsb must not be NULL.");
	SCE_GNM_VALIDATE(outVgtPrimCount, "outVgtPrimCount must not be NULL.");
	SCE_GNM_VALIDATE(outPatchCount, "outPatchCount must not be NULL.");
	
	// Determine numPatches per TG given the limits on HS resource usage
	uint32_t tempNumPatches = computeNumPatches(&hsb->m_hsStageRegisters, &hsb->m_hullStateConstants, lsb->m_lsStride, maxHsVgprCount, maxHsLdsBytes);
	SCE_GNM_VALIDATE(tempNumPatches > 0 , 
					 "The requested VGPR and LDS limits cannot be satisfied for the given shaders. Please adjust maxHsVgprCount and maxHsLdsBytes accordingly.\n"
					 "  - maxHsVgprCount must be greater or equal than: %i\n"
					 "  - maxHsLdsBytes must be greater or equal than:  %i\n",
					 hsb->getNumVgprs(), sce::Gnm::computeLdsUsagePerPatchInBytesPerThreadGroup(&hsb->m_hullStateConstants, lsb->m_lsStride));

	// The GPU contains two VGTs and for good results we wish to toggle between the two
	// for every (approximately) 256 vertices. Vertex caching is disabled when using tessellation
	// so we can determine the max. number of input patches to 256 vertices by the following
	uint32_t vgtPrimCount = 256 / hsb->m_hullStateConstants.m_numInputCP;

	// It is most efficient to send a number of patches through a VGT which is a multiple
	// of numPatches since the TGs are formed by the VGTs. Otherwise we get
	// a partially filled TG just before each toggle between VGTs.
	if(vgtPrimCount > tempNumPatches)
	{
		vgtPrimCount -= (vgtPrimCount % tempNumPatches); // closest multiple
		*outPatchCount = tempNumPatches; // patch count is unchanged
	}
	else
	{
		*outPatchCount = vgtPrimCount; // if numPatches>vgtPrimCount, reduse LDS consumption by adjusting patch count
	}

	*outVgtPrimCount = vgtPrimCount;
}


void sce::Gnmx::computeVgtPrimitiveCountAndAdjustNumPatches(uint32_t *outVgtPrimCount, uint32_t *inoutPatchCount, const HsShader *hsb)
{
	SCE_GNM_VALIDATE(hsb != 0, "hsb must not be NULL.");
	SCE_GNM_VALIDATE(outVgtPrimCount, "outVgtPrimCount must not be NULL.");
	SCE_GNM_VALIDATE(inoutPatchCount, "inoutPatchCount must not be NULL.");
	SCE_GNM_VALIDATE(*inoutPatchCount > 0 , "The input value of inoutPatchCount must greater than 0.");

	const uint32_t requestedNumPatchCount = *inoutPatchCount;

	// The GPU contains two VGTs and for good results we wish to toggle between the two
	// for every (approximately) 256 vertices. Vertex caching is disabled when using tessellation
	// so we can determine the max. number of input patches to 256 vertices by the following
	uint32_t vgtPrimCount = 256 / hsb->m_hullStateConstants.m_numInputCP;

	// It is most efficient to send a number of patches through a VGT which is a multiple
	// of numPatches since the TGs are formed by the VGTs. Otherwise we get
	// a partially filled TG just before each toggle between VGTs.
	if ( vgtPrimCount > requestedNumPatchCount )
	{
		vgtPrimCount -= (vgtPrimCount % requestedNumPatchCount); // closest multiple
	}
	else
	{
		*inoutPatchCount = vgtPrimCount; // if numPatches>vgtPrimCount, reduse LDS consumption by adjusting patch count
	}

	*outVgtPrimCount = vgtPrimCount;
}

bool sce::Gnmx::computeOnChipGsConfiguration(uint32_t *outLdsSizeIn512Bytes, uint32_t *outGsPrimsPerSubGroup, EsShader const* esb, GsShader const* gsb, uint32_t maxLdsUsage)
{
	SCE_GNM_VALIDATE(esb != NULL, "esb must not be NULL.");
	SCE_GNM_VALIDATE(gsb != NULL, "gsb must not be NULL.");
	SCE_GNM_VALIDATE(maxLdsUsage <= 64*1024, "maxLdsUsage must be no greater than 64KB.");
	SCE_GNM_VALIDATE(outLdsSizeIn512Bytes != NULL && outGsPrimsPerSubGroup != NULL, "outLdsSizeIn512Bytes and outGsPrimsPerSubGroup must not be NULL.");
	uint32_t gsSizePerPrim = gsb->getSizePerPrimitiveInBytes();
	if (gsSizePerPrim > maxLdsUsage) {
		*outGsPrimsPerSubGroup = *outLdsSizeIn512Bytes = 0;
		return false;
	}
	uint32_t gsPrimsPerSubGroup = maxLdsUsage / gsSizePerPrim;
	// Creating on-chip GS sub-groups with a multiple of 64 will ensure that all ES and GS
	// wavefronts will all have full thread utilization.
	// Creating on-chip-GS sub-groups with greater than 64 primitives might slightly
	// increase the thread utilization of VS wavefronts, but it is usually better to create
	// the smallest possible sub-groups that can reach maximum thread occupancy, as
	// smaller sub-groups will do a better job of finding holes and filling GPU resources.

	// For the specific case of Patch32 input topology, 64 GS prims * 32 input vertices/prim =
	// 2048 ES vertex would just exceed the GPU maximum of 2047 ES verts per sub-group, and
	// we must reduce the GS prims per sub-group to 63.
	uint32_t maxGsPrimsPerSubGroupForInputVertexCount = (gsb->m_inputVertexCountMinus1 == 31) ? 63 : 64;

	if (gsPrimsPerSubGroup > maxGsPrimsPerSubGroupForInputVertexCount)
		gsPrimsPerSubGroup = maxGsPrimsPerSubGroupForInputVertexCount;
	*outGsPrimsPerSubGroup = gsPrimsPerSubGroup;
	uint32_t ldsSizeInBytes = (gsPrimsPerSubGroup * gsSizePerPrim);
	*outLdsSizeIn512Bytes = (ldsSizeInBytes + 511) / 512;
	return true;
}

void sce::Gnmx::fillData(DrawCommandBuffer *dcb, void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL");
	SCE_GNM_VALIDATE(numBytes % 4 == 0, "numBytes (0x%08X) must be a multiple of 4 bytes.", numBytes);

	if ( numBytes == 0 )
		return;

	const uint32_t kMaxDmaDWSize = Gnm::kDmaMaximumSizeInBytes & ~0x3;

	const uint32_t	maximizedDmaCount = numBytes / kMaxDmaDWSize;
	const uint32_t	partialDmaSize    = numBytes - maximizedDmaCount * kMaxDmaDWSize;

	const uint32_t	numNonBlockingDmas = maximizedDmaCount - (partialDmaSize? 0 : 1);
	const uint32_t	finalDmaSize	   = partialDmaSize ? partialDmaSize : kMaxDmaDWSize;

	for (uint32_t iNonBlockingDma = 0; iNonBlockingDma < numNonBlockingDmas; ++iNonBlockingDma)
	{
		dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
					 Gnm::kDmaDataSrcData, (uint64_t)srcData,
					 kMaxDmaDWSize, Gnm::kDmaDataBlockingDisable); // only the final transfer is blocking
		dstGpuAddr = (uint8_t*)dstGpuAddr + kMaxDmaDWSize;
	}

	dcb->dmaData(Gnm::kDmaDataDstMemory,  (uint64_t)dstGpuAddr,
				 Gnm::kDmaDataSrcData, (uint64_t)srcData,
				 finalDmaSize, isBlocking);
}
void sce::Gnmx::fillData(MeasureDrawCommandBuffer *dcb, void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL");
	SCE_GNM_VALIDATE(numBytes % 4 == 0, "numBytes (0x%08X) must be a multiple of 4 bytes.", numBytes);

	if ( numBytes == 0 )
		return;

	const uint32_t kMaxDmaDWSize = Gnm::kDmaMaximumSizeInBytes & ~0x3;

	const uint32_t	maximizedDmaCount = numBytes / kMaxDmaDWSize;
	const uint32_t	partialDmaSize    = numBytes - maximizedDmaCount * kMaxDmaDWSize;

	const uint32_t	numNonBlockingDmas = maximizedDmaCount - (partialDmaSize? 0 : 1);
	const uint32_t	finalDmaSize	   = partialDmaSize ? partialDmaSize : kMaxDmaDWSize;

	for (uint32_t iNonBlockingDma = 0; iNonBlockingDma < numNonBlockingDmas; ++iNonBlockingDma)
	{
		dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
			Gnm::kDmaDataSrcData, (uint64_t)srcData,
			kMaxDmaDWSize, Gnm::kDmaDataBlockingDisable); // only the final transfer is blocking
		dstGpuAddr = (uint8_t*)dstGpuAddr + kMaxDmaDWSize;
	}

	dcb->dmaData(Gnm::kDmaDataDstMemory,  (uint64_t)dstGpuAddr,
		Gnm::kDmaDataSrcData, (uint64_t)srcData,
		finalDmaSize, isBlocking);
}

void sce::Gnmx::copyData(DrawCommandBuffer *dcb, void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL");
	if ( numBytes == 0 )
		return;

	const uint32_t		kMaxDmaDWSize	   = Gnm::kDmaMaximumSizeInBytes & ~0x3;	// to ensure DW alignment

	const uint32_t		maximizedDmaCount  = numBytes / kMaxDmaDWSize;
	const uint32_t		partialDmaSize	   = numBytes - maximizedDmaCount * kMaxDmaDWSize;

	const uint32_t		numNonBlockingDmas = maximizedDmaCount - (partialDmaSize? 0 : 1);
	const uint32_t		finalDmaSize	   = partialDmaSize ? partialDmaSize : kMaxDmaDWSize;

	for (uint32_t iNonBlockingDma = 0; iNonBlockingDma < numNonBlockingDmas; ++iNonBlockingDma)
	{
		dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
			Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
			kMaxDmaDWSize, Gnm::kDmaDataBlockingDisable); // only the final transfer is blocking
		dstGpuAddr = (uint8_t*)dstGpuAddr + kMaxDmaDWSize;
		srcGpuAddr = (uint8_t*)srcGpuAddr + kMaxDmaDWSize;
	}

	dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
		Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
		finalDmaSize, isBlocking);
}
void sce::Gnmx::copyData(MeasureDrawCommandBuffer *dcb, void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL");
	if ( numBytes == 0 )
		return;

	const uint32_t		kMaxDmaDWSize	   = Gnm::kDmaMaximumSizeInBytes & ~0x3;	// to ensure DW alignment

	const uint32_t		maximizedDmaCount  = numBytes / kMaxDmaDWSize;
	const uint32_t		partialDmaSize	   = numBytes - maximizedDmaCount * kMaxDmaDWSize;

	const uint32_t		numNonBlockingDmas = maximizedDmaCount - (partialDmaSize? 0 : 1);
	const uint32_t		finalDmaSize	   = partialDmaSize ? partialDmaSize : kMaxDmaDWSize;

	for (uint32_t iNonBlockingDma = 0; iNonBlockingDma < numNonBlockingDmas; ++iNonBlockingDma)
	{
		dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
			Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
			kMaxDmaDWSize, Gnm::kDmaDataBlockingDisable); // only the final transfer is blocking
		dstGpuAddr = (uint8_t*)dstGpuAddr + kMaxDmaDWSize;
		srcGpuAddr = (uint8_t*)srcGpuAddr + kMaxDmaDWSize;
	}

	dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
		Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
		finalDmaSize, isBlocking);
}

void* sce::Gnmx::embedData(DrawCommandBuffer *dcb, const void *dataStream, uint32_t sizeInDword, EmbeddedDataAlignment alignment)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL");
	const uint32_t sizeInByte = sizeInDword*sizeof(uint32_t);
	void* dest = dcb->allocateFromCommandBuffer(sizeInByte,alignment);
	if (dest != NULL)
	{
		memcpy(dest, dataStream, sizeInByte);
	}
	return dest;
}
void* sce::Gnmx::embedData(MeasureDrawCommandBuffer *dcb, const void *, uint32_t sizeInDword, EmbeddedDataAlignment alignment)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL");
	const uint32_t sizeInByte = sizeInDword*sizeof(uint32_t);
	dcb->allocateFromCommandBuffer(sizeInByte,alignment);
	return NULL;
	
}

//01234567  89ABCDEF
//01234567 -87654321
void sce::Gnmx::setAaDefaultSampleLocations(DrawCommandBuffer *dcb, NumSamples numAASamples)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL");
	dcb->pushMarker("GfxContext::SetAASampleLocs");

	uint32_t msaa_sample_locs[16] = {0};
	uint64_t centroid_priority = 0;
	if (numAASamples == Gnm::kNumSamples16)
	{
		msaa_sample_locs[0]  = 0x5BB137D9;
		msaa_sample_locs[1]  = 0x1FF5739D;
		msaa_sample_locs[2]  = 0x6E8224A8;
		msaa_sample_locs[3]  = 0x0AC640EC;
		msaa_sample_locs[4]  = 0x5BB137D9;
		msaa_sample_locs[5]  = 0x1FF5739D;
		msaa_sample_locs[6]  = 0x6E8224A8;
		msaa_sample_locs[7]  = 0x0AC640EC;
		msaa_sample_locs[8]  = 0x5BB137D9;
		msaa_sample_locs[9]  = 0x1FF5739D;
		msaa_sample_locs[10] = 0x6E8224A8;
		msaa_sample_locs[11] = 0x0AC640EC;
		msaa_sample_locs[12] = 0x5BB137D9;
		msaa_sample_locs[13] = 0x1FF5739D;
		msaa_sample_locs[14] = 0x6E8224A8;
		msaa_sample_locs[15] = 0x0AC640EC;
		centroid_priority = 0xFEDCBA9876543210ULL;
	}
	else if (numAASamples == Gnm::kNumSamples8)
	{
		msaa_sample_locs[0]  = 0x5BB137D9;
		msaa_sample_locs[1]  = 0x1FF5739D;
		msaa_sample_locs[2]  = 0x5BB137D9;
		msaa_sample_locs[3]  = 0x1FF5739D;
		msaa_sample_locs[4]  = 0x5BB137D9;
		msaa_sample_locs[5]  = 0x1FF5739D;
		msaa_sample_locs[6]  = 0x5BB137D9;
		msaa_sample_locs[7]  = 0x1FF5739D;
		msaa_sample_locs[8]  = 0x5BB137D9;
		msaa_sample_locs[9]  = 0x1FF5739D;
		msaa_sample_locs[10] = 0x5BB137D9;
		msaa_sample_locs[11] = 0x1FF5739D;
		msaa_sample_locs[12] = 0x5BB137D9;
		msaa_sample_locs[13] = 0x1FF5739D;
		msaa_sample_locs[14] = 0x5BB137D9;
		msaa_sample_locs[15] = 0x1FF5739D;
		centroid_priority = 0x7654321076543210LL;
	}
	else if (numAASamples == Gnm::kNumSamples4)
	{
		msaa_sample_locs[0]  = 0x22EEA66A;
		msaa_sample_locs[1]  = 0x00000000;
		msaa_sample_locs[2]  = 0x00000000;
		msaa_sample_locs[3]  = 0x00000000;
		msaa_sample_locs[4]  = 0x22EEA66A;
		msaa_sample_locs[5]  = 0x00000000;
		msaa_sample_locs[6]  = 0x00000000;
		msaa_sample_locs[7]  = 0x00000000;
		msaa_sample_locs[8]  = 0x22EEA66A;
		msaa_sample_locs[9]  = 0x00000000;
		msaa_sample_locs[10] = 0x00000000;
		msaa_sample_locs[11] = 0x00000000;
		msaa_sample_locs[12] = 0x22EEA66A;
		msaa_sample_locs[13] = 0x00000000;
		msaa_sample_locs[14] = 0x00000000;
		msaa_sample_locs[15] = 0x00000000;
		centroid_priority = 0x0000000000003210LL;
	}
	else if (numAASamples == Gnm::kNumSamples2)
	{
		msaa_sample_locs[0]  = 0x0000C44C;
		msaa_sample_locs[1]  = 0x00000000;
		msaa_sample_locs[2]  = 0x00000000;
		msaa_sample_locs[3]  = 0x00000000;
		msaa_sample_locs[4]  = 0x0000C44C;
		msaa_sample_locs[5]  = 0x00000000;
		msaa_sample_locs[6]  = 0x00000000;
		msaa_sample_locs[7]  = 0x00000000;
		msaa_sample_locs[8]  = 0x0000C44C;
		msaa_sample_locs[9]  = 0x00000000;
		msaa_sample_locs[10] = 0x00000000;
		msaa_sample_locs[11] = 0x00000000;
		msaa_sample_locs[12] = 0x0000C44C;
		msaa_sample_locs[13] = 0x00000000;
		msaa_sample_locs[14] = 0x00000000;
		msaa_sample_locs[15] = 0x00000000;
		centroid_priority = 0x0000000000000010LL;
	}
	dcb->setCentroidPriority(centroid_priority);
	dcb->setAaSampleLocations(msaa_sample_locs);
	//SetRegisterField(normal, PA_SC_CENTROID_PRIORITY_0, DISTANCE_0, priority_index[0]);
	dcb->popMarker();
}
void sce::Gnmx::setAaDefaultSampleLocations(MeasureDrawCommandBuffer *dcb, NumSamples )
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL");
	dcb->pushMarker("GfxContext::SetAASampleLocs");

	uint32_t msaa_sample_locs[16] = {0};
	uint64_t centroid_priority = 0;
	
	dcb->setCentroidPriority(centroid_priority);
	dcb->setAaSampleLocations(msaa_sample_locs);
	//SetRegisterField(normal, PA_SC_CENTROID_PRIORITY_0, DISTANCE_0, priority_index[0]);
	dcb->popMarker();
}

void sce::Gnmx::setupScreenViewport(DrawCommandBuffer *dcb, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	int32_t width = right - left;
	int32_t height = bottom - top;
	SCE_GNM_VALIDATE(width >= 0 && width <= 16367, "right (%d) - left (%d) must be in the range [0..16367].", right, left);
	SCE_GNM_VALIDATE(height >= 0 && height <= 16367, "bottom (%d) - top (%d) must be in the range [0..16367].", bottom, top);

	// viewport transform
	// Set viewport for to the entire render surface, with a screen-space Z range of 0 to 1
	float scale[3] = {(float)(width)*0.5f, - (float)(height)*0.5f, zScale};
	float offset[3] = {(float)(left + width*0.5f), (float)(top + height*0.5f), zOffset};
	dcb->setViewport(0, 0.0f, 1.0f, scale, offset);
	// Disable viewport pass-through (which would prevent the viewport transform from occurring at all)
	Gnm::ViewportTransformControl vpc;
	vpc.init();
	vpc.setPassThroughEnable(false);
	dcb->setViewportTransformControl(vpc);

	// Set screen scissor to cover the entire render surface. It is CRITICAL that the scissor region not
	// extend beyond the bounds of the render surface, or else memory corruption and crashes will occur.
	dcb->setScreenScissor(left, top, right, bottom);

	// Set the guard band offset so that the guard band is centered around the viewport region.
	// 10xx limits hardware offset to multiples of 16 pixels
	int hwOffsetX = (int)floor(offset[0]/16.0f + 0.5f);
	int hwOffsetY = (int)floor(offset[1]/16.0f + 0.5f);
	dcb->setHardwareScreenOffset(hwOffsetX, hwOffsetY);

	// Set the guard band clip distance to the maximum possible values by calculating the minimum distance
	// from the closest viewport edge to the edge of the hardware's coordinate space
	float hwMin = -(float)(1<<23) / (float)(1<<8);
	float hwMax =  (float)((1<<23) - 1) / (float)(1<<8);
	float gbMaxX = SCE_GNM_MIN(hwMax - fabsf(scale[0]) - offset[0] + hwOffsetX*16, -fabsf(scale[0]) + offset[0] - hwOffsetX*16 - hwMin);
	float gbMaxY = SCE_GNM_MIN(hwMax - fabsf(scale[1]) - offset[1] + hwOffsetY*16, -fabsf(scale[1]) + offset[1] - hwOffsetY*16 - hwMin);
	float gbHorizontalClipAdjust = gbMaxX / fabsf(scale[0]);
	float gbVerticalClipAdjust = gbMaxY / fabsf(scale[1]);
	dcb->setGuardBandClip(gbHorizontalClipAdjust, gbVerticalClipAdjust);

	// No need to adjust the guard band discard size
	// But if we skip this setting we see fliker with cold boot in ORBIS environment.
	dcb->setGuardBandDiscard(1.0f, 1.0f);
}

void sce::Gnmx::setupScreenViewport(MeasureDrawCommandBuffer *dcb, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, float zScale, float zOffset)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	int32_t width = right - left;
	int32_t height = bottom - top;
	SCE_GNM_VALIDATE(width >= 0 && width <= 16367, "right (%d) - left (%d) must be in the range [0..16367].", right, left);
	SCE_GNM_VALIDATE(height >= 0 && height <= 16367, "bottom (%d) - top (%d) must be in the range [0..16367].", bottom, top);

	// viewport transform
	// Set viewport for to the entire render surface, with a screen-space Z range of 0 to 1
	float scale[3] = {(float)(width)*0.5f, - (float)(height)*0.5f, zScale};
	float offset[3] = {(float)(left + width*0.5f), (float)(top + height*0.5f), zOffset};
	dcb->setViewport(0, 0.0f, 1.0f, scale, offset);
	// Disable viewport pass-through (which would prevent the viewport transform from occurring at all)
	Gnm::ViewportTransformControl vpc;
	vpc.init();
	vpc.setPassThroughEnable(false);
	dcb->setViewportTransformControl(vpc);

	// Set screen scissor to cover the entire render surface. It is CRITICAL that the scissor region not
	// extend beyond the bounds of the render surface, or else memory corruption and crashes will occur.
	dcb->setScreenScissor(left, top, right, bottom);

	// Set the guard band offset so that the guard band is centered around the viewport region.
	// 10xx limits hardware offset to multiples of 16 pixels
	int hwOffsetX = (int)floor(offset[0]/16.0f + 0.5f);
	int hwOffsetY = (int)floor(offset[1]/16.0f + 0.5f);
	dcb->setHardwareScreenOffset(hwOffsetX, hwOffsetY);

	// Set the guard band clip distance to the maximum possible values by calculating the minimum distance
	// from the closest viewport edge to the edge of the hardware's coordinate space
	float hwMin = -(float)(1<<23) / (float)(1<<8);
	float hwMax =  (float)((1<<23) - 1) / (float)(1<<8);
	float gbMaxX = SCE_GNM_MIN(hwMax - fabsf(scale[0]) - offset[0] + hwOffsetX*16, -fabsf(scale[0]) + offset[0] - hwOffsetX*16 - hwMin);
	float gbMaxY = SCE_GNM_MIN(hwMax - fabsf(scale[1]) - offset[1] + hwOffsetY*16, -fabsf(scale[1]) + offset[1] - hwOffsetY*16 - hwMin);
	float gbHorizontalClipAdjust = gbMaxX / fabsf(scale[0]);
	float gbVerticalClipAdjust = gbMaxY / fabsf(scale[1]);
	dcb->setGuardBandClip(gbHorizontalClipAdjust, gbVerticalClipAdjust);

	// No need to adjust the guard band discard size
	// But if we skip this setting we see fliker with cold boot in ORBIS environment.
	dcb->setGuardBandDiscard(1.0f, 1.0f);
}

void sce::Gnmx::clearAppendConsumeCounters(DrawCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, uint32_t clearValue)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	return dcb->dmaData(Gnm::kDmaDataDstGds, destRangeByteOffset + startApiSlot*sizeof(uint32_t),
		Gnm::kDmaDataSrcData, clearValue,
		numApiSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
void sce::Gnmx::clearAppendConsumeCounters(MeasureDrawCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, uint32_t clearValue)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	dcb->dmaData(Gnm::kDmaDataDstGds, destRangeByteOffset + startApiSlot*sizeof(uint32_t),
		Gnm::kDmaDataSrcData, clearValue,
		numApiSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}

void sce::Gnmx::writeAppendConsumeCounters(DrawCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, const void *srcGpuAddr)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	return dcb->dmaData(Gnm::kDmaDataDstGds, destRangeByteOffset + startApiSlot*sizeof(uint32_t),
		Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
		numApiSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
void sce::Gnmx::writeAppendConsumeCounters(MeasureDrawCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots, const void *srcGpuAddr)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	dcb->dmaData(Gnm::kDmaDataDstGds, destRangeByteOffset + startApiSlot*sizeof(uint32_t),
		Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
		numApiSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}

void sce::Gnmx::readAppendConsumeCounters(DrawCommandBuffer *dcb, void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	return dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)destGpuAddr,
		Gnm::kDmaDataSrcGds, srcRangeByteOffset + startApiSlot*sizeof(uint32_t),
		numApiSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
void sce::Gnmx::readAppendConsumeCounters(MeasureDrawCommandBuffer *dcb, void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startApiSlot, uint32_t numApiSlots)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)destGpuAddr,
		Gnm::kDmaDataSrcGds, srcRangeByteOffset + startApiSlot*sizeof(uint32_t),
		numApiSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef SCE_GNM_HP3D
void sce::Gnmx::fillData(DispatchCommandBuffer *dcb, void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	SCE_GNM_VALIDATE(numBytes % 4 == 0, "numBytes (%d) must be a multiple of 4 bytes.", numBytes);
	const uint32_t kMaxDmaDWSize = Gnm::kDmaMaximumSizeInBytes & ~0x3;

	const uint32_t	maximizedDmaCount = numBytes / kMaxDmaDWSize;
	const uint32_t	partialDmaSize    = numBytes - maximizedDmaCount * kMaxDmaDWSize;

	const uint32_t	numNonBlockingDmas = maximizedDmaCount - (partialDmaSize? 0 : 1);
	const uint32_t	finalDmaSize	   = partialDmaSize ? partialDmaSize : maximizedDmaCount;

	for (uint32_t iNonBlockingDma = 0; iNonBlockingDma < numNonBlockingDmas; ++iNonBlockingDma)
	{
		dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
						Gnm::kDmaDataSrcData, srcData,
						kMaxDmaDWSize, Gnm::kDmaDataBlockingDisable);
		dstGpuAddr = (uint8_t*)dstGpuAddr + kMaxDmaDWSize;
	}

	dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
					Gnm::kDmaDataSrcData, srcData,
					finalDmaSize, isBlocking);
}
void sce::Gnmx::fillData(MeasureDispatchCommandBuffer *dcb, void *dstGpuAddr, uint32_t srcData, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	SCE_GNM_VALIDATE(numBytes % 4 == 0, "numBytes (%d) must be a multiple of 4 bytes.", numBytes);
	const uint32_t kMaxDmaDWSize = Gnm::kDmaMaximumSizeInBytes & ~0x3;

	const uint32_t	maximizedDmaCount = numBytes / kMaxDmaDWSize;
	const uint32_t	partialDmaSize    = numBytes - maximizedDmaCount * kMaxDmaDWSize;

	const uint32_t	numNonBlockingDmas = maximizedDmaCount - (partialDmaSize? 0 : 1);
	const uint32_t	finalDmaSize	   = partialDmaSize ? partialDmaSize : maximizedDmaCount;

	for (uint32_t iNonBlockingDma = 0; iNonBlockingDma < numNonBlockingDmas; ++iNonBlockingDma)
	{
		dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
			Gnm::kDmaDataSrcData, srcData,
			kMaxDmaDWSize, Gnm::kDmaDataBlockingDisable);
		dstGpuAddr = (uint8_t*)dstGpuAddr + kMaxDmaDWSize;
	}

	dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
		Gnm::kDmaDataSrcData, srcData,
		finalDmaSize, isBlocking);
}
void sce::Gnmx::copyData(DispatchCommandBuffer *dcb, void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	const uint32_t		kMaxDmaDWSize	   = Gnm::kDmaMaximumSizeInBytes & ~0x3;	// to ensure DW alignment

	const uint32_t		maximizedDmaCount  = numBytes / kMaxDmaDWSize;
	const uint32_t		partialDmaSize	   = numBytes - maximizedDmaCount * kMaxDmaDWSize;

	const uint32_t		numNonBlockingDmas = maximizedDmaCount - (partialDmaSize? 0 : 1);
	const uint32_t		finalDmaSize	   = partialDmaSize ? partialDmaSize : maximizedDmaCount;

	for (uint32_t iNonBlockingDma = 0; iNonBlockingDma < numNonBlockingDmas; ++iNonBlockingDma)
	{
		dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
						Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
						kMaxDmaDWSize, Gnm::kDmaDataBlockingDisable);
		dstGpuAddr = (uint8_t*)dstGpuAddr + kMaxDmaDWSize;
		srcGpuAddr = (uint8_t*)srcGpuAddr + kMaxDmaDWSize;
	}
	dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
					Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
					finalDmaSize, isBlocking);
}
void sce::Gnmx::copyData(MeasureDispatchCommandBuffer *dcb, void *dstGpuAddr, const void *srcGpuAddr, uint32_t numBytes, Gnm::DmaDataBlockingMode isBlocking)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	const uint32_t		kMaxDmaDWSize	   = Gnm::kDmaMaximumSizeInBytes & ~0x3;	// to ensure DW alignment

	const uint32_t		maximizedDmaCount  = numBytes / kMaxDmaDWSize;
	const uint32_t		partialDmaSize	   = numBytes - maximizedDmaCount * kMaxDmaDWSize;

	const uint32_t		numNonBlockingDmas = maximizedDmaCount - (partialDmaSize? 0 : 1);
	const uint32_t		finalDmaSize	   = partialDmaSize ? partialDmaSize : maximizedDmaCount;

	for (uint32_t iNonBlockingDma = 0; iNonBlockingDma < numNonBlockingDmas; ++iNonBlockingDma)
	{
		dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
			Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
			kMaxDmaDWSize, Gnm::kDmaDataBlockingDisable);
		dstGpuAddr = (uint8_t*)dstGpuAddr + kMaxDmaDWSize;
		srcGpuAddr = (uint8_t*)srcGpuAddr + kMaxDmaDWSize;
	}
	dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)dstGpuAddr,
		Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
		finalDmaSize, isBlocking);
}
void* sce::Gnmx::embedData(DispatchCommandBuffer *dcb, const void *dataStream, uint32_t sizeInDword, sce::Gnm::EmbeddedDataAlignment alignment)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	const uint32_t sizeInByte = sizeInDword*sizeof(uint32_t);
	void* dest = dcb->allocateFromCommandBuffer(sizeInByte,alignment);
	if (dest != NULL)
	{
		memcpy(dest, dataStream, sizeInByte);
	}

	return dest;
}
void* sce::Gnmx::embedData(MeasureDispatchCommandBuffer *dcb, const void *, uint32_t sizeInDword, sce::Gnm::EmbeddedDataAlignment alignment)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	const uint32_t sizeInByte = sizeInDword*sizeof(uint32_t);
	dcb->allocateFromCommandBuffer(sizeInByte,alignment);
	
	return NULL;
}
void sce::Gnmx::clearAppendConsumeCounters(DispatchCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startSlot, uint32_t numSlots, uint32_t clearValue)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	return dcb->dmaData(Gnm::kDmaDataDstGds, destRangeByteOffset + startSlot*sizeof(uint32_t),
						   Gnm::kDmaDataSrcData, clearValue,
						   numSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
void sce::Gnmx::clearAppendConsumeCounters(MeasureDispatchCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startSlot, uint32_t numSlots, uint32_t clearValue)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	dcb->dmaData(Gnm::kDmaDataDstGds, destRangeByteOffset + startSlot*sizeof(uint32_t),
		Gnm::kDmaDataSrcData, clearValue,
		numSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
void sce::Gnmx::writeAppendConsumeCounters(DispatchCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startSlot, uint32_t numSlots, const void *srcGpuAddr)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	return dcb->dmaData(Gnm::kDmaDataDstGds, destRangeByteOffset + startSlot*sizeof(uint32_t),
						   Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
						   numSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
void sce::Gnmx::writeAppendConsumeCounters(MeasureDispatchCommandBuffer *dcb, uint32_t destRangeByteOffset, uint32_t startSlot, uint32_t numSlots, const void *srcGpuAddr)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	dcb->dmaData(Gnm::kDmaDataDstGds, destRangeByteOffset + startSlot*sizeof(uint32_t),
		Gnm::kDmaDataSrcMemory, (uint64_t)srcGpuAddr,
		numSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}

void sce::Gnmx::readAppendConsumeCounters(DispatchCommandBuffer *dcb, void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startSlot, uint32_t numSlots)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	return dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)destGpuAddr,
						   Gnm::kDmaDataSrcGds, srcRangeByteOffset + startSlot*sizeof(uint32_t),
						   numSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
void sce::Gnmx::readAppendConsumeCounters(MeasureDispatchCommandBuffer *dcb, void *destGpuAddr, uint32_t srcRangeByteOffset, uint32_t startSlot, uint32_t numSlots)
{
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	// dmaData knows what ring it's running on, and addresses the appropriate segment of GDS accordingly.
	dcb->dmaData(Gnm::kDmaDataDstMemory, (uint64_t)destGpuAddr,
		Gnm::kDmaDataSrcGds, srcRangeByteOffset + startSlot*sizeof(uint32_t),
		numSlots*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
#endif // SCE_GNM_HP3D
