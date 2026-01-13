/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.700.081
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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996) // Use deprecated variable
#elif defined(__ORBIS__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
static bool handleReserveFailed(CommandBuffer *cb, uint32_t dwordCount, void *userData)
{
	GfxContext &gfxc = *(GfxContext*)userData;
	// If either command buffer has actually reached the end of its full buffer, then invoke m_bufferFullCallback (if one has been registered)
	if (static_cast<void*>(cb) == static_cast<void*>(&(gfxc.m_dcb)) && gfxc.m_dcb.m_cmdptr + dwordCount > gfxc.m_actualDcbEnd)
	{
		if (gfxc.m_cbFullCallback.m_func != 0)
			return gfxc.m_cbFullCallback.m_func(&gfxc, cb, dwordCount, gfxc.m_cbFullCallback.m_userData);
		else if (gfxc.m_bufferFullCallback.m_func != 0)
			return gfxc.m_bufferFullCallback.m_func(cb, dwordCount, gfxc.m_bufferFullCallback.m_userData);
		else
		{
			SCE_GNM_ERROR("Out of DCB command buffer space, and no callback bound.");
			return false;
		}
	}
	if (static_cast<void*>(cb) == static_cast<void*>(&(gfxc.m_ccb)) && gfxc.m_ccb.m_cmdptr + dwordCount > gfxc.m_actualCcbEnd)
	{
		if (gfxc.m_cbFullCallback.m_func != 0)
			return gfxc.m_cbFullCallback.m_func(&gfxc, cb, dwordCount, gfxc.m_cbFullCallback.m_userData);
		else if (gfxc.m_bufferFullCallback.m_func != 0)
			return gfxc.m_bufferFullCallback.m_func(cb, dwordCount, gfxc.m_bufferFullCallback.m_userData);
		else
		{
			SCE_GNM_ERROR("Out of CCB command buffer space, and no callback bound.");
			return false;
		}
	}
	return gfxc.splitCommandBuffers();
}
#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__ORBIS__)
#pragma clang diagnostic pop
#endif

GfxContext::GfxContext(void)
{
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
	m_acb.m_beginptr = m_acb.m_cmdptr = m_acb.m_endptr = NULL;
	m_currentAcbSubmissionStart = m_actualAcbEnd = NULL;

#ifdef SCE_GNMX_ENABLE_CUE_V2
	m_cue.bindCommandBuffers(&m_dcb, &m_ccb, &m_acb);
#endif // SCE_GNMX_ENABLE_CUE_V2

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
	m_cbFullCallback.m_func = NULL;
	m_cbFullCallback.m_userData = NULL;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996) // Use deprecated variable
#elif defined(__ORBIS__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
	m_bufferFullCallback.m_func = NULL;
	m_bufferFullCallback.m_userData = NULL;
#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__ORBIS__)
#pragma clang diagnostic pop
#endif

#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
	m_recordLastCompletionMode = kRecordLastCompletionDisabled;
	m_addressOfOffsetOfLastCompletion = 0;
#endif
	m_dispatchDrawIndexDeallocMask = 0;
	m_dispatchDrawNumInstancesMinus1 = 0;
	m_dispatchDrawFlags = 0;
}

#ifdef SCE_GNMX_ENABLE_CUE_V2
void GfxContext::init(void *cueHeapAddr, uint32_t numRingEntries, ConstantUpdateEngine::RingSetup ringSetup,
					  void *dcbBuffer, uint32_t dcbSizeInBytes, void *ccbBuffer, uint32_t ccbSizeInBytes)
{
	m_dcb.init(dcbBuffer, std::min((uint32_t)Gnm::kIndirectBufferMaximumSizeInBytes, dcbSizeInBytes), handleReserveFailed, this);
	m_ccb.init(ccbBuffer, std::min((uint32_t)Gnm::kIndirectBufferMaximumSizeInBytes, ccbSizeInBytes), handleReserveFailed, this);
	m_cue.init(cueHeapAddr, numRingEntries, ringSetup);
	m_cue.bindCommandBuffers(&m_dcb, &m_ccb, &m_acb);
	m_acb.m_beginptr = m_acb.m_cmdptr = m_acb.m_endptr = NULL;
	m_currentAcbSubmissionStart = m_actualAcbEnd = NULL;

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
	m_cbFullCallback.m_func = NULL;
	m_cbFullCallback.m_userData = NULL;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996) // Use deprecated variable
#elif defined(__ORBIS__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
	m_bufferFullCallback.m_func = NULL;
	m_bufferFullCallback.m_userData = NULL;
#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__ORBIS__)
#pragma clang diagnostic pop
#endif

#if defined(SCE_GNMX_RECORD_LAST_COMPLETION)
	m_recordLastCompletionMode = kRecordLastCompletionDisabled;
	m_addressOfOffsetOfLastCompletion = 0;
#endif
	m_dispatchDrawIndexDeallocMask = 0;
	m_dispatchDrawNumInstancesMinus1 = 0;
	m_dispatchDrawFlags = 0;
}
#endif // SCE_GNMX_ENABLE_CUE_V2


void GfxContext::initDispatchDrawCommandBuffer(void *acbBuffer, uint32_t acbSizeInBytes)
{
	m_acb.init(acbBuffer, std::min((uint32_t)Gnm::kIndirectBufferMaximumSizeInBytes, acbSizeInBytes), handleReserveFailed, this);
	m_currentAcbSubmissionStart = m_acb.m_beginptr;
	m_actualAcbEnd = (uint32_t*)acbBuffer+(acbSizeInBytes/4);

	m_dispatchDrawIndexDeallocMask = 0;
	m_dispatchDrawNumInstancesMinus1 = 0;
	m_dispatchDrawFlags = 0;
	if (m_acb.m_beginptr != NULL) {
		// We do not use the CCB to prefetch the dispatch draw control data.
		// Instead we allocate copies from the ACB and prefetch explicitly.
		// As the current version of DispatchDrawTriangleCullData contains data which changes with every draw call,
		// we must currently allocate and copy 68 bytes for every dispatchDraw call.
		// In the future, we could put constant data {m_bufferIrb, m_gdsOffsetOfIrbWptr, m_sizeofIrbInIndices} and
		// data which changes infrequently { m_clipCullSettings, m_quantErrorScreenX, m_quantErrorScreenY, 
		// m_gbHorizClipAdjust, m_gbVertClipAdjust } in a separate structure with a pointer in the base data, 
		// which would generally reduce this copy and command buffer allocation to 36 bytes per dispatchDraw call.
		m_pDispatchDrawData = (Gnmx::DispatchDrawTriangleCullData*)m_acb.allocateFromCommandBuffer(sizeof(Gnmx::DispatchDrawTriangleCullData), Gnm::kEmbeddedDataAlignment4);
		memset(m_pDispatchDrawData, 0, sizeof(Gnmx::DispatchDrawTriangleCullData));
	}
}

uint32_t GfxContext::getRequiredSizeOfGdsDispatchDrawArea(uint32_t numKickRingBufferElems)
{
	uint32_t const kNumDispatchDrawCounters = 8;	// Currently using 4, but will need 4 more for VRB support
	SCE_GNM_VALIDATE(numKickRingBufferElems >= 2 && (numKickRingBufferElems*3 + kNumDispatchDrawCounters)*4 < kGdsAccessibleMemorySizeInBytes, "numKickRingBufferElems must be in the range [0:%u]", (kGdsAccessibleMemorySizeInBytes/4 - kNumDispatchDrawCounters)/3);
	return (numKickRingBufferElems*3 + kNumDispatchDrawCounters)*4;
}

void GfxContext::setupDispatchDrawRingBuffers(void *pIndexRingBuffer, uint32_t sizeofIndexRingBufferInBytes, uint32_t numKickRingBufferElems, uint32_t gdsOffsetDispatchDrawArea, uint32_t gdsOaCounterForDispatchDraw)
{
	uint32_t const kNumDispatchDrawCounters = 8;	// Currently using 4, but will need 4 more for VRB support
	uint32_t krbCount = numKickRingBufferElems, gdsDwOffsetKrb = gdsOffsetDispatchDrawArea/4, gdsDwOffsetKrbCounters = gdsDwOffsetKrb + krbCount*3, gdsDwOffsetIrbWptr = gdsDwOffsetKrbCounters + 3;
	uint32_t sizeofIndexRingBufferInIndices = sizeofIndexRingBufferInBytes>>1;
	SCE_GNM_VALIDATE(gdsDwOffsetKrb*sizeof(uint32_t) < kGdsAccessibleMemorySizeInBytes, "gdsDwOffsetKrb (%d) must be less than %ld", gdsDwOffsetKrb, kGdsAccessibleMemorySizeInBytes/sizeof(uint32_t));
	SCE_GNM_VALIDATE(m_pDispatchDrawData != NULL, "initDispatchDrawCommandBuffer must be called before setupDispatchDrawRingBuffers");
	SCE_GNM_VALIDATE(numKickRingBufferElems >= 2, "numKickRingBufferElems must be greater than or equal to 2");
	SCE_GNM_VALIDATE(!(gdsOffsetDispatchDrawArea & 0x3), "gdsOffsetDispatchDrawCounters must be 4 byte aligned");
	SCE_GNM_VALIDATE((gdsDwOffsetKrbCounters + kNumDispatchDrawCounters)*4 <= kGdsAccessibleMemorySizeInBytes, "gdsOffsetDispatchDrawCounters [0x%04x:0x%04x] does not fit in user accessible GDS area [0x%04x:0x%04x]", gdsOffsetDispatchDrawArea*4, (gdsDwOffsetKrbCounters + kNumDispatchDrawCounters)*4, 0, kGdsAccessibleMemorySizeInBytes);
	SCE_GNM_VALIDATE(!((uintptr_t)pIndexRingBuffer & 0xFF), "pIndexRingBuffer must be 256 byte aligned");
	SCE_GNM_VALIDATE(sizeofIndexRingBufferInBytes > 0 && !(sizeofIndexRingBufferInBytes & 0xFF), "sizeofIndexRingBufferInBytes must be a multiple of 256 bytes greater than 0");
	
	// Clear the KRB counters to zero.
	// It is not necessary to clear the KRB entries, as the CP clears them as it allocates KRB entries to dispatchDraw calls, 
	// but it can make debugging simpler if the initial state is all zero, and the cost of doing so is minimal.
//	m_acb.dmaData(Gnm::kDmaDataDstGds, gdsDwOffsetKrbCounters*sizeof(uint32_t), Gnm::kDmaDataSrcData, (uint64_t)0, kNumDispatchDrawCounters*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);
	m_acb.dmaData(Gnm::kDmaDataDstGds, gdsDwOffsetKrb*sizeof(uint32_t), Gnm::kDmaDataSrcData, (uint64_t)0, (krbCount*3 + kNumDispatchDrawCounters)*sizeof(uint32_t), Gnm::kDmaDataBlockingEnable);

	// Configure the GDS ordered append unit counter[gdsOaCounterForDispatchDraw] to watch for ds_ordered_count operations 
	// targeting GDS address gdsDwOffsetIrbWptr.  Those coming from this compute pipe are treated as ring buffer allocations
	// and are stalled if they would exceed the specified ring size.  Those coming from any other compute pipe or graphics   
	// stage (the VS stage in the case of dispatchDraw) are treated as ring buffer frees, which will return space to the 
	// space available counter and allow more compute waves to allocate.
	m_acb.enableOrderedAppendAllocationCounter(gdsOaCounterForDispatchDraw, gdsDwOffsetIrbWptr, 0, sizeofIndexRingBufferInIndices);

	// m_acb.setupDispatchDrawKickRingBuffer implicitly waits for one increment of the CS counter.
	// incrementCeCounterForDispatchDraw increments both the CE counter (for the CUE) and CS counter
	// (for dispatchDraw).
	m_ccb.incrementCeCounterForDispatchDraw();

	// Configure the compute command processor with the dispatch draw index ring buffer parameters and the GDS layout of the 
	// KRB and counters.  This sets up internal registers in the compute command processor used to handle dispatchDraw 
	// commands later in the frame.  The compute command processor will allocate a KRB entry per dispatchDraw and free KRB 
	// entries as dispatchDraw commands complete.
	m_acb.setupDispatchDrawKickRingBuffer(krbCount, gdsDwOffsetKrb, gdsDwOffsetKrbCounters);
	// As both the CS and CE counters were incremented, but m_acb.setupDispatchDrawKickRingBuffer only consumed the CS 
	// counter, the ACB must also wait for the CE counter here to keep the command processor counters in sync.
	m_acb.waitOnCe();

	// Configures the draw command processor with the dispatch draw index ring buffer parameters and the GDS layout of the 
	// KRB and counters.  This sets up internal registers in the draw command processor used to handle dispatchDraw 
	// commands later in the frame.  The draw command processor watches the active KRB entry for notifications that index 
	// data is available for processing, and issues a series of dispatch draw sub-draws which launch VS waves to consume
	// the index data out of the index ring buffer.
	m_dcb.waitForSetupDispatchDrawKickRingBuffer(krbCount, gdsDwOffsetKrb, gdsDwOffsetKrbCounters, pIndexRingBuffer, sizeofIndexRingBufferInBytes);
	// The CE counter was incremented, so the DCB must also wait for the CE counter and increment the DE counter to keep 
	// the command processor counters in sync, much as if a draw call had been issued.
	m_dcb.waitOnCe();
	m_dcb.incrementDeCounter();

	// Set the IRB configuration and GDS layout relevant to dispatch draw shaders in the dispatch draw data
	m_pDispatchDrawData->m_bufferIrb.initAsDataBuffer(pIndexRingBuffer, Gnm::kDataFormatR16Uint, sizeofIndexRingBufferInIndices);
	m_pDispatchDrawData->m_bufferIrb.setResourceMemoryType(Gnm::kResourceMemoryTypeGC);	// the index ring buffer must be writeable and cached in the GPU L2 for performance.  Caching it in the GPU L1 is unnecessary as the writes are streaming, but wouldn't hurt coherency as the index read up by the input assembly (IA) unit has no cache and so is always effectively "GC".
	m_pDispatchDrawData->m_gdsOffsetOfIrbWptr = (uint16_t)(gdsDwOffsetIrbWptr<<2);
	m_pDispatchDrawData->m_sizeofIrbInIndices = sizeofIndexRingBufferInIndices;
	// Also initialize default back-face culling settings matching the Gnm defaults for hardware culling
	m_pDispatchDrawData->m_clipCullSettings = kDispatchDrawClipCullFlagClipSpaceOGL;

	m_dispatchDrawFlags |= kDispatchDrawFlagIrbValid;
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
	m_cue.setAsynchronousComputeShader(NULL, 0, (void*)0);

	m_dispatchDrawIndexDeallocMask = 0;
	m_dispatchDrawNumInstancesMinus1 = 0;
	m_dispatchDrawFlags = 0;
	if (m_acb.m_beginptr != NULL) {
		// We do not use the CCB to prefetch the dispatch draw control data.
		// Instead we allocate copies from the ACB and prefetch explicitly.
		// As the current version of DispatchDrawTriangleCullData contains data which changes with every draw call,
		// we must currently allocate and copy 68 bytes for every dispatchDraw call.
		// In the future, we could put constant data {m_bufferIrb, m_gdsOffsetOfIrbWptr, m_sizeofIrbInIndices} and
		// data which changes infrequently { m_clipCullSettings, m_quantErrorScreenX, m_quantErrorScreenY, 
		// m_gbHorizClipAdjust, m_gbVertClipAdjust } in a separate structure with a pointer in the base data, 
		// which would generally reduce this copy and command buffer allocation to 36 bytes per dispatchDraw call.
		m_pDispatchDrawData = (Gnmx::DispatchDrawTriangleCullData*)m_acb.allocateFromCommandBuffer(sizeof(Gnmx::DispatchDrawTriangleCullData), Gnm::kEmbeddedDataAlignment4);
		memset(m_pDispatchDrawData, 0, sizeof(Gnmx::DispatchDrawTriangleCullData));
	}
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
#ifdef SCE_GNM_DEBUG
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
	SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "endDispatchDraw was not called before submit");

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
	int errSubmit = Gnm::submitCommandBuffers(m_submissionCount+1, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes);
	if (numAcbSubmits && errSubmit == sce::Gnm::kSubmissionSuccess) {
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
	return errSubmit;
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
	SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "endDispatchDraw was not called before submitAndFlip");

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
	int errSubmit = Gnm::submitAndFlipCommandBuffers(m_submissionCount+1, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes,
													 videoOutHandle, rtIndex, flipMode, flipArg);
	if (numAcbSubmits && errSubmit == sce::Gnm::kSubmissionSuccess) {
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
	return errSubmit;
}

int32_t GfxContext::submitAndFlip(uint32_t videoOutHandle, uint32_t rtIndex, uint32_t flipMode, int64_t flipArg,
								  void *labelAddr, uint32_t value)
{
	SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "endDispatchDraw was not called before submitAndFlip");

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
	int32_t errSubmit = Gnm::submitAndFlipCommandBuffers(m_submissionCount+1, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes,
														 videoOutHandle, rtIndex, flipMode, flipArg);
	if (numAcbSubmits && errSubmit == sce::Gnm::kSubmissionSuccess) {
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
	return errSubmit;
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
	SCE_GNM_VALIDATE((lsb && hsb) || (!lsb && !hsb), "lsb (0x%p) and hsb (0x%p) must either both be NULL, or both be non-NULL.", lsb, hsb);
	Gnm::LsStageRegisters lsRegs;
	Gnm::TessellationRegisters tessRegs;
	if ( hsb )
	{
		tessRegs.init(&hsb->m_hullStateConstants, numPatches);
		lsRegs = lsb->m_lsStageRegisters;
		lsRegs.updateLdsSize(&hsb->m_hullStateConstants,
							 lsb->m_lsStride, numPatches);	// set TG lds size
	}
	m_cue.setLsShader(lsb, shaderModifier, fetchShaderAddr, &lsRegs);
	m_cue.setHsShader(hsb, &tessRegs);
}

void GfxContext::setGsVsShaders(const GsShader *gsb)
{
	if ( gsb )
	{
		SCE_GNM_VALIDATE(!gsb->isOnChip(), "setGsVsShaders called with an on-chip GS shader; use setOnChipGsVsShaders instead");
		m_dcb.setGsMode(gsb->isOnChip() ? Gnm::kGsModeEnableOnChip : Gnm::kGsModeEnable, gsb->getGsMaxOutputVertexCount()); // TODO: use the entire gdb->copyShader->gsMode value
	}
	else
	{
		m_dcb.setGsMode(Gnm::kGsModeDisable, Gnm::kGsMaxOutputVertexCount1024);
	}

	m_cue.setGsVsShaders(gsb);
}

void GfxContext::setOnChipGsVsShaders(const GsShader *gsb, uint32_t gsPrimsPerSubGroup)
{
	if ( gsb )
	{
		uint16_t esVertsPerSubGroup = (uint16_t)((gsb->m_inputVertexCountMinus1+1)*gsPrimsPerSubGroup);
		SCE_GNM_VALIDATE(gsb->isOnChip(), "setOnChipGsVsShaders called with an off-chip GS shader; use setGsVsShaders instead");
		SCE_GNM_VALIDATE(gsPrimsPerSubGroup > 0, "gsPrimsPerSubGroup must be greater than 0");
		SCE_GNM_VALIDATE(gsPrimsPerSubGroup*gsb->getSizePerPrimitiveInBytes() <= 64*1024, "gsPrimsPerSubGroup*gsb->getSizePerPrimitiveInBytes() will not fit in 64KB LDS");
		SCE_GNM_VALIDATE(esVertsPerSubGroup <= 2047, "gsPrimsPerSubGroup*(gsb->m_inputVertexCountMinus1+1) can't be greater than 2047");
		m_dcb.setGsMode(Gnm::kGsModeEnableOnChip, gsb->getGsMaxOutputVertexCount()); // TODO: use the entire gdb->copyShader->gsMode value
		m_dcb.setGsOnChipControl(esVertsPerSubGroup, gsPrimsPerSubGroup);
		m_cue.setOnChipEsVertsPerSubGroup(esVertsPerSubGroup);
	}
	else
	{
		m_dcb.setGsMode(Gnm::kGsModeDisable, Gnm::kGsMaxOutputVertexCount1024);
	}

	m_cue.setGsVsShaders(gsb);
}


void GfxContext::setEsGsRingBuffer(void *baseAddr, uint32_t ringSize, uint32_t maxExportVertexSizeInDword)
{
	SCE_GNM_VALIDATE(baseAddr != NULL || ringSize == 0, "if baseAddr is NULL, ringSize must be 0.");
	Gnm::Buffer ringReadDescriptor;
	Gnm::Buffer ringWriteDescriptor;

	ringReadDescriptor.initAsEsGsReadDescriptor(baseAddr, ringSize);
	ringWriteDescriptor.initAsEsGsWriteDescriptor(baseAddr, ringSize);

	m_cue.setGlobalDescriptor(Gnm::kShaderGlobalResourceEsGsReadDescriptor, &ringReadDescriptor);
	m_cue.setGlobalDescriptor(Gnm::kShaderGlobalResourceEsGsWriteDescriptor, &ringWriteDescriptor);

	m_dcb.setupEsGsRingRegisters(maxExportVertexSizeInDword);
}

void GfxContext::setOnChipEsGsLdsLayout(uint32_t maxExportVertexSizeInDword)
{
	m_cue.setOnChipEsExportVertexSizeInDword((uint16_t)maxExportVertexSizeInDword);
	m_dcb.setupEsGsRingRegisters(maxExportVertexSizeInDword);
}

void GfxContext::setGsVsRingBuffers(void *baseAddr, uint32_t ringSize,
									const uint32_t vtxSizePerStreamInDword[4], uint32_t maxOutputVtxCount)
{
	SCE_GNM_VALIDATE(baseAddr != NULL || ringSize == 0, "if baseAddr is NULL, ringSize must be 0.");
	SCE_GNM_VALIDATE(vtxSizePerStreamInDword != NULL, "vtxSizePerStreamInDword must not be NULL.");
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

void GfxContext::setOnChipGsVsLdsLayout(const uint32_t vtxSizePerStreamInDword[4], uint32_t maxOutputVtxCount)
{
	m_dcb.setupGsVsRingRegisters(vtxSizePerStreamInDword, maxOutputVtxCount);
}

void GfxContext::setupDispatchDrawScreenViewport(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	// The standard dispatch draw triangle culling CS shader implements frustum culling in software, 
	// and so viewport settings must be passed to the shader in m_pDispatchDrawData.
	// The CS shader is slightly conservative in it's culling where quantization might affect the 
	// outcome, relying on the standard clipping/culling hardware to make the final determination for 
	// borderline cases and to clip triangles that require clipping.

	SCE_GNM_VALIDATE(m_pDispatchDrawData != NULL, "initDispatchDrawCommandBuffer must be called before setupDispatchDrawScreenViewport");
	// Unfortunately, we have to duplicate much of the work done in Gnmx::setupScreenViewport here:
	int32_t width = right - left;
	int32_t height = bottom - top;
	SCE_GNM_VALIDATE(width > 0 && width <= 16384, "right (%d) - left (%d) must be in the range [1..16384].", right, left);
	SCE_GNM_VALIDATE(height > 0 && height <= 16384, "bottom (%d) - top (%d) must be in the range [1..16384].", bottom, top);

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

	m_pDispatchDrawData->m_quantErrorScreenX = fQuantErrorX;
	m_pDispatchDrawData->m_quantErrorScreenY = fQuantErrorY;
	m_pDispatchDrawData->m_gbHorizClipAdjust = gbHorizontalClipAdjust;
	m_pDispatchDrawData->m_gbVertClipAdjust = gbVerticalClipAdjust;
}

void GfxContext::setupDispatchDrawClipCullSettings(PrimitiveSetup primitiveSetup, ClipControl clipControl)
{
	// The standard dispatch draw triangle culling CS shader implements back-face culling in software, 
	// and so settings which affect back-face culling must be passed to the shader in m_pDispatchDrawData.
	// The CS shader is slightly conservative in it's culling where quantization might affect the 
	// outcome, relying on the standard clipping/culling hardware to make the final determination for 
	// borderline cases.

	SCE_GNM_VALIDATE(m_pDispatchDrawData != NULL, "initDispatchDrawCommandBuffer must be called before setupDispatchDrawClipCullSettings");
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
	m_pDispatchDrawData->m_clipCullSettings = (uint16_t)((m_pDispatchDrawData->m_clipCullSettings &~kDispatchDrawClipCullMask) | (dispatchDrawClipCullFlags & kDispatchDrawClipCullMask));
}

void GfxContext::beginDispatchDraw()
{
	//To prevent dispatch draw deadlocks, this must be set to the value of the internal config register VGT_VTX_VECT_EJECT_REG + 2.  VGT_VTX_VECT_EJECT_REG = 127, so numPrimsPerVgt must be 129 for dispatch draw
	static const uint32_t numPrimsPerVgt = 129;
	SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) == 0, "beginDispatchDraw can't be called between beginDispatchDraw and endDispatchDraw");
	SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagIrbValid) != 0, "setupDispatchDrawRingBuffers must be called before beginDispatchDraw");

	// The standard triangle culling dispatch draw shaders are hard coded to produce 16-bit indices:
	m_dcb.setIndexSize(Gnm::kIndexSize16ForDispatchDraw);
	m_dcb.setIndexBuffer(m_pDispatchDrawData->m_bufferIrb.getBaseAddress());
	// To prevent deadlocks in dispatch draw, must set kVgtPartialVsWaveEnable and kVgtSwitchOnEopDisable:
	m_dcb.setVgtControl(static_cast<uint8_t>(numPrimsPerVgt-1), Gnm::kVgtPartialVsWaveEnable, Gnm::kVgtSwitchOnEopDisable);

	// If the previous draw was not dispatch draw, make sure the dispatch draw compute dispatches wait until the dispatch draw draw command is processed to start,
	// in order to make sure the dispatch draw compute waves do not fill up the GPU before the associated graphics waves can start consuming their output:
	uint32_t labelDispatchDraw = 0xDDC0DDC0;
	uint32_t *pLabelDispatchDraw = (uint32_t*)m_dcb.allocateFromCommandBuffer(sizeof(uint32_t), Gnm::kEmbeddedDataAlignment4);
	*pLabelDispatchDraw = 0;
	m_dcb.writeDataInline(pLabelDispatchDraw, &labelDispatchDraw, 1, Gnm::kWriteDataConfirmDisable);
	m_acb.waitOnAddress(pLabelDispatchDraw, 0xFFFFFFFF, Gnm::kWaitCompareFuncEqual, labelDispatchDraw);

	m_dispatchDrawFlags |= kDispatchDrawFlagInDispatchDraw;
}

void GfxContext::endDispatchDraw(IndexSize indexSize, const void* indexAddr, uint8_t primGroupSizeMinus1, VgtPartialVsWaveMode partialVsWaveMode, VgtSwitchOnEopMode switchOnEopMode)
{
	SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) != 0, "endDispatchDraw can only be called after a corresponding beginDispatchDraw");
	SCE_GNM_VALIDATE(indexSize == Gnm::kIndexSize16 || indexSize == Gnm::kIndexSize32, "indexSize can only be restored to kIndexSize16 or kIndexSize32");

	// Restore state for non-dispatchDraw rendering:
	m_dcb.setIndexSize(indexSize);
	m_dcb.setIndexBuffer(indexAddr);
	m_dcb.setVgtControl(primGroupSizeMinus1, partialVsWaveMode, switchOnEopMode);

	m_dispatchDrawFlags &=~kDispatchDrawFlagInDispatchDraw;
}

void GfxContext::dispatchDraw(DispatchDrawTriangleCullIndexData const *pDispatchDrawIndexData)
{
	//To prevent dispatch draw deadlocks, this must be set to the value of the internal config register VGT_VTX_VECT_EJECT_REG + 2.  VGT_VTX_VECT_EJECT_REG = 127, so numPrimsPerVgt must be 129 for dispatch draw
	static const uint32_t numPrimsPerVgt = 129;
	// The subDrawIndexCount must be a multiple of 2*numPrimsPerVgt primitives worth of indices
	// (numPrimsPerVgt*3*2 for triangle primitives) to prevent deadlocks.
	// There is not much reason to set it to any multiple of the minimum value (774 indices), as
	// that is already large enough that overhead from sub-draws is probably minimal, and using
	// a larger value would correspondingly reduce the effective IRB size.
	static const uint32_t subDrawIndexCount = numPrimsPerVgt*3*2;

	uint32_t firstBlock = 0;		//Do we need to be able to render a partial set of blocks?
	uint32_t indexOffset = 0;		//Do we need to be able to set the index offset for dispatch draw?

	SCE_GNM_VALIDATE((m_dispatchDrawFlags & kDispatchDrawFlagInDispatchDraw) != 0, "dispatchDraw can only be called between beginDispatchDraw and endDispatchDraw");
	SCE_GNM_VALIDATE(pDispatchDrawIndexData->m_magic == Gnmx::kDispatchDrawTriangleCullIndexDataMagic, "dispatchDraw: index data magic is wrong");
	SCE_GNM_VALIDATE(pDispatchDrawIndexData->m_versionMajor == Gnmx::kDispatchDrawTriangleCullIndexDataVersionMajor, "dispatchDraw: index data version major index does not match");
	SCE_GNM_VALIDATE(pDispatchDrawIndexData->m_numIndexDataBlocks > 0 && pDispatchDrawIndexData->m_numIndexDataBlocks <= 0xFFFF, "dispatchDraw: m_numIndexDataBlocks must be in the range [1:65536]");
	SCE_GNM_VALIDATE(pDispatchDrawIndexData->m_numIndexBits >= 1 && pDispatchDrawIndexData->m_numIndexBits <= 16, "dispatchDraw: m_numIndexBits must be in the range [1:16]");
	SCE_GNM_VALIDATE(pDispatchDrawIndexData->m_numIndexSpaceBits < pDispatchDrawIndexData->m_numIndexBits, "dispatchDraw: m_numIndexSpaceBits must be less than m_numIndexBits");

	// Store bufferInputData and numBlocksTotal to kShaderInputUsagePtrDispatchDraw data
	m_pDispatchDrawData->m_bufferInputIndexData = pDispatchDrawIndexData->m_bufferInputIndexData;
	m_pDispatchDrawData->m_numIndexDataBlocks = (uint16_t)pDispatchDrawIndexData->m_numIndexDataBlocks;
	m_pDispatchDrawData->m_numIndexBits = pDispatchDrawIndexData->m_numIndexBits;
	m_pDispatchDrawData->m_numInstancesPerTgMinus1 = pDispatchDrawIndexData->m_numInstancesPerTgMinus1;
	m_pDispatchDrawData->m_firstIndexDataBlock = (uint16_t)firstBlock;
	// Pass a pointer to the dispatch draw data for the CUE to pass to shaders as kShaderInputUsagePtrDispatchDraw.
	// Its contents must not be modified until after this dispatchDraw call's shaders have finished running.
	m_cue.setDispatchDrawData(m_pDispatchDrawData, sizeof(Gnmx::DispatchDrawTriangleCullData));
	{
		// We do not use the CCB to prefetch the dispatch draw control data.
		// Instead we allocate copies from the ACB which the CUE prefetches explicitly.
		// As the current version of DispatchDrawTriangleCullData contains data which changes with every draw call,
		// we must currently allocate and copy 68 bytes for every dispatchDraw call.
		// In the future, we could put constant data {m_bufferIrb, m_gdsOffsetOfIrbWptr, m_sizeofIrbInIndices} and
		// data which changes infrequently { m_clipCullSettings, m_quantErrorScreenX, m_quantErrorScreenY, 
		// m_gbHorizClipAdjust, m_gbVertClipAdjust } in a separate structure with a pointer in the base data, 
		// which would generally reduce this copy and command buffer allocation to 36 bytes per dispatchDraw call.
		Gnmx::DispatchDrawTriangleCullData* pDispatchDrawDataNew = (Gnmx::DispatchDrawTriangleCullData*)m_acb.allocateFromCommandBuffer(sizeof(Gnmx::DispatchDrawTriangleCullData), Gnm::kEmbeddedDataAlignment4);
		memcpy(pDispatchDrawDataNew, m_pDispatchDrawData, sizeof(Gnmx::DispatchDrawTriangleCullData));
		m_pDispatchDrawData = pDispatchDrawDataNew;
	}

	// m_cue.preDispatchDraw looks up settings from the currently set CsVsShader
	// NOTE: For the standard triangle culling dispatch draw shaders, many of these settings are fixed:
	//		orderedAppendMode = kDispatchOrderedAppendModeIndexPerThreadgroup
	//		dispatchDrawMode = kDispatchDrawModeIndexRingBufferOnly
	//		dispatchDrawIndexDeallocMask = 0xFC00  (CsShader.m_dispatchDrawIndexDeallocNumBits = 10)
	//		user SGPR locations may vary depending on the vertex shader source which is compiled
	Gnm::DispatchOrderedAppendMode orderedAppendMode;	//from CsShader.m_orderedAppendMode
	Gnm::DispatchDrawMode dispatchDrawMode = Gnm::kDispatchDrawModeIndexRingBufferOnly;	//from VsShader inputUsageSlots (kDispatchDrawModeIndexAndVertexRingBuffers if sgprVrbLoc is found)
	uint32_t dispatchDrawIndexDeallocMask = 0;	//from CsShader.m_dispatchDrawIndexDeallocNumBits
	uint32_t sgprKrbLoc = 0;	//get the user SGPR index from the CsShader inputUsageSlots kShaderInputUsageImmGdsKickRingBufferOffset, which must always be present
	uint32_t sgprVrbLoc = 0;	//get the user SGPR index from the VsShader inputUsageSlots kShaderInputUsageImmVertexRingBufferOffset, if any, or (uint32_t)-1 if not found; dispatchDrawMode returns kDispatchDrawModeIndexAndVertexRingBuffer if found.
	uint32_t sgprInstancesCs = 0;	//get the user SGPR index from the CsShader inputUsageSlots kShaderInputUsageImmDispatchDrawInstances, if any, or (uint32_t)-1 if not found
	uint32_t sgprInstancesVs = 0;	//get the user SGPR index from the VsShader inputUsageSlots kShaderInputUsageImmDispatchDrawInstances, if any, or (uint32_t)-1 if not found

	// Tell the CUE to set up CCB constant data for the current set shaders:
	m_cue.preDispatchDraw(&m_dcb, &m_acb, &m_ccb, &orderedAppendMode, &dispatchDrawIndexDeallocMask, &sgprKrbLoc, &sgprInstancesCs, &dispatchDrawMode, &sgprVrbLoc, &sgprInstancesVs);
	// Notify the GPU of the dispatchDrawIndexDeallocMask required by the current CsShader:
	if (dispatchDrawIndexDeallocMask != m_dispatchDrawIndexDeallocMask) {
		m_dispatchDrawIndexDeallocMask = dispatchDrawIndexDeallocMask;
		m_dcb.setDispatchDrawIndexDeallocationMask(dispatchDrawIndexDeallocMask);
	}

	uint32_t maxInstancesPerCall = (dispatchDrawIndexDeallocMask >> pDispatchDrawIndexData->m_numIndexBits) + ((dispatchDrawIndexDeallocMask & (0xFFFF << pDispatchDrawIndexData->m_numIndexSpaceBits)) != dispatchDrawIndexDeallocMask ? 1 : 0);
	if (maxInstancesPerCall == 0) {
		// For the standard triangle culling dispatch draw shaders, dispatchDrawIndexDeallocMask = 0xFC00,
		// which limits dispatchDraw calls to use no more than 63K (0xFC00) indices:
		uint32_t mask = dispatchDrawIndexDeallocMask, dispatchDrawIndexDeallocNumBits = 0;
		if (!(mask & 0xFF))	mask >>= 8, dispatchDrawIndexDeallocNumBits |= 8;
		if (!(mask & 0xF))	mask >>= 4, dispatchDrawIndexDeallocNumBits |= 4;
		if (!(mask & 0x3))	mask >>= 2, dispatchDrawIndexDeallocNumBits |= 2;
		dispatchDrawIndexDeallocNumBits |= (0x1 &~ mask);
		SCE_GNM_VALIDATE(maxInstancesPerCall > 0, "dispatchDraw requires numIndexBits (%u) < 16 or numIndexSpaceBits (%u) > m_dispatchDrawIndexDeallocNumBits (%u) for the asynchronous compute shader", pDispatchDrawIndexData->m_numIndexBits, pDispatchDrawIndexData->m_numIndexSpaceBits, dispatchDrawIndexDeallocNumBits);
		m_cue.postDispatchDraw(&m_dcb, &m_acb, &m_ccb);
		return;
	}

	uint32_t numInstancesMinus1 = (m_dispatchDrawNumInstancesMinus1 & 0xFFFF);
	uint32_t numCalls = 1, numTgY = 1, numInstancesPerCall = 1;
	uint32_t numTgYLastCall = 1, numInstancesLastCall = 1;
	if (numInstancesMinus1 != 0) {
		// To implement instancing in software in an efficient way, the CS shader can pack some of the bits
		// of the instanceId into the output index data, provided the output indices are using a small enough
		// range of the available 63K index space.
		// This allows each dispatchDraw command buffer command to render multiple instances, reducing the
		// command processing and dispatch/draw overhead by a corresponding factor.  
		// If the total number of instances is greater than the number which can be rendered within a single 
		// dispatch, only a change to the kShaderInputUsageImmDispatchDrawInstances user SGPRs is required
		// between each dispatch, which keeps the overhead of additional instances to a minimum.
		// In addition, if an object is smaller than 256 triangles, it becomes possible to render multiple
		// instances of that object in a single thread group (each of which processes up to 512 triangles),
		// which correspondingly reduces the number of thread groups launched.

		// Here, we calculate how many dispatches and how many thread groups we will have to launch to
		// render the required total number of instances:
		uint32_t numInstances = numInstancesMinus1 + 1;
		uint32_t maxInstancesPerTg = pDispatchDrawIndexData->m_numInstancesPerTgMinus1+1;
		numInstancesLastCall = numInstances;
		numInstancesPerCall = numInstances;
		if (numInstances > maxInstancesPerCall) {
			numCalls = (numInstances + maxInstancesPerCall-1)/maxInstancesPerCall;
			numInstancesPerCall = maxInstancesPerCall;
			numInstancesLastCall -= (numCalls - 1) * maxInstancesPerCall;
		}
		if (numInstancesPerCall > maxInstancesPerTg)
			numTgY = (numInstancesPerCall + maxInstancesPerTg-1)/maxInstancesPerTg;
		if (numInstancesLastCall > maxInstancesPerTg)
			numTgYLastCall = (numInstancesLastCall + maxInstancesPerTg-1)/maxInstancesPerTg;
#ifdef SCE_GNM_DEBUG
		SCE_GNM_VALIDATE(numInstancesPerCall*(numCalls-1) + numInstancesLastCall == numInstances && numTgY*(numCalls-1) + numTgYLastCall == (numInstances + maxInstancesPerTg-1)/maxInstancesPerTg, "dispatchDraw instancing internal error");
#endif
	}

	SCE_GNM_VALIDATE(sgprKrbLoc < 16, "dispatchDraw requires an asynchronous compute shader with a kShaderInputUsageImmKickRingBufferOffset userdata sgpr");
	SCE_GNM_VALIDATE(dispatchDrawMode == Gnm::kDispatchDrawModeIndexRingBufferOnly || sgprVrbLoc < 16, "dispatchDraw with a vertex ring buffer requires a VS shader with a kShaderInputUsageImmVertexRingBufferOffset userdata sgpr");
	SCE_GNM_VALIDATE((numInstancesMinus1 == 0) || (sgprInstancesCs < 16 && sgprInstancesVs < 16), "dispatchDraw with instancing requires asynchronous compute and VS shaders with kShaderInputUsageImmDispatchDrawInstances userdata sgprs");

	// Here we iterate over however many dispatchDraw commands are required to render all requested instances,
	// adjusting the kShaderInputUsageImmDispatchDrawInstances user SGPRs for each call:
	uint32_t firstInstance = 0;
	for (uint32_t nCall = 0; nCall+1 < numCalls; ++nCall, firstInstance += numInstancesPerCall) {
		uint32_t dispatchDrawInstances = (firstInstance<<16)|(numInstancesPerCall-1);
		m_acb.setUserData(sgprInstancesCs, dispatchDrawInstances);
		m_dcb.setUserData(Gnm::kShaderStageVs, sgprInstancesVs, dispatchDrawInstances);
		m_acb.dispatchDraw(pDispatchDrawIndexData->m_numIndexDataBlocks, numTgY, 1, orderedAppendMode, sgprKrbLoc);
		m_dcb.dispatchDraw(Gnm::kPrimitiveTypeTriList, indexOffset, subDrawIndexCount, dispatchDrawMode, sgprVrbLoc);
	}
	{
		uint32_t dispatchDrawInstances = (firstInstance<<16)|(numInstancesLastCall-1);
		if (sgprInstancesCs < 16)
			m_acb.setUserData(sgprInstancesCs, dispatchDrawInstances);
		if (sgprInstancesVs < 16)
			m_dcb.setUserData(Gnm::kShaderStageVs, sgprInstancesVs, dispatchDrawInstances);
	}
	m_acb.dispatchDraw(pDispatchDrawIndexData->m_numIndexDataBlocks, numTgYLastCall, 1, orderedAppendMode, sgprKrbLoc);
	m_dcb.dispatchDraw(Gnm::kPrimitiveTypeTriList, indexOffset, subDrawIndexCount, dispatchDrawMode, sgprVrbLoc);

	// Notify the CUE that we are done issuing draw commands that will refer to the current CCB constant data:
	m_cue.postDispatchDraw(&m_dcb, &m_acb, &m_ccb);
}

void GfxContext::dispatchDrawComputeWaitForEndOfPipe()
{
	uint32_t labelDispatchDraw = 0xDDCEDDCE;
	uint64_t *pLabelDispatchDraw = (uint64_t*)m_dcb.allocateFromCommandBuffer(sizeof(uint64_t), Gnm::kEmbeddedDataAlignment8);
	*pLabelDispatchDraw = 0;
	m_dcb.writeAtEndOfPipe(Gnm::kEopCbDbReadsDone, Gnm::kEventWriteDestMemory, pLabelDispatchDraw, Gnm::kEventWriteSource32BitsImmediate, labelDispatchDraw, Gnm::kCacheActionNone, Gnm::kCachePolicyLru);
	m_acb.waitOnAddress(pLabelDispatchDraw, 0xFFFFFFFF, Gnm::kWaitCompareFuncEqual, labelDispatchDraw);
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

bool GfxContext::splitCommandBuffers(void)
{
	// Register a new submit up to the current DCB/CCB command pointers
	if (m_submissionCount >= GfxContext::kMaxNumStoredSubmissions)
	{
		SCE_GNM_VALIDATE(m_submissionCount < GfxContext::kMaxNumStoredSubmissions, "Out of space for stored submissions. More can be added by increasing kMaxNumStoredSubmissions.");
		return false;
	}
	m_submissionRanges[m_submissionCount].m_dcbStartDwordOffset = (uint32_t)(m_currentDcbSubmissionStart - m_dcb.m_beginptr);
	m_submissionRanges[m_submissionCount].m_dcbSizeInDwords     = (uint32_t)(m_dcb.m_cmdptr - m_currentDcbSubmissionStart);
	m_submissionRanges[m_submissionCount].m_ccbStartDwordOffset = (uint32_t)(m_currentCcbSubmissionStart - m_ccb.m_beginptr);
	m_submissionRanges[m_submissionCount].m_ccbSizeInDwords     = (uint32_t)(m_ccb.m_cmdptr - m_currentCcbSubmissionStart);
	m_submissionCount++;
	m_currentDcbSubmissionStart = m_dcb.m_cmdptr;
	m_currentCcbSubmissionStart = m_ccb.m_cmdptr;
	// Advance CB end pointers to the next (possibly artificial) boundary -- either current+(4MB-4), or the end of the actual buffer
	m_dcb.m_endptr = std::min(m_dcb.m_cmdptr+kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)m_actualDcbEnd);
	m_ccb.m_endptr = std::min(m_ccb.m_cmdptr+kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)m_actualCcbEnd);
	return true;
}

#ifdef SCE_GNMX_ENABLE_GFXCONTEXT_CALLCOMMANDBUFFERS
void GfxContext::callCommandBuffers(void *dcbAddr, uint32_t dcbSizeInDwords, void *ccbAddr, uint32_t ccbSizeInDwords)
{
	if (dcbSizeInDwords > 0)
	{
		SCE_GNM_VALIDATE(dcbAddr != NULL, "dcbAddr must not be NULL if dcbSizeInDwords > 0");
		m_dcb.chainCommandBuffer(dcbAddr, dcbSizeInDwords);
	}
	if (ccbSizeInDwords > 0)
	{
		SCE_GNM_VALIDATE(ccbAddr != NULL, "ccbAddr must not be NULL if ccbSizeInDwords > 0");
		m_ccb.chainCommandBuffer(ccbAddr, ccbSizeInDwords);
	}
	if (dcbSizeInDwords == 0 && ccbSizeInDwords == 0)
	{
		return;
	}
	splitCommandBuffers();
	m_cue.invalidateAllBindings();
}
#endif

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
	SCE_GNM_VALIDATE((uintptr_t(srcPersistentBuffer) & 0xFF) == 0, "srcPersistentBuffer (0x%p) must be 256-byte aligned.", srcPersistentBuffer);

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
	int errSubmit = Gnm::submitCommandBuffers(m_submissionCount, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes);
	if (numAcbSubmits && errSubmit == sce::Gnm::kSubmissionSuccess) {
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
	return errSubmit;
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

	int errSubmit = Gnm::submitAndFlipCommandBuffers(m_submissionCount, dcbGpuAddrs, dcbSizes, ccbGpuAddrs, ccbSizes,
													 videoOutHandle, rtIndex, flipMode, flipArg);
	if (numAcbSubmits && errSubmit == sce::Gnm::kSubmissionSuccess) {
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
	return errSubmit;
}

#endif // !defined(SCE_GNM_OFFLINE_MODE)
