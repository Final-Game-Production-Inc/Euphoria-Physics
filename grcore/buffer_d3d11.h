
#ifndef _GRCORE_BUFFER_D3D11_H
#define _GRCORE_BUFFER_D3D11_H

#include "grcore/config.h"
#include "grcore/device.h"
#include "grcore/locktypes.h"

struct ID3D11Buffer; 

#define DYNAMIC_PLACEMENT_VB_ALIGNMENT		( 64 )
#define GRC_BUFFER_ALIGN(x, y)				(((x) + (y - 1)) & ~(y - 1))
#define STAGING_SIZE						32
#define STAGING_QUEUE						512

namespace rage
{

enum grcBufferMiscFlags
{
	grcBufferMisc_None					= 0x0,
	grcBufferMisc_GenerateMips			= 0x1L,		//D3D11_RESOURCE_MISC_GENERATE_MIPS
#if !RSG_DURANGO
	grcBufferMisc_Shared				= 0x2,		//D3D11_RESOURCE_MISC_SHARED
#endif
	grcBufferMisc_TextureCub			= 0x4L,		//D3D11_RESOURCE_MISC_TEXTURECUBE
	grcBufferMisc_DrawIndirectArgs		= 0x10L,	//D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS
	grcBufferMisc_AllowRawViews			= 0x20L,	//D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
	grcBufferMisc_BufferStructured		= 0x40L,	//D3D11_RESOURCE_MISC_BUFFER_STRUCTURED
	grcBufferMisc_ResourceClamp			= 0x80L,	//D3D11_RESOURCE_MISC_RESOURCE_CLAMP
	grcBufferMisc_SharedMutex			= 0x100L,	//D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX
#if !RSG_DURANGO
	grcBufferMisc_GDICompatible			= 0x200L	//D3D11_RESOURCE_MISC_GDI_COMPATIBLE
#endif
};

enum grcUAVBufferFlags
{
	grcBuffer_UAV_FLAG_RAW       = 0x1,		//D3D11_BUFFER_UAV_FLAG_RAW
	grcBuffer_UAV_FLAG_APPEND    = 0x2,		//D3D11_BUFFER_UAV_FLAG_APPEND
	grcBuffer_UAV_FLAG_COUNTER   = 0x4		//D3D11_BUFFER_UAV_FLAG_COUNTER
};

#if RSG_PC && __D3D11

enum grcBufferCreateType
{
	grcsBufferCreate_NeitherReadNorWrite	= 0x0, // Expects data to be available upon creation (equivalent to D3D11_USAGE_IMMUTABLE)
	grcsBufferCreate_ReadOnly				= 0x1, // Buffer can only be read from (equivalent to D3D11_USAGE_IMMUTABLE + CPU copy).
	grcsBufferCreate_ReadWrite				= 0x2, // Buffer can be read from and written to (equivalent to D3D11_USAGE_DEFAULT + CPU copy).
	grcsBufferCreate_ReadWriteOnceOnly		= 0x3, // Buffer can be read from and written to once only (equivalent to D3D11_USAGE_DEFAULT + temporary CPU copy).
	grcsBufferCreate_ReadDynamicWrite		= 0x4, // Buffer can only be written to. (equivalent to D3D11_USAGE_DYNAMIC + CPU copy)
	grcsBufferCreate_DynamicWrite			= 0x5, // Buffer can only be written to. (equivalent to D3D11_USAGE_DYNAMIC)
};


enum grcsBufferSyncType
{
	grcsBufferSync_Mutex			= 0x0, // Uses a mutex on the buffer, minimal stalls, can be called only once per frame, "frame skips" in GPU copy possible.
	grcsBufferSync_DirtySemaphore	= 0x1, // Uses a semaphore to prevent overwriting data before the GPU has been updated. Supports calling once per frame only, could incur long stalls.
	grcsBufferSync_CopyData			= 0x2, // Makes a copy of the data. Supports multiple calls per frame but has to copy buffer contents.
	grcsBufferSync_None				= 0x3, // Expects only to be updated on the render thread or not at all.
};

class grcBufferD3D11
{
	// Construction/Destruction related functions.
public:
	grcBufferD3D11();
	~grcBufferD3D11();
public:
	void Initialise(u32 Count, u32 Stride, u32 BindType, grcBufferCreateType CreateType, grcsBufferSyncType ThreadBoundaryMigrationType, void *pExistingData, bool AllocateCPUCopy, u32 miscFlags = grcBufferMisc_None);
	void CleanUp();
	static void ShutdownClass();

	// Lock()/Unlock() related functions.
public:
	void *Lock(u32 Flags, u32 Offset, u32 Size) const;
	void Unlock() const;
	// Updates the GPU version. Can only be a called from the render thread.
	void UpdateGPUCopy(void *pData, u32 Offset, u32 Size) const;
#if RSG_PC
	void UpdateGPUCopy();
#endif // RSG_PC
	void PointerFixup(datResource& rsc) { rsc.PointerFixup(m_pCPUCopyOfData); }

	const void *GetUnsafeReadPtr() const;
	bool OwnsCopy() const { return m_WeOwnCPUCopy; }

	// Internal Lock/Unlock etc.
private:
	// Perform the Lock().
	void *Lock_Internal(u32 Flags, u32 Offset, u32 Size, bool AllowContextLock) const;
	// Unlocks the buffer and returns true if the GPU version needs updating.
	void Unlock_Internal(bool AllowContextLock) const;
	// Updates GPU version with thread synchronization.
	void UpdateGPUCopy_WithThreadSyncing(void *pData, u32 Offset, u32 Size) const;
	// Updates the GPU version.
	void UpdateGPUCopy_Internal(void *pData, u32 Offset, u32 Size) const;

	// Allocates memory for the CPU side copy buffer.
	void AllocCPUCopy(u32 Size);
	// Free the CPU copy.
	void FreeCPUCopy();

	// Helper function for interpreting offset and size.
	void InterpretOffsetAndSize(u32 &o_Offset, u32 &o_Size) const;

	static void InitialiseStagingBufferCache();
	static ID3D11Buffer *GetStagingBuffer(u32 Size);
	static void *AllocTempMemory(u32 Size);

	// Access functions.
public:
	ID3D11Buffer *GetD3DBuffer();
	// Sets the callback function to insert GPU updates into the draw list (or equivalent) for the calling thread.
	static void SetInsertUpdateCommandIntoDrawListFunc(void (*pFunc)(void *, void *, u32 Offset, u32 Size, bool uponCancelMemoryHint)) { sm_pInsertUpdateGPUCopyCommandIntoDrawList = pFunc; }
	// Sets the cancel update GPU copy call back.
	static void SetCancelPendingUpdateGPUCopy(void (*pFunc)(void *)) { sm_pCancelPendingUpdateGPUCopy = pFunc; }
	// Sets the temp allocator call back.
	static void SetAllocTempMemory(void *(*pFunc)(u32)) { sm_pAllocTempMemory = pFunc; }

private:
	bool GetOwnsCPUCopy() { return m_WeOwnCPUCopy; }

	// Data members.
private:
	ID3D11Buffer *m_pD3DBuffer;
	void *m_pCPUCopyOfData;
	mutable u32 m_LockOffset;
	mutable u32 m_LockSize;
	mutable u32 m_CreateType : 3,
				m_ThreadBoundaryMigration : 2,
				m_WeOwnCPUCopy: 1,
				m_HasBeenDeleted: 1,
				m_LockType : 7,
				m_Dirty : 1;
		union
		{
			sysIpcMutex m_Mutex;
			sysIpcSema m_DirtySemaphore;
		};

	static ID3D11Buffer *sm_pStagingBuffers[STAGING_SIZE][STAGING_QUEUE];
	static u32 sm_aFrameUsed[STAGING_SIZE][STAGING_QUEUE];
	static u32 sm_auActiveIndex[STAGING_SIZE];
	static u32 sm_auPrecacheSize[STAGING_SIZE];
	static u32 sm_auStagingPrecacheSize[STAGING_SIZE];	
	static bool sm_bInitializeCached;
	static bool sm_bUseStagingToUpdate;

	// Called when we need to get a GPU update inserted into the "draw list" or similar.
	static __THREAD void (*sm_pInsertUpdateGPUCopyCommandIntoDrawList)(void *, void *, u32 Offset, u32 Size, bool uponCancelMemoryHint);
	// Called when a buffer's pending GPU updates needs to be canceled.
	static __THREAD void (*sm_pCancelPendingUpdateGPUCopy)(void *);
	// Callback to provide memory for game thread buffer updates - must be live for 2 frames.
	static void *(*sm_pAllocTempMemory)(u32 size);

	friend class grcVertexBufferD3D11;
	friend class dlCmdGrcDeviceUpdateBuffer;
	friend class grcBufferD3D11Resource;
	friend class grcDevice;
};

class grcBufferD3D11Resource : public grcBufferD3D11
{
public:
	grcBufferD3D11Resource();
	~grcBufferD3D11Resource();
	void Initialise(u32 Count, u32 Stride, u32 BindType, grcBufferCreateType CreateType, grcsBufferSyncType ThreadBoundaryMigrationType, void *pExistingData, bool AllocateCPUCopy, u32 miscFlags = grcBufferMisc_None, u32 UAVBufferFlags = 0);
	void CleanUp();

	grcDeviceView* GetShaderResourceView()  const { return m_SRView; }
	grcDeviceView* GetUnorderedAccessView() const { return m_UAView; }

private:

	grcDeviceView *m_SRView;
	grcDeviceView *m_UAView;
};

/*--------------------------------------------------------------------------------------------------*/
/* Lock()/Unlock() related functions.																*/
/*--------------------------------------------------------------------------------------------------*/

inline void *grcBufferD3D11::Lock(u32 Flags, u32 Offset, u32 Size) const
{
	return Lock_Internal(Flags, Offset, Size, false);
}


inline void grcBufferD3D11::Unlock() const
{
	Unlock_Internal(false);
}

inline const void *grcBufferD3D11::GetUnsafeReadPtr() const
{
	return m_pCPUCopyOfData;
}

#endif // RSG_PC && __D3D11

} // namespace rage


#endif // _GRCORE_BUFFER_D3D11_H

