// 
// grcore/indexbuffer_d3d.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/indexbuffer.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "grcore/device.h"
#include "grcore/locktypes.h"

#include "system/memory.h"
#include "system/ipc.h"
#include "system/platform.h"
#include "system/xtl.h"
#include "system/d3d10.h"
#include "diag/tracker.h"
#include "resourcecache.h"

#if __WIN32 && !__D3D11
#pragma warning(disable:4062)
#include "system/d3d9.h"
#pragma warning(error:4062)

#if __XENON
#include <ppcintrinsics.h>
#define DBG 0
#include <xgraphics.h>
#endif

#if !__XENON
#include "xenon_resource.h"
#endif

#if __WIN32PC
#include "grcore/d3dwrapper.h"
#endif // __WIN32PC

#include "grcore/wrapper_d3d.h"

#define SLEEPWAITS	__OPTIMIZED // PIX doesn't handle do not wait properly
#define USE_STAGING 1

#if __RESOURCECOMPILER
int g_usePhysicalMemory = 0;
#endif

namespace rage
{
#if __WIN32PC && !__RESOURCECOMPILER
grcIndexBufferD3D* grcIndexBufferD3D::sm_First = NULL;

grcIndexManager::grcIndexManager()
{
	static bool bOnce = true;

	if (bOnce)
	{
		GRCDEVICE.RegisterDeviceLostCallbacks(MakeFunctor(grcIndexManager::Lost), MakeFunctor(grcIndexManager::Reset));
		bOnce = true;
	}
}

bool grcIndexManager::Lost()
{
	grcIndexBufferD3D* t = grcIndexBufferD3D::sm_First;
	while (t) 
	{
		AssertVerify(t->Lost());
		const grcIndexBufferD3D::D3DInternalData* pData = (const grcIndexBufferD3D::D3DInternalData*)t->storage;
		t = pData->m_Next;
	}
	return true;
}

bool grcIndexManager::Reset()
{
	grcIndexBufferD3D *t = grcIndexBufferD3D::sm_First;
	while (t) 
	{
		AssertVerify(t->Reset());
		const grcIndexBufferD3D::D3DInternalData* pData = (const grcIndexBufferD3D::D3DInternalData*)t->storage;
		t = pData->m_Next;
	}
	return true;
}
#endif // __WIN32PC && !__RESOURCECOMPILER


grcIndexBufferD3D::grcIndexBufferD3D(int indexCount, void* preAllocatedMemory) : grcIndexBuffer(indexCount)
{
	(void)sizeof(preAllocatedMemory);
#if __WIN32PC && !__RESOURCECOMPILER
	D3DInternalData* pData = GetInternalData();
	pData->m_D3DStagingBuffer = NULL;
	pData->m_Pool			= 0;
	pData->m_LockType		= 0;
	pData->m_uOffset		= 0;
	pData->m_uSize			= 0;

	pData->m_Previous = NULL;
	pData->m_Next	  = sm_First;

	sm_First = this;

	if (pData->m_Next)
		pData->m_Next->GetInternalData()->m_Previous = this;
#elif __WIN32PC
	D3DInternalData* pData = GetInternalData();
	pData->m_D3DStagingBuffer = NULL;
	pData->m_Pool		   = 0;
	pData->m_LockType	   = 0;
	pData->m_uOffset		= 0;
	pData->m_uSize			= 0;

	pData->m_Previous = NULL;
	pData->m_Next	  = NULL;
#endif // __WIN32PC && !__RESOURCECOMPILER

#if __RESOURCECOMPILER
	Assert(sysMemAllocator::GetCurrent().IsBuildingResource());
	if (g_sysPlatform == platform::WIN32PC || g_sysPlatform == platform::WIN64PC || g_sysPlatform == platform::DURANGO) 
	{
		if(g_usePhysicalMemory && g_sysPlatform == platform::WIN64PC)
		{
			m_IndexData = (u16*)physical_new(GetIndexCount() * sizeof(u16), RAGE_INDEXBUFFER_ALIGNMENT);
		}
		else
		{
			m_IndexData = (u16*) RESOURCE_ALLOCATOR(GetIndexCount() * sizeof(u16),RAGE_INDEXBUFFER_ALIGNMENT);
		}

		
		m_D3DBuffer = 0;
	}
	else
	{
		CompileTimeAssert(sizeof(D3DIndexBuffer) <= sizeof(storage));
		D3DIndexBuffer *ib = (D3DIndexBuffer*) storage;
		u32 Length = GetIndexCount() * sizeof(u16);
		u16* baseLock = (u16*) physical_new(Length,RAGE_INDEXBUFFER_ALIGNMENT);
		m_IndexData = baseLock;
		ib->Common = D3DCOMMON_TYPE_INDEXBUFFER | D3DCOMMON_CPU_CACHED_MEMORY | (_XDK_VER < 2638? 1 : 0);
		ib->Fence = 0;
		ib->Common |= 0x20000000; // GPUENDIAN_8IN16 << D3DINDEXBUFFER_ENDIAN_SHIFT;	// A bit of a guess!
		ib->ReferenceCount = 1;
		ib->ReadFence = 0;
		ib->Identifier = 0;
		ib->BaseFlush = D3DFLUSH_INITIAL_VALUE;
		ib->Address = (u32)baseLock;
		ib->Size = Length;

		m_D3DBuffer = (d3dIndexBuffer*) ib;
	}
#elif __XENON
	if (preAllocatedMemory)
	{
		MarkPreallocated();
		CompileTimeAssert(sizeof(D3DIndexBuffer) == sizeof(storage));
		D3DIndexBuffer* ib = (d3dIndexBuffer*) storage;
		XGSetIndexBufferHeader(GetIndexCount() * sizeof(u16), D3DUSAGE_CPU_CACHED_MEMORY, D3DFMT_INDEX16, 0, 0, ib);
		Assert((reinterpret_cast<u32>(preAllocatedMemory) & (RAGE_INDEXBUFFER_ALIGNMENT-1)) == 0);
		XGOffsetResourceAddress(ib, preAllocatedMemory);
		m_D3DBuffer = (d3dIndexBuffer*) ib;
	}
	else
	{
		CreateInternal();
	}
#else //__WIN32PC

	if (preAllocatedMemory != NULL)
	{
#if !(__WIN32PC && !__D3D11 && !__RESOURCECOMPILER)
		Warningf("Pre-allocated memory not supported for __WIN32PC index buffers\n");
#endif
	}
	CreateInternal(WIN32PC_ONLY(true));

#endif // __RESOURCECOMPILER
}


void grcIndexBufferD3D::CreateInternal(WIN32PC_ONLY(bool bUseStaging))
{
#if __WIN32PC && (__RESOURCECOMPILER || (__D3D9))
	bUseStaging;
#endif
	ASSERT_ONLY(HRESULT hr);
#if __D3D9
	{
#if __WIN32PC && !__RESOURCECOMPILER
		int nUsage = 0;
		D3DPOOL ePool = (D3DPOOL)GRCDEVICE.GetBufferResourcePool();
		GetInternalData()->m_Pool = ePool;

		if (ePool == RESOURCE_UNMANAGED)
		{
			nUsage |= D3DUSAGE_WRITEONLY;
		}
#endif // __WIN32PC

#if __ASSERT
#if __XENON || __RESOURCECOMPILER
		D3DPOOL ePool = D3DPOOL_MANAGED;
		int nUsage = 0;
#endif // __XENON
		hr = GRCDEVICE.GetCurrent()->CreateIndexBuffer(GetIndexCount() * sizeof(u16), nUsage, D3DFMT_INDEX16, ePool, (INDEXBUFFERDX9**)&m_D3DBuffer, 0);
		switch(hr)
		{
		case D3D_OK:
			{
				// All good
				break;
			}
		case D3DERR_INVALIDCALL:
			{
				Assertf( SUCCEEDED( hr ), "D3DERR_INVALIDCALL returned by CreateIndexBuffer");
				break;
			}
		case D3DERR_OUTOFVIDEOMEMORY:
			{
				Assertf( SUCCEEDED( hr ), "D3DERR_OUTOFVIDEOMEMORY returned by CreateIndexBuffer");
				break;
			}
		case E_OUTOFMEMORY:
			{
				Assertf( SUCCEEDED( hr ), "E_OUTOFMEMORY returned by CreateIndexBuffer");
				break;
			}
		default:
			{
				Assertf( SUCCEEDED( hr ), "Unknown error code %d returned by CreateIndexBuffer", hr);
				break;
			}
		}
#else
		CHECK_HRESULT(GRCDEVICE.GetCurrent()->CreateIndexBuffer(GetIndexCount() * sizeof(u16), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, (INDEXBUFFERDX9**)&m_D3DBuffer, 0));
#endif
	}
#endif // __D3D9
}


u16* grcIndexBufferD3D::LockInternal(u32 lockFlagGrc, u32 offset, u32 size) const
{
	u32 lockFlagD3D = D3DLOCK_NOSYSLOCK;
#if __WIN32PC
	lockFlagD3D |= (lockFlagGrc & grcsDoNotWait) ? D3DLOCK_DONOTWAIT : 0;
	lockFlagD3D |= (lockFlagGrc & grcsDiscard) ? D3DLOCK_DISCARD : 0;
#endif // __WIN32PC
	lockFlagD3D |= (lockFlagGrc & grcsNoOverwrite) ? D3DLOCK_NOOVERWRITE : 0;
	Assert(!AreLockFlagsReadOnly(lockFlagGrc));

#if !__RESOURCECOMPILER
	if (m_D3DBuffer)
	{
#if __D3D9
		{
			void *lockPtr = NULL;
			AssertVerify(((INDEXBUFFERDX9*)m_D3DBuffer)->Lock(offset,size,&lockPtr,lockFlagD3D) == D3D_OK);
			return (u16*) lockPtr;
		}
#else
		(void)size;
		(void)offset;
#endif // __D3D9
	}
#else
	(void)size;
	(void)offset;
#endif // !__RESOURCECOMPILER
	return m_IndexData;
}

void grcIndexBufferD3D::UnlockInternal() const
{
#if !__RESOURCECOMPILER
	if (m_D3DBuffer)
	{
#if __D3D9
			CHECK_HRESULT(((INDEXBUFFERDX9*)m_D3DBuffer)->Unlock());
#endif
	}
#endif // !__RESOURCECOMPILER
}


/*
#if __WIN32PC
grcIndexBufferD3D::D3DInternalData* grcIndexBufferD3D::GetInternalData() const
{
	return (D3DInternalData*)storage;
}
#endif // __WIN32
*/


grcIndexBufferD3D::grcIndexBufferD3D(datResource& rsc) : grcIndexBuffer(rsc)
{
#if __WIN32PC && !__RESOURCECOMPILER
	D3DInternalData* pData = (D3DInternalData*)storage;

	if (datResource_IsDefragmentation) {
		// Repair linked list nodes (including the head!)
		rsc.PointerFixup(pData->m_Previous);
		rsc.PointerFixup(pData->m_Next);
		if (pData->m_Previous)
			pData->m_Previous->GetInternalData()->m_Next = this;
		else
			sm_First = this;
		if (pData->m_Next)
			pData->m_Next->GetInternalData()->m_Previous = this;
		return;
	}

	pData->m_D3DStagingBuffer = NULL;
	pData->m_Pool		   = 0;
	pData->m_LockType	   = 0;

	pData->m_Previous = NULL;
	pData->m_Next	  = sm_First;

	sm_First = this;

	if (pData->m_Next)
		pData->m_Next->GetInternalData()->m_Previous = this;
#endif // __WIN32PC && !__RESOURCECOMPILER

#if __WIN32PC && __PAGING
	if (m_IndexData) 
	{
		CreateInternal(USE_STAGING);

#if __D3D9		// Initialization is handled in CreateInternal already under D3D11
		// TODO: Make a proper interface to temporarily allowing write locks
		sysMemCpy(LockRW(), m_IndexData, GetIndexCount() * sizeof(u16));
		UnlockRW();
#endif
	}
#elif __XENON
	{
		rsc.PointerFixup(m_D3DBuffer);
		m_D3DBuffer->Address += (rsc.GetFixup((void*)m_D3DBuffer->Address));
	}
#endif
}

grcIndexBufferD3D::~grcIndexBufferD3D()
{
	ReleaseD3DBuffer();
}

// Line by line identical with the destructor
void grcIndexBufferD3D::ReleaseD3DBuffer()
{
	if( m_D3DBuffer ) {
#if __XENON
			if( m_D3DBuffer->Common & D3DCOMMON_D3DCREATED )
				m_D3DBuffer->Release();
			/* else
				delete m_D3DBuffer;	// Now part of base object, not a separate allocation */
#else
		if (g_sysPlatform == platform::XENON)
			delete m_D3DBuffer;
		else
			SAFE_RELEASE_RESOURCE(m_D3DBuffer);
#endif
	}
#if __WIN32PC && !__RESOURCECOMPILER
	if (GetStagingD3DBuffer() != NULL)
	{
		FREE_RESOURCE(RT_IndexBuffer, GetInternalData()->m_D3DStagingBuffer);
	}
#endif

#if __WIN32PC && !__RESOURCECOMPILER
	D3DInternalData* pData = GetInternalData();

	if (sm_First == this)
	{
		sm_First = pData->m_Next;
	}

	if (pData->m_Next)
	{
		pData->m_Next->GetInternalData()->m_Previous = pData->m_Previous;
	}

	if (pData->m_Previous != NULL)
	{
		pData->m_Previous->GetInternalData()->m_Next = pData->m_Next;
	}
#endif // __WIN32PC && !__RESOURCECOMPILER
}


#if __XENON

	const void *grcIndexBuffer::GetUnsafeReadPtr() const
	{
		return (const void*)((static_cast<const grcIndexBufferD3D*>(this)->GetD3DBuffer())->Address);
	}

	const u16 *grcIndexBuffer::LockRO() const
	{
		return (const u16*)GetUnsafeReadPtr();
	}

#endif // __XENON


#if __WIN32PC && !__RESOURCECOMPILER
bool grcIndexBufferD3D::Lost()
{
#if USE_RESOURCE_CACHE
	if (m_D3DBuffer)
	{
		if (GetInternalData()->m_Pool == RESOURCE_UNMANAGED)
		{
			SAFE_RELEASE_RESOURCE(m_D3DBuffer);
			m_D3DBuffer = NULL;
		}
	}
#endif // USE_RESOURCE_CACHE
	return true;
}

bool grcIndexBufferD3D::Reset()
{
#if USE_RESOURCE_CACHE
	if (m_D3DBuffer == NULL)
	{
		CreateInternal(true);
	}
#endif // USE_RESOURCE_CACHE
	return true;
}

#endif // __WIN32PC && !__RESOURCECOMPILER


#if __DECLARESTRUCT
#ifndef IS_COMBINED
inline void datSwapper(DWORD &d) { datSwapper(reinterpret_cast<u32&>(d)); }
#endif

void grcIndexBufferD3D::DeclareStruct(datTypeStruct &s)
{
	grcIndexBuffer::DeclareStruct(s);
	STRUCT_BEGIN(grcIndexBufferD3D);
	D3DIndexBuffer *ib = (D3DIndexBuffer*) m_D3DBuffer;
	if (g_ByteSwap) {
		datSwapper(ib->Common);
		datSwapper(ib->Fence);
		datSwapper(ib->ReferenceCount);
		datSwapper(ib->ReadFence);
		datSwapper(ib->Identifier);
		datSwapper(ib->BaseFlush);
		datSwapper((void*&)ib->Address);
		datSwapper(ib->Size);
	}
	STRUCT_FIELD_VP(m_D3DBuffer);
	STRUCT_SKIP(storage,sizeof(storage));
	STRUCT_END();
}
#endif


}

#endif	// __WIN32 && !__D3D11

