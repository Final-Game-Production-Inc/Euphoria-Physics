// 
// grcore/indexbuffer_common.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/indexbuffer.h"
#include "grcore/resourcecache.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "grcore/device.h"

#include "system/memory.h"
#include "system/ipc.h"
#include "system/platform.h"
#include "system/xtl.h"
#include "diag/tracker.h"

namespace rage
{

void grcIndexBuffer::Place(grcIndexBuffer *that,datResource &rsc) {
#if __PPU
	::new (that) grcIndexBufferGCM(rsc);
#elif __GNM
	::new (that) grcIndexBufferGNM(rsc);
#elif RSG_DURANGO
	::new (that) grcIndexBufferDurango(rsc);
#else

#if !__D3D11
	::new (that) grcIndexBufferD3D(rsc);
#else // !__D3D11
	::new (that) grcIndexBufferD3D11(rsc);
#endif// !__D3D11

#endif
}


grcIndexBuffer::grcIndexBuffer(int indexCount)
{
	Assert((indexCount & ~IDX_COUNT_MASK) == 0);
	m_IndexCountAndFlags = indexCount;
	Assert(GetIndexCount() == indexCount);
}


grcIndexBuffer::grcIndexBuffer(datResource& rsc)
{
	(void) rsc;
}


grcIndexBuffer::~grcIndexBuffer()
{
	if( m_IndexData )
	{
		RESOURCE_DEALLOCATE(m_IndexData);
		m_IndexData = NULL;
	}
}

#if __PSP2
class grcIndexBufferPSP2: public grcIndexBuffer
{
public:
	grcIndexBufferPSP2(int indexCount,void *preallocateMemory) : grcIndexBuffer(indexCount) {
		if (preallocateMemory)
			MarkPreallocated();
		m_IndexData = preallocateMemory? (u16*)preallocateMemory : rage_new u16[indexCount];
	}
	u16* Lock(u32,u32,u32) const { return m_IndexData; }
	const u16* LockRO() const { return m_IndexData; }
	u16* LockRW() const { return m_IndexData; }
	void Unlock() const { }
};
#endif

grcIndexBuffer* grcIndexBuffer::Create(int indexCount, bool dynamic,void *preAllocatedMemory)
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(grcIndexBuffer);

#if RSG_PC
	Assertf(!preAllocatedMemory, "PC doesn't support buffer creation with pre-allocated memory for some reason");
	(void)preAllocatedMemory;
#if !__D3D11
	if (g_sysPlatform==platform::PS3)
		return rage_new grcIndexBufferGCM(indexCount, dynamic);
#if __64BIT && __RESOURCECOMPILER
	else if(g_sysPlatform==platform::DURANGO)
		return rage_new grcIndexBufferDurango(indexCount, preAllocatedMemory);
	else if(g_sysPlatform==platform::ORBIS)
		return rage_new grcIndexBufferGNM(indexCount, dynamic, preAllocatedMemory);
#endif // __64BIT && __RESOURCECOMPILER
	else
		return rage_new grcIndexBufferD3D(indexCount);

#else //!__D3D11
	(void)dynamic;
	// DX11 TODO:- Assume Create()->Lock()->Fill->Unlock() once all buffers for now.
	return rage_new grcIndexBufferD3D11(indexCount, grcsBufferCreate_ReadWriteOnceOnly);
#endif //!__D3D11

#elif RSG_DURANGO
	(void)dynamic;
	return rage_new grcIndexBufferDurango(indexCount, preAllocatedMemory);
#elif __XENON
	(void)dynamic;
	return rage_new grcIndexBufferD3D(indexCount, preAllocatedMemory);
#elif __PPU
	return rage_new grcIndexBufferGCM(indexCount, dynamic, preAllocatedMemory);
#elif __PSP2
	return rage_new grcIndexBufferPSP2(indexCount, preAllocatedMemory);
#elif RSG_ORBIS
	return rage_new grcIndexBufferGNM(indexCount, dynamic, preAllocatedMemory);
#endif
}


#if __D3D11 && RSG_PC

grcIndexBuffer* grcIndexBuffer::Clone(class grcIndexBuffer *pExisting)
{
	// DX11 TODO:- See which instances require only write access.
	grcIndexBuffer *pRet = CreateWithData(pExisting->GetIndexCount(), grcsBufferCreate_ReadWrite, pExisting->GetUnsafeReadPtr(), true);
	// Prevent cloning of a clone (we can`t guarantee the validity/life time of the m_IndexData pointer).
	pRet->m_IndexData = 0;
	return pRet;
}

grcIndexBuffer* grcIndexBuffer::CreateWithData(int indexCount, grcBufferCreateType CreateType, const void* preAllocatedMemory, bool AllocateCPUCopy)
{
	return rage_new grcIndexBufferD3D11(indexCount, CreateType, (u16 *)preAllocatedMemory, AllocateCPUCopy);
}

#endif //__D3D11 && RSG_PC

#if RSG_DURANGO

grcIndexBuffer* grcIndexBuffer::Clone(class grcIndexBuffer *pExisting)
{
	grcIndexBuffer *pRet = rage_new grcIndexBufferDurango(pExisting->GetIndexCount(), NULL);

	u16 *pDest = pRet->LockWO();
	memcpy(pDest, pExisting->LockRO(), pExisting->GetIndexCount()*sizeof(u16));
	pRet->UnlockWO();
	pExisting->UnlockRO();

	return pRet;
}

grcIndexBuffer* grcIndexBuffer::CreateWithData(int indexCount, bool UNUSED_PARAM(bDynamic), const void* preAllocatedMemory)
{
	grcIndexBuffer *pRet = rage_new grcIndexBufferDurango(indexCount, NULL);

	u16 *pDest = pRet->LockWO();
	memcpy(pDest, preAllocatedMemory, indexCount*sizeof(u16));
	pRet->UnlockWO();

	return pRet;
}

#endif // RSG_DURANGO

#if RSG_ORBIS

grcIndexBuffer* grcIndexBuffer::Clone(class grcIndexBuffer *pExisting)
{
	grcIndexBuffer *pRet = rage_new grcIndexBufferGNM(pExisting->GetIndexCount(), false, NULL);

	u16 *pDest = pRet->LockWO();
	memcpy(pDest, pExisting->LockRO(), pExisting->GetIndexCount()*sizeof(u16));
	pRet->UnlockWO();
	pExisting->UnlockRO();

	return pRet;
}

grcIndexBuffer* grcIndexBuffer::CreateWithData(int indexCount, bool bDynamic, const void* preAllocatedMemory)
{
	grcIndexBuffer *pRet = rage_new grcIndexBufferGNM(indexCount, bDynamic, NULL);

	u16 *pDest = pRet->LockWO();
	memcpy(pDest, preAllocatedMemory, indexCount*sizeof(u16));
	pRet->UnlockWO();

	return pRet;
}

#endif // RSG_ORBIS

void grcIndexBuffer::Set(rage::grcIndexBuffer* pSource)
{
	Assert(GetIndexCount() == pSource->GetIndexCount());
	const u16 *srcLock = pSource->LockRO();
	u16 *destLock = LockWO();
	sysMemCpy(destLock, srcLock, GetIndexCount()*sizeof(u16));
	UnlockWO();
	pSource->UnlockRO();
}

#if __DECLARESTRUCT
void grcIndexBuffer::DeclareStruct(datTypeStruct &s)
{
#if __RESOURCECOMPILER
	if (g_ByteSwap)
		ByteSwapBuffer();
#endif
	STRUCT_BEGIN(grcIndexBuffer);
	STRUCT_FIELD(m_IndexCountAndFlags);
	STRUCT_FIELD(m_IndexData);
	STRUCT_END();
}

#if __RESOURCECOMPILER
void grcIndexBuffer::ByteSwapBuffer()
{
	u16* pPtr16 = LockRW();
	for(int i=0; i<GetIndexCount(); ++i)
	{
		*pPtr16 = (*pPtr16)>>8 | (*pPtr16)<<8;
		++pPtr16;
	}
	UnlockRW();
}
#endif
#endif


} // namespace rage

