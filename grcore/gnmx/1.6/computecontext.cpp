/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/computecontext.h"
#include <gnm/gpumem.h>
#include <gnm/platform.h>

#include <algorithm>

using namespace sce::Gnm;
using namespace sce::Gnmx;

namespace
{
	const uint32_t kSerializedContextTempHeaderVersion = 2;
	const uint32_t kSerializedContextTempHeaderMagic = 0x00544343; // "CCT\0"
	struct SerializedContextTempHeader
	{
		uint32_t m_magic;
		uint32_t m_version;

		uint32_t m_submissionRangesByteOffset; ///< byte offset from the beginning of the file to the beginning of the SubmissionRange[] array
		uint32_t m_submissionRangeCount;
		uint32_t m_dcbPatchTableByteOffset;
		uint32_t m_dcbPatchTableSizeInBytes;
	};

	const uint32_t kSerializedContextPersistentHeaderVersion = 2;
	const uint32_t kSerializedContextPersistentHeaderMagic = 0x00504343; // "CCP\0"
	struct SerializedContextPersistentHeader
	{
		uint32_t m_magic;
		uint32_t m_version;

		uint32_t m_dcbByteOffset;
		uint32_t m_dcbSizeInDwords;
	};
}

static bool handleReserveFailed(CommandBuffer *cb, uint32_t dwordCount, void *userData)
{
	ComputeContext &cmpc = *(ComputeContext*)userData;
	// If either command buffer has actually reached the end of its full buffer, then invoke m_bufferFullCallback (if one has been registered)
	if (static_cast<void*>(cb) == static_cast<void*>(&(cmpc.m_dcb)) && cmpc.m_dcb.m_cmdptr + dwordCount > cmpc.m_actualDcbEnd)
	{
		if (cmpc.m_bufferFullCallback.m_func != 0)
			return cmpc.m_bufferFullCallback.m_func(cb, dwordCount, cmpc.m_bufferFullCallback.m_userData);
		else
		{
			SCE_GNM_ERROR("Out of DCB command buffer space, and no callback bound.");
			return false;
		}
	}
	// Register a new submission up to the current DCB command pointer
	if (cmpc.m_submissionCount >= ComputeContext::kMaxNumStoredSubmissions)
	{
		SCE_GNM_VALIDATE(cmpc.m_submissionCount < ComputeContext::kMaxNumStoredSubmissions, "Out of space for stored submissions. More can be added by increasing kMaxNumStoredSubmissions.");
		return false;
	}
	cmpc.m_submissionRanges[cmpc.m_submissionCount].m_dcbStartDwordOffset = (uint32_t)(cmpc.m_currentDcbSubmissionStart - cmpc.m_dcb.m_beginptr);
	cmpc.m_submissionRanges[cmpc.m_submissionCount].m_dcbSizeInDwords     = (uint32_t)(cmpc.m_dcb.m_cmdptr - cmpc.m_currentDcbSubmissionStart);
	cmpc.m_submissionCount++;
	cmpc.m_currentDcbSubmissionStart = cmpc.m_dcb.m_cmdptr;
	// Advance CB end pointers to the next (possibly artificial) boundary -- either current+(4MB-4), or the end of the actual buffer
	cmpc.m_dcb.m_endptr = std::min(cmpc.m_dcb.m_cmdptr+kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)cmpc.m_actualDcbEnd);
	return true;
}

ComputeContext::ComputeContext(void)
{
}

ComputeContext::~ComputeContext(void)
{
}

void ComputeContext::init(void *dcbBuffer, uint32_t dcbSizeInBytes)
{
	m_dcb.init(dcbBuffer, std::min((uint32_t)Gnm::kIndirectBufferMaximumSizeInBytes, dcbSizeInBytes), handleReserveFailed, this);

	for(uint32_t iSub=0; iSub<kMaxNumStoredSubmissions; ++iSub)
	{
		m_submissionRanges[iSub].m_dcbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_dcbSizeInDwords = 0;
	}
	m_submissionCount = 0;
	m_currentDcbSubmissionStart = m_dcb.m_beginptr;
	m_actualDcbEnd = (uint32_t*)dcbBuffer+(dcbSizeInBytes/4);
	m_bufferFullCallback.m_func = NULL;
	m_bufferFullCallback.m_userData = NULL;
	m_UsingLightweightConstantUpdateEngine = false;
}

#if !defined(SCE_GNM_OFFLINE_MODE)
void ComputeContext::init(uint32_t* dcbBuffer, uint32_t dcbBufferSizeInDwords, uint32_t* resourceBufferInGarlic, int32_t resourceBufferSizeInDwords, uint32_t* globalInternalResourceTableAddr)
{
	SCE_GNM_VALIDATE(resourceBufferInGarlic != NULL, "resourceBufferInGarlic must not be NULL. A resource Buffer is required when using the lightweight Constant Update Engine");

	m_dcb.init(dcbBuffer, std::min((uint32_t)Gnm::kIndirectBufferMaximumSizeInBytes, dcbBufferSizeInDwords), handleReserveFailed, this);

	for(uint32_t iSub=0; iSub<kMaxNumStoredSubmissions; ++iSub)
	{
		m_submissionRanges[iSub].m_dcbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_dcbSizeInDwords = 0;
	}
	m_submissionCount = 0;
	m_currentDcbSubmissionStart = m_dcb.m_beginptr;
	m_actualDcbEnd = (uint32_t*)dcbBuffer+(dcbBufferSizeInDwords/4);
	m_bufferFullCallback.m_func = NULL;
	m_bufferFullCallback.m_userData = NULL;

	m_lwcue.init(&resourceBufferInGarlic, 1, resourceBufferSizeInDwords, globalInternalResourceTableAddr);
	m_lwcue.setDispatchCommandBuffer(&m_dcb);
	m_UsingLightweightConstantUpdateEngine = true;

#if defined (SCE_GNM_LCUE_CLEAR_HARDWARE_KCACHE)
	// first order of business, invalidate the KCache/L1/L2 to rid us of any stale data
	m_dcb.flushShaderCachesAndWait(Gnm::kCacheActionWriteBackAndInvalidateL1andL2, Gnm::kExtendedCacheActionInvalidateKCache);
#endif
}
#endif // !defined(SCE_GNM_OFFLINE_MODE)

void ComputeContext::reset(void)
{
	m_dcb.resetBuffer();
	// Restore end pointer to artificial limits
	m_dcb.m_endptr = std::min(m_dcb.m_cmdptr+Gnm::kIndirectBufferMaximumSizeInBytes/4, (uint32_t*)m_actualDcbEnd);

	// Restore submission ranges to default values
	for(uint32_t iSub=0; iSub<kMaxNumStoredSubmissions; ++iSub)
	{
		m_submissionRanges[iSub].m_dcbStartDwordOffset = 0;
		m_submissionRanges[iSub].m_dcbSizeInDwords = 0;
	}
	m_submissionCount = 0;
	m_currentDcbSubmissionStart = m_dcb.m_beginptr;

#if !defined(SCE_GNM_OFFLINE_MODE)
	// swap buffers for the LCUE
	if ( m_UsingLightweightConstantUpdateEngine )
		m_lwcue.swapBuffers();
#endif // !defined(SCE_GNM_OFFLINE_MODE)
}

#if defined(SCE_GNM_OFFLINE_MODE)
void ComputeContext::getSerializedSizes(size_t *outTempBufferSize, size_t *outPersistentBufferSize) const
{
	SCE_GNM_VALIDATE(outTempBufferSize         != NULL,         "outTempBufferSize must not be NULL.");
	SCE_GNM_VALIDATE(outPersistentBufferSize   != NULL,   "outPersistentBufferSize must not be NULL.");
	SCE_GNM_VALIDATE(m_dcb.m_patchTableBuilder != NULL, "m_dcb.m_patchTableBuilder must not be NULL.");
	size_t dcbPatchTableSize = m_dcb.m_patchTableBuilder->getSerializedSize();

	size_t dcbSize = (m_dcb.m_cmdptr - m_dcb.m_beginptr) * sizeof(uint32_t);

	size_t submissionTableSize = (m_submissionCount+1)*sizeof(SubmissionRange); // m_submissionRanges array, plus current DCB

	*outTempBufferSize = sizeof(SerializedContextTempHeader) + submissionTableSize + dcbPatchTableSize;
	// DCB must start on a 256-byte boundary
	*outPersistentBufferSize = ((sizeof(SerializedContextPersistentHeader) + 0xFF) & ~0xFF) +
		dcbSize;
	return;
}

void ComputeContext::serializeIntoBuffers(void *destTempBuffer, size_t tempBufferSize, void *destPersistentBuffer, size_t persistentBufferSize) const
{
#ifdef GNM_DEBUG
	size_t minTempBufferSize = 0, minPersistentBufferSize = 0;
	getSerializedSizes(&minTempBufferSize, &minPersistentBufferSize);
	SCE_GNM_VALIDATE(destTempBuffer, "destTempBuffer must not be NULL.");
	SCE_GNM_VALIDATE(tempBufferSize >= minTempBufferSize, "tempBufferSize (%d bytes) is smaller than the required size (%d bytes). Use getSerializedSize().", tempBufferSize, minTempBufferSize);
	SCE_GNM_VALIDATE(destPersistentBuffer, "destPersistentBuffer must not be NULL.");
	SCE_GNM_VALIDATE(persistentBufferSize >= minPersistentBufferSize, "persistentBufferSize (%d bytes) is smaller than the required size (%d bytes). Use getSerializedSize().", persistentBufferSize, minPersistentBufferSize);
	SCE_GNM_VALIDATE(m_dcb.m_patchTableBuilder != NULL, "m_dcb.m_patchTableBuilder must not be NULL.");
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
	tempHeader.m_submissionRangesByteOffset = sizeof(SerializedContextTempHeader);
	tempHeader.m_submissionRangeCount = m_submissionCount+1; // include the submission currently under construction
	tempHeader.m_dcbPatchTableByteOffset  = tempHeader.m_submissionRangesByteOffset + tempHeader.m_submissionRangeCount * sizeof(SubmissionRange);
	tempHeader.m_dcbPatchTableSizeInBytes = (uint32_t)m_dcb.m_patchTableBuilder->getSerializedSize();
	memcpy(tempBufferBytes, &tempHeader, sizeof(SerializedContextTempHeader));
	tempBufferBytes += sizeof(SerializedContextTempHeader);

	// Write SubmissionRange table
	SCE_GNM_VALIDATE(tempBufferBytes - (uint8_t*)destTempBuffer == (ptrdiff_t)tempHeader.m_submissionRangesByteOffset, "Incorrect byte offset for submission table");
	memcpy(tempBufferBytes, m_submissionRanges, m_submissionCount * sizeof(SubmissionRange));
	tempBufferBytes += m_submissionCount*sizeof(SubmissionRange);
	// Append the current submission to the table
	SubmissionRange currentSubmissionRange;
	currentSubmissionRange.m_dcbStartDwordOffset = (uint32_t)(m_currentDcbSubmissionStart - m_dcb.m_beginptr);
	currentSubmissionRange.m_dcbSizeInDwords     = (uint32_t)(m_dcb.m_cmdptr - m_currentDcbSubmissionStart);
	memcpy(tempBufferBytes, &currentSubmissionRange, sizeof(SubmissionRange));
	tempBufferBytes += sizeof(SubmissionRange);

	// Write DCB patch table
	SCE_GNM_VALIDATE(tempBufferBytes - (uint8_t*)destTempBuffer == (ptrdiff_t)tempHeader.m_dcbPatchTableByteOffset, "Incorrect byte offset for DCB patch table");
	m_dcb.m_patchTableBuilder->serializeIntoBuffer(tempBufferBytes, tempHeader.m_dcbPatchTableSizeInBytes);
	tempBufferBytes += tempHeader.m_dcbPatchTableSizeInBytes;

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
	persistentHeader.m_dcbByteOffset = (sizeof(SerializedContextPersistentHeader) + 0xFF) & ~0xFF;
	persistentHeader.m_dcbSizeInDwords = (uint32_t)(m_dcb.m_cmdptr - m_dcb.m_beginptr);
	memcpy(persistentBufferBytes, &persistentHeader, sizeof(SerializedContextPersistentHeader));

	// Write DCB
	persistentBufferBytes = (uint8_t*)destPersistentBuffer + persistentHeader.m_dcbByteOffset;
	memcpy(persistentBufferBytes, m_dcb.m_beginptr, persistentHeader.m_dcbSizeInDwords*sizeof(uint32_t));
}
#endif // !defined(SCE_GNM_OFFLINE_MODE)

//////////////////////////////////////////////////////////////////////////////////
#if !defined(SCE_GNM_OFFLINE_MODE)
ComputeContextSubmitOnly::ComputeContextSubmitOnly(void)
{
}

ComputeContextSubmitOnly::~ComputeContextSubmitOnly(void)
{
}

void ComputeContextSubmitOnly::init(void *srcTempBuffer, uint32_t srcTempBufferSize, void *srcPersistentBuffer, uint32_t srcPersistentBufferSize)
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
	SCE_GNM_VALIDATE(tempHeader.m_submissionRangeCount <= kMaxNumStoredSubmissions, "srcTempBuffer's submission count (%d) must be <= kMaxNumStoredSubmissions (%d).", tempHeader.m_submissionRangeCount, kMaxNumStoredSubmissions);
	SerializedContextPersistentHeader &persistentHeader = *(SerializedContextPersistentHeader*)srcPersistentBuffer;
	SCE_GNM_VALIDATE(persistentHeader.m_version == kSerializedContextPersistentHeaderVersion, "Version mismatch in srcTempBuffer (expected %d, actual %d).", kSerializedContextPersistentHeaderVersion, persistentHeader.m_version);
	SCE_GNM_VALIDATE(persistentHeader.m_magic == kSerializedContextPersistentHeaderMagic, "Magic number mismatch in srcTempBuffer (expected 0x%08X, actual 0x%08X).", kSerializedContextPersistentHeaderMagic, persistentHeader.m_magic);

	uint8_t *tempBufferBytes       = (uint8_t*)srcTempBuffer;
	uint8_t *persistentBufferBytes = (uint8_t*)srcPersistentBuffer;

	m_dcbPatchTable.init(tempBufferBytes + tempHeader.m_dcbPatchTableByteOffset, tempHeader.m_dcbPatchTableSizeInBytes);
	m_submissionCount = tempHeader.m_submissionRangeCount;
	memcpy(m_submissionRanges, tempBufferBytes + tempHeader.m_submissionRangesByteOffset, tempHeader.m_submissionRangeCount*sizeof(ComputeContext::SubmissionRange));

	m_dcb.init(persistentBufferBytes + persistentHeader.m_dcbByteOffset, persistentHeader.m_dcbSizeInDwords*sizeof(uint32_t), NULL, NULL);

	// Use the CUE heap as the custom base for DWORD offsets in the patch table
	//m_dcbPatchTable.setCustomDwordOffsetBaseAddress(m_cueHeapAddr);
}

bool ComputeContextSubmitOnly::setAddressForPatchId(uint32_t id, void *finalAddr)
{
	return m_dcbPatchTable.setAddressForPatchId(id, finalAddr);
}

void ComputeContextSubmitOnly::patchCommandBuffers(void)
{
	m_dcbPatchTable.applyPatchesToCommandBuffer(&m_dcb);
}
#endif // !defined(SCE_GNM_OFFLINE_MODE)
