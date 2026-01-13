
#ifndef SYSTEM_MEMMANAGER_H
#define SYSTEM_MEMMANAGER_H

#include "atl/inmap.h"
#include "atl/pool.h"
#include "system/container.h"
#include "system/criticalsection.h"
#include "system/memory.h"
#include "system/simpleallocator.h"
#include "system/splitallocator.h"

#if MEMORY_TRACKER
#include "system/systemallocator_system.h"
#endif

#define SCRATCH_ALLOCATOR ((__PS3 || __XENON) && !__TOOL && !__RESOURCECOMPILER)

#define ENABLE_CHUNKY_ALLOCATOR			RSG_ORBIS || RSG_DURANGO
#define ENABLE_POOL_ALLOCATOR			RSG_ORBIS || RSG_DURANGO

namespace rage {

#define MEMMANAGER sysMemManager::GetInstance()

#if __PPU
	class sysMemAllocator;
	extern sysMemAllocator* g_pResidualAllocator;
	extern u32 g_bDontUseResidualAllocator;
#endif

#if ENABLE_CHUNKY_ALLOCATOR
class ChunkyWrapper
{
public:
	ChunkyWrapper(sysMemAllocator& alloc, bool allocating, char const* pOwner, char const* pDetail = "")
		: m_a(alloc)
		, m_allocating(allocating)
		, m_pOwner(pOwner)
		, m_pDetail(pDetail)
	{
		m_availableBefore = sysMemAllocator::GetCurrent().GetMemoryAvailable();
		Displayf("[CHUNKY] %s %s from chunky... %u Before (%s)", m_pOwner, m_allocating ? "Allocating" : "Deallocating", (u32)sysMemAllocator::GetCurrent().GetMemoryAvailable(), m_pDetail);
	}

	~ChunkyWrapper()
	{
		Displayf("[CHUNKY] %s %s from chunky... %u After (diff %d) (%s)", m_pOwner, m_allocating ? "Allocating" : "Deallocating", (u32)sysMemAllocator::GetCurrent().GetMemoryAvailable(), (int)sysMemAllocator::GetCurrent().GetMemoryAvailable() - (int)m_availableBefore, m_pDetail);
	}

	sysMemAutoUseAllocator m_a;
	size_t m_availableBefore;
	bool m_allocating;
	char const* m_pOwner;
	char const* m_pDetail;
};
#endif // ENABLE_CHUNKY_ALLOCATOR

// EJ: Manages all game-level heaps and allocators
class sysMemManager
{
private:
	void* m_pFragMemory;
	void* m_pGlobalScriptMemory;
	sysMemAllocator* m_pFragCacheAllocator;	

#if RESOURCE_HEADER
	struct HeaderInfo
	{
		enum  
		{ 
			BITS_LOCKCOUNT  = 7,
			BITS_DATA = 24, 
			BITS_PADDING = 1,
			LOCK_COUNT_SATURATE = ((1 << BITS_LOCKCOUNT) - 2),
			LOCK_COUNT_DEFRAG = ((1 << BITS_LOCKCOUNT) - 1)
		};
		
		HeaderInfo() : LockCount(1), Data(~0U), Pad(0) { }

		inmap_node<void*, HeaderInfo> m_link;
		u32 LockCount : BITS_LOCKCOUNT;
		u32 Data : BITS_DATA;
		u32 Pad : BITS_PADDING;
	};

	typedef atPool<HeaderInfo> HeaderPool;
	typedef inmap<void*, HeaderInfo, &HeaderInfo::m_link> HeaderMap;

	sysCriticalSectionToken m_headerToken;
	HeaderPool m_headerPool;
	HeaderMap m_headerMap;
#endif

#if SCRATCH_ALLOCATOR
	sysMemStack* m_pScratchAllocator;
#endif

#if COMMERCE_CONTAINER
	sysMemSplitAllocator* m_pFlexAllocator;
	VirtualMemHeap* m_pVirtualMemHeap;
	void* m_pMovePedMemory;
	void* m_pNetworkMemory;	
	bool m_bFlexHeapEnabled;
#elif MEMORY_TRACKER
	sysMemSystemTracker* m_pSystemTracker;
#endif

#if RSG_DURANGO || RSG_ORBIS || RSG_PC
	sysMemSimpleAllocator* m_pReplayAllocator;
#endif

#if ENABLE_CHUNKY_ALLOCATOR
	sysMemSimpleAllocator* m_pChunkyAllocator;
#endif // ENABLE_CHUNKY_ALLOCATOR

#if ENABLE_POOL_ALLOCATOR
	sysMemSimpleAllocator* m_pPoolAllocator;
#endif // ENABLE_POOL_ALLOCATOR

#if RSG_ORBIS
	sysMemSimpleAllocator* m_pFlexAllocator;
#endif

#if !__FINAL && (RSG_ORBIS || RSG_DURANGO || RSG_PC)
	sysMemSimpleAllocator* m_pHemlisAllocator;
	sysMemSimpleAllocator* m_pRecorderAllocator;
#endif

private:
	sysMemManager();

public:
	inline static sysMemManager& GetInstance()
	{
		static sysMemManager s_instance;
		return s_instance;
	}

#if __FINAL && __PPU
	static void CrashTheGameIfQAHasMisconfiguredATestKitWhenRunningTheGameInPS3Final(const size_t bytes);
#endif

#if API_HOOKER
	size_t m_virtualAllocTotal;
	size_t m_heapAllocTotal;
#endif

#if RESOURCE_HEADER	
	void InitHeaders(size_t size);

	size_t GetTotalHeaders() const { return m_headerPool.GetSize(); }
	size_t GetUsedHeaders() const { return m_headerPool.GetNoOfUsedSpaces(); }
	size_t GetFreeHeaders() const { return m_headerPool.GetNoOfFreeSpaces(); }

	HeaderInfo* AddNodeInfo(const void* pKey);
	void DeleteNodeInfo(const void* pKey) ;

	// User Data
	void SetUserData(const void* pKey, u32 data);
	u32 GetUserData(const void* pKey);

	void SetLockCount(const void* pKey, u32 lock);
	u32 GetLockCount(const void* pKey);

	bool TryLockBlock(const void* pKey, u32 lockCount);
	void UnlockBlock(const void* pKey, u32 unlockCount);
#endif

	// Frag
	inline void* GetFragMemory() {return m_pFragMemory;}
	inline void SetFragMemory(void* const pMemory) {m_pFragMemory = pMemory;}

	inline sysMemAllocator* GetFragCacheAllocator() {return m_pFragCacheAllocator;}
	inline void SetFragCacheAllocator(sysMemAllocator* pAllocator) {m_pFragCacheAllocator = pAllocator;}
	inline void SetFragCacheAllocator(sysMemAllocator& allocator) {m_pFragCacheAllocator = &allocator;}

#if SCRATCH_ALLOCATOR
	// Scratch
	inline sysMemStack* GetScratchAllocator() {return m_pScratchAllocator;}
	inline void SetScratchAllocator(sysMemStack* pAllocator) {m_pScratchAllocator = pAllocator;}
	inline void SetScratchAllocator(sysMemStack& allocator) {m_pScratchAllocator = &allocator;}
#endif

	// Global Script
	inline void* GetGlobalScriptMemory() {return m_pGlobalScriptMemory;}
	inline void SetGlobalScriptMemory(void* const pMemory) {m_pGlobalScriptMemory = pMemory;}

#if !__FINAL && (RSG_ORBIS || RSG_DURANGO || RSG_PC)
#if (RSG_ORBIS || RSG_DURANGO)	
	// Hemlis
	inline sysMemSimpleAllocator* GetHemlisAllocator() {return m_pHemlisAllocator;}
	inline void SetHemlisAllocator(sysMemSimpleAllocator* pAllocator) {m_pHemlisAllocator = pAllocator;}
	inline void SetHemlisAllocator(sysMemSimpleAllocator& allocator) {m_pHemlisAllocator = &allocator;}
#endif
	// Recorder
	inline sysMemSimpleAllocator* GetRecorderAllocator() {return m_pRecorderAllocator;}
	inline void SetRecorderAllocator(sysMemSimpleAllocator* pAllocator) {m_pRecorderAllocator = pAllocator;}
	inline void SetRecorderAllocator(sysMemSimpleAllocator& allocator) {m_pRecorderAllocator = &allocator;}
#endif

#if COMMERCE_CONTAINER
	// Flex
	inline sysMemSplitAllocator* GetFlexAllocator() {return m_pFlexAllocator;}
	inline void SetFlexAllocator(sysMemSplitAllocator* pAllocator) {m_pFlexAllocator = pAllocator;}
	inline void SetFlexAllocator(sysMemSplitAllocator& allocator) {m_pFlexAllocator = &allocator;}

	inline void EnableFlexHeap() {m_bFlexHeapEnabled = true;}
	inline void DisableFlexHeap() {m_bFlexHeapEnabled = false;}
	inline bool IsFlexHeapEnabled() {return m_bFlexHeapEnabled;}
	inline bool IsFlexHeapDisabled() {return !m_bFlexHeapEnabled;}

	// MovePed
	inline void* GetMovePedMemory() {return m_pMovePedMemory;}	
	inline void SetMovePedMemory(void* const pMemory) {m_pMovePedMemory = pMemory;}

	// Network
	inline void* GetNetworkMemory() {return m_pNetworkMemory;}
	inline void SetNetworkMemory(void* const pMemory) {m_pNetworkMemory = pMemory;}

	// Virtual Memory
	inline VirtualMemHeap* GetVirtualMemHeap() {return m_pVirtualMemHeap;}
	inline void SetVirtualMemHeap(VirtualMemHeap* pHeap) {m_pVirtualMemHeap = pHeap;}
	inline void SetVirtualMemHeap(VirtualMemHeap& heap) {m_pVirtualMemHeap = &heap;}

	// Commerce
	bool PageOutContainerMemory(bool store = true);
	bool PageInContainerMemory(bool store = true);

#elif MEMORY_TRACKER
	// System Memory
	inline sysMemSystemTracker* GetSystemTracker() {return m_pSystemTracker;}
	inline void SetSystemTracker(sysMemSystemTracker* pAllocator) {m_pSystemTracker = pAllocator;}
	inline void SetSystemTracker(sysMemSystemTracker& allocator) {m_pSystemTracker = &allocator;}
#endif // COMMERCE_CONTAINER

#if RSG_DURANGO || RSG_ORBIS || RSG_PC
	inline sysMemSimpleAllocator* GetReplayAllocator() { return m_pReplayAllocator; }
	inline void SetReplayAllocator(sysMemSimpleAllocator* pAllocator) { m_pReplayAllocator = pAllocator; }
#endif

#if ENABLE_CHUNKY_ALLOCATOR
	inline sysMemSimpleAllocator* GetChunkyAllocator() { return m_pChunkyAllocator; }
	inline void SetChunkyAllocator(sysMemSimpleAllocator* pAllocator) { m_pChunkyAllocator = pAllocator; }
	inline void SetChunkyAllocator(sysMemSimpleAllocator& allocator) { m_pChunkyAllocator = &allocator; }
#endif // ENABLE_CHUNKY_ALLOCATOR

#if ENABLE_POOL_ALLOCATOR
	inline sysMemSimpleAllocator* GetPoolAllocator() { return m_pPoolAllocator; }
	inline void SetPoolAllocator(sysMemSimpleAllocator* pAllocator) { m_pPoolAllocator = pAllocator; }
	inline void SetPoolAllocator(sysMemSimpleAllocator& allocator) { m_pPoolAllocator = &allocator; }
#endif // ENABLE_POOL_ALLOCATOR

#if RSG_ORBIS
	// Flex
	inline sysMemSimpleAllocator* GetFlexAllocator() {return m_pFlexAllocator;}
	inline void SetFlexAllocator(sysMemSimpleAllocator* pAllocator) {m_pFlexAllocator = pAllocator;}
	inline void SetFlexAllocator(sysMemSimpleAllocator& allocator) {m_pFlexAllocator = &allocator;}
#endif

#if !__FINAL
	size_t GetExeSize() { return m_ExeSize; }
	void SetExeSize(size_t exeSize) { m_ExeSize = exeSize; }

private:
	size_t	m_ExeSize;
#endif
};

#if COMMERCE_CONTAINER
extern void sysMemStartFlex();
extern void sysMemEndFlex();
#else
#define sysMemStartFlex()
#define sysMemEndFlex()
#endif

struct sysMemAutoUseFlexMemory
{
	sysMemAutoUseFlexMemory() {sysMemStartFlex();}
	~sysMemAutoUseFlexMemory() {sysMemEndFlex();}
};

// Frag Cache
extern void sysMemStartFragCache();
extern void sysMemEndFragCache();
struct sysMemAutoUseFragCacheMemory
{
	sysMemAutoUseFragCacheMemory() {sysMemStartFragCache();}
	~sysMemAutoUseFragCacheMemory() {sysMemEndFragCache();}
};

#if SCRATCH_ALLOCATOR
// Scratch
extern void sysMemStartScratch();
extern void sysMemEndScratch();
struct sysMemAutoUseScratchMemory
{
	sysMemAutoUseScratchMemory() {sysMemStartScratch();}
	~sysMemAutoUseScratchMemory() {sysMemEndScratch();}
};
#endif
} // namespace rage

#endif // SYSTEM_MEMMANAGER_H
