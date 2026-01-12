#include <math.h>
#include "profile/element.h"
#include "physicsprofilecapture.h"

#if USE_PHYSICS_PROFILE_CAPTURE
namespace rage
{

#if 0
	#define PPC_PRINTF printf
	#define PPC_NEWLINE printf("\n");
#else
	#define PPC_PRINTF Displayf
	#define PPC_NEWLINE 
#endif

struct PhysicsCaptureStat
{
	enum 
	{
		TYPE_TIMER = 0,
		TYPE_COUNTER = 1,
		MAX_CAPTURE_SIZE = 60*10,

		CAPTURE_STATE_OFF = 0,
		CAPTURE_STATE_RUNNING = 1,
		CAPTURE_STATE_FINISHED = 2,

		MAX_STAT_LIST_SIZE = 3,
	};

	const char * m_name;
	int m_type;
	u64 m_FrameCaputureList[MAX_CAPTURE_SIZE];
	int m_curFrameCapture;
	int m_minFrameCapture;
	int m_maxFrameCapture;
	int m_captureState;

	u64 m_tickStart;
	u64 m_TickTotal;
	bool m_inTimer;
	bool m_inFrame;

	float m_statTotal;
	float m_statAverage;
	float m_statSTD;
	float m_statMax[MAX_STAT_LIST_SIZE];
	int m_statFirstFrame;
	int m_statLastFrame;

	PhysicsCaptureStat(const char * name, int type, int minFrameCapture, int maxFrameCapture) : m_name(name), m_type(type), m_minFrameCapture(minFrameCapture), m_maxFrameCapture(maxFrameCapture)
	{
		FastAssert(m_maxFrameCapture > 0 && m_maxFrameCapture <= MAX_CAPTURE_SIZE);
		FastAssert(m_type == TYPE_TIMER || m_type == TYPE_COUNTER);
		m_curFrameCapture = 0;
		m_captureState = CAPTURE_STATE_OFF;
		m_inTimer = false;
		m_inFrame = false;
	}

	void CalcStats(const int firstFrame, const int lastFrame)
	{
		double total_ = 0.0;
		double average_ = 0.0;
		double std_ = 0.0;
		for (int i = 0 ; i < MAX_STAT_LIST_SIZE ; i++)
			m_statMax[i] = 0.0f;
		m_statFirstFrame = firstFrame;
		m_statLastFrame = lastFrame;
		FastAssert(m_statFirstFrame >= 0);
		FastAssert(m_statFirstFrame <= m_statLastFrame);
		FastAssert(m_statLastFrame <= m_maxFrameCapture);
		const int numFrames = lastFrame - firstFrame;
		if (numFrames > 0)
		{
			double tempList[MAX_CAPTURE_SIZE];
#ifdef CONST_FREQ
			const double TicksToMilliseconds = (1000.0 / double(CONST_FREQ));
#else
			const double TicksToMilliseconds = double(sysTimer::GetTicksToMilliseconds());
#endif 
			for (int i = m_statFirstFrame ; i < m_statLastFrame ; i++)
			{
				if (m_type == TYPE_TIMER)
					tempList[i] = double(m_FrameCaputureList[i]) * TicksToMilliseconds;
				else
				{
					FastAssert(m_type == TYPE_COUNTER);
					tempList[i] = double(m_FrameCaputureList[i]);
				}
				total_ += tempList[i];

				// Insert into the max stat list.
				FastAssert(MAX_STAT_LIST_SIZE > 0);
				const float tempf = float(tempList[i]);
				int maxi = MAX_STAT_LIST_SIZE - 1;
				if (tempf > m_statMax[maxi])
				{
					m_statMax[maxi] = tempf;
					while (maxi > 0 && tempf > m_statMax[maxi-1])
					{
						const int maxi_ = maxi - 1;
						m_statMax[maxi] = m_statMax[maxi_];
						m_statMax[maxi_] = tempf;
						maxi = maxi_;
					}
				}
			}
			average_ = total_ / double(numFrames);
			for (int i = m_statFirstFrame ; i < m_statLastFrame ; i++)
			{
				const double temp = tempList[i] - average_; 
				std_ += temp * temp;
			}
			std_ = sqrt(std_ / double(numFrames));
		}
		m_statTotal = float(total_);
		m_statAverage = float(average_);
		m_statSTD = float(std_);
	}

	void CalcStats()
	{
		CalcStats(m_minFrameCapture,m_maxFrameCapture);
	}

	void DisplayStats()
	{
		FastAssert(MAX_STAT_LIST_SIZE == 3);
		static const char * type[] = {"Timer","Counter"};
		PPC_PRINTF("%s %s(%d,%d): %4.3f, %4.3f, %4.3f, [%4.3f, %4.3f, %4.3f]",m_name,type[m_type],m_statFirstFrame,m_statLastFrame,m_statTotal,m_statAverage,m_statSTD,m_statMax[0],m_statMax[1],m_statMax[2]);
		PPC_NEWLINE;
	}

	void StartCapture()
	{
		FastAssert(m_maxFrameCapture <= MAX_CAPTURE_SIZE);
		FastAssert(m_inTimer == false);
		FastAssert(m_inFrame == false);
		FastAssert(m_maxFrameCapture > 0);
		m_curFrameCapture = 0;
		m_captureState = CAPTURE_STATE_RUNNING;
	}

	void FrameStart()
	{
		FastAssert(m_inFrame == false);
		FastAssert(m_inTimer == false);
		m_inFrame = true;
		if (m_captureState == CAPTURE_STATE_RUNNING)
		{
			m_TickTotal = 0;
		}
	}

	void FrameEnd()
	{
		FastAssert(m_inFrame == true);
		FastAssert(m_inTimer == false);
		m_inFrame = false;
		if (m_captureState == CAPTURE_STATE_RUNNING)
		{
			FastAssert(m_maxFrameCapture <= MAX_CAPTURE_SIZE);
			FastAssert(m_curFrameCapture < m_maxFrameCapture);
			FastAssert(m_curFrameCapture >= 0);
			m_FrameCaputureList[m_curFrameCapture] = m_TickTotal;
			m_curFrameCapture++;
			if (m_curFrameCapture == m_maxFrameCapture)
				m_captureState = CAPTURE_STATE_FINISHED;
		}
	}

	void TimerStart()
	{
		if (m_inFrame == true)
		{
			FastAssert(m_type == TYPE_TIMER);
			FastAssert(m_inTimer == false);
			m_inTimer = true;
			if (m_captureState == CAPTURE_STATE_RUNNING)
			{
				m_tickStart = sysTimer::GetTicks();
			}
		}
	}

	void TimerStop()
	{
		if (m_inFrame == true)
		{
			FastAssert(m_type == TYPE_TIMER);
			FastAssert(m_inTimer == true);
			m_inTimer = false;
			if (m_captureState == CAPTURE_STATE_RUNNING)
			{
				const u64 tickEnd = sysTimer::GetTicks();
				m_TickTotal += tickEnd - m_tickStart;
			}
		}
	}

	void CounterIncrement(const u32 count)
	{
		if (m_inFrame == true)
		{
			FastAssert(m_type == TYPE_COUNTER);
			FastAssert(m_inTimer == false);
			if (m_captureState == CAPTURE_STATE_RUNNING)
			{
				m_TickTotal += u64(count);
			}
		}
	}
};

#undef PPC_STAT_INC
#define PPC_STAT_INC(name,label,type,firstFrame,lastFrame) PhysicsCaptureStat g_##name(label,PhysicsCaptureStat::type,firstFrame,lastFrame);
#include "physics/physicsprofilecapture.inc"

static PhysicsCaptureStat * g_captureStatList[] = {
#undef PPC_STAT_INC
#define PPC_STAT_INC(name,label,type,firstFrame,lastFrame) &g_##name,
#include "physics/physicsprofilecapture.inc"
#undef PPC_STAT_INC
};

static int g_captureStatListSize = sizeof(g_captureStatList) / sizeof(PhysicsCaptureStat*);

void PhysicsCaptureStart()
{
	PPC_PRINTF("***** Physics Capture Start *****");
	PPC_NEWLINE;
	for (int i = 0 ; i < g_captureStatListSize ; i++)
		g_captureStatList[i]->StartCapture();
}

bool PhysicsCaptureIsRunning()
{
	for (int i = 0 ; i < g_captureStatListSize ; i++)
	{
		if (g_captureStatList[i]->m_captureState == PhysicsCaptureStat::CAPTURE_STATE_RUNNING)
			return true;
	}
	return false;
}


void PhysicsCaptureFrameStart()
{
#if USE_DETERMINISTIC_ORDERING
	PHMANIFOLD->SortFreeSlots();
	PHCONTACT->SortFreeSlots();
	PHCOMPOSITEPOINTERS->SortFreeSlots();
#endif // USE_DETERMINISTIC_ORDERING

	for (int i = 0 ; i < g_captureStatListSize ; i++)
		g_captureStatList[i]->FrameStart();

}

void PhysicsCaptureFrameEnd()
{
	bool allFinished = true;
	for (int i = 0 ; i < g_captureStatListSize ; i++)
	{
		g_captureStatList[i]->FrameEnd();
		if (g_captureStatList[i]->m_captureState != PhysicsCaptureStat::CAPTURE_STATE_FINISHED)
			allFinished = false;
	}
	if (allFinished)
	{
		for (int i = 0 ; i < g_captureStatListSize ; i++)
		{
			g_captureStatList[i]->m_captureState = PhysicsCaptureStat::CAPTURE_STATE_OFF;
			g_captureStatList[i]->CalcStats();
			g_captureStatList[i]->DisplayStats();
		}
		PPC_PRINTF("***** Physics Capture End *****");
		PPC_NEWLINE;
	}
}

void PhysicsCaptureStat_TimerStart(PhysicsCaptureStat * stat) {	stat->TimerStart();	}
void PhysicsCaptureStat_TimerStop(PhysicsCaptureStat * stat) { stat->TimerStop(); }
void PhysicsCaptureStat_CounterInc(PhysicsCaptureStat * stat, const u32 count) { stat->CounterIncrement(count);	}

} // namepsace rage

#endif // USE_PHYSICS_PROFILE_CAPTURE
