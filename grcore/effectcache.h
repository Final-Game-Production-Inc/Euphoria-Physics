// 
// grcore/effectcache.h 
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#ifndef GRCORE_EFFECTCACHE_H
#define GRCORE_EFFECTCACHE_H

#include "effect.h"

namespace rage
{

#define CACHE_STATS (__DEV || 1)
#define PRINT_CACHE_STATS (0)

#if CACHE_STATS
#define CACHE_STATS_ONLY(x) (x)
#else
#define CACHE_STATS_ONLY(x)
#endif

#if __SPU

// An SPU side cache for grcEffects.
// Could templatize this to make a generic cache for variable-sized data? 
struct ALIGNAS(16) grcEffectCache
{
	grcEffect* Get(void* ppuPtr);
	void MarkDirty(grcEffect* lsPtr); // if the cached data changes and we're doing write-on-flush?
	void Flush(const void* ppuPtr);
	void FlushAll();

	void Init(u32 cacheSize);
	void Finalize();

	// RETURNS: The LS effect pointer if its cached, NULL otherwise
	inline grcEffect* IsCached(const void* ppuPtr);

private:
	static const int MAX_EFFECTS = 8;

	char	m_EffectHeader[sizeof(grcEffect)]; // this needs to be first, so its aligned.

	struct CacheEntry
	{
		grcEffect* m_EffectPtr;
		void* m_PpuEffectPtr;
		u32 m_BucketSize; // note - this is 'allocated' size - may be larger than real effect size
		int m_LastGet;
		void Reset() 
		{
			m_EffectPtr = NULL;
			m_PpuEffectPtr = NULL;
			m_BucketSize = 0;
			m_LastGet = 0;
		}
		char* NextEffectAddr() {return (char*)m_EffectPtr + m_BucketSize;}
	};

	CacheEntry m_CacheEntries[MAX_EFFECTS];
	int			m_NumEffects;
	int			m_NumGets;
	char*		m_Storage;
	char*		m_EndOfStorage;

#if CACHE_STATS
public:
	u32			m_NumHits;
	u32			m_NumMisses;
	u32			m_Flushes;
	u32			m_FullFlushes;
	u32			m_NumFlushed;
	u32			m_MaxEntriesUsed;
	u32			m_BiggestEntry;
	u32			m_SizeAccumulator;
#endif
private:


	grcEffect& GetEffectHeader() {return *reinterpret_cast<grcEffect*>(&m_EffectHeader);}
	u32 SpaceRemaining() {return m_NumEffects ? m_EndOfStorage - m_CacheEntries[m_NumEffects-1].NextEffectAddr() : m_EndOfStorage - m_Storage;}
	void LoadEffectHeader(void* ppuPtr);
	void LoadEffect(CacheEntry& entry);
	u32 SizeFromHeader() {return (sizeof(grcEffect) + GetEffectHeader().m_Container.m_Size + 15) & ~0xf;} // Assumes effect header was just loaded
	CacheEntry& Append(void* ppuPtr); // Assumes effect header was just loaded
	CacheEntry& Replace(int index, void* ppuPtr);
	int FindEmpty(u32 minSize);
} ;

grcEffect* grcEffectCache::IsCached(const void* ppuPtr)
{
	for(int i = 0; i < m_NumEffects; i++)
	{
		if (m_CacheEntries[i].m_PpuEffectPtr == ppuPtr)
		{
			return m_CacheEntries[i].m_EffectPtr;
		}
	}
	return NULL;
}

int grcEffectCache::FindEmpty(u32 minSize)
{
	for(int i = 0; i < m_NumEffects; i++)
	{
		if (m_CacheEntries[i].m_PpuEffectPtr == NULL && m_CacheEntries[i].m_BucketSize >= minSize)
		{
			return i;
		}
	}
	return -1;
}

#define INSTLOADER_SIZE (3 * 1024)
#define NUM_INSTLOADERS 4
CompileTimeAssert(INSTLOADER_SIZE % 16 == 0);

// This is a multi-step loader for grcEffectInstances. Use it to pre-fetch instances by doing non
struct ALIGNAS(16) grcEffectInstanceLoaderData
{
	enum LoadState // i.e. what's been loaded so far
	{
		NONE,
		INSTDATA_STARTED,
		PAYLOAD_STARTED,
		TEXTURE_POINTERS_STARTED,
		FINISHED
	};

	__forceinline void Reset()
	{
		m_State = FINISHED;
		m_PpuInst = NULL;
		m_TextureCount = 0;
	}

	__forceinline void StartLoad(void* ppuInstance)
	{
		Reset();
		m_PpuInst = ppuInstance;
		m_State = NONE;
	}

	void LoadStep();

	__forceinline void FinishLoad()
	{
		while(m_State != FINISHED)
		{
			LoadStep();
		}
	}

	void Abort();

	__forceinline bool IsEmpty() // i.e. not loading anything
	{
		return m_PpuInst == NULL;
	}

	__forceinline void* GetPpuInstance()
	{
		return m_PpuInst;
	}

	__forceinline grcInstanceData* GetInstance() {
		return reinterpret_cast<grcInstanceData*>(&m_InstStorage[0]);
	}

	char m_Storage[INSTLOADER_SIZE]; // needs to be aligned
	char m_InstStorage[sizeof(grcInstanceData)]; // so we don't have to deal with c'tors or d'tors. Also needs to be aligned
	void* m_PpuInst;
	char* m_NextTexture;
	LoadState m_State;
	u32 m_TextureCount;
	u32 m_Tag;
		
} ;


struct grcEffectInstanceLoader
{
	void Init()
	{
		m_LastBootedOut = 0;
		for(int i = 0; i < NUM_INSTLOADERS; i++)
		{
			m_Loaders[i].Reset();
			m_Loaders[i].m_Tag = 10 + i;
		}
	}

	void StartPreloading(void* ppuInstance)
	{
		for(int i = 0; i < NUM_INSTLOADERS; i++)
		{
			// look for an empty loader.
			if (m_Loaders[i].m_PpuInst == ppuInstance)
			{
				return; // already loading
			}
			if (m_Loaders[i].IsEmpty())
			{
				m_Loaders[i].StartLoad(ppuInstance);
				return;
			}
		}
		// nothing eh? fine - kick out anything. keep a counter so we're not alway booting the same thing

		m_Loaders[m_LastBootedOut].Abort();
		m_Loaders[m_LastBootedOut].StartLoad(ppuInstance);
		m_LastBootedOut++;
		m_LastBootedOut %= NUM_INSTLOADERS;
	}

	void StepLoads()
	{
		for(int i = 0; i < NUM_INSTLOADERS; i++)
		{
			m_Loaders[i].LoadStep();
		}
	}

	grcEffectInstanceLoaderData* GetInstance(void* ppuInstance)
	{
		for(int i = 0; i < NUM_INSTLOADERS; i++)
		{
			if (m_Loaders[i].m_PpuInst == ppuInstance)
			{
				m_Loaders[i].FinishLoad();
				return &m_Loaders[i];
			}
		}
		return NULL; // not preloaded 
	}

	grcEffectInstanceLoaderData m_Loaders[NUM_INSTLOADERS];
	int m_LastBootedOut;
};



#endif

}

#endif
