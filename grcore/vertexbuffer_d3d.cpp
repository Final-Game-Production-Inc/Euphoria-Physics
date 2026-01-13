// 
// grcore/vertexbuffer_d3d.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "grcore/vertexbuffer.h"
#include "grcore/vertexbuffereditor.h"
#include "grcore/locktypes.h"
#include "grcore/device.h"
#include "grcore/fvf.h"
#include "grcore/channel.h"
#include "mesh/mesh.h"
#include "grcore/resourcecache.h"
#include "data/resource.h"
#include "data/resourcehelpers.h"
#include "data/struct.h"
#include "atl/array_struct.h"
#include "channel.h"

#include "system/cache.h"
#include "system/memory.h"
#include "system/ipc.h"
#include "system/platform.h"
#include "system/xtl.h"
#include "system/d3d10.h"
#include "diag/tracker.h"

#include "profile/page.h"
#include "profile/group.h"

#include "math/float16.h"

#if __RESOURCECOMPILER
extern int g_usePhysicalMemory;
#endif

#if __WIN32 && !__D3D11
#pragma warning(disable:4062)
#include "system/d3d9.h"
#pragma warning(error:4062)

#if !__XENON 
#include "xenon_resource.h"
#else
#include "system/xgraphics.h"
#include "xdk.h"
#endif

#if __WIN32PC
#include "grcore/d3dwrapper.h"
#endif // __WIN32PC

#define TRUE_DYNAMIC 1 // - Better to use managed instead of dynamic for performance
#define SLEEPWAITS	__OPTIMIZED // PIX doesn't handle do not wait properly

namespace rage
{

#if __WIN32PC && !__RESOURCECOMPILER
grcVertexBufferD3D* grcVertexBufferD3D::sm_First = NULL;

grcVertexManager::grcVertexManager()
{
	static bool bOnce = true;
	if (bOnce)
	{
		Functor0 resetCb = MakeFunctor(grcVertexManager::Reset);
		Functor0 lostCb = MakeFunctor(grcVertexManager::Lost);
		
		GRCDEVICE.RegisterDeviceLostCallbacks(lostCb, resetCb);
		bOnce = true;
	}
}

bool grcVertexManager::Lost()
{
	grcVertexBufferD3D *t = grcVertexBufferD3D::sm_First;

	while (t) 
	{
		AssertVerify(t->Lost());
		const grcVertexBufferD3D::D3DInternalData* pData = (const grcVertexBufferD3D::D3DInternalData*)t->storage;
		t = pData->m_Next;
	}
	return true;
}

bool grcVertexManager::Reset()
{
	grcVertexBufferD3D *t = grcVertexBufferD3D::sm_First;
	while (t) 
	{
		AssertVerify(t->Reset());
		const grcVertexBufferD3D::D3DInternalData* pData = (const grcVertexBufferD3D::D3DInternalData*)t->storage;
		t = pData->m_Next;
	}
	return true;
}
#endif // __WIN32PC && !__RESOURCECOMPILER

grcVertexBufferD3D::grcVertexBufferD3D(class datResource& rsc) : grcVertexBuffer(rsc)
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

	pData->m_Pool		   = 0;
	pData->m_LockType	   = 0;

	pData->m_Previous = NULL;
	pData->m_Next	  = sm_First;

	sm_First = this;

	if (pData->m_Next)
		pData->m_Next->GetInternalData()->m_Previous = this;
#endif // __WIN32PC && !__RESOURCECOMPILER

#if __WIN32PC && __PAGING
	Assert(m_VertexData);
	if (m_VertexData)
	{
#if __D3D9
		CreateInternal();

		// Read-write lock then unlock to force update of D3D copy
		LockRW();
		UnlockRW();
#else
		CHECK_FOR_PHYSICAL_PTR(m_VertexData);
#endif
	}
#elif __XENON
	{
		rsc.PointerFixup(m_D3DBuffer);
		m_D3DBuffer->Format.BaseAddress += (rsc.GetFixup((void*)(m_D3DBuffer->Format.BaseAddress << 2)) >> 2);
	}
#endif
}

grcVertexBufferD3D::~grcVertexBufferD3D()
{
	Assert(IsValid());

	if (m_D3DBuffer) {
#if __XENON
		if( m_D3DBuffer->Common & D3DCOMMON_D3DCREATED )
		{
			Assert(!IsPreallocatedMemory());
			m_D3DBuffer->Release();
		}
		/* else
			delete m_D3DBuffer; // Now part of base object, not a separate allocation */
#else
		if (g_sysPlatform == platform::XENON)
			delete m_D3DBuffer;
		else
			SAFE_RELEASE_RESOURCE(m_D3DBuffer);
#endif
		m_D3DBuffer = NULL;
	}

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

	if (pData->m_Previous)
	{
		pData->m_Previous->GetInternalData()->m_Next = pData->m_Next;
	}
	pData->m_Next = pData->m_Previous = NULL;
#endif // __WIN32PC && !__RESOURCECOMPILER
}

void grcVertexBufferD3D::ReleaseD3DBufferHack()
{
	if (!g_ByteSwap) {
		if( m_D3DBuffer  && !sysMemAllocator::GetCurrent().IsBuildingResource()  )
		{
#if __XENON
			if( m_D3DBuffer->Common & D3DCOMMON_D3DCREATED )
			{
				Assert(!IsPreallocatedMemory());
#endif
				SAFE_RELEASE_RESOURCE(m_D3DBuffer);
#if __XENON
			}
		/* else
			delete m_D3DBuffer; // Now part of base object, not a separate allocation */
#endif
		}
		m_D3DBuffer = NULL;
	}

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

	if (pData->m_Previous)
	{
		pData->m_Previous->GetInternalData()->m_Next = pData->m_Next;
	}
	pData->m_Next = pData->m_Previous = NULL;
#endif // __WIN32PC && !__RESOURCECOMPILER
}


grcVertexBufferD3D::grcVertexBufferD3D(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, u8* preAllocatedMemory )
	: grcVertexBuffer(vertCount,pFvf,bReadWrite,bDynamic,XENON_ONLY(preAllocatedMemory) WIN32PC_ONLY(NULL))
{
	Assert(!bDynamic || bReadWrite);    // bDynamic => bReadWrite

#if __WIN32PC && !__RESOURCECOMPILER
	D3DInternalData* pData = GetInternalData();
	pData->m_Pool		   = 0;
	pData->m_LockType	   = 0;
	pData->m_uOffset		= 0;
	pData->m_uSize			= 0;

	pData->m_Previous = NULL;
	pData->m_Next	  = sm_First;

	sm_First = this;

	Assert(m_VertCount);

	if (pData->m_Next)
		pData->m_Next->GetInternalData()->m_Previous = this;
#elif __WIN32PC
	D3DInternalData* pData = GetInternalData();
	pData->reserved        = 0;
	pData->m_Pool		   = 0;
	pData->m_LockType	   = 0;
	pData->m_uOffset		= 0;
	pData->m_uSize			= 0;

	pData->m_Previous = NULL;
	pData->m_Next	  = NULL;
#endif // __WIN32PC && !__RESOURCECOMPILER

#if __WIN32PC && __PAGING
#if !__RESOURCECOMPILER
#if USE_RESOURCE_CACHE
	m_VertexData = preAllocatedMemory;
#else
	m_VertexData = rage_new u8[GetVertexStride() * m_VertCount];

	if (preAllocatedMemory)
	{
		sysMemCpy(m_VertexData, preAllocatedMemory, GetVertexStride() * GetVertexCount());
	} 
#endif // USE_RESOURCE_CACHE
	m_D3DBuffer = 0;
#else // __RESOURCECOMPILER
	if( !g_ByteSwap )
	{

		if(g_usePhysicalMemory && g_sysPlatform == platform::WIN64PC)
		{
			m_VertexData = (u8*)physical_new(GetVertexStride() * m_VertCount, RAGE_VERTEXBUFFER_ALIGNMENT);
		}
		else
		{
			m_VertexData = (u8*)RESOURCE_ALLOCATOR(GetVertexStride() * m_VertCount, RAGE_VERTEXBUFFER_ALIGNMENT);
		}


		m_D3DBuffer = 0;
	}
	else
#endif // !__RESOURCECOMPILER
#endif // __WIN32PC && __PAGING
	{

#if __XENON
		if ( preAllocatedMemory )
		{
			AssertMsg( !sysMemAllocator::GetCurrent().IsBuildingResource(), "preAllocated vertex bufer memory cannot be used at resource creation" );
			m_D3DBuffer = (d3dVertexBuffer*) storage; 
			DWORD usage = bDynamic ? D3DUSAGE_CPU_CACHED_MEMORY : 0;
			XGSetVertexBufferHeader(GetVertexStride() * m_VertCount, usage, 0, 0, m_D3DBuffer);
			// @NOTE if bDynamic is not specified, preAllocated memory should be physical, readwrite, writecombined.
			// In any case, preAllocatedMemory should be aligned to RAGE_VERTEXBUFFER_ALIGNMENT
			Assert((reinterpret_cast<u32>(preAllocatedMemory) & (RAGE_VERTEXBUFFER_ALIGNMENT-1)) == 0);
			XGOffsetResourceAddress(m_D3DBuffer, preAllocatedMemory);
		}
		else 
#elif __WIN32PC
		if ( preAllocatedMemory != NULL )
		{
#if !(__WIN32PC && !__D3D11 && !__RESOURCECOMPILER)
			Warningf("Pre-allocated memory not supported for WIN32PC vertex buffers\n");
#endif
		}
#endif

		if( (__XENON || g_ByteSwap) && sysMemAllocator::GetCurrent().IsBuildingResource() )
		{	
			CompileTimeAssert(sizeof(storage) >= sizeof(D3DVertexBuffer));
			D3DVertexBuffer *vb = (D3DVertexBuffer*)storage;
			u32 Length = GetVertexStride() * m_VertCount;
			u8* baseLock = (g_ByteSwap && bDynamic) ? rage_aligned_new (RAGE_VERTEXBUFFER_ALIGNMENT) u8[Length] : (u8*)physical_new(Length,RAGE_VERTEXBUFFER_ALIGNMENT);
			m_VertexData = baseLock;

			vb->Common = D3DCOMMON_TYPE_VERTEXBUFFER | D3DCOMMON_CPU_CACHED_MEMORY;
			vb->Fence = 0;
			vb->ReferenceCount = 1;
			vb->ReadFence = 0;
			vb->Identifier = 0;
			vb->BaseFlush = D3DFLUSH_INITIAL_VALUE;
			vb->Format.dword[0] = 3/*VERTEX*/ | (u32)baseLock;
			vb->Format.dword[1] = 2/*ENDIAN*/ | Length | (/* REQUESTSIZE = 512bit */ 1<<28);

			m_D3DBuffer = (d3dVertexBuffer*) vb;
		}
		else
			CreateInternal();
	}
}

void grcVertexBufferD3D::CreateInternal()
{
	int nUsage = 0; // m_Usage;

#if __WIN32PC
	D3DPOOL ePool = (D3DPOOL)GRCDEVICE.GetBufferResourcePool();
	if (IsDynamic())
	{
#if TRUE_DYNAMIC
		nUsage |= D3DUSAGE_DYNAMIC;
		ePool = (D3DPOOL)RESOURCE_UNMANAGED;
#else // TRUE_DYNAMIC
		ePool = (D3DPOOL)RESOURCE_MANAGED;
#endif // TRUE_DYNAMIC
	}

#if !__RESOURCECOMPILER
	GetInternalData()->m_Pool = ePool;
#endif // !__RESOURCECOMPILER

	if (ePool == RESOURCE_UNMANAGED)
	{
		nUsage |= D3DUSAGE_WRITEONLY;
	}
#else // __WIN32PC
	D3DPOOL ePool = D3DPOOL_MANAGED;
#endif // __WIN32PC

#if __XENON
	nUsage |= D3DUSAGE_CPU_CACHED_MEMORY;
#endif
	ASSERT_ONLY(grcDevice::Result uReturnCode = 0);
#if __D3D9
	{
		Assert(m_VertCount);
		ASSERT_ONLY(uReturnCode =) GRCDEVICE.GetCurrent()->CreateVertexBuffer(GetVertexStride() * m_VertCount, nUsage, 0, ePool, &m_D3DBuffer, 0);
	}
#endif // __WIN32PC

#if __ASSERT
	if (uReturnCode == E_OUTOFMEMORY)
	{
		grcErrorf("Out of memory");
	}
#endif

	Assertf(m_D3DBuffer, "grcVertexBufferD3D::CreateInternal() failed return code:0x%08x - VertexStride=%d m_VertCount=%d nUsage=%d", uReturnCode, GetVertexStride(), m_VertCount, nUsage);
}


static inline void *LockD3DVertexBuffer(d3dVertexBuffer *buf)
{
	void *lock;
	HRESULT hr = buf->Lock(0, 0, &lock, D3DLOCK_NOSYSLOCK WIN32PC_ONLY(|D3DLOCK_DISCARD));
	WIN32PC_ONLY(Assert((hr != DXGI_ERROR_WAS_STILL_DRAWING) && "Failed to Lock Vertex Buffer");)
	(void) hr;
	Assert(lock);
	return lock;
}


#if __XENON

// This is currently disabled, as it is unsafe unless we align vertex buffers
// that may be read-write locked to 128-byte cache line boundaries.  Otherwise
// the DCBF below (WritebackDC) is insufficient, as there could be something
// else sharing the same cache line that gets loaded into the L1 via the
// read-only EA alias.
//
// Since we aren't really making use of write-combined memory, this doesn't
// matter anyways.
//
#define ENABLE_360_READ_ONLY_CACHED_ALIAS   0

void grcVertexBufferD3D::LockInternal(bool invalidateReadOnlyCache) const
{
#if ENABLE_360_READ_ONLY_CACHED_ALIAS
	// This is a workaround for the 360 hardware bug where the CPU will lockup
	// if a read is performed from the non-cachable, write-combined ea alias,
	// while the same physical memory is also cached in the L1 of the same core
	// as a read-only ea alias.  This is an issue with 64KB pages only.  To
	// prevent this, we flush the cached read-only alias when first performing a
	// read-write lock.  This will also evict it from the cache on any other
	// core, but that is not a big issue.
	//
	// Note that because of this bug, it is extremely important that if a
	// write-only lock is requested, then it really is never read from.
	//
	if (invalidateReadOnlyCache)
	{
		const void *const roAlias = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(
			(void*)(static_cast<const grcVertexBufferD3D*>(this)->GetD3DBuffer()->Format.dword[0] & ~3));
		const unsigned bytes = GetVertexStride() * m_VertCount;
		const bool flush = true;
		WritebackDC(roAlias, bytes, flush);
	}
#else
	(void) invalidateReadOnlyCache;
#endif

	m_LockPtr = LockD3DVertexBuffer(m_D3DBuffer);
}


const void *grcVertexBuffer::LockRO() const
{
	VBSTATS_ONLY(PF_FUNC(LockRO);)
#if ENABLE_360_READ_ONLY_CACHED_ALIAS
	m_LockPtr = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(
		(void*)(static_cast<const grcVertexBufferD3D*>(this)->GetD3DBuffer()->Format.dword[0] & ~3));
#else
	m_LockPtr = 
		(void*)(static_cast<const grcVertexBufferD3D*>(this)->GetD3DBuffer()->Format.dword[0] & ~3);
#endif
	Assert(m_LockPtr);
	return m_LockPtr;
}


const void *grcVertexBuffer::GetUnsafeReadPtr() const
{
	return (const void*)(static_cast<const grcVertexBufferD3D*>(this)->GetD3DBuffer()->Format.dword[0] & ~3);
}

#undef ENABLE_360_READ_ONLY_CACHED_ALIAS

#endif // __XENON


void grcVertexBufferD3D::UnlockInternal() const
{
#if __D3D9

	VBSTATS_ONLY(PF_FUNC(Unlock);)

#if __WIN32PC && __PAGING
	if (__RESOURCECOMPILER || sysMemAllocator::GetCurrent().IsBuildingResource())
	{
		return;
	}
#endif

	if (!m_D3DBuffer)
	{
		return;
	}

#if !__XENON
	// Lock the actual D3D vertex buffer
	void *const d3dBuf = LockD3DVertexBuffer(m_D3DBuffer);

	// Copy data to D3D vertex buffer
	sysMemCpy(d3dBuf, m_VertexData, m_Fvf->GetTotalSize() * m_VertCount);
#endif // !__XENON

	// Unlock the actual D3D vertex buffer
	AssertVerify(((IDirect3DVertexBuffer9*)m_D3DBuffer)->Unlock() == D3D_OK);
#if __XENON
	// ensure that the GPU sees the modified data
	WritebackDC(m_LockPtr, m_Stride * m_VertCount);
#endif

#endif // __D3D9
}


#if __WIN32PC && !__RESOURCECOMPILER
bool grcVertexBufferD3D::Lost()
{
	if (m_D3DBuffer)
	{
		if (GetInternalData()->m_Pool == RESOURCE_UNMANAGED)
		{
			SAFE_RELEASE_RESOURCE(m_D3DBuffer);
			m_D3DBuffer = NULL;
		}
	}
	return true;
}

bool grcVertexBufferD3D::Reset()
{
	if (m_D3DBuffer == NULL)
	{
		CreateInternal();
	}
	return true;
}

#endif // __WIN32PC && !__RESOURCECOMPILER

#if __DECLARESTRUCT
inline void datSwapper(DWORD &d) { datSwapper(reinterpret_cast<u32&>(d)); }

void grcVertexBufferD3D::DeclareStruct(datTypeStruct &s)
{
	grcVertexBuffer::DeclareStruct(s);
	STRUCT_BEGIN(grcVertexBufferD3D);
	D3DVertexBuffer *vb = (D3DVertexBuffer*) m_D3DBuffer;
	if (g_ByteSwap) {
		datSwapper(vb->Common);
		datSwapper(vb->Fence);
		datSwapper(vb->ReferenceCount);
		datSwapper(vb->ReadFence);
		datSwapper(vb->Identifier);
		datSwapper(vb->BaseFlush);
		datSwapper((void*&)vb->Format.dword[0]);
		datSwapper(vb->Format.dword[1]);
	}
	STRUCT_FIELD_VP(m_D3DBuffer);
	STRUCT_SKIP(storage,sizeof(storage));
	STRUCT_END();
}
#endif	// __FINAL

} // namespace rage

#endif
