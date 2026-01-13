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

#include "system/buddyallocator_config.h"
#include "system/memory.h"
#include "system/ipc.h"
#include "system/platform.h"
#include "system/xtl.h"
#include "system/d3d10.h"
#include "diag/tracker.h"
#include "resourcecache.h"

#if	RSG_PC && __D3D11

#include "grcore/d3dwrapper.h"

#define SLEEPWAITS	__OPTIMIZED // PIX doesn't handle do not wait properly


namespace rage
{

bool ValidateIndexData(u16* puSrcData, u32 uCount)
{
	if (puSrcData == NULL)
		return true;

	// Assume index should be at most 3 * count of indices
	const u32 uMaxValue = uCount * 3;
	for (u32 uOffset = 0; uOffset < uCount; uOffset++)
	{
		if (puSrcData[uOffset] > uMaxValue)
		{
			Warningf("Index Offset %x - %d of %d is offset %d", puSrcData, uOffset, uCount, puSrcData[uOffset]);
			memset(puSrcData, 0, sizeof(u16) * uCount);
			return false;
		}
	}
	return true;
}

/*--------------------------------------------------------------------------------------------------*/
/* grcIndexManager functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

grcIndexBufferD3D11* grcIndexBufferD3D11::sm_First = NULL;
sysCriticalSectionToken s_IndexBufferList;

#if RSG_PC
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
	grcIndexBufferD3D11* t = grcIndexBufferD3D11::sm_First;
	while (t) 
	{
		AssertVerify(t->Lost());
		const grcIndexBufferD3D11::D3D11InternalData* pData = (const grcIndexBufferD3D11::D3D11InternalData*)t->storage;
		t = pData->m_Next;
	}
	return true;
}

bool grcIndexManager::Reset()
{
	grcIndexBufferD3D11 *t = grcIndexBufferD3D11::sm_First;
	while (t) 
	{
		AssertVerify(t->Reset());
		const grcIndexBufferD3D11::D3D11InternalData* pData = (const grcIndexBufferD3D11::D3D11InternalData*)t->storage;
		t = pData->m_Next;
	}
	return true;
}
#endif

/*--------------------------------------------------------------------------------------------------*/
/* Construction/Destruction related functions.														*/
/*--------------------------------------------------------------------------------------------------*/


grcIndexBufferD3D11::grcIndexBufferD3D11(int indexCount, grcBufferCreateType CreateType) : grcIndexBuffer(indexCount)
{
	D3D11InternalData* pData = GetInternalData();
	pData->m_Previous = NULL;
	pData->m_Next	  = NULL;
	CreateInternal(CreateType, true);
}

grcIndexBufferD3D11::grcIndexBufferD3D11(int indexCount, grcBufferCreateType CreateType, u16 *pIndexData, bool AllocateCPUCopy) : grcIndexBuffer(indexCount)
{
	D3D11InternalData* pData = GetInternalData();
	pData->m_Previous = NULL;
	pData->m_Next	  = NULL;
	// Record the buffer details.
	Assert(pIndexData);
	m_IndexData = pIndexData;
	Assert(ValidateIndexData(m_IndexData, indexCount));
	CreateInternal(CreateType, AllocateCPUCopy);
	if (!AllocateCPUCopy)
		m_IndexData = NULL;
}


grcIndexBufferD3D11::grcIndexBufferD3D11(datResource& rsc) : grcIndexBuffer(rsc)
{
	D3D11InternalData* pData = (D3D11InternalData*)storage;

	if (datResource_IsDefragmentation) {
		SYS_CS_SYNC(s_IndexBufferList);
		// Repair linked list nodes (including the head!)
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
		SYS_CS_SYNC(s_IndexBufferList);
		pData->m_Previous = NULL;
		pData->m_Next	  = sm_First;

		sm_First = this;

		if (pData->m_Next)
			pData->m_Next->GetInternalData()->m_Previous = this;
	}

	// TODO: Can we remove this branch?  If this assert doesn't fire, then should be ok.
	Assert(m_IndexData);
#if __DEV
	if (!m_IndexData)
		return;
#endif // __DEV
	Assert(ValidateIndexData(m_IndexData, GetIndexCount()));
	CreateInternal(/*grcsBufferCreate_NeitherReadNorWrite*/ grcsBufferCreate_ReadOnly, false);

	CHECK_FOR_PHYSICAL_PTR(m_IndexData);
}

grcIndexBufferD3D11::~grcIndexBufferD3D11()
{
	ReleaseD3DBuffer();
}
// Line by line identical with the destructor
void grcIndexBufferD3D11::ReleaseD3DBuffer()
{
	if (GetInternalData()->m_Buffer.OwnsCopy())
		m_IndexData = NULL;

	GetInternalData()->m_Buffer.CleanUp();

	D3D11InternalData* pData = GetInternalData();

	SYS_CS_SYNC(s_IndexBufferList);
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
}

#define RAGE_INDEX_BUFFER_THREAD_ACCESS_TYPE grcsBufferThreadBoundaryMigration_CopyData

void grcIndexBufferD3D11::CreateInternal(grcBufferCreateType CreateType, bool AllocateCPUCopy)
{
	RESOURCE_CACHE_MANAGE_ONLY(AllocateCPUCopy |= true);

	GetInternalData()->m_Buffer.Initialise(GetIndexCount(), sizeof(u16), grcBindIndexBuffer, CreateType, 
		// DX11 TODO:- Make this a parameter.
		CreateType == grcsBufferCreate_ReadWrite ? grcsBufferSync_Mutex : grcsBufferSync_None, 
		m_IndexData, AllocateCPUCopy);

	Assert(ValidateIndexData(m_IndexData, GetIndexCount()));

	if (AllocateCPUCopy)
		m_IndexData = (u16*)GetInternalData()->m_Buffer.GetUnsafeReadPtr();
}


#if TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO
void grcIndexBufferD3D11::RecreateInternal()
{
	CreateInternal(/*grcsBufferCreate_NeitherReadNorWrite*/ grcsBufferCreate_ReadOnly, false);
}
#endif //TEMP_RUNTIME_CALCULATE_TERRAIN_OPEN_EDGE_INFO


/*--------------------------------------------------------------------------------------------------*/
/* Lock()/Unlock() related functions.																*/
/*--------------------------------------------------------------------------------------------------*/

#if __RESOURCECOMPILER
#	define GRCINDEXBUFFERD3D11  grcIndexBufferD3D11
#else
#	define GRCINDEXBUFFERD3D11  grcIndexBuffer
#endif

u16* GRCINDEXBUFFERD3D11::Lock(u32 flags, u32 offset, u32 size) const
{
	const grcIndexBufferD3D11 *const self = static_cast<const grcIndexBufferD3D11*>(this);
	Assert(self->GetInternalData()->m_Buffer.GetD3DBuffer());
	u16* puIndexBuffer = (u16*)self->GetInternalData()->m_Buffer.Lock(flags, offset, size);
	Assert(!AreLockFlagsReadOnly(flags) || ValidateIndexData(puIndexBuffer, GetIndexCount()));
	Assert(puIndexBuffer != NULL);
	return puIndexBuffer;
}


void GRCINDEXBUFFERD3D11::UnlockRO() const
{
	const grcIndexBufferD3D11 *const self = static_cast<const grcIndexBufferD3D11*>(this);
	Assert(self->GetInternalData()->m_Buffer.GetD3DBuffer());
	self->GetInternalData()->m_Buffer.Unlock();
}


void GRCINDEXBUFFERD3D11::UnlockRW() const
{
	const grcIndexBufferD3D11 *const self = static_cast<const grcIndexBufferD3D11*>(this);
	Assert(self->GetInternalData()->m_Buffer.GetD3DBuffer());
	self->GetInternalData()->m_Buffer.Unlock();
}


void GRCINDEXBUFFERD3D11::UnlockWO() const
{
	UnlockRW();
}


const u16* GRCINDEXBUFFERD3D11::LockRO() const
{
	return Lock(grcsRead);
}


u16* GRCINDEXBUFFERD3D11::LockRW() const
{
	return Lock(grcsRead | grcsWrite);
}


u16* GRCINDEXBUFFERD3D11::LockWO() const
{
	return Lock(grcsWrite | grcsDiscard);
}


const void* GRCINDEXBUFFERD3D11::GetUnsafeReadPtr() const
{
	const grcIndexBufferD3D11 *const self = static_cast<const grcIndexBufferD3D11*>(this);
	Assert(self->GetInternalData()->m_Buffer.GetD3DBuffer());
	return (u16*)self->GetInternalData()->m_Buffer.GetUnsafeReadPtr();
}

#undef GRCINDEXBUFFERD3D11

/*--------------------------------------------------------------------------------------------------*/
/* Device reset/lost.																				*/
/*--------------------------------------------------------------------------------------------------*/


bool grcIndexBufferD3D11::Lost()
{
	// DX11 TODO:- Determine what we`re supposed to do here.
	return true;
}

bool grcIndexBufferD3D11::Reset()
{
	// DX11 TODO:- Determine what we`re supposed to do here.
	return true;
}


/*--------------------------------------------------------------------------------------------------*/
/* Resource builder functions.																		*/
/*--------------------------------------------------------------------------------------------------*/

// Needed for non-unity build.
#include "grcore/xenon_resource.h"


#if __DECLARESTRUCT
extern void datSwapperD3D11(DWORD &d);

void grcIndexBufferD3D11::DeclareStruct(datTypeStruct &s)
{
	grcIndexBuffer::DeclareStruct(s);
	STRUCT_BEGIN(grcIndexBufferD3D11);
	D3DIndexBuffer *ib = (D3DIndexBuffer*) m_D3DBuffer_Unused;
	if (g_ByteSwap) {
		datSwapperD3D11(ib->Common);
		datSwapperD3D11(ib->Fence);
		datSwapperD3D11(ib->ReferenceCount);
		datSwapperD3D11(ib->ReadFence);
		datSwapperD3D11(ib->Identifier);
		datSwapperD3D11(ib->BaseFlush);
		datSwapperD3D11((DWORD &)ib->Address);
		datSwapperD3D11(ib->Size);
	}
	STRUCT_FIELD_VP(m_D3DBuffer_Unused);
	STRUCT_SKIP(storage,sizeof(storage));
	STRUCT_END();
}
#endif

}

#endif // RSG_PC && __D3D11

