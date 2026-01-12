//
// physics/lockconfig.h
//
// Copyright (C) 1999-2013 Rockstar Games.  All Rights Reserved.
//

#ifndef PHYSICS_LOCKCONFIG_H
#define PHYSICS_LOCKCONFIG_H

// This define turns on/off the physics system critical section functions that protect
// many level and simulator functions from multiple threads accessing them simultaneously.
#define ENABLE_PHYSICS_LOCK 1

#include "system/threadtype.h"

#if ENABLE_PHYSICS_LOCK
#if __PS3
#include "system/cellsyncmutex.h"
#include "system/ipc.h"
#include "system/interlocked.h"
#else // __PS3
//#include "system/ipc.h"
#include "system/criticalsection.h"
#endif // __PS3
#endif

namespace rage
{

#if ENABLE_PHYSICS_LOCK

// Enables automatic logging of physics lock-related events into a buffer.
#define LOCKCONFIG_LOG_ENABLED	0

// Makes it so that writers won't get pushed aside indefinitely when new readers appear.
#define LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS	1

#if LOCKCONFIG_LOG_ENABLED

	enum PhysLockEvent
	{
		PLE_WAIT_AS_READER	=	0,
		PLE_ACQUIRED_COUNT_LOCK,
		PLE_ACQUIRED_READ_LOCK,
		PLE_WAIT_AS_WRITER,
		PLE_ACQUIRED_WRITE_LOCK,
		PLE_RELEASE_AS_READER,
		PLE_RELEASED_READ_LOCK,
		PLE_RELEASE_AS_WRITER,
		PLE_RELEASED_WRITE_LOCK,
	};

	static const u32 g_PhysLockMaxLogSize = 100;

	static u32 g_PhysLockCurLogSize = 0;
	static u32 g_PhysLockCurLogIndex = 0;

	class PhysLockLogEntry
	{
	public:
		PhysLockEvent m_Type;
		u32 m_ThreadID;
		u32 m_Data0, m_Data1, m_Data2;
	};

	static PhysLockLogEntry g_PhysLockLogs[g_PhysLockMaxLogSize];
	static const PhysLockLogEntry *g_PhysLockLogsPtr = g_PhysLockLogs;

	inline void g_PhysLockAddLogEntry(PhysLockEvent logType, const void *threadId, u32 data0, u32 data1, u32 data2)
	{
		PhysLockLogEntry &curPhysLockLog = g_PhysLockLogs[g_PhysLockCurLogIndex];
		curPhysLockLog.m_Type = logType;
		curPhysLockLog.m_ThreadID = (u32)(threadId);
		curPhysLockLog.m_Data0 = data0;
		curPhysLockLog.m_Data1 = data1;
		curPhysLockLog.m_Data2 = data2;

		g_PhysLockCurLogSize = g_PhysLockCurLogSize == g_PhysLockMaxLogSize ? g_PhysLockMaxLogSize : g_PhysLockCurLogSize + 1;
		g_PhysLockCurLogIndex = (g_PhysLockCurLogIndex + 1) % g_PhysLockMaxLogSize;
	}

	inline void g_PhysLockDumpLogs()
	{
		for(u32 logIndex = 0; logIndex < g_PhysLockCurLogSize; ++logIndex)
		{
			const int curLogIndex = (g_PhysLockCurLogIndex + logIndex) % g_PhysLockMaxLogSize;
			const PhysLockLogEntry &curPhysLockLog = g_PhysLockLogs[curLogIndex];
			const PhysLockEvent type = curPhysLockLog.m_Type;
			const u32 threadId = curPhysLockLog.m_ThreadID;
			const u32 data0 = curPhysLockLog.m_Data0;
			const u32 data1 = curPhysLockLog.m_Data1;
			const u32 data2 = curPhysLockLog.m_Data2;

			Displayf("%d: [%d][%d]", logIndex, type, threadId, data0, data1, data2);
		}
	}

	// NOTE: This macro automatically logs the global reader count but that value is only reliable if recorded when the reader count lock was already acquired.
#	define PHYSLOCK_LOG(x)	g_PhysLockAddLogEntry(x, &g_PerThreadReadLockCount, g_PerThreadReadLockCount, g_PerThreadWriteLockCount, GetReaderCount())

#else	// LOCKCONFIG_LOG_ENABLED

#	define PHYSLOCK_LOG(x)

#endif	// LOCKCONFIG_LOG_ENABLED

#if !__SPU
	extern __THREAD int g_PerThreadWriteLockCount;
	extern __THREAD int g_PerThreadReadLockCount;
	extern __THREAD int g_PerThreadAllowNewReaderLockCount;
#endif // !__SPU

	// Lets us avoid getting the write lock on the main thread
	extern bool g_OnlyMainThreadIsUpdatingPhysicsLockConfig;

	// phMultiReaderLock is a wrapper around a binary semaphore (aka a simple mutex) with one important distinction - it allows multiple readers access at the
	//   same time and writers must wait until all readers are gone.
	// With greater power comes greater responsibility.  Something to be keenly aware of when using this is that a single thread can't acquire a read lock and
	//   then try to acquire a write lock.	Acquiring a lock as a reader and then subsequently acquiring a lock as a writer *in the same thread* is a recipe
	//   for deadlock if another thread tries to do the same thing (neither can proceed due to the outstanding read locks but neither will ever release their read
	//   locks to allow the other to proceed).
	// The converse, acquiring a read lock when you've already acquired a write lock is safe as, once you've acquired you pretty much own things (nobody else
	//   will ever interrupt you in connection with this lock).
	// Acquiring the same type of lock more than once (as in acquiring a write lock when you've already acquired a write lock) is always acceptable.
	// Despite the ph- prefix there shouldn't actually be anything here that's physics specific.
	class phMultiReaderLockToken
	{
	public:
		phMultiReaderLockToken()
		{
#if __PPU
			rageCellSyncMutexInitialize(&m_PhysicsMutex);
			m_ReaderCount = 0;
			rageCellSyncMutexInitialize(&m_ModifyReaderMutex);
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
			rageCellSyncMutexInitialize(&m_AllowNewReaderMutex);
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
#elif __SPU
			// TODO: Consider having an alternate constructor on SPU that requires these parameters.
			m_PPUPhysicsMutex = NULL;
			m_PPUReaderCount = NULL;
			m_PPUModifyReaderCountMutex = NULL;
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
			m_PPUAllowNewReaderMutex = NULL;
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS

			m_PerThreadReadLockCount = 0;
			m_PerThreadWriteLockCount = 0;
			m_PerThreadAllowNewReaderLockCount = 0;
#else
			m_PhysicsMutex = sysIpcCreateSema(1);
			m_ReaderCount = 0;
			m_ModifyReaderMutex = sysIpcCreateSema(1);
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
			m_AllowNewReaderMutex = sysIpcCreateSema(1);
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
#endif
		}

		~phMultiReaderLockToken()
		{
#if __PPU
			// Is there really no mutex clean-up to do on PS3?
#elif __SPU
#else
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
			sysIpcDeleteSema(m_AllowNewReaderMutex);
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
			sysIpcDeleteSema(m_ModifyReaderMutex);
			sysIpcDeleteSema(m_PhysicsMutex);
#endif
		}

#if __PPU
		RageCellSyncMutex *GetPhysicsMutexPtr()
		{
			return &m_PhysicsMutex;
		}

		u32 *GetGlobalReaderCountPtr()
		{
			return &m_ReaderCount;
		}

		RageCellSyncMutex *GetModifyReaderCountMutexPtr()
		{
			return &m_ModifyReaderMutex;
		}

#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
		RageCellSyncMutex *GetAllowNewReaderMutex()
		{
			return &m_AllowNewReaderMutex;
		}
#	endif
#endif

#if __SPU
		// On the SPU we have to tell this lock where this mutex lives on the PPU.
		void SetPPUPhysicsMutexPtr(RageCellSyncMutex *physicsMutex)
		{
			m_PPUPhysicsMutex = physicsMutex;
		}

		// On the SPU we have to tell this lock where this count lives on the PPU.
		void SetPPUReaderCountPtr(u32 *readerCount)
		{
			m_PPUReaderCount = readerCount;
		}

		// On the SPU we have to tell this lock where this mutex lives on the PPU.
		void SetPPUModifyReaderCountMutexPtr(RageCellSyncMutex *modifyReaderCountMutex)
		{
			Assert(modifyReaderCountMutex != NULL);
			m_PPUModifyReaderCountMutex = modifyReaderCountMutex;
		}

#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
		void SetPPUAllowNewReaderMutexPtr(RageCellSyncMutex *allowNewReaderMutex)
		{
			Assert(allowNewReaderMutex != NULL);
			m_PPUAllowNewReaderMutex = allowNewReaderMutex;
		}
#	endif
#endif	// !__SPU




#if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
	__forceinline void AcquireAllowNewReaderLock()
	{
		// If we already have the new reader lock just increment the counter
		if(GetPerThreadAllowNewReaderLockCount() == 0)
		{
			Assertf(GetPerThreadWriteLockCount() == 0, "How do we have a write lock without having an allow new reader lock?");
#	if __PPU
			rageCellSyncMutexLock(&m_AllowNewReaderMutex);
#	elif __SPU
			rageCellSyncMutexLock((u32)m_PPUAllowNewReaderMutex);
#	else
			sysIpcWaitSema(m_AllowNewReaderMutex);
#	endif
		}
		IncrementPerThreadAllowNewReaderLockCount();
	}

	__forceinline void ReleaseAllowNewReaderLock()
	{
		Assertf(GetPerThreadAllowNewReaderLockCount() > 0, "Calling ReleaseAllowNewReaderLock more times than AcquireAllowNewReaderLock on the same thread.");
		// If multiple people still have the new reader lock just decrement the counter
		if(GetPerThreadAllowNewReaderLockCount() == 1)
		{
#	if __PPU
				rageCellSyncMutexUnlock(&m_AllowNewReaderMutex);
#	elif __SPU
				rageCellSyncMutexUnlock((u32)m_PPUAllowNewReaderMutex);
#	else
				sysIpcSignalSema(m_AllowNewReaderMutex);
#	endif
		}
		DecrementPerThreadAllowNewReaderLockCount();
	}
#endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS



		// Execution proceeds immediately if no writers are currently holding the lock or blocking new readers.
		void WaitAsReader()
		{
			PHYSLOCK_LOG(PLE_WAIT_AS_READER);

#if !__TOOL && !__SPU
			if(!g_OnlyMainThreadIsUpdatingPhysicsLockConfig || !sysThreadType::IsUpdateThread())
#endif // !__TOOL && !__SPU
			{
				// If our thread has already acquired a write lock we get a free pass and the reader is *only* counted in g_PerThreadReadLockCount.
				if(GetPerThreadWriteLockCount() == 0)
				{
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
					// Make sure that a writer isn't next in line.
					AcquireAllowNewReaderLock();
					ReleaseAllowNewReaderLock();
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS

					// Prevent other readers from modifying the reader count.
					AcquireReaderCountLock();
					PHYSLOCK_LOG(PLE_ACQUIRED_COUNT_LOCK);
					Assert((u32)(GetPerThreadReadLockCount()) <= GetReaderCount());
					if(IncrementReaderCount() == 1)
					{
						// We are the first reader and so acquire the lock to lock out writers.
						// This will immediately succeed unless a writer is currently locking things up.
						AcquirePhysicsLock();
						PHYSLOCK_LOG(PLE_ACQUIRED_READ_LOCK);
					}
					else
					{
						// If we're the first reader on this thread then a reader from another thread should already have the lock.
						//					Assert(g_PerThreadReadLockCount != 0 || !m_PhysicsMutex.TryLock());
					}

					// Allow other readers to get in on the action now that we've opened the flood-gates.
					ReleaseReaderCountLock();
				}

				IncrementPerThreadReadLockCount();
			}
		}

		// Execution proceeds immediately if no readers are current active and no writers from other threads are active.
		void WaitAsWriter()
		{
			PHYSLOCK_LOG(PLE_WAIT_AS_WRITER);

#if !__TOOL
			if(g_OnlyMainThreadIsUpdatingPhysicsLockConfig)
			{
#if __SPU
				Assertf(false,"Attempting to get write lock on non-main when sm_LockConfigWritersOnMainThreadOnly is enabled");
#else // __SPU
				Assertf(sysThreadType::IsUpdateThread(),"Attempting to get write lock on non-main when sm_LockConfigWritersOnMainThreadOnly is enabled");
#endif // __SPU
			}
#endif // !__TOOL

			Assertf(GetPerThreadReadLockCount() == 0, "Trying to acquire a write lock after acquiring a read lock on the same thread - this is a recipe for deadlock. (%d)", GetPerThreadReadLockCount());
			if(GetPerThreadWriteLockCount() == 0)
			{
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
				// First block out any new readers from acquiring the physics lock.  This wait should generally succeed immediately (or almost immediately)
				//   except in the case where another writer is already 
				AcquireAllowNewReaderLock();
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS

				// Now try and acquire the physics lock.
				AcquirePhysicsLock();
				PHYSLOCK_LOG(PLE_ACQUIRED_WRITE_LOCK);
				ASSERT_ONLY(const u32 kReaderCount = GetReaderCount());
				Assertf(kReaderCount == 0 || kReaderCount == 1, "WaitAsWriter(): Reader count (global/this thread): (%d/%d)", kReaderCount, GetPerThreadReadLockCount());
			}
			else
			{
				// Another writer (on this thread) should be holding the lock.
			}
			IncrementPerThreadWriteLockCount();
		}

		// 
		void ReleaseAsReader()
		{
			PHYSLOCK_LOG(PLE_RELEASE_AS_READER);
#if !__TOOL && !__SPU
			if(!g_OnlyMainThreadIsUpdatingPhysicsLockConfig || !sysThreadType::IsUpdateThread())
#endif // !__TOOL && !__SPU
			{
				Assertf(GetPerThreadReadLockCount() > 0, "Can't release a read lock that wasn't acquired.");
				DecrementPerThreadReadLockCount();
				if(GetPerThreadWriteLockCount() == 0)
				{
					AcquireReaderCountLock();
					PHYSLOCK_LOG(PLE_ACQUIRED_COUNT_LOCK);
					if(DecrementReaderCount() == 0)
					{
						// Last reader out turns off the lights (releases the lock).
						ReleasePhysicsLock();
						PHYSLOCK_LOG(PLE_RELEASED_READ_LOCK);
						ASSERT_ONLY(const u32 kReaderCount = GetReaderCount());
						Assertf(kReaderCount == 0, "Reader count: %d", kReaderCount);
					}
					else
					{
						// If we're the last reader left on this thread then a reader from another thread should already have the lock.
						//					Assertf(g_PerThreadReadLockCount != 0 || !m_PhysicsMutex.TryLock(), "%d %p %d %d", g_PhysLockLogsPtr, g_PhysLockCurLogIndex, g_PhysLockCurLogSize);
					}
					ReleaseReaderCountLock();
				}
				else
				{
					// We're releasing a reader lock that was obtained on a thread that had already obtained a write lock.  There should be at most
					//   one "pure" reader being counted in the reader count.
					ASSERT_ONLY(const u32 kReaderCount = GetReaderCount());
					Assertf(kReaderCount == 0 || kReaderCount == 1, "Reader count: %d", kReaderCount);
				}
			}
		}

		// 
		void ReleaseAsWriter()
		{
			PHYSLOCK_LOG(PLE_RELEASE_AS_WRITER);
			Assertf(GetPerThreadWriteLockCount() > 0, "Can't release a writer lock that wasn't acquired.");
			Assertf(GetPerThreadReadLockCount() == 0, "Releasing a writer when readers from this thread are still holding locks - you must be missing a ReleaseAsReader() somewhere. (%d)", GetPerThreadReadLockCount());
			// Can't assert that m_ReaderCount == 0 because, if there is a reader waiting to acquire the lock, m_ReaderCount will be 1.
			// m_ReaderCount cannot be greater than 1, however, because only the first reader will have been permitted to increment that count.
			ASSERT_ONLY(const u32 kReaderCount = GetReaderCount());
			Assertf(kReaderCount == 0 || kReaderCount == 1, "ReleaseAsWriter(): Reader count (global/this thread): (%d/%d)", kReaderCount, GetPerThreadReadLockCount());
			DecrementPerThreadWriteLockCount();
			if(GetPerThreadWriteLockCount() == 0)
			{
				ReleasePhysicsLock();
				PHYSLOCK_LOG(PLE_RELEASED_WRITE_LOCK);
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
				ReleaseAllowNewReaderLock();
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
			}
			else
			{
				// Another writer (on this thread) should still be holding the lock.
			}
		}
	private:

		// Platform-specific implementations of some low-level functionality.

		__forceinline void AcquireReaderCountLock()
		{
#if __PPU
			rageCellSyncMutexLock(&m_ModifyReaderMutex);
#elif __SPU
			rageCellSyncMutexLock((u32)m_PPUModifyReaderCountMutex);
#else
			sysIpcWaitSema(m_ModifyReaderMutex);
#endif
		}

		__forceinline void ReleaseReaderCountLock()
		{
#if __PPU
			rageCellSyncMutexUnlock(&m_ModifyReaderMutex);
#elif __SPU
			rageCellSyncMutexUnlock((u32)m_PPUModifyReaderCountMutex);
#else
			sysIpcSignalSema(m_ModifyReaderMutex);
#endif
		}

		__forceinline void AcquirePhysicsLock()
		{
#if __PPU
			rageCellSyncMutexLock(&m_PhysicsMutex);
#elif __SPU
			rageCellSyncMutexLock((u32)m_PPUPhysicsMutex);
#else
			sysIpcWaitSema(m_PhysicsMutex);
#endif
		}

		__forceinline void ReleasePhysicsLock()
		{
#if __PPU
			rageCellSyncMutexUnlock(&m_PhysicsMutex);
#elif __SPU
			rageCellSyncMutexUnlock((u32)m_PPUPhysicsMutex);
#else
			sysIpcSignalSema(m_PhysicsMutex);
#endif
		}

		__forceinline u32 IncrementReaderCount()
		{
#if __SPU
			return sysInterlockedIncrement(m_PPUReaderCount);
#else
			return sysInterlockedIncrement(&m_ReaderCount);
#endif
		}

		__forceinline u32 DecrementReaderCount()
		{
#if __SPU
			return sysInterlockedDecrement(m_PPUReaderCount);
#else
			return sysInterlockedDecrement(&m_ReaderCount);
#endif
		}

		__forceinline u32 GetReaderCount()
		{
#if __SPU
			return sysDmaGetUInt32((uint64_t)m_PPUReaderCount, DMA_TAG(1));
#else
			return m_ReaderCount;
#endif
		}

		__forceinline void IncrementPerThreadReadLockCount()
		{
#if __SPU
			++m_PerThreadReadLockCount;
#else
			++g_PerThreadReadLockCount;
#endif
		}

		__forceinline void DecrementPerThreadReadLockCount()
		{
#if __SPU
			--m_PerThreadReadLockCount;
#else
			--g_PerThreadReadLockCount;
#endif
		}

		__forceinline u32 GetPerThreadReadLockCount() const
		{
#if __SPU
			return m_PerThreadReadLockCount;
#else
			return g_PerThreadReadLockCount;
#endif
		}

		__forceinline void IncrementPerThreadWriteLockCount()
		{
#if __SPU
			++m_PerThreadWriteLockCount;
#else
			++g_PerThreadWriteLockCount;
#endif
		}

		__forceinline void DecrementPerThreadWriteLockCount()
		{
#if __SPU
			--m_PerThreadWriteLockCount;
#else
			--g_PerThreadWriteLockCount;
#endif
		}

		__forceinline u32 GetPerThreadAllowNewReaderLockCount() const
		{
#if __SPU
			return m_PerThreadAllowNewReaderLockCount;
#else
			return g_PerThreadAllowNewReaderLockCount;
#endif
		}

		__forceinline void IncrementPerThreadAllowNewReaderLockCount()
		{
#if __SPU
			++m_PerThreadAllowNewReaderLockCount;
#else
			++g_PerThreadAllowNewReaderLockCount;
#endif
		}

		__forceinline void DecrementPerThreadAllowNewReaderLockCount()
		{
#if __SPU
			--m_PerThreadAllowNewReaderLockCount;
#else
			--g_PerThreadAllowNewReaderLockCount;
#endif
		}

		__forceinline u32 GetPerThreadWriteLockCount() const
		{
#if __SPU
			return m_PerThreadWriteLockCount;
#else
			return g_PerThreadWriteLockCount;
#endif
		}


#if __PPU

		// On PS3 PPU we own these objects.
		RageCellSyncMutex			m_PhysicsMutex;
		u32							m_ReaderCount;
		RageCellSyncMutex			m_ModifyReaderMutex;
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
		RageCellSyncMutex			m_AllowNewReaderMutex;			// This being locked means that no new readers will/should be allowed to proceed.
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS

#elif __SPU

		// On SPU all of these point to PPU objects *and need to be initialized*.
		RageCellSyncMutex			*m_PPUPhysicsMutex;				// Holding this means that no writers can change the physics level.
		u32							*m_PPUReaderCount;				// The current reader count.
		RageCellSyncMutex			*m_PPUModifyReaderCountMutex;	// Holding this means that you can read/modify the reader count.
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
		RageCellSyncMutex			*m_PPUAllowNewReaderMutex;		// This being locked means that no new readers will/should be allowed to proceed.
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
		u32							m_PerThreadReadLockCount;		// On SPU each SPU is a 'thread' so we store this directly as a member of the lock.
		u32							m_PerThreadWriteLockCount;		// On SPU each SPU is a 'thread' so we store this directly as a member of the lock.
		u32							m_PerThreadAllowNewReaderLockCount;	// On SPU each SPU is a 'thread' so we store this directly as a member of the lock.
#else

		// On Xbox360 and PC we own these objects.
		sysIpcSema					m_PhysicsMutex;
		u32							m_ReaderCount;
		sysIpcSema					m_ModifyReaderMutex;
#	if LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS
		sysIpcSema					m_AllowNewReaderMutex;			// This being locked means that no new readers will/should be allowed to proceed.
#	endif	// LOCKCONFIG_WRITERS_ONLY_WAIT_FOR_CURRENT_READERS

#endif
	};


	// PURPOSE: Acquire and hold a write lock for as long as the object exists.
	class phMultiReaderScopedWriteLock
	{
	public:
		phMultiReaderScopedWriteLock(phMultiReaderLockToken& token)
			: m_Token(token) 
		{
			m_Token.WaitAsWriter();
		}

		~phMultiReaderScopedWriteLock()
		{
			m_Token.ReleaseAsWriter();
		}

	protected:
		phMultiReaderLockToken	&m_Token;
	};


	// PURPOSE: Acquire and hold a read lock for as long as the object exists.
	class phMultiReaderScopedReadLock
	{
	public:
		phMultiReaderScopedReadLock(phMultiReaderLockToken& token)
			: m_Token(token) 
		{
			m_Token.WaitAsReader();
		}

		~phMultiReaderScopedReadLock()
		{
			m_Token.ReleaseAsReader();
		}

	protected:
		phMultiReaderLockToken	&m_Token;
	};

#if !__SPU
	extern phMultiReaderLockToken g_GlobalPhysicsLock;
#endif

#define PHLOCK_SCOPEDWRITELOCK	phMultiReaderScopedWriteLock lock(g_GlobalPhysicsLock)
#define PHLOCK_SCOPEDREADLOCK	phMultiReaderScopedReadLock lock(g_GlobalPhysicsLock)

#else // ENABLE_PHYSICS_LOCK

#define PH_LOCK_SCOPE
#define PH_LOCK_ACQUIRE
#define PH_LOCK_RELEASE
#define PHYSICS_LOCK_ONLY(x)

#endif

}

#endif // PHYSICS_LOCKCONFIG_H
