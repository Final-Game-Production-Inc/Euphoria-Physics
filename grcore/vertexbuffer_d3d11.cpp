// 
// grcore/vertexbuffer_d3d.cpp 
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
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

#include "system/buddyallocator_config.h"
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

#if	RSG_PC  && __D3D11

#include "xenon_resource.h"
#include "grcore/d3dwrapper.h"

#define TRUE_DYNAMIC 1 // - Better to use managed instead of dynamic for performance
#define SLEEPWAITS	__OPTIMIZED // PIX doesn't handle do not wait properly
#define USE_STAGING 1 // Should disable this to save memory but some resourced data is using vertex data dynamically (Sample_wilderness)

namespace rage
{

/*--------------------------------------------------------------------------------------------------*/
/* grcVertexManager functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

grcVertexBufferD3D11* grcVertexBufferD3D11::sm_First = NULL;
sysCriticalSectionToken s_VertexBufferList;

#if RSG_PC
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
	grcVertexBufferD3D11 *t = grcVertexBufferD3D11::sm_First;

	while (t) 
	{
		AssertVerify(t->Lost());
		const grcVertexBufferD3D11::D3D11InternalData* pData = (const grcVertexBufferD3D11::D3D11InternalData*)t->storage;
		t = pData->m_Next;
	}
	return true;
}

bool grcVertexManager::Reset()
{
	grcVertexBufferD3D11 *t = grcVertexBufferD3D11::sm_First;
	while (t) 
	{
		AssertVerify(t->Reset());
		const grcVertexBufferD3D11::D3D11InternalData* pData = (const grcVertexBufferD3D11::D3D11InternalData*)t->storage;
		t = pData->m_Next;
	}
	return true;
}
#endif


/*--------------------------------------------------------------------------------------------------*/
/* Construction/Destruction related functions.														*/
/*--------------------------------------------------------------------------------------------------*/


grcVertexBufferD3D11::grcVertexBufferD3D11(int vertCount, const grcFvf& pFvf, bool bReadWrite, bool bDynamic, grcBufferCreateType CreateType, grcsBufferSyncType ThreadBoundaryMigrationType, u8* preAllocatedMemory, bool AllocateCPUCopy) : grcVertexBuffer(vertCount,pFvf,bReadWrite,bDynamic,preAllocatedMemory)
{
	{
		SYS_CS_SYNC(s_VertexBufferList);
		D3D11InternalData *pData = GetInternalData();
		pData->m_Previous = NULL;
		pData->m_Next	  = sm_First;
		sm_First = this;

		if (pData->m_Next)
			pData->m_Next->GetInternalData()->m_Previous = this;
	}

	Assert(m_VertCount);

	// Record the buffer details.
	m_VertCount = vertCount;
	m_VertexData = preAllocatedMemory;
	CreateInternal(CreateType, ThreadBoundaryMigrationType, AllocateCPUCopy);
}


grcVertexBufferD3D11::grcVertexBufferD3D11(class datResource& rsc) : grcVertexBuffer(rsc)
{
	D3D11InternalData* pData = (D3D11InternalData*)storage;

	if (datResource_IsDefragmentation) {
		SYS_CS_SYNC(s_VertexBufferList);
		// Repair linked list nodes	(including the head!)
		rsc.PointerFixup(pData->m_Previous);
		rsc.PointerFixup(pData->m_Next);
		if (pData->m_Previous)
			pData->m_Previous->GetInternalData()->m_Next = this;
		else
			sm_First = this;
		if (pData->m_Next)
			pData->m_Next->GetInternalData()->m_Previous = this;
		pData->m_Buffer.PointerFixup(rsc);
		return;
	}

	{
		SYS_CS_SYNC(s_VertexBufferList);
		pData->m_Previous = NULL;
		pData->m_Next	  = sm_First;

		sm_First = this;

		if (pData->m_Next)
			pData->m_Next->GetInternalData()->m_Previous = this;
	}

	if (m_VertexData)
	{
		if (IsReadWrite())
		{
			CreateInternal(grcsBufferCreate_ReadWrite, grcsBufferSync_Mutex, false);
		}
		else
		{
			CreateInternal(/*grcsBufferCreate_NeitherReadNorWrite*/ grcsBufferCreate_ReadOnly, grcsBufferSync_None, false);
		}
	}

	CHECK_FOR_PHYSICAL_PTR(m_VertexData);
}

grcVertexBufferD3D11::~grcVertexBufferD3D11()
{
	CleanUpInternal();

	D3D11InternalData *pData = GetInternalData();

	SYS_CS_SYNC(s_VertexBufferList);

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
}


void grcVertexBufferD3D11::ReleaseD3DBufferHack()
{
	if (!g_ByteSwap) 
	{
		if( IsValid()  && !sysMemAllocator::GetCurrent().IsBuildingResource() )
		{
			CleanUpInternal();
		}
	}

	D3D11InternalData *pData = GetInternalData();

	SYS_CS_SYNC(s_VertexBufferList);

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
}



void grcVertexBufferD3D11::CreateInternal(grcBufferCreateType CreateType, grcsBufferSyncType ThreadBoundaryMigrationType, bool AllocateCPUCopy)
{
	RESOURCE_CACHE_MANAGE_ONLY(if (CreateType != grcsBufferCreate_DynamicWrite) AllocateCPUCopy |= true);

	GetInternalData()->m_Buffer.Initialise(GetVertexCount(), m_Stride, grcBindVertexBuffer, CreateType, ThreadBoundaryMigrationType, m_VertexData, AllocateCPUCopy);

	if (AllocateCPUCopy)
		m_VertexData = (u8 *)GetInternalData()->m_Buffer.GetUnsafeReadPtr();
}


void grcVertexBufferD3D11::CleanUpInternal()
{
	// If m_Buffer allocated the buffer memory then we don`t want to twice delete it (see grcVertexBuffer base class).
	if(GetInternalData()->m_Buffer.GetOwnsCPUCopy())
	{
		m_VertexData = NULL;
	}

	GetInternalData()->m_Buffer.CleanUp();
}


#if TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO
void grcVertexBufferD3D11::RecreateInternal()
{
	GetInternalData()->m_Buffer.CleanUp();
	CreateInternal(grcsBufferCreate_ReadOnly, grcsBufferSync_None, false);
}
#endif //TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO

/*--------------------------------------------------------------------------------------------------*/
/* Lock()/Unlock() related functions.																*/
/*--------------------------------------------------------------------------------------------------*/


void grcVertexBufferD3D11::LockInternal(u32 flags, u32 offset, u32 size) const
{
	if(AreLockFlagsReadOnly(flags))
	{
		m_LockPtr = m_VertexData;
		Assert(m_LockPtr);
		AlignedAssertf(size_t(m_LockPtr), 16, "grcVertexBufferD3D11::LockInternal pointer %x is not aligned properly", m_LockPtr);
		return;
	}

	Assert(GetInternalData()->m_Buffer.GetD3DBuffer());

	m_LockPtr = GetInternalData()->m_Buffer.Lock(flags, offset, size);
	Assert(m_LockPtr);
#if __ASSERT
	// 16 bytes alignment assert is only valid if the incoming offset is multiple of 16 bytes also (or zero).
	if((offset & 0xf) == 0)
		AlignedAssertf(size_t(m_LockPtr), 16, "grcVertexBufferD3D11::LockInternal pointer %x is not aligned properly", m_LockPtr);
#endif //__ASSERT
}


void grcVertexBufferD3D11::UnlockInternal() const
{
	Assert(GetInternalData()->m_Buffer.GetD3DBuffer());
	GetInternalData()->m_Buffer.Unlock();
}


/*--------------------------------------------------------------------------------------------------*/
/* Device reset/lost.																				*/
/*--------------------------------------------------------------------------------------------------*/


bool grcVertexBufferD3D11::Lost()
{
	// DX11 TODO:- Determine what we`re supposed to do here.
	return true;
}

bool grcVertexBufferD3D11::Reset()
{
	// DX11 TODO:- Determine what we`re supposed to do here.
	return true;
}


/*--------------------------------------------------------------------------------------------------*/
/* Resource builder functions.																		*/
/*--------------------------------------------------------------------------------------------------*/


#if __DECLARESTRUCT
void datSwapperD3D11(DWORD &d) { datSwapper(reinterpret_cast<u32&>(d)); }

void grcVertexBufferD3D11::DeclareStruct(datTypeStruct &s)
{
	grcVertexBuffer::DeclareStruct(s);
	STRUCT_BEGIN(grcVertexBufferD3D);
	D3DVertexBuffer *vb = (D3DVertexBuffer*) m_D3DBuffer_Unused;
	if (g_ByteSwap) {
		datSwapperD3D11(vb->Common);
		datSwapperD3D11(vb->Fence);
		datSwapperD3D11(vb->ReferenceCount);
		datSwapperD3D11(vb->ReadFence);
		datSwapperD3D11(vb->Identifier);
		datSwapperD3D11(vb->BaseFlush);
		datSwapperD3D11((DWORD &)vb->Format.dword[0]);
		datSwapperD3D11(vb->Format.dword[1]);
	}
	STRUCT_FIELD_VP(m_D3DBuffer_Unused);
	STRUCT_SKIP(storage,sizeof(storage));
	STRUCT_END();
}
#endif	// __FINAL


} // namespace rage

#endif // RSG_PC  && __D3D11


