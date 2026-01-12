#include "asyncshapetestmgr.h"

// Rage headers
#include "atl/functor.h"
#include "intersection.h"
#include "math/amath.h"
#include "system/timemgr.h"
#include "system/task.h"

#if __PPU
// Needed for sys_timer_usleep().
#include "sys/timer.h"
#endif


using namespace rage;


#define ASYNCSHAPETESTMGR_LOGGING_ENABLED	0

#if ASYNCSHAPETESTMGR_LOGGING_ENABLED
class LogEntry
{
public:
	u32 m_Event;
	u32 m_Data;
};


// Must be a power of two.
const int kLogCount = 128;

class Log
{
public:
	Log()
	{
		m_NextEntry = 0;
	}

	void Add(u32 event, u32 data)
	{
		m_Logs[m_NextEntry].m_Event = event;
		m_Logs[m_NextEntry].m_Data = data;
		m_NextEntry = (m_NextEntry + 1) & (kLogCount - 1);
	}

	u32 m_NextEntry;
	LogEntry m_Logs[kLogCount];
};

static Log s_Log;
#define LOG(x,y)	s_Log.Add(x,y)
#else	// ASYNCSHAPETESTMGR_LOGGING_ENABLED
#define LOG(x,y)
#endif	// ASYNCSHAPETESTMGR_LOGGING_ENABLED


phAsyncShapeTestMgr::phAsyncShapeTestMgr() :
m_New_NumAvailableBackingIntersections(PHASYNCSHAPETESTMGR_MAXBACKINGISECTS)
, m_iThreadId(0)
, m_iManagerThreadWakeUpFreqMs(1)
#if !__FINAL
, m_iNumProbesRequestedThisFrame(0)
, m_iNumShapeTestsIssuedThisFrame(0)
#endif
, m_WorkerThreadCallBackFunc(MakeFunctorRet(phAsyncShapeTestMgr::DefaultWorkerThreadCallBackFunc))
, m_bWarnWhenShapeTestsFail(false)
, m_TimeOutsSuspended(false)
, m_NewTasksSuspended(false)
{
}


phAsyncShapeTestMgr::~phAsyncShapeTestMgr()
{
}


void phAsyncShapeTestMgr::Init(const int schedulerIndex, const unsigned stackSize)
{
	// See if the client provided a scheduler to use.  If not, we'll create our own.
	if(schedulerIndex == -1)
	{
#if !__PPU	// PS3 doesn't have task schedulers
		// JB : needs a large stack size for shapetests
		static const int iStackSize = 65536;
		static const int iScratchSize = 32768;

		// This priority needs to be PRIO_ABOVE_NORMAL or these shapetests don't get a look-in, even with all cores in the bitmask.
		// AF: Reduced this priority to normal. These tasks cannot interrupt the physics and anim tasks as that will cause the
		// game to slow down.
		static const rage::sysIpcPriority iTaskPriority = PRIO_ABOVE_NORMAL;	// PRIO_NORMAL

		//AssertMsg(WORLDPROBEASYNC_NUMTASKS==1, "Number of schedulers has changed - you will have to alter the core usage accordingly");
		// NB : Avoid HW thread 3 which had the render thread on it
		// When we are using the manager thread, then avoid the main game thread 0
		const u32 iThreadMask = (1<<1) | (1<<2);// | (1<<3) | (1<<4) | (1<<5)

		m_iSchedulerIndex = sysTaskManager::AddScheduler("WldProbeAsyncSched", iStackSize, iScratchSize, iTaskPriority, iThreadMask, 0);
#else
		m_iSchedulerIndex = 0;
#endif // !__PSN
	}
	else
	{
		// PS3 still needs a scheduler index to known which SPU job chain to use
		m_iSchedulerIndex = schedulerIndex;
	}

	int iCpuId = 1;

	m_UpdateSema = sysIpcCreateSema(0);

	m_bQuit = false;

	m_iThreadId = sysIpcCreateThread(ThreadEntryPointFn, this, stackSize, PRIO_BELOW_NORMAL, "AsyncShapeTestMgr", iCpuId, "AsyncShapeTestMgr");

	// NEW STUFF
	for(int i = PHASYNCSHAPETESTMGR_MAXREQUESTS - 1; i >= 0; --i)
	{
		m_New_AvailableHandles[i] = i;
	}
	sysMemZeroBytes<PHASYNCSHAPETESTMGR_MAXREQUESTS * 2>(m_New_GenerationIDs);
	m_New_NumAvailableShapeTestHandles = PHASYNCSHAPETESTMGR_MAXREQUESTS;
	for(int i = PHASYNCSHAPETESTMGR_MAXBACKINGISECTS - 1; i >= 0; --i)
	{
		m_New_AvailableBackingIntersectionIndices[i] = (u16)(i);
	}
	m_New_NumAvailableBackingIntersections = PHASYNCSHAPETESTMGR_MAXBACKINGISECTS;
}


void phAsyncShapeTestMgr::Shutdown()
{
	// Should we be doing something else here?
	m_bQuit = true;
	sysIpcSignalSema(m_UpdateSema);
	sysIpcWaitThreadExit(m_iThreadId);
	m_iThreadId = 0;
}


New_phAsyncShapeTestHandle phAsyncShapeTestMgr::New_SubmitProbe(phIntersection *destIsects, const u32 numIsects, Vec3V_In vStart, Vec3V_In vEnd, 
										   const u32 stateIncludeFlags, const u32 typeFlags, const u32 typeIncludeFlags, const u32 typeExcludeFlags, 
										   const phInst *const *excludeInstList, const u32 excludeInstCount, u8 userData)
{
	Assertf(excludeInstCount < MAX_EXCLUDE_INSTANCES, "Number of exclude instances (%d) must be less than maximum (%d)", excludeInstCount, MAX_EXCLUDE_INSTANCES);
	Assertf(IsLessThanAll(Mag(vStart),ScalarV(V_FLT_LARGE_8)),"New_SubmitProbe has an impractical segment starting point %f, %f, %f",vStart.GetElemf(0),vStart.GetElemf(1),vStart.GetElemf(2));
	Assertf(IsLessThanAll(Mag(vEnd),ScalarV(V_FLT_LARGE_8)),"New_SubmitProbe has an impractical segment ending point %f, %f, %f",vEnd.GetElemf(0),vEnd.GetElemf(1),vEnd.GetElemf(2));

	// NOTE: Strictly speaking, the two critical sections here don't have to reference the same token - it's okay to be modifying one of the data
	//   data structures while somebody else is modifying the other.
	sysCriticalSection requestsCritSec(m_AsyncRequestsCriticalSectionToken);
	New_phAsyncShapeTestHandle newShapeTestHandle = New_GetAvailableShapeTestHandle();

	// Note: It used to be the case that this would Exit() here, but that looked unsafe to me.
	// Then, I don't know what would protect the backing intersections from being assigned
	// at the same time by two threads. Before, it would also grab the critical section token again
	// briefly. I changed this because I think it's just safer and most likely not significantly
	// slower to just hold on to the critical section token. /FF
	//	requestsCritSec.Exit();

	if(newShapeTestHandle != InvalidShapeTestHandle)
	{
		// We've successfully been issued a shape test handle.
		if(destIsects == NULL && numIsects == 1)
		{
			// They want us to provide the storage for the intersection results, let's see if we can accommodate that.
			u16 backingIntersectionIndex = New_GetAvailableBackingIntersectionIndex();
			if(backingIntersectionIndex == (u16)(-1))
			{
				New_ReturnShapeTestHandle(newShapeTestHandle);

				// I added this, because we no longer Exit() above. /FF
				requestsCritSec.Exit();

				return InvalidShapeTestHandle;
			}
			destIsects = &m_New_BackingIntersections[backingIntersectionIndex];
		}
		New_phAsyncProbeRequest &newRequest = m_New_ProbeRequests[New_GetRequestIndex(newShapeTestHandle)];
		newRequest.Init(destIsects, numIsects, vStart, vEnd, stateIncludeFlags, typeFlags, typeIncludeFlags, typeExcludeFlags, excludeInstList, excludeInstCount, userData);

		// Removed - we already hold the critical section token now. /FF
		//		requestsCritSec.Enter();

		New_PushPendingShapeTestRequest(newShapeTestHandle);

		// Removed, we'll exit below now. /FF
		//		requestsCritSec.Exit();
	}

	// I added this, as we now hold on to the token the whole time. /FF
	requestsCritSec.Exit();

	LOG(8, newShapeTestHandle);
	return newShapeTestHandle;
}


void phAsyncShapeTestMgr::New_ReleaseProbe(New_phAsyncShapeTestHandle hHandle)
{
	sysCriticalSection requestsCritSec(m_AsyncRequestsCriticalSectionToken);

	Assertf(New_IsIndexPlausible(hHandle), "%x", hHandle);
	New_phAsyncProbeRequest &request = m_New_ProbeRequests[New_GetRequestIndex(hHandle)];
	Assert(request.IsInUse());

	LOG(0, hHandle);
	// The probe we are releasing might be queued up on the request queue
	// yet, without any task starting to process it yet. The call to
	// HasResult() here is just so that we don't have to call a somewhat
	// expensive atQueue::Find() in the common case of releasing a probe
	// handle that has already been processed. /FF
	if(!request.HasResult())
	{
		LOG(1, hHandle);
		// Check to see if the request is in the queue, and if so,
		// delete it. /FF
		int index = -1;
		if(m_New_PendingShapeTestRequests.Find(hHandle, &index))
		{
			LOG(2, hHandle);
			m_New_PendingShapeTestRequests.Delete(index);
		}
		else
		{
			// The probe is currently being tested. Mark it for release when it is finished. 
			request.SetReleaseWhenFinished();
			return;
		}
	}
	else
	{
		ASSERT_ONLY(int index = -1);
		Assert(!m_New_PendingShapeTestRequests.Find(hHandle, &index));
	}
	LOG(3, hHandle);

	// I added this part. It checks to see if the destination intersection came
	// from our array, i.e. was allocated by a call to New_GetAvailableBackingIntersectionIndex()
	// from New_SubmitProbe(). In that case, we need to return it. The user can't do it,
	// because New_ReturnBackingIntersectionIndex() is protected and also for symmetry reasons
	// - he/she wasn't who called New_GetAvailableBackingIntersectionIndex() in the first place. /FF
	phIntersection *isect = request.GetDestIntersections();
	if(isect >= m_New_BackingIntersections && isect < m_New_BackingIntersections + PHASYNCSHAPETESTMGR_MAXBACKINGISECTS)
	{
		// Compute the index of the intersection. /FF
		const int index = (int) (isect - m_New_BackingIntersections);
		Assert(index >= 0 && index < PHASYNCSHAPETESTMGR_MAXBACKINGISECTS);
		New_ReturnBackingIntersectionIndex((u16)index);
	}

	// Note: Even with the code above for removing the request if no task has
	// started to work on it yet, I'm not convinced that this is all safe if you
	// cancel a task that's in progress. I think what happens is that the task
	// just continues to do the probe. This may be fine because it may be cheaper
	// to let the probe finish than to add more thread communication and have this
	// calling thread stall, but I suspect that there are some safety issues when
	// processing the result (possibly including writing to a phIntersection belonging
	// to the user who wanted to cancel the probe by calling New_ReleaseProbe(). /FF

	New_ReturnShapeTestHandle(hHandle);

	// I moved this. It used to be done after the Exit() below, but that seems to
	// leave the possibility open that another thread has been assigned this
	// request and already started using it. In that case, the Clear() would presumably
	// be a bad thing to do. /FF
	request.Clear();

	requestsCritSec.Exit();
}


// Releases a handle from the pool of available handles.  DOES NOT mark the corresponding request as in use.
New_phAsyncShapeTestHandle phAsyncShapeTestMgr::New_GetAvailableShapeTestHandle()
{
	if(m_New_NumAvailableShapeTestHandles <= 0)
	{
		// Release any requests that completed some time ago but haven't been released.
		// Do this here when a new shapetest handle is requested and we don't have any left, 
		// so that we only take the overhead cost of checking all the requests to see if they've 
		// timed out when we absolutely need to do this check.
		New_ReleaseTimedOutRequests();
	}

	if(m_New_NumAvailableShapeTestHandles > 0)
	{
		--m_New_NumAvailableShapeTestHandles;
		New_phAsyncShapeTestHandle newHandle = m_New_AvailableHandles[m_New_NumAvailableShapeTestHandles];
		Assert((newHandle & 0x0000FFFF) == newHandle);		// Handles should come out of the available handles list with their generation ID set to zero.
		Assert(!m_New_ProbeRequests[newHandle].IsInUse());

		// Mix in the generation ID for this request.
		++m_New_GenerationIDs[newHandle];
		newHandle = newHandle | ((u32)(m_New_GenerationIDs[newHandle]) << 16);

		return newHandle;
	}
	else
	{
		Warningf("phAsyncShapeTestMgr - Ran out of async shape test handles");
		return InvalidShapeTestHandle;
	}
}


// Returns a handle to the pool of available handles.  DOES NOT mark the corresponding request as not in use.
void phAsyncShapeTestMgr::New_ReturnShapeTestHandle(New_phAsyncShapeTestHandle handle)
{
	Assert(m_New_NumAvailableShapeTestHandles < PHASYNCSHAPETESTMGR_MAXREQUESTS);
	Assert(New_IsInUse(handle));

	// Find the handle being returned in the "used handles" 
	int usedHandleLocation = PHASYNCSHAPETESTMGR_MAXREQUESTS - 1;
	const u32 requestIndex = New_GetRequestIndex(handle);
	while(m_New_AvailableHandles[usedHandleLocation] != requestIndex && usedHandleLocation > m_New_NumAvailableShapeTestHandles)
	{
		--usedHandleLocation;
	}

	Assert(m_New_AvailableHandles[usedHandleLocation] == requestIndex);

	SwapEm(m_New_AvailableHandles[m_New_NumAvailableShapeTestHandles], m_New_AvailableHandles[usedHandleLocation]);
	++m_New_NumAvailableShapeTestHandles;
}


u16 phAsyncShapeTestMgr::New_GetAvailableBackingIntersectionIndex()
{
	if(m_New_NumAvailableBackingIntersections > 0 && m_New_NumAvailableBackingIntersections <= PHASYNCSHAPETESTMGR_MAXBACKINGISECTS)
	{
		--m_New_NumAvailableBackingIntersections;
		u16 newIsectIndex = m_New_AvailableBackingIntersectionIndices[m_New_NumAvailableBackingIntersections];
		return newIsectIndex;
	}
	else
	{
		Warningf("phAsyncShapeTestMgr - Ran out of backing intersections!");
		return (u16)(-1);
	}
}


void phAsyncShapeTestMgr::New_ReturnBackingIntersectionIndex(u16 isectIndex)
{
	Assert(m_New_NumAvailableBackingIntersections < PHASYNCSHAPETESTMGR_MAXBACKINGISECTS);
	Assert(isectIndex < PHASYNCSHAPETESTMGR_MAXBACKINGISECTS);
	m_New_AvailableBackingIntersectionIndices[m_New_NumAvailableBackingIntersections] = isectIndex;
	++m_New_NumAvailableBackingIntersections;
}


void phAsyncShapeTestMgr::New_PushPendingShapeTestRequest(New_phAsyncShapeTestHandle handle)
{
	Assert(!m_New_ProbeRequests[New_GetRequestIndex(handle)].HasResult());
	LOG(4, handle);
	m_New_PendingShapeTestRequests.Push(handle);
}


New_phAsyncShapeTestHandle phAsyncShapeTestMgr::New_PopPendingShapeTestRequest()
{
	New_phAsyncShapeTestHandle handle = m_New_PendingShapeTestRequests.Pop();
	LOG(5, handle);
	return handle;
}


int phAsyncShapeTestMgr::New_GetNumPendingShapeTestRequests() const
{
	return m_New_PendingShapeTestRequests.GetCount();
}


int phAsyncShapeTestMgr::New_GetFreeShapeTestTaskIndex()
{
	for(int t = 0; t < PHASYNCSHAPETESTMGR_NUMTASKS; t++)
	{
		const New_phShapeTestTask * pTask = &m_New_ShapeTestTasks[t];
		Assert((pTask->m_hTaskHandle != NULL) == (pTask->m_iNumProbes > 0));
		if(pTask->m_iNumProbes == 0)
			return t;
	}
	return -1;
}


void phAsyncShapeTestMgr::New_InitShapeTestTaskProbe(New_phShapeTestTask *pTask, const int iIndexInTask, New_phAsyncShapeTestHandle hRequestHandle)
{
	Assert(iIndexInTask >= 0 && iIndexInTask < New_phShapeTestTask::PHASYNCSHAPETESTMGR_MAXPROBESPERTASK);
	Assert(iIndexInTask == pTask->m_iNumProbes);			// The fact that we increment pTask->m_iNumProbes below seems to imply that this must be the case.
	phShapeTest<phShapeProbe> * pShapeTest = &pTask->m_probeShapeTests[iIndexInTask];

	// We have to know which handle is associated with which task probe, for retrieving the results later
	// We need the request handle because we need to mark the request as completed.
	// TODO: Should double check that this isn't resulting in an integer division (shouldn't if the class size is a power of two).
	pTask->m_hHandle[iIndexInTask] = hRequestHandle;
	LOG(7, hRequestHandle);
	New_phAsyncProbeRequest &curRequest = m_New_ProbeRequests[New_GetRequestIndex(hRequestHandle)];
	Assert(curRequest.IsInUse());
	Assert(!curRequest.HasResult());
	curRequest.InitShapeTest(pShapeTest);

	pTask->m_iNumProbes++;
	Assert(pTask->m_iNumProbes <= New_phShapeTestTask::PHASYNCSHAPETESTMGR_MAXPROBESPERTASK);
}


sysTaskHandle phAsyncShapeTestMgr::New_StartSingleShapeTestTask(New_phShapeTestTask * pTask, const int iSchedulerToUse)
{
	Assert(pTask->m_iNumProbes > 0);
//	if(pTask->m_iNumProbes > 0)
	{

#if !__FINAL
		m_iNumShapeTestsIssuedThisFrame += pTask->m_iNumProbes;
#endif

		// Set up the async task data
		pTask->m_shapeTestTaskData.Init();
		pTask->m_shapeTestTaskData.SetProbes(pTask->m_probeShapeTests, pTask->m_iNumProbes);

		// Now actually create the task & get a handle to it
		Assert(iSchedulerToUse != -1);
		pTask->m_hTaskHandle = pTask->m_shapeTestTaskData.CreateTask(iSchedulerToUse);
	}

	return pTask->m_hTaskHandle;
}


void phAsyncShapeTestMgr::New_ReleaseTimedOutRequests()
{
	// Check all of the completed requests and see if it's been long enough since each one completed.  If it has, consider the request abandoned
	//   and release it.
	const float curTime = TIME.GetUnpausedElapsedTime();
	for(New_phAsyncShapeTestHandle requestIndex = 0; requestIndex < PHASYNCSHAPETESTMGR_MAXREQUESTS; ++requestIndex)
	{
		New_phAsyncProbeRequest &curRequest = m_New_ProbeRequests[requestIndex];
		if(!New_GetSuspendTimeouts())
		{
			if(Unlikely(curRequest.IsInUse() && curRequest.HasResult()))
			{
				const float requestCompletedTime = curRequest.GetCompletionTime();
				if(Unlikely(curTime - requestCompletedTime > PHASYNCSHAPETESTMGR_TIMEOUTMS * 0.001f))
				{
					// New_ReleaseProbe() expects a handle so construct the handle from the request index and generation ID.
					// Since, ultimately, New_ReleaseProbe() is going to convert the handle back into a request index we could get around this by
					//   having a New_ReleaseProbeInternal() that takes a request index.
					New_phAsyncShapeTestHandle handle = requestIndex | ((u32)(m_New_GenerationIDs[requestIndex]) << 16);
					Warningf("phAsyncShapeTestMgr - Releasing probe request with handle %d because it has timed out.", handle);
					New_ReleaseProbe(handle);
				}
			}
		}
		else
		{
			// Pretend like the request was just completed now.
			curRequest.SetCompletionTime(curTime);
		}
	}
}


bool phAsyncShapeTestMgr::New_ProcessCompletedTasks()
{
	bool bTaskPending = false;
	for(int t = 0; t < PHASYNCSHAPETESTMGR_NUMTASKS; t++)
	{
		New_phShapeTestTask * pTask = &m_New_ShapeTestTasks[t];

		// Check if the task was ever started.
		if(pTask->m_hTaskHandle != NULL)
		{
			if(!New_IsAsyncTaskComplete(pTask))
			{
				bTaskPending = true;
				continue;
			}

			for(int probeIndex = 0; probeIndex < pTask->m_iNumProbes; ++probeIndex)
			{
				const New_phAsyncShapeTestHandle hCurHandle = pTask->m_hHandle[probeIndex];
				Assert(New_GetRequestIndex(hCurHandle) < PHASYNCSHAPETESTMGR_MAXREQUESTS);
				Assert(New_IsCurrent(hCurHandle));

				New_phAsyncProbeRequest &curRequest = m_New_ProbeRequests[New_GetRequestIndex(hCurHandle)];
				if(pTask->m_probeShapeTests[probeIndex].GetShape().GetNumHits() > 0)
				{
					curRequest.SetHitSomething();
				}
				curRequest.SetCompletionTime(TIME.GetUnpausedElapsedTime());

				// Mark this last since that's the signal that the result's worth looking at.
				LOG(6, hCurHandle);
				curRequest.SetHasResult();

				// If the request was marked for release when finished, release it here.
				if(curRequest.ReleaseWhenFinished())
				{
					// This must be done after SetHasResult, or New_ReleaseProbe won't do anything.
					New_ReleaseProbe(hCurHandle);
				}
			}

			pTask->m_iNumProbes = 0;
			pTask->m_hTaskHandle = 0;
		}
	}

	return bTaskPending;
}


bool phAsyncShapeTestMgr::New_IsAsyncTaskComplete(New_phShapeTestTask * pTask) const
{
	Assert(pTask->m_hTaskHandle);
//	if(pTask->m_hTaskHandle)
//	{
		return sysTaskManager::Poll(pTask->m_hTaskHandle);
//	}
//	else
//	{
//		// Handle should always be valid - but if not then allow task to be reused immediately
//		return true;
//	}
}


void phAsyncShapeTestMgr::SetWorkerThreadCallBackFunc(WorkerThreadCallBackFunc func)
{
	m_WorkerThreadCallBackFunc = func;
}


DECLARE_THREAD_FUNC(ThreadEntryPointFn)
{
	Assert(ptr != NULL);
	phAsyncShapeTestMgr *shapeTestMgr = (phAsyncShapeTestMgr *)(ptr);
	shapeTestMgr->ThreadWorkerFn();
}


void phAsyncShapeTestMgr::ThreadWorkerFn()
{
	Assert(this != NULL);

	while(!m_bQuit)
	{
		sysIpcWaitSema(m_UpdateSema);
		while(!m_bQuit)
		{
			bool bCallbackDone = m_WorkerThreadCallBackFunc();

			////////////////////////////////////////////////////////////////////////////////////////////////////
			// NEW STUFF:

			sysCriticalSection New_requestsCritSec(m_AsyncRequestsCriticalSectionToken);

			// Look for tasks that have completed and mark the requests that they were processing as ready.
			bool bTasksPending = New_ProcessCompletedTasks();

			int New_NumPendingRequests = New_GetNumPendingShapeTestRequests();

			// Used to be done here, but it was moved because otherwise it's presumably
			// not really safe to use New_NumPendingRequests to see if something is queued
			// up (because the request may have been cancelled by the time we are popping
			// from the queue). /FF
			//	New_requestsCritSec.Exit();
			for (int iNewTask=0; iNewTask < PHASYNCSHAPETESTMGR_NUMTASKS; iNewTask++)
			{
				if (New_NumPendingRequests > 0 && !m_NewTasksSuspended)
				{
					const int iTaskIndex = New_GetFreeShapeTestTaskIndex();
					if (iTaskIndex != -1)
					{
						// We've got at least one pending request and a task with which to process, let's get things started.
						New_phShapeTestTask *pTask = &m_New_ShapeTestTasks[iTaskIndex];
						Assert(pTask->m_iNumProbes == 0);

						// Tear through the pending requests and assign them to this task.
						const int kNumRequestsToProcess = Min(New_NumPendingRequests, (int)(New_phShapeTestTask::PHASYNCSHAPETESTMGR_MAXPROBESPERTASK));
						New_NumPendingRequests -= kNumRequestsToProcess;
						//Displayf("[%d] Going to grab %d requests to process!", iTaskIndex, kNumRequestsToProcess);
						for(int shapeTestInTaskIndex = 0; shapeTestInTaskIndex < kNumRequestsToProcess; ++shapeTestInTaskIndex)
						{
							New_phAsyncShapeTestHandle handleToInitiate = New_PopPendingShapeTestRequest();

							Assert(New_IsIndexPlausible(handleToInitiate));
							ASSERT_ONLY(New_phAsyncProbeRequest &requestToInitiate = m_New_ProbeRequests[New_GetRequestIndex(handleToInitiate)];)
							Assert(requestToInitiate.IsInUse());

							New_InitShapeTestTaskProbe(pTask, shapeTestInTaskIndex, handleToInitiate);
						}

						New_StartSingleShapeTestTask(pTask, m_iSchedulerIndex);
						bTasksPending = true;
					}
					else
					{
						// No more tasks available.
						break;
					}
				}
			}

			bool bDoneProcessingForNow = (bCallbackDone && !bTasksPending && New_NumPendingRequests == 0);

			// This is the new location for the Exit() that used to be done above, right after
			// the New_GetNumPendingShapeTestRequests() call. /FF
			New_requestsCritSec.Exit();

			//
			////////////////////////////////////////////////////////////////////////////////////////////////////

			if(bDoneProcessingForNow)
			{
				break;
			}

			// Wait for a request to be passed into the system, or for timeout to expire
			sysIpcSleep(m_iManagerThreadWakeUpFreqMs);
		}
	}
}

void phAsyncShapeTestMgr::KickAsyncWorker()
{
	// Signal the update semaphore so that the worker thread wakes up and starts processing.
	// This way the user has total control over when the worker thread runs by simply calling 
	// the StartProcessing() method.
	sysIpcSignalSema(m_UpdateSema);
};

//**************************************************************************
