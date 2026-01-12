#ifndef _PHYSICS_PROFILE_CAPTURE_H_
#define _PHYSICS_PROFILE_CAPTURE_H_

#include "phcore/constants.h"

#if USE_PHYSICS_PROFILE_CAPTURE

namespace rage
{
	struct PhysicsCaptureStat;

	const int PPC_FRAME_START = 60 * 2;
	const int PPC_FRAME_END = 60 * 6;

	void PhysicsCaptureStat_TimerStart(PhysicsCaptureStat * stat);
	void PhysicsCaptureStat_TimerStop(PhysicsCaptureStat * stat);
	void PhysicsCaptureStat_CounterInc(PhysicsCaptureStat * stat, const u32 count);

	#undef PPC_STAT_INC
	#define PPC_STAT_INC(name,label,type,firstFrame,lastFrame) extern PhysicsCaptureStat g_##name;
	#include "physics/physicsprofilecapture.inc"
	#undef PPC_STAT_INC

	struct PhysicsCaptureStatScoped
	{
		PhysicsCaptureStat * m_stat;
		PhysicsCaptureStatScoped(PhysicsCaptureStat * stat) : m_stat(stat) { PhysicsCaptureStat_TimerStart(m_stat); }
		~PhysicsCaptureStatScoped() { PhysicsCaptureStat_TimerStop(m_stat); }
	};

	void PhysicsCaptureStart();
	bool PhysicsCaptureIsRunning();
	void PhysicsCaptureFrameStart();
	void PhysicsCaptureFrameEnd();

}; // namespace rage

#define PPC_STAT_TIMER_START(name) rage::PhysicsCaptureStat_TimerStart(&rage::g_##name); 
#define PPC_STAT_TIMER_STOP(name) rage::PhysicsCaptureStat_TimerStop(&rage::g_##name); 
#define PPC_STAT_TIMER_SCOPED(name) rage::PhysicsCaptureStatScoped name##scoped(&rage::g_##name);
#define PPC_STAT_COUNTER_INC(name,count) rage::PhysicsCaptureStat_CounterInc(&rage::g_##name,count); 

#else // USE_PHYSICS_PROFILE_CAPTURE

namespace rage
{
	inline void PhysicsCaptureStart() {}
	inline bool PhysicsCaptureIsRunning() { return false; }
	inline void PhysicsCaptureFrameStart() {}
	inline void PhysicsCaptureFrameEnd() {}
}; // namespace rage

#define PPC_STAT_TIMER_START(name)
#define PPC_STAT_TIMER_STOP(name)
#define PPC_STAT_TIMER_SCOPED(name)
#define PPC_STAT_COUNTER_INC(name,count)

#endif // USE_PHYSICS_PROFILE_CAPTURE

#endif // _PHYSICS_PROFILE_CAPTURE_H_

