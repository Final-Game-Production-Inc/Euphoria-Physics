// 
// grcore/effectcache.cpp 
// 
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved. 
// 

#include "effectcache.h"

#include "effect.h"

#include "atl/simplecache.h"
#include "math/amath.h"

using namespace rage;

#if __SPU

#if USE_PACKED_GCMTEX
#else
struct TexturePage
{
	uint32_t addresses[32];		// fill an entire cache line
};

atSimpleCache4<TexturePage> s_texturePageCache;

uint32_t GetTextureAddr(const void *ppuAddr)
{
	// Mask off least significant bits
	uint32_t ppuAligned = (uint32_t)ppuAddr & ~(sizeof(TexturePage)-1);
	uint32_t ppuOffset = ((uint32_t)ppuAddr & (sizeof(TexturePage)-1)) >> 2;
	TexturePage *page = s_texturePageCache.Get((TexturePage*)ppuAligned);
	uint32_t result = page->addresses[ppuOffset];

#if __ASSERT
	// changed assert temporarily to get more info on bs#444782, bs#415343
	if (result - 1 < 65535)
	{
		grcDisplayf("Attempting to transfer a texture that was just deleted (result=0x%x ppuAligned=0x%x ppuOffset=%u)", result, ppuAligned, ppuOffset);
		grcDisplayf("page=0x%x 0=0x%x 1=0x%x 2=0x%x 3=0x%x 4=0x%x 5=0x%x 6=0x%x 7=0x%x", (uint32_t)page, page->addresses[0], page->addresses[1], page->addresses[2], page->addresses[3], page->addresses[4], page->addresses[5], page->addresses[6], page->addresses[7]);
		Assert(0);
	}
#endif
	//AssertMsg(result == 0 || result > 65535,"Attempting to transfer a texture that was just deleted.");
	return result;
}

#if __SPU && __BANK
// NOTE: temporary debug data to gather info on deleted textures
// before crashing on drawable spu
bool DbgGetTextureAddr(const void *ppuAddr)
{
	// Mask off least significant bits
	uint32_t ppuAligned = (uint32_t)ppuAddr & ~(sizeof(TexturePage)-1);
	uint32_t ppuOffset = ((uint32_t)ppuAddr & (sizeof(TexturePage)-1)) >> 2;
	TexturePage *page = s_texturePageCache.Get((TexturePage*)ppuAligned);
	uint64_t result = page->addresses[ppuOffset];

	if (result - 1 < 65535)
	{
		grcDisplayf("Attempting to transfer a texture that was just deleted (result=0x%llx ppuAligned=0x%x ppuOffset=%u)", result, ppuAligned, ppuOffset);
		grcDisplayf("page=0x%x 0=0x%x 1=0x%x 2=0x%x 3=0x%x 4=0x%x 5=0x%x 6=0x%x 7=0x%x", (uint32_t)page, page->addresses[0], page->addresses[1], page->addresses[2], page->addresses[3], page->addresses[4], page->addresses[5], page->addresses[6], page->addresses[7]);
		return false;
	}
	return true;
}
#endif

#endif		// USE_PACKED_GCMTEX

void grcEffectCache::Init(u32 cacheSize)
{
	Assert(spuScratch != NULL);
	Assert(spuScratch + cacheSize < spuScratchTop);
	m_Storage = spuScratch;
	m_EndOfStorage = spuScratch + cacheSize;
	spuScratch += cacheSize;
	m_NumGets = 0;
	m_NumEffects = 0;
	for(int i = 0; i < MAX_EFFECTS; i++)
	{	
		m_CacheEntries[i].Reset();
	}
#if CACHE_STATS
	m_NumHits = 0;
	m_NumMisses = 0;
	m_Flushes = 0;
	m_FullFlushes = 0;
	m_NumFlushed = 0;
	m_MaxEntriesUsed = 0;
	m_BiggestEntry = 0;
	m_SizeAccumulator = 0;
#endif
}

void grcEffectCache::Finalize()
{
#if CACHE_STATS && PRINT_CACHE_STATS
	if (m_NumGets != 0)
	{
		grcDisplayf("h: %d, m: %d, hr: %f, fa: %d, max#: %d, maxSize: %d", 
			m_NumHits, m_NumMisses, (float)m_NumHits / (float)m_NumMisses,
			m_FullFlushes, m_MaxEntriesUsed, m_BiggestEntry);
	}
#endif
}

grcEffect* grcEffectCache::Get(void* ppuPtr)
{
	Assert(m_NumEffects <= MAX_EFFECTS);
	using namespace rage;
	if ((u32)ppuPtr < 256*1024)
		return reinterpret_cast<grcEffect*>(ppuPtr);

	// If its in the cache already, return it.
	m_NumGets++;
	for(int i = 0; i < m_NumEffects; i++)
	{
		CacheEntry& entry = m_CacheEntries[i];
		if (ppuPtr == entry.m_PpuEffectPtr)
		{
			CACHE_STATS_ONLY(m_NumHits++);
			CACHE_STATS_ONLY(m_SizeAccumulator += m_NumEffects);
			Assert(entry.m_EffectPtr);
			entry.m_LastGet = m_NumGets;
			return entry.m_EffectPtr;
		}
	}

	CACHE_STATS_ONLY(m_NumMisses++);

	// Not in the cache yet, first find out how big it's going to be
	LoadEffectHeader(ppuPtr);
	u32 size = SizeFromHeader();

#if 0
	// Simple (grow-only) cache algorithm:
	// If we are using MAX_EFFECTs or don't have room at the end of the cache:
	//		Flush the cache
	// Insert this effect at the end

	if (m_NumEffects == MAX_EFFECTS || size > SpaceRemaining())
	{
		FlushAll();
	}
	return Append(ppuPtr).m_EffectPtr;
#endif

	// More complex reuse algorithm:
	// If there is an empty slot where this one will fit, put it there
	// else if we are using less than MAX_EFFECTS and have room at the end of the cache:
	//		Insert this effect at the end
	//	Else
	//		Look for the oldest cache entry with room
	//		If found
	//			Insert the effect there
	//		Else
	//			Flush the whole cache (more complicated: flush N cache entries until we have room?)
	int emptyIndex = FindEmpty(size);
	if (emptyIndex >= 0)
	{
		return Replace(emptyIndex, ppuPtr).m_EffectPtr;
	}
	else if (m_NumEffects < MAX_EFFECTS && size <= SpaceRemaining())
	{
		return Append(ppuPtr).m_EffectPtr;
	}
	else
	{
		int oldestItem = -1;
		int oldestCount = 0x7fffffff;
		for(int i = 0; i < m_NumEffects; i++)
		{
			CacheEntry& entry = m_CacheEntries[i];
			if (entry.m_BucketSize >= size && entry.m_LastGet < oldestCount)
			{
				oldestItem = i;
				oldestCount = entry.m_LastGet;
			}
		}
		if (oldestItem >= 0)
		{
			return Replace(oldestItem, ppuPtr).m_EffectPtr;
		}
		else
		{
			FlushAll();
			return Append(ppuPtr).m_EffectPtr;
		}
	}
}

grcEffectCache::CacheEntry& grcEffectCache::Replace(int index, void* ppuPtr)
{
	Assert(m_CacheEntries[index].m_BucketSize > 0);
	Assert(m_CacheEntries[index].m_EffectPtr != NULL);
	u32 size = SizeFromHeader();
	Assert(size <= m_CacheEntries[index].m_BucketSize);

	CacheEntry& entry = m_CacheEntries[index];
	entry.m_PpuEffectPtr = ppuPtr;
	entry.m_LastGet = m_NumGets;
	CACHE_STATS_ONLY(m_SizeAccumulator += m_NumEffects);

	LoadEffect(entry);

	CACHE_STATS_ONLY(m_BiggestEntry = Max(m_BiggestEntry, size));
	CACHE_STATS_ONLY(m_MaxEntriesUsed = Max(m_MaxEntriesUsed, (u32)m_NumEffects));

	return entry;
}

grcEffectCache::CacheEntry& grcEffectCache::Append(void* ppuPtr)
{
	Assert(m_NumEffects < MAX_EFFECTS);
	// Inserting at the end of the list
	u32 size = SizeFromHeader();
#if __ASSERT
	if (SpaceRemaining() < size)
	{
		grcDisplayf("This = %p, size=%d, free=%d", this, SizeFromHeader(), SpaceRemaining());
	}
#endif
	Assert(SpaceRemaining() >= size);

	char* startAddr = m_Storage;
	if (m_NumEffects > 0)
	{
		startAddr = m_CacheEntries[m_NumEffects-1].NextEffectAddr();
	}

	CacheEntry& entry = m_CacheEntries[m_NumEffects];
	Assert(entry.m_EffectPtr == NULL && entry.m_PpuEffectPtr == NULL && entry.m_BucketSize == 0);
	entry.m_PpuEffectPtr = ppuPtr;
	entry.m_BucketSize = size;
	entry.m_EffectPtr = reinterpret_cast<grcEffect*>(startAddr);
	entry.m_LastGet = m_NumGets;
	m_NumEffects++;
	CACHE_STATS_ONLY(m_SizeAccumulator += m_NumEffects);

	LoadEffect(entry);

	CACHE_STATS_ONLY(m_BiggestEntry = Max(m_BiggestEntry, size));
	CACHE_STATS_ONLY(m_MaxEntriesUsed = Max(m_MaxEntriesUsed, (u32)m_NumEffects));

	return entry;
}

void grcEffectCache::MarkDirty(grcEffect* lsPtr)
{
	// Nothing clever, just flush for now.
	FlushAll();

	// Next we could just flush the one entry (when we can handle holes in the list)
#if 0
	Assert(lsPtr < 256*1024);
	int i;
	for(i = 0; i < m_NumEffects; i++)
	{
		if (m_CacheEntries[i].m_EffectPtr == lsPtr)
		{
			Flush(lsPtr);
			return;
		}
	}
	Assert(i != MAX_EFFECTS);
#endif
}

void grcEffectCache::Flush(const void* ppuPtr)
{
	for(int i = 0; i < m_NumEffects; i++)
	{
		if (m_CacheEntries[i].m_PpuEffectPtr == ppuPtr)
		{
			m_CacheEntries[i].m_PpuEffectPtr = NULL;
			m_CacheEntries[i].m_LastGet = 0;
			CACHE_STATS_ONLY(m_NumFlushed++);
			CACHE_STATS_ONLY(m_Flushes++);
		}
	}
}

void grcEffectCache::FlushAll()
{
	Assert(m_NumEffects <= MAX_EFFECTS);
	CACHE_STATS_ONLY(m_FullFlushes++);
	CACHE_STATS_ONLY(m_NumFlushed += m_NumEffects);
	for(int i = 0; i < m_NumEffects; i++) // could probably just reset the first one?
	{
		m_CacheEntries[i].Reset();
	}
	m_NumGets = 0;
	m_NumEffects = 0;
}

void grcEffectCache::LoadEffectHeader(void* ppuPtr)
{
	// Use a fenced get on spuBindTag, since there could still be a put from grcEffect__SetVar_grcTexture in flight.
	char* lsAddr = &m_EffectHeader[0];
	sysDmaLargeGetfAndWait(lsAddr, (u64)ppuPtr, sizeof(grcEffect), spuBindTag);
}

void grcEffectCache::LoadEffect(CacheEntry& entry)
{
	// For now, just reload the header data. Also, mess with the spuScratch pointer so we can just use spuGet_ below.
	char* oldSpuScratch = spuScratch;
	spuScratch = reinterpret_cast<char*>(entry.m_EffectPtr);
	grcEffect* ppuPtr = reinterpret_cast<grcEffect*>(entry.m_PpuEffectPtr);
	AssertVerify(grcEffect::spuGet_(ppuPtr) == entry.m_EffectPtr);

	spuScratch = oldSpuScratch;
}

void grcEffectInstanceLoaderData::Abort()
{
	if (m_State > NONE && m_State != FINISHED)
	{
		// one of the DMAs have been started, wait for it.
		sysDmaWaitTagStatusAll(1<<m_Tag);
	}
	Reset();
}

#if 0
void grcEffectInstanceLoaderData::LoadStep()
{

	if (m_State > NONE && m_State != FINISHED)
	{
		// one of the DMAs have been started, wait for it.
		sysDmaWaitTagStatusAll(1<<m_Tag);
	}

	switch(m_State)
	{
	case NONE:
		sysDmaGet(&m_InstStorage, (uint64_t)m_PpuInst, sizeof(grcInstanceData) /* 16 now */, m_Tag);
		m_State = INSTDATA_STARTED;
		break;
	case INSTDATA_STARTED:
		if (!GetInstance()->TotalSize) {
			GetInstance()->Data = NULL;
			m_State = FINISHED;
			return;
		}
		// Pull over entire payload section
		Assert((GetInstance()->SpuSize) < (int)sizeof(m_Storage));
		sysDmaGet(&m_Storage[0],(uint64_t)GetInstance()->Data,GetInstance()->SpuSize,m_Tag);
		m_State = PAYLOAD_STARTED;
		break;

	case PAYLOAD_STARTED:
		{
			// payload just finished
			int orig = (int)GetInstance()->Data;
			GetInstance()->Data = (grcInstanceData::Entry*) (&m_Storage[0]);
			// to avoid unnecessarily breaking texture resources, fudge the base pointer a bit.
			m_NextTexture = (&m_Storage[0] + (GetInstance()->SpuSize));

			int fixup = (int)GetInstance()->Data - orig;

			m_TextureCount = 0;

			for (int i=0; i<GetInstance()->Count; i++) {
				if (GetInstance()->Data[i].Count) {
					GetInstance()->Data[i].Any = (char*)GetInstance()->Data[i].Any + fixup;
					Assert(GetInstance()->Data[i].Any > &m_Storage[0] && GetInstance()->Data[i].Any < m_NextTexture);
		#if DEBUG_GET
					grcDisplayf("parameter %d points to %p",i,GetInstance()->Data[i].Any);
		#endif
				}
				// The fact that it's nonzero is enough for us, we don't care about the original grcTexture being stored here.
				else if (GetInstance()->Data[i].Texture) {
					sysDmaGet(m_NextTexture, GetTextureAddr(GetInstance()->Data[i].Texture), SPU_TEXTURE_SIZE, m_Tag);
					GetInstance()->Data[i].Texture = (grcTexture*) m_NextTexture;
					m_NextTexture += SPU_TEXTURE_SIZE;
				}
			}

			m_State = TEXTURE_POINTERS_STARTED;
			break;
		}

	case TEXTURE_POINTERS_STARTED:
		m_State = FINISHED;
		break;

	case FINISHED:
		break;
	}

}
#endif	/// 0

#endif // __SPU
