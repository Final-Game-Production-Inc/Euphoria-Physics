// 
// physics/sleepmgr.h 
// 
// Copyright (C) 1999-2009 Rockstar Games.  All Rights Reserved. 
// 

#ifndef PHYSICS_SLEEPMGR_H 
#define PHYSICS_SLEEPMGR_H 

#include "phcore/constants.h"
#include "handle.h"
#include "ringbuffer.h"
#include "sleepisland.h"

#include "grprofile/drawmanager.h"

namespace rage {

#if PH_SLEEP_DEBUG
enum phSleepDebugEvent
{
	UNKNOWN,
	WAKE,
	RESET,
	BLOCK
};
#endif // PH_SLEEP_DEBUG

PH_SLEEP_DEBUG_ONLY(class phSleepDebugMgr;)

class phSleepMgr
{
public:
	phSleepMgr(size_t bufferSize, u32 maxIslands);
	~phSleepMgr();

#if __PFDRAW
	void ProfileDraw();
#endif

	phSleepIsland* AllocateSleepIsland(int numInsts);
	void InvalidateSleepIsland(u16 islandIndex);

	phSleepIsland* GetSleepIsland(u16 levelIndex);

#if PH_SLEEP_DEBUG
	void AddDebugRecord(phSleepDebugEvent event, phHandle handle);
	void RenderDebugRecord(phHandle handle, Vec3V_In position);
#endif

private:
	phSleepIsland* GetSleepIsland(phSleepIsland::Id islandId);

	u16 GetUnusedIndex();
	void RecycleUnusedIndex(u16 islandIndex);

	struct ActiveIsland
	{
		phSleepIsland* island;
		u16 generationId;

		ActiveIsland()
			: island(NULL)
			, generationId(0)
		{
		}
	};

	ActiveIsland* m_ActiveIslands;
	phRingBuffer<phSleepIsland> m_IslandBuffer;

	u16* m_UnusedIndices;
	u32 m_NumUnusedIndices;
	u32 m_MaxIslands;

#if PH_SLEEP_DEBUG
	phSleepDebugMgr* m_DebugMgr;
#endif // PH_SLEEP_DEBUG
};

#if PH_SLEEP_DEBUG
class phSleepDebugMgr
{
public:
	phSleepDebugMgr();

	void AddDebugRecord(phSleepDebugEvent event, phHandle handle);
	void RenderDebugRecord(phHandle handle, Vec3V_In position);

private:

	static const u32 MAX_STACK_SIZE = 16;

	struct Record
	{
		Record()
			: lastAccess(0)
		{ }

		phHandle handle;
		u32 lastAccess;
		phSleepDebugEvent event;
		size_t stack[MAX_STACK_SIZE];
	};

    Record& FindRecord(phHandle handle);

	static const int MAX_RECORDS = 128;
	Record m_Records[MAX_RECORDS];
};
#endif // PH_SLEEP_DEBUG

} // namespace rage

#endif // PHYSICS_SLEEPMGR_H 
