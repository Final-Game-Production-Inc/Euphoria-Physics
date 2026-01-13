/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/gfxcontext.h"

#include <gnm/buffer.h>
#include <gnm/gpumem.h>
#include <gnm/offline.h>
#include <gnm/platform.h>
#include <gnm/rendertarget.h>
#include <gnm/tessellation.h>
#include "grcore/gnmx/dispatchdraw.h"

#include <algorithm>

using namespace sce::Gnm;
using namespace sce::Gnmx;

namespace
{
	const uint32_t kSerializedContextTempHeaderVersion = 2;
	const uint32_t kSerializedContextTempHeaderMagic = 0x00544347; // "GCT\0"
	struct SerializedContextTempHeader
	{
		uint32_t m_magic;
		uint32_t m_version;

		uint32_t m_cueHeapSize;
		uint32_t m_submissionRangesByteOffset; ///< byte offset from the beginning of the file to the beginning of the SubmissionRanges[] array
		uint32_t m_submissionRangeCount;
		uint32_t m_dcbPatchTableByteOffset;
		uint32_t m_dcbPatchTableSizeInBytes;
		uint32_t m_acbPatchTableByteOffset;
		uint32_t m_acbPatchTableSizeInBytes;
		uint32_t m_ccbPatchTableByteOffset;
		uint32_t m_ccbPatchTableSizeInBytes;
	};

	const uint32_t kSerializedContextPersistentHeaderVersion = 2;
	const uint32_t kSerializedContextPersistentHeaderMagic = 0x00504347; // "GCP\0"
	struct SerializedContextPersistentHeader
	{
		uint32_t m_magic;
		uint32_t m_version;

		uint32_t m_dcbByteOffset;
		uint32_t m_dcbSizeInDwords;
		uint32_t m_acbByteOffset;
		uint32_t m_acbSizeInDwords;
		uint32_t m_ccbByteOffset;
		uint32_t m_ccbSizeInDwords;
	};
}

static bool handleReserveFailed(CommandBuffer *cb, uint32_t dwordCount, void *userData)
{
	GfxContext &gfxc = *(GfxContext*)userData;
	// If either command buffer has actually reached the end of its full buffer, then invoke m_bufferFullCallback (if one has been registered)
	if (static_cast<void*>(cb) == static_cast<void*>(&(gfxc.m_dcb)) && gfxc.m_dcb.m_cmdptr + dwordCount > gfxc.m_actualDcbEnd)
	{
		if (gfxc.m_bufferFullCallback.m_func != 0)
			return gfxc.m_bufferFullCallback.m_func(cb, dwordCount, gfxc.m_bufferFullCallback.m_userData);
		else
		{
			SCE_GNM_ERROR("Out of DCB command buffer space, and no callback bound.");
			return false;
		}
	}
	if (static_cast<void*>(cb) == static_cast<void*>(&(gfxc.m_ccb)) && gfxc.m_ccb.m_cmdptr + dwordCount > gfxc.m_actualCcbEnd)
	{
		if (gfxc.m_bufferFullCallback.m_func != 0)
			return gfxc.m_bufferFullCallback.m_func(cb, dwordCount, gfxc.m_bufferFullCallback.m_userData);
		else
		{
			SCE_GNM_ERROR("Out of CCB command buffer space, and no callback bound.");
			return false;
		}
	}
	// Register a new submit up to the current DCB/CCB command pointers
	if (gfxc.m_submissionCount >= GfxContext::kMaxNumStoredSubmissions)
	{
		SCE_GNM_VALIDATE(gfxc.m_submissionCount < GfxContext::kMaxNumStoredSubmissions, "Out of space for stored submissions. More can be added by increasing kMaxNumStoredSubmissions.");
		return false;
	}
	gfxc.m_submissionRanges[gfxc.m_submissionCount].m_dcbStartDwordOffset = (uint32_t)(gfxc.m_currentDcbSubmissionStart - gfxc.m_dcb.m_beginptr);
	gfxc.m_submissionRanges[gfxc.m_submissionCount].m_dcbSizeInDwords     = (uint32_t)(gfxc.m_dcb.m_cmdptr - gfxc.m_currentDcbSubmissionStart);
	gfxc.m_submissionRanges[gfxc.m_submissionCount].m_ccbStartDwordOffset = (uint32_t)(gfxc.m_currentCcbSubmissionStart - gfxc.m_ccb.m_beginptr);
	gfxc.m_submissionRanges[gfxc.m_submissionCount].m_ccbSizeInDwords     = (uint32_t)(gfxc.m_ccb.m_cmdptr - gfxc.m_currentCcbSubmissionStart);
	gfxc.m_submissionCount++;
	gfxc.m_currentDcbSubmissionStart = gfxc.m_dcb.m_cmdptr;
	gfxc.m_currentCcbSubmissionStart = gfxc.m_ccb.m_cmdptr;
	// Advance CB end pointers to the next (possibly artificial) boundary -- either current+(4MB-4), or the end of the actual buffer
	gfxc.m_dcb.m_endptr = std::min(gfxc.m_dcb.m_cmdptr+kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)gfxc.m_actualDcbEnd);
	gfxc.m_ccb.m_endptr = std::min(gfxc.m_ccb.m_cmdptr+kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)gfxc.m_actualCcbEnd);
	return true;
}

GfxContext::GfxContext(void)
{
	m_acb.m_beginptr = m_acb.m_cmdptr = m_acb.m_endptr = NULL;
	m_currentAcbSubmissionStart = m_actualAcbEnd = NULL;
}

GfxContext::~GfxContext(void)
{
}

void GfxContext::init(void *cueCpRamShadowBuffer, void *cueHeapAddr, uint32_t numRingEntries,
					  void *dcbBuffer, uint32_t dcbSizeInBytes, void *ccbBuffer, uint32_t ccbSizeInBytes)
{
	m_dcb.init(dcbBuffer, std::min((uint32_t)Gnm::kIndirectBufferMaximumSizeInBytes, dcbSizeInBytes), handleReserveFailed, this);
	m_ccb.init(ccbBuffer, std::min((uint32_t)Gnm::kIndirectBufferMaximumSizeInBytes, ccbSizeInBytes), handleReserveFailed, this);
	m_cue.init(cueCpRamShadowBuffer, cueHeapAddr, numRingEntries);

#ifdef CUE_V2
	m_cue.bindCommandBuffers(&m_dcb, &m_ccb);
#endif // CUE_V2

	for(uint32_t iSub=0; iSub<kMaxNumStoredSubmissions; ++iSub)
	{
		m_submissionRanges[iSub].m_dcbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_dcbSizeInDwords = 0;
		m_submissionRanges[iSub].m_acbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_acbSizeInDwords = 0;
		m_submissionRanges[iSub].m_ccbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_ccbSizeInDwords = 0;
	}
	m_submissionCount = 0;
	m_currentDcbSubmissionStart = m_dcb.m_beginptr;
	m_currentCcbSubmissionStart = m_ccb.m_beginptr;
	m_actualDcbEnd = (uint32_t*)dcbBuffer+(dcbSizeInBytes/4);
	m_actualCcbEnd = (uint32_t*)ccbBuffer+(ccbSizeInBytes/4);
	m_bufferFullCallback.m_func = NULL;
	m_bufferFullCallback.m_userData = NULL;
#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
	m_recordLastCompletionMode = kRecordLastCompletionDisabled;
	m_addressOfOffsetOfLastCompletion = 0;
#endif
	m_dispatchDrawState = 255u << kDispatchDrawStateShiftPrimGroupSize;
}

void GfxContext::initDispatchDrawCommandBuffer(void *acbBuffer, uint32_t acbSizeInBytes)
{
	m_acb.init(acbBuffer, std::min((uint32_t)Gnm::kIndirectBufferMaximumSizeInBytes, acbSizeInBytes), handleReserveFailed, this);
	m_currentAcbSubmissionStart = m_acb.m_beginptr;
	m_actualAcbEnd = (uint32_t*)acbBuffer+(acbSizeInBytes/4);
	m_dispatchDrawState = 255u << kDispatchDrawStateShiftPrimGroupSize;	//setVgtControl(255, Gnm::kVgtPartialVsWaveDisable, Gnm::kVgtSwitchOnEopDisable);
}

uint32_t GfxContext::getRequiredSizeOfGdsDispatchDrawArea(uint32_t numKickRingBufferElems)
{
	uint32_t const kNumDispatchDrawCounters = 8;	// Currently using 4, but will need 4 more for VRB support
	SCE_GNM_VALIDATE(numKickRingBufferElems >= 2 && (numKickRingBufferElems*3 + kNumDispatchDrawCounters)*4 < kGdsAccessibleMemorySizeInBytes, "numKickRingBufferElems must be in the range [0:%u]", (kGdsAccessibleMemorySizeInBytes/4 - kNumDispatchDrawCounters)/3);
	return (numKickRingBufferElems*3 + kNumDispatchDrawCounters)*4;
}

void GfxContext::setupDispatchDrawRingBuffers(void *pIndexRingBuffer, uint32_t sizeofIndexRingBufferInBytes, uint32_t numKickRingBufferElems, uint32_t gdsOffsetDispatchDrawArea)
{
	uint32_t const kNumDispatchDrawCounters = 8;	// Currently using 4, but will need 4 more for VRB support
	uint32_t krbCount = numKickRingBufferElems, gdsDwOffsetKrb = gdsOffsetDispatchDrawArea/4, gdsDwOffsetKrbCounters = gdsDwOffsetKrb + krbCount*3, gdsDwOffsetIrbWptr = gdsDwOffsetKrbCounters + 3;
	uint32_t sizeofIndexRingBufferInIndices = sizeofIndexRingBufferInBytes>>1;
	SCE_GNM_VALIDATE(numKickRingBufferElems >= 2, "numKickRingBufferElems must be greater than or equal to 2");
	SCE_GNM_VALIDATE(!(gdsOffsetDispatchDrawArea & 0x3), "gdsOffsetDispatchDrawCounters must be 4 byte aligned");
	SCE_GNM_VALIDATE((gdsDwOffsetKrbCounters + kNumDispatchDrawCounters)*4 <= kGdsAccessibleMemorySizeInBytes, "gdsOffsetDispatchDrawCounters [0x%04x:0x%04x] does not fit in user accessible GDS area [0x%04x:0x%04x]", gdsOffsetDispatchDrawArea*4, (gdsDwOffsetKrbCounters + kNumDispatchDrawCounters)*4, 0, kGdsAccessibleMemorySizeInBytes);
	SCE_GNM_VALIDATE(!((uintptr_t)pIndexRingBuffer & 0xFF), "pIndexRingBuffer must be 256 byte aligned");
	SCE_GNM_VALIDATE(sizeofIndexRingBufferInBytes > 0 && !(sizeofIndexRingBufferInBytes & 0xFF), "sizeofIndexRingBufferInBytes must be a multiple of 256 bytes greater than 0");
	
	m_acb.dmaData(Gnm::kDmaDataDstGds, gdsDwOffsetKrbCounters*sizeof(uint32_t), Gnm::kDmaDataSrcData, (uint64_t)0, kNumDispatchDrawCounters*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);

	// Configure the dispatch draw index ring buffer
	m_acb.setupDispatchDrawIndexRingBuffer(gdsDwOffsetIrbWptr, sizeofIndexRingBufferInIndices);

	// Initialize the dispatch draw kick ring buffer
	m_ccb.incrementCeCounterForDispatchDraw();

	m_acb.setupDispatchDrawKickRingBuffer(krbCount, gdsDwOffsetKrb, gdsDwOffsetKrbCounters);
	m_acb.waitOnCe();

	m_dcb.waitForSetupDispatchDrawKickRingBuffer(krbCount, gdsDwOffsetKrb, gdsDwOffsetKrbCounters, pIndexRingBuffer, sizeofIndexRingBufferInBytes);
	m_dcb.waitOnCe();
	m_dcb.incrementDeCounter();

	Gnm::Buffer bufferIndexRingBuffer;
	bufferIndexRingBuffer.initAsDataBuffer(pIndexRingBuffer, Gnm::kDataFormatR16Uint, sizeofIndexRingBufferInIndices);
	bufferIndexRingBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypePV);	// the index ring buffer must be writeable and cached in the GPU L2 for performance.  It is never read via the K$
	m_cue.setVsharpInDispatchDrawData(0, &bufferIndexRingBuffer);
	m_cue.setDwordMaskedInDispatchDrawData(8, (gdsDwOffsetIrbWptr<<18), 0xFFFF0000);
	m_cue.setDwordInDispatchDrawData(9, sizeofIndexRingBufferInIndices);
	m_pIndexRingBuffer = pIndexRingBuffer;
}

void GfxContext::reset(void)
{
	m_cue.advanceFrame();
	m_dcb.resetBuffer();
	m_acb.resetBuffer();
	m_ccb.resetBuffer();
	// Restore end pointers to artificial limits
	m_dcb.m_endptr = std::min(m_dcb.m_cmdptr+Gnm::kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)m_actualDcbEnd);
	m_acb.m_endptr = std::min(m_acb.m_cmdptr+Gnm::kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)m_actualAcbEnd);
	m_ccb.m_endptr = std::min(m_ccb.m_cmdptr+Gnm::kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)m_actualCcbEnd);

	// Restore submit ranges to default values
	for(uint32_t iSub=0; iSub<kMaxNumStoredSubmissions; ++iSub)
	{
		m_submissionRanges[iSub].m_dcbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_dcbSizeInDwords = 0;
		m_submissionRanges[iSub].m_acbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_acbSizeInDwords = 0;
		m_submissionRanges[iSub].m_ccbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_ccbSizeInDwords = 0;
	}
	m_submissionCount = 0;
	m_currentDcbSubmissionStart = m_dcb.m_beginptr;
	m_currentAcbSubmissionStart = m_acb.m_beginptr;
	m_currentCcbSubmissionStart = m_ccb.m_beginptr;

	// Unbind all shaders in the CUE
	m_cue.setVsShader(NULL, 0, (void*)0);
	m_cue.setPsShader(NULL);
	m_cue.setCsShader(NULL);
	m_cue.setLsShader(NULL, 0, (void*)0);
	m_cue.setHsShader(NULL, NULL);
	m_cue.setEsShader(NULL, 0, (void*)0);
	m_cue.setGsVsShaders(NULL);
}

#if defined(SCE_GNM_OFFLINE_MODE)
void GfxContext::getSerializedSizes(size_t *outTempBufferSize, size_t *outPersistentBufferSize) const
{
	SCE_GNM_VALIDATE(outTempBufferSize         != NULL,         "outTempBufferSize must not be NULL.");
	SCE_GNM_VALIDATE(outPersistentBufferSize   != NULL,   "outPersistentBufferSize must not be NULL.");
	SCE_GNM_VALIDATE(m_dcb.m_patchTableBuilder != NULL, "m_dcb.m_patchTableBuilder must not be NULL.");
	SCE_GNM_VALIDATE(m_acb.m_beginptr == NULL || m_acb.m_patchTableBuilder != NULL, "m_acb.m_patchTableBuilder must not be NULL if m_acb has been initialized.");
	SCE_GNM_VALIDATE(m_ccb.m_patchTableBuilder != NULL, "m_ccb.m_patchTableBuilder must not be NULL.");
	size_t dcbPatchTableSize = m_dcb.m_patchTableBuilder->getSerializedSize();
	size_t acbPatchTableSize = m_acb.m_patchTableBuilder ? m_acb.m_patchTableBuilder->getSerializedSize() : 0;
	size_t ccbPatchTableSize = m_ccb.m_patchTableBuilder->getSerializedSize();

	size_t dcbSize = (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t);
	size_t acbSize = (m_acb.m_cmdptr - m_acb.m_beginptr) * sizeof(uint32_t);
	size_t ccbSize = (m_ccb.m_cmdptr - m_ccb.m_beginptr) * sizeof(uint32_t);

	size_t submissionTableSize = (m_submissionCount+1)*sizeof(SubmissionRange); // m_submissionRanges array, plus current DCB/CCB pair

	*outTempBufferSize = sizeof(SerializedContextTempHeader) + submissionTableSize + dcbPatchTableSize + acbPatchTableSize + ccbPatchTableSize;
	// CCB, ACB, and DCB must both start on a 256-byte boundary
	*outPersistentBufferSize = sizeof(SerializedContextPersistentHeader);
	if (dcbSize != 0)
		*outPersistentBufferSize = ((*outPersistentBufferSize + 0xFF) & ~0xFF) + dcbSize;
	if (acbSize != 0)
		*outPersistentBufferSize = ((*outPersistentBufferSize + 0xFF) & ~0xFF) + acbSize;
	if (ccbSize != 0)
		*outPersistentBufferSize = ((*outPersistentBufferSize + 0xFF) & ~0xFF) + ccbSize;
	return;
}

void GfxContext::serializeIntoBuffers(void *destTempBuffer, size_t tempBufferSize, void *destPersistentBuffer, size_t persistentBufferSize) const
{
#ifdef GNM_DEBUG
	size_t minTempBufferSize = 0, minPersistentBufferSize = 0;
	getSerializedSizes(&minTempBufferSize, &minPersistentBufferSize);
	SCE_GNM_VALIDATE(destTempBuffer, "destTempBuffer must not be NULL.");
	SCE_GNM_VALIDATE(tempBufferSize >= minTempBufferSize, "tempBufferSize (%d bytes) is smaller than the required size (%d bytes). Use getSerializedSize().", tempBufferSize, minTempBufferSize);
	SCE_GNM_VALIDATE(destPersistentBuffer, "destPersistentBuffer must not be NULL.");
	SCE_GNM_VALIDATE(persistentBufferSize >= minPersistentBufferSize, "persistentBufferSize (%d bytes) is smaller than the required size (%d bytes). Use getSerializedSize().", persistentBufferSize, minPersistentBufferSize);
	SCE_GNM_VALIDATE(m_dcb.m_patchTableBuilder != NULL, "m_dcb.m_patchTableBuilder must not be NULL.");
	SCE_GNM_VALIDATE(m_acb.m_beginptr == NULL || m_acb.m_patchTableBuilder != NULL, "m_acb.m_patchTableBuilder must not be NULL if m_acb has been initialized.");
	SCE_GNM_VALIDATE(m_ccb.m_patchTableBuilder != NULL, "m_ccb.m_patchTableBuilder must not be NULL.");
#endif

	//
	// Write temp buffer
	//

	memset(destTempBuffer, 0, tempBufferSize);
	uint8_t *tempBufferBytes = (uint8_t*)destTempBuffer;
	
	// Write header
	SerializedContextTempHeader tempHeader;
	tempHeader.m_magic = kSerializedContextTempHeaderMagic;
	tempHeader.m_version = kSerializedContextTempHeaderVersion;
	tempHeader.m_cueHeapSize = m_cue.getHeapSize();
	tempHeader.m_submissionRangesByteOffset = sizeof(SerializedContextTempHeader);
	tempHeader.m_submissionRangeCount = m_submissionCount+1; // include the submit currently under construction
	tempHeader.m_dcbPatchTableByteOffset  = tempHeader.m_submissionRangesByteOffset + tempHeader.m_submissionRangeCount * sizeof(SubmissionRange);
	tempHeader.m_dcbPatchTableSizeInBytes = (uint32_t)m_dcb.m_patchTableBuilder->getSerializedSize();
	tempHeader.m_acbPatchTableByteOffset  = tempHeader.m_dcbPatchTableByteOffset + tempHeader.m_dcbPatchTableSizeInBytes;
	tempHeader.m_acbPatchTableSizeInBytes = (uint32_t)(m_acb.m_patchTableBuilder ? m_acb.m_patchTableBuilder->getSerializedSize() : 0);
	tempHeader.m_ccbPatchTableByteOffset  = tempHeader.m_acbPatchTableByteOffset + tempHeader.m_acbPatchTableSizeInBytes;
	tempHeader.m_ccbPatchTableSizeInBytes = (uint32_t)m_ccb.m_patchTableBuilder->getSerializedSize();
	memcpy(tempBufferBytes, &tempHeader, sizeof(SerializedContextTempHeader));
	tempBufferBytes += sizeof(SerializedContextTempHeader);

	// Write SubmissionRange table
	SCE_GNM_VALIDATE(tempBufferBytes - (uint8_t*)destTempBuffer == (ptrdiff_t)tempHeader.m_submissionRangesByteOffset, "Incorrect byte offset for submit table");
	memcpy(tempBufferBytes, m_submissionRanges, m_submissionCount * sizeof(SubmissionRange));
	tempBufferBytes += m_submissionCount*sizeof(SubmissionRange);
	// Append the current submit to the table
	SubmissionRange currentSubmissionRange;
	currentSubmissionRange.m_dcbStartDwordOffset = (uint32_t)(m_currentDcbSubmissionStart - m_dcb.m_beginptr);
	currentSubmissionRange.m_dcbSizeInDwords     = (uint32_t)(m_dcb.m_cmdptr - m_currentDcbSubmissionStart);
	currentSubmissionRange.m_acbStartDwordOffset = (uint32_t)(m_currentAcbSubmissionStart - m_acb.m_beginptr);
	currentSubmissionRange.m_acbSizeInDwords     = (uint32_t)(m_acb.m_cmdptr - m_currentAcbSubmissionStart);
	currentSubmissionRange.m_ccbStartDwordOffset = (uint32_t)(m_currentCcbSubmissionStart - m_ccb.m_beginptr);
	currentSubmissionRange.m_ccbSizeInDwords     = (uint32_t)(m_ccb.m_cmdptr - m_currentCcbSubmissionStart);
	memcpy(tempBufferBytes, &currentSubmissionRange, sizeof(SubmissionRange));
	tempBufferBytes += sizeof(SubmissionRange);

	// Write DCB patch table
	SCE_GNM_VALIDATE(tempBufferBytes - (uint8_t*)destTempBuffer == (ptrdiff_t)tempHeader.m_dcbPatchTableByteOffset, "Incorrect byte offset for DCB patch table");
	m_dcb.m_patchTableBuilder->serializeIntoBuffer(tempBufferBytes, tempHeader.m_dcbPatchTableSizeInBytes);
	tempBufferBytes += tempHeader.m_dcbPatchTableSizeInBytes;

	if (m_acb.m_patchTableBuilder) {
		// Write ACB patch table
		SCE_GNM_VALIDATE(tempBufferBytes - (uint8_t*)destTempBuffer == (ptrdiff_t)tempHeader.m_acbPatchTableByteOffset, "Incorrect byte offset for ACB patch table");
		m_acb.m_patchTableBuilder->serializeIntoBuffer(tempBufferBytes, tempHeader.m_acbPatchTableSizeInBytes);
		tempBufferBytes += tempHeader.m_acbPatchTableSizeInBytes;
	}

	// Write CCB patch table
	SCE_GNM_VALIDATE(tempBufferBytes - (uint8_t*)destTempBuffer == (ptrdiff_t)tempHeader.m_ccbPatchTableByteOffset, "Incorrect byte offset for CCB patch table");
	m_ccb.m_patchTableBuilder->serializeIntoBuffer(tempBufferBytes, tempHeader.m_ccbPatchTableSizeInBytes);
	tempBufferBytes += tempHeader.m_ccbPatchTableSizeInBytes;

	SCE_GNM_VALIDATE(tempBufferBytes - (uint8_t*)destTempBuffer <= (ptrdiff_t)tempBufferSize, "Memory overwrite while writing to destTempBuffer (expected to write %d bytes; actually wrote %d bytes).",
		tempBufferSize, tempBufferBytes - (uint8_t*)destTempBuffer);

	//
	// Write persistent buffer
	//

	memset(destPersistentBuffer, 0, persistentBufferSize);
	uint8_t *persistentBufferBytes = (uint8_t*)destPersistentBuffer;

	// Write header
	SerializedContextPersistentHeader persistentHeader;
	persistentHeader.m_magic = kSerializedContextPersistentHeaderMagic;
	persistentHeader.m_version = kSerializedContextPersistentHeaderVersion;
	uint32_t offsetEnd = sizeof(SerializedContextPersistentHeader);
	persistentHeader.m_dcbSizeInDwords = (uint32_t)(m_dcb.m_cmdptr - m_dcb.m_beginptr);
	if (persistentHeader.m_dcbSizeInDwords) {
		persistentHeader.m_dcbByteOffset = (offsetEnd + 0xFF) & ~0xFF;
		offsetEnd = persistentHeader.m_dcbByteOffset + persistentHeader.m_dcbSizeInDwords;
	} else
		persistentHeader.m_dcbByteOffset = offsetEnd;
	persistentHeader.m_acbSizeInDwords = (uint32_t)(m_acb.m_cmdptr - m_acb.m_beginptr);
	if (persistentHeader.m_acbSizeInDwords) {
		persistentHeader.m_acbByteOffset = (offsetEnd + 0xFF) & ~0xFF;
		offsetEnd = persistentHeader.m_acbByteOffset + persistentHeader.m_acbByteOffset;
	} else
		persistentHeader.m_acbByteOffset = offsetEnd;
	persistentHeader.m_ccbSizeInDwords = (uint32_t)(m_ccb.m_cmdptr - m_ccb.m_beginptr);
	if (persistentHeader.m_ccbSizeInDwords) {
		persistentHeader.m_ccbByteOffset = (offsetEnd + 0xFF) & ~0xFF;
		offsetEnd = persistentHeader.m_ccbByteOffset + persistentHeader.m_ccbByteOffset;
	} else
		persistentHeader.m_ccbByteOffset = offsetEnd;
	memcpy(persistentBufferBytes, &persistentHeader, sizeof(SerializedContextPersistentHeader));
	
	// Write DCB
	if (persistentHeader.m_dcbSizeInDwords) {
		persistentBufferBytes = (uint8_t*)destPersistentBuffer + persistentHeader.m_dcbByteOffset;
		memcpy(persistentBufferBytes, m_dcb.m_beginptr, persistentHeader.m_dcbSizeInDwords*sizeof(uint32_t));
	}

	// Write ACB
	if (persistentHeader.m_acbSizeInDwords) {
		persistentBufferBytes = (uint8_t*)destPersistentBuffer + persistentHeader.m_acbByteOffset;
		memcpy(persistentBufferBytes, m_acb.m_beginptr, persistentHeader.m_acbSizeInDwords*sizeof(uint32_t));
	}

	// Write CCB
	if (persistentHeader.m_ccbSizeInDwords) {
		persistentBufferBytes = (uint8_t*)destPersistentBuffer + persistentHeader.m_ccbByteOffset;
		memcpy(persistentBufferBytes, m_ccb.m_beginptr, persistentHeader.m_ccbSizeInDwords*sizeof(uint32_t));
	}
}
#else
int32_t GfxContext::submit(void)
{
	void *dcbGpuAddrs[kMaxNumStoredSubmissions+1], *acbGpuAddrs[kMaxNumStoredSubmissions+1], *ccbGpuAddrs[kMaxNumStoredSubmissions+1];
	uint32_t dcbSizes[kMaxNumStoredSubmissions+1], acbSizes[kMaxNumStoredSubmissions+1], ccbSizes[kMaxNumStoredSubmissions+1];
	uint32_t numAcbSubmits = 0;
	// Submit each previously stored range
	for(uint32_t iSub=0; iSub<m_submissionCount; ++iSub)
	{
		dcbSizes[iSub]    = m_submissionRanges[iSub].m_dcbSizeInDwords*sizeof(uint32_t);
		dcbGpuAddrs[iSub] = m_dcb.m_beginptr + m_submissionRanges[iSub].m_dcbStartDwordOffset;
		ccbSizes[iSub]    = m_submissionRanges[iSub].m_ccbSizeInDwords*sizeof(uint32_t);
		ccbGpuAddrs[iSub] = (ccbSizes[iSub] > 0) ? m_ccb.m_beginptr + m_submissionRanges[iSub].m_ccbStartDwordOffset : 0;
		if (m_submissionRanges[iSub].m_acbSizeInDwords) {
			acbSizes[numAcbSubmits]    = m_submissionRanges[iSub].m_acbSizeInDwords*sizeof(uint32_t);
			acbGpuAddrs[numAcbSubmits] = m_acb.m_beginptr + m_submissionRanges[iSub].m_acbStartDwordOffset;
			++numAcbSubmits;
		}
	}
	// Submit anything left over after the final stored range
	dcbSizes[m_submissionCount]    = static_cast<uint32_t>(m_dcb.m_cmdptr - m_currentDcbSubmissionStart)*4;
	dcbGpuAddrs[m_submissionCount] = (void*)m_currentDcbSubmissionStart;
	ccbSizes[m_submissionCount]    = static_cast<uint32_t>(m_ccb.m_cmdptr - m_currentCcbSubmissionStart)*4;
	ccbGpuAddrs[m_submissionCount] = (ccbSizes[m_submissionCount] > 0) ? (void*)m_currentCcbSubmissionStart : 0;
	if (m_acb.m_cmdptr > m_currentAcbSubmissionStart) {
		acbSizes[numAcbSubmits]    = static_cast<uint32_t>(m_acb.m_cmdptr - m_currentAcbSubmissionStart)*4;
		acbGpuAddrs[numAcbSubmits] = (void*)m_currentAcbSubmissionStart;
		++numAcbSubmits;
	}
	if (numAcbSubmits) {
		SCE_GNM_VALIDATE(m_pQueue != NULL && m_pQueue->isMapped(), "ComputeQueue was not %s before submitting for dispatch draw", m_pQueue == NULL ? "set" : "mapped");
		ComputeQueue::SubmissionStatus err = m_pQueue->submit(numAcbSubmits, acbGpuAddrs, acbSizes);
		if (err == ComputeQueue::kSubmitFailQueueIsFull) {
			//FIXME: should this be some new SCE_GNM_WARNING() macro?
			if (sce::Gnm::getErrorResponseLevel() != sce::Gnm::kErrorResponseLevelIgnore)
				sce::Gnm::printErrorMessage(__FILE__, __LINE__, __FUNCTION__, "ComputeQueue for dispatch draw is full; waiting for space...");
			do {
				err = m_pQueue->submit(numAcbSubmits, acbGpuAddrs, acbSizes);
			} while (err == ComputeQueue::kSubmitFailQueueIsFull);
		}
		if (err != ComputeQueue::kSubmitOK)
			return err;
	}
	return Gnm::submitCommandBuffers(m_submissionCount+1, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes);
}

int32_t GfxContext::validate(void)
{
	void *dcbGpuAddrs[kMaxNumStoredSubmissions+1], *acbGpuAddrs[kMaxNumStoredSubmissions+1], *ccbGpuAddrs[kMaxNumStoredSubmissions+1];
	uint32_t dcbSizes[kMaxNumStoredSubmissions+1], acbSizes[kMaxNumStoredSubmissions+1], ccbSizes[kMaxNumStoredSubmissions+1];
	uint32_t numAcbSubmits = 0;
	// Submit each previously stored range
	for(uint32_t iSub=0; iSub<m_submissionCount; ++iSub)
	{
		dcbSizes[iSub]    = m_submissionRanges[iSub].m_dcbSizeInDwords*sizeof(uint32_t);
		dcbGpuAddrs[iSub] = m_dcb.m_beginptr + m_submissionRanges[iSub].m_dcbStartDwordOffset;
		ccbSizes[iSub]    = m_submissionRanges[iSub].m_ccbSizeInDwords*sizeof(uint32_t);
		ccbGpuAddrs[iSub] = (ccbSizes[iSub] > 0) ? m_ccb.m_beginptr + m_submissionRanges[iSub].m_ccbStartDwordOffset : 0;
		if (m_submissionRanges[iSub].m_acbSizeInDwords) {
			acbSizes[numAcbSubmits]    = m_submissionRanges[iSub].m_acbSizeInDwords*sizeof(uint32_t);
			acbGpuAddrs[numAcbSubmits] = m_acb.m_beginptr + m_submissionRanges[iSub].m_acbStartDwordOffset;
			++numAcbSubmits;
		}
	}
	// Submit anything left over after the final stored range
	dcbSizes[m_submissionCount]    = static_cast<uint32_t>(m_dcb.m_cmdptr - m_currentDcbSubmissionStart)*4;
	dcbGpuAddrs[m_submissionCount] = (void*)m_currentDcbSubmissionStart;
	ccbSizes[m_submissionCount]    = static_cast<uint32_t>(m_ccb.m_cmdptr - m_currentCcbSubmissionStart)*4;
	ccbGpuAddrs[m_submissionCount] = (ccbSizes[m_submissionCount] > 0) ? (void*)m_currentCcbSubmissionStart : 0;
	if (m_acb.m_cmdptr > m_currentAcbSubmissionStart) {
		acbSizes[numAcbSubmits]    = static_cast<uint32_t>(m_acb.m_cmdptr - m_currentAcbSubmissionStart)*4;
		acbGpuAddrs[numAcbSubmits] = (void*)m_currentAcbSubmissionStart;
		++numAcbSubmits;
	}
	if (numAcbSubmits) {
		SCE_GNM_UNUSED(acbSizes);
//		Gnm::validateDispatchCommandBuffers(numAcbSubmits, acbGpuAddrs, acbSizes);
	}
	return Gnm::validateCommandBuffers(m_submissionCount+1, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes);
}

int32_t GfxContext::submitAndFlip(uint32_t videoOutHandle, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg)
{
	m_dcb.prepareFlip();

	void *dcbGpuAddrs[kMaxNumStoredSubmissions+1], *acbGpuAddrs[kMaxNumStoredSubmissions+1], *ccbGpuAddrs[kMaxNumStoredSubmissions+1];
	uint32_t dcbSizes[kMaxNumStoredSubmissions+1], acbSizes[kMaxNumStoredSubmissions+1], ccbSizes[kMaxNumStoredSubmissions+1];
	uint32_t numAcbSubmits = 0;
	// Submit each previously stored range
	for(uint32_t iSub=0; iSub<m_submissionCount; ++iSub)
	{
		dcbSizes[iSub]    = m_submissionRanges[iSub].m_dcbSizeInDwords*sizeof(uint32_t);
		dcbGpuAddrs[iSub] = m_dcb.m_beginptr + m_submissionRanges[iSub].m_dcbStartDwordOffset;
		ccbSizes[iSub]    = m_submissionRanges[iSub].m_ccbSizeInDwords*sizeof(uint32_t);
		ccbGpuAddrs[iSub] = (ccbSizes[iSub] > 0) ? m_ccb.m_beginptr + m_submissionRanges[iSub].m_ccbStartDwordOffset : 0;
		if (m_submissionRanges[iSub].m_acbSizeInDwords) {
			acbSizes[numAcbSubmits]    = m_submissionRanges[iSub].m_acbSizeInDwords*sizeof(uint32_t);
			acbGpuAddrs[numAcbSubmits] = m_acb.m_beginptr + m_submissionRanges[iSub].m_acbStartDwordOffset;
			++numAcbSubmits;
		}
	}
	// Submit anything left over after the final stored range
	dcbSizes[m_submissionCount]    = static_cast<uint32_t>(m_dcb.m_cmdptr - m_currentDcbSubmissionStart)*4;
	dcbGpuAddrs[m_submissionCount] = (void*)m_currentDcbSubmissionStart;
	ccbSizes[m_submissionCount]    = static_cast<uint32_t>(m_ccb.m_cmdptr - m_currentCcbSubmissionStart)*4;
	ccbGpuAddrs[m_submissionCount] = (ccbSizes[m_submissionCount] > 0) ? (void*)m_currentCcbSubmissionStart : 0;
	if (m_acb.m_cmdptr > m_currentAcbSubmissionStart) {
		acbSizes[numAcbSubmits]    = static_cast<uint32_t>(m_acb.m_cmdptr - m_currentAcbSubmissionStart)*4;
		acbGpuAddrs[numAcbSubmits] = (void*)m_currentAcbSubmissionStart;
		++numAcbSubmits;
	}
	if (numAcbSubmits) {
		SCE_GNM_VALIDATE(m_pQueue != NULL && m_pQueue->isMapped(), "ComputeQueue was not %s before submitting for dispatch draw", m_pQueue == NULL ? "set" : "mapped");
		ComputeQueue::SubmissionStatus err = m_pQueue->submit(numAcbSubmits, acbGpuAddrs, acbSizes);
		if (err == ComputeQueue::kSubmitFailQueueIsFull) {
			//FIXME: should this be some new SCE_GNM_WARNING() macro?
			if (sce::Gnm::getErrorResponseLevel() != sce::Gnm::kErrorResponseLevelIgnore)
				sce::Gnm::printErrorMessage(__FILE__, __LINE__, __FUNCTION__, "ComputeQueue for dispatch draw is full; waiting for space...");
			do {
				err = m_pQueue->submit(numAcbSubmits, acbGpuAddrs, acbSizes);
			} while (err == ComputeQueue::kSubmitFailQueueIsFull);
		}
		if (err != ComputeQueue::kSubmitOK)
			return err;
	}
	return Gnm::submitAndFlipCommandBuffers(m_submissionCount+1, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes,
											videoOutHandle, rtIndex, flipMode, flipArg);
}

int32_t GfxContext::submitAndFlip(uint32_t videoOutHandle, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg,
								  void *labelAddr, uint32_t value)
{
	m_dcb.prepareFlip(labelAddr, value);

	void *dcbGpuAddrs[kMaxNumStoredSubmissions+1], *acbGpuAddrs[kMaxNumStoredSubmissions+1], *ccbGpuAddrs[kMaxNumStoredSubmissions+1];
	uint32_t dcbSizes[kMaxNumStoredSubmissions+1], acbSizes[kMaxNumStoredSubmissions+1], ccbSizes[kMaxNumStoredSubmissions+1];
	uint32_t numAcbSubmits = 0;
	// Submit each previously stored range
	for(uint32_t iSub=0; iSub<m_submissionCount; ++iSub)
	{
		dcbSizes[iSub]    = m_submissionRanges[iSub].m_dcbSizeInDwords*sizeof(uint32_t);
		dcbGpuAddrs[iSub] = m_dcb.m_beginptr + m_submissionRanges[iSub].m_dcbStartDwordOffset;
		ccbSizes[iSub]    = m_submissionRanges[iSub].m_ccbSizeInDwords*sizeof(uint32_t);
		ccbGpuAddrs[iSub] = (ccbSizes[iSub] > 0) ? m_ccb.m_beginptr + m_submissionRanges[iSub].m_ccbStartDwordOffset : 0;
		if (m_submissionRanges[iSub].m_acbSizeInDwords) {
			acbSizes[numAcbSubmits]    = m_submissionRanges[iSub].m_acbSizeInDwords*sizeof(uint32_t);
			acbGpuAddrs[numAcbSubmits] = m_acb.m_beginptr + m_submissionRanges[iSub].m_acbStartDwordOffset;
			++numAcbSubmits;
		}
	}
	// Submit anything left over after the final stored range
	dcbSizes[m_submissionCount]    = static_cast<uint32_t>(m_dcb.m_cmdptr - m_currentDcbSubmissionStart)*4;
	dcbGpuAddrs[m_submissionCount] = (void*)m_currentDcbSubmissionStart;
	ccbSizes[m_submissionCount]    = static_cast<uint32_t>(m_ccb.m_cmdptr - m_currentCcbSubmissionStart)*4;
	ccbGpuAddrs[m_submissionCount] = (ccbSizes[m_submissionCount] > 0) ? (void*)m_currentCcbSubmissionStart : 0;
	if (m_acb.m_cmdptr > m_currentAcbSubmissionStart) {
		acbSizes[numAcbSubmits]    = static_cast<uint32_t>(m_acb.m_cmdptr - m_currentAcbSubmissionStart)*4;
		acbGpuAddrs[numAcbSubmits] = (void*)m_currentAcbSubmissionStart;
		++numAcbSubmits;
	}
	if (numAcbSubmits) {
		SCE_GNM_VALIDATE(m_pQueue != NULL && m_pQueue->isMapped(), "ComputeQueue was not %s before submitting for dispatch draw", m_pQueue == NULL ? "set" : "mapped");
		ComputeQueue::SubmissionStatus err = m_pQueue->submit(numAcbSubmits, acbGpuAddrs, acbSizes);
		if (err == ComputeQueue::kSubmitFailQueueIsFull) {
			//FIXME: should this be some new SCE_GNM_WARNING() macro?
			if (sce::Gnm::getErrorResponseLevel() != sce::Gnm::kErrorResponseLevelIgnore)
				sce::Gnm::printErrorMessage(__FILE__, __LINE__, __FUNCTION__, "ComputeQueue for dispatch draw is full; waiting for space...");
			do {
				err = m_pQueue->submit(numAcbSubmits, acbGpuAddrs, acbSizes);
			} while (err == ComputeQueue::kSubmitFailQueueIsFull);
		}
		if (err != ComputeQueue::kSubmitOK)
			return err;
	}
	return Gnm::submitAndFlipCommandBuffers(m_submissionCount+1, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes,
											videoOutHandle, rtIndex, flipMode, flipArg);
}


#endif // defined(SCE_GNM_OFFLINE_MODE)

void GfxContext::setTessellationDataConstantBuffer(void *tcbAddr, ShaderStage domainStage)
{
	SCE_GNM_VALIDATE(domainStage == Gnm::kShaderStageEs || domainStage == Gnm::kShaderStageVs, "domainStage (%d) must be kShaderStageEs or kShaderStageVs.", domainStage);

	Gnm::Buffer tcbdef;
	tcbdef.initAsConstantBuffer(tcbAddr, sizeof(Gnm::TessellationDataConstantBuffer));

	// Slot 19 is currently reserved by the compiler for the tessellation data cb:
	this->setConstantBuffers(Gnm::kShaderStageHs, 19,1, &tcbdef);
	this->setConstantBuffers(domainStage, 19,1, &tcbdef);
}

void GfxContext::setTessellationFactorBuffer(void *tessFactorMemoryBaseAddr)
{
	Gnm::Buffer tfbdef;
	tfbdef.initAsTessellationFactorBuffer(tessFactorMemoryBaseAddr, Gnm::kTfRingSizeInBytes);
	m_cue.setGlobalDescriptor(Gnm::kShaderGlobalResourceTessFactorBuffer, &tfbdef);
}


void GfxContext::setLsHsShaders(LsShader *lsb, uint32_t shaderModifier, void *fetchShaderAddr, const HsShader *hsb, uint32_t numPatches)
{
	SCE_GNM_VALIDATE((lsb && hsb) || (!lsb && !hsb), "lsb (0x%010llX) and hsb (0x%010llX) must either both be NULL, or both be non-NULL.", lsb, hsb);
	Gnm::TessellationRegisters tessRegs;
	if ( hsb )
	{
		tessRegs.init(&hsb->m_hullStateConstants, numPatches);
		lsb->m_lsStageRegisters.updateLdsSize(&hsb->m_hullStateConstants,
											  lsb->m_lsStride, numPatches);	// set TG lds size
	}
	m_cue.setLsShader(lsb, shaderModifier, fetchShaderAddr);
	m_cue.setHsShader(hsb, &tessRegs);
}

void GfxContext::setGsVsShaders(const GsShader *gsb)
{
	Gnm::GsMaxOutputVertexCount maxVertCount;

	if ( gsb )
	{
		if ( gsb->m_maxOutputVertexCount <= 128 )
		{
			maxVertCount = Gnm::kGsMaxOutputVertexCount128;
		}
		else if ( gsb->m_maxOutputVertexCount <= 256 )
		{
			maxVertCount = Gnm::kGsMaxOutputVertexCount256;
		}
		else if ( gsb->m_maxOutputVertexCount <= 512 )
		{
			maxVertCount = Gnm::kGsMaxOutputVertexCount512;
		}
		else
		{
			maxVertCount = Gnm::kGsMaxOutputVertexCount1024;
		}
		m_dcb.setGsMode(Gnm::kGsModeEnable, maxVertCount); // TODO: use the gdb->copyShader->gsMode value
	}
	else
	{
		m_dcb.setGsMode(Gnm::kGsModeDisable, Gnm::kGsMaxOutputVertexCount1024);
	}

	m_cue.setGsVsShaders(gsb);
}

void GfxContext::setGsVsShadersOnChip(const GsShader *gsb)
{
	Gnm::GsMaxOutputVertexCount maxVertCount;

	if ( gsb )
	{
		if ( gsb->m_maxOutputVertexCount <= 128 )
		{
			maxVertCount = Gnm::kGsMaxOutputVertexCount128;
		}
		else if ( gsb->m_maxOutputVertexCount <= 256 )
		{
			maxVertCount = Gnm::kGsMaxOutputVertexCount256;
		}
		else if ( gsb->m_maxOutputVertexCount <= 512 )
		{
			maxVertCount = Gnm::kGsMaxOutputVertexCount512;
		}
		else
		{
			maxVertCount = Gnm::kGsMaxOutputVertexCount1024;
		}
		m_dcb.setGsMode(Gnm::kGsModeEnableOnChip, maxVertCount); // TODO: use the gdb->copyShader->gsMode value
	}
	else
	{
		m_dcb.setGsMode(Gnm::kGsModeDisable, Gnm::kGsMaxOutputVertexCount1024);
	}

	m_cue.setGsVsShaders(gsb);
}

void GfxContext::setEsGsRingBuffer(void *baseAddr, uint32_t ringSize, uint32_t maxExportVertexSizeInDword)
{
	Gnm::Buffer ringReadDescriptor;
	Gnm::Buffer ringWriteDescriptor;

	ringReadDescriptor.initAsEsGsReadDescriptor(baseAddr, ringSize);
	ringWriteDescriptor.initAsEsGsWriteDescriptor(baseAddr, ringSize);

	m_cue.setGlobalDescriptor(Gnm::kShaderGlobalResourceEsGsReadDescriptor, &ringReadDescriptor);
	m_cue.setGlobalDescriptor(Gnm::kShaderGlobalResourceEsGsWriteDescriptor, &ringWriteDescriptor);

	m_dcb.setupEsGsRingRegisters(maxExportVertexSizeInDword);
}

void GfxContext::setGsVsRingBuffers(void *baseAddr, uint32_t ringSize,
									const uint32_t vtxSizePerStreamInDword[4], uint32_t maxOutputVtxCount)
{
	Gnm::Buffer ringReadDescriptor;
	Gnm::Buffer ringWriteDescriptor;

	ringReadDescriptor.initAsGsVsReadDescriptor(baseAddr, ringSize);
	m_cue.setGlobalDescriptor(Gnm::kShaderGlobalResourceGsVsReadDescriptor, &ringReadDescriptor);

	for (uint32_t iStream = 0; iStream < 4; ++iStream)
	{
		ringWriteDescriptor.initAsGsVsWriteDescriptor(baseAddr, iStream,
													  vtxSizePerStreamInDword, maxOutputVtxCount);
		m_cue.setGlobalDescriptor((Gnm::ShaderGlobalResourceType)(Gnm::kShaderGlobalResourceGsVsWriteDescriptor0 + iStream),
								  &ringWriteDescriptor);
	}

	m_dcb.setupGsVsRingRegisters(vtxSizePerStreamInDword, maxOutputVtxCount);
}

void GfxContext::setupDispatchDrawScreenViewport(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	// Unfortunately, we have to duplicate much of the work done in Gnmx::setupScreenViewport here:
	int32_t width = right - left;
	int32_t height = bottom - top;

	float scale[2] = {(float)(width)*0.5f, - (float)(height)*0.5f};
	float offset[2] = {(float)(left + width*0.5f), (float)(top + height*0.5f)};

	// Set the guard band offset so that the guard band is centered around the viewport region.
	// 10xx limits hardware offset to multiples of 16 pixels
	int hwOffsetX = (int)floor(offset[0]/16.0f + 0.5f);
	int hwOffsetY = (int)floor(offset[1]/16.0f + 0.5f);

	// Set the guard band clip distance to the maximum possible values by calculating the minimum distance
	// from the closest viewport edge to the edge of the hardware's coordinate space
	float hwMin = -(float)(1<<23) / (float)(1<<8);
	float hwMax =  (float)((1<<23) - 1) / (float)(1<<8);
	float gbMaxX = SCE_GNM_MIN(hwMax - fabsf(scale[0]) - offset[0] + hwOffsetX*16, -fabsf(scale[0]) + offset[0] - hwOffsetX*16 - hwMin);
	float gbMaxY = SCE_GNM_MIN(hwMax - fabsf(scale[1]) - offset[1] + hwOffsetY*16, -fabsf(scale[1]) + offset[1] - hwOffsetY*16 - hwMin);
	float gbHorizontalClipAdjust = gbMaxX / fabsf(scale[0]);
	float gbVerticalClipAdjust = gbMaxY / fabsf(scale[1]);

	// Quantization to screen quantizes to 1/256 pixel resolution, with a max error of +/- 0.5/256 pixel.
	// This translates back to a max error in (x/w) of (+/- 0.5/256) / viewport_scale.x and (y/w) of (+/- 0.5/256) / viewport_scale.y.
	float fQuantErrorX = 0.5f / (fabsf(scale[0]) * 256.0f);
	float fQuantErrorY = 0.5f / (fabsf(scale[1]) * 256.0f);

	m_cue.setFloatInDispatchDrawData(12, fQuantErrorX);
	m_cue.setFloatInDispatchDrawData(13, fQuantErrorY);
	m_cue.setFloatInDispatchDrawData(14, gbHorizontalClipAdjust);
	m_cue.setFloatInDispatchDrawData(15, gbVerticalClipAdjust);
}

void GfxContext::setupDispatchDrawClipCullSettings(PrimitiveSetup primitiveSetup, ClipControl clipControl)
{
	Gnm::ClipControlClipSpace clipSpace = (Gnm::ClipControlClipSpace)((clipControl.m_reg >> 19) & 1);	//SCE_GNM_GET_FIELD(clipControl.m_reg, PA_CL_CLIP_CNTL, DX_CLIP_SPACE_DEF);

	uint32_t cullFront = primitiveSetup.m_reg & 1;			//SCE_GNM_GET_FIELD(primitiveSetup.m_reg, PA_SU_SC_MODE_CNTL, CULL_FRONT);
	uint32_t cullBack = (primitiveSetup.m_reg >> 1) & 1;	//SCE_GNM_GET_FIELD(primitiveSetup.m_reg, PA_SU_SC_MODE_CNTL, CULL_BACK);
	Gnm::PrimitiveSetupFrontFace frontFace = (Gnm::PrimitiveSetupFrontFace)((primitiveSetup.m_reg >> 2) & 1);	//SCE_GNM_GET_FIELD(primitiveSetup.m_reg, PA_SU_SC_MODE_CNTL, FACE);

	uint32_t dispatchDrawClipCullFlags = (clipSpace == Gnm::kClipControlClipSpaceOGL) ? Gnmx::kDispatchDrawClipCullFlagClipSpaceOGL : Gnmx::kDispatchDrawClipCullFlagClipSpaceDX;
	if (frontFace == Gnm::kPrimitiveSetupFrontFaceCcw) {
		if (cullBack)	dispatchDrawClipCullFlags |= Gnmx::kDispatchDrawClipCullFlagCullCW;
		if (cullFront)	dispatchDrawClipCullFlags |= Gnmx::kDispatchDrawClipCullFlagCullCCW;
	} else {
		if (cullBack)	dispatchDrawClipCullFlags |= Gnmx::kDispatchDrawClipCullFlagCullCCW;
		if (cullFront)	dispatchDrawClipCullFlags |= Gnmx::kDispatchDrawClipCullFlagCullCW;
	}
	m_cue.setDwordMaskedInDispatchDrawData(10, dispatchDrawClipCullFlags, 0x7);
}

void GfxContext::dispatchDraw(Buffer bufferInputData, uint32_t numBlocksTotal, uint32_t numPrimsPerVgt)
{
	uint32_t indexOffset = 0;					//Do we need to be able to set the index offset for dispatch draw?
	SCE_GNM_VALIDATE(numPrimsPerVgt == 129, "dispatchDraw requires numPrimsPerVgt to be 129; parameter will be deprecated in future SDKs");
	numPrimsPerVgt = 129;	//To prevent dispatch draw deadlocks.

	// store bufferInputData and numBlocksTotal to kShaderInputUsagePtrDispatchDraw data
	m_cue.setVsharpInDispatchDrawData(4, &bufferInputData);	//unused by VS
	m_cue.setDwordMaskedInDispatchDrawData(8, numBlocksTotal, 0x0000FFFF);		//unused by VS

	if (!(m_dispatchDrawState & kDispatchDrawStateFlagIndexSize)) {
		m_dispatchDrawState |= kDispatchDrawStateFlagIndexSize;
		m_dcb.setIndexSize(Gnm::kIndexSize16ForDispatchDraw);
	}
	if (!(m_dispatchDrawState & kDispatchDrawStateFlagIndexBuffer)) {
		m_dispatchDrawState |= kDispatchDrawStateFlagIndexBuffer;
		m_dcb.setIndexBuffer(m_pIndexRingBuffer);
	}
	if (!(m_dispatchDrawState & kDispatchDrawStateFlagSetVgtControl) || ((m_dispatchDrawState & kDispatchDrawStateMaskPrimGroupSize) >> kDispatchDrawStateShiftPrimGroupSize)+1 != numPrimsPerVgt) {
		m_dispatchDrawState = (m_dispatchDrawState & ~kDispatchDrawStateMaskPrimGroupSize) | (((numPrimsPerVgt-1) << kDispatchDrawStateShiftPrimGroupSize) & kDispatchDrawStateMaskPrimGroupSize) | kDispatchDrawStateFlagSetVgtControl;
		m_dcb.setVgtControl(static_cast<uint8_t>(numPrimsPerVgt-1), Gnm::kVgtPartialVsWaveEnable, Gnm::kVgtSwitchOnEopEnable);
	}

	// get orderedAppendMode, sgprKrbLoc, dispatchDrawMode, sgprVrbLoc from currently set shaders in preDispatchDraw
	Gnm::DispatchOrderedAppendMode orderedAppendMode;	//from CsShader.m_orderedAppendMode
	Gnm::DispatchDrawMode dispatchDrawMode = Gnm::kDispatchDrawModeIndexRingBufferOnly;	//from VsShader inputUsageSlots (kDispatchDrawModeIndexAndVertexRingBuffers if sgprVrbLoc is found)
	uint16_t dispatchDrawIndexDeallocMask = 0;	//from CsShader.m_dispatchDrawIndexDeallocNumBits
	uint32_t sgprKrbLoc = 0;	//from CsShader inputUsageSlots
	uint32_t sgprVrbLoc = 0;	//from VsShader inputUsageSlots
	m_cue.preDispatchDraw(&m_dcb, &m_acb, &m_ccb, &orderedAppendMode, &dispatchDrawIndexDeallocMask, &sgprKrbLoc, &dispatchDrawMode, &sgprVrbLoc);
	if (dispatchDrawIndexDeallocMask != (m_dispatchDrawState & kDispatchDrawStateMaskIndexDeallocMask)) {
		m_dispatchDrawState = (m_dispatchDrawState &~ kDispatchDrawStateMaskIndexDeallocMask) | dispatchDrawIndexDeallocMask;
		m_dcb.setDispatchDrawIndexDeallocationMask(dispatchDrawIndexDeallocMask);
	}

	m_acb.dispatchDraw(numBlocksTotal, 1, 1, orderedAppendMode, sgprKrbLoc);
	m_dcb.dispatchDraw(Gnm::kPrimitiveTypeTriList, indexOffset, numPrimsPerVgt*3*2, dispatchDrawMode, sgprVrbLoc);

	m_cue.postDispatchDraw(&m_dcb, &m_acb, &m_ccb);
}

void GfxContext::setRenderTarget(uint32_t rtSlot, sce::Gnm::RenderTarget const *target)
{
	if (target == NULL)
		return m_dcb.setRenderTarget(rtSlot, NULL);
	// Workaround for multiple render target bug with CMASKs but no FMASKs
	RenderTarget rtCopy = *target;
	if (!rtCopy.getFmaskCompressionEnable()        && rtCopy.getCmaskFastClearEnable() &&
		rtCopy.getFmaskAddress256ByteBlocks() == 0 && rtCopy.getCmaskAddress256ByteBlocks() != 0)
	{
		rtCopy.disableFmaskCompressionForMrtWithCmask();
	}
	return m_dcb.setRenderTarget(rtSlot, &rtCopy);
}

#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
void GfxContext::initializeRecordLastCompletion(RecordLastCompletionMode mode)
{
	m_recordLastCompletionMode = mode;
	// Allocate space in the command buffer to store the byte offset of the most recently executed draw command,
	// for debugging purposes.
	m_addressOfOffsetOfLastCompletion = static_cast<uint32_t*>(allocateFromCommandBuffer(sizeof(uint32_t), Gnm::kEmbeddedDataAlignment8));
	*m_addressOfOffsetOfLastCompletion = 0;
    fillData(m_addressOfOffsetOfLastCompletion, 0xFFFFFFFF, sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
}
#endif


//////////////////////////////////////////////////////////////////////////////////
#if !defined(SCE_GNM_OFFLINE_MODE)
GfxContextSubmitOnly::GfxContextSubmitOnly(void)
{
}

GfxContextSubmitOnly::~GfxContextSubmitOnly(void)
{
}

size_t GfxContextSubmitOnly::getRequiredCueHeapSize(const void *srcTempBuffer)
{
	SCE_GNM_VALIDATE(srcTempBuffer != NULL, "srcTempBuffer must not be NULL.");
	const SerializedContextTempHeader &tempHeader = *static_cast<const SerializedContextTempHeader*>(srcTempBuffer);
	SCE_GNM_VALIDATE(tempHeader.m_version == kSerializedContextTempHeaderVersion, "Version mismatch in srcTempBuffer (expected %d, actual %d).", kSerializedContextTempHeaderVersion, tempHeader.m_version);
	SCE_GNM_VALIDATE(tempHeader.m_magic == kSerializedContextTempHeaderMagic, "Magic number mismatch in srcTempBuffer (expected 0x%08X, actual 0x%08X).", kSerializedContextTempHeaderMagic, tempHeader.m_magic);
	return tempHeader.m_cueHeapSize;
}

void GfxContextSubmitOnly::init(void *cueHeapAddr, void *srcTempBuffer, uint32_t srcTempBufferSize, void *srcPersistentBuffer, uint32_t srcPersistentBufferSize)
{
	// Not currently used, but may be useful for validation someday
	SCE_GNM_UNUSED(srcTempBufferSize);
	SCE_GNM_UNUSED(srcPersistentBufferSize);

	SCE_GNM_VALIDATE(srcTempBuffer != NULL, "srcTempBuffer must not be NULL.");
	SCE_GNM_VALIDATE(srcPersistentBuffer != NULL, "srcPersistentBuffer must not be NULL.");
	SCE_GNM_VALIDATE((uintptr_t(srcPersistentBuffer) & 0xFF) == 0, "srcPersistentBuffer (0x%010llX) must be 256-byte aligned.", srcPersistentBuffer);

	SerializedContextTempHeader &tempHeader = *(SerializedContextTempHeader*)srcTempBuffer;
	SCE_GNM_VALIDATE(tempHeader.m_version == kSerializedContextTempHeaderVersion, "Version mismatch in srcTempBuffer (expected %d, actual %d).", kSerializedContextTempHeaderVersion, tempHeader.m_version);
	SCE_GNM_VALIDATE(tempHeader.m_magic == kSerializedContextTempHeaderMagic, "Magic number mismatch in srcTempBuffer (expected 0x%08X, actual 0x%08X).", kSerializedContextTempHeaderMagic, tempHeader.m_magic);
	SCE_GNM_VALIDATE(tempHeader.m_submissionRangeCount <= kMaxNumStoredSubmissions, "srcTempBuffer's submit count (%d) must be <= kMaxNumStoredSubmissions (%d).", tempHeader.m_submissionRangeCount, kMaxNumStoredSubmissions);
	SerializedContextPersistentHeader &persistentHeader = *(SerializedContextPersistentHeader*)srcPersistentBuffer;
	SCE_GNM_VALIDATE(persistentHeader.m_version == kSerializedContextPersistentHeaderVersion, "Version mismatch in srcTempBuffer (expected %d, actual %d).", kSerializedContextPersistentHeaderVersion, persistentHeader.m_version);
	SCE_GNM_VALIDATE(persistentHeader.m_magic == kSerializedContextPersistentHeaderMagic, "Magic number mismatch in srcTempBuffer (expected 0x%08X, actual 0x%08X).", kSerializedContextPersistentHeaderMagic, persistentHeader.m_magic);

	uint8_t *tempBufferBytes       = (uint8_t*)srcTempBuffer;
	uint8_t *persistentBufferBytes = (uint8_t*)srcPersistentBuffer;
	
	m_dcbPatchTable.init(tempBufferBytes + tempHeader.m_dcbPatchTableByteOffset, tempHeader.m_dcbPatchTableSizeInBytes);
	m_acbPatchTable.init(tempBufferBytes + tempHeader.m_acbPatchTableByteOffset, tempHeader.m_acbPatchTableSizeInBytes);
	m_ccbPatchTable.init(tempBufferBytes + tempHeader.m_ccbPatchTableByteOffset, tempHeader.m_ccbPatchTableSizeInBytes);
	m_submissionCount = tempHeader.m_submissionRangeCount;
	memcpy(m_submissionRanges, tempBufferBytes + tempHeader.m_submissionRangesByteOffset, tempHeader.m_submissionRangeCount*sizeof(GfxContext::SubmissionRange));
			
	m_dcb.init(persistentBufferBytes + persistentHeader.m_dcbByteOffset, persistentHeader.m_dcbSizeInDwords*sizeof(uint32_t), NULL, NULL);
	m_acb.init(persistentBufferBytes + persistentHeader.m_acbByteOffset, persistentHeader.m_acbSizeInDwords*sizeof(uint32_t), NULL, NULL);
	m_ccb.init(persistentBufferBytes + persistentHeader.m_ccbByteOffset, persistentHeader.m_ccbSizeInDwords*sizeof(uint32_t), NULL, NULL);
	m_cueHeapAddr = cueHeapAddr;

	// Use the CUE heap as the custom base for DWORD offsets in the patch table
	m_dcbPatchTable.setCustomDwordOffsetBaseAddress(m_cueHeapAddr);
	m_ccbPatchTable.setCustomDwordOffsetBaseAddress(m_cueHeapAddr);
}

bool GfxContextSubmitOnly::setAddressForPatchId(uint32_t id, void *finalAddr)
{
	if (m_dcbPatchTable.setAddressForPatchId(id, finalAddr))
		return true;
	if (m_acbPatchTable.setAddressForPatchId(id, finalAddr))
		return true;
	return m_ccbPatchTable.setAddressForPatchId(id, finalAddr);
}

void GfxContextSubmitOnly::patchCommandBuffers(void)
{
	m_dcbPatchTable.applyPatchesToCommandBuffer(&m_dcb);
	m_acbPatchTable.applyPatchesToCommandBuffer(&m_acb);
	m_ccbPatchTable.applyPatchesToCommandBuffer(&m_ccb);
}

int32_t GfxContextSubmitOnly::submit(void)
{
	void *dcbGpuAddrs[kMaxNumStoredSubmissions], *acbGpuAddrs[kMaxNumStoredSubmissions], *ccbGpuAddrs[kMaxNumStoredSubmissions];
	uint32_t dcbSizes[kMaxNumStoredSubmissions], acbSizes[kMaxNumStoredSubmissions], ccbSizes[kMaxNumStoredSubmissions];
	uint32_t numAcbSubmits = 0;
	// Submit each previously stored range
	for(uint32_t iSub=0; iSub<m_submissionCount; ++iSub)
	{
		dcbSizes[iSub]    = m_submissionRanges[iSub].m_dcbSizeInDwords*sizeof(uint32_t);
		dcbGpuAddrs[iSub] = m_dcb.m_beginptr + m_submissionRanges[iSub].m_dcbStartDwordOffset;
		ccbSizes[iSub]    = m_submissionRanges[iSub].m_ccbSizeInDwords*sizeof(uint32_t);
		ccbGpuAddrs[iSub] = (ccbSizes[iSub] > 0) ? m_ccb.m_beginptr + m_submissionRanges[iSub].m_ccbStartDwordOffset : 0;
		if (m_submissionRanges[iSub].m_acbSizeInDwords) {
			acbSizes[numAcbSubmits]    = m_submissionRanges[iSub].m_acbSizeInDwords*sizeof(uint32_t);
			acbGpuAddrs[numAcbSubmits] = m_acb.m_beginptr + m_submissionRanges[iSub].m_acbStartDwordOffset;
			++numAcbSubmits;
		}
	}
	if (numAcbSubmits) {
		SCE_GNM_VALIDATE(m_pQueue != NULL && m_pQueue->isMapped(), "ComputeQueue was not %s before submitting for dispatch draw", m_pQueue == NULL ? "set" : "mapped");
		ComputeQueue::SubmissionStatus err = m_pQueue->submit(numAcbSubmits, acbGpuAddrs, acbSizes);
		if (err == ComputeQueue::kSubmitFailQueueIsFull) {
			//FIXME: should this be some new SCE_GNM_WARNING() macro?
			if (sce::Gnm::getErrorResponseLevel() != sce::Gnm::kErrorResponseLevelIgnore)
				sce::Gnm::printErrorMessage(__FILE__, __LINE__, __FUNCTION__, "ComputeQueue for dispatch draw is full; waiting for space...");
			do {
				err = m_pQueue->submit(numAcbSubmits, acbGpuAddrs, acbSizes);
			} while (err == ComputeQueue::kSubmitFailQueueIsFull);
		}
		if (err != ComputeQueue::kSubmitOK)
			return err;
	}
	return Gnm::submitCommandBuffers(m_submissionCount, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes);
}

int32_t GfxContextSubmitOnly::validate(void)
{
	void *dcbGpuAddrs[kMaxNumStoredSubmissions], *acbGpuAddrs[kMaxNumStoredSubmissions], *ccbGpuAddrs[kMaxNumStoredSubmissions];
	uint32_t dcbSizes[kMaxNumStoredSubmissions], acbSizes[kMaxNumStoredSubmissions], ccbSizes[kMaxNumStoredSubmissions];
	uint32_t numAcbSubmits = 0;
	// Submit each previously stored range
	for(uint32_t iSub=0; iSub<m_submissionCount; ++iSub)
	{
		dcbSizes[iSub]    = m_submissionRanges[iSub].m_dcbSizeInDwords*sizeof(uint32_t);
		dcbGpuAddrs[iSub] = m_dcb.m_beginptr + m_submissionRanges[iSub].m_dcbStartDwordOffset;
		ccbSizes[iSub]    = m_submissionRanges[iSub].m_ccbSizeInDwords*sizeof(uint32_t);
		ccbGpuAddrs[iSub] = (ccbSizes[iSub] > 0) ? m_ccb.m_beginptr + m_submissionRanges[iSub].m_ccbStartDwordOffset : 0;
		if (m_submissionRanges[iSub].m_acbSizeInDwords) {
			acbSizes[numAcbSubmits]    = m_submissionRanges[iSub].m_acbSizeInDwords*sizeof(uint32_t);
			acbGpuAddrs[numAcbSubmits] = m_acb.m_beginptr + m_submissionRanges[iSub].m_acbStartDwordOffset;
			++numAcbSubmits;
		}
	}
	if (numAcbSubmits) {
		SCE_GNM_UNUSED(acbSizes);
//		Gnm::validateDispatchCommandBuffers(numAcbSubmits, acbGpuAddrs, acbSizes);
	}
	return Gnm::validateCommandBuffers(m_submissionCount, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes);
}

int32_t GfxContextSubmitOnly::submitAndFlip(uint32_t videoOutHandle, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg)
{
	m_dcb.prepareFlip();

	void *dcbGpuAddrs[kMaxNumStoredSubmissions], *ccbGpuAddrs[kMaxNumStoredSubmissions];
	uint32_t dcbSizes[kMaxNumStoredSubmissions], ccbSizes[kMaxNumStoredSubmissions];
	// Submit each previously stored range
	for(uint32_t iSub=0; iSub<m_submissionCount; ++iSub)
	{
		dcbSizes[iSub]    = m_submissionRanges[iSub].m_dcbSizeInDwords*sizeof(uint32_t);
		dcbGpuAddrs[iSub] = m_dcb.m_beginptr + m_submissionRanges[iSub].m_dcbStartDwordOffset;
		ccbSizes[iSub]    = m_submissionRanges[iSub].m_ccbSizeInDwords*sizeof(uint32_t);
		ccbGpuAddrs[iSub] = (ccbSizes[iSub] > 0) ? m_ccb.m_beginptr + m_submissionRanges[iSub].m_ccbStartDwordOffset : 0;
	}

	return Gnm::submitAndFlipCommandBuffers(m_submissionCount, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes,
		videoOutHandle, rtIndex, flipMode, flipArg);
}

#endif // !defined(SCE_GNM_OFFLINE_MODE)
