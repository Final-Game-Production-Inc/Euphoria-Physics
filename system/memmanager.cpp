
#include "memmanager.h"

#if COMMERCE_CONTAINER
#include <sys/memory.h>
#include <cell/sysmodule.h>
#include "criticalsection.h"
#include "system/virtualmemory_psn.h"
extern sys_memory_container_t g_commerce_container;
extern sys_memory_container_t g_prx_container;
#endif

namespace rage {

#if RESOURCE_HEADER
#if __SPU
#define MEMMANAGER_CRITICAL_SECTION
#else
#define MEMMANAGER_CRITICAL_SECTION sysCriticalSection cs(m_headerToken)
#endif

// Static
#if __FINAL && __PPU
void sysMemManager::CrashTheGameIfQAHasMisconfiguredATestKitWhenRunningTheGameInPS3Final(const size_t RAGE_MEMORY_DEBUG_ONLY(bytes))
{
	Memoryf("Misconfigured Final Kit! Only %d/%d MB available in Game Virtual! Please ask your QA Lead to show you how to properly run PS3 Final!", bytes >> 20, 256);
	Quitf("Misconfigured Final Kit! Only %d/%d MB available in Game Virtual! Please ask your QA Lead to show you how to properly run PS3 Final!", bytes >> 20, 256);
}
#endif

void sysMemManager::InitHeaders(size_t size) 
{ 
	m_headerMap.clear(); 
	m_headerPool.Init(size); 
}

sysMemManager::HeaderInfo* sysMemManager::AddNodeInfo(const void* pKey)
{
	MEMMANAGER_CRITICAL_SECTION;
	
	void* ptr = m_headerPool.New();
	HeaderInfo* pValue = rage_placement_new(ptr) HeaderInfo;
	m_headerMap.insert(const_cast<void*>(pKey), pValue);
	
	return pValue;
}

void sysMemManager::DeleteNodeInfo(const void* pKey) 
{ 
	MEMMANAGER_CRITICAL_SECTION;

	HeaderMap::iterator it = m_headerMap.find(const_cast<void*>(pKey));
	HeaderInfo* pValue = (it != m_headerMap.end()) ? it->second : NULL;

	m_headerMap.erase(const_cast<void*>(pKey));
	if (pValue)
		m_headerPool.Delete(pValue);
}	

// User Data
void sysMemManager::SetUserData(const void* pKey, u32 data) 
{ 
	MEMMANAGER_CRITICAL_SECTION;
		
	HeaderMap::iterator it = m_headerMap.find(const_cast<void*>(pKey));
	HeaderInfo* pValue = (it == m_headerMap.end()) ? AddNodeInfo(pKey) : it->second;
	pValue->Data = data;
}

u32 sysMemManager::GetUserData(const void* pKey)
{ 
	MEMMANAGER_CRITICAL_SECTION;

	HeaderMap::iterator it = m_headerMap.find(const_cast<void*>(pKey));
	return (it != m_headerMap.end()) ? it->second->Data : sysMemAllocator::INVALID_USER_DATA;
}

void sysMemManager::SetLockCount(const void* pKey, u32 lock) 
{
	FastAssert(lock < (1 << HeaderInfo::BITS_LOCKCOUNT)); 

	MEMMANAGER_CRITICAL_SECTION;

	HeaderMap::iterator it = m_headerMap.find(const_cast<void*>(pKey));
	HeaderInfo* pValue = (it == m_headerMap.end()) ? AddNodeInfo(pKey) : it->second;
	pValue->LockCount = lock;
}

u32 sysMemManager::GetLockCount(const void* pKey)
{ 
	MEMMANAGER_CRITICAL_SECTION;

	HeaderMap::iterator it = m_headerMap.find(const_cast<void*>(pKey));
	return (it != m_headerMap.end()) ? it->second->LockCount : 0;
}

bool sysMemManager::TryLockBlock(const void* pKey, u32 lockCount)
{
	MEMMANAGER_CRITICAL_SECTION;

	unsigned count = GetLockCount(pKey);

	// Currently in use by an inflight defrag?
	if (Unlikely(count == HeaderInfo::LOCK_COUNT_DEFRAG))
		return false;

#if !__NO_OUTPUT
	// This warning should be ultra-rare, if not, then we'll need to steal more
	// bits for the counter from somewhere.
	if (Unlikely(count + lockCount >= HeaderInfo::LOCK_COUNT_SATURATE))
		Warningf("sysBuddyHeap::LockBlock saturating lock count, will not be able to defrag this allocation any more");
#endif

	count = (count+lockCount<HeaderInfo::LOCK_COUNT_SATURATE) ? count+lockCount : HeaderInfo::LOCK_COUNT_SATURATE;
	SetLockCount(pKey, count);
	return true;
}

void sysMemManager::UnlockBlock(const void* pKey, u32 unlockCount)
{
	MEMMANAGER_CRITICAL_SECTION;

	unsigned count = GetLockCount(pKey);
	FatalAssert(count >= unlockCount);

	// Unlock after defrag ?
	if (count == HeaderInfo::LOCK_COUNT_DEFRAG)
		count = 0;
	// Unlock after TryLockBlock ?
	// Only decrement count if not saturated
	else if (count<HeaderInfo::LOCK_COUNT_SATURATE)
		count -= unlockCount;

	SetLockCount(pKey, count);
}
#endif

// Flex Allocator
#if COMMERCE_CONTAINER
static __THREAD sysMemAllocator* s_FlexPrevAllocator;

void sysMemStartFlex()
{
	s_FlexPrevAllocator = &sysMemAllocator::GetCurrent();
	sysMemAllocator::SetCurrent(*sysMemManager::GetInstance().GetFlexAllocator());
}

void sysMemEndFlex()
{
	sysMemAllocator::SetCurrent(*s_FlexPrevAllocator);
	s_FlexPrevAllocator = NULL;
}
#endif

// Frag Cache
static __THREAD sysMemAllocator* s_FragCacheAllocator;

void sysMemStartFragCache()
{
	s_FragCacheAllocator = &sysMemAllocator::GetCurrent();
	sysMemAllocator::SetCurrent(*sysMemManager::GetInstance().GetFragCacheAllocator());
}

void sysMemEndFragCache()
{
	sysMemAllocator::SetCurrent(*s_FragCacheAllocator);
	s_FragCacheAllocator = NULL;
}

// Scratch
#if SCRATCH_ALLOCATOR
static __THREAD sysMemAllocator* s_ScratchAllocator;

void sysMemStartScratch()
{
	s_ScratchAllocator = &sysMemAllocator::GetCurrent();
	sysMemAllocator::SetCurrent(*sysMemManager::GetInstance().GetScratchAllocator());
}

void sysMemEndScratch()
{
	sysMemAllocator::SetCurrent(*s_ScratchAllocator);
	s_ScratchAllocator = NULL;
}
#endif

sysMemManager::sysMemManager()
#if API_HOOKER
 : m_virtualAllocTotal(0), m_heapAllocTotal(0)
#endif 
{
	m_pFragMemory = NULL;
	m_pGlobalScriptMemory = NULL;
	m_pFragCacheAllocator = NULL;

#if RSG_ORBIS
	m_pChunkyAllocator = NULL;
#endif // RSG_ORBIS

#if COMMERCE_CONTAINER
	m_pFlexAllocator = NULL;
	m_pVirtualMemHeap = NULL;
	m_pMovePedMemory = NULL;
	m_pNetworkMemory = NULL;	
	m_bFlexHeapEnabled = true;
#elif MEMORY_TRACKER
	m_pSystemTracker = NULL;
#endif
}

#if COMMERCE_CONTAINER
// NOTE: Store is the full 17MB of memory (PRX + commerce module)
bool sysMemManager::PageOutContainerMemory(bool store /*= true*/)
{	
	int result = CELL_OK;
	(void)result;

	// VRAM MEMORY
	//
	// The maximum amount of memory that can be returned is the difference of the total amount of physical memory used by 
	// the virtual memory area minus 1MB. When an amount exceeding this value is specified, EBUSY (0x8001000A) will return.
	const size_t heapSize = store ? (COMMERCE_HEAP_SIZE - (1 << 20)) : COMMERCE_PRX_SIZE;

	// EJ: Save the container memory to the hard drive
	// sys_vm_return_memory() returns the physical memory used for the virtual memory area to the memory container specified 
	// upon first creating the virtual memory area. Specify the virtual memory area by its start address in addr. Specify the 
	// amount of memory to return in size. Memory can be returned in 1MB units.
	VirtualMemHeap* pVirtualMemHeap = GetVirtualMemHeap();
	Assert(pVirtualMemHeap);

	sys_addr_t addr = pVirtualMemHeap->GetHeapAddr();
	if ((result = sys_vm_return_memory(addr, heapSize)) != CELL_OK)
	{
		Quitf("Unable to save container memory! Code %x\n", result);
		return false;
	}

	if (store)
	{
		// MARKETPLACE CONTAINER
		//
		// sys_memory_container_create() creates a memory container. The size of the memory container is specified by memsize. 
		if ((result = sys_memory_container_create(&g_commerce_container, heapSize - COMMERCE_PRX_SIZE)) != CELL_OK)
		{
			Quitf("Unable to create marketplace container! Code %x\n", result);
			return false;
		}
	}	

	// PRX CONTAINER
	//
	// sys_memory_container_create() creates a memory container. The size of the memory container is specified by memsize. 
	if ((result = sys_memory_container_create(&g_prx_container, COMMERCE_PRX_SIZE)) != CELL_OK)
	{
		Quitf("Unable to create PRX container! Code %x\n", result);
		return false;
	}

	// cellSysmoduleSetMemcontainer() sets a memory container so that subsequent module calls are made within the memory container specified.
	if ((result = cellSysmoduleSetMemcontainer(g_prx_container)) != CELL_OK)
	{
		Quitf("[Unable to override PRX memory container! Code %x\n", result);
		return false;
	}

	return true;
}

// NOTE: Store is the full 17MB of memory (PRX + commerce module)
bool sysMemManager::PageInContainerMemory(bool store /*= true*/)
{
	OUTPUT_ONLY(int result = CELL_OK);

	// PRX CONTAINER
	//
	// cellSysmoduleSetMemcontainer() sets a memory container so that subsequent module calls are made within the memory container specified.
	if ((OUTPUT_ONLY(result =) cellSysmoduleSetMemcontainer(SYS_MEMORY_CONTAINER_ID_INVALID)) != CELL_OK)
	{
		Quitf("Unable to override PRX memory container! Code %x\n", result);
		return false;
	}

	// EJ: Destroy the memory containers required by PSN Marketplace
	// sys_memory_container_destroy() destroys a memory container. All memory taken from the memory container to be destroyed 
	// must be released. Memory allocated to the destroyed memory container is returned to the default memory container (the 
	// memory container which represents the whole user memory that the game process can use) of the process.
	if ((OUTPUT_ONLY(result =) sys_memory_container_destroy(g_prx_container)) != CELL_OK)
	{
		Quitf("Unable to destroy PRX container! Code %x\n", result);
		return false;
	}

	if (store)
	{
		// MARKETPLACE CONTAINER
		//
		// EJ: Destroy the memory containers required by PSN Marketplace
		// sys_memory_container_destroy() destroys a memory container. All memory taken from the memory container to be destroyed 
		// must be released. Memory allocated to the destroyed memory container is returned to the default memory container (the 
		// memory container which represents the whole user memory that the game process can use) of the process.
		if ((OUTPUT_ONLY(result =) sys_memory_container_destroy(g_commerce_container)) != CELL_OK)
		{
			Quitf("Unable to destroy commerce container! Code %x\n", result);
			return false;
		}
	}

	// VRAM MEMORY
	//
	// The maximum amount of memory that can be returned is the difference of the total amount of physical memory used by 
	// the virtual memory area minus 1MB. When an amount exceeding this value is specified, EBUSY (0x8001000A) will return.
	const size_t heapSize = store ? (COMMERCE_HEAP_SIZE - (1 << 20)) : COMMERCE_PRX_SIZE;

	// EJ: Load the container memory from the hard drive
	// sys_vm_append_memory() adds physical memory used for the virtual memory area. Specify the virtual memory area by its 
	// start address in addr. Specify the amount of memory to add in size. Memory can be added in 1MB units. When making a 
	// specification where the total amount of physical memory becomes 257MB or more, ENOMEM will return. The physical memory 
	// to be added will be taken from the memory container specified upon first creating the virtual memory area.
	VirtualMemHeap* pVirtualMemHeap = GetVirtualMemHeap();
	Assert(pVirtualMemHeap);

	sys_addr_t addr = pVirtualMemHeap->GetHeapAddr();
	if ((OUTPUT_ONLY(result =) sys_vm_append_memory(addr, heapSize)) != CELL_OK)
	{
		Quitf("Unable to load the container memory! Code %x\n", result);
		return false;
	}

	// EJ: Bring the physical memory into scope
	// sys_vm_touch() allocates physical memory to a partial area. Specify the start address and size of the partial area to addr and size.
	if ((OUTPUT_ONLY(result =) sys_vm_touch(addr, heapSize)) != CELL_OK)
	{
		Quitf("Unable to touch commerce memory! Code %x\n", result);
		return false;
	}

	return true;
}
#endif // COMMERCE_CONTAINER

} // namespace rage
