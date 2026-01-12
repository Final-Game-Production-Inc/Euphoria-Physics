#ifndef GJK_SIMPLEX_CACHE_H
#define GJK_SIMPLEX_CACHE_H

#include "phcore/constants.h"

#define GJK_COLLISION_DEBUG 0

#if GJK_COLLISION_DEBUG
	//#pragma optimize("",off) //PRAGMA-OPTIMIZE-ALLOW
	#define GJK_COLLISION_OPTIMIZE_OFF() PRAGMA_OPTIMIZE_OFF() //PRAGMA-OPTIMIZE-ALLOW
#else
	#define GJK_COLLISION_OPTIMIZE_OFF()
#endif

#define USE_GJK_CACHE 1

#if __PS3
	#define USE_FRAME_PERSISTENT_GJK_CACHE USE_NEW_SIMPLEX_SOLVER	// This should only be set to 1 if USE_GJK_CACHE is also 1.
#else
	#define USE_FRAME_PERSISTENT_GJK_CACHE 1	// This should only be set to 1 if USE_GJK_CACHE is also 1.
#endif

#define USE_SPU_FRAME_PERSISTENT_GJK_CACHE (__SPU && USE_FRAME_PERSISTENT_GJK_CACHE)
#define USE_PPU_FRAME_PERSISTENT_GJK_CACHE (__PPU && USE_FRAME_PERSISTENT_GJK_CACHE)
#define USE_PS3_FRAME_PERSISTENT_GJK_CACHE (__PS3 && USE_FRAME_PERSISTENT_GJK_CACHE)

#define GJK_CACHE_MULTITHREADED 1

struct PairListWorkUnitInput;

namespace rage 
{

#if USE_GJK_CACHE

class phManifold;
class GJKCache;
class phBound;
class VoronoiSimplexSolver;
class GJKCacheSystem;

GJKCacheSystem * CreateGJKCacheSystem();
void DestroyGJKCacheSystem(GJKCacheSystem * gjkCacheSystem);

typedef u32 GJKCacheKey;

class GJKCacheDB
{
public:
	GJKCache * m_root;
	GJKCache * m_list;
	GJKCacheDB * m_next;
#if USE_PS3_FRAME_PERSISTENT_GJK_CACHE
	u16 m_cacheIndexStart;
	u16 m_cacheIndexEnd;
#endif // USE_PS3_FRAME_PERSISTENT_GJK_CACHE
	bool m_delete;
#if __ASSERT
	bool m_inCollision;
#endif // __ASSERT

	GJKCacheDB();
	~GJKCacheDB();

	void SetDelete()
	{
#if __ASSERT
		FastAssert(m_inCollision == false);
#endif // __ASSERT
		m_delete = true;
	}
	__forceinline void SetNext(GJKCacheDB * next) { m_next = next; }
#if __ASSERT
	GJKCache * GetGJKCache(GJKCacheSystem * gjkCacheSystem, const GJKCacheKey & key, const int boundTypeA, const int boundTypeB);
#else // __ASSERT
	GJKCache * GetGJKCache(GJKCacheSystem * gjkCacheSystem, const GJKCacheKey & key);
#endif // __ASSERT
	void UpdateCache(GJKCacheSystem * gjkCacheSystem);
	void FlushCache(GJKCacheSystem * gjkCacheSystem);
} ;

class GJKCacheQueryInput
{
public:
#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE || __ASSERT
	GJKCacheQueryInput() { m_gjkCacheDB = NULL; }
	~GJKCacheQueryInput();
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE

	GJKCacheSystem * m_gjkCacheSystem;
	GJKCacheDB * m_gjkCacheDB;
	GJKCache * m_gjkCache;
	u32 m_ComponentA;
	u32 m_ComponentB;
	u32 m_PartIndex;

	void SetCacheDatabase(const bool useGjkCache, GJKCacheSystem * CCD_RESTRICT gjkCacheSystem, phManifold * CCD_RESTRICT rootManifold);

	void SetComponentIndex(const u32 ComponentA, const u32 ComponentB)
	{
		FastAssert(ComponentA <= 0xFF);
		FastAssert(ComponentB <= 0xFF);
		m_ComponentA = ComponentA;
		m_ComponentB = ComponentB;
	}

	void SetPartIndex(const u32 PartIndex)
	{
		FastAssert(PartIndex <= 0xFFFF);
		m_PartIndex = PartIndex;
	}
};

#if __BANK
extern bool g_UseFramePersistentGJKCache;
extern bool g_UseGJKCache;
#endif // __BANK

void UpdateGJKCacheProlog(VoronoiSimplexSolver * CCD_RESTRICT simplexSolver, GJKCacheQueryInput * CCD_RESTRICT gjkCacheQI, const phBound * CCD_RESTRICT boundA, const phBound * CCD_RESTRICT boundB);
void UpdateGJKCacheEpilog(VoronoiSimplexSolver * CCD_RESTRICT simplexSolver, GJKCacheQueryInput * CCD_RESTRICT gjkCacheQI);
void GJKCacheSystemPostCollisionUpdate(GJKCacheSystem * gjkCacheSystem);

#if USE_SPU_FRAME_PERSISTENT_GJK_CACHE
void SpuInitGjkCacheSystem(const PairListWorkUnitInput & wui);
#endif // USE_SPU_FRAME_PERSISTENT_GJK_CACHE

#if USE_PPU_FRAME_PERSISTENT_GJK_CACHE
void GJKCacheCollisionProlog(GJKCacheSystem * CCD_RESTRICT gjkCacheSystem);
void GJKCacheCollisionEpilog(GJKCacheSystem * CCD_RESTRICT gjkCacheSystem);
#endif // USE_PPU_FRAME_PERSISTENT_GJK_CACHE

#else //USE_GJK_CACHE

class phManifold;
class phBound;
class GJKCacheQueryInput
{
public:
	void SetCacheDatabase(const bool, GJKCacheSystem*, phManifold*) {}
	void SetComponentIndex(u32, u32) {}
	void SetPartIndex(u32) {}
};

#endif //USE_GJK_CACHE

} // namespace rage

#endif // GJK_SIMPLEX_CACHE_H
