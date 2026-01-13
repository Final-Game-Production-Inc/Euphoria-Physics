//
// grcore/resourcecache.h
//
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved.
//

#ifndef GRCORE_RESOURCECACHE_H
#define GRCORE_RESOURCECACHE_H

#include "grcore/config.h"
#include "system/buddyallocator_config.h"
#include "system/new.h"

#if defined(__RGSC_DLL) && __RGSC_DLL
#define USE_RESOURCE_CACHE 0
#else
#define USE_RESOURCE_CACHE (1 && RSG_PC && !__TOOL && !__RESOURCECOMPILER && __D3D11)
#endif
#define USE_DX9_SINGLE_THREADED (USE_RESOURCE_CACHE)
#define KEEP_SYSTEM_COPIES (!USE_DX9_SINGLE_THREADED)
#define RESOURCE_CACHE_MANAGING (1 && USE_RESOURCE_CACHE)
#define RESOURCE_CACHE_MANAGE_BACKUPS (0 && USE_RESOURCE_CACHE)
#define CONTROL_RATE_OF_CREATIONS (0)

#define REUSE_IMMUTABLE_RESOURCES (0 && USE_RESOURCE_CACHE)
#define REUSE_ALLOW_RESIZING (0 && USE_RESOURCE_CACHE)
#define REUSE_RESOURCE (0 && USE_RESOURCE_CACHE)
#if REUSE_RESOURCE
	#define	REUSE_RESOURCE_ONLY(x) x
#else
	#define	REUSE_RESOURCE_ONLY(x)
#endif



#define LOCK_STACK_DEPTH 32
#define INTERNAL_SYS_MANAGEMENT (1 && USE_RESOURCE_CACHE && RESOURCE_CACHE_MANAGE_BACKUPS)
#define ENABLE_SYSTEMCACHE (1 && REUSE_RESOURCE)
#define USE_CREATION_THREAD 0

#define FREELIST_BUFFER_SIZE 16384

#if USE_RESOURCE_CACHE
	#define RESOURCE_CACHE_ONLY(x) x
	
	#define SAFE_RELEASE_RESOURCE(x) { grcResourceCache::GetInstance().Release(x); x = NULL; }
	#define INSTANT_RELEASE_RESOURCE(x) { grcResourceCache::GetInstance().InstantRelease(x); x = NULL; }
	#if INTERNAL_SYS_MANAGEMENT
		#define ALLOCATE_RESOURCE(eType, uSize) grcResourceCache::GetInstance().Allocate(eType, uSize);
		#define FREE_RESOURCE(eType, pvResource) grcResourceCache::GetInstance().Free(eType, pvResource);
	#else
		#define ALLOCATE_RESOURCE(eType, uSize) sysMemVirtualAllocate(uSize);
		#define FREE_RESOURCE(eType, pvResource) { sysMemVirtualFree(pvResource); pvResource = NULL; }
	#endif

	#if (__FINAL || __NO_OUTPUT)
		#define VIDEOMEMORY_LOCK_RESOURCES grcResourceCache::GetInstance().Lock(MT_Video);
		#define VIDEOMEMORY_UNLOCK_RESOURCES grcResourceCache::GetInstance().Unlock(MT_Video);
		#define SYSTEMMEMORY_LOCK_RESOURCES grcResourceCache::GetInstance().Lock(MT_System);
		#define SYSTEMMEMORY_UNLOCK_RESOURCES grcResourceCache::GetInstance().Unlock(MT_System);
		#define LOCK_DELETION_QUEUE grcResourceCache::GetInstance().LockDeleteQueue();
		#define UNLOCK_DELETION_QUEUE grcResourceCache::GetInstance().UnlockDeleteQueue();
		#define IS_LOCKED_DELETION_QUEUE(x)
	#else
		#define VIDEOMEMORY_LOCK_RESOURCES grcResourceCache::GetInstance().Lock(MT_Video, __FILE__, __LINE__);
		#define VIDEOMEMORY_UNLOCK_RESOURCES grcResourceCache::GetInstance().Unlock(MT_Video, __FILE__, __LINE__);
		#define SYSTEMMEMORY_LOCK_RESOURCES grcResourceCache::GetInstance().Lock(MT_System, __FILE__, __LINE__);
		#define SYSTEMMEMORY_UNLOCK_RESOURCES grcResourceCache::GetInstance().Unlock(MT_System, __FILE__, __LINE__);
		#define LOCK_DELETION_QUEUE grcResourceCache::GetInstance().LockDeleteQueue(__FILE__, __LINE__);
		#define UNLOCK_DELETION_QUEUE grcResourceCache::GetInstance().UnlockDeleteQueue(__FILE__, __LINE__);
		#define IS_LOCKED_DELETION_QUEUE(x) grcResourceCache::GetInstance().IsLockedDeleteQueue(x)
	#endif
	#if USE_CREATION_THREAD
		#define QUEUE_FOR_CREATION(resource) grcResourceCache::GetInstance().QueueForCreation(resource)
		#define CLEAR_FROM_CREATION(resource) grcResourceCache::GetInstance().ClearFromCreationQueue(resource)
	#else
		#define QUEUE_FOR_CREATION(resource)
		#define CLEAR_FROM_CREATION(resource)
	#endif
#else
	#define RESOURCE_CACHE_ONLY(x)
	#define SAFE_RELEASE_RESOURCE(x) if ((x) != NULL) { (x)->Release(); (x) = NULL; }
	#define INSTANT_RELEASE_RESOURCE(x) if ((x) != NULL) { (x)->Release(); (x) = NULL; }
	#define ALLOCATE_RESOURCE(eType, uSize) sysMemVirtualAllocate(uSize);
	#define FREE_RESOURCE(eType, pvResource) { sysMemVirtualFree(pvResource); pvResource = NULL; }

	#define QUEUE_FOR_CREATION(resource)
	#define CLEAR_FROM_CREATION(resource)

	#define VIDEOMEMORY_LOCK_RESOURCES
	#define VIDEOMEMORY_UNLOCK_RESOURCES
	#define SYSTEMMEMORY_LOCK_RESOURCES
	#define SYSTEMMEMORY_UNLOCK_RESOURCES
#endif // USE_RESOURCE_CACHE

#if RESOURCE_CACHE_MANAGE_BACKUPS
	#define RESOURCE_CACHE_MANAGE_ONLY(x) x
	#define CHECK_FOR_PHYSICAL_PTR(x)	
	#define RESOURCE_ALLOCATOR(uSize, uAlignment) physical_new(uSize, uAlignment)
#else
	#define RESOURCE_CACHE_MANAGE_ONLY(x) 
#if RSG_PC && FREE_PHYSICAL_RESOURCES
	#define CHECK_FOR_PHYSICAL_PTR(x) if (sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_RESOURCE_PHYSICAL)->GetPointerOwner(x) != NULL) { x = NULL; /*Warningf("Asset needs to be rebuilt");*/ }
#else
	#define CHECK_FOR_PHYSICAL_PTR(x)
#endif // RSG_PC
	#define RESOURCE_ALLOCATOR(uSize, uAlignment) rage_aligned_new(uAlignment) u8[uSize];
#endif // RESOURCE_CACHE_MANAGING

#if RSG_PC
	#define RESOURCE_DEALLOCATE(ptr) if(sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_RESOURCE_VIRTUAL)->GetPointerOwner(ptr)) { delete[] ptr; } else { physical_delete(ptr); }
#else
	#define RESOURCE_DEALLOCATE(ptr) physical_delete(ptr);
#endif // RSG_PC

#if	USE_RESOURCE_CACHE

#define CREATE_MUTEX(x) x // x = sysIpcCreateMutex()
#define DESTROY_MUTEX(x) x // sysIpcDeleteMutex(x);
#define MUTEX_TYPE sysCriticalSectionToken

#include    <map>
#include	<unordered_map>
#include	"system/xtl.h"

#if (!__D3D11)
#include "grcore/d3dwrapper.h"
//#include	"system/d3d9.h"
#endif // !__D3D11

#include	"dds.h"
#include	"system\ipc.h"
#include	"system\criticalsection.h"

#include	"atl/array.h"
#include	"atl/map.h"
#include	"atl/dlist.h"
#include	"atl/inmap.h"
#include	"atl/inlist.h"
#include	"data/base.h"
#include	"grcore/channel.h"
#include	"effect_config.h"
#include	"grcore/texture.h"
#include	"grcore/indexbuffer.h"
#include	"grcore/vertexbuffer.h"

#define EXPECT_REF_COUNT 0 // Changes with debug runtime to 1

#if (!__D3D11)
// DX9
struct IDirect3DTexture9;
struct IDirect3DVolumeTexture9;
struct IDirect3DCubeTexture9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;
struct IDirect3DSurface9;
#else
// DX10
struct D3D10_TEXTURE1D_DESC;
struct D3D10_SUBRESOURCE_DATA;
struct ID3D10Texture1D;
struct D3D10_TEXTURE2D_DESC;
struct ID3D10Texture2D;
struct D3D10_TEXTURE3D_DESC;
struct ID3D10Texture3D;
struct D3D10_BUFFER_DESC;
struct ID3D10Buffer;

// DX11
struct D3D11_TEXTURE1D_DESC;
struct D3D11_SUBRESOURCE_DATA;
struct ID3D11Texture1D;
struct D3D11_TEXTURE2D_DESC;
struct ID3D11Texture2D;
struct D3D11_TEXTURE3D_DESC;
struct ID3D11Texture3D;
struct D3D11_BUFFER_DESC;
struct ID3D11Buffer;
struct D3D11_DEPTH_STENCIL_VIEW_DESC;
struct D3D11_RENDER_TARGET_VIEW_DESC;
struct D3D11_SHADER_RESOURCE_VIEW_DESC;
struct D3D11_UNORDERED_ACCESS_VIEW_DESC;
struct ID3D11DepthStencilView;
struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11Resource;
#endif

typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef long LONG;
typedef unsigned long ULONG;
typedef LONG HRESULT;

namespace rage {

class grcResourceCache;
class ResourceData;

enum MemoryType
{
	MT_Video = 0,
	MT_System,
	MT_Last
};

enum OSVersion
{
	OS_XP,
	OS_VISTA, // Win7
	OS_LAST
};

enum ResourceType
{
	RT_Texture = 0,
	RT_VolumeTexture,
	RT_CubeTexture,
	RT_RenderTarget,
	RT_DepthStencil,
	RT_ConstantBuffer,
	RT_VertexBuffer, // Vertex - Stream must maintain this order
	RT_IndexBuffer,
	RT_StreamOutput,
	RT_IndirectBuffer,
	RT_Staging,
	RT_UnorderedAccess,
	RT_DSView,
	RT_RTView,
	RT_SRView,
	RT_UNVIew,
	RT_Totals,
	RT_LAST = RT_Totals 
};

typedef atDNode<ResourceData*, datBase> CacheListNode;
typedef bool (*IsMainThreadCallback)();
typedef bool (*GameRunningCallback)();

class SysFreeContainer
{
public:
	SysFreeContainer() {}

	inlist_node<SysFreeContainer> m_AgeList;
	inmap_node<size_t, SysFreeContainer> m_SizeSort;
};


template<typename T>
struct MapAllocator : public std::allocator<T> {
	MapAllocator() {}
	template<class Other> MapAllocator( const MapAllocator<Other>& /*_Right*/ ) {}

	inline typename std::allocator<T>::pointer allocate(typename std::allocator<T>::size_type n, typename std::allocator<void>::const_pointer = 0) 
	{	
		return reinterpret_cast<typename std::allocator<T>::pointer>( ::operator rage_heap_new(n * sizeof(T))  ); 
	}

	inline void deallocate(typename std::allocator<T>::pointer p, typename std::allocator<T>::size_type ) {
		::operator delete(p);
	}

	template<typename U>
	struct rebind {
		typedef MapAllocator<U> other;
	};
};

class grcResourceCache
{
public:
	static void InitClass();		
	static void ShutdownClass();
	static grcResourceCache& GetInstance();

	bool Lock(MemoryType eMemory 
#if !(__FINAL || __NO_OUTPUT)
		, const char* pszFile, u32 uLine
#endif // !__FINAL
		);
	bool Unlock(MemoryType eMemory 
#if !(__FINAL || __NO_OUTPUT)
		, const char* pszFile, u32 uLine
#endif // !__FINAL
		);

	bool LockDeleteQueue(
#if !(__FINAL || __NO_OUTPUT)
		const char* pszFile, u32 uLine
#endif // !__FINAL
		);

	bool UnlockDeleteQueue(
#if !(__FINAL || __NO_OUTPUT)
		const char* pszFile, u32 uLine
#endif // !__FINAL
		);
	bool IsLockedDeleteQueue(int iExpectedStatus = -1);

	static u32 GetCRCForResource(const ResourceData &oResource);

	static OSVersion GetOSVersion();

	template <class T> unsigned long Release(T poResource);
	template <class T> unsigned long InstantRelease(T poResource);
		
	void* Allocate(ResourceType eType, u32 uSize);
	void  Free(ResourceType eType, void* pvResource);

#if !__D3D11
	HRESULT CreateTexture( UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle);
	HRESULT CreateVolumeTexture( UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle);
	HRESULT CreateCubeTexture( UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle);
	HRESULT CreateVertexBuffer( UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle);
	HRESULT CreateIndexBuffer( UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle);
	HRESULT CreateRenderTarget( UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	HRESULT CreateDepthStencilSurface( UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
#else
	HRESULT CreateTexture1D(const D3D10_TEXTURE1D_DESC *pDesc, const D3D10_SUBRESOURCE_DATA *pInitialData, ID3D10Texture1D **ppTexture1D);
	HRESULT CreateTexture2D(const D3D10_TEXTURE2D_DESC *pDesc, const D3D10_SUBRESOURCE_DATA *pInitialData, ID3D10Texture2D **ppTexture2D);
	HRESULT CreateTexture3D(const D3D10_TEXTURE3D_DESC *pDesc, const D3D10_SUBRESOURCE_DATA *pInitialData, ID3D10Texture3D **ppTexture3D);
	HRESULT CreateBuffer(const D3D10_BUFFER_DESC *pDesc, const D3D10_SUBRESOURCE_DATA *pInitialData, ID3D10Buffer **ppBuffer);

	HRESULT CreateTexture1D(const D3D11_TEXTURE1D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture1D **ppTexture1D);
	HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture2D **ppTexture2D);
	HRESULT CreateTexture3D(const D3D11_TEXTURE3D_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Texture3D **ppTexture3D);
	HRESULT CreateBuffer(const D3D11_BUFFER_DESC *pDesc, const D3D11_SUBRESOURCE_DATA *pInitialData, ID3D11Buffer **ppBuffer);

	HRESULT CreateDepthStencilView(ID3D11Resource *pResource, const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc, ID3D11DepthStencilView **ppDepthStencilView);
	HRESULT CreateRenderTargetView(ID3D11Resource *pResource, const D3D11_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView **ppRTView);
	HRESULT CreateShaderResourceView(ID3D11Resource *pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc, ID3D11ShaderResourceView **ppSRView);
	HRESULT CreateUnorderedAccessView(ID3D11Resource *pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc, ID3D11UnorderedAccessView **ppUAView);
#endif // __D3D11

	const char* GetResourceName(ResourceType eResType);
	bool GetName(ID3D11Resource *pResource, char* pszName, int maxLen);

	u32 GetActiveCount(MemoryType eType, ResourceType eResType) const { Assert(eType < MT_Last); Assert(eResType <= RT_LAST); return m_auTotalActiveItems[eType][eResType]; }
	s64 GetActiveMemory(MemoryType eType, ResourceType eResType) const { Assert(eType < MT_Last); Assert(eResType <= RT_LAST); return m_auTypeTotalActiveMemory[eType][eResType]; }

	s64 GetTotalUsedMemory(MemoryType eType) const { Assert(eType <= MT_Last); return GetActiveMemory(eType, RT_Totals) REUSE_RESOURCE_ONLY( + GetCachedMemory(eType, RT_Totals)); }

#if REUSE_RESOURCE
	u32 GetCachedCount(MemoryType eType, ResourceType eResType) const { Assert(eType < MT_Last); Assert(eResType <= RT_LAST); return m_auTotalCacheItems[eType][eResType]; }
	s64 GetCachedMemory(MemoryType eType, ResourceType eResType) const { Assert(eType < MT_Last); Assert(eResType <= RT_LAST); return m_auTypeTotalCachedMemory[eType][eResType]; }
#else
	u32 GetCachedCount(MemoryType /*eType*/, ResourceType /*eResType*/) const { return 0; }
	s64 GetCachedMemory(MemoryType /*eType*/, ResourceType /*eResType*/) const { return 0; }
#endif // REUSE_RESOURCE

	s64 GetTotalMemory(MemoryType eType) const { Assert(eType < MT_Last); return m_aiTotalAvailableMemory[eType]; }
	s64 GetTotalFreeMemory(MemoryType eType, bool bIgnoreCache = true) const;
	s64 GetVideoCardFreeMemory() const;
	s64 GetQueuedMemoryForDeletion(MemoryType eType) const;
	bool UpdateTotalAvailableMemory();
	float GetPercentageForStreaming() const;
	s64 GetStreamingMemory(float fPercentageOfMemoryToUse) const;
	s64 GetStreamingMemory() const;

	static void SetReservedMemory(MemoryType eType, s64 uReservedMemory);
	static void ResetReservedMemory(MemoryType eType);
	static bool ManageResources() { return sm_bManagerResources; }

	static void SetExtraMemory(MemoryType eType, s64 iExtraMemory);
	static s64  GetExtraMemory(MemoryType eType) { Assert(eType < MT_Last); return sm_aiExtraAvailableMemory[eType]; }

	static void SetMemoryRestriction(MemoryType eType, s64 uAmountOfMemory = 0) { Assert(eType < MT_Last); sm_aiMemoryRestriction[eType] = uAmountOfMemory; }
	static s64  GetMemoryRestriction(MemoryType eType) { Assert(eType < MT_Last); return sm_aiMemoryRestriction[eType]; }

	bool Precache();
	bool UnloadCache();
	void ResetStats();

	OSVersion GetOperatingSystem() const { return sm_eOS; }
	bool CanUse3Gigs() const { return sm_bUseLargeMemory; }

	u32 GetNewAddItemsCount(MemoryType eType, ResourceType eResType) const { Assert(eType < MT_Last); Assert(eResType <= RT_LAST); return m_auNewAddItems[eType][eResType]; }

#if REUSE_RESOURCE
	u32 GetNewCacheItemsCount(MemoryType eType, ResourceType eResType) const { Assert(eType < MT_Last); Assert(eResType <= RT_LAST); return m_auNewCacheItems[eType][eResType]; }
	u32 GetNewClearCacheItemsCount(MemoryType eType, ResourceType eResType) const { Assert(eType < MT_Last); Assert(eResType <= RT_LAST); return m_auNewClearCacheItems[eType][eResType]; }
	u32 GetNewUsedCacheItemsCount(MemoryType eType, ResourceType eResType) const { Assert(eType < MT_Last); Assert(eResType <= RT_LAST); return m_auNewUsedCacheItems[eType][eResType]; }
#else
	u32 GetNewCacheItemsCount(MemoryType /*eType*/, ResourceType /*eResType*/) const { return 0; }
	u32 GetNewClearCacheItemsCount(MemoryType /*eType*/, ResourceType /*eResType*/) const { return 0; }
	u32 GetNewUsedCacheItemsCount(MemoryType /*eType*/, ResourceType /*eResType*/) const { return 0; }
#endif // REUSE_RESOURCE

#if USE_CREATION_THREAD
	bool QueueForCreation(grcIndexBuffer* pvResource);
	bool QueueForCreation(grcVertexBuffer* pvResource);
	bool QueueForCreation(grcTexture* pvResource);
	bool ClearFromCreationQueue(void* pvResource);
#endif // USE_CREATION_THREAD
#if !__FINAL
	template <class T> bool Validate(T poResource) const;
#endif // __FINAL

	// PURPOSE: Properly handle a device lost event
	static void DeviceLost();

	// PURPOSE: Properly handle a device reset event
	static void DeviceReset();

	static void SetGameRunningCallback(GameRunningCallback fnGameRunning) { sm_fnGameRunning = fnGameRunning; }
	static void SetPlacementThreadId(sysIpcCurrentThreadId iThreadId) { sm_iPlacementThreadId = iThreadId; }

	static bool IsOkToCreateResources();
#if __ASSERT
#if RSG_PC
	static void SetSafeResourceCreate(bool safe){ sm_SafeCreate = safe; }
	static bool sm_SafeCreate;
#endif
#endif //__ASSERT

#if __BANK
	void DisplayStats();

	static bool m_bDisplayStats;
	static bool m_bDisplaySystemMemStats;
	static bool m_bResetStats;
	static bool m_bSaveResourceData;
	static bool m_bSaveRTs;
	static bool m_bDumpMemoryStats;
	static bool m_bDumpRenderTargetDependencies;
	static s64 sm_iWasteSystemMemory;
	static s64 sm_iWasteVideoMemory;
	void SaveResource(fiStream* fid, ResourceData* poData, u32 uResType);
	void SaveResourceData(bool bEnable = false, const char* poFileName = NULL);
	void SaveRenderTargets(bool bEnable = false, const char* poFileName = NULL);
	void DumpMemoryStats();
	void DumpRenderTargetDependencies();
	void InitWidgets();

	// HACK: ejanderson - Prevents crash at shutdown
	bool IsCacheEmpty() const {return 0 == m_lstItemToFree.GetCount();}
#endif // __BANK

	bool ClearCacheQueue();
	bool ManageSystemMemoryCache(bool bFailedAllocation = false);

private:
	grcResourceCache();
	virtual ~grcResourceCache();

#if __BANK
	static void WasteSystemMemory();
	static void WasteVideoMemory();
#endif

	HRESULT Get(ResourceData &oResource);
	HRESULT Add(ResourceData &oResource);
	bool Cache(void* pvResource, bool bCache = REUSE_RESOURCE);

	bool IsQueuedForCaching(void* pvResource);
	bool QueueForCaching(void* pvResource);
	
	bool CleanCache(MemoryType eType);
	bool ClearCacheItem(CacheListNode* poNode);
	bool ReleaseCacheItem(CacheListNode* poNode);
	u32 Match(const ResourceData &oSrc1, const ResourceData &oSrc2);

#if defined(_MSC_VER) && (_MSC_VER <= 1600)
	typedef std::map<void*, ResourceData*, std::less<void*>, MapAllocator<std::pair<const void*, ResourceData*>> >    MapActive;
#else
	typedef std::unordered_map<void*, ResourceData*, std::hash<void*>, std::equal_to<void*>, MapAllocator<std::pair<void*, ResourceData*>> >    MapActive;
#endif
	MapActive m_mapActiveResources;
	
#if REUSE_RESOURCE
	typedef std::map< void*, ResourceData*, std::less<void*>, MapAllocator<std::pair<const void*, ResourceData*>> >   MapResource; // Pointer to D3D Resource, 
	typedef std::map< u32, MapResource*, std::less<u32>, MapAllocator<std::pair<const u32, MapResource*>> >	   MapSubClass;		// CRC, Map to all identical CRCs
	typedef std::multimap< u32, MapSubClass*, std::less<u32>, MapAllocator<std::pair<const u32, MapSubClass*>> > MultiMapCache;	// Size, Map to CRC Resource
	MultiMapCache m_ammapCachedResources[RT_LAST];

	typedef atDList<ResourceData*, datBase> CacheList;
	CacheList m_lstFreeCache;
#endif // REUSE_RESOURCE

#if ENABLE_SYSTEMCACHE
	typedef inmultimap<size_t, SysFreeContainer, &SysFreeContainer::m_SizeSort> SystemMemorySizes;
	SystemMemorySizes m_mmapCacheSysSize;
	
	typedef inlist<SysFreeContainer, &SysFreeContainer::m_AgeList> SystemMemoryList;
	SystemMemoryList m_listSysMemCache;
#endif // ENABLE_SYSTEMCACHE

	typedef atPool<ResourceData> ResourceDataPool;
	ResourceDataPool* m_ResDataPool;

	atFixedArray<void*, FREELIST_BUFFER_SIZE> m_lstItemToFree;
#if USE_CREATION_THREAD
	static atFixedArray<grcIndexBuffer*, 4096> m_queueForResourcingIndexBuffers;
	static atFixedArray<grcVertexBuffer*, 4096> m_queueForResourcingVertexBuffers;
	static atFixedArray<grcTexture*, 4096> m_queueForResourcingTextures;
	static void CreationWorker(void*);
#endif // USE_CREATION_THREAD
	static s64 m_aiTotalAvailableMemory[MT_Last];
	static s64 m_aiTotalPhysicalMemory[MT_Last];
	static s64 sm_aiMemoryRestriction[MT_Last];
	static s64 sm_aiReservedMemory[MT_Last];
	static s64 sm_aiReservedApplicationMemory[MT_Last];
	static s64 sm_aiUnloadMemory[MT_Last];
	static s64 sm_aiMaxCacheMemory[MT_Last];

	static s64 sm_aiExtraAvailableMemory[MT_Last];
	static bool sm_bExtraCooldownTimer;
	static u32 sm_uCooldownDelayMS;
	static u32 sm_uExtraUpdateTime;
	static float sm_fMemoryReductionRate;

	static OSVersion sm_eOS;
	static bool sm_bUseLargeMemory;
	static bool sm_bManagerResources;
	bool m_bWarmedCache;
	bool m_bLazyDelete;

	static float sm_fPercentageForCache;

#if !__FINAL
	bool m_bForceFailures;
#endif

	// Statistics	
	u32 m_auTotalActiveItems[MT_Last][RT_LAST+1];
	s64 m_auTypeTotalActiveMemory[MT_Last][RT_LAST+1];
	
	u32 m_auNewAddItems[MT_Last][RT_LAST+1];

#if REUSE_RESOURCE
	u32 m_auTotalCacheItems[MT_Last][RT_LAST+1];
	s64 m_auTypeTotalCachedMemory[MT_Last][RT_LAST+1];

	u32 m_auNewCacheItems[MT_Last][RT_LAST+1];	
	u32 m_auNewClearCacheItems[MT_Last][RT_LAST+1];
	u32 m_auNewUsedCacheItems[MT_Last][RT_LAST+1];
#endif // REUSE_RESOURCE

	MUTEX_TYPE m_aMutex[MT_Last];
	sysIpcCurrentThreadId m_aThreadID[MT_Last];
	u32 m_auLockCount[MT_Last];
	const char* m_apszFile[MT_Last][LOCK_STACK_DEPTH];
	u32 m_auLineNumber[MT_Last][LOCK_STACK_DEPTH];

	u32 m_uDeleteLockCount;
	sysIpcCurrentThreadId m_aDeleteThreadID;
	const char* m_apszDeleteFile[LOCK_STACK_DEPTH];
	u32 m_auDeleteLineNumber[LOCK_STACK_DEPTH];
	mutable MUTEX_TYPE m_MutexCachingQueue;

	sysIpcMutex m_DeviceCreate;

#if USE_CREATION_THREAD
	static MUTEX_TYPE m_MutexResourcingQueue;
	static sysIpcThreadId s_CreationWorkerThread;
#endif // USE_CREATION_THREAD

#if CONTROL_RATE_OF_CREATIONS
	static float m_fDataCreationRate;
	static float m_fCreateRate;
	static float m_fFalloffFromPreviousFrame;
	static float m_fMaxWaitTime;

	static float m_fCurrentDataCreateRate;
	static float m_fCurrentCreateRate;
#endif

	static GameRunningCallback  sm_fnGameRunning;
	static sysIpcCurrentThreadId sm_iPlacementThreadId;
};

template <class T> inline unsigned long grcResourceCache::Release(T poResource)
{
	if (poResource == NULL)
	{
		grcWarningf("Attempting to release NULL resource");
		return 0;
	}

#if USE_DX9_SINGLE_THREADED || __D3D11
	//if (GRCDEVICE.GetDxVersion() < 1000) // Avoids Video memory lock if we do this all the time
	{
		AssertVerify(QueueForCaching(poResource));
		return 0;
	}
#endif // USE_DX9_SINGLE_THREADED
}

template <class T> unsigned long grcResourceCache::InstantRelease(T poResource)
{
	if (poResource == NULL)
	{
		grcWarningf("Attempting to release NULL resource");
		return 0;
	}
	LOCK_DELETION_QUEUE
	AssertVerify(Cache(poResource, false));
	UNLOCK_DELETION_QUEUE
	return 0;
}


#if !__FINAL
template <class T> inline bool grcResourceCache::Validate(const T poResource) const
{
	if (!m_bLazyDelete)
	{
		// Check against deletion list
		LOCK_DELETION_QUEUE
		for (s32 iIndex = 0; iIndex < m_lstItemToFree.GetCount(); iIndex++)
		{
			const T pvDeletionResource = (const T)m_lstItemToFree[iIndex];
			if (pvDeletionResource == poResource)
			{
				Errorf("Resource is pending for deletion");
				UNLOCK_DELETION_QUEUE
				return false;
			}
		}
		UNLOCK_DELETION_QUEUE
	}

	// Check that the resource actually exists
	VIDEOMEMORY_LOCK_RESOURCES;

	if (poResource == NULL)
	{
		grcWarningf("Invalid Resource - NULL");
		VIDEOMEMORY_UNLOCK_RESOURCES;
		return false;
	}

	MapActive::const_iterator itActive = m_mapActiveResources.find(poResource);
	Assertf(itActive != m_mapActiveResources.end(), "Unable to find resource %p - Invalid resource pointer", poResource);
	if (itActive == m_mapActiveResources.end())
	{
		grcErrorf("Invalid Resource %x - Ignoring", poResource);
		VIDEOMEMORY_UNLOCK_RESOURCES;
		return false;
	}
	VIDEOMEMORY_UNLOCK_RESOURCES;

	return true;
}
#endif // __FINAL

} // End of Rage Namespace

#endif // __WIN32PC && USE_RESOURCE_CACHE

#endif // GRCORE_RESOURCECACHE_H
