//
// system/memory.cpp
//
// Copyright (C) 1999-2014 Rockstar Games.  All Rights Reserved.
//

#include "memory.h"
#if !__SPU
#include "diag/output.h"
#endif
#include "diag/tracker.h"
#include "paging/rscbuilder.h"
#include "system/criticalsection.h"
#include "system/ipc.h"
#include "system/nelem.h"
#include "system/xtl.h"
#include "system/memvisualize.h"
#include "system/systemallocator_system.h" // potentially system related
#include "system/memmanager.h"

#if COMMERCE_CONTAINER
#include <sys/memory.h>
#include <cell/sysmodule.h>
#include "system/virtualmemory_psn.h"

sys_memory_container_t g_commerce_container;
sys_memory_container_t g_prx_container;
#endif // COMMERCE_CONTAINER

#if __PPU
#if !__TOOL
#include <cell/gcm.h>
#include <sys/vm.h>
#endif // !__TOOL
#elif __PSP2
#include <kernel.h>
#elif __XENON
#include <xbdm.h>
#elif __WIN32PC
#include <Psapi.h>
#include <windows.h>
#pragma comment(lib,"Psapi.lib")
#elif RSG_ORBIS
#include <kernel.h>
#include <sys/mman.h>
#elif RSG_DURANGO
#include "system/d3d11.h"
#endif // __WIN32PC

#include <stdlib.h>

#if API_HOOKER
#include "system/minhook.h"
#include "system/threadtype.h"
#endif

// Has to be outside of namespace rage because of SPU symbol binding restrictions
#if SUPPORT_DEBUG_MEMORY_FILL
unsigned char g_EnableDebugMemoryFill = __DEV | __TOOL? 0xFF : 0x00;
#endif

#if RAGE_ENABLE_RAGE_NEW
extern __THREAD int RAGE_LOG_DISABLE;
#endif

#if API_HOOKER
typedef PVOID(WINAPI* VirtualAllocHooker)(void*, size_t, unsigned long, unsigned long);
VirtualAllocHooker g_virtualAllocHooker = NULL;

typedef BOOL(WINAPI* VirtualFreeHooker)(void*, size_t, unsigned long);
VirtualFreeHooker g_virtualFreeHooker = NULL;

typedef PVOID(WINAPI* HeapAllocHooker)(void*, unsigned long, size_t);
HeapAllocHooker g_heapAllocHooker = NULL;

typedef PVOID(WINAPI* HeapReAllocHooker)(void*, unsigned long, void*, size_t);
HeapReAllocHooker g_heapReAllocHooker;

typedef BOOL(WINAPI* HeapFreeHooker)(void*, unsigned long, void*);
HeapFreeHooker g_heapFreeHooker = NULL;

typedef PVOID(WINAPI* LocalAllocHooker)(unsigned int, size_t);
LocalAllocHooker g_localAllocHooker;

typedef HLOCAL(WINAPI* LocalReAllocHooker)(void*, size_t, unsigned int);
LocalReAllocHooker g_localReAllocHooker;

typedef HLOCAL(WINAPI* LocalFreeHooker)(void*);
LocalFreeHooker g_localFreeHooker;

typedef HLOCAL(WINAPI* GlobalFreeHooker)(void*);
GlobalFreeHooker g_globalFreeHooker;
#endif

namespace rage {

__THREAD int sysMemCurrentMemoryBucket = 0;
__THREAD int sysMemAllowResourceAlloc = 0;

#if MEM_VALIDATE_USERDATA
__THREAD u32 sysMemCurrentUserData;
#endif // MEM_VALIDATE_USERDATA


#if __ASSERT
__THREAD int sysMemStreamingCount = 0;
#endif

// PURPOSE
//   The memory word used to fill newly allocated memory
// NOTES
//   Try 0x7F FF FF FF for a NaN
unsigned sysMemoryFillWord = 0xCDCDCDCD;

// PURPOSE
//   When true, causes newly allocated memory to be filled with the pattern in
//   sysMemoryFillWord, repeated every four bytes.
bool sysEnableMemoryFill = __DEV;

// PURPOSE
//   When true, causes newly allocated memory to be filled with its own address.
// NOTES
//   Can be useful when uninitialized memory is being used somewhere, but the
//   memory fill masks the resultant random behavior in __DEV builds.
bool sysEnableRandomFill = false;

#if ENABLE_DEBUG_HEAP
bool g_sysHasDebugHeap = false;
int g_sysDirectDebugEnabled = 0;
int g_sysDirectVirtualEnabled = 0;
int g_sysDirectFlexEnabled = 0;
#endif

#if !__TOOL
__THREAD sysMemAllocator* sysMemAllocator_sm_Current;
__THREAD sysMemAllocator* sysMemAllocator_sm_Master;
__THREAD sysMemAllocator* sysMemAllocator_sm_Container;
__THREAD int sysMemAllocator_sm_LockHeap;
sysMemAllocator* sysMemAllocator_sm_Emergency;
#endif

#if !__NO_OUTPUT
	bool (*sysMemAllocator::sm_DebugPrintAllocInfo)(const void*) = NULL;
#endif


#if LIST_PHYSICAL_ALLOCS_CALLBACK
	static SysMemPhysicalAlloc s_PhysicalAllocs[64];
#	define MAX_NUM_PHYSICAL_ALLOCS (NELEM(s_PhysicalAllocs)-1)      // -1 because we need a sentinal
	static unsigned s_NumPhysicalAllocs = 0;
	static sysCriticalSectionToken s_PhysicalAllocsCritSec;
	const SysMemPhysicalAlloc *sysMemListPhysicalAllocs()
	{
		return s_PhysicalAllocs;
	}
#endif // LIST_PHYSICAL_ALLOCS_CALLBACK


// Helpful function pointers
void (*sysMemOutOfMemoryDisplay)() = NULL;
ASSERT_ONLY(VerifyMainThread sysMemVerifyMainThread = NULL;)

sysMemAllocator::~sysMemAllocator()
{
#	if LIST_PHYSICAL_ALLOCS_CALLBACK
		// Nonsense just to ensure that sysMemListPhysicalAllocs is never dead stripped
		if ((const SysMemPhysicalAlloc*(*)())(s_PhysicalAllocs[0].base) == sysMemListPhysicalAllocs)
		{
			s_PhysicalAllocs[1].base = (void*)sysMemListPhysicalAllocs;
		}
#	endif
}

void sysMemAllocator::DeferredFree(const void *ptr)
{
	Free(ptr);
}

bool
sysMemAllocator::SetQuitOnFail(const bool /*quitOnFail*/)
{
    AssertMsg(0, "SetQuitOnFail() is not implemented in this class");

    return false;
}

void*
sysMemAllocator::TryAllocate(size_t /*size*/,size_t /*align*/,int /*heapIndex*/)
{
    AssertMsg(0, "TryAllocate() is not implemented in this class");

    return NULL;
}

size_t sysMemAllocator::GetSize(const void * /*ptr*/) const {
	return 0;
}


size_t sysMemAllocator::GetFragmentation()
{
	if (GetLargestAvailableBlock() > 0)
	{
		return 100 - u32(((u64)GetLargestAvailableBlock() * 100) / GetMemoryAvailable());
	}	

	return 0;
}

unsigned sysOpNewAlign = 0;


#define ALLOC_MAX_STACK_SIZE 8u
#if ENABLE_POOL_ALLOCATOR
static __THREAD sysMemAllocator* s_PoolAllocatorStack[ALLOC_MAX_STACK_SIZE];
static __THREAD u32 s_PoolAllocatorTop = 0;
#endif // ENABLE_POOL_ALLOCATOR

void sysMemStartPool()
{
#if ENABLE_POOL_ALLOCATOR
	sysMemAllocator* current = &sysMemAllocator::GetCurrent();
	if (current == sysMemManager::GetInstance().GetPoolAllocator())
		__debugbreak();

	sysMemAllocator::SetCurrent(*sysMemManager::GetInstance().GetPoolAllocator());

	TrapGE(s_PoolAllocatorTop, ALLOC_MAX_STACK_SIZE);
	s_PoolAllocatorStack[s_PoolAllocatorTop++] = current;
#endif // ENABLE_POOL_ALLOCATOR
}

void sysMemEndPool()
{
#if ENABLE_POOL_ALLOCATOR
	TrapZ(s_PoolAllocatorTop);
	sysMemAllocator* old = s_PoolAllocatorStack[--s_PoolAllocatorTop];
	sysMemAllocator::SetCurrent(*old);
#endif // ENABLE_POOL_ALLOCATOR
}


static __THREAD sysMemAllocator* s_TempAllocator;
static __THREAD sysMemAllocator* s_TempContainer;
static __THREAD int s_TempNestingCount;

void sysMemStartTemp() {
#if RAGE_ENABLE_RAGE_NEW	
	++RAGE_LOG_DISABLE;
#endif
	if (&sysMemAllocator::GetCurrent() == &sysMemAllocator::GetMaster())
		++s_TempNestingCount;
	else {
		Assert(!s_TempAllocator);
		s_TempAllocator = &sysMemAllocator::GetCurrent();
		s_TempContainer = &sysMemAllocator::GetContainer();
		sysMemAllocator::SetCurrent(sysMemAllocator::GetMaster());
		sysMemAllocator::SetContainer(sysMemAllocator::GetMaster());
	}
}


void sysMemEndTemp() {
#if RAGE_ENABLE_RAGE_NEW
	--RAGE_LOG_DISABLE;
#endif
	if (s_TempNestingCount)
		--s_TempNestingCount;
	else {
		sysMemAllocator::SetCurrent(*s_TempAllocator);
		sysMemAllocator::SetContainer(*s_TempContainer);
		s_TempAllocator = NULL;
		s_TempContainer = NULL;
	}
}

#if ENABLE_DEBUG_HEAP
static __THREAD sysMemAllocator* s_DebugAllocator = NULL;
static __THREAD int s_DebugNestingCount = 0;

void sysMemStartDebug() {
	if (s_DebugAllocator)
		++s_DebugNestingCount;
	else {
		s_DebugAllocator = &sysMemAllocator::GetCurrent();
		sysMemAllocator::SetCurrent(*sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_DEBUG_VIRTUAL));
	}
}


void sysMemEndDebug() {
	if (s_DebugNestingCount)
		--s_DebugNestingCount;
	else {
		sysMemAllocator::SetCurrent(*s_DebugAllocator);
		s_DebugAllocator = NULL;
	}
}


bool sysMemDebugIsActive() {
	return &sysMemAllocator::GetCurrent() == sysMemAllocator::GetMaster().GetAllocator(MEMTYPE_DEBUG_VIRTUAL);
}

static __THREAD sysMemAllocator* s_RecorderAllocator = NULL;
static __THREAD int s_RecorderNestingCount = 0;

void sysMemStartRecorder() 
{
	if (s_RecorderAllocator)
		++s_RecorderNestingCount;
	else 
	{
		s_RecorderAllocator = &sysMemAllocator::GetCurrent();
		sysMemAllocator::SetCurrent(*MEMMANAGER.GetRecorderAllocator());
	}
}


void sysMemEndRecorder() 
{
	if (s_RecorderNestingCount)
		--s_RecorderNestingCount;
	else 
	{
		sysMemAllocator::SetCurrent(*s_RecorderAllocator);
		s_RecorderAllocator = NULL;
	}
}


bool sysMemRecorderIsActive() 
{
	return &sysMemAllocator::GetCurrent() == MEMMANAGER.GetRecorderAllocator();
}
#endif


/*
PURPOSE
	Check the memory allocation error.
*/
// CompileTimeAssert(sizeof(kMemoryBucketNames)/sizeof(*kMemoryBucketNames)==MEMBUCKET_MAXBUCKETS);


#if SUPPORT_DEBUG_MEMORY_FILL
void debug_memory_fill(void *dest,size_t bytes) {

	if	(sysEnableRandomFill)
	{
		unsigned char *d = (unsigned char *)dest;
		while	(bytes)
		{
			*d = (unsigned char)(size_t)d;
			d++;
			bytes--;
		}

		return;
	}

	// see if we can use fast fill
	if (((sysMemoryFillWord & 0xff) == ((sysMemoryFillWord >> 8) & 0xff)) &&
		((sysMemoryFillWord & 0xff) == ((sysMemoryFillWord >> 16) & 0xff)) &&
		((sysMemoryFillWord & 0xff) == ((sysMemoryFillWord >> 24) & 0xff))) {
		sysMemSet(dest, sysMemoryFillWord & 0xff, bytes);
		return;
	}

	unsigned *d = (unsigned *) dest;
	unsigned w = sysMemoryFillWord;
	while (bytes > 3) {
		*d++ = w;
		bytes -= 4;
	}
	unsigned char *cd = (unsigned char*) d;
	while (bytes) {
		*cd++ = (unsigned char) w;
		w >>= 8;
		--bytes;
	}
}
#endif	// SUPPORT_DEBUG_MEMORY_FILL

void* physical_new_(size_t size,size_t align RAGE_NEW_EXTRA_ARGS) 
{ 
#if RAGE_ENABLE_RAGE_NEW
	void *result = sysMemAllocator::GetCurrent().LoggedAllocate(size,align,MEMTYPE_GAME_PHYSICAL,file,line); 
#else
	void *result = sysMemAllocator::GetCurrent().Allocate(size,align,MEMTYPE_GAME_PHYSICAL); 
#endif
#if !__NO_OUTPUT
	if (!result)
	{
		sysMemAllocator *current = sysMemAllocator::GetCurrent().GetAllocator(MEMTYPE_GAME_PHYSICAL);
		Warningf("physical_new(%u,%d) failed.(%d/%d).",(u32)size,(s32)align,(u32)current->GetLargestAvailableBlock(),(u32)current->GetHeapSize());
	}
#endif
#if RAGE_ENABLE_RAGE_NEW
	if (result) {
		RAGE_LOG_NEW(result,align,file,line);
	}
#endif
	return result;
}

void physical_delete(void *ptr) 
{
	sysMemAllocator::GetCurrent().Free(ptr); 
}

const char * sysMemGetBucketName(MemoryBucketIds bucket)
{
	switch(bucket)
	{
	case MEMBUCKET_INVALID:
		return "Invalid";
	case MEMBUCKET_DEFAULT:
		return "Default";
	case MEMBUCKET_ANIMATION:
		return "Animation";
	case MEMBUCKET_STREAMING:
		return "Streaming";
	case MEMBUCKET_WORLD:
		return "World";
	case MEMBUCKET_GAMEPLAY:
		return "Gameplay";
	case MEMBUCKET_FX:
		return "FX";
	case MEMBUCKET_RENDER:
		return "Rendering";
	case MEMBUCKET_PHYSICS:
		return "Physics";
	case MEMBUCKET_AUDIO:
		return "Audio";
	case MEMBUCKET_NETWORK:
		return "Network";
	case MEMBUCKET_SYSTEM:
		return "System";
	case MEMBUCKET_SCALEFORM:
		return "Scaleform";
	case MEMBUCKET_SCRIPT:
		return "Script";
	case MEMBUCKET_RESOURCE:
		return "Resource";
	case MEMBUCKET_UI:
		return "UI";
	case MEMBUCKET_DEBUG:
		return "Debug";
	default:
		return NULL;
	}
}


#if __XENON
void* sysMemVirtualAllocate(size_t size)
{
	return sysMemPhysicalAllocate(size);
}

void sysMemVirtualFree(void *ptr)
{
	sysMemPhysicalFree(ptr);
}

void* sysMemPhysicalAllocate(size_t size)
{
	// If allocation is multiple of 16MB then use 16MB pages
	void *ret;
	if((size & 0xffffff) == 0)
		ret = XPhysicalAlloc(size,MAXULONG_PTR,0,PAGE_READWRITE | MEM_16MB_PAGES);
	else
		ret = XPhysicalAlloc(size,MAXULONG_PTR,0,PAGE_READWRITE | MEM_LARGE_PAGES);

#	if LIST_PHYSICAL_ALLOCS_CALLBACK
	{
		SYS_CS_SYNC(s_PhysicalAllocsCritSec);
		const unsigned idx = s_NumPhysicalAllocs++;
		if (idx >= MAX_NUM_PHYSICAL_ALLOCS)
		{
			Quitf("Need to increase size of s_PhysicalAllocs[] in \"system/memory.cpp\"");
		}
		s_PhysicalAllocs[idx].base = ret;
		s_PhysicalAllocs[idx].size = size;
	}
#	endif

	return ret;
}

void sysMemPhysicalFree(void *ptr)
{
	if (ptr)
	{
		XPhysicalFree(ptr);

#		if LIST_PHYSICAL_ALLOCS_CALLBACK
		{
			SYS_CS_SYNC(s_PhysicalAllocsCritSec);
			for (unsigned idx=0; idx<s_NumPhysicalAllocs; ++idx)
			{
				if (s_PhysicalAllocs[idx].base == ptr)
				{
					s_PhysicalAllocs[idx] = s_PhysicalAllocs[--s_NumPhysicalAllocs];
					s_PhysicalAllocs[s_NumPhysicalAllocs].base = NULL;
					break;
				}
			}
		}
#		endif
	}
}
#elif RSG_DURANGO
void* sysMemVirtualAllocate(size_t size)
{
#if ENABLE_DEBUG_HEAP
	if (g_sysDirectVirtualEnabled)
		Quitf("FATAL ERROR: You are attempting to allocate system memory directly. This is not supported.");
#endif

	void* ptr;

#if __BANK
	// Round up to 4k size, then add 4k for guard page. 
	size_t adjustedSize = ((size + 4095U) & ~4095U) + 4096U;
	ptr = VirtualAlloc(NULL, adjustedSize, MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
	DWORD oldProtect;

	if (!ptr || !VirtualProtect((char*)ptr + adjustedSize - 4096,4096,PAGE_NOACCESS,&oldProtect))
	{
#if !__FINAL
		static bool bOnce = true;
		if (bOnce)
		{
			Errorf("sysMemVirtualAllocate(%u) failed.",size);
			bOnce = false;
		}
#endif // !__FINAL
	}
#else
	// OPTIMIZATION: Avoids TLB misses on Durango and saves 1.5+ ms
	const size_t flags = MEM_COMMIT | (((size & 0x3FFFFF) == 0) ? MEM_4MB_PAGES : MEM_LARGE_PAGES);
	ptr = VirtualAlloc(NULL, size, flags, PAGE_READWRITE);
#endif

	return ptr;
}

void sysMemVirtualFree(void *ptr)
{
	if (ptr && !VirtualFree(ptr,0,MEM_RELEASE))
		Errorf("sysMemVirtualFree(%p) failed",ptr);
}

void* sysMemPhysicalAllocate(size_t size, size_t alignment, PhysicalMemoryType memType /*= PhysicalMemoryType::PHYS_MEM_DEFAULT*/)
{
	Assert(memType >= PhysicalMemoryType::PHYS_MEM_MIN && memType <= PhysicalMemoryType::PHYS_MEM_MAX);

	size_t type;
	switch (memType)
	{
		/*
		Memory is CPU-cached and is coherent between the CPU and GPU. Memory is cached by both the CPU and the GPU. 
		GPU reads and writes will snoop CPU caches, but CPU reads and writes will not snoop GPU caches. 
		GPU memory access is I/O coherent, and will always uses the ONION bus.
		GPU reads and writes to coherent memory run at half and quarter rate, respectively. 
		If memory is written by the GPU, the appropriate range in the GPU caches must be flushed by the title before reading on the CPU. 
		If memory is written by the CPU, the appropriate range in the GPU caches must be flushed by the title before reading on the GPU. 
		*/
		case PhysicalMemoryType::PHYS_MEM_ONION_WRITEBACK: 
			type = PAGE_GPU_COHERENT | PAGE_READWRITE;
			break;
		case PhysicalMemoryType::PHYS_MEM_ONION_WRITECOMBINE: 
			type = PAGE_GPU_COHERENT | PAGE_WRITECOMBINE | PAGE_READWRITE;
			break;
		/*
		Memory is CPU-cached and is not coherent between the CPU and GPU. Memory is cached by both the CPU and the GPU. 
		GPU memory access is noncoherent, and it can usually use the GARLIC bus. 
		GPU reads and writes to non-coherent memory run at full rate. 
		If memory is written by the CPU, the appropriate ranges in both the CPU and GPU caches must be flushed by the title before reading on the GPU. 
		Cached noncoherent memory is not recommended for scenarios involving GPU writes and CPU reads, because this is difficult to perform correctly. 
		*/
		case PhysicalMemoryType::PHYS_MEM_GARLIC_WRITEBACK: 
			type = PAGE_READWRITE; // (GPU cannot write to this memory otherwise the CPU can corrupt it)
			break;
		/*
		Memory is CPU write-combined and is not coherent between the CPU and GPU. Memory pages are cached by the GPU, but not by the CPU. 
		GPU memory access is noncoherent, and can usually use the GARLIC bus. 
		GPU reads and writes to non-coherent memory run at full rate. 
		If memory is written by the CPU, the write combine buffers must be flushed (via a write memory barrier), and the appropriate range in the GPU caches must be flushed by the title before accessing on the GPU. 
		*/
		case PhysicalMemoryType::PHYS_MEM_GARLIC_WRITECOMBINE: 
			type = PAGE_WRITECOMBINE | PAGE_READWRITE;
			break;
		default:
			FastAssert(0); // This should never happen
			return NULL;
	}

	// On Durango graphics memory allocations must be multiples of 64KiB
	const size_t alignedSize = (size + (alignment - 1)) & ~(alignment - 1);
	const size_t flags = (MEM_GRAPHICS | MEM_RESERVE | MEM_COMMIT) | (((alignedSize & 0x3FFFFF) == 0) ? MEM_4MB_PAGES : MEM_LARGE_PAGES);

	void* ptr = VirtualAlloc(nullptr, alignedSize, flags, type);
	FastAssert(!(reinterpret_cast<size_t>(ptr) & (alignment - 1)));

	return ptr;
}

void sysMemPhysicalFree(void* ptr)
{
	if (ptr && !VirtualFree(ptr, 0, MEM_RELEASE))
		Errorf("sysMemVirtualFree(%p) failed",ptr);
}

void* sysMemHeapAllocate(size_t size)
{
	void *ptr = HeapAlloc(GetProcessHeap(), 0, size);		
	Assert(ptr != NULL);

#if RAGE_TRACKING && MEMORY_TRACKER
	if (ptr && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
	{		
		sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
		pTracker->Allocate(ptr, size);
	}
#endif

	return ptr;
}

bool sysMemHeapFree(void *ptr)
{
	if (ptr)
	{
#if RAGE_TRACKING && MEMORY_TRACKER
		if (diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{		
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
			pTracker->Free(ptr);
		}
#endif
		if (!HeapFree(GetProcessHeap(),0,ptr))
		{
			Assertf(0, "HeapFree failed - GetLastError - %u", GetLastError());
			return false;
		}
		return true;
	}
	return false;
}
#elif __WIN32PC
void* sysMemVirtualAllocate(size_t size, bool bTracked/*=false*/)
{
	// Round up to 4k size, then add 4k for guard page. 
	size_t adjustedSize = ((size + 4095U) & ~4095U) + 4096U;
	void *result = VirtualAlloc(NULL, adjustedSize, MEM_RESERVE | MEM_COMMIT | (bTracked ? MEM_WRITE_WATCH : 0), PAGE_READWRITE);
	DWORD oldProtect;
	if (!result || !VirtualProtect((char*)result + adjustedSize - 4096,4096,PAGE_NOACCESS,&oldProtect))
	{
#if !__FINAL
		static bool bOnce = true;
		if (bOnce)
		{
			Errorf("sysMemVirtualAllocate(%u) failed.",size);
			bOnce = false;
		}
#endif // !__FINAL
	}
	return result;
}

void sysMemVirtualFree(void *ptr)
{
	if (ptr && !VirtualFree(ptr,0,MEM_RELEASE))
		Errorf("sysMemVirtualFree(%p) failed",ptr);
}

void* sysMemPhysicalAllocate(size_t size)
{
	return sysMemVirtualAllocate(size);
}

void sysMemPhysicalFree(void *ptr)
{
	sysMemVirtualFree(ptr);
}

void* sysMemHeapAllocate(size_t size)
{
	void *ptr = HeapAlloc(GetProcessHeap(), 0, size);		
	Assert(ptr != NULL);
	return ptr;
}

bool sysMemHeapFree(void *ptr)
{
	if (ptr)
	{
		if (!HeapFree(GetProcessHeap(),0,ptr))
		{
			Assertf(0, "HeapFree failed - GetLastError - %u", GetLastError());
			return false;
		}
		return true;
	}
	return false;
}

bool sysMemLockMemory(void* memory, size_t size)
{
	DWORD origProtect;
	bool result = VirtualProtect((char*)memory,size,PAGE_READONLY,&origProtect) != 0;
	if (!result)
	{
#if !__FINAL
		Errorf("sysMemLockMemory(%p, %u) failed.",memory, size);
#endif // !__FINAL
	}
	return result;
}

bool sysMemUnlockMemory(void* memory, size_t size)
{
	DWORD origProtect;
	bool result = VirtualProtect((char*)memory,size,PAGE_READWRITE,&origProtect) != 0;
	if (!result)
	{
#if !__FINAL
		Errorf("sysMemUnlockMemory(%p, %u) failed.",memory, size);
#endif // !__FINAL
	}
	return result;
}

#if !RSG_DURANGO	// GlobalMemoryStatus and friends don't seem to work on Metro

u64 sysMemTotalFreeMemory()
{
	MEMORYSTATUSEX oMemoryStatus;
	oMemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	BOOL bRet = GlobalMemoryStatusEx(&oMemoryStatus);
	if (bRet == false)
	{
		Warningf("Failed to Query for System Memory - Error Code %d", GetLastError());
		return 0x7FFFFFFF;
	}
	return oMemoryStatus.ullAvailPhys;
}

u64 sysMemTotalMemory()
{
	static u64 uTotalMemory = 0;
	if (uTotalMemory == 0)
	{
		MEMORYSTATUSEX oMemoryStatus;
		oMemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
		BOOL bRet = GlobalMemoryStatusEx(&oMemoryStatus);
		if (bRet == false)
		{
			Warningf("Failed to Query for System Memory - Error Code %d", GetLastError());
			return 0x7FFFFFFF;
		}
		uTotalMemory = oMemoryStatus.ullTotalPhys;
	}
	return uTotalMemory;
}

u64 sysMemLargestFreeBlock()
{
	HANDLE process = OpenProcess(
        PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 
        false,
		GetCurrentProcessId());

	unsigned char *p = NULL;
    MEMORY_BASIC_INFORMATION info;
	u64 uLargestFreeBlock = 0;

    for ( p = NULL;
          VirtualQueryEx(process, p, &info, sizeof(info)) 
              == sizeof(info);
          p += info.RegionSize ) 
	{
		if (info.State == MEM_FREE) 
		{	
			//printf("%x, %u\n", info.BaseAddress, info.RegionSize/1024);
			uLargestFreeBlock = max(info.RegionSize, uLargestFreeBlock);
		}
    }
	return uLargestFreeBlock;
}

u64 sysMemTotalFreeAppMemory()
{
	HANDLE process = OpenProcess(
		PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 
		false,
		GetCurrentProcessId());

	unsigned char *p = NULL;
	MEMORY_BASIC_INFORMATION info;
	SIZE_T uTotalFree = 0;

	for ( p = NULL;
		VirtualQueryEx(process, p, &info, sizeof(info)) 
		== sizeof(info);
		p += info.RegionSize ) 
	{
		if (info.State == MEM_FREE) 
		{	
			uTotalFree += info.RegionSize;
		}
	}
	return (u64)uTotalFree;
}

u64 sysMemTotalKernelMemory()
{
#if !defined(RAGEBUILDER) && !defined(SCRIPTCOMPILER)
	PERFORMANCE_INFORMATION oMemoryInfo;
	if (GetPerformanceInfo(&oMemoryInfo, sizeof(oMemoryInfo)))
	{
		return (u64)(oMemoryInfo.KernelTotal * oMemoryInfo.PageSize);
	}
	Assert(0 && "Failed Memory Query - Tell Oscar");
#endif
	return 10 * 1024 * 1024;
}

u64 sysMemTotalFreeKernelMemory()
{
#if !defined(RAGEBUILDER) && !defined(SCRIPTCOMPILER)
	PERFORMANCE_INFORMATION oMemoryInfo;
	if (GetPerformanceInfo(&oMemoryInfo, sizeof(oMemoryInfo)))
	{
		return (u64)(oMemoryInfo.KernelNonpaged * oMemoryInfo.PageSize);
	}
	Assert(0 && "Failed Memory Query - Tell Oscar");
#endif
	return 10 * 1024 * 1024;
}

#endif	// !RSG_DURANGO

#elif __PPU
void* sysMemVirtualAllocate(size_t)
{
	Quitf("Don't call sysMemVirtualAllocate on PS3");
	return NULL;
}
	
void sysMemVirtualFree(void *)
{
	Quitf("Don't call sysMemVirtualFree on PS3");
}

void* sysMemPhysicalAllocate(size_t size)
{
	size = (size + 65535) & ~65535;
	CHECKPOINT_SYSTEM_MEMORY;
	sys_addr_t addr;
	int err = sys_memory_allocate(size, SYS_MEMORY_PAGE_SIZE_64K, &addr);
	CHECKPOINT_SYSTEM_MEMORY;
	if (err == CELL_OK)
		return (void*)(size_t)addr;
	else {
		Errorf("sysMemVirtualAllocate(%d) returned %x",size,err);
		return NULL;
	}
}

void sysMemPhysicalFree(void *ptr)
{
	if (ptr)
	{
		CHECKPOINT_SYSTEM_MEMORY;
		int err = sys_memory_free((sys_addr_t)ptr);
		CHECKPOINT_SYSTEM_MEMORY;
		if (err != CELL_OK)
			Errorf("sysMemVirtualFree(%p) returned %x",ptr,err);
	}
}
#elif __PSP2

static void *allocate_memory(size_t size,SceKernelMemBlockType type)
{
	SceUID id = sceKernelAllocMemBlock("mem",type,size,NULL);
	if (id < 0)
		return NULL;
	void *base = NULL;
	return sceKernelGetMemBlockBase(id,&base)==0? base : NULL;
}

void* sysMemVirtualAllocate(size_t size)
{
	return allocate_memory(size,SCE_KERNEL_MEMBLOCK_TYPE_USER_RW);
}

void* sysMemPhysicalAllocate(size_t size)
{
	return allocate_memory(size,SCE_KERNEL_MEMBLOCK_TYPE_USER_RW /*SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW*/);
}

void sysMemVirtualFree(void *ptr)
{
	SceKernelMemBlockInfo info;
	if (sceKernelGetMemBlockInfoByAddr(ptr,&info) == 0) {
		// sceKernelFreeMemBlock(info.access);		// WRONG
	}
}

void sysMemPhysicalFree(void *ptr)
{
	sysMemVirtualFree(ptr);
}


#elif __SPU
void* sysMemVirtualAllocate(size_t size)
{
	return memalign(128, size);
}

void sysMemVirtualFree(void *ptr)
{
	free(ptr);
}

void* sysMemPhysicalAllocate(size_t size)
{
	return sysMemVirtualAllocate(size);
}

void sysMemPhysicalFree(void *ptr)
{
	sysMemVirtualFree(ptr);
}

#elif RSG_ORBIS

#if __DEV
u64 s_TotalAllocatedMemory = 0;
u64 s_PeakAllocatedMemory = 0;
#endif // __DEV

template <size_t numPtrs,int memType> class memorySizeManager {
public:
	memorySizeManager() : count(0) { }

	void addSize(void *ptr,off_t offset,size_t size) {
		Assertf(count<numPtrs,"Increase size of this memorySizeManager");
		array[count].ptr = ptr;
		array[count].offset = offset;
		array[count].size = size;
		++count;

#if __DEV
		if (ptr)
		{
			s_TotalAllocatedMemory += size;
			if (s_PeakAllocatedMemory < s_TotalAllocatedMemory)
			{
				s_PeakAllocatedMemory = s_TotalAllocatedMemory;
				Displayf("Peak %" I64FMT "d", s_PeakAllocatedMemory);
			}
		}
#endif // __DEV

	}

	void releaseSize(void *ptr,off_t &offset,size_t &size) {
		for (size_t i=count; i--;) {
			if (array[i].ptr == ptr) {
				offset = array[i].offset;
				size = array[i].size;
				array[i] = array[--count];

#if __DEV
				s_TotalAllocatedMemory -= size;
#endif // __DEV

				return;
			}
		}
		Assertf(false,"bad pointer passed to releaseSize, double-free maybe?");
	}

	void* alloc(size_t size) {
		off_t offset;
		void *ptr = NULL;
		if (sceKernelAllocateDirectMemory(0,
			(SCE_KERNEL_MAIN_DMEM_SIZE),
			size,
			2 * 1024 * 1024, // alignment
			memType,
			&offset) < 0)
			return NULL;
		if (sceKernelMapDirectMemory(&ptr,
			size,
			SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_ALL,
			0,						//flags
			offset,
			2 * 1024 * 1024) < 0)
			return NULL;

		addSize(ptr,offset,size);
		return ptr;
	}

	void free(void *ptr) {
		if (ptr) {
			off_t offset;
			size_t size;
			releaseSize(ptr,offset,size);
			sceKernelReleaseDirectMemory(offset,size);
		}
	}
private:
	struct { void *ptr; off_t offset; size_t size; } array[numPtrs];
	size_t count;
};

#if SCE_ORBIS_SDK_VERSION < (0x00930020u)
memorySizeManager<512,SCE_KERNEL_WB_ONION_NONVOLATILE> s_debugManager;
memorySizeManager<128,SCE_KERNEL_WB_ONION_NONVOLATILE> s_virtualManager;
#else
memorySizeManager<512,SCE_KERNEL_WB_ONION> s_debugManager;
memorySizeManager<128,SCE_KERNEL_WB_ONION> s_virtualManager;
#endif

void* sysMemVirtualAllocate(size_t size)
{
#if ENABLE_DEBUG_HEAP
	if (g_sysDirectVirtualEnabled)
		Quitf("FATAL ERROR: You are attempting to allocate system memory directly. This is not supported.");
#endif

	void* ptr = s_virtualManager.alloc(size);
	Assertf(ptr != NULL, "Failed to allocate system memory %" I64FMT "d", size);
	return ptr;
}

void sysMemVirtualFree(void *ptr)
{
	s_virtualManager.free(ptr);
}

void* sysMemPhysicalAllocate(size_t size)
{
	return sysMemVirtualAllocate(size);
}

void sysMemPhysicalFree(void *ptr)
{
	sysMemVirtualFree(ptr);
}

void* sysMemFlexAllocate(size_t size)
{
#if ENABLE_DEBUG_HEAP
	if (g_sysDirectFlexEnabled)
		Quitf("FATAL ERROR: You are attempting to allocate flex memory directly. This is not supported.");
#endif

	void* ptr = nullptr;
	ASSERT_ONLY(const int result =) sceKernelMapFlexibleMemory(&ptr, size, SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_CPU_WRITE | SCE_KERNEL_PROT_GPU_READ | SCE_KERNEL_PROT_GPU_WRITE, 0);
	Assertf(ptr != NULL && result == SCE_OK, "Failed to allocate flex memory %" I64FMT "d", size);
	return ptr;
}

int sysMemFlexFree(void* ptr, size_t size)
{
	return sceKernelMunmap(ptr, size);
}

#endif

#if RSG_DURANGO && !__FINAL && !RSG_TOOL
void* VirtualAllocTracked(void* lpAddress, size_t dwSize, unsigned long flAllocationType, unsigned long flProtect)
{
	void* ptr = ::VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect);

#if RAGE_TRACKING && MEMORY_TRACKER
	if (ptr && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
	{		
		sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
		pTracker->Allocate(ptr, dwSize);
	}
#endif

	return ptr;
}

int VirtualFreeTracked(void* lpAddress, size_t dwSize, unsigned long dwFreeType)
{
#if RAGE_TRACKING && MEMORY_TRACKER
	if (lpAddress && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
	{		
		sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
		pTracker->Free(lpAddress);
	}
#endif

	return ::VirtualFree(lpAddress, dwSize, dwFreeType);
}
#endif

#if API_HOOKER
static sysCriticalSectionToken s_MemoryToken;
#define MEMORY_CRITICAL_SECTION sysCriticalSection cs(s_MemoryToken)

static __THREAD int s_VirtualAllocHookCount = 0;
static __THREAD int s_VirtualReAllocHookCount = 0;
static __THREAD int s_VirtualFreeHookCount = 0;

static __THREAD int s_HeapAllocHookCount = 0;
static __THREAD int s_HeapReAllocHookCount = 0;
static __THREAD int s_HeapFreeHookCount = 0;

static __THREAD int s_LocalFreeHookCount = 0;
static __THREAD int s_GlobalFreeHookCount = 0;

//
// NOTE: You must update this to match the address of ntdll.dll!LdrInitializeThunk()
//
static void* g_ldrAddress = (void*) 0x777dc34e;
static const int s_stackEntries = 16;

PVOID VirtualAllocHooked(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
#if RAGE_TRACKING && MEMORY_TRACKER
	++s_VirtualAllocHookCount;
#endif

	// Call the original function
	void* ptr = g_virtualAllocHooker(lpAddress, dwSize, flAllocationType, flProtect);

#if RAGE_TRACKING && MEMORY_TRACKER
	--s_VirtualAllocHookCount;

	if (ptr && !s_VirtualAllocHookCount)
	{
		{
			MEMORY_CRITICAL_SECTION;
			MEMMANAGER.m_virtualAllocTotal += dwSize;
			const size_t total = MEMMANAGER.m_virtualAllocTotal;
			(void) total;
		}		

		if (sysMemAllocator_sm_Current)
		{
			sysMemAutoUseDebugMemory mem;
			Memoryf("VirtualAlloc,%p,%d,%x\n", ptr, dwSize, flAllocationType);

			if (diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
			{		
				sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
				pTracker->Allocate(ptr, dwSize);
			}
		}
	}	
#endif

	return ptr;
}

BOOL VirtualFreeHooked(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType)
{
#if RAGE_TRACKING && MEMORY_TRACKER
	++s_VirtualFreeHookCount;
#endif

	BOOL status = g_virtualFreeHooker(lpAddress, dwSize, dwFreeType);

#if RAGE_TRACKING && MEMORY_TRACKER
	--s_VirtualFreeHookCount;

	if (status && !s_VirtualFreeHookCount)
	{
		{
			MEMORY_CRITICAL_SECTION;
			MEMMANAGER.m_virtualAllocTotal -= dwSize;
			const size_t total = MEMMANAGER.m_virtualAllocTotal;
			(void) total;
		}		

		if (sysMemAllocator_sm_Current)
		{
			sysMemAutoUseDebugMemory mem;
			Memoryf("VirtualFree,%p,%d,%x\n", lpAddress, dwSize, dwFreeType);

			if (diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
			{		
				sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
				pTracker->Free(lpAddress, dwSize);
			}
		}		
	}
#endif

	// Call the original function
	return status;
}

LPVOID HeapAllocHooked(HANDLE hHeap, DWORD dwFlags, SIZE_T dwSize)
{
#if RAGE_TRACKING && MEMORY_TRACKER	
	size_t trace[s_stackEntries] = { 0 };
	sysStack::CaptureStackTrace(trace, s_stackEntries, 1);

	// NOTE: For this to work we will need to parse the symbol file manually at startup
	//
	//if (!g_ldrAddress)
	//{
	//	// Look for ntdll.dll!LdrInitializeThunk() address
	//	for (int i = (s_stackEntries - 1); i >- 0; --i)
	//	{
	//		if (!trace[i])
	//			continue;

	//		char symname[256] = { 0 };
	//		sysStack::ParseMapFileSymbol(symname, sizeof(symname), trace[i]);
	//		if (!strcmp(symname, "nosymbols"))
	//			return g_heapAllocHooker(hHeap, dwFlags, dwSize);

	//		Memoryf("Symbol = %s\n", symname);
	//		if (strstr(symname, "LdrInitializeThunk"))
	//		{
	//			g_ldrAddress = reinterpret_cast<void*>(trace[i]);

	//			// Don't access TLS variables during DLL load
	//			return g_heapAllocHooker(hHeap, dwFlags, dwSize);
	//		}
	//	}
	//}

	// Don't access TLS variables during DLL load
	for (int i = (s_stackEntries - 1); i >- 0; --i)
	{
		if (reinterpret_cast<void*>(trace[i]) == g_ldrAddress)
			return g_heapAllocHooker(hHeap, dwFlags, dwSize);
	}

	// Call the original function
	++s_HeapAllocHookCount;
#endif

	void* ptr = g_heapAllocHooker(hHeap, dwFlags, dwSize);

#if RAGE_TRACKING && MEMORY_TRACKER
	--s_HeapAllocHookCount;

	if (ptr && !s_HeapAllocHookCount)
	{
		const size_t size = HeapSize(hHeap, dwFlags, ptr);
		{
			MEMORY_CRITICAL_SECTION;		
			MEMMANAGER.m_heapAllocTotal += size;
			const size_t total = MEMMANAGER.m_heapAllocTotal;
			(void) total;
		}		

		if (ptr && sysMemAllocator_sm_Current && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{
			sysMemAutoUseDebugMemory mem;			
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
			pTracker->Allocate(ptr, size);
		}
	}
#endif

	return ptr;
}

LPVOID HeapReAllocHooked(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwSize)
{	
#if RAGE_TRACKING && MEMORY_TRACKER	
	const size_t size = HeapSize(hHeap, dwFlags, lpMem);
	++s_HeapReAllocHookCount;
#endif

	// Call the original function	
	void* ptr = g_heapReAllocHooker(hHeap, dwFlags, lpMem, dwSize);	

#if RAGE_TRACKING && MEMORY_TRACKER
	--s_HeapReAllocHookCount;

	if (ptr && !s_HeapReAllocHookCount)
	{
		{
			MEMORY_CRITICAL_SECTION;
			MEMMANAGER.m_heapAllocTotal -= size;
			const size_t total = MEMMANAGER.m_heapAllocTotal;
			(void) total;
		}		

		if (sysMemAllocator_sm_Current && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{
			sysMemAutoUseDebugMemory mem;			
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();		
			pTracker->Free(lpMem, size);
		}
	}
#endif

	return ptr;
}

BOOL HeapFreeHooked(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
#if RAGE_TRACKING && MEMORY_TRACKER
	//++s_HeapFreeHookCount;
	const size_t size = lpMem ? HeapSize(hHeap, dwFlags, lpMem) : 0;
#endif

	// Call the original function		
	BOOL status = g_heapFreeHooker(hHeap, dwFlags, lpMem);	
	Assertf(status, "Unable to free heap memory: %p (%d bytes)", lpMem, size);

#if RAGE_TRACKING && MEMORY_TRACKER
	if (status && !s_HeapFreeHookCount)
	{
		{
			MEMORY_CRITICAL_SECTION;
			MEMMANAGER.m_heapAllocTotal -= size;
			const size_t total = MEMMANAGER.m_heapAllocTotal;
			(void) total;
		}	

		if (lpMem && sysMemAllocator_sm_Current && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{
			sysMemAutoUseDebugMemory mem;
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
			pTracker->Free(lpMem, size);
		}
	}	
#endif

	return status;
}

HLOCAL LocalFreeHooked(LPVOID lpMem)
{
#if RAGE_TRACKING && MEMORY_TRACKER	
	++s_LocalFreeHookCount;
	const size_t size = lpMem ? LocalSize(lpMem) : 0;
#endif

	// Call the original function
	HLOCAL status = g_localFreeHooker(lpMem);	
	Assertf(!status, "Unable to free local memory: %p (%d bytes)", lpMem, size);

#if RAGE_TRACKING && MEMORY_TRACKER
	--s_LocalFreeHookCount;

	if (!status && !s_LocalFreeHookCount)
	{
		{
			MEMORY_CRITICAL_SECTION;
			MEMMANAGER.m_heapAllocTotal -= size;
			const size_t total = MEMMANAGER.m_heapAllocTotal;
			(void) total;
		}	

		if (lpMem && sysMemAllocator_sm_Current && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{
			sysMemAutoUseDebugMemory mem;
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
			pTracker->Free(lpMem, size);
		}
	}	
#endif

	return status;
}

HLOCAL GlobalFreeHooked(LPVOID lpMem)
{
#if RAGE_TRACKING && MEMORY_TRACKER	
	++s_GlobalFreeHookCount;
	const size_t size = lpMem ? GlobalSize(lpMem) : 0;
#endif

	// Call the original function
	HLOCAL status = g_globalFreeHooker(lpMem);	
	Assertf(!status, "Unable to free global memory: %p (%d bytes)", lpMem, size);

#if RAGE_TRACKING && MEMORY_TRACKER
	--s_GlobalFreeHookCount;

	if (!status && !s_GlobalFreeHookCount)
	{
		{
			MEMORY_CRITICAL_SECTION;
			MEMMANAGER.m_heapAllocTotal -= size;
			const size_t total = MEMMANAGER.m_heapAllocTotal;
			(void) total;
		}	

		if (lpMem && sysMemAllocator_sm_Current && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{
			sysMemAutoUseDebugMemory mem;
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
			pTracker->Free(lpMem, size);
		}
	}	
#endif

	return status;
}
#endif

#if ENABLE_DEBUG_HEAP
void* sysMemDebugAllocate(size_t size)
{
	if (g_sysDirectDebugEnabled)
		Quitf("FATAL ERROR: You are attempting to allocate system memory directly. This is not supported.");

	// Round up to next multiple of 64k
	size = (size + 65535) & ~65535;
#if __PS3
	sys_memory_info_t mem_info;
	sys_memory_get_user_memory_size( &mem_info );
	if (mem_info.total_user_memory <= 220 * 1024 * 1024)	// don't even try on Test station or kit in retail mode
		return 0;
	sys_addr_t addr = 0;
	if (sys_memory_allocate(size, SYS_MEMORY_PAGE_SIZE_64K, &addr) == CELL_OK) {
		/* sys_page_attr_t attr;
		if (sys_memory_get_page_attribute(addr,&attr) == CELL_OK)
			printf("page size %d, access %llx (%s)\n",attr.page_size,attr.access_right,attr.access_right & SYS_MEMORY_ACCESS_RIGHT_SPU_THR?"spu thread":"no spu");
		else
			printf("*** sys_memory_get_page_attribute on %p failed\n",addr); */
		return (void*)(size_t)addr;
	}
	else
		return NULL;
#elif __XENON
	DWORD status = DM_CONSOLEMEMCONFIG_NOADDITIONALMEM;
	if (SUCCEEDED(DmGetConsoleDebugMemoryStatus(&status)) && status == DM_CONSOLEMEMCONFIG_ADDITIONALMEMENABLED)
		return DmDebugAlloc(size);
	else
		return NULL;
#elif RSG_DURANGO
	Assertf((size & 0x3fffff) == 0, "sysMemDebugAllocate called called with a non 4MB-aligned size: %" SIZETFMT "d!", size);
	return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_4MB_PAGES, PAGE_READWRITE);
#elif __WIN32PC
	return VirtualAlloc(NULL, size,  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#elif RSG_ORBIS
	return sysMemFlexAllocate(size);
#endif
}


void sysMemDebugFree(void *ptr)
{
	if (g_sysDirectDebugEnabled)
		Quitf("FATAL ERROR: You are attempting to allocate system memory directly. This is not supported.");

	if (ptr)
#if __PS3
		sys_memory_free((sys_addr_t)ptr);
#elif __XENON
		// Only way pointer could be nonzero is if we were allowed to call DmDebugAlloc anyway so no reason to repeat DmGetCDMS call again
		DmDebugFree(ptr);	
#elif __WIN32PC || RSG_DURANGO
		VirtualFree(ptr,0,MEM_RELEASE);
#elif RSG_ORBIS
	{
		off_t offset;
		size_t size;
		s_debugManager.releaseSize(ptr,offset,size);
		munmap(ptr,size);
		sceKernelReleaseDirectMemory(offset,size);
	}
#endif
}
#endif			// ENABLE_DEBUG_HEAP

#if !__FINAL
int sysMemAllocator::sm_BreakOnAlloc = -1;
void *sysMemAllocator::sm_BreakOnAddr = (void*)-1;
const char * sysMemAllocator::sm_MemoryBucketNames[16] = { NULL };
#endif
int __THREAD sysMemAllocId;

#if RSG_DURANGO

	// Pair of hook functions per allocator id.
	struct XMemHookPair
	{
		sysMemXMemAllocHook     *allocHook;
		sysMemXMemFreeHook      *freeHook;
	};

	// Array of hook function pairs, indexed by allocator id.
	static XMemHookPair s_XMemHooks[256] =
	{
#		define XMEMHOOKPAIR_X2      {XMemAllocDefault,XMemFreeDefault}, {XMemAllocDefault,XMemFreeDefault}
#		define XMEMHOOKPAIR_X4      XMEMHOOKPAIR_X2,   XMEMHOOKPAIR_X2
#		define XMEMHOOKPAIR_X8      XMEMHOOKPAIR_X4,   XMEMHOOKPAIR_X4
#		define XMEMHOOKPAIR_X16     XMEMHOOKPAIR_X8,   XMEMHOOKPAIR_X8
#		define XMEMHOOKPAIR_X32     XMEMHOOKPAIR_X16,  XMEMHOOKPAIR_X16
#		define XMEMHOOKPAIR_X64     XMEMHOOKPAIR_X32,  XMEMHOOKPAIR_X32
#		define XMEMHOOKPAIR_X128    XMEMHOOKPAIR_X64,  XMEMHOOKPAIR_X64
#		define XMEMHOOKPAIR_X256    XMEMHOOKPAIR_X128, XMEMHOOKPAIR_X128

		XMEMHOOKPAIR_X256

#		undef XMEMHOOKPAIR_X256
#		undef XMEMHOOKPAIR_X128
#		undef XMEMHOOKPAIR_X64
#		undef XMEMHOOKPAIR_X32
#		undef XMEMHOOKPAIR_X16
#		undef XMEMHOOKPAIR_X8
#		undef XMEMHOOKPAIR_X4
#		undef XMEMHOOKPAIR_X2
	};

	void *sysMemXMemAlloc(size_t size, u64 attributes) {
		XALLOC_ATTRIBUTES a;
		a.dwAttributes = attributes;
		void* ptr = s_XMemHooks[a.s.dwAllocatorId].allocHook(size, attributes);
		if(!ptr)
		{
			diagLoggedPrintf("WARNING: sysMemXMemAlloc(%u) with attribs 0x%" I64FMT "x returned null from allocator ud %u, requesting type %u, with alignment flags %u",
				(u32)size, attributes, a.s.dwAllocatorId, a.s.dwMemoryType, a.s.dwAlignment );
		}
#if RAGE_TRACKING && MEMORY_TRACKER
		else if (diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{		
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
			pTracker->Allocate(ptr, size, a.s.dwAllocatorId);
		}
#endif
		return ptr;
	}

	void sysMemXMemFree(void *ptr, u64 attributes) {
		XALLOC_ATTRIBUTES a;
		a.dwAttributes = attributes;
		s_XMemHooks[a.s.dwAllocatorId].freeHook(ptr, attributes);

#if RAGE_TRACKING && MEMORY_TRACKER
		if (ptr && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{		
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
			pTracker->Free(ptr, a.s.dwAllocatorId);
		}
#endif
	}

	void sysMemSetXMemHooks(u8 allocatorId, sysMemXMemAllocHook *allocHook, sysMemXMemFreeHook *freeHook) {
		s_XMemHooks[allocatorId].allocHook = allocHook;
		s_XMemHooks[allocatorId].freeHook  = freeHook;
	}

	size_t sysMemXMemCalculateAlignment( u64 const alignmentFlags )
	{
		size_t const c_alignResult = ( 1 << alignmentFlags );
		return c_alignResult;
	}

#endif // RSG_DURANGO

} // namespace rage

extern "C" void* rage_malloc(size_t siz) {
	return rage_new char[siz];
}


extern "C" void rage_free(void *ptr) {
	delete[] (char*) ptr;
}
