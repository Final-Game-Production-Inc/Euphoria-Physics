#ifndef VIRTUALMEMORY_PSN_H
#define VIRTUALMEMORY_PSN_H
#if __PS3
// PS3 Virtual Memory Helper functions

#include <sys/memory.h>
#include <sys/vm.h>

#define MB_TO_BYTES(x)	((x)*1024*1024)
#define BYTES_TO_MB(x)	((x)/(1024*1024))

namespace rage
{
	class VirtualMemHeap
	{
		public:
			VirtualMemHeap();
			~VirtualMemHeap();
			// Allocate virtual memory
			void *AllocateMemory(size_t mappedMemorySize, size_t actualMemorySize);

			// Allocate virtual memory but if the requested amount of real memory is not available
			// then try to allocate less. minRemaining is the minumum MB that should be left free after
			// the allocation
			void *AllocateMemoryBackedByAvailable(size_t mappedMemorySize, size_t actualMemorySize, int minRemainingMB);

			// Free Memory
			void FreeMemory();

			// Get the pointer to the virtual memory			
			sys_addr_t	GetHeapAddr()			{ return m_vmPtr; }
			void*		GetHeapPtr()			{ return (void*) m_vmPtr; }
			void		SetHeapPtr(void *ptr)	{ m_vmPtr = (sys_addr_t)ptr; }

			void UpdateStats();
			void StatsEndFrame();

			size_t	GetVirtualSize()	{ return m_virtualMemorySize; }
			size_t	GetPhysicalSize()	{ return m_realMemorySize; }
#if !__FINAL
			uint64_t	GetNumPageFaultPPU()	{ return statCurrentPageFaultPPU; }
			uint64_t	GetNumPageFaultSPU()	{ return statCurrentPageFaultSPU; }
			uint64_t	GetNumPageIn()			{ return statCurrentPageIn; }
			uint64_t	GetNumPageOut()			{ return statCurrentPageOut; }
#endif // !__FINAL
		private:
			sys_addr_t m_vmPtr;
			size_t m_virtualMemorySize;
			size_t m_realMemorySize;
#if !__FINAL
			// stats for this frame (for profiling purposes)
			uint64_t statCurrentPageFaultPPU;
			uint64_t statCurrentPageFaultSPU;
			uint64_t statCurrentPageIn;
			uint64_t statCurrentPageOut;
#endif	// !__FINAL
	};

#if !__FINAL
	namespace VirtualMemoryStats
	{
		// Updates the running virtual memory stats with the stats from this heap. This can be called multiple
		// times per frame if you want to accumulate totals over several heaps
		void UpdateVirtualMemoryProfileStats(VirtualMemHeap *vmHeap);

		// Call this when you want to reset the counts for this frame
		void VirtualMemoryProfileEndFrame();

		// Total number of page faults (tracked per mission)
		int GetTotalNumberPageFaults();
	}
#endif // !__FINAL
};

#endif // __PS3
#endif // VIRTUALMEMORY_PSN_H
