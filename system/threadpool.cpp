// 
// system/threadpool.cpp
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#include "threadpool.h"
#include "file/file_config.h"

#if RSG_PC || RSG_DURANGO
#pragma warning(disable: 4668)
#include <windows.h>
#pragma warning(error: 4668)
#endif

namespace rage
{
//////////////////////////////////////////////////////////////////////////
//  sysThreadPool::Thread
//////////////////////////////////////////////////////////////////////////

sysThreadPool::Thread::Thread()
: m_Owner(NULL)
, m_ThreadId(sysIpcThreadIdInvalid)
, m_State(STATE_NONE)
, m_WaitSema(0)
, m_Stop(false)
{
}

sysThreadPool::Thread::~Thread()
{
	// if something calls ExitProcess, then the threads will terminate before this
	// function gets called by the C-runtime's atexit() process, and a deadlock occurs.
	// Instead, in the Social Club DLL, we call ShutdownNetThreadPool() manually during
	// our normal shutdown procedure. If ExitProcess() is called by the game, we know
	// the threads are already shutdown, so don't try to do it again here.
#if !__RGSC_DLL
    Assert(!this->Running());

    if(m_Owner)
    {
        m_Owner->RemoveThread(this);
    }

    Assert(!m_Owner);
    Assert(sysIpcThreadIdInvalid == m_ThreadId);
    Assert(!m_WaitSema);
#endif
}

bool
sysThreadPool::Thread::Running() const
{
    return STATE_RUNNING == m_State;
}

bool
sysThreadPool::Thread::Stopped() const
{
    return STATE_STOPPED == m_State;
}

bool
	sysThreadPool::Thread::IsAlive() const
{
	#if RSG_PC || RSG_DURANGO 
	return WaitForSingleObject(m_ThreadId, 0) != WAIT_OBJECT_0;
	#else
	//TODO: Figure out how to check live-ality for PS4 threads.
	return true;
	#endif
}

//////////////////////////////////////////////////////////////////////////
//  sysThreadPool::WorkItem
//////////////////////////////////////////////////////////////////////////

sysThreadPool::WorkItem::WorkItem()
: m_State(STATE_NONE)
, m_Id(INVALID_WORK_ITEM_ID)
, m_WasCanceled(false)
{
}

sysThreadPool::WorkItem::~WorkItem()
{
    Assert(!this->Pending());
    while(this->Pending())
    {
        sysIpcSleep(1);
    }
}

unsigned
sysThreadPool::WorkItem::GetId() const
{
    return this->Pending() ? m_Id : INVALID_WORK_ITEM_ID;
}

bool
sysThreadPool::WorkItem::Pending() const
{
    return (STATE_QUEUED == m_State || STATE_ACTIVE == m_State);
}

bool
sysThreadPool::WorkItem::Active() const
{
	return (STATE_ACTIVE == m_State);
}

bool
sysThreadPool::WorkItem::Queued() const
{
	return (STATE_QUEUED == m_State);
}

bool
sysThreadPool::WorkItem::Finished() const
{
    return (STATE_FINISHED == m_State);
}

bool
sysThreadPool::WorkItem::WasCanceled() const
{
    return m_WasCanceled;
}

void
sysThreadPool::WorkItem::Finish()
{
	Assert(this->Pending());
	m_State = STATE_FINISHED;
}

//////////////////////////////////////////////////////////////////////////
//  sysThreadPool
//////////////////////////////////////////////////////////////////////////
sysThreadPool::sysThreadPool()
: m_NextId(0)
{
}

sysThreadPool::~sysThreadPool()
{
	// if something calls ExitProcess, then the threads will terminate before this
	// function gets called by the C-runtime's atexit() process, and a deadlock occurs.
	// Instead, in the Social Club DLL, we call ShutdownNetThreadPool() manually during
	// our normal shutdown procedure. If ExitProcess() is called by the game, we know
	// the threads are already shutdown, so don't try to do it again here.
#if !__RGSC_DLL
	this->Shutdown();
#endif
}

bool
sysThreadPool::Init()
{
    bool success = false;

    SYS_CS_SYNC(m_Cs);

    if(Verifyf(!m_Initialized, "Threadpool already initialized"))
    {
        Assert(m_QueuedWorkItems.empty());
        Assert(m_Threads.empty());

        m_NextId = 0;

        m_WorkAvailableSema = sysIpcCreateSema(0);
        if(Verifyf(m_WorkAvailableSema, "Could not create semaphore"))
        {
            m_Initialized = true;
            success = true;
        }
    }

    return success;
}

void
sysThreadPool::Shutdown()
{
    SYS_CS_SYNC(m_Cs);

    if(m_Initialized)
    {
        while(!m_QueuedWorkItems.empty())
        {
            this->CancelWork(m_QueuedWorkItems.front()->m_Id);
        }

        while(!m_Threads.empty())
        {
            this->RemoveThread(m_Threads.front());
        }

        sysIpcDeleteSema(m_WorkAvailableSema);
        m_Initialized = false;
    }
}

bool
sysThreadPool::AddThread(Thread* th, const char* name, const int cpuAffinity, unsigned int stackSize, rage::sysIpcPriority threadPriority)
{
    bool success = false;

    SYS_CS_SYNC(m_Cs);

    if(Verifyf(m_Initialized, "Threadpool not initialized yet")
        && Verifyf(NULL == th->m_Owner, "Threadpool already has an owner"))
    {
        Assert(sysIpcThreadIdInvalid == th->m_ThreadId);

        m_Threads.push_back(th);
        th->m_Owner = this;
        th->m_State = Thread::STATE_RUNNING;
        th->m_WaitSema = sysIpcCreateSema(0);
        th->m_Stop = false;

        if(Verifyf(th->m_WaitSema, "Could not create semaphore"))
        {
            th->m_ThreadId = 
                sysIpcCreateThread(&sysThreadPool::ThreadFunc,
                                    th,
                                    stackSize,
                                    threadPriority,
                                    name,
                                    cpuAffinity);
        }

        if(sysIpcThreadIdInvalid != th->m_ThreadId)
        {
            success = true;
        }
        else
        {
            m_Threads.erase(th);
            th->m_Owner = NULL;
            th->m_State = Thread::STATE_NONE;
        }
    }

    return success;
}

bool
	sysThreadPool::ResetThread(Thread* th)
{
	bool success = false;

	SYS_CS_SYNC(m_Cs);

	if(	Verifyf(m_Initialized, "Threadpool not initialized")
		&& Verifyf(this == th->m_Owner, "Removing a thread that is not owned by this threadpool")
		)
	{
		
		th->m_Stop = true;
		m_Threads.erase(th);
		th->m_Owner = NULL;
		th->m_ThreadId = sysIpcThreadIdInvalid;
		th->m_WaitSema = 0;
		th->m_State = Thread::STATE_STOPPED;
		success = true;
	}

	return success;
}

bool
sysThreadPool::RemoveThread(Thread* th)
{
    bool success = false;

    SYS_CS_SYNC(m_Cs);

    if(Verifyf(m_Initialized, "Threadpool not initialized")
        && Verifyf(this == th->m_Owner, "Removing a thread that is not owned by this threadpool")
        && Verifyf(!th->m_Stop, "Removing a thread that is still busy"))
    {
        Assert(Thread::STATE_RUNNING == th->m_State);
        Assert(th->m_WaitSema);

        th->m_Stop = true;
        //Wait for the thread to stop.
        sysIpcWaitSema(th->m_WaitSema);

		m_Threads.erase(th);
		sysIpcWaitThreadExit(th->m_ThreadId);
        th->m_Owner = NULL;
        th->m_ThreadId = sysIpcThreadIdInvalid;
        sysIpcDeleteSema(th->m_WaitSema);
        th->m_WaitSema = 0;
        th->m_State = Thread::STATE_STOPPED;
        success = true;
    }

    return success;
}

bool
sysThreadPool::QueueWork(WorkItem* workItem)
{
    AssertMsg(m_Threads.size() > 0, "No worker threads have been registered");

    bool success = false;

    SYS_CS_SYNC(m_Cs);

    if(Verifyf(m_Initialized, "Threadpool not initialized")
        && Verifyf(!workItem->Pending(), "Work item is already pending"))
    {
        m_QueuedWorkItems.push_back(workItem);

        //Assign a unique non-zero id.
        do
        {
            workItem->m_Id = ++m_NextId;
        } while(INVALID_WORK_ITEM_ID == workItem->m_Id);

        workItem->m_State = WorkItem::STATE_QUEUED;
        workItem->m_WasCanceled = false;

        sysIpcSignalSema(m_WorkAvailableSema);

        success = true;
    }

    return success;
}

size_t
sysThreadPool::NumQueuedWorkItems()
{
	SYS_CS_SYNC(m_Cs);
	return m_QueuedWorkItems.size();
}

size_t
sysThreadPool::NumActiveWorkItems()
{
	SYS_CS_SYNC(m_Cs);
	return m_ActiveWorkItems.size();
}

bool
sysThreadPool::CancelWork(const unsigned workItemId)
{
    bool success = false;

    SYS_CS_SYNC(m_Cs);

    WorkItem* workItem = this->FindWorkItem(workItemId);

    if(workItem)
    {
        Assert(INVALID_WORK_ITEM_ID != workItemId);
        Assert(workItem->Pending());

        if(WorkItem::STATE_QUEUED == workItem->m_State)
        {
            m_QueuedWorkItems.erase(workItem);
        }
        else
        {
            Assert(WorkItem::STATE_ACTIVE == workItem->m_State);

            m_ActiveWorkItems.erase(workItem);
        }

        workItem->m_WasCanceled = true;
		workItem->Cancel();

        success = true;
    }

    return success;
}

//private:

sysThreadPool::WorkItem*
sysThreadPool::FindWorkItem(const unsigned workItemId)
{
    SYS_CS_SYNC(m_Cs);

    WorkItem* workItem = NULL;

    WorkItems::iterator it = m_QueuedWorkItems.begin();
    WorkItems::const_iterator stop = m_QueuedWorkItems.end();

    for(; stop != it; ++it)
    {
        if((*it)->GetId() == workItemId)
        {
            workItem = *it;
            break;
        }
    }

    if(!workItem)
    {
        it = m_ActiveWorkItems.begin();
        stop = m_ActiveWorkItems.end();

        for(; stop != it; ++it)
        {
            if((*it)->GetId() == workItemId)
            {
                workItem = *it;
                break;
            }
        }
    }

    return workItem;
}

void
sysThreadPool::ThreadFunc(void* p)
{
    Thread* th = (Thread*) p;
    sysThreadPool* threadPool = th->m_Owner;

    while(!th->m_Stop)
    {
        if(sysIpcWaitSemaTimed(threadPool->m_WorkAvailableSema, 1000))
        {
            //sysIpcWaitSemaTimed returned true indicating it didn't time out
            //and there's work available.
            unsigned workItemId = 0;

            if(th->m_Stop)
            {
                //We need to stop, but we've already committed to doing
                //work.  We need to re-signal the sema so some other
                //thread can do the work.
                sysIpcSignalSema(threadPool->m_WorkAvailableSema);
            }
            else
            {
                SYS_CS_SYNC(threadPool->m_Cs);
                if(!threadPool->m_QueuedWorkItems.empty())
                {
                    WorkItem* workItem = threadPool->m_QueuedWorkItems.front();
                    threadPool->m_QueuedWorkItems.pop_front();
                    threadPool->m_ActiveWorkItems.push_back(workItem);

                    Assert(WorkItem::STATE_QUEUED == workItem->m_State);
                    workItem->m_State = WorkItem::STATE_ACTIVE;
                    workItemId = workItem->GetId();
                }
            }

            if(workItemId)
            {
                WorkItem* workItem = threadPool->FindWorkItem(workItemId);

                if(workItem)
                {
                    workItem->DoWork();

                    SYS_CS_SYNC(threadPool->m_Cs);
                    Assert(WorkItem::STATE_ACTIVE == workItem->m_State);
                    if(!workItem->WasCanceled())
                    {
                        threadPool->m_ActiveWorkItems.erase(workItem);
                    }
                    workItem->m_State = WorkItem::STATE_FINISHED;

					if (!workItem->WasCanceled())
					{
						workItem->OnFinished();
					}
                }
            }
        }
    }

    sysIpcSignalSema(th->m_WaitSema);
}

}; //namespace rage
