// 
// system/barrier.h 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

// NOTE - This code doesn't seem to be actually used anywhere.  If it is used, it needs to be ported to Orbis.

#ifndef SYSTEM_BARRIER_H 
#define SYSTEM_BARRIER_H 

#if __PS3
#include <cell/spurs/barrier.h>
#endif

namespace rage {

// PURPOSE: When sysBarrier::Wait is called from multiple threads, they will all be
// suspended until all of them have called Wait. When all of them have called Wait, 
// they will all be resumed simultaneously.
// NOTES:
//     - One thing this is useful for is a parallel algorithm where the computation
//       occurs in rounds, each of which needs to wait for the results of the previous
//       round before it begins.
//     - Generally sysBarrier can help with any computational dependency, such as when several
//       computations need to contribute their output before some other computation can begin.
//     - The PS3 version is intended to be used for blocking SPURS tasks or jobs. It
//       provides the same functionality as CellSpursBarrier.
class sysBarrier
{
public:
	// PURPOSE: Initialize the internal semaphore and generally prepare to be used
	// PARAMS:
	//     count - The number of SPUS/threads to use
	//     useSpinLocks - If true, use a spin lock instead of a semaphore.
	// NOTES:
	//     - do not have more threads/SPUs waiting on the barrier than "count"
	//     - Init cannot be performed from SPU
	//     - Only use the spin lock if your wait should be very short in duration.
#if __WIN32
	void Init(int count, bool useSpinLocks = false);
#elif __PPU
	void Init(int count, CellSpursTaskset* taskSet);
#endif

	// PURPOSE: Destroy the internal semaphore and generally prepare to be deleted
	// NOTES:
	//     - Init cannot be performed from SPU
#if !__SPU
	void Shutdown();
#endif

	// PURPOSE: Set the number of threads that will need to wait on this barrier.
	// NOTES: Only valid to call this while there are no threads waiting at the barrier.
	void SetCount(int count);

	// PURPOSE: This function will not return until all the threads/SPUs have called Wait.
	// RETURNS: true is returned one of the threads/SPUs each frame, false is returned to the others
	// NOTES:
	//     - The return value is useful if you want to have one of your threads performing
	//       a special duty each round.
	//     - You must have the same number of threads call Wait as the count you specified
	//       in Init.
#if __WIN32 || __SPU
	void Wait();
#endif

private:

#if __PS3
	CellSpursBarrier m_Barrier;
	CellSpursTaskset* m_TaskSet;
#elif __WIN32
	u32 m_CurrentCount;
	u32 m_InitialCount;
	u32 m_Step;

    bool m_UseSpinLocks;

    u32 m_Breached[2];
	void* m_Semaphores[2];
#endif

};

} // namespace rage

#endif // SYSTEM_BARRIER_H 
