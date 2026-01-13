// 
// grcore/indexbuffer_gcm.cpp 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/indexbuffer.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"

#include <string.h>

#if __GCM || __WIN32PC

#include "wrapper_gcm.h"

namespace rage
{

grcIndexBufferGCM::grcIndexBufferGCM(int indexCount, bool dynamic, void* preAllocatedMemory) : grcIndexBuffer(indexCount)
{
	if ( preAllocatedMemory )
	{
		Assert( gcm::IsLocalPtr( preAllocatedMemory  )|| gcm::IsMainPtr( preAllocatedMemory) );
		Assert( gcm::IsLocalPtr( preAllocatedMemory  )|| dynamic );
		Assert( gcm::IsLocalPtr( (u8*)(preAllocatedMemory) + sizeof(u16) * indexCount -1 )|| gcm::IsMainPtr( (u8*)(preAllocatedMemory) + sizeof(u16) * indexCount -1 ) );

		MarkPreallocated();
		m_IndexData = (u16*) preAllocatedMemory;
	}
	else
	{
		m_IndexData = (u16*) gcm::AllocatePtr(indexCount * sizeof(u16),dynamic);
	}
#if __PPU
	m_Offset = gcm::EncodeOffset(m_IndexData);
#else
	m_Offset = 0;
#endif
}

grcIndexBufferGCM::grcIndexBufferGCM(datResource& rsc) : grcIndexBuffer(rsc)
{
#if __PPU
	m_Offset = gcm::EncodeOffset(m_IndexData);
#else
	m_Offset = 0;
#endif
}

grcIndexBufferGCM::~grcIndexBufferGCM()
{
	if (m_IndexData) {
		if (!IsPreallocatedMemory())
			gcm::FreePtr(m_IndexData);
		m_IndexData = NULL;
	}
}

#if __DECLARESTRUCT
void grcIndexBufferGCM::DeclareStruct(datTypeStruct &s)
{
	grcIndexBuffer::DeclareStruct(s);
	STRUCT_BEGIN(grcIndexBufferGCM);
	STRUCT_IGNORE(m_Offset);
	STRUCT_END();
}
#endif

} // namespace rage

#endif // __OPENGL
