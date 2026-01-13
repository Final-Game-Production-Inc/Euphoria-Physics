// 
// system/poolallocator.h 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// Eric J Anderson
// 
#ifndef SYSTEM_POOL_ALLOCATOR_H
#define SYSTEM_POOL_ALLOCATOR_H

#include "atl/inlist.h"
#include "atl/map.h"
#include "atl/pool.h"
#include "system/criticalsection.h"
#include "system/memory.h"
#include "system/memvisualize.h"

// DO NOT enable this on __FINAL
#define POOL_ALLOCATOR_DEBUG (!__FINAL)

// EJ: When this is enabled, memvisualize will track individual pool new/delete requests.
#define RAGE_POOL_ALLOCATOR_TRACKING (RAGE_TRACKING && 1)

#if RAGE_POOL_ALLOCATOR_TRACKING
	#define REGISTER_POOL_ALLOCATOR_DEBUG(_CLASS, ptr, bytes)													\
		rage::diagTracker* t = rage::diagTracker::GetCurrent();													\
		if (t && rage::sysMemVisualize::GetInstance().HasMisc())												\
		{																										\
			t->InitHeap(#_CLASS, ptr, bytes);																	\
		}
#else
	#define REGISTER_POOL_ALLOCATOR_DEBUG(_CLASS, ptr, bytes)
#endif // RAGE_POOL_ALLOCATOR_TRACKING

#if RAGE_POOL_TRACKING
	#define INIT_POOL_ALLOCATOR_AND_TRACKER(ptr, bytes, size, bAllocInstanceIdArray, _CLASSNAME)				\
		_ms_pAllocator->Init(ptr, bytes, size, bAllocInstanceIdArray, _CLASSNAME)
#else
	#define INIT_POOL_ALLOCATOR_AND_TRACKER(ptr, bytes, size, bAllocInstanceIdArray, _CLASSNAME)				\
		_ms_pAllocator->Init(ptr, bytes, size, bAllocInstanceIdArray)
#endif // RAGE_POOL_TRACKING

#define REGISTER_POOL_ALLOCATOR(_CLASS, _THREADSAFE)															\
	typedef rage::sysMemPoolAllocator::PoolWrapper<_CLASS> Pool;												\
	static rage::sysMemPoolAllocator* _ms_pAllocator;															\
	static Pool* _ms_pPool;																						\
	static Pool* GetPool() { return _ms_pPool; }																\
	static rage::sysMemPoolAllocator* GetAllocator() { return _ms_pAllocator; }									\
	static void InitPool(const size_t bytes, const size_t size,													\
		const rage::MemoryBucketIds membucketId = rage::MEMBUCKET_DEFAULT,										\
		const bool bAllocInstanceIdArray = false);																\
	static void ShutdownPool();																					\
	void* operator new(size_t bytes RAGE_NEW_EXTRA_ARGS_UNUSED)													\
	{																											\
		Assert(_ms_pAllocator);																					\
		void* ptr;																								\
		if (!_THREADSAFE)																						\
			ptr = _ms_pAllocator->Allocate(bytes, 16);															\
		else																									\
			ptr = _ms_pAllocator->SafeAllocate(bytes, 16);														\
		if (!ptr)																								\
			Quitf(ERR_MEM_POOLALLOC_ALLOC_2,"sysMemPoolAllocator - Unable to allocate %" SIZETFMT "d bytes", bytes);						\
		return ptr;																								\
	}																											\
	void operator delete(void* ptr)																				\
	{																											\
		Assert(_ms_pAllocator);																					\
		if (!_THREADSAFE)																						\
			_ms_pAllocator->Free(ptr);																			\
		else																									\
			_ms_pAllocator->SafeFree(ptr);																		\
	}

#define INSTANTIATE_POOL_ALLOCATOR(_CLASS)																		\
	typedef rage::sysMemPoolAllocator::PoolWrapper<_CLASS> Pool;												\
	rage::sysMemPoolAllocator* _CLASS::_ms_pAllocator = NULL;													\
	Pool* _CLASS::_ms_pPool = NULL;																				\
	void _CLASS::InitPool(const size_t bytes, const size_t size, const rage::MemoryBucketIds membucketId,		\
						  const bool bAllocInstanceIdArray)														\
	{																											\
		rage::USE_MEMBUCKET(membucketId);																		\
																												\
		/* The following code allocates memory for the vehicle pool */											\
		Assertf(_ms_pAllocator == NULL, "sysMemPoolAllocator - Trying to call InitPool without first calling ShutdownPool");	\
																												\
		_ms_pAllocator = rage_new rage::sysMemPoolAllocator();													\
		if (!_ms_pAllocator)																					\
		{																										\
			Quitf(ERR_MEM_POOLALLOC_ALLOC_3,"Unable to allocate sysMemPoolAllocator object");												\
		}																										\
																												\
		_ms_pPool = rage_new Pool(_ms_pAllocator);																\
		if (!_ms_pPool)																							\
		{																										\
			Quitf(ERR_MEM_POOLALLOC_ALLOC_4,"Unable to allocate sysMemPoolAllocator::Pool object");										\
		}																										\
																												\
		char* const ptr = rage_aligned_new(16) char[bytes];														\
		if (!ptr)																								\
		{																										\
			Quitf(ERR_MEM_POOLALLOC_ALLOC_5,"Unable to allocate sysMemPoolAllocator memory");												\
		}																										\
																												\
		INIT_POOL_ALLOCATOR_AND_TRACKER(ptr, bytes, size, bAllocInstanceIdArray, #_CLASS);						\
		REGISTER_POOL_ALLOCATOR_DEBUG(_CLASS, ptr, bytes);														\
	}																											\
																												\
	void _CLASS::ShutdownPool()																					\
	{																											\
		/* The following code allocates memory for the vehicle pool*/											\
		if (_ms_pPool)																							\
		{																										\
			delete _ms_pPool;																					\
			_ms_pPool = NULL;																					\
		}																										\
																												\
		if (_ms_pAllocator)																						\
		{																										\
			delete _ms_pAllocator;																				\
			_ms_pAllocator = NULL;																				\
		}																										\
	}

namespace rage
{

#if RAGE_POOL_TRACKING
	class sysMemPoolAllocator;
	class sysMemPoolAllocatorTracker : public PoolTracker 
	{
	public:
		void SetAllocator(const sysMemPoolAllocator* allocator) { m_poolPointer = allocator; }
		const sysMemPoolAllocator* GetAllocator() const { return (const sysMemPoolAllocator*)m_poolPointer; }

		void SetName(const char* name) { m_Name = name; }
		virtual const char* GetClassName() const { return GetName(); }

		virtual size_t GetStorageSize() const { return 0; } // Undefined; element size is variable
		virtual s32 GetSize() const;
		virtual s32 GetNoOfUsedSpaces() const;
		virtual s32 GetPeakSlotsUsed() const;
		virtual s32 GetNoOfFreeSpaces() const;
		virtual s32 GetActualMemoryUse() const;
		virtual const char* GetName() const { return m_Name.c_str(); }
		virtual s32 GetMemoryUsed() const;

	private:
		ConstString m_Name;
	};
#endif // RAGE_POOL_TRACKING

	/* 
		[Author]
		Eric J Anderson

		[Overview]
		sysMemPoolAllocator is a 16 byte-aligned allocator. It acts like an iteratable pool but with dynamic allocation abilities. The class was 
		designed as a memory optimization for fwPool objects that are sized to a largest, fixed amount. Such pools waste memory when the average 
		allocation size is much less than the worst-case scenario.
		
		[Notes]
		* All allocations are 16 byte aligned and sized to 16 byte chunks
		* No support for alignments > 16 bytes - this saves processing time and management overhead
		* 
	*/
	class sysMemPoolAllocator
	{
	private:
		struct PoolNode
		{
			void* m_ptr;
		};

		struct SizeNode
		{
			size_t m_used : 1;							// used = 1, free = 0
			size_t m_size : ((sizeof(void*) * 8) - 1);	// 2,147,483,648 max bytes  (32 bit)

			SizeNode() : m_used(false), m_size(0) { }
			SizeNode(const size_t size, const bool used) : m_used(used), m_size(size) { }
		};

		struct FreeNode
		{
			size_t m_used : 1;							// used = 1, free = 0
			size_t m_size : ((sizeof(void*) * 8) - 1);	// 2,147,483,648 max bytes  (32 bit)			
			inlist_node<FreeNode> m_ListLink;

			FreeNode() : m_used(false), m_size(0) { }
			explicit FreeNode(const size_t size) : m_used(false), m_size(size) { }
		};

		struct UsedNode
		{
			PoolNode* m_pPoolNode;
			size_t m_prev_used : 1;						// used = 1, free = 0
			size_t m_size : ((sizeof(void*) * 8) - 1);	// 2,147,483,648 max bytes  (32 bit)			

			UsedNode(PoolNode* pPoolNode, const size_t size) : m_pPoolNode(pPoolNode), m_size(size), m_prev_used(true) { }

			inline void* GetPtr() const { Assert(m_pPoolNode); return m_pPoolNode->m_ptr; }
		};
		
		typedef atIteratablePool<PoolNode> HeaderPool;
		typedef atSimplePooledMapType<void*, UsedNode, sizeof(void*)> PoolHashTable;
		typedef inlist<FreeNode, &FreeNode::m_ListLink> FreeList;

		typedef bool (*Callback)(void* item, void* data);
		typedef void (*PoolFullCB)(void* item);

	private:
		HeaderPool m_pool;
		FreeList m_freeList;
		PoolHashTable m_poolHashTable;
		sysCriticalSectionToken m_Token;

		// Pointer to an array of InstanceIDs. This array is optionally allocated in Init() and
		// is sized the same as the HeaderPool.  Each time an allocation is made, we increment
		// the instanceId at the same index as the PoolNode used for the allocation.  This allows
		// us to generate a handle by calling GetIndex() that can then be used to access the element
		// using GetAt().  GetAt() will compare the instanceId from the passed in handle with the
		// current instanceId value at the slot.  If they differ, we will know that the PoolNode has
		// been re-used and Get() will return NULL (the item we initally had a handle to has been
		// deleted, and a new item has been allocated using the same PoolNode).
		// (This has been implemented to mimic the reference values in fwBasePool.  It's optional because
		// there is some memory overhead involved and this behavior is not required for all uses
		// of the sysMemPoolAllocator)
		u8*	m_aInstanceIds;

		void* m_base;
		size_t m_capacity;
		size_t m_used;
		

#if POOL_ALLOCATOR_DEBUG || RAGE_POOL_TRACKING
		size_t m_peakUsed;
		PoolFullCB m_poolFullCB;
#endif

#if RAGE_POOL_TRACKING
		sysMemPoolAllocatorTracker m_tracker;
#endif

	public:
		const static size_t s_alignment = 16;
		const static size_t s_minFreeBlockSize = static_cast<size_t>(sizeof(FreeNode) + sizeof(SizeNode));
		const static size_t s_minAllocateSize =  (s_minFreeBlockSize + s_alignment - 1) & (~(s_alignment - 1));
		const static size_t s_maxCapacity = static_cast<size_t>(~0) >> 1;
		const static size_t s_invalidIndex = 0x7FFFFFFF;

	private:
#if POOL_ALLOCATOR_DEBUG
		// Verify
		inline bool IsValid(const size_t bytes) const { return bytes && IsAligned(bytes) && (bytes <= s_maxCapacity); }
		inline bool IsValid(const void* const ptr) const { return IsWithinHeap(ptr) && IsAligned(ptr); }
		inline bool IsValid(const PoolNode* const pPoolNode) const { return m_pool.IsInPool(pPoolNode) && IsWithinHeap(pPoolNode->m_ptr) && IsAligned(pPoolNode->m_ptr); }
		inline bool IsValid(const SizeNode* const pSizeNode) const { return IsWithinHeap(pSizeNode) && pSizeNode->m_size && !pSizeNode->m_used; }
		inline bool IsValid(const FreeNode* const pFreeNode) const { return IsWithinHeap(pFreeNode) && IsAligned(pFreeNode) && pFreeNode->m_size >= s_minFreeBlockSize && !pFreeNode->m_used; }
		inline bool IsValid(const UsedNode* const pUsedNode) const
		{ 
			const PoolNode* pPoolNode = pUsedNode->m_pPoolNode;
			return pPoolNode ? IsWithinHeap(pPoolNode->m_ptr) && IsAligned(pPoolNode->m_ptr) && pUsedNode->m_size : false;
		}
#endif
		// Free
		FreeNode* MarkFree(void* const ptr, const size_t bytes);
		FreeNode* GetPrevFreeNode(UsedNode* pUsedNode) const;
		FreeNode* GetNextFreeNode(UsedNode* pUsedNode) const;
		void AddToFreeList(FreeNode* pNode);

#if POOL_ALLOCATOR_DEBUG
		bool IsFreePointer(const void* const ptr) const;
#endif
		// Used
		void SetUsedNode(PoolNode* pPoolNode, const size_t bytes);
		inline UsedNode* GetUsedNode(const void* const ptr) const { return const_cast<UsedNode*>(m_poolHashTable.Access(const_cast<void*>(ptr))); }		

		// Utility
		inline bool IsWithinHeap(const void* const ptr) const { return (ptr >= m_base) && ptr < (static_cast<char*>(m_base) + m_capacity); }
		inline void* GetOffset(const void* const ptr, const int bytes) const { return static_cast<char*>(const_cast<void*>(ptr)) + bytes; }

		inline bool IsAligned(const void* const ptr) const { return !(reinterpret_cast<size_t>(ptr) & (s_alignment - 1)); }
		inline bool IsAligned(const size_t bytes) const { return !(bytes & (s_alignment - 1)); }

	public:
		sysMemPoolAllocator() : m_base(NULL), m_capacity(0), m_used(0), m_aInstanceIds(NULL) {}
		sysMemPoolAllocator(void* const ptr, const size_t bytes, const size_t size, const bool bAllocInstanceIdArray = false)
			: m_base(NULL), m_capacity(0), m_used(0), m_aInstanceIds(NULL)
		{
			Init(ptr, bytes, size, bAllocInstanceIdArray RAGE_POOL_TRACKING_ONLY(, ""));
		}
		~sysMemPoolAllocator();

		// PURPOSE:	Init the pool and heap
		void Init(void* const ptr, const size_t bytes, const size_t size, bool bAllocInstanceIdArray = false
			RAGE_POOL_TRACKING_ONLY(, const char* debugName = ""));

		// PURPOSE:	Allocate memory with a specified alignment
		void* Allocate(size_t bytes, size_t align, bool threadSafe = false);
		void* SafeAllocate(size_t bytes, size_t align);

		// PURPOSE:	Free memory allocated via Allocate.
		void Free(void* const ptr, bool threadSafe = false);
		void SafeFree(void* const ptr);

		// PURPOSE:	Returns actual amount of memory associated with the allocation.
		inline size_t GetSize(const void* const ptr) const
		{
			const UsedNode* pUsedNode = GetUsedNode(ptr);
			return pUsedNode ? pUsedNode->m_size : 0;
		}

		// RETURNS:	Base address of the heap
		inline void* GetHeapBase() const { return m_base; }

		// PURPOSE: Returns amount of memory used on heap
		inline size_t GetMemoryUsed() const { return m_used; }

		// PURPOSE: Returns amount of memory remaining on heap
		inline size_t GetMemoryAvailable() const { return m_capacity - m_used; }

		// PURPOSE:	Returns largest amount of memory that can be allocated in a single contiguous block
		inline size_t GetLargestAvailableBlock() const
		{
			if(m_freeList.empty())
			{
				return 0;
			}
			const FreeNode* pFreeNode = m_freeList.back();
			return pFreeNode ? pFreeNode->m_size : 0;
		}

		// PURPOSE: Amount of fragmentation, as a percentage (0-100).  Returns -1 if unknown.
		inline size_t GetFragmentation() const
		{
			if (GetLargestAvailableBlock() > 0)
				return 100 - static_cast<size_t>((static_cast<u64>(GetLargestAvailableBlock()) * 100) / GetMemoryAvailable());

			return 0;
		}

		// PURPOSE:	True if this is a valid heap pointer owned by this heap.
		inline bool IsValidPointer(const void* const ptr) const
		{ 
			ASSERT_ONLY(if (sysMemVerifyMainThread) sysMemVerifyMainThread();)
			return m_poolHashTable.Access(const_cast<void*>(ptr)) != NULL; 
		}

		// PURPOSE: Return the size of the heap; the same as the size parameter passed into Init
		inline size_t GetHeapSize() const { return m_capacity; }

		// PURPOSE: Resets all pool entries to free; O(N) operation
		void Reset();

		// for all items in a pool		
		void ForAll(Callback cb, void* data);

#if POOL_ALLOCATOR_DEBUG || RAGE_POOL_TRACKING
		inline size_t GetPeakMemoryUsed() const { return m_peakUsed; }
		inline size_t GetPeakSlotsUsed() const { return m_pool.GetPeakSlotsUsed(); }
		inline void RegisterPoolCallback(PoolFullCB callback) { m_poolFullCB = callback; }
		void PoolFullCallback() const;
#endif
		// PURPOSE:	Run basic sanity checks on the heap.
		ASSERT_ONLY(void SanityCheck();)

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// The following class and functions "mimic" pool classes.  In essence, we are tricking the game code into 
		// thinking that it's working directly with pools while all the time the sysMemPoolAllocator is in control.
		//
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		template <typename T>
		class PoolWrapper
		{
		public:
			typedef T ValueType;

		private:
			sysMemPoolAllocator* m_pAllocator;

		public:
			inline PoolWrapper() : m_pAllocator(NULL) { }
			inline explicit PoolWrapper(sysMemPoolAllocator* pAllocator) : m_pAllocator(pAllocator) { Assert(pAllocator); }
			
			// Utility
			inline bool IsValidPtr(const void* const ptr) const { return m_pAllocator->IsValidPointer(ptr); }

			inline size_t GetSize() const { return m_pAllocator->GetSize(); }
			inline size_t GetCount() const { return m_pAllocator->GetCount(); }
			inline size_t GetNoOfFreeSpaces() const	{ return m_pAllocator->GetNoOfFreeSpaces(); }
			inline size_t GetNoOfUsedSpaces() const	{ return m_pAllocator->GetNoOfUsedSpaces(); }

			inline size_t GetJustIndex(const void* const ptr) const { return m_pAllocator->GetJustIndex(ptr); }
			inline bool GetIsFree(s32 index) const { return m_pAllocator->GetIsFree(index); }
			inline bool GetIsUsed(s32 index) const { return m_pAllocator->GetIsUsed(index); }

			inline const T* GetSlot(s32 index) const { return static_cast<T*>(m_pAllocator->GetSlot(index)); }
			inline T* GetSlot(s32 index) { return static_cast<T*>(m_pAllocator->GetSlot(index)); }

			inline void* GetAt(s32 index) { return m_pAllocator->GetAt(index); }
			inline s32 GetIndex(const void* ptr) const { return m_pAllocator->GetIndex(ptr); }

			inline void ForAll(Callback cb, void* data) { m_pAllocator->ForAll(cb, data); }

#if POOL_ALLOCATOR_DEBUG || RAGE_POOL_TRACKING
			inline size_t GetPeakSlotsUsed() const { return m_pAllocator->GetPeakSlotsUsed(); }
			inline void RegisterPoolCallback(PoolFullCB callback) { m_pAllocator->RegisterPoolCallback(callback); }
#endif
			// Mutators
			inline void SetAllocator(sysMemPoolAllocator* pAllocator) { m_pAllocator = pAllocator; }

			// Accessors
			inline sysMemPoolAllocator* GetAllocator() const { return m_pAllocator; }
		};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// Pool Functions
		//
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		// RETURNS: Total number of entries in the pool
		inline size_t GetSize() const { return m_pool.GetSize(); }

		// RETURNS: Total number of used entries in the pool
		inline size_t GetCount() const { return m_pool.GetCount(); }

		// PURPOSE: Gets the number of free spaces
		inline size_t GetNoOfFreeSpaces() const	{ return m_pool.GetNoOfFreeSpaces(); }

		// PURPOSE: Gets the number of used spaces
		inline size_t GetNoOfUsedSpaces() const	{ return m_pool.GetNoOfUsedSpaces(); }

		// PURPOSE: Determine the state of the pointer associated with the corresponding index
		inline bool GetIsFree(s32 index) const { return m_pool.GetIsFree(index); }
		inline bool GetIsUsed(s32 index) const { return m_pool.GetIsUsed(index); }

		// PURPOSE: Identify the index of an element so it can be accessed
		inline size_t GetJustIndex(const void* const ptr) const
		{
			const UsedNode* pUsedNode = GetUsedNode(ptr);
			return pUsedNode ? m_pool.GetJustIndex(pUsedNode->m_pPoolNode) : s_invalidIndex;
		}

		// PURPOSE: Gets the allocation at a given slot
		inline void* GetSlot(s32 index) { return const_cast<void*>(static_cast<const sysMemPoolAllocator*>(this)->GetSlot(index)); }
		inline const void* GetSlot(s32 index) const
		{
			const PoolNode* pPoolNode = m_pool.GetSlot(index);
			return pPoolNode ? pPoolNode->m_ptr : NULL;
		}

		inline void* GetAt(s32 index)
		{
			Assertf(m_aInstanceIds != NULL, "Instance Id's are not being tracked for this pool");
			
			const u32 i = (index >> 8);
			if(m_aInstanceIds[i] != (index & 0xff))
			{
				return NULL;
			}

			const PoolNode* pPoolNode = m_pool.GetSlot(i);
			if(!pPoolNode)
			{
				return NULL;
			}

			return pPoolNode->m_ptr;
		}

		inline s32 GetIndex(const void* const ptr) const
		{
			Assertf(m_aInstanceIds != NULL, "Instance Id's are not being tracked for this pool");

			const UsedNode* pUsedNode = GetUsedNode(ptr);
			if(!pUsedNode)
			{
				return s_invalidIndex;
			}

			s32 index = m_pool.GetJustIndex(pUsedNode->m_pPoolNode);
			return ((index << 8) + m_aInstanceIds[index]);
		}

	};

} // namespace rage

#endif // SYSTEM_POOL_ALLOCATOR_H
