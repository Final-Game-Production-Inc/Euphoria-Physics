// 
// physics/sleepmgr.cpp 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#include "sleepmgr.h"
#include "system/timemgr.h"
#include "grcore/font.h"

#include "levelnew.h"

namespace rage {

phSleepMgr::phSleepMgr(size_t bufferSize, u32 maxIslands)
	: m_ActiveIslands(rage_new ActiveIsland[maxIslands])
	, m_IslandBuffer(bufferSize, maxIslands + 1)
	, m_UnusedIndices(rage_new u16[maxIslands])
	, m_NumUnusedIndices(maxIslands)
	, m_MaxIslands(maxIslands)
#if PH_SLEEP_DEBUG
	, m_DebugMgr(NULL)
#endif // PH_SLEEP_DEBUG
{
	for (u16 index = 0; index < maxIslands; ++index)
	{
		m_UnusedIndices[index] = index;
	}
}

phSleepMgr::~phSleepMgr()
{
	delete [] m_UnusedIndices;
	delete [] m_ActiveIslands;
	PH_SLEEP_DEBUG_ONLY(delete m_DebugMgr);
}

#if __PFDRAW
EXT_PFD_DECLARE_ITEM(SleepIslands);
PH_SLEEP_DEBUG_ONLY(EXT_PFD_DECLARE_ITEM(SleepDebug);)

void phSleepMgr::ProfileDraw()
{
	if (PFD_SleepIslands.Begin())
	{
		for (u32 islandIndex = 0; islandIndex < m_MaxIslands; ++islandIndex)
		{
			ActiveIsland& activeIsland = m_ActiveIslands[islandIndex];
			if (phSleepIsland* island = activeIsland.island)
			{
				island->ProfileDraw();
			}
		}

		PFD_SleepIslands.End();
	}
}
#endif

phSleepIsland* phSleepMgr::AllocateSleepIsland(int numInsts)
{
	if (m_NumUnusedIndices == 0)
	{
		m_IslandBuffer.ReleaseTail();
	}
	Assert(m_NumUnusedIndices > 0);

	if (phSleepIsland* newIsland = m_IslandBuffer.Allocate(numInsts))
	{
		u16 newIndex = GetUnusedIndex();

		ActiveIsland& activeIsland = m_ActiveIslands[newIndex];
		activeIsland.island = newIsland;
		newIsland->SetId(phSleepIsland::Id(newIndex, activeIsland.generationId));

		return newIsland;
	}

	return NULL;
}

void phSleepMgr::InvalidateSleepIsland(u16 islandIndex)
{
	TrapLT(islandIndex, 0);
	TrapGE((u32)islandIndex, m_MaxIslands);

	ActiveIsland& activeIsland = m_ActiveIslands[islandIndex];
	activeIsland.island = NULL;
	++activeIsland.generationId;
	RecycleUnusedIndex(islandIndex);
}

phSleepIsland* phSleepMgr::GetSleepIsland(u16 levelIndex)
{
	return GetSleepIsland(PHLEVEL->GetSleepIsland(levelIndex));
}

phSleepIsland* phSleepMgr::GetSleepIsland(phSleepIsland::Id islandId)
{
	u16 islandIndex = islandId.index;
	if (islandIndex < m_MaxIslands)
	{
		ActiveIsland& activeIsland = m_ActiveIslands[islandIndex];
		if (activeIsland.generationId == islandId.generationId)
		{
			return activeIsland.island;
		}
	}

	return NULL;
}

u16 phSleepMgr::GetUnusedIndex()
{
	TrapLE(m_NumUnusedIndices, (u32)0);
	TrapGT(m_NumUnusedIndices, m_MaxIslands);
	u16 islandIndex = m_UnusedIndices[--m_NumUnusedIndices];

	TrapLT(islandIndex, 0);
	TrapGE((u32)islandIndex, m_MaxIslands);
	++m_ActiveIslands[islandIndex].generationId;

	return islandIndex;
}

void phSleepMgr::RecycleUnusedIndex(u16 islandIndex)
{
	TrapLT(m_NumUnusedIndices, (u32)0);
	TrapGE(m_NumUnusedIndices, m_MaxIslands);

	m_UnusedIndices[m_NumUnusedIndices++] = islandIndex;
}

#if PH_SLEEP_DEBUG
void phSleepMgr::AddDebugRecord(phSleepDebugEvent event, phHandle handle)
{
	if (!m_DebugMgr)
	{
		m_DebugMgr = rage_new phSleepDebugMgr;
	}

	if (PFD_SleepDebug.WillDraw())
	{
		m_DebugMgr->AddDebugRecord(event, handle);
	}
}

void phSleepMgr::RenderDebugRecord(phHandle handle, Vec3V_In position)
{
	if (!m_DebugMgr)
	{
		m_DebugMgr = rage_new phSleepDebugMgr;
	}

	m_DebugMgr->RenderDebugRecord(handle, position);
}

phSleepDebugMgr::phSleepDebugMgr()
{

}

static u32 s_RenderLine;
static Vec3V s_RenderPosition;

void RenderStackTrace(size_t addr, const char* sym, size_t offset)
{
	grcDrawLabelf(RCC_VECTOR3(s_RenderPosition), 0, (s_RenderLine + 1) * grcFont::GetCurrent().GetHeight(), "%8" SIZETFMT "x - %s+%" SIZETFMT "x\n", addr, sym, offset);
	s_RenderLine++;
}

void phSleepDebugMgr::AddDebugRecord(phSleepDebugEvent event, phHandle handle)
{
	Record& record = FindRecord(handle);

	record.event = event;
	sysStack::CaptureStackTrace(record.stack, MAX_STACK_SIZE, 2U);
}

void phSleepDebugMgr::RenderDebugRecord(phHandle handle, Vec3V_In position)
{
	Record& record = FindRecord(handle);
	const char* eventString = "";

	switch (record.event)
	{
	case UNKNOWN:
		eventString = "UNKNOWN";
		break;
	case WAKE:
		eventString = "WAKE";
		break;
	case RESET:
		eventString = "RESET";
		break;
	case BLOCK:
		eventString = "BLOCK";
		break;
	default:
		Assert(false);
		break;
	}

	grcDrawLabelf(RCC_VECTOR3(position), eventString);

	s_RenderLine = 0;
	s_RenderPosition = position;
	sysStack::PrintCapturedStackTrace(record.stack, MAX_STACK_SIZE, &RenderStackTrace);
}

phSleepDebugMgr::Record& phSleepDebugMgr::FindRecord(phHandle handle)
{
	u32 lruFrame = INT_MAX;
	u32 lruIndex = 0;

	for (int recordIndex = 0; recordIndex < MAX_RECORDS; ++recordIndex)
	{
		Record& record = m_Records[recordIndex];

		if (record.lastAccess < lruFrame)
		{
			lruFrame = record.lastAccess;
			lruIndex = recordIndex;
		}
		
		if (record.handle == handle)
		{
			record.lastAccess = TIME.GetFrameCount();

			return record;
		}
	}

	Record& lruRecord = m_Records[lruIndex];
	lruRecord.handle = handle;
	lruRecord.lastAccess = TIME.GetFrameCount();
	lruRecord.event = UNKNOWN;

	sysMemSet(lruRecord.stack, 0, sizeof(lruRecord.stack));

	return lruRecord;
}
#endif // PH_SLEEP_DEBUG

} // namespace rage
