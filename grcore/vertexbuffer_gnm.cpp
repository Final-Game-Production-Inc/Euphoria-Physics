//
// grcore/vertexbuffer_gnm.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//
#include "vertexbuffer.h"

#if __GNM || (RSG_PC && __64BIT && __RESOURCECOMPILER)

#if __GNM
#include "wrapper_gnm.h"
#endif //__GNM

#if __RESOURCECOMPILER
extern int g_useVirtualMemory;
#endif

namespace rage {

grcVertexBufferGNM::grcVertexBufferGNM(class datResource& rsc): grcVertexBuffer(rsc)
{
	m_LockPtr = m_VertexData;
}

grcVertexBufferGNM::grcVertexBufferGNM(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, u8* preAllocatedMemory) : grcVertexBuffer(vertCount,pFvf,bReadWrite,bDynamic,preAllocatedMemory)
{
#if __GNM
#if __RESOURCECOMPILER
	if(g_useVirtualMemory && g_sysPlatform == platform::ORBIS)
	{
		m_VertexData = (u8*) (preAllocatedMemory ? preAllocatedMemory : rage_new u8[(vertCount+1) * pFvf.GetTotalSize()]); //  allocateVideoSharedMemory(vertCount * pFvf.GetTotalSize(),sce::Gnm::kAlignmentOfBufferInBytes));
	}
	else
#endif
	{
		Assertf(((preAllocatedMemory == NULL) || isAllocatedVideoMemory(preAllocatedMemory)), "Preallocated Vertex buffer not allocated from proper memory pool %p", preAllocatedMemory);
		m_VertexData = (u8*) (preAllocatedMemory ? preAllocatedMemory :  allocateVideoSharedMemory(vertCount * pFvf.GetTotalSize(),sce::Gnm::kAlignmentOfBufferInBytes));
	}
#else //__GNM
#if __RESOURCECOMPILER
	if(g_useVirtualMemory && g_sysPlatform == platform::ORBIS)
	{
		m_VertexData = rage_new u8[(vertCount+1) * pFvf.GetTotalSize()]; // (u8 *)physical_new(vertCount * pFvf.GetTotalSize(), 16);
	}
	else
#endif
	{
		m_VertexData = (u8 *)physical_new(vertCount * pFvf.GetTotalSize(), 16);
	}
#endif //__GNM
	m_LockPtr = m_VertexData;
}

grcVertexBufferGNM::~grcVertexBufferGNM()
{
#if __GNM
	if (!IsPreallocatedMemory()) 
	{
		freeVideoSharedMemory(m_VertexData);

		m_VertexData = NULL;
	}
#else //__GNM
	if(m_VertexData)
	{
		physical_delete(m_VertexData);
	}
	m_VertexData = NULL;
#endif //__GNM
}

#if __GNM
#define GRCVERTEXBUFFERGNM	grcVertexBuffer
#else //__GNM
#define GRCVERTEXBUFFERGNM	grcVertexBufferGNM
#endif //__GNM


void *GRCVERTEXBUFFERGNM::Lock(u32 UNUSED_PARAM(flags),u32 offset,u32 UNUSED_PARAM(size)) const
{
	return (char*)m_LockPtr + offset;
}

const void *GRCVERTEXBUFFERGNM::LockRO() const
{
	return Lock(grcsRead);
}

void *GRCVERTEXBUFFERGNM::LockRW() const
{
	return Lock(grcsRead|grcsWrite);
}

void *GRCVERTEXBUFFERGNM::LockWO() const
{
	return Lock(grcsWrite);
}

void GRCVERTEXBUFFERGNM::UnlockRO() const
{
}

void GRCVERTEXBUFFERGNM::UnlockRW() const
{
}

void GRCVERTEXBUFFERGNM::UnlockWO() const
{
}

#if __RESOURCECOMPILER
const void *GRCVERTEXBUFFERGNM::GetUnsafeReadPtr() const
{
	return m_VertexData;
}
#endif //__RESOURCECOMPILER


#if __DECLARESTRUCT
void grcVertexBufferGNM::DeclareStruct(datTypeStruct &s)
{
	grcVertexBuffer::DeclareStruct(s);

	STRUCT_BEGIN(grcVertexBufferGNM);
	STRUCT_END();

}
#endif

} // namespace rage

#endif //__GNM || (RSG_PC && __64BIT && __RESOURCECOMPILER)