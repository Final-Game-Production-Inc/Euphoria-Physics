
#ifndef SYSTEM_ALLOCATOR_H
#define SYSTEM_ALLOCATOR_H

#include "atl/inmap.h"
#include "atl/pool.h"
#include "system/criticalsection.h"
#include "system/memory.h"

#if MEMORY_TRACKER

namespace rage 
{
#if RSG_DURANGO || (RSG_PC && !RSG_TOOL)

#if RSG_DURANGO
	// XBone Memory Page Sizes
	// If MEM_GRAPHICS is specified, then VirtualAddress must be NULL or in the range 4 GB to 1 TB.
	#define MM_VIRTUAL_GRAPHICS_BASE    ((size_t)0x100000000)
	#define MM_VIRTUAL_GRAPHICS_END     ((size_t)0x10000000000)
	#define MM_VIRTUAL_GRAPHICS_SIZE    (MM_VIRTUAL_GRAPHICS_END - MM_VIRTUAL_GRAPHICS_BASE)

	// If MEM_GRAPHICS is not specified, VirtualAddress must be NULL or in the range 1 TB to 8 TB.
	#define MM_VIRTUAL_1TB_BASE   ((size_t)0x10000000000)
	#define MM_VIRTUAL_1TB_END    ((size_t)0x20000000000)
	#define MM_VIRTUAL_1TB_SIZE   (MM_VIRTUAL_1TB_END - MM_VIRTUAL_1TB_BASE)

	#define MM_VIRTUAL_2TB_BASE   ((size_t)0x20000000000)
	#define MM_VIRTUAL_2TB_END    ((size_t)0x30000000000)
	#define MM_VIRTUAL_2TB_SIZE   (MM_VIRTUAL_2TB_END - MM_VIRTUAL_2TB_BASE)

	#define MM_VIRTUAL_3TB_BASE   ((size_t)0x30000000000)
	#define MM_VIRTUAL_3TB_END    ((size_t)0x40000000000)
	#define MM_VIRTUAL_3TB_SIZE   (MM_VIRTUAL_3TB_END - MM_VIRTUAL_3TB_BASE)

	#define MM_VIRTUAL_4TB_BASE   ((size_t)0x40000000000)
	#define MM_VIRTUAL_4TB_END    ((size_t)0x50000000000)
	#define MM_VIRTUAL_4TB_SIZE   (MM_VIRTUAL_4TB_END - MM_VIRTUAL_4TB_BASE)

	#define MM_VIRTUAL_5TB_BASE   ((size_t)0x50000000000)
	#define MM_VIRTUAL_5TB_END    ((size_t)0x60000000000)
	#define MM_VIRTUAL_5TB_SIZE   (MM_VIRTUAL_5TB_END - MM_VIRTUAL_5TB_BASE)

	#define MM_VIRTUAL_6TB_BASE   ((size_t)0x60000000000)
	#define MM_VIRTUAL_6TB_END    ((size_t)0x70000000000)
	#define MM_VIRTUAL_6TB_SIZE   (MM_VIRTUAL_6TB_END - MM_VIRTUAL_6TB_BASE)

	#define MM_VIRTUAL_7TB_BASE   ((size_t)0x70000000000)
	#define MM_VIRTUAL_7TB_END    ((size_t)0x80000000000)
	#define MM_VIRTUAL_7TB_SIZE   (MM_VIRTUAL_7TB_END - MM_VIRTUAL_7TB_BASE)
#elif RSG_PC
	#define MM_VIRTUAL_A_BASE   ((size_t)0x00000000000)
	#define MM_VIRTUAL_A_END    ((size_t)0x02000000000)
	#define MM_VIRTUAL_A_SIZE   (MM_VIRTUAL_A_END - MM_VIRTUAL_A_BASE)

	#define MM_VIRTUAL_B_BASE   ((size_t)0x02000000000)
	#define MM_VIRTUAL_B_END    ((size_t)0x04000000000)
	#define MM_VIRTUAL_B_SIZE   (MM_VIRTUAL_B_END - MM_VIRTUAL_B_BASE)

	#define MM_VIRTUAL_C_BASE   ((size_t)0x04000000000)
	#define MM_VIRTUAL_C_END    ((size_t)0x06000000000)
	#define MM_VIRTUAL_C_SIZE   (MM_VIRTUAL_C_END - MM_VIRTUAL_C_BASE)

	#define MM_VIRTUAL_D_BASE   ((size_t)0x06000000000)
	#define MM_VIRTUAL_D_END    ((size_t)0x08000000000)
	#define MM_VIRTUAL_D_SIZE   (MM_VIRTUAL_D_END - MM_VIRTUAL_D_BASE)

	#define MM_VIRTUAL_E_BASE   ((size_t)0x08000000000)
	#define MM_VIRTUAL_E_END    ((size_t)0x0A000000000)
	#define MM_VIRTUAL_E_SIZE   (MM_VIRTUAL_E_END - MM_VIRTUAL_E_BASE)

	#define MM_VIRTUAL_F_BASE   ((size_t)0x0A000000000)
	#define MM_VIRTUAL_F_END    ((size_t)0x0C000000000)
	#define MM_VIRTUAL_F_SIZE   (MM_VIRTUAL_F_END - MM_VIRTUAL_F_BASE)

	#define MM_VIRTUAL_G_BASE   ((size_t)0x0C000000000)
	#define MM_VIRTUAL_G_END    ((size_t)0x0E000000000)
	#define MM_VIRTUAL_G_SIZE   (MM_VIRTUAL_G_END - MM_VIRTUAL_G_BASE)

	#define MM_VIRTUAL_H_BASE   ((size_t)0x0E000000000)
	#define MM_VIRTUAL_H_END    ((size_t)0x10000000000)
	#define MM_VIRTUAL_H_SIZE   (MM_VIRTUAL_H_END - MM_VIRTUAL_H_BASE)

	#define MM_VIRTUAL_REMAINDER_BASE   ((size_t)0x10000000000)
	#define MM_VIRTUAL_REMAINDER_END    ((size_t)0xFFFFFFFFFFFFFFFF)
	#define MM_VIRTUAL_REMAINDER_SIZE   (MM_VIRTUAL_REMAINDER_END - MM_VIRTUAL_REMAINDER_BASE)
#endif

	class sysMemSystemTracker final
	{
		static const int SYSTEM_ALLOCATOR_BUCKETS = 256;

#if RSG_DURANGO
		// Nodes - We have to do this shit because XMemSize doesn't exist on XBone yet!
		struct Node
		{
			Node() { }
			Node(const size_t size) { m_size = size; }

			inmap_node<void*, Node> m_ByAddr;
			size_t m_size;
		};

		typedef inmap<void*, Node, &Node::m_ByAddr> NodeMap;
		typedef atPool<Node> NodePool;
#endif

	private:
		size_t m_memoryUsed;
		size_t m_memoryUsedByID[SYSTEM_ALLOCATOR_BUCKETS];		

		size_t m_totalAllocations;
		size_t m_totalAllocationsByID[SYSTEM_ALLOCATOR_BUCKETS];

#if RSG_DURANGO
		sysCriticalSectionToken m_Token;
		NodePool m_pool;
		NodeMap m_map;
#endif

	public:
#if RSG_DURANGO
		sysMemSystemTracker(const size_t size = (1 << 20));
#elif RSG_PC
		sysMemSystemTracker();
#endif		
		size_t GetSize() const;
		size_t GetUsed(int bucket = -1) const;
		size_t GetAvailable() const;

		void Allocate(void* ptr, size_t size, int id = 0);
#if RSG_DURANGO
		void Free(void* ptr, int id = 0);
#else
		void Free(void* ptr, size_t size, int id = 0);
#endif
		void Print() const;
	};

#elif __XENON
	// Xbox 360 Memory Page Sizes
	#define MM_VIRTUAL_4KB_BASE    ((size_t)0x00000000)
	#define MM_VIRTUAL_4KB_END     ((size_t)0x3FFFFFFF)
	#define MM_VIRTUAL_4KB_SIZE    (MM_VIRTUAL_4KB_END - MM_VIRTUAL_4KB_BASE + 1)

	#define MM_VIRTUAL_64KB_BASE   ((size_t)0x40000000)
	#define MM_VIRTUAL_64KB_END    ((size_t)0x7FFFFFFF)
	#define MM_VIRTUAL_64KB_SIZE   (MM_VIRTUAL_64KB_END - MM_VIRTUAL_64KB_BASE + 1)

	#define MM_IMAGE_64KB_BASE     ((size_t)0x80000000)
	#define MM_IMAGE_64KB_END      ((size_t)0x8BFFFFFF)
	#define MM_IMAGE_64KB_SIZE     (MM_IMAGE_64KB_END - MM_IMAGE_64KB_BASE + 1)

	#define MM_ENCRYPTED_64KB_BASE ((size_t)0x8C000000)
	#define MM_ENCRYPTED_64KB_END  ((size_t)0x8DFFFFFF)
	#define MM_ENCRYPTED_64KB_SIZE (MM_ENCRYPTED_64KB_END - MM_ENCRYPTED_64KB_BASE + 1)

	#define MM_IMAGE_4KB_BASE      ((size_t)0x90000000)
	#define MM_IMAGE_4KB_END       ((size_t)0x9FFFFFFF)
	#define MM_IMAGE_4KB_SIZE	   (MM_IMAGE_4KB_END - MM_IMAGE_4KB_BASE + 1)

	#define MM_PHYSICAL_64KB_BASE  ((size_t)0xA0000000)
	#define MM_PHYSICAL_64KB_END   ((size_t)0xBFFFFFFF)
	#define MM_PHYSICAL_64KB_SIZE  (MM_PHYSICAL_64KB_END - MM_PHYSICAL_64KB_BASE + 1)

	#define MM_PHYSICAL_16MB_BASE  ((size_t)0xC0000000)
	#define MM_PHYSICAL_16MB_END   ((size_t)0xDFFFFFFF)
	#define MM_PHYSICAL_16MB_SIZE  (MM_PHYSICAL_16MB_END - MM_PHYSICAL_16MB_BASE + 1)

	#define MM_PHYSICAL_4KB_BASE   ((size_t)0xE0000000)
	#define MM_PHYSICAL_4KB_END    ((size_t)0xFFFFFFFF)
	#define MM_PHYSICAL_4KB_SIZE   (MM_PHYSICAL_4KB_END - MM_PHYSICAL_4KB_BASE + 1)

		// XDK definitions for different enums
	#define XENON_ALLOCATOR_BUCKETS_START (128)
	#define XENON_ALLOCATOR_BUCKETS (39)

	class sysMemSystemTracker : public sysMemAllocator
	{
	private:
		size_t m_memoryUsed;
		size_t m_memoryUsedByID[XENON_ALLOCATOR_BUCKETS];

	private:
		size_t GetAvailable() const;
		static int GetMemBucketType(int index) {return index + XENON_ALLOCATOR_BUCKETS_START;}
		static int GetMemBucketIndex(int type)
		{
			Assert(type >= XENON_ALLOCATOR_BUCKETS_START && type <= (XENON_ALLOCATOR_BUCKETS_START + XENON_ALLOCATOR_BUCKETS));
			return type - XENON_ALLOCATOR_BUCKETS_START;
		}

		static const char* GetMemBucketStringByType(int type);
		static const char* GetMemBucketStringByIndex(int index)
		{
			Assert(index >= 0 && index < XENON_ALLOCATOR_BUCKETS);
			const int type = GetMemBucketType(index);
			return GetMemBucketStringByType(type);
		}

	public:
		sysMemSystemTracker();

		SYS_MEM_VIRTUAL void* Allocate(size_t size,size_t align, int heapIndex);
		SYS_MEM_VIRTUAL void Free(const void *ptr);

		SYS_MEM_VIRTUAL void* GetHeapBase() const;
		SYS_MEM_VIRTUAL size_t GetHeapSize() const;

		SYS_MEM_VIRTUAL size_t GetMemoryUsed(int bucket);
		SYS_MEM_VIRTUAL size_t GetMemoryAvailable();

		void TrackMemAlloc(size_t size, int allocId);
		void TrackMemFree(size_t size, int allocId);
		void PrintMemBuckets() const;
	};
#endif

} // namespace rage

#endif // MEMORY_TRACKER

#endif // SYSTEM_ALLOCATOR_H
