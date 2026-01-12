#ifndef PHYSICS_ASYNCSHAPETESTMGR_H
#define PHYSICS_ASYNCSHAPETESTMGR_H


//rage headers
#include "atl/queue.h"
#include "physics/shapetest.h"			// Needed for phShapeTestTaskData.
#include "system/criticalsection.h"
#include "system/ipc.h"

namespace rage
{
	class phIntersection;

	// TODO: Perhaps break this out into its own file.  I know that sounds lame, but it really helps reduce dependencies since a typedef can't be
	//   forward-declared and people are often going to need the definition of that but not anything else.
	typedef u32 New_phAsyncShapeTestHandle;

	class New_phAsyncProbeRequest
	{
	public:
		New_phAsyncProbeRequest() : m_RequestFlags(0)
		{
		}

		inline void Init(phIntersection *destIsects, const u32 numIsects, Vec3V_In vStartPos, Vec3V_In vEndPos, const u32 stateIncludeFlags, const u32 typeFlags, const u32 typeIncludeFlags, const u32 typeExcludeFlags, const phInst * const *excludeInstList, const u32 excludeInstCount, const u8 userData)
		{
			FastAssert(destIsects != NULL || numIsects == 0);
			m_StartPos = vStartPos;
			m_EndPos = vEndPos;
			m_DestinationIntersections = destIsects;
			FastAssert(numIsects < 65536);
			m_NumIntersections = (u16)(numIsects);
			m_UserData = userData;
			FastAssert(stateIncludeFlags < 256);
			m_StateIncludeFlags = (u8)(stateIncludeFlags);
			m_TypeFlags = typeFlags;
			m_TypeIncludeFlags = typeIncludeFlags;
			m_TypeExcludeFlags = typeExcludeFlags;

			m_NumExcludeInstances = excludeInstCount;
			// excludeInstList == NULL is valid iff excludeInstCount == 0
			FastAssert((excludeInstCount == 0) == (excludeInstList == NULL));
			if(excludeInstCount == 1)
			{
				m_ExcludeInst = excludeInstList[0];
				m_ExcludeInstList = NULL;
			}
			else
			{
				m_ExcludeInst = NULL;
				m_ExcludeInstList = excludeInstList;
			}

			m_RequestFlags = PROBEREQUESTFLAG_IN_USE;
			m_CompletionTime = FLT_MAX;
		}

		inline void InitShapeTest(phShapeTest<phShapeProbe> *shapeTestToInit) const
		{
			FastAssert(shapeTestToInit != NULL);
			shapeTestToInit->SetIncludeFlags(m_TypeIncludeFlags);
			shapeTestToInit->SetTypeExcludeFlags(m_TypeExcludeFlags);
			shapeTestToInit->SetTypeFlags(m_TypeFlags);
			shapeTestToInit->SetStateIncludeFlags(m_StateIncludeFlags);
			if(m_NumExcludeInstances > 1)
			{
				shapeTestToInit->SetExcludeInstanceList(m_ExcludeInstList, m_NumExcludeInstances);
			}
			else
			{
				shapeTestToInit->SetExcludeInstance(m_ExcludeInst);
			}
			shapeTestToInit->GetShape().InitProbe(phSegmentV(m_StartPos, m_EndPos), m_DestinationIntersections, m_NumIntersections);
		}

		inline void Clear()
		{
			m_RequestFlags = 0;
		}

		inline void SetIsInUse()
		{
			m_RequestFlags |= PROBEREQUESTFLAG_IN_USE;
		}

		inline void SetHasResult()
		{
			m_RequestFlags |= PROBEREQUESTFLAG_HAS_RESULT;
		}

		inline void SetHitSomething()
		{
			m_RequestFlags |= PROBEREQUESTFLAG_HIT_SOMETHING;
		}

		inline void SetReleaseWhenFinished()
		{
			m_RequestFlags |= PROBEREQUESTFLAG_RELEASE_WHEN_FINISHED;
		}

		inline void SetCompletionTime(float completionTime)
		{
			m_CompletionTime = completionTime;
		}

		inline bool IsInUse() const
		{
			return (m_RequestFlags & PROBEREQUESTFLAG_IN_USE) != 0;
		}

		inline bool HasResult() const
		{
			FastAssert(IsInUse());
			return (m_RequestFlags & PROBEREQUESTFLAG_HAS_RESULT) != 0;
		}

		inline bool HitSomething() const
		{
			FastAssert(IsInUse());
			FastAssert(HasResult());
			return (m_RequestFlags & PROBEREQUESTFLAG_HIT_SOMETHING) != 0;
		}

		inline bool ReleaseWhenFinished() const
		{
			FastAssert(IsInUse());
			return (m_RequestFlags & PROBEREQUESTFLAG_RELEASE_WHEN_FINISHED) != 0;
		}

		inline u8 GetUserData() const
		{
			FastAssert(IsInUse());
			return m_UserData;
		}

		inline phIntersection *GetDestIntersections() const
		{
			FastAssert(IsInUse());
			return m_DestinationIntersections;
		}

		inline u32 GetNumDestIntersections() const
		{
			FastAssert(IsInUse());
			return (u32)(m_NumIntersections);
		}

		inline float GetCompletionTime() const
		{
			FastAssert(IsInUse());
			FastAssert(HasResult());
			return m_CompletionTime;
		}

	protected:
		enum Flags
		{
			PROBEREQUESTFLAG_IN_USE =					1 << 0, // Handle has been issued corresponding to this request.
			PROBEREQUESTFLAG_HAS_RESULT =				1 << 1,	// Results are ready.
			PROBEREQUESTFLAG_HIT_SOMETHING =			1 << 2,	// At least one object was hit.
			PROBEREQUESTFLAG_RELEASE_WHEN_FINISHED =	1 << 3	// Release this when finished.
		};

		Vec3V m_StartPos;						// w-component is available if needed
		Vec3V m_EndPos;							// w-component is available if needed
		phIntersection *m_DestinationIntersections;
		u16 m_NumIntersections;
		u8 m_UserData;
		u8 m_StateIncludeFlags;					// What state(s) do we want to find?
		u32 m_TypeFlags;						// What type(s) are we?
		u32 m_TypeIncludeFlags;					// What type(s) do we want to find?
		u32 m_TypeExcludeFlags;					// What type(s) do we want to exclude?
		const phInst *m_ExcludeInst;
		const phInst *const *m_ExcludeInstList;
		u32 m_NumExcludeInstances;
		u32 m_RequestFlags;
		float m_CompletionTime;					// If this request is ready, for how long has it been ready?  Used for timing out requests.
	};


	class New_phShapeTestTask
	{
	public:
		New_phShapeTestTask() : m_hTaskHandle(0), m_iNumProbes(0) {};
		enum { PHASYNCSHAPETESTMGR_MAXPROBESPERTASK			= 5 };

		// Internal handle to the task which is running the shapetest
		sysTaskHandle m_hTaskHandle;
		// The number of probes we will be submitting
		int m_iNumProbes;
		// Handle(s) of the shapetest(s) this task is processing.
		// TODO: These REALLY are request indices, not handles (that is, their upper 16 bits are always zero).
		New_phAsyncShapeTestHandle m_hHandle[PHASYNCSHAPETESTMGR_MAXPROBESPERTASK];
		// An array of phShapeProbe shapetests which we will pass into the phShapeTestTaskData
		phShapeTest<phShapeProbe> m_probeShapeTests[PHASYNCSHAPETESTMGR_MAXPROBESPERTASK];
		// This is the main class which handles the creating of the async task
		phShapeTestTaskData m_shapeTestTaskData;
	};


	// phAsyncShapeTestMgr is a multi-threaded shape test manager based on a system implemented and used in GTAIV.  Like the already-existing
	//   multi-threaded shape test manager in RAGE (phShapeTestTaskManager) phAsyncShapeTestMgr provides a convenient interface for making good use
	//   of multiple cores and SPUs.  However, whereas phShapeTestTaskManager is designed for high throughput when immediate results are needed,
	//   phAsyncShapeTestMgr is designed for a minimal impact on computing resources while maintaining good throughput when immediate results are
	//   not needed.
	// One big difference with phShapeTestTaskManager that, since results are not immediately available, requests that are made are issued handles
	//   that can be queried at a future time.  Additionally, to avoid bogging down the system (and stalling other operations) shape test tasks are
	//   issued at a controlled rate during the frame.  Additional convenient features of the system include time-outs of handles after a certain
	//   period of time (to avoid handle leaks), ability to check whether a given handle is in use and the ability to check (through the use of
	//   generation IDs) whether instances referenced by given shape test results are still current.
	class phAsyncShapeTestMgr
	{
	public:
		enum ELosResult
		{
			// The Los was blocked
			EBlocked,
			// The Los was clear
			EClear,
			// The Los has not been processed yet
			ENotYetReady,
			// The specified Los is not found in the system!  error!
			ENotFound
		};

		static const New_phAsyncShapeTestHandle InvalidShapeTestHandle = (New_phAsyncShapeTestHandle)-1;

		enum { PHASYNCSHAPETESTMGR_MAXREQUESTS				= 90 };
		enum { PHASYNCSHAPETESTMGR_MAXRESULTS				= 90 };
		enum { PHASYNCSHAPETESTMGR_MAXBACKINGISECTS			= 90 };
		enum { PHASYNCSHAPETESTMGR_TIMEOUTMS				= 4000 };

		// NEW
		// Probably this could/should be smaller than the total number of requests.
		enum { PHASYNCSHAPETESTMGR_MAXPENDINGREQUESTS		= 90 };

		enum { PHASYNCSHAPETESTMGR_NUMTASKS					= 4 };

		phAsyncShapeTestMgr();
		~phAsyncShapeTestMgr();

		void Init(const int schedulerIndex, const unsigned stackSize=2048);
		void Shutdown();

		// PURPOSE: Control/query as to whether requests will time out or not.
		// NOTES: Generally this is going to be most useful when coming into or out of a paused application state.  When paused, many game systems are no longer
		//   updating and it could be unfortunate to require all such systems to re-issue all of their queries immediately upon return to normal gameplay (think
		//   of AI who may have made queries into the world around them right before entering pause and would now be further delayed in receiving those results).
		//     You can call this redundantly as many times as you want, and it is not an error to do so (calls do not nest).
		inline void New_SetSuspendTimeouts(const bool suspendTimeOuts);
		inline bool New_GetSuspendTimeouts() const;

		// PURPOSE: Control/query as to whether new tasks for performing shape tests should be spawned in response to the request queue containing work.
		// NOTES: This is intended for use around large sections of code that will be needing write access to the physics level.  Because, with the current
		//   physics lock system, writers will be held up indefinitely until all readers are through, it is important to reduce the likelihood that the
		//   (hopefully well-defined and well-contained) code that needs write access to make some simple changes will be held up by a deluge of shape test
		//   tasks.
		//     Note also that this does not suspend or terminate any tasks that may already be in process.
		inline void New_SetSuspendNewTasks(const bool suspendNewTasks);
		inline bool New_GetSuspendNewTasks() const;

		// PURPOSE: Submit a new probe query to the manager.
		// RETURN: A handle that you use to refer to this query.
		// NOTES: destIsects can be NULL.  If it is, one of two different things will happen, depending on numIsects.  If numIsects is 0, then
		//   you will get only hit/no-hit results.  If numIsects is 1, you will be issued a temporary intersection in which the detailed results
		//   will be stored.
		//     excludeInstList is an array of pointers to instances to be excluded by this shape test.  If excludeInstCount is greater than 1, *excludeInstList
		//   must point to an array of instance pointers that will persist until the shape test completes*.  If excludeInstCount is 1, then the first element of
		//   excludeInstList will be cached off and excludeInstList will not need to persist after the call to New_SubmitProbe() completes.  If excludeInstCount
		//   is 0, excludeInstList must be NULL.
		New_phAsyncShapeTestHandle New_SubmitProbe(phIntersection *destIsects, const u32 numIsects, Vec3V_In vStart, Vec3V_In vEnd, const u32 stateIncludeFlags, 
			const u32 typeFlags, const u32 typeIncludeFlags, const u32 typeExcludeFlags, const phInst *const *excludeInstList, const u32 excludeInstCount, u8 userData);

		// PURPOSE: Find out if a shape test that you submitted has completed yet.
		// NOTES: Until you have verified that the results are available, it doesn't make any sense ask most other questions about a given
		//   New_phAsyncShapeTestHandle.
		inline bool New_IsResultReady(const New_phAsyncShapeTestHandle hHandle) const;

		// PURPOSE: Find out if a completed shape test hit anything.
		// NOTES: Don't bother calling this if you haven't verified that results are ready.
		//   This is typically the most useful in situations in which you specified no intersections were to be used for recording results.
		inline bool New_GetHitSomething(const New_phAsyncShapeTestHandle hHandle) const;

		// PURPOSE: Find out where a shape test is going to record data when it completes.
		// NOTES: This is one of the few "handle query" functions that is safe to call even if the shape test to which it corresponds has not
		//   completed.  Of course, actually dereferencing this pointer and looking at the data should not be done until the results are ready.
		inline const phIntersection *New_GetDestIsect(const New_phAsyncShapeTestHandle hHandle) const;

		// PURPOSE: Find out if a given shape test handle has been issued and not yet released.
		// NOTES: You should probably never have any reason to call this as you should know whether or not you've released a handle yet, but
		//   it's included here for completeness because the information's available.  <- That's actually not so true now that requests can time
		//   out.
		inline bool New_IsInUse(const New_phAsyncShapeTestHandle hHandle) const;

		// PURPOSE: Tell the manager that you are done with a given handle and that it can be re-issued.
		// NOTES: Call this as soon as you are done with a handle (ie, you will not be making any more queries through that handle, such as to check
		//   whether or not its results are ready).  If you provided your own destination intersections then the data in them will still be good.
		// Typically you will call this as soon as you have confirmed that the results are ready.
		// Perhaps this will get called ReleaseHandle() in the future once there are more shape test types.
		void New_ReleaseProbe(New_phAsyncShapeTestHandle hHandle);

		void ThreadWorkerFn();

		// PURPOSE: Allow user code to set an optional callback function in order that additional code can be run as part of the asynchronous
		// worker thread update.
		typedef Functor0Ret<bool> WorkerThreadCallBackFunc;
		void SetWorkerThreadCallBackFunc(WorkerThreadCallBackFunc func);
		static bool DefaultWorkerThreadCallBackFunc() { return true; }

		void KickAsyncWorker();

		u32 m_iManagerThreadWakeUpFreqMs;

	protected:
		// Some terminology:
		// A "request index" refers to the index of where the data corresponding to given request is being stored.
		// A "generation ID" tells you how many times a given request slot has been used (modulo 65536).
		// A "shape test handle", which is passed back to clients, is a combination of a request index and a generation ID.

		// Available shape test handle management.
		New_phAsyncShapeTestHandle New_GetAvailableShapeTestHandle();
		void New_ReturnShapeTestHandle(New_phAsyncShapeTestHandle handle);

		u16 New_GetAvailableBackingIntersectionIndex();
		void New_ReturnBackingIntersectionIndex(u16 isectIndex);

		// Pending shape test request management.
		void New_PushPendingShapeTestRequest(New_phAsyncShapeTestHandle handle);
		New_phAsyncShapeTestHandle New_PopPendingShapeTestRequest();
		int New_GetNumPendingShapeTestRequests() const;

		// 
		inline bool New_IsIndexPlausible(New_phAsyncShapeTestHandle handle) const;

		int New_GetFreeShapeTestTaskIndex();

		void New_InitShapeTestTaskProbe(New_phShapeTestTask * pTask, const int iIndexInTask, New_phAsyncShapeTestHandle hRequestHandle);

		sysTaskHandle New_StartSingleShapeTestTask(New_phShapeTestTask * pTask, const int iSchedulerToUse);

		// Look for requests that have completed a while ago but haven't been released.
		void New_ReleaseTimedOutRequests();

		bool New_ProcessCompletedTasks();

		bool New_IsAsyncTaskComplete(New_phShapeTestTask * pTask) const;

		// Internally a New_phAsyncShapeTestHandle has the request index in the lower 16 bits and a 'generation ID' in the upper 16 bits, to
		//   distinguish between two different shape tests that happen to have used the same request slot (at different times).
		// This function strips off the generation ID from a shape test handle.
		// This function should *not* be made public as there should be no reason for clients to know/care about stripping off the generation ID
		//   and exposing it could cause clients to try and use it.
		inline u32 New_GetRequestIndex(New_phAsyncShapeTestHandle handle) const;

		// This function checks if a shape test handle is current, that is, its generation ID matches the generation ID for its request index.
		// Like above, there should be no reason to expose this function to clients.
		inline bool New_IsCurrent(New_phAsyncShapeTestHandle handle) const;

		New_phAsyncProbeRequest m_New_ProbeRequests[PHASYNCSHAPETESTMGR_MAXREQUESTS];
		// The generation ID for a given request index is incremented every time a handle is issued referring to that request.
		// This is to be able to distinguish between two different handles that have been issued (at different times) 
		u16 m_New_GenerationIDs[PHASYNCSHAPETESTMGR_MAXREQUESTS];

		// Stack/list (combined into one) for tracking which request indices are in use and which aren't.
		// [0, m_New_NumAvailableShapeTestHandles - 1] correspond to the request indices that are currently unused, and
		//   [m_New_NumAvailableShapeTestHandles, PHASYNCSHAPETESTMGR_MAXREQUESTS - 1] correspond to the indices that are currently being used.		
		// For tracking the available indices it can be a stack because, when giving out an index, we can always just pop from the head whatever
		//   index happens to be there and, when returning a index, we can just push it back onto the head.
		// The used indices, however, cannot be a stack because, when an index gets returned, we must find that *specific* index in the used
		//   section.
		// TODO: Rename this member to reflect the fact that it no longer tracks only the available indices, and that it tracks indices, not handles.
		// TODO: Also, we really only need to store u16's here.
		New_phAsyncShapeTestHandle m_New_AvailableHandles[PHASYNCSHAPETESTMGR_MAXREQUESTS];
		u16 m_New_NumAvailableShapeTestHandles;

		phIntersection m_New_BackingIntersections[PHASYNCSHAPETESTMGR_MAXBACKINGISECTS];

		// Stack for handing out backing intersection objects.
		u16 m_New_AvailableBackingIntersectionIndices[PHASYNCSHAPETESTMGR_MAXBACKINGISECTS];
		u16 m_New_NumAvailableBackingIntersections;

		// Queue for handling pending shape test requests (requests that have been submitted but haven't yet been started).
		// TODO: This really only needs to be tracking request indices, which only need 16 bits apiece.
		atQueue<New_phAsyncShapeTestHandle, PHASYNCSHAPETESTMGR_MAXPENDINGREQUESTS> m_New_PendingShapeTestRequests;

		New_phShapeTestTask m_New_ShapeTestTasks[PHASYNCSHAPETESTMGR_NUMTASKS];

		sysIpcSema m_UpdateSema;

		// This contains the task data for issuing these queries asynchronously.
		int m_iSchedulerIndex;

		// The system Id of the thread which marshals the shapetest tasks.
		sysIpcThreadId m_iThreadId;

		// An optional callback function so that game code can run additional code as part of the asynchronous worker thread update.
		WorkerThreadCallBackFunc m_WorkerThreadCallBackFunc;

		bool m_bWarnWhenShapeTestsFail;
		bool m_TimeOutsSuspended;
		bool m_NewTasksSuspended;

#if !__FINAL
		s32 m_iNumProbesRequestedThisFrame;
		s32 m_iNumShapeTestsIssuedThisFrame;
#endif
		sysCriticalSectionToken m_AsyncRequestsCriticalSectionToken;

		bool m_bQuit;
	};


	inline void phAsyncShapeTestMgr::New_SetSuspendTimeouts(const bool suspendTimeOuts)
	{
		m_TimeOutsSuspended = suspendTimeOuts;
	}

	inline bool phAsyncShapeTestMgr::New_GetSuspendTimeouts() const
	{
		return m_TimeOutsSuspended;
	}


	inline void phAsyncShapeTestMgr::New_SetSuspendNewTasks(const bool suspendNewTasks)
	{
		m_NewTasksSuspended = suspendNewTasks;
	}

	inline bool phAsyncShapeTestMgr::New_GetSuspendNewTasks() const
	{
		return m_NewTasksSuspended;
	}


	inline bool phAsyncShapeTestMgr::New_IsResultReady(const New_phAsyncShapeTestHandle hHandle) const
	{
		FastAssert(New_IsInUse(hHandle));
		const New_phAsyncProbeRequest &request = m_New_ProbeRequests[New_GetRequestIndex(hHandle)];
		return request.HasResult();
	}


	inline bool phAsyncShapeTestMgr::New_GetHitSomething(const New_phAsyncShapeTestHandle hHandle) const
	{
		FastAssert(New_IsInUse(hHandle));
		const New_phAsyncProbeRequest &request = m_New_ProbeRequests[New_GetRequestIndex(hHandle)];
		return request.HitSomething();
	}


	inline const phIntersection *phAsyncShapeTestMgr::New_GetDestIsect(const New_phAsyncShapeTestHandle hHandle) const
	{
		FastAssert(New_IsInUse(hHandle));
		const New_phAsyncProbeRequest &request = m_New_ProbeRequests[New_GetRequestIndex(hHandle)];
		return request.GetDestIntersections();
	}


	inline bool phAsyncShapeTestMgr::New_IsInUse(const New_phAsyncShapeTestHandle hHandle) const
	{
		FastAssert(New_IsIndexPlausible(hHandle));
		const New_phAsyncProbeRequest &request = m_New_ProbeRequests[New_GetRequestIndex(hHandle)];
		return New_IsCurrent(hHandle) && request.IsInUse();
	}


	inline bool phAsyncShapeTestMgr::New_IsCurrent(New_phAsyncShapeTestHandle handle) const
	{
		FastAssert(New_IsIndexPlausible(handle));
		u32 generationID = handle >> 16;
		return (generationID == (u32)(m_New_GenerationIDs[New_GetRequestIndex(handle)]));
	}


	inline bool phAsyncShapeTestMgr::New_IsIndexPlausible(New_phAsyncShapeTestHandle handle) const
	{
		// TODO: Maybe should there be more things I test for here?
		return New_GetRequestIndex(handle) < PHASYNCSHAPETESTMGR_MAXREQUESTS;
	}


	inline u32 phAsyncShapeTestMgr::New_GetRequestIndex(New_phAsyncShapeTestHandle handle) const
	{
		const u32 retVal = (handle & 0x0000FFFF);
		return retVal;
	}
};

PRE_DECLARE_THREAD_FUNC(ThreadEntryPointFn);

#endif	// PHYSICS_ASYNCSHAPETESTMGR_H
