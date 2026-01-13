// 
// system/barrier.cpp 
// 
// Copyright (C) 1999-2008 Rockstar Games.  All Rights Reserved. 
// 

#include "barrier.h"

#if !__PSP2

#if __WIN32
#include "xtl.h"
#endif

#include "interlocked.h"

namespace rage {

#if __WIN32
void sysBarrier::Init(int count, bool useSpinLocks)
{
	Assert(count > 0);
	m_UseSpinLocks = useSpinLocks;
	m_CurrentCount = m_InitialCount = count;
	if (useSpinLocks)
	{
	    m_Breached[0] = m_Breached[1] = 0;
		m_Semaphores[0] = m_Semaphores[1] = NULL;
	}
	else
	{
#if RSG_DURANGO
		m_Semaphores[0] = CreateSemaphoreExW(NULL, 0, count, NULL, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);
		m_Semaphores[1] = CreateSemaphoreExW(NULL, 0, count, NULL, 0, SYNCHRONIZE | SEMAPHORE_MODIFY_STATE);
#else
		m_Semaphores[0] = CreateSemaphore(NULL, 0, count, NULL);
		m_Semaphores[1] = CreateSemaphore(NULL, 0, count, NULL);
#endif
		Assert(m_Semaphores[0] && m_Semaphores[1]);
	}
}
#elif __PPU
void sysBarrier::Init(int count, CellSpursTaskset* taskSet)
{
	m_TaskSet = taskSet;
	ASSERT_ONLY(int result =) cellSpursBarrierInitialize(taskSet, &m_Barrier, count);
	Assertf(result == CELL_OK, "Result was %d, should have been CELL_OK(%d)", result, CELL_OK);
}
#endif

#if !__SPU
void sysBarrier::Shutdown()
{
#if __WIN32
	if (!m_UseSpinLocks)
	{
		CloseHandle(m_Semaphores[0]);
		CloseHandle(m_Semaphores[1]);
	}
#endif
}
#endif

void sysBarrier::SetCount(int count)
{
#if __WIN32
	Assertf(m_InitialCount == m_CurrentCount, "Invalid attempt to change the barrier count from %d to %d while %d tasks are waiting on the barrier already.", m_InitialCount, count, m_InitialCount - m_CurrentCount);

	m_InitialCount = m_CurrentCount = count;
#elif __PPU
	ASSERT_ONLY(int result =) cellSpursBarrierInitialize(m_TaskSet, &m_Barrier, count);
	Assert(result == CELL_OK);
#endif
}

#if __WIN32 || __SPU
void sysBarrier::Wait()
{
#if __SPU
	while (cellSpursBarrierTryNotify(uint64_t(&m_Barrier)) != CELL_OK) { }
	while (cellSpursBarrierTryWait(uint64_t(&m_Barrier)) != CELL_OK) { }
#else
	u32 step = m_Step;

    if (sysInterlockedDecrement(&m_CurrentCount) == 0)
    {
		m_CurrentCount = m_InitialCount;

        if (m_InitialCount > 1)
        {
            if (m_UseSpinLocks)
            {
                sysInterlockedAdd(&m_Breached[step], m_InitialCount - 1);
            }
            else
            {
                ReleaseSemaphore(m_Semaphores[step], m_InitialCount - 1, NULL);
            }
        }
    }
    else
    {
        if (m_UseSpinLocks)
        {
            u32 stopper = 8;
            while (m_Breached[step] == 0) {
                volatile u32 spinner = 0;
                while (spinner < stopper)
                    ++spinner;
                if (stopper < 1024)
                    stopper <<= 1;
                XENON_ONLY(__lwsync());
				YieldProcessor();
            }

            sysInterlockedDecrement(&m_Breached[step]);
        }
        else
        {
            WaitForSingleObject(m_Semaphores[step], INFINITE);
        }
    }

	sysInterlockedExchange(&m_Step, 1 - step);
#endif
}
#endif

} // namespace rage

#endif		// !__PSP2