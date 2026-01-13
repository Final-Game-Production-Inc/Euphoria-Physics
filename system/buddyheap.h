// 
// system/buddyheap.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_BUDDYHEAP_H 
#define SYSTEM_BUDDYHEAP_H 

#include <stddef.h>		// for size_t
#include "buddyallocator_config.h"
#include "atl/array.h" 
#include "atl/pool.h"
#include "system/memory.h"

namespace rage {

struct sysMemDistribution;

#if RSG_PC || RSG_DURANGO || RSG_ORBIS
typedef u32 sysBuddyNodeIdx;
#else
typedef u16 sysBuddyNodeIdx;
#endif

struct sysBuddyHeapDefragInfo
{
	static const int c_MAX_TO_MOVE = 32;
	struct NodeInfo {
		sysBuddyNodeIdx from, to;			// source and destination for move (should really be sysBuddyNodeIdx)
		sysBuddyNodeIdx curSize;			// size, in leaves
	};
	int Count;					// number of nodes to move
	NodeInfo Nodes[c_MAX_TO_MOVE];// which nodes to move, and where
};

#if RESOURCE_POOLS
class sysBuddyHeap;
class sysBuddyPool
{
	friend class sysBuddyHeap;

private:
	size_t m_size;
	size_t m_leafSize;
	sysBuddyNodeIdx m_baseNode, m_endNode;
	atPoolBase m_pool;
	void* m_heapBase;
	void* m_ptr;

public:
	sysBuddyPool() : m_size(0), m_leafSize(0), m_pool(sizeof(void*)), m_heapBase(NULL), m_ptr(NULL), m_baseNode(0xFFFF), m_endNode(0xFFFF) {}

	void Init(void* heapBase, size_t leafSize, void* ptr, size_t capacity, size_t size);
	
	sysBuddyNodeIdx Allocate();
	void Free(sysBuddyNodeIdx addr);
	inline bool IsValidAddr(sysBuddyNodeIdx addr) const {return (addr >= m_baseNode) && (addr < m_endNode);}
	inline bool IsFull() const {return m_pool.IsFull();}

	inline size_t GetLeafSize() const {return m_leafSize;}	
	inline size_t GetStorageSize() const {return m_size;}
	inline void* GetHeapBase() const {return m_heapBase;}
	inline void* GetPtr() const {return m_ptr;}

	inline sysBuddyNodeIdx GetBaseNode() const {return m_baseNode;}
	inline sysBuddyNodeIdx GetEndNode() const {return m_endNode;}

	inline size_t GetSize() const {return m_pool.GetSize();}
	inline size_t GetUsedSize() const {return m_pool.GetNoOfUsedSpaces();}
	inline size_t GetFreeSize() const {return m_pool.GetNoOfFreeSpaces();}

#if !__FINAL
	inline size_t GetPeakSize() const {return m_pool.GetPeakSlotsUsed();}
#endif
};
#endif

class sysBuddyHeap 
{
	friend class sysMemBuddyAllocator;

#pragma pack(push)
#pragma pack(1)
	struct NodeInfo
	{
		sysBuddyNodeIdx _Prev, _Next;
		enum {
			BITS_HEIGHT     = 5,
			BITS_LOCKCOUNT  = 7,
			BITS_BUCKET     = 4,
			BITS_USERLOW    = 16,
		};
		u16 _Height : BITS_HEIGHT, _LockCount : BITS_LOCKCOUNT, _Bucket : BITS_BUCKET;
		u16 _UserLow;
		u8 _UserHigh;
	};
#pragma pack(pop)
	// Must match COMPUTE_BUDDYHEAP_WORKSPACE_SIZE macro in buddyallocator.h
	CompileTimeAssert( sizeof(NodeInfo) == (RSG_PC || RSG_DURANGO || RSG_ORBIS ? 13 : 9) );
public:
#if RSG_PC || RSG_DURANGO || RSG_ORBIS
	static const sysBuddyNodeIdx c_NONE = 0xFFFFFFFF;
	static const unsigned c_MAX_HEIGHT = 24;		// Could probably be as high as 31
#else
	static const sysBuddyNodeIdx c_NONE = 0xFFFF;
	static const unsigned c_MAX_HEIGHT = 16;
#endif
	static const sysBuddyNodeIdx c_INVALID = c_NONE - 1;

	sysBuddyHeap() : m_Nodes(NULL) { }

	/*	PURPOSE:	Intialize the buddy heap.
		PARAMS:		leafCount - Number of leaves in the heap; does not have to be a power of two.
					nodes - pointer to persistent storage of COMPUTE_BUDDYHEAP_WORKSPACE_SIZE(leafCount) bytes */
	void Init(size_t leafCount,void *nodes);

#if RESOURCE_POOLS
	void HideUsedMemory(sysBuddyNodeIdx addr);
	void InitPool(int pos, void* heapBase, size_t leafSize, void* ptr, size_t capacity, size_t size);
	sysBuddyNodeIdx AllocatePoolNode(unsigned level);	

	sysBuddyPool* GetPool(int pos) {return (pos >= 0 && pos < NUM_POOLS) ? &m_pool[pos] : NULL;}
	static size_t GetNumPools() {return NUM_POOLS;}
#endif

	/*	PURPOSE:	Allocate memory from the heap.
		PARAMS:		leafCount - Number of leaves to allocate; must be a power of two.
		RETURNS:	Zero-based node index on success, or c_NONE on failure.
		NOTES:		To turn this into a memory address, multiply by the leaf size and add the base address of the heap. */
	sysBuddyNodeIdx Allocate(size_t leafCount);

	/*  PURPOSE:    Get the first node index of the allocation that a node belongs to.
		PARAMS:     addr - Zero-based node index inside of allocation.
		RETURNS:    Zero-based node index of first node in allocation.
		NOTES:      Undefined if addr is free. */
	sysBuddyNodeIdx GetFirstNode(sysBuddyNodeIdx addr) const;

	/*	PURPOSE:	Free memory from the heap.
		PARAMS:		addr - Zero-based node index previously returned by Allocate
		NOTES:		We trap invalid addresses (out of range, already freed, or not an active node. 
					If our buddy is already free, we coalesce up the tree at this time. */
	void Free(sysBuddyNodeIdx addr);

	/*	PURPOSE:	Perform one step of defragmentation operation.
		PARAMS:		outInfo - Pointer to info array filled out by this function.
		RETURNS:	True if there is any data to be processed by the caller  
		NOTES:		This function operates by identifying a source and destination subtree that
					would improve the amount of unfragmented memory.  The destination subtree
					is immediately allocated by this function.  The caller must, for each node
					listed, either choose to perform the move and free the source block, or choose
					not to perform the mood and instead free the destination block, after which both
					the source and destination blocks must have UnlockBlock called on them.  Failure to do
					all of this will result in a memory leak.  Also, if you choose not to move
					some of the memory blocks, you're defeating the purpose and may not actually
					make any progress on defragmentation. */
	bool Defragment(sysBuddyHeapDefragInfo &outInfo, sysMemDefragmentationFree& outDefragInfo, void *heapBase,size_t nodeSize, unsigned maxHeight);

	/*	PURPOSE:	Verify that the buddy heap is in an internally consistent state 
		NOTES:		This function uses a fair amount of stack space (leafCount/8 bytes). */
	void SanityCheck() const;

	/*	PURPOSE:	Returns the size of the node if it's valid and in use */
	size_t GetSize(sysBuddyNodeIdx addr) const;

	size_t GetLargestAvailableBlock() const;

	size_t GetMemoryUsed(int bucket) const;

	size_t GetMemoryFree() const;

	bool TryLockBlock(sysBuddyNodeIdx addr, unsigned lockCount);

	void UnlockBlock(sysBuddyNodeIdx addr, unsigned unlockCount);

	int GetBlockLockCount(sysBuddyNodeIdx addr) { return GetNodeLockCount(addr); }

	void GetMemoryDistribution(sysMemDistribution & /*outDist*/) const;

	void SetUserData(sysBuddyNodeIdx node,u32 userData) {
		FastAssert(node<m_LeafCount); 
		FastAssert(userData < 0xFFFFFF || userData == 0xFFFFFFFF);
		m_Nodes[node]._UserLow = (u16) userData;
		m_Nodes[node]._UserHigh = (u8) (userData >> 16);
	}

	u32 GetUserData(sysBuddyNodeIdx node) const { 
		FastAssert(node<m_LeafCount); 
		u32 result = ((u32) m_Nodes[node]._UserHigh << 16) | m_Nodes[node]._UserLow; 
		return (result == 0xFFFFFF)? 0xFFFFFFFF : result; 
	}

#if ENABLE_DEFRAG_CALLBACK
	static inline void SetIsObjectReadyToDelete(IsObjectReadyToDelete callback) {s_isObjectReadyToDelete = callback;}
#endif

#if __DEV
	// PURPOSE:	Writes the current state of the tree of a .dot file viewable by graphviz.
	bool DumpTreeToDotFile(const char *filename);
#endif

private:
#if RESOURCE_POOLS
	enum {NUM_POOLS = 2};
#endif

	void SetNodeNext(size_t node,sysBuddyNodeIdx next) { FastAssert(node<m_LeafCount); m_Nodes[node]._Next = next; }
	sysBuddyNodeIdx GetNodeNext(size_t node) const { FastAssert(node<m_LeafCount); return m_Nodes[node]._Next; }
	void SetNodePrev(size_t node,sysBuddyNodeIdx prev) { FastAssert(node<m_LeafCount); m_Nodes[node]._Prev = prev; }
	sysBuddyNodeIdx GetNodePrev(size_t node) const { FastAssert(node<m_LeafCount); return m_Nodes[node]._Prev; }
	void SetNodeHeight(size_t node,unsigned height) { FastAssert(node<m_LeafCount); FastAssert(height<=c_MAX_HEIGHT); m_Nodes[node]._Height = height; }
	unsigned GetNodeHeight(size_t node) const { FastAssert(node<m_LeafCount); return m_Nodes[node]._Height; }
	void SetNodeLockCount(size_t node,unsigned lock) { FastAssert(node<m_LeafCount); FastAssert(lock<(1<<NodeInfo::BITS_LOCKCOUNT)); m_Nodes[node]._LockCount = lock; }
	unsigned GetNodeLockCount(size_t node) const { FastAssert(node<m_LeafCount); return m_Nodes[node]._LockCount; }
	void SetNodeBucket(size_t node,unsigned bucket) { FastAssert(node<m_LeafCount); m_Nodes[node]._Bucket = bucket; }
	unsigned GetNodeBucket(size_t node) const { FastAssert(node<m_LeafCount); return m_Nodes[node]._Bucket; }

	void DefragInflightLockBlock(sysBuddyNodeIdx addr);

	void RemoveFromFreeList(sysBuddyNodeIdx nodeIdx,unsigned level);
	void AddToFreeList(sysBuddyNodeIdx nodeIdx,unsigned level);

	sysBuddyNodeIdx AllocateRecurse(unsigned level);

	// One entry per node.  If the node is in use, it will contain a value of at least c_HEIGHT_OFFSET,
	// which encodes its height in the tree (and therefore its size).  If the node is not in use,
	// it will either contain the index of the next node at the same height that isn't in use, or c_NONE.
	NodeInfo *m_Nodes;

	// Number of leaves in the tree; by storing this explicitly we don't require the buddy heap's size
	// to be an exact multiple of a power of two.  Nodes along the right edge will just have invisible
	// buddies that are always in use (we just do a quick compare, we don't need extra storage).
	size_t m_LeafCount;

	// Number of leaves currently in use.
	size_t m_LeavesUsed;

	// Number of leaves currently in use, by bucket
	size_t m_LeavesUsedByBucket[16];

	// Number of nodes of size N currently in use and free, by size
	sysBuddyNodeIdx m_UsedBySize[c_MAX_HEIGHT];
	sysBuddyNodeIdx m_FreeBySize[c_MAX_HEIGHT];

	// Contains the index of the first node of this height that is available for allocation,
	// or c_NONE if none are available.
	sysBuddyNodeIdx m_FirstFreeBySize[c_MAX_HEIGHT];

#if RESOURCE_POOLS
	atRangeArray<sysBuddyPool, NUM_POOLS> m_pool;
#endif

#if ENABLE_DEFRAG_CALLBACK
	static IsObjectReadyToDelete s_isObjectReadyToDelete;
#endif
};

} // namespace rage

#endif // SYSTEM_BUDDYHEAP_H 
