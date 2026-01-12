#include "GjkSimplexCache.h"
#include "ConvexIntersector.h"
#include "phcore/avl_tree.h"
#include "phcore/pool.h"
#include "physics/broadphase.h"
#include "physics/levelnew.h"
#include "profile/element.h"
#include "VoronoiSimplexSolver.h"
#include "GjkPairDetector.h"
#include "physics/physicsprofilecapture.h"
#include "phbound/support.h"	// Includes headers for the different bound types, needed for SeedSimplex.
#include "CollisionWorkUnit.h"

GJK_COLLISION_OPTIMIZE_OFF()

#define ALIGNOF(type) __alignof(type)

#define STORE_SIMPLEX_SUPPORT_POINTS 0

#if VERIFY_SUPPORT_FUNCTIONS
extern void VerifyBoundSupportFunction(const rage::phBound * bound, rage::Vec3V_In dir);
#endif // VERIFY_SUPPORT_FUNCTIONS

namespace rage
{


namespace phCollisionStats
{
	EXT_PF_COUNTER(GJKCacheCount);
	EXT_PF_COUNTER(GJKCacheCreateCount);
	EXT_PF_COUNTER(GJKCacheDBCount);
	EXT_PF_COUNTER(GJKCacheDBCreateCount);

	EXT_PF_COUNTER(GJKCache_CachedSimplexCount);
	EXT_PF_COUNTER(GJKCache_CachedSupportDirCount);

	EXT_PF_TIMER(GJKCacheSytemPostCollisionUpdate);
	EXT_PF_TIMER(GJKCacheQueryProlog);
	EXT_PF_TIMER(GJKCacheQueryEpilog);
}

#define GJK_PF_FUNC(x) //PF_FUNC(x)
#define GJK_PF_INCREMENT(x) //PF_INCREMENT(x)
#define GJK_PF_INCREMENTBY(x,c) //PF_INCREMENTBY(x,c)

using namespace phCollisionStats;

#if USE_GJK_CACHE

__forceinline GJKCacheKey MakeGJKCacheKey(const u32 ComponentA, const u32 ComponentB, const u32 PartIndex)//, const int swap)
{
	FastAssert(ComponentA <= 0xFF);
	FastAssert(ComponentB <= 0xFF);
	FastAssert(PartIndex <= 0xFFFF);
	//FastAssert(swap == 0 || swap == 1);
	GJKCacheKey key;
	//key = (PartIndex << 16) | (ComponentA << (8 * (1-swap))) | (ComponentB << (8 * swap));
	key = (PartIndex << 16) | (ComponentA << 8) | (ComponentB);
	return key;
}

typedef u16 GJK_VERTEX_ID; // TODO: Temporary fix for a crash.
//typedef u8 GJK_VERTEX_ID;
#if USE_FRAME_PERSISTENT_GJK_CACHE
// PS3 non-unity and PS3 physics samples complain about unused const variables.
const u32 GJK_VERTEX_ID_MAX = (1 << (8 * sizeof(GJK_VERTEX_ID))) - 1;
#endif
typedef u8 GJK_FLAG_TYPE;

class GJKCache
{
public:
#if STORE_SIMPLEX_SUPPORT_POINTS
	Vec3V m_AVerts[3];
	Vec3V m_BVerts[3];
#endif
	Vec3V m_supportDir;

	GJK_VERTEX_ID m_AVertInds[3];
	GJK_VERTEX_ID m_BVertInds[3];

	// In-place list params.
	GJKCache * m_next;
	
	// AVL tree params.
	GJKCacheKey m_key;
	GJKCache * m_left;
	GJKCache * m_right;
	s8 m_balance; 

	GJK_FLAG_TYPE m_flags;
	u8 m_VertCount;

#if __ASSERT
	u8 m_boundTypeA : 4;
	u8 m_boundTypeB : 4;
	void SetBoundTypes(const int boundTypeA, const int boundTypeB)
	{
		FastAssert(boundTypeA <= 0xF);
		FastAssert(boundTypeB <= 0xF);
		m_boundTypeA = boundTypeA;
		m_boundTypeB = boundTypeB;
	}
	void VerifyBoundTypes(const int boundTypeA, const int boundTypeB)
	{
		FastAssert(m_boundTypeA == boundTypeA);
		FastAssert(m_boundTypeB == boundTypeB);
	}
#endif // __ASSERT

	enum
	{
		FLAG_TOUCHED = (1 << 0),
		FLAG_SUPPORT_DIR_VALID = (1 << 1),
		FLAG_IS_ADDED_TO_MAP = (1 << 2),
	};

	__forceinline u32 get_flag(const u32 mask_) const 
	{
		return m_flags & mask_;
	}

	__forceinline void set_flag_true(const u32 mask_)
	{
		m_flags |= mask_;
	}

	__forceinline void set_flag_false(const u32 mask_)
	{
		m_flags &= ~mask_;
	}

#if __ASSERT
	__forceinline void init(const GJKCacheKey & key, const int boundTypeA, const int boundTypeB)
#else // __ASSERT
	__forceinline void init(const GJKCacheKey & key)
#endif // __ASSERT
	{
#if __ASSERT
		SetBoundTypes(boundTypeA,boundTypeB);
#endif // __ASSERT
		m_VertCount = 0;
		m_flags = 0;
#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
		set_flag_true(FLAG_TOUCHED);
#else // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
		set_flag_true(FLAG_TOUCHED|FLAG_IS_ADDED_TO_MAP);
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
		m_key = key;
	}

	__forceinline void SetNotTouched()
	{
		set_flag_false(FLAG_TOUCHED);
	}

	__forceinline void Touch()
	{
		set_flag_true(FLAG_TOUCHED);
	}

	__forceinline u32 IsTouched() const
	{
		return get_flag(FLAG_TOUCHED);
	}

	__forceinline u32 IsSupportDirValid() const
	{
		return get_flag(FLAG_SUPPORT_DIR_VALID);
	}

	__forceinline void SetSupportDir(Vec3V_In supportDir)
	{
		m_supportDir = supportDir;
		set_flag_true(FLAG_SUPPORT_DIR_VALID);
	}

	__forceinline Vec3V_Out GetSupportDir() const
	{
		FastAssert(IsSupportDirValid());
		return m_supportDir;
	}

	__forceinline void InvalidateSupportDir()
	{
		set_flag_false(FLAG_SUPPORT_DIR_VALID);
	}

	__forceinline u32 IsAddedToMap() const
	{
		return get_flag(FLAG_IS_ADDED_TO_MAP);
	}

	__forceinline void SetAddedToMap()
	{
		set_flag_true(FLAG_IS_ADDED_TO_MAP);
	}

	__forceinline int IsSimplexValid() const
	{
		FastAssert(m_VertCount <= 3);
		return (m_VertCount > 0);
	}

	__forceinline void InvalidateSimplex()
	{
		m_VertCount = 0;
	}
};

#if 0
class SlotAllocator
{
public:
	struct node
	{
		node * m_next;
	};

	SlotAllocator() : m_buf(NULL), m_first(NULL) 
	{
	}

	SlotAllocator(const int slot_size, const int slot_alignment, const int slot_count) : m_buf(NULL) 
	{ 
		AllocateSlotBuffer(slot_size,slot_alignment,slot_count); 
	}

	~SlotAllocator() 
	{ 
		FreeSlotBuffer(); 
	}

	char * m_buf;
	int m_slot_size;
	int m_slot_alignment;
	int m_slot_count;
	node * m_first;

	void AllocateSlotBuffer(const int slot_size, const int slot_alignment, const int slot_count)
	{
		FastAssert(m_buf == NULL);
		m_slot_size = slot_size;
		m_slot_alignment = slot_alignment;
		m_slot_count = slot_count;
		m_buf = rage_aligned_new(m_slot_alignment) char[m_slot_size * m_slot_count];
		FastAssert(m_buf != NULL);
		InitSlots();
	}

	void FreeSlotBuffer()
	{
		delete [] m_buf;
		m_buf = NULL;
	}

	void InitSlots()
	{
		m_first = reinterpret_cast<node*>(m_buf+0);
		for (int i = 0 ; i < m_slot_count - 1 ; i++)
		{
			node * cur = reinterpret_cast<node*>(m_buf + m_slot_size * i);
			node * next = reinterpret_cast<node*>(m_buf + m_slot_size * (i + 1));
			cur->m_next = next;
		}
		node * last = reinterpret_cast<node*>(m_buf + m_slot_size * (m_slot_count - 1));
		last->m_next = NULL;
	}

#if __ASSERT
	void verify(void * ptr)
	{
		ptrdiff_t index = (char*)ptr - m_buf;
		FastAssert(ptr);
		FastAssert(index % m_slot_size == 0);
		FastAssert(index >= 0);
		FastAssert(index / m_slot_size < m_slot_count);
		// TODO: Add check for double deletion.
	}
#endif

	void * alloc()
	{
		FastAssert(m_buf);
		if (Likely(m_first))
		{
			void * ptr = m_first;
			m_first = m_first->m_next;
			return ptr;
		}
		else
			return NULL;
	}

	void free(void * ptr)
	{
		FastAssert(m_buf);
		if (Likely(ptr))
		{
			ASSERT_ONLY(verify(ptr);)
			node * cur = reinterpret_cast<node*>(ptr);
			cur->m_next = m_first;
			m_first = cur;
		}
	}
};
#endif // #if 0

#if USE_FRAME_PERSISTENT_GJK_CACHE
#if 0
template <class Allocator, class T> class GJKObjectAllocator
{
public:
	Allocator m_allocator;

	GJKObjectAllocator(int numObjects) : m_allocator(numObjects)
	{
	}

	T * Allocate()
	{
		void * ptr = m_allocator.alloc();
		if (Likely(ptr))
			new(ptr) T;
		return ptr;
	}

	void Free(T * ptr)
	{
		if (Likely(ptr))
		{
			ptr->~T();
			m_allocator.free(ptr);
		}
	}
};
#endif // #if 0

template <class T> class GJKObjectAllocator
{
public:

	class T_internal : public T
	{
	public:
		T_internal() {}
		void AboutToBeReleased() {}
		void Reset() {}
	};

	phPool<T_internal> m_pool;

#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
	
	T * AllocateSPU(T_internal ** Pool_EA)
	{
		return m_pool.AllocateSPU(Pool_EA);
	}

#else // USE_SPU_FRAME_PERSISTENT_GJK_CACHE

	GJKObjectAllocator(int numObjects) : m_pool(numObjects)
	{
	}

	T * Allocate()
	{
		// phPool calls the default constructor.
		return m_pool.Allocate();
	}

	void Free(T * ptr)
	{
		if (Likely(ptr))
		{
			// phPool doesn't call the destructor.
			T_internal * ptr_ = reinterpret_cast<T_internal*>(ptr);
			ptr_->~T_internal();
			m_pool.Release(ptr_);
		}
	}

#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
};

#if USE_PS3_FRAME_PERSISTENT_GJK_CACHE
class GJKCacheElement
{
public:
	GJKCache * m_cache;
	GJKCacheKey m_key;
};
#endif // USE_PS3_FRAME_PERSISTENT_GJK_CACHE

class GJKCacheSystem
{
public:

	enum { GJK_CACHE_DB_SIZE = 512 };
	enum { GJK_CACHE_SIZE = 512 };

	GJKObjectAllocator<GJKCacheDB> m_scdb_allocator;
	GJKObjectAllocator<GJKCache> m_sc_allocator;

#if USE_PS3_FRAME_PERSISTENT_GJK_CACHE
	ALIGNAS(16) GJKCacheElement m_cacheElementList[GJK_CACHE_SIZE] ;
	int m_cacheElementListCount;
#endif // USE_PS3_FRAME_PERSISTENT_GJK_CACHE

#if GJK_CACHE_MULTITHREADED
	
	volatile GJKCacheDB * m_SCDBList;
	volatile u32 m_SCDBListCount;

#else // GJK_CACHE_MULTITHREADED

	GJKCacheDB * m_SCDBList;
	u32 m_SCDBListCount;

#endif // GJK_CACHE_MULTITHREADED

	void SCDBListAdd(GJKCacheDB * gjkCacheDB_EA, GJKCacheDB * gjkCacheDB);
	void SCDBListRemove(GJKCacheDB *);

#if !__SPU
	GJKCacheSystem() : m_sc_allocator(GJK_CACHE_SIZE), m_scdb_allocator(GJK_CACHE_DB_SIZE)
	{
		m_SCDBList = NULL;
		m_SCDBListCount = 0;
	}
#endif // !__SPU
};

struct GJKCacheAvlTreeAccessor
{
	typedef GJKCache* NID;	// Node ID type. Could be and index, pointer, etc.
	typedef GJKCacheKey KT;	// Key type.
	typedef s8 BT;			// Balance type.

	static __forceinline NID null() { return NULL; }
	static __forceinline NID & get_left(NID & nid) { return nid->m_left; }
	static __forceinline NID & get_right(NID & nid) { return nid->m_right; }
	static __forceinline BT & get_bal(NID & nid) { return nid->m_balance; }
	static __forceinline const KT & get_key(const NID & nid) { return nid->m_key; }
	static __forceinline void set_key(NID & /*nid*/, const KT & /*key*/) { /* key already set */ }//{ nid->key = key; }
	static __forceinline int cmp(const KT & k1, const KT & k2) { return (k1 < k2); }
	static __forceinline int equ(const KT & k1, const KT & k2) { return (k1 == k2); }
	static __forceinline void prefetch_find(NID & nid) { PrefetchObject<GJKCache>(nid); }
	static __forceinline void prefetch_insert(NID & /*nid*/) { /*PrefetchObject<GJKCache>(nid);*/ }
	static __forceinline void prefetch_remove(NID & nid) { PrefetchObject<GJKCache>(nid); }

};

typedef avl_tree<GJKCacheAvlTreeAccessor> GJKCacheMap;

template<class T> __forceinline void GJKListAddMT(volatile T ** list, T * item_EA, T * item)
{
#if !__SPU
	FastAssert(item_EA == item);
#endif
	FastAssert(item_EA);
	FastAssert(item);
	for (;;)
	{
#if __SPU
		const int TAG_ID = 17;
		T * saveList = reinterpret_cast<T*>(cellDmaGetUint32((uint64_t)list, DMA_TAG(TAG_ID), 0, 0));
#else
		T * saveList = (T*)*list;
#endif
		item->SetNext(saveList);
		if (sysInterlockedCompareExchangePointer((void**)list,*&item_EA,*&saveList) == *&saveList)
			break;
	}
}

template<class T> __forceinline void GJKListAdd(T ** list, T * item)
{
	FastAssert(item);
	item->SetNext(*list);
	*list = item;
}

#if GJK_CACHE_MULTITHREADED
	
__forceinline void GJKCacheSystem::SCDBListAdd(GJKCacheDB * gjkCacheDB_EA, GJKCacheDB * gjkCacheDB)
{
	GJKListAddMT(&m_SCDBList,gjkCacheDB_EA,gjkCacheDB);
	sysInterlockedIncrement(&m_SCDBListCount);
}

__forceinline void GJKCacheSystem::SCDBListRemove(GJKCacheDB *)
{
	sysInterlockedDecrement(&m_SCDBListCount);
}

#else // GJK_CACHE_MULTITHREADED

__forceinline void GJKCacheSystem::SCDBListAdd(GJKCacheDB * cache)
{
	GJKListAdd(&m_SCDBList,cache);
	m_SCDBListCount++;
}

__forceinline void GJKCacheSystem::SCDBListRemove(GJKCacheDB *)
{
	m_SCDBListCount--;
}

#endif // GJK_CACHE_MULTITHREADED

GJKCacheSystem * CreateGJKCacheSystem()
{
	return rage_aligned_new(__alignof(GJKCacheSystem)) GJKCacheSystem();
}

void DestroyGJKCacheSystem(GJKCacheSystem * gjkCacheSystem)
{
	if (gjkCacheSystem)
		delete gjkCacheSystem;
}

#else // USE_FRAME_PERSISTENT_GJK_CACHE

class GJKCacheSystem
{
};

GJKCacheSystem * CreateGJKCacheSystem()
{
	return NULL;
}

void DestroyGJKCacheSystem(GJKCacheSystem *)
{
}

#endif // USE_FRAME_PERSISTENT_GJK_CACHE

#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
ALIGNAS(16) GJKCacheElement g_cacheElementList[GJKCacheSystem::GJK_CACHE_SIZE] ;
int g_cacheElementListCount;

GJKCacheSystem * g_gjkCacheSystem_EA;
GJKObjectAllocator<GJKCacheDB>::T_internal ** g_gjkCacheDB_Pool_EA;
GJKObjectAllocator<GJKCache>::T_internal ** g_gjkCache_Pool_EA;
void SpuInitGjkCacheSystem(const PairListWorkUnitInput & wui)
{
	g_gjkCacheSystem_EA = wui.m_gjkCacheSystem_EA;
	g_gjkCacheDB_Pool_EA = g_gjkCacheSystem_EA->m_scdb_allocator.m_pool.GetPool_EA();
	g_gjkCache_Pool_EA = g_gjkCacheSystem_EA->m_sc_allocator.m_pool.GetPool_EA();
#if __BANK
	g_UseGJKCache = wui.m_UseGJKCache;
	g_UseFramePersistentGJKCache = wui.m_UseFramePersistentGJKCache;
#endif // __BANK

	const int TAG_ID = 17;
	g_cacheElementListCount = cellDmaGetUint32((uint64_t)&g_gjkCacheSystem_EA->m_cacheElementListCount, DMA_TAG(TAG_ID), 0, 0);
	const int dmaCount = g_cacheElementListCount + (g_cacheElementListCount & 1);
	FastAssert(dmaCount >= 0 && dmaCount <= GJKCacheSystem::GJK_CACHE_SIZE);
	const int dmaSize = sizeof(GJKCacheElement)*dmaCount;
	FastAssert((dmaSize & 0xF) == 0);
	sysDmaGet(g_cacheElementList,(uint64_t)&g_gjkCacheSystem_EA->m_cacheElementList,dmaSize,DMA_TAG(TAG_ID));
	sysDmaWait(DMA_MASK(TAG_ID));
}
ALIGNAS(16) u8 g_gjkCacheDBBuffer[sizeof(GJKCacheDB)] ;
GJKCacheDB * g_gjkCacheDB_EA;
ALIGNAS(16) u8 g_gjkCacheBuffer[sizeof(GJKCache)] ;
GJKCache * g_gjkCache_EA;

inline GJKCache * QueryCacheElementList(int start_i, int end_i, const GJKCacheKey & key)
{
	while (start_i <= end_i)
	{
		const int mid_i = (start_i + end_i) >> 1;
		const GJKCacheElement & ce = g_cacheElementList[mid_i];
		if (key < ce.m_key)
			end_i = mid_i - 1;
		else if (key > ce.m_key)
			start_i = mid_i + 1;
		else
		{
			const int TAG_ID = 17;
			sysDmaGet(g_gjkCacheBuffer,(uint64_t)ce.m_cache,sizeof(GJKCache),DMA_TAG(TAG_ID));
			sysDmaWait(DMA_MASK(TAG_ID));
			return ce.m_cache;
		}
	}
	return NULL;
}
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE

GJKCacheDB * GetGJKCacheDB(GJKCacheSystem * CCD_RESTRICT gjkCacheSystem, phManifold * CCD_RESTRICT manifold)
{
#if USE_FRAME_PERSISTENT_GJK_CACHE
#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE

	GJKCacheDB * gjkCacheDB = reinterpret_cast<GJKCacheDB*>(g_gjkCacheDBBuffer);
	GJKCacheDB * gjkCacheDB_EA = manifold->GetGJKCacheDB();
	if (Likely(gjkCacheDB_EA))
	{
		const int CACHEDB_TAG = 1;
		sysDmaGet(g_gjkCacheDBBuffer, (uint64_t)manifold->GetGJKCacheDB(), sizeof(GJKCacheDB), DMA_TAG(CACHEDB_TAG));
		sysDmaWait(DMA_MASK(CACHEDB_TAG));
	}
	else
	{
		gjkCacheDB_EA = gjkCacheSystem->m_scdb_allocator.AllocateSPU(g_gjkCacheDB_Pool_EA);
		if (Likely(gjkCacheDB_EA))
		{
			new ((void*)gjkCacheDB) GJKCacheDB;
			gjkCacheSystem->SCDBListAdd(gjkCacheDB_EA,gjkCacheDB);
			manifold->SetGJKCacheDB(gjkCacheDB_EA);
		}
		else
			gjkCacheDB = NULL;
	}
	g_gjkCacheDB_EA = gjkCacheDB_EA;
#if __ASSERT
	if (Likely(gjkCacheDB))
		gjkCacheDB->m_inCollision = true;
#endif // __ASSERT
	return gjkCacheDB;
	
#else // USE_SPU_FRAME_PERSISTENT_GJK_CACHE

	GJKCacheDB * gjkCacheDB = manifold->GetGJKCacheDB();
	if (Unlikely(gjkCacheDB == NULL))
	{
		GJK_PF_INCREMENT(GJKCacheDBCreateCount);
		gjkCacheDB = gjkCacheSystem->m_scdb_allocator.Allocate();
		if (Likely(gjkCacheDB))
		{
			gjkCacheSystem->SCDBListAdd(gjkCacheDB,gjkCacheDB);
			manifold->SetGJKCacheDB(gjkCacheDB);
		}
	}
#if __ASSERT
	if (Likely(gjkCacheDB))
		gjkCacheDB->m_inCollision = true;
#endif // __ASSERT
	return gjkCacheDB;

#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
#else // USE_FRAME_PERSISTENT_GJK_CACHE
	(void)gjkCacheSystem;
	(void)manifold;
	return NULL;
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

void GJKCacheQueryInput::SetCacheDatabase(const bool useGjkCache, GJKCacheSystem * CCD_RESTRICT gjkCacheSystem, phManifold * CCD_RESTRICT rootManifold)
{
#if __BANK
	if (Likely(useGjkCache && g_UseGJKCache && g_UseFramePersistentGJKCache))
#else // __BANK
	if (Likely(useGjkCache))
#endif // __BANK
	{
		m_gjkCacheSystem = gjkCacheSystem;
		m_gjkCacheDB = GetGJKCacheDB(gjkCacheSystem,rootManifold);
#if __ASSERT
		if (Likely(m_gjkCacheDB))
			FastAssert(m_gjkCacheDB->m_delete == false);
#endif // __ASSERT
	}
	else
	{
		m_gjkCacheSystem = NULL;
		m_gjkCacheDB = NULL;
	}
}

#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE || __ASSERT
GJKCacheQueryInput::~GJKCacheQueryInput()
{
	if (Likely(m_gjkCacheDB))
	{
#if __ASSERT
	m_gjkCacheDB->m_inCollision = false;
#endif // __ASSERT
#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
		const int TAG_ID = 17;
		sysDmaPut(m_gjkCacheDB,(uint64_t)g_gjkCacheDB_EA,sizeof(GJKCacheDB),DMA_TAG(TAG_ID));
		sysDmaWait(DMA_MASK(TAG_ID));
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
	}
}
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE

GJKCacheDB::GJKCacheDB()
{
#if USE_FRAME_PERSISTENT_GJK_CACHE
	m_root = GJKCacheAvlTreeAccessor::null();
	m_list = GJKCacheAvlTreeAccessor::null();
	m_delete = false;
#if __ASSERT
	m_inCollision = false;
#endif // __ASSERT
	FastAssert(sizeof(m_root) >= sizeof(GJKCacheAvlTreeAccessor::NID));
#if USE_PS3_FRAME_PERSISTENT_GJK_CACHE
	m_cacheIndexStart = 0;
	m_cacheIndexEnd = 0;
#endif // USE_PS3_FRAME_PERSISTENT_GJK_CACHE
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

GJKCacheDB::~GJKCacheDB() 
{
	//FlushCache(); 
	//FastAssert(m_root == NULL);
	//FastAssert(m_list == NULL);
	//FastAssert(m_delete == true);
}

void GJKCacheDB::FlushCache(GJKCacheSystem * gjkCacheSystem)
{
	PPC_STAT_COUNTER_INC(GJKCacheDBFlushCounter,1);
#if USE_FRAME_PERSISTENT_GJK_CACHE && !__SPU
	GJKCache * cur = reinterpret_cast<GJKCache*>(m_list);
	PrefetchBuffer<sizeof(GJKCache)>(cur);
	while (cur)
	{
		GJKCache * next = cur->m_next;
		PrefetchBuffer<sizeof(GJKCache)>(next);
		gjkCacheSystem->m_sc_allocator.Free(cur);
		cur = next;
	}
	m_root = GJKCacheAvlTreeAccessor::null();
	m_list = NULL;
#else // USE_FRAME_PERSISTENT_GJK_CACHE
	(void)gjkCacheSystem;
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

#if __ASSERT
__forceinline GJKCache * GJKCacheDB::GetGJKCache(GJKCacheSystem * gjkCacheSystem, const GJKCacheKey & key, const int boundTypeA, const int boundTypeB)
#else // __ASSERT
__forceinline GJKCache * GJKCacheDB::GetGJKCache(GJKCacheSystem * gjkCacheSystem, const GJKCacheKey & key)
#endif // __ASSERT
{
#if USE_FRAME_PERSISTENT_GJK_CACHE
#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
	GJKCache * gjkCache = reinterpret_cast<GJKCache*>(g_gjkCacheBuffer);
/*	
	GJKCache * gjkCache_EA = m_root;
	//const int TAG_ID = 17;
	while (gjkCache_EA)
	{
		sysDmaGet(g_gjkCacheBuffer,(uint64_t)gjkCache_EA,sizeof(GJKCache),DMA_TAG(TAG_ID));
		sysDmaWait(DMA_MASK(TAG_ID));
		if (GJKCacheAvlTreeAccessor::equ(key,GJKCacheAvlTreeAccessor::get_key(gjkCache)))
			break;
		else if (GJKCacheAvlTreeAccessor::cmp(key,GJKCacheAvlTreeAccessor::get_key(gjkCache)))
			gjkCache_EA = GJKCacheAvlTreeAccessor::get_left(gjkCache);
		else
		{
			AVL_ASSERT(GJKCacheAvlTreeAccessor::cmp(GJKCacheAvlTreeAccessor::get_key(gjkCache),key),"invalid key comparison logic.");
			gjkCache_EA = GJKCacheAvlTreeAccessor::get_right(gjkCache);
		}
	}
*/
	GJKCache * gjkCache_EA = QueryCacheElementList(m_cacheIndexStart,m_cacheIndexEnd-1,key);
	if (Likely(gjkCache_EA))
	{
		gjkCache->Touch();
	}
	else
	{
		gjkCache_EA = gjkCacheSystem->m_sc_allocator.AllocateSPU(g_gjkCache_Pool_EA);
		if (Likely(gjkCache_EA))
		{
			new ((void*)gjkCache) GJKCache;
#if __ASSERT
			gjkCache->init(key,boundTypeA,boundTypeB);
#else // __ASSERT
			gjkCache->init(key);
#endif // __ASSERT

			//cache_map.insert(gjkCache,key);
			//m_root = cache_map.get_root();
			// we'll add this cache to the map later on the ppu.

			gjkCache->m_next = m_list;
			m_list = gjkCache_EA;
		}
		else
			gjkCache = NULL;
	}
	g_gjkCache_EA = gjkCache_EA;
#else // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
	GJKCacheMap cache_map(m_root);
	GJKCache * gjkCache = cache_map.find(key);
	if (Likely(gjkCache))
	{
		gjkCache->Touch();
	}
	else
	{
		// TODO: if necessary, properly handle swapping of pair elements.
		GJKCache * gjkCache = gjkCacheSystem->m_sc_allocator.Allocate();
		if (Likely(gjkCache))
		{
			GJK_PF_INCREMENT(GJKCacheCreateCount);
			//PPC_STAT_COUNTER_INC(GJKCacheCreateCounter,1);
#if __ASSERT
			gjkCache->init(key,boundTypeA,boundTypeB);
#else // __ASSERT
			gjkCache->init(key);
#endif // __ASSERT

			cache_map.insert(gjkCache,key);
			m_root = cache_map.get_root();

			gjkCache->m_next = m_list;
			m_list = gjkCache;
		}
	}
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
#if __ASSERT
	if (Likely(gjkCache))
	{
		FastAssert(gjkCache->IsTouched());
		gjkCache->VerifyBoundTypes(boundTypeA,boundTypeB);
	}
#endif
	return gjkCache;
#else // USE_FRAME_PERSISTENT_GJK_CACHE
#if __ASSERT
	(void)boundTypeA;
	(void)boundTypeB;
#endif // __ASSERT
	(void)gjkCacheSystem;
	(void)key;
	return NULL;
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

void GJKCacheDB::UpdateCache(GJKCacheSystem * gjkCacheSystem)
{
	PPC_STAT_COUNTER_INC(GJKCacheDBUpdateCounter,1);
#if USE_FRAME_PERSISTENT_GJK_CACHE && !__SPU
	GJKCacheMap cache_map(m_root);
	GJKCache ** cur_i = &m_list;
	PrefetchObject<GJKCache>(*cur_i);
	while (*cur_i)
	{
		//PPC_STAT_COUNTER_INC(GJKCacheCounter,1);
		GJKCache * cur = *cur_i;
		FastAssert(cur->IsAddedToMap());
		if (Unlikely(cur->IsTouched() == 0))
		{
			*cur_i = cur->m_next;
			PrefetchObject<GJKCache>(*cur_i);

			cache_map.remove(cur->m_key);
			gjkCacheSystem->m_sc_allocator.Free(cur);
			//PPC_STAT_COUNTER_INC(GJKCacheDestroyCounter,1);
		}
		else
		{
			cur_i = &cur->m_next;
			PrefetchObject<GJKCache>(*cur_i);

			cur->SetNotTouched();
			GJK_PF_INCREMENT(GJKCacheCount);
		}
	}
	m_root = cache_map.get_root();
#else // USE_FRAME_PERSISTENT_GJK_CACHE
	(void)gjkCacheSystem;
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

void GJKCacheSystemPostCollisionUpdate(GJKCacheSystem * gjkCacheSystem)
{
	PPC_STAT_TIMER_SCOPED(GJKCacheSystemUpdateTimer);
	PPC_STAT_COUNTER_INC(GJKCacheSystemUpdateCounter,1);
	GJK_PF_FUNC(GJKCacheSytemPostCollisionUpdate);

#if USE_FRAME_PERSISTENT_GJK_CACHE && !__SPU
#if 0
	{
		int cache_count = 0;
		btBroadphasePair * prunedPairs = PHLEVEL->GetBroadPhase()->getPrunedPairs();
		const int prunedPairCount = PHLEVEL->GetBroadPhase()->getPrunedPairCount();

		// Go over all the pairs of objects overlapping in the broadphase
		for( int pairIndex = 0; pairIndex < prunedPairCount; ++pairIndex )
		{
			btBroadphasePair * pair = prunedPairs + pairIndex;
			if (pair->GetManifold())
			{
				GJKCacheDB * cur = pair->GetManifold()->m_cache_ptr.get_ptr();
				if (cur)
				{
					cur->UpdateCache(gjkCacheSystem);
					cache_count++;
				}
			}
		}
		FastAssert(cache_count == gjkCacheSystem->m_SCDBListCount);
		GJK_PF_INCREMENTBY(GJKCacheDBCount,cache_count);
	}
#else

#if __ASSERT
	u32 cache_count = 0;
#endif
	GJKCacheDB ** cur_i = (GJKCacheDB**)&gjkCacheSystem->m_SCDBList;
	PrefetchObject<GJKCacheDB>(*cur_i);
	while (*cur_i)
	{	
		GJKCacheDB * cur = *cur_i;
		if (Likely(cur->m_delete == false))
		{
			cur_i = &cur->m_next;
			PrefetchObject<GJKCacheDB>(*cur_i);

			cur->UpdateCache(gjkCacheSystem);
#if __ASSERT
			cache_count++;
#endif
		}
		else
		{
			*cur_i = cur->m_next;
			PrefetchObject<GJKCacheDB>(*cur_i);

			cur->FlushCache(gjkCacheSystem);
			gjkCacheSystem->SCDBListRemove(cur);
			gjkCacheSystem->m_scdb_allocator.Free(cur);
		}
	}
#if __ASSERT
	FastAssert(cache_count == gjkCacheSystem->m_SCDBListCount);
#endif
	GJK_PF_INCREMENTBY(GJKCacheDBCount,gjkCacheSystem->m_SCDBListCount);

#endif

#else // USE_FRAME_PERSISTENT_GJK_CACHE
	(void)gjkCacheSystem;
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

CompileTimeAssert(phBound::NUM_BOUND_TYPES <= 32);
const int USE_PERSISTANT_CACHE_FLAGS = (1 << phBound::BOX) | (1 << phBound::TRIANGLE) | (1 << phBound::CAPSULE) | (1 << phBound::SPHERE) | (1 << phBound::GEOMETRY);
__forceinline int UseFramePersistentCache(const phBound * CCD_RESTRICT boundA, const phBound * CCD_RESTRICT boundB)
{
#if __BANK
	if (Unlikely(!(g_UseFramePersistentGJKCache && g_UseGJKCache)))
		return false;
#endif
#if STORE_SIMPLEX_SUPPORT_POINTS
	(void)boundA;
	(void)boundB;
	return true;
#else // STORE_SIMPLEX_SUPPORT_POINTS
	const int fAB = (1 << boundA->GetType()) | (1 << boundB->GetType());
	//return (fAB & USE_PERSISTANT_CACHE_FLAGS) == fAB;
	return ~GenerateMaskNE(fAB,fAB & USE_PERSISTANT_CACHE_FLAGS);
#endif // STORE_SIMPLEX_SUPPORT_POINTS
}

#if USE_FRAME_PERSISTENT_GJK_CACHE
#if __ASSERT
__forceinline GJKCache * GetGJKCache(GJKCacheQueryInput * gjkCacheInfo, const int boundTypeA, const int boundTypeB)
#else // __ASSERT
__forceinline GJKCache * GetGJKCache(GJKCacheQueryInput * gjkCacheInfo)
#endif // __ASSERT
{
	FastAssert(gjkCacheInfo);
	if (Likely(gjkCacheInfo->m_gjkCacheDB))
	{
		FastAssert(gjkCacheInfo->m_gjkCacheDB->m_delete == false);
		const GJKCacheKey key = MakeGJKCacheKey(gjkCacheInfo->m_ComponentA,gjkCacheInfo->m_ComponentB,gjkCacheInfo->m_PartIndex);//,0);
#if __ASSERT
		return gjkCacheInfo->m_gjkCacheDB->GetGJKCache(gjkCacheInfo->m_gjkCacheSystem,key,boundTypeA,boundTypeB);
#else // __ASSERT
		return gjkCacheInfo->m_gjkCacheDB->GetGJKCache(gjkCacheInfo->m_gjkCacheSystem,key);
#endif // __ASSERT
	}
	else
		return NULL;
}

static const VecBoolV BOX_VERTS[8] = 
{
						//zyx
	VecBoolV(V_F_F_F_F),//000
	VecBoolV(V_T_F_F_F),//001
	VecBoolV(V_F_T_F_F),//010
	VecBoolV(V_T_T_F_F),//011
	VecBoolV(V_F_F_T_F),//100
	VecBoolV(V_T_F_T_F),//101
	VecBoolV(V_F_T_T_F),//110
	VecBoolV(V_T_T_T_F),//111
};

#define COPY_CI(i,min_ind,max_ind) FastAssert(cachedInd##i >= min_ind && cachedInd##i < max_ind); inds[i] = cachedInd##i
void SeedSimplex(const phBound * CCD_RESTRICT bound, Vec3V * CCD_RESTRICT verts, int * CCD_RESTRICT inds, const GJK_VERTEX_ID * CCD_RESTRICT cachedInds, const int numVerts)
{
#if USE_FULL_SIMPLEX_COPY
	(void)numVerts;
#endif
	const int cachedInd0 = cachedInds[0];
	const int cachedInd1 = cachedInds[1];
	const int cachedInd2 = cachedInds[2];
	//inds[0] = cachedInd0;
	//inds[1] = cachedInd1;
	//inds[2] = cachedInd2;
	switch(bound->GetType())
	{
	case phBound::BOX:
		{
			const phBoundBox * CCD_RESTRICT box = static_cast<const phBoundBox *>(bound);
			const Vec3V margin(box->GetMarginV());
			const Vec3V BBMin = box->GetBoundingBoxMin() + margin;
			const Vec3V BBMax = box->GetBoundingBoxMax() - margin;
#if USE_FULL_SIMPLEX_COPY
			verts[2] = SelectFT(BOX_VERTS[cachedInd2], BBMin, BBMax);
			COPY_CI(2,0,8);
			verts[1] = SelectFT(BOX_VERTS[cachedInd1], BBMin, BBMax);
			COPY_CI(1,0,8);
			verts[0] = SelectFT(BOX_VERTS[cachedInd0], BBMin, BBMax);
			COPY_CI(0,0,8);
#else // USE_FULL_SIMPLEX_COPY
			switch(numVerts)
			{
			case 3:
				verts[2] = SelectFT(BOX_VERTS[cachedInd2], BBMin, BBMax);
				COPY_CI(2,0,8);
			case 2:
				verts[1] = SelectFT(BOX_VERTS[cachedInd1], BBMin, BBMax);
				COPY_CI(1,0,8);
			case 1:
				verts[0] = SelectFT(BOX_VERTS[cachedInd0], BBMin, BBMax);
				COPY_CI(0,0,8);
			}
#endif // USE_FULL_SIMPLEX_COPY
		}
		break;
	case phBound::TRIANGLE:
		{
			const TriangleShape * CCD_RESTRICT tri = static_cast<const TriangleShape *>(bound);
#if USE_FULL_SIMPLEX_COPY
			verts[2] = tri->m_vertices1[cachedInd2];
			COPY_CI(2,0,3);
			verts[1] = tri->m_vertices1[cachedInd1];
			COPY_CI(1,0,3);
			verts[0] = tri->m_vertices1[cachedInd0];
			COPY_CI(0,0,3);
#else // USE_FULL_SIMPLEX_COPY
			switch(numVerts)
			{
			case 3:
				verts[2] = tri->m_vertices1[cachedInd2];
				COPY_CI(2,0,3);
			case 2:
				verts[1] = tri->m_vertices1[cachedInd1];
				COPY_CI(1,0,3);
			case 1:
				verts[0] = tri->m_vertices1[cachedInd0];
				COPY_CI(0,0,3);
			}
#endif // USE_FULL_SIMPLEX_COPY
		}
		break;
	case phBound::CAPSULE:
		{
			const phBoundCapsule * CCD_RESTRICT cap = static_cast<const phBoundCapsule *>(bound);
			const Vec3V end = Vec3V(cap->GetHalfLengthV()) & Vec3V(V_MASKY);
			const Vec3V center = cap->GetCentroidOffset();
			const Vec3V CAP_VERTS[2] = {center-end,center+end};
#if USE_FULL_SIMPLEX_COPY
			verts[2] = CAP_VERTS[cachedInd2];
			COPY_CI(2,0,2);
			verts[1] = CAP_VERTS[cachedInd1];
			COPY_CI(1,0,2);
			verts[0] = CAP_VERTS[cachedInd0];
			COPY_CI(0,0,2);
#else // USE_FULL_SIMPLEX_COPY
			switch(numVerts)
			{
			case 3:
				verts[2] = CAP_VERTS[cachedInd2];
				COPY_CI(2,0,2);
			case 2:
				verts[1] = CAP_VERTS[cachedInd1];
				COPY_CI(1,0,2);
			case 1:
				verts[0] = CAP_VERTS[cachedInd0];
				COPY_CI(0,0,2);
			}
#endif // USE_FULL_SIMPLEX_COPY
		}
		break;
	case phBound::SPHERE:
		{
			const phBoundSphere * CCD_RESTRICT sph = static_cast<const phBoundSphere *>(bound);
			const Vec3V center = sph->GetCentroidOffset();
#if USE_FULL_SIMPLEX_COPY
			verts[2] = center;
			COPY_CI(2,0,1);
			verts[1] = center;
			COPY_CI(1,0,1);
			verts[0] = center;
			COPY_CI(0,0,1);
#else // USE_FULL_SIMPLEX_COPY
			switch(numVerts)
			{
			case 3:
				verts[2] = center;
				COPY_CI(2,0,1);
			case 2:
				verts[1] = center;
				COPY_CI(1,0,1);
			case 1:
				verts[0] = center;
				COPY_CI(0,0,1);
			}
#endif // USE_FULL_SIMPLEX_COPY
		}
		break;
	case phBound::GEOMETRY:
		{
			const phBoundPolyhedron * CCD_RESTRICT geom = static_cast<const phBoundPolyhedron *>(bound);
#if COMPRESSED_VERTEX_METHOD == 0
			const Vec3V * vertices = geom->GetShrunkVertexPointer();
			if (Unlikely(vertices == NULL))
				vertices = geom->GetVertexPointer();

#if USE_FULL_SIMPLEX_COPY
			verts[2] = vertices[cachedInd2];
			COPY_CI(2,0,geom->GetNumConvexHullVertices());
			verts[1] = vertices[cachedInd1];
			COPY_CI(1,0,geom->GetNumConvexHullVertices());
			verts[0] = vertices[cachedInd0];
			COPY_CI(0,0,geom->GetNumConvexHullVertices());
#else // USE_FULL_SIMPLEX_COPY
			switch(numVerts)
			{
			case 3:
				verts[2] = vertices[cachedInd2];
				COPY_CI(2,0,geom->GetNumConvexHullVertices());
			case 2:
				verts[1] = vertices[cachedInd1];
				COPY_CI(1,0,geom->GetNumConvexHullVertices());
			case 1:
				verts[0] = vertices[cachedInd0];
				COPY_CI(0,0,geom->GetNumConvexHullVertices());
			}
#endif // USE_FULL_SIMPLEX_COPY

#else // COMPRESSED_VERTEX_METHOD == 0			
			const CompressedVertexType * compressedVertices = geom->GetShrunkVertexPointer();
			if (Unlikely(compressedVertices == NULL))
				compressedVertices = geom->GetCompressedVertexPointer();

#if USE_FULL_SIMPLEX_COPY
			verts[2] = geom->DecompressVertex(compressedVertices + 3 * cachedInd2);
			COPY_CI(2,0,geom->GetNumConvexHullVertices());
			verts[1] = geom->DecompressVertex(compressedVertices + 3 * cachedInd1);
			COPY_CI(1,0,geom->GetNumConvexHullVertices());
			verts[0] = geom->DecompressVertex(compressedVertices + 3 * cachedInd0);
			COPY_CI(0,0,geom->GetNumConvexHullVertices());
#else // USE_FULL_SIMPLEX_COPY
			switch(numVerts)
			{
			case 3:
				verts[2] = geom->DecompressVertex(compressedVertices + 3 * cachedInd2);
				COPY_CI(2,0,geom->GetNumConvexHullVertices());
			case 2:
				verts[1] = geom->DecompressVertex(compressedVertices + 3 * cachedInd1);
				COPY_CI(1,0,geom->GetNumConvexHullVertices());
			case 1:
				verts[0] = geom->DecompressVertex(compressedVertices + 3 * cachedInd0);
				COPY_CI(0,0,geom->GetNumConvexHullVertices());
			}
#endif // USE_FULL_SIMPLEX_COPY 

#endif // COMPRESSED_VERTEX_METHOD == 0
		}
		break;
	default:
		Assertf(0,"Invalid bound passed to SeedSimplex:%d",bound->GetType());
	}
}
#endif // USE_FRAME_PERSISTENT_GJK_CACHE


void UpdateGJKCacheProlog(VoronoiSimplexSolver * CCD_RESTRICT simplexSolver, GJKCacheQueryInput * CCD_RESTRICT gjkCacheQI, const phBound * CCD_RESTRICT boundA, const phBound * CCD_RESTRICT boundB)
{
	GJK_PF_FUNC(GJKCacheQueryProlog);
	FastAssert(simplexSolver);
#if USE_FRAME_PERSISTENT_GJK_CACHE
	//if (Likely(gjkCacheQI))
	FastAssert(gjkCacheQI);
	{
		if (UseFramePersistentCache(boundA,boundB))
		{
#if __ASSERT
			GJKCache * gjkCache = GetGJKCache(gjkCacheQI,boundA->GetType(),boundB->GetType());
#else //__ASSERT
			GJKCache * gjkCache = GetGJKCache(gjkCacheQI);
#endif // __ASSERT
			if (Likely(gjkCache))
			{
				simplexSolver->m_hasCache = true;
				gjkCacheQI->m_gjkCache = gjkCache;
				if (gjkCache->IsSimplexValid())
				{
					GJK_PF_INCREMENT(GJKCache_CachedSimplexCount);

#if __ASSERT && USE_FULL_SIMPLEX_COPY
					switch (gjkCache->m_VertCount)
					{
						case 1:
							FastAssert(gjkCache->m_AVertInds[1] == 0);
							FastAssert(gjkCache->m_BVertInds[1] == 0);
						case 2:
							FastAssert(gjkCache->m_AVertInds[2] == 0);
							FastAssert(gjkCache->m_BVertInds[2] == 0);
					}
#endif // __ASSERT

#if STORE_SIMPLEX_SUPPORT_POINTS
					#define LOOP_(vert_i) \
					{ \
					simplexSolver->m_LocalA[vert_i] = gjkCache->m_AVerts[vert_i]; \
					simplexSolver->m_LocalB[vert_i] = gjkCache->m_BVerts[vert_i]; \
					simplexSolver->m_VertexIndexA[vert_i] = gjkCache->m_AVertInds[vert_i]; \
					simplexSolver->m_VertexIndexB[vert_i] = gjkCache->m_BVertInds[vert_i]; \
					}

#if USE_FULL_SIMPLEX_COPY
					LOOP_(2);
					LOOP_(1);
					LOOP_(0);
#else // USE_FULL_SIMPLEX_COPY
					switch(gjkCache->m_VertCount)
					{
					case 3:
						LOOP_(2);
					case 2:
						LOOP_(1);
					case 1:
						LOOP_(0);
					}
#endif // USE_FULL_SIMPLEX_COPY
					#undef LOOP_

#else // STORE_SIMPLEX_SUPPORT_POINTS

					SeedSimplex(boundA,simplexSolver->m_LocalA,simplexSolver->m_VertexIndexA,gjkCache->m_AVertInds,gjkCache->m_VertCount);
					SeedSimplex(boundB,simplexSolver->m_LocalB,simplexSolver->m_VertexIndexB,gjkCache->m_BVertInds,gjkCache->m_VertCount);

#endif // STORE_SIMPLEX_SUPPORT_POINTS

					simplexSolver->m_numVertices = gjkCache->m_VertCount;
				}
				else if (Likely(gjkCache->IsSupportDirValid()))
				{	
					GJK_PF_INCREMENT(GJKCache_CachedSupportDirCount);
					simplexSolver->m_numVertices = 0;
					simplexSolver->SetCachedV(gjkCache->GetSupportDir());
				}
				else
				{
					//simplexSolver->m_numVertices = 0;
					//simplexSolver->SetCachedV(CachedSeparatingAxisDefault);
					simplexSolver->m_hasCache = false;
				}
				return;
			}
		}
		gjkCacheQI->m_gjkCache = NULL;
	}
	FastAssert(simplexSolver->m_hasCache == false);
#else // USE_FRAME_PERSISTENT_GJK_CACHE
	(void)simplexSolver;
	(void)boundA;
	(void)boundB;
	if (Likely(gjkCacheQI))
		gjkCacheQI->m_gjkCache = NULL;
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

const float SUPPORT_DIR_MIN_LENGTH = 0.0001f;
const ScalarV SUPPORT_DIR_MIN_LENGTH_V(ScalarVFromF32(SUPPORT_DIR_MIN_LENGTH));
const ScalarV SUPPORT_DIR_MIN_LENGTH_SQ_V(ScalarVFromF32(SUPPORT_DIR_MIN_LENGTH * SUPPORT_DIR_MIN_LENGTH));

void UpdateGJKCacheEpilog(VoronoiSimplexSolver * CCD_RESTRICT simplexSolver, GJKCacheQueryInput * CCD_RESTRICT gjkCacheQI)
{
	GJK_PF_FUNC(GJKCacheQueryEpilog);
#if USE_FRAME_PERSISTENT_GJK_CACHE
	//if (Likely(gjkCacheQI))
	FastAssert(gjkCacheQI);
	{
		if (Likely(gjkCacheQI->m_gjkCache))
		{
			GJKCache * gjkCache = gjkCacheQI->m_gjkCache;
			if (!simplexSolver->IsSeparated() && simplexSolver->IsSimplexValid())
			{
				FastAssert(simplexSolver->IsSimplexValid());

#if STORE_SIMPLEX_SUPPORT_POINTS
				#define LOOP_(vert_i) \
				{ \
				gjkCache->m_AVerts[vert_i] = simplexSolver->m_LocalA[vert_i]; \
				gjkCache->m_BVerts[vert_i] = simplexSolver->m_LocalB[vert_i]; \
				FastAssert(simplexSolver->m_VertexIndexA[vert_i] >= 0 && simplexSolver->m_VertexIndexA[vert_i] <= (int)GJK_VERTEX_ID_MAX); \
				FastAssert(simplexSolver->m_VertexIndexB[vert_i] >= 0 && simplexSolver->m_VertexIndexB[vert_i] <= (int)GJK_VERTEX_ID_MAX); \
				gjkCache->m_AVertInds[vert_i] = (GJK_VERTEX_ID)simplexSolver->m_VertexIndexA[vert_i]; \
				gjkCache->m_BVertInds[vert_i] = (GJK_VERTEX_ID)simplexSolver->m_VertexIndexB[vert_i]; \
				} 
#else // STORE_SIMPLEX_SUPPORT_POINTS
				#define LOOP_(vert_i) \
				{ \
				FastAssert(simplexSolver->m_VertexIndexA[vert_i] >= 0 && simplexSolver->m_VertexIndexA[vert_i] <= (int)GJK_VERTEX_ID_MAX); \
				FastAssert(simplexSolver->m_VertexIndexB[vert_i] >= 0 && simplexSolver->m_VertexIndexB[vert_i] <= (int)GJK_VERTEX_ID_MAX); \
				gjkCache->m_AVertInds[vert_i] = (GJK_VERTEX_ID)simplexSolver->m_VertexIndexA[vert_i]; \
				gjkCache->m_BVertInds[vert_i] = (GJK_VERTEX_ID)simplexSolver->m_VertexIndexB[vert_i]; \
				} 
#endif // STORE_SIMPLEX_SUPPORT_POINTS

				gjkCache->m_VertCount = (s8)simplexSolver->m_numVertices;
				switch(simplexSolver->m_numVertices)
				{
				case 3:
					LOOP_(2);
				case 2:
					LOOP_(1);
				case 1:
					LOOP_(0);
				}
				#undef LOOP_

#if USE_FULL_SIMPLEX_COPY
				// Set the unused vertices to zero. This is so we can more efficiently initialize the simplex on the next frame.
				#define LOOP_(vert_i) gjkCache->m_AVertInds[vert_i] = 0; gjkCache->m_BVertInds[vert_i] = 0;
				switch(simplexSolver->m_numVertices)
				{
					case 1:
						LOOP_(1);
					case 2:
						LOOP_(2);
				}
				#undef LOOP_
#endif // USE_FULL_SIMPLEX_COPY
			}
			else
			{
				gjkCache->InvalidateSimplex();
			}	

			{
#if 1
				// TODO: GJK or the penetration solver should always return a non zero support direction.
				const ScalarV ncachedV_sq = MagSquared(simplexSolver->m_cachedV);
				if (Likely(IsGreaterThanAll(ncachedV_sq,SUPPORT_DIR_MIN_LENGTH_SQ_V)))
					gjkCache->SetSupportDir(simplexSolver->m_cachedV);
				else
					gjkCache->InvalidateSupportDir();
#else // #if 0/1
#if __ASSERT
				const float THRESH = 0.0001f;
				const float THRESH_SQ = THRESH * THRESH;
				const ScalarV ncachedV_sq = MagSquared(simplexSolver->m_cachedV);
				FastAssert(IsTrue(ncachedV_sq > ScalarV(THRESH_SQ)));
#endif // __ASSERT
				gjkCache->SetSupportDir(simplexSolver->m_cachedV);
#endif // #if 0/1
			}
#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
			const int TAG_ID = 17;
			sysDmaPut(g_gjkCacheBuffer,(uint64_t)g_gjkCache_EA,sizeof(GJKCache),DMA_TAG(TAG_ID));
			sysDmaWait(DMA_MASK(TAG_ID));
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE
		}
	}
#else // USE_FRAME_PERSISTENT_GJK_CACHE
#if __ASSERT
	if (Likely(gjkCacheQI))
		FastAssert(gjkCacheQI->m_gjkCache == NULL);
#endif // __ASSERT
	(void)simplexSolver;
	(void)gjkCacheQI;
#endif // USE_FRAME_PERSISTENT_GJK_CACHE
}

#if USE_PPU_FRAME_PERSISTENT_GJK_CACHE

GJKCacheElement * CCD_RESTRICT g_tempList;
int g_tempListCount;
void InitCacheElementListRecurse(GJKCache * CCD_RESTRICT root)
{
	if (root)
	{
		InitCacheElementListRecurse(root->m_left);
		GJKCacheElement * ce = g_tempList + g_tempListCount;
		g_tempListCount++;
		ce->m_cache = root;
		ce->m_key = root->m_key;
		InitCacheElementListRecurse(root->m_right);
	}
}

void GJKCacheCollisionProlog(GJKCacheSystem * CCD_RESTRICT gjkCacheSystem)
{
	g_tempListCount = 0;
	g_tempList = gjkCacheSystem->m_cacheElementList;
	GJKCacheDB * gjkCacheDB = (GJKCacheDB*)gjkCacheSystem->m_SCDBList;
	while (gjkCacheDB)
	{
		gjkCacheDB->m_cacheIndexStart = g_tempListCount;
		InitCacheElementListRecurse(gjkCacheDB->m_root);
		gjkCacheDB->m_cacheIndexEnd = g_tempListCount;
		gjkCacheDB = gjkCacheDB->m_next;
	}
	gjkCacheSystem->m_cacheElementListCount = g_tempListCount;
}


void GJKCacheCollisionEpilog(GJKCacheSystem * CCD_RESTRICT gjkCacheSystem)
{
	GJKCacheDB * gjkCacheDB = (GJKCacheDB*)gjkCacheSystem->m_SCDBList;
	while (gjkCacheDB)
	{
		GJKCacheMap cache_map(gjkCacheDB->m_root);
		GJKCache * gjkCache = gjkCacheDB->m_list;
		while (gjkCache && (gjkCache->IsAddedToMap() == 0))
		{
			gjkCache->SetAddedToMap();
			cache_map.insert(gjkCache,gjkCache->m_key);
			gjkCache = gjkCache->m_next;
		}
		gjkCacheDB->m_root = cache_map.get_root();
		gjkCacheDB = gjkCacheDB->m_next;
	}
}

#endif // USE_PPU_FRAME_PERSISTENT_GJK_CACHE

#endif // USE_GJK_CACHE

#if USE_FRAME_PERSISTENT_GJK_CACHE
bool GetGJKCacheSeparatingDirection(const DiscreteCollisionDetectorInterface::ClosestPointInput & input, Vec3V_InOut sepAxis, ScalarV_InOut sepDist)
{
	if (input.m_gjkCache)
	{
		const GJKCache * gjkCache = reinterpret_cast<const GJKCache *>(input.m_gjkCache);
		if (gjkCache->IsSupportDirValid())
		{
			const Vec3V cachedSupportDir = gjkCache->GetSupportDir();
			const ScalarV ncachedSupportDirSq = MagSquared(cachedSupportDir);
			Assert(IsGreaterThanOrEqualAll(ncachedSupportDirSq,SUPPORT_DIR_MIN_LENGTH_SQ_V));
			const Vec3V axis = cachedSupportDir / Sqrt(ncachedSupportDirSq);

			const Vec3V seperatingAxisInA = UnTransform3x3Ortho(input.m_transformA,-axis);
			const Vec3V seperatingAxisInB = UnTransform3x3Ortho(input.m_transformB,axis);

#if VERIFY_SUPPORT_FUNCTIONS
			VerifyBoundSupportFunction(input.m_boundA,seperatingAxisInA);
			VerifyBoundSupportFunction(input.m_boundB,seperatingAxisInB);
#endif // VERIFY_SUPPORT_FUNCTIONS

			const Vec3V pInA = input.m_boundA->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInA.GetIntrin128());
			const Vec3V qInB = input.m_boundB->LocalGetSupportingVertexWithoutMarginNotInlined(seperatingAxisInB.GetIntrin128());
			const Vec3V pWorld = Transform3x3(input.m_transformA, pInA);
			const Vec3V qWorld = Transform3x3(input.m_transformB, qInB);

			const Vec3V separationOffset = input.m_transformA.GetCol3() - input.m_transformB.GetCol3();

			const Vec3V w = (qWorld - pWorld) - separationOffset;
			const ScalarV delta = Dot(axis, w);
			const ScalarV dist = Max(delta + PENETRATION_CHECK_EXTRA_MARGIN_SV, ScalarV(V_ZERO));

			sepAxis = axis;
			sepDist = dist;

			return true;
		}
	}
	return false;
}
#endif // USE_FRAME_PERSISTENT_GJK_CACHE

} // namespace rage
