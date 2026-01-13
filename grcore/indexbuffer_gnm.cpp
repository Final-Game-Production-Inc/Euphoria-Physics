//
// grcore/indexbuffer_gnm.cpp
//
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved.
//

#include "indexbuffer.h"

#if __GNM || (RSG_PC && __64BIT && __RESOURCECOMPILER)

#if __GNM
#include "wrapper_gnm.h"
#endif //__GNM

#if __RESOURCECOMPILER
extern int g_useVirtualMemory;
#endif

namespace rage {

grcIndexBufferGNM::grcIndexBufferGNM(datResource& rsc) : grcIndexBuffer(rsc)
{
	MarkPreallocated();		// A bit of a lie, but don't waste time trying to free the memory if it's own by resource
}

grcIndexBufferGNM::grcIndexBufferGNM(int indexCount, bool dynamic, void* preAllocatedMemory /* = NULL */) : grcIndexBuffer(indexCount)
{
	if (preAllocatedMemory)
		MarkPreallocated();
#if __GNM
#if __RESOURCECOMPILER
if(g_useVirtualMemory && g_sysPlatform == platform::ORBIS)
{
	m_IndexData = (u16*) (preAllocatedMemory? preAllocatedMemory : rage_new u16[indexCount]); // aligned_new(16) u16[allocateVideoSharedMemory(indexCount * 2,sce::Gnm::kAlignmentOfBufferInBytes));
}
else
#endif
{
	Assertf(((preAllocatedMemory == NULL) || isAllocatedVideoMemory(preAllocatedMemory)), "Preallocated Index buffer not allocated from proper memory pool %p", preAllocatedMemory);
	m_IndexData = (u16*) (preAllocatedMemory? preAllocatedMemory : allocateVideoSharedMemory(indexCount * 2,sce::Gnm::kAlignmentOfBufferInBytes));
}
#else //__GNM
	(void)dynamic;
#if __RESOURCECOMPILER
	if(g_useVirtualMemory && g_sysPlatform == platform::ORBIS)
	{
		m_IndexData = rage_new u16[indexCount]; // (u16 *)physical_new(indexCount*2, 16);
	}
	else
#endif
	{
		m_IndexData = (u16 *)physical_new(indexCount*2, 16);
	}
#endif //__GNM
}

grcIndexBufferGNM::~grcIndexBufferGNM()
{
#if __GNM
	if (!IsPreallocatedMemory())
	{
		freeVideoSharedMemory(m_IndexData);
		
		m_IndexData = NULL;
	}
#else //__GNM
	if(m_IndexData)
	{
		physical_delete(m_IndexData);
	}
	m_IndexData = NULL;
#endif //__GNM
}

#if __GNM
#define GRCINDEXBUFFERGNM	grcIndexBuffer
#else //__GNM
#define GRCINDEXBUFFERGNM	grcIndexBufferGNM
#endif //__GNM

u16 *GRCINDEXBUFFERGNM::Lock(u32 flags,u32 offset,u32 size) const
{
	(void)size;
	(void)flags;
	return m_IndexData + offset;
}

const u16 *GRCINDEXBUFFERGNM::LockRO() const
{
	return Lock(grcsRead);
}

u16 *GRCINDEXBUFFERGNM::LockRW() const
{
	return Lock(grcsRead|grcsWrite);
}

u16 *GRCINDEXBUFFERGNM::LockWO() const
{
	return Lock(grcsWrite);
}

void GRCINDEXBUFFERGNM::UnlockRO() const
{
}

void GRCINDEXBUFFERGNM::UnlockRW() const
{
}

void GRCINDEXBUFFERGNM::UnlockWO() const
{
}

const void *GRCINDEXBUFFERGNM::GetUnsafeReadPtr() const
{
	return m_IndexData;
}


#if __DECLARESTRUCT
void grcIndexBufferGNM::DeclareStruct(class datTypeStruct &s)
{
	grcIndexBuffer::DeclareStruct(s);

	STRUCT_BEGIN(grcIndexBufferGNM);
	STRUCT_END();
}
#endif //__DECLARESTRUCT

}	// namespace rage

#endif //__GNM || (RSG_PC && __64BIT && __RESOURCECOMPILER)

