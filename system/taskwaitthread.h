#ifndef	SYS_TASK_WAIT_THREAD_H
#define SYS_TASK_WAIT_THREAD_H

#include "system/task.h"

namespace rage {

#if 1 //__WIN32		// current implementation relies on functionality that doesn't exist on PS3

const int MAX_NUM_TASKS_IN_QUEUE = 128;


// a class to manage blocking on tasks completion.  This blocking can happens in it's own thread
// common useage pattern:
//
//	for( all tasks )
//	{
//		sysTaskWaitThread->CreateTask( TASK_INTERFACE(foo), sysparams );
//	}
//
//	sysTaskWaitThread->WakeUpWaitThread();
//	sysTaskWaitThread->WaitUntilAllTasksFinished();
//	sysTaskWaitThread->RemoveFinishedTasksFromQueue( NULL, NULL, 0 );
//
class sysTaskWaitThread
{
public:

	typedef enum { STATE_GOING_TO_INACTIVE, STATE_INACTIVE, STATE_TASKS_IN_FLIGHT, STATE_WAKING, STATE_WAITING_ON_TASK_COMPLETION } tTaskCoordinatorState;

	sysTaskWaitThread( int cpu = 0);

	~sysTaskWaitThread();

	// create a task and kick it off if defered parameter == false
	// if defered == true then queue it up to be kicked off when CreateDeferedTasks is called
	// returns the task handle if it was kicked off, NULL if specified as defered
	sysTaskHandle CreateTask( TASK_INTERFACE_PARAMS, sysTaskParameters *p, bool defered = false )
	{
		if( defered == false )
		{
			FastAssert( GetState() != STATE_WAITING_ON_TASK_COMPLETION );
			FastAssert( m_taskQueueTop < MAX_NUM_TASKS_IN_QUEUE );
			SetState( STATE_TASKS_IN_FLIGHT );

			m_taskHandles[ m_taskQueueTop ] = sysTaskManager::Create(TASK_INTERFACE_PARAMS_PASS,*p,m_scheduler);
			m_taskParameters[ m_taskQueueTop ] = p;
			m_taskQueueTop++;
			return m_taskHandles[ m_taskQueueTop-1 ];
		}
		else
		{
			m_deferedTaskParameters[m_nDefered] = p;
			m_nDefered++;
			return NULL;
		}
	}

	// create and kick off all tasks that were queued up for defered creation
	// return all task handles in the prassed array
	int CreateDeferedTasks( TASK_INTERFACE_PARAMS, int maxReturnedHandles, sysTaskHandle *returnedHandles )
	{
		int nRet = 0;
		int iDefer;
		for( iDefer = 0; iDefer < m_nDefered; iDefer++ )
		{
			if( maxReturnedHandles > iDefer )
			{
				nRet++;
				FastAssert( GetState() != STATE_WAITING_ON_TASK_COMPLETION );
				SetState( STATE_TASKS_IN_FLIGHT );
				returnedHandles[iDefer] = CreateTask( TASK_INTERFACE_PARAMS_PASS, m_deferedTaskParameters[iDefer], false );
			}
		}
		m_nDefered = 0;

		return nRet;
	}

	// wake up the thread that blocks on task completion with a call to sysTaskManager::WaitMultiple
	void WakeUpWaitThread();

	// block on caller thread until tasks finished ( blocks until block on other thread is done )
	void WaitUntilAllTasksFinished();

	// set the current state of the sysTaskWaitThread
	void SetState( tTaskCoordinatorState state )
	{
//		const char *stateStrings[4] = { "INACTIVE", "TASKS_IN_FLIGHT", "WAKING", "WAITING_ON_TASK_COMPLETION" };
//		Displayf( "state: %s -> %s", stateStrings[m_state], stateStrings[state] );
		m_state = state;
	}

	// return the current state of the sysTaskWaitThread
	tTaskCoordinatorState GetState() const
	{
		return m_state;
	}

	// request the waitTask thread to shutdown
	void RequestShutdown()
	{
		m_shouldShutdown = true;
	}

	// returns true if shutdown was requested
	bool ShutDownRequestRaised() const
	{
		return m_shouldShutdown;
	}

	// clear the shutdown request
	void ClearShutdownRequest()
	{
		m_shouldShutdown = false;
	}

	// the number of active tasks we have created.  defered tasks will not show up here
	int GetNumTasksInQueue() const
	{
		return m_taskQueueTop - m_taskQueueBottom;
	}

	// the active task queue
	sysTaskHandle *GetTaskQueue()
	{
		return m_taskHandles + m_taskQueueBottom;
	}

	// clear out any active tasks that have completed, calls sysTaskManager::Poll on each task
	int RemoveFinishedTasksFromQueue( sysTaskHandle *taskHandlesOut, sysTaskParameters **taskParametersOut, int maxNumTaskInfoOut );

	// set all active tasks to completed
	void TaskQueueDone()
	{
		m_taskQueueBottom = m_taskQueueTop;
	}

	// the ID of the wait task thread
	sysIpcThreadId GetThreadId()
	{
		return m_threadId;
	}

	sysIpcSema GetWaiter()
	{
		return m_pauseWaiter;
	}

	
	// data
	sysTaskHandle m_taskHandles[MAX_NUM_TASKS_IN_QUEUE];
	sysTaskParameters *m_taskParameters[MAX_NUM_TASKS_IN_QUEUE];
	sysTaskParameters *m_deferedTaskParameters[MAX_NUM_TASKS_IN_QUEUE];
	int m_nDefered;
	tTaskCoordinatorState m_state;
	bool m_shouldShutdown;
	int m_taskQueueBottom;
	int m_taskQueueTop;
	sysIpcThreadId m_threadId;
	sysIpcSema m_pauseWaiter;
	int m_scheduler;

};

#endif	// __WIN32

} // namespace rage

#endif  // SYS_TASK_WAIT_THREAD_H
