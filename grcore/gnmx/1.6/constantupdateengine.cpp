/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.071
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "grcore/gnmx/common.h"
#ifndef CUE_V2

#include <gnm.h>
#include <gnm/measuredrawcommandbuffer.h>
#include <gnm/measureconstantcommandbuffer.h>
#include <gnm/measuredispatchcommandbuffer.h>
#include "grcore/gnmx/constantupdateengine.h"
#include "grcore/gnmx/shaderbinary.h"
using namespace sce::Gnm;
using namespace sce::Gnmx;

#include <algorithm>
#include <cstring>

// If undefined or 0, passing NULL to one of the set[ResourceType]s() functions will early-out with no further effect.
// If defined to non-zero, passing a NULL pointer to one of the set[ResourceType]s() functions will actually zero out the
// appropriate regions of CPRAM. This is *not* necessary for regular operation (there is no cost associated with leaving
// stale values in unused/unreferenced resource slots), and indeed will actually generate additional work for the CPU/GPU.
// However, it may be useful for debugging.
#define SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM 0

#define ENABLE_HARDWARE_KCACHE

namespace
{
	// bitfield manipulation functions
	inline void setBit(uint8_t *bytes, uint32_t bitIndex)
	{
		bytes[bitIndex/8] |= (1 << (bitIndex%8));
	}
	inline void clearBit(uint8_t *bytes, uint32_t bitIndex)
	{
		bytes[bitIndex/8] &= ~(1 << (bitIndex%8));
	}
	inline bool testBit(const uint8_t *bytes, uint32_t bitIndex)
	{
		return (bytes[bitIndex/8] & (1 << (bitIndex%8))) != 0;
	}

	const uint32_t kWaitOnDeCounterDiffSizeInDword = 2;
}

uint32_t ConstantUpdateEngine::CueRingBuffer::computeSpaceRequirements(uint32_t elemSizeDwords, uint32_t elemCount)
{
	return (elemSizeDwords)*elemCount*sizeof(uint32_t);
}

void ConstantUpdateEngine::CueRingBuffer::init(void *bufferAddr, uint32_t bufferBytes, uint32_t elemSizeDwords, uint32_t elemCount)
{
	SCE_GNM_VALIDATE(bufferBytes >= computeSpaceRequirements(elemSizeDwords, elemCount), "bufferBytes (%d) is too small; use computeSpaceRequirements() to determine the minimum size.", bufferBytes);
	SCE_GNM_UNUSED(bufferBytes);

	m_headElemIndex = elemCount-1;
	m_elementsAddr = bufferAddr;
	m_elemSizeDwords = elemSizeDwords;
	SCE_GNM_ASSERT(elemCount > 1); // Need at least two elements
	m_elemCount = elemCount;
	m_wrappedIndex = 0;
	m_halfPointIndex = m_elemCount/2;
}

// Gets a pointer to the current head element.
void *ConstantUpdateEngine::CueRingBuffer::getCurrentHead(void) const
{
	void *headAddr = (uint8_t*)m_elementsAddr + (m_headElemIndex * m_elemSizeDwords * sizeof(uint32_t));
#if defined(SCE_GNM_OFFLINE_MODE)
	// In offline mode, the heap base address should be 0, so all ring buffer addresses are effectively byte offsets into the CUE heap.
	// Convert to a DWORD offset and disguise (using the custom base address). At runtime, set the custom base address to the *real* CUE heap.
	headAddr = Gnm::CommandBufferPatchTableBuilder::disguiseDwordOffsetAsAddress((uint32_t)uintptr_t(headAddr), Gnm::kDwordOffsetBaseCustom);
#endif
	return headAddr;
}

// Advances the head pointer (if the buffer isn't stalled) and returns a pointer to the new head element.
bool ConstantUpdateEngine::CueRingBuffer::advanceHead(void)
{
	m_headElemIndex = (m_headElemIndex+1) % m_elemCount;
	return m_headElemIndex == m_wrappedIndex || m_headElemIndex == m_halfPointIndex;
}

void ConstantUpdateEngine::CueRingBuffer::submitted()
{
	m_wrappedIndex = m_headElemIndex;
	m_halfPointIndex = (m_headElemIndex + m_elemCount/2)%m_elemCount;
}

uint32_t ConstantUpdateEngine::computeHeapSize(uint32_t numRingEntries)
{
	// Note: this code is duplicated in the beginning of ConstantUpdateEngine::init()
	const uint32_t kResourceRingBufferBytes	        = CueRingBuffer::computeSpaceRequirements(Gnm::kSlotCountResource        *Gnm::kDwordSizeResource,         numRingEntries);
	const uint32_t kRwResourceRingBufferBytes	    = CueRingBuffer::computeSpaceRequirements(Gnm::kSlotCountRwResource      *Gnm::kDwordSizeRwResource,       numRingEntries);
	const uint32_t kSamplerRingBufferBytes		    = CueRingBuffer::computeSpaceRequirements(Gnm::kSlotCountSampler         *Gnm::kDwordSizeSampler,          numRingEntries);
	const uint32_t kVertexBufferRingBufferBytes     = CueRingBuffer::computeSpaceRequirements(Gnm::kSlotCountVertexBuffer    *Gnm::kDwordSizeVertexBuffer,     numRingEntries);
	const uint32_t kConstantBufferRingBufferBytes   = CueRingBuffer::computeSpaceRequirements(Gnm::kSlotCountConstantBuffer  *Gnm::kDwordSizeConstantBuffer,   numRingEntries);
	const uint32_t kStreamoutBufferRingBufferBytes  = CueRingBuffer::computeSpaceRequirements(Gnm::kSlotCountStreamoutBuffer *Gnm::kDwordSizeStreamoutBuffer,  numRingEntries);
	const uint32_t kExtendedUserDataRingBufferBytes = CueRingBuffer::computeSpaceRequirements(Gnm::kSlotCountExtendedUserData*Gnm::kDwordSizeExtendedUserData, numRingEntries);
	const uint32_t kDispatchDrawDataRingBufferBytes = CueRingBuffer::computeSpaceRequirements(Gnm::kSlotCountDispatchDrawData*Gnm::kDwordSizeDispatchDrawData, numRingEntries);
	const uint32_t numStages = Gnm::kShaderStageCount;
	const uint32_t totalRingBufferRequiredSize = numStages * (kChunkCountResource        *kResourceRingBufferBytes +
															  kChunkCountRwResource      *kRwResourceRingBufferBytes +
															  kChunkCountSampler         *kSamplerRingBufferBytes +
															  kChunkCountVertexBuffer    *kVertexBufferRingBufferBytes +
															  kChunkCountConstantBuffer  *kConstantBufferRingBufferBytes +
															  kChunkCountStreamoutBuffer *kStreamoutBufferRingBufferBytes + 
															  kChunkCountExtendedUserData*kExtendedUserDataRingBufferBytes + 
															  kChunkCountDispatchDrawData*kDispatchDrawDataRingBufferBytes) + 
												 0;

	return totalRingBufferRequiredSize;
}

uint32_t ConstantUpdateEngine::computeCpRamShadowSize()
{
	return 48*1024;
}

ConstantUpdateEngine::ConstantUpdateEngine(void)
{
}
ConstantUpdateEngine::~ConstantUpdateEngine()
{
}

static const int kShaderStageAcbCs = sce::Gnm::kShaderStageCount;

void ConstantUpdateEngine::init(void *cpramShadowBuffer, void *heapAddr, uint32_t numRingEntries)
{
	SCE_GNM_VALIDATE((uintptr_t(heapAddr) & 0x3) == 0, "heapAddr (0x%010llX) must be aligned to a 4-byte boundary.", heapAddr);
	SCE_GNM_VALIDATE(((uintptr_t)cpramShadowBuffer & 0x3) == 0, "cpramShaderBuffer (0x%010llX) must be aligned to a 4-byte boundary.", cpramShadowBuffer);
#if defined(ENABLE_HARDWARE_KCACHE)
	SCE_GNM_VALIDATE(numRingEntries>=4, "If hardware kcache invalidation is enabled, numRingEntries (%d) must be at least 4.", numRingEntries);
#endif //ENABLE_HARDWARE_KCACHE

#if defined(SCE_GNM_OFFLINE_MODE)
	SCE_GNM_VALIDATE(uintptr_t(heapAddr) == 0, "In offline mode, heapAddr should always be 0 in order for serialization and runtime patching to work correctly.");
#endif

	m_cpRamShadowDwords = (uint32_t*)cpramShadowBuffer;
	memset(m_cpRamShadowDwords, 0, computeCpRamShadowSize());

	memset(m_dirtyResourceChunkBits, 0, sizeof(m_dirtyResourceChunkBits));
	memset(m_dirtyRwResourceChunkBits, 0, sizeof(m_dirtyRwResourceChunkBits));
	memset(m_dirtySamplerChunkBits, 0, sizeof(m_dirtySamplerChunkBits));
	memset(m_dirtyVertexBufferChunkBits, 0, sizeof(m_dirtyVertexBufferChunkBits));
	memset(m_dirtyConstantBufferChunkBits, 0, sizeof(m_dirtyConstantBufferChunkBits));
	memset(m_dirtyExtendedUserDataChunkBits, 0, sizeof(m_dirtyExtendedUserDataChunkBits));
	memset(m_dirtyDispatchDrawDataChunkBits, 0, sizeof(m_dirtyDispatchDrawDataChunkBits));

	memset(m_dirtyResourceSlotBits, 0, sizeof(m_dirtyResourceSlotBits));
	memset(m_dirtyRwResourceSlotBits, 0, sizeof(m_dirtyRwResourceSlotBits));
	memset(m_dirtySamplerSlotBits, 0, sizeof(m_dirtySamplerSlotBits));
	memset(m_dirtyVertexBufferSlotBits, 0, sizeof(m_dirtyVertexBufferSlotBits));
	memset(m_dirtyConstantBufferSlotBits, 0, sizeof(m_dirtyConstantBufferSlotBits));
	memset(m_dirtyExtendedUserDataSlotBits, 0, sizeof(m_dirtyExtendedUserDataSlotBits));
	memset(m_dirtyDispatchDrawDataSlotBits, 0, sizeof(m_dirtyDispatchDrawDataSlotBits));

	memset(m_internalSrtBuffers, 0, sizeof(m_internalSrtBuffers));
	memset(m_userSrtData, 0, sizeof(m_userSrtData));
	memset(m_userSrtDataSizeInDwords, 0, sizeof(m_userSrtDataSizeInDwords));

	for(uint32_t iStage=0; iStage<kShaderStageCount; ++iStage)
	{
		m_dirtyEudSlotRanges[iStage].m_firstSlot = 0xFF;
		m_dirtyEudSlotRanges[iStage].m_lastSlotPlusOne = 0;

		m_activeResourceSlotCounts[        iStage] = Gnm::kSlotCountResource;
		m_activeRwResourceSlotCounts[      iStage] = Gnm::kSlotCountRwResource;
		m_activeSamplerSlotCounts[         iStage] = Gnm::kSlotCountSampler;
		m_activeVertexBufferSlotCounts[    iStage] = Gnm::kSlotCountVertexBuffer;
		m_activeConstantBufferSlotCounts[  iStage] = Gnm::kSlotCountConstantBuffer;
	}
	m_dirtyDddDwordRanges.m_firstDword = 0xFF;
	m_dirtyDddDwordRanges.m_lastDwordPlusOne = 0;

	m_activeStreamoutBufferSlotCount               = Gnm::kSlotCountStreamoutBuffer;

	memset(m_dirtyShaders, 0, sizeof(m_dirtyShaders));
	memset(m_isSrtShader, 0, sizeof(m_isSrtShader));

	m_chunksPerRingBuffer	   = numRingEntries;
	m_betweenPreAndPostDraw	   = false;
	m_currentVSFetchShaderAddr = 0;
	m_currentLSFetchShaderAddr = 0;
	m_currentESFetchShaderAddr = 0;
	m_currentAcbCSFetchShaderAddr = 0;
	m_currentVsShaderModifier  = 0;
	m_currentLsShaderModifier  = 0;
	m_currentEsShaderModifier  = 0;
	m_currentAcbCsShaderModifier = 0;
	m_currentVSB			   = NULL;
	m_currentPSB			   = NULL;
	m_currentLSB			   = NULL;
	m_currentHSB			   = NULL;
	m_currentESB			   = NULL;
	m_currentGSB			   = NULL;
	m_currentAcbCSB            = NULL;
	m_globalTableAddr		   = 0;
	m_anyWrapped			   = true;
	m_psInputs                 = 0;
	m_activeShaderStages       = kActiveShaderStagesVsPs;
	m_currentTessRegs.m_vgtLsHsConfig = 0; // TODO: safe default initialization?

	// Divvy up the heap between the ring buffers.
	// This is a duplicate of the body of ConstantUpdateEngine::computeHeapSize(). We don't
	// just call computeHeapSize() here because init() needs the intermediate k*RingBufferBytes values.
	const uint32_t kResourceRingBufferBytes	        = CueRingBuffer::computeSpaceRequirements(Gnm::kChunkSlotCountResource        *Gnm::kDwordSizeResource,         numRingEntries);
	const uint32_t kRwResourceRingBufferBytes	    = CueRingBuffer::computeSpaceRequirements(Gnm::kChunkSlotCountRwResource      *Gnm::kDwordSizeRwResource,       numRingEntries);
	const uint32_t kSamplerRingBufferBytes		    = CueRingBuffer::computeSpaceRequirements(Gnm::kChunkSlotCountSampler         *Gnm::kDwordSizeSampler,          numRingEntries);
	const uint32_t kVertexBufferRingBufferBytes     = CueRingBuffer::computeSpaceRequirements(Gnm::kChunkSlotCountVertexBuffer    *Gnm::kDwordSizeVertexBuffer,     numRingEntries);
	const uint32_t kConstantBufferRingBufferBytes   = CueRingBuffer::computeSpaceRequirements(Gnm::kChunkSlotCountConstantBuffer  *Gnm::kDwordSizeConstantBuffer,   numRingEntries);
	const uint32_t kStreamoutBufferRingBufferBytes  = CueRingBuffer::computeSpaceRequirements(Gnm::kChunkSlotCountStreamoutBuffer *Gnm::kDwordSizeStreamoutBuffer,  numRingEntries);
	const uint32_t kExtendedUserDataRingBufferBytes = CueRingBuffer::computeSpaceRequirements(Gnm::kChunkSlotCountExtendedUserData*Gnm::kDwordSizeExtendedUserData, numRingEntries);
	const uint32_t kDispatchDrawDataRingBufferBytes = CueRingBuffer::computeSpaceRequirements(Gnm::kChunkSlotCountDispatchDrawData*Gnm::kDwordSizeDispatchDrawData, numRingEntries);

	m_heapSize = computeHeapSize(numRingEntries);
	SCE_GNM_VALIDATE(kPerStageDwordSize*sizeof(uint32_t)*Gnm::kShaderStageCount<= computeCpRamShadowSize(), "internal check failed: CPRAM shadow buffer won't fit in CPRAM!"); // total size of all stages' resources must fit in the available CPRAM!

	// Initialize ring buffers
	void *freeAddr = heapAddr;
	for(int iStage=0; iStage<Gnm::kShaderStageCount; ++iStage)
	{
		int iRB = 0;
		// Initialize resource ring buffers
		for(int iTRB=0; iTRB<kChunkCountResource; ++iTRB)
		{
			SCE_GNM_VALIDATE(uintptr_t(freeAddr) <= uintptr_t(heapAddr)+m_heapSize, "freeAddr (0x%10llX) outside of heap bounds [0x%10llX-0x%10llX].", freeAddr, heapAddr, uintptr_t(heapAddr)+m_heapSize-1);
			m_ringBuffers[iStage][kRingBuffersIndexResource+iTRB].init(freeAddr, kResourceRingBufferBytes, Gnm::kChunkSlotCountResource*Gnm::kDwordSizeResource, m_chunksPerRingBuffer);
			freeAddr = (uint8_t*)freeAddr + kResourceRingBufferBytes;
			++iRB;
		}
		// Initialize read/write resource ring buffers
		for(int iURB=0; iURB<kChunkCountRwResource; ++iURB)
		{
			SCE_GNM_VALIDATE(uintptr_t(freeAddr) <= uintptr_t(heapAddr)+m_heapSize, "freeAddr (0x%10llX) outside of heap bounds [0x%10llX-0x%10llX].", freeAddr, heapAddr, uintptr_t(heapAddr)+m_heapSize-1);
			m_ringBuffers[iStage][kRingBuffersIndexRwResource+iURB].init(freeAddr, kRwResourceRingBufferBytes, Gnm::kChunkSlotCountRwResource*Gnm::kDwordSizeRwResource, m_chunksPerRingBuffer);
			freeAddr = (uint8_t*)freeAddr + kRwResourceRingBufferBytes;
			++iRB;
		}
		// Initialize sampler ring buffers
		for(int iSRB=0; iSRB<kChunkCountSampler; ++iSRB)
		{
			SCE_GNM_VALIDATE(uintptr_t(freeAddr) <= uintptr_t(heapAddr)+m_heapSize, "freeAddr (0x%10llX) outside of heap bounds [0x%10llX-0x%10llX].", freeAddr, heapAddr, uintptr_t(heapAddr)+m_heapSize-1);
			m_ringBuffers[iStage][kRingBuffersIndexSampler+iSRB].init(freeAddr, kSamplerRingBufferBytes, Gnm::kChunkSlotCountSampler*Gnm::kDwordSizeSampler, m_chunksPerRingBuffer);
			freeAddr = (uint8_t*)freeAddr + kSamplerRingBufferBytes;
			++iRB;
		}
		// Initialize Vertex Buffer ring buffers
		for(int iVRB=0; iVRB<kChunkCountVertexBuffer; ++iVRB)
		{
			SCE_GNM_VALIDATE(uintptr_t(freeAddr) <= uintptr_t(heapAddr)+m_heapSize, "freeAddr (0x%10llX) outside of heap bounds [0x%10llX-0x%10llX].", freeAddr, heapAddr, uintptr_t(heapAddr)+m_heapSize-1);
			m_ringBuffers[iStage][kRingBuffersIndexVertexBuffer+iVRB].init(freeAddr, kVertexBufferRingBufferBytes, Gnm::kChunkSlotCountVertexBuffer*Gnm::kDwordSizeVertexBuffer, m_chunksPerRingBuffer);
			freeAddr = (uint8_t*)freeAddr + kVertexBufferRingBufferBytes;
			++iRB;
		}
		// Initialize Constant Buffer ring buffers
		for(int iCRB=0; iCRB<kChunkCountConstantBuffer; ++iCRB)
		{
			SCE_GNM_VALIDATE(uintptr_t(freeAddr) <= uintptr_t(heapAddr)+m_heapSize, "freeAddr (0x%10llX) outside of heap bounds [0x%10llX-0x%10llX].", freeAddr, heapAddr, uintptr_t(heapAddr)+m_heapSize-1);
			m_ringBuffers[iStage][kRingBuffersIndexConstantBuffer+iCRB].init(freeAddr, kConstantBufferRingBufferBytes, Gnm::kChunkSlotCountConstantBuffer*Gnm::kDwordSizeConstantBuffer, m_chunksPerRingBuffer);
			freeAddr = (uint8_t*)freeAddr + kConstantBufferRingBufferBytes;
			++iRB;
		}
		for( int iSOB=0; iSOB<kChunkCountStreamoutBuffer; ++iSOB)
		{
			SCE_GNM_VALIDATE(uintptr_t(freeAddr) <= uintptr_t(heapAddr)+m_heapSize, "freeAddr (0x%10llX) outside of heap bounds [0x%10llX-0x%10llX].", freeAddr, heapAddr, uintptr_t(heapAddr)+m_heapSize-1);
			m_ringBuffers[iStage][kRingBuffersIndexStreamoutBuffer+iSOB].init(freeAddr,kStreamoutBufferRingBufferBytes, Gnm::kChunkSlotCountStreamoutBuffer*Gnm::kDwordSizeStreamoutBuffer,m_chunksPerRingBuffer);
			freeAddr = (uint8_t*)freeAddr + kStreamoutBufferRingBufferBytes;
			++iRB;
		}
		// Initialize Extended User Data ring buffer
		for(int iEUDRB=0; iEUDRB<kChunkCountExtendedUserData; ++iEUDRB)
		{
			SCE_GNM_VALIDATE(uintptr_t(freeAddr) <= uintptr_t(heapAddr)+m_heapSize, "freeAddr (0x%10llX) outside of heap bounds [0x%10llX-0x%10llX].", freeAddr, heapAddr, uintptr_t(heapAddr)+m_heapSize-1);
			m_ringBuffers[iStage][kRingBuffersIndexExtendedUserData+iEUDRB].init(freeAddr, kExtendedUserDataRingBufferBytes, Gnm::kChunkSlotCountExtendedUserData*Gnm::kDwordSizeExtendedUserData, m_chunksPerRingBuffer);
			freeAddr = (uint8_t*)freeAddr + kExtendedUserDataRingBufferBytes;
			++iRB;
		}
		// Initialize Dispatch Draw Data ring buffer
		for(int iDDDRB=0; iDDDRB<kChunkCountDispatchDrawData; ++iDDDRB)
		{
			SCE_GNM_VALIDATE(uintptr_t(freeAddr) <= uintptr_t(heapAddr)+m_heapSize, "freeAddr (0x%10llX) outside of heap bounds [0x%10llX-0x%10llX].", freeAddr, heapAddr, uintptr_t(heapAddr)+m_heapSize-1);
			m_ringBuffers[iStage][kRingBuffersIndexDispatchDrawData+iDDDRB].init(freeAddr, kDispatchDrawDataRingBufferBytes, Gnm::kChunkSlotCountDispatchDrawData*Gnm::kDwordSizeDispatchDrawData, m_chunksPerRingBuffer);
			freeAddr = (uint8_t*)freeAddr + kDispatchDrawDataRingBufferBytes;
			++iRB;
		}
		SCE_GNM_ASSERT(iRB == kNumRingBuffersPerStage); // sanity check
	}
	uint32_t actualUsed = static_cast<uint32_t>(uintptr_t(freeAddr) - uintptr_t(heapAddr));
	SCE_GNM_ASSERT(actualUsed == m_heapSize); // sanity check
	SCE_GNM_UNUSED(actualUsed);

	// Setup the global resource table:
	memset(m_globalTablePtr, 0, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE);
	m_globalTableNeedsUpdate = false;
}


void ConstantUpdateEngine::setGlobalResourceTableAddr(void *addr)
{
	m_globalTableAddr = addr;
}


void ConstantUpdateEngine::setTextures(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Texture *textures)
{
	if (numApiSlots == 0)
		return;
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountResource, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountResource);
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= m_activeResourceSlotCounts[stage], "startApiSlot (%d) + numApiSlots (%d) exceeds the active slot range for this stage [0..%d].", startApiSlot, numApiSlots, m_activeResourceSlotCounts[stage]);
	uint16_t dwordOffset = getResourceDwordOffset(stage, startApiSlot);
	if (textures == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeResource*sizeof(uint32_t));
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{
		memcpy(m_cpRamShadowDwords+dwordOffset, textures, numApiSlots*Gnm::kDwordSizeResource*sizeof(uint32_t));
	}
	// Mark the appropriate slots as dirty
	for(uint32_t iSlot=startApiSlot, iSlotMax=startApiSlot+numApiSlots; iSlot<iSlotMax; ++iSlot)
	{
		SCE_GNM_VALIDATE(textures == NULL || textures[iSlot-startApiSlot].isTexture(), "textures[%d] is invalid (isTexture() returned false).", iSlot-startApiSlot);
		setBit(m_dirtyResourceSlotBits[stage], iSlot);
	}
	// Mark the appropriate chunks as dirty
	uint32_t firstChunk = startApiSlot / Gnm::kChunkSlotCountResource;
	uint32_t lastChunk = (startApiSlot + numApiSlots-1) / Gnm::kChunkSlotCountResource;
	for(uint32_t iChunk=firstChunk; iChunk<=lastChunk; ++iChunk)
	{
		setBit(m_dirtyResourceChunkBits[stage], iChunk);
	}
}

void ConstantUpdateEngine::setBuffers(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Buffer *buffers)
{
	if (numApiSlots == 0)
		return;
	// Buffers are actually treated as texture resources under the hood. They just take up half as much space; the rest is wasted padding.
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountResource, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountResource);
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= m_activeResourceSlotCounts[stage], "startApiSlot (%d) + numApiSlots (%d) exceeds the active slot range for this stage [0..%d].", startApiSlot, numApiSlots, m_activeResourceSlotCounts[stage]);
	uint16_t dwordOffset = getResourceDwordOffset(stage, startApiSlot);
	if (buffers == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeResource*sizeof(uint32_t));
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{
		for(uint32_t iSlot=0; iSlot<numApiSlots; ++iSlot)
		{
			SCE_GNM_VALIDATE(buffers[iSlot].isBuffer(), "buffers[%d] is invalid (isBuffer() returned false).", iSlot-startApiSlot);
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeResource+0] = buffers[iSlot].m_regs[0];
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeResource+1] = buffers[iSlot].m_regs[1];
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeResource+2] = buffers[iSlot].m_regs[2];
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeResource+3] = buffers[iSlot].m_regs[3];
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeResource+4] = 0;
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeResource+5] = 0;
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeResource+6] = 0;
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeResource+7] = 0;
		}
	}
	// Mark the appropriate slots as dirty
	for(uint32_t iSlot=startApiSlot, iSlotMax=startApiSlot+numApiSlots; iSlot<iSlotMax; ++iSlot)
	{
		setBit(m_dirtyResourceSlotBits[stage], iSlot);
	}
	// Mark the appropriate chunks as dirty
	uint32_t firstChunk = startApiSlot / Gnm::kChunkSlotCountResource;
	uint32_t lastChunk = (startApiSlot + numApiSlots-1) / Gnm::kChunkSlotCountResource;
	for(uint32_t iChunk=firstChunk; iChunk<=lastChunk; ++iChunk)
	{
		setBit(m_dirtyResourceChunkBits[stage], iChunk);
	}
}

void ConstantUpdateEngine::setRwTextures(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Texture *rwTextures)
{
	if (numApiSlots == 0)
		return;
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountRwResource, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountRwResource);
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= m_activeRwResourceSlotCounts[stage], "startApiSlot (%d) + numApiSlots (%d) exceeds the active slot range for this stage [0..%d].", startApiSlot, numApiSlots, m_activeRwResourceSlotCounts[stage]);
	uint16_t dwordOffset = getRwResourceDwordOffset(stage, startApiSlot);
	if (rwTextures == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeRwResource*sizeof(uint32_t));
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{
		memcpy(m_cpRamShadowDwords+dwordOffset, rwTextures, numApiSlots*Gnm::kDwordSizeRwResource*sizeof(uint32_t));
	}
	// Mark the appropriate slots as dirty
	for(uint32_t iSlot=startApiSlot, iSlotMax=startApiSlot+numApiSlots; iSlot<iSlotMax; ++iSlot)
	{
		SCE_GNM_VALIDATE(rwTextures == NULL || rwTextures[iSlot-startApiSlot].isTexture(), "rwTextures[%d] is invalid (isTexture() returned false).", iSlot-startApiSlot);
		setBit(m_dirtyRwResourceSlotBits[stage], iSlot);
	}
	// Mark the appropriate chunks as dirty
	uint32_t firstChunk = startApiSlot / Gnm::kChunkSlotCountRwResource;
	uint32_t lastChunk = (startApiSlot + numApiSlots-1) / Gnm::kChunkSlotCountRwResource;
	for(uint32_t iChunk=firstChunk; iChunk<=lastChunk; ++iChunk)
	{
		setBit(m_dirtyRwResourceChunkBits[stage], iChunk);
	}
}

void ConstantUpdateEngine::setRwBuffers(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Buffer *rwBuffers)
{
	if (numApiSlots == 0)
		return;
	// Buffers are actually treated as texture resources under the hood. They just take up half as much space; the rest is wasted padding.
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountRwResource, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountRwResource);
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= m_activeRwResourceSlotCounts[stage], "startApiSlot (%d) + numApiSlots (%d) exceeds the active slot range for this stage [0..%d].", startApiSlot, numApiSlots, m_activeRwResourceSlotCounts[stage]);
	uint16_t dwordOffset = getRwResourceDwordOffset(stage, startApiSlot);
	if (rwBuffers == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeRwResource*sizeof(uint32_t));
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{
		for(uint32_t iSlot=0; iSlot<numApiSlots; ++iSlot)
		{
			SCE_GNM_VALIDATE(rwBuffers[iSlot].isBuffer(), "rwBuffers[%d] is invalid (isBuffer() returned false).", iSlot-startApiSlot);
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeRwResource+0] = rwBuffers[iSlot].m_regs[0];
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeRwResource+1] = rwBuffers[iSlot].m_regs[1];
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeRwResource+2] = rwBuffers[iSlot].m_regs[2];
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeRwResource+3] = rwBuffers[iSlot].m_regs[3];
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeRwResource+4] = 0;
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeRwResource+5] = 0;
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeRwResource+6] = 0;
			m_cpRamShadowDwords[dwordOffset+iSlot*Gnm::kDwordSizeRwResource+7] = 0;
		}
	}
	// Mark the appropriate slots as dirty
	for(uint32_t iSlot=startApiSlot, iSlotMax=startApiSlot+numApiSlots; iSlot<iSlotMax; ++iSlot)
	{
		setBit(m_dirtyRwResourceSlotBits[stage], iSlot);
	}
	// Mark the appropriate chunks as dirty
	uint32_t firstChunk = startApiSlot / Gnm::kChunkSlotCountRwResource;
	uint32_t lastChunk = (startApiSlot + numApiSlots-1) / Gnm::kChunkSlotCountRwResource;
	for(uint32_t iChunk=firstChunk; iChunk<=lastChunk; ++iChunk)
	{
		setBit(m_dirtyRwResourceChunkBits[stage], iChunk);
	}
}

void ConstantUpdateEngine::setSamplers(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Sampler *samplers)
{
	if (numApiSlots == 0)
		return;
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountSampler, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountSampler);
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= m_activeSamplerSlotCounts[stage], "startApiSlot (%d) + numApiSlots (%d) exceeds the active slot range for this stage [0..%d].", startApiSlot, numApiSlots, m_activeSamplerSlotCounts[stage]);
	uint16_t dwordOffset = getSamplerDwordOffset(stage, startApiSlot);
	if (samplers == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeSampler*sizeof(uint32_t));
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{
		memcpy(m_cpRamShadowDwords+dwordOffset, samplers, numApiSlots*Gnm::kDwordSizeSampler*sizeof(uint32_t));
	}
	// Mark the appropriate slots as dirty
	for(uint32_t iSlot=startApiSlot, iSlotMax=startApiSlot+numApiSlots; iSlot<iSlotMax; ++iSlot)
	{
		setBit(m_dirtySamplerSlotBits[stage], iSlot);
	}
	// Mark the appropriate chunks as dirty
	uint32_t firstChunk = startApiSlot / Gnm::kChunkSlotCountSampler;
	uint32_t lastChunk = (startApiSlot + numApiSlots-1) / Gnm::kChunkSlotCountSampler;
	for(uint32_t iChunk=firstChunk; iChunk<=lastChunk; ++iChunk)
	{
		setBit(m_dirtySamplerChunkBits[stage], iChunk);
	}
}

void ConstantUpdateEngine::setConstantBuffers(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Buffer *buffers)
{
	if (numApiSlots == 0)
		return;
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountConstantBuffer, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountConstantBuffer);
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= m_activeConstantBufferSlotCounts[stage], "startApiSlot (%d) + numApiSlots (%d) exceeds the active slot range for this stage [0..%d].", startApiSlot, numApiSlots, m_activeConstantBufferSlotCounts[stage]);
	uint16_t dwordOffset = getConstantBufferDwordOffset(stage, startApiSlot);
	if (buffers == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeConstantBuffer*sizeof(uint32_t));
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{
		memcpy(m_cpRamShadowDwords+dwordOffset, buffers, numApiSlots*Gnm::kDwordSizeConstantBuffer*sizeof(uint32_t));
	}

	// Mark the appropriate slots as dirty
	for(uint32_t iSlot=startApiSlot, iSlotMax=startApiSlot+numApiSlots; iSlot<iSlotMax; ++iSlot)
	{
		SCE_GNM_VALIDATE(buffers[iSlot-startApiSlot].isBuffer(), "buffers[%d] is invalid (isBuffer() returned false).", iSlot-startApiSlot);
		setBit(m_dirtyConstantBufferSlotBits[stage], iSlot);
	}
	// Mark the appropriate chunks as dirty
	uint32_t firstChunk = startApiSlot / Gnm::kChunkSlotCountConstantBuffer;
	uint32_t lastChunk = (startApiSlot + numApiSlots-1) / Gnm::kChunkSlotCountConstantBuffer;
	for(uint32_t iChunk=firstChunk; iChunk<=lastChunk; ++iChunk)
	{
		setBit(m_dirtyConstantBufferChunkBits[stage], iChunk);
	}
}

void ConstantUpdateEngine::setVertexBuffers(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const Buffer *buffers)
{
	if (numApiSlots == 0)
		return;
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountVertexBuffer, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountVertexBuffer);
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= m_activeVertexBufferSlotCounts[stage], "startApiSlot (%d) + numApiSlots (%d) exceeds the active slot range for this stage [0..%d].", startApiSlot, numApiSlots, m_activeVertexBufferSlotCounts[stage]);
	uint16_t dwordOffset = getVertexBufferDwordOffset(stage, startApiSlot);
	if (buffers == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeVertexBuffer*sizeof(uint32_t));
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{	
		memcpy(m_cpRamShadowDwords+dwordOffset, buffers, numApiSlots*Gnm::kDwordSizeVertexBuffer*sizeof(uint32_t));
	}
	// Mark the appropriate slots as dirty
	for(uint32_t iSlot=startApiSlot, iSlotMax=startApiSlot+numApiSlots; iSlot<iSlotMax; ++iSlot)
	{
		SCE_GNM_VALIDATE(buffers[iSlot-startApiSlot].isBuffer(), "buffers[%d] is invalid (isBuffer() returned false).", iSlot-startApiSlot);
		setBit(m_dirtyVertexBufferSlotBits[stage], iSlot);
	}
	// Mark the appropriate chunks as dirty
	uint32_t firstChunk = startApiSlot / Gnm::kChunkSlotCountVertexBuffer;
	uint32_t lastChunk = (startApiSlot + numApiSlots-1) / Gnm::kChunkSlotCountVertexBuffer;
	for(uint32_t iChunk=firstChunk; iChunk<=lastChunk; ++iChunk)
	{
		setBit(m_dirtyVertexBufferChunkBits[stage], iChunk);
	}
}

void ConstantUpdateEngine::setStreamoutBuffers(uint32_t startApiSlot, uint32_t numApiSlots, const Buffer *buffers)
{
	const ShaderStage stage = Gnm::kShaderStageVs;
	if (numApiSlots == 0)
		return;
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountStreamoutBuffer, "startSlot (%d) + numSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountStreamoutBuffer);
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= m_activeStreamoutBufferSlotCount, "startApiSlot (%d) + numApiSlots (%d) exceeds the active slot range [0..%d].", startApiSlot, numApiSlots, m_activeStreamoutBufferSlotCount);
	uint16_t dwordOffset = getStreamoutDwordOffset(stage, startApiSlot);
	if (buffers == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeStreamoutBuffer*sizeof(uint32_t));
#else
		return; // early-out if we're not writing zeros
#endif
	}
	else
	{	
		memcpy(m_cpRamShadowDwords+dwordOffset, buffers, numApiSlots*Gnm::kDwordSizeStreamoutBuffer*sizeof(uint32_t));
	}
    
	// Mark the appropriate slots as dirty
	for(uint32_t iSlot=startApiSlot, iSlotMax=startApiSlot+numApiSlots; iSlot<iSlotMax; ++iSlot)
	{
		SCE_GNM_VALIDATE(buffers[iSlot-startApiSlot].isBuffer(), "buffers[%d] is invalid (isBuffer() returned false).", iSlot-startApiSlot);
		setBit(m_dirtyStreamoutBufferSlotBits[stage], iSlot);
	}
	// Mark the appropriate chunks as dirty
	uint32_t firstChunk = startApiSlot / Gnm::kChunkSlotCountStreamoutBuffer;
	uint32_t lastChunk = (startApiSlot + numApiSlots-1) / Gnm::kChunkSlotCountStreamoutBuffer;
	for(uint32_t iChunk=firstChunk; iChunk<=lastChunk; ++iChunk)
	{
		setBit(m_dirtyStreamoutBufferChunkBits[stage], iChunk);
	}
}

void ConstantUpdateEngine::setBoolConstants(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const uint32_t *bits)
{
	if (numApiSlots == 0)
		return;
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountBoolConstant, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountBoolConstant);
	uint16_t dwordOffset = getBoolConstantDwordOffset(stage, startApiSlot);
	if (bits == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeBoolConstant);
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{	
		for(uint32_t iSlot=0; iSlot<numApiSlots; ++iSlot)
			m_cpRamShadowDwords[dwordOffset+iSlot] = bits[iSlot];
	}
}

void ConstantUpdateEngine::setFloatConstants(ShaderStage stage, uint32_t startApiSlot, uint32_t numApiSlots, const float *floats)
{
	if (numApiSlots == 0)
		return;
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(startApiSlot+numApiSlots <= Gnm::kSlotCountFloatConstant, "startApiSlot (%d) + numApiSlots (%d) must be in the range [0..%d].", startApiSlot, numApiSlots, Gnm::kSlotCountFloatConstant);
	uint16_t dwordOffset = getFloatConstantDwordOffset(stage, startApiSlot);
	if (floats == NULL)
	{
#if SET_NULL_RESOURCE_WRITES_ZERO_TO_CPRAM
		memset(m_cpRamShadowDwords+dwordOffset, 0, numApiSlots*Gnm::kDwordSizeFloatConstant);
#else
		return; // early-out if we're not writing zeroes
#endif
	}
	else
	{	
		const uint32_t *floatsAsDwords = (const uint32_t*)floats;
		for(uint32_t iSlot=0; iSlot<numApiSlots; ++iSlot)
			m_cpRamShadowDwords[dwordOffset+iSlot] = floatsAsDwords[iSlot];
	}
}

void ConstantUpdateEngine::setAppendConsumeCounterRange(ShaderStage stage, uint32_t baseOffsetInBytes, uint32_t rangeSizeInBytes)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
	SCE_GNM_VALIDATE(((baseOffsetInBytes | rangeSizeInBytes)&3) == 0, "baseOffsetInBytes (%d) as well as baseOffsetInBytes (%d) must be multiple of 4s.", baseOffsetInBytes, rangeSizeInBytes);
	SCE_GNM_VALIDATE((baseOffsetInBytes+rangeSizeInBytes) < kGdsAccessibleMemorySizeInBytes, "baseOffsetInBytes (%d) + rangeSizeInBytes (%d) must be less than %i.", baseOffsetInBytes, rangeSizeInBytes, kGdsAccessibleMemorySizeInBytes);

	uint16_t dwordOffset = getAppendConsumeCounterRangeDwordOffset(stage, 0);
	const uint32_t ringBasedOffset = baseOffsetInBytes;
	m_cpRamShadowDwords[dwordOffset] = (ringBasedOffset << 16) | rangeSizeInBytes;
}

void ConstantUpdateEngine::setVsShader(const VsShader *vsb, uint32_t shaderModifier, void *fetchShaderAddr)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
#ifdef GNM_DEBUG
	if (vsb != NULL)
	{
		// Confirm that a fetch shader was passed if the VS is expecting one
		bool vsExpectsFS = false;
		const Gnm::InputUsageSlot *inputUsageSlots = vsb->getInputUsageSlotTable();
		for(uint32_t iUsage=0, iUsageMax=vsb->m_common.m_numInputUsageSlots; iUsage<iUsageMax; ++iUsage)
		{
			if (inputUsageSlots[iUsage].m_usageType == Gnm::kShaderInputUsageSubPtrFetchShader)
			{
				vsExpectsFS = true;
				break;
			}
		}
		if ( vsExpectsFS && fetchShaderAddr == 0 )
		{
			SCE_GNM_ERROR("VsShader vsb [0x%010llX] expects a fetch shader, but fetchShaderAddr==0.", vsb);
			return;
		}
	}
#endif
	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	m_dirtyShaders[kShaderStageVs] = true;
	m_currentVSB = vsb;
	m_currentVSFetchShaderAddr = fetchShaderAddr;
	m_currentVsShaderModifier = shaderModifier;
	m_psInputs = 0; // new shader -> needs new psInput table
	if (vsb == NULL)
		return;
	m_isSrtShader[kShaderStageVs] = (vsb->m_common.m_shaderIsUsingSrt != 0);

	if (vsb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Set up the internal constants:
		Gnm::Buffer vsEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)vsb->m_vsStageRegisters.m_spiShaderPgmHiVs << 32) + vsb->m_vsStageRegisters.m_spiShaderPgmLoVs );
		vsEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + vsb->m_common.m_shaderSize ),
												   vsb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		setConstantBuffers(kShaderStageVs, 15, 1, &vsEmbeddedConstBuffer);
	}
}

void ConstantUpdateEngine::setCsVsShaders(const CsVsShader *csvsb, uint32_t shaderModifierVs, void *fetchShaderAddrVs, uint32_t shaderModifierCs, void *fetchShaderAddrCs)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw(), preDispatch() and postDispatch(), or preDispatchDraw() and postDispatchDraw().");
	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	if (csvsb == NULL) {
		setAsynchronousComputeShader(NULL, 0, NULL);
		setVsShader(NULL, 0, NULL);
		return;
	}
	setAsynchronousComputeShader(csvsb->getComputeShader(), shaderModifierCs, fetchShaderAddrCs);
	setVsShader(csvsb->getVertexShader(), shaderModifierVs, fetchShaderAddrVs);

	SCE_GNM_VALIDATE(m_currentAcbCSB->m_common.m_embeddedConstantBufferSizeInDQW == 0 || m_currentAcbCSB->m_common.m_embeddedConstantBufferSizeInDQW == csvsb->getVertexShader()->m_common.m_embeddedConstantBufferSizeInDQW, "CsVsShader csvsb [0x%010llX] asynchronous compute shader embedded constant buffer does not match VS shader embedded constant buffer.", csvsb);
}

void ConstantUpdateEngine::setAsynchronousComputeShader(const CsShader *csb, uint32_t shaderModifierCs, void *fetchShaderAddrCs)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw(), preDispatch() and postDispatch(), or preDispatchDraw() and postDispatchDraw().");
#ifdef GNM_DEBUG
	if (csb != NULL)
	{
		// Confirm that a fetch shader was passed if the ACB CS shader is expecting one.
		// Also confirm that the ACB CS shader is compatible with the VS shader - all CS input usage slots which will reside in CUE ring buffers must match a VS input usage slot.
		bool csExpectsFS = false;
		const Gnm::InputUsageSlot *inputUsageSlotsCs = csb->getInputUsageSlotTable();
		for(uint32_t iUsage=0, iUsageMax=csb->m_common.m_numInputUsageSlots; iUsage<iUsageMax; ++iUsage)
		{
			if (inputUsageSlotsCs[iUsage].m_usageType == Gnm::kShaderInputUsageSubPtrFetchShader) {
				csExpectsFS = true;
				break;
			}
		}
		if ( csExpectsFS && fetchShaderAddrCs == 0 )
		{
			SCE_GNM_ERROR("AsynchronousComputeShader csb [0x%010llX] shader expects a fetch shader, but fetchShaderAddrCs==0.", csb);
			return;
		}
	}
#endif
	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	if (csb == NULL) {
		m_dirtyShaders[kShaderStageAcbCs] = true;
		m_currentAcbCSB = NULL;
		m_currentAcbCSFetchShaderAddr = NULL;
		m_currentAcbCsShaderModifier = 0;
		return;
	}
	m_dirtyShaders[kShaderStageAcbCs] = true;
	m_currentAcbCSB = csb;
	m_currentAcbCSFetchShaderAddr = fetchShaderAddrCs;
	m_currentAcbCsShaderModifier = shaderModifierCs;
	m_isSrtShader[kShaderStageAcbCs] = (m_currentAcbCSB->m_common.m_shaderIsUsingSrt != 0);

//	SCE_GNM_VALIDATE(m_currentAcbCSB->m_common.m_embeddedConstantBufferSizeInDQW == 0 || m_currentAcbCSB->m_common.m_embeddedConstantBufferSizeInDQW == csvsb->getVertexShader()->m_common.m_embeddedConstantBufferSizeInDQW, "AsynchronousComputeShader csb [0x%010llX] shader embedded constant buffer does not match VS shader embedded constant buffer.", csb);
	// 	if (m_currentAcbCSB->m_common.m_embeddedConstantBufferSizeInDQW)
	// 	{
	// 		// Setup the internal constants
	// 		Gnm::Buffer csEmbeddedConstBuffer;
	// 		void *shaderAddr = (void*)( ((uintptr_t)m_currentAcbCSB->m_csStageRegisters.m_computePgmHi << 32) + m_currentAcbCSB->m_csStageRegisters.m_computePgmLo );
	// 		csEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + m_currentAcbCSB->m_common.m_shaderSize ),
	// 			m_currentAcbCSB->m_common.m_embeddedConstantBufferSizeInDQW*16);
	// 
	// 		// The embedded constant is always set to slot 15, by convention
	// 		setConstantBuffers(kShaderStageCs, 15, 1, &csEmbeddedConstBuffer);
	// 	}
}

void ConstantUpdateEngine::setPsShader(const PsShader *psb)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");

	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	m_dirtyShaders[kShaderStagePs] = true;
	m_currentPSB = psb;
	m_psInputs = 0; // new shader -> needs new psInput table
	if (psb == NULL)
		return;
	m_isSrtShader[kShaderStagePs] = (psb->m_common.m_shaderIsUsingSrt != 0);

	if (psb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Setup the internal constants
		Gnm::Buffer psEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)psb->m_psStageRegisters.m_spiShaderPgmHiPs << 32) + psb->m_psStageRegisters.m_spiShaderPgmLoPs );
		psEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + psb->m_common.m_shaderSize ),
												   psb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		setConstantBuffers(kShaderStagePs, 15, 1, &psEmbeddedConstBuffer);
	}
}

void ConstantUpdateEngine::setCsShader(const CsShader *csb)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");

	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	m_dirtyShaders[kShaderStageCs] = true;
	m_currentCSB = csb;
	if (csb == NULL)
		return;
	m_isSrtShader[kShaderStageCs] = (csb->m_common.m_shaderIsUsingSrt != 0);

	if (csb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Setup the internal constants
		Gnm::Buffer csEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)csb->m_csStageRegisters.m_computePgmHi << 32) + csb->m_csStageRegisters.m_computePgmLo );
		csEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + csb->m_common.m_shaderSize ),
												   csb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		setConstantBuffers(kShaderStageCs, 15, 1, &csEmbeddedConstBuffer);
	}
}

void ConstantUpdateEngine::setHsShader(const HsShader *hsb, const TessellationRegisters *tessRegs)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");

	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	m_dirtyShaders[Gnm::kShaderStageHs] = true;
	m_currentHSB = hsb;
	if (hsb == NULL)
		return;
	m_isSrtShader[kShaderStageHs] = (hsb->m_common.m_shaderIsUsingSrt != 0);
	m_currentTessRegs = *tessRegs;

	if (hsb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Setup the internal constants
		Gnm::Buffer hsEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)hsb->m_hsStageRegisters.m_spiShaderPgmHiHs << 32) + hsb->m_hsStageRegisters.m_spiShaderPgmLoHs );
		hsEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + hsb->m_common.m_shaderSize ),
												   hsb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		setConstantBuffers(Gnm::kShaderStageHs, 15, 1, &hsEmbeddedConstBuffer);
	}
}

void ConstantUpdateEngine::setLsShader(const LsShader *lsb, uint32_t shaderModifier, void *fetchShaderAddr)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");

#ifdef GNM_DEBUG
	if (lsb != NULL)
	{
		// Confirm that a fetch shader was passed if the LS is expecting one
		bool lsExpectsFS = false;
		const Gnm::InputUsageSlot *inputUsageSlots = lsb->getInputUsageSlotTable();
		for(uint32_t iUsage=0, iUsageMax=lsb->m_common.m_numInputUsageSlots; iUsage<iUsageMax; ++iUsage)
		{
			if (inputUsageSlots[iUsage].m_usageType == Gnm::kShaderInputUsageSubPtrFetchShader)
			{
				lsExpectsFS = true;
				break;
			}
		}
		if ( lsExpectsFS && fetchShaderAddr == 0 )
		{
			SCE_GNM_ERROR("LsShader lsb [0x%010llX] expects a fetch shader, but fetchShaderAddr=0.", lsb);
			return;
		}
	}
#endif

	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	m_dirtyShaders[Gnm::kShaderStageLs] = true;
	m_currentLSB = lsb;
	m_currentLSFetchShaderAddr = fetchShaderAddr;
	m_currentLsShaderModifier = shaderModifier;
	if (lsb == NULL)
		return;
	m_isSrtShader[kShaderStageLs] = (lsb->m_common.m_shaderIsUsingSrt != 0);

	if (lsb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Setup the internal constants:
		Gnm::Buffer lsEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)lsb->m_lsStageRegisters.m_spiShaderPgmHiLs << 32) + lsb->m_lsStageRegisters.m_spiShaderPgmLoLs );
		lsEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + lsb->m_common.m_shaderSize ),
			lsb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		setConstantBuffers(Gnm::kShaderStageLs, 15, 1, &lsEmbeddedConstBuffer);
	}
}

void ConstantUpdateEngine::setEsShader(const EsShader *esb, uint32_t shaderModifier, void *fetchShaderAddr)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");

#ifdef GNM_DEBUG
	if (esb != NULL)
	{
		// Confirm that a fetch shader was passed if the ES is expecting one
		bool esExpectsFS = false;
		const Gnm::InputUsageSlot *inputUsageSlots = esb->getInputUsageSlotTable();
		for(uint32_t iUsage=0, iUsageMax=esb->m_common.m_numInputUsageSlots; iUsage<iUsageMax; ++iUsage)
		{
			if (inputUsageSlots[iUsage].m_usageType == Gnm::kShaderInputUsageSubPtrFetchShader)
			{
				esExpectsFS = true;
				break;
			}
		}
		if ( esExpectsFS && fetchShaderAddr == 0 )
		{
			SCE_GNM_ERROR("EsShader esb [0x%010llX] expects a fetch shader, but fetchShaderAddr=0.", esb);
			return;
		}
	}
#endif

	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	m_dirtyShaders[kShaderStageEs] = true;
	m_currentESB = esb;
	m_currentESFetchShaderAddr = fetchShaderAddr;
	m_currentEsShaderModifier = shaderModifier;
	if (esb == NULL)
		return;
	m_isSrtShader[kShaderStageEs] = (esb->m_common.m_shaderIsUsingSrt != 0);

	if (esb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Setup the internal constants:
		Gnm::Buffer esEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)esb->m_esStageRegisters.m_spiShaderPgmHiEs << 32) + esb->m_esStageRegisters.m_spiShaderPgmLoEs );
		esEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + esb->m_common.m_shaderSize ),
												   esb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		setConstantBuffers(kShaderStageEs, 15, 1, &esEmbeddedConstBuffer);
	}
}

void ConstantUpdateEngine::setGsVsShaders(const GsShader *gsb)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");

	// It is not safe to compare the shader pointers to the previously cached values and early-out if they match.
	// The shader object itself may have changed, and a deeper comparison is impractical.

	// Cache the shader binary for the preDraw()
	m_dirtyShaders[Gnm::kShaderStageGs] = true;
	m_currentGSB = gsb;
	if (gsb == NULL)
	{
		setVsShader(NULL, 0, (void*)0); // Setting GS also sets the VS
		return;
	}
	m_isSrtShader[kShaderStageGs] = (gsb->m_common.m_shaderIsUsingSrt != 0);
	if (gsb->m_common.m_embeddedConstantBufferSizeInDQW)
	{
		// Setup the internal constants
		Gnm::Buffer gsEmbeddedConstBuffer;
		void *shaderAddr = (void*)( ((uintptr_t)gsb->m_gsStageRegisters.m_spiShaderPgmHiGs << 32) + gsb->m_gsStageRegisters.m_spiShaderPgmLoGs );
		gsEmbeddedConstBuffer.initAsConstantBuffer((void*)( (uintptr_t(shaderAddr)<<8) + gsb->m_common.m_shaderSize ),
												   gsb->m_common.m_embeddedConstantBufferSizeInDQW*16);

		// The embedded constant is always set to slot 15, by convention
		setConstantBuffers(Gnm::kShaderStageGs, 15, 1, &gsEmbeddedConstBuffer);
	}

	setVsShader(gsb->getCopyShader(), 0, (void*)0); // Setting the GS also set the VS copy shader
}

void ConstantUpdateEngine::setInternalSrtBuffer(ShaderStage stage, void *buf)
{
	SCE_GNM_VALIDATE(stage < Gnm::kShaderStageCount, "Invalid shader stage (%d).", stage);
	m_internalSrtBuffers[stage] = buf;
}

void ConstantUpdateEngine::setUserSrtBuffer(ShaderStage stage, const void *buf, uint32_t bufSizeInDwords)
{
	SCE_GNM_VALIDATE(stage < Gnm::kShaderStageCount, "Invalid shader stage (%d).", stage);
	SCE_GNM_VALIDATE(buf == 0 || (bufSizeInDwords > 0 && bufSizeInDwords <= 8), "bufSizeInDwords (%d) must be 0 if buf is NULL, or [1..8] if non-NULL.", bufSizeInDwords);
	m_userSrtDataSizeInDwords[stage] = bufSizeInDwords;
	const uint32_t *bufDwords = (const uint32_t *)buf;
	uint32_t *stageDwords = m_userSrtData[stage];
	switch(bufSizeInDwords)
	{
	case 8: stageDwords[7] = bufDwords[7];
	case 7: stageDwords[6] = bufDwords[6];
	case 6: stageDwords[5] = bufDwords[5];
	case 5: stageDwords[4] = bufDwords[4];
	case 4: stageDwords[3] = bufDwords[3];
	case 3: stageDwords[2] = bufDwords[2];
	case 2: stageDwords[1] = bufDwords[1];
	case 1: stageDwords[0] = bufDwords[0];
	}
}

void ConstantUpdateEngine::dumpDirtyChunkToRingBuffer(ConstantCommandBuffer *ccb, uint16_t chunkByteOffset, uint32_t chunkBytes, CueRingBuffer &ringBuffer)
{
	// Chunk is dirty in CPRAM; write to the next available RB slot
#ifdef ENABLE_HARDWARE_KCACHE
	m_anyWrapped |= ringBuffer.advanceHead();
#else // ENABLE_HARDWARE_KCACHE
	ringBuffer.advanceHead();
#endif // ENABLE_HARDWARE_KCACHE

	void *chunkAddr = ringBuffer.getCurrentHead();
	ccb->cpRamDump(chunkAddr, chunkByteOffset, chunkBytes/4);
}

void ConstantUpdateEngine::setGlobalDescriptor(ShaderGlobalResourceType resType, const Buffer *res)
{
	memcpy((uint8_t*)m_globalTablePtr+resType*sizeof(Gnm::Buffer), res, sizeof(Gnm::Buffer));
	m_globalTableNeedsUpdate = true;
}


void ConstantUpdateEngine::advanceFrame(void)
{
	SCE_GNM_VALIDATE(!m_betweenPreAndPostDraw, "This function must not be called between preDraw() and postDraw() or preDispatch() and postDispatch().");
#ifdef ENABLE_HARDWARE_KCACHE
	m_anyWrapped = true;
	for (uint32_t iStage = 0; iStage < kShaderStageCount; ++iStage)
	{
	    for (uint32_t iRing = 0; iRing < kNumRingBuffersPerStage; ++iRing)
		{
		    m_ringBuffers[iStage][iRing].submitted();
		}
	}
#endif // ENABLE_HARDWARE_KCACHE
}

void ConstantUpdateEngine::updateCpRam(ConstantCommandBuffer *ccb, ShaderStage stage, const InputUsageSlot *usageSlots, const uint32_t numInputSlots, void *outUsageChunkAddrs[16])
{
	// If the shader currently bound to this stage uses SRTs, we shouldn't be updating CPRAM
	bool isSrtShader = m_isSrtShader[stage];
	// Pre-pass over the usages to see if a new Extended User Data entry is required.
	// For now, if the shader uses the EUD at all, we allocate a new one. Long-term, we only need to advance if we're writing new
	// data to an EUD slot that's already been written to.
	bool foundEudPtrUsage = false;
	bool foundResourcesInEud = false;
	bool foundDirtyResourceInEud = false;
	for(uint32_t iUsage=0, iUsageMax=numInputSlots; iUsage<iUsageMax; ++iUsage)
	{
		uint8_t dstUserDataSlot = usageSlots[iUsage].m_startRegister;
		switch(usageSlots[iUsage].m_usageType)
		{
		case Gnm::kShaderInputUsageImmResource:
		case Gnm::kShaderInputUsageImmSampler:
		case Gnm::kShaderInputUsageImmConstBuffer:
		case Gnm::kShaderInputUsageImmVertexBuffer:
		case Gnm::kShaderInputUsageImmRwResource:
		case Gnm::kShaderInputUsageImmAluFloatConst:
		case Gnm::kShaderInputUsageImmAluBool32Const:
		case Gnm::kShaderInputUsageImmGdsCounterRange:
			foundResourcesInEud = foundResourcesInEud || (dstUserDataSlot >= 16);
			break;

		case Gnm::kShaderInputUsageImmGdsMemoryRange:
		case Gnm::kShaderInputUsageImmGwsBase:
			SCE_GNM_ASSERT(0); // not supported yet
			break;
		case Gnm::kShaderInputUsageSubPtrFetchShader:
			// nothing to do
			break;
		case Gnm::kShaderInputUsagePtrResourceTable:
		case Gnm::kShaderInputUsagePtrSamplerTable:
		case Gnm::kShaderInputUsagePtrConstBufferTable:
		case Gnm::kShaderInputUsagePtrVertexBufferTable:
		case Gnm::kShaderInputUsagePtrRwResourceTable:
			foundResourcesInEud = foundResourcesInEud || (dstUserDataSlot >= 16);
			break;

		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			 // nothing to do
			break;

		case Gnm::kShaderInputUsagePtrExtendedUserData:
			foundEudPtrUsage = true;
			break;

		case Gnm::kShaderInputUsagePtrSoBufferTable:
			// nothing to do
			break;

		case Gnm::kShaderInputUsagePtrIndirectResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
		default:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType); // not supported yet
			break;
		}

		if (foundEudPtrUsage && foundDirtyResourceInEud)
			break; // We've found everything we need
	}
	SCE_GNM_VALIDATE(!foundEudPtrUsage || foundResourcesInEud, "Found resources in Extended User Data, but no EUD table pointer.");

	// TEMPORARY: For now, the EUD is always dirty.
	foundDirtyResourceInEud = foundResourcesInEud;
	
	m_dirtyExtendedUserDataSlotBits[stage][0] = foundDirtyResourceInEud ? 1 : 0; // Should be setBit()
	m_dirtyExtendedUserDataChunkBits[stage][0] = foundDirtyResourceInEud ? 1 : 0; // Should be setBit()
	int32_t eudUsageIndex = -1;

	// Loop again over the usages updating the CPRAM contents from the shadow buffer, dumping dirty chunks from CPRAM into the appropriate ring buffers,
	// and determining the final GPU address for each usage's data (which we'll write to the shader user data in applyConstantsForD*().
	for(uint32_t iUsage=0, iUsageMax=numInputSlots; iUsage<iUsageMax; ++iUsage)
	{
		uint8_t dstUserDataSlot = usageSlots[iUsage].m_startRegister;
		uint8_t srcApiSlot      = usageSlots[iUsage].m_apiSlot; // For non-chunked input types, the srcApiSlot field contains the srcApiSlot.
		uint8_t chunkIndex      = srcApiSlot; // For chunked input types, the srcApiSlot field contains the chunk index
		uint8_t regCount        = usageSlots[iUsage].m_registerCount; // 1 -> 8 DW; 0 -> 4 DW
		uint32_t ringBufferIndex = 0;
		switch(usageSlots[iUsage].m_usageType)
		{
			// Immediate usage types don't go through CPRAM; they're set directly from the shadow buffer in applyConstantsForDraw().
		case Gnm::kShaderInputUsageImmResource:
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
			{
				// immediate is in ExtendedUserData; copy it into the shadow-CPRAM EUD chunk.
				SCE_GNM_VALIDATE(regCount <= 1, "regCount (%d) must be 0 or 1.", regCount);
				if (regCount == 1)
					setTsharpInExtendedUserData(stage, dstUserDataSlot, (const Gnm::Texture*)(m_cpRamShadowDwords + getResourceDwordOffset(stage, srcApiSlot)));
				else
					setVsharpInExtendedUserData(stage, dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getResourceDwordOffset(stage, srcApiSlot)));
			}
			break;
		case Gnm::kShaderInputUsageImmSampler:
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// immediate is in ExtendedUserData; copy it into the shadow-CPRAM EUD chunk.
				setSsharpInExtendedUserData(stage, dstUserDataSlot, (const Gnm::Sampler*)(m_cpRamShadowDwords + getSamplerDwordOffset(stage, srcApiSlot)));
			break;
		case Gnm::kShaderInputUsageImmConstBuffer:
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// immediate is in ExtendedUserData; copy it into the shadow-CPRAM EUD chunk.
				setVsharpInExtendedUserData(stage, dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getConstantBufferDwordOffset(stage, srcApiSlot)));
			break;
		case Gnm::kShaderInputUsageImmVertexBuffer:
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// immediate is in ExtendedUserData; copy it into the shadow-CPRAM EUD chunk.
				setVsharpInExtendedUserData(stage, dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getVertexBufferDwordOffset(stage, srcApiSlot)));
			break;
		case Gnm::kShaderInputUsageImmRwResource:
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
			{
				// immediate is in ExtendedUserData; copy it into the shadow-CPRAM EUD chunk.
				SCE_GNM_VALIDATE(regCount <= 1, "regCount (%d) must be 0 or 1.", regCount);
				if (regCount == 1)
					setTsharpInExtendedUserData(stage, dstUserDataSlot, (const Gnm::Texture*)(m_cpRamShadowDwords + getRwResourceDwordOffset(stage, srcApiSlot)));
				else
					setVsharpInExtendedUserData(stage, dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getRwResourceDwordOffset(stage, srcApiSlot)));
			}
			break;
		case Gnm::kShaderInputUsageImmAluFloatConst:
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// immediate is in ExtendedUserData; copy it into the shadow-CPRAM EUD chunk.
				setDwordInExtendedUserData(stage, dstUserDataSlot, m_cpRamShadowDwords[getFloatConstantDwordOffset(stage, srcApiSlot)]);
			break;
		case Gnm::kShaderInputUsageImmAluBool32Const:
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// immediate is in ExtendedUserData; copy it into the shadow-CPRAM EUD chunk.
					setDwordInExtendedUserData(stage, dstUserDataSlot, m_cpRamShadowDwords[getBoolConstantDwordOffset(stage, srcApiSlot)]);
			break;
		case Gnm::kShaderInputUsageImmGdsCounterRange:
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// immediate is in ExtendedUserData; copy it into the shadow-CPRAM EUD chunk.
				setDwordInExtendedUserData(stage, dstUserDataSlot, m_cpRamShadowDwords[getAppendConsumeCounterRangeDwordOffset(stage, srcApiSlot)]);
			break;

		case Gnm::kShaderInputUsageImmGdsMemoryRange:
		case Gnm::kShaderInputUsageImmGwsBase:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
		case Gnm::kShaderInputUsageImmShaderResourceTable:
			// Handle this in applyConstantsForDraw(); for now, just validate.
			SCE_GNM_VALIDATE(isSrtShader, "non-SRT shaders should not be looking for Shader Resource Tables!");
			break;
		case Gnm::kShaderInputUsageSubPtrFetchShader:
			// TODO: For now, fetch shaders are set manually by application and not by this class
			break;
		case Gnm::kShaderInputUsagePtrResourceTable:
			SCE_GNM_VALIDATE(chunkIndex < kChunkCountResource, "chunkIndex (%d) must be in the range [0..%d].", chunkIndex, kChunkCountResource-1);
			SCE_GNM_VALIDATE(chunkIndex == 0, "resource chunking is not yet supported."); // The active slot feature is not yet compatible with chunking, so make sure chunking isn't implemented yet.
			ringBufferIndex = kRingBuffersIndexResource + chunkIndex;
			SCE_GNM_VALIDATE(ringBufferIndex < kNumRingBuffersPerStage, "ringBufferIndex (%d) must be in the range [0..%d].", ringBufferIndex, kNumRingBuffersPerStage-1);
			if (testBit(m_dirtyResourceChunkBits[stage], chunkIndex))
			{
				uint16_t chunkByteOffset = (uint16_t)(getResourceDwordOffset(stage, chunkIndex*m_activeResourceSlotCounts[stage]/*Gnm::kChunkSlotCountResource*/)*sizeof(uint32_t));
				writeDirtyRangesToCpRam(ccb, m_dirtyResourceSlotBits[stage], chunkIndex*m_activeResourceSlotCounts[stage]/*Gnm::kChunkSlotCountResource*/, m_activeResourceSlotCounts[stage]/*Gnm::kChunkSlotCountResource*/, chunkByteOffset, Gnm::kDwordSizeResource*sizeof(uint32_t));
				dumpDirtyChunkToRingBuffer(ccb, chunkByteOffset, Gnm::kDwordSizeResource*m_activeResourceSlotCounts[stage]*sizeof(uint32_t)/*kChunkBytesResource*/, m_ringBuffers[stage][ringBufferIndex]);
			}
			outUsageChunkAddrs[iUsage] = m_ringBuffers[stage][ringBufferIndex].getCurrentHead();
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// Chunk is in ExtendedUserData; copy its pointer into the shadow-CPRAM EUD chunk
				setPointerInExtendedUserData(stage, dstUserDataSlot, outUsageChunkAddrs[iUsage]);
			break;
		case Gnm::kShaderInputUsagePtrInternalResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType); // not supported yet
			break;
		case Gnm::kShaderInputUsagePtrSamplerTable:
			SCE_GNM_VALIDATE(chunkIndex < kChunkCountSampler, "chunkIndex (%d) must be in the range [0..%d].", chunkIndex, kChunkCountSampler-1);
			SCE_GNM_VALIDATE(chunkIndex == 0, "resource chunking is not yet supported."); // The active slot feature is not yet compatible with chunking, so make sure chunking isn't implemented yet.
			ringBufferIndex = kRingBuffersIndexSampler + chunkIndex;
			SCE_GNM_VALIDATE(ringBufferIndex < kNumRingBuffersPerStage, "ringBufferIndex (%d) must be in the range [0..%d].", ringBufferIndex, kNumRingBuffersPerStage-1);
			if (testBit(m_dirtySamplerChunkBits[stage], chunkIndex))
			{
				uint16_t chunkByteOffset = (uint16_t)(getSamplerDwordOffset(stage, chunkIndex*m_activeSamplerSlotCounts[stage]/*Gnm::kChunkSlotCountSampler*/)*sizeof(uint32_t));
				writeDirtyRangesToCpRam(ccb, m_dirtySamplerSlotBits[stage], chunkIndex*m_activeSamplerSlotCounts[stage]/*Gnm::kChunkSlotCountSampler*/, m_activeSamplerSlotCounts[stage]/*Gnm::kChunkSlotCountSampler*/, chunkByteOffset, Gnm::kDwordSizeSampler*sizeof(uint32_t));
				dumpDirtyChunkToRingBuffer(ccb, chunkByteOffset, Gnm::kDwordSizeSampler*m_activeSamplerSlotCounts[stage]*sizeof(uint32_t)/*kChunkBytesSampler*/, m_ringBuffers[stage][ringBufferIndex]);
			}
			outUsageChunkAddrs[iUsage] = m_ringBuffers[stage][ringBufferIndex].getCurrentHead();
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// Chunk is in ExtendedUserData; copy its pointer into the shadow-CPRAM EUD chunk
				setPointerInExtendedUserData(stage, dstUserDataSlot, outUsageChunkAddrs[iUsage]);
			break;
		case Gnm::kShaderInputUsagePtrConstBufferTable:
			SCE_GNM_VALIDATE(chunkIndex < kChunkCountConstantBuffer, "chunkIndex (%d) must be in the range [0..%d].", chunkIndex, kChunkCountConstantBuffer-1);
			SCE_GNM_VALIDATE(chunkIndex == 0, "resource chunking is not yet supported."); // The active slot feature is not yet compatible with chunking, so make sure chunking isn't implemented yet.
			ringBufferIndex = kRingBuffersIndexConstantBuffer + chunkIndex;
			SCE_GNM_VALIDATE(ringBufferIndex < kNumRingBuffersPerStage, "ringBufferIndex (%d) must be in the range [0..%d].", ringBufferIndex, kNumRingBuffersPerStage-1);
			if (testBit(m_dirtyConstantBufferChunkBits[stage], chunkIndex))
			{
				uint16_t chunkByteOffset = (uint16_t)(getConstantBufferDwordOffset(stage, chunkIndex*m_activeConstantBufferSlotCounts[stage]/*Gnm::kChunkSlotCountConstantBuffer*/)*sizeof(uint32_t));
				writeDirtyRangesToCpRam(ccb, m_dirtyConstantBufferSlotBits[stage], chunkIndex*m_activeConstantBufferSlotCounts[stage]/*Gnm::kChunkSlotCountConstantBuffer*/, m_activeConstantBufferSlotCounts[stage]/*Gnm::kChunkSlotCountConstantBuffer*/, chunkByteOffset, Gnm::kDwordSizeConstantBuffer*sizeof(uint32_t));
				dumpDirtyChunkToRingBuffer(ccb, chunkByteOffset, Gnm::kDwordSizeConstantBuffer*m_activeConstantBufferSlotCounts[stage]*sizeof(uint32_t)/*kChunkBytesConstantBuffer*/, m_ringBuffers[stage][ringBufferIndex]);
			}
			outUsageChunkAddrs[iUsage] = m_ringBuffers[stage][ringBufferIndex].getCurrentHead();
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// Chunk is in ExtendedUserData; copy its pointer into the shadow-CPRAM EUD chunk
					setPointerInExtendedUserData(stage, dstUserDataSlot, outUsageChunkAddrs[iUsage]);
			break;
		case Gnm::kShaderInputUsagePtrVertexBufferTable:
			SCE_GNM_VALIDATE(chunkIndex < kChunkCountVertexBuffer, "chunkIndex (%d) must be in the range [0..%d].", chunkIndex, kChunkCountVertexBuffer-1);
			SCE_GNM_VALIDATE(chunkIndex == 0, "resource chunking is not yet supported."); // The active slot feature is not yet compatible with chunking, so make sure chunking isn't implemented yet.
			ringBufferIndex = kRingBuffersIndexVertexBuffer + chunkIndex;
			SCE_GNM_VALIDATE(ringBufferIndex < kNumRingBuffersPerStage, "ringBufferIndex (%d) must be in the range [0..%d].", ringBufferIndex, kNumRingBuffersPerStage-1);
			if (testBit(m_dirtyVertexBufferChunkBits[stage], chunkIndex))
			{
				uint16_t chunkByteOffset = (uint16_t)(getVertexBufferDwordOffset(stage, chunkIndex*m_activeVertexBufferSlotCounts[stage]/*Gnm::kChunkSlotCountVertexBuffer*/)*sizeof(uint32_t));
				writeDirtyRangesToCpRam(ccb, m_dirtyVertexBufferSlotBits[stage], chunkIndex*m_activeVertexBufferSlotCounts[stage]/*Gnm::kChunkSlotCountVertexBuffer*/, m_activeVertexBufferSlotCounts[stage]/*Gnm::kChunkSlotCountVertexBuffer*/, chunkByteOffset, Gnm::kDwordSizeVertexBuffer*sizeof(uint32_t));
				dumpDirtyChunkToRingBuffer(ccb, chunkByteOffset, Gnm::kDwordSizeVertexBuffer*m_activeVertexBufferSlotCounts[stage]*sizeof(uint32_t)/*kChunkBytesVertexBuffer*/, m_ringBuffers[stage][ringBufferIndex]);
			}
			outUsageChunkAddrs[iUsage] = m_ringBuffers[stage][ringBufferIndex].getCurrentHead();
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// Chunk is in ExtendedUserData; copy its pointer into the shadow-CPRAM EUD chunk
				setPointerInExtendedUserData(stage, dstUserDataSlot, outUsageChunkAddrs[iUsage]);
			break;
		case Gnm::kShaderInputUsagePtrSoBufferTable:
			SCE_GNM_VALIDATE(chunkIndex < kChunkCountStreamoutBuffer, "chunkIndex (%d) must be in the range [0..%d].", chunkIndex, kChunkCountStreamoutBuffer-1);
			SCE_GNM_VALIDATE(chunkIndex == 0, "resource chunking is not yet supported."); // The active slot feature is not yet compatible with chunking, so make sure chunking isn't implemented yet.
			SCE_GNM_VALIDATE(stage==Gnm::kShaderStageVs, "Stream out only support VS Stage");
			ringBufferIndex = kRingBuffersIndexStreamoutBuffer + chunkIndex;
			SCE_GNM_VALIDATE(ringBufferIndex < kNumRingBuffersPerStage, "ringBufferIndex (%d) must be in the range [0..%d].", ringBufferIndex, kNumRingBuffersPerStage-1);
			if (testBit(m_dirtyStreamoutBufferChunkBits[stage], chunkIndex))
			{
				uint16_t chunkByteOffset = (uint16_t)(getStreamoutDwordOffset(stage, chunkIndex*m_activeStreamoutBufferSlotCount/*Gnm::kChunkSlotCountStreamoutBuffer*/)*sizeof(uint32_t));
				writeDirtyRangesToCpRam(ccb, m_dirtyStreamoutBufferSlotBits[stage], chunkIndex*m_activeStreamoutBufferSlotCount/*Gnm::kChunkSlotCountStreamoutBuffer*/, m_activeStreamoutBufferSlotCount/*Gnm::kChunkSlotCountStreamoutBuffer*/, chunkByteOffset, Gnm::kDwordSizeStreamoutBuffer*sizeof(uint32_t));
				dumpDirtyChunkToRingBuffer(ccb, chunkByteOffset, Gnm::kDwordSizeStreamoutBuffer*m_activeStreamoutBufferSlotCount*sizeof(uint32_t)/*kChunkBytesStreamoutBuffer*/, m_ringBuffers[stage][ringBufferIndex]);
			}
			outUsageChunkAddrs[iUsage] = m_ringBuffers[stage][ringBufferIndex].getCurrentHead();
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// Chunk is in ExtendedUserData; copy its pointer into the shadow-CPRAM EUD chunk
					setPointerInExtendedUserData(stage, dstUserDataSlot, outUsageChunkAddrs[iUsage]);
			break;
		case Gnm::kShaderInputUsagePtrRwResourceTable:
			SCE_GNM_VALIDATE(chunkIndex < kChunkCountRwResource, "chunkIndex (%d) must be in the range [0..%d].", chunkIndex, kChunkCountRwResource-1);
			SCE_GNM_VALIDATE(chunkIndex == 0, "resource chunking is not yet supported."); // The active slot feature is not yet compatible with chunking, so make sure chunking isn't implemented yet.
			ringBufferIndex = kRingBuffersIndexRwResource + chunkIndex;
			SCE_GNM_VALIDATE(ringBufferIndex < kNumRingBuffersPerStage, "ringBufferIndex (%d) must be in the range [0..%d].", ringBufferIndex, kNumRingBuffersPerStage-1);
			if (testBit(m_dirtyRwResourceChunkBits[stage], chunkIndex))
			{
				uint16_t chunkByteOffset = (uint16_t)(getRwResourceDwordOffset(stage, chunkIndex*m_activeRwResourceSlotCounts[stage]/*Gnm::kChunkSlotCountRwResource*/)*sizeof(uint32_t));
				writeDirtyRangesToCpRam(ccb, m_dirtyRwResourceSlotBits[stage], chunkIndex*m_activeRwResourceSlotCounts[stage]/*Gnm::kChunkSlotCountRwResource*/, m_activeRwResourceSlotCounts[stage]/*Gnm::kChunkSlotCountRwResource*/, chunkByteOffset, Gnm::kDwordSizeRwResource*sizeof(uint32_t));
				dumpDirtyChunkToRingBuffer(ccb, chunkByteOffset, Gnm::kDwordSizeRwResource*m_activeRwResourceSlotCounts[stage]*sizeof(uint32_t)/*kChunkBytesRwResource*/, m_ringBuffers[stage][ringBufferIndex]);
			}
			outUsageChunkAddrs[iUsage] = m_ringBuffers[stage][ringBufferIndex].getCurrentHead();
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// Chunk is in ExtendedUserData; copy its pointer into the shadow-CPRAM EUD chunk
					setPointerInExtendedUserData(stage, dstUserDataSlot, outUsageChunkAddrs[iUsage]);
			break;
		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			SCE_GNM_VALIDATE(m_globalTableAddr, "Shader requires a global resource table, but none has been specified. Call setGlobalResourceTableAddr() first!");
			outUsageChunkAddrs[iUsage] = m_globalTableAddr;
			if (dstUserDataSlot >= 16 && m_dirtyExtendedUserDataSlotBits[stage][0])
				// Chunk is in ExtendedUserData; copy its pointer into the shadow-CPRAM EUD chunk
				setPointerInExtendedUserData(stage, dstUserDataSlot, outUsageChunkAddrs[iUsage]);
			break;

		case Gnm::kShaderInputUsagePtrExtendedUserData:
			eudUsageIndex = static_cast<int32_t>(iUsage);
			// handled after the loop finishes
			break;
		case Gnm::kShaderInputUsagePtrIndirectResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType); // not supported yet
			break;
		case Gnm::kShaderInputUsagePtrDispatchDraw:
			SCE_GNM_VALIDATE(chunkIndex < kChunkCountDispatchDrawData, "chunkIndex (%d) must be in the range [0..%d].", chunkIndex, kChunkCountDispatchDrawData-1);
			SCE_GNM_VALIDATE(stage==Gnm::kShaderStageVs || stage==Gnm::kShaderStageCs, "kShaderInputUsagePtrDispatchDraw only supported for VS and CS Stages");
			SCE_GNM_VALIDATE((dstUserDataSlot & 1) || dstUserDataSlot+2 <= 16, "kShaderInputUsagePtrDispatchDraw requires m_startRegister (%d) to be 2-sgpr aligned and in real user sgprs [0..%d].", dstUserDataSlot, 16-2);
			ringBufferIndex = kRingBuffersIndexDispatchDrawData + chunkIndex;
			SCE_GNM_VALIDATE(ringBufferIndex < kNumRingBuffersPerStage, "ringBufferIndex (%d) must be in the range [0..%d].", ringBufferIndex, kNumRingBuffersPerStage-1);
			if (testBit(m_dirtyDispatchDrawDataChunkBits, chunkIndex))
			{
				uint16_t chunkByteOffset = (uint16_t)(getDispatchDrawDataDwordOffset(chunkIndex*Gnm::kChunkSlotCountDispatchDrawData)*sizeof(uint32_t));
				uint16_t dddChunkByteOffset = (uint16_t)(chunkByteOffset + m_dirtyDddDwordRanges.m_firstDword * sizeof(uint32_t));
				uint16_t dddChunkByteSize = (m_dirtyDddDwordRanges.m_lastDwordPlusOne - m_dirtyDddDwordRanges.m_firstDword) * sizeof(uint32_t);
				writeDirtyRangesToCpRam(ccb, m_dirtyDispatchDrawDataSlotBits, chunkIndex*Gnm::kChunkSlotCountDispatchDrawData, Gnm::kChunkSlotCountDispatchDrawData, dddChunkByteOffset, dddChunkByteSize);
				dumpDirtyChunkToRingBuffer(ccb, chunkByteOffset, kChunkBytesDispatchDrawData, m_ringBuffers[kShaderStageCs][ringBufferIndex]);
				m_dirtyDddDwordRanges.m_firstDword = Gnm::kDwordSizeDispatchDrawData;
				m_dirtyDddDwordRanges.m_lastDwordPlusOne = 0;
			}
			outUsageChunkAddrs[iUsage] = m_ringBuffers[kShaderStageCs][ringBufferIndex].getCurrentHead();
			break;
		default:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType); // not supported yet
			break;
		}
	}

	// SRT shaders use the EUD buffer for internal SRT data, but do not copy it to CPRAM.
	if (!isSrtShader)
	{
		// Now that the shadow EUD is fully populated, we can copy it to/from CPRAM if necessary.
		if (testBit(m_dirtyExtendedUserDataChunkBits[stage], 0))
		{
			SCE_GNM_VALIDATE(testBit(m_dirtyExtendedUserDataSlotBits[stage], 0), "Found dirty EUD chunk with clean EUD slot.");
			// Only copy the range of EUD DWORDs that are actually dirty
			SCE_GNM_VALIDATE(m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne > m_dirtyEudSlotRanges[stage].m_firstSlot, "Expected last+1 > first in dirty EUD chunk.");
			SCE_GNM_VALIDATE(m_dirtyEudSlotRanges[stage].m_firstSlot >= 16, "First dirty EUD slot (%d) is not in EUD!", m_dirtyEudSlotRanges[stage].m_firstSlot);
			SCE_GNM_VALIDATE(m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne <= 16+Gnm::kDwordSizeExtendedUserData, "dirty EUD slot range extends beyond EUD size.");

			uint16_t eudChunkByteOffset = (uint16_t)(getExtendedUserDataDwordOffset(stage, 0) + m_dirtyEudSlotRanges[stage].m_firstSlot-16) * sizeof(uint32_t);
			uint16_t eudChunkByteSize = (m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne - m_dirtyEudSlotRanges[stage].m_firstSlot) * sizeof(uint32_t);
			writeDirtyRangesToCpRam(ccb, m_dirtyExtendedUserDataSlotBits[stage], 0, Gnm::kChunkSlotCountExtendedUserData, eudChunkByteOffset, eudChunkByteSize);
			dumpDirtyChunkToRingBuffer(ccb, eudChunkByteOffset, eudChunkByteSize, m_ringBuffers[stage][kRingBuffersIndexExtendedUserData]);
			// Reset dirty EUD slot range to an invalid state (first > lastPlusOne) now that it's clean.
			m_dirtyEudSlotRanges[stage].m_firstSlot = Gnm::kDwordSizeExtendedUserData;
			m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne = 0;
		}
		if (eudUsageIndex >= 0)
		{
			outUsageChunkAddrs[eudUsageIndex] = m_ringBuffers[stage][kRingBuffersIndexExtendedUserData].getCurrentHead();
		}
	}
}

void ConstantUpdateEngine::applyConstantsForDraw(DrawCommandBuffer *dcb, ShaderStage stage, const InputUsageSlot *usageSlots, const uint32_t numInputSlots, void *usageChunkAddrs[16])
{
	// If the shader currently bound to this stage uses SRTs, we can safely skip the CPRAM
	bool isSrtShader = m_isSrtShader[stage];

	for(uint32_t iUsage=0, iUsageMax=numInputSlots; iUsage<iUsageMax; ++iUsage)
	{
		uint8_t dstUserDataSlot = usageSlots[iUsage].m_startRegister;
		uint8_t srcApiSlot      = usageSlots[iUsage].m_apiSlot; // For non-chunked input types, the srcApiSlot field contains the source API slot.
		uint8_t regCount        = usageSlots[iUsage].m_registerCount; // 1 -> 8 DW; 0 -> 4 DW
		
		switch(usageSlots[iUsage].m_usageType)
		{
		case Gnm::kShaderInputUsageImmResource:
			if (dstUserDataSlot < 16)
			{
				SCE_GNM_VALIDATE(regCount <= 1, "regCount (%d) must be 0 or 1.", regCount);
				if ( regCount == 1 )
				{
					dcb->setTsharpInUserData(stage, dstUserDataSlot, (const Gnm::Texture*)(m_cpRamShadowDwords + getResourceDwordOffset(stage, srcApiSlot)));
				}
				else
				{
					dcb->setVsharpInUserData(stage, dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getResourceDwordOffset(stage, srcApiSlot)));
				}
			}
			break;
		case Gnm::kShaderInputUsageImmSampler:
			if (dstUserDataSlot < 16)
			{
				dcb->setSsharpInUserData(stage, dstUserDataSlot, (const Gnm::Sampler*)(m_cpRamShadowDwords + getSamplerDwordOffset(stage, srcApiSlot)));
			}
			break;
		case Gnm::kShaderInputUsageImmConstBuffer:
			if (dstUserDataSlot < 16)
			{
				dcb->setVsharpInUserData(stage, dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getConstantBufferDwordOffset(stage, srcApiSlot)));
			}
			break;
		case Gnm::kShaderInputUsageImmVertexBuffer:
			if (dstUserDataSlot < 16)
			{
				dcb->setVsharpInUserData(stage, dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getVertexBufferDwordOffset(stage, srcApiSlot)));
			}
			break;
		case Gnm::kShaderInputUsageImmRwResource:
			if (dstUserDataSlot < 16)
			{
				SCE_GNM_VALIDATE(regCount <= 1, "regCount (%d) must be 0 or 1.", regCount);
				if ( regCount == 1 )
				{
					dcb->setTsharpInUserData(stage, dstUserDataSlot, (const Gnm::Texture*)(m_cpRamShadowDwords + getRwResourceDwordOffset(stage, srcApiSlot)));
				}
				else
				{
					dcb->setVsharpInUserData(stage, dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getRwResourceDwordOffset(stage, srcApiSlot)));
				}
			}
			break;
		case Gnm::kShaderInputUsageImmAluFloatConst:
			if (dstUserDataSlot < 16)
			{
				dcb->setUserData(stage, dstUserDataSlot, m_cpRamShadowDwords[getFloatConstantDwordOffset(stage, srcApiSlot)]);
			}
			break;
		case Gnm::kShaderInputUsageImmAluBool32Const:
			if (dstUserDataSlot < 16)
			{
				dcb->setUserData(stage, dstUserDataSlot, m_cpRamShadowDwords[getBoolConstantDwordOffset(stage, srcApiSlot)]);
			}
			break;
		case Gnm::kShaderInputUsageImmGdsCounterRange:
			if (dstUserDataSlot < 16)
			{
				dcb->setUserData(stage, dstUserDataSlot, m_cpRamShadowDwords[getAppendConsumeCounterRangeDwordOffset(stage, srcApiSlot)]);
			}
			break;
		case Gnm::kShaderInputUsageImmGdsMemoryRange:
		case Gnm::kShaderInputUsageImmGwsBase:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
		case Gnm::kShaderInputUsageImmShaderResourceTable:
			{
				uint32_t dwCount = usageSlots[iUsage].m_srtSizeInDWordMinusOne+1;
				SCE_GNM_VALIDATE(dwCount == m_userSrtDataSizeInDwords[stage], "SRT user data size mismatch: shader expected %d DWORDS, but caller provided %d.", 
					dwCount, m_userSrtDataSizeInDwords[stage]);
				for(uint32_t iDW=0; iDW<dwCount; ++iDW)
				{
					dcb->setUserData(stage, dstUserDataSlot+iDW, m_userSrtData[stage][iDW]);
				}
			}
			break;
		case Gnm::kShaderInputUsageSubPtrFetchShader:
			// TODO: For now, fetch shaders are set manually by application and not by this class
			break;
		// Chunk-based usages bind to the address determined in updateCpRam()
		case Gnm::kShaderInputUsagePtrResourceTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyResourceChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				dcb->setPointerInUserData(stage, dstUserDataSlot, (void*)usageChunkAddrs[iUsage]);
			}
			clearBit(m_dirtyResourceChunkBits[stage], srcApiSlot);
			break;
		case Gnm::kShaderInputUsagePtrInternalResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
		case Gnm::kShaderInputUsagePtrSamplerTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtySamplerChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				dcb->setPointerInUserData(stage, dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			clearBit(m_dirtySamplerChunkBits[stage], srcApiSlot);
			break;
		case Gnm::kShaderInputUsagePtrConstBufferTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyConstantBufferChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				dcb->setPointerInUserData(stage, dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			clearBit(m_dirtyConstantBufferChunkBits[stage], srcApiSlot);
			break;
		case Gnm::kShaderInputUsagePtrVertexBufferTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyVertexBufferChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				dcb->setPointerInUserData(stage, dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			clearBit(m_dirtyVertexBufferChunkBits[stage], srcApiSlot);
			break;
		case Gnm::kShaderInputUsagePtrSoBufferTable:
			SCE_GNM_VALIDATE(stage==Gnm::kShaderStageVs, "Stream out only support VS Stage");
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyStreamoutBufferChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				dcb->setPointerInUserData(stage, dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			clearBit(m_dirtyStreamoutBufferChunkBits[stage], srcApiSlot);
			break;
		case Gnm::kShaderInputUsagePtrRwResourceTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyRwResourceChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				dcb->setPointerInUserData(stage, dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			clearBit(m_dirtyRwResourceChunkBits[stage], srcApiSlot);
			break;
		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			if (dstUserDataSlot < 16)
			{
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				dcb->setPointerInUserData(stage, dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			break;
		case Gnm::kShaderInputUsagePtrExtendedUserData:
			SCE_GNM_ASSERT(dstUserDataSlot < 16); // EUD pointers can't be stored in EUD
			if (isSrtShader)
			{
				dcb->setPointerInUserData(stage, dstUserDataSlot, m_internalSrtBuffers[stage]);
			}
			else
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyExtendedUserDataChunkBits[stage], 0))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				dcb->setPointerInUserData(stage, dstUserDataSlot, usageChunkAddrs[iUsage]);
				clearBit(m_dirtyExtendedUserDataChunkBits[stage], 0);
			}
			break;
		case Gnm::kShaderInputUsagePtrIndirectResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
		case Gnm::kShaderInputUsagePtrDispatchDraw:
			SCE_GNM_ASSERT(dstUserDataSlot + 2 <= 16); // DDD pointers can't be stored in EUD
			if (!m_dirtyShaders[stage] && !testBit(m_dirtyDispatchDrawDataChunkBits, 0))
				continue; // chunk-based usages don't need to be set redundantly
			SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
			dcb->setPointerInUserData(stage, dstUserDataSlot, usageChunkAddrs[iUsage]);
			clearBit(m_dirtyDispatchDrawDataChunkBits, 0);
			break;
		default:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
		}
	}
}

void ConstantUpdateEngine::applyConstantsForDispatchDrawAcb(DispatchCommandBuffer *acb, const InputUsageSlot *usageSlots, const uint32_t numInputSlots, void *usageChunkAddrs[16])
{
	ShaderStage stage = kShaderStageVs;
	for(uint32_t iUsage=0, iUsageMax=numInputSlots; iUsage<iUsageMax; ++iUsage)
	{
		uint8_t dstUserDataSlot = usageSlots[iUsage].m_startRegister;
		uint8_t srcApiSlot      = usageSlots[iUsage].m_apiSlot; // For non-chunked input types, the srcApiSlot field contains the source API slot.
		uint8_t regCount        = usageSlots[iUsage].m_registerCount; // 1 -> 8 DW; 0 -> 4 DW

		switch(usageSlots[iUsage].m_usageType)
		{
		case Gnm::kShaderInputUsageImmResource:
			if (dstUserDataSlot < 16)
			{
				SCE_GNM_VALIDATE(regCount <= 1, "regCount (%d) must be 0 or 1.", regCount);
				if ( regCount == 1 )
				{
					acb->setTsharpInUserData(dstUserDataSlot, (const Gnm::Texture*)(m_cpRamShadowDwords + getResourceDwordOffset(stage, srcApiSlot)));
				}
				else
				{
					acb->setVsharpInUserData(dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getResourceDwordOffset(stage, srcApiSlot)));
				}
			}
			break;
		case Gnm::kShaderInputUsageImmSampler:
			if (dstUserDataSlot < 16)
			{
				acb->setSsharpInUserData(dstUserDataSlot, (const Gnm::Sampler*)(m_cpRamShadowDwords + getSamplerDwordOffset(stage, srcApiSlot)));
			}
			break;
		case Gnm::kShaderInputUsageImmConstBuffer:
			if (dstUserDataSlot < 16)
			{
				acb->setVsharpInUserData(dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getConstantBufferDwordOffset(stage, srcApiSlot)));
			}
			break;
		case Gnm::kShaderInputUsageImmVertexBuffer:
			if (dstUserDataSlot < 16)
			{
				acb->setVsharpInUserData(dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getVertexBufferDwordOffset(stage, srcApiSlot)));
			}
			break;
		case Gnm::kShaderInputUsageImmRwResource:
			if (dstUserDataSlot < 16)
			{
				SCE_GNM_VALIDATE(regCount <= 1, "regCount (%d) must be 0 or 1.", regCount);
				if ( regCount == 1 )
				{
					acb->setTsharpInUserData(dstUserDataSlot, (const Gnm::Texture*)(m_cpRamShadowDwords + getRwResourceDwordOffset(stage, srcApiSlot)));
				}
				else
				{
					acb->setVsharpInUserData(dstUserDataSlot, (const Gnm::Buffer*)(m_cpRamShadowDwords + getRwResourceDwordOffset(stage, srcApiSlot)));
				}
			}
			break;
		case Gnm::kShaderInputUsageImmAluFloatConst:
			if (dstUserDataSlot < 16)
			{
				acb->setUserData(dstUserDataSlot, m_cpRamShadowDwords[getFloatConstantDwordOffset(stage, srcApiSlot)]);
			}
			break;
		case Gnm::kShaderInputUsageImmAluBool32Const:
			if (dstUserDataSlot < 16)
			{
				acb->setUserData(dstUserDataSlot, m_cpRamShadowDwords[getBoolConstantDwordOffset(stage, srcApiSlot)]);
			}
			break;
		case Gnm::kShaderInputUsageImmGdsCounterRange:
			if (dstUserDataSlot < 16)
			{
				acb->setUserData(dstUserDataSlot, m_cpRamShadowDwords[getAppendConsumeCounterRangeDwordOffset(stage, srcApiSlot)]);
			}
			break;
		case Gnm::kShaderInputUsageImmGdsMemoryRange:
		case Gnm::kShaderInputUsageImmGwsBase:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
			// Chunk-based usages bind to the address determined in updateCpRam()
		case Gnm::kShaderInputUsageSubPtrFetchShader:
			// TODO: For now, fetch shaders are set manually by application and not by this class
			break;
		case Gnm::kShaderInputUsagePtrResourceTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyResourceChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				acb->setPointerInUserData(dstUserDataSlot, (void*)usageChunkAddrs[iUsage]);
			}
			break;
		case Gnm::kShaderInputUsagePtrInternalResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
		case Gnm::kShaderInputUsagePtrSamplerTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtySamplerChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				acb->setPointerInUserData(dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			break;
		case Gnm::kShaderInputUsagePtrConstBufferTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyConstantBufferChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				acb->setPointerInUserData(dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			break;
		case Gnm::kShaderInputUsagePtrVertexBufferTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyVertexBufferChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				acb->setPointerInUserData(dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			break;
		case Gnm::kShaderInputUsagePtrSoBufferTable:
			SCE_GNM_VALIDATE(stage==Gnm::kShaderStageVs, "Stream out only support VS Stage");
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyStreamoutBufferChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				acb->setPointerInUserData(dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			break;
		case Gnm::kShaderInputUsagePtrRwResourceTable:
			if (dstUserDataSlot < 16)
			{
				if (!m_dirtyShaders[stage] && !testBit(m_dirtyRwResourceChunkBits[stage], srcApiSlot))
					continue; // chunk-based usages don't need to be set redundantly
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				acb->setPointerInUserData(dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			break;
		case Gnm::kShaderInputUsagePtrInternalGlobalTable:
			if (dstUserDataSlot < 16)
			{
				SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
				acb->setPointerInUserData(dstUserDataSlot, usageChunkAddrs[iUsage]);
			}
			break;
		case Gnm::kShaderInputUsagePtrExtendedUserData:
			SCE_GNM_ASSERT(dstUserDataSlot < 16); // EUD pointers can't be stored in EUD
			if (!m_dirtyShaders[stage] && !testBit(m_dirtyExtendedUserDataChunkBits[stage], 0))
				continue; // chunk-based usages don't need to be set redundantly
			SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
			acb->setPointerInUserData(dstUserDataSlot, usageChunkAddrs[iUsage]);
			break;
		case Gnm::kShaderInputUsagePtrIndirectResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectInternalResourceTable:
		case Gnm::kShaderInputUsagePtrIndirectRwResourceTable:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
		case Gnm::kShaderInputUsagePtrDispatchDraw:
			SCE_GNM_ASSERT(dstUserDataSlot < 16); // DDD pointers can't be stored in EUD
			if (!m_dirtyShaders[stage] && !testBit(m_dirtyDispatchDrawDataChunkBits, 0))
				continue; // chunk-based usages don't need to be set redundantly
			SCE_GNM_ASSERT(usageChunkAddrs[iUsage] != 0);
			acb->setPointerInUserData(dstUserDataSlot, usageChunkAddrs[iUsage]);
			break;
		default:
			SCE_GNM_ERROR("Unsupported shader input usage type 0x%02X.", usageSlots[iUsage].m_usageType);
			break;
		}
	}
}

void ConstantUpdateEngine::preDraw(DrawCommandBuffer *dcb, ConstantCommandBuffer *ccb)
{
	m_betweenPreAndPostDraw = true;

	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	SCE_GNM_VALIDATE(ccb, "ccb must not be NULL.");

	// Let the constant engine run ahead by n draw calls.  This is dependent on the size of the ring buffers.
#ifdef ENABLE_HARDWARE_KCACHE
	ccb->waitOnDeCounterDiff(m_chunksPerRingBuffer/4);
	ConstantCommandBuffer savedCcb = *ccb;
	savedCcb.m_cmdptr -= kWaitOnDeCounterDiffSizeInDword;
#else // ENABLE_HARDWARE_KCACHE
	ccb->waitOnDeCounterDiff(m_chunksPerRingBuffer);
#endif // ENABLE_HARDWARE_KCACHE

	uint32_t lshs_en = 0, esgs_en = 0; 
	switch (m_activeShaderStages) {
	case kActiveShaderStagesVsPs:			break;
	case kActiveShaderStagesLsHsVsPs:		lshs_en = 1;	break;
	case kActiveShaderStagesLsHsEsGsVsPs:	lshs_en = esgs_en = 1;	break;
	case kActiveShaderStagesEsGsVsPs:		esgs_en = 1;	break;
	default:
		SCE_GNM_VALIDATE(0, "setActiveShaderStages must be VsPs, EsGsVsPs, LsHsVsPs, or LsHsEsGsVsPs");
	}

	// Bind current shaders. This would be a great place for some validation.
	if (m_dirtyShaders[Gnm::kShaderStagePs])
	{
		dcb->setPsShader(m_currentPSB ? &m_currentPSB->m_psStageRegisters : NULL);
	}
	if ( m_currentVSB && m_dirtyShaders[Gnm::kShaderStageVs] )
	{
		dcb->setVsShader(&m_currentVSB->m_vsStageRegisters, m_currentVsShaderModifier);
		if ( m_currentVSFetchShaderAddr )
		{
			dcb->setPointerInUserData(Gnm::kShaderStageVs, 0, m_currentVSFetchShaderAddr); // Fetch shader is Hardcoded to register 0
		}
	}
	if ( m_currentLSB && lshs_en && m_dirtyShaders[Gnm::kShaderStageLs] )
	{
		dcb->setLsShader(&m_currentLSB->m_lsStageRegisters, m_currentLsShaderModifier);
		if ( m_currentLSFetchShaderAddr )
		{
			dcb->setPointerInUserData(Gnm::kShaderStageLs, 0, m_currentLSFetchShaderAddr); // Fetch shader is Hardcoded to register 0
		}
	}
	if ( m_currentHSB && lshs_en && m_dirtyShaders[Gnm::kShaderStageHs] )
	{
		dcb->setHsShader(&m_currentHSB->m_hsStageRegisters, &m_currentTessRegs);
	}
	if ( m_currentESB && esgs_en && m_dirtyShaders[Gnm::kShaderStageEs] )
	{
		dcb->setEsShader(&m_currentESB->m_esStageRegisters, m_currentEsShaderModifier);
		if ( m_currentESFetchShaderAddr )
		{
			dcb->setPointerInUserData(Gnm::kShaderStageEs, 0, m_currentESFetchShaderAddr); // Fetch shader is Hardcoded to register 0
		}
	}
	if ( m_currentGSB && esgs_en && m_dirtyShaders[Gnm::kShaderStageGs] )
	{
		dcb->setGsShader(&m_currentGSB->m_gsStageRegisters);
	}
	// Don't clear shader dirty bits until after the calls to applyConstantsForDraw()

	// Set up VS to PS semantic mapping, but only if VS and PS stage shaders are set, and if one (or both) shaders are dirty
	if ((m_currentVSB != NULL && m_currentPSB != NULL) && (m_dirtyShaders[Gnm::kShaderStageVs] || m_dirtyShaders[Gnm::kShaderStagePs]))
	{
		if (m_currentPSB->m_numInputSemantics != 0)
		{
			if ( m_psInputs )
			{
				dcb->setPsShaderUsage(m_psInputs, m_currentPSB->m_numInputSemantics);
			}
			else
			{
				uint32_t psInputs[32];
				Gnm::generatePsShaderUsageTable(psInputs,
												m_currentVSB->getExportSemanticTable(), m_currentVSB->m_numExportSemantics,
												m_currentPSB->getPixelInputSemanticTable(), m_currentPSB->m_numInputSemantics);
				dcb->setPsShaderUsage(psInputs, m_currentPSB->m_numInputSemantics);
			}
		}
	}

	if (m_currentVSB != NULL)
	{
		void *usageChunkAddrs[16] = {0};
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentVSB->getInputUsageSlotTable();
		updateCpRam(ccb, Gnm::kShaderStageVs, inputUsageSlots, m_currentVSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		applyConstantsForDraw(dcb, Gnm::kShaderStageVs, inputUsageSlots, m_currentVSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	}
	if (m_currentPSB != NULL)
	{
		void *usageChunkAddrs[16] = {0};
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentPSB->getInputUsageSlotTable();
		updateCpRam(ccb, Gnm::kShaderStagePs, inputUsageSlots, m_currentPSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		applyConstantsForDraw(dcb, Gnm::kShaderStagePs, inputUsageSlots, m_currentPSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	}
	if (m_currentLSB != NULL && lshs_en)
	{
		void *usageChunkAddrs[16] = {0};
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentLSB->getInputUsageSlotTable();
		updateCpRam(ccb, Gnm::kShaderStageLs, inputUsageSlots, m_currentLSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		applyConstantsForDraw(dcb, Gnm::kShaderStageLs, inputUsageSlots, m_currentLSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	}
	if (m_currentHSB != NULL && lshs_en)
	{
		void *usageChunkAddrs[16] = {0};
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentHSB->getInputUsageSlotTable();
		updateCpRam(ccb, Gnm::kShaderStageHs, inputUsageSlots, m_currentHSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		applyConstantsForDraw(dcb, Gnm::kShaderStageHs, inputUsageSlots, m_currentHSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	}
	if (m_currentESB != NULL && esgs_en)
	{
		void *usageChunkAddrs[16] = {0};
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentESB->getInputUsageSlotTable();
		updateCpRam(ccb, Gnm::kShaderStageEs, inputUsageSlots, m_currentESB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		applyConstantsForDraw(dcb, Gnm::kShaderStageEs, inputUsageSlots, m_currentESB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	}
	if (m_currentGSB != NULL && esgs_en)
	{
		void *usageChunkAddrs[16] = {0};
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentGSB->getInputUsageSlotTable();
		updateCpRam(ccb, Gnm::kShaderStageGs, inputUsageSlots, m_currentGSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		applyConstantsForDraw(dcb, Gnm::kShaderStageGs, inputUsageSlots, m_currentGSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	}
	// We can now safely clear the shader dirty bits
	m_dirtyShaders[Gnm::kShaderStagePs] = false;
	m_dirtyShaders[Gnm::kShaderStageVs] = false;
	if (lshs_en) {
		m_dirtyShaders[Gnm::kShaderStageLs] = false;
		m_dirtyShaders[Gnm::kShaderStageHs] = false;
	}
	if (esgs_en) {
		m_dirtyShaders[Gnm::kShaderStageEs] = false;
		m_dirtyShaders[Gnm::kShaderStageGs] = false;
	}

	// Check if we need to update the global table:
	if ( m_globalTableNeedsUpdate )
	{
		SCE_GNM_VALIDATE(m_globalTableAddr, "Global table pointer not specified. Call setGlobalResourceTableAddr() first!");
		dcb->writeDataInlineThroughL2(m_globalTableAddr, m_globalTablePtr, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE/4, Gnm::kCachePolicyLru, Gnm::kWriteDataConfirmEnable);
		// Invalidate KCache:
		dcb->flushShaderCachesAndWait(kCacheActionNone,kExtendedCacheActionInvalidateKCache, kStallCommandBufferParserDisable);
		m_globalTableNeedsUpdate = false;
	}

	dcb->waitOnCe();
	ccb->incrementCeCounter();

#ifdef ENABLE_HARDWARE_KCACHE
	if ( !m_anyWrapped )
	{
		savedCcb.insertNop(kWaitOnDeCounterDiffSizeInDword);
	}

	m_anyWrapped = false;
#else // ENABLE_HARDWARE_KCACHE
	// Invalidate KCache:
	dcb->flushShaderCachesAndWait(Gnm::kWaitCacheFlushK);
#endif // ENABLE_HARDWARE_KCACHE
}

void ConstantUpdateEngine::postDraw(DrawCommandBuffer *dcb, ConstantCommandBuffer *ccb)
{
	m_betweenPreAndPostDraw = false;

	SCE_GNM_UNUSED(ccb);
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	SCE_GNM_VALIDATE(ccb, "ccb must not be NULL.");

	// Inform the constant engine that this draw has ended so that the constant engine can reuse
	// the constant memory allocated to this draw call
	dcb->incrementDeCounter();
}


#ifndef SCE_GNM_HP3D
void ConstantUpdateEngine::preDispatch(DrawCommandBuffer *dcb, ConstantCommandBuffer *ccb)
{
	m_betweenPreAndPostDraw = true;

	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	SCE_GNM_VALIDATE(ccb, "ccb must not be NULL.");

	// Let the constant engine run ahead by n draw calls.  This is dependent on the size of the ring buffers.
#ifdef ENABLE_HARDWARE_KCACHE
	ccb->waitOnDeCounterDiff(m_chunksPerRingBuffer/4);
	ConstantCommandBuffer savedCcb = *ccb;
	savedCcb.m_cmdptr -= kWaitOnDeCounterDiffSizeInDword;
#else // ENABLE_HARDWARE_KCACHE
	ccb->waitOnDeCounterDiff(m_chunksPerRingBuffer);
#endif // ENABLE_HARDWARE_KCACHE

	if (m_dirtyShaders[Gnm::kShaderStageCs])
	{
		dcb->setCsShader(m_currentCSB ? &m_currentCSB->m_csStageRegisters : NULL);
	}
	// Don't clear shader dirty bits until after the calls to applyConstantsForDraw()

	void *usageChunkAddrs[16] = {0};
	const Gnm::InputUsageSlot *inputUsageSlots = m_currentCSB->getInputUsageSlotTable();
	updateCpRam(ccb, Gnm::kShaderStageCs, inputUsageSlots, m_currentCSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	applyConstantsForDraw(dcb, Gnm::kShaderStageCs, inputUsageSlots, m_currentCSB->m_common.m_numInputUsageSlots, usageChunkAddrs);

	// We can now safely clear the shader dirty bits
	m_dirtyShaders[Gnm::kShaderStageCs] = false;

	// Check if we need to update the global table:
	if ( m_globalTableNeedsUpdate )
	{
		SCE_GNM_VALIDATE(m_globalTableAddr, "Global table pointer not specified. Call setGlobalResourceTableAddr() first!");
		dcb->writeDataInlineThroughL2(m_globalTableAddr, m_globalTablePtr, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE/4, Gnm::kCachePolicyLru, Gnm::kWriteDataConfirmEnable);
		// Invalidate KCache:
		dcb->flushShaderCachesAndWait(kCacheActionNone,kExtendedCacheActionInvalidateKCache, kStallCommandBufferParserDisable);
		m_globalTableNeedsUpdate = false;
	}

	dcb->waitOnCe();
	ccb->incrementCeCounter();

#ifdef ENABLE_HARDWARE_KCACHE
	if ( !m_anyWrapped )
	{
		savedCcb.insertNop(kWaitOnDeCounterDiffSizeInDword);
	}

	m_anyWrapped = false;
#else // ENABLE_HARDWARE_KCACHE
	// Invalidate KCache:
	dcb->flushShaderCachesAndWait(Gnm::kWaitCacheFlushK);
#endif // ENABLE_HARDWARE_KCACHE
}


void ConstantUpdateEngine::postDispatch(DrawCommandBuffer *dcb, ConstantCommandBuffer *ccb)
{
	m_betweenPreAndPostDraw = false;

	SCE_GNM_UNUSED(ccb);
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	SCE_GNM_VALIDATE(ccb, "ccb must not be NULL.");

	// Inform the constant engine that this dispatch has ended so that the constant engine can reuse
	// the constant memory allocated to this dispatch
	dcb->incrementDeCounter();
}


void ConstantUpdateEngine::preDispatchDraw(DrawCommandBuffer *dcb, DispatchCommandBuffer *acb, ConstantCommandBuffer *ccb,
	DispatchOrderedAppendMode *pOut_orderedAppendMode, uint16_t *pOut_dispatchDrawIndexDeallocationMask, uint32_t *pOut_sgprKrbLoc, DispatchDrawMode *pOut_dispatchDrawMode, uint32_t *pOut_sgprVrbLoc)
{
	m_betweenPreAndPostDraw = true;

	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	SCE_GNM_VALIDATE(acb, "acb must not be NULL.");
	SCE_GNM_VALIDATE(ccb, "ccb must not be NULL.");

	// Let the constant engine run ahead by n draw calls.  This is dependent on the size of the ring buffers.
#ifdef ENABLE_HARDWARE_KCACHE
	ccb->waitOnDeCounterDiff(m_chunksPerRingBuffer/4);
	ConstantCommandBuffer savedCcb = *ccb;
	savedCcb.m_cmdptr -= kWaitOnDeCounterDiffSizeInDword;
#else // ENABLE_HARDWARE_KCACHE
	ccb->waitOnDeCounterDiff(m_chunksPerRingBuffer);
#endif // ENABLE_HARDWARE_KCACHE

	SCE_GNM_VALIDATE(m_activeShaderStages == Gnm::kActiveShaderStagesDispatchDrawVsPs, "setActiveShaderStages must be DispatchDrawVsPs");

#ifdef GNM_DEBUG
	if (m_currentVSB && m_dirtyShaders[Gnm::kShaderStageVs] && m_currentAcbCSB && m_dirtyShaders[kShaderStageAcbCs])
	{
		// Confirm that the ACB CS shader is compatible with the VS shader - all CS input usage slots which will reside in CUE ring buffers must match a VS input usage slot.
		const Gnm::InputUsageSlot *inputUsageSlotsCs = m_currentAcbCSB->getInputUsageSlotTable();
		const Gnm::InputUsageSlot *inputUsageSlotsVs = m_currentVSB->getInputUsageSlotTable();
		uint32_t iUsageVs = 0, iUsageVsMax = m_currentVSB->m_common.m_numInputUsageSlots;
		for(uint32_t iUsage=0, iUsageMax=m_currentAcbCSB->m_common.m_numInputUsageSlots; iUsage<iUsageMax; ++iUsage)
		{
			while (iUsageVs < iUsageVsMax && inputUsageSlotsVs[iUsageVs].m_startRegister < inputUsageSlotsCs[iUsage].m_startRegister)
				++iUsageVs;
			if (inputUsageSlotsCs[iUsage].m_startRegister >= 16
			|| (inputUsageSlotsCs[iUsage].m_usageType >= Gnm::kShaderInputUsagePtrResourceTable && inputUsageSlotsCs[iUsage].m_usageType <= Gnm::kShaderInputUsagePtrIndirectRwResourceTable)
			|| (inputUsageSlotsCs[iUsage].m_usageType == Gnm::kShaderInputUsagePtrDispatchDraw))
			{
				if (iUsageVs < iUsageVsMax && inputUsageSlotsCs[iUsage].m_startRegister == inputUsageSlotsVs[iUsageVs].m_startRegister) {
					if (inputUsageSlotsCs[iUsage].m_usageType != inputUsageSlotsVs[iUsageVs].m_usageType) {
						if (inputUsageSlotsCs[iUsage].m_startRegister < 16) {
							SCE_GNM_ERROR("DispatchDraw user data sgpr %u CsShader.inputUsageSlot[%u].m_usageType 0x%02x != VsShader.inputUsageSlot[%u].m_usageType 0x%02x",
								inputUsageSlotsCs[iUsage].m_startRegister,
								iUsage, inputUsageSlotsCs[iUsage].m_usageType,
								iUsageVs, inputUsageSlotsVs[iUsageVs].m_usageType);
						} else {
							SCE_GNM_ERROR("DispatchDraw extended user data dword %u CsShader.inputUsageSlot[%u].m_usageType 0x%02x != VsShader.inputUsageSlot[%u].m_usageType 0x%02x",
								inputUsageSlotsCs[iUsage].m_startRegister-16, iUsage, inputUsageSlotsCs[iUsage].m_usageType, iUsageVs, inputUsageSlotsVs[iUsageVs].m_usageType);
						}
					} else if (inputUsageSlotsCs[iUsage].m_usageType != Gnm::kShaderInputUsageImmShaderResourceTable) {
						if (inputUsageSlotsCs[iUsage].m_apiSlot != inputUsageSlotsVs[iUsageVs].m_apiSlot
							||	inputUsageSlotsCs[iUsage].m_registerCount != inputUsageSlotsVs[iUsageVs].m_registerCount
							||	inputUsageSlotsCs[iUsage].m_resourceType != inputUsageSlotsVs[iUsageVs].m_resourceType)
						{
							if (inputUsageSlotsCs[iUsage].m_startRegister < 16) {
								SCE_GNM_ERROR("DispatchDraw user data sgpr %u CsShader.inputUsageSlot[%u].{m_usageType 0x%02x, m_apiSlot %u, m_registerCount %u, m_resouceType %u} != VsShader.inputUsageSlot[%u].{m_usageType 0x%02x, m_apiSlot %u, m_registerCount %u, m_resouceType %u}",
									inputUsageSlotsCs[iUsage].m_startRegister,
									iUsage, inputUsageSlotsCs[iUsage].m_usageType, inputUsageSlotsCs[iUsage].m_apiSlot, inputUsageSlotsCs[iUsage].m_registerCount, inputUsageSlotsCs[iUsage].m_resourceType,
									iUsageVs, inputUsageSlotsVs[iUsageVs].m_usageType, inputUsageSlotsVs[iUsageVs].m_apiSlot, inputUsageSlotsVs[iUsageVs].m_registerCount, inputUsageSlotsVs[iUsageVs].m_resourceType);
							} else {
								SCE_GNM_ERROR("DispatchDraw extended user data dword %u CsShader.inputUsageSlot[%u].{m_usageType 0x%02x, m_apiSlot %u, m_registerCount %u, m_resouceType %u} != VsShader.inputUsageSlot[%u].{m_usageType 0x%02x, m_apiSlot %u, m_registerCount %u, m_resouceType %u}",
									inputUsageSlotsCs[iUsage].m_startRegister-16,
									iUsage, inputUsageSlotsCs[iUsage].m_usageType, inputUsageSlotsCs[iUsage].m_apiSlot, inputUsageSlotsCs[iUsage].m_registerCount, inputUsageSlotsCs[iUsage].m_resourceType,
									iUsageVs, inputUsageSlotsVs[iUsageVs].m_usageType, inputUsageSlotsVs[iUsageVs].m_apiSlot, inputUsageSlotsVs[iUsageVs].m_registerCount, inputUsageSlotsVs[iUsageVs].m_resourceType);
							}
						}
					} else {
						if (inputUsageSlotsCs[iUsage].m_srtSizeInDWordMinusOne != inputUsageSlotsVs[iUsageVs].m_srtSizeInDWordMinusOne) {
							if (inputUsageSlotsCs[iUsage].m_startRegister < 16) {
								SCE_GNM_ERROR("DispatchDraw user data sgpr %u CsShader.inputUsageSlot[%u].m_srtSizeInDWordMinusOne %u != VsShader.inputUsageSlot[%u].m_srtSizeInDWordMinusOne %u",
									inputUsageSlotsCs[iUsage].m_startRegister,
									iUsage, inputUsageSlotsCs[iUsage].m_srtSizeInDWordMinusOne,
									iUsageVs, inputUsageSlotsVs[iUsageVs].m_srtSizeInDWordMinusOne);
							} else {
								SCE_GNM_ERROR("DispatchDraw extended user data dword %u CsShader.inputUsageSlot[%u].m_srtSizeInDWordMinusOne %u != VsShader.inputUsageSlot[%u].m_srtSizeInDWordMinusOne %u",
									inputUsageSlotsCs[iUsage].m_startRegister-16,
									iUsage, inputUsageSlotsCs[iUsage].m_srtSizeInDWordMinusOne,
									iUsageVs, inputUsageSlotsVs[iUsageVs].m_srtSizeInDWordMinusOne);
							}
						}
					}
				} else {
					if (inputUsageSlotsCs[iUsage].m_startRegister < 16) {
						SCE_GNM_ERROR("DispatchDraw user data sgpr %u CsShader.inputUsageSlot[%u] not found in VsShader.inputUsageSlot[]", inputUsageSlotsCs[iUsage].m_startRegister, iUsage);
					} else {
						SCE_GNM_ERROR("DispatchDraw extended user data dword %u CsShader.inputUsageSlot[%u] not found in VsShader.inputUsageSlot[]", inputUsageSlotsCs[iUsage].m_startRegister-16, iUsage);
					}
				}
			}
		}
	}
#endif
	// Bind current shaders. This would be a great place for some validation.
	if (m_dirtyShaders[Gnm::kShaderStagePs])
	{
		dcb->setPsShader(m_currentPSB ? &m_currentPSB->m_psStageRegisters : NULL);
	}
	if ( m_currentVSB && m_dirtyShaders[Gnm::kShaderStageVs] )
	{
		dcb->setVsShader(&m_currentVSB->m_vsStageRegisters, m_currentVsShaderModifier);
		if ( m_currentVSFetchShaderAddr )
		{
			dcb->setPointerInUserData(Gnm::kShaderStageVs, 0, m_currentVSFetchShaderAddr); // Fetch shader is Hardcoded to register 0
		}
	}
	if (m_dirtyShaders[kShaderStageAcbCs])
	{
		//FIXME: should probably create a new setCsShader that accepts a shader modifier?
		if (m_currentAcbCsShaderModifier && m_currentAcbCSB) {
			CsStageRegisters csRegsCopy = m_currentAcbCSB->m_csStageRegisters;
			csRegsCopy.applyFetchShaderModifier(m_currentAcbCsShaderModifier);
			acb->setCsShader(&csRegsCopy);
		} else
			acb->setCsShader(m_currentAcbCSB ? &m_currentAcbCSB->m_csStageRegisters : NULL);
		if ( m_currentAcbCSFetchShaderAddr )
		{
			acb->setPointerInUserData(0, m_currentAcbCSFetchShaderAddr); // Fetch shader is Hardcoded to register 0
		}
	}
	// Don't clear shader dirty bits until after the calls to applyConstantsForDraw()

	// Set up VS to PS semantic mapping, but only if VS and PS stage shaders are set, and if one (or both) shaders are dirty
	if ((m_currentVSB != NULL && m_currentPSB != NULL) && (m_dirtyShaders[Gnm::kShaderStageVs] || m_dirtyShaders[Gnm::kShaderStagePs]))
	{
		uint32_t psInputs[32];
		Gnm::generatePsShaderUsageTable(psInputs,
			m_currentVSB->getExportSemanticTable(), m_currentVSB->m_numExportSemantics,
			m_currentPSB->getPixelInputSemanticTable(), m_currentPSB->m_numInputSemantics);
		if (m_currentPSB->m_numInputSemantics != 0)
		{
			dcb->setPsShaderUsage(psInputs, m_currentPSB->m_numInputSemantics);
		}
	}

	if (m_currentAcbCSB != NULL)
	{
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentAcbCSB->getInputUsageSlotTable();

		bool bFoundImmGdsKickRingBufferOffset = false;
		for (uint32_t iUsage = 0, iUsageMax = m_currentAcbCSB->m_common.m_numInputUsageSlots; iUsage < iUsageMax; ++iUsage) {
			if (inputUsageSlots[iUsage].m_startRegister >= 16)
				break;
			if (inputUsageSlots[iUsage].m_usageType == Gnm::kShaderInputUsageImmGdsKickRingBufferOffset) {
				*pOut_sgprKrbLoc = inputUsageSlots[iUsage].m_startRegister;
				bFoundImmGdsKickRingBufferOffset = true;
				break;
			}
		}
		*pOut_orderedAppendMode = (Gnm::DispatchOrderedAppendMode)m_currentAcbCSB->m_orderedAppendMode;
		*pOut_dispatchDrawIndexDeallocationMask = (uint16_t)(0xFFFF << m_currentAcbCSB->m_dispatchDrawIndexDeallocNumBits);
		SCE_GNM_VALIDATE(m_currentAcbCSB->m_orderedAppendMode == Gnm::kDispatchOrderedAppendModeIndexPerWavefront || m_currentAcbCSB->m_orderedAppendMode == Gnm::kDispatchOrderedAppendModeIndexPerThreadgroup, "dispatchDraw expects a CS shader with m_orderedAppendMode != kDispatchOrderedAppendModeDisabled");
		SCE_GNM_VALIDATE(m_currentAcbCSB->m_dispatchDrawIndexDeallocNumBits >= 1 || m_currentAcbCSB->m_dispatchDrawIndexDeallocNumBits <= 15, "dispatchDraw expects a CS shader with m_dispatchDrawIndexDeallocNumBits in the range [1:15]");
		SCE_GNM_VALIDATE(bFoundImmGdsKickRingBufferOffset, "dispatchDraw expects a CS shader with an InputUsageSlot of m_usageType kShaderInputUsageImmGdsKickRingBufferOffset in a real user sgpr");
	}
	if (m_currentVSB != NULL)
	{
		void *usageChunkAddrs[16] = {0};
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentVSB->getInputUsageSlotTable();

		bool bFoundImmVertexRingBufferOffset = false;
		*pOut_sgprVrbLoc = 0;
		for (uint32_t iUsage = 0, iUsageMax = m_currentCSB->m_common.m_numInputUsageSlots; iUsage < iUsageMax; ++iUsage) {
			if (inputUsageSlots[iUsage].m_startRegister >= 16)
				break;
			if (inputUsageSlots[iUsage].m_usageType == Gnm::kShaderInputUsageImmVertexRingBufferOffset) {
				*pOut_sgprVrbLoc = inputUsageSlots[iUsage].m_startRegister;
				bFoundImmVertexRingBufferOffset = true;
				break;
			}
		}
		*pOut_dispatchDrawMode = bFoundImmVertexRingBufferOffset ? Gnm::kDispatchDrawModeIndexAndVertexRingBuffer : Gnm::kDispatchDrawModeIndexRingBufferOnly;

		updateCpRam(ccb, Gnm::kShaderStageVs, inputUsageSlots, m_currentVSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		// dispatch draw CS shader shares all CUE ring buffer data with VS shader:
		if (m_currentAcbCSB != NULL)
			applyConstantsForDispatchDrawAcb(acb, m_currentAcbCSB->getInputUsageSlotTable(), m_currentAcbCSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		applyConstantsForDraw(dcb, Gnm::kShaderStageVs, inputUsageSlots, m_currentVSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	}
	if (m_currentPSB != NULL)
	{
		void *usageChunkAddrs[16] = {0};
		const Gnm::InputUsageSlot *inputUsageSlots = m_currentPSB->getInputUsageSlotTable();
		updateCpRam(ccb, Gnm::kShaderStagePs, inputUsageSlots, m_currentPSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
		applyConstantsForDraw(dcb, Gnm::kShaderStagePs, inputUsageSlots, m_currentPSB->m_common.m_numInputUsageSlots, usageChunkAddrs);
	}
	// We can now safely clear the shader dirty bits
	m_dirtyShaders[Gnm::kShaderStagePs] = false;
	m_dirtyShaders[Gnm::kShaderStageVs] = false;
	m_dirtyShaders[kShaderStageAcbCs] = false;

	// Check if we need to update the global table:
	if ( m_globalTableNeedsUpdate )
	{
		SCE_GNM_VALIDATE(m_globalTableAddr, "Global table pointer not specified. Call setGlobalResourceTableAddr() first!");
		dcb->writeDataInlineThroughL2(m_globalTableAddr, m_globalTablePtr, SCE_GNM_SHADER_GLOBAL_TABLE_SIZE/4, Gnm::kCachePolicyLru, Gnm::kWriteDataConfirmEnable);
		// Invalidate KCache:
		dcb->flushShaderCachesAndWait(kCacheActionNone, kExtendedCacheActionInvalidateKCache, kStallCommandBufferParserDisable);
		m_globalTableNeedsUpdate = false;
	}

	dcb->waitOnCe();
	acb->waitOnCe();
	ccb->incrementCeCounterForDispatchDraw();

#ifdef ENABLE_HARDWARE_KCACHE
	if ( !m_anyWrapped )
	{
		savedCcb.insertNop(kWaitOnDeCounterDiffSizeInDword);
	}

	m_anyWrapped = false;
#else // ENABLE_HARDWARE_KCACHE
	// Invalidate KCache:
	dcb->flushShaderCachesAndWait(Gnm::kWaitCacheFlushK);
#endif // ENABLE_HARDWARE_KCACHE
}

void ConstantUpdateEngine::postDispatchDraw(DrawCommandBuffer *dcb, DispatchCommandBuffer *acb, ConstantCommandBuffer *ccb)
{
	m_betweenPreAndPostDraw = false;

	SCE_GNM_UNUSED(ccb);
	SCE_GNM_VALIDATE(dcb, "dcb must not be NULL.");
	SCE_GNM_VALIDATE(acb, "acb must not be NULL.");
	SCE_GNM_VALIDATE(ccb, "ccb must not be NULL.");

	// Inform the constant engine that this draw has ended so that the constant engine can reuse
	// the constant memory allocated to this draw call
	dcb->incrementDeCounter();
}
#endif // SCE_GNM_HP3D
//

void ConstantUpdateEngine::setActiveResourceSlotCount(ShaderStage stage, uint32_t count)
{
	SCE_GNM_VALIDATE(count <= Gnm::kSlotCountResource, "count (%d) must be in the range [0..%d].", count, Gnm::kSlotCountResource);
	SCE_GNM_VALIDATE((uint32_t)stage < Gnm::kShaderStageCount, "invalid shader stage (%d).", stage);
	m_activeResourceSlotCounts[stage] = static_cast<uint8_t>(count);
}
void ConstantUpdateEngine::setActiveRwResourceSlotCount(ShaderStage stage, uint32_t count)
{
	SCE_GNM_VALIDATE(count <= Gnm::kSlotCountRwResource, "count (%d) must be in the range [0..%d].", count, Gnm::kSlotCountRwResource);
	SCE_GNM_VALIDATE((uint32_t)stage < Gnm::kShaderStageCount, "invalid shader stage (%d).", stage);
	m_activeRwResourceSlotCounts[stage] = static_cast<uint8_t>(count);
}
void ConstantUpdateEngine::setActiveSamplerSlotCount(ShaderStage stage, uint32_t count)
{
	SCE_GNM_VALIDATE(count <= Gnm::kSlotCountSampler, "count (%d) must be in the range [0..%d].", count, Gnm::kSlotCountSampler);
	SCE_GNM_VALIDATE((uint32_t)stage < Gnm::kShaderStageCount, "invalid shader stage (%d).", stage);
	m_activeSamplerSlotCounts[stage] = static_cast<uint8_t>(count);
}
void ConstantUpdateEngine::setActiveVertexBufferSlotCount(ShaderStage stage, uint32_t count)
{
	SCE_GNM_VALIDATE(count <= Gnm::kSlotCountVertexBuffer, "count (%d) must be in the range [0..%d].", count, Gnm::kSlotCountVertexBuffer);
	SCE_GNM_VALIDATE((uint32_t)stage < Gnm::kShaderStageCount, "invalid shader stage (%d).", stage);
	m_activeVertexBufferSlotCounts[stage] = static_cast<uint8_t>(count);
}
void ConstantUpdateEngine::setActiveConstantBufferSlotCount(ShaderStage stage, uint32_t count)
{
	SCE_GNM_VALIDATE(count <= Gnm::kSlotCountConstantBuffer, "count (%d) must be in the range [0..%d].", count, Gnm::kSlotCountConstantBuffer);
	SCE_GNM_VALIDATE((uint32_t)stage < Gnm::kShaderStageCount, "invalid shader stage (%d).", stage);
	m_activeConstantBufferSlotCounts[stage] = static_cast<uint8_t>(count);
}
void ConstantUpdateEngine::setActiveStreamoutBufferSlotCount(uint32_t count)
{
	SCE_GNM_VALIDATE(count <= Gnm::kSlotCountStreamoutBuffer, "count (%d) must be in the range [0..%d].", count, Gnm::kSlotCountStreamoutBuffer);
	m_activeStreamoutBufferSlotCount = static_cast<uint8_t>(count);
}

//

void ConstantUpdateEngine::setTsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Texture *tex)
{
	SCE_GNM_VALIDATE(tex->isTexture(), "tex (0x%010llX) is invalid (isTexture() returned false).", tex);
	const uint32_t numDwords = sizeof(Texture)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(dispatchDrawDword + numDwords <= Gnm::kDwordSizeDispatchDrawData, "dispatchDrawDword (%d) + numDwords (%d) must be <= %d.", dispatchDrawDword, numDwords, Gnm::kDwordSizeDispatchDrawData);
	uint32_t *destDdd = m_cpRamShadowDwords + getDispatchDrawDataDwordOffset(0);
	destDdd[dispatchDrawDword+ 0] = tex->m_regs[0];
	destDdd[dispatchDrawDword+ 1] = tex->m_regs[1];
	destDdd[dispatchDrawDword+ 2] = tex->m_regs[2];
	destDdd[dispatchDrawDword+ 3] = tex->m_regs[3];
	destDdd[dispatchDrawDword+ 4] = tex->m_regs[4];
	destDdd[dispatchDrawDword+ 5] = tex->m_regs[5];
	destDdd[dispatchDrawDword+ 6] = tex->m_regs[6];
	destDdd[dispatchDrawDword+ 7] = tex->m_regs[7];
	m_dirtyDddDwordRanges.m_firstDword       = std::min(m_dirtyDddDwordRanges.m_firstDword,       dispatchDrawDword);
	m_dirtyDddDwordRanges.m_lastDwordPlusOne = std::max(m_dirtyDddDwordRanges.m_lastDwordPlusOne, static_cast<uint8_t>(dispatchDrawDword+numDwords));
	// Mark the appropriate slots as dirty
	setBit(m_dirtyDispatchDrawDataSlotBits, 0);
	// Mark the appropriate chunks as dirty
	setBit(m_dirtyDispatchDrawDataChunkBits, 0);
}

void ConstantUpdateEngine::setSsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Sampler *sampler)
{
	const uint32_t numDwords = sizeof(Sampler)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(dispatchDrawDword + numDwords <= Gnm::kDwordSizeDispatchDrawData, "dispatchDrawDword (%d) + numDwords (%d) must be <= %d.", dispatchDrawDword, numDwords, Gnm::kDwordSizeDispatchDrawData);
	uint32_t *destDdd = m_cpRamShadowDwords + getDispatchDrawDataDwordOffset(0);
	destDdd[dispatchDrawDword+ 0] = sampler->m_regs[0];
	destDdd[dispatchDrawDword+ 1] = sampler->m_regs[1];
	destDdd[dispatchDrawDword+ 2] = sampler->m_regs[2];
	destDdd[dispatchDrawDword+ 3] = sampler->m_regs[3];
	m_dirtyDddDwordRanges.m_firstDword       = std::min(m_dirtyDddDwordRanges.m_firstDword,       dispatchDrawDword);
	m_dirtyDddDwordRanges.m_lastDwordPlusOne = std::max(m_dirtyDddDwordRanges.m_lastDwordPlusOne, static_cast<uint8_t>(dispatchDrawDword+numDwords));
	// Mark the appropriate slots as dirty
	setBit(m_dirtyDispatchDrawDataSlotBits, 0);
	// Mark the appropriate chunks as dirty
	setBit(m_dirtyDispatchDrawDataChunkBits, 0);
}

void ConstantUpdateEngine::setVsharpInDispatchDrawData(uint8_t dispatchDrawDword, const Buffer *buffer)
{
	SCE_GNM_VALIDATE(buffer->isBuffer(), "buffer (0x%010llX) is invalid (isBuffer() returned false).", buffer);
	const uint32_t numDwords = sizeof(Buffer)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(dispatchDrawDword + numDwords <= Gnm::kDwordSizeDispatchDrawData, "dispatchDrawDword (%d) + numDwords (%d) must be <= %d.", dispatchDrawDword, numDwords, Gnm::kDwordSizeDispatchDrawData);
	uint32_t *destDdd = m_cpRamShadowDwords + getDispatchDrawDataDwordOffset(0);
	destDdd[dispatchDrawDword+ 0] = buffer->m_regs[0];
	destDdd[dispatchDrawDword+ 1] = buffer->m_regs[1];
	destDdd[dispatchDrawDword+ 2] = buffer->m_regs[2];
	destDdd[dispatchDrawDword+ 3] = buffer->m_regs[3];
	m_dirtyDddDwordRanges.m_firstDword       = std::min(m_dirtyDddDwordRanges.m_firstDword,       dispatchDrawDword);
	m_dirtyDddDwordRanges.m_lastDwordPlusOne = std::max(m_dirtyDddDwordRanges.m_lastDwordPlusOne, static_cast<uint8_t>(dispatchDrawDword+numDwords));
	// Mark the appropriate slots as dirty
	setBit(m_dirtyDispatchDrawDataSlotBits, 0);
	// Mark the appropriate chunks as dirty
	setBit(m_dirtyDispatchDrawDataChunkBits, 0);
}

void ConstantUpdateEngine::setPointerInDispatchDrawData(uint8_t dispatchDrawDword, void *gpuAddr)
{
	const uint32_t numDwords = sizeof(void*)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(dispatchDrawDword + numDwords <= Gnm::kDwordSizeDispatchDrawData, "dispatchDrawDword (%d) + numDwords (%d) must be <= %d.", dispatchDrawDword, numDwords, Gnm::kDwordSizeDispatchDrawData);
	uint32_t *destDdd = m_cpRamShadowDwords + getDispatchDrawDataDwordOffset(0);
	destDdd[dispatchDrawDword+ 0] = (uint32_t)(uintptr_t(gpuAddr)&0xFFFFFFFF);
	destDdd[dispatchDrawDword+ 1] = (uint32_t)(uintptr_t(gpuAddr)>>32);
	m_dirtyDddDwordRanges.m_firstDword       = std::min(m_dirtyDddDwordRanges.m_firstDword,       dispatchDrawDword);
	m_dirtyDddDwordRanges.m_lastDwordPlusOne = std::max(m_dirtyDddDwordRanges.m_lastDwordPlusOne, static_cast<uint8_t>(dispatchDrawDword+numDwords));
	// Mark the appropriate slots as dirty
	setBit(m_dirtyDispatchDrawDataSlotBits, 0);
	// Mark the appropriate chunks as dirty
	setBit(m_dirtyDispatchDrawDataChunkBits, 0);
}

void ConstantUpdateEngine::setDwordInDispatchDrawData(uint8_t dispatchDrawDword, uint32_t data)
{
	const uint32_t numDwords = sizeof(uint32_t)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(dispatchDrawDword + numDwords <= Gnm::kDwordSizeDispatchDrawData, "dispatchDrawDword (%d) + numDwords (%d) must be <= %d.", dispatchDrawDword, numDwords, Gnm::kDwordSizeDispatchDrawData);
	uint32_t *destDdd = m_cpRamShadowDwords + getDispatchDrawDataDwordOffset(0);
	destDdd[dispatchDrawDword+ 0] = data;
	m_dirtyDddDwordRanges.m_firstDword       = std::min(m_dirtyDddDwordRanges.m_firstDword,       dispatchDrawDword);
	m_dirtyDddDwordRanges.m_lastDwordPlusOne = std::max(m_dirtyDddDwordRanges.m_lastDwordPlusOne, static_cast<uint8_t>(dispatchDrawDword+numDwords));
	// Mark the appropriate slots as dirty
	setBit(m_dirtyDispatchDrawDataSlotBits, 0);
	// Mark the appropriate chunks as dirty
	setBit(m_dirtyDispatchDrawDataChunkBits, 0);
}

void ConstantUpdateEngine::setDwordMaskedInDispatchDrawData(uint8_t dispatchDrawDword, uint32_t data, uint32_t mask)
{
	const uint32_t numDwords = sizeof(uint32_t)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(dispatchDrawDword + numDwords <= Gnm::kDwordSizeDispatchDrawData, "dispatchDrawDword (%d) + numDwords (%d) must be <= %d.", dispatchDrawDword, numDwords, Gnm::kDwordSizeDispatchDrawData);
	uint32_t *destDdd = m_cpRamShadowDwords + getDispatchDrawDataDwordOffset(0);
	destDdd[dispatchDrawDword+ 0] = (destDdd[dispatchDrawDword+ 0] & ~mask) | (data & mask);
	m_dirtyDddDwordRanges.m_firstDword       = std::min(m_dirtyDddDwordRanges.m_firstDword,       dispatchDrawDword);
	m_dirtyDddDwordRanges.m_lastDwordPlusOne = std::max(m_dirtyDddDwordRanges.m_lastDwordPlusOne, static_cast<uint8_t>(dispatchDrawDword+numDwords));
	// Mark the appropriate slots as dirty
	setBit(m_dirtyDispatchDrawDataSlotBits, 0);
	// Mark the appropriate chunks as dirty
	setBit(m_dirtyDispatchDrawDataChunkBits, 0);
}

//

uint16_t ConstantUpdateEngine::getResourceDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountResource, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountResource-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetResource;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeResource));
}
uint16_t ConstantUpdateEngine::getRwResourceDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountRwResource, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountRwResource-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetRwResource;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeRwResource));
}
uint16_t ConstantUpdateEngine::getSamplerDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountSampler, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountSampler-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetSampler;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeSampler));
}
uint16_t ConstantUpdateEngine::getConstantBufferDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountConstantBuffer, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountConstantBuffer-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetConstantBuffer;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeConstantBuffer));
}
uint16_t ConstantUpdateEngine::getVertexBufferDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountVertexBuffer, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountVertexBuffer-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetVertexBuffer;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeVertexBuffer));
}
uint16_t ConstantUpdateEngine::getBoolConstantDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountBoolConstant, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountBoolConstant-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetBoolConstant;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeBoolConstant));
}
uint16_t ConstantUpdateEngine::getFloatConstantDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountFloatConstant, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountFloatConstant-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetFloatConstant;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeFloatConstant));
}
uint16_t ConstantUpdateEngine::getAppendConsumeCounterRangeDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountAppendConsumeCounterRange, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountAppendConsumeCounterRange-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetAppendConsumeGdsCounterRange;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeAppendConsumeCounterRange));
}
uint16_t ConstantUpdateEngine::getStreamoutDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountResource, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountAppendConsumeCounterRange-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetStreamoutBuffer;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeStreamoutBuffer));
}
uint16_t ConstantUpdateEngine::getExtendedUserDataDwordOffset(ShaderStage stage, uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountExtendedUserData, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountExtendedUserData-1);
	uint32_t stageOffset = stage*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetExtendedUserData;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeExtendedUserData));
}
uint16_t ConstantUpdateEngine::getDispatchDrawDataDwordOffset(uint32_t slot) const
{
	SCE_GNM_VALIDATE(slot < Gnm::kSlotCountDispatchDrawData, "slot (%d) must be in the range [0..%d].", slot, Gnm::kSlotCountDispatchDrawData-1);
	uint32_t stageOffset = Gnm::kShaderStageVs*kPerStageDwordSize;
	uint32_t typeOffset = kDwordOffsetDispatchDrawData;
	return (uint16_t)(stageOffset + typeOffset + (slot*Gnm::kDwordSizeDispatchDrawData));
}

void ConstantUpdateEngine::writeDirtyRangesToCpRam(ConstantCommandBuffer *ccb, uint8_t *dirtyBits, int32_t firstBitIndex, int32_t bitCount, uint16_t baseByteOffset, uint32_t recordBytes)
{
	int32_t bitIndex = firstBitIndex;
	int32_t rangeStart = -1;
	while(bitIndex < firstBitIndex+bitCount)
	{
		int32_t byteIndex = bitIndex / 8;
		int32_t bitWithinByte = bitIndex % 8;
		bool insideRange = (rangeStart>=0);
		// Shortcut: if the rest of the byte is empty/full, skip to the end of it
		uint8_t shortcutMask = 0xFF << bitWithinByte;
		uint8_t targetValue = insideRange ? shortcutMask : 0;
		if ((dirtyBits[byteIndex] & shortcutMask) == targetValue)
		{
			dirtyBits[byteIndex] &= ~shortcutMask; // zero out the bits
			bitIndex = (byteIndex+1)*8;
			continue;
		}
		// Test the current bit
		bool bitIsSet = testBit(dirtyBits, bitIndex);//(dirtyBits[byteIndex] & (1<<bitWithinByte)) != 0;
		if (bitIsSet && !insideRange)
			rangeStart = bitIndex; // start a new range
		else if (!bitIsSet && insideRange)
		{
			// End range, and dump these resources
			int32_t rangeBitCount = bitIndex-rangeStart;
			uint16_t rangeByteOffset = (uint16_t)(baseByteOffset + rangeStart*recordBytes);
			ccb->writeToCpRam(rangeByteOffset, (void*)&m_cpRamShadowDwords[rangeByteOffset/sizeof(uint32_t)], rangeBitCount*recordBytes/sizeof(uint32_t));
			rangeStart = -1;
		}
		// Clear the bit after we test, regardless
		clearBit(dirtyBits, bitIndex);//dirtyBits[byteIndex] &= ~(1<<bitWithinByte));
		// Advance
		++bitIndex;
	}
	// Send one last range, if necessary
	if (rangeStart >= 0)
	{
		// End range, and dump these resources
		int32_t rangeBitCount = bitIndex-rangeStart;
		uint16_t rangeByteOffset = (uint16_t)(baseByteOffset + rangeStart*recordBytes);
		ccb->writeToCpRam(rangeByteOffset, (void*)&m_cpRamShadowDwords[rangeByteOffset/sizeof(uint32_t)], rangeBitCount*recordBytes/sizeof(uint32_t));
		rangeStart = -1;
	}
}

// Wrappers around dcb->set*UserData() that work with extended user data as well
void ConstantUpdateEngine::setTsharpInExtendedUserData(ShaderStage stage, uint8_t userDataSlot, const Texture *tex)
{
	//Not necessarily valid. See: https://ps4.scedev.net/support/issue/22117
	//SCE_GNM_VALIDATE(tex->isTexture(), "tex (0x%010llX) is invalid (isTexture() returned false).", tex);
	const uint32_t numDwords = sizeof(Texture)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(userDataSlot >= 16, "userDataSlot (%d) must be >= 16.", userDataSlot);
	SCE_GNM_VALIDATE(userDataSlot + numDwords <= Gnm::kDwordSizeExtendedUserData+16, "userDataSlot (%d) + numDwords (%d) must be <= %d.", userDataSlot, numDwords, Gnm::kDwordSizeExtendedUserData+16);
	bool isSrtShader = m_isSrtShader[stage];
	uint32_t *destEud = isSrtShader ? (uint32_t*)m_internalSrtBuffers[stage] : (m_cpRamShadowDwords + getExtendedUserDataDwordOffset(stage, 0));
	destEud[userDataSlot-16+ 0] = tex->m_regs[0];
	destEud[userDataSlot-16+ 1] = tex->m_regs[1];
	destEud[userDataSlot-16+ 2] = tex->m_regs[2];
	destEud[userDataSlot-16+ 3] = tex->m_regs[3];
	destEud[userDataSlot-16+ 4] = tex->m_regs[4];
	destEud[userDataSlot-16+ 5] = tex->m_regs[5];
	destEud[userDataSlot-16+ 6] = tex->m_regs[6];
	destEud[userDataSlot-16+ 7] = tex->m_regs[7];
	if (!isSrtShader)
	{
		m_dirtyEudSlotRanges[stage].m_firstSlot       = std::min(m_dirtyEudSlotRanges[stage].m_firstSlot,       userDataSlot);
		m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne = std::max(m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne, static_cast<uint8_t>(userDataSlot+numDwords));
	}
}

void ConstantUpdateEngine::setSsharpInExtendedUserData(ShaderStage stage, uint8_t userDataSlot, const Sampler *sampler)
{
	const uint32_t numDwords = sizeof(Sampler)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(userDataSlot >= 16, "userDataSlot (%d) must be >= 16.", userDataSlot);
	SCE_GNM_VALIDATE(userDataSlot + numDwords <= Gnm::kDwordSizeExtendedUserData+16, "userDataSlot (%d) + numDwords (%d) must be <= %d.", userDataSlot, numDwords, Gnm::kDwordSizeExtendedUserData+16);
	bool isSrtShader = m_isSrtShader[stage];
	uint32_t *destEud = isSrtShader ? (uint32_t*)m_internalSrtBuffers[stage] : (m_cpRamShadowDwords + getExtendedUserDataDwordOffset(stage, 0));
	destEud[userDataSlot-16+ 0] = sampler->m_regs[0];
	destEud[userDataSlot-16+ 1] = sampler->m_regs[1];
	destEud[userDataSlot-16+ 2] = sampler->m_regs[2];
	destEud[userDataSlot-16+ 3] = sampler->m_regs[3];
	if (!isSrtShader)
	{
		m_dirtyEudSlotRanges[stage].m_firstSlot       = std::min(m_dirtyEudSlotRanges[stage].m_firstSlot,       userDataSlot);
		m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne = std::max(m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne, static_cast<uint8_t>(userDataSlot+numDwords));
	}
}

void ConstantUpdateEngine::setVsharpInExtendedUserData(ShaderStage stage, uint8_t userDataSlot, const Buffer *buffer)
{
	//Not necessarily valid. See: https://ps4.scedev.net/support/issue/22117
	//SCE_GNM_VALIDATE(buffer->isBuffer(), "buffer (0x%010llX) is invalid (isBuffer() returned false).", buffer);
	const uint32_t numDwords = sizeof(Buffer)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(userDataSlot >= 16, "userDataSlot (%d) must be >= 16.", userDataSlot);
	SCE_GNM_VALIDATE(userDataSlot + numDwords <= Gnm::kDwordSizeExtendedUserData+16, "userDataSlot (%d) + numDwords (%d) must be <= %d.", userDataSlot, numDwords, Gnm::kDwordSizeExtendedUserData+16);
	bool isSrtShader = m_isSrtShader[stage];
	uint32_t *destEud = isSrtShader ? (uint32_t*)m_internalSrtBuffers[stage] : (m_cpRamShadowDwords + getExtendedUserDataDwordOffset(stage, 0));
	destEud[userDataSlot-16+ 0] = buffer->m_regs[0];
	destEud[userDataSlot-16+ 1] = buffer->m_regs[1];
	destEud[userDataSlot-16+ 2] = buffer->m_regs[2];
	destEud[userDataSlot-16+ 3] = buffer->m_regs[3];
	if (!isSrtShader)
	{
		m_dirtyEudSlotRanges[stage].m_firstSlot       = std::min(m_dirtyEudSlotRanges[stage].m_firstSlot,       userDataSlot);
		m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne = std::max(m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne, static_cast<uint8_t>(userDataSlot+numDwords));
	}
}

void ConstantUpdateEngine::setPointerInExtendedUserData(ShaderStage stage, uint8_t userDataSlot, void *gpuAddr)
{
	const uint32_t numDwords = sizeof(void*)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(userDataSlot >= 16, "userDataSlot (%d) must be >= 16.", userDataSlot);
	SCE_GNM_VALIDATE(userDataSlot + numDwords <= Gnm::kDwordSizeExtendedUserData+16, "userDataSlot (%d) + numDwords (%d) must be <= %d.", userDataSlot, numDwords, Gnm::kDwordSizeExtendedUserData+16);
	bool isSrtShader = m_isSrtShader[stage];
	uint32_t *destEud = isSrtShader ? (uint32_t*)m_internalSrtBuffers[stage] : (m_cpRamShadowDwords + getExtendedUserDataDwordOffset(stage, 0));
	destEud[userDataSlot-16+ 0] = (uint32_t)(uintptr_t(gpuAddr)&0xFFFFFFFF);
	destEud[userDataSlot-16+ 1] = (uint32_t)(uintptr_t(gpuAddr)>>32);
	if (!isSrtShader)
	{
		m_dirtyEudSlotRanges[stage].m_firstSlot       = std::min(m_dirtyEudSlotRanges[stage].m_firstSlot,       userDataSlot);
		m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne = std::max(m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne, static_cast<uint8_t>(userDataSlot+numDwords));
	}
}

void ConstantUpdateEngine::setDwordInExtendedUserData(ShaderStage stage, uint8_t userDataSlot, uint32_t data)
{
	const uint32_t numDwords = sizeof(uint32_t)/sizeof(uint32_t);
	SCE_GNM_VALIDATE(userDataSlot >= 16, "userDataSlot (%d) must be >= 16.", userDataSlot);
	SCE_GNM_VALIDATE(userDataSlot + numDwords <= Gnm::kDwordSizeExtendedUserData+16, "userDataSlot (%d) + numDwords (%d) must be <= %d.", userDataSlot, numDwords, Gnm::kDwordSizeExtendedUserData+16);
	bool isSrtShader = m_isSrtShader[stage];
	uint32_t *destEud = isSrtShader ? (uint32_t*)m_internalSrtBuffers[stage] : (m_cpRamShadowDwords + getExtendedUserDataDwordOffset(stage, 0));
	destEud[userDataSlot-16+ 0] = data;
	if (!isSrtShader)
	{
		m_dirtyEudSlotRanges[stage].m_firstSlot       = std::min(m_dirtyEudSlotRanges[stage].m_firstSlot,       userDataSlot);
		m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne = std::max(m_dirtyEudSlotRanges[stage].m_lastSlotPlusOne, static_cast<uint8_t>(userDataSlot+numDwords));
	}
}
void ConstantUpdateEngine::setVertexAndInstanceOffset(sce::Gnm::DrawCommandBuffer *dcb, uint32_t vertexOffset, uint32_t instanceOffset)
{
	ShaderStage fetchShaderStage = Gnm::kShaderStageCount; // initialize with an invalid value
	uint32_t vertexOffsetUserSlot = 0;
	uint32_t instanceOffsetUserSlot = 0;

	// find where the fetch shader is 
	switch(m_activeShaderStages)
	{
		case Gnm::kActiveShaderStagesVsPs:
			SCE_GNM_ASSERT(m_currentVSB);
			fetchShaderStage= Gnm::kShaderStageVs;
			vertexOffsetUserSlot = m_currentVSB->getVertexOffsetUserRegister();
			instanceOffsetUserSlot = m_currentVSB->getInstanceOffsetUserRegister();
			break;
		case Gnm::kActiveShaderStagesEsGsVsPs:
			SCE_GNM_ASSERT(m_currentESB);
			fetchShaderStage = Gnm::kShaderStageEs;
			vertexOffsetUserSlot = m_currentESB->getVertexOffsetUserRegister();
			instanceOffsetUserSlot = m_currentESB->getInstanceOffsetUserRegister();
			break;
		case Gnm::kActiveShaderStagesLsHsEsGsVsPs:
		case Gnm::kActiveShaderStagesLsHsVsPs:
			SCE_GNM_ASSERT(m_currentLSB);
			fetchShaderStage = Gnm::kShaderStageLs;
			vertexOffsetUserSlot = m_currentLSB->getVertexOffsetUserRegister();
			instanceOffsetUserSlot = m_currentLSB->getInstanceOffsetUserRegister();
			break;
		case kActiveShaderStagesDispatchDrawVsPs:
			SCE_GNM_ERROR("Current shader configuration does not support vertex/instance offset");
	}
	if (fetchShaderStage != kShaderStageCount)
	{
		if( vertexOffsetUserSlot )
			dcb->setUserData(fetchShaderStage,vertexOffsetUserSlot,vertexOffset);
		if( instanceOffsetUserSlot )
			dcb->setUserData(fetchShaderStage,instanceOffsetUserSlot, instanceOffset);
	}
}
bool ConstantUpdateEngine::isVertexOrInstanceOffsetEnabled() const
{
	uint32_t vertexOffsetUserSlot = 0;
	uint32_t instanceOffsetUserSlot = 0;

	// find where the fetch shader is 
	switch(m_activeShaderStages)
	{
	case Gnm::kActiveShaderStagesVsPs:
		SCE_GNM_ASSERT(m_currentVSB);
		vertexOffsetUserSlot = m_currentVSB->getVertexOffsetUserRegister();
		instanceOffsetUserSlot = m_currentVSB->getInstanceOffsetUserRegister();
		break;
	case Gnm::kActiveShaderStagesEsGsVsPs:
		SCE_GNM_ASSERT(m_currentESB);
		vertexOffsetUserSlot = m_currentESB->getVertexOffsetUserRegister();
		instanceOffsetUserSlot = m_currentESB->getInstanceOffsetUserRegister();
		break;
	case Gnm::kActiveShaderStagesLsHsEsGsVsPs:
	case Gnm::kActiveShaderStagesLsHsVsPs:
		SCE_GNM_ASSERT(m_currentLSB);
		vertexOffsetUserSlot = m_currentLSB->getVertexOffsetUserRegister();
		instanceOffsetUserSlot = m_currentLSB->getInstanceOffsetUserRegister();
		break;
	case kActiveShaderStagesDispatchDrawVsPs:
		return false;
	}
	return vertexOffsetUserSlot || instanceOffsetUserSlot;
}
#endif // !CUE_V2
