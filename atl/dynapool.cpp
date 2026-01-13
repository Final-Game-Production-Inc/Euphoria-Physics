//
// atl/dynapool.cpp
//
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved.
//

#include "dynapool.h"

#include "file/stream.h"

namespace rage {

PARAM(assertdynapooloverrun, "[atl] Assert if a dynapool switches to dynamic allocation");


#if ENABLE_DYNAPOOL_MANAGER

// List of all registered pools.
atArray<atDynaPoolBaseWithStats *> atDynaPoolManager::m_PoolList;


/* PURPOSE: Register a dynamic pool so it will show up in DumpStats.
 * This should happen automatically in a pool's constructor, no need
 * to do it manually.
 */
void atDynaPoolManager::RegisterPool(atDynaPoolBaseWithStats &pool)
{
	m_PoolList.Grow() = &pool;
}

/* PURPOSE: Unregister a dynamic pool. This should happen automatically
 * in a pool's destructor.
 */
void atDynaPoolManager::UnregisterPool(atDynaPoolBaseWithStats &pool)
{
	int count = m_PoolList.GetCount();
	atDynaPoolBaseWithStats **elements = m_PoolList.GetElements();

	for (int x=0; x<count; x++)
	{
		if (elements[x] == &pool)
		{
			m_PoolList.DeleteFast(x);
			return;
		}
	}

	Warningf("Unregistering a dynamic pool that had never been registered");
}

/* PURPOSE: Dump the stats of every dynamic pool that has been registered
 * with this manager.
 */
void atDynaPoolManager::DumpStats()
{
	Displayf("==== List of dynamic pools =====");
	Displayf("Pool name,Element Size,Currently allocated,Peak allocation,Pool Size,Usage");

	int count = m_PoolList.GetCount();
	atDynaPoolBaseWithStats **elements = m_PoolList.GetElements();

	for (int x=0; x<count; x++)
	{
		atDynaPoolBaseWithStats &pool = *elements[x];
		int used = pool.StatsGetNumUsed();
		int peak = pool.StatsGetMaxUsedRef();
		int max = pool.StatsGetNumRemaining() + used;
		size_t size = pool.StatsGetElementSize();
		int usage = peak * 100 / max;

		Displayf("%s,%d,%d,%d,%d,%d%%", pool.GetName(), (int)size, used, peak, max, usage);
	}

	Displayf("==== End of dynamic pool list =====");
}

/* PURPOSE: Dump the stats of every dynamic pool that has been registered
 * with this manager to a file.
 */
void atDynaPoolManager::DumpStatsToFile(fiStream *S)
{
	fputs("==== List of dynamic pools =====\r\n", S);
	fputs("Pool name,Element Size,Currently allocated,Peak allocation,Pool Size,Usage\r\n", S);

	int count = m_PoolList.GetCount();
	atDynaPoolBaseWithStats **elements = m_PoolList.GetElements();

	for (int x=0; x<count; x++)
	{
		char buffer[256];
		atDynaPoolBaseWithStats &pool = *elements[x];
		int used = pool.StatsGetNumUsed();
		int peak = pool.StatsGetMaxUsedRef();
		int max = pool.StatsGetNumRemaining() + used;
		size_t size = pool.StatsGetElementSize();
		int usage = peak * 100 / max;

		formatf(buffer, "%s,%d,%d,%d,%d,%d%%\r\n", pool.GetName(), size, used, peak, max, usage);
		fputs(buffer, S);
	}

	fputs("==== End of dynamic pool list =====\r\n", S);
}


#endif // ENABLE_DYNAPOOL_MANAGER


} // namespace rage
