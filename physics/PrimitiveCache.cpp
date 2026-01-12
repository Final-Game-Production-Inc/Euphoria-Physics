#include "PrimitiveCache.h"
#include "phcore/avl_tree.h"
#include "phbound/OptimizedBvh.h"
#include "phbound/boundbvh.h"
#include "system/cache.h"

//PRAGMA_OPTIMIZE_OFF() //PRAGMA-OPTIMIZE-ALLOW

namespace rage
{

__forceinline PrimitiveLeaf::key_t PrimitiveLeaf_KeyMake(const u32 levelIndex, const u16 component, const int partIndex)
{
	FastAssert(levelIndex <= 0xFFFF);// && levelIndex >= 0);
	FastAssert(component <= 0xFF);// && component >= 0);
	FastAssert(partIndex <= 0xFFFF && partIndex >= 0);
	PrimitiveLeaf::key_t k;
	k.a1 = (((u32)levelIndex) << 8) | (u32)component;
	k.a2 = (u32)partIndex;
	return k;
}

__forceinline int PrimitiveLeaf_KeyCmp(const PrimitiveLeaf::key_t & k1, const PrimitiveLeaf::key_t & k2)
{
	return (k1.a1 == k2.a1) ? (k1.a2 < k2.a2) : (k1.a1 < k2.a1);
}

__forceinline int PrimitiveLeaf_KeyEqu(const PrimitiveLeaf::key_t & k1, const PrimitiveLeaf::key_t & k2)
{
	return (k1.a1 == k2.a1 && k1.a2 == k2.a2);
}

struct PrimitiveCacheAvlTreeAccessor
{
	typedef PrimitiveLeaf* NID;			// Node ID type. Could be an index, pointer, etc.
	typedef PrimitiveLeaf::key_t KT;	// Key type.
	typedef char BT;					// Balance type.

	static __forceinline NID null() { return NULL; }
	static __forceinline NID & get_left(NID & nid) { return nid->m_left; }
	static __forceinline NID & get_right(NID & nid) { return nid->m_right; }
	static __forceinline BT & get_bal(NID & nid) { return nid->m_balance; }
	static __forceinline const KT & get_key(const NID & nid) { return nid->m_key; }
	static __forceinline void set_key(NID & /*nid*/, const KT & /*key*/) { /* key already set */ }//{ nid->key = key; }
	static __forceinline int cmp(const KT & k1, const KT & k2) { return PrimitiveLeaf_KeyCmp(k1,k2); }
	static __forceinline int equ(const KT & k1, const KT & k2) { return PrimitiveLeaf_KeyEqu(k1,k2); }
	static __forceinline void prefetch_find(NID & nid) { PrefetchObject<PrimitiveLeaf>(nid); }
	static __forceinline void prefetch_insert(NID & /*nid*/) { /*PrefetchObject<PrimitiveLeaf>(nid);*/ }
	static __forceinline void prefetch_remove(NID & /*nid*/) { /*PrefetchObject<PrimitiveLeaf>(nid);*/ }
};

typedef avl_tree<PrimitiveCacheAvlTreeAccessor> PrimitiveCacheMap_t;

#define PRIM_CACHE_NEW(type) new(Alloc(sizeof(type),__alignof(type))) type

// Simple template meta-program to compute the maximum primitive size as a compile time constant.
template <const int A, const int B> struct GetMaxTM1 { enum { VAL = ((A > B) ? A : B) }; };
const int MAX_PRIMITIVE_SIZE = 
#undef PRIMITIVE_TYPE_INC
#define PRIMITIVE_TYPE_INC(className,enumLabel) GetMaxTM1<sizeof(className),
#include "PrimitiveTypes.inc"
0
#undef PRIMITIVE_TYPE_INC
#define PRIMITIVE_TYPE_INC(className,enumLabel) >::VAL
#include "PrimitiveTypes.inc"
#undef PRIMITIVE_TYPE_INC
;

PrimitiveLeaf * PrimitiveCache::GetLeaf(const phBoundBVH * boundBVH, const phOptimizedBvhNode * curNode, const u32 levelIndex, const u16 component CHECK_TRI_ONLY(const phInst * pInstOfBvh))
{
	// Search for the leaf in the cache map.
	const int startPrimIndex = curNode->GetPolygonStartIndex();
	PrimitiveCacheMap_t primCacheMap(m_root);
	PrimitiveLeaf::key_t key = PrimitiveLeaf_KeyMake(levelIndex,component,startPrimIndex);
	PrimitiveLeaf * leaf = primCacheMap.find(key);
	if (!leaf)
	{
		// The leaf doesn't exist so now we create one.

		const int primCount = curNode->GetPolygonCount();
		FastAssert(primCount > 0);

#if __SPU
		const int PRIMITIVE_TAG_ID = 3;
		const int MAX_NUM_PRIMS = 16;
		FastAssert(primCount <= MAX_NUM_PRIMS);
		u8 primitiveBuffer[sizeof(phPrimitive) * MAX_NUM_PRIMS];	// Assume that they haven't built the bvh with more than 16 primitives per node.
		// Avoid using TAG(1) because that is the manifold contacts and we don't need to wait on them yet
		sysDmaLargeGet(primitiveBuffer, (uint64_t)&boundBVH->GetPrimitive(startPrimIndex), sizeof(phPrimitive) * primCount, DMA_TAG(PRIMITIVE_TAG_ID));
#endif

		// Reset the cache if we don't have enough memory.
		const int ALIGNMENT = 16;
		const int EXTRA = 16;
		const int MAX_ALLOC_SIZE = sizeof(PrimitiveLeaf) + primCount * MAX_PRIMITIVE_SIZE + EXTRA;
		if (!m_buffer.CanAlloc(MAX_ALLOC_SIZE,ALIGNMENT))
			Reset();
		FastAssert(m_buffer.CanAlloc(MAX_ALLOC_SIZE,ALIGNMENT));

		// Create the leaf.
		leaf = PRIM_CACHE_NEW(PrimitiveLeaf);
		leaf->Init(key,boundBVH,curNode CHECK_TRI_ONLY(pInstOfBvh));

		// Insert the leaf into the map.
		primCacheMap.set_root(m_root);
		primCacheMap.insert(leaf,key);
		m_root = primCacheMap.get_root();

		PrimitiveBase ** cur_last = &leaf->m_first;

#if __SPU
		// This is waiting on the primitives
		sysDmaWaitTagStatusAll(DMA_MASK(PRIMITIVE_TAG_ID));
#endif

		// Create the primitives.
		const int endPrimIndex = startPrimIndex + primCount;
		for (int curPrimIndex = startPrimIndex ; curPrimIndex < endPrimIndex ; curPrimIndex++)
		{
#if __SPU
			const phPrimitive &curPrimitive = reinterpret_cast<const phPrimitive&>(primitiveBuffer[sizeof(phPrimitive) * (curPrimIndex-startPrimIndex)]);
#else
			const phPrimitive &curPrimitive = boundBVH->GetPrimitive(curPrimIndex);
#endif
			PrimitiveBase * prim;
			switch(curPrimitive.GetType())
			{
				#undef PRIMITIVE_TYPE_INC
				#define PRIMITIVE_TYPE_INC(className,enumLabel) \
				case enumLabel: \
				{ \
					prim = PRIM_CACHE_NEW(className); \
					prim->InitBase(leaf); \
					((className*)prim)->Init(curPrimitive,curPrimIndex); \
					break; \
				}
				#include "PrimitiveTypes.inc"
				#undef PRIMITIVE_TYPE_INC

				default:
					FastAssert(0);
					prim = NULL;
			}
			FastAssert(prim);
#if STORE_PRIM_TYPE
			prim->m_primType = curPrimitive.GetType();
#endif
			*cur_last = prim;
			cur_last = &prim->m_next;
		}
		*cur_last = NULL;
	}
#if __SPU
	else
	{
		//FastAssert(leaf->m_boundBVH == boundBVH);
		//FastAssert(leaf->m_curNode == curNode);
		leaf->ReInit(boundBVH,curNode);
	}
#endif // __SPU
	FastAssert(leaf);
	return leaf;
}

} // namespace rage