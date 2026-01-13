#ifndef SYSTEM_TINYBUDDYHEAP_H
#define SYSTEM_TINYBUDDYHEAP_H

// GTA is still using VS2008 for ragebuilder, etc.  Rather than attempt to
// support this ancient compiler (which has very poor C++11 typed enum support),
// just #ifdef out this entire file instead.
#if !defined(_MSC_VER) || (_MSC_VER > 1600 && !RSG_TOOL)

#include "math/amath.h"


namespace rage {

#if RSG_ORBIS || RSG_DURANGO
	enum { MEMTYPE_CPU_RO               = 0x08 }; // CPU read-only, if not set, then read/write
	enum { MEMTYPE_CPU_WB               = 0x04 }; // CPU write back caching, if not set, then non-cached, write combined
	enum { MEMTYPE_GPU_RO               = 0x02 }; // GPU read-only, if not set, then read/write
	enum { MEMTYPE_GPU_GARLIC           = 0x01 }; // GPU GARLIC access, if not set, then ONION access
	enum { NUM_MEMTYPES                 = 16   };
#	if RSG_ORBIS
		enum { MEMTYPE_DEFAULT              = /*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0 };
		enum { MEMTYPE_DEFAULT_GPU_RO       =   MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC   };
		enum { MEMTYPE_DEFAULT_GPU_RW       = /*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC   };
		enum { LEGAL_MEMTYPES               =
			  (false << (/*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0))
			| (true  << (/*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  ))
			| (false << (/*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 |   MEMTYPE_GPU_RO    | /*MEMTYPE_GPU_ONION*/0))
			| (false << (/*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  ))
			| (true  << (/*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0))
			| (false << (/*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  ))
			| (false << (/*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    | /*MEMTYPE_GPU_ONION*/0))
			| (false << (/*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  ))
			| (false << (  MEMTYPE_CPU_RO    | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0))
			| (false << (  MEMTYPE_CPU_RO    | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  ))
			| (false << (  MEMTYPE_CPU_RO    | /*MEMTYPE_CPU_WC*/0 |   MEMTYPE_GPU_RO    | /*MEMTYPE_GPU_ONION*/0))
			| (false << (  MEMTYPE_CPU_RO    | /*MEMTYPE_CPU_WC*/0 |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  ))
			| (false << (  MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0))
			| (false << (  MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  ))
			| (false << (  MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    | /*MEMTYPE_GPU_ONION*/0))
			| (true  << (  MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  )) };
#	elif RSG_DURANGO
		// TODO: WB/GARLIC should be read-only for the GPU
		//enum { MEMTYPE_DEFAULT              = /*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  };
		enum { MEMTYPE_DEFAULT              = /*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  };
		enum { MEMTYPE_DEFAULT_GPU_RO       = /*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  };
		enum { MEMTYPE_DEFAULT_GPU_RW       = /*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  };
		enum { LEGAL_MEMTYPES               =
			  (false << (/*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0))
			| (true  << (/*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  ))
			| (false << (/*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 |   MEMTYPE_GPU_RO    | /*MEMTYPE_GPU_ONION*/0))
			| (false << (/*MEMTYPE_CPU_RW*/0 | /*MEMTYPE_CPU_WC*/0 |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  ))
			| (true  << (/*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0))
			| (true  << (/*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  )) // this really should not be legal, but we are currently using it
			| (false << (/*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    | /*MEMTYPE_GPU_ONION*/0))
			| (true  << (/*MEMTYPE_CPU_RW*/0 |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  ))
			| (false << (  MEMTYPE_CPU_RO    | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0))
			| (false << (  MEMTYPE_CPU_RO    | /*MEMTYPE_CPU_WC*/0 | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  ))
			| (false << (  MEMTYPE_CPU_RO    | /*MEMTYPE_CPU_WC*/0 |   MEMTYPE_GPU_RO    | /*MEMTYPE_GPU_ONION*/0))
			| (false << (  MEMTYPE_CPU_RO    | /*MEMTYPE_CPU_WC*/0 |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  ))
			| (false << (  MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 | /*MEMTYPE_GPU_ONION*/0))
			| (false << (  MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    | /*MEMTYPE_GPU_RW*/0 |   MEMTYPE_GPU_GARLIC  ))
			| (false << (  MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    | /*MEMTYPE_GPU_ONION*/0))
			| (false << (  MEMTYPE_CPU_RO    |   MEMTYPE_CPU_WB    |   MEMTYPE_GPU_RO    |   MEMTYPE_GPU_GARLIC  )) };
#	endif
#else
	enum { MEMTYPE_DEFAULT              = 0 };
	enum { MEMTYPE_DEFAULT_GPU_RO       = 0 };
	enum { MEMTYPE_DEFAULT_GPU_RW       = 0 };
	enum { NUM_MEMTYPES                 = 1 };
	enum { LEGAL_MEMTYPES               = (1<<NUM_MEMTYPES)-1 };
#endif


class sysTinyBuddyHeap {
public:
	struct Node {

		enum : u64 {
			// Free node specific bit masks //

			// When a node is free, it is stored in a doubly linked list of free nodes of the same height.
			// Each page is 64KiB, so 22-bits covers 256GiB.
			NEXT_FREE_MASK                  = 0xfffffc0000000000uLL,
			PREV_FREE_MASK                  = 0x000003fffff00000uLL,


			// Allocated virtual node specific bit masks //

			// When virtual nodes are allocated, they store the index of the
			// physical node they are mapped to.  This overlaps the bits for
			// NEXT_FREE_MASK/PREV_FREE_MASK.
			PHYSICAL_MAPPING_MASK           = 0xfffffc0000000000uLL,

			// Allocated virtual nodes also store a 32-bit user data value.
			// This also overlaps the bits for NEXT_FREE_MASK/PREV_FREE_MASK.
			USER_DATA_MASK                  = 0x000003fffffffc00uLL,


			// Allocated physical node specific bit masks //

			// Physical nodes allocated to hold pooled allocations, need to
			// maintain a singly-linked list of nodes with free pool entries.
			POOLED_NODE_NEXT_MASK           = 0xfffffc0000000000uLL,

			// They also store a bit mask of free pool entries.  Set bit
			// indicates entry is free.
			POOLED_NODE_FREE_MASK_MASK      = 0x000003fc00000000uLL,

			// Log base 2 of the number of pool entries per allocation in this
			// node.
			POOLED_NODE_ALLOC_HEIGHT_MASK   = 0x0000000300000000uLL,


			// General bit masks valid in all node types //

			MEMTYPE_MASK                    = 0x00000000000003c0uLL,
			HEIGHT_MASK                     = 0x000000000000003euLL,
			IS_USED_MASK                    = 0x0000000000000001uLL,


			// Shift values for the masks //

			NEXT_FREE_SHIFT                 = CompileTimeCountTrailingZeroes<NEXT_FREE_MASK                 >::value,
			PREV_FREE_SHIFT                 = CompileTimeCountTrailingZeroes<PREV_FREE_MASK                 >::value,
			PHYSICAL_MAPPING_SHIFT          = CompileTimeCountTrailingZeroes<PHYSICAL_MAPPING_MASK          >::value,
			USER_DATA_SHIFT                 = CompileTimeCountTrailingZeroes<USER_DATA_MASK                 >::value,
			POOLED_NODE_NEXT_SHIFT          = CompileTimeCountTrailingZeroes<POOLED_NODE_NEXT_MASK          >::value,
			POOLED_NODE_FREE_MASK_SHIFT     = CompileTimeCountTrailingZeroes<POOLED_NODE_FREE_MASK_MASK     >::value,
			POOLED_NODE_ALLOC_HEIGHT_SHIFT  = CompileTimeCountTrailingZeroes<POOLED_NODE_ALLOC_HEIGHT_MASK  >::value,
			MEMTYPE_SHIFT                   = CompileTimeCountTrailingZeroes<MEMTYPE_MASK                   >::value,
			HEIGHT_SHIFT                    = CompileTimeCountTrailingZeroes<HEIGHT_MASK                    >::value,
			IS_USED_SHIFT                   = CompileTimeCountTrailingZeroes<IS_USED_MASK                   >::value,


			MAX_NUM_NODES                   = (NEXT_FREE_MASK>>NEXT_FREE_SHIFT)+1,
			MAX_HEIGHT                      = CompileTimeLog2Floor<MAX_NUM_NODES>::value,
		};

		u64 bits;

		static void CompileTimeAssertFunction() {
			// Doubly linked free list needs same number of bits for next and
			// prev.  And also the same number of bits as for the mapping of
			// virtual to physical nodes, and the physical pool allocation list.
			CompileTimeAssert((NEXT_FREE_MASK>>NEXT_FREE_SHIFT) == (PREV_FREE_MASK>>PREV_FREE_SHIFT));
			CompileTimeAssert((NEXT_FREE_MASK>>NEXT_FREE_SHIFT) == (PHYSICAL_MAPPING_MASK>>PHYSICAL_MAPPING_SHIFT));
			CompileTimeAssert((NEXT_FREE_MASK>>NEXT_FREE_SHIFT) == (POOLED_NODE_NEXT_MASK>>POOLED_NODE_NEXT_SHIFT));

			// Must be enough bits in height mask to store the log of the allocation size.
			CompileTimeAssert(CompileTimeLog2Floor<(NEXT_FREE_MASK>>NEXT_FREE_SHIFT)+1>::value <= (HEIGHT_MASK>>HEIGHT_SHIFT)+1);

			// Pooled node allocation height must have enough bits to represent
			// node being split into two entries.
			CompileTimeAssert((CompileTimeLog2Floor<(POOLED_NODE_FREE_MASK_MASK>>POOLED_NODE_FREE_MASK_SHIFT)+1>::value>>1)-1
				<= (POOLED_NODE_ALLOC_HEIGHT_MASK>>POOLED_NODE_ALLOC_HEIGHT_SHIFT));

			// Must have enough bits to store all possible memory types.
			CompileTimeAssert((MEMTYPE_MASK>>MEMTYPE_SHIFT)+1 >= NUM_MEMTYPES);

			// User data must be 32-bits.
			CompileTimeAssert((USER_DATA_MASK>>USER_DATA_SHIFT)+1 == 1uLL<<32);
		}

		inline u32 GetIsUsed() const;
		inline u32 GetNextFree() const;
		inline u32 GetPrevFree() const;
		inline u32 GetPhysicalMapping() const;
		inline u32 GetUserData() const;
		inline u32 GetPooledNodeNext() const;
		inline u32 GetPooledNodeFreeMask() const;
		inline u32 GetPooledNodeAllocHeight() const;
		inline u32 GetMemType() const;
		inline u32 GetHeight() const;

		inline void SetNextFree(u32 nextFree);
		inline void SetPrevFree(u32 prevFree);
		inline void SetPhysicalMapping(u32 physicalMapping);
		inline void SetUserData(u32 userData);
		inline void SetPooledNodeNext(u32 pooledNodeNext);
		inline void SetPooledNodeFreeMask(u32 pooledNodeFreeMask);
		inline void SetPooledNodeAllocHeight(u32 pooledNodeAllocHeight);
		inline void SetMemType(u32 memtype);
		inline void SetHeight(u32 height);
		inline void SetIsUsed(u32 isUsed);
	};

	static const u32 TERMINATOR = Node::MAX_NUM_NODES-1;
	void Init(u32 nodeCount,Node *nodeStorage);
	u32 Allocate(u32 count,u32 preferredMemtype);
	void Free(u32 idx);

	inline u32 GetFirstNode(u32 idx) const;
	inline u32 GetSize(u32 idx) const;
	inline u32 GetMemType(u32 idx) const;
	inline u32 GetPhysicalMapping(u32 idx);
	inline u32 GetUserData(u32 idx) const;
	inline u32 GetPooledNodeNext(u32 idx) const;
	inline u32 GetPooledNodeFreeMask(u32 idx) const;
	inline u32 GetPooledNodeAllocHeight(u32 idx) const;
	inline u32 GetNodesUsed() const;
	inline u32 GetNodeCount() const;
	inline Node *GetNodes() const;

	inline void SetMemType(u32 idx, u32 memtype);
	inline void SetPhysicalMapping(u32 idx, u32 physicalMapping);
	inline void SetUserData(u32 idx, u32 userData);
	inline void SetPooledNodeNext(u32 idx, u32 pooledNodeNext);
	inline void SetPooledNodeFreeMask(u32 idx, u32 pooledNodeFreeMask);
	inline void SetPooledNodeAllocHeight(u32 idx, u32 pooledNodeAllocHeight);

private:
	u32 AllocateRecurse(u32 height,u32 memtypeMask);
	void AddToFreeList(u32 idx);
	void RemoveFromFreeList(u32 idx);
	void SanityCheck();
	Node *m_Nodes;
	u32 m_NodeCount, m_Used;
	u32 m_FreeListByHeight[Node::MAX_HEIGHT+1][NUM_MEMTYPES];
};

inline u32 sysTinyBuddyHeap::Node::GetIsUsed() const {
	return (u32)((bits&IS_USED_MASK) >> IS_USED_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetNextFree() const {
	FatalAssert(!GetIsUsed());
	return (u32)((bits&NEXT_FREE_MASK) >> NEXT_FREE_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetPrevFree() const {
	FatalAssert(!GetIsUsed());
	return (u32)((bits&PREV_FREE_MASK) >> PREV_FREE_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetPhysicalMapping() const {
	FatalAssert(GetIsUsed());
	return (u32)((bits&PHYSICAL_MAPPING_MASK) >> PHYSICAL_MAPPING_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetUserData() const {
	FatalAssert(GetIsUsed());
	return (u32)((bits&USER_DATA_MASK) >> USER_DATA_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetPooledNodeNext() const {
	FatalAssert(GetIsUsed());
	return (u32)((bits&POOLED_NODE_NEXT_MASK) >> POOLED_NODE_NEXT_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetPooledNodeFreeMask() const {
	FatalAssert(GetIsUsed());
	return (u32)((bits&POOLED_NODE_FREE_MASK_MASK) >> POOLED_NODE_FREE_MASK_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetPooledNodeAllocHeight() const {
	FatalAssert(GetIsUsed());
	return (u32)((bits&POOLED_NODE_ALLOC_HEIGHT_MASK) >> POOLED_NODE_ALLOC_HEIGHT_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetMemType() const {
	return (u32)((bits&MEMTYPE_MASK) >> MEMTYPE_SHIFT);
}

inline u32 sysTinyBuddyHeap::Node::GetHeight() const {
	return (u32)((bits&HEIGHT_MASK) >> HEIGHT_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetNextFree(u32 nextFree) {
	FatalAssert(!GetIsUsed());
	FatalAssert(nextFree <= NEXT_FREE_MASK>>NEXT_FREE_SHIFT);
	bits = (bits&~NEXT_FREE_MASK) | ((u64)nextFree<<NEXT_FREE_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetPrevFree(u32 prevFree) {
	FatalAssert(!GetIsUsed());
	FatalAssert(prevFree <= PREV_FREE_MASK>>PREV_FREE_SHIFT);
	bits = (bits&~PREV_FREE_MASK) | ((u64)prevFree<<PREV_FREE_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetPhysicalMapping(u32 physicalMapping) {
	FatalAssert(GetIsUsed());
	FatalAssert(physicalMapping <= PHYSICAL_MAPPING_MASK>>PHYSICAL_MAPPING_SHIFT);
	bits = (bits&~PHYSICAL_MAPPING_MASK) | ((u64)physicalMapping<<PHYSICAL_MAPPING_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetUserData(u32 userData) {
	FatalAssert(GetIsUsed());
	FatalAssert(userData <= USER_DATA_MASK>>USER_DATA_SHIFT);
	bits = (bits&~USER_DATA_MASK) | ((u64)userData<<USER_DATA_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetPooledNodeNext(u32 pooledNodeNext) {
	FatalAssert(GetIsUsed());
	FatalAssert(pooledNodeNext <= POOLED_NODE_NEXT_MASK>>POOLED_NODE_NEXT_SHIFT);
	bits = (bits&~POOLED_NODE_NEXT_MASK) | ((u64)pooledNodeNext<<POOLED_NODE_NEXT_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetPooledNodeFreeMask(u32 pooledNodeFreeMask) {
	FatalAssert(GetIsUsed());
	FatalAssert(pooledNodeFreeMask <= POOLED_NODE_FREE_MASK_MASK>>POOLED_NODE_FREE_MASK_SHIFT);
	bits = (bits&~POOLED_NODE_FREE_MASK_MASK) | ((u64)pooledNodeFreeMask<<POOLED_NODE_FREE_MASK_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetPooledNodeAllocHeight(u32 pooledNodeAllocHeight) {
	FatalAssert(GetIsUsed());
	FatalAssert(pooledNodeAllocHeight <= POOLED_NODE_ALLOC_HEIGHT_MASK>>POOLED_NODE_ALLOC_HEIGHT_SHIFT);
	bits = (bits&~POOLED_NODE_ALLOC_HEIGHT_MASK) | ((u64)pooledNodeAllocHeight<<POOLED_NODE_ALLOC_HEIGHT_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetMemType(u32 memtype) {
	FatalAssert(memtype <= MEMTYPE_MASK>>MEMTYPE_SHIFT);
	bits = (bits&~MEMTYPE_MASK) | ((u64)memtype<<MEMTYPE_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetHeight(u32 height) {
	FatalAssert(height <= HEIGHT_MASK>>HEIGHT_SHIFT);
	bits = (bits&~HEIGHT_MASK) | ((u64)height<<HEIGHT_SHIFT);
}

inline void sysTinyBuddyHeap::Node::SetIsUsed(u32 isUsed) {
	FatalAssert(isUsed <= IS_USED_MASK>>IS_USED_SHIFT);
	bits = (bits&~IS_USED_MASK) | ((u64)isUsed<<IS_USED_SHIFT);
}


inline u32 sysTinyBuddyHeap::GetFirstNode(u32 idx) const {
	FatalAssert(idx < m_NodeCount);
	const u32 height = m_Nodes[idx].GetHeight();
	const u32 firstNode = idx&-(1<<height);
	FatalAssert(m_Nodes[firstNode].GetIsUsed());     // height not necissarily valid for internal nodes of free memory blocks
	return firstNode;
}

inline u32 sysTinyBuddyHeap::GetSize(u32 idx) const {
	FatalAssert(idx < m_NodeCount);
	FatalAssert(m_Nodes[idx].GetIsUsed());
	return 1 << m_Nodes[idx].GetHeight();
}

inline u32 sysTinyBuddyHeap::GetMemType(u32 idx) const {
	FatalAssert(idx < m_NodeCount);
	return m_Nodes[idx].GetMemType();
}

inline u32 sysTinyBuddyHeap::GetPhysicalMapping(u32 idx) {
	FatalAssert(idx < m_NodeCount);
	return m_Nodes[idx].GetPhysicalMapping();
}

inline void sysTinyBuddyHeap::SetPhysicalMapping(u32 idx, u32 physicalMapping) {
	FatalAssert(idx < m_NodeCount);
	m_Nodes[idx].SetPhysicalMapping(physicalMapping);
}

inline u32 sysTinyBuddyHeap::GetUserData(u32 idx) const {
	FatalAssert(idx < m_NodeCount);
	return m_Nodes[idx].GetUserData();
}

inline u32 sysTinyBuddyHeap::GetPooledNodeNext(u32 idx) const {
	FatalAssert(idx < m_NodeCount);
	return m_Nodes[idx].GetPooledNodeNext();
}

inline u32 sysTinyBuddyHeap::GetPooledNodeFreeMask(u32 idx) const {
	FatalAssert(idx < m_NodeCount);
	return m_Nodes[idx].GetPooledNodeFreeMask();
}

inline u32 sysTinyBuddyHeap::GetPooledNodeAllocHeight(u32 idx) const {
	FatalAssert(idx < m_NodeCount);
	return m_Nodes[idx].GetPooledNodeAllocHeight();
}

inline u32 sysTinyBuddyHeap::GetNodesUsed() const {
	return m_Used;
}

inline u32 sysTinyBuddyHeap::GetNodeCount() const {
	return m_NodeCount;
}

inline sysTinyBuddyHeap::Node *sysTinyBuddyHeap::GetNodes() const {
	return m_Nodes;
}

inline void sysTinyBuddyHeap::SetMemType(u32 idx, u32 memtype) {
	FatalAssert(idx < m_NodeCount);
	m_Nodes[idx].SetMemType(memtype);
}

inline void sysTinyBuddyHeap::SetUserData(u32 idx, u32 userData) {
	FatalAssert(idx < m_NodeCount);
	m_Nodes[idx].SetUserData(userData);
}

inline void sysTinyBuddyHeap::SetPooledNodeNext(u32 idx, u32 pooledNodeNext) {
	FatalAssert(idx < m_NodeCount);
	m_Nodes[idx].SetPooledNodeNext(pooledNodeNext);
}

inline void sysTinyBuddyHeap::SetPooledNodeFreeMask(u32 idx, u32 pooledNodeFreeMask) {
	FatalAssert(idx < m_NodeCount);
	m_Nodes[idx].SetPooledNodeFreeMask(pooledNodeFreeMask);
}

inline void sysTinyBuddyHeap::SetPooledNodeAllocHeight(u32 idx, u32 pooledNodeAllocHeight) {
	FatalAssert(idx < m_NodeCount);
	m_Nodes[idx].SetPooledNodeAllocHeight(pooledNodeAllocHeight);
}

}	// namespace rage

#endif // !defined(_MSC_VER) || _MSC_VER > 1600

#endif // SYSTEM_TINYBUDDYHEAP_H
