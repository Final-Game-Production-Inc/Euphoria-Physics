// 
// grcore/vertexbuffer_gcm.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/vertexbuffer.h"
#include "grcore/device.h"
#include "grcore/fvf.h"
#include "mesh/mesh.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "data/safestruct.h"
#include "atl/array_struct.h"

#include "system/cache.h"
#include "system/memory.h"
#include "system/ipc.h"
#include "system/xtl.h"
#include "diag/tracker.h"

#include "profile/page.h"
#include "profile/group.h"

#include "math/float16.h"

#if __GCM || __WIN32PC

#include "wrapper_gcm.h"

namespace rage
{

#if !__SPU

grcVertexBufferGCM::grcVertexBufferGCM(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, u8* preAllocatedMemory )
:	grcVertexBuffer(vertCount, pFvf, bReadWrite, bDynamic, preAllocatedMemory)
{
	if ( preAllocatedMemory )
	{
		Assert( gcm::IsLocalPtr( preAllocatedMemory  )|| gcm::IsMainPtr( preAllocatedMemory) );
		Assert( gcm::IsLocalPtr( preAllocatedMemory  )|| IsDynamic() );
		Assert( gcm::IsLocalPtr( preAllocatedMemory + m_VertCount * m_Stride -1 )|| gcm::IsMainPtr( preAllocatedMemory + m_VertCount * m_Stride -1 ) );

		m_GCMBuffer = preAllocatedMemory;
	}
	else
	{
		m_GCMBuffer = gcm::AllocatePtr(m_VertCount * m_Stride, IsDynamic() );
		Assert(bReadWrite);     // without preallocated vertices, doesn't make sense to be read only
	}
#if __PPU
	m_VertexData = (u8*) (IsDynamic() ? 0x80000000 | gcm::MainOffset(m_GCMBuffer) : gcm::LocalOffset(m_GCMBuffer));
#endif
}

grcVertexBufferGCM::grcVertexBufferGCM(class datResource& rsc) : grcVertexBuffer(rsc)
{
	rsc.PointerFixup(m_GCMBuffer);
#if __PPU
	m_VertexData = (u8*) (IsDynamic() ? 0x80000000 | gcm::MainOffset(m_GCMBuffer) : gcm::LocalOffset(m_GCMBuffer));
#endif
}

grcVertexBufferGCM::~grcVertexBufferGCM()
{
#if __PPU
	m_VertexData = NULL;
#endif

	if ( !IsPreallocatedMemory() )
	{
		// Delete the buffer
		gcm::FreePtr(m_GCMBuffer);
	}
}

#endif // !__SPU

#if __DECLARESTRUCT
void grcVertexBufferGCM::DeclareStruct(datTypeStruct &s)
{
	grcVertexBuffer::DeclareStruct(s);
	SSTRUCT_BEGIN_BASE(grcVertexBufferGCM, grcVertexBuffer)
	SSTRUCT_FIELD_VP(grcVertexBufferGCM, m_GCMBuffer)
	SSTRUCT_END(grcVertexBufferGCM);
}
#endif

} // namespace rage

#endif // __OPENGL
