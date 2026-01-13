// 
// system/taskwaitthread.cpp
// 
// Copyright (C) 1999-2006 Rockstar Games.  All Rights Reserved. 
// 

#include "system/taskwaitthread.h"
#include "system/memory.h"
#include "system/alloca.h"

namespace rage {

#if 1 //__WIN32

// Give'er!
DECLARE_THREAD_FUNC(ThreadOfWaitingForTasks)
{
	sysTaskWaitThread *doer = ( sysTaskWaitThread * )ptr;

	if( doer->GetState() != sysTaskWaitThread::STATE_TASKS_IN_FLIGHT )
	{
		doer->SetState( sysTaskWaitThread::STATE_GOING_TO_INACTIVE );
	}

//	sysIpcSuspendThread(doer->GetThreadId());
	sysIpcWaitSema( doer->m_pauseWaiter );

	while( !doer->ShutDownRequestRaised() )
	{
		doer->SetState( sysTaskWaitThread::STATE_WAITING_ON_TASK_COMPLETION );

		sysTaskManager::WaitMultiple( doer->GetNumTasksInQueue(), doer->GetTaskQueue());

		doer->TaskQueueDone();
		doer->SetState( sysTaskWaitThread::STATE_GOING_TO_INACTIVE );
//		sysIpcSuspendThread(doer->GetThreadId());
		sysIpcWaitSema( doer->m_pauseWaiter );

	}

	doer->ClearShutdownRequest();

}


void sysTaskWaitThread::WakeUpWaitThread()
{
	
	//a bit anal... Assert( GetState() != STATE_INACTIVE );

	if( GetState() == STATE_GOING_TO_INACTIVE )
	{
		sysIpcYield(0);
		SetState( STATE_INACTIVE );
	}

	if( (GetState() == STATE_INACTIVE) || (GetState() == STATE_TASKS_IN_FLIGHT) )
	{
		SetState( STATE_WAKING );
		sysIpcSignalSema( m_pauseWaiter );
//		sysIpcResumeThread(m_threadId);
	}
}


void sysTaskWaitThread::WaitUntilAllTasksFinished()
{

	//!me this the right thing to do?
	while( !(GetState() == STATE_GOING_TO_INACTIVE) )
	{
		sysIpcYield(0);
	}

	SetState( STATE_INACTIVE );
}

int sysTaskWaitThread::RemoveFinishedTasksFromQueue( sysTaskHandle *taskHandlesOut, sysTaskParameters **taskParametersOut, int maxNumTaskInfoOut )
{
	// 
	Assert( GetState() == sysTaskWaitThread::STATE_INACTIVE );

	if( GetState() != sysTaskWaitThread::STATE_INACTIVE )
	{
		return 0;
	}

	int nTask = m_taskQueueTop - m_taskQueueBottom;

	int iTask;
	for( iTask = 0; iTask < m_taskQueueBottom && iTask < maxNumTaskInfoOut; iTask++ )
	{
		if( maxNumTaskInfoOut > iTask )
		{
			taskHandlesOut[iTask] = m_taskHandles[iTask];
			taskParametersOut[iTask] = m_taskParameters[iTask];
		}
	}

	bool *tasksDone = Alloca( bool, nTask );
	int nTaskDone = 0;
	sysMemSet( tasksDone, 0, (nTask)*sizeof( bool ) );

	for( iTask = m_taskQueueBottom; iTask < m_taskQueueTop; iTask++ )
	{
		if( sysTaskManager::Poll( m_taskHandles[iTask] ) )
		{
			if( maxNumTaskInfoOut > iTask )
			{

				taskHandlesOut[iTask] = m_taskHandles[iTask]; 
				taskParametersOut[iTask] = m_taskParameters[iTask];
			}
			tasksDone[iTask-m_taskQueueBottom] = true;
			nTaskDone++;
		}
	}

	int iTaskRemain = 0;

	// compact list and move to start
	for( iTask = m_taskQueueBottom; iTask < m_taskQueueTop; iTask++ )
	{
		if( tasksDone[iTask-m_taskQueueBottom] == false )
		{
			m_taskParameters[iTaskRemain] = m_taskParameters[iTask];
			m_taskHandles[iTaskRemain] = m_taskHandles[iTask];
			iTaskRemain++;
		}
	}

	Assert( iTaskRemain == nTask - nTaskDone );

	m_taskQueueBottom = 0;
	m_taskQueueTop = iTaskRemain;

	return nTaskDone;

}


sysTaskWaitThread::sysTaskWaitThread( int cpu )
{
	int iTask;
	for( iTask = 0; iTask < MAX_NUM_TASKS_IN_QUEUE; iTask++ )
	{
		m_taskHandles[ iTask ] = NULL;
	}

	m_state = STATE_INACTIVE;
	m_shouldShutdown = false;
	m_taskQueueBottom = 0;
	m_taskQueueTop = 0;
	m_nDefered = 0;
	m_scheduler = 0;
	m_pauseWaiter = sysIpcCreateSema(true);

	m_threadId = sysIpcCreateThread( ThreadOfWaitingForTasks, this, sysIpcMinThreadStackSize, PRIO_ABOVE_NORMAL, "sysTaskWaitThread", cpu );

}

sysTaskWaitThread::~sysTaskWaitThread()
{
	sysIpcWaitThreadExit( m_threadId );
	sysIpcWaitSema( m_pauseWaiter );
	sysIpcDeleteSema( m_pauseWaiter );
}


#endif	// __WIN32

} // namespace rage
