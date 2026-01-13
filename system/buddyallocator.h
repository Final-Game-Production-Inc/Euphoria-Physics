// 
// system/buddyallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_BUDDYALLOCATOR_H 
#define SYSTEM_BUDDYALLOCATOR_H 

#include "system/memory.h"
#include "system/sparseallocator.h"

// Round workspace size up to 4k boundary so the heap itself is guaranteed to start on a 4k boundary as well.
// The size needs to be sizeof(sysBuddyHeap::NodeInfo) + sizeof(u8) per node.
#if __WIN32PC || RSG_DURANGO || RSG_ORBIS
#define COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(x)	((((x)*13)+4095)&~4095)
#else
#define COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(x)	((((x)*9)+4095)&~4095)
#endif

#if ENABLE_BUDDY_ALLOCATOR

#include "system/buddyheap.h"

namespace rage {

/*
	This class bridges sysExternalHeap and our memory classes to
	that it can be used as a normal low-overhead (albeit slower)
	game heap.  All memory it uses is owned by its creator.
*/
class sysMemBuddyAllocator: public sysMemAllocator 
{
public:
	// PURPOSE:	Constructor
	// PARAMS:	heapBase - Base address of the heap (never actually referenced, but this
	//				is the base address Allocate and Free are relative to)
	//			leafSize - Size of a leaf node
	//			maxPointers - Maximum number of memory nodes to reserve (<65535)
	//			workspace - pointer to COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(maxPointers) bytes of storage
	sysMemBuddyAllocator();
	sysMemBuddyAllocator(void *heapBase,size_t leafSize,size_t maxPointers,void *workspace);

	// PURPOSE:	Destructor
	~sysMemBuddyAllocator();
	
#if RESOURCE_POOLS
	void HideUsedMemory(void* ptr);
	void InitPool(int pos, void* ptr, size_t capacity, size_t size);
	inline sysBuddyPool* GetPool(int pos) {return m_Heap.GetPool(pos);}
#endif

	WIN32PC_ONLY(virtual) void* Allocate(size_t size,size_t align,int heapIndex);

	WIN32PC_ONLY(virtual) void* TryAllocate(size_t size,size_t align,int heapIndex);

	WIN32PC_ONLY(virtual) void Free(const void *ptr);

	WIN32PC_ONLY(virtual) size_t GetMemoryUsed(int);

	WIN32PC_ONLY(virtual) size_t GetHeapSize(void) const { return m_Size; }

	WIN32PC_ONLY(virtual) size_t GetMemoryAvailable();

	WIN32PC_ONLY(virtual) size_t GetLargestAvailableBlock();

	RAGE_MEMORY_DEBUG_ONLY(size_t GetHighWaterMark(bool reset);)

	virtual bool GetMemoryDistribution(sysMemDistribution & /*outDist*/);

	virtual bool IsValidPointer(const void * ptr) const;

	virtual bool Defragment(sysMemDefragmentation& /*outDefrag*/, sysMemDefragmentationFree& /*outDefragFree*/, size_t maxSize);

	// RETURNS:	Reference to underlying sysExternalHeap object.
	WIN32PC_ONLY(virtual) const sysBuddyHeap& GetHeap() const { return m_Heap; }

	// RETURNS: Heap base
	WIN32PC_ONLY(virtual) void *GetHeapBase() const { return m_HeapBase; }

	WIN32PC_ONLY(virtual) void SetHeapBase(void *hb) { m_HeapBase=hb; }

	// RETURNS: Workspace
	WIN32PC_ONLY(virtual) void *GetWorkspace() const { return m_Workspace; }

	virtual size_t GetSize(const void *ptr) const;

	virtual void SanityCheck();

	WIN32PC_ONLY(virtual) const void *GetCanonicalBlockPtr(const void *ptr) const;

	WIN32PC_ONLY(virtual) bool TryLockBlock(const void *, unsigned);

	WIN32PC_ONLY(virtual) void UnlockBlock(const void *, unsigned);

	WIN32PC_ONLY(virtual) u32 GetUserData(const void *) const;

	WIN32PC_ONLY(virtual) void SetUserData(const void *,u32);

	WIN32PC_ONLY(virtual) int GetBlockLockCount(const void *);

	WIN32PC_ONLY(virtual) size_t GetFragmentation();

	WIN32PC_ONLY(virtual) size_t GetLeafSize(void) const { return m_LeafSize; }

	WIN32PC_ONLY(virtual) sysBuddyNodeIdx GetNodeIdxFromPtr(void* ptr) const
	{
		Assert(ptr >= m_HeapBase && ptr < (char*)m_HeapBase + m_Size);
		return (sysBuddyNodeIdx)(((char*)ptr-(char*)m_HeapBase) / m_LeafSize);
	}

	WIN32PC_ONLY(virtual) void* GetPtrFromNodeIdx(sysBuddyNodeIdx index) const
	{
		Assert(index < m_Size / m_LeafSize);
		return (char*)(m_HeapBase) + index * m_LeafSize;
	};
	
	static inline size_t CeilLeafShift(size_t value, size_t leafSize) 
	{
		size_t result = 0;
		while (value > leafSize) {
			leafSize <<= 1;
			++result;
		}
		return result;
	}

	static inline size_t ComputeNodeSize(size_t value, size_t leafSize) 
	{
		Assert(value != 0xFFFFFFFF);
		while (value > leafSize) {
			leafSize <<= 1;
		}
		return leafSize;
	}

private:
	void			*m_HeapBase;
	void			*m_Workspace;
	size_t			m_Size;
	size_t			m_LeafSize;
	sysBuddyHeap	m_Heap;

	RAGE_MEMORY_DEBUG_ONLY(size_t m_watermark;)
};

#if RSG_PC

class sysMemGrowBuddyAllocator: public sysMemBuddyAllocator
{
public:
	sysMemGrowBuddyAllocator(size_t leafSize, size_t maxSize RAGE_TRACKING_ONLY(, const char* allocatorName));
	~sysMemGrowBuddyAllocator(); /*{ }*/

	void* Allocate(size_t size,size_t align,int heapIndex);
	void* TryAllocate(size_t size,size_t align,int heapIndex);

	void Free(const void *ptr);
	size_t GetMemoryUsed(int bucket);
	size_t GetHeapSize(void) const;
	size_t GetMemoryAvailable();
	size_t GetLargestAvailableBlock();

	virtual bool GetMemoryDistribution(sysMemDistribution & outDist);
	virtual bool IsValidPointer(const void * ptr) const; 

	bool Defragment(sysMemDefragmentation& /*outDefrag*/, sysMemDefragmentationFree& /*outDefragFree*/, size_t /*maxSize*/);
	void *GetHeapBase() const { return m_aHeaps[0].GetHeapBase(); }
	void SetHeapBase(void *) { Assertf(0, "Can not use this function"); }
	void *GetWorkspace() const { Assertf(0, "Can not use this function"); return NULL; }

	virtual size_t GetSize(const void *ptr) const;


	void SanityCheck();

	const void *GetCanonicalBlockPtr(const void *ptr) const;

	bool TryLockBlock(const void *, unsigned);

	void UnlockBlock(const void *, unsigned);

	u32 GetUserData(const void *) const;

	void SetUserData(const void *,u32);

	int GetBlockLockCount(const void *);

	size_t GetFragmentation();

	size_t GetLeafSize(void) const { return m_LeafSize; }

	sysBuddyNodeIdx GetNodeIdxFromPtr(void* ptr) const;
	void* GetPtrFromNodeIdx(sysBuddyNodeIdx index) const;

	RAGE_TRACKING_ONLY(void TrackExistingPages(diagTracker&) const;)

	static const u32 sm_MaxHeaps = 32;
	static const u32 PagesPerBlock = 32768;
	static const size_t sm_DefaultHeapSize = 256 * 1024 * 1024;
private:
	s32 GrowHeap();

	size_t			m_Size;
	size_t			m_LeafSize;
	s32				m_CurrentIndexToAllocateFrom;
	s32				m_NumOfHeaps;

#if RAGE_TRACKING
	char			m_Name[48];
#endif	

	sysMemBuddyAllocator	m_aHeaps[sm_MaxHeaps];
	void*					m_apWorkspace[sm_MaxHeaps];
	void*					m_apvHeapSpace[sm_MaxHeaps];
};

#define COMPUTE_SPARSEBUDDYHEAP_WORKSPACE_SIZE(x)		((x)*sizeof(rage::sysMemSparseBuddyAllocator::BuddyData))

class sysMemSparseBuddyAllocator: public sysMemBuddyAllocator
{
public:
	struct BuddyData 
	{
		void *memory;
		enum {
			BITS_LOCKCOUNT  = 8,
			BITS_USERDATA   = 24,
		};
		u32 lockCount:BITS_LOCKCOUNT, userData:BITS_USERDATA;
		u32 size;
	};
	sysMemSparseBuddyAllocator(size_t heapSize,size_t leafSize,size_t leafCount,void *workspace);
	~sysMemSparseBuddyAllocator();

	void* Allocate(size_t size,size_t align,int heapIndex /* = 0 */);

	void* TryAllocate(size_t size,size_t align,int heapIndex /* = 0 */);

	void Free(const void *ptr);

	void DeferredFree(const void *ptr);

	size_t GetLeafSize() const { return m_LeafSize; }

	const void *GetCanonicalBlockPtr(const void *ptr) const;

	bool TryLockBlock(const void * /*block*/, unsigned);

	void UnlockBlock(const void * /*block*/, unsigned);

	int GetBlockLockCount(const void *);

	void SetUserData(const void *,u32);

	u32 GetUserData(const void *) const;

	bool Defragment(sysMemDefragmentation& /*outDefrag*/, sysMemDefragmentationFree& /*outDefragFree*/, size_t maxSize);

	bool IsValidPointer(const void * ptr) const; 

	size_t GetSize(const void *ptr) const;

	size_t GetMemoryUsed(int bucket);

	size_t GetHeapSize(void) const;

	size_t GetMemoryAvailable();

	size_t GetLargestAvailableBlock();

	sysMemSparseAllocator* GetInternalAllocator() { return &m_SparseAllocator; }

private:
	BuddyData* GetBuddyData(const void *ptr) const;

	sysMemSparseAllocator m_SparseAllocator;
	size_t m_LeafSize, m_LeafCount;
	BuddyData *m_BuddyData, *m_BuddyDataEnd, *m_BuddyDataCapacity;		// m_LeafCount of these (borrowed from workspace for consistency with original code paths)
	void *m_DeferredFree;
};

#endif // RSG_PC

} // namespace rage

#endif	// ENABLE_BUDDY_ALLOCATOR

#endif // SYSTEM_BUDDYALLOCATOR_H 
