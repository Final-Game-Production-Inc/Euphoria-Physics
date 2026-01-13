#include "virtualallocator.h"

#include "system/ipc.h"
#include "system/memvisualize.h"
#include "system/memmanager.h"
#include "system/messagequeue.h"
#include "system/systemallocator_system.h"
#include "system/spinlock.h"		// for sys_lwsync
#include "system/timer.h"			// TEMPORARY
#include "math/amath.h"				// for Log2Floor/Log2Ceil
#include <algorithm>

#if RSG_ORBIS
#include <kernel.h>
#include <sdk_version.h>
# if SCE_ORBIS_SDK_VERSION != (0x01700601u)
# error "You need to install the special SDK 1.73 runtime patch on this machine."
# error "It's located at <S:\Development\Orbis\SDK Manager\Current\1.73 SDK Patch> for R*North"
# error "It's located at <\\rsgsanfil4\setup\ORBIS-Tools\SDK1.73-special-version-for-GTA5> for R*SD"
# error "It's located at <N:\RSGLDS\Software\Platform_Development_Software\Sony\PS4\SDK\Current\SDK173-Patch> for R*Leeds"
# error "It's located at <N:\RSGTOR\IT_Programs\Installables.GTAV\17.PS4\SDK173-Patch> for R*Toronto"
# error "It's located at <N:\RSGNWE\GENERAL\SDKs and Build Tools\PS4 Development\Current\1.730.000_Patch_GTAV> for R*NE"
# error "See the README.txt file there for where to extract it."
# endif
#else
#include "system/xtl.h"
#endif

#if RSG_ORBIS || RSG_DURANGO

namespace rage {

#if !ENABLE_BUDDY_ALLOCATOR

sysMemVirtualAllocator *sysMemVirtualAllocator::sm_Instance;

// Given a list (by height) return the appropriate bitmask indicating a completely free page.
const u8 sysMemVirtualAllocator::sm_FullBits[sysMemVirtualAllocator::POOLSHIFT] = { 0xFF, 0x55, 0x11 };
// Given a list (by height) return the number of pages per buddy page (same as the number of 1 bits in sm_FullBits)
// This is the same as 1 << (POOLSHIFT-list)
const u8 sysMemVirtualAllocator::sm_PagesBySize[sysMemVirtualAllocator::POOLSHIFT] = { 8, 4, 2 };

sysMemVirtualAllocator::sysMemVirtualAllocator(size_t heapSize) : m_PhysicalHeapSize(heapSize) {
	FatalAssert(!(m_PhysicalHeapSize & BuddyPageSizeMask));
	// Increase the heap size by enough to hold the physical metadata.
	size_t physicalPageCount = heapSize / BuddyPageSize;
	size_t vmExpand = 4;
	size_t virtualPageCount = physicalPageCount*vmExpand;
	m_PhysicalPageCount = physicalPageCount;
	m_VirtualPageCount = virtualPageCount;
	size_t nodeSize = (physicalPageCount+virtualPageCount)*sizeof(sysTinyBuddyHeap::Node);

#if RSG_ORBIS
	size_t metaSize = (nodeSize + (physicalPageCount<<POOLSHIFT)*sizeof(u32) + BuddyPageSizeMask) & ~BuddyPageSizeMask;
	long metaOffset;
	if (sceKernelAllocateDirectMemory(0,SCE_KERNEL_MAIN_DMEM_SIZE,metaSize,BuddyPageSize,SCE_KERNEL_WB_ONION,&metaOffset) != SCE_OK)
		Quitf("Unable to allocate os meta heap");
	void *metaData = NULL;
	if (sceKernelMapDirectMemory(&metaData,metaSize,SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE,0,metaOffset,BuddyPageSize) != SCE_OK)
		Quitf("Unable to map meta region");
	if (sceKernelAllocateDirectMemory(0,SCE_KERNEL_MAIN_DMEM_SIZE,heapSize,BuddyPageSize,SCE_KERNEL_WB_ONION,(long*)&m_PhysicalBase) != SCE_OK)
		Quitf("Unable to allocate os physheap");
	m_VirtualBase = (void*) SCE_KERNEL_APP_MAP_AREA_START_ADDR;

#elif RSG_DURANGO
	size_t pageArraySize = physicalPageCount*sizeof(ULONG_PTR);
	size_t metaSize = (pageArraySize + nodeSize + (physicalPageCount<<POOLSHIFT)*sizeof(u32) + BuddyPageSizeMask) & ~BuddyPageSizeMask;
	void *metaData = VirtualAllocTracked(NULL, metaSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!metaData)
		Quitf("Unable to allocate os meta heap");
	m_PageArray = (ULONG_PTR*)metaData;
	ULONG_PTR numPages = physicalPageCount;
	if (!AllocateTitlePhysicalPages(GetCurrentProcess(), MEM_LARGE_PAGES, &numPages, m_PageArray) || numPages!=physicalPageCount)
		Quitf("Unable to allocate title physical pages.");
	std::sort(m_PageArray, m_PageArray+numPages); // sort the page frame numbers so that address ranges m_PhysicalHeap thinks are contiguous, are as contiguous as possible
	metaData = (char*)metaData+pageArraySize;
	m_VirtualBase = VirtualAllocTracked(NULL, heapSize * (vmExpand+1), MEM_RESERVE | MEM_GRAPHICS | MEM_LARGE_PAGES, PAGE_READWRITE);
	if (!m_VirtualBase)
		Quitf("Unable to allocate heap virtual address space");

#endif

	m_PhysicalHeap.Init((u32)physicalPageCount, (sysTinyBuddyHeap::Node*)metaData);
	metaData = (sysTinyBuddyHeap::Node*)metaData+physicalPageCount;
	m_VirtualHeap.Init((u32)virtualPageCount, (sysTinyBuddyHeap::Node*)metaData);
	metaData = (sysTinyBuddyHeap::Node*)metaData+virtualPageCount;

	m_PoolUserData = (u32*)metaData;
	for (u32 i=0; i<physicalPageCount<<POOLSHIFT; ++i)
		m_PoolUserData[i] = INVALID_USER_DATA;

	for (int i=0; i<POOLSHIFT; i++) {
		m_FirstBySize[i] = sysTinyBuddyHeap::TERMINATOR;
		m_FreeBySize[i] = 0;
	}

	sm_Instance = this;
}


size_t sysMemVirtualAllocator::ComputeAllocationSize(size_t size) {
	if (!size)
		return 0;
	else {
		size = (size + PooledBuddyPageSizeMask) & ~PooledBuddyPageSizeMask;
		// If it's not a perfect power of two, bump the shift count up by one.
		return (size_t)1 << Log2Ceil(size);
	}
}

// TODO: Should be inline
/*inline*/ bool sysMemVirtualAllocator::IsPooled(const void *ptr) const {
	return ((uptr)ptr-(uptr)m_VirtualBase)/BuddyPageSize < m_PhysicalPageCount;
}

inline u32 sysMemVirtualAllocator::GetVirtualPoolIndex(const void *ptr) const {
	const uptr poolIdx = ((uptr)ptr-(uptr)m_VirtualBase)/PooledBuddyPageSize;
#if __ASSERT
	const uptr physIdx = poolIdx>>POOLSHIFT;
	FatalAssert(physIdx < m_PhysicalPageCount);
	const u32 height = m_PhysicalHeap.GetPooledNodeAllocHeight(physIdx);
	FatalAssert((poolIdx&(((uptr)1<<height)-1)) == 0);
#endif
	return (u32)poolIdx;
}

inline u32 sysMemVirtualAllocator::GetVirtualPageIndex(const void *ptr) const {
	const uptr idx = (((uptr)ptr-(uptr)m_VirtualBase)/BuddyPageSize)-m_PhysicalPageCount;
	FatalAssert(idx < 0x100000000uLL);
	return (u32)idx;
}

inline void *sysMemVirtualAllocator::GetVirtualPointer(u32 idx) const {
	return (char*)m_VirtualBase+((uptr)idx+m_PhysicalPageCount)*BuddyPageSize;
}

size_t sysMemVirtualAllocator::GetSize(const void *ptr) const {
	if (IsPooled(ptr))
		return ((size_t)1<<m_PhysicalHeap.GetPooledNodeAllocHeight(GetVirtualPoolIndex(ptr)>>POOLSHIFT))*PooledBuddyPageSize;
	else
		return (size_t)m_VirtualHeap.GetSize(GetVirtualPageIndex(ptr))*BuddyPageSize;
}

void sysMemVirtualAllocator::SetUserData(const void *ptr,u32 userData) {
	if (IsPooled(ptr))
		m_PoolUserData[GetVirtualPoolIndex(ptr)] = userData;
	else
		m_VirtualHeap.SetUserData(GetVirtualPageIndex(ptr),userData);
}

u32 sysMemVirtualAllocator::GetUserData(const void *ptr) const {
	if (IsPooled(ptr))
		return m_PoolUserData[GetVirtualPoolIndex(ptr)];
	else
		return m_VirtualHeap.GetUserData(GetVirtualPageIndex(ptr));
}

bool sysMemVirtualAllocator::IsValidPointer(const void *ptr) const {
	return ((uptr)ptr-(uptr)m_VirtualBase)/BuddyPageSize < m_PhysicalPageCount+m_VirtualPageCount;
}

const void *sysMemVirtualAllocator::GetCanonicalBlockPtr(const void *ptr) const {
	if (IsPooled(ptr)) {
		uptr poolIdx = ((uptr)ptr-(uptr)m_VirtualBase)/PooledBuddyPageSize;
		const uptr physIdx = poolIdx>>POOLSHIFT;
		FatalAssert(physIdx < m_PhysicalPageCount);
		const u32 height = m_PhysicalHeap.GetPooledNodeAllocHeight(physIdx);
		poolIdx &= -((ptrdiff_t)1<<height);
		return (void*)((uptr)m_VirtualBase+poolIdx*PooledBuddyPageSize);
	}
	else {
		const u32 idx = m_VirtualHeap.GetFirstNode(GetVirtualPageIndex(ptr));
		return GetVirtualPointer(idx);
	}
}

#if RSG_DURANGO
static inline DWORD GetWin32MemProtectFlags(u32 memtype) {
	return
		  ((memtype & MEMTYPE_CPU_RO)     ? PAGE_READONLY     : PAGE_READWRITE)
		| ((memtype & MEMTYPE_CPU_WB)     ? 0                 : PAGE_WRITECOMBINE)
		| ((memtype & MEMTYPE_GPU_RO)     ? PAGE_GPU_READONLY : 0)
		| ((memtype & MEMTYPE_GPU_GARLIC) ? 0                 : PAGE_GPU_COHERENT);
}
#elif RSG_ORBIS
static inline int GetOrbisMemType(u32 memtype) {
	return (memtype & MEMTYPE_CPU_WB)
		?   (memtype & MEMTYPE_GPU_GARLIC) ?   /*SCE_KERNEL_WB_GARLIC*/10   : SCE_KERNEL_WB_ONION
		: /*(memtype & MEMTYPE_GPU_GARLIC) ?*/   SCE_KERNEL_WC_GARLIC     /*: SCE_KERNEL_WC_ONION*/; // kernel doesn't support WC/ONION
}
static inline int GetOrbisMemProtectionFlags(u32 memtype) {
	return SCE_KERNEL_PROT_CPU_READ | SCE_KERNEL_PROT_GPU_READ
		| ((memtype & MEMTYPE_CPU_RO) ? 0 : SCE_KERNEL_PROT_CPU_WRITE)
		| ((memtype & MEMTYPE_GPU_RO) ? 0 : SCE_KERNEL_PROT_GPU_WRITE);
}
#endif

void sysMemVirtualAllocator::SetMemTypeKeepContents(const void *addr, size_t size, u32 memtype) {
	SYS_CS_SYNC(m_CS);

	if (size < BuddyPageSize)
		return;
	FatalAssert((size & BuddyPageSizeMask) == 0);

#if RSG_DURANGO
	DWORD protect = GetWin32MemProtectFlags(memtype);
	uptr idx = GetVirtualPageIndex(addr);
	uptr endIdx = idx + size/BuddyPageSize;

	uptr changeIdx = sysTinyBuddyHeap::TERMINATOR;
	while (idx <= endIdx) {
		if (idx == endIdx || m_VirtualHeap.GetMemType(idx) == memtype) {
			if (changeIdx != sysTinyBuddyHeap::TERMINATOR) {
				void *p = GetVirtualPointer(changeIdx);
				size_t s = (idx-changeIdx)*BuddyPageSize;
				DWORD oldProtect;
				if (!VirtualProtect(p,s,protect,&oldProtect))
					Quitf("VirtualProtect call %p, 0x%010zx, 0x%x failed, 0x%08x.",p,s,protect,GetLastError());
				do {
					m_VirtualHeap.SetMemType(changeIdx, memtype);
				} while (++changeIdx != idx);
				changeIdx = sysTinyBuddyHeap::TERMINATOR;
			}
		}
		else if (changeIdx == sysTinyBuddyHeap::TERMINATOR)
			changeIdx = idx;
		++idx;
	}

#elif RSG_ORBIS
	int type = GetOrbisMemType(memtype);
	int prot = GetOrbisMemProtectionFlags(memtype);
	uptr idx = GetVirtualPageIndex(addr);
	uptr endIdx = idx + size/BuddyPageSize;

	uptr changeIdx = sysTinyBuddyHeap::TERMINATOR;
	while (idx <= endIdx) {
		if (idx == endIdx || m_VirtualHeap.GetMemType(idx) == memtype) {
			if (changeIdx != sysTinyBuddyHeap::TERMINATOR) {
				void *p = GetVirtualPointer(changeIdx);
				size_t s = (idx-changeIdx)*BuddyPageSize;
				int ret = sceKernelMtypeprotect(p,s,type,prot);
				if (ret != SCE_OK)
					Quitf("sceKernelMtypeprotect call %p, 0x%010zx, 0x%x, 0x%x failed, 0x%08x.",p,s,type,prot,ret);
				do {
					m_VirtualHeap.SetMemType(changeIdx, memtype);
				} while (++changeIdx != idx);
				changeIdx = sysTinyBuddyHeap::TERMINATOR;
			}
		}
		else if (changeIdx == sysTinyBuddyHeap::TERMINATOR)
			changeIdx = idx;
		++idx;
	}
#endif
}

void sysMemVirtualAllocator::SetMemTypeDiscardContents(const void *addr, size_t size, u32 memtype) {
	// Hoping in the future that we can get functions exposed in the SDK/XDK to
	// allow memory type changes where the contents are undefinted.  This would
	// remove the need for the OS to do some fairly expensive cache flushes.
	//
	// Currently there is no such function, so SetMemTypeDiscardContents just
	// calls through to SetMemTypeKeepContents, but keep this API so that
	// switching over is simple in the future.
	//
	SetMemTypeKeepContents(addr, size, memtype);
}

u32 sysMemVirtualAllocator::GetMemType(const void *addr) {
	if (IsPooled(addr))
		return MEMTYPE_DEFAULT;
	else
		return m_VirtualHeap.GetMemType(GetVirtualPageIndex(addr));
}

void sysMemVirtualAllocator::SetSmallPageDefaultMemType(void *ptr) {
#if RSG_DURANGO
	size_t size = BuddyPageSize;
	u32 idx = GetVirtualPoolIndex(ptr)>>POOLSHIFT;
	// Notice that we are using m_PhysicalHeap here, not m_VirtualHeap.  Page indices <
	// m_PhysicalPageCount are the same for both heaps.  So there is no reason for
	// copying the memory type information from m_PhysicalHeap to m_VirtualHeap.
	if (m_PhysicalHeap.GetMemType(idx) != MEMTYPE_DEFAULT) {
		DWORD protect = PAGE_READWRITE|PAGE_GPU_READONLY;
		FatalAssert(protect == GetWin32MemProtectFlags(MEMTYPE_DEFAULT));
		void *mem = (void*)((uptr)ptr&~BuddyPageSizeMask);
		DWORD oldProtect;
		if (!VirtualProtect(mem,size,protect,&oldProtect))
			Quitf("VirtualProtect call %p, 0x%010zx, 0x%x failed, 0x%08x.",mem,size,protect,GetLastError());
		m_PhysicalHeap.SetMemType(idx,MEMTYPE_DEFAULT);
	}

#elif RSG_ORBIS
	size_t size = BuddyPageSize;
	u32 idx = GetVirtualPoolIndex(ptr)>>POOLSHIFT;
	if (m_PhysicalHeap.GetMemType(idx) != MEMTYPE_DEFAULT) {
		int type = SCE_KERNEL_WB_ONION;
		int prot = SCE_KERNEL_PROT_CPU_READ|SCE_KERNEL_PROT_CPU_WRITE|SCE_KERNEL_PROT_GPU_READ|SCE_KERNEL_PROT_GPU_WRITE;
		void *mem = (void*)((uptr)ptr&~BuddyPageSizeMask);
		int ret = sceKernelMtypeprotect(mem,size,type,prot);
		if (ret != SCE_OK)
			Quitf("sceKernelMtypeprotect call %p, 0x%010zx, 0x%x, 0x%x failed, 0x%08x.",mem,size,type,prot,ret);
		m_PhysicalHeap.SetMemType(idx,MEMTYPE_DEFAULT);
	}
#endif
}

#if RSG_ORBIS
// WARNING: MAX_NUM macro argument evaluated multiple times!
#define AllocaBatchMapping(MAX_NUM) \
	rage_placement_new ((BatchMapping*)(((uptr)RageAlloca(sizeof(BatchMapping)+((MAX_NUM)-1)*sizeof(SceKernelBatchMapEntry)+15)+15)&~15)) BatchMapping(MAX_NUM)
#define AddMapping(_addr,_page,_size,_memtype) do { \
	FatalAssert(bm->num < bm->maxNum); \
	bm->entries[bm->num].start = (_addr); \
	bm->entries[bm->num].offset = m_PhysicalBase + BuddyPageSize * (_page); \
	bm->entries[bm->num].length = (_size); \
	bm->entries[bm->num].protection = GetOrbisMemProtectionFlags(_memtype); \
	bm->entries[bm->num].operation = SCE_KERNEL_MAP_OP_MAP_DIRECT; \
	++(bm->num); \
} while (0)
#elif RSG_DURANGO
void sysMemVirtualAllocator::AddMapping(void *_addr,u32 _page,size_t _size,u32 _memtype) {
	DWORD protect = GetWin32MemProtectFlags(_memtype);
	if (!MapTitlePhysicalPages((_addr),(_size) / BuddyPageSize,MEM_LARGE_PAGES | MEM_GRAPHICS,protect,m_PageArray + (_page)))
		Quitf("MapTitlePhysicalPages failed - %p / %x / %" SIZETFMT "x.",_addr,_page,_size);
}
#endif

#if RSG_ORBIS
#define AddUnmapping(_addr,_size) do { \
	FatalAssert(bm->num < bm->maxNum); \
	bm->entries[bm->num].start = (_addr); \
	bm->entries[bm->num].offset = 0; \
	bm->entries[bm->num].length = (_size); \
	bm->entries[bm->num].protection = 0; \
	bm->entries[bm->num].operation = SCE_KERNEL_MAP_OP_UNMAP; \
	++(bm->num); \
} while (0)
#define MEMTYPES_THAT_MUST_BE_REVERTED (1<<(MEMTYPE_CPU_RO|MEMTYPE_CPU_WB|MEMTYPE_GPU_RO|MEMTYPE_GPU_GARLIC))
#elif RSG_DURANGO
#define AddUnmapping(_addr,_size) do { \
	if (!VirtualFreeTracked((_addr), (_size), MEM_DECOMMIT)) \
		Quitf("AddUnmapping - VirtualFreeTracked(%p,%" SIZETFMT "u) failed, Allocated %" SIZETFMT "uM",(_addr),u32(_size)); \
} while (0)
#endif


#if RSG_ORBIS
void sysMemVirtualAllocator::DoBatchMapping(BatchMapping *bm) {
	// Note this really shouldn't fail unless there's a bug in this code.
	if (bm->num) {
		int outNum = 0;
		int ret = sceKernelBatchMap(bm->entries,bm->num,&outNum);
		if (ret != SCE_OK || bm->num != outNum)
			Quitf("Internal error, sceKernelBatchMap failed, 0x%08x", ret);
	}
}
#endif

u32 sysMemVirtualAllocator::AllocatePhysical(size_t *outPaSizePairs,size_t size) {
	SYS_CS_SYNC(m_CS);
	// Allocation size must be a multiple of BuddyPageSize, but it need not be a
	// power-of-two multiple.  Since the tiny buddy heap requires power-of-two
	// multiples, split into multiple allocations.  Also means this call can
	// succeed with fragmented physical memory.
	FatalAssert((size & BuddyPageSizeMask) == 0);
	u32 numPages = (u32)(size/BuddyPageSize);
	size_t *out = outPaSizePairs;
	while (numPages) {
		u32 thisCount = 1<<Log2Floor(numPages);
		u32 result;
		for (;;) {
			result = m_PhysicalHeap.Allocate(thisCount,MEMTYPE_DEFAULT); // TODO: memType ???
			if (result != sysTinyBuddyHeap::TERMINATOR)
				break;
			if (thisCount == 1) {
				while (out > outPaSizePairs) {
					out -= 2;
					FreePhysical(*out);
				}
				return 0;
			}
			thisCount >>= 1;
		}
		*out++ = result;
		*out++ = thisCount;
		numPages -= thisCount;
	}
	return (out-outPaSizePairs)>>1;
}

void sysMemVirtualAllocator::FreePhysical(size_t physicalAddr) {
	SYS_CS_SYNC(m_CS);
	m_PhysicalHeap.Free(physicalAddr);
}

void sysMemVirtualAllocator::FreePhysical(const size_t *paSizePairs,u32 numPairs) {
	SYS_CS_SYNC(m_CS);
	const size_t *const end = paSizePairs+(numPairs<<1);
	while (paSizePairs < end) {
		const size_t physicalAddr = *paSizePairs;
		FatalAssert(m_PhysicalHeap.GetSize(physicalAddr)*BuddyPageSize == paSizePairs[1]);
		m_PhysicalHeap.Free(physicalAddr);
		paSizePairs += 2;
	}
}

void *sysMemVirtualAllocator::AllocateVirtual(size_t size) {
	SYS_CS_SYNC(m_CS);
	FatalAssert((size & BuddyPageSizeMask) == 0);
	const u32 numPages = 1<<Log2Ceil(size/BuddyPageSize);
	const u32 vmPage = m_VirtualHeap.Allocate(numPages,MEMTYPE_DEFAULT);    // TODO: memType ???
	if (vmPage==sysTinyBuddyHeap::TERMINATOR)
		return NULL;
	m_VirtualHeap.SetUserData(vmPage,INVALID_USER_DATA);
	for (u32 i=0; i<numPages; ++i)
		m_VirtualHeap.SetPhysicalMapping(vmPage+i,sysTinyBuddyHeap::TERMINATOR);
	return GetVirtualPointer(vmPage);
}

void sysMemVirtualAllocator::FreeVirtual(void *virtualAddr) {
	if (!virtualAddr)
		return;
	SYS_CS_SYNC(m_CS);
	m_VirtualHeap.Free(GetVirtualPageIndex(virtualAddr));
}

void sysMemVirtualAllocator::MapVirtualToPhysical(void *virtualAddr,const size_t *paSizePairs,u32 numPairs,u32 memtype) {
	SYS_CS_SYNC(m_CS);

#if RSG_ORBIS
	BatchMapping *bm = AllocaBatchMapping(numPairs);
#endif

	char *va = (char*)virtualAddr;
	u32 vmPage = GetVirtualPageIndex(virtualAddr);
	const size_t *const end = paSizePairs+(numPairs<<1);
	while (paSizePairs < end) {
		const u32 pmPage   = (u32)(paSizePairs[0]);
		const u32 numPages = (u32)(paSizePairs[1]);
		for (u32 i=0; i<numPages; ++i) {
			m_VirtualHeap.SetPhysicalMapping(vmPage,pmPage+i);
#if RSG_ORBIS
			m_VirtualHeap.SetMemType(vmPage,m_PhysicalHeap.GetMemType(pmPage+i));
#else
			m_VirtualHeap.SetMemType(vmPage,memtype);
#endif
			++vmPage;
		}
		AddMapping(va,pmPage,numPages*BuddyPageSize,memtype);
		paSizePairs += 2;
		va += numPages*BuddyPageSize;
	}

#if RSG_ORBIS
	// Perform all pending mapping operations.
	DoBatchMapping(bm);

	// While the memory type and protection is really a property of the virtual
	// pages, the Orbis OS seems to associate it with the physical pages.
	// Pressumably to prevent issues from multiple virtual pages sharing the
	// same physical page but with different caching.  Subsequent mappings of
	// virtual pages inherit the type and protection from the physical page.
	//
	// But, attempting to map call sceKernelMtypeprotect (even with the same
	// type and protection) on subsequent virtual mappings returns
	// SCE_KERNEL_ERROR_BUSY (0x80020010).  As a workaround, only call
	// sceKernelMtypeprotect if reports that we don't already have what we want.
	//
	const size_t size = va-(char*)virtualAddr;
	const u32 numPages = (u32)(size/BuddyPageSize);
	const int prot = GetOrbisMemProtectionFlags(memtype);
	const int type = GetOrbisMemType(memtype);
	for (u32 i=0; i<numPages; ++i) {
		SceKernelVirtualQueryInfo info;
		ASSERT_ONLY(int ret =) sceKernelVirtualQuery(virtualAddr,0,&info,sizeof(info));
		FatalAssert(ret == SCE_OK);
		FatalAssert(info.isDirectMemory);
		if (info.protection!=prot || info.memoryType!=type) {
			SetMemTypeKeepContents(virtualAddr,size,memtype);
			break;
		}
	}

#endif
}

void sysMemVirtualAllocator::UnmapVirtual(void *virtualAddr,size_t size) {
	SYS_CS_SYNC(m_CS);
 	FatalAssert(((uptr)virtualAddr&BuddyPageSizeMask) == 0);
	FatalAssert((size&BuddyPageSizeMask) == 0);
#if RSG_ORBIS
	const int ret = sceKernelMunmap(virtualAddr, size);
	if (ret != SCE_OK)
		Quitf("Failed to unmap %p size 0x%lx, error=0x%08x", virtualAddr, size, ret);
#else
	if (!VirtualFreeTracked(virtualAddr, size, MEM_DECOMMIT))
		Quitf("Failed to unmap %p size 0x%llx", virtualAddr, size);
#endif
	const u32 vmPage = GetVirtualPageIndex(virtualAddr);
	const u32 numPages = (u32)(size/BuddyPageSize);
	for (u32 i=0; i<numPages; ++i)
		m_VirtualHeap.SetPhysicalMapping(vmPage+i,sysTinyBuddyHeap::TERMINATOR);
}

u32 sysMemVirtualAllocator::GetPhysicalAddressSizePairs(size_t *outPaSizePairs,const void *virtualAddr,size_t size) {
	SYS_CS_SYNC(m_CS);
	FatalAssert(!IsPooled(virtualAddr));
	u32 vmPage = GetVirtualPageIndex(virtualAddr);
	const u32 end = vmPage+(size+BuddyPageSizeMask)/BuddyPageSize;
	size_t *out = outPaSizePairs;
	while (vmPage < end) {
		const u32 pmPage = m_VirtualHeap.GetPhysicalMapping(vmPage);
		u32 i;
		for (i=1; i<end-vmPage; ++i)
			if (pmPage+i != m_VirtualHeap.GetPhysicalMapping(vmPage+i))
				break;
		*out++ = pmPage;
		*out++ = i;
		vmPage += i;
	}
	return (out-outPaSizePairs)>>1;
}

void *sysMemVirtualAllocator::AllocateInternal(size_t size, u32 memtype ORBIS_ONLY(, BatchMapping *bm)) {
	size_t thisSize = ComputeAllocationSize(size);
	if (thisSize < BuddyPageSize) {
		u32 list = Log2Floor(thisSize / PooledBuddyPageSize);
		if (!m_FreeBySize[list]) {
			// Need to add a new page
			u32 newPage = m_PhysicalHeap.Allocate(1,MEMTYPE_DEFAULT);
			if (newPage == sysTinyBuddyHeap::TERMINATOR)
				Quitf("Internal error, out of small pages");
			// Insert into linked list of pages this size
			m_PhysicalHeap.SetPooledNodeNext(newPage,m_FirstBySize[list]);
			m_FirstBySize[list] = newPage;
			// Remember we have new pages available now
			m_FreeBySize[list] = sm_PagesBySize[list];
			m_PhysicalHeap.SetPooledNodeFreeMask(newPage,sm_FullBits[list]);
			m_PhysicalHeap.SetPooledNodeAllocHeight(newPage,list);
			// Small pages must always be set to MEMTYPE_DEFAULT
#if RSG_DURANGO
			m_PhysicalHeap.SetMemType(newPage,MEMTYPE_DEFAULT);
#endif
			AddMapping((char*)m_VirtualBase + newPage * BuddyPageSize,newPage,BuddyPageSize,MEMTYPE_DEFAULT);
			// Displayf("Adding new mapping at %p (%u)",(char*)m_VirtualBase + newPage * BuddyPageSize,newPage);
		}
		// If we made it to here we're guaranteed to find something.
		--m_FreeBySize[list];
		u32 j = m_FirstBySize[list];
		for (;;) {
			FatalAssert(j != sysTinyBuddyHeap::TERMINATOR);
			u32 freeBitMask = m_PhysicalHeap.GetPooledNodeFreeMask(j);
			if (freeBitMask != 0) {
				u32 allocBitMask = freeBitMask&-(s32)freeBitMask;
				u32 allocBitIdx = Log2Floor(allocBitMask);
				m_PhysicalHeap.SetPooledNodeFreeMask(j,freeBitMask^allocBitMask);
				m_PoolUserData[(j<<POOLSHIFT)+allocBitIdx] = INVALID_USER_DATA;

				void* ptr = (char*)m_VirtualBase + j * BuddyPageSize + allocBitIdx * PooledBuddyPageSize;

#if RAGE_TRACKING && MEMORY_TRACKER
				if (ptr && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
				{		
					sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
					pTracker->Allocate(ptr, thisSize, 0);
				}
#endif
				return ptr;
			}
			else
				j = m_PhysicalHeap.GetPooledNodeNext(j);
		}
	}
	else {
		u32 thisCount = (u32)(thisSize / BuddyPageSize);
		FatalAssert(thisCount);
		size_t remain = thisCount;
		u32 vmPage = m_VirtualHeap.Allocate(thisCount,MEMTYPE_DEFAULT);
		if (vmPage == sysTinyBuddyHeap::TERMINATOR)	// TODO: We can recover from this...
			Quitf("Virtual address space exhausted, this shouldn't happen?");
		m_VirtualHeap.SetUserData(vmPage,INVALID_USER_DATA);
		void *base = GetVirtualPointer(vmPage);
		void *const ret = base;
		while (remain) {
			u32 result = m_PhysicalHeap.Allocate(thisCount,memtype);
			if (result != sysTinyBuddyHeap::TERMINATOR) {
				FatalAssert(thisCount);
				for (u32 i=0; i<thisCount; i++) {
					m_VirtualHeap.SetPhysicalMapping(vmPage,result+i);
#if RSG_DURANGO
					m_VirtualHeap.SetMemType(vmPage,memtype);
#elif RSG_ORBIS
					m_VirtualHeap.SetMemType(vmPage,m_PhysicalHeap.GetMemType(result+i));
#endif
					++vmPage;
				}
				AddMapping(base,result,thisCount * BuddyPageSize,memtype);
				remain -= thisCount;
				base = (char*)base + thisCount * BuddyPageSize;
			}
			else {
				thisCount >>= 1;
				if (!thisCount)
					Quitf("Internal error, out of large pages.");
			}
		}

#if RAGE_TRACKING && MEMORY_TRACKER
		if (ret && diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
		{		
			sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
			pTracker->Allocate(ret, thisSize, 0);
		}
#endif
		return ret;
	}
}


class sysMemVirtualAllocator::PageCounter {
public:

	PageCounter() {
#if RSG_ORBIS
		maxBme = 0;
#endif
		totalLarge = 0;
		for (u32 i=0; i<POOLSHIFT; i++)
			totalSmall[i] = 0;
	}

	void Add(size_t size) {
		size_t thisSize = ComputeAllocationSize(size);
		if (thisSize >= BuddyPageSize)
			totalLarge += thisSize / BuddyPageSize;
		else
			totalSmall[Log2Floor(thisSize/PooledBuddyPageSize)]++;
#if RSG_ORBIS
		maxBme += (thisSize + BuddyPageSizeMask) / BuddyPageSize;
#endif
	}

	size_t GetTotal(const u32 *freeBySize) const {
		size_t totalPages = totalLarge;
		for (u32 i=0; i<POOLSHIFT; i++) {
			// If we don't have enough small pages free, subtract however many we do have free, round up to next multiple, and convert to large pages.
			if (totalSmall[i] > freeBySize[i])
				totalPages += ((totalSmall[i]-freeBySize[i]) + ((1<<(POOLSHIFT-i))-1)) >> (POOLSHIFT-i);
		}
		return totalPages;
	}

#if RSG_ORBIS
	int GetMaxBatchMapEntries() const {
		return maxBme;
	}
#endif

private:

#if RSG_ORBIS
	int maxBme;
#endif
	size_t totalLarge;
	atRangeArray<size_t,POOLSHIFT> totalSmall;
};


void* sysMemVirtualAllocator::AllocateMemtype(size_t size,size_t /*align*/,u32 memtype) {
	SYS_CS_SYNC(m_CS);
	size = ComputeAllocationSize(size);
	PageCounter pc;
	pc.Add(size);
	size_t totalPages = pc.GetTotal(&m_FreeBySize[0]);
	if (m_PhysicalHeap.GetNodesUsed() + totalPages > m_PhysicalPageCount)
		return NULL;
#if RSG_ORBIS
	int maxBme = pc.GetMaxBatchMapEntries();    // AllocaBatchMapping is a macro and evaluates argument multiple times, .: must be stored in a temporary
	BatchMapping *bm = AllocaBatchMapping(maxBme);
#endif

	void *ret = AllocateInternal(size,memtype ORBIS_ONLY(,bm));
	FatalAssert(ret); // currently assumes virtual address space doesn't fragment enough for this to fail, but is that a safe assumption!?

#if RSG_ORBIS
	// Perform all pending mapping operations.
	DoBatchMapping(bm);

	// Only Orbis needs to set the memory type as a separate step from setting
	// up the virtual->physical mapping.
	if (size < BuddyPageSize)
		SetSmallPageDefaultMemType(ret);
	else
		SetMemTypeDiscardContents(ret,size,memtype);
#endif

	return ret;
}


bool sysMemVirtualAllocator::AllocateMap(datResourceMap &map) {
	SYS_CS_SYNC(m_CS);
	int count = map.VirtualCount + map.PhysicalCount;
	PageCounter pc;
	for (int i=0; i<count; i++) {
		pc.Add(map.Chunks[i].Size);
		// Make sure destination addresses are clear in case the allocation fails and we need to clean up.
		map.Chunks[i].DestAddr = NULL;
	}
	size_t totalPages = pc.GetTotal(&m_FreeBySize[0]);
	if (m_PhysicalHeap.GetNodesUsed() + totalPages > m_PhysicalPageCount)
		return false;
#if RSG_ORBIS
	int maxBme = pc.GetMaxBatchMapEntries();    // AllocaBatchMapping is a macro and evaluates argument multiple times, .: must be stored in a temporary
	BatchMapping *bm = AllocaBatchMapping(maxBme);
#endif

	for (int i=0; i<count; i++) {
		map.Chunks[i].DestAddr = AllocateInternal(map.Chunks[i].Size,MEMTYPE_DEFAULT ORBIS_ONLY(,bm));
		FatalAssert(map.Chunks[i].DestAddr); // currently assumes virtual address space doesn't fragment enough for this to fail, but is that a safe assumption!?
	}

#if RSG_ORBIS
	// Perform all pending mapping operations.
	DoBatchMapping(bm);

	// Only Orbis needs to set the memory type as a separate step from setting
	// up the virtual->physical mapping.
	for (int i=0; i<count; i++) {
		void *mem = map.Chunks[i].DestAddr;
		size_t size = ComputeAllocationSize(map.Chunks[i].Size);
		if (size < BuddyPageSize)
			SetSmallPageDefaultMemType(mem);
		else
			SetMemTypeDiscardContents(mem,size,MEMTYPE_DEFAULT);
	}
#endif

	map.VirtualBase = map.Chunks[map.RootVirtualChunk].DestAddr;

	return true;
}

void sysMemVirtualAllocator::FreeInternal(void *ptr, size_t size ORBIS_ONLY(, BatchMapping *bm)) {
#if RAGE_TRACKING && MEMORY_TRACKER
	if (diagTracker::GetCurrent() && sysMemVisualize::GetInstance().HasXTL())
	{		
		sysMemSystemTracker* pTracker = sysMemManager::GetInstance().GetSystemTracker();
		pTracker->Free(ptr);
	}
#endif

	if (size < BuddyPageSize) {
		u32 list = Log2Floor(size / PooledBuddyPageSize);
		u32 idx = GetVirtualPoolIndex(ptr);
		m_FreeBySize[list]++;
		u32 subIdx = idx & ((1<<POOLSHIFT)-1);
		idx >>= POOLSHIFT;
		// Displayf("Freeing height %u page at %u/%u (0x%x) @ %p",list,idx,subIdx,0x80 >> subIdx,map.Chunks[i].DestAddr);
		u32 mask = 1 << subIdx;
		u32 freeBits = m_PhysicalHeap.GetPooledNodeFreeMask(idx);
		FatalAssert(!(freeBits & mask));
		freeBits |= mask;
		m_PhysicalHeap.SetPooledNodeFreeMask(idx,freeBits);
		if (freeBits == sm_FullBits[list]) {
			// Remove the pool from the appropriate list
			u32 j = m_FirstBySize[list];
			if (j == idx)
				m_FirstBySize[list] = m_PhysicalHeap.GetPooledNodeNext(idx);
			else {
				for (;;) {
					u32 k = m_PhysicalHeap.GetPooledNodeNext(j);
					if (k == idx)
						break;
					j = k;
				}
				m_PhysicalHeap.SetPooledNodeNext(j,m_PhysicalHeap.GetPooledNodeNext(idx));
			}
			m_PhysicalHeap.Free(idx);
			// We just removed several free pages, adjust the counter
			FatalAssert(m_FreeBySize[list] >= sm_PagesBySize[list]);
			m_FreeBySize[list] -= sm_PagesBySize[list];
			// ...unmap the page
			// Displayf("Unmapping freed page at %p (%u)",(char*)m_VirtualBase + idx * BuddyPageSize,idx);
			AddUnmapping((char*)m_VirtualBase + idx * BuddyPageSize,BuddyPageSize);
		}
	}
	else {
		u32 vmPage = GetVirtualPageIndex(ptr);
		u32 memtype = m_VirtualHeap.GetMemType(vmPage);
#ifdef MEMTYPES_THAT_MUST_BE_REVERTED
		if ((1<<memtype) & MEMTYPES_THAT_MUST_BE_REVERTED) {
			SetMemTypeDiscardContents(ptr,size,MEMTYPE_DEFAULT);
			memtype = MEMTYPE_DEFAULT;
		}
#endif
		u32 v = vmPage;
		u32 vend = v+size/BuddyPageSize;
		while (v < vend) {
			u32 p = m_VirtualHeap.GetPhysicalMapping(v);
			if (p == sysTinyBuddyHeap::TERMINATOR) {
				++v;
				ptr = (char*)ptr+BuddyPageSize;
			}
			else {
				FatalAssert(m_PhysicalHeap.GetFirstNode(p) == p);
				u32 numPages = m_PhysicalHeap.GetSize(p);
#if __ASSERT
				FatalAssert(numPages <= vend-v);
				u32 i;
				for (i=1; i<numPages; ++i)
					if (p+i != m_VirtualHeap.GetPhysicalMapping(v+i))
						break;
				FatalAssert(numPages == i);
#endif
				m_PhysicalHeap.SetMemType(p,memtype);
				m_PhysicalHeap.Free(p);
				u32 psize = numPages*BuddyPageSize;
				AddUnmapping(ptr,psize);
				v += numPages;
				ptr = (char*)ptr+psize;
			}
		}
		m_VirtualHeap.Free(vmPage);
	}
	// Displayf("Freeing known pointer [%p,%p]",p->first,(char*)p->first + p->second.size - 1);
}

void sysMemVirtualAllocator::Free(const void *ptr) {
	SYS_CS_SYNC(m_CS);
	FatalAssert(ptr == GetCanonicalBlockPtr(ptr));
	size_t size = GetSize(ptr);
#if RSG_ORBIS
	int maxBme = (size + BuddyPageSizeMask) / BuddyPageSize;
	BatchMapping *bm = AllocaBatchMapping(maxBme);
	bm->num = 0;
	ASSERT_ONLY(bm->maxNum = maxBme;)
#endif
	FreeInternal(const_cast<void*>(ptr),size ORBIS_ONLY(,bm));
#if RSG_ORBIS
	// Hopefully this is faster than doing it all separately.
	DoBatchMapping(bm);
#endif
}

void sysMemVirtualAllocator::FreeMap(const datResourceMap &map) {
	SYS_CS_SYNC(m_CS);
	int count = map.VirtualCount + map.PhysicalCount;

#if RSG_ORBIS
	int maxBme = 0;
	for (int i=0; i<count; i++)
		maxBme += (ComputeAllocationSize(map.Chunks[i].Size) + BuddyPageSizeMask) / BuddyPageSize;
	BatchMapping *bm = AllocaBatchMapping(maxBme);
	bm->num = 0;
	ASSERT_ONLY(bm->maxNum = maxBme;)
#endif

	for (int i=0; i<count; i++) {
		void *ptr = map.Chunks[i].DestAddr;
		FreeInternal(ptr,GetSize(ptr) ORBIS_ONLY(,bm));
	}

#if RSG_ORBIS
	// Hopefully this is faster than doing it all separately.
	DoBatchMapping(bm);
#endif
}

#endif	// !ENABLE_BUDDY_ALLOCATOR

}	// namespace rage

#endif	// RSG_ORBIS || RSG_DURANGO
