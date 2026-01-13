
#if __PS3

#include "virtualmemory_psn.h"
#include "profile/group.h"
#include "profile/page.h"


#include <stdio.h>

namespace rage
{

VirtualMemHeap::VirtualMemHeap()
{
	m_vmPtr = 0;
	m_virtualMemorySize = 0;
	m_realMemorySize = 0;
}

VirtualMemHeap::~VirtualMemHeap()
{
	if(GetHeapPtr())
	{
		FreeMemory();
	}
}

// Allocate virtual memory
void *VirtualMemHeap::AllocateMemory(size_t mappedMemorySize, size_t actualMemorySize)
{
	// Sizes must be in MB
	Assertf(MB_TO_BYTES(BYTES_TO_MB(mappedMemorySize)) == mappedMemorySize, "Memory size must be a multiple of 1MB");
	Assertf(MB_TO_BYTES(BYTES_TO_MB(actualMemorySize)) == actualMemorySize, "Memory size must be a multiple of 1MB");

	// Mapped size must be a multiple of 32 MB
	Assertf((BYTES_TO_MB(mappedMemorySize) % 32) == 0, "Mapped memory size must be a multiple of 32MB");

	int ret = sys_vm_memory_map( mappedMemorySize, actualMemorySize, SYS_MEMORY_CONTAINER_ID_INVALID,
		SYS_MEMORY_PAGE_SIZE_64K, SYS_VM_POLICY_AUTO_RECOMMENDED, &m_vmPtr );

	if ( Verifyf(ret == CELL_OK, "There was a problem allocating the virtual memory [0x%x]", ret) )
	{
		m_virtualMemorySize = mappedMemorySize;
		m_realMemorySize = actualMemorySize;
	}
	return (void*)m_vmPtr;
}

void *VirtualMemHeap::AllocateMemoryBackedByAvailable(size_t mappedMemorySize, size_t actualMemorySize, int minRemainingMB)
{
	Assertf(MB_TO_BYTES(BYTES_TO_MB(mappedMemorySize)) == mappedMemorySize, "Memory size must be a multiple of 1MB");
	Assertf(MB_TO_BYTES(BYTES_TO_MB(actualMemorySize)) == actualMemorySize, "Memory size must be a multiple of 1MB");

	// Find out how much memory is available and round down to the nearest MB
	sys_memory_info_t	meminfo;
	int ret = sys_memory_get_user_memory_size(&meminfo);
	int availableMB = BYTES_TO_MB(meminfo.available_user_memory);
	int actualMemorySizeMB = BYTES_TO_MB(actualMemorySize);

	if (availableMB >= (actualMemorySizeMB + minRemainingMB))
	{
		// We have enough memory, allocate the full amount
		return AllocateMemory(mappedMemorySize, actualMemorySize);
	}
	else
	{
		// Not enough to allocate the full amount, can we allocate anything?
		int usableSizeMB = availableMB - minRemainingMB;
		if (usableSizeMB > 0)
			return AllocateMemory(mappedMemorySize, MB_TO_BYTES(usableSizeMB));
	}	

	// There is no memory available...
	return NULL;
}

// Free Memory
void VirtualMemHeap::FreeMemory()
{
	Assertf(GetHeapPtr() != NULL, "This virtual memory heap is not allocated!" );
	ASSERT_ONLY(int ret =) sys_vm_unmap(m_vmPtr);
	Assertf(ret == CELL_OK, "There was a problem deallocating the virtual memory [0x%x]", ret);
	m_vmPtr = 0;
	m_virtualMemorySize = 0;
	m_realMemorySize = 0;
}

void VirtualMemHeap::UpdateStats()
{
#if !__FINAL
	Assertf(GetHeapPtr() != NULL, "This virtual memory heap is not allocated!" );
	sys_vm_statistics_t stats;
	int ret = sys_vm_get_statistics(m_vmPtr, &stats);
	if (Verifyf(ret == CELL_OK, "There was a problem getting virtual memory stats!"))
	{
		statCurrentPageFaultPPU = stats.page_fault_ppu;
		statCurrentPageFaultSPU = stats.page_fault_spu;
		statCurrentPageIn = stats.page_in;
		statCurrentPageOut = stats.page_out;
	}
#endif // !__FINAL
}

void VirtualMemHeap::StatsEndFrame()
{
	// Not necessary anymore
}

#if __STATS
namespace VirtualMemoryStats
{
	PF_PAGE(vmStatsPage,"Virtual Memory Stats");
	PF_GROUP(vmStatsGeneric);
	PF_LINK(vmStatsPage, vmStatsGeneric); 

	PF_VALUE_INT(vmStats_PageFaultPPU,	vmStatsGeneric);	// Number of times a PPU thread caused a page fault
	PF_VALUE_INT(vmStats_PageFaultSPU,	vmStatsGeneric);	// Number of times an SPU thread or raw spu caused a page fault
	PF_VALUE_INT(vmStats_PageIn,		vmStatsGeneric);	// Number of page-ins (reads from the backing store)
	PF_VALUE_INT(vmStats_PageOut,		vmStatsGeneric);	// Number of page-outs (writes to the backing store)

	//PF_GROUP(vmStatsUsage);
	//PF_LINK(vmStatsPage, vmStatsUsage);
	//PF_VALUE_INT(vmStats_PhysicalMemUsed, vmStatsUsage);	// Size of physical memory currently used by the virtual memory area
};
#endif // __STATS
#if !__FINAL
using namespace VirtualMemoryStats;
// Running stats for this frame
static uint64_t statCurrentPageFaultPPU = 0;
static uint64_t statCurrentPageFaultSPU = 0;
static uint64_t statCurrentPageIn = 0;
static uint64_t statCurrentPageOut = 0;
static bool		statResetFrame = true;
#endif // !__FINAL

#if !__FINAL
namespace VirtualMemoryStats 
{
void UpdateVirtualMemoryProfileStats(VirtualMemHeap* vmHeap)
{
	if (statResetFrame)
	{
		statResetFrame = false;
		statCurrentPageFaultPPU = 0;
		statCurrentPageFaultSPU = 0;
		statCurrentPageIn = 0;
		statCurrentPageOut = 0;
	}

	// Update the running totals
	statCurrentPageFaultPPU += vmHeap->GetNumPageFaultPPU();
	statCurrentPageFaultSPU += vmHeap->GetNumPageFaultSPU();
	statCurrentPageIn += vmHeap->GetNumPageIn();
	statCurrentPageOut += vmHeap->GetNumPageOut();	
#if __STATS
		PF_SET(vmStats_PageFaultPPU, statCurrentPageFaultPPU);
		PF_SET(vmStats_PageFaultSPU, statCurrentPageFaultSPU);
		PF_SET(vmStats_PageIn, statCurrentPageIn);
		PF_SET(vmStats_PageOut, statCurrentPageOut);
		//PF_SET(vmStats_PhysicalMemUsed, stats.pmem_used);
#endif // __STATS
}

void VirtualMemoryProfileEndFrame()
{
	statResetFrame = true;
}

int GetTotalNumberPageFaults()
{
	return statCurrentPageFaultPPU + statCurrentPageFaultSPU;
}
} // namespace VirtualMemoryStats
#endif // !__FINAL

} // namespace rage

#endif // __PS3
