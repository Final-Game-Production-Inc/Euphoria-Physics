// 
// system/simpleallocator.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 
#ifndef SYSTEM_SIMPLEALLOCATOR_H
#define SYSTEM_SIMPLEALLOCATOR_H

#define ENABLE_SMALLOCATOR_CODE	(!__SPU)

#include "system/memory.h"
#include "diag/tracker.h"
#include "system/criticalsection.h"

#if ENABLE_SMALLOCATOR_CODE
#include "system/smallocator.h"
#endif
#include "atl/array.h"
#include "atl/bitset.h"

namespace rage {

class fiStream;

/*
	This simple allocation class currently retrieves its storage via malloc.
	Memory allocation is O(N) in the number of free nodes, with an early exit
	on an exact size match.  Freed nodes are placed at the beginning of the
	free list and are immediately coalesced with neighbors.  Freeing memory
	is O(1).  All memory blocks	are sixteen-byte-aligned and have a sixteen byte 
	header before them.  Newly allocated memory is always initialized via
	debug_memory_fill (to 0xCD by default).  Freed memory is initialized to
	0xDD, except that the first eight bytes of the freed memory are used
	for free list connectivity and do not have a defined value.
*/
class sysMemSimpleAllocator: public sysMemAllocator 
{
#if ENABLE_SMALLOCATOR_CODE
	friend class sysSmallocator;
	friend struct sysSmallocator::Chunk;
#endif

public:
#if ENABLE_SMALLOCATOR_CODE
	enum {SM_ALLOCATOR_INCREMENT = sysMemAllocator::DEFAULT_ALIGNMENT, SM_ALLOCATOR_RANGE = sysSmallocator::MAX_SMALLOCATOR_SIZE, SM_ALLOCATOR_SIZE = (sysSmallocator::MAX_SMALLOCATOR_SIZE / SM_ALLOCATOR_INCREMENT)};
#endif

    enum
    {
        DEFAULT_QUIT_ON_FAILURE     = true
    };

	enum
	{
		HEAP_MAIN = 0,
		HEAP_NET = 1,
		HEAP_DEBUG = 2,
		HEAP_FLEX = 3,
		HEAP_FRAG = 4,
		HEAP_HEADER = 5,

		HEAP_APP_1 = 6,
		HEAP_APP_2 = 7,

		HEAP_REPLAY = 8,
		HEAP_CHUNKY = 9
	};

    //PURPOSE
    //  Initialize the allocator with a caller-allocated heap.
    //PARAMETERS
    //  heap        - Pointer to the heap.
    //  heapSize    - Size in bytes of the heap.
	//	heapId		- unique heap index to bias the allocation id's and avoid overlaps, should be between 1 and 7 (0 is main heap)
    //NOTES
    //  The caller must not deallocate the heap memory until the allocator
    //  instance is destroyed.
    sysMemSimpleAllocator( void* heap, const int heapSize, const int heapId, bool allowSmallAllocator = true );

	sysMemSimpleAllocator(int heapSize, const int heapId, bool allowSmallAllocator = true);
	SYS_MEM_VIRTUAL ~sysMemSimpleAllocator();

    //PURPOSE
    //  Configures how the allocator behaves when an allocation fails.
    //PARAMS
    //  quitOnFail  - If true, an allocation failure will Quitf().
    //                If false, an allocation failure will return NULL
    //                from Allocate().
    //RETURNS
    //  Old quit-on-fail value.
    SYS_MEM_VIRTUAL bool SetQuitOnFail(const bool quitOnFail);

	// NOTES:	Memory allocation is O(N) in the number of free nodes.
	//			The allocator will return the first exact match or the
	//			best fit.
	SYS_MEM_VIRTUAL void *Allocate(size_t size,size_t align,int heapIndex = 0);

	SYS_MEM_VIRTUAL void* TryAllocate(size_t size,size_t align,int heapIndex = 0);

	// NOTES:	Memory freeing is O(1).
	SYS_MEM_VIRTUAL void Free(const void *ptr);

	SYS_MEM_VIRTUAL void Resize(const void *ptr,size_t newSmallerSize);

	SYS_MEM_VIRTUAL size_t GetSize(const void *ptr) const;

	SYS_MEM_VIRTUAL size_t GetSizeWithOverhead(const void *ptr) const;

	SYS_MEM_VIRTUAL size_t GetMemoryUsed(int bucket);

	SYS_MEM_VIRTUAL size_t GetMemoryAvailable();

	SYS_MEM_VIRTUAL size_t GetLargestAvailableBlock();

	SYS_MEM_VIRTUAL bool HasMemoryBuckets() const { return true; }

	SYS_MEM_VIRTUAL void SanityCheck();

#if !__FINAL && !__SPU
	void VerifyFreelist() const;
#endif

	SYS_MEM_VIRTUAL bool IsValidPointer(const void *ptr) const;
	BANK_ONLY(SYS_MEM_VIRTUAL bool IsValidUsedPointer(const void *ptr) const;)

	SYS_MEM_VIRTUAL void BeginLayer();

	SYS_MEM_VIRTUAL int EndLayer(const char *layerName,const char *leakfile);

	SYS_MEM_VIRTUAL void BeginMemoryLog(const char *filename,bool logStackTracebacks);

	SYS_MEM_VIRTUAL void EndMemoryLog();

	static  void DisplayLeaksAsErrors( bool val = true )	{ sm_leaksAsErrors = val;}

	static  void HideAllocatorOutput( bool val = true )	{ sm_hideAllocOutput = val;}
	
	SYS_MEM_VIRTUAL void* GetHeapBase() const;

	SYS_MEM_VIRTUAL size_t GetHeapSize() const;

	SYS_MEM_VIRTUAL size_t GetLowWaterMark(bool reset);

	SYS_MEM_VIRTUAL size_t GetHighWaterMark(bool reset) {return GetHeapSize() - GetLowWaterMark(reset);}

	SYS_MEM_VIRTUAL void UpdateMemorySnapshot();

	SYS_MEM_VIRTUAL size_t GetMemorySnapshot(int bucket);

#if RESOURCE_HEADER
	SYS_MEM_VIRTUAL u32 GetUserData(const void* ptr) const;
	SYS_MEM_VIRTUAL void SetUserData(const void* ptr, u32 userData);
	SYS_MEM_VIRTUAL const void *GetCanonicalBlockPtr(const void* ptr) const;
#endif

#if RAGE_TRACKING
	SYS_MEM_VIRTUAL bool IsTallied() { return m_IsTallied; }
	void SetTallied(bool isTallied) { m_IsTallied = isTallied; }
#endif

#if __DEV
    // *** DEBUG ONLY - VERY SLOW! ***
    void SanityCheckHeapOnDoAllocate( bool val ) { m_sanityCheckHeapOnDoAllocate = val; }
#endif
#if !__FINAL
	void PrintMemoryUsage() const;
	void PrintBuckets() const;
	void PrintFreeList() const;

#if (__PPU || __XENON) && !__TOOL && !__RESOURCECOMPILER
	// HACK: Consolidate the bucket values for multi sysMemSimpleAllocator classes
	static void PrintAllMemoryUsage();
#endif // #if (__PPU || __XENON) && !__TOOL && !__RESOURCECOMPILER
#endif

#if __BANK
	SYS_MEM_VIRTUAL void AddWidgets(bkGroup& grp);
#endif

#if ENABLE_SMALLOCATOR_CODE
	bool HasSmallocator() { return m_EnableSmallocator; }
	u32 GetSmallocatorBlockCount() { return m_PageArray.CountBits(true); }
	size_t GetSmallocatorTotalSize() { return m_PageArray.CountBits(true) * sysSmallocator::CHUNK_ALIGNMENT; }

#if !__NO_OUTPUT
	void DumpSMAllocators();
#endif // !__NO_OUTPUT
#endif // ENABLE_SMALLOCATOR_CODE

protected:
    void InitHeap( void* heap, int heapSize, bool allowSmallAllocator );

	void *DoAllocate(size_t size,size_t align);
	void DoFree(const void *ptr);
	void DoSanityCheck();

#if !__FINAL
	void BumpAllocId();
#endif

	static const size_t c_Align = 16;
	struct Node {		// sizeof(node) must equal c_Align
		u32 Guard;	// Initialized with (lower 32 bits of) its own address to catch overruns
		u32 Size;	// Size of payload (rounded up to c_Align)
#if __64BIT
		s32 PrevOffs;// Offset to previous block
#else
		Node *PrevPtr;
#endif
		u32 AllocId: 26,	// Allocation id, for leak identification (keep in sync with top of Allocate function)
			Visited : 1,	// Used internally during SanityCheck, should always be zero otherwise.
			Used : 1,		// Memory block is in used (as opposed to free)
			Bucket : 4;		// Bucket id, for memory classification

#if __64BIT
		Node* GetPrev() const
		{
			return PrevOffs? (Node*)((intptr_t)this - (intptr_t)PrevOffs) : (Node*) NULL;
		}
		void SetPrev(Node *prev)
		{
			if (!prev)
				PrevOffs = 0;
			else {
				ptrdiff_t offs = ((intptr_t)this - (intptr_t)prev);
				Assign(PrevOffs,offs);
			}
			Assert(GetPrev() == prev);
		}
#else
		void SetPrev(Node *prev) { PrevPtr = prev; }
		Node* GetPrev() const { return PrevPtr; }
#endif
	};
	struct FreeNode: public Node {	// sizeof(FreeNode) must equal 2*c_Align
		FreeNode *PrevFree, *NextFree;
#if !__64BIT
		void *Pad0, *Pad1;
#endif
	};
	CompileTimeAssert(sizeof(Node)==(c_Align) && sizeof(FreeNode)==(c_Align*2));
	Node* GetNextNode(Node* current) const {
		Node * n = (Node*) ((char*)current + ((current->Size + c_Align))); 
		return ((char*)n >= (char*)m_Heap + m_HeapSize)? NULL : n;
	}
	int GetFreeListIndex(size_t size);
	void RemoveFromFreeList(FreeNode *fn,int which);
	void AddToFreeList(FreeNode *fn,int which);
	FreeNode* CheckFreeList(int fli,size_t size,size_t align);
	void *m_Heap;			// The memory arena (aligned).
    void* m_OrigHeap;       // The original unaligned pointer to the heap.
	static const int c_FreeListCount = 32;
	atRangeArray<FreeNode *,c_FreeListCount> m_FreeLists;	// Pointer to first free block
	size_t m_HeapSize;			// Total size of the heap
    size_t m_OrigHeapSize;      // Original heap size specified by the caller
                                // This could be different from m_HeapSize due
                                // to alignment.
	size_t m_MemoryUsed;		// Total memory used
	size_t m_MemoryUsedByBucket[16];		// ...and by bucket (should add up to m_MemoryUsed)
	size_t m_MemoryUsedByBucketSnapshot[16];// ...and snapshot of buckets at some previous time (e.g. just after startup)
	size_t m_MemoryAvailable;	// Total memory available
	size_t m_LeastMemoryAvailable;

	static const int c_MaxLayers = 8;
	u32 m_AllocIdByLayer[c_MaxLayers];
	int m_LayerCount;
	bool m_IsLocked;
	bool m_LogStackTracebacks;
	bool m_OwnHeap;
#if RAGE_TRACKING
	bool m_IsTallied;
#endif
#if ENABLE_SMALLOCATOR_CODE
	bool m_EnableSmallocator;
#endif
	fiStream *m_StackFile;
	int m_MemAllocId;
	
	static bool sm_leaksAsErrors;
	static bool sm_hideAllocOutput;

#if ENABLE_SMALLOCATOR_CODE
#if RAGE_MEMORY_DEBUG
	void DoDebugAllocate(const void* ptr);
	void DoDebugFree(const void* ptr);
#endif

	sysSmallocator *GetSmallocator(size_t size);
	size_t GetSmallocatorRange() const {return SM_ALLOCATOR_RANGE;}
	sysSmallocator::Chunk *IsSmallocatorChunk(const void *ptr) const;
	void InitSmallocator();

	int GetPageIndex(const void *ptr) const 
	{
		FastAssert(ptr>=m_Heap&&ptr<(char*)m_Heap+m_HeapSize);
		return int(((char*)ptr - m_PageBase) >> sysSmallocator::CHUNK_ALIGNMENT_SHIFT);
	}
	
	void SetPageBit(const void *ptr) 
	{ 
		int idx = GetPageIndex(ptr);
		FastAssert(!m_PageArray.IsSet(idx));
		m_PageArray.Set(idx);
	}
	
	void ClearPageBit(const void *ptr)
	{
		int idx = GetPageIndex(ptr);
		FastAssert(m_PageArray.IsSet(idx));
		m_PageArray.Clear(idx);
	}

	sysSmallocator m_smallAllocator[SM_ALLOCATOR_SIZE];

#if RAGE_MEMORY_DEBUG
	u32 m_smAllocationCount[SM_ALLOCATOR_SIZE];
	u32 m_smAllocationMax[SM_ALLOCATOR_SIZE];	
#endif

	// ====================== Platform Dependent =========================
	static const int MAX_HEAP_SIZE = ((RSG_PC || RSG_DURANGO || RSG_ORBIS) ? 1024 : 256) * 1024 * 1024;
	// ====================== Platform Dependent =========================
	static const int PAGE_TABLE_SIZE = MAX_HEAP_SIZE / sysSmallocator::CHUNK_ALIGNMENT;
	char *m_PageBase;
	// Max heap size is 256*1024/64=4096 pages long.
	// Extra slop for the case of a 256M heap not on a 64k boundary.
	atFixedBitSet<PAGE_TABLE_SIZE+8> m_PageArray;
#endif

    //If true (default) allocation failures will Quitf().
    bool m_QuitOnFailure    : 1;

#if __DEV
    //If true sanity check the heap on every allocation
    // *** DEBUG ONLY - VERY SLOW! ***
    bool m_sanityCheckHeapOnDoAllocate : 1;
#endif

	sysCriticalSectionToken m_Token;
};

};

#endif
