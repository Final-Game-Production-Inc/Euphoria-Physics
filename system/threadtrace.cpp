//
// system/threadtrace.h
//
// Copyright (C) 2013-2013 Rockstar Games.  All Rights Reserved.
//

#include "threadtrace.h"

#if SYSTEM_THREADTRACE_ENABLE

#if __PS3

#	include <algorithm>
#	include <math.h>

#	include <cell/perf/performance.h>
#	include <sys/dbg.h>
#	include <sys/event.h>
#	include <sys/synchronization.h>
#	include <sys/sys_time.h>
#	include <sys/timer.h>

#	include "system/criticalsection.h"
#	include "system/ipc.h"
#	include "system/memops.h"
#	include "system/tls.h"

#	pragma comment(lib, "perf")

	extern "C" int snIsTunerRunning(void);

	extern __THREAD int RAGE_LOG_DISABLE;


#	define PPU_SCHED_TRACE_BUF_SIZE    (0x400000)
#	define TRACE_THREAD_PERIOD_MS      (2)


	namespace rage
	{

	namespace sysThreadTrace
	{
		// Synchronize ThreadTraceThread and other threads reading the stats
		static sysCriticalSectionToken s_mutex;

		static sysIpcThreadId s_threadId;
		static volatile bool s_threadExit;

		// Periodic timer to get ThreadTraceThread to run at fixed intervals
		static sys_timer_t s_timerId;
		static sys_event_queue_t s_timerEventQueueId;

		// Buffer containing Lv2 OS thread trace
		static uint32_t *s_ppuSchedTraceBuf;

		enum { MAX_NUM_THREADS = 128 };

		struct ThreadStats
		{
			struct Info
			{
				uint32_t    id;
				uint32_t    dispatch;
			};

			Info        info[MAX_NUM_THREADS];
			uint64_t    time[MAX_NUM_THREADS];
			bool        tainted;
		};

		static ThreadStats s_stats;


		////////////////////////////////////////////////////////////////////////////
		//  Taint
		////////////////////////////////////////////////////////////////////////////

		static inline void Taint(ThreadStats *stats)
		{
			for (unsigned i=0; i<MAX_NUM_THREADS; ++i)
			{
				stats->info[i].dispatch = 0;
			}
			stats->tainted = true;
		}


		////////////////////////////////////////////////////////////////////////////
		//  HandlePacket
		////////////////////////////////////////////////////////////////////////////

		static inline uint32_t HandlePacket(ThreadStats *stats, uint32_t readPointer)
		{
			// Fetch a packet
			const uint32_t *const bufBegin = s_ppuSchedTraceBuf + CELL_PERF_LV2_TRACE_HEADER_SIZE/sizeof(uint32_t);
			const CellPerfLv2TracePacket *const packet = (CellPerfLv2TracePacket*)(bufBegin + readPointer);
			const uint32_t tag = packet->header.tag;
			const uint32_t length = packet->header.length;
			const uint32_t timebase = packet->header.timebase;
			const uint32_t threadId = packet->payload.schedule.threadId;
			const uint32_t incident = packet->payload.schedule.incident;
			uint32_t nextReadPointer = readPointer + length*2;
			if (Unlikely(nextReadPointer >= (PPU_SCHED_TRACE_BUF_SIZE - CELL_PERF_LV2_TRACE_HEADER_SIZE)/sizeof(uint32_t)))
			{
				nextReadPointer = 0;
			}
			if (Unlikely(tag != CELL_PERF_LV2_TRACE_TYPE_PPU_SCHEDULER))
			{
				return nextReadPointer;
			}

			// Find thread matching stats
			const unsigned hashFailed = threadId & (MAX_NUM_THREADS-1);
			unsigned hashIdx = hashFailed;
			bool foundSlot = false;
			do
			{
				if (Likely(stats->info[hashIdx].id == threadId))
				{
					foundSlot = true;
					break;
				}

				else if (Likely(stats->info[hashIdx].dispatch == 0 && stats->time[hashIdx] == 0))
				{
					stats->info[hashIdx].id = threadId;
					foundSlot = true;
					break;
				}
				else
				{
					hashIdx = (hashIdx+1) & (MAX_NUM_THREADS-1);
				}
			}
			while (Likely(hashIdx != hashFailed));

			// Hash not found and no space to add it.
			if (Unlikely(!foundSlot))
			{
				Assertf(0, "ran out of thread hash table slots");
				return nextReadPointer;
			}

			switch (incident)
			{
				case CELL_PERF_LV2OS_TRACE_SCHED_DISPATCHED:
				{
					stats->info[hashIdx].dispatch = timebase;
					break;
				}

				case CELL_PERF_LV2OS_TRACE_SCHED_PREEMPTED:
				case CELL_PERF_LV2OS_TRACE_SCHED_YIELD:
				case CELL_PERF_LV2OS_TRACE_SCHED_SLEEP:
				case CELL_PERF_LV2OS_TRACE_SCHED_EXIT:
				{
					const uint32_t dispatch = stats->info[hashIdx].dispatch;
					if (Likely(dispatch))
					{
						stats->time[hashIdx] += timebase - dispatch;
						stats->info[hashIdx].dispatch = 0;
					}
					else
					{
						stats->tainted = true;
					}
					break;
				}
			}

			return nextReadPointer;
		}


		////////////////////////////////////////////////////////////////////////////
		//  ThreadTraceThread
		////////////////////////////////////////////////////////////////////////////

		static void ThreadTraceThread(void*)
		{
			// The trace buffer really is const.  Stores to it will generate a data
			// htab miss.
			const volatile CellPerfLv2TraceHeader *const traceHeader = (CellPerfLv2TraceHeader*)s_ppuSchedTraceBuf;
			const uint32_t *const bufBegin = s_ppuSchedTraceBuf + CELL_PERF_LV2_TRACE_HEADER_SIZE/sizeof(uint32_t);

			cellPerfStart();

			uint32_t readPointer = traceHeader->readPointer;
			uint32_t prevLastValidReadPointer = readPointer;
			uint32_t prevLastValidReadTimebase = 0;

			while (Likely(!s_threadExit))
			{
				s_mutex.Lock();

				// Has the last packet read been overwriten while we were waiting for the timer?
				const CellPerfLv2TracePacket *packet = (CellPerfLv2TracePacket*)(bufBegin + prevLastValidReadPointer);
				if (Unlikely(prevLastValidReadTimebase != packet->header.timebase))
				{
					Taint(&s_stats);
					readPointer = traceHeader->readPointer;
					prevLastValidReadTimebase = packet->header.timebase;
				}

				// Backup copy of the stats for updating, as we may need to roll back.
				ThreadStats statsBackup;
				sysMemCpy(&statsBackup, &s_stats, sizeof(statsBackup));

				// Read all available packets
				uint32_t lastValidReadPointer = prevLastValidReadPointer;
				const uint32_t writePointer = traceHeader->writePointer;
				while (Likely(readPointer != writePointer))
				{
					lastValidReadPointer = readPointer;
					readPointer = HandlePacket(&s_stats, readPointer);
				}

				// Is the last packet from the previous loop still valid, if not,
				// then the packets we just processed may be wrong.
				__lwsync();
				if (Unlikely(prevLastValidReadTimebase != packet->header.timebase))
				{
					sysMemCpy(&s_stats, &statsBackup, sizeof(s_stats));
					Taint(&s_stats);
				}

				packet = (CellPerfLv2TracePacket*)(bufBegin + lastValidReadPointer);
				prevLastValidReadPointer = lastValidReadPointer;
				prevLastValidReadTimebase = packet->header.timebase;

				s_mutex.Unlock();

				sys_event_t event;
				const usecond_t timeout = 0;
				sys_event_queue_receive(s_timerEventQueueId, &event, timeout);
			}
		}
	}
	// namespace sysThreadTrace


	////////////////////////////////////////////////////////////////////////////////
	//  sysThreadTraceInit
	////////////////////////////////////////////////////////////////////////////////

	void sysThreadTraceInit()
	{
		using namespace sysThreadTrace;

		if (snIsTunerRunning())
		{
			return;
		}

		int ret;

		// Create the ppu thread scheduler trace module.
		// Note that cellPerfAddLv2OSTrace calls operator new, so we increment
		// RAGE_LOG_DISABLE to prevent an assert.
		++RAGE_LOG_DISABLE;
		const uint32_t type = CELL_PERF_LV2_TRACE_TYPE_PPU_SCHEDULER;
		const size_t   size = PPU_SCHED_TRACE_BUF_SIZE;
		const uint32_t mode = CELL_PERF_LV2_TRACE_MODE_OVERWRITE;
		if (Unlikely((ret=cellPerfAddLv2OSTrace(type, size, mode, &s_ppuSchedTraceBuf)) != CELL_OK))
		{
			Quitf("sysThreadTraceInit: cellPerfAddLv2OSTrace failed 0x%08x", ret);
		}
		--RAGE_LOG_DISABLE;

		// Create an event queue for the timer events that will be used by the trace processing thread.
		sys_event_queue_attribute_t attr;
		sys_event_queue_attribute_initialize(attr);
		sys_event_queue_attribute_name_set(attr.name, "ThrdTrc");
		const int queueDepth = 1;
		if (Unlikely((ret=sys_event_queue_create(&s_timerEventQueueId, &attr, SYS_EVENT_QUEUE_LOCAL, queueDepth)) != CELL_OK))
		{
			Quitf("sysThreadTraceInit: sys_event_queue_create failed 0x%08x", ret);
		}

		// Create the timer, hook it up to the event queue, and start it.
		if (Unlikely((ret=sys_timer_create(&s_timerId)) != CELL_OK))
		{
			Quitf("sysThreadTraceInit: sys_timer_create failed 0x%08x", ret);
		}
		const uint64_t data1 = 0;
		const uint64_t data2 = 0;
		if (Unlikely((ret=sys_timer_connect_event_queue(s_timerId, s_timerEventQueueId, SYS_TIMER_EVENT_NO_NAME, data1, data2)) != CELL_OK))
		{
			Quitf("sysThreadTraceInit: sys_timer_connect_event_queue failed 0x%08x", ret);
		}
		if (Unlikely((ret=sys_timer_start_periodic(s_timerId, TRACE_THREAD_PERIOD_MS*1000)) != CELL_OK))
		{
			Quitf("sysThreadTraceInit: sys_timer_start_periodic failed 0x%08x", ret);
		}

		void *const closure = NULL;
		const unsigned stackSize = 0x1000;
		const sysIpcPriority priority = PRIO_TIME_CRITICAL;
		const char *const threadName = "sysThreadTrace";
		s_threadId = sysIpcCreateThread(ThreadTraceThread, closure, stackSize, priority, threadName);
	}


	////////////////////////////////////////////////////////////////////////////////
	//  sysThreadTraceShutdown
	////////////////////////////////////////////////////////////////////////////////

	void sysThreadTraceShutdown()
	{
		using namespace sysThreadTrace;

		s_threadExit = true;
		sysIpcWaitThreadExit(s_threadId);

		sys_timer_disconnect_event_queue(s_timerId);
		sys_timer_destroy(s_timerId);
		sys_event_queue_destroy(s_timerEventQueueId, SYS_EVENT_QUEUE_DESTROY_FORCE);

		cellPerfDeleteLv2OSTrace(CELL_PERF_LV2_TRACE_TYPE_PPU_SCHEDULER);
	}


	////////////////////////////////////////////////////////////////////////////////
	//  sysThreadTraceBeginFrame
	////////////////////////////////////////////////////////////////////////////////

	void sysThreadTraceBeginFrame()
	{
		using namespace sysThreadTrace;

		SYS_CS_SYNC(s_mutex);
		ThreadStats *const stats = &s_stats;
		sysMemSet(stats->time, 0, sizeof(stats->time));
		stats->tainted = false;
	}


	////////////////////////////////////////////////////////////////////////////////
	//  sysThreadTracePrintThreadStats
	////////////////////////////////////////////////////////////////////////////////

	void sysThreadTracePrintThreadStats()
	{
		using namespace sysThreadTrace;

		// Copy stats
		ThreadStats stats;
		{
			SYS_CS_SYNC(s_mutex);
			sysMemCpy(&stats, &s_stats, sizeof(stats));
		}

		// Can't use std::sort here because we are sorting two parallel arrays.  So
		// use a simple arse bubble sort, because (a) not many items, (b) easy to
		// code.
		bool sorted;
		do
		{
			sorted = true;
			for (unsigned i=1; i<MAX_NUM_THREADS; ++i)
			{
				if (stats.time[i] > stats.time[i-1])
				{
					std::swap(stats.time[i],    stats.time[i-1]);
					std::swap(stats.info[i].id, stats.info[i-1].id);
					// don't care about dispatch
					sorted = false;
				}
			}
		}
		while (!sorted);

		const uint64_t timebaseFreq = sys_time_get_timebase_frequency();
		const double timebasePeriod = 1.0 / (double)timebaseFreq;

		Displayf("================================================================");
		if (stats.tainted)
		{
			Warningf("**** Timings are tainted.                                 ****");
			Warningf("**** Displayed values may be less than the actual timings ****");
		}
		for (unsigned i=0; i<MAX_NUM_THREADS; ++i)
		{
			const uint32_t threadId = stats.info[i].id;
			if (stats.info[i].id)
			{
				char threadName[28];
				if (sys_dbg_get_ppu_thread_name((sys_ppu_thread_t)threadId, threadName) != CELL_OK)
				{
					//sysMemCpy(threadName, "???", 4);
					continue;
				}
				const uint64_t timebase = stats.time[i];
				if (timebase == 0)
				{
					break;
				}
				double tot = (double)timebase*timebasePeriod;
				const double sec  = floor(tot); tot = (tot - sec)  * 1000.0;
				const double msec = floor(tot); tot = (tot - msec) * 1000.0;
				const double usec = floor(tot); tot = (tot - usec) * 1000.0;
				const double nsec = floor(tot); tot = (tot - nsec) * 1000.0;
				const double psec = floor(tot);
				Displayf("%.0f.%03.0f,%03.0f,%03.0f,%03.0fs 0x%08x \"%s\"", sec, msec, usec, nsec, psec, threadId, threadName);
			}
		}
	}


	}
	// namespace rage

#endif // __PS3

#endif // SYSTEM_THREADTRACE_ENABLE
