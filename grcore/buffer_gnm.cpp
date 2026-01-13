// 
// grcore/texture_gnm.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 
#if RSG_ORBIS
#include "buffer_gnm.h"

namespace rage {

grcBufferGNM::grcBufferGNM(grcBufferType type, bool bOwn)
: m_Type(type), m_GdsCounterOffset(0), m_bOwn(bOwn), m_BaseAddress(NULL)
{
	memset( &m_GnmBuffer, 0, sizeof(m_GnmBuffer) );
}

void grcBufferGNM::Initialize(void *const address, u32 numElements, bool bGpuWrite, u32 stride, const sce::Gnm::DataFormat &format)
{
	switch (m_Type)
	{
	case grcBuffer_Raw:
		Assert( !stride || stride == 1 );
		m_GnmBuffer.initAsByteBuffer( address, numElements );
		break;
	case grcBuffer_Typed:
		Assert( format.isValid() && format.supportsBuffer() );
		Assert( !stride || stride == format.getBytesPerElement() );
		m_GnmBuffer.initAsDataBuffer( address, format, numElements );
		break;
	case grcBuffer_Structured:
		m_GnmBuffer.initAsRegularBuffer( address, stride, numElements );
		break;
	}

	m_BaseAddress = address;
	m_GnmBuffer.setResourceMemoryType( bGpuWrite ? sce::Gnm::kResourceMemoryTypeGC : sce::Gnm::kResourceMemoryTypeRO );
}

void grcBufferGNM::Allocate(const sce::Gnm::SizeAlign &sa, bool bGpuWrite, const void *pExistingData, u32 stride, const sce::Gnm::DataFormat &format)
{
	Assert( m_bOwn && !m_BaseAddress );

	uint nElem = 0;
	switch (m_Type)
	{
	case grcBuffer_Raw:
		nElem = sa.m_size;
		break;
	case grcBuffer_Typed:
		nElem = sa.m_size / format.getBytesPerElement();
		break;
	case grcBuffer_Structured:
		nElem = sa.m_size / stride;
		break;
	}

	void *const addr = allocateVideoPrivateMemory( sa.m_size, sa.m_align );
	if (pExistingData)
	{
		memcpy( addr, pExistingData, sa.m_size );
		//gfxc.copyData( addr, pExistingData, sa.m_size, sce::Gnm::kDmaDataBlockingDisable );
	}
	
	Initialize( addr, nElem, bGpuWrite, stride, format );
	Assert( GetSize() == sa.m_size );
}

void grcBufferGNM::CleanUp()
{
	if (m_bOwn && m_GnmBuffer.isBuffer())
	{
		freeVideoPrivateMemory( m_GnmBuffer.getBaseAddress() );
		m_GnmBuffer.initAsByteBuffer( NULL, 0 );
	}
}

grcBufferGNM::~grcBufferGNM()
{
	CleanUp();
}

}	//rage
#endif	//RSG_ORBIS