
#include "buffer_d3d11.h"
#include "channel.h"
#include "wrapper_d3d.h"

#include "math/amath.h"
#include "system/ipc.h"
#include "system/xtl.h"
#include "system/d3d11.h"

#include "system/memory.h"
#include "system/param.h"

#include "paging/rscbuilder.h"
#include "profile/timebars.h"
#include "resourcecache.h"

namespace rage
{
#if RSG_PC && __D3D11

/*--------------------------------------------------------------------------------------------------*/
/* Construction/Destruction related functions.														*/
/*--------------------------------------------------------------------------------------------------*/
#define USE_STAGING_TO_UPDATE (0) // AMD is going to fix their driver and make a profile for us until they fix it for all games.

ID3D11Buffer *grcBufferD3D11::sm_pStagingBuffers[STAGING_SIZE][STAGING_QUEUE] = { NULL };

u32 grcBufferD3D11::sm_auStagingPrecacheSize[STAGING_SIZE] = { 0, 0, 0, 0, 0, 3, 1, 3, 1, 32, 72, 100, 345, 85, 127, 100, 3, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

//u32 grcBufferD3D11::sm_auPrecacheSize[STAGING_SIZE] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31 };
u32   grcBufferD3D11::sm_auPrecacheSize[STAGING_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 9, 9, 9, 9,12, 9, 9, 9, 9, 9, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

u32 grcBufferD3D11::sm_auActiveIndex[STAGING_SIZE] = { 0 };
u32 grcBufferD3D11::sm_aFrameUsed[STAGING_SIZE][STAGING_QUEUE] = { 0 };
bool grcBufferD3D11::sm_bInitializeCached = false;
bool grcBufferD3D11::sm_bUseStagingToUpdate = USE_STAGING_TO_UPDATE;

PARAM(UseStaging, "Buffers use staging for updating");

// Called when we need to get a GPU update inserted into the "draw list" or similar.
__THREAD void (*grcBufferD3D11::sm_pInsertUpdateGPUCopyCommandIntoDrawList)(void *, void *, u32 Offset, u32 Size, bool uponCancelMemoryHint) = 0;
// Called when a buffer`s pending GPU updates need to be canceled.
__THREAD void (*grcBufferD3D11::sm_pCancelPendingUpdateGPUCopy)(void *) = 0;
// Callback to provide memory for game thread buffer updates - must be live for 2 frames.
void *(*grcBufferD3D11::sm_pAllocTempMemory)(u32 size) = 0;


grcBufferD3D11::grcBufferD3D11()
{
	m_pD3DBuffer = NULL;
	m_CreateType = 0;
	m_ThreadBoundaryMigration = grcsBufferSync_None;
	m_WeOwnCPUCopy = false;
	ASSERT_ONLY(m_HasBeenDeleted = false);
	m_LockType = 0;
	m_LockOffset = 0;
	m_LockSize = 0;
}


grcBufferD3D11::~grcBufferD3D11()
{
	CleanUp();
}

ID3D11Buffer* grcBufferD3D11::GetD3DBuffer()
{
#if RSG_PC
	UpdateGPUCopy();
#endif // RSG_PC
	return m_pD3DBuffer;
}

#if RSG_PC
void grcBufferD3D11::UpdateGPUCopy()
{
#if REUSE_RESOURCE
	if (m_Dirty && GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread())
	{
		u32 Offset = 0;
		u32 Size = 0;
		InterpretOffsetAndSize(Offset, Size);

		D3D11_BOX Box;
		Box.left = 0;
		Box.right = Size;
		Box.top = 0;
		Box.bottom = 1;
		Box.front = 0;
		Box.back = 1;

#if USE_TELEMETRY
		static char szBufferSize[128];
		formatf(szBufferSize, sizeof(szBufferSize) - 1, "UpdateSubresource %d",Size);
		PF_AUTO_PUSH_TIMEBAR_BUDGETED(szBufferSize, 1.0f);
#endif
		g_grcCurrentContext->UpdateSubresource(m_pD3DBuffer, 0, &Box, m_pCPUCopyOfData, Size, Size);
		m_Dirty = false;
	}
#endif // REUSE_RESOURCE
}
#endif // RSG_PC

void grcBufferD3D11::Initialise(u32 Count, u32 Stride, u32 BindType, grcBufferCreateType CreateType, grcsBufferSyncType ThreadBoundaryMigrationType, void *pExistingData, bool AllocateCPUCopy, u32 miscFlags)
{
	ASSERT_ONLY(grcDevice::Result uReturnCode = 0);
#if REUSE_RESOURCE
	m_Dirty = false;
	if ((CreateType == grcsBufferCreate_NeitherReadNorWrite) || (CreateType == grcsBufferCreate_ReadOnly)) 
	{
		CreateType = grcsBufferCreate_ReadWriteOnceOnly;
		if ((pExistingData != NULL) && ((BindType & D3D11_BIND_VERTEX_BUFFER) || (BindType & D3D11_BIND_INDEX_BUFFER)))
			m_Dirty = true;
	}
#endif // REUSE_RESOURCE

	m_CreateType = CreateType;
	m_WeOwnCPUCopy = false;
	m_ThreadBoundaryMigration = ThreadBoundaryMigrationType;

	D3D11_BUFFER_DESC Desc = {0};
	Desc.ByteWidth = Stride*Count;
	Desc.BindFlags = BindType;					
	Desc.MiscFlags = miscFlags;
	Desc.StructureByteStride = Stride;
	// All access is done via the global staging buffer.
	Desc.CPUAccessFlags = grcCPUNoAccess; 

	m_LockSize = Desc.ByteWidth;

	// Create the GPU buffer.
	if((CreateType == grcsBufferCreate_NeitherReadNorWrite) || (CreateType == grcsBufferCreate_ReadOnly))
	{
		Desc.Usage = static_cast<D3D11_USAGE>(grcUsageImmutable);
	}
	else if ((CreateType == grcsBufferCreate_DynamicWrite) || (CreateType == grcsBufferCreate_ReadDynamicWrite))
	{
		Desc.Usage = static_cast<D3D11_USAGE>(grcUsageDynamic);
		Desc.CPUAccessFlags = grcCPUWrite;
	}
	else
	{
		Desc.Usage = static_cast<D3D11_USAGE>(grcUsageDefault);
	}

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = pExistingData;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	m_pD3DBuffer = NULL;
	ASSERT_ONLY(uReturnCode = )GRCDEVICE.GetCurrent()->CreateBuffer(&Desc, 
#if !REUSE_RESOURCE
									(Desc.Usage != D3D11_USAGE_DYNAMIC && pExistingData) ? &InitData : 
#endif // !REUSE_RESOURCE
									NULL, &m_pD3DBuffer);
	Assertf(SUCCEEDED(uReturnCode), "Failed to Create Buffer %x", uReturnCode);
#if 0 && !__FINAL
	char szTempName[256];
	sprintf_s(szTempName, "Buffer Size %d Usage %x", Count, Desc.Usage);
	CHECK_HRESULT(m_pD3DBuffer->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName));
	Displayf(szTempName);
#endif // !__FINAL

	//--------------------------------------------------------------------------------------------------//

	// Allocate the CPU copy if needs be.
	switch(CreateType)
	{
	case grcsBufferCreate_NeitherReadNorWrite:
	case grcsBufferCreate_DynamicWrite:
		{
			m_pCPUCopyOfData = NULL;
			break;
		}
	case grcsBufferCreate_ReadOnly:
		{
			Assertf(pExistingData, "Read only buffers require data upon creation.");
#if !RESOURCE_CACHE_MANAGE_BACKUPS
			m_pCPUCopyOfData = pExistingData;
			AlignedAssertf(size_t(m_pCPUCopyOfData), 16, "grcBufferD3D11::Initialise pointer %x is not aligned properly", m_pCPUCopyOfData);
			CHECK_FOR_PHYSICAL_PTR(m_pCPUCopyOfData);
			break;
#endif // !RESOURCE_CACHE_MANAGE_BACKUPS
		} // PASS THROUGH - Be careful - When resource cache managed back ups is enabled
	case grcsBufferCreate_ReadWrite:
	case grcsBufferCreate_ReadWriteOnceOnly:
	case grcsBufferCreate_ReadDynamicWrite:
		{
			if(AllocateCPUCopy || pExistingData == NULL)
			{
				// Allocate the data. 
				AllocCPUCopy(Count*Stride);

				// Copy the initial data if we have some.
				if(pExistingData)
				{
					memcpy(m_pCPUCopyOfData, pExistingData, Count*Stride);
				}
#if __DEV
				else
				{
					memset(m_pCPUCopyOfData, 0, Count*Stride);
				}
#endif
			}
			else
			{
				m_pCPUCopyOfData = pExistingData;
				AlignedAssertf(size_t(m_pCPUCopyOfData), 16, "grcBufferD3D11::Initialise pointer %x is not aligned properly", m_pCPUCopyOfData);
			}
			break;
		}
		/*
	case grcsBufferCreate_ReadWriteOnceOnly:
		{
			AssertMsg(pExistingData == NULL, "grcBufferD3D11::Initialise()...Passing pre-allocated memory into a grcsBufferCreate_ReadWriteOnceOnly buffer!\n");

			// Allocate the data. 
			AllocCPUCopy(Count*Stride);
			break;
		}
		*/
	default:
		{
			Assertf(0, "%s is not a valid grcBufferCreateType.", CreateType);
			break;
		}
	}

	//--------------------------------------------------------------------------------------------------//

	// Initialise the GPU side if needs be (and we have the data).
	if(((CreateType == grcsBufferCreate_DynamicWrite) || (CreateType == grcsBufferCreate_ReadDynamicWrite)) && (pExistingData != NULL))
	{
		void *pDest = this->Lock_Internal(grcsNoOverwrite /*grcsDiscard*/, 0, 0, true);
		memcpy(pDest, (void *)pExistingData, Stride*Count);
		this->Unlock_Internal(true);
	}

	//--------------------------------------------------------------------------------------------------//

	// Create any synchronization primitives needed.
	if(m_ThreadBoundaryMigration == grcsBufferSync_Mutex)
	{
		m_Mutex = sysIpcCreateMutex();
	}
	else if(m_ThreadBoundaryMigration == grcsBufferSync_DirtySemaphore)
	{
		m_DirtySemaphore = sysIpcCreateSema(1);
	}
	ASSERT_ONLY(m_HasBeenDeleted = false);
}


void grcBufferD3D11::CleanUp()
{
	AssertMsg(m_HasBeenDeleted == false, "Buffer has been deleted.");

	// Are we NOT on the render thread ?
// NOTE: fix for B*1950250 (disable the if statement below). Cloth drawables are deleted during the safemode operation which is run on the render thread
// So the following if statement would have left some cloth deferred updates hanging , those deferred updates need to be canceled
// -Svetli
//	if(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() == false)
	{
		grcAssertf(((sm_pInsertUpdateGPUCopyCommandIntoDrawList != NULL) && (sm_pCancelPendingUpdateGPUCopy != NULL)) || ((sm_pInsertUpdateGPUCopyCommandIntoDrawList == NULL) && (sm_pCancelPendingUpdateGPUCopy == NULL)), "grcBufferD3D11::CleanUp()...No GPU update cancel function set!");

		// Very infrequently buffers are updated and destroyed upon the same frame so we need to cancel pending GPU copy updates.
		if(sm_pCancelPendingUpdateGPUCopy)
		{
			sm_pCancelPendingUpdateGPUCopy((void *)this);
		}
	}
	// Destroy synchronization primitives.
	if(m_ThreadBoundaryMigration == grcsBufferSync_Mutex)
	{
		sysIpcDeleteMutex(m_Mutex);
		m_Mutex = NULL;
	}
	else if(m_ThreadBoundaryMigration == grcsBufferSync_DirtySemaphore)
	{
		sysIpcDeleteSema(m_DirtySemaphore);
		m_DirtySemaphore = NULL;
	}

	if(m_pD3DBuffer)
	{
		// Release the D3D buffer.
		SAFE_RELEASE_RESOURCE(m_pD3DBuffer);
	}

	// Free the CPU copy.
	FreeCPUCopy();

	ASSERT_ONLY(m_HasBeenDeleted = true);
}

void grcBufferD3D11::ShutdownClass()
{
	for(int i=0; i < STAGING_SIZE; i++)
	{
		for (int j=0; j < STAGING_QUEUE; j++)
		{
			if(sm_pStagingBuffers[i][j])
			{
				SAFE_RELEASE_RESOURCE(sm_pStagingBuffers[i][j]);
			}
		}
	}
}

/*--------------------------------------------------------------------------------------------------*/
/* Lock()/Unlock() related functions.																*/
/*--------------------------------------------------------------------------------------------------*/


// Updates the GPU version. Can only be a called from the render thread.
void grcBufferD3D11::UpdateGPUCopy(void *pData, u32 Offset, u32 Size) const
{
	AssertMsg(m_HasBeenDeleted == false, "Buffer has been deleted.");
	AssertMsg( GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread(), "grcBuffer_d3d11::UpdateGPUCopy()...Attempting to update GPU buffer when not on render thread!");
	UpdateGPUCopy_WithThreadSyncing(pData, Offset, Size);
}


// Updates GPU version with thread synchronization.
void grcBufferD3D11::UpdateGPUCopy_WithThreadSyncing(void *pData, u32 Offset, u32 Size) const
{
	AssertMsg(m_HasBeenDeleted == false, "Buffer has been deleted.");

	// Adhere to specified thread boundary mechanism.
	if(m_ThreadBoundaryMigration == grcsBufferSync_Mutex)
	{
		sysIpcLockMutex(m_Mutex);
	}

	UpdateGPUCopy_Internal(pData, Offset, Size);

	// Perform specified thread boundary operations.
	if(m_ThreadBoundaryMigration == grcsBufferSync_Mutex)
	{
		sysIpcUnlockMutex(m_Mutex);
	}
	else if(m_ThreadBoundaryMigration == grcsBufferSync_DirtySemaphore)
	{
		sysIpcSignalSema(m_DirtySemaphore);
	}
}


/*--------------------------------------------------------------------------------------------------*/
/* Internal Lock/Unlock etc.																		*/
/*--------------------------------------------------------------------------------------------------*/


// Perform the Lock().
void *grcBufferD3D11::Lock_Internal(u32 Flags, u32 Offset, u32 Size, bool AllowContextLock) const
{
	AssertMsg(m_HasBeenDeleted == false, "Buffer has been deleted.");

#if 0 && __DEV
	const u32 cuMinFramesBetweenLocks = 3;
	if ((m_LockOffset + cuMinFramesBetweenLocks) > GRCDEVICE.GetFrameCounter())
	{
		Warningf("Locking Buffer on Frame %d but last lock was %d", m_LockOffset, GRCDEVICE.GetFrameCounter());
	}
#endif // __DEV

	// Record the offset and flags etc.
	m_LockOffset = Offset;
	m_LockType = Flags;

	if(Size != 0)
	{
		m_LockSize = Size;
	}

	// Zero flags usually means read/write.
	if(Flags == 0)
	{
		m_LockType = grcsRead | grcsWrite;
	}
	if(Flags & grcsDiscard)
	{
		m_LockType |= grcsNoOverwrite; // grcsDiscard; // grcsNoOverwrite; // ;
	}
	if(Flags & grcsNoOverwrite)
	{
		m_LockType |= grcsNoOverwrite;
	}

#if __ASSERT
	if(m_LockType & grcsRead)
	{
		Assertf(((m_CreateType == grcsBufferCreate_ReadOnly) || (m_CreateType == grcsBufferCreate_ReadWrite) || (m_CreateType == grcsBufferCreate_ReadWriteOnceOnly) || (m_CreateType == grcsBufferCreate_ReadDynamicWrite)), "grcBufferD3D11::Lock()...Creation flags %x don`t allow read access!",m_CreateType);
	}
	if(m_LockType & (grcsWrite | grcsDiscard | grcsNoOverwrite))
	{
		Assertf(((m_CreateType == grcsBufferCreate_ReadWrite) || (m_CreateType == grcsBufferCreate_ReadWriteOnceOnly) || (m_CreateType == grcsBufferCreate_ReadDynamicWrite || (m_CreateType == grcsBufferCreate_DynamicWrite))), "grcBufferD3D11::Lock()...Creation flags %x don`t allow write access!",m_CreateType);
	}
#endif

	//--------------------------------------------------------------------------------------------------//

	// Only perform thread boundary precautions upon writes and NOT on the render thread.
	if((m_LockType & (grcsWrite | grcsDiscard | grcsNoOverwrite)) && (GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() == false) && (AllowContextLock == false))
	{
		// Adhere to specified thread boundary mechanism.
		if(m_ThreadBoundaryMigration == grcsBufferSync_Mutex)
		{
			sysIpcLockMutex(m_Mutex);
		}
		else if(m_ThreadBoundaryMigration == grcsBufferSync_DirtySemaphore)
		{
			sysIpcWaitSema(m_DirtySemaphore);
		}
	}

	if (m_pCPUCopyOfData)
	{
		AlignedAssertf(size_t(m_pCPUCopyOfData), 16, "grcBufferD3D11::LockInternal pointer %x is not aligned properly", m_pCPUCopyOfData);
		// Return the CPU copy.
		return &((u8 *)m_pCPUCopyOfData)[Offset];
	}
	else
	{
		Assert(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() == true);
		if (GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() == true)
		{
			PF_AUTO_PUSH_TIMEBAR_BUDGETED("Lock_Internal", 0.4f);
			D3D11_MAPPED_SUBRESOURCE MappedRes;
			// Map into it.
			HRESULT uReturnCode = g_grcCurrentContext->Map(m_pD3DBuffer, 0, ((m_LockType & grcsDiscard) ? D3D11_MAP_WRITE_DISCARD : ((m_LockType & grcsNoOverwrite) ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE)), 0, &MappedRes);
			Assertf(SUCCEEDED(uReturnCode), "Failed to lock buffer %x", uReturnCode);
			if (SUCCEEDED(uReturnCode))
			{
				AlignedAssertf(size_t(MappedRes.pData), 16, "grcBufferD3D11::LockInternal pointer %x is not aligned properly", MappedRes.pData);
				return &((u8 *)MappedRes.pData)[Offset];
			}
		}
		return NULL;
	}
}


// Unlocks the buffer and returns true if the GPU version needs updating.
void grcBufferD3D11::Unlock_Internal(bool AllowContextLock) const
{
	AssertMsg(m_HasBeenDeleted == false, "Buffer has been deleted.");

	// Have we a read only lock ?
	if((m_LockType & (grcsWrite | grcsDiscard | grcsNoOverwrite)) == 0)
	{
		// There`s nothing to be done.
		return;
	}

	if (m_pD3DBuffer == NULL)
	{
		return;
	}

	if (m_pCPUCopyOfData == NULL)
	{
		Assert(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread() == true);
		// Unmap it.
		g_grcCurrentContext->Unmap(m_pD3DBuffer, 0);
		m_Dirty = false;
		return;
	}

	// Are we on the "render" thread ?
	if(GRCDEVICE.IsCurrentThreadTheDeviceOwnerThread())
	{
		// Allow GPU copy update as normal.
		UpdateGPUCopy_Internal((void *)m_pCPUCopyOfData, m_LockOffset, m_LockSize);
	}
	else
	{
		// We allow full context locking for Lock->Fill->Unlock once only buffers as these are usually only encountered at start up.
		if(AllowContextLock || (m_CreateType == grcsBufferCreate_ReadWriteOnceOnly))
		{
			PF_AUTO_PUSH_TIMEBAR_BUDGETED("Create Lock", 0.4f);
			GRCDEVICE.LockContext();

			// Update the GPU copy.
			UpdateGPUCopy_Internal((void *)m_pCPUCopyOfData, m_LockOffset, m_LockSize);

			// DX11 TODO:- Assert on further Lock()s for grcsBufferCreate_ReadWriteOnceOnly.

			GRCDEVICE.UnlockContext();
		}
		else
		{
			// The GPU side needs to be updated...Perform specified thread boundary operations.
			if(m_ThreadBoundaryMigration == grcsBufferSync_Mutex)
			{
				sysIpcUnlockMutex(m_Mutex);
			}

			// Has a mechanism for dealing with GPU updates been registered ?
			if(sm_pInsertUpdateGPUCopyCommandIntoDrawList)
			{	
				bool isTempMemory = false;
				void *pGPUImage = m_pCPUCopyOfData;

				// Are we to copy the data to prevent it being over-written before the GPU update ?
				if(m_ThreadBoundaryMigration == grcsBufferSync_CopyData)
				{
					u32 Offset = m_LockOffset;
					u32 Size = m_LockSize;
					InterpretOffsetAndSize(Offset, Size);
					pGPUImage = AllocTempMemory(Size);
					
					// NOTE:- A fail safe for this temp allocation returning NULL is to not update the GPU version.
					if(pGPUImage)
					{
						memcpy(pGPUImage, m_pCPUCopyOfData, Size);
					}
					isTempMemory = true;
				}
				// Get a command inserted into the draw list, or similar, to deal with the update.
				if (pGPUImage) sm_pInsertUpdateGPUCopyCommandIntoDrawList((void *)this, pGPUImage, m_LockOffset, m_LockSize, isTempMemory);
			}
			else
			{
				grcWarningf("grcBuffer_d3d11::Unlock_Internal()...GPU version of buffer not updated!\n");
			}
		}
	}
#if 0 && __DEV
	m_LockOffset = GRCDEVICE.GetFrameCounter();
#endif // __DEV
}


// Updates the GPU version. Can only be a called from the render thread.
void grcBufferD3D11::UpdateGPUCopy_Internal(void *pData, u32 Offset, u32 Size) const
{
	AssertMsg(m_HasBeenDeleted == false, "Buffer has been deleted.");
	if (m_pD3DBuffer == NULL)
	{
		Warningf("UpdateGPUCopy_Internal - D3D Buffer is NULL");
		return;
	}
	FatalAssert(pData != NULL);
	if (pData == NULL)
	{
		Warningf("UpdateGPUCopy_Internal - Source Data is NULL");
		return;
	}
	m_Dirty = false;

	InterpretOffsetAndSize(Offset, Size);

	D3D11_BOX Box;
	Box.left = 0;
	Box.right = Size;
	Box.top = 0;
	Box.bottom = 1;
	Box.front = 0;
	Box.back = 1;

	if (sm_bUseStagingToUpdate || m_CreateType == grcsBufferCreate_ReadDynamicWrite || m_CreateType == grcsBufferCreate_DynamicWrite)
	{
		PF_PUSH_TIMEBAR_BUDGETED("GetStagingBuffer", 0.1f);
		D3D11_MAPPED_SUBRESOURCE MappedRes;
		// Obtain a suitable staging buffer.
		ID3D11Buffer *pStagingBuffer = GetStagingBuffer(Size);
		if (pStagingBuffer == NULL)
		{
			Warningf("UpdateGPUCopy_Internal - Staging Buffer is NULL");
			PF_POP_TIMEBAR();
			return;
		}
		PF_POP_TIMEBAR();

		// Map into it.
		PF_PUSH_TIMEBAR_BUDGETED("Map StagingBuffer", 0.1f);
		HRESULT uReturnCode = g_grcCurrentContext->Map(pStagingBuffer, 0, D3D11_MAP_WRITE, 0, &MappedRes);
		Assertf(SUCCEEDED(uReturnCode), "Failed to lock buffer %x", uReturnCode);
		if (SUCCEEDED(uReturnCode))
		{
			// NOTE:- A fail safe for this temp allocation returning NULL is to not update the GPU version.
			if(pData)
			{
				// Copy over our new data.
				memcpy(MappedRes.pData, (void *)(&((u8 *)pData)[Offset]), Size);
			}
			// Unmap it.
			g_grcCurrentContext->Unmap(pStagingBuffer, 0);
		}
		PF_POP_TIMEBAR();

		// Update the GPU data.
		PF_PUSH_TIMEBAR_BUDGETED("CopySubresourceRegion", 0.1f);
		g_grcCurrentContext->CopySubresourceRegion(m_pD3DBuffer, 0, Offset, 0, 0, pStagingBuffer, 0, &Box);
		PF_POP_TIMEBAR();
	}
	else
	{
#if USE_TELEMETRY
		static char szBufferSize[128];
		formatf(szBufferSize, sizeof(szBufferSize) - 1, "UpdateSubresource %d",Size);
		PF_AUTO_PUSH_TIMEBAR_BUDGETED(szBufferSize, 1.0f);
#endif
		g_grcCurrentContext->UpdateSubresource(m_pD3DBuffer, 0, &Box, (void *)(&((u8 *)pData)[Offset]), Size, Size);
	}
}


// Allocates memory for the CPU side copy buffer.
void grcBufferD3D11::AllocCPUCopy(u32 Size)
{
	AssertMsg(m_WeOwnCPUCopy == false, "grcBufferD3D11::AllocCPUCopy()...We already own the CPU copy (i.e. already allocated it)!");
	sysMemStartTemp();

	USE_MEMBUCKET(MEMBUCKET_RESOURCE);
	sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_RESOURCE_VIRTUAL);
	ASSERT_ONLY(++sysMemStreamingCount);
	sysMemAllowResourceAlloc++;
	Size = (u32)pgRscBuilder::ComputeLeafSize(Size, false);
	m_pCPUCopyOfData = pAllocator->Allocate(Size, 16);
	AlignedAssertf(size_t(m_pCPUCopyOfData), 16, "grcBufferD3D11::AllocCPUCopy pointer %x is not aligned properly", m_pCPUCopyOfData);
	sysMemAllowResourceAlloc--;
	ASSERT_ONLY(--sysMemStreamingCount);

#if __DEV
	memset(m_pCPUCopyOfData, 0xCD, Size);
#endif // __DEV
	m_WeOwnCPUCopy = true;

	sysMemEndTemp();
}


// Free the CPU copy.
void grcBufferD3D11::FreeCPUCopy()
{
	if(m_WeOwnCPUCopy)
	{
		sysMemStartTemp();

		USE_MEMBUCKET(MEMBUCKET_RESOURCE);
		sysMemAllocator* pAllocator = sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_RESOURCE_VIRTUAL);
		sysMemAllowResourceAlloc++;
		pAllocator->DeferredFree(m_pCPUCopyOfData);
		sysMemAllowResourceAlloc--;
		m_pCPUCopyOfData = NULL;

		sysMemEndTemp();

		m_WeOwnCPUCopy = false;
	}
}


// Helper function for interpreting offset and size.
void grcBufferD3D11::InterpretOffsetAndSize(u32 &o_Offset, u32 &o_Size) const
{
	Assert(m_pD3DBuffer != NULL);
	// DX11 TODO:- Find suitable place to store these upon creation.
	if(o_Offset == 0 && o_Size == 0)
	{
		D3D11_BUFFER_DESC Desc;
		m_pD3DBuffer->GetDesc(&Desc);
		o_Size = Desc.ByteWidth;
		o_Offset = 0;
	}
#if __ASSERT
	else
	{
		D3D11_BUFFER_DESC Desc;
		m_pD3DBuffer->GetDesc(&Desc);
		Assert(o_Offset+o_Size <= Desc.ByteWidth);
	}
#endif
}

ID3D11Buffer *CreateBuffer(u32 uSize)
{
	D3D11_BUFFER_DESC Desc = {0};

	// Create the buffer used to accommodate an update of the given size.
	Desc.ByteWidth = uSize;
	Desc.CPUAccessFlags = grcCPUWrite | grcCPURead;
	Desc.Usage = static_cast<D3D11_USAGE>(grcUsageStage);
	Desc.BindFlags = 0;					
	Desc.MiscFlags = 0;
	Desc.StructureByteStride = 0;
	ID3D11Buffer* pBuffer = NULL;

	ASSERT_ONLY(grcDevice::Result uReturnCode = )GRCDEVICE.GetCurrent()->CreateBuffer(&Desc, NULL, &pBuffer);
	Assertf(SUCCEEDED(uReturnCode), "Failed to Create Buffer %x", uReturnCode);
#if 0 && !__FINAL
	char szTempName[256];
	sprintf_s(szTempName, "Buffer Size %d Usage %x", uSize, Desc.Usage);
	CHECK_HRESULT(pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName));
	Displayf(szTempName);
#endif // !__FINAL

	return pBuffer;
}

void grcBufferD3D11::InitialiseStagingBufferCache()
{
	if (!sm_bInitializeCached)
	{
		sm_bUseStagingToUpdate = PARAM_UseStaging.Get();
		u32* pauPrecacheSize = sm_bUseStagingToUpdate ? sm_auStagingPrecacheSize : sm_auPrecacheSize;

		for (u32 uIndex = 0; uIndex < STAGING_SIZE; uIndex++)
		{
			for (u32 uQueueIndex = 0; uQueueIndex < pauPrecacheSize[uIndex]; uQueueIndex++)
			{
				sm_pStagingBuffers[uIndex][uQueueIndex] = CreateBuffer(0x1 << uIndex);
			}
		}
		sm_bInitializeCached = true;
	}
}


ID3D11Buffer *grcBufferD3D11::GetStagingBuffer(u32 Size)
{
	InitialiseStagingBufferCache();

	u32 Index = Log2Floor(Size) + 1;

	const u32 cuMinFramesBetweenLocks = 3;
	u32 uQueueIndex = 0;
	u32 uOldestIndex = 0;
	u32 uOldestFrame = ~0U;
	bool bFirstPass = true;
	u32 uStartIndex = sm_auActiveIndex[Index];

	for (uQueueIndex = uStartIndex; uQueueIndex < STAGING_QUEUE; uQueueIndex++)
	{
		if ((sm_aFrameUsed[Index][uQueueIndex] + cuMinFramesBetweenLocks) > GRCDEVICE.GetFrameCounter())
		{
			if (sm_aFrameUsed[Index][uQueueIndex] <= uOldestFrame)
			{
				uOldestFrame = sm_aFrameUsed[Index][uQueueIndex];
				uOldestIndex = uQueueIndex;
			}
			continue;
		}

		if( sm_pStagingBuffers[Index][uQueueIndex] == NULL)
		{
			if (bFirstPass)
			{
				uStartIndex = uQueueIndex = 0;
				bFirstPass = false;
				continue;
			}

			Assertf(grcResourceCache::IsOkToCreateResources() == true, "grcBufferD3D11::GetStagingBuffer()...Increase %d Size %d from initial buffer count o %d.", Index, Size, sm_bUseStagingToUpdate ? sm_auStagingPrecacheSize[Index] : sm_auPrecacheSize[Index]);
			PF_AUTO_PUSH_TIMEBAR_BUDGETED("GetStagingBuffer", 0.4f);
			sm_pStagingBuffers[Index][uQueueIndex] = CreateBuffer(0x1 << Index);
			sm_auPrecacheSize[Index]++;
			sm_auStagingPrecacheSize[Index]++;
		}
		break;
	}
	if (uQueueIndex >= STAGING_QUEUE)
	{
		Warningf("Staging Queue is full for index %d size %d full queue of %d reusing %d from frame %d of frame %d", Index, Size, STAGING_QUEUE, uOldestIndex, sm_aFrameUsed[Index][uOldestIndex], GRCDEVICE.GetFrameCounter());
		sm_auActiveIndex[Index] = uOldestIndex;
		sm_aFrameUsed[Index][uOldestIndex] = GRCDEVICE.GetFrameCounter();
		return sm_pStagingBuffers[Index][uOldestIndex];
	}

	sm_auActiveIndex[Index] = uQueueIndex;
	sm_aFrameUsed[Index][uQueueIndex] = GRCDEVICE.GetFrameCounter();
	return sm_pStagingBuffers[Index][uQueueIndex];
}


void *grcBufferD3D11::AllocTempMemory(u32 Size)
{
	Assertf(sm_pAllocTempMemory, "grcBufferD3D11::AllocTempMemory()....No temp allocator set.");
	return sm_pAllocTempMemory(Size);
}


/****************************************************************************************************************************/
/* grcBufferD3D11Resource class.																							*/
/****************************************************************************************************************************/


grcBufferD3D11Resource::grcBufferD3D11Resource()
{
	m_SRView = NULL;
	m_UAView = NULL;
}


grcBufferD3D11Resource::~grcBufferD3D11Resource()
{
	CleanUp();
}


void grcBufferD3D11Resource::Initialise(u32 Count, u32 Stride, u32 BindType, grcBufferCreateType CreateType, grcsBufferSyncType ThreadBoundaryMigrationType, void *pExistingData, bool AllocateCPUCopy, u32 miscFlags, u32 UAVBufferFlags)
{
	grcBufferD3D11::Initialise(Count, Stride, BindType, CreateType, ThreadBoundaryMigrationType, pExistingData, AllocateCPUCopy, miscFlags);

	bool isRawBuffer = (UAVBufferFlags == grcBuffer_UAV_FLAG_RAW);
	if(BindType & grcBindShaderResource)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = isRawBuffer ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = isRawBuffer ? D3D11_SRV_DIMENSION_BUFFEREX : D3D11_SRV_DIMENSION_BUFFER;
		if(isRawBuffer)
		{
			srvDesc.BufferEx.FirstElement = 0;
			srvDesc.BufferEx.NumElements = Count;
			srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		}
		else
		{
			srvDesc.Buffer.ElementOffset = 0;
			srvDesc.Buffer.ElementWidth = Count;
		}
		GRCDEVICE.GetCurrent()->CreateShaderResourceView(m_pD3DBuffer, &srvDesc, (ID3D11ShaderResourceView**)&m_SRView);

#if 0 && !__FINAL
		char szTempName[256];
		sprintf_s(szTempName, "Shader View Buffer Size %d Bind %x", Count, BindType);
		CHECK_HRESULT(m_SRView->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName));
		Displayf(szTempName);
#endif // !__FINAL
	}
	if(BindType & grcBindUnorderedAccess)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = isRawBuffer ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = UAVBufferFlags;
		uavDesc.Buffer.NumElements = Count;
		GRCDEVICE.GetCurrent()->CreateUnorderedAccessView(m_pD3DBuffer, &uavDesc, (ID3D11UnorderedAccessView**)&m_UAView);
#if 0 && !__FINAL
		char szTempName[256];
		sprintf_s(szTempName, "Unordered View Buffer Size %d Bind %x", Count, BindType);
		CHECK_HRESULT(m_UAView->SetPrivateData(WKPDID_D3DDebugObjectName,(unsigned)strlen( szTempName )+1,szTempName));
		Displayf(szTempName);
#endif // !__FINAL
	}
}

void grcBufferD3D11Resource::CleanUp()
{
	if(m_SRView)
	{
		SAFE_RELEASE_RESOURCE(m_SRView);
		m_SRView = NULL;
	}
	if(m_UAView)
	{
		SAFE_RELEASE_RESOURCE(m_UAView);
		m_UAView = NULL;
	}
}

#endif // RSG_PC && __D3D11

} // namespace rage
