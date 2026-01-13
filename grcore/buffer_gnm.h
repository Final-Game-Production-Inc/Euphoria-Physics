// 
// grcore/buffer_gnm.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_BUFFER_GNM_H
#define GRCORE_BUFFER_GNM_H

#include "grcore/config.h"

#if __GNM

#include <stdint.h>
#include <gnm/buffer.h>

#include "wrapper_gnm.h"

#define BUFFER_ELEMENT_TYPE	uint32_t

namespace rage {

enum grcBufferType
{
	grcBuffer_Raw,
	grcBuffer_Typed,
	grcBuffer_Structured,
};

class grcBufferGNM	{
	grcBufferGNM(const grcBufferGNM&);
	void operator=(const grcBufferGNM&);
public:
	const grcBufferType m_Type;
	u32	m_GdsCounterOffset;
	
	grcBufferGNM(grcBufferType type, bool bOwn);
	~grcBufferGNM();

	void Initialize(void *const address, u32 numElements, bool bGpuWrite, u32 stride = 0, const sce::Gnm::DataFormat &format = sce::Gnm::kDataFormatInvalid);
	void Allocate(const sce::Gnm::SizeAlign &sa, bool bGpuWrite, const void *pExistingData = NULL, u32 stride = 0, const sce::Gnm::DataFormat &format = sce::Gnm::kDataFormatInvalid);
	void CleanUp();

	const sce::Gnm::Buffer* GetUnorderedAccessView() const { Assert(m_GnmBuffer.isBuffer()); return &m_GnmBuffer; }
	void*		GetAddress()	{ Assert(m_GnmBuffer.isBuffer()); return m_BaseAddress; }
	uint32_t	GetSize()		{ Assert(m_GnmBuffer.isBuffer()); return m_GnmBuffer.getSize(); }
	
private:
	sce::Gnm::Buffer m_GnmBuffer;
	const bool m_bOwn;
	void *m_BaseAddress;
};

//Simple interface to allow default construction of grcBufferGNM buffers. Useful if you need to construct an array of them.
template<class T, int bufferType, bool own>
class grcBufferConstructable : public T
{
public:
	typedef T parent_type;
	grcBufferConstructable() : parent_type(static_cast<grcBufferType>(bufferType), own) {}
};

}	// namespace rage

#endif		// __GNM

#endif	//GRCORE_BUFFER_GNM_H