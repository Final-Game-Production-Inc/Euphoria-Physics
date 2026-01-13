// 
// system/threadpool.h
// 
// Copyright (C) 1999-2012 Rockstar Games.  All Rights Reserved. 
// 

#ifndef SYSTEM_THREADPOOL_H
#define SYSTEM_THREADPOOL_H

#include "atl/inlist.h"
#include "system/criticalsection.h"
#include "system/ipc.h"


namespace rage
{

//PURPOSE
//  Manages a pool of threads that are available to do work.  Work
//  is submitted in the form of a work item.  When a thread becomes
//  available it pulls a work item from the queue of work items
//  and calls the DoWork() virtual function.  When DoWork() returns
//  the thread sets its state to FINISHED and waits for another
//  work item to arrive on the queue.
class sysThreadPool
{
public:

    static const unsigned INVALID_WORK_ITEM_ID  = 0;

    //PURPOSE
    //  Implements the worker thread that pulls items from the work
    //  queue and calls their DoWork() function.
    //  Instances of Thread are registered with an instance of sysThreadPool.
    //  Each registered instance of thread runs in a separate thread.
    //  Threads are registered with a cpu affinity which provides a hint
    //  to which CPU on which the thread can run.
    class Thread
    {
        friend class sysThreadPool;

    public:

        Thread();

        ~Thread();

        //PURPOSE
        //  Returns true when the thread is running.  A thread will
        //  begin running when it has been registered with a thread pool.
        bool Running() const;

        //PURPOSE
        //  Returns true when the thread has been stopped.  A thread is
        //  stopped when it is unregistered from a thread pool
        bool Stopped() const;

		//PURPOSE
		// REturns true if the thread is still alive. A thread is considered
		// not alive if it's been forced close / killed by the user
		// utilizing Process Explorer / etc
		bool IsAlive() const;

    private:

        enum State
        {
            STATE_NONE,
            STATE_RUNNING,
            STATE_STOPPED
        };

        sysThreadPool* m_Owner;

        sysIpcThreadId m_ThreadId;

        State m_State;

        //Used to wait for the thread to stop when it's been
        //unregistered from a thread pool.
        sysIpcSema m_WaitSema;

        inlist_node<Thread> m_ListLink;

        //Set to true to request the thread to stop.
        bool m_Stop         : 1;
    };

	//PURPOSE
	//  Work items are queued to the thread pool in order to do work.
	//  Subclasses must implement DoWork().
	//  DoWork() is called once from a thread and when it returns it
	//  is assumed the work is finished.
	//
	//  A work item's DoWork() implementations should check WasCanceled()
	//  at every opportunity, and if it returns true then abort
	//  any pending operations if possible.
	class WorkItem
	{
		friend class sysThreadPool;

	public:

		WorkItem();

		virtual ~WorkItem();

		//PURPOSE
		//  Must be overridden by subclasses to do work.
		virtual void DoWork() = 0;

		//PURPOSE
		//  Returns the unique id of the work item.  The id is
		//  assigned when the work item is queued to the thread
		//  pool.
		unsigned GetId() const;

		//PURPOSE
		//  Returns true while the work item is pending.
		//NOTE
		//  DO NOT destroy the work item until Pending() returns false
		//  regardless of whether it has been canceled.
		//
		//  Pending() might continue to return true even after the
		//  work item has been canceled in order to permit the work
		//  item to finish any non-interruptible operations.
		bool Pending() const;
		bool Active() const;
		bool Queued() const;

		//PURPOSE
		//  Returns true when the work item is finished.
		bool Finished() const;

		//PURPOSE
		//  Returns true if the work item was canceled.
		//NOTE
		//  DO NOT destroy the work item until Pending() returns false
		//  regardless of whether it has been canceled.
		//
		//  Pending() might continue to return true even after the
		//  work item has been canceled in order to permit the work
		//  item to finish any non-interruptible operations.
		//
		//  A work item's DoWork() implementations should check WasCanceled()
		//  at every opportunity, and if it returns true then abort
		//  any pending operations if possible.
		bool WasCanceled() const;

		//PURPOSE
		//  Called when cancelled
		virtual void Cancel() {}

	protected:
		
		//PURPOSE
		//  Call to complete the task
		virtual void Finish();

	private:

		// PURPOSE:
		//	Called after the work is complete (not cancelled) and the work item has been removed from the active list
		//  The thread pool will no longer reference the work item after this call
		virtual void OnFinished() {}

		enum State
		{
			STATE_NONE,
			STATE_QUEUED,
			STATE_ACTIVE,
			STATE_FINISHED,
		};

		State m_State;

		unsigned m_Id;

		inlist_node<WorkItem> m_ListLink;

		bool m_WasCanceled  : 1;
	};

	sysThreadPool();

	~sysThreadPool();

	//PURPOSE
	//  Initialize the thread pool
	bool Init();

	//PURPOSE
	//  Shut down the thread pool.
	void Shutdown();

	//PURPOSE
	//  Add a worker thread.
	//PARAMS
	//  th              - The thread instance.
	//  name            - Name of the thread.
	//  cpuAffinitiy    - Hint as to the CPU on which the thread should run.
	//  threadPriority  - Priority of the thread to excute
	//NOTE
	//  Worker threads should be added before QueueWork() is called.
	bool AddThread(Thread* th, const char* name, const int cpuAffinity, unsigned int stackSize = sysIpcMinThreadStackSize, rage::sysIpcPriority threadPriority = PRIO_NORMAL);

	//PURPOSE
	//  Reset a worker thread.
	//NOTES
	//  This is necessary for the scenario where a thread has been forcefully killed by the operating system
	//  and we need to clean up the state and re-initialize it.
	bool ResetThread(Thread* th);

	//PURPOSE
	//  Remove a worker thread.
	//NOTES
	//  If the thread is currently running a work item then this function
	//  will block until the work item has finished.
	bool RemoveThread(Thread* th);

	//PURPOSE
	//  Queues a work item to be run.
	bool QueueWork(WorkItem* workItem);

	//PURPOSE
	//  Cancels a work item.
	//NOTES
	//  When canceled a work item might be in the queue, or it might be
	//  running.  CancelWork() will return immediately but the work item
	//  will not finish unless it explicitly handles being canceled, and
	//  it has finished any non-interruptible operations.
	//
	//  A work item's DoWork() implementations should check WasCanceled()
	//  at every opportunity, and if it returns true then abort
	//  any pending operations if possible.
	bool CancelWork(const unsigned workItemId);

	// PURPOSE
	//	Returns the number of threads in the pool
	size_t NumThreads() { return m_Threads.size(); }

	// PURPOSE
	//	Returns the number of queued work items.
	size_t NumQueuedWorkItems();

	// PURPOSE
	//	Returns the number of active work items.
	size_t NumActiveWorkItems();

private:

	WorkItem* FindWorkItem(const unsigned workItemId);

	typedef inlist<WorkItem, &WorkItem::m_ListLink> WorkItems;

	typedef inlist<Thread, &Thread::m_ListLink> Threads;

	sysCriticalSectionToken m_Cs;
	WorkItems m_QueuedWorkItems;
	WorkItems m_ActiveWorkItems;
	Threads m_Threads;
	unsigned m_NextId;

	sysIpcSema m_WorkAvailableSema;

	static void ThreadFunc(void* p);

	bool m_Initialized  : 1;
};

} //namespace rage

#endif //SYSTEM_THREADPOOL_H
