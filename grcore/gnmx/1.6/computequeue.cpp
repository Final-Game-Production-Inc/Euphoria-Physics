/* SCE CONFIDENTIAL
PlayStation(R)4 Programmer Tool Runtime Library Release 01.600.051
* Copyright (C) 2011 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/


#include <gnm.h>
#include "grcore/gnmx/computequeue.h"
#include "grcore/gnmx/computecontext.h"

using namespace sce::Gnm;
using namespace sce::Gnmx;

void ComputeQueue::initialize(uint32_t pipeId, uint32_t queueId)
{
	m_pipeId  = pipeId;
	m_queueId = queueId;

	m_vqueueId	  = 0;
}

#ifndef SCE_GNM_OFFLINE_MODE
bool ComputeQueue::map(void *ringBaseAddr, uint32_t ringSizeInDW, void *readPtrAddr)
{
	SCE_GNM_VALIDATE((uintptr_t(ringBaseAddr)&0xFF)==0, "Ring Base Address (0x%010llX) must be 256 bytes aligned.", ringBaseAddr);
	SCE_GNM_VALIDATE((uintptr_t(readPtrAddr)&0x3)==0, "Read Ptr Address (0x%010llX) must be 4 bytes aligned.", readPtrAddr);
	SCE_GNM_VALIDATE((ringSizeInDW & (ringSizeInDW-1)) == 0 && ringSizeInDW*4 >= 512, "Ring Size must be a power of 2 and at least 512");

	const int status = sce::Gnm::mapComputeQueue(&m_vqueueId, 
												 m_pipeId, m_queueId,
												 ringBaseAddr, ringSizeInDW, readPtrAddr);
	SCE_GNM_VALIDATE(!status, "sce::Gnm::mapComputeQueue() failed [err: %i].\n", status);
	SCE_GNM_UNUSED(status);

	m_dcbRoot.init((void*)ringBaseAddr, ringSizeInDW*4, 0, 0);
	m_readPtrAddr = (volatile uint32_t*)readPtrAddr;

	return m_vqueueId != 0;
}

void ComputeQueue::unmap()
{
	sce::Gnm::unmapComputeQueue(m_vqueueId);
	m_vqueueId = 0;
}

ComputeQueue::SubmissionStatus ComputeQueue::submit(uint32_t numBuffers, void const*const *apDcb, uint32_t const *aDcbSizes)
{
	// TODO: Make sure the Ring doesn't overlap too fast. (check readPtr)
	const uint32_t kSizeOfIbPacketInDWords = 4;

	// Check if queue is full:
	const uint32_t ringSizeInDWord = m_dcbRoot.m_endptr - m_dcbRoot.m_beginptr;
	const uint32_t readOffset      = *m_readPtrAddr;
	const uint32_t curOffset       = m_dcbRoot.m_cmdptr - m_dcbRoot.m_beginptr;
	const  int32_t relOffset       = readOffset - curOffset;
	const uint32_t adjOffset       = relOffset + ((relOffset >> 31) & ringSizeInDWord);
	if ( adjOffset && adjOffset < (1 + numBuffers) * kSizeOfIbPacketInDWords )
		return kSubmitFailQueueIsFull;

	// Wrap check:
	const uint32_t ringSpaceRemainingInDW = m_dcbRoot.m_endptr - m_dcbRoot.m_cmdptr;
	if ( ringSpaceRemainingInDW < kSizeOfIbPacketInDWords )
	{
		m_dcbRoot.insertNop(ringSpaceRemainingInDW);
		m_dcbRoot.resetBuffer();
	}
	for (uint32_t iSubmit = 0; iSubmit < numBuffers; ++iSubmit)
		m_dcbRoot.callCommandBuffer((void*)apDcb[iSubmit], aDcbSizes[iSubmit]/4);
	if ( m_dcbRoot.m_endptr == m_dcbRoot.m_cmdptr )
		m_dcbRoot.resetBuffer();

	sce::Gnm::dingDong(m_vqueueId, 	m_dcbRoot.m_cmdptr - m_dcbRoot.m_beginptr);
	return kSubmitOK;
}

ComputeQueue::SubmissionStatus ComputeQueue::submit(Gnmx::ComputeContext *cmpc)
{
	return submit(cmpc->m_dcb.m_beginptr, (uint32_t)(cmpc->m_dcb.m_cmdptr - cmpc->m_dcb.m_beginptr)*4);
}

#endif // !SCE_GNM_OFFLINE_MODE
