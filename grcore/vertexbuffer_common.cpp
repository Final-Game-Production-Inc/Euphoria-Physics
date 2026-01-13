// 
// grcore/vertexbuffer_common.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/channel.h"
#include "grcore/vertexbuffer.h"
#include "grcore/vertexbuffereditor.h"
#include "grcore/device.h"
#include "grcore/fvf.h"
#include "grcore/resourcecache.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "data/safestruct.h"
#include "atl/array_struct.h"

#if __PPU
#include "grcore/wrapper_gcm.h"
#endif

#include "system/cache.h"
#include "system/memory.h"
#include "system/ipc.h"
#include "system/xtl.h"
#include "system/platform.h"
#include "diag/tracker.h"

#include "profile/page.h"
#include "profile/group.h"

namespace rage
{

u32 g_AllowVertexBufferVramLocks = 0;

void grcVertexBuffer::Place(grcVertexBuffer *that,datResource &rsc) {
#if __PPU
	::new (that) grcVertexBufferGCM(rsc);
#elif RSG_ORBIS
	::new (that) grcVertexBufferGNM(rsc);
#elif RSG_DURANGO
	::new (that) grcVertexBufferDurango(rsc);
#elif !__D3D11
	::new (that) grcVertexBufferD3D(rsc);
#else //!__D3D11
	::new (that) grcVertexBufferD3D11(rsc);
#endif //!__D3D11
}

grcVertexBuffer::grcVertexBuffer(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, void* preallocatedMemory)
: m_Flags(0)
, m_LockPtr(0)
, m_VertexData(NULL)
, m_VertCount(vertCount)
{
	Assert(!bDynamic || bReadWrite);    // bDynamic => bReadWrite

	if (bReadWrite)
		m_Flags |= kReadWrite;

	if (bDynamic)
		m_Flags |= kDynamic;

	if (preallocatedMemory != NULL)
		m_Flags |= kPreallocatedMemory;

	SetFvf(rage_new grcFvf(pFvf, pFvf.IsDynamicOrder()));
}

grcVertexBuffer::grcVertexBuffer(class datResource& rsc)
{
	// datResource::IsDefragmentation() is a static function on some platforms,
	// but not others, which leads to an unreferenced parameter warning.
	(void)rsc;

	if (!rsc.IsDefragmentation())
	{
		m_LockPtr = 0;
	}
}

grcVertexBuffer::~grcVertexBuffer()
{
	if( m_Fvf )
		delete m_Fvf;
#if __WIN32		// PS3 uses this to store raw offset data, it's not a real pointer.
	if( m_VertexData )
	{
#if !__D3D11	// in DX11 this is a copy of a pointer to previously allocated memory - don't free it twice!
		RESOURCE_DEALLOCATE(m_VertexData);
#endif
		m_VertexData = NULL;
	}
#endif
}

#if __PSP2
class grcVertexBufferPSP2: public grcVertexBuffer {
public:
	grcVertexBufferPSP2(int vertCount,const grcFvf &pFvf, bool bReadWrite, bool bDynamic, void* preAllocatedMemory) : grcVertexBuffer(vertCount,pFvf,bReadWrite,bDynamic,preAllocatedMemory) {
		m_VertexData = preAllocatedMemory? (u8*)preAllocatedMemory : rage_new u8[m_Stride * vertCount];
		if (!bReadWrite)
		{
			Assert(preAllocatedMemory); // doesn't make sense to be read only if we have no initialized vertex data yet
			MakeReadOnly();
		}
	}
	~grcVertexBufferPSP2() {
		if (!m_Flags & kPreallocatedMemory)
			delete[] m_VertexData;
	}
	bool IsValid() const { return true; }
	bool Lock(u32, u32 , u32) const { m_LockPtr = m_VertexData; return true; }
	bool LockReadOnly() const { m_LockPtr = m_VertexData; return true; }
	bool Unlock() const { m_LockPtr = 0; return true; }
#if __DECLARESTRUCT
	virtual void DeclareStruct(datTypeStruct &s) { grcVertexBuffer::DeclareStruct(s); }
#endif

};
#endif

#if __D3D11 || RSG_ORBIS
#define RAGE_VERTEX_BUFFER_THREAD_ACCESS_TYPE grcsBufferSync_None
#endif //__D3D11


grcVertexBuffer* grcVertexBuffer::Create(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, void* preAllocatedMemory )
{
	RAGE_TRACK(Graphics);
	RAGE_TRACK(grcVertexBuffer);

	Assert(!bDynamic || bReadWrite);            // bDynamic => bReadWrite
	Assert(bReadWrite || preAllocatedMemory);   // unless we already have vertex data, then doesn't make sense to be read only yet

	grcVertexBuffer *vb;

#if RSG_PC

#if !__D3D11
	if (g_sysPlatform==platform::PS3)
		vb = rage_new grcVertexBufferGCM(vertCount, pFvf, bReadWrite, bDynamic, (u8*)preAllocatedMemory);
#if __64BIT && __RESOURCECOMPILER
	else if(g_sysPlatform==platform::DURANGO)
		vb = rage_new grcVertexBufferDurango(vertCount, pFvf, bReadWrite, bDynamic, (u8*)preAllocatedMemory);
	else if(g_sysPlatform==platform::ORBIS)
		vb = rage_new grcVertexBufferGNM(vertCount, pFvf, bReadWrite, bDynamic, (u8*)preAllocatedMemory);
#endif //__64BIT && __RESOURCECOMPILER
	else
		vb = rage_new grcVertexBufferD3D(vertCount, pFvf, bReadWrite, bDynamic, (u8*)preAllocatedMemory);
	
#else // !__D3D11
	// DX11 TODO:- Assume fully dynamic or Create()->Lock()->Fill->Unlock() once all buffers for now.
	vb = rage_new grcVertexBufferD3D11(vertCount, pFvf, bReadWrite, bDynamic,
		bDynamic ? grcsBufferCreate_ReadDynamicWrite : (preAllocatedMemory ? grcsBufferCreate_NeitherReadNorWrite : grcsBufferCreate_ReadWriteOnceOnly),
		// DX11 TODO:- Make this a parameter.
		bReadWrite ? RAGE_VERTEX_BUFFER_THREAD_ACCESS_TYPE : grcsBufferSync_None,
		(u8*)preAllocatedMemory, false);
#endif //!__D3D11

#elif RSG_DURANGO
	vb = rage_new grcVertexBufferDurango(vertCount, pFvf, bReadWrite, bDynamic, (u8*)preAllocatedMemory);
#elif __XENON
	vb = rage_new grcVertexBufferD3D(vertCount, pFvf, bReadWrite, bDynamic, (u8*)preAllocatedMemory);
#elif __PPU
	vb = rage_new grcVertexBufferGCM(vertCount, pFvf, bReadWrite, bDynamic, (u8*)preAllocatedMemory);
#elif __PSP2
	vb = rage_new grcVertexBufferPSP2(vertCount, pFvf, bReadWrite, bDynamic, preAllocatedMemory);
#elif RSG_ORBIS
	vb = rage_new grcVertexBufferGNM(vertCount, pFvf, bReadWrite, bDynamic, (u8*)preAllocatedMemory);
#endif

	if (Unlikely(!vb->IsValid()))
	{
		delete vb;
		vb = NULL;
	}
	return vb;
}


#if __D3D11	&& RSG_PC
grcVertexBuffer* grcVertexBuffer::Create(int vertCount, const grcFvf& pFvf, grcBufferCreateType CreateType, grcsBufferSyncType SyncType, void* preAllocatedMemory, bool AllocateCPUCopy)
{
	bool bDynamic = (CreateType == grcsBufferCreate_NeitherReadNorWrite) ? false : true;
	bool bReadWrite = bDynamic;
	grcVertexBuffer *vb = rage_new grcVertexBufferD3D11(vertCount, pFvf, bReadWrite, bDynamic, CreateType, SyncType, (u8*)preAllocatedMemory, AllocateCPUCopy);

	if (Unlikely(!vb->IsValid()))
	{
		delete vb;
		vb = NULL;
	}
	return vb;
}


grcVertexBuffer* grcVertexBuffer::Clone( class grcVertexBuffer *pExisting )
{
	// DX11 TODO:- See which instances require only write access.
	const bool bReadWrite = true;
	const bool bDynamic = true; // Not sure we need these to by dynamic. 
	// Higher level cloth code triple buffers these, so just read/write access will do (meaning we don`t need to Map(discard) to work.)
	grcVertexBuffer *pRet = (grcVertexBuffer*)(rage_new grcVertexBufferD3D11(pExisting->GetVertexCount(), *pExisting->GetFvf(), bReadWrite, bDynamic,
		grcsBufferCreate_ReadWrite, // grcsBufferCreate_ReadDynamicWrite, // grcsBufferCreate_ReadWrite, 
		// DX11 TODO:- Make this a parameter.
		RAGE_VERTEX_BUFFER_THREAD_ACCESS_TYPE, 
		(u8*)pExisting->GetVertexData(), true));
	return pRet;
}


grcVertexBuffer* grcVertexBuffer::CreateWithData(int VertCount, const grcFvf& pFvf, grcBufferCreateType CreateType, const void *pVertexData, bool AllocateCPUCopy)
{
	bool bDynamic = (CreateType == grcsBufferCreate_ReadWrite) ? true : false;
	bool bReadWrite = bDynamic;
	return (grcVertexBuffer*)(rage_new grcVertexBufferD3D11(VertCount, pFvf, bReadWrite, bDynamic, CreateType,
		// DX11 TODO:- Make this a parameter.
		bDynamic ? RAGE_VERTEX_BUFFER_THREAD_ACCESS_TYPE : grcsBufferSync_None,
		(u8*)pVertexData, AllocateCPUCopy));
}
#endif // __D3D11 && RSG_PC

#if RSG_DURANGO
grcVertexBuffer* grcVertexBuffer::Clone( class grcVertexBuffer *pExisting )
{
	/*
	grcVertexBuffer *pRet = (grcVertexBuffer*)(rage_new grcVertexBufferDurango(pExisting->GetVertexCount(), *pExisting->GetFvf(), true, false, NULL));
	void *dest = pRet->LockWO();
	memcpy(dest, (u8*)pExisting->GetVertexData(), pExisting->GetVertexCount()*pExisting->GetVertexStride());
	pRet->UnlockWO();
	return pRet;
	*/
	return CreateWithData(pExisting->GetVertexCount(), *pExisting->GetFvf(), pExisting->GetVertexData());

}
#endif // RSG_DURANGO

#if RSG_ORBIS || RSG_DURANGO
grcVertexBuffer* grcVertexBuffer::CreateWithData(int vertCount, const grcFvf& fvf, const void * pData)
{
	// B *1598735 - Clean up this entry point.
	grcVertexBuffer *vb = grcVertexBuffer::Create(vertCount,fvf,true,false,NULL);
	void *dest = vb->LockWO();
	memcpy(dest, pData, vertCount * vb->GetVertexStride());
	vb->UnlockWO();
	return vb;
}
#endif // RSG_ORBIS || RSG_DURANGO
void grcVertexBuffer::MakeReadOnly()
{
#	if !__PS3
		// For non-PS3 platforms, should never attempt to make a dynamic buffer
		// read only.  This assert is disabled on PS3, as "dynamic" just means
		// main memory/xdr, which is perfectly valid to be read only.
		Assert(~m_Flags & kDynamic);
#	endif
	Assert(m_Flags & kReadWrite);   // should have previously been read-write
	m_Flags &= ~kReadWrite;
}

#if __DECLARESTRUCT
void grcVertexBuffer::DeclareStruct(datTypeStruct &s)
{
#if __RESOURCECOMPILER
	if (g_ByteSwap)
	{
		ByteSwapBuffer();
	}
#endif // __RESOURCECOMPILER

	m_reserved = 0;
	SSTRUCT_BEGIN(grcVertexBuffer)
	SSTRUCT_FIELD(grcVertexBuffer, m_Stride)
	SSTRUCT_FIELD(grcVertexBuffer, m_reserved0)
	SSTRUCT_FIELD(grcVertexBuffer, m_Flags)
	SSTRUCT_FIELD_VP(grcVertexBuffer, m_LockPtr)
	SSTRUCT_FIELD(grcVertexBuffer, m_VertCount)	
	SSTRUCT_FIELD(grcVertexBuffer, m_VertexData)
	SSTRUCT_IGNORE(grcVertexBuffer, m_reserved)
	SSTRUCT_FIELD(grcVertexBuffer, m_Fvf)
	SSTRUCT_END(grcVertexBuffer)
}

#if __RESOURCECOMPILER
void grcVertexBuffer::ByteSwapBuffer()
{
	Assert(m_Fvf);
	LockHelper helper(this);

	u8* ptr = (u8*)helper.GetLockPtr();

	for (int i = 0; i < grcFvf::grcfcCount; ++i)
	{
		grcFvf::grcFvfChannels channel = (grcFvf::grcFvfChannels)i;
		if (m_Fvf->IsChannelActive(channel))
		{
			int attrOffset = m_Fvf->GetOffset(channel);
			grcFvf::grcDataSize dataSize = m_Fvf->GetDataSizeType(channel);
			for (u32 j = 0; j < m_VertCount; ++j)
			{
				u8* attr = ptr + m_Stride * j + attrOffset;
				u16* attr16 = (u16*)attr;
				u32* attr32 = (u32*)attr;

				switch (dataSize)
				{
				case grcFvf::grcdsHalf4:
				case grcFvf::grcdsShort4:
					datSwapper(attr16[3]);
				case grcFvf::grcdsHalf3:
					datSwapper(attr16[2]);
				case grcFvf::grcdsHalf2:
				case grcFvf::grcdsShort2:
					datSwapper(attr16[1]);
				case grcFvf::grcdsHalf:
					datSwapper(attr16[0]);
					break;
				case grcFvf::grcdsFloat4:
					datSwapper(attr32[3]);
				case grcFvf::grcdsFloat3:
					datSwapper(attr32[2]);
				case grcFvf::grcdsFloat2:
					datSwapper(attr32[1]);
				case grcFvf::grcdsFloat:
				case grcFvf::grcdsUBYTE4:
				case grcFvf::grcdsColor:
				case grcFvf::grcdsPackedNormal:
					datSwapper(attr32[0]);
					break;

#if __PS3
				case grcFvf::grcdsEDGE0:
				case grcFvf::grcdsEDGE1:
				case grcFvf::grcdsEDGE2:
#else
				case grcFvf::grcdsShort_unorm:
				case grcFvf::grcdsShort2_unorm:
				case grcFvf::grcdsByte2_unorm:
#endif

				case grcFvf::grcdsCount:
					Errorf("Unhandled/invalid attribute data type");
					break;
				}
			}
		}
	}
}
#endif // __RESOURCECOMPILER
#endif // __DECLARESTRUCT

}		// namespace rage

